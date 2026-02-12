
/*
    IME module driver

    NT96510 IME module driver.

    @file       ime_function_base.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/



#include "ime_reg.h"
#include "ime_int_public.h"

#include "ime_function_base.h"

#if (IPP_DRIVER_NEW_REG_HANDLE == DISABLE)

// IQ related APIs

UINT32 ime_get_function0_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_function1_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------


#if 0
VOID ime_set_chroma_median_filter_enable_reg(UINT32 uiEnOp)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.IME_CMF_EN = uiEnOp;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
#endif
//-------------------------------------------------------------------------------





#if 0
VOID ime_set_ssr_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.IME_SSR_EN = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_ssr_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return arg.bit.IME_SSR_EN;
}
//-------------------------------------------------------------------------------

VOID ime_set_ssr_threshold_reg(UINT32 uiDTh, UINT32 uiHVTh)
{
	T_IME_SINGLE_FRAME_SUPER_RESOLUATION_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_SINGLE_FRAME_SUPER_RESOLUATION_REGISTER0_OFS);

	arg.bit.SSR_DTH = uiDTh;
	arg.bit.SSR_HVTH = uiHVTh;

	IME_SETREG(IME_SINGLE_FRAME_SUPER_RESOLUATION_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------
#endif

#if 0
VOID ime_set_film_grain_noise_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.IME_GRNS_EN = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_film_grain_noise_threshold_reg(UINT32 fgn_lum_th)
{
	T_IME_FILM_GRAIN_NOISE_REGISTER2 arg1;

	arg1.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER2_OFS);

	arg1.bit.FGN_LUM_TH = fgn_lum_th;

	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER2_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_film_grain_noise_param_p1_reg(UINT32 fgn_init_val, UINT32 fgn_nl)
{
	T_IME_FILM_GRAIN_NOISE_REGISTER1 arg0;
	T_IME_FILM_GRAIN_NOISE_REGISTER0 arg1;

	arg0.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER1_OFS);
	arg0.bit.FGN_INIT_VAL_P1 = fgn_init_val;
	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER1_OFS, arg0.reg);

	arg1.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER0_OFS);
	arg1.bit.FGN_NL_P1 = fgn_nl;
	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER0_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_film_grain_noise_param_p2_reg(UINT32 fgn_init_val, UINT32 fgn_nl)
{
	T_IME_FILM_GRAIN_NOISE_REGISTER1 arg0;
	T_IME_FILM_GRAIN_NOISE_REGISTER0 arg1;

	arg0.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER1_OFS);
	arg0.bit.FGN_INIT_VAL_P2 = fgn_init_val;
	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER1_OFS, arg0.reg);

	arg1.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER0_OFS);
	arg1.bit.FGN_NL_P2 = fgn_nl;
	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER0_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_film_grain_noise_param_p3_reg(UINT32 fgn_init_val, UINT32 fgn_nl)
{
	T_IME_FILM_GRAIN_NOISE_REGISTER1 arg0;
	T_IME_FILM_GRAIN_NOISE_REGISTER0 arg1;

	arg0.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER1_OFS);
	arg0.bit.FGN_INIT_VAL_P3 = fgn_init_val;
	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER1_OFS, arg0.reg);

	arg1.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER0_OFS);
	arg1.bit.FGN_NL_P3 = fgn_nl;
	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER0_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_film_grain_noise_param_p5_reg(UINT32 fgn_init_val, UINT32 fgn_nl)
{
	T_IME_FILM_GRAIN_NOISE_REGISTER1 arg0;
	T_IME_FILM_GRAIN_NOISE_REGISTER0 arg1;

	arg0.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER1_OFS);
	arg0.bit.FGN_INIT_VAL_P5 = fgn_init_val;
	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER1_OFS, arg0.reg);

	arg1.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER0_OFS);
	arg1.bit.FGN_NL_P5 = fgn_nl;
	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER0_OFS, arg1.reg);
}
#endif
//-------------------------------------------------------------------------------


#if 0
VOID ime_set_rgb2ycc_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.RGB_TO_YUV_EN = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_ycc2rgb_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.YUV_TO_RGB_EN = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------
#endif

#if 0
VOID ime_set_p2ifilter_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.IME_PTOI_EN = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
#endif
//------------------------------------------------------------------------------

VOID ime_set_yuv_converter_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_ycc_cvt_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_yuv_converter_sel_reg(UINT32 set_sel)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_ycc_cvt_sel = set_sel;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_yuv_converter_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return arg.bit.ime_ycc_cvt_en;
}
//------------------------------------------------------------------------------

UINT32 ime_get_yuv_converter_sel_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return arg.bit.ime_ycc_cvt_sel;
}





// stitching
// stitching - path5
#if 0
VOID ime_set_stitching_enable_p5_reg(UINT32 set_en)
{
	T_IME_OUTPUT_PATH5_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_CONTROL_REGISTER0_OFS);

	arg.bit.IME_SPRT_OUT_EN_P5 = set_en;

	IME_SETREG(IME_OUTPUT_PATH5_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_stitching_base_position_p5_reg(UINT32 base_pos)
{
	T_IME_OUTPUT_PATH5_CONTROL_REGISTER10 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_CONTROL_REGISTER10_OFS);

	arg.bit.IME_SPRT_HBL_P5 = base_pos;

	IME_SETREG(IME_OUTPUT_PATH5_CONTROL_REGISTER10_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_enable_p5_reg(VOID)
{
	T_IME_OUTPUT_PATH5_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_CONTROL_REGISTER0_OFS);

	return arg.bit.IME_SPRT_OUT_EN_P5;
}
//-------------------------------------------------------------------------------



VOID ime_set_stitching_output_lineoffset_y2_p5_reg(UINT32 set_lofs)
{
	T_IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER2 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER2_OFS);

	arg.bit.DRAM_OFSO_P5_Y1 = (set_lofs >> 2);

	IME_SETREG(IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER2_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_stitching_output_lineoffset_c2_p5_reg(UINT32 set_lofs)
{
	T_IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER3_OFS);

	arg.bit.DRAM_OFSO_P5_UV1 = (set_lofs >> 2);

	IME_SETREG(IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_lineoffset_y2_p5_reg(VOID)
{
	T_IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER2 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER2_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_lineoffset_c2_p5_reg(VOID)
{
	T_IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER3_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------


VOID ime_set_stitching_output_channel_addr2_p5_reg(UINT32 addr_y, UINT32 addr_u, UINT32 addr_v)
{
	T_IME_OUTPUT_PATH5_DMA_BUFFER2_REGISTER3 arg0;
	T_IME_OUTPUT_PATH5_DMA_BUFFER1_REGISTER4 arg1;

	arg0.reg = IME_GETREG(IME_OUTPUT_PATH5_DMA_BUFFER2_REGISTER3_OFS);
	arg1.reg = IME_GETREG(IME_OUTPUT_PATH5_DMA_BUFFER1_REGISTER4_OFS);

	arg0.bit.DRAM_SAO_P5_Y1 = addr_y;
	arg1.bit.DRAM_SAO_P5_UV1 = addr_u;

	IME_SETREG(IME_OUTPUT_PATH5_DMA_BUFFER2_REGISTER3_OFS, arg0.reg);
	IME_SETREG(IME_OUTPUT_PATH5_DMA_BUFFER1_REGISTER4_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_addr_y2_p5_reg(VOID)
{
	T_IME_OUTPUT_PATH5_DMA_BUFFER2_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_DMA_BUFFER2_REGISTER3_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_addr_cb2_p5_reg(VOID)
{
	T_IME_OUTPUT_PATH5_DMA_BUFFER1_REGISTER4 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_DMA_BUFFER1_REGISTER4_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------
#endif


#else

// IQ related APIs

UINT32 ime_get_function0_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return imeg->reg_1.word;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_function1_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return imeg->reg_2.word;
}
//-------------------------------------------------------------------------------


#if 0
VOID ime_set_chroma_median_filter_enable_reg(UINT32 uiEnOp)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_cmf_en = uienop;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
#endif
//-------------------------------------------------------------------------------





#if 0
VOID ime_set_ssr_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_ssr_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_ssr_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return arg.bit.IME_SSR_EN;
}
//-------------------------------------------------------------------------------

VOID ime_set_ssr_threshold_reg(UINT32 uiDTh, UINT32 uiHVTh)
{
	T_IME_SINGLE_FRAME_SUPER_RESOLUATION_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_SINGLE_FRAME_SUPER_RESOLUATION_REGISTER0_OFS);

	arg.bit.ssr_dth = uidth;
	arg.bit.ssr_hvth = uihvth;

	IME_SETREG(IME_SINGLE_FRAME_SUPER_RESOLUATION_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------
#endif

#if 0
VOID ime_set_film_grain_noise_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.IME_GRNS_EN = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_film_grain_noise_threshold_reg(UINT32 fgn_lum_th)
{
	T_IME_FILM_GRAIN_NOISE_REGISTER2 arg1;

	arg1.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER2_OFS);

	arg1.bit.FGN_LUM_TH = fgn_lum_th;

	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER2_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_film_grain_noise_param_p1_reg(UINT32 fgn_init_val, UINT32 fgn_nl)
{
	T_IME_FILM_GRAIN_NOISE_REGISTER1 arg0;
	T_IME_FILM_GRAIN_NOISE_REGISTER0 arg1;

	arg0.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER1_OFS);
	arg0.bit.FGN_INIT_VAL_P1 = fgn_init_val;
	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER1_OFS, arg0.reg);

	arg1.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER0_OFS);
	arg1.bit.FGN_NL_P1 = fgn_nl;
	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER0_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_film_grain_noise_param_p2_reg(UINT32 fgn_init_val, UINT32 fgn_nl)
{
	T_IME_FILM_GRAIN_NOISE_REGISTER1 arg0;
	T_IME_FILM_GRAIN_NOISE_REGISTER0 arg1;

	arg0.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER1_OFS);
	arg0.bit.FGN_INIT_VAL_P2 = fgn_init_val;
	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER1_OFS, arg0.reg);

	arg1.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER0_OFS);
	arg1.bit.FGN_NL_P2 = fgn_nl;
	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER0_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_film_grain_noise_param_p3_reg(UINT32 fgn_init_val, UINT32 fgn_nl)
{
	T_IME_FILM_GRAIN_NOISE_REGISTER1 arg0;
	T_IME_FILM_GRAIN_NOISE_REGISTER0 arg1;

	arg0.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER1_OFS);
	arg0.bit.FGN_INIT_VAL_P3 = fgn_init_val;
	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER1_OFS, arg0.reg);

	arg1.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER0_OFS);
	arg1.bit.FGN_NL_P3 = fgn_nl;
	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER0_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_film_grain_noise_param_p5_reg(UINT32 fgn_init_val, UINT32 fgn_nl)
{
	T_IME_FILM_GRAIN_NOISE_REGISTER1 arg0;
	T_IME_FILM_GRAIN_NOISE_REGISTER0 arg1;

	arg0.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER1_OFS);
	arg0.bit.FGN_INIT_VAL_P5 = fgn_init_val;
	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER1_OFS, arg0.reg);

	arg1.reg = IME_GETREG(IME_FILM_GRAIN_NOISE_REGISTER0_OFS);
	arg1.bit.FGN_NL_P5 = fgn_nl;
	IME_SETREG(IME_FILM_GRAIN_NOISE_REGISTER0_OFS, arg1.reg);
}
#endif
//-------------------------------------------------------------------------------


#if 0
VOID ime_set_rgb2ycc_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.RGB_TO_YUV_EN = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_ycc2rgb_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.YUV_TO_RGB_EN = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------
#endif

#if 0
VOID ime_set_p2ifilter_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.IME_PTOI_EN = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
#endif
//------------------------------------------------------------------------------

VOID ime_set_yuv_converter_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_ycc_cvt_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_yuv_converter_sel_reg(UINT32 set_sel)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_ycc_cvt_sel = set_sel;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_yuv_converter_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return imeg->reg_2.bit.ime_ycc_cvt_en;
}
//------------------------------------------------------------------------------

UINT32 ime_get_yuv_converter_sel_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return imeg->reg_2.bit.ime_ycc_cvt_sel;
}
//------------------------------------------------------------------------------




// stitching
// stitching - path5
#if 0
VOID ime_set_stitching_enable_p5_reg(UINT32 set_en)
{
	T_IME_OUTPUT_PATH5_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_CONTROL_REGISTER0_OFS);

	arg.bit.IME_SPRT_OUT_EN_P5 = set_en;

	IME_SETREG(IME_OUTPUT_PATH5_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_stitching_base_position_p5_reg(UINT32 base_pos)
{
	T_IME_OUTPUT_PATH5_CONTROL_REGISTER10 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_CONTROL_REGISTER10_OFS);

	arg.bit.IME_SPRT_HBL_P5 = base_pos;

	IME_SETREG(IME_OUTPUT_PATH5_CONTROL_REGISTER10_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_enable_p5_reg(VOID)
{
	T_IME_OUTPUT_PATH5_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_CONTROL_REGISTER0_OFS);

	return arg.bit.IME_SPRT_OUT_EN_P5;
}
//-------------------------------------------------------------------------------



VOID ime_set_stitching_output_lineoffset_y2_p5_reg(UINT32 set_lofs)
{
	T_IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER2 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER2_OFS);

	arg.bit.DRAM_OFSO_P5_Y1 = (set_lofs >> 2);

	IME_SETREG(IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER2_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_stitching_output_lineoffset_c2_p5_reg(UINT32 set_lofs)
{
	T_IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER3_OFS);

	arg.bit.DRAM_OFSO_P5_UV1 = (set_lofs >> 2);

	IME_SETREG(IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER3_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_lineoffset_y2_p5_reg(VOID)
{
	T_IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER2 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER2_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_lineoffset_c2_p5_reg(VOID)
{
	T_IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_DMA_LINEOFFSET_REGISTER3_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------


VOID ime_set_stitching_output_channel_addr2_p5_reg(UINT32 addr_y, UINT32 addr_u, UINT32 addr_v)
{
	T_IME_OUTPUT_PATH5_DMA_BUFFER2_REGISTER3 arg0;
	T_IME_OUTPUT_PATH5_DMA_BUFFER1_REGISTER4 arg1;

	arg0.reg = IME_GETREG(IME_OUTPUT_PATH5_DMA_BUFFER2_REGISTER3_OFS);
	arg1.reg = IME_GETREG(IME_OUTPUT_PATH5_DMA_BUFFER1_REGISTER4_OFS);

	arg0.bit.DRAM_SAO_P5_Y1 = addr_y;
	arg1.bit.DRAM_SAO_P5_UV1 = addr_u;

	IME_SETREG(IME_OUTPUT_PATH5_DMA_BUFFER2_REGISTER3_OFS, arg0.reg);
	IME_SETREG(IME_OUTPUT_PATH5_DMA_BUFFER1_REGISTER4_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_addr_y2_p5_reg(VOID)
{
	T_IME_OUTPUT_PATH5_DMA_BUFFER2_REGISTER3 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_DMA_BUFFER2_REGISTER3_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_stitching_output_addr_cb2_p5_reg(VOID)
{
	T_IME_OUTPUT_PATH5_DMA_BUFFER1_REGISTER4 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH5_DMA_BUFFER1_REGISTER4_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------
#endif


#endif



