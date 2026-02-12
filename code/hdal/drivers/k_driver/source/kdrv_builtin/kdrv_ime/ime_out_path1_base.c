
/*
    IME module driver

    NT96510 IME module driver.

    @file       ime_out_path1_base.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2017  All rights reserved.
*/


#include "ime_reg.h"
#include "ime_int_public.h"

#include "ime_out_path1_base.h"



#if 0
#include "Type.h"

#include "ime_reg.h"
#include "ime_int_public.h"

#include "ime_out_path1_base.h"
#endif


UINT32 ime_p1_out_y_addr = 0, ime_p1_out_u_addr = 0, ime_p1_out_v_addr = 0;

UINT32 ime_p1_out_stitching_y_addr = 0x0;
UINT32 ime_p1_out_stitching_u_addr = 0x0;

#if (IPP_DRIVER_NEW_REG_HANDLE == DISABLE)

VOID ime_open_output_p1_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_p1_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_flip_enable_p1_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_p1_flip_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_dram_enable_p1_reg(UINT32 set_en)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	arg.bit.ime_out_en_p1 = set_en;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS, arg.reg);
}

//-------------------------------------------------------------------------------

VOID ime_set_output_dest_p1_reg(UINT32 dst)
{
#if 0

	T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	arg.bit.IME_OUT_DEST_P1 = dst;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS, arg.reg);

#endif
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_dest_p1_reg(VOID)
{
#if 0

	T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	return arg.bit.IME_OUT_DEST_P1;

#endif

	return 0;
}
//-------------------------------------------------------------------------------

VOID ime_set_scale_interpolation_method_p1_reg(UINT32 scale_method_sel)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	arg.bit.ime_scl_method_p1 = scale_method_sel;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_type_p1_reg(UINT32 out_fmt_type)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER0  arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	arg.bit.ime_out_type_p1 = out_fmt_type;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_lineoffset_y_p1_reg(UINT32 lofs_y)
{
	T_IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER0_OFS);

	arg.bit.dram_ofso_p1_y0 = (lofs_y >> 2);

	IME_SETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_lineoffset_c_p1_reg(UINT32 lofs)
{
	T_IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER1_OFS);

	arg.bit.dram_ofso_p1_uv0 = (lofs >> 2);

	IME_SETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_output_channel_addr_p1_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr)
{
	T_IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER0 arg_y;
	T_IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER1 arg_cb;
	T_IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER2 arg_cr;

	ime_p1_out_y_addr = out_addr_y;
	ime_p1_out_u_addr = out_addr_cb;
	ime_p1_out_v_addr = out_addr_cr;

	arg_y.reg  = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER0_OFS);
	arg_cb.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER1_OFS);
	arg_cr.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER2_OFS);

	arg_y.bit.dram_sao_p1_y0  = ime_platform_va2pa(ime_p1_out_y_addr);
	arg_cb.bit.dram_sao_p1_u0 = ime_platform_va2pa(ime_p1_out_u_addr);;
	arg_cr.bit.dram_sao_p1_v0 = ime_platform_va2pa(ime_p1_out_v_addr);;

	IME_SETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER0_OFS, arg_y.reg);
	IME_SETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER1_OFS, arg_cb.reg);
	IME_SETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER2_OFS, arg_cr.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_horizontal_scale_filtering_p1_reg(UINT32 set_en, UINT32 set_coef)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS);

#if 0
	arg.bit.h1_filtmode = 0;
	arg.bit.h1_filtcoef = 0;
#else
	arg.bit.h1_filtmode = set_en;
	arg.bit.h1_filtcoef = set_coef;
#endif

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_vertical_scale_filtering_p1_reg(UINT32 set_en, UINT32 set_coef)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS);

#if 0
	arg.bit.v1_filtmode = 0;
	arg.bit.v1_filtcoef = 0;
#else
	arg.bit.v1_filtmode = set_en;
	arg.bit.v1_filtcoef = set_coef;
#endif

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor_base_p1_reg(UINT32 base_scale_h_factor, UINT32 base_scale_v_factor)
{
#if 0
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER3_OFS);

	arg.bit.ISD_H_BASE_P1 = base_scale_h_factor;
	arg.bit.ISD_V_BASE_P1 = base_scale_v_factor;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER3_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------


