/*
    IME module driver

    NT96510 IME module driver.

    @file       ime_int.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/



#include "ime_int.h"


#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
//#include <Memory.h>


#include "Type.h"
#include "ErrorNo.h"
#include "DrvCommon.h"
#include "interrupt.h"
//#include "pll.h"
#include "pll_protected.h"
#include "top.h"
#include "Utility.h"

#endif


#include "ime_int.h"


static IME_STRIPE_MODE_SEL  ime_stripe_operation_mode = IME_SINGLE_STRIPE;

UINT32 ime_max_stp_isd_out_buf_size = 1344;
UINT32 ime_stp_size_max = 672, ime_stp_num_max = 255;


volatile NT98520_IME_REGISTER_STRUCT *imeg = NULL;

IME_OPMODE              ime_engine_operation_mode = IME_OPMODE_D2D;
//IME_HV_STRIPE_INFO      ime_stripe_info = {0x0};
IME_HV_STRIPE_INFO      ime_get_stripe_info = {0x0};
//IME_SCALE_FILTER_INFO   ime_scale_filter_params = {0x0};


IME_SCALE_FACTOR_INFO   ime_scale_factor_params = {0x0};


//IME_SCALE_FACTOR_INFO   ime_scale_factor_path1_params = {0x0};
//IME_SCALE_FACTOR_INFO   ime_scale_factor_path2_params = {0x0};
//IME_SCALE_FACTOR_INFO   ime_scale_factor_path3_params = {0x0};
//IME_SCALE_FACTOR_INFO   ime_scale_factor_path4_params = {0x0};
//IME_SCALE_FACTOR_INFO   ime_scale_factor_path5_params = {0x0};
//IME_SCALE_FACTOR_INFO   ime_scale_factor_lca_subout_params = {0x0};


//IME_DMA_ADDR_INFO ime_input_buffer_addr = {0x0}, ime_output_buffer_addr = {0x0};
//IME_DMA_ADDR_INFO ime_tmp_addr0 = {0x0}, ime_tmp_addr1 = {0x0};

//IME_SSR_MODE_SEL g_SsrMode = IME_SSR_MODE_USER;

//IME_LINEOFFSET_INFO ime_lofs_params = {0x0};
UINT32 ime_input_image_h_scale = 0, ime_input_image_v_scale = 0;
UINT32 ime_input_image_h_ori = 0, ime_input_image_v_ori = 0;

UINT32 ime_output_path1_h_scale = 0, ime_output_path1_v_scale = 0;
UINT32 ime_output_path2_h_scale = 0, ime_output_path2_v_scale = 0;
UINT32 ime_output_path3_h_scale = 0, ime_output_path3_v_scale = 0;
UINT32 ime_output_path5_h_scale = 0, ime_output_path5_v_scale = 0;


//IME_SIZE_INFO ime_input_size = {0x0}, ime_output_size = {0x0};
IME_SIZE_INFO ime_output_size_path1 = {0x0}, ime_output_size_path2 = {0x0}, ime_output_size_path3 = {0x0}, ime_output_size_path4 = {0x0}, ime_output_size_path5 = {0x0};
UINT32 ime_tmp_input_size = 0, ime_tmp_output_size_path1 = 0, ime_tmp_output_size_path2 = 0, ime_tmp_output_size_path3 = 0, ime_tmp_output_size_path4 = 0, ime_tmp_output_size_path5 = 0, ime_tmp_output_size_max = 0;


IME_SCALE_METHOD_SEL ime_output_scale_method = 0;

static UINT32 ime_scale_rate = 0, ime_scale_factor = 0, ime_scale_nor_k = 0, ime_scale_isd_k = 0;
//static UINT32 ime_scale_factorHist[4097] = {0x0};
//static UINT32 g_uiMaxFactor = 0, g_uiMinFactor = 4096;
//static UINT32 g_uiBinVal = 0, g_uiMaxValBin = 0x0;
IME_ISD_FACTOR ime_scale_factor_base[6] = {0x0};
//IME_STITCH_INFO ime_stitching_image_params[3] = {0x0};
IME_STITCH_INFO ime_stitching_image_params = {0x0};

IME_OUTPUT_BUF_INFO ime_out_buf_flush[IME_OUT_BUF_MAX] = {0};


// cal. APIs

static UINT32 ime_cal_isd_base_factor(UINT32 in_size, UINT32 out_size);
static ER ime_cal_image_scale_params(IME_SIZE_INFO *p_in_img_size, IME_SIZE_INFO *p_out_img_size, IME_SCALE_METHOD_SEL scale_method, IME_SCALE_FACTOR_INFO *p_scl_factor_info);
static UINT32 ime_cal_scale_filter_coefs(UINT32 in_img_size, UINT32 out_img_size);


static IME_POINT ime_pm_cvx_point[4];
static IME_POINT ime_pm_center_point;
static IME_POINT ime_pm_point_a, ime_pm_point_b, ime_pm_point_c, ime_pm_point_d;


#if defined (__UITRON) || defined (__ECOS)
static IME_POINT ime_pm_cvx_tmp[4];
static IME_POINT ime_pm_center_tmp = {0.0};
static FLOAT ime_get_degree_between_points(IME_POINT cur_pnt, IME_POINT cent_pnt);
static VOID ime_point_compare_sort(IME_POINT *p_pnt, INT32 pnt_num);
#endif

//static FLOAT ime_absf(FLOAT fval);

static VOID ime_cal_linear_coef(IME_POINT pnt_a, IME_POINT pnt_b, IME_POINT pnt_cent, INT32 *p_coefs);

//static ER ime_check_motion_detection(IME_MD_PARAM_T param_id, UINT32 *value, IME_MD_INFO *p_set_info);

//---------------------------------------------------------------------------------------------------
#if 0
static FLOAT ime_absf(FLOAT fval)
{
	FLOAT tmp = 0.0;

	tmp = (fval < 0.0) ? (-1.0 * fval) : fval;

	return tmp;
}
#endif
//---------------------------------------------------------------------------------------------------

#if defined (__UITRON) || defined (__ECOS)

static FLOAT ime_get_degree_between_points(IME_POINT cur_pnt, IME_POINT cent_pnt)
{
	FLOAT dx = 0.0, dy = 0.0, r = 0.0;
	FLOAT degree = 0.0, rads = 0.0;

	dx = (cur_pnt.pnt_x - cent_pnt.pnt_x);
	dy = (cur_pnt.pnt_y - cent_pnt.pnt_y);
	r = sqrt((dx * dx) + (dy * dy));

	rads = acos(dx / r);
	degree = (rads * 57.29578);//(rads / 3.1415926 * 180.0);
	if (cur_pnt.pnt_y < cent_pnt.pnt_y) {
		degree = (180.0 + degree);
	} else {
		degree = (180.0 - degree);
	}

	degree = 360.0 - degree;

	return degree;
}
//---------------------------------------------------------------------------------------------------

static VOID ime_point_compare_sort(IME_POINT *p_pnt, INT32 pnt_num)
{
	INT32 i = 0, j = 0;
	IME_POINT tmp = {0.0};

	for (j = (pnt_num - 1); j >= 0; j--) {
		for (i = 0; i < j; i++) {
			if (pnt_num[i].pnt_dg > pnt_num[i + 1].pnt_dg) {
				tmp = pnt_num[i];
				pnt_num[i] = pnt_num[i + 1];
				pnt_num[i + 1] = tmp;
			}
		}
	}
}
#endif
//---------------------------------------------------------------------------------------------------

static VOID ime_cal_linear_coef(IME_POINT pnt_a, IME_POINT pnt_b, IME_POINT pnt_cent, INT32 *p_coefs)
{
	// reserved
	INT32 x1 = 0, y1 = 0;
	INT32 x2 = 0, y2 = 0;
	INT32 check = 0;

	x1 = (INT32)pnt_a.pnt_x;
	y1 = (INT32)pnt_a.pnt_y;
	x2 = (INT32)pnt_b.pnt_x;
	y2 = (INT32)pnt_b.pnt_y;

	// 0: >=,  1: <=,  2: >,  3: <

	p_coefs[0] = (y2 - y1);
	p_coefs[1] = (x1 - x2);
	p_coefs[2] = (x1 * y2 - x2 * y1);

	check = (p_coefs[0] * pnt_cent.pnt_x) + (p_coefs[1] * pnt_cent.pnt_y);
	if (check >= p_coefs[2]) {
		p_coefs[3] = 0;
	} else if (check <= p_coefs[2]) {
		p_coefs[3] = 1;
	}
}
//---------------------------------------------------------------------------------------------------


VOID ime_cal_convex_hull_coefs(IME_PM_POINT *p_pm_cvx_point, INT32 pnt_num, INT32 *p_coefs_line1, INT32 *p_coefs_line2, INT32 *p_coefs_line3, INT32 *p_coefs_line4)
{
	// reserved
	INT32 i = 0;
	//FLOAT area = 0.0;
	//FLOAT get_val_a = 0.0, get_val_b = 0.0, get_val_c = 0.0;
	//FLOAT comp_a = 0.0, comp_b = 0.0, comp_c = 0.0;

#if defined (__KERNEL__) || defined (__FREERTOS)

	for (i = 0; i < 4; i++) {
		ime_pm_cvx_point[i].pnt_x = p_pm_cvx_point[i].coord_x;
		ime_pm_cvx_point[i].pnt_y = p_pm_cvx_point[i].coord_y;
	}

	ime_pm_center_point.pnt_x = (ime_pm_cvx_point[0].pnt_x + ime_pm_cvx_point[1].pnt_x + ime_pm_cvx_point[2].pnt_x + ime_pm_cvx_point[3].pnt_x) >> 2;
	ime_pm_center_point.pnt_y = (ime_pm_cvx_point[0].pnt_y + ime_pm_cvx_point[1].pnt_y + ime_pm_cvx_point[2].pnt_y + ime_pm_cvx_point[3].pnt_y) >> 2;

	ime_pm_point_a = ime_pm_cvx_point[3];
	ime_pm_point_b = ime_pm_cvx_point[0];
	ime_pm_point_c = ime_pm_cvx_point[1];
	ime_pm_point_d = ime_pm_cvx_point[2];

	ime_cal_linear_coef(ime_pm_point_b, ime_pm_point_a, ime_pm_center_point, p_coefs_line1);
	ime_cal_linear_coef(ime_pm_point_c, ime_pm_point_b, ime_pm_center_point, p_coefs_line2);
	ime_cal_linear_coef(ime_pm_point_d, ime_pm_point_a, ime_pm_center_point, p_coefs_line3);
	ime_cal_linear_coef(ime_pm_point_c, ime_pm_point_d, ime_pm_center_point, p_coefs_line4);


#else

	for (i = 0; i < 4; i++) {
		ime_pm_cvx_point[i].pnt_x = (FLOAT)p_pm_cvx_point[i].coord_x;
		ime_pm_cvx_point[i].pnt_y = (FLOAT)p_pm_cvx_point[i].coord_y;
	}

	ime_pm_center_point.pnt_x = (ime_pm_cvx_point[0].pnt_x + ime_pm_cvx_point[1].pnt_x + ime_pm_cvx_point[2].pnt_x + ime_pm_cvx_point[3].pnt_x) / 4.0;
	ime_pm_center_point.pnt_y = (ime_pm_cvx_point[0].pnt_y + ime_pm_cvx_point[1].pnt_y + ime_pm_cvx_point[2].pnt_y + ime_pm_cvx_point[3].pnt_y) / 4.0;

	for (i = 0; i < 4; i++) {
		ime_pm_cvx_tmp[i].pnt_x = ime_pm_cvx_point[i].pnt_x - ime_pm_center_point.pnt_x;
		ime_pm_cvx_tmp[i].pnt_y = ime_pm_cvx_point[i].pnt_y - ime_pm_center_point.pnt_y;
	}
	ime_pm_center_tmp.pnt_x = 0.0;
	ime_pm_center_tmp.pnt_y = 0.0;

	for (i = 0; i < 4; i++) {
		ime_pm_cvx_point[i].fDG = ime_get_degree_between_points(ime_pm_cvx_tmp[i], ime_pm_center_tmp);
	}
	//cout << endl;

	ime_point_compare_sort(ime_pm_cvx_point, 4);

	ime_pm_point_a = ime_pm_cvx_point[3];
	ime_pm_point_b = ime_pm_cvx_point[0];
	ime_pm_point_c = ime_pm_cvx_point[1];
	ime_pm_point_d = ime_pm_cvx_point[2];

	ime_cal_linear_coef(ime_pm_point_b, ime_pm_point_a, ime_pm_center_point, p_coefs_line1);
	ime_cal_linear_coef(ime_pm_point_c, ime_pm_point_b, ime_pm_center_point, p_coefs_line2);
	ime_cal_linear_coef(ime_pm_point_d, ime_pm_point_a, ime_pm_center_point, p_coefs_line3);
	ime_cal_linear_coef(ime_pm_point_c, ime_pm_point_d, ime_pm_center_point, p_coefs_line4);
#endif

}
//---------------------------------------------------------------------------------------------------


static UINT32 ime_cal_isd_base_factor(UINT32 in_size, UINT32 out_size)
{
	UINT32 i = 0, j = 0;
	UINT32 s = (in_size - 1) * 65536 / (out_size - 1);
	UINT32 init_w = (s + 65536) >> 1, w = 0;
	UINT32 factor_cnt = 0;
	UINT32 rem = 0;
	UINT32 same_factor = 0, same_factor_idx_cnt = 0;
	UINT32 sum = 0, cnt = 0, base_factor = 0;

	//ime_scale_rate = 0, ime_scale_factor = 0, ime_scale_nor_k = 0, ime_scale_isd_k = 0;
	//g_uiMaxFactor = 0, g_uiMinFactor = 4096;
	//g_uiBinVal = 0, g_uiMaxValBin = 0x0;

	//memset(ime_scale_factorHist, 0, sizeof(UINT32)*4097);

	for (j = 0; j < 6; j++) {
		ime_scale_factor_base[j].uiFactor = 0;
		ime_scale_factor_base[j].uiCnt = 0;
	}

	for (i = 0; i < 160; i++) {
		factor_cnt = 0;

		rem = s;
		factor_cnt += ((init_w + 128) >> 8);
		rem -= init_w;

#if 0
		while (rem >= 65536) {
			W = 65536;
			factor_cnt += ((W + 128) >> 8);
			rem -= W;
		}
#else
		factor_cnt += ((rem >> 16) << 8);
		rem = (rem & 0xffff);
#endif

		init_w = 65536 - rem;
		w = rem;
		factor_cnt += ((w + 128) >> 8);
		// coverity[assigned_value]
		rem -= w;

		same_factor = 0;
		for (j = 0; j < 6; j++) {
			if (ime_scale_factor_base[j].uiFactor == factor_cnt) {
				same_factor = 1;
				ime_scale_factor_base[j].uiCnt += 1;
				break;
			}
		}

		if (same_factor == 0) {
			ime_scale_factor_base[same_factor_idx_cnt].uiFactor = factor_cnt;
			ime_scale_factor_base[same_factor_idx_cnt].uiCnt += 1;

			same_factor_idx_cnt += 1;
		}

		//ime_scale_factorHist[factor_cnt] += 1;
		//if(factor_cnt >= g_uiMaxFactor)
		//{
		//    g_uiMaxFactor = factor_cnt;
		//}

		//if(factor_cnt <= g_uiMinFactor)
		//{
		//    g_uiMinFactor = factor_cnt;
		//}
	}


	//for(i = 0; i < 4097; i++)
	//{
	//    if(ime_scale_factorHist[i] >= g_uiBinVal)
	//    {
	//        g_uiBinVal = ime_scale_factorHist[i];
	//        g_uiMaxValBin = i;
	//    }
	//}

	sum = 0, cnt = 0;
	for (j = 0; j < 6; j++) {
		if (ime_scale_factor_base[j].uiCnt != 0) {
			sum += ime_scale_factor_base[j].uiFactor;
			cnt += 1;
		}
	}

	base_factor = s;
	if (cnt != 0) {
		base_factor = ((sum + (cnt >> 1)) / cnt);
	}

	return base_factor;
}
//-------------------------------------------------------------------------------


static ER ime_cal_image_scale_params(IME_SIZE_INFO *p_in_img_size, IME_SIZE_INFO *p_out_img_size, IME_SCALE_METHOD_SEL scale_method, IME_SCALE_FACTOR_INFO *p_scl_factor_info)
{
	UINT32 InWidth = 0, InHeight = 0;
	UINT32 OutWidth = 0, OutHeight = 0;
	UINT32 s = 0;
	UINT32 CHKF = 0;

	if (p_in_img_size == NULL) {
		DBG_ERR("IME: input image size parameter NULL...\r\n");

		return E_PAR;
	}

	if (p_out_img_size == NULL) {
		DBG_ERR("IME: output image size parameter NULL...\r\n");

		return E_PAR;
	}

	if (p_scl_factor_info == NULL) {
		DBG_ERR("IME: scaling factor parameter NULL...\r\n");

		return E_PAR;
	}

	InWidth = p_in_img_size->size_h;
	InHeight = p_in_img_size->size_v;

	OutWidth = p_out_img_size->size_h;
	OutHeight = p_out_img_size->size_v;

	if (OutWidth == 0) {
		DBG_ERR("IME: divided by zero...\r\n");

		return E_PAR;
	}

	p_scl_factor_info->isd_coef_ctrl = IME_ISD_WITHOUT_UCOEF;


	// cal. horizontal
	p_scl_factor_info->isd_scale_h_base_ftr = 256;
	p_scl_factor_info->isd_scale_h_ftr[0] = 4096;
	p_scl_factor_info->isd_scale_h_ftr[1] = 4096;
	p_scl_factor_info->isd_scale_h_ftr[2] = 4096;
	if (InWidth == OutWidth) {
		p_scl_factor_info->scale_h_ud = IME_SCALE_DOWN;

		p_scl_factor_info->scale_h_dr = 0;
		p_scl_factor_info->scale_h_ftr = 0;
	} else {
		s = (InWidth - 1) * 65536 / (OutWidth - 1);
		ime_scale_nor_k = s;

		CHKF = ((InWidth - 1) * 512) / (OutWidth - 1);

		ime_scale_rate = (ime_scale_nor_k >> 16);
		ime_scale_factor = ime_scale_nor_k - (ime_scale_rate << 16);

		p_scl_factor_info->scale_h_dr = ime_scale_rate;
		if (ime_scale_rate >= 1) {
			p_scl_factor_info->scale_h_dr = ime_scale_rate - 1;
		}

		p_scl_factor_info->scale_h_ftr = ime_scale_factor;
		if (OutWidth > InWidth) {
			p_scl_factor_info->scale_h_ud = IME_SCALE_UP;
		} else {
			p_scl_factor_info->scale_h_ud = IME_SCALE_DOWN;

			if ((CHKF < 8192) && (scale_method == IMEALG_INTEGRATION)) { // (CHKF >= 1.98) &&
				ime_scale_isd_k = ime_cal_isd_base_factor(InWidth, OutWidth);  //(IsdK >> 8);
				p_scl_factor_info->isd_scale_h_base_ftr = ime_scale_isd_k;

				//p_scl_factor_info->isd_scale_h_ftr[0] = (UINT32)((1048576.0 / (FLOAT)(ime_scale_isd_k - 1)) + 0.5);  // 1048576 = 256 * 4096
				//p_scl_factor_info->isd_scale_h_ftr[1] = (UINT32)((1048576.0 / (FLOAT)(ime_scale_isd_k - 0)) + 0.5);
				//p_scl_factor_info->isd_scale_h_ftr[2] = (UINT32)((1048576.0 / (FLOAT)(ime_scale_isd_k + 1)) + 0.5);
				p_scl_factor_info->isd_scale_h_ftr[0] = ((1048576 + ((ime_scale_isd_k - 1) >> 1)) / (ime_scale_isd_k - 1)); // 1048576 = 256 * 4096
				p_scl_factor_info->isd_scale_h_ftr[1] = ((1048576 + ((ime_scale_isd_k - 0) >> 1)) / (ime_scale_isd_k - 0));
				p_scl_factor_info->isd_scale_h_ftr[2] = ((1048576 + ((ime_scale_isd_k + 1) >> 1)) / (ime_scale_isd_k + 1));

				p_scl_factor_info->isd_scale_h_coef_nums = ime_scaling_isd_init_kernel_number(InWidth, OutWidth);
			}
		}
	}

	// cal. vertical
	p_scl_factor_info->isd_scale_v_base_ftr = 256;
	p_scl_factor_info->isd_scale_v_ftr[0] = 4096;
	p_scl_factor_info->isd_scale_v_ftr[1] = 4096;
	p_scl_factor_info->isd_scale_v_ftr[2] = 4096;
	if (InHeight == OutHeight) {
		p_scl_factor_info->scale_v_ud = IME_SCALE_DOWN;

		p_scl_factor_info->scale_v_dr = 0;
		p_scl_factor_info->scale_v_ftr = 0;
	} else {
		s = (InHeight - 1) * 65536 / (OutHeight - 1);
		ime_scale_nor_k = s;

		CHKF = ((InHeight - 1) * 512) / (OutHeight - 1);

		ime_scale_rate = (ime_scale_nor_k >> 16);
		ime_scale_factor = ime_scale_nor_k - (ime_scale_rate << 16);

		p_scl_factor_info->scale_v_dr = ime_scale_rate;
		if (ime_scale_rate >= 1) {
			p_scl_factor_info->scale_v_dr = ime_scale_rate - 1;
		}
		p_scl_factor_info->scale_v_ftr = ime_scale_factor;

		if (OutHeight > InHeight) {
			p_scl_factor_info->scale_v_ud = IME_SCALE_UP;
		} else {
			p_scl_factor_info->scale_v_ud = IME_SCALE_DOWN;

			if ((CHKF < 8192) && (scale_method == IMEALG_INTEGRATION)) { //(ime_scale_rate >= 2)
				ime_scale_isd_k = ime_cal_isd_base_factor(InHeight, OutHeight);//(IsdK >> 8);
				p_scl_factor_info->isd_scale_v_base_ftr = ime_scale_isd_k;

				//p_scl_factor_info->isd_scale_v_ftr[0] = (UINT32)((1048576.0 / (FLOAT)(ime_scale_isd_k - 1)) + 0.5);
				//p_scl_factor_info->isd_scale_v_ftr[1] = (UINT32)((1048576.0 / (FLOAT)(ime_scale_isd_k - 0)) + 0.5);
				//p_scl_factor_info->isd_scale_v_ftr[2] = (UINT32)((1048576.0 / (FLOAT)(ime_scale_isd_k + 1)) + 0.5);
				p_scl_factor_info->isd_scale_v_ftr[0] = ((1048576 + ((ime_scale_isd_k - 1) >> 1)) / (ime_scale_isd_k - 1));
				p_scl_factor_info->isd_scale_v_ftr[1] = ((1048576 + ((ime_scale_isd_k - 0) >> 1)) / (ime_scale_isd_k - 0));
				p_scl_factor_info->isd_scale_v_ftr[2] = ((1048576 + ((ime_scale_isd_k + 1) >> 1)) / (ime_scale_isd_k + 1));

				p_scl_factor_info->isd_scale_v_coef_nums = ime_scaling_isd_init_kernel_number(InHeight, OutHeight);
			}
		}
	}

	return E_OK;
}
//-------------------------------------------------------------------------------

static UINT32 ime_cal_scale_filter_coefs(UINT32 in_img_size, UINT32 out_img_size)
{
	UINT32 coef = 0x0;

	int in_size, out_size, wet, scale_down_rate;

	if (out_img_size < 2) {
		return 0;
	}

	if (in_img_size > out_img_size) {
#if 0

		coef = ((inImgSize * 20) / outImgSize) - 16;
		if (coef > 64) {
			coef = 64;
		}
		coef = 64 - coef;

#else
		in_size = (int)in_img_size;
		out_size = (int)out_img_size;

		scale_down_rate = ((in_size - 1) * 32768 / (out_size - 1));

		wet = 0;
		if (scale_down_rate <= 294912) {
			//
			wet = ((-8 * scale_down_rate) + (2359296)) >> 15;  // 2359296 = 72 x 32768
		}

		coef = (UINT32)wet;

		if (coef > 63) {
			coef = 63;
		}
		//coef = 63 - coef;
#endif
	}

	return coef;
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
// extern APIs

IME_H_STRIPE_OVLP_SEL ime_cal_overlap_size(IME_SIZE_INFO *p_in_size, IME_SIZE_INFO *p_out_size)
{
	// reserved
	UINT32 scale_rate_h = 0;//, ScaleRateV = 0.0;
	IME_H_STRIPE_OVLP_SEL overlap = IME_H_ST_OVLP_32P;

	if ((p_out_size->size_h > 2) && (p_out_size->size_v > 2)) {
		scale_rate_h = (p_in_size->size_h - 1) * 512 / (p_out_size->size_h - 1);
		//ScaleRateV = (FLOAT)(pin_size->size_v - 1) / (FLOAT)(pOutSize->size_v - 1);

		if ((scale_rate_h < 3584)) {
			overlap = IME_H_ST_OVLP_16P;
		}
		//else if((ScaleRateH <= 10.0))
		//{
		//    Overlap = IME_H_ST_OVLP_24P;
		//}
		else {
			overlap = IME_H_ST_OVLP_32P;
		}
	}

	return overlap;
}
//---------------------------------------------------------------------------------------------------


VOID ime_cal_hv_strip_param(UINT32 in_width, UINT32 in_height, UINT32 stripe_size, IME_HV_STRIPE_INFO *p_strip_param)
{
	// reserved
	UINT32 s_ime_st_size;
	UINT32 s_ime_hn = 0, s_ime_hm = 0, s_ime_hl = 0, s_ime_h_overlap = 0;
	UINT32 check_size = 0;
	//IME_FUNC_EN GetSSREn;

	//p_strip_param->overlap_h_sel = IME_H_ST_OVLP_32P;
	//p_strip_param->overlap_h_size = IME_H_OVERLAP;

	//GetSSREn = ime_get_ssr_enable();
	//s_ime_st_size = ((GetSSREn == IME_FUNC_DISABLE) ? 2592 : 1312);
	s_ime_st_size = stripe_size;

	if (in_width <= s_ime_st_size) {
		ime_stripe_operation_mode = IME_SINGLE_STRIPE;

		p_strip_param->stp_h.stp_n = 0x0;
		p_strip_param->stp_h.stp_l = in_width / 4;
		p_strip_param->stp_h.stp_m = 0x0;

		p_strip_param->stp_v.stp_n = 0x0;
		p_strip_param->stp_v.stp_l = in_height;
		p_strip_param->stp_v.stp_m = 0x0;
	} else {
		ime_stripe_operation_mode = IME_MULTI_STRIPE;

		s_ime_h_overlap = p_strip_param->overlap_h_size;

		//==========================================================================
		// horizontal stripe computation
		s_ime_hn = s_ime_st_size;
		s_ime_hm = in_width / (s_ime_st_size - s_ime_h_overlap);
		s_ime_hl = in_width - ((s_ime_st_size - s_ime_h_overlap) * s_ime_hm);
		while ((s_ime_hl < 64)) {
			s_ime_st_size -= 8;

			s_ime_hn = s_ime_st_size;
			s_ime_hm = in_width / (s_ime_st_size - s_ime_h_overlap);
			s_ime_hl = in_width - ((s_ime_st_size - s_ime_h_overlap) * s_ime_hm);
		}

		s_ime_hm += 1;

		check_size = ((s_ime_hn - s_ime_h_overlap) * (s_ime_hm - 1)) + s_ime_hl;
		if (check_size != in_width) {
			DBG_ERR("IME: horizontal stripe size error...\r\n");
		}

		p_strip_param->stp_h.stp_n = s_ime_hn >> 2;
		p_strip_param->stp_h.stp_l = s_ime_hl >> 2;
		p_strip_param->stp_h.stp_m = s_ime_hm - 1;

		//==========================================================================
		// vertical stripe computation
		p_strip_param->stp_v.stp_n = 0;
		p_strip_param->stp_v.stp_l = in_height;
		p_strip_param->stp_v.stp_m = 0;
	}
}
//---------------------------------------------------------------------------------------------------

IME_OPMODE ime_get_operate_mode(VOID)
{
	// reserved
	return ime_engine_operation_mode;
}

//---------------------------------------------------------------------------------------------------

ER ime_check_state_machine(IME_STATE_MACHINE current_status, IME_ACTION_OP action_op)
{
	// reserved
	ER er_return = E_OK;

	switch (current_status) {
	case IME_ENGINE_IDLE:
		switch (action_op) {
		case IME_ACTOP_OPEN:
			//case IME_ACTOP_SWRESET:
			er_return = E_OK;
			break;

		default:
			er_return = E_OACV;
			break;
		}
		break;

	case IME_ENGINE_READY:
		switch (action_op) {
		case IME_ACTOP_CLOSE:
		case IME_ACTOP_PARAM:
		//case IME_ACTOP_SWRESET:
		case IME_ACTOP_DYNAMICPARAM:
			er_return = E_OK;
			break;

		default:
			er_return = E_OACV;
			break;
		}
		break;

	case IME_ENGINE_PAUSE:
		switch (action_op) {
		case IME_ACTOP_CLOSE:
		case IME_ACTOP_START:
		//case IME_ACTOP_SWRESET:
		case IME_ACTOP_PARAM:
		case IME_ACTOP_DYNAMICPARAM:
		case IME_ACTOP_CHGCLOCK:
		case IME_ACTOP_PAUSE:
			//case IME_ACTOP_TRIGGERSLICE:
			er_return = E_OK;
			break;

		default:
			er_return = E_OACV;
			break;
		}
		break;

	case IME_ENGINE_RUN:
		switch (action_op) {
		case IME_ACTOP_PAUSE:
		case IME_ACTOP_DYNAMICPARAM:
			// for VD_latch test
			//case IME_ACTOP_PARAM:
			//case IME_ACTOP_START:
			// --------

			er_return = E_OK;
			break;

		default:
			er_return = E_OACV;
			break;
		}
		break;

	default:
		er_return = E_OACV;
		break;
	}

#if (defined(_NVT_EMULATION_) == ON)
	er_return = E_OK;
#endif

	return er_return;
}
//-----------------------------------------------------------------------------

#if 0
VOID ime_set_engine_control(IME_ENGEIN_SET_CTRL set_sel, UINT32 set_val)
{
	switch (set_sel) {
	case IME_ENGINE_SET_RESET:
		ime_reset_reg(set_val);
		break;

	case IME_ENGINE_SET_START:
		ime_start_reg(set_val);
		break;

	case IME_ENGINE_SET_LOAD:
		ime_load_reg(set_val, ENABLE);
		break;

	case IME_ENGINE_SET_INSRC:
		//ime_set_in_path_source(set_val);
		ime_set_input_source_reg(set_val);
		break;

	case IME_ENGINE_SET_DRCL:
		ime_set_direct_path_control_reg(set_val);
		break;

	case IME_ENGINE_SET_INTPE:
		ime_set_interrupt_enable_reg(set_val);
		break;

	case IME_ENGINE_SET_INTPS:
		ime_set_interrupt_status_reg(set_val);
		break;

	default:
		break;
	}
}
#endif
//-----------------------------------------------------------------------------

#if 0
UINT32 ime_get_engine_control(IME_ENGEIN_GET_CTRL get_sel)
{
	UINT32 get_val = 0;

	switch (get_sel) {
	case IME_ENGINE_GET_INTP:
		get_val = ime_get_interrupt_status_reg();
		break;

	case IME_ENGINE_GET_INTPE:
		get_val = ime_get_interrupt_enable_reg();
		break;

	//case IME_ENGINE_GET_LOCKSTATUS:
	//  GetVal = ime_eng_lock_status;
	//  break;

	default:
		break;
	}

	return get_val;
}
#endif
//-----------------------------------------------------------------------------

#if 0
VOID ime_set_init(VOID)
{
	//ime_set_engine_control(IME_ENGINE_SET_INTPS, IME_INTS_ALL);
	//ime_set_engine_control(IME_ENGINE_SET_INTPE, 0x00000000);

	ime_set_interrupt_enable_reg(0x00000000);
	ime_set_interrupt_status_reg(IME_INTS_ALL);

	ime_set_init_reg();
}
#endif
//-----------------------------------------------------------------------------

UINT32 ime_input_dma_buf_flush(UINT32 in_addr, UINT32 in_size, IME_OPMODE op_mode)
{
	// reserved
	UINT32 tmp_addr = in_addr;
	UINT32 tmp_size = in_size;

	if (in_size == 0) {
		DBG_WRN("IME: input buffer size zero...\r\n");
		tmp_size = 64;
	}

#if 1
	if (ime_platform_dma_is_cacheable(tmp_addr)) {
		if (op_mode == IME_OPMODE_D2D) {
			ime_platform_dma_flush_mem2dev(tmp_addr, tmp_size);
		} else {
			ime_platform_dma_flush_mem2dev_for_video_mode(tmp_addr, tmp_size);
		}
	}
#else
#ifdef __KERNEL__

	if (uioperation_mode == IME_OPMODE_D2D) {
		fmem_dcache_sync((void *)uiInAddr, TmpSize, DMA_BIDIRECTIONAL);
	} else {
		fmem_dcache_sync_vb((void *)uiInAddr, TmpSize, DMA_BIDIRECTIONAL);
	}
#else

#if (IME_DMA_CACHE_HANDLE == 1)
	if (uioperation_mode == IME_OPMODE_D2D) {
		dma_flushWriteCache(tmp_addr, in_size);
	}
	//tmp_addr = dma_getPhyAddr(tmp_addr);
#else
	tmp_addr = uiInAddr;
#endif
#endif
#endif

	return tmp_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_output_dma_buf_flush(UINT32 out_addr, UINT32 out_size, BOOL flash_type, IME_BUF_FLUSH_SEL is_dma_flush, IME_OPMODE op_mode)
{
	// reserved
	UINT32 tmp_addr = out_addr;
	UINT32 tmp_size = out_size;

	if (out_size == 0) {
		DBG_WRN("IME: output buffer size zero...\r\n");
		tmp_size = 64;
	}

#if 1
	if (ime_platform_dma_is_cacheable(tmp_addr)) {
		if ((op_mode == IME_OPMODE_D2D)) {
			if (is_dma_flush == IME_DO_BUF_FLUSH) {
				if (flash_type == TRUE) {
					ime_platform_dma_flush_dev2mem(tmp_addr, tmp_size);
				} else {
					ime_platform_dma_flush_dev2mem_width_neq_loff(tmp_addr, tmp_size);
				}
			}
		} else {
			ime_platform_dma_flush_dev2mem_for_video_mode(tmp_addr, tmp_size);
		}
	}
#else
#ifdef __KERNEL__
	if ((uioperation_mode == IME_OPMODE_D2D)) {
		fmem_dcache_sync((void *)uiOutAddr, TmpSize, DMA_BIDIRECTIONAL);
	} else {
		fmem_dcache_sync_vb((void *)uiOutAddr, TmpSize, DMA_BIDIRECTIONAL);
	}
#else

#if (IME_DMA_CACHE_HANDLE == 1)
	if ((uioperation_mode == IME_OPMODE_D2D)) {
		if (bFlashType == TRUE) {
			dma_flushReadCache(tmp_addr, uiOutSize);
		} else {
			dma_flushReadCacheWidthNEQLineOffset(tmp_addr, uiOutSize);
		}
	}
#else
	tmp_addr = uiOutAddr;
#endif

#endif
#endif

	return tmp_addr;
}
//-------------------------------------------------------------------------------

void ime_output_buffer_flush_dma_end(void)
{
	UINT32 i = 0;

	for (i = 0; i < IME_OUT_BUF_MAX; i++) {

		if (ime_out_buf_flush[i].flush_en == TRUE) {
			ime_platform_dma_post_flush_dev2mem(ime_out_buf_flush[i].buf_addr, ime_out_buf_flush[i].buf_size);
		}
	}
}
//-------------------------------------------------------------------------------


#if 0
ER ime_check_dma_addr(UINT32 addr_y, UINT32 addr_u, UINT32 addr_v, IME_DMA_DIRECTIOIN in_out_sel)
{
	/*
	#ifdef __KERNEL__

	#else
	    UINT32 AlignedY = 0, AlignedC = 0;
	    UINT32 AddrMax = 0x3FFFFFFF;
	    UINT32 RemY = 0, RemU = 0, RemV = 0;

	    if ((addrY == 0x0) || (addrU == 0x0) || (addrV == 0x0)) {
	        DBG_ERR("IME: address zero...\r\n");
	        return E_PAR;
	    }

	    if ((addrY > AddrMax) || (addrU > AddrMax) || (addrV > AddrMax)) {
	        DBG_ERR("IME: address overflow...\r\n");
	        return E_PAR;
	    }

	    if (InOutSel == IME_DMA_INPUT) {
	        AlignedY = 4;
	        AlignedC = 4;

	        RemY = addrY & (AlignedY - 1);
	        RemU = addrU & (AlignedC - 1);
	        RemV = addrV & (AlignedC - 1);
	        if ((RemY != 0) || (RemU != 0) || (RemV != 0)) {
	            DBG_ERR("IME: input path address is not 4x...\r\n");

	            return E_PAR;
	        }
	    } else {
	        //AlignedY = 1;
	        AlignedC = 2;

	        //RemY = (addrY % AlignedY);
	        RemU = addrU & (AlignedC - 1);
	        RemV = addrV & (AlignedC - 1);
	        if ((RemU != 0) || (RemV != 0)) {
	            DBG_ERR("IME: output path address is not 2x...\r\n");

	            return E_PAR;
	        }
	    }
	#endif
	*/

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

