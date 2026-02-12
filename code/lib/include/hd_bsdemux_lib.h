/**
	@brief Header file of bitstream demuxer module.\n
	This file contains the functions which is related to bitstream demuxer in the chip.

	@file hd_bsdemux_lib.h

	@ingroup mlib

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef	_HD_BSDEMUX_H_
#define	_HD_BSDEMUX_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "hd_util.h"
#include "avfile/MediaReadLib.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define HD_BSDEMUX_MAX_IN                 64      ///< max count of HD_IN() of this device (interface)
#define HD_BSDEMUX_MAX_OUT                64      ///< max count of HD_OUT() of this device (interface)
#define HD_BSDEMUX_MAX_DATA_TYPE          4       ///< max count of output pool of this device (interface)

/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/
#define HD_BSDEMUX_SET_COUNT(a, b)		((a)*10)+(b)	///< ex: use HD_BSDEMUX_SET_COUNT(1, 5) for setting 1.5

/********************************************************************
	TYPE DEFINITION
********************************************************************/
/**
     @name HD_BSDEMUX_IN capability
*/
typedef enum _HD_BSDEMUX_IN_CAPS {
	HD_BSDEMUX_IN_CAPS_MP4			= 0x00000001, ///< support bitstream demux from mp4
	HD_BSDEMUX_IN_CAPS_TS			= 0x00000002, ///< support bitstream demux from ts
	ENUM_DUMMY4WORD(HD_BSDEMUX_IN_CAPS)
} HD_BSDEMUX_IN_CAPS;

/**
     @name HD_BSDEMUX_OUT capability
*/
typedef enum _HD_BSDEMUX_OUT_CAPS {
	HD_BSDEMUX_OUT_CAPS_VIDEO		= 0x00000001, ///< support bitstream demux to video
	HD_BSDEMUX_OUT_CAPS_AUDIO		= 0x00000002, ///< support bitstream demux to audio
	ENUM_DUMMY4WORD(HD_BSDEMUX_OUT_CAPS)
} HD_BSDEMUX_OUT_CAPS;

/**
     @name HD_BSDEMUX system capability
*/
typedef struct _HD_BSDEMUX_SYSCAPS {
	HD_DAL               dev_id;                       ///< device id
	UINT32               chip;                         ///< chip id of hardware
	UINT32               max_in_count;                 ///< supported max count of HD_IN()
    UINT32               max_out_count;                ///< supported max count of HD_OUT()
    HD_DEVICE_CAPS       dev_caps;                     ///< capability of device, using HD_DEVICE_CAPS
	HD_BSDEMUX_IN_CAPS   in_caps[HD_BSDEMUX_MAX_IN];   ///< capability of input, using HD_BSDEMUX_CAPS
	HD_BSDEMUX_OUT_CAPS  out_caps[HD_BSDEMUX_MAX_OUT]; ///< capability of output, using HD_VIDEO_CAPS
} HD_BSDEMUX_SYSCAPS;

/**
	NOTE: for IPC SYSCAPS,

	max_in_count = 16;
	max_out_count = 16;
	dev_caps =
		HD_CAPS_PATHCONFIG;				///< require: set HD_BSDEMUX_PATH_CONFIG struct
	in_caps[0~16] =
		 HD_BSDEMUX_CAPS_MP4		  	///< support bitstream demux from mp4
		 | HD_BSDEMUX_CAPS_TS		    ///< support bitstream demux from ts
	out_caps[0~16] =
		 HD_BSDEMUX_OUT_CAPS_VIDEO		///< support bitstream demux to video
		 | HD_BSDEMUX_OUT_CAPS_AUDIO;	///< support bitstream demux to audio
*/

/**
	bitstream type (video / audio)
*/
typedef enum _HD_BSDEMUX_BS_TPYE {
	HD_BSDEMUX_BS_TPYE_VIDEO = 0x00000001, ///< video bitstream type
	HD_BSDEMUX_BS_TPYE_AUDIO = 0x00000002, ///< audio bitstream type
	ENUM_DUMMY4WORD(HD_BSDEMUX_BS_TPYE)
} HD_BSDEMUX_BS_TPYE;

/**
    bsdemux push in buf descriptor.
*/
typedef struct _HD_BSDEMUX_BUF {
	UINT32              sign;                       ///< MAKEFOURCC('B','S','D','X')
	//UINT32              addr;                       ///< data address
	//UINT64              size;                       ///< data size
	HD_BSDEMUX_BS_TPYE   bs_type;   ///< bitstream type (video / audio)
	UINT32               index;     ///< bitstream index
} HD_BSDEMUX_BUF;

/**
     bsdemux file format
*/
typedef enum _HD_BSDEMUX_FILE_FMT {
	HD_BSDEMUX_FILE_FMT_MP4			= 0x00000001, ///< mp4 file format
	HD_BSDEMUX_FILE_FMT_TS			= 0x00000002, ///< ts file format
	ENUM_DUMMY4WORD(HD_BSDEMUX_FILE_FMT)
} HD_BSDEMUX_FILE_FMT;

/**
    bsdemux decrypt information
*/
/*typedef struct _HD_BSDEMUX_DECRYPT_INFO {
	UINT32 addr;                                   ///< addr of decrypt key
	UINT32 len;                                    ///< size of decrypt key
} HD_BSDEMUX_DECRYPT_INFO;*/

