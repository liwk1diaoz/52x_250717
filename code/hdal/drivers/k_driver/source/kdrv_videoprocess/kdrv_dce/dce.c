/*
   DCE module driver

   NT96520 DCE module driver.

   @file       DCE.c
   @ingroup    mIDrvIPP_DCE
   @note       None

   Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include "dce_lib.h"
#include "dce_int.h"
#include "dce_reg.h"
//#include "dce_platform.h"
#include "comm/hwclock.h"
#include "dce_dbg.h"

//driver function control
#define PAUSE_AT_ISR_ERROR 0
#define DCE_DRV_DEBUG 0
#define DCE_CTRL_SRAM_EN 0
#define DCE_DMA_CACHE_HANDLE (1)
#define PRINT_2DLUT_AT_ISR_ERROR 0

#if defined (_NVT_EMULATION_)
#define FOR_CODEC_EN 0
#define DCE_POWER_SAVING_EN (0)//do not enable power saving in emulation
#else
#define FOR_CODEC_EN 1
#define DCE_POWER_SAVING_EN (1)
#endif

//constants
#define LCA_MAX_SCUP_FACT 4
#define MAX_OUT_SEG_CNT 128 //max out width 8188 / (min segment = min ipe ovlp 8 + min ime ovlp 16) ~= 342
#define MIN_GDC_INTRP_V 2 //bilinear 2x2 + input 1 line
#define DCE_DMA_OUT_NUM 4

static DCE_ENGINE_STATE g_dce_engine_state = DCE_STATE_IDLE;
static DCE_SEM_LOCK_STATUS g_dce_sem_lock_status = DCE_SEM_UNLOCKED;
static DCE_OPMODE dce_opmode = DCE_OPMODE_SIE_IME;
static DCE_LOAD_TYPE g_dce_load_type = DCE_DIRECT_START_LOAD_TYPE;
#if PRINT_2DLUT_AT_ISR_ERROR
static UINT32 g_2dlutAddr = 0;
#endif
static BOOL g_dce_clk_en = FALSE;

DCE_INT_STATUS_CNT dce_int_stat_cnt = {0};
#define DCE_ERR_MSG_FREQ 90
#define DCE_ERR_MSG_CNT_MAX 60

#if defined (_NVT_EMULATION_)
#include "comm/timer.h"
static TIMER_ID dce_timer_id;
BOOL dce_end_time_out_status = FALSE;
#endif

static ER dce_check_state_machine(DCE_ENGINE_STATE current_state, DCE_OPERATION operation);
static ER dce_lock_sem(void);
static ER dce_unlock_sem(void);
#if (DCE_DMA_CACHE_HANDLE == 1)
static DCE_OUT_DMA_INFO dce_dma_out[DCE_DMA_OUT_NUM] = {0};
static INT32 g_dma_och = 0;
#endif
static DCE_SEM_LOCK_STATUS dce_get_sem_lock_status(void);

#if 0//(DRV_SUPPORT_IST == ENABLE)
#else
static void (*g_pDceCBFunc)(UINT32 IntStatus) = NULL;
#endif

BOOL fw_power_saving_en = TRUE;

static void dce_attach(void);
static void dce_detach(void);
static void dce_set_reset(BOOL reset_en);
static void dce_set_ll_fire(void);
static void dce_set_ll_terminate(void);
static void dce_set_start(BOOL start_en);
static void dce_set_start_fd_load(void);
static void dce_set_load(DCE_LOAD_TYPE type);
static void dce_set_op_mode(DCE_OPMODE mode);
static void dce_set_stp_mode(DCE_STP_MODE mode);
static void dce_set_dram_out_mode(DCE_DRAM_OUTPUT_MODE outmode);
static void dce_set_dram_single_out_en(DCE_DRAM_SINGLE_OUT_EN *ch_en);
static void dce_enable_function(UINT32 func_sel);
static void dce_disable_function(UINT32 func_sel);
static void dce_set_function(UINT32 func_sel);
static void dce_set_sram_mode(DCE_SRAM_SEL ui_sram_clk_sel);
static void dce_set_gdc_mode(DCE_GDC_LUT_MODE ui_gdc_mode);
//WDR
static void dce_set_wdr_dither(DCE_WDR_DITHER *p_dither);
static void dce_set_wdr_scaling_fact(DCE_HV_FACTOR scaling_fact);
static void dce_set_wdr_subimg_size(DCE_IMG_SIZE subimg_size);
static void dce_set_wdr_subout(DCE_IMG_SIZE blk_size, DCE_HV_FACTOR blk_cent_fact);
static void dce_set_wdr_subimg_filtcoeff(UINT32 *p_filt_coeff);
static void dce_set_wdr_input_bld(WDR_IN_BLD_PARAM *p_input_bld);
static void dce_set_wdr_strength(WDR_STRENGTH_PARAM *p_strength_param);
static void dce_set_wdr_gain_ctrl(WDR_GAINCTRL_PARAM *p_gainctrl_param);
static void dce_set_wdr_outbld_en(BOOL enable);
static void dce_set_wdr_sat_reduct(WDR_SAT_REDUCT_PARAM *p_sat_param);
static void dce_set_wdr_tcurve_idx(UINT32 *p_wdr_tcurve_idx);
static void dce_set_wdr_tcurve_split(UINT32 *p_wdr_tcurve_split);
static void dce_set_wdr_tcurve_val(UINT32 *p_wdr_tcurve_val);
static void dce_set_wdr_outbld_idx(UINT32 *p_wdr_outbld_idx);
static void dce_set_wdr_outbld_split(UINT32 *p_wdr_outbld_split);
static void dce_set_wdr_outbld_val(UINT32 *p_wdr_outbld_val);
static void dce_set_hist_sel(DCE_HIST_SEL hist_sel);
static void dce_set_hist_step(UINT32 hstep, UINT32 vstep);
//
static void dce_set_color_gain(DCE_CFA_CGAIN *color_gain);
static void dce_set_d2d_rand(BOOL rand, BOOL randrst);
static void dce_set_d2d_fmt(DCE_D2D_FMT format);
static void dce_set_yuv2rgb_fmt(YUV2RGB_FMT format);
static void dce_set_img_size(DCE_IMG_SIZE *p_size);
static void dce_set_y_in_addr(UINT32 addr);
static void dce_set_y_in_lofs(UINT32 lineoffset);
static void dce_set_uv_in_addr(UINT32 addr);
static void dce_set_uv_in_lofs(UINT32 lineoffset);
static void dce_set_y_out_addr(UINT32 addr);
static void dce_set_y_out_lofs(UINT32 lineoffset);
static void dce_set_uv_out_addr(UINT32 addr);
static void dce_set_uv_out_lofs(UINT32 lineoffset);
static void dce_set_wdr_subin_addr(UINT32 addr);
static void dce_set_wdr_subin_lofs(UINT32 lineoffset);
static void dce_set_cfa_subout_addr(UINT32 addr);
static void dce_set_cfa_subout_lofs(UINT32 lineoffset);
static void dce_set_wdr_subout_addr(UINT32 addr);
static void dce_set_wdr_subout_lofs(UINT32 lineoffset);
static void dce_enable_int(UINT32 intr);
static void dce_disable_int(UINT32 intr);
static void dce_set_int_en(UINT32 intr);
static void dce_set_distort_corr_sel(DCE_DC_SEL dc_sel);
static void dce_set_img_center(DCE_IMG_CENT *p_center);
static void dce_set_img_ratio(DCE_IMG_RAT *p_ratio);
static void dce_set_dist_norm(DCE_DIST_NORM *p_dist_norm);
static void dce_set_cac_gain(DCE_CAC_GAIN *p_gain);
static void dce_set_cac_sel(DCE_CAC_SEL cac_sel);
static void dce_set_fov_gain(UINT32 fov_gain);
static void dce_set_fov_bound_sel(DCE_FOV_SEL fov_sel);
static void dce_set_fov_bound_rgb(DCE_FOV_RGB *p_fov_rgb);
static void dce_set_cfa_subout_sel(DCE_CFA_SUBOUT_PARAM *p_cfa_subout);
static void dce_set_cfa_interp(DCE_CFA_INTERP *p_cfa_interp);
static void dce_set_cfa_freq(UINT32 freq_th, UINT32 *p_freq_wt);
static void dce_set_cfa_luma_wt(UINT32 *p_luma_wt);
static void dce_set_cfa_corr(DCE_CFA_CORR *p_cfa_corr);
static void dce_set_cfa_fcs(DCE_CFA_FCS *p_fcs);
static void dce_set_cfa_ir_hfc(DCE_CFA_IR_HFC *p_ir_hfc);
static void dce_set_ir_sub(DCE_CFA_IR *p_ir_sub);
static void dce_set_pink_reduct(DCE_CFA_PINKR *p_pink_reduct);
static void dce_set_h_stp(UINT32 *p_hstp);
static void dce_set_hstripe_inc_dec(DCE_HSTP_INCDEC *p_inc_dec);
static void dce_set_hstripe_in_start(DCE_STP_INST in_start);
static void dce_set_ime_hstpout_ovlp_sel(DCE_HSTP_IMEOLAP_SEL out_ovlp_sel);
static void dce_set_ime_hstpout_ovlp_val(UINT32 out_ovlp_val);
static void dce_set_ipe_hstpout_ovlp_sel(DCE_HSTP_IPEOLAP_SEL out_ovlp_sel);
static void dce_set_img_crop(DCE_OUT_CROP *p_crop);
static void dce_set_dma_burst(DCE_DMA_BURST in_burst, DCE_DMA_BURST out_burst);
static void dce_set_sram_control(UINT32 sram_ctrl);
static void dce_set_g_geo_lut(UINT32 *p_lut);
static void dce_set_r_cac_lut(INT32 *p_lut);
static void dce_set_b_cac_lut(INT32 *p_lut);
static void dce_set_back_rsv_line(UINT32 rsv_line);
static void dce_set_2dlut_in_addr(UINT32 addr);
static void dce_set_2dlut_ofs(DCE_2DLUT_OFS *p_lut_ofs);
static void dce_set_2dlut_num(DCE_2DLUT_NUM lut_num);
static void dce_set_2dlut_ymin_auto_en(UINT32 top_ymin_auto_en);
static void dce_set_2dlut_scale_fact(DCE_2DLUT_SCALE *p_factor);
static void dce_sel_buffer_param(DCE_SRAM_SEL sram_sel);

INT32 seg_width = 0;
UINT32 partition_width;
UINT32 seg_buf_xst[MAX_OUT_SEG_CNT] = {0};
UINT32 seg_buf_xed[MAX_OUT_SEG_CNT] = {0};
UINT32 seg_buf_clm_xst[MAX_OUT_SEG_CNT] = {0};
UINT32 seg_buf_clm_xed[MAX_OUT_SEG_CNT] = {0};
UINT32 seg_bufv_g[MAX_OUT_SEG_CNT] = {0};
UINT32 seg_bufv_cac[MAX_OUT_SEG_CNT] = {0};
UINT32 buf_xo[MAX_OUT_SEG_CNT] = {0};
UINT32 buf_dxo[MAX_OUT_SEG_CNT] = {0};
UINT32 g_hstp[PARTITION_NUM] = {0};

//NT98520
#define BUFFER_TYPE_520 41 //42 for 520, 41 for IFE2 HW
#define HW_MAX_OUT_STP_WIDTH_520 2640
UINT32 buffer_h_rgb_520[BUFFER_TYPE_520] = {    //must be in descending order
	/*2688,*/   2624,   2560,   2496,   2432,   2368,   2304,   2240,   2176,   2112,  2048,
	1984,   1920,   1856,   1792,   1728,   1664,   1600,   1536,   1472,   1408,
	1344,   1280,   1216,   1152,   1088,   1024,   960,    896,    832,    768,
	704,    640,    576,    512,    448,    384,    320,    256,    192,    128,
	64
};

UINT32 buffer_v_rgb_dce_520[BUFFER_TYPE_520] = {
	/*8,*/      8,      8,      8,      8,      8,      8,     8,      8,      10,      10,
	10,     10,     10,     12,     12,     12,     12,     14,     14,     14,
	16,     16,     16,     18,     18,     20,     22,     24,     24,     28,
	30,     32,     36,     42,     48,     56,     66,     84,    112,    168,
	336
};

UINT32 buffer_v_rgb_cnn_520[BUFFER_TYPE_520] = {
	/*28,*/     30,     30,     32,     32,     32,     34,     34,     36,     36,      38,
	40,     40,     42,     44,     46,     48,     48,     52,     54,     56,
	58,     62,     64,     68,     72,     78,     82,     88,     96,    104,
	112,    124,    138,    156,    178,    208,    248,    312,    416,   624,
	1248
};

UINT32 buffer_h_520[BUFFER_TYPE_520], buffer_v_520[BUFFER_TYPE_520];

//NT98528
#define BUFFER_TYPE_528 64
#define HW_MAX_OUT_STP_WIDTH_528 4092
UINT32 buffer_h_rgb_528[BUFFER_TYPE_528] = {    //must be in descending order
	4096, 4032, 3968, 3904, 3840, 3776, 3712, 3648, 3584, 3520,
	3456, 3392, 3328, 3264, 3200, 3136, 3072, 3008, 2944, 2880,
	2816, 2752, 2688, 2624, 2560, 2496, 2432, 2368, 2304, 2240,
	2176, 2112, 2048, 1984, 1920, 1856, 1792, 1728, 1664, 1600,
	1536, 1472, 1408, 1344, 1280, 1216, 1152, 1088, 1024, 960,
	896, 832, 768, 704, 640, 576, 512, 448, 384, 320,
	256, 192, 128, 64
};

UINT32 buffer_v_rgb_dce_528[BUFFER_TYPE_528] = {
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 12, 12, 12, 12, 12, 12, 12, 12, 14,
	14, 14, 14, 14, 16, 16, 16, 16, 16, 18,
	18, 18, 20, 20, 20, 22, 22, 22, 24, 24,
	26, 26, 28, 30, 32, 32, 34, 36, 40, 42,
	44, 48, 52, 58, 64, 70, 80, 90, 106, 128,
	160, 212, 320, 640
};

UINT32 buffer_v_rgb_cnn_528[BUFFER_TYPE_528] = {
	24, 24, 24, 24, 26, 26, 26, 26, 28, 28,
	28, 28, 30, 30, 30, 32, 32, 32, 34, 34,
	34, 36, 36, 38, 38, 40, 40, 42, 42, 44,
	46, 46, 48, 50, 52, 54, 56, 58, 60, 62,
	64, 68, 70, 74, 78, 82, 86, 92, 98, 104,
	112, 120, 130, 142, 156, 174, 196, 224, 260, 312,
	392, 522, 784, 1022
};
UINT32 buffer_h_528[BUFFER_TYPE_528], buffer_v_528[BUFFER_TYPE_528];

UINT32 buffer_type = BUFFER_TYPE_520;
UINT32 max_out_stp_w = HW_MAX_OUT_STP_WIDTH_520;
UINT32 *p_buffer_h_rgb = NULL, *p_buffer_v_rgb = NULL;

