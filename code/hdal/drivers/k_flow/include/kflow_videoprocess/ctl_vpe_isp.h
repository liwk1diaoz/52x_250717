/**
    IPL Ctrl Layer, ISP Interface

    @file       ctl_vpe_isp.h
    @ingroup    mIVPE_Ctrl
    @note       None

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/


#ifndef _CTL_VPE_ISP_H
#define _CTL_VPE_ISP_H

#include "kflow_common/isp_if.h"
#include "kflow_common/type_vdo.h"

#define CTL_VPE_ISP_NOISE_CURVE_NUMS		17
#define CTL_VPE_ISP_GEO_LUT_NUMS			65
#define CTL_VPE_ISP_2D_LUT_NUMS				(260*257)	/* 257x257 with width 4x align */
#define CTL_VPE_ISP_OUT_PATH_NUMS			(4)

typedef enum {
	CTL_VPE_ISP_ITEM_IQ_PARAM = 0,					///< [SET], 	data_type: CTL_VPE_ISP_IQ_ALL
	CTL_VPE_ISP_ITEM_MAX,
} CTL_VPE_ISP_ITEM;

typedef struct {
	UINT8  enable;
	UINT8  edge_weight_src_sel; 					///< Select source of edge weight calculation, 0~1
	UINT8  edge_weight_th; 							///< Edge weight coring threshold, 0~255
	UINT8  edge_weight_gain; 						///< Edge weight gain, 0~255
	UINT8  noise_level; 							///< Noise Level, 0~255
	UINT8  noise_curve[CTL_VPE_ISP_NOISE_CURVE_NUMS]; ///< 17 control points of noise modulation curve, 0~255
	UINT8  blend_inv_gamma; 						///< Blending ratio of HPF results, 0~128
	UINT8  edge_sharp_str1; 						///< Sharpen strength1 of edge region, 0~255
	UINT8  edge_sharp_str2; 						///< Sharpen strength2 of edge region, 0~255
	UINT8  flat_sharp_str; 							///< Sharpen strength of flat region,0~255
	UINT8  coring_th; 								///< Coring threshold, 0~255
	UINT8  bright_halo_clip; 						///< Bright halo clip ratio, 0~255
	UINT8  dark_halo_clip; 							///< Dark halo clip ratio, 0~255
	UINT8  sharpen_out_sel;
} CTL_VPE_ISP_SHARPEN_PARAM;

typedef enum {
	CTL_VPE_ISP_DCE_MODE_GDC_ONLY = 0,
	CTL_VPE_ISP_DCE_MODE_2DLUT_ONLY,
	CTL_VPE_ISP_DCE_MODE_2DLUT_DCTG,
	CTL_VPE_ISP_DCE_MODE_MAX,
} CTL_VPE_ISP_DCE_MODE;

/* Note that 2dlut width need 4x align
	that means 65x65 table will be 68x65 table with trail dummy 0
*/
typedef enum {
	CTL_VPE_ISP_2DLUT_SZ_9X9 = 0,
	CTL_VPE_ISP_2DLUT_SZ_65X65 = 3,
	CTL_VPE_ISP_2DLUT_SZ_129X129 = 4,
	CTL_VPE_ISP_2DLUT_SZ_257X257 = 5,
} CTL_VPE_ISP_2DLUT_SZ;

typedef struct {
	UINT8 	fovbound;
	UINT16 	boundy;
	UINT16 	boundu;
	UINT16	boundv;
	UINT16 	xdist_a1;
	UINT16 	ydist_a1;
	UINT16 	fovgain;
	UINT16 	geo_lut[CTL_VPE_ISP_GEO_LUT_NUMS];
	UINT16 	cent_x_ratio;		/* cent_x will be calculate: (img_width * ratio / ratio_base) */
	UINT16 	cent_y_ratio;		/* cent_y will be calculate: (img_height * ratio / ratio_base) */
	UINT16  cent_ratio_base;	/* ratio base. cant not be zero */
} CTL_VPE_ISP_DCE_GDC_PARAM;

typedef enum {
	CTL_VPE_ISP_FLIP_NONE,
	CTL_VPE_ISP_FLIP_H,
	CTL_VPE_ISP_FLIP_V,
	CTL_VPE_ISP_FLIP_HV,
	CTL_VPE_ISP_FLIP_MAX,
} CTL_VPE_ISP_FLIP_TYPE;

typedef enum {
	CTL_VPE_ISP_ROT_RAT_FIX_ASPECT_RATIO_UP,
	CTL_VPE_ISP_ROT_RAT_FIX_ASPECT_RATIO_DOWN,
	CTL_VPE_ISP_ROT_RAT_FIT_ROI,
	CTL_VPE_ISP_ROT_RAT_MANUAL,
	CTL_VPE_ISP_ROT_RAT_MAX
} CTL_VPE_ISP_MANUAL_ROT_RATIO;

typedef struct {
	UINT32 rot_degree; 		//rot degree, 1 degree = 10, range:0~360 degree, val: 0 ~ 3600
	CTL_VPE_ISP_FLIP_TYPE flip;
	CTL_VPE_ISP_MANUAL_ROT_RATIO ratio_mode;
	UINT16 ratio; 			//100:1x 1 ~ max(depend image size & rot deg)
	UINT8  fovbound;
	UINT16 boundy;
	UINT16 boundu;
	UINT16 boundv;
}  CTL_VPE_ISP_MANUAL_ROT_PARAM;

