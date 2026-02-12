/**
    @file       dal_ife.c
    @ingroup    Predefined_group_name

    @brief      ife device abstraction layer

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "ife_platform.h"
#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kdrv_videoprocess/kdrv_ife.h"
#include "kdrv_ife_int.h"

#if 0
#if defined __UITRON || defined __ECOS
#define __MODULE__    kdrv_ife
#define __DBGLVL__    2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*" // *=All, [mark1]=CustomClass
#include "DebugModule.h"
#else
#include "ife_dbg.h"
#endif
#endif

#define KDRV_NRS_B_OFS_LEN        6
#define KDRV_NRS_B_W_LEN          6
#define KDRV_NRS_B_RANGE_LEN      5
#define KDRV_NRS_B_TH_LEN         5
#define KDRV_NRS_GB_OFS_LEN       6
#define KDRV_NRS_GB_W_LEN         6
#define KDRV_NRS_GB_RANGE_LEN     5
#define KDRV_NRS_GB_TH_LEN        5
#define KDRV_NRS_ORD_W_LEN        8
#define KDRV_SPATIAL_W_LEN        6
#define KDRV_RNG_TH_LEN           6
#define KDRV_RNG_LUT_LEN         17
#define KDRV_CGAIN_LEN            5
#define KDRV_OUTL_TH_LEN          5
#define KDRV_OUTL_ORD_W_LEN       8
#define KDRV_OUTL_CNT_LEN         2
#define KDRV_VIG_CEN_LEN          4
#define KDRV_VIG_LUT_LEN         17
#define KDRV_GBAL_OFS_LEN        17
#define KDRV_RBFILL_LUMA_NUM     17
#define KDRV_RBFILL_RATIO_NUM    32


// vig
static INT32                    g_vig_center_x  [KDRV_IFE_HANDLE_MAX_NUM][KDRV_VIG_CEN_LEN];
static INT32                    g_vig_center_y  [KDRV_IFE_HANDLE_MAX_NUM][KDRV_VIG_CEN_LEN];

//static KDRV_IFE_PRAM            g_kdrv_ife_info  [KDRV_IFE_HANDLE_MAX_NUM];
//static KDRV_IFE_HANDLE          g_kdrv_ife_handle_tab[KDRV_IFE_HANDLE_MAX_NUM];
//static UINT32                   g_kdrv_ife_ipl_ctrl_en[KDRV_IFE_HANDLE_MAX_NUM] = {0};

static KDRV_IFE_VIG_PARAM       *g_kdrv_ife_vig_set_info = NULL;
static KDRV_IFE_PRAM            *g_kdrv_ife_info = NULL;
static KDRV_IFE_HANDLE          *g_kdrv_ife_handle_tab = NULL;
static UINT32                   *g_kdrv_ife_ipl_ctrl_en = NULL;
static UINT32                   *g_kdrv_ife_iq_ctrl_en = NULL;
static UINT32                   g_kdrv_ife_init_flg = 0;
static KDRV_IFE_HANDLE          *g_kdrv_ife_trig_hdl = NULL;
static KDRV_IFE_OPENCFG         g_kdrv_ife_open_cfg = {0};
static UINT32                   kdrv_ife_open_cnt = 0;

static IFE_PARAM                g_ife_param = {0};
static BOOL                     g_ife_update_flag[KDRV_IFE_HANDLE_MAX_NUM][KDRV_IFE_PARAM_MAX] = {FALSE};

UINT32                          kdrv_ife_channel_num = 0;
UINT32                          kdrv_ife_lock_chls = 0;
UINT8  dct_qtable_inx[8] = {0};
UINT16 degamma_table[33] = {0};


UINT8 fcurve_y_weight[17] = {255, 255, 255, 255, 224, 192, 160, 128, 96, 64, 32, 16, 0, 0, 0, 0, 0};
UINT8 fcurve_index[32] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62};
UINT8 fcurve_split[32] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
UINT16 fcurve_value[65] = {0, 835, 1257, 1542, 1758, 1931, 2076, 2200, 2309, 2407, 2494, 2574, 2647, 2715, 2777, 2836, 2891, 2943,
						   2992, 3038, 3082, 3124, 3164, 3203, 3240, 3275, 3309, 3341, 3373, 3403, 3433, 3461, 3489, 3516, 3542,
						   3567, 3592, 3616, 3639, 3662, 3684, 3705, 3726, 3747, 3767, 3787, 3806, 3825, 3843, 3861, 3879, 3896,
						   3913, 3930, 3946, 3962, 3978, 3994, 4009, 4024, 4039, 4053, 4068, 4082, 4095
						  };

static void kdrv_ife_init_param(int channel)
{
	UINT8 i ;

	memset(&g_kdrv_ife_info[channel], 0, sizeof(g_kdrv_ife_info[channel]));
	memset(&g_kdrv_ife_vig_set_info[channel], 0, sizeof(g_kdrv_ife_vig_set_info[channel]));
	

	for (i = 0; i < KDRV_IFE_HANDLE_MAX_NUM; i++) {
		g_kdrv_ife_ipl_ctrl_en[i] = 0x00000000;
		g_kdrv_ife_iq_ctrl_en[i]  = 0x00000000;
	}

	g_kdrv_ife_info[channel].info_ring_buf.dmaloop_line = 1;

	// cgain
	g_kdrv_ife_info[channel].par_cgain.enable = FALSE;
	g_kdrv_ife_info[channel].par_cgain.inv = 0;
	g_kdrv_ife_info[channel].par_cgain.hinv = 0;
	g_kdrv_ife_info[channel].par_cgain.mask = 0xFFF;

	g_kdrv_ife_info[channel].par_cgain.cgain_r    =   256;
	g_kdrv_ife_info[channel].par_cgain.cgain_gr   =   256;
	g_kdrv_ife_info[channel].par_cgain.cgain_gb   =   256;
	g_kdrv_ife_info[channel].par_cgain.cgain_b    =   256;
	g_kdrv_ife_info[channel].par_cgain.cgain_ir   =   256;

	g_kdrv_ife_info[channel].par_cgain.cofs_r     =   0;
	g_kdrv_ife_info[channel].par_cgain.cofs_gr    =   0;
	g_kdrv_ife_info[channel].par_cgain.cofs_gb    =   0;
	g_kdrv_ife_info[channel].par_cgain.cofs_b     =   0;
	g_kdrv_ife_info[channel].par_cgain.cofs_ir    =   0;

	g_kdrv_ife_info[channel].info_fusion.enable = FALSE;
	// Fusion
	g_kdrv_ife_info[channel].par_fusion.fu_ctrl.y_mean_sel = 0;
	g_kdrv_ife_info[channel].par_fusion.fu_ctrl.mode    = 0;
	g_kdrv_ife_info[channel].par_fusion.fu_ctrl.ev_ratio = 16;

	g_kdrv_ife_info[channel].par_fusion.bld_cur.nor_sel = 1;
	g_kdrv_ife_info[channel].par_fusion.bld_cur.dif_sel = 2;

	g_kdrv_ife_info[channel].par_fusion.bld_cur.l_nor_w_edge = 0;
	g_kdrv_ife_info[channel].par_fusion.bld_cur.l_nor_range = 9;
	g_kdrv_ife_info[channel].par_fusion.bld_cur.l_nor_knee[0] = 3072;
	//g_kdrv_ife_info[channel].par_fusion.bld_cur.l_nor_knee[1] = 3584;

	g_kdrv_ife_info[channel].par_fusion.bld_cur.s_nor_w_edge = 1;
	g_kdrv_ife_info[channel].par_fusion.bld_cur.s_nor_range = 6;
	g_kdrv_ife_info[channel].par_fusion.bld_cur.s_nor_knee[0] = 192;
	//g_kdrv_ife_info[channel].par_fusion.bld_cur.s_nor_knee[1] = 256;

	g_kdrv_ife_info[channel].par_fusion.bld_cur.l_dif_w_edge = 0;
	g_kdrv_ife_info[channel].par_fusion.bld_cur.l_dif_range = 9;
	g_kdrv_ife_info[channel].par_fusion.bld_cur.l_dif_knee[0] = 3072;
	//g_kdrv_ife_info[channel].par_fusion.bld_cur.l_dif_knee[1] = 3584;

	g_kdrv_ife_info[channel].par_fusion.bld_cur.s_dif_w_edge = 1;
	g_kdrv_ife_info[channel].par_fusion.bld_cur.s_dif_range = 6;
	g_kdrv_ife_info[channel].par_fusion.bld_cur.s_dif_knee[0] = 192;
	//g_kdrv_ife_info[channel].par_fusion.bld_cur.s_dif_knee[1] = 256;

	g_kdrv_ife_info[channel].par_fusion.mc_para.lum_th = 256;
	g_kdrv_ife_info[channel].par_fusion.mc_para.diff_ratio = 0;
	g_kdrv_ife_info[channel].par_fusion.mc_para.pos_diff_w[0] = 0;
	g_kdrv_ife_info[channel].par_fusion.mc_para.pos_diff_w[1] = 2;
	g_kdrv_ife_info[channel].par_fusion.mc_para.pos_diff_w[2] = 4;
	g_kdrv_ife_info[channel].par_fusion.mc_para.pos_diff_w[3] = 8;
	g_kdrv_ife_info[channel].par_fusion.mc_para.pos_diff_w[4] = 10;
	g_kdrv_ife_info[channel].par_fusion.mc_para.pos_diff_w[5] = 12;
	g_kdrv_ife_info[channel].par_fusion.mc_para.pos_diff_w[6] = 14;
	g_kdrv_ife_info[channel].par_fusion.mc_para.pos_diff_w[7] = 16;
	g_kdrv_ife_info[channel].par_fusion.mc_para.pos_diff_w[8] = 14;
	g_kdrv_ife_info[channel].par_fusion.mc_para.pos_diff_w[9] = 12;
	g_kdrv_ife_info[channel].par_fusion.mc_para.pos_diff_w[10] = 10;
	g_kdrv_ife_info[channel].par_fusion.mc_para.pos_diff_w[11] = 8;
	g_kdrv_ife_info[channel].par_fusion.mc_para.pos_diff_w[12] = 6;
	g_kdrv_ife_info[channel].par_fusion.mc_para.pos_diff_w[13] = 4;
	g_kdrv_ife_info[channel].par_fusion.mc_para.pos_diff_w[14] = 2;
	g_kdrv_ife_info[channel].par_fusion.mc_para.pos_diff_w[15] = 0;
	g_kdrv_ife_info[channel].par_fusion.mc_para.dwd = 0;

	g_kdrv_ife_info[channel].par_fusion.mc_para.neg_diff_w[0] = 0;
	g_kdrv_ife_info[channel].par_fusion.mc_para.neg_diff_w[1] = 2;
	g_kdrv_ife_info[channel].par_fusion.mc_para.neg_diff_w[2] = 4;
	g_kdrv_ife_info[channel].par_fusion.mc_para.neg_diff_w[3] = 8;
	g_kdrv_ife_info[channel].par_fusion.mc_para.neg_diff_w[4] = 10;
	g_kdrv_ife_info[channel].par_fusion.mc_para.neg_diff_w[5] = 12;
	g_kdrv_ife_info[channel].par_fusion.mc_para.neg_diff_w[6] = 14;
	g_kdrv_ife_info[channel].par_fusion.mc_para.neg_diff_w[7] = 16;
	g_kdrv_ife_info[channel].par_fusion.mc_para.neg_diff_w[8] = 14;
	g_kdrv_ife_info[channel].par_fusion.mc_para.neg_diff_w[9] = 12;
	g_kdrv_ife_info[channel].par_fusion.mc_para.neg_diff_w[10] = 10;
	g_kdrv_ife_info[channel].par_fusion.mc_para.neg_diff_w[11] = 8;
	g_kdrv_ife_info[channel].par_fusion.mc_para.neg_diff_w[12] = 6;
	g_kdrv_ife_info[channel].par_fusion.mc_para.neg_diff_w[13] = 4;
	g_kdrv_ife_info[channel].par_fusion.mc_para.neg_diff_w[14] = 2;
	g_kdrv_ife_info[channel].par_fusion.mc_para.neg_diff_w[15] = 0;

	g_kdrv_ife_info[channel].par_fusion.dk_sat.th[0] = 32;
	g_kdrv_ife_info[channel].par_fusion.dk_sat.step[0] = 16;
	g_kdrv_ife_info[channel].par_fusion.dk_sat.low_bound[0] = 0;

	g_kdrv_ife_info[channel].par_fusion.dk_sat.th[1] = 32;
	g_kdrv_ife_info[channel].par_fusion.dk_sat.step[1] = 16;
	g_kdrv_ife_info[channel].par_fusion.dk_sat.low_bound[1] = 0;

	g_kdrv_ife_info[channel].par_fusion.s_comp.enable = FALSE;
	g_kdrv_ife_info[channel].par_fusion.s_comp.knee[0] = 16;
	g_kdrv_ife_info[channel].par_fusion.s_comp.knee[1] = 64;
	g_kdrv_ife_info[channel].par_fusion.s_comp.knee[2] = 256;
	g_kdrv_ife_info[channel].par_fusion.s_comp.sub_point[0] = 0;
	g_kdrv_ife_info[channel].par_fusion.s_comp.sub_point[1] = 64;
	g_kdrv_ife_info[channel].par_fusion.s_comp.sub_point[2] = 256;
	g_kdrv_ife_info[channel].par_fusion.s_comp.sub_point[3] = 1024;
	g_kdrv_ife_info[channel].par_fusion.s_comp.shift[0] = 2;
	g_kdrv_ife_info[channel].par_fusion.s_comp.shift[1] = 2;
	g_kdrv_ife_info[channel].par_fusion.s_comp.shift[2] = 2;
	g_kdrv_ife_info[channel].par_fusion.s_comp.shift[3] = 2;

	g_kdrv_ife_info[channel].par_fusion.fu_cgain.enable = FALSE;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.bit_field = 0;

	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcgain_path0_r   = 256;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcgain_path0_gr  = 256;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcgain_path0_gb  = 256;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcgain_path0_b   = 256;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcgain_path0_ir  = 256;

	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcgain_path1_r   = 256;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcgain_path1_gr  = 256;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcgain_path1_gb  = 256;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcgain_path1_b   = 256;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcgain_path1_ir  = 256;

	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcofs_path0_r   = 0;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcofs_path0_gr  = 0;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcofs_path0_gb  = 0;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcofs_path0_b   = 0;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcofs_path0_ir  = 0;

	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcofs_path1_r   = 0;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcofs_path1_gr  = 0;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcofs_path1_gb  = 0;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcofs_path1_b   = 0;
	g_kdrv_ife_info[channel].par_fusion.fu_cgain.fcofs_path1_ir  = 0;

	// Fcurve
	g_kdrv_ife_info[channel].par_fcurve.enable = FALSE;
	g_kdrv_ife_info[channel].par_fcurve.fcur_ctrl.yv_w = 1;
	g_kdrv_ife_info[channel].par_fcurve.fcur_ctrl.y_mean_sel = 0;


	for (i = 0; i < FCURVE_Y_W_NUM; i++) {
		g_kdrv_ife_info[channel].par_fcurve.y_weight.y_w_lut[i] = fcurve_y_weight[i];
	}
	for (i = 0; i < FCURVE_IDX_NUM; i++) {
		g_kdrv_ife_info[channel].par_fcurve.index.idx_lut[i] = fcurve_index[i];
		g_kdrv_ife_info[channel].par_fcurve.split.split_lut[i] = fcurve_split[i];
	}
	for (i = 0; i < FCURVE_VAL_NUM; i++) {
		g_kdrv_ife_info[channel].par_fcurve.value.val_lut[i] = fcurve_value[i];
	}
}

static UINT32 vig_square(UINT32 x)
{
	UINT64 min = 0, max = 0, mid, i = 0;
	max = x << 8;
	mid = (max + min) >> 1;

	for (i = 0; i < 20; i++) {
		if ((((mid >> 8)* mid)) < (x << 8)) {
			min = mid;
		} else if ((((mid >> 8) * mid)) > (x << 8)) {
			max = mid;
		} else {
			return ((UINT32)mid);
		}
		mid = (max + min) >> 1;
	}
	return ((UINT32)mid);
}
static void ife_cal_vig_tab_gain(IFE_VIG_PARAM *p_vig_param, KDRV_IFE_VIG_PARAM *vig_para)
{
	UINT32 real_gain, max_gain = 0, i;

	for (i = 0; i < VIG_CH0_LUT_SIZE; i++) {
		if (max_gain < vig_para->ch_r_lut[i]) {
			max_gain = vig_para->ch_r_lut[i];
		}
		if (max_gain < vig_para->ch_b_lut[i]) {
			max_gain = vig_para->ch_b_lut[i];
		}
		if (g_ife_param.bayer_format == IFE_BAYER_RGGB) {
			if (max_gain < vig_para->ch_gr_lut[i]) {
				max_gain = vig_para->ch_gr_lut[i];
			}
			if (max_gain < vig_para->ch_gb_lut[i]) {
				max_gain = vig_para->ch_gb_lut[i];
			}
		} else if (g_ife_param.bayer_format == IFE_BAYER_RGBIR) {
			if (max_gain < vig_para->ch_gr_lut[i]) {
				max_gain = vig_para->ch_gr_lut[i];
			}
			if (max_gain < vig_para->ch_ir_lut[i]) {
				max_gain = vig_para->ch_ir_lut[i];
			}
		}
	}

	real_gain = 1 + (max_gain >> 10);
	if (real_gain < 2) {
		p_vig_param->vig_tab_gain = 0;
	} else if (real_gain < 3) {
		p_vig_param->vig_tab_gain = 1;
	} else if (real_gain < 5) {
		p_vig_param->vig_tab_gain = 2;
	} else if (real_gain < 9) {
		p_vig_param->vig_tab_gain = 3;
	} else {
		DBG_ERR("Wrong gain value !!");
	}

	for (i = 0; i < VIG_CH0_LUT_SIZE; i++) {
		vig_para->ch_r_lut [i] = vig_para->ch_r_lut [i] >> p_vig_param->vig_tab_gain;
		vig_para->ch_gr_lut[i] = vig_para->ch_gr_lut[i] >> p_vig_param->vig_tab_gain;
		vig_para->ch_gb_lut[i] = vig_para->ch_gb_lut[i] >> p_vig_param->vig_tab_gain;
		vig_para->ch_b_lut [i] = vig_para->ch_b_lut [i] >> p_vig_param->vig_tab_gain;
		vig_para->ch_ir_lut[i] = vig_para->ch_ir_lut[i] >> p_vig_param->vig_tab_gain;
	}
	if (g_ife_param.bayer_format == IFE_BAYER_RGGB) {
		p_vig_param->p_vig_lut_c0  = &vig_para->ch_r_lut [0];
		p_vig_param->p_vig_lut_c1  = &vig_para->ch_gr_lut[0];
		p_vig_param->p_vig_lut_c2  = &vig_para->ch_gb_lut[0];
		p_vig_param->p_vig_lut_c3  = &vig_para->ch_b_lut [0];
	} else {
		p_vig_param->p_vig_lut_c0  = &vig_para->ch_r_lut  [0];
		p_vig_param->p_vig_lut_c1  = &vig_para->ch_gr_lut [0];
		p_vig_param->p_vig_lut_c2  = &vig_para->ch_ir_lut [0];
		p_vig_param->p_vig_lut_c3  = &vig_para->ch_b_lut  [0];
	}
}
static int ife_cal_vig_setting(IFE_VIG_PARAM *p_vig_param, UINT32 in_w, UINT32 in_h)
{
	UINT32 ui_abs_dif_max_x, ui_abs_dif_max_y;
	UINT32 ui_ratio_x, ui_ratio_y;
	UINT32 uiN, uiM, uiK, uiL, uiJ, uiI, uiH, uiG;
	UINT32 rate_x, rate_y, center_x, center_y;

	UINT64 numerator, denominator;

	center_x = p_vig_param->p_vig_x[0];
	center_y = p_vig_param->p_vig_y[0];

	rate_x = 1000;
	rate_y = rate_x * in_h / in_w;

	// step-1, LUT coverage
	uiN = 16 << 6; // cover 0~16 LUT

	// step-2, before sqrt
	uiM = uiN * uiN;

	// step-3, before addition
	if (center_x >= (in_w / 2))    {
		ui_abs_dif_max_x = center_x;
	} else {
		ui_abs_dif_max_x = in_w - center_x;
	}
	if (ui_abs_dif_max_x > 0x1fff) {
		//nvt_dbg(WRN,"x dist >0x1fff");
		ui_abs_dif_max_x = 0x1fff;
	}

	ui_ratio_x = ui_abs_dif_max_x * rate_x / in_w;
	ui_ratio_x = ui_ratio_x * ui_ratio_x;

	if (center_y >= (in_h / 2))    {
		ui_abs_dif_max_y = center_y;
	} else {
		ui_abs_dif_max_y = in_h - center_y;
	}
	if (ui_abs_dif_max_y > 0x1fff) {
		//nvt_dbg(WRN,"y dist >0x1fff");
		ui_abs_dif_max_y = 0x1fff;
	}
	ui_ratio_y = ui_abs_dif_max_y * rate_y / in_h;
	ui_ratio_y = ui_ratio_y * ui_ratio_y;

//	uiK = (UINT32)((UINT64)uiM * (UINT64)ui_ratioX / (UINT64)(ui_ratioX + ui_ratioY));
//	uiL = (UINT32)((UINT64)uiM * (UINT64)ui_ratioY / (UINT64)(ui_ratioX + ui_ratioY));

	numerator = (UINT64)uiM;
	numerator *= ((UINT64)(ui_ratio_x));
	denominator = (UINT64)(ui_ratio_x + ui_ratio_y);
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	numerator = numerator / denominator;
#else
	do_div(numerator, denominator);
#endif
	uiK = (UINT32)numerator;

	numerator = uiM;
	numerator *= ui_ratio_y;
	denominator = (UINT64)(ui_ratio_x + ui_ratio_y);
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	numerator = numerator / denominator;
#else
	do_div(numerator, denominator);
#endif
	uiL = (UINT32)(numerator);

	// step-4, before 2th-power
	uiI = (UINT32)(vig_square(uiK));
	uiJ = (UINT32)(vig_square(uiL));

	// step-5 DIV-parameters
	if (ui_abs_dif_max_x == 0 || ui_abs_dif_max_y == 0) {
		DBG_ERR("Vig center or input size error!\r\n");
		return -1;
	}

	uiG = ((uiI << 10) / ui_abs_dif_max_x) >> 8;

	if (uiG > 0xfff)    {
		//DBG_WRN("XDIV %x clamped to 0xfff\r\n", (unsigned int)uiG);
		uiG = 0xfff;
	} /*else if (uiG < 100) {
        //DBG_WRN("XDIV accuracy is 1/%d\r\n", (unsigned int)uiG);
    }*/

	uiH = ((uiJ << 10) / ui_abs_dif_max_y) >> 8;

	if (uiH > 0xfff)    {
		//DBG_WRN("YDIV %x clamped to 0xfff\r\n", (unsigned int)uiH);
		uiH = 0xfff;
	} /*else if (uiH < 100) {
        //DBG_WRN("YDIV accuracy is 1/%d\r\n", (unsigned int)uiH);
    }*/
	// step-6 output
	p_vig_param->vig_xdiv = uiG;
	p_vig_param->vig_ydiv = uiH;
	return 0;
}

static ER kdrv_ife_update_in_info(UINT32 id)
{
	IFE_CFASEL CFAMap_RGGB[] = {IFE_PAT0, IFE_PAT1, IFE_PAT2, IFE_PAT3};
	IFE_CFASEL CFAMap_RGBIr[] = {IFE_PAT0, IFE_PAT1, IFE_PAT2, IFE_PAT3, IFE_PAT4, IFE_PAT5, IFE_PAT6, IFE_PAT7};

	g_ife_param.bayer_format = g_kdrv_ife_info[id].in_img_info.type;
	g_ife_param.in_bit         = g_kdrv_ife_info[id].in_img_info.img_info.bit;
	g_ife_param.width       = g_kdrv_ife_info[id].in_img_info.img_info.width;
	g_ife_param.height      = g_kdrv_ife_info[id].in_img_info.img_info.height;
	g_ife_param.crop_width   = g_kdrv_ife_info[id].in_img_info.img_info.crop_width;
	g_ife_param.crop_height  = g_kdrv_ife_info[id].in_img_info.img_info.crop_height;
	g_ife_param.crop_hpos    = g_kdrv_ife_info[id].in_img_info.img_info.crop_hpos;
	g_ife_param.crop_vpos    = g_kdrv_ife_info[id].in_img_info.img_info.crop_vpos;

//--------------------------------------------------------------------------------------
	if (g_kdrv_ife_info[id].in_img_info.img_info.st_pix < KDRV_IPP_RGGB_PIX_MAX) {
		g_ife_param.bayer_format   = IFE_BAYER_RGGB;
		g_ife_param.cfa_pat          = CFAMap_RGGB[g_kdrv_ife_info[id].in_img_info.img_info.st_pix - KDRV_IPP_RGGB_PIX_R];
	} /*else if (g_kdrv_ife_info[id].in_img_info.img_info.st_pix >= KDRV_IPP_RGBIR_PIX_RIR && g_kdrv_ife_info[id].in_img_info.img_info.st_pix < KDRV_IPP_RGBIR_PIX_MAX) {
        g_ife_param.bayer_format   = IFE_BAYER_RGBIR;
        g_ife_param.cfa_pat          = CFAMap_RGBIr[g_kdrv_ife_info[id].in_img_info.img_info.st_pix - KDRV_IPP_RGBIR_PIX_RIR];
    }*/else if (g_kdrv_ife_info[id].in_img_info.img_info.st_pix >= KDRV_IPP_RGBIR_PIX_RG_GI && g_kdrv_ife_info[id].in_img_info.img_info.st_pix < KDRV_IPP_RGBIR_PIX_MAX) {
		g_ife_param.bayer_format   = IFE_BAYER_RGBIR;
		g_ife_param.cfa_pat          = CFAMap_RGBIr[g_kdrv_ife_info[id].in_img_info.img_info.st_pix - KDRV_IPP_RGBIR_PIX_RG_GI];
	}  else {
		g_ife_param.bayer_format   = IFE_BAYER_RGGB;
		g_ife_param.cfa_pat          = IFE_PAT0;
		DBG_ERR("IFE only support RGGB/RGBIR start pixel\r\n");
	}
//--------------------------------------------------------------------------------------

	g_ife_param.ife_set_sel[IFE_SET_INFO] = TRUE;

	return E_OK;
}