//-----------------------------------Layer3-----------------------------------------------------//
ER dce_get_norm_stripe(DCE_NORMSTP_INFO *p_ns_info, DCE_NORMSTP_RSLT *p_ns_result, DCE_MODE_INFO *dce_mode_info)
{
	UINT32 image_size_h, image_size_v;
	BOOL   crop_en;
	//DCE_OPMODE OPmode;

	INT32  seg_num, i, j, inc_dec_search_tap = 12;
	UINT32 seg_idx;
	//UINT32 MinCropH = 16;
	UINT32 min_xst, max_xed;
	DCE_OUT_CROP crop_info;
	UINT32 buffer_percent = 100;
	UINT32 min_hstripe;

	UINT32 ostp_stx, ostp_edx_max, ostp_edx, istp_stx;
	UINT32 num_idx;
	UINT32 buffer_idx;
	UINT32 dxi = 0, dxo = 0, dyi = 0, dyi_cfa = 0, dyi_cac = 0;
	UINT32 max_inc, max_dec, inc_dec_margin = 16;
	UINT32 tmp_dxi, max_edxi;
	UINT32 buf_st_idx = 0;
	UINT32 mod_buffer_h[BUFFER_TYPE_528], mod_buffer_v[BUFFER_TYPE_528];
	UINT32 partition_buffer[PARTITION_NUM] = {0};

	DCE_HSTP_PARAM h_stp_param = {0};
	DCE_OP_PARAM op_param = {0};
	DCE_OUT_CROP seg_crop_param = {0};
	DCE_D2D_PARAM d2d_param = {0};
	DCE_HSTP_STATUS hstp_status = {0};

	//check current chip and sram selection to set parameters
	dce_sel_buffer_param(p_ns_info->sram_sel);

	//Assign image size
	image_size_h = p_ns_info->in_width;
	image_size_v = p_ns_info->in_height;

	//Min output buffer restriction
#if DCE_DRV_DEBUG
	DBG_DUMP("ipe_h_ovlap=%d, ime_h_ovlap=%d\r\n", p_ns_info->ipe_h_ovlap, p_ns_info->ime_h_ovlap);
#endif
	min_hstripe = p_ns_info->ipe_h_ovlap + p_ns_info->ime_h_ovlap;
	if ((p_ns_info->ipe_h_ovlap + p_ns_info->ime_h_ovlap) > 0) {
		seg_width = p_ns_info->ipe_h_ovlap + p_ns_info->ime_h_ovlap;
	} else {
		seg_width = 64;
	}

	//16 partition of image width
	partition_width = (image_size_h / PARTITION_NUM);

	//Get OP mode
	//OPmode = dce_mode_info->op_param.op_mode;

	//Get crop info
	crop_en = p_ns_info->crop_en;
	if (!crop_en) {
		crop_info.h_size = image_size_h;
		crop_info.v_size = image_size_v;
		crop_info.h_start = 0;
		crop_info.v_start = 0;
	} else {
		crop_info = p_ns_info->crop_info;
	}

	if (dce_mode_info->op_param.op_mode != DCE_OPMODE_DCE_D2D && crop_en == TRUE) {
		DBG_FUNC("Crop should be disabled at DCE direct mode!\r\n");
		return E_PAR;
	}

	//initialize stripe settings
	for (i = 0; i < PARTITION_NUM; i++) {
		p_ns_result->gdc_h_stp[i] = 0;
		g_hstp[i] = 0;
	}

	for (i = 0; i < MAX_OUT_SEG_CNT; i++) {
		seg_buf_xst[i] = 0;
		seg_buf_xed[i] = 0;
		seg_buf_clm_xst[i] = 0;
		seg_buf_clm_xed[i] = 0;
		seg_bufv_g[i] = 0;
		seg_bufv_cac[i] = 0;
		buf_xo[i] = 0;
		buf_dxo[i] = 0;
	}

	if (p_ns_info->stp_mode == DCE_STPMODE_SST) {
		return E_OK;
	} else if (p_ns_info->stp_mode != DCE_STPMODE_MST) {
		return dce_cal_stripe_with_ime_limitation(p_ns_info, p_ns_result);
	}

	//DC EN judgment
	if (!(dce_mode_info->func_en_msk & DCE_FUNC_DC)) {
		for (i = 0; i < PARTITION_NUM; i++) {
			//p_ns_result->gdc_h_stp[i] = ((max_out_stp_w - 8) >> 3) << 3;//4 pt cfa on each side
			p_ns_result->gdc_h_stp[i] = ((max_out_stp_w - 10) >> 3) << 3;//4 pt cfa on each side
		}
		p_ns_result->max_inc = 0;
		p_ns_result->max_dec = 0;
		return E_OK;
	}


	// ========= Trigger hw to calculate the boundary ========= //
	//disable X coordinate out of stripe boundaries
	dce_mode_info->int_en_msk = DCE_INT_ALL & (~DCE_INT_STPOB);

	//Set DCE mode info
	dce_set_mode(dce_mode_info);

	//Change stripe
	for (i = 0; i < PARTITION_NUM; i++) {
		g_hstp[i] = seg_width;
	}
	h_stp_param.p_h_stripe = g_hstp;
	h_stp_param.h_stp_inc_dec.max_inc = 0;
	h_stp_param.h_stp_inc_dec.max_dec = 0;
	h_stp_param.h_stp_in_start = DCE_1ST_STP_POS_CALC;
	h_stp_param.ime_out_hstp_ovlap = DCE_HSTP_IMEOLAP_16;
	h_stp_param.ime_ovlap_user_val = 0;//don't care
	h_stp_param.ipe_out_hstp_ovlap = DCE_HSTP_IPEOLAP_8;
	if (dce_change_h_stripe(&h_stp_param) != E_OK) {
		return E_PAR;
	}

	//Change OP mode
	op_param.op_mode = DCE_OPMODE_DCE_D2D;
	op_param.stp_mode = dce_mode_info->op_param.stp_mode;
	if (dce_change_operation(&op_param) != E_OK) {
		return E_PAR;
	}

	//CFA V line consumption decision
	//4x4 GDC interpolation(bilinear, bicubic) +
	//1(HW requirement)
	d2d_param.d2d_fmt = DCE_D2D_YUV422IN_YUV422OUT;

	//Change D2D param, DRAM in/out disabled
	d2d_param.y_in_addr = d2d_param.y_out_addr = d2d_param.y_in_lofs = d2d_param.y_out_lofs = 0;
	d2d_param.uv_in_addr = d2d_param.uv_out_addr = d2d_param.uv_in_lofs = d2d_param.uv_out_lofs = 0;
	if (dce_change_d2d_in_out(&d2d_param, op_param.op_mode) != E_OK) {
		return E_PAR;
	}

	//Change interrupt
	if (dce_change_int_en(DCE_INT_FRMEND, DCE_SET_SEL) != E_OK) {
		return E_SYS;
	}

	//disable X out of boundary
	if (dce_change_int_en(DCE_INT_STPOB, DCE_DISABLE_SEL) != E_OK) {
		return E_SYS;
	}

	//Disable histogram, wdr subout, cfa subout
	dce_change_func_en(DCE_FUNC_WDR_SUBOUT | DCE_FUNC_CFA_SUBOUT | DCE_FUNC_HISTOGRAM, DCE_DISABLE_SEL);

	//Enable Crop, Disable DRAM in/out
	dce_change_func_en(DCE_FUNC_CROP | DCE_FUNC_D2D_IOSTOP, DCE_ENABLE_SEL);

#if DCE_DRV_DEBUG
	DBG_DUMP("crop_info.h_start=%d, crop_info.h_size=%d \r\n", crop_info.h_start, crop_info.h_size);
#endif

	//Trigger DCE to get vertical buffer height
	for (i = 0; (crop_info.h_start + i * seg_width) < (crop_info.h_start + crop_info.h_size); i++) {

		if (i >= MAX_OUT_SEG_CNT) {
			DBG_ERR("Output segment number is larger than maximum %d!\r\n", MAX_OUT_SEG_CNT);
			break;
		}

		//Set Crop size
		seg_crop_param.v_start = 0;
		seg_crop_param.v_size = image_size_v;
		seg_crop_param.h_start = crop_info.h_start + i * seg_width;
		if ((seg_crop_param.h_start + seg_width) < (crop_info.h_start + crop_info.h_size)) {
			seg_crop_param.h_size = seg_width;
		} else {
			seg_crop_param.h_size = (crop_info.h_size - i * seg_width);
		}
#if DCE_DRV_DEBUG
		DBG_DUMP("seg crop start (%d, %d) size (%d, %d)\r\n", seg_crop_param.h_start, seg_crop_param.v_start, seg_crop_param.h_size, seg_crop_param.v_size);
#endif

		dce_change_img_out_crop(&seg_crop_param);
		//Trigger DCE
		dce_clear_int(DCE_INT_ALL);
		dce_clear_flag_frame_end();

		if (dce_start() != E_OK) {
			DBG_ERR("dce_start fail!\r\n");
			return E_SYS;
		}
		dce_wait_flag_frame_end(DCE_FLAG_NO_CLEAR);
		if (dce_pause() != E_OK) {
			DBG_ERR("dce_pause fail!\r\n");
			return E_SYS;
		}
		//Get status
		dce_get_h_stp_status(&hstp_status);
		if (hstp_status.buf_height_g < MIN_GDC_INTRP_V) {
			DBG_ERR("Seg %u h_start %u, G buffer height %u is less than CFA V lines %u\r\n", (unsigned int)i, (unsigned int)seg_crop_param.h_start, (unsigned int)hstp_status.buf_height_g, MIN_GDC_INTRP_V);
			return E_SYS;
		}

		seg_bufv_g[i] = hstp_status.buf_height_rgb - MIN_GDC_INTRP_V;
		seg_bufv_cac[i] = hstp_status.buf_height_pix;
		seg_buf_xst[i] = hstp_status.h_stp_st_x;
		seg_buf_xed[i] = hstp_status.h_stp_ed_x;
		seg_buf_clm_xst[i] = hstp_status.h_stp_clm_st_x;
		seg_buf_clm_xed[i] = hstp_status.h_stp_clm_ed_x;
#if DCE_DRV_DEBUG
		DBG_DUMP("(in)Segment %d, Xst %d, Xed %d, XstClm %d, XedClm %d, BufV_G %d, BufV_CAC %d \r\n", i, seg_buf_xst[i], seg_buf_xed[i], seg_buf_clm_xst[i], seg_buf_clm_xed[i], seg_bufv_g[i], seg_bufv_cac[i]);
		DBG_DUMP("(out)crop H start %d, V start %d, H size %d, V size %d\r\n", seg_crop_param.h_start, seg_crop_param.v_start, seg_crop_param.h_size, seg_crop_param.v_size);
#endif
	}
	dce_clear_int(DCE_INT_ALL);//clear remaining status
	seg_num = i;
	if (seg_num <= 0) {
		DBG_ERR("Output segment number %d is too small!\r\n", (int)seg_num);
		return E_PAR;
	}
	if (seg_num > MAX_OUT_SEG_CNT) {
		DBG_ERR("Output segment number %d is larger than maximum %d!\r\n", (int)seg_num, MAX_OUT_SEG_CNT);
		return E_PAR;
	}
#if DCE_DRV_DEBUG
	DBG_DUMP("seg_num = %d\r\n", seg_num);
#endif

	// ========= Max Inc Dec search from previous hw result ========= //
	max_inc = max_dec = 0;
	for (i = 0; i < seg_num; i++) {
		min_xst = seg_buf_xst[i];
		for (j = 1; j <= inc_dec_search_tap; j++) {
			min_xst = min(min_xst, seg_buf_xst[clamp(i + j, 0, seg_num - 1)]);
		}
		max_xed = seg_buf_xed[i];
		for (j = 1; j <= inc_dec_search_tap; j++) {
			max_xed = max(max_xed, seg_buf_xed[clamp(i - j, 0, seg_num - 1)]);
		}
		max_dec = max(max_dec, seg_buf_clm_xst[i] - min_xst);
		max_inc = max(max_inc, max_xed - seg_buf_clm_xed[i]);
#if DCE_DRV_DEBUG
		//DBG_DUMP("%dSTP: i %d, min_xst %d, seg_buf_clm_xst %d, seg_buf_clm_xed %d, max_xed %d\r\n", seg_width, i, min_xst, seg_buf_clm_xst[i], seg_buf_clm_xed[i], max_xed);
		//DBG_DUMP("%dSTP: max_inc %d, max_dec %d\r\n", seg_width, max_inc, max_dec);
#endif
	}
	//max_inc = (max_inc > 0) ? (max_inc + inc_dec_margin) : 0;
	//max_dec = (max_dec > 0) ? (max_dec + inc_dec_margin) : 0;
	max_inc = max_inc + inc_dec_margin;
	max_dec = max_dec + inc_dec_margin;
	// ============================================================== //

#if DCE_DRV_DEBUG
	//DBG_DUMP("%dSTP: max_inc %d, max_dec %d\r\n", seg_width, max_inc, max_dec);
#endif

	//Assign input buffer type and percentage
	for (i = 0; i < (INT16)buffer_type; i++) {
		mod_buffer_h[i] = (p_buffer_h_rgb[i] * buffer_percent / 100);
		mod_buffer_v[i] = p_buffer_v_rgb[i] * buffer_percent / 100 - 2;
	}

	//Stripe calculation
	ostp_stx = crop_info.h_start;
	num_idx = 0;
	buf_st_idx = 0;
	/*if(dce_mode_info->op_param.op_mode == DCE_OPMODE_RHE_IME)
	    buf_st_idx = 2;//for IPP direct, the first two buffer types are not supported*/
	while (ostp_stx < (crop_info.h_start + crop_info.h_size - 1)) {
		ostp_edx_max = min(ostp_stx + max_out_stp_w, (crop_info.h_start + crop_info.h_size));//buffer type scan end at the largest stripe width or input image end X
#if DCE_DRV_DEBUG
		DBG_DUMP(DBG_COLOR_GREEN"---------- Ostp %d: ostp_stx=%d, ostp_edx_max=%d ----------\r\n"DBG_COLOR_END, num_idx, ostp_stx, ostp_edx_max);
#endif

		for (buffer_idx = buf_st_idx; buffer_idx < buffer_type; buffer_idx++) {
			BOOL b_rstrc_hit_chk = FALSE;
#if DCE_DRV_DEBUG
			DBG_DUMP(DBG_COLOR_BLUE"Try buffer_idx %d, mod_buffer_h %d, mod_buffer_v %d\r\n"DBG_COLOR_END, buffer_idx, mod_buffer_h[buffer_idx], mod_buffer_v[buffer_idx]);
#endif

			//get index of the current output segment
			seg_idx = (ostp_stx - crop_info.h_start) / seg_width;
			max_edxi = seg_buf_xed[seg_idx];
			dxi = seg_buf_xed[seg_idx] - seg_buf_xst[seg_idx];
			dxi = dxi + (max_inc + max_dec);
			if (dxi >= mod_buffer_h[buffer_idx]) {
				DBG_ERR("Err: 1 segment h diff %d exceeds max buffer width[%d] %d \r\n", (unsigned int)dxi, buffer_idx, (unsigned int)mod_buffer_h[buffer_idx]);
				return E_SYS;
			}

			istp_stx = seg_buf_xst[seg_idx];//input image start X of current output segment start X
			dyi = dyi_cac = 0;
			//break when current buffer type can't cover current input stripe
			for (ostp_edx = ostp_stx; ostp_edx < ostp_edx_max; ostp_edx += seg_width) {
				seg_idx = (ostp_edx - crop_info.h_start) / seg_width;
				if (seg_idx >= (UINT32)seg_num) {
					//prevent index overflow
					DBG_DUMP("seg_idx %d does not exist, clamp at the last segment %d\r\n", (unsigned int)seg_idx, (unsigned int)(seg_num - 1));
					seg_idx = seg_num - 1;
				}
				dxo = ostp_edx - ostp_stx;
				max_edxi = max(max_edxi, seg_buf_xed[seg_idx]);
				dxi = max_edxi - istp_stx;
				dxi = dxi + (max_inc + max_dec);
				dyi += seg_bufv_g[seg_idx];
				dyi_cac = max(dyi_cac, seg_bufv_cac[seg_idx]);
				dyi_cfa = dyi + dyi_cac + MIN_GDC_INTRP_V;
#if DCE_DRV_DEBUG
				DBG_DUMP("buffer_idx %d, ostp_edx %d, dxo %d, dxi %d, dyi_cfa %d, seg_buf_xed %d, istp_stx %d, seg_idx %d\r\n", buffer_idx, ostp_edx, dxo, dxi, dyi_cfa, seg_buf_xed[seg_idx], istp_stx, seg_idx);
#endif
				if ((dxi >= mod_buffer_h[buffer_idx]) || (dyi_cfa >= mod_buffer_v[buffer_idx])) {
#if DCE_DRV_DEBUG
					DBG_DUMP(DBG_COLOR_BLUE"Restriction hit: buffer_idx %d, dxi %d, dyi_cfa %d, mod_buffer_h=%d, mod_buffer_v=%d\r\n"DBG_COLOR_END, buffer_idx, dxi, dyi_cfa, mod_buffer_h[buffer_idx], mod_buffer_v[buffer_idx]);
#endif
					b_rstrc_hit_chk = TRUE;
					break;
				}
			}
			if (!b_rstrc_hit_chk) {
#if DCE_DRV_DEBUG
				DBG_DUMP(DBG_COLOR_BLUE"Restriction not hit: Seg %d, H,V restrictions of buffer type %d was not hit.\r\n"DBG_COLOR_END, seg_idx, buffer_idx);
				DBG_DUMP("Limit current output stripe settings by MaxHStripe.\r\n");
				DBG_DUMP("buf_xo[%d]=%d, buf_dxo[%d]=%d, ostp_edx=%d, dxo=%d \r\n", num_idx - 1, buf_xo[num_idx - 1], num_idx - 1, buf_dxo[num_idx - 1], ostp_edx, dxo);
#endif
				if (num_idx > 0 && (((buf_xo[num_idx - 1] - buf_dxo[num_idx - 1]) / partition_width)) == ((ostp_edx - dxo) / partition_width)) {
					if (buf_dxo[num_idx - 1] < dxo && buf_dxo[num_idx - 1] != 0) {
						//DBG_DUMP("One HSTP has 2 stripe width, Case01 \r\n");
						buf_dxo[num_idx] = buf_dxo[num_idx - 1];
						buf_xo[num_idx] = ostp_stx + buf_dxo[num_idx];
						dxo = buf_dxo[num_idx];
					}
					if (buf_dxo[num_idx - 1] > dxo && dxo != 0) {
						//DBG_DUMP("One HSTP has 2 stripe width, Case02 \r\n");
						//DBG_DUMP("ostp_stx=%d, buf_xo[num_idx - 1]=%d, buf_dxo[num_idx - 1]=%d, ostp_edx=%d, dxo=%d \r\n", ostp_stx,buf_xo[num_idx - 1],buf_dxo[num_idx - 1],ostp_edx,dxo);
						ostp_stx = ostp_stx - buf_dxo[num_idx - 1] + dxo;
						buf_xo[num_idx - 1] = buf_xo[num_idx - 1] - buf_dxo[num_idx - 1] + dxo;
						buf_dxo[num_idx - 1] = dxo;
						buffer_idx = buf_st_idx;
						continue;
					}
				}
				ostp_edx = ostp_edx_max;
				dxo = ostp_edx - ostp_stx;

			}
#if DCE_DRV_DEBUG
			DBG_DUMP(DBG_COLOR_CYAN"buf_idx=[%d], ostp_stx=%d, ostp_edx=%d, dxi=%d, dyi_cfa=%d\r\n"DBG_COLOR_END, buffer_idx, ostp_stx, ostp_edx, dxi, dyi_cfa);

#endif

			// ========= Case0: Only horizontal distortion exceeds buffer width ========= //
			if ((dyi_cfa < mod_buffer_v[buffer_idx]) && ((dxi >= mod_buffer_h[buffer_idx]) || (ostp_edx > (ostp_edx_max - seg_width)))) {
#if DCE_DRV_DEBUG
				DBG_DUMP(DBG_COLOR_YELLOW"Case 0\r\n"DBG_COLOR_END);
				DBG_DUMP("dyi_cfa=%d, mod_buffer_v=%d, dxi=%d, mod_buffer_h=%d, Xo=%d, EdXo=%d, seg_width=%d\r\n", dyi_cfa, mod_buffer_v[buffer_idx], dxi, mod_buffer_h[buffer_idx], ostp_edx_max, ostp_edx, seg_width);
#endif
				//Case0: Only horizontal distortion exceeds buffer width
				if ((ostp_edx <= (ostp_edx_max - seg_width)) && ((ostp_edx - ostp_stx) < min_hstripe)) {
					DBG_ERR("Err:case0 output min stripe violation\r\n");
					return E_SYS;
				}

				buf_xo[num_idx] = ostp_edx;
				buf_dxo[num_idx] = dxo;

				if (num_idx > 0 && (((buf_xo[num_idx - 1] - buf_dxo[num_idx - 1]) / partition_width)) == ((ostp_edx - dxo) / partition_width)) {
					if (buf_dxo[num_idx - 1] < dxo) {
						//DBG_DUMP("One HSTP has 2 stripe width, Case01 \r\n");
						buf_dxo[num_idx] = buf_dxo[num_idx - 1];
						buf_xo[num_idx] = ostp_stx + buf_dxo[num_idx];
						dxo = buf_dxo[num_idx];
						ostp_edx = buf_xo[num_idx];
					}
					if (buf_dxo[num_idx - 1] > dxo) {
						//DBG_DUMP("One HSTP has 2 stripe width, Case02 \r\n");
						//DBG_DUMP("ostp_stx=%d, buf_xo[num_idx - 1]=%d, buf_dxo[num_idx - 1]=%d, ostp_edx=%d, dxo=%d \r\n", ostp_stx,buf_xo[num_idx - 1],buf_dxo[num_idx - 1],ostp_edx,dxo);
						ostp_stx = ostp_stx - buf_dxo[num_idx - 1] + dxo;
						buf_xo[num_idx - 1] = buf_xo[num_idx - 1] - buf_dxo[num_idx - 1] + dxo;
						buf_dxo[num_idx - 1] = dxo;
						buffer_idx = buf_st_idx;
						continue;
					}
				}

#if DCE_DRV_DEBUG
				DBG_DUMP("case0 buf_dxo[%d] %d, dxi %d, dyi_cfa %d, ostp_edx_max %d, mod_buffer_h %d, mod_buffer_v %d\r\n", num_idx, buf_dxo[num_idx], dxi, dyi_cfa, ostp_edx_max, mod_buffer_h[buffer_idx], mod_buffer_v[buffer_idx]);
#endif
				num_idx = num_idx + 1;

				//Assign new ostp_stx
				if (ostp_edx < ((crop_info.h_start + crop_info.h_size) - 1)) {
					ostp_stx = ostp_edx - (p_ns_info->ime_h_ovlap + p_ns_info->ipe_h_ovlap);
#if DCE_DRV_DEBUG
					DBG_DUMP("case0 Not last stripe(-overlap) ostp_stx=%d, ostp_edx=%d\r\n", ostp_stx, ostp_edx);
#endif
				} else {
					ostp_stx = ostp_edx;
#if DCE_DRV_DEBUG
					DBG_DUMP("Last stripe. StXo=%d, ostp_edx=%d\r\n", ostp_stx, ostp_edx);
#endif
				}
				//Aligned to seg_width
				//ostp_stx = ostp_stx - (ostp_stx % seg_width);
				break;
#if DCE_DRV_DEBUG
				DBG_DUMP("case0 Search next buffer type\r\n");
#endif
			} else if ((dyi_cfa >= mod_buffer_v[buffer_idx]) && (dxi < mod_buffer_h[buffer_idx]) && (buffer_idx <= (buffer_type - 1))) {
				// ========= Case1: Only vertical distortion exceeds buffer height ========= //
#if DCE_DRV_DEBUG
				DBG_DUMP(DBG_COLOR_YELLOW"Case 1\r\n"DBG_COLOR_END);
#endif
				//Case1: Only vertical distortion exceeds buffer height
				if (dxo < min_hstripe) {
#if DCE_DRV_DEBUG
					DBG_DUMP("case1: current output stripe is smaller than min stripe!\r\n");
#endif
				} else {
					//Check next buffer_idx buffer width
					tmp_dxi = seg_buf_xed[seg_idx] - seg_buf_xst[seg_idx];
					if ((buffer_idx < (buffer_type - 1) && ((dxi >= mod_buffer_h[buffer_idx + 1]) || (tmp_dxi >= mod_buffer_h[buffer_idx + 1]))) || (buffer_idx == buffer_type - 1)) {
						buf_xo[num_idx] = ostp_edx;
						buf_dxo[num_idx] = dxo;

						if (num_idx > 0 && (((buf_xo[num_idx - 1] - buf_dxo[num_idx - 1]) / partition_width)) == ((ostp_edx - dxo) / partition_width)) {
							if (buf_dxo[num_idx - 1] < dxo) {
								//DBG_DUMP("One HSTP has 2 stripe width, Case01 \r\n");
								buf_dxo[num_idx] = buf_dxo[num_idx - 1];
								buf_xo[num_idx] = ostp_stx + buf_dxo[num_idx];
								dxo = buf_dxo[num_idx];
								ostp_edx = buf_xo[num_idx];
							}
							if (buf_dxo[num_idx - 1] > dxo) {
								//DBG_DUMP("One HSTP has 2 stripe width, Case02 \r\n");
								//DBG_DUMP("ostp_stx=%d, buf_xo[num_idx - 1]=%d, buf_dxo[num_idx - 1]=%d, ostp_edx=%d, dxo=%d \r\n", ostp_stx,buf_xo[num_idx - 1],buf_dxo[num_idx - 1],ostp_edx,dxo);
								ostp_stx = ostp_stx - buf_dxo[num_idx - 1] + dxo;
								buf_xo[num_idx - 1] = buf_xo[num_idx - 1] - buf_dxo[num_idx - 1] + dxo;
								buf_dxo[num_idx - 1] = dxo;
								buffer_idx = buf_st_idx;
								continue;
							}
						}

#if DCE_DRV_DEBUG
						DBG_DUMP("case1 buf_dxo[%d] %d, dxi %d, dyi_cfa %d, mod_buffer_h %d, mod_buffer_v %d\r\n", num_idx, buf_dxo[num_idx], dxi, dyi_cfa, mod_buffer_h[buffer_idx], mod_buffer_v[buffer_idx]);
#endif
						num_idx = num_idx + 1;

						//Assign new ostp_stx
						if (ostp_edx < ((crop_info.h_start + crop_info.h_size) - 1)) {
							ostp_stx = ostp_edx - (p_ns_info->ime_h_ovlap + p_ns_info->ipe_h_ovlap);
#if DCE_DRV_DEBUG
							DBG_DUMP("case1 Not last stripe(-overlap) ostp_stx=%d, ostp_edx=%d\r\n", ostp_stx, ostp_edx);
#endif
						} else {
							ostp_stx = ostp_edx;
#if DCE_DRV_DEBUG
							DBG_DUMP("Last stripe. StXo=%d, ostp_edx=%d\r\n", ostp_stx, ostp_edx);
#endif
						}
						//Aligned to seg_width
						//ostp_stx = ostp_stx - (ostp_stx % seg_width);
						break;
					}
#if DCE_DRV_DEBUG
					DBG_DUMP("case1 Search next buffer type\r\n");
#endif
				}
			} else if ((dyi_cfa >= mod_buffer_v[buffer_idx]) && (dxi >= mod_buffer_h[buffer_idx])) {
				// ========= Case2: Both horizontal & vertical distortion exceeds buffer width ========= //
#if DCE_DRV_DEBUG
				DBG_DUMP(DBG_COLOR_YELLOW"Case 2\r\n"DBG_COLOR_END);
#endif
				//Case2: Both horizontal & vertical distortion exceeds buffer width
				if ((dxo - (seg_width / 2)) < min_hstripe) {
#if DCE_DRV_DEBUG
					DBG_DUMP("Err:case2 output min stripe violation \r\n");
#endif
				} else {
					//ostp_edx = ostp_edx - (seg_width / 2);
					//dxo = dxo - (seg_width / 2);
					buf_xo[num_idx] = ostp_edx;
					buf_dxo[num_idx] = dxo;

					if (num_idx > 0 && (((buf_xo[num_idx - 1] - buf_dxo[num_idx - 1]) / partition_width)) == ((ostp_edx - dxo) / partition_width)) {
						if (buf_dxo[num_idx - 1] < dxo) {
							//DBG_DUMP("One HSTP has 2 stripe width, Case01 \r\n");
							buf_dxo[num_idx] = buf_dxo[num_idx - 1];
							buf_xo[num_idx] = ostp_stx + buf_dxo[num_idx];
							dxo = buf_dxo[num_idx];
							ostp_edx = buf_xo[num_idx];
						}
						if (buf_dxo[num_idx - 1] > dxo) {
							//DBG_DUMP("One HSTP has 2 stripe width, Case02 \r\n");
							//DBG_DUMP("ostp_stx=%d, buf_xo[num_idx - 1]=%d, buf_dxo[num_idx - 1]=%d, ostp_edx=%d, dxo=%d \r\n", ostp_stx,buf_xo[num_idx - 1],buf_dxo[num_idx - 1],ostp_edx,dxo);
							ostp_stx = ostp_stx - buf_dxo[num_idx - 1] + dxo;
							buf_xo[num_idx - 1] = buf_xo[num_idx - 1] - buf_dxo[num_idx - 1] + dxo;
							buf_dxo[num_idx - 1] = dxo;
							buffer_idx = buf_st_idx;
							continue;
						}
					}

#if DCE_DRV_DEBUG
					DBG_DUMP("case2 buf_dxo[%d] %d, dxi %d, dyi_cfa %d(one extra segment value), mod_buffer_h %d, mod_buffer_v %d\r\n", num_idx, buf_dxo[num_idx], dxi, dyi_cfa, mod_buffer_h[buffer_idx], mod_buffer_v[buffer_idx]);
#endif
					num_idx = num_idx + 1;

					//Assign new ostp_stx
					if (ostp_edx < ((crop_info.h_start + crop_info.h_size) - 1)) {
						ostp_stx = ostp_edx - (p_ns_info->ime_h_ovlap + p_ns_info->ipe_h_ovlap);
#if DCE_DRV_DEBUG
						DBG_DUMP("case2 Not last stripe(-overlap) ostp_stx=%d, ostp_edx=%d\r\n", ostp_stx, ostp_edx);
#endif
					} else {
						ostp_stx = ostp_edx;
#if DCE_DRV_DEBUG
						DBG_DUMP("Last stripe. StXo=%d, ostp_edx=%d\r\n", ostp_stx, ostp_edx);
#endif
					}
					//Aligned to seg_width
					//ostp_stx = ostp_stx - (ostp_stx % seg_width);
					break;
				}
#if DCE_DRV_DEBUG
				DBG_DUMP("case2 Search next buffer type\r\n");
#endif
			} else if (buffer_idx == (buffer_type - 1)) {
				DBG_ERR("Err: No buffer type match \r\n");
				return E_SYS;
			}
		}
	}

#if DCE_DRV_DEBUG
	DBG_DUMP("buf_dxo:\r\n");
	for (i = 0; i < (INT16)num_idx; i++) {
		DBG_DUMP("%d ", buf_dxo[i]);
	}
	DBG_DUMP("\r\n");

	DBG_DUMP("buf_xo:\r\n");
	for (i = 0; i < (INT16)num_idx; i++) {
		DBG_DUMP("%d ", buf_xo[i]);
	}
	DBG_DUMP("\r\n");
#endif

	//Partition based stripe modification
	for (i = 0; i < (INT32)num_idx; i++) {
		INT32 par_sta_idx = ((INT32)buf_xo[i] - (INT32)buf_dxo[i] - 1) / (INT32)partition_width;
		INT32 par_end_idx = (INT32)buf_xo[i] / (INT32)partition_width;

		if (par_sta_idx > PARTITION_NUM) {
			par_sta_idx = 16;
		}
		if (par_end_idx > PARTITION_NUM) {
			par_end_idx = 16;
		}
		if (par_sta_idx < 0) {
			par_sta_idx = 0;
		}
		if (par_end_idx < 0) {
			par_end_idx = 0;
		}
#if DCE_DRV_DEBUG
		DBG_DUMP("par_sta_idx %d, par_end_idx %d\r\n", par_sta_idx, par_end_idx);
#endif
		for (j = par_sta_idx; j < par_end_idx; j++) {
			if (i != ((INT32)num_idx - 1)) {
				partition_buffer[j] = (buf_dxo[i] >> 3) << 3;
			} else {
				partition_buffer[j] = buf_dxo[i];//don't round down in case of last stripe not long enough
			}
		}

		//starting stripe
		if (i == 0 && par_sta_idx > 0) {
			for (j = 0; j < (INT16)par_sta_idx; j++) {
				partition_buffer[j] = (buf_dxo[i] >> 3) << 3;
			}
		}
		//ending stripe
		if (i == ((INT32)num_idx - 1) && par_end_idx < PARTITION_NUM) {
			for (j = par_end_idx; j < PARTITION_NUM; j++) {
				partition_buffer[j] = buf_dxo[i];//don't round down in case of last stripe not long enough
			}
		}
	}

#if DCE_DRV_DEBUG
	DBG_DUMP("partition_buffer:\r\n");
	for (i = 0; i < PARTITION_NUM; i++) {
		DBG_DUMP("%d ", partition_buffer[i]);
	}
	DBG_DUMP("\r\n");
#endif

	//Assign output result
	for (i = 0; i < PARTITION_NUM; i++) {
		//H stripe must be a multiple of 8(for IPE eth)
		p_ns_result->gdc_h_stp[i] = partition_buffer[i];
		if (p_ns_result->gdc_h_stp[i] < min_hstripe) {
			DBG_ERR("Err: Stripe %d size %d is smaller than min stripe %d \r\n", (unsigned int)i, (unsigned int)p_ns_result->gdc_h_stp[i], (unsigned int)min_hstripe);
			return E_SYS;
		}
		if (p_ns_result->gdc_h_stp[i] > max_out_stp_w) {
			DBG_ERR("Err: Stripe %d size %d is larger than max stripe %d \r\n", (unsigned int)i, (unsigned int)p_ns_result->gdc_h_stp[i], (unsigned int)max_out_stp_w);
			return E_SYS;
		}
	}
	p_ns_result->max_inc = max_inc;
	p_ns_result->max_dec = max_dec;

#if DCE_DRV_DEBUG
	DBG_DUMP("H stripe:\r\n");
	for (i = 0; i < PARTITION_NUM; i++) {
		DBG_DUMP("%d ", p_ns_result->gdc_h_stp[i]);
	}
	DBG_DUMP("\r\n");
#endif

	//enable X out of boundary
	dce_change_int_en(DCE_INT_STPOB, DCE_ENABLE_SEL);

	return E_OK;
}

static void dce_sel_buffer_param(DCE_SRAM_SEL sram_sel)
{
	if (nvt_get_chip_id() == CHIP_NA51055) {
		DBG_UNIT("520 DCE buffer parameter\r\n");
		buffer_type = BUFFER_TYPE_520;
		max_out_stp_w = HW_MAX_OUT_STP_WIDTH_520;
		p_buffer_h_rgb = buffer_h_rgb_520;
		if (sram_sel == DCE_SRAM_DCE) {
			p_buffer_v_rgb = buffer_v_rgb_dce_520;
		} else {
			p_buffer_v_rgb = buffer_v_rgb_cnn_520;
		}
	} else {
		DBG_UNIT("528 DCE buffer parameter\r\n");
		buffer_type = BUFFER_TYPE_528;
		max_out_stp_w = HW_MAX_OUT_STP_WIDTH_528;
		p_buffer_h_rgb = buffer_h_rgb_528;
		if (sram_sel == DCE_SRAM_DCE) {
			p_buffer_v_rgb = buffer_v_rgb_dce_528;
		} else {
			p_buffer_v_rgb = buffer_v_rgb_cnn_528;
		}
	}
}

ER dce_cal_stripe_with_ime_limitation(DCE_NORMSTP_INFO *p_ns_info, DCE_NORMSTP_RSLT *p_ns_result)
{
	DCE_OUT_CROP crop_info = {0};
	UINT32 ime_out_N = 0;//IME out stripe width
	INT32 i = 0;
	INT32 idx_2nd = 0, idx_final = 0;
	UINT32 ipe_ovlp_1side = p_ns_info->ipe_h_ovlap / 2;//total 16 for DCE stripe width keeping at 8x
	UINT32 ime_ovlp_1side = p_ns_info->ime_h_ovlap / 2;
	UINT32 ime_strp_multiple = 128;//IME stripe width is a multiple of 128
	UINT32 ipe_ime_ovlp_1side = ipe_ovlp_1side + ime_ovlp_1side;
	UINT32 strp_1st = 0;
	UINT32 strp_body = 0;
	UINT32 strp_final = 0;
	UINT32 partition_width = (p_ns_info->in_width / PARTITION_NUM);
	UINT32 stripe_num = 0;

#if DCE_DRV_DEBUG
	UINT32 ime_hn, ime_hl, ime_hm;
#endif

	stripe_num = (UINT32)p_ns_info->stp_mode;

	if (!p_ns_info->crop_en) {
		crop_info.h_size = p_ns_info->in_width;
		crop_info.v_size = p_ns_info->in_height;
		crop_info.h_start = 0;
		crop_info.v_start = 0;
	} else {
		crop_info = p_ns_info->crop_info;
	}

	/*
	 *if (!p_ns_info->ime_3dnr_en && (p_ns_info->ime_enc_en || p_ns_info->ime_p1_enc_en)) {
	 *    ime_strp_multiple = 32;
	 *}
	 */

	DBG_FUNC("ipe_ovlp_1side %d, ime_ovlp_1side %d, ime_strp_multiple %d, partition_width %d\r\n", ipe_ovlp_1side, ime_ovlp_1side, ime_strp_multiple, partition_width);

	if (p_ns_info->stp_mode == DCE_STPMODE_2ST) {
		ime_out_N = (crop_info.h_size / 2);
		ime_out_N += (((ime_out_N % ime_strp_multiple) == 0) ? 0 : (ime_strp_multiple - (ime_out_N % ime_strp_multiple)));//ceil_multiple
		if (ime_out_N < ime_strp_multiple) {
			DBG_ERR("ime_out_N %d is not a multiple of %d! Image width is too small!\r\n", (unsigned int)ime_out_N, (unsigned int)ime_strp_multiple);
			return E_PAR;
		}
		//DCE stripe settings
		strp_1st = ime_out_N + ipe_ime_ovlp_1side;//1st stripe, ime_out_N + IPE_ovlp + IME_ovlp
		strp_final = ((crop_info.h_size - ((stripe_num - 1) * ime_out_N - ipe_ime_ovlp_1side) + 7) >> 3) << 3;
		DBG_FUNC("strp_1st=%d, strp_final=%d\r\n", strp_1st, strp_final);

		idx_final = (INT32)((crop_info.h_start + (stripe_num - 1) * ime_out_N - ipe_ime_ovlp_1side - 1) / partition_width); //last stripe start idx
		DBG_FUNC("idx_final=%d\r\n", idx_final);
		for (i = 0; i < idx_final; i++) {
			p_ns_result->gdc_h_stp[i] = strp_1st;
		}
		for (i = idx_final; i < PARTITION_NUM; i++) {
			p_ns_result->gdc_h_stp[i] = strp_final;
		}
#if DCE_DRV_DEBUG
		//IME stripe settings
		ime_hm = 1;
		ime_hn = ime_out_N + ime_ovlp_1side;
		ime_hl = crop_info.h_size + (ime_hm * (ime_hn - 2 * ime_ovlp_1side));
#endif
	} else {
		unsigned int mod_cnt = 0;
		ime_out_N = (crop_info.h_size / stripe_num);//initialize equal stripe length
		DBG_FUNC("Initial ime_out_N=%d\r\n", ime_out_N);
#if FOR_CODEC_EN
		ime_out_N += (((ime_out_N % ime_strp_multiple) == 0) ? 0 : (ime_strp_multiple - (ime_out_N % ime_strp_multiple)));//ceil_multiple
#else
		ime_out_N -= (ime_out_N % ime_strp_multiple);//floor_multiple((width+2*ime_ovlp_1side)/4)
#endif
		DBG_FUNC("Codec ime_out_N=%d\r\n", ime_out_N);
		if (ime_out_N < ime_strp_multiple) {
			DBG_ERR("ime_out_N %d is not a multiple of %d! Image width is too small!\r\n", (unsigned int)ime_out_N, (unsigned int)ime_strp_multiple);
			return E_PAR;
		}

		//DCE stripe settings
		//1st stripe, ime_out_N + IPE,IME 1-side overlap
		strp_1st = ime_out_N + ipe_ime_ovlp_1side;
		//2nd~(final-1)th stripe, ime_out_N + IPE,IME 2-sides overlap
		strp_body = ime_out_N + 2 * ipe_ime_ovlp_1side;
		//final stripe, dce out width - ((stripe_num - 1) * ime_out_N - IPE,IME 1-side overlap)
		strp_final = ((crop_info.h_size - ((stripe_num - 1) * ime_out_N - ipe_ime_ovlp_1side) + 7) >> 3) << 3;
		while ((INT32)strp_final <= 0) {
			stripe_num -= 1;
			strp_final = ((crop_info.h_size - ((stripe_num - 1) * ime_out_N - ipe_ime_ovlp_1side) + 7) >> 3) << 3;
		}
		DBG_FUNC("Initial strp_1st=%d, strp_body=%d, strp_final=%d\r\n", strp_1st, strp_body, strp_final);

#if FOR_CODEC_EN
		while (strp_body > (1280 + 2 * ipe_ime_ovlp_1side)) { //For Codec 3 tile, 2nd tile must be <= 1280
			strp_body -= ime_strp_multiple;
			strp_final += ime_strp_multiple;
			mod_cnt++;
			DBG_FUNC("Codec mod_cnt=%d\r\n", mod_cnt);
		}
#endif

#if NEW_STRIPE_RULE
		if (stripe_num > 3) {
			while ((((INT32)strp_1st - (INT32)strp_final) >= ((INT32)ime_strp_multiple * 3 / 2)) && //minimize the difference between 1st and last stripe
					((crop_info.h_start + (strp_1st - ime_strp_multiple) - 2 * ipe_ime_ovlp_1side - 1) > partition_width)) { //Prevent strp_1st to be smaller than partition width
				strp_1st -= ime_strp_multiple;
				strp_final += ime_strp_multiple;
				mod_cnt++;
				DBG_FUNC("GDC mod_cnt=%d\r\n", mod_cnt);
			}
		}
#endif
		DBG_FUNC("strp_1st=%d, strp_body=%d, strp_final=%d\r\n", strp_1st, strp_body, strp_final);

		idx_2nd = (INT32)((crop_info.h_start + strp_1st - 2 * ipe_ime_ovlp_1side - 1) / partition_width); //2nd stripe start idx_2nd
		idx_final = (INT32)((crop_info.h_start + (stripe_num - 1) * ime_out_N - (ime_strp_multiple * mod_cnt) - ipe_ime_ovlp_1side - 1) / partition_width); //last stripe start idx
		DBG_FUNC("idx_2nd=%d, idx_final=%d\r\n", idx_2nd, idx_final);
		for (i = 0; i < idx_2nd; i++) {
			p_ns_result->gdc_h_stp[i] = strp_1st;
		}
		for (i = idx_2nd; i < idx_final; i++) {
			p_ns_result->gdc_h_stp[i] = strp_body;
		}
		for (i = idx_final; i < PARTITION_NUM; i++) {
			p_ns_result->gdc_h_stp[i] = strp_final;
		}
		//IME stripe settings
#if DCE_DRV_DEBUG
		ime_hm = (stripe_num - 1);//HM
		ime_hn = ime_out_N + ime_ovlp_1side;//HN
		ime_hl = crop_info.h_size - (ime_hm * (ime_hn - 2 * ime_ovlp_1side));//HL
#endif
	}
	DBG_FUNC("stripe_num=%d\r\n", stripe_num);
	DBG_FUNC("crop_info h_start=%d h_size=%d\r\n", crop_info.h_start, crop_info.h_size);
	DBG_FUNC("ime_out_N=%d\r\n", ime_out_N);

	if ((INT32)strp_1st <= 0) {
		DBG_ERR("strp_1st %d abnormal!\r\n", strp_1st);
		return E_PAR;
	}
	if (p_ns_info->stp_mode != DCE_STPMODE_2ST && ((INT32)strp_body <= 0)) {
		DBG_ERR("strp_body %d abnormal!\r\n", strp_body);
		return E_PAR;
	}
	if ((INT32)strp_final <= 0) {
		DBG_ERR("strp_final %d abnormal!\r\n", strp_final);
		return E_PAR;
	}
	if (idx_2nd < 0 || idx_2nd > 15) {
		DBG_ERR("idx_2nd %d abnormal!\r\n", idx_2nd);
		return E_PAR;
	}
	if (idx_final < 0 || idx_final > 15) {
		DBG_ERR("idx_final %d abnormal!\r\n", idx_final);
		return E_PAR;
	}
	for (i = 0; i < PARTITION_NUM; i++) {
		if (p_ns_result->gdc_h_stp[i] > max_out_stp_w) {
			DBG_ERR("dce hstp[%d] %d > max output stripe width %d!\r\n", i, p_ns_result->gdc_h_stp[i], max_out_stp_w);
			return E_PAR;
		}
	}

#if DCE_DRV_DEBUG
	DBG_DUMP("HN=%d\r\n", ime_hn);
	DBG_DUMP("HL=%d\r\n", ime_hl);
	DBG_DUMP("HM=%d\r\n", ime_hm);
#endif

	return E_OK;
}

