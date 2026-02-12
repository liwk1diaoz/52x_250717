/*
    IME module driver

    NT96520 IME module driver.

    @file       ime_pm_base.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/


#include "ime_reg.h"
#include "ime_int_public.h"
#include "ime_pm_base.h"


static UINT32 ime_pm_in_pxl_addr = 0x0;

#if (IPP_DRIVER_NEW_REG_HANDLE == DISABLE)

VOID ime_set_privacy_mask_pxl_blk_size_reg(UINT32 set_size)
{
	T_IME_PRIVACY_MASK_SET0_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER0_OFS);

	arg.bit.ime_pm_pxlsize = set_size;

	IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set0_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_pm0_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_privacy_mask_set0_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return arg.bit.ime_pm0_en;
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set0_hollow_enable_reg(UINT32 set_en)
{
	T_IME_PRIVACY_MASK_SET0_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER0_OFS);

	arg.bit.ime_pm0_hlw_en = set_en;

	IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set0_type_reg(UINT32 set_type)
{
	T_IME_PRIVACY_MASK_SET0_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER0_OFS);

	arg.bit.ime_pm0_type = set_type;

	IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set0_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v)
{
	T_IME_PRIVACY_MASK_SET0_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER0_OFS);

	arg.bit.ime_pm0_color_y = y;
	arg.bit.ime_pm0_color_u = u;
	arg.bit.ime_pm0_color_v = v;

	IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set0_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET0_REGISTER1 arg0;
	T_IME_PRIVACY_MASK_SET0_REGISTER2 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER1_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER2_OFS);

	arg0.bit.ime_pm0_line0_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm0_line0_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm0_line0_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm0_line0_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm0_line0_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm0_line0_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm0_line0_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER1_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER2_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set0_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET0_REGISTER3 arg0;
	T_IME_PRIVACY_MASK_SET0_REGISTER4 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER3_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER4_OFS);

	arg0.bit.ime_pm0_line1_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm0_line1_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm0_line1_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm0_line1_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm0_line1_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm0_line1_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm0_line1_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER3_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER4_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set0_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET0_REGISTER5 arg0;
	T_IME_PRIVACY_MASK_SET0_REGISTER6 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER5_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER6_OFS);

	arg0.bit.ime_pm0_line2_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm0_line2_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm0_line2_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm0_line2_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm0_line2_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm0_line2_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm0_line2_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER5_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER6_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set0_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET0_REGISTER7 arg0;
	T_IME_PRIVACY_MASK_SET0_REGISTER8 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER7_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER8_OFS);

	arg0.bit.ime_pm0_line3_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm0_line3_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm0_line3_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm0_line3_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm0_line3_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm0_line3_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm0_line3_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER7_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER8_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set0_weight_reg(UINT32 wet)
{
	T_IME_PRIVACY_MASK_ALPHA_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS);

	arg.bit.ime_pm0_awet = wet;

	IME_SETREG(IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set1_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_pm1_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_privacy_mask_set1_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return arg.bit.ime_pm1_en;
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set1_type_reg(UINT32 set_type)
{
	T_IME_PRIVACY_MASK_SET1_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER0_OFS);

	arg.bit.ime_pm1_type = set_type;

	IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set1_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v)
{
	T_IME_PRIVACY_MASK_SET1_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER0_OFS);

	arg.bit.ime_pm1_color_y = y;
	arg.bit.ime_pm1_color_u = u;
	arg.bit.ime_pm1_color_v = v;

	IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set1_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET1_REGISTER1 arg0;
	T_IME_PRIVACY_MASK_SET1_REGISTER2 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER1_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER2_OFS);

	arg0.bit.ime_pm1_line0_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm1_line0_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm1_line0_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm1_line0_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm1_line0_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm1_line0_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm1_line0_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER1_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER2_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set1_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET1_REGISTER3 arg0;
	T_IME_PRIVACY_MASK_SET1_REGISTER4 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER3_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER4_OFS);

	arg0.bit.ime_pm1_line1_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm1_line1_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm1_line1_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm1_line1_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm1_line1_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm1_line1_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm1_line1_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER3_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER4_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set1_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET1_REGISTER5 arg0;
	T_IME_PRIVACY_MASK_SET1_REGISTER6 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER5_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER6_OFS);

	arg0.bit.ime_pm1_line2_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm1_line2_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm1_line2_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm1_line2_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm1_line2_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm1_line2_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm1_line2_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER5_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER6_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set1_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET1_REGISTER7 arg0;
	T_IME_PRIVACY_MASK_SET1_REGISTER8 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER7_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER8_OFS);

	arg0.bit.ime_pm1_line3_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm1_line3_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm1_line3_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm1_line3_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm1_line3_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm1_line3_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm1_line3_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER7_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER8_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set1_seight_reg(UINT32 wet)
{
	T_IME_PRIVACY_MASK_ALPHA_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS);

	arg.bit.ime_pm1_awet = wet;

	IME_SETREG(IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set2_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_pm2_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_privacy_mask_set2_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return arg.bit.ime_pm2_en;
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set2_hollow_enable_reg(UINT32 set_en)
{
	T_IME_PRIVACY_MASK_SET2_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER0_OFS);

	arg.bit.ime_pm2_hlw_en = set_en;

	IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set2_type_reg(UINT32 set_type)
{
	T_IME_PRIVACY_MASK_SET2_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER0_OFS);

	arg.bit.ime_pm2_type = set_type;

	IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set2_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v)
{
	T_IME_PRIVACY_MASK_SET2_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER0_OFS);

	arg.bit.ime_pm2_color_y = y;
	arg.bit.ime_pm2_color_u = u;
	arg.bit.ime_pm2_color_v = v;

	IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set2_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET2_REGISTER1 arg0;
	T_IME_PRIVACY_MASK_SET2_REGISTER2 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER1_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER2_OFS);

	arg0.bit.ime_pm2_line0_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm2_line0_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm2_line0_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm2_line0_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm2_line0_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm2_line0_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm2_line0_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER1_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER2_OFS, arg1.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set2_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET2_REGISTER3 arg0;
	T_IME_PRIVACY_MASK_SET2_REGISTER4 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER3_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER4_OFS);

	arg0.bit.ime_pm2_line1_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm2_line1_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm2_line1_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm2_line1_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm2_line1_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm2_line1_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm2_line1_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER3_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER4_OFS, arg1.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set2_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET2_REGISTER5 arg0;
	T_IME_PRIVACY_MASK_SET2_REGISTER6 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER5_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER6_OFS);

	arg0.bit.ime_pm2_line2_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm2_line2_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm2_line2_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm2_line2_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm2_line2_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm2_line2_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm2_line2_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER5_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER6_OFS, arg1.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set2_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET2_REGISTER7 arg0;
	T_IME_PRIVACY_MASK_SET2_REGISTER8 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER7_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER8_OFS);

	arg0.bit.ime_pm2_line3_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm2_line3_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm2_line3_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm2_line3_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm2_line3_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm2_line3_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm2_line3_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER7_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER8_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set2_weight_reg(UINT32 wet)
{
	T_IME_PRIVACY_MASK_ALPHA_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS);

	arg.bit.ime_pm2_awet = wet;

	IME_SETREG(IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------



VOID ime_set_privacy_mask_set3_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_pm3_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_privacy_mask_set3_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return arg.bit.ime_pm3_en;
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set3_type_reg(UINT32 set_type)
{
	T_IME_PRIVACY_MASK_SET3_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER0_OFS);

	arg.bit.ime_pm3_type = set_type;

	IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set3_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v)
{
	T_IME_PRIVACY_MASK_SET3_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER0_OFS);

	arg.bit.ime_pm3_color_y = y;
	arg.bit.ime_pm3_color_u = u;
	arg.bit.ime_pm3_color_v = v;

	IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set3_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET3_REGISTER1 arg0;
	T_IME_PRIVACY_MASK_SET3_REGISTER2 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER1_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER2_OFS);

	arg0.bit.ime_pm3_line0_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm3_line0_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm3_line0_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm3_line0_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm3_line0_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm3_line0_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm3_line0_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER1_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER2_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set3_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET3_REGISTER3 arg0;
	T_IME_PRIVACY_MASK_SET3_REGISTER4 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER3_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER4_OFS);

	arg0.bit.ime_pm3_line1_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm3_line1_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm3_line1_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm3_line1_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm3_line1_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm3_line1_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm3_line1_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER3_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER4_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set3_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET3_REGISTER5 arg0;
	T_IME_PRIVACY_MASK_SET3_REGISTER6 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER5_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER6_OFS);

	arg0.bit.ime_pm3_line2_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm3_line2_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm3_line2_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm3_line2_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm3_line2_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm3_line2_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm3_line2_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER5_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER6_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set3_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET3_REGISTER7 arg0;
	T_IME_PRIVACY_MASK_SET3_REGISTER8 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER7_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER8_OFS);

	arg0.bit.ime_pm3_line3_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm3_line3_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm3_line3_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm3_line3_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm3_line3_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm3_line3_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm3_line3_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER7_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER8_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set3_weight_reg(UINT32 wet)
{
	T_IME_PRIVACY_MASK_ALPHA_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS);

	arg.bit.ime_pm3_awet = wet;

	IME_SETREG(IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------



VOID ime_set_privacy_mask_set4_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_pm4_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_privacy_mask_set4_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return arg.bit.ime_pm4_en;
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set4_hollow_enable_reg(UINT32 set_en)
{
	T_IME_PRIVACY_MASK_SET4_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER0_OFS);

	arg.bit.ime_pm4_hlw_en = set_en;

	IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


extern VOID ime_set_privacy_mask_set4_type_reg(UINT32 set_type)
{
	T_IME_PRIVACY_MASK_SET4_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER0_OFS);

	arg.bit.ime_pm4_type = set_type;

	IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set4_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v)
{
	T_IME_PRIVACY_MASK_SET4_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER0_OFS);

	arg.bit.ime_pm4_color_y = y;
	arg.bit.ime_pm4_color_u = u;
	arg.bit.ime_pm4_color_v = v;

	IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set4_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET4_REGISTER1 arg0;
	T_IME_PRIVACY_MASK_SET4_REGISTER2 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER1_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER2_OFS);

	arg0.bit.ime_pm4_line0_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm4_line0_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm4_line0_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm4_line0_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm4_line0_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm4_line0_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm4_line0_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER1_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER2_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set4_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET4_REGISTER3 arg0;
	T_IME_PRIVACY_MASK_SET4_REGISTER4 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER3_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER4_OFS);

	arg0.bit.ime_pm4_line1_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm4_line1_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm4_line1_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm4_line1_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm4_line1_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm4_line1_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm4_line1_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER3_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER4_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set4_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET4_REGISTER5 arg0;
	T_IME_PRIVACY_MASK_SET4_REGISTER6 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER5_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER6_OFS);

	arg0.bit.ime_pm4_line2_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm4_line2_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm4_line2_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm4_line2_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm4_line2_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm4_line2_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm4_line2_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER5_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER6_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set4_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET4_REGISTER7 arg0;
	T_IME_PRIVACY_MASK_SET4_REGISTER8 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER7_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER8_OFS);

	arg0.bit.ime_pm4_line3_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm4_line3_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm4_line3_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm4_line3_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm4_line3_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm4_line3_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm4_line3_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER7_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER8_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set4_weight_reg(UINT32 wet)
{
	T_IME_PRIVACY_MASK_ALPHA_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS);

	arg.bit.ime_pm4_awet = wet;

	IME_SETREG(IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set5_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_pm5_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_privacy_mask_set5_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return arg.bit.ime_pm5_en;
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set5_type_reg(UINT32 set_type)
{
	T_IME_PRIVACY_MASK_SET5_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER0_OFS);

	arg.bit.ime_pm5_type = set_type;

	IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set5_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v)
{
	T_IME_PRIVACY_MASK_SET5_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER0_OFS);

	arg.bit.ime_pm5_color_y = y;
	arg.bit.ime_pm5_color_u = u;
	arg.bit.ime_pm5_color_v = v;

	IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set5_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET5_REGISTER1 arg0;
	T_IME_PRIVACY_MASK_SET5_REGISTER2 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER1_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER2_OFS);

	arg0.bit.ime_pm5_line0_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm5_line0_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm5_line0_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm5_line0_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm5_line0_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm5_line0_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm5_line0_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER1_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER2_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set5_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET5_REGISTER3 arg0;
	T_IME_PRIVACY_MASK_SET5_REGISTER4 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER3_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER4_OFS);

	arg0.bit.ime_pm5_line1_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm5_line1_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm5_line1_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm5_line1_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm5_line1_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm5_line1_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm5_line1_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER3_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER4_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set5_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET5_REGISTER5 arg0;
	T_IME_PRIVACY_MASK_SET5_REGISTER6 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER5_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER6_OFS);

	arg0.bit.ime_pm5_line2_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm5_line2_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm5_line2_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm5_line2_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm5_line2_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm5_line2_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm5_line2_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER5_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER6_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set5_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET5_REGISTER7 arg0;
	T_IME_PRIVACY_MASK_SET5_REGISTER8 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER7_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER8_OFS);

	arg0.bit.ime_pm5_line3_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm5_line3_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm5_line3_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm5_line3_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm5_line3_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm5_line3_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm5_line3_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER7_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER8_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set5_weight_reg(UINT32 wet)
{
	T_IME_PRIVACY_MASK_ALPHA_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS);

	arg.bit.ime_pm5_awet = wet;

	IME_SETREG(IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set6_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_pm6_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_privacy_mask_set6_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return arg.bit.ime_pm6_en;
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set6_hollow_enable_reg(UINT32 set_en)
{
	T_IME_PRIVACY_MASK_SET6_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER0_OFS);

	arg.bit.ime_pm6_hlw_en = set_en;

	IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set6_hollow_type_reg(UINT32 set_type)
{
	T_IME_PRIVACY_MASK_SET6_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER0_OFS);

	arg.bit.ime_pm6_type = set_type;

	IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set6_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v)
{
	T_IME_PRIVACY_MASK_SET6_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER0_OFS);

	arg.bit.ime_pm6_color_y = y;
	arg.bit.ime_pm6_color_u = u;
	arg.bit.ime_pm6_color_v = v;

	IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set6_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET6_REGISTER1 arg0;
	T_IME_PRIVACY_MASK_SET6_REGISTER2 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER1_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER2_OFS);

	arg0.bit.ime_pm6_line0_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm6_line0_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm6_line0_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm6_line0_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm6_line0_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm6_line0_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm6_line0_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER1_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER2_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set6_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET6_REGISTER3 arg0;
	T_IME_PRIVACY_MASK_SET6_REGISTER4 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER3_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER4_OFS);

	arg0.bit.ime_pm6_line1_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm6_line1_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm6_line1_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm6_line1_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm6_line1_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm6_line1_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm6_line1_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER3_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER4_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set6_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET6_REGISTER5 arg0;
	T_IME_PRIVACY_MASK_SET6_REGISTER6 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER5_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER6_OFS);

	arg0.bit.ime_pm6_line2_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm6_line2_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm6_line2_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm6_line2_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm6_line2_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm6_line2_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm6_line2_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER5_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER6_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set6_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET6_REGISTER7 arg0;
	T_IME_PRIVACY_MASK_SET6_REGISTER8 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER7_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER8_OFS);

	arg0.bit.ime_pm6_line3_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm6_line3_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm6_line3_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm6_line3_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm6_line3_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm6_line3_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm6_line3_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER7_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER8_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set6_weight_reg(UINT32 wet)
{
	T_IME_PRIVACY_MASK_ALPHA_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS);

	arg.bit.ime_pm6_awet = wet;

	IME_SETREG(IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set7_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_pm7_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_privacy_mask_set7_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return arg.bit.ime_pm7_en;
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set7_type_reg(UINT32 set_type)
{
	T_IME_PRIVACY_MASK_SET7_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER0_OFS);

	arg.bit.ime_pm7_type = set_type;

	IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set7_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v)
{
	T_IME_PRIVACY_MASK_SET7_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER0_OFS);

	arg.bit.ime_pm7_color_y = y;
	arg.bit.ime_pm7_color_u = u;
	arg.bit.ime_pm7_color_v = v;

	IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set7_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET7_REGISTER1 arg0;
	T_IME_PRIVACY_MASK_SET7_REGISTER2 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER1_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER2_OFS);

	arg0.bit.ime_pm7_line0_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm7_line0_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm7_line0_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm7_line0_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm7_line0_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm7_line0_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm7_line0_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER1_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER2_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set7_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET7_REGISTER3 arg0;
	T_IME_PRIVACY_MASK_SET7_REGISTER4 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER3_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER4_OFS);

	arg0.bit.ime_pm7_line1_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm7_line1_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm7_line1_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm7_line1_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm7_line1_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm7_line1_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm7_line1_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER3_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER4_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set7_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET7_REGISTER5 arg0;
	T_IME_PRIVACY_MASK_SET7_REGISTER6 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER5_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER6_OFS);

	arg0.bit.ime_pm7_line2_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm7_line2_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm7_line2_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm7_line2_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm7_line2_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm7_line2_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm7_line2_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER5_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER6_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set7_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	T_IME_PRIVACY_MASK_SET7_REGISTER7 arg0;
	T_IME_PRIVACY_MASK_SET7_REGISTER8 arg1;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER7_OFS);
	arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER8_OFS);

	arg0.bit.ime_pm7_line3_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	arg0.bit.ime_pm7_line3_signa = ((coef_a < 0) ? 1 : 0);

	arg0.bit.ime_pm7_line3_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	arg0.bit.ime_pm7_line3_signb = ((coef_b < 0) ? 1 : 0);

	arg1.bit.ime_pm7_line3_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	arg1.bit.ime_pm7_line3_signc = ((coef_c < 0) ? 1 : 0);

	arg0.bit.ime_pm7_line3_comp = coef_d;

	IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER7_OFS, arg0.reg);
	IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER8_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set7_weight_reg(UINT32 wet)
{
	T_IME_PRIVACY_MASK_ALPHA_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS);

	arg.bit.ime_pm7_awet = wet;

	IME_SETREG(IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_pxl_image_size_reg(UINT32 size_h, UINT32 size_v)
{
	T_IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER_OFS);

	arg.bit.ime_pm_h_size = size_h - 1;
	arg.bit.ime_pm_v_size = size_v - 1;

	IME_SETREG(IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_pxl_image_format_reg(UINT32 set_fmt)
{
	T_IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER_OFS);

	arg.bit.ime_pm_fmt = set_fmt;

	IME_SETREG(IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_pxl_image_lineoffset_reg(UINT32 lofs)
{
	T_IME_PRIVACY_MASK_SUB_IMAGE_Y_CHANNEL_LINEOFFSET_REGISTER arg0;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SUB_IMAGE_Y_CHANNEL_LINEOFFSET_REGISTER_OFS);

	arg0.bit.ime_pm_y_ofsi = (lofs >> 2);

	IME_SETREG(IME_PRIVACY_MASK_SUB_IMAGE_Y_CHANNEL_LINEOFFSET_REGISTER_OFS, arg0.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_pxl_image_dma_addr_reg(UINT32 addr)
{
	T_IME_PRIVACY_MASK_IMAGE_INPUT_Y_CHANNEL_DMA_ADDRESS_REGISTER arg0;

	ime_pm_in_pxl_addr = addr;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_IMAGE_INPUT_Y_CHANNEL_DMA_ADDRESS_REGISTER_OFS);

	arg0.bit.ime_pm_y_sai = ime_platform_va2pa(ime_pm_in_pxl_addr) >> 2;//(uiaddr >> 2);

	IME_SETREG(IME_PRIVACY_MASK_IMAGE_INPUT_Y_CHANNEL_DMA_ADDRESS_REGISTER_OFS, arg0.reg);
}
//------------------------------------------------------------------------------

VOID ime_get_privacy_mask_pxl_image_size_reg(UINT32 *p_size_h, UINT32 *p_size_v)
{
	T_IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER arg;

	arg.reg = IME_GETREG(IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER_OFS);

	*p_size_h = arg.bit.ime_pm_h_size + 1;
	*p_size_v = arg.bit.ime_pm_v_size + 1;
}
//------------------------------------------------------------------------------

VOID ime_get_privacy_mask_pxl_image_lineoffset_reg(UINT32 *p_lofs)
{
	T_IME_PRIVACY_MASK_SUB_IMAGE_Y_CHANNEL_LINEOFFSET_REGISTER arg0;

	arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SUB_IMAGE_Y_CHANNEL_LINEOFFSET_REGISTER_OFS);

	*p_lofs = (arg0.bit.ime_pm_y_ofsi << 2);
}
//------------------------------------------------------------------------------

VOID ime_get_privacy_mask_pxl_image_dma_addr_reg(UINT32 *p_addr)
{
	//T_IME_PRIVACY_MASK_IMAGE_INPUT_Y_CHANNEL_DMA_ADDRESS_REGISTER arg0;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_IMAGE_INPUT_Y_CHANNEL_DMA_ADDRESS_REGISTER_OFS);

	*p_addr = ime_pm_in_pxl_addr;//(arg0.bit.ime_pm_y_sai << 2);
}
//------------------------------------------------------------------------------

#else


VOID ime_set_privacy_mask_pxl_blk_size_reg(UINT32 set_size)
{
	//T_IME_PRIVACY_MASK_SET0_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER0_OFS);

	imeg->reg_264.bit.ime_pm_pxlsize = set_size;

	//IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set0_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_pm0_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_privacy_mask_set0_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return imeg->reg_2.bit.ime_pm0_en;
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set0_hollow_enable_reg(UINT32 set_en)
{
	//T_IME_PRIVACY_MASK_SET0_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER0_OFS);

	imeg->reg_264.bit.ime_pm0_hlw_en = set_en;

	//IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set0_type_reg(UINT32 set_type)
{
	//T_IME_PRIVACY_MASK_SET0_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER0_OFS);

	imeg->reg_264.bit.ime_pm0_type = set_type;

	//IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set0_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v)
{
	//T_IME_PRIVACY_MASK_SET0_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER0_OFS);

	imeg->reg_264.bit.ime_pm0_color_y = y;
	imeg->reg_264.bit.ime_pm0_color_u = u;
	imeg->reg_264.bit.ime_pm0_color_v = v;

	//IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set0_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET0_REGISTER1 arg0;
	//T_IME_PRIVACY_MASK_SET0_REGISTER2 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER1_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER2_OFS);

	imeg->reg_265.bit.ime_pm0_line0_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_265.bit.ime_pm0_line0_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_265.bit.ime_pm0_line0_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_265.bit.ime_pm0_line0_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_266.bit.ime_pm0_line0_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_266.bit.ime_pm0_line0_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_265.bit.ime_pm0_line0_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER1_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER2_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set0_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET0_REGISTER3 arg0;
	//T_IME_PRIVACY_MASK_SET0_REGISTER4 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER3_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER4_OFS);

	imeg->reg_267.bit.ime_pm0_line1_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_267.bit.ime_pm0_line1_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_267.bit.ime_pm0_line1_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_267.bit.ime_pm0_line1_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_268.bit.ime_pm0_line1_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_268.bit.ime_pm0_line1_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_267.bit.ime_pm0_line1_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER3_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER4_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set0_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET0_REGISTER5 arg0;
	//T_IME_PRIVACY_MASK_SET0_REGISTER6 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER5_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER6_OFS);

	imeg->reg_269.bit.ime_pm0_line2_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_269.bit.ime_pm0_line2_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_269.bit.ime_pm0_line2_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_269.bit.ime_pm0_line2_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_270.bit.ime_pm0_line2_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_270.bit.ime_pm0_line2_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_269.bit.ime_pm0_line2_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER5_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER6_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set0_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET0_REGISTER7 arg0;
	//T_IME_PRIVACY_MASK_SET0_REGISTER8 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER7_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET0_REGISTER8_OFS);

	imeg->reg_271.bit.ime_pm0_line3_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_271.bit.ime_pm0_line3_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_271.bit.ime_pm0_line3_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_271.bit.ime_pm0_line3_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_272.bit.ime_pm0_line3_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_272.bit.ime_pm0_line3_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_271.bit.ime_pm0_line3_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER7_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET0_REGISTER8_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set0_weight_reg(UINT32 wet)
{
	//T_IME_PRIVACY_MASK_ALPHA_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS);

	imeg->reg_300.bit.ime_pm0_awet = wet;

	//IME_SETREG(IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set1_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_pm1_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_privacy_mask_set1_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return imeg->reg_2.bit.ime_pm1_en;
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set1_type_reg(UINT32 set_type)
{
	//T_IME_PRIVACY_MASK_SET1_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER0_OFS);

	imeg->reg_273.bit.ime_pm1_type = set_type;

	//IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set1_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v)
{
	//T_IME_PRIVACY_MASK_SET1_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER0_OFS);

	imeg->reg_273.bit.ime_pm1_color_y = y;
	imeg->reg_273.bit.ime_pm1_color_u = u;
	imeg->reg_273.bit.ime_pm1_color_v = v;

	//IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set1_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET1_REGISTER1 arg0;
	//T_IME_PRIVACY_MASK_SET1_REGISTER2 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER1_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER2_OFS);

	imeg->reg_274.bit.ime_pm1_line0_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_274.bit.ime_pm1_line0_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_274.bit.ime_pm1_line0_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_274.bit.ime_pm1_line0_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_275.bit.ime_pm1_line0_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_275.bit.ime_pm1_line0_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_274.bit.ime_pm1_line0_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER1_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER2_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set1_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET1_REGISTER3 arg0;
	//T_IME_PRIVACY_MASK_SET1_REGISTER4 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER3_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER4_OFS);

	imeg->reg_276.bit.ime_pm1_line1_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_276.bit.ime_pm1_line1_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_276.bit.ime_pm1_line1_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_276.bit.ime_pm1_line1_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_277.bit.ime_pm1_line1_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_277.bit.ime_pm1_line1_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_276.bit.ime_pm1_line1_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER3_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER4_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set1_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET1_REGISTER5 arg0;
	//T_IME_PRIVACY_MASK_SET1_REGISTER6 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER5_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER6_OFS);

	imeg->reg_278.bit.ime_pm1_line2_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_278.bit.ime_pm1_line2_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_278.bit.ime_pm1_line2_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_278.bit.ime_pm1_line2_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_279.bit.ime_pm1_line2_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_279.bit.ime_pm1_line2_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_278.bit.ime_pm1_line2_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER5_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER6_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set1_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET1_REGISTER7 arg0;
	//T_IME_PRIVACY_MASK_SET1_REGISTER8 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER7_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET1_REGISTER8_OFS);

	imeg->reg_280.bit.ime_pm1_line3_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_280.bit.ime_pm1_line3_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_280.bit.ime_pm1_line3_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_280.bit.ime_pm1_line3_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_281.bit.ime_pm1_line3_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_281.bit.ime_pm1_line3_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_280.bit.ime_pm1_line3_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER7_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET1_REGISTER8_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set1_seight_reg(UINT32 wet)
{
	//T_IME_PRIVACY_MASK_ALPHA_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS);

	imeg->reg_300.bit.ime_pm1_awet = wet;

	//IME_SETREG(IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set2_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_pm2_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_privacy_mask_set2_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return imeg->reg_2.bit.ime_pm2_en;
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set2_hollow_enable_reg(UINT32 set_en)
{
	//T_IME_PRIVACY_MASK_SET2_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER0_OFS);

	imeg->reg_282.bit.ime_pm2_hlw_en = set_en;

	//IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set2_type_reg(UINT32 set_type)
{
	//T_IME_PRIVACY_MASK_SET2_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER0_OFS);

	imeg->reg_282.bit.ime_pm2_type = set_type;

	//IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set2_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v)
{
	//T_IME_PRIVACY_MASK_SET2_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER0_OFS);

	imeg->reg_282.bit.ime_pm2_color_y = y;
	imeg->reg_282.bit.ime_pm2_color_u = u;
	imeg->reg_282.bit.ime_pm2_color_v = v;

	//IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set2_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET2_REGISTER1 arg0;
	//T_IME_PRIVACY_MASK_SET2_REGISTER2 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER1_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER2_OFS);

	imeg->reg_283.bit.ime_pm2_line0_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_283.bit.ime_pm2_line0_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_283.bit.ime_pm2_line0_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_283.bit.ime_pm2_line0_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_284.bit.ime_pm2_line0_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_284.bit.ime_pm2_line0_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_283.bit.ime_pm2_line0_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER1_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER2_OFS, arg1.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set2_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET2_REGISTER3 arg0;
	//T_IME_PRIVACY_MASK_SET2_REGISTER4 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER3_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER4_OFS);

	imeg->reg_285.bit.ime_pm2_line1_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_285.bit.ime_pm2_line1_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_285.bit.ime_pm2_line1_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_285.bit.ime_pm2_line1_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_286.bit.ime_pm2_line1_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_286.bit.ime_pm2_line1_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_285.bit.ime_pm2_line1_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER3_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER4_OFS, arg1.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set2_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET2_REGISTER5 arg0;
	//T_IME_PRIVACY_MASK_SET2_REGISTER6 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER5_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER6_OFS);

	imeg->reg_287.bit.ime_pm2_line2_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_287.bit.ime_pm2_line2_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_287.bit.ime_pm2_line2_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_287.bit.ime_pm2_line2_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_288.bit.ime_pm2_line2_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_288.bit.ime_pm2_line2_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_287.bit.ime_pm2_line2_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER5_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER6_OFS, arg1.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set2_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET2_REGISTER7 arg0;
	//T_IME_PRIVACY_MASK_SET2_REGISTER8 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER7_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET2_REGISTER8_OFS);

	imeg->reg_289.bit.ime_pm2_line3_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_289.bit.ime_pm2_line3_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_289.bit.ime_pm2_line3_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_289.bit.ime_pm2_line3_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_290.bit.ime_pm2_line3_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_290.bit.ime_pm2_line3_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_289.bit.ime_pm2_line3_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER7_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET2_REGISTER8_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set2_weight_reg(UINT32 wet)
{
	//T_IME_PRIVACY_MASK_ALPHA_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS);

	imeg->reg_300.bit.ime_pm2_awet = wet;

	//IME_SETREG(IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------



VOID ime_set_privacy_mask_set3_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_pm3_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_privacy_mask_set3_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return imeg->reg_2.bit.ime_pm3_en;
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set3_type_reg(UINT32 set_type)
{
	//T_IME_PRIVACY_MASK_SET3_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER0_OFS);

	imeg->reg_291.bit.ime_pm3_type = set_type;

	//IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set3_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v)
{
	//T_IME_PRIVACY_MASK_SET3_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER0_OFS);

	imeg->reg_291.bit.ime_pm3_color_y = y;
	imeg->reg_291.bit.ime_pm3_color_u = u;
	imeg->reg_291.bit.ime_pm3_color_v = v;

	//IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set3_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET3_REGISTER1 arg0;
	//T_IME_PRIVACY_MASK_SET3_REGISTER2 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER1_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER2_OFS);

	imeg->reg_292.bit.ime_pm3_line0_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_292.bit.ime_pm3_line0_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_292.bit.ime_pm3_line0_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_292.bit.ime_pm3_line0_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_293.bit.ime_pm3_line0_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_293.bit.ime_pm3_line0_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_292.bit.ime_pm3_line0_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER1_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER2_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set3_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET3_REGISTER3 arg0;
	//T_IME_PRIVACY_MASK_SET3_REGISTER4 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER3_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER4_OFS);

	imeg->reg_294.bit.ime_pm3_line1_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_294.bit.ime_pm3_line1_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_294.bit.ime_pm3_line1_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_294.bit.ime_pm3_line1_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_295.bit.ime_pm3_line1_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_295.bit.ime_pm3_line1_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_294.bit.ime_pm3_line1_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER3_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER4_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set3_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET3_REGISTER5 arg0;
	//T_IME_PRIVACY_MASK_SET3_REGISTER6 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER5_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER6_OFS);

	imeg->reg_296.bit.ime_pm3_line2_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_296.bit.ime_pm3_line2_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_296.bit.ime_pm3_line2_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_296.bit.ime_pm3_line2_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_297.bit.ime_pm3_line2_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_297.bit.ime_pm3_line2_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_296.bit.ime_pm3_line2_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER5_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER6_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set3_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET3_REGISTER7 arg0;
	//T_IME_PRIVACY_MASK_SET3_REGISTER8 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER7_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET3_REGISTER8_OFS);

	imeg->reg_298.bit.ime_pm3_line3_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_298.bit.ime_pm3_line3_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_298.bit.ime_pm3_line3_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_298.bit.ime_pm3_line3_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_299.bit.ime_pm3_line3_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_299.bit.ime_pm3_line3_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_298.bit.ime_pm3_line3_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER7_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET3_REGISTER8_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set3_weight_reg(UINT32 wet)
{
	//T_IME_PRIVACY_MASK_ALPHA_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS);

	imeg->reg_300.bit.ime_pm3_awet = wet;

	//IME_SETREG(IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------



VOID ime_set_privacy_mask_set4_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_pm4_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_privacy_mask_set4_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return imeg->reg_2.bit.ime_pm4_en;
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set4_hollow_enable_reg(UINT32 set_en)
{
	//T_IME_PRIVACY_MASK_SET4_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER0_OFS);

	imeg->reg_301.bit.ime_pm4_hlw_en = set_en;

	//IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


extern VOID ime_set_privacy_mask_set4_type_reg(UINT32 set_type)
{
	//T_IME_PRIVACY_MASK_SET4_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER0_OFS);

	imeg->reg_301.bit.ime_pm4_type = set_type;

	//IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set4_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v)
{
	//T_IME_PRIVACY_MASK_SET4_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER0_OFS);

	imeg->reg_301.bit.ime_pm4_color_y = y;
	imeg->reg_301.bit.ime_pm4_color_u = u;
	imeg->reg_301.bit.ime_pm4_color_v = v;

	//IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set4_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET4_REGISTER1 arg0;
	//T_IME_PRIVACY_MASK_SET4_REGISTER2 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER1_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER2_OFS);

	imeg->reg_302.bit.ime_pm4_line0_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_302.bit.ime_pm4_line0_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_302.bit.ime_pm4_line0_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_302.bit.ime_pm4_line0_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_303.bit.ime_pm4_line0_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_303.bit.ime_pm4_line0_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_302.bit.ime_pm4_line0_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER1_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER2_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set4_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET4_REGISTER3 arg0;
	//T_IME_PRIVACY_MASK_SET4_REGISTER4 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER3_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER4_OFS);

	imeg->reg_304.bit.ime_pm4_line1_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_304.bit.ime_pm4_line1_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_304.bit.ime_pm4_line1_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_304.bit.ime_pm4_line1_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_305.bit.ime_pm4_line1_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_305.bit.ime_pm4_line1_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_304.bit.ime_pm4_line1_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER3_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER4_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set4_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET4_REGISTER5 arg0;
	//T_IME_PRIVACY_MASK_SET4_REGISTER6 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER5_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER6_OFS);

	imeg->reg_306.bit.ime_pm4_line2_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_306.bit.ime_pm4_line2_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_306.bit.ime_pm4_line2_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_306.bit.ime_pm4_line2_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_307.bit.ime_pm4_line2_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_307.bit.ime_pm4_line2_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_306.bit.ime_pm4_line2_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER5_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER6_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set4_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET4_REGISTER7 arg0;
	//T_IME_PRIVACY_MASK_SET4_REGISTER8 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER7_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET4_REGISTER8_OFS);

	imeg->reg_308.bit.ime_pm4_line3_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_308.bit.ime_pm4_line3_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_308.bit.ime_pm4_line3_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_308.bit.ime_pm4_line3_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_309.bit.ime_pm4_line3_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_309.bit.ime_pm4_line3_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_308.bit.ime_pm4_line3_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER7_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET4_REGISTER8_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set4_weight_reg(UINT32 wet)
{
	//T_IME_PRIVACY_MASK_ALPHA_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS);

	imeg->reg_337.bit.ime_pm4_awet = wet;

	//IME_SETREG(IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set5_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_pm5_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_privacy_mask_set5_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return imeg->reg_2.bit.ime_pm5_en;
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set5_type_reg(UINT32 set_type)
{
	//T_IME_PRIVACY_MASK_SET5_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER0_OFS);

	imeg->reg_310.bit.ime_pm5_type = set_type;

	//IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set5_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v)
{
	//T_IME_PRIVACY_MASK_SET5_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER0_OFS);

	imeg->reg_310.bit.ime_pm5_color_y = y;
	imeg->reg_310.bit.ime_pm5_color_u = u;
	imeg->reg_310.bit.ime_pm5_color_v = v;

	//IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set5_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET5_REGISTER1 arg0;
	//T_IME_PRIVACY_MASK_SET5_REGISTER2 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER1_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER2_OFS);

	imeg->reg_311.bit.ime_pm5_line0_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_311.bit.ime_pm5_line0_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_311.bit.ime_pm5_line0_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_311.bit.ime_pm5_line0_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_312.bit.ime_pm5_line0_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_312.bit.ime_pm5_line0_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_311.bit.ime_pm5_line0_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER1_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER2_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set5_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET5_REGISTER3 arg0;
	//T_IME_PRIVACY_MASK_SET5_REGISTER4 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER3_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER4_OFS);

	imeg->reg_313.bit.ime_pm5_line1_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_313.bit.ime_pm5_line1_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_313.bit.ime_pm5_line1_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_313.bit.ime_pm5_line1_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_314.bit.ime_pm5_line1_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_314.bit.ime_pm5_line1_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_313.bit.ime_pm5_line1_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER3_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER4_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set5_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET5_REGISTER5 arg0;
	//T_IME_PRIVACY_MASK_SET5_REGISTER6 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER5_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER6_OFS);

	imeg->reg_315.bit.ime_pm5_line2_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_315.bit.ime_pm5_line2_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_315.bit.ime_pm5_line2_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_315.bit.ime_pm5_line2_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_316.bit.ime_pm5_line2_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_316.bit.ime_pm5_line2_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_315.bit.ime_pm5_line2_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER5_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER6_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set5_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET5_REGISTER7 arg0;
	//T_IME_PRIVACY_MASK_SET5_REGISTER8 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER7_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET5_REGISTER8_OFS);

	imeg->reg_317.bit.ime_pm5_line3_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_317.bit.ime_pm5_line3_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_317.bit.ime_pm5_line3_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_317.bit.ime_pm5_line3_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_318.bit.ime_pm5_line3_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_318.bit.ime_pm5_line3_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_317.bit.ime_pm5_line3_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER7_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET5_REGISTER8_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set5_weight_reg(UINT32 wet)
{
	//T_IME_PRIVACY_MASK_ALPHA_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS);

	imeg->reg_337.bit.ime_pm5_awet = wet;

	//IME_SETREG(IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set6_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_pm6_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_privacy_mask_set6_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return imeg->reg_2.bit.ime_pm6_en;
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set6_hollow_enable_reg(UINT32 set_en)
{
	//T_IME_PRIVACY_MASK_SET6_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER0_OFS);

	imeg->reg_319.bit.ime_pm6_hlw_en = set_en;

	//IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set6_hollow_type_reg(UINT32 set_type)
{
	//T_IME_PRIVACY_MASK_SET6_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER0_OFS);

	imeg->reg_319.bit.ime_pm6_type = set_type;

	//IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set6_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v)
{
	//T_IME_PRIVACY_MASK_SET6_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER0_OFS);

	imeg->reg_319.bit.ime_pm6_color_y = y;
	imeg->reg_319.bit.ime_pm6_color_u = u;
	imeg->reg_319.bit.ime_pm6_color_v = v;

	//IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set6_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET6_REGISTER1 arg0;
	//T_IME_PRIVACY_MASK_SET6_REGISTER2 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER1_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER2_OFS);

	imeg->reg_320.bit.ime_pm6_line0_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_320.bit.ime_pm6_line0_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_320.bit.ime_pm6_line0_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_320.bit.ime_pm6_line0_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_321.bit.ime_pm6_line0_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_321.bit.ime_pm6_line0_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_320.bit.ime_pm6_line0_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER1_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER2_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set6_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET6_REGISTER3 arg0;
	//T_IME_PRIVACY_MASK_SET6_REGISTER4 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER3_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER4_OFS);

	imeg->reg_322.bit.ime_pm6_line1_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_322.bit.ime_pm6_line1_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_322.bit.ime_pm6_line1_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_322.bit.ime_pm6_line1_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_323.bit.ime_pm6_line1_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_323.bit.ime_pm6_line1_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_322.bit.ime_pm6_line1_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER3_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER4_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set6_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET6_REGISTER5 arg0;
	//T_IME_PRIVACY_MASK_SET6_REGISTER6 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER5_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER6_OFS);

	imeg->reg_324.bit.ime_pm6_line2_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_324.bit.ime_pm6_line2_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_324.bit.ime_pm6_line2_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_324.bit.ime_pm6_line2_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_325.bit.ime_pm6_line2_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_325.bit.ime_pm6_line2_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_324.bit.ime_pm6_line2_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER5_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER6_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set6_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET6_REGISTER7 arg0;
	//T_IME_PRIVACY_MASK_SET6_REGISTER8 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER7_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET6_REGISTER8_OFS);

	imeg->reg_326.bit.ime_pm6_line3_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_326.bit.ime_pm6_line3_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_326.bit.ime_pm6_line3_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_326.bit.ime_pm6_line3_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_327.bit.ime_pm6_line3_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_327.bit.ime_pm6_line3_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_326.bit.ime_pm6_line3_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER7_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET6_REGISTER8_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set6_weight_reg(UINT32 wet)
{
	//T_IME_PRIVACY_MASK_ALPHA_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS);

	imeg->reg_337.bit.ime_pm6_awet = wet;

	//IME_SETREG(IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set7_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_pm7_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_privacy_mask_set7_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return imeg->reg_2.bit.ime_pm7_en;
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set7_type_reg(UINT32 set_type)
{
	//T_IME_PRIVACY_MASK_SET7_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER0_OFS);

	imeg->reg_328.bit.ime_pm7_type = set_type;

	//IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_privacy_mask_set7_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v)
{
	//T_IME_PRIVACY_MASK_SET7_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER0_OFS);

	imeg->reg_328.bit.ime_pm7_color_y = y;
	imeg->reg_328.bit.ime_pm7_color_u = u;
	imeg->reg_328.bit.ime_pm7_color_v = v;

	//IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set7_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET7_REGISTER1 arg0;
	//T_IME_PRIVACY_MASK_SET7_REGISTER2 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER1_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER2_OFS);

	imeg->reg_329.bit.ime_pm7_line0_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_329.bit.ime_pm7_line0_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_329.bit.ime_pm7_line0_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_329.bit.ime_pm7_line0_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_330.bit.ime_pm7_line0_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_330.bit.ime_pm7_line0_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_329.bit.ime_pm7_line0_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER1_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER2_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set7_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET7_REGISTER3 arg0;
	//T_IME_PRIVACY_MASK_SET7_REGISTER4 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER3_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER4_OFS);

	imeg->reg_331.bit.ime_pm7_line1_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_331.bit.ime_pm7_line1_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_331.bit.ime_pm7_line1_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_331.bit.ime_pm7_line1_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_332.bit.ime_pm7_line1_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_332.bit.ime_pm7_line1_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_331.bit.ime_pm7_line1_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER3_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER4_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set7_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET7_REGISTER5 arg0;
	//T_IME_PRIVACY_MASK_SET7_REGISTER6 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER5_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER6_OFS);

	imeg->reg_333.bit.ime_pm7_line2_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_333.bit.ime_pm7_line2_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_333.bit.ime_pm7_line2_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_333.bit.ime_pm7_line2_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_334.bit.ime_pm7_line2_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_334.bit.ime_pm7_line2_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_333.bit.ime_pm7_line2_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER5_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER6_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set7_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d)
{
	//T_IME_PRIVACY_MASK_SET7_REGISTER7 arg0;
	//T_IME_PRIVACY_MASK_SET7_REGISTER8 arg1;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER7_OFS);
	//arg1.reg = IME_GETREG(IME_PRIVACY_MASK_SET7_REGISTER8_OFS);

	imeg->reg_335.bit.ime_pm7_line3_coefa = ((coef_a < 0) ? (-1 * coef_a) : coef_a);
	imeg->reg_335.bit.ime_pm7_line3_signa = ((coef_a < 0) ? 1 : 0);

	imeg->reg_335.bit.ime_pm7_line3_coefb = ((coef_b < 0) ? (-1 * coef_b) : coef_b);
	imeg->reg_335.bit.ime_pm7_line3_signb = ((coef_b < 0) ? 1 : 0);

	imeg->reg_336.bit.ime_pm7_line3_coefc = ((coef_c < 0) ? (-1 * coef_c) : coef_c);
	imeg->reg_336.bit.ime_pm7_line3_signc = ((coef_c < 0) ? 1 : 0);

	imeg->reg_335.bit.ime_pm7_line3_comp = coef_d;

	//IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER7_OFS, arg0.reg);
	//IME_SETREG(IME_PRIVACY_MASK_SET7_REGISTER8_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_set7_weight_reg(UINT32 wet)
{
	//T_IME_PRIVACY_MASK_ALPHA_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS);

	imeg->reg_337.bit.ime_pm7_awet = wet;

	//IME_SETREG(IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_pxl_image_size_reg(UINT32 size_h, UINT32 size_v)
{
	//T_IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER_OFS);

	imeg->reg_338.bit.ime_pm_h_size = size_h - 1;
	imeg->reg_338.bit.ime_pm_v_size = size_v - 1;

	//IME_SETREG(IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_pxl_image_format_reg(UINT32 set_fmt)
{
	//T_IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER_OFS);

	imeg->reg_338.bit.ime_pm_fmt = set_fmt;

	//IME_SETREG(IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_pxl_image_lineoffset_reg(UINT32 lofs)
{
	//T_IME_PRIVACY_MASK_SUB_IMAGE_Y_CHANNEL_LINEOFFSET_REGISTER arg0;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SUB_IMAGE_Y_CHANNEL_LINEOFFSET_REGISTER_OFS);

	imeg->reg_339.bit.ime_pm_y_ofsi = (lofs >> 2);

	//IME_SETREG(IME_PRIVACY_MASK_SUB_IMAGE_Y_CHANNEL_LINEOFFSET_REGISTER_OFS, arg0.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_privacy_mask_pxl_image_dma_addr_reg(UINT32 addr)
{
	//T_IME_PRIVACY_MASK_IMAGE_INPUT_Y_CHANNEL_DMA_ADDRESS_REGISTER arg0;

	ime_pm_in_pxl_addr = addr;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_IMAGE_INPUT_Y_CHANNEL_DMA_ADDRESS_REGISTER_OFS);

	imeg->reg_340.bit.ime_pm_y_sai = ime_platform_va2pa(ime_pm_in_pxl_addr) >> 2;//(uiaddr >> 2);

	//IME_SETREG(IME_PRIVACY_MASK_IMAGE_INPUT_Y_CHANNEL_DMA_ADDRESS_REGISTER_OFS, arg0.reg);
}
//------------------------------------------------------------------------------

VOID ime_get_privacy_mask_pxl_image_size_reg(UINT32 *p_size_h, UINT32 *p_size_v)
{
	//T_IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER arg;

	//arg.reg = IME_GETREG(IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER_OFS);

	*p_size_h = imeg->reg_338.bit.ime_pm_h_size + 1;
	*p_size_v = imeg->reg_338.bit.ime_pm_v_size + 1;
}
//------------------------------------------------------------------------------

VOID ime_get_privacy_mask_pxl_image_lineoffset_reg(UINT32 *p_lofs)
{
	//T_IME_PRIVACY_MASK_SUB_IMAGE_Y_CHANNEL_LINEOFFSET_REGISTER arg0;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_SUB_IMAGE_Y_CHANNEL_LINEOFFSET_REGISTER_OFS);

	*p_lofs = (imeg->reg_339.bit.ime_pm_y_ofsi << 2);
}
//------------------------------------------------------------------------------

VOID ime_get_privacy_mask_pxl_image_dma_addr_reg(UINT32 *p_addr)
{
	//T_IME_PRIVACY_MASK_IMAGE_INPUT_Y_CHANNEL_DMA_ADDRESS_REGISTER arg0;

	//arg0.reg = IME_GETREG(IME_PRIVACY_MASK_IMAGE_INPUT_Y_CHANNEL_DMA_ADDRESS_REGISTER_OFS);

	*p_addr = ime_pm_in_pxl_addr;//(arg0.bit.ime_pm_y_sai << 2);
}
//------------------------------------------------------------------------------



#endif