static ER kdrv_ife_update_in_addr_1(UINT32 id)
{
	g_ife_param.in_addr0 = g_kdrv_ife_info[id].in_pixel_addr_1;

	g_ife_param.ife_set_sel[IFE_SET_INFO] = TRUE;

	return E_OK;
}

static ER kdrv_ife_update_in_addr_2(UINT32 id)
{
	g_ife_param.in_addr1 = g_kdrv_ife_info[id].in_pixel_addr_2;

	g_ife_param.ife_set_sel[IFE_SET_INFO] = TRUE;

	return E_OK;
}

static ER kdrv_ife_update_in_ofs_1(UINT32 id)
{
	g_ife_param.in_ofs0 = g_kdrv_ife_info[id].line_ofs_1;

	g_ife_param.ife_set_sel[IFE_SET_INFO] = TRUE;

	return E_OK;
}

static ER kdrv_ife_update_in_ofs_2(UINT32 id)
{
	g_ife_param.in_ofs1 = g_kdrv_ife_info[id].line_ofs_2;

	g_ife_param.ife_set_sel[IFE_SET_INFO] = TRUE;

	return E_OK;
}

static ER kdrv_ife_update_out_info(UINT32 id)
{
	g_ife_param.out_ofs = g_kdrv_ife_info[id].out_img_info.line_ofs;

	// ife output
	if (g_kdrv_ife_info[id].in_img_info.in_src == KDRV_IFE_IN_MODE_D2D) {
		g_ife_param.out_bit  = g_kdrv_ife_info[id].out_img_info.bit;
	} else { //D2D
		g_ife_param.out_bit  = KDRV_IFE_RAW_BIT_12;
	}

	g_ife_param.ife_set_sel[IFE_SET_INFO] = TRUE;

	return E_OK;
}

static ER kdrv_ife_update_out_addr(UINT32 id)
{
	g_ife_param.out_addr = g_kdrv_ife_info[id].out_pixel_addr;

	g_ife_param.ife_set_sel[IFE_SET_INFO] = TRUE;

	return E_OK;
}

static ER kdrv_ife_update_ring_buf_info(UINT32 id)
{
	if (g_kdrv_ife_info[id].info_ring_buf.dmaloop_en == TRUE && g_kdrv_ife_info[id].info_fusion.fnum == 0) {
		DBG_IND("if just 1 input, dmaloop can not enable! \r\n");
	}

	g_ife_param.ife_ringbuf_info.dmaloop_en   = (g_kdrv_ife_info[id].info_ring_buf.dmaloop_en & g_kdrv_ife_info[id].info_fusion.fnum);
	g_ife_param.ife_ringbuf_info.dmaloop_line = g_kdrv_ife_info[id].info_ring_buf.dmaloop_line;
    if (g_ife_param.ife_ringbuf_info.dmaloop_en == 1 && g_ife_param.ife_ringbuf_info.dmaloop_line == 0) {
		DBG_ERR("ring buf line can not set to 0 when ring buf is enable\r\n");
	}
	g_ife_param.ife_set_sel[IFE_SET_RING_BUF] = TRUE;

	return E_OK;
}

static ER kdrv_ife_update_dma_wait_sie2_st_disable(UINT32 id)
{
	g_ife_param.b_ife_dma_wait_sie2_st_disable  = g_kdrv_ife_info[id].info_wait_sie2.dma_wait_sie2_start_disable;

	g_ife_param.ife_set_sel[IFE_SET_WAIT_SIE2] = TRUE;

	return E_OK;
}

static ER kdrv_ife_update_enable_mirror(UINT32 id)
{
	g_ife_param.ife_ctrl.b_mirror_en = g_kdrv_ife_info[id].par_mirror.enable;

	g_ife_param.ife_set_sel[IFE_SET_MIRROR] = TRUE;

	return E_OK;
}

static ER kdrv_ife_update_rde_info(UINT32 id)
{
	g_ife_param.ife_ctrl.b_decode_en    = g_kdrv_ife_info[id].info_rde.enable;
	g_ife_param.rde_set.encode_rate    = g_kdrv_ife_info[id].info_rde.encode_rate;

	g_ife_param.ife_set_sel[IFE_SET_RDE] = TRUE;

	return E_OK;
}

static ER kdrv_ife_update_nrs(UINT32 id)
{
	g_ife_param.ife_ctrl.b_nrs_en = g_kdrv_ife_info[id].par_nrs.enable;

	g_ife_param.nrs_set.b_ord_en        = g_kdrv_ife_info[id].par_nrs.order.enable;
	g_ife_param.nrs_set.b_bilat_en      = g_kdrv_ife_info[id].par_nrs.bilat.b_enable;
	g_ife_param.nrs_set.b_gbilat_en     = g_kdrv_ife_info[id].par_nrs.bilat.gb_enable;

	g_ife_param.nrs_set.ord_range_bri   = g_kdrv_ife_info[id].par_nrs.order.rng_bri;
	g_ife_param.nrs_set.ord_range_dark  = g_kdrv_ife_info[id].par_nrs.order.rng_dark;
	g_ife_param.nrs_set.ord_diff_th     = g_kdrv_ife_info[id].par_nrs.order.diff_th;
	g_ife_param.nrs_set.p_ord_bri_w      = &g_kdrv_ife_info[id].par_nrs.order.bri_w[0];
	g_ife_param.nrs_set.p_ord_dark_w     = &g_kdrv_ife_info[id].par_nrs.order.dark_w[0];

	g_ife_param.nrs_set.bilat_str      = g_kdrv_ife_info[id].par_nrs.bilat.b_str;
	g_ife_param.nrs_set.p_bilat_offset  = &g_kdrv_ife_info[id].par_nrs.bilat.b_ofs[0];
	g_ife_param.nrs_set.p_bilat_range   = &g_kdrv_ife_info[id].par_nrs.bilat.b_rng[0];
	g_ife_param.nrs_set.p_bilat_th      = &g_kdrv_ife_info[id].par_nrs.bilat.b_th[0];
	g_ife_param.nrs_set.p_bilat_weight  = &g_kdrv_ife_info[id].par_nrs.bilat.b_weight[0];

	g_ife_param.nrs_set.gbilat_str     = g_kdrv_ife_info[id].par_nrs.bilat.gb_str;
	g_ife_param.nrs_set.gbilat_w       = g_kdrv_ife_info[id].par_nrs.bilat.gb_blend_w;
	g_ife_param.nrs_set.p_gbilat_weight = &g_kdrv_ife_info[id].par_nrs.bilat.gb_weight[0];
	g_ife_param.nrs_set.p_gbilat_offset = &g_kdrv_ife_info[id].par_nrs.bilat.gb_ofs[0];
	g_ife_param.nrs_set.p_gbilat_range  = &g_kdrv_ife_info[id].par_nrs.bilat.gb_rng[0];
	g_ife_param.nrs_set.p_gbilat_th     = &g_kdrv_ife_info[id].par_nrs.bilat.gb_th[0];

	g_ife_param.ife_set_sel[IFE_SET_NRS] = TRUE;

	return E_OK;
}

static ER kdrv_ife_update_fusion_info(UINT32 id)
{
	g_ife_param.ife_ctrl.b_fusion_en = g_kdrv_ife_info[id].info_fusion.enable;
	g_ife_param.fusion_number      = g_kdrv_ife_info[id].info_fusion.fnum;

	g_ife_param.ife_set_sel[IFE_SET_RDE] = TRUE;

	return E_OK;
}

static ER kdrv_ife_update_fusion(UINT32 id)
{
	g_ife_param.fusion_set.y_mean_sel = g_kdrv_ife_info[id].par_fusion.fu_ctrl.y_mean_sel;
	g_ife_param.fusion_set.mode     = g_kdrv_ife_info[id].par_fusion.fu_ctrl.mode;
	g_ife_param.fusion_set.ev_ratio  = g_kdrv_ife_info[id].par_fusion.fu_ctrl.ev_ratio;

	g_ife_param.fusion_set.nor_blendcur_sel         = g_kdrv_ife_info[id].par_fusion.bld_cur.nor_sel;
	g_ife_param.fusion_set.diff_blendcur_sel        = g_kdrv_ife_info[id].par_fusion.bld_cur.dif_sel;
	g_ife_param.fusion_set.p_long_nor_blendcur_knee   = &g_kdrv_ife_info[id].par_fusion.bld_cur.l_nor_knee[0];
	g_ife_param.fusion_set.p_short_nor_blendcur_knee  = &g_kdrv_ife_info[id].par_fusion.bld_cur.s_nor_knee[0];
	g_ife_param.fusion_set.p_long_diff_blendcur_knee  = &g_kdrv_ife_info[id].par_fusion.bld_cur.l_dif_knee[0];
	g_ife_param.fusion_set.p_short_diff_blendcur_knee = &g_kdrv_ife_info[id].par_fusion.bld_cur.s_dif_knee[0];
	g_ife_param.fusion_set.long_nor_blendcur_range   = g_kdrv_ife_info[id].par_fusion.bld_cur.l_nor_range;
	g_ife_param.fusion_set.long_nor_blendcur_wedge   = g_kdrv_ife_info[id].par_fusion.bld_cur.l_nor_w_edge;
	g_ife_param.fusion_set.short_nor_blendcur_range  = g_kdrv_ife_info[id].par_fusion.bld_cur.s_nor_range;
	g_ife_param.fusion_set.short_nor_blendcur_wedge  = g_kdrv_ife_info[id].par_fusion.bld_cur.s_nor_w_edge;
	g_ife_param.fusion_set.long_diff_blendcur_range  = g_kdrv_ife_info[id].par_fusion.bld_cur.l_dif_range;
	g_ife_param.fusion_set.long_diff_blendcur_wedge  = g_kdrv_ife_info[id].par_fusion.bld_cur.l_dif_w_edge;
	g_ife_param.fusion_set.short_diff_blendcur_range = g_kdrv_ife_info[id].par_fusion.bld_cur.s_dif_range;
	g_ife_param.fusion_set.short_diff_blendcur_wedge = g_kdrv_ife_info[id].par_fusion.bld_cur.s_dif_w_edge;

	g_ife_param.fusion_set.mc_diff_lumth_diff_w = g_kdrv_ife_info[id].par_fusion.mc_para.dwd;
	g_ife_param.fusion_set.mc_diff_ratio      = g_kdrv_ife_info[id].par_fusion.mc_para.diff_ratio;
	g_ife_param.fusion_set.mc_lum_th          = g_kdrv_ife_info[id].par_fusion.mc_para.lum_th;
	g_ife_param.fusion_set.p_mc_neg_diff_w      = &g_kdrv_ife_info[id].par_fusion.mc_para.neg_diff_w[0];
	g_ife_param.fusion_set.p_mc_pos_diff_w      = &g_kdrv_ife_info[id].par_fusion.mc_para.pos_diff_w[0];

	g_ife_param.fusion_set.p_dark_sat_reduce_th       = &g_kdrv_ife_info[id].par_fusion.dk_sat.th[0];
	g_ife_param.fusion_set.p_dark_sat_reduce_step     = &g_kdrv_ife_info[id].par_fusion.dk_sat.step[0];
	g_ife_param.fusion_set.p_dark_sat_reduce_lowbound = &g_kdrv_ife_info[id].par_fusion.dk_sat.low_bound[0];

	g_ife_param.ife_ctrl.b_fcgain_en = g_kdrv_ife_info[id].par_fusion.fu_cgain.enable;
	if (g_kdrv_ife_info[id].info_fusion.enable == TRUE) {
		//g_ife_param.ife_ctrl.b_fcgain_en        = TRUE;
		g_ife_param.ife_ctrl.b_scompression_en  = g_kdrv_ife_info[id].par_fusion.s_comp.enable;
	} else {
		//g_ife_param.ife_ctrl.b_fcgain_en        = FALSE;
		g_ife_param.ife_ctrl.b_scompression_en  = FALSE;
	}
  
	g_ife_param.fusion_set.p_short_comp_knee_point = &g_kdrv_ife_info[id].par_fusion.s_comp.knee[0];
	g_ife_param.fusion_set.p_short_comp_sub_point  = &g_kdrv_ife_info[id].par_fusion.s_comp.sub_point[0];
	g_ife_param.fusion_set.p_short_comp_shift_bit  = &g_kdrv_ife_info[id].par_fusion.s_comp.shift[0];

	g_ife_param.fcgain_set.fcgain_range       = g_kdrv_ife_info[id].par_fusion.fu_cgain.bit_field;

	g_ife_param.fcgain_set.p_fusion_cgain_path0[0] = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path0_r;
	g_ife_param.fcgain_set.p_fusion_cgain_path0[1] = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path0_gr;
	g_ife_param.fcgain_set.p_fusion_cgain_path0[2] = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path0_gb;
	g_ife_param.fcgain_set.p_fusion_cgain_path0[3] = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path0_b;
	g_ife_param.fcgain_set.p_fusion_cgain_path0[4] = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path0_ir;

	g_ife_param.fcgain_set.p_fusion_cgain_path1[0] = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path1_r;
	g_ife_param.fcgain_set.p_fusion_cgain_path1[1] = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path1_gr;
	g_ife_param.fcgain_set.p_fusion_cgain_path1[2] = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path1_gb;
	g_ife_param.fcgain_set.p_fusion_cgain_path1[3] = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path1_b;
	g_ife_param.fcgain_set.p_fusion_cgain_path1[4] = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path1_ir;

	g_ife_param.fcgain_set.p_fusion_cofs_path0[0]  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path0_r;
	g_ife_param.fcgain_set.p_fusion_cofs_path0[1]  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path0_gr;
	g_ife_param.fcgain_set.p_fusion_cofs_path0[2]  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path0_gb;
	g_ife_param.fcgain_set.p_fusion_cofs_path0[3]  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path0_b;
	g_ife_param.fcgain_set.p_fusion_cofs_path0[4]  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path0_ir;

	g_ife_param.fcgain_set.p_fusion_cofs_path1[0]  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path1_r;
	g_ife_param.fcgain_set.p_fusion_cofs_path1[1]  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path1_gr;
	g_ife_param.fcgain_set.p_fusion_cofs_path1[2]  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path1_gb;
	g_ife_param.fcgain_set.p_fusion_cofs_path1[3]  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path1_b;
	g_ife_param.fcgain_set.p_fusion_cofs_path1[4]  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path1_ir;

	g_ife_param.ife_set_sel[IFE_SET_FUSION] = TRUE;

	return E_OK;
}
static ER kdrv_ife_update_fcurve(UINT32 id)
{
	if (g_kdrv_ife_info[id].info_fusion.enable == TRUE) {
		g_ife_param.ife_ctrl.b_fcurve_en = TRUE;
	} else {
		g_ife_param.ife_ctrl.b_fcurve_en = g_kdrv_ife_info[id].par_fcurve.enable;
	}

	g_ife_param.fcurve_set.yv_weight    = g_kdrv_ife_info[id].par_fcurve.fcur_ctrl.yv_w;
	g_ife_param.fcurve_set.y_mean_select = g_kdrv_ife_info[id].par_fcurve.fcur_ctrl.y_mean_sel;
	g_ife_param.fcurve_set.p_y_weight_lut = &g_kdrv_ife_info[id].par_fcurve.y_weight.y_w_lut[0];
	g_ife_param.fcurve_set.p_index_lut   = &g_kdrv_ife_info[id].par_fcurve.index.idx_lut[0];
	g_ife_param.fcurve_set.p_split_lut   = &g_kdrv_ife_info[id].par_fcurve.split.split_lut[0];
	g_ife_param.fcurve_set.p_value_lut   = &g_kdrv_ife_info[id].par_fcurve.value.val_lut[0];

	g_ife_param.ife_set_sel[IFE_SET_FCURVE] = TRUE;

	return E_OK;
}
static ER kdrv_ife_update_outl(UINT32 id)
{
	g_ife_param.ife_ctrl.b_outl_en = g_kdrv_ife_info[id].par_outl.enable;

	g_ife_param.outl_set.p_bri_th       = &g_kdrv_ife_info[id].par_outl.bright_th[0];
	g_ife_param.outl_set.p_dark_th      = &g_kdrv_ife_info[id].par_outl.dark_th[0];
	g_ife_param.outl_set.p_outl_cnt     = &g_kdrv_ife_info[id].par_outl.outl_cnt[0];
	g_ife_param.outl_set.outl_w        = g_kdrv_ife_info[id].par_outl.outl_weight;
	g_ife_param.outl_set.dark_ofs      = g_kdrv_ife_info[id].par_outl.dark_ofs;
	g_ife_param.outl_set.bright_ofs    = g_kdrv_ife_info[id].par_outl.bright_ofs;

	g_ife_param.outl_set.b_avg_mode     = g_kdrv_ife_info[id].par_outl.avg_mode;

	g_ife_param.outl_set.ord_range_bri  = g_kdrv_ife_info[id].par_outl.ord_rng_bri;
	g_ife_param.outl_set.ord_range_dark = g_kdrv_ife_info[id].par_outl.ord_rng_dark;
	g_ife_param.outl_set.ord_protect_th = g_kdrv_ife_info[id].par_outl.ord_protect_th;
#if NAME_ERR
	g_ife_param.outl_set.ord_blend_w    = g_kdrv_ife_info[id].par_outl.ord_blend_w;
#else
	g_ife_param.outl_set.ord_blend_w    = g_kdrv_ife_info[id].par_outl.ord_bleand_w;
#endif

	g_ife_param.outl_set.p_ord_bri_w     = &g_kdrv_ife_info[id].par_outl.ord_bri_w[0];
	g_ife_param.outl_set.p_ord_dark_w    = &g_kdrv_ife_info[id].par_outl.ord_dark_w[0];

	g_ife_param.ife_set_sel[IFE_SET_OUTL] = TRUE;

	return E_OK;
}
static ER kdrv_ife_update_filter(UINT32 id)
{
	g_ife_param.ife_ctrl.b_filter_en = g_kdrv_ife_info[id].par_filter.enable;

	g_ife_param.p_spatial_weight    = &g_kdrv_ife_info[id].par_filter.spatial.weight[0];

	g_ife_param.rng_th_a.p_rnglut_c0 = &g_kdrv_ife_info[id].par_filter.rng_filt_r.a_lut[0];
	g_ife_param.rng_th_a.p_rngth_c0  = &g_kdrv_ife_info[id].par_filter.rng_filt_r.a_th[0];
	g_ife_param.rng_th_b.p_rnglut_c0 = &g_kdrv_ife_info[id].par_filter.rng_filt_r.b_lut[0];
	g_ife_param.rng_th_b.p_rngth_c0  = &g_kdrv_ife_info[id].par_filter.rng_filt_r.b_th[0];

	g_ife_param.rng_th_a.p_rnglut_c3 = &g_kdrv_ife_info[id].par_filter.rng_filt_b.a_lut[0];
	g_ife_param.rng_th_a.p_rngth_c3  = &g_kdrv_ife_info[id].par_filter.rng_filt_b.a_th[0];
	g_ife_param.rng_th_b.p_rnglut_c3 = &g_kdrv_ife_info[id].par_filter.rng_filt_b.b_lut[0];
	g_ife_param.rng_th_b.p_rngth_c3  = &g_kdrv_ife_info[id].par_filter.rng_filt_b.b_th[0];
	if (g_ife_param.bayer_format == IFE_BAYER_RGGB) {
		g_ife_param.rng_th_a.p_rnglut_c1 = &g_kdrv_ife_info[id].par_filter.rng_filt_gr.a_lut[0];
		g_ife_param.rng_th_a.p_rngth_c1  = &g_kdrv_ife_info[id].par_filter.rng_filt_gr.a_th[0];
		g_ife_param.rng_th_b.p_rnglut_c1 = &g_kdrv_ife_info[id].par_filter.rng_filt_gr.b_lut[0];
		g_ife_param.rng_th_b.p_rngth_c1  = &g_kdrv_ife_info[id].par_filter.rng_filt_gr.b_th[0];

		g_ife_param.rng_th_a.p_rnglut_c2 = &g_kdrv_ife_info[id].par_filter.rng_filt_gb.a_lut[0];
		g_ife_param.rng_th_a.p_rngth_c2  = &g_kdrv_ife_info[id].par_filter.rng_filt_gb.a_th[0];
		g_ife_param.rng_th_b.p_rnglut_c2 = &g_kdrv_ife_info[id].par_filter.rng_filt_gb.b_lut[0];
		g_ife_param.rng_th_b.p_rngth_c2  = &g_kdrv_ife_info[id].par_filter.rng_filt_gb.b_th[0];
	} else if (g_ife_param.bayer_format == IFE_BAYER_RGBIR) {
		g_ife_param.rng_th_a.p_rnglut_c1 = &g_kdrv_ife_info[id].par_filter.rng_filt_gr.a_lut[0];
		g_ife_param.rng_th_a.p_rngth_c1  = &g_kdrv_ife_info[id].par_filter.rng_filt_gr.a_th[0];
		g_ife_param.rng_th_b.p_rnglut_c1 = &g_kdrv_ife_info[id].par_filter.rng_filt_gr.b_lut[0];
		g_ife_param.rng_th_b.p_rngth_c1  = &g_kdrv_ife_info[id].par_filter.rng_filt_gr.b_th[0];

		g_ife_param.rng_th_a.p_rnglut_c2 = &g_kdrv_ife_info[id].par_filter.rng_filt_ir.a_lut[0];
		g_ife_param.rng_th_a.p_rngth_c2  = &g_kdrv_ife_info[id].par_filter.rng_filt_ir.a_th[0];
		g_ife_param.rng_th_b.p_rnglut_c2 = &g_kdrv_ife_info[id].par_filter.rng_filt_ir.b_lut[0];
		g_ife_param.rng_th_b.p_rngth_c2  = &g_kdrv_ife_info[id].par_filter.rng_filt_ir.b_th[0];
	}

	g_ife_param.ife_ctrl.b_cen_mod_en       = g_kdrv_ife_info[id].par_filter.center_mod.enable;
	g_ife_param.center_mod_set.bilat_th1 = g_kdrv_ife_info[id].par_filter.center_mod.th1;
	g_ife_param.center_mod_set.bilat_th2 = g_kdrv_ife_info[id].par_filter.center_mod.th2;

	g_ife_param.rth_w                  = g_kdrv_ife_info[id].par_filter.rng_th_w;
	g_ife_param.binn                    = g_kdrv_ife_info[id].par_filter.bin;
	g_ife_param.bilat_w                = g_kdrv_ife_info[id].par_filter.blend_w;

	g_ife_param.clamp_set.clamp_th        = g_kdrv_ife_info[id].par_filter.clamp.th;
	g_ife_param.clamp_set.clamp_mul       = g_kdrv_ife_info[id].par_filter.clamp.mul;
	g_ife_param.clamp_set.clamp_dlt       = g_kdrv_ife_info[id].par_filter.clamp.dlt;

	g_ife_param.ife_ctrl.b_rb_fill_en         = g_kdrv_ife_info[id].par_filter.rbfill.enable;
	g_ife_param.rb_fill_set.p_rbfill_rbluma   = &g_kdrv_ife_info[id].par_filter.rbfill.luma[0];
	g_ife_param.rb_fill_set.p_rbfill_rbratio  = &g_kdrv_ife_info[id].par_filter.rbfill.ratio[0];
	g_ife_param.rb_fill_set.rbfill_ratio_mode = g_kdrv_ife_info[id].par_filter.rbfill.ratio_mode;

	g_ife_param.ife_set_sel[IFE_SET_FILTER] = TRUE;

	return E_OK;

}
static ER kdrv_ife_update_cgain(UINT32 id)
{
	g_ife_param.ife_ctrl.b_cgain_en = g_kdrv_ife_info[id].par_cgain.enable;

	g_ife_param.cgain_set.b_cgain_inv      = g_kdrv_ife_info  [id].par_cgain.inv;
	g_ife_param.cgain_set.b_cgain_hinv     = g_kdrv_ife_info  [id].par_cgain.hinv;
	g_ife_param.cgain_set.cgain_range     = g_kdrv_ife_info  [id].par_cgain.bit_field;
	g_ife_param.cgain_set.cgain_mask    = g_kdrv_ife_info  [id].par_cgain.mask;

	g_ife_param.cgain_set.p_cgain[0]       = g_kdrv_ife_info[id].par_cgain.cgain_r;
	g_ife_param.cgain_set.p_cgain[1]       = g_kdrv_ife_info[id].par_cgain.cgain_gr;
	g_ife_param.cgain_set.p_cgain[2]       = g_kdrv_ife_info[id].par_cgain.cgain_gb;
	g_ife_param.cgain_set.p_cgain[3]       = g_kdrv_ife_info[id].par_cgain.cgain_b;
	g_ife_param.cgain_set.p_cgain[4]       = g_kdrv_ife_info[id].par_cgain.cgain_ir;

	g_ife_param.cgain_set.p_cofs[0]        = g_kdrv_ife_info[id].par_cgain.cofs_r;
	g_ife_param.cgain_set.p_cofs[1]        = g_kdrv_ife_info[id].par_cgain.cofs_gr;
	g_ife_param.cgain_set.p_cofs[2]        = g_kdrv_ife_info[id].par_cgain.cofs_gb;
	g_ife_param.cgain_set.p_cofs[3]        = g_kdrv_ife_info[id].par_cgain.cofs_b;
	g_ife_param.cgain_set.p_cofs[4]        = g_kdrv_ife_info[id].par_cgain.cofs_ir;

	g_ife_param.ife_set_sel[IFE_SET_CGAIN] = TRUE;

	return E_OK;

}
static ER kdrv_ife_update_vig(UINT32 id)
{
	int cal_vig_status;
	g_ife_param.ife_ctrl.b_vig_en = g_kdrv_ife_info[id].par_vig.enable;

	memcpy(g_kdrv_ife_vig_set_info[id].ch_r_lut , g_kdrv_ife_info[id].par_vig.ch_r_lut , sizeof(UINT32)*KDRV_VIG_LUT_LEN);
	memcpy(g_kdrv_ife_vig_set_info[id].ch_gr_lut, g_kdrv_ife_info[id].par_vig.ch_gr_lut, sizeof(UINT32)*KDRV_VIG_LUT_LEN);
	memcpy(g_kdrv_ife_vig_set_info[id].ch_gb_lut, g_kdrv_ife_info[id].par_vig.ch_gb_lut, sizeof(UINT32)*KDRV_VIG_LUT_LEN);
	memcpy(g_kdrv_ife_vig_set_info[id].ch_b_lut , g_kdrv_ife_info[id].par_vig.ch_b_lut , sizeof(UINT32)*KDRV_VIG_LUT_LEN);
	memcpy(g_kdrv_ife_vig_set_info[id].ch_ir_lut, g_kdrv_ife_info[id].par_vig.ch_ir_lut, sizeof(UINT32)*KDRV_VIG_LUT_LEN);

	if (g_ife_param.bayer_format == IFE_BAYER_RGGB) {
		g_ife_param.vig_set.p_vig_lut_c0	  = &g_kdrv_ife_vig_set_info[id].ch_r_lut[0]; //&g_viglut_ch0   [id][0];
		g_ife_param.vig_set.p_vig_lut_c1	  = &g_kdrv_ife_vig_set_info[id].ch_gr_lut[0]; //&g_viglut_ch1	[id][0];
		g_ife_param.vig_set.p_vig_lut_c2	  = &g_kdrv_ife_vig_set_info[id].ch_gb_lut[0];//&g_viglut_ch2   [id][0];
		g_ife_param.vig_set.p_vig_lut_c3	  = &g_kdrv_ife_vig_set_info[id].ch_b_lut[0]; //&g_viglut_ch3   [id][0];
	} else if (g_ife_param.bayer_format == IFE_BAYER_RGBIR) {
		g_ife_param.vig_set.p_vig_lut_c0	  = &g_kdrv_ife_vig_set_info[id].ch_r_lut[0] ; //&g_viglut_ch0	[id][0];
		g_ife_param.vig_set.p_vig_lut_c1	  = &g_kdrv_ife_vig_set_info[id].ch_gr_lut[0] ; //&g_viglut_ch1	 [id][0];
		g_ife_param.vig_set.p_vig_lut_c2	  = &g_kdrv_ife_vig_set_info[id].ch_ir_lut[0];//&g_viglut_ch2   [id][0];
		g_ife_param.vig_set.p_vig_lut_c3	  = &g_kdrv_ife_vig_set_info[id].ch_b_lut[0] ; //&g_viglut_ch3	[id][0];
	}

	g_ife_param.vig_set.p_vig_x           = &g_vig_center_x [id][0];
	g_ife_param.vig_set.p_vig_y           = &g_vig_center_y [id][0];
	g_ife_param.vig_set.vig_dist_th           = g_kdrv_ife_info  [id].par_vig.dist_th;

	g_ife_param.vig_set.b_vig_dither_en     = g_kdrv_ife_info  [id].par_vig.dither_enable;
	g_ife_param.vig_set.b_vig_dither_rst    = g_kdrv_ife_info  [id].par_vig.dither_rst_enable;

	g_ife_param.vig_set.vig_fisheye_gain_en	= g_kdrv_ife_info[id].par_vig.vig_fisheye_gain_en;
	g_ife_param.vig_set.vig_fisheye_slope	= g_kdrv_ife_info[id].par_vig.vig_fisheye_slope;
	g_ife_param.vig_set.vig_fisheye_radius	= g_kdrv_ife_info[id].par_vig.vig_fisheye_radius;

	//if calc. div > 4095 -> clamp to 4095
	//no show warning message to avoid always print message in too large or too small size image
	cal_vig_status = ife_cal_vig_setting(&g_ife_param.vig_set, g_ife_param.width, g_ife_param.height);
	if (cal_vig_status == -1) {
		DBG_ERR("Calculate Vig setting is fail!\r\n");
		DBG_ERR("Vig setting wrong para!\r\n");
	}
	ife_cal_vig_tab_gain(&g_ife_param.vig_set, &g_kdrv_ife_vig_set_info[id]);

	g_ife_param.ife_set_sel[IFE_SET_VIG] = TRUE;
	g_ife_param.ife_set_sel[IFE_SET_VIG_CENTER] = TRUE;

	return E_OK;
}
static ER kdrv_ife_update_gbal(UINT32 id)
{
	g_ife_param.ife_ctrl.b_gbal_en = g_kdrv_ife_info[id].par_gbal.enable;

	g_ife_param.gbal_set.b_protect_en       = g_kdrv_ife_info[id].par_gbal.protect_enable;
	g_ife_param.gbal_set.diff_th_str        = g_kdrv_ife_info[id].par_gbal.diff_th_str;
	g_ife_param.gbal_set.diff_w_max         = g_kdrv_ife_info[id].par_gbal.diff_w_max;
	g_ife_param.gbal_set.edge_protect_th1   = g_kdrv_ife_info[id].par_gbal.edge_protect_th1;
	g_ife_param.gbal_set.edge_protect_th0   = g_kdrv_ife_info[id].par_gbal.edge_protect_th0;
	g_ife_param.gbal_set.edge_w_max         = g_kdrv_ife_info[id].par_gbal.edge_w_max;
	g_ife_param.gbal_set.edge_w_min         = g_kdrv_ife_info[id].par_gbal.edge_w_min;
	g_ife_param.gbal_set.p_gbal_ofs         = &g_kdrv_ife_info[id].par_gbal.gbal_ofs[0];

	g_ife_param.ife_set_sel[IFE_SET_GBAL] = TRUE;

	return E_OK;
}

