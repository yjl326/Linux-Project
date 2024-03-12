#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

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
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <alsa/asoundlib.h>
#include <pthread.h>

#include "agora_rtc_api.h"
#include "file_parser.h"
#include "file_writer.h"
#include "utility.h"
#include "pacer.h"
#include "log.h"
#include "fifo.h"

#define DEFAULT_CHANNEL_NAME "hello_demo"
#define DEFAULT_CERTIFACTE_FILENAME "certificate.bin"
#define DEFAULT_SEND_VIDEO_FILENAME "send_video.h264"
//#define DEFAULT_SEND_AUDIO_FILENAME "send_audio_16k_1ch.pcm"
#define DEFAULT_8K_SEND_AUDIO_FILENAME "send_audio_8k_1ch.pcm"
#define DEFAULT_SEND_AUDIO_FILENAME "send_audio_16k_test.pcm"

#define DEFAULT_RECV_AUDIO_BASENAME "recv_audio.bin"
#define DEFAULT_RECV_VIDEO_BASENAME "recv_video.bin"
#define DEFAULT_SEND_VIDEO_FRAME_RATE (25)
#define DEFAULT_BANDWIDTH_ESTIMATE_MIN_BITRATE (100000)
#define DEFAULT_BANDWIDTH_ESTIMATE_MAX_BITRATE (1000000)
#define DEFAULT_BANDWIDTH_ESTIMATE_START_BITRATE (500000)
#define DEFAULT_SEND_AUDIO_FRAME_PERIOD_MS (20)
#define DEFAULT_PCM_SAMPLE_RATE (16000)
#define DEFAULT_PCM_CHANNEL_NUM (1)

typedef struct {
  // common config
  const char *p_sdk_log_dir;

  const char *p_appid;
  const char *p_token;
  const char *p_channel;
  const char *p_license;
  uint32_t uid;
  uint32_t area;

  // video related config
  video_data_type_e video_data_type;
  int send_video_frame_rate;
  const char *send_video_file_path;

  // audio related config
  audio_data_type_e audio_data_type;
  audio_codec_type_e audio_codec_type;
  const char *send_audio_file_path;
  uint32_t pcm_sample_rate;
  uint32_t pcm_channel_num;

  // advanced config
  bool send_video_generic_flag;
  bool enable_audio_mixer;
  bool receive_data_only;
  const char *local_ap;
  bool domain_limit;
} app_config_t;

void app_print_usage(int argc, char **argv)
{
  LOGS("\nUsage: %s [OPTION]", argv[0]);
  LOGS(" -h, --help                : show help info");
  LOGS(" -i, --app-id              : application id; either app-id OR token MUST be set");
  LOGS(" -t, --token               : token for authentication");
  LOGS(" -c, --channel-id          : channel name; default is 'demo'");
  LOGS(" -u, --user-id             : user id; default is 0");
  LOGS(" -l, --license             : license value MUST be set when release");
  LOGS(" -v, --video-type          : video data type for the input video file; default is 2");
  LOGS("                             support: 2=H264, 20=JPEG");
  LOGS(" -a, --audio-type          : audio data type for the input audio file; default is 100");
  LOGS("                             support: 1=OPUS, 5=G722, 8=AACLC, 9=HEAAC, 100=PCM");
  LOGS(" -C, --audio-codec         : audio codec type; only valid when audio type is PCM; default is 1");
  LOGS("                             support: 1=OPUS, 2=G722 4=G711U");
  LOGS(" -f  --fps                 : video frame rate; default is 30");
  LOGS(" -s, --send-video-file     : send video file path; default is './%s'",
       DEFAULT_SEND_VIDEO_FILENAME);
  LOGS(" -S, --send-audio-file     : send audio file path; default is './%s'",
       DEFAULT_SEND_AUDIO_FILENAME);
  LOGS(" -r, --pcm-sample-rate     : sample rate for the input PCM data; only valid when audio type is PCM");
  LOGS(" -n, --pcm-channel-num     : channel number for the input PCM data; only valid when audio type is PCM");
  LOGS(" -A, --area                : hex format with 0x header, supported area_code list:");
  LOGS("                             CN (Mainland China) : 0x00000001");
  LOGS("                             NA (North America)  : 0x00000002");
  LOGS("                             EU (Europe)         : 0x00000004");
  LOGS("                             AS (Asia)           : 0x00000008");
  LOGS("                             JP (Japan)          : 0x00000010");
  LOGS("                             IN (India)          : 0x00000020");
  LOGS("                             OC (Oceania)        : 0x00000040");
  LOGS("                             SA (South-American) : 0x00000080");
  LOGS("                             AF (Africa)         : 0x00000100");
  LOGS("                             KR (South Korea)    : 0x00000200");
  LOGS("                             OVS (Global except China): 0xFFFFFFFE");
  LOGS("                             GLOB (Global)       : 0xFFFFFFFF");
  LOGS(" -g, --send-video-generic  : enable generic codec flag for sending video");
  LOGS(" -m, --audio-mixer         : enable audio mixer to mix multiple incoming audio streams");
  LOGS(" -R, --recv-only           : do not send video and audio data");
  LOGS(" --local-ap                : params_str = {\"ipList\": [\"ip1\", \"ip2\"], \"domainList\":[\"domain1\", \"domain2\"], \"mode\": 1}");
  LOGS("                             ps:");
  LOGS("                              - mode: 0: ConnectivityFirst, 1: LocalOnly");
  LOGS(" -d, --domain-limit        : domain limit is enabled");
  LOGS("\nExample:");
  LOGS("    %s --app-id xxx [--token xxx] --channel-id xxx --send-video-file ./video.h264 --fps 15 \
        --send-audio-file ./audio.pcm --license <your license>",
       argv[0]);
}

