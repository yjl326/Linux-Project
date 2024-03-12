/*************************************************************
 * File  :  hello_rtsa.c
 * Module:  Agora SD-RTN SDK RTC C API demo application.
 *
 * This is a part of the Agora RTC Service SDK.
 * Copyright (C) 2020 Agora IO
 * All rights reserved.
 *
 *************************************************************/
#include "app_config.h"
#include <pthread.h>

pthread_mutex_t audio_data_lock = PTHREAD_MUTEX_INITIALIZER; /* 静态初始化 */

FILE *play_pcm = NULL;
pthread_t cap_thread, play_thread;
uint8_t g_play_thread_start = 0;
uint8_t g_play_flag = 0;

typedef struct {
  app_config_t config;

  void *audio_file_parser;
  void *audio_file_writer;

  connection_id_t conn_id;
  bool b_stop_flag;
  bool b_connected_flag;
  bool b_joined_flag;
  bool b_capture_play_flag;
} app_t;

static app_t g_app = {
    .config = {
        // common config
        .p_sdk_log_dir              = "io.agora.rtc_sdk",
        .p_appid                    = "",
        .p_channel                  = DEFAULT_CHANNEL_NAME,
        .p_token                    = "",
		    .p_license                  = "",
        .uid                        = 0,
        .area                       = AREA_CODE_GLOB,

        // audio related 
        .audio_data_type            = AUDIO_DATA_TYPE_PCM,
        .audio_codec_type           = AUDIO_CODEC_TYPE_OPUS,
        .send_audio_file_path       = DEFAULT_SEND_AUDIO_FILENAME,

        // pcm related config
        .pcm_sample_rate            = DEFAULT_PCM_SAMPLE_RATE,
        .pcm_channel_num            = DEFAULT_PCM_CHANNEL_NUM,

        // advanced config
        .enable_audio_mixer         = false,
        .receive_data_only          = false,
		    .local_ap                   = "",
		    .domain_limit               = false,
    },

    .audio_file_parser      = NULL,
    .audio_file_writer      = NULL,

    .b_stop_flag            = false,
    .b_connected_flag       = false,
    .b_joined_flag          = false,
    .b_capture_play_flag     = false,
};

static void app_signal_handler(int sig)
{
  switch (sig) {
  case SIGINT:
    g_app.b_stop_flag = true;
    break;
  default:
    LOGW("no handler, sig=%d", sig);
  }
}

#define AUDIO_BUFFER_SIZE 4096
#define one_frame_len parser_cfg.u.audio_cfg.sampleRateHz * parser_cfg.u.audio_cfg.framePeriodMs / 1000 * parser_cfg.u.audio_cfg.numberOfChannels * sizeof(int16_t)

parser_cfg_t parser_cfg = { 0 };
snd_pcm_t *playback_handle;
snd_pcm_t *capture_handle;
uint8_t audio_buffer[640] = {0};
// fifo_t *audio_fifo;

#define AUDIO_DATA_LEN 640
#define AUDIO_DATA_CNT 120
uint8_t* audio_data_buffer[AUDIO_DATA_CNT] = {NULL};
uint8_t g_write_idx = 0;
uint8_t g_read_idx = 0;
uint8_t g_buffer_size = 0;

void create_buffer()
{
    for (int i=0; i<AUDIO_DATA_CNT; i++)
        audio_data_buffer[i] = (uint8_t *)malloc(AUDIO_DATA_LEN);
    g_write_idx = 0;
    g_read_idx = 0;
    g_buffer_size = 0;
}

void write_buffer(uint8_t *buff)
{
    pthread_mutex_lock(&audio_data_lock);
    if (g_buffer_size < AUDIO_DATA_CNT)
    {
        memcpy(audio_data_buffer[g_write_idx], buff, AUDIO_DATA_LEN);
        g_write_idx++;
        g_buffer_size++;
        /*回环写*/
        if (g_write_idx >= AUDIO_DATA_CNT)
            g_write_idx = 0;
        /*限制越界*/
        if (g_buffer_size >= AUDIO_DATA_CNT)
            g_buffer_size = AUDIO_DATA_CNT-1;
    }
    pthread_mutex_unlock(&audio_data_lock);
}

