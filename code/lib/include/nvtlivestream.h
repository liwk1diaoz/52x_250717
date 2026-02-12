/**
    Live555 inquiry functions.

    Sample module detailed description.
    @file nvtlive555.h
    @ingroup LIVE555
    @note Nothing (or anything need to be mentioned).
    Copyright Novatek Microelectronics Corp. 2014. All rights reserved.
*/

#ifndef _NVTLIVSTREAM_H
#define _NVTLIVESTREAM_H


#if !defined(ENUM_DUMMY4WORD)
#define ENUM_DUMMY4WORD(name)   E_##name = 0x10000000
#endif

typedef enum _NVTLIVESTREAM_CODEC {
	NVTLIVESTREAM_CODEC_UNKNOWN,
	NVTLIVESTREAM_CODEC_MJPG,
	NVTLIVESTREAM_CODEC_H264,
	NVTLIVESTREAM_CODEC_H265,
	NVTLIVESTREAM_CODEC_PCM,
	NVTLIVESTREAM_CODEC_AAC,
	NVTLIVESTREAM_CODEC_G711_ALAW,
	NVTLIVESTREAM_CODEC_G711_ULAW,
	NVTLIVESTREAM_CODEC_META,
	NVTLIVESTREAM_CODEC_COUNT,
	ENUM_DUMMY4WORD(NVTLIVESTREAM_CODEC),
} NVTLIVESTREAM_CODEC;

typedef struct _NVTLIVESTREAM_VIDEO_INFO {
	 NVTLIVESTREAM_CODEC codec_type;
	unsigned char vps[64];
	int vps_size;
	unsigned char sps[64];
	int sps_size;
	unsigned char pps[64];
	int pps_size;
} NVTLIVESTREAM_VIDEO_INFO;

typedef struct _NVTLIVESTREAM_AUDIO_INFO {
	 NVTLIVESTREAM_CODEC codec_type;
	int sample_per_second;
	int bit_per_sample;
	int channel_cnt;
} NVTLIVESTREAM_AUDIO_INFO;

typedef struct _NVTLIVESTREAM_STRM_INFO {
	unsigned int addr;
	unsigned int size;
	unsigned long long timestamp;
	unsigned int svc_idx; //only h264 only
} NVTLIVESTREAM_STRM_INFO;
#endif /* nvtlivestream.h  */
