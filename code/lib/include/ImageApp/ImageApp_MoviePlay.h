/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       ImageApp_MoviePlay.h
    @ingroup    mIImageApp

    @note       Nothing.

    @date       2017/05/08
*/

#ifndef IMAGEAPP_MOVIEPLAY_H
#define IMAGEAPP_MOVIEPLAY_H


#include "hdal.h"
#include "FileSysTsk.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "kwrap/task.h"
#include "vendor_audioout.h"
#include "avfile/MediaReadLib.h"

// Debug Macro
#if 0
#define DBG_DUMP printf
#define DBG_WRN(fmtstr, args...) DBG_DUMP("\033[33m%s(): \033[0m" fmtstr, __func__, ##args)
#define DBG_IND(fmtstr, args...) DBG_DUMP("%s(): " fmtstr, __func__, ##args)
#define DBG_ERR(fmtstr, args...) DBG_DUMP("\033[31m%s(): \033[0m" fmtstr, __func__, ##args)
#define DBGD(x) DBG_DUMP("\033[0;35m%s=%d\033[0m\r\n", #x, x)
#define DBGH(x) DBG_DUMP("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define CHKPNT DBG_DUMP("\033[37mCHK: %d, %s\033[0m\r\n", __LINE__, __func__)
#endif

// Other Macro
#define nvtmpp_vb_block2addr(args...)       nvtmpp_vb_blk2va(args)

#define NMEDIAPLAY_ENABLE              ENABLE

#define FULL_FILE_PATH_LEN			   64
#define MOVIEPLAY_HEADER_READSIZE	   0x3E8000	///< File Read Size for Media Header Information Parsing
#define MOVIEPLAY_TS_H26X_WORKBUF      64 * 1024   ///< TS format H26x working buffer (for TsParser)

#if 1 // To remove hard code in the near future.
#define VDO_SIZE_W          1920          // video pattern width
#define VDO_SIZE_H          1080          // video pattern height
#define DISP_SIZE_W         1920         // display panel width
#define DISP_SIZE_H         1080         // display panel height
#endif
#define BS_BLK_SIZE         0x200000     // bitstream buffer size
#define BS_BLK_DESC_SIZE    0x200     // bitstream buffer size

/**
    @addtogroup mIImageApp
*/
//@{
/**
    @name SMedia Video Decoder H265 Nal Unit Type Max Buffer Size

    SMedia video decoder h265 nal unit type max buffer size
    (please see definition "DecNalBufSize" in h265dec_header.c)
*/
//@{
#define MOVIEPLAY_VIDDEC_H265_NAL_MAXSIZE    512          ///< H265 Nal unit type max buffer size
//@}

typedef struct {
	ISF_UNIT					*p_vdoout;
} MOVIEPLAY_VDO_OUT_INFO;

typedef enum {
	_CFG_FILEPLAY_ID_1 = 0,
	_CFG_FILEPLAY_ID_MAX,
	_CFG_STRMPLAY_ID_1,
	_CFG_STRMPLAY_ID_MAX,
	ENUM_DUMMY4WORD(MOVIEPLAY_CFG_PLAY_ID)
} MOVIEPLAY_CFG_PLAY_ID;

typedef enum {
	MOVIEPLAY_DISP_ID_MIN = 0,
	MOVIEPLAY_DISP_ID_1 = MOVIEPLAY_DISP_ID_MIN,
	MOVIEPLAY_DISP_ID_2,
	MOVIEPLAY_DISP_ID_3,
	MOVIEPLAY_DISP_ID_4,
	MOVIEPLAY_DISP_ID_MAX,
	ENUM_DUMMY4WORD(MOVIEPLAY_DISP_ID)
} MOVIEPLAY_DISP_ID;

typedef enum {
	MOVIEPLAY_VID_OUT_1 = 0,
	MOVIEPLAY_VID_OUT_2,
	MOVIEPLAY_VID_OUT_MAX,
	ENUM_DUMMY4WORD(MOVIEPLAY_VID_OUT)
} MOVIEPLAY_VID_OUT;

