
#if defined (__KERNEL__)

//#include <linux/version.h>
//#include <linux/module.h>
//#include <linux/seq_file.h>
//#include <linux/proc_fs.h>
//#include <linux/bitops.h>
//#include <linux/interrupt.h>
//#include <linux/mm.h>
//#include <asm/uaccess.h>
//#include <asm/atomic.h>
//#include <asm/io.h>

#elif defined (__FREERTOS)

#else

#endif

//#include "frammap/frammap_if.h"
#include "kdrv_videoprocess/kdrv_vpe.h"

#include "vpe_int.h"
#include "vpe_ll_base.h"
#include "vpe_config_base.h"
#include "vpe_sca_base.h"
#include "kdrv_vpe_int.h"

static void vpe_set_in_src_col_reg(uint32_t col_idx, VPE_COL_SZ_CFG *p_col_cfg)
{
	if (p_col_cfg != NULL) {
		switch (col_idx) {
		case 0:
			vpeg->reg_512.bit.cl0_proc_width   = p_col_cfg->proc_width;
			vpeg->reg_512.bit.cl0_proc_x_start = p_col_cfg->proc_x_start;
			vpeg->reg_518.bit.cl0_col_x_start  = p_col_cfg->col_x_start;
			break;

		case 1:
			vpeg->reg_520.bit.cl1_proc_width   = p_col_cfg->proc_width;
			vpeg->reg_520.bit.cl1_proc_x_start = p_col_cfg->proc_x_start;
			vpeg->reg_526.bit.cl1_col_x_start  = p_col_cfg->col_x_start;
			break;

		case 2:
			vpeg->reg_528.bit.cl2_proc_width   = p_col_cfg->proc_width;
			vpeg->reg_528.bit.cl2_proc_x_start = p_col_cfg->proc_x_start;
			vpeg->reg_534.bit.cl2_col_x_start  = p_col_cfg->col_x_start;
			break;

		case 3:
			vpeg->reg_536.bit.cl3_proc_width   = p_col_cfg->proc_width;
			vpeg->reg_536.bit.cl3_proc_x_start = p_col_cfg->proc_x_start;
			vpeg->reg_542.bit.cl3_col_x_start  = p_col_cfg->col_x_start;
			break;

		case 4:
			vpeg->reg_544.bit.cl4_proc_width   = p_col_cfg->proc_width;
			vpeg->reg_544.bit.cl4_proc_x_start = p_col_cfg->proc_x_start;
			vpeg->reg_550.bit.cl4_col_x_start  = p_col_cfg->col_x_start;
			break;

		case 5:
			vpeg->reg_552.bit.cl5_proc_width   = p_col_cfg->proc_width;
			vpeg->reg_552.bit.cl5_proc_x_start = p_col_cfg->proc_x_start;
			vpeg->reg_558.bit.cl5_col_x_start  = p_col_cfg->col_x_start;
			break;

		case 6:
			vpeg->reg_560.bit.cl6_proc_width   = p_col_cfg->proc_width;
			vpeg->reg_560.bit.cl6_proc_x_start = p_col_cfg->proc_x_start;
			vpeg->reg_566.bit.cl6_col_x_start  = p_col_cfg->col_x_start;
			break;

		case 7:
			vpeg->reg_568.bit.cl7_proc_width   = p_col_cfg->proc_width;
			vpeg->reg_568.bit.cl7_proc_x_start = p_col_cfg->proc_x_start;
			vpeg->reg_574.bit.cl7_col_x_start  = p_col_cfg->col_x_start;
			break;

		default:
			break;
		}
	}
}


static void vpe_set_res_ctl_reg(uint32_t res_idx, VPE_RES_CTL_CFG *p_res_ctl_cfg)
{

	if (p_res_ctl_cfg != NULL) {
		switch (res_idx) {
		case 0:
			vpeg->reg_320.bit.res0_sca_en = p_res_ctl_cfg->sca_en;
			vpeg->reg_320.bit.res0_sca_bypass_en = p_res_ctl_cfg->sca_bypass_en;
			vpeg->reg_320.bit.res0_sca_crop_en = p_res_ctl_cfg->sca_crop_en;
			vpeg->reg_320.bit.res0_tc_en = p_res_ctl_cfg->tc_en;
			vpeg->reg_320.bit.res0_sca_uv_sel = p_res_ctl_cfg->sca_chroma_sel;
			vpeg->reg_320.bit.res0_des_drt = p_res_ctl_cfg->des_drt;
			vpeg->reg_320.bit.res0_des_yuv420 = p_res_ctl_cfg->des_yuv420;
			vpeg->reg_320.bit.res0_des_uv_swap = p_res_ctl_cfg->des_uv_swap;
			vpeg->reg_320.bit.res0_out_bg_sel = p_res_ctl_cfg->out_bg_sel;

			vpeg->reg_321.bit.res0_des_dp0_ycc_enc_en = p_res_ctl_cfg->des_dp0_ycc_enc_en;
			vpeg->reg_321.bit.res0_des_dp0_chrw = p_res_ctl_cfg->des_dp0_chrw;
			vpeg->reg_321.bit.res0_des_dp0_format = p_res_ctl_cfg->des_dp0_format;
			break;

		case 1:
			vpeg->reg_352.bit.res1_sca_en = p_res_ctl_cfg->sca_en;
			vpeg->reg_352.bit.res1_sca_bypass_en = p_res_ctl_cfg->sca_bypass_en;
			vpeg->reg_352.bit.res1_sca_crop_en = p_res_ctl_cfg->sca_crop_en;
			vpeg->reg_352.bit.res1_tc_en = p_res_ctl_cfg->tc_en;
			vpeg->reg_352.bit.res1_sca_uv_sel = p_res_ctl_cfg->sca_chroma_sel;
			vpeg->reg_352.bit.res1_des_drt = p_res_ctl_cfg->des_drt;
			vpeg->reg_352.bit.res1_des_yuv420 = p_res_ctl_cfg->des_yuv420;
			vpeg->reg_352.bit.res1_des_uv_swap = p_res_ctl_cfg->des_uv_swap;
			vpeg->reg_352.bit.res1_out_bg_sel = p_res_ctl_cfg->out_bg_sel;

			vpeg->reg_353.bit.res1_des_dp0_ycc_enc_en = p_res_ctl_cfg->des_dp0_ycc_enc_en;
			vpeg->reg_353.bit.res1_des_dp0_chrw = p_res_ctl_cfg->des_dp0_chrw;
			vpeg->reg_353.bit.res1_des_dp0_format = p_res_ctl_cfg->des_dp0_format;
			break;

		case 2:
			vpeg->reg_768.bit.res2_sca_en = p_res_ctl_cfg->sca_en;
			vpeg->reg_768.bit.res2_sca_bypass_en = p_res_ctl_cfg->sca_bypass_en;
			vpeg->reg_768.bit.res2_sca_crop_en = p_res_ctl_cfg->sca_crop_en;
			vpeg->reg_768.bit.res2_tc_en = p_res_ctl_cfg->tc_en;
			vpeg->reg_768.bit.res2_sca_uv_sel = p_res_ctl_cfg->sca_chroma_sel;
			vpeg->reg_768.bit.res2_des_drt = p_res_ctl_cfg->des_drt;
			vpeg->reg_768.bit.res2_des_yuv420 = p_res_ctl_cfg->des_yuv420;
			vpeg->reg_768.bit.res2_des_uv_swap = p_res_ctl_cfg->des_uv_swap;
			vpeg->reg_768.bit.res2_out_bg_sel = p_res_ctl_cfg->out_bg_sel;

			vpeg->reg_769.bit.res2_des_dp0_chrw = p_res_ctl_cfg->des_dp0_chrw;
			vpeg->reg_769.bit.res2_des_dp0_format = p_res_ctl_cfg->des_dp0_format;
			break;

		case 3:
			vpeg->reg_800.bit.res3_sca_en = p_res_ctl_cfg->sca_en;
			vpeg->reg_800.bit.res3_sca_bypass_en = p_res_ctl_cfg->sca_bypass_en;
			vpeg->reg_800.bit.res3_sca_crop_en = p_res_ctl_cfg->sca_crop_en;
			vpeg->reg_800.bit.res3_tc_en = p_res_ctl_cfg->tc_en;
			vpeg->reg_800.bit.res3_sca_uv_sel = p_res_ctl_cfg->sca_chroma_sel;
			vpeg->reg_800.bit.res3_des_drt = p_res_ctl_cfg->des_drt;
			vpeg->reg_800.bit.res3_des_yuv420 = p_res_ctl_cfg->des_yuv420;
			vpeg->reg_800.bit.res3_des_uv_swap = p_res_ctl_cfg->des_uv_swap;
			vpeg->reg_800.bit.res3_out_bg_sel = p_res_ctl_cfg->out_bg_sel;

			vpeg->reg_801.bit.res3_des_dp0_chrw = p_res_ctl_cfg->des_dp0_chrw;
			vpeg->reg_801.bit.res3_des_dp0_format = p_res_ctl_cfg->des_dp0_format;
			break;

		default:
			break;
		}
	}
}

static void vpe_set_res_ctl_enable_reg(uint32_t res_idx, uint32_t set_en)
{
	switch (res_idx) {
	case 0:
		vpeg->reg_320.bit.res0_sca_en = set_en;
		break;

	case 1:
		vpeg->reg_352.bit.res1_sca_en = set_en;
		break;

	case 2:
		vpeg->reg_768.bit.res2_sca_en = set_en;
		break;

	case 3:
		vpeg->reg_800.bit.res3_sca_en = set_en;
		break;

	default:
		break;
	}
}

static void vpe_set_res_gbl_size_reg(uint32_t res_idx, VPE_RES_SIZE_CFG *p_res_gbl_size_cfg, VPE_RES_SCA_CROP_CFG *p_res_gbl_sca_crop_cfg, VPE_RES_TC_CFG *p_res_gbl_tc_cfg)
{
	if (p_res_gbl_size_cfg != NULL) {
		switch (res_idx) {
		case 0:
			vpeg->reg_322.bit.res0_sca_height = p_res_gbl_size_cfg->sca_height;

			vpeg->reg_323.bit.res0_des_buf_width = p_res_gbl_size_cfg->des_width;
			vpeg->reg_323.bit.res0_des_buf_height = p_res_gbl_size_cfg->des_height;

			vpeg->reg_324.bit.res0_out_y_start = p_res_gbl_size_cfg->out_y_start;
			vpeg->reg_324.bit.res0_out_height = p_res_gbl_size_cfg->out_height;

			vpeg->reg_325.bit.res0_rlt_y_start = p_res_gbl_size_cfg->rlt_y_start;
			vpeg->reg_325.bit.res0_rlt_height = p_res_gbl_size_cfg->rlt_height;

			vpeg->reg_326.bit.res0_pip_y_start = p_res_gbl_size_cfg->pip_y_start;
			vpeg->reg_326.bit.res0_pip_height = p_res_gbl_size_cfg->pip_height;

			vpeg->reg_327.bit.res0_sca_crop_y_start = p_res_gbl_sca_crop_cfg->sca_crop_y_start;
			vpeg->reg_327.bit.res0_sca_crop_height = p_res_gbl_sca_crop_cfg->sca_crop_height;

			vpeg->reg_336.bit.res0_tc_crop_y_start = p_res_gbl_tc_cfg->tc_crop_y_start;
			vpeg->reg_336.bit.res0_tc_crop_height = p_res_gbl_tc_cfg->tc_crop_height;
			break;

		case 1:
			vpeg->reg_354.bit.res1_sca_height = p_res_gbl_size_cfg->sca_height;

			vpeg->reg_355.bit.res1_des_buf_width = p_res_gbl_size_cfg->des_width;
			vpeg->reg_355.bit.res1_des_buf_height = p_res_gbl_size_cfg->des_height;

			vpeg->reg_356.bit.res1_out_y_start = p_res_gbl_size_cfg->out_y_start;
			vpeg->reg_356.bit.res1_out_height = p_res_gbl_size_cfg->out_height;

			vpeg->reg_357.bit.res1_rlt_y_start = p_res_gbl_size_cfg->rlt_y_start;
			vpeg->reg_357.bit.res1_rlt_height = p_res_gbl_size_cfg->rlt_height;

			vpeg->reg_358.bit.res1_pip_y_start = p_res_gbl_size_cfg->pip_y_start;
			vpeg->reg_358.bit.res1_pip_height = p_res_gbl_size_cfg->pip_height;

			vpeg->reg_359.bit.res1_sca_crop_y_start = p_res_gbl_sca_crop_cfg->sca_crop_y_start;
			vpeg->reg_359.bit.res1_sca_crop_height = p_res_gbl_sca_crop_cfg->sca_crop_height;

			vpeg->reg_368.bit.res1_tc_crop_y_start = p_res_gbl_tc_cfg->tc_crop_y_start;
			vpeg->reg_368.bit.res1_tc_crop_height = p_res_gbl_tc_cfg->tc_crop_height;
			break;

		case 2:
			vpeg->reg_770.bit.res2_sca_height = p_res_gbl_size_cfg->sca_height;

			vpeg->reg_771.bit.res2_des_buf_width = p_res_gbl_size_cfg->des_width;
			vpeg->reg_771.bit.res2_des_buf_height = p_res_gbl_size_cfg->des_height;

			vpeg->reg_772.bit.res2_out_y_start = p_res_gbl_size_cfg->out_y_start;
			vpeg->reg_772.bit.res2_out_height = p_res_gbl_size_cfg->out_height;

			vpeg->reg_773.bit.res2_rlt_y_start = p_res_gbl_size_cfg->rlt_y_start;
			vpeg->reg_773.bit.res2_rlt_height = p_res_gbl_size_cfg->rlt_height;

			vpeg->reg_774.bit.res2_pip_y_start = p_res_gbl_size_cfg->pip_y_start;
			vpeg->reg_774.bit.res2_pip_height = p_res_gbl_size_cfg->pip_height;

			vpeg->reg_775.bit.res2_sca_crop_y_start = p_res_gbl_sca_crop_cfg->sca_crop_y_start;
			vpeg->reg_775.bit.res2_sca_crop_height = p_res_gbl_sca_crop_cfg->sca_crop_height;

			vpeg->reg_784.bit.res2_tc_crop_y_start = p_res_gbl_tc_cfg->tc_crop_y_start;
			vpeg->reg_784.bit.res2_tc_crop_height = p_res_gbl_tc_cfg->tc_crop_height;
			break;

		case 3:
			vpeg->reg_802.bit.res3_sca_height = p_res_gbl_size_cfg->sca_height;

			vpeg->reg_803.bit.res3_des_buf_width = p_res_gbl_size_cfg->des_width;
			vpeg->reg_803.bit.res3_des_buf_height = p_res_gbl_size_cfg->des_height;

			vpeg->reg_804.bit.res3_out_y_start = p_res_gbl_size_cfg->out_y_start;
			vpeg->reg_804.bit.res3_out_height = p_res_gbl_size_cfg->out_height;

			vpeg->reg_805.bit.res3_rlt_y_start = p_res_gbl_size_cfg->rlt_y_start;
			vpeg->reg_805.bit.res3_rlt_height = p_res_gbl_size_cfg->rlt_height;

			vpeg->reg_806.bit.res3_pip_y_start = p_res_gbl_size_cfg->pip_y_start;
			vpeg->reg_806.bit.res3_pip_height = p_res_gbl_size_cfg->pip_height;

			vpeg->reg_807.bit.res3_sca_crop_y_start = p_res_gbl_sca_crop_cfg->sca_crop_y_start;
			vpeg->reg_807.bit.res3_sca_crop_height = p_res_gbl_sca_crop_cfg->sca_crop_height;

			vpeg->reg_816.bit.res3_tc_crop_y_start = p_res_gbl_tc_cfg->tc_crop_y_start;
			vpeg->reg_816.bit.res3_tc_crop_height = p_res_gbl_tc_cfg->tc_crop_height;
			break;

		default:
			break;
		}
	}
}