uint8_t read_buffer(uint8_t *buff)
{
    pthread_mutex_lock(&audio_data_lock);
    if (g_buffer_size > 0)
    {
        memcpy(buff, audio_data_buffer[g_read_idx], AUDIO_DATA_LEN);
        g_read_idx++;
        g_buffer_size--;
        /*回环写*/
        if (g_read_idx >= AUDIO_DATA_CNT)
            g_read_idx = 0;
        /*限制越界*/
        if (g_buffer_size <= 0)
            g_buffer_size = 0;

        pthread_mutex_unlock(&audio_data_lock);
        return 1;
    }
    pthread_mutex_unlock(&audio_data_lock);
    return 0;
}

static int app_init(void)
{
    app_config_t *config = &g_app.config;
    
    signal(SIGINT, app_signal_handler);

    if (config->audio_data_type == AUDIO_DATA_TYPE_PCM) {
        if (config->audio_codec_type == AUDIO_CODEC_TYPE_G711U) {
            config->pcm_sample_rate = 8000;
            config->send_audio_file_path = DEFAULT_8K_SEND_AUDIO_FILENAME;
        }
        parser_cfg.u.audio_cfg.sampleRateHz = config->pcm_sample_rate;
        parser_cfg.u.audio_cfg.numberOfChannels = config->pcm_channel_num;
        parser_cfg.u.audio_cfg.framePeriodMs = DEFAULT_SEND_AUDIO_FRAME_PERIOD_MS;
    }

    // g_app.audio_file_parser =
    //         create_file_parser(audio_data_type_to_file_type(config->audio_data_type),
    //                           config->send_audio_file_path, &parser_cfg);
    // if (!g_app.audio_file_parser) {
    //    return -1;
    // }

    // g_app.audio_file_writer = create_file_writer(DEFAULT_RECV_AUDIO_BASENAME);

    return 0;
}

