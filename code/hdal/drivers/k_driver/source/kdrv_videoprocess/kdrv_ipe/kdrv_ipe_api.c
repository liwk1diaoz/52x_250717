/**
    @file       kdrv_ipe.c
    @ingroup    Predefined_group_name

    @brief      ipe device abstraction layer

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

// NOTE: XXXXX
#include "ipe_platform.h"
#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kdrv_videoprocess/kdrv_ipe.h"
#include "kdrv_ipe_int.h"

// NOTE: XXXXX
#if 0
#if defined __UITRON || defined __ECOS

#define __MODULE__    kdrv_ipe
#define __DBGLVL__    2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*" // *=All, [mark1]=CustomClass
#include "DebugModule.h"
#else
#include "ipe_dbg.h"
#include <linux/kernel.h>
#endif
#endif

// NOTE: XXXXX
#if 0
static DEFINE_SPINLOCK(my_lock);

#define loc_cpu(myflags)   spin_lock_irqsave(&my_lock, myflags)
#define unl_cpu(myflags)   spin_unlock_irqrestore(&my_lock, myflags)
#endif

#define IQ_DBG_NONE             0x00000000
#define IQ_DBG_LCE              0x00000001
#define IQ_DBG_EDGEGAM          0x00000002
#define IQ_DBG_RGBLPF           0x00000004

static UINT32 g_kdrv_ipe_init_flg = 0;
static KDRV_IPE_HANDLE *g_kdrv_ipe_trig_hdl = NULL;
#if 0
static KDRV_IPE_HANDLE g_kdrv_ipe_handle_tab[KDRV_IPE_HANDLE_MAX_NUM];
KDRV_IPE_PRAM g_kdrv_ipe_info[KDRV_IPE_HANDLE_MAX_NUM];
static UINT32 g_kdrv_ipe_ipl_func_en_sel[KDRV_IPE_HANDLE_MAX_NUM] = {0};
static UINT32 g_kdrv_ipe_load_flg[KDRV_IPE_HANDLE_MAX_NUM][KDRV_IPE_PARAM_MAX] = {0};
KDRV_IPE_IQ_EN_PARAM g_kdrv_ipe_iq_en_info[KDRV_IPE_HANDLE_MAX_NUM];
#else
UINT32 g_kdrv_ipe_channel_num = 0;
UINT32 kdrv_ipe_lock_chls = 0;
KDRV_IPE_PRAM *g_kdrv_ipe_info;
static KDRV_IPE_IQ_EN_PARAM *g_kdrv_ipe_iq_en_info;
static UINT32 *g_kdrv_ipe_ipl_func_en_sel;
static KDRV_IPE_HANDLE *g_kdrv_ipe_handle_tab;
static UINT32 **g_kdrv_ipe_load_flg;
#endif

#define IPE_DRAM_GAMMA_WORD (208)
#define IPE_DRAM_3DLUT_WORD (912)
#define IPE_DRAM_YCURVE_WORD (80)

//UINT32 *p_buffer = NULL;
UINT32 *p_gamma_buffer = NULL;
UINT32 *p_ycurve_buffer = NULL;
//UINT32 *p_3dcc_buffer = NULL;

// internal use
//extern UINT32 *p_gamma_buffer;
//extern UINT32 *p_ycurve_buffer;
//extern UINT32 *p_3dcc_buffer;



//open
static UINT32 g_kdrv_ipe_open_cnt = 0;
static KDRV_IPE_OPENCFG g_kdrv_ipe_open_cfg = {480};
static INT16 g_cc_coef[KDRV_IPE_COEF_LEN] = {0};
static UINT8 fstab_init[KDRV_IPE_FTAB_LEN] = {0};
static UINT8 fdtab_init[KDRV_IPE_FTAB_LEN]  = {0};
static INT16 g_cc_cvt_range[3][KDRV_IPE_COEF_LEN] = {
	{358, 256, 0, -183, 256, -88, 0, 256, 452}, //Full range
	{409, 298, 0, -209, 298, -101, 0, 298, 517},//BT601
	{460, 298, 0, -137, 298, -55, 0, 298, 541}  //BT709
};
static UINT8 dfg_interp_diff_init[17] = {63, 58, 43, 27, 13, 6, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static UINT8 dfg_in_bld_init[17] = {0, 32, 64, 96, 128, 160, 192, 224, 255};
static UINT16 dfg_fog_mod_init[17] = {0, 64, 128, 192, 256, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1023};
static UINT16 dfg_fog_target_init[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static UINT8 dfg_outbld_luma_init[17] = {192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192};
static UINT8 dfg_outbld_diff_init[17] = {0, 0, 24, 36, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48};
static INT16 cst_coef_init[9] = {77, 150, 29, -43, -85, 128, 128, -107, -21 };
static INT16 edge_ker_init[12] = {380, 42, -80, -30, -35, 4, 10, 10, 4, 1, 10, 6};
//static UINT8 edir_tab_init[8] = {8, 8, 8, 8, 8, 8, 8, 8};
static UINT32 esmap_init[16] = {64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 52, 44, 38, 32, 32};
static UINT16 gamma_day_init[129] = {
	0, 37, 73, 107, 139, 169, 198, 225, 241, 257,
	273, 289, 304, 318, 333, 347, 360, 373, 386, 399,
	411, 422, 433, 444, 455, 465, 475, 484, 493, 502,
	510, 518, 525, 532, 539, 546, 553, 560, 566, 573,
	580, 587, 594, 601, 607, 614, 621, 627, 634, 640,
	647, 653, 659, 666, 672, 678, 684, 691, 697, 703,
	709, 715, 721, 727, 733, 739, 744, 750, 756, 762,
	767, 772, 777, 782, 787, 792, 797, 801, 806, 811,
	816, 821, 826, 830, 835, 840, 845, 849, 854, 859,
	863, 868, 872, 877, 882, 886, 891, 895, 900, 904,
	908, 913, 917, 921, 926, 930, 934, 939, 943, 947,
	951, 955, 959, 964, 968, 972, 976, 980, 984, 988,
	992, 996, 1000, 1003, 1007, 1011, 1015, 1019, 1023
};
UINT32 y_curve_init[129] = { //linear
	0, 8, 16, 24, 32, 40, 48, 56, 64, 72,
	80, 88, 96, 104, 112, 120, 128, 136, 144, 152,
	160, 168, 176, 184, 192, 200, 208, 216, 224, 232,
	240, 248, 256, 264, 272, 280, 288, 296, 304, 312,
	320, 328, 336, 344, 352, 360, 368, 376, 384, 392,
	400, 408, 416, 424, 432, 440, 448, 456, 464, 472,
	480, 488, 496, 504, 512, 520, 528, 536, 544, 552,
	560, 568, 576, 584, 592, 600, 608, 616, 624, 632,
	640, 648, 656, 664, 672, 680, 688, 696, 704, 712,
	720, 728, 736, 744, 752, 760, 768, 776, 784, 792,
	800, 808, 816, 824, 832, 840, 848, 856, 864, 872,
	880, 888, 896, 904, 912, 920, 928, 936, 944, 952,
	960, 968, 976, 984, 992, 1000, 1008, 1016, 1023
};
UINT16 tone_remap_init[65] = {
	0, 40, 80, 128, 176, 224, 264, 300, 340, 376,
	412, 452, 480, 504, 528, 548, 572, 596, 620, 640,
	660, 676, 688, 700, 712, 728, 740, 752, 768, 780,
	788, 796, 804, 812, 816, 824, 832, 840, 844, 852,
	860, 868, 872, 880, 888, 892, 900, 904, 912, 920,
	924, 932, 940, 944, 952, 960, 964, 972, 980, 984,
	992, 1000, 1008, 1016, 1023,
};
UINT32 tone_remap_init_mod1[65] = {
	0, 100, 145, 182, 215, 244, 272, 298, 322, 338, 354,
	369, 384, 400, 414, 429, 443, 457, 470, 484, 497, 509,
	521, 532, 543, 553, 564, 575, 586, 596, 607, 618, 630,
	642, 655, 667, 680, 693, 705, 718, 731, 743, 756, 768,
	781, 793, 806, 818, 830, 843, 855, 867, 879, 891, 904,
	916, 928, 940, 952, 963, 975, 987, 999, 1011, 1023
};

static UINT32 ccontab_init[17] = {128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128};

// IPE DRV structure
static IPE_MODEINFO g_ipe = {0};
IPE_EEXT_PARAM drv_eext_param = {0};
IPE_EPROC_PARAM drv_eproc_param = {0};
IPE_RGBLPF_PARAM drv_rgb_lpf_param;
IPE_CC_PARAM drv_cc_param = {0};
IPE_CST_PARAM drv_cst_param = {0};
IPE_CCTRL_PARAM drv_cctrl_param = {0};
IPE_CADJ_EENH_PARAM drv_cadj_eenh_param = {0};
IPE_CADJ_EINV_PARAM drv_cadj_einv_param = {0};
IPE_CADJ_YCCON_PARAM drv_cadj_yccon_param = {0};
IPE_CADJ_COFS_PARAM drv_cadj_cofs_param = {0};
IPE_CADJ_RAND_PARAM drv_cadj_rand_param = {0};
IPE_CADJ_FIXTH_PARAM drv_cadj_fixth_param = {0};
IPE_CADJ_MASK_PARAM drv_cadj_mask_param = {0};
IPE_CSTP_PARAM drv_cstp_param = {0};
IPE_GAMYRAND drv_gamma_ycurve_rand = {0};
IPE_DEFOGROUND drv_defog_round_param = {0};
IPE_ETH_PARAM drv_eth = {0};
IPE_PFR_PARAM drv_pfr_param = {0};
IPE_DEFOG_SUBIMG_PARAM drv_defog_subimg_param = {0};
IPE_DEFOG_PARAM drv_defog_param = {0};
IPE_LCE_PARAM drv_lce_param = {0};
IPE_YCURVE_PARAM drv_ycurve_param = {0};
IPE_VA_FLTR_GROUP_PARAM drv_va_filtg1_param = {0};
IPE_VA_FLTR_GROUP_PARAM drv_va_filtg2_param = {0};
IPE_VA_WIN_PARAM drv_va_win_param;
IPE_EDGE_REGION_PARAM drv_edge_region_str_param = {0};
INT16 g_drv_edge_ker[KDRV_IPE_EDGE_KER_DIV_LEN] = {0};
UINT32 g_drv_tone_map_lut[KDRV_IPE_TONE_MAP_LUT_LEN] = {0};
UINT8 g_drv_edge_map_lut[KDRV_IPE_EDGE_MAP_LUT_LEN] = {0};
UINT8 g_drv_es_map_lut[KDRV_IPE_ES_MAP_LUT_LEN] = {0};
IPE_EDGEMAP_INFOR g_drv_edge_map_th;
IPE_ESMAP_INFOR g_drv_es_map_th;
UINT32 g_drv_pfr_luma_lut[KDRV_IPE_PFR_LUMA_LEN] = {0};
INT16 g_drv_coef[KDRV_IPE_COEF_LEN] = {0};
UINT8 g_drv_fstab[KDRV_IPE_FTAB_LEN] = {0};
UINT8 g_drv_fdtab[KDRV_IPE_FTAB_LEN] = {0};
INT16 g_drv_cst_coef[KDRV_IPE_COEF_LEN] = {0};
UINT8 g_drv_hue_tab[KDRV_IPE_CCTRL_TAB_LEN] = {0};
INT32 g_drv_sat_tab[KDRV_IPE_CCTRL_TAB_LEN] = {0};
INT32 g_drv_int_tab[KDRV_IPE_CCTRL_TAB_LEN] = {0};
UINT8 g_drv_edge_tab[KDRV_IPE_CCTRL_TAB_LEN] = {0};
UINT8 g_drv_dds_tab[KDRV_IPE_DDS_TAB_LEN] = {0};
UINT32 g_drv_ccon_tab[KDRV_IPE_CCONTAB_LEN] = {0};
STR_YTH1_INFOR g_drv_yth1;
STR_YTH2_INFOR g_drv_yth2;
STR_CTH_INFOR g_drv_cth;
//IPE_3DCCROUND g_drv_cc3d_rnd;
UINT8 g_drv_subimg_filt_coeff[3] = {0};
UINT8 g_drv_dfg_interp_diff[KDRV_DFG_INTERP_DIFF_LEN] = {0};
UINT8 g_drv_dfg_inbld[KDRV_DFG_INPUT_BLD_LEN] = {0};
UINT16 g_drv_dfg_env_airlight[KDRV_DFG_AIRLIGHT_NUM] = {0};
UINT16 g_drv_dfg_env_fog_mod_lut[KDRV_DFG_FOG_MOD_LEN] = {0};
UINT16 g_drv_dfg_str_target_lut[KDRV_DFG_TARGET_LEN] = {0};
UINT8 g_drv_dfg_outbld_diff_lut[KDRV_DFG_OUTPUT_BLD_LEN] = {0};
UINT8 g_drv_dfg_outbld_luma_lut[KDRV_DFG_OUTPUT_BLD_LEN] = {0};
UINT8 g_drv_lce_luma_lut[KDRV_LCE_LUMA_LEN] = {0};



static BOOL kdrv_ipe_check_align(UINT32 chk_value, UINT32 align)
{
	return ((chk_value % align) == 0) ? TRUE : FALSE;
}

static BOOL kdrv_ipe_check_noncache_addr(UINT32 chk_addr)
{
	//return !(dma_isCacheAddr(chk_addr));
	return TRUE;
}

static IPE_INOUT_FORMAT kdrv_ipe_conv_in_imgfmt(UINT32 fmt)
{
	switch (fmt) {
	case KDRV_IPE_IN_FMT_Y_PACK_UV444:
		return IPE_YCC444;
	case KDRV_IPE_IN_FMT_Y_PACK_UV422:
		return IPE_YCC422;
	case KDRV_IPE_IN_FMT_Y_PACK_UV420:
		return IPE_YCC420;
	case KDRV_IPE_IN_FMT_Y:
		return IPE_YONLY;
	default:
		break;
	}
	DBG_ERR("input fmt error %d\r\n", (int)fmt);
	return IPE_YCC420;
}

static IPE_INOUT_FORMAT kdrv_ipe_conv_out_imgfmt(UINT32 fmt)
{
	switch (fmt) {
	case KDRV_IPE_OUT_FMT_Y_PACK_UV444:
		return IPE_YCC444;
	case KDRV_IPE_OUT_FMT_Y_PACK_UV422:
		return IPE_YCC422;
	case KDRV_IPE_OUT_FMT_Y_PACK_UV420:
		return IPE_YCC420;
	case KDRV_IPE_OUT_FMT_Y:
		return IPE_YONLY;
	default:
		break;
	}
	DBG_ERR("input fmt error %d\r\n", (int)fmt);
	return IPE_YCC420;
}

static UINT32 *kdrv_ipe_iq_gammacurve_conv(UINT32 *r_lut, UINT32 *g_lut, UINT32 *b_lut)
{
	int i, page;
	/*
	    convert input format for IPE DRV format
	*/
	//memset(kdrv_ipe_gamma_buf[Id], 0, sizeof(UINT32)*195);
	page = 0;
	for (i = 0; i < 129; i++) {
		if (i % 2 == 0) {
			p_gamma_buffer[page + i / 2] = r_lut[i];
		} else {
			p_gamma_buffer[page + i / 2] |= (r_lut[i] << 10);
		}
	}

	page = 65;
	for (i = 0; i < 129; i++) {
		if (i % 2 == 0) {
			p_gamma_buffer[page + i / 2] = g_lut[i];
		} else {
			p_gamma_buffer[page + i / 2] |= (g_lut[i] << 10);
		}
	}

	page = 65 * 2;
	for (i = 0; i < 129; i++) {
		if (i % 2 == 0) {
			p_gamma_buffer[page + i / 2] = b_lut[i];
		} else {
			p_gamma_buffer[page + i / 2] |= (b_lut[i] << 10);
		}
	}

	return p_gamma_buffer;
}

static UINT32 *kdrv_ipe_iq_ycurve_conv(UINT32 *y_lut)
{
	int i;
	/*
	    convert input format for IPE DRV format
	*/
	//memset(kdrv_ipe_ycurve_buf[Id], 0, sizeof(UINT32)*195);
	for (i = 0; i < 129; i++) {
		if (i % 2 == 0) {
			p_ycurve_buffer[i / 2] = y_lut[i];
		} else {
			p_ycurve_buffer[i / 2] |= (y_lut[i] << 10);
		}
	}

	return p_ycurve_buffer;
}

static void kdrv_ipe_init_drv_param(void)
{
	drv_eext_param.p_edgeker_b = &g_drv_edge_ker[0];
	drv_eext_param.p_tonemap_lut = &g_drv_tone_map_lut[0];
	drv_eproc_param.p_edgemap_th = &g_drv_edge_map_th;
	drv_eproc_param.p_edgemap_lut = &g_drv_edge_map_lut[0];
	drv_eproc_param.p_esmap_th = &g_drv_es_map_th;
	drv_eproc_param.p_esmap_lut = &g_drv_es_map_lut[0];

	drv_pfr_param.p_luma_lut = &g_drv_pfr_luma_lut[0];

	drv_cc_param.p_cc_coeff = &g_drv_coef[0];
	drv_cc_param.p_fdtab = &g_drv_fdtab[0];
	drv_cc_param.p_fstab = &g_drv_fstab[0];

	drv_cst_param.p_cst_coeff = &g_drv_cst_coef[0];

	drv_cctrl_param.p_hue_tab = &g_drv_hue_tab[0];
	drv_cctrl_param.p_sat_tab = &g_drv_sat_tab[0];
	drv_cctrl_param.p_int_tab = &g_drv_int_tab[0];
	drv_cctrl_param.p_edge_tab = &g_drv_edge_tab[0];
	drv_cctrl_param.p_dds_tab = &g_drv_dds_tab[0];

	drv_cadj_fixth_param.p_cth = &g_drv_cth;
	drv_cadj_fixth_param.p_yth1 = &g_drv_yth1;
	drv_cadj_fixth_param.p_yth2 = &g_drv_yth2;

	drv_cadj_yccon_param.p_ccon_tab = &g_drv_ccon_tab[0];

	drv_defog_subimg_param.p_subimg_ftrcoef = &g_drv_subimg_filt_coeff[0];
	drv_defog_param.scalup_param.p_interp_diff_lut = &g_drv_dfg_interp_diff[0];
	drv_defog_param.input_bld.p_in_blend_wt = &g_drv_dfg_inbld[0];
	drv_defog_param.env_estimation.p_dfg_airlight = &g_drv_dfg_env_airlight[0];
	drv_defog_param.env_estimation.p_fog_mod_lut = &g_drv_dfg_env_fog_mod_lut[0];
	drv_defog_param.dfg_strength.p_target_lut = &g_drv_dfg_str_target_lut[0];
	drv_defog_param.dfg_outbld.p_outbld_diff_wt = &g_drv_dfg_outbld_diff_lut[0];
	drv_defog_param.dfg_outbld.p_outbld_lum_wt = &g_drv_dfg_outbld_luma_lut[0];

	drv_lce_param.p_lum_wt_lut = &g_drv_lce_luma_lut[0];

	// set IQ parameters
	g_ipe.iq_info.p_eext_param = &drv_eext_param;
	g_ipe.iq_info.p_eproc_param = &drv_eproc_param;
	g_ipe.iq_info.p_rgblpf_param = &drv_rgb_lpf_param;
	g_ipe.iq_info.p_cc_param = &drv_cc_param;
	g_ipe.iq_info.p_cst_param = &drv_cst_param;
	g_ipe.iq_info.p_cctrl_param = &drv_cctrl_param;
	g_ipe.iq_info.p_cadj_eenh_param = &drv_cadj_eenh_param;
	g_ipe.iq_info.p_cadj_einv_param = &drv_cadj_einv_param;
	g_ipe.iq_info.p_cadj_yccon_param = &drv_cadj_yccon_param;
	g_ipe.iq_info.p_cadj_cofs_param = &drv_cadj_cofs_param;
	g_ipe.iq_info.p_cadj_rand_param = &drv_cadj_rand_param;
	g_ipe.iq_info.p_cadj_fixth_param = &drv_cadj_fixth_param;
	g_ipe.iq_info.p_cadj_mask_param = &drv_cadj_mask_param;
	g_ipe.iq_info.p_cstp_param = &drv_cstp_param;
	g_ipe.iq_info.p_gamycur_rand = &drv_gamma_ycurve_rand;
	g_ipe.iq_info.p_defog_rand = &drv_defog_round_param;
	g_ipe.iq_info.p_eth = &drv_eth;
	g_ipe.iq_info.p_pfr_param = &drv_pfr_param;
	g_ipe.iq_info.p_dfg_subimg_param = &drv_defog_subimg_param;
	g_ipe.iq_info.p_defog_param = &drv_defog_param;
	g_ipe.iq_info.p_lce_param = &drv_lce_param;
	g_ipe.iq_info.p_ycurve_param = &drv_ycurve_param;
	g_ipe.iq_info.p_va_filt_g1 = &drv_va_filtg1_param;
	g_ipe.iq_info.p_va_filt_g2 = &drv_va_filtg2_param;
	g_ipe.iq_info.p_va_win_info = &drv_va_win_param;

	g_ipe.iq_info.p_edge_region_param = &drv_edge_region_str_param;
}


static void kdrv_ipe_memset_drv_param(void)
{
	memset((void *)&g_ipe, 0x0, sizeof(IPE_MODEINFO));
	memset((void *)&drv_eext_param, 0x0, sizeof(IPE_EEXT_PARAM));
	memset((void *)&drv_eproc_param, 0x0, sizeof(IPE_EPROC_PARAM));
	memset((void *)&drv_rgb_lpf_param, 0x0, sizeof(IPE_RGBLPF_PARAM));
	memset((void *)&drv_cc_param, 0x0, sizeof(IPE_CC_PARAM));
	memset((void *)&drv_cst_param, 0x0, sizeof(IPE_CST_PARAM));
	memset((void *)&drv_cctrl_param, 0x0, sizeof(IPE_CCTRL_PARAM));
	memset((void *)&drv_cadj_eenh_param, 0x0, sizeof(IPE_CADJ_EENH_PARAM));
	memset((void *)&drv_cadj_einv_param, 0x0, sizeof(IPE_CADJ_EINV_PARAM));
	memset((void *)&drv_cadj_yccon_param, 0x0, sizeof(IPE_CADJ_YCCON_PARAM));
	memset((void *)&drv_cadj_cofs_param, 0x0, sizeof(IPE_CADJ_COFS_PARAM));
	memset((void *)&drv_cadj_rand_param, 0x0, sizeof(IPE_CADJ_RAND_PARAM));
	memset((void *)&drv_cadj_fixth_param, 0x0, sizeof(IPE_CADJ_FIXTH_PARAM));
	memset((void *)&drv_cadj_mask_param, 0x0, sizeof(IPE_CADJ_MASK_PARAM));
	memset((void *)&drv_cstp_param, 0x0, sizeof(IPE_CSTP_PARAM));
	memset((void *)&drv_gamma_ycurve_rand, 0x0, sizeof(IPE_GAMYRAND));
	memset((void *)&drv_defog_round_param, 0x0, sizeof(IPE_DEFOGROUND));
	memset((void *)&drv_eth, 0x0, sizeof(IPE_ETH_PARAM));
	memset((void *)&drv_pfr_param, 0x0, sizeof(IPE_PFR_PARAM));
	memset((void *)&drv_defog_subimg_param, 0x0, sizeof(IPE_DEFOG_SUBIMG_PARAM));
	memset((void *)&drv_defog_param, 0x0, sizeof(IPE_DEFOG_PARAM));
	memset((void *)&drv_lce_param, 0x0, sizeof(IPE_LCE_PARAM));
	memset((void *)&drv_ycurve_param, 0x0, sizeof(IPE_YCURVE_PARAM));
	memset((void *)&drv_va_filtg1_param, 0x0, sizeof(IPE_VA_FLTR_GROUP_PARAM));
	memset((void *)&drv_va_filtg2_param, 0x0, sizeof(IPE_VA_FLTR_GROUP_PARAM));
	memset((void *)&drv_va_win_param, 0x0, sizeof(IPE_VA_WIN_PARAM));

	memset((void *)&drv_edge_region_str_param, 0x0, sizeof(IPE_EDGE_REGION_PARAM));

	kdrv_ipe_init_drv_param();

}

static ER kdrv_ipe_init_param(INT32 id)
{
	ER rt = E_OK;
	UINT32 i = 0;

	memset(&g_kdrv_ipe_info[id], 0, sizeof(g_kdrv_ipe_info[id]));
	memset(&g_kdrv_ipe_iq_en_info[id], 0, sizeof(g_kdrv_ipe_iq_en_info[id]));
	g_kdrv_ipe_ipl_func_en_sel[id] = 0;

	g_kdrv_ipe_info[id].dram_out_mode = KDRV_IPE_DRAM_OUT_NORMAL;
	g_kdrv_ipe_info[id].single_out_ch.single_out_y_en = 1;
	g_kdrv_ipe_info[id].single_out_ch.single_out_c_en = 1;
	g_kdrv_ipe_info[id].single_out_ch.single_out_va_en = 1;
	g_kdrv_ipe_info[id].single_out_ch.single_out_defog_en = 1;

	//init regular parameter setting
	//edge
	g_kdrv_ipe_info[id].eext_tonemap_param.gamma_sel = KDRV_IPE_EEXT_POST_GAM;
	//g_kdrv_ipe_info[id].eext_param.edge_ch_sel = KDRV_IPE_EEXT_RGBIN;
	for (i = 0; i < KDRV_IPE_EDGE_KER_DIV_LEN ; i++) {
		g_kdrv_ipe_info[id].eext_param.edge_ker[i] = edge_ker_init[i];
	}

	//g_kdrv_ipe_info[id].eext_param.dir_edge_ker = 2;
	//for (i = 0; i < 8 ; i++) {
	//  g_kdrv_ipe_info[id].eext_param.dir_edge_wtbl[i] = edir_tab_init[i];
	//}
	for (i = 0; i < 65 ; i++) {
		g_kdrv_ipe_info[id].eext_tonemap_param.tone_map_lut[i] = (UINT32)tone_remap_init[i];
	}
#if 1
	g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq0.eext_enh = 24;
	g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq0.eext_div = 3;
	g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq1.eext_enh = 24;
	g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq1.eext_div = 3;
	g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq2.eext_enh = 24;
	g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq2.eext_div = 3;

	g_kdrv_ipe_info[id].eext_param.eext_engcon.eext_div_con = 0;
	g_kdrv_ipe_info[id].eext_param.eext_engcon.eext_div_eng = 0;
	g_kdrv_ipe_info[id].eext_param.eext_engcon.wt_con_eng = 4;

	g_kdrv_ipe_info[id].eext_param.ker_thickness.wt_ker_thin = 0;
	g_kdrv_ipe_info[id].eext_param.ker_thickness.wt_ker_robust = 0;
	g_kdrv_ipe_info[id].eext_param.ker_thickness.iso_ker_thin = 8;
	g_kdrv_ipe_info[id].eext_param.ker_thickness.iso_ker_robust = 0;

	g_kdrv_ipe_info[id].eext_param.ker_thickness_hld.wt_ker_thin = 0;
	g_kdrv_ipe_info[id].eext_param.ker_thickness_hld.wt_ker_robust = 0;
	g_kdrv_ipe_info[id].eext_param.ker_thickness_hld.iso_ker_thin = 8;
	g_kdrv_ipe_info[id].eext_param.ker_thickness_hld.iso_ker_robust = 0;

	g_kdrv_ipe_info[id].eext_param.eext_region.reg_wt.wt_low = 0;
	g_kdrv_ipe_info[id].eext_param.eext_region.reg_wt.wt_high = 8;
	g_kdrv_ipe_info[id].eext_param.eext_region.reg_wt.wt_low_hld = 0;
	g_kdrv_ipe_info[id].eext_param.eext_region.reg_wt.wt_high_hld = 8;
	g_kdrv_ipe_info[id].eext_param.eext_region.reg_th.th_flat = 40;
	g_kdrv_ipe_info[id].eext_param.eext_region.reg_th.th_edge = 240;
	g_kdrv_ipe_info[id].eext_param.eext_region.reg_th.th_flat_hld = 40;
	g_kdrv_ipe_info[id].eext_param.eext_region.reg_th.th_edge_hld = 240;
	g_kdrv_ipe_info[id].eext_param.eext_region.reg_th.th_lum_hld = 600;

	g_kdrv_ipe_info[id].overshoot_param.overshoot_en = 1;
	g_kdrv_ipe_info[id].overshoot_param.wt_overshoot = 128;
	g_kdrv_ipe_info[id].overshoot_param.th_overshoot = 128;
	g_kdrv_ipe_info[id].overshoot_param.slope_overshoot = 96;

	g_kdrv_ipe_info[id].overshoot_param.wt_undershoot = 128;
	g_kdrv_ipe_info[id].overshoot_param.th_undershoot = 0;
	g_kdrv_ipe_info[id].overshoot_param.slope_undershoot = 256;

	g_kdrv_ipe_info[id].overshoot_param.th_undershoot_lum = 128;
	g_kdrv_ipe_info[id].overshoot_param.th_undershoot_eng = 20;
	g_kdrv_ipe_info[id].overshoot_param.slope_undershoot_lum = 96;
	g_kdrv_ipe_info[id].overshoot_param.slope_undershoot_eng = 96;
	g_kdrv_ipe_info[id].overshoot_param.clamp_wt_mod_lum = 5;
	g_kdrv_ipe_info[id].overshoot_param.clamp_wt_mod_eng = 5;
	g_kdrv_ipe_info[id].overshoot_param.strength_lum_eng = 1;
	g_kdrv_ipe_info[id].overshoot_param.norm_lum_eng = 0;

#else
	g_kdrv_ipe_info[id].eext_param.dir_edge_eext.yd_range = (KDRV_IPE_YDRANGE)1;
	g_kdrv_ipe_info[id].eext_param.dir_edge_eext.yd_max_th = 22;
	g_kdrv_ipe_info[id].eext_param.dir_edge_eext.pnd_adj_en = 0;
	g_kdrv_ipe_info[id].eext_param.dir_edge_eext.pnd_shift = 3;
	g_kdrv_ipe_info[id].eext_param.dir_edge_eext.edge_shift = 2;
	g_kdrv_ipe_info[id].eext_param.dir_edge_eext.ker_sel = (KDRV_IPE_KERSEL)0;

	g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.a0 = (INT8)3;
	g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.b0 = (INT8)(-1);
	g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.c0 = (INT8)(-1);
	g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.d0 = 0;
	g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.a1 = (INT8)5;
	g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.b1 = (INT8)1;
	g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.c1 = (INT8)1;
	g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.d1 = 0;
	g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.a2 = (INT8)3;
	g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.b2 = (INT8)0;
	g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.c2 = (INT8)(-1);
	g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.d2 = 0;

	g_kdrv_ipe_info[id].eext_param.dir_edge_score_th.score_th0 = 7;
	g_kdrv_ipe_info[id].eext_param.dir_edge_score_th.score_th1 = 28;
	g_kdrv_ipe_info[id].eext_param.dir_edge_score_th.score_th2 = 3;
	g_kdrv_ipe_info[id].eext_param.dir_edge_score_th.score_th3 = 21;

	g_kdrv_ipe_info[id].eext_param.power_save_en = 0;

	g_kdrv_ipe_info[id].eext_param.dir_edge_con.score_center = (KDRV_IPE_DECONSCR)0;
	g_kdrv_ipe_info[id].eext_param.dir_edge_con.score_edge = (KDRV_IPE_DECONSCR)1;
	g_kdrv_ipe_info[id].eext_param.dir_edge_con.score_detatil = (KDRV_IPE_DECONSCR)0;
	g_kdrv_ipe_info[id].eext_param.dir_edge_con.cnt_num_th = 2;
	g_kdrv_ipe_info[id].eext_param.dir_edge_con.max1_th = 3;
	g_kdrv_ipe_info[id].eext_param.dir_edge_con.max_cnt_th = 0;
	g_kdrv_ipe_info[id].eext_param.dir_edge_con.max2_th = 7;
	g_kdrv_ipe_info[id].eext_param.dir_edge_con.idx_sel = (KDRV_IPE_DECON_IDXSEL)0;

	g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.ker_sel = (KDRV_IPE_KERSEL)0;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.adwt0 = 8;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.adwt1 = 8;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.adwt2 = 16;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.adwt3 = 16;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.adwt4 = 16;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.adwt5 = 16;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.ads0 = (INT8)3;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.ads1 = (INT8)3;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.ads2 = (INT8)3;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.ads3 = (INT8)3;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.ads4 = (INT8)3;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.ads5 = (INT8)3;

	g_kdrv_ipe_info[id].eext_param.dir_edge_w_b.kel_sel = (KDRV_IPE_EW_KBOUTSEL)1;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w_b.bdwt0 = 1;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w_b.bdwt1 = 1;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w_b.bds0 = (INT8)2;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w_b.bds1 = (INT8)2;

	g_kdrv_ipe_info[id].eext_param.dir_edge_w.dynamic_map_en = (IPE_ENABLE)0;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w.ewd0 = 8;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w.ewd1 = 8;
	g_kdrv_ipe_info[id].eext_param.dir_edge_w.ewd2 = 8;
#endif
	g_kdrv_ipe_info[id].eproc_param.edge_map_th.ethr_low = 0;
	g_kdrv_ipe_info[id].eproc_param.edge_map_th.ethr_high = 256;
	g_kdrv_ipe_info[id].eproc_param.edge_map_th.etab_low = 0;
	g_kdrv_ipe_info[id].eproc_param.edge_map_th.etab_high = 0;
	g_kdrv_ipe_info[id].eproc_param.es_map_th.ethr_low = 0;
	g_kdrv_ipe_info[id].eproc_param.es_map_th.ethr_high = 512;
	g_kdrv_ipe_info[id].eproc_param.es_map_th.etab_low = 5;
	g_kdrv_ipe_info[id].eproc_param.es_map_th.etab_high = 5;
	for (i = 0; i < KDRV_IPE_EDGE_MAP_LUT_LEN; i++) {
		g_kdrv_ipe_info[id].eproc_param.es_map_lut[i] = esmap_init[i];
		g_kdrv_ipe_info[id].eproc_param.edge_map_lut[i] = 255;
	}
	g_kdrv_ipe_info[id].rgb_lpf_param.enable = 0;
	g_kdrv_ipe_info[id].pfr_param.enable = 0;
	g_kdrv_ipe_info[id].cc_en_param.enable = 0;
	g_kdrv_ipe_info[id].cc_param.cc2_sel = KDRV_IPE_CC2_IDENTITY;
	g_kdrv_ipe_info[id].cc_param.cc_stab_sel = KDRV_IPE_CC_MAX;
	//g_kdrv_ipe_info[id].cc_param.cc_ofs_sel = KDRV_CC_OFS_BYPASS;
	for (i = 0; i < KDRV_IPE_FTAB_LEN; i++) {
		g_kdrv_ipe_info[id].cc_param.fstab[i] = fstab_init[i];
		g_kdrv_ipe_info[id].cc_param.fdtab[i] = fdtab_init[i];
	}
	g_kdrv_ipe_info[id].cctrl_en_param.enable = 0;
	g_kdrv_ipe_info[id].cadj_ee_param.enable = 0;
	g_kdrv_ipe_info[id].cadj_ee_param.edge_enh_p = 80;
	g_kdrv_ipe_info[id].cadj_ee_param.edge_enh_n = 80;
	g_kdrv_ipe_info[id].cadj_ee_param.edge_inv_p_en = 0;
	g_kdrv_ipe_info[id].cadj_ee_param.edge_inv_n_en = 0;
	g_kdrv_ipe_info[id].cadj_yccon_param.enable = 0;
	g_kdrv_ipe_info[id].cadj_cofs_param.enable = 1;
	g_kdrv_ipe_info[id].cadj_cofs_param.cb_ofs = 128;
	g_kdrv_ipe_info[id].cadj_cofs_param.cr_ofs = 128;
	g_kdrv_ipe_info[id].cadj_rand_param.enable = 1;
	g_kdrv_ipe_info[id].cadj_hue_param.enable = 0;
	g_kdrv_ipe_info[id].cadj_fixth_param.enable = 0;
	g_kdrv_ipe_info[id].cadj_mask_param.enable = 1;
	g_kdrv_ipe_info[id].cadj_mask_param.y_mask = 255;
	g_kdrv_ipe_info[id].cadj_mask_param.cb_mask = 255;
	g_kdrv_ipe_info[id].cadj_mask_param.cr_mask = 255;

	g_kdrv_ipe_info[id].cst_param.enable = 1;
	for (i = 0; i < KDRV_IPE_COEF_LEN; i++) {
		g_kdrv_ipe_info[id].cst_param.cst_coef[i] = cst_coef_init[i];
	}
	g_kdrv_ipe_info[id].cst_param.cst_off_sel = CST_MINUS128;

	g_kdrv_ipe_info[id].cstp_param.enable = 0;
	g_kdrv_ipe_info[id].cstp_param.cstp_ratio = 0;

	g_kdrv_ipe_info[id].gamy_rand_param.enable = 0;

	g_kdrv_ipe_info[id].gamma.enable = 1;
	g_kdrv_ipe_info[id].gamma.option = KDRV_IPE_GAMMA_RGB_COMBINE;
	for (i = 0; i < KDRV_IPE_GAMMA_LEN; i++) {
		g_kdrv_ipe_info[id].gamma.gamma_lut[0][i] = (UINT32)gamma_day_init[i];
		g_kdrv_ipe_info[id].gamma.gamma_lut[1][i] = (UINT32)gamma_day_init[i];
		g_kdrv_ipe_info[id].gamma.gamma_lut[2][i] = (UINT32)gamma_day_init[i];
	}

	g_kdrv_ipe_info[id].y_curve.enable = 0;

	//g_kdrv_ipe_info[id]._3dcc_param.enable = 0;
	//g_kdrv_ipe_info[id]._3dcc_param.cc3d_rnd.round_opt = KDRV_IPE_ROUNDING;
	//g_kdrv_ipe_info[id]._3dcc_param.cc3d_rnd.rst_en = 0;

	g_kdrv_ipe_info[id].subimg_param.subimg_size.h_size = 16;
	g_kdrv_ipe_info[id].subimg_param.subimg_size.v_size = 9;
	g_kdrv_ipe_info[id].subimg_param.subimg_ftrcoef[0] = 4;
	g_kdrv_ipe_info[id].subimg_param.subimg_ftrcoef[1] = 2;
	g_kdrv_ipe_info[id].subimg_param.subimg_ftrcoef[2] = 1;
	g_kdrv_ipe_info[id].subimg_param.subimg_lofs_in = 16 << 2;
	g_kdrv_ipe_info[id].subimg_param.subimg_lofs_out = 16 << 2;

	g_kdrv_ipe_info[id].dfg_param.enable = 0;
	g_kdrv_ipe_info[id].dfg_param.scalup_param.interp_wdist = 8;
	g_kdrv_ipe_info[id].dfg_param.scalup_param.interp_wout = 8;
	g_kdrv_ipe_info[id].dfg_param.scalup_param.interp_wcenter = 1;
	g_kdrv_ipe_info[id].dfg_param.scalup_param.interp_wsrc = 26;
	for (i = 0; i < KDRV_DFG_INTERP_DIFF_LEN; i++) {
		g_kdrv_ipe_info[id].dfg_param.scalup_param.interp_diff_lut[i] = dfg_interp_diff_init[i];
	}
	for (i = 0; i < KDRV_DFG_INPUT_BLD_LEN; i++) {
		g_kdrv_ipe_info[id].dfg_param.input_bld.in_blend_wt[i] = dfg_in_bld_init[i];
	}
	g_kdrv_ipe_info[id].dfg_param.env_estimation.dfg_self_comp_en = 0;
	g_kdrv_ipe_info[id].dfg_param.env_estimation.dfg_min_diff = 512;
	g_kdrv_ipe_info[id].dfg_param.env_estimation.dfg_airlight[0] = 1023;
	g_kdrv_ipe_info[id].dfg_param.env_estimation.dfg_airlight[1] = 1023;
	g_kdrv_ipe_info[id].dfg_param.env_estimation.dfg_airlight[2] = 1023;
	for (i = 0; i < KDRV_DFG_FOG_MOD_LEN; i++) {
		g_kdrv_ipe_info[id].dfg_param.env_estimation.fog_mod_lut[i] = dfg_fog_mod_init[i];
	}

	g_kdrv_ipe_info[id].dfg_param.dfg_strength.str_mode_sel = KDRV_DEFOG_METHOD_B;
	g_kdrv_ipe_info[id].dfg_param.dfg_strength.fog_ratio = 255;
	g_kdrv_ipe_info[id].dfg_param.dfg_strength.dgain_ratio = 255;
	g_kdrv_ipe_info[id].dfg_param.dfg_strength.gain_th = 96;
	for (i = 0; i < KDRV_DFG_TARGET_LEN; i++) {
		g_kdrv_ipe_info[id].dfg_param.dfg_strength.target_lut[i] = dfg_fog_target_init[i];
	}

	g_kdrv_ipe_info[id].dfg_param.dfg_outbld.outbld_ref_sel = KDRV_DEFOG_OUTBLD_REF_AFTER;
	g_kdrv_ipe_info[id].dfg_param.dfg_outbld.outbld_local_en = 1;
	for (i = 0; i < KDRV_DFG_OUTPUT_BLD_LEN; i++) {
		g_kdrv_ipe_info[id].dfg_param.dfg_outbld.outbld_lum_wt[i] = dfg_outbld_luma_init[i];
	}
	for (i = 0; i < KDRV_DFG_OUTPUT_BLD_LEN; i++) {
		g_kdrv_ipe_info[id].dfg_param.dfg_outbld.outbld_diff_wt[i] = dfg_outbld_diff_init[i];
	}

	g_kdrv_ipe_info[id].dfg_param.dfg_stcs.airlight_stcs_ratio = 8;

	g_kdrv_ipe_info[id].lce_param.enable = 0;
	g_kdrv_ipe_info[id].lce_param.diff_wt_avg = 128;
	g_kdrv_ipe_info[id].lce_param.diff_wt_pos = 0;
	g_kdrv_ipe_info[id].lce_param.diff_wt_neg = 0;
	g_kdrv_ipe_info[id].lce_param.lum_wt_lut[0] = 64;
	g_kdrv_ipe_info[id].lce_param.lum_wt_lut[1] = 68;
	g_kdrv_ipe_info[id].lce_param.lum_wt_lut[2] = 72;
	g_kdrv_ipe_info[id].lce_param.lum_wt_lut[3] = 85;
	g_kdrv_ipe_info[id].lce_param.lum_wt_lut[4] = 85;
	g_kdrv_ipe_info[id].lce_param.lum_wt_lut[5] = 85;
	g_kdrv_ipe_info[id].lce_param.lum_wt_lut[6] = 85;
	g_kdrv_ipe_info[id].lce_param.lum_wt_lut[7] = 72;
	g_kdrv_ipe_info[id].lce_param.lum_wt_lut[8] = 68;
	g_kdrv_ipe_info[id].edgdbg_param.enable = 0;
	g_kdrv_ipe_info[id].edgdbg_param.mode_sel = KDRV_DBG_EDGE_STRENGTH;

	g_kdrv_ipe_info[id].va_param.enable = 0;
	g_kdrv_ipe_info[id].va_param.indep_va_enable = 0;
	g_kdrv_ipe_info[id].va_win_info.ratio_base = 1000;
	g_kdrv_ipe_info[id].va_win_info.winsz_ratio.w = 100;
	g_kdrv_ipe_info[id].va_win_info.winsz_ratio.h = 100;
	g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[0].x = 0;
	g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[0].y = 0;
	g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[0].w = 100;
	g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[0].h = 100;


	g_kdrv_ipe_info[id].edge_region_param.enable = 0;
	g_kdrv_ipe_info[id].edge_region_param.enh_thin = 0x40;
	g_kdrv_ipe_info[id].edge_region_param.enh_robust = 0x40;
	g_kdrv_ipe_info[id].edge_region_param.slope_flat = 0;
	g_kdrv_ipe_info[id].edge_region_param.str_flat = 0x40;
	g_kdrv_ipe_info[id].edge_region_param.slope_edge = 0;
	g_kdrv_ipe_info[id].edge_region_param.str_edge = 0x40;
	

	g_kdrv_ipe_info[id].subout_dma_addr.enable = 0;

	g_kdrv_ipe_iq_en_info[id].cst_param_enable = 1;

	return rt;
}