static void vpe_set_sca_factor_reg(uint32_t res_idx, VPE_RES_SCA_CFG *p_res_sca_factor_cfg)
{
	if (p_res_sca_factor_cfg != NULL) {
		switch (res_idx) {
		case 0:
			vpeg->reg_328.bit.res0_sca_y_luma_algo_en = p_res_sca_factor_cfg->sca_y_luma_algo_en;
			vpeg->reg_328.bit.res0_sca_x_luma_algo_en = p_res_sca_factor_cfg->sca_x_luma_algo_en;
			vpeg->reg_328.bit.res0_sca_y_chroma_algo_en = p_res_sca_factor_cfg->sca_y_chroma_algo_en;
			vpeg->reg_328.bit.res0_sca_x_chroma_algo_en = p_res_sca_factor_cfg->sca_x_chroma_algo_en;
			vpeg->reg_328.bit.res0_sca_map_sel = p_res_sca_factor_cfg->sca_map_sel;

			vpeg->reg_329.bit.res0_sca_hstep = p_res_sca_factor_cfg->sca_hstep;
			vpeg->reg_329.bit.res0_sca_vstep = p_res_sca_factor_cfg->sca_vstep;

			vpeg->reg_330.bit.res0_sca_hsca_divisor = p_res_sca_factor_cfg->sca_hsca_divisor;
			vpeg->reg_330.bit.res0_sca_havg_pxl_msk = p_res_sca_factor_cfg->sca_havg_pxl_msk;

			vpeg->reg_331.bit.res0_sca_vsca_divisor = p_res_sca_factor_cfg->sca_vsca_divisor;
			vpeg->reg_331.bit.res0_sca_vavg_pxl_msk = p_res_sca_factor_cfg->sca_vavg_pxl_msk;

			vpeg->reg_332.bit.res0_sca_coef_h0 = p_res_sca_factor_cfg->coeffHorizontal[0];
			vpeg->reg_332.bit.res0_sca_coef_h1 = p_res_sca_factor_cfg->coeffHorizontal[1];
			vpeg->reg_333.bit.res0_sca_coef_h2 = p_res_sca_factor_cfg->coeffHorizontal[2];
			vpeg->reg_333.bit.res0_sca_coef_h3 = p_res_sca_factor_cfg->coeffHorizontal[3];

			vpeg->reg_334.bit.res0_sca_coef_v0 = p_res_sca_factor_cfg->coeffVertical[0];
			vpeg->reg_334.bit.res0_sca_coef_v1 = p_res_sca_factor_cfg->coeffVertical[1];
			vpeg->reg_335.bit.res0_sca_coef_v2 = p_res_sca_factor_cfg->coeffVertical[2];
			vpeg->reg_335.bit.res0_sca_coef_v3 = p_res_sca_factor_cfg->coeffVertical[3];
			break;

		case 1:
			vpeg->reg_360.bit.res1_sca_y_luma_algo_en = p_res_sca_factor_cfg->sca_y_luma_algo_en;
			vpeg->reg_360.bit.res1_sca_x_luma_algo_en = p_res_sca_factor_cfg->sca_x_luma_algo_en;
			vpeg->reg_360.bit.res1_sca_y_chroma_algo_en = p_res_sca_factor_cfg->sca_y_chroma_algo_en;
			vpeg->reg_360.bit.res1_sca_x_chroma_algo_en = p_res_sca_factor_cfg->sca_x_chroma_algo_en;
			vpeg->reg_360.bit.res1_sca_map_sel = p_res_sca_factor_cfg->sca_map_sel;

			vpeg->reg_361.bit.res1_sca_hstep = p_res_sca_factor_cfg->sca_hstep;
			vpeg->reg_361.bit.res1_sca_vstep = p_res_sca_factor_cfg->sca_vstep;

			vpeg->reg_362.bit.res1_sca_hsca_divisor = p_res_sca_factor_cfg->sca_hsca_divisor;
			vpeg->reg_362.bit.res1_sca_havg_pxl_msk = p_res_sca_factor_cfg->sca_havg_pxl_msk;

			vpeg->reg_363.bit.res1_sca_vsca_divisor = p_res_sca_factor_cfg->sca_vsca_divisor;
			vpeg->reg_363.bit.res1_sca_vavg_pxl_msk = p_res_sca_factor_cfg->sca_vavg_pxl_msk;

			vpeg->reg_364.bit.res1_sca_coef_h0 = p_res_sca_factor_cfg->coeffHorizontal[0];
			vpeg->reg_364.bit.res1_sca_coef_h1 = p_res_sca_factor_cfg->coeffHorizontal[1];
			vpeg->reg_365.bit.res1_sca_coef_h2 = p_res_sca_factor_cfg->coeffHorizontal[2];
			vpeg->reg_365.bit.res1_sca_coef_h3 = p_res_sca_factor_cfg->coeffHorizontal[3];

			vpeg->reg_366.bit.res1_sca_coef_v0 = p_res_sca_factor_cfg->coeffVertical[0];
			vpeg->reg_366.bit.res1_sca_coef_v1 = p_res_sca_factor_cfg->coeffVertical[1];
			vpeg->reg_367.bit.res1_sca_coef_v2 = p_res_sca_factor_cfg->coeffVertical[2];
			vpeg->reg_367.bit.res1_sca_coef_v3 = p_res_sca_factor_cfg->coeffVertical[3];
			break;

		case 2:
			vpeg->reg_776.bit.res2_sca_y_luma_algo_en = p_res_sca_factor_cfg->sca_y_luma_algo_en;
			vpeg->reg_776.bit.res2_sca_x_luma_algo_en = p_res_sca_factor_cfg->sca_x_luma_algo_en;
			vpeg->reg_776.bit.res2_sca_y_chroma_algo_en = p_res_sca_factor_cfg->sca_y_chroma_algo_en;
			vpeg->reg_776.bit.res2_sca_x_chroma_algo_en = p_res_sca_factor_cfg->sca_x_chroma_algo_en;
			vpeg->reg_776.bit.res2_sca_map_sel = p_res_sca_factor_cfg->sca_map_sel;

			vpeg->reg_777.bit.res2_sca_hstep = p_res_sca_factor_cfg->sca_hstep;
			vpeg->reg_777.bit.res2_sca_vstep = p_res_sca_factor_cfg->sca_vstep;

			vpeg->reg_778.bit.res2_sca_hsca_divisor = p_res_sca_factor_cfg->sca_hsca_divisor;
			vpeg->reg_778.bit.res2_sca_havg_pxl_msk = p_res_sca_factor_cfg->sca_havg_pxl_msk;

			vpeg->reg_779.bit.res2_sca_vsca_divisor = p_res_sca_factor_cfg->sca_vsca_divisor;
			vpeg->reg_779.bit.res2_sca_vavg_pxl_msk = p_res_sca_factor_cfg->sca_vavg_pxl_msk;

			vpeg->reg_780.bit.res2_sca_coef_h0 = p_res_sca_factor_cfg->coeffHorizontal[0];
			vpeg->reg_780.bit.res2_sca_coef_h1 = p_res_sca_factor_cfg->coeffHorizontal[1];
			vpeg->reg_781.bit.res2_sca_coef_h2 = p_res_sca_factor_cfg->coeffHorizontal[2];
			vpeg->reg_781.bit.res2_sca_coef_h3 = p_res_sca_factor_cfg->coeffHorizontal[3];

			vpeg->reg_782.bit.res2_sca_coef_v0 = p_res_sca_factor_cfg->coeffVertical[0];
			vpeg->reg_782.bit.res2_sca_coef_v1 = p_res_sca_factor_cfg->coeffVertical[1];
			vpeg->reg_783.bit.res2_sca_coef_v2 = p_res_sca_factor_cfg->coeffVertical[2];
			vpeg->reg_783.bit.res2_sca_coef_v3 = p_res_sca_factor_cfg->coeffVertical[3];
			break;

		case 3:
			vpeg->reg_808.bit.res3_sca_y_luma_algo_en = p_res_sca_factor_cfg->sca_y_luma_algo_en;
			vpeg->reg_808.bit.res3_sca_x_luma_algo_en = p_res_sca_factor_cfg->sca_x_luma_algo_en;
			vpeg->reg_808.bit.res3_sca_y_chroma_algo_en = p_res_sca_factor_cfg->sca_y_chroma_algo_en;
			vpeg->reg_808.bit.res3_sca_x_chroma_algo_en = p_res_sca_factor_cfg->sca_x_chroma_algo_en;
			vpeg->reg_808.bit.res3_sca_map_sel = p_res_sca_factor_cfg->sca_map_sel;

			vpeg->reg_809.bit.res3_sca_hstep = p_res_sca_factor_cfg->sca_hstep;
			vpeg->reg_809.bit.res3_sca_vstep = p_res_sca_factor_cfg->sca_vstep;

			vpeg->reg_810.bit.res3_sca_hsca_divisor = p_res_sca_factor_cfg->sca_hsca_divisor;
			vpeg->reg_810.bit.res3_sca_havg_pxl_msk = p_res_sca_factor_cfg->sca_havg_pxl_msk;

			vpeg->reg_811.bit.res3_sca_vsca_divisor = p_res_sca_factor_cfg->sca_vsca_divisor;
			vpeg->reg_811.bit.res3_sca_vavg_pxl_msk = p_res_sca_factor_cfg->sca_vavg_pxl_msk;

			vpeg->reg_812.bit.res3_sca_coef_h0 = p_res_sca_factor_cfg->coeffHorizontal[0];
			vpeg->reg_812.bit.res3_sca_coef_h1 = p_res_sca_factor_cfg->coeffHorizontal[1];
			vpeg->reg_813.bit.res3_sca_coef_h2 = p_res_sca_factor_cfg->coeffHorizontal[2];
			vpeg->reg_813.bit.res3_sca_coef_h3 = p_res_sca_factor_cfg->coeffHorizontal[3];

			vpeg->reg_814.bit.res3_sca_coef_v0 = p_res_sca_factor_cfg->coeffVertical[0];
			vpeg->reg_814.bit.res3_sca_coef_v1 = p_res_sca_factor_cfg->coeffVertical[1];
			vpeg->reg_815.bit.res3_sca_coef_v2 = p_res_sca_factor_cfg->coeffVertical[2];
			vpeg->reg_815.bit.res3_sca_coef_v3 = p_res_sca_factor_cfg->coeffVertical[3];
			break;

		default:
			break;
		}
	}
}

uint32_t vpe_res_col_start_ofs[4][8] = {
	{
		RES0_COL0_SIZE_1_REGISTER_OFS, RES0_COL1_SIZE_1_REGISTER_OFS, RES0_COL2_SIZE_1_REGISTER_OFS, RES0_COL3_SIZE_1_REGISTER_OFS,
		RES0_COL4_SIZE_1_REGISTER_OFS, RES0_COL5_SIZE_1_REGISTER_OFS, RES0_COL6_SIZE_1_REGISTER_OFS, RES0_COL7_SIZE_1_REGISTER_OFS,
	},

	{
		RES1_COL0_SIZE_1_REGISTER_OFS, RES1_COL1_SIZE_1_REGISTER_OFS, RES1_COL2_SIZE_1_REGISTER_OFS, RES1_COL3_SIZE_1_REGISTER_OFS,
		RES1_COL4_SIZE_1_REGISTER_OFS, RES1_COL5_SIZE_1_REGISTER_OFS, RES1_COL6_SIZE_1_REGISTER_OFS, RES1_COL7_SIZE_1_REGISTER_OFS,
	},

	{
		RES2_COL0_SIZE_1_REGISTER_OFS, RES2_COL1_SIZE_1_REGISTER_OFS, RES2_COL2_SIZE_1_REGISTER_OFS, RES2_COL3_SIZE_1_REGISTER_OFS,
		RES2_COL4_SIZE_1_REGISTER_OFS, RES2_COL5_SIZE_1_REGISTER_OFS, RES2_COL6_SIZE_1_REGISTER_OFS, RES2_COL7_SIZE_1_REGISTER_OFS,
	},

	{
		RES3_COL0_SIZE_1_REGISTER_OFS, RES3_COL1_SIZE_1_REGISTER_OFS, RES3_COL2_SIZE_1_REGISTER_OFS, RES3_COL3_SIZE_1_REGISTER_OFS,
		RES3_COL4_SIZE_1_REGISTER_OFS, RES3_COL5_SIZE_1_REGISTER_OFS, RES3_COL6_SIZE_1_REGISTER_OFS, RES3_COL7_SIZE_1_REGISTER_OFS,
	},
};


static void vpe_set_res_col_size_reg(uint32_t res_idx, uint32_t col_idx, VPE_RES_COL_SIZE_CFG *p_res_col_size_cfg)
{
	uint32_t init_ofs = 0, reg_ofs = 0, set_val = 0;

	if (p_res_col_size_cfg != NULL) {
		init_ofs = vpe_res_col_start_ofs[res_idx][col_idx];

		//printk("### res_idx: %d, col_idx: %d\r\n", res_idx, col_idx);

		set_val = 0;
		set_val = (p_res_col_size_cfg->sca_width) + (p_res_col_size_cfg->sca_hstep_oft << 16);
		reg_ofs = init_ofs + (0 << 2);
		//printk("reg_ofs: %08x,  set_val: %08x\r\n", reg_ofs, set_val);
		VPE_SETREG(reg_ofs, set_val);

		set_val = 0;
		set_val = (p_res_col_size_cfg->out_x_start) + (p_res_col_size_cfg->out_width << 16);
		reg_ofs = init_ofs + (1 << 2);
		//printk("reg_ofs: %08x,  set_val: %08x\r\n", reg_ofs, set_val);
		VPE_SETREG(reg_ofs, set_val);

		set_val = 0;
		set_val = (p_res_col_size_cfg->rlt_x_start) + (p_res_col_size_cfg->rlt_width << 16);
		reg_ofs = init_ofs + (2 << 2);
		//printk("reg_ofs: %08x,  set_val: %08x\r\n", reg_ofs, set_val);
		VPE_SETREG(reg_ofs, set_val);

		set_val = 0;
		set_val = (p_res_col_size_cfg->pip_x_start) + (p_res_col_size_cfg->pip_width << 16);
		reg_ofs = init_ofs + (3 << 2);
		//printk("reg_ofs: %08x,  set_val: %08x\r\n", reg_ofs, set_val);
		VPE_SETREG(reg_ofs, set_val);

		set_val = 0;
		set_val = (p_res_col_size_cfg->sca_crop_x_start) + (p_res_col_size_cfg->sca_crop_width << 16);
		reg_ofs = init_ofs + (4 << 2);
		//printk("reg_ofs: %08x,  set_val: %08x\r\n", reg_ofs, set_val);
		VPE_SETREG(reg_ofs, set_val);

		set_val = 0;
		set_val = (p_res_col_size_cfg->tc_crop_x_start) + (p_res_col_size_cfg->tc_crop_width << 16) + (p_res_col_size_cfg->tc_crop_skip << 31);
		reg_ofs = init_ofs + (5 << 2);
		//printk("reg_ofs: %08x,  set_val: %08x\r\n\r\n", reg_ofs, set_val);
		VPE_SETREG(reg_ofs, set_val);
	}
}

#if 0
VPE_REG_DEFINE vpe_sca_reg_def[VPE_REG_SCA_MAXNUM] = {
	{VPE_REG_SCA_Y_LUMA_ALGO_EN, 0, 0x20, 2, 0, 0, 0x3, {0, 0, 0, 0}},
	{VPE_REG_SCA_X_LUMA_ALGO_EN, 0, 0x20, 2, 2, 0, 0x3, {0, 0, 0, 0}},
	{VPE_REG_SCA_Y_CHROMA_ALGO_EN, 0, 0x20, 2, 4, 0, 0x3, {0, 0, 0, 0}},
	{VPE_REG_SCA_X_CHROMA_ALGO_EN, 0, 0x20, 2, 6, 0, 0x3, {0, 0, 0, 0}},
	{VPE_REG_SCA_MAP_SEL, 0, 0x20, 1, 8, 0, 0x1, {1, 1, 1, 1}},

	{VPE_REG_SCA_HSTEP, 0, 0x24, 14, 0, 0, 0x3ffff, {0, 0, 0, 0}},
	{VPE_REG_SCA_VSTEP, 0, 0x24, 14, 16, 0, 0x3ffff, {0, 0, 0, 0}},

	{VPE_REG_SCA_HSCA_DIVISOR, 0, 0x28, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_SCA_HAVG_PXL_MSK, 0, 0x28, 8, 16, 0, 0xff, {0, 0, 0, 0}},

	{VPE_REG_SCA_VSCA_DIVISOR, 0, 0x2c, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_SCA_VAVG_PXL_MSK, 0, 0x2c, 8, 16, 0, 0xff, {0, 0, 0, 0}},

	{VPE_REG_SCA_CEFFH_0, 0, 0x30, 10, 0, 0, 0x3ff, {0, 0, 0, 0}},
	{VPE_REG_SCA_CEFFH_1, 0, 0x30, 10, 16, 0, 0x3ff, {0, 0, 0, 0}},

	{VPE_REG_SCA_CEFFH_2, 0, 0x34, 10, 0, 0, 0x3ff, {0, 0, 0, 0}},
	{VPE_REG_SCA_CEFFH_3, 0, 0x34, 10, 16, 0, 0x3ff, {0, 0, 0, 0}},

	{VPE_REG_SCA_CEFFV_0, 0, 0x38, 10, 0, 0, 0x3ff, {0, 0, 0, 0}},
	{VPE_REG_SCA_CEFFV_1, 0, 0x38, 10, 16, 0, 0x3ff, {0, 0, 0, 0}},

	{VPE_REG_SCA_CEFFV_2, 0, 0x3c, 10, 0, 0, 0x3ff, {0, 0, 0, 0}},
	{VPE_REG_SCA_CEFFV_3, 0, 0x3c, 10, 16, 0, 0x3ff, {0, 0, 0, 0}},
};

VPE_REG_DEFINE vpe_res_ctl_reg_def[VPE_REG_RES_CTL_MAXNUM] = {
	{VPE_REG_RES_CTL_SCA_EN, 0, 0x0, 1, 0, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_RES_CTL_SCA_BYPASS_EN, 0, 0x0, 1, 1, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_RES_CTL_SCA_CROP_EN, 0, 0x0, 1, 2, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_RES_CTL_TC_EN, 0, 0x0, 1, 3, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_RES_CTL_SCA_CHROMA_SEL, 0, 0x0, 1, 4, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_RES_CTL_DES_DRT, 0, 0x0, 2, 8, 0, 0x3, {0, 0, 0, 0}},
	{VPE_REG_RES_CTL_DES_YUV420, 0, 0x0, 1, 11, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_RES_CTL_DES_UV_SWAP, 0, 0x0, 1, 15, 0, 0x3, {0, 0, 0, 0}},
	{VPE_REG_RES_CTL_OUT_BG_SEL, 0, 0x0, 3, 16, 0, 0x3, {0, 0, 0, 0}},
	{VPE_REG_RES_CTL_DES_DP0_YCC_ENC_EN, 0, 0x4, 1, 0, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_RES_CTL_DES_DP0_CHRW, 0, 0x4, 1, 4, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_RES_CTL_DES_DP0_FORMAT, 0, 0x4, 2, 8, 0, 0x1, {0, 0, 0, 0}},

};

VPE_REG_DEFINE vpe_res_size_reg_def[VPE_REG_RES_SIZE_MAXNUM] = {
	{VPE_REG_RES_SIZE_SCA_HEIGHT, 0, 0x8, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_RES_SIZE_DES_WIDTH, 0, 0xc, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_RES_SIZE_DES_HEIGHT, 0, 0xc, 13, 16, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_RES_SIZE_OUT_Y_START, 0, 0x10, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_RES_SIZE_OUT_HEIGHT, 0, 0x10, 13, 16, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_RES_SIZE_RLT_Y_START, 0, 0x14, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_RES_SIZE_RLT_HEIGHT, 0, 0x14, 13, 16, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_RES_SIZE_PIP_Y_START, 0, 0x18, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_RES_SIZE_PIP_HEIGHT, 0, 0x18, 13, 16, 0, 0x1fff, {0, 0, 0, 0}},
};

