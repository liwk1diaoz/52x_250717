/**
    @file      kdrv_dce.c
    @ingroup    Predefined_group_name

    @brief    dce device abstraction layer

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "dce_platform.h"
#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kdrv_dce_int.h"
#include "comm/hwclock.h"

static DCE_MODE_INFO g_dce_TotalInfo = {0};
static INT32 g_kdrv_dce_open_count = 0;
static DCE_OPENOBJ g_dce_open_cfg;
static UINT32 g_kdrv_dce_init_flg = 0;
static KDRV_DCE_HANDLE *g_kdrv_dce_trig_hdl = NULL;
static UINT32 *gp_2dlut_buffer = NULL;
static UINT32 *gp_ori_2dlut_buffer = NULL;
DCE_NORMSTP_RSLT g_dce_strip_result;
BOOL b_kdrv_dce_cal_strip_flow = FALSE;
KDRV_IPP_ISRCB g_dce_isrcb_fp;

#if 0
KDRV_DCE_PRAM g_kdrv_dce_info[KDRV_DCE_HANDLE_MAX_NUM];
static KDRV_DCE_HANDLE g_kdrv_dce_handle_tab[KDRV_DCE_HANDLE_MAX_NUM];
static UINT32 g_kdrv_dce_ipl_ctrl_en[KDRV_DCE_HANDLE_MAX_NUM] = {0};
static DCE_DIST_NORM g_kdrv_dce_norm[KDRV_DCE_HANDLE_MAX_NUM];
DCE_STP_MODE g_kdrv_dce_stripe_mode[KDRV_DCE_HANDLE_MAX_NUM] = {0};
static BOOL g_kdrv_dce_load_flg[KDRV_DCE_HANDLE_MAX_NUM][KDRV_DCE_PARAM_MAX] = {0};
#else
KDRV_DCE_PRAM *g_kdrv_dce_info = NULL;
static KDRV_DCE_HANDLE *g_kdrv_dce_handle_tab = NULL;
static UINT32 *g_kdrv_dce_ipl_ctrl_en = NULL;
static UINT32 *g_kdrv_dce_iq_ctrl_en = NULL;
static DCE_DIST_NORM *g_kdrv_dce_norm = NULL;
static DCE_STP_MODE *g_kdrv_dce_stripe_mode = NULL;
static BOOL **g_kdrv_dce_load_flg = NULL;

UINT32 kdrv_dce_channel_num = 0;
UINT32 kdrv_dce_lock_chls = 0;
#endif

#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)
//buffer allocate handle
#if defined __UITRON || defined __ECOS || defined __FREERTOS
#else
static void *handle_2dlut_buff = NULL;
#endif
#endif

static UINT32 cfa_freq_lut[KDRV_CFA_FREQ_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8, 8, 8, 8, 15};
static UINT32 cfa_luma_lut[KDRV_CFA_LUMA_NUM] = {128, 120, 95, 72, 58, 48, 40, 36, 32, 32, 32, 32, 32, 32, 32, 32, 32};
static UINT32 cfa_fcs_lut[KDRV_CFA_FCS_NUM] = {15, 15, 7, 4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0};
static UINT32 wdr_inbld_lut[WDR_INPUT_BLD_NUM] = {255, 255, 224, 192, 160, 128, 96, 64, 32, 64, 96, 128, 160, 192, 224, 255, 255};
static UINT32 wdr_tonecurve_idx[WDR_NEQ_TABLE_IDX_NUM] = {0, 8, 16, 24, 28, 32, 36, 38, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};
static UINT32 wdr_tonecurve_split[WDR_NEQ_TABLE_IDX_NUM] = {3, 3, 3, 2, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static UINT32 wdr_tonecurve_val[WDR_NEQ_TABLE_VAL_NUM] = {
	0, 65, 128, 189, 248, 304, 359, 413, 464, 514,
	563, 611, 657, 701, 745, 788, 829, 870, 910, 948,
	986, 1023, 1060, 1095, 1130, 1197, 1262, 1325, 1385, 1444,
	1500, 1555, 1607, 1659, 1709, 1757, 1804, 1894, 1980, 2062,
	2140, 2286, 2421, 2545, 2662, 2770, 2873, 2969, 3061, 3147,
	3230, 3308, 3384, 3456, 3525, 3591, 3655, 3717, 3776, 3834,
	3890, 3943, 3996, 4046, 4095
};
static UINT32 wdr_outbld_idx[WDR_NEQ_TABLE_IDX_NUM] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 15, 23, 31, 39, 43, 47, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};
static UINT32 wdr_outbld_split[WDR_NEQ_TABLE_IDX_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static UINT32 wdr_outbld_val[WDR_NEQ_TABLE_VAL_NUM] = {
	4059, 4043, 4023, 3995, 3959, 3913, 3855, 3784, 3698, 3599,
	3545, 3487, 3457, 3427, 3396, 3365, 3349, 3333, 3317, 3301,
	3285, 3269, 3253, 3236, 3220, 3203, 3187, 3171, 3154, 3138,
	3121, 3105, 3088, 3072, 3056, 3040, 3023, 3007, 2991, 2975,
	2944, 2912, 2882, 2851, 2822, 2793, 2764, 2736, 2683, 2632,
	2540, 2459, 2391, 2332, 2283, 2243, 2209, 2181, 2158, 2139,
	2124, 2111, 2100, 2092, 2084
};
static UINT32 kdrv_dce_gdc_sstp_maxsize = 0;
#if NEW_STRIPE_RULE
static UINT32 kdrv_dce_gdc_3stp_maxsize = 0;
static UINT32 kdrv_dce_gdc_4stp_maxsize = 0;
static UINT32 kdrv_dce_gdc_6stp_maxsize = 0;
#endif
static UINT32 kdrv_dce_low_lat_sstp_maxsize = 0;
static UINT32 kdrv_dce_2dlut_sstp_maxsize = 0;
static UINT32 kdrv_dce_2stp_maxsize = 0;
static UINT32 kdrv_dce_3stp_maxsize = 0;

static UINT64 kdrv_dce_do_64b_div(UINT64 dividend, UINT64 divisor)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	dividend = (dividend / divisor);
#else
	do_div(dividend, divisor);
#endif
	return dividend;
}

static DCE_DIST_NORM g_kdrv_dce_norm_calculation(UINT32 id)
{
	UINT64 img_w = (UINT64)g_kdrv_dce_info[id].in_img_info.img_size_h;
	UINT64 img_h = (UINT64)g_kdrv_dce_info[id].in_img_info.img_size_v;
	UINT64 cen_x = (UINT64)g_kdrv_dce_info[id].gdc_center.geo_center_x;
	UINT64 cen_y = (UINT64)g_kdrv_dce_info[id].gdc_center.geo_center_y;
	UINT64 dist_x = 0, dist_sqr_x = 0, dist_y = 0, dist_sqr_y = 0;
	UINT64 GDCi = 0, GDCtmp = 0;
	UINT64 TmpX = 0, TmpY = 0;

	dist_x = kdrv_dce_do_64b_div((((cen_x > (img_w - cen_x)) ? cen_x : (img_w - cen_x)) * 120), 100);
	dist_y = kdrv_dce_do_64b_div((((cen_y > (img_h - cen_y)) ? cen_y : (img_h - cen_y)) * 120), 100);

	dist_sqr_x = dist_x * dist_x;
	dist_sqr_y = dist_y * dist_y;

	TmpX = (dist_sqr_x * ((UINT64)g_kdrv_dce_info[id].dc_cac_param.dc.geo_dist_x + 1)) >> 12;
	TmpY = (dist_sqr_y * ((UINT64)g_kdrv_dce_info[id].dc_cac_param.dc.geo_dist_y + 1)) >> 12;

	GDCtmp = TmpX + TmpY;

	while (((UINT64)1 << GDCi) < GDCtmp) {
		GDCi++;
	}

	g_kdrv_dce_norm[id].nrm_bit = (UINT32)GDCi;
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	g_kdrv_dce_norm[id].nrm_fact = (UINT32)((((UINT64)1 << (GDCi + 7))) / GDCtmp);
	g_kdrv_dce_norm[id].nrm_fact_10b = (UINT32)((((UINT64)1 << (GDCi + 9))) / GDCtmp);
#else
	{
		UINT64 mod, x;
		x = (((UINT64)1 << (GDCi + 7)));
		mod = do_div(x, GDCtmp);
		g_kdrv_dce_norm[id].nrm_fact = (UINT32)(x);
		x = (((UINT64)1 << (GDCi + 9)));
		mod = do_div(x, GDCtmp);
		g_kdrv_dce_norm[id].nrm_fact_10b = (UINT32)(x);
	}
#endif
	if (g_kdrv_dce_norm[id].nrm_fact > 255) {
		g_kdrv_dce_norm[id].nrm_fact = 255;
	}
	if (g_kdrv_dce_norm[id].nrm_fact_10b > 1023) {
		g_kdrv_dce_norm[id].nrm_fact_10b = 1023;
	}

	return g_kdrv_dce_norm[id];
}

static void kdrv_dce_memset_drv_param(void)
{
	memset((void *)&g_dce_TotalInfo, 0x0, sizeof(DCE_MODE_INFO));
}

static ER kdrv_dce_check_param(INT32 id)
{
	//check enable
	if (!g_kdrv_dce_info[id].dc_cac_param.dc_enable && g_kdrv_dce_info[id].dc_cac_param.cac.cac_enable) {
		DBG_ERR("CAC is enabled but DC is not!\r\n");
		return E_PAR;
	}

	//check subimg size
	if (g_kdrv_dce_info[id].wdr_subout_addr.enable || g_kdrv_dce_info[id].wdr_param.wdr_enable) {
		if (g_kdrv_dce_info[id].wdr_subimg_param.subimg_size_h < 4) {
			g_kdrv_dce_info[id].wdr_subimg_param.subimg_size_h = 16;
			DBG_WRN("subimg size h cannot be smaller than 4, modify to 16\r\n");
		}
		if (g_kdrv_dce_info[id].wdr_subimg_param.subimg_size_v < 4) {
			g_kdrv_dce_info[id].wdr_subimg_param.subimg_size_v = 9;
			DBG_WRN("subimg size v cannot be smaller than 4, modify to 9\r\n");
		}
	}
	if ((g_kdrv_dce_info[id].wdr_param.wdr_enable)  && (g_kdrv_dce_info[id].wdr_subimg_param.subimg_lofs_in == 0)) {
		g_kdrv_dce_info[id].wdr_subimg_param.subimg_lofs_in = g_kdrv_dce_info[id].wdr_subimg_param.subimg_size_h  * 8;
		DBG_WRN("WDR is enabled but sub-image input line offset is not assigned!\r\n");
		DBG_WRN("Assign a value of sub-image width * 8.\r\n");
	}
	if ((g_kdrv_dce_info[id].wdr_subout_addr.enable == 1) && (g_kdrv_dce_info[id].wdr_subimg_param.subimg_lofs_out == 0)) {
		g_kdrv_dce_info[id].wdr_subimg_param.subimg_lofs_out = g_kdrv_dce_info[id].wdr_subimg_param.subimg_size_h * 8;
		DBG_WRN("WDR sub-out is enabled but sub-image output line offset is not assigned!\r\n");
		DBG_WRN("Assign a value of sub-image width * 8.\r\n");
	}

	return E_OK;
}

static ER kdrv_dce_set_dram_single_mode_info(UINT32 id,  DCE_MODE_INFO *p_dce)
{
	ER rt = E_OK;

	p_dce->dram_out_mode = g_kdrv_dce_info[id].dram_out_mode;
	p_dce->single_out_en.single_out_wdr_en = g_kdrv_dce_info[id].single_out_ch.single_out_wdr_en;
	p_dce->single_out_en.single_out_cfa_en = g_kdrv_dce_info[id].single_out_ch.single_out_cfa_en;

	return rt;
}

static ER kdrv_dce_set_dma_in_info(UINT32 id,  DCE_MODE_INFO *p_dce)
{
	ER rt = E_OK;

	p_dce->input_info.wdr_in_en = g_kdrv_dce_info[id].wdr_param.wdr_enable;
	p_dce->input_info.wdr_subin_addr = g_kdrv_dce_info[id].wdr_subin_addr.addr;
	p_dce->input_info.wdr_subin_lofs = g_kdrv_dce_info[id].wdr_subimg_param.subimg_lofs_in;

	return rt;
}

static ER kdrv_dce_set_dma_out_info(UINT32 id,  DCE_MODE_INFO *p_dce)
{
	ER rt = E_OK;

	p_dce->output_info.wdr_out_en = g_kdrv_dce_info[id].wdr_subout_addr.enable;
	if (g_kdrv_dce_info[id].wdr_subout_addr.enable &&
		g_kdrv_dce_info[id].wdr_subout_addr.addr != 0 &&
		g_kdrv_dce_info[id].wdr_subimg_param.subimg_lofs_out != 0) {
		p_dce->output_info.wdr_subout_addr = g_kdrv_dce_info[id].wdr_subout_addr.addr;
		p_dce->output_info.wdr_subout_lofs = g_kdrv_dce_info[id].wdr_subimg_param.subimg_lofs_out;

		p_dce->func_en_msk |= (DCE_FUNC_WDR_SUBOUT);
	} else {
		p_dce->func_en_msk &= (~DCE_FUNC_WDR_SUBOUT);
	}

	p_dce->output_info.cfa_out_en = g_kdrv_dce_info[id].cfa_subout_info.cfa_subout_enable;
	if (g_kdrv_dce_info[id].cfa_subout_info.cfa_subout_enable &&
		g_kdrv_dce_info[id].cfa_subout_info.cfa_addr != 0 &&
		g_kdrv_dce_info[id].cfa_subout_info.cfa_lofs != 0) {
		p_dce->output_info.cfa_subout_addr = g_kdrv_dce_info[id].cfa_subout_info.cfa_addr;
		p_dce->output_info.cfa_subout_lofs = g_kdrv_dce_info[id].cfa_subout_info.cfa_lofs;

		p_dce->func_en_msk |= DCE_FUNC_CFA_SUBOUT;

		if (g_kdrv_dce_info[id].cfa_subout_info.cfa_subout_flip_enable) {
			p_dce->func_en_msk |= DCE_FUNC_CFA_SUBOUT_FLIP;
		} else {
			p_dce->func_en_msk &= (~DCE_FUNC_CFA_SUBOUT_FLIP);
		}
	} else {
		p_dce->func_en_msk &= (~(DCE_FUNC_CFA_SUBOUT | DCE_FUNC_CFA_SUBOUT_FLIP));
	}

	return rt;
}


static ER kdrv_dce_set_in_out_info(UINT32 id,  DCE_MODE_INFO *p_dce)
{
	ER rt = E_OK;

	//dce input
	if (g_kdrv_dce_info[id].in_info.in_src == KDRV_DCE_IN_SRC_DRAM ||
		g_kdrv_dce_info[id].in_info.in_src == KDRV_DCE_IN_SRC_DCE_IME) {

		if (g_kdrv_dce_info[id].in_info.in_src == KDRV_DCE_IN_SRC_DRAM) {
			p_dce->op_param.op_mode = DCE_OPMODE_DCE_D2D;
			p_dce->d2d_param.yuv2rgb_fmt = 0;
			p_dce->func_en_msk &= (~(DCE_FUNC_YUV2RGB));
		} else {
			p_dce->op_param.op_mode = DCE_OPMODE_DCE_IME;
			p_dce->d2d_param.yuv2rgb_fmt = (g_kdrv_dce_info[id].in_info.yuvfmt == KDRV_DCE_YUV2RGB_FMT_FULL) ? KDRV_DCE_YUV2RGB_FMT_FULL : KDRV_DCE_YUV2RGB_FMT_BT601;

			p_dce->func_en_msk |= (DCE_FUNC_YUV2RGB);
		}

		p_dce->d2d_param.d2d_fmt = g_kdrv_dce_info[id].in_info.type;

		p_dce->d2d_param.d2d_rand_en = 0;

		p_dce->d2d_param.y_in_addr =  g_kdrv_dce_info[id].in_dma_info.addr_y;
		p_dce->d2d_param.y_in_lofs =  g_kdrv_dce_info[id].in_img_info.lineofst[KDRV_DCE_YUV_Y];
		p_dce->d2d_param.uv_in_addr =  g_kdrv_dce_info[id].in_dma_info.addr_cbcr;
		p_dce->d2d_param.uv_in_lofs =  g_kdrv_dce_info[id].in_img_info.lineofst[KDRV_DCE_YUV_UV];

		p_dce->d2d_param.y_out_addr =  g_kdrv_dce_info[id].out_dma_info.addr_y;
		p_dce->d2d_param.y_out_lofs =  g_kdrv_dce_info[id].out_img_info.lineofst[KDRV_DCE_YUV_Y];
		p_dce->d2d_param.uv_out_addr =  g_kdrv_dce_info[id].out_dma_info.addr_cbcr;
		p_dce->d2d_param.uv_out_lofs =  g_kdrv_dce_info[id].out_img_info.lineofst[KDRV_DCE_YUV_UV];

	} else if (g_kdrv_dce_info[id].in_info.in_src == KDRV_DCE_IN_SRC_DIRECT) {
		p_dce->op_param.op_mode = DCE_OPMODE_IFE_IME;
		p_dce->func_en_msk &= (~(DCE_FUNC_YUV2RGB));
	} else {
		p_dce->op_param.op_mode = DCE_OPMODE_SIE_IME;
		p_dce->func_en_msk &= (~(DCE_FUNC_YUV2RGB));
	}

	p_dce->img_in_size.h_size = g_kdrv_dce_info[id].in_img_info.img_size_h;
	p_dce->img_in_size.v_size = g_kdrv_dce_info[id].in_img_info.img_size_v;

	p_dce->img_out_crop.h_size = g_kdrv_dce_info[id].out_img_info.crop_size_h;
	p_dce->img_out_crop.v_size = g_kdrv_dce_info[id].out_img_info.crop_size_v;
	p_dce->img_out_crop.h_start = g_kdrv_dce_info[id].out_img_info.crop_start_h;
	p_dce->img_out_crop.v_start = g_kdrv_dce_info[id].out_img_info.crop_start_v;

	if ((p_dce->img_in_size.h_size != p_dce->img_out_crop.h_size) || (p_dce->img_in_size.v_size != p_dce->img_out_crop.v_size)) {
		p_dce->func_en_msk |= DCE_FUNC_CROP;
	} else {
		p_dce->func_en_msk &= ~DCE_FUNC_CROP;
	}
	return rt;
}

static ER kdrv_dce_set_iq_wdr(UINT32 id,  DCE_MODE_INFO *p_dce)
{
	ER rt = E_OK;

	p_dce->wdr_param.wdr_input_bld.wdr_bld_sel = g_kdrv_dce_info[id].wdr_param.input_bld.bld_sel;
	p_dce->wdr_param.wdr_input_bld.wdr_bld_wt = g_kdrv_dce_info[id].wdr_param.input_bld.bld_wt;
	p_dce->wdr_param.wdr_input_bld.p_inblend_lut = g_kdrv_dce_info[id].wdr_param.input_bld.blend_lut;

	if (g_kdrv_dce_info[id].wdr_param.wdr_enable || g_kdrv_dce_info[id].wdr_param.tonecurve_enable) {

		if (g_kdrv_dce_info[id].wdr_param.wdr_enable) {
			p_dce->wdr_param.p_ftrcoef = g_kdrv_dce_info[id].wdr_param.ftrcoef;
			p_dce->wdr_param.wdr_strength.p_wdr_coeff = g_kdrv_dce_info[id].wdr_param.wdr_str.wdr_coeff;
			p_dce->wdr_param.wdr_strength.contrast = g_kdrv_dce_info[id].wdr_param.wdr_str.contrast;
			p_dce->wdr_param.wdr_strength.strength = g_kdrv_dce_info[id].wdr_param.wdr_str.strength;

			p_dce->wdr_param.wdr_gainctrl.gainctrl_en = g_kdrv_dce_info[id].wdr_param.gainctrl.gainctrl_en;
			p_dce->wdr_param.wdr_gainctrl.max_gain = g_kdrv_dce_info[id].wdr_param.gainctrl.max_gain;
			p_dce->wdr_param.wdr_gainctrl.min_gain = g_kdrv_dce_info[id].wdr_param.gainctrl.min_gain;

			p_dce->wdr_param.wdr_outbld.outbld_en = g_kdrv_dce_info[id].wdr_param.outbld.outbld_en;
			p_dce->wdr_param.wdr_outbld.outbld_lut.p_lut_idx = g_kdrv_dce_info[id].wdr_param.outbld.outbld_lut.lut_idx;
			p_dce->wdr_param.wdr_outbld.outbld_lut.p_lut_split = g_kdrv_dce_info[id].wdr_param.outbld.outbld_lut.lut_split;
			p_dce->wdr_param.wdr_outbld.outbld_lut.p_lut_val = g_kdrv_dce_info[id].wdr_param.outbld.outbld_lut.lut_val;

			p_dce->func_en_msk |= (DCE_FUNC_WDR);
		} else {
			p_dce->func_en_msk &= (~(DCE_FUNC_WDR));
		}

		if (g_kdrv_dce_info[id].wdr_param.tonecurve_enable) {
			p_dce->wdr_param.wdr_tonecurve.p_lut_idx = g_kdrv_dce_info[id].wdr_param.tonecurve.lut_idx;
			p_dce->wdr_param.wdr_tonecurve.p_lut_split = g_kdrv_dce_info[id].wdr_param.tonecurve.lut_split;
			p_dce->wdr_param.wdr_tonecurve.p_lut_val = g_kdrv_dce_info[id].wdr_param.tonecurve.lut_val;

			p_dce->func_en_msk |= DCE_FUNC_TCURVE;
		} else {
			p_dce->func_en_msk &= (~(DCE_FUNC_TCURVE));
		}

		p_dce->wdr_param.wdr_sat_reduct.sat_delta = g_kdrv_dce_info[id].wdr_param.sat_reduct.sat_delta;
		p_dce->wdr_param.wdr_sat_reduct.sat_th = g_kdrv_dce_info[id].wdr_param.sat_reduct.sat_th;
		p_dce->wdr_param.wdr_sat_reduct.sat_wt_low = g_kdrv_dce_info[id].wdr_param.sat_reduct.sat_wt_low;

	} else {
		p_dce->func_en_msk &= (~(DCE_FUNC_TCURVE | DCE_FUNC_WDR));
	}

	p_dce->wdr_subimg_param.input_size.h_size = g_kdrv_dce_info[id].in_img_info.img_size_h;
	p_dce->wdr_subimg_param.input_size.v_size = g_kdrv_dce_info[id].in_img_info.img_size_v;
	p_dce->wdr_subimg_param.wdr_subimg_size.h_size = g_kdrv_dce_info[id].wdr_subimg_param.subimg_size_h;
	p_dce->wdr_subimg_param.wdr_subimg_size.v_size = g_kdrv_dce_info[id].wdr_subimg_param.subimg_size_v;
	p_dce->wdr_subimg_param.subimg_param_mode = DCE_PARAM_AUTO_MODE;

	p_dce->wdr_dither.wdr_rand_rst = 0;
	p_dce->wdr_dither.wdr_rand_sel = DCE_WDR_ROUNDING;

	return rt;
}

static ER kdrv_dce_set_iq_hist(UINT32 id,  DCE_MODE_INFO *p_dce)
{
	ER rt = E_OK;

	if (g_kdrv_dce_info[id].hist_param.hist_enable) {
		p_dce->hist_param.hist_sel = g_kdrv_dce_info[id].hist_param.hist_sel;
		p_dce->hist_param.hist_step_h = g_kdrv_dce_info[id].hist_param.step_h;
		p_dce->hist_param.hist_step_v = g_kdrv_dce_info[id].hist_param.step_v;

		p_dce->func_en_msk |= DCE_FUNC_HISTOGRAM;
	} else {
		p_dce->func_en_msk &= (~DCE_FUNC_HISTOGRAM);
	}

	return rt;
}

static ER kdrv_dce_set_iq_cfa_subout(UINT32 id,  DCE_MODE_INFO *p_dce)
{
	ER rt = E_OK;
	if (g_kdrv_dce_info[id].cfa_subout_info.cfa_subout_enable) {
		p_dce->cfa_subout_param.subout_ch_sel = g_kdrv_dce_info[id].cfa_subout_info.subout_ch_sel;
		p_dce->cfa_subout_param.subout_byte = g_kdrv_dce_info[id].cfa_subout_info.subout_byte;
		p_dce->cfa_subout_param.subout_shiftbit = g_kdrv_dce_info[id].cfa_subout_info.subout_shiftbit;
	}

	return rt;
}

static ER kdrv_dce_set_iq_cfa(UINT32 id,  DCE_MODE_INFO *p_dce)
{
	ER rt = E_OK;

	if (g_kdrv_dce_info[id].cfa_param.cfa_enable) {
		if (g_kdrv_dce_info[id].in_img_info.cfa_pat < KDRV_IPP_RGGB_PIX_MAX) {
			p_dce->cfa_param.raw_fmt = (DCE_RAW_FMT)DCE_BAYER_2X2;
			p_dce->cfa_param.cfa_pat = (DCE_CFA_PAT)g_kdrv_dce_info[id].in_img_info.cfa_pat;
		} else if (g_kdrv_dce_info[id].in_img_info.cfa_pat > KDRV_IPP_RGGB_PIX_MAX && g_kdrv_dce_info[id].in_img_info.cfa_pat < KDRV_IPP_RGBIR_PIX_MAX) {
			p_dce->cfa_param.raw_fmt = (DCE_RAW_FMT)DCE_RGBIR_4X4;
			p_dce->cfa_param.cfa_pat = (DCE_CFA_PAT)(g_kdrv_dce_info[id].in_img_info.cfa_pat - (KDRV_IPP_RGGB_PIX_MAX + 1));
		} else {
			DBG_ERR("CFA pattern %d is unavailable!\r\n", (int)g_kdrv_dce_info[id].in_img_info.cfa_pat);
			return E_PAR;
		}

		p_dce->cfa_param.cfa_interp.edge_dth = g_kdrv_dce_info[id].cfa_param.cfa_interp.edge_dth;
		p_dce->cfa_param.cfa_interp.edge_dth2 = g_kdrv_dce_info[id].cfa_param.cfa_interp.edge_dth2;
		p_dce->cfa_param.cfa_interp.freq_th = g_kdrv_dce_info[id].cfa_param.cfa_interp.freq_th;
		p_dce->cfa_param.cfa_interp.p_freq_lut = g_kdrv_dce_info[id].cfa_param.cfa_interp.freq_lut;
		p_dce->cfa_param.cfa_interp.p_luma_wt = g_kdrv_dce_info[id].cfa_param.cfa_interp.luma_wt;

		p_dce->cfa_param.cfa_corr.rb_corr_en = g_kdrv_dce_info[id].cfa_param.cfa_correction.rb_corr_enable;
		p_dce->cfa_param.cfa_corr.rb_corr_th1 = g_kdrv_dce_info[id].cfa_param.cfa_correction.rb_corr_th1;
		p_dce->cfa_param.cfa_corr.rb_corr_th2 = g_kdrv_dce_info[id].cfa_param.cfa_correction.rb_corr_th2;

		p_dce->cfa_param.cfa_fcs.fcs_dirsel = g_kdrv_dce_info[id].cfa_param.cfa_fcs.fcs_dirsel;
		p_dce->cfa_param.cfa_fcs.fcs_coring = g_kdrv_dce_info[id].cfa_param.cfa_fcs.fcs_coring;
		p_dce->cfa_param.cfa_fcs.fcs_weight = g_kdrv_dce_info[id].cfa_param.cfa_fcs.fcs_weight;
		p_dce->cfa_param.cfa_fcs.p_fcs_strength = g_kdrv_dce_info[id].cfa_param.cfa_fcs.fcs_strength;

		p_dce->cfa_param.cfa_ir_hfc.ir_cl_check_en = g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.cl_check_enable;
		p_dce->cfa_param.cfa_ir_hfc.ir_hf_check_en = g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.hf_check_enable;
		p_dce->cfa_param.cfa_ir_hfc.ir_average_mode = g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.average_mode;
		p_dce->cfa_param.cfa_ir_hfc.ir_cl_sel = g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.cl_sel;
		p_dce->cfa_param.cfa_ir_hfc.ir_cl_th = g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.cl_th;
		p_dce->cfa_param.cfa_ir_hfc.ir_hf_gth = g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.hfg_th;
		p_dce->cfa_param.cfa_ir_hfc.ir_hf_diff = g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.hf_diff;
		p_dce->cfa_param.cfa_ir_hfc.ir_hf_eth = g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.hfe_th;
		p_dce->cfa_param.cfa_ir_hfc.ir_g_edge_th = g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.ir_g_edge_th;
		p_dce->cfa_param.cfa_ir_hfc.ir_rb_cstrength = g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.ir_rb_cstrength;

		p_dce->cfa_param.cfa_ir_sub.ir_sub_r = g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_r;
		p_dce->cfa_param.cfa_ir_sub.ir_sub_g = g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_g;
		p_dce->cfa_param.cfa_ir_sub.ir_sub_b = g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_b;
		p_dce->cfa_param.cfa_ir_sub.ir_sub_wt_lb = g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_wt_lb;
		p_dce->cfa_param.cfa_ir_sub.ir_sub_th = g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_th;
		p_dce->cfa_param.cfa_ir_sub.ir_sub_range = g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_range;

		//if (p_dce->cfa_param.raw_fmt == DCE_BAYER_2X2) {
		//  p_dce->cfa_param.cfa_ir_sub.ir_sat_gain = 256;
		//} else {
		p_dce->cfa_param.cfa_ir_sub.ir_sat_gain = g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sat_gain;
		//}

		if (g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_en) {
			p_dce->func_en_msk |= DCE_FUNC_CFA_PINKR;
		}
		p_dce->cfa_param.cfa_pink_reduc.pink_rd_mode = g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_mode;
		p_dce->cfa_param.cfa_pink_reduc.pink_rd_th1 = g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_th1;
		p_dce->cfa_param.cfa_pink_reduc.pink_rd_th2 = g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_th2;
		p_dce->cfa_param.cfa_pink_reduc.pink_rd_th3 = g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_th3;
		p_dce->cfa_param.cfa_pink_reduc.pink_rd_th4 = g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_th4;

		p_dce->cfa_param.cfa_cgain.gain_range = g_kdrv_dce_info[id].cfa_param.cfa_cgain.gain_range;
		p_dce->cfa_param.cfa_cgain.gain_r = g_kdrv_dce_info[id].cfa_param.cfa_cgain.r_gain;
		p_dce->cfa_param.cfa_cgain.gain_g = g_kdrv_dce_info[id].cfa_param.cfa_cgain.g_gain;
		p_dce->cfa_param.cfa_cgain.gain_b = g_kdrv_dce_info[id].cfa_param.cfa_cgain.b_gain;

		p_dce->func_en_msk |= DCE_FUNC_CFA;
	} else {
		p_dce->func_en_msk &= (~(DCE_FUNC_CFA | DCE_FUNC_CFA_PINKR));
	}
	return rt;
}

static ER kdrv_dce_set_iq_dc_fov(UINT32 id,  DCE_MODE_INFO *p_dce)
{

	ER rt = E_OK;

	//FOV
	if (g_kdrv_dce_info[id].fov_param.fov_enable || p_dce->dc_sel == DCE_DCSEL_GDC_ONLY) {
		p_dce->fov_param.fov_gain = g_kdrv_dce_info[id].fov_param.fov_gain;
		p_dce->fov_param.fov_sel = (DCE_FOV_SEL)g_kdrv_dce_info[id].fov_param.fov_bnd_sel;
		p_dce->fov_param.fov_rgb_val.r_bound = g_kdrv_dce_info[id].fov_param.fov_bnd_r;
		p_dce->fov_param.fov_rgb_val.g_bound = g_kdrv_dce_info[id].fov_param.fov_bnd_g;
		p_dce->fov_param.fov_rgb_val.b_bound = g_kdrv_dce_info[id].fov_param.fov_bnd_b;
	} else {
		p_dce->fov_param.fov_gain = 1024;
		p_dce->fov_param.fov_sel = DCE_FOV_BOUND_DUPLICATE;
		p_dce->fov_param.fov_rgb_val.r_bound = 0;
		p_dce->fov_param.fov_rgb_val.g_bound = 0;
		p_dce->fov_param.fov_rgb_val.b_bound = 0;
	}
	DBG_IND("fov gain %d\r\n", (unsigned int)g_kdrv_dce_info[id].fov_param.fov_gain);

	return rt;
}

static ER kdrv_dce_set_iq_dc_img_center(UINT32 id,  DCE_MODE_INFO *p_dce)
{

	ER rt = E_OK;

	p_dce->img_cent.x_cent = g_kdrv_dce_info[id].gdc_center.geo_center_x;
	p_dce->img_cent.y_cent = g_kdrv_dce_info[id].gdc_center.geo_center_y;
	DBG_IND("cent (%d, %d)\r\n",
			(unsigned int)g_kdrv_dce_info[id].gdc_center.geo_center_x,
			(unsigned int)g_kdrv_dce_info[id].gdc_center.geo_center_y);

	return rt;
}


static ER kdrv_dce_set_iq_gdc_cac(UINT32 id,  DCE_MODE_INFO *p_dce)
{
	INT32 i;
	ER rt = E_OK;

	if (g_kdrv_dce_info[id].dc_cac_param.dc_enable) {

		p_dce->gdc_cac_dz_param.p_g_geo_lut =  g_kdrv_dce_info[id].dc_cac_param.dc.geo_lut_g;
		for (i = 0; i < GDC_TABLE_NUM; i++) {
			DBG_UNIT("%u, ", (unsigned int)p_dce->gdc_cac_dz_param.p_g_geo_lut[i]);
		}
		DBG_UNIT("\r\n");

		p_dce->gdc_cac_dz_param.img_ratio.x_dist = g_kdrv_dce_info[id].dc_cac_param.dc.geo_dist_x;
		p_dce->gdc_cac_dz_param.img_ratio.y_dist = g_kdrv_dce_info[id].dc_cac_param.dc.geo_dist_y;
		p_dce->gdc_cac_dz_param.distance_norm = g_kdrv_dce_norm_calculation(id);

		if (g_kdrv_dce_info[id].dc_cac_param.cac.cac_enable) {
			p_dce->gdc_cac_oz_param.cac_sel = (DCE_CAC_SEL)g_kdrv_dce_info[id].dc_cac_param.cac.cac_sel;
			p_dce->gdc_cac_oz_param.cac_gain.g_lut_gain = g_kdrv_dce_info[id].dc_cac_param.cac.geo_g_lut_gain;
			p_dce->gdc_cac_oz_param.cac_gain.r_lut_gain = g_kdrv_dce_info[id].dc_cac_param.cac.geo_r_lut_gain;
			p_dce->gdc_cac_oz_param.cac_gain.b_lut_gain = g_kdrv_dce_info[id].dc_cac_param.cac.geo_b_lut_gain;
			p_dce->gdc_cac_oz_param.p_r_cac_lut = g_kdrv_dce_info[id].dc_cac_param.cac.geo_lut_r; // 0:R
			p_dce->gdc_cac_oz_param.p_b_cac_lut = g_kdrv_dce_info[id].dc_cac_param.cac.geo_lut_b; // 2:B

			p_dce->func_en_msk |= DCE_FUNC_CAC;
		} else {
			p_dce->func_en_msk &= (~DCE_FUNC_CAC);
		}

	}
	return rt;
}

static ER kdrv_dce_set_iq_2dlut(UINT32 id,  DCE_MODE_INFO *p_dce)
{

	ER rt = E_OK;

	if (g_kdrv_dce_info[id].lut_2d_param.lut_2d_enable) {
		INT32 i;
		for (i = 0; i < LUT2D_MAX_TABLE_SIZE; i++) {
			gp_2dlut_buffer[i] = g_kdrv_dce_info[id].lut_2d_param.lut_2d_value[i];
		}
		p_dce->lut2d_param.lut2d_addr = (UINT32)gp_2dlut_buffer;
		p_dce->lut2d_param.ymin_auto_en = 1;
		p_dce->lut2d_param.lut2d_xy_ofs.x_ofs_int = 0;
		p_dce->lut2d_param.lut2d_xy_ofs.x_ofs_frac = 0;
		p_dce->lut2d_param.lut2d_xy_ofs.y_ofs_int = 0;
		p_dce->lut2d_param.lut2d_xy_ofs.y_ofs_frac = 0;

		p_dce->lut2d_param.lut2d_num_sel = (DCE_2DLUT_NUM)g_kdrv_dce_info[id].lut_2d_param.lut_num_select;

		p_dce->func_en_msk &= (~DCE_FUNC_CAC);//shouldn't enable due to algorithm difference
	}

	return rt;

}

static ER kdrv_dce_set_iq_dc(UINT32 id,  DCE_MODE_INFO *p_dce)
{
	ER rt = E_OK;

	DBG_IND("dc enable %d, lut_2d_enable %d\r\n", (unsigned int)g_kdrv_dce_info[id].dc_cac_param.dc_enable, (unsigned int)g_kdrv_dce_info[id].lut_2d_param.lut_2d_enable);
	if (g_kdrv_dce_info[id].dc_cac_param.dc_enable || g_kdrv_dce_info[id].lut_2d_param.lut_2d_enable) {

		//common settings for DC & 2d lut
		if (g_kdrv_dce_info[id].dc_cac_param.dc_enable && (!g_kdrv_dce_info[id].lut_2d_param.lut_2d_enable)) {
			p_dce->dc_sel = DCE_DCSEL_GDC_ONLY;
		} else if ((!g_kdrv_dce_info[id].dc_cac_param.dc_enable) && g_kdrv_dce_info[id].lut_2d_param.lut_2d_enable) {
			p_dce->dc_sel = DCE_DCSEL_2DLUT_ONLY;
		} else {
			p_dce->dc_sel = DCE_DCSEL_GDC_ONLY;
			DBG_ERR("dc and lut_2d cannot be enabled at the same time\r\n");
		}
		DBG_IND("dc_sel %d\r\n", (unsigned int)p_dce->dc_sel);

		p_dce->sram_sel = g_kdrv_dce_info[id].dc_sram_sel.sram_mode;

		//todo.//back_rsv_line
		if (p_dce->op_param.op_mode == DCE_OPMODE_SIE_IME) {
			p_dce->gdc_cac_oz_param.gdc_mode = X_ONLY;
			p_dce->back_rsv_line = 0;
		} else if ((p_dce->dc_sel == DCE_DCSEL_2DLUT_ONLY) && (g_kdrv_dce_info[id].lut_2d_param.lut_2d_y_dist_off == TRUE)) {
			p_dce->gdc_cac_oz_param.gdc_mode = X_ONLY;
			p_dce->back_rsv_line = 0;
		} else if ((p_dce->dc_sel == DCE_DCSEL_GDC_ONLY) && (g_kdrv_dce_info[id].dc_cac_param.dc_y_dist_off == TRUE)) {
			p_dce->gdc_cac_oz_param.gdc_mode = X_ONLY;
			p_dce->back_rsv_line = 0;
		} else {
			p_dce->gdc_cac_oz_param.gdc_mode = XY_BOTH;
			p_dce->back_rsv_line = 1; //todo
		}

		rt |= kdrv_dce_set_iq_dc_fov(id, p_dce);
		rt |= kdrv_dce_set_iq_dc_img_center(id, p_dce);
		rt |= kdrv_dce_set_iq_gdc_cac(id, p_dce);
		rt |= kdrv_dce_set_iq_2dlut(id, p_dce);

		p_dce->func_en_msk |= DCE_FUNC_DC;

	} else {
		p_dce->func_en_msk &= (~(DCE_FUNC_DC | DCE_FUNC_CAC));
	}

	return rt;
}


static ER kdrv_dce_get_stripe_num(UINT32 id, void *data)
{
	UINT32 num = 0;
	KDRV_DCE_STRIPE_NUM *p_stripe_num = (KDRV_DCE_STRIPE_NUM *)data;

	if (nvt_get_chip_id() == CHIP_NA51055) {
		DBG_UNIT("520 stripe rule\r\n");
		if (g_kdrv_dce_info[id].stripe_rule_info.dce_strp_rule == KDRV_DCE_STRP_GDC_BEST) {
			kdrv_dce_gdc_sstp_maxsize = 720;
			kdrv_dce_gdc_3stp_maxsize = 1280;
			kdrv_dce_gdc_4stp_maxsize = 1920;
			kdrv_dce_gdc_6stp_maxsize = 2592;
		} else {
			kdrv_dce_gdc_sstp_maxsize = 1280;
		}
		kdrv_dce_low_lat_sstp_maxsize = 2048;
		kdrv_dce_2dlut_sstp_maxsize = 2688;
		kdrv_dce_2stp_maxsize = 2816;
		kdrv_dce_3stp_maxsize = 4096;
	} else {
		DBG_UNIT("528 stripe rule\r\n");
		if (g_kdrv_dce_info[id].stripe_rule_info.dce_strp_rule == KDRV_DCE_STRP_GDC_BEST) {
			kdrv_dce_gdc_sstp_maxsize = 720;
			kdrv_dce_gdc_3stp_maxsize = 1280;
			kdrv_dce_gdc_4stp_maxsize = 1920;
			kdrv_dce_gdc_6stp_maxsize = 2592;
		} else {
			kdrv_dce_gdc_sstp_maxsize = 1280;
		}
		kdrv_dce_low_lat_sstp_maxsize = 4096;
		kdrv_dce_2dlut_sstp_maxsize = 4096;
		kdrv_dce_2stp_maxsize = 2816;
		kdrv_dce_3stp_maxsize = 4096;
	}

	if (g_kdrv_dce_info[id].stripe_param.stripe_type == KDRV_DCE_STRIPE_AUTO) {
		if (g_kdrv_dce_info[id].in_info.in_src == KDRV_DCE_IN_SRC_ALL_DIRECT) {
			//direct
			num = (UINT32)DCE_STPMODE_SST;
		} else {
			//D2D
#if NEW_STRIPE_RULE
			if (g_kdrv_dce_info[id].stripe_rule_info.dce_strp_rule == KDRV_DCE_STRP_GDC_BEST) {
				if (g_kdrv_dce_info[id].in_img_info.img_size_h <= kdrv_dce_gdc_sstp_maxsize) {
					num = (UINT32)DCE_STPMODE_SST;
				} else if (g_kdrv_dce_info[id].in_img_info.img_size_h <= kdrv_dce_gdc_3stp_maxsize) {
					num = (UINT32)DCE_STPMODE_3ST;
				} else if (g_kdrv_dce_info[id].in_img_info.img_size_h <= kdrv_dce_gdc_4stp_maxsize) {
					num = (UINT32)DCE_STPMODE_4ST;
				} else if (g_kdrv_dce_info[id].in_img_info.img_size_h <= kdrv_dce_gdc_6stp_maxsize) {
					num = (UINT32)DCE_STPMODE_6ST;
				} else {
					num = (UINT32)DCE_STPMODE_8ST; //for resolution larger than 5M
				}
			} else if (g_kdrv_dce_info[id].stripe_rule_info.dce_strp_rule == KDRV_DCE_STRP_GDC_HIGH_PRIOR) {
				if (g_kdrv_dce_info[id].in_img_info.img_size_h <= kdrv_dce_gdc_sstp_maxsize) {
					num = (UINT32)DCE_STPMODE_SST;
				} else if (g_kdrv_dce_info[id].in_img_info.img_size_h <= kdrv_dce_2stp_maxsize) {
					num = (UINT32)DCE_STPMODE_2ST;
				} else if (g_kdrv_dce_info[id].in_img_info.img_size_h <= kdrv_dce_3stp_maxsize) {
					num = (UINT32)DCE_STPMODE_3ST;
				} else {
					num = (UINT32)DCE_STPMODE_4ST;
				}
			} else if (g_kdrv_dce_info[id].stripe_rule_info.dce_strp_rule == KDRV_DCE_STRP_LOW_LAT_HIGH_PRIOR) {
				if (g_kdrv_dce_info[id].in_img_info.img_size_h <= kdrv_dce_low_lat_sstp_maxsize) {
					num = (UINT32)DCE_STPMODE_SST;
				} else if (nvt_get_chip_id() == CHIP_NA51055 && g_kdrv_dce_info[id].in_img_info.img_size_h <= kdrv_dce_2stp_maxsize) {
					num = (UINT32)DCE_STPMODE_2ST;
				} else {
					num = (UINT32)DCE_STPMODE_3ST;
				}
			} else if (g_kdrv_dce_info[id].stripe_rule_info.dce_strp_rule == KDRV_DCE_STRP_2DLUT_HIGH_PRIOR) {
				if (g_kdrv_dce_info[id].in_img_info.img_size_h <= kdrv_dce_2dlut_sstp_maxsize) {
					num = (UINT32)DCE_STPMODE_SST;
				} else if (nvt_get_chip_id() == CHIP_NA51055 && g_kdrv_dce_info[id].in_img_info.img_size_h <= kdrv_dce_2stp_maxsize) {
					num = (UINT32)DCE_STPMODE_2ST;
				} else if (nvt_get_chip_id() == CHIP_NA51055 && g_kdrv_dce_info[id].in_img_info.img_size_h <= kdrv_dce_3stp_maxsize) {
					num = (UINT32)DCE_STPMODE_3ST;
				} else {
					num = (UINT32)DCE_STPMODE_4ST;
				}
			}
#else
			if (((g_kdrv_dce_info[id].stripe_rule_info.dce_strp_rule == KDRV_DCE_STRP_GDC_HIGH_PRIOR) && (g_kdrv_dce_info[id].in_img_info.img_size_h < kdrv_dce_gdc_sstp_maxsize)) ||
				((g_kdrv_dce_info[id].stripe_rule_info.dce_strp_rule == KDRV_DCE_STRP_LOW_LAT_HIGH_PRIOR) && (g_kdrv_dce_info[id].in_img_info.img_size_h <= kdrv_dce_low_lat_sstp_maxsize)) ||
				((g_kdrv_dce_info[id].stripe_rule_info.dce_strp_rule == KDRV_DCE_STRP_2DLUT_HIGH_PRIOR) && (g_kdrv_dce_info[id].in_img_info.img_size_h <= kdrv_dce_2dlut_sstp_maxsize))
			   ) {
				num = (UINT32)DCE_STPMODE_SST;
			} else if (g_kdrv_dce_info[id].in_img_info.img_size_h <= kdrv_dce_2stp_maxsize) {
				num = (UINT32)DCE_STPMODE_2ST;
			} else if (g_kdrv_dce_info[id].in_img_info.img_size_h <= kdrv_dce_3stp_maxsize) {
				num = (UINT32)DCE_STPMODE_3ST;
			} else {
				if (g_kdrv_dce_info[id].stripe_rule_info.dce_strp_rule == KDRV_DCE_STRP_LOW_LAT_HIGH_PRIOR) {
					num = (UINT32)DCE_STPMODE_3ST;
				} else {
					num = (UINT32)DCE_STPMODE_4ST;
				}
			}
#endif
		}
	} else if (g_kdrv_dce_info[id].stripe_param.stripe_type == KDRV_DCE_STRIPE_MANUAL_2STRIPE) {
		num = (UINT32)DCE_STPMODE_2ST;
	} else if (g_kdrv_dce_info[id].stripe_param.stripe_type == KDRV_DCE_STRIPE_MANUAL_3STRIPE) {
		num = (UINT32)DCE_STPMODE_3ST;
	} else if (g_kdrv_dce_info[id].stripe_param.stripe_type == KDRV_DCE_STRIPE_MANUAL_4STRIPE) {
		num = (UINT32)DCE_STPMODE_4ST;
	} else if (g_kdrv_dce_info[id].stripe_param.stripe_type == KDRV_DCE_STRIPE_MANUAL_5STRIPE) {
		num = (UINT32)DCE_STPMODE_5ST;
	} else if (g_kdrv_dce_info[id].stripe_param.stripe_type == KDRV_DCE_STRIPE_MANUAL_6STRIPE) {
		num = (UINT32)DCE_STPMODE_6ST;
	} else if (g_kdrv_dce_info[id].stripe_param.stripe_type == KDRV_DCE_STRIPE_MANUAL_7STRIPE) {
		num = (UINT32)DCE_STPMODE_7ST;
	} else if (g_kdrv_dce_info[id].stripe_param.stripe_type == KDRV_DCE_STRIPE_MANUAL_8STRIPE) {
		num = (UINT32)DCE_STPMODE_8ST;
	} else if (g_kdrv_dce_info[id].stripe_param.stripe_type == KDRV_DCE_STRIPE_MANUAL_9STRIPE) {
		num = (UINT32)DCE_STPMODE_9ST;
	} else if (g_kdrv_dce_info[id].stripe_param.stripe_type == KDRV_DCE_STRIPE_MANUAL_MULTI) {
		num = (UINT32)DCE_STPMODE_MST;
	} else {
		num = (UINT32)g_kdrv_dce_info[id].stripe_param.stripe_type;
	}

	p_stripe_num->num = num;

	DBG_UNIT("Stripe rule set number to %u\r\n", num);

	return E_OK;
}

static ER kdrv_dce_set_mode(UINT32 id,  DCE_MODE_INFO *p_dce)
{
	ER rt = E_OK;
	int i = 0;
	unsigned long myflags;
	BOOL  ipl_cfa_en, ipl_dc_en, ipl_cac_en, ipl_2dlut_en;
	BOOL  ipl_wdr_en, ipl_tonecurve_en, ipl_hist_en, ipl_fov_en;
	BOOL  iq_cfa_en, iq_dc_en, iq_cac_en, iq_2dlut_en;
	BOOL  iq_wdr_en, iq_tonecurve_en, iq_hist_en, iq_fov_en;
	KDRV_DCE_STRIPE_NUM stripe_num;

#if !defined (CONFIG_NVT_SMALL_HDAL)
	snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): ", hwclock_get_longcounter(), total_log_idx, __func__);
	log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
	total_log_idx++;
#endif

	DBG_UNIT("[%d]IPL %x\r\n", id , g_kdrv_dce_ipl_ctrl_en[id]);
	DBG_UNIT("[%d]IQ %x\r\n", id , g_kdrv_dce_iq_ctrl_en[id]);

	//Both IPL & PQ enable must be considered. Therefore, sync IPL enable/disable settings here
	ipl_cfa_en          = (g_kdrv_dce_ipl_ctrl_en[id] & KDRV_DCE_IQ_FUNC_CFA) ? (TRUE) : (FALSE);
	ipl_dc_en           = (g_kdrv_dce_ipl_ctrl_en[id] & KDRV_DCE_IQ_FUNC_DC) ? (TRUE) : (FALSE);
	ipl_cac_en          = (g_kdrv_dce_ipl_ctrl_en[id] & KDRV_DCE_IQ_FUNC_CAC) ? (TRUE) : (FALSE);
	ipl_2dlut_en        = (g_kdrv_dce_ipl_ctrl_en[id] & KDRV_DCE_IQ_FUNC_2DLUT) ? (TRUE) : (FALSE);
	ipl_wdr_en          = (g_kdrv_dce_ipl_ctrl_en[id] & KDRV_DCE_IQ_FUNC_WDR) ? (TRUE) : (FALSE);
	ipl_tonecurve_en    = (g_kdrv_dce_ipl_ctrl_en[id] & KDRV_DCE_IQ_FUNC_TONECURVE) ? (TRUE) : (FALSE);
	ipl_hist_en         = (g_kdrv_dce_ipl_ctrl_en[id] & KDRV_DCE_IQ_FUNC_HISTOGRAM) ? (TRUE) : (FALSE);
	ipl_fov_en          = (g_kdrv_dce_ipl_ctrl_en[id] & KDRV_DCE_IQ_FUNC_FOV) ? (TRUE) : (FALSE);

	iq_cfa_en           = (g_kdrv_dce_iq_ctrl_en[id] & KDRV_DCE_IQ_FUNC_CFA) ? (TRUE) : (FALSE);
	iq_dc_en            = (g_kdrv_dce_iq_ctrl_en[id] & KDRV_DCE_IQ_FUNC_DC) ? (TRUE) : (FALSE);
	iq_cac_en           = (g_kdrv_dce_iq_ctrl_en[id] & KDRV_DCE_IQ_FUNC_CAC) ? (TRUE) : (FALSE);
	iq_2dlut_en         = (g_kdrv_dce_iq_ctrl_en[id] & KDRV_DCE_IQ_FUNC_2DLUT) ? (TRUE) : (FALSE);
	iq_wdr_en           = (g_kdrv_dce_iq_ctrl_en[id] & KDRV_DCE_IQ_FUNC_WDR) ? (TRUE) : (FALSE);
	iq_tonecurve_en     = (g_kdrv_dce_iq_ctrl_en[id] & KDRV_DCE_IQ_FUNC_TONECURVE) ? (TRUE) : (FALSE);
	iq_hist_en          = (g_kdrv_dce_iq_ctrl_en[id] & KDRV_DCE_IQ_FUNC_HISTOGRAM) ? (TRUE) : (FALSE);
	iq_fov_en           = (g_kdrv_dce_iq_ctrl_en[id] & KDRV_DCE_IQ_FUNC_FOV) ? (TRUE) : (FALSE);

	DBG_UNIT("IPL cfa %d, dc %d, cac %d, 2dlut %d, wdr %d, tone curve %d, histogrma %d, fov %d\r\n"
			, (int)ipl_cfa_en
			, (int)ipl_dc_en
			, (int)ipl_cac_en
			, (int)ipl_2dlut_en
			, (int)ipl_wdr_en
			, (int)ipl_tonecurve_en
			, (int)ipl_hist_en
			, (int)ipl_fov_en
			);
	DBG_UNIT("IQ  cfa %d, dc %d, cac %d, 2dlut %d, wdr %d, tone curve %d, histogrma %d, fov %d\r\n"
			, (int)iq_cfa_en
			, (int)iq_dc_en
			, (int)iq_cac_en
			, (int)iq_2dlut_en
			, (int)iq_wdr_en
			, (int)iq_tonecurve_en
			, (int)iq_hist_en
			, (int)iq_fov_en
			);

	//physical register enable
	g_kdrv_dce_info[id].cfa_param.cfa_enable         = (ipl_cfa_en && iq_cfa_en);
	g_kdrv_dce_info[id].dc_cac_param.dc_enable       = (ipl_dc_en && iq_dc_en);
	g_kdrv_dce_info[id].dc_cac_param.cac.cac_enable  = (ipl_cac_en && iq_cac_en);
	g_kdrv_dce_info[id].lut_2d_param.lut_2d_enable   = (ipl_2dlut_en && iq_2dlut_en);
	g_kdrv_dce_info[id].wdr_param.wdr_enable         = (ipl_wdr_en && iq_wdr_en);
	g_kdrv_dce_info[id].wdr_param.tonecurve_enable   = (ipl_tonecurve_en && iq_tonecurve_en);
	g_kdrv_dce_info[id].hist_param.hist_enable       = (ipl_hist_en && iq_hist_en);
	g_kdrv_dce_info[id].fov_param.fov_enable         = (ipl_fov_en && iq_fov_en);

	if (g_kdrv_dce_info[id].in_info.in_src == KDRV_DCE_IN_SRC_DCE_IME) {
		g_kdrv_dce_info[id].cfa_param.cfa_enable  = FALSE;
	}

	// clear load flag
	if (dce_ctrl_flow_to_do == ENABLE) {
		myflags = dce_platform_spin_lock();
		for (i = 0; i < KDRV_DCE_PARAM_MAX; i++) {
			g_kdrv_dce_load_flg[id][i] = FALSE;
		}
		dce_platform_spin_unlock(myflags);
	}

	rt |= kdrv_dce_check_param(id);

	rt |= kdrv_dce_set_dram_single_mode_info(id, p_dce);
	rt |= kdrv_dce_set_in_out_info(id, p_dce);
	rt |= kdrv_dce_set_dma_in_info(id, p_dce);
	rt |= kdrv_dce_set_dma_out_info(id, p_dce);

	kdrv_dce_get_stripe_num(id, (void *)&stripe_num);
	p_dce->op_param.stp_mode = (DCE_STP_MODE)stripe_num.num;
	g_kdrv_dce_stripe_mode[id] = p_dce->op_param.stp_mode;
	DBG_IND("stripe mode %d\r\n", g_kdrv_dce_stripe_mode[id]);

	if ((p_dce->op_param.op_mode == DCE_OPMODE_SIE_IME) && (p_dce->op_param.stp_mode != DCE_STPMODE_SST)) {
		DBG_ERR("Current stripe mode is %d, Only SST(%d) is supported in direct mode!\r\n", (int)p_dce->op_param.stp_mode, (int)DCE_STPMODE_SST);
		return E_NOSPT;
	}
	p_dce->h_stp_param.h_stp_inc_dec.max_inc = g_dce_strip_result.max_inc;
	p_dce->h_stp_param.h_stp_inc_dec.max_dec = g_dce_strip_result.max_dec;
	p_dce->h_stp_param.h_stp_in_start = DCE_1ST_STP_POS_CALC;
	p_dce->h_stp_param.p_h_stripe = g_dce_strip_result.gdc_h_stp;

	DBG_IND("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\r\n"
			, (unsigned int)p_dce->h_stp_param.p_h_stripe[0]
			, (unsigned int)p_dce->h_stp_param.p_h_stripe[1]
			, (unsigned int)p_dce->h_stp_param.p_h_stripe[2]
			, (unsigned int)p_dce->h_stp_param.p_h_stripe[3]
			, (unsigned int)p_dce->h_stp_param.p_h_stripe[4]
			, (unsigned int)p_dce->h_stp_param.p_h_stripe[5]
			, (unsigned int)p_dce->h_stp_param.p_h_stripe[6]
			, (unsigned int)p_dce->h_stp_param.p_h_stripe[7]
			, (unsigned int)p_dce->h_stp_param.p_h_stripe[8]
			, (unsigned int)p_dce->h_stp_param.p_h_stripe[9]
			, (unsigned int)p_dce->h_stp_param.p_h_stripe[10]
			, (unsigned int)p_dce->h_stp_param.p_h_stripe[11]
			, (unsigned int)p_dce->h_stp_param.p_h_stripe[12]
			, (unsigned int)p_dce->h_stp_param.p_h_stripe[13]
			, (unsigned int)p_dce->h_stp_param.p_h_stripe[14]
			, (unsigned int)p_dce->h_stp_param.p_h_stripe[15]);

	//IPP overlap rule
	p_dce->h_stp_param.ipe_out_hstp_ovlap = DCE_HSTP_IPEOLAP_16;//for IPE eth 2bit subout
	p_dce->h_stp_param.ime_out_hstp_ovlap = DCE_HSTP_IMEOLAP_32;

	if (g_kdrv_dce_stripe_mode[id] != DCE_STPMODE_SST) {
		if (g_kdrv_dce_info[id].extend_info.ime_3dnr_enable ||
			g_kdrv_dce_info[id].extend_info.ime_enc_enable ||
			g_kdrv_dce_info[id].extend_info.ime_dec_enable) {
			p_dce->h_stp_param.ime_out_hstp_ovlap = DCE_HSTP_IMEOLAP_USER;

			//overlap value of different rules are in ascending order
			if (g_kdrv_dce_info[id].extend_info.ime_3dnr_enable) {
				p_dce->h_stp_param.ime_ovlap_user_val = 128;
			}
			if (g_kdrv_dce_info[id].extend_info.ime_enc_enable || g_kdrv_dce_info[id].extend_info.ime_dec_enable) {
				p_dce->h_stp_param.ime_ovlap_user_val = 256;
			}
		}
	}

	rt |= kdrv_dce_set_iq_wdr(id, p_dce);
	rt |= kdrv_dce_set_iq_hist(id, p_dce);
	rt |= kdrv_dce_set_iq_cfa_subout(id, p_dce);
	rt |= kdrv_dce_set_iq_cfa(id, p_dce);
	rt |= kdrv_dce_set_iq_dc(id, p_dce);

	p_dce->int_en_msk = g_kdrv_dce_info[id].inte_en; //DCE_INT_FRMST | DCE_INT_FRMEND

	DBG_USER("func_en_msk 0x%08x\r\n", (unsigned int)p_dce->func_en_msk);

	rt |= dce_set_mode(p_dce);

	return rt;
}

static ER kdrv_dce_set_load(UINT32 id, void *data)
{

	ER rt = E_OK;
	UINT32 i = 0;//, map_id = 0;
	unsigned long myflags;
	BOOL kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_MAX] = {0};
	BOOL flush = FALSE;

	DCE_MODE_INFO *p_dce = &g_dce_TotalInfo;

	BOOL  ipl_cfa_en, ipl_dc_en, ipl_cac_en, ipl_2dlut_en;
	BOOL  ipl_wdr_en, ipl_tonecurve_en, ipl_hist_en, ipl_fov_en;
	BOOL  iq_cfa_en, iq_dc_en, iq_cac_en, iq_2dlut_en;
	BOOL  iq_wdr_en, iq_tonecurve_en, iq_hist_en, iq_fov_en;

	/** runtime change **/
	//copy to local flag kdrv_dce_load_flg_tmp and clear global flag
	for (i = 0; i < KDRV_DCE_PARAM_MAX; i++) {
		kdrv_dce_load_flg_tmp[i] = g_kdrv_dce_load_flg[id][i];
	}

	myflags = dce_platform_spin_lock();
	if (data != NULL) {
		KDRV_DCE_PARAM_ID item = *(KDRV_DCE_PARAM_ID *)data;
		g_kdrv_dce_load_flg[id][item] = FALSE;
	} else {
		for (i = 0; i < KDRV_DCE_PARAM_MAX; i++) {
			g_kdrv_dce_load_flg[id][i] = FALSE;
		}
	}
	dce_platform_spin_unlock(myflags);

	DBG_UNIT("[%d]IPL %x\r\n", id , g_kdrv_dce_ipl_ctrl_en[id]);
	DBG_UNIT("[%d]IQ %x\r\n", id , g_kdrv_dce_iq_ctrl_en[id]);

	//Both IPL & PQ enable must be considered. Therefore, sync IPL enable/disable settings here
	ipl_cfa_en          = (g_kdrv_dce_ipl_ctrl_en[id] & KDRV_DCE_IQ_FUNC_CFA) ? (TRUE) : (FALSE);
	ipl_dc_en           = (g_kdrv_dce_ipl_ctrl_en[id] & KDRV_DCE_IQ_FUNC_DC) ? (TRUE) : (FALSE);
	ipl_cac_en          = (g_kdrv_dce_ipl_ctrl_en[id] & KDRV_DCE_IQ_FUNC_CAC) ? (TRUE) : (FALSE);
	ipl_2dlut_en        = (g_kdrv_dce_ipl_ctrl_en[id] & KDRV_DCE_IQ_FUNC_2DLUT) ? (TRUE) : (FALSE);
	ipl_wdr_en          = (g_kdrv_dce_ipl_ctrl_en[id] & KDRV_DCE_IQ_FUNC_WDR) ? (TRUE) : (FALSE);
	ipl_tonecurve_en    = (g_kdrv_dce_ipl_ctrl_en[id] & KDRV_DCE_IQ_FUNC_TONECURVE) ? (TRUE) : (FALSE);
	ipl_hist_en         = (g_kdrv_dce_ipl_ctrl_en[id] & KDRV_DCE_IQ_FUNC_HISTOGRAM) ? (TRUE) : (FALSE);
	ipl_fov_en          = (g_kdrv_dce_ipl_ctrl_en[id] & KDRV_DCE_IQ_FUNC_FOV) ? (TRUE) : (FALSE);

	iq_cfa_en           = (g_kdrv_dce_iq_ctrl_en[id] & KDRV_DCE_IQ_FUNC_CFA) ? (TRUE) : (FALSE);
	iq_dc_en            = (g_kdrv_dce_iq_ctrl_en[id] & KDRV_DCE_IQ_FUNC_DC) ? (TRUE) : (FALSE);
	iq_cac_en           = (g_kdrv_dce_iq_ctrl_en[id] & KDRV_DCE_IQ_FUNC_CAC) ? (TRUE) : (FALSE);
	iq_2dlut_en         = (g_kdrv_dce_iq_ctrl_en[id] & KDRV_DCE_IQ_FUNC_2DLUT) ? (TRUE) : (FALSE);
	iq_wdr_en           = (g_kdrv_dce_iq_ctrl_en[id] & KDRV_DCE_IQ_FUNC_WDR) ? (TRUE) : (FALSE);
	iq_tonecurve_en     = (g_kdrv_dce_iq_ctrl_en[id] & KDRV_DCE_IQ_FUNC_TONECURVE) ? (TRUE) : (FALSE);
	iq_hist_en          = (g_kdrv_dce_iq_ctrl_en[id] & KDRV_DCE_IQ_FUNC_HISTOGRAM) ? (TRUE) : (FALSE);
	iq_fov_en           = (g_kdrv_dce_iq_ctrl_en[id] & KDRV_DCE_IQ_FUNC_FOV) ? (TRUE) : (FALSE);

	DBG_UNIT("IPL cfa %d, dc %d, cac %d, 2dlut %d, wdr %d, tone curve %d, histogrma %d, fov %d\r\n"
			, (int)ipl_cfa_en
			, (int)ipl_dc_en
			, (int)ipl_cac_en
			, (int)ipl_2dlut_en
			, (int)ipl_wdr_en
			, (int)ipl_tonecurve_en
			, (int)ipl_hist_en
			, (int)ipl_fov_en
			);
	DBG_UNIT("IQ  cfa %d, dc %d, cac %d, 2dlut %d, wdr %d, tone curve %d, histogrma %d, fov %d\r\n"
			, (int)iq_cfa_en
			, (int)iq_dc_en
			, (int)iq_cac_en
			, (int)iq_2dlut_en
			, (int)iq_wdr_en
			, (int)iq_tonecurve_en
			, (int)iq_hist_en
			, (int)iq_fov_en
			);

	g_kdrv_dce_info[id].cfa_param.cfa_enable         = (ipl_cfa_en && iq_cfa_en);
	g_kdrv_dce_info[id].dc_cac_param.dc_enable       = (ipl_dc_en && iq_dc_en);
	g_kdrv_dce_info[id].dc_cac_param.cac.cac_enable  = (ipl_cac_en && iq_cac_en);
	g_kdrv_dce_info[id].lut_2d_param.lut_2d_enable   = (ipl_2dlut_en && iq_2dlut_en);
	g_kdrv_dce_info[id].wdr_param.wdr_enable         = (ipl_wdr_en && iq_wdr_en);
	g_kdrv_dce_info[id].wdr_param.tonecurve_enable   = (ipl_tonecurve_en && iq_tonecurve_en);
	g_kdrv_dce_info[id].hist_param.hist_enable       = (ipl_hist_en && iq_hist_en);
	g_kdrv_dce_info[id].fov_param.fov_enable         = (ipl_fov_en && iq_fov_en);

	if (g_kdrv_dce_info[id].in_info.in_src == KDRV_DCE_IN_SRC_DCE_IME) {
		g_kdrv_dce_info[id].cfa_param.cfa_enable  = FALSE;
	}

	rt |= kdrv_dce_check_param(id);

	//change operation
	if ((kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_IN_INFO])) {

		if (g_kdrv_dce_info[id].in_info.in_src == KDRV_DCE_IN_SRC_DRAM) {
			p_dce->op_param.op_mode = DCE_OPMODE_DCE_D2D;
		} else if (g_kdrv_dce_info[id].in_info.in_src == KDRV_DCE_IN_SRC_DCE_IME) {
			p_dce->op_param.op_mode = DCE_OPMODE_DCE_IME;
		} else if (g_kdrv_dce_info[id].in_info.in_src == KDRV_DCE_IN_SRC_DIRECT) {
			p_dce->op_param.op_mode = DCE_OPMODE_IFE_IME;
		} else {
			p_dce->op_param.op_mode = DCE_OPMODE_SIE_IME;
		}

		rt |= dce_change_operation(&p_dce->op_param);
	}

	//dram out mode
	if ((kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_OUTPUT_MODE])) {
		rt |= kdrv_dce_set_dram_single_mode_info(id, p_dce);
		rt |= dce_change_dram_out_mode(p_dce->dram_out_mode);
	}

	//dram single out channel enable
	if ((kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_SINGLE_OUT])) {
		rt |= kdrv_dce_set_dram_single_mode_info(id, p_dce);
		rt |= dce_change_dram_single_ch_en(&p_dce->single_out_en);
	}

	//change d2d param and input size, crop size
	if ((kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_IN_INFO]    ||
		 kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_IN_IMG]     ||
		 kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_OUT_IMG]    ||
		 kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_IMG_DMA_IN] ||
		 kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_IMG_DMA_OUT])) {
		rt |= kdrv_dce_set_in_out_info(id, p_dce);
		if ((g_kdrv_dce_info[id].in_info.in_src == KDRV_DCE_IN_SRC_DRAM ||
			 g_kdrv_dce_info[id].in_info.in_src == KDRV_DCE_IN_SRC_DCE_IME) &&
			(p_dce->op_param.op_mode == DCE_OPMODE_DCE_D2D ||
			 p_dce->op_param.op_mode == DCE_OPMODE_DCE_IME)) {
			rt |= dce_change_d2d_in_out(&p_dce->d2d_param, p_dce->op_param.op_mode);
			flush = TRUE;
		}

		if (kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_IN_IMG]) {
			rt |= dce_change_img_in_size(&p_dce->img_in_size);
		}
		if (kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_OUT_IMG]) {
			rt |= dce_change_img_out_crop(&p_dce->img_out_crop);
		}
	}

	//change input info
	if ((kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_DMA_SUBIN] ||
		 kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IQ_WDR_SUBIMG])) {
		rt |= kdrv_dce_set_dma_in_info(id, p_dce);
		if (p_dce->func_en_msk & DCE_FUNC_WDR) {
			rt |= dce_change_input_info(&p_dce->input_info);
			flush = TRUE;
		}
	}

	//change output info
	if ((kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_DMA_SUBOUT] ||
		 kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IQ_WDR_SUBIMG]  ||
		 kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_CFAOUT]     ||
		 kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_OUTPUT_MODE] ||
		 kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_SINGLE_OUT])) {
		rt |= kdrv_dce_set_dma_out_info(id, p_dce);
		if ((p_dce->func_en_msk & DCE_FUNC_WDR_SUBOUT || p_dce->func_en_msk & DCE_FUNC_CFA_SUBOUT)) {
			rt |= dce_change_output_info(&p_dce->output_info);
			flush = TRUE;
		}
	}

	//change cfa
	dce_set_raw_fmt(p_dce->cfa_param.raw_fmt);
	dce_set_cfa_pat(p_dce->cfa_param.cfa_pat);
	if (kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IQ_CFA]     ||
		kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_IN_IMG]) {
		rt |= kdrv_dce_set_iq_cfa(id, p_dce);
		if ((p_dce->func_en_msk & DCE_FUNC_CFA)) {
			rt |= dce_change_cfa_interp(&p_dce->cfa_param);
		}
	}

	//change cfa subout
	if (kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_CFAOUT]) {
		rt |= kdrv_dce_set_iq_cfa_subout(id, p_dce);
		if ((p_dce->func_en_msk & DCE_FUNC_CFA_SUBOUT)) {
			rt |= dce_change_cfa_subout(&p_dce->cfa_subout_param);
		}
	}

	//change wdr, wdr_dither, wdr_subout
	if (kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IQ_WDR]           ||
		kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IQ_WDR_SUBIMG]    ||
		kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_IN_IMG]) {
		rt |= kdrv_dce_set_iq_wdr(id, p_dce);

		if ((kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IQ_WDR]) &&
			((p_dce->func_en_msk & DCE_FUNC_WDR) || (p_dce->func_en_msk & DCE_FUNC_TCURVE) || (p_dce->func_en_msk & DCE_FUNC_WDR_SUBOUT))) {
			rt |= dce_change_wdr((p_dce->func_en_msk & DCE_FUNC_WDR), (p_dce->func_en_msk & DCE_FUNC_TCURVE), &p_dce->wdr_param);
		}

		if (((kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IQ_WDR_SUBIMG])  ||
			 (kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_IN_IMG])) &&
			((p_dce->func_en_msk & DCE_FUNC_WDR) || (p_dce->func_en_msk & DCE_FUNC_WDR_SUBOUT))) {
			rt |= dce_change_wdr_subout(&p_dce->wdr_subimg_param);
		}
	}

	//change histogram
	if (kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IQ_HIST]) {
		rt |= kdrv_dce_set_iq_hist(id, p_dce);
		if ((p_dce->func_en_msk & DCE_FUNC_HISTOGRAM)) {
			rt |= dce_change_histogram(&p_dce->hist_param);
		}
	}

	//change dc related setting
	if ((kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IQ_DC_CAC]) ||
		(kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IQ_2DLUT])  ||
		(kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IQ_FOV])    ||
		(kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_GDC_CENTER])) {

		rt |= kdrv_dce_set_iq_dc(id, p_dce);

		if (kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IQ_DC_CAC]) {
			rt |= dce_change_distort_corr_sel(p_dce->dc_sel);
			if (p_dce->func_en_msk & DCE_FUNC_DC && p_dce->dc_sel != DCE_DCSEL_2DLUT_ONLY) {
				rt |= dce_change_gdc_mode(p_dce->gdc_cac_oz_param.gdc_mode);
				rt |= dce_change_gdc_cac_digi_zoom(&(p_dce->gdc_cac_dz_param));
				if (g_kdrv_dce_info[id].dc_cac_param.cac.cac_enable) {
					rt |= dce_change_gdc_cac_opti_zoom(&(p_dce->gdc_cac_oz_param));
				}
			}
		}

		if (kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IQ_2DLUT]) {
			rt |= dce_change_distort_corr_sel(p_dce->dc_sel);
			if (p_dce->func_en_msk & DCE_FUNC_DC && p_dce->dc_sel != DCE_DCSEL_GDC_ONLY) {
				rt |= dce_change_gdc_mode(p_dce->gdc_cac_oz_param.gdc_mode);
				rt |= dce_change_2dlut(&p_dce->lut2d_param, &p_dce->img_in_size);
			}
		}

		if (kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IQ_FOV]) {
			rt |= dce_change_fov(&(p_dce->fov_param));
		}

		if (kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_GDC_CENTER]) {
			rt |= dce_change_img_center(&(p_dce->img_cent));
		}
	}

	//flush
	if (flush == TRUE) {
		dce_flush_cache(p_dce);
	}

	//change sram mode
	if (kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_DC_SRAM_MODE]) {
		if (g_kdrv_dce_info[id].dc_cac_param.dc_enable || g_kdrv_dce_info[id].lut_2d_param.lut_2d_enable) {
			p_dce->sram_sel = g_kdrv_dce_info[id].dc_sram_sel.sram_mode;
		}
		rt |= dce_change_sram_mode(p_dce->sram_sel);
	}

	if (kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IQ_STRIPE_PARAM]) {
		if (g_kdrv_dce_info[id].stripe_param.stripe_type != KDRV_DCE_STRIPE_AUTO) {
			DBG_WRN("set load item KDRV_DCE_PARAM_IQ_STRIPE_PARAM fail \r\n");
			DBG_WRN("direct mode supports SST only!\r\n");
		}
	}

	if (kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_EXTEND]) {
		if (g_kdrv_dce_info[id].stripe_param.stripe_type != KDRV_DCE_STRIPE_AUTO) {
			DBG_WRN("set load item KDRV_DCE_PARAM_IPL_EXTEND fail \r\n");
			DBG_WRN("direct mode supports SST only!\r\n");
		}
	}

	//change interrupt enable
	if (kdrv_dce_load_flg_tmp[KDRV_DCE_PARAM_IPL_INTER]) {
		p_dce->int_en_msk = g_kdrv_dce_info[id].inte_en;
		rt |= dce_change_int_en(p_dce->int_en_msk, DCE_SET_SEL);
	}

	//change function enable
	rt |= dce_change_func_en(p_dce->func_en_msk, DCE_SET_SEL);

	DBG_IND("func_en_mask (set_load) = 0x%08x \r\n", (unsigned int)p_dce->func_en_msk);

	return rt;

}

