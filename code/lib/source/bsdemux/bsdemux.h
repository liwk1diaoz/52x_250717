#ifndef _NMEDIAPLAYBSDEMUX_H
#define _NMEDIAPLAYBSDEMUX_H

#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"
#include "kwrap/platform.h"
#include "kwrap/stdio.h"
#include "sxcmd_wrapper.h"

#define    NMP_BSDEMUX_US_IN_SECOND       (1000*1000)
#define    NMP_BSDEMUX_MAX_PATH           16

// queue
//#define    NMP_BSDEMUX_INQ_MAX            4
#define    NMP_BSDEMUX_IN_VQ_MAX          240                 ///< max_frame_rate
#define    NMP_BSDEMUX_IN_AQ_MAX          50                  ///< max_sample_rate / sample_per_frame(=1024)
//#define    NMP_BSDEMUX_OUTQ_MAX           30

typedef enum {
	NMP_BSDEMUX_PARAM_START,
	NMP_BSDEMUX_PARAM_MAX_MEM_INFO = NMP_BSDEMUX_PARAM_START,     ///< set max buf info
	NMP_BSDEMUX_PARAM_VDO_CODEC,                              ///< video codec
	NMP_BSDEMUX_PARAM_VDO_WID,                                ///< video width
	NMP_BSDEMUX_PARAM_VDO_HEI,                                ///< video height
	NMP_BSDEMUX_PARAM_VDO_ENABLE,                             ///< video enable (default: 0)
	NMP_BSDEMUX_PARAM_AUD_ENABLE,                             ///< audio enable (default: 0)
	NMP_BSDEMUX_PARAM_CONTAINER,                              ///< media container
	NMP_BSDEMUX_PARAM_VDO_DECRYPT,                            ///< video decrypt
	NMP_BSDEMUX_PARAM_VDO_GOP,                                ///< video group of picture number
	NMP_BSDEMUX_PARAM_DESC_SIZE,                              ///< h.26x description size (sps, pps, ...)
	NMP_BSDEMUX_PARAM_MEM_RANGE,                              ///< set memory range (allocate memory)
	NMP_BSDEMUX_PARAM_BUF_STARTADDR,                          ///< file buffer start addr
	NMP_BSDEMUX_PARAM_BLK_TTNUM,                              ///< total file block number
	NMP_BSDEMUX_PARAM_BLK_SIZE,                               ///< file block size
	NMP_BSDEMUX_PARAM_REG_CB,                                 ///< register callback
	NMP_BSDEMUX_PARAM_TRIG_TSDMX,                             ///< TS only, trigger TS demux
	NMP_BSDEMUX_PARAM_FILESIZE,                               ///< TS only, total file size
	NMP_BSDEMUX_PARAM_TSDMX_BUFBLK,                           ///< TS only, TS demux buffer block number
	NMP_BSDEMUX_PARAM_TSDMX_BUF_RESET,                        ///< TS only, Reset accum size
	NMP_BSDEMUX_PARAM_FILE_REMAIN_SIZE,						  ///< TS only
	NMP_BSDEMUX_PARAM_MAX,
} NMP_BSDEMUX_PARAM;

typedef enum {
	NMP_BSDEMUX_ACTION_PRELOADBS,                             ///< pre-load bitstream
	NMP_BSDEMUX_ACTION_START_VDO,                             ///< set interval and start timer trigger
	NMP_BSDEMUX_ACTION_STOP,                                  ///< stop timer trigger
	NMP_BSDEMUX_ACTION_START,                                 ///< start play
	NMP_BSDEMUX_ACTION_PAUSE,                                 ///< pause play
	NMP_BSDEMUX_ACTION_RESUME,                                ///< resume play
	NMP_BSDEMUX_ACTION_MAX,
} NMP_BSDEMUX_ACTION;

typedef enum {
	NMP_BSDEMUX_EVENT_PUSH_VDOBS,
    NMP_BSDEMUX_EVENT_PUSH_AUDBS,
   	NMP_BSDEMUX_EVENT_READ_FILE,
   	NMP_BSDEMUX_EVENT_REFRESH_VDO,
	NMP_BSDEMUX_EVENT_MAX,
	ENUM_DUMMY4WORD(NMP_BSDEMUX_CB_EVENT)
} NMP_BSDEMUX_CB_EVENT;

/**
	bitstream type (video / audio)
*/
typedef enum _NMP_BSDEMUX_BS_TPYE {
	NMP_BSDEMUX_BS_TPYE_VIDEO = 0x00000001, ///< video bitstream type
	NMP_BSDEMUX_BS_TPYE_AUDIO = 0x00000002, ///< audio bitstream type
	ENUM_DUMMY4WORD(NMP_BSDEMUX_BS_TPYE)
} NMP_BSDEMUX_BS_TPYE;