VPE_REG_DEFINE vpe_res_col_size_reg_def[VPE_REG_RES_COL_SIZE_MAXNUM] = {
	{VPE_REG_RES_COL_SIZE_SCA_WIDTH, 0, 0x0, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_RES_COL_SIZE_SCA_HSTEP_OFT, 0, 0x0, 15, 16, 0, 0x7fff, {0, 0, 0, 0}},

	{VPE_REG_RES_COL_SIZE_OUT_X_START, 0, 0x4, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_RES_COL_SIZE_OUT_WIDTH, 0, 0x4, 13, 16, 0, 0x1fff, {0, 0, 0, 0}},

	{VPE_REG_RES_COL_SIZE_RLT_X_START, 0, 0x8, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_RES_COL_SIZE_RLT_WIDTH, 0, 0x8, 13, 16, 0, 0x1fff, {0, 0, 0, 0}},

	{VPE_REG_RES_COL_SIZE_PIP_X_START, 0, 0xc, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_RES_COL_SIZE_PIP_WIDTH, 0, 0xc, 13, 16, 0, 0x1fff, {0, 0, 0, 0}},

	{VPE_REG_RES_COL_SIZE_SCA_CROP_X_START, 0, 0x10, 12, 0, 0, 0xfff, {0, 0, 0, 0}},
	{VPE_REG_RES_COL_SIZE_SCA_CROP_WIDTH, 0, 0x10, 13, 16, 0, 0x1000, {0, 0, 0, 0}},

	{VPE_REG_RES_COL_SIZE_TC_CROP_X_START, 0, 0x14, 13, 0, 0, 0xfff, {0, 0, 0, 0}},
	{VPE_REG_RES_COL_SIZE_TC_CROP_WIDTH, 0, 0x14, 13, 16, 0, 0x1000, {0, 0, 0, 0}},
	{VPE_REG_RES_COL_SIZE_TC_CROP_SKIP, 0, 0x14, 1, 31, 0, 0x1, {0, 0, 0, 0}},
};

VPE_REG_DEFINE vpe_res_tc_reg_def[VPE_REG_TC_MAXNUM] = {
	{VPE_REG_TC_TC_CROP_Y_START, 0, 0x40, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_TC_TC_CROP_HEIGHT, 0, 0x40, 13, 16, 0, 0x1fff, {0, 0, 0, 0}},
};

