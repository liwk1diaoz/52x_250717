/**
	@brief Header file of image application movie player module.\n
	This file contains the functions which is related to image application movie player in the chip.

	@file iamovieplay_tsk.h

	@ingroup mlib

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include <string.h>
#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/debug.h"
#include "kwrap/stdio.h"
#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"
#include "kwrap/platform.h"
#include "hd_type.h"
#include "hd_common.h"
#include "comm/timer.h"
#include "avfile/media_def.h"
#include "avfile/MediaReadLib.h"
#include "avfile/AVFile_ParserTs.h"
#include "hd_bsdemux_lib.h"
#include "ImageApp/ImageApp_MoviePlay.h"


#define    IAMOVIEPLAY_US_IN_SECOND           (1000*1000)
#define    IAMOVIEPLAY_TIMER_INTVAL_DEFAULT   33333
#define    BS_VDODEC_BLK_SIZE     0x200000
#define    FILEIN_BLK_SIZE        0x200000

#define    IAMMOVIEPLAY_AUTO_BIND_VOUT        DISABLE

/**
    NMP Audio Decoder Bit-Stream Decode Sample Per One Entry
*/
#define  IAMOVIEPLAY_BS_DECODE_SAMPLES        1024    ///< refer to NMI_AUDDEC_BS_DECODE_SAMPLES

/**
    Flags for Demux Task
*/
#define FLG_IAMOVIEPLAY_DEMUX_IDLE         FLGPTN_BIT(0)
#define FLG_IAMOVIEPLAY_DEMUX_START        FLGPTN_BIT(1)                  ///< start demux
#define FLG_IAMOVIEPLAY_DEMUX_STOP         FLGPTN_BIT(2)                  ///< stop demux
#define FLG_IAMOVIEPLAY_DEMUX_VBS_DEMUX    FLGPTN_BIT(3)                  ///< video bitstream demux
#define FLG_IAMOVIEPLAY_DEMUX_ABS_DEMUX    FLGPTN_BIT(4)                  ///< audio bitstream demux
#define FLG_IAMOVIEPLAY_DEMUX_VBS_OUT      FLGPTN_BIT(5)                  ///< output video bitstream
#define FLG_IAMOVIEPLAY_DEMUX_IFRM_ONESHOT FLGPTN_BIT(6)                  ///< i-frame one shot

/**
	Flags for Read Task
*/
#define FLG_IAMOVIEPLAY_READ_IDLE         	FLGPTN_BIT(0)
#define FLG_IAMOVIEPLAY_READ_TRIG			FLGPTN_BIT(1)               ///< trigger file read checking
#define FLG_IAMOVIEPLAY_READ_STOP         	FLGPTN_BIT(2)               ///< stop task

/**
	Flags for Audio PB Task
*/
#define FLG_IAMOVIEPLAY_AUDIO_PB_IDLE      	FLGPTN_BIT(0)
#define FLG_IAMOVIEPLAY_AUDIO_PB_TRIG_DEC	FLGPTN_BIT(1)               ///< trigger audio decode
#define FLG_IAMOVIEPLAY_AUDIO_PB_PAUSE     	FLGPTN_BIT(2)               ///< pause audio PB
#define FLG_IAMOVIEPLAY_AUDIO_PB_RESUME    	FLGPTN_BIT(3)               ///< resume audio PB
#define FLG_IAMOVIEPLAY_AUDIO_PB_STOP    	FLGPTN_BIT(4)

/**
	Flags for Audio decode Task
*/
#define FLG_IAMOVIEPLAY_AUDIO_DECODE_IDLE      	FLGPTN_BIT(0)
#define FLG_IAMOVIEPLAY_AUDIO_DECODE_TRIG		FLGPTN_BIT(1)           ///< TRIG audio decode
#define FLG_IAMOVIEPLAY_AUDIO_DECODE_STOP		FLGPTN_BIT(2)

/**
	Flags for Display Task
*/
#define FLG_IAMOVIEPLAY_DISP_IDLE         	FLGPTN_BIT(0)
#define FLG_IAMOVIEPLAY_DISP_TRIG			FLGPTN_BIT(1)               ///< trigger display task
#define FLG_IAMOVIEPLAY_DISP_STOP         	FLGPTN_BIT(2)               ///< stop task

/**
    audio decode codec type
*/
#define IAMOVIEPLAY_DEC_PCM		MEDIAAUDIO_CODEC_SOWT		///< PCM audio format
#define IAMOVIEPLAY_DEC_AAC		MEDIAAUDIO_CODEC_MP4A		///< AAC audio format
#define IAMOVIEPLAY_DEC_PPCM	MEDIAAUDIO_CODEC_RAW8		///< RAW audio format
#define IAMOVIEPLAY_DEC_ULAW	MEDIAAUDIO_CODEC_ULAW		///< ULAW audio format
#define IAMOVIEPLAY_DEC_ALAW	MEDIAAUDIO_CODEC_ALAW		///< ULAW audio format
#define IAMOVIEPLAY_DEC_XX		MEDIAAUDIO_CODEC_XX			///< Unknown audio format

