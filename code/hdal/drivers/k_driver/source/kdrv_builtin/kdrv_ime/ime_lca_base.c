/*
    IME module driver

    NT96520 IME module driver.

    @file       ime_lca_base.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/


#include "ime_reg.h"
#include "ime_int_public.h"

#include "ime_lca_base.h"

static UINT32 ime_lca_in_addr = 0x0;
static UINT32 ime_lca_subout_addr = 0x0;

#if (IPP_DRIVER_NEW_REG_HANDLE == DISABLE)

VOID ime_set_lca_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_chra_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_bypass_enable_reg(UINT32 set_en)
{
	T_IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS);

	arg.bit.chra_bypass = set_en;

	IME_SETREG(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


UINT32 ime_get_lca_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_chra_en;
}
//-------------------------------------------------------------------------------


VOID ime_set_lca_chroma_adj_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_chra_ca_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_la_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_chra_la_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------
#if 0

VOID ime_set_lca_enable_ppb_reg(UINT32 set_en)
{

	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.IME_CHRA_IPPB_EN = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
#endif

//-------------------------------------------------------------------------------

VOID ime_set_lca_image_size_reg(UINT32 size_h, UINT32 size_v)
{
	T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER0_OFS);

	arg.bit.chra_h_size = size_h;
	arg.bit.chra_v_size = size_v;

	IME_SETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_lca_image_size_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER0_OFS);

	*p_get_size_h = arg.bit.chra_h_size;
	*p_get_size_v = arg.bit.chra_v_size;
}
//-------------------------------------------------------------------------------


VOID ime_set_lca_scale_factor_reg(UINT32 factor_h, UINT32 factor_v)
{
	T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER1_OFS);

	arg.bit.chra_h_sfact = factor_h;
	arg.bit.chra_v_sfact = factor_v;

	IME_SETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_lineoffset_reg(UINT32 lofs_y, UINT32 lofs_c)
{
	T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER2 arg_y;
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER5 argC;

	arg_y.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER2_OFS);
	//argC.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER5_OFS);

	arg_y.bit.chra_dram_y_ofsi = (lofs_y >> 2);
	//argC.bit.CHRA_DRAM_C_OFSI = (uiLofsC >> 2);

	IME_SETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER2_OFS, arg_y.reg);
	//IME_SETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER5_OFS, argC.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_lca_lineoffset_reg(UINT32 *p_get_lofs_y, UINT32 *p_get_lofs_c)
{
	T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER2 arg_y;
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER5 argC;

	arg_y.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER2_OFS);
	//argC.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER5_OFS);

	*p_get_lofs_y = (arg_y.bit.chra_dram_y_ofsi << 2);
	*p_get_lofs_c = (arg_y.bit.chra_dram_y_ofsi << 2);
}
//-------------------------------------------------------------------------------


VOID ime_set_lca_dma_addr0_reg(UINT32 addr_y, UINT32 addr_c)
{
	T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3 arg_y;
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER6 argC;

	ime_lca_in_addr = addr_y;

	arg_y.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3_OFS);
	//argC.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER6_OFS);

	arg_y.bit.chra_dram_y_sai0 = ime_platform_va2pa(ime_lca_in_addr) >> 2;//(uiaddry >> 2);
	//argC.bit.CHRA_DRAM_C_SAI0 = (uiAddrC >> 2);

	IME_SETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3_OFS, arg_y.reg);
	//IME_SETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER6_OFS, argC.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_lca_dma_addr0_reg(UINT32 *p_get_addr_y, UINT32 *p_get_addr_c)
{
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3 arg_y;
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER6 arg_c;

	//arg_y.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3_OFS);
	//arg_c.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER6_OFS);

	*p_get_addr_y = ime_lca_in_addr;//(arg_y.bit.chra_dram_y_sai0 << 2);
	*p_get_addr_c = ime_lca_in_addr;//(arg_y.bit.chra_dram_y_sai0 << 2);
}
//-------------------------------------------------------------------------------


VOID ime_set_lca_dma_addr1_reg(UINT32 addr_y, UINT32 addr_c)
{
	T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3 arg_y;
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER7 argC;

	ime_lca_in_addr = addr_y;

	arg_y.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3_OFS);
	//argC.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER7_OFS);

	arg_y.bit.chra_dram_y_sai0 = ime_platform_va2pa(ime_lca_in_addr) >> 2;//(uiaddry >> 2);
	//argC.bit.CHRA_DRAM_C_SAI1 = (uiAddrC >> 2);

	IME_SETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3_OFS, arg_y.reg);
	//IME_SETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER7_OFS, argC.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_lca_dma_addr1_reg(UINT32 *p_get_addr_y, UINT32 *p_get_addr_c)
{
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3 arg_y;
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER7 arg_c;

	//arg_y.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3_OFS);
	//arg_c.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER7_OFS);

	*p_get_addr_y = ime_lca_in_addr;//(arg_y.bit.chra_dram_y_sai0 << 2);
	*p_get_addr_c = ime_lca_in_addr;//(arg_y.bit.chra_dram_y_sai0 << 2);
}
//-------------------------------------------------------------------------------

#if 0
UINT32 ime_get_lca_active_ppb_id_reg(VOID)
{
#if 0
	T_IME_CHROMA_ADAPTATION_PPB_STATUS_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_PPB_STATUS_REGISTER0_OFS);

	return arg.bit.CHRA_ACTPPB_ID;
#endif

	return 0;
}
#endif
//-------------------------------------------------------------------------------

VOID ime_set_lca_format_reg(UINT32 set_fmt)
{
	T_IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS);

	arg.bit.chra_fmt = 0;

	IME_SETREG(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_lca_format_reg(VOID)
{
	T_IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS);

	return arg.bit.chra_fmt;
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_source_reg(UINT32 set_src)
{
	T_IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS);

	arg.bit.chra_src = set_src;

	IME_SETREG(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_lca_ca_adjust_center_reg(UINT32 u, UINT32 v)
{
	T_IME_CHROMA_ADJUSTMENT_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADJUSTMENT_REGISTER0_OFS);

	arg.bit.chra_ca_ctr_u = u;
	arg.bit.chra_ca_ctr_v = v;

	IME_SETREG(IME_CHROMA_ADJUSTMENT_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_ca_range_reg(UINT32 rng, UINT32 wet_prc)
{
	T_IME_CHROMA_ADJUSTMENT_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADJUSTMENT_REGISTER1_OFS);

	arg.bit.chra_ca_rng = rng;
	arg.bit.chra_ca_wtprc = wet_prc;

	IME_SETREG(IME_CHROMA_ADJUSTMENT_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

extern VOID ime_set_lca_ca_weight_reg(UINT32 th, UINT32 wt_s, UINT32 wt_e)
{
	T_IME_CHROMA_ADJUSTMENT_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADJUSTMENT_REGISTER1_OFS);

	arg.bit.chra_ca_th = th;
	arg.bit.chra_ca_wts = wt_s;
	arg.bit.chra_ca_wte = wt_e;

	IME_SETREG(IME_CHROMA_ADJUSTMENT_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_chroma_ref_image_weight_reg(UINT32 ref_y_wt, UINT32 ref_c_wt, UINT32 out_wt)
{
	T_IME_CHROMA_ADAPTATION_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_REGISTER0_OFS);

	arg.bit.chra_refy_wt = ref_y_wt;
	arg.bit.chra_refc_wt = ref_c_wt;
	arg.bit.chra_out_wt = out_wt;

	IME_SETREG(IME_CHROMA_ADAPTATION_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_luma_ref_image_weight_reg(UINT32 ref_y_wt, UINT32 out_wt)
{
	T_IME_CHROMA_ADAPTATION_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_REGISTER0_OFS);

	arg.bit.luma_refy_wt = ref_y_wt;
	arg.bit.luma_out_wt = out_wt;

	IME_SETREG(IME_CHROMA_ADAPTATION_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_chroma_range_y_reg(UINT32 rng, UINT32 wet_prc)
{
	T_IME_CHROMA_ADAPTATION_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_REGISTER1_OFS);

	arg.bit.chra_y_rng = rng;
	arg.bit.chra_y_wtprc = wet_prc;

	IME_SETREG(IME_CHROMA_ADAPTATION_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_chroma_weight_y_reg(UINT32 th, UINT32 wt_s, UINT32 wt_e)
{
	T_IME_CHROMA_ADAPTATION_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_REGISTER1_OFS);

	arg.bit.chra_y_th = th;
	arg.bit.chra_y_wts = wt_s;
	arg.bit.chra_y_wte = wt_e;

	IME_SETREG(IME_CHROMA_ADAPTATION_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_chroma_range_uv_reg(UINT32 rng, UINT32 wt_prc)
{
	T_IME_CHROMA_ADAPTATION_REGISTER2 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_REGISTER2_OFS);

	arg.bit.chra_uv_rng = rng;
	arg.bit.chra_uv_wtprc = wt_prc;

	IME_SETREG(IME_CHROMA_ADAPTATION_REGISTER2_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_chroma_weight_uv_reg(UINT32 th, UINT32 wt_s, UINT32 wt_e)
{
	T_IME_CHROMA_ADAPTATION_REGISTER2 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_REGISTER2_OFS);

	arg.bit.chra_uv_th = th;
	arg.bit.chra_uv_wts = wt_s;
	arg.bit.chra_uv_wte = wt_e;

	IME_SETREG(IME_CHROMA_ADAPTATION_REGISTER2_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_luma_range_y_reg(UINT32 rng, UINT32 wt_prc)
{
	T_IME_CHROMA_ADAPTATION_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_REGISTER3_OFS);

	arg.bit.luma_rng = rng;
	arg.bit.luma_wtprc = wt_prc;

	IME_SETREG(IME_CHROMA_ADAPTATION_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_luma_weight_y_reg(UINT32 th, UINT32 wt_s, UINT32 wt_e)
{
	T_IME_CHROMA_ADAPTATION_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_REGISTER3_OFS);

	arg.bit.luma_th = th;
	arg.bit.luma_wts = wt_s;
	arg.bit.luma_wte = wt_e;

	IME_SETREG(IME_CHROMA_ADAPTATION_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_subout_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_chra_subout_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_lca_subout_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_chra_subout_en;
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_subout_source_reg(UINT32 src)
{
	T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0_OFS);

	arg.bit.chra_subout_src = src;

	IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_subout_scale_factor_h_reg(UINT32 scl_rate, UINT32 scl_factor)
{
	T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0 arg0;
	T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER1 arg1;

	arg0.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0_OFS);
	arg1.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER1_OFS);

	arg0.bit.chra_h_dnrate = scl_rate;
	arg1.bit.chra_h_sfact = scl_factor;

	IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0_OFS, arg0.reg);
	IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER1_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_subout_scale_factor_v_reg(UINT32 scl_rate, UINT32 scl_factor)
{
	T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0 arg0;
	T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER1 arg1;

	arg0.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0_OFS);
	arg1.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER1_OFS);

	arg0.bit.chra_v_dnrate = scl_rate;
	arg1.bit.chra_v_sfact = scl_factor;

	IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0_OFS, arg0.reg);
	IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER1_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_subout_isd_scale_factor_base_reg(UINT32 scl_base_factor_h, UINT32 scl_base_factor_v)
{
	T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER2 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER2_OFS);

	arg.bit.chra_isd_h_base = scl_base_factor_h;
	arg.bit.chra_isd_v_base = scl_base_factor_v;

	IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER2_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_subout_isd_scale_factor0_reg(UINT32 scl_factor_h, UINT32 scl_factor_v)
{
	T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER3_OFS);

	arg.bit.chra_isd_h_sfact0 = scl_factor_h;
	arg.bit.chra_isd_v_sfact0 = scl_factor_v;

	IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_lca_subout_isd_scale_factor1_reg(UINT32 scl_factor_h, UINT32 scl_factor_v)
{
	T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER4 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER4_OFS);

	arg.bit.chra_isd_h_sfact1 = scl_factor_h;
	arg.bit.chra_isd_v_sfact1 = scl_factor_v;

	IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER4_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_subout_isd_scale_factor2_reg(UINT32 scl_factor_h, UINT32 scl_factor_v)
{
	T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER5 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER5_OFS);

	arg.bit.chra_isd_h_sfact2 = scl_factor_h;
	arg.bit.chra_isd_v_sfact2 = scl_factor_v;

	IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER5_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_subout_isd_scale_coef_number_reg(UINT32 coef_num_h, UINT32 coef_num_v)
{
	T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER8 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER8_OFS);

	arg.bit.chra_isd_h_coef_num = coef_num_h;
	arg.bit.chra_isd_v_coef_num = coef_num_v;

	IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER8_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_lca_subout_lineoffset_reg(UINT32 lofs)
{
	T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER6 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER6_OFS);

	arg.bit.chra_dram_ofso = (lofs >> 2);

	IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER6_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_lca_subout_lineoffset_reg(VOID)
{
	T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER6 arg;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER6_OFS);

	return (arg.bit.chra_dram_ofso << 2);
}
//-------------------------------------------------------------------------------


VOID ime_set_lca_subout_dma_addr_reg(UINT32 addr)
{
	T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER7 arg;

	ime_lca_subout_addr = addr;

	arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER7_OFS);

	arg.bit.chra_dram_sao = ime_platform_va2pa(ime_lca_subout_addr) >> 2;//(uiaddr >> 2);

	IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER7_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_lca_subout_dma_addr_reg(VOID)
{
	//T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER7 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER7_OFS);

	//return (arg.bit.chra_dram_sao << 2);

	return ime_lca_subout_addr;
}
//-------------------------------------------------------------------------------
#else


VOID ime_set_lca_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_chra_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_bypass_enable_reg(UINT32 set_en)
{
	//T_IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS);

	imeg->reg_140.bit.chra_bypass = set_en;

	//IME_SETREG(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


UINT32 ime_get_lca_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return imeg->reg_1.bit.ime_chra_en;
}
//-------------------------------------------------------------------------------


VOID ime_set_lca_chroma_adj_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_chra_ca_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_la_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_chra_la_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------
#if 0

VOID ime_set_lca_enable_ppb_reg(UINT32 set_en)
{

	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_chra_ippb_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
#endif

//-------------------------------------------------------------------------------

VOID ime_set_lca_image_size_reg(UINT32 size_h, UINT32 size_v)
{
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER0_OFS);

	imeg->reg_136.bit.chra_h_size = size_h;
	imeg->reg_136.bit.chra_v_size = size_v;

	//IME_SETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_lca_image_size_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER0_OFS);

	*p_get_size_h = imeg->reg_136.bit.chra_h_size;
	*p_get_size_v = imeg->reg_136.bit.chra_v_size;
}
//-------------------------------------------------------------------------------


VOID ime_set_lca_scale_factor_reg(UINT32 factor_h, UINT32 factor_v)
{
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER1_OFS);

	imeg->reg_137.bit.chra_h_sfact = factor_h;
	imeg->reg_137.bit.chra_v_sfact = factor_v;

	//IME_SETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_lineoffset_reg(UINT32 lofs_y, UINT32 lofs_c)
{
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER2 arg_y;
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER5 argC;

	//arg_y.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER2_OFS);
	//argC.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER5_OFS);

	imeg->reg_138.bit.chra_dram_y_ofsi = (lofs_y >> 2);
	//argC.bit.CHRA_DRAM_C_OFSI = (uiLofsC >> 2);

	//IME_SETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER2_OFS, arg_y.reg);
	//IME_SETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER5_OFS, argC.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_lca_lineoffset_reg(UINT32 *p_get_lofs_y, UINT32 *p_get_lofs_c)
{
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER2 arg_y;
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER5 argC;

	//arg_y.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER2_OFS);
	//argC.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER5_OFS);

	*p_get_lofs_y = (imeg->reg_138.word);
	*p_get_lofs_c = (imeg->reg_138.word);
}
//-------------------------------------------------------------------------------


VOID ime_set_lca_dma_addr0_reg(UINT32 addr_y, UINT32 addr_c)
{
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3 arg_y;
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER6 arg_c;

	ime_lca_in_addr = addr_y;

	//arg_y.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3_OFS);
	//arg_c.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER6_OFS);

	//arg_y.bit.chra_dram_y_sai0 = ime_platform_va2pa(ime_lca_in_addr) >> 2;//(uiaddry >> 2);
	imeg->reg_139.bit.chra_dram_y_sai0 = ime_platform_va2pa(ime_lca_in_addr) >> 2;//(uiaddry >> 2);
	//arg_c.bit.chra_dram_c_sai0 = (addr_c >> 2);

	//IME_SETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3_OFS, arg_y.reg);
	//IME_SETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER6_OFS, arg_c.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_lca_dma_addr0_reg(UINT32 *p_get_addr_y, UINT32 *p_get_addr_c)
{
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3 arg_y;
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER6 arg_c;

	//arg_y.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3_OFS);
	//arg_c.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER6_OFS);

	*p_get_addr_y = ime_lca_in_addr;//(arg_y.bit.chra_dram_y_sai0 << 2);
	*p_get_addr_c = ime_lca_in_addr;//(arg_y.bit.chra_dram_y_sai0 << 2);
}
//-------------------------------------------------------------------------------


VOID ime_set_lca_dma_addr1_reg(UINT32 addr_y, UINT32 addr_c)
{
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3 arg_y;
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER7 argC;

	ime_lca_in_addr = addr_y;

	//arg_y.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3_OFS);
	//argC.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER7_OFS);

	//arg_y.bit.chra_dram_y_sai0 = ime_platform_va2pa(ime_lca_in_addr) >> 2;//(uiaddry >> 2);
	imeg->reg_139.bit.chra_dram_y_sai0 = ime_platform_va2pa(ime_lca_in_addr) >> 2;//(uiaddry >> 2);
	//argC.bit.CHRA_DRAM_C_SAI1 = (uiAddrC >> 2);

	//IME_SETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3_OFS, arg_y.reg);
	//IME_SETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER7_OFS, argC.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_lca_dma_addr1_reg(UINT32 *p_get_addr_y, UINT32 *p_get_addr_c)
{
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3 arg_y;
	//T_IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER7 arg_c;

	//arg_y.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3_OFS);
	//arg_c.reg = IME_GETREG(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER7_OFS);

	*p_get_addr_y = ime_lca_in_addr;//(arg_y.bit.chra_dram_y_sai0 << 2);
	*p_get_addr_c = ime_lca_in_addr;//(arg_y.bit.chra_dram_y_sai0 << 2);
}
//-------------------------------------------------------------------------------

#if 0
UINT32 ime_get_lca_active_ppb_id_reg(VOID)
{
#if 0
	//T_IME_CHROMA_ADAPTATION_PPB_STATUS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_PPB_STATUS_REGISTER0_OFS);

	return arg.bit.chra_actppb_id;
#endif

	return 0;
}
#endif
//-------------------------------------------------------------------------------

VOID ime_set_lca_format_reg(UINT32 set_fmt)
{
	//T_IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS);

	imeg->reg_140.bit.chra_fmt = 0;

	//IME_SETREG(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_lca_format_reg(VOID)
{
	//T_IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS);

	return imeg->reg_140.bit.chra_fmt;
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_source_reg(UINT32 set_src)
{
	//T_IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS);

	imeg->reg_140.bit.chra_src = set_src;

	//IME_SETREG(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_lca_ca_adjust_center_reg(UINT32 u, UINT32 v)
{
	//T_IME_CHROMA_ADJUSTMENT_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADJUSTMENT_REGISTER0_OFS);

	imeg->reg_141.bit.chra_ca_ctr_u = u;
	imeg->reg_141.bit.chra_ca_ctr_v = v;

	//IME_SETREG(IME_CHROMA_ADJUSTMENT_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_ca_range_reg(UINT32 rng, UINT32 wet_prc)
{
	//T_IME_CHROMA_ADJUSTMENT_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADJUSTMENT_REGISTER1_OFS);

	imeg->reg_142.bit.chra_ca_rng = rng;
	imeg->reg_142.bit.chra_ca_wtprc = wet_prc;

	//IME_SETREG(IME_CHROMA_ADJUSTMENT_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

extern VOID ime_set_lca_ca_weight_reg(UINT32 th, UINT32 wt_s, UINT32 wt_e)
{
	//T_IME_CHROMA_ADJUSTMENT_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADJUSTMENT_REGISTER1_OFS);

	imeg->reg_142.bit.chra_ca_th = th;
	imeg->reg_142.bit.chra_ca_wts = wt_s;
	imeg->reg_142.bit.chra_ca_wte = wt_e;

	//IME_SETREG(IME_CHROMA_ADJUSTMENT_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_chroma_ref_image_weight_reg(UINT32 ref_y_wt, UINT32 ref_c_wt, UINT32 out_wt)
{
	//T_IME_CHROMA_ADAPTATION_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_REGISTER0_OFS);

	imeg->reg_143.bit.chra_refy_wt = ref_y_wt;
	imeg->reg_143.bit.chra_refc_wt = ref_c_wt;
	imeg->reg_143.bit.chra_out_wt = out_wt;

	//IME_SETREG(IME_CHROMA_ADAPTATION_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_luma_ref_image_weight_reg(UINT32 ref_y_wt, UINT32 out_wt)
{
	//T_IME_CHROMA_ADAPTATION_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_REGISTER0_OFS);

	imeg->reg_143.bit.luma_refy_wt = ref_y_wt;
	imeg->reg_143.bit.luma_out_wt = out_wt;

	//IME_SETREG(IME_CHROMA_ADAPTATION_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_chroma_range_y_reg(UINT32 rng, UINT32 wet_prc)
{
	//T_IME_CHROMA_ADAPTATION_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_REGISTER1_OFS);

	imeg->reg_144.bit.chra_y_rng = rng;
	imeg->reg_144.bit.chra_y_wtprc = wet_prc;

	//IME_SETREG(IME_CHROMA_ADAPTATION_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_chroma_weight_y_reg(UINT32 th, UINT32 wt_s, UINT32 wt_e)
{
	//T_IME_CHROMA_ADAPTATION_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_REGISTER1_OFS);

	imeg->reg_144.bit.chra_y_th = th;
	imeg->reg_144.bit.chra_y_wts = wt_s;
	imeg->reg_144.bit.chra_y_wte = wt_e;

	//IME_SETREG(IME_CHROMA_ADAPTATION_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_chroma_range_uv_reg(UINT32 rng, UINT32 wt_prc)
{
	//T_IME_CHROMA_ADAPTATION_REGISTER2 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_REGISTER2_OFS);

	imeg->reg_145.bit.chra_uv_rng = rng;
	imeg->reg_145.bit.chra_uv_wtprc = wt_prc;

	//IME_SETREG(IME_CHROMA_ADAPTATION_REGISTER2_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_chroma_weight_uv_reg(UINT32 th, UINT32 wt_s, UINT32 wt_e)
{
	//T_IME_CHROMA_ADAPTATION_REGISTER2 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_REGISTER2_OFS);

	imeg->reg_145.bit.chra_uv_th = th;
	imeg->reg_145.bit.chra_uv_wts = wt_s;
	imeg->reg_145.bit.chra_uv_wte = wt_e;

	//IME_SETREG(IME_CHROMA_ADAPTATION_REGISTER2_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_luma_range_y_reg(UINT32 rng, UINT32 wt_prc)
{
	//T_IME_CHROMA_ADAPTATION_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_REGISTER3_OFS);

	imeg->reg_146.bit.luma_rng = rng;
	imeg->reg_146.bit.luma_wtprc = wt_prc;

	//IME_SETREG(IME_CHROMA_ADAPTATION_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_luma_weight_y_reg(UINT32 th, UINT32 wt_s, UINT32 wt_e)
{
	//T_IME_CHROMA_ADAPTATION_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_REGISTER3_OFS);

	imeg->reg_146.bit.luma_th = th;
	imeg->reg_146.bit.luma_wts = wt_s;
	imeg->reg_146.bit.luma_wte = wt_e;

	//IME_SETREG(IME_CHROMA_ADAPTATION_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_subout_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_chra_subout_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_lca_subout_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return imeg->reg_1.bit.ime_chra_subout_en;
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_subout_source_reg(UINT32 src)
{
	//T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0_OFS);

	imeg->reg_148.bit.chra_subout_src = src;

	//IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_subout_scale_factor_h_reg(UINT32 scl_rate, UINT32 scl_factor)
{
	//T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0 arg0;
	//T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER1 arg1;

	//arg0.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0_OFS);
	//arg1.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER1_OFS);

	imeg->reg_148.bit.chra_h_dnrate = scl_rate;
	imeg->reg_149.bit.chra_h_sfact = scl_factor;

	//IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0_OFS, arg0.reg);
	//IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER1_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_subout_scale_factor_v_reg(UINT32 scl_rate, UINT32 scl_factor)
{
	//T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0 arg0;
	//T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER1 arg1;

	//arg0.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0_OFS);
	//arg1.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER1_OFS);

	imeg->reg_149.bit.chra_v_sfact = scl_factor;
	imeg->reg_148.bit.chra_v_dnrate = scl_rate;

	//IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0_OFS, arg0.reg);
	//IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER1_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_lca_subout_isd_scale_factor_base_reg(UINT32 scl_base_factor_h, UINT32 scl_base_factor_v)
{
	//T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER2 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER2_OFS);

	imeg->reg_150.bit.chra_isd_h_base = scl_base_factor_h;
	imeg->reg_150.bit.chra_isd_v_base = scl_base_factor_v;

	//IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER2_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_subout_isd_scale_factor0_reg(UINT32 scl_factor_h, UINT32 scl_factor_v)
{
	//T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER3 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER3_OFS);

	imeg->reg_151.bit.chra_isd_h_sfact0 = scl_factor_h;
	imeg->reg_151.bit.chra_isd_v_sfact0 = scl_factor_v;

	//IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_subout_isd_scale_factor1_reg(UINT32 scl_factor_h, UINT32 scl_factor_v)
{
	//T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER4 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER4_OFS);

	imeg->reg_152.bit.chra_isd_h_sfact1 = scl_factor_h;
	imeg->reg_152.bit.chra_isd_v_sfact1 = scl_factor_v;

	//IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER4_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_subout_isd_scale_factor2_reg(UINT32 scl_factor_h, UINT32 scl_factor_v)
{
	//T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER5 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER5_OFS);

	imeg->reg_153.bit.chra_isd_h_sfact2 = scl_factor_h;
	imeg->reg_153.bit.chra_isd_v_sfact2 = scl_factor_v;

	//IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER5_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_lca_subout_isd_scale_coef_number_reg(UINT32 coef_num_h, UINT32 coef_num_v)
{
	//T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER8 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER8_OFS);

	imeg->reg_156.bit.chra_isd_h_coef_num = coef_num_h;
	imeg->reg_156.bit.chra_isd_v_coef_num = coef_num_v;

	//IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER8_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_lca_subout_lineoffset_reg(UINT32 lofs)
{
	//T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER6 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER6_OFS);

	imeg->reg_154.bit.chra_dram_ofso = (lofs >> 2);

	//IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER6_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_lca_subout_lineoffset_reg(VOID)
{
	//T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER6 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER6_OFS);

	return (imeg->reg_154.word);
}
//-------------------------------------------------------------------------------


VOID ime_set_lca_subout_dma_addr_reg(UINT32 addr)
{
	//T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER7 arg;

	ime_lca_subout_addr = addr;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER7_OFS);

	imeg->reg_155.bit.chra_dram_sao = ime_platform_va2pa(ime_lca_subout_addr) >> 2;//(uiaddr >> 2);

	//IME_SETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER7_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_lca_subout_dma_addr_reg(VOID)
{
	//T_IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER7 arg;

	//arg.reg = IME_GETREG(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER7_OFS);

	//return (arg.bit.chra_dram_sao << 2);

	return ime_lca_subout_addr;
}
//-------------------------------------------------------------------------------


#endif