VPE_REG_DEFINE vpe_col_size_reg_def[VPE_REG_COL_SIZE_MAXNUM] = {
	{VPE_REG_COL_SIZE_PROC_WIDTH, 0, 0x0, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_COL_SIZE_PROC_X_START, 0, 0x0, 13, 16, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_COL_SIZE_COL_X_START, 0, 0x18, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
};

VPE_REG_DEFINE vpe_res_sca_crop_reg_def[VPE_REG_SCA_CROP_MAXNUM] = {
	{VPE_REG_SCA_CROP_Y_START, 0, 0x1c, 12, 0, 0, 0xfff, {0, 0, 0, 0}},
	{VPE_REG_SCA_CROP_HEIGHT, 0, 0x1c, 13, 16, 0, 0x1000, {0, 0, 0, 0}},
};
#endif


static UINT32 vpe_get_sca_hsca_vsca_divisor(UINT32 src, UINT32 des)
{
	INT32 ratio = 0;
	UINT32 ret = 0;

	ratio = src / des;

	if (src % des) {
		ratio ++;
	}

	switch (ratio) {
	case 8:
		ret = 512;
		break;
	case 7:
		ret = 585;
		break;
	case 6:
		ret = 683;
		break;
	case 5:
		ret = 819;
		break;
	case 4:
		ret = 1024;
		break;
	case 3:
		ret = 1365;
		break;
	case 2:
		ret = 2048;
		break;
	case 1:
		ret = 4096;
		break;
	default:
		ret = 512;
		break;
	}

	return ret;
}

static UINT32 vpe_get_sca_vsca_divisor_chroma(UINT32 src, UINT32 des, INT32 destType)
{
	UINT32 ratio = 0;
	UINT32 ret = 0;
	ratio = src * 10000 / des;

	if (destType == 1) {
		/* yuv 420*/
		if (ratio > 0 && ratio <= 10000) {
			ret = 4096;
		} else if (ratio > 10000 && ratio <= 15000) {
			ret = 1365;
		} else if (ratio > 15000 && ratio <= 20000) {
			ret = 1024;
		} else if (ratio > 20000 && ratio <= 25000) {
			ret = 819;
		} else if (ratio > 25000 && ratio <= 30000) {
			ret = 683;
		} else if (ratio > 30000 && ratio <= 35000) {
			ret = 585;
		} else {
			ret = 512;
		}
	} else {
		/* yuv 422*/
		if (ratio > 0 && ratio <= 10000) {
			ret = 4096;
		} else if (ratio > 10000 && ratio <= 20000) {
			ret = 2048;
		} else if (ratio > 20000 && ratio <= 30000) {
			ret = 1365;
		} else if (ratio > 30000 && ratio <= 40000) {
			ret = 1024;
		} else if (ratio > 40000 && ratio <= 50000) {
			ret = 819;
		} else if (ratio > 50000 && ratio <= 60000) {
			ret = 683;
		} else if (ratio > 60000 && ratio <= 70000) {
			ret = 585;
		} else {
			ret = 512;
		}
	}

	return ret;
}

static UINT32 vpe_get_sca_vavg_pxl_msk_chroma(UINT32 src, UINT32 des, INT32 destType)
{
	UINT32 ratio = 0;
	UINT32 ret = 0;
	ratio = src * 10000 / des;
	if (destType == 1) {
		/* yuv 420*/
		if (ratio > 0 && ratio <= 10000) {
			ret = 8;
		} else if (ratio > 10000 && ratio < 15000) {
			ret = 56;
		} else if (ratio == 15000) {
			ret = 28;
		} else if (ratio > 15000 && ratio <= 20000) {
			ret = 60;
		} else if (ratio > 20000 && ratio < 25000) {
			ret = 124;
		} else if (ratio == 25000) {
			ret = 62;
		} else if (ratio > 25000 && ratio <= 30000) {
			ret = 126;
		} else if (ratio > 30000 && ratio < 35000) {
			ret = 254;
		} else if (ratio == 35000) {
			ret = 127;
		} else {
			ret = 255;
		}
	} else {
		/* yuv 422*/
		if (ratio > 0 && ratio <= 10000) {
			ret = 8;
		} else if (ratio > 10000 && ratio <= 20000) {
			ret = 24;
		} else if (ratio > 20000 && ratio < 30000) {
			ret = 56;
		} else if (ratio == 30000) {
			ret = 28;
		} else if (ratio > 30000 && ratio <= 40000) {
			ret = 60;
		} else if (ratio > 40000 && ratio < 50000) {
			ret = 124;
		} else if (ratio == 50000) {
			ret = 62;
		} else if (ratio > 50000 && ratio <= 60000) {
			ret = 126;
		} else if (ratio > 60000 && ratio < 70000) {
			ret = 254;
		} else if (ratio == 70000) {
			ret = 127;
		} else {
			ret = 255;
		}
	}
	return ret;
}

static UINT32 vpe_get_sca_havg_vavg_pxl_msk(UINT32 src, UINT32 des)
{
	UINT32 ratio = 0;
	UINT32 ret = 0;
	ratio = src * 10000 / des;

	if (ratio > 0 && ratio <= 10000) {
		ret = 8;
	} else if (ratio > 10000 && ratio <= 20000) {
		ret = 24;
	} else if (ratio > 20000 && ratio < 30000) {
		ret = 56;
	} else if (ratio == 30000) {
		ret = 28;
	} else if (ratio > 30000 && ratio <= 40000) {
		ret = 60;
	} else if (ratio > 40000 && ratio < 50000) {
		ret = 124;
	} else if (ratio == 50000) {
		ret = 62;
	} else if (ratio > 50000 && ratio <= 60000) {
		ret = 126;
	} else if (ratio > 60000 && ratio < 70000) {
		ret = 254;
	} else if (ratio == 70000) {
		ret = 127;
	} else {
		ret = 255;
	}

	return ret;
}



int vpe_drv_sca_cal_ratio(VPE_DRV_CFG *p_drv_cfg)
{
	INT32 i = 0;
	UINT32 src_width = 0, src_height = 0, des_width = 0, des_height = 0;
	//double ratio = 0;
	for (i = 0; i < VPE_MAX_RES; i++) {
		if (p_drv_cfg->res[i].res_ctl.sca_en) {
			des_width = p_drv_cfg->res[i].res_col_size[0].sca_width;
			des_height = p_drv_cfg->res[i].res_size.sca_height;

			if (p_drv_cfg->res[i].res_ctl.sca_crop_en) {
				src_width = p_drv_cfg->res[i].res_col_size[0].sca_crop_width;
				src_height = p_drv_cfg->res[i].res_sca_crop.sca_crop_height;
			} else {
				src_width = p_drv_cfg->col_sz[0].proc_width;
				src_height = p_drv_cfg->glo_sz.proc_height;
			}

			if (p_drv_cfg->res[i].res_ctl.sca_bypass_en) {
				p_drv_cfg->res[i].res_col_size[0].sca_width = src_width;
				des_width = src_width;

				p_drv_cfg->res[i].res_size.sca_height = src_height;
				des_height = src_height;

				p_drv_cfg->res[i].res_sca.sca_hstep = 1024;
				p_drv_cfg->res[i].res_sca.sca_vstep = 1024;
			} else {
#if 1
				if (!p_drv_cfg->glo_ctl.col_num) {
					p_drv_cfg->res[i].res_sca.sca_hstep = (INT32)(src_width * 1024 / des_width);
					p_drv_cfg->res[i].res_sca.sca_vstep = (INT32)(src_height * 1024 / des_height);
				}
#else
				ratio = (double) src_width / des_width;
				p_drv_cfg->res[i].res_sca.sca_hstep = (INT32)(ratio * 1024);

				ratio = (double) src_height / des_height;
				p_drv_cfg->res[i].res_sca.sca_vstep = (INT32)(ratio * 1024);
#endif
			}
			if (!p_drv_cfg->res[i].res_sca.sca_iq_en) {
				if (p_drv_cfg->res[i].res_sca.sca_hstep <= 1024) {
					p_drv_cfg->res[i].res_sca.coeffHorizontal[0] = 0;
					p_drv_cfg->res[i].res_sca.coeffHorizontal[1] = 0;
					p_drv_cfg->res[i].res_sca.coeffHorizontal[2] = 0;
					p_drv_cfg->res[i].res_sca.coeffHorizontal[3] = 64;
				} else if (p_drv_cfg->res[i].res_sca.sca_hstep <= 2048) {
					p_drv_cfg->res[i].res_sca.coeffHorizontal[0] = -5;
					p_drv_cfg->res[i].res_sca.coeffHorizontal[1] = -2;
					p_drv_cfg->res[i].res_sca.coeffHorizontal[2] = 21;
					p_drv_cfg->res[i].res_sca.coeffHorizontal[3] = 36;
				} else if (p_drv_cfg->res[i].res_sca.sca_hstep <= 3072) {
					p_drv_cfg->res[i].res_sca.coeffHorizontal[0] = -3;
					p_drv_cfg->res[i].res_sca.coeffHorizontal[1] = 4;
					p_drv_cfg->res[i].res_sca.coeffHorizontal[2] = 18;
					p_drv_cfg->res[i].res_sca.coeffHorizontal[3] = 26;
				} else if (p_drv_cfg->res[i].res_sca.sca_hstep <= 4096) {
					p_drv_cfg->res[i].res_sca.coeffHorizontal[0] = -1;
					p_drv_cfg->res[i].res_sca.coeffHorizontal[1] = 7;
					p_drv_cfg->res[i].res_sca.coeffHorizontal[2] = 16;
					p_drv_cfg->res[i].res_sca.coeffHorizontal[3] = 20;
				} else {
					p_drv_cfg->res[i].res_sca.coeffHorizontal[0] = 4;
					p_drv_cfg->res[i].res_sca.coeffHorizontal[1] = 9;
					p_drv_cfg->res[i].res_sca.coeffHorizontal[2] = 12;
					p_drv_cfg->res[i].res_sca.coeffHorizontal[3] = 14;
				}

				if (p_drv_cfg->res[i].res_sca.sca_vstep <= 1024) {
					p_drv_cfg->res[i].res_sca.coeffVertical[0] = 0;
					p_drv_cfg->res[i].res_sca.coeffVertical[1] = 0;
					p_drv_cfg->res[i].res_sca.coeffVertical[2] = 0;
					p_drv_cfg->res[i].res_sca.coeffVertical[3] = 64;
				} else if (p_drv_cfg->res[i].res_sca.sca_vstep <= 2048) {
					p_drv_cfg->res[i].res_sca.coeffVertical[0] = -5;
					p_drv_cfg->res[i].res_sca.coeffVertical[1] = -2;
					p_drv_cfg->res[i].res_sca.coeffVertical[2] = 21;
					p_drv_cfg->res[i].res_sca.coeffVertical[3] = 36;
				} else if (p_drv_cfg->res[i].res_sca.sca_vstep <= 3072) {
					p_drv_cfg->res[i].res_sca.coeffVertical[0] = -3;
					p_drv_cfg->res[i].res_sca.coeffVertical[1] = 4;
					p_drv_cfg->res[i].res_sca.coeffVertical[2] = 18;
					p_drv_cfg->res[i].res_sca.coeffVertical[3] = 26;
				} else if (p_drv_cfg->res[i].res_sca.sca_vstep <= 4096) {
					p_drv_cfg->res[i].res_sca.coeffVertical[0] = -1;
					p_drv_cfg->res[i].res_sca.coeffVertical[1] = 7;
					p_drv_cfg->res[i].res_sca.coeffVertical[2] = 16;
					p_drv_cfg->res[i].res_sca.coeffVertical[3] = 20;
				} else {
					p_drv_cfg->res[i].res_sca.coeffVertical[0] = 4;
					p_drv_cfg->res[i].res_sca.coeffVertical[1] = 9;
					p_drv_cfg->res[i].res_sca.coeffVertical[2] = 12;
					p_drv_cfg->res[i].res_sca.coeffVertical[3] = 14;
				}
			}

			p_drv_cfg->res[i].res_sca.sca_hsca_divisor = vpe_get_sca_hsca_vsca_divisor(src_width, des_width);
			p_drv_cfg->res[i].res_sca.sca_vsca_divisor = vpe_get_sca_vsca_divisor_chroma(src_height, des_height, p_drv_cfg->res[i].res_ctl.des_yuv420);
			p_drv_cfg->res[i].res_sca.sca_vavg_pxl_msk = vpe_get_sca_vavg_pxl_msk_chroma(src_height, des_height, p_drv_cfg->res[i].res_ctl.des_yuv420);
			p_drv_cfg->res[i].res_sca.sca_havg_pxl_msk = vpe_get_sca_havg_vavg_pxl_msk(src_width, des_width);
		}

	}
	return 0;
}

inline UINT32 vpe_drv_sca_trans_config_to_regtable(VPE_RES_SCA_CFG *p_res_sca, UINT32 regIndex)
{
	UINT32 ret = 0;
	switch (regIndex) {
	case VPE_REG_SCA_Y_LUMA_ALGO_EN:
		ret = p_res_sca->sca_y_luma_algo_en;
		break;
	case VPE_REG_SCA_X_LUMA_ALGO_EN:
		ret = p_res_sca->sca_x_luma_algo_en;
		break;
	case VPE_REG_SCA_Y_CHROMA_ALGO_EN:
		ret = p_res_sca->sca_y_chroma_algo_en;
		break;
	case VPE_REG_SCA_X_CHROMA_ALGO_EN:
		ret = p_res_sca->sca_x_chroma_algo_en;
		break;
	case VPE_REG_SCA_MAP_SEL:
		ret = p_res_sca->sca_map_sel;
		break;
	case VPE_REG_SCA_HSTEP:
		ret = p_res_sca->sca_hstep;
		break;
	case VPE_REG_SCA_VSTEP:
		ret = p_res_sca->sca_vstep;
		break;
	case VPE_REG_SCA_HSCA_DIVISOR:
		ret = p_res_sca->sca_hsca_divisor;
		break;
	case VPE_REG_SCA_HAVG_PXL_MSK:
		ret = p_res_sca->sca_havg_pxl_msk;
		break;
	case VPE_REG_SCA_VSCA_DIVISOR:
		ret = p_res_sca->sca_vsca_divisor;
		break;
	case VPE_REG_SCA_VAVG_PXL_MSK:
		ret = p_res_sca->sca_vavg_pxl_msk;
		break;
	case VPE_REG_SCA_CEFFH_0:
		ret = p_res_sca->coeffHorizontal[0];
		break;
	case VPE_REG_SCA_CEFFH_1:
		ret = p_res_sca->coeffHorizontal[1];
		break;
	case VPE_REG_SCA_CEFFH_2:
		ret = p_res_sca->coeffHorizontal[2];
		break;
	case VPE_REG_SCA_CEFFH_3:
		ret = p_res_sca->coeffHorizontal[3];
		break;
	case VPE_REG_SCA_CEFFV_0:
		ret = p_res_sca->coeffVertical[0];
		break;
	case VPE_REG_SCA_CEFFV_1:
		ret = p_res_sca->coeffVertical[1];
		break;
	case VPE_REG_SCA_CEFFV_2:
		ret = p_res_sca->coeffVertical[2];
		break;
	case VPE_REG_SCA_CEFFV_3:
		ret = p_res_sca->coeffVertical[3];
		break;
	}
	return ret;
}
inline UINT32 vpe_drv_res_ctl_trans_config_to_regtable(VPE_RES_CTL_CFG *p_res_ctl, UINT32 regIndex)
{
	UINT32 ret = 0;
	switch (regIndex) {
	case VPE_REG_RES_CTL_SCA_EN:
		ret = p_res_ctl->sca_en;
		break;
	case VPE_REG_RES_CTL_SCA_BYPASS_EN:
		ret = p_res_ctl->sca_bypass_en;
		break;
	case VPE_REG_RES_CTL_SCA_CROP_EN:
		ret = p_res_ctl->sca_crop_en;
		break;
	case VPE_REG_RES_CTL_TC_EN:
		ret = p_res_ctl->tc_en;
		break;
	case VPE_REG_RES_CTL_DES_DRT:
		ret = p_res_ctl->des_drt;
		break;
	case VPE_REG_RES_CTL_DES_YUV420:
		ret = p_res_ctl->des_yuv420;
		break;
	case VPE_REG_RES_CTL_DES_UV_SWAP:
		ret = p_res_ctl->des_uv_swap;
		break;
	case VPE_REG_RES_CTL_OUT_BG_SEL:
		ret = p_res_ctl->out_bg_sel;
		break;
	case VPE_REG_RES_CTL_DES_DP0_YCC_ENC_EN:
		ret = p_res_ctl->des_dp0_ycc_enc_en;
		break;
	case VPE_REG_RES_CTL_DES_DP0_CHRW:
		ret = p_res_ctl->des_dp0_chrw;
		break;
	case VPE_REG_RES_CTL_DES_DP0_FORMAT:
		ret = p_res_ctl->des_dp0_format;
		break;

	}
	return ret;
}
inline UINT32 vpe_drv_res_size_trans_config_to_regtable(VPE_RES_SIZE_CFG *p_res_size, UINT32 regIndex)
{
	UINT32 ret = 0;
	switch (regIndex) {
	case VPE_REG_RES_SIZE_SCA_HEIGHT:
		ret = p_res_size->sca_height;
		break;
	case VPE_REG_RES_SIZE_DES_WIDTH:
		ret = p_res_size->des_width;
		break;
	case VPE_REG_RES_SIZE_DES_HEIGHT:
		ret = p_res_size->des_height;
		break;
	case VPE_REG_RES_SIZE_OUT_Y_START:
		ret = p_res_size->out_y_start;
		break;
	case VPE_REG_RES_SIZE_OUT_HEIGHT:
		ret = p_res_size->out_height;
		break;
	case VPE_REG_RES_SIZE_RLT_Y_START:
		ret = p_res_size->rlt_y_start;
		break;
	case VPE_REG_RES_SIZE_RLT_HEIGHT:
		ret = p_res_size->rlt_height;
		break;
	case VPE_REG_RES_SIZE_PIP_Y_START:
		ret = p_res_size->pip_y_start;
		break;
	case VPE_REG_RES_SIZE_PIP_HEIGHT:
		ret = p_res_size->pip_height;
		break;
	}
	return ret;
}

inline UINT32 vpe_drv_res_col_size_trans_config_to_regtable(VPE_RES_COL_SIZE_CFG *p_res_col_size, UINT32 regIndex)
{
	UINT32 ret = 0;
	switch (regIndex) {
	case VPE_REG_RES_COL_SIZE_SCA_WIDTH:
		ret = p_res_col_size->sca_width;
		break;
	case VPE_REG_RES_COL_SIZE_SCA_HSTEP_OFT:
		ret = p_res_col_size->sca_hstep_oft;
		break;
	case VPE_REG_RES_COL_SIZE_OUT_X_START:
		ret = p_res_col_size->out_x_start;
		break;
	case VPE_REG_RES_COL_SIZE_OUT_WIDTH:
		ret = p_res_col_size->out_width;
		break;
	case VPE_REG_RES_COL_SIZE_RLT_X_START:
		ret = p_res_col_size->rlt_x_start;
		break;
	case VPE_REG_RES_COL_SIZE_RLT_WIDTH:
		ret = p_res_col_size->rlt_width;
		break;
	case VPE_REG_RES_COL_SIZE_PIP_X_START:
		ret = p_res_col_size->pip_x_start;
		break;
	case VPE_REG_RES_COL_SIZE_PIP_WIDTH:
		ret = p_res_col_size->pip_width;
		break;
	case VPE_REG_RES_COL_SIZE_SCA_CROP_X_START:
		ret = p_res_col_size->sca_crop_x_start;
		break;
	case VPE_REG_RES_COL_SIZE_SCA_CROP_WIDTH:
		ret = p_res_col_size->sca_crop_width;
		break;
	case VPE_REG_RES_COL_SIZE_TC_CROP_X_START:
		ret = p_res_col_size->tc_crop_x_start;
		break;
	case VPE_REG_RES_COL_SIZE_TC_CROP_WIDTH:
		ret = p_res_col_size->tc_crop_width;
		break;
	case VPE_REG_RES_COL_SIZE_TC_CROP_SKIP:
		ret = p_res_col_size->tc_crop_skip;
		break;

	}
	return ret;
}

UINT32 vpe_drv_res_tc_trans_config_to_regtable(VPE_RES_TC_CFG *p_res_tc, UINT32 regIndex)
{
	UINT32 ret = 0;
	switch (regIndex) {
	case VPE_REG_TC_TC_CROP_Y_START:
		ret = p_res_tc->tc_crop_y_start;
		break;
	case VPE_REG_TC_TC_CROP_HEIGHT:
		ret = p_res_tc->tc_crop_height;
		break;

	}
	return ret;
}

UINT32 vpe_drv_res_sca_crop_trans_config_to_regtable(VPE_RES_SCA_CROP_CFG *p_res_sca_crop, UINT32 regIndex)
{
	UINT32 ret = 0;
	switch (regIndex) {
	case VPE_REG_SCA_CROP_Y_START:
		ret = p_res_sca_crop->sca_crop_y_start;
		break;
	case VPE_REG_SCA_CROP_HEIGHT:
		ret = p_res_sca_crop->sca_crop_height;
		break;

	}
	return ret;
}



UINT32 vpe_drv_col_size_trans_config_to_regtable(VPE_COL_SZ_CFG *p_col_sz, UINT32 regIndex)
{
	UINT32 ret = 0;
	switch (regIndex) {
	case VPE_REG_COL_SIZE_PROC_WIDTH:
		ret = p_col_sz->proc_width;
		break;
	case VPE_REG_COL_SIZE_PROC_X_START:
		ret = p_col_sz->proc_x_start;
		break;
	case VPE_REG_COL_SIZE_COL_X_START:
		ret = p_col_sz->col_x_start;
		break;

	}
	return ret;
}

#if 0
int vpe_drv_sca_trans_regtable_to_cmdlist(VPE_DRV_CFG *p_drv_cfg, VPE_LLCMD_DATA *p_llcmd)
{
	UINT32 i, j, k, l, mask;
	UINT32 *cmduffer;
	UINT32 reg_start;
	UINT32 tmpval = 0, regoffset = 0, regval = 0;

	cmduffer = (UINT32 *)p_llcmd->vaddr;

	for (i = 0; i < VPE_TOTAL_RES; i++) {
		if (i < 2) {
			reg_start = 0x500 + 0x80 * i;
		} else {
			reg_start = 0xc00 + 0x80 * (i - 2);
		}

		for (j = VPE_REG_RES_CTL_START; j <= VPE_REG_RES_CTL_END; j += VPE_REG_OFFSET) {
			tmpval = 0;
			mask = 0;
			regoffset = reg_start + j;
			for (k = 0; k < VPE_REG_RES_CTL_MAXNUM; k++) {
				if (vpe_res_ctl_reg_def[k].reg_add == j) {
					if (vpe_res_ctl_reg_def[k].numofbits >= VPE_REG_MAXBIT) {
						mask = 0xffffffff;
					} else {
						mask = (1UL << vpe_res_ctl_reg_def[k].numofbits) - 1;
					}

					regval = vpe_drv_res_ctl_trans_config_to_regtable(&(p_drv_cfg->res[i].res_ctl), k);
					tmpval |= ((UINT32)(regval & mask)) << vpe_res_ctl_reg_def[k].startBit;
				}
			}

			if (mask > 0) {
				cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM_VAL(tmpval);
				cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM(0xf, regoffset);
			}
		}

		for (j = VPE_REG_RES_SIZE_START; j <= VPE_REG_RES_SIZE_END; j += VPE_REG_OFFSET) {
			tmpval = 0;
			mask = 0;
			regoffset = reg_start + j;
			for (k = 0; k < VPE_REG_RES_SIZE_MAXNUM; k++) {
				if (vpe_res_size_reg_def[k].reg_add == j) {
					if (vpe_res_size_reg_def[k].numofbits >= VPE_REG_MAXBIT) {
						mask = 0xffffffff;
					} else {
						mask = (1UL << vpe_res_size_reg_def[k].numofbits) - 1;
					}

					regval = vpe_drv_res_size_trans_config_to_regtable(&(p_drv_cfg->res[i].res_size), k);
					tmpval |= ((UINT32)(regval & mask)) << vpe_res_size_reg_def[k].startBit;
				}
			}

			if (mask > 0) {
				cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM_VAL(tmpval);
				cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM(0xf, regoffset);
			}
		}

		for (j = VPE_REG_SCA_START; j <= VPE_REG_SCA_END; j += VPE_REG_OFFSET) {
			tmpval = 0;
			mask = 0;
			regoffset = reg_start + j;
			for (k = 0; k < VPE_REG_SCA_MAXNUM; k++) {
				if (vpe_sca_reg_def[k].reg_add == j) {
					if (vpe_sca_reg_def[k].numofbits >= VPE_REG_MAXBIT) {
						mask = 0xffffffff;
					} else {
						mask = (1UL << vpe_sca_reg_def[k].numofbits) - 1;
					}

					regval = vpe_drv_sca_trans_config_to_regtable(&(p_drv_cfg->res[i].res_sca), k);
					tmpval |= ((UINT32)(regval & mask)) << vpe_sca_reg_def[k].startBit;
				}
			}

			if (mask > 0) {
				cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM_VAL(tmpval);
				cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM(0xf, regoffset);
			}
		}

		for (j = VPE_REG_TC_START; j <= VPE_REG_TC_END; j += VPE_REG_OFFSET) {
			tmpval = 0;
			mask = 0;
			regoffset = reg_start + j;
			for (k = 0; k < VPE_REG_TC_MAXNUM; k++) {
				if (vpe_res_tc_reg_def[k].reg_add == j) {
					if (vpe_res_tc_reg_def[k].numofbits >= VPE_REG_MAXBIT) {
						mask = 0xffffffff;
					} else {
						mask = (1UL << vpe_res_tc_reg_def[k].numofbits) - 1;
					}

					regval = vpe_drv_res_tc_trans_config_to_regtable(&(p_drv_cfg->res[i].res_tc), k);
					tmpval |= ((UINT32)(regval & mask)) << vpe_res_tc_reg_def[k].startBit;
				}
			}

			if (mask > 0) {
				cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM_VAL(tmpval);
				cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM(0xf, regoffset);
			}
		}

		for (j = VPE_REG_SCA_CROP_START; j <= VPE_REG_SCA_CROP_END; j += VPE_REG_OFFSET) {
			tmpval = 0;
			mask = 0;
			regoffset = reg_start + j;
			for (k = 0; k < VPE_REG_SCA_CROP_MAXNUM; k++) {
				if (vpe_res_sca_crop_reg_def[k].reg_add == j) {
					if (vpe_res_sca_crop_reg_def[k].numofbits >= VPE_REG_MAXBIT) {
						mask = 0xffffffff;
					} else {
						mask = (1UL << vpe_res_sca_crop_reg_def[k].numofbits) - 1;
					}

					regval = vpe_drv_res_sca_crop_trans_config_to_regtable(&(p_drv_cfg->res[i].res_sca_crop), k);
					tmpval |= ((UINT32)(regval & mask)) << vpe_res_sca_crop_reg_def[k].startBit;
				}
			}

			if (mask > 0) {
				cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM_VAL(tmpval);
				cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM(0xf, regoffset);
			}
		}


		if (i < 2) {
			reg_start = 0x600 + 0x100 * i;
		} else {
			reg_start = 0xd00 + 0x100 * (i - 2);
		}

		for (j = VPE_REG_RES_COL_SIZE_START; j <= VPE_REG_RES_COL_SIZE_END; j += VPE_REG_OFFSET) {
			for (k = 0; k < VPE_TOTAL_COL; k++) {
#if VPE_CPU_UTILIZATION
				if (k > p_drv_cfg->glo_ctl.col_num) {
					break;
				}
#endif
				tmpval = 0;
				mask = 0;
				regoffset = reg_start + (0x20 * k) + j;

				for (l = 0; l < VPE_REG_RES_COL_SIZE_MAXNUM; l++) {
					if (vpe_res_col_size_reg_def[l].reg_add == j) {
						if (vpe_res_col_size_reg_def[l].numofbits >= VPE_REG_MAXBIT) {
							mask = 0xffffffff;
						} else {
							mask = (1UL << vpe_res_col_size_reg_def[l].numofbits) - 1;
						}

						regval = vpe_drv_res_col_size_trans_config_to_regtable(&(p_drv_cfg->res[i].res_col_size[k]), l);
						tmpval |= ((UINT32)(regval & mask)) << vpe_res_col_size_reg_def[l].startBit;
					}
				}

				if (mask > 0) {
					cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM_VAL(tmpval);
					cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM(0xf, regoffset);
				}
			}
		}
	}




	return 0;
}
#endif

#if 0
int vpe_drv_col_trans_regtable_to_cmdlist(VPE_DRV_CFG *p_drv_cfg, VPE_LLCMD_DATA *p_llcmd)
{
	UINT32 i, j, k, mask;
	UINT32 *cmduffer;
	UINT32 reg_start;
	UINT32 tmpval = 0, regoffset = 0, regval = 0;

	cmduffer = (UINT32 *)p_llcmd->vaddr;
	for (i = 0; i < VPE_TOTAL_COL; i++) {
		reg_start = 0x800 + (0x20 * i);
#if VPE_CPU_UTILIZATION
		if (i > p_drv_cfg->glo_ctl.col_num) {
			break;
		}
#endif
		for (j = VPE_REG_COL_SIZE_START; j <= VPE_REG_COL_SIZE_END; j += VPE_REG_OFFSET) {
			tmpval = 0;
			mask = 0;
			regoffset = reg_start + j;
			for (k = 0; k < VPE_REG_COL_SIZE_MAXNUM; k++) {
				if (vpe_col_size_reg_def[k].reg_add == j) {
					if (vpe_col_size_reg_def[k].numofbits >= VPE_REG_MAXBIT) {
						mask = 0xffffffff;
					} else {
						mask = (1UL << vpe_col_size_reg_def[k].numofbits) - 1;
					}

					regval = vpe_drv_col_size_trans_config_to_regtable(&(p_drv_cfg->col_sz[i]), k);
					tmpval |= ((UINT32)(regval & mask)) << vpe_col_size_reg_def[k].startBit;
				}
			}

			if (mask > 0) {
				cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM_VAL(tmpval);
				cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM(0xf, regoffset);
			}
		}
	}

	return 0;
}
#endif

#if 0
UINT16 vpe_drv_sca_cal_column(UINT16 src, UINT16 des, UINT16 src_col)
{
	UINT16 des_col = 0;

	des_col = des * src_col / src;

	return des_col;
}
#endif

int vpe_drv_get_dest_align(UINT32 des_type, UINT32 type)
{
	INT32 ret = 0;
	UINT32 align_des_width = 0, align_des_height = 0;
	UINT32 align_out_width = 0, align_out_height = 0, align_out_x_start = 0, align_out_y_start = 0;
	UINT32 align_rlt_width = 0, align_rlt_height = 0, align_rlt_x_start = 0, align_rlt_y_start = 0;
	INT32 align_pip_width = 0, align_pip_height = 0, align_pip_x_start = 0, align_pip_y_start = 0;

	switch (des_type) {


	case KDRV_VPE_DES_TYPE_YUV420:
		align_des_width = ALIGN_16;
		align_des_height = ALIGN_2;
		align_out_x_start = ALIGN_4;
		align_out_y_start = ALIGN_2;
		align_out_width = ALIGN_4;
		align_out_height = ALIGN_2;
		align_rlt_x_start = ALIGN_2;
		align_rlt_y_start = ALIGN_2;
		align_rlt_width = ALIGN_4;
		align_rlt_height = ALIGN_2;
		align_pip_x_start = ALIGN_4;
		align_pip_y_start = ALIGN_2;
		align_pip_width = ALIGN_4;
		align_pip_height = ALIGN_2;
		break;


	case KDRV_VPE_DES_TYPE_YCC_YUV420:
		align_des_width = ALIGN_32;
		align_des_height = ALIGN_2;
		align_out_x_start = ALIGN_32;
		align_out_y_start = ALIGN_2;
		align_out_width = ALIGN_32;
		align_out_height = ALIGN_2;
		align_rlt_x_start = ALIGN_2;
		align_rlt_y_start = ALIGN_2;
		align_rlt_width = ALIGN_8;
		align_rlt_height = ALIGN_2;
		align_pip_x_start = ALIGN_32;
		align_pip_y_start = ALIGN_2;
		align_pip_width = ALIGN_32;
		align_pip_height = ALIGN_2;
		break;
	default:
		break;
	}

	switch (type) {
	case ALIGM_TYPE_DES_W:
		ret = align_des_width;
		break;
	case ALIGM_TYPE_DES_H:
		ret = align_des_height;
		break;
	case ALIGM_TYPE_OUT_W:
		ret = align_out_width;
		break;
	case ALIGM_TYPE_OUT_H:
		ret = align_out_height;
		break;
	case ALIGM_TYPE_OUT_X:
		ret = align_out_x_start;
		break;
	case ALIGM_TYPE_OUT_Y:
		ret = align_out_y_start;
		break;
	case ALIGM_TYPE_RLT_W:
		ret = align_rlt_width;
		break;
	case ALIGM_TYPE_RLT_H:
		ret = align_rlt_height;
		break;
	case ALIGM_TYPE_RLT_X:
		ret = align_rlt_x_start;
		break;
	case ALIGM_TYPE_RLT_Y:
		ret = align_rlt_y_start;
		break;
	case ALIGM_TYPE_PIP_W:
		ret = align_pip_width;
		break;
	case ALIGM_TYPE_PIP_H:
		ret = align_pip_height;
		break;
	case ALIGM_TYPE_PIP_X:
		ret = align_pip_x_start;
		break;
	case ALIGM_TYPE_PIP_Y:
		ret = align_pip_y_start;
		break;
	default:
		ret = 0;
	}
	return ret;
}


int vpe_drv_column_cal(KDRV_VPE_COL_INFO_GET *p_col_info)
{
	INT32 i = 0;
	INT32 column_en = 0, max_col_w = 0, per_col_w = 0;

	INT32 rot_dir = 0;
	INT32 remain_w = 0;

	INT32 res_out_w_max = 0, col_w_temp = 0;
	INT32 src_in_x = 0;
	INT32 src_width = 0;
	INT32 col_overlap = COLUMN_OVERLAP;

	INT32 out_width[KDRV_VPE_MAX_IMG];
	INT32 j = 0;
	INT32 align_out_width = 0;
	INT32 temp_proc_x = 0;

	if (p_col_info != NULL) {
		rot_dir = 0;
		src_in_x = p_col_info->src_in_x;

		for (i = 0; i < KDRV_VPE_MAX_IMG; i++) {

			if (p_col_info->des_type[i] == KDRV_VPE_DES_TYPE_YCC_YUV420) {
				col_overlap = COLUMN_SCE_OVERLAP;
				break;
			}

			//if (col_overlap == COLUMN_SCE_OVERLAP) {
			//  break;
			//}
		}


		max_col_w = VPE_COL_MAX_W;
		per_col_w = COLUMN_MAX_PROC_W;

		if (p_col_info->src_in_width > max_col_w) {
			column_en = 0x1;
		}



		for (i = 0; i < KDRV_VPE_MAX_IMG; i++) {
			if (p_col_info->out_width[i] > VPE_COL_MAX_W) {

				if (res_out_w_max < p_col_info->out_width[i]) {
					res_out_w_max = p_col_info->out_width[i];
				}
				column_en = column_en | 0x2;
			}
		}

#if KDRV_VPE_SUPPORT_DEWARP
		if (p_col_info->column_type) {
			column_en = 0x1;
		}
#endif

		if (p_col_info->column_type == KDRV_VPE_COL_TYPE_PARANOMA) {
			remain_w = res_out_w_max;
			src_width = remain_w;
		} else {
			remain_w = p_col_info->src_in_width;
			src_width = remain_w;
		}



		if (column_en) {
			if (column_en == 1) {
#if KDRV_VPE_SUPPORT_DEWARP
				col_overlap = 8;
				if (p_col_info->column_type == KDRV_VPE_COL_TYPE_DCTG) {
					col_w_temp = p_col_info->src_in_width / 2 + col_overlap;
				} else if (p_col_info->column_type == KDRV_VPE_COL_TYPE_PARANOMA) {
					col_w_temp = res_out_w_max / 4 + col_overlap;
				} else
#endif
					col_w_temp = per_col_w;

			} else if (column_en == 2) {
				col_w_temp = (src_width * VPE_COL_MAX_W) / res_out_w_max;
			} else {
				col_w_temp = (src_width * VPE_COL_MAX_W) / res_out_w_max;
				if (col_w_temp > per_col_w) {
					col_w_temp = per_col_w;
				}
			}
			if (col_overlap == COLUMN_SCE_OVERLAP) {
				if (col_w_temp % 32) {
					col_w_temp = col_w_temp - (col_w_temp % 32);
				}
			} else {
				if (col_w_temp % 8) {
					col_w_temp = col_w_temp - (col_w_temp % 8);
				}
			}
			for (i = 0; i < 8; i++) {
				if (remain_w <= col_w_temp) {
					p_col_info->col_size_info[i].proc_width = remain_w + col_overlap;
					remain_w = 0;
				} else {
					p_col_info->col_size_info[i].proc_width = col_w_temp;
				}

#if KDRV_VPE_SUPPORT_DEWARP
				if (p_col_info->column_type == KDRV_VPE_COL_TYPE_DCTG && i) {
					p_col_info->col_size_info[i].proc_width = remain_w + col_overlap;
					remain_w = 0;
				}
#if KDRV_VPE_SUPPORT_PARANOMA
				else if (p_col_info->column_type == KDRV_VPE_COL_TYPE_PARANOMA) {
					if (i == 0) {
						p_col_info->col_size_info[i].proc_width = col_w_temp;
					} else {
						p_col_info->col_size_info[i].proc_width = col_w_temp + col_overlap;
					}
				}
#endif
#endif

				if (remain_w == 0) {

					p_col_info->col_size_info[i].proc_width = src_width + src_in_x - p_col_info->col_size_info[i].proc_x_start;
					//p_col_info->col_size_info[i].proc_x_start = p_col_info->col_size_info[i].proc_x_start;
					if (col_overlap == COLUMN_SCE_OVERLAP) {
						if (p_col_info->col_size_info[i].proc_width % 32) {
							p_col_info->col_size_info[i].proc_x_start = p_col_info->col_size_info[i].proc_x_start - (32 - (p_col_info->col_size_info[i].proc_width % 32));
							p_col_info->col_size_info[i].proc_width = p_col_info->col_size_info[i].proc_width + 32 - (p_col_info->col_size_info[i].proc_width % 32);
						}
					} else {
						if (p_col_info->col_size_info[i].proc_width % 8) {
							p_col_info->col_size_info[i].proc_x_start = p_col_info->col_size_info[i].proc_x_start - (8 - (p_col_info->col_size_info[i].proc_width % 8));
							p_col_info->col_size_info[i].proc_width = p_col_info->col_size_info[i].proc_width + 8 - (p_col_info->col_size_info[i].proc_width % 8);
						}
					}
				} else {
					if (i == 7) {
						return -2;
					}

					if (i == 0) {
						p_col_info->col_size_info[i].proc_x_start = src_in_x;
					}

					for (j = 0; j < KDRV_VPE_MAX_IMG; j++) {
#if VPE_RES_LIMIT
						if (p_col_info->out_width[j] == 0) {
							break;
						}
#else
						if (p_col_info->out_width[j] == 0) {
							continue;
						}
#endif
						out_width[j] = p_col_info->col_size_info[i].proc_width * p_col_info->out_width[j] / p_col_info->src_in_width;


						if (align_out_width < vpe_drv_get_dest_align(p_col_info->des_type[j], ALIGM_TYPE_OUT_W)) {
							align_out_width = vpe_drv_get_dest_align(p_col_info->des_type[j], ALIGM_TYPE_OUT_W);
						}


						if (!align_out_width) {
							vpe_err("vpe_drv_column_cal: config error, align_out_width %d\n", align_out_width);
							return -1;
						}
						if (i == 0) {
							out_width[j] = out_width[j] - (col_overlap * 1 * 1024 / (p_col_info->src_in_width * 1024 / p_col_info->out_width[j]));
						} else {
							out_width[j] = out_width[j] - (col_overlap * 2 * 1024 / (p_col_info->src_in_width * 1024 / p_col_info->out_width[j]));
						}

						temp_proc_x = p_col_info->col_size_info[i].proc_x_start + p_col_info->col_size_info[i].proc_width - col_overlap * 2;

						if (out_width[j] % align_out_width) {
							temp_proc_x = temp_proc_x - ((out_width[j] % align_out_width) * p_col_info->src_in_width / p_col_info->out_width[j]);

							if (temp_proc_x % 2) {
								temp_proc_x = temp_proc_x - 1;
							}
						}
#if VPE_RES_LIMIT
						if (j == 0)
#else
						if (j == 0 || p_col_info->col_size_info[i + 1].proc_x_start == 0)
#endif
							p_col_info->col_size_info[i + 1].proc_x_start = temp_proc_x;
						else {
							if (p_col_info->col_size_info[i + 1].proc_x_start > temp_proc_x) {
								p_col_info->col_size_info[i + 1].proc_x_start = p_col_info->col_size_info[i].proc_x_start + temp_proc_x;
							}
						}


					}

					if (remain_w < (p_col_info->col_size_info[i + 1].proc_x_start - p_col_info->col_size_info[i].proc_x_start)) {
						remain_w = 0;
					} else {
						remain_w = remain_w - (p_col_info->col_size_info[i + 1].proc_x_start - p_col_info->col_size_info[i].proc_x_start);
					}

				}

#if KDRV_VPE_SUPPORT_DEWARP
				if (p_col_info->column_type) {
					if (i) {
						p_col_info->col_size_info[i].col_x_start = p_col_info->col_size_info[i].proc_x_start - src_in_x;
					} else {
						p_col_info->col_size_info[i].col_x_start = 0;
					}

					p_col_info->col_size_info[i].proc_x_start = src_in_x;
				} else {
					p_col_info->col_size_info[i].col_x_start = p_col_info->col_size_info[i].proc_x_start;
				}

#else
				p_col_info->col_size_info[i].col_x_start = p_col_info->col_size_info[i].proc_x_start;
#endif

				if (!remain_w) {
					break;
				}
			}
			p_col_info->col_num = i;

		} else {
			p_col_info->col_num = 0;
			p_col_info->col_size_info[0].proc_width = src_width;
			p_col_info->col_size_info[0].proc_x_start = src_in_x;
			p_col_info->col_size_info[0].col_x_start = 0;
		}

		return 0;

	} else {
		DBG_ERR("VPE: parameter NULL...\r\n");

		return -1;
	}

}

#if 0
int vpe_drv_dewarp_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg)
{
	INT32 i = 0, j = 0;
	//INT32 col_overlap = 8;
	INT32 col_overlap = 48;

	for (i = 0; i < p_drv_cfg->glo_ctl.col_num + 1; i++) {
		p_drv_cfg->col_sz[i].proc_width = p_vpe_info->col_info.col_size_info[i].proc_width;
		//p_drv_cfg->col_sz[i].proc_x_start = p_vpe_info->col_info.col_size_info[i].proc_x_start;  // dce_en = 1, set the same to proc_x_start of column0
		p_drv_cfg->col_sz[i].proc_x_start = p_vpe_info->col_info.col_size_info[0].proc_x_start;  // dce_en = 1, set the same to proc_x_start of column0
		p_drv_cfg->col_sz[i].col_x_start = p_vpe_info->col_info.col_size_info[i].col_x_start;
	}

	p_drv_cfg->glo_sz.src_width = p_vpe_info->glb_img_info.src_width;
	p_drv_cfg->glo_sz.src_height = p_vpe_info->glb_img_info.src_height;
	p_drv_cfg->glo_sz.presca_merge_width = p_vpe_info->dce_param.dewarp_width;
	p_drv_cfg->glo_sz.dce_width = p_vpe_info->dce_param.dewarp_width;
	p_drv_cfg->glo_sz.dce_height = p_vpe_info->dce_param.dewarp_width;
	p_drv_cfg->glo_sz.proc_height = p_vpe_info->glb_img_info.src_in_h;
	p_drv_cfg->glo_sz.proc_y_start = p_vpe_info->glb_img_info.src_in_y;



	for (i = 0; i < VPE_MAX_RES; i++) {
		p_drv_cfg->res[i].res_ctl.sca_en = 0;
		p_drv_cfg->res[i].res_ctl.tc_en = 1;
		if (p_vpe_info->out_img_info[i].out_dup_num) {
			p_drv_cfg->res[i].res_ctl.sca_en = 1;
			p_drv_cfg->res[i].res_ctl.out_bg_sel = p_vpe_info->out_img_info[i].out_bg_sel;
			p_drv_cfg->res[i].res_ctl.des_dp0_chrw = 1;
			p_drv_cfg->res[0].res_ctl.des_yuv420 = 1;
			p_drv_cfg->res[i].res_ctl.des_dp0_format = 0;

			p_drv_cfg->res[i].res_sca_crop.sca_crop_height = p_vpe_info->out_img_info[i].res_dim_info.sca_crop_h;
			p_drv_cfg->res[i].res_sca_crop.sca_crop_y_start = p_vpe_info->out_img_info[i].res_dim_info.sca_crop_y;

			p_drv_cfg->res[i].res_size.sca_height = p_vpe_info->out_img_info[i].res_dim_info.des_out_h;

			//p_drv_cfg->res[i].res_size.des_width = p_vpe_info->dce_param.dewarp_width;
			p_drv_cfg->res[i].res_size.des_width = p_vpe_info->out_img_info[i].res_dim_info.des_width;
			p_drv_cfg->res[i].res_size.des_height = p_vpe_info->out_img_info[i].res_dim_info.des_height;

			p_drv_cfg->res[i].res_size.out_y_start = 0;
			p_drv_cfg->res[i].res_size.out_height = p_vpe_info->out_img_info[i].res_dim_info.des_height;
			p_drv_cfg->res[i].res_size.rlt_y_start = 0;
			p_drv_cfg->res[i].res_size.rlt_height = p_drv_cfg->res[i].res_size.out_height;
			p_drv_cfg->res[i].res_size.pip_y_start = 0;
			p_drv_cfg->res[i].res_size.pip_height = 0;

			p_drv_cfg->res[i].res_tc.tc_crop_y_start = 0;
			p_drv_cfg->res[i].res_tc.tc_crop_height = p_drv_cfg->res[i].res_size.out_height;

		} else {
			continue;
		}

		if (p_drv_cfg->glo_ctl.col_num == 0) {
			col_overlap = 0;
		} else {
			col_overlap = 48;
		}


		for (j = 0; j < p_drv_cfg->glo_ctl.col_num + 1; j++) {
			if (j == 0) {
#if 0
				p_drv_cfg->res[i].res_col_size[j].sca_width = p_drv_cfg->col_sz[i].proc_width;
				p_drv_cfg->res[i].res_col_size[j].sca_hstep_oft = 0;
				p_drv_cfg->res[i].res_col_size[j].out_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].out_width = p_drv_cfg->col_sz[i].proc_width - col_overlap;
				p_drv_cfg->res[i].res_col_size[j].rlt_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].rlt_width = p_drv_cfg->col_sz[i].proc_width - col_overlap;
				p_drv_cfg->res[i].res_col_size[j].pip_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].pip_width = 0;
				p_drv_cfg->res[i].res_col_size[j].sca_crop_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].sca_crop_width = p_drv_cfg->col_sz[i].proc_width;
				p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].tc_crop_width = p_drv_cfg->col_sz[i].proc_width - col_overlap;