// The following part is moved to ImageApp_MovieCommon.h
typedef enum {
	_MOVPLAY_CFG_CODEC_MJPG = 1,
	_MOVPLAY_CFG_CODEC_H264,
	_MOVPLAY_CFG_CODEC_H265,
	ENUM_DUMMY4WORD(MOVIEPLAY_CFG_CODEC)
} MOVIEPLAY_CFG_CODEC;

typedef enum {
	_MOVPLAY_CFG_AUD_CODEC_NONE = 0,
	_MOVPLAY_CFG_AUD_CODEC_PCM,
	_MOVPLAY_CFG_AUD_CODEC_AAC,
	ENUM_DUMMY4WORD(MOVIEPLAY_CFG_AUD_CODEC)
} MOVIEPLAY_CFG_AUD_CODEC;

typedef enum {
	_MOVPLAY_CFG_FILE_FORMAT_MP4 = 0,
	_MOVPLAY_CFG_FILE_FORMAT_TS,
	_MOVPLAY_CFG_FILE_FORMAT_MOV,
	ENUM_DUMMY4WORD(MOVIEPLAY_CFG_FILE_FORMAT)
} MOVIEPLAY_CFG_FILE_FORMAT;

typedef enum {
	_CFG_PLAY_TYPE_NORMAL = 0,          ///< Normal play
	_CFG_PLAY_TYPE_FASTFWR,             ///< Fast-forward
	_CFG_PLAY_TYPE_FASTRWD,             ///< Fast-rewind
	_CFG_PLAY_TYPE_STEP,                ///< Step play
	_CFG_PLAY_TYPE_STRM_FILE,           ///< Stream file
	_CFG_PLAY_TYPE_STRM_LIVE,           ///< Stream live view
	ENUM_DUMMY4WORD(MOVIEPLAY_CFG_PLAY_MODE)
} MOVIEPLAY_CFG_PLAY_MODE;

typedef enum {
	_CFG_PLAY_STS_NORMAL = 0,          ///< Normal play
	_CFG_PLAY_STS_PAUSE = 0,           ///< Normal play
	_CFG_PLAY_STS_FASTFWR,             ///< Fast-forward
	_CFG_PLAY_STS_FASTRWD,             ///< Fast-rewind
	_CFG_PLAY_STS_STEP,                ///< Step play
	_CFG_PLAY_STS_STRM_FILE,           ///< Stream file
	_CFG_PLAY_STS_STRM_LIVE,           ///< Stream live view
	ENUM_DUMMY4WORD(MOVIEPLAY_CFG_PLAY_STATUS)
} MOVIEPLAY_CFG_PLAY_STATUS;

/**
    Maximum memory information
*/
typedef struct {
    UINT32                      FileFormat;         ///<[in] File format (e.g. MP4, TS, ..., etc.)
    UINT32                      VidFR;              ///<[in] Video frame rate
    UINT32                      AudFR;              ///<[in] Audio frame rate
    UINT32                      TotalSecs;          ///<[in] Total seconds
    UINT32                      CodecType;          ///<[in] Codec type (e.g. H.264, H.265, ..., etc.)
	UINT32                      ImageWidth;         ///<[in] Image width
	UINT32                      ImageHeight;        ///<[in] Image height
    UINT32                      NeedBufsize;        ///<[out] Needed buffer size
} SMEDIAPLAY_MAX_MEM_INFO, *PSMEDIAPLAY_MAX_MEM_INFO;

typedef struct {
	MOVIEPLAY_CFG_PLAY_ID           fileplay_id;      ///< play ID
	FST_FILE                        file_handle;      ///< file handler
	UINT32                          curr_speed;       ///< current play speed
	UINT32                          curr_direct;      ///< current play direction
	UINT32                          blk_time;         ///< play time per one read file block
	UINT32                          blk_plnum;        ///< preload block number
	UINT32                          blk_ttnum;        ///< total block number
	UINT32                          blk_size;        ///< blk size
	void (*event_cb)(UINT32 event_id, UINT32 p1, UINT32 p2, UINT32 p3);                ///< user play callback event
    UINT32							ts_demux_buf_size;
} MOVIEPLAY_FILEPLAY_INFO;