/**
    video decode codec type
*/
#define IAMOVIEPLAY_DEC_MJPG		MEDIAPLAY_VIDEO_MJPG    	///< motion-jpeg
#define IAMOVIEPLAY_DEC_H264		MEDIAPLAY_VIDEO_H264    	///< H.264
#define IAMOVIEPLAY_DEC_H265		MEDIAPLAY_VIDEO_H265    	///< H.265
#define IAMOVIEPLAY_DEC_YUV			MEDIAPLAY_VIDEO_YUV    		///< YUV

typedef enum {
	IAMMOVIEPLAY_DBG_COMM = 1,
	IAMMOVIEPLAY_DBG_DECRYPT = 2,
} IAMMOVIEPLAY_DBG_LEVEL;


/* comm debug */
#define IAMMOVIEPLAY_COMM_DUMP(fmtstr, args...)  if (g_movply_dbglvl & IAMMOVIEPLAY_DBG_COMM) DBG_DUMP(fmtstr, ##args) /* debug dump macro */

/* decrypt debug */
#define IAMMOVIEPLAY_DECRYPT_DUMP(fmtstr, args...) if (g_movply_dbglvl & IAMMOVIEPLAY_DBG_DECRYPT) DBG_DUMP(fmtstr, ##args)

#define IAMMOVIEPLAY_DECRYPT_DUMP_LEN 16
/**
    File format type definition
*/
typedef enum {
	IAMOVIEPLAY_FILEFMT_MP4 = MEDIA_FILEFORMAT_MP4,
	IAMOVIEPLAY_FILEFMT_TS  = MEDIA_FILEFORMAT_TS,
	IAMOVIEPLAY_FILEFMT_MOV = MEDIA_FILEFORMAT_MOV,
	ENUM_DUMMY4WORD(IAMOVIEPLAY_FILEFMT)
} IAMOVIEPLAY_FILEFMT;

typedef enum {
    IAMOVIEPLAY_STATE_CLOSE = 0,                                    ///< close state
    IAMOVIEPLAY_STATE_OPEN,                                         ///< open state
    IAMOVIEPLAY_STATE_PLAYING,                                      ///< playing state
    IAMOVIEPLAY_STATE_PAUSE,                                        ///< pause state
    IAMOVIEPLAY_STATE_RESUME,                                       ///< resume state
    IAMOVIEPLAY_STATE_STEP,                                         ///< step state
    IAMOVIEPLAY_STATE_STOP,                                         ///< stop state
	ENUM_DUMMY4WORD(IAMOVIEPLAY_PLAY_STATE)
} IAMOVIEPLAY_PLAY_STATE;

typedef enum {
    IAMOVIEPLAY_SPEED_NORMAL  = 0,                                  ///< normal speed
    IAMOVIEPLAY_SPEED_2X      = 2,                                  ///< 2x    speed, trigger by timer (Play 2*I-frame number in second)
    IAMOVIEPLAY_SPEED_4X      = 4,                                  ///< 4x    speed, trigger by timer (Play 4*I-frame number in second)
    IAMOVIEPLAY_SPEED_8X      = 8,                                  ///< 8x    speed, trigger by timer (Play 8*I-frame number in second)
    IAMOVIEPLAY_SPEED_16X     = 16,                                 ///< 16x   speed, trigger by timer (Play 8*I-frame number & skip 1 I-frames in second)
    IAMOVIEPLAY_SPEED_32X     = 32,                                 ///< 32x   speed, trigger by timer (Play 8*I-frame number & skip 3 I-frames in second)
    IAMOVIEPLAY_SPEED_64X     = 64,                                 ///< 64x   speed, trigger by timer (Play 8*I-frame number & skip 7 I-frames in second)
    IAMOVIEPLAY_SPEED_1_2X    = IAMOVIEPLAY_US_IN_SECOND / 2,       ///< 1/2x  speed, trigger by timer (Play 1/2*I-frame number in second)
    IAMOVIEPLAY_SPEED_1_4X    = IAMOVIEPLAY_US_IN_SECOND / 4,       ///< 1/4x  speed, trigger by timer (Play 1/4*I-frame number in second)
    IAMOVIEPLAY_SPEED_1_8X    = IAMOVIEPLAY_US_IN_SECOND / 8,       ///< 1/8x  speed, trigger by timer (Play 1/8*I-frame number in second)
    IAMOVIEPLAY_SPEED_1_16X   = IAMOVIEPLAY_US_IN_SECOND / 16,      ///< 1/16x speed, trigger by timer (Play 1/16*I-frame number in second)
	ENUM_DUMMY4WORD(IAMOVIEPLAY_SPEED_TYPE)
} IAMOVIEPLAY_SPEED_TYPE;