#else
				//p_drv_cfg->res[i].res_col_size[j].sca_width = p_drv_cfg->col_sz[j].proc_width;
				p_drv_cfg->res[i].res_col_size[j].sca_width = p_vpe_info->out_img_info[i].res_dim_info.des_out_w;
				p_drv_cfg->res[i].res_col_size[j].sca_hstep_oft = 0;

				p_drv_cfg->res[i].res_col_size[j].out_x_start = 0;
				//p_drv_cfg->res[i].res_col_size[j].out_width = p_drv_cfg->col_sz[j].proc_width - col_overlap;
				p_drv_cfg->res[i].res_col_size[j].out_width = p_drv_cfg->res[i].res_col_size[j].sca_width - col_overlap;

				p_drv_cfg->res[i].res_col_size[j].rlt_x_start = 0;
				//p_drv_cfg->res[i].res_col_size[j].rlt_width = p_drv_cfg->col_sz[j].proc_width - col_overlap;
				p_drv_cfg->res[i].res_col_size[j].rlt_width = p_drv_cfg->res[i].res_col_size[j].out_width - col_overlap;

				p_drv_cfg->res[i].res_col_size[j].pip_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].pip_width = 0;

				p_drv_cfg->res[i].res_col_size[j].sca_crop_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].sca_crop_width = p_drv_cfg->col_sz[j].proc_width;
				p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].tc_crop_width = p_drv_cfg->col_sz[j].proc_width - col_overlap;
#endif


			} else if (j == p_drv_cfg->glo_ctl.col_num) {
#if 0
				p_drv_cfg->res[i].res_col_size[j].sca_width = p_drv_cfg->col_sz[i].proc_width;
				p_drv_cfg->res[i].res_col_size[j].sca_hstep_oft = 0;
				p_drv_cfg->res[i].res_col_size[j].out_x_start = p_drv_cfg->res[i].res_col_size[j - 1].out_width + p_drv_cfg->res[i].res_col_size[j - 1].out_x_start;
				p_drv_cfg->res[i].res_col_size[j].out_width = p_drv_cfg->res[i].res_col_size[j].sca_width - col_overlap;
				p_drv_cfg->res[i].res_col_size[j].rlt_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].rlt_width = p_drv_cfg->res[i].res_col_size[j].out_width;
				p_drv_cfg->res[i].res_col_size[j].pip_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].pip_width = 0;
				p_drv_cfg->res[i].res_col_size[j].sca_crop_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].sca_crop_width = p_drv_cfg->res[i].res_col_size[j].sca_width;
				p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start = col_overlap;
				p_drv_cfg->res[i].res_col_size[j].tc_crop_width = p_drv_cfg->res[i].res_col_size[j].out_width;
