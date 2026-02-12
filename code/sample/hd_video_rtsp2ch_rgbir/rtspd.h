/*
 * rtspd.h
 *
 *  Created on: 2018/6/1
 *      Author: jack
 */

#ifndef __RTSPD_H__
#define __RTSPD_H__

#if 1
#define DBG_MSG printf
#define ERR_MSG printf
#define QAS_MSG printf
#else
#define DBG_MSG
#endif

typedef enum _RTSP_SERVER_STATE_ {
	RTSP_SERVER_STATE_UNINIT = 0,
	RTSP_SERVER_STATE_INIT,
	RTSP_SERVER_STATE_IDLE,
	RTSP_SERVER_STATE_RUNNING,
	RTSP_SERVER_STATE_MAX_NUM
} RTSP_SERVER_STATE;

typedef enum _VIDEO_CODEC_TYPE_ {
	VIDEO_CODEC_TYPE_H265 = 0,
	VIDEO_CODEC_TYPE_H264,
	VIDEO_CODEC_TYPE_MPEG,
	VIDEO_CODEC_TYPE_JPEG,
	VIDEO_CODEC_TYPE_MJPEG
} VIDEO_CODEC_TYPE;

typedef enum _AUDIO_CODEC_TYPE_ {
	AUDIO_CODEC_TYPE_PCM = 0,
	AUDIO_CODEC_TYPE_AAC,
	AUDIO_CODEC_TYPE_ALAW,
	AUDIO_CODEC_TYPE_ULAW
} AUDIO_CODEC_TYPE;

extern RTSP_SERVER_STATE g_rtsp_server_state;

RTSP_SERVER_STATE get_rtsp_server_state(void);


int rtspd_start(int max_ch_cnt, int max_bs_cnt, VIDEO_CODEC_TYPE enc_type);	//Modify data type of enc_type from int to VIDEO_CODEC_TYPE - Elvis - 20190523
int rtspd_stop(void);
int update_video_sdp(int ch_num, int bs_num , VIDEO_CODEC_TYPE enc_type , unsigned char *data , int data_len); //data must be key frame 	//Modify data type of enc_type from int to VIDEO_CODEC_TYPE - Elvis - 20190523
int write_rtp_frame_ext_video(int ch_num, int bs_num ,void *data, int data_len , int is_keyframe , unsigned int tv_ms);
int write_rtp_frame_ext_audio(int ch_num, int au_enc_type , void *data, int data_len , unsigned int tv_ms);

#endif /* __RTSPD_H__ */