void kdrv_ife_d2d_stripe_auto_calc(void)
{
	//UINT32 ife_HM_max = 16;
	UINT32 rounding = 0;
	UINT32 ife_HN = 0, ife_HL = 0, ife_HM = 0, time = 0;
	//----------------- Calculate multiple stripe (MST) -----------------------------
	ife_HM = 3;
	while (1) {
		rounding  = ((ife_HM + 1) / 2);
		ife_HN = ((g_ife_param.width + (ife_HM * 24)) + rounding) / (ife_HM + 1);
		ife_HN = ife_HN >> 2; // 4x
		ife_HL = g_ife_param.width - ((ife_HN - 6) * ife_HM * 4);
		ife_HL = ife_HL >> 2; // 4x
		if (ife_HN >= 8 && ife_HL >= 8 && ife_HN <= 672 && ife_HL <= 672) {
			break;
		}

		if (ife_HN <= 8 || ife_HL <= 8) {
			ife_HM --;
		} else if (ife_HN >= 672 || ife_HL >= 672) {
			ife_HM ++;
		}

		time ++;

		if (time > 10) {
			ife_HM = 0;
			ife_HN = 0;
			ife_HL = g_ife_param.width >> 2;
			break;
		}
	}
	g_ife_param.ife_stripe_info.hn  = ife_HN;
	g_ife_param.ife_stripe_info.hl  = ife_HL;
	g_ife_param.ife_stripe_info.hm  = ife_HM;
}

static ER kdrv_ife_setmode(UINT32 id)
{
	ER rt;

	BOOL  ipl_nrs_en,
		  ipl_fcurve_en,
		  ipl_outl_en,
		  ipl_filter_en,
		  ipl_cg_en,
		  ipl_vig_en,
		  ipl_gbal_en;

	BOOL  iq_nrs_en,
		  iq_fcurve_en,
		  iq_outl_en,
		  iq_filter_en,
		  iq_cg_en,
		  iq_vig_en,
		  iq_gbal_en;	
	// ife input
	if (g_kdrv_ife_info[id].in_img_info.in_src == KDRV_IFE_IN_MODE_DIRECT) {
		g_ife_param.mode    = IFE_OPMODE_ALL_DIRECT;
	} else if (g_kdrv_ife_info[id].in_img_info.in_src == KDRV_IFE_IN_MODE_IPP) {
		g_ife_param.mode    = IFE_OPMODE_IPP;
	} else {  //D2D
		g_ife_param.mode    = IFE_OPMODE_D2D;
	}

	kdrv_ife_update_in_info(id);

	kdrv_ife_update_in_addr_1(id);

	kdrv_ife_update_in_addr_2(id);

	kdrv_ife_update_in_ofs_1(id);

	kdrv_ife_update_in_ofs_2(id);

	kdrv_ife_update_out_info(id);

	kdrv_ife_update_out_addr(id);

	if (g_ife_param.mode == IFE_OPMODE_D2D) {
		kdrv_ife_d2d_stripe_auto_calc();
		g_ife_param.h_shift = 0;
	}
	//g_ife_param.ife_stripe_info.hn  = g_kdrv_ife_info[id].in_img_info.img_info.stripe_hn;
	//g_ife_param.ife_stripe_info.hl  = g_kdrv_ife_info[id].in_img_info.img_info.stripe_hl;
	//g_ife_param.ife_stripe_info.hm  = g_kdrv_ife_info[id].in_img_info.img_info.stripe_hm;
	//g_ife_param.h_shift   = g_kdrv_ife_info[id].in_img_info.img_info.h_shift;


	// ife interrupt
	g_ife_param.intr_en    = g_kdrv_ife_info[id].inte_en;

	//Priority of IPL will higher than PQ. Therefore, assigning IPL enable/disable setting in here
	ipl_nrs_en     = (g_kdrv_ife_ipl_ctrl_en[id] & KDRV_IFE_IQ_FUNC_NRS) ? (TRUE) : (FALSE);
	ipl_fcurve_en  = (g_kdrv_ife_ipl_ctrl_en[id] & KDRV_IFE_IQ_FUNC_FCURVE) ? (TRUE) : (FALSE);
	ipl_outl_en     = (g_kdrv_ife_ipl_ctrl_en[id] & KDRV_IFE_IQ_FUNC_OUTL) ? (TRUE) : (FALSE);
	ipl_filter_en   = (g_kdrv_ife_ipl_ctrl_en[id] & KDRV_IFE_IQ_FUNC_FILTER) ? (TRUE) : (FALSE);
	ipl_cg_en       = (g_kdrv_ife_ipl_ctrl_en[id] & KDRV_IFE_IQ_FUNC_CGAIN) ? (TRUE) : (FALSE);
	ipl_vig_en      = (g_kdrv_ife_ipl_ctrl_en[id] & KDRV_IFE_IQ_FUNC_VIG_PARAM) ? (TRUE) : (FALSE);
	ipl_gbal_en     = (g_kdrv_ife_ipl_ctrl_en[id] & KDRV_IFE_IQ_FUNC_GBAL) ? (TRUE) : (FALSE);

	iq_nrs_en      = (g_kdrv_ife_iq_ctrl_en[id] & KDRV_IFE_IQ_FUNC_NRS) ? (TRUE) : (FALSE);
	iq_fcurve_en   = (g_kdrv_ife_iq_ctrl_en[id] & KDRV_IFE_IQ_FUNC_FCURVE) ? (TRUE) : (FALSE);
	iq_outl_en     = (g_kdrv_ife_iq_ctrl_en[id] & KDRV_IFE_IQ_FUNC_OUTL) ? (TRUE) : (FALSE);
	iq_filter_en   = (g_kdrv_ife_iq_ctrl_en[id] & KDRV_IFE_IQ_FUNC_FILTER) ? (TRUE) : (FALSE);
	iq_cg_en       = (g_kdrv_ife_iq_ctrl_en[id] & KDRV_IFE_IQ_FUNC_CGAIN) ? (TRUE) : (FALSE);
	iq_vig_en      = (g_kdrv_ife_iq_ctrl_en[id] & KDRV_IFE_IQ_FUNC_VIG_PARAM) ? (TRUE) : (FALSE);
	iq_gbal_en     = (g_kdrv_ife_iq_ctrl_en[id] & KDRV_IFE_IQ_FUNC_GBAL) ? (TRUE) : (FALSE);
	
	g_kdrv_ife_info[id].par_nrs.enable    = (ipl_nrs_en    & iq_nrs_en);
	g_kdrv_ife_info[id].par_fcurve.enable = (ipl_fcurve_en & iq_fcurve_en);
	g_kdrv_ife_info[id].par_outl.enable   = (ipl_outl_en   & iq_outl_en);
	g_kdrv_ife_info[id].par_filter.enable = (ipl_filter_en & iq_filter_en);
	g_kdrv_ife_info[id].par_cgain.enable  = (ipl_cg_en     & iq_cg_en);
	g_kdrv_ife_info[id].par_vig.enable    = (ipl_vig_en    & iq_vig_en);
	g_kdrv_ife_info[id].par_gbal.enable   = (ipl_gbal_en   & iq_gbal_en);
	/*	
	g_kdrv_ife_info[id].par_nrs.enable    &= ipl_nrs_en;
	g_kdrv_ife_info[id].par_fcurve.enable &= ipl_fcurve_en;
	g_kdrv_ife_info[id].par_outl.enable   &= ipl_outl_en;
	g_kdrv_ife_info[id].par_filter.enable &= ipl_filter_en;
	g_kdrv_ife_info[id].par_cgain.enable  &= ipl_cg_en;
	g_kdrv_ife_info[id].par_vig.enable    &= ipl_vig_en;
	g_kdrv_ife_info[id].par_gbal.enable   &= ipl_gbal_en;
*/


	// ife IPL setting
	//--------------- Mirror ---------------//
	kdrv_ife_update_enable_mirror(id);

	//------------- Ring buffer ------------//
	kdrv_ife_update_ring_buf_info(id);

	//------------- HDR wait SIE2 first line end or not -------------------//
	kdrv_ife_update_dma_wait_sie2_st_disable(id);

	//--------------- Rde ---------------//
	kdrv_ife_update_rde_info(id);
	g_ife_param.rde_set.p_dct_qtable_inx = dct_qtable_inx;
	g_ife_param.rde_set.p_degamma_table = degamma_table;

	// ife IQ setting
	//---------------- Nrs ----------------//
	kdrv_ife_update_nrs(id);

	//---------------- Fusion ----------------//
	kdrv_ife_update_fusion_info(id);
	kdrv_ife_update_fusion(id);

	//---------------- Fcurve ----------------//
	kdrv_ife_update_fcurve(id);

	//---------------- Outlier ----------------//
	kdrv_ife_update_outl(id);

	//---------------- 2DNR ----------------//
	kdrv_ife_update_filter(id);

	//---------------- Color gain ----------------//
	kdrv_ife_update_cgain(id);

	//---------------- Vig ----------------//
	kdrv_ife_update_vig(id);

	//---------------- G balance ----------------//
	kdrv_ife_update_gbal(id);

	rt = ife_set_mode(&g_ife_param);
	return rt;
}


static ER kdrv_ife_set_load(UINT32 id, void *data)
{
	UINT32 i;
	UINT32 ife_lock_flag;
	UINT32 ife_update_flag_tmp[KDRV_IFE_PARAM_MAX] = {FALSE};

	IFE_CFASEL CFAMap_RGGB[] = {IFE_PAT0, IFE_PAT1, IFE_PAT2, IFE_PAT3};
	IFE_CFASEL CFAMap_RGBIr[] = {IFE_PAT0, IFE_PAT1, IFE_PAT2, IFE_PAT3, IFE_PAT4, IFE_PAT5, IFE_PAT6, IFE_PAT7};

	BOOL  ipl_nrs_en,
		  ipl_fcurve_en,
		  ipl_outl_en,
		  ipl_filter_en,
		  ipl_cg_en,
		  ipl_vig_en,
		  ipl_gbal_en;

	BOOL  iq_nrs_en,
		  iq_fcurve_en,
		  iq_outl_en,
		  iq_filter_en,
		  iq_cg_en,
		  iq_vig_en,
		  iq_gbal_en;

	ife_lock_flag = ife_platform_spin_lock();
	for (i = 0; i < KDRV_IFE_PARAM_MAX; i++) {
		ife_update_flag_tmp[i]   = g_ife_update_flag[id][i];
		g_ife_update_flag[id][i] = FALSE;
	}
	ife_platform_spin_unlock(ife_lock_flag);

	// ife input
	if (g_kdrv_ife_info[id].in_img_info.in_src == KDRV_IFE_IN_MODE_DIRECT) {
		g_ife_param.mode    = IFE_OPMODE_ALL_DIRECT;
	} else if (g_kdrv_ife_info[id].in_img_info.in_src == KDRV_IFE_IN_MODE_IPP) {
		g_ife_param.mode    = IFE_OPMODE_IPP;
	} else {  //D2D
		g_ife_param.mode    = IFE_OPMODE_D2D;
	}

	g_ife_param.in_bit   = g_kdrv_ife_info[id].in_img_info.img_info.bit;
//--------------------------------------------------------------------------------------
	if (g_kdrv_ife_info[id].in_img_info.img_info.st_pix < KDRV_IPP_RGGB_PIX_MAX) {
		g_ife_param.bayer_format   = IFE_BAYER_RGGB;
		g_ife_param.cfa_pat          = CFAMap_RGGB[g_kdrv_ife_info[id].in_img_info.img_info.st_pix - KDRV_IPP_RGGB_PIX_R];
	} /*else if (g_kdrv_ife_info[id].in_img_info.img_info.st_pix >= KDRV_IPP_RGBIR_PIX_RIR && g_kdrv_ife_info[id].in_img_info.img_info.st_pix < KDRV_IPP_RGBIR_PIX_MAX) {
        g_ife_param.bayer_format   = IFE_BAYER_RGBIR;
        g_ife_param.cfa_pat          = CFAMap_RGBIr[g_kdrv_ife_info[id].in_img_info.img_info.st_pix - KDRV_IPP_RGBIR_PIX_RIR];
    }*/else if (g_kdrv_ife_info[id].in_img_info.img_info.st_pix >= KDRV_IPP_RGBIR_PIX_RG_GI && g_kdrv_ife_info[id].in_img_info.img_info.st_pix < KDRV_IPP_RGBIR_PIX_MAX) {
		g_ife_param.bayer_format   = IFE_BAYER_RGBIR;
		g_ife_param.cfa_pat          = CFAMap_RGBIr[g_kdrv_ife_info[id].in_img_info.img_info.st_pix - KDRV_IPP_RGBIR_PIX_RG_GI];
	} else {
		g_ife_param.bayer_format   = IFE_BAYER_RGGB;
		g_ife_param.cfa_pat          = IFE_PAT0;
		DBG_ERR("IFE only support RGGB/RGBIR start pixel\r\n");
	}
//--------------------------------------------------------------------------------------

	//Priority of IPL will higher than PQ. Therefore, assigning IPL enable/disable setting in here
	ipl_nrs_en     = (g_kdrv_ife_ipl_ctrl_en[id] & KDRV_IFE_IQ_FUNC_NRS) ? (TRUE) : (FALSE);
	ipl_fcurve_en  = (g_kdrv_ife_ipl_ctrl_en[id] & KDRV_IFE_IQ_FUNC_FCURVE) ? (TRUE) : (FALSE);
	ipl_outl_en     = (g_kdrv_ife_ipl_ctrl_en[id] & KDRV_IFE_IQ_FUNC_OUTL) ? (TRUE) : (FALSE);
	ipl_filter_en   = (g_kdrv_ife_ipl_ctrl_en[id] & KDRV_IFE_IQ_FUNC_FILTER) ? (TRUE) : (FALSE);
	ipl_cg_en       = (g_kdrv_ife_ipl_ctrl_en[id] & KDRV_IFE_IQ_FUNC_CGAIN) ? (TRUE) : (FALSE);
	ipl_vig_en      = (g_kdrv_ife_ipl_ctrl_en[id] & KDRV_IFE_IQ_FUNC_VIG_PARAM) ? (TRUE) : (FALSE);
	ipl_gbal_en     = (g_kdrv_ife_ipl_ctrl_en[id] & KDRV_IFE_IQ_FUNC_GBAL) ? (TRUE) : (FALSE);

	iq_nrs_en     = (g_kdrv_ife_iq_ctrl_en[id] & KDRV_IFE_IQ_FUNC_NRS) ? (TRUE) : (FALSE);
	iq_fcurve_en  = (g_kdrv_ife_iq_ctrl_en[id] & KDRV_IFE_IQ_FUNC_FCURVE) ? (TRUE) : (FALSE);
	iq_outl_en     = (g_kdrv_ife_iq_ctrl_en[id] & KDRV_IFE_IQ_FUNC_OUTL) ? (TRUE) : (FALSE);
	iq_filter_en   = (g_kdrv_ife_iq_ctrl_en[id] & KDRV_IFE_IQ_FUNC_FILTER) ? (TRUE) : (FALSE);
	iq_cg_en       = (g_kdrv_ife_iq_ctrl_en[id] & KDRV_IFE_IQ_FUNC_CGAIN) ? (TRUE) : (FALSE);
	iq_vig_en      = (g_kdrv_ife_iq_ctrl_en[id] & KDRV_IFE_IQ_FUNC_VIG_PARAM) ? (TRUE) : (FALSE);
	iq_gbal_en     = (g_kdrv_ife_iq_ctrl_en[id] & KDRV_IFE_IQ_FUNC_GBAL) ? (TRUE) : (FALSE);	

	g_kdrv_ife_info[id].par_nrs.enable	  = (ipl_nrs_en    & iq_nrs_en);
	g_kdrv_ife_info[id].par_fcurve.enable = (ipl_fcurve_en & iq_fcurve_en);
	g_kdrv_ife_info[id].par_outl.enable   = (ipl_outl_en   & iq_outl_en);
	g_kdrv_ife_info[id].par_filter.enable = (ipl_filter_en & iq_filter_en);
	g_kdrv_ife_info[id].par_cgain.enable  = (ipl_cg_en     & iq_cg_en);
	g_kdrv_ife_info[id].par_vig.enable	  = (ipl_vig_en    & iq_vig_en);
	g_kdrv_ife_info[id].par_gbal.enable   = (ipl_gbal_en   & iq_gbal_en);


/*
	g_kdrv_ife_info[id].par_nrs.enable    &= ipl_nrs_en;
	g_kdrv_ife_info[id].par_fcurve.enable &= ipl_fcurve_en;
	g_kdrv_ife_info[id].par_outl.enable   &= ipl_outl_en;
	g_kdrv_ife_info[id].par_filter.enable &= ipl_filter_en;
	g_kdrv_ife_info[id].par_cgain.enable  &= ipl_cg_en;
	g_kdrv_ife_info[id].par_vig.enable    &= ipl_vig_en;
	g_kdrv_ife_info[id].par_gbal.enable   &= ipl_gbal_en;
*/
	if (ife_update_flag_tmp[KDRV_IFE_PARAM_IPL_IN_IMG] ||
		ife_update_flag_tmp[KDRV_IFE_PARAM_IPL_IN_ADDR_1] ||
		ife_update_flag_tmp[KDRV_IFE_PARAM_IPL_IN_ADDR_2] ||
		ife_update_flag_tmp[KDRV_IFE_PARAM_IPL_IN_OFS_1] ||
		ife_update_flag_tmp[KDRV_IFE_PARAM_IPL_IN_OFS_2] ||
		ife_update_flag_tmp[KDRV_IFE_PARAM_IPL_OUT_IMG] ||
		ife_update_flag_tmp[KDRV_IFE_PARAM_IPL_OUT_ADDR]) {
		kdrv_ife_update_in_info(id);
		kdrv_ife_update_in_addr_1(id);
		kdrv_ife_update_in_addr_2(id);
		kdrv_ife_update_in_ofs_1(id);
		kdrv_ife_update_in_ofs_2(id);
		kdrv_ife_update_out_info(id);
		kdrv_ife_update_out_addr(id);

		ife_change_roi(&g_ife_param);
	}

	if (ife_update_flag_tmp[KDRV_IFE_PARAM_IPL_MIRROR] == TRUE) {
		kdrv_ife_update_enable_mirror(id);
	}
	if (ife_update_flag_tmp[KDRV_IFE_PARAM_IPL_RINGBUF_INFO] == TRUE) {
		kdrv_ife_update_ring_buf_info(id);
	}
	if (ife_update_flag_tmp[KDRV_IFE_PARAM_IPL_WAIT_SIE2_DISABLE] == TRUE) {
		kdrv_ife_update_dma_wait_sie2_st_disable(id);
	}
	if (ife_update_flag_tmp[KDRV_IFE_PARAM_IPL_RDE_INFO] == TRUE) {
		kdrv_ife_update_rde_info(id);
	}
	if (ife_update_flag_tmp[KDRV_IFE_PARAM_IPL_FUSION_INFO] == TRUE) {
		kdrv_ife_update_fusion_info(id);
	}
	if (ife_update_flag_tmp[KDRV_IFE_PARAM_IQ_NRS] == TRUE) {
		kdrv_ife_update_nrs(id);
	}
	if (ife_update_flag_tmp[KDRV_IFE_PARAM_IQ_FUSION] == TRUE) {
		kdrv_ife_update_fusion(id);
	}
	if (ife_update_flag_tmp[KDRV_IFE_PARAM_IQ_FCURVE] == TRUE) {
		kdrv_ife_update_fcurve(id);
	}
	if (ife_update_flag_tmp[KDRV_IFE_PARAM_IQ_OUTL] == TRUE) {
		kdrv_ife_update_outl(id);
	}
	if (ife_update_flag_tmp[KDRV_IFE_PARAM_IQ_FILTER] == TRUE) {
		kdrv_ife_update_filter(id);
	}
	if (ife_update_flag_tmp[KDRV_IFE_PARAM_IQ_CGAIN] == TRUE) {
		kdrv_ife_update_cgain(id);
	}
	if (ife_update_flag_tmp[KDRV_IFE_PARAM_IQ_VIG] == TRUE) {
		kdrv_ife_update_vig(id);
	}
	if (ife_update_flag_tmp[KDRV_IFE_PARAM_IQ_GBAL] == TRUE) {
		kdrv_ife_update_gbal(id);
	}

	ife_change_param(&g_ife_param);

	return E_OK;
}

static void kdrv_ife_lock(KDRV_IFE_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		FLGPTN flag_ptn;
		wai_flg(&flag_ptn, p_handle->flag_id, p_handle->lock_bit, TWF_CLR);
	} else {
		set_flg(p_handle->flag_id, p_handle->lock_bit);
	}
}


static KDRV_IFE_HANDLE *kdrv_ife_chk_handle(KDRV_IFE_HANDLE *p_handle)
{
	UINT32 i;

	for (i = 0; i < kdrv_ife_channel_num; i ++) {
		if (p_handle == &g_kdrv_ife_handle_tab[i]) {
			return p_handle;
		}
	}

	return NULL;
}