#else
				//p_drv_cfg->res[i].res_col_size[j].sca_width = p_drv_cfg->col_sz[j].proc_width;
				p_drv_cfg->res[i].res_col_size[j].sca_width = p_vpe_info->out_img_info[i].res_dim_info.des_out_w;
				p_drv_cfg->res[i].res_col_size[j].sca_hstep_oft = 0;

				p_drv_cfg->res[i].res_col_size[j].out_x_start = p_drv_cfg->res[i].res_col_size[j - 1].out_width + p_drv_cfg->res[i].res_col_size[j - 1].out_x_start;
				p_drv_cfg->res[i].res_col_size[j].out_width = p_drv_cfg->res[i].res_col_size[j].sca_width - col_overlap;

				p_drv_cfg->res[i].res_col_size[j].rlt_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].rlt_width = p_drv_cfg->res[i].res_col_size[j].out_width;

				p_drv_cfg->res[i].res_col_size[j].pip_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].pip_width = 0;

				p_drv_cfg->res[i].res_col_size[j].sca_crop_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].sca_crop_width = p_drv_cfg->res[i].res_col_size[j].sca_width;

				p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start = col_overlap;
				p_drv_cfg->res[i].res_col_size[j].tc_crop_width = p_drv_cfg->res[i].res_col_size[j].out_width;
#endif
			} else {
#if 0
				p_drv_cfg->res[i].res_col_size[j].sca_width = p_drv_cfg->col_sz[i].proc_width + col_overlap;
				p_drv_cfg->res[i].res_col_size[j].sca_hstep_oft = 0;
				p_drv_cfg->res[i].res_col_size[j].out_x_start = p_drv_cfg->res[i].res_col_size[j - 1].out_width + p_drv_cfg->res[i].res_col_size[j - 1].out_x_start;
				p_drv_cfg->res[i].res_col_size[j].out_width = p_drv_cfg->res[i].res_col_size[j].sca_width - col_overlap * 2;
				p_drv_cfg->res[i].res_col_size[j].rlt_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].rlt_width = p_drv_cfg->res[i].res_col_size[j].out_width;
				p_drv_cfg->res[i].res_col_size[j].pip_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].pip_width = 0;
				p_drv_cfg->res[i].res_col_size[j].sca_crop_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].sca_crop_width = p_drv_cfg->res[i].res_col_size[j].sca_width;
				p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start = col_overlap;
				p_drv_cfg->res[i].res_col_size[j].tc_crop_width = p_drv_cfg->res[i].res_col_size[j].out_width;
#else
				//p_drv_cfg->res[i].res_col_size[j].sca_width = p_drv_cfg->col_sz[j].proc_width + col_overlap;
				p_drv_cfg->res[i].res_col_size[j].sca_width = p_vpe_info->out_img_info[i].res_dim_info.des_out_w + col_overlap;
				p_drv_cfg->res[i].res_col_size[j].sca_hstep_oft = 0;

				p_drv_cfg->res[i].res_col_size[j].out_x_start = p_drv_cfg->res[i].res_col_size[j - 1].out_width + p_drv_cfg->res[i].res_col_size[j - 1].out_x_start;
				p_drv_cfg->res[i].res_col_size[j].out_width = p_drv_cfg->res[i].res_col_size[j].sca_width - col_overlap * 2;

				p_drv_cfg->res[i].res_col_size[j].rlt_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].rlt_width = p_drv_cfg->res[i].res_col_size[j].out_width;

				p_drv_cfg->res[i].res_col_size[j].pip_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].pip_width = 0;

				p_drv_cfg->res[i].res_col_size[j].sca_crop_x_start = 0;
				p_drv_cfg->res[i].res_col_size[j].sca_crop_width = p_drv_cfg->res[i].res_col_size[j].sca_width;

				p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start = col_overlap;
				p_drv_cfg->res[i].res_col_size[j].tc_crop_width = p_drv_cfg->res[i].res_col_size[j].out_width;
#endif
			}
			p_drv_cfg->res[i].res_col_size[j].tc_crop_skip = 0;

		}

#if 0
		if (p_vpe_info->dce_param.dewarp_width == p_vpe_info->out_img_info[i].res_dim_info.des_width && p_vpe_info->dce_param.dewarp_height == p_vpe_info->out_img_info[i].res_dim_info.des_height) {
			p_drv_cfg->res[i].res_ctl.sca_bypass_en = 1;
			p_drv_cfg->res[i].res_sca.sca_hstep = 1024;
			p_drv_cfg->res[i].res_sca.sca_vstep = 1024;
		} else {
			p_drv_cfg->res[i].res_sca.sca_hstep = (INT32)(p_vpe_info->dce_param.dewarp_width * 1024 / p_drv_cfg->res[i].res_size.des_width);
			p_drv_cfg->res[i].res_sca.sca_vstep = (INT32)(p_vpe_info->dce_param.dewarp_height * 1024 / p_drv_cfg->res[i].res_size.des_height);
		}

		if (p_drv_cfg->res[i].res_sca.sca_hstep <= 1024) {
			p_drv_cfg->res[i].res_sca.coeffHorizontal[0] = 0;
			p_drv_cfg->res[i].res_sca.coeffHorizontal[1] = 0;
			p_drv_cfg->res[i].res_sca.coeffHorizontal[2] = 0;
			p_drv_cfg->res[i].res_sca.coeffHorizontal[3] = 64;
		} else if (p_drv_cfg->res[i].res_sca.sca_hstep <= 2048) {
			p_drv_cfg->res[i].res_sca.coeffHorizontal[0] = -5;
			p_drv_cfg->res[i].res_sca.coeffHorizontal[1] = -2;
			p_drv_cfg->res[i].res_sca.coeffHorizontal[2] = 21;
			p_drv_cfg->res[i].res_sca.coeffHorizontal[3] = 36;
		} else if (p_drv_cfg->res[i].res_sca.sca_hstep <= 3072) {
			p_drv_cfg->res[i].res_sca.coeffHorizontal[0] = -3;
			p_drv_cfg->res[i].res_sca.coeffHorizontal[1] = 4;
			p_drv_cfg->res[i].res_sca.coeffHorizontal[2] = 18;
			p_drv_cfg->res[i].res_sca.coeffHorizontal[3] = 26;
		} else if (p_drv_cfg->res[i].res_sca.sca_hstep <= 4096) {
			p_drv_cfg->res[i].res_sca.coeffHorizontal[0] = -1;
			p_drv_cfg->res[i].res_sca.coeffHorizontal[1] = 7;
			p_drv_cfg->res[i].res_sca.coeffHorizontal[2] = 16;
			p_drv_cfg->res[i].res_sca.coeffHorizontal[3] = 20;
		} else {
			p_drv_cfg->res[i].res_sca.coeffHorizontal[0] = 4;
			p_drv_cfg->res[i].res_sca.coeffHorizontal[1] = 9;
			p_drv_cfg->res[i].res_sca.coeffHorizontal[2] = 12;
			p_drv_cfg->res[i].res_sca.coeffHorizontal[3] = 14;
		}

		if (p_drv_cfg->res[i].res_sca.sca_vstep <= 1024) {
			p_drv_cfg->res[i].res_sca.coeffVertical[0] = 0;
			p_drv_cfg->res[i].res_sca.coeffVertical[1] = 0;
			p_drv_cfg->res[i].res_sca.coeffVertical[2] = 0;
			p_drv_cfg->res[i].res_sca.coeffVertical[3] = 64;
		} else if (p_drv_cfg->res[i].res_sca.sca_vstep <= 2048) {
			p_drv_cfg->res[i].res_sca.coeffVertical[0] = -5;
			p_drv_cfg->res[i].res_sca.coeffVertical[1] = -2;
			p_drv_cfg->res[i].res_sca.coeffVertical[2] = 21;
			p_drv_cfg->res[i].res_sca.coeffVertical[3] = 36;
		} else if (p_drv_cfg->res[i].res_sca.sca_vstep <= 3072) {
			p_drv_cfg->res[i].res_sca.coeffVertical[0] = -3;
			p_drv_cfg->res[i].res_sca.coeffVertical[1] = 4;
			p_drv_cfg->res[i].res_sca.coeffVertical[2] = 18;
			p_drv_cfg->res[i].res_sca.coeffVertical[3] = 26;
		} else if (p_drv_cfg->res[i].res_sca.sca_vstep <= 4096) {
			p_drv_cfg->res[i].res_sca.coeffVertical[0] = -1;
			p_drv_cfg->res[i].res_sca.coeffVertical[1] = 7;
			p_drv_cfg->res[i].res_sca.coeffVertical[2] = 16;
			p_drv_cfg->res[i].res_sca.coeffVertical[3] = 20;
		} else {
			p_drv_cfg->res[i].res_sca.coeffVertical[0] = 4;
			p_drv_cfg->res[i].res_sca.coeffVertical[1] = 9;
			p_drv_cfg->res[i].res_sca.coeffVertical[2] = 12;
			p_drv_cfg->res[i].res_sca.coeffVertical[3] = 14;
		}


		p_drv_cfg->res[i].res_sca.sca_hsca_divisor = vpe_get_sca_hsca_vsca_divisor(p_vpe_info->dce_param.dewarp_width, p_drv_cfg->res[i].res_size.des_width);
		p_drv_cfg->res[i].res_sca.sca_vsca_divisor = vpe_get_sca_vsca_divisor_chroma(p_vpe_info->dce_param.dewarp_height, p_drv_cfg->res[i].res_size.des_height, p_drv_cfg->res[i].res_ctl.des_yuv420);
		p_drv_cfg->res[i].res_sca.sca_vavg_pxl_msk = vpe_get_sca_vavg_pxl_msk_chroma(p_vpe_info->dce_param.dewarp_height, p_drv_cfg->res[i].res_size.des_height, p_drv_cfg->res[i].res_ctl.des_yuv420);
		p_drv_cfg->res[i].res_sca.sca_havg_pxl_msk = vpe_get_sca_havg_vavg_pxl_msk(p_vpe_info->dce_param.dewarp_width, p_drv_cfg->res[i].res_size.des_width);
#else
		vpe_drv_sca_cal_ratio(p_drv_cfg);
#endif

	}


#if 0
	p_drv_cfg->glo_ctl.col_num = 3;
	p_drv_cfg->col_sz[0].proc_width = 968;
	p_drv_cfg->col_sz[0].proc_x_start = 0;
	p_drv_cfg->col_sz[0].col_x_start = 0;

	p_drv_cfg->col_sz[1].proc_width = 976;
	p_drv_cfg->col_sz[1].proc_x_start = 0;
	p_drv_cfg->col_sz[1].col_x_start = 952;

	p_drv_cfg->col_sz[2].proc_width = 976;
	p_drv_cfg->col_sz[2].proc_x_start = 0;
	p_drv_cfg->col_sz[2].col_x_start = 1912;

	p_drv_cfg->col_sz[3].proc_width = 968;
	p_drv_cfg->col_sz[3].proc_x_start = 0;
	p_drv_cfg->col_sz[3].col_x_start = 2872;



	p_drv_cfg->glo_sz.src_width = 2880;
	p_drv_cfg->glo_sz.src_height = 2880;
	p_drv_cfg->glo_sz.presca_merge_width = 3840;
	p_drv_cfg->glo_sz.dce_width = 3840;
	p_drv_cfg->glo_sz.dce_height = 3840;
	p_drv_cfg->glo_sz.proc_height = 2880;
	p_drv_cfg->glo_sz.proc_y_start = 0;


	p_drv_cfg->res[0].res_ctl.sca_en = 1;
	p_drv_cfg->res[0].res_ctl.sca_bypass_en = 1;
	p_drv_cfg->res[0].res_ctl.tc_en = 1;
	p_drv_cfg->res[0].res_ctl.sca_chroma_sel = 0;
	p_drv_cfg->res[0].res_ctl.des_drt = 0;
	p_drv_cfg->res[0].res_ctl.des_yuv420 = 1;
	p_drv_cfg->res[0].res_ctl.des_uv_swap = 0;
	p_drv_cfg->res[0].res_ctl.out_bg_sel = 0;
	p_drv_cfg->res[0].res_ctl.des_dp0_ycc_enc_en = 0;
	p_drv_cfg->res[0].res_ctl.des_dp0_chrw = 1;
	p_drv_cfg->res[0].res_ctl.des_dp0_format = 0;

	p_drv_cfg->res[0].res_size.sca_height = 2160;
	p_drv_cfg->res[0].res_size.des_width = 3840;
	p_drv_cfg->res[0].res_size.des_height = 2160;
	p_drv_cfg->res[0].res_size.out_y_start = 0;
	p_drv_cfg->res[0].res_size.out_height = 2160;
	p_drv_cfg->res[0].res_size.rlt_y_start = 0;
	p_drv_cfg->res[0].res_size.rlt_height = 2160;
	p_drv_cfg->res[0].res_size.pip_y_start = 0;
	p_drv_cfg->res[0].res_size.pip_height = 0;

	p_drv_cfg->res[0].res_sca_crop.sca_crop_y_start = 0;
	p_drv_cfg->res[0].res_sca_crop.sca_crop_height = 2160;

	p_drv_cfg->res[0].res_sca.sca_y_luma_algo_en = 0;
	p_drv_cfg->res[0].res_sca.sca_x_luma_algo_en = 0;
	p_drv_cfg->res[0].res_sca.sca_y_chroma_algo_en = 0;
	p_drv_cfg->res[0].res_sca.sca_x_chroma_algo_en = 0;
	p_drv_cfg->res[0].res_sca.sca_map_sel = 1;
	p_drv_cfg->res[0].res_sca.sca_hstep = 1024;
	p_drv_cfg->res[0].res_sca.sca_vstep = 1024;
	p_drv_cfg->res[0].res_sca.sca_hsca_divisor = 4096;
	p_drv_cfg->res[0].res_sca.sca_havg_pxl_msk = 8;
	p_drv_cfg->res[0].res_sca.sca_vsca_divisor = 1365;
	p_drv_cfg->res[0].res_sca.sca_vavg_pxl_msk = 56;
	p_drv_cfg->res[0].res_sca.coeffVertical[0] = 0;
	p_drv_cfg->res[0].res_sca.coeffVertical[1] = 0;
	p_drv_cfg->res[0].res_sca.coeffVertical[2] = 0;
	p_drv_cfg->res[0].res_sca.coeffVertical[3] = 64;
	p_drv_cfg->res[0].res_sca.coeffHorizontal[0] = 0;
	p_drv_cfg->res[0].res_sca.coeffHorizontal[1] = 0;
	p_drv_cfg->res[0].res_sca.coeffHorizontal[2] = 0;
	p_drv_cfg->res[0].res_sca.coeffHorizontal[3] = 64;

	p_drv_cfg->res[0].res_tc.tc_crop_y_start = 0;
	p_drv_cfg->res[0].res_tc.tc_crop_height = 2160;

	p_drv_cfg->res[0].res_col_size[0].sca_width = 968;
	p_drv_cfg->res[0].res_col_size[0].sca_hstep_oft = 0;
	p_drv_cfg->res[0].res_col_size[0].out_x_start = 0;
	p_drv_cfg->res[0].res_col_size[0].out_width = 960;
	p_drv_cfg->res[0].res_col_size[0].rlt_x_start = 0;
	p_drv_cfg->res[0].res_col_size[0].rlt_width = 960;
	p_drv_cfg->res[0].res_col_size[0].pip_x_start = 0;
	p_drv_cfg->res[0].res_col_size[0].pip_width = 0;
	p_drv_cfg->res[0].res_col_size[0].sca_crop_x_start = 0;
	p_drv_cfg->res[0].res_col_size[0].sca_crop_width = 968;
	p_drv_cfg->res[0].res_col_size[0].tc_crop_x_start = 0;
	p_drv_cfg->res[0].res_col_size[0].tc_crop_width = 960;
	p_drv_cfg->res[0].res_col_size[0].tc_crop_skip = 0;

	p_drv_cfg->res[0].res_col_size[1].sca_width = 976;
	p_drv_cfg->res[0].res_col_size[1].sca_hstep_oft = 0;
	p_drv_cfg->res[0].res_col_size[1].out_x_start = 960;
	p_drv_cfg->res[0].res_col_size[1].out_width = 960;
	p_drv_cfg->res[0].res_col_size[1].rlt_x_start = 0;
	p_drv_cfg->res[0].res_col_size[1].rlt_width = 960;
	p_drv_cfg->res[0].res_col_size[1].pip_x_start = 0;
	p_drv_cfg->res[0].res_col_size[1].pip_width = 0;
	p_drv_cfg->res[0].res_col_size[1].sca_crop_x_start = 0;
	p_drv_cfg->res[0].res_col_size[1].sca_crop_width = 976;
	p_drv_cfg->res[0].res_col_size[1].tc_crop_x_start = 8;
	p_drv_cfg->res[0].res_col_size[1].tc_crop_width = 960;
	p_drv_cfg->res[0].res_col_size[1].tc_crop_skip = 0;

	p_drv_cfg->res[0].res_col_size[2].sca_width = 976;
	p_drv_cfg->res[0].res_col_size[2].sca_hstep_oft = 0;
	p_drv_cfg->res[0].res_col_size[2].out_x_start = 1920;
	p_drv_cfg->res[0].res_col_size[2].out_width = 960;
	p_drv_cfg->res[0].res_col_size[2].rlt_x_start = 0;
	p_drv_cfg->res[0].res_col_size[2].rlt_width = 960;
	p_drv_cfg->res[0].res_col_size[2].pip_x_start = 0;
	p_drv_cfg->res[0].res_col_size[2].pip_width = 0;
	p_drv_cfg->res[0].res_col_size[2].sca_crop_x_start = 0;
	p_drv_cfg->res[0].res_col_size[2].sca_crop_width = 976;
	p_drv_cfg->res[0].res_col_size[2].tc_crop_x_start = 8;
	p_drv_cfg->res[0].res_col_size[2].tc_crop_width = 960;
	p_drv_cfg->res[0].res_col_size[2].tc_crop_skip = 0;

	p_drv_cfg->res[0].res_col_size[3].sca_width = 968;
	p_drv_cfg->res[0].res_col_size[3].sca_hstep_oft = 0;
	p_drv_cfg->res[0].res_col_size[3].out_x_start = 2880;
	p_drv_cfg->res[0].res_col_size[3].out_width = 960;
	p_drv_cfg->res[0].res_col_size[3].rlt_x_start = 0;
	p_drv_cfg->res[0].res_col_size[3].rlt_width = 960;
	p_drv_cfg->res[0].res_col_size[3].pip_x_start = 0;
	p_drv_cfg->res[0].res_col_size[3].pip_width = 0;
	p_drv_cfg->res[0].res_col_size[3].sca_crop_x_start = 0;
	p_drv_cfg->res[0].res_col_size[3].sca_crop_width = 968;
	p_drv_cfg->res[0].res_col_size[3].tc_crop_x_start = 8;
	p_drv_cfg->res[0].res_col_size[3].tc_crop_width = 960;
	p_drv_cfg->res[0].res_col_size[3].tc_crop_skip = 0;