ER ime_check_lineoffset(UINT32 lofs_y, UINT32 lofs_c, BOOL zero_check_en)
{
	// reserved
	UINT32 aligned = 4;
	UINT32 lineoffset_max = (1 << 20) - 1;
	UINT32 remy = 0, remc = 0;

    if (zero_check_en == TRUE) {
    	if ((lofs_y == 0) || (lofs_c == 0)) {
    		DBG_ERR("IME: lineoffset is zero...\r\n");

    		return E_PAR;
    	}
	}

	if ((lofs_y > lineoffset_max) || (lofs_c > lineoffset_max)) {
		DBG_ERR("IME: lineoffset is overflow...\r\n");

		return E_PAR;
	}

	remy = lofs_y & (aligned - 1);
	remc = lofs_y & (aligned - 1);

	if ((remy != 0) || (remc != 0)) {
		DBG_ERR("IME: lineoffset is not 4x...\r\n");

		return E_PAR;
	}

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_check_input_path_stripe(VOID)
{
	// reserved
	IME_SIZE_INFO in_size = {0x0}, out_size = {0x0};

	IME_SCALE_METHOD_SEL scl_method1 = IMEALG_BICUBIC, scl_method2 = IMEALG_BICUBIC, scl_method3 = IMEALG_BICUBIC, scl_method_lca_subout = IMEALG_INTEGRATION; //scl_method4 = IMEALG_BICUBIC, scl_method5 = IMEALG_BICUBIC,

	UINT32 factor = 0, stp_size_h = 0, Strout_size = 0;
	IME_FUNC_EN path1_en = IME_FUNC_DISABLE, path2_en = IME_FUNC_DISABLE, path3_en = IME_FUNC_DISABLE;//, path4_en = IME_FUNC_DISABLE; //Path5En = IME_FUNC_DISABLE;
	//IME_FUNC_EN SSREn = IME_FUNC_DISABLE;
	IME_FUNC_EN lca_subout_en = IME_FUNC_DISABLE;
	UINT32 ttl_en = 0;
	IME_INSRC_SEL get_in_src;

	//float base_isd_factor_rate = 0.0;


	//ime_get_in_path_source(&get_in_src);
	get_in_src = ime_get_input_source_reg();
	if (get_in_src == IME_INSRC_IPE) {
		return E_OK;
	}

	//ime_get_out_path_enable_p1(&path1_en);
	//ime_get_out_path_enable_p2(&path2_en);
	//ime_get_out_path_enable_p3(&path3_en);
	//ime_get_out_path_enable_p4(&path4_en);
	//ime_get_out_path_enable_p5(&Path5En);
	//lca_subout_en = (IME_FUNC_EN)ime_get_lca_subout_enable();

	path1_en = ime_get_output_path_enable_status_p1_reg();
	path2_en = ime_get_output_path_enable_status_p2_reg();
	path3_en = ime_get_output_path_enable_status_p3_reg();
	//path4_en = ime_get_output_path_enable_status_p4_reg();
	lca_subout_en = (IME_FUNC_EN)ime_get_lca_subout_enable_reg();

	if ((path1_en == IME_FUNC_DISABLE) && (path2_en == IME_FUNC_DISABLE) && (path3_en == IME_FUNC_DISABLE) && (lca_subout_en == IME_FUNC_DISABLE)) {
		return E_OK;
	}

	//ime_get_out_path_scale_method_p1(&scl_method1);
	//ime_get_out_path_scale_method_p2(&scl_method2);
	//ime_get_out_path_scale_method_p3(&scl_method3);
	//ime_get_out_path_scale_method_p4(&scl_method4);
	//ime_get_out_path_scale_method_p5(&scl_method5);

	scl_method1 = (IME_SCALE_METHOD_SEL)ime_get_scaling_method_p1_reg();
	scl_method2 = (IME_SCALE_METHOD_SEL)ime_get_scaling_method_p2_reg();
	scl_method3 = (IME_SCALE_METHOD_SEL)ime_get_scaling_method_p3_reg();
	//scl_method4 = (IME_SCALE_METHOD_SEL)ime_get_scaling_method_p4_reg();

	if ((scl_method1 != IMEALG_INTEGRATION) && (scl_method2 != IMEALG_INTEGRATION) && (scl_method3 != IMEALG_INTEGRATION) && (lca_subout_en == IME_FUNC_DISABLE)) {
		return E_OK;
	}

	//SSREn = ime_get_ssr_enable();

	//base_isd_factor_rate = 1.98; //(2592 / 1312)
	//if(SSREn == ENABLE)
	//{
	//  base_isd_factor_rate = 4.13;  // ((2592 * 2) - 1) / 1312
	//}

	ttl_en = (UINT32)path1_en + (UINT32)path2_en + (UINT32)path3_en + (UINT32)lca_subout_en;
	if (ttl_en != 0) {
		ime_get_input_path_stripe_info(&ime_get_stripe_info);
		//ime_get_in_path_image_size(&in_size);
		ime_get_input_image_size_reg(&(in_size.size_h), &(in_size.size_v));

		if (ime_get_stripe_info.stp_size_mode == IME_STRIPE_SIZE_MODE_VARIED) {
			return E_OK;
		}

		if (ime_get_stripe_info.stp_h.stp_m > 1) { // MST
			stp_size_h = ime_get_stripe_info.stp_h.stp_n;//(ime_get_stripe_info.stp_h.stp_n - ime_get_stripe_info.overlap_h_size);
		} else if (ime_get_stripe_info.stp_h.stp_m == 1) { // SST
			stp_size_h = ime_get_stripe_info.stp_h.stp_l;
		}

#if 0
		if (SSREn == IME_FUNC_ENABLE) {
			stp_size_h = (stp_size_h * 2.0) - 1.0;
			in_size.size_h = (in_size.size_h * 2.0) - 1.0;
		}
#endif

#if 0
		if ((path1_en == IME_FUNC_ENABLE) && (scl_method1 == IMEALG_INTEGRATION) && (ime_get_stripe_info.stp_h.stp_m > 1)) {
			ime_get_out_path_scale_size_p1(&out_size);
			ime_get_scaling_size_p1_reg(&(out_size.size_h), &(out_size.size_v));

			if (in_size.size_h > out_size.size_h) {
				factor = (in_size.size_h - 1) * 512 / (out_size.size_h - 1);
				if (factor > 512) {
					DBG_ERR("IME: input stripe size is overflow for the scaling size of path1 (ISD)...\r\n");
					DBG_ERR("stp_size_h: %f, factor: %f, Strout_size: %f\n", stp_size_h, factor, Strout_size);

					return E_PAR;
				}

				if (factor < base_isd_factor_rate) {
					Strout_size = ((stp_size_h - 1.0) / factor) + 1.0;
					if (Strout_size > ime_max_stp_isd_out_buf_size) {
						DBG_ERR("IME: input stripe size is overflow for the scaling size of path1 (ISD)...\r\n");
						DBG_ERR("stp_size_h: %f, factor: %f, Strout_size: %f\n", stp_size_h, factor, Strout_size);

						return E_PAR;
					}
				}
			}
		}
#endif

		if ((path2_en == IME_FUNC_ENABLE) && (scl_method2 == IMEALG_INTEGRATION) && (ime_get_stripe_info.stp_h.stp_m > 1)) {
			//ime_get_out_path_scale_size_p2(&out_size);

			ime_get_scaling_size_p2_reg(&(out_size.size_h), &(out_size.size_v));

			factor = ((in_size.size_h - 1) * 512 / (out_size.size_h - 1));

			Strout_size = ((stp_size_h - 1) * 512 / factor) + 1;
			if ((Strout_size > ime_max_stp_isd_out_buf_size) && (factor > 512)) {
				//
				DBG_ERR("IME: input stripe size is not matching for the scaling size of path2 (ISD)...\r\n");
				return E_PAR;
			}
		}

		if ((path3_en == IME_FUNC_ENABLE) && (scl_method3 == IMEALG_INTEGRATION) && (ime_get_stripe_info.stp_h.stp_m > 1)) {
			//ime_get_out_path_scale_size_p3(&out_size);

			ime_get_scaling_size_p3_reg(&(out_size.size_h), &(out_size.size_v));

			factor = ((in_size.size_h - 1) * 512 / (out_size.size_h - 1));

			Strout_size = ((stp_size_h - 1) * 512 / factor) + 1;
			if ((Strout_size > ime_max_stp_isd_out_buf_size) && (factor > 512)) {
				//
				DBG_ERR("IME: input stripe size is not matching for the scaling size of path3 (ISD)...\r\n");
				return E_PAR;
			}
		}

#if 0
		if ((Path5En == IME_FUNC_ENABLE) && (scl_method5 == IMEALG_INTEGRATION) && (ime_get_stripe_info.stp_h.stp_m > 1)) {
			ime_get_out_path_scale_size_p5(&out_size);

			if (in_size.size_h > out_size.size_h) {
				factor = ((FLOAT)(in_size.size_h - 1) / (FLOAT)(out_size.size_h - 1));
				if (factor > 15.99) {
					DBG_ERR("IME: input stripe size is overflow for the scaling size of path5 (ISD)...\r\n");
					DBG_ERR("stp_size_h: %f, factor: %f, Strout_size: %f\n", stp_size_h, factor, Strout_size);

					return E_PAR;
				}

				if (factor < base_isd_factor_rate) {
					Strout_size = ((stp_size_h - 1.0) / factor) + 1.0;
					if (Strout_size > ime_max_stp_isd_out_buf_size) {
						DBG_ERR("IME: input stripe size is overflow for the scaling size of path5 (ISD)...\r\n");
						DBG_ERR("stp_size_h: %f, factor: %f, Strout_size: %f\n", stp_size_h, factor, Strout_size);

						return E_PAR;
					}
				}
			}
		}
#endif

		if ((lca_subout_en == IME_FUNC_ENABLE) && (scl_method_lca_subout == IMEALG_INTEGRATION) && (ime_get_stripe_info.stp_h.stp_m > 1)) {
			ime_get_input_path_stripe_info(&ime_get_stripe_info);
			//ime_get_lca_image_size_info(&out_size);
			ime_get_lca_image_size_reg(&(out_size.size_h), &(out_size.size_v));

			//ime_get_in_path_image_size(&in_size);
			ime_get_input_image_size_reg(&(in_size.size_h), &(in_size.size_v));

			factor = ((in_size.size_h - 1) * 512 / (out_size.size_h - 1));

			Strout_size = ((stp_size_h - 1) * 512 / factor) + 1;
			if ((Strout_size > ime_max_stp_isd_out_buf_size) && (factor > 512)) {
				//
				DBG_ERR("IME: input stripe size is not matching for the scaling size of LCA subout (ISD)...\r\n");
				return E_PAR;
			}
		}
	}

	return E_OK;
}
//-------------------------------------------------------------------------------
#if 0
ER ime_check_stripe_mode_data_stamp(IME_DS_SETNUM set_num)
{

	ER er_return = E_OK;

	switch (set_num) {
	case IME_DS_SET0:
		if ((ime_get_osd_set0_enable() == IME_FUNC_ENABLE) && (ime_getStripeMode_Reg() == IME_MULTI_STRIPE)) {
			er_return = E_PAR;
		}
		break;

	case IME_DS_SET1:
		if ((ime_getDS_P1Set1_Enable() == IME_FUNC_ENABLE) && (ime_getStripeMode_Reg() == IME_MULTI_STRIPE)) {
			er_return = E_PAR;
		}
		break;

	case IME_DS_SET2:
		if ((ime_getDS_P1Set2_Enable() == IME_FUNC_ENABLE) && (ime_getStripeMode_Reg() == IME_MULTI_STRIPE)) {
			er_return = E_PAR;
		}
		break;

	case IME_DS_SET3:
		if ((ime_getDS_P1Set3_Enable() == IME_FUNC_ENABLE) && (ime_getStripeMode_Reg() == IME_MULTI_STRIPE)) {
			er_return = E_PAR;
		}
		break;

	default:
		er_return = E_PAR;
		break;
	}



	return E_OK;;
}
#endif
//-------------------------------------------------------------------------------
#if 0
ER ime_check_stripe_mode_statistical(VOID)
{

	ER er_return = E_OK;

	if ((ime_get_adas_enable_reg() == IME_FUNC_DISABLE)) {
		er_return = E_PAR;
	}

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------
#if 0
ER ime_check_stripe_mode_stitching(IME_PATH_SEL set_path)
{


	ER er_return = E_OK;

	switch (set_path) {
	case IME_PATH1_SEL:
		if ((ime_get_stitching_enable_p1_reg() == IME_FUNC_ENABLE)) {
			er_return = E_PAR;
		}
		break;

	case IME_PATH2_SEL:
		if ((ime_get_stitching_enable_p2_reg() == IME_FUNC_ENABLE)) {
			er_return = E_PAR;
		}
		break;

	case IME_PATH3_SEL:
		if ((ime_get_stitching_enable_p3_reg() == IME_FUNC_ENABLE)) {
			er_return = E_PAR;
		}
		break;

	default:
		er_return = E_PAR;
		break;
	}


	return E_OK;
}
#endif
//-------------------------------------------------------------------------------


//VOID ime_set_in_path_source(IME_INSRC_SEL in_src)
//{
//	ime_set_input_source_reg((UINT32)in_src);
//}
//-------------------------------------------------------------------------------

//VOID ime_set_in_path_direct_ctrl(IME_SRC_CTRL_SEL drct_ctrl)
//{
//	ime_set_direct_path_control_reg((UINT32)drct_ctrl);
//}
//-------------------------------------------------------------------------------

//VOID ime_get_in_path_direct_ctrl(IME_SRC_CTRL_SEL *p_get_drct_ctrl)
//{
//	*p_get_drct_ctrl = (IME_SRC_CTRL_SEL)ime_get_direct_path_control_reg();
//}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_in_path_stripe_info(IME_HV_STRIPE_INFO *p_set_stripe)
{
	if (p_set_stripe == NULL) {
		DBG_ERR("IME: stripe parameter NULL...\r\n");

		return E_PAR;
	}

	if (p_set_stripe->stp_h.stp_n > 1023) {
		DBG_ERR("IME: stripe size HN overflow...\r\n");

		return E_PAR;
	}

	if (p_set_stripe->stp_h.stp_l > 1023) {
		DBG_ERR("IME: stripe size HL overflow...\r\n");

		return E_PAR;
	}

	if (p_set_stripe->stp_h.stp_m > 255) {
		DBG_ERR("IME: stripe number HM overflow...\r\n");

		return E_PAR;
	}

	ime_set_stripe_size_mode_reg((UINT32)p_set_stripe->stp_size_mode);

	ime_set_horizontal_stripe_number_reg(p_set_stripe->stp_h.stp_m);

	ime_set_horizontal_fixed_stripe_size_reg(p_set_stripe->stp_h.stp_n, p_set_stripe->stp_h.stp_l);

	if (p_set_stripe->stp_size_mode == IME_STRIPE_SIZE_MODE_VARIED) {
		ime_set_horizontal_varied_stripe_size_reg(p_set_stripe->stp_h.varied_size);
	} else {
		// do nothing...
	}

	ime_set_vertical_stripe_info_reg(p_set_stripe->stp_v.stp_n, p_set_stripe->stp_v.stp_l, p_set_stripe->stp_v.stp_m);
	ime_set_horizontal_overlap_reg((UINT32)p_set_stripe->overlap_h_sel, p_set_stripe->overlap_h_size, (UINT32)p_set_stripe->prt_h_sel, p_set_stripe->prt_h_size);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
VOID ime_get_in_path_stripe_info(IME_HV_STRIPE_INFO *p_get_stripe_size)
{
	UINT32 uiOverlapSel = 0;

	ime_get_horizontal_stripe_info_reg(&(p_get_stripe_size->stp_h.stp_n), &(p_get_stripe_size->stp_h.stp_l), &(p_get_stripe_size->stp_h.stp_m));
	ime_get_vertical_stripe_info_reg(&(p_get_stripe_size->stp_v.stp_n), &(p_get_stripe_size->stp_v.stp_l), &(p_get_stripe_size->stp_v.stp_m));
	ime_get_horizontal_overlap_reg(&(uiOverlapSel), &(p_get_stripe_size->overlap_h_size));

	p_get_stripe_size->overlap_h_sel = (IME_H_STRIPE_OVLP_SEL)uiOverlapSel;
	p_get_stripe_size->stp_size_mode = (IME_STRIPE_MODE_SEL)ime_get_stripe_mode_reg();

	p_get_stripe_size->stripe_cal_mode = ime_stripe_info.stripe_cal_mode;
}
#endif
//-------------------------------------------------------------------------------

//IME_STRIPE_MODE_SEL ime_get_in_path_stripe_mode(VOID)
//{
//	return (IME_STRIPE_MODE_SEL)ime_get_stripe_mode_reg();
//}
//-------------------------------------------------------------------------------


//VOID ime_set_in_path_image_format(IME_INPUT_FORMAT_SEL set_in_fmt)
//{
//	ime_set_input_image_format_reg((UINT32)set_in_fmt);
//}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_in_path_image_size(IME_SIZE_INFO *p_set_img_size)
{
	if (p_set_img_size == NULL) {
		DBG_ERR("IME: input, image size parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_img_size->size_h & 0x3) != 0) {
		DBG_ERR("IME: input, image size error, horizontal size is not 4x...\r\n");

		return E_PAR;
	}

	if (p_set_img_size->size_h < 8) {
		//
		DBG_WRN("IME: input, image width less than (<) 8 is risky, please check!!!\r\n");
	}

	if (p_set_img_size->size_v < 8) {
		//
		DBG_WRN("IME: input, image height less than (<) 8 is risky, please check!!!\r\n");
	}

	ime_set_input_image_size_reg(p_set_img_size->size_h, p_set_img_size->size_v);
	//ime_set3dnr_image_size_reg(p_set_img_size->size_h, p_set_img_size->size_v);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_in_path_lineoffset(IME_LINEOFFSET_INFO *p_set_lofs)
{
	ER er_return = E_OK;
	IME_OPMODE mode = 0;

	// do not update to PatternGen.
	mode = ime_get_operate_mode();

	if (mode == IME_OPMODE_D2D) {
		er_return = ime_check_lineoffset(p_set_lofs->lineoffset_y, p_set_lofs->lineoffset_cb);
		if (er_return != E_OK) {

			DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)p_set_lofs->lineoffset_y);
			DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)p_set_lofs->lineoffset_cb);

			return er_return;
		}

		ime_set_input_lineoffset_y_reg(p_set_lofs->lineoffset_y);
		ime_set_input_lineoffset_c_reg(p_set_lofs->lineoffset_cb);
	}

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_in_path_dma_addr(IME_DMA_ADDR_INFO *p_set_addr)
{
	ER er_return = E_OK;

	IME_OPMODE mode = 0;

	// do not update to PatternGen.
	mode = ime_get_operate_mode();

	if (mode == IME_OPMODE_D2D) {
		//er_return = ime_check_dma_addr(p_set_addr->addr_y, p_set_addr->addr_cb, p_set_addr->addr_cr, IME_DMA_INPUT);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		ime_set_input_channel_addr1_reg(p_set_addr->addr_y, p_set_addr->addr_cb, p_set_addr->addr_cr);
	}

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

VOID ime_set_in_path_burst_size(IME_BURST_SEL bst_y, IME_BURST_SEL bst_u, IME_BURST_SEL bst_v)
{
	// reserved
	UINT32 set_bst_y = 0, set_bst_u = 0, set_bst_v = 0;

	if (bst_y == IME_BURST_32W) {
		set_bst_y = 0;
	} else if (bst_y == IME_BURST_64W) {
		set_bst_y = 1;
	} else {
		// do nothing...
	}

	if (bst_u == IME_BURST_32W) {
		set_bst_u = 0;
	} else if (bst_u == IME_BURST_64W) {
		set_bst_u = 1;
	} else {
		// do nothing...
	}

	if (bst_v == IME_BURST_32W) {
		set_bst_v = 0;
	} else if (bst_v == IME_BURST_64W) {
		set_bst_v = 1;
	} else {
		// do nothing...
	}

	ime_set_input_path_burst_size_reg(set_bst_y, set_bst_u, set_bst_v);
}
//-------------------------------------------------------------------------------


//VOID ime_get_in_path_source(IME_INSRC_SEL *p_get_src)
//{
//	*p_get_src = ime_get_input_source_reg();
//}
//-------------------------------------------------------------------------------


//VOID ime_get_in_path_image_format(IME_INPUT_FORMAT_SEL *p_get_fmt)
//{
//	*p_get_fmt = ime_get_input_image_format_reg();
//}
//-------------------------------------------------------------------------------


//VOID ime_get_in_path_image_size(IME_SIZE_INFO *p_set_img_size)
//{
//	ime_get_input_image_size_reg(&(p_set_img_size->size_h), &(p_set_img_size->size_v));
//}
//-------------------------------------------------------------------------------

//VOID ime_get_in_path_lineoffset(IME_LINEOFFSET_INFO *p_get_lofs)
//{
//	p_get_lofs->lineoffset_y = ime_get_input_lineoffset_y_reg();
//	p_get_lofs->lineoffset_cb = ime_get_input_lineoffset_c_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_in_path_dma_addr(IME_DMA_ADDR_INFO *p_get_addr)
//{
//	ime_get_input_channel_addr1_reg(&(p_get_addr->addr_y), &(p_get_addr->addr_cb), &(p_get_addr->addr_cr));
//}
//-------------------------------------------------------------------------------

//UINT32 ime_get_function0_enable(VOID)
//{
//	return ime_get_function0_enable_reg();
//}
//-------------------------------------------------------------------------------

//UINT32 ime_get_function1_enable(VOID)
//{
//	return ime_get_function1_enable_reg();
//}
//-------------------------------------------------------------------------------


// path1 APIs
#if 0
VOID ime_set_out_path_p1_enable(IME_FUNC_EN set_path_en)
{
	ime_open_output_p1_reg((UINT32)set_path_en);

	// 2014/10/21,
	// issue:
	// when path1 disable and output to H.264,
	// engine is still to check the output status of H.264
	// hw. bug, checked by Heaven
	//if(SetPathEn == IME_FUNC_DISABLE)
	//{
	//    ime_set_output_dest_p1_reg(0);  // remove from NT96680
	//}
}
#endif
//-------------------------------------------------------------------------------

VOID ime_set_out_path_p1_burst_size(IME_BURST_SEL bst_y, IME_BURST_SEL bst_u, IME_BURST_SEL bst_v)
{
	// reserved
	UINT32 set_bst_y = 0, set_bst_u = 0, set_bst_v = 0;

	if (bst_y == IME_BURST_32W) {
		set_bst_y = 0;
	} else if (bst_y == IME_BURST_16W) {
		set_bst_y = 1;
	} else if (bst_y == IME_BURST_48W) {
		set_bst_y = 2;
	} else if (bst_y == IME_BURST_64W) {
		set_bst_y = 3;
	} else {
		// do nothing...
	}

	if (bst_u == IME_BURST_32W) {
		set_bst_u = 0;
	} else if (bst_u == IME_BURST_16W) {
		set_bst_u = 1;
	} else if (bst_u == IME_BURST_48W) {
		set_bst_u = 2;
	} else if (bst_u == IME_BURST_64W) {
		set_bst_u = 3;
	} else {
		// do nothing...
	}

	if (bst_v == IME_BURST_32W) {
		set_bst_v = 0;
	} else if (bst_v == IME_BURST_64W) {
		set_bst_v = 1;
	} else {
		// do nothing...
	}

	ime_set_output_path_p1_burst_size_reg(set_bst_y, set_bst_u, set_bst_v);
}
//------------------------------------------------------------------------------

#if 0
VOID ime_set_out_path_image_format_p1(IME_OUTPUT_FORMAT_TYPE set_type, IME_OUTPUT_IMG_FORMAT_SEL set_fmt)
{
	ime_set_output_type_p1_reg((UINT32)set_type);
	ime_set_output_image_format_p1_reg((UINT32)set_fmt);
}
#endif
//-------------------------------------------------------------------------------

#if 0
VOID ime_set_out_path_scale_method_p1(IME_SCALE_METHOD_SEL set_scl_method)
{
	ime_set_scale_interpolation_method_p1_reg((UINT32)set_scl_method);
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_scale_factor_p1(IME_SCALE_FACTOR_INFO *p_set_scl_factor)
{
	//UINT32 i = 0x0;

	if (p_set_scl_factor == NULL) {
		DBG_ERR("IME: path1, scale factor parameter NULL...\r\n");

		return E_PAR;
	}

	ime_set_scale_factor_h_p1_reg((UINT32)p_set_scl_factor->scale_h_ud, p_set_scl_factor->scale_h_dr, p_set_scl_factor->scale_h_ftr);
	ime_set_scale_factor_v_p1_reg((UINT32)p_set_scl_factor->scale_v_ud, p_set_scl_factor->scale_v_dr, p_set_scl_factor->scale_v_ftr);

#if 0 // output path1 do not support ISD
	for (i = 0; i < 3; i++) {
		if ((p_set_scl_factor->isd_scale_h_ftr[i] > 4096)) {
			DBG_ERR("IME: path1, ISD H-factor overflow...%d\r\n", (int)p_set_scl_factor->isd_scale_h_ftr[i]);

			return E_PAR;
		}

		if ((p_set_scl_factor->isd_scale_v_ftr[i] > 4096)) {
			DBG_ERR("IME: path1, ISD V-factor overflow...%d\r\n", (int)p_set_scl_factor->isd_scale_v_ftr[i]);

			return E_PAR;
		}
	}

	if ((p_set_scl_factor->isd_scale_h_base_ftr > 4096) || (p_set_scl_factor->isd_scale_v_base_ftr > 4096)) {
		DBG_ERR("IME: path1, ISD factor base overflow...\r\n");

		return E_PAR;
	}
#endif

	ime_set_integration_scaling_factor_base_p1_reg(p_set_scl_factor->isd_scale_h_base_ftr, p_set_scl_factor->isd_scale_v_base_ftr);

	ime_set_integration_scaling_factor0_p1_reg(p_set_scl_factor->isd_scale_h_ftr[0], p_set_scl_factor->isd_scale_v_ftr[0]);
	ime_set_integration_scaling_factor1_p1_reg(p_set_scl_factor->isd_scale_h_ftr[1], p_set_scl_factor->isd_scale_v_ftr[1]);
	ime_set_integration_scaling_factor2_p1_reg(p_set_scl_factor->isd_scale_h_ftr[2], p_set_scl_factor->isd_scale_v_ftr[2]);


	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_scale_filter_p1(IME_SCALE_FILTER_INFO *p_set_scl_filter)
{
	if (p_set_scl_filter == NULL) {
		DBG_ERR("IME: path1, scale filter parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_scl_filter->scale_h_filter_coef > 63) || (p_set_scl_filter->scale_v_filter_coef > 63)) {
		DBG_ERR("IME: path1, scale filter coef. overflow...\r\n");

		return E_PAR;
	}

	ime_set_horizontal_scale_filtering_p1_reg((UINT32)p_set_scl_filter->scale_h_filter_enable, p_set_scl_filter->scale_h_filter_coef);
	ime_set_vertical_scale_filtering_p1_reg((UINT32)p_set_scl_filter->scale_v_filter_enable, p_set_scl_filter->scale_v_filter_coef);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_scale_size_p1(IME_SIZE_INFO *p_set_size)
{
	if (p_set_size == NULL) {
		DBG_ERR("IME: path1, scaling size parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_size->size_h > 65535) || (p_set_size->size_v > 65535)) {
		DBG_ERR("IME: path1, scaling size overflow...\r\n");

		return E_PAR;
	}

	if (p_set_size->size_h < 8) {
		//
		DBG_WRN("IME: path1, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
	}

	if (p_set_size->size_v < 8) {
		//
		DBG_WRN("IME: path1, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
	}

	ime_set_scaling_size_p1_reg(p_set_size->size_h, p_set_size->size_v);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_output_crop_size_p1(IME_SIZE_INFO *p_set_scale_size, IME_SIZE_INFO *p_set_out_size, IME_POS_INFO *p_set_crop_pos)
{
	if (p_set_out_size == NULL) {
		DBG_ERR("IME: path1, crop size parameter NULL...\r\n");

		return E_PAR;
	}

	if (p_set_crop_pos == NULL) {
		DBG_ERR("IME: path1, crop position parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_out_size->size_h > 65535) || (p_set_out_size->size_v > 65535)) {
		DBG_ERR("IME: path1, horizontal or vertical crop size overflow...\r\n");

		return E_PAR;
	}

	if (p_set_out_size->size_h < 32) {
		//
		DBG_WRN("IME: path1, horizontal crop size less than (<) 32 is risky, please check!!!\r\n");
	}

	if (p_set_out_size->size_v < 32) {
		//
		DBG_WRN("IME: path1, vertical crop size less than (<) 32 is risky, please check!!!\r\n");
	}

	if (((p_set_crop_pos->pos_x + p_set_out_size->size_h) > p_set_scale_size->size_h) || ((p_set_crop_pos->pos_y + p_set_out_size->size_v) > p_set_scale_size->size_v)) {
		DBG_ERR("IME: path1, horizontal or vertical crop size is exceeding boundary...\r\n");

		return E_PAR;
	}

	ime_set_output_size_p1_reg(p_set_out_size->size_h, p_set_out_size->size_v);
	ime_set_output_crop_coordinate_p1_reg(p_set_crop_pos->pos_x, p_set_crop_pos->pos_y);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

VOID ime_get_out_path_crop_start_pos_p1(IME_POS_INFO *p_get_pos)
{
	ime_get_output_crop_coordinate_p1_reg(&(p_get_pos->pos_x), &(p_get_pos->pos_y));
}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_lineoffset_p1(IME_LINEOFFSET_INFO *p_set_lofs)
{
	ER er_return = E_OK;

	if (p_set_lofs == NULL) {
		DBG_ERR("IME: path1, lineoffset parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_lineoffset(p_set_lofs->lineoffset_y, p_set_lofs->lineoffset_cb);
	if (er_return != E_OK) {

		DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)p_set_lofs->lineoffset_y);
		DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)p_set_lofs->lineoffset_cb);

		return E_PAR;
	}

	ime_set_output_lineoffset_y_p1_reg(p_set_lofs->lineoffset_y);
	ime_set_output_lineoffset_c_p1_reg(p_set_lofs->lineoffset_cb);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_dma_addr_p1(IME_DMA_ADDR_INFO *p_set_addr)
{
	//ER er_return = E_OK;

	if (p_set_addr == NULL) {
		DBG_ERR("IME: path1, DMA address parameter NULL...\r\n");

		return E_PAR;
	}

	//er_return = ime_check_dma_addr(p_set_addr->addr_y, p_set_addr->addr_cb, p_set_addr->addr_cr, IME_DMA_OUTPUT);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	ime_set_output_channel_addr_p1_reg(p_set_addr->addr_y, p_set_addr->addr_cb, p_set_addr->addr_cr);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------


//VOID ime_set_encode_enable_p1(IME_FUNC_EN set_en)
//{
//	ime_set_encode_enable_p1_reg((UINT32)set_en);
//}
//------------------------------------------------------------------------

//VOID ime_get_out_path_enable_p1(IME_FUNC_EN *p_get_path_en)
//{
//	*p_get_path_en = (IME_FUNC_EN)ime_get_output_path_enable_status_p1_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_format_type_p1(IME_OUTPUT_FORMAT_TYPE *p_get_type)
//{
//	*p_get_type = (IME_OUTPUT_FORMAT_TYPE)ime_get_output_format_type_p1_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_image_format_p1(IME_OUTPUT_IMG_FORMAT_SEL *p_get_fmt)
//{
//	*p_get_fmt = (IME_OUTPUT_FORMAT_TYPE)ime_get_output_format_p1_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_scale_size_p1(IME_SIZE_INFO *p_get_scale_size)
//{
//	ime_get_scaling_size_p1_reg(&(p_get_scale_size->size_h), &(p_get_scale_size->size_v));
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_output_size_p1(IME_SIZE_INFO *p_get_out_size)
//{
//	ime_get_output_size_p1_reg(&(p_get_out_size->size_h), &(p_get_out_size->size_v));
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_lineoffset_p1(IME_LINEOFFSET_INFO *p_get_lofs)
//{
//	p_get_lofs->lineoffset_y = ime_get_output_lineoffset_y_p1_reg();
//	p_get_lofs->lineoffset_cb = ime_get_output_lineoffset_c_p1_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_lineoffset2_p1(IME_LINEOFFSET_INFO *p_get_lofs)
//{
//	p_get_lofs->lineoffset_y = ime_get_stitching_output_lineoffset_y2_p1_reg();
//	p_get_lofs->lineoffset_cb = ime_get_stitching_output_lineoffset_c2_p1_reg();
//}
//-------------------------------------------------------------------------------


VOID ime_get_out_path_dma_addr_p1(IME_DMA_ADDR_INFO *p_get_addr)
{
	p_get_addr->addr_y = ime_get_output_addr_y_p1_reg();
	p_get_addr->addr_cb = ime_get_output_addr_cb_p1_reg();
	p_get_addr->addr_cr = ime_get_output_addr_cr_p1_reg();
}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_dma_addr2_p1(IME_DMA_ADDR_INFO *p_get_addr)
//{
//	p_get_addr->addr_y = ime_get_stitching_output_addr_y2_p1_reg();
//	p_get_addr->addr_cb = ime_get_stitching_output_addr_cb2_p1_reg();
//	p_get_addr->addr_cr = ime_get_stitching_output_addr_cb2_p1_reg();
//}
//-------------------------------------------------------------------------------


//VOID ime_get_out_path_scale_method_p1(IME_SCALE_METHOD_SEL *p_get_method)
//{
//	*p_get_method = ime_get_scaling_method_p1_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_sprt_out_enable_p1(IME_FUNC_EN *p_get_spr_en)
//{
//	*p_get_spr_en = (IME_FUNC_EN)ime_get_stitching_enable_p1_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_out_dest_p1(IME_OUTDST_CTRL_SEL *p_get_out_dest)
//{
//	//*p_get_out_dest = (IME_OUTDST_CTRL_SEL)ime_get_output_dest_p1_reg();
//	DBG_WRN("IME: path1, output to dram...\r\n");
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_encoder_enable_p1(IME_FUNC_EN *p_get_en)
//{
//	*p_get_en = (IME_FUNC_EN)ime_get_encode_enable_p1_reg();
//}
//-------------------------------------------------------------------------------



// path2 APIs
//VOID ime_set_out_path_p2_enable(IME_FUNC_EN set_path_en)
//{
//	ime_open_output_p2_reg((UINT32)set_path_en);
//}
//-------------------------------------------------------------------------------


VOID ime_set_out_path_p2_burst_size(IME_BURST_SEL BstY, IME_BURST_SEL BstU)
{
	// reserved
	UINT32 set_bst_y = 0, set_bst_u = 0;

	if (BstY == IME_BURST_32W) {
		set_bst_y = 0;
	} else if (BstY == IME_BURST_16W) {
		set_bst_y = 1;
	} else if (BstY == IME_BURST_48W) {
		set_bst_y = 2;
	} else if (BstY == IME_BURST_64W) {
		set_bst_y = 3;
	} else {
		// do nothing...
	}

	if (BstU == IME_BURST_32W) {
		set_bst_u = 0;
	} else if (BstU == IME_BURST_16W) {
		set_bst_u = 1;
	} else if (BstU == IME_BURST_48W) {
		set_bst_u = 2;
	} else if (BstU == IME_BURST_64W) {
		set_bst_u = 3;
	} else {
		// do nothing...
	}

	ime_set_output_path_p2_burst_size_reg(set_bst_y, set_bst_u);
}
//-------------------------------------------------------------------------------

//VOID ime_set_out_path_image_format_p2(IME_OUTPUT_FORMAT_TYPE set_type, IME_OUTPUT_IMG_FORMAT_SEL set_fmt)
//{
//	ime_set_output_type_p2_reg((UINT32)set_type);
//	ime_set_output_image_format_p2_reg((UINT32)set_fmt);
//}
//-------------------------------------------------------------------------------

//VOID ime_set_out_path_scale_method_p2(IME_SCALE_METHOD_SEL set_scl_method)
//{
//	ime_set_scale_interpolation_method_p2_reg((UINT32)set_scl_method);
//}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_scale_factor_p2(IME_SCALE_FACTOR_INFO *p_set_scl_factor)
{
	//UINT32 i = 0x0;

	if (p_set_scl_factor == NULL) {
		DBG_ERR("IME: path2, scale factor parameter NULL...\r\n");

		return E_PAR;
	}

	ime_set_scale_factor_h_p2_reg((UINT32)p_set_scl_factor->scale_h_ud, p_set_scl_factor->scale_h_dr, p_set_scl_factor->scale_h_ftr);
	ime_set_scale_factor_v_p2_reg((UINT32)p_set_scl_factor->scale_v_ud, p_set_scl_factor->scale_v_dr, p_set_scl_factor->scale_v_ftr);

#if 0
	for (i = 0; i < 3; i++) {
		if ((p_set_scl_factor->isd_scale_h_ftr[i] > 4096)) {
			DBG_ERR("IME: path2, ISD H-factor overflow...%d\r\n", (int)p_set_scl_factor->isd_scale_h_ftr[i]);

			return E_PAR;
		}

		if ((p_set_scl_factor->isd_scale_v_ftr[i] > 4096)) {
			DBG_ERR("IME: path2, ISD V-factor overflow...%d\r\n", (int)p_set_scl_factor->isd_scale_v_ftr[i]);

			return E_PAR;
		}
	}

	if ((p_set_scl_factor->isd_scale_h_base_ftr > 4096) || (p_set_scl_factor->isd_scale_v_base_ftr > 4096)) {
		DBG_ERR("IME: path2, ISD factor base overflow...\r\n");

		return E_PAR;
	}

	ime_set_integration_scaling_factor_base_p2_reg(p_set_scl_factor->isd_scale_h_base_ftr, p_set_scl_factor->isd_scale_v_base_ftr);

	ime_set_integration_scaling_factor0_p2_reg(p_set_scl_factor->isd_scale_h_ftr[0], p_set_scl_factor->isd_scale_v_ftr[0]);
	ime_set_integration_scaling_factor1_p2_reg(p_set_scl_factor->isd_scale_h_ftr[1], p_set_scl_factor->isd_scale_v_ftr[1]);
	ime_set_integration_scaling_factor2_p2_reg(p_set_scl_factor->isd_scale_h_ftr[2], p_set_scl_factor->isd_scale_v_ftr[2]);

	ime_set_integration_scaling_ctrl_p2_reg(p_set_scl_factor->isd_coef_ctrl, p_set_scl_factor->isd_scale_h_coef_nums, p_set_scl_factor->isd_scale_v_coef_nums);
	ime_set_integration_scaling_h_coefs_p2_reg(p_set_scl_factor->isd_scale_h_coefs);
	ime_set_integration_scaling_v_coefs_p2_reg(p_set_scl_factor->isd_scale_v_coefs);
	ime_set_integration_scaling_h_coefs_sum_p2_reg(p_set_scl_factor->isd_scale_h_coefs_all_sum, p_set_scl_factor->isd_scale_h_coefs_half_sum);
	ime_set_integration_scaling_v_coefs_sum_p2_reg(p_set_scl_factor->isd_scale_v_coefs_all_sum, p_set_scl_factor->isd_scale_v_coefs_half_sum);
#endif

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_isd_scale_factor_p2(IME_SCALE_FACTOR_INFO *p_set_scl_factor)
{
	UINT32 i = 0x0;

	if (p_set_scl_factor == NULL) {
		DBG_ERR("IME: path2, scale factor parameter NULL...\r\n");

		return E_PAR;
	}

	//ime_set_scale_factor_h_p2_reg((UINT32)p_set_scl_factor->scale_h_ud, p_set_scl_factor->scale_h_dr, p_set_scl_factor->scale_h_ftr);
	//ime_set_scale_factor_v_p2_reg((UINT32)p_set_scl_factor->scale_v_ud, p_set_scl_factor->scale_v_dr, p_set_scl_factor->scale_v_ftr);

	for (i = 0; i < 3; i++) {
		if ((p_set_scl_factor->isd_scale_h_ftr[i] > 4096)) {
			DBG_ERR("IME: path2, ISD H-factor overflow...%d\r\n", (int)p_set_scl_factor->isd_scale_h_ftr[i]);

			return E_PAR;
		}

		if ((p_set_scl_factor->isd_scale_v_ftr[i] > 4096)) {
			DBG_ERR("IME: path2, ISD V-factor overflow...%d\r\n", (int)p_set_scl_factor->isd_scale_v_ftr[i]);

			return E_PAR;
		}
	}

	if ((p_set_scl_factor->isd_scale_h_base_ftr > 4096) || (p_set_scl_factor->isd_scale_v_base_ftr > 4096)) {
		DBG_ERR("IME: path2, ISD factor base overflow...\r\n");

		return E_PAR;
	}

	ime_set_integration_scaling_factor_base_p2_reg(p_set_scl_factor->isd_scale_h_base_ftr, p_set_scl_factor->isd_scale_v_base_ftr);

	ime_set_integration_scaling_factor0_p2_reg(p_set_scl_factor->isd_scale_h_ftr[0], p_set_scl_factor->isd_scale_v_ftr[0]);
	ime_set_integration_scaling_factor1_p2_reg(p_set_scl_factor->isd_scale_h_ftr[1], p_set_scl_factor->isd_scale_v_ftr[1]);
	ime_set_integration_scaling_factor2_p2_reg(p_set_scl_factor->isd_scale_h_ftr[2], p_set_scl_factor->isd_scale_v_ftr[2]);

	ime_set_integration_scaling_ctrl_p2_reg(p_set_scl_factor->isd_coef_ctrl, p_set_scl_factor->isd_scale_h_coef_nums, p_set_scl_factor->isd_scale_v_coef_nums);
	ime_set_integration_scaling_h_coefs_p2_reg(p_set_scl_factor->isd_scale_h_coefs);
	ime_set_integration_scaling_v_coefs_p2_reg(p_set_scl_factor->isd_scale_v_coefs);
	ime_set_integration_scaling_h_coefs_sum_p2_reg(p_set_scl_factor->isd_scale_h_coefs_all_sum, p_set_scl_factor->isd_scale_h_coefs_half_sum);
	ime_set_integration_scaling_v_coefs_sum_p2_reg(p_set_scl_factor->isd_scale_v_coefs_all_sum, p_set_scl_factor->isd_scale_v_coefs_half_sum);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_scale_filter_p2(IME_SCALE_FILTER_INFO *p_set_scl_filter)
{
	if (p_set_scl_filter == NULL) {
		DBG_ERR("IME: path2, scale filter parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_scl_filter->scale_h_filter_coef > 63) || (p_set_scl_filter->scale_v_filter_coef > 63)) {
		DBG_ERR("IME: path2, scale filter coef. overflow...\r\n");

		return E_PAR;
	}

	ime_set_horizontal_scale_filtering_p2_reg((UINT32)p_set_scl_filter->scale_h_filter_enable, p_set_scl_filter->scale_h_filter_coef);
	ime_set_vertical_scale_filtering_p2_reg((UINT32)p_set_scl_filter->scale_v_filter_enable, p_set_scl_filter->scale_v_filter_coef);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_scale_size_p2(IME_SIZE_INFO *p_set_size)
{
	if (p_set_size == NULL) {
		DBG_ERR("IME: path2, scaling size parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_size->size_h > 65535) || (p_set_size->size_v > 65535)) {
		DBG_ERR("IME: path2, scaling size overflow...\r\n");

		return E_PAR;
	}

	if (p_set_size->size_h < 8) {
		//
		DBG_WRN("IME: path2, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
	}

	if (p_set_size->size_v < 8) {
		//
		DBG_WRN("IME: path2, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
	}

	ime_set_scaling_size_p2_reg(p_set_size->size_h, p_set_size->size_v);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_output_crop_size_p2(IME_SIZE_INFO *p_set_scale_size, IME_SIZE_INFO *p_set_out_size, IME_POS_INFO *p_set_crop_pos)
{
	if (p_set_out_size == NULL) {
		DBG_ERR("IME: path2, crop size parameter NULL...\r\n");

		return E_PAR;
	}

	if (p_set_crop_pos == NULL) {
		DBG_ERR("IME: path2, crop position parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_out_size->size_h > 65535) || (p_set_out_size->size_v > 65535)) {
		DBG_ERR("IME: path2, horizontal or vertical crop size is overflow...\r\n");

		return E_PAR;
	}

	if (p_set_out_size->size_h < 8) {
		//
		DBG_WRN("IME: path2, horizontal crop size less than (<) 8 is risky, please check!!!\r\n");
	}

	if (p_set_out_size->size_v < 8) {
		//
		DBG_WRN("IME: path2, vertical crop size less than (<) 8 is risky, please check!!!\r\n");
	}

	if (((p_set_crop_pos->pos_x + p_set_out_size->size_h) > p_set_scale_size->size_h) || ((p_set_crop_pos->pos_y + p_set_out_size->size_v) > p_set_scale_size->size_v)) {
		DBG_ERR("IME: path2, horizontal or vertical crop size is exceeding boundary...\r\n");

		return E_PAR;
	}

	ime_set_output_size_p2_reg(p_set_out_size->size_h, p_set_out_size->size_v);
	ime_set_output_crop_coordinate_p2_reg(p_set_crop_pos->pos_x, p_set_crop_pos->pos_y);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_crop_start_pos_p2(IME_POS_INFO *p_get_pos)
//{
//	ime_get_output_crop_coordinate_p2_reg(&(p_get_pos->pos_x), &(p_get_pos->pos_y));
//}
//-------------------------------------------------------------------------------


ER ime_set_out_path_lineoffset_p2(IME_LINEOFFSET_INFO *p_set_lofs)
{
	ER er_return = E_OK;

	if (p_set_lofs == NULL) {
		DBG_ERR("IME: path2, lineoffset parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_lineoffset(p_set_lofs->lineoffset_y, p_set_lofs->lineoffset_cb, TRUE);
	if (er_return != E_OK) {

		DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)p_set_lofs->lineoffset_y);
		DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)p_set_lofs->lineoffset_cb);

		return E_PAR;
	}

	ime_set_output_lineoffset_y_p2_reg(p_set_lofs->lineoffset_y);
	ime_set_output_lineoffset_c_p2_reg(p_set_lofs->lineoffset_cb);

	return E_OK;
}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_dma_addr_p2(IME_DMA_ADDR_INFO *p_set_addr)
{
	//ER er_return = E_OK;

	if (p_set_addr == NULL) {
		DBG_ERR("IME: path2, DMA address parameter NULL...\r\n");

		return E_PAR;
	}

	//er_return = ime_check_dma_addr(p_set_addr->addr_y, p_set_addr->addr_cb, p_set_addr->addr_cr, IME_DMA_OUTPUT);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	ime_set_output_channel_addr_p2_reg(p_set_addr->addr_y, p_set_addr->addr_cb, p_set_addr->addr_cr);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_enable_p2(IME_FUNC_EN *p_get_path_en)
//{
//	*p_get_path_en = (IME_FUNC_EN)ime_get_output_path_enable_status_p2_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_format_type_p2(IME_OUTPUT_FORMAT_TYPE *p_get_type)
//{
//	*p_get_type = (IME_OUTPUT_FORMAT_TYPE)ime_get_output_format_type_p2_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_image_format_p2(IME_OUTPUT_IMG_FORMAT_SEL *pGetFmt)
//{
//	*pGetFmt = (IME_OUTPUT_FORMAT_TYPE)ime_get_output_format_p2_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_scale_size_p2(IME_SIZE_INFO *p_get_scale_size)
//{
//	ime_get_scaling_size_p2_reg(&(p_get_scale_size->size_h), &(p_get_scale_size->size_v));
//}
//-------------------------------------------------------------------------------


//VOID ime_get_out_path_output_size_p2(IME_SIZE_INFO *p_get_out_size)
//{
//	ime_get_output_size_p2_reg(&(p_get_out_size->size_h), &(p_get_out_size->size_v));
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_lineoffset_p2(IME_LINEOFFSET_INFO *p_get_lofs)
//{
//	p_get_lofs->lineoffset_y = ime_get_output_lineoffset_y_p2_reg();
//	p_get_lofs->lineoffset_cb = ime_get_output_lineoffset_c_p2_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_lineoffset2_p2(IME_LINEOFFSET_INFO *p_get_lofs)
//{
//	p_get_lofs->lineoffset_y = ime_get_stitching_output_lineoffset_y2_p2_reg();
//	p_get_lofs->lineoffset_cb = ime_get_stitching_output_lineoffset_c2_p2_reg();
//}
//-------------------------------------------------------------------------------


//VOID ime_get_out_path_dma_addr_p2(IME_DMA_ADDR_INFO *p_get_addr)
//{
//	p_get_addr->addr_y = ime_get_output_addr_y_p2_reg();
//	p_get_addr->addr_cb = ime_get_output_addr_cb_p2_reg();
//	p_get_addr->addr_cr = ime_get_output_addr_cb_p2_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_dma_addr2_p2(IME_DMA_ADDR_INFO *p_get_addr)
//{
//	p_get_addr->addr_y = ime_get_stitching_output_addr_y2_p2_reg();
//	p_get_addr->addr_cb = ime_get_stitching_output_addr_cb2_p2_reg();
//	p_get_addr->addr_cr = ime_get_stitching_output_addr_cb2_p2_reg();
//}
//-------------------------------------------------------------------------------


//VOID ime_get_out_path_scale_method_p2(IME_SCALE_METHOD_SEL *p_get_method)
//{
//	*p_get_method = ime_get_scaling_method_p2_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_sprt_out_enable_p2(IME_FUNC_EN *p_get_spr_en)
//{
//	*p_get_spr_en = (IME_FUNC_EN)ime_get_stitching_enable_p2_reg();
//}
//-------------------------------------------------------------------------------


// path3 APIs
//VOID ime_set_out_path_p3_enable(IME_FUNC_EN set_path_en)
//{
//	ime_open_output_p3_reg((UINT32)set_path_en);
//}
//-------------------------------------------------------------------------------



VOID ime_set_out_path_p3_burst_size(IME_BURST_SEL BstY, IME_BURST_SEL BstU)
{
	// reserved
	UINT32 set_bst_y = 0, set_bst_u = 0;

	if (BstY == IME_BURST_32W) {
		set_bst_y = 0;
	} else if (BstY == IME_BURST_64W) {
		set_bst_y = 1;
	} else {
		// do nothing...
	}

	if (BstU == IME_BURST_32W) {
		set_bst_u = 0;
	} else if (BstU == IME_BURST_64W) {
		set_bst_u = 1;
	} else {
		// do nothing...
	}

	ime_set_output_path_p3_burst_size_reg(set_bst_y, set_bst_u);
}
//-------------------------------------------------------------------------------

//VOID ime_set_out_path_image_format_p3(IME_OUTPUT_FORMAT_TYPE set_type, IME_OUTPUT_IMG_FORMAT_SEL set_fmt)
//{
//	ime_set_output_type_p3_reg((UINT32)set_type);
//	ime_set_output_image_format_p3_reg((UINT32)set_fmt);
//}
//-------------------------------------------------------------------------------

//VOID ime_set_out_path_scale_method_p3(IME_SCALE_METHOD_SEL set_scl_method)
//{
//	ime_set_scale_interpolation_method_p3_reg((UINT32)set_scl_method);
//}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_scale_factor_p3(IME_SCALE_FACTOR_INFO *p_set_scl_factor)
{
	//UINT32 i = 0x0;

	if (p_set_scl_factor == NULL) {
		DBG_ERR("IME: path3, scale factor parameter NULL...\r\n");

		return E_PAR;
	}

	ime_set_scale_factor_h_p3_reg((UINT32)p_set_scl_factor->scale_h_ud, p_set_scl_factor->scale_h_dr, p_set_scl_factor->scale_h_ftr);
	ime_set_scale_factor_v_p3_reg((UINT32)p_set_scl_factor->scale_v_ud, p_set_scl_factor->scale_v_dr, p_set_scl_factor->scale_v_ftr);

#if 0
	for (i = 0; i < 3; i++) {
		if ((p_set_scl_factor->isd_scale_h_ftr[i] > 4096)) {
			DBG_ERR("IME: path3, ISD H-factor overflow...%d\r\n", (int)p_set_scl_factor->isd_scale_h_ftr[i]);

			return E_PAR;
		}

		if ((p_set_scl_factor->isd_scale_v_ftr[i] > 4096)) {
			DBG_ERR("IME: path3, ISD V-factor overflow...%d\r\n", (int)p_set_scl_factor->isd_scale_v_ftr[i]);

			return E_PAR;
		}
	}

	if ((p_set_scl_factor->isd_scale_h_base_ftr > 4096) || (p_set_scl_factor->isd_scale_v_base_ftr > 4096)) {
		DBG_ERR("IME: path3, ISD factor base overflow...\r\n");

		return E_PAR;
	}

	ime_set_integration_scaling_factor_base_p3_reg(p_set_scl_factor->isd_scale_h_base_ftr, p_set_scl_factor->isd_scale_v_base_ftr);

	ime_set_integration_scaling_factor0_p3_reg(p_set_scl_factor->isd_scale_h_ftr[0], p_set_scl_factor->isd_scale_v_ftr[0]);
	ime_set_integration_scaling_factor1_p3_reg(p_set_scl_factor->isd_scale_h_ftr[1], p_set_scl_factor->isd_scale_v_ftr[1]);
	ime_set_integration_scaling_factor2_p3_reg(p_set_scl_factor->isd_scale_h_ftr[2], p_set_scl_factor->isd_scale_v_ftr[2]);

	ime_set_integration_scaling_ctrl_p3_reg(p_set_scl_factor->isd_coef_ctrl, p_set_scl_factor->isd_scale_h_coef_nums, p_set_scl_factor->isd_scale_v_coef_nums);
	ime_set_integration_scaling_h_coefs_p3_reg(p_set_scl_factor->isd_scale_h_coefs);
	ime_set_integration_scaling_v_coefs_p3_reg(p_set_scl_factor->isd_scale_v_coefs);

	ime_set_integration_scaling_h_coefs_sum_p3_reg(p_set_scl_factor->isd_scale_h_coefs_all_sum, p_set_scl_factor->isd_scale_h_coefs_half_sum);
	ime_set_integration_scaling_v_coefs_sum_p3_reg(p_set_scl_factor->isd_scale_v_coefs_all_sum, p_set_scl_factor->isd_scale_v_coefs_half_sum);
#endif

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_isd_scale_factor_p3(IME_SCALE_FACTOR_INFO *p_set_scl_factor)
{
	UINT32 i = 0x0;

	if (p_set_scl_factor == NULL) {
		DBG_ERR("IME: path3, scale factor parameter NULL...\r\n");

		return E_PAR;
	}

	//ime_set_scale_factor_h_p3_reg((UINT32)p_set_scl_factor->scale_h_ud, p_set_scl_factor->scale_h_dr, p_set_scl_factor->scale_h_ftr);
	//ime_set_scale_factor_v_p3_reg((UINT32)p_set_scl_factor->scale_v_ud, p_set_scl_factor->scale_v_dr, p_set_scl_factor->scale_v_ftr);

	for (i = 0; i < 3; i++) {
		if ((p_set_scl_factor->isd_scale_h_ftr[i] > 4096)) {
			DBG_ERR("IME: path3, ISD H-factor overflow...%d\r\n", (int)p_set_scl_factor->isd_scale_h_ftr[i]);

			return E_PAR;
		}

		if ((p_set_scl_factor->isd_scale_v_ftr[i] > 4096)) {
			DBG_ERR("IME: path3, ISD V-factor overflow...%d\r\n", (int)p_set_scl_factor->isd_scale_v_ftr[i]);

			return E_PAR;
		}
	}

	if ((p_set_scl_factor->isd_scale_h_base_ftr > 4096) || (p_set_scl_factor->isd_scale_v_base_ftr > 4096)) {
		DBG_ERR("IME: path3, ISD factor base overflow...\r\n");

		return E_PAR;
	}

	ime_set_integration_scaling_factor_base_p3_reg(p_set_scl_factor->isd_scale_h_base_ftr, p_set_scl_factor->isd_scale_v_base_ftr);

	ime_set_integration_scaling_factor0_p3_reg(p_set_scl_factor->isd_scale_h_ftr[0], p_set_scl_factor->isd_scale_v_ftr[0]);
	ime_set_integration_scaling_factor1_p3_reg(p_set_scl_factor->isd_scale_h_ftr[1], p_set_scl_factor->isd_scale_v_ftr[1]);
	ime_set_integration_scaling_factor2_p3_reg(p_set_scl_factor->isd_scale_h_ftr[2], p_set_scl_factor->isd_scale_v_ftr[2]);

	ime_set_integration_scaling_ctrl_p3_reg(p_set_scl_factor->isd_coef_ctrl, p_set_scl_factor->isd_scale_h_coef_nums, p_set_scl_factor->isd_scale_v_coef_nums);
	ime_set_integration_scaling_h_coefs_p3_reg(p_set_scl_factor->isd_scale_h_coefs);
	ime_set_integration_scaling_v_coefs_p3_reg(p_set_scl_factor->isd_scale_v_coefs);

	ime_set_integration_scaling_h_coefs_sum_p3_reg(p_set_scl_factor->isd_scale_h_coefs_all_sum, p_set_scl_factor->isd_scale_h_coefs_half_sum);
	ime_set_integration_scaling_v_coefs_sum_p3_reg(p_set_scl_factor->isd_scale_v_coefs_all_sum, p_set_scl_factor->isd_scale_v_coefs_half_sum);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_scale_filter_p3(IME_SCALE_FILTER_INFO *p_set_scl_filter)
{
	if (p_set_scl_filter == NULL) {
		DBG_ERR("IME: path3, scale filter parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_scl_filter->scale_h_filter_coef > 63) || (p_set_scl_filter->scale_v_filter_coef > 63)) {
		DBG_ERR("IME: path3, scale filter coef. overflow...\r\n");

		return E_PAR;
	}

	ime_set_horizontal_scale_filtering_p3_reg((UINT32)p_set_scl_filter->scale_h_filter_enable, p_set_scl_filter->scale_h_filter_coef);
	ime_set_vertical_scale_filtering_p3_reg((UINT32)p_set_scl_filter->scale_v_filter_enable, p_set_scl_filter->scale_v_filter_coef);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_scale_size_p3(IME_SIZE_INFO *p_set_size)
{
	if (p_set_size == NULL) {
		DBG_ERR("IME: path3, scale size parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_size->size_h > 65535) || (p_set_size->size_v > 65535)) {
		DBG_ERR("IME: path3, scaling size overflow...\r\n");

		return E_PAR;
	}

	if (p_set_size->size_h < 8) {
		//
		DBG_WRN("IME: path3, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
	}

	if (p_set_size->size_v < 8) {
		//
		DBG_WRN("IME: path3, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
	}

	ime_set_scaling_size_p3_reg(p_set_size->size_h, p_set_size->size_v);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_output_crop_size_p3(IME_SIZE_INFO *p_set_scale_size, IME_SIZE_INFO *p_set_out_size, IME_POS_INFO *p_set_crop_pos)
{
	if (p_set_out_size == NULL) {
		DBG_ERR("IME: path3, crop size parameter NULL...\r\n");

		return E_PAR;
	}

	if (p_set_crop_pos == NULL) {
		DBG_ERR("IME: path3, crop position parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_out_size->size_h > 65535) || (p_set_out_size->size_v > 65535)) {
		DBG_ERR("IME: path3, output size overflow...\r\n");

		return E_PAR;
	}

	if (p_set_out_size->size_h < 8) {
		//
		DBG_WRN("IME: path3, horizontal output size less than (<) 8 is risky, please check!!!\r\n");
	}

	if (p_set_out_size->size_v < 8) {
		//
		DBG_WRN("IME: path3, vertical output size less than (<) 8 is risky, please check!!!\r\n");
	}

	if (((p_set_crop_pos->pos_x + p_set_out_size->size_h) > p_set_scale_size->size_h) || ((p_set_crop_pos->pos_y + p_set_out_size->size_v) > p_set_scale_size->size_v)) {
		DBG_ERR("IME: path3, horizontal or vertical crop size is exceeding boundary...\r\n");

		return E_PAR;
	}

	ime_set_output_size_p3_reg(p_set_out_size->size_h, p_set_out_size->size_v);
	ime_set_output_crop_coordinate_p3_reg(p_set_crop_pos->pos_x, p_set_crop_pos->pos_y);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_crop_start_pos_p3(IME_POS_INFO *p_get_pos)
//{
//	ime_get_output_crop_coordinate_p3_reg(&(p_get_pos->pos_x), &(p_get_pos->pos_y));
//}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_lineoffset_p3(IME_LINEOFFSET_INFO *p_set_lofs)
{
	ER er_return = E_OK;

	if (p_set_lofs == NULL) {
		DBG_ERR("IME: path3, lineoffset parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_lineoffset(p_set_lofs->lineoffset_y, p_set_lofs->lineoffset_cb);
	if (er_return != E_OK) {

		DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)p_set_lofs->lineoffset_y);
		DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)p_set_lofs->lineoffset_cb);

		return er_return;
	}

	ime_set_output_lineoffset_y_p3_reg(p_set_lofs->lineoffset_y);
	ime_set_output_lineoffset_c_p3_reg(p_set_lofs->lineoffset_cb);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_dma_addr_p3(IME_DMA_ADDR_INFO *p_set_addr)
{
	//ER er_return = E_OK;

	if (p_set_addr == NULL) {
		DBG_ERR("IME: path3, DMA address parameter NULL...\r\n");

		return E_PAR;
	}

	//er_return = ime_check_dma_addr(p_set_addr->addr_y, p_set_addr->addr_cb, p_set_addr->addr_cr, IME_DMA_OUTPUT);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	ime_set_output_channel_addr_p3_reg(p_set_addr->addr_y, p_set_addr->addr_cb, p_set_addr->addr_cr);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_enable_p3(IME_FUNC_EN *p_get_path_en)
//{
//	*p_get_path_en = (IME_FUNC_EN)ime_get_output_path_enable_status_p3_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_format_type_p3(IME_OUTPUT_FORMAT_TYPE *p_get_type)
//{
//	*p_get_type = (IME_OUTPUT_FORMAT_TYPE)ime_get_output_format_type_p3_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_image_format_p3(IME_OUTPUT_IMG_FORMAT_SEL *pGetFmt)
//{
//	*pGetFmt = (IME_OUTPUT_FORMAT_TYPE)ime_get_output_format_p3_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_scale_size_p3(IME_SIZE_INFO *p_get_scale_size)
//{
//	ime_get_scaling_size_p3_reg(&(p_get_scale_size->size_h), &(p_get_scale_size->size_v));
//}
//-------------------------------------------------------------------------------


//VOID ime_get_out_path_output_size_p3(IME_SIZE_INFO *p_get_out_size)
//{
//	ime_get_output_size_p3_reg(&(p_get_out_size->size_h), &(p_get_out_size->size_v));
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_lineoffset_p3(IME_LINEOFFSET_INFO *p_get_lofs)
//{
//	p_get_lofs->lineoffset_y = ime_get_output_lineoffset_y_p3_reg();
//	p_get_lofs->lineoffset_cb = ime_get_output_lineoffset_c_p3_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_lineoffset2_p3(IME_LINEOFFSET_INFO *p_get_lofs)
//{
//	p_get_lofs->lineoffset_y = ime_get_stitching_output_lineoffset_y2_p3_reg();
//	p_get_lofs->lineoffset_cb = ime_get_stitching_output_lineoffset_c2_p3_reg();
//}
//-------------------------------------------------------------------------------


//VOID ime_get_out_path_dma_addr_p3(IME_DMA_ADDR_INFO *p_get_addr)
//{
//	p_get_addr->addr_y = ime_get_output_addr_y_p3_reg();
//	p_get_addr->addr_cb = ime_get_output_addr_cb_p3_reg();
//	p_get_addr->addr_cr = p_get_addr->addr_cb;
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_dma_addr2_p3(IME_DMA_ADDR_INFO *p_get_addr)
//{
//	p_get_addr->addr_y = ime_get_stitching_output_addr_y2_p3_reg();
//	p_get_addr->addr_cb = ime_get_stitching_output_addr_cb2_p3_reg();
//	p_get_addr->addr_cr = ime_get_stitching_output_addr_cb2_p3_reg();
//}
//-------------------------------------------------------------------------------


//VOID ime_get_out_path_scale_method_p3(IME_SCALE_METHOD_SEL *p_get_method)
//{
//	*p_get_method = ime_get_scaling_method_p3_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_sprt_out_enable_p3(IME_FUNC_EN *p_get_spr_en)
//{
//	*p_get_spr_en = (IME_FUNC_EN)ime_get_stitching_enable_p3_reg();
//}
//-------------------------------------------------------------------------------


// path4 APIs
VOID ime_set_out_path_p4_enable(IME_FUNC_EN set_path_en)
{
	ime_set_output_p4_enable_reg((UINT32)set_path_en);

	// #2015/06/12# adas control flow bug
	if (set_path_en == IME_FUNC_DISABLE) {
		//ime_set_adas_enable(IME_FUNC_DISABLE);
		ime_set_adas_enable_reg(IME_FUNC_DISABLE);
	}
	// #2015/06/12# end
}
//-------------------------------------------------------------------------------

//VOID ime_set_out_path_image_format_p4(IME_OUTPUT_FORMAT_TYPE set_type, IME_OUTPUT_IMG_FORMAT_SEL set_fmt)
//{
//	ime_set_output_type_p4_reg((UINT32)set_type);
//	ime_set_output_image_format_p4_reg((UINT32)set_fmt);
//}
//-------------------------------------------------------------------------------

//VOID ime_set_out_path_scale_method_p4(IME_SCALE_METHOD_SEL set_scl_method)
//{
//	ime_set_scale_interpolation_method_p4_reg((UINT32)set_scl_method);
//}
//-------------------------------------------------------------------------------

ER ime_set_out_path_scale_factor_p4(IME_SCALE_FACTOR_INFO *p_set_scl_factor)
{
	//UINT32 i = 0x0;

	if (p_set_scl_factor == NULL) {
		DBG_ERR("IME: path4, scale factor parameter NULL...\r\n");

		return E_PAR;
	}

	ime_set_scale_factor_h_p4_reg((UINT32)p_set_scl_factor->scale_h_ud, p_set_scl_factor->scale_h_dr, p_set_scl_factor->scale_h_ftr);
	ime_set_scale_factor_v_p4_reg((UINT32)p_set_scl_factor->scale_v_ud, p_set_scl_factor->scale_v_dr, p_set_scl_factor->scale_v_ftr);

#if 0
	for (i = 0; i < 3; i++) {
		if ((p_set_scl_factor->isd_scale_h_ftr[i] > 4096) || (p_set_scl_factor->isd_scale_v_ftr[i] > 4096)) {
			DBG_ERR("IME: ISD factor overflow...\r\n");

			return E_PAR;
		}
	}
	ime_set_integration_scaling_factor0_p4_reg(p_set_scl_factor->isd_scale_h_ftr[0], p_set_scl_factor->isd_scale_v_ftr[0]);
	ime_set_integration_scaling_factor1_p4_reg(p_set_scl_factor->isd_scale_h_ftr[1], p_set_scl_factor->isd_scale_v_ftr[1]);
	ime_set_integration_scaling_factor2_p4_reg(p_set_scl_factor->isd_scale_h_ftr[2], p_set_scl_factor->isd_scale_v_ftr[2]);
#endif


	return E_OK;
}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_scale_filter_p4(IME_SCALE_FILTER_INFO *p_set_scl_filter)
{
	if (p_set_scl_filter == NULL) {
		DBG_ERR("IME: path4, scale filter parameter NULL...\r\n");

		return E_PAR;
	}

#if 0
	if ((p_set_scl_filter->scale_h_filter_coef > 63) || (p_set_scl_filter->scale_v_filter_coef > 63)) {
		DBG_ERR("IME: path4, scale filter coef. overflow...\r\n");

		return E_PAR;
	}

	ime_set_horizontal_scale_filtering_p4_reg((UINT32)p_set_scl_filter->scale_h_filter_enable, p_set_scl_filter->scale_h_filter_coef);
	ime_set_vertical_scale_filtering_p4_reg((UINT32)p_set_scl_filter->scale_v_filter_enable, p_set_scl_filter->scale_v_filter_coef);
#endif

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_scale_size_p4(IME_SIZE_INFO *p_set_size)
{
	if (p_set_size == NULL) {
		DBG_ERR("IME: path4, scaling size parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_size->size_h > 65535) || (p_set_size->size_v > 65535)) {
		DBG_ERR("IME: path4, scaling size overflow...\r\n");

		return E_PAR;
	}

	if (p_set_size->size_h < 8) {
		//
		DBG_WRN("IME: path4, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
	}

	if (p_set_size->size_v < 8) {
		//
		DBG_WRN("IME: path4, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
	}

	ime_set_scaling_size_p4_reg(p_set_size->size_h, p_set_size->size_v);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_output_crop_size_p4(IME_SIZE_INFO *p_set_scale_size, IME_SIZE_INFO *p_set_out_size, IME_POS_INFO *p_set_crop_pos)
{
	if (p_set_out_size == NULL) {
		DBG_ERR("IME: path4, crop size parameter NULL...\r\n");

		return E_PAR;
	}

	if (p_set_crop_pos == NULL) {
		DBG_ERR("IME: path4, crop position parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_out_size->size_h > 65535) || (p_set_out_size->size_v > 65535)) {
		DBG_ERR("IME: path4, horizontal or vertical crop size is overflow...\r\n");

		return E_PAR;
	}

	if (p_set_out_size->size_h < 8) {
		//
		DBG_WRN("IME: path4, horizontal output size less than (<) 8 is risky, please check!!!\r\n");
	}

	if (p_set_out_size->size_v < 8) {
		//
		DBG_WRN("IME: path4, vertical output size less than (<) 8 is risky, please check!!!\r\n");
	}

	if (((p_set_crop_pos->pos_x + p_set_out_size->size_h) > p_set_scale_size->size_h) || ((p_set_crop_pos->pos_y + p_set_out_size->size_v) > p_set_scale_size->size_v)) {
		DBG_ERR("IME: path4, horizontal or vertical crop size is exceeding boundary...\r\n");

		return E_PAR;
	}

	ime_set_output_size_p4_reg(p_set_out_size->size_h, p_set_out_size->size_v);
	ime_set_output_crop_coordinate_p4_reg(p_set_crop_pos->pos_x, p_set_crop_pos->pos_y);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_crop_start_pos_p4(IME_POS_INFO *p_get_pos)
//{
//	ime_get_output_crop_coordinate_p4_reg(&(p_get_pos->pos_x), &(p_get_pos->pos_y));
//}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_lineoffset_p4(IME_LINEOFFSET_INFO *p_set_lofs)
{
	ER er_return = E_OK;

	if (p_set_lofs == NULL) {
		DBG_ERR("IME: path4, lineoffset parameter NULL...\r\n");

		return E_PAR;
	}

	er_return = ime_check_lineoffset(p_set_lofs->lineoffset_y, p_set_lofs->lineoffset_y);
	if (er_return != E_OK) {

		DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)p_set_lofs->lineoffset_y);
		DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)p_set_lofs->lineoffset_cb);

		return er_return;
	}

	ime_set_output_lineoffset_y_p4_reg(p_set_lofs->lineoffset_y);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_dma_addr_p4(IME_DMA_ADDR_INFO *p_set_addr)
{
	//ER er_return = E_OK;

	if (p_set_addr == NULL) {
		DBG_ERR("IME: path4, DMA address parameter NULL...\r\n");

		return E_PAR;
	}

	//er_return = ime_check_dma_addr(p_set_addr->addr_y, p_set_addr->addr_y, p_set_addr->addr_y, IME_DMA_OUTPUT);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	ime_set_output_channel_addr0_p4_reg(p_set_addr->addr_y, p_set_addr->addr_y, p_set_addr->addr_y);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

VOID ime_set_out_path_p4_burst_size(IME_BURST_SEL BstY)
{
	// reserved
	UINT32 set_bst_y = 0;

	if (BstY == IME_BURST_32W) {
		set_bst_y = 0;
	} else if (BstY == IME_BURST_16W) {
		set_bst_y = 1;
	} else {
		// do nothing...
	}

	ime_set_output_path_p4_burst_size_reg(set_bst_y);
}
//-------------------------------------------------------------------------------


//VOID ime_get_out_path_enable_p4(IME_FUNC_EN *p_get_path_en)
//{
//	*p_get_path_en = (IME_FUNC_EN)ime_get_output_path_enable_status_p4_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_format_type_p4(IME_OUTPUT_FORMAT_TYPE *p_get_type)
//{
//	*p_get_type = (IME_OUTPUT_FORMAT_TYPE)ime_get_output_format_type_p4_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_image_format_p4(IME_OUTPUT_IMG_FORMAT_SEL *pGetFmt)
//{
//	*pGetFmt = (IME_OUTPUT_FORMAT_TYPE)ime_get_output_format_p4_reg();
//}
//-------------------------------------------------------------------------------

VOID ime_get_out_path_scale_size_p4(IME_SIZE_INFO *p_get_scale_size)
{
	ime_get_scaling_size_p4_reg(&(p_get_scale_size->size_h), &(p_get_scale_size->size_v));
}
//-------------------------------------------------------------------------------


//VOID ime_get_out_path_output_size_p4(IME_SIZE_INFO *p_get_out_size)
//{
//	ime_get_output_size_p4_reg(&(p_get_out_size->size_h), &(p_get_out_size->size_v));
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_lineoffset_p4(IME_LINEOFFSET_INFO *p_get_lofs)
//{
//	p_get_lofs->lineoffset_y = ime_get_output_lineoffset_y_p4_reg();
//	p_get_lofs->lineoffset_cb = ime_get_output_lineoffset_c_p4_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_dma_addr_p4(IME_DMA_ADDR_INFO *p_get_addr)
//{
//	p_get_addr->addr_y = ime_get_output_addr_y_p4_reg();
//	p_get_addr->addr_cb = ime_get_output_addr_cb_p4_reg();
//	p_get_addr->addr_cr = p_get_addr->addr_cb;
//}
//-------------------------------------------------------------------------------

//VOID ime_get_out_path_scale_method_p4(IME_SCALE_METHOD_SEL *p_get_method)
//{
//	*p_get_method = ime_get_scaling_method_p4_reg();
//}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_lca_enable(IME_FUNC_EN set_en)
{
	ER er_return = E_OK;

	ime_set_lca_enable_reg((UINT32)set_en);

	if (set_en == IME_FUNC_ENABLE) {
		er_return = ime_set_lca_scale_factor();
		if (er_return != E_OK) {
			DBG_ERR("IME: LCA, scale factor error...\r\n");

			return er_return;
		}
	}

	return er_return;
}
#endif
//-------------------------------------------------------------------------------

//VOID ime_set_lca_chroma_adj_enable(IME_FUNC_EN set_en)
//{
//	ime_set_lca_chroma_adj_enable_reg((UINT32)set_en);
//}
//-------------------------------------------------------------------------------

//VOID ime_set_lca_luma_enable(IME_FUNC_EN set_en)
//{
//	ime_set_lca_la_enable_reg((UINT32)set_en);
//}
//-------------------------------------------------------------------------------

ER ime_set_lca_scale_factor(VOID)
{
	// reserved
	UINT32 in_sizeH = 0, in_sizeV = 0;
	UINT32 OutSizeH = 0, OutSizeV = 0;
	UINT32 ScaleFactorH = 0, ScaleFactorV = 0;

	ime_get_input_image_size_reg(&in_sizeH, &in_sizeV);
	ime_get_lca_image_size_reg(&OutSizeH, &OutSizeV);

	if (in_sizeH <= 1) {
		DBG_ERR("IME: LCA, input image width less than 1...\r\n");
		return E_PAR;
	}

	if (in_sizeV <= 1) {
		DBG_ERR("IME: LCA, input image height less than 1...\r\n");
		return E_PAR;
	}

	ScaleFactorH = (OutSizeH - 1) * 65536 / (in_sizeH - 1);
	ScaleFactorV = (OutSizeV - 1) * 65536 / (in_sizeV - 1);

	ime_set_lca_scale_factor_reg(ScaleFactorH, ScaleFactorV);

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_set_lca_input_image_info(IME_CHROMA_ADAPTION_IMAGE_INFO *p_set_info)
{
	// reserved
	ER er_return = E_OK;

	UINT32 tmp_lofs_y = 0x0, tmp_lofs_uv = 0x0;
	//UINT32 ScaleFactorH = 0, ScaleFactorV = 0;
	UINT32 flush_size_y = 0x0, flush_size_c = 0x0;

	IME_DMA_ADDR_INFO set_addr = {0x0};

	if (p_set_info == NULL) {
		DBG_ERR("IME: LCA, input image parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_info->lca_img_size.size_h > 65535) || (p_set_info->lca_img_size.size_v > 65535)) {
		DBG_ERR("IME: LCA, image size overflow...\r\n");

		return E_PAR;
	}

#if 0
	if (p_set_info->lca_fmt == IME_LCAF_YCCYCC) {
		tmp_lofs_y = p_set_info->lca_lofs.lineoffset_y;
		tmp_lofs_uv = p_set_info->lca_lofs.lineoffset_y;
	} else if (p_set_info->lca_fmt == IME_LCAF_YCCP) {
		tmp_lofs_y = p_set_info->lca_lofs.lineoffset_y;
		tmp_lofs_uv = p_set_info->lca_lofs.lineoffset_cb;
	}

	er_return = ime_check_lineoffset(tmp_lofs_y, tmp_lofs_uv);
	if (er_return != E_OK) {
		return er_return;
	}

	//ime_get_in_path_image_size(&ime_input_size);
	//ScaleFactorH = (p_set_info->lca_img_size.size_h - 1) * 65536 / (ime_input_size.size_h - 1);
	//ScaleFactorV = (p_set_info->lca_img_size.size_v - 1) * 65536 / (ime_input_size.size_v - 1);


	set_addr = p_set_info->lca_dma_addr0;

#if 0 // removed from nt96680
	ime_tmp_addr1 = p_set_info->lca_dma_addr0;
	if (p_set_info->LcaPPBEnable == IME_FUNC_ENABLE) {
		ime_tmp_addr1 = p_set_info->LcaDmaAddr1;
	}
#endif

	//ime_engine_operation_mode = ime_get_operate_mode();

	flush_size_y = tmp_lofs_y * p_set_info->lca_img_size.size_v;
	flush_size_c = tmp_lofs_uv * p_set_info->lca_img_size.size_v;

	set_addr.addr_cr = set_addr.addr_cb = set_addr.addr_y;
	set_addr.addr_y  = ime_input_dma_buf_flush(set_addr.addr_y, flush_size_y, ime_engine_operation_mode);
	set_addr.addr_cb = ime_input_dma_buf_flush(set_addr.addr_cb, flush_size_c, ime_engine_operation_mode);
	set_addr.addr_cr = ime_input_dma_buf_flush(set_addr.addr_cr, flush_size_c, ime_engine_operation_mode);
	//er_return = ime_check_dma_addr(set_addr.addr_y, set_addr.addr_cb, set_addr.addr_cr, IME_DMA_INPUT);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

#if 0 // removed from nt96680
	ime_tmp_addr1.addr_cr = ime_tmp_addr1.addr_cb = ime_tmp_addr1.addr_y;
	ime_tmp_addr1.addr_y  = ime_input_dma_buf_flush(ime_tmp_addr1.addr_y, flush_size_y, ime_engine_operation_mode);
	ime_tmp_addr1.addr_cb = ime_input_dma_buf_flush(ime_tmp_addr1.addr_cb, flush_size_c, ime_engine_operation_mode);
	ime_tmp_addr1.addr_cr = ime_input_dma_buf_flush(ime_tmp_addr1.addr_cr, flush_size_c, ime_engine_operation_mode);
	//er_return = ime_check_dma_addr(ime_tmp_addr1.addr_y, ime_tmp_addr1.addr_cb, ime_tmp_addr1.addr_cr, IME_DMA_INPUT);
	//if (er_return != E_OK) {
	//  return er_return;
	//}
#endif

	//ime_set_chroma_adapt_enable_ppb_reg((UINT32)p_set_info->LcaPPBEnable);, remove from NT96680
	ime_set_lca_image_size_reg(p_set_info->lca_img_size.size_h, p_set_info->lca_img_size.size_v);
	//ime_set_lca_scale_factor_reg(ScaleFactorH, ScaleFactorV);
	ime_set_lca_source_reg((UINT32)p_set_info->lca_src);
	ime_set_lca_format_reg((UINT32)p_set_info->lca_fmt);
	ime_set_lca_lineoffset_reg(tmp_lofs_y, tmp_lofs_uv);
	ime_set_lca_dma_addr0_reg(set_addr.addr_y, set_addr.addr_cb);
	//ime_set_lca_dma_addr1_reg(ime_tmp_addr1.addr_y, ime_tmp_addr1.addr_cb);  // removed from nt96680

	er_return = ime_set_lca_scale_factor();
	if (er_return != E_OK) {
		return er_return;
	}
#else
	//ime_get_in_path_image_size(&ime_input_size);
	//ScaleFactorH = (p_set_info->lca_img_size.size_h - 1) * 65536 / (ime_input_size.size_h - 1);
	//ScaleFactorV = (p_set_info->lca_img_size.size_v - 1) * 65536 / (ime_input_size.size_v - 1);


	//ime_set_chroma_adapt_enable_ppb_reg((UINT32)p_set_info->LcaPPBEnable);, remove from NT96680
	ime_set_lca_image_size_reg(p_set_info->lca_img_size.size_h, p_set_info->lca_img_size.size_v);
	//ime_set_lca_scale_factor_reg(ScaleFactorH, ScaleFactorV);
	ime_set_lca_source_reg((UINT32)p_set_info->lca_src);
	ime_set_lca_format_reg((UINT32)p_set_info->lca_fmt);

	if ((p_set_info->lca_src == IME_LCA_SRC_DRAM)) {
		if (p_set_info->lca_fmt == IME_LCAF_YCCYCC) {
			tmp_lofs_y = p_set_info->lca_lofs.lineoffset_y;
			tmp_lofs_uv = p_set_info->lca_lofs.lineoffset_y;
		} else if (p_set_info->lca_fmt == IME_LCAF_YCCP) {
			tmp_lofs_y = p_set_info->lca_lofs.lineoffset_y;
			tmp_lofs_uv = p_set_info->lca_lofs.lineoffset_cb;
		}

		er_return = ime_check_lineoffset(tmp_lofs_y, tmp_lofs_uv, TRUE);
		if (er_return != E_OK) {

			DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)tmp_lofs_y);
			DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)tmp_lofs_uv);

			return er_return;
		}

		set_addr = p_set_info->lca_dma_addr0;

#if 0 // removed from nt96680
		ime_tmp_addr1 = p_set_info->lca_dma_addr0;
		if (p_set_info->LcaPPBEnable == IME_FUNC_ENABLE) {
			ime_tmp_addr1 = p_set_info->LcaDmaAddr1;
		}
#endif

		//ime_engine_operation_mode = ime_get_operate_mode();

		flush_size_y = tmp_lofs_y * p_set_info->lca_img_size.size_v;
		flush_size_y = IME_ALIGN_CEIL_32(flush_size_y);
		flush_size_c = tmp_lofs_uv * p_set_info->lca_img_size.size_v;
		flush_size_c = IME_ALIGN_CEIL_32(flush_size_c);

		set_addr.addr_cr = set_addr.addr_cb = set_addr.addr_y;
		set_addr.addr_y  = ime_input_dma_buf_flush(set_addr.addr_y, flush_size_y, ime_engine_operation_mode);
		set_addr.addr_cb = ime_input_dma_buf_flush(set_addr.addr_cb, flush_size_c, ime_engine_operation_mode);
		//set_addr.addr_cr = ime_input_dma_buf_flush(set_addr.addr_cr, flush_size_c, ime_engine_operation_mode);
		//er_return = ime_check_dma_addr(set_addr.addr_y, set_addr.addr_cb, set_addr.addr_cr, IME_DMA_INPUT);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

#if 0 // removed from nt96680
		ime_tmp_addr1.addr_cr = ime_tmp_addr1.addr_cb = ime_tmp_addr1.addr_y;
		ime_tmp_addr1.addr_y  = ime_input_dma_buf_flush(ime_tmp_addr1.addr_y, flush_size_y, ime_engine_operation_mode);
		ime_tmp_addr1.addr_cb = ime_input_dma_buf_flush(ime_tmp_addr1.addr_cb, flush_size_c, ime_engine_operation_mode);
		ime_tmp_addr1.addr_cr = ime_input_dma_buf_flush(ime_tmp_addr1.addr_cr, flush_size_c, ime_engine_operation_mode);
		//er_return = ime_check_dma_addr(ime_tmp_addr1.addr_y, ime_tmp_addr1.addr_cb, ime_tmp_addr1.addr_cr, IME_DMA_INPUT);
		//if (er_return != E_OK) {
		//  return er_return;
		//}
#endif

		ime_set_lca_lineoffset_reg(tmp_lofs_y, tmp_lofs_uv);
		ime_set_lca_dma_addr0_reg(set_addr.addr_y, set_addr.addr_cb);
		//ime_set_lca_dma_addr1_reg(ime_tmp_addr1.addr_y, ime_tmp_addr1.addr_cb);  // removed from nt96680
	} else if ((p_set_info->lca_src == IME_LCA_SRC_ALL)) {
		if (p_set_info->lca_fmt == IME_LCAF_YCCYCC) {
			tmp_lofs_y = p_set_info->lca_lofs.lineoffset_y;
			tmp_lofs_uv = p_set_info->lca_lofs.lineoffset_y;
		} else if (p_set_info->lca_fmt == IME_LCAF_YCCP) {
			tmp_lofs_y = p_set_info->lca_lofs.lineoffset_y;
			tmp_lofs_uv = p_set_info->lca_lofs.lineoffset_cb;
		}

		er_return = ime_check_lineoffset(tmp_lofs_y, tmp_lofs_uv, TRUE);
		if (er_return != E_OK) {

			DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)tmp_lofs_y);
			DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)tmp_lofs_uv);

			return er_return;
		}

		set_addr = p_set_info->lca_dma_addr0;

#if 0 // removed from nt96680
		ime_tmp_addr1 = p_set_info->lca_dma_addr0;
		if (p_set_info->LcaPPBEnable == IME_FUNC_ENABLE) {
			ime_tmp_addr1 = p_set_info->LcaDmaAddr1;
		}
#endif

		//ime_engine_operation_mode = ime_get_operate_mode();

		flush_size_y = tmp_lofs_y * p_set_info->lca_img_size.size_v;
		flush_size_y = IME_ALIGN_CEIL_32(flush_size_y);
		flush_size_c = tmp_lofs_uv * p_set_info->lca_img_size.size_v;
		flush_size_c = IME_ALIGN_CEIL_32(flush_size_c);

		set_addr.addr_cr = set_addr.addr_cb = set_addr.addr_y;
		set_addr.addr_y  = ime_input_dma_buf_flush(set_addr.addr_y, flush_size_y, ime_engine_operation_mode);
		set_addr.addr_cb = ime_input_dma_buf_flush(set_addr.addr_cb, flush_size_c, ime_engine_operation_mode);
		//set_addr.addr_cr = ime_input_dma_buf_flush(set_addr.addr_cr, flush_size_c, ime_engine_operation_mode);
		//er_return = ime_check_dma_addr(set_addr.addr_y, set_addr.addr_cb, set_addr.addr_cr, IME_DMA_INPUT);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

#if 0 // removed from nt96680
		ime_tmp_addr1.addr_cr = ime_tmp_addr1.addr_cb = ime_tmp_addr1.addr_y;
		ime_tmp_addr1.addr_y  = ime_input_dma_buf_flush(ime_tmp_addr1.addr_y, flush_size_y, ime_engine_operation_mode);
		ime_tmp_addr1.addr_cb = ime_input_dma_buf_flush(ime_tmp_addr1.addr_cb, flush_size_c, ime_engine_operation_mode);
		ime_tmp_addr1.addr_cr = ime_input_dma_buf_flush(ime_tmp_addr1.addr_cr, flush_size_c, ime_engine_operation_mode);
		//er_return = ime_check_dma_addr(ime_tmp_addr1.addr_y, ime_tmp_addr1.addr_cb, ime_tmp_addr1.addr_cr, IME_DMA_INPUT);
		//if (er_return != E_OK) {
		//  return er_return;
		//}
#endif

		ime_set_lca_lineoffset_reg(tmp_lofs_y, tmp_lofs_uv);
		ime_set_lca_dma_addr0_reg(set_addr.addr_y, set_addr.addr_cb);
		//ime_set_lca_dma_addr1_reg(ime_tmp_addr1.addr_y, ime_tmp_addr1.addr_cb);  // removed from nt96680


		ime_set_lca_source_reg((UINT32)IME_LCA_SRC_IFE2);
	}

	er_return = ime_set_lca_scale_factor();
	if (er_return != E_OK) {
		return er_return;
	}
#endif

	return E_OK;
}

//-------------------------------------------------------------------------------

#if 0
ER ime_set_lca_chroma_adjust_info(IME_CHROMA_ADAPTION_CA_INFO *p_set_info)
{
	if (p_set_info == NULL) {
		DBG_ERR("IME: LCA, chroma adjust parameter NULL...\r\n");

		return E_PAR;
	}

	if (p_set_info->lca_ca_uv_wet_prc <= IME_RANGE_8) {
		if ((p_set_info->lca_ca_uv_wet_start > 8) || (p_set_info->lca_ca_uv_wet_end > 8)) {
			DBG_ERR("IME: LCA, chroma adjust weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_ca_uv_wet_prc <= IME_RANGE_16) {
		if ((p_set_info->lca_ca_uv_wet_start > 16) || (p_set_info->lca_ca_uv_wet_end > 16)) {
			DBG_ERR("IME: LCA, chroma adjust weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_ca_uv_wet_prc <= IME_RANGE_32) {
		if ((p_set_info->lca_ca_uv_wet_start > 32) || (p_set_info->lca_ca_uv_wet_end > 32)) {
			DBG_ERR("IME: LCA, chroma adjust weighting error...\r\n");

			return E_PAR;
		}
	} else {
		DBG_ERR("IME: LCA, chroma adjust max weighting is 32...\r\n");

		return E_PAR;
	}

	//ime_set_lca_chroma_adj_enable_reg((UINT32)p_set_info->lca_ca_enable);

	ime_set_lca_chroma_adj_enable_reg((UINT32)p_set_info->lca_ca_enable);
	ime_set_lca_ca_adjust_center_reg(p_set_info->lca_ca_cent_u, p_set_info->lca_ca_cent_v);
	ime_set_lca_ca_range_reg((UINT32)p_set_info->lca_ca_uv_range, (UINT32)p_set_info->lca_ca_uv_wet_prc);
	ime_set_lca_ca_weight_reg(p_set_info->lca_ca_uv_th, p_set_info->lca_ca_uv_wet_start, p_set_info->lca_ca_uv_wet_end);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_lca_chroma_adapt_info(IME_CHROMA_ADAPTION_IQC_INFO *p_set_info)
{
	if (p_set_info == NULL) {
		DBG_ERR("IME: LCA, chroma adaptation parameter NULL...\r\n");

		return E_PAR;
	}

	if (p_set_info->lca_y_wet_prc <= IME_RANGE_8) {
		if ((p_set_info->lca_y_wet_start > 8) || (p_set_info->lca_y_wet_end > 8)) {
			DBG_ERR("IME: LCA, chroma adaptation Y weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_y_wet_prc <= IME_RANGE_16) {
		if ((p_set_info->lca_y_wet_start > 16) || (p_set_info->lca_y_wet_end > 16)) {
			DBG_ERR("IME: LCA, chroma adaptation Y weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_y_wet_prc <= IME_RANGE_32) {
		if ((p_set_info->lca_y_wet_start > 32) || (p_set_info->lca_y_wet_end > 32)) {
			DBG_ERR("IME: LCA, chroma adaptation Y weighting error...\r\n");

			return E_PAR;
		}
	}

	if (p_set_info->lca_uv_wet_prc <= IME_RANGE_8) {
		if ((p_set_info->lca_uv_wet_start > 8) || (p_set_info->lca_uv_wet_end > 8)) {
			DBG_ERR("IME: LCA, chroma adaptation UV weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_uv_wet_prc <= IME_RANGE_16) {
		if ((p_set_info->lca_uv_wet_start > 16) || (p_set_info->lca_uv_wet_end > 16)) {
			DBG_ERR("IME: LCA, chroma adaptation UV weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_uv_wet_prc <= IME_RANGE_32) {
		if ((p_set_info->lca_uv_wet_start > 32) || (p_set_info->lca_uv_wet_end > 32)) {
			DBG_ERR("IME: LCA, chroma adaptation UV weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_uv_wet_prc <= IME_RANGE_64) {
		if ((p_set_info->lca_uv_wet_start > 64) || (p_set_info->lca_uv_wet_end > 64)) {
			DBG_ERR("IME: LCA, chroma adaptation UV weighting error...\r\n");

			return E_PAR;
		}
	} else {
		DBG_ERR("IME: LCA, chroma adaptation UV max weighting is 64...\r\n");

		return E_PAR;
	}

	ime_set_lca_chroma_ref_image_weight_reg(p_set_info->lca_ref_y_wet, p_set_info->lca_ref_uv_wet, p_set_info->lca_out_uv_wet);
	ime_set_lca_chroma_range_y_reg(p_set_info->lca_y_range, p_set_info->lca_y_wet_prc);
	ime_set_lca_chroma_weight_y_reg(p_set_info->lca_y_th, p_set_info->lca_y_wet_start, p_set_info->lca_y_wet_end);

	ime_set_lca_chroma_range_uv_reg(p_set_info->lca_uv_range, p_set_info->lca_uv_wet_prc);
	ime_set_lca_chroma_weight_uv_reg(p_set_info->lca_uv_th, p_set_info->lca_uv_wet_start, p_set_info->lca_uv_wet_end);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_lca_luma_suppress_info(IME_CHROMA_ADAPTION_IQL_INFO *p_set_info)
{
	if (p_set_info == NULL) {
		DBG_ERR("IME: LCA, luma adaptation parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_info->lca_la_ref_wet > 31) || (p_set_info->lca_la_out_wet > 31)) {
		DBG_ERR("IME: LCA, luma adaptation Y weighting error...\r\n");

		return E_PAR;
	}

	if (p_set_info->lca_la_wet_prc <= IME_RANGE_8) {
		if ((p_set_info->lca_la_wet_start > 8) || (p_set_info->lca_la_wet_end > 8)) {
			DBG_ERR("IME: LCA, luma adaptation Y weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_la_wet_prc <= IME_RANGE_16) {
		if ((p_set_info->lca_la_wet_start > 16) || (p_set_info->lca_la_wet_end > 16)) {
			DBG_ERR("IME: LCA, luma adaptation Y weighting error...\r\n");

			return E_PAR;
		}
	} else if (p_set_info->lca_la_wet_prc <= IME_RANGE_32) {
		if ((p_set_info->lca_la_wet_start > 32) || (p_set_info->lca_la_wet_end > 32)) {
			DBG_ERR("IME: LCA, luma adaptation Y weighting error...\r\n");

			return E_PAR;
		}
	} else {
		DBG_ERR("IME: LCA, luma adaptation Y max weighting is 32...\r\n");

		return E_PAR;
	}

	ime_set_lca_la_enable_reg((UINT32)p_set_info->lca_la_enable);
	ime_set_lca_luma_ref_image_weight_reg(p_set_info->lca_la_ref_wet, p_set_info->lca_la_out_wet);
	ime_set_lca_luma_range_y_reg(p_set_info->lca_la_range, p_set_info->lca_la_wet_prc);
	ime_set_lca_luma_weight_y_reg(p_set_info->lca_la_th, p_set_info->lca_la_wet_start, p_set_info->lca_la_wet_end);

	return  E_OK;
}
#endif
//-------------------------------------------------------------------------------

VOID ime_set_in_path_lca_burst_size(IME_BURST_SEL bst)
{
	// reserved
	UINT32 set_bst = 0;

	if (bst == IME_BURST_32W) {
		set_bst = 0;
	} else if (bst == IME_BURST_16W) {
		set_bst = 1;
	} else {
		// do nothing...
	}

	ime_set_input_path_lca_burst_size_reg(set_bst);
}
//-------------------------------------------------------------------------------

VOID ime_set_out_path_lca_burst_size(IME_BURST_SEL bst)
{
	// reserved
	UINT32 set_bst = 0;

	if (bst == IME_BURST_32W) {
		set_bst = 0;
	} else if (bst == IME_BURST_16W) {
		set_bst = 1;
	} else {
		// do nothing...
	}

	ime_set_output_path_lca_burst_size_reg(set_bst);
}
//-------------------------------------------------------------------------------


//IME_FUNC_EN ime_get_ila_enable(VOID)
//{
//	return (IME_FUNC_EN)ime_get_lca_enable_reg();
//}
//-------------------------------------------------------------------------------

//VOID ime_get_lca_image_size_info(IME_SIZE_INFO *p_get_info)
//{
//	ime_get_lca_image_size_reg(&(p_get_info->size_h), &(p_get_info->size_v));
//}
//-------------------------------------------------------------------------------

//VOID ime_get_lca_lineoffset_info(IME_LINEOFFSET_INFO *p_get_info)
//{
//	ime_get_lca_lineoffset_reg(&(p_get_info->lineoffset_y), &(p_get_info->lineoffset_cb));
//}
//-------------------------------------------------------------------------------

//VOID ime_get_lca_dma_addr_info(IME_DMA_ADDR_INFO *p_get_info_set0, IME_DMA_ADDR_INFO *p_get_info_set1)
//{
//	if (p_get_info_set0 != NULL) {
//		ime_get_lca_dma_addr0_reg(&(p_get_info_set1->addr_y), &(p_get_info_set1->addr_cb));
//	}
//}
//-------------------------------------------------------------------------------

//ER ime_set_lca_subout_enable(IME_FUNC_EN set_en)
//{
//	ime_set_lca_subout_enable_reg((UINT32)set_en);
//
//	return E_OK;
//}
//-------------------------------------------------------------------------------

//VOID ime_set_lca_subout_src(IME_LCA_SUBOUT_SRC set_src)
//{
//	ime_set_lca_subout_source_reg((UINT32)set_src);
//}
//-------------------------------------------------------------------------------


//UINT32 ime_get_lca_subout_enable(VOID)
//{
//	return ime_get_lca_subout_enable_reg();
//}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_lca_subout_scale_factor(IME_SCALE_FACTOR_INFO *p_set_scl_factor)
{
	UINT32 i = 0x0;

	if (p_set_scl_factor == NULL) {
		DBG_ERR("IME: LCA subout, parameter NULL...\r\n");

		return E_PAR;
	}

	ime_set_lca_subout_scale_factor_h_reg(p_set_scl_factor->scale_h_dr, p_set_scl_factor->scale_h_ftr);
	ime_set_lca_subout_scale_factor_v_reg(p_set_scl_factor->scale_v_dr, p_set_scl_factor->scale_v_ftr);

	for (i = 0; i < 3; i++) {
		if ((p_set_scl_factor->isd_scale_h_ftr[i] > 4096) || (p_set_scl_factor->isd_scale_v_ftr[i] > 4096)) {
			DBG_ERR("IME: LCA subout, ISD factor overflow...\r\n");

			return E_PAR;
		}
	}

	if ((p_set_scl_factor->isd_scale_h_base_ftr > 4096) || (p_set_scl_factor->isd_scale_v_base_ftr > 4096)) {
		DBG_ERR("IME: LCA subout, factor base overflow...\r\n");

		return E_PAR;
	}

	ime_set_lca_subout_isd_scale_factor_base_reg(p_set_scl_factor->isd_scale_h_base_ftr, p_set_scl_factor->isd_scale_v_base_ftr);

	ime_set_lca_subout_isd_scale_factor0_reg(p_set_scl_factor->isd_scale_h_ftr[0], p_set_scl_factor->isd_scale_v_ftr[0]);
	ime_set_lca_subout_isd_scale_factor1_reg(p_set_scl_factor->isd_scale_h_ftr[1], p_set_scl_factor->isd_scale_v_ftr[1]);
	ime_set_lca_subout_isd_scale_factor2_reg(p_set_scl_factor->isd_scale_h_ftr[2], p_set_scl_factor->isd_scale_v_ftr[2]);

	ime_set_lca_subout_isd_scale_coef_number_reg(p_set_scl_factor->isd_scale_h_coef_nums, p_set_scl_factor->isd_scale_v_coef_nums);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_lca_subout_lineoffset(UINT32 lofs)
{
	ER er_return = E_OK;

	if (lofs == 0x0) {
		DBG_ERR("IME: LCA subout, lineoffset = 0x0, error\r\n");

		return E_PAR;
	}

	er_return = ime_check_lineoffset(lofs, lofs);
	if (er_return != E_OK) {

		DBG_ERR("IME - uiLineOffsetY: %08x\r\n", (unsigned int)lofs);
		DBG_ERR("IME - uiLineOffsetCb: %08x\r\n", (unsigned int)lofs);

		return E_PAR;
	}

	ime_set_lca_subout_lineoffset_reg(lofs);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

//UINT32 ime_get_lca_subout_lineoffset(VOID)
//{
//	return ime_get_lca_subout_lineoffset_reg();
//}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_lca_subout_dma_addr(UINT32 addr)
{
	//ER er_return = E_OK;

	//er_return = ime_check_dma_addr(addr, addr, addr, IME_DMA_OUTPUT);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	ime_set_lca_subout_dma_addr_reg(addr);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

//UINT32 ime_get_lca_subout_dma_addr(VOID)
//{
//	return ime_get_lca_subout_dma_addr_reg();
//}
//-------------------------------------------------------------------------------

ER ime_set_lca_subout(IME_CHROMA_APAPTION_SUBOUT_PARAM *p_set_info)
{
	// reserved
	ER er_return = E_OK;

	UINT32 scale_down_factor = 0, i = 0;

	ime_scale_factor_params = p_set_info->subout_scale_factor;
	if (p_set_info->subout_scale_factor.CalScaleFactorMode == IME_SCALE_FACTOR_COEF_AUTO_MODE) {
		scale_down_factor = (p_set_info->pri_img_size.size_h - 1) * 512 / (p_set_info->ref_img_size.size_h - 1);

		if (scale_down_factor > 8191) {
			DBG_ERR("IME: LCA subout, horizontal scale rate must be <= 15.99x\r\n");

			return E_PAR;
		}

		er_return = ime_cal_image_scale_params(&(p_set_info->pri_img_size), &(p_set_info->ref_img_size), IMEALG_INTEGRATION, &ime_scale_factor_params);
		if (er_return != E_OK) {
			return er_return;
		}
	}

	//-----------------------------------------------------------------------------------

	//er_return = ime_set_lca_subout_scale_factor(&ime_scale_factor_params);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	ime_set_lca_subout_scale_factor_h_reg(ime_scale_factor_params.scale_h_dr, ime_scale_factor_params.scale_h_ftr);
	ime_set_lca_subout_scale_factor_v_reg(ime_scale_factor_params.scale_v_dr, ime_scale_factor_params.scale_v_ftr);

	for (i = 0; i < 3; i++) {
		if ((ime_scale_factor_params.isd_scale_h_ftr[i] > 4096) || (ime_scale_factor_params.isd_scale_v_ftr[i] > 4096)) {
			DBG_ERR("IME: LCA subout, ISD factor overflow...\r\n");

			return E_PAR;
		}
	}

	if ((ime_scale_factor_params.isd_scale_h_base_ftr > 4096) || (ime_scale_factor_params.isd_scale_v_base_ftr > 4096)) {
		DBG_ERR("IME: LCA subout, factor base overflow...\r\n");

		return E_PAR;
	}

	ime_set_lca_subout_isd_scale_factor_base_reg(ime_scale_factor_params.isd_scale_h_base_ftr, ime_scale_factor_params.isd_scale_v_base_ftr);

	ime_set_lca_subout_isd_scale_factor0_reg(ime_scale_factor_params.isd_scale_h_ftr[0], ime_scale_factor_params.isd_scale_v_ftr[0]);
	ime_set_lca_subout_isd_scale_factor1_reg(ime_scale_factor_params.isd_scale_h_ftr[1], ime_scale_factor_params.isd_scale_v_ftr[1]);
	ime_set_lca_subout_isd_scale_factor2_reg(ime_scale_factor_params.isd_scale_h_ftr[2], ime_scale_factor_params.isd_scale_v_ftr[2]);

	ime_set_lca_subout_isd_scale_coef_number_reg(ime_scale_factor_params.isd_scale_h_coef_nums, ime_scale_factor_params.isd_scale_v_coef_nums);

	//-----------------------------------------------------------------------------------

	ime_set_lca_image_size_reg(p_set_info->ref_img_size.size_h, p_set_info->ref_img_size.size_v);

	return E_OK;
}
//-------------------------------------------------------------------------------


//VOID ime_set_dbcs_enable(IME_FUNC_EN set_en)
//{
//	ime_set_dbcs_enable_reg((UINT32)set_en);
//}
//-------------------------------------------------------------------------------

//IME_FUNC_EN ime_get_dbcs_enable(VOID)
//{
//	return (IME_FUNC_EN)ime_get_dbcs_enable_reg();
//}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_dbcs_iq_info(IME_DBCS_IQ_INFO *p_set_info)
{
	UINT32 i = 0x0;

	if (p_set_info == NULL) {
		DBG_ERR("IME: DBCS, setting parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_info->uiCentU > 255) || (p_set_info->uiCentV > 255)) {
		DBG_ERR("IME: DBCS, center parameter overflow...\r\n");

		return E_PAR;
	}

	for (i = 0; i < 16; i++) {
		if (p_set_info->puiWtY[i] > 16) {
			DBG_ERR("IME: DBCS, weighting y parameter overflow...\r\n");

			return E_PAR;
		}
	}

	for (i = 0; i < 16; i++) {
		if (p_set_info->puiWtC[i] > 16) {
			DBG_ERR("IME: DBCS, weighting uv parameter overflow...\r\n");

			return E_PAR;
		}
	}

	ime_set_dbcs_mode_reg((UINT32)p_set_info->OpMode);
	ime_set_dbcs_step_reg(p_set_info->uiStepY, p_set_info->uiStepC);
	ime_set_dbcs_center_reg(p_set_info->uiCentU, p_set_info->uiCentV);
	ime_set_dbcs_weight_y_reg(p_set_info->puiWtY);
	ime_set_dbcs_weight_uv_reg(p_set_info->puiWtC);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
VOID ime_set_cst_enable(IME_CST_INFO *SetCstEn)
{
	ime_set_rgb2ycc_enable_reg((UINT32)SetCstEn->Rgb2YccEnable);
	ime_set_ycc2rgb_enable_reg((UINT32)SetCstEn->Ycc2RgbEnable);
}
#endif
//-------------------------------------------------------------------------------

#if 0
VOID ime_set_ssr_enable(IME_FUNC_EN set_en)
{
	ime_set_ssr_enable_reg((UINT32)set_en);
}
//-------------------------------------------------------------------------------

IME_FUNC_EN ime_get_ssr_enable(VOID)
{
	IME_FUNC_EN GetEn;

	GetEn = ime_get_ssr_enable_reg();

	return GetEn;
}
//-------------------------------------------------------------------------------

ER ime_set_ssr_iq_info(IME_SSR_IQ_INFO *p_set_info)
{
	if (p_set_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_info->uiDTh > 255) || (p_set_info->uiHVTh > 255)) {
		DBG_ERR("IME: SSR threshold parameter overflow...\r\n");
	}

	ime_set_ssr_threshold_reg(p_set_info->uiDTh, p_set_info->uiHVTh);

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_set_scale_factor_with_ssr(VOID)
{
	ER er_return = E_OK;
	IME_FUNC_EN GetEn = IME_FUNC_DISABLE;
	IME_PATH_SEL MaxSizeCh = IME_PATH1_SEL;
	UINT32 scl_factor_h = 0, scl_factor_v = 0;

	ime_tmp_input_size = 0;
	ime_tmp_output_size_path1 = 0;
	ime_tmp_output_size_path2 = 0;
	ime_tmp_output_size_path3 = 0;
	ime_tmp_output_size_path4 = 0;
	ime_tmp_output_size_max = 0;

	ime_get_in_path_image_size(&ime_input_size);
	ime_tmp_input_size = ime_input_size.size_h * ime_input_size.size_v;

	ime_tmp_output_size_max = 0;
	if (g_SsrMode == IME_SSR_MODE_AUTO) {
		ime_get_out_path_enable_p1(&GetEn);
		GetEn = ime_get_output_path_enable_status_p1_reg();
		if (GetEn == IME_FUNC_ENABLE) {
			//ime_get_out_path_scale_size_p1(&ime_output_size_path1);
			ime_tmp_output_size_path1 = ime_output_size_path1.size_h * ime_output_size_path1.size_v;
			ime_get_scaling_size_p1_reg(&(ime_output_size_path1.size_h), &(ime_output_size_path1.size_v));

			if (ime_tmp_output_size_path1 >= ime_tmp_output_size_max) {
				ime_tmp_output_size_max = ime_tmp_output_size_path1;
				MaxSizeCh = IME_PATH1_SEL;
			}
		}

		ime_get_out_path_enable_p2(&GetEn);
		if (GetEn == IME_FUNC_ENABLE) {
			ime_get_out_path_scale_size_p2(&ime_output_size_path2);
			ime_tmp_output_size_path2 = ime_output_size_path2.size_h * ime_output_size_path2.size_v;

			if (ime_tmp_output_size_path2 >= ime_tmp_output_size_max) {
				ime_tmp_output_size_max = ime_tmp_output_size_path2;
				MaxSizeCh = IME_PATH2_SEL;
			}
		}

		ime_get_out_path_enable_p3(&GetEn);
		if (GetEn == IME_FUNC_ENABLE) {
			ime_get_out_path_scale_size_p3(&ime_output_size_path3);
			ime_tmp_output_size_path3 = ime_output_size_path3.size_h * ime_output_size_path3.size_v;

			if (ime_tmp_output_size_path3 >= ime_tmp_output_size_max) {
				ime_tmp_output_size_max = ime_tmp_output_size_path3;
				MaxSizeCh = IME_PATH3_SEL;
			}
		}

		ime_get_out_path_enable_p4(&GetEn);
		if (GetEn == IME_FUNC_ENABLE) {
			ime_get_out_path_scale_size_p4(&ime_output_size_path4);
			ime_tmp_output_size_path4 = ime_output_size_path4.size_h * ime_output_size_path4.size_v;

			if (ime_tmp_output_size_path4 >= ime_tmp_output_size_max) {
				ime_tmp_output_size_max = ime_tmp_output_size_path4;
				MaxSizeCh = IME_PATH4_SEL;
			}
		}

		ime_set_ssr_enable(IME_FUNC_DISABLE);
		if (MaxSizeCh == IME_PATH1_SEL) {
			ime_get_out_path_enable_p1(&GetEn);
			GetEn = ime_get_output_path_enable_status_p1_reg();
			if (GetEn == IME_FUNC_ENABLE) {
				scl_factor_h = (ime_output_size_path1.size_h / ime_input_size.size_h);
				scl_factor_v = (ime_output_size_path1.size_v / ime_input_size.size_v);
				if ((scl_factor_h >= 2) && (scl_factor_v >= 2)) {
					ime_set_ssr_enable(IME_FUNC_ENABLE);
				}
			}
		} else if (MaxSizeCh == IME_PATH2_SEL) {
			ime_get_out_path_enable_p2(&GetEn);
			if (GetEn == IME_FUNC_ENABLE) {
				scl_factor_h = (ime_output_size_path2.size_h / ime_input_size.size_h);
				scl_factor_v = (ime_output_size_path2.size_v / ime_input_size.size_v);
				if ((scl_factor_h >= 2) && (scl_factor_v >= 2)) {
					ime_set_ssr_enable(IME_FUNC_ENABLE);
				}
			}
		} else if (MaxSizeCh == IME_PATH3_SEL) {
			ime_get_out_path_enable_p3(&GetEn);
			if (GetEn == IME_FUNC_ENABLE) {
				scl_factor_h = (ime_output_size_path3.size_h / ime_input_size.size_h);
				scl_factor_v = (ime_output_size_path3.size_v / ime_input_size.size_v);
				if ((scl_factor_h >= 2) && (scl_factor_v >= 2)) {
					ime_set_ssr_enable(IME_FUNC_ENABLE);
				}
			}
		} else if (MaxSizeCh == IME_PATH4_SEL) {
			ime_get_out_path_enable_p4(&GetEn);
			if (GetEn == IME_FUNC_ENABLE) {
				scl_factor_h = (ime_output_size_path4.size_h / ime_input_size.size_h);
				scl_factor_v = (ime_output_size_path4.size_v / ime_input_size.size_v);
				if ((scl_factor_h >= 2) && (scl_factor_v >= 2)) {
					ime_set_ssr_enable(IME_FUNC_ENABLE);
				}
			}
		}

#if 0
		else if (MaxSizeCh == IME_PATH5_SEL) {
			ime_get_out_path_enable_p5(&GetEn);
			if (GetEn == IME_FUNC_ENABLE) {
				scl_factor_h = (ime_output_size_path5.size_h / ime_input_size.size_h);
				scl_factor_v = (ime_output_size_path5.size_v / ime_input_size.size_v);
				if ((scl_factor_h >= 2) && (scl_factor_v >= 2)) {
					ime_set_ssr_enable(IME_FUNC_ENABLE);
				}
			}
		}
#endif
	}

	GetEn = ime_get_ssr_enable();
	if (GetEn == IME_FUNC_ENABLE) {
		ime_input_size.size_h = (ime_input_size.size_h * 2) - 1;
		ime_input_size.size_v = (ime_input_size.size_v * 2) - 1;
	}

	//ime_get_out_path_enable_p1(&GetEn);
	GetEn = ime_get_output_path_enable_status_p1_reg();
	if (GetEn == IME_FUNC_ENABLE) {
		//ime_get_out_path_scale_size_p1(&ime_output_size);
		//ime_get_out_path_scale_method_p1(&ime_output_scale_method);
		ime_output_scale_method = ime_get_scaling_method_p1_reg();

		ime_get_scaling_size_p1_reg(&(ime_output_size.size_h), &(ime_output_size.size_v));
		er_return = ime_cal_image_scale_params(&ime_input_size, &ime_output_size, ime_output_scale_method, &ime_scale_factor_path1_params);
		if (er_return != E_OK) {
			return er_return;
		}

		//er_return = ime_set_out_path_scale_factor_p1(&ime_scale_factor_path1_params);
		//if (er_return != E_OK) {
		//  return er_return;
		//}
		ime_set_scale_factor_h_p1_reg((UINT32)ime_scale_factor_path1_params.scale_h_ud, ime_scale_factor_path1_params.scale_h_dr, ime_scale_factor_path1_params.scale_h_ftr);
		ime_set_scale_factor_v_p1_reg((UINT32)ime_scale_factor_path1_params.scale_v_ud, ime_scale_factor_path1_params.scale_v_dr, ime_scale_factor_path1_params.scale_v_ftr);
		ime_set_integration_scaling_factor_base_p1_reg(ime_scale_factor_path1_params.isd_scale_h_base_ftr, ime_scale_factor_path1_params.isd_scale_v_base_ftr);

		ime_set_integration_scaling_factor0_p1_reg(ime_scale_factor_path1_params.isd_scale_h_ftr[0], ime_scale_factor_path1_params.isd_scale_v_ftr[0]);
		ime_set_integration_scaling_factor1_p1_reg(ime_scale_factor_path1_params.isd_scale_h_ftr[1], ime_scale_factor_path1_params.isd_scale_v_ftr[1]);
		ime_set_integration_scaling_factor2_p1_reg(ime_scale_factor_path1_params.isd_scale_h_ftr[2], ime_scale_factor_path1_params.isd_scale_v_ftr[2]);
	}

	ime_get_out_path_enable_p2(&GetEn);
	if (GetEn == IME_FUNC_ENABLE) {
		ime_get_out_path_scale_size_p2(&ime_output_size);
		ime_get_out_path_scale_method_p2(&ime_output_scale_method);
		er_return = ime_cal_image_scale_params(&ime_input_size, &ime_output_size, ime_output_scale_method, &ime_scale_factor_path2_params);
		if (er_return != E_OK) {
			return er_return;
		}

		//er_return = ime_set_out_path_scale_factor_p2(&ime_scale_factor_path2_params);
		//if (er_return != E_OK) {
		//  return er_return;
		//}
		ime_set_scale_factor_h_p2_reg((UINT32)ime_scale_factor_path2_params.scale_h_ud, ime_scale_factor_path2_params.scale_h_dr, ime_scale_factor_path2_params.scale_h_ftr);
		ime_set_scale_factor_v_p2_reg((UINT32)ime_scale_factor_path2_params.scale_v_ud, ime_scale_factor_path2_params.scale_v_dr, ime_scale_factor_path2_params.scale_v_ftr);
	}

	ime_get_out_path_enable_p3(&GetEn);
	if (GetEn == IME_FUNC_ENABLE) {
		ime_get_out_path_scale_size_p3(&ime_output_size);
		ime_get_out_path_scale_method_p3(&ime_output_scale_method);
		er_return = ime_cal_image_scale_params(&ime_input_size, &ime_output_size, ime_output_scale_method, &ime_scale_factor_path3_params);
		if (er_return != E_OK) {
			return er_return;
		}

		//er_return = ime_set_out_path_scale_factor_p3(&ime_scale_factor_path3_params);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		ime_set_scale_factor_h_p3_reg((UINT32)ime_scale_factor_path3_params.scale_h_ud, ime_scale_factor_path3_params.scale_h_dr, ime_scale_factor_path3_params.scale_h_ftr);
		ime_set_scale_factor_v_p3_reg((UINT32)ime_scale_factor_path3_params.scale_v_ud, ime_scale_factor_path3_params.scale_v_dr, ime_scale_factor_path3_params.scale_v_ftr);
	}

	ime_get_out_path_enable_p4(&GetEn);
	if (GetEn == IME_FUNC_ENABLE) {
		ime_get_out_path_scale_size_p4(&ime_output_size);
		ime_get_out_path_scale_method_p4(&ime_output_scale_method);
		er_return = ime_cal_image_scale_params(&ime_input_size, &ime_output_size, ime_output_scale_method, &ime_scale_factor_path4_params);
		if (er_return != E_OK) {
			return er_return;
		}

		er_return = ime_set_out_path_scale_factor_p4(&ime_scale_factor_path4_params);
		if (er_return != E_OK) {
			return er_return;
		}
	}

	return E_OK;
}
//-------------------------------------------------------------------------------
#endif

#if 0  // 520 do not support
VOID ime_set_gn_enable(IME_FUNC_EN set_en)
{
	ime_set_film_grain_noise_enable_reg((UINT32)set_en);
}
//-------------------------------------------------------------------------------

ER ime_set_gn_iq_info(IME_FILM_GRAIN_IQ_INFO *p_set_info)
{
	if (p_set_info == NULL) {
		DBG_ERR("IME: grain noise, parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_info->uiFgnNLP1 > 8) || (p_set_info->uiFgnNLP2 > 8) || (p_set_info->uiFgnNLP3 > 8)) { // || (p_set_info->uiFgnNLP5 > 8)
		DBG_ERR("IME: grain noise, parameter overflow...\r\n");

		return E_PAR;
	}

	ime_set_film_grain_noise_threshold_reg(p_set_info->uiFgnLumTh);

	ime_set_film_grain_noise_param_p1_reg(p_set_info->uiFgnInitP1, p_set_info->uiFgnNLP1);
	ime_set_film_grain_noise_param_p2_reg(p_set_info->uiFgnInitP2, p_set_info->uiFgnNLP2);
	ime_set_film_grain_noise_param_p3_reg(p_set_info->uiFgnInitP3, p_set_info->uiFgnNLP3);
	//ime_set_film_grain_noise_param_p5_reg(p_set_info->uiFgnInitP5, p_set_info->uiFgnNLP5);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------
#if 0
VOID ime_set_p2i_enable(IME_FUNC_EN set_en)
{
	ime_set_p2ifilter_enable_reg((UINT32)set_en);
}
#endif
//-------------------------------------------------------------------------------

//VOID ime_set_ds_cst_enable(IME_FUNC_EN set_en)
//{
//	ime_set_osd_cst_enable_reg((UINT32)set_en);
//}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_osd_set0_image_info(IME_STAMP_IMAGE_INFO *p_set_info)
{
	ER er_return = E_OK;
	UINT32 set_addr = 0x0, data_size = 0x0;
	//IME_INSRC_SEL GetInSrc;
	//IME_OPMODE OpMode;


	if (p_set_info == NULL) {
		DBG_ERR("IME: OSD0, setting parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_info->ds_img_size.size_h > 8192) || (p_set_info->ds_img_size.size_v > 8192)) {
		DBG_ERR("IME: OSD0 input size parameter overflow...\r\n");

		return E_PAR;
	}

	//if(p_set_info->ds_img_size.size_v > 128)
	//{
	//    DBG_WRN("IME: V size is larger than 128, bandwidth may be not enought...\r\n");
	//}

	er_return  = ime_check_lineoffset(p_set_info->ds_lofs, p_set_info->ds_lofs);
	if (er_return != E_OK) {

		DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)p_set_info->ds_lofs);
		DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)p_set_info->ds_lofs);

		return er_return;
	}

	//GetInSrc = ime_get_engine_info(IME_GET_INPUT_SRC);
	//OpMode = ime_get_operate_mode();
	data_size = p_set_info->ds_lofs * p_set_info->ds_img_size.size_v;
	set_addr = ime_input_dma_buf_flush(p_set_info->ds_addr, data_size, ime_engine_operation_mode);
	//er_return = ime_check_dma_addr(set_addr, set_addr, set_addr, IME_DMA_INPUT);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	ime_set_osd_set0_image_size_reg(p_set_info->ds_img_size.size_h, p_set_info->ds_img_size.size_v);
	ime_set_osd_set0_format_reg((UINT32)p_set_info->ds_fmt);
	ime_set_osd_set0_position_reg(p_set_info->ds_pos.pos_x, p_set_info->ds_pos.pos_y);
	ime_set_osd_set0_lineoffset_reg(p_set_info->ds_lofs);
	ime_set_osd_set0_dma_addr_reg(set_addr);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_osd_set0_iq_info(IME_STAMP_IQ_INFO *p_set_info)
{
	if (p_set_info == NULL) {
		DBG_ERR("IME: OSD0, setting parameter NULL...\r\n");

		return E_PAR;
	}

	ime_set_osd_set0_blend_weight_reg(p_set_info->ds_bld_wet0, p_set_info->ds_bld_wet1);
	ime_set_osd_set0_color_key_enable_reg((UINT32)p_set_info->ds_ck_enable);

	ime_set_osd_set0_color_palette_enable_reg((UINT32)p_set_info->ds_plt_enable);
	ime_set_osd_set0_color_key_mode_reg((UINT32)p_set_info->ds_ck_mode);

	ime_set_osd_set0_color_key_reg(p_set_info->ds_ck_a, p_set_info->ds_ck_r, p_set_info->ds_ck_g, p_set_info->ds_ck_b);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

VOID ime_set_osd_set0_burst_size(IME_BURST_SEL bst)
{
	// reserved
	UINT32 set_bst = 0;

	if (bst == IME_BURST_32W) {
		set_bst = 0;
	} else if (bst == IME_BURST_16W) {
		set_bst = 1;
	} else {
		// do nothing...
	}

	ime_set_input_path_osd_set0_burst_size_reg(set_bst);
}
//-------------------------------------------------------------------------------

//IME_FUNC_EN ime_get_osd_set0_enable(VOID)
//{
//	return (IME_FUNC_EN)ime_get_osd_set0_enable_reg();
//}
//-------------------------------------------------------------------------------


//VOID ime_get_osd_set0_image_size(IME_SIZE_INFO *p_get_info)
//{
//	ime_get_osd_set0_image_size_reg(&(p_get_info->size_h), &(p_get_info->size_v));
//}
//-------------------------------------------------------------------------------

//VOID ime_get_osd_set0_lineoffset(IME_LINEOFFSET_INFO *p_get_info)
//{
//	ime_get_osd_set0_lineoffset_reg(&(p_get_info->lineoffset_y));
//
//	p_get_info->lineoffset_cb = p_get_info->lineoffset_y;
//}
//-------------------------------------------------------------------------------

//VOID ime_get_osd_set0_dma_addr(IME_DMA_ADDR_INFO *p_get_info)
//{
//	ime_get_osd_set0_dma_addr_reg(&(p_get_info->addr_y));
//}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_osd_set1_image_info(IME_STAMP_IMAGE_INFO *p_set_info)
{
	ER er_return = E_OK;
	UINT32 set_addr = 0x0, data_size = 0x0;
	//IME_INSRC_SEL GetInSrc;
	//IME_OPMODE OpMode;


	if (p_set_info == NULL) {
		DBG_ERR("IME: OSD1, setting parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_info->ds_img_size.size_h > 8192) || (p_set_info->ds_img_size.size_v > 8192)) {
		DBG_ERR("IME: OSD1, input size parameter overflow...\r\n");

		return E_PAR;
	}

	//if(p_set_info->ds_img_size.size_v > 128)
	//{
	//    DBG_WRN("IME: V size is larger than 128, bandwidth may be not enought...\r\n");
	//}

	er_return  = ime_check_lineoffset(p_set_info->ds_lofs, p_set_info->ds_lofs);
	if (er_return != E_OK) {

		DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)p_set_info->ds_lofs);
		DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)p_set_info->ds_lofs);

		return er_return;
	}

	//GetInSrc = ime_get_engine_info(IME_GET_INPUT_SRC);
	//OpMode = ime_get_operate_mode();
	data_size = p_set_info->ds_lofs * p_set_info->ds_img_size.size_v;
	set_addr = ime_input_dma_buf_flush(p_set_info->ds_addr, data_size, ime_engine_operation_mode);
	//er_return = ime_check_dma_addr(set_addr, set_addr, set_addr, IME_DMA_INPUT);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	ime_set_osd_set1_image_size_reg(p_set_info->ds_img_size.size_h, p_set_info->ds_img_size.size_v);
	ime_set_osd_set1_format_reg((UINT32)p_set_info->ds_fmt);
	ime_set_osd_set1_position_reg(p_set_info->ds_pos.pos_x, p_set_info->ds_pos.pos_y);
	ime_set_osd_set1_lineoffset_reg(p_set_info->ds_lofs);
	ime_set_osd_set1_dma_addr_reg(set_addr);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_osd_set1_iq_info(IME_STAMP_IQ_INFO *p_set_info)
{
	if (p_set_info == NULL) {
		DBG_ERR("IME: OSD1, parameter NULL...\r\n");

		return E_PAR;
	}

	ime_set_osd_set1_blend_weight_reg(p_set_info->ds_bld_wet0, p_set_info->ds_bld_wet1);
	ime_set_osd_set1_color_key_enable_reg((UINT32)p_set_info->ds_ck_enable);

	ime_set_osd_set1_color_palette_enable_reg((UINT32)p_set_info->ds_plt_enable);
	ime_set_osd_set1_color_key_mode_reg((UINT32)p_set_info->ds_ck_mode);

	ime_set_osd_set1_color_key_reg(p_set_info->ds_ck_a, p_set_info->ds_ck_r, p_set_info->ds_ck_g, p_set_info->ds_ck_b);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

//VOID ime_get_osd_set1_image_size(IME_SIZE_INFO *p_get_info)
//{
//	ime_get_osd_set1_image_size_reg(&(p_get_info->size_h), &(p_get_info->size_v));
//}
//-------------------------------------------------------------------------------

//VOID ime_get_osd_set1_lineoffset(IME_LINEOFFSET_INFO *p_get_info)
//{
//	ime_get_osd_set1_lineoffset_reg(&(p_get_info->lineoffset_y));
//
//	p_get_info->lineoffset_cb = p_get_info->lineoffset_y;
//}
//-------------------------------------------------------------------------------

//VOID ime_get_osd_set1_dma_addr(IME_DMA_ADDR_INFO *p_get_info)
//{
//	ime_get_osd_set1_dma_addr_reg(&(p_get_info->addr_y));
//}
//-------------------------------------------------------------------------------

//IME_FUNC_EN ime_get_osd_set1_enable(VOID)
//{
//	return (IME_FUNC_EN)ime_get_osd_set1_enable_reg();
//}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_osd_set2_image_info(IME_STAMP_IMAGE_INFO *p_set_info)
{
	ER er_return = E_OK;
	UINT32 SetAddr = 0x0, DataSize = 0x0;
	//IME_INSRC_SEL GetInSrc;
	//IME_OPMODE OpMode;


	if (p_set_info == NULL) {
		DBG_ERR("IME: OSD2, setting parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_info->ds_img_size.size_h > 8192) || (p_set_info->ds_img_size.size_v > 8192)) {
		DBG_ERR("IME: OSD2, input size parameter overflow...\r\n");

		return E_PAR;
	}

	//if(p_set_info->ds_img_size.size_v > 128)
	//{
	//    DBG_WRN("IME: V size is larger than 128, bandwidth may be not enought...\r\n");
	//}

	er_return  = ime_check_lineoffset(p_set_info->ds_lofs, p_set_info->ds_lofs);
	if (er_return != E_OK) {

		DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)p_set_info->ds_lofs);
		DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)p_set_info->ds_lofs);

		return er_return;
	}

	//GetInSrc = ime_get_engine_info(IME_GET_INPUT_SRC);
	//OpMode = ime_get_operate_mode();
	DataSize = p_set_info->ds_lofs * p_set_info->ds_img_size.size_v;
	SetAddr = ime_input_dma_buf_flush(p_set_info->ds_addr, DataSize, ime_engine_operation_mode);
	//er_return = ime_check_dma_addr(SetAddr, SetAddr, SetAddr, IME_DMA_INPUT);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	ime_set_osd_set2_image_size_reg(p_set_info->ds_img_size.size_h, p_set_info->ds_img_size.size_v);
	ime_set_osd_set2_format_reg((UINT32)p_set_info->ds_fmt);
	ime_set_osd_set2_position_reg(p_set_info->ds_pos.pos_x, p_set_info->ds_pos.pos_y);
	ime_set_osd_set2_lineoffset_reg(p_set_info->ds_lofs);
	ime_set_osd_set2_dma_addr_reg(SetAddr);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_osd_set2_iq_info(IME_STAMP_IQ_INFO *p_set_info)
{
	if (p_set_info == NULL) {
		DBG_ERR("IME: OSD2, setting parameter NULL...\r\n");

		return E_PAR;
	}

	ime_set_osd_set2_blend_weight_reg(p_set_info->ds_bld_wet0, p_set_info->ds_bld_wet1);
	ime_set_osd_set2_color_key_enable_reg((UINT32)p_set_info->ds_ck_enable);

	ime_set_osd_set2_color_palette_enable_reg((UINT32)p_set_info->ds_plt_enable);
	ime_set_osd_set2_color_key_mode_reg((UINT32)p_set_info->ds_ck_mode);

	ime_set_osd_set2_color_key_reg(p_set_info->ds_ck_a, p_set_info->ds_ck_r, p_set_info->ds_ck_g, p_set_info->ds_ck_b);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

//VOID ime_get_osd_set2_image_size(IME_SIZE_INFO *p_get_info)
//{
//	ime_get_osd_set2_image_size_reg(&(p_get_info->size_h), &(p_get_info->size_v));
//}
//-------------------------------------------------------------------------------

//VOID ime_get_osd_set2_lineoffset(IME_LINEOFFSET_INFO *p_get_info)
//{
//	ime_get_osd_set2_lineoffset_reg(&(p_get_info->lineoffset_y));
//
//	p_get_info->lineoffset_cb = p_get_info->lineoffset_y;
//}
//-------------------------------------------------------------------------------

//VOID ime_get_osd_set2_dma_addr(IME_DMA_ADDR_INFO *p_get_info)
//{
//	ime_get_osd_set2_dma_addr_reg(&(p_get_info->addr_y));
//}
//-------------------------------------------------------------------------------

//IME_FUNC_EN ime_get_osd_set2_enable(VOID)
//{
//	return (IME_FUNC_EN)ime_get_osd_set2_enable_reg();
//}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_osd_set3_image_info(IME_STAMP_IMAGE_INFO *p_set_info)
{
	ER er_return = E_OK;
	UINT32 set_addr = 0x0, data_size = 0x0;
	//IME_INSRC_SEL GetInSrc;
	//IME_OPMODE OpMode;


	if (p_set_info == NULL) {
		DBG_ERR("IME: OSD3, setting parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_info->ds_img_size.size_h > 8192) || (p_set_info->ds_img_size.size_v > 8192)) {
		DBG_ERR("IME: OSD3, input size parameter overflow...\r\n");

		return E_PAR;
	}

	//if(p_set_info->ds_img_size.size_v > 128)
	//{
	//    DBG_WRN("IME: V size is larger than 128, bandwidth may be not enought...\r\n");
	//}

	er_return  = ime_check_lineoffset(p_set_info->ds_lofs, p_set_info->ds_lofs);
	if (er_return != E_OK) {

		DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)p_set_info->ds_lofs);
		DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)p_set_info->ds_lofs);

		return er_return;
	}

	//GetInSrc = ime_get_engine_info(IME_GET_INPUT_SRC);
	//OpMode = ime_get_operate_mode();
	data_size = p_set_info->ds_lofs * p_set_info->ds_img_size.size_v;
	set_addr = ime_input_dma_buf_flush(p_set_info->ds_addr, data_size, ime_engine_operation_mode);
	//er_return = ime_check_dma_addr(set_addr, set_addr, set_addr, IME_DMA_INPUT);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	ime_set_osd_set3_image_size_reg(p_set_info->ds_img_size.size_h, p_set_info->ds_img_size.size_v);
	ime_set_osd_set3_format_reg((UINT32)p_set_info->ds_fmt);
	ime_set_osd_set3_position_reg(p_set_info->ds_pos.pos_x, p_set_info->ds_pos.pos_y);
	ime_set_osd_set3_lineoffset_reg(p_set_info->ds_lofs);
	ime_set_osd_set3_dma_addr_reg(set_addr);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_osd_set3_iq_info(IME_STAMP_IQ_INFO *p_set_info)
{
	if (p_set_info == NULL) {
		DBG_ERR("IME: OSD3, setting parameter NULL...\r\n");

		return E_PAR;
	}

	ime_set_osd_set3_blend_weight_reg(p_set_info->ds_bld_wet0, p_set_info->ds_bld_wet1);
	ime_set_osd_set3_color_key_enable_reg((UINT32)p_set_info->ds_ck_enable);

	ime_set_osd_set3_color_palette_enable_reg((UINT32)p_set_info->ds_plt_enable);
	ime_set_osd_set3_color_key_mode_reg((UINT32)p_set_info->ds_ck_mode);

	ime_set_osd_set3_color_key_reg(p_set_info->ds_ck_a, p_set_info->ds_ck_r, p_set_info->ds_ck_g, p_set_info->ds_ck_b);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

//VOID ime_get_osd_set3_image_size(IME_SIZE_INFO *p_get_info)
//{
//	ime_get_osd_set3_image_size_reg(&(p_get_info->size_h), &(p_get_info->size_v));
//}
//-------------------------------------------------------------------------------

//VOID ime_get_osd_set3_lineoffset(IME_LINEOFFSET_INFO *p_get_info)
//{
//	ime_get_osd_set3_lineoffset_reg(&(p_get_info->lineoffset_y));
//
//	p_get_info->lineoffset_cb = p_get_info->lineoffset_y;
//}
//-------------------------------------------------------------------------------

//VOID ime_get_osd_set3_dma_addr(IME_DMA_ADDR_INFO *p_get_info)
//{
//	ime_get_osd_set3_dma_addr_reg(&(p_get_info->addr_y));
//}
//-------------------------------------------------------------------------------

//IME_FUNC_EN ime_get_osd_set3_enable(VOID)
//{
//	return (IME_FUNC_EN)ime_get_osd_set3_enable_reg();
//}
//-------------------------------------------------------------------------------


//ER ime_set_osd_cst_coef(UINT32 coef0, UINT32 coef1, UINT32 coef2, UINT32 coef3)
//{
//	ime_set_osd_cst_coef_reg(coef0, coef1, coef2, coef3);
//
//	return E_OK;
//}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_osd_color_lut_info(UINT32 *p_color_lut_y, UINT32 *p_color_lut_u, UINT32 *p_color_lut_v)
{
#if 0

	UINT32 i = 0;

	if ((p_color_lut_Y == NULL) || (p_color_lut_U == NULL) || (p_color_lut_V == NULL)) {
		DBG_ERR("IME: data stamp color LUT NULL...\r\n");

		return E_PAR;
	}

	for (i = 0; i < 16; i++) {
		if ((p_color_lut_Y[i] > 255) || (p_color_lut_U[i] > 255) || (p_color_lut_V[i] > 255)) {
			DBG_ERR("IME: data stamp color LUT overflow...\r\n");

			return E_PAR;
		}
	}
	//ime_set_osd_color_lut_y_reg(p_color_lut_Y);
	//ime_set_osd_color_lut_u_reg(p_color_lut_U);
	//ime_set_osd_color_lut_v_reg(p_color_lut_V);

#endif

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------


VOID ime_set_privacy_mask_enable(IME_PM_SET_SEL set_sel, IME_FUNC_EN set_en)
{
	// reserved
	UINT32 set_enable = 0x0;

	set_enable = (UINT32)set_en;

	switch (set_sel) {
	case IME_PM_SET0:
		ime_set_privacy_mask_set0_enable_reg(set_enable);
		break;

	case IME_PM_SET1:
		ime_set_privacy_mask_set1_enable_reg(set_enable);
		break;

	case IME_PM_SET2:
		ime_set_privacy_mask_set2_enable_reg(set_enable);
		break;

	case IME_PM_SET3:
		ime_set_privacy_mask_set3_enable_reg(set_enable);
		break;

	case IME_PM_SET4:
		ime_set_privacy_mask_set4_enable_reg(set_enable);
		break;

	case IME_PM_SET5:
		ime_set_privacy_mask_set5_enable_reg(set_enable);
		break;

	case IME_PM_SET6:
		ime_set_privacy_mask_set6_enable_reg(set_enable);
		break;

	case IME_PM_SET7:
		ime_set_privacy_mask_set7_enable_reg(set_enable);
		break;

	default:
		break;
	}
}
//-------------------------------------------------------------------------------

IME_FUNC_EN ime_get_privacy_mask_enable(IME_PM_SET_SEL set_sel)
{
	// reserved
	IME_FUNC_EN get_en;

	switch (set_sel) {
	case IME_PM_SET0:
		get_en = ime_get_privacy_mask_set0_enable_reg();
		break;

	case IME_PM_SET1:
		get_en = ime_get_privacy_mask_set1_enable_reg();
		break;

	case IME_PM_SET2:
		get_en = ime_get_privacy_mask_set2_enable_reg();
		break;

	case IME_PM_SET3:
		get_en = ime_get_privacy_mask_set3_enable_reg();
		break;

	case IME_PM_SET4:
		get_en = ime_get_privacy_mask_set4_enable_reg();
		break;

	case IME_PM_SET5:
		get_en = ime_get_privacy_mask_set5_enable_reg();
		break;

	case IME_PM_SET6:
		get_en = ime_get_privacy_mask_set6_enable_reg();
		break;

	case IME_PM_SET7:
		get_en = ime_get_privacy_mask_set7_enable_reg();
		break;

	default:
		get_en = IME_FUNC_DISABLE;
		break;
	}

	return get_en;
}
//-------------------------------------------------------------------------------

VOID ime_set_privacy_mask_type(IME_PM_SET_SEL set_sel, IME_PM_MASK_TYPE set_type)
{
	// reserved
	switch (set_sel) {
	case IME_PM_SET0:
		ime_set_privacy_mask_set0_type_reg(set_type);
		break;

	case IME_PM_SET1:
		ime_set_privacy_mask_set1_type_reg(set_type);
		break;

	case IME_PM_SET2:
		ime_set_privacy_mask_set2_type_reg(set_type);
		break;

	case IME_PM_SET3:
		ime_set_privacy_mask_set3_type_reg(set_type);
		break;

	case IME_PM_SET4:
		ime_set_privacy_mask_set4_type_reg(set_type);
		break;

	case IME_PM_SET5:
		ime_set_privacy_mask_set5_type_reg(set_type);
		break;

	case IME_PM_SET6:
		ime_set_privacy_mask_set6_hollow_type_reg(set_type);
		break;

	case IME_PM_SET7:
		ime_set_privacy_mask_set7_type_reg(set_type);
		break;

	default:
		break;
	}
}
//-------------------------------------------------------------------------------


VOID ime_set_privacy_mask_color(IME_PM_SET_SEL set_sel, IME_PM_MASK_COLOR set_color)
{
	// reserved
	UINT32 set_y = 0x0, set_u = 0x0, set_v = 0x0;

	set_y = set_color.pm_color_y;
	set_u = set_color.pm_color_u;
	set_v = set_color.pm_color_v;

	switch (set_sel) {
	case IME_PM_SET0:
		ime_set_privacy_mask_set0_color_yuv_reg(set_y, set_u, set_v);
		break;

	case IME_PM_SET1:
		ime_set_privacy_mask_set1_color_yuv_reg(set_y, set_u, set_v);
		break;

	case IME_PM_SET2:
		ime_set_privacy_mask_set2_color_yuv_reg(set_y, set_u, set_v);
		break;

	case IME_PM_SET3:
		ime_set_privacy_mask_set3_color_yuv_reg(set_y, set_u, set_v);
		break;

	case IME_PM_SET4:
		ime_set_privacy_mask_set4_color_yuv_reg(set_y, set_u, set_v);
		break;

	case IME_PM_SET5:
		ime_set_privacy_mask_set5_color_yuv_reg(set_y, set_u, set_v);
		break;

	case IME_PM_SET6:
		ime_set_privacy_mask_set6_color_yuv_reg(set_y, set_u, set_v);
		break;

	case IME_PM_SET7:
		ime_set_privacy_mask_set7_color_yuv_reg(set_y, set_u, set_v);
		break;

	default:
		break;
	}
}
//-------------------------------------------------------------------------------

VOID ime_set_privacy_mask_weight(IME_PM_SET_SEL set_sel, UINT32 set_wet)
{
	// reserved
	switch (set_sel) {
	case IME_PM_SET0:
		ime_set_privacy_mask_set0_weight_reg(set_wet);
		break;

	case IME_PM_SET1:
		ime_set_privacy_mask_set1_seight_reg(set_wet);
		break;

	case IME_PM_SET2:
		ime_set_privacy_mask_set2_weight_reg(set_wet);
		break;

	case IME_PM_SET3:
		ime_set_privacy_mask_set3_weight_reg(set_wet);
		break;

	case IME_PM_SET4:
		ime_set_privacy_mask_set4_weight_reg(set_wet);
		break;

	case IME_PM_SET5:
		ime_set_privacy_mask_set5_weight_reg(set_wet);
		break;

	case IME_PM_SET6:
		ime_set_privacy_mask_set6_weight_reg(set_wet);
		break;

	case IME_PM_SET7:
		ime_set_privacy_mask_set7_weight_reg(set_wet);
		break;

	default:
		break;
	}
}
//-------------------------------------------------------------------------------

VOID ime_set_privacy_mask_line0(IME_PM_SET_SEL set_sel, INT32 *p_coefs)
{
	// reserved
	switch (set_sel) {
	case IME_PM_SET0:
		ime_set_privacy_mask_set0_line0_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET1:
		ime_set_privacy_mask_set1_line0_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET2:
		ime_set_privacy_mask_set2_line0_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET3:
		ime_set_privacy_mask_set3_line0_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET4:
		ime_set_privacy_mask_set4_line0_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET5:
		ime_set_privacy_mask_set5_line0_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET6:
		ime_set_privacy_mask_set6_line0_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET7:
		ime_set_privacy_mask_set7_line0_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	default:
		break;
	}
}
//-------------------------------------------------------------------------------

VOID ime_set_privacy_mask_line1(IME_PM_SET_SEL set_sel, INT32 *p_coefs)
{
	// reserved
	switch (set_sel) {
	case IME_PM_SET0:
		ime_set_privacy_mask_set0_line1_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET1:
		ime_set_privacy_mask_set1_line1_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET2:
		ime_set_privacy_mask_set2_line1_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET3:
		ime_set_privacy_mask_set3_line1_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET4:
		ime_set_privacy_mask_set4_line1_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET5:
		ime_set_privacy_mask_set5_line1_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET6:
		ime_set_privacy_mask_set6_line1_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET7:
		ime_set_privacy_mask_set7_line1_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	default:
		break;
	}
}
//-------------------------------------------------------------------------------

VOID ime_set_privacy_mask_line2(IME_PM_SET_SEL set_sel, INT32 *p_coefs)
{
	// reserved
	switch (set_sel) {
	case IME_PM_SET0:
		ime_set_privacy_mask_set0_line2_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET1:
		ime_set_privacy_mask_set1_line2_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET2:
		ime_set_privacy_mask_set2_line2_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET3:
		ime_set_privacy_mask_set3_line2_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET4:
		ime_set_privacy_mask_set4_line2_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET5:
		ime_set_privacy_mask_set5_line2_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET6:
		ime_set_privacy_mask_set6_line2_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET7:
		ime_set_privacy_mask_set7_line2_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	default:
		break;
	}
}
//-------------------------------------------------------------------------------


VOID ime_set_privacy_mask_line3(IME_PM_SET_SEL set_sel, INT32 *p_coefs)
{
	// reserved
	switch (set_sel) {
	case IME_PM_SET0:
		ime_set_privacy_mask_set0_line3_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET1:
		ime_set_privacy_mask_set1_line3_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET2:
		ime_set_privacy_mask_set2_line3_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET3:
		ime_set_privacy_mask_set3_line3_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET4:
		ime_set_privacy_mask_set4_line3_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET5:
		ime_set_privacy_mask_set5_line3_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET6:
		ime_set_privacy_mask_set6_line3_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	case IME_PM_SET7:
		ime_set_privacy_mask_set7_line3_reg(p_coefs[0], p_coefs[1], p_coefs[2], p_coefs[3]);
		break;

	default:
		break;
	}
}
//-------------------------------------------------------------------------------

//VOID ime_set_privacy_mask_pxl_image_size(UINT32 size_h, UINT32 size_v)
//{
//	ime_set_privacy_mask_pxl_image_size_reg(size_h, size_v);
//	ime_set_privacy_mask_pxl_image_format_reg(2);
//}
//-------------------------------------------------------------------------------

//VOID ime_set_privacy_mask_pxl_block_size(UINT32 blk_size)
//{
//	ime_set_privacy_mask_pxl_blk_size_reg(blk_size);
//}
//-------------------------------------------------------------------------------


//VOID ime_set_privacy_mask_pxl_image_lineoffset(UINT32 lofs)
//{
//	ime_set_privacy_mask_pxl_image_lineoffset_reg(lofs);
//}
//-------------------------------------------------------------------------------


//VOID ime_set_privacy_mask_pxl_image_dma_addr(UINT32 addr)
//{
//	ime_set_privacy_mask_pxl_image_dma_addr_reg(addr);
//}
//-------------------------------------------------------------------------------

VOID ime_set_privacy_mask_burst_size(IME_BURST_SEL bst)
{
	// reserved
	UINT32 set_bst = 0;

	if (bst == IME_BURST_32W) {
		set_bst = 0;
	} else if (bst == IME_BURST_16W) {
		set_bst = 1;
	} else {
		// do nothing...
	}

	ime_set_input_path_pixelation_burst_size_reg(set_bst);
}
//-------------------------------------------------------------------------------


//VOID ime_get_privacy_mask_pxl_image_size(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
//{
//	ime_get_privacy_mask_pxl_image_size_reg(p_get_size_h, p_get_size_v);
//}
//-------------------------------------------------------------------------------

//VOID ime_get_privacy_mask_pxl_image_lineoffset(UINT32 *p_get_lofs)
//{
//	ime_get_privacy_mask_pxl_image_lineoffset_reg(p_get_lofs);
//}
//-------------------------------------------------------------------------------

//VOID ime_get_privacy_mask_pxl_image_dma_addr(UINT32 *p_get_addr)
//{
//	ime_get_privacy_mask_pxl_image_dma_addr_reg(p_get_addr);
//}
//-------------------------------------------------------------------------------


// ADAS
//VOID ime_set_adas_enable(IME_FUNC_EN set_en)
//{
//	ime_set_adas_enable_reg(set_en);
//}
//-------------------------------------------------------------------------------

//IME_FUNC_EN ime_get_adas_enable(VOID)
//{
//	return ((IME_FUNC_EN)ime_get_adas_enable_reg());
//}
//-------------------------------------------------------------------------------

//VOID ime_set_adas_median_filter_enable(IME_FUNC_EN set_en)
//{
//	ime_set_adas_median_filter_enable_reg(set_en);
//}
//-------------------------------------------------------------------------------

//VOID ime_set_adas_image_out_sel(IME_STL_IMGOUT_SEL img_out_sel)
//{
//	ime_set_adas_after_filter_out_sel_reg((UINT32)img_out_sel);
//}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_adas_edge_kernel(IME_STL_EDGE_INFO *p_set_edge_ker_info)
{
	if (p_set_edge_ker_info == NULL) {
		DBG_ERR("IME: stl., edge kernal parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_edge_ker_info->stl_sft0 > 15) || (p_set_edge_ker_info->stl_sft1 > 15)) {
		DBG_ERR("IME: stl., edge kernal parameter overflow...\r\n");

		return E_PAR;
	}

	ime_set_adas_edge_kernel_enable_reg((UINT32)p_set_edge_ker_info->stl_edge_ker0_enable, (UINT32)p_set_edge_ker_info->stl_edge_ker1_enable);
	ime_set_adas_edge_kernel_sel_reg((UINT32)p_set_edge_ker_info->stl_edge_ker0, (UINT32)p_set_edge_ker_info->stl_edge_ker1);
	ime_set_adas_edge_kernel_shift_reg(p_set_edge_ker_info->stl_sft0, p_set_edge_ker_info->stl_sft1);

	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_adas_histogram(IME_STL_HIST_INFO *p_set_hist_info)
{
	if (p_set_hist_info == NULL) {
		DBG_ERR("IME: stl., histogram parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_hist_info->stl_hist_pos.pos_x > 65535) || (p_set_hist_info->stl_hist_pos.pos_y > 65535)) {
		DBG_ERR("IME: stl., histogram position parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_hist_info->stl_hist_size.size_h > 65535) || (p_set_hist_info->stl_hist_size.size_v > 65535)) {
		DBG_ERR("IME: stl., histogram size parameter overflow...\r\n");

		return E_PAR;
	}

	if (p_set_hist_info->stl_set_sel == IME_STL_HIST_SET0) {
		ime_set_adas_histogram_set0_reg(p_set_hist_info->stl_hist_pos.pos_x, p_set_hist_info->stl_hist_pos.pos_y, p_set_hist_info->stl_hist_size.size_h, p_set_hist_info->stl_hist_size.size_v);
		ime_set_adas_histogram_acc_target_set0_reg(p_set_hist_info->stl_hist_acc_tag.acc_tag);
	} else if (p_set_hist_info->stl_set_sel == IME_STL_HIST_SET1) {
		ime_set_adas_histogram_set1_reg(p_set_hist_info->stl_hist_pos.pos_x, p_set_hist_info->stl_hist_pos.pos_y, p_set_hist_info->stl_hist_size.size_h, p_set_hist_info->stl_hist_size.size_v);
		ime_set_adas_histogram_acc_target_set1_reg(p_set_hist_info->stl_hist_acc_tag.acc_tag);
	}

	return E_OK;

}
#endif
//-------------------------------------------------------------------------------

//VOID ime_set_adas_edge_map_lofs(UINT32 lofs)
//{
//	ime_set_adas_edge_map_dam_lineoffset_reg(lofs);
//}
//-------------------------------------------------------------------------------

//UINT32 ime_get_adas_edge_map_lofs(VOID)
//{
//	return ime_get_adas_edge_map_dam_lineoffset_reg();
//}

//-------------------------------------------------------------------------------

#if 0
ER ime_set_adas_edge_map_addr(UINT32 addr)
{
	//ER er_return = E_OK;
	UINT32 tmp_addr = 0x0;

	if (addr == 0x0) {
		DBG_ERR("IME: stl., edge map DMA address parameter NULL...\r\n");

		return E_PAR;
	}

	tmp_addr = addr;

	//er_return = ime_check_dma_addr(tmp_addr, tmp_addr, tmp_addr, IME_DMA_OUTPUT);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	ime_set_adas_edge_map_dam_addr_reg(tmp_addr);

	return E_OK;

}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_adas_histogram_addr(UINT32 addr)
{
	//ER er_return = E_OK;
	UINT32 tmp_addr = 0x0;

	if (addr == 0x0) {
		DBG_ERR("IME: stl., histogram DMA address = 0x0...\r\n");

		return E_PAR;
	}

	tmp_addr = addr;

	//er_return = ime_check_dma_addr(tmp_addr, tmp_addr, tmp_addr, IME_DMA_OUTPUT);
	//if (er_return != E_OK) {
	//  return er_return;
	//}

	ime_set_adas_histogram_dam_addr_reg(tmp_addr);

	return E_OK;

}
#endif
//-------------------------------------------------------------------------------

#if 0
ER ime_set_adas_roi(IME_STL_ROI_INFO *p_set_roi_info)
{
	if (p_set_roi_info == NULL) {
		DBG_ERR("IME: stl., ROI parameter NULL...\r\n");

		return E_PAR;
	}

	if ((p_set_roi_info->stl_roi0.stl_roi_th0 > 2047) || (p_set_roi_info->stl_roi0.stl_roi_th1 > 2047) || (p_set_roi_info->stl_roi0.stl_roi_th2 > 2047)) {
		DBG_ERR("IME: stl., ROI0 threshold parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_roi_info->stl_roi1.stl_roi_th0 > 2047) || (p_set_roi_info->stl_roi1.stl_roi_th1 > 2047) || (p_set_roi_info->stl_roi1.stl_roi_th2 > 2047)) {
		DBG_ERR("IME: stl., ROI1 threshold parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_roi_info->stl_roi2.stl_roi_th0 > 2047) || (p_set_roi_info->stl_roi2.stl_roi_th1 > 2047) || (p_set_roi_info->stl_roi2.stl_roi_th2 > 2047)) {
		DBG_ERR("IME: stl., ROI2 threshold parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_roi_info->stl_roi3.stl_roi_th0 > 2047) || (p_set_roi_info->stl_roi3.stl_roi_th1 > 2047) || (p_set_roi_info->stl_roi3.stl_roi_th2 > 2047)) {
		DBG_ERR("IME: stl., ROI3 threshold parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_roi_info->stl_roi4.stl_roi_th0 > 2047) || (p_set_roi_info->stl_roi4.stl_roi_th1 > 2047) || (p_set_roi_info->stl_roi4.stl_roi_th2 > 2047)) {
		DBG_ERR("IME: stl., ROI4 threshold parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_roi_info->stl_roi5.stl_roi_th0 > 2047) || (p_set_roi_info->stl_roi5.stl_roi_th1 > 2047) || (p_set_roi_info->stl_roi5.stl_roi_th2 > 2047)) {
		DBG_ERR("IME: stl., ROI5 threshold parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_roi_info->stl_roi6.stl_roi_th0 > 2047) || (p_set_roi_info->stl_roi6.stl_roi_th1 > 2047) || (p_set_roi_info->stl_roi6.stl_roi_th2 > 2047)) {
		DBG_ERR("IME: stl., ROI6 threshold parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_roi_info->stl_roi7.stl_roi_th0 > 2047) || (p_set_roi_info->stl_roi7.stl_roi_th1 > 2047) || (p_set_roi_info->stl_roi7.stl_roi_th2 > 2047)) {
		DBG_ERR("IME: stl., ROI7 threshold parameter overflow...\r\n");

		return E_PAR;
	}

	if ((p_set_roi_info->stl_row_pos.stl_row0 > 1080) || (p_set_roi_info->stl_row_pos.stl_row1 > 1080)) {
		DBG_ERR("IME: stl., ROW position parameter overflow...\r\n");

		return E_PAR;
	}

	ime_set_adas_row_position_reg(p_set_roi_info->stl_row_pos.stl_row0, p_set_roi_info->stl_row_pos.stl_row1);

	ime_set_adas_roi_threshold0_reg(p_set_roi_info->stl_roi0.stl_roi_th0, p_set_roi_info->stl_roi0.stl_roi_th1, p_set_roi_info->stl_roi0.stl_roi_th2, p_set_roi_info->stl_roi0.stl_roi_src);
	ime_set_adas_roi_threshold1_reg(p_set_roi_info->stl_roi1.stl_roi_th0, p_set_roi_info->stl_roi1.stl_roi_th1, p_set_roi_info->stl_roi1.stl_roi_th2, p_set_roi_info->stl_roi1.stl_roi_src);
	ime_set_adas_roi_threshold2_reg(p_set_roi_info->stl_roi2.stl_roi_th0, p_set_roi_info->stl_roi2.stl_roi_th1, p_set_roi_info->stl_roi2.stl_roi_th2, p_set_roi_info->stl_roi2.stl_roi_src);
	ime_set_adas_roi_threshold3_reg(p_set_roi_info->stl_roi3.stl_roi_th0, p_set_roi_info->stl_roi3.stl_roi_th1, p_set_roi_info->stl_roi3.stl_roi_th2, p_set_roi_info->stl_roi3.stl_roi_src);
	ime_set_adas_roi_threshold4_reg(p_set_roi_info->stl_roi4.stl_roi_th0, p_set_roi_info->stl_roi4.stl_roi_th1, p_set_roi_info->stl_roi4.stl_roi_th2, p_set_roi_info->stl_roi4.stl_roi_src);
	ime_set_adas_roi_threshold5_reg(p_set_roi_info->stl_roi5.stl_roi_th0, p_set_roi_info->stl_roi5.stl_roi_th1, p_set_roi_info->stl_roi5.stl_roi_th2, p_set_roi_info->stl_roi5.stl_roi_src);
	ime_set_adas_roi_threshold6_reg(p_set_roi_info->stl_roi6.stl_roi_th0, p_set_roi_info->stl_roi6.stl_roi_th1, p_set_roi_info->stl_roi6.stl_roi_th2, p_set_roi_info->stl_roi6.stl_roi_src);
	ime_set_adas_roi_threshold7_reg(p_set_roi_info->stl_roi7.stl_roi_th0, p_set_roi_info->stl_roi7.stl_roi_th1, p_set_roi_info->stl_roi7.stl_roi_th2, p_set_roi_info->stl_roi7.stl_roi_src);


	return E_OK;
}
#endif
//-------------------------------------------------------------------------------

//VOID ime_set_adas_burst_length(VOID)
//{
//	ime_get_adas_burst_length_reg();
//}
//-------------------------------------------------------------------------------

#if 0
VOID ime_get_adas_hitogram_info(IME_GET_STL_INFO *p_get_hist_info)
{
	UINT32 get_a = 0, get_b = 0;

	ime_get_adas_max_edge_reg(&get_a, &get_b);
	p_get_hist_info->get_hist_max0 = get_a;
	p_get_hist_info->get_hist_max1 = get_b;

	ime_get_adas_histogram_bin_reg(&get_a, &get_b);
	p_get_hist_info->get_acc_tag_bin0 = get_a;
	p_get_hist_info->get_acc_tag_bin1 = get_b;
}
#endif
//-------------------------------------------------------------------------------

#if 0
VOID ime_get_adas_edge_map_addr(IME_DMA_ADDR_INFO *p_get_addr)
{
	ime_get_adas_edge_map_addr_reg(&(p_get_addr->addr_y));

	p_get_addr->addr_cb = p_get_addr->addr_cr = p_get_addr->addr_y;
}
#endif
//-------------------------------------------------------------------------------

//VOID ime_get_adas_histogram_addr(IME_DMA_ADDR_INFO *p_get_addr)
//{
//	ime_get_adas_histogram_addr_reg(&(p_get_addr->addr_y));
//
//	p_get_addr->addr_cb = p_get_addr->addr_cr = p_get_addr->addr_y;
//}
//-------------------------------------------------------------------------------

//VOID ime_set_yuv_converter_enable(IME_FUNC_EN set_en)
//{
//	ime_set_yuv_converter_enable_reg((UINT32)set_en);
//}
//-------------------------------------------------------------------------------

//VOID ime_set_yuv_converter_sel(IME_YUV_CVT set_cvt)
//{
//	ime_set_yuv_converter_sel_reg((UINT32)set_cvt);
//}
//-------------------------------------------------------------------------------

//UINT32 ime_get_yuv_converter_enable(VOID)
//{
//	return ime_get_yuv_converter_enable_reg();
//}
//-------------------------------------------------------------------------------

//UINT32 ime_get_yuv_converter_sel(VOID)
//{
//	return ime_get_yuv_converter_sel_reg();
//}
//-------------------------------------------------------------------------------


// integration APIs
ER ime_set_in_path(IME_INPATH_INFO *p_in_path_info)
{
	ER er_return = 0x0;
	UINT32 flush_size_y = 0x0, flush_size_c = 0x0;
	IME_OPMODE mode = 0;
	IME_BUF_FLUSH_SEL get_flush = IME_DO_BUF_FLUSH;

	IME_LINEOFFSET_INFO set_lofs = {0x0};
	IME_DMA_ADDR_INFO set_addr = {0x0};

	if (p_in_path_info == NULL) {
		DBG_ERR("IME: input, setting parameter NULL...\r\n");

		return E_PAR;
	}

	//---------------------------------------------------------------------------------------------
	//er_return = ime_set_in_path_image_size(&(p_in_path_info->in_size));
	//if (er_return != E_OK) {
	//  return er_return;
	//}
	if ((p_in_path_info->in_size.size_h & 0x3) != 0) {
		DBG_ERR("IME: input, image size error, horizontal size is not 4x...\r\n");

		return E_PAR;
	}

	if (p_in_path_info->in_size.size_h < 8) {
		//
		DBG_WRN("IME: input, image width less than (<) 8 is risky, please check!!!\r\n");
	}

	if (p_in_path_info->in_size.size_v < 8) {
		//
		DBG_WRN("IME: input, image height less than (<) 8 is risky, please check!!!\r\n");
	}

	ime_set_input_image_size_reg(p_in_path_info->in_size.size_h, p_in_path_info->in_size.size_v);
	//---------------------------------------------------------------------------------------------

	// do not update to PatternGen.
	mode = ime_get_operate_mode();

	if (mode == IME_OPMODE_D2D) {

		get_flush = p_in_path_info->in_path_dma_flush;
		switch (p_in_path_info->in_path_dma_flush) {
		case IME_DO_BUF_FLUSH:
		case IME_NOTDO_BUF_FLUSH:
			break;

		default:
			get_flush = IME_DO_BUF_FLUSH;
			break;
		}

		//ime_set_in_path_image_format(p_in_path_info->in_format);
		ime_set_input_image_format_reg(p_in_path_info->in_format);

		set_lofs = p_in_path_info->in_lineoffset;
		set_addr = p_in_path_info->in_addr;

		// cal. flush size and address setting
		switch (p_in_path_info->in_format) {
		//case IME_INPUT_YCC_444:
		//case IME_INPUT_YCC_422:
		case IME_INPUT_YCC_420:
			//case IME_INPUT_RGB:
			flush_size_y = p_in_path_info->in_lineoffset.lineoffset_y * p_in_path_info->in_size.size_v;
			flush_size_c = p_in_path_info->in_lineoffset.lineoffset_cb * p_in_path_info->in_size.size_v;

			flush_size_y = IME_ALIGN_CEIL_32(flush_size_y);
			flush_size_c = IME_ALIGN_CEIL_32(flush_size_c);

			if (get_flush == IME_DO_BUF_FLUSH) {
				//ime_engine_operation_mode = ime_get_operate_mode();
				set_addr.addr_y  = ime_input_dma_buf_flush(set_addr.addr_y, flush_size_y, ime_engine_operation_mode);
				set_addr.addr_cb = ime_input_dma_buf_flush(set_addr.addr_cb, flush_size_c, ime_engine_operation_mode);
				set_addr.addr_cr = ime_input_dma_buf_flush(set_addr.addr_cr, flush_size_c, ime_engine_operation_mode);
			}
			break;

		//case IME_INPUT_YCCP_422:
		case IME_INPUT_YCCP_420:
			flush_size_y = p_in_path_info->in_lineoffset.lineoffset_y * p_in_path_info->in_size.size_v;
			flush_size_c = p_in_path_info->in_lineoffset.lineoffset_cb * p_in_path_info->in_size.size_v;

			flush_size_y = IME_ALIGN_CEIL_32(flush_size_y);
			flush_size_c = IME_ALIGN_CEIL_32(flush_size_c);

			// coverity[assigned_value]
			set_addr.addr_cr = set_addr.addr_cb;

			if (get_flush == IME_DO_BUF_FLUSH) {
				//ime_engine_operation_mode = ime_get_operate_mode();
				set_addr.addr_y  = ime_input_dma_buf_flush(set_addr.addr_y, flush_size_y, ime_engine_operation_mode);
				set_addr.addr_cb = ime_input_dma_buf_flush(set_addr.addr_cb, flush_size_c, ime_engine_operation_mode);
				//set_addr.addr_cr = ime_input_dma_buf_flush(set_addr.addr_cr, flush_size_c, ime_engine_operation_mode);
			}
			break;

		case IME_INPUT_Y_ONLY:
			flush_size_y = p_in_path_info->in_lineoffset.lineoffset_y * p_in_path_info->in_size.size_v;
			//flush_size_c = p_in_path_info->in_lineoffset.lineoffset_y * p_in_path_info->in_size.size_v;

			flush_size_y = IME_ALIGN_CEIL_32(flush_size_y);

			// coverity[assigned_value]
			set_lofs.lineoffset_cb = set_lofs.lineoffset_y;
			// coverity[assigned_value]
			set_addr.addr_cr = set_addr.addr_cb = set_addr.addr_y;

			if (get_flush == IME_DO_BUF_FLUSH) {
				//ime_engine_operation_mode = ime_get_operate_mode();
				set_addr.addr_y  = ime_input_dma_buf_flush(set_addr.addr_y, flush_size_y, ime_engine_operation_mode);
				//set_addr.addr_cb = ime_input_dma_buf_flush(set_addr.addr_cb, flush_size_c, ime_engine_operation_mode);
				//set_addr.addr_cr = ime_input_dma_buf_flush(set_addr.addr_cr, flush_size_c, ime_engine_operation_mode);
			}
			break;

		default:
			break;
		}

		//------------------------------------------------------------------------
		//er_return = ime_set_in_path_lineoffset(&set_lofs);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		//er_return = ime_set_in_path_dma_addr(&set_addr);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		// do not update to PatternGen.
		mode = ime_get_operate_mode();

		if (mode == IME_OPMODE_D2D) {
		    // do not check input line-offset zeor or not due to issue "NA51084-302"
			er_return = ime_check_lineoffset(set_lofs.lineoffset_y, set_lofs.lineoffset_cb, FALSE);
			if (er_return != E_OK) {

				DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)set_lofs.lineoffset_y);
				DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)set_lofs.lineoffset_cb);

				return er_return;
			}

			ime_set_input_lineoffset_y_reg(set_lofs.lineoffset_y);
			ime_set_input_lineoffset_c_reg(set_lofs.lineoffset_cb);

			ime_set_input_channel_addr1_reg(set_addr.addr_y, set_addr.addr_cb, set_addr.addr_cr);
		}
		//------------------------------------------------------------------------

	}

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_set_out_path_p1(IME_OUTPATH_INFO *p_out_path_info)
{
	// reserved
	ER er_return = E_OK;
	UINT32 flush_size_y = 0x0, flush_size_c = 0x0;
	BOOL flush_type_y = FALSE, flush_type_c = FALSE;

	IME_BUF_FLUSH_SEL get_flush = IME_DO_BUF_FLUSH;

	IME_LINEOFFSET_INFO set_lofs = {0x0};
	IME_DMA_ADDR_INFO set_addr = {0x0};

	IME_SIZE_INFO get_size = {0x0};
	IME_SCALE_FILTER_INFO scale_filter = {0};

	if (p_out_path_info == NULL) {
		DBG_ERR("IME: path1, setting parameter NULL...\r\n");

		return E_PAR;
	}

	if (p_out_path_info->out_path_enable == IME_FUNC_ENABLE) {
		//ime_set_out_path_image_format_p1(p_out_path_info->out_path_image_format.out_format_type_sel, p_out_path_info->out_path_image_format.out_format_sel);
		ime_set_output_type_p1_reg((UINT32)p_out_path_info->out_path_image_format.out_format_type_sel);
		ime_set_output_image_format_p1_reg((UINT32)p_out_path_info->out_path_image_format.out_format_sel);

		if (p_out_path_info->out_path_scale_method != IMEALG_INTEGRATION) { // 510 path1 do not support ISD
			//ime_set_out_path_scale_method_p1(p_out_path_info->out_path_scale_method);
			ime_set_scale_interpolation_method_p1_reg((UINT32)p_out_path_info->out_path_scale_method);
		} else {
			DBG_ERR("IME: path1, do not support ISD...\r\n");

			return E_PAR;
		}

		//---------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_scale_size_p1(&(p_out_path_info->out_path_scale_size));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_out_path_info->out_path_scale_size.size_h > 65535) || (p_out_path_info->out_path_scale_size.size_v > 65535)) {
			DBG_ERR("IME: path1, scaling size overflow...\r\n");

			return E_PAR;
		}

		if (p_out_path_info->out_path_scale_size.size_h < 8) {
			//
			DBG_WRN("IME: path1, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_out_path_info->out_path_scale_size.size_v < 8) {
			//
			DBG_WRN("IME: path1, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		ime_set_scaling_size_p1_reg(p_out_path_info->out_path_scale_size.size_h, p_out_path_info->out_path_scale_size.size_v);

		//---------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_output_crop_size_p1(&(p_out_path_info->out_path_scale_size), &(p_out_path_info->out_path_out_size), &(p_out_path_info->out_path_crop_pos));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_out_path_info->out_path_out_size.size_h > 65535) || (p_out_path_info->out_path_out_size.size_v > 65535)) {
			DBG_ERR("IME: path1, horizontal or vertical crop size overflow...\r\n");

			return E_PAR;
		}

		if (p_out_path_info->out_path_out_size.size_h < 32) {
			//
			DBG_WRN("IME: path1, horizontal crop size less than (<) 32 is risky, please check!!!\r\n");
		}

		if (p_out_path_info->out_path_out_size.size_v < 32) {
			//
			DBG_WRN("IME: path1, vertical crop size less than (<) 32 is risky, please check!!!\r\n");
		}

		if (((p_out_path_info->out_path_crop_pos.pos_x + p_out_path_info->out_path_out_size.size_h) > p_out_path_info->out_path_scale_size.size_h) || ((p_out_path_info->out_path_crop_pos.pos_y + p_out_path_info->out_path_out_size.size_v) > p_out_path_info->out_path_scale_size.size_v)) {
			DBG_ERR("IME: path1, horizontal or vertical crop size is exceeding boundary...\r\n");

			return E_PAR;
		}

		ime_set_output_size_p1_reg(p_out_path_info->out_path_out_size.size_h, p_out_path_info->out_path_out_size.size_v);
		ime_set_output_crop_coordinate_p1_reg(p_out_path_info->out_path_crop_pos.pos_x, p_out_path_info->out_path_crop_pos.pos_y);

		//---------------------------------------------------------------------------------------

		ime_set_scaling_enhance_factor_p1_reg(p_out_path_info->out_path_scale_enh.enh_factor, p_out_path_info->out_path_scale_enh.enh_bit);

		//ime_get_in_path_image_size(&get_size);
		ime_get_input_image_size_reg(&(get_size.size_h), &(get_size.size_v));

		scale_filter = p_out_path_info->out_path_scale_filter;
		if (scale_filter.CoefCalMode == IME_SCALE_FILTER_COEF_AUTO_MODE) {
			scale_filter.scale_h_filter_coef = ime_cal_scale_filter_coefs(get_size.size_h, p_out_path_info->out_path_scale_size.size_h);
			scale_filter.scale_h_filter_enable = ((get_size.size_h > p_out_path_info->out_path_scale_size.size_h) ? IME_FUNC_ENABLE : IME_FUNC_DISABLE);
			scale_filter.scale_v_filter_coef = ime_cal_scale_filter_coefs(get_size.size_v, p_out_path_info->out_path_scale_size.size_v);
			scale_filter.scale_v_filter_enable = ((get_size.size_v > p_out_path_info->out_path_scale_size.size_v) ? IME_FUNC_ENABLE : IME_FUNC_DISABLE);
		}
		//er_return = ime_set_out_path_scale_filter_p1(&scale_filter);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((scale_filter.scale_h_filter_coef > 63) || (scale_filter.scale_v_filter_coef > 63)) {
			DBG_ERR("IME: path1, scale filter coef. overflow...\r\n");

			return E_PAR;
		}

		ime_set_horizontal_scale_filtering_p1_reg((UINT32)scale_filter.scale_h_filter_enable, scale_filter.scale_h_filter_coef);
		ime_set_vertical_scale_filtering_p1_reg((UINT32)scale_filter.scale_v_filter_enable, scale_filter.scale_v_filter_coef);

		//-------------------------------------------------------------

		ime_scale_factor_params = p_out_path_info->out_path_scale_factors;
		if (ime_scale_factor_params.CalScaleFactorMode == IME_SCALE_FACTOR_COEF_AUTO_MODE) {
			er_return = ime_cal_image_scale_params(&get_size, &(p_out_path_info->out_path_scale_size), p_out_path_info->out_path_scale_method, &ime_scale_factor_params);
			if (er_return != E_OK) {
				return er_return;
			}
#if 0 // remove ISD limitation

			if (p_out_path_info->out_path_scale_method == IMEALG_INTEGRATION) {
				FLOAT scale_down_factor = 0.0;

				scale_down_factor = (FLOAT)(get_size.size_h - 1) / (FLOAT)(p_out_path_info->out_path_scale_size.size_h - 1);
				if (scale_down_factor <= 1.98) {
					//ime_set_out_path_scale_method_p1(IMEALG_BICUBIC);
					ime_set_scale_interpolation_method_p1_reg((UINT32)IMEALG_BICUBIC);
				}
			}
#endif
		}

		//-----------------------------------------------------------------------------------
		//er_return = ime_set_out_path_scale_factor_p1(&ime_scale_factor_params);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		ime_set_scale_factor_h_p1_reg((UINT32)ime_scale_factor_params.scale_h_ud, ime_scale_factor_params.scale_h_dr, ime_scale_factor_params.scale_h_ftr);
		ime_set_scale_factor_v_p1_reg((UINT32)ime_scale_factor_params.scale_v_ud, ime_scale_factor_params.scale_v_dr, ime_scale_factor_params.scale_v_ftr);
		ime_set_integration_scaling_factor_base_p1_reg(ime_scale_factor_params.isd_scale_h_base_ftr, ime_scale_factor_params.isd_scale_v_base_ftr);

		ime_set_integration_scaling_factor0_p1_reg(ime_scale_factor_params.isd_scale_h_ftr[0], ime_scale_factor_params.isd_scale_v_ftr[0]);
		ime_set_integration_scaling_factor1_p1_reg(ime_scale_factor_params.isd_scale_h_ftr[1], ime_scale_factor_params.isd_scale_v_ftr[1]);
		ime_set_integration_scaling_factor2_p1_reg(ime_scale_factor_params.isd_scale_h_ftr[2], ime_scale_factor_params.isd_scale_v_ftr[2]);

		//-----------------------------------------------------------------------------------

		ime_engine_operation_mode = ime_get_operate_mode();

		get_flush = p_out_path_info->out_path_dma_flush;
		switch (p_out_path_info->out_path_dma_flush) {
		case IME_DO_BUF_FLUSH:
		case IME_NOTDO_BUF_FLUSH:
			break;

		default:
			get_flush = IME_DO_BUF_FLUSH;
			break;
		}

		set_lofs = p_out_path_info->out_path_out_lineoffset;
		set_addr = p_out_path_info->out_path_addr;
		switch (p_out_path_info->out_path_image_format.out_format_sel) {
#if 0
		case IME_OUTPUT_YCC_444:
		case IME_OUTPUT_RGB_444:
			flush_size_y = set_lofs.lineoffset_y * p_out_path_info->out_path_out_size.size_v;
			flush_size_c = set_lofs.lineoffset_cb * p_out_path_info->out_path_out_size.size_v;

			flush_type_y = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_y) ? TRUE : FALSE);
			flush_type_c = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_cb) ? TRUE : FALSE);
			break;
#endif

		//case IME_OUTPUT_YCC_422_COS:
		//case IME_OUTPUT_YCC_422_CEN:
		//case IME_OUTPUT_YCC_420_COS:
		case IME_OUTPUT_YCC_420:
			flush_size_y = set_lofs.lineoffset_y * p_out_path_info->out_path_out_size.size_v;
			flush_size_c = set_lofs.lineoffset_cb * (p_out_path_info->out_path_out_size.size_v >> 1);

			flush_size_y = IME_ALIGN_CEIL_32(flush_size_y);
			flush_size_c = IME_ALIGN_CEIL_32(flush_size_c);

			flush_type_y = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_y) ? TRUE : FALSE);
			flush_type_c = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_cb) ? TRUE : FALSE);

			// video buffer do not flush
			switch (p_out_path_info->out_path_image_format.out_format_type_sel) {
			case IME_OUTPUT_YCC_PLANAR:
				set_addr.addr_y  = ime_output_dma_buf_flush(set_addr.addr_y, flush_size_y, flush_type_y, get_flush, ime_engine_operation_mode);
				set_addr.addr_cb = ime_output_dma_buf_flush(set_addr.addr_cb, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);
				set_addr.addr_cr = ime_output_dma_buf_flush(set_addr.addr_cr, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);

				if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (get_flush == IME_DO_BUF_FLUSH)) {

					ime_out_buf_flush[IME_OUT_P1_Y].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_P1_Y].buf_addr = set_addr.addr_y;
					ime_out_buf_flush[IME_OUT_P1_Y].buf_size = flush_size_y;

					ime_out_buf_flush[IME_OUT_P1_U].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_P1_U].buf_addr = set_addr.addr_cb;
					ime_out_buf_flush[IME_OUT_P1_U].buf_size = flush_size_c;

					ime_out_buf_flush[IME_OUT_P1_V].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_P1_V].buf_addr = set_addr.addr_cr;
					ime_out_buf_flush[IME_OUT_P1_V].buf_size = flush_size_c;
				}

				break;

			case IME_OUTPUT_YCC_UVPACKIN:
				// coverity[assigned_value]
				set_addr.addr_cr = set_addr.addr_cb;

				set_addr.addr_y  = ime_output_dma_buf_flush(set_addr.addr_y, flush_size_y, flush_type_y, get_flush, ime_engine_operation_mode);
				set_addr.addr_cb = ime_output_dma_buf_flush(set_addr.addr_cb, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);
				//set_addr.addr_cr = ime_output_dma_buf_flush(set_addr.addr_cr, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);

				if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (get_flush == IME_DO_BUF_FLUSH)) {

					ime_out_buf_flush[IME_OUT_P1_Y].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_P1_Y].buf_addr = set_addr.addr_y;
					ime_out_buf_flush[IME_OUT_P1_Y].buf_size = flush_size_y;

					ime_out_buf_flush[IME_OUT_P1_U].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_P1_U].buf_addr = set_addr.addr_cb;
					ime_out_buf_flush[IME_OUT_P1_U].buf_size = flush_size_c;
				}

				break;

			default:
				break;
			}
			break;

		case IME_OUTPUT_Y_ONLY:
			// coverity[assigned_value]
			set_lofs.lineoffset_cb = set_lofs.lineoffset_y;

			flush_size_y = set_lofs.lineoffset_y * p_out_path_info->out_path_out_size.size_v;
			//flush_size_c = set_lofs.lineoffset_y * p_out_path_info->out_path_out_size.size_v;

			flush_size_y = IME_ALIGN_CEIL_32(flush_size_y);

			// coverity[assigned_value]
			set_addr.addr_cr = set_addr.addr_cb = set_addr.addr_y;

			flush_type_y = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_y) ? TRUE : FALSE);
			//flush_type_c = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_cb) ? TRUE : FALSE);

			set_addr.addr_y  = ime_output_dma_buf_flush(set_addr.addr_y, flush_size_y, flush_type_y, get_flush, ime_engine_operation_mode);

			if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (get_flush == IME_DO_BUF_FLUSH)) {

				ime_out_buf_flush[IME_OUT_P1_Y].flush_en = ENABLE;
				ime_out_buf_flush[IME_OUT_P1_Y].buf_addr = set_addr.addr_y;
				ime_out_buf_flush[IME_OUT_P1_Y].buf_size = flush_size_y;
			}
			break;

		default:
			break;
		}

		//---------------------------------------------------------------

		//er_return = ime_set_out_path_lineoffset_p1(&set_lofs);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		er_return = ime_check_lineoffset(set_lofs.lineoffset_y, set_lofs.lineoffset_cb, TRUE);
		if (er_return != E_OK) {

			DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)set_lofs.lineoffset_y);
			DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)set_lofs.lineoffset_cb);

			return E_PAR;
		}

		ime_set_output_lineoffset_y_p1_reg(set_lofs.lineoffset_y);
		ime_set_output_lineoffset_c_p1_reg(set_lofs.lineoffset_cb);

		//---------------------------------------------------------------

		//er_return = ime_set_out_path_dma_addr_p1(&set_addr);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		ime_set_output_channel_addr_p1_reg(set_addr.addr_y, set_addr.addr_cb, set_addr.addr_cr);

		//---------------------------------------------------------------

		ime_chg_stitching_enable(IME_PATH1_SEL, p_out_path_info->out_path_sprtout_enable);
		if (p_out_path_info->out_path_sprtout_enable == IME_FUNC_ENABLE) {
			ime_stitching_image_params.stitch_pos = p_out_path_info->out_path_sprt_pos;
			ime_stitching_image_params.stitch_lofs = p_out_path_info->out_path_out_lineoffset2;
			ime_stitching_image_params.lofs_update = TRUE;
			ime_stitching_image_params.stitch_dma_addr = p_out_path_info->out_path_addr2;
			ime_stitching_image_params.dma_addr_update = TRUE;
			ime_stitching_image_params.dma_flush = get_flush;
			er_return = ime_chg_stitching_image_param(IME_PATH1_SEL, &(ime_stitching_image_params));
			if (er_return != E_OK) {
				return er_return;
			}
		}

		// encode enable
		//ime_set_encode_enable_p1(p_out_path_info->out_path_encode_enable);
		ime_set_encode_enable_p1_reg((UINT32)p_out_path_info->out_path_encode_enable);

		ime_comps_p1_encoder_shift_mode_enable_reg((UINT32)p_out_path_info->out_path_encode_smode_enable);


		ime_set_flip_enable_p1_reg((UINT32)p_out_path_info->out_path_flip_enable);

		ime_clamp_p1_reg(p_out_path_info->out_path_clamp.min_y, p_out_path_info->out_path_clamp.max_y, p_out_path_info->out_path_clamp.min_uv, p_out_path_info->out_path_clamp.max_uv);

		ime_set_output_dram_enable_p1_reg((UINT32)p_out_path_info->out_path_dram_enable);
	}

	//ime_set_out_path_p1_enable(p_out_path_info->out_path_enable);
	ime_open_output_p1_reg((UINT32)p_out_path_info->out_path_enable);

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_set_out_path_p2(IME_OUTPATH_INFO *p_out_path_info)
{
	// reserved
	ER er_return = E_OK;
	UINT32 flush_size_y = 0x0, flush_size_c = 0x0;
	BOOL flush_type_y = FALSE, flush_type_c = FALSE;

	IME_BUF_FLUSH_SEL get_flush = IME_DO_BUF_FLUSH;

	IME_LINEOFFSET_INFO set_lofs = {0x0};
	IME_DMA_ADDR_INFO set_addr = {0x0};

	IME_SIZE_INFO get_size = {0x0};

	IME_SCALE_FILTER_INFO scale_filter = {0};

	UINT32 i = 0;

	if (p_out_path_info == NULL) {
		DBG_ERR("IME: path2, setting parameter NULL...\r\n");

		return E_PAR;
	}

	if (p_out_path_info->out_path_enable == IME_FUNC_ENABLE) {
		//ime_set_out_path_image_format_p2(p_out_path_info->out_path_image_format.out_format_type_sel, p_out_path_info->out_path_image_format.out_format_sel);
		ime_set_output_type_p2_reg((UINT32)p_out_path_info->out_path_image_format.out_format_type_sel);
		ime_set_output_image_format_p2_reg((UINT32)p_out_path_info->out_path_image_format.out_format_sel);

		//ime_set_out_path_scale_method_p2(p_out_path_info->out_path_scale_method);
		ime_set_scale_interpolation_method_p2_reg((UINT32)p_out_path_info->out_path_scale_method);

		//-------------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_scale_size_p2(&(p_out_path_info->out_path_scale_size));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_out_path_info->out_path_scale_size.size_h > 65535) || (p_out_path_info->out_path_scale_size.size_v > 65535)) {
			DBG_ERR("IME: path2, scaling size overflow...\r\n");

			return E_PAR;
		}

		if (p_out_path_info->out_path_scale_size.size_h < 8) {
			//
			DBG_WRN("IME: path2, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_out_path_info->out_path_scale_size.size_v < 8) {
			//
			DBG_WRN("IME: path2, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		ime_set_scaling_size_p2_reg(p_out_path_info->out_path_scale_size.size_h, p_out_path_info->out_path_scale_size.size_v);

		//-------------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_output_crop_size_p2(&(p_out_path_info->out_path_scale_size), &(p_out_path_info->out_path_out_size), &(p_out_path_info->out_path_crop_pos));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_out_path_info->out_path_out_size.size_h > 65535) || (p_out_path_info->out_path_out_size.size_v > 65535)) {
			DBG_ERR("IME: path2, horizontal or vertical crop size is overflow...\r\n");

			return E_PAR;
		}

		if (p_out_path_info->out_path_out_size.size_h < 8) {
			//
			DBG_WRN("IME: path2, horizontal crop size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_out_path_info->out_path_out_size.size_v < 8) {
			//
			DBG_WRN("IME: path2, vertical crop size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (((p_out_path_info->out_path_crop_pos.pos_x + p_out_path_info->out_path_out_size.size_h) > p_out_path_info->out_path_scale_size.size_h) || ((p_out_path_info->out_path_crop_pos.pos_y + p_out_path_info->out_path_out_size.size_v) > p_out_path_info->out_path_scale_size.size_v)) {
			DBG_ERR("IME: path2, horizontal or vertical crop size is exceeding boundary...\r\n");

			return E_PAR;
		}

		ime_set_output_size_p2_reg(p_out_path_info->out_path_out_size.size_h, p_out_path_info->out_path_out_size.size_v);
		ime_set_output_crop_coordinate_p2_reg(p_out_path_info->out_path_crop_pos.pos_x, p_out_path_info->out_path_crop_pos.pos_y);


		//-------------------------------------------------------------------------------------------------

		ime_set_scaling_enhance_factor_p2_reg(p_out_path_info->out_path_scale_enh.enh_factor, p_out_path_info->out_path_scale_enh.enh_bit);

		//ime_get_in_path_image_size(&get_size);
		ime_get_input_image_size_reg(&(get_size.size_h), &(get_size.size_v));

		scale_filter = p_out_path_info->out_path_scale_filter;
		if (scale_filter.CoefCalMode == IME_SCALE_FILTER_COEF_AUTO_MODE) {
			scale_filter.scale_h_filter_coef = ime_cal_scale_filter_coefs(get_size.size_h, p_out_path_info->out_path_scale_size.size_h);
			scale_filter.scale_h_filter_enable = ((get_size.size_h > p_out_path_info->out_path_scale_size.size_h) ? IME_FUNC_ENABLE : IME_FUNC_DISABLE);
			scale_filter.scale_v_filter_coef = ime_cal_scale_filter_coefs(get_size.size_v, p_out_path_info->out_path_scale_size.size_v);
			scale_filter.scale_v_filter_enable = ((get_size.size_v > p_out_path_info->out_path_scale_size.size_v) ? IME_FUNC_ENABLE : IME_FUNC_DISABLE);
		}

		//er_return = ime_set_out_path_scale_filter_p2(&scale_filter);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((scale_filter.scale_h_filter_coef > 63) || (scale_filter.scale_v_filter_coef > 63)) {
			DBG_ERR("IME: path2, scale filter coef. overflow...\r\n");

			return E_PAR;
		}

		ime_set_horizontal_scale_filtering_p2_reg((UINT32)scale_filter.scale_h_filter_enable, scale_filter.scale_h_filter_coef);
		ime_set_vertical_scale_filtering_p2_reg((UINT32)scale_filter.scale_v_filter_enable, scale_filter.scale_v_filter_coef);

		//-------------------------------------------------------------------------------

		ime_scale_factor_params = p_out_path_info->out_path_scale_factors;
		if (ime_scale_factor_params.CalScaleFactorMode == IME_SCALE_FACTOR_COEF_AUTO_MODE) {
			er_return = ime_cal_image_scale_params(&get_size, &(p_out_path_info->out_path_scale_size), p_out_path_info->out_path_scale_method, &ime_scale_factor_params);
			if (er_return != E_OK) {
				return er_return;
			}
#if 0 // remove ISD limitation
			if (p_out_path_info->out_path_scale_method == IMEALG_INTEGRATION) {
				FLOAT scale_down_factor = 0.0;

				scale_down_factor = (FLOAT)(get_size.size_h - 1) / (FLOAT)(p_out_path_info->out_path_scale_size.size_h - 1);
				if (scale_down_factor <= 1.98) {
					//ime_set_out_path_scale_method_p2(IMEALG_BICUBIC);
					ime_set_scale_interpolation_method_p2_reg((UINT32)IMEALG_BICUBIC);
				}
			}
#endif
		}

		//er_return = ime_set_out_path_scale_factor_p2(&ime_scale_factor_params);
		//if (er_return != E_OK) {
		//  return er_return;
		//}
		ime_set_scale_factor_h_p2_reg((UINT32)ime_scale_factor_params.scale_h_ud, ime_scale_factor_params.scale_h_dr, ime_scale_factor_params.scale_h_ftr);
		ime_set_scale_factor_v_p2_reg((UINT32)ime_scale_factor_params.scale_v_ud, ime_scale_factor_params.scale_v_dr, ime_scale_factor_params.scale_v_ftr);

		//----------------------------------------------------------------------------------------


		if (p_out_path_info->out_path_scale_method == IMEALG_INTEGRATION) {
			//er_return = ime_set_out_path_isd_scale_factor_p2(&ime_scale_factor_params);
			//if (er_return != E_OK) {
			//  return er_return;
			//}

			for (i = 0; i < 3; i++) {
				if ((ime_scale_factor_params.isd_scale_h_ftr[i] > 4096)) {
					DBG_ERR("IME: path2, ISD H-factor overflow...%d\r\n", (int)ime_scale_factor_params.isd_scale_h_ftr[i]);

					return E_PAR;
				}

				if ((ime_scale_factor_params.isd_scale_v_ftr[i] > 4096)) {
					DBG_ERR("IME: path2, ISD V-factor overflow...%d\r\n", (int)ime_scale_factor_params.isd_scale_v_ftr[i]);

					return E_PAR;
				}
			}

			if ((ime_scale_factor_params.isd_scale_h_base_ftr > 4096) || (ime_scale_factor_params.isd_scale_v_base_ftr > 4096)) {
				DBG_ERR("IME: path2, ISD factor base overflow...\r\n");

				return E_PAR;
			}

			ime_set_integration_scaling_factor_base_p2_reg(ime_scale_factor_params.isd_scale_h_base_ftr, ime_scale_factor_params.isd_scale_v_base_ftr);

			ime_set_integration_scaling_factor0_p2_reg(ime_scale_factor_params.isd_scale_h_ftr[0], ime_scale_factor_params.isd_scale_v_ftr[0]);
			ime_set_integration_scaling_factor1_p2_reg(ime_scale_factor_params.isd_scale_h_ftr[1], ime_scale_factor_params.isd_scale_v_ftr[1]);
			ime_set_integration_scaling_factor2_p2_reg(ime_scale_factor_params.isd_scale_h_ftr[2], ime_scale_factor_params.isd_scale_v_ftr[2]);

			ime_set_integration_scaling_ctrl_p2_reg(ime_scale_factor_params.isd_coef_ctrl, ime_scale_factor_params.isd_scale_h_coef_nums, ime_scale_factor_params.isd_scale_v_coef_nums);
			ime_set_integration_scaling_h_coefs_p2_reg(ime_scale_factor_params.isd_scale_h_coefs);
			ime_set_integration_scaling_v_coefs_p2_reg(ime_scale_factor_params.isd_scale_v_coefs);
			ime_set_integration_scaling_h_coefs_sum_p2_reg(ime_scale_factor_params.isd_scale_h_coefs_all_sum, ime_scale_factor_params.isd_scale_h_coefs_half_sum);
			ime_set_integration_scaling_v_coefs_sum_p2_reg(ime_scale_factor_params.isd_scale_v_coefs_all_sum, ime_scale_factor_params.isd_scale_v_coefs_half_sum);
		}

		//----------------------------------------------------------------------------------------

		ime_engine_operation_mode = ime_get_operate_mode();

		get_flush = p_out_path_info->out_path_dma_flush;
		switch (p_out_path_info->out_path_dma_flush) {
		case IME_DO_BUF_FLUSH:
		case IME_NOTDO_BUF_FLUSH:
			break;

		default:
			get_flush = IME_DO_BUF_FLUSH;
			break;
		}

		set_lofs = p_out_path_info->out_path_out_lineoffset;
		set_addr = p_out_path_info->out_path_addr;
		switch (p_out_path_info->out_path_image_format.out_format_sel) {
#if 0
		case IME_OUTPUT_YCC_444:
		case IME_OUTPUT_RGB_444:
			flush_size_y = set_lofs.lineoffset_y * p_out_path_info->out_path_out_size.size_v;
			flush_size_c = set_lofs.lineoffset_cb * p_out_path_info->out_path_out_size.size_v;

			flush_type_y = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_y) ? TRUE : FALSE);
			flush_type_c = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_cb) ? TRUE : FALSE);
			break;
#endif

		//case IME_OUTPUT_YCC_422_COS:
		//case IME_OUTPUT_YCC_422_CEN:
		//case IME_OUTPUT_YCC_420_COS:
		case IME_OUTPUT_YCC_420:
			flush_size_y = set_lofs.lineoffset_y * p_out_path_info->out_path_out_size.size_v;
			flush_size_c = set_lofs.lineoffset_cb * (p_out_path_info->out_path_out_size.size_v >> 1);

			flush_size_y = IME_ALIGN_CEIL_32(flush_size_y);
			flush_size_c = IME_ALIGN_CEIL_32(flush_size_c);

			flush_type_y = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_y) ? TRUE : FALSE);
			flush_type_c = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_cb) ? TRUE : FALSE);

			switch (p_out_path_info->out_path_image_format.out_format_type_sel) {
			case IME_OUTPUT_YCC_PLANAR:
				set_addr.addr_y  = ime_output_dma_buf_flush(set_addr.addr_y, flush_size_y, flush_type_y, get_flush, ime_engine_operation_mode);
				set_addr.addr_cb = ime_output_dma_buf_flush(set_addr.addr_cb, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);
				set_addr.addr_cr = ime_output_dma_buf_flush(set_addr.addr_cr, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);

				if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (get_flush == IME_DO_BUF_FLUSH)) {

					ime_out_buf_flush[IME_OUT_P2_Y].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_P2_Y].buf_addr = set_addr.addr_y;
					ime_out_buf_flush[IME_OUT_P2_Y].buf_size = flush_size_y;

					ime_out_buf_flush[IME_OUT_P2_U].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_P2_U].buf_addr = set_addr.addr_cb;
					ime_out_buf_flush[IME_OUT_P2_U].buf_size = flush_size_c;
				}

				break;

			case IME_OUTPUT_YCC_UVPACKIN:
				// coverity[assigned_value]
				set_addr.addr_cr = set_addr.addr_cb;

				set_addr.addr_y  = ime_output_dma_buf_flush(set_addr.addr_y, flush_size_y, flush_type_y, get_flush, ime_engine_operation_mode);
				set_addr.addr_cb = ime_output_dma_buf_flush(set_addr.addr_cb, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);
				//set_addr.addr_cr = ime_output_dma_buf_flush(set_addr.addr_cr, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);

				if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (get_flush == IME_DO_BUF_FLUSH)) {

					ime_out_buf_flush[IME_OUT_P2_Y].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_P2_Y].buf_addr = set_addr.addr_y;
					ime_out_buf_flush[IME_OUT_P2_Y].buf_size = flush_size_y;

					ime_out_buf_flush[IME_OUT_P2_U].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_P2_U].buf_addr = set_addr.addr_cb;
					ime_out_buf_flush[IME_OUT_P2_U].buf_size = flush_size_c;
				}

				break;

			default:
				break;
			}

			break;

		case IME_OUTPUT_Y_ONLY:
			// coverity[assigned_value]
			set_lofs.lineoffset_cb = set_lofs.lineoffset_y;

			flush_size_y = set_lofs.lineoffset_y * p_out_path_info->out_path_out_size.size_v;
			//flush_size_c = set_lofs.lineoffset_y * p_out_path_info->out_path_out_size.size_v;

			flush_size_y = IME_ALIGN_CEIL_32(flush_size_y);

			// coverity[assigned_value]
			set_addr.addr_cr = set_addr.addr_cb = set_addr.addr_y;

			flush_type_y = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_y) ? TRUE : FALSE);
			//flush_type_c = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_cb) ? TRUE : FALSE);

			set_addr.addr_y  = ime_output_dma_buf_flush(set_addr.addr_y, flush_size_y, flush_type_y, get_flush, ime_engine_operation_mode);
			//set_addr.addr_cb = ime_output_dma_buf_flush(set_addr.addr_cb, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);
			//set_addr.addr_cr = ime_output_dma_buf_flush(set_addr.addr_cr, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);

			if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (get_flush == IME_DO_BUF_FLUSH)) {

				ime_out_buf_flush[IME_OUT_P2_Y].flush_en = ENABLE;
				ime_out_buf_flush[IME_OUT_P2_Y].buf_addr = set_addr.addr_y;
				ime_out_buf_flush[IME_OUT_P2_Y].buf_size = flush_size_y;
			}
			break;

		default:
			break;
		}

		//-------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_lineoffset_p2(&set_lofs);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		er_return = ime_check_lineoffset(set_lofs.lineoffset_y, set_lofs.lineoffset_cb, TRUE);
		if (er_return != E_OK) {

			DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)set_lofs.lineoffset_y);
			DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)set_lofs.lineoffset_cb);

			return E_PAR;
		}

		ime_set_output_lineoffset_y_p2_reg(set_lofs.lineoffset_y);
		ime_set_output_lineoffset_c_p2_reg(set_lofs.lineoffset_cb);
		//-------------------------------------------------------------------------------------------

		// video buffer do not flush
		//set_addr.addr_y  = ime_output_dma_buf_flush(set_addr.addr_y, flush_size_y, flush_type_y, get_flush, ime_engine_operation_mode);
		//set_addr.addr_cb = ime_output_dma_buf_flush(set_addr.addr_cb, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);
		//set_addr.addr_cr = ime_output_dma_buf_flush(set_addr.addr_cr, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);
		//er_return = ime_set_out_path_dma_addr_p2(&set_addr);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		ime_set_output_channel_addr_p2_reg(set_addr.addr_y, set_addr.addr_cb, set_addr.addr_cr);

		//-------------------------------------------------------------------------------------------

		ime_chg_stitching_enable(IME_PATH2_SEL, p_out_path_info->out_path_sprtout_enable);
		if (p_out_path_info->out_path_sprtout_enable == IME_FUNC_ENABLE) {
			ime_stitching_image_params.stitch_pos = p_out_path_info->out_path_sprt_pos;
			ime_stitching_image_params.stitch_lofs = p_out_path_info->out_path_out_lineoffset2;
			ime_stitching_image_params.lofs_update = TRUE;
			ime_stitching_image_params.stitch_dma_addr = p_out_path_info->out_path_addr2;
			ime_stitching_image_params.dma_addr_update = TRUE;
			ime_stitching_image_params.dma_flush = get_flush;
			er_return = ime_chg_stitching_image_param(IME_PATH2_SEL, &(ime_stitching_image_params));
			if (er_return != E_OK) {
				return er_return;
			}
		}

		ime_set_flip_enable_p2_reg(p_out_path_info->out_path_flip_enable);

		ime_clamp_p2_reg(p_out_path_info->out_path_clamp.min_y, p_out_path_info->out_path_clamp.max_y, p_out_path_info->out_path_clamp.min_uv, p_out_path_info->out_path_clamp.max_uv);

		ime_set_output_dram_enable_p2_reg((UINT32)p_out_path_info->out_path_dram_enable);
	}

	//ime_set_out_path_p2_enable(p_out_path_info->out_path_enable);
	ime_open_output_p2_reg((UINT32)p_out_path_info->out_path_enable);

	return E_OK;
}
//-------------------------------------------------------------------------------

ER ime_set_out_path_p3(IME_OUTPATH_INFO *p_out_path_info)
{
	// reserved
	ER er_return = E_OK;
	UINT32 flush_size_y = 0x0, flush_size_c = 0x0;
	BOOL flush_type_y = FALSE, flush_type_c = FALSE;

	IME_BUF_FLUSH_SEL get_flush = IME_DO_BUF_FLUSH;

	IME_LINEOFFSET_INFO set_lofs = {0x0};
	IME_DMA_ADDR_INFO set_addr = {0x0};

	IME_SIZE_INFO get_size = {0x0};
	IME_SCALE_FILTER_INFO scale_filter = {0};

	UINT32 i = 0;


	if (p_out_path_info == NULL) {
		DBG_ERR("IME: path3, setting parameter NULL...\r\n");

		return E_PAR;
	}

	if (p_out_path_info->out_path_enable == IME_FUNC_ENABLE) {
		//ime_set_out_path_image_format_p3(p_out_path_info->out_path_image_format.out_format_type_sel, p_out_path_info->out_path_image_format.out_format_sel);
		ime_set_output_type_p3_reg((UINT32)p_out_path_info->out_path_image_format.out_format_type_sel);
		ime_set_output_image_format_p3_reg((UINT32)p_out_path_info->out_path_image_format.out_format_sel);

		//ime_set_out_path_scale_method_p3(p_out_path_info->out_path_scale_method);
		ime_set_scale_interpolation_method_p3_reg((UINT32)p_out_path_info->out_path_scale_method);

		//-------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_scale_size_p3(&(p_out_path_info->out_path_scale_size));
		//if (er_return != E_OK) {
		//  return er_return;
		//}


		if (p_out_path_info->out_path_scale_size.size_h < 8) {
			//
			DBG_WRN("IME: path3, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_out_path_info->out_path_scale_size.size_v < 8) {
			//
			DBG_WRN("IME: path3, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		ime_set_scaling_size_p3_reg(p_out_path_info->out_path_scale_size.size_h, p_out_path_info->out_path_scale_size.size_v);

		//-------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_output_crop_size_p3(&(p_out_path_info->out_path_scale_size), &(p_out_path_info->out_path_out_size), &(p_out_path_info->out_path_crop_pos));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_out_path_info->out_path_out_size.size_h > 65535) || (p_out_path_info->out_path_out_size.size_v > 65535)) {
			DBG_ERR("IME: path3, output size overflow...\r\n");

			return E_PAR;
		}

		if (p_out_path_info->out_path_out_size.size_h < 8) {
			//
			DBG_WRN("IME: path3, horizontal output size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_out_path_info->out_path_out_size.size_v < 8) {
			//
			DBG_WRN("IME: path3, vertical output size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (((p_out_path_info->out_path_crop_pos.pos_x + p_out_path_info->out_path_out_size.size_h) > p_out_path_info->out_path_scale_size.size_h) || ((p_out_path_info->out_path_crop_pos.pos_y + p_out_path_info->out_path_out_size.size_v) > p_out_path_info->out_path_scale_size.size_v)) {
			DBG_ERR("IME: path3, horizontal or vertical crop size is exceeding boundary...\r\n");

			return E_PAR;
		}

		ime_set_output_size_p3_reg(p_out_path_info->out_path_out_size.size_h, p_out_path_info->out_path_out_size.size_v);
		ime_set_output_crop_coordinate_p3_reg(p_out_path_info->out_path_crop_pos.pos_x, p_out_path_info->out_path_crop_pos.pos_y);

		//-------------------------------------------------------------------------------------------

		ime_set_scaling_enhance_factor_p3_reg(p_out_path_info->out_path_scale_enh.enh_factor, p_out_path_info->out_path_scale_enh.enh_bit);

		//ime_get_in_path_image_size(&get_size);
		ime_get_input_image_size_reg(&(get_size.size_h), &(get_size.size_v));

		scale_filter = p_out_path_info->out_path_scale_filter;
		if (scale_filter.CoefCalMode == IME_SCALE_FILTER_COEF_AUTO_MODE) {
			scale_filter.scale_h_filter_coef = ime_cal_scale_filter_coefs(get_size.size_h, p_out_path_info->out_path_scale_size.size_h);
			scale_filter.scale_h_filter_enable = ((get_size.size_h > p_out_path_info->out_path_scale_size.size_h) ? IME_FUNC_ENABLE : IME_FUNC_DISABLE);
			scale_filter.scale_v_filter_coef = ime_cal_scale_filter_coefs(get_size.size_v, p_out_path_info->out_path_scale_size.size_v);
			scale_filter.scale_v_filter_enable = ((get_size.size_v > p_out_path_info->out_path_scale_size.size_v) ? IME_FUNC_ENABLE : IME_FUNC_DISABLE);
		}
		//er_return = ime_set_out_path_scale_filter_p3(&scale_filter);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((scale_filter.scale_h_filter_coef > 63) || (scale_filter.scale_v_filter_coef > 63)) {
			DBG_ERR("IME: path3, scale filter coef. overflow...\r\n");

			return E_PAR;
		}

		ime_set_horizontal_scale_filtering_p3_reg((UINT32)scale_filter.scale_h_filter_enable, scale_filter.scale_h_filter_coef);
		ime_set_vertical_scale_filtering_p3_reg((UINT32)scale_filter.scale_v_filter_enable, scale_filter.scale_v_filter_coef);

		//----------------------------------------------------------------------------------------

		ime_scale_factor_params = p_out_path_info->out_path_scale_factors;
		if (ime_scale_factor_params.CalScaleFactorMode == IME_SCALE_FACTOR_COEF_AUTO_MODE) {
			er_return = ime_cal_image_scale_params(&get_size, &(p_out_path_info->out_path_scale_size), p_out_path_info->out_path_scale_method, &ime_scale_factor_params);
			if (er_return != E_OK) {
				return er_return;
			}

#if 0 // remove ISD limitation
			if (p_out_path_info->out_path_scale_method == IMEALG_INTEGRATION) {
				FLOAT scale_down_factor = 0.0;

				scale_down_factor = (FLOAT)(get_size.size_h - 1) / (FLOAT)(p_out_path_info->out_path_scale_size.size_h - 1);
				if (scale_down_factor <= 1.98) {
					ime_set_out_path_scale_method_p3(IMEALG_BICUBIC);
					ime_set_scale_interpolation_method_p3_reg((UINT32)IMEALG_BICUBIC);
				}
			}
#endif
		}

		//----------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_scale_factor_p3(&ime_scale_factor_params);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		ime_set_scale_factor_h_p3_reg((UINT32)ime_scale_factor_params.scale_h_ud, ime_scale_factor_params.scale_h_dr, ime_scale_factor_params.scale_h_ftr);
		ime_set_scale_factor_v_p3_reg((UINT32)ime_scale_factor_params.scale_v_ud, ime_scale_factor_params.scale_v_dr, ime_scale_factor_params.scale_v_ftr);

		//----------------------------------------------------------------------------------------

		//if (p_out_path_info->out_path_scale_method == IMEALG_INTEGRATION) {
		//  er_return = ime_set_out_path_isd_scale_factor_p3(&ime_scale_factor_params);
		//  if (er_return != E_OK) {
		//      return er_return;
		//  }
		//}

		for (i = 0; i < 3; i++) {
			if ((ime_scale_factor_params.isd_scale_h_ftr[i] > 4096)) {
				DBG_ERR("IME: path3, ISD H-factor overflow...%d\r\n", (int)ime_scale_factor_params.isd_scale_h_ftr[i]);

				return E_PAR;
			}

			if ((ime_scale_factor_params.isd_scale_v_ftr[i] > 4096)) {
				DBG_ERR("IME: path3, ISD V-factor overflow...%d\r\n", (int)ime_scale_factor_params.isd_scale_v_ftr[i]);

				return E_PAR;
			}
		}

		if ((ime_scale_factor_params.isd_scale_h_base_ftr > 4096) || (ime_scale_factor_params.isd_scale_v_base_ftr > 4096)) {
			DBG_ERR("IME: path3, ISD factor base overflow...\r\n");

			return E_PAR;
		}

		ime_set_integration_scaling_factor_base_p3_reg(ime_scale_factor_params.isd_scale_h_base_ftr, ime_scale_factor_params.isd_scale_v_base_ftr);

		ime_set_integration_scaling_factor0_p3_reg(ime_scale_factor_params.isd_scale_h_ftr[0], ime_scale_factor_params.isd_scale_v_ftr[0]);
		ime_set_integration_scaling_factor1_p3_reg(ime_scale_factor_params.isd_scale_h_ftr[1], ime_scale_factor_params.isd_scale_v_ftr[1]);
		ime_set_integration_scaling_factor2_p3_reg(ime_scale_factor_params.isd_scale_h_ftr[2], ime_scale_factor_params.isd_scale_v_ftr[2]);

		ime_set_integration_scaling_ctrl_p3_reg(ime_scale_factor_params.isd_coef_ctrl, ime_scale_factor_params.isd_scale_h_coef_nums, ime_scale_factor_params.isd_scale_v_coef_nums);
		ime_set_integration_scaling_h_coefs_p3_reg(ime_scale_factor_params.isd_scale_h_coefs);
		ime_set_integration_scaling_v_coefs_p3_reg(ime_scale_factor_params.isd_scale_v_coefs);

		ime_set_integration_scaling_h_coefs_sum_p3_reg(ime_scale_factor_params.isd_scale_h_coefs_all_sum, ime_scale_factor_params.isd_scale_h_coefs_half_sum);
		ime_set_integration_scaling_v_coefs_sum_p3_reg(ime_scale_factor_params.isd_scale_v_coefs_all_sum, ime_scale_factor_params.isd_scale_v_coefs_half_sum);

		//----------------------------------------------------------------------------------------

		ime_engine_operation_mode = ime_get_operate_mode();

		get_flush = p_out_path_info->out_path_dma_flush;
		switch (p_out_path_info->out_path_dma_flush) {
		case IME_DO_BUF_FLUSH:
		case IME_NOTDO_BUF_FLUSH:
			break;

		default:
			get_flush = IME_DO_BUF_FLUSH;
			break;
		}

		set_lofs = p_out_path_info->out_path_out_lineoffset;
		set_addr = p_out_path_info->out_path_addr;
		switch (p_out_path_info->out_path_image_format.out_format_sel) {
#if 0
		case IME_OUTPUT_YCC_444:
		case IME_OUTPUT_RGB_444:
			flush_size_y = set_lofs.lineoffset_y * p_out_path_info->out_path_out_size.size_v;
			flush_size_c = set_lofs.lineoffset_cb * p_out_path_info->out_path_out_size.size_v;

			flush_type_y = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_y) ? TRUE : FALSE);
			flush_type_c = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_cb) ? TRUE : FALSE);
			break;
#endif

		//case IME_OUTPUT_YCC_422_COS:
		//case IME_OUTPUT_YCC_422_CEN:
		//case IME_OUTPUT_YCC_420_COS:
		case IME_OUTPUT_YCC_420:
			flush_size_y = set_lofs.lineoffset_y * p_out_path_info->out_path_out_size.size_v;
			flush_size_c = set_lofs.lineoffset_cb * (p_out_path_info->out_path_out_size.size_v >> 1);

			flush_size_y = IME_ALIGN_CEIL_32(flush_size_y);
			flush_size_c = IME_ALIGN_CEIL_32(flush_size_c);

			flush_type_y = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_y) ? TRUE : FALSE);
			flush_type_c = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_cb) ? TRUE : FALSE);

			switch (p_out_path_info->out_path_image_format.out_format_type_sel) {
			case IME_OUTPUT_YCC_PLANAR:
				set_addr.addr_y  = ime_output_dma_buf_flush(set_addr.addr_y, flush_size_y, flush_type_y, get_flush, ime_engine_operation_mode);
				set_addr.addr_cb = ime_output_dma_buf_flush(set_addr.addr_cb, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);
				set_addr.addr_cr = ime_output_dma_buf_flush(set_addr.addr_cr, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);

				if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (get_flush == IME_DO_BUF_FLUSH)) {

					ime_out_buf_flush[IME_OUT_P3_Y].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_P3_Y].buf_addr = set_addr.addr_y;
					ime_out_buf_flush[IME_OUT_P3_Y].buf_size = flush_size_y;

					ime_out_buf_flush[IME_OUT_P3_U].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_P3_U].buf_addr = set_addr.addr_cb;
					ime_out_buf_flush[IME_OUT_P3_U].buf_size = flush_size_c;
				}

				break;

			case IME_OUTPUT_YCC_UVPACKIN:
				// coverity[assigned_value]
				set_addr.addr_cr = set_addr.addr_cb;

				set_addr.addr_y  = ime_output_dma_buf_flush(set_addr.addr_y, flush_size_y, flush_type_y, get_flush, ime_engine_operation_mode);
				set_addr.addr_cb = ime_output_dma_buf_flush(set_addr.addr_cb, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);
				//set_addr.addr_cr = ime_output_dma_buf_flush(set_addr.addr_cr, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);

				if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (get_flush == IME_DO_BUF_FLUSH)) {

					ime_out_buf_flush[IME_OUT_P3_Y].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_P3_Y].buf_addr = set_addr.addr_y;
					ime_out_buf_flush[IME_OUT_P3_Y].buf_size = flush_size_y;

					ime_out_buf_flush[IME_OUT_P3_U].flush_en = ENABLE;
					ime_out_buf_flush[IME_OUT_P3_U].buf_addr = set_addr.addr_cb;
					ime_out_buf_flush[IME_OUT_P3_U].buf_size = flush_size_c;
				}

				break;

			default:
				break;
			}

			break;

		case IME_OUTPUT_Y_ONLY:
			// coverity[assigned_value]
			set_lofs.lineoffset_cb = set_lofs.lineoffset_y;

			flush_size_y = set_lofs.lineoffset_y * p_out_path_info->out_path_out_size.size_v;
			//flush_size_c = set_lofs.lineoffset_y * p_out_path_info->out_path_out_size.size_v;

			flush_size_y = IME_ALIGN_CEIL_32(flush_size_y);

			// coverity[assigned_value]
			set_addr.addr_cr = set_addr.addr_cb = set_addr.addr_y;

			flush_type_y = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_y) ? TRUE : FALSE);
			//flush_type_c = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_cb) ? TRUE : FALSE);

			set_addr.addr_y  = ime_output_dma_buf_flush(set_addr.addr_y, flush_size_y, flush_type_y, get_flush, ime_engine_operation_mode);
			//set_addr.addr_cb = ime_output_dma_buf_flush(set_addr.addr_cb, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);
			//set_addr.addr_cr = ime_output_dma_buf_flush(set_addr.addr_cr, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);

			if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (get_flush == IME_DO_BUF_FLUSH)) {

				ime_out_buf_flush[IME_OUT_P3_Y].flush_en = ENABLE;
				ime_out_buf_flush[IME_OUT_P3_Y].buf_addr = set_addr.addr_y;
				ime_out_buf_flush[IME_OUT_P3_Y].buf_size = flush_size_y;
			}

			break;

		default:
			break;
		}

		//-------------------------------------------------------------------

		//er_return = ime_set_out_path_lineoffset_p3(&set_lofs);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		er_return = ime_check_lineoffset(set_lofs.lineoffset_y, set_lofs.lineoffset_cb, TRUE);
		if (er_return != E_OK) {

			DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)set_lofs.lineoffset_y);
			DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)set_lofs.lineoffset_cb);

			return er_return;
		}

		ime_set_output_lineoffset_y_p3_reg(set_lofs.lineoffset_y);
		ime_set_output_lineoffset_c_p3_reg(set_lofs.lineoffset_cb);

		//-------------------------------------------------------------------

		// video buffer do not flush
		//set_addr.addr_y  = ime_output_dma_buf_flush(set_addr.addr_y, flush_size_y, flush_type_y, get_flush, ime_engine_operation_mode);
		//set_addr.addr_cb = ime_output_dma_buf_flush(set_addr.addr_cb, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);
		//set_addr.addr_cr = ime_output_dma_buf_flush(set_addr.addr_cr, flush_size_c, flush_type_c, get_flush, ime_engine_operation_mode);
		//er_return = ime_set_out_path_dma_addr_p3(&set_addr);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		ime_set_output_channel_addr_p3_reg(set_addr.addr_y, set_addr.addr_cb, set_addr.addr_cr);

		ime_chg_stitching_enable(IME_PATH3_SEL, p_out_path_info->out_path_sprtout_enable);
		if (p_out_path_info->out_path_sprtout_enable == IME_FUNC_ENABLE) {
			ime_stitching_image_params.stitch_pos = p_out_path_info->out_path_sprt_pos;
			ime_stitching_image_params.stitch_lofs = p_out_path_info->out_path_out_lineoffset2;
			ime_stitching_image_params.lofs_update = TRUE;
			ime_stitching_image_params.stitch_dma_addr = p_out_path_info->out_path_addr2;
			ime_stitching_image_params.dma_addr_update = TRUE;
			ime_stitching_image_params.dma_flush = get_flush;
			er_return = ime_chg_stitching_image_param(IME_PATH3_SEL, &(ime_stitching_image_params));
			if (er_return != E_OK) {
				return er_return;
			}
		}

		ime_set_flip_enable_p3_reg(p_out_path_info->out_path_flip_enable);

		ime_clamp_p3_reg(p_out_path_info->out_path_clamp.min_y, p_out_path_info->out_path_clamp.max_y, p_out_path_info->out_path_clamp.min_uv, p_out_path_info->out_path_clamp.max_uv);

		ime_set_output_dram_enable_p3_reg((UINT32)p_out_path_info->out_path_dram_enable);
	}

	//ime_set_out_path_p3_enable(p_out_path_info->out_path_enable);
	ime_open_output_p3_reg((UINT32)p_out_path_info->out_path_enable);

	return E_OK;
}
//-------------------------------------------------------------------------------


ER ime_set_out_path_p4(IME_OUTPATH_INFO *p_out_path_info)
{
	// reserved
	ER er_return = E_OK;
	UINT32 flush_size_y = 0x0;//flush_size_c = 0x0;
	BOOL flush_type_y = FALSE;// flush_type_c = FALSE;

	IME_BUF_FLUSH_SEL get_flush = IME_DO_BUF_FLUSH;

	IME_LINEOFFSET_INFO set_lofs = {0x0};
	IME_DMA_ADDR_INFO set_addr = {0x0};

	IME_SIZE_INFO get_size = {0x0};
	IME_SCALE_FILTER_INFO scale_filter = {0};

	if (p_out_path_info == NULL) {
		DBG_ERR("IME: path4, setting parameter NULL...\r\n");

		return E_PAR;
	}

	if (p_out_path_info->out_path_enable == IME_FUNC_ENABLE) {
		if (p_out_path_info->out_path_image_format.out_format_sel != IME_OUTPUT_Y_ONLY) {
			DBG_WRN("IME: path4, support only Y channel...\r\n");
		}
		//ime_set_out_path_image_format_p4(p_out_path_info->out_path_image_format.out_format_type_sel, p_out_path_info->out_path_image_format.out_format_sel);
		ime_set_output_type_p4_reg((UINT32)p_out_path_info->out_path_image_format.out_format_type_sel);
		ime_set_output_image_format_p4_reg((UINT32)p_out_path_info->out_path_image_format.out_format_sel);

		//-------------------------------------------------------------------------------------------------

		if (p_out_path_info->out_path_scale_method != IMEALG_BICUBIC) {
			DBG_ERR("IME: path4, scaling method support only Bicubic...\r\n");

			return E_PAR;
		}
		//ime_set_out_path_scale_method_p4(p_out_path_info->out_path_scale_method);
		ime_set_scale_interpolation_method_p4_reg((UINT32)p_out_path_info->out_path_scale_method);

		//------------------------------------------------------------------------------------------

		//er_return = ime_set_out_path_scale_size_p4(&(p_out_path_info->out_path_scale_size));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_out_path_info->out_path_scale_size.size_h > 65535) || (p_out_path_info->out_path_scale_size.size_v > 65535)) {
			DBG_ERR("IME: path4, scaling size overflow...\r\n");

			return E_PAR;
		}

		if (p_out_path_info->out_path_scale_size.size_h < 8) {
			//
			DBG_WRN("IME: path4, horizontal scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_out_path_info->out_path_scale_size.size_v < 8) {
			//
			DBG_WRN("IME: path4, vertical scaling size less than (<) 8 is risky, please check!!!\r\n");
		}

		ime_set_scaling_size_p4_reg(p_out_path_info->out_path_scale_size.size_h, p_out_path_info->out_path_scale_size.size_v);

		//------------------------------------------------------------------------------------------

		if ((IME_FUNC_EN)ime_get_adas_enable_reg() == IME_FUNC_ENABLE) {
			if ((p_out_path_info->out_path_out_size.size_h > 1920) || (p_out_path_info->out_path_out_size.size_v > 1080)) {
				DBG_ERR("IME: path4, stl. enable but output size is overflow...\r\n");
				return E_PAR;
			}
		}

		//er_return = ime_set_out_path_output_crop_size_p4(&(p_out_path_info->out_path_scale_size), &(p_out_path_info->out_path_out_size), &(p_out_path_info->out_path_crop_pos));
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		if ((p_out_path_info->out_path_out_size.size_h > 65535) || (p_out_path_info->out_path_out_size.size_v > 65535)) {
			DBG_ERR("IME: path4, horizontal or vertical crop size is overflow...\r\n");

			return E_PAR;
		}

		if (p_out_path_info->out_path_out_size.size_h < 8) {
			//
			DBG_WRN("IME: path4, horizontal output size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (p_out_path_info->out_path_out_size.size_v < 8) {
			//
			DBG_WRN("IME: path4, vertical output size less than (<) 8 is risky, please check!!!\r\n");
		}

		if (((p_out_path_info->out_path_crop_pos.pos_x + p_out_path_info->out_path_out_size.size_h) > p_out_path_info->out_path_scale_size.size_h) || ((p_out_path_info->out_path_crop_pos.pos_y + p_out_path_info->out_path_out_size.size_v) > p_out_path_info->out_path_scale_size.size_v)) {
			DBG_ERR("IME: path4, horizontal or vertical crop size is exceeding boundary...\r\n");

			return E_PAR;
		}

		ime_set_output_size_p4_reg(p_out_path_info->out_path_out_size.size_h, p_out_path_info->out_path_out_size.size_v);
		ime_set_output_crop_coordinate_p4_reg(p_out_path_info->out_path_crop_pos.pos_x, p_out_path_info->out_path_crop_pos.pos_y);

		//----------------------------------------------------------------------------------------------------------------------------------

		ime_set_scaling_enhance_factor_p4_reg(p_out_path_info->out_path_scale_enh.enh_factor, p_out_path_info->out_path_scale_enh.enh_bit);

		//ime_get_in_path_image_size(&get_size);
		ime_get_input_image_size_reg(&(get_size.size_h), &(get_size.size_v));

		scale_filter = p_out_path_info->out_path_scale_filter;
		if (scale_filter.CoefCalMode == IME_SCALE_FILTER_COEF_AUTO_MODE) {
			scale_filter.scale_h_filter_coef = ime_cal_scale_filter_coefs(get_size.size_h, p_out_path_info->out_path_scale_size.size_h);
			scale_filter.scale_h_filter_enable = ((get_size.size_h > p_out_path_info->out_path_scale_size.size_h) ? IME_FUNC_ENABLE : IME_FUNC_DISABLE);
			scale_filter.scale_v_filter_coef = ime_cal_scale_filter_coefs(get_size.size_v, p_out_path_info->out_path_scale_size.size_v);
			scale_filter.scale_v_filter_enable = ((get_size.size_v > p_out_path_info->out_path_scale_size.size_v) ? IME_FUNC_ENABLE : IME_FUNC_DISABLE);
		}

		//er_return = ime_set_out_path_scale_filter_p4(&scale_filter);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		ime_scale_factor_params = p_out_path_info->out_path_scale_factors;
		if (ime_scale_factor_params.CalScaleFactorMode == IME_SCALE_FACTOR_COEF_AUTO_MODE) {
			er_return = ime_cal_image_scale_params(&get_size, &(p_out_path_info->out_path_scale_size), p_out_path_info->out_path_scale_method, &ime_scale_factor_params);
			if (er_return != E_OK) {
				return er_return;
			}
		}

		//---------------------------------------------------------------------------------

		//er_return = ime_set_out_path_scale_factor_p4(&ime_scale_factor_params);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		ime_set_scale_factor_h_p4_reg((UINT32)ime_scale_factor_params.scale_h_ud, ime_scale_factor_params.scale_h_dr, ime_scale_factor_params.scale_h_ftr);
		ime_set_scale_factor_v_p4_reg((UINT32)ime_scale_factor_params.scale_v_ud, ime_scale_factor_params.scale_v_dr, ime_scale_factor_params.scale_v_ftr);

		//---------------------------------------------------------------------------------

		ime_engine_operation_mode = ime_get_operate_mode();

		get_flush = p_out_path_info->out_path_dma_flush;
		switch (p_out_path_info->out_path_dma_flush) {
		case IME_DO_BUF_FLUSH:
		case IME_NOTDO_BUF_FLUSH:
			break;

		default:
			get_flush = IME_DO_BUF_FLUSH;
			break;
		}

		set_lofs = p_out_path_info->out_path_out_lineoffset;
		set_addr = p_out_path_info->out_path_addr;
		switch (p_out_path_info->out_path_image_format.out_format_sel) {
#if 0
		case IME_OUTPUT_YCC_444:
		case IME_OUTPUT_RGB_444:
			flush_size_y = set_lofs.lineoffset_y * p_out_path_info->out_path_out_size.size_v;
			//flush_size_c = set_lofs.lineoffset_cb * p_out_path_info->out_path_out_size.size_v;

			flush_type_y = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_y) ? TRUE : FALSE);
			//flush_type_c = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_cb) ? TRUE : FALSE);
			break;
#endif

		//case IME_OUTPUT_YCC_422_COS:
		//case IME_OUTPUT_YCC_422_CEN:
		//case IME_OUTPUT_YCC_420_COS:
		case IME_OUTPUT_YCC_420:
			flush_size_y = set_lofs.lineoffset_y * p_out_path_info->out_path_out_size.size_v;
			//flush_size_c = set_lofs.lineoffset_cb * p_out_path_info->out_path_out_size.size_v;

			flush_size_y = IME_ALIGN_CEIL_32(flush_size_y);

			// coverity[assigned_value]
			set_addr.addr_cr = set_addr.addr_cb;
			//if (p_out_path_info->out_path_image_format.out_format_type_sel == IME_OUTPUT_YCC_UVPACKIN) {
			//  set_addr.addr_cr = set_addr.addr_cb;
			//}

			flush_type_y = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_y) ? TRUE : FALSE);
			//flush_type_c = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_cb) ? TRUE : FALSE);

			set_addr.addr_y  = ime_output_dma_buf_flush(set_addr.addr_y, flush_size_y, flush_type_y, get_flush, ime_engine_operation_mode);

			if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (get_flush == IME_DO_BUF_FLUSH)) {

				ime_out_buf_flush[IME_OUT_P4_Y].flush_en = ENABLE;
				ime_out_buf_flush[IME_OUT_P4_Y].buf_addr = set_addr.addr_y;
				ime_out_buf_flush[IME_OUT_P4_Y].buf_size = flush_size_y;
			}

			break;

		case IME_OUTPUT_Y_ONLY:
			// coverity[assigned_value]
			set_lofs.lineoffset_cb = set_lofs.lineoffset_y;

			flush_size_y = set_lofs.lineoffset_y * p_out_path_info->out_path_out_size.size_v;
			//flush_size_c = set_lofs.lineoffset_y * p_out_path_info->out_path_out_size.size_v;

			flush_size_y = IME_ALIGN_CEIL_32(flush_size_y);

			// coverity[assigned_value]
			set_addr.addr_cr = set_addr.addr_cb = set_addr.addr_y;

			flush_type_y = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_y) ? TRUE : FALSE);
			//flush_type_c = ((p_out_path_info->out_path_out_size.size_h == set_lofs.lineoffset_cb) ? TRUE : FALSE);

			set_addr.addr_y = ime_output_dma_buf_flush(set_addr.addr_y, flush_size_y, flush_type_y, get_flush, ime_engine_operation_mode);

			if ((ime_engine_operation_mode == IME_OPMODE_D2D) && (get_flush == IME_DO_BUF_FLUSH)) {

				ime_out_buf_flush[IME_OUT_P4_Y].flush_en = ENABLE;
				ime_out_buf_flush[IME_OUT_P4_Y].buf_addr = set_addr.addr_y;
				ime_out_buf_flush[IME_OUT_P4_Y].buf_size = flush_size_y;
			}

			break;

		default:
			break;
		}

		//er_return = ime_set_out_path_lineoffset_p4(&set_lofs);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		er_return = ime_check_lineoffset(set_lofs.lineoffset_y, set_lofs.lineoffset_y, TRUE);
		if (er_return != E_OK) {

			DBG_ERR("IME - lineoffset_y: %08x\r\n", (unsigned int)set_lofs.lineoffset_y);
			DBG_ERR("IME - lineoffset_cb: %08x\r\n", (unsigned int)set_lofs.lineoffset_cb);

			return er_return;
		}
		ime_set_output_lineoffset_y_p4_reg(set_lofs.lineoffset_y);



		// video buffer do not flush
		//set_addr.addr_y  = ime_output_dma_buf_flush(set_addr.addr_y, flush_size_y, flush_type_y, get_flush, ime_engine_operation_mode);
		//set_addr.addr_cb = set_addr.addr_y;//ime_output_dma_buf_flush(set_addr.addr_y, flush_size_c, flush_type_c, ime_engine_operation_mode);
		//set_addr.addr_cr = set_addr.addr_y;//ime_output_dma_buf_flush(set_addr.addr_y, flush_size_c, flush_type_c, ime_engine_operation_mode);
		//er_return = ime_set_out_path_dma_addr_p4(&set_addr);
		//if (er_return != E_OK) {
		//  return er_return;
		//}

		ime_set_output_channel_addr0_p4_reg(set_addr.addr_y, set_addr.addr_y, set_addr.addr_y);

		ime_set_flip_enable_p4_reg(p_out_path_info->out_path_flip_enable);

		ime_clamp_p4_reg(p_out_path_info->out_path_clamp.min_y, p_out_path_info->out_path_clamp.max_y, p_out_path_info->out_path_clamp.min_uv, p_out_path_info->out_path_clamp.max_uv);

		ime_set_output_dram_enable_p4_reg((UINT32)p_out_path_info->out_path_dram_enable);
	}


	//ime_set_out_path_p4_enable(p_out_path_info->out_path_enable);
	ime_set_output_p4_enable_reg((UINT32)p_out_path_info->out_path_enable);

	// #2015/06/12# adas control flow bug
	if (p_out_path_info->out_path_enable == IME_FUNC_DISABLE) {
		//ime_set_adas_enable(IME_FUNC_DISABLE);
		ime_set_adas_enable_reg(IME_FUNC_DISABLE);
	}
	// #2015/06/12# end

	return E_OK;
}
//-------------------------------------------------------------------------------