VOID ime_set_integration_scaling_factor0_p1_reg(UINT32 integration_scale_h_factor, UINT32 integration_scale_v_factor)
{
#if 0
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER4 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER4_OFS);

	arg.bit.ISD_H_SFACT0_P1 = integration_scale_h_factor;
	arg.bit.ISD_V_SFACT0_P1 = integration_scale_v_factor;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER4_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor1_p1_reg(UINT32 integration_scale_h_factor, UINT32 integration_scale_v_factor)
{
#if 0
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER5 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER5_OFS);

	arg.bit.ISD_H_SFACT1_P1 = integration_scale_h_factor;
	arg.bit.ISD_V_SFACT1_P1 = integration_scale_v_factor;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER5_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor2_p1_reg(UINT32 integration_scale_h_factor, UINT32 integration_scale_v_factor)
{
#if 0
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER6 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER6_OFS);

	arg.bit.ISD_H_SFACT2_P1 = integration_scale_h_factor;
	arg.bit.ISD_V_SFACT2_P1 = integration_scale_v_factor;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER6_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------


VOID ime_set_scaling_enhance_factor_p1_reg(UINT32 scl_enh_ftr, UINT32 scl_enh_shf)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	arg.bit.ime_scl_enh_fact_p1 = scl_enh_ftr;
	arg.bit.ime_scl_enh_bit_p1 = scl_enh_shf;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_scale_factor_h_p1_reg(UINT32 h_scale_sel, UINT32 h_scale_rate, UINT32 h_scale_factor)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER1 hscalectrl1;
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER2 hscalectrl2;

	hscalectrl1.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS);
	hscalectrl2.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER2_OFS);

	hscalectrl1.bit.h1_ud     = h_scale_sel;
	hscalectrl1.bit.h1_dnrate = h_scale_rate;
	hscalectrl2.bit.h1_sfact  = h_scale_factor;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS, hscalectrl1.reg);
	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER2_OFS, hscalectrl2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scaling_size_p1_reg(UINT32 h_scale_size, UINT32 v_scale_size)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER7 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER7_OFS);

	arg.bit.h1_scl_size = h_scale_size;
	arg.bit.v1_scl_size = v_scale_size;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER7_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_scaling_size_p1_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER7 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER7_OFS);

	*p_get_size_h = arg.bit.h1_scl_size;
	*p_get_size_v = arg.bit.v1_scl_size;
}
//-------------------------------------------------------------------------------


VOID ime_set_scale_factor_v_p1_reg(UINT32 v_scale_sle, UINT32 v_scale_rate, UINT32 v_scale_factor)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER1 vscalectrl1;
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER2 vscalectrl2;

	vscalectrl1.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS);
	vscalectrl2.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER2_OFS);

	vscalectrl1.bit.v1_ud     = v_scale_sle;
	vscalectrl1.bit.v1_dnrate = v_scale_rate;
	vscalectrl2.bit.v1_sfact  = v_scale_factor;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS, vscalectrl1.reg);
	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER2_OFS, vscalectrl2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_image_format_p1_reg(UINT32 out_format_sel)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 path1;

	path1.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	path1.bit.ime_omat1 = out_format_sel;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS, path1.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_crop_coordinate_p1_reg(UINT32 crop_pos_x, UINT32 crop_pos_y)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER8 path1_crop;

	path1_crop.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER8_OFS);

	path1_crop.bit.ime_cropout_x_p1 = crop_pos_x;
	path1_crop.bit.ime_cropout_y_p1 = crop_pos_y;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER8_OFS, path1_crop.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_output_crop_coordinate_p1_reg(UINT32 *p_get_crop_pos_x, UINT32 *p_get_crop_pos_y)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER8 path1_crop;

	path1_crop.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER8_OFS);

	*p_get_crop_pos_x = path1_crop.bit.ime_cropout_x_p1;
	*p_get_crop_pos_y = path1_crop.bit.ime_cropout_y_p1;
}
//-------------------------------------------------------------------------------


VOID ime_set_output_size_p1_reg(UINT32 set_size_h, UINT32 set_size_v)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER9 out_size_info;

	out_size_info.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER9_OFS);

	out_size_info.bit.h1_osize = set_size_h;
	out_size_info.bit.v1_osize = set_size_v;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER9_OFS, out_size_info.reg);
}
//-------------------------------------------------------------------------------