static ER kdrv_ipe_check_param(INT32 id)
{
	if (g_kdrv_ipe_info[id].subout_dma_addr.enable || g_kdrv_ipe_info[id].dfg_param.enable  || g_kdrv_ipe_info[id].lce_param.enable) {
		if (g_kdrv_ipe_info[id].subimg_param.subimg_size.h_size < 4) {
			g_kdrv_ipe_info[id].subimg_param.subimg_size.h_size = 16;
			DBG_IND("subimg size h cannot be smaller than 4, modify to 16\r\n");
		}
		if (g_kdrv_ipe_info[id].subimg_param.subimg_size.v_size < 4) {
			g_kdrv_ipe_info[id].subimg_param.subimg_size.v_size = 9;
			DBG_IND("subimg size v cannot be smaller than 4, modify to 9\r\n");
		}
	}
	if (((g_kdrv_ipe_info[id].dfg_param.enable == 1) || (g_kdrv_ipe_info[id].lce_param.enable == 1))
		&& (g_kdrv_ipe_info[id].subimg_param.subimg_lofs_in == 0)) {
		g_kdrv_ipe_info[id].subimg_param.subimg_lofs_in = g_kdrv_ipe_info[id].subimg_param.subimg_size.h_size * 4;
	}
	if ((g_kdrv_ipe_info[id].subout_dma_addr.enable == 1) && (g_kdrv_ipe_info[id].subimg_param.subimg_lofs_out == 0)) {
		g_kdrv_ipe_info[id].subimg_param.subimg_lofs_out = g_kdrv_ipe_info[id].subimg_param.subimg_size.h_size * 4;
	}


	if ((g_kdrv_ipe_info[id].gamma.option == KDRV_IPE_GAMMA_RGB_SEPERATE) &&
		(g_kdrv_ipe_info[id].in_img_info.in_src == KDRV_IPE_IN_SRC_ALL_DIRECT)) {
		g_kdrv_ipe_info[id].gamma.option = KDRV_IPE_GAMMA_RGB_COMBINE;
	}

	return E_OK;
}

static ER kdrv_ipe_set_input_param(INT32 id, IPE_INPUTINFO *info)
{
	ER rt = E_OK;
	// input info
	KDRV_IPE_IN_IMG_INFO in_img_info = g_kdrv_ipe_info[id].in_img_info;
	KDRV_IPE_DMA_ADDR_INFO in_pixel_addr = g_kdrv_ipe_info[id].in_pixel_addr;
	// output info
	KDRV_IPE_OUT_IMG_INFO out_img_info = g_kdrv_ipe_info[id].out_img_info;

	if (in_img_info.in_src == KDRV_IPE_IN_SRC_DIRECT) {
		info->ipe_mode = IPE_OPMODE_IPP;
	} else if (in_img_info.in_src == KDRV_IPE_IN_SRC_ALL_DIRECT) {
		info->ipe_mode = IPE_OPMODE_SIE_IPP;
	} else if (in_img_info.in_src == KDRV_IPE_IN_SRC_DRAM) {
		info->ipe_mode = IPE_OPMODE_D2D;
	} else if (in_img_info.in_src == KDRV_IPE_IN_SRC_DRAM_RGB) {
		info->ipe_mode = IPE_OPMODE_D2D;
	} else {
		info->ipe_mode = IPE_OPMODE_UNKNOWN;
		DBG_ERR("IPE_Mode set error(%d)\r\n", (int)in_img_info.in_src);
		return E_SYS;
	}

	info->in_size_y.h_size = in_img_info.ch.y_width;
	info->in_size_y.v_size = in_img_info.ch.y_height;

	info->addr_in0 = in_pixel_addr.addr_y;
	info->addr_in1 = in_pixel_addr.addr_uv;

	if ((in_img_info.in_src == KDRV_IPE_IN_SRC_DIRECT) || (in_img_info.in_src == KDRV_IPE_IN_SRC_ALL_DIRECT)) {
		info->in_fmt = IPE_YCC444;
		info->out_fmt = IPE_YCC444;
	} else {
		info->in_fmt = kdrv_ipe_conv_in_imgfmt(in_img_info.type);
		info->out_fmt = kdrv_ipe_conv_out_imgfmt(out_img_info.type);
		if ((info->in_fmt == IPE_YONLY) || (info->out_fmt == IPE_YONLY)) {
			if ((info->in_fmt != IPE_YONLY) || (info->out_fmt != IPE_YONLY)) {
				DBG_ERR("Y only format, in/out fmt must same (KDRV_IPE_IN_FMT %d KDRV_IPE_OUT_FMT %d)\r\n", (int)in_img_info.type, (int)out_img_info.type);
				info->in_fmt = IPE_YONLY;
				info->out_fmt = IPE_YONLY;
			}
		}
	}

	info->lofs_in0 = in_img_info.ch.y_line_ofs;
	info->lofs_in1 = in_img_info.ch.uv_line_ofs;

	info->in_rand_en = B_DISABLE;
	info->in_rand_rst = B_DISABLE;

	if (g_kdrv_ipe_info[id].eth.enable) {
		info->dram_out_sel = IPE_ETH;
		/*if (in_img_info.in_src == KDRV_IPE_IN_SRC_DIRECT) {
		    DBG_WRN("KDRV_IPE_FUNC_ETH_OUT: KDRV_IPE_IN_SRC need to set output to dram\r\n");
		}*/
		if (out_img_info.dram_out == ENABLE) {
			DBG_WRN("dram cannot output img & eth same time\r\n");
			info->dram_out_sel = IPE_ORIGINAL;
		}
	} else {
		info->dram_out_sel = IPE_ORIGINAL;
	}

	if ((out_img_info.dram_out) || (info->dram_out_sel == IPE_ETH)) {
		info->yc_to_dram_en = TRUE;
	} else {
		info->yc_to_dram_en = FALSE;
	}

	info->ethout_info.eth_outsel = g_kdrv_ipe_info[id].eth.eth_outsel;
	info->ethout_info.eth_outfmt = g_kdrv_ipe_info[id].eth.eth_outfmt;

	info->defog_subin_en = (g_kdrv_ipe_info[id].dfg_param.enable || g_kdrv_ipe_info[id].lce_param.enable);
	if (info->defog_subin_en) {
		info->addr_in_defog = g_kdrv_ipe_info[id].subin_dma_addr.addr;
		info->lofs_in_defog = g_kdrv_ipe_info[id].subimg_param.subimg_lofs_in;
	}

	info->hovlp_opt = HOLAP_AUTO;
	if (g_kdrv_ipe_info[id].ext_info.hstrp_num > 1) {
		info->hovlp_opt = HOLAP_8PX;
	}

	return rt;
}

static ER kdrv_ipe_set_output_param(INT32 id, IPE_INPUTINFO *input_info, IPE_OUTYCINFO *info)
{
	ER rt = E_OK;

	// input info
	KDRV_IPE_IN_IMG_INFO in_img_info = g_kdrv_ipe_info[id].in_img_info;
	// output info
	KDRV_IPE_DMA_ADDR_INFO out_pixel_addr = g_kdrv_ipe_info[id].out_pixel_addr;

	if ((in_img_info.in_src == KDRV_IPE_IN_SRC_DIRECT) || (in_img_info.in_src == KDRV_IPE_IN_SRC_ALL_DIRECT)) {
		info->yc_to_ime_en = TRUE;
	} else {
		info->yc_to_ime_en = FALSE;
	}

	info->yc_to_dram_en = input_info->yc_to_dram_en;
	if (info->yc_to_dram_en) {
		info->yc_format = input_info->out_fmt;
	}

	info->dram_out_mode = g_kdrv_ipe_info[id].dram_out_mode;
	info->single_out_en.single_out_y_en = g_kdrv_ipe_info[id].single_out_ch.single_out_y_en;
	info->single_out_en.single_out_c_en = g_kdrv_ipe_info[id].single_out_ch.single_out_c_en;
	info->single_out_en.single_out_va_en = g_kdrv_ipe_info[id].single_out_ch.single_out_va_en;
	info->single_out_en.single_out_defog_en = g_kdrv_ipe_info[id].single_out_ch.single_out_defog_en;

	info->sub_h_sel = YCC_DROP_RIGHT;

	info->addr_y = out_pixel_addr.addr_y;
	info->addr_c = out_pixel_addr.addr_uv;

	info->lofs_y = input_info->in_size_y.h_size;
	info->lofs_c = input_info->in_size_y.h_size * 2;

	if (input_info->out_fmt == IPE_YCC420) {
		info->lofs_c = input_info->in_size_y.h_size;
	}

	info->dram_out_sel = input_info->dram_out_sel;
	info->ethout_info.eth_outsel = input_info->ethout_info.eth_outsel;
	info->ethout_info.eth_outfmt = input_info->ethout_info.eth_outfmt;

	if (info->dram_out_sel == IPE_ETH) {
		UINT32 tmp_lofs = info->lofs_y;

		if (info->ethout_info.eth_outfmt == ETH_OUT_2BITS) {
			tmp_lofs = tmp_lofs >> 2;
		}
		if (info->ethout_info.eth_outsel == ETH_OUT_ORIGINAL || info->ethout_info.eth_outsel == ETH_OUT_BOTH) {
			tmp_lofs = ((tmp_lofs + 3) >> 2) << 2; //word align
			info->lofs_c = tmp_lofs;
		}
		if (info->ethout_info.eth_outsel == ETH_OUT_DOWNSAMPLED || info->ethout_info.eth_outsel == ETH_OUT_BOTH) {
			tmp_lofs = tmp_lofs >> 1;
			tmp_lofs = ((tmp_lofs + 3) >> 2) << 2; //word align
			info->lofs_y = tmp_lofs;
		}
	}

	info->defog_to_dram_en = g_kdrv_ipe_info[id].subout_dma_addr.enable;
	if (info->defog_to_dram_en) {
		info->addr_defog = g_kdrv_ipe_info[id].subout_dma_addr.addr;
		info->lofs_defog = g_kdrv_ipe_info[id].subimg_param.subimg_lofs_out;
	}

	info->va_to_dram_en = g_kdrv_ipe_info[id].va_param.enable;
	if (info->va_to_dram_en) {
		info->addr_va = g_kdrv_ipe_info[id].va_addr.addr_va;
		info->lofs_va = g_kdrv_ipe_info[id].va_param.va_lofs;
		info->va_outsel = g_kdrv_ipe_info[id].va_param.va_out_grp1_2;
	}

	return rt;
}

static ER kdrv_ipe_set_iq_eext(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	info->p_eext_param->eext_gam_sel = g_kdrv_ipe_info[id].eext_tonemap_param.gamma_sel;
	if (g_kdrv_ipe_info[id].in_img_info.in_src == KDRV_IPE_IN_SRC_DRAM) {
		info->p_eext_param->eext_ch_sel = EEXT_G_CHANNEL;
	} else {
		info->p_eext_param->eext_ch_sel = EEXT_Y_CHANNEL;
	}
	//info->p_eext_param->eext_ch_sel = g_kdrv_ipe_info[id].eext_param.edge_ch_sel;
	info->p_eext_param->p_edgeker_b = &(g_kdrv_ipe_info[id].eext_param.edge_ker[0]);
	info->p_eext_param->p_tonemap_lut = &(g_kdrv_ipe_info[id].eext_tonemap_param.tone_map_lut[0]);

	// IQ test
	{
		extern UINT32 ipe_dbg_en;
		extern unsigned int ipe_argv[3];

		if (ipe_dbg_en == IQ_DBG_EDGEGAM) {
			if (ipe_argv[0] == 1) {
				info->p_eext_param->p_tonemap_lut = &(tone_remap_init_mod1[0]);
			} else {
				info->p_eext_param->p_tonemap_lut = &(g_kdrv_ipe_info[id].eext_tonemap_param.tone_map_lut[0]);
			}
		}
	}

	info->p_eext_param->eext_kerstrength.ker_a.eext_enh = g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq0.eext_enh;
	info->p_eext_param->eext_kerstrength.ker_a.eext_div = g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq0.eext_div;
	info->p_eext_param->eext_kerstrength.ker_c.eext_enh = g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq1.eext_enh;
	info->p_eext_param->eext_kerstrength.ker_c.eext_div = g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq1.eext_div;
	info->p_eext_param->eext_kerstrength.ker_d.eext_enh = g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq2.eext_enh;
	info->p_eext_param->eext_kerstrength.ker_d.eext_div = g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq2.eext_div;

	info->p_eext_param->eext_engcon.eext_div_con = g_kdrv_ipe_info[id].eext_param.eext_engcon.eext_div_con;
	info->p_eext_param->eext_engcon.eext_div_eng = g_kdrv_ipe_info[id].eext_param.eext_engcon.eext_div_eng;
	info->p_eext_param->eext_engcon.wt_con_eng = g_kdrv_ipe_info[id].eext_param.eext_engcon.wt_con_eng;

	info->p_eext_param->ker_thickness.wt_ker_robust = g_kdrv_ipe_info[id].eext_param.ker_thickness.wt_ker_robust;
	info->p_eext_param->ker_thickness.wt_ker_thin = g_kdrv_ipe_info[id].eext_param.ker_thickness.wt_ker_thin;
	info->p_eext_param->ker_thickness.iso_ker_thin = g_kdrv_ipe_info[id].eext_param.ker_thickness.iso_ker_thin;
	info->p_eext_param->ker_thickness.iso_ker_robust = g_kdrv_ipe_info[id].eext_param.ker_thickness.iso_ker_robust;

	info->p_eext_param->ker_thickness_hld.wt_ker_robust = g_kdrv_ipe_info[id].eext_param.ker_thickness_hld.wt_ker_robust;
	info->p_eext_param->ker_thickness_hld.wt_ker_thin = g_kdrv_ipe_info[id].eext_param.ker_thickness_hld.wt_ker_thin;
	info->p_eext_param->ker_thickness_hld.iso_ker_thin = g_kdrv_ipe_info[id].eext_param.ker_thickness_hld.iso_ker_thin;
	info->p_eext_param->ker_thickness_hld.iso_ker_robust = g_kdrv_ipe_info[id].eext_param.ker_thickness_hld.iso_ker_robust;

	info->p_eext_param->eext_region.wt_low = g_kdrv_ipe_info[id].eext_param.eext_region.reg_wt.wt_low;
	info->p_eext_param->eext_region.wt_high = g_kdrv_ipe_info[id].eext_param.eext_region.reg_wt.wt_high;
	info->p_eext_param->eext_region.wt_low_hld = g_kdrv_ipe_info[id].eext_param.eext_region.reg_wt.wt_low_hld;
	info->p_eext_param->eext_region.wt_high_hld = g_kdrv_ipe_info[id].eext_param.eext_region.reg_wt.wt_high_hld;

	info->p_eext_param->eext_region.th_flat = g_kdrv_ipe_info[id].eext_param.eext_region.reg_th.th_flat;
	info->p_eext_param->eext_region.th_edge = g_kdrv_ipe_info[id].eext_param.eext_region.reg_th.th_edge;
	info->p_eext_param->eext_region.th_flat_hld = g_kdrv_ipe_info[id].eext_param.eext_region.reg_th.th_flat_hld;
	info->p_eext_param->eext_region.th_edge_hld = g_kdrv_ipe_info[id].eext_param.eext_region.reg_th.th_edge_hld;
	info->p_eext_param->eext_region.th_lum_hld = g_kdrv_ipe_info[id].eext_param.eext_region.reg_th.th_lum_hld;

	info->p_eext_param->eext_overshoot.overshoot_en = g_kdrv_ipe_info[id].overshoot_param.overshoot_en;
	info->p_eext_param->eext_overshoot.wt_overshoot = g_kdrv_ipe_info[id].overshoot_param.wt_overshoot;
	info->p_eext_param->eext_overshoot.th_overshoot = g_kdrv_ipe_info[id].overshoot_param.th_overshoot;
	info->p_eext_param->eext_overshoot.slope_overshoot = g_kdrv_ipe_info[id].overshoot_param.slope_overshoot;

	info->p_eext_param->eext_overshoot.wt_undershoot = g_kdrv_ipe_info[id].overshoot_param.wt_undershoot;
	info->p_eext_param->eext_overshoot.th_undershoot = g_kdrv_ipe_info[id].overshoot_param.th_undershoot;
	info->p_eext_param->eext_overshoot.slope_undershoot = g_kdrv_ipe_info[id].overshoot_param.slope_undershoot;

	info->p_eext_param->eext_overshoot.th_undershoot_lum = g_kdrv_ipe_info[id].overshoot_param.th_undershoot_lum;
	info->p_eext_param->eext_overshoot.th_undershoot_eng = g_kdrv_ipe_info[id].overshoot_param.th_undershoot_eng;
	info->p_eext_param->eext_overshoot.slope_undershoot_lum = g_kdrv_ipe_info[id].overshoot_param.slope_undershoot_lum;
	info->p_eext_param->eext_overshoot.slope_undershoot_eng = g_kdrv_ipe_info[id].overshoot_param.slope_undershoot_eng;
	info->p_eext_param->eext_overshoot.clamp_wt_mod_lum = g_kdrv_ipe_info[id].overshoot_param.clamp_wt_mod_lum;
	info->p_eext_param->eext_overshoot.clamp_wt_mod_eng = g_kdrv_ipe_info[id].overshoot_param.clamp_wt_mod_eng;
	info->p_eext_param->eext_overshoot.strength_lum_eng = g_kdrv_ipe_info[id].overshoot_param.strength_lum_eng;
	info->p_eext_param->eext_overshoot.norm_lum_eng = g_kdrv_ipe_info[id].overshoot_param.norm_lum_eng;

	info->param_update_sel |= IPE_SET_EDGEEXT;

	return rt;
}

static ER kdrv_ipe_set_iq_eproc(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	info->p_eproc_param->p_edgemap_th->ethr_low = g_kdrv_ipe_info[id].eproc_param.edge_map_th.ethr_low;
	info->p_eproc_param->p_edgemap_th->ethr_high = g_kdrv_ipe_info[id].eproc_param.edge_map_th.ethr_high;
	info->p_eproc_param->p_edgemap_th->etab_low = g_kdrv_ipe_info[id].eproc_param.edge_map_th.etab_low;
	info->p_eproc_param->p_edgemap_th->etab_high = g_kdrv_ipe_info[id].eproc_param.edge_map_th.etab_high;
	info->p_eproc_param->p_edgemap_th->ein_sel = g_kdrv_ipe_info[id].eproc_param.edge_map_th.map_sel;

	info->p_eproc_param->p_edgemap_lut = &(g_kdrv_ipe_info[id].eproc_param.edge_map_lut[0]);

	info->p_eproc_param->p_esmap_th->ethr_low = g_kdrv_ipe_info[id].eproc_param.es_map_th.ethr_low;
	info->p_eproc_param->p_esmap_th->ethr_high = g_kdrv_ipe_info[id].eproc_param.es_map_th.ethr_high;
	info->p_eproc_param->p_esmap_th->etab_low = g_kdrv_ipe_info[id].eproc_param.es_map_th.etab_low;
	info->p_eproc_param->p_esmap_th->etab_high = g_kdrv_ipe_info[id].eproc_param.es_map_th.etab_high;

	info->p_eproc_param->p_esmap_lut = &(g_kdrv_ipe_info[id].eproc_param.es_map_lut[0]);

	info->param_update_sel |= IPE_SET_EDGEPROC;

	return rt;
}

static ER kdrv_ipe_set_iq_rgblpf(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	if (g_kdrv_ipe_info[id].rgb_lpf_param.enable) {
		info->p_rgblpf_param->lpf_r_info.lpfw = g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_r.lpf_w;
		info->p_rgblpf_param->lpf_r_info.sonly_w = g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_r.s_only_w;
		info->p_rgblpf_param->lpf_r_info.range_th0 = g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_r.range_th0;
		info->p_rgblpf_param->lpf_r_info.range_th1 = g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_r.range_th1;
		info->p_rgblpf_param->lpf_r_info.filt_size = g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_r.filt_size;

		info->p_rgblpf_param->lpf_g_info.lpfw = g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_g.lpf_w;
		info->p_rgblpf_param->lpf_g_info.sonly_w = g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_g.s_only_w;
		info->p_rgblpf_param->lpf_g_info.range_th0 = g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_g.range_th0;
		info->p_rgblpf_param->lpf_g_info.range_th1 = g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_g.range_th1;
		info->p_rgblpf_param->lpf_g_info.filt_size = g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_g.filt_size;

		info->p_rgblpf_param->lpf_b_info.lpfw = g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_b.lpf_w;
		info->p_rgblpf_param->lpf_b_info.sonly_w = g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_b.s_only_w;
		info->p_rgblpf_param->lpf_b_info.range_th0 = g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_b.range_th0;
		info->p_rgblpf_param->lpf_b_info.range_th1 = g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_b.range_th1;
		info->p_rgblpf_param->lpf_b_info.filt_size = g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_b.filt_size;


		// IQ test
		{
			extern UINT32 ipe_dbg_en;
			extern unsigned int ipe_argv[3];

			if (ipe_dbg_en == IQ_DBG_RGBLPF) {
				info->p_rgblpf_param->lpf_r_info.range_th0 = ipe_argv[0];
				info->p_rgblpf_param->lpf_r_info.range_th1 = ipe_argv[1];
				info->p_rgblpf_param->lpf_g_info.range_th0 = ipe_argv[0];
				info->p_rgblpf_param->lpf_g_info.range_th1 = ipe_argv[1];
				info->p_rgblpf_param->lpf_b_info.range_th0 = ipe_argv[0];
				info->p_rgblpf_param->lpf_b_info.range_th1 = ipe_argv[1];

			}
		}


		info->param_update_sel |= IPE_SET_RGBLPF;
	} else {
		info->param_update_sel &= (~IPE_SET_RGBLPF);
	}

	return rt;
}

static ER kdrv_ipe_set_iq_pfr(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;
	int i = 0;

	if (g_kdrv_ipe_info[id].pfr_param.enable) {
		info->p_pfr_param->uv_filt_en = g_kdrv_ipe_info[id].pfr_param.uv_filt_en;
		info->p_pfr_param->luma_level_en = g_kdrv_ipe_info[id].pfr_param.luma_level_en;
		info->p_pfr_param->wet_out = g_kdrv_ipe_info[id].pfr_param.pfr_strength;
		info->p_pfr_param->edge_th = g_kdrv_ipe_info[id].pfr_param.edge_th;
		info->p_pfr_param->edge_strength = g_kdrv_ipe_info[id].pfr_param.edge_str;
		info->p_pfr_param->luma_th = g_kdrv_ipe_info[id].pfr_param.luma_th;
		info->p_pfr_param->p_luma_lut = g_kdrv_ipe_info[id].pfr_param.luma_lut;

		for (i = 0; i < KDRV_IPE_PFR_SET_NUM; i++) {
			info->p_pfr_param->color_wet_set[i].enable =  g_kdrv_ipe_info[id].pfr_param.color_wet_set[i].enable;
			info->p_pfr_param->color_wet_set[i].color_u = g_kdrv_ipe_info[id].pfr_param.color_wet_set[i].color_u;
			info->p_pfr_param->color_wet_set[i].color_v = g_kdrv_ipe_info[id].pfr_param.color_wet_set[i].color_v;
			info->p_pfr_param->color_wet_set[i].color_wet_r = g_kdrv_ipe_info[id].pfr_param.color_wet_set[i].color_wet_r;
			info->p_pfr_param->color_wet_set[i].color_wet_b = g_kdrv_ipe_info[id].pfr_param.color_wet_set[i].color_wet_b;
			info->p_pfr_param->color_wet_set[i].cdiff_th = g_kdrv_ipe_info[id].pfr_param.color_wet_set[i].cdiff_th;
			info->p_pfr_param->color_wet_set[i].cdiff_step = g_kdrv_ipe_info[id].pfr_param.color_wet_set[i].cdiff_step;
			info->p_pfr_param->color_wet_set[i].cdiff_lut[0] = g_kdrv_ipe_info[id].pfr_param.color_wet_set[i].cdiff_lut[0];
			info->p_pfr_param->color_wet_set[i].cdiff_lut[1] = g_kdrv_ipe_info[id].pfr_param.color_wet_set[i].cdiff_lut[1];
			info->p_pfr_param->color_wet_set[i].cdiff_lut[2] = g_kdrv_ipe_info[id].pfr_param.color_wet_set[i].cdiff_lut[2];
			info->p_pfr_param->color_wet_set[i].cdiff_lut[3] = g_kdrv_ipe_info[id].pfr_param.color_wet_set[i].cdiff_lut[3];
		}

		info->param_update_sel |= IPE_SET_PFR;
	} else {
		info->param_update_sel &= (~IPE_SET_PFR);
	}

	return rt;
}

static ER kdrv_ipe_set_iq_cc(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	if (g_kdrv_ipe_info[id].cc_en_param.enable) {
		info->p_cc_param->cc_range = g_kdrv_ipe_info[id].ccm_param.cc_range;
		info->p_cc_param->cc2_sel = g_kdrv_ipe_info[id].cc_param.cc2_sel;
		info->p_cc_param->cc_gam_sel = g_kdrv_ipe_info[id].ccm_param.cc_gamma_sel;
		info->p_cc_param->cc_stab_sel = g_kdrv_ipe_info[id].cc_param.cc_stab_sel;
		info->p_cc_param->cc_ofs_sel = CC_OFS_BYPASS;//g_kdrv_ipe_info[id].cc_param.cc_ofs_sel;
		info->p_cc_param->p_cc_coeff = &(g_kdrv_ipe_info[id].ccm_param.coef[0]);
		info->p_cc_param->p_fstab = &(g_kdrv_ipe_info[id].cc_param.fstab[0]);
		info->p_cc_param->p_fdtab = &(g_kdrv_ipe_info[id].cc_param.fdtab[0]);

		info->param_update_sel |= IPE_SET_COLORCORRECT;
	} else {
		info->param_update_sel &= (~IPE_SET_COLORCORRECT);
	}

	return rt;
}

static ER kdrv_ipe_set_iq_cctrl(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	if (g_kdrv_ipe_info[id].cctrl_en_param.enable) {

		info->p_cctrl_param->int_ofs = g_kdrv_ipe_info[id].cctrl_param.int_ofs;
		info->p_cctrl_param->sat_ofs = g_kdrv_ipe_info[id].cctrl_param.sat_ofs;
		info->p_cctrl_param->hue_rotate_en = g_kdrv_ipe_info[id].cctrl_ct_param.hue_rotate_en;
		info->p_cctrl_param->hue_c2g_en = g_kdrv_ipe_info[id].cctrl_param.hue_c2g;
		info->p_cctrl_param->cctrl_sel = g_kdrv_ipe_info[id].cctrl_param.cctrl_sel;
		info->p_cctrl_param->vdet_div = g_kdrv_ipe_info[id].cctrl_param.vdet_div;
		info->p_cctrl_param->p_hue_tab = &(g_kdrv_ipe_info[id].cctrl_ct_param.hue_tab[0]);
		info->p_cctrl_param->p_sat_tab = &(g_kdrv_ipe_info[id].cctrl_ct_param.sat_tab[0]);
		info->p_cctrl_param->p_int_tab = &(g_kdrv_ipe_info[id].cctrl_ct_param.int_tab[0]);
		info->p_cctrl_param->p_edge_tab = &(g_kdrv_ipe_info[id].cctrl_param.edge_tab[0]);
		info->p_cctrl_param->p_dds_tab = &(g_kdrv_ipe_info[id].cctrl_param.dds_tab[0]);

		info->param_update_sel |= IPE_SET_CCTRL;
	} else {
		info->param_update_sel &= (~IPE_SET_CCTRL);
	}

	return rt;
}

