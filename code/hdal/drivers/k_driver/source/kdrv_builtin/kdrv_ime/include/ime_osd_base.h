/*
    IME module driver

    NT96520 IME internal header file.

    @file       ime_osd_base.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/


#ifndef _IME_OSD_REG_
#define _IME_OSD_REG_

#ifdef __cplusplus
extern "C" {
#endif

#include "ime_platform.h"



//-------------------------------------------------------------------------------
// data stamp
extern VOID ime_set_osd_cst_enable_reg(UINT32 set_en);


// data stamp0
extern VOID ime_set_osd_set0_enable_reg(UINT32 set_en);
extern VOID ime_set_osd_set0_image_size_reg(UINT32 size_h, UINT32 size_v);
extern VOID ime_set_osd_set0_format_reg(UINT32 set_fmt);
extern VOID ime_set_osd_set0_blend_weight_reg(UINT32 wet0, UINT32 wet1);
extern VOID ime_set_osd_set0_position_reg(UINT32 pos_h, UINT32 pos_v);
extern VOID ime_set_osd_set0_color_key_reg(UINT32 ck_a, UINT32 ck_r, UINT32 ck_g, UINT32 ck_b);
extern VOID ime_set_osd_set0_lineoffset_reg(UINT32 lofs);
extern VOID ime_set_osd_set0_dma_addr_reg(UINT32 addr);
extern VOID ime_get_osd_set0_image_size_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v);
extern VOID ime_get_osd_set0_lineoffset_reg(UINT32 *g_get_lofs);
extern VOID ime_get_osd_set0_dma_addr_reg(UINT32 *p_get_addr);
extern UINT32 ime_get_osd_set0_enable_reg(VOID);
extern VOID ime_set_osd_set0_color_key_enable_reg(UINT32 set_en);
extern VOID ime_set_osd_set0_color_palette_enable_reg(UINT32 set_en);
extern VOID ime_set_osd_set0_color_key_mode_reg(UINT32 set_mode);

//-------------------------------------------------------------------------------
// data stamp1
extern VOID ime_set_osd_set1_enable_reg(UINT32 set_en);
extern VOID ime_set_osd_set1_image_size_reg(UINT32 size_h, UINT32 size_v);
extern VOID ime_set_osd_set1_blend_weight_reg(UINT32 wet0, UINT32 wet1);
extern VOID ime_set_osd_set1_format_reg(UINT32 fmt);
extern VOID ime_set_osd_set1_position_reg(UINT32 pos_h, UINT32 pos_v);
extern VOID ime_set_osd_set1_color_key_reg(UINT32 ck_a, UINT32 ck_r, UINT32 ck_g, UINT32 ck_b);
extern VOID ime_set_osd_set1_lineoffset_reg(UINT32 lofs);
extern VOID ime_set_osd_set1_dma_addr_reg(UINT32 addr);
extern VOID ime_get_osd_set1_image_size_reg(UINT32 *p_size_h, UINT32 *p_size_v);
extern VOID ime_get_osd_set1_lineoffset_reg(UINT32 *p_lofs);
extern VOID ime_get_osd_set1_dma_addr_reg(UINT32 *p_addr);
extern UINT32 ime_get_osd_set1_enable_reg(VOID);
extern VOID ime_set_osd_set1_color_key_enable_reg(UINT32 set_en);
extern VOID ime_set_osd_set1_color_palette_enable_reg(UINT32 set_en);
extern VOID ime_set_osd_set1_color_key_mode_reg(UINT32 set_mode);


//-------------------------------------------------------------------------------
// data stamp2
extern VOID ime_set_osd_set2_enable_reg(UINT32 set_en);
extern VOID ime_set_osd_set2_image_size_reg(UINT32 size_h, UINT32 size_v);
extern VOID ime_set_osd_set2_blend_weight_reg(UINT32 wet0, UINT32 wet1);
extern VOID ime_set_osd_set2_format_reg(UINT32 fmt);
extern VOID ime_set_osd_set2_position_reg(UINT32 pos_h, UINT32 pos_v);
extern VOID ime_set_osd_set2_color_key_reg(UINT32 ck_a, UINT32 ck_r, UINT32 ck_g, UINT32 ck_b);
extern VOID ime_set_osd_set2_lineoffset_reg(UINT32 lofs);
extern VOID ime_set_osd_set2_dma_addr_reg(UINT32 addr);
extern VOID ime_get_osd_set2_image_size_reg(UINT32 *p_size_h, UINT32 *p_size_v);
extern VOID ime_get_osd_set2_lineoffset_reg(UINT32 *p_lofs);
extern VOID ime_get_osd_set2_dma_addr_reg(UINT32 *p_addr);
extern UINT32 ime_get_osd_set2_enable_reg(VOID);
extern VOID ime_set_osd_set2_color_key_enable_reg(UINT32 set_en);

extern VOID ime_set_osd_set2_color_palette_enable_reg(UINT32 set_en);
extern VOID ime_set_osd_set2_color_key_mode_reg(UINT32 set_mode);



//-------------------------------------------------------------------------------
// data stamp3
extern VOID ime_set_osd_set3_enable_reg(UINT32 set_en);
extern VOID ime_set_osd_set3_image_size_reg(UINT32 size_h, UINT32 size_v);
extern VOID ime_set_osd_set3_blend_weight_reg(UINT32 wet0, UINT32 wet1);
extern VOID ime_set_osd_set3_format_reg(UINT32 fmt);
extern VOID ime_set_osd_set3_position_reg(UINT32 pos_h, UINT32 pos_v);
extern VOID ime_set_osd_set3_color_key_reg(UINT32 ck_a, UINT32 ck_r, UINT32 ck_g, UINT32 ck_b);
extern VOID ime_set_osd_set3_lineoffset_reg(UINT32 lofs);
extern VOID ime_set_osd_set3_dma_addr_reg(UINT32 addr);
extern VOID ime_get_osd_set3_image_size_reg(UINT32 *p_size_h, UINT32 *p_size_v);
extern VOID ime_get_osd_set3_lineoffset_reg(UINT32 *p_lofs);
extern VOID ime_get_osd_set3_dma_addr_reg(UINT32 *p_addr);
extern UINT32 ime_get_osd_set3_enable_reg(VOID);
extern VOID ime_set_osd_set3_color_key_enable_reg(UINT32 set_en);

extern VOID ime_set_osd_set3_color_palette_enable_reg(UINT32 set_en);
extern VOID ime_set_osd_set3_color_key_mode_reg(UINT32 set_mode);



// data stamp CST coefficients
extern VOID ime_set_osd_cst_coef_reg(UINT32 c0, UINT32 c1, UINT32 c2, UINT32 c3);

extern VOID ime_set_osd_color_palette_mode_reg(UINT32 set_mode);
extern VOID ime_set_osd_color_palette_lut_reg(UINT32 *p_lut_a, UINT32 *p_lut_r, UINT32 *p_lut_g, UINT32 *p_lut_b);




#ifdef __cplusplus
}
#endif


#endif // _IME_OSD_REG_