VOID ime_get_output_size_p1_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER9 out_size_info;

	out_size_info.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER9_OFS);

	*p_get_size_h = out_size_info.bit.h1_osize;
	*p_get_size_v = out_size_info.bit.v1_osize;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_lineoffset_y_p1_reg(VOID)
{
	T_IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER0 lofs_y;

	lofs_y.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER0_OFS);

	return lofs_y.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_lineoffset_c_p1_reg(VOID)
{
	T_IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER1 lofs_cb;

	lofs_cb.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER1_OFS);

	return lofs_cb.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_path_enable_status_p1_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 get_en;

	get_en.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return get_en.bit.ime_p1_en;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_output_format_p1_reg(VOID)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 out_fmt;

	out_fmt.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	return out_fmt.bit.ime_omat1;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_format_type_p1_reg(VOID)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 out_fmt_type;

	out_fmt_type.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	return out_fmt_type.bit.ime_out_type_p1;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_scaling_method_p1_reg(VOID)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_scl_method_p1;
}

//-------------------------------------------------------------------------------

UINT32 ime_get_output_addr_y_p1_reg(VOID)
{
	//T_IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER0 AddrRegY;

	//AddrRegY.reg  = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER0_OFS);

	//return (AddrRegY.reg);

	return ime_p1_out_y_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_addr_cb_p1_reg(VOID)
{
	//T_IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER1 AddrRegCb;

	//AddrRegCb.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER1_OFS);

	//return (AddrRegCb.reg);

	return ime_p1_out_u_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_addr_cr_p1_reg(VOID)
{
	//T_IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER2 AddrRegCr;

	//AddrRegCr.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER2_OFS);

	//return (AddrRegCr.reg);

	return ime_p1_out_v_addr;
}
//-------------------------------------------------------------------------------

VOID ime_set_encode_enable_p1_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_p1_enc_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_encode_enable_p1_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_p1_enc_en;
}
//-------------------------------------------------------------------------------

VOID ime_clamp_p1_reg(UINT32 min_y, UINT32 max_y, UINT32 min_uv, UINT32 max_uv)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER10 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER10_OFS);

	arg.bit.ime_y_clamp_min_p1  = min_y;
	arg.bit.ime_y_clamp_max_p1  = max_y;
	arg.bit.ime_uv_clamp_min_p1 = min_uv;
	arg.bit.ime_uv_clamp_max_p1 = max_uv;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER10_OFS, arg.reg);
}

//-------------------------------------------------------------------------------
// stitching
// stitching - path1
VOID ime_set_stitching_enable_p1_reg(UINT32 set_en)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	arg.bit.ime_sprt_out_en_p1 = set_en;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_stitching_base_position_p1_reg(UINT32 base_pos)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER11 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER11_OFS);

	arg.bit.ime_sprt_hbl_p1 = base_pos;

	IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER11_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


UINT32 ime_get_stitching_enable_p1_reg(VOID)
{
	T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_sprt_out_en_p1;
}
//-------------------------------------------------------------------------------

VOID ime_set_stitching_output_lineoffset_y2_p1_reg(UINT32 lofs)
{
	T_IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER2 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER2_OFS);

	arg.bit.dram_ofso_p1_y1 = (lofs >> 2);

	IME_SETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER2_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_stitching_output_lineoffset_c2_p1_reg(UINT32 lofs)
{
	T_IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER3_OFS);

	arg.bit.dram_ofso_p1_uv1 = (lofs >> 2);

	IME_SETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_lineoffset_y2_p1_reg(VOID)
{
	T_IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER2 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER2_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------

extern UINT32 ime_get_stitching_output_lineoffset_c2_p1_reg(VOID)
{
	T_IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER3_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------


VOID ime_set_stitching_output_channel_addr2_p1_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr)
{
	T_IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER3 arg0;
	T_IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER4 arg1;

	ime_p1_out_stitching_y_addr = out_addr_y;
	ime_p1_out_stitching_u_addr = out_addr_cb;

	arg0.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER3_OFS);
	arg1.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER4_OFS);

	arg0.bit.dram_sao_p1_y1 = ime_platform_va2pa(ime_p1_out_stitching_y_addr);//uioutaddry;
	arg1.bit.dram_sao_p1_uv1 = ime_platform_va2pa(ime_p1_out_stitching_u_addr);//uioutaddrcb;

	IME_SETREG(IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER3_OFS, arg0.reg);
	IME_SETREG(IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER4_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_addr_y2_p1_reg(VOID)
{
	//T_IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER3_OFS);

	//return arg.reg;

	return ime_p1_out_stitching_y_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_addr_cb2_p1_reg(VOID)
{
	//T_IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER4 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER4_OFS);

	//return arg.reg;

	return ime_p1_out_stitching_u_addr;
}

#else


VOID ime_open_output_p1_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_p1_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_flip_enable_p1_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_p1_flip_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_dram_enable_p1_reg(UINT32 set_en)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	imeg->reg_24.bit.ime_out_en_p1 = set_en;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS, arg.reg);
}