static ER kdrv_ipe_set_iq_cadjee(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	if (g_kdrv_ipe_info[id].cadj_ee_param.enable) {
		info->p_cadj_eenh_param->enh_p = g_kdrv_ipe_info[id].cadj_ee_param.edge_enh_p;
		info->p_cadj_eenh_param->enh_n = g_kdrv_ipe_info[id].cadj_ee_param.edge_enh_n;
		info->p_cadj_einv_param->einv_p_en = g_kdrv_ipe_info[id].cadj_ee_param.edge_inv_p_en;
		info->p_cadj_einv_param->einv_n_en = g_kdrv_ipe_info[id].cadj_ee_param.edge_inv_n_en;

		info->param_update_sel |= (IPE_SET_CADJ_EENH | IPE_SET_CADJ_EINV);
	} else {
		info->param_update_sel &= (~IPE_SET_CADJ_EENH | IPE_SET_CADJ_EINV);
	}

	return rt;
}


static ER kdrv_ipe_set_iq_cadjyccon(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	if (g_kdrv_ipe_info[id].cadj_yccon_param.enable) {
		info->p_cadj_yccon_param->ycon = g_kdrv_ipe_info[id].cadj_yccon_param.y_con;
		info->p_cadj_yccon_param->ccon = g_kdrv_ipe_info[id].cadj_yccon_param.c_con;
		info->p_cadj_yccon_param->ccontab_sel = g_kdrv_ipe_info[id].cadj_yccon_param.ccontab_sel;
		info->p_cadj_yccon_param->p_ccon_tab = g_kdrv_ipe_info[id].cadj_yccon_param.cconlut;
		info->param_update_sel |= IPE_SET_CADJ_YCCON;
	} else {
		//info->param_update_sel &= (~IPE_SET_CADJ_YCCON);
		info->p_cadj_yccon_param->ycon = 0x80;   // default value
		info->p_cadj_yccon_param->ccon = 0x80;   // default value
		info->p_cadj_yccon_param->ccontab_sel = 0;
		info->p_cadj_yccon_param->p_ccon_tab = ccontab_init;
		info->param_update_sel |= IPE_SET_CADJ_YCCON;
	}

	return rt;
}

static ER kdrv_ipe_set_iq_cadjcofs(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	if (g_kdrv_ipe_info[id].cadj_cofs_param.enable) {
		info->p_cadj_cofs_param->cb_ofs = g_kdrv_ipe_info[id].cadj_cofs_param.cb_ofs;
		info->p_cadj_cofs_param->cr_ofs = g_kdrv_ipe_info[id].cadj_cofs_param.cr_ofs;

		info->param_update_sel |= IPE_SET_CADJ_COFS;
	} else {
		//info->param_update_sel &= (~IPE_SET_CADJ_COFS);
		info->p_cadj_cofs_param->cb_ofs = 0x80;   // default value
		info->p_cadj_cofs_param->cr_ofs = 0x80;   // default value
		info->param_update_sel |= IPE_SET_CADJ_COFS;
	}

	return rt;
}

static ER kdrv_ipe_set_iq_cadjrand(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	if (g_kdrv_ipe_info[id].cadj_rand_param.enable) {
		info->p_cadj_rand_param->yrand_en = g_kdrv_ipe_info[id].cadj_rand_param.rand_en_y;
		info->p_cadj_rand_param->crand_en = g_kdrv_ipe_info[id].cadj_rand_param.rand_en_c;
		info->p_cadj_rand_param->yrand_level = g_kdrv_ipe_info[id].cadj_rand_param.rand_level_y;
		info->p_cadj_rand_param->crand_level = g_kdrv_ipe_info[id].cadj_rand_param.rand_level_c;
		info->p_cadj_rand_param->yc_rand_rst = g_kdrv_ipe_info[id].cadj_rand_param.rand_reset;

		info->param_update_sel |= IPE_SET_CADJ_RAND;
	} else {
		//info->param_update_sel &= (~IPE_SET_CADJ_RAND);
		info->p_cadj_rand_param->yrand_en = FALSE;
		info->p_cadj_rand_param->crand_en = FALSE;
		info->p_cadj_rand_param->yrand_level = 0;
		info->p_cadj_rand_param->crand_level = 0;
		info->p_cadj_rand_param->yc_rand_rst = FALSE;

		info->param_update_sel |= IPE_SET_CADJ_RAND;
	}

	return rt;
}

static ER kdrv_ipe_set_iq_cadjfixth(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	if (g_kdrv_ipe_info[id].cadj_fixth_param.enable) {
		info->p_cadj_fixth_param->p_yth1->y_th = g_kdrv_ipe_info[id].cadj_fixth_param.yth1.y_th;
		info->p_cadj_fixth_param->p_yth1->edgeth = g_kdrv_ipe_info[id].cadj_fixth_param.yth1.edge_th;
		info->p_cadj_fixth_param->p_yth1->hit_sel = g_kdrv_ipe_info[id].cadj_fixth_param.yth1.ycth_sel_hit;
		info->p_cadj_fixth_param->p_yth1->nhit_sel = g_kdrv_ipe_info[id].cadj_fixth_param.yth1.ycth_sel_nonhit;
		info->p_cadj_fixth_param->p_yth1->hit_value = g_kdrv_ipe_info[id].cadj_fixth_param.yth1.value_hit;
		info->p_cadj_fixth_param->p_yth1->nhit_value = g_kdrv_ipe_info[id].cadj_fixth_param.yth1.nonvalue_hit;

		info->p_cadj_fixth_param->p_yth2->y_th = g_kdrv_ipe_info[id].cadj_fixth_param.yth2.y_th;
		info->p_cadj_fixth_param->p_yth2->hit_sel = g_kdrv_ipe_info[id].cadj_fixth_param.yth2.ycth_sel_hit;
		info->p_cadj_fixth_param->p_yth2->nhit_sel = g_kdrv_ipe_info[id].cadj_fixth_param.yth2.ycth_sel_nonhit;
		info->p_cadj_fixth_param->p_yth2->hit_value = g_kdrv_ipe_info[id].cadj_fixth_param.yth2.value_hit;
		info->p_cadj_fixth_param->p_yth2->nhit_value = g_kdrv_ipe_info[id].cadj_fixth_param.yth2.nonvalue_hit;

		info->p_cadj_fixth_param->p_cth->edgeth = g_kdrv_ipe_info[id].cadj_fixth_param.cth.edge_th;
		info->p_cadj_fixth_param->p_cth->yth_low = g_kdrv_ipe_info[id].cadj_fixth_param.cth.y_th_low;
		info->p_cadj_fixth_param->p_cth->yth_high = g_kdrv_ipe_info[id].cadj_fixth_param.cth.y_th_high;
		info->p_cadj_fixth_param->p_cth->cbth_low = g_kdrv_ipe_info[id].cadj_fixth_param.cth.cb_th_low;
		info->p_cadj_fixth_param->p_cth->cbth_high = g_kdrv_ipe_info[id].cadj_fixth_param.cth.cb_th_high;
		info->p_cadj_fixth_param->p_cth->crth_low = g_kdrv_ipe_info[id].cadj_fixth_param.cth.cr_th_low;
		info->p_cadj_fixth_param->p_cth->crth_high = g_kdrv_ipe_info[id].cadj_fixth_param.cth.cr_th_high;
		info->p_cadj_fixth_param->p_cth->hit_sel = g_kdrv_ipe_info[id].cadj_fixth_param.cth.ycth_sel_hit;
		info->p_cadj_fixth_param->p_cth->nhit_sel = g_kdrv_ipe_info[id].cadj_fixth_param.cth.ycth_sel_nonhit;
		info->p_cadj_fixth_param->p_cth->cb_hit_value = g_kdrv_ipe_info[id].cadj_fixth_param.cth.cb_value_hit;
		info->p_cadj_fixth_param->p_cth->cb_nhit_value = g_kdrv_ipe_info[id].cadj_fixth_param.cth.cb_value_nonhit;
		info->p_cadj_fixth_param->p_cth->cr_hit_value = g_kdrv_ipe_info[id].cadj_fixth_param.cth.cr_value_hit;
		info->p_cadj_fixth_param->p_cth->cr_nhit_value = g_kdrv_ipe_info[id].cadj_fixth_param.cth.cr_value_nonhit;

		info->param_update_sel |= IPE_SET_CADJ_FIXTH;
	} else {
		info->param_update_sel &= (~IPE_SET_CADJ_FIXTH);
	}

	return rt;
}

static ER kdrv_ipe_set_iq_cadjmask(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	if (g_kdrv_ipe_info[id].cadj_mask_param.enable) {
		info->p_cadj_mask_param->ymask = g_kdrv_ipe_info[id].cadj_mask_param.y_mask;
		info->p_cadj_mask_param->cbmask = g_kdrv_ipe_info[id].cadj_mask_param.cb_mask;
		info->p_cadj_mask_param->crmask = g_kdrv_ipe_info[id].cadj_mask_param.cr_mask;

		info->param_update_sel |= IPE_SET_CADJ_MASK;
	} else {
		//info->param_update_sel &= (~IPE_SET_CADJ_MASK);
		info->p_cadj_mask_param->ymask = 0xff;   // default value
		info->p_cadj_mask_param->cbmask = 0xff;  // default value
		info->p_cadj_mask_param->crmask = 0xff;  // default value

		info->param_update_sel |= IPE_SET_CADJ_MASK;
	}

	return rt;
}

static ER kdrv_ipe_set_iq_cst(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	if (g_kdrv_ipe_info[id].cst_param.enable) {
		info->p_cst_param->p_cst_coeff = &(g_kdrv_ipe_info[id].cst_param.cst_coef[0]);
		info->p_cst_param->cstoff_sel = g_kdrv_ipe_info[id].cst_param.cst_off_sel;

		info->param_update_sel |= IPE_SET_CST;
	} else {
		info->p_cst_param->p_cst_coeff = cst_coef_init;
		info->p_cst_param->cstoff_sel = CST_MINUS128;
		info->param_update_sel |= (IPE_SET_CST);
	}

	return rt;
}

static ER kdrv_ipe_set_iq_cstp(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	if (g_kdrv_ipe_info[id].cstp_param.enable) {
		info->p_cstp_param->cstp_rto = g_kdrv_ipe_info[id].cstp_param.cstp_ratio;

		info->param_update_sel |= IPE_SET_CSTP;
	} else {
		info->param_update_sel &= (~IPE_SET_CSTP);
	}

	return rt;
}

static ER kdrv_ipe_set_iq_gammarand(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	if (g_kdrv_ipe_info[id].gamy_rand_param.enable) {
		if ((g_kdrv_ipe_info[id].gamma.enable) || (g_kdrv_ipe_info[id].y_curve.enable)) {
			info->p_gamycur_rand->gam_y_rand_en = g_kdrv_ipe_info[id].gamy_rand_param.rand_en;
			info->p_gamycur_rand->gam_y_rand_rst = g_kdrv_ipe_info[id].gamy_rand_param.rst_en;
			info->p_gamycur_rand->gam_y_rand_shft = g_kdrv_ipe_info[id].gamy_rand_param.rand_shift;
		} else {
			DBG_WRN("PARAM_GAMYRAND only support when PARAM_GAMMA or PARAM_YCURVE enable\r\n");
		}

//		info->param_update_sel |= IPE_SET_GAMMA; //fake update bit
	} else {
//		info->param_update_sel &= (~IPE_SET_GAMMA);
	}

	return rt;
}

static ER kdrv_ipe_set_iq_gammacurve(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;
	BOOL b_err = FALSE, b_align = TRUE;
	KDRV_IPE_GAMMA_OPTION input_option = g_kdrv_ipe_info[id].gamma.option;
	UINT32 input_addr[KDRV_IPP_RGB_MAX_CH];

	input_addr[KDRV_IPP_RGB_R] = (UINT32) & (g_kdrv_ipe_info[id].gamma.gamma_lut[KDRV_IPP_RGB_R][0]);
	input_addr[KDRV_IPP_RGB_G] = (UINT32) & (g_kdrv_ipe_info[id].gamma.gamma_lut[KDRV_IPP_RGB_G][0]);
	input_addr[KDRV_IPP_RGB_B] = (UINT32) & (g_kdrv_ipe_info[id].gamma.gamma_lut[KDRV_IPP_RGB_B][0]);

	if (g_kdrv_ipe_info[id].gamma.enable) {
		// check addr
		if (input_option == KDRV_IPE_GAMMA_RGB_COMBINE) {
			b_align = kdrv_ipe_check_align(input_addr[KDRV_IPP_RGB_R], 4);
			if ((b_align == FALSE) || (input_addr[KDRV_IPP_RGB_R] == 0)) {
				DBG_WRN("parameter error (gamma.gamma_addr[KDRV_IPP_RGB_R] 0x%.8x)\r\n", (unsigned int)input_addr[KDRV_IPP_RGB_R]);
				b_err = TRUE;
			} else {
				input_addr[KDRV_IPP_RGB_G] = input_addr[KDRV_IPP_RGB_R];
				input_addr[KDRV_IPP_RGB_B] = input_addr[KDRV_IPP_RGB_R];
			}
		} else if (input_option == KDRV_IPE_GAMMA_RGB_SEPERATE) {
			b_align &= kdrv_ipe_check_align(input_addr[KDRV_IPP_RGB_R], 4);
			b_align &= kdrv_ipe_check_align(input_addr[KDRV_IPP_RGB_G], 4);
			b_align &= kdrv_ipe_check_align(input_addr[KDRV_IPP_RGB_B], 4);
			if ((b_align == FALSE) || (input_addr[KDRV_IPP_RGB_R] == 0)
				|| (input_addr[KDRV_IPP_RGB_G] == 0) || (input_addr[KDRV_IPP_RGB_B] == 0)) {
				DBG_WRN("parameter error (gamma.gamma_addr 0x%.8x 0x%.8x 0x%.8x)\r\n"
						, (unsigned int)input_addr[KDRV_IPP_RGB_R], (unsigned int)input_addr[KDRV_IPP_RGB_G], (unsigned int)input_addr[KDRV_IPP_RGB_B]);
				b_err = TRUE;
			}
		} else {
			DBG_WRN("input parameter error (gamma.option %d)\r\n", input_option);
			b_err = TRUE;
		}

		// convert data and set to driver addr
		if (b_err == TRUE) {
			info->param_update_sel &= (~IPE_SET_GAMMA);
		} else {
			info->addr_gamma = (UINT32)kdrv_ipe_iq_gammacurve_conv(
								   (UINT32 *)input_addr[KDRV_IPP_RGB_R], (UINT32 *)input_addr[KDRV_IPP_RGB_G], (UINT32 *)input_addr[KDRV_IPP_RGB_B]);
			info->param_update_sel |= IPE_SET_GAMMA;

			//DBG_USER("kdrv gamma addr = 0x%08x\r\n", (unsigned int)info->addr_gamma);
		}
	} else {
		info->param_update_sel &= (~IPE_SET_GAMMA);
	}

	return rt;
}


static ER kdrv_ipe_set_iq_ycurve(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;
	BOOL b_align = TRUE;
	UINT32 input_addr = (UINT32) & (g_kdrv_ipe_info[id].y_curve.y_curve_lut[0]);

	if (g_kdrv_ipe_info[id].y_curve.enable) {
//		info->uiYCurveAddr = g_kdrv_ipe_info[id].y_curve.y_curve_addr;
		b_align &= kdrv_ipe_check_align(input_addr, 4);
		if ((b_align == FALSE) || (input_addr == 0)) {
			DBG_WRN("parameter error (y_curve.y_curve_addr 0x%.8x)\r\n", (unsigned int)input_addr);
			info->param_update_sel &= (~IPE_SET_YCURVE);
		} else {
			info->p_ycurve_param->addr_ycurve = (UINT32)kdrv_ipe_iq_ycurve_conv((UINT32 *)input_addr);
			info->p_ycurve_param->ycurve_sel = g_kdrv_ipe_info[id].y_curve.ycurve_sel;
			//DBG_USER("kdrv ycurve addr = 0x%08x\r\n", (unsigned int)info->p_ycurve_param->addr_ycurve);
			info->param_update_sel |= IPE_SET_YCURVE;
		}

	} else {
		info->param_update_sel &= (~IPE_SET_YCURVE);
	}

	return rt;
}

static ER kdrv_ipe_set_iq_3dcc(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	info->p_defog_rand->rand_opt = KDRV_IPE_ROUNDING;
	info->p_defog_rand->rand_rst = 0;

	return rt;
}

static ER kdrv_ipe_set_iq_defog_subimg(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	info->p_dfg_subimg_param->input_size.h_size = g_kdrv_ipe_info[id].in_img_info.ch.y_width;
	info->p_dfg_subimg_param->input_size.v_size = g_kdrv_ipe_info[id].in_img_info.ch.y_height;
	info->p_dfg_subimg_param->dfg_subimg_size.h_size = g_kdrv_ipe_info[id].subimg_param.subimg_size.h_size;
	info->p_dfg_subimg_param->dfg_subimg_size.v_size = g_kdrv_ipe_info[id].subimg_param.subimg_size.v_size;

	info->p_dfg_subimg_param->subimg_param_mode = IPE_PARAM_AUTO_MODE ;
	info->p_dfg_subimg_param->p_subimg_ftrcoef = g_kdrv_ipe_info[id].subimg_param.subimg_ftrcoef;

	if (g_kdrv_ipe_info[id].subout_dma_addr.enable) {
		info->param_update_sel |= IPE_SET_DEFOG_SUBOUT;
	} else {
		info->param_update_sel &= (~IPE_SET_DEFOG_SUBOUT);
	}

	return rt;
}

static ER kdrv_ipe_set_iq_defog(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;
	UINT32 width, height;

	if (g_kdrv_ipe_info[id].dfg_param.enable) {

		info->p_defog_param->scalup_param.p_interp_diff_lut = g_kdrv_ipe_info[id].dfg_param.scalup_param.interp_diff_lut;
		info->p_defog_param->scalup_param.interp_wdist = g_kdrv_ipe_info[id].dfg_param.scalup_param.interp_wdist;
		info->p_defog_param->scalup_param.interp_wcenter = g_kdrv_ipe_info[id].dfg_param.scalup_param.interp_wcenter;
		info->p_defog_param->scalup_param.interp_wout = g_kdrv_ipe_info[id].dfg_param.scalup_param.interp_wout;
		info->p_defog_param->scalup_param.interp_wsrc = g_kdrv_ipe_info[id].dfg_param.scalup_param.interp_wsrc;

		info->p_defog_param->input_bld.p_in_blend_wt = g_kdrv_ipe_info[id].dfg_param.input_bld.in_blend_wt;

		info->p_defog_param->env_estimation.dfg_self_comp_en = g_kdrv_ipe_info[id].dfg_param.env_estimation.dfg_self_comp_en;
		info->p_defog_param->env_estimation.dfg_min_diff = g_kdrv_ipe_info[id].dfg_param.env_estimation.dfg_min_diff;
		info->p_defog_param->env_estimation.p_dfg_airlight = g_kdrv_ipe_info[id].dfg_param.env_estimation.dfg_airlight;
		info->p_defog_param->env_estimation.p_fog_mod_lut = g_kdrv_ipe_info[id].dfg_param.env_estimation.fog_mod_lut;

		info->p_defog_param->dfg_strength.str_mode_sel = g_kdrv_ipe_info[id].dfg_param.dfg_strength.str_mode_sel;
		info->p_defog_param->dfg_strength.p_target_lut = g_kdrv_ipe_info[id].dfg_param.dfg_strength.target_lut;
		info->p_defog_param->dfg_strength.fog_ratio = g_kdrv_ipe_info[id].dfg_param.dfg_strength.fog_ratio;
		info->p_defog_param->dfg_strength.dgain_ratio = g_kdrv_ipe_info[id].dfg_param.dfg_strength.dgain_ratio;
		info->p_defog_param->dfg_strength.gain_th = g_kdrv_ipe_info[id].dfg_param.dfg_strength.gain_th;

		info->p_defog_param->dfg_outbld.airlight_param_mode = IPE_PARAM_AUTO_MODE;
		info->p_defog_param->dfg_outbld.airlight_nfactor = 1025;
		info->p_defog_param->dfg_outbld.outbld_ref_sel = g_kdrv_ipe_info[id].dfg_param.dfg_outbld.outbld_ref_sel;
		info->p_defog_param->dfg_outbld.outbld_local_en = g_kdrv_ipe_info[id].dfg_param.dfg_outbld.outbld_local_en;
		info->p_defog_param->dfg_outbld.p_outbld_diff_wt = g_kdrv_ipe_info[id].dfg_param.dfg_outbld.outbld_diff_wt;
		info->p_defog_param->dfg_outbld.p_outbld_lum_wt = g_kdrv_ipe_info[id].dfg_param.dfg_outbld.outbld_lum_wt;

		width = g_kdrv_ipe_info[id].in_img_info.ch.y_width;
		height = g_kdrv_ipe_info[id].in_img_info.ch.y_height;
		info->p_defog_param->dfg_stcs.dfg_stcs_pixelcnt = (UINT32)(((width * height) * g_kdrv_ipe_info[id].dfg_param.dfg_stcs.airlight_stcs_ratio) >> 12);
		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFOG), "dfg_stcs_pixelcnt = [%d] \r\n", (int)info->p_defog_param->dfg_stcs.dfg_stcs_pixelcnt);

		info->param_update_sel |= IPE_SET_DEFOG;
	} else {
		info->param_update_sel &= (~IPE_SET_DEFOG);
	}

	return rt;
}

static ER kdrv_ipe_set_iq_lce(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	if (g_kdrv_ipe_info[id].lce_param.enable) {
		info->p_lce_param->diff_wt_neg = g_kdrv_ipe_info[id].lce_param.diff_wt_neg;
		info->p_lce_param->diff_wt_pos = g_kdrv_ipe_info[id].lce_param.diff_wt_pos;
		info->p_lce_param->diff_wt_avg = g_kdrv_ipe_info[id].lce_param.diff_wt_avg;
		info->p_lce_param->p_lum_wt_lut = g_kdrv_ipe_info[id].lce_param.lum_wt_lut;
		info->param_update_sel |= IPE_SET_LCE;

		// IQ test
		{
			extern UINT32 ipe_dbg_en;
			extern unsigned int ipe_argv[3];

			if (ipe_dbg_en == IQ_DBG_LCE) {
				info->p_lce_param->diff_wt_neg = ipe_argv[0];
				info->p_lce_param->diff_wt_avg = ipe_argv[1];
				info->p_lce_param->diff_wt_pos = ipe_argv[2];
			}
		}

	} else {
		info->param_update_sel &= (~IPE_SET_LCE);
	}

	return rt;
}

static ER kdrv_ipe_set_iq_edgedbg_moe(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	if (g_kdrv_ipe_info[id].edgdbg_param.enable) {
		info->edge_dbg_mode_sel = g_kdrv_ipe_info[id].edgdbg_param.mode_sel;
		info->param_update_sel |= IPE_SET_EDGE_DBG;
	} else {
		info->param_update_sel &= (~IPE_SET_EDGE_DBG);
	}

	return rt;
}

static ER kdrv_ipe_set_iq_va(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;
	BOOL dbg_printed = FALSE;
	UINT32 i;
	USIZE blk_size = {0};
	USIZE win_size = {0};
	URECT indep_roi[KDRV_IPE_VA_INDEP_NUM] = {0};
	IPE_IMG_SIZE img_size = {0};

	img_size.h_size = g_kdrv_ipe_info[id].in_img_info.ch.y_width;
	img_size.v_size = g_kdrv_ipe_info[id].in_img_info.ch.y_height;

	if (g_kdrv_ipe_info[id].va_param.enable) {
		info->p_va_win_info->win_numx = g_kdrv_ipe_info[id].va_param.win_num.w;
		info->p_va_win_info->win_numy = g_kdrv_ipe_info[id].va_param.win_num.h;

		blk_size.w = (img_size.h_size / g_kdrv_ipe_info[id].va_param.win_num.w);
		blk_size.h = (img_size.v_size / g_kdrv_ipe_info[id].va_param.win_num.h);
		win_size.w =  ALIGN_FLOOR(((g_kdrv_ipe_info[id].va_win_info.winsz_ratio.w * blk_size.w) / g_kdrv_ipe_info[id].va_win_info.ratio_base), 1);
		if (win_size.w > KDRV_IPE_VA_INDEP_WIN_MAX) {
            win_size.w = KDRV_IPE_VA_INDEP_WIN_MAX;
		}
		
		win_size.h =  ALIGN_FLOOR(((g_kdrv_ipe_info[id].va_win_info.winsz_ratio.h * blk_size.h) / g_kdrv_ipe_info[id].va_win_info.ratio_base), 1);
		if (win_size.h > KDRV_IPE_VA_INDEP_WIN_MAX) {
            win_size.h = KDRV_IPE_VA_INDEP_WIN_MAX;
		}

		info->p_va_win_info->win_szx = win_size.w;
		info->p_va_win_info->win_szy = win_size.h;
		info->p_va_win_info->win_stx = (blk_size.w - win_size.w) >> 1;
		info->p_va_win_info->win_sty = (blk_size.h - win_size.h) >> 1;
		info->p_va_win_info->win_spx = (blk_size.w - win_size.w);
		info->p_va_win_info->win_spy = (blk_size.h - win_size.h);

		if ((win_size.w > KDRV_IPE_VA_INDEP_WIN_MAX) || (win_size.h > KDRV_IPE_VA_INDEP_WIN_MAX) ||
			(info->p_va_win_info->win_spx >= 64) || (info->p_va_win_info->win_spy >= 32)) {

			//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFAULTON), "va window size max (511, 511)\r\n");
			//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFAULTON), "va window spacing max (63, 31)\r\n");
			//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFAULTON), "img size (%d, %d), win num (%d,%d), blk size(%d,%d) \r\n",
			//        (int)img_size.h_size, (int)img_size.v_size,
			//        (int)g_kdrv_ipe_info[id].va_param.win_num.w, (int)g_kdrv_ipe_info[id].va_param.win_num.h,
			//        (int)blk_size.w, (int)blk_size.h);
			//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFAULTON), "win size ratio = (%d,%d)/%d, win_sz = (%d, %d)\r\n",
			//        (int)g_kdrv_ipe_info[id].va_win_info.winsz_ratio.w, (int)g_kdrv_ipe_info[id].va_win_info.winsz_ratio.h,
			//        (int)g_kdrv_ipe_info[id].va_win_info.ratio_base, (int)win_size.w, (int)win_size.h);
			//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFAULTON), "starting point = (%d, %d), spacing = (%d, %d)\r\n",
			//        (int)info->p_va_win_info->win_stx, (int)info->p_va_win_info->win_sty,
			//        (int)info->p_va_win_info->win_spx, (int)info->p_va_win_info->win_spy);

			dbg_printed = TRUE;

			win_size.w = min(win_size.w, (unsigned int)(KDRV_IPE_VA_INDEP_WIN_MAX));
			win_size.h = min(win_size.h, (unsigned int)(KDRV_IPE_VA_INDEP_WIN_MAX));
			info->p_va_win_info->win_spx = min(info->p_va_win_info->win_spx, (unsigned int)63);
			info->p_va_win_info->win_spy = min(info->p_va_win_info->win_spy, (unsigned int)31);
		}

		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "va img size (%d, %d)\r\n", (int)img_size.h_size, (int)img_size.v_size);
		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "va win_num  (%d, %d), win size: (%d, %d) \r\n",
		//        (int)g_kdrv_ipe_info[id].va_param.win_num.w, (int)g_kdrv_ipe_info[id].va_param.win_num.h, (int)win_size.w, (int)win_size.h);
		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "va start point (%d, %d), spacing (%d, %d) \r\n",
		//        (int)info->p_va_win_info->win_stx, (int)info->p_va_win_info->win_sty,
		//        (int)info->p_va_win_info->win_spx, (int)info->p_va_win_info->win_spy);

		rt = ipe_check_va_win_info(info->p_va_win_info, img_size);
		if (rt != E_OK) {
			return rt;
		}

		info->p_va_filt_g1->cnt_en = g_kdrv_ipe_info[id].va_param.group_1.count_enable;
		info->p_va_filt_g2->cnt_en = g_kdrv_ipe_info[id].va_param.group_2.count_enable;
		info->p_va_filt_g1->linemax_en = g_kdrv_ipe_info[id].va_param.group_1.linemax_mode;
		info->p_va_filt_g2->linemax_en = g_kdrv_ipe_info[id].va_param.group_2.linemax_mode;

		info->param_update_sel |= IPE_SET_VA;
	} else {
		info->param_update_sel &= (~IPE_SET_VA);
	}

	if (g_kdrv_ipe_info[id].va_param.indep_va_enable == TRUE) {
		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "indep va, img size (%d, %d)\r\n", (int)img_size.h_size, (int)img_size.v_size);
		for (i = 0; i < KDRV_IPE_VA_INDEP_NUM; i++) {
			if (g_kdrv_ipe_info[id].va_param.indep_win[i].enable == TRUE) {

				info->p_va_indep_win_info[i].indep_va_en = g_kdrv_ipe_info[id].va_param.indep_win[i].enable;
				info->p_va_indep_win_info[i].linemax_g1_en = g_kdrv_ipe_info[id].va_param.indep_win[i].linemax_g1;
				info->p_va_indep_win_info[i].linemax_g2_en = g_kdrv_ipe_info[id].va_param.indep_win[i].linemax_g2;

				indep_roi[i].x = ALIGN_FLOOR(((g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[i].x * img_size.h_size) / g_kdrv_ipe_info[id].va_win_info.ratio_base), 1);
				indep_roi[i].y = ALIGN_FLOOR(((g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[i].y * img_size.v_size) / g_kdrv_ipe_info[id].va_win_info.ratio_base), 1);
				indep_roi[i].w = ALIGN_FLOOR(((g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[i].w * img_size.h_size) / g_kdrv_ipe_info[id].va_win_info.ratio_base), 1);
				indep_roi[i].h = ALIGN_FLOOR(((g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[i].h * img_size.v_size) / g_kdrv_ipe_info[id].va_win_info.ratio_base), 1);

				if (indep_roi[i].w > KDRV_IPE_VA_INDEP_WIN_MAX) {
					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFAULTON), "indep va [%d] window size h is too large\r\n", (int)i);
					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFAULTON), "indep va [%d] img size h = %d, win size ratio = %d/%d, win_sz_h = %d, xstart = %d\r\n", (int)i, (int)img_size.h_size,
					//        (int)g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[i].w, (int)g_kdrv_ipe_info[id].va_win_info.ratio_base, (int)indep_roi[i].w, (int)indep_roi[i].x);

					indep_roi[i].x = indep_roi[i].x + ALIGN_FLOOR((indep_roi[i].w - KDRV_IPE_VA_INDEP_WIN_MAX) / 2, 1);
					indep_roi[i].w = KDRV_IPE_VA_INDEP_WIN_MAX;


					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFAULTON), "indep va [%d] size h is clamped to %d, xstart is changed to %d\r\n", (int)i, (int)indep_roi[i].w, (int)indep_roi[i].x);

					dbg_printed = TRUE;
				}
				if (indep_roi[i].h > KDRV_IPE_VA_INDEP_WIN_MAX) {
					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFAULTON), "indep va [%d] window size v is too large\r\n", (int)i);
					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFAULTON), "indep va [%d] img size v = %d, win size ratio = %d/%d, win_sz_v = %d, ystart = %d\r\n", (int)i, (int)img_size.v_size,
					//        (int)g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[i].h, (int)g_kdrv_ipe_info[id].va_win_info.ratio_base, (int)indep_roi[i].h, (int)indep_roi[i].y);

					indep_roi[i].y = indep_roi[i].y + ALIGN_FLOOR((indep_roi[i].h - KDRV_IPE_VA_INDEP_WIN_MAX) / 2, 1);
					indep_roi[i].h = KDRV_IPE_VA_INDEP_WIN_MAX;

					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFAULTON), "indep va [%d] size v is clamped to %d, ystart is changed to %d\r\n", (int)i, (int)indep_roi[i].h, (int)indep_roi[i].y);

					dbg_printed = TRUE;
				}

				info->p_va_indep_win_info[i].win_stx = indep_roi[i].x;
				info->p_va_indep_win_info[i].win_sty = indep_roi[i].y;
				info->p_va_indep_win_info[i].win_szx = indep_roi[i].w;
				info->p_va_indep_win_info[i].win_szy = indep_roi[i].h;

				//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "indep va [%d] (x,y,w,h) = (%d,%d,%d,%d)\r\n", (int)i, (int)indep_roi[i].x, (int)indep_roi[i].y, (int)indep_roi[i].w, (int)indep_roi[i].h);
			}
		}
		info->param_update_sel |= (IPE_SET_INDEP_VA);
	} else {
		info->param_update_sel &= (~IPE_SET_INDEP_VA);
	}

	if (dbg_printed == TRUE) {
		ipe_dbg_cmd &= ~(IPE_DBG_DEFAULTON);
	}

	if (info->param_update_sel & (IPE_SET_VA | IPE_SET_INDEP_VA)) {

		info->p_va_filt_g1->filt_h.fltr_size = g_kdrv_ipe_info[id].va_param.group_1.h_filt.filter_size;
		info->p_va_filt_g1->filt_h.filt_symm = g_kdrv_ipe_info[id].va_param.group_1.h_filt.symmetry;
		info->p_va_filt_g1->filt_h.tap_a = g_kdrv_ipe_info[id].va_param.group_1.h_filt.tap_a;
		info->p_va_filt_g1->filt_h.tap_b = g_kdrv_ipe_info[id].va_param.group_1.h_filt.tap_b;
		info->p_va_filt_g1->filt_h.tap_c = g_kdrv_ipe_info[id].va_param.group_1.h_filt.tap_c;
		info->p_va_filt_g1->filt_h.tap_d = g_kdrv_ipe_info[id].va_param.group_1.h_filt.tap_d;
		info->p_va_filt_g1->filt_h.div = g_kdrv_ipe_info[id].va_param.group_1.h_filt.div;
		info->p_va_filt_g1->filt_h.th_low = g_kdrv_ipe_info[id].va_param.group_1.h_filt.th_l;
		info->p_va_filt_g1->filt_h.th_high = g_kdrv_ipe_info[id].va_param.group_1.h_filt.th_u;

		info->p_va_filt_g1->filt_v.fltr_size = g_kdrv_ipe_info[id].va_param.group_1.v_filt.filter_size;
		info->p_va_filt_g1->filt_v.filt_symm = g_kdrv_ipe_info[id].va_param.group_1.v_filt.symmetry;
		info->p_va_filt_g1->filt_v.tap_a = g_kdrv_ipe_info[id].va_param.group_1.v_filt.tap_a;
		info->p_va_filt_g1->filt_v.tap_b = g_kdrv_ipe_info[id].va_param.group_1.v_filt.tap_b;
		info->p_va_filt_g1->filt_v.tap_c = g_kdrv_ipe_info[id].va_param.group_1.v_filt.tap_c;
		info->p_va_filt_g1->filt_v.tap_d = g_kdrv_ipe_info[id].va_param.group_1.v_filt.tap_d;
		info->p_va_filt_g1->filt_v.div = g_kdrv_ipe_info[id].va_param.group_1.v_filt.div;
		info->p_va_filt_g1->filt_v.th_low = g_kdrv_ipe_info[id].va_param.group_1.v_filt.th_l;
		info->p_va_filt_g1->filt_v.th_high = g_kdrv_ipe_info[id].va_param.group_1.v_filt.th_u;

		info->p_va_filt_g2->filt_h.fltr_size = g_kdrv_ipe_info[id].va_param.group_2.h_filt.filter_size;
		info->p_va_filt_g2->filt_h.filt_symm = g_kdrv_ipe_info[id].va_param.group_2.h_filt.symmetry;
		info->p_va_filt_g2->filt_h.tap_a = g_kdrv_ipe_info[id].va_param.group_2.h_filt.tap_a;
		info->p_va_filt_g2->filt_h.tap_b = g_kdrv_ipe_info[id].va_param.group_2.h_filt.tap_b;
		info->p_va_filt_g2->filt_h.tap_c = g_kdrv_ipe_info[id].va_param.group_2.h_filt.tap_c;
		info->p_va_filt_g2->filt_h.tap_d = g_kdrv_ipe_info[id].va_param.group_2.h_filt.tap_d;
		info->p_va_filt_g2->filt_h.div = g_kdrv_ipe_info[id].va_param.group_2.h_filt.div;
		info->p_va_filt_g2->filt_h.th_low = g_kdrv_ipe_info[id].va_param.group_2.h_filt.th_l;
		info->p_va_filt_g2->filt_h.th_high = g_kdrv_ipe_info[id].va_param.group_2.h_filt.th_u;

		info->p_va_filt_g2->filt_v.fltr_size = g_kdrv_ipe_info[id].va_param.group_2.v_filt.filter_size;
		info->p_va_filt_g2->filt_v.filt_symm = g_kdrv_ipe_info[id].va_param.group_2.v_filt.symmetry;
		info->p_va_filt_g2->filt_v.tap_a = g_kdrv_ipe_info[id].va_param.group_2.v_filt.tap_a;
		info->p_va_filt_g2->filt_v.tap_b = g_kdrv_ipe_info[id].va_param.group_2.v_filt.tap_b;
		info->p_va_filt_g2->filt_v.tap_c = g_kdrv_ipe_info[id].va_param.group_2.v_filt.tap_c;
		info->p_va_filt_g2->filt_v.tap_d = g_kdrv_ipe_info[id].va_param.group_2.v_filt.tap_d;
		info->p_va_filt_g2->filt_v.div = g_kdrv_ipe_info[id].va_param.group_2.v_filt.div;
		info->p_va_filt_g2->filt_v.th_low = g_kdrv_ipe_info[id].va_param.group_2.v_filt.th_l;
		info->p_va_filt_g2->filt_v.th_high = g_kdrv_ipe_info[id].va_param.group_2.v_filt.th_u;
	}

	return rt;
}

