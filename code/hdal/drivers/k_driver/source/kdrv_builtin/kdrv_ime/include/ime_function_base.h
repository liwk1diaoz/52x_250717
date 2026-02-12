
/*
    IME module driver

    NT96510 IME internal header file.

    @file       ime_function_base.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/


#ifndef _IME_FUNCTION_REG_
#define _IME_FUNCTION_REG_

#ifdef __cplusplus
extern "C" {
#endif

#include "ime_platform.h"


#define IME_RNG_SEL_8   0
#define IME_RNG_SEL_16  1
#define IME_RNG_SEL_32  2
#define IME_RNG_SEL_64  3


#define IME_WT_SEL_8    0
#define IME_WT_SEL_16   1
#define IME_WT_SEL_32   2
#define IME_WT_SEL_64   3

// total functions
extern UINT32 ime_get_function0_enable_reg(VOID);
extern UINT32 ime_get_function1_enable_reg(VOID);


// chroma medain filter
//extern VOID ime_set_chroma_median_filter_enable_reg(UINT32 set_en);






//-------------------------------------------------------------------------------
// SSR
//extern VOID ime_set_ssr_enable_reg(UINT32 set_en);
//extern UINT32 ime_get_ssr_enable_reg(VOID);
//extern VOID ime_set_ssr_threshold_reg(UINT32 d_th, UINT32 hv_th);


//-------------------------------------------------------------------------------
// film grain noise
#if 0
extern VOID ime_set_film_grain_noise_enable_reg(UINT32 set_en);
extern VOID ime_set_film_grain_noise_threshold_reg(UINT32 fgn_lum_th);
extern VOID ime_set_film_grain_noise_param_p1_reg(UINT32 fgn_init_val, UINT32 fgn_nl);
extern VOID ime_set_film_grain_noise_param_p2_reg(UINT32 fgn_init_val, UINT32 fgn_nl);
extern VOID ime_set_film_grain_noise_param_p3_reg(UINT32 fgn_init_val, UINT32 fgn_nl);
//extern VOID ime_set_film_grain_noise_param_p5_reg(UINT32 fgn_init_val, UINT32 fgn_nl);
#endif

//-------------------------------------------------------------------------------
// CST
#if 0
extern VOID ime_set_rgb2ycc_enable_reg(UINT32 set_en);
extern VOID ime_set_ycc2rgb_enable_reg(UINT32 set_en);
#endif

//-------------------------------------------------------------------------------
// P2I filter
//extern VOID ime_set_p2ifilter_enable_reg(UINT32 set_en);





//-------------------------------------------------------------------
// YUV converter
extern VOID ime_set_yuv_converter_enable_reg(UINT32 set_en);
extern VOID ime_set_yuv_converter_sel_reg(UINT32 set_sel);
extern UINT32 ime_get_yuv_converter_enable_reg(VOID);
extern UINT32 ime_get_yuv_converter_sel_reg(VOID);









//-------------------------------------------------------------------------------
// stitching
// stitching - path5
#if 0
extern VOID ime_set_stitching_enable_p5_reg(UINT32 set_en);
extern VOID ime_set_stitching_base_position_p5_reg(UINT32 base_pos);
extern VOID ime_set_stitching_output_lineoffset_y2_p5_reg(UINT32 set_lofs);
extern VOID ime_set_stitching_output_lineoffset_c2_p5_reg(UINT32 set_lofs);
extern VOID ime_set_stitching_output_channel_addr2_p5_reg(UINT32 addr_y, UINT32 addr_u, UINT32 addr_v);

extern UINT32 ime_get_stitching_enable_p5_reg(VOID);
extern UINT32 ime_get_stitching_output_lineoffset_y2_p5_reg(VOID);
extern UINT32 ime_get_stitching_output_lineoffset_c2_p5_reg(VOID);
extern UINT32 ime_get_stitching_output_addr_y2_p5_reg(VOID);
extern UINT32 ime_get_stitching_output_addr_cb2_p5_reg(VOID);
#endif




#ifdef __cplusplus
}
#endif


#endif // _IME_FUNCTION_REG_


