/*
    IME module driver

    NT96520 IME internal header file.

    @file       ime_adas_base.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/


#ifndef _IME_ADAS_REG_
#define _IME_ADAS_REG_

#ifdef __cplusplus
extern "C" {
#endif

#include "ime_platform.h"


//-------------------------------------------------------------------------------
// statistical
extern VOID ime_set_adas_enable_reg(UINT32 set_en);
extern VOID ime_set_adas_flip_enable_reg(UINT32 set_en);
extern VOID ime_set_adas_median_filter_enable_reg(UINT32 set_en);
extern VOID ime_set_adas_after_filter_out_sel_reg(UINT32 sel);
extern VOID ime_set_adas_row_position_reg(UINT32 row0, UINT32 row1);
extern VOID ime_set_adas_edge_kernel_enable_reg(UINT32 set_en0, UINT32 set_en1);
extern VOID ime_set_adas_edge_kernel_sel_reg(UINT32 ker_sel0, UINT32 ker_sel1);
extern VOID ime_set_adas_edge_kernel_shift_reg(UINT32 ker_sft0, UINT32 ker_sft1);
extern VOID ime_set_adas_histogram_set0_reg(UINT32 pos_h, UINT32 pos_v, UINT32 size_h, UINT32 size_v);
extern VOID ime_set_adas_histogram_set1_reg(UINT32 pos_h, UINT32 pos_v, UINT32 size_h, UINT32 size_v);
extern VOID ime_set_adas_histogram_acc_target_set0_reg(UINT32 acc_tag);
extern VOID ime_set_adas_histogram_acc_target_set1_reg(UINT32 acc_tag);
extern VOID ime_set_adas_roi_threshold0_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src);
extern VOID ime_set_adas_roi_threshold1_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src);
extern VOID ime_set_adas_roi_threshold2_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src);
extern VOID ime_set_adas_roi_threshold3_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src);
extern VOID ime_set_adas_roi_threshold4_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src);
extern VOID ime_set_adas_roi_threshold5_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src);
extern VOID ime_set_adas_roi_threshold6_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src);
extern VOID ime_set_adas_roi_threshold7_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src);
extern VOID ime_set_adas_edge_map_dam_addr_reg(UINT32 addr);
extern VOID ime_set_adas_edge_map_dam_lineoffset_reg(UINT32 lofs);
extern UINT32 ime_get_adas_edge_map_dam_lineoffset_reg(VOID);
extern VOID ime_set_adas_histogram_dam_addr_reg(UINT32 addr);
extern VOID ime_get_adas_max_edge_reg(UINT32 *p_get_max0, UINT32 *p_get_max1);
extern VOID ime_get_adas_histogram_bin_reg(UINT32 *p_get_hist_bin0, UINT32 *p_get_hist_bin1);
extern VOID ime_get_adas_edge_map_addr_reg(UINT32 *p_get_addr);
extern VOID ime_get_adas_histogram_addr_reg(UINT32 *p_get_addr);
extern UINT32 ime_get_adas_enable_reg(VOID);
extern VOID ime_get_adas_burst_length_reg(VOID);




#ifdef __cplusplus
}
#endif


#endif // _IME_ADAS_REG_