static void kdrv_dce_lock(KDRV_DCE_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		FLGPTN flag_ptn;
		wai_flg(&flag_ptn, p_handle->flag_id, p_handle->lock_bit, TWF_CLR);
	} else {
		set_flg(p_handle->flag_id, p_handle->lock_bit);
	}
}

static KDRV_DCE_HANDLE *kdrv_dce_chk_handle(KDRV_DCE_HANDLE *p_handle)
{
	UINT32 i;

	for (i = 0; i < kdrv_dce_channel_num; i ++) {
		if (p_handle == &g_kdrv_dce_handle_tab[i]) {
			return p_handle;
		}
	}

	return NULL;
}

static void kdrv_dce_handle_lock(void)
{
	FLGPTN wait_flg;
	wai_flg(&wait_flg, kdrv_dce_get_flag_id(FLG_ID_KDRV_DCE), KDRV_IPP_DCE_HDL_UNLOCK, (TWF_ORW | TWF_CLR));
}

static void kdrv_dce_handle_unlock(void)
{
	set_flg(kdrv_dce_get_flag_id(FLG_ID_KDRV_DCE), KDRV_IPP_DCE_HDL_UNLOCK);
}

static void kdrv_dce_handle_alloc_all(UINT32 *eng_init_flag)
{
	UINT32 i;

	kdrv_dce_handle_lock();
	*eng_init_flag = FALSE;
	for (i = 0; i < kdrv_dce_channel_num; i ++) {
		if (!(g_kdrv_dce_init_flg & (1 << i))) {

			if (g_kdrv_dce_init_flg == 0) {
				*eng_init_flag = TRUE;
			}

			g_kdrv_dce_init_flg |= (1 << i);

			memset(&g_kdrv_dce_handle_tab[i], 0, sizeof(KDRV_DCE_HANDLE));
			g_kdrv_dce_handle_tab[i].entry_id = i;
			g_kdrv_dce_handle_tab[i].flag_id = kdrv_dce_get_flag_id(FLG_ID_KDRV_DCE);
			g_kdrv_dce_handle_tab[i].lock_bit = (1 << i);
			g_kdrv_dce_handle_tab[i].sts |= KDRV_DCE_HANDLE_LOCK;
			g_kdrv_dce_handle_tab[i].sem_id = kdrv_dce_get_sem_id(SEMID_KDRV_DCE);
		}
	}
	kdrv_dce_handle_unlock();

	return;
}