#endif
	return 0;
}
#endif

#if 0
int vpe_drv_sca_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg)
{
	INT32 i = 0, j = 0, k = 0;
	INT32 align_rlt_w = 0, align_rlt_h = 0;
	INT32 align_out_w = 0, align_out_h = 0;
	INT32 src_in_w = 0, src_in_h = 0;
	INT32 align_rlt_w_dup = 0, align_rlt_h_dup = 0;
	INT32 align_out_w_dup = 0, align_out_h_dup = 0;
	//INT32 remain_w = 0;
	INT32 col_overlap = 0;

	INT32 start_pip_out_x = 0, dest_pip_out_x = 0;
	INT32 start_rlt_out_x = 0, dest_rlt_out_x = 0;

	INT32 sca_o_stx = 0;
	INT32 fs_x = 0, fi_x = 0;
	INT32 fs_0 = 0, fi_0 = 0;

	INT32 temp = 0;
	INT32 temp1 = 0, proc_w_align = 0;
	INT32 temp_proc_x = 0;


	p_drv_cfg->glo_sz.presca_merge_width = p_vpe_info->dce_param.dewarp_width;

	p_drv_cfg->glo_sz.src_width = p_vpe_info->glb_img_info.src_width;
	p_drv_cfg->glo_sz.src_height = p_vpe_info->glb_img_info.src_height;


	p_drv_cfg->glo_sz.proc_height = p_vpe_info->glb_img_info.src_in_h;
	p_drv_cfg->glo_sz.proc_y_start = p_vpe_info->glb_img_info.src_in_y;
	src_in_w = p_vpe_info->glb_img_info.src_in_w;
	src_in_h = p_vpe_info->glb_img_info.src_in_h;



	for (i = 0; i < (p_drv_cfg->glo_ctl.col_num + 1); i++) {
		p_drv_cfg->col_sz[i].proc_width = p_vpe_info->col_info.col_size_info[i].proc_width;
		p_drv_cfg->col_sz[i].proc_x_start = p_vpe_info->col_info.col_size_info[i].proc_x_start;

		p_drv_cfg->col_sz[i].col_x_start = p_vpe_info->col_info.col_size_info[i].col_x_start;
	}

#if KDRV_VPE_SUPPORT_DEWARP
	if (p_drv_cfg->glo_ctl.dce_en) {

		p_drv_cfg->glo_sz.dce_width = p_vpe_info->dce_param.dewarp_width;

		p_drv_cfg->glo_sz.dce_height = p_vpe_info->dce_param.dewarp_height;
	}
#endif

	for (i = 0; i < VPE_MAX_RES; i++) {

		p_drv_cfg->res[i].res_ctl.sca_en = 0;
		p_drv_cfg->res[i].res_ctl.tc_en = 1;

		align_rlt_w = 0;
		align_rlt_h = 0;
		align_out_w = 0;
		align_out_h = 0;

		if (p_vpe_info->out_img_info[i].out_dup_num) {
			align_rlt_w_dup = 0;
			align_rlt_h_dup = 0;
			align_out_w_dup = 0;
			align_out_h_dup = 0;

			for (j = 0; j < p_vpe_info->out_img_info[i].out_dup_num; j++) {
				align_rlt_w_dup = vpe_drv_get_align(p_vpe_info, ALIGM_TYPE_RLT_W, i, j);
				align_rlt_h_dup = vpe_drv_get_align(p_vpe_info, ALIGM_TYPE_RLT_H, i, j);
				align_out_w_dup = vpe_drv_get_align(p_vpe_info, ALIGM_TYPE_OUT_W, i, j);
				align_out_h_dup = vpe_drv_get_align(p_vpe_info, ALIGM_TYPE_OUT_H, i, j);

				if (align_rlt_w < align_rlt_w_dup) {
					align_rlt_w = align_rlt_w_dup;
				}
				if (align_rlt_h < align_rlt_h_dup) {
					align_rlt_h = align_rlt_h_dup;
				}
				if (align_out_w < align_out_w_dup) {
					align_out_w = align_out_w_dup;
				}
				if (align_out_h < align_out_h_dup) {
					align_out_h = align_out_h_dup;
				}
			}

			if (!align_out_w || !align_out_h) {
				vpe_err("vpe_drv_sca_config: config error align_out_w(%d) align_out_h(%d)\n", align_out_w, align_out_h);
				return -1;
			}
			p_drv_cfg->res[i].res_ctl.sca_en = 1;
			p_drv_cfg->res[i].res_ctl.out_bg_sel = p_vpe_info->out_img_info[i].out_bg_sel;
			p_drv_cfg->res[i].res_ctl.des_dp0_chrw = 1;
			p_drv_cfg->res[i].res_ctl.des_dp0_format = 0;

			p_drv_cfg->res[i].res_size.des_width = p_vpe_info->out_img_info[i].res_dim_info.des_width;
			p_drv_cfg->res[i].res_size.des_height = p_vpe_info->out_img_info[i].res_dim_info.des_height;

			if (!p_drv_cfg->glo_ctl.col_num && p_vpe_info->out_img_info[i].res_dim_info.sca_crop_w && p_vpe_info->out_img_info[i].res_dim_info.sca_crop_h) {

				src_in_w = p_vpe_info->out_img_info[i].res_dim_info.sca_crop_w;
				src_in_h = p_vpe_info->out_img_info[i].res_dim_info.sca_crop_h;

			}

			if ((src_in_w == p_vpe_info->out_img_info[i].res_dim_info.des_rlt_w) &&
				(src_in_h == p_vpe_info->out_img_info[i].res_dim_info.des_rlt_h)) {
				p_drv_cfg->res[i].res_ctl.sca_bypass_en = 1;
				p_drv_cfg->res[i].res_sca.sca_hstep = 1024;
				p_drv_cfg->res[i].res_sca.sca_vstep = 1024;
			} else {
				p_drv_cfg->res[i].res_ctl.sca_bypass_en = 0;
				p_drv_cfg->res[i].res_sca.sca_hstep = (INT32)(src_in_w * 1024 / p_vpe_info->out_img_info[i].res_dim_info.des_rlt_w);
				p_drv_cfg->res[i].res_sca.sca_vstep = (INT32)(src_in_h * 1024 / p_vpe_info->out_img_info[i].res_dim_info.des_rlt_h);
			}

			if (p_drv_cfg->glo_ctl.col_num) {
				p_drv_cfg->res[i].res_ctl.sca_crop_en = 0;
				if (p_drv_cfg->res[i].res_ctl.des_dp0_ycc_enc_en) {
					col_overlap = COLUMN_SCE_OVERLAP;
				} else {
					col_overlap = COLUMN_OVERLAP;
				}

				for (j = 0; j < (p_drv_cfg->glo_ctl.col_num + 1); j++) {
					/* out_width / out_x_stat */
#if 1 // Support RLT < OUT under column mode
					p_drv_cfg->res[i].res_col_size[j].out_width = ((p_drv_cfg->col_sz[j].proc_width) * p_vpe_info->out_img_info[i].res_dim_info.des_rlt_w / src_in_w) ;
#else
					p_drv_cfg->res[i].res_col_size[j].out_width = ((p_drv_cfg->col_sz[j].proc_width) * p_vpe_info->out_img_info[i].res_dim_info.des_out_w / src_in_w) ;
#endif

					if (!j) {
						p_drv_cfg->res[i].res_col_size[j].out_width = p_drv_cfg->res[i].res_col_size[j].out_width - (col_overlap * 1 * 1024 / p_drv_cfg->res[i].res_sca.sca_hstep);
						if (p_drv_cfg->res[i].res_col_size[j].out_width % align_out_w) {
							p_drv_cfg->res[i].res_col_size[j].out_width = p_drv_cfg->res[i].res_col_size[j].out_width - (p_drv_cfg->res[i].res_col_size[j].out_width % align_out_w);
						}

						p_drv_cfg->res[i].res_col_size[j].out_x_start = p_vpe_info->out_img_info[i].res_dim_info.des_out_x;


					} else if (j == p_drv_cfg->glo_ctl.col_num) {
						p_drv_cfg->res[i].res_col_size[j].out_width = p_vpe_info->out_img_info[i].res_dim_info.des_out_w - p_drv_cfg->res[i].res_col_size[j - 1].out_width - p_drv_cfg->res[i].res_col_size[j - 1].out_x_start + p_drv_cfg->res[i].res_col_size[0].out_x_start;
						if (p_drv_cfg->res[i].res_col_size[j].out_width % align_out_w) {
							p_drv_cfg->res[i].res_col_size[j].out_width = p_drv_cfg->res[i].res_col_size[j].out_width - (p_drv_cfg->res[i].res_col_size[j].out_width % align_out_w);
						}
						p_drv_cfg->res[i].res_col_size[j].out_x_start = p_drv_cfg->res[i].res_col_size[j - 1].out_x_start + p_drv_cfg->res[i].res_col_size[j - 1].out_width ;
					} else {
						p_drv_cfg->res[i].res_col_size[j].out_width = p_drv_cfg->res[i].res_col_size[j].out_width - (col_overlap * 2 * 1024 / p_drv_cfg->res[i].res_sca.sca_hstep);
						if (p_drv_cfg->res[i].res_col_size[j].out_width % align_out_w) {
							p_drv_cfg->res[i].res_col_size[j].out_width = p_drv_cfg->res[i].res_col_size[j].out_width - (p_drv_cfg->res[i].res_col_size[j].out_width % align_out_w);
						}

						p_drv_cfg->res[i].res_col_size[j].out_x_start = p_drv_cfg->res[i].res_col_size[j - 1].out_x_start + p_drv_cfg->res[i].res_col_size[j - 1].out_width;
					}

					/* sca_width */
					p_drv_cfg->res[i].res_col_size[j].sca_width = (p_drv_cfg->col_sz[j].proc_width * 1024) / p_drv_cfg->res[i].res_sca.sca_hstep;
#if 1 // Support RLT < OUT under column mode
					if (p_drv_cfg->res[i].res_col_size[j].sca_width % 8) {
						p_drv_cfg->res[i].res_col_size[j].sca_width = p_drv_cfg->res[i].res_col_size[j].sca_width + (8 - (p_drv_cfg->res[i].res_col_size[j].sca_width % 8));
					}
#endif

					/* tc_crop_width/tc_crop_x_start and rlt_width/rlt_x_start */
#if 1 // Support RLT < OUT under column mode

					start_rlt_out_x = p_vpe_info->out_img_info[i].res_dim_info.des_rlt_x + p_vpe_info->out_img_info[i].res_dim_info.des_out_x;
					dest_rlt_out_x = p_vpe_info->out_img_info[i].res_dim_info.des_rlt_x + p_vpe_info->out_img_info[i].res_dim_info.des_rlt_w + p_vpe_info->out_img_info[i].res_dim_info.des_out_x;


					//for(j = 0; j < (p_drv_cfg->glo_ctl.col_num + 1); j++) {

					if ((p_drv_cfg->res[i].res_col_size[j].out_x_start <= start_rlt_out_x) && ((p_drv_cfg->res[i].res_col_size[j].out_x_start + p_drv_cfg->res[i].res_col_size[j].out_width) > start_rlt_out_x)) {
						p_drv_cfg->res[i].res_col_size[j].rlt_x_start = start_rlt_out_x - p_drv_cfg->res[i].res_col_size[j].out_x_start;
						if ((p_drv_cfg->res[i].res_col_size[j].out_x_start + p_drv_cfg->res[i].res_col_size[j].out_width) >= dest_rlt_out_x) {
							p_drv_cfg->res[i].res_col_size[j].rlt_width = p_vpe_info->out_img_info[i].res_dim_info.des_rlt_w;
						} else {
							p_drv_cfg->res[i].res_col_size[j].rlt_width = (p_drv_cfg->res[i].res_col_size[j].out_width - p_drv_cfg->res[i].res_col_size[j].rlt_x_start);
						}
					} else if ((p_drv_cfg->res[i].res_col_size[j].out_x_start >= start_rlt_out_x) && ((p_drv_cfg->res[i].res_col_size[j].out_x_start + p_drv_cfg->res[i].res_col_size[j].out_width) <= dest_rlt_out_x)) {
						p_drv_cfg->res[i].res_col_size[j].rlt_x_start = 0;
						p_drv_cfg->res[i].res_col_size[j].rlt_width = p_drv_cfg->res[i].res_col_size[j].out_width;
					} else if ((p_drv_cfg->res[i].res_col_size[j].out_x_start < dest_rlt_out_x) && ((p_drv_cfg->res[i].res_col_size[j].out_x_start + p_drv_cfg->res[i].res_col_size[j].out_width) >= dest_rlt_out_x)) {
						p_drv_cfg->res[i].res_col_size[j].rlt_x_start = 0;
						p_drv_cfg->res[i].res_col_size[j].rlt_width = dest_rlt_out_x - p_drv_cfg->res[i].res_col_size[j].out_x_start;
					}


					//}
					p_drv_cfg->res[i].res_col_size[j].tc_crop_width = p_drv_cfg->res[i].res_col_size[j].rlt_width;
					if (!p_drv_cfg->res[i].res_col_size[j].tc_crop_width) {
						p_drv_cfg->res[i].res_col_size[j].tc_crop_width = 4;
						p_drv_cfg->res[i].res_col_size[j].rlt_x_start = p_drv_cfg->res[i].res_col_size[j].out_width;
					}

					if (!j) {
						p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start = 0;
					} else if (!p_drv_cfg->col_sz[j].proc_x_start) {
						p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start = (col_overlap * 1024 / p_drv_cfg->res[i].res_sca.sca_hstep);
						if (p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start % 2) {
							p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start = p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start + 1;
						}
					} else {

						p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start = (col_overlap * 1024 / p_drv_cfg->res[i].res_sca.sca_hstep);
						if (p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start % 2) {
							p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start = p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start + 1;
						}
						//p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start = p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start + 2;

#if 1
						sca_o_stx = p_drv_cfg->res[i].res_col_size[j].out_x_start - p_drv_cfg->res[i].res_col_size[0].out_x_start - p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start;

						fs_x = (p_drv_cfg->res[i].res_sca.sca_hstep / 2) + (sca_o_stx * p_drv_cfg->res[i].res_sca.sca_hstep);
						fi_x = (fs_x - 512) / 1024;
						if (fi_x < 0) {
							fi_x = 0;
						} else if (fi_x > p_drv_cfg->glo_sz.src_width - 1) {
							fi_x = p_vpe_info->glb_img_info.src_in_w - 1;
						}

						fs_0 = p_drv_cfg->res[i].res_sca.sca_hstep / 2;
						fi_0 = (fs_0 - 512) / 1024;
						if (fi_0 < 0) {
							fi_0 = 0;
						} else if (fi_0 > p_drv_cfg->glo_sz.src_width - 1) {
							fi_0 = p_vpe_info->glb_img_info.src_in_w - 1;
						}


						proc_w_align = 8;

						temp_proc_x = 0;
						if ((p_drv_cfg->col_sz[j].proc_x_start - p_drv_cfg->col_sz[0].proc_x_start) > fi_x) {
#if 1
							temp = p_drv_cfg->col_sz[j].proc_x_start - p_drv_cfg->col_sz[0].proc_x_start;
							temp_proc_x = p_drv_cfg->col_sz[j].proc_x_start;
							p_drv_cfg->col_sz[j].proc_x_start = fi_x;

							if (p_drv_cfg->col_sz[j].proc_x_start % proc_w_align) {
								p_drv_cfg->col_sz[j].proc_x_start = p_drv_cfg->col_sz[j].proc_x_start - (p_drv_cfg->col_sz[j].proc_x_start % proc_w_align);
								p_drv_cfg->col_sz[j].proc_width = p_drv_cfg->col_sz[j].proc_width + (temp - p_drv_cfg->col_sz[j].proc_x_start);
								if (p_drv_cfg->col_sz[j].proc_width % 8) {
									p_drv_cfg->col_sz[j].proc_width = ((p_drv_cfg->col_sz[j].proc_width / 8) + 1) * 8;

								}
							}

							p_drv_cfg->col_sz[j].proc_x_start = p_drv_cfg->col_sz[j].proc_x_start + p_drv_cfg->col_sz[0].proc_x_start;

							p_drv_cfg->res[i].res_col_size[j].sca_width = (p_drv_cfg->col_sz[j].proc_width * 1024) / p_drv_cfg->res[i].res_sca.sca_hstep;

							if (p_drv_cfg->res[i].res_col_size[j].sca_width % 8) {
								p_drv_cfg->res[i].res_col_size[j].sca_width = p_drv_cfg->res[i].res_col_size[j].sca_width + (8 - (p_drv_cfg->res[i].res_col_size[j].sca_width % 8));
							}

#else
							temp = p_drv_cfg->col_sz[j].proc_x_start;

							p_drv_cfg->col_sz[j].proc_x_start = fi_x + p_drv_cfg->col_sz[0].proc_x_start;

#if 1
							if (p_drv_cfg->col_sz[j].proc_x_start % proc_w_align) {
								p_drv_cfg->col_sz[j].proc_x_start = p_drv_cfg->col_sz[j].proc_x_start - (p_drv_cfg->col_sz[j].proc_x_start % proc_w_align);
								p_drv_cfg->col_sz[j].proc_width = p_drv_cfg->col_sz[j].proc_width + (temp - p_drv_cfg->col_sz[j].proc_x_start);
							}
#else
							if (p_drv_cfg->col_sz[j].proc_x_start % 2) {
								p_drv_cfg->col_sz[j].proc_x_start = p_drv_cfg->col_sz[j].proc_x_start - 1;
							}
#endif
#endif
						}

						p_drv_cfg->res[i].res_col_size[j].sca_hstep_oft = ((fs_x - 512) - (fi_x * 1024)) - ((fs_0 - 512) - (fi_0 * 1024));
						temp1 = (fi_x - fi_0 - p_drv_cfg->col_sz[j].proc_x_start + p_drv_cfg->col_sz[0].proc_x_start);

						if (temp1 > 0) {
							if (p_drv_cfg->res[i].res_ctl.sca_crop_en) {
								p_drv_cfg->res[i].res_col_size[j].sca_crop_x_start = temp1;
								p_drv_cfg->res[i].res_col_size[j].sca_crop_width = p_drv_cfg->col_sz[j].proc_x_start + p_drv_cfg->col_sz[j].proc_width - p_drv_cfg->res[i].res_col_size[j].sca_crop_x_start;
							} else {
								p_drv_cfg->res[i].res_ctl.sca_crop_en = 1;
								for (k = 0; k < j; k++) {
									p_drv_cfg->res[i].res_col_size[k].sca_crop_width = p_drv_cfg->res[i].res_col_size[k].sca_width;
								}

								p_drv_cfg->res[i].res_col_size[j].sca_crop_x_start = temp1;
								p_drv_cfg->res[i].res_col_size[j].sca_crop_width = p_drv_cfg->col_sz[j].proc_x_start + p_drv_cfg->col_sz[j].proc_width - p_drv_cfg->res[i].res_col_size[j].sca_crop_x_start;

							}

						} else if (temp1 < 0) {
							p_drv_cfg->res[i].res_col_size[j].sca_hstep_oft = p_drv_cfg->res[i].res_col_size[j].sca_hstep_oft + (temp1 * 1024);
						}

#endif


					}
#else

					p_drv_cfg->res[i].res_col_size[j].rlt_width = p_drv_cfg->res[i].res_col_size[j].out_width;
					p_drv_cfg->res[i].res_col_size[j].tc_crop_width = p_drv_cfg->res[i].res_col_size[j].rlt_width;

					if (!j) {
						if ((p_vpe_info->out_img_info[i].res_dim_info.des_rlt_x + p_drv_cfg->res[i].res_col_size[j].rlt_width) > p_drv_cfg->res[i].res_col_size[j].out_width) {
							p_drv_cfg->res[i].res_col_size[j].rlt_x_start = 0;
						} else {
							p_drv_cfg->res[i].res_col_size[j].rlt_x_start = p_vpe_info->out_img_info[i].res_dim_info.des_rlt_x;
						}

						p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start = 0;
					} else if (j == p_drv_cfg->glo_ctl.col_num) {
						p_drv_cfg->res[i].res_col_size[j].rlt_x_start = 0;
						p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start = p_drv_cfg->res[i].res_col_size[j].sca_width - p_drv_cfg->res[i].res_col_size[j].tc_crop_width;
					} else {
						p_drv_cfg->res[i].res_col_size[j].rlt_x_start = 0;
						p_drv_cfg->res[i].res_col_size[j].tc_crop_x_start = (col_overlap * 1024 / p_drv_cfg->res[i].res_sca.sca_hstep);


					}
#endif
				}

#if 1 // Support RLT < OUT under column mode
				if (p_drv_cfg->glo_ctl.col_num) {
					if (p_drv_cfg->res[i].res_col_size[p_drv_cfg->glo_ctl.col_num].out_width > 2048) {
						p_drv_cfg->res[i].res_col_size[p_drv_cfg->glo_ctl.col_num].out_width = 2048;
#if 1
						p_drv_cfg->glo_ctl.col_num = p_drv_cfg->glo_ctl.col_num + 1;
						p_drv_cfg->col_sz[p_drv_cfg->glo_ctl.col_num].proc_width = p_drv_cfg->col_sz[p_drv_cfg->glo_ctl.col_num - 1].proc_width;
						p_drv_cfg->col_sz[p_drv_cfg->glo_ctl.col_num].proc_x_start = p_drv_cfg->col_sz[p_drv_cfg->glo_ctl.col_num - 1].proc_x_start;

						p_drv_cfg->res[i].res_col_size[p_drv_cfg->glo_ctl.col_num].sca_width = p_drv_cfg->res[i].res_col_size[p_drv_cfg->glo_ctl.col_num - 1].sca_width;

						p_drv_cfg->res[i].res_col_size[p_drv_cfg->glo_ctl.col_num].out_width = p_vpe_info->out_img_info[i].res_dim_info.des_out_w - p_drv_cfg->res[i].res_col_size[p_drv_cfg->glo_ctl.col_num - 1].out_width - p_drv_cfg->res[i].res_col_size[p_drv_cfg->glo_ctl.col_num - 1].out_x_start + p_drv_cfg->res[i].res_col_size[0].out_x_start;
						p_drv_cfg->res[i].res_col_size[p_drv_cfg->glo_ctl.col_num].out_x_start = p_drv_cfg->res[i].res_col_size[p_drv_cfg->glo_ctl.col_num - 1].out_x_start + p_drv_cfg->res[i].res_col_size[p_drv_cfg->glo_ctl.col_num - 1].out_width ;

						p_drv_cfg->res[i].res_col_size[p_drv_cfg->glo_ctl.col_num].tc_crop_width = 4;
						p_drv_cfg->res[i].res_col_size[p_drv_cfg->glo_ctl.col_num].tc_crop_x_start = 0;
						p_drv_cfg->res[i].res_col_size[p_drv_cfg->glo_ctl.col_num].rlt_width = 0;
						p_drv_cfg->res[i].res_col_size[p_drv_cfg->glo_ctl.col_num].rlt_x_start = p_drv_cfg->res[i].res_col_size[p_drv_cfg->glo_ctl.col_num].out_width;

#endif
					}
				}
#endif
				/* sca_height */
				p_drv_cfg->res[i].res_size.sca_height = (p_drv_cfg->glo_sz.proc_height * 1024) / p_drv_cfg->res[i].res_sca.sca_vstep;
				if (p_drv_cfg->res[i].res_size.sca_height % align_rlt_h) {
					p_drv_cfg->res[i].res_size.sca_height = p_drv_cfg->res[i].res_size.sca_height - (p_drv_cfg->res[i].res_size.sca_height % align_rlt_h);
				}

				if (p_drv_cfg->res[i].res_size.sca_height > p_vpe_info->out_img_info[i].res_dim_info.des_out_h) {
					p_drv_cfg->res[i].res_size.sca_height = p_vpe_info->out_img_info[i].res_dim_info.des_out_h;
				}
			} else {
				p_drv_cfg->res[i].res_col_size[0].sca_width = p_vpe_info->out_img_info[i].res_dim_info.des_rlt_w;
				p_drv_cfg->res[i].res_size.sca_height = p_vpe_info->out_img_info[i].res_dim_info.des_rlt_h;
#if 1 // 20190724
				if (p_drv_cfg->res[i].res_col_size[0].sca_width % 4) {
					p_drv_cfg->res[i].res_col_size[0].sca_width = p_drv_cfg->res[i].res_col_size[0].sca_width + (4 - (p_drv_cfg->res[i].res_col_size[0].sca_width % 4));
				}
#endif
				p_drv_cfg->res[i].res_col_size[0].out_width = p_vpe_info->out_img_info[i].res_dim_info.des_out_w;
				p_drv_cfg->res[i].res_col_size[0].out_x_start = p_vpe_info->out_img_info[i].res_dim_info.des_out_x;

				p_drv_cfg->res[i].res_col_size[0].rlt_width = p_vpe_info->out_img_info[i].res_dim_info.des_rlt_w;
				p_drv_cfg->res[i].res_col_size[0].rlt_x_start = p_vpe_info->out_img_info[i].res_dim_info.des_rlt_x;

				p_drv_cfg->res[i].res_col_size[0].tc_crop_width = p_vpe_info->out_img_info[i].res_dim_info.des_rlt_w;
				p_drv_cfg->res[i].res_col_size[0].tc_crop_x_start = 0;
			}


			/* tc_crop_height/tc_crop_y_start and rlt_height/rlt_y_start */
			if (p_drv_cfg->res[i].res_ctl.tc_en) {
				p_drv_cfg->res[i].res_tc.tc_crop_height = p_drv_cfg->res[i].res_size.sca_height;
				p_drv_cfg->res[i].res_size.rlt_height = p_drv_cfg->res[i].res_tc.tc_crop_height;
			} else {
				p_drv_cfg->res[i].res_size.rlt_height = p_drv_cfg->res[i].res_size.sca_height;
			}
			p_drv_cfg->res[i].res_tc.tc_crop_y_start = 0;
			p_drv_cfg->res[i].res_size.rlt_y_start = p_vpe_info->out_img_info[i].res_dim_info.des_rlt_y;

			p_drv_cfg->res[i].res_size.out_height = p_vpe_info->out_img_info[i].res_dim_info.des_out_h;
			p_drv_cfg->res[i].res_size.out_y_start = p_vpe_info->out_img_info[i].res_dim_info.des_out_y;

			if (!p_drv_cfg->glo_ctl.col_num) {
				if (!p_vpe_info->out_img_info[i].res_dim_info.sca_crop_w || !p_vpe_info->out_img_info[i].res_dim_info.sca_crop_h) {
					p_drv_cfg->res[i].res_ctl.sca_crop_en = 0;
					p_drv_cfg->res[i].res_col_size[0].sca_crop_width = 0;
					p_drv_cfg->res[i].res_sca_crop.sca_crop_height = 0;
					p_drv_cfg->res[i].res_col_size[0].sca_crop_x_start = 0;
					p_drv_cfg->res[i].res_sca_crop.sca_crop_y_start = 0;
				} else {
					p_drv_cfg->res[i].res_ctl.sca_crop_en = 1;

					p_drv_cfg->res[i].res_col_size[0].sca_crop_width = p_vpe_info->out_img_info[i].res_dim_info.sca_crop_w;
					p_drv_cfg->res[i].res_sca_crop.sca_crop_height = p_vpe_info->out_img_info[i].res_dim_info.sca_crop_h;
					p_drv_cfg->res[i].res_col_size[0].sca_crop_x_start = p_vpe_info->out_img_info[i].res_dim_info.sca_crop_x;
					p_drv_cfg->res[i].res_sca_crop.sca_crop_y_start = p_vpe_info->out_img_info[i].res_dim_info.sca_crop_y;

				}
			}


		}

		if (p_drv_cfg->glo_ctl.col_num && (p_vpe_info->out_img_info[i].res_dim_info.hole_w && p_vpe_info->out_img_info[i].res_dim_info.hole_h)) {

			start_pip_out_x = p_drv_cfg->res[i].res_col_size[0].out_x_start + p_vpe_info->out_img_info[i].res_dim_info.hole_x;
			dest_pip_out_x = p_drv_cfg->res[i].res_col_size[0].out_x_start + p_vpe_info->out_img_info[i].res_dim_info.hole_x + p_vpe_info->out_img_info[i].res_dim_info.hole_w;

			for (j = 0; j < (p_drv_cfg->glo_ctl.col_num + 1); j++) {

				if ((p_drv_cfg->res[i].res_col_size[j].out_x_start <= start_pip_out_x) && ((p_drv_cfg->res[i].res_col_size[j].out_x_start + p_drv_cfg->res[i].res_col_size[j].out_width) > start_pip_out_x)) {
					p_drv_cfg->res[i].res_col_size[j].pip_x_start = start_pip_out_x - p_drv_cfg->res[i].res_col_size[j].out_x_start;
					if ((p_drv_cfg->res[i].res_col_size[j].out_x_start + p_drv_cfg->res[i].res_col_size[j].out_width) >= dest_pip_out_x) {
						p_drv_cfg->res[i].res_col_size[j].pip_width = p_vpe_info->out_img_info[i].res_dim_info.hole_w;
					} else {
						p_drv_cfg->res[i].res_col_size[j].pip_width = (p_drv_cfg->res[i].res_col_size[j].out_width - p_drv_cfg->res[i].res_col_size[j].pip_x_start);
					}
				} else if ((p_drv_cfg->res[i].res_col_size[j].out_x_start >= start_pip_out_x) && ((p_drv_cfg->res[i].res_col_size[j].out_x_start + p_drv_cfg->res[i].res_col_size[j].out_width) <= dest_pip_out_x)) {
					p_drv_cfg->res[i].res_col_size[j].pip_x_start = 0;
					p_drv_cfg->res[i].res_col_size[j].pip_width = p_drv_cfg->res[i].res_col_size[j].out_width;
				} else if ((p_drv_cfg->res[i].res_col_size[j].out_x_start < dest_pip_out_x) && ((p_drv_cfg->res[i].res_col_size[j].out_x_start + p_drv_cfg->res[i].res_col_size[j].out_width) >= dest_pip_out_x)) {
					p_drv_cfg->res[i].res_col_size[j].pip_x_start = 0;
					p_drv_cfg->res[i].res_col_size[j].pip_width = dest_pip_out_x - p_drv_cfg->res[i].res_col_size[j].out_x_start;
				}


			}
			p_drv_cfg->res[i].res_size.pip_height = p_vpe_info->out_img_info[i].res_dim_info.hole_h;
			p_drv_cfg->res[i].res_size.pip_y_start = p_vpe_info->out_img_info[i].res_dim_info.hole_y;
		} else {
			if (p_vpe_info->out_img_info[i].res_dim_info.hole_w && p_vpe_info->out_img_info[i].res_dim_info.hole_h) {
				p_drv_cfg->res[i].res_col_size[0].pip_width = p_vpe_info->out_img_info[i].res_dim_info.hole_w;
				p_drv_cfg->res[i].res_size.pip_height = p_vpe_info->out_img_info[i].res_dim_info.hole_h;
				p_drv_cfg->res[i].res_col_size[0].pip_x_start = p_vpe_info->out_img_info[i].res_dim_info.hole_x;
				p_drv_cfg->res[i].res_size.pip_y_start = p_vpe_info->out_img_info[i].res_dim_info.hole_y;
			} else {
				for (j = 0; j < (p_drv_cfg->glo_ctl.col_num + 1); j++) {
					p_drv_cfg->res[i].res_col_size[j].pip_width = 0;
					p_drv_cfg->res[i].res_col_size[j].pip_x_start = 0;
				}
				p_drv_cfg->res[i].res_size.pip_height = 0;
				p_drv_cfg->res[i].res_size.pip_y_start = 0;
			}
		}

		if (p_vpe_info->out_img_info[i].res_sca_info.sca_iq_en) {
			p_drv_cfg->res[i].res_sca.sca_iq_en = 1;
			p_drv_cfg->res[i].res_sca.sca_y_luma_algo_en = p_vpe_info->out_img_info[i].res_sca_info.sca_y_luma_algo_en;
			p_drv_cfg->res[i].res_sca.sca_x_luma_algo_en = p_vpe_info->out_img_info[i].res_sca_info.sca_x_luma_algo_en;
			p_drv_cfg->res[i].res_sca.sca_y_chroma_algo_en = p_vpe_info->out_img_info[i].res_sca_info.sca_y_chroma_algo_en;
			p_drv_cfg->res[i].res_sca.sca_x_chroma_algo_en = p_vpe_info->out_img_info[i].res_sca_info.sca_x_chroma_algo_en;
			p_drv_cfg->res[i].res_sca.sca_map_sel = p_vpe_info->out_img_info[i].res_sca_info.sca_map_sel;
			for (j = 0; j < 4; j++) {
				p_drv_cfg->res[i].res_sca.coeffHorizontal[j] = p_vpe_info->out_img_info[i].res_sca_info.sca_ceff_h[j];
				p_drv_cfg->res[i].res_sca.coeffVertical[j] = p_vpe_info->out_img_info[i].res_sca_info.sca_ceff_v[j];
			}
		} else {
			p_drv_cfg->res[i].res_sca.sca_iq_en = 0;
		}

	}

	vpe_drv_sca_cal_ratio(p_drv_cfg);

	return 0;
}
#endif

