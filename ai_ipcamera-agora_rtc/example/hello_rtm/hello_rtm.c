/*************************************************************
 * File  :  hello_rtsa.c
 * Module:  Agora SD-RTN SDK RTC C API demo application.
 *
 * This is a part of the Agora RTC Service SDK.
 * Copyright (C) 2020 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "agora_rtc_api.h"
#include "utility.h"
#include "log.h"

#define TAG_APP "[app]"
#define TAG_API "[api]"
#define TAG_EVENT "[event]"

#define INVALID_FD -1

#define DEFAULT_RTM_SEND_FILENAME "send_video.h264"
#define DEFAULT_RTM_RECV_FILENAME "rtm_recv_file.h264"
#define DEFAULT_RTM_SEND_SIZE (32 * 1024)
#define DEFAULT_RTM_SEND_INTERVAL_MS (100)
#define MIN_RTM_SEND_INTERVAL_MS (17)
#define MIN_RTM_SEND_MBPS (1)
#define MAX_RTM_SEND_MBPS (8)

typedef struct {
  const char *p_sdk_log_dir;

  const char *p_appid;
  const char *p_token;
	const char *p_license;
  const char *p_peer_uid;
  const char *p_rtm_uid;

  int32_t rtm_role;
  int32_t rtm_send_size;
  int32_t rtm_send_mbps;
  int32_t rtm_send_enable_flag;
  int32_t rtm_send_interval_ms;
  const char *rtm_send_file_path;
  int32_t rtm_recv_dump_flag;
  const char *rtm_recv_file_path;
} app_config_t;

typedef struct {
  app_config_t config;

  int32_t rtm_send_file_fd;
  int32_t rtm_recv_file_fd;

  int32_t b_stop_flag;
  int32_t b_rtm_login_success_flag;
} app_t;

static app_t g_app_instance = {
	.config =
			{
					.p_sdk_log_dir = "io.agora.rtc_sdk",
					.p_appid = "",
					.p_token = NULL,
					.p_license = "",
					.p_peer_uid = "peer",
					.p_rtm_uid = "user",

					.rtm_role = 3,
					.rtm_send_size = DEFAULT_RTM_SEND_SIZE,
					.rtm_send_mbps = 1,
					.rtm_send_interval_ms = DEFAULT_RTM_SEND_INTERVAL_MS,
					.rtm_send_enable_flag = 1,
					.rtm_send_file_path = DEFAULT_RTM_SEND_FILENAME,
					.rtm_recv_dump_flag = 1,
					.rtm_recv_file_path = DEFAULT_RTM_RECV_FILENAME,
			},

	.rtm_send_file_fd = INVALID_FD,
	.rtm_recv_file_fd = INVALID_FD,

	.b_stop_flag = 0,
	.b_rtm_login_success_flag = 0,
};

app_t *app_get_instance(void)
{
  return &g_app_instance;
}

static void app_signal_handler(int32_t sig)
{
  app_t *p_app = app_get_instance();
  switch (sig) {
  case SIGQUIT:
  case SIGABRT:
  case SIGINT:
    p_app->b_stop_flag = 1;
    break;
  default:
    LOGW("no handler, sig=%d", sig);
  }
}

void app_print_usage(int32_t argc, char **argv)
{
  LOGS("\nUsage: %s [OPTION]", argv[0]);
  LOGS(" -h, --help               : show help info");
  LOGS(" -i, --appId              : application id, either appId OR token MUST be set");
  LOGS(" -t, --token              : token for authentication");
  LOGS(" -l, --license            : license value MUST be set when release");
  LOGS(" -p, --peerUid            : peer uid, default is './%s'", "peer");
  LOGS(" -R, --rtmUid             : rtm uid, default is './%s'", "user");
  LOGS(" -o, --role               : 0: send and receive file, 1: send file, 2: receive file, 3: send text, default is '3'");
  LOGS(" -f, --sendFile           : file to send, default is './%s'", DEFAULT_RTM_SEND_FILENAME);
  LOGS(" -F, --recvFile           : file received to save, default is './%s'",
       DEFAULT_RTM_RECV_FILENAME);
  LOGS(" -s, --sendSize           : send data length, default is '%d'", DEFAULT_RTM_SEND_SIZE);
  LOGS(" -S, --sendFreq           : send data frequency, default is '%dms per frame'",
       DEFAULT_RTM_SEND_INTERVAL_MS);
  LOGS(" -b, --mbps               : send data mbps, default is '%d'", 1);
  LOGS("\nExample:");
  LOGS("    %s --appId xxx [--token xxx] --rtmUid xxx --peerUid xxx --role xxx --sendFile xxx --recvFile xxx --sendSize xxx --sendFreq xxx",
       argv[0]);
}

int32_t app_parse_args(app_config_t *p_config, int32_t argc, char **argv)
{
  const char *av_short_option = "hi:t:l:f:p:R:o:F:s:S:b:";
  const struct option av_long_option[] = { { "help", 0, NULL, 'h' },
                                           { "appId", 1, NULL, 'i' },
                                           { "token", 1, NULL, 't' },
                                           { "license", 1, NULL, 'l' },
                                           { "peerUid", 1, NULL, 'p' },
                                           { "rtmUid", 1, NULL, 'R' },
                                           { "role", 1, NULL, 'o' },
                                           { "sendFile", 1, NULL, 'f' },
                                           { "recvFile", 1, NULL, 'F' },
                                           { "sendSize", 1, NULL, 's' },
                                           { "sendFreq", 1, NULL, 'S' },
                                           { "mbps", 1, NULL, 'b' },
                                           { 0, 0, 0, 0 } };

  int32_t ch = -1;
  int32_t optidx = 0;
  int32_t rval = 0;

  while (1) {
    optidx++;
    ch = getopt_long(argc, argv, av_short_option, av_long_option, NULL);
    if (ch == -1) {
      break;
    }

    switch (ch) {
    case 'h': {
      rval = -1;
      goto EXIT;
    } break;
    case 'i': {
      p_config->p_appid = optarg;
    } break;
    case 't': {
      p_config->p_token = optarg;
    } break;
    case 'l': {
      p_config->p_license = optarg;
    } break;
    case 'p': {
      p_config->p_peer_uid = optarg;
    } break;
    case 'R': {
      p_config->p_rtm_uid = optarg;
    } break;
    case 'o': {
      p_config->rtm_role = strtol(optarg, NULL, 10);
    } break;
    case 'f': {
      p_config->rtm_send_file_path = optarg;
    } break;
    case 'F': {
      p_config->rtm_recv_file_path = optarg;
    } break;
    case 's': {
      p_config->rtm_send_size = strtol(optarg, NULL, 10);
    } break;
    case 'b': {
      p_config->rtm_send_mbps = strtol(optarg, NULL, 10);
    } break;
    default: {
      rval = -1;
      LOGS("%s parse cmd param: %s error.", TAG_APP, argv[optidx]);
      goto EXIT;
    }
    }
  }

  // check key parameters
  if (strcmp(p_config->p_appid, "") == 0) {
    rval = -1;
    LOGE("%s appid MUST be provided", TAG_APP);
    goto EXIT;
  }

  if (!p_config->rtm_send_file_path || strcmp(p_config->rtm_send_file_path, "") == 0 ||
      !p_config->rtm_recv_file_path || strcmp(p_config->rtm_recv_file_path, "") == 0) {
    rval = -1;
    LOGE("%s invalid rtm send recv path", TAG_APP);
    goto EXIT;
  }

  if (!p_config->p_peer_uid || strcmp(p_config->p_peer_uid, "") == 0) {
    rval = -1;
    LOGE("%s invalid peer uid", TAG_APP);
    goto EXIT;
  }

  if (!p_config->p_rtm_uid || strcmp(p_config->p_rtm_uid, "") == 0) {
    rval = -1;
    LOGE("%s invalid rtm uid", TAG_APP);
    goto EXIT;
  }

  if (p_config->rtm_role < 0 || p_config->rtm_role > 3) {
    LOGE("Invalid rtm role");
    rval = -1;
    goto EXIT;
  }

  if (p_config->rtm_send_size > DEFAULT_RTM_SEND_SIZE || p_config->rtm_send_size <= 0) {
    p_config->rtm_send_size = DEFAULT_RTM_SEND_SIZE;
    LOGW("RTM send size should between 1 and 32kb");
  }

  if (p_config->rtm_send_mbps < MIN_RTM_SEND_MBPS || p_config->rtm_send_mbps > MAX_RTM_SEND_MBPS) {
    p_config->rtm_send_mbps = MIN_RTM_SEND_MBPS;
  }
  LOGS("RTM send %d mbps", p_config->rtm_send_mbps);

  p_config->rtm_send_interval_ms =
          p_config->rtm_send_size * 8 * 1000 / (p_config->rtm_send_mbps * 1000 * 1000);
  LOGS("RTM send one frame per %dms", p_config->rtm_send_interval_ms);

EXIT:
  return rval;
}

static int32_t app_init(app_t *p_app)
{
  int32_t rval = 0;

  signal(SIGQUIT, app_signal_handler);
  signal(SIGABRT, app_signal_handler);
  signal(SIGINT, app_signal_handler);

  app_config_t *p_config = &p_app->config;

  if (p_config->rtm_role == 0) {
    p_config->rtm_send_enable_flag = 1;
    p_config->rtm_recv_dump_flag = 1;
  } else if (p_config->rtm_role == 1) {
    p_config->rtm_send_enable_flag = 1;
    p_config->rtm_recv_dump_flag = 0;
  } else if (p_config->rtm_role == 2) {
    p_config->rtm_send_enable_flag = 0;
    p_config->rtm_recv_dump_flag = 2;
  } else if (p_config->rtm_role == 3) {
    p_config->rtm_send_enable_flag = 0;
    p_config->rtm_recv_dump_flag = 0;
  } else {
    LOGE("Invalid rtm role");
    return -1;
  }

  if (p_config->rtm_send_enable_flag) {
    p_app->rtm_send_file_fd = open(p_app->config.rtm_send_file_path, O_RDONLY);
    if (INVALID_FD == p_app->rtm_send_file_fd) {
      LOGE("%s parser open send file:%s failed", TAG_APP, p_config->rtm_send_file_path);
      rval = -1;
      goto EXIT;
    }
  }

  if (p_config->rtm_recv_dump_flag) {
    p_app->rtm_recv_file_fd =
            open(p_config->rtm_recv_file_path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (p_app->rtm_recv_file_fd == INVALID_FD) {
      rval = -1;
      LOGE("%s open audio dump file: %s failed:%d", TAG_APP, p_config->rtm_recv_file_path,
           p_app->rtm_recv_file_fd);
      goto EXIT;
    }
  }

EXIT:
  return rval;
}

static void app_deinit(app_t *p_app)
{
  if (p_app->rtm_send_file_fd > 0) {
    close(p_app->rtm_send_file_fd);
    p_app->rtm_send_file_fd = INVALID_FD;
  }

  if (p_app->rtm_recv_file_fd > 0) {
    close(p_app->rtm_recv_file_fd);
    p_app->rtm_recv_file_fd = INVALID_FD;
  }

  p_app->b_rtm_login_success_flag = 0;
  p_app->b_stop_flag = 0;
}

static int32_t app_send_rtm(app_t *p_app)
{
  static uint32_t message_id = 0;
  static char buffer[DEFAULT_RTM_SEND_SIZE] = { 0 };
  memset(buffer, 0, sizeof(buffer));
  int32_t rval = read(p_app->rtm_send_file_fd, buffer, p_app->config.rtm_send_size);
  if (rval != p_app->config.rtm_send_size) {
    if (rval > 0) {
      // send the rest data
      LOGI("send message_id %d", message_id);
      rval = agora_rtc_send_rtm_data(p_app->config.p_peer_uid, ++message_id, buffer, rval);
      if (rval < 0) {
        LOGE("%s send data failed, rval=%d", TAG_API, rval);
        goto EXIT;
      }
    } else {
      rval = -1;
      LOGE("%s read file failed, rval=%d ", TAG_APP, rval);
    }
    goto EXIT;
  }

  LOGI("send message_id %d", message_id);
  rval = agora_rtc_send_rtm_data(p_app->config.p_peer_uid, ++message_id, buffer, rval);
  if (rval < 0) {
    LOGE("%s send data failed, rval=%d", TAG_API, rval);
    goto EXIT;
  }
EXIT:
  return rval;
}

static int32_t app_send_rtm_message(app_t *p_app)
{
  return 0;
}

uint64_t app_get_time_ms()
{
  struct timeval tv;
  if (gettimeofday(&tv, NULL) < 0) {
    return 0;
  }
  return (((uint64_t)tv.tv_sec * (uint64_t)1000) + tv.tv_usec / 1000);
}

void app_sleep_ms(int64_t ms)
{
  usleep(ms * 1000);
}

static void __on_rtm_data(const char *user_id, const void *data, size_t data_len)
{
  app_t *p_app = app_get_instance();
  app_config_t *p_config = &p_app->config;

  if (p_config->rtm_role == 3) {
    LOGD("Receive data[%s] from user[%s] length[%lu]", (char *)data, user_id, data_len);
  } else {
    LOGD("data_callback %s data[], length[%lu]", user_id, data_len);
  }

  if (p_app->config.rtm_recv_dump_flag && p_app->rtm_recv_file_fd != INVALID_FD) {
    if (write(p_app->rtm_recv_file_fd, data, data_len) != data_len) {
      LOGE("write error");
      return;
    }
  }
}

static void __on_rtm_event(const char *user_id, uint32_t event_id, uint32_t event_code)
{
  LOGD("%s event id[%u], event code[%u]", user_id, event_id, event_code);
  if (event_id == 0 && event_code == 0) {
    app_t *p_app = app_get_instance();
    p_app->b_rtm_login_success_flag = 1;
  }
}

static void __on_rtm_send_data_res(uint32_t msg_id, uint32_t error_code)
{
  LOGD("msg id [%u], error_code[%u]", msg_id, error_code);
}

static agora_rtc_event_handler_t event_handler = {
  .on_join_channel_success = NULL,
  .on_error = NULL,
  .on_user_joined = NULL,
  .on_user_offline = NULL,
  .on_audio_data = NULL,
  .on_video_data = NULL,
  .on_key_frame_gen_req = NULL,
  .on_target_bitrate_changed = NULL,
  .on_connection_lost = NULL,
  .on_rejoin_channel_success = NULL,
};

static agora_rtm_handler_t rtm_handler = {
  .on_rtm_data = __on_rtm_data,
  .on_rtm_event = __on_rtm_event,
  .on_send_rtm_data_result = __on_rtm_send_data_res,
};

int32_t main(int32_t argc, char **argv)
{
  app_t *p_app = app_get_instance();
  app_config_t *p_config = &p_app->config;

  // 0. app parse args
  int32_t rval = app_parse_args(p_config, argc, argv);
  if (rval != 0) {
    app_print_usage(argc, argv);
    goto EXIT;
  }

  LOGS("%s Welcome to RTSA SDK v%s", TAG_APP, agora_rtc_get_version());

  // 1. app init
  rval = app_init(p_app);
  if (rval < 0) {
    LOGE("%s init failed, rval=%d", TAG_APP, rval);
    goto EXIT;
  }

  // 2. API: init agora rtc sdk
  int32_t appid_len = strlen(p_config->p_appid);
  void *p_appid = (void *)(appid_len == 0 ? NULL : p_config->p_appid);
  rtc_service_option_t service_opt;
  memset(&service_opt, 0, sizeof(service_opt));
  service_opt.area_code = AREA_CODE_GLOB;
  service_opt.log_cfg.log_path = p_config->p_sdk_log_dir;
	snprintf(service_opt.license_value, sizeof(service_opt.license_value), "%s", p_config->p_license);
  rval = agora_rtc_init(p_appid, &event_handler, &service_opt);
  if (rval < 0) {
    LOGE("%s agora sdk init failed, rval=%d error=%s", TAG_API, rval, agora_rtc_err_2_str(rval));
    goto EXIT;
  }

  // 3. API:
  rval = agora_rtc_login_rtm(p_config->p_rtm_uid, p_config->p_token, &rtm_handler);
  if (rval < 0) {
    LOGE("login rtm failed");
    goto EXIT;
  }

  // 4. wait until rtm login success or Ctrl-C trigger stop
  while (1) {
    if (p_app->b_stop_flag || p_app->b_rtm_login_success_flag) {
      break;
    }
    app_sleep_ms(10);
  }

  // 5. rtm transmit loop
  int64_t rtm_predict_time_ms = 0;
  int64_t sleep_ms = 0;

  while (!p_app->b_stop_flag) {
    int64_t cur_time_ms = app_get_time_ms();

    if (p_config->rtm_send_enable_flag) {
      if (rtm_predict_time_ms == 0) {
        rtm_predict_time_ms = cur_time_ms;
      }
      if (cur_time_ms >= rtm_predict_time_ms) {
        if (app_send_rtm(p_app) < 0) {
          LOGD("Stop send rtm");
          p_app->b_stop_flag = 1;
        }
        rtm_predict_time_ms += p_config->rtm_send_interval_ms;
      }
    }

    if (p_config->rtm_send_enable_flag) {
      sleep_ms = p_config->rtm_send_interval_ms - cur_time_ms;
    } else {
      sleep_ms = 100;
    }

    if (p_config->rtm_role == 3) {
      char tmp[32 * 1024 + 1] = { 0 };
      static uint32_t msg_id = 0;
      LOGD("Send to user id[%s], please enter the message:", p_app->config.p_peer_uid);
      if (fgets((char *)tmp, 32 * 1024, stdin) != NULL) {
        LOGD("Content length[%d] buf[%s]", (int)strlen(tmp), tmp);
        agora_rtc_send_rtm_data(p_app->config.p_peer_uid, ++msg_id, tmp, strlen(tmp));
      }
    }

    if (sleep_ms > 0) {
      app_sleep_ms(sleep_ms);
    }
  }

  // 6. API: logout rtm
  agora_rtc_logout_rtm();

  // 7. API: fini rtc sdk
  agora_rtc_fini();

EXIT:
  // 8. app deinit
  app_deinit(p_app);
  return rval;
}