static void kdrv_ife_handle_lock(void)
{
	FLGPTN wait_flg;
	wai_flg(&wait_flg, kdrv_ife_get_flag_id(FLG_ID_KDRV_IFE), KDRV_IPP_IFE_HDL_UNLOCK, (TWF_ORW | TWF_CLR));
}

static void kdrv_ife_handle_unlock(void)
{
	set_flg(kdrv_ife_get_flag_id(FLG_ID_KDRV_IFE), KDRV_IPP_IFE_HDL_UNLOCK);
}


static UINT32 kdrv_ife_handle_alloc(UINT32 *eng_init_flag)
{
	UINT32 i;
	//KDRV_IFE_HANDLE* p_handle;

	kdrv_ife_handle_lock();
	//p_handle = NULL;
	*eng_init_flag = FALSE;
	for (i = 0; i < kdrv_ife_channel_num; i ++) {
		if (!(g_kdrv_ife_init_flg & (1 << i))) {

			if (g_kdrv_ife_init_flg == 0) {
				*eng_init_flag = TRUE;
			}
			g_kdrv_ife_init_flg |= (1 << i);

			memset(&g_kdrv_ife_handle_tab[i], 0, sizeof(KDRV_IFE_HANDLE));
			g_kdrv_ife_handle_tab[i].entry_id = i;
			g_kdrv_ife_handle_tab[i].flag_id = kdrv_ife_get_flag_id(FLG_ID_KDRV_IFE);
			g_kdrv_ife_handle_tab[i].lock_bit = (1 << i);
			g_kdrv_ife_handle_tab[i].sts |= KDRV_IFE_HANDLE_LOCK;
			g_kdrv_ife_handle_tab[i].sem_id = kdrv_ife_get_sem_id(SEMID_KDRV_IFE);
			//p_handle = &g_kdrv_ife_handle_tab[i];
			//break;
		}
	}
	kdrv_ife_handle_unlock();
	/*
	    if (i >= KDRV_IFE_HANDLE_MAX_NUM) {
	        DBG_ERR("get free handle fail(0x%.8x)\r\n", g_kdrv_ife_init_flg);
	    }
	*/
	//return p_handle;
	return E_OK;
}

static UINT32 kdrv_ife_handle_free(void)
{
	UINT32 rt = FALSE;
	UINT32 channel;
	KDRV_IFE_HANDLE *p_handle;

	kdrv_ife_handle_lock();

	for (channel = 0; channel < kdrv_ife_channel_num; channel++) {
		p_handle = &g_kdrv_ife_handle_tab[channel];
		p_handle->sts = 0;
		g_kdrv_ife_init_flg &= ~(1 << channel);
		if (g_kdrv_ife_init_flg == 0) {
			rt = TRUE;
		}
	}

	kdrv_ife_handle_unlock();

	return rt;
}

static KDRV_IFE_HANDLE *kdrv_ife_entry_id_conv2_handle(UINT32 entry_id)
{
	return  &g_kdrv_ife_handle_tab[entry_id];
}

static void kdrv_ife_sem(KDRV_IFE_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		SEM_WAIT(*p_handle->sem_id);    // wait semaphore
	} else {
		SEM_SIGNAL_ISR(*p_handle->sem_id);  // wait semaphore
	}
}

static void kdrv_ife_frm_end(KDRV_IFE_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		iset_flg(p_handle->flag_id, KDRV_IPP_IFE_FMD);
	} else {
		clr_flg(p_handle->flag_id, KDRV_IPP_IFE_FMD);
	}
}

static void kdrv_ife_isr_cb(UINT32 intstatus)
{
	KDRV_IFE_HANDLE *p_handle;
	p_handle = g_kdrv_ife_trig_hdl;

	if (p_handle == NULL) {
		return;
	}

	if (intstatus & IFE_INT_FRMEND) {
		//ife_pause();
		kdrv_ife_sem(p_handle, FALSE);
		kdrv_ife_frm_end(p_handle, TRUE);
	}

	if (p_handle->isrcb_fp != NULL) {
		p_handle->isrcb_fp((UINT32)p_handle, intstatus, NULL, NULL);
	}
}

static ER kdrv_ife_set_clock(UINT32 id, void *data)
{
	KDRV_IFE_OPENCFG kdrv_ife_open_obj;

	kdrv_ife_open_obj = *(KDRV_IFE_OPENCFG *)data;
	g_kdrv_ife_open_cfg.ife_clock_sel = kdrv_ife_open_obj.ife_clock_sel;

	//DBG_ERR("IFE KDRV kdrv_ife_set_clock_value\r\n");

	return E_OK;
}

#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)
UINT32 buf_size = 0, buf_addr = 0;
#if defined (__LINUX) || defined (__KERNEL__)
struct nvt_fmem_mem_info_t kdrv_ife_ch_buf = {0};
void *handle_ife_ch_buf = NULL;
int  ret = 0;
#elif defined (__FREERTOS)
UINT32 *kdrv_ife_ch_buf = NULL;
#endif
#endif

void kdrv_ife_init(void)
{
#if !(defined __UITRON || defined __ECOS)
#if defined __FREERTOS
	ife_platform_create_resource();
#endif
	ife_platform_set_clk_rate(280);
#endif
	kdrv_ife_install_id();

#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)

	buf_size = kdrv_ife_buf_query(KDRV_IFE_HANDLE_MAX_NUM);

#if defined (__LINUX) || defined (__KERNEL__)
	ret = nvt_fmem_mem_info_init(&kdrv_ife_ch_buf, NVT_FMEM_ALLOC_CACHE, buf_size, NULL);
	if (ret >= 0) {
		handle_ife_ch_buf = fmem_alloc_from_cma(&kdrv_ife_ch_buf, 0);
		buf_addr = (UINT32)kdrv_ife_ch_buf.vaddr;
	}
#elif defined (__FREERTOS)
	if (kdrv_ife_ch_buf != NULL) {
		free(kdrv_ife_ch_buf);
		kdrv_ife_ch_buf = NULL;
	}

	kdrv_ife_ch_buf = (UINT32 *)malloc(buf_size * sizeof(UINT32));

	if (kdrv_ife_ch_buf != NULL) {
		buf_addr = (UINT32)kdrv_ife_ch_buf;
	}
#else
	buf_addr = 0;
#endif
	kdrv_ife_buf_init(buf_addr, KDRV_IFE_HANDLE_MAX_NUM);
#endif
}

void kdrv_ife_uninit(void)
{
#if !(defined __UITRON || defined __ECOS)
	ife_platform_release_resource();
#endif

	kdrv_ife_uninstall_id();

#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)
#if defined (__LINUX) || defined (__KERNEL__)
	ret = fmem_release_from_cma(handle_ife_ch_buf, 0);
	handle_ife_ch_buf = NULL;
#elif defined (__FREERTOS)
	free(kdrv_ife_ch_buf);
	kdrv_ife_ch_buf = NULL;
#endif

	kdrv_ife_buf_uninit();

#endif
}

UINT32 kdrv_ife_buf_query(UINT32 channel_num)
{
	UINT32 buffer_size = 0;

	if (channel_num == 0) {
		DBG_ERR("IFE min channel number = 1\r\n");
		kdrv_ife_channel_num =  1;
	} else if (channel_num > KDRV_IPP_MAX_CHANNEL_NUM) {
		DBG_ERR("IFE max channel number = %d\r\n", KDRV_IPP_MAX_CHANNEL_NUM);
		kdrv_ife_channel_num =  KDRV_IPP_MAX_CHANNEL_NUM;
	} else {
		kdrv_ife_channel_num = channel_num;
	}

	kdrv_ife_lock_chls = (1 << kdrv_ife_channel_num) - 1;



	buffer_size += ALIGN_CEIL_64(kdrv_ife_channel_num * sizeof(KDRV_IFE_PRAM));   //g_kdrv_ife_info
	buffer_size += ALIGN_CEIL_64(kdrv_ife_channel_num * sizeof(KDRV_IFE_HANDLE)); //g_kdrv_ife_handle_tab
	buffer_size += ALIGN_CEIL_64(kdrv_ife_channel_num * sizeof(UINT32));          //g_kdrv_ife_ipl_ctrl_en
	buffer_size += ALIGN_CEIL_64(kdrv_ife_channel_num * sizeof(UINT32));          //g_kdrv_ife_iq_ctrl_en
	buffer_size += ALIGN_CEIL_64(kdrv_ife_channel_num * sizeof(KDRV_IFE_VIG_PARAM)); //g_kdrv_ife_vig_setting_info

	return buffer_size;

}

UINT32 kdrv_ife_buf_init(UINT32 input_addr, UINT32 channel_num)
{
//	kdrv_ife_flow_init();
//  return 0;

	UINT32 buffer_size;

	kdrv_ife_flow_init();

	g_kdrv_ife_info = (KDRV_IFE_PRAM *)input_addr;
	buffer_size = ALIGN_CEIL_64(channel_num * sizeof(KDRV_IFE_PRAM));

	g_kdrv_ife_handle_tab = (KDRV_IFE_HANDLE *)(input_addr + buffer_size);
	buffer_size += ALIGN_CEIL_64(channel_num * sizeof(KDRV_IFE_HANDLE));

	g_kdrv_ife_ipl_ctrl_en = (UINT32 *)(input_addr + buffer_size);
	buffer_size += ALIGN_CEIL_64(channel_num * sizeof(UINT32));

	g_kdrv_ife_iq_ctrl_en = (UINT32 *)(input_addr + buffer_size);
	buffer_size += ALIGN_CEIL_64(channel_num * sizeof(UINT32));

	g_kdrv_ife_vig_set_info = (KDRV_IFE_VIG_PARAM *)(input_addr + buffer_size);
	buffer_size += ALIGN_CEIL_64(channel_num * sizeof(KDRV_IFE_VIG_PARAM));
	
	return buffer_size;
}


void kdrv_ife_buf_uninit(void)
{
	g_kdrv_ife_info        = NULL;
	g_kdrv_ife_handle_tab  = NULL;
	g_kdrv_ife_ipl_ctrl_en = NULL;
	g_kdrv_ife_iq_ctrl_en  = NULL;
	g_kdrv_ife_vig_set_info = NULL;

	kdrv_ife_channel_num = 0;
	kdrv_ife_lock_chls = 0;

	if (kdrv_ife_open_cnt == 0) {
		g_kdrv_ife_init_flg = 0;
	}
}

INT32 kdrv_ife_open(UINT32 chip, UINT32 engine)
{
	/*
	ife_drv_obj.FP_IFEISR_CB = kdrv_ife_isr_cb;
	if (ife_open(&ife_drv_obj) != E_OK) {
	    DBG_WRN("KDRV IFE: ife_open failed\r\n");
	    return E_SYS;
	}
	kdrv_ife_init();
	DBG_ERR("IFE KDRV kdrv_ife_open\r\n");
	return E_OK;
	*/
	IFE_OPENOBJ ife_drv_obj;
	KDRV_IFE_OPENCFG kdrv_ife_open_obj;
	//UINT32 eng_init_flag;
	UINT32 ch;

	if (g_kdrv_ife_init_flg == 0) {
		kdrv_ife_handle_alloc(&g_kdrv_ife_init_flg);
	}

	if (g_kdrv_ife_init_flg == FALSE) {
		DBG_WRN("KDRV IFE: no free handle, max handle num = %d\r\n", kdrv_ife_channel_num);
		return E_SYS;
	}


	if (kdrv_ife_open_cnt == 0) {
		if (g_kdrv_ife_init_flg) {
			kdrv_ife_open_obj = g_kdrv_ife_open_cfg;
			ife_drv_obj.ui_ife_clock_sel = kdrv_ife_open_obj.ife_clock_sel;
			ife_drv_obj.FP_IFEISR_CB = kdrv_ife_isr_cb;

			if (ife_open(&ife_drv_obj) != E_OK) {
				kdrv_ife_handle_free();
				DBG_WRN("KDRV IFE: ife_open failed\r\n");
				return E_SYS;
			}
		}
		for (ch = 0; ch < kdrv_ife_channel_num; ch ++) {
			kdrv_ife_lock(&g_kdrv_ife_handle_tab[ch], TRUE);
			// init internal parameter
			kdrv_ife_init_param(ch);
			kdrv_ife_lock(&g_kdrv_ife_handle_tab[ch], FALSE);
		}
	}
	kdrv_ife_open_cnt ++;

	return E_OK;
}

/*
UINT32 kdrv_ife_open_org(void* open_cfg)
{
    IFE_OPENOBJ ife_drv_obj;
    KDRV_IFE_OPENOBJ kdrv_ife_open_obj;
    KDRV_IFE_HANDLE* p_handle;
    UINT32 eng_init_flag;

    p_handle = kdrv_ife_handle_alloc(&eng_init_flag);
    if (p_handle == NULL) {
        DBG_WRN("KDRV IFE: no free handle, max handle num = %d\r\n", KDRV_IFE_HANDLE_MAX_NUM);
        return 0;
    }

    if (eng_init_flag) {
        kdrv_ife_open_obj = *(KDRV_IFE_OPENOBJ *)open_cfg;
        ife_drv_obj.uiIfeClockSel = kdrv_ife_open_obj.ife_clock_sel;
        ife_drv_obj.FP_IFEISR_CB = kdrv_ife_isr_cb;

        if (ife_open(&ife_drv_obj) != E_OK) {
            kdrv_ife_handle_free(p_handle);
            DBG_WRN("KDRV IFE: ife_open failed\r\n");
            return 0;
        }
    }

    kdrv_ife_lock(p_handle, TRUE);

    // init internal parameter
    kdrv_ife_init(p_handle->entry_id);

    kdrv_ife_lock(p_handle, FALSE);

    DBG_ERR("IFE KDRV kdrv_ife_open\r\n");
    return (UINT32)p_handle;
}
EXPORT_SYMBOL(kdrv_ife_open);
*/

INT32 kdrv_ife_close(UINT32 chip, UINT32 engine)
{
	ER rt = E_OK;
	UINT32 channel = 0;
	KDRV_IFE_HANDLE *p_handle;

	for (channel = 0; channel < kdrv_ife_channel_num; channel++) {
		p_handle = &g_kdrv_ife_handle_tab[channel];
		if (kdrv_ife_chk_handle(p_handle) == NULL) {
			DBG_ERR("KDRV IFE: illegal handle 0x%.8x\r\n", (unsigned int)p_handle);
			return E_SYS;
		}
		if ((p_handle->sts & KDRV_IFE_HANDLE_LOCK) != KDRV_IFE_HANDLE_LOCK) {
			DBG_ERR("KDRV IFE: illegal handle sts 0x%.8x\r\n", (unsigned int)p_handle->sts);
			return E_SYS;
		}
	}

	kdrv_ife_open_cnt --;
	if (kdrv_ife_open_cnt == 0) {
		for (channel = 0; channel < kdrv_ife_channel_num; channel++) {
			kdrv_ife_lock(&g_kdrv_ife_handle_tab[channel], TRUE);
		}
		if (kdrv_ife_handle_free()) {
			rt = ife_close();
		}
		for (channel = 0; channel < kdrv_ife_channel_num; channel++) {
			kdrv_ife_lock(&g_kdrv_ife_handle_tab[channel], FALSE);
		}

		g_kdrv_ife_trig_hdl = NULL;
	}

	return rt;
}

/*
ER kdrv_ife_close_org(UINT32 hdl)
{
    ER rt = E_OK;
    KDRV_IFE_HANDLE* p_handle;

    p_handle = (KDRV_IFE_HANDLE *)hdl;
    if (kdrv_ife_chk_handle(p_handle) == NULL) {
        DBG_ERR("KDRV IFE: illegal handle 0x%.8x\r\n", hdl);
        return E_SYS;
    }

    if ((p_handle->sts & KDRV_IFE_HANDLE_LOCK) != KDRV_IFE_HANDLE_LOCK) {
        DBG_ERR("KDRV IFE: illegal handle sts 0x%.8x\r\n", p_handle->sts);
        return E_SYS;
    }

    kdrv_ife_lock(p_handle, TRUE);

    if (kdrv_ife_handle_free(p_handle)) {
        ife_close();
    }

    kdrv_ife_lock(p_handle, FALSE);
    return rt;
}
EXPORT_SYMBOL(kdrv_ife_close);
*/


#if 0
#endif


/*
ER kdrv_ife_trigger_org(UINT32 id, void* data)
{
    ER rt = E_ID;
    KDRV_IFE_HANDLE *p_handle;
    p_handle = kdrv_ife_entry_id_conv2_handle(id);

    kdrv_ife_sem(p_handle, TRUE);

    //set ife kdrv parameters to ife driver
    rt = kdrv_ife_setmode(id);
    if (rt != E_OK) {
        kdrv_ife_sem(p_handle, FALSE);
        return rt;
    }
    //update trig id and trig_cfg
    g_kdrv_ife_trig_hdl = p_handle;
    kdrv_ife_frm_end(p_handle, FALSE);

    //trigger ife start
    rt = ife_start();
    if (rt != E_OK) {
        kdrv_ife_frm_end(p_handle, FALSE);
        return rt;
    }

    DBG_ERR("IFE KDRV kdrv_ife_set_trig\r\n");

    return rt;
}
*/
INT32 kdrv_ife_pause(UINT32 chip, UINT32 engine)
{
	ER rt = E_OK;

	rt = ife_pause();
	if (rt != E_OK) {
		return rt;
	}

	return rt;
}

INT32 kdrv_ife_trigger(UINT32 id, KDRV_IFE_TRIGGER_PARAM *p_ife_param, KDRV_CALLBACK_FUNC *p_cb_func, void *p_user_data)
{
	ER rt = E_ID;
	KDRV_IFE_HANDLE *p_handle;
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);

	p_handle = kdrv_ife_entry_id_conv2_handle(channel);
	kdrv_ife_sem(p_handle, TRUE);

	//set ife kdrv parameters to ife driver
	if (kdrv_ife_setmode(channel) != E_OK) {
		kdrv_ife_sem(p_handle, FALSE);
		return rt;
	}
	//update trig id and trig_cfg
	g_kdrv_ife_trig_hdl = p_handle;
	kdrv_ife_frm_end(p_handle, FALSE);

	//trigger ife start
	rt = ife_start();
	if (rt != E_OK) {
		kdrv_ife_frm_end(p_handle, FALSE);
		return rt;
	}

	//DBG_ERR("IFE KDRV kdrv_ife_set_trig\r\n");
	return rt;
}

/*
static ER kdrv_ife_set_trig(UINT32 id, void* data)
{
    ER rt = E_ID;
    KDRV_IFE_HANDLE *p_handle;
    p_handle = kdrv_ife_entry_id_conv2_handle(id);

    kdrv_ife_sem(p_handle, TRUE);

    //set ife kdrv parameters to ife driver
    rt = kdrv_ife_setmode(id);
    if (rt != E_OK) {
        kdrv_ife_sem(p_handle, FALSE);
        return rt;
    }
    //update trig id and trig_cfg
    g_kdrv_ife_trig_hdl = p_handle;
    kdrv_ife_frm_end(p_handle, FALSE);

    //trigger ife start
    rt = ife_start();
    if (rt != E_OK) {
        kdrv_ife_frm_end(p_handle, FALSE);
        return rt;
    }

    DBG_ERR("IFE KDRV kdrv_ife_set_trig\r\n");

    return rt;
}
*/

static ER kdrv_ife_set_reset(UINT32 id, void *data)
{
	KDRV_IFE_HANDLE *p_handle;

	if (ife_reset() != E_OK) {
		DBG_ERR("KDRV IFE: engine reset error...\r\n");
		return E_PAR;
	}

	p_handle = kdrv_ife_entry_id_conv2_handle(id);
	kdrv_ife_sem(p_handle, FALSE);

	return E_OK;
}

static ER kdrv_ife_set_builtin_flow_disable(UINT32 id, void *data)
{
	ife_set_builtin_flow_disable();

	return E_OK;
}