int app_parse_args(int argc, char **argv, app_config_t *config)
{
  const char *av_short_option = "hi:t:c:u:v:a:C:f:S:s:r:n:A:gmRl:d";
  int av_option_flag = 0;
  const struct option av_long_option[] = { { "help", 0, NULL, 'h' },
                                           { "app-id", 1, NULL, 'i' },
                                           { "token", 1, NULL, 't' },
                                           { "channel-id", 1, NULL, 'c' },
                                           { "user-id", 1, NULL, 'u' },
                                           { "video-type", 1, NULL, 'v' },
                                           { "audio-type", 1, NULL, 'a' },
                                           { "audio-codec", 1, NULL, 'C' },
                                           { "fps", 1, NULL, 'f' },
                                           { "send-audio-file", 1, NULL, 'S' },
                                           { "send-video-file", 1, NULL, 's' },
                                           { "pcm-sample-rate", 1, NULL, 'r' },
                                           { "pcm-channel-num", 1, NULL, 'n' },
                                           { "area", 0, NULL, 'A' },
                                           { "send-video-generic", 0, NULL, 'g' },
                                           { "audio-mixer", 0, NULL, 'm' },
                                           { "recv-only", 0, NULL, 'R' },
                                           { "local-ap", 1, &av_option_flag, 1 },
                                           { "license", 1, NULL, 'l' },
                                           { "domain-limit", 0, NULL, 'd' },
                                           { 0, 0, 0, 0 } };

  int ch = -1;
  int optidx = 0;
  int rval = 0;

  while (1) {
    ch = getopt_long(argc, argv, av_short_option, av_long_option, &optidx);
    if (ch == -1) {
      break;
    }

    switch (ch) {
    case 'h':
      return -1;
    case 'i':
      config->p_appid = optarg;
      break;
    case 't':
      config->p_token = optarg;
      break;
    case 'c':
      config->p_channel = optarg;
      break;
    case 'u':
      config->uid = strtoul(optarg, NULL, 10);
      break;
    case 'v':
      config->video_data_type = strtol(optarg, NULL, 10);
      break;
    case 'a':
      config->audio_data_type = strtol(optarg, NULL, 10);
      break;
    case 'C':
      config->audio_codec_type = strtol(optarg, NULL, 10);
      break;
    case 'f':
      config->send_video_frame_rate = strtol(optarg, NULL, 10);
      break;
    case 'S':
      config->send_audio_file_path = optarg;
      break;
    case 's':
      config->send_video_file_path = optarg;
      break;
    case 'r':
      config->pcm_sample_rate = atoi(optarg);
      break;
    case 'n':
      config->pcm_channel_num = atoi(optarg);
      break;
    case 'A':
      config->area = strtol(optarg, NULL, 16);
      break;
    case 'g':
      config->send_video_generic_flag = true;
      break;
    case 'm':
      config->enable_audio_mixer = true;
      break;
    case 'R':
      config->receive_data_only = true;
      break;
    case 'l':
      config->p_license = optarg;
      break;
    case 'd':
      config->domain_limit = true;
      break;
    case 0:
      /* Pure long options */
      switch (av_option_flag) {
      case 1: // local-ap
        config->local_ap = optarg;
        break;
      default:
        LOGE("Unknown cmd param %s", av_long_option[optidx].name);
        return -1;
      }
      break;
    default:
      LOGS("Unknown cmd param %s", av_long_option[optidx].name);
      return -1;
    }
  }

  // check parameter sanity
  if (strcmp(config->p_appid, "") == 0) {
    LOGE("MUST provide App ID");
    return -1;
  }

  if (strcmp(config->p_channel, "") == 0) {
    LOGE("MUST provide channel name");
    return -1;
  }

  // if (config->send_video_frame_rate <= 0) {
  //   LOGE("Invalid video frame rate: %d", config->send_video_frame_rate);
  //   return -1;
  // }

  return 0;
}

static media_file_type_e video_data_type_to_file_type(video_data_type_e type)
{
  media_file_type_e file_type;
  switch (type) {
  case VIDEO_DATA_TYPE_H264:
    file_type = MEDIA_FILE_TYPE_H264;
    break;
  default:
    file_type = MEDIA_FILE_TYPE_H264;
    break;
  }
  return file_type;
}

static media_file_type_e audio_data_type_to_file_type(audio_data_type_e type)
{
  media_file_type_e file_type;
  switch (type) {
  case AUDIO_DATA_TYPE_PCM:
    file_type = MEDIA_FILE_TYPE_PCM;
    break;
  case AUDIO_DATA_TYPE_OPUS:
    file_type = MEDIA_FILE_TYPE_OPUS;
    break;
  case AUDIO_DATA_TYPE_AACLC:
    file_type = MEDIA_FILE_TYPE_AACLC;
    break;
  case AUDIO_DATA_TYPE_HEAAC:
    file_type = MEDIA_FILE_TYPE_HEAAC;
    break;
  case AUDIO_DATA_TYPE_G722:
    file_type = MEDIA_FILE_TYPE_G722;
    break;
  case AUDIO_DATA_TYPE_PCMA:
  case AUDIO_DATA_TYPE_PCMU:
    file_type = MEDIA_FILE_TYPE_G711;
    break;
  default:
    file_type = MEDIA_FILE_TYPE_PCM;
    break;
  }
  return file_type;
}

#endif