/**
    Public header file for KDRV_IPP_utility

    This file is the header file that define data type for KDRV_IPP.

    @file       kdrv_ipp_utility.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _KDRV_IPP_UTILITY_H_
#define _KDRV_IPP_UTILITY_H_

#if defined __UITRON || defined __ECOS
#include "Type.h"
#include "mIType.h"
#else
#ifdef __KERNEL__
#include "kwrap/type.h"
#include "kwrap/nvt_type.h"
#endif
#endif

#define KDRV_IPP_VAR2STR(x)   (#x)
#define KDRV_IPP_DUMPD(x)      DBG_DUMP("%s = %d\r\n", KDRV_IPP_VAR2STR(x), x)
#define KDRV_IPP_DUMPH(x)      DBG_DUMP("%s = 0x%.8x\r\n", KDRV_IPP_VAR2STR(x), x)

/**
    IPP process ID
*/
typedef enum _KDRV_IPP_PROC_ID {
	KDRV_IPP_ID_1 = 0,                       ///< process id 0
	KDRV_IPP_ID_2,                           ///< process id 1
	KDRV_IPP_ID_3,                           ///< process id 2
	KDRV_IPP_ID_4,                           ///< process id 3
	KDRV_IPP_ID_5,                           ///< process id 5
	KDRV_IPP_ID_6,                           ///< process id 6
	KDRV_IPP_ID_7,                           ///< process id 7
	KDRV_IPP_ID_8,                           ///< process id 8
	KDRV_IPP_ID_MAX_NUM,
	ENUM_DUMMY4WORD(KDRV_IPP_PROC_ID)
} KDRV_IPP_PROC_ID;

/**
    callback function prototype
    UINT32 msg_type
    void* in_data
    void* out_data
*/
typedef INT32(*KDRV_IPP_ISRCB)(UINT32, UINT32, void *, void *);

/**
    YUV channel index
*/
//@{
typedef enum _KDRV_IPP_RAW_FMT {
	KDRV_IPP_RAW_BAYER_2x2 = 0,               ///< Bayer 2x2: R, Gr, Gb, B
	KDRV_IPP_RAW_RGBIR_4x4,                ///< RGBIR 4x4: R, Gr, Gir1, Ir1, B, Gb, Gir2, Ir2
	KDRV_IPP_RAWFMT_MAX_CH,
} KDRV_IPP_RAW_FMT;

/**
    YUV channel index
*/
//@{
typedef enum _KDRV_IPP_YUV_CH {
	KDRV_IPP_YUV_Y = 0,                  ///< process channel Y
	KDRV_IPP_YUV_U,                      ///< process channel U
	KDRV_IPP_YUV_V,                      ///< process channel V
	KDRV_IPP_YUV_MAX_CH,
	ENUM_DUMMY4WORD(KDRV_IPP_YUV_CH)
} KDRV_IPP_YUV_CH;

/**
    RGB channel index
*/
//@{
typedef enum {
	KDRV_IPP_RGB_R = 0,                  ///< process channel R
	KDRV_IPP_RGB_G,                      ///< process channel G
	KDRV_IPP_RGB_B,                      ///< process channel B
	KDRV_IPP_RGB_MAX_CH,
	ENUM_DUMMY4WORD(KDRV_IPP_RGB_CH)
} KDRV_IPP_RGB_CH;

/**
    IPL flip information
*/
typedef enum _KDRV_IPP_FLIP {
	KDRV_IPP_FLIP_NONE = 0x00000000,         ///< no flip
	KDRV_IPP_FLIP_H    = 0x00000001,         ///< H flip
	KDRV_IPP_FLIP_V    = 0x00000002,         ///< V flip
	KDRV_IPP_FLIP_H_V  = 0x00000003,         ///<H & V flip(Flip_H | Flip_V) don't modify
	KDRV_IPP_FLIP_MAX,
	ENUM_DUMMY4WORD(KDRV_IPP_FLIP)
} KDRV_IPP_FLIP;

/**
    IPL RAW Image start pixel
*/
typedef enum _KDRV_IPP_PIX {
	// RGB start pixel
	KDRV_IPP_RGGB_PIX_R = 0,  ///< start pixel R
	KDRV_IPP_RGGB_PIX_GR,     ///< start pixel Gr
	KDRV_IPP_RGGB_PIX_GB,     ///< start pixel Gb
	KDRV_IPP_RGGB_PIX_B,      ///< start pixel B
	KDRV_IPP_RGGB_PIX_MAX,

	// RGBIR data format
//	KDRV_IPP_RGBIR_PIX_RIR,   ///< start pixel RIR
//	KDRV_IPP_RGBIR_PIX_RG,    ///< start pixel RG
	KDRV_IPP_RGBIR_PIX_RG_GI,
	KDRV_IPP_RGBIR_PIX_GB_IG,
	KDRV_IPP_RGBIR_PIX_GI_BG,
	KDRV_IPP_RGBIR_PIX_IG_GR,
	KDRV_IPP_RGBIR_PIX_BG_GI,
	KDRV_IPP_RGBIR_PIX_GR_IG,
	KDRV_IPP_RGBIR_PIX_GI_RG,
	KDRV_IPP_RGBIR_PIX_IG_GB,
	KDRV_IPP_RGBIR_PIX_MAX,

	// RCCB start pixel
	KDRV_IPP_RCCB_PIX_RC,     ///< start pixel RC
	KDRV_IPP_RCCB_PIX_CR,     ///< start pixel CR
	KDRV_IPP_RCCB_PIX_CB,         ///< start pixel CB
	KDRV_IPP_RCCB_PIX_BC, ///< start pixel BC
	KDRV_IPP_RCCB_PIX_MAX,

	ENUM_DUMMY4WORD(KDRV_IPP_PIX)
} KDRV_IPP_PIX;


/**
    IPL RAW Image bits
*/
typedef enum _KDRV_IPP_RAW_BIT {
	KDRV_IPP_RAW_BIT_8 = 8,      ///< 8 bits
	KDRV_IPP_RAW_BIT_10 = 10,    ///< 10 bits
	KDRV_IPP_RAW_BIT_12 = 12,    ///< 12 bits
	KDRV_IPP_RAW_BIT_16 = 16,    ///< 16 bits
	KDRV_IPP_RAW_BIT_MAX_CNT,
	ENUM_DUMMY4WORD(KDRV_IPP_RAW_BIT)
} KDRV_IPP_RAW_BIT;

/**
    IPL YUV Image type
*/
typedef enum {
	KDRV_IPP_YUV444 = 0,        ///< 3 channel, yuv 444 planar
	KDRV_IPP_YUV422,            ///< 3 channel, yuv 422 planar
	KDRV_IPP_YUV420,            ///< 3 channel, yuv 420 planar
	KDRV_IPP_Y_PACK_UV444,  ///< 2 channel, y planar uv pack, 444 format
	KDRV_IPP_Y_PACK_UV422,  ///< 2 channel, y planar uv pack, 422 format
	KDRV_IPP_Y_PACK_UV420,  ///< 2 channel, y planar uv pack, 420 format
	KDRV_IPP_PACK_YUV444,   ///< 1 channel, yuv pack, 444 format
	KDRV_IPP_Y_ONLY,            ///< 1 channel, y only
	ENUM_DUMMY4WORD(KDRV_IPP_YUV_TYPE)
} KDRV_IPP_YUV_TYPE;

#endif //_KDRV_IPP_UTILITY_H_