static ER kdrv_ipe_set_iq_eth(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	if (g_kdrv_ipe_info[id].eth.enable) {
		info->p_eth->eth_low = g_kdrv_ipe_info[id].eth.th_low;
		info->p_eth->eth_mid = g_kdrv_ipe_info[id].eth.th_mid;
		info->p_eth->eth_high = g_kdrv_ipe_info[id].eth.th_high;
		info->p_eth->eth_hout = g_kdrv_ipe_info[id].eth.outsel_h;
		info->p_eth->eth_vout = g_kdrv_ipe_info[id].eth.outsel_v;

		info->param_update_sel |= IPE_SET_ETH;
	} else {
		info->param_update_sel &= (~IPE_SET_ETH);
	}

	return rt;
}

static ER kdrv_ipe_set_iq_yuvcvt_for_yuvin(INT32 id, IPE_IQINFO *info)
{

	UINT32 i = 0, j = 0, k = 0, cvt_idx = 0;
	UINT32 bitshift = 0;
	INT16 *p_c_coef;
	INT32 ccmatrix[KDRV_IPE_COEF_LEN] = {0};
	BOOL overflow = FALSE;

	cvt_idx = (UINT32)(g_kdrv_ipe_info[id].in_img_info.yuv_range_fmt);

	if (g_kdrv_ipe_info[id].cc_en_param.enable) {

		p_c_coef = &(g_kdrv_ipe_info[id].ccm_param.coef[0]);

		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++) {
				ccmatrix[3 * i + j] = 0;
				for (k = 0; k < 3; k++) {
					ccmatrix[3 * i + j] += p_c_coef[3 * i + k] * g_cc_cvt_range[cvt_idx][3 * k + j];
				}
				if ((abs(ccmatrix[3 * i + j]) >> 8) > (2047)) {
					overflow = TRUE;
				}
			}
		}

		if (overflow && (g_kdrv_ipe_info[id].ccm_param.cc_range != KDRV_IPE_CCRANGE_4_7)) {
			//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_CCM), "overflow\r\n");
			bitshift = 9;
			info->p_cc_param->cc_range = g_kdrv_ipe_info[id].ccm_param.cc_range + 1;
		} else {
			bitshift = 8;
			info->p_cc_param->cc_range = g_kdrv_ipe_info[id].ccm_param.cc_range;
		}

		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_CCM), "ccm: \r\n");
		for (i = 0; i < KDRV_IPE_COEF_LEN; i++) {
			g_cc_coef[i] = clamp(((ccmatrix[i]  + (1 << (bitshift - 1))) >> bitshift), -2048, 2047);
			//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_CCM), "%d, ", (int)g_cc_coef[i]);
		}
		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_CCM), "\r\n");
		info->p_cc_param->cc2_sel = g_kdrv_ipe_info[id].cc_param.cc2_sel;
		info->p_cc_param->cc_gam_sel = g_kdrv_ipe_info[id].ccm_param.cc_gamma_sel;
		info->p_cc_param->cc_stab_sel = g_kdrv_ipe_info[id].cc_param.cc_stab_sel;
		info->p_cc_param->p_cc_coeff = &(g_cc_coef[0]);
		info->p_cc_param->p_fstab = &(g_kdrv_ipe_info[id].cc_param.fstab[0]);
		info->p_cc_param->p_fdtab = &(g_kdrv_ipe_info[id].cc_param.fdtab[0]);

	} else {

		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_CCM), "ccm: \r\n");
		for (i = 0; i < KDRV_IPE_COEF_LEN; i++) {
			g_cc_coef[i] = g_cc_cvt_range[cvt_idx][i];
			//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_CCM), "%d ,", (int)g_cc_coef[i]);
		}
		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_CCM), "\r\n");

		info->p_cc_param->cc_range = CCRANGE_3_8;
		info->p_cc_param->cc2_sel = CC2_IDENTITY;
		info->p_cc_param->cc_gam_sel = CC_PRE_GAM;
		info->p_cc_param->cc_stab_sel = CC_MAX;
		info->p_cc_param->p_cc_coeff = &(g_cc_coef[0]);
		info->p_cc_param->p_fstab = &(fstab_init[0]);
		info->p_cc_param->p_fdtab = &(fdtab_init[0]);

	}

	//cc_ofs
	switch (g_kdrv_ipe_info[id].in_img_info.yuv_range_fmt) {
	case KDRV_IPE_YUV_FMT_FULL:
	default:
		info->p_cc_param->cc_ofs_sel = CC_OFS_Y_FULL;
		break ;
	case KDRV_IPE_YUV_FMT_BT601:
	case KDRV_IPE_YUV_FMT_BT709:
		info->p_cc_param->cc_ofs_sel = CC_OFS_Y_BTU;
		break ;
	}

	info->param_update_sel |= IPE_SET_COLORCORRECT;

	ipe_dbg_cmd &= ~(IPE_DBG_CCM);


	return E_OK;
}


static ER kdrv_ipe_set_iq_edge_region_strength(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	if (g_kdrv_ipe_info[id].edge_region_param.enable) {
        info->p_edge_region_param->region_str_en  = g_kdrv_ipe_info[id].edge_region_param.enable;
        info->p_edge_region_param->enh_thin       = g_kdrv_ipe_info[id].edge_region_param.enh_thin;
        info->p_edge_region_param->enh_robust     = g_kdrv_ipe_info[id].edge_region_param.enh_robust;
        info->p_edge_region_param->slope_flat     = g_kdrv_ipe_info[id].edge_region_param.slope_flat;
        info->p_edge_region_param->str_flat       = g_kdrv_ipe_info[id].edge_region_param.str_flat;
        info->p_edge_region_param->slope_edge     = g_kdrv_ipe_info[id].edge_region_param.slope_edge;
        info->p_edge_region_param->str_edge       = g_kdrv_ipe_info[id].edge_region_param.str_edge;		

		info->param_update_sel |= (IPE_SET_EDGE_REGION_STR);
	} else {
		info->param_update_sel &= (~IPE_SET_EDGE_REGION_STR);
	}

	return rt;
}


#if 0
static ER kdrv_ipe_set_iq_funcsel_yuvin(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	//todo: SC
	if (info->param_update_sel & (IPE_SET_COLORCORRECT)) {
		DBG_WRN("Color correction is not supported for D2D yuv in\r\n");
	}
	if (info->param_update_sel & (IPE_SET_GAMMA)) {
		DBG_WRN("Gamma is not supported for D2D yuv in\r\n");
	}
	if (info->param_update_sel & (IPE_SET_YCURVE)) {
		DBG_WRN("Ycurve is not supported for D2D yuv in\r\n");
	}
	/*
	if (info->param_update_sel & (IPE_SET_CC3D)) {
	    DBG_WRN("3DCC is not supported for D2D yuv in\r\n");
	}*/
	if (info->param_update_sel & (IPE_SET_CCTRL)) {
		DBG_WRN("Color control is not supported for D2D yuv in\r\n");
	}
	if (info->param_update_sel & (IPE_SET_CST)) {
		DBG_WRN("Color space transform is not supported for D2D yuv in\r\n");
	}

	info->param_update_sel &= (~IPE_SET_COLORCORRECT);
	info->param_update_sel &= (~IPE_SET_GAMMA);
	info->param_update_sel &= (~IPE_SET_YCURVE);
	//info->param_update_sel &= (~IPE_SET_CC3D);
	info->param_update_sel &= (~IPE_SET_CCTRL);
	info->param_update_sel &= (~IPE_SET_CST);

	return rt;
}
#endif

static ER kdrv_ipe_set_iq_funcsel(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;
	UINT32 update_sel = info->param_update_sel;

	//DBG_USER("update_sel 0x%08x\r\n", (unsigned int)update_sel);

	if (g_kdrv_ipe_info[id].rgb_lpf_param.enable) {
		info->func_sel |= IPE_RGBLPF_EN;
	} else {
		info->func_sel &= (~IPE_RGBLPF_EN);
	}

	if (g_kdrv_ipe_info[id].pfr_param.enable) {
		info->func_sel |= IPE_PFR_EN;
	} else {
		info->func_sel &= (~IPE_PFR_EN);
	}

	if (g_kdrv_ipe_info[id].cc_en_param.enable) {
		info->func_sel |= IPE_CCR_EN;
	} else {
		if ((g_kdrv_ipe_info[id].in_img_info.in_src == KDRV_IPE_IN_SRC_DRAM) && (update_sel & IPE_SET_COLORCORRECT)) {
			info->func_sel |= IPE_CCR_EN;
		} else {
			info->func_sel &= (~IPE_CCR_EN);
		}
	}

	if (g_kdrv_ipe_info[id].cst_param.enable) {
		info->func_sel |= IPE_CST_EN;
	} else {
		info->func_sel &= (~IPE_CST_EN);
	}

	if (g_kdrv_ipe_info[id].cctrl_en_param.enable) {
		info->func_sel |= IPE_CTRL_EN;
	} else {
		info->func_sel &= (~IPE_CTRL_EN);
	}

	if (g_kdrv_ipe_info[id].cadj_ee_param.enable) {
		info->func_sel |= IPE_CADJ_YENH_EN;
	} else {
		info->func_sel &= (~IPE_CADJ_YENH_EN);
	}

	if (g_kdrv_ipe_info[id].cadj_yccon_param.enable) {
		info->func_sel |= IPE_CADJ_YCON_EN;
	} else {
		info->func_sel &= (~IPE_CADJ_YCON_EN);
	}

	if (g_kdrv_ipe_info[id].cadj_yccon_param.enable) {
		info->func_sel |= IPE_CADJ_CCON_EN;
	} else {
		info->func_sel &= (~IPE_CADJ_CCON_EN);
	}

	if (g_kdrv_ipe_info[id].cadj_fixth_param.enable) {
		info->func_sel |= IPE_YCTH_EN;
	} else {
		info->func_sel &= (~IPE_YCTH_EN);
	}

	if ((g_kdrv_ipe_info[id].cadj_ee_param.enable)
		|| (g_kdrv_ipe_info[id].cadj_yccon_param.enable) || (g_kdrv_ipe_info[id].cadj_cofs_param.enable)
		|| (g_kdrv_ipe_info[id].cadj_rand_param.enable) || (g_kdrv_ipe_info[id].cadj_fixth_param.enable)
		|| (g_kdrv_ipe_info[id].cadj_mask_param.enable) || (g_kdrv_ipe_info[id].edge_region_param.enable)) {
		info->func_sel |= IPE_CADJ_EN;
	} else {
		info->func_sel &= (~IPE_CADJ_EN);
	}

	if (g_kdrv_ipe_info[id].cstp_param.enable) {
		info->func_sel |= IPE_CSTP_EN;
	} else {
		info->func_sel &= (~IPE_CSTP_EN);
	}

	if (g_kdrv_ipe_info[id].dfg_param.enable) {
		info->func_sel |= IPE_DEFOG_EN;
	} else {
		info->func_sel &= (~IPE_DEFOG_EN);
	}

	if (g_kdrv_ipe_info[id].subout_dma_addr.enable) {
		info->func_sel |= IPE_DEFOG_SUBOUT_EN;
	} else {
		info->func_sel &= (~IPE_DEFOG_SUBOUT_EN);
	}

	if (g_kdrv_ipe_info[id].lce_param.enable) {
		info->func_sel |= IPE_LCE_EN;
	} else {
		info->func_sel &= (~IPE_LCE_EN);
	}

	if (g_kdrv_ipe_info[id].gamma.enable) {
		info->func_sel |= IPE_RGBGAMMA_EN;
	} else {
		info->func_sel &= (~IPE_RGBGAMMA_EN);
	}

	/*if (update_sel & IPE_SET_CC3D) {
	    info->func_sel |= IPE_CC3D_EN;
	} else {
	    info->func_sel &= (~IPE_CC3D_EN);
	}*/

	if (g_kdrv_ipe_info[id].y_curve.enable) {
		info->func_sel |= IPE_YCURVE_EN;
	} else {
		info->func_sel &= (~IPE_YCURVE_EN);
	}

	if (g_kdrv_ipe_info[id].cadj_hue_param.enable) {
		info->func_sel |= IPE_HADJ_EN;
	} else {
		info->func_sel &= (~IPE_HADJ_EN);
	}

	if (g_kdrv_ipe_info[id].edgdbg_param.enable) {
		info->func_sel |= IPE_EDGE_DBG_EN;
	} else {
		info->func_sel &= (~IPE_EDGE_DBG_EN);
	}

	if (g_kdrv_ipe_info[id].va_param.enable) {
		info->func_sel |= IPE_VACC_EN;
	} else {
		info->func_sel &= (~IPE_VACC_EN);
	}

	if (g_kdrv_ipe_info[id].va_param.indep_va_enable && g_kdrv_ipe_info[id].va_param.indep_win[0].enable) {
		info->func_sel |= IPE_VA_IDNEP_WIN0_EN;
	} else {
		info->func_sel &= (~IPE_VA_IDNEP_WIN0_EN);
	}

	if (g_kdrv_ipe_info[id].va_param.indep_va_enable && g_kdrv_ipe_info[id].va_param.indep_win[1].enable) {
		info->func_sel |= IPE_VA_IDNEP_WIN1_EN;
	} else {
		info->func_sel &= (~IPE_VA_IDNEP_WIN1_EN);
	}

	if (g_kdrv_ipe_info[id].va_param.indep_va_enable && g_kdrv_ipe_info[id].va_param.indep_win[2].enable) {
		info->func_sel |= IPE_VA_IDNEP_WIN2_EN;
	} else {
		info->func_sel &= (~IPE_VA_IDNEP_WIN2_EN);
	}

	if (g_kdrv_ipe_info[id].va_param.indep_va_enable && g_kdrv_ipe_info[id].va_param.indep_win[3].enable) {
		info->func_sel |= IPE_VA_IDNEP_WIN3_EN;
	} else {
		info->func_sel &= (~IPE_VA_IDNEP_WIN3_EN);
	}

	if (g_kdrv_ipe_info[id].va_param.indep_va_enable && g_kdrv_ipe_info[id].va_param.indep_win[4].enable) {
		info->func_sel |= IPE_VA_IDNEP_WIN4_EN;
	} else {
		info->func_sel &= (~IPE_VA_IDNEP_WIN4_EN);
	}

	return rt;
}

static ER kdrv_ipe_load_iq_data(INT32 id, IPE_IQINFO *info, BOOL wait_end)
{
	ER rt = E_OK;
	KDRV_IPE_GAMMA_OPTION input_option;

	// need load data after ipe_setMode
	UINT32 update_sel;
	//DBG_USER("driver update_sel 0x%08x\r\n", (int)update_sel);


	input_option = g_kdrv_ipe_info[id].gamma.option;
	update_sel = info->param_update_sel;

	if (ipe_ctrl_flow_to_do == DISABLE) {  // for fastboot flow control
		return E_OK;
	}

	// load data
	/*if (update_sel & IPE_SET_CC3D) {
	    ipe_clr3DLutInEnd();
	    ipe_load3DLut();
	}
	*/
	if ((update_sel & IPE_SET_GAMMA) && (update_sel & IPE_SET_YCURVE)) {
		if (input_option == KDRV_IPE_GAMMA_RGB_SEPERATE) {
			/* seperate gamma + y_curve, load RGB gamma and y_curve seperatly */
			ipe_clr_gamma_in_end();
			ipe_load_gamma(LOAD_GAMMA_RGB);
			ipe_wait_gamma_in_end();

			ipe_clr_gamma_in_end();
			ipe_load_gamma(LOAD_GAMMA_Y);
			ipe_wait_gamma_in_end();
		} else {
			if (wait_end) {
				ipe_clr_gamma_in_end();
			}
			ipe_load_gamma(LOAD_GAMMA_R_Y);
		}
	} else {
		if (update_sel & IPE_SET_GAMMA) {
			if (wait_end) {
				ipe_clr_gamma_in_end();
			}
			ipe_load_gamma(LOAD_GAMMA_RGB);
		}
		if (update_sel & IPE_SET_YCURVE) {
			if (wait_end) {
				ipe_clr_gamma_in_end();
			}
			ipe_load_gamma(LOAD_GAMMA_Y);
		}
	}

	// wait load data end
	if (wait_end) {
		/*if (update_sel & IPE_SET_CC3D) {
		    ipe_wait3DLutInEnd();
		}*/

		if ((update_sel & IPE_SET_GAMMA) && (update_sel & IPE_SET_YCURVE)) {
			if (input_option == KDRV_IPE_GAMMA_RGB_SEPERATE) {
				/* do nothing */
			} else {
				ipe_wait_gamma_in_end();
			}
		} else {
			if (update_sel & IPE_SET_GAMMA) {
				ipe_wait_gamma_in_end();
				//DBG_USER("gamma end\r\n");
			}
			if (update_sel & IPE_SET_YCURVE) {
				ipe_wait_gamma_in_end();
			}
		}
	}

	return rt;
}

static ER kdrv_ipe_set_iq_param(INT32 id, IPE_IQINFO *info)
{
	ER rt = E_OK;

	// update parameter & param_update_sel
	rt |= kdrv_ipe_set_iq_eext(id, info);
	rt |= kdrv_ipe_set_iq_eproc(id, info);
	rt |= kdrv_ipe_set_iq_rgblpf(id, info);
	rt |= kdrv_ipe_set_iq_pfr(id, info);

	rt |= kdrv_ipe_set_iq_cc(id, info);
	rt |= kdrv_ipe_set_iq_cctrl(id, info);
	rt |= kdrv_ipe_set_iq_cadjee(id, info);
	rt |= kdrv_ipe_set_iq_cadjyccon(id, info);
	rt |= kdrv_ipe_set_iq_cadjcofs(id, info);
	rt |= kdrv_ipe_set_iq_cadjrand(id, info);
	rt |= kdrv_ipe_set_iq_cadjfixth(id, info);
	rt |= kdrv_ipe_set_iq_cadjmask(id, info);
	rt |= kdrv_ipe_set_iq_cst(id, info);
	rt |= kdrv_ipe_set_iq_cstp(id, info);

	rt |= kdrv_ipe_set_iq_gammarand(id, info);
	rt |= kdrv_ipe_set_iq_gammacurve(id, info);
	rt |= kdrv_ipe_set_iq_ycurve(id, info);
	rt |= kdrv_ipe_set_iq_3dcc(id, info);

	rt |= kdrv_ipe_set_iq_defog_subimg(id, info);
	rt |= kdrv_ipe_set_iq_defog(id, info); // include defog rand
	rt |= kdrv_ipe_set_iq_lce(id, info);
	rt |= kdrv_ipe_set_iq_edgedbg_moe(id, info);

	rt |= kdrv_ipe_set_iq_eth(id, info);

	rt |= kdrv_ipe_set_iq_va(id, info);

	rt |= kdrv_ipe_set_iq_edge_region_strength(id, info);

	// add yuv converter for yuv in
	if (g_kdrv_ipe_info[id].in_img_info.in_src == KDRV_IPE_IN_SRC_DRAM) {
		kdrv_ipe_set_iq_yuvcvt_for_yuvin(id, info);
	}

	// disble functions for d2d yuv input
	//if (g_kdrv_ipe_info[id].in_img_info.in_src == KDRV_IPE_IN_SRC_DRAM) {
	//  kdrv_ipe_set_iq_funcsel_yuvin(id, info);
	//}

	// update func_sel & FuncUpdateSel
	rt |= kdrv_ipe_set_iq_funcsel(id, info);

	info->func_update_sel = IPE_FUNC_SET;

	return rt;
}

static ER kdrv_ipe_setmode(INT32 id)
{
	ER rt = E_OK;
	UINT32 i = 0;
	//unsigned long loc_status;

	uint8_t  rgblpf_en, cc_en, cst_en, cctrl_en, cadj_ee_en, cadj_ycon_en, cadj_cofs_en, cadj_rand_en;
	uint8_t  cadj_hue_en, cadj_fixth_en, cadj_mask_en, cstp_en, gamy_rand_en, gamma_en, ycurve_en;//, cc3d_en;
	uint8_t  pfr_en, defog_en, lce_en, edgedbg_en, va_en, va_indep_en;
	uint8_t  edge_region_str_en;

	uint8_t  rgblpf_iq_en, cc_iq_en, cst_iq_en, cctrl_iq_en, cadj_ee_iq_en, cadj_ycon_iq_en, cadj_cofs_iq_en, cadj_rand_iq_en;
	uint8_t  cadj_hue_iq_en, cadj_fixth_iq_en, cadj_mask_iq_en, cstp_iq_en, gamy_rand_iq_en, gamma_iq_en, ycurve_iq_en;//, cc3d_iq_en;
	uint8_t  pfr_iq_en, defog_iq_en, lce_iq_en, edgedbg_iq_en, va_iq_en, va_indep_iq_en;
	uint8_t  edge_region_str_iq_en;
	

	//loc_status = ipe_platform_spin_lock();

	// and IPL enable/disable setting
	rgblpf_en     = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_RGBLPF) ? TRUE : FALSE);
	cc_en         = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CC) ? TRUE : FALSE);
	cst_en        = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CST) ? TRUE : FALSE);
	cctrl_en      = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CCTRL) ? TRUE : FALSE);
	cadj_ee_en    = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CADJ_EE) ? TRUE : FALSE);
	cadj_ycon_en  = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CADJ_YCCON) ? TRUE : FALSE);
	cadj_cofs_en  = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CADJ_COFS) ? TRUE : FALSE);
	cadj_rand_en  = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CADJ_RAND) ? TRUE : FALSE);
	cadj_hue_en   = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CADJ_HUE) ? TRUE : FALSE);
	cadj_fixth_en = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CADJ_FIXTH) ? TRUE : FALSE);
	cadj_mask_en  = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CADJ_MASK) ? TRUE : FALSE);
	cstp_en       = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CSTP) ? TRUE : FALSE);
	gamy_rand_en  = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_GAMMA_RAND) ? TRUE : FALSE);
	gamma_en      = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_GAMMA) ? TRUE : FALSE);
	ycurve_en     = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_YCURVE) ? TRUE : FALSE);
	//cc3d_en     = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CC3D) ? TRUE : FALSE);
	pfr_en        = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_PFR) ? TRUE : FALSE);
	defog_en      = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_DEFOG) ? TRUE : FALSE);
	lce_en        = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_LCE) ? TRUE : FALSE);
	edgedbg_en    = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_EDGEDBG) ? TRUE : FALSE);
	va_en         = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_VA) ? TRUE : FALSE);
	va_indep_en   = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_VA_INDEP) ? TRUE : FALSE);
	edge_region_str_en = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_EDGE_REGION_STR) ? TRUE : FALSE);


	rgblpf_iq_en     = g_kdrv_ipe_iq_en_info[id].rgb_lpf_param_enable;
	cc_iq_en         = g_kdrv_ipe_iq_en_info[id].cc_en_param_enable;
	cst_iq_en        = g_kdrv_ipe_iq_en_info[id].cst_param_enable;
	cctrl_iq_en      = g_kdrv_ipe_iq_en_info[id].cctrl_en_param_enable;
	cadj_ee_iq_en    = g_kdrv_ipe_iq_en_info[id].cadj_ee_param_enable;
	cadj_ycon_iq_en  = g_kdrv_ipe_iq_en_info[id].cadj_yccon_param_enable;
	cadj_cofs_iq_en  = g_kdrv_ipe_iq_en_info[id].cadj_cofs_param_enable;
	cadj_rand_iq_en  = g_kdrv_ipe_iq_en_info[id].cadj_rand_param_enable;
	cadj_hue_iq_en   = g_kdrv_ipe_iq_en_info[id].cadj_hue_param_enable;
	cadj_fixth_iq_en = g_kdrv_ipe_iq_en_info[id].cadj_fixth_param_enable;
	cadj_mask_iq_en  = g_kdrv_ipe_iq_en_info[id].cadj_mask_param_enable;
	cstp_iq_en       = g_kdrv_ipe_iq_en_info[id].cstp_param_enable;
	gamy_rand_iq_en  = g_kdrv_ipe_iq_en_info[id].gamy_rand_param_enable;
	gamma_iq_en      = g_kdrv_ipe_iq_en_info[id].gamma_enable;
	ycurve_iq_en     = g_kdrv_ipe_iq_en_info[id].y_curve_enable;
	//cc3d_iq_en       = (g_kdrv_ipe_ipl_func_iq_en_sel[id] & KDRV_IPE_IQ_FUNC_CC3D) ? TRUE : FALSE;
	pfr_iq_en        = g_kdrv_ipe_iq_en_info[id].pfr_param_enable;
	defog_iq_en      = g_kdrv_ipe_iq_en_info[id].dfg_param_enable;
	lce_iq_en        = g_kdrv_ipe_iq_en_info[id].lce_param_enable;
	edgedbg_iq_en    = g_kdrv_ipe_iq_en_info[id].edgdbg_param_enable;
	va_iq_en         = g_kdrv_ipe_iq_en_info[id].va_param_enable;
	va_indep_iq_en   = g_kdrv_ipe_iq_en_info[id].va_param_indep_va_enable;
	edge_region_str_iq_en = g_kdrv_ipe_iq_en_info[id].edge_region_str_enable;


	g_kdrv_ipe_info[id].rgb_lpf_param.enable     = (BOOL)(rgblpf_iq_en & rgblpf_en);
	g_kdrv_ipe_info[id].cc_en_param.enable       = (BOOL)(cc_iq_en & cc_en);
	g_kdrv_ipe_info[id].cst_param.enable         = (BOOL)(cst_iq_en & cst_en);
	g_kdrv_ipe_info[id].cctrl_en_param.enable    = (BOOL)(cctrl_iq_en & cctrl_en);
	g_kdrv_ipe_info[id].cadj_ee_param.enable     = (BOOL)(cadj_ee_iq_en & cadj_ee_en);
	g_kdrv_ipe_info[id].cadj_yccon_param.enable  = (BOOL)(cadj_ycon_iq_en & cadj_ycon_en);
	g_kdrv_ipe_info[id].cadj_cofs_param.enable   = (BOOL)(cadj_cofs_iq_en & cadj_cofs_en);
	g_kdrv_ipe_info[id].cadj_rand_param.enable   = (BOOL)(cadj_rand_iq_en & cadj_rand_en);
	g_kdrv_ipe_info[id].cadj_hue_param.enable    = (BOOL)(cadj_hue_iq_en & cadj_hue_en);
	g_kdrv_ipe_info[id].cadj_fixth_param.enable  = (BOOL)(cadj_fixth_iq_en & cadj_fixth_en);
	g_kdrv_ipe_info[id].cadj_mask_param.enable   = (BOOL)(cadj_mask_iq_en & cadj_mask_en);
	g_kdrv_ipe_info[id].cstp_param.enable        = (BOOL)(cstp_iq_en & cstp_en);
	g_kdrv_ipe_info[id].gamy_rand_param.enable   = (BOOL)(gamy_rand_iq_en & gamy_rand_en);
	g_kdrv_ipe_info[id].gamma.enable             = (BOOL)(gamma_iq_en & gamma_en);
	g_kdrv_ipe_info[id].y_curve.enable           = (BOOL)(ycurve_iq_en & ycurve_en);
	//g_kdrv_ipe_info[id]._3dcc_param.enable     = (BOOL)(cc3d_iq_en & cc3d_en);
	g_kdrv_ipe_info[id].pfr_param.enable         = (BOOL)(pfr_iq_en & pfr_en);
	g_kdrv_ipe_info[id].dfg_param.enable         = (BOOL)(defog_iq_en & defog_en);
	g_kdrv_ipe_info[id].lce_param.enable         = (BOOL)(lce_iq_en & lce_en);
	g_kdrv_ipe_info[id].edgdbg_param.enable      = (BOOL)(edgedbg_iq_en & edgedbg_en);
	g_kdrv_ipe_info[id].va_param.enable          = (BOOL)(va_iq_en & va_en);
	g_kdrv_ipe_info[id].va_param.indep_va_enable = (BOOL)(va_indep_iq_en & va_indep_en);
	g_kdrv_ipe_info[id].edge_region_param.enable = (BOOL)(edge_region_str_iq_en & edge_region_str_en);

#if 0
	g_kdrv_ipe_info[id].rgb_lpf_param.enable     &= rgblpf_en;
	g_kdrv_ipe_info[id].cc_en_param.enable       &= cc_en;
	g_kdrv_ipe_info[id].cst_param.enable         &= cst_en;
	g_kdrv_ipe_info[id].cctrl_en_param.enable    &= cctrl_en;
	g_kdrv_ipe_info[id].cadj_ee_param.enable     &= cadj_ee_en;
	g_kdrv_ipe_info[id].cadj_yccon_param.enable  &= cadj_ycon_en;
	g_kdrv_ipe_info[id].cadj_cofs_param.enable   &= cadj_cofs_en;
	g_kdrv_ipe_info[id].cadj_rand_param.enable   &= cadj_rand_en;
	g_kdrv_ipe_info[id].cadj_hue_param.enable    &= cadj_hue_en;
	g_kdrv_ipe_info[id].cadj_fixth_param.enable  &= cadj_fixth_en;
	g_kdrv_ipe_info[id].cadj_mask_param.enable   &= cadj_mask_en;
	g_kdrv_ipe_info[id].cstp_param.enable        &= cstp_en;
	g_kdrv_ipe_info[id].gamy_rand_param.enable   &= gamy_rand_en;
	g_kdrv_ipe_info[id].gamma.enable             &= gamma_en;
	g_kdrv_ipe_info[id].y_curve.enable           &= ycurve_en;
	//g_kdrv_ipe_info[id]._3dcc_param.enable       &= cc3d_en;
	g_kdrv_ipe_info[id].pfr_param.enable         &= pfr_en;
	g_kdrv_ipe_info[id].dfg_param.enable         &= defog_en;
	g_kdrv_ipe_info[id].lce_param.enable         &= lce_en;
	g_kdrv_ipe_info[id].edgdbg_param.enable      &= edgedbg_en;
	g_kdrv_ipe_info[id].va_param.enable          &= va_en;
	g_kdrv_ipe_info[id].va_param.indep_va_enable &= va_indep_en;
#endif

	//DBG_USER("ipe kdrv setmode \n");

	//kdrv_ipe_memset_drv_param();

	// clear load flag
	if (ipe_ctrl_flow_to_do == ENABLE) {

		for (i = 0; i < KDRV_IPE_PARAM_MAX; i++) {
			g_kdrv_ipe_load_flg[id][i] = FALSE;
		}
	}

	// interrupt enable
	//g_kdrv_ipe_info[id].inte_en = KDRV_IPE_INTE_ALL;
	g_ipe.intr_en = g_kdrv_ipe_info[id].inte_en;

	//ipe_platform_spin_unlock(loc_status);


	rt |= kdrv_ipe_check_param(id);

	// set input/output parameters
	rt |= kdrv_ipe_set_input_param(id, &g_ipe.in_info);
	rt |= kdrv_ipe_set_output_param(id, &g_ipe.in_info, &g_ipe.out_info);

	// set IQ parameters
	rt |= kdrv_ipe_set_iq_param(id, &g_ipe.iq_info);

	// set to IPE engine
	rt |= ipe_set_mode(&g_ipe);

	// load IQ parameters
	rt |= kdrv_ipe_load_iq_data(id, &g_ipe.iq_info, TRUE);

	return rt;
}


KDRV_IPE_SET_LOAD_FP kdrv_ipe_set_load_fp[DRV_IPE_SETLAOD_IQ_MAX] = {
	kdrv_ipe_set_iq_eext,
	kdrv_ipe_set_iq_eproc,
	kdrv_ipe_set_iq_rgblpf,
	kdrv_ipe_set_iq_pfr,
	kdrv_ipe_set_iq_cc,
	kdrv_ipe_set_iq_cctrl,
	kdrv_ipe_set_iq_cadjee,
	kdrv_ipe_set_iq_cadjyccon,
	kdrv_ipe_set_iq_cadjcofs,
	kdrv_ipe_set_iq_cadjrand,
	NULL,
	kdrv_ipe_set_iq_cadjfixth,
	kdrv_ipe_set_iq_cadjmask,
	kdrv_ipe_set_iq_cst,
	kdrv_ipe_set_iq_cstp,
	kdrv_ipe_set_iq_gammarand,
	kdrv_ipe_set_iq_gammacurve,
	kdrv_ipe_set_iq_ycurve,
	kdrv_ipe_set_iq_3dcc,
	kdrv_ipe_set_iq_defog_subimg,
	kdrv_ipe_set_iq_defog,
	kdrv_ipe_set_iq_lce,
	kdrv_ipe_set_iq_edgedbg_moe,
	kdrv_ipe_set_iq_eth,
	kdrv_ipe_set_iq_va,
	kdrv_ipe_set_iq_edge_region_strength,
};