#if 0
ER ime_set_out_path_p5(IME_OUTPATH_INFO *p_out_path_info)
{
	ER er_return = E_OK;
	UINT32 flush_size_y = 0x0, flush_size_c = 0x0;
	BOOL flush_type_y = FALSE, flush_type_c = FALSE;

	IME_SIZE_INFO get_size = {0x0};

	if (p_out_path_info == NULL) {
		DBG_ERR("IME: parameter NULL...\r\n");

		return E_PAR;
	}

	ime_set_out_path_p5_enable(p_out_path_info->out_path_enable);

	if (p_out_path_info->out_path_enable == IME_FUNC_ENABLE) {
		ime_set_out_path_dma_output_enable_p5(p_out_path_info->out_path_dram_enable);

		ime_setOutPath_ImageFormat_P5(p_out_path_info->out_path_image_format.out_format_type_sel, p_out_path_info->out_path_image_format.out_format_sel);
		ime_set_out_path_scale_method_p5(p_out_path_info->out_path_scale_method);

		er_return = ime_set_out_path_scale_size_p5(&(p_out_path_info->out_path_scale_size));
		if (er_return != E_OK) {
			return er_return;
		}

		er_return = ime_set_out_path_output_crop_size_p5(&(p_out_path_info->out_path_scale_size), &(p_out_path_info->out_path_out_size), &(p_out_path_info->out_path_crop_pos));
		if (er_return != E_OK) {
			return er_return;
		}

		er_return = ime_set_out_path_enhance_p5(&(p_out_path_info->out_path_scale_enh));
		if (er_return != E_OK) {
			return er_return;
		}

		//ime_get_in_path_image_size(&get_size);
		ime_get_input_image_size_reg(&(get_size.size_h), &(get_size.size_v));

		ime_scale_filter_params = p_out_path_info->out_path_scale_filter;
		if (ime_scale_filter_params.CoefCalMode == IME_SCALE_FILTER_COEF_AUTO_MODE) {
			ime_scale_filter_params.scale_h_filter_coef = ime_cal_scale_filter_coefs(get_size.size_h, p_out_path_info->out_path_scale_size.size_h);
			ime_scale_filter_params.scale_h_filter_enable = ((ime_scale_filter_params.scale_h_filter_coef == 0x0) ? IME_FUNC_DISABLE : IME_FUNC_ENABLE);
			ime_scale_filter_params.scale_v_filter_coef = ime_cal_scale_filter_coefs(get_size.size_v, p_out_path_info->out_path_scale_size.size_v);
			ime_scale_filter_params.scale_v_filter_enable = ((ime_scale_filter_params.scale_v_filter_coef == 0x0) ? IME_FUNC_DISABLE : IME_FUNC_ENABLE);
		}
		er_return = ime_set_out_path_scale_filter_p5(&ime_scale_filter_params);
		if (er_return != E_OK) {
			return er_return;
		}

		ime_scale_factor_path5_params = p_out_path_info->out_path_scale_factors;
		if (ime_scale_factor_path5_params.CalScaleFactorMode == IME_SCALE_FACTOR_COEF_AUTO_MODE) {
			er_return = ime_cal_image_scale_params(&get_size, &(p_out_path_info->out_path_scale_size), p_out_path_info->out_path_scale_method, &ime_scale_factor_path5_params);
			if (er_return != E_OK) {
				return er_return;
			}

			if (p_out_path_info->out_path_scale_method == IMEALG_INTEGRATION) {
				FLOAT scale_down_factor = 0.0;

				scale_down_factor = (FLOAT)(get_size.size_h - 1) / (FLOAT)(p_out_path_info->out_path_scale_size.size_h - 1);
				if (scale_down_factor <= 1.98) {
					ime_set_out_path_scale_method_p5(IMEALG_BICUBIC);
				}
			}
		}

		er_return = ime_set_out_path_scale_factor_p5(&ime_scale_factor_path5_params);
		if (er_return != E_OK) {
			return er_return;
		}

		ime_lofs_params = p_out_path_info->out_path_out_lineoffset;
		ime_output_buffer_addr = p_out_path_info->out_path_addr;
		switch (p_out_path_info->out_path_image_format.out_format_sel) {
		case IME_OUTPUT_YCC_444:
		case IME_OUTPUT_RGB_444:
			flush_size_y = ime_lofs_params.lineoffset_y * p_out_path_info->out_path_out_size.size_v;
			flush_size_c = ime_lofs_params.lineoffset_cb * p_out_path_info->out_path_out_size.size_v;

			flush_type_y = ((p_out_path_info->out_path_out_size.size_h == ime_lofs_params.lineoffset_y) ? TRUE : FALSE);
			flush_type_c = ((p_out_path_info->out_path_out_size.size_h == ime_lofs_params.lineoffset_cb) ? TRUE : FALSE);
			break;

		case IME_OUTPUT_YCC_422_COS:
		case IME_OUTPUT_YCC_422_CEN:
		case IME_OUTPUT_YCC_420_COS:
		case IME_OUTPUT_YCC_420_CEN:
			flush_size_y = ime_lofs_params.lineoffset_y * p_out_path_info->out_path_out_size.size_v;
			flush_size_c = ime_lofs_params.lineoffset_cb * p_out_path_info->out_path_out_size.size_v;

			if (p_out_path_info->out_path_image_format.out_format_type_sel == IME_OUTPUT_YCC_UVPACKIN) {
				ime_output_buffer_addr.addr_cr = ime_output_buffer_addr.addr_cb;
			}

			flush_type_y = ((p_out_path_info->out_path_out_size.size_h == ime_lofs_params.lineoffset_y) ? TRUE : FALSE);
			flush_type_c = ((p_out_path_info->out_path_out_size.size_h == ime_lofs_params.lineoffset_cb) ? TRUE : FALSE);
			break;

		case IME_OUTPUT_Y_ONLY:
			ime_lofs_params.lineoffset_cb = ime_lofs_params.lineoffset_y;

			flush_size_y = ime_lofs_params.lineoffset_y * p_out_path_info->out_path_out_size.size_v;
			flush_size_c = ime_lofs_params.lineoffset_y * p_out_path_info->out_path_out_size.size_v;

			ime_output_buffer_addr.addr_cr = ime_output_buffer_addr.addr_cb = ime_output_buffer_addr.addr_y;

			flush_type_y = ((p_out_path_info->out_path_out_size.size_h == ime_lofs_params.lineoffset_y) ? TRUE : FALSE);
			flush_type_c = ((p_out_path_info->out_path_out_size.size_h == ime_lofs_params.lineoffset_cb) ? TRUE : FALSE);
			break;

		default:
			break;
		}

		er_return = ime_set_out_path_lineoffset_p5(&ime_lofs_params);
		if (er_return != E_OK) {
			return er_return;
		}

		//ime_engine_operation_mode = ime_get_operate_mode();
		ime_output_buffer_addr.addr_y  = ime_output_dma_buf_flush(ime_output_buffer_addr.addr_y, flush_size_y, flush_type_y, ime_engine_operation_mode);
		ime_output_buffer_addr.addr_cb = ime_output_dma_buf_flush(ime_output_buffer_addr.addr_cb, flush_size_c, flush_type_c, ime_engine_operation_mode);
		ime_output_buffer_addr.addr_cr = ime_output_dma_buf_flush(ime_output_buffer_addr.addr_cr, flush_size_c, flush_type_c, ime_engine_operation_mode);
		er_return = ime_set_out_path_dma_addr_p5(&ime_output_buffer_addr);
		if (er_return != E_OK) {
			return er_return;
		}

#if 0
		ime_chg_stitching_enable(IME_PATH5_SEL, p_out_path_info->out_path_sprtout_enable);
		if (p_out_path_info->out_path_sprtout_enable == IME_FUNC_ENABLE) {
			ime_stitching_image_params.stitch_pos = p_out_path_info->out_path_sprt_pos;
			ime_stitching_image_params.stitch_lofs = p_out_path_info->out_path_out_lineoffset2;
			ime_stitching_image_params.lofs_update = TRUE;
			ime_stitching_image_params.stitch_dma_addr = p_out_path_info->out_path_addr2;
			ime_stitching_image_params.dma_addr_update = TRUE;
			er_return = ime_chg_stitching_image_param(IME_PATH5_SEL, &ime_stitching_image_params);
			if (er_return != E_OK) {
				return er_return;
			}
		}
#endif
	}

	return E_OK;
}
#endif