//-------------------------------------------------------------------------------

VOID ime_set_output_dest_p1_reg(UINT32 dst)
{
#if 0

	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	arg.bit.ime_out_dest_p1 = dst;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS, arg.reg);

#endif
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_dest_p1_reg(VOID)
{
#if 0

	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_out_dest_p1;

#endif

	return 0;
}
//-------------------------------------------------------------------------------

VOID ime_set_scale_interpolation_method_p1_reg(UINT32 scale_method_sel)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	imeg->reg_24.bit.ime_scl_method_p1 = scale_method_sel;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_type_p1_reg(UINT32 out_fmt_type)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER0  arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	imeg->reg_24.bit.ime_out_type_p1 = out_fmt_type;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_lineoffset_y_p1_reg(UINT32 lofs_y)
{
	//T_IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER0_OFS);

	imeg->reg_35.bit.dram_ofso_p1_y0 = (lofs_y >> 2);

	//IME_SETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_lineoffset_c_p1_reg(UINT32 lofs)
{
	//T_IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER1_OFS);

	imeg->reg_36.bit.dram_ofso_p1_uv0 = (lofs >> 2);

	//IME_SETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_output_channel_addr_p1_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr)
{
	//T_IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER0 arg_y;
	//T_IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER1 arg_cb;
	//T_IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER2 arg_cr;

	ime_p1_out_y_addr = out_addr_y;
	ime_p1_out_u_addr = out_addr_cb;
	ime_p1_out_v_addr = out_addr_cr;

	//arg_y.reg  = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER0_OFS);
	//arg_cb.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER1_OFS);
	//arg_cr.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER2_OFS);

	imeg->reg_37.bit.dram_sao_p1_y0  = ime_platform_va2pa(ime_p1_out_y_addr);
	imeg->reg_38.bit.dram_sao_p1_u0 = ime_platform_va2pa(ime_p1_out_u_addr);;
	imeg->reg_39.bit.dram_sao_p1_v0 = ime_platform_va2pa(ime_p1_out_v_addr);;

	//IME_SETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER0_OFS, arg_y.reg);
	//IME_SETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER1_OFS, arg_cb.reg);
	//IME_SETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER2_OFS, arg_cr.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_horizontal_scale_filtering_p1_reg(UINT32 set_en, UINT32 set_coef)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS);

#if 0
	imeg->reg_25.bit.h1_filtmode = 0;
	imeg->reg_25.bit.h1_filtcoef = 0;
#else
	imeg->reg_25.bit.h1_filtmode = set_en;
	imeg->reg_25.bit.h1_filtcoef = set_coef;
#endif

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_vertical_scale_filtering_p1_reg(UINT32 set_en, UINT32 set_coef)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS);

#if 0
	imeg->reg_25.bit.v1_filtmode = 0;
	imeg->reg_25.bit.v1_filtcoef = 0;
#else
	imeg->reg_25.bit.v1_filtmode = set_en;
	imeg->reg_25.bit.v1_filtcoef = set_coef;
#endif

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor_base_p1_reg(UINT32 base_scale_h_factor, UINT32 base_scale_v_factor)
{
#if 0
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER3_OFS);

	arg.bit.isd_h_base_p1 = base_scale_h_factor;
	arg.bit.isd_v_base_p1 = base_scale_v_factor;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER3_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------


VOID ime_set_integration_scaling_factor0_p1_reg(UINT32 integration_scale_h_factor, UINT32 integration_scale_v_factor)
{
#if 0
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER4 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER4_OFS);

	arg.bit.isd_h_sfact0_p1 = integration_scale_h_factor;
	arg.bit.isd_v_sfact0_p1 = integration_scale_v_factor;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER4_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor1_p1_reg(UINT32 integration_scale_h_factor, UINT32 integration_scale_v_factor)
{
#if 0
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER5 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER5_OFS);

	arg.bit.isd_h_sfact1_p1 = integration_scale_h_factor;
	arg.bit.isd_v_sfact1_p1 = integration_scale_v_factor;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER5_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------

VOID ime_set_integration_scaling_factor2_p1_reg(UINT32 integration_scale_h_factor, UINT32 integration_scale_v_factor)
{
#if 0
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER6 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER6_OFS);

	arg.bit.isd_h_sfact2_p1 = integration_scale_h_factor;
	arg.bit.isd_v_sfact2_p1 = integration_scale_v_factor;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER6_OFS, arg.reg);
#endif
}
//-------------------------------------------------------------------------------