static ER kdrv_ipe_set_load(UINT32 id, void *data)
{

	ER rt = E_OK;
	UINT32 i = 0, map_id = 0;
	BOOL kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_MAX] = {0};

	IPE_INPUTINFO *p_in_info = &g_ipe.in_info;
	IPE_OUTYCINFO *p_out_info = &g_ipe.out_info;
	IPE_IQINFO *p_iq_info = &g_ipe.iq_info;

	uint8_t  rgblpf_en, cc_en, cst_en, cctrl_en, cadj_ee_en, cadj_ycon_en, cadj_cofs_en, cadj_rand_en;
	uint8_t  cadj_hue_en, cadj_fixth_en, cadj_mask_en, cstp_en, gamy_rand_en, gamma_en, ycurve_en;//, cc3d_en;
	uint8_t  pfr_en, defog_en, lce_en, edgedbg_en, va_en, va_indep_en;
	uint8_t  edge_region_str_en;

	uint8_t  rgblpf_iq_en, cc_iq_en, cst_iq_en, cctrl_iq_en, cadj_ee_iq_en, cadj_ycon_iq_en, cadj_cofs_iq_en, cadj_rand_iq_en;
	uint8_t  cadj_hue_iq_en, cadj_fixth_iq_en, cadj_mask_iq_en, cstp_iq_en, gamy_rand_iq_en, gamma_iq_en, ycurve_iq_en;//, cc3d_iq_en;
	uint8_t  pfr_iq_en, defog_iq_en, lce_iq_en, edgedbg_iq_en, va_iq_en, va_indep_iq_en;
	uint8_t  edge_region_str_iq_en;

	//unsigned long loc_status;

	/** runtime change **/
	//copy to local flag kdrv_ipe_load_flg_tmp and clear global flag
	for (i = 0; i < KDRV_IPE_PARAM_MAX; i++) {
		kdrv_ipe_load_flg_tmp[i] = g_kdrv_ipe_load_flg[id][i];
	}

	//loc_status = ipe_platform_spin_lock();

	if (data != NULL) {
		KDRV_IPE_PARAM_ID item = *(KDRV_IPE_PARAM_ID *)data;

		g_kdrv_ipe_load_flg[id][item] = FALSE;
	} else {
		for (i = 0; i < KDRV_IPE_PARAM_MAX; i++) {
			g_kdrv_ipe_load_flg[id][i] = FALSE;
		}
	}


	//----  and IPL enable/disable setting -------//
	rgblpf_en     = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_RGBLPF) ? TRUE : FALSE);
	cc_en         = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CC) ? TRUE : FALSE);
	cst_en        = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CST) ? TRUE : FALSE);
	cctrl_en      = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CCTRL) ? TRUE : FALSE);
	cadj_ee_en    = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CADJ_EE) ? TRUE : FALSE);
	cadj_ycon_en  = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CADJ_YCCON) ? TRUE : FALSE);
	cadj_cofs_en  = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CADJ_COFS) ? TRUE : FALSE);
	cadj_rand_en  = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CADJ_RAND) ? TRUE : FALSE);
	cadj_hue_en   = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CADJ_HUE) ? TRUE : FALSE);
	cadj_fixth_en = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CADJ_FIXTH) ? TRUE : FALSE);
	cadj_mask_en  = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CADJ_MASK) ? TRUE : FALSE);
	cstp_en       = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CSTP) ? TRUE : FALSE);
	gamy_rand_en  = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_GAMMA_RAND) ? TRUE : FALSE);
	gamma_en      = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_GAMMA) ? TRUE : FALSE);
	ycurve_en     = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_YCURVE) ? TRUE : FALSE);
	//cc3d_en     = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_CC3D) ? TRUE : FALSE);
	pfr_en        = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_PFR) ? TRUE : FALSE);
	defog_en      = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_DEFOG) ? TRUE : FALSE);
	lce_en        = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_LCE) ? TRUE : FALSE);
	edgedbg_en    = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_EDGEDBG) ? TRUE : FALSE);
	va_en         = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_VA) ? TRUE : FALSE);
	va_indep_en   = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_VA_INDEP) ? TRUE : FALSE);
	edge_region_str_en = (uint8_t)((g_kdrv_ipe_ipl_func_en_sel[id] & KDRV_IPE_IQ_FUNC_EDGE_REGION_STR) ? TRUE : FALSE);


	rgblpf_iq_en     = g_kdrv_ipe_iq_en_info[id].rgb_lpf_param_enable;
	cc_iq_en         = g_kdrv_ipe_iq_en_info[id].cc_en_param_enable;
	cst_iq_en        = g_kdrv_ipe_iq_en_info[id].cst_param_enable;
	cctrl_iq_en      = g_kdrv_ipe_iq_en_info[id].cctrl_en_param_enable;
	cadj_ee_iq_en    = g_kdrv_ipe_iq_en_info[id].cadj_ee_param_enable;
	cadj_ycon_iq_en  = g_kdrv_ipe_iq_en_info[id].cadj_yccon_param_enable;
	cadj_cofs_iq_en  = g_kdrv_ipe_iq_en_info[id].cadj_cofs_param_enable;
	cadj_rand_iq_en  = g_kdrv_ipe_iq_en_info[id].cadj_rand_param_enable;
	cadj_hue_iq_en   = g_kdrv_ipe_iq_en_info[id].cadj_hue_param_enable;
	cadj_fixth_iq_en = g_kdrv_ipe_iq_en_info[id].cadj_fixth_param_enable;
	cadj_mask_iq_en  = g_kdrv_ipe_iq_en_info[id].cadj_mask_param_enable;
	cstp_iq_en       = g_kdrv_ipe_iq_en_info[id].cstp_param_enable;
	gamy_rand_iq_en  = g_kdrv_ipe_iq_en_info[id].gamy_rand_param_enable;
	gamma_iq_en      = g_kdrv_ipe_iq_en_info[id].gamma_enable;
	ycurve_iq_en     = g_kdrv_ipe_iq_en_info[id].y_curve_enable;
	//cc3d_iq_en     = (g_kdrv_ipe_ipl_func_iq_en_sel[id] & KDRV_IPE_IQ_FUNC_CC3D) ? TRUE : FALSE;
	pfr_iq_en        = g_kdrv_ipe_iq_en_info[id].pfr_param_enable;
	defog_iq_en      = g_kdrv_ipe_iq_en_info[id].dfg_param_enable;
	lce_iq_en        = g_kdrv_ipe_iq_en_info[id].lce_param_enable;
	edgedbg_iq_en    = g_kdrv_ipe_iq_en_info[id].edgdbg_param_enable;
	va_iq_en         = g_kdrv_ipe_iq_en_info[id].va_param_enable;
	va_indep_iq_en   = g_kdrv_ipe_iq_en_info[id].va_param_indep_va_enable;
	edge_region_str_iq_en = g_kdrv_ipe_iq_en_info[id].edge_region_str_enable;


	g_kdrv_ipe_info[id].rgb_lpf_param.enable     = (BOOL)(rgblpf_iq_en & rgblpf_en);
	g_kdrv_ipe_info[id].cc_en_param.enable       = (BOOL)(cc_iq_en & cc_en);
	g_kdrv_ipe_info[id].cst_param.enable         = (BOOL)(cst_iq_en & cst_en);
	g_kdrv_ipe_info[id].cctrl_en_param.enable    = (BOOL)(cctrl_iq_en & cctrl_en);
	g_kdrv_ipe_info[id].cadj_ee_param.enable     = (BOOL)(cadj_ee_iq_en & cadj_ee_en);
	g_kdrv_ipe_info[id].cadj_yccon_param.enable  = (BOOL)(cadj_ycon_iq_en & cadj_ycon_en);
	g_kdrv_ipe_info[id].cadj_cofs_param.enable   = (BOOL)(cadj_cofs_iq_en & cadj_cofs_en);
	g_kdrv_ipe_info[id].cadj_rand_param.enable   = (BOOL)(cadj_rand_iq_en & cadj_rand_en);
	g_kdrv_ipe_info[id].cadj_hue_param.enable    = (BOOL)(cadj_hue_iq_en & cadj_hue_en);
	g_kdrv_ipe_info[id].cadj_fixth_param.enable  = (BOOL)(cadj_fixth_iq_en & cadj_fixth_en);
	g_kdrv_ipe_info[id].cadj_mask_param.enable   = (BOOL)(cadj_mask_iq_en & cadj_mask_en);
	g_kdrv_ipe_info[id].cstp_param.enable        = (BOOL)(cstp_iq_en & cstp_en);
	g_kdrv_ipe_info[id].gamy_rand_param.enable   = (BOOL)(gamy_rand_iq_en & gamy_rand_en);
	g_kdrv_ipe_info[id].gamma.enable             = (BOOL)(gamma_iq_en & gamma_en);
	g_kdrv_ipe_info[id].y_curve.enable           = (BOOL)(ycurve_iq_en & ycurve_en);
	//g_kdrv_ipe_info[id]._3dcc_param.enable     = (BOOL)(cc3d_iq_en & cc3d_en);
	g_kdrv_ipe_info[id].pfr_param.enable         = (BOOL)(pfr_iq_en & pfr_en);
	g_kdrv_ipe_info[id].dfg_param.enable         = (BOOL)(defog_iq_en & defog_en);
	g_kdrv_ipe_info[id].lce_param.enable         = (BOOL)(lce_iq_en & lce_en);
	g_kdrv_ipe_info[id].edgdbg_param.enable      = (BOOL)(edgedbg_iq_en & edgedbg_en);
	g_kdrv_ipe_info[id].va_param.enable          = (BOOL)(va_iq_en & va_en);
	g_kdrv_ipe_info[id].va_param.indep_va_enable = (BOOL)(va_indep_iq_en & va_indep_en);
	g_kdrv_ipe_info[id].edge_region_param.enable = (BOOL)(edge_region_str_iq_en& edge_region_str_en);

	//ipe_platform_spin_unlock(loc_status);

#if 0
	g_kdrv_ipe_info[id].rgb_lpf_param.enable     &= rgblpf_en;
	g_kdrv_ipe_info[id].cc_en_param.enable       &= cc_en;
	g_kdrv_ipe_info[id].cst_param.enable         &= cst_en;
	g_kdrv_ipe_info[id].cctrl_en_param.enable    &= cctrl_en;
	g_kdrv_ipe_info[id].cadj_ee_param.enable     &= cadj_ee_en;
	g_kdrv_ipe_info[id].cadj_yccon_param.enable  &= cadj_ycon_en;
	g_kdrv_ipe_info[id].cadj_cofs_param.enable   &= cadj_cofs_en;
	g_kdrv_ipe_info[id].cadj_rand_param.enable   &= cadj_rand_en;
	g_kdrv_ipe_info[id].cadj_hue_param.enable    &= cadj_hue_en;
	g_kdrv_ipe_info[id].cadj_fixth_param.enable  &= cadj_fixth_en;
	g_kdrv_ipe_info[id].cadj_mask_param.enable   &= cadj_mask_en;
	g_kdrv_ipe_info[id].cstp_param.enable        &= cstp_en;
	g_kdrv_ipe_info[id].gamy_rand_param.enable   &= gamy_rand_en;
	g_kdrv_ipe_info[id].gamma.enable             &= gamma_en;
	g_kdrv_ipe_info[id].y_curve.enable           &= ycurve_en;
	//g_kdrv_ipe_info[id]._3dcc_param.enable       &= cc3d_en;
	g_kdrv_ipe_info[id].pfr_param.enable         &= pfr_en;
	g_kdrv_ipe_info[id].dfg_param.enable         &= defog_en;
	g_kdrv_ipe_info[id].lce_param.enable         &= lce_en;
	g_kdrv_ipe_info[id].edgdbg_param.enable      &= edgedbg_en;
	g_kdrv_ipe_info[id].va_param.enable          &= va_en;
	g_kdrv_ipe_info[id].va_param.indep_va_enable &= va_indep_en;
#endif

	rt |= kdrv_ipe_check_param(id);

	//DBG_USER("set load function : \r\n");
	//****** setting input/output parameter ******//
	if ((kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_IN_IMG])      ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_IN_ADDR])     ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_OUT_IMG])     ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_OUT_ADDR])    ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_SUBIN_ADDR])  ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_SUBOUT_ADDR]) ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_OUTPUT_MODE]) ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_SINGLE_OUT])  ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_EXTEND_INFO]) ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_ETH])         ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_DEFOG])        ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_LCE])          ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_SUBIMG])       ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_VA_ADDR])     ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_VA_PARAM])) {

		rt |= kdrv_ipe_set_input_param(id, p_in_info);
		rt |= kdrv_ipe_set_output_param(id, p_in_info, p_out_info);

		rt |= ipe_change_input(p_in_info);
		rt |= ipe_change_output(p_out_info);
		ipe_flush_cache(&g_ipe);
		//DBG_USER("runtime change in/out\r\n");
	}

	//************ setting interrupt  ************//

	if ((kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_INTER])) {

		g_ipe.intr_en = g_kdrv_ipe_info[id].inte_en;
		rt |= ipe_change_interrupt(g_ipe.intr_en);
	}

	//********** setting IQ parameter ***********//
	p_iq_info->param_update_sel = 0x0;

	//change eext
	if ((kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_EEXT]) ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_EEXT_TONEMAP])) {
		rt |= kdrv_ipe_set_load_fp[DRV_IPE_SETLAOD_IQ_EEXT](id, p_iq_info);
	}

	//change edge proc, rgblpf, pfr
	for (i = KDRV_IPE_PARAM_IQ_EPROC; i <= KDRV_IPE_PARAM_IQ_PFR; i++) {
		map_id = DRV_IPE_SETLAOD_IQ_EPROC + (i - KDRV_IPE_PARAM_IQ_EPROC);
		if (kdrv_ipe_load_flg_tmp[i]) {
			rt |= kdrv_ipe_set_load_fp[map_id](id, p_iq_info);
		}
	}

	//change cc
	if ((kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_CC_EN]) ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_CC]) ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_CCM])) {
		rt |= kdrv_ipe_set_load_fp[DRV_IPE_SETLAOD_IQ_CC](id, p_iq_info);
		//DBG_USER("runtime change cc\r\n");
	}

	//change cctrl
	if ((kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_CCTRL_EN]) ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_CCTRL]) ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_CCTRL_CT])) {
		rt |= kdrv_ipe_set_load_fp[DRV_IPE_SETLAOD_IQ_CCTRL](id, p_iq_info);
		//DBG_USER("runtime change cctrl\r\n");
	}

	//change CADJ related, cst, cstp
	for (i = KDRV_IPE_PARAM_IQ_CADJ_EE; i <= KDRV_IPE_PARAM_IQ_CSTP; i++) {
		if (i == KDRV_IPE_PARAM_IQ_CADJ_HUE) {
			continue;
		}
		map_id = DRV_IPE_SETLAOD_IQ_CADJ_EE + (i - KDRV_IPE_PARAM_IQ_CADJ_EE);
		if (kdrv_ipe_load_flg_tmp[i]) {
			rt |= kdrv_ipe_set_load_fp[map_id](id, p_iq_info);
			//DBG_USER("runtime change %d\r\n", (int)i);
		}
	}

	//change gamma, ycurve, 3dcc
	for (i = KDRV_IPE_PARAM_IQ_GAMMA; i <= KDRV_IPE_PARAM_IQ_YCURVE; i++) {
		map_id = DRV_IPE_SETLAOD_IQ_GAMMA + (i - KDRV_IPE_PARAM_IQ_GAMMA);
		if (kdrv_ipe_load_flg_tmp[i]) {
			rt |= kdrv_ipe_set_load_fp[map_id](id, p_iq_info);
			//DBG_USER("runtime change %d\r\n", (int)i);
		}
	}

	//change gamma ycurve rand
	if ((kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_GAMMA])  ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_YCURVE]) ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_GAMYRAND])) {
		rt |= kdrv_ipe_set_load_fp[DRV_IPE_SETLAOD_IQ_GAMYRAND](id, p_iq_info);
		//DBG_USER("runtime change gamma, ycurve\r\n");
	}

	//change subimg
	if ((kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_DEFOG])   ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_LCE])     ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_SUBIMG])  ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_IN_IMG]) ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_SUBOUT_ADDR])) {
		rt |= kdrv_ipe_set_load_fp[DRV_IPE_SETLAOD_IQ_DEFOG_SUBIMG](id, p_iq_info);
		//DBG_USER("runtime change subimg\r\n");
	}

	//change defog
	if ((kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_DEFOG]) ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_IN_IMG])) {
		rt |= kdrv_ipe_set_load_fp[DRV_IPE_SETLAOD_IQ_DEFOG](id, p_iq_info);
	}

	//change lce, edgedbg, eth
	for (i = KDRV_IPE_PARAM_IQ_LCE; i <= KDRV_IPE_PARAM_IPL_ETH; i++) {
		map_id = DRV_IPE_SETLAOD_IQ_LCE + (i - KDRV_IPE_PARAM_IQ_LCE);
		if (kdrv_ipe_load_flg_tmp[i]) {
			rt |= kdrv_ipe_set_load_fp[map_id](id, p_iq_info);
		}
	}

	//change va
	if ((kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_VA_PARAM]) ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_VA_WIN_INFO]) ||
		(kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IPL_IN_IMG])) {
		rt |= kdrv_ipe_set_load_fp[DRV_IPE_SETLAOD_IQ_VA](id, p_iq_info);
		//DBG_USER("runtime change va\r\n");
	}

	// change edge region strength
	if ((kdrv_ipe_load_flg_tmp[KDRV_IPE_PARAM_IQ_EDGE_REGION_STR])) {
        rt |= kdrv_ipe_set_load_fp[DRV_IPE_SETLAOD_IQ_EDGE_REGION_STR](id, p_iq_info);
	}

	// add yuv converter for yuv in
	if (g_kdrv_ipe_info[id].in_img_info.in_src == KDRV_IPE_IN_SRC_DRAM) {
		rt |= kdrv_ipe_set_iq_yuvcvt_for_yuvin(id, p_iq_info);
	}

	// disble functions for d2d yuv input
	//if (g_kdrv_ipe_info[id].in_img_info.in_src == KDRV_IPE_IN_SRC_DRAM) {
	//  kdrv_ipe_set_iq_funcsel_yuvin(id, p_iq_info);
	//}

	// function select
	rt |= kdrv_ipe_set_iq_funcsel(id, p_iq_info);
	p_iq_info->func_update_sel = IPE_FUNC_SET;
	//DBG_USER("driver func en 0x%08x\r\n", (int)p_iq_info->func_sel);
	//DBG_USER("change param\r\n");

	rt |= ipe_check_param(&g_ipe);
	if (rt != E_OK) {
		return rt;
	}
	rt |= ipe_change_param(p_iq_info);

	// load IQ parameters
	if (g_kdrv_ipe_info[id].in_img_info.in_src == KDRV_IPE_IN_SRC_ALL_DIRECT) {
		rt |= kdrv_ipe_load_iq_data(id, &g_ipe.iq_info, FALSE);
	} else {
		rt |= kdrv_ipe_load_iq_data(id, &g_ipe.iq_info, TRUE);
	}

	//DBG_USER("set load\r\n");

	//*****************************************//

	return rt;

}

static void kdrv_ipe_lock(KDRV_IPE_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		FLGPTN flag_ptn;

		wai_flg(&flag_ptn, p_handle->flag_id, p_handle->lock_bit, TWF_CLR);
	} else {
		set_flg(p_handle->flag_id, p_handle->lock_bit);
	}
}


static KDRV_IPE_HANDLE *kdrv_ipe_chk_handle(KDRV_IPE_HANDLE *p_handle)
{
	UINT32 i;

	for (i = 0; i < g_kdrv_ipe_channel_num; i++) {
		if (p_handle == &g_kdrv_ipe_handle_tab[i]) {
			return p_handle;
		}
	}

	return NULL;
}

static void kdrv_ipe_handle_lock(void)
{
	FLGPTN wait_flg;

	wai_flg(&wait_flg, kdrv_ipe_get_flag_id(FLG_ID_KDRV_IPE), KDRV_IPP_IPE_HDL_UNLOCK, (TWF_ORW | TWF_CLR));
}

static void kdrv_ipe_handle_unlock(void)
{
	set_flg(kdrv_ipe_get_flag_id(FLG_ID_KDRV_IPE), KDRV_IPP_IPE_HDL_UNLOCK);
}

static INT32 kdrv_ipe_handle_alloc(void)
{
	UINT32 i;
	ER rt = E_OK;

	kdrv_ipe_handle_lock();
	for (i = 0; i < g_kdrv_ipe_channel_num; i++) {
		if (!(g_kdrv_ipe_init_flg & (1 << i))) {

			g_kdrv_ipe_init_flg |= (1 << i);

			memset(&g_kdrv_ipe_handle_tab[i], 0, sizeof(KDRV_IPE_HANDLE));
			g_kdrv_ipe_handle_tab[i].flag_id = kdrv_ipe_get_flag_id(FLG_ID_KDRV_IPE);
			g_kdrv_ipe_handle_tab[i].lock_bit = (1 << i);
			g_kdrv_ipe_handle_tab[i].sts |= KDRV_IPE_HANDLE_LOCK;
			g_kdrv_ipe_handle_tab[i].sem_id = kdrv_ipe_get_sem_id(SEMID_KDRV_IPE);
		}
	}
	kdrv_ipe_handle_unlock();

	//if (i >= g_kdrv_ipe_channel_num) {
	//  DBG_ERR("get free handle fail(0x%.8x)\r\n", g_kdrv_ipe_init_flg);
	//  return E_SYS;
	//}
	return rt;
}

static UINT32 kdrv_ipe_handle_free(void)
{
	UINT32 rt = FALSE;
	UINT32 ch = 0;
	KDRV_IPE_HANDLE *p_handle;

	kdrv_ipe_handle_lock();
	for (ch = 0; ch < g_kdrv_ipe_channel_num; ch++) {
		p_handle = &(g_kdrv_ipe_handle_tab[ch]);
		p_handle->sts = 0;
		g_kdrv_ipe_init_flg &= ~(1 << ch);
		if (g_kdrv_ipe_init_flg == 0) {
			rt = TRUE;
		}
	}
	kdrv_ipe_handle_unlock();

	return rt;
}

static KDRV_IPE_HANDLE *kdrv_ipe_entry_id_conv2_handle(UINT32 entry_id)
{
	return  &g_kdrv_ipe_handle_tab[entry_id];
}


static void kdrv_ipe_sem(KDRV_IPE_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		SEM_WAIT(*p_handle->sem_id);    // wait semaphore
	} else {
		SEM_SIGNAL_ISR(*p_handle->sem_id);  // wait semaphore
	}
}

static void kdrv_ipe_frm_end(KDRV_IPE_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		iset_flg(p_handle->flag_id, KDRV_IPP_IPE_FMD);
	} else {
		clr_flg(p_handle->flag_id, KDRV_IPP_IPE_FMD);
	}
}

static void kdrv_ipe_isr_cb(UINT32 intstatus)
{
	KDRV_IPE_HANDLE *p_handle;

	p_handle = g_kdrv_ipe_trig_hdl;

	if (p_handle == NULL) {
		return;
	}

	if (intstatus & IPE_INTE_FRMEND) {
		//ipe_pause();
		kdrv_ipe_sem(p_handle, FALSE);
		kdrv_ipe_frm_end(p_handle, TRUE);
	}

	if (p_handle->isrcb_fp != NULL) {
		p_handle->isrcb_fp((UINT32)p_handle, intstatus, NULL, NULL);
	}
}

INT32 kdrv_ipe_open(UINT32 chip, UINT32 engine)
{
	IPE_OPENOBJ ipe_drv_obj;
	KDRV_IPE_OPENCFG kdrv_ipe_open_obj;
	UINT32 ch = 0;

	if (engine != KDRV_VIDEOPROCS_IPE_ENGINE0) {
		DBG_ERR("Invalid engine 0x%x\r\n", (int)engine);
		return E_ID;
	}

	if (g_kdrv_ipe_init_flg == 0) {
		if (kdrv_ipe_handle_alloc()) {
			DBG_WRN("KDRV IPE: no free handle, max handle num = %d\r\n", g_kdrv_ipe_channel_num);
			return E_SYS;
		}
	}

	if (g_kdrv_ipe_open_cnt == 0) {

		kdrv_ipe_open_obj = g_kdrv_ipe_open_cfg; //*(KDRV_IPE_OPENCFG *)open_cfg;
		ipe_drv_obj.ipe_clk_sel = kdrv_ipe_open_obj.ipe_clock_sel;
		ipe_drv_obj.FP_IPEISR_CB = kdrv_ipe_isr_cb;

		if (ipe_open(&ipe_drv_obj) != E_OK) {
			kdrv_ipe_handle_free();
			DBG_WRN("KDRV IPE: ipe_open failed\r\n");
			return E_SYS;
		}

		kdrv_ipe_memset_drv_param();

		for (ch = 0; ch < g_kdrv_ipe_channel_num; ch++) {
			kdrv_ipe_lock(&g_kdrv_ipe_handle_tab[ch], TRUE);

			// init internal parameter
			kdrv_ipe_init_param(ch);

			kdrv_ipe_lock(&g_kdrv_ipe_handle_tab[ch], FALSE);
		}

		if (kdrv_ipe_dma_buffer_allocate() != E_OK) {
			DBG_ERR("KDRV IPE: buffer allocate fail...\r\n");
			return E_SYS;
		}

	}
	g_kdrv_ipe_open_cnt++;

	return E_OK;
}

INT32 kdrv_ipe_close(UINT32 chip, UINT32 engine)
{
	ER rt = E_OK;
	KDRV_IPE_HANDLE *p_handle;
	UINT32 ch = 0;

	if (engine != KDRV_VIDEOPROCS_IPE_ENGINE0) {
		DBG_ERR("Invalid engine 0x%x\r\n", (unsigned int)engine);
		return E_ID;
	}

	for (ch = 0; ch < g_kdrv_ipe_channel_num; ch++) {
		p_handle = (KDRV_IPE_HANDLE *)&g_kdrv_ipe_handle_tab[ch];
		if (kdrv_ipe_chk_handle(p_handle) == NULL) {
			DBG_ERR("KDRV IPE: illegal handle[%d] 0x%.8x\r\n", (int)ch, (unsigned int)&g_kdrv_ipe_handle_tab[ch]);
			return E_SYS;
		}

		if ((p_handle->sts & KDRV_IPE_HANDLE_LOCK) != KDRV_IPE_HANDLE_LOCK) {
			DBG_ERR("KDRV IPE: illegal handle[%d] sts 0x%.8x\r\n", (int)ch, (unsigned int)p_handle->sts);;
			return E_SYS;
		}

	}

	g_kdrv_ipe_open_cnt--;
	if (g_kdrv_ipe_open_cnt == 0) {
		//DBG_IND("ipe count 0 close\r\n");

		for (ch = 0; ch < g_kdrv_ipe_channel_num; ch++) {
			p_handle = (KDRV_IPE_HANDLE *)&g_kdrv_ipe_handle_tab[ch];
			kdrv_ipe_lock(p_handle, TRUE);
		}

		if (kdrv_ipe_handle_free()) {
			rt = ipe_close();
		}

		for (ch = 0; ch < g_kdrv_ipe_channel_num; ch++) {
			p_handle = (KDRV_IPE_HANDLE *)&g_kdrv_ipe_handle_tab[ch];
			kdrv_ipe_lock(p_handle, FALSE);
		}

		if (kdrv_ipe_dma_buffer_free() != E_OK) {
			nvt_dbg(ERR, "ipe buffer free fail...\r\n");
			return E_SYS;
		}

		g_kdrv_ipe_trig_hdl = NULL;
	}

	return rt;
}

INT32 kdrv_ipe_pause(UINT32 chip, UINT32 engine)
{
	ER rt = E_OK;

	rt = ipe_pause();
	if (rt != E_OK) {
		return rt;
	}

	return rt;
}

INT32 kdrv_ipe_trigger(UINT32 id, KDRV_IPE_TRIGGER_PARAM *p_ipe_param,
					   KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	ER rt = E_OK;
	KDRV_IPE_HANDLE *p_handle;
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);

	//DBG_IND("id = 0x%x\r\n", id);
	//if (p_ipe_param == NULL) {
	//  DBG_ERR("p_ipe_param is NULL\r\n");
	//  return E_SYS;
	//}
	if (KDRV_DEV_ID_ENGINE(id) != KDRV_VIDEOPROCS_IPE_ENGINE0) {
		DBG_ERR("Invalid engine 0x%x\r\n", (unsigned int)KDRV_DEV_ID_ENGINE(id));
		return E_ID;
	}

	p_handle = kdrv_ipe_entry_id_conv2_handle(channel);
	kdrv_ipe_sem(p_handle, TRUE);
	//set ipe kdrv parameters to ipe driver
	rt = kdrv_ipe_setmode(channel);
	if (rt != E_OK) {
		kdrv_ipe_sem(p_handle, FALSE);
		return rt;
	}
	//update trig id and trig_cfg
	g_kdrv_ipe_trig_hdl = p_handle;
	//trigger ipe start
	kdrv_ipe_frm_end(p_handle, FALSE);
	ipe_clr_frame_end();

	//trigger ipe start
	rt = ipe_start();
	if (rt != E_OK) {
		kdrv_ipe_sem(p_handle, FALSE);
		return rt;
	}
	return rt;
}

#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)
static UINT32 *g_ipe_channel_buf = NULL;

#if defined __UITRON || defined __ECOS || defined __FREERTOS
#else
static void *handle_ipe_channel_buff = NULL;
#endif

#if defined __UITRON || defined __ECOS || defined __FREERTOS
static INT32 kdrv_ipe_alloc_channel_buff(UINT32 buf_size)
{
	g_ipe_channel_buf = (UINT32 *)malloc(buf_size * sizeof(UINT32));
	if (g_ipe_channel_buf == NULL) {
		DBG_ERR("ipe buffer allocation for ipe channel buffer fail!\r\n");
		return E_NOMEM;
	} else {
		return E_OK;
	}
}

static INT32 kdrv_ipe_free_channel_buff(void)
{
	if (g_ipe_channel_buf != NULL) {
		free(g_ipe_channel_buf);
		g_ipe_channel_buf = NULL;
	} else {
		DBG_WRN("ipe channel buffer is not allocated, nothing happend!\r\n");
	}
	return E_OK;
}
#else
static INT32 kdrv_ipe_alloc_channel_buff(UINT32 buf_size)
{

	int ret = 0;
	struct nvt_fmem_mem_info_t info = {0};
	ret = nvt_fmem_mem_info_init(&info, NVT_FMEM_ALLOC_CACHE, buf_size, NULL);
	if (ret >= 0) {
		handle_ipe_channel_buff = fmem_alloc_from_cma(&info, 0);
	}

	g_ipe_channel_buf = info.vaddr;
	if (g_ipe_channel_buf == NULL) {
		DBG_ERR("frm_get_buf_ddr for ipe channel buffer fail!\r\n");
		return E_NOMEM;
	} else {
		return E_OK;
	}
}

static INT32 kdrv_ipe_free_channel_buff(void)
{
	if (g_ipe_channel_buf != NULL) {

		int ret = 0;
		ret = fmem_release_from_cma(handle_ipe_channel_buff, 0);
		if (ret < 0) {
			pr_info("error release %d\n", __LINE__);
		}
		g_ipe_channel_buf = NULL;
	} else {
		DBG_WRN("ipe channel buffer is not allocated, nothing happend!\r\n");
	}

	return E_OK;
}
#endif
#endif //#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)

VOID kdrv_ipe_init(VOID)
{
#if !(defined __UITRON || defined __ECOS)
#if defined __FREERTOS
	ipe_platform_create_resource();
#endif
	ipe_platform_set_clk_rate(280);
#endif

	kdrv_ipe_install_id();

#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)
	{
		UINT32 buf_size, buf_size_used;
		buf_size = IPE_ALIGN_CEIL_64(kdrv_ipe_buf_query(KDRV_IPP_CBUF_MAX_NUM));
		kdrv_ipe_alloc_channel_buff(buf_size);
		buf_size_used = kdrv_ipe_buf_init((UINT32)g_ipe_channel_buf, KDRV_IPP_CBUF_MAX_NUM);
		if (buf_size_used != buf_size) {
			DBG_ERR("IPE channel buffer init size mismatch!\r\n");
			DBG_ERR("IPE buffer query size = %d...\r\n", buf_size);
			DBG_ERR("IPE buffer init size = %d...\r\n", buf_size_used);
		}
	}
#endif
}

VOID kdrv_ipe_uninit(VOID)
{
#if !(defined __UITRON || defined __ECOS)
	ipe_platform_release_resource();
#endif
	kdrv_ipe_uninstall_id();

#if (KDRV_IPP_NEW_INIT_FLOW == DISABLE)
	kdrv_ipe_buf_uninit();
	kdrv_ipe_free_channel_buff();
#endif

}

UINT32 kdrv_ipe_buf_query(UINT32 channel_num)
{
	UINT32 buffer_size = 0;

	if (channel_num == 0) {
		DBG_ERR("IPE min channel number = 1\r\n");
		g_kdrv_ipe_channel_num =  1;
	} else if (channel_num > KDRV_IPP_MAX_CHANNEL_NUM) {
		DBG_ERR("IPE max channel number = %d\r\n", KDRV_IPP_MAX_CHANNEL_NUM);
		g_kdrv_ipe_channel_num =  KDRV_IPP_MAX_CHANNEL_NUM;
	} else {
		g_kdrv_ipe_channel_num = channel_num;
	}

	kdrv_ipe_lock_chls = (1 << g_kdrv_ipe_channel_num) - 1;

	buffer_size  = IPE_ALIGN_CEIL_64(g_kdrv_ipe_channel_num * sizeof(KDRV_IPE_PRAM));
	buffer_size += IPE_ALIGN_CEIL_64(g_kdrv_ipe_channel_num * sizeof(KDRV_IPE_HANDLE));
	buffer_size += IPE_ALIGN_CEIL_64(g_kdrv_ipe_channel_num * sizeof(KDRV_IPE_IQ_EN_PARAM));
	buffer_size += IPE_ALIGN_CEIL_64(g_kdrv_ipe_channel_num * sizeof(UINT32));
	buffer_size += IPE_ALIGN_CEIL_64(IPE_DRAM_GAMMA_WORD * sizeof(UINT32));  // gamma buffer
	buffer_size += IPE_ALIGN_CEIL_64(IPE_DRAM_YCURVE_WORD * sizeof(UINT32));  // y-curve buffer

	buffer_size += g_kdrv_ipe_channel_num * IPE_ALIGN_CEIL_64(sizeof(UINT32 *));
	buffer_size += g_kdrv_ipe_channel_num * IPE_ALIGN_CEIL_64(KDRV_IPE_PARAM_MAX * sizeof(UINT32));

	return buffer_size;
}


UINT32 kdrv_ipe_buf_init(UINT32 input_addr, UINT32 channel_num)
{
	UINT32 i = 0;
	UINT32 buffer_size = 0;
	UINT32 channel_used = channel_num;

	kdrv_ipe_flow_init();

	if (channel_used == 0) {
		DBG_ERR("IPE min channel number = 1\r\n");
		channel_used = 1;
	} else if (channel_used > KDRV_IPP_MAX_CHANNEL_NUM) {
		DBG_ERR("IPE max channel number = %d\r\n", KDRV_IPP_MAX_CHANNEL_NUM);
		channel_used =  KDRV_IPP_MAX_CHANNEL_NUM;
	}

	g_kdrv_ipe_info = (KDRV_IPE_PRAM *)input_addr;
	buffer_size = IPE_ALIGN_CEIL_64(channel_used * sizeof(KDRV_IPE_PRAM));

	g_kdrv_ipe_handle_tab = (KDRV_IPE_HANDLE *)(input_addr + buffer_size);
	buffer_size += IPE_ALIGN_CEIL_64(channel_used * sizeof(KDRV_IPE_HANDLE));

	g_kdrv_ipe_iq_en_info = (KDRV_IPE_IQ_EN_PARAM *)(input_addr + buffer_size);
	buffer_size += IPE_ALIGN_CEIL_64(channel_used * sizeof(KDRV_IPE_IQ_EN_PARAM));

	g_kdrv_ipe_ipl_func_en_sel = (UINT32 *)(input_addr + buffer_size);
	buffer_size += IPE_ALIGN_CEIL_64(channel_used * sizeof(UINT32));

	p_gamma_buffer = (UINT32 *)(input_addr + buffer_size);
	buffer_size += IPE_ALIGN_CEIL_64(IPE_DRAM_GAMMA_WORD * sizeof(UINT32));

	p_ycurve_buffer = (UINT32 *)(input_addr + buffer_size);
	buffer_size += IPE_ALIGN_CEIL_64(IPE_DRAM_YCURVE_WORD * sizeof(UINT32));

	g_kdrv_ipe_load_flg = (UINT32 **)(input_addr + buffer_size);
	for (i = 0; i < channel_used; i++) {
		g_kdrv_ipe_load_flg[i] = (UINT32 *)(input_addr + buffer_size);
		buffer_size += IPE_ALIGN_CEIL_64(sizeof(UINT32 *));
	}
	for (i = 0; i < channel_used; i++) {
		g_kdrv_ipe_load_flg[i][0] = (UINT32)(input_addr + buffer_size);
		buffer_size += IPE_ALIGN_CEIL_64(KDRV_IPE_PARAM_MAX * sizeof(UINT32));
	}

	return buffer_size;
}