//------------------------------------------------------------------------
// stitching
#if 0
VOID ime_set_stitching_enable(IME_PATH_SEL path_sel, IME_FUNC_EN set_en)
{
	switch (path_sel) {
	case IME_PATH1_SEL:
		ime_set_stitching_enable_p1_reg((UINT32)set_en);
		break;

	case IME_PATH2_SEL:
		ime_set_stitching_enable_p2_reg((UINT32)set_en);
		break;

	case IME_PATH3_SEL:
		ime_set_stitching_enable_p3_reg((UINT32)set_en);
		break;

	//case IME_PATH5_SEL:
	//    ime_set_stitching_enable_p5_reg((UINT32)set_en);
	//break;

	case IME_PATH4_SEL:
	default:
		break;
	}
}
#endif
//------------------------------------------------------------------------
#if 0
VOID ime_set_stitching_base_position(IME_PATH_SEL path_sel, UINT32 set_pos)
{
	switch (path_sel) {
	case IME_PATH1_SEL:
		ime_set_stitching_base_position_p1_reg((UINT32)set_pos);
		break;

	case IME_PATH2_SEL:
		ime_set_stitching_base_position_p2_reg((UINT32)set_pos);
		break;

	case IME_PATH3_SEL:
		ime_set_stitching_base_position_p3_reg((UINT32)set_pos);
		break;

	//case IME_PATH5_SEL:
	//    ime_set_stitching_base_position_p5_reg((UINT32)set_pos);
	//break;

	case IME_PATH4_SEL:
	default:
		break;
	}
}
#endif
//------------------------------------------------------------------------
#if 0
VOID ime_set_stitching_lineoffset(IME_PATH_SEL path_sel, IME_LINEOFFSET_INFO *p_set_lofs)
{
	switch (path_sel) {
	case IME_PATH1_SEL:
		ime_set_stitching_output_lineoffset_y2_p1_reg(p_set_lofs->lineoffset_y);
		ime_set_stitching_output_lineoffset_c2_p1_reg(p_set_lofs->lineoffset_cb);
		break;

	case IME_PATH2_SEL:
		ime_set_stitching_output_lineoffset_y2_p2_reg(p_set_lofs->lineoffset_y);
		ime_set_stitching_output_lineoffset_c2_p2_reg(p_set_lofs->lineoffset_cb);
		break;

	case IME_PATH3_SEL:
		ime_set_stitching_output_lineoffset_y2_p3_reg(p_set_lofs->lineoffset_y);
		ime_set_stitching_output_lineoffset_c2_p3_reg(p_set_lofs->lineoffset_cb);
		break;

	//case IME_PATH5_SEL:
	//    ime_set_stitching_output_lineoffset_y2_p5_reg(p_set_lofs->lineoffset_y);
	//    ime_set_stitching_output_lineoffset_c2_p5_reg(p_set_lofs->lineoffset_cb);
	//break;

	case IME_PATH4_SEL:
	default:
		break;
	}
}
#endif
//------------------------------------------------------------------------

