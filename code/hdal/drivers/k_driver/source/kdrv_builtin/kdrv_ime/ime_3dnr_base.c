
/*
    IME module driver

    NT96520 IME module driver.

    @file       ime_3dnr_base.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/



#include "ime_reg.h"
#include "ime_int_public.h"

#include "ime_3dnr_base.h"

BOOL ime_tmnr_in_ref_va2pa_flag = TRUE;

static UINT32 ime_tmnr_in_ref_y_addr = 0x0;
static UINT32 ime_tmnr_in_ref_uv_addr = 0x0;
static UINT32 ime_tmnr_out_ref_y_addr = 0x0;
static UINT32 ime_tmnr_out_ref_uv_addr = 0x0;
static UINT32 ime_tmnr_in_ms_addr = 0x0;
static UINT32 ime_tmnr_out_ms_addr = 0x0;
static UINT32 ime_tmnr_out_roi_ms_addr = 0x0;
static UINT32 ime_tmnr_in_mv_addr = 0x0;
static UINT32 ime_tmnr_out_mv_addr = 0x0;
static UINT32 ime_tmnr_out_sta_addr = 0x0;

#if (IPP_DRIVER_NEW_REG_HANDLE == DISABLE)

VOID ime_tmnr_set_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_1.bit.ime_3dnr_en = set_en;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_3dnr_en;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_in_ref_decoder_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_tmnr_ref_in_dec_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_1.bit.ime_tmnr_ref_in_dec_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_out_ref_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_tmnr_ref_out_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_1.bit.ime_tmnr_ref_out_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_out_ref_encoder_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_tmnr_ref_out_enc_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_1.bit.ime_tmnr_ref_out_enc_en = set_en;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_ref_encoder_enable_reg(void)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_tmnr_ref_out_enc_en;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_in_ref_flip_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_3dnr_inref_flip_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);

	//imeg->reg_2.bit.ime_3dnr_inref_flip_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_out_ref_flip_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_3dnr_outref_flip_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);

	//imeg->reg_2.bit.ime_3dnr_outref_flip_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ms_roi_flip_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_3dnr_ms_roi_flip_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);

	//imeg->reg_2.bit.ime_3dnr_ms_roi_flip_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_luma_channel_enable_reg(UINT32 set_en)
{
	T_IME_TMNR_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_nr_y_ch_en = set_en;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_nr_y_ch_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_chroma_channel_enable_reg(UINT32 set_en)
{
	T_IME_TMNR_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_nr_c_ch_en = set_en;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_nr_c_ch_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_statistic_data_output_enable_reg(UINT32 set_en)
{
	T_IME_TMNR_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER1_OFS);

	arg.bit.ime_3dnr_statistic_output_en = set_en;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER1_OFS, arg.reg);

	//imeg->reg_385.bit.ime_3dnr_statistic_output_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ms_roi_output_enable_reg(UINT32 set_en)
{
	T_IME_TMNR_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_ps_smart_roi_ctrl = set_en;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_ps_smart_roi_ctrl = set_en;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_debug_reg(UINT32 set_mode, UINT32 set_mv0)
{
	T_IME_TMNR_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER1_OFS);

	arg.bit.ime_3dnr_dbg_mode = set_mode;
	arg.bit.ime_3dnr_dbg_mv0 = set_mv0;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_y_pre_blur_strength_reg(UINT32 set_val)
{
	T_IME_TMNR_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_pre_y_blur_str = set_val;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_pre_y_blur_str = set_val;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_nr_false_color_control_enable_reg(UINT32 set_en)
{
	T_IME_TMNR_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_nr_c_fsv_en = set_en;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_nr_c_fsv_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_false_color_control_strength_reg(UINT32 set_val)
{
	T_IME_TMNR_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_nr_c_fsv = set_val;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_nr_c_fsv = set_val;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_center_zero_enable_reg(UINT32 set_en)
{
	T_IME_TMNR_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_nr_center_wzero_y_3d = set_en;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_nr_center_wzero_y_3d = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_mv_check_enable_reg(UINT32 set_en)
{
	T_IME_TMNR_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_ps_mv_check_en = set_en;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_ps_mv_check_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_mv_roi_check_enable_reg(UINT32 set_en)
{
	T_IME_TMNR_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_ps_mv_check_roi_en = set_en;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_ps_mv_check_roi_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_control_reg(UINT32 set_mv_info_mode, UINT32 set_ps_mode)
{
	T_IME_TMNR_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_ps_mv_info_mode = set_mv_info_mode;
	arg.bit.ime_3dnr_ps_mode = set_ps_mode;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_ps_mv_info_mode = set_mv_info_mode;
	//imeg->reg_384.bit.ime_3dnr_ps_mode = set_ps_mode;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ne_sampling_horizontal_reg(UINT32 set_start_pos, UINT32 set_step, UINT32 set_num)
{
	T_IME_TMNR_CONTROL_REGISTER1 arg1;
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9 arg2;
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10 arg3;

	arg1.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER1_OFS);
	arg2.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9_OFS);
	arg3.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10_OFS);

	arg1.bit.ime_3dnr_ne_sample_step_x = set_start_pos;

	arg2.bit.ime_3dnr_ne_sample_num_x = set_step;

	arg3.bit.ime_3dnr_ne_sample_start_x = set_num;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER1_OFS, arg1.reg);
	IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9_OFS, arg2.reg);
	IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10_OFS, arg3.reg);


	//imeg->reg_385.bit.ime_3dnr_ne_sample_step_x = set_start_pos;
	//imeg->reg_403.bit.ime_3dnr_ne_sample_num_x = set_step;
	//imeg->reg_404.bit.ime_3dnr_ne_sample_start_x = set_num;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ne_sampling_vertical_reg(UINT32 set_start_pos, UINT32 set_step, UINT32 set_num)
{
	T_IME_TMNR_CONTROL_REGISTER1 arg1;
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9 arg2;
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10 arg3;

	arg1.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER1_OFS);
	arg2.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9_OFS);
	arg3.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10_OFS);

	arg1.bit.ime_3dnr_ne_sample_step_y = set_start_pos;

	arg2.bit.ime_3dnr_ne_sample_num_y = set_step;

	arg3.bit.ime_3dnr_ne_sample_start_y = set_num;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER1_OFS, arg1.reg);
	IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9_OFS, arg2.reg);
	IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10_OFS, arg3.reg);

	//imeg->reg_385.bit.ime_3dnr_ne_sample_step_y = set_start_pos;
	//imeg->reg_403.bit.ime_3dnr_ne_sample_num_y = set_step;
	//imeg->reg_404.bit.ime_3dnr_ne_sample_start_y = set_num;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_me_control_reg(UINT32 set_update_mode, UINT32 set_bndy, UINT32 set_ds_mode)
{
	T_IME_TMNR_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_me_update_mode = set_update_mode;
	arg.bit.ime_3dnr_me_boundary_set = set_bndy;
	arg.bit.ime_3dnr_me_mv_ds_mode = set_ds_mode;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_me_update_mode = set_update_mode;
	//imeg->reg_384.bit.ime_3dnr_me_boundary_set = set_bndy;
	//imeg->reg_384.bit.ime_3dnr_me_mv_ds_mode = set_ds_mode;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_me_sad_reg(UINT32 set_sad_type, UINT32 set_sad_shift)
{
	T_IME_TMNR_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_me_sad_type = set_sad_type;
	arg.bit.ime_3dnr_me_sad_shift = set_sad_shift;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_me_sad_type = set_sad_type;
	//imeg->reg_384.bit.ime_3dnr_me_sad_shift = set_sad_shift;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_me_sad_penalty_reg(UINT32 *p_sad_penalty)
{
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER0 arg1;
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER1 arg2;
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER2 arg3;

	arg1.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER0_OFS);
	arg2.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER1_OFS);
	arg3.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER2_OFS);

	arg1.bit.ime_3dnr_me_sad_penalty_0 = p_sad_penalty[0] & bit_mask[10];
	arg1.bit.ime_3dnr_me_sad_penalty_1 = p_sad_penalty[1] & bit_mask[10];
	arg1.bit.ime_3dnr_me_sad_penalty_2 = p_sad_penalty[2] & bit_mask[10];
	arg2.bit.ime_3dnr_me_sad_penalty_3 = p_sad_penalty[3] & bit_mask[10];
	arg2.bit.ime_3dnr_me_sad_penalty_4 = p_sad_penalty[4] & bit_mask[10];
	arg2.bit.ime_3dnr_me_sad_penalty_5 = p_sad_penalty[5] & bit_mask[10];
	arg3.bit.ime_3dnr_me_sad_penalty_6 = p_sad_penalty[6] & bit_mask[10];
	arg3.bit.ime_3dnr_me_sad_penalty_7 = p_sad_penalty[7] & bit_mask[10];

	IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER0_OFS, arg1.reg);
	IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER1_OFS, arg2.reg);
	IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER2_OFS, arg3.reg);
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_me_switch_threshold_reg(UINT32 *p_me_switch_th, UINT32 me_switch_ratio)
{
	UINT32 arg_val, i;
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER5 arg;

	for (i = 0; i < 8; i += 4) {
		arg_val = 0;
		arg_val = ((p_me_switch_th[i + 3] & bit_mask[8]) << 24) + ((p_me_switch_th[i + 2] & bit_mask[8]) << 16) + ((p_me_switch_th[i + 1] & bit_mask[8]) << 8) + (p_me_switch_th[i] & bit_mask[8]);

		IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER3_OFS + i, arg_val);
	}

#if 0
	imeg->reg_397.bit.ime_3dnr_me_switch_th0 = p_me_switch_th[0];
	imeg->reg_397.bit.ime_3dnr_me_switch_th1 = p_me_switch_th[1];
	imeg->reg_397.bit.ime_3dnr_me_switch_th2 = p_me_switch_th[2];
	imeg->reg_397.bit.ime_3dnr_me_switch_th3 = p_me_switch_th[3];
	imeg->reg_398.bit.ime_3dnr_me_switch_th4 = p_me_switch_th[4];
	imeg->reg_398.bit.ime_3dnr_me_switch_th5 = p_me_switch_th[5];
	imeg->reg_398.bit.ime_3dnr_me_switch_th6 = p_me_switch_th[6];
	imeg->reg_398.bit.ime_3dnr_me_switch_th7 = p_me_switch_th[7];
#endif

	arg.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER5_OFS);

	arg.bit.ime_3dnr_me_switch_ratio = me_switch_ratio;

	IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER5_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_me_detail_penalty_reg(UINT32 *p_me_detail_penalty)
{
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER6 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER6_OFS);

	arg.bit.ime_3dnr_me_detail_penalty0 = p_me_detail_penalty[0] & bit_mask[4];
	arg.bit.ime_3dnr_me_detail_penalty1 = p_me_detail_penalty[1] & bit_mask[4];
	arg.bit.ime_3dnr_me_detail_penalty2 = p_me_detail_penalty[2] & bit_mask[4];
	arg.bit.ime_3dnr_me_detail_penalty3 = p_me_detail_penalty[3] & bit_mask[4];
	arg.bit.ime_3dnr_me_detail_penalty4 = p_me_detail_penalty[4] & bit_mask[4];
	arg.bit.ime_3dnr_me_detail_penalty5 = p_me_detail_penalty[5] & bit_mask[4];
	arg.bit.ime_3dnr_me_detail_penalty6 = p_me_detail_penalty[6] & bit_mask[4];
	arg.bit.ime_3dnr_me_detail_penalty7 = p_me_detail_penalty[7] & bit_mask[4];

	IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER6_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_me_probability_reg(UINT32 *p_me_probability)
{
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7_OFS);

	arg.bit.ime_3dnr_me_probability0 = p_me_probability[0] & bit_mask[1];
	arg.bit.ime_3dnr_me_probability1 = p_me_probability[1] & bit_mask[1];
	arg.bit.ime_3dnr_me_probability2 = p_me_probability[2] & bit_mask[1];
	arg.bit.ime_3dnr_me_probability3 = p_me_probability[3] & bit_mask[1];
	arg.bit.ime_3dnr_me_probability4 = p_me_probability[4] & bit_mask[1];
	arg.bit.ime_3dnr_me_probability5 = p_me_probability[5] & bit_mask[1];
	arg.bit.ime_3dnr_me_probability6 = p_me_probability[6] & bit_mask[1];
	arg.bit.ime_3dnr_me_probability7 = p_me_probability[7] & bit_mask[1];

	IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7_OFS, arg.reg);

#if 0
	imeg->reg_401.bit.ime_3dnr_me_probability0 = p_me_probability[0];
	imeg->reg_401.bit.ime_3dnr_me_probability1 = p_me_probability[1];
	imeg->reg_401.bit.ime_3dnr_me_probability2 = p_me_probability[2];
	imeg->reg_401.bit.ime_3dnr_me_probability3 = p_me_probability[3];
	imeg->reg_401.bit.ime_3dnr_me_probability4 = p_me_probability[4];
	imeg->reg_401.bit.ime_3dnr_me_probability5 = p_me_probability[5];
	imeg->reg_401.bit.ime_3dnr_me_probability6 = p_me_probability[6];
	imeg->reg_401.bit.ime_3dnr_me_probability7 = p_me_probability[7];
#endif
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_me_rand_bit_reg(UINT32 me_rand_bitx, UINT32 me_rand_bity)
{
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7_OFS);

	arg.bit.ime_3dnr_me_rand_bit_x = me_rand_bitx;
	arg.bit.ime_3dnr_me_rand_bit_y = me_rand_bity;

	IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7_OFS, arg.reg);

	//imeg->reg_401.bit.ime_3dnr_me_rand_bit_x = me_rand_bitx;
	//imeg->reg_401.bit.ime_3dnr_me_rand_bit_y = me_rand_bity;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_me_threshold_reg(UINT32 me_min_detail, UINT32 me_periodic_th)
{
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7 arg1;
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER8 arg2;

	arg1.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7_OFS);
	arg2.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER8_OFS);

	arg1.bit.ime_3dnr_me_min_detail = me_min_detail;
	arg2.bit.ime_3dnr_me_periodic_th = me_periodic_th;

	IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7_OFS, arg1.reg);
	IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER8_OFS, arg2.reg);

	//imeg->reg_401.bit.ime_3dnr_me_min_detail = me_min_detail;
	//imeg->reg_402.bit.ime_3dnr_me_periodic_th = me_periodic_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_md_final_threshold_reg(UINT32 set_md_k1, UINT32 set_md_k2)
{
	T_IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER10 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER10_OFS);

	arg.bit.ime_3dnr_md_k1 = set_md_k1;
	arg.bit.ime_3dnr_md_k2 = set_md_k2;

	IME_SETREG(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER10_OFS, arg.reg);

	//imeg->reg_418.bit.ime_3dnr_md_k1 = set_md_k1;
	//imeg->reg_418.bit.ime_3dnr_md_k2 = set_md_k2;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_md_roi_final_threshold_reg(UINT32 set_md_roi_k1, UINT32 set_md_roi_k2)
{
	T_IME_3DNR_ROI_MOTION_DETECTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_ROI_MOTION_DETECTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_roi_md_k1 = set_md_roi_k1;
	arg.bit.ime_3dnr_roi_md_k2 = set_md_roi_k2;

	IME_SETREG(IME_3DNR_ROI_MOTION_DETECTION_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_444.bit.ime_3dnr_roi_md_k1 = set_md_roi_k1;
	//imeg->reg_444.bit.ime_3dnr_roi_md_k2 = set_md_roi_k2;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_md_sad_coefs_reg(UINT32 *p_set_coefa, UINT32 *p_set_coefb)
{
	UINT32 arg_val, i;

	for (i = 0; i < 8; i += 4) {
		arg_val = 0;
		arg_val = ((p_set_coefa[i + 3] & bit_mask[6]) << 24) + ((p_set_coefa[i + 2] & bit_mask[6]) << 16) + ((p_set_coefa[i + 1] & bit_mask[6]) << 8) + (p_set_coefa[i] & bit_mask[6]);

		IME_SETREG(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER0_OFS + i, arg_val);
	}

	for (i = 0; i < 8; i += 2) {
		arg_val = 0;
		arg_val = ((p_set_coefb[i + 1] & bit_mask[14]) << 16) + (p_set_coefb[i] & bit_mask[14]);

		IME_SETREG(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER2_OFS + (i << 1), arg_val);
	}

#if 0
	imeg->reg_408.bit.ime_3dnr_md_sad_coef_a0 = p_set_coefa[0];
	imeg->reg_408.bit.ime_3dnr_md_sad_coef_a1 = p_set_coefa[1];
	imeg->reg_408.bit.ime_3dnr_md_sad_coef_a2 = p_set_coefa[2];
	imeg->reg_408.bit.ime_3dnr_md_sad_coef_a3 = p_set_coefa[3];
	imeg->reg_409.bit.ime_3dnr_md_sad_coef_a4 = p_set_coefa[4];
	imeg->reg_409.bit.ime_3dnr_md_sad_coef_a5 = p_set_coefa[5];
	imeg->reg_409.bit.ime_3dnr_md_sad_coef_a6 = p_set_coefa[6];
	imeg->reg_409.bit.ime_3dnr_md_sad_coef_a7 = p_set_coefa[7];

	imeg->reg_410.bit.ime_3dnr_md_sad_coef_b0 = p_set_coefb[0];
	imeg->reg_410.bit.ime_3dnr_md_sad_coef_b1 = p_set_coefb[1];
	imeg->reg_411.bit.ime_3dnr_md_sad_coef_b2 = p_set_coefb[2];
	imeg->reg_411.bit.ime_3dnr_md_sad_coef_b3 = p_set_coefb[3];
	imeg->reg_412.bit.ime_3dnr_md_sad_coef_b4 = p_set_coefb[4];
	imeg->reg_412.bit.ime_3dnr_md_sad_coef_b5 = p_set_coefb[5];
	imeg->reg_413.bit.ime_3dnr_md_sad_coef_b6 = p_set_coefb[6];
	imeg->reg_413.bit.ime_3dnr_md_sad_coef_b7 = p_set_coefb[7];
#endif
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_md_sad_standard_deviation_reg(UINT32 *p_set_std)
{
	UINT32 arg_val, i;

	for (i = 0; i < 8; i += 2) {
		arg_val = 0;
		arg_val = ((p_set_std[i + 1] & bit_mask[14]) << 16) + (p_set_std[i] & bit_mask[14]);

		IME_SETREG(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER6_OFS + (i << 1), arg_val);
	}

#if 0
	imeg->reg_414.bit.ime_3dnr_md_sad_std0 = p_set_std[0];
	imeg->reg_414.bit.ime_3dnr_md_sad_std1 = p_set_std[1];
	imeg->reg_415.bit.ime_3dnr_md_sad_std2 = p_set_std[2];
	imeg->reg_415.bit.ime_3dnr_md_sad_std3 = p_set_std[3];
	imeg->reg_416.bit.ime_3dnr_md_sad_std4 = p_set_std[4];
	imeg->reg_416.bit.ime_3dnr_md_sad_std5 = p_set_std[5];
	imeg->reg_417.bit.ime_3dnr_md_sad_std6 = p_set_std[6];
	imeg->reg_417.bit.ime_3dnr_md_sad_std7 = p_set_std[7];
#endif
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_mc_noise_base_level_reg(UINT32 *p_set_base_level)
{
	UINT32 arg_val, i;

	for (i = 0; i < 8; i += 2) {
		arg_val = 0;
		arg_val = ((p_set_base_level[i + 1] & bit_mask[14]) << 16) + (p_set_base_level[i] & bit_mask[14]);

		IME_SETREG(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER0_OFS + (i << 1), arg_val);
	}

#if 0
	imeg->reg_424.bit.ime_3dnr_mc_sad_base0 = p_set_base_level[0];
	imeg->reg_424.bit.ime_3dnr_mc_sad_base1 = p_set_base_level[1];
	imeg->reg_425.bit.ime_3dnr_mc_sad_base2 = p_set_base_level[2];
	imeg->reg_425.bit.ime_3dnr_mc_sad_base3 = p_set_base_level[3];
	imeg->reg_426.bit.ime_3dnr_mc_sad_base4 = p_set_base_level[4];
	imeg->reg_426.bit.ime_3dnr_mc_sad_base5 = p_set_base_level[5];
	imeg->reg_427.bit.ime_3dnr_mc_sad_base6 = p_set_base_level[6];
	imeg->reg_427.bit.ime_3dnr_mc_sad_base7 = p_set_base_level[7];
#endif
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_mc_edge_coefs_offset_reg(UINT32 *p_set_coefa, UINT32 *p_set_coefb)
{
	UINT32 arg_val, i;

	for (i = 0; i < 8; i += 4) {
		arg_val = 0;
		arg_val = ((p_set_coefa[i + 3] & bit_mask[6]) << 24) + ((p_set_coefa[i + 2] & bit_mask[6]) << 16) + ((p_set_coefa[i + 1] & bit_mask[6]) << 8) + (p_set_coefa[i] & bit_mask[6]);

		IME_SETREG(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER4_OFS + i, arg_val);
	}

	for (i = 0; i < 8; i += 2) {
		arg_val = 0;
		arg_val = ((p_set_coefb[i + 1] & bit_mask[14]) << 16) + (p_set_coefb[i] & bit_mask[14]);

		IME_SETREG(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER6_OFS + (i << 1), arg_val);
	}

#if 0
	imeg->reg_428.bit.ime_3dnr_mc_sad_coef_a0 = p_set_coefa[0];
	imeg->reg_428.bit.ime_3dnr_mc_sad_coef_a1 = p_set_coefa[1];
	imeg->reg_428.bit.ime_3dnr_mc_sad_coef_a2 = p_set_coefa[2];
	imeg->reg_428.bit.ime_3dnr_mc_sad_coef_a3 = p_set_coefa[3];

	imeg->reg_429.bit.ime_3dnr_mc_sad_coef_a4 = p_set_coefa[4];
	imeg->reg_429.bit.ime_3dnr_mc_sad_coef_a5 = p_set_coefa[5];
	imeg->reg_429.bit.ime_3dnr_mc_sad_coef_a6 = p_set_coefa[6];
	imeg->reg_429.bit.ime_3dnr_mc_sad_coef_a7 = p_set_coefa[7];

	imeg->reg_430.bit.ime_3dnr_mc_sad_coef_b0 = p_set_coefb[0];
	imeg->reg_430.bit.ime_3dnr_mc_sad_coef_b1 = p_set_coefb[1];
	imeg->reg_431.bit.ime_3dnr_mc_sad_coef_b2 = p_set_coefb[2];
	imeg->reg_431.bit.ime_3dnr_mc_sad_coef_b3 = p_set_coefb[3];
	imeg->reg_432.bit.ime_3dnr_mc_sad_coef_b4 = p_set_coefb[4];
	imeg->reg_432.bit.ime_3dnr_mc_sad_coef_b5 = p_set_coefb[5];
	imeg->reg_433.bit.ime_3dnr_mc_sad_coef_b6 = p_set_coefb[6];
	imeg->reg_433.bit.ime_3dnr_mc_sad_coef_b7 = p_set_coefb[7];
#endif
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_mc_sad_standard_deviation_reg(UINT32 *p_set_std)
{
	UINT32 arg_val, i;

	for (i = 0; i < 8; i += 2) {
		arg_val = 0;
		arg_val = ((p_set_std[i + 1] & bit_mask[14]) << 16) + (p_set_std[i] & bit_mask[14]);

		IME_SETREG(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER10_OFS + (i << 1), arg_val);
	}

#if 0
	imeg->reg_434.bit.ime_3dnr_mc_sad_std0 = p_set_std[0];
	imeg->reg_434.bit.ime_3dnr_mc_sad_std1 = p_set_std[1];
	imeg->reg_435.bit.ime_3dnr_mc_sad_std2 = p_set_std[2];
	imeg->reg_435.bit.ime_3dnr_mc_sad_std3 = p_set_std[3];
	imeg->reg_436.bit.ime_3dnr_mc_sad_std4 = p_set_std[4];
	imeg->reg_436.bit.ime_3dnr_mc_sad_std5 = p_set_std[5];
	imeg->reg_437.bit.ime_3dnr_mc_sad_std6 = p_set_std[6];
	imeg->reg_437.bit.ime_3dnr_mc_sad_std7 = p_set_std[7];
#endif
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_mc_final_threshold_reg(UINT32 set_mc_k1, UINT32 set_mc_k2)
{
	T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER14 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER14_OFS);

	arg.bit.ime_3dnr_mc_k1 = set_mc_k1;
	arg.bit.ime_3dnr_mc_k2 = set_mc_k2;

	IME_SETREG(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER14_OFS, arg.reg);

	//imeg->reg_438.bit.ime_3dnr_mc_k1 = set_mc_k1;
	//imeg->reg_438.bit.ime_3dnr_mc_k2 = set_mc_k2;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_mc_roi_final_threshold_reg(UINT32 set_mc_roi_k1, UINT32 set_mc_roi_k2)
{
	T_IME_3DNR_ROI_MOTION_COMPENSATION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_ROI_MOTION_COMPENSATION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_roi_mc_k1 = set_mc_roi_k1;
	arg.bit.ime_3dnr_roi_mc_k2 = set_mc_roi_k2;

	IME_SETREG(IME_3DNR_ROI_MOTION_COMPENSATION_CONTROL_REGISTER0_OFS, arg.reg);


	//imeg->reg_452.bit.ime_3dnr_roi_mc_k1 = set_mc_roi_k1;
	//imeg->reg_452.bit.ime_3dnr_roi_mc_k2 = set_mc_roi_k2;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_mv_threshold_reg(UINT32 set_mv_th, UINT32 set_roi_mv_th)
{
	T_IME_3DNR_PS_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_ps_mv_th = set_mv_th;
	arg.bit.ime_3dnr_ps_roi_mv_th = set_roi_mv_th;

	IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_460.bit.ime_3dnr_ps_mv_th = set_mv_th;
	//imeg->reg_460.bit.ime_3dnr_ps_roi_mv_th = set_roi_mv_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_mix_ratio_reg(UINT32 set_mix_rto0, UINT32 set_mix_rto1)
{
	T_IME_3DNR_PS_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER1_OFS);

	arg.bit.ime_3dnr_ps_mix_ratio0 = set_mix_rto0;
	arg.bit.ime_3dnr_ps_mix_ratio1 = set_mix_rto1;

	IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER1_OFS, arg.reg);

	//imeg->reg_461.bit.ime_3dnr_ps_mix_ratio0 = set_mix_rto0;
	//imeg->reg_461.bit.ime_3dnr_ps_mix_ratio1 = set_mix_rto1;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_mix_threshold_reg(UINT32 set_mix_th0, UINT32 set_mix_th1)
{
	T_IME_3DNR_PS_CONTROL_REGISTER2 arg;

	arg.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER2_OFS);

	arg.bit.ime_3dnr_ps_mix_th0 = set_mix_th0;
	arg.bit.ime_3dnr_ps_mix_th1 = set_mix_th1;

	IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER2_OFS, arg.reg);


	//imeg->reg_462.bit.ime_3dnr_ps_mix_th0 = set_mix_th0;
	//imeg->reg_462.bit.ime_3dnr_ps_mix_th1 = set_mix_th1;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_mix_slope_reg(UINT32 set_mix_slp0, UINT32 set_mix_slp1)
{
	T_IME_3DNR_PS_CONTROL_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER3_OFS);

	arg.bit.ime_3dnr_ps_mix_slope0 = set_mix_slp0;
	arg.bit.ime_3dnr_ps_mix_slope1 = set_mix_slp1;

	IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER3_OFS, arg.reg);


	//imeg->reg_463.bit.ime_3dnr_ps_mix_slope0 = set_mix_slp0;
	//imeg->reg_463.bit.ime_3dnr_ps_mix_slope1 = set_mix_slp1;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_down_sample_reg(UINT32 set_ds_th)
{
	T_IME_3DNR_PS_CONTROL_REGISTER4 arg;

	arg.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER4_OFS);

	arg.bit.ime_3dnr_ps_ds_th = set_ds_th;

	IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER4_OFS, arg.reg);

	//imeg->reg_472.bit.ime_3dnr_ps_ds_th = set_ds_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_roi_down_sample_reg(UINT32 set_ds_roi_th)
{
	T_IME_3DNR_PS_CONTROL_REGISTER4 arg;

	arg.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER4_OFS);

	arg.bit.ime_3dnr_ps_ds_th_roi = set_ds_roi_th;

	IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER4_OFS, arg.reg);

	//imeg->reg_472.bit.ime_3dnr_ps_ds_th_roi = set_ds_roi_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_edge_control_reg(UINT32 set_start_point, UINT32 set_th0, UINT32 set_th1, UINT32 set_slope)
{
	T_IME_3DNR_PS_CONTROL_REGISTER4 arg1;
	T_IME_3DNR_PS_CONTROL_REGISTER5 arg2;
	T_IME_3DNR_PS_CONTROL_REGISTER6 arg3;

	arg1.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER4_OFS);
	arg2.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER5_OFS);
	arg3.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER6_OFS);

	arg1.bit.ime_3dnr_ps_edge_w = set_start_point;

	arg2.bit.ime_3dnr_ps_edge_th0 = set_th0;
	arg2.bit.ime_3dnr_ps_edge_th1 = set_th1;

	arg3.bit.ime_3dnr_ps_edge_slope = set_slope;

	IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER4_OFS, arg1.reg);
	IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER5_OFS, arg2.reg);
	IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER6_OFS, arg3.reg);

	//imeg->reg_472.bit.ime_3dnr_ps_edge_w = set_start_point;

	//imeg->reg_473.bit.ime_3dnr_ps_edge_th0 = set_th0;
	//imeg->reg_473.bit.ime_3dnr_ps_edge_th1 = set_th1;

	//imeg->reg_474.bit.ime_3dnr_ps_edge_slope = set_slope;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_path_error_threshold_reg(UINT32 set_th)
{
	T_IME_3DNR_PS_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_ps_fs_th = set_th;

	IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_460.bit.ime_3dnr_ps_fs_th = set_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_luma_residue_threshold_reg(UINT32 set_th)
{
	T_IME_3DNR_NR_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_nr_residue_th_y = set_th;

	IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_475.bit.ime_3dnr_nr_residue_th_y = set_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_chroma_residue_threshold_reg(UINT32 set_th)
{
	T_IME_3DNR_NR_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_nr_residue_th_c = set_th;

	IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_475.bit.ime_3dnr_nr_residue_th_c = set_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_varied_frequency_filter_weight_reg(UINT32 *p_set_wets)
{
	UINT32 arg_val, i;

	i = 0;
	arg_val = ((p_set_wets[i + 3] & bit_mask[8]) << 24) + ((p_set_wets[i + 2] & bit_mask[8]) << 16) + ((p_set_wets[i + 1] & bit_mask[8]) << 8) + (p_set_wets[i] & bit_mask[8]);

	IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER1_OFS, arg_val);

	//imeg->reg_476.bit.ime_3dnr_nr_freq_w0 = p_set_wets[0];
	//imeg->reg_476.bit.ime_3dnr_nr_freq_w1 = p_set_wets[1];
	//imeg->reg_476.bit.ime_3dnr_nr_freq_w2 = p_set_wets[2];
	//imeg->reg_476.bit.ime_3dnr_nr_freq_w3 = p_set_wets[3];
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_luma_filter_weight_reg(UINT32 *p_set_wets)
{
	UINT32 arg_val, i;

	for (i = 0; i < 8; i += 4) {
		arg_val = 0;
		arg_val = ((p_set_wets[i + 3] & bit_mask[8]) << 24) + ((p_set_wets[i + 2] & bit_mask[8]) << 16) + ((p_set_wets[i + 1] & bit_mask[8]) << 8) + (p_set_wets[i] & bit_mask[8]);

		IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER3_OFS + i, arg_val);
	}

	//imeg->reg_484.bit.ime_3dnr_nr_luma_w0 = p_set_wets[0];
	//imeg->reg_484.bit.ime_3dnr_nr_luma_w1 = p_set_wets[1];
	//imeg->reg_484.bit.ime_3dnr_nr_luma_w2 = p_set_wets[2];
	//imeg->reg_484.bit.ime_3dnr_nr_luma_w3 = p_set_wets[3];
	//imeg->reg_485.bit.ime_3dnr_nr_luma_w4 = p_set_wets[4];
	//imeg->reg_485.bit.ime_3dnr_nr_luma_w5 = p_set_wets[5];
	//imeg->reg_485.bit.ime_3dnr_nr_luma_w6 = p_set_wets[6];
	//imeg->reg_485.bit.ime_3dnr_nr_luma_w7 = p_set_wets[7];
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_prefiltering_reg(UINT32 *p_set_strs, UINT32 *p_set_rto)
{
	UINT32 arg_val, i;
	T_IME_3DNR_NR_CONTROL_REGISTER8 arg;

	i = 0;
	arg_val = (p_set_strs[i + 3] << 24) + (p_set_strs[i + 2] << 16) + (p_set_strs[i + 1] << 8) + (p_set_strs[i]);
	IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER6_OFS, arg_val);

	//imeg->reg_492.bit.ime_3dnr_nr_pre_filtering_str0 = p_set_strs[0];
	//imeg->reg_492.bit.ime_3dnr_nr_pre_filtering_str1 = p_set_strs[1];
	//imeg->reg_492.bit.ime_3dnr_nr_pre_filtering_str2 = p_set_strs[2];
	//imeg->reg_492.bit.ime_3dnr_nr_pre_filtering_str3 = p_set_strs[3];

	arg.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER8_OFS);
	arg.bit.ime_3dnr_nr_pre_filtering_ratio0 = p_set_rto[0];
	arg.bit.ime_3dnr_nr_pre_filtering_ratio1 = p_set_rto[1];

	IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER8_OFS, arg.reg);

	//imeg->reg_496.bit.ime_3dnr_nr_pre_filtering_ratio0 = p_set_rto[0];
	//imeg->reg_496.bit.ime_3dnr_nr_pre_filtering_ratio1 = p_set_rto[1];
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_snr_control_reg(UINT32 *p_set_strs, UINT32 set_th)
{
	T_IME_3DNR_NR_CONTROL_REGISTER8 arg1;
	T_IME_3DNR_NR_CONTROL_REGISTER9 arg2;
	T_IME_3DNR_NR_CONTROL_REGISTER10 arg3;

	arg1.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER8_OFS);
	arg2.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER9_OFS);
	arg3.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER10_OFS);

	arg1.bit.ime_3dnr_nr_snr_str0 = p_set_strs[0];
	arg1.bit.ime_3dnr_nr_snr_str1 = p_set_strs[1];
	arg2.bit.ime_3dnr_nr_snr_str2 = p_set_strs[2];

	arg3.bit.ime_3dnr_nr_base_th_snr = set_th;

	IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER8_OFS, arg1.reg);
	IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER9_OFS, arg2.reg);
	IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER10_OFS, arg3.reg);

	//imeg->reg_496.bit.ime_3dnr_nr_snr_str0 = p_set_strs[0];
	//imeg->reg_496.bit.ime_3dnr_nr_snr_str1 = p_set_strs[1];
	//imeg->reg_497.bit.ime_3dnr_nr_snr_str2 = p_set_strs[2];

	//imeg->reg_498.bit.ime_3dnr_nr_base_th_snr = set_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_tnr_control_reg(UINT32 *p_set_strs, UINT32 set_th)
{
	T_IME_3DNR_NR_CONTROL_REGISTER9 arg1;
	T_IME_3DNR_NR_CONTROL_REGISTER10 arg2;

	arg1.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER9_OFS);
	arg2.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER10_OFS);

	arg1.bit.ime_3dnr_nr_tnr_str0 = p_set_strs[0];
	arg1.bit.ime_3dnr_nr_tnr_str1 = p_set_strs[1];
	arg1.bit.ime_3dnr_nr_tnr_str2 = p_set_strs[2];

	arg2.bit.ime_3dnr_nr_base_th_tnr = set_th;

	IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER9_OFS, arg1.reg);
	IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER10_OFS, arg2.reg);

	//imeg->reg_497.bit.ime_3dnr_nr_tnr_str0 = p_set_strs[0];
	//imeg->reg_497.bit.ime_3dnr_nr_tnr_str1 = p_set_strs[1];
	//imeg->reg_497.bit.ime_3dnr_nr_tnr_str2 = p_set_strs[2];

	//imeg->reg_498.bit.ime_3dnr_nr_base_th_tnr = set_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_luma_noise_reduction_reg(UINT32 *p_set_th, UINT32 *p_set_lut)
{
	UINT32 arg_val, i;

	i = 0;
	arg_val = ((p_set_th[i + 3] & bit_mask[6]) << 24) + ((p_set_th[i + 2] & bit_mask[6]) << 16) + ((p_set_th[i + 1] & bit_mask[6]) << 8) + (p_set_th[i] & bit_mask[6]);

	IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER11_OFS + i, arg_val);

	//imeg->reg_499.bit.ime_3dnr_nr_y_3d_th0 = p_set_th[0];
	//imeg->reg_499.bit.ime_3dnr_nr_y_3d_th1 = p_set_th[1];
	//imeg->reg_499.bit.ime_3dnr_nr_y_3d_th2 = p_set_th[2];
	//imeg->reg_499.bit.ime_3dnr_nr_y_3d_th3 = p_set_th[3];

	for (i = 0; i < 8; i += 4) {
		arg_val = 0;
		arg_val = ((p_set_lut[i + 3] & bit_mask[7]) << 24) + ((p_set_lut[i + 2] & bit_mask[7]) << 16) + ((p_set_lut[i + 1] & bit_mask[7]) << 8) + (p_set_lut[i] & bit_mask[7]);

		IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER12_OFS + i, arg_val);
	}

#if 0
	imeg->reg_500.bit.ime_3dnr_nr_y_3d_lut0 = p_set_lut[0];
	imeg->reg_500.bit.ime_3dnr_nr_y_3d_lut1 = p_set_lut[1];
	imeg->reg_500.bit.ime_3dnr_nr_y_3d_lut2 = p_set_lut[2];
	imeg->reg_500.bit.ime_3dnr_nr_y_3d_lut3 = p_set_lut[3];
	imeg->reg_501.bit.ime_3dnr_nr_y_3d_lut4 = p_set_lut[4];
	imeg->reg_501.bit.ime_3dnr_nr_y_3d_lut5 = p_set_lut[5];
	imeg->reg_501.bit.ime_3dnr_nr_y_3d_lut6 = p_set_lut[6];
	imeg->reg_501.bit.ime_3dnr_nr_y_3d_lut7 = p_set_lut[7];
#endif
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_chroma_noise_reduction_reg(UINT32 *p_set_lut, UINT32 *p_set_rto)
{
	UINT32 arg_val, i;
	T_IME_3DNR_NR_CONTROL_REGISTER16 arg;

	for (i = 0; i < 8; i += 4) {
		arg_val = 0;
		arg_val = ((p_set_lut[i + 3] & bit_mask[7]) << 24) + ((p_set_lut[i + 2] & bit_mask[7]) << 16) + ((p_set_lut[i + 1] & bit_mask[7]) << 8) + (p_set_lut[i] & bit_mask[7]);

		IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER14_OFS + i, arg_val);
	}

	//imeg->reg_502.bit.ime_3dnr_nr_c_3d_lut0 = p_set_lut[0];
	//imeg->reg_502.bit.ime_3dnr_nr_c_3d_lut1 = p_set_lut[1];
	//imeg->reg_502.bit.ime_3dnr_nr_c_3d_lut2 = p_set_lut[2];
	//imeg->reg_502.bit.ime_3dnr_nr_c_3d_lut3 = p_set_lut[3];
	//imeg->reg_503.bit.ime_3dnr_nr_c_3d_lut4 = p_set_lut[4];
	//imeg->reg_503.bit.ime_3dnr_nr_c_3d_lut5 = p_set_lut[5];
	//imeg->reg_503.bit.ime_3dnr_nr_c_3d_lut6 = p_set_lut[6];
	//imeg->reg_503.bit.ime_3dnr_nr_c_3d_lut7 = p_set_lut[7];

	arg.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER16_OFS);

	arg.bit.ime_3dnr_nr_c_3d_ratio0 = p_set_rto[0];
	arg.bit.ime_3dnr_nr_c_3d_ratio1 = p_set_rto[1];

	IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER16_OFS, arg.reg);

	//imeg->reg_504.bit.ime_3dnr_nr_c_3d_ratio0 = p_set_rto[0];
	//imeg->reg_504.bit.ime_3dnr_nr_c_3d_ratio1 = p_set_rto[1];
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_statistic_data_output_start_position_reg(UINT32 set_start_x, UINT32 set_start_y)
{
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10_OFS);

	arg.bit.ime_3dnr_ne_sample_start_x = set_start_x;
	arg.bit.ime_3dnr_ne_sample_start_y = set_start_y;

	IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10_OFS, arg.reg);

	//imeg->reg_404.bit.ime_3dnr_ne_sample_start_x = set_start_x;
	//imeg->reg_404.bit.ime_3dnr_ne_sample_start_y = set_start_y;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_statistic_data_output_sample_numbers_reg(UINT32 set_num_x, UINT32 set_num_y)
{
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9_OFS);

	arg.bit.ime_3dnr_ne_sample_num_x = set_num_x;
	arg.bit.ime_3dnr_ne_sample_num_y = set_num_y;

	IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9_OFS, arg.reg);

	//imeg->reg_403.bit.ime_3dnr_ne_sample_num_x = set_num_x;
	//imeg->reg_403.bit.ime_3dnr_ne_sample_num_y = set_num_y;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_statistic_data_output_sample_steps_reg(UINT32 set_step_x, UINT32 set_step_y)
{
	T_IME_TMNR_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER1_OFS);

	arg.bit.ime_3dnr_ne_sample_step_x = set_step_x;
	arg.bit.ime_3dnr_ne_sample_step_y = set_step_y;

	IME_SETREG(IME_TMNR_CONTROL_REGISTER1_OFS, arg.reg);

	//imeg->reg_385.bit.ime_3dnr_ne_sample_step_x = set_step_x;
	//imeg->reg_385.bit.ime_3dnr_ne_sample_step_y = set_step_y;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_in_ref_y_lineoffset_reg(UINT32 set_ofs_y)
{
	T_IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0_OFS);

	arg.bit.ime_3dnr_ref_dram_y_ofsi = set_ofs_y >> 2;

	IME_SETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0_OFS, arg.reg);

	//imeg->reg_512.bit.ime_3dnr_ref_dram_y_ofsi = set_ofs_y >> 2;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_in_ref_y_lineoffset_reg(VOID)
{
	T_IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0_OFS);

	return (arg.bit.ime_3dnr_ref_dram_y_ofsi << 2);
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_in_ref_uv_lineoffset_reg(UINT32 set_ofs_uv)
{
	T_IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1_OFS);

	arg.bit.ime_3dnr_ref_dram_uv_ofsi = set_ofs_uv >> 2;

	IME_SETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1_OFS, arg.reg);

	//imeg->reg_513.bit.ime_3dnr_ref_dram_uv_ofsi = set_ofs_uv >> 2;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_in_ref_uv_lineoffset_reg(VOID)
{
	T_IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1_OFS);

	return (arg.bit.ime_3dnr_ref_dram_uv_ofsi << 2);
}
//-------------------------------------------------------------------------------



VOID ime_tmnr_set_in_ref_y_addr_reg(UINT32 set_addr_y)
{
	T_IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0_OFS);

	ime_tmnr_in_ref_y_addr = set_addr_y;

    if (ime_tmnr_in_ref_va2pa_flag == TRUE) {
	    arg.bit.ime_3dnr_ref_dram_y_sai = ime_platform_va2pa(ime_tmnr_in_ref_y_addr) >> 2;
	} else {
        arg.bit.ime_3dnr_ref_dram_y_sai = (ime_tmnr_in_ref_y_addr) >> 2;
	}

	IME_SETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0_OFS, arg.reg);

	//imeg->reg_514.bit.ime_3dnr_ref_dram_y_sai = ime_platform_va2pa(ime_tmnr_in_ref_y_addr) >> 2;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_in_ref_y_addr_reg(VOID)
{
	return ime_tmnr_in_ref_y_addr;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_in_ref_uv_addr_reg(UINT32 set_addr_uv)
{
	T_IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1_OFS);

	ime_tmnr_in_ref_uv_addr = set_addr_uv;

    if (ime_tmnr_in_ref_va2pa_flag == TRUE) {
	    arg.bit.ime_3dnr_ref_dram_uv_sai = ime_platform_va2pa(ime_tmnr_in_ref_uv_addr) >> 2;
	} else {
        arg.bit.ime_3dnr_ref_dram_uv_sai = (ime_tmnr_in_ref_uv_addr) >> 2;
	}

	IME_SETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1_OFS, arg.reg);

	//imeg->reg_515.bit.ime_3dnr_ref_dram_uv_sai = ime_platform_va2pa(ime_tmnr_in_ref_uv_addr) >> 2;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_in_ref_uv_addr_reg(VOID)
{
	return ime_tmnr_in_ref_uv_addr;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_out_ref_y_lineoffset_reg(UINT32 set_ofs_y)
{
	T_IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0_OFS);

	arg.bit.ime_3dnr_ref_dram_y_ofso = set_ofs_y >> 2;

	IME_SETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0_OFS, arg.reg);

	//imeg->reg_516.bit.ime_3dnr_ref_dram_y_ofso = set_ofs_y >> 2;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_ref_y_lineoffset_reg(VOID)
{
	T_IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0_OFS);

	return (arg.bit.ime_3dnr_ref_dram_y_ofso << 2);
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_out_ref_uv_lineoffset_reg(UINT32 set_ofs_uv)
{
	T_IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1_OFS);

	arg.bit.ime_3dnr_ref_dram_uv_ofso = set_ofs_uv >> 2;

	IME_SETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1_OFS, arg.reg);

	//imeg->reg_517.bit.ime_3dnr_ref_dram_uv_ofso = set_ofs_uv >> 2;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_ref_uv_lineoffset_reg(VOID)
{
	T_IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1_OFS);

	return (arg.bit.ime_3dnr_ref_dram_uv_ofso << 2);
}
//-------------------------------------------------------------------------------



VOID ime_tmnr_set_out_ref_y_addr_reg(UINT32 set_addr_y)
{
	T_IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0_OFS);

	ime_tmnr_out_ref_y_addr = set_addr_y;

	arg.bit.ime_3dnr_ref_dram_y_sao = ime_platform_va2pa(ime_tmnr_out_ref_y_addr) >> 2;

	IME_SETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0_OFS, arg.reg);

	//imeg->reg_518.bit.ime_3dnr_ref_dram_y_sao = ime_platform_va2pa(ime_tmnr_out_ref_y_addr) >> 2;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_ref_y_addr_reg(VOID)
{
	return ime_tmnr_out_ref_y_addr;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_out_ref_uv_addr_reg(UINT32 set_addr_uv)
{
	T_IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1_OFS);

	ime_tmnr_out_ref_uv_addr = set_addr_uv;

	arg.bit.ime_3dnr_ref_dram_uv_sao = ime_platform_va2pa(ime_tmnr_out_ref_uv_addr) >> 2;

	IME_SETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1_OFS, arg.reg);

	//imeg->reg_519.bit.ime_3dnr_ref_dram_uv_sao = ime_platform_va2pa(ime_tmnr_out_ref_uv_addr) >> 2;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_ref_uv_addr_reg(VOID)
{
	return ime_tmnr_out_ref_uv_addr;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_in_motion_status_lineoffset_reg(UINT32 set_ofs)
{
	T_IME_3DNR_MOTION_STATUS_INPUT_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_STATUS_INPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	arg.bit.ime_3dnr_mot_dram_ofsi = (set_ofs >> 2);

	IME_SETREG(IME_3DNR_MOTION_STATUS_INPUT_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);

	//imeg->reg_520.bit.ime_3dnr_mot_dram_ofsi = (set_ofs >> 2);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_in_motion_status_lineoffset_reg(VOID)
{
	T_IME_3DNR_MOTION_STATUS_INPUT_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_STATUS_INPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	return (arg.bit.ime_3dnr_mot_dram_ofsi << 2);
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_in_motion_status_address_reg(UINT32 set_addr)
{
	T_IME_3DNR_MOTION_STATUS_INPUT_DMA_STARTING_ADDRESS_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_STATUS_INPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS);

	ime_tmnr_in_ms_addr = set_addr;

	arg.bit.ime_3dnr_mot_dram_sai = ime_platform_va2pa(ime_tmnr_in_ms_addr) >> 2;

	IME_SETREG(IME_3DNR_MOTION_STATUS_INPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS, arg.reg);

	//imeg->reg_521.bit.ime_3dnr_mot_dram_sai = ime_platform_va2pa(ime_tmnr_in_ms_addr) >> 2;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_in_motion_status_address_reg(VOID)
{
	return ime_tmnr_in_ms_addr;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_out_motion_status_address_reg(UINT32 set_addr)
{
	T_IME_3DNR_MOTION_STATUS_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_STATUS_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS);
	ime_tmnr_out_ms_addr = set_addr;

	arg.bit.ime_3dnr_mot_dram_sao = ime_platform_va2pa(ime_tmnr_out_ms_addr) >> 2;

	IME_SETREG(IME_3DNR_MOTION_STATUS_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS, arg.reg);

	//imeg->reg_523.bit.ime_3dnr_mot_dram_sao = ime_platform_va2pa(ime_tmnr_out_ms_addr) >> 2;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_motion_status_address_reg(VOID)
{
	return ime_tmnr_out_ms_addr;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_out_roi_motion_status_lineoffset_reg(UINT32 set_ofs)
{
	T_IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	arg.bit.ime_3dnr_mot_roi_dram_ofso = (set_ofs >> 2);

	IME_SETREG(IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);

	//imeg->reg_524.bit.ime_3dnr_mot_roi_dram_ofso = (set_ofs >> 2);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_roi_motion_status_lineoffset_reg(VOID)
{
	T_IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	return (arg.bit.ime_3dnr_mot_roi_dram_ofso << 2);
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_out_roi_motion_status_address_reg(UINT32 set_addr)
{
	T_IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS);
	ime_tmnr_out_roi_ms_addr = set_addr;

	arg.bit.ime_3dnr_mot_roi_dram_sao = ime_platform_va2pa(ime_tmnr_out_roi_ms_addr) >> 2;

	IME_SETREG(IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS, arg.reg);

	//imeg->reg_525.bit.ime_3dnr_mot_roi_dram_sao = ime_platform_va2pa(ime_tmnr_out_roi_ms_addr) >> 2;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_roi_motion_status_address_reg(VOID)
{
	return ime_tmnr_out_roi_ms_addr;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_in_motion_vector_lineoffset_reg(UINT32 set_ofs)
{
	T_IME_3DNR_MOTION_VECTOR_INPUT_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_VECTOR_INPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	arg.bit.ime_3dnr_mv_dram_ofsi = (set_ofs >> 2);

	IME_SETREG(IME_3DNR_MOTION_VECTOR_INPUT_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);

	//imeg->reg_526.bit.ime_3dnr_mv_dram_ofsi = (set_ofs >> 2);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_in_motion_vector_lineoffset_reg(VOID)
{
	T_IME_3DNR_MOTION_VECTOR_INPUT_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_VECTOR_INPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	return (arg.bit.ime_3dnr_mv_dram_ofsi << 2);
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_in_motion_vector_address_reg(UINT32 set_addr)
{
	T_IME_3DNR_MOTION_VECTOR_INPUT_DMA_STARTING_ADDRESS_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_VECTOR_INPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS);
	ime_tmnr_in_mv_addr = set_addr;

	arg.bit.ime_3dnr_mv_dram_sai = ime_platform_va2pa(ime_tmnr_in_mv_addr) >> 2;

	IME_SETREG(IME_3DNR_MOTION_VECTOR_INPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS, arg.reg);

	//imeg->reg_527.bit.ime_3dnr_mv_dram_sai = ime_platform_va2pa(ime_tmnr_in_mv_addr) >> 2;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_in_motion_vector_address_reg(VOID)
{
	return ime_tmnr_in_mv_addr;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_out_motion_vector_address_reg(UINT32 set_addr)
{
	T_IME_3DNR_MOTION_VECTOR_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_MOTION_VECTOR_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS);
	ime_tmnr_out_mv_addr = set_addr;

	arg.bit.ime_3dnr_mv_dram_sao = ime_platform_va2pa(ime_tmnr_out_mv_addr) >> 2;

	IME_SETREG(IME_3DNR_MOTION_VECTOR_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS, arg.reg);

	//imeg->reg_529.bit.ime_3dnr_mv_dram_sao = ime_platform_va2pa(ime_tmnr_out_mv_addr) >> 2;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_motion_vector_address_reg(VOID)
{
	return ime_tmnr_out_mv_addr;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_out_statistic_lineoffset_reg(UINT32 set_ofs)
{
	T_IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	arg.bit.ime_3dnr_statistic_dram_ofso = (set_ofs >> 2);

	IME_SETREG(IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);

	//imeg->reg_530.bit.ime_3dnr_statistic_dram_ofso = (set_ofs >> 2);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_statistic_lineoffset_reg(VOID)
{
	T_IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	return (arg.bit.ime_3dnr_statistic_dram_ofso << 2);
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_out_statistic_address_reg(UINT32 set_addr)
{
	T_IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS);
	ime_tmnr_out_sta_addr = set_addr;

	arg.bit.ime_3dnr_statistic_dram_sao = ime_platform_va2pa(ime_tmnr_out_sta_addr) >> 2;

	IME_SETREG(IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS, arg.reg);

	//imeg->reg_531.bit.ime_3dnr_statistic_dram_sao = ime_platform_va2pa(ime_tmnr_out_sta_addr) >> 2;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_statistic_address_reg(VOID)
{
	return ime_tmnr_out_sta_addr;
}
//-------------------------------------------------------------------------------


// read only
UINT32 ime_tmnr_get_sum_of_sad_value_reg(VOID)
{
	T_IME_3DNR_READ_ONLY_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_3DNR_READ_ONLY_REGISTER0_OFS);

	return arg.bit.ime_3dnr_ro_sad_sum;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_sum_of_mv_length_reg(VOID)
{
	T_IME_3DNR_READ_ONLY_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_3DNR_READ_ONLY_REGISTER1_OFS);

	return arg.bit.ime_3dnr_ro_mv_sum;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_total_sampling_number_reg(VOID)
{
	T_IME_3DNR_READ_ONLY_REGISTER2 arg;

	arg.reg = IME_GETREG(IME_3DNR_READ_ONLY_REGISTER2_OFS);

	return arg.bit.ime_3dnr_ro_sample_cnt;
}
//-------------------------------------------------------------------------------
#else




VOID ime_tmnr_set_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_3dnr_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_1.bit.ime_3dnr_en = set_en;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return imeg->reg_1.bit.ime_3dnr_en;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_in_ref_decoder_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_tmnr_ref_in_dec_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_1.bit.ime_tmnr_ref_in_dec_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_out_ref_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_tmnr_ref_out_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_1.bit.ime_tmnr_ref_out_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_out_ref_encoder_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_tmnr_ref_out_enc_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_1.bit.ime_tmnr_ref_out_enc_en = set_en;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_ref_encoder_enable_reg(void)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return imeg->reg_1.bit.ime_tmnr_ref_out_enc_en;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_in_ref_flip_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_3dnr_inref_flip_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);

	//imeg->reg_2.bit.ime_3dnr_inref_flip_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_out_ref_flip_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_3dnr_outref_flip_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);

	//imeg->reg_2.bit.ime_3dnr_outref_flip_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ms_roi_flip_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_3dnr_ms_roi_flip_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);

	//imeg->reg_2.bit.ime_3dnr_ms_roi_flip_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_luma_channel_enable_reg(UINT32 set_en)
{
	//T_IME_TMNR_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	imeg->reg_384.bit.ime_3dnr_nr_y_ch_en = set_en;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_nr_y_ch_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_chroma_channel_enable_reg(UINT32 set_en)
{
	//T_IME_TMNR_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	imeg->reg_384.bit.ime_3dnr_nr_c_ch_en = set_en;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_nr_c_ch_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_statistic_data_output_enable_reg(UINT32 set_en)
{
	//T_IME_TMNR_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER1_OFS);

	imeg->reg_385.bit.ime_3dnr_statistic_output_en = set_en;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER1_OFS, arg.reg);

	//imeg->reg_385.bit.ime_3dnr_statistic_output_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ms_roi_output_enable_reg(UINT32 set_en)
{
	//T_IME_TMNR_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	imeg->reg_384.bit.ime_3dnr_ps_smart_roi_ctrl = set_en;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_ps_smart_roi_ctrl = set_en;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_debug_reg(UINT32 set_mode, UINT32 set_mv0)
{
	//T_IME_TMNR_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER1_OFS);

	imeg->reg_385.bit.ime_3dnr_dbg_mode = set_mode;
	imeg->reg_385.bit.ime_3dnr_dbg_mv0 = set_mv0;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_y_pre_blur_strength_reg(UINT32 set_val)
{
	//T_IME_TMNR_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	imeg->reg_384.bit.ime_3dnr_pre_y_blur_str = set_val;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_pre_y_blur_str = set_val;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_nr_false_color_control_enable_reg(UINT32 set_en)
{
	//T_IME_TMNR_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	imeg->reg_384.bit.ime_3dnr_nr_c_fsv_en = set_en;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_nr_c_fsv_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_false_color_control_strength_reg(UINT32 set_val)
{
	//T_IME_TMNR_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	imeg->reg_384.bit.ime_3dnr_nr_c_fsv = set_val;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_nr_c_fsv = set_val;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_center_zero_enable_reg(UINT32 set_en)
{
	//T_IME_TMNR_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	imeg->reg_384.bit.ime_3dnr_nr_center_wzero_y_3d = set_en;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_nr_center_wzero_y_3d = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_mv_check_enable_reg(UINT32 set_en)
{
	//T_IME_TMNR_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	imeg->reg_384.bit.ime_3dnr_ps_mv_check_en = set_en;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_ps_mv_check_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_mv_roi_check_enable_reg(UINT32 set_en)
{
	//T_IME_TMNR_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	imeg->reg_384.bit.ime_3dnr_ps_mv_check_roi_en = set_en;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_ps_mv_check_roi_en = set_en;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_control_reg(UINT32 set_mv_info_mode, UINT32 set_ps_mode)
{
	//T_IME_TMNR_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	imeg->reg_384.bit.ime_3dnr_ps_mv_info_mode = set_mv_info_mode;
	imeg->reg_384.bit.ime_3dnr_ps_mode = set_ps_mode;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_ps_mv_info_mode = set_mv_info_mode;
	//imeg->reg_384.bit.ime_3dnr_ps_mode = set_ps_mode;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ne_sampling_horizontal_reg(UINT32 set_start_pos, UINT32 set_step, UINT32 set_num)
{
	//T_IME_TMNR_CONTROL_REGISTER1 arg1;
	//T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9 arg2;
	//T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10 arg3;

	//arg1.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER1_OFS);
	//arg2.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9_OFS);
	//arg3.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10_OFS);

	imeg->reg_385.bit.ime_3dnr_ne_sample_step_x = set_start_pos;

	imeg->reg_403.bit.ime_3dnr_ne_sample_num_x = set_step;

	imeg->reg_404.bit.ime_3dnr_ne_sample_start_x = set_num;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER1_OFS, arg1.reg);
	//IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9_OFS, arg2.reg);
	//IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10_OFS, arg3.reg);


	//imeg->reg_385.bit.ime_3dnr_ne_sample_step_x = set_start_pos;
	//imeg->reg_403.bit.ime_3dnr_ne_sample_num_x = set_step;
	//imeg->reg_404.bit.ime_3dnr_ne_sample_start_x = set_num;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ne_sampling_vertical_reg(UINT32 set_start_pos, UINT32 set_step, UINT32 set_num)
{
	//T_IME_TMNR_CONTROL_REGISTER1 arg1;
	//T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9 arg2;
	//T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10 arg3;

	//arg1.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER1_OFS);
	//arg2.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9_OFS);
	//arg3.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10_OFS);

	imeg->reg_385.bit.ime_3dnr_ne_sample_step_y = set_start_pos;

	imeg->reg_403.bit.ime_3dnr_ne_sample_num_y = set_step;

	imeg->reg_404.bit.ime_3dnr_ne_sample_start_y = set_num;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER1_OFS, arg1.reg);
	//IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9_OFS, arg2.reg);
	//IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10_OFS, arg3.reg);

	//imeg->reg_385.bit.ime_3dnr_ne_sample_step_y = set_start_pos;
	//imeg->reg_403.bit.ime_3dnr_ne_sample_num_y = set_step;
	//imeg->reg_404.bit.ime_3dnr_ne_sample_start_y = set_num;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_me_control_reg(UINT32 set_update_mode, UINT32 set_bndy, UINT32 set_ds_mode)
{
	//T_IME_TMNR_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	imeg->reg_384.bit.ime_3dnr_me_update_mode = set_update_mode;
	imeg->reg_384.bit.ime_3dnr_me_boundary_set = set_bndy;
	imeg->reg_384.bit.ime_3dnr_me_mv_ds_mode = set_ds_mode;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_me_update_mode = set_update_mode;
	//imeg->reg_384.bit.ime_3dnr_me_boundary_set = set_bndy;
	//imeg->reg_384.bit.ime_3dnr_me_mv_ds_mode = set_ds_mode;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_me_sad_reg(UINT32 set_sad_type, UINT32 set_sad_shift)
{
	//T_IME_TMNR_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER0_OFS);

	imeg->reg_384.bit.ime_3dnr_me_sad_type = set_sad_type;
	imeg->reg_384.bit.ime_3dnr_me_sad_shift = set_sad_shift;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_384.bit.ime_3dnr_me_sad_type = set_sad_type;
	//imeg->reg_384.bit.ime_3dnr_me_sad_shift = set_sad_shift;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_me_sad_penalty_reg(UINT32 *p_sad_penalty)
{
	//T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER0 arg1;
	//T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER1 arg2;
	//T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER2 arg3;

	//arg1.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER0_OFS);
	//arg2.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER1_OFS);
	//arg3.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER2_OFS);

	imeg->reg_394.bit.ime_3dnr_me_sad_penalty_0 = p_sad_penalty[0] & bit_mask[10];
	imeg->reg_394.bit.ime_3dnr_me_sad_penalty_1 = p_sad_penalty[1] & bit_mask[10];
	imeg->reg_394.bit.ime_3dnr_me_sad_penalty_2 = p_sad_penalty[2] & bit_mask[10];
	imeg->reg_395.bit.ime_3dnr_me_sad_penalty_3 = p_sad_penalty[3] & bit_mask[10];
	imeg->reg_395.bit.ime_3dnr_me_sad_penalty_4 = p_sad_penalty[4] & bit_mask[10];
	imeg->reg_395.bit.ime_3dnr_me_sad_penalty_5 = p_sad_penalty[5] & bit_mask[10];
	imeg->reg_396.bit.ime_3dnr_me_sad_penalty_6 = p_sad_penalty[6] & bit_mask[10];
	imeg->reg_396.bit.ime_3dnr_me_sad_penalty_7 = p_sad_penalty[7] & bit_mask[10];

	//IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER0_OFS, arg1.reg);
	//IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER1_OFS, arg2.reg);
	//IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER2_OFS, arg3.reg);
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_me_switch_threshold_reg(UINT32 *p_me_switch_th, UINT32 me_switch_ratio)
{
	UINT32 arg_val, i;
	//T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER5 arg;

	for (i = 0; i < 8; i += 4) {
		arg_val = 0;
		arg_val = ((p_me_switch_th[i + 3] & bit_mask[8]) << 24) + ((p_me_switch_th[i + 2] & bit_mask[8]) << 16) + ((p_me_switch_th[i + 1] & bit_mask[8]) << 8) + (p_me_switch_th[i] & bit_mask[8]);

		IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER3_OFS + i, arg_val);
	}

#if 0
	imeg->reg_397.bit.ime_3dnr_me_switch_th0 = p_me_switch_th[0];
	imeg->reg_397.bit.ime_3dnr_me_switch_th1 = p_me_switch_th[1];
	imeg->reg_397.bit.ime_3dnr_me_switch_th2 = p_me_switch_th[2];
	imeg->reg_397.bit.ime_3dnr_me_switch_th3 = p_me_switch_th[3];
	imeg->reg_398.bit.ime_3dnr_me_switch_th4 = p_me_switch_th[4];
	imeg->reg_398.bit.ime_3dnr_me_switch_th5 = p_me_switch_th[5];
	imeg->reg_398.bit.ime_3dnr_me_switch_th6 = p_me_switch_th[6];
	imeg->reg_398.bit.ime_3dnr_me_switch_th7 = p_me_switch_th[7];
#endif

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER5_OFS);

	imeg->reg_399.bit.ime_3dnr_me_switch_ratio = me_switch_ratio;

	//IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER5_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_me_detail_penalty_reg(UINT32 *p_me_detail_penalty)
{
	//T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER6 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER6_OFS);

	imeg->reg_400.bit.ime_3dnr_me_detail_penalty0 = p_me_detail_penalty[0] & bit_mask[4];
	imeg->reg_400.bit.ime_3dnr_me_detail_penalty1 = p_me_detail_penalty[1] & bit_mask[4];
	imeg->reg_400.bit.ime_3dnr_me_detail_penalty2 = p_me_detail_penalty[2] & bit_mask[4];
	imeg->reg_400.bit.ime_3dnr_me_detail_penalty3 = p_me_detail_penalty[3] & bit_mask[4];
	imeg->reg_400.bit.ime_3dnr_me_detail_penalty4 = p_me_detail_penalty[4] & bit_mask[4];
	imeg->reg_400.bit.ime_3dnr_me_detail_penalty5 = p_me_detail_penalty[5] & bit_mask[4];
	imeg->reg_400.bit.ime_3dnr_me_detail_penalty6 = p_me_detail_penalty[6] & bit_mask[4];
	imeg->reg_400.bit.ime_3dnr_me_detail_penalty7 = p_me_detail_penalty[7] & bit_mask[4];

	//IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER6_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_me_probability_reg(UINT32 *p_me_probability)
{
	//T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7_OFS);

	imeg->reg_401.bit.ime_3dnr_me_probability0 = p_me_probability[0] & bit_mask[1];
	imeg->reg_401.bit.ime_3dnr_me_probability1 = p_me_probability[1] & bit_mask[1];
	imeg->reg_401.bit.ime_3dnr_me_probability2 = p_me_probability[2] & bit_mask[1];
	imeg->reg_401.bit.ime_3dnr_me_probability3 = p_me_probability[3] & bit_mask[1];
	imeg->reg_401.bit.ime_3dnr_me_probability4 = p_me_probability[4] & bit_mask[1];
	imeg->reg_401.bit.ime_3dnr_me_probability5 = p_me_probability[5] & bit_mask[1];
	imeg->reg_401.bit.ime_3dnr_me_probability6 = p_me_probability[6] & bit_mask[1];
	imeg->reg_401.bit.ime_3dnr_me_probability7 = p_me_probability[7] & bit_mask[1];

	//IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7_OFS, arg.reg);

#if 0
	imeg->reg_401.bit.ime_3dnr_me_probability0 = p_me_probability[0];
	imeg->reg_401.bit.ime_3dnr_me_probability1 = p_me_probability[1];
	imeg->reg_401.bit.ime_3dnr_me_probability2 = p_me_probability[2];
	imeg->reg_401.bit.ime_3dnr_me_probability3 = p_me_probability[3];
	imeg->reg_401.bit.ime_3dnr_me_probability4 = p_me_probability[4];
	imeg->reg_401.bit.ime_3dnr_me_probability5 = p_me_probability[5];
	imeg->reg_401.bit.ime_3dnr_me_probability6 = p_me_probability[6];
	imeg->reg_401.bit.ime_3dnr_me_probability7 = p_me_probability[7];
#endif
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_me_rand_bit_reg(UINT32 me_rand_bitx, UINT32 me_rand_bity)
{
	//T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7_OFS);

	imeg->reg_401.bit.ime_3dnr_me_rand_bit_x = me_rand_bitx;
	imeg->reg_401.bit.ime_3dnr_me_rand_bit_y = me_rand_bity;

	//IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7_OFS, arg.reg);

	//imeg->reg_401.bit.ime_3dnr_me_rand_bit_x = me_rand_bitx;
	//imeg->reg_401.bit.ime_3dnr_me_rand_bit_y = me_rand_bity;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_me_threshold_reg(UINT32 me_min_detail, UINT32 me_periodic_th)
{
	//T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7 arg1;
	//T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER8 arg2;

	//arg1.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7_OFS);
	//arg2.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER8_OFS);

	imeg->reg_401.bit.ime_3dnr_me_min_detail = me_min_detail;
	imeg->reg_402.bit.ime_3dnr_me_periodic_th = me_periodic_th;

	//IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7_OFS, arg1.reg);
	//IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER8_OFS, arg2.reg);

	//imeg->reg_401.bit.ime_3dnr_me_min_detail = me_min_detail;
	//imeg->reg_402.bit.ime_3dnr_me_periodic_th = me_periodic_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_md_final_threshold_reg(UINT32 set_md_k1, UINT32 set_md_k2)
{
	//T_IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER10 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER10_OFS);

	imeg->reg_418.bit.ime_3dnr_md_k1 = set_md_k1;
	imeg->reg_418.bit.ime_3dnr_md_k2 = set_md_k2;

	//IME_SETREG(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER10_OFS, arg.reg);

	//imeg->reg_418.bit.ime_3dnr_md_k1 = set_md_k1;
	//imeg->reg_418.bit.ime_3dnr_md_k2 = set_md_k2;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_md_roi_final_threshold_reg(UINT32 set_md_roi_k1, UINT32 set_md_roi_k2)
{
	//T_IME_3DNR_ROI_MOTION_DETECTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_ROI_MOTION_DETECTION_CONTROL_REGISTER0_OFS);

	imeg->reg_444.bit.ime_3dnr_roi_md_k1 = set_md_roi_k1;
	imeg->reg_444.bit.ime_3dnr_roi_md_k2 = set_md_roi_k2;

	//IME_SETREG(IME_3DNR_ROI_MOTION_DETECTION_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_444.bit.ime_3dnr_roi_md_k1 = set_md_roi_k1;
	//imeg->reg_444.bit.ime_3dnr_roi_md_k2 = set_md_roi_k2;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_md_sad_coefs_reg(UINT32 *p_set_coefa, UINT32 *p_set_coefb)
{
	UINT32 arg_val, i;

	for (i = 0; i < 8; i += 4) {
		arg_val = 0;
		arg_val = ((p_set_coefa[i + 3] & bit_mask[6]) << 24) + ((p_set_coefa[i + 2] & bit_mask[6]) << 16) + ((p_set_coefa[i + 1] & bit_mask[6]) << 8) + (p_set_coefa[i] & bit_mask[6]);

		IME_SETREG(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER0_OFS + i, arg_val);
	}

	for (i = 0; i < 8; i += 2) {
		arg_val = 0;
		arg_val = ((p_set_coefb[i + 1] & bit_mask[14]) << 16) + (p_set_coefb[i] & bit_mask[14]);

		IME_SETREG(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER2_OFS + (i << 1), arg_val);
	}

#if 0
	imeg->reg_408.bit.ime_3dnr_md_sad_coef_a0 = p_set_coefa[0];
	imeg->reg_408.bit.ime_3dnr_md_sad_coef_a1 = p_set_coefa[1];
	imeg->reg_408.bit.ime_3dnr_md_sad_coef_a2 = p_set_coefa[2];
	imeg->reg_408.bit.ime_3dnr_md_sad_coef_a3 = p_set_coefa[3];
	imeg->reg_409.bit.ime_3dnr_md_sad_coef_a4 = p_set_coefa[4];
	imeg->reg_409.bit.ime_3dnr_md_sad_coef_a5 = p_set_coefa[5];
	imeg->reg_409.bit.ime_3dnr_md_sad_coef_a6 = p_set_coefa[6];
	imeg->reg_409.bit.ime_3dnr_md_sad_coef_a7 = p_set_coefa[7];

	imeg->reg_410.bit.ime_3dnr_md_sad_coef_b0 = p_set_coefb[0];
	imeg->reg_410.bit.ime_3dnr_md_sad_coef_b1 = p_set_coefb[1];
	imeg->reg_411.bit.ime_3dnr_md_sad_coef_b2 = p_set_coefb[2];
	imeg->reg_411.bit.ime_3dnr_md_sad_coef_b3 = p_set_coefb[3];
	imeg->reg_412.bit.ime_3dnr_md_sad_coef_b4 = p_set_coefb[4];
	imeg->reg_412.bit.ime_3dnr_md_sad_coef_b5 = p_set_coefb[5];
	imeg->reg_413.bit.ime_3dnr_md_sad_coef_b6 = p_set_coefb[6];
	imeg->reg_413.bit.ime_3dnr_md_sad_coef_b7 = p_set_coefb[7];
#endif
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_md_sad_standard_deviation_reg(UINT32 *p_set_std)
{
	UINT32 arg_val, i;

	for (i = 0; i < 8; i += 2) {
		arg_val = 0;
		arg_val = ((p_set_std[i + 1] & bit_mask[14]) << 16) + (p_set_std[i] & bit_mask[14]);

		IME_SETREG(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER6_OFS + (i << 1), arg_val);
	}

#if 0
	imeg->reg_414.bit.ime_3dnr_md_sad_std0 = p_set_std[0];
	imeg->reg_414.bit.ime_3dnr_md_sad_std1 = p_set_std[1];
	imeg->reg_415.bit.ime_3dnr_md_sad_std2 = p_set_std[2];
	imeg->reg_415.bit.ime_3dnr_md_sad_std3 = p_set_std[3];
	imeg->reg_416.bit.ime_3dnr_md_sad_std4 = p_set_std[4];
	imeg->reg_416.bit.ime_3dnr_md_sad_std5 = p_set_std[5];
	imeg->reg_417.bit.ime_3dnr_md_sad_std6 = p_set_std[6];
	imeg->reg_417.bit.ime_3dnr_md_sad_std7 = p_set_std[7];
#endif
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_mc_noise_base_level_reg(UINT32 *p_set_base_level)
{
	UINT32 arg_val, i;

	for (i = 0; i < 8; i += 2) {
		arg_val = 0;
		arg_val = ((p_set_base_level[i + 1] & bit_mask[14]) << 16) + (p_set_base_level[i] & bit_mask[14]);

		IME_SETREG(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER0_OFS + (i << 1), arg_val);
	}

#if 0
	imeg->reg_424.bit.ime_3dnr_mc_sad_base0 = p_set_base_level[0];
	imeg->reg_424.bit.ime_3dnr_mc_sad_base1 = p_set_base_level[1];
	imeg->reg_425.bit.ime_3dnr_mc_sad_base2 = p_set_base_level[2];
	imeg->reg_425.bit.ime_3dnr_mc_sad_base3 = p_set_base_level[3];
	imeg->reg_426.bit.ime_3dnr_mc_sad_base4 = p_set_base_level[4];
	imeg->reg_426.bit.ime_3dnr_mc_sad_base5 = p_set_base_level[5];
	imeg->reg_427.bit.ime_3dnr_mc_sad_base6 = p_set_base_level[6];
	imeg->reg_427.bit.ime_3dnr_mc_sad_base7 = p_set_base_level[7];
#endif
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_mc_edge_coefs_offset_reg(UINT32 *p_set_coefa, UINT32 *p_set_coefb)
{
	UINT32 arg_val, i;

	for (i = 0; i < 8; i += 4) {
		arg_val = 0;
		arg_val = ((p_set_coefa[i + 3] & bit_mask[6]) << 24) + ((p_set_coefa[i + 2] & bit_mask[6]) << 16) + ((p_set_coefa[i + 1] & bit_mask[6]) << 8) + (p_set_coefa[i] & bit_mask[6]);

		IME_SETREG(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER4_OFS + i, arg_val);
	}

	for (i = 0; i < 8; i += 2) {
		arg_val = 0;
		arg_val = ((p_set_coefb[i + 1] & bit_mask[14]) << 16) + (p_set_coefb[i] & bit_mask[14]);

		IME_SETREG(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER6_OFS + (i << 1), arg_val);
	}

#if 0
	imeg->reg_428.bit.ime_3dnr_mc_sad_coef_a0 = p_set_coefa[0];
	imeg->reg_428.bit.ime_3dnr_mc_sad_coef_a1 = p_set_coefa[1];
	imeg->reg_428.bit.ime_3dnr_mc_sad_coef_a2 = p_set_coefa[2];
	imeg->reg_428.bit.ime_3dnr_mc_sad_coef_a3 = p_set_coefa[3];

	imeg->reg_429.bit.ime_3dnr_mc_sad_coef_a4 = p_set_coefa[4];
	imeg->reg_429.bit.ime_3dnr_mc_sad_coef_a5 = p_set_coefa[5];
	imeg->reg_429.bit.ime_3dnr_mc_sad_coef_a6 = p_set_coefa[6];
	imeg->reg_429.bit.ime_3dnr_mc_sad_coef_a7 = p_set_coefa[7];

	imeg->reg_430.bit.ime_3dnr_mc_sad_coef_b0 = p_set_coefb[0];
	imeg->reg_430.bit.ime_3dnr_mc_sad_coef_b1 = p_set_coefb[1];
	imeg->reg_431.bit.ime_3dnr_mc_sad_coef_b2 = p_set_coefb[2];
	imeg->reg_431.bit.ime_3dnr_mc_sad_coef_b3 = p_set_coefb[3];
	imeg->reg_432.bit.ime_3dnr_mc_sad_coef_b4 = p_set_coefb[4];
	imeg->reg_432.bit.ime_3dnr_mc_sad_coef_b5 = p_set_coefb[5];
	imeg->reg_433.bit.ime_3dnr_mc_sad_coef_b6 = p_set_coefb[6];
	imeg->reg_433.bit.ime_3dnr_mc_sad_coef_b7 = p_set_coefb[7];
#endif
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_mc_sad_standard_deviation_reg(UINT32 *p_set_std)
{
	UINT32 arg_val, i;

	for (i = 0; i < 8; i += 2) {
		arg_val = 0;
		arg_val = ((p_set_std[i + 1] & bit_mask[14]) << 16) + (p_set_std[i] & bit_mask[14]);

		IME_SETREG(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER10_OFS + (i << 1), arg_val);
	}

#if 0
	imeg->reg_434.bit.ime_3dnr_mc_sad_std0 = p_set_std[0];
	imeg->reg_434.bit.ime_3dnr_mc_sad_std1 = p_set_std[1];
	imeg->reg_435.bit.ime_3dnr_mc_sad_std2 = p_set_std[2];
	imeg->reg_435.bit.ime_3dnr_mc_sad_std3 = p_set_std[3];
	imeg->reg_436.bit.ime_3dnr_mc_sad_std4 = p_set_std[4];
	imeg->reg_436.bit.ime_3dnr_mc_sad_std5 = p_set_std[5];
	imeg->reg_437.bit.ime_3dnr_mc_sad_std6 = p_set_std[6];
	imeg->reg_437.bit.ime_3dnr_mc_sad_std7 = p_set_std[7];
#endif
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_mc_final_threshold_reg(UINT32 set_mc_k1, UINT32 set_mc_k2)
{
	//T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER14 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER14_OFS);

	imeg->reg_438.bit.ime_3dnr_mc_k1 = set_mc_k1;
	imeg->reg_438.bit.ime_3dnr_mc_k2 = set_mc_k2;

	//IME_SETREG(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER14_OFS, arg.reg);

	//imeg->reg_438.bit.ime_3dnr_mc_k1 = set_mc_k1;
	//imeg->reg_438.bit.ime_3dnr_mc_k2 = set_mc_k2;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_mc_roi_final_threshold_reg(UINT32 set_mc_roi_k1, UINT32 set_mc_roi_k2)
{
	//T_IME_3DNR_ROI_MOTION_COMPENSATION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_ROI_MOTION_COMPENSATION_CONTROL_REGISTER0_OFS);

	imeg->reg_452.bit.ime_3dnr_roi_mc_k1 = set_mc_roi_k1;
	imeg->reg_452.bit.ime_3dnr_roi_mc_k2 = set_mc_roi_k2;

	//IME_SETREG(IME_3DNR_ROI_MOTION_COMPENSATION_CONTROL_REGISTER0_OFS, arg.reg);


	//imeg->reg_452.bit.ime_3dnr_roi_mc_k1 = set_mc_roi_k1;
	//imeg->reg_452.bit.ime_3dnr_roi_mc_k2 = set_mc_roi_k2;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_mv_threshold_reg(UINT32 set_mv_th, UINT32 set_roi_mv_th)
{
	//T_IME_3DNR_PS_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER0_OFS);

	imeg->reg_460.bit.ime_3dnr_ps_mv_th = set_mv_th;
	imeg->reg_460.bit.ime_3dnr_ps_roi_mv_th = set_roi_mv_th;

	//IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_460.bit.ime_3dnr_ps_mv_th = set_mv_th;
	//imeg->reg_460.bit.ime_3dnr_ps_roi_mv_th = set_roi_mv_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_mix_ratio_reg(UINT32 set_mix_rto0, UINT32 set_mix_rto1)
{
	//T_IME_3DNR_PS_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER1_OFS);

	imeg->reg_461.bit.ime_3dnr_ps_mix_ratio0 = set_mix_rto0;
	imeg->reg_461.bit.ime_3dnr_ps_mix_ratio1 = set_mix_rto1;

	//IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER1_OFS, arg.reg);

	//imeg->reg_461.bit.ime_3dnr_ps_mix_ratio0 = set_mix_rto0;
	//imeg->reg_461.bit.ime_3dnr_ps_mix_ratio1 = set_mix_rto1;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_mix_threshold_reg(UINT32 set_mix_th0, UINT32 set_mix_th1)
{
	//T_IME_3DNR_PS_CONTROL_REGISTER2 arg;

	//arg.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER2_OFS);

	imeg->reg_462.bit.ime_3dnr_ps_mix_th0 = set_mix_th0;
	imeg->reg_462.bit.ime_3dnr_ps_mix_th1 = set_mix_th1;

	//IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER2_OFS, arg.reg);


	//imeg->reg_462.bit.ime_3dnr_ps_mix_th0 = set_mix_th0;
	//imeg->reg_462.bit.ime_3dnr_ps_mix_th1 = set_mix_th1;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_mix_slope_reg(UINT32 set_mix_slp0, UINT32 set_mix_slp1)
{
	//T_IME_3DNR_PS_CONTROL_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER3_OFS);

	imeg->reg_463.bit.ime_3dnr_ps_mix_slope0 = set_mix_slp0;
	imeg->reg_463.bit.ime_3dnr_ps_mix_slope1 = set_mix_slp1;

	//IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER3_OFS, arg.reg);


	//imeg->reg_463.bit.ime_3dnr_ps_mix_slope0 = set_mix_slp0;
	//imeg->reg_463.bit.ime_3dnr_ps_mix_slope1 = set_mix_slp1;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_down_sample_reg(UINT32 set_ds_th)
{
	//T_IME_3DNR_PS_CONTROL_REGISTER4 arg;

	//arg.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER4_OFS);

	imeg->reg_472.bit.ime_3dnr_ps_ds_th = set_ds_th;

	//IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER4_OFS, arg.reg);

	//imeg->reg_472.bit.ime_3dnr_ps_ds_th = set_ds_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_roi_down_sample_reg(UINT32 set_ds_roi_th)
{
	//T_IME_3DNR_PS_CONTROL_REGISTER4 arg;

	//arg.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER4_OFS);

	imeg->reg_472.bit.ime_3dnr_ps_ds_th_roi = set_ds_roi_th;

	//IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER4_OFS, arg.reg);

	//imeg->reg_472.bit.ime_3dnr_ps_ds_th_roi = set_ds_roi_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_edge_control_reg(UINT32 set_start_point, UINT32 set_th0, UINT32 set_th1, UINT32 set_slope)
{
	//T_IME_3DNR_PS_CONTROL_REGISTER4 arg1;
	//T_IME_3DNR_PS_CONTROL_REGISTER5 arg2;
	//T_IME_3DNR_PS_CONTROL_REGISTER6 arg3;

	//arg1.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER4_OFS);
	//arg2.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER5_OFS);
	//arg3.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER6_OFS);

	imeg->reg_472.bit.ime_3dnr_ps_edge_w = set_start_point;

	imeg->reg_473.bit.ime_3dnr_ps_edge_th0 = set_th0;
	imeg->reg_473.bit.ime_3dnr_ps_edge_th1 = set_th1;

	imeg->reg_474.bit.ime_3dnr_ps_edge_slope = set_slope;

	//IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER4_OFS, arg1.reg);
	//IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER5_OFS, arg2.reg);
	//IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER6_OFS, arg3.reg);

	//imeg->reg_472.bit.ime_3dnr_ps_edge_w = set_start_point;

	//imeg->reg_473.bit.ime_3dnr_ps_edge_th0 = set_th0;
	//imeg->reg_473.bit.ime_3dnr_ps_edge_th1 = set_th1;

	//imeg->reg_474.bit.ime_3dnr_ps_edge_slope = set_slope;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_ps_path_error_threshold_reg(UINT32 set_th)
{
	//T_IME_3DNR_PS_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_PS_CONTROL_REGISTER0_OFS);

	imeg->reg_460.bit.ime_3dnr_ps_fs_th = set_th;

	//IME_SETREG(IME_3DNR_PS_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_460.bit.ime_3dnr_ps_fs_th = set_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_type_reg(UINT32 set_type)
{
	imeg->reg_385.bit.ime_3dnr_nr_type = set_type;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_luma_residue_threshold_reg(UINT32 set_th)
{
	//T_IME_3DNR_NR_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER0_OFS);

	imeg->reg_475.bit.ime_3dnr_nr_residue_th_y = set_th;

	//IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_475.bit.ime_3dnr_nr_residue_th_y = set_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_chroma_residue_threshold_reg(UINT32 set_th)
{
	//T_IME_3DNR_NR_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER0_OFS);

	imeg->reg_475.bit.ime_3dnr_nr_residue_th_c = set_th;

	//IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER0_OFS, arg.reg);

	//imeg->reg_475.bit.ime_3dnr_nr_residue_th_c = set_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_varied_frequency_filter_weight_reg(UINT32 *p_set_wets)
{
	UINT32 arg_val, i;

	i = 0;
	arg_val = ((p_set_wets[i + 3] & bit_mask[8]) << 24) + ((p_set_wets[i + 2] & bit_mask[8]) << 16) + ((p_set_wets[i + 1] & bit_mask[8]) << 8) + (p_set_wets[i] & bit_mask[8]);

	IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER1_OFS, arg_val);

	//imeg->reg_476.bit.ime_3dnr_nr_freq_w0 = p_set_wets[0];
	//imeg->reg_476.bit.ime_3dnr_nr_freq_w1 = p_set_wets[1];
	//imeg->reg_476.bit.ime_3dnr_nr_freq_w2 = p_set_wets[2];
	//imeg->reg_476.bit.ime_3dnr_nr_freq_w3 = p_set_wets[3];
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_luma_filter_weight_reg(UINT32 *p_set_wets)
{
	UINT32 arg_val, i;

	for (i = 0; i < 8; i += 4) {
		arg_val = 0;
		arg_val = ((p_set_wets[i + 3] & bit_mask[8]) << 24) + ((p_set_wets[i + 2] & bit_mask[8]) << 16) + ((p_set_wets[i + 1] & bit_mask[8]) << 8) + (p_set_wets[i] & bit_mask[8]);

		IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER3_OFS + i, arg_val);
	}

	//imeg->reg_484.bit.ime_3dnr_nr_luma_w0 = p_set_wets[0];
	//imeg->reg_484.bit.ime_3dnr_nr_luma_w1 = p_set_wets[1];
	//imeg->reg_484.bit.ime_3dnr_nr_luma_w2 = p_set_wets[2];
	//imeg->reg_484.bit.ime_3dnr_nr_luma_w3 = p_set_wets[3];
	//imeg->reg_485.bit.ime_3dnr_nr_luma_w4 = p_set_wets[4];
	//imeg->reg_485.bit.ime_3dnr_nr_luma_w5 = p_set_wets[5];
	//imeg->reg_485.bit.ime_3dnr_nr_luma_w6 = p_set_wets[6];
	//imeg->reg_485.bit.ime_3dnr_nr_luma_w7 = p_set_wets[7];
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_prefiltering_reg(UINT32 *p_set_strs, UINT32 *p_set_rto)
{
	UINT32 arg_val, i;
	//T_IME_3DNR_NR_CONTROL_REGISTER8 arg;

	i = 0;
	arg_val = (p_set_strs[i + 3] << 24) + (p_set_strs[i + 2] << 16) + (p_set_strs[i + 1] << 8) + (p_set_strs[i]);
	IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER6_OFS, arg_val);

	//imeg->reg_492.bit.ime_3dnr_nr_pre_filtering_str0 = p_set_strs[0];
	//imeg->reg_492.bit.ime_3dnr_nr_pre_filtering_str1 = p_set_strs[1];
	//imeg->reg_492.bit.ime_3dnr_nr_pre_filtering_str2 = p_set_strs[2];
	//imeg->reg_492.bit.ime_3dnr_nr_pre_filtering_str3 = p_set_strs[3];

	//arg.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER8_OFS);
	imeg->reg_496.bit.ime_3dnr_nr_pre_filtering_ratio0 = p_set_rto[0];
	imeg->reg_496.bit.ime_3dnr_nr_pre_filtering_ratio1 = p_set_rto[1];

	//IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER8_OFS, arg.reg);

	//imeg->reg_496.bit.ime_3dnr_nr_pre_filtering_ratio0 = p_set_rto[0];
	//imeg->reg_496.bit.ime_3dnr_nr_pre_filtering_ratio1 = p_set_rto[1];
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_snr_control_reg(UINT32 *p_set_strs, UINT32 set_th)
{
	//T_IME_3DNR_NR_CONTROL_REGISTER8 arg1;
	//T_IME_3DNR_NR_CONTROL_REGISTER9 arg2;
	//T_IME_3DNR_NR_CONTROL_REGISTER10 arg3;

	//arg1.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER8_OFS);
	//arg2.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER9_OFS);
	//arg3.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER10_OFS);

	imeg->reg_496.bit.ime_3dnr_nr_snr_str0 = p_set_strs[0];
	imeg->reg_496.bit.ime_3dnr_nr_snr_str1 = p_set_strs[1];
	imeg->reg_497.bit.ime_3dnr_nr_snr_str2 = p_set_strs[2];

	imeg->reg_498.bit.ime_3dnr_nr_base_th_snr = set_th;

	//IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER8_OFS, arg1.reg);
	//IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER9_OFS, arg2.reg);
	//IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER10_OFS, arg3.reg);

	//imeg->reg_496.bit.ime_3dnr_nr_snr_str0 = p_set_strs[0];
	//imeg->reg_496.bit.ime_3dnr_nr_snr_str1 = p_set_strs[1];
	//imeg->reg_497.bit.ime_3dnr_nr_snr_str2 = p_set_strs[2];

	//imeg->reg_498.bit.ime_3dnr_nr_base_th_snr = set_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_tnr_control_reg(UINT32 *p_set_strs, UINT32 set_th)
{
	//T_IME_3DNR_NR_CONTROL_REGISTER9 arg1;
	//T_IME_3DNR_NR_CONTROL_REGISTER10 arg2;

	//arg1.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER9_OFS);
	//arg2.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER10_OFS);

	imeg->reg_497.bit.ime_3dnr_nr_tnr_str0 = p_set_strs[0];
	imeg->reg_497.bit.ime_3dnr_nr_tnr_str1 = p_set_strs[1];
	imeg->reg_497.bit.ime_3dnr_nr_tnr_str2 = p_set_strs[2];

	imeg->reg_498.bit.ime_3dnr_nr_base_th_tnr = set_th;

	//IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER9_OFS, arg1.reg);
	//IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER10_OFS, arg2.reg);

	//imeg->reg_497.bit.ime_3dnr_nr_tnr_str0 = p_set_strs[0];
	//imeg->reg_497.bit.ime_3dnr_nr_tnr_str1 = p_set_strs[1];
	//imeg->reg_497.bit.ime_3dnr_nr_tnr_str2 = p_set_strs[2];

	//imeg->reg_498.bit.ime_3dnr_nr_base_th_tnr = set_th;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_luma_noise_reduction_reg(UINT32 *p_set_th, UINT32 *p_set_lut)
{
	UINT32 arg_val, i;

	i = 0;
	arg_val = ((p_set_th[i + 3] & bit_mask[6]) << 24) + ((p_set_th[i + 2] & bit_mask[6]) << 16) + ((p_set_th[i + 1] & bit_mask[6]) << 8) + (p_set_th[i] & bit_mask[6]);

	IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER11_OFS + i, arg_val);

	//imeg->reg_499.bit.ime_3dnr_nr_y_3d_th0 = p_set_th[0];
	//imeg->reg_499.bit.ime_3dnr_nr_y_3d_th1 = p_set_th[1];
	//imeg->reg_499.bit.ime_3dnr_nr_y_3d_th2 = p_set_th[2];
	//imeg->reg_499.bit.ime_3dnr_nr_y_3d_th3 = p_set_th[3];

	for (i = 0; i < 8; i += 4) {
		arg_val = 0;
		arg_val = ((p_set_lut[i + 3] & bit_mask[7]) << 24) + ((p_set_lut[i + 2] & bit_mask[7]) << 16) + ((p_set_lut[i + 1] & bit_mask[7]) << 8) + (p_set_lut[i] & bit_mask[7]);

		IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER12_OFS + i, arg_val);
	}

#if 0
	imeg->reg_500.bit.ime_3dnr_nr_y_3d_lut0 = p_set_lut[0];
	imeg->reg_500.bit.ime_3dnr_nr_y_3d_lut1 = p_set_lut[1];
	imeg->reg_500.bit.ime_3dnr_nr_y_3d_lut2 = p_set_lut[2];
	imeg->reg_500.bit.ime_3dnr_nr_y_3d_lut3 = p_set_lut[3];
	imeg->reg_501.bit.ime_3dnr_nr_y_3d_lut4 = p_set_lut[4];
	imeg->reg_501.bit.ime_3dnr_nr_y_3d_lut5 = p_set_lut[5];
	imeg->reg_501.bit.ime_3dnr_nr_y_3d_lut6 = p_set_lut[6];
	imeg->reg_501.bit.ime_3dnr_nr_y_3d_lut7 = p_set_lut[7];
#endif
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_nr_chroma_noise_reduction_reg(UINT32 *p_set_lut, UINT32 *p_set_rto)
{
	UINT32 arg_val, i;
	//T_IME_3DNR_NR_CONTROL_REGISTER16 arg;

	for (i = 0; i < 8; i += 4) {
		arg_val = 0;
		arg_val = ((p_set_lut[i + 3] & bit_mask[7]) << 24) + ((p_set_lut[i + 2] & bit_mask[7]) << 16) + ((p_set_lut[i + 1] & bit_mask[7]) << 8) + (p_set_lut[i] & bit_mask[7]);

		IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER14_OFS + i, arg_val);
	}

	//imeg->reg_502.bit.ime_3dnr_nr_c_3d_lut0 = p_set_lut[0];
	//imeg->reg_502.bit.ime_3dnr_nr_c_3d_lut1 = p_set_lut[1];
	//imeg->reg_502.bit.ime_3dnr_nr_c_3d_lut2 = p_set_lut[2];
	//imeg->reg_502.bit.ime_3dnr_nr_c_3d_lut3 = p_set_lut[3];
	//imeg->reg_503.bit.ime_3dnr_nr_c_3d_lut4 = p_set_lut[4];
	//imeg->reg_503.bit.ime_3dnr_nr_c_3d_lut5 = p_set_lut[5];
	//imeg->reg_503.bit.ime_3dnr_nr_c_3d_lut6 = p_set_lut[6];
	//imeg->reg_503.bit.ime_3dnr_nr_c_3d_lut7 = p_set_lut[7];

	//arg.reg = IME_GETREG(IME_3DNR_NR_CONTROL_REGISTER16_OFS);

	imeg->reg_504.bit.ime_3dnr_nr_c_3d_ratio0 = p_set_rto[0];
	imeg->reg_504.bit.ime_3dnr_nr_c_3d_ratio1 = p_set_rto[1];

	//IME_SETREG(IME_3DNR_NR_CONTROL_REGISTER16_OFS, arg.reg);

	//imeg->reg_504.bit.ime_3dnr_nr_c_3d_ratio0 = p_set_rto[0];
	//imeg->reg_504.bit.ime_3dnr_nr_c_3d_ratio1 = p_set_rto[1];
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_statistic_data_output_start_position_reg(UINT32 set_start_x, UINT32 set_start_y)
{
	//T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10_OFS);

	imeg->reg_404.bit.ime_3dnr_ne_sample_start_x = set_start_x;
	imeg->reg_404.bit.ime_3dnr_ne_sample_start_y = set_start_y;

	//IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10_OFS, arg.reg);

	//imeg->reg_404.bit.ime_3dnr_ne_sample_start_x = set_start_x;
	//imeg->reg_404.bit.ime_3dnr_ne_sample_start_y = set_start_y;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_statistic_data_output_sample_numbers_reg(UINT32 set_num_x, UINT32 set_num_y)
{
	//T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9_OFS);

	imeg->reg_403.bit.ime_3dnr_ne_sample_num_x = set_num_x;
	imeg->reg_403.bit.ime_3dnr_ne_sample_num_y = set_num_y;

	//IME_SETREG(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9_OFS, arg.reg);

	//imeg->reg_403.bit.ime_3dnr_ne_sample_num_x = set_num_x;
	//imeg->reg_403.bit.ime_3dnr_ne_sample_num_y = set_num_y;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_statistic_data_output_sample_steps_reg(UINT32 set_step_x, UINT32 set_step_y)
{
	//T_IME_TMNR_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_TMNR_CONTROL_REGISTER1_OFS);

	imeg->reg_385.bit.ime_3dnr_ne_sample_step_x = set_step_x;
	imeg->reg_385.bit.ime_3dnr_ne_sample_step_y = set_step_y;

	//IME_SETREG(IME_TMNR_CONTROL_REGISTER1_OFS, arg.reg);

	//imeg->reg_385.bit.ime_3dnr_ne_sample_step_x = set_step_x;
	//imeg->reg_385.bit.ime_3dnr_ne_sample_step_y = set_step_y;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_in_ref_y_lineoffset_reg(UINT32 set_ofs_y)
{
	//T_IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0_OFS);

	imeg->reg_512.bit.ime_3dnr_ref_dram_y_ofsi = set_ofs_y >> 2;

	//IME_SETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_in_ref_y_lineoffset_reg(VOID)
{
	//T_IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0_OFS);

	return (imeg->reg_512.bit.ime_3dnr_ref_dram_y_ofsi << 2);
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_in_ref_uv_lineoffset_reg(UINT32 set_ofs_uv)
{
	//T_IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1_OFS);

	imeg->reg_513.bit.ime_3dnr_ref_dram_uv_ofsi = set_ofs_uv >> 2;

	//IME_SETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1_OFS, arg.reg);

	//imeg->reg_513.bit.ime_3dnr_ref_dram_uv_ofsi = set_ofs_uv >> 2;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_in_ref_uv_lineoffset_reg(VOID)
{
	//T_IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1_OFS);

	return (imeg->reg_513.bit.ime_3dnr_ref_dram_uv_ofsi << 2);
}
//-------------------------------------------------------------------------------



VOID ime_tmnr_set_in_ref_y_addr_reg(UINT32 set_addr_y)
{
	//T_IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0_OFS);

	ime_tmnr_in_ref_y_addr = set_addr_y;

    if (ime_tmnr_in_ref_va2pa_flag == TRUE) {
	    imeg->reg_514.bit.ime_3dnr_ref_dram_y_sai = ime_platform_va2pa(ime_tmnr_in_ref_y_addr) >> 2;
	} else {
        imeg->reg_514.bit.ime_3dnr_ref_dram_y_sai = (ime_tmnr_in_ref_y_addr) >> 2;
	}

	//IME_SETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_in_ref_y_addr_reg(VOID)
{
	return ime_tmnr_in_ref_y_addr;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_in_ref_uv_addr_reg(UINT32 set_addr_uv)
{
	//T_IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1_OFS);

	ime_tmnr_in_ref_uv_addr = set_addr_uv;

    if (ime_tmnr_in_ref_va2pa_flag == TRUE) {
	    imeg->reg_515.bit.ime_3dnr_ref_dram_uv_sai = ime_platform_va2pa(ime_tmnr_in_ref_uv_addr) >> 2;
	} else {
        imeg->reg_515.bit.ime_3dnr_ref_dram_uv_sai = (ime_tmnr_in_ref_uv_addr) >> 2;
	}

	//IME_SETREG(IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_in_ref_uv_addr_reg(VOID)
{
	return ime_tmnr_in_ref_uv_addr;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_out_ref_y_lineoffset_reg(UINT32 set_ofs_y)
{
	//T_IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0_OFS);

	imeg->reg_516.bit.ime_3dnr_ref_dram_y_ofso = set_ofs_y >> 2;

	//IME_SETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_ref_y_lineoffset_reg(VOID)
{
	//T_IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0_OFS);

	return (imeg->reg_516.bit.ime_3dnr_ref_dram_y_ofso << 2);
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_out_ref_uv_lineoffset_reg(UINT32 set_ofs_uv)
{
	//T_IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1_OFS);

	imeg->reg_517.bit.ime_3dnr_ref_dram_uv_ofso = set_ofs_uv >> 2;

	//IME_SETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_ref_uv_lineoffset_reg(VOID)
{
	//T_IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1_OFS);

	return (imeg->reg_517.bit.ime_3dnr_ref_dram_uv_ofso << 2);
}
//-------------------------------------------------------------------------------



VOID ime_tmnr_set_out_ref_y_addr_reg(UINT32 set_addr_y)
{
	//T_IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0_OFS);

	ime_tmnr_out_ref_y_addr = set_addr_y;

	imeg->reg_518.bit.ime_3dnr_ref_dram_y_sao = ime_platform_va2pa(ime_tmnr_out_ref_y_addr) >> 2;

	//IME_SETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_ref_y_addr_reg(VOID)
{
	return ime_tmnr_out_ref_y_addr;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_out_ref_uv_addr_reg(UINT32 set_addr_uv)
{
	//T_IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1_OFS);

	ime_tmnr_out_ref_uv_addr = set_addr_uv;

	imeg->reg_519.bit.ime_3dnr_ref_dram_uv_sao = ime_platform_va2pa(ime_tmnr_out_ref_uv_addr) >> 2;

	//IME_SETREG(IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_ref_uv_addr_reg(VOID)
{
	return ime_tmnr_out_ref_uv_addr;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_in_motion_status_lineoffset_reg(UINT32 set_ofs)
{
	//T_IME_3DNR_MOTION_STATUS_INPUT_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_STATUS_INPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	imeg->reg_520.bit.ime_3dnr_mot_dram_ofsi = (set_ofs >> 2);

	//IME_SETREG(IME_3DNR_MOTION_STATUS_INPUT_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_in_motion_status_lineoffset_reg(VOID)
{
	//T_IME_3DNR_MOTION_STATUS_INPUT_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_STATUS_INPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	return (imeg->reg_520.bit.ime_3dnr_mot_dram_ofsi << 2);
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_in_motion_status_address_reg(UINT32 set_addr)
{
	//T_IME_3DNR_MOTION_STATUS_INPUT_DMA_STARTING_ADDRESS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_STATUS_INPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS);

	ime_tmnr_in_ms_addr = set_addr;

	imeg->reg_521.bit.ime_3dnr_mot_dram_sai = ime_platform_va2pa(ime_tmnr_in_ms_addr) >> 2;

	//IME_SETREG(IME_3DNR_MOTION_STATUS_INPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_in_motion_status_address_reg(VOID)
{
	return ime_tmnr_in_ms_addr;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_out_motion_status_address_reg(UINT32 set_addr)
{
	//T_IME_3DNR_MOTION_STATUS_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_STATUS_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS);
	ime_tmnr_out_ms_addr = set_addr;

	imeg->reg_523.bit.ime_3dnr_mot_dram_sao = ime_platform_va2pa(ime_tmnr_out_ms_addr) >> 2;

	//IME_SETREG(IME_3DNR_MOTION_STATUS_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_motion_status_address_reg(VOID)
{
	return ime_tmnr_out_ms_addr;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_out_roi_motion_status_lineoffset_reg(UINT32 set_ofs)
{
	//T_IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	imeg->reg_524.bit.ime_3dnr_mot_roi_dram_ofso = (set_ofs >> 2);

	//IME_SETREG(IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_roi_motion_status_lineoffset_reg(VOID)
{
	//T_IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	return (imeg->reg_524.bit.ime_3dnr_mot_roi_dram_ofso << 2);
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_out_roi_motion_status_address_reg(UINT32 set_addr)
{
	//T_IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS);
	ime_tmnr_out_roi_ms_addr = set_addr;

	imeg->reg_525.bit.ime_3dnr_mot_roi_dram_sao = ime_platform_va2pa(ime_tmnr_out_roi_ms_addr) >> 2;

	//IME_SETREG(IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_roi_motion_status_address_reg(VOID)
{
	return ime_tmnr_out_roi_ms_addr;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_in_motion_vector_lineoffset_reg(UINT32 set_ofs)
{
	//T_IME_3DNR_MOTION_VECTOR_INPUT_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_VECTOR_INPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	imeg->reg_526.bit.ime_3dnr_mv_dram_ofsi = (set_ofs >> 2);

	//IME_SETREG(IME_3DNR_MOTION_VECTOR_INPUT_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_in_motion_vector_lineoffset_reg(VOID)
{
	//T_IME_3DNR_MOTION_VECTOR_INPUT_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_VECTOR_INPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	return (imeg->reg_526.bit.ime_3dnr_mv_dram_ofsi << 2);
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_in_motion_vector_address_reg(UINT32 set_addr)
{
	//T_IME_3DNR_MOTION_VECTOR_INPUT_DMA_STARTING_ADDRESS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_VECTOR_INPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS);
	ime_tmnr_in_mv_addr = set_addr;

	imeg->reg_527.bit.ime_3dnr_mv_dram_sai = ime_platform_va2pa(ime_tmnr_in_mv_addr) >> 2;

	//IME_SETREG(IME_3DNR_MOTION_VECTOR_INPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_in_motion_vector_address_reg(VOID)
{
	return ime_tmnr_in_mv_addr;
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_out_motion_vector_address_reg(UINT32 set_addr)
{
	//T_IME_3DNR_MOTION_VECTOR_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_MOTION_VECTOR_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS);
	ime_tmnr_out_mv_addr = set_addr;

	imeg->reg_529.bit.ime_3dnr_mv_dram_sao = ime_platform_va2pa(ime_tmnr_out_mv_addr) >> 2;

	//IME_SETREG(IME_3DNR_MOTION_VECTOR_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_motion_vector_address_reg(VOID)
{
	return ime_tmnr_out_mv_addr;
}
//-------------------------------------------------------------------------------

VOID ime_tmnr_set_out_statistic_lineoffset_reg(UINT32 set_ofs)
{
	//T_IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	imeg->reg_530.bit.ime_3dnr_statistic_dram_ofso = (set_ofs >> 2);

	//IME_SETREG(IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_statistic_lineoffset_reg(VOID)
{
	//T_IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	return (imeg->reg_530.bit.ime_3dnr_statistic_dram_ofso << 2);
}
//-------------------------------------------------------------------------------


VOID ime_tmnr_set_out_statistic_address_reg(UINT32 set_addr)
{
	//T_IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS);
	ime_tmnr_out_sta_addr = set_addr;

	imeg->reg_531.bit.ime_3dnr_statistic_dram_sao = ime_platform_va2pa(ime_tmnr_out_sta_addr) >> 2;

	//IME_SETREG(IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_out_statistic_address_reg(VOID)
{
	return ime_tmnr_out_sta_addr;
}
//-------------------------------------------------------------------------------


// read only
UINT32 ime_tmnr_get_sum_of_sad_value_reg(VOID)
{
	//T_IME_3DNR_READ_ONLY_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_3DNR_READ_ONLY_REGISTER0_OFS);

	return imeg->reg_391.bit.ime_3dnr_ro_sad_sum;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_sum_of_mv_length_reg(VOID)
{
	//T_IME_3DNR_READ_ONLY_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_3DNR_READ_ONLY_REGISTER1_OFS);

	return imeg->reg_392.bit.ime_3dnr_ro_mv_sum;
}
//-------------------------------------------------------------------------------

UINT32 ime_tmnr_get_total_sampling_number_reg(VOID)
{
	//T_IME_3DNR_READ_ONLY_REGISTER2 arg;

	//arg.reg = IME_GETREG(IME_3DNR_READ_ONLY_REGISTER2_OFS);

	return imeg->reg_393.bit.ime_3dnr_ro_sample_cnt;
}
//-------------------------------------------------------------------------------

#endif


VOID ime_tmrn_set_ref_in_va2pa_enable_reg(BOOL set_en)
{
    ime_tmnr_in_ref_va2pa_flag = set_en;
}
//-------------------------------------------------------------------------------


