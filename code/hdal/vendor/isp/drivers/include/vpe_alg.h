#ifndef _VPE_ALG_H_
#define _VPE_ALG_H_

#if defined(__KERNEL__) || defined(__FREERTOS)
#include "kwrap/type.h"
#endif

//=============================================================================
// struct & definition
//=============================================================================
#define VPE_GDC_LUT_NUMS 65
#define VPE_2DLUT_NUM 260*257
#define VPE_DRT_PATH_NUM 4

/**
	VPE process id
*/
typedef enum _VPE_ID {
	VPE_ID_1 = 0,                      ///< vpe id 1
	VPE_ID_2,                          ///< vpe id 2
	VPE_ID_3,                          ///< vpe id 3
	VPE_ID_4,                          ///< vpe id 4
	VPE_ID_5,                          ///< vpe id 5
	VPE_ID_6,                          ///< vpe id 6
	VPE_ID_7,                          ///< vpe id 7
	VPE_ID_8,                          ///< vpe id 8
	VPE_ID_MAX_NUM,
	ENUM_DUMMY4WORD(VPE_ID)
} VPE_ID;

typedef enum _VPE_ISP_DCE_MODE {
	VPE_ISP_DCE_MODE_GDC_ONLY = 0,
	VPE_ISP_DCE_MODE_2DLUT_ONLY,
	VPE_ISP_DCE_MODE_2DLUT_DCTG,
	VPE_ISP_DCE_MODE_MAX,
	ENUM_DUMMY4WORD(VPE_ISP_DCE_MODE)
} VPE_ISP_DCE_MODE;

typedef enum _VPE_ISP_2DLUT_SZ {
	VPE_ISP_2DLUT_SZ_9X9 = 0,
	VPE_ISP_2DLUT_SZ_65X65 = 3,
	VPE_ISP_2DLUT_SZ_129X129 = 4,
	VPE_ISP_2DLUT_SZ_257X257 = 5,
	VPE_ISP_2DLUT_SZ_MAX,
	ENUM_DUMMY4WORD(VPE_ISP_2DLUT_SZ)
} VPE_ISP_2DLUT_SZ;

typedef enum _VPE_ISP_FLIP_TYPE {
	VPE_ISP_FLIP_NONE,
	VPE_ISP_FLIP_H,
	VPE_ISP_FLIP_V,
	VPE_ISP_FLIP_HV,
	VPE_ISP_FLIP_MAX,
	ENUM_DUMMY4WORD(VPE_ISP_FLIP_TYPE)
} VPE_ISP_FLIP_TYPE;

typedef enum _VPE_ISP_MANUAL_ROT_RATIO {
	VPE_ISP_ROT_RAT_FIX_ASPECT_RATIO_UP,
	VPE_ISP_ROT_RAT_FIX_ASPECT_RATIO_DOWN,
	VPE_ISP_ROT_RAT_FIT_ROI,
	VPE_ISP_ROT_RAT_NORMAL,
	VPE_ISP_ROT_RAT_MAX,
	ENUM_DUMMY4WORD(VPE_ISP_MANUAL_ROT_RATIO)
} VPE_ISP_MANUAL_ROT_RATIO;

typedef enum _VPE_ISP_FLIP_ROT_MODE {
	VPE_ISP_ROTATE_0 = 0,       //rotate 0 degree
	VPE_ISP_ROTATE_90,          //rotate 90 degree
	VPE_ISP_ROTATE_180,         //rotate 180 degree
	VPE_ISP_ROTATE_270,         //rotate 270 degree
	VPE_ISP_H_FLIP_ROTATE_0,    //horizontal flip + rotate 0 degree
	VPE_ISP_H_FLIP_ROTATE_90,   //horizontal flip + rotate 90 degree
	VPE_ISP_H_FLIP_ROTATE_180,  //horizontal flip + rotate 180 degree
	VPE_ISP_H_FLIP_ROTATE_270,  //horizontal flip + rotate 270 degree
	VPE_ISP_ROTATE_MANUAL,      //manual mode, need set CTL_VPE_ISP_MANUAL_ROT_PARAM
	VPE_ISP_FLIP_ROTATE_MAX,
	ENUM_DUMMY4WORD(VPE_ISP_FLIP_ROT_MODE)
} VPE_ISP_FLIP_ROT_MODE;