#if 0
static ER dce_cal_stripe_no_limitation(DCE_NORMSTP_INFO *p_ns_info, DCE_NORMSTP_RSLT *p_ns_result)
{
	INT16 i;
	DCE_OUT_CROP crop_info;
	UINT32 stripe_num;
	//UINT32 Idx = 0;
	//UINT32 partition_width = (p_ns_info->in_width / PARTITION_NUM);

	//crop
	BOOL crop_en = p_ns_info->crop_en;
	if (!crop_en) {
		crop_info.h_size = p_ns_info->in_width;
		crop_info.v_size = p_ns_info->in_height;
		crop_info.h_start = 0;
		crop_info.v_start = 0;
	} else {
		crop_info = p_ns_info->crop_info;
	}

	DBG_IND("ipe_h_ovlap %d, ime_h_ovlap %d\r\n", p_ns_info->ipe_h_ovlap, p_ns_info->ime_h_ovlap);

#if 0
	//Stripe mode judgment
	if (p_ns_info->stp_mode == DCE_STPMODE_2ST) {
		//Idx = crop_info.h_start / partition_width;
		for (i = 0; i < PARTITION_NUM; i++) {
			//HSTP must be 8 pixel multiples, in 8 pixel unit
			if (crop_en == ENABLE) {
				p_ns_result->gdc_h_stp[i] = ((((crop_info.h_size + p_ns_info->ime_h_ovlap + p_ns_info->ipe_h_ovlap) / 2) + 7) >> 3) << 3;
			} else {
				p_ns_result->gdc_h_stp[i] = ((((p_ns_info->in_width + p_ns_info->ime_h_ovlap + p_ns_info->ipe_h_ovlap) / 2) + 7) >> 3) << 3;
			}
			if (p_ns_result->gdc_h_stp[i] > max_out_stp_w) {
				DBG_ERR("2ST %d exceeds max out stripe limit %d\r\n", p_ns_result->gdc_h_stp[i], max_out_stp_w);
				return E_SYS;
			}
		}
		return E_OK;
	} else if (p_ns_info->stp_mode == DCE_STPMODE_4ST) {
		//Idx = crop_info.h_start / partition_width;
		for (i = 0; i < PARTITION_NUM; i++) {
			//HSTP must be 8 pixel multiples, in 8 pixel unit
			if (crop_en == ENABLE) {
				p_ns_result->gdc_h_stp[i] = ((((crop_info.h_size + 3 * (p_ns_info->ime_h_ovlap + p_ns_info->ipe_h_ovlap)) / 4) + 7) >> 3) << 3;
			} else {
				p_ns_result->gdc_h_stp[i] = ((((p_ns_info->in_width + 3 * (p_ns_info->ime_h_ovlap + p_ns_info->ipe_h_ovlap)) / 4) + 7) >> 3) << 3;
			}
			if (p_ns_result->gdc_h_stp[i] > max_out_stp_w) {
				DBG_ERR("4ST %d exceeds max stripe limit %d\r\n", p_ns_result->gdc_h_stp[i], max_out_stp_w);
				return E_SYS;
			}
		}
		return E_OK;
	} else {
		return E_PAR;
	}
#else

	stripe_num = dce_trans_stripe_num(p_ns_info->stp_mode);

	//Stripe mode judgment
	for (i = 0; i < PARTITION_NUM; i++) {
		//HSTP must be 8 pixel multiples, in 8 pixel unit
		if (crop_en == ENABLE) {
			p_ns_result->gdc_h_stp[i] = ((((crop_info.h_size + (stripe_num - 1) * (p_ns_info->ime_h_ovlap + p_ns_info->ipe_h_ovlap)) / stripe_num) + 7) >> 3) << 3;
		} else {
			p_ns_result->gdc_h_stp[i] = ((((p_ns_info->in_width + (stripe_num - 1) * (p_ns_info->ime_h_ovlap + p_ns_info->ipe_h_ovlap)) / stripe_num) + 7) >> 3) << 3;
		}
		if (p_ns_result->gdc_h_stp[i] > max_out_stp_w) {
			DBG_ERR("%dST %d exceeds max stripe limit %d\r\n", (unsigned int)stripe_num, (unsigned int)p_ns_result->gdc_h_stp[i], (unsigned int)max_out_stp_w);
			return E_SYS;
		}
	}
	return E_OK;
#endif
}

ER dce_cal_simple_stripe_settings(DCE_NORMSTP_INFO *p_ns_info, DCE_NORMSTP_RSLT *p_ns_result)
{
	if (p_ns_info->ime_3dnr_en || p_ns_info->ime_enc_en || p_ns_info->ime_p1_enc_en) {
		return dce_cal_stripe_with_ime_limitation(p_ns_info, p_ns_result);
	} else {
		return dce_cal_stripe_no_limitation(p_ns_info, p_ns_result);
	}
}
#endif

VOID dce_get_output_stp_setting(DCE_NORMSTP_INFO *p_ns_info, DCE_NORMSTP_RSLT *p_ns_result)
{
	DCE_OUT_CROP crop_info;
	INT32 i = 0;

	if (!p_ns_info->crop_en) {
		crop_info.h_size = p_ns_info->in_width;
		crop_info.v_size = p_ns_info->in_height;
		crop_info.h_start = 0;
		crop_info.v_start = 0;
	} else {
		crop_info = p_ns_info->crop_info;
	}

	if (p_ns_info->stp_mode != DCE_STPMODE_SST) {
		UINT32 ui_out_img_hstp_stx = crop_info.h_start;
		UINT32 ui_partition_wd = (p_ns_info->in_width / PARTITION_NUM);
		UINT32 ui_cur_ime_out_stp_sum = 0;
		DBG_FUNC("in wd %d, crop st %d, crop wd %d\r\n", p_ns_info->in_width, crop_info.h_start, crop_info.h_size);
		DBG_FUNC("IPE_ovlp %d, IME_ovlp %d\r\n", p_ns_info->ipe_h_ovlap, p_ns_info->ime_h_ovlap);
		DBG_FUNC("stripe param:\r\n%u %u %u %u %u %u %u %u\r\n%u %u %u %u %u %u %u %u\r\n"
				, p_ns_result->gdc_h_stp[0] , p_ns_result->gdc_h_stp[1] , p_ns_result->gdc_h_stp[2] , p_ns_result->gdc_h_stp[3]
				, p_ns_result->gdc_h_stp[4] , p_ns_result->gdc_h_stp[5] , p_ns_result->gdc_h_stp[6] , p_ns_result->gdc_h_stp[7]
				, p_ns_result->gdc_h_stp[8] , p_ns_result->gdc_h_stp[9] , p_ns_result->gdc_h_stp[10] , p_ns_result->gdc_h_stp[11]
				, p_ns_result->gdc_h_stp[12] , p_ns_result->gdc_h_stp[13] , p_ns_result->gdc_h_stp[14] , p_ns_result->gdc_h_stp[15]);

		for (i = 0; i < IMG_MAX_STP_NUM; i++) {
			UINT32 ui_gdc_hstp_idx = 0;

			if (ui_out_img_hstp_stx % ui_partition_wd != 0 || ui_out_img_hstp_stx == 0) {
				ui_gdc_hstp_idx = ui_out_img_hstp_stx / ui_partition_wd;
			} else {
				ui_gdc_hstp_idx = ui_out_img_hstp_stx / ui_partition_wd - 1;
			}
			DBG_IND("ui_gdc_hstp_idx %u, ui_partition_wd %u, ui_out_img_hstp_stx %u, gdc_h_stp %u\r\n", ui_gdc_hstp_idx, ui_partition_wd, ui_out_img_hstp_stx, p_ns_result->gdc_h_stp[ui_gdc_hstp_idx]);
			p_ns_result->ui_dce_out_hstp[i] = (UINT16)p_ns_result->gdc_h_stp[ui_gdc_hstp_idx];
			if (i == 0) {
				p_ns_result->ui_ime_in_hstp[i] = (UINT16)(p_ns_result->gdc_h_stp[ui_gdc_hstp_idx] - p_ns_info->ipe_h_ovlap / 2);
				p_ns_result->ui_ime_out_hstp[i] = (UINT16)(p_ns_result->gdc_h_stp[ui_gdc_hstp_idx] - (p_ns_info->ipe_h_ovlap + p_ns_info->ime_h_ovlap) / 2);
			} else {
				p_ns_result->ui_ime_in_hstp[i] = (UINT16)(p_ns_result->gdc_h_stp[ui_gdc_hstp_idx] - p_ns_info->ipe_h_ovlap);
				p_ns_result->ui_ime_out_hstp[i] = (UINT16)(p_ns_result->gdc_h_stp[ui_gdc_hstp_idx] - (p_ns_info->ipe_h_ovlap + p_ns_info->ime_h_ovlap));
			}

			if ((ui_out_img_hstp_stx + p_ns_result->ui_dce_out_hstp[i]) < (crop_info.h_start + crop_info.h_size)) {
				ui_out_img_hstp_stx += ((UINT32)p_ns_result->ui_dce_out_hstp[i] - p_ns_info->ipe_h_ovlap - p_ns_info->ime_h_ovlap);
			} else {
				p_ns_result->ui_ime_out_hstp[i] = (UINT16)(crop_info.h_size - ui_cur_ime_out_stp_sum);
				p_ns_result->ui_ime_in_hstp[i] = (UINT16)(p_ns_result->ui_ime_out_hstp[i] + p_ns_info->ime_h_ovlap / 2);
				p_ns_result->ui_dce_out_hstp[i] = (UINT16)(p_ns_result->ui_ime_out_hstp[i] + (p_ns_info->ipe_h_ovlap + p_ns_info->ime_h_ovlap) / 2);
				break;
			}
			ui_cur_ime_out_stp_sum += p_ns_result->ui_ime_out_hstp[i];
			DBG_IND("hstp_stx %u, idx %u\r\n", ui_out_img_hstp_stx, ui_gdc_hstp_idx);
		}

		p_ns_result->ui_hstp_num = (i + 1);
	} else {
		p_ns_result->ui_hstp_num = 1;
		p_ns_result->ui_dce_out_hstp[0] = (UINT16)crop_info.h_size;
		p_ns_result->ui_ime_in_hstp[0] = (UINT16)crop_info.h_size;
		p_ns_result->ui_ime_out_hstp[0] = (UINT16)crop_info.h_size;
	}
	DBG_UNIT("total stripe num %u\r\n", p_ns_result->ui_hstp_num);
	DBG_FUNC("dce_out_hstp %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\r\n"
				 , p_ns_result->ui_dce_out_hstp[0]
				 , p_ns_result->ui_dce_out_hstp[1]
				 , p_ns_result->ui_dce_out_hstp[2]
				 , p_ns_result->ui_dce_out_hstp[3]
				 , p_ns_result->ui_dce_out_hstp[4]
				 , p_ns_result->ui_dce_out_hstp[5]
				 , p_ns_result->ui_dce_out_hstp[6]
				 , p_ns_result->ui_dce_out_hstp[7]
				 , p_ns_result->ui_dce_out_hstp[8]
				 , p_ns_result->ui_dce_out_hstp[9]
				 , p_ns_result->ui_dce_out_hstp[10]
				 , p_ns_result->ui_dce_out_hstp[11]
				 , p_ns_result->ui_dce_out_hstp[12]
				 , p_ns_result->ui_dce_out_hstp[13]
				 , p_ns_result->ui_dce_out_hstp[14]
				 , p_ns_result->ui_dce_out_hstp[15]);
	DBG_FUNC("ime_in_hstp  %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\r\n"
				 , p_ns_result->ui_ime_in_hstp[0]
				 , p_ns_result->ui_ime_in_hstp[1]
				 , p_ns_result->ui_ime_in_hstp[2]
				 , p_ns_result->ui_ime_in_hstp[3]
				 , p_ns_result->ui_ime_in_hstp[4]
				 , p_ns_result->ui_ime_in_hstp[5]
				 , p_ns_result->ui_ime_in_hstp[6]
				 , p_ns_result->ui_ime_in_hstp[7]
				 , p_ns_result->ui_ime_in_hstp[8]
				 , p_ns_result->ui_ime_in_hstp[9]
				 , p_ns_result->ui_ime_in_hstp[10]
				 , p_ns_result->ui_ime_in_hstp[11]
				 , p_ns_result->ui_ime_in_hstp[12]
				 , p_ns_result->ui_ime_in_hstp[13]
				 , p_ns_result->ui_ime_in_hstp[14]
				 , p_ns_result->ui_ime_in_hstp[15]);
	DBG_UNIT("ime_out_hstp %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\r\n"
				 , p_ns_result->ui_ime_out_hstp[0]
				 , p_ns_result->ui_ime_out_hstp[1]
				 , p_ns_result->ui_ime_out_hstp[2]
				 , p_ns_result->ui_ime_out_hstp[3]
				 , p_ns_result->ui_ime_out_hstp[4]
				 , p_ns_result->ui_ime_out_hstp[5]
				 , p_ns_result->ui_ime_out_hstp[6]
				 , p_ns_result->ui_ime_out_hstp[7]
				 , p_ns_result->ui_ime_out_hstp[8]
				 , p_ns_result->ui_ime_out_hstp[9]
				 , p_ns_result->ui_ime_out_hstp[10]
				 , p_ns_result->ui_ime_out_hstp[11]
				 , p_ns_result->ui_ime_out_hstp[12]
				 , p_ns_result->ui_ime_out_hstp[13]
				 , p_ns_result->ui_ime_out_hstp[14]
				 , p_ns_result->ui_ime_out_hstp[15]);
}

//-----------------------------------Layer2-----------------------------------------------------//
ER dce_reset(BOOL sw_rst_en)
{
	ER erReturn;

	//Check state-machine
	erReturn = dce_check_state_machine(g_dce_engine_state, DCE_OP_RESET);
	if (erReturn != E_OK) {
		dce_unlock_sem();
		return E_SYS;
	}

	dce_set_start(DISABLE);

	if (dce_platform_is_err_clk()) {
		DBG_ERR("failed to get dce clock\n");
		return E_PAR;
	} else {
		dce_detach();
		dce_platform_unprepare_clk();
	}
	dce_platform_prepare_clk();
	dce_attach();

	dce_clear_flag_frame_end();

	dce_clear_int(DCE_INT_ALL);

	dce_set_reset(sw_rst_en);
	dce_set_reset(DISABLE);

	memset((VOID *)&dce_int_stat_cnt, 0, sizeof(dce_int_stat_cnt));

	//Update state-machine
	g_dce_engine_state = DCE_STATE_READY;

	return E_OK;
}
/**
  Open DCE engine

  When ones would like to use DCE engine, this function must be called firstly before calling any function

  @param[in] pDCE_OpenInfo  Engine open object, containing clock selection as well as external ISR call-back function
  -@b Supported clocks: ?Mhz

  @return ER Erroc code to check engine opening status\n
  -@b E_OK  Execute function, success.\n
  -@b E_SYS  Engine open, fail.\n
  */
ER dce_open(DCE_OPENOBJ *p_open_info)
{
	ER er_return;

#if !defined (CONFIG_NVT_SMALL_HDAL)
	snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): function start state %d", hwclock_get_longcounter(), total_log_idx, __func__, g_dce_engine_state);
	log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
	total_log_idx++;
#endif

	if (p_open_info == NULL) {
		DBG_ERR("dce_open() open info is NULL\r\n");
		return E_SYS;
	}

	//Lock semaphore
	er_return = dce_lock_sem();
	if (er_return != E_OK) {
		DBG_ERR("dce_open() Semaphore is locked, can't open DCE\r\n");
		return er_return;
	}

	//Check state-machine
	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_OPEN);
	if (er_return != E_OK) {
		dce_unlock_sem();
		return E_SYS;
	}

#if 0//ndef __KERNEL__
	//Power on module(power domain on)
	pmc_turnonPower(PMC_MODULE_DCE);
#endif

	//Select/Enable PLL clock source
	dce_platform_set_clk_rate(p_open_info->dce_clock_sel);

	//Attach(enable engine clock)
	dce_attach();

	//Enable SRAM
	dce_platform_disable_sram_shutdown();

	//Enable engine interrupt
	dce_platform_int_enable();

#if 0 // remove for builtin
#if DCE_CTRL_SRAM_EN
	dce_set_sram_control(DCE_CTRL_SRAM);
#else
	dce_set_sram_control(DCE_NOT_CTRL_SRAM);
#endif

	//Set DMA burst
	dce_set_dma_burst(DCE_BURST_32W, DCE_BURST_32W);

	//Clear engine interrupt status
	dce_clear_int(DCE_INT_ALL);

	//Clear SW flag
	dce_clear_flag_frame_end();
	dce_clear_flag_frame_start();

	//SW reset
	dce_set_reset(ENABLE);
	dce_set_reset(DISABLE);
#endif


#if defined (__KERNEL__)
	//if (kdrv_builtin_is_fastboot() == DISABLE) {
	if (dce_ctrl_flow_to_do == ENABLE) {
#if DCE_CTRL_SRAM_EN
		dce_set_sram_control(DCE_CTRL_SRAM);
#else
		dce_set_sram_control(DCE_NOT_CTRL_SRAM);
#endif

		//Set DMA burst
		dce_set_dma_burst(DCE_BURST_32W, DCE_BURST_32W);

		//Clear engine interrupt status
		dce_clear_int(DCE_INT_ALL);

		//Clear SW flag
		dce_clear_flag_frame_end();
		dce_clear_flag_frame_start();

		//SW reset
		dce_set_reset(ENABLE);
		dce_set_reset(DISABLE);
	}
#else

#if DCE_CTRL_SRAM_EN
	dce_set_sram_control(DCE_CTRL_SRAM);
#else
	dce_set_sram_control(DCE_NOT_CTRL_SRAM);
#endif

	//Set DMA burst
	dce_set_dma_burst(DCE_BURST_32W, DCE_BURST_32W);

	//Clear engine interrupt status
	dce_clear_int(DCE_INT_ALL);

	//Clear SW flag
	dce_clear_flag_frame_end();
	dce_clear_flag_frame_start();

	//SW reset
	dce_set_reset(ENABLE);
	dce_set_reset(DISABLE);
#endif

	//Hook call-back function
#if 1//def __KERNEL__
	g_pDceCBFunc = p_open_info->FP_DCEISR_CB;
#else
	pfIstCB_DCE = p_open_info->FP_DCEISR_CB;
#endif

	//Update state-machine
	g_dce_engine_state = DCE_STATE_READY;

	return E_OK;
}

/**
  Close DCE engine

  Close DCE engine

  @return ER Erroc code to check engine closing status\n
  -@b E_OK  Execute function, success.\n
  -@b E_SYS Execute function, fail.\n
  */
ER dce_close(void)
{
	ER er_return;

#if !defined (CONFIG_NVT_SMALL_HDAL)
	snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): function start state %d", hwclock_get_longcounter(), total_log_idx, __func__, g_dce_engine_state);
	log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
	total_log_idx++;
#endif

	//Check state-machine
	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CLOSE);
	if (er_return != E_OK) {
		return E_SYS;
	}

	//Check semaphore
	if (dce_get_sem_lock_status() == DCE_SEM_UNLOCKED) {
		DBG_WRN("dce_close() semaphore is already unlocked\r\n");
		return E_SYS;
	}

	//Disable engine interrupt
	dce_platform_int_disable();

	//Disable SRAM
	dce_platform_enable_sram_shutdown();

#if DCE_CTRL_SRAM_EN
	dce_set_sram_control(DCE_CTRL_SRAM);
#else
	dce_set_sram_control(DCE_NOT_CTRL_SRAM);
#endif

	//Detach(disable engine clock)
	dce_detach();

	//Update state-machine
	g_dce_engine_state = DCE_STATE_IDLE;

	//Unlock semaphore
	er_return = dce_unlock_sem();
	if (er_return != E_OK) {
		return E_SYS;
	}

	return E_OK;
}

ER dce_start_linkedlist(void)
{
	ER      er_return;


	g_dce_engine_state = DCE_STATE_PAUSE;

	//Check state-machine
	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_START);
	if (er_return != E_OK) {
		return er_return;
	}

#if (DCE_POWER_SAVING_EN == 1)
	if (fw_power_saving_en) {
		//for power saving
		if (dce_opmode != DCE_OPMODE_SIE_IME) {
			dce_platform_disable_sram_shutdown();
			dce_attach(); //enable engine clock
		}
	}
#endif

	//Clear SW flag
	dce_clear_flag_frame_end();
	dce_clear_flag_frame_start();

	//Load and start
	//dce_set_load(DCE_START_LOAD_TYPE);
	//dce_set_start(ENABLE);
	dce_set_ll_fire();

	//Update state-machine
	g_dce_engine_state = DCE_STATE_RUN;

#if !defined (CONFIG_NVT_SMALL_HDAL)
	snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): set state = %d", hwclock_get_longcounter(), total_log_idx, __func__, g_dce_engine_state);
	log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
	total_log_idx++;
#endif

	return E_OK;
}

ER dce_terminate_linkedlist(void)
{
	ER      er_return;


	//Check state-machine
	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_PAUSE);
	if (er_return != E_OK) {
		return er_return;
	}

	//Stop
	dce_set_ll_terminate();

#if (DCE_POWER_SAVING_EN == 1)
	if (fw_power_saving_en) {
		//for power saving
		if (dce_opmode != DCE_OPMODE_SIE_IME) {
			dce_platform_enable_sram_shutdown();
			dce_detach(); //disable engine clock
		}
	}
#endif

	//Update state-machine
	g_dce_engine_state = DCE_STATE_PAUSE;

	return E_OK;
}


/**
  DCE start

  Clear DCE flag and trigger start

  @param None.

  @return ER DCE start status.
  */
ER dce_start(void)
{
	ER er_return = E_OK;

#if !defined (CONFIG_NVT_SMALL_HDAL)
	snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): function start state %d", hwclock_get_longcounter(), total_log_idx, __func__, g_dce_engine_state);
	log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
	total_log_idx++;
#endif

	//Check state-machine
	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_START);
	if (er_return != E_OK) {
#if !defined (CONFIG_NVT_SMALL_HDAL)
		snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): dce_check_state_machine error", hwclock_get_longcounter(), total_log_idx, __func__);
		log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
		total_log_idx++;
#endif
		return er_return;
	}

#if (DCE_POWER_SAVING_EN == 1)
	if (fw_power_saving_en) {
		//for power saving
		if (dce_opmode != DCE_OPMODE_SIE_IME) {
			dce_platform_disable_sram_shutdown();
			dce_attach(); //enable engine clock
		}
	}
#endif

#if 0  // remove for builtin
	//Clear SW flag
	dce_clear_flag_frame_end();
	dce_clear_flag_frame_start();

	//Update state-machine before load & start for multi-thread processing
	g_dce_engine_state = DCE_STATE_RUN;

	//Load and start
	dce_set_load(DCE_START_LOAD_TYPE);
	dce_set_start(ENABLE);
#endif

#if defined (__KERNEL__)
	//if (kdrv_builtin_is_fastboot() == DISABLE) {
	if (dce_ctrl_flow_to_do == ENABLE) {
		//Clear SW flag
		dce_clear_flag_frame_end();
		dce_clear_flag_frame_start();

		//Update state-machine before load & start for multi-thread processing
		g_dce_engine_state = DCE_STATE_RUN;

		//Load and start
		if (dce_opmode == DCE_OPMODE_SIE_IME) {
			dce_set_load(DCE_DIRECT_START_LOAD_TYPE);
		} else {
			dce_set_load(DCE_START_LOAD_TYPE);
		}
		dce_set_start(ENABLE);
	} else {
		if (dce_opmode == DCE_OPMODE_SIE_IME) {
			//Update state-machine before load & start for multi-thread processing
			g_dce_engine_state = DCE_STATE_RUN;
		} else {
			//Clear SW flag
			dce_clear_flag_frame_end();
			dce_clear_flag_frame_start();

			//Update state-machine before load & start for multi-thread processing
			g_dce_engine_state = DCE_STATE_RUN;

			//Load and start
			dce_set_load(DCE_START_LOAD_TYPE);
			dce_set_start(ENABLE);
		}

	}
#else
	//Clear SW flag
	dce_clear_flag_frame_end();
	dce_clear_flag_frame_start();

	//Update state-machine before load & start for multi-thread processing
	g_dce_engine_state = DCE_STATE_RUN;

	//Load and start
	dce_set_load(DCE_START_LOAD_TYPE);
	dce_set_start(ENABLE);
#endif

	dce_ctrl_flow_to_do = ENABLE;

#if !defined (CONFIG_NVT_SMALL_HDAL)
	snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): set state = %d", hwclock_get_longcounter(), total_log_idx, __func__, g_dce_engine_state);
	log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
	total_log_idx++;
#endif

#if defined (_NVT_EMULATION_)
	dce_end_time_out_status = FALSE;
#endif

	return E_OK;
}

ER dce_start_fend_load(void)
{
	ER er_return = E_OK;

	//Check state-machine
	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_START);
	if (er_return != E_OK) {
		return er_return;
	}

#if (DCE_POWER_SAVING_EN == 1)
	if (fw_power_saving_en) {
		//for power saving
		if (dce_opmode != DCE_OPMODE_SIE_IME) {
			dce_platform_disable_sram_shutdown();
			dce_attach(); //enable engine clock
		}
	}
#endif

	//Clear SW flag
	dce_clear_flag_frame_end();
	dce_clear_flag_frame_start();

	//Load and start
	dce_set_start_fd_load();

	//Update state-machine
	g_dce_engine_state = DCE_STATE_RUN;

#if !defined (CONFIG_NVT_SMALL_HDAL)
	snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): set state = %d", hwclock_get_longcounter(), total_log_idx, __func__, g_dce_engine_state);
	log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
	total_log_idx++;
#endif

#if defined (_NVT_EMULATION_)
	dce_end_time_out_status = FALSE;
#endif

	return E_OK;
}

#if defined (_NVT_EMULATION_)
ER dce_start_without_load(void)
{
	ER er_return = E_OK;

	//Check state-machine
	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_START);
	if (er_return != E_OK) {
		return er_return;
	}

#if (DCE_POWER_SAVING_EN == 1)
	if (fw_power_saving_en) {
		//for power saving
		if (dce_opmode != DCE_OPMODE_SIE_IME) {
			dce_platform_disable_sram_shutdown();
			dce_attach(); //enable engine clock
		}
	}
#endif

	//Clear SW flag
	dce_clear_flag_frame_end();
	dce_clear_flag_frame_start();

	//Load and start
	dce_set_start(ENABLE);

	//Update state-machine
	g_dce_engine_state = DCE_STATE_RUN;

#if !defined (CONFIG_NVT_SMALL_HDAL)
	snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): set state = %d", hwclock_get_longcounter(), total_log_idx, __func__, g_dce_engine_state);
	log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
	total_log_idx++;
#endif

#if defined (_NVT_EMULATION_)
	dce_end_time_out_status = FALSE;
#endif

	return E_OK;
}
#endif

/**
  DCE pause

  Pause DCE operation

  @param None.

  @return ER DCE pause status.
  */
ER dce_pause(void)
{
	ER      er_return;

#if !defined (CONFIG_NVT_SMALL_HDAL)
	snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): function start state %d", hwclock_get_longcounter(), total_log_idx, __func__, g_dce_engine_state);
	log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
	total_log_idx++;
#endif

	//Check state-machine
	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_PAUSE);
	if (er_return != E_OK) {
#if !defined (CONFIG_NVT_SMALL_HDAL)
		snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): dce_check_state_machine error", hwclock_get_longcounter(), total_log_idx, __func__);
		log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
		total_log_idx++;
#endif
		return er_return;
	}

	//Stop
	dce_set_start(DISABLE);

#if (DCE_POWER_SAVING_EN == 1)
	if (fw_power_saving_en) {
		//for power saving
		if (dce_opmode != DCE_OPMODE_SIE_IME) {
			dce_platform_enable_sram_shutdown();
			dce_detach(); //disable engine clock
		}
	}
#endif

	//Update state-machine
	g_dce_engine_state = DCE_STATE_PAUSE;

#if !defined (CONFIG_NVT_SMALL_HDAL)
	snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): set state = %d", hwclock_get_longcounter(), total_log_idx, __func__, g_dce_engine_state);
	log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
	total_log_idx++;
#endif

	return E_OK;
}

VOID dce_set_builtin_flow_disable(VOID)
{
	dce_ctrl_flow_to_do = ENABLE;
}

/**
  DCE set mode

  Initial setup for DCE prv/cap/vdo mode operation

  @param p_mode_info DCE mode information, including all parameters.

  @return ER DCE set mode status.
  */
ER dce_set_mode(DCE_MODE_INFO *p_mode_info)
{
	ER er_return;

	if (p_mode_info == NULL) {
		DBG_ERR("DCE_MODE_INFO is not assigned!\r\n");
		return E_PAR;
	}

#if !defined (CONFIG_NVT_SMALL_HDAL)
	snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): function start state %d", hwclock_get_longcounter(), total_log_idx, __func__, g_dce_engine_state);
	log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
	total_log_idx++;
#endif

	//Check state-machine
	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_SETMODE);
	if (er_return != E_OK) {
		return er_return;
	}

	dce_opmode = p_mode_info->op_param.op_mode;

	//for change parameter functions
#if !defined (_NVT_EMULATION_)
	if (dce_opmode == DCE_OPMODE_SIE_IME) {
		g_dce_load_type = DCE_DIRECT_START_LOAD_TYPE;
	} else {
		g_dce_load_type = DCE_START_LOAD_TYPE;
	}
#else
	g_dce_load_type = DCE_START_LOAD_TYPE;//emulation
#endif