#if 0
VOID ime_set_stitching_dma_addr(IME_PATH_SEL path_sel, IME_DMA_ADDR_INFO *p_set_addr)
{
	switch (path_sel) {
	case IME_PATH1_SEL:
		ime_set_stitching_output_channel_addr2_p1_reg(p_set_addr->addr_y, p_set_addr->addr_cb, p_set_addr->addr_cr);
		break;

	case IME_PATH2_SEL:
		ime_set_stitching_output_channel_addr2_p2_reg(p_set_addr->addr_y, p_set_addr->addr_cb, p_set_addr->addr_cr);
		break;

	case IME_PATH3_SEL:
		ime_set_stitching_output_channel_addr2_p3_reg(p_set_addr->addr_y, p_set_addr->addr_cb, p_set_addr->addr_cr);
		break;

	//case IME_PATH5_SEL:
	//    ime_set_stitching_output_channel_addr2_p5_reg(p_set_addr->addr_y, p_set_addr->addr_cb, p_set_addr->addr_cr);
	//break;

	case IME_PATH4_SEL:
	default:
		break;
	}
}
#endif
//------------------------------------------------------------------------

#if 0
ER ime_check_stitching(IME_PATH_SEL path_sel)
{
	//ER er_return = E_OK;

	if (path_sel == IME_PATH4_SEL) {
		DBG_ERR("IME: path4, do not support stitching...\r\n");

		return E_PAR;
	}

#if 0 // removed from nt96680
	er_return = ime_check_stripe_mode_stitching(path_sel);
	if (er_return != E_OK) {
		DBG_ERR("IME: stitching only for single stripe mode...\r\n");
		return E_PAR;
	}
#endif

	return E_OK;
}
#endif
//------------------------------------------------------------------------


