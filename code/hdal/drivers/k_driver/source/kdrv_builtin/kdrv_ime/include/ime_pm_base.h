/*
    IME module driver

    NT96520 IME internal header file.

    @file       ime_pm_base.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/


#ifndef _IME_PM_REG_
#define _IME_PM_REG_

#ifdef __cplusplus
extern "C" {
#endif

#include "ime_platform.h"


//------------------------------------------------------------------------------
// privacy mask

extern VOID ime_set_privacy_mask_pxl_blk_size_reg(UINT32 set_size);

extern VOID ime_set_privacy_mask_set0_enable_reg(UINT32 set_en);
extern UINT32 ime_get_privacy_mask_set0_enable_reg(VOID);
extern VOID ime_set_privacy_mask_set0_hollow_enable_reg(UINT32 set_en);
extern VOID ime_set_privacy_mask_set0_type_reg(UINT32 set_type);
extern VOID ime_set_privacy_mask_set0_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v);
extern VOID ime_set_privacy_mask_set0_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set0_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set0_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set0_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set0_weight_reg(UINT32 wet);


extern VOID ime_set_privacy_mask_set1_enable_reg(UINT32 set_en);
extern UINT32 ime_get_privacy_mask_set1_enable_reg(VOID);
extern VOID ime_set_privacy_mask_set1_type_reg(UINT32 set_type);
extern VOID ime_set_privacy_mask_set1_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v);
extern VOID ime_set_privacy_mask_set1_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set1_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set1_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set1_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set1_seight_reg(UINT32 wet);


extern VOID ime_set_privacy_mask_set2_enable_reg(UINT32 set_en);
extern UINT32 ime_get_privacy_mask_set2_enable_reg(VOID);
extern VOID ime_set_privacy_mask_set2_hollow_enable_reg(UINT32 set_en);
extern VOID ime_set_privacy_mask_set2_type_reg(UINT32 set_type);
extern VOID ime_set_privacy_mask_set2_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v);
extern VOID ime_set_privacy_mask_set2_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set2_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set2_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set2_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set2_weight_reg(UINT32 wet);


extern VOID ime_set_privacy_mask_set3_enable_reg(UINT32 set_en);
extern UINT32 ime_get_privacy_mask_set3_enable_reg(VOID);
extern VOID ime_set_privacy_mask_set3_type_reg(UINT32 set_type);
extern VOID ime_set_privacy_mask_set3_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v);
extern VOID ime_set_privacy_mask_set3_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set3_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set3_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set3_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set3_weight_reg(UINT32 set);

extern VOID ime_set_privacy_mask_set4_enable_reg(UINT32 set_en);
extern UINT32 ime_get_privacy_mask_set4_enable_reg(VOID);
extern VOID ime_set_privacy_mask_set4_hollow_enable_reg(UINT32 set_en);
extern VOID ime_set_privacy_mask_set4_type_reg(UINT32 set_type);
extern VOID ime_set_privacy_mask_set4_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v);
extern VOID ime_set_privacy_mask_set4_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set4_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set4_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set4_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set4_weight_reg(UINT32 wet);

extern VOID ime_set_privacy_mask_set5_enable_reg(UINT32 set_en);
extern UINT32 ime_get_privacy_mask_set5_enable_reg(VOID);
extern VOID ime_set_privacy_mask_set5_type_reg(UINT32 set_type);
extern VOID ime_set_privacy_mask_set5_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v);
extern VOID ime_set_privacy_mask_set5_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set5_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set5_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set5_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set5_weight_reg(UINT32 wet);


extern VOID ime_set_privacy_mask_set6_enable_reg(UINT32 set_en);
extern UINT32 ime_get_privacy_mask_set6_enable_reg(VOID);
extern VOID ime_set_privacy_mask_set6_hollow_enable_reg(UINT32 set_en);
extern VOID ime_set_privacy_mask_set6_hollow_type_reg(UINT32 set_type);
extern VOID ime_set_privacy_mask_set6_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v);
extern VOID ime_set_privacy_mask_set6_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set6_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set6_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set6_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set6_weight_reg(UINT32 wet);


extern VOID ime_set_privacy_mask_set7_enable_reg(UINT32 set_en);
extern UINT32 ime_get_privacy_mask_set7_enable_reg(VOID);

extern VOID ime_set_privacy_mask_set7_type_reg(UINT32 set_type);
extern VOID ime_set_privacy_mask_set7_color_yuv_reg(UINT32 y, UINT32 u, UINT32 v);
extern VOID ime_set_privacy_mask_set7_line0_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set7_line1_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set7_line2_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set7_line3_reg(INT32 coef_a, INT32 coef_b, INT32 coef_c, INT32 coef_d);
extern VOID ime_set_privacy_mask_set7_weight_reg(UINT32 wet);

extern VOID ime_set_privacy_mask_pxl_image_size_reg(UINT32 size_h, UINT32 size_v);
extern VOID ime_set_privacy_mask_pxl_image_format_reg(UINT32 set_fmt);
extern VOID ime_set_privacy_mask_pxl_image_lineoffset_reg(UINT32 lofs);
extern VOID ime_set_privacy_mask_pxl_image_dma_addr_reg(UINT32 addr);

extern VOID ime_get_privacy_mask_pxl_image_size_reg(UINT32 *p_size_h, UINT32 *p_size_v);
extern VOID ime_get_privacy_mask_pxl_image_lineoffset_reg(UINT32 *p_lofs);
extern VOID ime_get_privacy_mask_pxl_image_dma_addr_reg(UINT32 *p_addr);





#ifdef __cplusplus
}
#endif


#endif // _IME_PM_REG_
