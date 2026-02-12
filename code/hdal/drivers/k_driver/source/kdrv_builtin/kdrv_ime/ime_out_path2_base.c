
/*
    IME module driver

    NT96510 IME module driver.

    @file       ime_out_path2_base.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/



#include "ime_reg.h"
#include "ime_int_public.h"


#include "ime_out_path2_base.h"

UINT32 ime_p2_out_y_addr = 0x0;
UINT32 ime_p2_out_u_addr = 0x0;
UINT32 ime_p2_out_stitching_y_addr = 0x0;
UINT32 ime_p2_out_stitching_u_addr = 0x0;

#if (IPP_DRIVER_NEW_REG_HANDLE == DISABLE)

// output path2 APIs
VOID ime_open_output_p2_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 path_en;

	path_en.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	path_en.bit.ime_p2_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, path_en.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_flip_enable_p2_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_p2_flip_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_output_dram_enable_p2_reg(UINT32 set_en)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	arg.bit.ime_out_en_p2 = set_en;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_image_format_p2_reg(UINT32 fmt_sel)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	arg.bit.ime_omat2 = fmt_sel;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_scale_interpolation_method_p2_reg(UINT32 set_scl_method)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	arg.bit.ime_scl_method_p2 = set_scl_method;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_type_p2_reg(UINT32 set_out_fmt_type)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	arg.bit.ime_out_type_p2 = 1;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scale_factor_h_p2_reg(UINT32 h_scale_sel, UINT32 h_scale_rate, UINT32 h_scale_factor)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER1 arg1;
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER2 arg2;

	arg1.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS);
	arg2.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER2_OFS);

	arg1.bit.h2_ud     = h_scale_sel;
	arg1.bit.h2_dnrate = h_scale_rate;
	arg2.bit.h2_sfact  = h_scale_factor;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS, arg1.reg);
	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER2_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scale_factor_v_p2_reg(UINT32 v_scale_sel, UINT32 v_scale_rate, UINT32 v_scale_factor)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER1 arg1;
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER2 arg2;

	arg1.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS);
	arg2.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER2_OFS);

	arg1.bit.v2_ud     = v_scale_sel;
	arg1.bit.v2_dnrate = v_scale_rate;
	arg2.bit.v2_sfact  = v_scale_factor;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS, arg1.reg);
	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER2_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scaling_size_p2_reg(UINT32 h_scale_size, UINT32 v_scale_size)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER22 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER22_OFS);

	arg.bit.h2_scl_size = h_scale_size;
	arg.bit.v2_scl_size = v_scale_size;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER22_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_scaling_size_p2_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER22 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER22_OFS);

	*p_get_size_h = arg.bit.h2_scl_size;
	*p_get_size_v = arg.bit.v2_scl_size;
}
//-------------------------------------------------------------------------------


VOID ime_set_horizontal_scale_filtering_p2_reg(UINT32 set_en, UINT32 set_coef)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS);

#if 0
	arg.bit.h2_filtmode = 0;
	arg.bit.h2_filtcoef = 0;
#else
	arg.bit.h2_filtmode = set_en;
	arg.bit.h2_filtcoef = set_coef;
#endif

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_vertical_scale_filtering_p2_reg(UINT32 set_en, UINT32 set_coef)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS);

#if 0
	arg.bit.v2_filtmode = 0;
	arg.bit.v2_filtcoef = 0;
#else
	arg.bit.v2_filtmode = set_en;
	arg.bit.v2_filtcoef = set_coef;
#endif

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor_base_p2_reg(UINT32 set_base_h_factor, UINT32 set_base_v_factor)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER3_OFS);

	arg.bit.isd_h_base_p2 = set_base_h_factor;
	arg.bit.isd_v_base_p2 = set_base_v_factor;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_integration_scaling_factor0_p2_reg(UINT32 set_h_factor, UINT32 set_v_factor)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER4 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER4_OFS);

	arg.bit.isd_h_sfact0_p2 = set_h_factor;
	arg.bit.isd_v_sfact0_p2 = set_v_factor;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER4_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor1_p2_reg(UINT32 set_h_factor, UINT32 set_v_factor)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER5 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER5_OFS);

	arg.bit.isd_h_sfact1_p2 = set_h_factor;
	arg.bit.isd_v_sfact1_p2 = set_v_factor;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER5_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor2_p2_reg(UINT32 set_h_factor, UINT32 set_v_factor)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER6 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER6_OFS);

	arg.bit.isd_h_sfact2_p2 = set_h_factor;
	arg.bit.isd_v_sfact2_p2 = set_v_factor;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER6_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_ctrl_p2_reg(UINT32 isd_mode, UINT32 isd_h_coef_num, UINT32 isd_v_coef_num)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER7 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER7_OFS);

	arg.bit.isd_p2_mode = isd_mode;
	arg.bit.isd_p2_h_coef_num = isd_h_coef_num - 1;
	arg.bit.isd_p2_v_coef_num = isd_v_coef_num - 1;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER7_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_h_coefs_p2_reg(INT32 *hcoefs)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER8 arg0;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER9 arg1;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER10 arg2;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER11 arg3;
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER17 arg4;

	T_IME_OUTPUT_PATH2_CONTROL_REGISTER18 arg5;
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER19 arg6;


	//arg0.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER8_OFS);
	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER9_OFS);
	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER10_OFS);
	//arg3.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER11_OFS);
	//arg4.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS);

	arg5.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER18_OFS);
	arg6.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER19_OFS);

	for (i = 0; i < 17; i++) {
		get_coef_val[i] = ime_int_to_2comp(hcoefs[i], 12);
	}

#if 0
	arg0.bit.isd_p2_h_coef0 = get_coef_val[0] & 0x000000ff;
	arg0.bit.isd_p2_h_coef1 = get_coef_val[1] & 0x000000ff;
	arg0.bit.isd_p2_h_coef2 = get_coef_val[2] & 0x000000ff;
	arg0.bit.isd_p2_h_coef3 = get_coef_val[3] & 0x000000ff;

	arg1.bit.isd_p2_h_coef4 = get_coef_val[4] & 0x000000ff;
	arg1.bit.isd_p2_h_coef5 = get_coef_val[5] & 0x000000ff;
	arg1.bit.isd_p2_h_coef6 = get_coef_val[6] & 0x000000ff;
	arg1.bit.isd_p2_h_coef7 = get_coef_val[7] & 0x000000ff;

	arg2.bit.isd_p2_h_coef8 = get_coef_val[8] & 0x000000ff;
	arg2.bit.isd_p2_h_coef9 = get_coef_val[9] & 0x000000ff;
	arg2.bit.isd_p2_h_coef10 = get_coef_val[10] & 0x000000ff;
	arg2.bit.isd_p2_h_coef11 = get_coef_val[11] & 0x000000ff;

	arg3.bit.isd_p2_h_coef12 = get_coef_val[12] & 0x000000ff;
	arg3.bit.isd_p2_h_coef13 = get_coef_val[13] & 0x000000ff;
	arg3.bit.isd_p2_h_coef14 = get_coef_val[14] & 0x000000ff;
	arg3.bit.isd_p2_h_coef15 = get_coef_val[15] & 0x000000ff;
#endif

	for (i = 0; i < 4; i++) {
		reg_set_val = 0;

		idx = (i << 2);
		reg_ofs = IME_OUTPUT_PATH2_CONTROL_REGISTER8_OFS + (i << 2);
		reg_set_val = (get_coef_val[idx] & 0xFF) + ((get_coef_val[idx + 1] & 0xFF) << 8) + ((get_coef_val[idx + 2] & 0xFF) << 16) + ((get_coef_val[idx + 3] & 0xFF) << 24);

		IME_SETREG(reg_ofs, reg_set_val);
	}

	arg4.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS);
	arg4.bit.isd_p2_h_coef16 = get_coef_val[16];
	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS, arg4.reg);


	arg5.bit.isd_p2_h_coef_msb0 = (get_coef_val[0] >> 8) & 0x0000000f;
	arg5.bit.isd_p2_h_coef_msb1 = (get_coef_val[1] >> 8) & 0x0000000f;
	arg5.bit.isd_p2_h_coef_msb2 = (get_coef_val[2] >> 8) & 0x0000000f;
	arg5.bit.isd_p2_h_coef_msb3 = (get_coef_val[3] >> 8) & 0x0000000f;
	arg5.bit.isd_p2_h_coef_msb4 = (get_coef_val[4] >> 8) & 0x0000000f;
	arg5.bit.isd_p2_h_coef_msb5 = (get_coef_val[5] >> 8) & 0x0000000f;
	arg5.bit.isd_p2_h_coef_msb6 = (get_coef_val[6] >> 8) & 0x0000000f;
	arg5.bit.isd_p2_h_coef_msb7 = (get_coef_val[7] >> 8) & 0x0000000f;

	arg6.bit.isd_p2_h_coef_msb8 = (get_coef_val[8] >> 8) & 0x0000000f;
	arg6.bit.isd_p2_h_coef_msb9 = (get_coef_val[9] >> 8) & 0x0000000f;
	arg6.bit.isd_p2_h_coef_msb10 = (get_coef_val[10] >> 8) & 0x0000000f;
	arg6.bit.isd_p2_h_coef_msb11 = (get_coef_val[11] >> 8) & 0x0000000f;
	arg6.bit.isd_p2_h_coef_msb12 = (get_coef_val[12] >> 8) & 0x0000000f;
	arg6.bit.isd_p2_h_coef_msb13 = (get_coef_val[13] >> 8) & 0x0000000f;
	arg6.bit.isd_p2_h_coef_msb14 = (get_coef_val[14] >> 8) & 0x0000000f;
	arg6.bit.isd_p2_h_coef_msb15 = (get_coef_val[15] >> 8) & 0x0000000f;


	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER8_OFS, arg0.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER9_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER10_OFS, arg2.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER11_OFS, arg3.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS, arg4.reg);

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER18_OFS, arg5.reg);
	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER19_OFS, arg6.reg);

}
//-------------------------------------------------------------------------------


VOID ime_set_integration_scaling_v_coefs_p2_reg(INT32 *vcoefs)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER13 arg0;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER14 arg1;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER15 arg2;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER16 arg3;
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER17 arg4;

	T_IME_OUTPUT_PATH2_CONTROL_REGISTER20 arg5;
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER21 arg6;

	//arg0.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER13_OFS);
	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER14_OFS);
	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER15_OFS);
	//arg3.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER16_OFS);
	//arg4.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS);

	arg5.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER20_OFS);
	arg6.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER21_OFS);

	for (i = 0; i < 17; i++) {
		get_coef_val[i] = ime_int_to_2comp(vcoefs[i], 12);
	}

#if 0
	arg0.bit.isd_p2_v_coef0 = get_coef_val[0] & 0x000000ff;
	arg0.bit.isd_p2_v_coef1 = get_coef_val[1] & 0x000000ff;
	arg0.bit.isd_p2_v_coef2 = get_coef_val[2] & 0x000000ff;
	arg0.bit.isd_p2_v_coef3 = get_coef_val[3] & 0x000000ff;

	arg1.bit.isd_p2_v_coef4 = get_coef_val[4] & 0x000000ff;
	arg1.bit.isd_p2_v_coef5 = get_coef_val[5] & 0x000000ff;
	arg1.bit.isd_p2_v_coef6 = get_coef_val[6] & 0x000000ff;
	arg1.bit.isd_p2_v_coef7 = get_coef_val[7] & 0x000000ff;

	arg2.bit.isd_p2_v_coef8 = get_coef_val[8] & 0x000000ff;
	arg2.bit.isd_p2_v_coef9 = get_coef_val[9] & 0x000000ff;
	arg2.bit.isd_p2_v_coef10 = get_coef_val[10] & 0x000000ff;
	arg2.bit.isd_p2_v_coef11 = get_coef_val[11] & 0x000000ff;

	arg3.bit.isd_p2_v_coef12 = get_coef_val[12] & 0x000000ff;
	arg3.bit.isd_p2_v_coef13 = get_coef_val[13] & 0x000000ff;
	arg3.bit.isd_p2_v_coef14 = get_coef_val[14] & 0x000000ff;
	arg3.bit.isd_p2_v_coef15 = get_coef_val[15] & 0x000000ff;
#endif

	for (i = 0; i < 4; i++) {
		reg_set_val = 0;

		idx = (i << 2);
		reg_ofs = IME_OUTPUT_PATH2_CONTROL_REGISTER13_OFS + (i << 2);
		reg_set_val = (get_coef_val[idx] & 0xFF) + ((get_coef_val[idx + 1] & 0xFF) << 8) + ((get_coef_val[idx + 2] & 0xFF) << 16) + ((get_coef_val[idx + 3] & 0xFF) << 24);

		IME_SETREG(reg_ofs, reg_set_val);
	}

	arg4.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS);
	arg4.bit.isd_p2_v_coef16 = get_coef_val[16] & 0x000000ff;
	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS, arg4.reg);


	arg5.bit.isd_p2_v_coef_msb0 = (get_coef_val[0] >> 8) & 0x0000000f;
	arg5.bit.isd_p2_v_coef_msb1 = (get_coef_val[1] >> 8) & 0x0000000f;
	arg5.bit.isd_p2_v_coef_msb2 = (get_coef_val[2] >> 8) & 0x0000000f;
	arg5.bit.isd_p2_v_coef_msb3 = (get_coef_val[3] >> 8) & 0x0000000f;
	arg5.bit.isd_p2_v_coef_msb4 = (get_coef_val[4] >> 8) & 0x0000000f;
	arg5.bit.isd_p2_v_coef_msb5 = (get_coef_val[5] >> 8) & 0x0000000f;
	arg5.bit.isd_p2_v_coef_msb6 = (get_coef_val[6] >> 8) & 0x0000000f;
	arg5.bit.isd_p2_v_coef_msb7 = (get_coef_val[7] >> 8) & 0x0000000f;

	arg6.bit.isd_p2_v_coef_msb8 = (get_coef_val[8] >> 8) & 0x0000000f;
	arg6.bit.isd_p2_v_coef_msb9 = (get_coef_val[9] >> 8) & 0x0000000f;
	arg6.bit.isd_p2_v_coef_msb10 = (get_coef_val[10] >> 8) & 0x0000000f;
	arg6.bit.isd_p2_v_coef_msb11 = (get_coef_val[11] >> 8) & 0x0000000f;
	arg6.bit.isd_p2_v_coef_msb12 = (get_coef_val[12] >> 8) & 0x0000000f;
	arg6.bit.isd_p2_v_coef_msb13 = (get_coef_val[13] >> 8) & 0x0000000f;
	arg6.bit.isd_p2_v_coef_msb14 = (get_coef_val[14] >> 8) & 0x0000000f;
	arg6.bit.isd_p2_v_coef_msb15 = (get_coef_val[15] >> 8) & 0x0000000f;


	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER13_OFS, arg0.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER14_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER15_OFS, arg2.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER16_OFS, arg3.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS, arg4.reg);

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER20_OFS, arg5.reg);
	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER21_OFS, arg6.reg);

}
//-------------------------------------------------------------------------------


VOID ime_set_integration_scaling_h_coefs_sum_p2_reg(INT32 coef_sum_all, INT32 coef_sum_half)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER26 arg1;
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER27 arg2;

	arg1.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER26_OFS);
	arg2.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER27_OFS);

	arg1.bit.ime_p2_isd_h_coef_all = ime_int_to_2comp(coef_sum_all, 17);
	arg2.bit.ime_p2_isd_h_coef_half = ime_int_to_2comp(coef_sum_half, 17);

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER26_OFS, arg1.reg);
	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER27_OFS, arg2.reg);
}

//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_v_coefs_sum_p2_reg(INT32 coef_sum_all, INT32 coef_sum_half)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER28 arg1;
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER29 arg2;

	arg1.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER28_OFS);
	arg2.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER29_OFS);

	arg1.bit.ime_p2_isd_v_coef_all = ime_int_to_2comp(coef_sum_all, 17);
	arg2.bit.ime_p2_isd_v_coef_half = ime_int_to_2comp(coef_sum_half, 17);

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER28_OFS, arg1.reg);
	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER29_OFS, arg2.reg);
}

//-------------------------------------------------------------------------------



VOID ime_set_scaling_enhance_factor_p2_reg(UINT32 scl_enh_ftr, UINT32 scl_enh_sht)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	arg.bit.ime_scl_enh_fact_p2 = scl_enh_ftr;
	arg.bit.ime_scl_enh_bit_p2 = scl_enh_sht;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_output_crop_coordinate_p2_reg(UINT32 crop_pos_x, UINT32 crop_pos_y)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER23 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER23_OFS);

	arg.bit.ime_cropout_x_p2 = crop_pos_x;
	arg.bit.ime_cropout_y_p2 = crop_pos_y;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER23_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_output_crop_coordinate_p2_reg(UINT32 *p_get_crop_pos_x, UINT32 *p_get_crop_pos_y)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER23 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER23_OFS);

	*p_get_crop_pos_x = arg.bit.ime_cropout_x_p2;
	*p_get_crop_pos_y = arg.bit.ime_cropout_y_p2;
}
//-------------------------------------------------------------------------------



VOID ime_set_output_lineoffset_y_p2_reg(UINT32 lofs)
{
	T_IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER0_OFS);

	arg.bit.dram_ofso_p2_y0 = (lofs >> 2);

	IME_SETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_lineoffset_c_p2_reg(UINT32 lofs)
{
	T_IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER1_OFS);

	arg.bit.dram_ofso_p2_uv0 = (lofs >> 2);

	IME_SETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_channel_addr_p2_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr)
{
	T_IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER0 arg1;
	T_IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER1 arg2;

	ime_p2_out_y_addr = out_addr_y;
	ime_p2_out_u_addr = out_addr_cb;

	arg1.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER0_OFS);
	arg2.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER1_OFS);

	arg1.bit.dram_sao_p2_y0 = ime_platform_va2pa(ime_p2_out_y_addr);
	arg2.bit.dram_sao_p2_uv0 = ime_platform_va2pa(ime_p2_out_u_addr);

	IME_SETREG(IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER0_OFS, arg1.reg);
	IME_SETREG(IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER1_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_size_p2_reg(UINT32 set_size_h, UINT32 set_size_v)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER24 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER24_OFS);

	arg.bit.h2_osize = set_size_h;
	arg.bit.v2_osize = set_size_v;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER24_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_get_output_size_p2_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER24 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER24_OFS);

	*p_get_size_h = arg.bit.h2_osize;
	*p_get_size_v = arg.bit.v2_osize;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_output_lineoffset_y_p2_reg(VOID)
{
	T_IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER0_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_output_lineoffset_c_p2_reg(VOID)
{
	T_IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER1_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_output_path_enable_status_p2_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_p2_en;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_format_p2_reg(VOID)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_omat2;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_format_type_p2_reg(VOID)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_out_type_p2;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_scaling_method_p2_reg(VOID)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_scl_method_p2;
}
//-------------------------------------------------------------------------------


// output path2
UINT32 ime_get_output_addr_y_p2_reg(VOID)
{
	//T_IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER0 Path2Info;

	//Path2Info.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER0_OFS);

	//return (Path2Info.reg);

	return ime_p2_out_y_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_addr_cb_p2_reg(VOID)
{
	//T_IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER1 Path2Info;

	//Path2Info.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER1_OFS);

	//return (Path2Info.reg);

	return ime_p2_out_u_addr;
}
//-------------------------------------------------------------------------------

VOID ime_clamp_p2_reg(UINT32 min_y, UINT32 max_y, UINT32 min_uv, UINT32 max_uv)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER25 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER25_OFS);

	arg.bit.ime_y_clamp_min_p2  = min_y;
	arg.bit.ime_y_clamp_max_p2  = max_y;
	arg.bit.ime_uv_clamp_min_p2 = min_uv;
	arg.bit.ime_uv_clamp_max_p2 = max_uv;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER25_OFS, arg.reg);
}

//-------------------------------------------------------------------------------
// stitching
// stitching - path2
VOID ime_set_stitching_enable_p2_reg(UINT32 set_en)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	arg.bit.ime_sprt_out_en_p2 = set_en;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_stitching_base_position_p2_reg(UINT32 base_pos)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER30 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER30_OFS);

	arg.bit.ime_sprt_hbl_p2 = base_pos;

	IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER30_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_enable_p2_reg(VOID)
{
	T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_sprt_out_en_p2;
}
//-------------------------------------------------------------------------------

VOID ime_set_stitching_output_lineoffset_y2_p2_reg(UINT32 lofs)
{
	T_IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER2 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER2_OFS);

	arg.bit.dram_ofso_p2_y1 = (lofs >> 2);

	IME_SETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER2_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_stitching_output_lineoffset_c2_p2_reg(UINT32 lofs)
{
	T_IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER3_OFS);

	arg.bit.dram_ofso_p2_uv1 = (lofs >> 2);

	IME_SETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_lineoffset_y2_p2_reg(VOID)
{
	T_IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER2 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER2_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_lineoffset_c2_p2_reg(VOID)
{
	T_IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER3_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------



VOID ime_set_stitching_output_channel_addr2_p2_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr)
{
	T_IME_OUTPUT_PATH2_DMA_BUFFER2_REGISTER3 arg0;
	T_IME_OUTPUT_PATH2_DMA_BUFFER1_REGISTER4 arg1;

	ime_p2_out_stitching_y_addr = out_addr_y;
	ime_p2_out_stitching_u_addr = out_addr_cb;

	arg0.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_BUFFER2_REGISTER3_OFS);
	arg1.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_BUFFER1_REGISTER4_OFS);

	arg0.bit.dram_sao_p2_y1 = ime_platform_va2pa(ime_p2_out_stitching_y_addr);//uioutaddry;
	arg1.bit.dram_sao_p2_uv1 = ime_platform_va2pa(ime_p2_out_stitching_u_addr);//uioutaddrcb;

	IME_SETREG(IME_OUTPUT_PATH2_DMA_BUFFER2_REGISTER3_OFS, arg0.reg);
	IME_SETREG(IME_OUTPUT_PATH2_DMA_BUFFER1_REGISTER4_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_addr_y2_p2_reg(VOID)
{
	//T_IME_OUTPUT_PATH2_DMA_BUFFER2_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_BUFFER2_REGISTER3_OFS);

	//return arg.reg;

	return ime_p2_out_stitching_y_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_addr_cb2_p2_reg(VOID)
{
	//T_IME_OUTPUT_PATH2_DMA_BUFFER1_REGISTER4 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_BUFFER1_REGISTER4_OFS);

	//return arg.reg;

	return ime_p2_out_stitching_u_addr;
}
//-------------------------------------------------------------------------------

#else

// output path2 APIs
VOID ime_open_output_p2_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 path_en;

	//path_en.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_p2_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, path_en.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_flip_enable_p2_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_p2_flip_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_output_dram_enable_p2_reg(UINT32 set_en)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	imeg->reg_44.bit.ime_out_en_p2 = set_en;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_image_format_p2_reg(UINT32 fmt_sel)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	imeg->reg_44.bit.ime_omat2 = fmt_sel;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_scale_interpolation_method_p2_reg(UINT32 set_scl_method)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	imeg->reg_44.bit.ime_scl_method_p2 = set_scl_method;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_type_p2_reg(UINT32 set_out_fmt_type)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	imeg->reg_44.bit.ime_out_type_p2 = 1;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scale_factor_h_p2_reg(UINT32 h_scale_sel, UINT32 h_scale_rate, UINT32 h_scale_factor)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER1 arg1;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER2 arg2;

	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS);
	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER2_OFS);

	imeg->reg_45.bit.h2_ud     = h_scale_sel;
	imeg->reg_45.bit.h2_dnrate = h_scale_rate;
	imeg->reg_46.bit.h2_sfact  = h_scale_factor;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER2_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scale_factor_v_p2_reg(UINT32 v_scale_sel, UINT32 v_scale_rate, UINT32 v_scale_factor)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER1 arg1;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER2 arg2;

	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS);
	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER2_OFS);

	imeg->reg_45.bit.v2_ud     = v_scale_sel;
	imeg->reg_45.bit.v2_dnrate = v_scale_rate;
	imeg->reg_46.bit.v2_sfact  = v_scale_factor;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER2_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scaling_size_p2_reg(UINT32 h_scale_size, UINT32 v_scale_size)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER22 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER22_OFS);

	imeg->reg_65.bit.h2_scl_size = h_scale_size;
	imeg->reg_65.bit.v2_scl_size = v_scale_size;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER22_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_scaling_size_p2_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER22 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER22_OFS);

	*p_get_size_h = imeg->reg_65.bit.h2_scl_size;
	*p_get_size_v = imeg->reg_65.bit.v2_scl_size;
}
//-------------------------------------------------------------------------------


VOID ime_set_horizontal_scale_filtering_p2_reg(UINT32 set_en, UINT32 set_coef)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS);

#if 0
	imeg->reg_45.bit.h2_filtmode = 0;
	imeg->reg_45.bit.h2_filtcoef = 0;
#else
	imeg->reg_45.bit.h2_filtmode = set_en;
	imeg->reg_45.bit.h2_filtcoef = set_coef;
#endif

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_vertical_scale_filtering_p2_reg(UINT32 set_en, UINT32 set_coef)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS);

#if 0
	imeg->reg_45.bit.v2_filtmode = 0;
	imeg->reg_45.bit.v2_filtcoef = 0;
#else
	imeg->reg_45.bit.v2_filtmode = set_en;
	imeg->reg_45.bit.v2_filtcoef = set_coef;
#endif

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor_base_p2_reg(UINT32 set_base_h_factor, UINT32 set_base_v_factor)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER3_OFS);

	imeg->reg_47.bit.isd_h_base_p2 = set_base_h_factor;
	imeg->reg_47.bit.isd_v_base_p2 = set_base_v_factor;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_integration_scaling_factor0_p2_reg(UINT32 set_h_factor, UINT32 set_v_factor)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER4 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER4_OFS);

	imeg->reg_48.bit.isd_h_sfact0_p2 = set_h_factor;
	imeg->reg_48.bit.isd_v_sfact0_p2 = set_v_factor;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER4_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor1_p2_reg(UINT32 set_h_factor, UINT32 set_v_factor)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER5 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER5_OFS);

	imeg->reg_49.bit.isd_h_sfact1_p2 = set_h_factor;
	imeg->reg_49.bit.isd_v_sfact1_p2 = set_v_factor;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER5_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor2_p2_reg(UINT32 set_h_factor, UINT32 set_v_factor)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER6 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER6_OFS);

	imeg->reg_50.bit.isd_h_sfact2_p2 = set_h_factor;
	imeg->reg_50.bit.isd_v_sfact2_p2 = set_v_factor;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER6_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_ctrl_p2_reg(UINT32 isd_mode, UINT32 isd_h_coef_num, UINT32 isd_v_coef_num)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER7 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER7_OFS);

	imeg->reg_51.bit.isd_p2_mode = isd_mode;
	imeg->reg_51.bit.isd_p2_h_coef_num = isd_h_coef_num - 1;
	imeg->reg_51.bit.isd_p2_v_coef_num = isd_v_coef_num - 1;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER7_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_h_coefs_p2_reg(INT32 *hcoefs)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER8 arg0;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER9 arg1;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER10 arg2;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER11 arg3;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER17 arg4;

	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER18 arg5;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER19 arg6;


	//arg0.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER8_OFS);
	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER9_OFS);
	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER10_OFS);
	//arg3.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER11_OFS);
	//arg4.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS);

	//arg5.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER18_OFS);
	//arg6.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER19_OFS);

	for (i = 0; i < 17; i++) {
		get_coef_val[i] = ime_int_to_2comp(hcoefs[i], 12);
	}

#if 0
	arg0.bit.isd_p2_h_coef0 = get_coef_val[0] & 0x000000ff;
	arg0.bit.isd_p2_h_coef1 = get_coef_val[1] & 0x000000ff;
	arg0.bit.isd_p2_h_coef2 = get_coef_val[2] & 0x000000ff;
	arg0.bit.isd_p2_h_coef3 = get_coef_val[3] & 0x000000ff;

	arg1.bit.isd_p2_h_coef4 = get_coef_val[4] & 0x000000ff;
	arg1.bit.isd_p2_h_coef5 = get_coef_val[5] & 0x000000ff;
	arg1.bit.isd_p2_h_coef6 = get_coef_val[6] & 0x000000ff;
	arg1.bit.isd_p2_h_coef7 = get_coef_val[7] & 0x000000ff;

	arg2.bit.isd_p2_h_coef8 = get_coef_val[8] & 0x000000ff;
	arg2.bit.isd_p2_h_coef9 = get_coef_val[9] & 0x000000ff;
	arg2.bit.isd_p2_h_coef10 = get_coef_val[10] & 0x000000ff;
	arg2.bit.isd_p2_h_coef11 = get_coef_val[11] & 0x000000ff;

	arg3.bit.isd_p2_h_coef12 = get_coef_val[12] & 0x000000ff;
	arg3.bit.isd_p2_h_coef13 = get_coef_val[13] & 0x000000ff;
	arg3.bit.isd_p2_h_coef14 = get_coef_val[14] & 0x000000ff;
	arg3.bit.isd_p2_h_coef15 = get_coef_val[15] & 0x000000ff;
#endif

	for (i = 0; i < 4; i++) {
		reg_set_val = 0;

		idx = (i << 2);
		reg_ofs = IME_OUTPUT_PATH2_CONTROL_REGISTER8_OFS + (i << 2);
		reg_set_val = (get_coef_val[idx] & 0x000000FF) + ((get_coef_val[idx + 1] & 0x000000FF) << 8) + ((get_coef_val[idx + 2] & 0x000000FF) << 16) + ((get_coef_val[idx + 3] & 0x000000FF) << 24);

		IME_SETREG(reg_ofs, reg_set_val);
	}

	//arg4.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS);
	imeg->reg_60.bit.isd_p2_h_coef16 = (get_coef_val[16] & 0x000000FF);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS, arg4.reg);


	imeg->reg_61.bit.isd_p2_h_coef_msb0 = (get_coef_val[0] >> 8) & 0x0000000f;
	imeg->reg_61.bit.isd_p2_h_coef_msb1 = (get_coef_val[1] >> 8) & 0x0000000f;
	imeg->reg_61.bit.isd_p2_h_coef_msb2 = (get_coef_val[2] >> 8) & 0x0000000f;
	imeg->reg_61.bit.isd_p2_h_coef_msb3 = (get_coef_val[3] >> 8) & 0x0000000f;
	imeg->reg_61.bit.isd_p2_h_coef_msb4 = (get_coef_val[4] >> 8) & 0x0000000f;
	imeg->reg_61.bit.isd_p2_h_coef_msb5 = (get_coef_val[5] >> 8) & 0x0000000f;
	imeg->reg_61.bit.isd_p2_h_coef_msb6 = (get_coef_val[6] >> 8) & 0x0000000f;
	imeg->reg_61.bit.isd_p2_h_coef_msb7 = (get_coef_val[7] >> 8) & 0x0000000f;

	imeg->reg_62.bit.isd_p2_h_coef_msb8 = (get_coef_val[8] >> 8) & 0x0000000f;
	imeg->reg_62.bit.isd_p2_h_coef_msb9 = (get_coef_val[9] >> 8) & 0x0000000f;
	imeg->reg_62.bit.isd_p2_h_coef_msb10 = (get_coef_val[10] >> 8) & 0x0000000f;
	imeg->reg_62.bit.isd_p2_h_coef_msb11 = (get_coef_val[11] >> 8) & 0x0000000f;
	imeg->reg_62.bit.isd_p2_h_coef_msb12 = (get_coef_val[12] >> 8) & 0x0000000f;
	imeg->reg_62.bit.isd_p2_h_coef_msb13 = (get_coef_val[13] >> 8) & 0x0000000f;
	imeg->reg_62.bit.isd_p2_h_coef_msb14 = (get_coef_val[14] >> 8) & 0x0000000f;
	imeg->reg_62.bit.isd_p2_h_coef_msb15 = (get_coef_val[15] >> 8) & 0x0000000f;


	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER8_OFS, arg0.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER9_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER10_OFS, arg2.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER11_OFS, arg3.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS, arg4.reg);

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER18_OFS, arg5.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER19_OFS, arg6.reg);

}
//-------------------------------------------------------------------------------


VOID ime_set_integration_scaling_v_coefs_p2_reg(INT32 *vcoefs)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER13 arg0;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER14 arg1;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER15 arg2;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER16 arg3;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER17 arg4;

	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER20 arg5;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER21 arg6;

	//arg0.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER13_OFS);
	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER14_OFS);
	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER15_OFS);
	//arg3.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER16_OFS);
	//arg4.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS);

	//arg5.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER20_OFS);
	//arg6.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER21_OFS);

	for (i = 0; i < 17; i++) {
		get_coef_val[i] = ime_int_to_2comp(vcoefs[i], 12);
	}

#if 0
	arg0.bit.isd_p2_v_coef0 = get_coef_val[0] & 0x000000ff;
	arg0.bit.isd_p2_v_coef1 = get_coef_val[1] & 0x000000ff;
	arg0.bit.isd_p2_v_coef2 = get_coef_val[2] & 0x000000ff;
	arg0.bit.isd_p2_v_coef3 = get_coef_val[3] & 0x000000ff;

	arg1.bit.isd_p2_v_coef4 = get_coef_val[4] & 0x000000ff;
	arg1.bit.isd_p2_v_coef5 = get_coef_val[5] & 0x000000ff;
	arg1.bit.isd_p2_v_coef6 = get_coef_val[6] & 0x000000ff;
	arg1.bit.isd_p2_v_coef7 = get_coef_val[7] & 0x000000ff;

	arg2.bit.isd_p2_v_coef8 = get_coef_val[8] & 0x000000ff;
	arg2.bit.isd_p2_v_coef9 = get_coef_val[9] & 0x000000ff;
	arg2.bit.isd_p2_v_coef10 = get_coef_val[10] & 0x000000ff;
	arg2.bit.isd_p2_v_coef11 = get_coef_val[11] & 0x000000ff;

	arg3.bit.isd_p2_v_coef12 = get_coef_val[12] & 0x000000ff;
	arg3.bit.isd_p2_v_coef13 = get_coef_val[13] & 0x000000ff;
	arg3.bit.isd_p2_v_coef14 = get_coef_val[14] & 0x000000ff;
	arg3.bit.isd_p2_v_coef15 = get_coef_val[15] & 0x000000ff;
#endif

	for (i = 0; i < 4; i++) {
		reg_set_val = 0;

		idx = (i << 2);
		reg_ofs = IME_OUTPUT_PATH2_CONTROL_REGISTER13_OFS + (i << 2);
		reg_set_val = (get_coef_val[idx] & 0x000000FF) + ((get_coef_val[idx + 1] & 0x000000FF) << 8) + ((get_coef_val[idx + 2] & 0x000000FF) << 16) + ((get_coef_val[idx + 3] & 0x000000FF) << 24);

		IME_SETREG(reg_ofs, reg_set_val);
	}

	//arg4.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS);
	imeg->reg_60.bit.isd_p2_v_coef16 = (get_coef_val[16] & 0x000000ff);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS, arg4.reg);


	imeg->reg_63.bit.isd_p2_v_coef_msb0 = (get_coef_val[0] >> 8) & 0x0000000f;
	imeg->reg_63.bit.isd_p2_v_coef_msb1 = (get_coef_val[1] >> 8) & 0x0000000f;
	imeg->reg_63.bit.isd_p2_v_coef_msb2 = (get_coef_val[2] >> 8) & 0x0000000f;
	imeg->reg_63.bit.isd_p2_v_coef_msb3 = (get_coef_val[3] >> 8) & 0x0000000f;
	imeg->reg_63.bit.isd_p2_v_coef_msb4 = (get_coef_val[4] >> 8) & 0x0000000f;
	imeg->reg_63.bit.isd_p2_v_coef_msb5 = (get_coef_val[5] >> 8) & 0x0000000f;
	imeg->reg_63.bit.isd_p2_v_coef_msb6 = (get_coef_val[6] >> 8) & 0x0000000f;
	imeg->reg_63.bit.isd_p2_v_coef_msb7 = (get_coef_val[7] >> 8) & 0x0000000f;

	imeg->reg_64.bit.isd_p2_v_coef_msb8 = (get_coef_val[8] >> 8) & 0x0000000f;
	imeg->reg_64.bit.isd_p2_v_coef_msb9 = (get_coef_val[9] >> 8) & 0x0000000f;
	imeg->reg_64.bit.isd_p2_v_coef_msb10 = (get_coef_val[10] >> 8) & 0x0000000f;
	imeg->reg_64.bit.isd_p2_v_coef_msb11 = (get_coef_val[11] >> 8) & 0x0000000f;
	imeg->reg_64.bit.isd_p2_v_coef_msb12 = (get_coef_val[12] >> 8) & 0x0000000f;
	imeg->reg_64.bit.isd_p2_v_coef_msb13 = (get_coef_val[13] >> 8) & 0x0000000f;
	imeg->reg_64.bit.isd_p2_v_coef_msb14 = (get_coef_val[14] >> 8) & 0x0000000f;
	imeg->reg_64.bit.isd_p2_v_coef_msb15 = (get_coef_val[15] >> 8) & 0x0000000f;


	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER13_OFS, arg0.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER14_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER15_OFS, arg2.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER16_OFS, arg3.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS, arg4.reg);

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER20_OFS, arg5.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER21_OFS, arg6.reg);

}
//-------------------------------------------------------------------------------


VOID ime_set_integration_scaling_h_coefs_sum_p2_reg(INT32 coef_sum_all, INT32 coef_sum_half)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER26 arg1;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER27 arg2;

	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER26_OFS);
	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER27_OFS);

	imeg->reg_120.bit.ime_p2_isd_h_coef_all = ime_int_to_2comp(coef_sum_all, 17);
	imeg->reg_121.bit.ime_p2_isd_h_coef_half = ime_int_to_2comp(coef_sum_half, 17);

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER26_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER27_OFS, arg2.reg);
}

//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_v_coefs_sum_p2_reg(INT32 coef_sum_all, INT32 coef_sum_half)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER28 arg1;
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER29 arg2;

	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER28_OFS);
	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER29_OFS);

	imeg->reg_122.bit.ime_p2_isd_v_coef_all = ime_int_to_2comp(coef_sum_all, 17);
	imeg->reg_123.bit.ime_p2_isd_v_coef_half = ime_int_to_2comp(coef_sum_half, 17);

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER28_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER29_OFS, arg2.reg);
}

//-------------------------------------------------------------------------------



VOID ime_set_scaling_enhance_factor_p2_reg(UINT32 scl_enh_ftr, UINT32 scl_enh_sht)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	imeg->reg_44.bit.ime_scl_enh_fact_p2 = scl_enh_ftr;
	imeg->reg_44.bit.ime_scl_enh_bit_p2 = scl_enh_sht;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_output_crop_coordinate_p2_reg(UINT32 crop_pos_x, UINT32 crop_pos_y)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER23 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER23_OFS);

	imeg->reg_66.bit.ime_cropout_x_p2 = crop_pos_x;
	imeg->reg_66.bit.ime_cropout_y_p2 = crop_pos_y;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER23_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_output_crop_coordinate_p2_reg(UINT32 *p_get_crop_pos_x, UINT32 *p_get_crop_pos_y)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER23 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER23_OFS);

	*p_get_crop_pos_x = imeg->reg_66.bit.ime_cropout_x_p2;
	*p_get_crop_pos_y = imeg->reg_66.bit.ime_cropout_y_p2;
}
//-------------------------------------------------------------------------------



VOID ime_set_output_lineoffset_y_p2_reg(UINT32 lofs)
{
	//T_IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER0_OFS);

	imeg->reg_69.bit.dram_ofso_p2_y0 = (lofs >> 2);

	//IME_SETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_lineoffset_c_p2_reg(UINT32 lofs)
{
	//T_IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER1_OFS);

	imeg->reg_70.bit.dram_ofso_p2_uv0 = (lofs >> 2);

	//IME_SETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_channel_addr_p2_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr)
{
	//T_IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER0 arg1;
	//T_IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER1 arg2;

	ime_p2_out_y_addr = out_addr_y;
	ime_p2_out_u_addr = out_addr_cb;

	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER0_OFS);
	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER1_OFS);

	imeg->reg_71.bit.dram_sao_p2_y0 = ime_platform_va2pa(ime_p2_out_y_addr);
	imeg->reg_72.bit.dram_sao_p2_uv0 = ime_platform_va2pa(ime_p2_out_u_addr);

	//IME_SETREG(IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER0_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER1_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_size_p2_reg(UINT32 set_size_h, UINT32 set_size_v)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER24 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER24_OFS);

	imeg->reg_67.bit.h2_osize = set_size_h;
	imeg->reg_67.bit.v2_osize = set_size_v;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER24_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_get_output_size_p2_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER24 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER24_OFS);

	*p_get_size_h = imeg->reg_67.bit.h2_osize;
	*p_get_size_v = imeg->reg_67.bit.v2_osize;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_output_lineoffset_y_p2_reg(VOID)
{
	//T_IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER0_OFS);

	return imeg->reg_69.word;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_output_lineoffset_c_p2_reg(VOID)
{
	//T_IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER1_OFS);

	return imeg->reg_70.word;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_output_path_enable_status_p2_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return imeg->reg_1.bit.ime_p2_en;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_format_p2_reg(VOID)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	return imeg->reg_44.bit.ime_omat2;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_format_type_p2_reg(VOID)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	return imeg->reg_44.bit.ime_out_type_p2;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_scaling_method_p2_reg(VOID)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	return imeg->reg_44.bit.ime_scl_method_p2;
}
//-------------------------------------------------------------------------------


// output path2
UINT32 ime_get_output_addr_y_p2_reg(VOID)
{
	//T_IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER0 Path2Info;

	//Path2Info.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER0_OFS);

	//return (Path2Info.reg);

	return ime_p2_out_y_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_addr_cb_p2_reg(VOID)
{
	//T_IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER1 Path2Info;

	//Path2Info.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER1_OFS);

	//return (Path2Info.reg);

	return ime_p2_out_u_addr;
}
//-------------------------------------------------------------------------------

VOID ime_clamp_p2_reg(UINT32 min_y, UINT32 max_y, UINT32 min_uv, UINT32 max_uv)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER25 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER25_OFS);

	imeg->reg_68.bit.ime_y_clamp_min_p2  = min_y;
	imeg->reg_68.bit.ime_y_clamp_max_p2  = max_y;
	imeg->reg_68.bit.ime_uv_clamp_min_p2 = min_uv;
	imeg->reg_68.bit.ime_uv_clamp_max_p2 = max_uv;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER25_OFS, arg.reg);
}

//-------------------------------------------------------------------------------
// stitching
// stitching - path2
VOID ime_set_stitching_enable_p2_reg(UINT32 set_en)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	imeg->reg_44.bit.ime_sprt_out_en_p2 = set_en;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_stitching_base_position_p2_reg(UINT32 base_pos)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER30 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER30_OFS);

	imeg->reg_245.bit.ime_sprt_hbl_p2 = base_pos;

	//IME_SETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER30_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_enable_p2_reg(VOID)
{
	//T_IME_OUTPUT_PATH2_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS);

	return imeg->reg_44.bit.ime_sprt_out_en_p2;
}
//-------------------------------------------------------------------------------

VOID ime_set_stitching_output_lineoffset_y2_p2_reg(UINT32 lofs)
{
	//T_IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER2 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER2_OFS);

	imeg->reg_246.bit.dram_ofso_p2_y1 = (lofs >> 2);

	//IME_SETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER2_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_stitching_output_lineoffset_c2_p2_reg(UINT32 lofs)
{
	//T_IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER3_OFS);

	imeg->reg_247.bit.dram_ofso_p2_uv1 = (lofs >> 2);

	//IME_SETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_lineoffset_y2_p2_reg(VOID)
{
	T_IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER2 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER2_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_lineoffset_c2_p2_reg(VOID)
{
	T_IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER3_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------



VOID ime_set_stitching_output_channel_addr2_p2_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr)
{
	//T_IME_OUTPUT_PATH2_DMA_BUFFER2_REGISTER3 arg0;
	//T_IME_OUTPUT_PATH2_DMA_BUFFER1_REGISTER4 arg1;

	ime_p2_out_stitching_y_addr = out_addr_y;
	ime_p2_out_stitching_u_addr = out_addr_cb;

	//arg0.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_BUFFER2_REGISTER3_OFS);
	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_BUFFER1_REGISTER4_OFS);

	imeg->reg_248.bit.dram_sao_p2_y1 = ime_platform_va2pa(ime_p2_out_stitching_y_addr);//uioutaddry;
	imeg->reg_249.bit.dram_sao_p2_uv1 = ime_platform_va2pa(ime_p2_out_stitching_u_addr);//uioutaddrcb;

	//IME_SETREG(IME_OUTPUT_PATH2_DMA_BUFFER2_REGISTER3_OFS, arg0.reg);
	//IME_SETREG(IME_OUTPUT_PATH2_DMA_BUFFER1_REGISTER4_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_addr_y2_p2_reg(VOID)
{
	//T_IME_OUTPUT_PATH2_DMA_BUFFER2_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_BUFFER2_REGISTER3_OFS);

	//return arg.reg;

	return ime_p2_out_stitching_y_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_addr_cb2_p2_reg(VOID)
{
	//T_IME_OUTPUT_PATH2_DMA_BUFFER1_REGISTER4 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH2_DMA_BUFFER1_REGISTER4_OFS);

	//return arg.reg;

	return ime_p2_out_stitching_u_addr;
}
//-------------------------------------------------------------------------------



#endif