typedef struct _VPE_SHARPEN_PARAM {
	UINT8 enable;
	UINT8 edge_weight_gain;
	UINT8 edge_sharp_str1;
	UINT8 edge_sharp_str2;
	UINT8 flat_sharp_str;
} VPE_SHARPEN_PARAM;

typedef struct _VPE_DCE_CTL_PARAM {
	UINT8 enable;
	VPE_ISP_DCE_MODE dce_mode;
	UINT32 lens_radius;
} VPE_DCE_CTL_PARAM;

typedef struct _VPE_GDC_PARAM {
	UINT16 	fovgain;
	UINT16 	gdc_lut[VPE_GDC_LUT_NUMS];
	UINT16 	cent_x_ratio;		/* cent_x will be calculate: (img_width * ratio / ratio_base) */
	UINT16 	cent_y_ratio;		/* cent_y will be calculate: (img_height * ratio / ratio_base) */
} VPE_GDC_PARAM;

typedef struct _VPE_2DLUT_PARAM {
	VPE_ISP_2DLUT_SZ lut_sz;
	UINT32 	lut[VPE_2DLUT_NUM];
} VPE_2DLUT_PARAM;

typedef enum {
	VPE_YUV_CVT_NONE = 0,
	VPE_YUV_CVT_PC,
	VPE_YUV_CVT_TV,
	VPE_YUV_CVT_MAX,
} VPE_YUV_CVT;

typedef struct _VPE_DRT_PARAM {
	VPE_YUV_CVT cvt_sel[VPE_DRT_PATH_NUM];
} VPE_DRT_PARAM;

typedef enum {
	VPE_ISP_DCTG_2DLUT_SZ_9X9 = 0,
	VPE_ISP_DCTG_2DLUT_SZ_65X65 = 3,
} VPE_ISP_DCTG_2DLUT_SZ;

typedef struct _VPE_DCTG_PARAM {
	UINT8  mount_type;
	VPE_ISP_DCTG_2DLUT_SZ  lut2d_sz;
	UINT16 lens_r;
	UINT16 lens_x_st;
	UINT16 lens_y_st;
	INT32  theta_st;
	INT32  theta_ed;
	INT32  phi_st;
	INT32  phi_ed;
	INT32  rot_z;
	INT32  rot_y;
} VPE_DCTG_PARAM;

typedef struct _VPE_MANUAL_ROT_PARAM {
	UINT32 rot_degree;      //rot degree, 1 degree = 10, range:0~360 degree, val: 0 ~ 3600
	VPE_ISP_FLIP_TYPE flip;
	VPE_ISP_MANUAL_ROT_RATIO ratio_mode;
	UINT8  fovbound;
	UINT16 boundy;
	UINT16 boundu;
	UINT16 boundv;
} VPE_MANUAL_ROT_PARAM;

typedef struct _VPE_FLIP_ROT_PARAM {
	VPE_ISP_FLIP_ROT_MODE flip_rot_mode;
	VPE_MANUAL_ROT_PARAM rot_manual_param;   //valid only when flip_rot_mode = CTL_VPE_ISP_ROTATE_MANUAL
} VPE_FLIP_ROT_PARAM;

typedef struct _VPE_PARAM_PTR {
	VPE_SHARPEN_PARAM     *sharpen;
	VPE_DCE_CTL_PARAM     *dce_ctl;
	VPE_GDC_PARAM         *gdc;
	VPE_2DLUT_PARAM       *lut2d;
	VPE_DRT_PARAM         *drt;
	VPE_DCTG_PARAM        *dctg;
	VPE_FLIP_ROT_PARAM    *flip_rot;
} VPE_PARAM_PTR;

#endif
