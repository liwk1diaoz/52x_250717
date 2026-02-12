/**
    Header file of Novatek Media Bitstream Demux Task

    Exported header file of novatek media bitstream demux task.

    @file       bsdemux_tsk.h
    @ingroup    mIAPPMEDIAPLAY

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _NMEDIABSDEMUXTSK_H
#define _NMEDIABSDEMUXTSK_H

#include <stdio.h>
#include <string.h>
#include "kwrap/type.h"
#include "bsdemux.h"
#include "FileSysTsk.h"
#include "hd_bsdemux_lib.h"

#define SIZE_32X(a)                  (((a + 31)>>5)<<5)

/**
    Video/Audio default pre-demux number (only for MP4)
*/
#define NMP_BSDEMUX_VDO_PRENUM       30
#define NMP_BSDEMUX_AUD_PRENUM       30


/**
    NMP Audio Decoder Bit-Stream Decode Sample Per One Entry
*/
#define  NMP_BSDEMUX_BS_DECODE_SAMPLES					1024

/**
    Flags for NMediaPlay BsDemux
*/
#define FLG_NMP_BSDEMUX_IDLE         FLGPTN_BIT(0)
#define FLG_NMP_BSDEMUX_START        FLGPTN_BIT(1)                  ///< start video/audio demux
#define FLG_NMP_BSDEMUX_VDO_DEMUX    FLGPTN_BIT(2)                  ///< video bitstream demux
#define FLG_NMP_BSDEMUX_AUD_DEMUX    FLGPTN_BIT(3)                  ///< audio bitstream demux
#define FLG_NMP_BSDEMUX_STOP         FLGPTN_BIT(4)                  ///< stop bsdemuxer
#define FLG_NMP_BSDEMUX_STOP_DONE    FLGPTN_BIT(5)                  ///< stop done

/**
    File format type definition
*/
typedef enum {
	NMP_BSDEMUX_FILEFMT_MP4 = MEDIA_FILEFORMAT_MP4,
	NMP_BSDEMUX_FILEFMT_TS  = MEDIA_FILEFORMAT_TS,
	ENUM_DUMMY4WORD(NMP_BSDEMUX_FILEFMT)
} NMP_BSDEMUX_FILEFMT;

/**
    NMediaPlay BsDemux Object
*/
typedef struct _NMP_BSDEMUX_OBJ {
	NMP_BSDEMUX_FILEFMT     file_fmt;                               ///< file format (mp4/mov or ts)
	UINT32                  vdo_codec;                              ///< video codec type
    UINT32                  vdo_wid;                                ///< video width
    UINT32                  vdo_hei;                                ///< video height
    UINT32                  vdo_gop;                                ///< video gop number
    UINT32                  desc_size;                              ///< h.26x description size
    BOOL                    vdo_en;                                 ///< video enable
    BOOL                    aud_en;                                 ///< audio enable
    NMP_BSDEMUX_ALLOC_MEM   alloc_mem;                              ///< allocated memory information
    NMP_BSDEMUX_FILE_INFO   file_info;                              ///< file information
    NMP_BSDEMUX_IN_VQ       vdo_que;                                ///< (input) video queue
    NMP_BSDEMUX_IN_AQ       aud_que;                                ///< (input) audio queue
    HD_BSDEMUX_CALLBACK     callback;                               ///< event callback function
} NMP_BSDEMUX_OBJ, *PNMP_BSDEMUX_OBJ;

#endif