void vpe_set_src_column_reg(uint32_t col_nums, VPE_COL_SZ_CFG *p_col_sz)
{
	uint32_t col_idx = 0;

	if (p_col_sz != NULL) {
		for (col_idx = 0; col_idx < col_nums; col_idx++) {
			vpe_set_in_src_col_reg(col_idx, &(p_col_sz[col_idx]));
		}
	}
}

VPE_RES_COL_SIZE_CFG g_get_res_col_info = {0};

void vpe_set_res_reg(UINT32 col_num, VPE_RES_CFG *p_res)
{
	uint32_t res_idx = 0, col_idx = 0;

	if (p_res != NULL) {
		for (res_idx = 0; res_idx < VPE_TOTAL_RES; res_idx++) {

			vpe_set_res_ctl_enable_reg(res_idx, p_res[res_idx].res_ctl.sca_en);

			if (p_res[res_idx].res_ctl.sca_en == 1) {
				vpe_set_res_ctl_reg(res_idx, &(p_res[res_idx].res_ctl));

				vpe_set_res_gbl_size_reg(res_idx, &(p_res[res_idx].res_size), &(p_res[res_idx].res_sca_crop), &(p_res[res_idx].res_tc));

				vpe_set_sca_factor_reg(res_idx, &(p_res[res_idx].res_sca));

				for (col_idx = 0; col_idx < (col_num + 1); col_idx++) {
					g_get_res_col_info = p_res[res_idx].res_col_size[col_idx];
					vpe_set_res_col_size_reg(res_idx, col_idx, &g_get_res_col_info);
				}
			}
		}
	}
}