#define MAX_DISP_PATH               1

typedef struct {
	BOOL                 enable;
	MOVIEPLAY_DISP_ID    disp_id;
	UINT32               width;
	UINT32               height;
	UINT32               width_ratio;
	UINT32               height_ratio;
	UINT32               rotate_dir;
	MOVIEPLAY_VID_OUT    vid_out;		///< VdoOut path number
	UINT32				 dev_id;
} MOVIEPLAY_DISP_INFO;

typedef struct {
	HD_PATH_ID vout_ctrl;
	HD_PATH_ID vout_path;
	USIZE      ratio;
	UINT32 queue_depth;
} MOVIEPLAY_CFG_DISP_INFO;

typedef struct {
	HD_RESULT 	ret;			/* hdal return code	*/
	BOOL		is_video; 		/* TRUE: video FALSE: audio */
	UINT32		frame_idx;		/* reserved column( currently not support ) */
} MOVIEPLAY_EVENT_DECODE_ERR_INFO;

typedef struct {

	BOOL pwr_en;
	HD_AUDIOOUT_DRV_CONFIG drv_cfg;

} MOVIEPLAY_AOUT_INFO_EX;

typedef struct {
	UINT32 width;
	UINT32 height;
	UINT32 frame_idx;
	UINT32 fps;
	UINT32 bs_addr;
} MOVIEPLAY_VDEC_SKIP_SVC_FRAME_BS_INFO;

typedef struct {
	BOOL enabled;
	BOOL (*condition_callback)(
			const MOVIEPLAY_VDEC_SKIP_SVC_FRAME_BS_INFO* bs_info,
			const void* user_data);
	void* user_data;
} MOVIEPLAY_VDEC_SKIP_SVC_FRAME;

enum {
	MOVIEPLAY_CONFIG_DISP_INFO  = 0xA000A000,
	MOVIEPLAY_CONFIG_FILEPLAY_INFO,
	MOVIEPLAY_CONFIG_STRMPLAY_INFO,
	MOVIEPLAY_CONFIG_MEM_POOL_INFO,
	MOVIEPLAY_CONFIG_AUDOUT_INFO,
	MOVIEPLAY_CONFIG_DECRYPT_INFO,
	MOVIEPLAY_CONFIG_AUDOUT_INFO_EX, /* MOVIEPLAY_CONFIG_AUDOUT_INFO can be replaced with MOVIEPLAY_CONFIG_AUDOUT_INFO_EX */
	MOVIEPLAY_CONFIG_VDEC_SKIP_SVC_FRAME,
};

enum MOVIEPLAY_VID_SPEEDUP {
	MOVIEPLAY_MOV_1x = 1,
	MOVIEPLAY_MOV_2x = 2,
	MOVIEPLAY_MOV_4x = 4,
	MOVIEPLAY_MOV_8x = 8,
};

// For setting parameters
enum {
	MOVIEPLAY_PARAM_START   = 0x0000AA000,
	MOVIEPLAY_PARAM_FILEHDL = MOVIEPLAY_PARAM_START,                         ///< file handle
	MOVIEPLAY_PARAM_CURSPD,                                                  ///< current speed
	MOVIEPLAY_PARAM_CURDIR,                                                  ///< current direction
	MOVIEPLAY_PARAM_EVENTCB,                                                 ///< callback event
	MOVIEPLAY_PARAM_MAX_MEM_INFO,
};

/**
    Memory Allocation Information
*/
typedef enum _MOVIEPLAY_MEM_INDEX {
	MOVIEPLAY_MEM_VDOENTTBL = 0,    ///< video frame entry table
	MOVIEPLAY_MEM_AUDENTTBL,                          ///< audio frame entry table
	MOVIEPLAY_MEM_CNT,
	ENUM_DUMMY4WORD(_MOVIEPLAY_MEM_INDEX)
} MOVIEPLAY_MEM_INDEX;