snd_pcm_t *open_sound_dev(snd_pcm_stream_t type)
{
  int err;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *hw_params;
	unsigned int rate = 16000;
	int audio_channels = 1;

/*
int snd_pcm_open(snd_pcm_t **pcmp, const char *name, snd_pcm_stream_t stream, int mode)

pcmp 打开的pcm句柄
name 要打开的pcm设备名字，默认default，或者从asound.conf或者asoundrc里面选择所要打开的设备
stream SND_PCM_STREAM_PLAYBACK 或 SND_PCM_STREAM_CAPTURE，分别表示播放和录音的PCM流
mode 打开pcm句柄时的一些附加参数 SND_PCM_NONBLOCK 非阻塞打开（默认阻塞打开）, SND_PCM_ASYNC 异步模式打开
返回值 0 表示打开成功，负数表示失败，对应错误码
*/

  if ((err = snd_pcm_open (&handle, "plughw:1,0", type, 0)) < 0) {
    return NULL;
  }
/*
snd_pcm_hw_params_malloc( ) 在栈中分配 snd_pcm_hw_params_t 结构的空间，然后使用 snd_pcm_hw_params_any( ) 函数用声卡的全配置空间参数初始化已经分配的 snd_pcm_hw_params_t 结构。snd_pcm_hw_params_set_access ( ) 设置访问类型，常用访问类型的宏定义有：

SND_PCM_ACCESS_RW_INTERLEAVED 
交错访问。在缓冲区的每个 PCM 帧都包含所有设置的声道的连续的采样数据。比如声卡要播放采样长度是 16-bit 的 PCM 立体声数据，表示每个 PCM 帧中有 16-bit 的左声道数据，然后是 16-bit 右声道数据。

SND_PCM_ACCESS_RW_NONINTERLEAVED 
非交错访问。每个 PCM 帧只是一个声道需要的数据，如果使用多个声道，那么第一帧是第一个声道的数据，第二帧是第二个声道的数据，依此类推。

函数 snd_pcm_hw_params_set_format() 设置数据格式，主要控制输入的音频数据的类型、无符号还是有符号、是 little-endian 还是 bit-endian。比如对于 16-bit 长度的采样数据可以设置为：	
*/   
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror (err));
		return NULL;
	}
			 
	if ((err = snd_pcm_hw_params_any (handle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_access (handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {// 交错访问
		fprintf (stderr, "cannot set access type (%s)\n", snd_strerror (err));
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_format (handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {  //  SND_PCM_FORMAT_S16_LE      有符号16 bit Little Endian
		fprintf (stderr, "cannot set sample format (%s)\n", snd_strerror (err));
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_rate_near (handle, hw_params, &rate, 0)) < 0) {    //snd_pcm_hw_params_set_rate_near () 函数设置音频数据的最接近目标的采样率
		fprintf (stderr, "cannot set sample rate (%s)\n", snd_strerror (err));
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_channels (handle, hw_params, audio_channels)) < 0) {
		fprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (err));
		return NULL;
	}
  //snd_pcm_hw_params( ) 从设备配置空间选择一个配置，让函数 snd_pcm_prepare() 准备好 PCM 设备，以便写入 PCM 数据。
	if ((err = snd_pcm_hw_params (handle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err));
		return NULL;
	}

	snd_pcm_hw_params_free (hw_params);

	return handle;
}

void close_sound_dev(snd_pcm_t *handle)
{
	snd_pcm_close (handle);
}

snd_pcm_t *open_playback(void)
{
  return open_sound_dev(SND_PCM_STREAM_PLAYBACK);
}

snd_pcm_t *open_capture(void)
{
  return open_sound_dev(SND_PCM_STREAM_CAPTURE);
}

int soundcard_prepare(void) 
{
  int err;

  playback_handle = open_playback();
  if (!playback_handle) {
    fprintf (stderr, "cannot open for playback\n");
    return -1;
  }

  capture_handle = open_capture();
  if (!capture_handle) {
    fprintf (stderr, "cannot open for capture\n");
    return -1;
  }

	if ((err = snd_pcm_prepare (playback_handle)) < 0) {
    fprintf (stderr, "cannot prepare audio interface for use (%s)\n",snd_strerror (err));
    return -1;
	}

	if ((err = snd_pcm_prepare (capture_handle)) < 0) {
    fprintf (stderr, "cannot prepare audio interface for use (%s)\n",snd_strerror (err));
    return -1;
	}

  return 0;
}

int catpture_pcm(void)
{
    int err;
    uint8_t cap_buf[4096];
    int cap_frame_num = 320;

    app_config_t *config = &g_app.config;

    audio_frame_info_t info = { 0 };
    info.data_type = config->audio_data_type;

    /*注册信号捕获退出接口*/
    signal(SIGINT, app_signal_handler);

    while ((!g_app.b_stop_flag) && g_app.b_joined_flag) {
        if ((err = snd_pcm_readi (capture_handle, cap_buf, cap_frame_num)) != cap_frame_num) {
            fprintf (stderr, "read from audio interface failed (%s)\n",snd_strerror (err));
            return -1;
        }

        int rval = agora_rtc_send_audio_data(g_app.conn_id, cap_buf, cap_frame_num * 2, &info);

        if (rval < 0) {
            LOGE("Failed to send audio data, reason: %s", agora_rtc_err_2_str(rval));
            return -1;
        }

        if (g_app.b_stop_flag) { 
            printf("stop!\n");
            break;
        }
    }

    snd_pcm_close (capture_handle);
    LOGE("catpture_pcm exit");

    return 0;
}

int play_recv_pcm(void)
{
    int err = 0; 
    int play_frame_num = 320;

    while ((!g_app.b_stop_flag) && g_app.b_joined_flag) {
    
        uint8_t play_buf[640] = {0};
        int ret = read_buffer(play_buf);
        if (ret == 0) {
            continue;
        }

        if ((err = snd_pcm_writei (playback_handle, play_buf, play_frame_num)) != play_frame_num) {   //snd_pcm_writei() 用来把交错的音频数据写入到音频设备。
            if (err < 0)
                err = snd_pcm_recover(playback_handle, err, 0);
            if (err < 0) {
                fprintf (stderr, "write to audio interface failed (%s)\n",snd_strerror (err));
                g_play_thread_start = 0;
                return -1;
            }
            
        }

        if (g_app.b_stop_flag) {
            printf("stop!\n");
            break;
        }
    }

    snd_pcm_close (playback_handle);
    LOGE("play_recv_pcm exit");

    return 0;
}

void init_sound_device(void)
{
    g_play_thread_start = 1;
    soundcard_prepare();
    pthread_create(&play_thread, NULL, (void *)play_recv_pcm, NULL);
    pthread_create(&cap_thread, NULL, (void *)catpture_pcm, NULL);
}

static void __on_join_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed)
{
    g_app.b_connected_flag = true;
    connection_info_t conn_info = { 0 };
    agora_rtc_get_connection_info(g_app.conn_id, &conn_info);
    LOGI("[conn-%u] Join the channel %s successfully, uid %u elapsed %d ms", conn_id,
        conn_info.channel_name, uid, elapsed);
}

static void __on_connection_lost(connection_id_t conn_id)
{
    g_app.b_connected_flag = false;
    LOGW("[conn-%u] Lost connection from the channel", conn_id);
}

static void __on_rejoin_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed_ms)
{
    g_app.b_connected_flag = true;
    LOGI("[conn-%u] Rejoin the channel successfully, uid %u elapsed %d ms", conn_id, uid, elapsed_ms);
}

