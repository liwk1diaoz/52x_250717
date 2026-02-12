/**
	@brief Header file of vendor videocapture module.\n
	This file contains the functions which is related to vendor videocapture.

	@file vendor_videocapture.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _VENDOR_VIDEOCAP_H_
#define _VENDOR_VIDEOCAP_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "hd_common.h"
#include "hd_videocapture.h"
/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define HD_VIDEO_PIX_Y             0x0500
/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/

/********************************************************************
	TYPE DEFINITION
********************************************************************/
typedef struct _VENDOR_VIDEOCAP_AE_PRESET {
	BOOL  enable;
	UINT32 exp_time;
	UINT32 gain_ratio;
} VENDOR_VIDEOCAP_AE_PRESET;

typedef struct _VENDOR_VIDEOCAP_FAST_OPEN_SENSOR {
	HD_VIDEOCAP_SEN_CONFIG  sen_cfg;
	UINT32 sen_mode;                         ///< referring to sensor driver, or set HD_VIDEOCAP_SEN_MODE_AUTO for AUTO selecting
	HD_VIDEO_FRC frc;                        ///< frame rate
	HD_DIM dim;                              ///< dim w,h. only valid when sen_mode is HD_VIDEOCAP_SEN_MODE_AUTO
	HD_VIDEO_PXLFMT pxlfmt;                  ///< pixel format. only valid when sen_mode is HD_VIDEOCAP_SEN_MODE_AUTO
	HD_VIDEOCAP_SEN_FRAME_NUM out_frame_num; ///< sensor output frame number, 1 for linear mode and 2/3/4 for sensor HDR mode.
	UINT32 data_lane;                        ///< set data lane number for selecting sensor mode
	BOOL builtin_hdr;                        ///< set TRUE for enabling built-in HDR mode
	VENDOR_VIDEOCAP_AE_PRESET ae_preset;     ///< ae preset
} VENDOR_VIDEOCAP_FAST_OPEN_SENSOR;

#define AD_CHIP(id)    ((((UINT32)(id)) & 0x000000ff)<<24)
#define AD_IN(ch)    ((((UINT32)(ch)) & 0x000000ff)<<16)
#define VENDOR_VIDEOCAP_AD_MAP(ad_chip, ad_in, vdocap_id) (AD_CHIP(ad_chip)|AD_IN(ad_in)|(vdocap_id))

typedef enum {
	VENDOR_VIDEOCAP_FIELD_DISABLE = 0,
	VENDOR_VIDEOCAP_FIELD_EN_0,
	VENDOR_VIDEOCAP_FIELD_EN_1,
	ENUM_DUMMY4WORD(VENDOR_VIDEOCAP_FIELD_SEL)
} VENDOR_VIDEOCAP_FIELD_SEL;

typedef enum {
	VENDOR_VIDEOCAP_CCIR_FMT_SEL_CCIR601 = 0,
	VENDOR_VIDEOCAP_CCIR_FMT_SEL_CCIR656,
	VENDOR_VIDEOCAP_CCIR_FMT_SEL_CCIR709,
	VENDOR_VIDEOCAP_CCIR_FMT_SEL_CCIR1120,
	VENDOR_VIDEOCAP_CCIR_FMT_SEL_MAX_NUM,
	ENUM_DUMMY4WORD(VENDOR_VIDEOCAP_CCIR_FMT_SEL)
} VENDOR_VIDEOCAP_CCIR_FMT_SEL;

typedef struct _VENDOR_VIDEOCAP_CCIR_INFO {
	VENDOR_VIDEOCAP_FIELD_SEL field_sel;
	VENDOR_VIDEOCAP_CCIR_FMT_SEL fmt;
	BOOL interlace;                         ///< enable interlace or not
	UINT32 mux_data_index;                  ///< mux data index
} VENDOR_VIDEOCAP_CCIR_INFO;

typedef struct {
	HD_DIM size;        ///< [out], current plug size
	UINT32 fps;         ///< [out], current plug fps
	BOOL   interlace;   ///< [out] TRUE:CTL_SEN_MODE_CCIR_INTERLACE, FALSE:CTL_SEN_MODE_CCIR
	UINT32 param[5];    ///< [out], current plug info
} VENDOR_VIDEOCAP_GET_PLUG_INFO;

typedef enum {
	VENDOR_VIDEOCAP_QUEUE_FLUSH_ALL = 0,   ///< default setting
	VENDOR_VIDEOCAP_QUEUE_FLUSH_OLDEST,
	ENUM_DUMMY4WORD(VENDOR_VIDEOCAP_QUEUE_FLUSH_SCHEME)
} VENDOR_VIDEOCAP_QUEUE_FLUSH_SCHEME;
typedef enum {
	VENDOR_VCAP_SEN_MODE_TYPE_UNKNOWN = 0,
	VENDOR_VCAP_SEN_MODE_LINEAR,               ///< 1 frame, linear raw
	VENDOR_VCAP_SEN_MODE_BUILTIN_HDR,          ///< 1 frame, bulid in HDR
	VENDOR_VCAP_SEN_MODE_CCIR,                 ///< 1 frame, ccir progressive
	VENDOR_VCAP_SEN_MODE_CCIR_INTERLACE,       ///< 1 frame, ccir interlace
	VENDOR_VCAP_SEN_MODE_RAW_PDAF,             ///< 1 frame, get pdaf from raw plane 3
	VENDOR_VCAP_SEN_MODE_BUILTIN_DCG_HDR,      ///< 1 frame, pwl & dcg strategy
	VENDOR_VCAP_SEN_MODE_STAGGER_HDR,          ///< 2 frame, stagger HDR
	VENDOR_VCAP_SEN_MODE_PDAF,                 ///< 2 frame, linear raw + pdaf
	VENDOR_VCAP_SEN_MODE_BUILTIN_DCG_SHDR,     ///< 2 frame, path 1: pwl, path 2: linear
	VENDOR_VCAP_SEN_MODE_STAGGER_PDAF,         ///< 2 frame, get pdaf from raw plane 3
	VENDOR_VCAP_SEN_MODE_DCG_HDR,              ///< 2 frame, dcg strategy
	VENDOR_VCAP_SEN_MODE_DCG_SHDR,             ///< 3 frame, path 1/2 dcg strategy
	VENDOR_VCAP_SEN_MODE_STAGGER3_HDR,         ///< 3 frame
	ENUM_DUMMY4WORD(VENDOR_VCAP_SEN_MODE_TYPE)
} VENDOR_VCAP_SEN_MODE_TYPE;