VOID kdrv_ipe_buf_uninit(VOID)
{
	UINT32 i = 0;

	g_kdrv_ipe_info        = NULL;
	g_kdrv_ipe_handle_tab  = NULL;
	g_kdrv_ipe_ipl_func_en_sel = NULL;
	g_kdrv_ipe_iq_en_info = NULL;
	p_gamma_buffer = NULL;
	p_ycurve_buffer = NULL;

	for (i = 0; i < g_kdrv_ipe_channel_num; i++) {
		g_kdrv_ipe_load_flg[i] = NULL;
	}
	g_kdrv_ipe_load_flg = NULL;

	g_kdrv_ipe_channel_num = 0;
	kdrv_ipe_lock_chls = 0;

	if (g_kdrv_ipe_open_cnt == 0) {
		g_kdrv_ipe_init_flg = 0;
	}
}

static ER kdrv_ipe_set_opencfg(UINT32 id, void *data)
{
	KDRV_IPE_OPENCFG *open_param;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}
	open_param = (KDRV_IPE_OPENCFG *)data;
	g_kdrv_ipe_open_cfg.ipe_clock_sel = open_param->ipe_clock_sel;

	return E_OK;
}

static ER kdrv_ipe_set_in_addr(UINT32 id, void *data)
{
	BOOL b_align = TRUE;
	BOOL b_noncache_addr = TRUE;
	KDRV_IPE_DMA_ADDR_INFO *in_pixel_addr;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	in_pixel_addr = (KDRV_IPE_DMA_ADDR_INFO *)data;

	b_align &= kdrv_ipe_check_align(in_pixel_addr->addr_y, 4);
	b_align &= kdrv_ipe_check_align(in_pixel_addr->addr_uv, 4);

	if (!b_align) {
		DBG_ERR("id %d input param error 0x%.8x 0x%.8x\r\n", (int)id, (unsigned int)in_pixel_addr->addr_y, (unsigned int)in_pixel_addr->addr_uv);
		return E_PAR;
	}

	b_noncache_addr &= kdrv_ipe_check_noncache_addr(in_pixel_addr->addr_y);
	b_noncache_addr &= kdrv_ipe_check_noncache_addr(in_pixel_addr->addr_uv);

	if (!b_noncache_addr) {
		DBG_ERR("id %d input param error 0x%.8x 0x%.8x\r\n", (int)id, (unsigned int)in_pixel_addr->addr_y, (unsigned int)in_pixel_addr->addr_uv);
		return E_PAR;
	}
	//DBG_USER("id %d input addr  0x%.8x 0x%.8x\r\n", (int)id, (unsigned int)in_pixel_addr->addr_y, (unsigned int)in_pixel_addr->addr_uv);
	g_kdrv_ipe_info[id].in_pixel_addr = *in_pixel_addr;

	//ipe_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ipe_set_in_img_info(UINT32 id, void *data)
{
	BOOL b_align = TRUE;
	KDRV_IPE_IN_IMG_INFO *in_img_info;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	in_img_info = (KDRV_IPE_IN_IMG_INFO *)data;

	b_align &= kdrv_ipe_check_align(in_img_info->ch.y_line_ofs, 4);
	b_align &= kdrv_ipe_check_align(in_img_info->ch.uv_line_ofs, 4);

	if (!b_align) {
		DBG_ERR("id %d input param error 0x%.8x 0x%.8x\r\n", (int)id, (int)in_img_info->ch.y_line_ofs, (int)in_img_info->ch.uv_line_ofs);
		return E_PAR;
	}

	g_kdrv_ipe_info[id].in_img_info = *in_img_info;

	//ipe_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ipe_set_out_addr(UINT32 id, void *data)
{
	BOOL b_align = TRUE;
	BOOL b_noncache_addr = TRUE;
	KDRV_IPE_DMA_ADDR_INFO *out_pixel_addr;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	out_pixel_addr = (KDRV_IPE_DMA_ADDR_INFO *)data;

	b_align &= kdrv_ipe_check_align(out_pixel_addr->addr_y, 4);
	b_align &= kdrv_ipe_check_align(out_pixel_addr->addr_uv, 4);

	if (!b_align) {
		DBG_ERR("id %d ouput param error 0x%.8x 0x%.8x\r\n", (int)id, (unsigned int)out_pixel_addr->addr_y, (unsigned int)out_pixel_addr->addr_uv);
		return E_PAR;
	}

	b_noncache_addr &= kdrv_ipe_check_noncache_addr(out_pixel_addr->addr_y);
	b_noncache_addr &= kdrv_ipe_check_noncache_addr(out_pixel_addr->addr_uv);

	if (!b_noncache_addr) {
		DBG_ERR("id %d ouput param error 0x%.8x 0x%.8x\r\n", (int)id, (unsigned int)out_pixel_addr->addr_y, (unsigned int)out_pixel_addr->addr_uv);
		return E_PAR;
	}

	g_kdrv_ipe_info[id].out_pixel_addr = *out_pixel_addr;

	//ipe_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ipe_set_out_img_info(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	g_kdrv_ipe_info[id].out_img_info = *(KDRV_IPE_OUT_IMG_INFO *)data;

	//ipe_platform_spin_unlock(loc_status);

	return E_OK;
}


static ER kdrv_ipe_set_subin_dma_addr(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	g_kdrv_ipe_info[id].subin_dma_addr = *(KDRV_IPE_DMA_SUBIN_ADDR *)data;

	//ipe_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ipe_set_subout_dma_addr(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	g_kdrv_ipe_info[id].subout_dma_addr = *(KDRV_IPE_DMA_SUBOUT_ADDR *)data;

	//ipe_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ipe_set_dram_out_mode(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	g_kdrv_ipe_info[id].dram_out_mode = *(KDRV_IPE_DRAM_OUTPUT_MODE *)data;

	//ipe_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ipe_set_single_out_channel(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	g_kdrv_ipe_info[id].single_out_ch = *(KDRV_IPE_DRAM_SINGLE_OUT_EN *)data;

	//ipe_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ipe_set_isrcb(UINT32 id, void *data)
{
	KDRV_IPE_HANDLE *p_handle;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	p_handle = kdrv_ipe_entry_id_conv2_handle(id);
	p_handle->isrcb_fp = (KDRV_IPP_ISRCB)data;

	return E_OK;
}

static ER kdrv_ipe_set_interrupt_en(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	g_kdrv_ipe_info[id].inte_en = *(KDRV_IPE_INTE *)data;

	//ipe_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ipe_set_param_eext(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_EEXT_PARAM *eext_param;
	UINT32 i;

	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	eext_param = (KDRV_IPE_EEXT_PARAM *)data;

	for (i = 0; i < KDRV_IPE_EDGE_KER_DIV_LEN; i++) {
		g_kdrv_ipe_info[id].eext_param.edge_ker[i] = ((INT16 *)eext_param->edge_ker)[i];
	}

	g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq0.eext_enh = eext_param->eext_kerstrength.ker_freq0.eext_enh;
	g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq0.eext_div = eext_param->eext_kerstrength.ker_freq0.eext_div;
	g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq1.eext_enh = eext_param->eext_kerstrength.ker_freq1.eext_enh;
	g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq1.eext_div = eext_param->eext_kerstrength.ker_freq1.eext_div;
	g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq2.eext_enh = eext_param->eext_kerstrength.ker_freq2.eext_enh;
	g_kdrv_ipe_info[id].eext_param.eext_kerstrength.ker_freq2.eext_div = eext_param->eext_kerstrength.ker_freq2.eext_div;

	g_kdrv_ipe_info[id].eext_param.eext_engcon.eext_div_con = eext_param->eext_engcon.eext_div_con;
	g_kdrv_ipe_info[id].eext_param.eext_engcon.eext_div_eng = eext_param->eext_engcon.eext_div_eng;
	g_kdrv_ipe_info[id].eext_param.eext_engcon.wt_con_eng =  eext_param->eext_engcon.wt_con_eng;

	g_kdrv_ipe_info[id].eext_param.ker_thickness.wt_ker_robust  = eext_param->ker_thickness.wt_ker_robust;
	g_kdrv_ipe_info[id].eext_param.ker_thickness.wt_ker_thin    = eext_param->ker_thickness.wt_ker_thin;
	g_kdrv_ipe_info[id].eext_param.ker_thickness.iso_ker_thin   = eext_param->ker_thickness.iso_ker_thin ;
	g_kdrv_ipe_info[id].eext_param.ker_thickness.iso_ker_robust = eext_param->ker_thickness.iso_ker_robust;

	g_kdrv_ipe_info[id].eext_param.ker_thickness_hld.wt_ker_robust  = eext_param->ker_thickness_hld.wt_ker_robust;
	g_kdrv_ipe_info[id].eext_param.ker_thickness_hld.wt_ker_thin    = eext_param->ker_thickness_hld.wt_ker_thin;
	g_kdrv_ipe_info[id].eext_param.ker_thickness_hld.iso_ker_thin   = eext_param->ker_thickness_hld.iso_ker_thin ;
	g_kdrv_ipe_info[id].eext_param.ker_thickness_hld.iso_ker_robust = eext_param->ker_thickness_hld.iso_ker_robust;

	g_kdrv_ipe_info[id].eext_param.eext_region.reg_wt.wt_low  = eext_param->eext_region.reg_wt.wt_low;
	g_kdrv_ipe_info[id].eext_param.eext_region.reg_wt.wt_high = eext_param->eext_region.reg_wt.wt_high;
	g_kdrv_ipe_info[id].eext_param.eext_region.reg_wt.wt_low_hld  = eext_param->eext_region.reg_wt.wt_low_hld;
	g_kdrv_ipe_info[id].eext_param.eext_region.reg_wt.wt_high_hld = eext_param->eext_region.reg_wt.wt_high_hld;
	g_kdrv_ipe_info[id].eext_param.eext_region.reg_th.th_flat = eext_param->eext_region.reg_th.th_flat;
	g_kdrv_ipe_info[id].eext_param.eext_region.reg_th.th_edge = eext_param->eext_region.reg_th.th_edge;
	g_kdrv_ipe_info[id].eext_param.eext_region.reg_th.th_flat_hld = eext_param->eext_region.reg_th.th_flat_hld;
	g_kdrv_ipe_info[id].eext_param.eext_region.reg_th.th_edge_hld = eext_param->eext_region.reg_th.th_edge_hld;
	g_kdrv_ipe_info[id].eext_param.eext_region.reg_th.th_lum_hld  = eext_param->eext_region.reg_th.th_lum_hld;

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_eext_tonemap(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_EEXT_TONEMAP_PARAM *eext_tonemap_param;
	UINT32 i;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	eext_tonemap_param = (KDRV_IPE_EEXT_TONEMAP_PARAM *)data;

	g_kdrv_ipe_info[id].eext_tonemap_param.gamma_sel = eext_tonemap_param->gamma_sel;
	for (i = 0; i < KDRV_IPE_TONE_MAP_LUT_LEN; i++) {
		g_kdrv_ipe_info[id].eext_tonemap_param.tone_map_lut[i] = ((UINT32 *)eext_tonemap_param->tone_map_lut)[i];
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_edge_overshoot(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_EDGE_OVERSHOOT_PARAM *param;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	param = (KDRV_IPE_EDGE_OVERSHOOT_PARAM *)data;

	g_kdrv_ipe_info[id].overshoot_param.overshoot_en = param->overshoot_en;
	g_kdrv_ipe_info[id].overshoot_param.wt_overshoot = param->wt_overshoot;
	g_kdrv_ipe_info[id].overshoot_param.th_overshoot = param->th_overshoot;
	g_kdrv_ipe_info[id].overshoot_param.slope_overshoot = param->slope_overshoot;

	g_kdrv_ipe_info[id].overshoot_param.wt_undershoot = param->wt_undershoot;
	g_kdrv_ipe_info[id].overshoot_param.th_undershoot = param->th_undershoot;
	g_kdrv_ipe_info[id].overshoot_param.slope_undershoot = param->slope_undershoot;

	g_kdrv_ipe_info[id].overshoot_param.th_undershoot_lum    = param->th_undershoot_lum;
	g_kdrv_ipe_info[id].overshoot_param.th_undershoot_eng    = param->th_undershoot_eng;
	g_kdrv_ipe_info[id].overshoot_param.slope_undershoot_lum = param->slope_undershoot_lum;
	g_kdrv_ipe_info[id].overshoot_param.slope_undershoot_eng = param->slope_undershoot_eng;
	g_kdrv_ipe_info[id].overshoot_param.clamp_wt_mod_lum     = param->clamp_wt_mod_lum;
	g_kdrv_ipe_info[id].overshoot_param.clamp_wt_mod_eng     = param->clamp_wt_mod_eng;
	g_kdrv_ipe_info[id].overshoot_param.strength_lum_eng     = param->strength_lum_eng;
	g_kdrv_ipe_info[id].overshoot_param.norm_lum_eng         = param->norm_lum_eng;

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_edge_proc(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_EPROC_PARAM *eproc_param;
	UINT32 i;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	eproc_param = (KDRV_IPE_EPROC_PARAM *)data;

	g_kdrv_ipe_info[id].eproc_param.edge_map_th.map_sel = eproc_param->edge_map_th.map_sel;
	g_kdrv_ipe_info[id].eproc_param.edge_map_th.ethr_low = eproc_param->edge_map_th.ethr_low;
	g_kdrv_ipe_info[id].eproc_param.edge_map_th.ethr_high = eproc_param->edge_map_th.ethr_high;
	g_kdrv_ipe_info[id].eproc_param.edge_map_th.etab_low = eproc_param->edge_map_th.etab_low;
	g_kdrv_ipe_info[id].eproc_param.edge_map_th.etab_high = eproc_param->edge_map_th.etab_high;

	g_kdrv_ipe_info[id].eproc_param.es_map_th.ethr_low = eproc_param->es_map_th.ethr_low;
	g_kdrv_ipe_info[id].eproc_param.es_map_th.ethr_high = eproc_param->es_map_th.ethr_high;
	g_kdrv_ipe_info[id].eproc_param.es_map_th.etab_low = eproc_param->es_map_th.etab_low;
	g_kdrv_ipe_info[id].eproc_param.es_map_th.etab_high = eproc_param->es_map_th.etab_high;

	for (i = 0; i < KDRV_IPE_EDGE_MAP_LUT_LEN; i++) {
		g_kdrv_ipe_info[id].eproc_param.edge_map_lut[i] = eproc_param->edge_map_lut[i];
	}

	for (i = 0; i < KDRV_IPE_EDGE_MAP_LUT_LEN; i++) {
		g_kdrv_ipe_info[id].eproc_param.es_map_lut[i] = eproc_param->es_map_lut[i];
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_rgblpf(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_RGBLPF_PARAM *rgb_lpf_param;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	rgb_lpf_param = (KDRV_IPE_RGBLPF_PARAM *)data;

	//g_kdrv_ipe_info[id].rgb_lpf_param.enable = rgb_lpf_param->enable;
	g_kdrv_ipe_iq_en_info[id].rgb_lpf_param_enable = rgb_lpf_param->enable;

	if (rgb_lpf_param->enable) {
		g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_r = rgb_lpf_param->lpf_param_r;
		g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_g = rgb_lpf_param->lpf_param_g;
		g_kdrv_ipe_info[id].rgb_lpf_param.lpf_param_b = rgb_lpf_param->lpf_param_b;
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_pfr(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_PFR_PARAM *pfr_param;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	pfr_param = (KDRV_IPE_PFR_PARAM *)data;

	//g_kdrv_ipe_info[id].pfr_param.enable = pfr_param->enable;
	g_kdrv_ipe_iq_en_info[id].pfr_param_enable = pfr_param->enable;

	//if (g_kdrv_ipe_info[id].pfr_param.enable) {
	if (pfr_param->enable) {
		g_kdrv_ipe_info[id].pfr_param = *(KDRV_IPE_PFR_PARAM *)data;
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_cc_en(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_CC_EN_PARAM *cc_en_param;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	cc_en_param = (KDRV_IPE_CC_EN_PARAM *)data;

	//g_kdrv_ipe_info[id].cc_en_param.enable = cc_en_param->enable;

	g_kdrv_ipe_iq_en_info[id].cc_en_param_enable = cc_en_param->enable;

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_cc(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_CC_PARAM *cc_param;
	UINT32 i;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	cc_param = (KDRV_IPE_CC_PARAM *)data;

	//if (g_kdrv_ipe_info[id].cc_en_param.enable) {
	if (g_kdrv_ipe_iq_en_info[id].cc_en_param_enable) {
		g_kdrv_ipe_info[id].cc_param.cc2_sel = cc_param->cc2_sel;
		g_kdrv_ipe_info[id].cc_param.cc_stab_sel = cc_param->cc_stab_sel;
		//g_kdrv_ipe_info[id].cc_param.cc_ofs_sel = cc_param->cc_ofs_sel;

		for (i = 0; i < KDRV_IPE_FTAB_LEN; i++) {
			g_kdrv_ipe_info[id].cc_param.fstab[i] = ((UINT8 *)cc_param->fstab)[i];
		}
		for (i = 0; i < KDRV_IPE_FTAB_LEN; i++) {
			g_kdrv_ipe_info[id].cc_param.fdtab[i] = ((UINT8 *)cc_param->fdtab)[i];
		}
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_ccm(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_CCM_PARAM *ccm_param;
	UINT32 i;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	ccm_param = (KDRV_IPE_CCM_PARAM *)data;

	//if (g_kdrv_ipe_info[id].cc_en_param.enable) {
	if (g_kdrv_ipe_iq_en_info[id].cc_en_param_enable) {
		g_kdrv_ipe_info[id].ccm_param.cc_range = ccm_param->cc_range;
		g_kdrv_ipe_info[id].ccm_param.cc_gamma_sel = ccm_param->cc_gamma_sel;

		for (i = 0; i < KDRV_IPE_COEF_LEN; i++) {
			g_kdrv_ipe_info[id].ccm_param.coef[i] = ((INT16 *)ccm_param->coef)[i];
		}
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_cctrl_en(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_CCTRL_EN_PARAM *cctrl_en_param;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	cctrl_en_param = (KDRV_IPE_CCTRL_EN_PARAM *)data;

	//g_kdrv_ipe_info[id].cctrl_en_param.enable = cctrl_en_param->enable;

	g_kdrv_ipe_iq_en_info[id].cctrl_en_param_enable = cctrl_en_param->enable;

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_cctrl(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_CCTRL_PARAM *cctrl_param;
	UINT32 i;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	cctrl_param = (KDRV_IPE_CCTRL_PARAM *)data;

	//if (g_kdrv_ipe_info[id].cctrl_en_param.enable) {
	if (g_kdrv_ipe_iq_en_info[id].cctrl_en_param_enable) {
		g_kdrv_ipe_info[id].cctrl_param.int_ofs = cctrl_param->int_ofs;
		g_kdrv_ipe_info[id].cctrl_param.sat_ofs = cctrl_param->sat_ofs;
		g_kdrv_ipe_info[id].cctrl_param.hue_c2g = cctrl_param->hue_c2g;
		g_kdrv_ipe_info[id].cctrl_param.cctrl_sel = cctrl_param->cctrl_sel;
		g_kdrv_ipe_info[id].cctrl_param.vdet_div = cctrl_param->vdet_div;

		for (i = 0; i < KDRV_IPE_CCTRL_TAB_LEN; i++) {
			g_kdrv_ipe_info[id].cctrl_param.edge_tab[i] = ((UINT8 *)cctrl_param->edge_tab)[i];
		}
		for (i = 0; i < KDRV_IPE_DDS_TAB_LEN; i++) {
			g_kdrv_ipe_info[id].cctrl_param.dds_tab[i] = ((UINT8 *)cctrl_param->dds_tab)[i];
		}
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_cctrl_ct(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_CCTRL_CT_PARAM *cctrl_ct_param;
	UINT32 i;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	cctrl_ct_param = (KDRV_IPE_CCTRL_CT_PARAM *)data;

	//if (g_kdrv_ipe_info[id].cctrl_en_param.enable) {
	if (g_kdrv_ipe_iq_en_info[id].cctrl_en_param_enable) {
		g_kdrv_ipe_info[id].cctrl_ct_param.hue_rotate_en = cctrl_ct_param->hue_rotate_en;

		for (i = 0; i < KDRV_IPE_CCTRL_TAB_LEN; i++) {
			g_kdrv_ipe_info[id].cctrl_ct_param.hue_tab[i] = ((UINT8 *)cctrl_ct_param->hue_tab)[i];
		}

		for (i = 0; i < KDRV_IPE_CCTRL_TAB_LEN; i++) {
			g_kdrv_ipe_info[id].cctrl_ct_param.sat_tab[i] = ((INT32 *)cctrl_ct_param->sat_tab)[i];
		}
		for (i = 0; i < KDRV_IPE_CCTRL_TAB_LEN; i++) {
			g_kdrv_ipe_info[id].cctrl_ct_param.int_tab[i] = ((INT32 *)cctrl_ct_param->int_tab)[i];
		}
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_cadjee(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_CADJ_EE_PARAM *cadj_ee_param;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	cadj_ee_param = (KDRV_IPE_CADJ_EE_PARAM *)data;

	//g_kdrv_ipe_info[id].cadj_ee_param.enable = cadj_ee_param->enable;
	g_kdrv_ipe_iq_en_info[id].cadj_ee_param_enable = cadj_ee_param->enable;

	if (cadj_ee_param->enable) {
		g_kdrv_ipe_info[id].cadj_ee_param.edge_enh_p = cadj_ee_param->edge_enh_p;
		g_kdrv_ipe_info[id].cadj_ee_param.edge_enh_n = cadj_ee_param->edge_enh_n;
		g_kdrv_ipe_info[id].cadj_ee_param.edge_inv_p_en = cadj_ee_param->edge_inv_p_en;
		g_kdrv_ipe_info[id].cadj_ee_param.edge_inv_n_en = cadj_ee_param->edge_inv_n_en;
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_cadjyccon(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_CADJ_YCCON_PARAM *cadj_yccon_param;
	UINT32 i;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	cadj_yccon_param = (KDRV_IPE_CADJ_YCCON_PARAM *)data;

	//g_kdrv_ipe_info[id].cadj_yccon_param.enable = cadj_yccon_param->enable;
	g_kdrv_ipe_iq_en_info[id].cadj_yccon_param_enable = cadj_yccon_param->enable;

	//if (g_kdrv_ipe_info[id].cadj_yccon_param.enable) {
	if (cadj_yccon_param->enable) {
		g_kdrv_ipe_info[id].cadj_yccon_param.y_con = cadj_yccon_param->y_con;
		g_kdrv_ipe_info[id].cadj_yccon_param.c_con = cadj_yccon_param->c_con;
		for (i = 0; i < KDRV_IPE_CCONTAB_LEN; i++) {
			g_kdrv_ipe_info[id].cadj_yccon_param.cconlut[i] = cadj_yccon_param->cconlut[i];
		}
		g_kdrv_ipe_info[id].cadj_yccon_param.ccontab_sel = cadj_yccon_param->ccontab_sel;
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_cadjcofs(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_CADJ_COFS_PARAM *cadj_cofs_param;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	cadj_cofs_param = (KDRV_IPE_CADJ_COFS_PARAM *)data;

	//g_kdrv_ipe_info[id].cadj_cofs_param.enable = cadj_cofs_param->enable;
	g_kdrv_ipe_iq_en_info[id].cadj_cofs_param_enable = cadj_cofs_param->enable;

	//if (g_kdrv_ipe_info[id].cadj_cofs_param.enable) {
	if (cadj_cofs_param->enable) {
		g_kdrv_ipe_info[id].cadj_cofs_param.cb_ofs = cadj_cofs_param->cb_ofs;
		g_kdrv_ipe_info[id].cadj_cofs_param.cr_ofs = cadj_cofs_param->cr_ofs;
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_cadjrand(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_CADJ_RAND_PARAM *cadj_rand_param;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	cadj_rand_param = (KDRV_IPE_CADJ_RAND_PARAM *)data;

	//g_kdrv_ipe_info[id].cadj_rand_param.enable = cadj_rand_param->enable;
	g_kdrv_ipe_iq_en_info[id].cadj_rand_param_enable = cadj_rand_param->enable;

	//if (g_kdrv_ipe_info[id].cadj_rand_param.enable) {
	if (cadj_rand_param->enable) {
		g_kdrv_ipe_info[id].cadj_rand_param.rand_en_y = cadj_rand_param->rand_en_y;
		g_kdrv_ipe_info[id].cadj_rand_param.rand_en_c = cadj_rand_param->rand_en_c;
		g_kdrv_ipe_info[id].cadj_rand_param.rand_level_y = cadj_rand_param->rand_level_y;
		g_kdrv_ipe_info[id].cadj_rand_param.rand_level_c = cadj_rand_param->rand_level_c;
		g_kdrv_ipe_info[id].cadj_rand_param.rand_reset = cadj_rand_param->rand_reset;
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_cadjhue(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_CADJ_HUE_PARAM *cadj_hue_param;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	cadj_hue_param = (KDRV_IPE_CADJ_HUE_PARAM *)data;

	//g_kdrv_ipe_info[id].cadj_hue_param.enable = cadj_hue_param->enable;
	g_kdrv_ipe_iq_en_info[id].cadj_hue_param_enable = cadj_hue_param->enable;

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_cadjfixth(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_CADJ_FIXTH_PARAM *cadj_fixth_param;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	cadj_fixth_param = (KDRV_IPE_CADJ_FIXTH_PARAM *)data;

	//g_kdrv_ipe_info[id].cadj_fixth_param.enable = cadj_fixth_param->enable;
	g_kdrv_ipe_iq_en_info[id].cadj_fixth_param_enable = cadj_fixth_param->enable;

	//if (g_kdrv_ipe_info[id].cadj_fixth_param.enable) {
	if (cadj_fixth_param->enable) {
		g_kdrv_ipe_info[id].cadj_fixth_param.yth1.y_th = cadj_fixth_param->yth1.y_th;
		g_kdrv_ipe_info[id].cadj_fixth_param.yth1.edge_th = cadj_fixth_param->yth1.edge_th;
		g_kdrv_ipe_info[id].cadj_fixth_param.yth1.ycth_sel_hit = cadj_fixth_param->yth1.ycth_sel_hit;
		g_kdrv_ipe_info[id].cadj_fixth_param.yth1.ycth_sel_nonhit = cadj_fixth_param->yth1.ycth_sel_nonhit;
		g_kdrv_ipe_info[id].cadj_fixth_param.yth1.value_hit = cadj_fixth_param->yth1.value_hit;
		g_kdrv_ipe_info[id].cadj_fixth_param.yth1.nonvalue_hit = cadj_fixth_param->yth1.nonvalue_hit;

		g_kdrv_ipe_info[id].cadj_fixth_param.yth2.y_th = cadj_fixth_param->yth2.y_th;
		g_kdrv_ipe_info[id].cadj_fixth_param.yth2.ycth_sel_hit = cadj_fixth_param->yth2.ycth_sel_hit;
		g_kdrv_ipe_info[id].cadj_fixth_param.yth2.ycth_sel_nonhit = cadj_fixth_param->yth2.ycth_sel_nonhit;
		g_kdrv_ipe_info[id].cadj_fixth_param.yth2.value_hit = cadj_fixth_param->yth2.value_hit;
		g_kdrv_ipe_info[id].cadj_fixth_param.yth2.nonvalue_hit = cadj_fixth_param->yth2.nonvalue_hit;

		g_kdrv_ipe_info[id].cadj_fixth_param.cth.edge_th = cadj_fixth_param->cth.edge_th;
		g_kdrv_ipe_info[id].cadj_fixth_param.cth.y_th_low = cadj_fixth_param->cth.y_th_low;
		g_kdrv_ipe_info[id].cadj_fixth_param.cth.y_th_high = cadj_fixth_param->cth.y_th_high;
		g_kdrv_ipe_info[id].cadj_fixth_param.cth.cb_th_low = cadj_fixth_param->cth.cb_th_low;
		g_kdrv_ipe_info[id].cadj_fixth_param.cth.cb_th_high = cadj_fixth_param->cth.cb_th_high;
		g_kdrv_ipe_info[id].cadj_fixth_param.cth.cr_th_low = cadj_fixth_param->cth.cr_th_low;
		g_kdrv_ipe_info[id].cadj_fixth_param.cth.cr_th_high = cadj_fixth_param->cth.cr_th_high;
		g_kdrv_ipe_info[id].cadj_fixth_param.cth.ycth_sel_hit = cadj_fixth_param->cth.ycth_sel_hit;
		g_kdrv_ipe_info[id].cadj_fixth_param.cth.ycth_sel_nonhit = cadj_fixth_param->cth.ycth_sel_nonhit;
		g_kdrv_ipe_info[id].cadj_fixth_param.cth.cb_value_hit = cadj_fixth_param->cth.cb_value_hit;
		g_kdrv_ipe_info[id].cadj_fixth_param.cth.cb_value_nonhit = cadj_fixth_param->cth.cb_value_nonhit;
		g_kdrv_ipe_info[id].cadj_fixth_param.cth.cr_value_hit = cadj_fixth_param->cth.cr_value_hit;
		g_kdrv_ipe_info[id].cadj_fixth_param.cth.cr_value_nonhit = cadj_fixth_param->cth.cr_value_nonhit;

	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_cadjmask(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_CADJ_MASK_PARAM *cadj_mask_param;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	cadj_mask_param = (KDRV_IPE_CADJ_MASK_PARAM *)data;

	//g_kdrv_ipe_info[id].cadj_mask_param.enable = cadj_mask_param->enable;
	g_kdrv_ipe_iq_en_info[id].cadj_mask_param_enable = cadj_mask_param->enable;

	if (cadj_mask_param->enable) {
		g_kdrv_ipe_info[id].cadj_mask_param.y_mask = cadj_mask_param->y_mask;
		g_kdrv_ipe_info[id].cadj_mask_param.cb_mask = cadj_mask_param->cb_mask;
		g_kdrv_ipe_info[id].cadj_mask_param.cr_mask = cadj_mask_param->cr_mask;
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_cst(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_CST_PARAM *cst_param;
	UINT32 i;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	cst_param = (KDRV_IPE_CST_PARAM *)data;

	//g_kdrv_ipe_info[id].cst_param.enable = cst_param->enable;
	g_kdrv_ipe_iq_en_info[id].cst_param_enable = cst_param->enable;

	//if (g_kdrv_ipe_info[id].cst_param.enable) {
	if (cst_param->enable) {
		g_kdrv_ipe_info[id].cst_param.cst_off_sel = cst_param->cst_off_sel;

		for (i = 0; i < KDRV_IPE_COEF_LEN; i++) {
			g_kdrv_ipe_info[id].cst_param.cst_coef[i] = ((INT16 *)cst_param->cst_coef)[i];
		}
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_cstp(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_CSTP_PARAM *cstp_param;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	cstp_param = (KDRV_IPE_CSTP_PARAM *)data;

	//g_kdrv_ipe_info[id].cstp_param.enable = cstp_param->enable;
	g_kdrv_ipe_iq_en_info[id].cstp_param_enable = cstp_param->enable;

	//if (g_kdrv_ipe_info[id].cstp_param.enable) {
	if (cstp_param->enable) {
		g_kdrv_ipe_info[id].cstp_param.cstp_ratio = cstp_param->cstp_ratio;
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_gammarand(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_GAMYRAND_PARAM *gamma_ycurve_rand;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	gamma_ycurve_rand = (KDRV_IPE_GAMYRAND_PARAM *)data;

	//g_kdrv_ipe_info[id].gamy_rand_param.enable = gamma_ycurve_rand->enable;
	g_kdrv_ipe_iq_en_info[id].gamy_rand_param_enable = gamma_ycurve_rand->enable;

	//if (g_kdrv_ipe_info[id].gamy_rand_param.enable) {
	if (gamma_ycurve_rand->enable) {
		g_kdrv_ipe_info[id].gamy_rand_param.rand_en = gamma_ycurve_rand->rand_en;
		g_kdrv_ipe_info[id].gamy_rand_param.rst_en = gamma_ycurve_rand->rst_en;
		g_kdrv_ipe_info[id].gamy_rand_param.rand_shift = gamma_ycurve_rand->rand_shift;
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_gammacurve(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_GAMMA_PARAM *gamma;
	UINT32 i;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	gamma = (KDRV_IPE_GAMMA_PARAM *)data;

	//g_kdrv_ipe_info[id].gamma.enable = gamma->enable;
	g_kdrv_ipe_iq_en_info[id].gamma_enable = gamma->enable;

	if (gamma->enable) {
		g_kdrv_ipe_info[id].gamma.option = gamma->option;

		for (i = 0; i < KDRV_IPE_GAMMA_LEN; i++) {
			g_kdrv_ipe_info[id].gamma.gamma_lut[KDRV_IPP_RGB_R][i] = gamma->gamma_lut[KDRV_IPP_RGB_R][i];
			g_kdrv_ipe_info[id].gamma.gamma_lut[KDRV_IPP_RGB_G][i] = gamma->gamma_lut[KDRV_IPP_RGB_G][i];
			g_kdrv_ipe_info[id].gamma.gamma_lut[KDRV_IPP_RGB_B][i] = gamma->gamma_lut[KDRV_IPP_RGB_B][i];
		}
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_ycurve(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_YCURVE_PARAM *y_curve;
	UINT32 i;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	y_curve = (KDRV_IPE_YCURVE_PARAM *)data;

	//g_kdrv_ipe_info[id].y_curve.enable = y_curve->enable;
	g_kdrv_ipe_iq_en_info[id].y_curve_enable = y_curve->enable;

	//if (g_kdrv_ipe_info[id].y_curve.enable) {
	if (y_curve->enable) {
		g_kdrv_ipe_info[id].y_curve.ycurve_sel = y_curve->ycurve_sel;
		for (i = 0; i < KDRV_IPE_YCURVE_LEN; i++) {
			g_kdrv_ipe_info[id].y_curve.y_curve_lut[i] = y_curve->y_curve_lut[i];
		}
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_defog(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_DEFOG_PARAM *defog;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	defog = (KDRV_IPE_DEFOG_PARAM *)data;

	//g_kdrv_ipe_info[id].dfg_param.enable = defog->enable;
	g_kdrv_ipe_iq_en_info[id].dfg_param_enable = defog->enable;

	if (defog->enable) {
		g_kdrv_ipe_info[id].dfg_param = *(KDRV_IPE_DEFOG_PARAM *)data;
		g_kdrv_ipe_info[id].dfg_param.dfg_stcs.airlight_stcs_ratio =  min(g_kdrv_ipe_info[id].dfg_param.dfg_stcs.airlight_stcs_ratio, (unsigned int)4096);

	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}


static ER kdrv_ipe_set_param_lce(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_LCE_PARAM *lce;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	lce = (KDRV_IPE_LCE_PARAM *)data;

	//g_kdrv_ipe_info[id].lce_param.enable = lce->enable;
	g_kdrv_ipe_iq_en_info[id].lce_param_enable = lce->enable;

	//if (g_kdrv_ipe_info[id].lce_param.enable) {
	if (lce->enable) {
		g_kdrv_ipe_info[id].lce_param = *(KDRV_IPE_LCE_PARAM *)data;
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_subimg(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	g_kdrv_ipe_info[id].subimg_param = *(KDRV_IPE_SUBIMG_PARAM *)data;

	//ipe_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ipe_set_param_edge_debug_mode(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_EDGEDBG_PARAM *edgedbg;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	edgedbg = (KDRV_IPE_EDGEDBG_PARAM *)data;

	//g_kdrv_ipe_info[id].edgdbg_param.enable = edgedbg->enable;
	g_kdrv_ipe_iq_en_info[id].edgdbg_param_enable = edgedbg->enable;

	//if (g_kdrv_ipe_info[id].edgdbg_param.enable) {
	if (edgedbg->enable) {
		g_kdrv_ipe_info[id].edgdbg_param.mode_sel = edgedbg->mode_sel;
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_param_eth(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_ETH_INFO *eth;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	eth = (KDRV_IPE_ETH_INFO *)data;

	g_kdrv_ipe_info[id].eth.enable = eth->enable;

	if (g_kdrv_ipe_info[id].eth.enable) {
		g_kdrv_ipe_info[id].eth.eth_outsel = eth->eth_outsel;
		g_kdrv_ipe_info[id].eth.eth_outfmt = eth->eth_outfmt;
		g_kdrv_ipe_info[id].eth.th_low = eth->th_low;
		g_kdrv_ipe_info[id].eth.th_mid = eth->th_mid;
		g_kdrv_ipe_info[id].eth.th_high = eth->th_high;
		g_kdrv_ipe_info[id].eth.outsel_h = eth->outsel_h;
		g_kdrv_ipe_info[id].eth.outsel_v = eth->outsel_v;
	}

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}

static ER kdrv_ipe_set_va_addr(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	g_kdrv_ipe_info[id].va_addr = *(KDRV_IPE_DMA_VA_ADDR *)data;

	//ipe_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ipe_set_va_win_info(UINT32 id, void *data)
{
	UINT32 i = 0;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	g_kdrv_ipe_info[id].va_win_info = *(KDRV_IPE_VA_WIN_INFO *)data;

	g_kdrv_ipe_info[id].va_win_info.ratio_base = max(g_kdrv_ipe_info[id].va_win_info.ratio_base, (unsigned int)1);

	g_kdrv_ipe_info[id].va_win_info.winsz_ratio.w = min(g_kdrv_ipe_info[id].va_win_info.winsz_ratio.w, g_kdrv_ipe_info[id].va_win_info.ratio_base);
	g_kdrv_ipe_info[id].va_win_info.winsz_ratio.h = min(g_kdrv_ipe_info[id].va_win_info.winsz_ratio.h, g_kdrv_ipe_info[id].va_win_info.ratio_base);
	for (i = 0; i < KDRV_IPE_VA_INDEP_NUM; i++) {
		g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[i].x = min(g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[i].x, g_kdrv_ipe_info[id].va_win_info.ratio_base);
		g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[i].y = min(g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[i].y, g_kdrv_ipe_info[id].va_win_info.ratio_base);
		g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[i].w = min(g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[i].w, g_kdrv_ipe_info[id].va_win_info.ratio_base);
		g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[i].h = min(g_kdrv_ipe_info[id].va_win_info.indep_roi_ratio[i].h, g_kdrv_ipe_info[id].va_win_info.ratio_base);
	}

	//ipe_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ipe_set_param_va(UINT32 id, void *data)
{
	KDRV_IPE_VA_PARAM *set_va_parm;
	uint32_t i = 0;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	set_va_parm = (KDRV_IPE_VA_PARAM *)data;

	g_kdrv_ipe_iq_en_info[id].va_param_enable = set_va_parm->enable;
	g_kdrv_ipe_iq_en_info[id].va_param_indep_va_enable = set_va_parm->indep_va_enable;


	g_kdrv_ipe_info[id].va_param.group_1 = set_va_parm->group_1;
	g_kdrv_ipe_info[id].va_param.group_2 = set_va_parm->group_2;

	g_kdrv_ipe_info[id].va_param.va_out_grp1_2 = set_va_parm->va_out_grp1_2;
	g_kdrv_ipe_info[id].va_param.va_lofs = set_va_parm->va_lofs;
	g_kdrv_ipe_info[id].va_param.win_num = set_va_parm->win_num;

	for (i = 0; i < KDRV_IPE_VA_INDEP_NUM; i++) {
		g_kdrv_ipe_info[id].va_param.indep_win[i] = set_va_parm->indep_win[i];
	}

	if ((g_kdrv_ipe_info[id].va_param.win_num.w > KDRV_IPE_VA_MAX_WINNUM) || (g_kdrv_ipe_info[id].va_param.win_num.h > KDRV_IPE_VA_MAX_WINNUM)) {
		DBG_IND("va max window number is %d \r\n", KDRV_IPE_VA_MAX_WINNUM);
		g_kdrv_ipe_info[id].va_param.win_num.w = min(g_kdrv_ipe_info[id].va_param.win_num.w, (unsigned int)KDRV_IPE_VA_MAX_WINNUM);
		g_kdrv_ipe_info[id].va_param.win_num.h = min(g_kdrv_ipe_info[id].va_param.win_num.h, (unsigned int)KDRV_IPE_VA_MAX_WINNUM);
	}


	//g_kdrv_ipe_info[id].va_param = *(KDRV_IPE_VA_PARAM *)data;
	//if ((g_kdrv_ipe_info[id].va_param.win_num.w > KDRV_IPE_VA_MAX_WINNUM) || (g_kdrv_ipe_info[id].va_param.win_num.h > KDRV_IPE_VA_MAX_WINNUM)) {
	//	DBG_IND("va max window number is %d \r\n", KDRV_IPE_VA_MAX_WINNUM);
	//	g_kdrv_ipe_info[id].va_param.win_num.w = min(g_kdrv_ipe_info[id].va_param.win_num.w, (unsigned int)KDRV_IPE_VA_MAX_WINNUM);
	//	g_kdrv_ipe_info[id].va_param.win_num.h = min(g_kdrv_ipe_info[id].va_param.win_num.h, (unsigned int)KDRV_IPE_VA_MAX_WINNUM);
	//}

	//ipe_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ipe_set_param_edge_region_strength(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_IPE_EDGE_REGION_PARAM *p_edge_region_str;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	p_edge_region_str = (KDRV_IPE_EDGE_REGION_PARAM *)data;

    g_kdrv_ipe_iq_en_info[id].edge_region_str_enable = p_edge_region_str->enable;
	g_kdrv_ipe_info[id].edge_region_param = *(KDRV_IPE_EDGE_REGION_PARAM *)data;

	//ipe_platform_spin_unlock(loc_status);

	return rt;
}


static ER kdrv_ipe_set_ext_info(UINT32 id, void *data)
{
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	g_kdrv_ipe_info[id].ext_info = *(KDRV_IPE_EXTEND_INFO *)data;

	//ipe_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ipe_set_iq_all(UINT32 id, void *data)
{
	ER retval;

	KDRV_IPE_PARAM_IQ_ALL_PARAM *info = {0};

	if (data == NULL) {
		DBG_ERR("KDRV IPE: parameter NULL...\r\n");
		return E_PAR;
	}

	info = (KDRV_IPE_PARAM_IQ_ALL_PARAM *)data;

	if (info->p_eext != NULL) {
		retval = kdrv_ipe_set_param_eext(id, (void *)info->p_eext);
		if (retval != E_OK) {
			return retval;
		}
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_EEXT] = TRUE;
	}

	if (info->p_eext_tonemap != NULL) {
		retval = kdrv_ipe_set_param_eext_tonemap(id, (void *)info->p_eext_tonemap);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_EEXT_TONEMAP] = TRUE;
	}


	if (info->p_edge_overshoot != NULL) {
		retval = kdrv_ipe_set_param_edge_overshoot(id, (void *)info->p_edge_overshoot);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_EDGE_OVERSHOOT] = TRUE;
	}

	if (info->p_eproc != NULL) {
		retval = kdrv_ipe_set_param_edge_proc(id, (void *)info->p_eproc);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_EPROC] = TRUE;
	}

	if (info->p_rgb_lpf != NULL) {
		retval = kdrv_ipe_set_param_rgblpf(id, (void *)info->p_rgb_lpf);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_RGBLPF] = TRUE;
	}

	if (info->p_pfr != NULL) {
		retval = kdrv_ipe_set_param_pfr(id, (void *)info->p_pfr);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_PFR] = TRUE;
	}

	if (info->p_cc_en != NULL) {
		retval = kdrv_ipe_set_param_cc_en(id, (void *)info->p_cc_en);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CC_EN] = TRUE;
	}

	if (info->p_cc != NULL) {
		retval = kdrv_ipe_set_param_cc(id, (void *)info->p_cc);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CC] = TRUE;
	}

	if (info->p_ccm != NULL) {
		retval = kdrv_ipe_set_param_ccm(id, (void *)info->p_ccm);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CCM] = TRUE;
	}

	if (info->p_cctrl_en != NULL) {
		retval = kdrv_ipe_set_param_cctrl_en(id, (void *)info->p_cctrl_en);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CCTRL_EN] = TRUE;
	}

	if (info->p_cctrl != NULL) {
		retval = kdrv_ipe_set_param_cctrl(id, (void *)info->p_cctrl);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CCTRL] = TRUE;
	}

	if (info->p_cctrl_ct != NULL) {
		retval = kdrv_ipe_set_param_cctrl_ct(id, (void *)info->p_cctrl_ct);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CCTRL_CT] = TRUE;
	}

	if (info->p_cadj_ee != NULL) {
		retval = kdrv_ipe_set_param_cadjee(id, (void *)info->p_cadj_ee);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CADJ_EE] = TRUE;
	}

	if (info->p_cadj_yccon != NULL) {
		retval = kdrv_ipe_set_param_cadjyccon(id, (void *)info->p_cadj_yccon);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CADJ_YCCON] = TRUE;
	}

	if (info->p_cadj_cofs != NULL) {
		retval = kdrv_ipe_set_param_cadjcofs(id, (void *)info->p_cadj_cofs);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CADJ_COFS] = TRUE;

	}

	if (info->p_cadj_rand != NULL) {
		retval = kdrv_ipe_set_param_cadjrand(id, (void *)info->p_cadj_rand);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CADJ_RAND] = TRUE;
	}

	if (info->p_cadj_hue != NULL) {
		retval = kdrv_ipe_set_param_cadjhue(id, (void *)info->p_cadj_hue);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CADJ_HUE] = TRUE;
	}

	if (info->p_cadj_fixth != NULL) {
		retval = kdrv_ipe_set_param_cadjfixth(id, (void *)info->p_cadj_fixth);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CADJ_FIXTH] = TRUE;
	}

	if (info->p_cadj_mask != NULL) {
		retval = kdrv_ipe_set_param_cadjmask(id, (void *)info->p_cadj_mask);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CADJ_MASK] = TRUE;
	}

	if (info->p_cst != NULL) {
		retval = kdrv_ipe_set_param_cst(id, (void *)info->p_cst);
		if (retval != E_OK) {
			return retval;
		}
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CST] = TRUE;
	}

	if (info->p_cstp != NULL) {
		retval = kdrv_ipe_set_param_cstp(id, (void *)info->p_cstp);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CSTP] = TRUE;
	}

	if (info->p_gamy_rand != NULL) {
		retval = kdrv_ipe_set_param_gammarand(id, (void *)info->p_gamy_rand);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_GAMYRAND] = TRUE;
	}

	if (info->p_gamma != NULL) {
		retval = kdrv_ipe_set_param_gammacurve(id, (void *)info->p_gamma);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_GAMMA] = TRUE;
	}

	if (info->p_y_curve != NULL) {
		retval = kdrv_ipe_set_param_ycurve(id, (void *)info->p_y_curve);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_YCURVE] = TRUE;
	}

	if (info->p_defog != NULL) {
		retval = kdrv_ipe_set_param_defog(id, (void *)info->p_defog);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_DEFOG] = TRUE;
	}

	if (info->p_lce != NULL) {
		retval = kdrv_ipe_set_param_lce(id, (void *)info->p_lce);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_LCE] = TRUE;
	}

	if (info->p_subimg != NULL) {
		retval = kdrv_ipe_set_param_subimg(id, (void *)info->p_subimg);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_SUBIMG] = TRUE;
	}

	if (info->p_edgedbg != NULL) {
		retval = kdrv_ipe_set_param_edge_debug_mode(id, (void *)info->p_edgedbg);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_EDGEDBG] = TRUE;
	}

	if (info->p_va != NULL) {
		retval = kdrv_ipe_set_param_va(id, (void *)info->p_va);
		if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_VA_PARAM] = TRUE;
	}

	if (info->p_edge_region_str != NULL) {
        retval = kdrv_ipe_set_param_edge_region_strength(id, (void *)info->p_edge_region_str);
        if (retval != E_OK) {
			return retval;
		}

		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_EDGE_REGION_STR] = TRUE;
	}

	return E_OK;
}

static ER kdrv_ipe_set_ipl_func_enable(UINT32 id, void *data)
{

	KDRV_IPE_PARAM_IPL_FUNC_EN *info = {0};
	UINT32 ipe_ipl_func_en_sel_temp = 0, ipe_ipl_func_en_diff = 0;
	//unsigned long loc_status;

	if (data == NULL) {
		DBG_ERR("KDRV IPE: set all IQ function enable parameter NULL...\r\n");
		return E_PAR;
	}

	//loc_status = ipe_platform_spin_lock();

	info = (KDRV_IPE_PARAM_IPL_FUNC_EN *)data;

	ipe_ipl_func_en_sel_temp = g_kdrv_ipe_ipl_func_en_sel[id];

	if (info->enable) {
		g_kdrv_ipe_ipl_func_en_sel[id] |= info->ipl_ctrl_func;
	} else {
		g_kdrv_ipe_ipl_func_en_sel[id] &= (~info->ipl_ctrl_func);
	}

	ipe_ipl_func_en_diff = ipe_ipl_func_en_sel_temp ^ g_kdrv_ipe_ipl_func_en_sel[id];


	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_RGBLPF)  {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_RGBLPF] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_CC)  {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CC_EN] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_CST) {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CST] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_CCTRL)   {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CCTRL_EN] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_CADJ_EE) {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CADJ_EE] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_CADJ_YCCON)  {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CADJ_YCCON] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_CADJ_COFS)   {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CADJ_COFS] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_CADJ_RAND)   {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CADJ_RAND] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_CADJ_HUE)    {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CADJ_HUE] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_CADJ_FIXTH)  {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CADJ_FIXTH] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_CADJ_MASK)   {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CADJ_MASK] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_CSTP)    {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_CSTP] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_GAMMA_RAND)  {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_GAMYRAND] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_GAMMA)   {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_GAMMA] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_YCURVE)  {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_YCURVE] = TRUE;
	}

	//if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_CC3D)    {
	//  g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_3DCC] = TRUE;
	//}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_PFR) {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_PFR] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_DEFOG)   {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_DEFOG] = TRUE;
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_SUBIMG] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_LCE) {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_LCE] = TRUE;
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_SUBIMG] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_EDGEDBG) {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_EDGEDBG] = TRUE;
	}

	if (ipe_ipl_func_en_diff & KDRV_IPE_IQ_FUNC_EDGE_REGION_STR) {
		g_kdrv_ipe_load_flg[id][KDRV_IPE_PARAM_IQ_EDGE_REGION_STR] = TRUE;
	}

	//ipe_platform_spin_unlock(loc_status);

	return E_OK;
}

static ER kdrv_ipe_set_reset(UINT32 id, void *data)
{
	KDRV_IPE_HANDLE *p_handle;

	if (ipe_reset(TRUE) != E_OK) {
		DBG_ERR("KDRV IPE: engine reset error...\r\n");
		return E_PAR;
	}

	p_handle = kdrv_ipe_entry_id_conv2_handle(id);
	kdrv_ipe_sem(p_handle, FALSE);

	return E_OK;
}

static ER kdrv_ipe_set_builtin_flow_disable(UINT32 id, void *data)
{
    ipe_set_builtin_flow_disable();

    return E_OK;
}

KDRV_IPE_SET_FP kdrv_ipe_set_fp[KDRV_IPE_PARAM_MAX] = {
	kdrv_ipe_set_opencfg,
	kdrv_ipe_set_in_img_info,
	kdrv_ipe_set_in_addr,

	kdrv_ipe_set_out_img_info,
	kdrv_ipe_set_out_addr,

	kdrv_ipe_set_subin_dma_addr,
	kdrv_ipe_set_subout_dma_addr,

	kdrv_ipe_set_dram_out_mode,
	kdrv_ipe_set_single_out_channel,

	kdrv_ipe_set_isrcb,
	kdrv_ipe_set_interrupt_en,
	kdrv_ipe_set_load,

	kdrv_ipe_set_param_eext,
	kdrv_ipe_set_param_eext_tonemap,
	kdrv_ipe_set_param_edge_overshoot,
	kdrv_ipe_set_param_edge_proc,
	kdrv_ipe_set_param_rgblpf,
	kdrv_ipe_set_param_pfr,

	kdrv_ipe_set_param_cc_en,
	kdrv_ipe_set_param_cc,
	kdrv_ipe_set_param_ccm,
	kdrv_ipe_set_param_cctrl_en,
	kdrv_ipe_set_param_cctrl,
	kdrv_ipe_set_param_cctrl_ct,
	kdrv_ipe_set_param_cadjee,
	kdrv_ipe_set_param_cadjyccon,
	kdrv_ipe_set_param_cadjcofs,
	kdrv_ipe_set_param_cadjrand,
	kdrv_ipe_set_param_cadjhue,
	kdrv_ipe_set_param_cadjfixth,
	kdrv_ipe_set_param_cadjmask,
	kdrv_ipe_set_param_cst,
	kdrv_ipe_set_param_cstp,

	kdrv_ipe_set_param_gammarand,
	kdrv_ipe_set_param_gammacurve,
	kdrv_ipe_set_param_ycurve,

	kdrv_ipe_set_param_subimg,
	kdrv_ipe_set_param_defog,
	kdrv_ipe_set_param_lce,
	kdrv_ipe_set_param_edge_debug_mode,

	kdrv_ipe_set_param_eth,

	kdrv_ipe_set_va_addr,
	kdrv_ipe_set_va_win_info,
	kdrv_ipe_set_param_va,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	kdrv_ipe_set_ext_info,
	kdrv_ipe_set_iq_all,
	kdrv_ipe_set_ipl_func_enable,
	kdrv_ipe_set_reset,
	kdrv_ipe_set_builtin_flow_disable,
	kdrv_ipe_set_param_edge_region_strength,
};


INT32 kdrv_ipe_set(UINT32 id, KDRV_IPE_PARAM_ID param_id, void *p_param)
{
	ER rt = E_OK;
	KDRV_IPE_HANDLE *p_handle;
	UINT32 ign_chk;

	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);
	UINT32 engine = KDRV_DEV_ID_ENGINE(id);

	if (engine != KDRV_VIDEOPROCS_IPE_ENGINE0) {
		DBG_ERR("Invalid engine 0x%x\r\n", (int)engine);
		return E_ID;
	}

	ign_chk = (KDRV_IPE_IGN_CHK & param_id);
	param_id = param_id & (~KDRV_IPE_IGN_CHK);

    if (param_id == KDRV_IPE_PARAM_IPL_BUILTIN_DISABLE) {
        if (kdrv_ipe_set_fp[param_id] != NULL) {
    		rt = kdrv_ipe_set_fp[param_id](channel, p_param);
    	}
    } else {
    	if ((g_kdrv_ipe_open_cnt == 0) && (param_id != KDRV_IPE_PARAM_IPL_OPENCFG)) {
    		DBG_ERR("KDRV IPE: engine is not opened. Only OPENCFG can be set. ID = %d\r\n", (int)param_id);
    		return E_SYS;
    	} else if ((g_kdrv_ipe_open_cnt > 0) && (param_id == KDRV_IPE_PARAM_IPL_OPENCFG)) {
    		DBG_ERR("KDRV IPE: engine is opened. OPENCFG cannot be set.\r\n");
    		return E_SYS;
    	}
    	//todo: param_id cannot be set_load before trigger //?

    	if (g_kdrv_ipe_init_flg == 0) {
    		if (kdrv_ipe_handle_alloc()) {
    			DBG_WRN("KDRV IPE: no free handle, max handle num = %d\r\n", g_kdrv_ipe_channel_num);
    			return E_SYS;
    		}
    	}

    	p_handle = &g_kdrv_ipe_handle_tab[channel];
    	if (kdrv_ipe_chk_handle(p_handle) == NULL) {
    		DBG_ERR("KDRV IPE: illegal handle 0x%.8x\r\n", (unsigned int)&g_kdrv_ipe_handle_tab[channel]);
    		return E_SYS;
    	}

    	if ((p_handle->sts & KDRV_IPE_HANDLE_LOCK) != KDRV_IPE_HANDLE_LOCK) {
    		DBG_ERR("KDRV IPE: illegal handle sts 0x%.8x\r\n", (unsigned int)p_handle->sts);
    		return E_SYS;
    	}

    	if (!ign_chk) {
    		kdrv_ipe_lock(p_handle, TRUE);
    	}

    	if (kdrv_ipe_set_fp[param_id] != NULL) {
    		rt = kdrv_ipe_set_fp[param_id](channel, p_param);
    		g_kdrv_ipe_load_flg[channel][param_id] = TRUE;
    	}

    	if (!ign_chk) {
    		kdrv_ipe_lock(p_handle, FALSE);
    	}
	}

	return rt;
}

static ER kdrv_ipe_get_opencfg(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_OPENCFG *)data = g_kdrv_ipe_open_cfg;

	return E_OK;
}

static ER kdrv_ipe_get_in_addr(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_DMA_ADDR_INFO *)data = g_kdrv_ipe_info[id].in_pixel_addr;

	return E_OK;
}