static void __on_user_joined(connection_id_t conn_id, uint32_t uid, int elapsed_ms)
{
    g_app.b_joined_flag = true;
    LOGI("[conn-%u] Remote user \"%u\" has joined the channel, elapsed %d ms", conn_id, uid,
      elapsed_ms);

    if (g_app.b_capture_play_flag == false) {
        g_app.b_capture_play_flag = true;
        soundcard_prepare();
        pthread_create(&play_thread, NULL, (void *)play_recv_pcm, NULL);
        pthread_create(&cap_thread, NULL, (void *)catpture_pcm, NULL);
    }
    
}

static void __on_user_offline(connection_id_t conn_id, uint32_t uid, int reason)
{
    g_app.b_joined_flag = false;
    pthread_join(cap_thread, NULL);
    pthread_join(play_thread, NULL);
    g_app.b_capture_play_flag = false;
    LOGI("[conn-%u] Remote user \"%u\" has left the channel, reason %d", conn_id, uid, reason);
}

static void __on_user_mute_audio(connection_id_t conn_id, uint32_t uid, bool muted)
{
    LOGI("[conn-%u] audio: uid=%u muted=%d", conn_id, uid, muted);
}

static void __on_user_mute_video(connection_id_t conn_id, uint32_t uid, bool muted)
{
    LOGI("[conn-%u] video: uid=%u muted=%d", conn_id, uid, muted);
}

static void __on_error(connection_id_t conn_id, int code, const char *msg)
{
    if (code == ERR_SEND_VIDEO_OVER_BANDWIDTH_LIMIT) {
        LOGE("Not enough uplink bandwdith. Error msg \"%s\"", msg);
        return;
    }

    if (code == ERR_INVALID_APP_ID) {
        LOGE("Invalid App ID. Please double check. Error msg \"%s\"", msg);
    } else if (code == ERR_INVALID_CHANNEL_NAME) {
        LOGE("Invalid channel name for conn_id %u. Please double check. Error msg \"%s\"", conn_id,
            msg);
    } else if (code == ERR_INVALID_TOKEN || code == ERR_TOKEN_EXPIRED) {
        LOGE("Invalid token. Please double check. Error msg \"%s\"", msg);
    } else if (code == ERR_DYNAMIC_TOKEN_BUT_USE_STATIC_KEY) {
        LOGE("Dynamic token is enabled but is not provided. Error msg \"%s\"", msg);
    } else {
        LOGW("Error %d is captured. Error msg \"%s\"", code, msg);
    }

    g_app.b_stop_flag = true;
}