static UINT32 kdrv_dce_handle_free(KDRV_DCE_HANDLE *p_handle)
{
	UINT32 rt = FALSE;
	kdrv_dce_handle_lock();
	p_handle->sts = 0;
	g_kdrv_dce_init_flg &= ~(1 << p_handle->entry_id);
	if (g_kdrv_dce_init_flg == 0) {
		rt = TRUE;
	}
	kdrv_dce_handle_unlock();

	return rt;
}

static KDRV_DCE_HANDLE *kdrv_dce_entry_id_conv2_handle(UINT32 entry_id)
{
	return  &g_kdrv_dce_handle_tab[entry_id];
}


static void kdrv_dce_sem(KDRV_DCE_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		SEM_WAIT(*p_handle->sem_id);    // wait semaphore
	} else {
		SEM_SIGNAL_ISR(*p_handle->sem_id);  // wait semaphore
	}
}

static void kdrv_dce_frm_end(KDRV_DCE_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		iset_flg(p_handle->flag_id, KDRV_IPP_DCE_FMD);
	} else {
		clr_flg(p_handle->flag_id, KDRV_IPP_DCE_FMD);
	}
}

static void kdrv_dce_isr_cb(UINT32 intstatus)
{
	KDRV_DCE_HANDLE *p_handle;
	p_handle = g_kdrv_dce_trig_hdl;

	if (p_handle == NULL) {
		return;
	}

	if (b_kdrv_dce_cal_strip_flow == FALSE) {
		//DBG_ISR("intstatus %d\r\n", (unsigned int)intstatus);
		if (intstatus & DCE_INT_FRMEND) {
			//dce_pause();
			kdrv_dce_sem(p_handle, FALSE);
			kdrv_dce_frm_end(p_handle, TRUE);
		}

		if (p_handle->isrcb_fp != NULL) {
			p_handle->isrcb_fp((UINT32)p_handle, intstatus, NULL, NULL);
		}
	}
}

