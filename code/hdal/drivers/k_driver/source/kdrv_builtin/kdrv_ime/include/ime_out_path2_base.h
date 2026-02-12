
/*
    IME module driver

    NT96510 IME internal header file.

    @file       ime_out_path2_base.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/


#ifndef _IME_OUT_PATH2_REG_
#define _IME_OUT_PATH2_REG_

#ifdef __cplusplus
extern "C" {
#endif

#include "ime_platform.h"



// output path2 register APIs
extern VOID ime_open_output_p2_reg(UINT32 set_en);

extern VOID ime_set_flip_enable_p2_reg(UINT32 set_en);

extern VOID ime_set_output_dram_enable_p2_reg(UINT32 set_en);

extern VOID ime_set_output_image_format_p2_reg(UINT32 fmt_sel);

extern VOID ime_set_scale_interpolation_method_p2_reg(UINT32 set_scl_method);

extern VOID ime_set_output_type_p2_reg(UINT32 set_out_fmt_type);

extern VOID ime_set_scaling_size_p2_reg(UINT32 h_scale_size, UINT32 v_scale_size);
extern VOID ime_get_scaling_size_p2_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v);

extern VOID ime_set_scale_factor_h_p2_reg(UINT32 h_scale_sel, UINT32 h_scale_rate, UINT32 h_scale_factor);
extern VOID ime_set_scale_factor_v_p2_reg(UINT32 v_scale_sel, UINT32 v_scale_rate, UINT32 v_scale_factor);

extern VOID ime_set_output_size_p2_reg(UINT32 set_size_h, UINT32 set_size_v);
extern VOID ime_get_output_size_p2_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v);


extern VOID ime_set_horizontal_scale_filtering_p2_reg(UINT32 set_en, UINT32 set_coef);
extern VOID ime_set_vertical_scale_filtering_p2_reg(UINT32 set_en, UINT32 set_coef);

extern VOID ime_set_integration_scaling_factor_base_p2_reg(UINT32 set_base_h_factor, UINT32 set_base_v_factor);
extern VOID ime_set_integration_scaling_factor0_p2_reg(UINT32 set_h_factor, UINT32 set_v_factor);
extern VOID ime_set_integration_scaling_factor1_p2_reg(UINT32 set_h_factor, UINT32 set_v_factor);
extern VOID ime_set_integration_scaling_factor2_p2_reg(UINT32 set_h_factor, UINT32 set_v_factor);

extern VOID ime_set_integration_scaling_ctrl_p2_reg(UINT32 isd_mode, UINT32 isd_h_coef_num, UINT32 isd_v_coef_num);
extern VOID ime_set_integration_scaling_h_coefs_p2_reg(INT32 *hcoefs);
extern VOID ime_set_integration_scaling_v_coefs_p2_reg(INT32 *vcoefs);
extern VOID ime_set_integration_scaling_h_coefs_sum_p2_reg(INT32 coef_sum_all, INT32 coef_sum_half);
extern VOID ime_set_integration_scaling_v_coefs_sum_p2_reg(INT32 coef_sum_all, INT32 coef_sum_half);


extern VOID ime_set_scaling_enhance_factor_p2_reg(UINT32 scl_enh_ftr, UINT32 scl_enh_sht);

extern VOID ime_set_output_crop_coordinate_p2_reg(UINT32 crop_pos_x, UINT32 crop_pos_y);
extern VOID ime_get_output_crop_coordinate_p2_reg(UINT32 *p_get_crop_pos_x, UINT32 *p_get_crop_pos_y);


extern VOID ime_set_output_lineoffset_y_p2_reg(UINT32 lofs);
extern VOID ime_set_output_lineoffset_c_p2_reg(UINT32 lofs);
extern UINT32 ime_get_output_lineoffset_y_p2_reg(VOID);
extern UINT32 ime_get_output_lineoffset_c_p2_reg(VOID);

extern VOID ime_set_output_channel_addr_p2_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr);
extern UINT32 ime_get_output_addr_y_p2_reg(VOID);
extern UINT32 ime_get_output_addr_cb_p2_reg(VOID);

extern UINT32 ime_get_output_path_enable_status_p2_reg(VOID);
extern UINT32 ime_get_output_format_p2_reg(VOID);

extern UINT32 ime_get_output_format_type_p2_reg(VOID);
extern UINT32 ime_get_scaling_method_p2_reg(VOID);

extern VOID ime_clamp_p2_reg(UINT32 min_y, UINT32 max_y, UINT32 min_uv, UINT32 max_uv);

//-------------------------------------------------------------------------------
// stitching
// stitching - path2
extern VOID ime_set_stitching_enable_p2_reg(UINT32 set_en);
extern VOID ime_set_stitching_base_position_p2_reg(UINT32 base_pos);
extern VOID ime_set_stitching_output_lineoffset_y2_p2_reg(UINT32 lofs);
extern VOID ime_set_stitching_output_lineoffset_c2_p2_reg(UINT32 lofs);
extern VOID ime_set_stitching_output_channel_addr2_p2_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr);

extern UINT32 ime_get_stitching_enable_p2_reg(VOID);
extern UINT32 ime_get_stitching_output_lineoffset_y2_p2_reg(VOID);
extern UINT32 ime_get_stitching_output_lineoffset_c2_p2_reg(VOID);
extern UINT32 ime_get_stitching_output_addr_y2_p2_reg(VOID);
extern UINT32 ime_get_stitching_output_addr_cb2_p2_reg(VOID);


//-------------------------------------------------------------------------------

#ifdef __cplusplus
}  // extern "C" {
#endif

#endif // _IME_OUT_PATH2_REG_