static void __on_license_failed(connection_id_t conn_id, int reason)
{
    LOGE("License verified failed, reason: %d", reason);
    g_app.b_stop_flag = true;
}

static void __on_audio_data(connection_id_t conn_id, const uint32_t uid, uint16_t sent_ts,
                            const void *data, size_t len, const audio_frame_info_t *info_ptr)
{
//   LOGD("[conn-%u] on_audio_data, uid %u sent_ts %u data_type %d, len %zu", conn_id, uid, sent_ts,
//        info_ptr->data_type, len);
  // write_file(g_app.audio_file_writer, info_ptr->data_type, data, len);
  
    // write_fifo(audio_fifo, (uint8_t *)data, len);
    write_buffer((uint8_t *)data);

    /*收到数据后才创建播放线程*/
    // if (g_play_thread_start == 0) {
    //     // pthread_create(&play_thread, NULL, (void *)play_recv_pcm, NULL);
    //     init_sound_device();
    // }
}

static void __on_mixed_audio_data(connection_id_t conn_id, const void *data, size_t len,
                                  const audio_frame_info_t *info_ptr)
{
    LOGD("[conn-%u] on_mixed_audio_data, data_type %d, len %zu", conn_id, info_ptr->data_type, len);
    // write_file(g_app.audio_file_writer, info_ptr->data_type, data, len);
}

static void __on_target_bitrate_changed(connection_id_t conn_id, uint32_t target_bps)
{
    LOGI("[conn-%u] Bandwidth change detected. Please adjust encoder bitrate to %u kbps", conn_id,
        target_bps / 1000);
}

static void __on_key_frame_gen_req(connection_id_t conn_id, uint32_t uid,
                                   video_stream_type_e stream_type)
{
    LOGW("[conn-%u] Frame loss detected. Please notify the encoder to generate key frame immediately",
        conn_id);
}

static void app_init_event_handler(agora_rtc_event_handler_t *event_handler, app_config_t *config)
{
    event_handler->on_join_channel_success = __on_join_channel_success;
    event_handler->on_connection_lost = __on_connection_lost;
    event_handler->on_rejoin_channel_success = __on_rejoin_channel_success;
    event_handler->on_user_joined = __on_user_joined;
    event_handler->on_user_offline = __on_user_offline;
    event_handler->on_user_mute_audio = __on_user_mute_audio;
    event_handler->on_user_mute_video = __on_user_mute_video;
    event_handler->on_target_bitrate_changed = __on_target_bitrate_changed;
    event_handler->on_key_frame_gen_req = __on_key_frame_gen_req;
    event_handler->on_error = __on_error;
    event_handler->on_license_validation_failure = __on_license_failed;

    if (config->enable_audio_mixer) {
        event_handler->on_mixed_audio_data = __on_mixed_audio_data;
    } else {
        event_handler->on_audio_data = __on_audio_data;
    }
}