//------------------------------------------------------------------------


VOID ime_set_in_path_3dnr_burst_size(IME_BURST_SEL bst_y, IME_BURST_SEL bst_c, IME_BURST_SEL bst_mv, IME_BURST_SEL bst_ms)
{
	// reserved
	UINT32 set_bst_y = 0, set_bst_u = 0, set_bst_mv = 0, set_bst_ms = 0;

	if (bst_y == IME_BURST_64W) {
		set_bst_y = 0;
	} else if (bst_y == IME_BURST_32W) {
		set_bst_y = 1;
	} else {
		// do nothing...
	}

	if (bst_c == IME_BURST_64W) {
		set_bst_u = 0;
	} else if (bst_c == IME_BURST_32W) {
		set_bst_u = 1;
	} else {
		// do nothing...
	}

	if (bst_mv == IME_BURST_32W) {
		set_bst_mv = 0;
	} else if (bst_mv == IME_BURST_16W) {
		set_bst_mv = 1;
	} else {
		// do nothing...
	}

	if (bst_ms == IME_BURST_32W) {
		set_bst_ms = 0;
	} else if (bst_ms == IME_BURST_16W) {
		set_bst_ms = 1;
	} else {
		// do nothing...
	}

	ime_set_input_path_tmnr_burst_size_reg(set_bst_y, set_bst_u, set_bst_mv, set_bst_ms);
}
//-------------------------------------------------------------------------------

