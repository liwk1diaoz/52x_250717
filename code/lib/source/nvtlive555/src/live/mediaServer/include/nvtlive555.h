/**
    Live555 inquiry functions.

    Sample module detailed description.
    @file nvtlive555.h
    @ingroup LIVE555
    @note Nothing (or anything need to be mentioned).
    Copyright Novatek Microelectronics Corp. 2014. All rights reserved.
*/

#ifndef _NVTLIVE555_H
#define _NVTLIVE555_H

#define NVTLIVE555_INTERFACE_VER_VERSION 0x17022219

#if !defined(ENUM_DUMMY4WORD)
#define ENUM_DUMMY4WORD(name)   E_##name = 0x10000000
#endif

#define NVTLIVE555_SVC_LEVEL_0 0 ///< all frames are sent
#define NVTLIVE555_SVC_LEVEL_1 1 ///< drop some frames smoothly
#define NVTLIVE555_SVC_LEVEL_2 2 ///< drop more frames smoothly

#define NVTLIVE555_EN_MASK_SVC_LEVEL 0x01
#define NVTLIVE555_EN_MASK_SVC_MAP   0x02

typedef enum _LIVE555_SERVER_EVENT {
	LIVE555_SERVER_EVENT_OPTIONS,
	LIVE555_SERVER_EVENT_DESCRIBE,
	LIVE555_SERVER_EVENT_SETUP,
	LIVE555_SERVER_EVENT_TEARDOWN,
	LIVE555_SERVER_EVENT_PLAY,
	LIVE555_SERVER_EVENT_PAUSE,
} LIVE555_SERVER_EVENT;

typedef struct _NVTLIVE555_CFG {
	int support_rtcp; ///< 0: not support, 1: support
	int support_meta; ///< 0: not support, 1: support (for onvif)
	unsigned int port; ///< rtsp port number,  normally is 554
	unsigned int snd_size;  ///< SO_SNDBUF size
	int tos; ///< tos value
	int max_clients; ///< accept max connections at the same time
} NVTLIVE555_CFG;

typedef struct _NVTLIVE555_URL_INFO {
	unsigned int channel_id;  ///< channel id
	unsigned int svc_level;   ///< must be NVTLIVE555_SVC_LEVEL_
	unsigned int total_ms;    ///< for liveview set it to 0;
} NVTLIVE555_URL_INFO;

typedef struct _NVTLIVE555_STATISTIC {
#if 0
	unsigned int channel_id; ///< channel id, for ecos it is always 0
	unsigned int is_svc_support; ///< 0: not support, 1: supported
	unsigned int client_ip_addr; ///< in_addr //wifiteam
	unsigned int skip_svc_frm[3]; //wifiteam skipped frame count
	unsigned int skip_svc_byte[3]; //wifiteam skipped bytes
#else //for backward compatible 
	unsigned int channel_id; ///< channel id, for ecos it is always 0
	unsigned int connect_id; ///< connection id, for ecos it is always 0
	int          ms_to_gop;  ///< the time(ms) distance from last frame of previous GOP to current GOP's 1st frame
	unsigned int is_svc_support; ///< 0: not support, 1: supported
	unsigned int client_ip_addr; ///< in_addr //wifiteam
	unsigned int skip_svc_frm[3]; ///< wifiteam skipped frame count
	unsigned int skip_svc_byte[3]; ///< wifiteam skipped bytes
#endif
} NVTLIVE555_STATISTIC;

typedef struct _NVTLIVE555_STRATEGY {
	int en_mask; ///< NVTLIVE555_EN_MASK
	int svc_level; ///< svc strategy for NVTLIVE555_EN_MASK_SVC_LEVEL
	int svc_sock_priority[3]; ///< set it [0, 0, 1] that is svc0,1 use socket[0], svc2 use socket[1] value cannot be larger than 1 for NVTLIVE555_EN_MASK_SVC_MAP
} NVTLIVE555_STRATEGY;

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__WIN32__) || defined(_WIN32)
#define __WEAK
#else
#define __WEAK  __attribute__ ((weak))
#endif

typedef int(*NVTLIVE555_GET_CFG)(NVTLIVE555_CFG *p_cfg); // given a default proffered config, user can modify it
typedef int(*NVTLIVE555_PARSE_URL)(const char *url, NVTLIVE555_URL_INFO *p_info);
typedef int(*NVTLIVE555_BRC)(const NVTLIVE555_STATISTIC *p_statistic, NVTLIVE555_STRATEGY *p_strategy);
typedef int(*NVTLIVE555_GET_META)(char *p_txt, unsigned int txt_buf_size, int first_meta);
typedef int(*NVTLIVE555_RTSP_EVENT)(LIVE555_SERVER_EVENT evt);
typedef int(*NVTLIVE555_REQUIRE_KEY_FRAME)(int channel);
typedef int(*NVTLIVE555_SEEK)(int channel, double request, double *response);
typedef int(*NVTLIVE555_PAUSE)(int channel);
typedef int(*NVTLIVE555_RESUME)(int channel);