#if defined (__KERNEL__)
	//if (kdrv_builtin_is_fastboot() == DISABLE) {
	if (dce_ctrl_flow_to_do == ENABLE) {
		dce_clear_int(DCE_INT_ALL);

		er_return = dce_change_int_en(p_mode_info->int_en_msk, DCE_SET_SEL);
		if (er_return != E_OK) {
			return er_return;
		}
	} else {
		if (dce_opmode != DCE_OPMODE_SIE_IME) {
			dce_clear_int(DCE_INT_ALL);

			er_return = dce_change_int_en(p_mode_info->int_en_msk, DCE_SET_SEL);
			if (er_return != E_OK) {
				return er_return;
			}
		}
	}
#else
	dce_clear_int(DCE_INT_ALL);

	er_return = dce_change_int_en(p_mode_info->int_en_msk, DCE_SET_SEL);
	if (er_return != E_OK) {
		return er_return;
	}
#endif

	if (dce_ctrl_flow_to_do == DISABLE) {
		goto SKIP_PARAMS;
	}

	if (p_mode_info->func_en_msk & DCE_FUNC_DC && p_mode_info->dc_sel == DCE_DCSEL_2DLUT_ONLY &&
		p_mode_info->op_param.stp_mode != DCE_STPMODE_SST) {
		DBG_ERR("2DLUT can only run in single stripe mode!\r\n");
		return E_PAR;
	}

	dce_set_back_rsv_line(p_mode_info->back_rsv_line);

#if 0
	dce_clear_int(DCE_INT_ALL);

	er_return = dce_change_int_en(p_mode_info->int_en_msk, DCE_SET_SEL);
	if (er_return != E_OK) {
		return er_return;
	}
#endif

	er_return = dce_change_func_en(p_mode_info->func_en_msk, DCE_SET_SEL);
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = dce_change_sram_mode((p_mode_info->sram_sel));
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = dce_change_operation(&(p_mode_info->op_param));
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = dce_change_img_in_size(&(p_mode_info->img_in_size));
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = dce_change_img_out_crop(&(p_mode_info->img_out_crop));
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = dce_change_dram_out_mode((p_mode_info->dram_out_mode));
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = dce_change_dram_single_ch_en(&(p_mode_info->single_out_en));
	if (er_return != E_OK) {
		return er_return;
	}

	if (p_mode_info->func_en_msk & DCE_FUNC_WDR) {
		er_return = dce_change_input_info(&(p_mode_info->input_info));
		if (er_return != E_OK) {
			return er_return;
		}
	}
	DBG_USER("change input info\r\n");

	if (p_mode_info->func_en_msk & DCE_FUNC_WDR_SUBOUT || p_mode_info->func_en_msk & DCE_FUNC_CFA_SUBOUT) {
		er_return = dce_change_output_info(&(p_mode_info->output_info));
		if (er_return != E_OK) {
			return er_return;
		}
	}
	DBG_USER("change output info\r\n");

	er_return = dce_change_wdr_dither(&(p_mode_info->wdr_dither));
	if (er_return != E_OK) {
		return er_return;
	}
	DBG_USER("change wdr dither\r\n");

	if ((p_mode_info->func_en_msk & DCE_FUNC_WDR) ||
		(p_mode_info->func_en_msk & DCE_FUNC_TCURVE) ||
		(p_mode_info->func_en_msk & DCE_FUNC_WDR_SUBOUT) ||
		(p_mode_info->func_en_msk & DCE_FUNC_HISTOGRAM)) {
		er_return = dce_change_wdr((p_mode_info->func_en_msk & DCE_FUNC_WDR), (p_mode_info->func_en_msk & DCE_FUNC_TCURVE), &(p_mode_info->wdr_param));
		if (er_return != E_OK) {
			return er_return;
		}
	}
	DBG_USER("change wdr\r\n");

	if ((p_mode_info->func_en_msk & DCE_FUNC_WDR) || (p_mode_info->func_en_msk & DCE_FUNC_WDR_SUBOUT)) {
		er_return = dce_change_wdr_subout(&(p_mode_info->wdr_subimg_param));
		if (er_return != E_OK) {
			return er_return;
		}
	}
	DBG_USER("change wdr subout\r\n");

	if ((p_mode_info->func_en_msk & DCE_FUNC_HISTOGRAM)) {
		er_return = dce_change_histogram(&(p_mode_info->hist_param));
		if (er_return != E_OK) {
			return er_return;
		}
	}
	DBG_USER("change histogram\r\n");

	dce_set_raw_fmt(p_mode_info->cfa_param.raw_fmt);
	dce_set_cfa_pat(p_mode_info->cfa_param.cfa_pat);
	if ((p_mode_info->func_en_msk & DCE_FUNC_CFA)) {
		er_return = dce_change_cfa_interp(&(p_mode_info->cfa_param));
		if (er_return != E_OK) {
			return er_return;
		}
	}
	DBG_USER("change cfa\r\n");

	if ((p_mode_info->func_en_msk & DCE_FUNC_CFA_SUBOUT)) {
		er_return = dce_change_cfa_subout(&(p_mode_info->cfa_subout_param));
		if (er_return != E_OK) {
			return er_return;
		}
	}
	DBG_USER("change cfa subout\r\n");

	er_return = dce_change_distort_corr_sel(p_mode_info->dc_sel);
	if (er_return != E_OK) {
		return er_return;
	}
	DBG_USER("change dc_sel\r\n");

	er_return = dce_change_img_center(&(p_mode_info->img_cent));
	if (er_return != E_OK) {
		return er_return;
	}
	DBG_USER("change img center\r\n");

	//GDC
	if (p_mode_info->func_en_msk & DCE_FUNC_DC && p_mode_info->dc_sel != DCE_DCSEL_2DLUT_ONLY) {
		DBG_IND("%d %d\r\n", (unsigned int)(p_mode_info->func_en_msk & DCE_FUNC_DC), (unsigned int)p_mode_info->dc_sel);
		er_return = dce_change_gdc_cac_digi_zoom(&(p_mode_info->gdc_cac_dz_param));
		if (er_return != E_OK) {
			return er_return;
		}
		//CAC
		if (p_mode_info->func_en_msk & DCE_FUNC_CAC) {
			er_return = dce_change_gdc_cac_opti_zoom(&(p_mode_info->gdc_cac_oz_param));
			if (er_return != E_OK) {
				return er_return;
			}
		}
	}

	//2DLUT
	if (p_mode_info->func_en_msk & DCE_FUNC_DC && p_mode_info->dc_sel != DCE_DCSEL_GDC_ONLY) {
		er_return = dce_change_2dlut(&(p_mode_info->lut2d_param), &(p_mode_info->img_in_size));
		if (er_return != E_OK) {
			return er_return;
		}
	}
	DBG_USER("change lut2d \r\n");

	if (p_mode_info->func_en_msk & DCE_FUNC_DC) {
		er_return = dce_change_gdc_mode(p_mode_info->gdc_cac_oz_param.gdc_mode);
		if (er_return != E_OK) {
			return er_return;
		}
	}
	DBG_USER("change GDC mode \r\n");

	er_return = dce_change_fov(&(p_mode_info->fov_param));
	if (er_return != E_OK) {
		return er_return;
	}
	DBG_USER("change fov\r\n");


	if (p_mode_info->op_param.stp_mode != DCE_STPMODE_SST) {
		er_return = dce_change_h_stripe(&(p_mode_info->h_stp_param));
		if (er_return != E_OK) {
			return er_return;
		}
	}
	DBG_USER("change h stripe\r\n");

	if (p_mode_info->op_param.op_mode == DCE_OPMODE_DCE_D2D || p_mode_info->op_param.op_mode == DCE_OPMODE_DCE_IME) {
		er_return = dce_change_d2d_in_out(&(p_mode_info->d2d_param), p_mode_info->op_param.op_mode);
		if (er_return != E_OK) {
			return er_return;
		}
	}
	DBG_USER("change d2d in/out \r\n");

#if (DCE_DMA_CACHE_HANDLE == 1)
	dce_flush_cache(p_mode_info);
#else
	DBG_WRN("Cache handling is disabled!\r\n");
#endif

	//Update state-machine
SKIP_PARAMS:  // for fastboot control flow
	g_dce_engine_state = DCE_STATE_PAUSE;

	return E_OK;

}

#if 0
/**
  DCE change all settings

  Change all DCE parameters

  @param p_mode_info DCE mode information, including all parameters.

  @return ER DCE set mode status.
  */
ER dce_change_all(DCE_MODE_INFO *p_mode_info)
{
	ER er_return;

	er_return = dce_change_int_en(p_mode_info->int_en_msk, DCE_SET_SEL);
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = dce_change_func_en(p_mode_info->func_en_msk, DCE_SET_SEL);
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = dce_change_sram_mode((p_mode_info->sram_sel));
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = dce_change_operation(&(p_mode_info->op_param));
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = dce_change_img_in_size(&(p_mode_info->img_in_size));
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = dce_change_img_out_crop(&(p_mode_info->img_out_crop));
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = dce_change_dram_out_mode((p_mode_info->dram_out_mode));
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = dce_change_dram_single_ch_en(&(p_mode_info->single_out_en));
	if (er_return != E_OK) {
		return er_return;
	}

	if (p_mode_info->func_en_msk & DCE_FUNC_WDR) {
		er_return = dce_change_input_info(&(p_mode_info->input_info));
		if (er_return != E_OK) {
			return er_return;
		}
	}

	if (p_mode_info->func_en_msk & DCE_FUNC_WDR_SUBOUT || p_mode_info->func_en_msk & DCE_FUNC_CFA_SUBOUT) {
		er_return = dce_change_output_info(&(p_mode_info->output_info));
		if (er_return != E_OK) {
			return er_return;
		}
	}

	er_return = dce_change_wdr_dither(&(p_mode_info->wdr_dither));
	if (er_return != E_OK) {
		return er_return;
	}

	if ((p_mode_info->func_en_msk & DCE_FUNC_WDR) ||
		(p_mode_info->func_en_msk & DCE_FUNC_TCURVE) ||
		(p_mode_info->func_en_msk & DCE_FUNC_WDR_SUBOUT) ||
		(p_mode_info->func_en_msk & DCE_FUNC_HISTOGRAM)) {
		er_return = dce_change_wdr((p_mode_info->func_en_msk & DCE_FUNC_WDR), (p_mode_info->func_en_msk & DCE_FUNC_TCURVE), &(p_mode_info->wdr_param));
		if (er_return != E_OK) {
			return er_return;
		}
	}

	if ((p_mode_info->func_en_msk & DCE_FUNC_WDR) || (p_mode_info->func_en_msk & DCE_FUNC_WDR_SUBOUT)) {
		er_return = dce_change_wdr_subout(&(p_mode_info->wdr_subimg_param));
		if (er_return != E_OK) {
			return er_return;
		}
	}

	if ((p_mode_info->func_en_msk & DCE_FUNC_HISTOGRAM)) {
		er_return = dce_change_histogram(&(p_mode_info->hist_param));
		if (er_return != E_OK) {
			return er_return;
		}
	}

	dce_set_raw_fmt(p_mode_info->cfa_param.raw_fmt);
	dce_set_cfa_pat(p_mode_info->cfa_param.cfa_pat);
	if ((p_mode_info->func_en_msk & DCE_FUNC_CFA)) {
		er_return = dce_change_cfa_interp(&(p_mode_info->cfa_param));
		if (er_return != E_OK) {
			return er_return;
		}
	}

	if ((p_mode_info->func_en_msk & DCE_FUNC_CFA_SUBOUT)) {
		er_return = dce_change_cfa_subout(&(p_mode_info->cfa_subout_param));
		if (er_return != E_OK) {
			return er_return;
		}
	}

	er_return = dce_change_distort_corr_sel(p_mode_info->dc_sel);
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = dce_change_img_center(&(p_mode_info->img_cent));
	if (er_return != E_OK) {
		return er_return;
	}

	//GDC
	if (p_mode_info->func_en_msk & DCE_FUNC_DC && p_mode_info->dc_sel != DCE_DCSEL_2DLUT_ONLY) {
		DBG_IND("%d %d\r\n", (unsigned int)(p_mode_info->func_en_msk & DCE_FUNC_DC), (unsigned int)p_mode_info->dc_sel);
		er_return = dce_change_gdc_cac_digi_zoom(&(p_mode_info->gdc_cac_dz_param));
		if (er_return != E_OK) {
			return er_return;
		}
		//CAC
		if (p_mode_info->func_en_msk & DCE_FUNC_CAC) {
			er_return = dce_change_gdc_cac_opti_zoom(&(p_mode_info->gdc_cac_oz_param));
			if (er_return != E_OK) {
				return er_return;
			}
		}
	}

	//2DLUT
	if (p_mode_info->func_en_msk & DCE_FUNC_DC && p_mode_info->dc_sel != DCE_DCSEL_GDC_ONLY) {
		er_return = dce_change_2dlut(&(p_mode_info->lut2d_param), &(p_mode_info->img_in_size));
		if (er_return != E_OK) {
			return er_return;
		}
	}

	if (p_mode_info->func_en_msk & DCE_FUNC_DC) {
		er_return = dce_change_gdc_mode(p_mode_info->gdc_cac_oz_param.gdc_mode);
		if (er_return != E_OK) {
			return er_return;
		}
	}

	er_return = dce_change_fov(&(p_mode_info->fov_param));
	if (er_return != E_OK) {
		return er_return;
	}


	if (p_mode_info->op_param.stp_mode != DCE_STPMODE_SST) {
		er_return = dce_change_h_stripe(&(p_mode_info->h_stp_param));
		if (er_return != E_OK) {
			return er_return;
		}
	}

	if (p_mode_info->op_param.op_mode == DCE_OPMODE_DCE_D2D || p_mode_info->op_param.op_mode == DCE_OPMODE_DCE_IME) {
		er_return = dce_change_d2d_in_out(&(p_mode_info->d2d_param), p_mode_info->op_param.op_mode);
		if (er_return != E_OK) {
			return er_return;
		}
	}

#if (DCE_DMA_CACHE_HANDLE == 1)
	dce_flush_cache(p_mode_info);
#endif

	return E_OK;
}
#endif

void dce_flush_cache(DCE_MODE_INFO *p_mode_info)
{
#if (DCE_DMA_CACHE_HANDLE == 1)
	UINT32 addr, img_size;
	UINT32 width;
	UINT32 cfa_subout_byte;

	//initialize DMA out struct
	for (g_dma_och = 0; g_dma_och < DCE_DMA_OUT_NUM; g_dma_och++) {
		dce_dma_out[g_dma_och].enable = FALSE;
		dce_dma_out[g_dma_och].width_eq_lofst = FALSE;
		dce_dma_out[g_dma_och].addr = 0;
		dce_dma_out[g_dma_och].size = 0;
	}

	//main input
	if (p_mode_info->op_param.op_mode == DCE_OPMODE_DCE_D2D || p_mode_info->op_param.op_mode == DCE_OPMODE_DCE_IME) {
		width = p_mode_info->img_in_size.h_size;
		// YUV
		// Y in address
		addr = p_mode_info->d2d_param.y_in_addr;
		img_size = p_mode_info->d2d_param.y_in_lofs * p_mode_info->img_in_size.v_size;
		if (addr != 0) {
			if (width == p_mode_info->d2d_param.y_in_lofs) {
				vos_cpu_dcache_sync_vb((VOS_ADDR)addr, img_size, VOS_DMA_TO_DEVICE);
			} else {
				vos_cpu_dcache_sync_vb((VOS_ADDR)addr, img_size, VOS_DMA_BIDIRECTIONAL);
			}
		}
		DBG_IND(DBG_COLOR_CYAN"flush Y in\r\n"DBG_COLOR_END);

		// UV in address
		switch (p_mode_info->d2d_param.d2d_fmt) {
		case DCE_D2D_YUV422IN_YUV422OUT:
			addr = p_mode_info->d2d_param.uv_in_addr;
			img_size = p_mode_info->d2d_param.uv_in_lofs * p_mode_info->img_in_size.v_size;
			if (addr != 0) {
				if (width == p_mode_info->d2d_param.uv_in_lofs) {
					vos_cpu_dcache_sync_vb((VOS_ADDR)addr, img_size, VOS_DMA_TO_DEVICE);
				} else {
					vos_cpu_dcache_sync_vb((VOS_ADDR)addr, img_size, VOS_DMA_BIDIRECTIONAL);
				}
			}
			break;
		case DCE_D2D_YUV420IN_YUV420OUT:
			addr = p_mode_info->d2d_param.uv_in_addr;
			img_size = p_mode_info->d2d_param.uv_in_lofs * (p_mode_info->img_in_size.v_size >> 1);
			if (addr != 0) {
				if (width == p_mode_info->d2d_param.uv_in_lofs) {
					vos_cpu_dcache_sync_vb((VOS_ADDR)addr, img_size, VOS_DMA_TO_DEVICE);
				} else {
					vos_cpu_dcache_sync_vb((VOS_ADDR)addr, img_size, VOS_DMA_BIDIRECTIONAL);
				}
			}
			break;
		case DCE_D2D_BAYERIN_YUV422OUT:
		default:
			DBG_FUNC("UV size = 0\r\n");
			break;
		}
		DBG_IND(DBG_COLOR_CYAN"flush UV in\r\n"DBG_COLOR_END);
	}

	//WDR sub-image in/out
	width = p_mode_info->wdr_subimg_param.wdr_subimg_size.h_size * 2;
	if (p_mode_info->input_info.wdr_in_en) {
		addr = p_mode_info->input_info.wdr_subin_addr;
		img_size = p_mode_info->input_info.wdr_subin_lofs * p_mode_info->wdr_subimg_param.wdr_subimg_size.v_size;
		if (addr != 0) {
			if (width == p_mode_info->input_info.wdr_subin_lofs) {
				vos_cpu_dcache_sync((VOS_ADDR)addr, img_size, VOS_DMA_TO_DEVICE);
			} else {
				vos_cpu_dcache_sync((VOS_ADDR)addr, img_size, VOS_DMA_BIDIRECTIONAL);
			}
		}
		DBG_IND(DBG_COLOR_CYAN"flush WDR in\r\n"DBG_COLOR_END);
	}

	if (p_mode_info->output_info.wdr_out_en) {
		addr = p_mode_info->output_info.wdr_subout_addr;
		img_size = p_mode_info->output_info.wdr_subout_lofs * p_mode_info->wdr_subimg_param.wdr_subimg_size.v_size;
		if (addr != 0) {
			if (width == p_mode_info->output_info.wdr_subout_lofs) {
				dce_dma_out[0].width_eq_lofst = TRUE;
				vos_cpu_dcache_sync((VOS_ADDR)addr, img_size, VOS_DMA_FROM_DEVICE);
			} else {
				dce_dma_out[0].width_eq_lofst = FALSE;
				vos_cpu_dcache_sync((VOS_ADDR)addr, img_size, VOS_DMA_BIDIRECTIONAL);
			}
		}
		DBG_IND(DBG_COLOR_CYAN"flush WDR out\r\n"DBG_COLOR_END);
		dce_dma_out[0].enable = TRUE;
		dce_dma_out[0].addr = addr;
		dce_dma_out[0].size = img_size;
	}

	//CFA sub-image out
	if (p_mode_info->output_info.cfa_out_en) {
		DBG_IND(DBG_COLOR_CYAN"flush CFA out\r\n"DBG_COLOR_END);
		addr = p_mode_info->output_info.cfa_subout_addr;
		img_size = p_mode_info->output_info.cfa_subout_lofs * (p_mode_info->img_in_size.v_size >> 1);
		cfa_subout_byte = (p_mode_info->cfa_subout_param.subout_byte == 0) ? 1 : 2;
		width = (p_mode_info->img_in_size.h_size >> 1) * cfa_subout_byte;
#if defined (_NVT_EMULATION_) // disabled in project for efficiency
		if (addr != 0) {
			if (width == p_mode_info->output_info.cfa_subout_lofs) {
				dce_dma_out[1].width_eq_lofst = TRUE;
				vos_cpu_dcache_sync((VOS_ADDR)addr, img_size, VOS_DMA_FROM_DEVICE);
			} else {
				dce_dma_out[1].width_eq_lofst = FALSE;
				vos_cpu_dcache_sync((VOS_ADDR)addr, img_size, VOS_DMA_BIDIRECTIONAL);
			}
		}
#else
		if (addr != 0) {
			if (width == p_mode_info->output_info.cfa_subout_lofs) {
				dce_dma_out[1].width_eq_lofst = TRUE;
				vos_cpu_dcache_sync_vb((VOS_ADDR)addr, img_size, VOS_DMA_FROM_DEVICE);
			} else {
				dce_dma_out[1].width_eq_lofst = FALSE;
				vos_cpu_dcache_sync_vb((VOS_ADDR)addr, img_size, VOS_DMA_BIDIRECTIONAL);
			}
		}
#endif
		DBG_IND(DBG_COLOR_CYAN"flush CFA out\r\n"DBG_COLOR_END);
		dce_dma_out[1].enable = TRUE;
		dce_dma_out[1].addr = addr;
		dce_dma_out[1].size = img_size;
	}

	//main output
	if (p_mode_info->op_param.op_mode == DCE_OPMODE_DCE_D2D) {

		width = p_mode_info->img_in_size.h_size;

		// Y out address
		addr = p_mode_info->d2d_param.y_out_addr;
		img_size = p_mode_info->d2d_param.y_out_lofs * p_mode_info->img_in_size.v_size;
		if (addr != 0) {
			if (width == p_mode_info->d2d_param.y_out_lofs) {
				dce_dma_out[2].width_eq_lofst = TRUE;
				vos_cpu_dcache_sync_vb((VOS_ADDR)addr, img_size, VOS_DMA_FROM_DEVICE);
			} else {
				dce_dma_out[2].width_eq_lofst = FALSE;
				vos_cpu_dcache_sync_vb((VOS_ADDR)addr, img_size, VOS_DMA_BIDIRECTIONAL);
			}
		}
		DBG_IND(DBG_COLOR_CYAN"flush Y out\r\n"DBG_COLOR_END);
		dce_dma_out[2].enable = TRUE;
		dce_dma_out[2].addr = addr;
		dce_dma_out[2].size = img_size;

		// UV out address
		addr = p_mode_info->d2d_param.uv_out_addr;
		switch (p_mode_info->d2d_param.d2d_fmt) {
		case DCE_D2D_BAYERIN_YUV422OUT:
		case DCE_D2D_YUV422IN_YUV422OUT:
		default:
			img_size = p_mode_info->d2d_param.uv_out_lofs * p_mode_info->img_in_size.v_size;
			if (addr != 0) {
				if (width == p_mode_info->d2d_param.uv_out_lofs) {
					dce_dma_out[3].width_eq_lofst = TRUE;
					vos_cpu_dcache_sync_vb((VOS_ADDR)addr, img_size, VOS_DMA_FROM_DEVICE);
				} else {
					dce_dma_out[3].width_eq_lofst = FALSE;
					vos_cpu_dcache_sync_vb((VOS_ADDR)addr, img_size, VOS_DMA_BIDIRECTIONAL);
				}
			}
			break;
		case DCE_D2D_YUV420IN_YUV420OUT:
			img_size = p_mode_info->d2d_param.uv_out_lofs * (p_mode_info->img_in_size.v_size >> 1);
			if (addr != 0) {
				if (width == p_mode_info->d2d_param.uv_out_lofs) {
					dce_dma_out[3].width_eq_lofst = TRUE;
					vos_cpu_dcache_sync_vb((VOS_ADDR)addr, img_size, VOS_DMA_FROM_DEVICE);
				} else {
					dce_dma_out[3].width_eq_lofst = FALSE;
					vos_cpu_dcache_sync_vb((VOS_ADDR)addr, img_size, VOS_DMA_BIDIRECTIONAL);
				}
			}
			break;
		}
		DBG_IND(DBG_COLOR_CYAN"flush UV out\r\n"DBG_COLOR_END);
		dce_dma_out[3].enable = TRUE;
		dce_dma_out[3].addr = addr;
		dce_dma_out[3].size = img_size;
	}
#endif
}


/**
  DCE change interrupt

  DCE interrupt configuration.

  @param int_sel Interrupt selection.
  @param setting_sel Enable/disable/set selection.

  @return ER DCE change interrupt status.
  */
ER dce_change_int_en(UINT32 int_sel, DCE_SETTING_SEL setting_sel)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}

	switch (setting_sel) {
	case DCE_ENABLE_SEL:
		dce_enable_int(int_sel);
		break;
	case DCE_DISABLE_SEL:
		dce_disable_int(int_sel);
		break;
	case DCE_SET_SEL:
		dce_set_int_en(int_sel);
		break;
	default:
		DBG_ERR("dce_changeInterrupt() setting select error!\r\n");
		break;
	}

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change function enable

  DCE function enable configuration.

  @param func_sel Function selection.
  @param setting_sel Enable/disable/set selection.

  @return ER DCE change function enable status.
  */
ER dce_change_func_en(UINT32 func_sel, DCE_SETTING_SEL setting_sel)
{
	ER er_return;
	UINT32 func_en_msk;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}

	switch (setting_sel) {
	case DCE_ENABLE_SEL:
		dce_enable_function(func_sel);
		break;
	case DCE_DISABLE_SEL:
		dce_disable_function(func_sel);
		break;
	case DCE_SET_SEL:
		dce_set_function(func_sel);
		break;
	default:
		DBG_ERR("dce_change_func_en() setting select error!\r\n");
		break;
	}

	func_en_msk = dce_get_func_en();
	if (((func_en_msk & DCE_FUNC_CROP)) &&
		((func_en_msk & DCE_FUNC_WDR_SUBOUT) || (func_en_msk & DCE_FUNC_CFA_SUBOUT) || (func_en_msk & DCE_FUNC_HISTOGRAM))) {
		DBG_ERR("dce_set_mode() CFA subout & wdr subout & histogram cannot be enabled when DCE crop enabled!\r\n");
		return E_NOSPT;
	}


	//if ((func_en_msk & DCE_FUNC_CROP) && (!(func_en_msk & DCE_FUNC_DC)) && (!(func_en_msk & DCE_FUNC_CFA))) {
	//  DBG_ERR("dce_change_func_en() Crop cannot be enabled when DC & CFA are disabled!\r\n");
	//return E_NOSPT;
	//}

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change operation mode

  DCE operation mode configuration.

  @param p_in_param operation mode parameters.

  @return ER DCE change operation status.
  */
ER dce_change_operation(DCE_OP_PARAM *p_in_param)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	dce_set_op_mode(p_in_param->op_mode);
	dce_set_stp_mode(p_in_param->stp_mode);

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change image input size

  DCE image size configuration.

  @param p_in_param Image input size parameters.

  @return ER DCE change image input size status.
  */
ER dce_change_img_in_size(DCE_IMG_SIZE *p_in_param)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	dce_set_img_size(p_in_param);

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change image output crop

  DCE output crop configuration.

  @param p_in_param Image output crop parameters.

  @return ER DCE change image output crop status.
  */
ER dce_change_img_out_crop(DCE_OUT_CROP *p_in_param)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	dce_set_img_crop(p_in_param);

	dce_set_load(g_dce_load_type);

	return E_OK;
}


/**
  DCE change dram input infomation

  DCE dram input infomation configuration.

  @param p_in_param Image dram input infomation.

  @return ER DCE change dram input infomation status.
  */
ER dce_change_input_info(DCE_INPUT_INFO *p_in_param)
{
	ER er_return;
	UINT32 phy_addr;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	if (p_in_param->wdr_in_en) {

#if (DCE_DMA_CACHE_HANDLE == 1)
		phy_addr = dce_platform_va2pa(p_in_param->wdr_subin_addr);
#else
		phy_addr = p_in_param->wdr_subin_addr;
#endif

		if (phy_addr != 0) {
			dce_set_wdr_subin_addr(phy_addr);
		}
		dce_set_wdr_subin_lofs(p_in_param->wdr_subin_lofs);

		dce_set_load(g_dce_load_type);
	}

	return E_OK;
}

/**
  DCE change dram output infomation

  DCE dram output infomation configuration.

  @param p_in_param Image dram output infomation.

  @return ER DCE change dram output infomation status.
  */
ER dce_change_output_info(DCE_OUTPUT_INFO *p_in_param)
{
	ER er_return;
	UINT32 phy_addr0, phy_addr1;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	if (p_in_param->wdr_out_en) {

#if (DCE_DMA_CACHE_HANDLE == 1)
		phy_addr0 = dce_platform_va2pa(p_in_param->wdr_subout_addr);
#else
		phy_addr0 = p_in_param->wdr_subout_addr;
#endif

		if (phy_addr0 != 0) {
			dce_set_wdr_subout_addr(phy_addr0);
		}
		dce_set_wdr_subout_lofs(p_in_param->wdr_subout_lofs);
	}

	if (p_in_param->cfa_out_en) {
#if (DCE_DMA_CACHE_HANDLE == 1)
		phy_addr1 = dce_platform_va2pa(p_in_param->cfa_subout_addr);
#else
		phy_addr1 = p_in_param->cfa_subout_addr;
#endif

		if (phy_addr1 != 0) {
			dce_set_cfa_subout_addr(phy_addr1);
		}
		dce_set_cfa_subout_lofs(p_in_param->cfa_subout_lofs);
	}

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change Dram out mode Parameters

  DCE Dram out mode configuration.

  @param p_in_param Dram out mode Parameters.

  @return ER DCE change Dram out mode status.
  */
ER dce_change_dram_out_mode(DCE_DRAM_OUTPUT_MODE outmode)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}

	dce_set_dram_out_mode(outmode);

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change Dram single out channel enable Parameters

  DCE Dram single out channel enable configuration.

  @param p_in_param Dram single out channel enable Parameters.

  @return ER DCE change Dram single out channel enable status.
  */
ER dce_change_dram_single_ch_en(DCE_DRAM_SINGLE_OUT_EN *ch_en)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (ch_en == NULL) {
		return E_PAR;
	}


	dce_set_dram_single_out_en(ch_en);

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change WDR/Tone curve Parameters

  DCE WDR/Tone curve configuration.

  @param p_in_param WDR/Tone curve Parameters.

  @return ER DCE change WDR/Tone curve status.
  */
