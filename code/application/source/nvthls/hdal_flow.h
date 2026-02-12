#ifndef _HDAL_FLOW
#define _HDAL_FLOW

#include "hdal.h"
#include "hd_bsmux_lib.h"
#include "hd_fileout_lib.h"



#define HLS_HTTPS_PORT 443
#define HLS_HTTP_PORT 8080

#define HLS_H265 0
#define HLS_H264 1
#define HLS_JPEG 2
#define HLS_ENC_TYPE HLS_H264
#define HLS_FRAME_RATE 30
#define HLS_BITRATE	8*1024*1024
#define HLS_GOP	15
#define HLS_RECORD_TIME 3



#define HLS_AUDIO_ENABLE 1

#define HLS_TS_VID_SIZE    21
#define HLS_TS_AUD_SIZE    21

#define HLS_HDMI_MODEL 1

#if (HLS_AUDIO_ENABLE == 1)
	#define HLS_AUDIO_SAMPLE_RATE 		HD_AUDIO_SR_48000
	#define HLS_AUDIO_SAMPLEBIT_WIDTH	HD_AUDIO_BIT_WIDTH_16
	#define HLS_AUDIO_MODE			HD_AUDIO_SOUND_MODE_MONO //HD_AUDIO_SOUND_MODE_STEREO
	#define HLS_AUDIO_FRAME_SAMPLE		1024
	#define HLS_AUDIO_AAC	0
	#define HLS_AUDIO_ULAW 1
	#define HLS_AUDIO_ALAW 2
	#define HLS_AUDIO_TYPE	HLS_AUDIO_AAC
#endif

//header
#define DBGINFO_BUFSIZE()       (0x200)
//RAW
#define VDO_RAW_BUFSIZE(w, h, pxlfmt)   (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//CA for AWB
#define CA_WIN_NUM_W            32
#define CA_WIN_NUM_H            32
#define VDO_CA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 3) << 1)
//LA for AE
#define LA_WIN_NUM_W            32
#define LA_WIN_NUM_H            32
#define VDO_LA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 1) << 1)

//YUV
#define VDO_SIZE_W              1920
#define VDO_SIZE_H              1080
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)   (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))

#if HLS_HDMI_MODEL == 1
	#define SEN_OUT_FMT     HD_VIDEO_PXLFMT_YUV422
	#define CAP_OUT_FMT     HD_VIDEO_PXLFMT_YUV420
#else
	#define CAP_OUT_FMT             HD_VIDEO_PXLFMT_RAW12
	#define SEN_OUT_FMT             HD_VIDEO_PXLFMT_RAW12
#endif
//fileyss


#define HLS_POOL_SIZE_FILESYS      (ALIGN_CEIL_64(0xEC000)+ALIGN_CEIL_64(0x80020))
typedef struct _VIDEO_RECORD {

        // (1)
        HD_VIDEOCAP_SYSCAPS cap_syscaps;
        HD_PATH_ID cap_ctrl;
        HD_PATH_ID cap_path;

        HD_DIM  cap_dim;
        HD_DIM  proc_max_dim;

        // (2)
        HD_VIDEOPROC_SYSCAPS proc_syscaps;
        HD_PATH_ID proc_ctrl;
        HD_PATH_ID proc_path;

        HD_DIM  enc_max_dim;
        HD_DIM  enc_dim;
	INT32	enc_bitrate;

        // (3)
        HD_VIDEOENC_SYSCAPS enc_syscaps;
        HD_PATH_ID enc_path;

        HD_PATH_ID bsmux_path;
	INT32 (*bsmux_cb_fun)(CHAR *, HD_BSMUX_CBINFO *, UINT32 *);


	UINT32 record_time;
	UINT32 max_num_of_file_in_m3u8;


        HD_PATH_ID fileout_path;
	CHAR file_folder[256];
	INT32 (*fileout_cb_fun)(CHAR *, HD_FILEOUT_CBINFO *, UINT32 *);


	#if HLS_AUDIO_ENABLE == 1
		HD_PATH_ID a_cap_ctrl;
		HD_PATH_ID a_cap_path;
		HD_PATH_ID a_enc_path;
	#endif

} VIDEO_RECORD;



HD_RESULT hdal_init(VIDEO_RECORD *stream);
HD_RESULT hdal_stop(VIDEO_RECORD *stream);
HD_RESULT hdal_mem_uinit(void);
#endif