VOID ime_set_out_path_3dnr_burst_size(IME_BURST_SEL BstY, IME_BURST_SEL BstC, IME_BURST_SEL BstMV, IME_BURST_SEL BstMO, IME_BURST_SEL BstMOROI, IME_BURST_SEL BstSta)
{
	// reserved
	UINT32 set_bst_y = 0, set_bst_u = 0, set_bst_mv = 0, set_bst_sta = 0;
	UINT32 set_bst_mo = 0, set_bst_mo_roi = 0;

	if (BstY == IME_BURST_32W) {
		set_bst_y = 0;
	} else if (BstY == IME_BURST_16W) {
		set_bst_y = 1;
	} else if (BstY == IME_BURST_48W) {
		set_bst_y = 2;
	} else if (BstY == IME_BURST_64W) {
		set_bst_y = 3;
	} else {
		// do nothing...
	}

	if (BstC == IME_BURST_32W) {
		set_bst_u = 0;
	} else if (BstC == IME_BURST_16W) {
		set_bst_u = 1;
	} else if (BstC == IME_BURST_48W) {
		set_bst_u = 2;
	} else if (BstC == IME_BURST_64W) {
		set_bst_u = 3;
	} else {
		// do nothing...
	}

	if (BstMV == IME_BURST_32W) {
		set_bst_mv = 0;
	} else if (BstMV == IME_BURST_16W) {
		set_bst_mv = 1;
	} else {
		// do nothing...
	}

	if (BstMO == IME_BURST_32W) {
		set_bst_mo = 0;
	} else if (BstMO == IME_BURST_16W) {
		set_bst_mo = 1;
	} else {
		// do nothing...
	}

	if (BstMOROI == IME_BURST_32W) {
		set_bst_mo_roi = 0;
	} else if (BstMOROI == IME_BURST_16W) {
		set_bst_mo_roi = 1;
	} else {
		// do nothing...
	}

	if (BstSta == IME_BURST_32W) {
		set_bst_sta = 0;
	} else if (BstSta == IME_BURST_16W) {
		set_bst_sta = 1;
	} else {
		// do nothing...
	}

	ime_set_output_path_tmnr_burst_size_reg(set_bst_y, set_bst_u, set_bst_mv, set_bst_mo, set_bst_mo_roi, set_bst_sta);
}

//-------------------------------------------------------------------------------


//VOID ime_get_debug_message(VOID)
//{
//	ime_show_debug_message_reg();
//}
//------------------------------------------------------------------------

INT32 ime_get_burst_size_info(UINT32 chl_num)
{
	return ime_get_burst_size_reg(chl_num);
}
//------------------------------------------------------------------------