ER dce_change_wdr(BOOL wdr_en, BOOL tonecurve_en, DCE_WDR_PARAM *p_in_param)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	//input blending
	dce_set_wdr_input_bld(&p_in_param->wdr_input_bld);

	if (tonecurve_en) {
		//tone curve
		if (p_in_param->wdr_tonecurve.p_lut_idx != NULL) {
			dce_set_wdr_tcurve_idx(p_in_param->wdr_tonecurve.p_lut_idx);
		}
		if (p_in_param->wdr_tonecurve.p_lut_split != NULL) {
			dce_set_wdr_tcurve_split(p_in_param->wdr_tonecurve.p_lut_split);
		}
		if (p_in_param->wdr_tonecurve.p_lut_val != NULL) {
			dce_set_wdr_tcurve_val(p_in_param->wdr_tonecurve.p_lut_val);
		}
	}

	if (wdr_en) {

		//filter coefficent
		if (p_in_param->p_ftrcoef != NULL) {
			dce_set_wdr_subimg_filtcoeff(p_in_param->p_ftrcoef);
		}

		//strength
		dce_set_wdr_strength(&p_in_param->wdr_strength);

		//gain control
		dce_set_wdr_gain_ctrl(&p_in_param->wdr_gainctrl);

		//output blending
		dce_set_wdr_outbld_en(p_in_param->wdr_outbld.outbld_en);
		if (p_in_param->wdr_outbld.outbld_lut.p_lut_idx != NULL) {
			dce_set_wdr_outbld_idx(p_in_param->wdr_outbld.outbld_lut.p_lut_idx);
		}
		if (p_in_param->wdr_outbld.outbld_lut.p_lut_split != NULL) {
			dce_set_wdr_outbld_split(p_in_param->wdr_outbld.outbld_lut.p_lut_split);
		}
		if (p_in_param->wdr_outbld.outbld_lut.p_lut_val != NULL) {
			dce_set_wdr_outbld_val(p_in_param->wdr_outbld.outbld_lut.p_lut_val);
		}
	}

	//saturation reduction
	dce_set_wdr_sat_reduct(&p_in_param->wdr_sat_reduct);

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change WDR subout Parameters

  DCE WDR subout configuration.

  @param p_in_param WDR subout parameters.

  @return ER DCE change WDR subout status.
  */
ER dce_change_wdr_subout(DCE_WDR_SUBOUT_PARAM *p_in_param)
{
	ER er_return;
	UINT32 subout_sizeh, subout_sizev, img_width, img_height;
	DCE_IMG_SIZE blk_size = {0};
	DCE_HV_FACTOR blk_cent_fact = {0};
	DCE_HV_FACTOR subin_scalfact = {0};

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	//size
	dce_set_wdr_subimg_size(p_in_param->wdr_subimg_size);

	if (p_in_param->subimg_param_mode == DCE_PARAM_AUTO_MODE) {
		img_width = p_in_param->input_size.h_size;
		img_height = p_in_param->input_size.v_size;
		subout_sizeh = p_in_param->wdr_subimg_size.h_size;
		subout_sizev = p_in_param->wdr_subimg_size.v_size;

		if (subout_sizeh == 0) {
			subout_sizeh = 16;
			DBG_ERR("subimg size h cannot be 0, modify to 16\r\n");
		}
		if (subout_sizev == 0) {
			subout_sizev = 9;
			DBG_ERR("subimg size v cannot be 0, modify to 9\r\n");
		}

		blk_size.h_size = (((img_width - 1) / (subout_sizeh)));
		blk_size.v_size = (((img_height - 1) / (subout_sizev)));
		blk_cent_fact.factor_h = ((((img_width - 1) << 12) / (subout_sizeh - 1)));
		blk_cent_fact.factor_v = ((((img_height - 1) << 12) / (subout_sizev - 1)));

		DBG_USER("img size = %d x %d \r\n", (unsigned int)img_width, (unsigned int)img_height);
		DBG_USER("subimg size = %d x %d \r\n", (unsigned int)subout_sizeh, (unsigned int)subout_sizev);
		DBG_USER("blk_size (%d x %d) \r\n", (unsigned int)blk_size.h_size, (unsigned int)blk_size.v_size);
		DBG_USER("blk_cent_fact (%d x %d) \r\n", (unsigned int)blk_cent_fact.factor_h, (unsigned int)blk_cent_fact.factor_v);

		dce_set_wdr_subout(blk_size, blk_cent_fact);
	} else {

		blk_size.h_size = p_in_param->subout_blksize.h_size;
		blk_size.v_size = p_in_param->subout_blksize.v_size;
		blk_cent_fact.factor_h = p_in_param->subout_blk_centfact.factor_h;
		blk_cent_fact.factor_v = p_in_param->subout_blk_centfact.factor_v;
		dce_set_wdr_subout(blk_size, blk_cent_fact);
	}

	//scaling factor
	if (p_in_param->subimg_param_mode == DCE_PARAM_AUTO_MODE) {
		img_width = p_in_param->input_size.h_size;
		img_height = p_in_param->input_size.v_size;
		subout_sizeh = p_in_param->wdr_subimg_size.h_size;
		subout_sizev = p_in_param->wdr_subimg_size.v_size;
		if (img_width == 0) {
			img_width = 1920;
			DBG_ERR("img_width cannot be 0, modify to 1920\r\n");
		}
		if (img_height == 0) {
			img_height = 1080;
			DBG_ERR("img_height cannot be 0, modify to 1080\r\n");
		}
		subin_scalfact.factor_h = ((((subout_sizeh - 1) << 16) / (img_width - 1)));
		subin_scalfact.factor_v = ((((subout_sizev - 1) << 16) / (img_height - 1)));
		DBG_USER("scaling (%d x %d) \r\n", (unsigned int)subin_scalfact.factor_h, (unsigned int)subin_scalfact.factor_v);
		dce_set_wdr_scaling_fact(subin_scalfact);
	} else {
		subin_scalfact.factor_h = p_in_param->subin_scaling_factor.factor_h;
		subin_scalfact.factor_v = p_in_param->subin_scaling_factor.factor_v;
		dce_set_wdr_scaling_fact(subin_scalfact);
	}

	dce_set_load(g_dce_load_type);

	return E_OK;
}


/**
  DCE change WDR histogram Parameters

  DCE WDR histogram configuration.

  @param p_in_param WDR histogram parameters.

  @return ER DCE change WDR histogram status.
  */
ER dce_change_histogram(DCE_HIST_PARAM *p_in_param)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	dce_set_hist_sel(p_in_param->hist_sel);
	dce_set_hist_step(p_in_param->hist_step_h, p_in_param->hist_step_v);

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change WDR dithering Parameters

  DCE WDR dithering configuration.

  @param p_in_param WDR dithering parameters.

  @return ER DCE change WDR dithering status.
  */
ER dce_change_wdr_dither(DCE_WDR_DITHER *p_in_param)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	dce_set_wdr_dither(p_in_param);

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change CFA interpolation

  DCE CFA interpolation configuration.

  @param p_in_param CFA interpolation parameters.

  @return ER DCE change CFA interpolation status.
  */
ER dce_change_cfa_interp(DCE_CFA_PARAM *p_in_param)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	if (&(p_in_param->cfa_interp) != NULL) {
		dce_set_cfa_interp(&(p_in_param->cfa_interp));
		dce_set_cfa_freq(p_in_param->cfa_interp.freq_th, p_in_param->cfa_interp.p_freq_lut);
		dce_set_cfa_luma_wt(p_in_param->cfa_interp.p_luma_wt);
	}
	if (&(p_in_param->cfa_corr) != NULL) {
		dce_set_cfa_corr(&(p_in_param->cfa_corr));
	}
	if (&(p_in_param->cfa_fcs) != NULL) {
		dce_set_cfa_fcs(&(p_in_param->cfa_fcs));
	}
	if (&(p_in_param->cfa_ir_hfc) != NULL) {
		dce_set_cfa_ir_hfc(&(p_in_param->cfa_ir_hfc));
	}
	if (&(p_in_param->cfa_ir_sub) != NULL) {
		dce_set_ir_sub(&(p_in_param->cfa_ir_sub));
	}
	if (&(p_in_param->cfa_pink_reduc) != NULL) {
		dce_set_pink_reduct(&(p_in_param->cfa_pink_reduc));
	}
	dce_set_color_gain(&(p_in_param->cfa_cgain));
	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change CFA subout

  DCE CFA subout  configuration.

  @param p_in_param CFA subout  parameters.

  @return ER DCE change CFA subout status.
  */
ER dce_change_cfa_subout(DCE_CFA_SUBOUT_PARAM *p_in_param)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	dce_set_cfa_subout_sel(p_in_param);

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change distortion correction selection

  DCE distortion selection configuration.

  @param InParam distortion selection parameters.

  @return ER DCE change distortion correction status.
  */
ER dce_change_distort_corr_sel(DCE_DC_SEL dcsel)
{
	ER erReturn;

	erReturn = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (erReturn != E_OK) {
		return erReturn;
	}

	dce_set_distort_corr_sel(dcsel);

	dce_set_load(g_dce_load_type);

	return E_OK;
}


/**
  DCE change GDC/CAC image center

  DCE image center configuration.

  @param p_in_param Image center parameters.

  @return ER DCE change image center status.
  */
ER dce_change_img_center(DCE_IMG_CENT *p_in_param)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	dce_set_img_center(p_in_param);

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change GDC/CAC digital zoom parameters

  DCE digital zoom GDC/CAC configuration.

  @param p_in_param digital zoom GDC/CAC parameters.

  @return ER DCE change digital zoom GDC/CAC status.
  */
ER dce_change_gdc_cac_digi_zoom(DCE_GDC_CAC_DZOOM_PARAM *p_in_param)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	if (&(p_in_param->img_ratio) != NULL) {
		dce_set_img_ratio(&(p_in_param->img_ratio));
	}
	if (&(p_in_param->distance_norm) != NULL) {
		dce_set_dist_norm(&(p_in_param->distance_norm));
	}
	if (p_in_param->p_g_geo_lut != NULL) {
		dce_set_g_geo_lut(p_in_param->p_g_geo_lut);
	}

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change GDC/CAC optical zoom parameters

  DCE optical zoom GDC/CAC configuration.

  @param p_in_param optical zoom GDC/CAC parameters.

  @return ER DCE change optical zoom GDC/CAC status.
  */
ER dce_change_gdc_cac_opti_zoom(DCE_GDC_CAC_OZOOM_PARAM *p_in_param)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	//dce_set_gdc_mode(p_in_param->gdc_mode);
	dce_set_cac_sel(p_in_param->cac_sel);
	if (&(p_in_param->cac_gain) != NULL) {
		dce_set_cac_gain(&(p_in_param->cac_gain));
	}
	if (p_in_param->p_r_cac_lut != NULL) {
		dce_set_r_cac_lut(p_in_param->p_r_cac_lut);
	}
	if (p_in_param->p_b_cac_lut != NULL) {
		dce_set_b_cac_lut(p_in_param->p_b_cac_lut);
	}

	dce_set_load(g_dce_load_type);

	return E_OK;
}


/**
  DCE change GDC mode parameters

  DCE GDC mode configuration.

  @param gdc_mode GDC mode parameters.

  @return ER DCE change GDC mode status.
  */
ER dce_change_gdc_mode(DCE_GDC_LUT_MODE gdc_mode)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}

	dce_set_gdc_mode(gdc_mode);

	dce_set_load(g_dce_load_type);

	return E_OK;
}


/**
  DCE change 2D grid lut parameters

  DCE 2D grid lut parameters configuration.

  @param p_in_param 2D grid lut parameters.

  @return ER DCE change 2D grid lut parameters status.
  */
ER dce_change_2dlut(DCE_2DLUT_PARAM *p_in_param, DCE_IMG_SIZE *in_img_sz)
{
	ER er_return;
	UINT32 addr, phy_addr;
#if !defined (_NVT_EMULATION_)
	DCE_2DLUT_SCALE lut2d_scale_factor = {0};
#endif
#if (DCE_DMA_CACHE_HANDLE == 1)
	UINT32 lut_tap_num = 65;
	UINT32 size;
#endif

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	//Cache flush
	addr = p_in_param->lut2d_addr;
#if (DCE_DMA_CACHE_HANDLE == 1)
	switch (p_in_param->lut2d_num_sel) {
	case DCE_2DLUT_65_65_GRID:
		lut_tap_num = 65;
		break;
	case DCE_2DLUT_33_33_GRID:
		lut_tap_num = 33;
		break;
	case DCE_2DLUT_17_17_GRID:
		lut_tap_num = 17;
		break;
	case DCE_2DLUT_9_9_GRID:
		lut_tap_num = 9;
		break;
	default:
		lut_tap_num = 65;
		break;
	}
	size = lut_tap_num * lut_tap_num * 4;
	if (addr != 0) {
		DBG_IND(DBG_COLOR_CYAN"flush 2DLUT in\r\n"DBG_COLOR_END);
		vos_cpu_dcache_sync(addr, KDRV_DCE_CEIL_64(size), VOS_DMA_BIDIRECTIONAL);
	}
#endif
	phy_addr = dce_platform_va2pa(addr);

	if (phy_addr != 0) {
		dce_set_2dlut_in_addr(phy_addr);
	} else {
		return E_PAR;
	}
	if (&(p_in_param->lut2d_xy_ofs) != NULL) {
		dce_set_2dlut_ofs(&(p_in_param->lut2d_xy_ofs));
	}
	dce_set_2dlut_num(p_in_param->lut2d_num_sel);
	dce_set_2dlut_ymin_auto_en(p_in_param->ymin_auto_en);

#if defined (_NVT_EMULATION_)
	dce_set_2dlut_scale_fact(&p_in_param->lut2d_factor);
#else
	lut2d_scale_factor.h_scale_fact = ((lut_tap_num - 1) << LUT2D_FRAC_BIT_NUM) / (in_img_sz->h_size - 1);
	lut2d_scale_factor.v_scale_fact = ((lut_tap_num - 1) << LUT2D_FRAC_BIT_NUM) / (in_img_sz->v_size - 1);
	dce_set_2dlut_scale_fact(&lut2d_scale_factor);
#endif

	dce_set_load(g_dce_load_type);

	return E_OK;
}


/**
  DCE change FOV parameters

  DCE FOV parameters configuration.

  @param p_in_param FOV parameters.

  @return ER DCE change FOV parameters status.
  */
ER dce_change_fov(DCE_FOV_PARAM *p_in_param)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	dce_set_fov_gain(p_in_param->fov_gain);
	dce_set_fov_bound_sel(p_in_param->fov_sel);
	if (&(p_in_param->fov_rgb_val) != NULL) {
		dce_set_fov_bound_rgb(&(p_in_param->fov_rgb_val));
	}

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change H stripe parameters

  DCE H stripe parameters configuration.

  @param p_in_param H stripe parameters.

  @return ER DCE change H stripe parameters status.
  */
ER dce_change_h_stripe(DCE_HSTP_PARAM *p_in_param)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

	if (p_in_param->p_h_stripe != NULL) {
		dce_set_h_stp(p_in_param->p_h_stripe);
	}
	dce_set_hstripe_inc_dec(&(p_in_param->h_stp_inc_dec));
	dce_set_hstripe_in_start(p_in_param->h_stp_in_start);
	dce_set_ime_hstpout_ovlp_sel(p_in_param->ime_out_hstp_ovlap);
	if (p_in_param->ime_out_hstp_ovlap == DCE_HSTP_IMEOLAP_USER) {
		dce_set_ime_hstpout_ovlp_val(p_in_param->ime_ovlap_user_val);
	}
	dce_set_ipe_hstpout_ovlp_sel(p_in_param->ipe_out_hstp_ovlap);

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change D2D I/O parameters

  DCE D2D I/O parameters configuration.

  @param p_in_param D2D I/O parameters.

  @return ER DCE change D2D I/O parameters status.
  */
ER dce_change_d2d_in_out(DCE_D2D_PARAM *p_in_param, DCE_OPMODE op_mode)
{
	ER er_return;
	UINT32 phy_addr_in0, phy_addr_in1, phy_addr_out0, phy_addr_out1;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}
	if (p_in_param == NULL) {
		return E_PAR;
	}

#if (DCE_DMA_CACHE_HANDLE == 1)
	phy_addr_in0 = dce_platform_va2pa(p_in_param->y_in_addr);
	phy_addr_in1 = dce_platform_va2pa(p_in_param->uv_in_addr);
	if (op_mode == DCE_OPMODE_DCE_D2D) {
		phy_addr_out0 = dce_platform_va2pa(p_in_param->y_out_addr);
		phy_addr_out1 = dce_platform_va2pa(p_in_param->uv_out_addr);
	}
#else
	phy_addr_in0 = p_in_param->y_in_addr;
	phy_addr_in1 = p_in_param->uv_in_addr;
	if (op_mode == DCE_OPMODE_DCE_D2D) {
		phy_addr_out0 = p_in_param->y_out_addr;
		phy_addr_out1 = p_in_param->uv_out_addr;
	}
#endif

	dce_set_d2d_rand(p_in_param->d2d_rand_en, p_in_param->d2d_rand_rest);
	dce_set_d2d_fmt(p_in_param->d2d_fmt);
	dce_set_yuv2rgb_fmt(p_in_param->yuv2rgb_fmt);
	if (phy_addr_in0 != 0) {
		dce_set_y_in_addr(phy_addr_in0);
	}
	dce_set_y_in_lofs(p_in_param->y_in_lofs);
	if (phy_addr_in1 != 0) {
		dce_set_uv_in_addr(phy_addr_in1);
	}
	dce_set_uv_in_lofs(p_in_param->uv_in_lofs);

	if (op_mode == DCE_OPMODE_DCE_D2D) {
		if (phy_addr_out0 != 0) {
			dce_set_y_out_addr(phy_addr_out0);
		}
		dce_set_y_out_lofs(p_in_param->y_out_lofs);
		if (phy_addr_out1 != 0) {
			dce_set_uv_out_addr(phy_addr_out1);
		}
		dce_set_uv_out_lofs(p_in_param->uv_out_lofs);
	}

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  DCE change D2D I/O parameters

  DCE D2D I/O parameters configuration.

  @param p_in_param D2D I/O parameters.

  @return ER DCE change D2D I/O parameters status.
  */
ER dce_change_sram_mode(DCE_SRAM_SEL ui_sram_mode)
{
	ER er_return;

	er_return = dce_check_state_machine(g_dce_engine_state, DCE_OP_CHGPARAM);
	if (er_return != E_OK) {
		return er_return;
	}

	dce_set_sram_mode(ui_sram_mode);

	dce_set_load(g_dce_load_type);

	return E_OK;
}

/**
  Get DCE interrupt enable bits

  Get DCE interrupt enable bits

  @param None

  @return DCE interrupt enable bits
  */
UINT32 dce_get_int_en(void)
{
	T_DCE_INTERRUPT_ENABLE_REGISTER local_reg;
	UINT32 int_sel = 0;

	local_reg.reg = DCE_GETREG(DCE_INTERRUPT_ENABLE_REGISTER_OFS);
	int_sel = local_reg.reg & DCE_INT_ALL;

	return int_sel;
}

/**
  Get DCE function enable bits

  Get DCE function enable bits

  @param None

  @return DCE function enable bits
  */
UINT32 dce_get_func_en(void)
{
	T_DCE_FUNCTION_REGISTER local_reg;
	UINT32 func_sel = 0;

	local_reg.reg = DCE_GETREG(DCE_FUNCTION_REGISTER_OFS);
	func_sel = local_reg.reg & DCE_FUNC_ALL;

	return func_sel;
}

/**
  Get DCE image size

  Get DCE image size

  @param p_size DCE image size

  @return None
  */
void dce_get_img_size(DCE_IMG_SIZE *p_size)
{
	T_DCE_INPUT_SIZE_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_INPUT_SIZE_REGISTER_OFS);
	p_size->h_size = (local_reg.bit.DCE_HSIZEIN) << 2;
	p_size->v_size = (local_reg.bit.DCE_VSIZEIN) << 1;
}

/**
  Get DCE CFA pattern

  Get DCE CFA pattern

  @param None

  @return DCE CFA pattern
  */
DCE_CFA_PAT dce_get_cfa_pat(void)
{
	T_COLOR_INTERPOLATION_REGISTER_0 local_reg;
	DCE_CFA_PAT cfa_pat;

	local_reg.reg = DCE_GETREG(COLOR_INTERPOLATION_REGISTER_0_OFS);
	if (local_reg.bit.BAYER_FORMAT == DCE_BAYER_2X2) {
		switch (local_reg.bit.CFAPAT) {
		case DCE_CFA_PATR:
			cfa_pat = DCE_CFA_PATR;
			break;
		case DCE_CFA_PATGR:
			cfa_pat = DCE_CFA_PATGR;
			break;
		case DCE_CFA_PATGB:
			cfa_pat = DCE_CFA_PATGB;
			break;
		case DCE_CFA_PATB:
			cfa_pat = DCE_CFA_PATB;
			break;
		default:
			cfa_pat = DCE_CFA_PATR;
			DBG_ERR("dce_getCFAPat() error!\r\n");
			break;
		}
	} else {
		switch (local_reg.bit.CFAPAT) {
		case DCE_CFA_PATR:
			cfa_pat = DCE_CFA_PATR;
			break;
		case DCE_CFA_PATGR:
			cfa_pat = DCE_CFA_PATGR;
			break;
		case DCE_CFA_PAT_IR_GIR1:
			cfa_pat = DCE_CFA_PAT_IR_GIR1;
			break;
		case DCE_CFA_PAT_IR_IR1:
			cfa_pat = DCE_CFA_PAT_IR_IR1;
			break;
		case DCE_CFA_PAT_IR_B:
			cfa_pat = DCE_CFA_PAT_IR_B;
			break;
		case DCE_CFA_PAT_IR_GB:
			cfa_pat = DCE_CFA_PAT_IR_GB;
			break;
		case DCE_CFA_PAT_IR_GIR2:
			cfa_pat = DCE_CFA_PAT_IR_GIR2;
			break;
		case DCE_CFA_PAT_IR_IR2:
			cfa_pat = DCE_CFA_PAT_IR_IR2;
			break;
		default:
			cfa_pat = DCE_CFA_PATR;
			DBG_ERR("dce_getCFAPat() error!\r\n");
			break;
		}
	}
	return cfa_pat;
}

/**
  Get DCE busy status

  Get DCE busy status

  @param None

  @return DCE busy status
  */
DCE_STATUS dce_get_busy_status(void)
{
	T_DCE_STATUS_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_STATUS_REGISTER_OFS);
	if (local_reg.bit.DCE_STATUS == 1) {
		return DCE_BUSY;
	} else {
		return DCE_IDLE;
	}
}

/**
  Get DCE interrupt status

  Get DCE interrupt status

  @param None

  @return DCE interrupt status
  */
UINT32 dce_get_int_status(void)
{
	UINT32 reg;

	reg = DCE_GETREG(DCE_INTERRUPT_STATUS_REGISTER_OFS);
	return reg;
}

/**
  Get DCE H stripe count in process

  Get DCE H stripe count in process

  @param None

  @return H stripe count in process
  */
UINT32 dce_get_h_stp_cnt(void)
{
	T_DCE_STATUS_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_STATUS_REGISTER_OFS);
	return local_reg.bit.HCNT;
}

/**
  Get DCE H stripe settings

  Get DCE H stripe settings

  @param p_size H stripe settings

  @return None
  */
void dce_get_h_stripe(DCE_HSTP_PARAM *p_hstp)
{
	T_DCE_HORIZONTAL_STRIPE_REGISTER0 local_reg0;
	T_DCE_HORIZONTAL_STRIPE_REGISTER1 local_reg1;
	T_DCE_HORIZONTAL_STRIPE_REGISTER2 local_reg2;
	T_DCE_HORIZONTAL_STRIPE_REGISTER3 local_reg3;
	T_DCE_HORIZONTAL_STRIPE_REGISTER4 local_reg4;
	T_DCE_HORIZONTAL_STRIPE_REGISTER5 local_reg5;
	T_DCE_HORIZONTAL_STRIPE_REGISTER6 local_reg6;

	local_reg0.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER0_OFS);
	p_hstp->p_h_stripe[0] = (local_reg0.bit.HSTP0) << 2;
	p_hstp->p_h_stripe[1] = (local_reg0.bit.HSTP1) << 2;
	p_hstp->p_h_stripe[2] = (local_reg0.bit.HSTP2) << 2;

	local_reg1.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER1_OFS);
	p_hstp->p_h_stripe[3] = (local_reg1.bit.HSTP3) << 2;
	p_hstp->p_h_stripe[4] = (local_reg1.bit.HSTP4) << 2;
	p_hstp->p_h_stripe[5] = (local_reg1.bit.HSTP5) << 2;

	local_reg2.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER2_OFS);
	p_hstp->p_h_stripe[6] = (local_reg2.bit.HSTP6) << 2;
	p_hstp->p_h_stripe[7] = (local_reg2.bit.HSTP7) << 2;
	p_hstp->p_h_stripe[8] = (local_reg2.bit.HSTP8) << 2;

	local_reg3.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER3_OFS);
	p_hstp->p_h_stripe[9] = (local_reg3.bit.HSTP9) << 2;
	p_hstp->p_h_stripe[10] = (local_reg3.bit.HSTP10) << 2;
	p_hstp->p_h_stripe[11] = (local_reg3.bit.HSTP11) << 2;

	local_reg4.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER4_OFS);
	p_hstp->p_h_stripe[12] = (local_reg4.bit.HSTP12) << 2;
	p_hstp->p_h_stripe[13] = (local_reg4.bit.HSTP13) << 2;
	p_hstp->p_h_stripe[14] = (local_reg4.bit.HSTP14) << 2;

	local_reg5.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER5_OFS);
	p_hstp->p_h_stripe[15] = (local_reg5.bit.HSTP15) << 2;

	local_reg6.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER6_OFS);
	p_hstp->h_stp_inc_dec.max_inc = local_reg6.bit.HSTP_MAXINC;
	p_hstp->h_stp_inc_dec.max_dec = local_reg6.bit.HSTP_MAXDEC;
	p_hstp->h_stp_in_start = local_reg6.bit.HSTP_INST;
	p_hstp->ime_out_hstp_ovlap = local_reg6.bit.HSTP_IMEOLAP_SEL;
}

/**
  Get DCE H stripe status

  Get DCE H stripe status

  @param p_size H stripe status

  @return None
  */
void dce_get_h_stp_status(DCE_HSTP_STATUS *p_hstp_status)
{
	T_DCE_HORIZONTAL_STRIPE_STATUS_REGISTER_1 local_reg1;
	T_DCE_HORIZONTAL_STRIPE_STATUS_REGISTER_2 local_reg2;
	T_DCE_HORIZONTAL_STRIPE_STATUS_REGISTER_3 local_reg3;
	T_DCE_BUFFER_HEIGHT_STATUS_REGISTER local_reg4;

	local_reg1.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_STATUS_REGISTER_1_OFS);
	p_hstp_status->h_stp_st_x = local_reg1.bit.HSTP_STX;
	p_hstp_status->h_stp_ed_x = local_reg1.bit.HSTP_EDX;

	local_reg2.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_STATUS_REGISTER_2_OFS);
	p_hstp_status->h_stp_clm_st_x = local_reg2.bit.HSTP_CLM_STX;
	p_hstp_status->h_stp_clm_ed_x = local_reg2.bit.HSTP_CLM_EDX;

	local_reg3.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_STATUS_REGISTER_3_OFS);
	p_hstp_status->h_stp_buf_mode = local_reg3.bit.HSTP_BUF_MODE;

	local_reg4.reg = DCE_GETREG(DCE_BUFFER_HEIGHT_STATUS_REGISTER_OFS);
	p_hstp_status->buf_height_g = local_reg4.bit.BUFHEIGHT_G;
	p_hstp_status->buf_height_rgb = local_reg4.bit.BUFHEIGHT_RGB;
	p_hstp_status->buf_height_pix = local_reg4.bit.BUFHEIGHT_PIX;
}

/**
  Get DCE In/Out burst length

  Get DCE In/Out burst length

  @param p_burst_len In/Out burst length

  @return None
  */
void dce_get_burst_length(DCE_BURST_LENGTH *p_burst_len)
{
	T_DMA_BURST_LENGTH_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DMA_BURST_LENGTH_REGISTER_OFS);

	p_burst_len->in_burst_len = (DCE_DMA_BURST)local_reg.bit.DMA_IN_BURST;
	p_burst_len->out_burst_len = (DCE_DMA_BURST)local_reg.bit.DMA_OUT_BURST;
}

/*
   DCE interrupt handler

   DCE interrupt service routine.

   @param None.

   @return None.
   */