/**
	Callback Event for Play_MovieCB()
*/
typedef enum {
	MOVIEPLAY_EVENT_MEDIAINFO_READY,                  ///< media info ready
	MOVIEPLAY_EVENT_ONE_DISPLAYFRAME,                 ///< one display frame
	// [ToDo]
	MOVIEPLAY_EVENT_START,                            ///< play start
	MOVIEPLAY_EVENT_STOP,                             ///< play stop
	MOVIEPLAY_EVENT_FINISH,                           ///< play finish
	MOVIEPLAY_EVENT_ONE_SECOND,                       ///< play One second arrives
	MOVIEPLAY_EVENT_CURR_VIDFRAME,                    ///< current video frame stream data ready
	MOVIEPLAY_EVENT_CURR_AUDFRAME,                    ///< current audio frame stream data ready
	MOVIEPLAY_EVENT_DECODE_ERR,						  /* decode error, payload: MOVIEPLAY_EVENT_DECODE_ERR_INFO */
	ENUM_DUMMY4WORD(MOVIEPLAY_EVENT)
} MOVIEPLAY_EVENT;

/**
	MoviePlay Parameter for NMediaPlay framework
*/
typedef enum {
	MOVIEPLAY_NM_PARAM_BEGIN = 0x00000000,
	/* common */
	MOVIEPLAY_NM_PARAM_VDO_CODEC = MOVIEPLAY_NM_PARAM_BEGIN, ///< video codec type
	MOVIEPLAY_NM_PARAM_BLK_TTNUM,                         ///< total block number
	MOVIEPLAY_NM_PARAM_GOP,                               ///< GOP (Group of Picture)
	MOVIEPLAY_NM_PARAM_AUD_SR,                            ///< audio sample rate
	/* FileIn */
	MOVIEPLAY_NM_PARAM_FILEHDL,                           ///< file handle
	MOVIEPLAY_NM_PARAM_FILESIZE,                          ///< total file size
	MOVIEPLAY_NM_PARAM_FILEDUR,                           ///< total file duration (sec)
	MOVIEPLAY_NM_PARAM_BLK_TIME,                          ///< play time per one read file block
	MOVIEPLAY_NM_PARAM_BLK_PLNUM,                         ///< preload block number
	MOVIEPLAY_NM_PARAM_CUR_VDOBS,                         ///< current used video bs addr & size
	/* BsDemux */
	MOVIEPLAY_NM_PARAM_VDO_ENABLE,                        ///< video enable (default: 0)
	MOVIEPLAY_NM_PARAM_VDO_FR,                            ///< video frame rate
	MOVIEPLAY_NM_PARAM_VDO_FIRSTFRMSIZE,            ///< first video frame size
	MOVIEPLAY_NM_PARAM_VDO_TTFRAME,                       ///< total video frames
	MOVIEPLAY_NM_PARAM_VDO_CURRFRAME,					  /* current frame */
	MOVIEPLAY_NM_PARAM_AUD_ENABLE,                        ///< audio enable (default: 0)
	MOVIEPLAY_NM_PARAM_AUD_TTFRAME,                       ///< total audio frames
	MOVIEPLAY_NM_PARAM_CONTAINER,                         ///< media container
	MOVIEPLAY_NM_PARAM_FILE_FMT,                          ///< file format (mp4/mov or ts)
	MOVIEPLAY_NM_PARAM_TSDMX_BUFBLK,                      ///< TS demux buffer block number
	/* VdoDec */
	MOVIEPLAY_NM_PARAM_DECDESC,                           ///< decode description
	MOVIEPLAY_NM_PARAM_WIDTH,                             ///< width
	MOVIEPLAY_NM_PARAM_HEIGHT,                            ///< height
	/* AudDec */
	MOVIEPLAY_NM_PARAM_AUD_CODEC,                         ///< audio codec type
	MOVIEPLAY_NM_PARAM_AUD_CHS,                           ///< audio channels
	/* GetParam */
	MOVIEPLAY_NM_PARAM_MEDIA_INFO,                        ///< get first media info

	/* VdoDec part 2 */
	MOVIEPLAY_NM_PARAM_RAW_BLK_NUM,                       ///< setup RAW block num.
	MOVIEPLAY_NM_PARAM_STEP_PLAY,                         ///< enter step play state.

	/* Parameter maximum */
	MOVIEPLAY_NM_PARAM_MAX,
	ENUM_DUMMY4WORD(MOVIEPLAY_NM_PARAM)
} MOVIEPLAY_NM_PARAM;

