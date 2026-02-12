
/*
    IME module driver

    NT96510 IME module driver.

    @file       ime_out_path3_base.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/



#include "ime_reg.h"
#include "ime_int_public.h"

#include "ime_out_path3_base.h"

UINT32 ime_p3_out_y_addr = 0x0;
UINT32 ime_p3_out_u_addr = 0x0;
static UINT32 ime_p3_out_stitching_y_addr = 0x0;
static UINT32 ime_p3_out_stitching_u_addr = 0x0;


#if (IPP_DRIVER_NEW_REG_HANDLE == DISABLE)

// output path3 APIs
VOID ime_open_output_p3_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_p3_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_flip_enable_p3_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_p3_flip_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_output_dram_enable_p3_reg(UINT32 set_en)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	arg.bit.ime_out_en_p3 = set_en;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS, arg.reg);
}

//-------------------------------------------------------------------------------


VOID ime_set_output_image_format_p3_reg(UINT32 set_out_fmt)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	arg.bit.ime_omat3 = set_out_fmt;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scale_interpolation_method_p3_reg(UINT32 scl_method)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	arg.bit.ime_scl_method_p3 = scl_method;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_type_p3_reg(UINT32 out_fmt_type)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	arg.bit.ime_out_type_p3 = 1;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_scale_factor_h_p3_reg(UINT32 h_scl_sel, UINT32 h_scl_rate, UINT32 h_scl_factor)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER1 arg1;
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER2 arg2;

	arg1.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS);
	arg1.bit.h3_ud = h_scl_sel;
	arg1.bit.h3_dnrate = h_scl_rate;

	arg2.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER2_OFS);
	arg2.bit.h3_sfact = h_scl_factor;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS, arg1.reg);
	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER2_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scale_factor_v_p3_reg(UINT32 v_scl_sel, UINT32 v_scl_rate, UINT32 v_scl_factor)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER1 arg1;
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER2 arg2;

	arg1.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS);
	arg1.bit.v3_ud = v_scl_sel;
	arg1.bit.v3_dnrate = v_scl_rate;
	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS, arg1.reg);

	arg2.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER2_OFS);
	arg2.bit.v3_sfact = v_scl_factor;
	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER2_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scaling_size_p3_reg(UINT32 h_scl_size, UINT32 v_scl_size)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER21 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER21_OFS);

	arg.bit.h3_scl_size = h_scl_size;
	arg.bit.v3_scl_size = v_scl_size;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER21_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_scaling_size_p3_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER21 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER21_OFS);

	*p_get_size_h = arg.bit.h3_scl_size;
	*p_get_size_v = arg.bit.v3_scl_size;
}

//-------------------------------------------------------------------------------


VOID ime_set_horizontal_scale_filtering_p3_reg(UINT32 h_scl_filter_en, UINT32 h_scl_filter_coef)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS);

#if 0
	arg.bit.h3_filtmode = 0;
	arg.bit.h3_filtcoef = 0;
#else
	arg.bit.h3_filtmode = h_scl_filter_en;
	arg.bit.h3_filtcoef = h_scl_filter_coef;
#endif

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_vertical_scale_filtering_p3_reg(UINT32 v_scl_filter_en, UINT32 v_scl_filter_coef)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS);

#if 0
	arg.bit.v3_filtmode = 0;
	arg.bit.v3_filtcoef = 0;
#else
	arg.bit.v3_filtmode = v_scl_filter_en;
	arg.bit.v3_filtcoef = v_scl_filter_coef;
#endif

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor_base_p3_reg(UINT32 base_scl_h_factor, UINT32 base_scl_v_factor)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER3_OFS);

	arg.bit.isd_h_base_p3 = base_scl_h_factor;
	arg.bit.isd_v_base_p3 = base_scl_v_factor;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_integration_scaling_factor0_p3_reg(UINT32 isd_scl_h_factor, UINT32 isd_scl_v_factor)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER4 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER4_OFS);

	arg.bit.isd_h_sfact0_p3 = isd_scl_h_factor;
	arg.bit.isd_v_sfact0_p3 = isd_scl_v_factor;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER4_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor1_p3_reg(UINT32 isd_scl_h_factor, UINT32 isd_scl_v_factor)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER5 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER5_OFS);

	arg.bit.isd_h_sfact1_p3 = isd_scl_h_factor;
	arg.bit.isd_v_sfact1_p3 = isd_scl_v_factor;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER5_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor2_p3_reg(UINT32 isd_scl_h_factor, UINT32 isd_scl_v_factor)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER6 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER6_OFS);

	arg.bit.isd_h_sfact2_p3 = isd_scl_h_factor;
	arg.bit.isd_v_sfact2_p3 = isd_scl_v_factor;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER6_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_integration_scaling_ctrl_p3_reg(UINT32 isd_mode, UINT32 isd_h_coef_num, UINT32 isd_v_coef_num)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER7 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER7_OFS);

	arg.bit.isd_p3_mode = isd_mode;
	arg.bit.isd_p3_h_coef_num = isd_h_coef_num - 1;
	arg.bit.isd_p3_v_coef_num = isd_v_coef_num - 1;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER7_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_h_coefs_p3_reg(INT32 *hcoefs)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER8 arg0;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER9 arg1;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER10 arg2;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER11 arg3;
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER16 arg4;

	T_IME_OUTPUT_PATH3_CONTROL_REGISTER17 arg5;
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER18 arg6;

	//arg0.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER8_OFS);
	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER9_OFS);
	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER10_OFS);
	//arg3.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER11_OFS);
	//arg4.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS);

	arg5.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER17_OFS);
	arg6.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER18_OFS);

	for (i = 0; i < 17; i++) {
		get_coef_val[i] = ime_int_to_2comp(hcoefs[i], 12);
	}

#if 0
	arg0.bit.isd_p3_h_coef0 = get_coef_val[0] & 0x000000ff;
	arg0.bit.isd_p3_h_coef1 = get_coef_val[1] & 0x000000ff;
	arg0.bit.isd_p3_h_coef2 = get_coef_val[2] & 0x000000ff;
	arg0.bit.isd_p3_h_coef3 = get_coef_val[3] & 0x000000ff;

	arg1.bit.isd_p3_h_coef4 = get_coef_val[4] & 0x000000ff;
	arg1.bit.isd_p3_h_coef5 = get_coef_val[5] & 0x000000ff;
	arg1.bit.isd_p3_h_coef6 = get_coef_val[6] & 0x000000ff;
	arg1.bit.isd_p3_h_coef7 = get_coef_val[7] & 0x000000ff;

	arg2.bit.isd_p3_h_coef8 = get_coef_val[8] & 0x000000ff;
	arg2.bit.isd_p3_h_coef9 = get_coef_val[9] & 0x000000ff;
	arg2.bit.isd_p3_h_coef10 = get_coef_val[10] & 0x000000ff;
	arg2.bit.isd_p3_h_coef11 = get_coef_val[11] & 0x000000ff;

	arg3.bit.isd_p3_h_coef12 = get_coef_val[12] & 0x000000ff;
	arg3.bit.isd_p3_h_coef13 = get_coef_val[13] & 0x000000ff;
	arg3.bit.isd_p3_h_coef14 = get_coef_val[14] & 0x000000ff;
	arg3.bit.isd_p3_h_coef15 = get_coef_val[15] & 0x000000ff;
#endif

	for (i = 0; i < 4; i++) {
		reg_set_val = 0;

		idx = (i << 2);
		reg_ofs = IME_OUTPUT_PATH3_CONTROL_REGISTER8_OFS + (i << 2);
		reg_set_val = (get_coef_val[idx] & 0xFF) + ((get_coef_val[idx + 1] & 0xFF) << 8) + ((get_coef_val[idx + 2] & 0xFF) << 16) + ((get_coef_val[idx + 3] & 0xFF) << 24);

		IME_SETREG(reg_ofs, reg_set_val);
	}

	arg4.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS);
	arg4.bit.isd_p3_h_coef16 = get_coef_val[16];
	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS, arg4.reg);


	arg5.bit.isd_p3_h_coef_msb0 = (get_coef_val[0] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_h_coef_msb1 = (get_coef_val[1] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_h_coef_msb2 = (get_coef_val[2] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_h_coef_msb3 = (get_coef_val[3] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_h_coef_msb4 = (get_coef_val[4] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_h_coef_msb5 = (get_coef_val[5] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_h_coef_msb6 = (get_coef_val[6] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_h_coef_msb7 = (get_coef_val[7] >> 8) & 0x0000000f;

	arg6.bit.isd_p3_h_coef_msb8 = (get_coef_val[8] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_h_coef_msb9 = (get_coef_val[9] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_h_coef_msb10 = (get_coef_val[10] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_h_coef_msb11 = (get_coef_val[11] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_h_coef_msb12 = (get_coef_val[12] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_h_coef_msb13 = (get_coef_val[13] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_h_coef_msb14 = (get_coef_val[14] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_h_coef_msb15 = (get_coef_val[15] >> 8) & 0x0000000f;


	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER8_OFS, arg0.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER9_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER10_OFS, arg2.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER11_OFS, arg3.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS, arg4.reg);

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER17_OFS, arg5.reg);
	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER18_OFS, arg6.reg);

}
//-------------------------------------------------------------------------------


VOID ime_set_integration_scaling_v_coefs_p3_reg(INT32 *vcoefs)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER12 arg0;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER13 arg1;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER14 arg2;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER15 arg3;
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER16 arg4;

	T_IME_OUTPUT_PATH3_CONTROL_REGISTER19 arg5;
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER20 arg6;

	//arg0.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER12_OFS);
	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER13_OFS);
	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER14_OFS);
	//arg3.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER15_OFS);
	//arg4.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS);

	arg5.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER19_OFS);
	arg6.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER20_OFS);

	for (i = 0; i < 17; i++) {
		get_coef_val[i] = ime_int_to_2comp(vcoefs[i], 12);
	}

#if 0
	arg0.bit.isd_p3_v_coef0 = get_coef_val[0] & 0x000000ff;
	arg0.bit.isd_p3_v_coef1 = get_coef_val[1] & 0x000000ff;
	arg0.bit.isd_p3_v_coef2 = get_coef_val[2] & 0x000000ff;
	arg0.bit.isd_p3_v_coef3 = get_coef_val[3] & 0x000000ff;

	arg1.bit.isd_p3_v_coef4 = get_coef_val[4] & 0x000000ff;
	arg1.bit.isd_p3_v_coef5 = get_coef_val[5] & 0x000000ff;
	arg1.bit.isd_p3_v_coef6 = get_coef_val[6] & 0x000000ff;
	arg1.bit.isd_p3_v_coef7 = get_coef_val[7] & 0x000000ff;

	arg2.bit.isd_p3_v_coef8 = get_coef_val[8] & 0x000000ff;
	arg2.bit.isd_p3_v_coef9 = get_coef_val[9] & 0x000000ff;
	arg2.bit.isd_p3_v_coef10 = get_coef_val[10] & 0x000000ff;
	arg2.bit.isd_p3_v_coef11 = get_coef_val[11] & 0x000000ff;

	arg3.bit.isd_p3_v_coef12 = get_coef_val[12] & 0x000000ff;
	arg3.bit.isd_p3_v_coef13 = get_coef_val[13] & 0x000000ff;
	arg3.bit.isd_p3_v_coef14 = get_coef_val[14] & 0x000000ff;
	arg3.bit.isd_p3_v_coef15 = get_coef_val[15] & 0x000000ff;
#endif

	for (i = 0; i < 4; i++) {
		reg_set_val = 0;

		idx = (i << 2);
		reg_ofs = IME_OUTPUT_PATH3_CONTROL_REGISTER12_OFS + (i << 2);
		reg_set_val = (get_coef_val[idx] & 0xFF) + ((get_coef_val[idx + 1] & 0xFF) << 8) + ((get_coef_val[idx + 2] & 0xFF) << 16) + ((get_coef_val[idx + 3] & 0xFF) << 24);

		IME_SETREG(reg_ofs, reg_set_val);
	}

	arg4.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS);
	arg4.bit.isd_p3_v_coef16 = get_coef_val[16] & 0x000000ff;
	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS, arg4.reg);


	arg5.bit.isd_p3_v_coef_msb0 = (get_coef_val[0] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_v_coef_msb1 = (get_coef_val[1] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_v_coef_msb2 = (get_coef_val[2] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_v_coef_msb3 = (get_coef_val[3] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_v_coef_msb4 = (get_coef_val[4] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_v_coef_msb5 = (get_coef_val[5] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_v_coef_msb6 = (get_coef_val[6] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_v_coef_msb7 = (get_coef_val[7] >> 8) & 0x0000000f;

	arg6.bit.isd_p3_v_coef_msb8 = (get_coef_val[8] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_v_coef_msb9 = (get_coef_val[9] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_v_coef_msb10 = (get_coef_val[10] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_v_coef_msb11 = (get_coef_val[11] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_v_coef_msb12 = (get_coef_val[12] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_v_coef_msb13 = (get_coef_val[13] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_v_coef_msb14 = (get_coef_val[14] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_v_coef_msb15 = (get_coef_val[15] >> 8) & 0x0000000f;


	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER12_OFS, arg0.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER13_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER14_OFS, arg2.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER15_OFS, arg3.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS, arg4.reg);

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER19_OFS, arg5.reg);
	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER20_OFS, arg6.reg);

}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_h_coefs_sum_p3_reg(INT32 coef_sum_all, INT32 coef_sum_half)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER25 arg1;
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER26 arg2;

	arg1.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER25_OFS);
	arg2.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER26_OFS);

	arg1.bit.ime_p3_isd_h_coef_all = ime_int_to_2comp(coef_sum_all, 17);
	arg2.bit.ime_p3_isd_h_coef_half = ime_int_to_2comp(coef_sum_half, 17);

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER25_OFS, arg1.reg);
	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER26_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_v_coefs_sum_p3_reg(INT32 coef_sum_all, INT32 coef_sum_half)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER27 arg1;
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER28 arg2;

	arg1.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER27_OFS);
	arg2.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER28_OFS);

	arg1.bit.ime_p3_isd_v_coef_all = ime_int_to_2comp(coef_sum_all, 17);
	arg2.bit.ime_p3_isd_v_coef_half = ime_int_to_2comp(coef_sum_half, 17);

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER27_OFS, arg1.reg);
	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER28_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_scaling_enhance_factor_p3_reg(UINT32 scl_enh_factor, UINT32 scl_enh_sft_bit)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	arg.bit.ime_scl_enh_fact_p3 = scl_enh_factor;
	arg.bit.ime_scl_enh_bit_p3 = scl_enh_sft_bit;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_crop_coordinate_p3_reg(UINT32 crop_pos_x, UINT32 crop_pos_y)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER22 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER22_OFS);

	arg.bit.ime_cropout_x_p3 = crop_pos_x;
	arg.bit.ime_cropout_y_p3 = crop_pos_y;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER22_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_output_crop_coordinate_p3_reg(UINT32 *p_get_cpos_x, UINT32 *p_get_cpos_y)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER22 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER22_OFS);

	*p_get_cpos_x = arg.bit.ime_cropout_x_p3;
	*p_get_cpos_y = arg.bit.ime_cropout_y_p3;
}
//-------------------------------------------------------------------------------


VOID ime_set_output_lineoffset_y_p3_reg(UINT32 out_lofs)
{
	T_IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER0_OFS);

	arg.bit.dram_ofso_p3_y0 = (out_lofs >> 2);

	IME_SETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_lineoffset_c_p3_reg(UINT32 out_lofs)
{
	T_IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER1_OFS);

	arg.bit.dram_ofso_p3_uv0 = (out_lofs >> 2);

	IME_SETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_channel_addr_p3_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr)
{
	T_IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER0 arg1;
	T_IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER1 arg2;

	ime_p3_out_y_addr = out_addr_y;
	ime_p3_out_u_addr = out_addr_cb;

	arg1.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER0_OFS);
	arg2.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER1_OFS);

	arg1.bit.dram_sao_p3_y0 = ime_platform_va2pa(ime_p3_out_y_addr);
	arg2.bit.dram_sao_p3_uv0 = ime_platform_va2pa(ime_p3_out_u_addr);

	IME_SETREG(IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER0_OFS, arg1.reg);
	IME_SETREG(IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER1_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_output_size_p3_reg(UINT32 set_size_h, UINT32 set_size_v)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER23 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER23_OFS);

	arg.bit.h3_osize = set_size_h;
	arg.bit.v3_osize = set_size_v;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER23_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_get_output_size_p3_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER23 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER23_OFS);

	*p_get_size_h = arg.bit.h3_osize;
	*p_get_size_v = arg.bit.v3_osize;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_lineoffset_y_p3_reg(VOID)
{
	T_IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER0_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_lineoffset_c_p3_reg(VOID)
{
	T_IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER1_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_output_path_enable_status_p3_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 path_en;

	path_en.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return path_en.bit.ime_p3_en;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_output_format_p3_reg(VOID)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_omat3;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_format_type_p3_reg(VOID)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_out_type_p3;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_scaling_method_p3_reg(VOID)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_scl_method_p3;
}
//-------------------------------------------------------------------------------

// output path3
UINT32 ime_get_output_addr_y_p3_reg(VOID)
{
	//T_IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER0 Path3Info;

	//Path3Info.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER0_OFS);

	//return (Path3Info.reg);

	return ime_p3_out_y_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_addr_cb_p3_reg(VOID)
{
	//T_IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER1 Path3Info;

	//Path3Info.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER1_OFS);

	//return (Path3Info.reg);

	return ime_p3_out_u_addr;
}
//-------------------------------------------------------------------------------

VOID ime_clamp_p3_reg(UINT32 min_y, UINT32 max_y, UINT32 min_uv, UINT32 max_uv)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER24 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER24_OFS);

	arg.bit.ime_y_clamp_min_p3  = min_y;
	arg.bit.ime_y_clamp_max_p3  = max_y;
	arg.bit.ime_uv_clamp_min_p3 = min_uv;
	arg.bit.ime_uv_clamp_max_p3 = max_uv;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER24_OFS, arg.reg);
}


// stitching
// stitching - path3
VOID ime_set_stitching_enable_p3_reg(UINT32 set_en)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	arg.bit.ime_sprt_out_en_p3 = set_en;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_stitching_base_position_p3_reg(UINT32 base_pos)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER30 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER30_OFS);

	arg.bit.ime_sprt_hbl_p3 = base_pos;

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER30_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_enable_p3_reg(VOID)
{
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_sprt_out_en_p3;
}
//-------------------------------------------------------------------------------



VOID ime_set_stitching_output_lineoffset_y2_p3_reg(UINT32 lofs)
{
	T_IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER2 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER2_OFS);

	arg.bit.dram_ofso_p3_y1 = (lofs >> 2);

	IME_SETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER2_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_stitching_output_lineoffset_c2_p3_reg(UINT32 lofs)
{
	T_IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER3_OFS);

	arg.bit.dram_ofso_p3_uv1 = (lofs >> 2);

	IME_SETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_lineoffset_y2_p3_reg(VOID)
{
	T_IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER2 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER2_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_lineoffset_c2_p3_reg(VOID)
{
	T_IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER3_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------


VOID ime_set_stitching_output_channel_addr2_p3_reg(UINT32 addr_y, UINT32 addr_cb, UINT32 addr_cr)
{
	T_IME_OUTPUT_PATH3_DMA_BUFFER2_REGISTER3 arg0;
	T_IME_OUTPUT_PATH3_DMA_BUFFER1_REGISTER4 arg1;

	ime_p3_out_stitching_y_addr = addr_y;
	ime_p3_out_stitching_u_addr = addr_cb;

	arg0.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_BUFFER2_REGISTER3_OFS);
	arg1.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_BUFFER1_REGISTER4_OFS);

	arg0.bit.dram_sao_p3_y1 = ime_platform_va2pa(ime_p3_out_stitching_y_addr);//uioutaddry;
	arg1.bit.dram_sao_p3_uv1 = ime_platform_va2pa(ime_p3_out_stitching_u_addr);//uioutaddrcb;

	IME_SETREG(IME_OUTPUT_PATH3_DMA_BUFFER2_REGISTER3_OFS, arg0.reg);
	IME_SETREG(IME_OUTPUT_PATH3_DMA_BUFFER1_REGISTER4_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_addr_y2_p3_reg(VOID)
{
	// T_IME_OUTPUT_PATH3_DMA_BUFFER2_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_BUFFER2_REGISTER3_OFS);

	//return arg.reg;

	return ime_p3_out_stitching_y_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_addr_cb2_p3_reg(VOID)
{
	//T_IME_OUTPUT_PATH3_DMA_BUFFER1_REGISTER4 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_BUFFER1_REGISTER4_OFS);

	//return arg.reg;

	return ime_p3_out_stitching_u_addr;
}
//-------------------------------------------------------------------------------

#else

// output path3 APIs
VOID ime_open_output_p3_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_p3_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_flip_enable_p3_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_p3_flip_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_output_dram_enable_p3_reg(UINT32 set_en)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	imeg->reg_76.bit.ime_out_en_p3 = set_en;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS, arg.reg);
}

//-------------------------------------------------------------------------------


VOID ime_set_output_image_format_p3_reg(UINT32 set_out_fmt)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	imeg->reg_76.bit.ime_omat3 = set_out_fmt;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scale_interpolation_method_p3_reg(UINT32 scl_method)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	imeg->reg_76.bit.ime_scl_method_p3 = scl_method;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_type_p3_reg(UINT32 out_fmt_type)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	imeg->reg_76.bit.ime_out_type_p3 = 1;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_scale_factor_h_p3_reg(UINT32 h_scl_sel, UINT32 h_scl_rate, UINT32 h_scl_factor)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER1 arg1;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER2 arg2;

	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS);
	imeg->reg_77.bit.h3_ud = h_scl_sel;
	imeg->reg_77.bit.h3_dnrate = h_scl_rate;

	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER2_OFS);
	imeg->reg_78.bit.h3_sfact = h_scl_factor;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER2_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scale_factor_v_p3_reg(UINT32 v_scl_sel, UINT32 v_scl_rate, UINT32 v_scl_factor)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER1 arg1;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER2 arg2;

	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS);
	imeg->reg_77.bit.v3_ud = v_scl_sel;
	imeg->reg_77.bit.v3_dnrate = v_scl_rate;
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS, arg1.reg);

	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER2_OFS);
	imeg->reg_78.bit.v3_sfact = v_scl_factor;
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER2_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scaling_size_p3_reg(UINT32 h_scl_size, UINT32 v_scl_size)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER21 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER21_OFS);

	imeg->reg_97.bit.h3_scl_size = h_scl_size;
	imeg->reg_97.bit.v3_scl_size = v_scl_size;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER21_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_scaling_size_p3_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER21 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER21_OFS);

	*p_get_size_h = imeg->reg_97.bit.h3_scl_size;
	*p_get_size_v = imeg->reg_97.bit.v3_scl_size;
}

//-------------------------------------------------------------------------------


VOID ime_set_horizontal_scale_filtering_p3_reg(UINT32 h_scl_filter_en, UINT32 h_scl_filter_coef)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS);

#if 0
	imeg->reg_77.bit.h3_filtmode = 0;
	imeg->reg_77.bit.h3_filtcoef = 0;
#else
	imeg->reg_77.bit.h3_filtmode = h_scl_filter_en;
	imeg->reg_77.bit.h3_filtcoef = h_scl_filter_coef;
#endif

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_vertical_scale_filtering_p3_reg(UINT32 v_scl_filter_en, UINT32 v_scl_filter_coef)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS);

#if 0
	imeg->reg_77.bit.v3_filtmode = 0;
	imeg->reg_77.bit.v3_filtcoef = 0;
#else
	imeg->reg_77.bit.v3_filtmode = v_scl_filter_en;
	imeg->reg_77.bit.v3_filtcoef = v_scl_filter_coef;
#endif

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor_base_p3_reg(UINT32 base_scl_h_factor, UINT32 base_scl_v_factor)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER3_OFS);

	imeg->reg_79.bit.isd_h_base_p3 = base_scl_h_factor;
	imeg->reg_79.bit.isd_v_base_p3 = base_scl_v_factor;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_integration_scaling_factor0_p3_reg(UINT32 isd_scl_h_factor, UINT32 isd_scl_v_factor)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER4 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER4_OFS);

	imeg->reg_80.bit.isd_h_sfact0_p3 = isd_scl_h_factor;
	imeg->reg_80.bit.isd_v_sfact0_p3 = isd_scl_v_factor;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER4_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor1_p3_reg(UINT32 isd_scl_h_factor, UINT32 isd_scl_v_factor)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER5 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER5_OFS);

	imeg->reg_81.bit.isd_h_sfact1_p3 = isd_scl_h_factor;
	imeg->reg_81.bit.isd_v_sfact1_p3 = isd_scl_v_factor;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER5_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor2_p3_reg(UINT32 isd_scl_h_factor, UINT32 isd_scl_v_factor)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER6 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER6_OFS);

	imeg->reg_82.bit.isd_h_sfact2_p3 = isd_scl_h_factor;
	imeg->reg_82.bit.isd_v_sfact2_p3 = isd_scl_v_factor;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER6_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_integration_scaling_ctrl_p3_reg(UINT32 isd_mode, UINT32 isd_h_coef_num, UINT32 isd_v_coef_num)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER7 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER7_OFS);

	imeg->reg_83.bit.isd_p3_mode = isd_mode;
	imeg->reg_83.bit.isd_p3_h_coef_num = isd_h_coef_num - 1;
	imeg->reg_83.bit.isd_p3_v_coef_num = isd_v_coef_num - 1;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER7_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_h_coefs_p3_reg(INT32 *hcoefs)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER8 arg0;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER9 arg1;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER10 arg2;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER11 arg3;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER16 arg4;

	T_IME_OUTPUT_PATH3_CONTROL_REGISTER17 arg5;
	T_IME_OUTPUT_PATH3_CONTROL_REGISTER18 arg6;

	//arg0.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER8_OFS);
	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER9_OFS);
	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER10_OFS);
	//arg3.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER11_OFS);
	//arg4.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS);

	arg5.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER17_OFS);
	arg6.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER18_OFS);

	for (i = 0; i < 17; i++) {
		get_coef_val[i] = ime_int_to_2comp(hcoefs[i], 12);
	}

#if 0
	arg0.bit.isd_p3_h_coef0 = get_coef_val[0] & 0x000000ff;
	arg0.bit.isd_p3_h_coef1 = get_coef_val[1] & 0x000000ff;
	arg0.bit.isd_p3_h_coef2 = get_coef_val[2] & 0x000000ff;
	arg0.bit.isd_p3_h_coef3 = get_coef_val[3] & 0x000000ff;

	arg1.bit.isd_p3_h_coef4 = get_coef_val[4] & 0x000000ff;
	arg1.bit.isd_p3_h_coef5 = get_coef_val[5] & 0x000000ff;
	arg1.bit.isd_p3_h_coef6 = get_coef_val[6] & 0x000000ff;
	arg1.bit.isd_p3_h_coef7 = get_coef_val[7] & 0x000000ff;

	arg2.bit.isd_p3_h_coef8 = get_coef_val[8] & 0x000000ff;
	arg2.bit.isd_p3_h_coef9 = get_coef_val[9] & 0x000000ff;
	arg2.bit.isd_p3_h_coef10 = get_coef_val[10] & 0x000000ff;
	arg2.bit.isd_p3_h_coef11 = get_coef_val[11] & 0x000000ff;

	arg3.bit.isd_p3_h_coef12 = get_coef_val[12] & 0x000000ff;
	arg3.bit.isd_p3_h_coef13 = get_coef_val[13] & 0x000000ff;
	arg3.bit.isd_p3_h_coef14 = get_coef_val[14] & 0x000000ff;
	arg3.bit.isd_p3_h_coef15 = get_coef_val[15] & 0x000000ff;
#endif

	for (i = 0; i < 4; i++) {
		reg_set_val = 0;

		idx = (i << 2);
		reg_ofs = IME_OUTPUT_PATH3_CONTROL_REGISTER8_OFS + (i << 2);
		reg_set_val = (get_coef_val[idx] & 0x000000FF) + ((get_coef_val[idx + 1] & 0x000000FF) << 8) + ((get_coef_val[idx + 2] & 0x000000FF) << 16) + ((get_coef_val[idx + 3] & 0x000000FF) << 24);

		IME_SETREG(reg_ofs, reg_set_val);
	}

	//arg4.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS);
	imeg->reg_92.bit.isd_p3_h_coef16 = get_coef_val[16] & 0x000000FF;
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS, arg4.reg);


	arg5.bit.isd_p3_h_coef_msb0 = (get_coef_val[0] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_h_coef_msb1 = (get_coef_val[1] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_h_coef_msb2 = (get_coef_val[2] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_h_coef_msb3 = (get_coef_val[3] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_h_coef_msb4 = (get_coef_val[4] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_h_coef_msb5 = (get_coef_val[5] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_h_coef_msb6 = (get_coef_val[6] >> 8) & 0x0000000f;
	arg5.bit.isd_p3_h_coef_msb7 = (get_coef_val[7] >> 8) & 0x0000000f;

	arg6.bit.isd_p3_h_coef_msb8 = (get_coef_val[8] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_h_coef_msb9 = (get_coef_val[9] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_h_coef_msb10 = (get_coef_val[10] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_h_coef_msb11 = (get_coef_val[11] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_h_coef_msb12 = (get_coef_val[12] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_h_coef_msb13 = (get_coef_val[13] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_h_coef_msb14 = (get_coef_val[14] >> 8) & 0x0000000f;
	arg6.bit.isd_p3_h_coef_msb15 = (get_coef_val[15] >> 8) & 0x0000000f;


	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER8_OFS, arg0.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER9_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER10_OFS, arg2.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER11_OFS, arg3.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS, arg4.reg);

	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER17_OFS, arg5.reg);
	IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER18_OFS, arg6.reg);

}
//-------------------------------------------------------------------------------


VOID ime_set_integration_scaling_v_coefs_p3_reg(INT32 *vcoefs)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER12 arg0;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER13 arg1;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER14 arg2;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER15 arg3;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER16 arg4;

	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER19 arg5;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER20 arg6;

	//arg0.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER12_OFS);
	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER13_OFS);
	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER14_OFS);
	//arg3.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER15_OFS);
	//arg4.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS);

	//arg5.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER19_OFS);
	//arg6.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER20_OFS);

	for (i = 0; i < 17; i++) {
		get_coef_val[i] = ime_int_to_2comp(vcoefs[i], 12);
	}

#if 0
	arg0.bit.isd_p3_v_coef0 = get_coef_val[0] & 0x000000ff;
	arg0.bit.isd_p3_v_coef1 = get_coef_val[1] & 0x000000ff;
	arg0.bit.isd_p3_v_coef2 = get_coef_val[2] & 0x000000ff;
	arg0.bit.isd_p3_v_coef3 = get_coef_val[3] & 0x000000ff;

	arg1.bit.isd_p3_v_coef4 = get_coef_val[4] & 0x000000ff;
	arg1.bit.isd_p3_v_coef5 = get_coef_val[5] & 0x000000ff;
	arg1.bit.isd_p3_v_coef6 = get_coef_val[6] & 0x000000ff;
	arg1.bit.isd_p3_v_coef7 = get_coef_val[7] & 0x000000ff;

	arg2.bit.isd_p3_v_coef8 = get_coef_val[8] & 0x000000ff;
	arg2.bit.isd_p3_v_coef9 = get_coef_val[9] & 0x000000ff;
	arg2.bit.isd_p3_v_coef10 = get_coef_val[10] & 0x000000ff;
	arg2.bit.isd_p3_v_coef11 = get_coef_val[11] & 0x000000ff;

	arg3.bit.isd_p3_v_coef12 = get_coef_val[12] & 0x000000ff;
	arg3.bit.isd_p3_v_coef13 = get_coef_val[13] & 0x000000ff;
	arg3.bit.isd_p3_v_coef14 = get_coef_val[14] & 0x000000ff;
	arg3.bit.isd_p3_v_coef15 = get_coef_val[15] & 0x000000ff;
#endif

	for (i = 0; i < 4; i++) {
		reg_set_val = 0;

		idx = (i << 2);
		reg_ofs = IME_OUTPUT_PATH3_CONTROL_REGISTER12_OFS + (i << 2);
		reg_set_val = (get_coef_val[idx] & 0x000000FF) + ((get_coef_val[idx + 1] & 0x000000FF) << 8) + ((get_coef_val[idx + 2] & 0x000000FF) << 16) + ((get_coef_val[idx + 3] & 0x000000FF) << 24);

		IME_SETREG(reg_ofs, reg_set_val);
	}

	//arg4.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS);
	imeg->reg_92.bit.isd_p3_v_coef16 = get_coef_val[16] & 0x000000FF;
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS, arg4.reg);

	imeg->reg_95.bit.isd_p3_v_coef_msb0 = (get_coef_val[0] >> 8) & 0x0000000f;
	imeg->reg_95.bit.isd_p3_v_coef_msb1 = (get_coef_val[1] >> 8) & 0x0000000f;
	imeg->reg_95.bit.isd_p3_v_coef_msb2 = (get_coef_val[2] >> 8) & 0x0000000f;
	imeg->reg_95.bit.isd_p3_v_coef_msb3 = (get_coef_val[3] >> 8) & 0x0000000f;
	imeg->reg_95.bit.isd_p3_v_coef_msb4 = (get_coef_val[4] >> 8) & 0x0000000f;
	imeg->reg_95.bit.isd_p3_v_coef_msb5 = (get_coef_val[5] >> 8) & 0x0000000f;
	imeg->reg_95.bit.isd_p3_v_coef_msb6 = (get_coef_val[6] >> 8) & 0x0000000f;
	imeg->reg_95.bit.isd_p3_v_coef_msb7 = (get_coef_val[7] >> 8) & 0x0000000f;

	imeg->reg_96.bit.isd_p3_v_coef_msb8 = (get_coef_val[8] >> 8) & 0x0000000f;
	imeg->reg_96.bit.isd_p3_v_coef_msb9 = (get_coef_val[9] >> 8) & 0x0000000f;
	imeg->reg_96.bit.isd_p3_v_coef_msb10 = (get_coef_val[10] >> 8) & 0x0000000f;
	imeg->reg_96.bit.isd_p3_v_coef_msb11 = (get_coef_val[11] >> 8) & 0x0000000f;
	imeg->reg_96.bit.isd_p3_v_coef_msb12 = (get_coef_val[12] >> 8) & 0x0000000f;
	imeg->reg_96.bit.isd_p3_v_coef_msb13 = (get_coef_val[13] >> 8) & 0x0000000f;
	imeg->reg_96.bit.isd_p3_v_coef_msb14 = (get_coef_val[14] >> 8) & 0x0000000f;
	imeg->reg_96.bit.isd_p3_v_coef_msb15 = (get_coef_val[15] >> 8) & 0x0000000f;


	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER12_OFS, arg0.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER13_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER14_OFS, arg2.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER15_OFS, arg3.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS, arg4.reg);

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER19_OFS, arg5.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER20_OFS, arg6.reg);

}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_h_coefs_sum_p3_reg(INT32 coef_sum_all, INT32 coef_sum_half)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER25 arg1;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER26 arg2;

	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER25_OFS);
	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER26_OFS);

	imeg->reg_124.bit.ime_p3_isd_h_coef_all = ime_int_to_2comp(coef_sum_all, 17);
	imeg->reg_125.bit.ime_p3_isd_h_coef_half = ime_int_to_2comp(coef_sum_half, 17);

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER25_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER26_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_v_coefs_sum_p3_reg(INT32 coef_sum_all, INT32 coef_sum_half)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER27 arg1;
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER28 arg2;

	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER27_OFS);
	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER28_OFS);

	imeg->reg_126.bit.ime_p3_isd_v_coef_all = ime_int_to_2comp(coef_sum_all, 17);
	imeg->reg_127.bit.ime_p3_isd_v_coef_half = ime_int_to_2comp(coef_sum_half, 17);

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER27_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER28_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_scaling_enhance_factor_p3_reg(UINT32 scl_enh_factor, UINT32 scl_enh_sft_bit)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	imeg->reg_76.bit.ime_scl_enh_fact_p3 = scl_enh_factor;
	imeg->reg_76.bit.ime_scl_enh_bit_p3 = scl_enh_sft_bit;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_crop_coordinate_p3_reg(UINT32 crop_pos_x, UINT32 crop_pos_y)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER22 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER22_OFS);

	imeg->reg_98.bit.ime_cropout_x_p3 = crop_pos_x;
	imeg->reg_98.bit.ime_cropout_y_p3 = crop_pos_y;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER22_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_output_crop_coordinate_p3_reg(UINT32 *p_get_cpos_x, UINT32 *p_get_cpos_y)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER22 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER22_OFS);

	*p_get_cpos_x = imeg->reg_98.bit.ime_cropout_x_p3;
	*p_get_cpos_y = imeg->reg_98.bit.ime_cropout_y_p3;
}
//-------------------------------------------------------------------------------


VOID ime_set_output_lineoffset_y_p3_reg(UINT32 out_lofs)
{
	//T_IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER0_OFS);

	imeg->reg_101.bit.dram_ofso_p3_y0 = (out_lofs >> 2);

	//IME_SETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_lineoffset_c_p3_reg(UINT32 out_lofs)
{
	//T_IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER1_OFS);

	imeg->reg_102.bit.dram_ofso_p3_uv0 = (out_lofs >> 2);

	//IME_SETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_channel_addr_p3_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr)
{
	//T_IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER0 arg1;
	//T_IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER1 arg2;

	ime_p3_out_y_addr = out_addr_y;
	ime_p3_out_u_addr = out_addr_cb;

	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER0_OFS);
	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER1_OFS);

	imeg->reg_103.bit.dram_sao_p3_y0 = ime_platform_va2pa(ime_p3_out_y_addr);
	imeg->reg_104.bit.dram_sao_p3_uv0 = ime_platform_va2pa(ime_p3_out_u_addr);

	//IME_SETREG(IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER0_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER1_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_output_size_p3_reg(UINT32 set_size_h, UINT32 set_size_v)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER23 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER23_OFS);

	imeg->reg_99.bit.h3_osize = set_size_h;
	imeg->reg_99.bit.v3_osize = set_size_v;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER23_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_get_output_size_p3_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER23 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER23_OFS);

	*p_get_size_h = imeg->reg_99.bit.h3_osize;
	*p_get_size_v = imeg->reg_99.bit.v3_osize;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_lineoffset_y_p3_reg(VOID)
{
	//T_IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER0_OFS);

	return imeg->reg_101.word;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_lineoffset_c_p3_reg(VOID)
{
	//T_IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER1_OFS);

	return imeg->reg_102.word;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_output_path_enable_status_p3_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 path_en;

	//path_en.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return imeg->reg_1.bit.ime_p3_en;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_output_format_p3_reg(VOID)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	return imeg->reg_76.bit.ime_omat3;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_format_type_p3_reg(VOID)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	return imeg->reg_76.bit.ime_out_type_p3;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_scaling_method_p3_reg(VOID)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	return imeg->reg_76.bit.ime_scl_method_p3;
}
//-------------------------------------------------------------------------------

// output path3
UINT32 ime_get_output_addr_y_p3_reg(VOID)
{
	//T_IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER0 Path3Info;

	//Path3Info.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER0_OFS);

	//return (Path3Info.reg);

	return ime_p3_out_y_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_addr_cb_p3_reg(VOID)
{
	//T_IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER1 Path3Info;

	//Path3Info.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER1_OFS);

	//return (Path3Info.reg);

	return ime_p3_out_u_addr;
}
//-------------------------------------------------------------------------------

VOID ime_clamp_p3_reg(UINT32 min_y, UINT32 max_y, UINT32 min_uv, UINT32 max_uv)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER24 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER24_OFS);

	imeg->reg_100.bit.ime_y_clamp_min_p3  = min_y;
	imeg->reg_100.bit.ime_y_clamp_max_p3  = max_y;
	imeg->reg_100.bit.ime_uv_clamp_min_p3 = min_uv;
	imeg->reg_100.bit.ime_uv_clamp_max_p3 = max_uv;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER24_OFS, arg.reg);
}


// stitching
// stitching - path3
VOID ime_set_stitching_enable_p3_reg(UINT32 set_en)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	imeg->reg_76.bit.ime_sprt_out_en_p3 = set_en;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_stitching_base_position_p3_reg(UINT32 base_pos)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER30 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER30_OFS);

	imeg->reg_250.bit.ime_sprt_hbl_p3 = base_pos;

	//IME_SETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER30_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_enable_p3_reg(VOID)
{
	//T_IME_OUTPUT_PATH3_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS);

	return imeg->reg_76.bit.ime_sprt_out_en_p3;
}
//-------------------------------------------------------------------------------



VOID ime_set_stitching_output_lineoffset_y2_p3_reg(UINT32 lofs)
{
	//T_IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER2 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER2_OFS);

	imeg->reg_251.bit.dram_ofso_p3_y1 = (lofs >> 2);

	//IME_SETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER2_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_stitching_output_lineoffset_c2_p3_reg(UINT32 lofs)
{
	//T_IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER3_OFS);

	imeg->reg_252.bit.dram_ofso_p3_uv1 = (lofs >> 2);

	//IME_SETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_lineoffset_y2_p3_reg(VOID)
{
	//T_IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER2 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER2_OFS);

	return imeg->reg_251.word;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_lineoffset_c2_p3_reg(VOID)
{
	//T_IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER3_OFS);

	return imeg->reg_252.word;
}
//-------------------------------------------------------------------------------


VOID ime_set_stitching_output_channel_addr2_p3_reg(UINT32 addr_y, UINT32 addr_cb, UINT32 addr_cr)
{
	//T_IME_OUTPUT_PATH3_DMA_BUFFER2_REGISTER3 arg0;
	//T_IME_OUTPUT_PATH3_DMA_BUFFER1_REGISTER4 arg1;

	ime_p3_out_stitching_y_addr = addr_y;
	ime_p3_out_stitching_u_addr = addr_cb;

	//arg0.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_BUFFER2_REGISTER3_OFS);
	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_BUFFER1_REGISTER4_OFS);

	imeg->reg_253.bit.dram_sao_p3_y1 = ime_platform_va2pa(ime_p3_out_stitching_y_addr);//uioutaddry;
	imeg->reg_254.bit.dram_sao_p3_uv1 = ime_platform_va2pa(ime_p3_out_stitching_u_addr);//uioutaddrcb;

	//IME_SETREG(IME_OUTPUT_PATH3_DMA_BUFFER2_REGISTER3_OFS, arg0.reg);
	//IME_SETREG(IME_OUTPUT_PATH3_DMA_BUFFER1_REGISTER4_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_addr_y2_p3_reg(VOID)
{
	// T_IME_OUTPUT_PATH3_DMA_BUFFER2_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_BUFFER2_REGISTER3_OFS);

	//return arg.reg;

	return ime_p3_out_stitching_y_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_addr_cb2_p3_reg(VOID)
{
	//T_IME_OUTPUT_PATH3_DMA_BUFFER1_REGISTER4 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH3_DMA_BUFFER1_REGISTER4_OFS);

	//return arg.reg;

	return ime_p3_out_stitching_u_addr;
}
//-------------------------------------------------------------------------------


#endif