static ER kdrv_ife_set_in_addr_1(UINT32 id, void *data)
{
	KDRV_IFE_IN_ADDR *ptr = (KDRV_IFE_IN_ADDR *)data;
	UINT32 ife_lock_flag;
	
	if(data == NULL){
		DBG_ERR("id %d ife set in_addr_1 error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;		
	}

	if ((ptr->ife_in_addr_1) & 0x3) {
		DBG_ERR("IFE KDRV input address must be the multiple of 4\r\n");
		return E_SYS;
	}
	ife_lock_flag = ife_platform_spin_lock();
	
	g_kdrv_ife_info[id].in_pixel_addr_1 = ptr->ife_in_addr_1;
	g_ife_update_flag[id][KDRV_IFE_PARAM_IPL_IN_ADDR_1] = TRUE;
	
	ife_platform_spin_unlock(ife_lock_flag);

	return E_OK;
}

static ER kdrv_ife_set_in_addr_2(UINT32 id, void *data)
{
	KDRV_IFE_IN_ADDR *ptr = (KDRV_IFE_IN_ADDR *)data;
	UINT32 ife_lock_flag;
	
	if(data == NULL){
		DBG_ERR("id %d ife set in_addr_2 error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;		
	}	
	if ((ptr->ife_in_addr_2) & 0x3) {
		DBG_ERR("IFE KDRV input address must be the multiple of 4\r\n");
		return E_SYS;
	}

	ife_lock_flag = ife_platform_spin_lock();

	g_kdrv_ife_info[id].in_pixel_addr_2 = ptr->ife_in_addr_2;
	g_ife_update_flag[id][KDRV_IFE_PARAM_IPL_IN_ADDR_2] = TRUE;
	
	ife_platform_spin_unlock(ife_lock_flag);

	return E_OK;
}

static ER kdrv_ife_set_in_ofs_1(UINT32 id, void *data)
{
	KDRV_IFE_IN_OFS *ptr = (KDRV_IFE_IN_OFS *)data;
	UINT32 ife_lock_flag;
	
	if(data == NULL){
		DBG_ERR("id %d ife set in_ofs_1 error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;		
	}	
	ife_lock_flag = ife_platform_spin_lock();

	g_kdrv_ife_info[id].line_ofs_1 = ptr->line_ofs_1;
	g_ife_update_flag[id][KDRV_IFE_PARAM_IPL_IN_OFS_1] = TRUE;
	
	ife_platform_spin_unlock(ife_lock_flag);

	return E_OK;
}

static ER kdrv_ife_set_in_ofs_2(UINT32 id, void *data)
{
	KDRV_IFE_IN_OFS *ptr = (KDRV_IFE_IN_OFS *)data;
	UINT32 ife_lock_flag;
	
	if(data == NULL){
		DBG_ERR("id %d ife set in_ofs_2 error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;		
	}	
	
	ife_lock_flag = ife_platform_spin_lock();

	g_kdrv_ife_info[id].line_ofs_2 = ptr->line_ofs_2;
	g_ife_update_flag[id][KDRV_IFE_PARAM_IPL_IN_OFS_2] = TRUE;

	ife_platform_spin_unlock(ife_lock_flag);

	return E_OK;
}

static ER kdrv_ife_set_in_img_info(UINT32 id, void *data)
{
	KDRV_IFE_IN_IMG_INFO *ptr;
	UINT32 ife_lock_flag;


	if(data == NULL){
		DBG_ERR("id %d ife set in_img_info error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;		
	}

	ptr = (KDRV_IFE_IN_IMG_INFO *)data;

	ife_lock_flag = ife_platform_spin_lock();

	g_kdrv_ife_info[id].in_img_info.in_src               = ptr->in_src;
	g_kdrv_ife_info[id].in_img_info.type                 = ptr->type;
	g_kdrv_ife_info[id].in_img_info.img_info.width       = ptr->img_info.width;
	g_kdrv_ife_info[id].in_img_info.img_info.height      = ptr->img_info.height;
	g_kdrv_ife_info[id].in_img_info.img_info.crop_width  = ptr->img_info.crop_width;
	g_kdrv_ife_info[id].in_img_info.img_info.crop_height = ptr->img_info.crop_height;
	g_kdrv_ife_info[id].in_img_info.img_info.crop_hpos   = ptr->img_info.crop_hpos;
	g_kdrv_ife_info[id].in_img_info.img_info.crop_vpos   = ptr->img_info.crop_vpos;

	//g_kdrv_ife_info[id].in_img_info.img_info.stripe_hm   = ptr->img_info.stripe_hm;
	//g_kdrv_ife_info[id].in_img_info.img_info.stripe_hl   = ptr->img_info.stripe_hl;
	//g_kdrv_ife_info[id].in_img_info.img_info.stripe_hn   = ptr->img_info.stripe_hn;
	//g_kdrv_ife_info[id].in_img_info.img_info.h_shift     = ptr->img_info.h_shift;

	//g_kdrv_ife_info[id].in_img_info.img_info.cfa_pat     = ptr->img_info.cfa_pat;
	g_kdrv_ife_info[id].in_img_info.img_info.st_pix      = ptr->img_info.st_pix;

	switch (ptr->img_info.bit) {
	case KDRV_IPP_RAW_BIT_8:
		g_kdrv_ife_info[id].in_img_info.img_info.bit = KDRV_IFE_RAW_BIT_8;
		break;
	case KDRV_IPP_RAW_BIT_10:
		g_kdrv_ife_info[id].in_img_info.img_info.bit = KDRV_IFE_RAW_BIT_10;
		break;
	case KDRV_IPP_RAW_BIT_12:
		g_kdrv_ife_info[id].in_img_info.img_info.bit = KDRV_IFE_RAW_BIT_12;
		break;
	case KDRV_IPP_RAW_BIT_16:
		g_kdrv_ife_info[id].in_img_info.img_info.bit = KDRV_IFE_RAW_BIT_16;
		break;
	default:
		DBG_ERR("IFE: unknown input bit %d\r\n", (unsigned int)ptr->img_info.bit);
		g_kdrv_ife_info[id].in_img_info.img_info.bit = KDRV_IFE_RAW_BIT_8;
		break;
	}

	g_ife_update_flag[id][KDRV_IFE_PARAM_IPL_IN_IMG] = TRUE;

	ife_platform_spin_unlock(ife_lock_flag);

	return E_OK;
}

static ER kdrv_ife_set_out_addr(UINT32 id, void *data)
{
	KDRV_IFE_OUT_ADDR *ptr = (KDRV_IFE_OUT_ADDR *)data;
	UINT32 ife_lock_flag;
	
	if(data == NULL){
		DBG_ERR("id %d ife set out_addr error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;		
	}		
	if ((ptr->ife_out_addr) & 0x3) {
		DBG_ERR("IFE KDRV output address must be the multiple of 4\r\n");
		return E_SYS;
	}
	ife_lock_flag = ife_platform_spin_lock();

	g_kdrv_ife_info[id].out_pixel_addr = ptr->ife_out_addr;
	g_ife_update_flag[id][KDRV_IFE_PARAM_IPL_OUT_ADDR] = TRUE;

	ife_platform_spin_unlock(ife_lock_flag);	

	return E_OK;
}

static ER kdrv_ife_set_out_img_info(UINT32 id, void *data)
{
	KDRV_IFE_OUT_IMG_INFO *ptr = (KDRV_IFE_OUT_IMG_INFO *)data;
	UINT32 ife_lock_flag;
	
	if(data == NULL){
		DBG_ERR("id %d ife set out_img_info error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;		
	}	

	ife_lock_flag = ife_platform_spin_lock();

	g_kdrv_ife_info[id].out_img_info.line_ofs = ptr->line_ofs;

	switch (ptr->bit) {
	case KDRV_IPP_RAW_BIT_8:
		g_kdrv_ife_info[id].out_img_info.bit = KDRV_IFE_RAW_BIT_8;
		break;
	case KDRV_IPP_RAW_BIT_12:
		g_kdrv_ife_info[id].out_img_info.bit = KDRV_IFE_RAW_BIT_12;
		break;
	case KDRV_IPP_RAW_BIT_16:
		g_kdrv_ife_info[id].out_img_info.bit = KDRV_IFE_RAW_BIT_16;
		break;
	default:
		DBG_ERR("IFE: unknown output bit %d\r\n", (unsigned int)ptr->bit);
		g_kdrv_ife_info[id].out_img_info.bit = KDRV_IFE_RAW_BIT_8;
		break;
	}

	g_ife_update_flag[id][KDRV_IFE_PARAM_IPL_OUT_IMG] = TRUE;
	
	ife_platform_spin_unlock(ife_lock_flag);

	return E_OK;
}

static ER kdrv_ife_set_ring_buf_info(UINT32 id, void *data)
{
	KDRV_IFE_RING_BUF_INFO *ptr = (KDRV_IFE_RING_BUF_INFO *)data;
	UINT32 ife_lock_flag;
	
	if(data == NULL){
		DBG_ERR("id %d ife set ring_buf_info error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;		
	}

	ife_lock_flag = ife_platform_spin_lock();

	g_kdrv_ife_info[id].info_ring_buf.dmaloop_en = ptr->dmaloop_en;
	if (ptr->dmaloop_line == 0 && ptr->dmaloop_en == 1) {
		DBG_ERR("IFE ring buffer: if dmaloop_en enable, dmaloop_line can not be 0 !!\r\n");
	}
	g_kdrv_ife_info[id].info_ring_buf.dmaloop_line = ptr->dmaloop_line;
	g_ife_update_flag[id][KDRV_IFE_PARAM_IPL_RINGBUF_INFO] = TRUE;

	ife_platform_spin_unlock(ife_lock_flag);

	return E_OK;
}


static ER kdrv_ife_set_dma_wait_sie2_st_disable(UINT32 id, void *data)
{
	KDRV_IFE_WAIT_SIE2_DISABLE_INFO *ptr = (KDRV_IFE_WAIT_SIE2_DISABLE_INFO *)data;
	UINT32 ife_lock_flag;
	
	if(data == NULL){
		DBG_ERR("id %d ife set dma_wait_sie2_start_disable error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;		
	}

	ife_lock_flag = ife_platform_spin_lock();

	g_kdrv_ife_info[id].info_wait_sie2.dma_wait_sie2_start_disable = ptr->dma_wait_sie2_start_disable;

	g_ife_update_flag[id][KDRV_IFE_PARAM_IPL_WAIT_SIE2_DISABLE] = TRUE;

	ife_platform_spin_unlock(ife_lock_flag);

	return E_OK;
}



static ER kdrv_ife_set_isrcb(UINT32 id, void *data)
{
	KDRV_IFE_HANDLE *p_handle = kdrv_ife_entry_id_conv2_handle(id);
	p_handle->isrcb_fp = (KDRV_IPP_ISRCB)data;

	return E_OK;
}

static ER kdrv_ife_set_inte_en(UINT32 id, void *data)
{
	UINT32 ife_lock_flag;	
	if(data == NULL){
		DBG_ERR("id %d ife set inte_en error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;		
	}

	ife_lock_flag = ife_platform_spin_lock();

	g_kdrv_ife_info[id].inte_en = *(KDRV_IFE_INTE *)data;
	g_ife_update_flag[id][KDRV_IFE_PARAM_IPL_INTER] = TRUE;

	ife_platform_spin_unlock(ife_lock_flag);

	return E_OK;
}

static ER kdrv_ife_set_rde_info(UINT32 id, void *data)
{
	KDRV_IFE_RDE_INFO *ptr = (KDRV_IFE_RDE_INFO *)data;
	UINT32 ife_lock_flag;
	
	if(data == NULL){
		DBG_ERR("id %d ife set rde_info error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;		
	}

	ife_lock_flag = ife_platform_spin_lock();

	g_kdrv_ife_info[id].info_rde.enable          = ptr->enable;
	if (ptr->enable) {
		g_kdrv_ife_info[id].info_rde.encode_rate = ptr->encode_rate;
	} else {
		g_kdrv_ife_info[id].info_rde.encode_rate = KDRV_IFE_ENCODE_RATE_50;
	}
	g_ife_update_flag[id][KDRV_IFE_PARAM_IPL_RDE_INFO] = TRUE;
	
	ife_platform_spin_unlock(ife_lock_flag);

	return E_OK;
}


static ER kdrv_ife_set_nrs_order_param(UINT32 id, void *data)
{
	KDRV_IFE_ORDER_PARAM *ptr = (KDRV_IFE_ORDER_PARAM *)data;
	UINT32 i;

	g_kdrv_ife_info[id].par_nrs.order.enable    = ptr->enable;

	if (g_kdrv_ife_info[id].par_nrs.order.enable) {
		g_kdrv_ife_info[id].par_nrs.order.rng_bri   = ptr->rng_bri;
		g_kdrv_ife_info[id].par_nrs.order.rng_dark  = ptr->rng_dark;
		g_kdrv_ife_info[id].par_nrs.order.diff_th   = ptr->diff_th;
		for (i = 0; i < KDRV_NRS_ORD_W_LEN; i++) {
			g_kdrv_ife_info[id].par_nrs.order.bri_w[i]  = ptr->bri_w[i];
			g_kdrv_ife_info[id].par_nrs.order.dark_w[i] = ptr->dark_w[i];
		}
	}

	return E_OK;
}

static ER kdrv_ife_set_nrs_bilat_param(UINT32 id, void *data)
{
	KDRV_IFE_BILAT_PARAM *ptr = (KDRV_IFE_BILAT_PARAM *)data;
	UINT32 i;

	g_kdrv_ife_info[id].par_nrs.bilat.b_enable   = ptr->b_enable;
	if (g_kdrv_ife_info[id].par_nrs.bilat.b_enable) {
		g_kdrv_ife_info[id].par_nrs.bilat.b_str      = ptr->b_str;
		for (i = 0; i < KDRV_NRS_B_OFS_LEN; i++) {
			g_kdrv_ife_info[id].par_nrs.bilat.b_ofs[i] = ptr->b_ofs[i];
		}
		for (i = 0; i < KDRV_NRS_B_W_LEN; i++) {
			g_kdrv_ife_info[id].par_nrs.bilat.b_weight[i] = ptr->b_weight[i];
		}
		for (i = 0; i < KDRV_NRS_B_RANGE_LEN; i++) {
			g_kdrv_ife_info[id].par_nrs.bilat.b_rng[i] = ptr->b_rng[i];
		}
		for (i = 0; i < KDRV_NRS_B_TH_LEN; i++) {
			g_kdrv_ife_info[id].par_nrs.bilat.b_th[i]  = ptr->b_th[i];
		}
		/*
		for (i = 0; i < KDRV_NRS_B_W_LEN; i++) {
		    g_kdrv_ife_info[id].par_nrs.bilat.b_w[i]  = ptr->b_w[i];
		}*/
	}

	g_kdrv_ife_info[id].par_nrs.bilat.gb_enable   = ptr->gb_enable;
	if (g_kdrv_ife_info[id].par_nrs.bilat.gb_enable) {
		g_kdrv_ife_info[id].par_nrs.bilat.gb_str      = ptr->gb_str;
		g_kdrv_ife_info[id].par_nrs.bilat.gb_blend_w  = ptr->gb_blend_w;
		for (i = 0; i < KDRV_NRS_GB_OFS_LEN; i++) {
			g_kdrv_ife_info[id].par_nrs.bilat.gb_ofs[i] = ptr->gb_ofs[i];
		}
		for (i = 0; i < KDRV_NRS_GB_W_LEN; i++) {
			g_kdrv_ife_info[id].par_nrs.bilat.gb_weight[i] = ptr->gb_weight[i];
		}
		for (i = 0; i < KDRV_NRS_GB_RANGE_LEN; i++) {
			g_kdrv_ife_info[id].par_nrs.bilat.gb_rng[i] = ptr->gb_rng[i];
		}
		for (i = 0; i < KDRV_NRS_GB_TH_LEN; i++) {
			g_kdrv_ife_info[id].par_nrs.bilat.gb_th[i]  = ptr->gb_th[i];
		}
		/*
		        for (i = 0; i < KDRV_NRS_GB_W_LEN; i++) {
		            g_kdrv_ife_info[id].par_nrs.bilat.gb_weight[i]   = ptr->gb_weight[i];
		        }
		        */
	}

	return E_OK;
}

static ER kdrv_ife_set_nrs_param(UINT32 id, void *data)
{
	KDRV_IFE_NRS_PARAM *ptr;
	UINT32 ife_lock_flag;


	if(data == NULL){
		DBG_ERR("id %d ife set nrs_param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;		
	}

	ptr = (KDRV_IFE_NRS_PARAM *)data;
	
	ife_lock_flag = ife_platform_spin_lock();

	//g_kdrv_ife_info[id].par_nrs.enable          = ptr->enable;
	if(ptr->enable == TRUE){
		g_kdrv_ife_iq_ctrl_en[id] |= KDRV_IFE_IQ_FUNC_NRS;
	}else{
		g_kdrv_ife_iq_ctrl_en[id] &= (~KDRV_IFE_IQ_FUNC_NRS);
	}	

	if (ptr->enable == TRUE) {
		kdrv_ife_set_nrs_order_param(id, &ptr->order);
		kdrv_ife_set_nrs_bilat_param(id, &ptr->bilat);
	}
	g_ife_update_flag[id][KDRV_IFE_PARAM_IQ_NRS] = TRUE;

	ife_platform_spin_unlock(ife_lock_flag);
	
	return E_OK;
}

static ER kdrv_ife_set_fusion_fcgain_param(UINT32 id, void *data)
{
	KDRV_IFE_FCGAIN *ptr = (KDRV_IFE_FCGAIN *)data;

	g_kdrv_ife_info[id].par_fusion.fu_cgain.enable         = ptr->enable;

	if (g_kdrv_ife_info[id].par_fusion.fu_cgain.enable) {
		g_kdrv_ife_info[id].par_fusion.fu_cgain.bit_field  = ptr->bit_field;

		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path0_r   = ptr->fcgain_path0_r;
		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path0_gr  = ptr->fcgain_path0_gr;
		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path0_gb  = ptr->fcgain_path0_gb;
		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path0_b   = ptr->fcgain_path0_b;
		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path0_ir  = ptr->fcgain_path0_ir;

		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path1_r   = ptr->fcgain_path1_r;
		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path1_gr  = ptr->fcgain_path1_gr;
		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path1_gb  = ptr->fcgain_path1_gb;
		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path1_b   = ptr->fcgain_path1_b;
		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path1_ir  = ptr->fcgain_path1_ir;

		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path0_r    = ptr->fcofs_path0_r;
		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path0_gr   = ptr->fcofs_path0_gr;
		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path0_gb   = ptr->fcofs_path0_gb;
		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path0_b    = ptr->fcofs_path0_b;
		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path0_ir   = ptr->fcofs_path0_ir;

		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path1_r    = ptr->fcofs_path1_r;
		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path1_gr   = ptr->fcofs_path1_gr;
		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path1_gb   = ptr->fcofs_path1_gb;
		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path1_b    = ptr->fcofs_path1_b;
		g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path1_ir   = ptr->fcofs_path1_ir;
	}

	return E_OK;
}

static ER kdrv_ife_set_fusion_ctrl_param(UINT32 id, void *data)
{
	KDRV_IFE_FUSION_CTRL *ptr = (KDRV_IFE_FUSION_CTRL *)data;

	g_kdrv_ife_info[id].par_fusion.fu_ctrl.y_mean_sel = ptr->y_mean_sel;
	g_kdrv_ife_info[id].par_fusion.fu_ctrl.mode       = ptr->mode;
	g_kdrv_ife_info[id].par_fusion.fu_ctrl.ev_ratio   = ptr->ev_ratio;

	return E_OK;
}

static ER kdrv_ife_set_fusion_bld_cur_param(UINT32 id, void *data)
{
	KDRV_IFE_BLEND_CURVE *ptr = (KDRV_IFE_BLEND_CURVE *)data;
	UINT32 i;

	g_kdrv_ife_info[id].par_fusion.bld_cur.nor_sel  = ptr->nor_sel;
	g_kdrv_ife_info[id].par_fusion.bld_cur.dif_sel  = ptr->dif_sel;

	for (i = 0; i < FUSION_BLD_CUR_KNEE_NUM; i++) {
		g_kdrv_ife_info[id].par_fusion.bld_cur.l_nor_knee[i] = ptr->l_nor_knee[i];
		g_kdrv_ife_info[id].par_fusion.bld_cur.s_nor_knee[i] = ptr->s_nor_knee[i];
		g_kdrv_ife_info[id].par_fusion.bld_cur.l_dif_knee[i] = ptr->l_dif_knee[i];
		g_kdrv_ife_info[id].par_fusion.bld_cur.s_dif_knee[i] = ptr->s_dif_knee[i];
	}

	g_kdrv_ife_info[id].par_fusion.bld_cur.l_nor_range    = ptr->l_nor_range;
	g_kdrv_ife_info[id].par_fusion.bld_cur.l_nor_w_edge   = ptr->l_nor_w_edge;

	g_kdrv_ife_info[id].par_fusion.bld_cur.s_nor_range    = ptr->s_nor_range;
	g_kdrv_ife_info[id].par_fusion.bld_cur.s_nor_w_edge   = ptr->s_nor_w_edge;

	g_kdrv_ife_info[id].par_fusion.bld_cur.l_dif_range    = ptr->l_dif_range;
	g_kdrv_ife_info[id].par_fusion.bld_cur.l_dif_w_edge   = ptr->l_dif_w_edge;

	g_kdrv_ife_info[id].par_fusion.bld_cur.s_dif_range    = ptr->s_dif_range;
	g_kdrv_ife_info[id].par_fusion.bld_cur.s_dif_w_edge   = ptr->s_dif_w_edge;

	return E_OK;
}

static ER kdrv_ife_set_fusion_mc_param(UINT32 id, void *data)
{
	KDRV_IFE_MC *ptr = (KDRV_IFE_MC *)data;
	UINT32 i;

	g_kdrv_ife_info[id].par_fusion.mc_para.dwd = ptr->dwd;
	g_kdrv_ife_info[id].par_fusion.mc_para.diff_ratio = ptr->diff_ratio;
	g_kdrv_ife_info[id].par_fusion.mc_para.lum_th = ptr->lum_th;

	for (i = 0; i < FUSION_MC_DIFF_W_NUM; i++) {
		g_kdrv_ife_info[id].par_fusion.mc_para.neg_diff_w[i] = ptr->neg_diff_w[i];
		g_kdrv_ife_info[id].par_fusion.mc_para.pos_diff_w[i] = ptr->pos_diff_w[i];
	}

	return E_OK;
}

static ER kdrv_ife_set_fusion_dark_sat_param(UINT32 id, void *data)
{
	KDRV_IFE_DARK_SAT *ptr = (KDRV_IFE_DARK_SAT *)data;
	UINT32 i;

	for (i = 0; i < FUSION_DARK_SAT_NUM; i++) {
		g_kdrv_ife_info[id].par_fusion.dk_sat.th[i]        = ptr->th[i];
		g_kdrv_ife_info[id].par_fusion.dk_sat.step[i]      = ptr->step[i];
		g_kdrv_ife_info[id].par_fusion.dk_sat.low_bound[i] = ptr->low_bound[i];
	}

	return E_OK;
}

static ER kdrv_ife_set_fusion_s_comp_param(UINT32 id, void *data)
{
	KDRV_IFE_S_COMP *ptr = (KDRV_IFE_S_COMP *)data;
	UINT32 i;

	g_kdrv_ife_info[id].par_fusion.s_comp.enable           = ptr->enable;

	for (i = 0; i < FUSION_SHORT_COMP_KNEE_NUM; i++) {
		g_kdrv_ife_info[id].par_fusion.s_comp.knee[i]      = ptr->knee[i];
	}
	for (i = 0; i < FUSION_SHORT_COMP_SUB_NUM; i++) {
		g_kdrv_ife_info[id].par_fusion.s_comp.sub_point[i] = ptr->sub_point[i];
	}
	for (i = 0; i < FUSION_SHORT_COMP_SHIFT_NUM; i++) {
		g_kdrv_ife_info[id].par_fusion.s_comp.shift[i]     = ptr->shift[i];
	}

	return E_OK;
}

static ER kdrv_ife_set_fusion_info(UINT32 id, void *data)
{
	KDRV_IFE_FUSION_INFO *ptr = (KDRV_IFE_FUSION_INFO *)data;
	UINT32 ife_lock_flag;

	if(data == NULL){
		DBG_ERR("id %d ife set fusion_info error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;		
	}

	ife_lock_flag = ife_platform_spin_lock();

	g_kdrv_ife_info[id].info_fusion.enable   = ptr->enable;
	if (ptr->enable) {
		if(ptr->fnum == 0){
			DBG_ERR("KDRV IFE: fusion enable but fnum is 0 !!\r\n");
		}
		g_kdrv_ife_info[id].info_fusion.fnum = ptr->fnum;
	}else{
		if(ptr->fnum == 1){
			DBG_ERR("KDRV IFE: fusion disable but fnum is 1 !!\r\n");
		}
		g_kdrv_ife_info[id].info_fusion.fnum = ptr->fnum;
	}
	g_ife_update_flag[id][KDRV_IFE_PARAM_IPL_FUSION_INFO] = TRUE;

	ife_platform_spin_unlock(ife_lock_flag);

	return E_OK;
}

static ER kdrv_ife_set_fusion_param(UINT32 id, void *data)
{
	KDRV_IFE_FUSION_PARAM *ptr;
	UINT32 ife_lock_flag;

	if(data == NULL){
		DBG_ERR("id %d ife set fusion_param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;		
	}

	ptr = (KDRV_IFE_FUSION_PARAM *)data;

	ife_lock_flag = ife_platform_spin_lock();
	
	kdrv_ife_set_fusion_ctrl_param(id, &ptr->fu_ctrl);
	kdrv_ife_set_fusion_bld_cur_param(id, &ptr->bld_cur);
	kdrv_ife_set_fusion_mc_param(id, &ptr->mc_para);
	kdrv_ife_set_fusion_dark_sat_param(id, &ptr->dk_sat);
	kdrv_ife_set_fusion_s_comp_param(id, &ptr->s_comp);
	kdrv_ife_set_fusion_fcgain_param(id, &ptr->fu_cgain);

	g_ife_update_flag[id][KDRV_IFE_PARAM_IQ_FUSION] = TRUE;

	ife_platform_spin_unlock(ife_lock_flag);

	return E_OK;
}

static ER kdrv_ife_set_fcurve_ctrl_param(UINT32 id, void *data)
{
	KDRV_IFE_FCURVE_CTRL *ptr = (KDRV_IFE_FCURVE_CTRL *)data;

	g_kdrv_ife_info[id].par_fcurve.fcur_ctrl.yv_w        = ptr->yv_w;
	g_kdrv_ife_info[id].par_fcurve.fcur_ctrl.y_mean_sel  = ptr->y_mean_sel;

	return E_OK;
}

static ER kdrv_ife_set_fcurve_yw_param(UINT32 id, void *data)
{
	KDRV_IFE_Y_W *ptr = (KDRV_IFE_Y_W *)data;
	UINT32 i;

	for (i = 0; i < FCURVE_Y_W_NUM; i++) {
		g_kdrv_ife_info[id].par_fcurve.y_weight.y_w_lut[i] = ptr->y_w_lut[i];
	}

	return E_OK;
}

static ER kdrv_ife_set_fcurve_idx_param(UINT32 id, void *data)
{
	KDRV_IFE_IDX_LUT *ptr = (KDRV_IFE_IDX_LUT *)data;
	UINT32 i;

	for (i = 0; i < FCURVE_IDX_NUM; i++) {
		g_kdrv_ife_info[id].par_fcurve.index.idx_lut[i] = ptr->idx_lut[i];
	}

	return E_OK;
}

static ER kdrv_ife_set_fcurve_split_param(UINT32 id, void *data)
{
	KDRV_IFE_SPLIT_LUT *ptr = (KDRV_IFE_SPLIT_LUT *)data;
	UINT32 i;

	for (i = 0; i < FCURVE_SPLIT_NUM; i++) {
		g_kdrv_ife_info[id].par_fcurve.split.split_lut[i] = ptr->split_lut[i];
	}

	return E_OK;
}

static ER kdrv_ife_set_fcurve_val_param(UINT32 id, void *data)
{
	KDRV_IFE_VAL_LUT *ptr = (KDRV_IFE_VAL_LUT *)data;
	UINT32 i;

	for (i = 0; i < FCURVE_VAL_NUM; i++) {
		g_kdrv_ife_info[id].par_fcurve.value.val_lut[i] = ptr->val_lut[i];
	}

	return E_OK;
}

static ER kdrv_ife_set_fcurve_param(UINT32 id, void *data)
{
	KDRV_IFE_FCURVE_PARAM *ptr;
	UINT32 ife_lock_flag;

	if(data == NULL){
		DBG_ERR("id %d ife set fcurve_param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	ptr = (KDRV_IFE_FCURVE_PARAM *)data;

	ife_lock_flag = ife_platform_spin_lock();
	//g_kdrv_ife_info[id].par_fcurve.enable     = ptr->enable;
	if(ptr->enable == TRUE){
		g_kdrv_ife_iq_ctrl_en[id] |= KDRV_IFE_IQ_FUNC_FCURVE;
	}else{
		g_kdrv_ife_iq_ctrl_en[id] &= (~KDRV_IFE_IQ_FUNC_FCURVE);
	}	
	if (ptr->enable == TRUE) {
		kdrv_ife_set_fcurve_ctrl_param(id, &ptr->fcur_ctrl);
		kdrv_ife_set_fcurve_yw_param(id, &ptr->y_weight);
		kdrv_ife_set_fcurve_idx_param(id, &ptr->index);
		kdrv_ife_set_fcurve_split_param(id, &ptr->split);
		kdrv_ife_set_fcurve_val_param(id, &ptr->value);
	}
	g_ife_update_flag[id][KDRV_IFE_PARAM_IQ_FCURVE] = TRUE;

	ife_platform_spin_unlock(ife_lock_flag);
	return E_OK;
}

static ER kdrv_ife_set_mirror(UINT32 id, void *data)
{
	KDRV_IFE_MIRROR *ptr = (KDRV_IFE_MIRROR *)data;
	UINT32 ife_lock_flag;
	
	if(data == NULL){
		DBG_ERR("id %d ife set mirror error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}	
	ife_lock_flag = ife_platform_spin_lock();

	g_kdrv_ife_info[id].par_mirror.enable     = ptr->enable;
	g_ife_update_flag[id][KDRV_IFE_PARAM_IPL_MIRROR] = TRUE;
	
	ife_platform_spin_unlock(ife_lock_flag);

	return E_OK;
}

static ER kdrv_ife_set_outl_param(UINT32 id, void *data)
{
	KDRV_IFE_OUTL_PARAM *ptr;
	UINT32 i;
	UINT32 ife_lock_flag;

	if(data == NULL){
		DBG_ERR("id %d ife set outl_param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	ptr = (KDRV_IFE_OUTL_PARAM *)data;

	ife_lock_flag = ife_platform_spin_lock();

	if(ptr->enable == TRUE){
		//g_kdrv_ife_info[id].par_outl.enable              = ptr->enable;
		g_kdrv_ife_iq_ctrl_en[id] |= KDRV_IFE_IQ_FUNC_OUTL;
	}else{
		g_kdrv_ife_iq_ctrl_en[id] &= (~KDRV_IFE_IQ_FUNC_OUTL);	
	}
	
	if (ptr->enable == TRUE) {
		g_kdrv_ife_info[id].par_outl.outl_weight     = ptr->outl_weight;
		g_kdrv_ife_info[id].par_outl.dark_ofs        = ptr->dark_ofs;
		g_kdrv_ife_info[id].par_outl.bright_ofs      = ptr->bright_ofs;
		for (i = 0; i < KDRV_OUTL_TH_LEN; i++) {
			g_kdrv_ife_info[id].par_outl.bright_th[i] = ptr->bright_th[i];
			g_kdrv_ife_info[id].par_outl.dark_th[i]   = ptr->dark_th  [i];
		}
		g_kdrv_ife_info[id].par_outl.outl_cnt[0]    = ptr->outl_cnt[0];
		g_kdrv_ife_info[id].par_outl.outl_cnt[1]    = ptr->outl_cnt[1];

		g_kdrv_ife_info[id].par_outl.avg_mode       = ptr->avg_mode;

		g_kdrv_ife_info[id].par_outl.ord_rng_bri    = ptr->ord_rng_bri;
		g_kdrv_ife_info[id].par_outl.ord_rng_dark   = ptr->ord_rng_dark;
		g_kdrv_ife_info[id].par_outl.ord_protect_th = ptr->ord_protect_th;
#if NAME_ERR
		g_kdrv_ife_info[id].par_outl.ord_blend_w   = ptr->ord_blend_w;
#else
		g_kdrv_ife_info[id].par_outl.ord_bleand_w   = ptr->ord_bleand_w;
#endif

		for (i = 0; i < KDRV_OUTL_ORD_W_LEN; i++) {
			g_kdrv_ife_info[id].par_outl.ord_bri_w[i]  = ptr->ord_bri_w[i];
			g_kdrv_ife_info[id].par_outl.ord_dark_w[i] = ptr->ord_dark_w[i];
		}
	}
	g_ife_update_flag[id][KDRV_IFE_PARAM_IQ_OUTL] = TRUE;

	ife_platform_spin_unlock(ife_lock_flag);

	return E_OK;
}

static ER kdrv_ife_set_filter_spatial_param(UINT32 id, void *data)
{
	KDRV_IFE_SPATIAL *ptr = (KDRV_IFE_SPATIAL *)data;
	UINT32 i;

	for (i = 0; i < KDRV_SPATIAL_W_LEN; i++) {
		g_kdrv_ife_info[id].par_filter.spatial.weight[i] = ptr->weight[i];
	}

	return E_OK;
}

static ER kdrv_ife_set_filter_range_ch_r_param(UINT32 id, void *data)
{
	KDRV_IFE_RANGE_FILTER_R *ptr = (KDRV_IFE_RANGE_FILTER_R *)data;
	UINT32 i;

	for (i = 0; i < KDRV_RNG_TH_LEN; i++) {
		g_kdrv_ife_info[id].par_filter.rng_filt_r.a_th[i] = ptr->a_th[i];
		g_kdrv_ife_info[id].par_filter.rng_filt_r.b_th[i] = ptr->b_th[i];
	}
	for (i = 0; i < KDRV_RNG_LUT_LEN; i++) {
		g_kdrv_ife_info[id].par_filter.rng_filt_r.a_lut[i] = ptr->a_lut[i];
		g_kdrv_ife_info[id].par_filter.rng_filt_r.b_lut[i] = ptr->b_lut[i];
	}

	return E_OK;
}

static ER kdrv_ife_set_filter_range_ch_gr_param(UINT32 id, void *data)
{
	KDRV_IFE_RANGE_FILTER_GR *ptr = (KDRV_IFE_RANGE_FILTER_GR *)data;
	UINT32 i;

	for (i = 0; i < KDRV_RNG_TH_LEN; i++) {
		g_kdrv_ife_info[id].par_filter.rng_filt_gr.a_th[i] = ptr->a_th[i];
		g_kdrv_ife_info[id].par_filter.rng_filt_gr.b_th[i] = ptr->b_th[i];
	}
	for (i = 0; i < KDRV_RNG_LUT_LEN; i++) {
		g_kdrv_ife_info[id].par_filter.rng_filt_gr.a_lut[i] = ptr->a_lut[i];
		g_kdrv_ife_info[id].par_filter.rng_filt_gr.b_lut[i] = ptr->b_lut[i];
	}

	return E_OK;
}

static ER kdrv_ife_set_filter_range_ch_gb_param(UINT32 id, void *data)
{
	KDRV_IFE_RANGE_FILTER_GB *ptr = (KDRV_IFE_RANGE_FILTER_GB *)data;
	UINT32 i;

	for (i = 0; i < KDRV_RNG_TH_LEN; i++) {
		g_kdrv_ife_info[id].par_filter.rng_filt_gb.a_th[i] = ptr->a_th[i];
		g_kdrv_ife_info[id].par_filter.rng_filt_gb.b_th[i] = ptr->b_th[i];
	}
	for (i = 0; i < KDRV_RNG_LUT_LEN; i++) {
		g_kdrv_ife_info[id].par_filter.rng_filt_gb.a_lut[i] = ptr->a_lut[i];
		g_kdrv_ife_info[id].par_filter.rng_filt_gb.b_lut[i] = ptr->b_lut[i];
	}

	return E_OK;
}

static ER kdrv_ife_set_filter_range_ch_b_param(UINT32 id, void *data)
{
	KDRV_IFE_RANGE_FILTER_B *ptr = (KDRV_IFE_RANGE_FILTER_B *)data;
	UINT32 i;

	for (i = 0; i < KDRV_RNG_TH_LEN; i++) {
		g_kdrv_ife_info[id].par_filter.rng_filt_b.a_th[i] = ptr->a_th[i];
		g_kdrv_ife_info[id].par_filter.rng_filt_b.b_th[i] = ptr->b_th[i];
	}
	for (i = 0; i < KDRV_RNG_LUT_LEN; i++) {
		g_kdrv_ife_info[id].par_filter.rng_filt_b.a_lut[i] = ptr->a_lut[i];
		g_kdrv_ife_info[id].par_filter.rng_filt_b.b_lut[i] = ptr->b_lut[i];
	}

	return E_OK;
}

static ER kdrv_ife_set_filter_range_ch_ir_param(UINT32 id, void *data)
{
	KDRV_IFE_RANGE_FILTER_IR *ptr = (KDRV_IFE_RANGE_FILTER_IR *)data;
	UINT32 i;

	for (i = 0; i < KDRV_RNG_TH_LEN; i++) {
		g_kdrv_ife_info[id].par_filter.rng_filt_ir.a_th[i] = ptr->a_th[i];
		g_kdrv_ife_info[id].par_filter.rng_filt_ir.b_th[i] = ptr->b_th[i];
	}
	for (i = 0; i < KDRV_RNG_LUT_LEN; i++) {
		g_kdrv_ife_info[id].par_filter.rng_filt_ir.a_lut[i] = ptr->a_lut[i];
		g_kdrv_ife_info[id].par_filter.rng_filt_ir.b_lut[i] = ptr->b_lut[i];
	}

	return E_OK;
}

static ER kdrv_ife_set_filter_cnt_mod_param(UINT32 id, void *data)
{
	KDRV_IFE_CENTER_MODIFY *ptr = (KDRV_IFE_CENTER_MODIFY *)data;

	g_kdrv_ife_info[id].par_filter.center_mod.enable  =  ptr->enable;
	if (g_kdrv_ife_info[id].par_filter.center_mod.enable) {
		g_kdrv_ife_info[id].par_filter.center_mod.th1  =  ptr->th1;
		g_kdrv_ife_info[id].par_filter.center_mod.th2  =  ptr->th2;
	}

	return E_OK;
}

static ER kdrv_ife_set_filter_clamp_param(UINT32 id, void *data)
{
	KDRV_IFE_CLAMP *ptr = (KDRV_IFE_CLAMP *)data;

	g_kdrv_ife_info[id].par_filter.clamp.th      = ptr->th;
	g_kdrv_ife_info[id].par_filter.clamp.mul     = ptr->mul;
	g_kdrv_ife_info[id].par_filter.clamp.dlt     = ptr->dlt;

	return E_OK;
}

static ER kdrv_ife_set_filter_rbfill_param(UINT32 id, void *data)
{
	KDRV_IFE_RBFILL_PARAM *ptr = (KDRV_IFE_RBFILL_PARAM *)data;
	UINT32 i;

	g_kdrv_ife_info[id].par_filter.rbfill.enable = ptr->enable;

	if (g_kdrv_ife_info[id].par_filter.rbfill.enable) {
		g_kdrv_ife_info[id].par_filter.rbfill.ratio_mode  = ptr->ratio_mode;
		for (i = 0; i < KDRV_RBFILL_LUMA_NUM; i++) {
			g_kdrv_ife_info[id].par_filter.rbfill.luma [i] = ptr->luma[i];
		}
		for (i = 0; i < KDRV_RBFILL_RATIO_NUM; i++) {
			g_kdrv_ife_info[id].par_filter.rbfill.ratio[i] = ptr->ratio[i];
		}
	}

	return E_OK;
}

static ER kdrv_ife_set_filter_param(UINT32 id, void *data)
{
	KDRV_IFE_FILTER_PARAM *ptr;
	UINT32 ife_lock_flag;

	if(data == NULL){
		DBG_ERR("id %d ife set filter_param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}
	
	ptr = (KDRV_IFE_FILTER_PARAM *)data;

	ife_lock_flag = ife_platform_spin_lock();
	//g_kdrv_ife_info[id].par_filter.enable = ptr->enable;
	if(ptr->enable == TRUE){
		g_kdrv_ife_iq_ctrl_en[id] |= KDRV_IFE_IQ_FUNC_FILTER;
	}else{
		g_kdrv_ife_iq_ctrl_en[id] &= (~KDRV_IFE_IQ_FUNC_FILTER);
	}

	if (ptr->enable) {

		g_kdrv_ife_info[id].par_filter.blend_w     = ptr->blend_w;
		g_kdrv_ife_info[id].par_filter.bin         = ptr->bin;
		g_kdrv_ife_info[id].par_filter.rng_th_w    = ptr->rng_th_w;

		kdrv_ife_set_filter_spatial_param(id, &ptr->spatial);

		kdrv_ife_set_filter_range_ch_r_param(id, &ptr->rng_filt_r);
		kdrv_ife_set_filter_range_ch_gr_param(id, &ptr->rng_filt_gr);
		kdrv_ife_set_filter_range_ch_gb_param(id, &ptr->rng_filt_gb);
		kdrv_ife_set_filter_range_ch_b_param(id, &ptr->rng_filt_b);
		kdrv_ife_set_filter_range_ch_ir_param(id, &ptr->rng_filt_ir);

		kdrv_ife_set_filter_cnt_mod_param(id, &ptr->center_mod);
		kdrv_ife_set_filter_clamp_param(id, &ptr->clamp);
		kdrv_ife_set_filter_rbfill_param(id, &ptr->rbfill);
	}
	g_ife_update_flag[id][KDRV_IFE_PARAM_IQ_FILTER] = TRUE;
	
	ife_platform_spin_unlock(ife_lock_flag);

	return E_OK;
}

static ER kdrv_ife_set_cgain_param(UINT32 id, void *data)
{
	KDRV_IFE_CGAIN_PARAM *ptr;
	UINT32 ife_lock_flag;

	//g_kdrv_ife_info[id].par_cgain.enable         = ptr->enable;

	if(data == NULL){
		DBG_ERR("id %d ife set cgain_param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	ptr = (KDRV_IFE_CGAIN_PARAM *)data;
		
	ife_lock_flag = ife_platform_spin_lock();
	
	if(ptr->enable){
		g_kdrv_ife_iq_ctrl_en[id] |= KDRV_IFE_IQ_FUNC_CGAIN;
	}else{
		g_kdrv_ife_iq_ctrl_en[id] &= (~KDRV_IFE_IQ_FUNC_CGAIN);
	}

	if (ptr->enable) {
		
		g_kdrv_ife_info[id].par_cgain.inv      = ptr->inv;
		g_kdrv_ife_info[id].par_cgain.hinv     = ptr->hinv;
		g_kdrv_ife_info[id].par_cgain.bit_field  = ptr->bit_field;
		g_kdrv_ife_info[id].par_cgain.mask       = ptr->mask;

		g_kdrv_ife_info[id].par_cgain.cgain_r  = ptr->cgain_r;
		g_kdrv_ife_info[id].par_cgain.cgain_gr = ptr->cgain_gr;
		g_kdrv_ife_info[id].par_cgain.cgain_gb = ptr->cgain_gb;
		g_kdrv_ife_info[id].par_cgain.cgain_b  = ptr->cgain_b;
		g_kdrv_ife_info[id].par_cgain.cgain_ir = ptr->cgain_ir;

		g_kdrv_ife_info[id].par_cgain.cofs_r  = ptr->cofs_r;
		g_kdrv_ife_info[id].par_cgain.cofs_gr = ptr->cofs_gr;
		g_kdrv_ife_info[id].par_cgain.cofs_gb = ptr->cofs_gb;
		g_kdrv_ife_info[id].par_cgain.cofs_b  = ptr->cofs_b;
		g_kdrv_ife_info[id].par_cgain.cofs_ir = ptr->cofs_ir;
	}

	g_ife_update_flag[id][KDRV_IFE_PARAM_IQ_CGAIN] = TRUE;
	
	ife_platform_spin_unlock(ife_lock_flag);
	
	return E_OK;
}

static ER kdrv_ife_set_vig_param(UINT32 id, void *data)
{
	KDRV_IFE_VIG_PARAM *ptr;
	UINT32 ife_lock_flag;

	if(data == NULL){
		DBG_ERR("id %d ife set vig_param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	ptr = (KDRV_IFE_VIG_PARAM *)data;
	
	ife_lock_flag = ife_platform_spin_lock();
	
	//g_kdrv_ife_info[id].par_vig.enable               = ptr->enable;
	if(ptr->enable == TRUE){
		g_kdrv_ife_iq_ctrl_en[id] |= KDRV_IFE_IQ_FUNC_VIG_PARAM;
	}else{
		g_kdrv_ife_iq_ctrl_en[id] &= (~KDRV_IFE_IQ_FUNC_VIG_PARAM);
	}

	if (ptr->enable == TRUE){

		g_kdrv_ife_info[id].par_vig.dist_th           = ptr->dist_th;
		g_kdrv_ife_info[id].par_vig.dither_enable     = ptr->dither_enable;
		g_kdrv_ife_info[id].par_vig.dither_rst_enable = ptr->dither_rst_enable;

		g_kdrv_ife_info[id].par_vig.vig_fisheye_gain_en = ptr->vig_fisheye_gain_en;
		g_kdrv_ife_info[id].par_vig.vig_fisheye_radius  = ptr->vig_fisheye_radius;
		g_kdrv_ife_info[id].par_vig.vig_fisheye_slope   = ptr->vig_fisheye_slope;

		memcpy(g_kdrv_ife_info[id].par_vig.ch_r_lut, &ptr->ch_r_lut, sizeof(UINT32)*KDRV_VIG_LUT_LEN);
		memcpy(g_kdrv_ife_info[id].par_vig.ch_gr_lut, &ptr->ch_gr_lut, sizeof(UINT32)*KDRV_VIG_LUT_LEN);
		memcpy(g_kdrv_ife_info[id].par_vig.ch_gb_lut, &ptr->ch_gb_lut, sizeof(UINT32)*KDRV_VIG_LUT_LEN);
		memcpy(g_kdrv_ife_info[id].par_vig.ch_b_lut, &ptr->ch_b_lut, sizeof(UINT32)*KDRV_VIG_LUT_LEN);
		memcpy(g_kdrv_ife_info[id].par_vig.ch_ir_lut, &ptr->ch_ir_lut, sizeof(UINT32)*KDRV_VIG_LUT_LEN);
	}

	g_ife_update_flag[id][KDRV_IFE_PARAM_IQ_VIG] = TRUE;

	ife_platform_spin_unlock(ife_lock_flag);

	return E_OK;
}

static ER kdrv_ife_set_vig_info(UINT32 id, void *data)
{
	KDRV_IFE_VIG_CENTER *ptr = (KDRV_IFE_VIG_CENTER *)data;

	g_vig_center_x[id][0] = g_kdrv_ife_info[id].vig_info.ch0.x     = ptr->ch0.x;
	g_vig_center_x[id][1] = g_kdrv_ife_info[id].vig_info.ch1.x     = ptr->ch1.x;
	g_vig_center_x[id][2] = g_kdrv_ife_info[id].vig_info.ch2.x     = ptr->ch2.x;
	g_vig_center_x[id][3] = g_kdrv_ife_info[id].vig_info.ch3.x     = ptr->ch3.x;

	g_vig_center_y[id][0] = g_kdrv_ife_info[id].vig_info.ch0.y     = ptr->ch0.y;
	g_vig_center_y[id][1] = g_kdrv_ife_info[id].vig_info.ch1.y     = ptr->ch1.y;
	g_vig_center_y[id][2] = g_kdrv_ife_info[id].vig_info.ch2.y     = ptr->ch2.y;
	g_vig_center_y[id][3] = g_kdrv_ife_info[id].vig_info.ch3.y     = ptr->ch3.y;


	// Vig center
	//g_vig_center_x[id][0] = g_kdrv_ife_info[id].vig_info.ch0.x;
	//g_vig_center_x[id][1] = g_kdrv_ife_info[id].vig_info.ch1.x;
	//g_vig_center_x[id][2] = g_kdrv_ife_info[id].vig_info.ch2.x;
	//g_vig_center_x[id][3] = g_kdrv_ife_info[id].vig_info.ch3.x;
	//g_vig_center_y[id][0] = g_kdrv_ife_info[id].vig_info.ch0.y;
	//g_vig_center_y[id][1] = g_kdrv_ife_info[id].vig_info.ch1.y;
	//g_vig_center_y[id][2] = g_kdrv_ife_info[id].vig_info.ch2.y;
	//g_vig_center_y[id][3] = g_kdrv_ife_info[id].vig_info.ch3.y;

	return E_OK;
}

static ER kdrv_ife_set_gbal_param(UINT32 id, void *data)
{
	KDRV_IFE_GBAL_PARAM *ptr;
	UINT32 i, ife_lock_flag;

	if(data == NULL){
		DBG_ERR("id %d ife set gbal_param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	ptr = (KDRV_IFE_GBAL_PARAM *)data;

	ife_lock_flag = ife_platform_spin_lock();

	if(ptr->enable == TRUE){
		g_kdrv_ife_iq_ctrl_en[id] |= KDRV_IFE_IQ_FUNC_GBAL;
	}else{
		g_kdrv_ife_iq_ctrl_en[id] &= (~KDRV_IFE_IQ_FUNC_GBAL);
	}
	//g_kdrv_ife_info[id].par_gbal.enable              = ptr->enable;

	if (ptr->enable == TRUE) {

		g_kdrv_ife_info[id].par_gbal.protect_enable      = ptr->protect_enable;
		g_kdrv_ife_info[id].par_gbal.diff_th_str         = ptr->diff_th_str;
		g_kdrv_ife_info[id].par_gbal.diff_w_max          = ptr->diff_w_max;
		g_kdrv_ife_info[id].par_gbal.edge_protect_th1    = ptr->edge_protect_th1;
		g_kdrv_ife_info[id].par_gbal.edge_protect_th0    = ptr->edge_protect_th0;
		g_kdrv_ife_info[id].par_gbal.edge_w_max          = ptr->edge_w_max;
		g_kdrv_ife_info[id].par_gbal.edge_w_min          = ptr->edge_w_min;

		for (i = 0; i < KDRV_GBAL_OFS_LEN; i++) {
			g_kdrv_ife_info[id].par_gbal.gbal_ofs[i]     = ptr->gbal_ofs[i];
		}
	}
	g_ife_update_flag[id][KDRV_IFE_PARAM_IQ_GBAL] = TRUE;

	ife_platform_spin_unlock(ife_lock_flag);
		
	return E_OK;
}
ER kdrv_ife_set_iq_all(UINT32 id, void  *data)
{
	KDRV_IFE_PARAM_IQ_ALL_PARAM  *iq_all = (KDRV_IFE_PARAM_IQ_ALL_PARAM *)data;

	if(data == NULL){
		DBG_ERR("id %d ife set_iq_all error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}	

	if (iq_all->p_nrs != NULL) {
		kdrv_ife_set_nrs_param(id, (void *)iq_all->p_nrs);
	}
	if (iq_all->p_fusion != NULL) {
		kdrv_ife_set_fusion_param(id, (void *)iq_all->p_fusion);
	}
	if (iq_all->p_fcurve != NULL) {
		kdrv_ife_set_fcurve_param(id, (void *)iq_all->p_fcurve);
	}


	if (iq_all->p_outl  != NULL) {
		kdrv_ife_set_outl_param(id, (void *)iq_all->p_outl);
	}
	if (iq_all->p_filt  != NULL) {
		kdrv_ife_set_filter_param(id, (void *)iq_all->p_filt);
	}
	if (iq_all->p_cgain != NULL) {
		kdrv_ife_set_cgain_param(id, (void *)iq_all->p_cgain);
	}
	if (iq_all->p_vig   != NULL) {
		kdrv_ife_set_vig_param(id, (void *)iq_all->p_vig);
	}
	if (iq_all->p_gbal  != NULL) {
		kdrv_ife_set_gbal_param(id, (void *)iq_all->p_gbal);
	}

	return E_OK;
}

static ER kdrv_ife_set_ipl_func_enable(UINT32 id, void *data)
{
	KDRV_IFE_PARAM_IPL_FUNC_EN *ipl_ctrl = (KDRV_IFE_PARAM_IPL_FUNC_EN *)data;

	if (ipl_ctrl->enable) {
		g_kdrv_ife_ipl_ctrl_en[id] |= ipl_ctrl->ipl_ctrl_func;
	} else {
		g_kdrv_ife_ipl_ctrl_en[id] &= (~ipl_ctrl->ipl_ctrl_func);
	}

	return E_OK;
}

KDRV_IFE_SET_FP kdrv_ife_set_fp[KDRV_IFE_PARAM_MAX] = {
	//kdrv_ife_set_trig,
	kdrv_ife_set_clock,
	kdrv_ife_set_in_img_info,
	kdrv_ife_set_in_addr_1,
	kdrv_ife_set_in_addr_2,
	kdrv_ife_set_in_ofs_1,
	kdrv_ife_set_in_ofs_2,
	kdrv_ife_set_out_img_info,
	kdrv_ife_set_out_addr,
	kdrv_ife_set_isrcb,
	kdrv_ife_set_inte_en,
	kdrv_ife_set_mirror,
	kdrv_ife_set_rde_info,
	kdrv_ife_set_fusion_info,
	kdrv_ife_set_ring_buf_info,
	kdrv_ife_set_dma_wait_sie2_st_disable,
	kdrv_ife_set_load,

	kdrv_ife_set_nrs_param,
	kdrv_ife_set_fusion_param,
	kdrv_ife_set_fcurve_param,

	kdrv_ife_set_outl_param,
	kdrv_ife_set_filter_param,
	kdrv_ife_set_cgain_param,
	kdrv_ife_set_vig_param,
	kdrv_ife_set_vig_info,
	kdrv_ife_set_gbal_param,
	kdrv_ife_set_iq_all,
	kdrv_ife_set_ipl_func_enable,
	kdrv_ife_set_reset,
	kdrv_ife_set_builtin_flow_disable,
};


INT32 kdrv_ife_set(UINT32 id, KDRV_IFE_PARAM_ID parm_id, void *p_param)
{
	ER rt = E_OK;
	UINT32 ign_chk;
	KDRV_IFE_HANDLE *p_handle;
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);

	if (KDRV_DEV_ID_ENGINE(id) != KDRV_VIDEOPROCS_IFE_ENGINE0) {
		DBG_ERR("Invalid engine 0x%x\r\n", (unsigned int)KDRV_DEV_ID_ENGINE(id));
		return E_SYS;
	}

	if (parm_id == KDRV_IFE_PARAM_IPL_BUILTIN_DISABLE) {
	    if (kdrv_ife_set_fp[parm_id] != NULL) {
    		rt = kdrv_ife_set_fp[parm_id](KDRV_DEV_ID_CHANNEL(id), p_param);
    	}
	} else {
    	if (g_kdrv_ife_init_flg == 0) {
    		if (kdrv_ife_handle_alloc(&g_kdrv_ife_init_flg)) {
    			DBG_WRN("KDRV IFE: no free handle, max handle num = %d\r\n", kdrv_ife_channel_num);
    			return E_SYS;
    		}
    	}

    	p_handle = &g_kdrv_ife_handle_tab[channel];

    	if (kdrv_ife_chk_handle(p_handle) == NULL) {
    		DBG_ERR("KDRV IFE: illegal handle 0x%.8x\r\n", (unsigned int)&g_kdrv_ife_handle_tab[channel]);
    		return E_SYS;
    	}

    	if ((p_handle->sts & KDRV_IFE_HANDLE_LOCK) != KDRV_IFE_HANDLE_LOCK) {
    		DBG_ERR("KDRV IFE: illegal handle sts 0x%.8x\r\n", (unsigned int)p_handle->sts);
    		return E_SYS;
    	}

    	ign_chk = (KDRV_IFE_IGN_CHK & parm_id);
    	parm_id = parm_id & (~KDRV_IFE_IGN_CHK);

    	if (!ign_chk) {
    		kdrv_ife_lock(p_handle, TRUE);
    	}

    	if (kdrv_ife_set_fp[parm_id] != NULL) {
    		rt = kdrv_ife_set_fp[parm_id](KDRV_DEV_ID_CHANNEL(id), p_param);
    	}

    	if (!ign_chk) {
    		kdrv_ife_lock(p_handle, FALSE);
    	}
	}

	return rt;
}

/*
INT32 kdrv_ife_set(UINT32 id, KDRV_IFE_PARAM_ID parm_id, void *p_param)
{
    ER rt = E_OK;
    UINT32 ign_chk;

    ign_chk = (KDRV_IFE_IGN_CHK & parm_id);
    parm_id = parm_id & (~KDRV_IFE_IGN_CHK);

    if (kdrv_ife_set_fp[parm_id] != NULL) {
        rt = kdrv_ife_set_fp[parm_id](KDRV_DEV_ID_CHANNEL(id), p_param);
    }
    return rt;
}
EXPORT_SYMBOL(kdrv_ife_set);
*/
/*
ER kdrv_ife_set_org(UINT32 hdl, KDRV_IFE_PARAM_ID item, void* data)
{
    ER rt = E_OK;
    KDRV_IFE_HANDLE* p_handle;
    UINT32 ign_chk;

    p_handle = (KDRV_IFE_HANDLE *)hdl;
    if (kdrv_ife_chk_handle(p_handle) == NULL) {
        DBG_ERR("KDRV IFE: illegal handle 0x%.8x\r\n", hdl);
        return E_SYS;
    }

    if ((p_handle->sts & KDRV_IFE_HANDLE_LOCK) != KDRV_IFE_HANDLE_LOCK) {
        DBG_ERR("KDRV IFE: illegal handle sts 0x%.8x\r\n", p_handle->sts);
        return E_SYS;
    }

    ign_chk = (KDRV_IFE_IGN_CHK & item);
    item = item & (~KDRV_IFE_IGN_CHK);

    if (!ign_chk) {
        kdrv_ife_lock(p_handle, TRUE);
    }
    if (kdrv_ife_set_fp[item] != NULL) {
        rt = kdrv_ife_set_fp[item](p_handle->entry_id, data);
    }

    if (!ign_chk) {
        kdrv_ife_lock(p_handle, FALSE);
    }
    return rt;
}
EXPORT_SYMBOL(kdrv_ife_set);
*/


#if 0
#endif
static ER kdrv_ife_get_in_addr_1(UINT32 id, void *data)
{
	*(UINT32 *)data = g_kdrv_ife_info[id].in_pixel_addr_1;
	return E_OK;
}

static ER kdrv_ife_get_in_addr_2(UINT32 id, void *data)
{
	*(UINT32 *)data = g_kdrv_ife_info[id].in_pixel_addr_2;
	return E_OK;
}

static ER kdrv_ife_get_in_ofs_1(UINT32 id, void *data)
{
	*(UINT32 *)data = g_kdrv_ife_info[id].line_ofs_1;
	return E_OK;
}

static ER kdrv_ife_get_in_ofs_2(UINT32 id, void *data)
{
	*(UINT32 *)data = g_kdrv_ife_info[id].line_ofs_2;
	return E_OK;
}

static ER kdrv_ife_get_in_img_info(UINT32 id, void *data)
{
	KDRV_IFE_IN_IMG_INFO *ptr = (KDRV_IFE_IN_IMG_INFO *)data;

	ptr->in_src               = g_kdrv_ife_info[id].in_img_info.in_src;
	ptr->type                 = g_kdrv_ife_info[id].in_img_info.type;
	ptr->img_info.width       = g_kdrv_ife_info[id].in_img_info.img_info.width;
	ptr->img_info.height      = g_kdrv_ife_info[id].in_img_info.img_info.height;
	ptr->img_info.st_pix      = g_kdrv_ife_info[id].in_img_info.img_info.st_pix;

	//ptr->img_info.stripe_hm = g_kdrv_ife_info[id].in_img_info.img_info.stripe_hm;
	//ptr->img_info.stripe_hl = g_kdrv_ife_info[id].in_img_info.img_info.stripe_hl;
	//ptr->img_info.stripe_hn = g_kdrv_ife_info[id].in_img_info.img_info.stripe_hn;
	//ptr->img_info.h_shift   = g_kdrv_ife_info[id].in_img_info.img_info.h_shift;

	//ptr->img_info.cfa_pat   = g_kdrv_ife_info[id].in_img_info.img_info.cfa_pat;

	switch (g_kdrv_ife_info[id].in_img_info.img_info.bit) {
	case KDRV_IFE_RAW_BIT_8:
		ptr->img_info.bit   = KDRV_IPP_RAW_BIT_8;
		break;
	case KDRV_IFE_RAW_BIT_10:
		ptr->img_info.bit   = KDRV_IPP_RAW_BIT_10;
		break;
	case KDRV_IFE_RAW_BIT_12:
		ptr->img_info.bit   = KDRV_IPP_RAW_BIT_12;
		break;
	case KDRV_IFE_RAW_BIT_16:
		ptr->img_info.bit   = KDRV_IPP_RAW_BIT_16;
		break;
	default:
		DBG_ERR("IFE: get input bit failed!\r\n");
		return E_SYS;
	}

	return E_OK;
}

static ER kdrv_ife_get_out_addr(UINT32 id, void *data)
{
	*(UINT32 *)data = g_kdrv_ife_info[id].out_pixel_addr;
	return E_OK;
}

static ER kdrv_ife_get_out_img_info(UINT32 id, void *data)
{
	KDRV_IFE_OUT_IMG_INFO *ptr = (KDRV_IFE_OUT_IMG_INFO *)data;

	ptr->line_ofs   = g_kdrv_ife_info[id].out_img_info.line_ofs;

	switch (g_kdrv_ife_info[id].out_img_info.bit) {
	case KDRV_IFE_RAW_BIT_8:
		ptr->bit    = KDRV_IPP_RAW_BIT_8;
		break;
	case KDRV_IFE_RAW_BIT_12:
		ptr->bit    = KDRV_IPP_RAW_BIT_12;
		break;
	case KDRV_IFE_RAW_BIT_16:
		ptr->bit    = KDRV_IPP_RAW_BIT_16;
		break;
	default:
		DBG_ERR("IFE: get output bit failed!\r\n");
		return E_SYS;
	}

	return E_OK;
}

static ER kdrv_ife_get_inte_en(UINT32 id, void *data)
{
	*(KDRV_IFE_INTE *)data = g_kdrv_ife_info[id].inte_en;
	return E_OK;
}

/*
static ER kdrv_ife_get_rde_q_table_info(UINT32 id, void *data)
{
    KDRV_IFE_Q_TAB *ptr = (KDRV_IFE_Q_TAB *)data;
    UINT32 i;

    for (i = 0; i < RDE_Q_TAB_NUM; i++) {
        ptr->idx[i] = g_kdrv_ife_info[id].par_rde.q_table.idx[i];
    }

    return E_OK;
}

static ER kdrv_ife_get_rde_degamma_info(UINT32 id, void *data)
{
    KDRV_IFE_DEGAMMA *ptr = (KDRV_IFE_DEGAMMA *)data;
    UINT32 i;

    ptr->enable = g_kdrv_ife_info[id].par_rde.degamma.enable;
    for (i = 0; i < RDE_DEGAMMA_NUM; i++) {
        ptr->table[i] = g_kdrv_ife_info[id].par_rde.degamma.table[i];
    }

    return E_OK;
}

static ER kdrv_ife_get_rde_dither_info(UINT32 id, void *data)
{
    KDRV_IFE_RDE_DITHER *ptr = (KDRV_IFE_RDE_DITHER *)data;

    ptr->enable = g_kdrv_ife_info[id].par_rde.dither.enable;

    ptr->dither_reset = g_kdrv_ife_info[id].par_rde.dither.dither_reset;
    ptr->Rand1Init1   = g_kdrv_ife_info[id].par_rde.dither.Rand1Init1;
    ptr->Rand1Init2   = g_kdrv_ife_info[id].par_rde.dither.Rand1Init2;
    ptr->Rand2Init1   = g_kdrv_ife_info[id].par_rde.dither.Rand2Init1;
    ptr->Rand2Init2   = g_kdrv_ife_info[id].par_rde.dither.Rand2Init2;

    return E_OK;
}
*/
static ER kdrv_ife_get_rde_info(UINT32 id, void *data)
{
	KDRV_IFE_RDE_INFO *ptr = (KDRV_IFE_RDE_INFO *)data;

	//ptr->enable      = g_kdrv_ife_info[id].info_rde.enable;
	//ptr->encode_rate = g_kdrv_ife_info[id].info_rde.encode_rate;

	*ptr = g_kdrv_ife_info[id].info_rde;

	/*
	    kdrv_ife_get_rde_q_table_info(id, &ptr->q_table);
	    kdrv_ife_get_rde_degamma_info(id, &ptr->degamma);
	    kdrv_ife_get_rde_dither_info(id, &ptr->dither);
	*/
	return E_OK;
}


static ER kdrv_ife_get_nrs_order_param(UINT32 id, void *data)
{
	KDRV_IFE_ORDER_PARAM *ptr = (KDRV_IFE_ORDER_PARAM *)data;
	//UINT32 i;

#if 0
	ptr->enable = g_kdrv_ife_info[id].par_nrs.order.enable;


	ptr->rng_bri  = g_kdrv_ife_info[id].par_nrs.order.rng_bri;
	ptr->rng_dark = g_kdrv_ife_info[id].par_nrs.order.rng_dark;
	ptr->diff_th  = g_kdrv_ife_info[id].par_nrs.order.diff_th;
	for (i = 0; i < KDRV_NRS_ORD_W_LEN; i++) {
		ptr->bri_w[i] = g_kdrv_ife_info[id].par_nrs.order.bri_w[i];
		ptr->dark_w[i] = g_kdrv_ife_info[id].par_nrs.order.dark_w[i];
	}
#endif

	*ptr = g_kdrv_ife_info[id].par_nrs.order;


	return E_OK;
}

static ER kdrv_ife_get_nrs_bilat_param(UINT32 id, void *data)
{
	KDRV_IFE_BILAT_PARAM *ptr = (KDRV_IFE_BILAT_PARAM *)data;
	//UINT32 i;

#if 0
	ptr->b_enable = g_kdrv_ife_info[id].par_nrs.bilat.b_enable;
	ptr->b_str    = g_kdrv_ife_info[id].par_nrs.bilat.b_str;
	for (i = 0; i < KDRV_NRS_B_OFS_LEN; i++) {
		ptr->b_ofs[i] = g_kdrv_ife_info[id].par_nrs.bilat.b_ofs[i];
	}
	for (i = 0; i < KDRV_NRS_B_RANGE_LEN; i++) {
		ptr->b_rng[i] = g_kdrv_ife_info[id].par_nrs.bilat.b_rng[i];
	}
	for (i = 0; i < KDRV_NRS_B_TH_LEN; i++) {
		ptr->b_th[i] = g_kdrv_ife_info[id].par_nrs.bilat.b_th[i];
	}

	ptr->gb_enable  = g_kdrv_ife_info[id].par_nrs.bilat.gb_enable;
	ptr->gb_str     = g_kdrv_ife_info[id].par_nrs.bilat.gb_str;
	ptr->gb_blend_w = g_kdrv_ife_info[id].par_nrs.bilat.gb_blend_w;
	for (i = 0; i < KDRV_NRS_GB_OFS_LEN; i++) {
		ptr->gb_ofs[i] = g_kdrv_ife_info[id].par_nrs.bilat.gb_ofs[i];
	}
	for (i = 0; i < KDRV_NRS_GB_RANGE_LEN; i++) {
		ptr->gb_rng[i] = g_kdrv_ife_info[id].par_nrs.bilat.gb_rng[i];
	}
	for (i = 0; i < KDRV_NRS_GB_TH_LEN; i++) {
		ptr->gb_th[i]  = g_kdrv_ife_info[id].par_nrs.bilat.gb_th[i];
	}
#endif

	*ptr = g_kdrv_ife_info[id].par_nrs.bilat;

	return E_OK;
}

static ER kdrv_ife_get_nrs_param(UINT32 id, void *data)
{
	KDRV_IFE_NRS_PARAM *ptr = (KDRV_IFE_NRS_PARAM *)data;

	//ptr->enable = g_kdrv_ife_info[id].par_nrs.enable;

	*ptr = g_kdrv_ife_info[id].par_nrs;


	kdrv_ife_get_nrs_order_param(id, &ptr->order);
	kdrv_ife_get_nrs_bilat_param(id, &ptr->bilat);


	return E_OK;
}

static ER kdrv_ife_get_fusion_fcgain_param(UINT32 id, void *data)
{
	KDRV_IFE_FCGAIN *ptr = (KDRV_IFE_FCGAIN *)data;

#if 0
	ptr->enable = g_kdrv_ife_info[id].par_fusion.fu_cgain.enable;
	ptr->bit_field = g_kdrv_ife_info[id].par_fusion.fu_cgain.bit_field;

	ptr->fcgain_path0_r  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path0_r;
	ptr->fcgain_path0_gr = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path0_gr;
	ptr->fcgain_path0_gb = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path0_gb;
	ptr->fcgain_path0_b  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path0_b;
	ptr->fcgain_path0_ir = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path0_ir;

	ptr->fcgain_path1_r  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path1_r;
	ptr->fcgain_path1_gr = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path1_gr;
	ptr->fcgain_path1_gb = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path1_gb;
	ptr->fcgain_path1_b  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path1_b;
	ptr->fcgain_path1_ir = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcgain_path1_ir;

	ptr->fcofs_path0_r  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path0_r;
	ptr->fcofs_path0_gr = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path0_gr;
	ptr->fcofs_path0_gb = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path0_gb;
	ptr->fcofs_path0_b  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path0_b;
	ptr->fcofs_path0_ir = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path0_ir;

	ptr->fcofs_path1_r  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path1_r;
	ptr->fcofs_path1_gr = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path1_gr;
	ptr->fcofs_path1_gb = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path1_gb;
	ptr->fcofs_path1_b  = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path1_b;
	ptr->fcofs_path1_ir = g_kdrv_ife_info[id].par_fusion.fu_cgain.fcofs_path1_ir;
#endif

	*ptr = g_kdrv_ife_info[id].par_fusion.fu_cgain;

	return E_OK;
}

static ER kdrv_ife_get_fusion_ctrl_param(UINT32 id, void *data)
{
	KDRV_IFE_FUSION_CTRL *ptr = (KDRV_IFE_FUSION_CTRL *)data;

	//ptr->y_mean_sel = g_kdrv_ife_info[id].par_fusion.fu_ctrl.y_mean_sel;
	//ptr->mode       = g_kdrv_ife_info[id].par_fusion.fu_ctrl.mode;
	//ptr->ev_ratio   = g_kdrv_ife_info[id].par_fusion.fu_ctrl.ev_ratio;

	*ptr = g_kdrv_ife_info[id].par_fusion.fu_ctrl;

	return E_OK;
}

static ER kdrv_ife_get_fusion_bld_cur_param(UINT32 id, void *data)
{
	KDRV_IFE_BLEND_CURVE *ptr = (KDRV_IFE_BLEND_CURVE *)data;
	//UINT32 i;

#if 0
	ptr->nor_sel = g_kdrv_ife_info[id].par_fusion.bld_cur.nor_sel;
	ptr->dif_sel = g_kdrv_ife_info[id].par_fusion.bld_cur.dif_sel;

	for (i = 0; i < FUSION_BLD_CUR_KNEE_NUM; i++) {
		ptr->l_nor_knee[i] = g_kdrv_ife_info[id].par_fusion.bld_cur.l_nor_knee[i];
		ptr->s_nor_knee[i] = g_kdrv_ife_info[id].par_fusion.bld_cur.s_nor_knee[i];
		ptr->l_dif_knee[i] = g_kdrv_ife_info[id].par_fusion.bld_cur.l_dif_knee[i];
		ptr->s_dif_knee[i] = g_kdrv_ife_info[id].par_fusion.bld_cur.s_dif_knee[i];
	}

	ptr->l_nor_range  = g_kdrv_ife_info[id].par_fusion.bld_cur.l_nor_range;
	ptr->l_nor_w_edge =  g_kdrv_ife_info[id].par_fusion.bld_cur.l_nor_w_edge;

	ptr->s_nor_range  = g_kdrv_ife_info[id].par_fusion.bld_cur.s_nor_range;
	ptr->s_nor_w_edge = g_kdrv_ife_info[id].par_fusion.bld_cur.s_nor_w_edge;

	ptr->l_dif_range  = g_kdrv_ife_info[id].par_fusion.bld_cur.l_dif_range;
	ptr->l_dif_w_edge = g_kdrv_ife_info[id].par_fusion.bld_cur.l_dif_w_edge;

	ptr->s_dif_range  = g_kdrv_ife_info[id].par_fusion.bld_cur.s_dif_range;
	ptr->s_dif_w_edge = g_kdrv_ife_info[id].par_fusion.bld_cur.s_dif_w_edge;
#endif

	*ptr = g_kdrv_ife_info[id].par_fusion.bld_cur;

	return E_OK;
}

static ER kdrv_ife_get_fusion_mc_param(UINT32 id, void *data)
{
	KDRV_IFE_MC *ptr = (KDRV_IFE_MC *)data;
	//UINT32 i;

#if 0
	ptr->dwd        = g_kdrv_ife_info[id].par_fusion.mc_para.dwd;
	ptr->diff_ratio = g_kdrv_ife_info[id].par_fusion.mc_para.diff_ratio;
	ptr->lum_th     = g_kdrv_ife_info[id].par_fusion.mc_para.lum_th;

	for (i = 0; i < FUSION_BLD_CUR_KNEE_NUM; i++) {
		ptr->neg_diff_w[i] = g_kdrv_ife_info[id].par_fusion.mc_para.neg_diff_w[i];
		ptr->pos_diff_w[i] = g_kdrv_ife_info[id].par_fusion.mc_para.pos_diff_w[i];
	}
#endif

	*ptr = g_kdrv_ife_info[id].par_fusion.mc_para;

	return E_OK;
}

static ER kdrv_ife_get_fusion_dark_sat_param(UINT32 id, void *data)
{
	KDRV_IFE_DARK_SAT *ptr = (KDRV_IFE_DARK_SAT *)data;
	//UINT32 i;

#if 0
	for (i = 0; i < FUSION_DARK_SAT_NUM; i++) {
		ptr->th[i]        = g_kdrv_ife_info[id].par_fusion.dk_sat.th[i];
		ptr->step[i]      = g_kdrv_ife_info[id].par_fusion.dk_sat.step[i];
		ptr->low_bound[i] = g_kdrv_ife_info[id].par_fusion.dk_sat.low_bound[i];
	}
#endif

	*ptr = g_kdrv_ife_info[id].par_fusion.dk_sat;

	return E_OK;
}

static ER kdrv_ife_get_fusion_s_comp_param(UINT32 id, void *data)
{
	KDRV_IFE_S_COMP *ptr = (KDRV_IFE_S_COMP *)data;
	//UINT32 i;

#if 0
	ptr->enable = g_kdrv_ife_info[id].par_fusion.s_comp.enable;

	for (i = 0; i < FUSION_SHORT_COMP_KNEE_NUM; i++) {
		ptr->knee[i] = g_kdrv_ife_info[id].par_fusion.s_comp.knee[i];
	}
	for (i = 0; i < FUSION_SHORT_COMP_SUB_NUM; i++) {
		ptr->sub_point[i] = g_kdrv_ife_info[id].par_fusion.s_comp.sub_point[i];
	}
	for (i = 0; i < FUSION_SHORT_COMP_SHIFT_NUM; i++) {
		ptr->shift[i] = g_kdrv_ife_info[id].par_fusion.s_comp.shift[i];
	}
#endif

	*ptr = g_kdrv_ife_info[id].par_fusion.s_comp;

	return E_OK;
}

static ER kdrv_ife_get_fusion_param(UINT32 id, void *data)
{
	KDRV_IFE_FUSION_PARAM *ptr = (KDRV_IFE_FUSION_PARAM *)data;

	kdrv_ife_get_fusion_ctrl_param(id, &ptr->fu_ctrl);
	kdrv_ife_get_fusion_bld_cur_param(id, &ptr->bld_cur);
	kdrv_ife_get_fusion_mc_param(id, &ptr->mc_para);
	kdrv_ife_get_fusion_dark_sat_param(id, &ptr->dk_sat);
	kdrv_ife_get_fusion_s_comp_param(id, &ptr->s_comp);
	kdrv_ife_get_fusion_fcgain_param(id, &ptr->fu_cgain);

	return E_OK;
}

static ER kdrv_ife_get_fusion_info(UINT32 id, void *data)
{
	KDRV_IFE_FUSION_INFO *ptr = (KDRV_IFE_FUSION_INFO *)data;

	ptr->enable = g_kdrv_ife_info[id].info_fusion.enable;

	if (g_kdrv_ife_info[id].info_fusion.enable) {
		ptr->fnum = g_kdrv_ife_info[id].info_fusion.fnum;
	}

	return E_OK;
}

static ER kdrv_ife_get_fcurve_ctrl_param(UINT32 id, void *data)
{
	KDRV_IFE_FCURVE_CTRL *ptr = (KDRV_IFE_FCURVE_CTRL *)data;

	//ptr->yv_w       = g_kdrv_ife_info[id].par_fcurve.fcur_ctrl.yv_w;
	//ptr->y_mean_sel = g_kdrv_ife_info[id].par_fcurve.fcur_ctrl.y_mean_sel;

	*ptr = g_kdrv_ife_info[id].par_fcurve.fcur_ctrl;

	return E_OK;
}

static ER kdrv_ife_get_fcurve_yw_param(UINT32 id, void *data)
{
	KDRV_IFE_Y_W *ptr = (KDRV_IFE_Y_W *)data;
	UINT32 i;

	for (i = 0; i < FCURVE_Y_W_NUM; i++) {
		ptr->y_w_lut[i] = g_kdrv_ife_info[id].par_fcurve.y_weight.y_w_lut[i];
	}

	return E_OK;
}

static ER kdrv_ife_get_fcurve_idx_param(UINT32 id, void *data)
{
	KDRV_IFE_IDX_LUT *ptr = (KDRV_IFE_IDX_LUT *)data;
	//UINT32 i;

#if 0
	for (i = 0; i < FCURVE_IDX_NUM; i++) {
		ptr->idx_lut[i] = g_kdrv_ife_info[id].par_fcurve.index.idx_lut[i];
	}
#endif

	*ptr = g_kdrv_ife_info[id].par_fcurve.index;

	return E_OK;
}

static ER kdrv_ife_get_fcurve_split_param(UINT32 id, void *data)
{
	KDRV_IFE_SPLIT_LUT *ptr = (KDRV_IFE_SPLIT_LUT *)data;
	//UINT32 i;

#if 0
	for (i = 0; i < FCURVE_SPLIT_NUM; i++) {
		ptr->split_lut[i] = g_kdrv_ife_info[id].par_fcurve.split.split_lut[i];
	}
#endif

	*ptr = g_kdrv_ife_info[id].par_fcurve.split;

	return E_OK;
}

static ER kdrv_ife_get_fcurve_val_param(UINT32 id, void *data)
{
	KDRV_IFE_VAL_LUT *ptr = (KDRV_IFE_VAL_LUT *)data;
	//UINT32 i;

#if 0
	for (i = 0; i < FCURVE_VAL_NUM; i++) {
		ptr->val_lut[i] = g_kdrv_ife_info[id].par_fcurve.value.val_lut[i];
	}
#endif

	*ptr = g_kdrv_ife_info[id].par_fcurve.value;

	return E_OK;
}

static ER kdrv_ife_get_fcurve_param(UINT32 id, void *data)
{
	KDRV_IFE_FCURVE_PARAM *ptr = (KDRV_IFE_FCURVE_PARAM *)data;

	ptr->enable = g_kdrv_ife_info[id].par_fcurve.enable;

	kdrv_ife_get_fcurve_ctrl_param(id, &ptr->fcur_ctrl);
	kdrv_ife_get_fcurve_yw_param(id, &ptr->y_weight);
	kdrv_ife_get_fcurve_idx_param(id, &ptr->index);
	kdrv_ife_get_fcurve_split_param(id, &ptr->split);
	kdrv_ife_get_fcurve_val_param(id, &ptr->value);

	return E_OK;
}

static ER kdrv_ife_get_mirror(UINT32 id, void *data)
{

	KDRV_IFE_MIRROR *ptr = (KDRV_IFE_MIRROR *)data;

	//ptr->enable = g_kdrv_ife_info[id].par_mirror.enable;
	*ptr = g_kdrv_ife_info[id].par_mirror;

	return E_OK;
}

static ER kdrv_ife_get_outl_param(UINT32 id, void *data)
{
	KDRV_IFE_OUTL_PARAM *ptr = (KDRV_IFE_OUTL_PARAM *)data;
	//UINT32 i;

	*ptr = g_kdrv_ife_info[id].par_outl;

#if 0
	ptr->enable = g_kdrv_ife_info[id].par_outl.enable;

	ptr->outl_weight = g_kdrv_ife_info[id].par_outl.outl_weight;
	ptr->dark_ofs    = g_kdrv_ife_info[id].par_outl.dark_ofs;
	ptr->bright_ofs  = g_kdrv_ife_info[id].par_outl.bright_ofs;

	for (i = 0; i < KDRV_OUTL_TH_LEN; i++) {
		ptr->bright_th[i] = g_kdrv_ife_info[id].par_outl.bright_th[i];
		ptr->dark_th  [i] = g_kdrv_ife_info[id].par_outl.dark_th[i];
	}
	ptr->outl_cnt[0]    = g_kdrv_ife_info[id].par_outl.outl_cnt[0];
	ptr->outl_cnt[1]    = g_kdrv_ife_info[id].par_outl.outl_cnt[1];

	ptr->avg_mode       = g_kdrv_ife_info[id].par_outl.avg_mode;

	ptr->ord_rng_bri    = g_kdrv_ife_info[id].par_outl.ord_rng_bri;
	ptr->ord_rng_dark   = g_kdrv_ife_info[id].par_outl.ord_rng_dark;
	ptr->ord_protect_th = g_kdrv_ife_info[id].par_outl.ord_protect_th;

	for (i = 0; i < KDRV_OUTL_ORD_W_LEN; i++) {
		ptr->ord_bri_w[i]  = g_kdrv_ife_info[id].par_outl.ord_bri_w[i];
		ptr->ord_dark_w[i] = g_kdrv_ife_info[id].par_outl.ord_dark_w[i];
	}

#if NAME_ERR
	ptr->ord_blend_w   = g_kdrv_ife_info[id].par_outl.ord_blend_w;
#else
	ptr->ord_bleand_w   = g_kdrv_ife_info[id].par_outl.ord_bleand_w;
#endif
#endif


	return E_OK;
}

static ER kdrv_ife_get_filter_spatial_param(UINT32 id, void *data)
{
	KDRV_IFE_SPATIAL *ptr = (KDRV_IFE_SPATIAL *)data;
	//UINT32 i;

#if 0
	for (i = 0; i < KDRV_SPATIAL_W_LEN; i++) {
		ptr->weight[i] = g_kdrv_ife_info[id].par_filter.spatial.weight[i];
	}
#endif

	*ptr = g_kdrv_ife_info[id].par_filter.spatial;

	return E_OK;
}

static ER kdrv_ife_get_filter_range_ch_r_param(UINT32 id, void *data)
{
	KDRV_IFE_RANGE_FILTER_R *ptr = (KDRV_IFE_RANGE_FILTER_R *)data;
	//UINT32 i;

#if 0
	for (i = 0; i < KDRV_RNG_TH_LEN; i++) {
		ptr->a_th[i] = g_kdrv_ife_info[id].par_filter.rng_filt_r.a_th[i];
		ptr->b_th[i] = g_kdrv_ife_info[id].par_filter.rng_filt_r.b_th[i];
	}
	for (i = 0; i < KDRV_RNG_LUT_LEN; i++) {
		ptr->a_lut[i] = g_kdrv_ife_info[id].par_filter.rng_filt_r.a_lut[i];
		ptr->b_lut[i] = g_kdrv_ife_info[id].par_filter.rng_filt_r.b_lut[i];
	}
#endif

	*ptr = g_kdrv_ife_info[id].par_filter.rng_filt_r;

	return E_OK;
}

static ER kdrv_ife_get_filter_range_ch_gr_param(UINT32 id, void *data)
{
	KDRV_IFE_RANGE_FILTER_GR *ptr = (KDRV_IFE_RANGE_FILTER_GR *)data;
	//UINT32 i;

#if 0
	for (i = 0; i < KDRV_RNG_TH_LEN; i++) {
		ptr->a_th[i] = g_kdrv_ife_info[id].par_filter.rng_filt_gr.a_th[i];
		ptr->b_th[i] = g_kdrv_ife_info[id].par_filter.rng_filt_gr.b_th[i];
	}
	for (i = 0; i < KDRV_RNG_LUT_LEN; i++) {
		ptr->a_lut[i] = g_kdrv_ife_info[id].par_filter.rng_filt_gr.a_lut[i];
		ptr->b_lut[i] = g_kdrv_ife_info[id].par_filter.rng_filt_gr.b_lut[i];
	}
#endif

	*ptr = g_kdrv_ife_info[id].par_filter.rng_filt_gr;

	return E_OK;
}

static ER kdrv_ife_get_filter_range_ch_gb_param(UINT32 id, void *data)
{
	KDRV_IFE_RANGE_FILTER_GB *ptr = (KDRV_IFE_RANGE_FILTER_GB *)data;
	//UINT32 i;

#if 0
	for (i = 0; i < KDRV_RNG_TH_LEN; i++) {
		ptr->a_th[i] = g_kdrv_ife_info[id].par_filter.rng_filt_gb.a_th[i];
		ptr->b_th[i] = g_kdrv_ife_info[id].par_filter.rng_filt_gb.b_th[i];
	}
	for (i = 0; i < KDRV_RNG_LUT_LEN; i++) {
		ptr->a_lut[i] = g_kdrv_ife_info[id].par_filter.rng_filt_gb.a_lut[i];
		ptr->b_lut[i] = g_kdrv_ife_info[id].par_filter.rng_filt_gb.b_lut[i];
	}
#endif

	*ptr = g_kdrv_ife_info[id].par_filter.rng_filt_gb;

	return E_OK;
}

static ER kdrv_ife_get_filter_range_ch_b_param(UINT32 id, void *data)
{
	KDRV_IFE_RANGE_FILTER_B *ptr = (KDRV_IFE_RANGE_FILTER_B *)data;
	//UINT32 i;

#if 0
	for (i = 0; i < KDRV_RNG_TH_LEN; i++) {
		ptr->a_th[i] = g_kdrv_ife_info[id].par_filter.rng_filt_b.a_th[i];
		ptr->b_th[i] = g_kdrv_ife_info[id].par_filter.rng_filt_b.b_th[i];
	}
	for (i = 0; i < KDRV_RNG_LUT_LEN; i++) {
		ptr->a_lut[i] = g_kdrv_ife_info[id].par_filter.rng_filt_b.a_lut[i];
		ptr->b_lut[i] = g_kdrv_ife_info[id].par_filter.rng_filt_b.b_lut[i];
	}
#endif

	*ptr = g_kdrv_ife_info[id].par_filter.rng_filt_b;

	return E_OK;
}

static ER kdrv_ife_get_filter_range_ch_ir_param(UINT32 id, void *data)
{
	KDRV_IFE_RANGE_FILTER_IR *ptr = (KDRV_IFE_RANGE_FILTER_IR *)data;
	//UINT32 i;

#if 0
	for (i = 0; i < KDRV_RNG_TH_LEN; i++) {
		ptr->a_th[i] = g_kdrv_ife_info[id].par_filter.rng_filt_ir.a_th[i];
		ptr->b_th[i] = g_kdrv_ife_info[id].par_filter.rng_filt_ir.b_th[i];
	}
	for (i = 0; i < KDRV_RNG_LUT_LEN; i++) {
		ptr->a_lut[i] = g_kdrv_ife_info[id].par_filter.rng_filt_ir.a_lut[i];
		ptr->b_lut[i] = g_kdrv_ife_info[id].par_filter.rng_filt_ir.b_lut[i];
	}
#endif

	*ptr = g_kdrv_ife_info[id].par_filter.rng_filt_ir;

	return E_OK;
}

static ER kdrv_ife_get_filter_cnt_mod_param(UINT32 id, void *data)
{
	KDRV_IFE_CENTER_MODIFY *ptr = (KDRV_IFE_CENTER_MODIFY *)data;

#if 0
	ptr->enable = g_kdrv_ife_info[id].par_filter.center_mod.enable;
	ptr->th1 = g_kdrv_ife_info[id].par_filter.center_mod.th1;
	ptr->th2 = g_kdrv_ife_info[id].par_filter.center_mod.th2;
#endif

	*ptr = g_kdrv_ife_info[id].par_filter.center_mod;

	return E_OK;
}

static ER kdrv_ife_get_filter_clamp_param(UINT32 id, void *data)
{
	KDRV_IFE_CLAMP *ptr = (KDRV_IFE_CLAMP *)data;

#if 0
	ptr->th  = g_kdrv_ife_info[id].par_filter.clamp.th;
	ptr->mul = g_kdrv_ife_info[id].par_filter.clamp.mul;
	ptr->dlt = g_kdrv_ife_info[id].par_filter.clamp.dlt;
#endif

	*ptr = g_kdrv_ife_info[id].par_filter.clamp;

	return E_OK;
}

static ER kdrv_ife_get_filter_rbfill_param(UINT32 id, void *data)
{
	KDRV_IFE_RBFILL_PARAM *ptr = (KDRV_IFE_RBFILL_PARAM *)data;
	//UINT32 i;

#if 0
	ptr->enable = g_kdrv_ife_info[id].par_filter.rbfill.enable;

	ptr->ratio_mode = g_kdrv_ife_info[id].par_filter.rbfill.ratio_mode;
	for (i = 0; i < KDRV_RBFILL_LUMA_NUM; i++) {
		ptr->luma[i] = g_kdrv_ife_info[id].par_filter.rbfill.luma[i];
	}
	for (i = 0; i < KDRV_RBFILL_RATIO_NUM; i++) {
		ptr->ratio[i] = g_kdrv_ife_info[id].par_filter.rbfill.ratio[i];
	}
#endif

	*ptr = g_kdrv_ife_info[id].par_filter.rbfill;

	return E_OK;
}

static ER kdrv_ife_get_filter_param(UINT32 id, void *data)
{
	KDRV_IFE_FILTER_PARAM *ptr = (KDRV_IFE_FILTER_PARAM *)data;

	g_kdrv_ife_info[id].par_filter.enable = ptr->enable;

	ptr->blend_w  = g_kdrv_ife_info[id].par_filter.blend_w;
	ptr->bin      = g_kdrv_ife_info[id].par_filter.bin;
	ptr->rng_th_w = g_kdrv_ife_info[id].par_filter.rng_th_w;

	kdrv_ife_get_filter_spatial_param(id, &ptr->spatial);

	kdrv_ife_get_filter_range_ch_r_param(id, &ptr->rng_filt_r);
	kdrv_ife_get_filter_range_ch_gr_param(id, &ptr->rng_filt_gr);
	kdrv_ife_get_filter_range_ch_gb_param(id, &ptr->rng_filt_gb);
	kdrv_ife_get_filter_range_ch_b_param(id, &ptr->rng_filt_b);
	kdrv_ife_get_filter_range_ch_ir_param(id, &ptr->rng_filt_ir);

	kdrv_ife_get_filter_cnt_mod_param(id, &ptr->center_mod);
	kdrv_ife_get_filter_clamp_param(id, &ptr->clamp);
	kdrv_ife_get_filter_rbfill_param(id, &ptr->rbfill);

	return E_OK;
}

static ER kdrv_ife_get_cgain_param(UINT32 id, void *data)
{
	KDRV_IFE_CGAIN_PARAM *ptr = (KDRV_IFE_CGAIN_PARAM *)data;

#if 0
	ptr->enable     = g_kdrv_ife_info[id].par_cgain.enable;
	ptr->inv        = g_kdrv_ife_info[id].par_cgain.inv;
	ptr->hinv       = g_kdrv_ife_info[id].par_cgain.hinv;
	ptr->bit_field  = g_kdrv_ife_info[id].par_cgain.bit_field;
	ptr->mask       = g_kdrv_ife_info[id].par_cgain.mask;

	ptr->cgain_r  = g_kdrv_ife_info[id].par_cgain.cgain_r;
	ptr->cgain_gr = g_kdrv_ife_info[id].par_cgain.cgain_gr;
	ptr->cgain_gb = g_kdrv_ife_info[id].par_cgain.cgain_gb;
	ptr->cgain_b  = g_kdrv_ife_info[id].par_cgain.cgain_b;
	ptr->cgain_ir = g_kdrv_ife_info[id].par_cgain.cgain_ir;

	ptr->cofs_r   = g_kdrv_ife_info[id].par_cgain.cofs_r;
	ptr->cofs_gr  = g_kdrv_ife_info[id].par_cgain.cofs_gr;
	ptr->cofs_gb  = g_kdrv_ife_info[id].par_cgain.cofs_gb;
	ptr->cofs_b   = g_kdrv_ife_info[id].par_cgain.cofs_b;
	ptr->cofs_ir  = g_kdrv_ife_info[id].par_cgain.cofs_ir;
#endif

	*ptr = g_kdrv_ife_info[id].par_cgain;

	return E_OK;
}

static ER kdrv_ife_get_vig_param(UINT32 id, void *data)
{
	KDRV_IFE_VIG_PARAM *ptr = (KDRV_IFE_VIG_PARAM *)data;

#if 0
	ptr->enable             = g_kdrv_ife_info[id].par_vig.enable;
	ptr->dist_th            = g_kdrv_ife_info[id].par_vig.dist_th;
	ptr->dither_enable      = g_kdrv_ife_info[id].par_vig.dither_enable;
	ptr->dither_rst_enable  = g_kdrv_ife_info[id].par_vig.dither_rst_enable;

	memcpy(&ptr->ch_r_lut, g_kdrv_ife_info[id].par_vig.ch_r_lut, sizeof(UINT32)*KDRV_VIG_LUT_LEN);
	memcpy(&ptr->ch_gr_lut, g_kdrv_ife_info[id].par_vig.ch_gr_lut, sizeof(UINT32)*KDRV_VIG_LUT_LEN);
	memcpy(&ptr->ch_gb_lut, g_kdrv_ife_info[id].par_vig.ch_gb_lut, sizeof(UINT32)*KDRV_VIG_LUT_LEN);
	memcpy(&ptr->ch_b_lut, g_kdrv_ife_info[id].par_vig.ch_b_lut, sizeof(UINT32)*KDRV_VIG_LUT_LEN);
	memcpy(&ptr->ch_ir_lut, g_kdrv_ife_info[id].par_vig.ch_ir_lut, sizeof(UINT32)*KDRV_VIG_LUT_LEN);
#endif

	*ptr = g_kdrv_ife_info[id].par_vig;

	return E_OK;
}

static ER kdrv_ife_get_vig_info(UINT32 id, void *data)
{
	KDRV_IFE_VIG_CENTER *ptr = (KDRV_IFE_VIG_CENTER *)data;

#if 0
	ptr->ch0.x       = g_kdrv_ife_info[id].vig_info.ch0.x;
	ptr->ch1.x       = g_kdrv_ife_info[id].vig_info.ch1.x;
	ptr->ch2.x       = g_kdrv_ife_info[id].vig_info.ch2.x;
	ptr->ch3.x       = g_kdrv_ife_info[id].vig_info.ch3.x;
	ptr->ch0.y       = g_kdrv_ife_info[id].vig_info.ch0.y;
	ptr->ch1.y       = g_kdrv_ife_info[id].vig_info.ch1.y;
	ptr->ch2.y       = g_kdrv_ife_info[id].vig_info.ch2.y;
	ptr->ch3.y       = g_kdrv_ife_info[id].vig_info.ch3.y;
#endif

	*ptr = g_kdrv_ife_info[id].vig_info;

	return E_OK;
}

static ER kdrv_ife_get_gbal_param(UINT32 id, void *data)
{
	KDRV_IFE_GBAL_PARAM *ptr = (KDRV_IFE_GBAL_PARAM *)data;
	//UINT32 i;

#if 0
	ptr->enable = g_kdrv_ife_info[id].par_gbal.enable;

	ptr->protect_enable   = g_kdrv_ife_info[id].par_gbal.protect_enable;
	ptr->diff_th_str      = g_kdrv_ife_info[id].par_gbal.diff_th_str;
	ptr->diff_w_max       = g_kdrv_ife_info[id].par_gbal.diff_w_max;
	ptr->edge_protect_th1 = g_kdrv_ife_info[id].par_gbal.edge_protect_th1;
	ptr->edge_protect_th0 = g_kdrv_ife_info[id].par_gbal.edge_protect_th0;
	ptr->edge_w_max       = g_kdrv_ife_info[id].par_gbal.edge_w_max;
	ptr->edge_w_min       = g_kdrv_ife_info[id].par_gbal.edge_w_min;

	for (i = 0; i < KDRV_GBAL_OFS_LEN; i++) {
		ptr->gbal_ofs[i] = g_kdrv_ife_info[id].par_gbal.gbal_ofs[i];
	}
#endif

	*ptr = g_kdrv_ife_info[id].par_gbal;

	return E_OK;
}

KDRV_IFE_GET_FP kdrv_ife_get_fp[KDRV_IFE_PARAM_MAX] = {
	NULL,
	kdrv_ife_get_in_img_info,
	kdrv_ife_get_in_addr_1,
	kdrv_ife_get_in_addr_2,
	kdrv_ife_get_in_ofs_1,
	kdrv_ife_get_in_ofs_2,
	kdrv_ife_get_out_img_info,
	kdrv_ife_get_out_addr,
	NULL,
	kdrv_ife_get_inte_en,
	kdrv_ife_get_mirror,
	kdrv_ife_get_rde_info,
	kdrv_ife_get_fusion_info,
	NULL,
	NULL,
	NULL,
	kdrv_ife_get_nrs_param,
	kdrv_ife_get_fusion_param,
	kdrv_ife_get_fcurve_param,

	kdrv_ife_get_outl_param,
	kdrv_ife_get_filter_param,
	kdrv_ife_get_cgain_param,
	kdrv_ife_get_vig_param,
	kdrv_ife_get_vig_info,
	kdrv_ife_get_gbal_param,
	NULL,
} ;


INT32 kdrv_ife_get(UINT32 id, KDRV_IFE_PARAM_ID parm_id, void *p_param)
{
	/*
	ER rt = E_OK;
	UINT32 ign_chk;

	ign_chk = (KDRV_IFE_IGN_CHK & parm_id);
	parm_id = parm_id & (~KDRV_IFE_IGN_CHK);
	if (kdrv_ife_get_fp[parm_id] != NULL) {
	    rt = kdrv_ife_get_fp[parm_id](KDRV_DEV_ID_CHANNEL(id), p_param);
	}
	return rt;
	*/
	ER rt = E_OK;
	KDRV_IFE_HANDLE *p_handle;
	UINT32 ign_chk;
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);

	p_handle = &g_kdrv_ife_handle_tab[channel];

	if (KDRV_DEV_ID_ENGINE(id) != KDRV_VIDEOPROCS_IFE_ENGINE0) {
		DBG_ERR("Invalid engine 0x%x\r\n", (unsigned int)KDRV_DEV_ID_ENGINE(id));
		return E_ID;
	}

	if (kdrv_ife_chk_handle(p_handle) == NULL) {
		DBG_ERR("KDRV IFE: illegal handle 0x%.8x\r\n", (unsigned int)&g_kdrv_ife_handle_tab[channel]);
		return E_SYS;
	}

	if ((p_handle->sts & KDRV_IFE_HANDLE_LOCK) != KDRV_IFE_HANDLE_LOCK) {
		DBG_ERR("KDRV IFE: illegal handle sts 0x%.8x\r\n", (unsigned int)p_handle->sts);
		return E_SYS;
	}

	ign_chk = (KDRV_IFE_IGN_CHK & parm_id);
	parm_id = parm_id & (~KDRV_IFE_IGN_CHK);

	if (!ign_chk) {
		kdrv_ife_lock(p_handle, TRUE);
	}
	if (kdrv_ife_get_fp[parm_id] != NULL) {
		rt = kdrv_ife_get_fp[parm_id](KDRV_DEV_ID_CHANNEL(id), p_param);
	}
	if (!ign_chk) {
		kdrv_ife_lock(p_handle, FALSE);
	}
	return rt;
}

/*INT32 kdrv_ife_get(UINT32 id, KDRV_IFE_PARAM_ID parm_id, void *p_param)
{
    ER rt = E_OK;

    UINT32 ign_chk;

    ign_chk = (KDRV_IFE_IGN_CHK & parm_id);
    parm_id = parm_id & (~KDRV_IFE_IGN_CHK);
    if (kdrv_ife_get_fp[parm_id] != NULL) {
        rt = kdrv_ife_get_fp[parm_id](KDRV_DEV_ID_CHANNEL(id), p_param);
    }

    return rt;
}
EXPORT_SYMBOL(kdrv_ife_get);
*/
/*
ER kdrv_ife_get_org(UINT32 hdl, KDRV_IFE_PARAM_ID item, void* data)
{
    ER rt = E_OK;
    KDRV_IFE_HANDLE* p_handle;
    UINT32 ign_chk;

    p_handle = (KDRV_IFE_HANDLE *)hdl;
    if (kdrv_ife_chk_handle(p_handle) == NULL) {
        DBG_ERR("KDRV IFE: illegal handle 0x%.8x\r\n", hdl);
        return E_SYS;
    }

    if ((p_handle->sts & KDRV_IFE_HANDLE_LOCK) != KDRV_IFE_HANDLE_LOCK) {
        DBG_ERR("KDRV IFE: illegal handle sts 0x%.8x\r\n", p_handle->sts);
        return E_SYS;
    }

    ign_chk = (KDRV_IFE_IGN_CHK & item);
    item = item & (~KDRV_IFE_IGN_CHK);

    if (!ign_chk) {
        kdrv_ife_lock(p_handle, TRUE);
    }
    if (kdrv_ife_get_fp[item] != NULL) {
        rt = kdrv_ife_get_fp[item](p_handle->entry_id, data);
    }
    if (!ign_chk) {
        kdrv_ife_lock(p_handle, FALSE);
    }
    return rt;
}
EXPORT_SYMBOL(kdrv_ife_get);
*/


static char *kdrv_ife_dump_str_in_src(KDRV_IFE_IN_MODE in_src)
{
	char *str_in_src[3 + 1] = {
		"D2D",
		"IPP",
		"Direct",
		"Error",
	};

	if (in_src >= 3) {
		return str_in_src[3];
	}
	return str_in_src[in_src];
}

static char *kdrv_ife_dump_str_in_fmt(KDRV_IFE_IN_FMT in_fmt)
{
	char *str_in_fmt[2 + 1] = {
		"Bayer",
		"RGBIr",
		"Error",
	};

	if (in_fmt >= 2) {
		return str_in_fmt[2];
	}
	return str_in_fmt[in_fmt];
}

static char *kdrv_ife_dump_str_st_pix(KDRV_IPP_PIX st_pix)
{
	char *str_st_pix[4 + 1 + 8 + 1] = {
		"RGGB_R",
		"RGGB_GR",
		"RGGB_GB",
		"RGGB_B",
		"ERROR",
		"RGBIR_PIX_RG_GI",
		"RGBIR_PIX_GB_IG",
		"RGBIR_PIX_GI_BG",
		"RGBIR_PIX_IG_GR",
		"RGBIR_PIX_BG_GI",
		"RGBIR_PIX_GR_IG",
		"RGBIR_PIX_GI_RG",
		"RGBIR_PIX_IG_GB",
		"ERROR",
	};


	if (st_pix >= 13) {
		return str_st_pix[13];
	}
	return str_st_pix[st_pix];
}

static char *kdrv_ife_dump_str_bit(KDRV_IFE_RAW_BIT bit)
{
	char *str_bit[4 + 1] = {
		"8",
		"10",
		"12",
		"16",
		"ERROR",
	};

	if (bit >= 4) {
		return str_bit[4];
	}
	return str_bit[bit];
}

#if defined __UITRON || defined __ECOS
void kdrv_ife_dump_info(void (*dump)(char *fmt, ...))
#else
void kdrv_ife_dump_info(int (*dump)(const char *fmt, ...))
#endif
{
	//BOOL is_print_header;
	BOOL is_open[kdrv_ife_channel_num];
	UINT32 id;

	if (g_kdrv_ife_init_flg == 0 || kdrv_ife_open_cnt == 0) {
		dump("\r\n[IFE] not open\r\n");
		return;
	}

	for (id = 0; id < kdrv_ife_channel_num; id ++) {
		is_open[id] = FALSE;
	}

	for (id = 0; id < kdrv_ife_channel_num; id++) {
		if (g_kdrv_ife_init_flg & (1 << id)) {
			is_open[id] = TRUE;
			kdrv_ife_lock(kdrv_ife_entry_id_conv2_handle(id), TRUE);
		}
	}

	dump("\r\n[KDRV IFE]\r\n");
	dump("\r\n-----ctrl info-----\r\n");
	dump("%12s%12s\r\n", "init_flg", "trig_id");
	dump("%#12x 0x%.8x\r\n", (unsigned int)g_kdrv_ife_init_flg, (unsigned int)g_kdrv_ife_trig_hdl);

	/**
	    input info
	*/
	dump("\r\n-----input info-----\r\n");
	dump("%3s%12s%12s%12s%12s%12s%12s%12s%12s%12s\r\n", "id", "Addr1", "Addr2", "in_src", "type",
		 "st_pix", "bit", "width", "height", "line_ofs1", "line_ofs2");
	for (id = 0; id < kdrv_ife_channel_num; id++) {
		if (!is_open[id]) {
			continue;
		}
		dump("%3d%#12x%#12x%12s%12s%12s%12s%12d%12d%12d%12d\r\n",
			 id,
			 g_kdrv_ife_info[id].in_pixel_addr_1,
			 g_kdrv_ife_info[id].in_pixel_addr_2,
			 kdrv_ife_dump_str_in_src(g_kdrv_ife_info[id].in_img_info.in_src),
			 kdrv_ife_dump_str_in_fmt(g_kdrv_ife_info[id].in_img_info.type),
			 kdrv_ife_dump_str_st_pix(g_kdrv_ife_info[id].in_img_info.img_info.st_pix),
			 kdrv_ife_dump_str_bit(g_kdrv_ife_info[id].in_img_info.img_info.bit),
			 g_kdrv_ife_info[id].in_img_info.img_info.width,
			 g_kdrv_ife_info[id].in_img_info.img_info.height,
			 g_kdrv_ife_info[id].line_ofs_1,
			 g_kdrv_ife_info[id].line_ofs_2);
	}

	/**
	    ouput info
	*/
	dump("\r\n-----output info-----\r\n");
	dump("%3s%12s%12s%12s\r\n", "id", "Addr", "bit", "line_ofs");
	for (id = 0; id < kdrv_ife_channel_num; id++) {
		if (!is_open[id]) {
			continue;
		}
		dump("%3d%#12x%12s%12d\r\n",
			 id,
			 g_kdrv_ife_info[id].out_pixel_addr,
			 kdrv_ife_dump_str_bit(g_kdrv_ife_info[id].out_img_info.bit),
			 g_kdrv_ife_info[id].out_img_info.line_ofs);
	}

	/**
	    interrupt info
	*/
	dump("\r\n-----interrupt info-----\r\n");
	dump("%3s%12s\r\n", "id", "inte_en:");
	for (id = 0; id < kdrv_ife_channel_num; id++) {
		if (!is_open[id]) {
			continue;
		}
		dump("%3d", id);

		if (g_kdrv_ife_info[id].inte_en & KDRV_IFE_INT_FMD) {
			dump("%12s\r\n", "FMD");
		} else {
			dump("%12s\r\n", "CLR");
		}
	}

	/**
	    IQ function enable info
	*/
	dump("\r\n-----iq function enable info-----\r\n");
	dump("%3s%8s%8s%8s%8s%8s%8s%8s%8s%8s\r\n", "id", "NRS", "FCURVE", "OUTL", "FILTER", "CGAIN", "VIG", "GBAL");
	for (id = 0; id < kdrv_ife_channel_num; id++) {
		if (!is_open[id]) {
			continue;
		}
		dump("%3d%8d%8d%8d%8d%8d%8d%8d\r\n",
			 id,
			 g_kdrv_ife_info[id].par_nrs.enable,
			 g_kdrv_ife_info[id].par_fcurve.enable,
			 g_kdrv_ife_info[id].par_outl.enable,
			 g_kdrv_ife_info[id].par_filter.enable,
			 g_kdrv_ife_info[id].par_cgain.enable,
			 g_kdrv_ife_info[id].par_vig.enable,
			 g_kdrv_ife_info[id].par_gbal.enable);
	}

	/**
	    Fusion function enable info
	*/
	dump("\r\n-----fusion function enable info-----\r\n");
	dump("%3s%8s%8s%8s%8s\r\n", "id", "Fusion", "FNUM", "Fu_cgain", "S_comp");
	for (id = 0; id < kdrv_ife_channel_num; id++) {
		if (!is_open[id]) {
			continue;
		}
		dump("%3d%8d%8d%8d%8d\r\n",
			 id,
			 g_kdrv_ife_info[id].info_fusion.enable,
			 g_kdrv_ife_info[id].info_fusion.fnum,
			 g_kdrv_ife_info[id].par_fusion.fu_cgain.enable,
			 g_kdrv_ife_info[id].par_fusion.s_comp.enable);
	}

	for (id = 0; id < kdrv_ife_channel_num; id++) {
		if (is_open[id]) {
			kdrv_ife_lock(kdrv_ife_entry_id_conv2_handle(id), FALSE);
		}
	}

}

#ifdef __KERNEL__
EXPORT_SYMBOL(kdrv_ife_open);
EXPORT_SYMBOL(kdrv_ife_close);
EXPORT_SYMBOL(kdrv_ife_pause);
EXPORT_SYMBOL(kdrv_ife_trigger);
EXPORT_SYMBOL(kdrv_ife_set);
EXPORT_SYMBOL(kdrv_ife_get);
EXPORT_SYMBOL(kdrv_ife_dump_info);
EXPORT_SYMBOL(kdrv_ife_init);
EXPORT_SYMBOL(kdrv_ife_uninit);
EXPORT_SYMBOL(kdrv_ife_buf_query);
EXPORT_SYMBOL(kdrv_ife_buf_init);
EXPORT_SYMBOL(kdrv_ife_buf_uninit);
#endif