#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)

#define DCE_2DLUT_BUF_SIZE (KDRV_DCE_CEIL_64(LUT2D_MAX_TABLE_SIZE * 4) + 64)
#if defined __UITRON || defined __ECOS || defined __FREERTOS
//allocate continuous buffer for 2DLUT
static INT32 kdrv_dce_alloc_2dlut_buff(void)
{
	gp_ori_2dlut_buffer = gp_2dlut_buffer = (UINT32 *)malloc(DCE_2DLUT_BUF_SIZE);
	if (!VOS_IS_ALIGNED((UINT32)gp_2dlut_buffer)) {
		gp_2dlut_buffer = (UINT32 *)KDRV_DCE_CEIL_64((UINT32)gp_2dlut_buffer);
	}
	if (gp_2dlut_buffer == NULL) {
		DBG_ERR("buffer allocation for 2D LUT fail!\r\n");
		return E_NOMEM;
	} else {
		return E_OK;
	}
}

static INT32 kdrv_dce_free_2dlut_buff(void)
{
	if (gp_ori_2dlut_buffer != NULL) {
		free(gp_ori_2dlut_buffer);
		gp_ori_2dlut_buffer = NULL;
	} else {
		DBG_WRN("2DLUT buffer is not allocated, nothing happend!\r\n");
	}
	return E_OK;
}
#else
//allocate continuous buffer for 2DLUT
static INT32 kdrv_dce_alloc_2dlut_buff(void)
{
	UINT32 size = DCE_2DLUT_BUF_SIZE;
#if 0
	frammap_buf_t buff = {0};

	buff.size = size;
	buff.align = 64;      ///< address alignment
	buff.name = "dce_2dlut";
	buff.alloc_type = ALLOC_CACHEABLE;
	frm_get_buf_ddr(DDR_ID0, &buff);
	gp_2dlut_buffer = (UINT32 *)buff.va_addr;
#else
	int ret = 0;
	struct nvt_fmem_mem_info_t info = {0};
	ret = nvt_fmem_mem_info_init(&info, NVT_FMEM_ALLOC_CACHE, size, NULL);
	if (ret >= 0) {
		handle_2dlut_buff = fmem_alloc_from_cma(&info, 0);
	}

	gp_ori_2dlut_buffer = gp_2dlut_buffer = info.vaddr;
#endif
	if (!VOS_IS_ALIGNED((UINT32)gp_2dlut_buffer)) {
		gp_2dlut_buffer = (UINT32 *)KDRV_DCE_CEIL_64((UINT32)gp_2dlut_buffer);
	}
	if (gp_2dlut_buffer == NULL) {
		DBG_ERR("frm_get_buf_ddr for 2DLUT fail!\r\n");
		return E_NOMEM;
	} else {
		return E_OK;
	}
}

static INT32 kdrv_dce_free_2dlut_buff(void)
{
	if (gp_ori_2dlut_buffer != NULL) {
#if 0
		frm_free_buf_ddr(gp_ori_2dlut_buffer);
#else
		int ret = 0;
		ret = fmem_release_from_cma(handle_2dlut_buff, 0);
		if (ret < 0) {
			pr_info("error release %d\n", __LINE__);
		}
#endif
		gp_ori_2dlut_buffer = NULL;
		gp_2dlut_buffer = NULL;
	} else {
		DBG_WRN("2DLUT buffer is not allocated, nothing happend!\r\n");
	}

	return E_OK;
}
#endif

#endif

static void kdrv_dce_init_param(INT32 id)
{
	UINT32 i = 0, j = 0;
	INT32 width = 640, height = 480;
	UINT32 temp_word = 0;

	g_kdrv_dce_ipl_ctrl_en[id] = 0x0;
	g_kdrv_dce_iq_ctrl_en[id] = 0x0;

	memset((void *)&g_kdrv_dce_info[id], 0, sizeof(KDRV_DCE_PRAM));

	g_kdrv_dce_info[id].stripe_param.stripe_type = KDRV_DCE_STRIPE_AUTO;
	for (i = 0; i < 16; i++) {
		g_kdrv_dce_info[id].stripe_param.hstp[i] = 64;
	}

	g_kdrv_dce_info[id].inte_en = DCE_INT_FRMST | DCE_INT_FRMEND;

	g_kdrv_dce_info[id].dram_out_mode = KDRV_DCE_DRAM_OUT_NORMAL;
	g_kdrv_dce_info[id].single_out_ch.single_out_wdr_en = 1;
	g_kdrv_dce_info[id].single_out_ch.single_out_cfa_en = 1;

	//WDR
	g_kdrv_dce_iq_ctrl_en[id] &= (~KDRV_DCE_IQ_FUNC_WDR);
	g_kdrv_dce_iq_ctrl_en[id] &= (~KDRV_DCE_IQ_FUNC_TONECURVE);
	g_kdrv_dce_info[id].wdr_param.ftrcoef[0] = 4;
	g_kdrv_dce_info[id].wdr_param.ftrcoef[1] = 2;
	g_kdrv_dce_info[id].wdr_param.ftrcoef[2] = 1;
	g_kdrv_dce_info[id].wdr_param.input_bld.bld_sel = 0;
	g_kdrv_dce_info[id].wdr_param.input_bld.bld_wt = 2;
	for (i = 0; i < WDR_INPUT_BLD_NUM; i++) {
		g_kdrv_dce_info[id].wdr_param.input_bld.blend_lut[i] = wdr_inbld_lut[i];
	}
	g_kdrv_dce_info[id].wdr_param.outbld.outbld_en = 1;
	for (i = 0; i < WDR_NEQ_TABLE_IDX_NUM; i++) {
		g_kdrv_dce_info[id].wdr_param.tonecurve.lut_idx[i] = wdr_tonecurve_idx[i];
		g_kdrv_dce_info[id].wdr_param.tonecurve.lut_split[i] = wdr_tonecurve_split[i];
		g_kdrv_dce_info[id].wdr_param.outbld.outbld_lut.lut_idx[i] = wdr_outbld_idx[i];
		g_kdrv_dce_info[id].wdr_param.outbld.outbld_lut.lut_split[i] = wdr_outbld_split[i];
	}
	for (i = 0; i < WDR_NEQ_TABLE_VAL_NUM; i++) {
		g_kdrv_dce_info[id].wdr_param.tonecurve.lut_val[i] = wdr_tonecurve_val[i];
		g_kdrv_dce_info[id].wdr_param.outbld.outbld_lut.lut_val[i] = wdr_outbld_val[i];
	}

	g_kdrv_dce_info[id].wdr_param.wdr_str.contrast = 128;
	g_kdrv_dce_info[id].wdr_param.wdr_str.strength = 128;
	g_kdrv_dce_info[id].wdr_param.wdr_str.wdr_coeff[0] = -1732;
	g_kdrv_dce_info[id].wdr_param.wdr_str.wdr_coeff[1] = -2715;
	g_kdrv_dce_info[id].wdr_param.wdr_str.wdr_coeff[2] = -2548;
	g_kdrv_dce_info[id].wdr_param.wdr_str.wdr_coeff[3] = -2548;

	g_kdrv_dce_info[id].wdr_param.gainctrl.gainctrl_en = 1;
	g_kdrv_dce_info[id].wdr_param.gainctrl.max_gain = 3;
	g_kdrv_dce_info[id].wdr_param.gainctrl.min_gain = 5;

	g_kdrv_dce_info[id].wdr_param.sat_reduct.sat_delta = 16;
	g_kdrv_dce_info[id].wdr_param.sat_reduct.sat_th = 32;
	g_kdrv_dce_info[id].wdr_param.sat_reduct.sat_wt_low = 0;

	//wdr subout
	g_kdrv_dce_info[id].wdr_subimg_param.subimg_size_h = 16;
	g_kdrv_dce_info[id].wdr_subimg_param.subimg_size_v = 9;
	g_kdrv_dce_info[id].wdr_subimg_param.subimg_lofs_in = 32 << 2;
	g_kdrv_dce_info[id].wdr_subimg_param.subimg_lofs_out = 32 << 2;

	g_kdrv_dce_info[id].wdr_subout_addr.enable = 0;

	//Hist
	g_kdrv_dce_iq_ctrl_en[id] &= (~KDRV_DCE_IQ_FUNC_HISTOGRAM);

	//CFA subout
	g_kdrv_dce_info[id].cfa_subout_info.cfa_subout_enable = 0;
	g_kdrv_dce_info[id].cfa_subout_info.cfa_subout_flip_enable = 0;
	g_kdrv_dce_info[id].cfa_subout_info.subout_byte = KDRV_DCE_CFA_SUBOUT_1BYTE;
	g_kdrv_dce_info[id].cfa_subout_info.subout_ch_sel = 0;
	g_kdrv_dce_info[id].cfa_subout_info.subout_shiftbit = 0;

	//CFA
	g_kdrv_dce_iq_ctrl_en[id] |= KDRV_DCE_IQ_FUNC_CFA;
	g_kdrv_dce_info[id].in_img_info.cfa_pat = 0;

	g_kdrv_dce_info[id].cfa_param.cfa_interp.edge_dth = 12;
	g_kdrv_dce_info[id].cfa_param.cfa_interp.edge_dth2 = 40;
	g_kdrv_dce_info[id].cfa_param.cfa_interp.freq_th = 160;
	for (i = 0; i < KDRV_CFA_FREQ_NUM; i++) {
		g_kdrv_dce_info[id].cfa_param.cfa_interp.freq_lut[i] = cfa_freq_lut[i];
	}
	for (i = 0; i < KDRV_CFA_LUMA_NUM; i++) {
		g_kdrv_dce_info[id].cfa_param.cfa_interp.luma_wt[i] = cfa_luma_lut[i];
	}

	g_kdrv_dce_info[id].cfa_param.cfa_correction.rb_corr_enable = TRUE;
	g_kdrv_dce_info[id].cfa_param.cfa_correction.rb_corr_th1 = 0;
	g_kdrv_dce_info[id].cfa_param.cfa_correction.rb_corr_th2 = 0;

	g_kdrv_dce_info[id].cfa_param.cfa_fcs.fcs_dirsel = 1;
	g_kdrv_dce_info[id].cfa_param.cfa_fcs.fcs_coring = 8;
	g_kdrv_dce_info[id].cfa_param.cfa_fcs.fcs_weight = 64;
	for (i = 0; i < KDRV_CFA_FCS_NUM; i++) {
		g_kdrv_dce_info[id].cfa_param.cfa_fcs.fcs_strength[i] = cfa_fcs_lut[i];
	}

	g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.cl_check_enable = TRUE;
	g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.hf_check_enable = TRUE;
	g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.average_mode = 1;
	g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.cl_sel = 0;
	g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.cl_th = 0x20;
	g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.hfg_th = 0x4;
	g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.hf_diff = 0x20;
	g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.hfe_th = 0x40;
	g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.ir_g_edge_th = 8;
	g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.ir_rb_cstrength = 4;

	g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_r = 256;
	g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_g = 256;
	g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_b = 256;
	g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_wt_lb = 0;
	g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_th = 128;
	g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_range = 0;
	g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sat_gain = 256;

	g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_en = 0;
	g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_mode = 0;
	g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_th1 = 192;
	g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_th2 = 224;
	g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_th3 = 255;
	g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_th4 = 255;

	g_kdrv_dce_info[id].cfa_param.cfa_cgain.gain_range = 0;
	g_kdrv_dce_info[id].cfa_param.cfa_cgain.r_gain = 256;
	g_kdrv_dce_info[id].cfa_param.cfa_cgain.g_gain = 256;
	g_kdrv_dce_info[id].cfa_param.cfa_cgain.b_gain = 256;

	//DC
	g_kdrv_dce_iq_ctrl_en[id] &= (~KDRV_DCE_IQ_FUNC_DC);
	g_kdrv_dce_info[id].dc_sram_sel.sram_mode = KDRV_DCE_SRAM_DCE;

	g_kdrv_dce_info[id].dc_cac_param.dc.geo_dist_x = 4095;
	g_kdrv_dce_info[id].dc_cac_param.dc.geo_dist_y = 4095;
	for (i = 0; i < GDC_TABLE_NUM; i++) {
		g_kdrv_dce_info[id].dc_cac_param.dc.geo_lut_g[i] = 65535;
	}

	g_kdrv_dce_iq_ctrl_en[id] &= (~KDRV_DCE_IQ_FUNC_CAC);
	g_kdrv_dce_info[id].dc_cac_param.cac.geo_r_lut_gain = 4096;
	g_kdrv_dce_info[id].dc_cac_param.cac.geo_g_lut_gain = 4096;
	g_kdrv_dce_info[id].dc_cac_param.cac.geo_b_lut_gain = 4096;
	for (i = 0; i < CAC_TABLE_NUM; i++) {
		g_kdrv_dce_info[id].dc_cac_param.cac.geo_lut_r[i] = 0;
		g_kdrv_dce_info[id].dc_cac_param.cac.geo_lut_b[i] = 0;
	}
	g_kdrv_dce_info[id].dc_cac_param.cac.cac_sel = 0;

	//2DLUT
	g_kdrv_dce_iq_ctrl_en[id] &= (~KDRV_DCE_IQ_FUNC_2DLUT);
	g_kdrv_dce_info[id].lut_2d_param.lut_num_select = KDRV_DCE_2DLUT_9_9_GRID;
	for (j = 0; j < 9; j++) {
		for (i = 0; i < 9; i++) {
			temp_word = (((j * (height / (9 - 1))) << 2) << 16) + ((i * (width / (9 - 1))) << 2);
			g_kdrv_dce_info[id].lut_2d_param.lut_2d_value[j * 9 + i] = temp_word;
		}
	}

	//FOV
	g_kdrv_dce_info[id].fov_param.fov_gain = 1024;
	g_kdrv_dce_info[id].fov_param.fov_bnd_sel = KDRV_DCE_FOV_BND_DUPLICATE;
	g_kdrv_dce_info[id].fov_param.fov_bnd_r = 0;
	g_kdrv_dce_info[id].fov_param.fov_bnd_g = 0;
	g_kdrv_dce_info[id].fov_param.fov_bnd_b = 0;

	//STRIPE
	g_kdrv_dce_info[id].stripe_rule_info.dce_strp_rule = KDRV_DCE_STRP_GDC_HIGH_PRIOR;//KDRV_DCE_STRP_LOW_LAT_HIGH_PRIOR;
}