void dce_isr(void)
{
	UINT32 int_status;
	//UINT32 cb_status;
	FLGPTN  flag;

	int_status = dce_get_int_status();

	if (dce_platform_get_init_status() == FALSE) {
		dce_clear_int(int_status);
		return;
	}

	int_status = int_status & (dce_get_int_en());
	dce_clear_int(int_status);

	if (int_status == 0) {
		return;
	}


	flag = 0;
	//cb_status = 0;

	if (int_status & DCE_INT_FRMST) {
		flag |= FLGPTN_DCE_FRAMESTART;
		dce_int_stat_cnt.frm_start_cnt++;
	}
	if (int_status & DCE_INT_FRMEND) {
#if !defined (CONFIG_NVT_SMALL_HDAL)
		snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): frame end", hwclock_get_longcounter(), total_log_idx, __func__);
		log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
		total_log_idx++;
#endif

		//DBG_ISR(DBG_COLOR_GREEN"%s():DCE end\r\n"DBG_COLOR_END, __func__);
		flag |= FLGPTN_DCE_FRAMEEND;
		/*
		 *if (dce_get_int_en() & DCE_INT_FRMEND) {
		 *    cb_status |= DCE_INT_FRMEND;
		 *}
		 */
		if(dce_int_stat_cnt.frm_end_cnt >= DCE_ERR_MSG_FREQ){
			dce_int_stat_cnt.frm_end_cnt = 0;
		}
		dce_int_stat_cnt.frm_end_cnt++;
	}
	if (int_status & DCE_INT_STEND) {
		//DBG_ISR("stripe End\r\n");
		dce_int_stat_cnt.strp_end_cnt++;
	}
	if (int_status & DCE_INT_STPERR) {
		if (dce_int_stat_cnt.frm_end_cnt == DCE_ERR_MSG_FREQ && dce_int_stat_cnt.strp_err_cnt < DCE_ERR_MSG_CNT_MAX) {
			DBG_ISR(DBG_COLOR_RED"%s():H stripe Error\r\n"DBG_COLOR_END, __func__);
			dce_int_stat_cnt.strp_err_cnt++;
		}
	}
	if (int_status & DCE_INT_LBOVF) {
		if (dce_int_stat_cnt.frm_end_cnt == DCE_ERR_MSG_FREQ && dce_int_stat_cnt.lbuf_ovfl_cnt < DCE_ERR_MSG_CNT_MAX) {
			DBG_ISR(DBG_COLOR_RED"%s():Line buffer overflow\r\n"DBG_COLOR_END, __func__);
			dce_int_stat_cnt.lbuf_ovfl_cnt++;
		}
	}
	if (int_status & DCE_INT_STPOB) {
		if (dce_int_stat_cnt.frm_end_cnt == DCE_ERR_MSG_FREQ && dce_int_stat_cnt.strp_out_of_bnd_cnt < DCE_ERR_MSG_CNT_MAX) {
			DBG_ISR(DBG_COLOR_RED"%s():X coordinate out of stripe boundaries\r\n"DBG_COLOR_END, __func__);
			dce_int_stat_cnt.strp_out_of_bnd_cnt++;
		}
	}
	if (int_status & DCE_INT_YBACK) {
		if (dce_int_stat_cnt.frm_end_cnt == DCE_ERR_MSG_FREQ && dce_int_stat_cnt.y_back_cnt < DCE_ERR_MSG_CNT_MAX) {
			DBG_ISR(DBG_COLOR_RED"%s():Y Coordinate backward skip\r\n"DBG_COLOR_END, __func__);
			dce_int_stat_cnt.y_back_cnt++;
		}
	}
	if (int_status & DCE_INT_LLEND) {
		dce_int_stat_cnt.dce_ll_end_cnt++;
	}
	if (int_status & DCE_INT_LLJOBEND) {
		dce_int_stat_cnt.dce_ll_jobend_cnt++;
	}
	if (int_status & DCE_INT_LLERR) {
		if (dce_int_stat_cnt.frm_end_cnt == DCE_ERR_MSG_FREQ && dce_int_stat_cnt.dce_ll_err_cnt < DCE_ERR_MSG_CNT_MAX) {
			DBG_ISR(DBG_COLOR_RED"%s():Linked-list command error\r\n"DBG_COLOR_END, __func__);
			dce_int_stat_cnt.dce_ll_err_cnt++;
		}
	}
	if (int_status & DCE_INT_LLERR2) {
		if (dce_int_stat_cnt.frm_end_cnt == DCE_ERR_MSG_FREQ && dce_int_stat_cnt.dce_ll_err2_cnt < DCE_ERR_MSG_CNT_MAX) {
			DBG_ISR(DBG_COLOR_RED"%s():Linked-list not finished in direct mode\r\n"DBG_COLOR_END, __func__);
			dce_int_stat_cnt.dce_ll_err2_cnt++;
		}
	}

#if 1//def __KERNEL__
	if (g_pDceCBFunc != NULL) {
		if (int_status) {
#if !defined (CONFIG_NVT_SMALL_HDAL)
			snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): callback", hwclock_get_longcounter(), total_log_idx, __func__);
			log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
			total_log_idx++;
#endif

			g_pDceCBFunc(int_status);
		}
	}
#else
	if (pfIstCB_DCE != NULL) {
		if (cb_status) {
			drv_setIstEvent(DRV_IST_MODULE_DCE, cb_status);
		}
	}
#endif

	dce_platform_flg_set(flag);

}

#if defined (_NVT_EMULATION_)
static void dce_time_out_cb(UINT32 event)
{
	DBG_ERR("DCE time out!\r\n");
	dce_end_time_out_status = TRUE;
	dce_platform_flg_set(FLGPTN_DCE_FRAMEEND);
}
#endif

/**
  Wait frame-end flag within system

  Wait system flag of frame-end of DCE engine

  @param[in] isClear  Clear flag selection\n
  -@b DCE_FLAG_NO_CLEAR  No clear flag\n
  -@b DCE_FLAG_CLEAR  Clear flag\n

  @return None
  */
void dce_wait_flag_frame_end(DCE_FLAG_CLEAR_SEL flag_clear)
{
	FLGPTN flag;

	if (flag_clear == DCE_FLAG_CLEAR) {
		dce_clear_flag_frame_end();
	}

#if defined (_NVT_EMULATION_)
	if (timer_open(&dce_timer_id, dce_time_out_cb) != E_OK) {
		DBG_WRN("DCE allocate timer fail\r\n");
	} else {
		timer_cfg(dce_timer_id, 6000000, TIMER_MODE_ONE_SHOT | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
	}
#endif

	dce_platform_flg_wait((PFLGPTN)&flag, FLGPTN_DCE_FRAMEEND);

#if defined (_NVT_EMULATION_)
	timer_close(dce_timer_id);
#endif

#if (DCE_DMA_CACHE_HANDLE == 1)
	//DMA flush read cache end
	for (g_dma_och = 0; g_dma_och < DCE_DMA_OUT_NUM; g_dma_och++) {
		if (dce_dma_out[g_dma_och].enable) {
			if (g_dma_och != 0) {
				if (dce_dma_out[g_dma_och].width_eq_lofst) {
					vos_cpu_dcache_sync_vb((VOS_ADDR)dce_dma_out[g_dma_och].addr, dce_dma_out[g_dma_och].size, VOS_DMA_FROM_DEVICE);
				} else {
					vos_cpu_dcache_sync_vb((VOS_ADDR)dce_dma_out[g_dma_och].addr, dce_dma_out[g_dma_och].size, VOS_DMA_BIDIRECTIONAL);
				}
			} else {
				if (dce_dma_out[g_dma_och].width_eq_lofst) {
					vos_cpu_dcache_sync((VOS_ADDR)dce_dma_out[g_dma_och].addr, dce_dma_out[g_dma_och].size, VOS_DMA_FROM_DEVICE);
				} else {
					vos_cpu_dcache_sync((VOS_ADDR)dce_dma_out[g_dma_och].addr, dce_dma_out[g_dma_och].size, VOS_DMA_BIDIRECTIONAL);
				}
			}
		}
	}
#endif
}

/**
  Clear frame-end flag within system

  Clear system flag of DCE engine frame-end

  @return None
  */
void dce_clear_flag_frame_end(void)
{
	dce_platform_flg_clear(FLGPTN_DCE_FRAMEEND);
}

/**
  Wait frame-start flag

  Wait system flag of frame-start of DCE engine

  @param[in] isClear  Clear flag selection\n
  -@b DCE_FLAG_NO_CLEAR  No clear flag\n
  -@b DCE_FLAG_CLEAR  Clear flag\n

  @return None
  */
void dce_wait_flag_frame_start(DCE_FLAG_CLEAR_SEL flag_clear)
{
	FLGPTN flag;

	if (flag_clear == DCE_FLAG_CLEAR) {
		dce_clear_flag_frame_end();
	}

	dce_platform_flg_wait((PFLGPTN)&flag, FLGPTN_DCE_FRAMESTART);
}

/**
  Clear frame-end flag within system

  Clear system flag of DCE engine frame-end

  @return None
  */
void dce_clear_flag_frame_start(void)
{
	dce_platform_flg_clear(FLGPTN_DCE_FRAMESTART);
}

/**
  Wait Linked list done flag

  Wait system flag of Linked list done of DCE engine

  @param[in] isClear  Clear flag selection\n
  -@b DCE_FLAG_NO_CLEAR  No clear flag\n
  -@b DCE_FLAG_CLEAR  Clear flag\n

  @return None
  */
void dce_wait_flag_ll_done(DCE_FLAG_CLEAR_SEL flag_clear)
{
	FLGPTN flag;

	if (flag_clear == DCE_FLAG_CLEAR) {
		dce_clear_flag_ll_done();
	}

	dce_platform_flg_wait((PFLGPTN)&flag, FLGPTN_DCE_LL_DONE);
}

/**
  Clear Linked list job end flag within system

  Clear system flag of DCE engine Linked list job end

  @return None
  */
void dce_clear_flag_ll_job_end(void)
{
	dce_platform_flg_clear(FLGPTN_DCE_LL_JOB_END);
}

/**
  Wait Linked list job end flag

  Wait system flag of Linked list job end of DCE engine

  @param[in] isClear  Clear flag selection\n
  -@b DCE_FLAG_NO_CLEAR  No clear flag\n
  -@b DCE_FLAG_CLEAR  Clear flag\n

  @return None
  */
void dce_wait_flag_ll_job_end(DCE_FLAG_CLEAR_SEL flag_clear)
{
	FLGPTN flag;

	if (flag_clear == DCE_FLAG_CLEAR) {
		dce_clear_flag_ll_job_end();
	}

	dce_platform_flg_wait((PFLGPTN)&flag, FLGPTN_DCE_LL_JOB_END);
}

/**
  Clear Linked list done flag within system

  Clear system flag of DCE engine Linked list done

  @return None
  */
void dce_clear_flag_ll_done(void)
{
	dce_platform_flg_clear(FLGPTN_DCE_LL_DONE);
}

//-----------------------------------Layer1-----------------------------------------------------//
ER dce_check_state_machine(DCE_ENGINE_STATE current_state, DCE_OPERATION operation)
{
	ER er_return;

	switch (current_state) {
	case DCE_STATE_IDLE:
		switch (operation) {
		case DCE_OP_OPEN:
		case DCE_OP_RESET:
			er_return = E_OK;
			break;
		default:
			er_return = E_SYS;
			break;
		}
		break;

	case DCE_STATE_READY:
		switch (operation) {
		case DCE_OP_CLOSE:
		case DCE_OP_PAUSE:
		case DCE_OP_SETMODE:
		case DCE_OP_CHGPARAM:
		case DCE_OP_RESET:
			er_return = E_OK;
			break;
		default:
			er_return = E_SYS;
			break;
		}
		break;

	case DCE_STATE_PAUSE:
		switch (operation) {
		case DCE_OP_CLOSE:
		case DCE_OP_PAUSE:
		case DCE_OP_START:
		case DCE_OP_SETMODE:
		case DCE_OP_CHGPARAM:
		case DCE_OP_RESET:
			er_return = E_OK;
			break;
		default:
			er_return = E_SYS;
			break;
		}
		break;

	case DCE_STATE_RUN:
		switch (operation) {
		case DCE_OP_PAUSE:
		case DCE_OP_CHGPARAM:
		case DCE_OP_RESET:
			er_return = E_OK;
			break;
		default:
			er_return = E_SYS;
			break;
		}
		break;

	default:
		er_return = E_SYS;
	}

	if (er_return != E_OK) {
#if !defined (CONFIG_NVT_SMALL_HDAL)
		int i;
		snprintf(g_kdrv_dce_ctrl_api_log[log_idx], KDRV_DCE_LOG_MSG_LENGTH, "%lld[%u]%s(): err op %d at state %d", hwclock_get_longcounter(), total_log_idx, __func__, operation, current_state);
		log_idx = (log_idx + 1) % KDRV_DCE_LOG_NUM;
		total_log_idx++;
		for (i = 0; i < KDRV_DCE_LOG_NUM; i++) {
			DBG_DUMP("[%d]%s\r\n", i, g_kdrv_dce_ctrl_api_log[i]);
		}
#endif
		DBG_ERR("Illegal operation %d at State %d\r\n", operation, current_state);
	}

	return er_return;
}

ER dce_lock_sem(void)
{
	ER er_return = E_OK;

	er_return = dce_platform_sem_wait();
	if (er_return != E_OK) {
		return er_return;
	}

	g_dce_sem_lock_status = DCE_SEM_LOCKED;

	return er_return;
}

ER dce_unlock_sem(void)
{

	ER er_return = E_OK;

	g_dce_sem_lock_status = DCE_SEM_UNLOCKED;

	er_return = dce_platform_sem_signal();
	if (er_return != E_OK) {
		return er_return;
	}

	return er_return;
}

DCE_SEM_LOCK_STATUS dce_get_sem_lock_status(void)
{
	return g_dce_sem_lock_status;
}

void dce_attach(void)
{
	if (g_dce_clk_en == FALSE) {
		dce_platform_enable_clk();
		g_dce_clk_en = TRUE;
	}
}

void dce_detach(void)
{
	if (g_dce_clk_en == TRUE) {
		dce_platform_disable_clk();
		g_dce_clk_en = FALSE;
	}
}



//-----------------------------------Layer0-----------------------------------------------------//

void dce_set_reset(BOOL reset_en)
{
	T_DCE_CONTROL_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_CONTROL_REGISTER_OFS);
	local_reg.bit.DCE_RST = reset_en;
	//HW clear load, force write zero to prevent multiple load
	local_reg.bit.DCE_START_LOAD = DISABLE;
	local_reg.bit.DCE_FRMEND_LOAD = DISABLE;
	local_reg.bit.DCE_FRMSTART_LOAD = DISABLE;

	DCE_SETREG(DCE_CONTROL_REGISTER_OFS, local_reg.reg);
}


void dce_set_ll_fire(void)
{
	T_DCE_CONTROL_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_CONTROL_REGISTER_OFS);
	local_reg.bit.LL_FIRE = ENABLE;
	local_reg.bit.DCE_START = DISABLE;
	local_reg.bit.DCE_START_LOAD = DISABLE;
	local_reg.bit.DCE_FRMEND_LOAD = DISABLE;
	local_reg.bit.DCE_FRMSTART_LOAD = DISABLE;

	DCE_SETREG(DCE_CONTROL_REGISTER_OFS, local_reg.reg);
}


void dce_set_ll_terminate(void)
{
	T_LINKED_LIST_MODE_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(LINKED_LIST_MODE_REGISTER_OFS);
	local_reg.bit.LL_TERMINATE = ENABLE;


	DCE_SETREG(LINKED_LIST_MODE_REGISTER_OFS, local_reg.reg);
}


void dce_set_start(BOOL start_en)
{
	T_DCE_CONTROL_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_CONTROL_REGISTER_OFS);
	local_reg.bit.DCE_START = start_en;
	//HW clear load, force write zero to prevent multiple load
	local_reg.bit.DCE_START_LOAD = DISABLE;
	local_reg.bit.DCE_FRMEND_LOAD = DISABLE;
	local_reg.bit.DCE_FRMSTART_LOAD = DISABLE;

	DCE_SETREG(DCE_CONTROL_REGISTER_OFS, local_reg.reg);
}

void dce_set_start_fd_load(void)
{
	T_DCE_CONTROL_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_CONTROL_REGISTER_OFS);
	local_reg.bit.DCE_START = ENABLE;
	local_reg.bit.DCE_FRMEND_LOAD = ENABLE;
	//HW clear load, force write zero to prevent multiple load
	local_reg.bit.DCE_START_LOAD = DISABLE;;
	local_reg.bit.DCE_FRMSTART_LOAD = DISABLE;

	DCE_SETREG(DCE_CONTROL_REGISTER_OFS, local_reg.reg);
}


void dce_set_load(DCE_LOAD_TYPE type)
{
	T_DCE_CONTROL_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_CONTROL_REGISTER_OFS);
	switch (type) {
	case DCE_START_LOAD_TYPE:
		local_reg.bit.DCE_START_LOAD = ENABLE;
		//HW clear load, force write zero to prevent multiple load
		local_reg.bit.DCE_FRMEND_LOAD = DISABLE;
		local_reg.bit.DCE_FRMSTART_LOAD = DISABLE;
		break;
	case DCE_FRMEND_LOAD_TYPE:
		if (g_dce_engine_state == DCE_STATE_RUN) {
			local_reg.bit.DCE_FRMEND_LOAD = ENABLE;
			//HW clear load, force write zero to prevent multiple load
			local_reg.bit.DCE_START_LOAD = DISABLE;
			local_reg.bit.DCE_FRMSTART_LOAD = DISABLE;
		}
		break;
	case DCE_IMD_LOAD_TYPE:
		/*
		   local_reg.bit.DCE_IMD_LOAD = ENABLE;
		//HW clear load, force write zero to prevent multiple load
		local_reg.bit.DCE_START_LOAD = DISABLE;
		local_reg.bit.DCE_FRMEND_LOAD = DISABLE;
		*/
		break;
	case DCE_DIRECT_START_LOAD_TYPE:
		local_reg.bit.DCE_FRMSTART_LOAD = ENABLE;
		//HW clear load, force write zero to prevent multiple load
		local_reg.bit.DCE_FRMEND_LOAD = DISABLE;
		local_reg.bit.DCE_START_LOAD = DISABLE;
		break;
	default:
		DBG_ERR("dce_set_load() load type error!\r\n");
		break;
	}

	DCE_SETREG(DCE_CONTROL_REGISTER_OFS, local_reg.reg);
}

void dce_set_op_mode(DCE_OPMODE mode)
{
	T_DCE_FUNCTION_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_FUNCTION_REGISTER_OFS);
	switch (mode) {
	case DCE_OPMODE_IFE_IME:
		local_reg.bit.DCE_OP = 0;
		break;
	case DCE_OPMODE_DCE_IME:
		local_reg.bit.DCE_OP = 1;
		break;
	case DCE_OPMODE_SIE_IME:
		local_reg.bit.DCE_OP = 2;
		break;
	case DCE_OPMODE_DCE_D2D:
		local_reg.bit.DCE_OP = 3;
		break;
	default:
		DBG_ERR("dce_set_op_mode() mode error!\r\n");
		break;
	}

	DCE_SETREG(DCE_FUNCTION_REGISTER_OFS, local_reg.reg);
}

void dce_set_stp_mode(DCE_STP_MODE mode)
{
	T_DCE_FUNCTION_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_FUNCTION_REGISTER_OFS);
	if (mode == DCE_STPMODE_SST) {
		local_reg.bit.DCE_STP = 0;
	} else {
		local_reg.bit.DCE_STP = 1;
	}

	DCE_SETREG(DCE_FUNCTION_REGISTER_OFS, local_reg.reg);
}

void dce_set_dram_out_mode(DCE_DRAM_OUTPUT_MODE outmode)
{
	T_DMA_OUTPUT_CHANNEL_ENABLE_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DMA_OUTPUT_CHANNEL_ENABLE_REGISTER_OFS);
	local_reg.bit.DCE_DRAM_OUT_MODE = outmode;

	DCE_SETREG(DMA_OUTPUT_CHANNEL_ENABLE_REGISTER_OFS, local_reg.reg);
}

void dce_set_dram_single_out_en(DCE_DRAM_SINGLE_OUT_EN *ch_en)
{

	T_DMA_OUTPUT_CHANNEL_ENABLE_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DMA_OUTPUT_CHANNEL_ENABLE_REGISTER_OFS);
	local_reg.bit.DCE_DRAM_OUT0_SINGLE_EN = ch_en->single_out_wdr_en;
	local_reg.bit.DCE_DRAM_OUT1_SINGLE_EN = ch_en->single_out_cfa_en;

	DCE_SETREG(DMA_OUTPUT_CHANNEL_ENABLE_REGISTER_OFS, local_reg.reg);
}

void dce_enable_function(UINT32 func_sel)
{
	T_DCE_FUNCTION_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_FUNCTION_REGISTER_OFS);
	local_reg.reg |= func_sel;

	DCE_SETREG(DCE_FUNCTION_REGISTER_OFS, local_reg.reg);
}

void dce_disable_function(UINT32 func_sel)
{
	T_DCE_FUNCTION_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_FUNCTION_REGISTER_OFS);
	local_reg.reg &= (~func_sel);

	DCE_SETREG(DCE_FUNCTION_REGISTER_OFS, local_reg.reg);
}

void dce_set_function(UINT32 func_sel)
{
	T_DCE_FUNCTION_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_FUNCTION_REGISTER_OFS);
	local_reg.reg &= (~DCE_FUNC_ALL);
	local_reg.reg |= func_sel;

	DCE_SETREG(DCE_FUNCTION_REGISTER_OFS, local_reg.reg);
}

void dce_set_sram_mode(DCE_SRAM_SEL ui_sram_sel)
{
	T_DCE_FUNCTION_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_FUNCTION_REGISTER_OFS);
	local_reg.bit.SRAM_MODE = ui_sram_sel;

	DCE_SETREG(DCE_FUNCTION_REGISTER_OFS, local_reg.reg);

	if (ui_sram_sel == DCE_SRAM_CNN) {
		// CNN2 & DCE won't be enabled at the same time
		// SRAM use right always changes from CPU to DCE or CNN2
		// Change only CNN2 SRAM select register and SRAM shutdown
		T_CNN_CONTROL_REGISTER cnn_reg;
		cnn_reg.reg = CNN2_GETREG(CNN_CONTROL_REGISTER_OFS);
		cnn_reg.bit.CNN_SRAMCTRL = 1;
		CNN2_SETREG(CNN_CONTROL_REGISTER_OFS, cnn_reg.reg);

		cnn2_platform_disable_sram_shutdown();
	}
}

void dce_set_gdc_mode(DCE_GDC_LUT_MODE ui_gdc_mode)
{
	T_GDC_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(GDC_REGISTER_OFS);
	local_reg.bit.GDC_MODE = ui_gdc_mode;

	DCE_SETREG(GDC_REGISTER_OFS, local_reg.reg);
}

void dce_set_wdr_dither(DCE_WDR_DITHER *p_dither)
{
	T_WDR_SUBIMAGE_REGISTER_0 local_reg;

	local_reg.reg = DCE_GETREG(WDR_SUBIMAGE_REGISTER_0_OFS);
	local_reg.bit.WDR_RAND_RESET = p_dither->wdr_rand_rst;
	local_reg.bit.WDR_RAND_SEL = p_dither->wdr_rand_sel;

	DCE_SETREG(WDR_SUBIMAGE_REGISTER_0_OFS, local_reg.reg);
}
void dce_set_wdr_scaling_fact(DCE_HV_FACTOR scaling_fact)
{
	T_WDR_SUBIMAGE_REGISTER_5 local_reg0;

	local_reg0.reg = DCE_GETREG(WDR_SUBIMAGE_REGISTER_5_OFS);
	local_reg0.bit.WDR_SUBIMG_HFACTOR = scaling_fact.factor_h;
	local_reg0.bit.WDR_SUBIMG_VFACTOR = scaling_fact.factor_v;

	DCE_SETREG(WDR_SUBIMAGE_REGISTER_5_OFS, local_reg0.reg);
}

void dce_set_wdr_subimg_size(DCE_IMG_SIZE subimg_size)
{
	T_WDR_SUBIMAGE_REGISTER_0 local_reg;

	local_reg.reg = DCE_GETREG(WDR_SUBIMAGE_REGISTER_0_OFS);
	local_reg.bit.WDR_SUBIMG_WIDTH = subimg_size.h_size - 1;
	local_reg.bit.WDR_SUBIMG_HEIGHT = subimg_size.v_size - 1;

	DCE_SETREG(WDR_SUBIMAGE_REGISTER_0_OFS, local_reg.reg);
}

void dce_set_wdr_subout(DCE_IMG_SIZE blk_size, DCE_HV_FACTOR blk_cent_fact)
{
	T_WDR_SUBIMAGE_REGISTER_6 local_reg0;
	T_WDR_SUBIMAGE_REGISTER_7 local_reg1;

	local_reg0.reg = DCE_GETREG(WDR_SUBIMAGE_REGISTER_6_OFS);
	local_reg0.bit.WDR_SUB_BLK_SIZEH = blk_size.h_size;
	local_reg0.bit.WDR_BLK_CENT_HFACTOR = blk_cent_fact.factor_h;
	local_reg1.reg = DCE_GETREG(WDR_SUBIMAGE_REGISTER_7_OFS);
	local_reg1.bit.WDR_SUB_BLK_SIZEV = blk_size.v_size;
	local_reg1.bit.WDR_BLK_CENT_VFACTOR = blk_cent_fact.factor_v;

	DCE_SETREG(WDR_SUBIMAGE_REGISTER_6_OFS, local_reg0.reg);
	DCE_SETREG(WDR_SUBIMAGE_REGISTER_7_OFS, local_reg1.reg);
}

void dce_set_wdr_subimg_filtcoeff(UINT32 *p_filt_coeff)
{
	T_WDR_SUBIMG_LPF_REGISTER local_reg0;

	local_reg0.reg = DCE_GETREG(WDR_SUBIMG_LPF_REGISTER_OFS);
	local_reg0.bit.WDR_LPF_C0 = p_filt_coeff[0];
	local_reg0.bit.WDR_LPF_C1 = p_filt_coeff[1];
	local_reg0.bit.WDR_LPF_C2 = p_filt_coeff[2];

	DCE_SETREG(WDR_SUBIMG_LPF_REGISTER_OFS, local_reg0.reg);
}

void dce_set_wdr_input_bld(WDR_IN_BLD_PARAM *p_input_bld)
{
	T_WDR_INPUT_BLENDING_REGISTER0 local_reg0;
	T_WDR_INPUT_BLENDING_REGISTER1 local_reg1;
	T_WDR_INPUT_BLENDING_REGISTER2 local_reg2;
	T_WDR_INPUT_BLENDING_REGISTER3 local_reg3;
	T_WDR_INPUT_BLENDING_REGISTER4 local_reg4;

	local_reg0.reg = DCE_GETREG(WDR_INPUT_BLENDING_REGISTER0_OFS);
	local_reg0.bit.WDR_INPUT_BLDRTO0 = p_input_bld->p_inblend_lut[0];
	local_reg0.bit.WDR_INPUT_BLDRTO1 = p_input_bld->p_inblend_lut[1];
	local_reg0.bit.WDR_INPUT_BLDRTO2 = p_input_bld->p_inblend_lut[2];
	local_reg0.bit.WDR_INPUT_BLDRTO3 = p_input_bld->p_inblend_lut[3];

	local_reg1.reg = DCE_GETREG(WDR_INPUT_BLENDING_REGISTER0_OFS);
	local_reg1.bit.WDR_INPUT_BLDRTO4 = p_input_bld->p_inblend_lut[4];
	local_reg1.bit.WDR_INPUT_BLDRTO5 = p_input_bld->p_inblend_lut[5];
	local_reg1.bit.WDR_INPUT_BLDRTO6 = p_input_bld->p_inblend_lut[6];
	local_reg1.bit.WDR_INPUT_BLDRTO7 = p_input_bld->p_inblend_lut[7];

	local_reg2.reg = DCE_GETREG(WDR_INPUT_BLENDING_REGISTER0_OFS);
	local_reg2.bit.WDR_INPUT_BLDRTO8 = p_input_bld->p_inblend_lut[8];
	local_reg2.bit.WDR_INPUT_BLDRTO9 = p_input_bld->p_inblend_lut[9];
	local_reg2.bit.WDR_INPUT_BLDRTO10 = p_input_bld->p_inblend_lut[10];
	local_reg2.bit.WDR_INPUT_BLDRTO11 = p_input_bld->p_inblend_lut[11];

	local_reg3.reg = DCE_GETREG(WDR_INPUT_BLENDING_REGISTER0_OFS);
	local_reg3.bit.WDR_INPUT_BLDRTO12 = p_input_bld->p_inblend_lut[12];
	local_reg3.bit.WDR_INPUT_BLDRTO13 = p_input_bld->p_inblend_lut[13];
	local_reg3.bit.WDR_INPUT_BLDRTO14 = p_input_bld->p_inblend_lut[14];
	local_reg3.bit.WDR_INPUT_BLDRTO15 = p_input_bld->p_inblend_lut[15];

	local_reg4.reg = DCE_GETREG(WDR_INPUT_BLENDING_REGISTER0_OFS);
	local_reg4.bit.WDR_INPUT_BLDRTO16 = p_input_bld->p_inblend_lut[16];
	local_reg4.bit.WDR_INPUT_BLDSRC_SEL = p_input_bld->wdr_bld_sel;
	local_reg4.bit.WDR_INPUT_BLDWT = p_input_bld->wdr_bld_wt;

	DCE_SETREG(WDR_INPUT_BLENDING_REGISTER0_OFS, local_reg0.reg);
	DCE_SETREG(WDR_INPUT_BLENDING_REGISTER1_OFS, local_reg1.reg);
	DCE_SETREG(WDR_INPUT_BLENDING_REGISTER2_OFS, local_reg2.reg);
	DCE_SETREG(WDR_INPUT_BLENDING_REGISTER3_OFS, local_reg3.reg);
	DCE_SETREG(WDR_INPUT_BLENDING_REGISTER4_OFS, local_reg4.reg);
}

void dce_set_wdr_strength(WDR_STRENGTH_PARAM *p_strength_param)
{
	T_WDR_PARAMETER_REGISTER0 local_reg0;
	T_WDR_PARAMETER_REGISTER1 local_reg1;
	T_WDR_OUTPUT_BLENDING_REGISTER local_reg2;

	local_reg0.reg = DCE_GETREG(WDR_PARAMETER_REGISTER0_OFS);
	local_reg0.bit.WDR_COEFF1 = p_strength_param->p_wdr_coeff[0];
	local_reg0.bit.WDR_COEFF2 = p_strength_param->p_wdr_coeff[1];

	local_reg1.reg = DCE_GETREG(WDR_PARAMETER_REGISTER1_OFS);
	local_reg1.bit.WDR_COEFF3 = p_strength_param->p_wdr_coeff[2];
	local_reg1.bit.WDR_COEFF4 = p_strength_param->p_wdr_coeff[3];

	local_reg2.reg = DCE_GETREG(WDR_OUTPUT_BLENDING_REGISTER_OFS);
	local_reg2.bit.WDR_STRENGTH = p_strength_param->strength;
	local_reg2.bit.WDR_CONTRAST = p_strength_param->contrast;

	DCE_SETREG(WDR_PARAMETER_REGISTER0_OFS, local_reg0.reg);
	DCE_SETREG(WDR_PARAMETER_REGISTER1_OFS, local_reg1.reg);
	DCE_SETREG(WDR_OUTPUT_BLENDING_REGISTER_OFS, local_reg2.reg);
}

void dce_set_wdr_gain_ctrl(WDR_GAINCTRL_PARAM *p_gainctrl_param)
{
	T_WDR_CONTROL_REGISTER local_reg0;

	local_reg0.reg = DCE_GETREG(WDR_CONTROL_REGISTER_OFS);
	local_reg0.bit.WDR_GAINCTRL_EN = p_gainctrl_param->gainctrl_en;
	local_reg0.bit.WDR_MAXGAIN = p_gainctrl_param->max_gain;
	local_reg0.bit.WDR_MINGAIN = p_gainctrl_param->min_gain;

	DCE_SETREG(WDR_CONTROL_REGISTER_OFS, local_reg0.reg);
}

