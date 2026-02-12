
/*
    IME module driver

    NT96510 IME internal header file.

    @file       ime_out_path4_base.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/


#ifndef _IME_OUT_PATH4_REG_
#define _IME_OUT_PATH4_REG_

#ifdef __cplusplus
extern "C" {
#endif

#include "ime_platform.h"


// output path4 register APIs
extern VOID ime_set_output_p4_enable_reg(UINT32 set_en);

extern VOID ime_set_flip_enable_p4_reg(UINT32 set_en);

extern VOID ime_set_output_dram_enable_p4_reg(UINT32 set_en);

extern VOID ime_set_output_image_format_p4_reg(UINT32 out_fmt_sel);

extern VOID ime_set_scale_interpolation_method_p4_reg(UINT32 scl_method_sel);

extern VOID ime_set_output_type_p4_reg(UINT32 out_fmt_type);

extern VOID ime_set_scaling_size_p4_reg(UINT32 h_scl_size, UINT32 v_scl_size);
extern VOID ime_get_scaling_size_p4_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v);

extern VOID ime_set_scale_factor_h_p4_reg(UINT32 h_scl_sel, UINT32 h_scl_rate, UINT32 h_scl_factor);
extern VOID ime_set_scale_factor_v_p4_reg(UINT32 v_scl_sel, UINT32 v_scl_rate, UINT32 v_scl_factor);

extern VOID ime_set_output_size_p4_reg(UINT32 set_size_h, UINT32 set_size_v);
extern VOID ime_get_output_size_p4_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v);


extern VOID ime_set_horizontal_scale_filtering_p4_reg(UINT32 h_scl_filter_en, UINT32 h_scl_filter_coef);
extern VOID ime_set_vertical_scale_filtering_p4_reg(UINT32 v_scl_filter_en, UINT32 v_scl_filter_coef);

extern VOID ime_set_integration_scaling_factor0_p4_reg(UINT32 isd_scl_h_factor, UINT32 isd_scl_v_factor);
extern VOID ime_set_integration_scaling_factor1_p4_reg(UINT32 isd_scl_h_factor, UINT32 isd_scl_v_factor);
extern VOID ime_set_integration_scaling_factor2_p4_reg(UINT32 isd_scl_h_factor, UINT32 isd_scl_v_factor);


extern VOID ime_set_scaling_enhance_factor_p4_reg(UINT32 scl_enh_factor, UINT32 scl_enh_sft_bit);

extern VOID ime_set_output_crop_coordinate_p4_reg(UINT32 crop_pos_x, UINT32 crop_pos_y);
extern VOID ime_get_output_crop_coordinate_p4_reg(UINT32 *p_get_cpos_x, UINT32 *p_get_cpos_y);


extern VOID ime_set_output_lineoffset_y_p4_reg(UINT32 lofs_y);
extern VOID ime_set_output_lineoffset_c_p4_reg(UINT32 lofs_c);
extern UINT32 ime_get_output_lineoffset_y_p4_reg(VOID);
extern UINT32 ime_get_output_lineoffset_c_p4_reg(VOID);


extern VOID ime_set_output_channel_addr0_p4_reg(UINT32 out_addr_y, UINT32 out_addr_cb, UINT32 out_addr_cr);
extern UINT32 ime_get_output_addr_y_p4_reg(VOID);
extern UINT32 ime_get_output_addr_cb_p4_reg(VOID);


extern UINT32 ime_get_output_path_enable_status_p4_reg(VOID);

extern UINT32 ime_get_output_format_p4_reg(VOID);
extern UINT32 ime_get_output_format_type_p4_reg(VOID);

extern UINT32 ime_get_scaling_method_p4_reg(VOID);

extern VOID ime_clamp_p4_reg(UINT32 min_y, UINT32 max_y, UINT32 min_uv, UINT32 max_uv);


#ifdef __cplusplus
}  // extern "C" {
#endif


#endif // _IME_OUT_PATH4_REG_