#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)
static UINT32 *g_dce_channel_buf = NULL;

#if defined __UITRON || defined __ECOS || defined __FREERTOS
#else
static void *handle_dce_channel_buff = NULL;
#endif

#if defined __UITRON || defined __ECOS || defined __FREERTOS
static INT32 kdrv_dce_alloc_channel_buff(UINT32 buf_size)
{
	g_dce_channel_buf = (UINT32 *)malloc(buf_size * sizeof(UINT32));
	if (g_dce_channel_buf == NULL) {
		DBG_ERR("DCE buffer allocation for ipe channel buffer fail!\r\n");
		return E_NOMEM;
	} else {
		return E_OK;
	}
}

static INT32 kdrv_dce_free_channel_buff(void)
{
	if (g_dce_channel_buf != NULL) {
		free(g_dce_channel_buf);
		g_dce_channel_buf = NULL;
	} else {
		DBG_WRN("DCE channel buffer is not allocated, nothing happend!\r\n");
	}
	return E_OK;
}
#else
static INT32 kdrv_dce_alloc_channel_buff(UINT32 buf_size)
{

	int ret = 0;
	struct nvt_fmem_mem_info_t info = {0};
	ret = nvt_fmem_mem_info_init(&info, NVT_FMEM_ALLOC_CACHE, buf_size, NULL);
	if (ret >= 0) {
		handle_dce_channel_buff = fmem_alloc_from_cma(&info, 0);
	}

	g_dce_channel_buf = info.vaddr;
	if (g_dce_channel_buf == NULL) {
		DBG_ERR("frm_get_buf_ddr for DCE channel buffer fail!\r\n");
		return E_NOMEM;
	} else {
		return E_OK;
	}
}

static INT32 kdrv_dce_free_channel_buff(void)
{
	if (g_dce_channel_buf != NULL) {

		int ret = 0;
		ret = fmem_release_from_cma(handle_dce_channel_buff, 0);
		if (ret < 0) {
			pr_info("error release %d\n", __LINE__);
		}
		g_dce_channel_buf = NULL;
	} else {
		DBG_WRN("DCE channel buffer is not allocated, nothing happend!\r\n");
	}

	return E_OK;
}
#endif
#endif //#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)



VOID kdrv_dce_init(VOID)
{
#if !(defined __UITRON || defined __ECOS)
#if defined __FREERTOS
	dce_platform_create_resource();
#endif
	dce_platform_set_clk_rate(280);
#endif

	kdrv_dce_install_id();

#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)
	{
		UINT32 buf_size, buf_size_used;
		buf_size = ALIGN_CEIL_64(kdrv_dce_buf_query(KDRV_IPP_CBUF_MAX_NUM));
		kdrv_dce_alloc_channel_buff(buf_size);
		buf_size_used = kdrv_dce_buf_init((UINT32)g_dce_channel_buf, KDRV_IPP_CBUF_MAX_NUM);
		if (buf_size_used != buf_size) {
			DBG_ERR("DCE channel buffer init size mismatch!\r\n");
			DBG_ERR("DCE buffer query size = %d...\r\n", buf_size);
			DBG_ERR("DCE buffer init size = %d...\r\n", buf_size_used);
		}
	}
#endif
}

VOID kdrv_dce_uninit(VOID)
{
#if !(defined __UITRON || defined __ECOS)
	dce_platform_release_resource();
#endif
	kdrv_dce_uninstall_id();

#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)
	kdrv_dce_buf_uninit();
	kdrv_dce_free_channel_buff();
#endif
}

UINT32 kdrv_dce_buf_query(UINT32 channel_num)
{

	UINT32 buffer_size;

	DBG_IND("channel num %d\r\n", channel_num);

	if (channel_num == 0) {
		DBG_ERR("DCE min channel number = 1\r\n");
		kdrv_dce_channel_num =  1;
	} else if (channel_num > KDRV_IPP_MAX_CHANNEL_NUM) {
		DBG_ERR("DCE max channel number = %d\r\n", KDRV_IPP_MAX_CHANNEL_NUM);
		kdrv_dce_channel_num =  KDRV_IPP_MAX_CHANNEL_NUM;
	} else {
		kdrv_dce_channel_num = channel_num;
	}

	kdrv_dce_lock_chls = (1 << kdrv_dce_channel_num) - 1;

	buffer_size  = ALIGN_CEIL_64(kdrv_dce_channel_num * sizeof(KDRV_DCE_PRAM));				//g_kdrv_dce_info
	buffer_size += ALIGN_CEIL_64(kdrv_dce_channel_num * sizeof(KDRV_DCE_HANDLE));           //g_kdrv_dce_handle_tab
	buffer_size += ALIGN_CEIL_64(kdrv_dce_channel_num * sizeof(UINT32));                    //g_kdrv_dce_ipl_ctrl_en
	buffer_size += ALIGN_CEIL_64(kdrv_dce_channel_num * sizeof(UINT32));                    //g_kdrv_dce_iq_ctrl_en
	buffer_size += ALIGN_CEIL_64(kdrv_dce_channel_num * sizeof(DCE_DIST_NORM));             //g_kdrv_dce_norm
	buffer_size += ALIGN_CEIL_64(kdrv_dce_channel_num * sizeof(DCE_STP_MODE));              //g_kdrv_dce_stripe_mode
	buffer_size += ALIGN_CEIL_64(sizeof(BOOL **));                    						//g_kdrv_dce_load_flg
	buffer_size += kdrv_dce_channel_num * ALIGN_CEIL_64(sizeof(BOOL *));                    //g_kdrv_dce_load_flg[i]
	buffer_size += kdrv_dce_channel_num * ALIGN_CEIL_64(KDRV_DCE_PARAM_MAX * sizeof(BOOL)); //g_kdrv_dce_load_flg[i][0]
	buffer_size += ALIGN_CEIL_64((LUT2D_MAX_TABLE_SIZE * 4) + 64); 							//g_ori_2dlut_buffer & gp_2dlut_buffer, extra 64 bytes for starting address alignment

	/*
	    DBG_IND("KDRV_DCE_PRAM size = %d\r\n", channel_num * sizeof(KDRV_DCE_PRAM));
	    DBG_IND("KDRV_DCE_HANDLE = %d\r\n", channel_num * sizeof(KDRV_DCE_HANDLE));
	    DBG_IND("ipl_ctrl_en size = %d\r\n", channel_num * sizeof(UINT32));
	*/
	return buffer_size;
}

UINT32 kdrv_dce_buf_init(UINT32 input_addr, UINT32 channel_num)
{

	UINT32 buffer_size, i;
	UINT32 channel_used = channel_num;

	kdrv_dce_flow_init();

	if (channel_used == 0) {
		DBG_ERR("DCE min channel number = 1\r\n");
		channel_used = 1;
	} else if (channel_used > KDRV_IPP_MAX_CHANNEL_NUM) {
		DBG_ERR("DCE max channel number = %d\r\n", KDRV_IPP_MAX_CHANNEL_NUM);
		channel_used =  KDRV_IPP_MAX_CHANNEL_NUM;
	}

	g_kdrv_dce_info = (KDRV_DCE_PRAM *)input_addr;
	buffer_size = ALIGN_CEIL_64(channel_used * sizeof(KDRV_DCE_PRAM));
	DBG_IND("g_kdrv_dce_info 0x%08x\r\n", (unsigned int)g_kdrv_dce_info);

	g_kdrv_dce_handle_tab = (KDRV_DCE_HANDLE *)(input_addr + buffer_size);
	buffer_size += ALIGN_CEIL_64(channel_used * sizeof(KDRV_DCE_HANDLE));
	DBG_IND("g_kdrv_dce_handle_tab 0x%08x\r\n", (unsigned int)g_kdrv_dce_handle_tab);

	g_kdrv_dce_ipl_ctrl_en = (UINT32 *)(input_addr + buffer_size);
	buffer_size += ALIGN_CEIL_64(channel_used * sizeof(UINT32));
	DBG_IND("g_kdrv_dce_ipl_ctrl_en 0x%08x\r\n", (unsigned int)g_kdrv_dce_ipl_ctrl_en);

	g_kdrv_dce_iq_ctrl_en = (UINT32 *)(input_addr + buffer_size);
	buffer_size += ALIGN_CEIL_64(channel_used * sizeof(UINT32));
	DBG_IND("g_kdrv_dce_iq_ctrl_en 0x%08x\r\n", (unsigned int)g_kdrv_dce_iq_ctrl_en);

	g_kdrv_dce_norm = (DCE_DIST_NORM *)(input_addr + buffer_size);
	buffer_size += ALIGN_CEIL_64(channel_used * sizeof(DCE_DIST_NORM));
	DBG_IND("g_kdrv_dce_norm 0x%08x\r\n", (unsigned int)g_kdrv_dce_norm);

	g_kdrv_dce_stripe_mode = (DCE_STP_MODE *)(input_addr + buffer_size);
	buffer_size += ALIGN_CEIL_64(channel_used * sizeof(DCE_STP_MODE));
	DBG_IND("g_kdrv_dce_stripe_mode 0x%08x\r\n", (unsigned int)g_kdrv_dce_stripe_mode);

	g_kdrv_dce_load_flg = (BOOL **)(input_addr + buffer_size);
	buffer_size += ALIGN_CEIL_64(sizeof(BOOL **));
	for (i = 0; i < channel_used; i++) {
		g_kdrv_dce_load_flg[i] = (BOOL *)(input_addr + buffer_size);
		buffer_size += ALIGN_CEIL_64(sizeof(BOOL *));
	}
	for (i = 0; i < channel_used; i++) {
		g_kdrv_dce_load_flg[i][0] = (BOOL)(input_addr + buffer_size);
		buffer_size += ALIGN_CEIL_64(KDRV_DCE_PARAM_MAX * sizeof(BOOL));
	}
	DBG_IND("g_kdrv_dce_load_flg 0x%08x\r\n", (unsigned int)g_kdrv_dce_load_flg);

	gp_ori_2dlut_buffer = gp_2dlut_buffer = (UINT32 *)(input_addr + buffer_size);
	buffer_size += ALIGN_CEIL_64((LUT2D_MAX_TABLE_SIZE * 4) + 64);
	DBG_IND("gp_2dlut_buffer 0x%08x\r\n", (unsigned int)gp_2dlut_buffer);

	return buffer_size;
}

VOID kdrv_dce_buf_uninit(VOID)
{
	UINT32 i = 0;

	g_kdrv_dce_info         = NULL;
	g_kdrv_dce_handle_tab   = NULL;
	g_kdrv_dce_ipl_ctrl_en  = NULL;
	g_kdrv_dce_iq_ctrl_en   = NULL;
	g_kdrv_dce_norm         = NULL;
	g_kdrv_dce_stripe_mode  = NULL;

	for (i = 0; i < kdrv_dce_channel_num; i++) {
		g_kdrv_dce_load_flg[i] = NULL;
	}
	g_kdrv_dce_load_flg = NULL;


	kdrv_dce_channel_num = 0;
	kdrv_dce_lock_chls = 0;

	if (g_kdrv_dce_open_count == 0) {
		g_kdrv_dce_init_flg = 0;
	}

	gp_ori_2dlut_buffer = gp_2dlut_buffer = NULL;
}

INT32 kdrv_dce_open(UINT32 chip, UINT32 engine)
{
#if !defined (CONFIG_NVT_SMALL_HDAL)
	memset(g_kdrv_dce_ctrl_api_log, 0, sizeof(g_kdrv_dce_ctrl_api_log));
	log_idx = 0;
	total_log_idx = 0;

	snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): ", hwclock_get_longcounter(), total_log_idx, __func__);
	log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
	total_log_idx++;
#endif

	if (g_kdrv_dce_init_flg == 0) {
		kdrv_dce_handle_alloc_all(&g_kdrv_dce_init_flg);
	}
	if (g_kdrv_dce_init_flg == FALSE) {
		DBG_WRN("KDRV DCE: no free handle, max handle num = %d\r\n", kdrv_dce_channel_num);
		return E_SYS;
	}

	if (g_kdrv_dce_open_count < (INT32)kdrv_dce_channel_num && g_kdrv_dce_open_count > 0) {
	} else if (g_kdrv_dce_open_count == 0) {
		UINT32 i;
		DCE_OPENOBJ dce_drv_obj;
		DBG_IND("Count 0, open dce HW\r\n");

		dce_drv_obj.dce_clock_sel = g_dce_open_cfg.dce_clock_sel;
		dce_drv_obj.FP_DCEISR_CB = kdrv_dce_isr_cb;
		if (dce_open(&dce_drv_obj) != E_OK) {
			DBG_WRN("KDRV DCE: dce_open failed\r\n");
			return -1;
		}
		kdrv_dce_memset_drv_param();
		for (i = 0; i < kdrv_dce_channel_num; i++) {
			kdrv_dce_lock(&g_kdrv_dce_handle_tab[i], TRUE);
			kdrv_dce_init_param(i);
			kdrv_dce_lock(&g_kdrv_dce_handle_tab[i], FALSE);
		}
	} else if (g_kdrv_dce_open_count > (INT32)kdrv_dce_channel_num) {
		DBG_ERR("open too many times!\r\n");
		g_kdrv_dce_open_count = kdrv_dce_channel_num;
		return -1;
	}

	g_kdrv_dce_open_count++;

	return 0;
}

INT32 kdrv_dce_close(UINT32 chip, UINT32 engine)
{
	ER rt = E_OK;

#if !defined (CONFIG_NVT_SMALL_HDAL)
	snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): ", hwclock_get_longcounter(), total_log_idx, __func__);
	log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
	total_log_idx++;
#endif

	g_kdrv_dce_open_count--;

	if (g_kdrv_dce_open_count > 0) {
	} else if (g_kdrv_dce_open_count == 0) {
		UINT32 i;
		DBG_IND("Count 0, close dce HW\r\n");
		rt = dce_close();
		if (rt != E_OK) {
			DBG_ERR("DCE HW close fail!\r\n");
			return rt;
		}
		if (rt != E_OK) {
			DBG_ERR("DCE free 2dlut buffer fail!\r\n");
			return rt;
		}
		for (i = 0; i < kdrv_dce_channel_num; i++) {
			kdrv_dce_lock(&g_kdrv_dce_handle_tab[i], TRUE);
			kdrv_dce_handle_free(&g_kdrv_dce_handle_tab[i]);
			kdrv_dce_lock(&g_kdrv_dce_handle_tab[i], FALSE);
		}

		g_kdrv_dce_trig_hdl = NULL;

	} else if (g_kdrv_dce_open_count < 0) {
		DBG_ERR("close too many times!\r\n");
		g_kdrv_dce_open_count = 0;
		g_kdrv_dce_trig_hdl = NULL;
		rt = E_PAR;
	}

	return rt;
}

INT32 kdrv_dce_pause(UINT32 chip, UINT32 engine)
{
	ER rt = E_OK;

	rt = dce_pause();
	if (rt != E_OK) {
		return rt;
	}

	return rt;
}


INT32 kdrv_dce_trigger(UINT32 id, KDRV_DCE_TRIGGER_PARAM *p_dce_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	ER rt = E_ID;
	KDRV_DCE_HANDLE *p_handle;
	INT32 channel = KDRV_DEV_ID_CHANNEL(id);

#if !defined (CONFIG_NVT_SMALL_HDAL)
	snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): (%d)", hwclock_get_longcounter(), total_log_idx, __func__, ((KDRV_DCE_TRIG_TYPE_INFO *)p_user_data)->opmode);
	log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
	total_log_idx++;
#endif

	p_handle = kdrv_dce_entry_id_conv2_handle(channel);

	DBG_IND("id 0x%x\r\n", (unsigned int)id);

	kdrv_dce_sem(p_handle, TRUE);

	DBG_IND("0\r\n");

	//set dce kdrv parameters to dce driver
	//kdrv_dce_memset_drv_param();
	if (kdrv_dce_set_mode(channel, &g_dce_TotalInfo) != E_OK) {
		kdrv_dce_sem(p_handle, FALSE);
		return rt;
	}
	DBG_IND("cfa_enable %d\r\n", (unsigned int)g_kdrv_dce_info[channel].cfa_param.cfa_enable);

	DBG_IND("1\r\n");

	//update trig channel and trig_cfg
	g_kdrv_dce_trig_hdl = p_handle;
	g_kdrv_dce_info[channel].trig_cfg = *(KDRV_DCE_TRIG_TYPE_INFO *)p_user_data;

	kdrv_dce_frm_end(p_handle, FALSE);
	dce_clear_flag_frame_end();

	//calculate stripe
	if (g_kdrv_dce_info[channel].trig_cfg.opmode == KDRV_DCE_OP_MODE_CAL_STRIP) {
		DCE_NORMSTP_INFO dce_InputExtendInfo;
		UINT32 i;

		DBG_IND("cal stripe\r\n");

		dce_InputExtendInfo.stp_mode = g_kdrv_dce_stripe_mode[channel];
		dce_InputExtendInfo.sram_sel = g_kdrv_dce_info[channel].dc_sram_sel.sram_mode;
		dce_InputExtendInfo.in_width = g_kdrv_dce_info[channel].in_img_info.img_size_h;
		dce_InputExtendInfo.in_height = g_kdrv_dce_info[channel].in_img_info.img_size_v;

		dce_InputExtendInfo.crop_info.h_size = g_kdrv_dce_info[channel].out_img_info.crop_size_h;
		dce_InputExtendInfo.crop_info.v_size = g_kdrv_dce_info[channel].out_img_info.crop_size_v;
		dce_InputExtendInfo.crop_info.h_start = g_kdrv_dce_info[channel].out_img_info.crop_start_h;
		dce_InputExtendInfo.crop_info.v_start = g_kdrv_dce_info[channel].out_img_info.crop_start_v;

		if ((dce_InputExtendInfo.in_width != dce_InputExtendInfo.crop_info.h_size) || (dce_InputExtendInfo.in_height != dce_InputExtendInfo.crop_info.v_size)) {
			dce_InputExtendInfo.crop_en = TRUE;
		} else {
			dce_InputExtendInfo.crop_en = FALSE;
		}

		//extend info
		//map register selection to actual overlap size
		switch ((DCE_HSTP_IPEOLAP_SEL)g_dce_TotalInfo.h_stp_param.ipe_out_hstp_ovlap) {
		default:
		case DCE_HSTP_IPEOLAP_8:
			dce_InputExtendInfo.ipe_h_ovlap = 8;
			break;
		case DCE_HSTP_IPEOLAP_16:
			dce_InputExtendInfo.ipe_h_ovlap = 16;
			break;
		}
		switch ((DCE_HSTP_IMEOLAP_SEL)g_dce_TotalInfo.h_stp_param.ime_out_hstp_ovlap) {
		case DCE_HSTP_IMEOLAP_16:
			dce_InputExtendInfo.ime_h_ovlap = 16;
			break;
		case DCE_HSTP_IMEOLAP_24:
			dce_InputExtendInfo.ime_h_ovlap = 24;
			break;
		default:
		case DCE_HSTP_IMEOLAP_32:
			dce_InputExtendInfo.ime_h_ovlap = 32;
			break;
		case DCE_HSTP_IMEOLAP_USER:
			dce_InputExtendInfo.ime_h_ovlap = g_dce_TotalInfo.h_stp_param.ime_ovlap_user_val;
			break;
		}
		dce_InputExtendInfo.ime_isd_en = g_kdrv_dce_info[channel].extend_info.ime_isd_enable;
		dce_InputExtendInfo.ime_ref_out_en = g_kdrv_dce_info[channel].extend_info.ime_ref_out_enable;
		dce_InputExtendInfo.ime_p1_enc_en = g_kdrv_dce_info[channel].extend_info.ime_p1_enc_enable;
		dce_InputExtendInfo.ime_3dnr_en = g_kdrv_dce_info[channel].extend_info.ime_3dnr_enable;
		dce_InputExtendInfo.ime_enc_en = g_kdrv_dce_info[channel].extend_info.ime_enc_enable;
		dce_InputExtendInfo.ime_dec_en = g_kdrv_dce_info[channel].extend_info.ime_dec_enable;
		//dce_InputExtendInfo.ime_ssr_en = DISABLE;
		//dce_InputExtendInfo.ime_isd_en = ENABLE;

		//Size & param info
		g_dce_TotalInfo.img_in_size.h_size = g_kdrv_dce_info[channel].in_img_info.img_size_h;
		g_dce_TotalInfo.img_in_size.v_size = g_kdrv_dce_info[channel].in_img_info.img_size_v;
		g_dce_TotalInfo.img_out_crop.h_size = g_kdrv_dce_info[channel].out_img_info.crop_size_h;
		g_dce_TotalInfo.img_out_crop.v_size = g_kdrv_dce_info[channel].out_img_info.crop_size_v;
		g_dce_TotalInfo.img_out_crop.h_start = g_kdrv_dce_info[channel].out_img_info.crop_start_h;
		g_dce_TotalInfo.img_out_crop.v_start = g_kdrv_dce_info[channel].out_img_info.crop_start_v;
		g_dce_TotalInfo.op_param.op_mode = (DCE_OPMODE)g_kdrv_dce_info[channel].in_info.in_src;
		g_dce_TotalInfo.op_param.stp_mode = g_kdrv_dce_stripe_mode[channel];
		if (g_kdrv_dce_info[channel].in_img_info.cfa_pat < KDRV_IPP_RGGB_PIX_MAX) {
			g_dce_TotalInfo.cfa_param.cfa_pat = (DCE_CFA_PAT)g_kdrv_dce_info[channel].in_img_info.cfa_pat;
		} else if (g_kdrv_dce_info[channel].in_img_info.cfa_pat > KDRV_IPP_RGGB_PIX_MAX && g_kdrv_dce_info[channel].in_img_info.cfa_pat < KDRV_IPP_RGBIR_PIX_MAX) {
			g_dce_TotalInfo.cfa_param.cfa_pat = (DCE_CFA_PAT)(g_kdrv_dce_info[channel].in_img_info.cfa_pat - (KDRV_IPP_RGGB_PIX_MAX + 1));
		} else {
			DBG_ERR("CFA pattern %d out of bound!\r\n", (int)g_kdrv_dce_info[channel].in_img_info.cfa_pat);
			return E_PAR;
		}
		g_dce_TotalInfo.d2d_param.d2d_fmt = (DCE_D2D_FMT)g_kdrv_dce_info[channel].in_info.type;

		if (g_kdrv_dce_info[channel].stripe_param.stripe_type == KDRV_DCE_STRIPE_CUSTOMER) {

			DBG_IND("stripe setting by customer setting\r\n");
			for (i = 0; i < DCE_PARTITION_NUM; i++) {
				g_dce_strip_result.gdc_h_stp[i] = g_kdrv_dce_info[channel].stripe_param.hstp[i];
				DBG_IND("hstp[%d] = %d\r\n", (unsigned int)i, (unsigned int)g_kdrv_dce_info[channel].stripe_param.hstp[i]);
			}

		} else {
			DBG_IND("dce_get_norm_stripe\r\n");
			b_kdrv_dce_cal_strip_flow = TRUE;
			rt = dce_get_norm_stripe(&dce_InputExtendInfo, &g_dce_strip_result, &g_dce_TotalInfo);
			b_kdrv_dce_cal_strip_flow = FALSE;

			if (rt != E_OK) {
				DBG_ERR("Cal stripe error!!\r\n");
				kdrv_dce_sem(p_handle, FALSE);
				return rt;
			}
		}

		DBG_IND("get ipl stripe result\r\n");
		dce_get_output_stp_setting(&dce_InputExtendInfo, &g_dce_strip_result);

		g_kdrv_dce_info[channel].ipl_stripe.hstp_num = g_dce_strip_result.ui_hstp_num;
		for (i = 0; i < IMG_MAX_STP_NUM; i ++) {
			if (i < g_dce_strip_result.ui_hstp_num) {
				g_kdrv_dce_info[channel].ipl_stripe.hstp[i] = g_dce_strip_result.ui_dce_out_hstp[i];
				g_kdrv_dce_info[channel].ipl_stripe.ime_in_hstp[i] = g_dce_strip_result.ui_ime_in_hstp[i];
				g_kdrv_dce_info[channel].ipl_stripe.ime_out_hstp[i] = g_dce_strip_result.ui_ime_out_hstp[i];
			} else {
				g_kdrv_dce_info[channel].ipl_stripe.hstp[i] = 0;
				g_kdrv_dce_info[channel].ipl_stripe.ime_in_hstp[i] = 0;
				g_kdrv_dce_info[channel].ipl_stripe.ime_out_hstp[i] = 0;
			}
		}
		kdrv_dce_sem(p_handle, FALSE);
	} else {
		//trigger dce start
		DBG_IND("dce start\r\n");
		rt = dce_start();
		if (rt != E_OK) {
			kdrv_dce_sem(p_handle, FALSE);
			return rt;
		}
	}

	DBG_IND("end\r\n");

	return rt;
}

