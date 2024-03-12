// #include <stdio.h>
// #include <stdlib.h>
// #include <alsa/asoundlib.h>

// int main() {
//     int err;
//     snd_pcm_t *capture_handle, *playback_handle;
//     snd_pcm_hw_params_t *hw_params;
//     snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
//     unsigned int rate = 8000;
//     int channels = 2;
//     char *capture_device = "plughw:1,0";
//     char *playback_device = "plughw:1,0";

//     // 打开录音设备
//     if ((err = snd_pcm_open(&capture_handle, capture_device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
//         fprintf(stderr, "Cannot open capture audio device %s (%s)\n", capture_device, snd_strerror(err));
//         return 1;
//     }

//     // 打开播放设备
//     if ((err = snd_pcm_open(&playback_handle, playback_device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
//         fprintf(stderr, "Cannot open playback audio device %s (%s)\n", playback_device, snd_strerror(err));
//         return 1;
//     }

//     // 分配硬件参数对象
//     if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
//         fprintf(stderr, "Cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
//         return 1;
//     }

//     // 初始化硬件参数
//     if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
//         fprintf(stderr, "Cannot initialize hardware parameter structure (%s)\n", snd_strerror(err));
//         return 1;
//     }

//     // 设置硬件参数
//     if ((err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
//         fprintf(stderr, "Cannot set access type (%s)\n", snd_strerror(err));
//         return 1;
//     }

//     if ((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, format)) < 0) {
//         fprintf(stderr, "Cannot set sample format (%s)\n", snd_strerror(err));
//         return 1;
//     }

//     if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate, 0)) < 0) {
//         fprintf(stderr, "Cannot set sample rate (%s)\n", snd_strerror(err));
//         return 1;
//     }

//     if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, channels)) < 0) {
//         fprintf(stderr, "Cannot set channel count (%s)\n", snd_strerror(err));
//         return 1;
//     }

//     // 应用硬件参数设置
//     if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
//         fprintf(stderr, "Cannot set parameters (%s)\n", snd_strerror(err));
//         return 1;
//     }

//     // 清空硬件参数对象
//     snd_pcm_hw_params_free(hw_params);

//     // 准备录音设备
//     if ((err = snd_pcm_prepare(capture_handle)) < 0) {
//         fprintf(stderr, "Cannot prepare audio interface for recording (%s)\n", snd_strerror(err));
//         return 1;
//     }

//     // 准备播放设备
//     if ((err = snd_pcm_prepare(playback_handle)) < 0) {
//         fprintf(stderr, "Cannot prepare audio interface for playback (%s)\n", snd_strerror(err));
//         return 1;
//     }

//     // 执行录音和播放
//     while (1) {
//         char buffer[4096];

//         // 从录音设备读取数据
//         if ((err = snd_pcm_readi(capture_handle, buffer, sizeof(buffer) / (2 * channels))) < 0) {
//             fprintf(stderr, "Read from capture error: %s\n", snd_strerror(err));
//             break;
//         }

//         // 将数据写入播放设备
//         if ((err = snd_pcm_writei(playback_handle, buffer, sizeof(buffer) / (2 * channels))) < 0) {
//             fprintf(stderr, "Write to playback error: %s\n", snd_strerror(err));
//             break;
//         }
//     }

//     // 关闭设备
//     snd_pcm_close(capture_handle);
//     snd_pcm_close(playback_handle);

//     return 0;
// }


#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#define   save_pcm_file     "temp.pcm"
#define   play_pcm_file     "send_audio_16k_1ch.pcm"


