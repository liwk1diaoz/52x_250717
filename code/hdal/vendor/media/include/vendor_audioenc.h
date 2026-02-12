/**
	@brief Header file of vendor audioenc module.\n
	This file contains the functions which is related to vendor audioenc.

	@file vendor_audioenc.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _VENDOR_AUDIOENC_H_
#define _VENDOR_AUDIOENC_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define VENDOR_AUDIOENC_NAME_LEN 32

/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/

/********************************************************************
	TYPE DEFINITION
********************************************************************/
typedef enum _VENDOR_AUDIOENC_ITEM {
	VENDOR_AUDIOENC_ITEM_BS_RESERVED_SIZE,      ///< reserved size configuration
	VENDOR_AUDIOENC_ITEM_AAC_VER,               ///< aac ADTS MPEG version
	VENDOR_AUDIOENC_ITEM_DATA_QUEUE_NUM,        ///< input data queue num
	VENDOR_AUDIOENC_ITEM_TARGET_RATE,           ///< target bitrate. Only support AAC.
	ENUM_DUMMY4WORD(VENDOR_AUDIOENC_ITEM)
} VENDOR_AUDIOENC_ITEM;

typedef enum _VENDOR_AUDIOENC_AAC_VER {
	VENDOR_AUDIOENC_AAC_VER_MPEG4 = 0,
	VENDOR_AUDIOENC_AAC_VER_MPEG2 = 1,
	ENUM_DUMMY4WORD(VENDOR_AUDIOENC_AAC_VER)
} VENDOR_AUDIOENC_AAC_VER;

typedef enum {
	VENDOR_AUDIOENC_BITRATE_16000                = 16000,
	VENDOR_AUDIOENC_BITRATE_32000                = 32000,
	VENDOR_AUDIOENC_BITRATE_48000                = 48000,
	VENDOR_AUDIOENC_BITRATE_64000                = 64000,
	VENDOR_AUDIOENC_BITRATE_80000                = 80000,
	VENDOR_AUDIOENC_BITRATE_96000                = 96000,
	VENDOR_AUDIOENC_BITRATE_112000               = 112000,
	VENDOR_AUDIOENC_BITRATE_128000               = 128000,
	VENDOR_AUDIOENC_BITRATE_144000               = 144000,
	VENDOR_AUDIOENC_BITRATE_160000               = 160000,
	VENDOR_AUDIOENC_BITRATE_192000               = 192000,
	ENUM_DUMMY4WORD(VENDOR_AUDIOENC_BITRATE)
} VENDOR_AUDIOENC_BITRATE;

typedef struct _VENDOR_AUDIOENC_BS_RESERVED_SIZE_CFG {
	UINT32 reserved_size;
} VENDOR_AUDIOENC_BS_RESERVED_SIZE_CFG;

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
HD_RESULT vendor_audioenc_set(UINT32 id, VENDOR_AUDIOENC_ITEM item, VOID *p_param);
HD_RESULT vendor_audioenc_get(UINT32 id, VENDOR_AUDIOENC_ITEM item, VOID *p_param);
#endif