/**
    bsdemux path configuration
*/
typedef struct _HD_BSDEMUX_PATH_CONFIG {
	HD_BSDEMUX_FILE_FMT file_fmt;               ///< file format
	CONTAINERPARSER     *container;             ///< file container
} HD_BSDEMUX_PATH_CONFIG;

/**
    bsdemux inport parameter
*/
typedef struct _HD_BSDEMUX_IN {
	HD_VIDEO_CODEC            video_codec;            ///< video codec type
	UINT32                    video_width;            ///< video width
	UINT32                    video_height;           ///< video height
	UINT32                    video_gop;              ///< video gop number
	UINT32                    desc_size;              ///< h.26x desc size
	UINT32                    video_en;               ///< enable video demux
	UINT32                    audio_en;               ///< enable audio demux
	UINT32                    decrypt_key;            ///< addr of decrypt key
} HD_BSDEMUX_IN;

/**
    bsdemux file related information
*/
typedef struct _HD_BSDEMUX_FILE {
	UINT32 buf_start_addr;         ///< read file buffer start addr
	UINT64 blk_size;               ///< file block size
	UINT32 tt_blknum;              ///< total file block number
} HD_BSDEMUX_FILE;

/**
    bsdemux callback event type.
*/
typedef enum _HD_BSDEMUX_CB_EVENT{
	HD_BSDEMUX_CB_EVENT_VIDEO_BS       = 0x00000001,
	HD_BSDEMUX_CB_EVENT_AUDIO_BS       = 0x00000002,
	HD_BSDEMUX_CB_EVENT_READ_VIDEO_BS  = 0x00000003,
	HD_BSDEMUX_CB_EVENT_READ_AUDIO_BS  = 0x00000004,
	ENUM_DUMMY4WORD(HD_BSDEMUX_CB_EVENT)
} HD_BSDEMUX_CB_EVENT;

/**
    bsdemux callback information.
    note: modified from uitron
*/
typedef struct _HD_BSDEMUX_CBINFO{
	UINT64   offset;      	/* bs file offset */
	UINT32   addr;        	/* bs address */
	UINT32   size;        	/* bs size */
	UINT32   index;       	/* bs index */
	BOOL	 eof;			/* indicate is last bs */
	BOOL 	 skippable;		/* inidicate is svc 1x p frame */
} HD_BSDEMUX_CBINFO;

/**
    bsdemux callback funcion prototype.

    @param HD_BSDEMUX_CB_EVENT event_id
    @param HD_BSDEMUX_CBINFO * param_1
*/
typedef INT32 (*HD_BSDEMUX_CALLBACK)(HD_BSDEMUX_CB_EVENT event_id, HD_BSDEMUX_CBINFO *p_param);

/**
    bsdemux callback info.
*/
typedef struct _HD_BSDEMUX_REG_CALLBACK {
	HD_BSDEMUX_CALLBACK callbackfunc;
} HD_BSDEMUX_REG_CALLBACK;

typedef enum _HD_BSDEMUX_PARAM_ID{
	HD_BSDEMUX_PARAM_DEVCOUNT,                    ///< support get with ctrl path, using HD_DEVCOUNT struct (device id max count)
	HD_BSDEMUX_PARAM_SYSCAPS,                     ///< support get with ctrl path, using HD_BSDEMUX_SYSCAPS
	HD_BSDEMUX_PARAM_PATH_CONFIG,                 ///< support get/set with i/o path, using HD_BSDEMUX_PATH_CONFIG
	HD_BSDEMUX_PARAM_IN,                          ///< support get/set with i/o path, using HD_BSDEMUX_IN struct
	HD_BSDEMUX_PARAM_FILE,                        ///< support get/set with i/o path, using HD_BSDEMUX_FILE struct
	HD_BSDEMUX_PARAM_REG_CALLBACK,                ///< register callback function for output data, using prototype HD_BSDEMUX_CALLBACK
	HD_BSDEMUX_PARAM_TRIG_TSDMX,
	HD_BSDEMUX_PARAM_FILESIZE,
	HD_BSDEMUX_PARAM_TSDMX_BUFBLK,
	HD_BSDEMUX_PARAM_TSDMX_BUF_RESET,
	HD_BSDEMUX_PARAM_FILE_REMAIN_SIZE,
	HD_BSDEMUX_PARAM_MAX,
	ENUM_DUMMY4WORD(HD_BSDEMUX_PARAM_ID)
} HD_BSDEMUX_PARAM_ID;

/********************************************************************
    EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
HD_RESULT hd_bsdemux_init(VOID);
HD_RESULT hd_bsdemux_open(HD_PATH_ID path_id);
//HD_RESULT hd_bsdemux_start(HD_PATH_ID path_id); // not used
//HD_RESULT hd_bsdemux_stop(HD_PATH_ID path_id); // not used
HD_RESULT hd_bsdemux_close(HD_PATH_ID path_id);
HD_RESULT hd_bsdemux_get(HD_PATH_ID path_id, HD_BSDEMUX_PARAM_ID id, VOID* p_param);
HD_RESULT hd_bsdemux_set(HD_PATH_ID path_id, HD_BSDEMUX_PARAM_ID id, VOID* p_param);
HD_RESULT hd_bsdemux_push_in_buf(HD_PATH_ID path_id, HD_BSDEMUX_BUF* p_in_bsdemux_buf, INT32 wait_ms);
HD_RESULT hd_bsdemux_uninit(VOID);

#endif