typedef struct {
	BOOL   en;          ///< enable gyro function
	UINT32 data_num;    ///< data number per axis
} VENDOR_VIDEOCAP_GYRO_INFO;

typedef enum _VENDOR_VIDEOCAP_PARAM_ID {
	VENDOR_VIDEOCAP_PARAM_FAST_OPEN_SENSOR,      ///< using device id, refer to VENDOR_VIDEOCAP_FAST_OPEN_SENSOR struct
	VENDOR_VIDEOCAP_PARAM_MCLK_SRC_SYNC_SET,     ///< for ctrl path, indicate which MCLKs of VCAP use the same clock source, bitwise setting for VIDEOCAPx, e.g. set 0x3 to sync VIDEOCAP0 and VIDEOCAP1
	VENDOR_VIDEOCAP_PARAM_BUILTIN_HDR,           ///< for i/o path, set TRUE for enabling built-in HDR mode
	VENDOR_VIDEOCAP_PARAM_DATA_LANE,             ///< for i/o path, set data lane number for selecting sensor mode
	VENDOR_VIDEOCAP_PARAM_CCIR_INFO,             ///< for i/o path, referring to VENDOR_VIDEOCAP_CCIR_INFO
	VENDOR_VIDEOCAP_PARAM_GET_PLUG,              ///< for ctrl path, get sensor plugged or unplugged
	VENDOR_VIDEOCAP_PARAM_GET_PLUG_INFO,         ///< for ctrl path, get sensor plugged info, referring to VENDOR_VIDEOCAP_GET_PLUG_INFO
	VENDOR_VIDEOCAP_PARAM_SET_FPS,               ///< for ctrl path, set sensor frame rate in running time and the fps should NOT greater HD_VIDEOCAP_IN.frc
	VENDOR_VIDEOCAP_PARAM_AD_MAP,                ///< for ctrl path, set Analog Decoder mapping, e.g. VENDOR_VIDEOCAP_AD_MAP(0, 0, HD_VIDEOCAP_0|HD_VIDEOCAP_1)
	VENDOR_VIDEOCAP_PARAM_AD_TYPE,               ///< for ctrl path, set Analog Decoder type
	VENDOR_VIDEOCAP_PARAM_PDAF_MAP,              ///< for ctrl path, set PDAF sensor mapping, e.g. (HD_VIDEOCAP_0|HD_VIDEOCAP_1)
	VENDOR_VIDEOCAP_PARAM_SIE_MAP,               ///< for ctrl path, re-map VCAP to other SIE, e.g. 0 for SIE1, 1 for SIE2 and so on.
	VENDOR_VIDEOCAP_PARAM_ENC_RATE,              ///< for i/o path, set RAW compression ratio and only support 50, 58 and 66
	VENDOR_VIDEOCAP_PARAM_AE_PRESET,             ///< for i/o path, set AE data, referring to VENDOR_VIDEOCAP_AE_PRESET.
	VENDOR_VIDEOCAP_PARAM_BP3_RATIO,             ///< for i/o path, set SIE act_start to bp ratio(1~100)
	VENDOR_VIDEOCAP_PARAM_QUEUE_FLUSH_SCHEME,    ///< for i/o path, set the scheme for flushing queue in the condifiton of queue full, referring to VENDOR_VIDEOCAP_QUEUE_FLUSH_SCHEME
	VENDOR_VIDEOCAP_PARAM_CSI_ERR_CNT,           ///< for ctrl path, get CSI error count(UINT32)
	VENDOR_VIDEOCAP_PARAM_SW_VD_SYNC,            ///< for ctrl path, indicate which VCAPs use software VD sync mechanism, bitwise setting for VIDEOCAPx, e.g. set 0x3 to sync VIDEOCAP0 and VIDEOCAP1
	VENDOR_VIDEOCAP_PARAM_MODE_TYPE,             ///< for i/o path, set sensor mode type, referring to VENDOR_VCAP_SEN_MODE_TYPE
	VENDOR_VIDEOCAP_PARAM_RESET_FC,              ///< for i/o path, reset the frame count of VCAPs
	VENDOR_VIDEOCAP_PARAM_GYRO_INFO,             ///< for i/o path, referring to VENDOR_VIDEOCAP_GYRO_INFO
	ENUM_DUMMY4WORD(VENDOR_VIDEOCAP_PARAM_ID)
} VENDOR_VIDEOCAP_PARAM_ID;
/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
HD_RESULT vendor_videocap_set(UINT32 id, VENDOR_VIDEOCAP_PARAM_ID param_id, VOID *p_param);
HD_RESULT vendor_videocap_get(UINT32 id, VENDOR_VIDEOCAP_PARAM_ID param_id, VOID *p_param);
#endif