VOID ime_set_scaling_enhance_factor_p1_reg(UINT32 scl_enh_ftr, UINT32 scl_enh_shf)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	imeg->reg_24.bit.ime_scl_enh_fact_p1 = scl_enh_ftr;
	imeg->reg_24.bit.ime_scl_enh_bit_p1 = scl_enh_shf;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_scale_factor_h_p1_reg(UINT32 h_scale_sel, UINT32 h_scale_rate, UINT32 h_scale_factor)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER1 hscalectrl1;
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER2 hscalectrl2;

	//hscalectrl1.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS);
	//hscalectrl2.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER2_OFS);

	imeg->reg_25.bit.h1_ud     = h_scale_sel;
	imeg->reg_25.bit.h1_dnrate = h_scale_rate;
	imeg->reg_26.bit.h1_sfact  = h_scale_factor;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS, hscalectrl1.reg);
	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER2_OFS, hscalectrl2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_scaling_size_p1_reg(UINT32 h_scale_size, UINT32 v_scale_size)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER7 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER7_OFS);

	imeg->reg_31.bit.h1_scl_size = h_scale_size;
	imeg->reg_31.bit.v1_scl_size = v_scale_size;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER7_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_scaling_size_p1_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER7 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER7_OFS);

	*p_get_size_h = imeg->reg_31.bit.h1_scl_size;
	*p_get_size_v = imeg->reg_31.bit.v1_scl_size;
}
//-------------------------------------------------------------------------------


VOID ime_set_scale_factor_v_p1_reg(UINT32 v_scale_sle, UINT32 v_scale_rate, UINT32 v_scale_factor)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER1 vscalectrl1;
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER2 vscalectrl2;

	//vscalectrl1.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS);
	//vscalectrl2.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER2_OFS);

	imeg->reg_25.bit.v1_ud     = v_scale_sle;
	imeg->reg_25.bit.v1_dnrate = v_scale_rate;
	imeg->reg_26.bit.v1_sfact  = v_scale_factor;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS, vscalectrl1.reg);
	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER2_OFS, vscalectrl2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_image_format_p1_reg(UINT32 out_format_sel)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 path1;

	//path1.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	imeg->reg_24.bit.ime_omat1 = out_format_sel;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS, path1.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_crop_coordinate_p1_reg(UINT32 crop_pos_x, UINT32 crop_pos_y)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER8 path1_crop;

	//path1_crop.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER8_OFS);

	imeg->reg_32.bit.ime_cropout_x_p1 = crop_pos_x;
	imeg->reg_32.bit.ime_cropout_y_p1 = crop_pos_y;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER8_OFS, path1_crop.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_output_crop_coordinate_p1_reg(UINT32 *p_get_crop_pos_x, UINT32 *p_get_crop_pos_y)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER8 path1_crop;

	//path1_crop.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER8_OFS);

	*p_get_crop_pos_x = imeg->reg_32.bit.ime_cropout_x_p1;
	*p_get_crop_pos_y = imeg->reg_32.bit.ime_cropout_y_p1;
}
//-------------------------------------------------------------------------------


VOID ime_set_output_size_p1_reg(UINT32 set_size_h, UINT32 set_size_v)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER9 out_size_info;

	//out_size_info.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER9_OFS);

	imeg->reg_33.bit.h1_osize = set_size_h;
	imeg->reg_33.bit.v1_osize = set_size_v;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER9_OFS, out_size_info.reg);
}
//-------------------------------------------------------------------------------


VOID ime_get_output_size_p1_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER9 out_size_info;

	//out_size_info.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER9_OFS);

	*p_get_size_h = imeg->reg_33.bit.h1_osize;
	*p_get_size_v = imeg->reg_33.bit.v1_osize;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_lineoffset_y_p1_reg(VOID)
{
	T_IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER0 lofs_y;

	lofs_y.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER0_OFS);

	return lofs_y.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_lineoffset_c_p1_reg(VOID)
{
	T_IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER1 lofs_cb;

	lofs_cb.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER1_OFS);

	return lofs_cb.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_path_enable_status_p1_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 get_en;

	//get_en.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return imeg->reg_1.bit.ime_p1_en;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_output_format_p1_reg(VOID)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 out_fmt;

	//out_fmt.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	return imeg->reg_24.bit.ime_omat1;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_format_type_p1_reg(VOID)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 out_fmt_type;

	//out_fmt_type.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	return imeg->reg_24.bit.ime_out_type_p1;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_scaling_method_p1_reg(VOID)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	return imeg->reg_24.bit.ime_scl_method_p1;
}