void dce_set_wdr_outbld_en(BOOL enable)
{

	T_WDR_CONTROL_REGISTER local_reg0;

	local_reg0.reg = DCE_GETREG(WDR_CONTROL_REGISTER_OFS);
	local_reg0.bit.WDR_OUTBLD_TABLE_EN = enable;

	DCE_SETREG(WDR_CONTROL_REGISTER_OFS, local_reg0.reg);
}

void dce_set_wdr_sat_reduct(WDR_SAT_REDUCT_PARAM *p_sat_param)
{
	T_WDR_SATURATION_REDUCTION_REGISTER local_reg0;

	local_reg0.reg = DCE_GETREG(WDR_SATURATION_REDUCTION_REGISTER_OFS);
	local_reg0.bit.WDR_SAT_TH = p_sat_param->sat_th;
	local_reg0.bit.WDR_SAT_WT_LOW = p_sat_param->sat_wt_low;
	local_reg0.bit.WDR_SAT_DELTA = p_sat_param->sat_delta;

	DCE_SETREG(WDR_SATURATION_REDUCTION_REGISTER_OFS, local_reg0.reg);
}

void dce_set_wdr_tcurve_idx(UINT32 *p_wdr_tcurve_idx)
{
	UINT32 i = 0;

	T_WDR_TONE_CURVE_REGISTER0 wdr_tcurve_idx;

	for (i = 0; i < 8; i++) {
		wdr_tcurve_idx.reg = DCE_GETREG(WDR_TONE_CURVE_REGISTER0_OFS + (i * 4));
		wdr_tcurve_idx.bit.WDR_TCURVE_INDEX_LUT0 = p_wdr_tcurve_idx[4 * i];
		wdr_tcurve_idx.bit.WDR_TCURVE_INDEX_LUT1 = p_wdr_tcurve_idx[4 * i + 1];
		wdr_tcurve_idx.bit.WDR_TCURVE_INDEX_LUT2 = p_wdr_tcurve_idx[4 * i + 2];
		wdr_tcurve_idx.bit.WDR_TCURVE_INDEX_LUT3 = p_wdr_tcurve_idx[4 * i + 3];
		DCE_SETREG((WDR_TONE_CURVE_REGISTER0_OFS + (i * 4)), wdr_tcurve_idx.reg);
	}
}

void dce_set_wdr_tcurve_split(UINT32 *p_wdr_tcurve_split)
{
	UINT32 i = 0;

	T_WDR_TONE_CURVE_REGISTER8 wdr_tcurve_split;

	for (i = 0; i < 2; i++) {
		wdr_tcurve_split.reg = DCE_GETREG(WDR_TONE_CURVE_REGISTER8_OFS + (i * 4));
		wdr_tcurve_split.bit.WDR_TCURVE_SPLIT_LUT0 = p_wdr_tcurve_split[16 * i];
		wdr_tcurve_split.bit.WDR_TCURVE_SPLIT_LUT1 = p_wdr_tcurve_split[16 * i + 1];
		wdr_tcurve_split.bit.WDR_TCURVE_SPLIT_LUT2 = p_wdr_tcurve_split[16 * i + 2];
		wdr_tcurve_split.bit.WDR_TCURVE_SPLIT_LUT3 = p_wdr_tcurve_split[16 * i + 3];
		wdr_tcurve_split.bit.WDR_TCURVE_SPLIT_LUT4 = p_wdr_tcurve_split[16 * i + 4];
		wdr_tcurve_split.bit.WDR_TCURVE_SPLIT_LUT5 = p_wdr_tcurve_split[16 * i + 5];
		wdr_tcurve_split.bit.WDR_TCURVE_SPLIT_LUT6 = p_wdr_tcurve_split[16 * i + 6];
		wdr_tcurve_split.bit.WDR_TCURVE_SPLIT_LUT7 = p_wdr_tcurve_split[16 * i + 7];
		wdr_tcurve_split.bit.WDR_TCURVE_SPLIT_LUT8 = p_wdr_tcurve_split[16 * i + 8];
		wdr_tcurve_split.bit.WDR_TCURVE_SPLIT_LUT9 = p_wdr_tcurve_split[16 * i + 9];
		wdr_tcurve_split.bit.WDR_TCURVE_SPLIT_LUT10 = p_wdr_tcurve_split[16 * i + 10];
		wdr_tcurve_split.bit.WDR_TCURVE_SPLIT_LUT11 = p_wdr_tcurve_split[16 * i + 11];
		wdr_tcurve_split.bit.WDR_TCURVE_SPLIT_LUT12 = p_wdr_tcurve_split[16 * i + 12];
		wdr_tcurve_split.bit.WDR_TCURVE_SPLIT_LUT13 = p_wdr_tcurve_split[16 * i + 13];
		wdr_tcurve_split.bit.WDR_TCURVE_SPLIT_LUT14 = p_wdr_tcurve_split[16 * i + 14];
		wdr_tcurve_split.bit.WDR_TCURVE_SPLIT_LUT15 = p_wdr_tcurve_split[16 * i + 15];
		DCE_SETREG((WDR_TONE_CURVE_REGISTER8_OFS + (i * 4)), wdr_tcurve_split.reg);
	}
}

void dce_set_wdr_tcurve_val(UINT32 *p_wdr_tcurve_val)
{
	UINT32 i = 0;

	T_WDR_TONE_CURVE_REGISTER10 wdr_tcurve_val;

	for (i = 0; i < 32; i++) {
		wdr_tcurve_val.reg = DCE_GETREG(WDR_TONE_CURVE_REGISTER10_OFS + (i * 4));
		wdr_tcurve_val.bit.WDR_TCURVE_VAL_LUT0 = p_wdr_tcurve_val[2 * i];
		wdr_tcurve_val.bit.WDR_TCURVE_VAL_LUT1 = p_wdr_tcurve_val[2 * i + 1];
		DCE_SETREG((WDR_TONE_CURVE_REGISTER10_OFS + (i * 4)), wdr_tcurve_val.reg);
	}

	wdr_tcurve_val.reg = DCE_GETREG(WDR_TONE_CURVE_REGISTER42_OFS);
	wdr_tcurve_val.bit.WDR_TCURVE_VAL_LUT0 = p_wdr_tcurve_val[64];
	DCE_SETREG(WDR_TONE_CURVE_REGISTER42_OFS, wdr_tcurve_val.reg);
}

void dce_set_wdr_outbld_idx(UINT32 *p_wdr_outbld_idx)
{
	UINT32 i = 0;

	T_WDR_OUTPUT_BLENDING_CURVE_REGISTER0 wdr_outbld_idx;

	for (i = 0; i < 8; i++) {
		wdr_outbld_idx.reg = DCE_GETREG(WDR_OUTPUT_BLENDING_CURVE_REGISTER0_OFS + (i * 4));
		wdr_outbld_idx.bit.WDR_OUTBLD_INDEX_LUT0 = p_wdr_outbld_idx[4 * i];
		wdr_outbld_idx.bit.WDR_OUTBLD_INDEX_LUT1 = p_wdr_outbld_idx[4 * i + 1];
		wdr_outbld_idx.bit.WDR_OUTBLD_INDEX_LUT2 = p_wdr_outbld_idx[4 * i + 2];
		wdr_outbld_idx.bit.WDR_OUTBLD_INDEX_LUT3 = p_wdr_outbld_idx[4 * i + 3];
		DCE_SETREG((WDR_OUTPUT_BLENDING_CURVE_REGISTER0_OFS + (i * 4)), wdr_outbld_idx.reg);
	}

}

void dce_set_wdr_outbld_split(UINT32 *p_wdr_outbld_split)
{
	UINT32 i = 0;

	T_WDR_OUTPUT_BLENDING_CURVE_REGISTER8 wdr_outbld_split;

	for (i = 0; i < 2; i++) {
		wdr_outbld_split.reg = DCE_GETREG(WDR_OUTPUT_BLENDING_CURVE_REGISTER8_OFS + (i * 4));
		wdr_outbld_split.bit.WDR_OUTBLD_SPLIT_LUT0 = p_wdr_outbld_split[16 * i];
		wdr_outbld_split.bit.WDR_OUTBLD_SPLIT_LUT1 = p_wdr_outbld_split[16 * i + 1];
		wdr_outbld_split.bit.WDR_OUTBLD_SPLIT_LUT2 = p_wdr_outbld_split[16 * i + 2];
		wdr_outbld_split.bit.WDR_OUTBLD_SPLIT_LUT3 = p_wdr_outbld_split[16 * i + 3];
		wdr_outbld_split.bit.WDR_OUTBLD_SPLIT_LUT4 = p_wdr_outbld_split[16 * i + 4];
		wdr_outbld_split.bit.WDR_OUTBLD_SPLIT_LUT5 = p_wdr_outbld_split[16 * i + 5];
		wdr_outbld_split.bit.WDR_OUTBLD_SPLIT_LUT6 = p_wdr_outbld_split[16 * i + 6];
		wdr_outbld_split.bit.WDR_OUTBLD_SPLIT_LUT7 = p_wdr_outbld_split[16 * i + 7];
		wdr_outbld_split.bit.WDR_OUTBLD_SPLIT_LUT8 = p_wdr_outbld_split[16 * i + 8];
		wdr_outbld_split.bit.WDR_OUTBLD_SPLIT_LUT9 = p_wdr_outbld_split[16 * i + 9];
		wdr_outbld_split.bit.WDR_OUTBLD_SPLIT_LUT10 = p_wdr_outbld_split[16 * i + 10];
		wdr_outbld_split.bit.WDR_OUTBLD_SPLIT_LUT11 = p_wdr_outbld_split[16 * i + 11];
		wdr_outbld_split.bit.WDR_OUTBLD_SPLIT_LUT12 = p_wdr_outbld_split[16 * i + 12];
		wdr_outbld_split.bit.WDR_OUTBLD_SPLIT_LUT13 = p_wdr_outbld_split[16 * i + 13];
		wdr_outbld_split.bit.WDR_OUTBLD_SPLIT_LUT14 = p_wdr_outbld_split[16 * i + 14];
		wdr_outbld_split.bit.WDR_OUTBLD_SPLIT_LUT15 = p_wdr_outbld_split[16 * i + 15];
		DCE_SETREG((WDR_OUTPUT_BLENDING_CURVE_REGISTER8_OFS + (i * 4)), wdr_outbld_split.reg);
	}
}

void dce_set_wdr_outbld_val(UINT32 *p_wdr_outbld_val)
{
	UINT32 i = 0;

	T_WDR_OUTPUT_BLENDING_CURVE_REGISTER10 wdr_outbld_val;

	for (i = 0; i < 32; i++) {
		wdr_outbld_val.reg = DCE_GETREG(WDR_OUTPUT_BLENDING_CURVE_REGISTER10_OFS + (i * 4));
		wdr_outbld_val.bit.WDR_OUTBLD_VAL_LUT0 = p_wdr_outbld_val[2 * i];
		wdr_outbld_val.bit.WDR_OUTBLD_VAL_LUT1 = p_wdr_outbld_val[2 * i + 1];
		DCE_SETREG((WDR_OUTPUT_BLENDING_CURVE_REGISTER10_OFS + (i * 4)), wdr_outbld_val.reg);
	}

	wdr_outbld_val.reg = DCE_GETREG(WDR_OUTPUT_BLENDING_CURVE_REGISTER42_OFS);
	wdr_outbld_val.bit.WDR_OUTBLD_VAL_LUT0 = p_wdr_outbld_val[64];
	DCE_SETREG(WDR_OUTPUT_BLENDING_CURVE_REGISTER42_OFS, wdr_outbld_val.reg);
}

void dce_set_hist_sel(DCE_HIST_SEL hist_sel)
{
	T_DCE_FUNCTION_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_FUNCTION_REGISTER_OFS);
	local_reg.bit.HISTOGRAM_SEL = hist_sel;

	DCE_SETREG(DCE_FUNCTION_REGISTER_OFS, local_reg.reg);
}

void dce_set_hist_step(UINT32 hstep, UINT32 vstep)
{
	T_HISTOGRAM_REGISTER0 local_reg;

	local_reg.reg = DCE_GETREG(HISTOGRAM_REGISTER0_OFS);
	local_reg.bit.HISTOGRAM_H_STEP = hstep;
	local_reg.bit.HISTOGRAM_V_STEP = vstep;

	DCE_SETREG(HISTOGRAM_REGISTER0_OFS, local_reg.reg);
}

void dce_get_hist_rslt(DCE_HIST_RSLT *p_result)
{
	UINT32 addr_ofs, i = 0;
	T_HISTOGRAM_REGISTER1 local_reg0;
	T_DCE_FUNCTION_REGISTER local_reg1;

	UINT16 *p_stcs = p_result->hist_stcs;

	for (i = 0; i < 64; i++) {
		addr_ofs = i * 4;
		local_reg0.reg = DCE_GETREG(HISTOGRAM_REGISTER1_OFS + addr_ofs);

		*(p_stcs + 2 * i) = (local_reg0.bit.HISTOGRAM_BIN0);
		*(p_stcs + 2 * i + 1) = (local_reg0.bit.HISTOGRAM_BIN1);
	}
	local_reg1.reg = DCE_GETREG(DCE_FUNCTION_REGISTER_OFS);
	p_result->enable = local_reg1.bit.HISTOGRAM_EN;
	p_result->hist_sel = (DCE_HIST_SEL)local_reg1.bit.HISTOGRAM_SEL;

}

void dce_set_raw_fmt(DCE_RAW_FMT raw_fmt)
{
	T_COLOR_INTERPOLATION_REGISTER_0 local_reg;

	local_reg.reg = DCE_GETREG(COLOR_INTERPOLATION_REGISTER_0_OFS);
	local_reg.bit.BAYER_FORMAT = raw_fmt;

	DCE_SETREG(COLOR_INTERPOLATION_REGISTER_0_OFS, local_reg.reg);
}

void dce_set_cfa_pat(DCE_CFA_PAT cfa_pat)
{
	T_COLOR_INTERPOLATION_REGISTER_0 local_reg;

	local_reg.reg = DCE_GETREG(COLOR_INTERPOLATION_REGISTER_0_OFS);
	local_reg.bit.CFAPAT = cfa_pat;

	DCE_SETREG(COLOR_INTERPOLATION_REGISTER_0_OFS, local_reg.reg);
}

void dce_set_color_gain(DCE_CFA_CGAIN *color_gain)
{
	T_COLOR_GAIN_REGISTER_1 local_reg0;
	T_COLOR_GAIN_REGISTER_2 local_reg1;

	local_reg0.reg = DCE_GETREG(COLOR_GAIN_REGISTER_1_OFS);
	local_reg0.bit.CGAIN_R = color_gain->gain_r;
	local_reg0.bit.CGAIN_G = color_gain->gain_g;

	local_reg1.reg = DCE_GETREG(COLOR_GAIN_REGISTER_2_OFS);
	local_reg1.bit.CGAIN_B = color_gain->gain_b;
	local_reg1.bit.CGAIN_RANGE = color_gain->gain_range;

	DCE_SETREG(COLOR_GAIN_REGISTER_1_OFS, local_reg0.reg);
	DCE_SETREG(COLOR_GAIN_REGISTER_2_OFS, local_reg1.reg);
}

void dce_set_d2d_rand(BOOL rand, BOOL randrst)
{
	T_DCE_FUNCTION_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_FUNCTION_REGISTER_OFS);
	local_reg.bit.D2DRAND = rand;
	local_reg.bit.D2DRAND_RST = randrst;

	DCE_SETREG(DCE_FUNCTION_REGISTER_OFS, local_reg.reg);
}

void dce_set_d2d_fmt(DCE_D2D_FMT format)
{
	T_DCE_FUNCTION_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_FUNCTION_REGISTER_OFS);
	local_reg.bit.D2DFMT = format;

	DCE_SETREG(DCE_FUNCTION_REGISTER_OFS, local_reg.reg);
}

void dce_set_yuv2rgb_fmt(YUV2RGB_FMT format)
{
	T_DCE_FUNCTION_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_FUNCTION_REGISTER_OFS);
	local_reg.bit.YUV2RGB_FMT = format;

	DCE_SETREG(DCE_FUNCTION_REGISTER_OFS, local_reg.reg);
}

void dce_set_img_size(DCE_IMG_SIZE *p_size)
{
	T_DCE_INPUT_SIZE_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_INPUT_SIZE_REGISTER_OFS);
	local_reg.bit.DCE_HSIZEIN = (p_size->h_size) >> 2;
	local_reg.bit.DCE_VSIZEIN = (p_size->v_size) >> 1;

	DCE_SETREG(DCE_INPUT_SIZE_REGISTER_OFS, local_reg.reg);
}

void dce_set_y_in_addr(UINT32 addr)
{
	T_DMA_INPUT_Y_CHANNEL_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DMA_INPUT_Y_CHANNEL_REGISTER_OFS);
	local_reg.bit.DRAM_SAIY = addr >> 2;

	DCE_SETREG(DMA_INPUT_Y_CHANNEL_REGISTER_OFS, local_reg.reg);
}

void dce_set_y_in_lofs(UINT32 lineoffset)
{
	T_DMA_INPUT_Y_CHANNEL_LINE_OFFSET_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DMA_INPUT_Y_CHANNEL_LINE_OFFSET_REGISTER_OFS);
	local_reg.bit.DRAM_OFSIY = lineoffset >> 2;

	DCE_SETREG(DMA_INPUT_Y_CHANNEL_LINE_OFFSET_REGISTER_OFS, local_reg.reg);
}

void dce_set_uv_in_addr(UINT32 addr)
{
	T_DMA_INPUT_UV_CHANNEL_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DMA_INPUT_UV_CHANNEL_REGISTER_OFS);
	local_reg.bit.DRAM_SAIUV = addr >> 2;

	DCE_SETREG(DMA_INPUT_UV_CHANNEL_REGISTER_OFS, local_reg.reg);
}

void dce_set_uv_in_lofs(UINT32 lineoffset)
{
	T_DMA_INPUT_UV_CHANNEL_LINE_OFFSET_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DMA_INPUT_UV_CHANNEL_LINE_OFFSET_REGISTER_OFS);
	local_reg.bit.DRAM_OFSIUV = lineoffset >> 2;

	DCE_SETREG(DMA_INPUT_UV_CHANNEL_LINE_OFFSET_REGISTER_OFS, local_reg.reg);
}

void dce_set_wdr_subin_addr(UINT32 addr)
{
	T_WDR_SUBIMAGE_REGISTER_1 local_reg;

	local_reg.reg = DCE_GETREG(WDR_SUBIMAGE_REGISTER_1_OFS);
	local_reg.bit.WDR_SUBIMG_DRAMSAI = addr >> 2;

	DCE_SETREG(WDR_SUBIMAGE_REGISTER_1_OFS, local_reg.reg);
}

void dce_set_wdr_subin_lofs(UINT32 lineoffset)
{
	T_WDR_SUBIMAGE_REGISTER_2 local_reg;

	local_reg.reg = DCE_GETREG(WDR_SUBIMAGE_REGISTER_2_OFS);
	local_reg.bit.WDR_SUBIMG_LOFSI = lineoffset >> 2;

	DCE_SETREG(WDR_SUBIMAGE_REGISTER_2_OFS, local_reg.reg);
}

void dce_set_y_out_addr(UINT32 addr)
{
	T_DMA_OUTPUT_Y_CHANNEL_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DMA_OUTPUT_Y_CHANNEL_REGISTER_OFS);
	local_reg.bit.DRAM_SAOY = addr >> 2;

	DCE_SETREG(DMA_OUTPUT_Y_CHANNEL_REGISTER_OFS, local_reg.reg);
}

void dce_set_y_out_lofs(UINT32 lineoffset)
{
	T_DMA_OUTPUT_Y_CHANNEL_LINE_OFFSET_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DMA_OUTPUT_Y_CHANNEL_LINE_OFFSET_REGISTER_OFS);
	local_reg.bit.DRAM_OFSOY = lineoffset >> 2;

	DCE_SETREG(DMA_OUTPUT_Y_CHANNEL_LINE_OFFSET_REGISTER_OFS, local_reg.reg);
}

void dce_set_uv_out_addr(UINT32 addr)
{
	T_DMA_OUTPUT_UV_CHANNEL_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DMA_OUTPUT_UV_CHANNEL_REGISTER_OFS);
	local_reg.bit.DRAM_SAOUV = addr >> 2;

	DCE_SETREG(DMA_OUTPUT_UV_CHANNEL_REGISTER_OFS, local_reg.reg);
}

void dce_set_uv_out_lofs(UINT32 lineoffset)
{
	T_DMA_OUTPUT_UV_CHANNEL_LINE_OFFSET_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DMA_OUTPUT_UV_CHANNEL_LINE_OFFSET_REGISTER_OFS);
	local_reg.bit.DRAM_OFSOUV = lineoffset >> 2;

	DCE_SETREG(DMA_OUTPUT_UV_CHANNEL_LINE_OFFSET_REGISTER_OFS, local_reg.reg);
}

void dce_set_cfa_subout_addr(UINT32 addr)
{
	T_DMA_CFA_IR_PLANE_OUTPUT_CHANNEL_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DMA_CFA_IR_PLANE_OUTPUT_CHANNEL_REGISTER_OFS);
	local_reg.bit.CFA_SUBIMG_DRAMSAO = addr >> 2;

	DCE_SETREG(DMA_CFA_IR_PLANE_OUTPUT_CHANNEL_REGISTER_OFS, local_reg.reg);
}

void dce_set_cfa_subout_lofs(UINT32 lineoffset)
{
	T_DMA_CFA_IR_PLANE_OUTPUT_CHANNEL_LINEOFFSET_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DMA_CFA_IR_PLANE_OUTPUT_CHANNEL_LINEOFFSET_REGISTER_OFS);
	local_reg.bit.CFA_SUBIMG_LOFSO = lineoffset >> 2;

	DCE_SETREG(DMA_CFA_IR_PLANE_OUTPUT_CHANNEL_LINEOFFSET_REGISTER_OFS, local_reg.reg);
}

void dce_set_wdr_subout_addr(UINT32 addr)
{
	T_WDR_SUBIMAGE_REGISTER_3 local_reg;

	local_reg.reg = DCE_GETREG(WDR_SUBIMAGE_REGISTER_3_OFS);
	local_reg.bit.WDR_SUBIMG_DRAMSAO = addr >> 2;

	DCE_SETREG(WDR_SUBIMAGE_REGISTER_3_OFS, local_reg.reg);
}

void dce_set_wdr_subout_lofs(UINT32 lineoffset)
{
	T_WDR_SUBIMAGE_REGISTER_4 local_reg;

	local_reg.reg = DCE_GETREG(WDR_SUBIMAGE_REGISTER_4_OFS);
	local_reg.bit.WDR_SUBIMG_LOFSO = lineoffset >> 2;

	DCE_SETREG(WDR_SUBIMAGE_REGISTER_4_OFS, local_reg.reg);
}

void dce_enable_int(UINT32 intr)
{
	T_DCE_INTERRUPT_ENABLE_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_INTERRUPT_ENABLE_REGISTER_OFS);
	local_reg.reg |= intr;

	DCE_SETREG(DCE_INTERRUPT_ENABLE_REGISTER_OFS, local_reg.reg);
}

void dce_disable_int(UINT32 intr)
{
	T_DCE_INTERRUPT_ENABLE_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_INTERRUPT_ENABLE_REGISTER_OFS);
	local_reg.reg &= (~intr);

	DCE_SETREG(DCE_INTERRUPT_ENABLE_REGISTER_OFS, local_reg.reg);
}

void dce_set_int_en(UINT32 intr)
{
	T_DCE_INTERRUPT_ENABLE_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DCE_INTERRUPT_ENABLE_REGISTER_OFS);
	local_reg.reg &= (~DCE_INT_ALL);
	local_reg.reg |= intr;

	DCE_SETREG(DCE_INTERRUPT_ENABLE_REGISTER_OFS, local_reg.reg);
}

void dce_clear_int(UINT32 intr)
{
	DCE_SETREG(DCE_INTERRUPT_STATUS_REGISTER_OFS, intr);
}

void dce_set_distort_corr_sel(DCE_DC_SEL dc_sel)
{
	T_DCE_FUNCTION_REGISTER LocalReg;

	LocalReg.reg = DCE_GETREG(DCE_FUNCTION_REGISTER_OFS);
	LocalReg.bit.DC_SEL = dc_sel;

	DCE_SETREG(DCE_FUNCTION_REGISTER_OFS, LocalReg.reg);
}


void dce_set_img_center(DCE_IMG_CENT *p_center)
{
	T_GEO_CENTER_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(GEO_CENTER_REGISTER_OFS);
	local_reg.bit.GDC_CENTX = p_center->x_cent;
	local_reg.bit.GDC_CENTY = p_center->y_cent;

	DCE_SETREG(GEO_CENTER_REGISTER_OFS, local_reg.reg);
}

void dce_set_img_ratio(DCE_IMG_RAT *p_ratio)
{
	T_GEO_AXIS_DISTANCE_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(GEO_AXIS_DISTANCE_REGISTER_OFS);
	local_reg.bit.GDC_XDIST = p_ratio->x_dist;
	local_reg.bit.GDC_YDIST = p_ratio->y_dist;

	DCE_SETREG(GEO_AXIS_DISTANCE_REGISTER_OFS, local_reg.reg);
}

void dce_set_dist_norm(DCE_DIST_NORM *p_dist_norm)
{
	T_GEO_DISTANCE_NORMALIZATION_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(GEO_DISTANCE_NORMALIZATION_REGISTER_OFS);
	local_reg.bit.GDC_NORMFACT_SEL = p_dist_norm->nrm_fact_sel;
	local_reg.bit.GDC_NORMFACT_10B = p_dist_norm->nrm_fact_10b;
	local_reg.bit.GDC_NORMFACT = p_dist_norm->nrm_fact;
	local_reg.bit.GDC_NORMBIT = p_dist_norm->nrm_bit;

	DCE_SETREG(GEO_DISTANCE_NORMALIZATION_REGISTER_OFS, local_reg.reg);
}

void dce_set_cac_gain(DCE_CAC_GAIN *p_gain)
{
	T_GEO_ABERRATION_REGISTER0 local_reg1;
	T_GEO_ABERRATION_REGISTER1 local_reg2;

	local_reg1.reg = DCE_GETREG(GEO_ABERRATION_REGISTER0_OFS);
	local_reg1.bit.CAC_RLUTGAIN = p_gain->r_lut_gain;
	local_reg1.bit.CAC_GLUTGAIN = p_gain->g_lut_gain;

	local_reg2.reg = DCE_GETREG(GEO_ABERRATION_REGISTER1_OFS);
	local_reg2.bit.CAC_BLUTGAIN = p_gain->b_lut_gain;

	DCE_SETREG(GEO_ABERRATION_REGISTER0_OFS, local_reg1.reg);
	DCE_SETREG(GEO_ABERRATION_REGISTER1_OFS, local_reg2.reg);
}

void dce_set_cac_sel(DCE_CAC_SEL cac_sel)
{
	T_GEO_ABERRATION_REGISTER1 local_reg;

	local_reg.reg = DCE_GETREG(GEO_ABERRATION_REGISTER1_OFS);
	local_reg.bit.CAC_SEL = cac_sel;

	DCE_SETREG(GEO_ABERRATION_REGISTER1_OFS, local_reg.reg);
}

void dce_set_fov_gain(UINT32 fov_gain)
{
	T_GDC_FOV_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(GDC_FOV_REGISTER_OFS);
	local_reg.bit.GDC_FOVGAIN = fov_gain;

	DCE_SETREG(GDC_FOV_REGISTER_OFS, local_reg.reg);
}

void dce_set_fov_bound_sel(DCE_FOV_SEL fov_sel)
{
	T_GDC_FOV_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(GDC_FOV_REGISTER_OFS);
	local_reg.bit.GDC_FOVBOUND = fov_sel;

	DCE_SETREG(GDC_FOV_REGISTER_OFS, local_reg.reg);
}

void dce_set_fov_bound_rgb(DCE_FOV_RGB *p_fov_rgb)
{
	T_GDC_FOV_BOUNDARY_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(GDC_FOV_BOUNDARY_REGISTER_OFS);
	local_reg.bit.GDC_BOUNDR = p_fov_rgb->r_bound;
	local_reg.bit.GDC_BOUNDG = p_fov_rgb->g_bound;
	local_reg.bit.GDC_BOUNDB = p_fov_rgb->b_bound;

	DCE_SETREG(GDC_FOV_BOUNDARY_REGISTER_OFS, local_reg.reg);
}

void dce_set_cfa_subout_sel(DCE_CFA_SUBOUT_PARAM *p_cfa_subout)
{
	T_DMA_CFA_IR_PLANE_OUTPUT_CHANNEL_LINEOFFSET_REGISTER local_reg0;

	local_reg0.reg = DCE_GETREG(DMA_CFA_IR_PLANE_OUTPUT_CHANNEL_LINEOFFSET_REGISTER_OFS);
	local_reg0.bit.CFA_SUBIMG_CHSEL = p_cfa_subout->subout_ch_sel;
	local_reg0.bit.CFA_SUBIMG_BIT = p_cfa_subout->subout_shiftbit;
	local_reg0.bit.CFA_SUBIMG_BYTE = p_cfa_subout->subout_byte;

	DCE_SETREG(DMA_CFA_IR_PLANE_OUTPUT_CHANNEL_LINEOFFSET_REGISTER_OFS, local_reg0.reg);
}


void dce_set_cfa_interp(DCE_CFA_INTERP *p_cfa_interp)
{
	T_COLOR_INTERPOLATION_REGISTER1 local_reg0;

	local_reg0.reg = DCE_GETREG(COLOR_INTERPOLATION_REGISTER1_OFS);
	local_reg0.bit.CFA_EDGE_DTH = p_cfa_interp->edge_dth;
	local_reg0.bit.CFA_EDGE_DTH2 = p_cfa_interp->edge_dth2;

	DCE_SETREG(COLOR_INTERPOLATION_REGISTER1_OFS, local_reg0.reg);
}

