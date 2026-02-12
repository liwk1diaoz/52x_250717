
/*
    IME module driver

    NT96510 IME internal header file.

    @file       ime_in_path_base.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/


#ifndef _IME_IN_PATH_REG_
#define _IME_IN_PATH_REG_

#ifdef __cplusplus
extern "C" {
#endif

#include "ime_platform.h"



extern VOID ime_set_input_image_size_reg(UINT32 set_size_h, UINT32 set_size_v);
extern VOID ime_get_input_image_size_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v);


extern VOID ime_set_input_source_reg(UINT32 in_src_sel);
extern VOID ime_set_input_image_format_reg(UINT32 in_fmt_sel);
extern UINT32 ime_get_input_source_reg(VOID);
extern UINT32 ime_get_input_image_format_reg(VOID);

extern VOID ime_set_stripe_size_mode_reg(UINT32 set_mode);
extern VOID ime_set_horizontal_fixed_stripe_size_reg(UINT32 set_hn, UINT32 set_hl);
extern VOID ime_set_horizontal_stripe_number_reg(UINT32 set_hm);
extern VOID ime_set_horizontal_varied_stripe_size_reg(UINT32 *p_set_hns);
extern VOID ime_set_vertical_stripe_info_reg(UINT32 set_vn, UINT32 set_vl, UINT32 set_vm);
extern VOID ime_set_horizontal_overlap_reg(UINT32 h_overlap_sel, UINT32 h_overlap, UINT32 h_prt_sel, UINT32 h_prt_size);
extern VOID ime_get_horizontal_stripe_info_reg(UINT32 *p_get_hn, UINT32 *p_get_hl, UINT32 *p_get_hm);
extern VOID ime_get_vertical_stripe_info_reg(UINT32 *p_get_vn, UINT32 *p_get_vl, UINT32 *p_get_vm);
extern VOID ime_get_horizontal_overlap_reg(UINT32 *p_get_h_overlap_sel, UINT32 *p_get_h_overlap);
extern UINT32 ime_get_stripe_mode_reg(VOID);


extern VOID ime_set_input_lineoffset_y_reg(UINT32 lofs);
extern VOID ime_set_input_lineoffset_c_reg(UINT32 lofs);
extern UINT32 ime_get_input_lineoffset_y_reg(VOID);
extern UINT32 ime_get_input_lineoffset_c_reg(VOID);


extern VOID ime_set_input_channel_addr1_reg(UINT32 addr_y, UINT32 addr_u, UINT32 addr_v);
extern VOID ime_get_input_channel_addr1_reg(UINT32 *p_get_addr_y, UINT32 *p_get_addr_u, UINT32 *p_get_addr_v);


#ifdef __cplusplus
}
#endif



#endif // _IME_IN_PATH_REG_


