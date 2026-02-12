/**
    @brief Header file of vf_gfx module.\n
    This file contains the functions which is related to vf_gfx in the chip.

    @file vf_gfx.h

    @ingroup mhdal

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _VF_GFX_H_
#define _VF_GFX_H_

/********************************************************************
    INCLUDE FILES
********************************************************************/
#include "hdal.h"

/********************************************************************
    MACRO CONSTANT DEFINITIONS
********************************************************************/

#define VF_GFX_LINEOFFSET_PATTERN     0x80000000 

/********************************************************************
    MACRO FUNCTION DEFINITIONS
********************************************************************/

/********************************************************************
    TYPE DEFINITION
********************************************************************/

typedef struct _VF_GFX_COPY {
	HD_VIDEO_FRAME          src_img;
	HD_VIDEO_FRAME          dst_img;
	HD_IRECT                src_region;
	HD_IPOINT               dst_pos;
	UINT32                  colorkey;
	UINT32                  alpha;
	UINT32                  engine;
} VF_GFX_COPY;

typedef struct _VF_GFX_SCALE {
	HD_VIDEO_FRAME          src_img;
	HD_VIDEO_FRAME          dst_img;
	HD_IRECT                src_region;
	HD_IRECT                dst_region;
	HD_GFX_SCALE_QUALITY    quality;
	UINT32                  engine;
} VF_GFX_SCALE;

typedef struct _VF_GFX_ROTATE {
	HD_VIDEO_FRAME          src_img;
	HD_VIDEO_FRAME          dst_img;
	HD_IRECT                src_region;
	HD_IPOINT               dst_pos;
	UINT32                  angle;
	UINT32                  engine;
} VF_GFX_ROTATE;

typedef struct _VF_GFX_DRAW_RECT {
	HD_VIDEO_FRAME          dst_img;
	UINT32                  color;
	HD_IRECT                rect;
	HD_GFX_RECT_TYPE        type;
	UINT32                  thickness;
	UINT32                  engine;
} VF_GFX_DRAW_RECT;

/********************************************************************
    EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/

HD_RESULT vf_init(HD_VIDEO_FRAME *p_img_buf, UINT32 width, UINT32 height, HD_VIDEO_PXLFMT pxlfmt, UINT32 lineoff, UINT32 addr, UINT32 available_size);

HD_RESULT vf_init_ex(HD_VIDEO_FRAME *p_img_buf, UINT32 width, UINT32 height, HD_VIDEO_PXLFMT pxlfmt, UINT32 lineoff[HD_VIDEO_MAX_PLANE], UINT32 pxladdr[HD_VIDEO_MAX_PLANE]);

HD_RESULT vf_gfx_copy(VF_GFX_COPY *p_param);

HD_RESULT vf_gfx_scale(VF_GFX_SCALE *p_param, int flush);

HD_RESULT vf_gfx_rotate(VF_GFX_ROTATE *p_param);

HD_RESULT vf_gfx_draw_rect(VF_GFX_DRAW_RECT *p_param);

HD_RESULT vf_gfx_I8_colorkey(VF_GFX_COPY *p_param);

#endif //_VF_GFX_H_

