
/*
    IME module driver

    NT96510 IME internal header file.

    @file       ime_control_base.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/


#ifndef _IME_CTRL_REG_
#define _IME_CTRL_REG_

#ifdef __cplusplus
extern "C" {
#endif

#include "ime_platform.h"



#define IME_LOADTYPE_START  0
#define IME_LOADTYPE_FMEND  1
#define IME_LOADTYPE_DIRECT_START    2

//--------------------------------------------------------------

extern VOID ime_load_reg(UINT32 load_type, UINT32 load_op);
extern VOID ime_start_reg(UINT32 op);
extern VOID ime_reset_reg(UINT32 op);
extern UINT32 ime_get_start_status_reg(VOID);

extern VOID ime_set_direct_path_control_reg(UINT32 src_ctrl);
extern UINT32 ime_get_direct_path_control_reg(VOID);

extern VOID ime_set_interrupt_status_reg(UINT32 set_val);
extern VOID ime_set_interrupt_enable_reg(UINT32 set_val);
extern UINT32 ime_get_interrupt_enable_reg(VOID);
extern UINT32 ime_get_interrupt_status_reg(VOID);

extern UINT32 ime_get_function_enable_status_reg(VOID);

extern VOID ime_set_input_path_burst_size_reg(UINT32 bst_y, UINT32 bst_u, UINT32 bst_v);
extern VOID ime_set_input_path_lca_burst_size_reg(UINT32 bst);
extern VOID ime_set_output_path_lca_burst_size_reg(UINT32 bst);
extern VOID ime_set_input_path_osd_set0_burst_size_reg(UINT32 bst);
extern VOID ime_set_input_path_pixelation_burst_size_reg(UINT32 bst);

extern VOID ime_set_input_path_tmnr_burst_size_reg(UINT32 bst0, UINT32 bst1, UINT32 bst2, UINT32 bst3);
extern VOID ime_set_output_path_tmnr_burst_size_reg(UINT32 bst0, UINT32 bst1, UINT32 bst2, UINT32 bst3, UINT32 bst4, UINT32 bst5);

extern VOID ime_set_output_path_p1_burst_size_reg(UINT32 bst_y, UINT32 bst_u, UINT32 bst_v);
extern VOID ime_set_output_path_p2_burst_size_reg(UINT32 bst_y, UINT32 bst_u);
extern VOID ime_set_output_path_p3_burst_size_reg(UINT32 bst_y, UINT32 bst_u);
extern VOID ime_set_output_path_p4_burst_size_reg(UINT32 bst_y);
//extern VOID ime_set_output_path_55_burst_size_reg(UINT32 bst_y, UINT32 bst_u);


//extern VOID ime_show_debug_message_reg(VOID);

extern INT32 ime_get_burst_size_reg(UINT32 chl_num);

extern VOID ime_set_init_reg(VOID);


extern VOID ime_set_linked_list_fire_reg(UINT32 set_fire);
extern VOID ime_set_linked_list_terminate_reg(UINT32 set_terminate);
extern VOID ime_set_linked_list_addr_reg(UINT32 set_addr);

extern VOID ime_set_line_based_break_point_reg(UINT32 set_bp1, UINT32 set_bp2, UINT32 set_bp3);
extern VOID ime_set_pixel_based_break_point_reg(UINT32 set_bp1, UINT32 set_bp2, UINT32 set_bp3);
extern VOID ime_set_break_point_mode_reg(UINT32 set_bp_mode);

extern VOID ime_set_single_output_reg(UINT32 set_en, UINT32 set_ch);
extern VOID ime_get_single_output_reg(UINT32 *p_get_en, UINT32 *p_get_ch);
extern VOID ime_set_low_delay_mode_reg(UINT32 set_en, UINT32 set_sel);

extern UINT32 ime_get_low_delay_mode_reg(VOID);

#ifdef __cplusplus
}
#endif

#endif // _IME_CTRL_REG_

