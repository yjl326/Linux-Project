#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <signal.h>

#define pcm_file_path  "../out/aarch64/recv_audio.bin.pcm"

int play_recv_pcm(void)
{
	int i;
	int err;
	uint8_t *buffer;
	int AUDIO_CHANNEL_SET = 2;
	int rate = 16000;
	int one_frame_len = 640;

  	FILE *pcm_data_file = NULL;
	snd_pcm_t *playback_handle; // 一个指向PCM设备的句柄
	snd_pcm_hw_params_t *hw_params; //此结构包含有关硬件的信息，可用于指定PCM流的配置

	/*PCM的采样格式在pcm.h文件里有定义*/
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE; // 采样位数：16bit、LE格式
 	// 采样位数：16bit、LE格式

	/*打开音频采集卡硬件，并判断硬件是否打开成功，若打开失败则打印出错误提示*/
	// SND_PCM_STREAM_PLAYBACK 输出流
	// SND_PCM_STREAM_CAPTURE  输入流
	if ((err = snd_pcm_open (&playback_handle, "hw:1,0", SND_PCM_STREAM_PLAYBACK,0)) < 0) {
		printf("无法打开音频设备: %s (%s)\n", "hw:1,0",snd_strerror (err));
		exit(1);
	}
	printf("音频接口打开成功.\n");

	/*创建一个保存PCM数据的文件*/
	if((pcm_data_file = fopen(pcm_file_path, "rb")) == NULL) {
		printf("无法创建%s音频文件.\n", pcm_file_path);
		exit(1);
	} 
	printf("用于播放的音频文件已打开.\n");

	/*分配硬件参数结构对象，并判断是否分配成功*/
	if((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		printf("无法分配硬件参数结构 (%s)\n",snd_strerror(err));
		exit(1);
	}
	printf("硬件参数结构已分配成功.\n");
	
	/*按照默认设置对硬件对象进行设置，并判断是否设置成功*/
	if((err=snd_pcm_hw_params_any(playback_handle, hw_params)) < 0) {
		printf("无法初始化硬件参数结构 (%s)\n", snd_strerror(err));
		exit(1);
	}
	printf("硬件参数结构初始化成功.\n");

	/*
		设置数据为交叉模式，并判断是否设置成功
		interleaved/non interleaved:交叉/非交叉模式。
		表示在多声道数据传输的过程中是采样交叉的模式还是非交叉的模式。
		对多声道数据，如果采样交叉模式，使用一块buffer即可，其中各声道的数据交叉传输；
		如果使用非交叉模式，需要为各声道分别分配一个buffer，各声道数据分别传输。
	*/
	if((err = snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		printf("无法设置访问类型(%s)\n",snd_strerror(err));
		exit(1);
	}
	printf("访问类型设置成功.\n");

	/*设置数据编码格式，并判断是否设置成功*/
	if ((err=snd_pcm_hw_params_set_format(playback_handle, hw_params, format)) < 0) {
		printf("无法设置格式 (%s)\n",snd_strerror(err));
		exit(1);
	}
	fprintf(stdout, "PCM数据格式设置成功.\n");

	/*设置采样频率，并判断是否设置成功*/
	if((err=snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &(rate) ,0))<0) {
		printf("无法设置采样率(%s)\n",snd_strerror(err));
		exit(1);
	}
	printf("采样率设置成功\n");

	/*设置声道，并判断是否设置成功*/
	if((err = snd_pcm_hw_params_set_channels(playback_handle, hw_params, AUDIO_CHANNEL_SET)) < 0) {
		printf("无法设置声道数(%s)\n",snd_strerror(err));
		exit(1);
	}
	printf("声道数设置成功.\n");

	/*将配置写入驱动程序中，并判断是否配置成功*/
	if ((err=snd_pcm_hw_params (playback_handle, hw_params))<0) {
		printf("无法向驱动程序设置参数(%s)\n",snd_strerror(err));
		exit(1);
	}
	printf("参数设置成功.\n");

	/*使采集卡处于空闲状态*/
	snd_pcm_hw_params_free(hw_params);

	/*准备音频接口,并判断是否准备好*/
	if((err=snd_pcm_prepare(playback_handle)) < 0) {
		printf("无法使用音频接口 (%s)\n",snd_strerror(err));
		exit(1);
	}
	printf("音频接口准备好.\n");

	/*配置一个数据缓冲区用来缓冲数据*/
	//snd_pcm_format_width(format) 获取样本格式对应的大小(单位是:bit)
	int frame_byte = snd_pcm_format_width(format) / 8;
  	int size = frame_byte * (one_frame_len * AUDIO_CHANNEL_SET);
	buffer = (uint8_t *)malloc(size);
	
	/*开始采集音频pcm数据*/
	printf("开始播放音频数据...\n");
	
  	while (1) {
		//memcpy(buffer, temp_buf, 640);
    	fread(buffer, 1, one_frame_len * 2, pcm_data_file);
		/*向声卡设备写一帧音频数据 */
		if((err = snd_pcm_writei(playback_handle, buffer, one_frame_len)) != one_frame_len) {
			printf("向音频接口写数据失败(%s)\n",snd_strerror(err));
			exit(1);
		}
	}

	printf("播放完成.\n");
	/*释放数据缓冲区*/
	free(buffer);

	/*关闭音频采集卡硬件*/
	snd_pcm_close(playback_handle);

	return 0;
}

int main(void)
{
	play_recv_pcm();

	return 0;
}