typedef enum {
    SMEDIAPLAY_SPEED_NORMAL  = 0,                 ///< normal speed
    SMEDIAPLAY_SPEED_2X      = 2,                 ///< 2x   speed, trigger by timer (Play 2*I-frame number in second)
    SMEDIAPLAY_SPEED_4X      = 4,                 ///< 4x   speed, trigger by timer (Play 4*I-frame number in second)
    SMEDIAPLAY_SPEED_8X      = 8,                 ///< 8x   speed, trigger by timer (Play 8*I-frame number in second)
    SMEDIAPLAY_SPEED_16X     = 16,                ///< 16x  speed, trigger by timer (Play 8*I-frame number & skip 1 I-frames in second)
    SMEDIAPLAY_SPEED_32X     = 32,                ///< 32x  speed, trigger by timer (Play 8*I-frame number & skip 3 I-frames in second)
    SMEDIAPLAY_SPEED_64X     = 64,                ///< 64x  speed, trigger by timer (Play 8*I-frame number & skip 7 I-frames in second)
   	SMEDIAPLAY_SPEED_1_2X    = 5000,              ///< 1/2x speed, trigger by timer (Play 1/2*I-frame number in second)
    SMEDIAPLAY_SPEED_1_4X    = 2500,              ///< 1/4x speed, trigger by timer (Play 1/4*I-frame number in second)
    SMEDIAPLAY_SPEED_1_8X    = 1250,              ///< 1/8x speed, trigger by timer (Play 1/8*I-frame number in second)
    SMEDIAPLAY_SPEED_1_16X   = 625,               ///< 1/16x speed, trigger by timer (Play 1/16*I-frame number in second)
	ENUM_DUMMY4WORD(SMEDIAPLAY_SPEEDUP_TYPE)
} SMEDIAPLAY_SPEEDUP_TYPE;

/**
    @name type of playing direction

    Type of playing direction
*/
//@{
#define SMEDIAPLAY_DIR_FORWARD              1       ///< Forward Direction
#define SMEDIAPLAY_DIR_BACKWARD             2       ///< Backward Direction
//@}

/* NMediaPlay new param */
typedef struct _MOVIEPLAY_VIDEO_PLAYBACK {

	// (1)
	HD_VIDEODEC_SYSCAPS  dec_syscaps;
	HD_PATH_ID           dec_path;
	HD_DIM               dec_max_dim;
	UINT32               dec_type;

	// (2)
	HD_VIDEOPROC_SYSCAPS proc_syscaps;
	HD_PATH_ID           proc_ctrl;
	HD_PATH_ID           proc_path;
	HD_DIM               proc_max_dim;

	// (3)
	HD_VIDEOOUT_SYSCAPS  out_syscaps;
	HD_PATH_ID           out_ctrl;
	HD_PATH_ID           out_path;
	HD_DIM               out_max_dim;
	HD_DIM               out_dim;
	UINT32				 out_queue_depth;
	HD_VIDEOOUT_HDMI_ID  hdmi_id;

	#if 0
	// (4) user push
	pthread_t            dec_thread_id;
	UINT32               dec_exit;
	UINT32               flow_start;
	#endif

} MOVIEPLAY_VIDEO_PLAYBACK;

typedef struct _MOVIEPLAY_AUDIO_PLAYBACK {

	// (1) audio dec
	HD_PATH_ID dec_path;
	UINT32 dec_type;

	// (2) audio out
	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;

	// (3) user push
	THREAD_HANDLE  play_thread_id;
	THREAD_HANDLE  dec_thread_id;
	UINT32     dec_exit;
	UINT32     play_exit;
	UINT32     dec_trigger;
	UINT32     flow_start;
	ID         play_flg_id;

	BOOL       enable;

} MOVIEPLAY_AUDIO_PLAYBACK;