snd_pcm_t *open_sound_dev(snd_pcm_stream_t type)
{
	int err;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *hw_params;
	unsigned int rate = 8000;
	int audio_channels = 2;

	// if (type == SND_PCM_STREAM_CAPTURE) {
	// 	rate = 16000;S
	// 	audio_channels = 2;
	// }
	// if (type == SND_PCM_STREAM_PLAYBACK) {
	// 	rate = 8000;
	// 	audio_channels = 2;
	// }
/*
int snd_pcm_open(snd_pcm_t **pcmp, const char *name, snd_pcm_stream_t stream, int mode)

pcmp 打开的pcm句柄
name 要打开的pcm设备名字，默认default，或者从asound.conf或者asoundrc里面选择所要打开的设备
stream SND_PCM_STREAM_PLAYBACK 或 SND_PCM_STREAM_CAPTURE，分别表示播放和录音的PCM流
mode 打开pcm句柄时的一些附加参数 SND_PCM_NONBLOCK 非阻塞打开（默认阻塞打开）, SND_PCM_ASYNC 异步模式打开
返回值 0 表示打开成功，负数表示失败，对应错误码
*/

	if ((err = snd_pcm_open (&handle, "hw:2,0", type, 0)) < 0) {
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
		fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
			 snd_strerror (err));
		return NULL;
	}
			 
	if ((err = snd_pcm_hw_params_any (handle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
			 snd_strerror (err));
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_access (handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {// 交错访问
		fprintf (stderr, "cannot set access type (%s)\n",
			 snd_strerror (err));
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_format (handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {  //  SND_PCM_FORMAT_S16_LE      有符号16 bit Little Endian
		fprintf (stderr, "cannot set sample format (%s)\n",
			 snd_strerror (err));
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_rate_near (handle, hw_params, &rate, 0)) < 0) {    //snd_pcm_hw_params_set_rate_near () 函数设置音频数据的最接近目标的采样率
		fprintf (stderr, "cannot set sample rate (%s)\n",
			 snd_strerror (err));
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_channels (handle, hw_params, audio_channels)) < 0) {
		fprintf (stderr, "cannot set channel count (%s)\n",
			 snd_strerror (err));
		return NULL;
	}
//snd_pcm_hw_params( ) 从设备配置空间选择一个配置，让函数 snd_pcm_prepare() 准备好 PCM 设备，以便写入 PCM 数据。
	if ((err = snd_pcm_hw_params (handle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n",
			 snd_strerror (err));
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

int play_recv_pcm(snd_pcm_t *handle, const void *data, int frame_num)
{
  // uint8_t play_buf[4096] = {0};

  // memcpy(play_buf, data, play_frame_num * 2);
  int err;

  if ((err = snd_pcm_writei (handle, data, frame_num)) != frame_num) {   //snd_pcm_writei() 用来把交错的音频数据写入到音频设备。
    fprintf (stderr, "write to audio interface failed (%s)\n",snd_strerror (err));
    return -1;
  }

  return 0;
}


int capture_and_play(void)
{
    int err;
	uint8_t cap_buf[4096], play_buf[4096];
	snd_pcm_t *playback_handle;
	snd_pcm_t *capture_handle;
    int cap_frame_num = 160;
	int play_frame_num = 160;

    FILE *save_pcm = NULL;
    FILE *play_pcm = NULL;

    
    if ((save_pcm = fopen(save_pcm_file, "wb")) == NULL) {
        printf("打开音频文件%s失败", save_pcm_file);
        exit(1);
    }

    if ((play_pcm = fopen(play_pcm_file, "rb")) == NULL) {
        printf("打开音频文件%s失败", play_pcm_file);
        exit(1);
    }

    playback_handle = open_playback();
    if (!playback_handle) {
		fprintf (stderr, "cannot open for playback\n");
        return -1;
    }


    capture_handle = open_capture();
    if (!capture_handle)
    {
		fprintf (stderr, "cannot open for capture\n");
        return -1;
    }

	if ((err = snd_pcm_prepare (playback_handle)) < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err));
		return -1;
	}

	if ((err = snd_pcm_prepare (capture_handle)) < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err));
		return -1;
	}

	while (1) {
		// if ((err = snd_pcm_readi (capture_handle, cap_buf, cap_frame_num)) != cap_frame_num) {
		// 	fprintf (stderr, "read from audio interface failed (%s)\n",snd_strerror (err));
		// 	return -1;
		// }
        // fwrite(cap_buf, 1, cap_frame_num * 4, save_pcm);

        fread(play_buf, 1, play_frame_num * 4, play_pcm);
		// if ((err = snd_pcm_writei (playback_handle, play_buf, play_frame_num)) != play_frame_num) {   //snd_pcm_writei() 用来把交错的音频数据写入到音频设备。
		// 	fprintf (stderr, "write to audio interface failed (%s)\n",snd_strerror (err));
		// 	return -1;
		// }
		play_recv_pcm(playback_handle, play_buf, play_frame_num);
	}


	fclose(save_pcm);
	fclose(play_pcm);

	snd_pcm_close (playback_handle);
	snd_pcm_close (capture_handle);

    return 0;
}

int main (int argc, char *argv[])
{
    capture_and_play();
    return 0;
}