static ER kdrv_dce_set_opencfg(UINT32 id, void *data)
{
	KDRV_DCE_OPENCFG kdrv_dce_open_obj;

	if (data == NULL) {
		DBG_ERR("id %d input data NULL!\r\n", (unsigned int)id);
		return E_PAR;
	}
	kdrv_dce_open_obj = *(KDRV_DCE_OPENCFG *)data;
	g_dce_open_cfg.dce_clock_sel = kdrv_dce_open_obj.dce_clock_sel;

	return E_OK;
}

static ER kdrv_dce_set_in_info(UINT32 id, void *data)
{
	g_kdrv_dce_info[id].in_info = *(KDRV_DCE_IN_INFO *)data;
	return E_OK;
}

static ER kdrv_dce_set_in_addr(UINT32 id, void *data)
{
	g_kdrv_dce_info[id].in_dma_info = *(KDRV_DCE_IMG_IN_DMA_INFO *)data;

	if ((g_kdrv_dce_info[id].in_dma_info.addr_y == 0) || (g_kdrv_dce_info[id].in_dma_info.addr_cbcr == 0)) {
		DBG_ERR("KDRV DCE:Input address should not be zero\r\n");
		return E_PAR;
	} else if ((g_kdrv_dce_info[id].in_dma_info.addr_y & 0x3) || (g_kdrv_dce_info[id].in_dma_info.addr_cbcr & 0x3)) {
		DBG_ERR("KDRV DCE: Input address should be word alignment\r\n");
		return E_PAR;
	}

#if 0
	if (dma_isCacheAddr(g_kdrv_dce_info[id].in_dma_info.addr_y)) {
		DBG_ERR("DAL DEC: ID%d set Invaid Input Address_Y: %#x, address should be non_cacheable\r\n", id + 1, g_kdrv_dce_info[id].in_dma_info.addr_y);
		return E_PAR;
	}
	if (dma_isCacheAddr(g_kdrv_dce_info[id].in_dma_info.addr_cbcr)) {
		DBG_ERR("DAL DEC: ID%d set Invaid Input Address_CbCr: %#x, address should be non_cacheable\r\n", id + 1, g_kdrv_dce_info[id].in_dma_info.addr_cbcr);
		return E_PAR;
	}
#endif

	return E_OK;
}

static ER kdrv_dce_set_in_img_info(UINT32 id, void *data)
{
	g_kdrv_dce_info[id].in_img_info = *(KDRV_DCE_IN_IMG_INFO *)data;
	return E_OK;
}

static ER kdrv_dce_set_out_addr(UINT32 id, void *data)
{
	g_kdrv_dce_info[id].out_dma_info = *(KDRV_DCE_IMG_OUT_DMA_INFO *)data;

	if ((g_kdrv_dce_info[id].out_dma_info.addr_y == 0) || (g_kdrv_dce_info[id].out_dma_info.addr_cbcr == 0)) {
		DBG_ERR("KDRV DCE: Output address should not be zero\r\n");
		return E_PAR;
	} else if ((g_kdrv_dce_info[id].out_dma_info.addr_y & 0x3) || (g_kdrv_dce_info[id].out_dma_info.addr_cbcr & 0x3)) {
		DBG_ERR("KDRV DEC:Output address should be word alignment\r\n");
		return E_PAR;
	}

#if 0
	if (dma_isCacheAddr(g_kdrv_dce_info[id].out_dma_info.addr_y)) {
		DBG_ERR("DAL DEC: ID%d set Invaid Output Address_Y: %#x, address should be non_cacheable\r\n", id + 1, g_kdrv_dce_info[id].out_dma_info.addr_y);
		return E_PAR;
	}
	if (dma_isCacheAddr(g_kdrv_dce_info[id].out_dma_info.addr_cbcr)) {
		DBG_ERR("DAL DEC: ID%d set Invaid Output Address_CbCr: %#x, address should be non_cacheable\r\n", id + 1, g_kdrv_dce_info[id].out_dma_info.addr_cbcr);
		return E_PAR;
	}
#endif
	return E_OK;
}

static ER kdrv_dce_set_out_img_info(UINT32 id, void *data)
{
	g_kdrv_dce_info[id].out_img_info = *(KDRV_DCE_OUT_IMG_INFO *)data;
	return E_OK;
}

static ER kdrv_dce_set_subin_addr(UINT32 id, void *data)
{
	g_kdrv_dce_info[id].wdr_subin_addr = *(KDRV_DCE_SUBIMG_IN_ADDR *)data;

	return E_OK;
}

static ER kdrv_dce_set_subout_addr(UINT32 id, void *data)
{
	g_kdrv_dce_info[id].wdr_subout_addr = *(KDRV_DCE_SUBIMG_OUT_ADDR *)data;

	return E_OK;
}

static ER kdrv_dce_set_dram_out_mode(UINT32 id, void *data)
{

	g_kdrv_dce_info[id].dram_out_mode = *(KDRV_DCE_DRAM_OUTPUT_MODE *)data;

	return E_OK;
}

static ER kdrv_dce_set_single_out_channel(UINT32 id, void *data)
{

	g_kdrv_dce_info[id].single_out_ch = *(KDRV_DCE_DRAM_SINGLE_OUT_EN *)data;

	return E_OK;
}

static ER kdrv_dce_set_isrcb(UINT32 id, void *data)
{
	KDRV_DCE_HANDLE *p_handle = kdrv_dce_entry_id_conv2_handle(id);
	p_handle->isrcb_fp = (KDRV_IPP_ISRCB)data;

	return E_OK;
}

static ER kdrv_dce_set_interrupt_enable(UINT32 id, void *data)
{
	g_kdrv_dce_info[id].inte_en = *(KDRV_DCE_INTE *)data;
	return E_OK;
}


static ER kdrv_dce_set_extend_info(UINT32 id, void *data)
{
	g_kdrv_dce_info[id].extend_info = *(KDRV_DCE_EXTEND_INFO *)data;
	return E_OK;
}

static ER kdrv_dce_set_gdc_center_info(UINT32 id, void *data)
{
	g_kdrv_dce_info[id].gdc_center = *(KDRV_DCE_GDC_CENTER_INFO *)data;
	return E_OK;
}

static ER kdrv_dce_set_dc_sram_sel(UINT32 id, void *data)
{
	g_kdrv_dce_info[id].dc_sram_sel = *(KDRV_DCE_DC_SRAM_SEL *)data;
	return E_OK;
}

static ER kdrv_dce_set_cfa_subout_info(UINT32 id, void *data)
{
	KDRV_DCE_CFA_SUBOUT_INFO *p_temp_info;

	p_temp_info = (KDRV_DCE_CFA_SUBOUT_INFO *)data;

	if (p_temp_info->cfa_subout_enable == TRUE) {
		g_kdrv_dce_info[id].cfa_subout_info = *p_temp_info;
	}
	return E_OK;
}

static ER kdrv_dce_set_stripe_rule_info(UINT32 id, void *data)
{
	g_kdrv_dce_info[id].stripe_rule_info = *(KDRV_DCE_STRP_RULE_INFO *)data;
	return E_OK;
}

static ER kdrv_dce_set_cfa_param(UINT32 id, void *data)
{
	UINT32 i;
	KDRV_DCE_CFA_PARAM *p_temp_info;

	p_temp_info = (KDRV_DCE_CFA_PARAM *)data;

	if (p_temp_info->cfa_enable) {
		g_kdrv_dce_iq_ctrl_en[id] |= KDRV_DCE_IQ_FUNC_CFA;
	} else {
		g_kdrv_dce_iq_ctrl_en[id] &= (~KDRV_DCE_IQ_FUNC_CFA);
	}

	if (p_temp_info->cfa_enable) {
		g_kdrv_dce_info[id].cfa_param.cfa_interp.edge_dth = p_temp_info->cfa_interp.edge_dth;
		g_kdrv_dce_info[id].cfa_param.cfa_interp.edge_dth2 = p_temp_info->cfa_interp.edge_dth2;
		g_kdrv_dce_info[id].cfa_param.cfa_interp.freq_th = p_temp_info->cfa_interp.freq_th;
		for (i = 0; i < KDRV_CFA_FREQ_NUM; i++) {
			g_kdrv_dce_info[id].cfa_param.cfa_interp.freq_lut[i] = p_temp_info->cfa_interp.freq_lut[i];
		}
		for (i = 0; i < KDRV_CFA_LUMA_NUM; i++) {
			g_kdrv_dce_info[id].cfa_param.cfa_interp.luma_wt[i] = p_temp_info->cfa_interp.luma_wt[i];
		}

		g_kdrv_dce_info[id].cfa_param.cfa_correction.rb_corr_enable = p_temp_info->cfa_correction.rb_corr_enable;
		g_kdrv_dce_info[id].cfa_param.cfa_correction.rb_corr_th1 = p_temp_info->cfa_correction.rb_corr_th1;
		g_kdrv_dce_info[id].cfa_param.cfa_correction.rb_corr_th2 = p_temp_info->cfa_correction.rb_corr_th2;

		g_kdrv_dce_info[id].cfa_param.cfa_fcs.fcs_dirsel = p_temp_info->cfa_fcs.fcs_dirsel;
		g_kdrv_dce_info[id].cfa_param.cfa_fcs.fcs_coring = p_temp_info->cfa_fcs.fcs_coring;
		g_kdrv_dce_info[id].cfa_param.cfa_fcs.fcs_weight = p_temp_info->cfa_fcs.fcs_weight;
		for (i = 0; i < KDRV_CFA_FCS_NUM; i++) {
			g_kdrv_dce_info[id].cfa_param.cfa_fcs.fcs_strength[i] = p_temp_info->cfa_fcs.fcs_strength[i];
		}

		g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.cl_check_enable = p_temp_info->cfa_ir_hfc.cl_check_enable;
		g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.hf_check_enable = p_temp_info->cfa_ir_hfc.hf_check_enable;
		g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.average_mode = p_temp_info->cfa_ir_hfc.average_mode;
		g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.cl_sel = p_temp_info->cfa_ir_hfc.cl_sel;
		g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.cl_th = p_temp_info->cfa_ir_hfc.cl_th;
		g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.hfg_th = p_temp_info->cfa_ir_hfc.hfg_th;
		g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.hf_diff = p_temp_info->cfa_ir_hfc.hf_diff;
		g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.hfe_th = p_temp_info->cfa_ir_hfc.hfe_th;
		g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.ir_g_edge_th = p_temp_info->cfa_ir_hfc.ir_g_edge_th;
		g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.ir_rb_cstrength = p_temp_info->cfa_ir_hfc.ir_rb_cstrength;

		g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_r = p_temp_info->cfa_ir_sub.ir_sub_r;
		g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_g = p_temp_info->cfa_ir_sub.ir_sub_g;
		g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_b = p_temp_info->cfa_ir_sub.ir_sub_b;
		g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_wt_lb = p_temp_info->cfa_ir_sub.ir_sub_wt_lb;
		g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_th = p_temp_info->cfa_ir_sub.ir_sub_th;
		g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sub_range = p_temp_info->cfa_ir_sub.ir_sub_range;
		g_kdrv_dce_info[id].cfa_param.cfa_ir_sub.ir_sat_gain = p_temp_info->cfa_ir_sub.ir_sat_gain;

		g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_en = p_temp_info->cfa_pink_reduc.pink_rd_en;
		g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_mode = p_temp_info->cfa_pink_reduc.pink_rd_mode;
		g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_th1 = p_temp_info->cfa_pink_reduc.pink_rd_th1;
		g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_th2 = p_temp_info->cfa_pink_reduc.pink_rd_th2;
		g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_th3 = p_temp_info->cfa_pink_reduc.pink_rd_th3;
		g_kdrv_dce_info[id].cfa_param.cfa_pink_reduc.pink_rd_th4 = p_temp_info->cfa_pink_reduc.pink_rd_th4;

		g_kdrv_dce_info[id].cfa_param.cfa_cgain.r_gain = p_temp_info->cfa_cgain.r_gain;
		g_kdrv_dce_info[id].cfa_param.cfa_cgain.g_gain = p_temp_info->cfa_cgain.g_gain;
		g_kdrv_dce_info[id].cfa_param.cfa_cgain.b_gain = p_temp_info->cfa_cgain.b_gain;
		g_kdrv_dce_info[id].cfa_param.cfa_cgain.gain_range = p_temp_info->cfa_cgain.gain_range;
	}
	return E_OK;
}

static ER kdrv_dce_set_dc_cac_param(UINT32 id, void *data)
{
	UINT32 i;
	KDRV_DCE_DC_CAC_PARAM *p_temp_info;

	p_temp_info = (KDRV_DCE_DC_CAC_PARAM *)data;

	if (p_temp_info->dc_enable) {
		g_kdrv_dce_iq_ctrl_en[id] |= KDRV_DCE_IQ_FUNC_DC;
	} else {
		g_kdrv_dce_iq_ctrl_en[id] &= (~KDRV_DCE_IQ_FUNC_DC);
	}

	if (p_temp_info->cac.cac_enable) {
		g_kdrv_dce_iq_ctrl_en[id] |= KDRV_DCE_IQ_FUNC_CAC;
	} else {
		g_kdrv_dce_iq_ctrl_en[id] &= (~KDRV_DCE_IQ_FUNC_CAC);
	}

	if (p_temp_info->dc_enable) {
		g_kdrv_dce_info[id].dc_cac_param.dc_y_dist_off = p_temp_info->dc_y_dist_off;
		g_kdrv_dce_info[id].dc_cac_param.dc.geo_dist_x = p_temp_info->dc.geo_dist_x;
		g_kdrv_dce_info[id].dc_cac_param.dc.geo_dist_y = p_temp_info->dc.geo_dist_y;
		for (i = 0; i < GDC_TABLE_NUM; i++) {
			g_kdrv_dce_info[id].dc_cac_param.dc.geo_lut_g[i] = p_temp_info->dc.geo_lut_g[i];
		}
		//table limitation check
		for (i = 1; i < GDC_TABLE_NUM; i++) {
			if (abs(g_kdrv_dce_info[id].dc_cac_param.dc.geo_lut_g[i - 1] - g_kdrv_dce_info[id].dc_cac_param.dc.geo_lut_g[i]) > 2047) {
				DBG_ERR("G LUT adjacent two entry's difference can not be > 2047\r\n");
				DBG_ERR("abs(LUT[%d]-LUT[%d]) = %ld\r\n", (unsigned int)(i - 1), (unsigned int)i, (long int)abs(g_kdrv_dce_info[id].dc_cac_param.dc.geo_lut_g[i - 1] - g_kdrv_dce_info[id].dc_cac_param.dc.geo_lut_g[i]));
				return E_PAR;
			}
		}
	}

	if (p_temp_info->cac.cac_enable) {
		g_kdrv_dce_info[id].dc_cac_param.cac.cac_sel= p_temp_info->cac.cac_sel;
		g_kdrv_dce_info[id].dc_cac_param.cac.geo_r_lut_gain = p_temp_info->cac.geo_r_lut_gain;
		g_kdrv_dce_info[id].dc_cac_param.cac.geo_g_lut_gain = p_temp_info->cac.geo_g_lut_gain;
		g_kdrv_dce_info[id].dc_cac_param.cac.geo_b_lut_gain = p_temp_info->cac.geo_b_lut_gain;
		for (i = 0; i < CAC_TABLE_NUM; i++) {
			g_kdrv_dce_info[id].dc_cac_param.cac.geo_lut_r[i] = p_temp_info->cac.geo_lut_r[i];
			g_kdrv_dce_info[id].dc_cac_param.cac.geo_lut_b[i] = p_temp_info->cac.geo_lut_b[i];
		}
	}

	return E_OK;
}

static ER kdrv_dce_set_2d_lut_param(UINT32 id, void *data)
{
	UINT32 i;
	KDRV_DCE_2DLUT_PARAM *p_temp_info;

	p_temp_info = (KDRV_DCE_2DLUT_PARAM *)data;

	if (p_temp_info->lut_2d_enable) {
		g_kdrv_dce_iq_ctrl_en[id] |= KDRV_DCE_IQ_FUNC_2DLUT;
	} else {
		g_kdrv_dce_iq_ctrl_en[id] &= (~KDRV_DCE_IQ_FUNC_2DLUT);
	}

	if (p_temp_info->lut_2d_enable) {
		g_kdrv_dce_info[id].lut_2d_param.lut_2d_y_dist_off = p_temp_info->lut_2d_y_dist_off;
		g_kdrv_dce_info[id].lut_2d_param.lut_num_select = p_temp_info->lut_num_select;
		for (i = 0; i < LUT2D_MAX_TABLE_SIZE; i++) {
			g_kdrv_dce_info[id].lut_2d_param.lut_2d_value[i] = p_temp_info->lut_2d_value[i];
		}
	}
	return E_OK;
}

static ER kdrv_dce_set_fov_param(UINT32 id, void *data)
{
	KDRV_DCE_FOV_PARAM *p_temp_info;

	p_temp_info = (KDRV_DCE_FOV_PARAM *)data;

	if (p_temp_info->fov_enable) {
		g_kdrv_dce_iq_ctrl_en[id] |= KDRV_DCE_IQ_FUNC_FOV;
	} else {
		g_kdrv_dce_iq_ctrl_en[id] &= (~KDRV_DCE_IQ_FUNC_FOV);
	}

	if (p_temp_info->fov_enable) {
		g_kdrv_dce_info[id].fov_param.fov_gain = p_temp_info->fov_gain;
		g_kdrv_dce_info[id].fov_param.fov_bnd_sel = p_temp_info->fov_bnd_sel;
		g_kdrv_dce_info[id].fov_param.fov_bnd_r = p_temp_info->fov_bnd_r;
		g_kdrv_dce_info[id].fov_param.fov_bnd_g = p_temp_info->fov_bnd_g;
		g_kdrv_dce_info[id].fov_param.fov_bnd_b = p_temp_info->fov_bnd_b;
	} else {
		g_kdrv_dce_info[id].fov_param.fov_gain = 1024;
		g_kdrv_dce_info[id].fov_param.fov_bnd_sel = KDRV_DCE_FOV_BND_DUPLICATE;
		g_kdrv_dce_info[id].fov_param.fov_bnd_r = 0;
		g_kdrv_dce_info[id].fov_param.fov_bnd_g = 0;
		g_kdrv_dce_info[id].fov_param.fov_bnd_b = 0;
	}

	return E_OK;
}

static ER kdrv_dce_set_stripe_param(UINT32 id, void *data)
{
	KDRV_DCE_STRIPE_PARAM *dce_stripe;
	INT32 i;

	dce_stripe = (KDRV_DCE_STRIPE_PARAM *)data;

	g_kdrv_dce_info[id].stripe_param.stripe_type = dce_stripe->stripe_type;
	DBG_IND("stripe type %d\r\n", (unsigned int)g_kdrv_dce_info[id].stripe_param.stripe_type);

	for (i = 0; i < DCE_PARTITION_NUM; i++) {
		g_kdrv_dce_info[id].stripe_param.hstp[i] = dce_stripe->hstp[i];
		DBG_IND("dce_stripe hstp[%d] %d\r\n", (unsigned int)i, (unsigned int)dce_stripe->hstp[i]);
	}

	return E_OK;
}


