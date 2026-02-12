
/*
    IME module driver

    NT96510 IME internal header file.

    @file       ime_out_path1_base.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/


#ifndef _IME_OUT_PATH1_REG_
#define _IME_OUT_PATH1_REG_

#ifdef __cplusplus
extern "C" {
#endif


#include "ime_platform.h"



// output path1 register APIs
extern VOID ime_open_output_p1_reg(UINT32 set_en);

extern VOID ime_set_flip_enable_p1_reg(UINT32 set_en);

extern VOID ime_set_output_dram_enable_p1_reg(UINT32 set_en);

extern VOID ime_set_output_dest_p1_reg(UINT32 dst);  // set by engine mode

extern VOID ime_set_output_image_format_p1_reg(UINT32 out_format_sel);

extern VOID ime_set_scale_interpolation_method_p1_reg(UINT32 scale_method_sel);

extern VOID ime_set_output_type_p1_reg(UINT32 out_fmt_type);

extern VOID ime_set_scaling_size_p1_reg(UINT32 h_scale_size, UINT32 v_scale_size);
extern VOID ime_get_scaling_size_p1_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v);

extern VOID ime_set_scale_factor_h_p1_reg(UINT32 h_scale_sel, UINT32 h_scale_rate, UINT32 h_scale_factor);
extern VOID ime_set_scale_factor_v_p1_reg(UINT32 v_scale_sle, UINT32 v_scale_rate, UINT32 v_scale_factor);

extern VOID ime_set_output_size_p1_reg(UINT32 set_size_h, UINT32 set_size_v);
extern VOID ime_get_output_size_p1_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v);


extern VOID ime_set_horizontal_scale_filtering_p1_reg(UINT32 set_en, UINT32 set_coef);
extern VOID ime_set_vertical_scale_filtering_p1_reg(UINT32 set_en, UINT32 set_coef);

extern VOID ime_set_integration_scaling_factor_base_p1_reg(UINT32 base_scale_h_factor, UINT32 base_scale_v_factor);
extern VOID ime_set_integration_scaling_factor0_p1_reg(UINT32 integration_scale_h_factor, UINT32 integration_scale_v_factor);
extern VOID ime_set_integration_scaling_factor1_p1_reg(UINT32 integration_scale_h_factor, UINT32 integration_scale_v_factor);
extern VOID ime_set_integration_scaling_factor2_p1_reg(UINT32 integration_scale_h_factor, UINT32 integration_scale_v_factor);


extern VOID ime_set_scaling_enhance_factor_p1_reg(UINT32 scl_enh_ftr, UINT32 scl_enh_shf);

extern VOID ime_set_output_crop_coordinate_p1_reg(UINT32 crop_pos_x, UINT32 crop_pos_y);
extern VOID ime_get_output_crop_coordinate_p1_reg(UINT32 *p_get_crop_pos_x, UINT32 *p_get_crop_pos_y);


extern VOID ime_set_output_lineoffset_y_p1_reg(UINT32 lofs);
extern VOID ime_set_output_lineoffset_c_p1_reg(UINT32 lofs);
extern UINT32 ime_get_output_lineoffset_y_p1_reg(VOID);
extern UINT32 ime_get_output_lineoffset_c_p1_reg(VOID);

extern VOID ime_set_output_channel_addr_p1_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr);
extern UINT32 ime_get_output_addr_y_p1_reg(VOID);
extern UINT32 ime_get_output_addr_cb_p1_reg(VOID);
extern UINT32 ime_get_output_addr_cr_p1_reg(VOID);

extern UINT32 ime_get_output_path_enable_status_p1_reg(VOID);
extern UINT32 ime_get_output_format_p1_reg(VOID);
extern UINT32 ime_get_output_format_type_p1_reg(VOID);
extern UINT32 ime_get_scaling_method_p1_reg(VOID);
extern UINT32 ime_get_output_dest_p1_reg(VOID);

extern VOID ime_set_encode_enable_p1_reg(UINT32 set_en);
extern UINT32 ime_get_encode_enable_p1_reg(VOID);

extern VOID ime_clamp_p1_reg(UINT32 min_y, UINT32 max_y, UINT32 min_uv, UINT32 max_uv);

//-------------------------------------------------------------------------------
// stitching
// stitching - path1
extern VOID ime_set_stitching_enable_p1_reg(UINT32 set_en);
extern VOID ime_set_stitching_base_position_p1_reg(UINT32 base_pos);
extern VOID ime_set_stitching_output_lineoffset_y2_p1_reg(UINT32 lofs);
extern VOID ime_set_stitching_output_lineoffset_c2_p1_reg(UINT32 lofs);
extern VOID ime_set_stitching_output_channel_addr2_p1_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr);

extern UINT32 ime_get_stitching_enable_p1_reg(VOID);
extern UINT32 ime_get_stitching_output_lineoffset_y2_p1_reg(VOID);
extern UINT32 ime_get_stitching_output_lineoffset_c2_p1_reg(VOID);
extern UINT32 ime_get_stitching_output_addr_y2_p1_reg(VOID);
extern UINT32 ime_get_stitching_output_addr_cb2_p1_reg(VOID);


#ifdef __cplusplus
}  // extern "C" {
#endif


#endif // _IME_OUT_PATH1_REG_