typedef enum {
    IAMOVIEPLAY_DIRECT_FORWARD  = 1,                                ///< forward direction
    IAMOVIEPLAY_DIRECT_BACKWARD = 2,                                ///< backward direction
	ENUM_DUMMY4WORD(IAMOVIEPLAY_DIRECT_TYPE)
} IAMOVIEPLAY_DIRECT_TYPE;

typedef struct _IAMOVIEPLAY_TSK {
    /*
    UINT32                  vdo_1stfrmsize;                         ///< first video frame size

    UINT32                  vdo_wid;                                ///< video width
    UINT32                  vdo_hei;                                ///< video height

    UINT32                  desc_addr;                              ///< video decode description addr
    UINT32                  desc_size;                              ///< video decode description addr
    NMI_BSDEMUX_CB          *CallBackFunc;                          ///< event inform callback*/
	// video
	BOOL                    vdo_en;                                 ///< video enable
	UINT32                  vdo_fr;                                 ///< video frame rate
	HD_VIDEO_CODEC          vdo_codec;                              ///< video codec type
	UINT32                  vdo_ttfrm;                              ///< total video frames
	UINT32                  gop;                                    ///< group of pictures
	HD_COMMON_MEM_VB_BLK    vdec_blk;         						///< video decode	blk

	// audio
	BOOL                    aud_en;                                 ///< audio enable
	UINT32                  aud_sr;                                 ///< audio frame rate
	UINT32                  aud_ttfrm;                              ///< total audio frames
	// file
	CONTAINERPARSER         *p_contr;                               ///< pointer of container handler
	UINT32                  file_fmt;                               ///< file format (mp4/mov or ts)
	// control
	UINT32                  speed;                                  ///< current play speed level
    UINT32                  direct;                                 ///< current play direction
    UINT32                  vdo_idx;                                ///< current video index
    UINT32                  aud_idx;                                ///< current audio index
    UINT32                  aud_count;                              ///< pulled audio count
    IAMOVIEPLAY_PLAY_STATE  state;                                  ///< current play state
    // timer
    volatile BOOL           timer_open;                             ///< timer open status (1: open, 0: close)
    volatile UINT32         timer_id;                               ///< timer id
    UINT32                  timer_intval;                           ///< timer interval (default: 33333 us)
	// eof/flow flag
    BOOL                    vdo_frm_eof;                            ///< video play to the end of frame
    BOOL                    aud_frm_eof;                            ///< audio play to the end of frame
    BOOL                    do_seek;                                ///< resume play flag, set true if do seek

} IAMOVIEPLAY_TSK;

/**
    BsDemux Bitstream callback info
*/
typedef struct _IAMOVIEPLAY_BSDEMUX_BS_INFO {
    UINT64 bs_offset;   ///< Bit-Stream offset for FileIn module.
    UINT32 bs_addr;     ///< Bit-Stream address
    UINT32 bs_addr_va;     ///< Bit-Stream virtual address
	UINT32 bs_size;     ///< Bit-Stream size
	UINT32 frm_idx;     ///< Frame Index
	UINT32 bufID;       ///< Buffer ID
	UINT64 timestamp;   ///< Timestamp
	BOOL   bIsAud;      ///< Is is audio or not
	BOOL   bIsEOF;      ///< End of Frame flag
} IAMOVIEPLAY_BSDEMUX_BS_INFO, *PIAMOVIEPLAY_BSDEMUX_BS_INFO;

extern ER iamovieplay_SetVdoTimerIntval(UINT32 Id);
extern ER iamovieplay_TskStart(void);
extern ER iamovieplay_TskDestroy(void);
extern ER iamovieplay_StartPlay(IAMOVIEPLAY_SPEED_TYPE speed, IAMOVIEPLAY_DIRECT_TYPE direct, INT32 disp_idx);
extern void iamovieplay_InstallID(void);
extern void iamovieplay_UninstallID(void);
extern INT32 _imageapp_movieplay_bsdemux_callback(HD_BSDEMUX_CB_EVENT event_id, HD_BSDEMUX_CBINFO *p_param);
extern ER   iamovieplay_SetPlayState(IAMOVIEPLAY_PLAY_STATE  state);
extern IAMOVIEPLAY_PLAY_STATE iamovieplay_GetPlayState(void);

extern void iamovieplay_set_dbg_level(UINT32 dbg_level);
extern void _iamovieplay_decrypt_hex_dump(MOVIEPLAY_DECRYPT_TYPE type, char* tag, UINT32 buf, UINT16 len);
extern void _iamovieplay_decrypt(UINT32 addr, UINT32 decrypt_size);
extern void _iamovieplay_bs_decrypt(UINT32 addr, UINT32 bs_size);