static ER kdrv_dce_set_wdr_subimg_param(UINT32 id, void *data)
{
	g_kdrv_dce_info[id].wdr_subimg_param = *(KDRV_DCE_WDR_SUBIMG_PARAM *)data;
	return E_OK;
}

static ER kdrv_dce_set_wdr_param(UINT32 id, void *data)
{
	KDRV_DCE_WDR_PARAM *p_temp_info;
	INT32 i;

	p_temp_info = (KDRV_DCE_WDR_PARAM *)data;

	if (p_temp_info->wdr_enable) {
		g_kdrv_dce_iq_ctrl_en[id] |= KDRV_DCE_IQ_FUNC_WDR;
	} else {
		g_kdrv_dce_iq_ctrl_en[id] &= (~KDRV_DCE_IQ_FUNC_WDR);
	}
	if (p_temp_info->tonecurve_enable) {
		g_kdrv_dce_iq_ctrl_en[id] |= KDRV_DCE_IQ_FUNC_TONECURVE;
	} else {
		g_kdrv_dce_iq_ctrl_en[id] &= (~KDRV_DCE_IQ_FUNC_TONECURVE);
	}

	if (p_temp_info->wdr_enable == TRUE || p_temp_info->tonecurve_enable == TRUE) {

		for (i = 0; i < WDR_SUBIMG_FILT_NUM; i++) {
			g_kdrv_dce_info[id].wdr_param.ftrcoef[i] = p_temp_info->ftrcoef[i];
		}

		for (i = 0; i < WDR_COEF_NUM; i++) {
			g_kdrv_dce_info[id].wdr_param.wdr_str.wdr_coeff[i] = p_temp_info->wdr_str.wdr_coeff[i];
		}
		g_kdrv_dce_info[id].wdr_param.wdr_str.strength = p_temp_info->wdr_str.strength;
		g_kdrv_dce_info[id].wdr_param.wdr_str.contrast = p_temp_info->wdr_str.contrast;

		g_kdrv_dce_info[id].wdr_param.gainctrl.gainctrl_en = p_temp_info->gainctrl.gainctrl_en;
		g_kdrv_dce_info[id].wdr_param.gainctrl.max_gain = p_temp_info->gainctrl.max_gain;
		g_kdrv_dce_info[id].wdr_param.gainctrl.min_gain = p_temp_info->gainctrl.min_gain;

		g_kdrv_dce_info[id].wdr_param.input_bld.bld_sel = p_temp_info->input_bld.bld_sel;
		g_kdrv_dce_info[id].wdr_param.input_bld.bld_wt = p_temp_info->input_bld.bld_wt;
		for (i = 0; i < WDR_INPUT_BLD_NUM; i++) {
			g_kdrv_dce_info[id].wdr_param.input_bld.blend_lut[i] = p_temp_info->input_bld.blend_lut[i];
		}

		g_kdrv_dce_info[id].wdr_param.outbld.outbld_en = p_temp_info->outbld.outbld_en;
		for (i = 0; i < WDR_NEQ_TABLE_IDX_NUM; i++) {
			g_kdrv_dce_info[id].wdr_param.tonecurve.lut_idx[i] = p_temp_info->tonecurve.lut_idx[i];
			g_kdrv_dce_info[id].wdr_param.tonecurve.lut_split[i] = p_temp_info->tonecurve.lut_split[i];
			g_kdrv_dce_info[id].wdr_param.outbld.outbld_lut.lut_idx[i] = p_temp_info->outbld.outbld_lut.lut_idx[i];
			g_kdrv_dce_info[id].wdr_param.outbld.outbld_lut.lut_split[i] = p_temp_info->outbld.outbld_lut.lut_split[i];
		}
		for (i = 0; i < WDR_NEQ_TABLE_VAL_NUM; i++) {
			g_kdrv_dce_info[id].wdr_param.tonecurve.lut_val[i] = p_temp_info->tonecurve.lut_val[i];
			g_kdrv_dce_info[id].wdr_param.outbld.outbld_lut.lut_val[i] = p_temp_info->outbld.outbld_lut.lut_val[i];
		}

		g_kdrv_dce_info[id].wdr_param.sat_reduct.sat_th = p_temp_info->sat_reduct.sat_th;
		g_kdrv_dce_info[id].wdr_param.sat_reduct.sat_wt_low = p_temp_info->sat_reduct.sat_wt_low;
		g_kdrv_dce_info[id].wdr_param.sat_reduct.sat_delta = p_temp_info->sat_reduct.sat_delta;


	}
	return E_OK;
}

static ER kdrv_dce_set_hist_param(UINT32 id, void *data)
{
	KDRV_DCE_HIST_PARAM *p_temp_info;

	p_temp_info = (KDRV_DCE_HIST_PARAM *)data;

	if (p_temp_info->hist_enable) {
		g_kdrv_dce_iq_ctrl_en[id] |= KDRV_DCE_IQ_FUNC_HISTOGRAM;
	} else {
		g_kdrv_dce_iq_ctrl_en[id] &= (~KDRV_DCE_IQ_FUNC_HISTOGRAM);
	}

	if (p_temp_info->hist_enable) {
		g_kdrv_dce_info[id].hist_param.hist_sel = p_temp_info->hist_sel;
		g_kdrv_dce_info[id].hist_param.step_h = p_temp_info->step_h;
		g_kdrv_dce_info[id].hist_param.step_v = p_temp_info->step_v;
	}
	return E_OK;
}



static ER kdrv_dce_set_iq_all(UINT32 id, void *data)//For IQ
{
	ER rt_val = E_OK;
	unsigned long myflags;
	KDRV_DCE_PARAM_IQ_ALL_PARAM  *iq_all = (KDRV_DCE_PARAM_IQ_ALL_PARAM *)data;

	if (iq_all->p_cfa_param != NULL) {
		rt_val = kdrv_dce_set_cfa_param(id, (void *)iq_all->p_cfa_param);
		myflags = dce_platform_spin_lock();
		g_kdrv_dce_load_flg[id][KDRV_DCE_PARAM_IQ_CFA]  = TRUE;
		dce_platform_spin_unlock(myflags);
	}

	if (iq_all->p_dc_cac_param != NULL) {
		rt_val = kdrv_dce_set_dc_cac_param(id, (void *)iq_all->p_dc_cac_param);
		myflags = dce_platform_spin_lock();
		g_kdrv_dce_load_flg[id][KDRV_DCE_PARAM_IQ_DC_CAC]  = TRUE;
		dce_platform_spin_unlock(myflags);
	}

	if (iq_all->p_2dlut_param != NULL) {
		rt_val = kdrv_dce_set_2d_lut_param(id, (void *)iq_all->p_2dlut_param);
		myflags = dce_platform_spin_lock();
		g_kdrv_dce_load_flg[id][KDRV_DCE_PARAM_IQ_2DLUT]  = TRUE;
		dce_platform_spin_unlock(myflags);
	}

	if (iq_all->p_fov_param != NULL) {
		rt_val = kdrv_dce_set_fov_param(id, (void *)iq_all->p_fov_param);
		myflags = dce_platform_spin_lock();
		g_kdrv_dce_load_flg[id][KDRV_DCE_PARAM_IQ_FOV]  = TRUE;
		dce_platform_spin_unlock(myflags);
	}

	if (iq_all->p_stripe_param != NULL) {
		rt_val = kdrv_dce_set_stripe_param(id, (void *)iq_all->p_stripe_param);
		myflags = dce_platform_spin_lock();
		g_kdrv_dce_load_flg[id][KDRV_DCE_PARAM_IQ_STRIPE_PARAM]  = TRUE;
		dce_platform_spin_unlock(myflags);
	}

	if (iq_all->p_wdr_param != NULL) {
		rt_val = kdrv_dce_set_wdr_param(id, (void *)iq_all->p_wdr_param);
		myflags = dce_platform_spin_lock();
		g_kdrv_dce_load_flg[id][KDRV_DCE_PARAM_IQ_WDR]  = TRUE;
		dce_platform_spin_unlock(myflags);
	}

	if (iq_all->p_wdr_subimg_param != NULL) {
		rt_val = kdrv_dce_set_wdr_subimg_param(id, (void *)iq_all->p_wdr_subimg_param);
		myflags = dce_platform_spin_lock();
		g_kdrv_dce_load_flg[id][KDRV_DCE_PARAM_IQ_WDR_SUBIMG]  = TRUE;
		dce_platform_spin_unlock(myflags);
	}

	if (iq_all->p_hist_param != NULL) {
		rt_val = kdrv_dce_set_hist_param(id, (void *)iq_all->p_hist_param);
		myflags = dce_platform_spin_lock();
		g_kdrv_dce_load_flg[id][KDRV_DCE_PARAM_IQ_HIST]  = TRUE;
		dce_platform_spin_unlock(myflags);
	}

	return rt_val;
}

static ER kdrv_dce_set_en_all(UINT32 id, void *data)//For IPL control
{
	unsigned long myflags;
	KDRV_DCE_PARAM_IPL_FUNC_EN  *ipl_ctrl = (KDRV_DCE_PARAM_IPL_FUNC_EN *)data;

	UINT32 dce_ipl_func_en_sel_temp = 0, dce_ipl_func_en_diff = 0;

	DBG_IND("called\r\n");

	dce_ipl_func_en_sel_temp = g_kdrv_dce_ipl_ctrl_en[id];

	if (ipl_ctrl->enable) {
		g_kdrv_dce_ipl_ctrl_en[id] |= ipl_ctrl->ipl_ctrl_func;
	} else {
		g_kdrv_dce_ipl_ctrl_en[id] &= (~ipl_ctrl->ipl_ctrl_func);
	}

	dce_ipl_func_en_diff = (dce_ipl_func_en_sel_temp ^ g_kdrv_dce_ipl_ctrl_en[id]);

	myflags = dce_platform_spin_lock();
	if (dce_ipl_func_en_diff & KDRV_DCE_IQ_FUNC_CFA)  {
		g_kdrv_dce_load_flg[id][KDRV_DCE_PARAM_IQ_CFA]  = TRUE;
	}

	if ((dce_ipl_func_en_diff & KDRV_DCE_IQ_FUNC_DC) ||
		(dce_ipl_func_en_diff & KDRV_DCE_IQ_FUNC_CAC)) {
		g_kdrv_dce_load_flg[id][KDRV_DCE_PARAM_IQ_DC_CAC] = TRUE;
	}
	if (dce_ipl_func_en_diff & KDRV_DCE_IQ_FUNC_2DLUT)  {
		g_kdrv_dce_load_flg[id][KDRV_DCE_PARAM_IQ_2DLUT] = TRUE;
	}
	if (dce_ipl_func_en_diff & KDRV_DCE_IQ_FUNC_FOV)  {
		g_kdrv_dce_load_flg[id][KDRV_DCE_PARAM_IQ_FOV] = TRUE;
	}
	if ((dce_ipl_func_en_diff & KDRV_DCE_IQ_FUNC_WDR) ||
		(dce_ipl_func_en_diff & KDRV_DCE_IQ_FUNC_TONECURVE))  {
		g_kdrv_dce_load_flg[id][KDRV_DCE_PARAM_IQ_WDR] = TRUE;
		g_kdrv_dce_load_flg[id][KDRV_DCE_PARAM_IQ_WDR_SUBIMG] = TRUE;
	}
	if (dce_ipl_func_en_diff & KDRV_DCE_IQ_FUNC_HISTOGRAM)  {
		g_kdrv_dce_load_flg[id][KDRV_DCE_PARAM_IQ_HIST] = TRUE;
	}
	dce_platform_spin_unlock(myflags);

	return E_OK;
}

static ER kdrv_dce_set_reset(UINT32 id, void *data)
{
	KDRV_DCE_HANDLE *p_handle;

	dce_reset(TRUE);

	p_handle = kdrv_dce_entry_id_conv2_handle(id);
	if (p_handle == NULL) {
		DBG_ERR("get handle error!\r\n");
	}
	kdrv_dce_sem(p_handle, FALSE);

	return E_OK;
}

static ER kdrv_dce_set_builtin_flow_disable(UINT32 id, void *data)
{
	dce_set_builtin_flow_disable();

	return E_OK;
}

KDRV_DCE_SET_FP kdrv_dce_set_fp[KDRV_DCE_PARAM_MAX] = {
	kdrv_dce_set_opencfg,
	kdrv_dce_set_in_info,
	kdrv_dce_set_in_img_info,
	kdrv_dce_set_out_img_info,
	kdrv_dce_set_in_addr,
	kdrv_dce_set_out_addr,
	kdrv_dce_set_subin_addr,
	kdrv_dce_set_subout_addr,

	kdrv_dce_set_dram_out_mode,
	kdrv_dce_set_single_out_channel,

	kdrv_dce_set_gdc_center_info,
	kdrv_dce_set_dc_sram_sel,
	NULL,
	kdrv_dce_set_isrcb,
	kdrv_dce_set_interrupt_enable,
	kdrv_dce_set_load,
	kdrv_dce_set_extend_info,
	kdrv_dce_set_cfa_subout_info,
	kdrv_dce_set_stripe_rule_info,
	kdrv_dce_set_cfa_param,
	kdrv_dce_set_dc_cac_param,
	kdrv_dce_set_2d_lut_param,
	kdrv_dce_set_fov_param,
	kdrv_dce_set_stripe_param,

	kdrv_dce_set_wdr_subimg_param,
	kdrv_dce_set_wdr_param,
	kdrv_dce_set_hist_param,
	NULL,
	NULL,

	kdrv_dce_set_iq_all,
	kdrv_dce_set_en_all,
	kdrv_dce_set_reset,
	kdrv_dce_set_builtin_flow_disable
} ;

INT32 kdrv_dce_set(UINT32 id, KDRV_DCE_PARAM_ID param_id, VOID *p_param)
{
	ER rt = E_OK;
	unsigned long myflags;
	KDRV_DCE_HANDLE *p_handle;
	UINT32 ign_chk;
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);
	UINT32 engine = KDRV_DEV_ID_ENGINE(id);

	if (engine != KDRV_VIDEOPROCS_DCE_ENGINE0) {
		DBG_ERR("Invalid engine 0x%x\r\n", (int)engine);
		return E_ID;
	}
	ign_chk = (KDRV_DCE_IGN_CHK & param_id);
	param_id = param_id & (~KDRV_DCE_IGN_CHK);

	if (param_id == KDRV_DCE_PARAM_IPL_BUILTIN_DISABLE) {
	    if (kdrv_dce_set_fp[param_id] != NULL) {
	        rt = kdrv_dce_set_fp[param_id](channel, p_param);
	    }
	} else {	
    	if ((g_kdrv_dce_open_count == 0) && (param_id != KDRV_DCE_PARAM_IPL_OPENCFG)) {
    		DBG_ERR("KDRV DCE: engine is not opened. Only OPENCFG can be set. ID = %d\r\n", (int)param_id);
    		return E_SYS;
    	} else if ((g_kdrv_dce_open_count > 0) && (param_id == KDRV_DCE_PARAM_IPL_OPENCFG)) {
    		DBG_ERR("KDRV DCE: engine is opened. OPENCFG cannot be set.\r\n");
    		return E_SYS;
    	}

    	if (g_kdrv_dce_init_flg == 0) {
    		kdrv_dce_handle_alloc_all(&g_kdrv_dce_init_flg);
    	}
    	if (g_kdrv_dce_init_flg == FALSE) {
    		DBG_WRN("KDRV DCE: no free handle, max handle num = %d\r\n", kdrv_dce_channel_num);
    		return E_SYS;
    	}

    	p_handle = &g_kdrv_dce_handle_tab[channel];
    	if (kdrv_dce_chk_handle(p_handle) == NULL) {
    		DBG_ERR("KDRV DCE: illegal handle 0x%.8x\r\n", (unsigned int)&g_kdrv_dce_handle_tab[channel]);
    		return E_SYS;
    	}

    	if ((p_handle->sts & KDRV_DCE_HANDLE_LOCK) != KDRV_DCE_HANDLE_LOCK) {
    		DBG_ERR("KDRV DCE: illegal handle sts 0x%.8x\r\n", (unsigned int)p_handle->sts);
    		return E_SYS;
    	}

    	if (!ign_chk) {
    		kdrv_dce_lock(p_handle, TRUE);
    	}
    	if (kdrv_dce_set_fp[param_id] != NULL) {
    		rt = kdrv_dce_set_fp[param_id](channel, p_param);
    		myflags = dce_platform_spin_lock();
    		g_kdrv_dce_load_flg[channel][param_id] = TRUE;
    		dce_platform_spin_unlock(myflags);
    	}

    	if (!ign_chk) {
    		kdrv_dce_lock(p_handle, FALSE);
    	}
    }

	return rt;
}

static ER kdrv_dce_get_in_info(UINT32 id, void *data)
{
	*(KDRV_DCE_IN_INFO *)data = g_kdrv_dce_info[id].in_info;
	return E_OK;
}

static ER kdrv_dce_get_in_addr(UINT32 id, void *data)
{
	*(KDRV_DCE_IMG_IN_DMA_INFO *)data = g_kdrv_dce_info[id].in_dma_info;
	return E_OK;
}

static ER kdrv_dce_get_in_img_info(UINT32 id, void *data)
{
	*(KDRV_DCE_IN_IMG_INFO *)data = g_kdrv_dce_info[id].in_img_info;
	return E_OK;
}

static ER kdrv_dce_get_out_addr(UINT32 id, void *data)
{
	*(KDRV_DCE_IMG_OUT_DMA_INFO *)data = g_kdrv_dce_info[id].out_dma_info;
	return E_OK;
}

static ER kdrv_dce_get_out_img_info(UINT32 id, void *data)
{
	*(KDRV_DCE_OUT_IMG_INFO *)data = g_kdrv_dce_info[id].out_img_info;
	return E_OK;
}

static ER kdrv_dce_get_subin_addr(UINT32 id, void *data)
{
	*(KDRV_DCE_SUBIMG_IN_ADDR *)data = g_kdrv_dce_info[id].wdr_subin_addr;
	return E_OK;
}
static ER kdrv_dce_get_subout_addr(UINT32 id, void *data)
{
	*(KDRV_DCE_SUBIMG_OUT_ADDR *)data = g_kdrv_dce_info[id].wdr_subout_addr;
	return E_OK;
}

static ER kdrv_dce_get_dram_out_mode(UINT32 id, void *data)
{

	*(KDRV_DCE_DRAM_OUTPUT_MODE *)data = g_kdrv_dce_info[id].dram_out_mode;

	return E_OK;
}

static ER kdrv_dce_get_single_out_channel(UINT32 id, void *data)
{

	*(KDRV_DCE_DRAM_SINGLE_OUT_EN *)data = g_kdrv_dce_info[id].single_out_ch;

	return E_OK;
}

static ER kdrv_dce_get_gdc_center(UINT32 id, void *data)
{
	*(KDRV_DCE_GDC_CENTER_INFO *)data = g_kdrv_dce_info[id].gdc_center;
	return E_OK;
}

static ER kdrv_dce_get_dc_sram_sel(UINT32 id, void *data)
{
	*(KDRV_DCE_DC_SRAM_SEL *)data = g_kdrv_dce_info[id].dc_sram_sel;
	return E_OK;
}


static ER kdrv_dce_get_stripe_info(UINT32 id, void *data)
{
	*(KDRV_DCE_STRIPE_INFO *)data = g_kdrv_dce_info[id].ipl_stripe;
	return E_OK;
}

static ER kdrv_dce_get_isr_cb(UINT32 id, void *data)
{
	*(KDRV_IPP_ISRCB *)data = g_kdrv_dce_handle_tab[id].isrcb_fp;
	return E_OK;
}

static ER kdrv_dce_get_extend_info(UINT32 id, void *data)
{
	*(KDRV_DCE_EXTEND_INFO *)data = g_kdrv_dce_info[id].extend_info;
	return E_OK;
}

static ER kdrv_dce_get_interrupt_enable(UINT32 id, void *data)
{
	*(KDRV_DCE_INTE *)data = g_kdrv_dce_info[id].inte_en;
	return E_OK;
}

static ER kdrv_dce_get_cfa_subout_info(UINT32 id, void *data)
{
	*(KDRV_DCE_CFA_SUBOUT_INFO *)data = g_kdrv_dce_info[id].cfa_subout_info;
	return E_OK;
}

static ER kdrv_dce_get_stripe_rule_info(UINT32 id, void *data)
{
	*(KDRV_DCE_STRP_RULE_INFO *)data = g_kdrv_dce_info[id].stripe_rule_info;
	return E_OK;
}

static ER kdrv_dce_get_cfa_param(UINT32 id, void *data)
{
	*(KDRV_DCE_CFA_PARAM *)data = g_kdrv_dce_info[id].cfa_param;
	return E_OK;
}

static ER kdrv_dce_get_dc_cac_param(UINT32 id, void *data)
{
	UINT32 i;
	KDRV_DCE_DC_CAC_PARAM *temp = (KDRV_DCE_DC_CAC_PARAM *)data;

	temp->dc = g_kdrv_dce_info[id].dc_cac_param.dc;
	temp->cac = g_kdrv_dce_info[id].dc_cac_param.cac;

	for (i = 0; i < GDC_TABLE_NUM; i++) {
		temp->dc.geo_lut_g[i] = g_kdrv_dce_info[id].dc_cac_param.dc.geo_lut_g[i];
	}
	for (i = 0; i < CAC_TABLE_NUM; i++) {
		temp->cac.geo_lut_r[i] = g_kdrv_dce_info[id].dc_cac_param.cac.geo_lut_r[i];
		temp->cac.geo_lut_b[i] = g_kdrv_dce_info[id].dc_cac_param.cac.geo_lut_b[i];
	}

	return E_OK;
}

static ER kdrv_dce_get_2d_lut_param(UINT32 id, void *data)
{
	*(KDRV_DCE_2DLUT_PARAM *)data = g_kdrv_dce_info[id].lut_2d_param;
	return E_OK;
}

static ER kdrv_dce_get_fov_param(UINT32 id, void *data)
{
	*(KDRV_DCE_FOV_PARAM *)data = g_kdrv_dce_info[id].fov_param;
	return E_OK;
}

static ER kdrv_dce_get_stripe_param(UINT32 id, void *data)
{
	UINT32 i;

	for (i = 0; i < 16; i++) {
		*(UINT32 *)((KDRV_DCE_STRIPE_PARAM *)data + i * 4) = g_kdrv_dce_info[id].stripe_param.hstp[i];
	}
	return E_OK;
}

static ER kdrv_dce_get_wdr_subimg_param(UINT32 id, void *data)
{
	*(KDRV_DCE_WDR_SUBIMG_PARAM *)data = g_kdrv_dce_info[id].wdr_subimg_param;
	return E_OK;
}