/**
	allocated memory information
*/
typedef struct _NMP_BSDEMUX_ALLOC_MEM {
	UINT32 pa;               ///< physical addr
	UINT32 va;               ///< virtual addr
	UINT32 size;             ///< memory size
} NMP_BSDEMUX_ALLOC_MEM;

/**
	file related information
*/
typedef struct _NMP_BSDEMUX_FILE_INFO {
	UINT32 buf_start_addr;         ///< read file buffer start addr
	UINT64 blk_size;               ///< file block size
	UINT32 tt_blknum;              ///< total file block number
} NMP_BSDEMUX_FILE_INFO;

typedef struct _NMP_BSDEMUX_MAX_MEM_INFO {
	UINT32   file_fmt;   ///< file format (MP4, TS)
} NMP_BSDEMUX_MAX_MEM_INFO, *PNMP_BSDEMUX_MAX_MEM_INFO;

/**
	(only for MP4) starting bitstream information
*/
/*typedef struct _NMP_BSDEMUX_START_BS {
	NMP_BSDEMUX_BS_TPYE bs_type;   ///< bitstream type (video / audio)
	UINT32              index;     ///< bitstream index
} NMP_BSDEMUX_START_BS;*/

/*typedef struct _NMP_BSDEMUX_METADATA {
	UINT32                        sign;     ///< signature = MAKEFOURCC(?,?,?,?)
	struct _NMP_BSDEMUX_METADATA* p_next;   ///< pointer to next meta
} NMP_BSDEMUX_METADATA;*/

/**
    input buffer information
*/
typedef struct _NMP_BSDEMUX_IN_BUF {
	//NMP_BSDEMUX_START_BS start_bs;   ///< (only for MP4) starting bitstream information
	//UINT64               offset;     ///< file offset of input buffer
	//UINT32               addr;       ///< starting addr of input buffer
	//UINT32               size;       ///< size of input buffer
	NMP_BSDEMUX_BS_TPYE   bs_type;   ///< bitstream type (video / audio)
	UINT32                index;     ///< bitstream index
} NMP_BSDEMUX_IN_BUF;

typedef struct _NMP_BSDEMUX_IN_VQ {
	UINT32               front;              ///< Front pointer
	UINT32               rear;               ///< Rear pointer
	UINT32               bfull;              ///< Full flag
	NMP_BSDEMUX_IN_BUF   in_buf[NMP_BSDEMUX_IN_VQ_MAX];
} NMP_BSDEMUX_IN_VQ;

typedef struct _NMP_BSDEMUX_IN_AQ {
	UINT32               front;              ///< Front pointer
	UINT32               rear;               ///< Rear pointer
	UINT32               bfull;              ///< Full flag
	NMP_BSDEMUX_IN_BUF   in_buf[NMP_BSDEMUX_IN_AQ_MAX];
} NMP_BSDEMUX_IN_AQ;

/**
	output buffer information
*/
/*typedef struct _NMP_BSDEMUX_OUT_BUF {
	//NMP_BSDEMUX_BS_TPYE    bs_type;     ///< bitstream type (video / audio)
	//NMP_BSDEMUX_METADATA*  p_next;      ///< pointer to next meta
	UINT32                 addr;        ///< bitstream addr
	UINT32                 size;        ///< bitstream size
	UINT32                 index;       ///< bitstream index
} NMP_BSDEMUX_OUT_BUF;

typedef struct _NMP_BSDEMUX_OUTQ {
	UINT32               front;              ///< Front pointer
	UINT32               rear;               ///< Rear pointer
	UINT32               bfull;              ///< Full flag
	NMP_BSDEMUX_OUT_BUF  out_buf[NMP_BSDEMUX_OUTQ_MAX];
} NMP_BSDEMUX_OUTQ;*/

/**
    bsdemux callback event type.
*/
/*typedef enum _NMP_BSDEMUX_CB_EVENT{
	NMP_BSDEMUX_CB_EVENT_VDOBS          = 0x00000001,
	NMP_BSDEMUX_CB_EVENT_AUDBS          = 0x00000002,
	ENUM_DUMMY4WORD(NMP_BSDEMUX_CB_EVENT)
} NMP_BSDEMUX_CB_EVENT;*/

//typedef void (NMP_BSDEMUX_CB)(NMP_BSDEMUX_CB_EVENT event, NMP_BSDEMUX_OUT_BUF *p_param);

extern void NMP_BsDemux_AddUnit(void);

#endif //_NMEDIAPLAYBSDEMUX_H