typedef struct {
	UINT64                          file_size;        ///< total file size
	FST_FILE                        file_handle;      ///< file handle
	UINT32                          file_dur;         ///< total file duration (sec)
	UINT32                          file_fmt;         ///< file format (MP4 or TS)
	UINT32                          width;            ///< video width
	UINT32                          height;           ///< video height
	UINT32                          tk_wid;           ///< video track width for sample aspect ratio
	UINT32                          tk_hei;           ///< video track height for sample aspect ratio
	UINT32                          vdo_fr;           ///< video frame rate
	UINT32                          vdo_codec;        ///< video codec type
	UINT32                          vdo_1stfrmsize;   ///< first video frame size
	UINT32                          vdo_ttfrm;        ///< total video frames
	UINT32                          desc_addr;        ///< video decode description addr (for H.264 and H.265)
	UINT32                          desc_size;        ///< video decode description size (for H.264 and H.265)
	UINT32                          vdec_path;        ///< video decode	path
	UINT32                          aud_fr;           ///< audio frame rate
	UINT32                          aud_sr;           ///< audio sameple rate
	UINT32                          aud_chs;          ///< audio channels
	UINT32                          aud_codec;        ///< audio codec type
	UINT32                          aud_ttfrm;        ///< total audio frames
	UINT32                          adec_path;        ///< audio decode	path
	HD_PATH_ID                      aout_path;        ///< audio out	path
	HD_PATH_ID                      aout_ctrl;        ///< audio out	ctrl
	UINT32                          gop;              ///< gop number
	BOOL                            vdo_en;           ///< video enable (default: 0)
	BOOL                            aud_en;           ///< audio enable (default: 0)
} MOVIEPLAY_PLAY_OBJ;

typedef struct {
    MOVIEPLAY_CFG_PLAY_ID    id;
    UINT32                   vb_pool_va;
    UINT32                   vb_pool_pa;
    ISF_UNIT                 *p_mediaplay;
    MOVIEPLAY_DISP_INFO      *p_disp_info;
	MOVIEPLAY_CFG_DISP_INFO  *p_cfg_disp_info;             ///< config HDAL vout module
	MOVIEPLAY_FILEPLAY_INFO  play_info;                    ///< play info for user input
	MOVIEPLAY_PLAY_OBJ       play_obj;                     ///< play obj for recording info in MoviePlay
} MOVIEPLAY_IMAGE_STREAM_INFO;

/// DispLink
typedef struct {
	// videoout
	HD_VIDEOOUT_SYSCAPS  vout_syscaps;
	HD_PATH_ID           vout_p_ctrl;
	// VdoOut flow refine: vout is opened in project
	#if 0
	HD_IN_ID             vout_i[1];
	HD_OUT_ID            vout_o[1];
	#endif
	HD_PATH_ID           vout_p[1];
	HD_DIM               vout_ratio;
	HD_URECT             vout_win;
	UINT32 vout_queue_depth;

} MOVIEPLAY_DISP_LINK;

typedef struct {
	// videoout
	UINT32               vout_p[1];
} MOVIEPLAY_DISP_LINK_STATUS;

typedef enum {
	MOVIEPLAY_DECRYPT_TYPE_NONE              = 0x00000000,
	MOVIEPLAY_DECRYPT_TYPE_CONTAINER         = 0x00000001,
	MOVIEPLAY_DECRYPT_TYPE_I_FRAME           = 0x00000002,
	MOVIEPLAY_DECRYPT_TYPE_P_FRAME           = 0x00000004,
	MOVIEPLAY_DECRYPT_TYPE_ALL_FRAME         = 0x00000006,
	MOVIEPLAY_DECRYPT_TYPE_AUDIO             = 0x00000008,         // (N/A)
	ENUM_DUMMY4WORD(MOVIEPLAY_ENCRYPT_TYPE)
} MOVIEPLAY_DECRYPT_TYPE;

