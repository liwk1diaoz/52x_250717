
/*
    IME module driver

    NT96510 IME internal header file.

    @file       ime_3dnr_base.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/


#ifndef _IME_3DNR_BASE_
#define _IME_3DNR_BASE_

#ifdef __cplusplus
extern "C" {
#endif

#include "ime_platform.h"


//-------------------------------------------------------------------------------
extern BOOL ime_tmnr_in_ref_va2pa_flag;

extern VOID ime_tmnr_set_enable_reg(UINT32 set_en);

extern UINT32 ime_tmnr_get_enable_reg(VOID);

extern VOID ime_tmnr_set_out_ref_enable_reg(UINT32 set_en);

extern VOID ime_tmnr_set_out_ref_encoder_enable_reg(UINT32 set_en);

extern UINT32 ime_tmnr_get_out_ref_encoder_enable_reg(void);

extern VOID ime_tmnr_set_in_ref_decoder_enable_reg(UINT32 set_en);

extern VOID ime_tmnr_set_in_ref_flip_enable_reg(UINT32 set_en);

extern VOID ime_tmnr_set_out_ref_flip_enable_reg(UINT32 set_en);

extern VOID ime_tmnr_set_ms_roi_output_enable_reg(UINT32 set_en);

extern VOID ime_tmnr_set_ms_roi_flip_enable_reg(UINT32 set_en);

extern VOID ime_tmnr_set_nr_luma_channel_enable_reg(UINT32 set_en);

extern VOID ime_tmnr_set_nr_chroma_channel_enable_reg(UINT32 set_en);

extern VOID ime_tmnr_set_statistic_data_output_enable_reg(UINT32 set_en);

extern VOID ime_tmnr_set_debug_reg(UINT32 set_mode, UINT32 set_mv0);

extern VOID ime_tmnr_set_y_pre_blur_strength_reg(UINT32 set_val);

extern VOID ime_tmnr_set_me_control_reg(UINT32 set_update_mode, UINT32 set_bndy, UINT32 set_ds_mode);

extern VOID ime_tmnr_set_me_sad_reg(UINT32 set_sad_type, UINT32 set_sad_shift);

extern VOID ime_tmnr_set_nr_false_color_control_enable_reg(UINT32 set_en);

extern VOID ime_tmnr_set_nr_false_color_control_strength_reg(UINT32 set_val);

extern VOID ime_tmnr_set_nr_center_zero_enable_reg(UINT32 set_en);

extern VOID ime_tmnr_set_ps_mv_check_enable_reg(UINT32 set_en);

extern VOID ime_tmnr_set_ps_mv_roi_check_enable_reg(UINT32 set_en);

extern VOID ime_tmnr_set_ps_control_reg(UINT32 set_mv_info_mode, UINT32 set_ps_mode);

extern VOID ime_tmnr_set_ne_sampling_horizontal_reg(UINT32 set_start_pos, UINT32 set_step, UINT32 set_num);

extern VOID ime_tmnr_set_ne_sampling_vertical_reg(UINT32 set_start_pos, UINT32 set_step, UINT32 set_num);

extern VOID ime_tmnr_set_me_sad_penalty_reg(UINT32 *p_sad_penalty);

extern VOID ime_tmnr_set_me_switch_threshold_reg(UINT32 *p_me_switch_th, UINT32 me_switch_ratio);

extern VOID ime_tmnr_set_me_detail_penalty_reg(UINT32 *p_me_detail_penalty);

extern VOID ime_tmnr_set_me_probability_reg(UINT32 *p_me_probability);

extern VOID ime_tmnr_set_me_rand_bit_reg(UINT32 me_rand_bitx, UINT32 me_rand_bity);

extern VOID ime_tmnr_set_me_threshold_reg(UINT32 me_min_detail, UINT32 me_periodic_th);

extern VOID ime_tmnr_set_md_final_threshold_reg(UINT32 set_md_k1, UINT32 set_md_k2);

extern VOID ime_tmnr_set_md_roi_final_threshold_reg(UINT32 set_md_roi_k1, UINT32 set_md_roi_k2);

extern VOID ime_tmnr_set_md_sad_coefs_reg(UINT32 *p_set_coefa, UINT32 *p_set_coefb);

extern VOID ime_tmnr_set_md_sad_standard_deviation_reg(UINT32 *p_set_std);

extern VOID ime_tmnr_set_mc_noise_base_level_reg(UINT32 *p_set_base_level);

extern VOID ime_tmnr_set_mc_edge_coefs_offset_reg(UINT32 *p_set_coefa, UINT32 *p_set_coefb);

extern VOID ime_tmnr_set_mc_sad_standard_deviation_reg(UINT32 *p_set_std);

extern VOID ime_tmnr_set_mc_final_threshold_reg(UINT32 set_mc_k1, UINT32 set_mc_k2);

extern VOID ime_tmnr_set_mc_roi_final_threshold_reg(UINT32 set_mc_roi_k1, UINT32 set_mc_roi_k2);

extern VOID ime_tmnr_set_ps_mv_threshold_reg(UINT32 set_mv_th, UINT32 set_roi_mv_th);

extern VOID ime_tmnr_set_ps_mix_ratio_reg(UINT32 set_mix_rto0, UINT32 set_mix_rto1);

extern VOID ime_tmnr_set_ps_mix_threshold_reg(UINT32 set_mix_th0, UINT32 set_mix_th1);

extern VOID ime_tmnr_set_ps_mix_slope_reg(UINT32 set_mix_slp0, UINT32 set_mix_slp1);

extern VOID ime_tmnr_set_ps_down_sample_reg(UINT32 set_ds_th);

extern VOID ime_tmnr_set_ps_roi_down_sample_reg(UINT32 set_ds_roi_th);

extern VOID ime_tmnr_set_ps_edge_control_reg(UINT32 set_start_point, UINT32 set_th0, UINT32 set_th1, UINT32 set_slope);

extern VOID ime_tmnr_set_ps_path_error_threshold_reg(UINT32 set_th);

extern VOID ime_tmnr_set_nr_type_reg(UINT32 set_type);

extern VOID ime_tmnr_set_nr_luma_residue_threshold_reg(UINT32 set_th);

extern VOID ime_tmnr_set_nr_chroma_residue_threshold_reg(UINT32 set_th);

extern VOID ime_tmnr_set_nr_varied_frequency_filter_weight_reg(UINT32 *p_set_wets);

extern VOID ime_tmnr_set_nr_luma_filter_weight_reg(UINT32 *p_set_wets);

extern VOID ime_tmnr_set_nr_prefiltering_reg(UINT32 *p_set_strs, UINT32 *p_set_rto);

extern VOID ime_tmnr_set_nr_snr_control_reg(UINT32 *p_set_strs, UINT32 set_th);

extern VOID ime_tmnr_set_nr_tnr_control_reg(UINT32 *p_set_strs, UINT32 set_th);

extern VOID ime_tmnr_set_nr_luma_noise_reduction_reg(UINT32 *p_set_th, UINT32 *p_set_lut);

extern VOID ime_tmnr_set_nr_chroma_noise_reduction_reg(UINT32 *p_set_lut, UINT32 *p_set_rto);


extern VOID ime_tmnr_set_statistic_data_output_start_position_reg(UINT32 set_start_x, UINT32 set_start_y);
extern VOID ime_tmnr_set_statistic_data_output_sample_numbers_reg(UINT32 set_num_x, UINT32 set_num_y);
extern VOID ime_tmnr_set_statistic_data_output_sample_steps_reg(UINT32 set_step_x, UINT32 set_step_y);



extern VOID ime_tmnr_set_in_ref_y_lineoffset_reg(UINT32 set_ofs_y);
extern UINT32 ime_tmnr_get_in_ref_y_lineoffset_reg(VOID);
extern VOID ime_tmnr_set_in_ref_uv_lineoffset_reg(UINT32 set_ofs_uv);
extern UINT32 ime_tmnr_get_in_ref_uv_lineoffset_reg(VOID);
extern VOID ime_tmnr_set_in_ref_y_addr_reg(UINT32 set_addr_y);
extern UINT32 ime_tmnr_get_in_ref_y_addr_reg(VOID);
extern VOID ime_tmnr_set_in_ref_uv_addr_reg(UINT32 set_addr_uv);
extern UINT32 ime_tmnr_get_in_ref_uv_addr_reg(VOID);


extern VOID ime_tmnr_set_out_ref_y_lineoffset_reg(UINT32 set_ofs_y);
extern UINT32 ime_tmnr_get_out_ref_y_lineoffset_reg(VOID);
extern VOID ime_tmnr_set_out_ref_uv_lineoffset_reg(UINT32 set_ofs_uv);
extern UINT32 ime_tmnr_get_out_ref_uv_lineoffset_reg(VOID);
extern VOID ime_tmnr_set_out_ref_y_addr_reg(UINT32 set_addr_y);
extern UINT32 ime_tmnr_get_out_ref_y_addr_reg(VOID);
extern VOID ime_tmnr_set_out_ref_uv_addr_reg(UINT32 set_addr_uv);
extern UINT32 ime_tmnr_get_out_ref_uv_addr_reg(VOID);

extern VOID ime_tmnr_set_in_motion_status_lineoffset_reg(UINT32 set_ofs);
extern UINT32 ime_tmnr_get_in_motion_status_lineoffset_reg(VOID);
extern VOID ime_tmnr_set_in_motion_status_address_reg(UINT32 set_addr);
extern UINT32 ime_tmnr_get_in_motion_status_address_reg(VOID);
extern VOID ime_tmnr_set_out_motion_status_address_reg(UINT32 set_addr);
extern UINT32 ime_tmnr_get_out_motion_status_address_reg(VOID);

extern VOID ime_tmnr_set_out_roi_motion_status_lineoffset_reg(UINT32 set_ofs);
extern UINT32 ime_tmnr_get_out_roi_motion_status_lineoffset_reg(VOID);
extern VOID ime_tmnr_set_out_roi_motion_status_address_reg(UINT32 set_addr);
extern UINT32 ime_tmnr_get_out_roi_motion_status_address_reg(VOID);

extern VOID ime_tmnr_set_in_motion_vector_lineoffset_reg(UINT32 set_ofs);
extern UINT32 ime_tmnr_get_in_motion_vector_lineoffset_reg(VOID);
extern VOID ime_tmnr_set_in_motion_vector_address_reg(UINT32 set_addr);
extern UINT32 ime_tmnr_get_in_motion_vector_address_reg(VOID);
extern VOID ime_tmnr_set_out_motion_vector_address_reg(UINT32 set_addr);
extern UINT32 ime_tmnr_get_out_motion_vector_address_reg(VOID);



extern VOID ime_tmnr_set_out_statistic_lineoffset_reg(UINT32 set_ofs);
extern UINT32 ime_tmnr_get_out_statistic_lineoffset_reg(VOID);
extern VOID ime_tmnr_set_out_statistic_address_reg(UINT32 set_addr);
extern UINT32 ime_tmnr_get_out_statistic_address_reg(VOID);


extern UINT32 ime_tmnr_get_sum_of_sad_value_reg(VOID);
extern UINT32 ime_tmnr_get_sum_of_mv_length_reg(VOID);
extern UINT32 ime_tmnr_get_total_sampling_number_reg(VOID);

extern VOID ime_tmrn_set_ref_in_va2pa_enable_reg(BOOL set_en);


#ifdef __cplusplus
}
#endif


#endif // _IME_3DNR_REG_


