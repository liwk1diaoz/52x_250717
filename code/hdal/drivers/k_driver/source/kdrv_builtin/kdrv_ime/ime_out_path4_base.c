
/*
    IME module driver

    NT96510 IME module driver.

    @file       ime_out_path4_base.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/


#include "ime_reg.h"
#include "ime_int_public.h"

#include "ime_out_path4_base.h"


static UINT32 ime_p4_out_y_addr = 0x0;


#if (IPP_DRIVER_NEW_REG_HANDLE == DISABLE)

// output path4 APIs
VOID ime_set_output_p4_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_p4_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_flip_enable_p4_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_p4_flip_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------



VOID ime_set_output_dram_enable_p4_reg(UINT32 set_en)
{
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS);

	arg.bit.ime_out_en_p4 = set_en;

	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS, arg.reg);
}

//-------------------------------------------------------------------------------


VOID ime_set_output_image_format_p4_reg(UINT32 out_fmt_sel)
{
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS);

	arg.bit.ime_omat4 = out_fmt_sel;

	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scale_interpolation_method_p4_reg(UINT32 scl_method_sel)
{
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS);

	arg.bit.ime_scl_method_p4 = 0;// only bicubic, //uiscale_methodsel;

	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_type_p4_reg(UINT32 out_fmt_type)
{

	T_IME_OUTPUT_PATH4_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS);

	arg.bit.ime_out_type_p4 = 1;

	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS, arg.reg);

}
//-------------------------------------------------------------------------------


VOID ime_set_scale_factor_h_p4_reg(UINT32 h_scl_sel, UINT32 h_scl_rate, UINT32 h_scl_factor)
{
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER1 arg1;
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER2 arg2;

	arg1.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS);
	arg1.bit.h4_ud = h_scl_sel;
	arg1.bit.h4_dnrate = h_scl_rate;

	arg2.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER2_OFS);
	arg2.bit.h4_sfact = h_scl_factor;

	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS, arg1.reg);
	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER2_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scale_factor_v_p4_reg(UINT32 v_scl_sel, UINT32 v_scl_rate, UINT32 v_scl_factor)
{
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER1 arg1;
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER2 arg2;

	arg1.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS);
	arg1.bit.v4_ud = v_scl_sel;
	arg1.bit.v4_dnrate = v_scl_rate;
	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS, arg1.reg);

	arg2.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER2_OFS);
	arg2.bit.v4_sfact = v_scl_factor;
	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER2_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scaling_size_p4_reg(UINT32 h_scl_size, UINT32 v_scl_size)
{
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER7 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER7_OFS);

	arg.bit.h4_scl_size = h_scl_size;
	arg.bit.v4_scl_size = v_scl_size;

	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER7_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_scaling_size_p4_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER7 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER7_OFS);

	*p_get_size_h = arg.bit.h4_scl_size;
	*p_get_size_v = arg.bit.v4_scl_size;
}

//-------------------------------------------------------------------------------


VOID ime_set_horizontal_scale_filtering_p4_reg(UINT32 h_scl_filter_en, UINT32 h_scl_filter_coef)
{
#if 0
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS);

	arg.bit.H4_FILTMODE = h_scl_filter_en;
	arg.bit.H4_FILTCOEF = h_scl_filter_coef;

	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------

VOID ime_set_vertical_scale_filtering_p4_reg(UINT32 v_scl_filter_en, UINT32 v_scl_filter_coef)
{
#if 0
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS);

	arg.bit.V4_FILTMODE = v_scl_filter_en;
	arg.bit.V4_FILTCOEF = v_scl_filter_coef;

	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor0_p4_reg(UINT32 isd_scl_h_factor, UINT32 isd_scl_v_factor)
{
#if 0
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER3_OFS);

	arg.bit.ISD_H_SFACT0_P4 = isd_scl_h_factor;
	arg.bit.ISD_V_SFACT0_P4 = isd_scl_v_factor;

	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER3_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor1_p4_reg(UINT32 isd_scl_h_factor, UINT32 isd_scl_v_factor)
{
#if 0
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER7 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER7_OFS);

	arg.bit.ISD_H_SFACT1_P4 = isd_scl_h_factor;
	arg.bit.ISD_V_SFACT1_P4 = isd_scl_v_factor;

	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER7_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor2_p4_reg(UINT32 isd_scl_h_factor, UINT32 isd_scl_v_factor)
{
#if 0
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER8 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER8_OFS);

	arg.bit.ISD_H_SFACT2_P4 = isd_scl_h_factor;
	arg.bit.ISD_V_SFACT2_P4 = isd_scl_v_factor;

	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER8_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------



VOID ime_set_scaling_enhance_factor_p4_reg(UINT32 scl_enh_factor, UINT32 scl_enh_sft_bit)
{
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS);

	arg.bit.ime_scl_enh_fact_p4 = scl_enh_factor;
	arg.bit.ime_scl_enh_bit_p4 = scl_enh_sft_bit;

	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_crop_coordinate_p4_reg(UINT32 crop_pos_x, UINT32 crop_pos_y)
{
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER8 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER8_OFS);

	arg.bit.ime_cropout_x_p4 = crop_pos_x;
	arg.bit.ime_cropout_y_p4 = crop_pos_y;

	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER8_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_output_crop_coordinate_p4_reg(UINT32 *p_get_cpos_x, UINT32 *p_get_cpos_y)
{
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER8 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER8_OFS);

	*p_get_cpos_x = arg.bit.ime_cropout_x_p4;
	*p_get_cpos_y = arg.bit.ime_cropout_y_p4;
}
//-------------------------------------------------------------------------------


VOID ime_set_output_lineoffset_y_p4_reg(UINT32 lofs_y)
{
	T_IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0_OFS);

	arg.bit.dram_ofso_y_p4 = (lofs_y >> 2);

	IME_SETREG(IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_lineoffset_c_p4_reg(UINT32 lofs_c)
{
#if 0 // without chroma channel
	//T_IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER1_OFS);

	//arg.bit.DRAM_OFSO_C_P4 = (uiOutLineOffset >> 2);

	//IME_SETREG(IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER1_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------

VOID ime_set_output_channel_addr0_p4_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr)
{
	T_IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0 arg1;

	ime_p4_out_y_addr = out_addr_y;

	arg1.reg = IME_GETREG(IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0_OFS);
	arg1.bit.dram_sao_p4_y0 = ime_platform_va2pa(ime_p4_out_y_addr);
	IME_SETREG(IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_output_size_p4_reg(UINT32 set_size_h, UINT32 set_size_v)
{
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER9 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER9_OFS);

	arg.bit.h4_osize = set_size_h;
	arg.bit.v4_osize = set_size_v;

	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER9_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_get_output_size_p4_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER9 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER9_OFS);

	*p_get_size_h = arg.bit.h4_osize;
	*p_get_size_v = arg.bit.v4_osize;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_lineoffset_y_p4_reg(VOID)
{
	T_IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_lineoffset_c_p4_reg(VOID)
{
	T_IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_path_enable_status_p4_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_p4_en;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_output_format_p4_reg(VOID)
{
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_omat4;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_format_type_p4_reg(VOID)
{

	T_IME_OUTPUT_PATH4_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_out_type_p4;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_scaling_method_p4_reg(VOID)
{
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_scl_method_p4;
}
//-------------------------------------------------------------------------------

// output path4
UINT32 ime_get_output_addr_y_p4_reg(VOID)
{
	//T_IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0 Path4Info;

	//Path4Info.reg = IME_GETREG(IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0_OFS);

	//return (Path4Info.reg);

	return ime_p4_out_y_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_addr_cb_p4_reg(VOID)
{
	//T_IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0 Path4Info;

	//Path4Info.reg = IME_GETREG(IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0_OFS);

	//return (Path4Info.reg);

	return ime_p4_out_y_addr;
}
//-------------------------------------------------------------------------------

VOID ime_clamp_p4_reg(UINT32 min_y, UINT32 max_y, UINT32 min_uv, UINT32 max_uv)
{
	T_IME_OUTPUT_PATH4_CONTROL_REGISTER10 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER10_OFS);

	arg.bit.ime_y_clamp_min_p4  = min_y;
	arg.bit.ime_y_clamp_max_p4  = max_y;
	arg.bit.ime_uv_clamp_min_p4 = min_uv;
	arg.bit.ime_uv_clamp_max_p4 = max_uv;

	IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER10_OFS, arg.reg);
}

#else


// output path4 APIs
VOID ime_set_output_p4_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_p4_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_flip_enable_p4_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_p4_flip_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------



VOID ime_set_output_dram_enable_p4_reg(UINT32 set_en)
{
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS);

	imeg->reg_108.bit.ime_out_en_p4 = set_en;

	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS, arg.reg);
}

//-------------------------------------------------------------------------------


VOID ime_set_output_image_format_p4_reg(UINT32 out_fmt_sel)
{
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS);

	imeg->reg_108.bit.ime_omat4 = out_fmt_sel;

	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scale_interpolation_method_p4_reg(UINT32 scl_method_sel)
{
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS);

	imeg->reg_108.bit.ime_scl_method_p4 = 0;// only bicubic, //uiscale_methodsel;

	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_type_p4_reg(UINT32 out_fmt_type)
{

	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS);

	imeg->reg_108.bit.ime_out_type_p4 = 1;

	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS, arg.reg);

}
//-------------------------------------------------------------------------------


VOID ime_set_scale_factor_h_p4_reg(UINT32 h_scl_sel, UINT32 h_scl_rate, UINT32 h_scl_factor)
{
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER1 arg1;
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER2 arg2;

	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS);
	imeg->reg_109.bit.h4_ud = h_scl_sel;
	imeg->reg_109.bit.h4_dnrate = h_scl_rate;

	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER2_OFS);
	imeg->reg_110.bit.h4_sfact = h_scl_factor;

	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS, arg1.reg);
	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER2_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scale_factor_v_p4_reg(UINT32 v_scl_sel, UINT32 v_scl_rate, UINT32 v_scl_factor)
{
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER1 arg1;
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER2 arg2;

	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS);
	imeg->reg_109.bit.v4_ud = v_scl_sel;
	imeg->reg_109.bit.v4_dnrate = v_scl_rate;
	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS, arg1.reg);

	//arg2.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER2_OFS);
	imeg->reg_110.bit.v4_sfact = v_scl_factor;
	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER2_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scaling_size_p4_reg(UINT32 h_scl_size, UINT32 v_scl_size)
{
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER7 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER7_OFS);

	imeg->reg_112.bit.h4_scl_size = h_scl_size;
	imeg->reg_112.bit.v4_scl_size = v_scl_size;

	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER7_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_scaling_size_p4_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER7 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER7_OFS);

	*p_get_size_h = imeg->reg_112.bit.h4_scl_size;
	*p_get_size_v = imeg->reg_112.bit.v4_scl_size;
}

//-------------------------------------------------------------------------------


VOID ime_set_horizontal_scale_filtering_p4_reg(UINT32 h_scl_filter_en, UINT32 h_scl_filter_coef)
{
#if 0
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS);

	arg.bit.h4_filtmode = h_scl_filter_en;
	arg.bit.h4_filtcoef = h_scl_filter_coef;

	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------

VOID ime_set_vertical_scale_filtering_p4_reg(UINT32 v_scl_filter_en, UINT32 v_scl_filter_coef)
{
#if 0
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS);

	arg.bit.v4_filtmode = v_scl_filter_en;
	arg.bit.v4_filtcoef = v_scl_filter_coef;

	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor0_p4_reg(UINT32 isd_scl_h_factor, UINT32 isd_scl_v_factor)
{
#if 0
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER3_OFS);

	arg.bit.isd_h_sfact0_p4 = isd_scl_h_factor;
	arg.bit.isd_v_sfact0_p4 = isd_scl_v_factor;

	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER3_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor1_p4_reg(UINT32 isd_scl_h_factor, UINT32 isd_scl_v_factor)
{
#if 0
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER7 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER7_OFS);

	arg.bit.isd_h_sfact1_p4 = isd_scl_h_factor;
	arg.bit.isd_v_sfact1_p4 = isd_scl_v_factor;

	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER7_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor2_p4_reg(UINT32 isd_scl_h_factor, UINT32 isd_scl_v_factor)
{
#if 0
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER8 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER8_OFS);

	arg.bit.isd_h_sfact2_p4 = isd_scl_h_factor;
	arg.bit.isd_v_sfact2_p4 = isd_scl_v_factor;

	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER8_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------



VOID ime_set_scaling_enhance_factor_p4_reg(UINT32 scl_enh_factor, UINT32 scl_enh_sft_bit)
{
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS);

	imeg->reg_108.bit.ime_scl_enh_fact_p4 = scl_enh_factor;
	imeg->reg_108.bit.ime_scl_enh_bit_p4 = scl_enh_sft_bit;

	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_crop_coordinate_p4_reg(UINT32 crop_pos_x, UINT32 crop_pos_y)
{
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER8 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER8_OFS);

	imeg->reg_113.bit.ime_cropout_x_p4 = crop_pos_x;
	imeg->reg_113.bit.ime_cropout_y_p4 = crop_pos_y;

	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER8_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_output_crop_coordinate_p4_reg(UINT32 *p_get_cpos_x, UINT32 *p_get_cpos_y)
{
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER8 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER8_OFS);

	*p_get_cpos_x = imeg->reg_113.bit.ime_cropout_x_p4;
	*p_get_cpos_y = imeg->reg_113.bit.ime_cropout_y_p4;
}
//-------------------------------------------------------------------------------


VOID ime_set_output_lineoffset_y_p4_reg(UINT32 lofs_y)
{
	//T_IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0_OFS);

	imeg->reg_116.bit.dram_ofso_y_p4 = (lofs_y >> 2);

	//IME_SETREG(IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_lineoffset_c_p4_reg(UINT32 lofs_c)
{
#if 0 // without chroma channel
	//T_IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER1_OFS);

	//arg.bit.dram_ofso_c_p4 = (uioutlineoffset >> 2);

	//IME_SETREG(IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER1_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------

VOID ime_set_output_channel_addr0_p4_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr)
{
	//T_IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0 arg1;

	ime_p4_out_y_addr = out_addr_y;

	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0_OFS);
	imeg->reg_118.bit.dram_sao_p4_y0 = ime_platform_va2pa(ime_p4_out_y_addr);
	//IME_SETREG(IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_output_size_p4_reg(UINT32 set_size_h, UINT32 set_size_v)
{
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER9 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER9_OFS);

	imeg->reg_114.bit.h4_osize = set_size_h;
	imeg->reg_114.bit.v4_osize = set_size_v;

	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER9_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_get_output_size_p4_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER9 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER9_OFS);

	*p_get_size_h = imeg->reg_114.bit.h4_osize;
	*p_get_size_v = imeg->reg_114.bit.v4_osize;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_lineoffset_y_p4_reg(VOID)
{
	//T_IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0_OFS);

	return imeg->reg_116.word;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_lineoffset_c_p4_reg(VOID)
{
	//T_IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0_OFS);

	return imeg->reg_116.word;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_path_enable_status_p4_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return imeg->reg_1.bit.ime_p4_en;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_output_format_p4_reg(VOID)
{
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS);

	return imeg->reg_108.bit.ime_omat4;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_format_type_p4_reg(VOID)
{

	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS);

	return imeg->reg_108.bit.ime_out_type_p4;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_scaling_method_p4_reg(VOID)
{
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS);

	return imeg->reg_108.bit.ime_scl_method_p4;
}
//-------------------------------------------------------------------------------

// output path4
UINT32 ime_get_output_addr_y_p4_reg(VOID)
{
	//T_IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0 Path4Info;

	//Path4Info.reg = IME_GETREG(IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0_OFS);

	//return (Path4Info.reg);

	return ime_p4_out_y_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_addr_cb_p4_reg(VOID)
{
	//T_IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0 Path4Info;

	//Path4Info.reg = IME_GETREG(IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0_OFS);

	//return (Path4Info.reg);

	return ime_p4_out_y_addr;
}
//-------------------------------------------------------------------------------

VOID ime_clamp_p4_reg(UINT32 min_y, UINT32 max_y, UINT32 min_uv, UINT32 max_uv)
{
	//T_IME_OUTPUT_PATH4_CONTROL_REGISTER10 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER10_OFS);

	imeg->reg_115.bit.ime_y_clamp_min_p4  = min_y;
	imeg->reg_115.bit.ime_y_clamp_max_p4  = max_y;
	//imeg->reg_115.bit.ime_uv_clamp_min_p4 = min_uv;
	//imeg->reg_115.bit.ime_uv_clamp_max_p4 = max_uv;

	//IME_SETREG(IME_OUTPUT_PATH4_CONTROL_REGISTER10_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


#endif