//-------------------------------------------------------------------------------

UINT32 ime_get_output_addr_y_p1_reg(VOID)
{
	//T_IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER0 AddrRegY;

	//AddrRegY.reg  = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER0_OFS);

	//return (AddrRegY.reg);

	return ime_p1_out_y_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_addr_cb_p1_reg(VOID)
{
	//T_IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER1 AddrRegCb;

	//AddrRegCb.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER1_OFS);

	//return (AddrRegCb.reg);

	return ime_p1_out_u_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_output_addr_cr_p1_reg(VOID)
{
	//T_IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER2 AddrRegCr;

	//AddrRegCr.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER2_OFS);

	//return (AddrRegCr.reg);

	return ime_p1_out_v_addr;
}
//-------------------------------------------------------------------------------

VOID ime_set_encode_enable_p1_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_p1_enc_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_encode_enable_p1_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return imeg->reg_1.bit.ime_p1_enc_en;
}
//-------------------------------------------------------------------------------

VOID ime_clamp_p1_reg(UINT32 min_y, UINT32 max_y, UINT32 min_uv, UINT32 max_uv)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER10 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER10_OFS);

	imeg->reg_34.bit.ime_y_clamp_min_p1  = min_y;
	imeg->reg_34.bit.ime_y_clamp_max_p1  = max_y;
	imeg->reg_34.bit.ime_uv_clamp_min_p1 = min_uv;
	imeg->reg_34.bit.ime_uv_clamp_max_p1 = max_uv;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER10_OFS, arg.reg);
}

//-------------------------------------------------------------------------------
// stitching
// stitching - path1
VOID ime_set_stitching_enable_p1_reg(UINT32 set_en)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	imeg->reg_24.bit.ime_sprt_out_en_p1 = set_en;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_stitching_base_position_p1_reg(UINT32 base_pos)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER11 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER11_OFS);

	imeg->reg_240.bit.ime_sprt_hbl_p1 = base_pos;

	//IME_SETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER11_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


UINT32 ime_get_stitching_enable_p1_reg(VOID)
{
	//T_IME_OUTPUT_PATH1_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS);

	return imeg->reg_24.bit.ime_sprt_out_en_p1;
}
//-------------------------------------------------------------------------------

VOID ime_set_stitching_output_lineoffset_y2_p1_reg(UINT32 lofs)
{
	//T_IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER2 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER2_OFS);

	imeg->reg_241.bit.dram_ofso_p1_y1 = (lofs >> 2);

	//IME_SETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER2_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_stitching_output_lineoffset_c2_p1_reg(UINT32 lofs)
{
	//T_IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER3_OFS);

	imeg->reg_242.bit.dram_ofso_p1_uv1 = (lofs >> 2);

	//IME_SETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_lineoffset_y2_p1_reg(VOID)
{
	//T_IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER2 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER2_OFS);

	return imeg->reg_241.word;
}
//-------------------------------------------------------------------------------

extern UINT32 ime_get_stitching_output_lineoffset_c2_p1_reg(VOID)
{
	//T_IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER3_OFS);

	return imeg->reg_242.word;
}
//-------------------------------------------------------------------------------


VOID ime_set_stitching_output_channel_addr2_p1_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr)
{
	//T_IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER3 arg0;
	//T_IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER4 arg1;

	ime_p1_out_stitching_y_addr = out_addr_y;
	ime_p1_out_stitching_u_addr = out_addr_cb;

	//arg0.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER3_OFS);
	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER4_OFS);

	imeg->reg_243.bit.dram_sao_p1_y1 = ime_platform_va2pa(ime_p1_out_stitching_y_addr);//uioutaddry;
	imeg->reg_244.bit.dram_sao_p1_uv1 = ime_platform_va2pa(ime_p1_out_stitching_u_addr);//uioutaddrcb;

	//IME_SETREG(IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER3_OFS, arg0.reg);
	//IME_SETREG(IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER4_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_addr_y2_p1_reg(VOID)
{
	//T_IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER3_OFS);

	//return arg.reg;

	return ime_p1_out_stitching_y_addr;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_addr_cb2_p1_reg(VOID)
{
	//T_IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER4 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER4_OFS);

	//return arg.reg;

	return ime_p1_out_stitching_u_addr;
}



#endif