int main(int argc, char **argv)
{
    app_config_t *config = &g_app.config;
    int rval;
    char params[512];

    // 1. app parse args
    rval = app_parse_args(argc, argv, config);
    if (rval < 0) {
        app_print_usage(argc, argv);
        return -1;
    }

    LOGI("Welcome to RTSA SDK v%s", agora_rtc_get_version());

    // 2. app init
    rval = app_init();
    if (rval < 0) {
        LOGE("Failed to initialize application");
        return -1;
    }
    create_buffer();
    // audio_fifo = fifo_create(MAX_FIFO_SIZE);
    // soundcard_prepare();

    // 4. API: init agora rtc sdk
    int appid_len = strlen(config->p_appid);
    void *p_appid = (void *)(appid_len == 0 ? NULL : config->p_appid);

    agora_rtc_event_handler_t event_handler = { 0 };
    app_init_event_handler(&event_handler, config);

    rtc_service_option_t service_opt = { 0 };
    service_opt.area_code = config->area;
    service_opt.log_cfg.log_path = config->p_sdk_log_dir;
    service_opt.log_cfg.log_level = RTC_LOG_DEBUG;
    snprintf(service_opt.license_value, sizeof(service_opt.license_value), "%s", config->p_license);

    rval = agora_rtc_init(p_appid, &event_handler, &service_opt);
    if (rval < 0) {
        LOGE("Failed to initialize Agora sdk, reason: %s", agora_rtc_err_2_str(rval));
        return -1;
    }

    rval = agora_rtc_set_bwe_param(CONNECTION_ID_ALL, DEFAULT_BANDWIDTH_ESTIMATE_MIN_BITRATE,
                                    DEFAULT_BANDWIDTH_ESTIMATE_MAX_BITRATE,
                                    DEFAULT_BANDWIDTH_ESTIMATE_START_BITRATE);
    if (rval != 0) {
        LOGE("Failed set bwe param, reason: %s", agora_rtc_err_2_str(rval));
        return -1;
    }

    if (config->local_ap && config->local_ap[0]) {
        snprintf(params, sizeof(params), "{\"rtc.local_ap_list\": %s}", config->local_ap);
        rval = agora_rtc_set_params(params);
        if (rval != 0) {
            LOGE("set local_ap_list %s failed, reason: %s", config->local_ap, agora_rtc_err_2_str(rval));
            return -1;
        }
    }

    // 5. API: Create connection
    rval = agora_rtc_create_connection(&g_app.conn_id);
    if (rval < 0) {
        LOGE("Failed to create connection, reason: %s", agora_rtc_err_2_str(rval));
        return -1;
    }

    // 6. API: join channel
    int token_len = strlen(config->p_token);
    void *p_token = (void *)(token_len == 0 ? NULL : config->p_token);

    rtc_channel_options_t channel_options;
    memset(&channel_options, 0, sizeof(channel_options));
    channel_options.auto_subscribe_audio = true;
    channel_options.auto_subscribe_video = true;
    channel_options.enable_audio_mixer = config->enable_audio_mixer;

    if (config->audio_data_type == AUDIO_DATA_TYPE_PCM) {
        /* If we want to send PCM data instead of encoded audio like AAC or Opus, here please enable
            audio codec, as well as configure the PCM sample rate and number of channels */
        channel_options.audio_codec_opt.audio_codec_type = config->audio_codec_type;
        channel_options.audio_codec_opt.pcm_sample_rate = config->pcm_sample_rate;
        channel_options.audio_codec_opt.pcm_channel_num = config->pcm_channel_num;
    }

    rval = agora_rtc_join_channel(g_app.conn_id, config->p_channel, config->uid, p_token,
                                    &channel_options);
    if (rval < 0) {
        LOGE("Failed to join channel \"%s\", reason: %s", config->p_channel, agora_rtc_err_2_str(rval));
        return -1;
    }

//   if ((play_pcm = fopen("send_audio_16k_1ch.pcm", "rb")) == NULL) {
//     printf("打开音频文件%s失败", "send_audio_16k_1ch.pcm");
//     exit(1);
//   }

    // 7. wait until we join channel successfully
    while (!g_app.b_connected_flag && !g_app.b_stop_flag) {
        usleep(100 * 1000);
    }

    // 8. send video and audio data in a loop
    int audio_send_interval_ms = DEFAULT_SEND_AUDIO_FRAME_PERIOD_MS;
    void *pacer = pacer_create(audio_send_interval_ms, 10);
    
    // pthread_create(&cap_thread, NULL, (void *)catpture_pcm, NULL);
    //   pthread_create(&play_thread, NULL, (void *)play_recv_pcm, NULL);
    while (!g_app.b_stop_flag) {
        sleep(3);
    }

    pthread_join(cap_thread, NULL);
    pthread_join(play_thread, NULL);

    // 9. API: leave channel
    agora_rtc_leave_channel(g_app.conn_id);

    // 10: API: Destroy connection
    agora_rtc_destroy_connection(g_app.conn_id);

    // 11. API: fini rtc sdk
    agora_rtc_fini();

    // 12. app clean up
    pacer_destroy(pacer);

    // fifo_release(audio_fifo);
    pthread_mutex_destroy(&audio_data_lock);

    return 0;
}