static ER kdrv_ipe_get_in_img_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_IN_IMG_INFO *)data = g_kdrv_ipe_info[id].in_img_info;

	return E_OK;
}

static ER kdrv_ipe_get_out_addr(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_DMA_ADDR_INFO *)data = g_kdrv_ipe_info[id].out_pixel_addr;

	return E_OK;
}

static ER kdrv_ipe_get_out_img_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_OUT_IMG_INFO *)data = g_kdrv_ipe_info[id].out_img_info;

	return E_OK;
}

static ER kdrv_ipe_get_subin_dma_addr(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_DMA_SUBIN_ADDR *)data = g_kdrv_ipe_info[id].subin_dma_addr;

	return E_OK;
}

static ER kdrv_ipe_get_subout_dma_addr(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_DMA_SUBOUT_ADDR *)data = g_kdrv_ipe_info[id].subout_dma_addr;

	return E_OK;
}

static ER kdrv_ipe_get_dram_out_mode(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_DRAM_OUTPUT_MODE *)data = g_kdrv_ipe_info[id].dram_out_mode;

	return E_OK;
}

static ER kdrv_ipe_get_single_out_channel(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_DRAM_SINGLE_OUT_EN *)data = g_kdrv_ipe_info[id].single_out_ch;

	return E_OK;
}


static ER kdrv_ipe_get_isrcb(UINT32 id, void *data)
{
	KDRV_IPE_HANDLE *p_handle;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	p_handle = kdrv_ipe_entry_id_conv2_handle(id);
	*(KDRV_IPP_ISRCB *)data = p_handle->isrcb_fp;

	return E_OK;
}

static ER kdrv_ipe_get_interrupt_en(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_INTE *)data = g_kdrv_ipe_info[id].inte_en;

	return E_OK;
}

static ER kdrv_ipe_get_param_eext(UINT32 id, void *data)
{
	KDRV_IPE_EEXT_PARAM *p_eext_param;
	UINT32 i;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_EEXT_PARAM *)data = g_kdrv_ipe_info[id].eext_param;
	p_eext_param = (KDRV_IPE_EEXT_PARAM *)data;
	//p_eext_param->edge_ch_sel = g_kdrv_ipe_info[id].eext_param.edge_ch_sel;
#if 0
	p_eext_param->dir_edge_ker = g_kdrv_ipe_info[id].eext_param.dir_edge_ker;
	p_eext_param->dir_edge_eext.yd_range = g_kdrv_ipe_info[id].eext_param.dir_edge_eext.yd_range;
	p_eext_param->dir_edge_eext.yd_max_th = g_kdrv_ipe_info[id].eext_param.dir_edge_eext.yd_max_th;
	p_eext_param->dir_edge_eext.pnd_adj_en = g_kdrv_ipe_info[id].eext_param.dir_edge_eext.pnd_adj_en;
	p_eext_param->dir_edge_eext.pnd_shift = g_kdrv_ipe_info[id].eext_param.dir_edge_eext.pnd_shift;
	p_eext_param->dir_edge_eext.edge_shift = g_kdrv_ipe_info[id].eext_param.dir_edge_eext.edge_shift;
	p_eext_param->dir_edge_eext.ker_sel = g_kdrv_ipe_info[id].eext_param.dir_edge_eext.ker_sel;

	p_eext_param->dir_edge_score_calc.a0 = g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.a0;
	p_eext_param->dir_edge_score_calc.b0 = g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.b0;
	p_eext_param->dir_edge_score_calc.c0 = g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.c0;
	p_eext_param->dir_edge_score_calc.d0 = g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.d0;
	p_eext_param->dir_edge_score_calc.a1 = g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.a1;
	p_eext_param->dir_edge_score_calc.b1 = g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.b1;
	p_eext_param->dir_edge_score_calc.c1 = g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.c1;
	p_eext_param->dir_edge_score_calc.d1 = g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.d1;
	p_eext_param->dir_edge_score_calc.a2 = g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.a2;
	p_eext_param->dir_edge_score_calc.b2 = g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.b2;
	p_eext_param->dir_edge_score_calc.c2 = g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.c2;
	p_eext_param->dir_edge_score_calc.d2 = g_kdrv_ipe_info[id].eext_param.dir_edge_score_calc.d2;

	p_eext_param->dir_edge_score_th.score_th0 = g_kdrv_ipe_info[id].eext_param.dir_edge_score_th.score_th0;
	p_eext_param->dir_edge_score_th.score_th1 = g_kdrv_ipe_info[id].eext_param.dir_edge_score_th.score_th1;
	p_eext_param->dir_edge_score_th.score_th2 = g_kdrv_ipe_info[id].eext_param.dir_edge_score_th.score_th2;
	p_eext_param->dir_edge_score_th.score_th3 = g_kdrv_ipe_info[id].eext_param.dir_edge_score_th.score_th3;

	p_eext_param->power_save_en = g_kdrv_ipe_info[id].eext_param.power_save_en;

	p_eext_param->dir_edge_con.score_center = g_kdrv_ipe_info[id].eext_param.dir_edge_con.score_center;
	p_eext_param->dir_edge_con.score_edge = g_kdrv_ipe_info[id].eext_param.dir_edge_con.score_edge;
	p_eext_param->dir_edge_con.score_detatil = g_kdrv_ipe_info[id].eext_param.dir_edge_con.score_detatil;
	p_eext_param->dir_edge_con.cnt_num_th = g_kdrv_ipe_info[id].eext_param.dir_edge_con.cnt_num_th;
	p_eext_param->dir_edge_con.max1_th = g_kdrv_ipe_info[id].eext_param.dir_edge_con.max1_th;
	p_eext_param->dir_edge_con.max_cnt_th = g_kdrv_ipe_info[id].eext_param.dir_edge_con.max_cnt_th;
	p_eext_param->dir_edge_con.max2_th = g_kdrv_ipe_info[id].eext_param.dir_edge_con.max2_th;
	p_eext_param->dir_edge_con.idx_sel = g_kdrv_ipe_info[id].eext_param.dir_edge_con.idx_sel;

	p_eext_param->dir_edge_w_a.ker_sel = g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.ker_sel;
	p_eext_param->dir_edge_w_a.adwt0 = g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.adwt0;
	p_eext_param->dir_edge_w_a.adwt1 = g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.adwt1;
	p_eext_param->dir_edge_w_a.adwt2 = g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.adwt2;
	p_eext_param->dir_edge_w_a.adwt3 = g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.adwt3;
	p_eext_param->dir_edge_w_a.adwt4 = g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.adwt4;
	p_eext_param->dir_edge_w_a.adwt5 = g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.adwt5;
	p_eext_param->dir_edge_w_a.ads0 = g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.ads0;
	p_eext_param->dir_edge_w_a.ads1 = g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.ads1;
	p_eext_param->dir_edge_w_a.ads2 = g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.ads2;
	p_eext_param->dir_edge_w_a.ads3 = g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.ads3;
	p_eext_param->dir_edge_w_a.ads4 = g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.ads4;
	p_eext_param->dir_edge_w_a.ads5 = g_kdrv_ipe_info[id].eext_param.dir_edge_w_a.ads5;

	p_eext_param->dir_edge_w_b.bdwt0 = g_kdrv_ipe_info[id].eext_param.dir_edge_w_b.bdwt0;
	p_eext_param->dir_edge_w_b.bdwt1 = g_kdrv_ipe_info[id].eext_param.dir_edge_w_b.bdwt1;
	p_eext_param->dir_edge_w_b.bds0 = g_kdrv_ipe_info[id].eext_param.dir_edge_w_b.bds0;
	p_eext_param->dir_edge_w_b.bds1 = g_kdrv_ipe_info[id].eext_param.dir_edge_w_b.bds1;
	p_eext_param->dir_edge_w_b.kel_sel = g_kdrv_ipe_info[id].eext_param.dir_edge_w_b.kel_sel;

	p_eext_param->dir_edge_w.ewd0 = g_kdrv_ipe_info[id].eext_param.dir_edge_w.ewd0;
	p_eext_param->dir_edge_w.ewd1 = g_kdrv_ipe_info[id].eext_param.dir_edge_w.ewd1;
	p_eext_param->dir_edge_w.ewd2 = g_kdrv_ipe_info[id].eext_param.dir_edge_w.ewd2;
	p_eext_param->dir_edge_w.dynamic_map_en = g_kdrv_ipe_info[id].eext_param.dir_edge_w.dynamic_map_en;
#endif
	for (i = 0; i < KDRV_IPE_EDGE_KER_DIV_LEN; i++) {
		p_eext_param->edge_ker[i] = g_kdrv_ipe_info[id].eext_param.edge_ker[i];
	}

	/*for (i = 0; i < KDRV_IPE_EDGE_DIR_TAB_LEN; i ++) {
	    p_eext_param->dir_edge_wtbl[i] = g_kdrv_ipe_info[id].eext_param.dir_edge_wtbl[i];
	}*/
	return E_OK;
}

static ER kdrv_ipe_get_param_eext_tonemap(UINT32 id, void *data)
{
	KDRV_IPE_EEXT_TONEMAP_PARAM *p_eext_tonemap_param;
	UINT32 i;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}
	p_eext_tonemap_param = (KDRV_IPE_EEXT_TONEMAP_PARAM *)data;
	p_eext_tonemap_param->gamma_sel = g_kdrv_ipe_info[id].eext_tonemap_param.gamma_sel;

	for (i = 0; i < KDRV_IPE_TONE_MAP_LUT_LEN; i++) {
		p_eext_tonemap_param->tone_map_lut[i] = g_kdrv_ipe_info[id].eext_tonemap_param.tone_map_lut[i];
	}
	return E_OK;
}

static ER kdrv_ipe_get_param_edge_proc(UINT32 id, void *data)
{
	KDRV_IPE_EPROC_PARAM *p_eproc_param;
	UINT32 i;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	p_eproc_param = (KDRV_IPE_EPROC_PARAM *)data;

	p_eproc_param->edge_map_th.map_sel = g_kdrv_ipe_info[id].eproc_param.edge_map_th.map_sel;
	p_eproc_param->edge_map_th.ethr_low = g_kdrv_ipe_info[id].eproc_param.edge_map_th.ethr_low;
	p_eproc_param->edge_map_th.ethr_high = g_kdrv_ipe_info[id].eproc_param.edge_map_th.ethr_high;
	p_eproc_param->edge_map_th.etab_low = g_kdrv_ipe_info[id].eproc_param.edge_map_th.etab_low;
	p_eproc_param->edge_map_th.etab_high = g_kdrv_ipe_info[id].eproc_param.edge_map_th.etab_high;

	p_eproc_param->es_map_th.ethr_low = g_kdrv_ipe_info[id].eproc_param.es_map_th.ethr_low;
	p_eproc_param->es_map_th.ethr_high = g_kdrv_ipe_info[id].eproc_param.es_map_th.ethr_high;
	p_eproc_param->es_map_th.etab_low = g_kdrv_ipe_info[id].eproc_param.es_map_th.etab_low;
	p_eproc_param->es_map_th.etab_high = g_kdrv_ipe_info[id].eproc_param.es_map_th.etab_high;

	for (i = 0; i < KDRV_IPE_EDGE_MAP_LUT_LEN; i++) {
		p_eproc_param->edge_map_lut[i] = g_kdrv_ipe_info[id].eproc_param.edge_map_lut[i];
	}

	for (i = 0; i < KDRV_IPE_EDGE_MAP_LUT_LEN; i++) {
		p_eproc_param->es_map_lut[i] = g_kdrv_ipe_info[id].eproc_param.es_map_lut[i];
	}

	return E_OK;
}

static ER kdrv_ipe_get_param_edge_overshoot(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_EDGE_OVERSHOOT_PARAM *)data = g_kdrv_ipe_info[id].overshoot_param;

	return E_OK;
}

static ER kdrv_ipe_get_param_rgblpf(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_RGBLPF_PARAM *)data = g_kdrv_ipe_info[id].rgb_lpf_param;

	return E_OK;
}

static ER kdrv_ipe_get_param_pfr(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_PFR_PARAM *)data = g_kdrv_ipe_info[id].pfr_param;

	return E_OK;
}

static ER kdrv_ipe_get_param_cc_en(UINT32 id, void *data)
{
	KDRV_IPE_CC_EN_PARAM *p_cc_en_param;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}
	p_cc_en_param = (KDRV_IPE_CC_EN_PARAM *)data;
	p_cc_en_param->enable = g_kdrv_ipe_info[id].cc_en_param.enable;

	return E_OK;
}

static ER kdrv_ipe_get_param_cc(UINT32 id, void *data)
{
	KDRV_IPE_CC_PARAM *p_cc_param;
	UINT32 i;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}
	p_cc_param = (KDRV_IPE_CC_PARAM *)data;
	p_cc_param->cc2_sel = g_kdrv_ipe_info[id].cc_param.cc2_sel;
	p_cc_param->cc_stab_sel = g_kdrv_ipe_info[id].cc_param.cc_stab_sel;

	for (i = 0; i < KDRV_IPE_FTAB_LEN; i++) {
		p_cc_param->fstab[i] = ((UINT8 *)g_kdrv_ipe_info[id].cc_param.fstab)[i];
		p_cc_param->fdtab[i] = ((UINT8 *)g_kdrv_ipe_info[id].cc_param.fdtab)[i];
	}

	return E_OK;
}

static ER kdrv_ipe_get_param_ccm(UINT32 id, void *data)
{
	KDRV_IPE_CCM_PARAM *p_ccm_param;
	UINT32 i;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}
	p_ccm_param = (KDRV_IPE_CCM_PARAM *)data;
	p_ccm_param->cc_range = g_kdrv_ipe_info[id].ccm_param.cc_range;
	p_ccm_param->cc_gamma_sel = g_kdrv_ipe_info[id].ccm_param.cc_gamma_sel;

	for (i = 0; i < KDRV_IPE_COEF_LEN; i++) {
		p_ccm_param->coef[i] = ((INT16 *)g_kdrv_ipe_info[id].ccm_param.coef)[i];
	}

	return E_OK;
}

static ER kdrv_ipe_get_param_cctrl_en(UINT32 id, void *data)
{
	KDRV_IPE_CCTRL_EN_PARAM *p_cctrl_en_param;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	p_cctrl_en_param = (KDRV_IPE_CCTRL_EN_PARAM *)data;
	p_cctrl_en_param->enable = g_kdrv_ipe_info[id].cctrl_en_param.enable;

	return E_OK;
}