typedef enum {
	CTL_VPE_ISP_ROTATE_0 = 0,		//rotate 0 degree
	CTL_VPE_ISP_ROTATE_90,			//rotate 90 degree
	CTL_VPE_ISP_ROTATE_180,			//rotate 180 degree
	CTL_VPE_ISP_ROTATE_270,			//rotate 270 degree
	CTL_VPE_ISP_H_FLIP_ROTATE_0,    //horizontal flip + rotate 0 degree
	CTL_VPE_ISP_H_FLIP_ROTATE_90,   //horizontal flip + rotate 90 degree
	CTL_VPE_ISP_H_FLIP_ROTATE_180,  //horizontal flip + rotate 180 degree
	CTL_VPE_ISP_H_FLIP_ROTATE_270,  //horizontal flip + rotate 270 degree
	CTL_VPE_ISP_ROTATE_MANUAL,		//manual mode, need set CTL_VPE_ISP_MANUAL_ROT_PARAM
	CTL_VPE_ISP_FLIP_ROTATE_MAX,
} CTL_VPE_ISP_FLIP_ROT_MODE;

typedef struct {
	CTL_VPE_ISP_FLIP_ROT_MODE flip_rot_mode;
	CTL_VPE_ISP_MANUAL_ROT_PARAM rot_manual_param;	//valid only when flip_rot_mode = CTL_VPE_ISP_ROTATE_MANUAL
}  CTL_VPE_ISP_FLIP_ROT_CTL;

typedef struct {
	UINT8 	xofs_i;
	UINT32 	xofs_f;
	UINT8 	yofs_i;
	UINT32 	yofs_f;
	CTL_VPE_ISP_2DLUT_SZ lut_sz;
	UINT32 	lut[CTL_VPE_ISP_2D_LUT_NUMS];
	USIZE 	out_size;	//2dlut output size
} CTL_VPE_ISP_DCE_2DLUT_PARAM;

typedef struct {
	UINT8 enable;
	UINT8 lsb_rand;
	CTL_VPE_ISP_DCE_MODE dce_mode;
	UINT32 lens_radius;
} CTL_VPE_ISP_DCE_CTL;

typedef enum {
	CTL_VPE_ISP_YUV_CVT_NONE = 0,
	CTL_VPE_ISP_YUV_CVT_PC,
	CTL_VPE_ISP_YUV_CVT_TV,
	CTL_VPE_ISP_YUV_CVT_MAX,
} CTL_VPE_ISP_YUV_CVT;

typedef enum {
	CTL_VPE_ISP_DCTG_2DLUT_SZ_9X9 = 0,
	CTL_VPE_ISP_DCTG_2DLUT_SZ_65X65 = 3,
} CTL_VPE_ISP_DCTG_2DLUT_SZ;

typedef struct _CTL_VPE_ISP_DCTG_PARAM_ {
	UINT8  enable;
	UINT8  mount_type;
	CTL_VPE_ISP_DCTG_2DLUT_SZ  lut2d_sz;
	UINT16 lens_r;
	UINT16 lens_x_st;
	UINT16 lens_y_st;
	INT32  theta_st;
	INT32  theta_ed;
	INT32  phi_st;
	INT32  phi_ed;
	INT32  rot_z;
	INT32  rot_y;
	USIZE  out_size;
} CTL_VPE_ISP_DCTG_PARAM;

typedef struct {
	CTL_VPE_ISP_YUV_CVT cvt_sel[CTL_VPE_ISP_OUT_PATH_NUMS];
} CTL_VPE_ISP_YUV_CVT_PARAM;

typedef struct {
	CTL_VPE_ISP_SHARPEN_PARAM	*p_sharpen_param;
	CTL_VPE_ISP_DCE_CTL			*p_dce_ctl;
	CTL_VPE_ISP_DCE_GDC_PARAM	*p_dce_gdc_param;
	CTL_VPE_ISP_DCE_2DLUT_PARAM	*p_dce_2dlut_param;
	CTL_VPE_ISP_YUV_CVT_PARAM   *p_yuv_cvt_param;
	CTL_VPE_ISP_DCTG_PARAM 	    *p_dctg_param;
	CTL_VPE_ISP_FLIP_ROT_CTL	*p_flip_rot_ctl;
} CTL_VPE_ISP_IQ_ALL;

typedef enum {
	CTL_VPE_ISP_CB_MSG_NONE = 0x00000000,
	CTL_VPE_ISP_CB_MAX_MAX,
} CTL_VPE_ISP_CB_MSG;

typedef struct {
	USIZE dce_in_size;
} CTL_VPE_ISP_CB_PARAM;

ER ctl_vpe_isp_evt_fp_reg(CHAR *name, ISP_EVENT_FP fp, ISP_EVENT evt, CTL_VPE_ISP_CB_MSG cb_msg);
ER ctl_vpe_isp_evt_fp_unreg(CHAR *name);
ER ctl_vpe_isp_set(ISP_ID id, CTL_VPE_ISP_ITEM item, void *data);
ER ctl_vpe_isp_get(ISP_ID id, CTL_VPE_ISP_ITEM item, void *data);

#endif