/*
    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.

    @file       ImageApp_Play.h
    @ingroup    mIImageApp

    @note       Nothing.

    @date       2014/11/28
*/

#ifndef PLAY_H
#define PLAY_H

#include "hdal.h"
#include <kwrap/debug.h>
#include <kwrap/error_no.h>

/**
    @addtogroup mIImageApp
*/
//@{

enum {
	PLAY_CONFIG_DUAL_DISP  = 0xF000F000,
	PLAY_CONFIG_DISP_INFO,
};

enum {
	PLAY_PARAM_START = 0x00005000,
	PLAY_PARAM_PANELSZ = PLAY_PARAM_START,
	PLAY_PARAM_DISP_SIZE,
	PLAY_PARAM_MAX_RAW_SIZE,
	PLAY_PARAM_MAX_FILE_SIZE,
	PLAY_PARAM_DUAL_DISP,
	PLAY_PARAM_DISP_INFO,
	PLAY_PARAM_DEC_VID,
	PLAY_PARAM_PLAY_FMT,
	PLAY_PARAM_SUSPEND_CLOSE_IMM,
	PLAY_PARAM_POOL_ADDRESS,
	PLAY_PARAM_DEC_SAR,
	PLAY_PARAM_MAX_VIDEO_SIZE,
	PLAY_PARAM_MEM_CFG,
	PLAY_PARAM_PIXEL_FORMAT,
	PLAY_PARAM_MAX_DECODE_WIDTH,
	PLAY_PARAM_MAX_DECODE_HEIGHT,
	PLAY_PARAM_DECRYPT_INFO,
	PLAY_PARAM_MAX_
};

typedef enum {
	PLAY_DISP_ID_MIN = 0,
	PLAY_DISP_ID_1 = PLAY_DISP_ID_MIN,
	PLAY_DISP_ID_2,
	PLAY_DISP_ID_3,
	PLAY_DISP_ID_4,
	PLAY_DISP_ID_MAX,
	ENUM_DUMMY4WORD(PLAY_DISP_ID)
} PLAY_DISP_ID;

typedef enum{
	PLAY_HD_VDOOUT_TV   = 0,
	PLAY_HD_VDOOUT_LCD  = 1,
	PLAY_HD_VDOOUT_HDMI = 2,
	PLAY_HD_VDOOUT_ID_MAX,
	ENUM_DUMMY4WORD(PLAY_HD_VDOOUT_ID)
}PLAY_HD_VDOOUT_ID;

typedef struct {
	BOOL                 enable;
	PLAY_DISP_ID         disp_id;
	UINT32               width;
	UINT32               height;
	UINT32               width_ratio;
	UINT32               height_ratio;
	UINT32               rotate_dir;
} PLAY_DISP_INFO;

typedef struct _IMAGEAPP_PLAY {
	HD_VIDEODEC_SYSCAPS  vdec_syscaps;	
	HD_PATH_ID           vdec_path[4];
	HD_DIM               vdec_max_dim;
	UINT32               vdec_type;

	HD_VIDEOOUT_SYSCAPS  vout_syscaps;
	HD_PATH_ID           vout_ctrl;
	HD_PATH_ID           vout_path;
	HD_DIM               vout_max_dim;
	HD_DIM               vout_dim;
	HD_VIDEOOUT_HDMI_ID  hdmi_id;
	HD_VIDEO_PXLFMT      vout_pxlfmt;
	PLAY_HD_VDOOUT_ID    out_type;
} IMAGEAPP_PLAY_STREAM;

typedef struct {
	HD_PATH_ID vout_ctrl;
	HD_PATH_ID vout_path;
} IMAGEAPP_PLAY_CFG_DISP_INFO;

typedef enum {
	PLAY_DECRYPT_TYPE_NONE              = 0x00000000,
	PLAY_DECRYPT_TYPE_CONTAINER         = 0x00000001,
	PLAY_DECRYPT_TYPE_I_FRAME           = 0x00000002,
	PLAY_DECRYPT_TYPE_P_FRAME           = 0x00000004,
	PLAY_DECRYPT_TYPE_ALL_FRAME         = 0x00000006,
	PLAY_DECRYPT_TYPE_AUDIO             = 0x00000008,         // (N/A)
	ENUM_DUMMY4WORD(PLAY_ENCRYPT_TYPE)
} PLAY_DECRYPT_TYPE;

typedef enum {
	PLAY_DECRYPT_MODE_AES128            = 2,
	PLAY_DECRYPT_MODE_AES256			= 3,
	ENUM_DUMMY4WORD(PLAY_DECRYPT_MODE)
} PLAY_DECRYPT_MODE;

typedef struct {
	PLAY_DECRYPT_TYPE type;
	PLAY_DECRYPT_MODE mode;
	UINT8 *key;
	UINT32 key_len;
} PLAY_FILE_DECRYPT;

extern ER   ImageApp_Play_Open(void);
extern ER   ImageApp_Play_Close(void);
extern ER   ImageApp_Play_SetParam(UINT32 param, UINT32 value);
extern UINT32 ImageApp_Play_GetParam(UINT32 param);
//@}
#endif//PLAY_H