static ER kdrv_ipe_get_param_cctrl(UINT32 id, void *data)
{
	KDRV_IPE_CCTRL_PARAM *p_cctrl_param;
	UINT32 i;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	p_cctrl_param = (KDRV_IPE_CCTRL_PARAM *)data;
	p_cctrl_param->int_ofs = g_kdrv_ipe_info[id].cctrl_param.int_ofs;
	p_cctrl_param->sat_ofs = g_kdrv_ipe_info[id].cctrl_param.sat_ofs;
	p_cctrl_param->hue_c2g = g_kdrv_ipe_info[id].cctrl_param.hue_c2g;
	p_cctrl_param->cctrl_sel = g_kdrv_ipe_info[id].cctrl_param.cctrl_sel;
	p_cctrl_param->vdet_div = g_kdrv_ipe_info[id].cctrl_param.vdet_div;

	for (i = 0; i < KDRV_IPE_CCTRL_TAB_LEN; i++) {
		p_cctrl_param->edge_tab[i] = ((UINT8 *)g_kdrv_ipe_info[id].cctrl_param.edge_tab)[i];
	}

	for (i = 0; i < KDRV_IPE_DDS_TAB_LEN; i++) {
		p_cctrl_param->dds_tab[i] = ((UINT8 *)g_kdrv_ipe_info[id].cctrl_param.dds_tab)[i];
	}

	return E_OK;
}
static ER kdrv_ipe_get_param_cctrl_ct(UINT32 id, void *data)
{
	KDRV_IPE_CCTRL_CT_PARAM *p_cctrl_ct_param;
	UINT32 i;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	p_cctrl_ct_param = (KDRV_IPE_CCTRL_CT_PARAM *)data;
	p_cctrl_ct_param->hue_rotate_en = g_kdrv_ipe_info[id].cctrl_ct_param.hue_rotate_en;

	for (i = 0; i < KDRV_IPE_CCTRL_TAB_LEN; i++) {
		p_cctrl_ct_param->hue_tab[i] = ((UINT8 *)g_kdrv_ipe_info[id].cctrl_ct_param.hue_tab)[i];
	}

	for (i = 0; i < KDRV_IPE_CCTRL_TAB_LEN; i++) {
		p_cctrl_ct_param->sat_tab[i] = ((INT32 *)g_kdrv_ipe_info[id].cctrl_ct_param.sat_tab)[i];
	}

	for (i = 0; i < KDRV_IPE_CCTRL_TAB_LEN; i++) {
		p_cctrl_ct_param->int_tab[i] = ((INT32 *)g_kdrv_ipe_info[id].cctrl_ct_param.int_tab)[i];
	}

	return E_OK;
}

static ER kdrv_ipe_get_param_cadjee(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_CADJ_EE_PARAM *)data = g_kdrv_ipe_info[id].cadj_ee_param;

	return E_OK;
}

static ER kdrv_ipe_get_param_cadjyccon(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_CADJ_YCCON_PARAM *)data = g_kdrv_ipe_info[id].cadj_yccon_param;

	return E_OK;
}

static ER kdrv_ipe_get_param_cadjcofs(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_CADJ_COFS_PARAM *)data = g_kdrv_ipe_info[id].cadj_cofs_param;

	return E_OK;
}

static ER kdrv_ipe_get_param_cadjrand(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_CADJ_RAND_PARAM *)data = g_kdrv_ipe_info[id].cadj_rand_param;

	return E_OK;
}

static ER kdrv_ipe_get_param_cadjhue(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_CADJ_HUE_PARAM *)data = g_kdrv_ipe_info[id].cadj_hue_param;

	return E_OK;
}

static ER kdrv_ipe_get_param_cadjfixth(UINT32 id, void *data)
{
	KDRV_IPE_CADJ_FIXTH_PARAM *cadj_fixth_param;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	cadj_fixth_param = (KDRV_IPE_CADJ_FIXTH_PARAM *)data;

	cadj_fixth_param->enable = g_kdrv_ipe_info[id].cadj_fixth_param.enable;

	cadj_fixth_param->yth1.y_th = g_kdrv_ipe_info[id].cadj_fixth_param.yth1.y_th;
	cadj_fixth_param->yth1.edge_th = g_kdrv_ipe_info[id].cadj_fixth_param.yth1.edge_th;
	cadj_fixth_param->yth1.ycth_sel_hit = g_kdrv_ipe_info[id].cadj_fixth_param.yth1.ycth_sel_hit;
	cadj_fixth_param->yth1.ycth_sel_nonhit = g_kdrv_ipe_info[id].cadj_fixth_param.yth1.ycth_sel_nonhit;
	cadj_fixth_param->yth1.value_hit = g_kdrv_ipe_info[id].cadj_fixth_param.yth1.value_hit;
	cadj_fixth_param->yth1.nonvalue_hit = g_kdrv_ipe_info[id].cadj_fixth_param.yth1.nonvalue_hit;

	cadj_fixth_param->yth2.y_th = g_kdrv_ipe_info[id].cadj_fixth_param.yth2.y_th;
	cadj_fixth_param->yth2.ycth_sel_hit = g_kdrv_ipe_info[id].cadj_fixth_param.yth2.ycth_sel_hit;
	cadj_fixth_param->yth2.ycth_sel_nonhit = g_kdrv_ipe_info[id].cadj_fixth_param.yth2.ycth_sel_nonhit;
	cadj_fixth_param->yth2.value_hit = g_kdrv_ipe_info[id].cadj_fixth_param.yth2.value_hit;
	cadj_fixth_param->yth2.nonvalue_hit = g_kdrv_ipe_info[id].cadj_fixth_param.yth2.nonvalue_hit;

	cadj_fixth_param->cth.edge_th = g_kdrv_ipe_info[id].cadj_fixth_param.cth.edge_th;
	cadj_fixth_param->cth.y_th_low = g_kdrv_ipe_info[id].cadj_fixth_param.cth.y_th_low;
	cadj_fixth_param->cth.y_th_high = g_kdrv_ipe_info[id].cadj_fixth_param.cth.y_th_high;
	cadj_fixth_param->cth.cb_th_low = g_kdrv_ipe_info[id].cadj_fixth_param.cth.cb_th_low;
	cadj_fixth_param->cth.cb_th_high = g_kdrv_ipe_info[id].cadj_fixth_param.cth.cb_th_high;
	cadj_fixth_param->cth.cr_th_low = g_kdrv_ipe_info[id].cadj_fixth_param.cth.cr_th_low;
	cadj_fixth_param->cth.cr_th_high = g_kdrv_ipe_info[id].cadj_fixth_param.cth.cr_th_high;
	cadj_fixth_param->cth.ycth_sel_hit = g_kdrv_ipe_info[id].cadj_fixth_param.cth.ycth_sel_hit;
	cadj_fixth_param->cth.ycth_sel_nonhit = g_kdrv_ipe_info[id].cadj_fixth_param.cth.ycth_sel_nonhit;
	cadj_fixth_param->cth.cb_value_hit = g_kdrv_ipe_info[id].cadj_fixth_param.cth.cb_value_hit;
	cadj_fixth_param->cth.cb_value_nonhit = g_kdrv_ipe_info[id].cadj_fixth_param.cth.cb_value_nonhit;
	cadj_fixth_param->cth.cr_value_hit = g_kdrv_ipe_info[id].cadj_fixth_param.cth.cr_value_hit;
	cadj_fixth_param->cth.cr_value_nonhit = g_kdrv_ipe_info[id].cadj_fixth_param.cth.cr_value_nonhit;

	return E_OK;
}

static ER kdrv_ipe_get_param_cadjmask(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_CADJ_MASK_PARAM *)data = g_kdrv_ipe_info[id].cadj_mask_param;

	return E_OK;
}

static ER kdrv_ipe_get_param_cst(UINT32 id, void *data)
{
	KDRV_IPE_CST_PARAM *p_cst_param;
	UINT32 i;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	p_cst_param = (KDRV_IPE_CST_PARAM *)data;

	p_cst_param->enable = g_kdrv_ipe_info[id].cst_param.enable;
	p_cst_param->cst_off_sel = g_kdrv_ipe_info[id].cst_param.cst_off_sel;

	for (i = 0; i < KDRV_IPE_COEF_LEN; i++) {
		p_cst_param->cst_coef[i] = ((INT16 *)g_kdrv_ipe_info[id].cst_param.cst_coef)[i];
	}

	return E_OK;
}

static ER kdrv_ipe_get_param_cstp(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_CSTP_PARAM *)data = g_kdrv_ipe_info[id].cstp_param;

	return E_OK;
}

static ER kdrv_ipe_get_param_gammarand(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_GAMYRAND_PARAM *)data = g_kdrv_ipe_info[id].gamy_rand_param;

	return E_OK;
}

static ER kdrv_ipe_get_param_gammacurve(UINT32 id, void *data)
{
	UINT32 i;
	KDRV_IPE_GAMMA_PARAM *p_gamma_param;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	p_gamma_param = (KDRV_IPE_GAMMA_PARAM *)data;

	p_gamma_param->enable = g_kdrv_ipe_info[id].gamma.enable;
	p_gamma_param->option = g_kdrv_ipe_info[id].gamma.option;

	for (i = 0; i < KDRV_IPE_GAMMA_LEN; i++) {
		p_gamma_param->gamma_lut[KDRV_IPP_RGB_R][i] = g_kdrv_ipe_info[id].gamma.gamma_lut[KDRV_IPP_RGB_R][i];
		p_gamma_param->gamma_lut[KDRV_IPP_RGB_G][i] = g_kdrv_ipe_info[id].gamma.gamma_lut[KDRV_IPP_RGB_G][i];
		p_gamma_param->gamma_lut[KDRV_IPP_RGB_B][i] = g_kdrv_ipe_info[id].gamma.gamma_lut[KDRV_IPP_RGB_B][i];
	}

	return E_OK;
}

static ER kdrv_ipe_get_param_ycurve(UINT32 id, void *data)
{
	UINT32 i;
	KDRV_IPE_YCURVE_PARAM *p_ycurve_param;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	p_ycurve_param = (KDRV_IPE_YCURVE_PARAM *)data;

	p_ycurve_param->enable = g_kdrv_ipe_info[id].y_curve.enable;

	for (i = 0; i < KDRV_IPE_YCURVE_LEN; i++) {
		p_ycurve_param->y_curve_lut[i] = g_kdrv_ipe_info[id].y_curve.y_curve_lut[i];
	}

	return E_OK;
}

#if 0
static ER kdrv_ipe_get_param_3dcc(UINT32 id, void *data)
{
	//UINT32 i;

	DBG_ERR("3DCC is not support.");
#if 0
	KDRV_IPE_3DCC_PARAM *p_3dcc_param;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}


	p_3dcc_param = (KDRV_IPE_3DCC_PARAM *)data;

	p_3dcc_param->enable = g_kdrv_ipe_info[id]._3dcc_param.enable;

	//for (i = 0; i < KDRV_IPE_3DCC_LEN; i++) {
	//  p_3dcc_param->cc3d_lut[i] = g_kdrv_ipe_info[id]._3dcc_param.cc3d_lut[i];
	//}
	p_3dcc_param->cc3d_rnd.round_opt = g_kdrv_ipe_info[id]._3dcc_param.cc3d_rnd.round_opt;
	p_3dcc_param->cc3d_rnd.rst_en = g_kdrv_ipe_info[id]._3dcc_param.cc3d_rnd.rst_en;
#endif

	return E_OK;
}
#endif

static ER kdrv_ipe_get_param_defog(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_DEFOG_PARAM *)data = g_kdrv_ipe_info[id].dfg_param;

	return E_OK;
}


static ER kdrv_ipe_get_param_lce(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_LCE_PARAM *)data = g_kdrv_ipe_info[id].lce_param;

	return E_OK;
}

static ER kdrv_ipe_get_param_subimg(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_SUBIMG_PARAM *)data = g_kdrv_ipe_info[id].subimg_param;

	return E_OK;
}

static ER kdrv_ipe_get_param_edge_debug_mode(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_EDGEDBG_PARAM *)data = g_kdrv_ipe_info[id].edgdbg_param;

	return E_OK;
}

static ER kdrv_ipe_get_param_eth(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_ETH_INFO *)data = g_kdrv_ipe_info[id].eth;

	return E_OK;
}

static ER kdrv_ipe_get_va_addr(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_DMA_VA_ADDR *)data = g_kdrv_ipe_info[id].va_addr;

	return E_OK;
}

static ER kdrv_ipe_get_va_win_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_VA_WIN_INFO *)data = g_kdrv_ipe_info[id].va_win_info;

	return E_OK;
}

static ER kdrv_ipe_get_param_va(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_VA_PARAM *)data = g_kdrv_ipe_info[id].va_param;

	return E_OK;
}

static ER kdrv_ipe_get_va_rslt(UINT32 id, void *data)
{
	KDRV_IPE_VA_RST *p_va_rst = (KDRV_IPE_VA_RST *)data;
	IPE_VA_RSLT rslt = {0};
	IPE_VA_SETTING setting = {0};

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	rslt.p_g1_h = p_va_rst->buf_g1_h;
	rslt.p_g1_v = p_va_rst->buf_g1_v;
	rslt.p_g1_h_cnt = p_va_rst->buf_g1_h_cnt;
	rslt.p_g1_v_cnt = p_va_rst->buf_g1_v_cnt;
	rslt.p_g2_h = p_va_rst->buf_g2_h;
	rslt.p_g2_v = p_va_rst->buf_g2_v;
	rslt.p_g2_h_cnt = p_va_rst->buf_g2_h_cnt;
	rslt.p_g2_v_cnt = p_va_rst->buf_g2_v_cnt;

	ipe_get_va_result(&setting, &rslt);

	return E_OK;
}

static ER kdrv_ipe_get_va_indep_rslt(UINT32 id, void *data)
{
	UINT32 i;
	KDRV_IPE_VA_INDEP_RSLT *p_va_rst = (KDRV_IPE_VA_INDEP_RSLT *)data;
	IPE_INDEP_VA_WIN_RSLT rst = {0};

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	for (i = 0; i < KDRV_IPE_VA_INDEP_NUM; i++) {
		ipe_get_indep_va_win_rslt(&rst, i);
		p_va_rst->g1_h[i] = rst.va_g1_h;
		p_va_rst->g1_v[i] = rst.va_g1_v;
		p_va_rst->g2_h[i] = rst.va_g2_h;
		p_va_rst->g2_v[i] = rst.va_g2_v;
		p_va_rst->g1_h_cnt[i] = rst.va_cnt_g1_h;
		p_va_rst->g1_v_cnt[i] = rst.va_cnt_g1_v;
		p_va_rst->g2_h_cnt[i] = rst.va_cnt_g2_h;
		p_va_rst->g2_v_cnt[i] = rst.va_cnt_g2_v;

	}

	return E_OK;
}


static ER kdrv_ipe_get_edge_stcs(UINT32 id, void *data)
{
	KDRV_IPE_EDGE_STCS *p_rst = (KDRV_IPE_EDGE_STCS *)data;
	IPE_EDGE_STCS_RSLT rst = {0};

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	ipe_get_edge_stcs(&rst);

	p_rst->localmax_max = rst.localmax_max;
	p_rst->coneng_max = rst.coneng_max;
	p_rst->coneng_avg = rst.coneng_avg;

	return E_OK;
}

static ER kdrv_ipe_get_edge_thres_auto(UINT32 id, void *data)
{
	KDRV_IPE_EDGE_THRES_AUTO *p_rst = (KDRV_IPE_EDGE_THRES_AUTO *)data;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	p_rst->region_th_auto.th_flat = ((p_rst->stcs.coneng_avg * p_rst->flat_ratio) >> 8);
	p_rst->region_th_auto.th_edge = max(((p_rst->stcs.coneng_max * p_rst->edge_ratio) >> 8), (p_rst->region_th_auto.th_flat + 1));
	p_rst->region_th_auto.th_flat_hld = (p_rst->region_th_auto.th_flat * 200) >> 8;
	p_rst->region_th_auto.th_edge_hld = p_rst->region_th_auto.th_edge;
	p_rst->region_th_auto.th_lum_hld = ((p_rst->stcs.localmax_max * p_rst->luma_ratio) >> 8);

	return E_OK;
}

static ER kdrv_ipe_get_defog_stcs(UINT32 id, void *data)
{
	KDRV_IPE_DEFOG_STCS *p_rst = (KDRV_IPE_DEFOG_STCS *)data;
	DEFOG_STCS_RSLT rst = {0};

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	ipe_get_defog_stcs(&rst);

	p_rst->dfg_airlight[0] = rst.airlight[0];
	p_rst->dfg_airlight[1] = rst.airlight[1];
	p_rst->dfg_airlight[2] = rst.airlight[2];

	if ((rst.airlight[0] == 0) && (rst.airlight[1] == 0) && (rst.airlight[2] == 0)) {
		p_rst->dfg_airlight[0] = 1023;
		p_rst->dfg_airlight[1] = 1023;
		p_rst->dfg_airlight[2] = 1023;
	}

	return E_OK;
}

static ER kdrv_ipe_get_ext_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_EXTEND_INFO *)data = g_kdrv_ipe_info[id].ext_info;

	return E_OK;
}


static ER kdrv_ipe_get_param_edge_region_strength(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	*(KDRV_IPE_EDGE_REGION_PARAM *)data = g_kdrv_ipe_info[id].edge_region_param;

	return E_OK;
}



KDRV_IPE_GET_FP kdrv_ipe_get_fp[KDRV_IPE_PARAM_MAX] = {
	kdrv_ipe_get_opencfg,
	kdrv_ipe_get_in_img_info,
	kdrv_ipe_get_in_addr,

	kdrv_ipe_get_out_img_info,
	kdrv_ipe_get_out_addr,

	kdrv_ipe_get_subin_dma_addr,
	kdrv_ipe_get_subout_dma_addr,

	kdrv_ipe_get_dram_out_mode,
	kdrv_ipe_get_single_out_channel,

	kdrv_ipe_get_isrcb,
	kdrv_ipe_get_interrupt_en,
	NULL,

	kdrv_ipe_get_param_eext,
	kdrv_ipe_get_param_eext_tonemap,
	kdrv_ipe_get_param_edge_overshoot,
	kdrv_ipe_get_param_edge_proc,
	kdrv_ipe_get_param_rgblpf,
	kdrv_ipe_get_param_pfr,

	kdrv_ipe_get_param_cc_en,
	kdrv_ipe_get_param_cc,
	kdrv_ipe_get_param_ccm,
	kdrv_ipe_get_param_cctrl_en,
	kdrv_ipe_get_param_cctrl,
	kdrv_ipe_get_param_cctrl_ct,
	kdrv_ipe_get_param_cadjee,
	kdrv_ipe_get_param_cadjyccon,
	kdrv_ipe_get_param_cadjcofs,
	kdrv_ipe_get_param_cadjrand,
	kdrv_ipe_get_param_cadjhue,
	kdrv_ipe_get_param_cadjfixth,
	kdrv_ipe_get_param_cadjmask,
	kdrv_ipe_get_param_cst,
	kdrv_ipe_get_param_cstp,

	kdrv_ipe_get_param_gammarand,
	kdrv_ipe_get_param_gammacurve,
	kdrv_ipe_get_param_ycurve,

	kdrv_ipe_get_param_subimg,
	kdrv_ipe_get_param_defog,
	kdrv_ipe_get_param_lce,
	kdrv_ipe_get_param_edge_debug_mode,

	kdrv_ipe_get_param_eth,

	kdrv_ipe_get_va_addr,
	kdrv_ipe_get_va_win_info,
	kdrv_ipe_get_param_va,
	kdrv_ipe_get_va_rslt,
	kdrv_ipe_get_va_indep_rslt,
	kdrv_ipe_get_edge_stcs,
	kdrv_ipe_get_edge_thres_auto,
	kdrv_ipe_get_defog_stcs,

	kdrv_ipe_get_ext_info,
	NULL,
	NULL,
	NULL,
	NULL,
	kdrv_ipe_get_param_edge_region_strength,
};

INT32 kdrv_ipe_get(UINT32 id, KDRV_IPE_PARAM_ID parm_id, VOID *p_param)
{
	ER rt = E_OK;
	KDRV_IPE_HANDLE *p_handle;
	UINT32 ign_chk;
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);

	//DBG_IND("id = 0x%x\r\n", id);
	if (KDRV_DEV_ID_ENGINE(id) != KDRV_VIDEOPROCS_IPE_ENGINE0) {
		DBG_ERR("Invalid engine 0x%x\r\n", (unsigned int)KDRV_DEV_ID_ENGINE(id));
		return E_ID;
	}

	p_handle = &g_kdrv_ipe_handle_tab[channel];
	if (kdrv_ipe_chk_handle(p_handle) == NULL) {
		DBG_ERR("KDRV IPE: illegal handle 0x%.8x\r\n", (unsigned int)&g_kdrv_ipe_handle_tab[channel]);
		return E_SYS;
	}

	if ((p_handle->sts & KDRV_IPE_HANDLE_LOCK) != KDRV_IPE_HANDLE_LOCK) {
		DBG_ERR("KDRV IPE: illegal handle sts 0x%.8x\r\n", (unsigned int)p_handle->sts);
		return E_SYS;
	}

	ign_chk = (KDRV_IPE_IGN_CHK & parm_id);
	parm_id = parm_id & (~KDRV_IPE_IGN_CHK);

	if (!ign_chk) {
		kdrv_ipe_lock(p_handle, TRUE);
	}
	if (kdrv_ipe_get_fp[parm_id] != NULL) {
		rt = kdrv_ipe_get_fp[parm_id](channel, p_param);
	}
	if (!ign_chk) {
		kdrv_ipe_lock(p_handle, FALSE);
	}

	return rt;
}

static CHAR *kdrv_ipe_dump_str_in_src(KDRV_IPE_IN_SRC in_src)
{
	CHAR *str_in_src[2 + 1] = {
		"DIRECT",
		"DRAM",
		"ERROR",
	};

	if (in_src >= 2) {
		return str_in_src[2];
	}
	return str_in_src[in_src];
}

static CHAR *kdrv_ipe_dump_str_in_fmt(KDRV_IPE_IN_FMT in_fmt)
{
	CHAR *str_in_fmt[4 + 1] = {
		"Y_PACK_UV444",
		"Y_PACK_UV422",
		"Y_PACK_UV420",
		"Y",
		"ERROR",
	};

	if (in_fmt >= 4) {
		return str_in_fmt[4];
	}
	return str_in_fmt[in_fmt];
}

static CHAR *kdrv_ipe_dump_str_out_fmt(KDRV_IPE_OUT_FMT out_fmt)
{
	CHAR *str_out_fmt[4 + 1] = {
		"Y_PACK_UV444",
		"Y_PACK_UV422",
		"Y_PACK_UV420",
		"Y",
		"ERROR",
	};

	if (out_fmt >= 4) {
		return str_out_fmt[4];
	}
	return str_out_fmt[out_fmt];
}

static CHAR *kdrv_ipe_dump_str_enable(BOOL enable)
{
	CHAR *str_enable[2] = {
		"0",
		"1",
	};
	return str_enable[enable];
}

UINT32 str_inte_en_mapping[][2] = {
	{0, 0x00000000,},
	{1, 0x00000002,},
	{2, 0x00000004,},
	{3, 0x00000008,},
	{4, 0x00000010,},
	{5, 0x00000020,},
	{6, 0x00000040,},
	{KDRV_IPE_MAX_INTEEN_NUM, 0xFFFFFFFF,},
};

#if defined __UITRON || defined __ECOS
void kdrv_ipe_dump_info(void (*dump)(char *fmt, ...))
#else
void kdrv_ipe_dump_info(int (*dump)(const char *fmt, ...))
#endif
{
	BOOL b_print_header;
	BOOL b_open[KDRV_IPP_MAX_CHANNEL_NUM] = {FALSE};
	//BOOL b_open[KDRV_IPE_HANDLE_MAX_NUM] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
	UINT32 id;

	if (g_kdrv_ipe_open_cnt == 0) {
		dump("\r\n[IPE] not open\r\n");
		return;
	}

	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((g_kdrv_ipe_init_flg & (1 << id)) == (UINT32)(1 << id)) {
			b_open[id] = TRUE;
			kdrv_ipe_lock(kdrv_ipe_entry_id_conv2_handle(id), TRUE);
		}
	}

	// dump gamma & 3dlut from register
	{
		extern UINT32 ipe_dbg_gamm_read_en;

		if (ipe_dbg_gamm_read_en != 0) {
			ipe_read_gamma(TRUE);
			ipe_dbg_gamm_read_en = 0;
		}
	}
	/*
	ipe_read_gamma(TRUE);
	//ipe_read3DLut(TRUE);
	*/

	dump("\r\n[KDRV IPE]\r\n");
	dump("\r\n-----ctrl info-----\r\n");
	dump("%12s%12s\r\n", "init_flg", "trig_id");
	dump("%#12x  0x%.8x\r\n", g_kdrv_ipe_init_flg, g_kdrv_ipe_trig_hdl);

	/**
	    input info
	*/
	dump("\r\n-----input info-----\r\n");
	dump("%3s%12s%12s%12s%14s%12s%12s%12s%12s\r\n", "id", "addr_y", "addr_uv", "in_src", "type", "y_width", "y_height", "y_line_ofs", "uv_line_ofs");
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if (!b_open[id]) {
			continue;
		}
		dump("%3d%#12x%#12x%12s%14s%12d%12d%12d%12d\r\n", id
			 , g_kdrv_ipe_info[id].in_pixel_addr.addr_y, g_kdrv_ipe_info[id].in_pixel_addr.addr_uv
			 , kdrv_ipe_dump_str_in_src(g_kdrv_ipe_info[id].in_img_info.in_src)
			 , kdrv_ipe_dump_str_in_fmt(g_kdrv_ipe_info[id].in_img_info.type)
			 , g_kdrv_ipe_info[id].in_img_info.ch.y_width, g_kdrv_ipe_info[id].in_img_info.ch.y_height
			 , g_kdrv_ipe_info[id].in_img_info.ch.y_line_ofs, g_kdrv_ipe_info[id].in_img_info.ch.uv_line_ofs);
	}

	/**
	    ouput info
	*/
	dump("\r\n-----output info-----\r\n");
	dump("%3s%12s%12s%14s%12s\r\n", "id", "AddrY", "AddrUV", "type", "dram_out");
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if (!b_open[id]) {
			continue;
		}
		dump("%3d%#12x%#12x%14s%12d\r\n", id
			 , g_kdrv_ipe_info[id].out_pixel_addr.addr_y, g_kdrv_ipe_info[id].out_pixel_addr.addr_uv
			 , kdrv_ipe_dump_str_out_fmt(g_kdrv_ipe_info[id].out_img_info.type)
			 , g_kdrv_ipe_info[id].out_img_info.dram_out);

	}

	/**
	    IQ function enable info
	*/
	dump("\r\n-----IQ function enable info-----\r\n");
	dump("%3s%12s%12s%12s%12s%12s%12s%12s%12s%12s%12s%12s\r\n", "id"
		 , "RGBLPF", "PFR", "CC", "CCTRL", "DEFOG", "LCE"
		 , "GAMYRAND", "GAMMA", "YCURVE", "VA", "VA_INDEP");
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if (!b_open[id]) {
			continue;
		}
		dump("%3d%12s%12s%12s%12s%12s%12s%12s%12s%12s%12s%12s\r\n", id
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].rgb_lpf_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].pfr_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].cc_en_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].cctrl_en_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].dfg_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].lce_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].gamy_rand_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].gamma.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].y_curve.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].va_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].va_param.indep_va_enable));
	}
	dump("%3s%12s%12s%12s%12s%12s%12s%12s%12s%12s%12s\r\n", "id"
		 , "CADJ_EE", "CADJ_YCCON", "CADJ_COFS", "CADJ_RAND", "CADJ_FIXTH", "CADJ_MASK", "HUE", "CST", "CSTP", "ETH");
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if (!b_open[id]) {
			continue;
		}
		dump("%3d%12s%12s%12s%12s%12s%12s%12s%12s%12s%12s\r\n", id
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].cadj_ee_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].cadj_yccon_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].cadj_cofs_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].cadj_rand_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].cadj_fixth_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].cadj_mask_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].cadj_hue_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].cst_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].cstp_param.enable)
			 , kdrv_ipe_dump_str_enable(g_kdrv_ipe_info[id].eth.enable));
	}

	/**
	    IQ parameter info
	*/
	dump("\r\n-----IQ parameter info-----\r\n");
	// KDRV_IPE_SET_PARAM_EEXT
	// TODO: if b_open[id], dump parameter

	// KDRV_IPE_SET_PARAM_EPROC
	// TODO: if b_open[id], dump parameter

	// KDRV_IPE_SET_PARAM_RGBLPF
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].rgb_lpf_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s\r\n", "RGBLPF", "id", "enable");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].rgb_lpf_param.enable);
	}


	// KDRV_IPE_SET_PARAM_PFR
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].pfr_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s\r\n", "PFR", "id", "enable");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].pfr_param.enable);
	}

	// KDRV_IPE_SET_PARAM_CC
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].cc_en_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s\r\n", "CC", "id", "enable");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].cc_en_param.enable);
	}

	// KDRV_IPE_SET_PARAM_CCTRL
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].cctrl_en_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s\r\n", "CCTRL", "id", "enable");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].cctrl_en_param.enable);
	}

	// KDRV_IPE_SET_PARAM_DEFOG
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].dfg_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s\r\n", "DEFOG", "id", "enable");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].dfg_param.enable);
	}

	// KDRV_IPE_SET_PARAM_LCE
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].lce_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s\r\n", "LCE", "id", "enable");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].lce_param.enable);
	}

	// KDRV_IPE_SET_PARAM_SUBIMG
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].subout_dma_addr.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s%12s%12s%12s%12s%12s\r\n", "SubImg", "id", "enable", "Addr", "LofsIn", "LofsOut", "SUB_SIZEX", "SUB_SIZEY");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d  0x%.8x%12d%12d%12d%12d\r\n"
			 , "--", id
			 , g_kdrv_ipe_info[id].subout_dma_addr.enable
			 , g_kdrv_ipe_info[id].subout_dma_addr.addr
			 , g_kdrv_ipe_info[id].subimg_param.subimg_lofs_in
			 , g_kdrv_ipe_info[id].subimg_param.subimg_lofs_out
			 , g_kdrv_ipe_info[id].subimg_param.subimg_size.h_size
			 , g_kdrv_ipe_info[id].subimg_param.subimg_size.v_size);
	}


	// KDRV_IPE_SET_PARAM_CADJ_EE
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].cadj_ee_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s\r\n", "CADJ_EE", "id", "enable");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].cadj_ee_param.enable);
	}

	// KDRV_IPE_SET_PARAM_CADJ_YCCON
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].cadj_yccon_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s%12s%12s\r\n", "CADJ_YCCON", "id", "enable", "y_con", "c_con");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d%12d%12d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].cadj_yccon_param.enable, g_kdrv_ipe_info[id].cadj_yccon_param.y_con, g_kdrv_ipe_info[id].cadj_yccon_param.c_con);
	}

	// KDRV_IPE_SET_PARAM_CADJ_COFS
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].cadj_cofs_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s\r\n", "CADJ_COFS", "id", "enable");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].cadj_cofs_param.enable);
	}

	// KDRV_IPE_SET_PARAM_CADJ_RAND
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].cadj_rand_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s\r\n", "CADJ_RAND", "id", "enable");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].cadj_rand_param.enable);
	}

	// KDRV_IPE_SET_PARAM_CADJ_FIXTH
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].cadj_fixth_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s\r\n", "CADJ_FIXTH", "id", "enable");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].cadj_fixth_param.enable);
	}

	// KDRV_IPE_SET_PARAM_CADJ_MASK
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].cadj_mask_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s\r\n", "CADJ_MASK", "id", "enable");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].cadj_mask_param.enable);
	}

	// KDRV_IPE_SET_PARAM_HUE
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].cadj_hue_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s\r\n", "HUE", "id", "enable");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].cadj_hue_param.enable);
	}

	// KDRV_IPE_SET_PARAM_CST
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].cst_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s%12s%12s%7s%7s%7s%7s%7s%7s%7s%7s%7s\r\n"
				 , "CST", "id", "enable", "cst_off_sel"
				 , "cst_coef:", "yr", "yg", "yb", "ur", "ug", "ub", "vr", "vg", "vb");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d%12d%12s%7d%7d%7d%7d%7d%7d%7d%7d%7d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].cst_param.enable, g_kdrv_ipe_info[id].cst_param.cst_off_sel
			 , "", g_kdrv_ipe_info[id].cst_param.cst_coef[0], g_kdrv_ipe_info[id].cst_param.cst_coef[1]
			 , g_kdrv_ipe_info[id].cst_param.cst_coef[2], g_kdrv_ipe_info[id].cst_param.cst_coef[3]
			 , g_kdrv_ipe_info[id].cst_param.cst_coef[4], g_kdrv_ipe_info[id].cst_param.cst_coef[5]
			 , g_kdrv_ipe_info[id].cst_param.cst_coef[6], g_kdrv_ipe_info[id].cst_param.cst_coef[7]
			 , g_kdrv_ipe_info[id].cst_param.cst_coef[8]);
	}

	// KDRV_IPE_SET_PARAM_CSTP
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].cstp_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s\r\n", "CSTP", "id", "enable");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].cstp_param.enable);
	}

	// KDRV_IPE_SET_PARAM_GAMYRAND
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].gamy_rand_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s\r\n", "GAMYRAND", "id", "enable");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].gamy_rand_param.enable);
	}

	// KDRV_IPE_SET_PARAM_GAMMA
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].gamma.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s\r\n", "GAMMA", "id", "enable");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].gamma.enable);
	}

	// KDRV_IPE_SET_PARAM_YCURVE
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].y_curve.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s%12s\r\n", "YCURVE", "id", "enable", "SEL");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d%12d\r\n"
			 , "--", id
			 , g_kdrv_ipe_info[id].y_curve.enable
			 , g_kdrv_ipe_info[id].y_curve.ycurve_sel);
	}

	// KDRV_IPE_SET_PARAM_ETH
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].eth.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s\r\n", "ETH", "id", "enable");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d\r\n"
			 , "--", id, g_kdrv_ipe_info[id].eth.enable);
	}

	// KDRV_IPE_SET_PARAM_VA
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].va_param.enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s%12s%12s%12s%12s%12s\r\n", "VA", "id", "enable", "Addr", "LofsOut", "OUTSEL", "WIN_X", "WIN_Y");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d  0x%.8x%12d%12d%12d%12d\r\n"
			 , "--", id
			 , g_kdrv_ipe_info[id].va_param.enable
			 , g_kdrv_ipe_info[id].va_addr.addr_va
			 , g_kdrv_ipe_info[id].va_param.va_lofs
			 , g_kdrv_ipe_info[id].va_param.va_out_grp1_2
			 , g_kdrv_ipe_info[id].va_param.win_num.h
			 , g_kdrv_ipe_info[id].va_param.win_num.w);
	}


	// KDRV_IPE_SET_PARAM_INDEP_VA
	b_print_header = FALSE;
	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if ((!b_open[id]) || (!g_kdrv_ipe_info[id].va_param.indep_va_enable)) {
			continue;
		}
		if (!b_print_header) {
			dump("%12s%3s%8s%12s%12s%12s%12s%12s\r\n", "VA_INDEP", "id", "enable", "WIN0_EN", "WIN1_EN", "WIN2_EN", "WIN3_EN", "WIN4_EN");
			b_print_header = TRUE;
		}
		dump("%12s%3d%8d%12d%12d%12d%12d%12d\r\n"
			 , "--", id
			 , g_kdrv_ipe_info[id].va_param.indep_va_enable
			 , g_kdrv_ipe_info[id].va_param.indep_win[0].enable
			 , g_kdrv_ipe_info[id].va_param.indep_win[1].enable
			 , g_kdrv_ipe_info[id].va_param.indep_win[2].enable
			 , g_kdrv_ipe_info[id].va_param.indep_win[3].enable
			 , g_kdrv_ipe_info[id].va_param.indep_win[4].enable);
	}

	for (id = 0; id < g_kdrv_ipe_channel_num; id++) {
		if (b_open[id]) {
			kdrv_ipe_lock(kdrv_ipe_entry_id_conv2_handle(id), FALSE);
		}
	}

}

#ifdef __KERNEL__
EXPORT_SYMBOL(kdrv_ipe_open);
EXPORT_SYMBOL(kdrv_ipe_close);
EXPORT_SYMBOL(kdrv_ipe_pause);
EXPORT_SYMBOL(kdrv_ipe_trigger);
EXPORT_SYMBOL(kdrv_ipe_init);
EXPORT_SYMBOL(kdrv_ipe_uninit);
EXPORT_SYMBOL(kdrv_ipe_buf_query);
EXPORT_SYMBOL(kdrv_ipe_buf_init);
EXPORT_SYMBOL(kdrv_ipe_buf_uninit);
EXPORT_SYMBOL(kdrv_ipe_set);
EXPORT_SYMBOL(kdrv_ipe_get);
EXPORT_SYMBOL(kdrv_ipe_dump_info);
#endif

