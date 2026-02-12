
/*
    IFE2 module driver

    NT98520 IFE2 registers header file.

    @file       ife2_base.h
    @ingroup    mIIPPIFE2
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _IFE2_BASE_H_
#define _IFE2_BASE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ife2_reg.h"
#include "ife2_platform.h"

#if 0
#ifdef __KERNEL__

#include "kwrap/type.h"

#else

#include "Type.h"

#endif // end #ifdef __KERNEL__
#endif


#define IFE2_LOADTYPE_START  0
#define IFE2_LOADTYPE_FMEND  1
#define IFE2_LOADTYPE_DIRECT_START    2

#define IFE2_INTP_LL_END     0
#define IFE2_INTP_LL_ERR     1
#define IFE2_INTP_LL_LATE    2
#define IFE2_INTP_LL_JEND    3
#define IFE2_INTP_OUT_OVF    30
#define IFE2_INTP_FRM_END    31
#define IFE2_INTP_ALL        32


extern VOID ife2_set_reset_reg(UINT32 op);
extern VOID ife2_set_start_reg(UINT32 op);
extern UINT32 ife2_get_start_reg(VOID);
extern VOID ife2_set_load_reg(UINT32 load_type, UINT32 op);
extern VOID ife2_set_burst_length_reg(UINT32 in_len, UINT32 out_len);
extern INT32 ife2_get_burst_length_reg(UINT32 get_sel);

extern VOID ife2_set_linked_list_fire_reg(UINT32 set_fire);
extern VOID ife2_set_linked_list_terminate_reg(UINT32 set_terminate);
extern VOID ife2_set_linked_list_dma_addr_reg(UINT32 set_addr);


extern VOID ife2_set_interrupt_enable_reg(UINT32 int_sel, UINT32 int_val);
extern VOID ife2_set_interrupt_status_reg(UINT32 int_sel, UINT32 int_val);
extern UINT32 ife2_get_interrupt_status_reg(VOID);
extern UINT32 ife2_get_interrupt_enable_reg(VOID);



extern VOID ife2_set_input_format_reg(UINT32 in_fmt);
extern VOID ife2_set_output_format_reg(UINT32 out_fmt);
extern UINT32 ife2_get_input_format_reg(VOID);
extern UINT32 ife2_get_output_format_reg(VOID);
extern VOID ife2_set_output_dest_reg(UINT32 out_dest);
extern VOID ife2_set_output_dram_reg(UINT32 set_en);
extern UINT32 ife2_get_output_dest_reg(VOID);
extern VOID ife2_set_filter_ychannel_enable_reg(UINT32 op);
extern VOID ife2_set_filter_size_reg(UINT32 filter_size, UINT32 edge_size);


extern VOID ife2_set_image_size_reg(UINT32 size_h, UINT32 size_v);
extern UINT32 ife2_get_img_size_h_reg(VOID);
extern UINT32 ife2_get_img_size_v_reg(VOID);
extern VOID ife2_set_filter_enable_reg(UINT32 op);


extern VOID ife2_set_input_lineoffset_reg(UINT32 lofs_y, UINT32 lofs_uv);
extern UINT32 ife2_get_input_lineoffset_y_reg(VOID);
extern VOID ife2_set_input_dma_addr_reg(UINT32 addr_y, UINT32 addr_uv);
extern UINT32 ife2_get_input_dmaaddress_y_reg(VOID);


extern VOID ife2_set_output_lineoffset_reg(UINT32 lofs_y, UINT32 lofs_uv);
extern UINT32 ife2_get_output_lineoffset_y_reg(VOID);
extern UINT32 ife2_get_output_lineoffset_uv_reg(VOID);

extern VOID ife2_set_output_dma_addr0_reg(UINT32 addr_y, UINT32 addr_uv);
extern UINT32 ife2_get_output_dma_addr_y0_reg(VOID);


extern VOID ife2_set_reference_center_cal_y_reg(UINT32 *p_range_th_y, UINT32 wet_y, UINT32 *p_range_wet_y, UINT32 outlier_th_y);
extern VOID ife2_set_reference_center_cal_uv_reg(UINT32 *p_range_th_uv, UINT32 wet_uv, UINT32 *p_range_wet_uv, UINT32 outlier_th_uv);
extern VOID ife2_set_outlier_difference_threshold_reg(UINT32 dif_th_y, UINT32 dif_th_u, UINT32 dif_th_v);

extern VOID ife2_set_edge_direction_threshold_reg(UINT32 pn_th, UINT32 hv_th);
extern VOID ife2_set_filter_computation_param_y_reg(UINT32 *p_th_y, UINT32 *p_wet_y);
extern VOID ife2_set_filter_computation_param_u_reg(UINT32 *p_th_u, UINT32 *p_wet_u);
extern VOID ife2_set_filter_computation_param_v_reg(UINT32 *p_th_v, UINT32 *p_wet_v);

extern VOID ife2_set_statistical_information_threshold_reg(UINT32 u_th0, UINT32 u_th1, UINT32 v_th0, UINT32 v_th1);

extern UINT32 ife2_get_statistical_information_sum_u_reg(VOID);
extern UINT32 ife2_get_statistical_information_sum_v_reg(VOID);
extern UINT32 ife2_get_statistical_information_cnt_reg(VOID);

extern UINT32 ife2_get_base_addr_reg(VOID);

extern VOID ife2_set_debug_mode_reg(UINT32 edge_en, UINT32 rc_en, UINT32 eng_en);


#ifdef __cplusplus
}  // extern "C" {
#endif

#endif // _IFE2_BASE_H_