void dce_set_cfa_freq(UINT32 freq_th, UINT32 *p_freq_wt)
{
	T_COLOR_INTERPOLATION_REGISTER3 local_reg0;
	T_COLOR_INTERPOLATION_REGISTER4 local_reg1;
	T_COLOR_INTERPOLATION_REGISTER9 local_reg2;

	local_reg0.reg = DCE_GETREG(COLOR_INTERPOLATION_REGISTER3_OFS);
	local_reg0.bit.CFA_FREQBLEND_LUT0 = p_freq_wt[0];
	local_reg0.bit.CFA_FREQBLEND_LUT1 = p_freq_wt[1];
	local_reg0.bit.CFA_FREQBLEND_LUT2 = p_freq_wt[2];
	local_reg0.bit.CFA_FREQBLEND_LUT3 = p_freq_wt[3];
	local_reg0.bit.CFA_FREQBLEND_LUT4 = p_freq_wt[4];
	local_reg0.bit.CFA_FREQBLEND_LUT5 = p_freq_wt[5];
	local_reg0.bit.CFA_FREQBLEND_LUT6 = p_freq_wt[6];
	local_reg0.bit.CFA_FREQBLEND_LUT7 = p_freq_wt[7];

	local_reg1.reg = DCE_GETREG(COLOR_INTERPOLATION_REGISTER4_OFS);
	local_reg1.bit.CFA_FREQBLEND_LUT8  = p_freq_wt[8];
	local_reg1.bit.CFA_FREQBLEND_LUT9  = p_freq_wt[9];
	local_reg1.bit.CFA_FREQBLEND_LUT10 = p_freq_wt[10];
	local_reg1.bit.CFA_FREQBLEND_LUT11 = p_freq_wt[11];
	local_reg1.bit.CFA_FREQBLEND_LUT12 = p_freq_wt[12];
	local_reg1.bit.CFA_FREQBLEND_LUT13 = p_freq_wt[13];
	local_reg1.bit.CFA_FREQBLEND_LUT14 = p_freq_wt[14];
	local_reg1.bit.CFA_FREQBLEND_LUT15 = p_freq_wt[15];

	local_reg2.reg = DCE_GETREG(COLOR_INTERPOLATION_REGISTER9_OFS);
	local_reg2.bit.CFA_FREQ_TH  = freq_th;

	DCE_SETREG(COLOR_INTERPOLATION_REGISTER3_OFS, local_reg0.reg);
	DCE_SETREG(COLOR_INTERPOLATION_REGISTER4_OFS, local_reg1.reg);
	DCE_SETREG(COLOR_INTERPOLATION_REGISTER9_OFS, local_reg2.reg);
}

void dce_set_cfa_luma_wt(UINT32 *p_luma_wt)
{
	T_COLOR_INTERPOLATION_REGISTER5 local_reg0;
	T_COLOR_INTERPOLATION_REGISTER6 local_reg1;
	T_COLOR_INTERPOLATION_REGISTER7 local_reg2;
	T_COLOR_INTERPOLATION_REGISTER8 local_reg3;
	T_COLOR_INTERPOLATION_REGISTER9 local_reg4;

	local_reg0.reg = DCE_GETREG(COLOR_INTERPOLATION_REGISTER5_OFS);
	local_reg0.bit.CFA_LUMA_WEIGHT0 = p_luma_wt[0];
	local_reg0.bit.CFA_LUMA_WEIGHT1 = p_luma_wt[1];
	local_reg0.bit.CFA_LUMA_WEIGHT2 = p_luma_wt[2];
	local_reg0.bit.CFA_LUMA_WEIGHT3 = p_luma_wt[3];

	local_reg1.reg = DCE_GETREG(COLOR_INTERPOLATION_REGISTER6_OFS);
	local_reg1.bit.CFA_LUMA_WEIGHT4 = p_luma_wt[4];
	local_reg1.bit.CFA_LUMA_WEIGHT5 = p_luma_wt[5];
	local_reg1.bit.CFA_LUMA_WEIGHT6 = p_luma_wt[6];
	local_reg1.bit.CFA_LUMA_WEIGHT7 = p_luma_wt[7];

	local_reg2.reg = DCE_GETREG(COLOR_INTERPOLATION_REGISTER7_OFS);
	local_reg2.bit.CFA_LUMA_WEIGHT8  = p_luma_wt[8];
	local_reg2.bit.CFA_LUMA_WEIGHT9  = p_luma_wt[9];
	local_reg2.bit.CFA_LUMA_WEIGHT10 = p_luma_wt[10];
	local_reg2.bit.CFA_LUMA_WEIGHT11 = p_luma_wt[11];

	local_reg3.reg = DCE_GETREG(COLOR_INTERPOLATION_REGISTER8_OFS);
	local_reg3.bit.CFA_LUMA_WEIGHT12 = p_luma_wt[12];
	local_reg3.bit.CFA_LUMA_WEIGHT13 = p_luma_wt[13];
	local_reg3.bit.CFA_LUMA_WEIGHT14 = p_luma_wt[14];
	local_reg3.bit.CFA_LUMA_WEIGHT15 = p_luma_wt[15];

	local_reg4.reg = DCE_GETREG(COLOR_INTERPOLATION_REGISTER9_OFS);
	local_reg4.bit.CFA_LUMA_WEIGHT16 = p_luma_wt[16];

	DCE_SETREG(COLOR_INTERPOLATION_REGISTER5_OFS, local_reg0.reg);
	DCE_SETREG(COLOR_INTERPOLATION_REGISTER6_OFS, local_reg1.reg);
	DCE_SETREG(COLOR_INTERPOLATION_REGISTER7_OFS, local_reg2.reg);
	DCE_SETREG(COLOR_INTERPOLATION_REGISTER8_OFS, local_reg3.reg);
	DCE_SETREG(COLOR_INTERPOLATION_REGISTER9_OFS, local_reg4.reg);
}

void dce_set_cfa_corr(DCE_CFA_CORR *p_cfa_corr)
{
	T_COLOR_INTERPOLATION_REGISTER2 local_reg;

	local_reg.reg = DCE_GETREG(COLOR_INTERPOLATION_REGISTER2_OFS);
	local_reg.bit.CFA_RBCEN = p_cfa_corr->rb_corr_en;
	local_reg.bit.CFA_RBCTH1 = p_cfa_corr->rb_corr_th1;
	local_reg.bit.CFA_RBCTH2 = p_cfa_corr->rb_corr_th2;

	DCE_SETREG(COLOR_INTERPOLATION_REGISTER2_OFS, local_reg.reg);
}

void dce_set_cfa_fcs(DCE_CFA_FCS *p_fcs)
{
	T_COLOR_INTERPOLATION_REGISTER10 local_reg0;
	T_COLOR_INTERPOLATION_REGISTER11 local_reg1;
	T_COLOR_INTERPOLATION_REGISTER12 local_reg2;

	local_reg0.reg = DCE_GETREG(COLOR_INTERPOLATION_REGISTER10_OFS);
	local_reg0.bit.CFA_FCS_CORING  = p_fcs->fcs_coring;
	local_reg0.bit.CFA_FCS_WEIGHT  = p_fcs->fcs_weight;
	local_reg0.bit.CFA_FCS_DIRSEL  = p_fcs->fcs_dirsel;

	local_reg1.reg = DCE_GETREG(COLOR_INTERPOLATION_REGISTER11_OFS);
	local_reg1.bit.CFA_FCS_STRENGTH0 = p_fcs->p_fcs_strength[0];
	local_reg1.bit.CFA_FCS_STRENGTH1 = p_fcs->p_fcs_strength[1];
	local_reg1.bit.CFA_FCS_STRENGTH2 = p_fcs->p_fcs_strength[2];
	local_reg1.bit.CFA_FCS_STRENGTH3 = p_fcs->p_fcs_strength[3];
	local_reg1.bit.CFA_FCS_STRENGTH4 = p_fcs->p_fcs_strength[4];
	local_reg1.bit.CFA_FCS_STRENGTH5 = p_fcs->p_fcs_strength[5];
	local_reg1.bit.CFA_FCS_STRENGTH6 = p_fcs->p_fcs_strength[6];
	local_reg1.bit.CFA_FCS_STRENGTH7 = p_fcs->p_fcs_strength[7];

	local_reg2.reg = DCE_GETREG(COLOR_INTERPOLATION_REGISTER12_OFS);
	local_reg2.bit.CFA_FCS_STRENGTH8  = p_fcs->p_fcs_strength[8];
	local_reg2.bit.CFA_FCS_STRENGTH9  = p_fcs->p_fcs_strength[9];
	local_reg2.bit.CFA_FCS_STRENGTH10 = p_fcs->p_fcs_strength[10];
	local_reg2.bit.CFA_FCS_STRENGTH11 = p_fcs->p_fcs_strength[11];
	local_reg2.bit.CFA_FCS_STRENGTH12 = p_fcs->p_fcs_strength[12];
	local_reg2.bit.CFA_FCS_STRENGTH13 = p_fcs->p_fcs_strength[13];
	local_reg2.bit.CFA_FCS_STRENGTH14 = p_fcs->p_fcs_strength[14];
	local_reg2.bit.CFA_FCS_STRENGTH15 = p_fcs->p_fcs_strength[15];

	DCE_SETREG(COLOR_INTERPOLATION_REGISTER10_OFS, local_reg0.reg);
	DCE_SETREG(COLOR_INTERPOLATION_REGISTER11_OFS, local_reg1.reg);
	DCE_SETREG(COLOR_INTERPOLATION_REGISTER12_OFS, local_reg2.reg);
}

void dce_set_cfa_ir_hfc(DCE_CFA_IR_HFC *p_ir_hfc)
{
	T_RGBIR_COLOR_INTERPOLATION_REGISTER0 local_reg0;
	T_RGBIR_COLOR_INTERPOLATION_REGISTER1 local_reg1;

	local_reg0.reg = DCE_GETREG(RGBIR_COLOR_INTERPOLATION_REGISTER0_OFS);
	local_reg0.bit.CFA_RGBIR_CL_EN = p_ir_hfc->ir_cl_check_en;
	local_reg0.bit.CFA_RGBIR_HF_EN = p_ir_hfc->ir_hf_check_en;
	local_reg0.bit.CFA_RGBIR_AVG_MODE = p_ir_hfc->ir_average_mode;
	local_reg0.bit.CFA_RGBIR_CL_SEL = p_ir_hfc->ir_cl_sel;
	local_reg0.bit.CFA_RGBIR_CL_THR = p_ir_hfc->ir_cl_th;
	local_reg0.bit.CFA_RGBIR_GEDGE_TH = p_ir_hfc->ir_g_edge_th;
	local_reg0.bit.CFA_RGBIR_RB_CSTRENGTH = p_ir_hfc->ir_rb_cstrength;

	local_reg1.reg = DCE_GETREG(RGBIR_COLOR_INTERPOLATION_REGISTER1_OFS);
	local_reg1.bit.CFA_RGBIR_HF_GTHR = p_ir_hfc->ir_hf_gth;
	local_reg1.bit.CFA_RGBIR_HF_DIFF = p_ir_hfc->ir_hf_diff;
	local_reg1.bit.CFA_RGBIR_HF_ETHR = p_ir_hfc->ir_hf_eth;


	DCE_SETREG(RGBIR_COLOR_INTERPOLATION_REGISTER0_OFS, local_reg0.reg);
	DCE_SETREG(RGBIR_COLOR_INTERPOLATION_REGISTER1_OFS, local_reg1.reg);
}

void dce_get_ir_sub(DCE_CFA_IR *p_ir_sub)
{
	T_RGBIR_COLOR_INTERPOLATION_REGISTER2 local_reg0;
	T_RGBIR_COLOR_INTERPOLATION_REGISTER3 local_reg1;
	T_CFA_SATURATION_GAIN local_reg2;

	local_reg0.reg = DCE_GETREG(RGBIR_COLOR_INTERPOLATION_REGISTER2_OFS);
	p_ir_sub->ir_sub_r = local_reg0.bit.IRSUB_R;
	p_ir_sub->ir_sub_g = local_reg0.bit.IRSUB_G;

	local_reg1.reg = DCE_GETREG(RGBIR_COLOR_INTERPOLATION_REGISTER3_OFS);
	p_ir_sub->ir_sub_b = local_reg1.bit.IRSUB_B;
	p_ir_sub->ir_sub_wt_lb = local_reg1.bit.IRSUB_WEIGHT_LB;
	p_ir_sub->ir_sub_th = local_reg1.bit.IRSUB_TH;
	p_ir_sub->ir_sub_range = local_reg1.bit.IRSUB_WEIGHT_RANGE;

	local_reg2.reg = DCE_GETREG(CFA_SATURATION_GAIN_OFS);
	p_ir_sub->ir_sat_gain = local_reg2.bit.CFA_SAT_GAIN;


}
void dce_set_ir_sub(DCE_CFA_IR *p_ir_sub)
{
	T_RGBIR_COLOR_INTERPOLATION_REGISTER2 local_reg0;
	T_RGBIR_COLOR_INTERPOLATION_REGISTER3 local_reg1;
	T_CFA_SATURATION_GAIN local_reg2;

	local_reg0.reg = DCE_GETREG(RGBIR_COLOR_INTERPOLATION_REGISTER2_OFS);
	local_reg0.bit.IRSUB_R = p_ir_sub->ir_sub_r;
	local_reg0.bit.IRSUB_G = p_ir_sub->ir_sub_g;

	local_reg1.reg = DCE_GETREG(RGBIR_COLOR_INTERPOLATION_REGISTER3_OFS);
	local_reg1.bit.IRSUB_B = p_ir_sub->ir_sub_b;
	local_reg1.bit.IRSUB_WEIGHT_LB = p_ir_sub->ir_sub_wt_lb;
	local_reg1.bit.IRSUB_TH = p_ir_sub->ir_sub_th;
	local_reg1.bit.IRSUB_WEIGHT_RANGE = p_ir_sub->ir_sub_range;

	local_reg2.reg = DCE_GETREG(CFA_SATURATION_GAIN_OFS);
	local_reg2.bit.CFA_SAT_GAIN = p_ir_sub->ir_sat_gain;


	DCE_SETREG(RGBIR_COLOR_INTERPOLATION_REGISTER2_OFS, local_reg0.reg);
	DCE_SETREG(RGBIR_COLOR_INTERPOLATION_REGISTER3_OFS, local_reg1.reg);
	DCE_SETREG(CFA_SATURATION_GAIN_OFS, local_reg2.reg);
}

void dce_set_pink_reduct(DCE_CFA_PINKR *p_pink_reduct)
{
	T_CFA_SATURATION_GAIN local_reg0;
	T_PINK_REDUCTION_REGISTER local_reg1;

	local_reg0.reg = DCE_GETREG(CFA_SATURATION_GAIN_OFS);
	local_reg0.bit.PINKR_MODE = p_pink_reduct->pink_rd_mode;

	local_reg1.reg = DCE_GETREG(PINK_REDUCTION_REGISTER_OFS);
	local_reg1.bit.PINKR_THR1 = p_pink_reduct->pink_rd_th1;
	local_reg1.bit.PINKR_THR2 = p_pink_reduct->pink_rd_th2;
	local_reg1.bit.PINKR_THR3 = p_pink_reduct->pink_rd_th3;
	local_reg1.bit.PINKR_THR4 = p_pink_reduct->pink_rd_th4;

	DCE_SETREG(CFA_SATURATION_GAIN_OFS, local_reg0.reg);
	DCE_SETREG(PINK_REDUCTION_REGISTER_OFS, local_reg1.reg);
}

void dce_set_h_stp(UINT32 *p_hstp)
{
	T_DCE_HORIZONTAL_STRIPE_REGISTER0 local_reg0;
	T_DCE_HORIZONTAL_STRIPE_REGISTER1 local_reg1;
	T_DCE_HORIZONTAL_STRIPE_REGISTER2 local_reg2;
	T_DCE_HORIZONTAL_STRIPE_REGISTER3 local_reg3;
	T_DCE_HORIZONTAL_STRIPE_REGISTER4 local_reg4;
	T_DCE_HORIZONTAL_STRIPE_REGISTER5 local_reg5;

	local_reg0.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER0_OFS);
	local_reg0.bit.HSTP0 = p_hstp[0] >> 2;
	local_reg0.bit.HSTP1 = p_hstp[1] >> 2;
	local_reg0.bit.HSTP2 = p_hstp[2] >> 2;

	local_reg1.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER1_OFS);
	local_reg1.bit.HSTP3 = p_hstp[3] >> 2;
	local_reg1.bit.HSTP4 = p_hstp[4] >> 2;
	local_reg1.bit.HSTP5 = p_hstp[5] >> 2;

	local_reg2.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER2_OFS);
	local_reg2.bit.HSTP6 = p_hstp[6] >> 2;
	local_reg2.bit.HSTP7 = p_hstp[7] >> 2;
	local_reg2.bit.HSTP8 = p_hstp[8] >> 2;

	local_reg3.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER3_OFS);
	local_reg3.bit.HSTP9 = p_hstp[9] >> 2;
	local_reg3.bit.HSTP10 = p_hstp[10] >> 2;
	local_reg3.bit.HSTP11 = p_hstp[11] >> 2;

	local_reg4.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER4_OFS);
	local_reg4.bit.HSTP12 = p_hstp[12] >> 2;
	local_reg4.bit.HSTP13 = p_hstp[13] >> 2;
	local_reg4.bit.HSTP14 = p_hstp[14] >> 2;

	local_reg5.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER5_OFS);
	local_reg5.bit.HSTP15 = p_hstp[15] >> 2;

	DCE_SETREG(DCE_HORIZONTAL_STRIPE_REGISTER0_OFS, local_reg0.reg);
	DCE_SETREG(DCE_HORIZONTAL_STRIPE_REGISTER1_OFS, local_reg1.reg);
	DCE_SETREG(DCE_HORIZONTAL_STRIPE_REGISTER2_OFS, local_reg2.reg);
	DCE_SETREG(DCE_HORIZONTAL_STRIPE_REGISTER3_OFS, local_reg3.reg);
	DCE_SETREG(DCE_HORIZONTAL_STRIPE_REGISTER4_OFS, local_reg4.reg);
	DCE_SETREG(DCE_HORIZONTAL_STRIPE_REGISTER5_OFS, local_reg5.reg);
}

void dce_set_hstripe_inc_dec(DCE_HSTP_INCDEC *p_inc_dec)
{
	T_DCE_HORIZONTAL_STRIPE_REGISTER6 local_reg;

	local_reg.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER6_OFS);
	local_reg.bit.HSTP_MAXINC = p_inc_dec->max_inc;
	local_reg.bit.HSTP_MAXDEC = p_inc_dec->max_dec;

	DCE_SETREG(DCE_HORIZONTAL_STRIPE_REGISTER6_OFS, local_reg.reg);
}

void dce_set_hstripe_in_start(DCE_STP_INST in_start)
{
	T_DCE_HORIZONTAL_STRIPE_REGISTER6 local_reg;

	local_reg.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER6_OFS);
	local_reg.bit.HSTP_INST = in_start;

	DCE_SETREG(DCE_HORIZONTAL_STRIPE_REGISTER6_OFS, local_reg.reg);
}

void dce_set_ipe_hstpout_ovlp_sel(DCE_HSTP_IPEOLAP_SEL out_ovlp_sel)
{
	T_DCE_HORIZONTAL_STRIPE_REGISTER6 local_reg;

	local_reg.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER6_OFS);
	local_reg.bit.HSTP_IPEOLAP_SEL = out_ovlp_sel;

	DCE_SETREG(DCE_HORIZONTAL_STRIPE_REGISTER6_OFS, local_reg.reg);
}

void dce_set_ime_hstpout_ovlp_sel(DCE_HSTP_IMEOLAP_SEL out_ovlp_sel)
{
	T_DCE_HORIZONTAL_STRIPE_REGISTER6 local_reg;

	local_reg.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER6_OFS);
	local_reg.bit.HSTP_IMEOLAP_SEL = out_ovlp_sel;

	DCE_SETREG(DCE_HORIZONTAL_STRIPE_REGISTER6_OFS, local_reg.reg);
}

void dce_set_ime_hstpout_ovlp_val(UINT32 out_ovlp_val)
{
	T_DCE_HORIZONTAL_STRIPE_REGISTER6 local_reg;

	local_reg.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER6_OFS);
	local_reg.bit.HSTP_IMEOLAP = out_ovlp_val - 1;

	DCE_SETREG(DCE_HORIZONTAL_STRIPE_REGISTER6_OFS, local_reg.reg);
}

void dce_set_img_crop(DCE_OUT_CROP *p_crop)
{
	T_OUTPUT_CROP_REGISTER_0 local_reg1;
	T_OUTPUT_CROP_REGISTER_1 local_reg2;

	local_reg1.reg = DCE_GETREG(OUTPUT_CROP_REGISTER_0_OFS);
	local_reg1.bit.CROP_HSIZE = (p_crop->h_size) >> 2;

	local_reg2.reg = DCE_GETREG(OUTPUT_CROP_REGISTER_1_OFS);
	local_reg2.bit.CROP_HSTART = p_crop->h_start;

	DCE_SETREG(OUTPUT_CROP_REGISTER_0_OFS, local_reg1.reg);
	DCE_SETREG(OUTPUT_CROP_REGISTER_1_OFS, local_reg2.reg);
}

void dce_set_dma_burst(DCE_DMA_BURST in_burst, DCE_DMA_BURST out_burst)
{
	T_DMA_BURST_LENGTH_REGISTER local_reg;

	local_reg.reg = DCE_GETREG(DMA_BURST_LENGTH_REGISTER_OFS);
	local_reg.bit.DMA_IN_BURST = in_burst;
	local_reg.bit.DMA_OUT_BURST = out_burst;

	DCE_SETREG(DMA_BURST_LENGTH_REGISTER_OFS, local_reg.reg);
}

void dce_set_sram_control(UINT32 sram_ctrl)
{
	DCE_SETREG(SRAM_CONTROL_REGISTER_OFS, sram_ctrl);
}

void dce_set_g_geo_lut(UINT32 *p_lut)
{
	UINT32 i = 0;

	T_GEOMETRIC_LOOK_UP_TABLEG1 geo_lut_tmp;

	for (i = 0; i < 32; i++) {
		geo_lut_tmp.reg = 0x0;

		geo_lut_tmp.bit.GDC_LUTG0 = p_lut[2 * i];
		geo_lut_tmp.bit.GDC_LUTG1 = p_lut[2 * i + 1];

		DCE_SETREG((GEOMETRIC_LOOK_UP_TABLEG1_OFS + (i * 4)), geo_lut_tmp.reg);
	}

	geo_lut_tmp.reg = 0;

	geo_lut_tmp.bit.GDC_LUTG0 = p_lut[64];
	geo_lut_tmp.bit.GDC_LUTG1 = 0;

	DCE_SETREG(GEOMETRIC_LOOK_UP_TABLEG33_OFS, geo_lut_tmp.reg);
}

void dce_set_r_cac_lut(INT32 *p_lut)
{
	UINT32 i = 0;

	T_GEOMETRIC_LOOK_UP_TABLER1 geo_lut_tmp;

	for (i = 0; i < 32; i++) {
		geo_lut_tmp.reg = 0x0;

		geo_lut_tmp.bit.CAC_LUTR0 = p_lut[2 * i];
		geo_lut_tmp.bit.CAC_LUTR1 = p_lut[2 * i + 1];

		DCE_SETREG((GEOMETRIC_LOOK_UP_TABLER1_OFS + (i * 4)), geo_lut_tmp.reg);
	}

	geo_lut_tmp.reg = 0;

	geo_lut_tmp.bit.CAC_LUTR0 = p_lut[64];
	geo_lut_tmp.bit.CAC_LUTR1 = 0;

	DCE_SETREG(GEOMETRIC_LOOK_UP_TABLER33_OFS, geo_lut_tmp.reg);
}

void dce_set_b_cac_lut(INT32 *p_lut)
{
	UINT32 i = 0;

	T_GEOMETRIC_LOOK_UP_TABLEB1 geo_lut_tmp;

	for (i = 0; i < 32; i++) {
		geo_lut_tmp.reg = 0x0;

		geo_lut_tmp.bit.CAC_LUTB0 = p_lut[2 * i];
		geo_lut_tmp.bit.CAC_LUTB1 = p_lut[2 * i + 1];

		DCE_SETREG((GEOMETRIC_LOOK_UP_TABLEB1_OFS + (i * 4)), geo_lut_tmp.reg);
	}

	geo_lut_tmp.reg = 0;

	geo_lut_tmp.bit.CAC_LUTB0 = p_lut[64];
	geo_lut_tmp.bit.CAC_LUTB1 = 0;

	DCE_SETREG(GEOMETRIC_LOOK_UP_TABLEB33_OFS, geo_lut_tmp.reg);
}


void dce_set_back_rsv_line(UINT32 rsv_line)
{
	T_DCE_HORIZONTAL_STRIPE_REGISTER5 local_reg;

	local_reg.reg = DCE_GETREG(DCE_HORIZONTAL_STRIPE_REGISTER5_OFS);
	local_reg.bit.LBUF_BACK_RSV_LINE = rsv_line;

	DCE_SETREG(DCE_HORIZONTAL_STRIPE_REGISTER5_OFS, local_reg.reg);
}

void dce_set_2dlut_in_addr(UINT32 addr)
{
	T_DMA_LUT2D_IN_ADDRESS local_reg;

	local_reg.reg = DCE_GETREG(DMA_LUT2D_IN_ADDRESS_OFS);
	local_reg.bit.DRAM_SAI2DLUT = addr >> 2;

	DCE_SETREG(DMA_LUT2D_IN_ADDRESS_OFS, local_reg.reg);
}

void dce_set_2dlut_ofs(DCE_2DLUT_OFS *p_lut_ofs)
{
	T_LUT2D_REGISTER1 LocalReg1;
	T_LUT2D_REGISTER2 LocalReg2;

	LocalReg1.reg = DCE_GETREG(LUT2D_REGISTER1_OFS);
	LocalReg1.bit.LUT2D_XOFS_INT = p_lut_ofs->x_ofs_int;
	LocalReg1.bit.LUT2D_XOFS_FRAC = p_lut_ofs->x_ofs_frac;

	LocalReg2.reg = DCE_GETREG(LUT2D_REGISTER2_OFS);
	LocalReg2.bit.LUT2D_YOFS_INT = p_lut_ofs->y_ofs_int;
	LocalReg2.bit.LUT2D_YOFS_FRAC = p_lut_ofs->y_ofs_frac;

	DCE_SETREG(LUT2D_REGISTER1_OFS, LocalReg1.reg);
	DCE_SETREG(LUT2D_REGISTER2_OFS, LocalReg2.reg);
}

void dce_set_2dlut_num(DCE_2DLUT_NUM lut_num)
{
	T_LUT2D_REGISTER4 local_reg;

	local_reg.reg = DCE_GETREG(LUT2D_REGISTER4_OFS);
	local_reg.bit.LUT2D_NUMSEL = lut_num;

	DCE_SETREG(LUT2D_REGISTER4_OFS, local_reg.reg);
}

void dce_set_2dlut_ymin_auto_en(UINT32 top_ymin_auto_en)
{
	T_LUT2D_REGISTER4 local_reg;

	local_reg.reg = DCE_GETREG(LUT2D_REGISTER4_OFS);
	local_reg.bit.LUT2D_TOP_YMIN_AUTO_EN = top_ymin_auto_en;

	DCE_SETREG(LUT2D_REGISTER4_OFS, local_reg.reg);
}

void dce_set_2dlut_scale_fact(DCE_2DLUT_SCALE *p_factor)
{
	T_LUT2D_REGISTER3 LocalReg1;
	T_LUT2D_REGISTER4 LocalReg2;

	LocalReg1.reg = DCE_GETREG(LUT2D_REGISTER3_OFS);
	LocalReg1.bit.LUT2D_HFACT = p_factor->h_scale_fact;
	DCE_SETREG(LUT2D_REGISTER3_OFS, LocalReg1.reg);

	LocalReg2.reg = DCE_GETREG(LUT2D_REGISTER4_OFS);
	LocalReg2.bit.LUT2D_VFACT = p_factor->v_scale_fact;
	DCE_SETREG(LUT2D_REGISTER4_OFS, LocalReg2.reg);
}


#ifdef __KERNEL__
EXPORT_SYMBOL(dce_open);
EXPORT_SYMBOL(dce_close);
EXPORT_SYMBOL(dce_start_linkedlist);
EXPORT_SYMBOL(dce_terminate_linkedlist);
EXPORT_SYMBOL(dce_start);
EXPORT_SYMBOL(dce_start_fend_load);
EXPORT_SYMBOL(dce_pause);
EXPORT_SYMBOL(dce_set_mode);
//EXPORT_SYMBOL(dce_change_all);
EXPORT_SYMBOL(dce_change_int_en);
EXPORT_SYMBOL(dce_change_func_en);
EXPORT_SYMBOL(dce_change_operation);
EXPORT_SYMBOL(dce_change_img_in_size);
EXPORT_SYMBOL(dce_change_img_out_crop);
EXPORT_SYMBOL(dce_change_output_info);
EXPORT_SYMBOL(dce_change_input_info);
EXPORT_SYMBOL(dce_change_cfa_interp);
EXPORT_SYMBOL(dce_change_cfa_subout);
EXPORT_SYMBOL(dce_change_wdr);
EXPORT_SYMBOL(dce_change_wdr_subout);
EXPORT_SYMBOL(dce_change_histogram);
EXPORT_SYMBOL(dce_change_wdr_dither);
EXPORT_SYMBOL(dce_change_2dlut);
EXPORT_SYMBOL(dce_change_distort_corr_sel);
EXPORT_SYMBOL(dce_change_img_center);
EXPORT_SYMBOL(dce_change_gdc_cac_digi_zoom);
EXPORT_SYMBOL(dce_change_gdc_cac_opti_zoom);
EXPORT_SYMBOL(dce_change_fov);
EXPORT_SYMBOL(dce_change_h_stripe);
EXPORT_SYMBOL(dce_change_d2d_in_out);
EXPORT_SYMBOL(dce_change_sram_mode);
EXPORT_SYMBOL(dce_get_hist_rslt);
EXPORT_SYMBOL(dce_change_dram_out_mode);
EXPORT_SYMBOL(dce_change_dram_single_ch_en);

#endif
