/*
    IME module driver

    NT96520 IME module driver.

    @file       ime_adas_base.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/


#include "ime_reg.h"
#include "ime_int_public.h"

#include "ime_adas_base.h"


#if (IPP_DRIVER_NEW_REG_HANDLE == DISABLE)

VOID ime_set_dbcs_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_dbcs_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_dbcs_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return (UINT32)arg.bit.ime_dbcs_en;
}
//-------------------------------------------------------------------------------



VOID ime_set_dbcs_center_reg(UINT32 cent_u, UINT32 cent_v)
{
	T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0_OFS);

	arg.bit.dbcs_ctr_u = cent_u;
	arg.bit.dbcs_ctr_v = cent_v;

	IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_dbcs_mode_reg(UINT32 set_mode)
{
	T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0_OFS);

	arg.bit.dbcs_mode = set_mode;

	IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_dbcs_step_reg(UINT32 step_y, UINT32 step_uv)
{
	T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0_OFS);

	arg.bit.dbcs_step_y  = step_y;
	arg.bit.dbcs_step_uv = step_uv;

	IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_dbcs_weight_y_reg(UINT32 *p_wts)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER0 arg0;
	//T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER1 arg1;
	T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER2 arg2;

	//arg0.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER0_OFS);
	//arg1.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER1_OFS);


#if 0
	arg0.bit.dbcs_y_wt0 = p_wts[0];
	arg0.bit.dbcs_y_wt1 = p_wts[1];
	arg0.bit.dbcs_y_wt2 = p_wts[2];
	arg0.bit.dbcs_y_wt3 = p_wts[3];
	arg0.bit.dbcs_y_wt4 = p_wts[4];
	arg0.bit.dbcs_y_wt5 = p_wts[5];


	arg1.bit.dbcs_y_wt6  = p_wts[6];
	arg1.bit.dbcs_y_wt7  = p_wts[7];
	arg1.bit.dbcs_y_wt8  = p_wts[8];
	arg1.bit.dbcs_y_wt9  = p_wts[9];
	arg1.bit.dbcs_y_wt10 = p_wts[10];
	arg1.bit.dbcs_y_wt11 = p_wts[11];
#endif

	for (i = 0; i < 2; i++) {
		reg_set_val = 0;

		idx = (i * 6);
		reg_ofs = IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER0_OFS + (i << 2);
		reg_set_val = (p_wts[idx] & 0x1F) + ((p_wts[idx + 1] & 0x1F) << 5) + ((p_wts[idx + 2] & 0x1F) << 10) + ((p_wts[idx + 3] & 0x1F) << 15) + ((p_wts[idx + 4] & 0x1F) << 20) + ((p_wts[idx + 5] & 0x1F) << 25);

		IME_SETREG(reg_ofs, reg_set_val);
	}

	arg2.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER2_OFS);
	arg2.bit.dbcs_y_wt12  = p_wts[12];
	arg2.bit.dbcs_y_wt13  = p_wts[13];
	arg2.bit.dbcs_y_wt14  = p_wts[14];
	arg2.bit.dbcs_y_wt15  = p_wts[15];
	IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER2_OFS, arg2.reg);


	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER0_OFS, arg0.reg);
	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER1_OFS, arg1.reg);
	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER2_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_dbcs_weight_uv_reg(UINT32 *p_wts)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER3 arg0;
	//T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER4 arg1;
	T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER5 arg2;

	//arg0.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER3_OFS);
	//arg1.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER4_OFS);
	arg2.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER5_OFS);

#if 0
	arg0.bit.dbcs_c_wt0 = p_wts[0];
	arg0.bit.dbcs_c_wt1 = p_wts[1];
	arg0.bit.dbcs_c_wt2 = p_wts[2];
	arg0.bit.dbcs_c_wt3 = p_wts[3];
	arg0.bit.dbcs_c_wt4 = p_wts[4];
	arg0.bit.dbcs_c_wt5 = p_wts[5];


	arg1.bit.dbcs_c_wt6  = p_wts[6];
	arg1.bit.dbcs_c_wt7  = p_wts[7];
	arg1.bit.dbcs_c_wt8  = p_wts[8];
	arg1.bit.dbcs_c_wt9  = p_wts[9];
	arg1.bit.dbcs_c_wt10 = p_wts[10];
	arg1.bit.dbcs_c_wt11 = p_wts[11];
#endif

	for (i = 0; i < 2; i++) {
		reg_set_val = 0;

		idx = (i * 6);
		reg_ofs = IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER3_OFS + (i << 2);
		reg_set_val = (p_wts[idx] & 0x1F) + ((p_wts[idx + 1] & 0x1F) << 5) + ((p_wts[idx + 2] & 0x1F) << 10) + ((p_wts[idx + 3] & 0x1F) << 15) + ((p_wts[idx + 4] & 0x1F) << 20) + ((p_wts[idx + 5] & 0x1F) << 25);

		IME_SETREG(reg_ofs, reg_set_val);
	}

	arg2.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER5_OFS);
	arg2.bit.dbcs_c_wt12  = p_wts[12];
	arg2.bit.dbcs_c_wt13  = p_wts[13];
	arg2.bit.dbcs_c_wt14  = p_wts[14];
	arg2.bit.dbcs_c_wt15  = p_wts[15];
	IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER5_OFS, arg2.reg);

	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER3_OFS, arg0.reg);
	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER4_OFS, arg1.reg);
	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER5_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

#else

VOID ime_set_dbcs_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_dbcs_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_dbcs_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return (UINT32)imeg->reg_1.bit.ime_dbcs_en;
}
//-------------------------------------------------------------------------------



VOID ime_set_dbcs_center_reg(UINT32 cent_u, UINT32 cent_v)
{
	//T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0_OFS);

	imeg->reg_160.bit.dbcs_ctr_u = cent_u;
	imeg->reg_160.bit.dbcs_ctr_v = cent_v;

	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_dbcs_mode_reg(UINT32 set_mode)
{
	//T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0_OFS);

	imeg->reg_160.bit.dbcs_mode = set_mode;

	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_dbcs_step_reg(UINT32 step_y, UINT32 step_uv)
{
	//T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0_OFS);

	imeg->reg_160.bit.dbcs_step_y  = step_y;
	imeg->reg_160.bit.dbcs_step_uv = step_uv;

	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_dbcs_weight_y_reg(UINT32 *p_wts)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER0 arg0;
	//T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER1 arg1;
	//T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER2 arg2;

	//arg0.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER0_OFS);
	//arg1.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER1_OFS);


#if 0
	arg0.bit.dbcs_y_wt0 = p_wts[0];
	arg0.bit.dbcs_y_wt1 = p_wts[1];
	arg0.bit.dbcs_y_wt2 = p_wts[2];
	arg0.bit.dbcs_y_wt3 = p_wts[3];
	arg0.bit.dbcs_y_wt4 = p_wts[4];
	arg0.bit.dbcs_y_wt5 = p_wts[5];


	arg1.bit.dbcs_y_wt6  = p_wts[6];
	arg1.bit.dbcs_y_wt7  = p_wts[7];
	arg1.bit.dbcs_y_wt8  = p_wts[8];
	arg1.bit.dbcs_y_wt9  = p_wts[9];
	arg1.bit.dbcs_y_wt10 = p_wts[10];
	arg1.bit.dbcs_y_wt11 = p_wts[11];
#endif

	for (i = 0; i < 2; i++) {
		reg_set_val = 0;

		idx = (i * 6);
		reg_ofs = IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER0_OFS + (i << 2);
		reg_set_val = (p_wts[idx] & 0x1F) + ((p_wts[idx + 1] & 0x1F) << 5) + ((p_wts[idx + 2] & 0x1F) << 10) + ((p_wts[idx + 3] & 0x1F) << 15) + ((p_wts[idx + 4] & 0x1F) << 20) + ((p_wts[idx + 5] & 0x1F) << 25);

		IME_SETREG(reg_ofs, reg_set_val);
	}

	//arg2.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER2_OFS);
	imeg->reg_163.bit.dbcs_y_wt12  = p_wts[12];
	imeg->reg_163.bit.dbcs_y_wt13  = p_wts[13];
	imeg->reg_163.bit.dbcs_y_wt14  = p_wts[14];
	imeg->reg_163.bit.dbcs_y_wt15  = p_wts[15];
	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER2_OFS, arg2.reg);


	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER0_OFS, arg0.reg);
	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER1_OFS, arg1.reg);
	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER2_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_dbcs_weight_uv_reg(UINT32 *p_wts)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER3 arg0;
	//T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER4 arg1;
	//T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER5 arg2;

	//arg0.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER3_OFS);
	//arg1.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER4_OFS);
	//arg2.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER5_OFS);

#if 0
	arg0.bit.dbcs_c_wt0 = p_wts[0];
	arg0.bit.dbcs_c_wt1 = p_wts[1];
	arg0.bit.dbcs_c_wt2 = p_wts[2];
	arg0.bit.dbcs_c_wt3 = p_wts[3];
	arg0.bit.dbcs_c_wt4 = p_wts[4];
	arg0.bit.dbcs_c_wt5 = p_wts[5];


	arg1.bit.dbcs_c_wt6  = p_wts[6];
	arg1.bit.dbcs_c_wt7  = p_wts[7];
	arg1.bit.dbcs_c_wt8  = p_wts[8];
	arg1.bit.dbcs_c_wt9  = p_wts[9];
	arg1.bit.dbcs_c_wt10 = p_wts[10];
	arg1.bit.dbcs_c_wt11 = p_wts[11];
#endif

	for (i = 0; i < 2; i++) {
		reg_set_val = 0;

		idx = (i * 6);
		reg_ofs = IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER3_OFS + (i << 2);
		reg_set_val = (p_wts[idx] & 0x1F) + ((p_wts[idx + 1] & 0x1F) << 5) + ((p_wts[idx + 2] & 0x1F) << 10) + ((p_wts[idx + 3] & 0x1F) << 15) + ((p_wts[idx + 4] & 0x1F) << 20) + ((p_wts[idx + 5] & 0x1F) << 25);

		IME_SETREG(reg_ofs, reg_set_val);
	}

	//arg2.reg = IME_GETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER5_OFS);
	imeg->reg_166.bit.dbcs_c_wt12  = p_wts[12];
	imeg->reg_166.bit.dbcs_c_wt13  = p_wts[13];
	imeg->reg_166.bit.dbcs_c_wt14  = p_wts[14];
	imeg->reg_166.bit.dbcs_c_wt15  = p_wts[15];
	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER5_OFS, arg2.reg);

	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER3_OFS, arg0.reg);
	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER4_OFS, arg1.reg);
	//IME_SETREG(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER5_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------


#endif