typedef enum {
	MOVIEPLAY_DECRYPT_MODE_AES128            = 2,
	MOVIEPLAY_DECRYPT_MODE_AES256			 = 3,
	ENUM_DUMMY4WORD(MOVIEPLAY_DECRYPT_MODE)
} MOVIEPLAY_DECRYPT_MODE;

typedef struct {
	MOVIEPLAY_DECRYPT_TYPE type;
	MOVIEPLAY_DECRYPT_MODE mode;
	UINT8                  *key;
	UINT32                  key_len;
} MOVIEPLAY_FILE_DECRYPT;

/**
    Set parameter for Movie Play.

    Set parameter for Movie Play.

    @param[in] id    MOVIEPLAY_CFG_PLAY_ID
    @param[in] param MOVIEPLAY_PARAM_ID.
    @param[in] value.

    @return ISF_RV
*/
extern ISF_RV ImageApp_MoviePlay_SetParam(UINT32 param, UINT32 value);

/**
    Get parameter for Movie Play.

    Get parameter for Movie Play.

    @param[in] param MOVIEPLAY_PARAM_ID.

    @return UINT32
*/
extern UINT32 ImageApp_MoviePlay_GetParam(UINT32 param);

/*
    Update Image Stream.

    Update Image Stream.

    @param[in] void

    @return ISF_RV
*/
extern ISF_RV ImageApp_MoviePlay_UpdateAll(void);

/**
    Config setting for Movie Play.

    Config setting for Movie Play.

    @param[in] void

    @return ISF_RV
*/
extern ISF_RV ImageApp_MoviePlay_Config(UINT32 config_id, UINT32 value);

/**
    Open Movie Play.

    Open Movie Play.

    @param[in] void

    @return ISF_RV
*/
extern ISF_RV ImageApp_MoviePlay_Open(void);

/**
    Close Movie Play.

    Close Movie Play.

    @param[in] void

    @return ISF_RV
*/
extern ISF_RV ImageApp_MoviePlay_Close(void);

/**
    Start Movie Play.

    Start Movie Play.

    @param[in] void

    @return ISF_RV
*/
extern ISF_RV ImageApp_MoviePlay_Start(void);

/**
    Pause Movie Play.

    Pause Movie Play.

    @param[in] void

    @return ISF_RV
*/
extern ISF_RV ImageApp_MoviePlay_FilePlay_Pause(void);

/**
    Resume Movie Play.

    Resume Movie Play.

    @param[in] void

    @return ISF_RV
*/
extern ISF_RV ImageApp_MoviePlay_FilePlay_Resume(void);

/**
    Update Speed and Direction.

    @param[in] speed    current play speed
    @param[in] direct   current play direction

    @return ISF_RV
*/
extern ISF_RV ImageApp_MoviePlay_FilePlay_UpdateSpeedDirect(UINT32 speed, UINT32 direct, INT32 disp_idx);

/**
    Trigger Step Play.

    Trigger Step Play.

    @param[in] direct

    @return ISF_RV
*/
ISF_RV ImageApp_MoviePlay_FilePlay_StepByStep(UINT32 direct, UINT32 disp_idx);

extern INT32 ImageApp_MoviePlay_SetVolume(UINT32 vol);

/**
    Video File Recovery Function API

    File recovery API for power-cut porection.

    @param[in] pFilePath   video file full path name
    @param[in] memAddr     memory address
    @param[in] size        memory size
    @return
     - @b E_OK:     Recover the video file successfully.
     - @b E_SYS:    Recovery function is failed.
*/

extern ER SMediaPlay_FileRecovery(char *pFilePath, UINT32 memAddr, UINT32 size);

extern int movieplay_open_main(void);
extern int movieplay_close_main(void);
extern MOVIEPLAY_IMAGE_STREAM_INFO		*g_pImageStream;
extern CONTAINERPARSER					g_movply_container_obj;

//@}
#endif//IMAGEAPP_MOVIEPLAY_H