typedef struct _NVTLIVE555_CB {
	NVTLIVE555_GET_CFG get_cfg; //(Optional, could be NULL)
	NVTLIVE555_PARSE_URL parse_url; //(Optional, could be NULL)
	NVTLIVE555_BRC get_brc; //(Optional, could be NULL)
	NVTLIVE555_GET_META get_meta; //(Optional, could be NULL)
	NVTLIVE555_RTSP_EVENT notify_rtsp_event; //(Optional, could be NULL)
	NVTLIVE555_REQUIRE_KEY_FRAME require_key_frame; //(Optional, could be NULL)
	NVTLIVE555_SEEK seek; //for playback file, it have to.
	NVTLIVE555_PAUSE pause; //for playback file, it have to.
	NVTLIVE555_RESUME resume; //for playback file, it have to.
} NVTLIVE555_CB;

typedef enum _NVTLIVE555_CODEC {
	NVTLIVE555_CODEC_UNKNOWN,
	NVTLIVE555_CODEC_MJPG,
	NVTLIVE555_CODEC_H264,
	NVTLIVE555_CODEC_H265,
	NVTLIVE555_CODEC_PCM,
	NVTLIVE555_CODEC_AAC,
	NVTLIVE555_CODEC_G711_ALAW,
	NVTLIVE555_CODEC_G711_ULAW,
	NVTLIVE555_CODEC_META,
	NVTLIVE555_CODEC_COUNT,
	ENUM_DUMMY4WORD(NVTLIVE555_CODEC),
} NVTLIVE555_CODEC;

typedef struct _NVTLIVE555_VIDEO_INFO {
	NVTLIVE555_CODEC codec_type;
	unsigned char vps[64];
	int vps_size;
	unsigned char sps[64];
	int sps_size;
	unsigned char pps[64];
	int pps_size;
} NVTLIVE555_VIDEO_INFO;

typedef struct _NVTLIVE555_AUDIO_INFO {
	NVTLIVE555_CODEC codec_type;
	int sample_per_second;
	int bit_per_sample;
	int channel_cnt;
} NVTLIVE555_AUDIO_INFO;

typedef struct _NVTLIVE555_STRM_INFO {
	unsigned int addr;
	unsigned int size;
	unsigned long long timestamp;
	unsigned int svc_idx; //only h264 only
} NVTLIVE555_STRM_INFO;

#define NVTLIVE555_ER_OK 0
#define NVTLIVE555_ER_NG (-1)
#define NVTLIVE555_ER_RETRY (-2)

typedef int(*NVTLIVE555_OPEN_STRM)(int channel);
typedef int(*NVTLIVE555_CLOSE_STRM)(int handle);
typedef int(*NVTLIVE555_GET_VIDEO_INFO)(int handle, int timeout_ms, NVTLIVE555_VIDEO_INFO *p_info);
typedef int(*NVTLIVE555_GET_AUDIO_INFO)(int handle, int timeout_ms, NVTLIVE555_AUDIO_INFO *p_info);
typedef int(*NVTLIVE555_LOCK_STRM)(int handle, int timeout_ms, NVTLIVE555_STRM_INFO *p_strm);
typedef int(*NVTLIVE555_UNLOCK_STRM)(int handle);

typedef struct _NVTLIVE555_HDAL_CB {
	// Video
	NVTLIVE555_OPEN_STRM open_video;
	NVTLIVE555_CLOSE_STRM close_video;
	NVTLIVE555_GET_VIDEO_INFO get_video_info;
	NVTLIVE555_LOCK_STRM lock_video;
	NVTLIVE555_UNLOCK_STRM unlock_video;
	// Audio
	NVTLIVE555_OPEN_STRM open_audio;
	NVTLIVE555_CLOSE_STRM close_audio;
	NVTLIVE555_GET_AUDIO_INFO get_audio_info;
	NVTLIVE555_LOCK_STRM lock_audio;
	NVTLIVE555_UNLOCK_STRM unlock_audio;
} NVTLIVE555_HDAL_CB;

typedef struct _NVTLIVE555_INIT {
	unsigned int version;       // MUST BE LIVE555_INTERFACE_VER
	NVTLIVE555_CB require_cb;   // requirement callbacks
	NVTLIVE555_HDAL_CB hdal_cb; // bitstream callbacks, for system has no stream sender
} NVTLIVE555_INIT;

/**
    notify live555 initialized

    user can initialize / create their own data by this function.
*/
extern int nvtlive555_open(NVTLIVE555_INIT *p_init);

/**
    notify live555 exit

    user can destroy their own dynamic data by this function.
*/
extern int nvtlive555_close(void);

/**
	debug command for linux only
*/
extern int nvtlive555_dbgcmd(int argc, char* argv[]);

#ifdef __cplusplus
}
#endif
/* ----------------------------------------------------------------- */
#endif /* nvtlive555.h  */