static ER kdrv_dce_get_wdr_param(UINT32 id, void *data)
{
	*(KDRV_DCE_WDR_PARAM *)data = g_kdrv_dce_info[id].wdr_param;
	return E_OK;
}

static ER kdrv_dce_get_hist_param(UINT32 id, void *data)
{
	*(KDRV_DCE_HIST_PARAM *)data = g_kdrv_dce_info[id].hist_param;
	return E_OK;
}

static ER kdrv_dce_get_hist_rslt(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (unsigned int)id, (unsigned int)data);
		return E_PAR;
	}

	dce_get_hist_rslt((DCE_HIST_RSLT *)data);

	return E_OK;
}

KDRV_DCE_GET_FP kdrv_dce_get_fp[KDRV_DCE_PARAM_MAX] = {
	NULL,
	kdrv_dce_get_in_info,
	kdrv_dce_get_in_img_info,
	kdrv_dce_get_out_img_info,
	kdrv_dce_get_in_addr,
	kdrv_dce_get_out_addr,
	kdrv_dce_get_subin_addr,
	kdrv_dce_get_subout_addr,
	kdrv_dce_get_dram_out_mode,
	kdrv_dce_get_single_out_channel,
	kdrv_dce_get_gdc_center,
	kdrv_dce_get_dc_sram_sel,
	kdrv_dce_get_stripe_info,
	kdrv_dce_get_isr_cb,
	kdrv_dce_get_interrupt_enable,
	NULL,
	kdrv_dce_get_extend_info,
	kdrv_dce_get_cfa_subout_info,
	kdrv_dce_get_stripe_rule_info,
	kdrv_dce_get_cfa_param,
	kdrv_dce_get_dc_cac_param,
	kdrv_dce_get_2d_lut_param,
	kdrv_dce_get_fov_param,
	kdrv_dce_get_stripe_param,

	kdrv_dce_get_wdr_subimg_param,
	kdrv_dce_get_wdr_param,
	kdrv_dce_get_hist_param,
	kdrv_dce_get_hist_rslt,
	kdrv_dce_get_stripe_num,
	NULL,
	NULL,
	NULL,

};

INT32 kdrv_dce_get(UINT32 id, KDRV_DCE_PARAM_ID parm_id, VOID *p_param)
{
	ER rt = E_OK;
	KDRV_DCE_HANDLE *p_handle;
	UINT32 ign_chk;
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);

	//DBG_IND("id = 0x%x\r\n", id);
	if (KDRV_DEV_ID_ENGINE(id) != KDRV_VIDEOPROCS_DCE_ENGINE0) {
		DBG_ERR("Invalid engine 0x%x\r\n", (unsigned int)KDRV_DEV_ID_ENGINE(id));
		return E_ID;
	}

	p_handle = &g_kdrv_dce_handle_tab[channel];
	if (kdrv_dce_chk_handle(p_handle) == NULL) {
		DBG_ERR("KDRV DCE: illegal handle 0x%.8x\r\n", (unsigned int)&g_kdrv_dce_handle_tab[channel]);
		return E_SYS;
	}

	if ((p_handle->sts & KDRV_DCE_HANDLE_LOCK) != KDRV_DCE_HANDLE_LOCK) {
		DBG_ERR("KDRV DCE: illegal handle sts 0x%.8x\r\n", (unsigned int)p_handle->sts);
		return E_SYS;
	}

	ign_chk = (KDRV_DCE_IGN_CHK & parm_id);
	parm_id = parm_id & (~KDRV_DCE_IGN_CHK);
	if (!ign_chk) {
		kdrv_dce_lock(p_handle, TRUE);
	}
	if (kdrv_dce_get_fp[parm_id] != NULL) {
		rt = kdrv_dce_get_fp[parm_id](channel, p_param);
	}
	if (!ign_chk) {
		kdrv_dce_lock(p_handle, FALSE);
	}

	return rt;
}

static CHAR *kdrv_dce_dump_str_in_src(KDRV_DCE_IN_SRC in_src)
{
	CHAR *str_in_src[4 + 1] = {
		"DIRECT",
		"ERROR",
		"ALL-DIRECT",
		"DRAM",
		"ERROR",
	};

	if (in_src >= 4) {
		return str_in_src[4];
	}
	return str_in_src[in_src];
}

static CHAR *kdrv_dce_dump_str_in_fmt(KDRV_DCE_D2D_FMT_TYPE in_fmt)
{
	CHAR *str_in_fmt[3 + 1] = {
		"Y_PACK_UV422",
		"Y_PACK_UV420",
		"BAYER",
		"ERROR",
	};

	if (in_fmt >= 3) {
		return str_in_fmt[3];
	}
	return str_in_fmt[in_fmt];
}

static CHAR *kdrv_dce_dump_str_strp_type(KDRV_DCE_STRIPE_TYPE strp_type)
{
	CHAR *str_strp_type[4 + 1] = {
		"STRP_AUTO",
		"STRP_MULTI",
		"STRP_2STRIP",
		"STRP_4STRIP",
		"ERROR",
	};

	if (strp_type >= 4) {
		return str_strp_type[4];
	}
	return str_strp_type[strp_type];
}


static CHAR *kdrv_dce_dump_str_enable(BOOL enable)
{
	CHAR *str_enable[2] = {
		"0",
		"1",
	};
	return str_enable[enable];
}

#if defined __UITRON || defined __ECOS
void dal_dce_dump_info(void (*dump)(char *fmt, ...))
#else
void kdrv_dce_dump_info(int (*dump)(const char *fmt, ...))
#endif
{
	BOOL is_print_header;
	BOOL is_open[KDRV_IPP_MAX_CHANNEL_NUM] = {FALSE};
	//BOOL is_open[KDRV_DCE_HANDLE_MAX_NUM] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
	UINT32 id;
	UINT32 i;

	if (g_kdrv_dce_open_count == 0) {
		dump("\r\n[DCE] not open\r\n");
		return;
	}

	for (id = 0; id < kdrv_dce_channel_num; id++) {
		is_open[id] = TRUE;
		kdrv_dce_lock(kdrv_dce_entry_id_conv2_handle(id), TRUE);
	}

	dump("\r\n[DCE]\r\n");
	dump("\r\n-----ctrl info-----\r\n");
	dump("%12s%12s\r\n", "init_flg", "trig_id");
	dump("%#12x 0x%.8x\r\n", g_kdrv_dce_init_flg, g_kdrv_dce_trig_hdl);

	/**
	    input info
	*/
	dump("\r\n-----input info-----\r\n");
	dump("%3s%12s%12s%12s%14s%12s%12s%12s%12s%12s\r\n", "id", "AddrY", "AddrUV", "in_src", "type", "y_width", "y_height", "y_line_ofs", "uv_line_ofs", "strp_type");
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if (!is_open[id]) {
			continue;
		}
		dump("%3d%#12x%#12x%12s%14s%12d%12d%12d%12d%12s\r\n", id
			 , g_kdrv_dce_info[id].in_dma_info.addr_y, g_kdrv_dce_info[id].in_dma_info.addr_cbcr
			 , kdrv_dce_dump_str_in_src(g_kdrv_dce_info[id].in_info.in_src)
			 , kdrv_dce_dump_str_in_fmt(g_kdrv_dce_info[id].in_info.type)
			 , g_kdrv_dce_info[id].in_img_info.img_size_h, g_kdrv_dce_info[id].in_img_info.img_size_v
			 , g_kdrv_dce_info[id].in_img_info.lineofst[KDRV_DCE_YUV_Y], g_kdrv_dce_info[id].in_img_info.lineofst[KDRV_DCE_YUV_UV]
			 , kdrv_dce_dump_str_strp_type(g_kdrv_dce_info[id].stripe_param.stripe_type));
	}

	/**
	    ouput info
	*/
	dump("\r\n-----output info-----\r\n");
	dump("%3s%12s%12s%12s%12s%12s%12s%12s%12s\r\n", "id", "AddrY", "AddrUV", "CropStartX", "CropStartY", "CropSizeX", "CropSizeY", "y_line_ofs", "uv_line_ofs");
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if (!is_open[id]) {
			continue;
		}
		dump("%3d%#12x%#12x%12d%12d%12d%12d%12d%12d\r\n", id
			 , g_kdrv_dce_info[id].out_dma_info.addr_y, g_kdrv_dce_info[id].out_dma_info.addr_cbcr
			 , g_kdrv_dce_info[id].out_img_info.crop_start_h, g_kdrv_dce_info[id].out_img_info.crop_start_v
			 , g_kdrv_dce_info[id].out_img_info.crop_size_h, g_kdrv_dce_info[id].out_img_info.crop_size_v
			 , g_kdrv_dce_info[id].out_img_info.lineofst[KDRV_DCE_YUV_Y], g_kdrv_dce_info[id].out_img_info.lineofst[KDRV_DCE_YUV_UV]);

	}

	/**
	    Function enable info
	*/
	dump("\r\n-----Function enable info-----\r\n");
	dump("%3s%12s%8s%9s%8s%12s%8s%8s%8s%8s%8s%8s\r\n", "id", "TONECURVE", "WDR", "HIST_EN", "CFA", "CFA_SUBOUT", "DC", "CAC", "2D_Lut", "Ime3dnr", "ImeEnc", "ImeDec");
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if (!is_open[id]) {
			continue;
		}
		dump("%3d%12s%8s%9s%8s%12s%8s%8s%8s%8s%8s%8s\r\n", id
			 , kdrv_dce_dump_str_enable(g_kdrv_dce_info[id].wdr_param.tonecurve_enable)
			 , kdrv_dce_dump_str_enable(g_kdrv_dce_info[id].wdr_param.wdr_enable)
			 , kdrv_dce_dump_str_enable(g_kdrv_dce_info[id].hist_param.hist_enable)
			 , kdrv_dce_dump_str_enable(g_kdrv_dce_info[id].cfa_param.cfa_enable)
			 , kdrv_dce_dump_str_enable(g_kdrv_dce_info[id].cfa_subout_info.cfa_subout_enable)
			 , kdrv_dce_dump_str_enable(g_kdrv_dce_info[id].dc_cac_param.dc_enable)
			 , kdrv_dce_dump_str_enable(g_kdrv_dce_info[id].dc_cac_param.cac.cac_enable)
			 , kdrv_dce_dump_str_enable(g_kdrv_dce_info[id].lut_2d_param.lut_2d_enable)
			 , kdrv_dce_dump_str_enable(g_kdrv_dce_info[id].extend_info.ime_3dnr_enable)
			 , kdrv_dce_dump_str_enable(g_kdrv_dce_info[id].extend_info.ime_enc_enable)
			 , kdrv_dce_dump_str_enable(g_kdrv_dce_info[id].extend_info.ime_dec_enable));
	}

	/**
	    IQ parameter info
	*/
	dump("\r\n-----IQ parameter info-----\r\n");

	// WDR
	is_print_header = FALSE;
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if ((!is_open[id]) || (!g_kdrv_dce_info[id].wdr_param.wdr_enable)) {
			continue;
		}
		if (!is_print_header) {
			dump("%12s%3s%12s%12s\r\n", "WDR", "id", "En", "Str");
			is_print_header = TRUE;
		}
		dump("%12s%3d%12d%12d\r\n"
			 , "--", id
			 , g_kdrv_dce_info[id].wdr_param.wdr_enable
			 , g_kdrv_dce_info[id].wdr_param.wdr_str.strength);
	}

	// ToneCurve
	is_print_header = FALSE;
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if ((!is_open[id]) || (!g_kdrv_dce_info[id].wdr_param.tonecurve_enable)) {
			continue;
		}
		if (!is_print_header) {
			dump("%12s%3s%12s\r\n", "Tonecurve", "id", "En");
			is_print_header = TRUE;
		}
		dump("%12s%3d%12d\r\n"
			 , "--", id
			 , g_kdrv_dce_info[id].wdr_param.tonecurve_enable);
	}


	// WDR subout
	is_print_header = FALSE;
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if ((!is_open[id]) || (!g_kdrv_dce_info[id].wdr_subout_addr.enable)) {
			continue;
		}
		if (!is_print_header) {
			dump("%12s%3s%12s%12s%12s%12s%12s%12s\r\n", "WDR_SUBOUT", "id", "En", "Addr", "LofsIn", "LofsOut", "SUB_SIZEX", "SUB_SIZEY");
			is_print_header = TRUE;
		}
		dump("%12s%3d%12d  0x%.8x%12d%12d%12d%12d\r\n"
			 , "--", id
			 , g_kdrv_dce_info[id].wdr_subout_addr.enable
			 , g_kdrv_dce_info[id].wdr_subout_addr.addr
			 , g_kdrv_dce_info[id].wdr_subimg_param.subimg_lofs_in
			 , g_kdrv_dce_info[id].wdr_subimg_param.subimg_lofs_out
			 , g_kdrv_dce_info[id].wdr_subimg_param.subimg_size_h
			 , g_kdrv_dce_info[id].wdr_subimg_param.subimg_size_v);
	}

	// Hist
	is_print_header = FALSE;
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if ((!is_open[id]) || (!g_kdrv_dce_info[id].hist_param.hist_enable)) {
			continue;
		}
		if (!is_print_header) {
			dump("%12s%3s%12s%12s\r\n", "HIST", "id", "HistEn", "SEL");
			is_print_header = TRUE;
		}
		dump("%12s%3d%12d%12d\r\n"
			 , "--", id
			 , g_kdrv_dce_info[id].hist_param.hist_enable
			 , g_kdrv_dce_info[id].hist_param.hist_sel);
	}

	// CFA
	is_print_header = FALSE;
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if ((!is_open[id]) || (!g_kdrv_dce_info[id].cfa_param.cfa_enable)) {
			continue;
		}
		if (!is_print_header) {
			dump("%12s%3s%12s\r\n", "CFA-Dir", "id", "PAT");
			is_print_header = TRUE;
		}
		dump("%12s%3d%12d\r\n"
			 , "--", id, g_kdrv_dce_info[id].in_img_info.cfa_pat);
	}

	is_print_header = FALSE;
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if ((!is_open[id]) || (!g_kdrv_dce_info[id].cfa_param.cfa_enable)) {
			continue;
		}
		if (!is_print_header) {
			dump("%12s%3s%12s%12s%12s\r\n", "CFA-Corr", "id", "RBCorrEn", "RBCorrTh1", "RBCorrTh2");
			is_print_header = TRUE;
		}
		dump("%12s%3d%12d%12d%12d\r\n"
			 , "--", id
			 , g_kdrv_dce_info[id].cfa_param.cfa_correction.rb_corr_enable
			 , g_kdrv_dce_info[id].cfa_param.cfa_correction.rb_corr_th1
			 , g_kdrv_dce_info[id].cfa_param.cfa_correction.rb_corr_th2);
	}

	is_print_header = FALSE;
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if ((!is_open[id]) || (!g_kdrv_dce_info[id].cfa_param.cfa_enable)) {
			continue;
		}
		if (!is_print_header) {
			dump("%12s%3s%12s%12s%12s%12s%12s%12s%12s\r\n", "CFA-Hfc", "id", "CLCheckEn", "HFCheckEn", "AvgMode", "CLTh", "HFGTh", "HFDiff", "HFEThr");
			is_print_header = TRUE;
		}
		dump("%12s%3d%12d%12d%12d%12d%12d%12d%12d\r\n"
			 , "--", id
			 , g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.cl_check_enable
			 , g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.hf_check_enable
			 , g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.average_mode
			 , g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.cl_th
			 , g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.hfg_th
			 , g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.hf_diff
			 , g_kdrv_dce_info[id].cfa_param.cfa_ir_hfc.hfe_th);
	}

	// CFA subout
	is_print_header = FALSE;
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if ((!is_open[id]) || (!g_kdrv_dce_info[id].cfa_subout_info.cfa_subout_enable)) {
			continue;
		}
		if (!is_print_header) {
			dump("%12s%3s%12s%12s%12s%12s%12s%12s%12s\r\n", "CFA_SUBOUT", "id", "En", "FlipEn", "Addr", "Lofs", "Ch", "Byte", "ShiftBit");
			is_print_header = TRUE;
		}
		dump("%12s%3d%12d%12d  0x%.8x%12d%12d%12d%12d\r\n"
			 , "--", id
			 , g_kdrv_dce_info[id].cfa_subout_info.cfa_subout_enable
			 , g_kdrv_dce_info[id].cfa_subout_info.cfa_subout_flip_enable
			 , g_kdrv_dce_info[id].cfa_subout_info.cfa_addr
			 , g_kdrv_dce_info[id].cfa_subout_info.cfa_lofs
			 , g_kdrv_dce_info[id].cfa_subout_info.subout_ch_sel
			 , g_kdrv_dce_info[id].cfa_subout_info.subout_byte
			 , g_kdrv_dce_info[id].cfa_subout_info.subout_shiftbit);
	}

	//Distortion correction
	is_print_header = FALSE;
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if ((!is_open[id]) || (!g_kdrv_dce_info[id].dc_cac_param.dc_enable)) {
			continue;
		}
		if (!is_print_header) {
			dump("%12s%3s%12s%12s%12s%12s%12s\r\n", "DC", "id", "CenterX", "CenterY", "DistX", "DistY", "GLutAddr");
			is_print_header = TRUE;
		}
		dump("%12s%3d%12d%12d%12d%12d%#12x\r\n"
			 , "--", id
			 , g_kdrv_dce_info[id].gdc_center.geo_center_x
			 , g_kdrv_dce_info[id].gdc_center.geo_center_y
			 , g_kdrv_dce_info[id].dc_cac_param.dc.geo_dist_x
			 , g_kdrv_dce_info[id].dc_cac_param.dc.geo_dist_y
			 , (UINT32)g_kdrv_dce_info[id].dc_cac_param.dc.geo_lut_g);
	}

	is_print_header = FALSE;
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if ((!is_open[id]) || (!g_kdrv_dce_info[id].dc_cac_param.dc_enable)) {
			continue;
		}
		if (!is_print_header) {
			dump("%12s%3s%12s%12s%12s\r\n", "DC-Nor", "id", "NormBit", "NormFact", "FovGain");
			is_print_header = TRUE;
		}
		dump("%12s%3d%12d%12d%12d\r\n"
			 , "--", id
			 , g_kdrv_dce_norm[id].nrm_bit
			 , g_kdrv_dce_norm[id].nrm_fact
			 , g_kdrv_dce_info[id].fov_param.fov_gain);
	}

	is_print_header = FALSE;
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if ((!is_open[id]) || (!g_kdrv_dce_info[id].dc_cac_param.dc_enable)) {
			continue;
		}
		if (!is_print_header) {
			dump("%12s%3s%12s%12s%12s%12s\r\n", "DC-FovBnd", "id", "BndSel", "Bnd_FixR", "Bnd_FixG", "Bnd_FixB");
			is_print_header = TRUE;
		}
		dump("%12s%3d%12d%12d%12d%12d\r\n"
			 , "--", id
			 , g_kdrv_dce_info[id].fov_param.fov_bnd_sel
			 , g_kdrv_dce_info[id].fov_param.fov_bnd_r
			 , g_kdrv_dce_info[id].fov_param.fov_bnd_g
			 , g_kdrv_dce_info[id].fov_param.fov_bnd_b);
	}

	is_print_header = FALSE;
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if ((!is_open[id]) || (!g_kdrv_dce_info[id].dc_cac_param.dc_enable)) {
			continue;
		}
		if (!is_print_header) {
			dump("%12s%3s%12s%12s%12s%12s\r\n", "DC-Enh", "id", "YEnhBit", "YEnhFact", "UVEnhBit", "UVEnhFact");
			is_print_header = TRUE;
		}
	}

	is_print_header = FALSE;
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if ((!is_open[id]) || (!g_kdrv_dce_info[id].dc_cac_param.cac.cac_enable)) {
			continue;
		}
		if (!is_print_header) {
			dump("%12s%3s%12s%12s%12s%12s%12s%12s%12s\r\n", "CAC", "id", "Sel", "Rgain", "Ggain", "Bgain", "RLutAddr", "GLutAddr", "BLutAddr");
			is_print_header = TRUE;
		}
		dump("%12s%3d%12d%12d%12d%12d%#12x%#12x%#12x\r\n"
			 , "--", id
			 , g_kdrv_dce_info[id].dc_cac_param.cac.cac_sel
			 , g_kdrv_dce_info[id].dc_cac_param.cac.geo_r_lut_gain
			 , g_kdrv_dce_info[id].dc_cac_param.cac.geo_g_lut_gain
			 , g_kdrv_dce_info[id].dc_cac_param.cac.geo_b_lut_gain
			 , (UINT32)g_kdrv_dce_info[id].dc_cac_param.cac.geo_lut_r
			 , (UINT32)g_kdrv_dce_info[id].dc_cac_param.dc.geo_lut_g
			 , (UINT32)g_kdrv_dce_info[id].dc_cac_param.cac.geo_lut_b);
	}

	is_print_header = FALSE;
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if ((!is_open[id]) || (!g_kdrv_dce_info[id].lut_2d_param.lut_2d_enable)) {
			continue;
		}
		if (!is_print_header) {
			dump("%12s%3s%12s%12s%12s%12s%12s%12s\r\n", "2D_Lut", "id", "Num", "XOfsInt", "XOfsFrac", "YOfsInt", "YOfsFrac", "Lut_Addr");
			is_print_header = TRUE;
		}
		dump("%12s%3d%12d\r\n"
			 , "--", id
			 , g_kdrv_dce_info[id].lut_2d_param.lut_num_select);
	}

	is_print_header = FALSE;
	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if (!is_open[id]) {
			continue;
		}
		if (!is_print_header) {
			dump("%12s%3s%12s%12s%d\r\n", "STRP", "id", "Num", "settings1~", g_kdrv_dce_info[id].ipl_stripe.hstp_num);
			is_print_header = TRUE;
		}
		dump("%12s%3d%12d"
			 , "--", id
			 , g_kdrv_dce_info[id].ipl_stripe.hstp_num);

		for (i = 0; i < g_kdrv_dce_info[id].ipl_stripe.hstp_num; i++) {
			dump("%6d", g_kdrv_dce_info[id].ipl_stripe.hstp[i]);
		}
		dump("\r\n");

	}


	for (id = 0; id < kdrv_dce_channel_num; id++) {
		if (is_open[id]) {
			kdrv_dce_lock(kdrv_dce_entry_id_conv2_handle(id), FALSE);
		}
	}

}

#ifdef __KERNEL__
EXPORT_SYMBOL(kdrv_dce_init);
EXPORT_SYMBOL(kdrv_dce_uninit);
EXPORT_SYMBOL(kdrv_dce_buf_query);
EXPORT_SYMBOL(kdrv_dce_buf_init);
EXPORT_SYMBOL(kdrv_dce_buf_uninit);
EXPORT_SYMBOL(kdrv_dce_open);
EXPORT_SYMBOL(kdrv_dce_close);
EXPORT_SYMBOL(kdrv_dce_pause);
EXPORT_SYMBOL(kdrv_dce_trigger);
EXPORT_SYMBOL(kdrv_dce_set);
EXPORT_SYMBOL(kdrv_dce_get);
EXPORT_SYMBOL(kdrv_dce_dump_info);
#endif
