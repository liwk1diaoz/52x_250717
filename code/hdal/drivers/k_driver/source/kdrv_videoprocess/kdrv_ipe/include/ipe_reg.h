/*
    IPE module register header

    NT98520 IPE module register header

    @file       Ipe_reg.h
    @ingroup    mIDrvIPP_IPE

    Copyright   Novatek Microelectronics Corp. 2019  All rights reserved.
*/
#ifndef __IPE_REG_H_
#define __IPE_REG_H_

// NOTE: XXXXX
#if 1
#include "ipe_platform.h"
#else
#ifdef __KERNEL__
#include "mach/rcw_macro.h"
#include "kwrap/type.h"
#else
#include "DrvCommon.h"
#endif
#endif

// NOTE: XXXXX
// move to ipe_platform.h
#if 0
#ifdef __KERNEL__
extern UINT32 _IPE_REG_BASE_ADDR;
#define IPE_REG_ADDR(ofs)        (_IPE_REG_BASE_ADDR+(ofs))
#define IPE_SETREG(ofs, value)   iowrite32(value, (void *)(_IPE_REG_BASE_ADDR + ofs))
#define IPE_GETREG(ofs)          ioread32((void *)(_IPE_REG_BASE_ADDR + ofs))
#else
#define _IPE_REG_BASE_ADDR       IOADDR_IPE_REG_BASE
#define IPE_REG_ADDR(ofs)        (_IPE_REG_BASE_ADDR+(ofs))
#define IPE_SETREG(ofs, value)   OUTW((_IPE_REG_BASE_ADDR + ofs), (value))
#define IPE_GETREG(ofs)          INW(_IPE_REG_BASE_ADDR + ofs)
#endif
#endif

/*
    reg_ipe_swrst      :    [0x0, 0x1],         bits : 0
    reg_ipe_start      :    [0x0, 0x1],         bits : 1
    reg_ipe_load_start :    [0x0, 0x1],         bits : 2
    reg_ipe_load_fd    :    [0x0, 0x1],         bits : 3
    reg_ipe_load_fs    :    [0x0, 0x1],         bits : 4
    reg_ipe_rwgamma    :    [0x0, 0x3],         bits : 9_8
    reg_ipe_rwgamma_opt:    [0x0, 0x3],         bits : 11_10
    reg_ll_fire        :    [0x0, 0x1],         bits : 28
*/
#define IPE_CONTROL_REGISTER_OFS 0x0000
REGDEF_BEGIN(IPE_CONTROL_REGISTER)
REGDEF_BIT(reg_ipe_swrst,           1)
REGDEF_BIT(reg_ipe_start,           1)
REGDEF_BIT(reg_ipe_load_start,      1)
REGDEF_BIT(reg_ipe_load_fd,         1)
REGDEF_BIT(reg_ipe_load_fs,         1)
REGDEF_BIT(,                        3)
REGDEF_BIT(reg_ipe_rwgamma,         2)
REGDEF_BIT(reg_ipe_rwgamma_opt,     2)
REGDEF_BIT(,                       16)
REGDEF_BIT(reg_ll_fire,             1)
REGDEF_END(IPE_CONTROL_REGISTER)


/*
    reg_ipe_mode       :    [0x0, 0x3],         bits : 1_0
    reg_ipe_imeon      :    [0x0, 0x1],         bits : 2
    reg_ipe_dmaon      :    [0x0, 0x1],         bits : 3
    reg_dmao_sel       :    [0x0, 0x3],         bits : 5_4
    reg_ipe_imat       :    [0x0, 0x3],         bits : 9_8
    reg_ipe_omat       :    [0x0, 0x3],         bits : 11_10
    reg_ipe_subhsel    :    [0x0, 0x1],         bits : 12
    reg_ipe_mst_hovlp  :    [0x0, 0x1],         bits : 27
    reg_iny_burst_mode :    [0x0, 0x1],         bits : 28
    reg_inc_burst_mode :    [0x0, 0x1],         bits : 29
    reg_outy_burst_mode:    [0x0, 0x1],         bits : 30
    reg_outc_burst_mode:    [0x0, 0x1],         bits : 31
*/
#define IPE_MODE_REGISTER_0_OFS 0x0004
REGDEF_BEGIN(IPE_MODE_REGISTER_0)
REGDEF_BIT(reg_ipe_mode,            2)
REGDEF_BIT(reg_ipe_imeon,           1)
REGDEF_BIT(reg_ipe_dmaon,           1)
REGDEF_BIT(reg_dmao_sel,            2)
REGDEF_BIT(,                        2)
REGDEF_BIT(reg_ipe_imat,            2)
REGDEF_BIT(reg_ipe_omat,            2)
REGDEF_BIT(reg_ipe_subhsel,         1)
REGDEF_BIT(,                       14)
REGDEF_BIT(reg_ipe_mst_hovlp,       1)
REGDEF_BIT(reg_iny_burst_mode,      1)
REGDEF_BIT(reg_inc_burst_mode,      1)
REGDEF_BIT(reg_outy_burst_mode,     1)
REGDEF_BIT(reg_outc_burst_mode,     1)
REGDEF_END(IPE_MODE_REGISTER_0)


/*
    reg_rgblpf_en          :    [0x0, 0x1],         bits : 0
    reg_rgbgamma_en        :    [0x0, 0x1],         bits : 1
    reg_ycurve_en          :    [0x0, 0x1],         bits : 2
    reg_cr_en              :    [0x0, 0x1],         bits : 3
    reg_defog_subimg_out_en:    [0x0, 0x1],         bits : 4
    reg_defog_en           :    [0x0, 0x1],         bits : 5
    reg_lce_en             :    [0x0, 0x1],         bits : 6
    reg_cst_en             :    [0x0, 0x1],         bits : 7
    reg_ctrl_en            :    [0x0, 0x1],         bits : 9
    reg_hadj_en            :    [0x0, 0x1],         bits : 10
    reg_cadj_en            :    [0x0, 0x1],         bits : 11
    reg_cadj_yenh_en       :    [0x0, 0x1],         bits : 12
    reg_cadj_ycon_en       :    [0x0, 0x1],         bits : 13
    reg_cadj_ccon_en       :    [0x0, 0x1],         bits : 14
    reg_cadj_ycth_en       :    [0x0, 0x1],         bits : 15
    reg_cstprotect_en      :    [0x0, 0x1],         bits : 16
    reg_edge_dbg_en        :    [0x0, 0x1],         bits : 17
    reg_vacc_en             :    [0x0, 0x1],         bits : 19
    reg_win0_va_en          :    [0x0, 0x1],         bits : 20
    reg_win1_va_en          :    [0x0, 0x1],         bits : 21
    reg_win2_va_en          :    [0x0, 0x1],         bits : 22
    reg_win3_va_en          :    [0x0, 0x1],         bits : 23
    reg_win4_va_en          :    [0x0, 0x1],         bits : 24
    reg_pfr_en             :    [0x0, 0x1],         bits : 25
*/
#define IPE_MODE_REGISTER_1_OFS 0x0008
REGDEF_BEGIN(IPE_MODE_REGISTER_1)
REGDEF_BIT(reg_rgblpf_en,           1)
REGDEF_BIT(reg_rgbgamma_en,         1)
REGDEF_BIT(reg_ycurve_en,           1)
REGDEF_BIT(reg_cr_en,               1)
REGDEF_BIT(reg_defog_subimg_out_en, 1)
REGDEF_BIT(reg_defog_en,            1)
REGDEF_BIT(reg_lce_en,              1)
REGDEF_BIT(reg_cst_en,              1)
REGDEF_BIT(,                        1)
REGDEF_BIT(reg_ctrl_en,             1)
REGDEF_BIT(reg_hadj_en,             1)
REGDEF_BIT(reg_cadj_en,             1)
REGDEF_BIT(reg_cadj_yenh_en,        1)
REGDEF_BIT(reg_cadj_ycon_en,        1)
REGDEF_BIT(reg_cadj_ccon_en,        1)
REGDEF_BIT(reg_cadj_ycth_en,        1)
REGDEF_BIT(reg_cstprotect_en,       1)
REGDEF_BIT(reg_edge_dbg_en,         1)
REGDEF_BIT(,                        1)
REGDEF_BIT(reg_vacc_en,             1)
REGDEF_BIT(reg_win0_va_en,          1)
REGDEF_BIT(reg_win1_va_en,          1)
REGDEF_BIT(reg_win2_va_en,          1)
REGDEF_BIT(reg_win3_va_en,          1)
REGDEF_BIT(reg_win4_va_en,          1)
REGDEF_BIT(reg_pfr_en,              1)
REGDEF_END(IPE_MODE_REGISTER_1)


/*
    reg_ipe_dram_out0_single_en:    [0x0, 0x1],         bits : 0
    reg_ipe_dram_out1_single_en:    [0x0, 0x1],         bits : 1
    reg_ipe_dram_out2_single_en:    [0x0, 0x1],         bits : 2
    reg_ipe_dram_out3_single_en:    [0x0, 0x1],         bits : 3
    reg_ipe_dram_out_mode      :    [0x0, 0x1],         bits : 4
*/
#define DMA_TO_IPE_REGISTER_0_OFS 0x000c
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_0)
REGDEF_BIT(reg_ipe_dram_out0_single_en,        1)
REGDEF_BIT(reg_ipe_dram_out1_single_en,        1)
REGDEF_BIT(reg_ipe_dram_out2_single_en,        1)
REGDEF_BIT(reg_ipe_dram_out3_single_en,        1)
REGDEF_BIT(reg_ipe_dram_out_mode,              1)
REGDEF_END(DMA_TO_IPE_REGISTER_0)


/*
    reg_ll_terminate:    [0x0, 0x1],            bits : 0
*/
#define LINKED_LIST_REGISTER_OFS 0x0010
REGDEF_BEGIN(LINKED_LIST_REGISTER)
REGDEF_BIT(reg_ll_terminate,        1)
REGDEF_END(LINKED_LIST_REGISTER)


/*
    reg_hn:    [0x0, 0x3ff],            bits : 9_0
    reg_hl:    [0x0, 0x3ff],            bits : 21_12
    reg_hm:    [0x0, 0xf],          bits : 27_24
*/
#define IPE_STRIPE_REGISTER_0_OFS 0x0014
REGDEF_BEGIN(IPE_STRIPE_REGISTER_0)
REGDEF_BIT(reg_hn,         11)
REGDEF_BIT(,                1)
REGDEF_BIT(reg_hl,         11)
REGDEF_BIT(,                1)
REGDEF_BIT(reg_hm,          4)
REGDEF_END(IPE_STRIPE_REGISTER_0)


/*
    reg_vl:    [0x0, 0x1fff],           bits : 12_0
*/
#define IPE_STRIPE_REGISTER_1_OFS 0x0018
REGDEF_BEGIN(IPE_STRIPE_REGISTER_1)
REGDEF_BIT(reg_vl,        13)
REGDEF_END(IPE_STRIPE_REGISTER_1)


/*
    reg_dram_sai_ll:    [0x0, 0x3fffffff],          bits : 31_2
*/
#define DMA_TO_IPE_REGISTER_1_OFS 0x001c
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_1)
REGDEF_BIT(,                            2)
REGDEF_BIT(reg_dram_sai_ll,            30)
REGDEF_END(DMA_TO_IPE_REGISTER_1)


/*
    reg_dram_sai_y:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define DMA_TO_IPE_REGISTER_2_OFS 0x0020
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_2)
REGDEF_BIT(,                            2)
REGDEF_BIT(reg_dram_sai_y,             30)
REGDEF_END(DMA_TO_IPE_REGISTER_2)


/*
    reg_dram_sai_c:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define DMA_TO_IPE_REGISTER_3_OFS 0x0024
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_3)
REGDEF_BIT(,                            2)
REGDEF_BIT(reg_dram_sai_c,             30)
REGDEF_END(DMA_TO_IPE_REGISTER_3)


/*
    reg_dram_sai_ycurve:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define DMA_TO_IPE_REGISTER_4_OFS 0x0028
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_4)
REGDEF_BIT(,                            2)
REGDEF_BIT(reg_dram_sai_ycurve,        30)
REGDEF_END(DMA_TO_IPE_REGISTER_4)


/*
    reg_dram_sai_gamma:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define DMA_TO_IPE_REGISTER_5_OFS 0x002c
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_5)
REGDEF_BIT(,                           2)
REGDEF_BIT(reg_dram_sai_gamma,        30)
REGDEF_END(DMA_TO_IPE_REGISTER_5)


/*
    reg_dram_ofsi_y:    [0x0, 0x3fff],            bits : 15_2
    reg_inrand_en  :    [0x0, 0x1],           bits : 28
    reg_inrand_rst :    [0x0, 0x1],           bits : 29
*/
#define DMA_TO_IPE_REGISTER_6_OFS 0x0030
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_6)
REGDEF_BIT(,                        2)
REGDEF_BIT(reg_dram_ofsi_y,        14)
REGDEF_BIT(,                       12)
REGDEF_BIT(reg_inrand_en,           1)
REGDEF_BIT(reg_inrand_rst,          1)
REGDEF_END(DMA_TO_IPE_REGISTER_6)


/*
    reg_dram_ofsi_c:    [0x0, 0x3fff],            bits : 15_2
*/
#define DMA_TO_IPE_REGISTER_7_OFS 0x0034
REGDEF_BEGIN(DMA_TO_IPE_REGISTER_7)
REGDEF_BIT(,                        2)
REGDEF_BIT(reg_dram_ofsi_c,        14)
REGDEF_END(DMA_TO_IPE_REGISTER_7)


/*
    reg_dram_sao_y:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define IPE_TO_DMA_YCC_CHANNEL_REGISTER_0_OFS 0x0038
REGDEF_BEGIN(IPE_TO_DMA_YCC_CHANNEL_REGISTER_0)
REGDEF_BIT(,                       2)
REGDEF_BIT(reg_dram_sao_y,        30)
REGDEF_END(IPE_TO_DMA_YCC_CHANNEL_REGISTER_0)


/*
    reg_dram_sao_c:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define IPE_TO_DMA_YCC_CHANNEL_REGISTER_1_OFS 0x003c
REGDEF_BEGIN(IPE_TO_DMA_YCC_CHANNEL_REGISTER_1)
REGDEF_BIT(,                       2)
REGDEF_BIT(reg_dram_sao_c,        30)
REGDEF_END(IPE_TO_DMA_YCC_CHANNEL_REGISTER_1)


/*
    reg_dram_ofso_y:    [0x0, 0x3fff],           bits : 15_2
*/
#define IPE_TO_DMA_YCC_CHANNEL_REGISTER_2_OFS 0x0040
REGDEF_BEGIN(IPE_TO_DMA_YCC_CHANNEL_REGISTER_2)
REGDEF_BIT(,                        2)
REGDEF_BIT(reg_dram_ofso_y,        14)
REGDEF_END(IPE_TO_DMA_YCC_CHANNEL_REGISTER_2)


/*
    reg_dram_ofso_c:    [0x0, 0x3fff],           bits : 15_2
*/
#define IPE_TO_DMA_YCC_CHANNEL_REGISTER_3_OFS 0x0044
REGDEF_BEGIN(IPE_TO_DMA_YCC_CHANNEL_REGISTER_3)
REGDEF_BIT(,                        2)
REGDEF_BIT(reg_dram_ofso_c,        14)
REGDEF_END(IPE_TO_DMA_YCC_CHANNEL_REGISTER_3)


/*
    reg_dram_sao_va:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define IPE_TO_DMA_VA_CHANNEL_REGISTER_0_OFS 0x0048
REGDEF_BEGIN(IPE_TO_DMA_VA_CHANNEL_REGISTER_0)
REGDEF_BIT(,                        2)
REGDEF_BIT(reg_dram_sao_va,        30)
REGDEF_END(IPE_TO_DMA_VA_CHANNEL_REGISTER_0)


/*
    reg_dram_ofso_va:    [0x0, 0x3fff],          bits : 15_2
*/
#define IPE_TO_DMA_VA_CHANNEL_REGISTER_1_OFS 0x004c
REGDEF_BEGIN(IPE_TO_DMA_VA_CHANNEL_REGISTER_1)
REGDEF_BIT(,                         2)
REGDEF_BIT(reg_dram_ofso_va,        14)
REGDEF_END(IPE_TO_DMA_VA_CHANNEL_REGISTER_1)


/*
    reg_inte_frmend        :    [0x0, 0x1],         bits : 1
    reg_inte_st            :    [0x0, 0x1],         bits : 2
    reg_inte_frmstart      :    [0x0, 0x1],         bits : 3
    reg_inte_ycoutend      :    [0x0, 0x1],         bits : 4
    reg_inte_dmain0end     :    [0x0, 0x1],         bits : 5
    reg_inte_dfg_subout_end:    [0x0, 0x1],         bits : 6
    reg_inte_vaoutend      :    [0x0, 0x1],         bits : 7
    reg_inte_ll_done       :    [0x0, 0x1],         bits : 8
    reg_inte_ll_job_end    :    [0x0, 0x1],         bits : 9
    reg_inte_ll_error      :    [0x0, 0x1],         bits : 10
    reg_inte_ll_error2     :    [0x0, 0x1],         bits : 11
    reg_inte_frame_err     :    [0x0, 0x1],         bits : 12
*/
#define IPE_INTERRUPT_ENABLE_REGISTER_OFS 0x0050
REGDEF_BEGIN(IPE_INTERRUPT_ENABLE_REGISTER)
REGDEF_BIT(,                           1)
REGDEF_BIT(reg_inte_frmend,            1)
REGDEF_BIT(reg_inte_st,                1)
REGDEF_BIT(reg_inte_frmstart,          1)
REGDEF_BIT(reg_inte_ycoutend,          1)
REGDEF_BIT(reg_inte_dmain0end,         1)
REGDEF_BIT(reg_inte_dfg_subout_end,    1)
REGDEF_BIT(reg_inte_vaoutend,          1)
REGDEF_BIT(reg_inte_ll_done,           1)
REGDEF_BIT(reg_inte_ll_job_end,        1)
REGDEF_BIT(reg_inte_ll_error,          1)
REGDEF_BIT(reg_inte_ll_error2,         1)
REGDEF_BIT(reg_inte_frame_err,         1)
REGDEF_END(IPE_INTERRUPT_ENABLE_REGISTER)


/*
    reg_int_frmend         :    [0x0, 0x1],          bits : 1
    reg_int_st             :    [0x0, 0x1],          bits : 2
    reg_int_frmstart       :    [0x0, 0x1],          bits : 3
    reg_int_ycoutend       :    [0x0, 0x1],          bits : 4
    reg_int_dmain0end      :    [0x0, 0x1],          bits : 5
    reg_int_dfg_subout_end :    [0x0, 0x1],          bits : 6
    reg_int_vaoutend       :    [0x0, 0x1],          bits : 7
    reg_ints_ll_done       :    [0x0, 0x1],          bits : 8
    reg_ints_ll_job_end    :    [0x0, 0x1],          bits : 9
    reg_ints_ll_error      :    [0x0, 0x1],          bits : 10
    reg_ints_ll_error2     :    [0x0, 0x1],          bits : 11
    reg_ints_frame_err     :    [0x0, 0x1],          bits : 12
*/
#define IPE_INTERRUPT_STATUS_REGISTER_OFS 0x0054
REGDEF_BEGIN(IPE_INTERRUPT_STATUS_REGISTER)
REGDEF_BIT(,                           1)
REGDEF_BIT(reg_int_frmend,             1)
REGDEF_BIT(reg_int_st,                 1)
REGDEF_BIT(reg_int_frmstart,           1)
REGDEF_BIT(reg_int_ycoutend,           1)
REGDEF_BIT(reg_int_dmain0end,          1)
REGDEF_BIT(reg_int_dfg_subout_end,     1)
REGDEF_BIT(reg_int_vaoutend,           1)
REGDEF_BIT(reg_ints_ll_done,           1)
REGDEF_BIT(reg_ints_ll_job_end,        1)
REGDEF_BIT(reg_ints_ll_error,          1)
REGDEF_BIT(reg_ints_ll_error2,         1)
REGDEF_BIT(reg_ints_frame_err,         1)
REGDEF_END(IPE_INTERRUPT_STATUS_REGISTER)


/*
    reg_ipestatus       :    [0x0, 0x1],            bits : 0
    reg_hcnt            :    [0x0, 0xf],            bits : 7_4
    reg_status_y        :    [0x0, 0x1],            bits : 12
    reg_status_c        :    [0x0, 0x1],            bits : 13
    reg_fstr_rst_disable:    [0x0, 0x1],            bits : 16
*/
#define DEBUG_STATUS_REGISTER_OFS 0x0058
REGDEF_BEGIN(DEBUG_STATUS_REGISTER)
REGDEF_BIT(reg_ipestatus,          1)
REGDEF_BIT(,                       3)
REGDEF_BIT(reg_hcnt,               4)
REGDEF_BIT(,                       4)
REGDEF_BIT(reg_status_y,           1)
REGDEF_BIT(reg_status_c,           1)
REGDEF_BIT(,                       2)
REGDEF_BIT(reg_fstr_rst_disable,   1)
REGDEF_END(DEBUG_STATUS_REGISTER)


/*
    reg_ll_table_idx_0:    [0x0, 0xff],         bits : 7_0
    reg_ll_table_idx_1:    [0x0, 0xff],         bits : 15_8
    reg_ll_table_idx_2:    [0x0, 0xff],         bits : 23_16
    reg_ll_table_idx_3:    [0x0, 0xff],         bits : 31_24
*/
#define LL_FRAME_REGISTER_0_OFS 0x005c
REGDEF_BEGIN(LL_FRAME_REGISTER_0)
REGDEF_BIT(reg_ll_table_idx_0,        8)
REGDEF_BIT(reg_ll_table_idx_1,        8)
REGDEF_BIT(reg_ll_table_idx_2,        8)
REGDEF_BIT(reg_ll_table_idx_3,        8)
REGDEF_END(LL_FRAME_REGISTER_0)


/*
    reg_ll_table_idx_4:    [0x0, 0xff],         bits : 7_0
    reg_ll_table_idx_5:    [0x0, 0xff],         bits : 15_8
    reg_ll_table_idx_6:    [0x0, 0xff],         bits : 23_16
    reg_ll_table_idx_7:    [0x0, 0xff],         bits : 31_24
*/
#define LL_FRAME_REGISTER_1_OFS 0x0060
REGDEF_BEGIN(LL_FRAME_REGISTER_1)
REGDEF_BIT(reg_ll_table_idx_4,        8)
REGDEF_BIT(reg_ll_table_idx_5,        8)
REGDEF_BIT(reg_ll_table_idx_6,        8)
REGDEF_BIT(reg_ll_table_idx_7,        8)
REGDEF_END(LL_FRAME_REGISTER_1)


/*
    reg_ll_table_idx_8 :    [0x0, 0xff],            bits : 7_0
    reg_ll_table_idx_9 :    [0x0, 0xff],            bits : 15_8
    reg_ll_table_idx_10:    [0x0, 0xff],            bits : 23_16
    reg_ll_table_idx_11:    [0x0, 0xff],            bits : 31_24
*/
#define LL_FRAME_REGISTER_2_OFS 0x0064
REGDEF_BEGIN(LL_FRAME_REGISTER_2)
REGDEF_BIT(reg_ll_table_idx_8,         8)
REGDEF_BIT(reg_ll_table_idx_9,         8)
REGDEF_BIT(reg_ll_table_idx_10,        8)
REGDEF_BIT(reg_ll_table_idx_11,        8)
REGDEF_END(LL_FRAME_REGISTER_2)


/*
    reg_ll_table_idx_12:    [0x0, 0xff],            bits : 7_0
    reg_ll_table_idx_13:    [0x0, 0xff],            bits : 15_8
    reg_ll_table_idx_14:    [0x0, 0xff],            bits : 23_16
    reg_ll_table_idx_15:    [0x0, 0xff],            bits : 31_24
*/
#define LL_FRAME_REGISTER_3_OFS 0x0068
REGDEF_BEGIN(LL_FRAME_REGISTER_3)
REGDEF_BIT(reg_ll_table_idx_12,        8)
REGDEF_BIT(reg_ll_table_idx_13,        8)
REGDEF_BIT(reg_ll_table_idx_14,        8)
REGDEF_BIT(reg_ll_table_idx_15,        8)
REGDEF_END(LL_FRAME_REGISTER_3)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED1_OFS 0x006c
REGDEF_BEGIN(IPE_RESERVED1)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED1)


/*
    reg_eext_chsel :    [0x0, 0x1],         bits : 0
    reg_eext_gamsel:    [0x0, 0x1],         bits : 1
    reg_eextdiv_e7 :    [0x0, 0xf],         bits : 7_4
    reg_eext_enh_e7:    [0x0, 0x1f],            bits : 12_8
    reg_eext0      :    [0x0, 0x3ff],           bits : 25_16
*/
#define EDGE_EXTRACTION_REGISTER_0_OFS 0x0070
REGDEF_BEGIN(EDGE_EXTRACTION_REGISTER_0)
REGDEF_BIT(reg_eext_chsel,        1)
REGDEF_BIT(reg_eext_gamsel,       1)
REGDEF_BIT(,                      2)
REGDEF_BIT(reg_eextdiv_e7,        4)
REGDEF_BIT(reg_eext_enh_e7,       5)
REGDEF_BIT(,                      3)
REGDEF_BIT(reg_eext0,            10)
REGDEF_END(EDGE_EXTRACTION_REGISTER_0)


/*
    reg_eext1:    [0x0, 0x3ff],         bits : 9_0
    reg_eext2:    [0x0, 0x3ff],         bits : 19_10
    reg_eext3:    [0x0, 0x3ff],         bits : 29_20
*/
#define EDGE_EXTRACTION_REGISTER_1_OFS 0x0074
REGDEF_BEGIN(EDGE_EXTRACTION_REGISTER_1)
REGDEF_BIT(reg_eext1,        10)
REGDEF_BIT(reg_eext2,        10)
REGDEF_BIT(reg_eext3,        10)
REGDEF_END(EDGE_EXTRACTION_REGISTER_1)


/*
    reg_eext4:    [0x0, 0x3ff],         bits : 9_0
    reg_eext5:    [0x0, 0x3ff],         bits : 19_10
    reg_eext6:    [0x0, 0x3ff],         bits : 29_20
*/
#define EDGE_EXTRACTION_REGISTER_2_OFS 0x0078
REGDEF_BEGIN(EDGE_EXTRACTION_REGISTER_2)
REGDEF_BIT(reg_eext4,        10)
REGDEF_BIT(reg_eext5,        10)
REGDEF_BIT(reg_eext6,        10)
REGDEF_END(EDGE_EXTRACTION_REGISTER_2)


/*
    reg_eext7:    [0x0, 0x3ff],         bits : 9_0
    reg_eext8:    [0x0, 0x3ff],         bits : 19_10
    reg_eext9:    [0x0, 0x3ff],         bits : 29_20
*/
#define EDGE_EXTRACTION_REGISTER_3_OFS 0x007c
REGDEF_BEGIN(EDGE_EXTRACTION_REGISTER_3)
REGDEF_BIT(reg_eext7,        10)
REGDEF_BIT(reg_eext8,        10)
REGDEF_BIT(reg_eext9,        10)
REGDEF_END(EDGE_EXTRACTION_REGISTER_3)


/*
    reg_eext_enh_e3 :    [0x0, 0x3f],           bits : 5_0
    reg_eext_enh_e5a:    [0x0, 0x3f],           bits : 13_8
    reg_eext_enh_e5b:    [0x0, 0x3f],           bits : 21_16
*/
#define EDGE_EXTRACTION_REGISTER_4_OFS 0x0080
REGDEF_BEGIN(EDGE_EXTRACTION_REGISTER_4)
REGDEF_BIT(reg_eext_enh_e3,         6)
REGDEF_BIT(,                        2)
REGDEF_BIT(reg_eext_enh_e5a,        6)
REGDEF_BIT(,                        2)
REGDEF_BIT(reg_eext_enh_e5b,        6)
REGDEF_END(EDGE_EXTRACTION_REGISTER_4)


/*
    reg_eextdiv_e3 :    [0x0, 0x7],         bits : 2_0
    reg_eextdiv_e5a:    [0x0, 0x7],         bits : 6_4
    reg_eextdiv_e5b:    [0x0, 0x7],         bits : 10_8
    reg_eextdiv_eng:    [0x0, 0xf],         bits : 15_12
    reg_eextdiv_con:    [0x0, 0xf],         bits : 19_16
*/
#define EDGE_EXTRACTION_REGISTER_5_OFS 0x0084
REGDEF_BEGIN(EDGE_EXTRACTION_REGISTER_5)
REGDEF_BIT(reg_eextdiv_e3,         3)
REGDEF_BIT(,                       1)
REGDEF_BIT(reg_eextdiv_e5a,        3)
REGDEF_BIT(,                       1)
REGDEF_BIT(reg_eextdiv_e5b,        3)
REGDEF_BIT(,                       1)
REGDEF_BIT(reg_eextdiv_eng,        4)
REGDEF_BIT(reg_eextdiv_con,        4)
REGDEF_END(EDGE_EXTRACTION_REGISTER_5)


/*
    reg_w_con_eng:    [0x0, 0xf],           bits : 3_0
    reg_w_low    :    [0x0, 0x1f],          bits : 8_4
    reg_w_high   :    [0x0, 0x1f],          bits : 16_12
*/
#define EDGE_REGION_EXTRACTION_REGISTER_0_OFS 0x0088
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_REGISTER_0)
REGDEF_BIT(reg_w_con_eng,        4)
REGDEF_BIT(reg_w_low,            5)
REGDEF_BIT(,                     3)
REGDEF_BIT(reg_w_high,           5)
REGDEF_END(EDGE_REGION_EXTRACTION_REGISTER_0)


/*
    reg_w_ker_thin    :    [0x0, 0xf],          bits : 3_0
    reg_w_ker_robust  :    [0x0, 0xf],          bits : 7_4
    reg_iso_ker_thin  :    [0x0, 0xf],          bits : 11_8
    reg_iso_ker_robust:    [0x0, 0xf],          bits : 15_12
*/
#define EDGE_REGION_EXTRACTION_REGISTER_1_OFS 0x008c
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_REGISTER_1)
REGDEF_BIT(reg_w_ker_thin,          4)
REGDEF_BIT(reg_w_ker_robust,        4)
REGDEF_BIT(reg_iso_ker_thin,        4)
REGDEF_BIT(reg_iso_ker_robust,      4)
REGDEF_END(EDGE_REGION_EXTRACTION_REGISTER_1)


/*
    reg_w_hld_low         :    [0x0, 0x1f],         bits : 4_0
    reg_w_hld_high        :    [0x0, 0x1f],         bits : 12_8
    reg_w_ker_thin_hld    :    [0x0, 0xf],          bits : 19_16
    reg_w_ker_robust_hld  :    [0x0, 0xf],          bits : 23_20
    reg_iso_ker_thin_hld  :    [0x0, 0xf],          bits : 27_24
    reg_iso_ker_robust_hld:    [0x0, 0xf],          bits : 31_28
*/
#define EDGE_REGION_EXTRACTION_REGISTER_2_OFS 0x0090
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_REGISTER_2)
REGDEF_BIT(reg_w_hld_low,            5)
REGDEF_BIT(,                         3)
REGDEF_BIT(reg_w_hld_high,           5)
REGDEF_BIT(,                         3)
REGDEF_BIT(reg_w_ker_thin_hld,       4)
REGDEF_BIT(reg_w_ker_robust_hld,     4)
REGDEF_BIT(reg_iso_ker_thin_hld,     4)
REGDEF_BIT(reg_iso_ker_robust_hld,   4)
REGDEF_END(EDGE_REGION_EXTRACTION_REGISTER_2)


/*
    reg_th_flat:    [0x0, 0x3ff],           bits : 9_0
    reg_th_edge:    [0x0, 0x3ff],           bits : 21_12
*/
#define EDGE_REGION_EXTRACTION_REGISTER_3_OFS 0x0094
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_REGISTER_3)
REGDEF_BIT(reg_th_flat,        10)
REGDEF_BIT(,                    2)
REGDEF_BIT(reg_th_edge,        10)
REGDEF_END(EDGE_REGION_EXTRACTION_REGISTER_3)


/*
    reg_th_flat_hld:    [0x0, 0x3ff],           bits : 9_0
    reg_th_edge_hld:    [0x0, 0x3ff],           bits : 21_12
    reg_th_lum_hld :    [0x0, 0x3ff],           bits : 31_22
*/
#define EDGE_REGION_EXTRACTION_REGISTER_4_OFS 0x0098
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_REGISTER_4)
REGDEF_BIT(reg_th_flat_hld,        10)
REGDEF_BIT(,                        2)
REGDEF_BIT(reg_th_edge_hld,        10)
REGDEF_BIT(reg_th_lum_hld,         10)
REGDEF_END(EDGE_REGION_EXTRACTION_REGISTER_4)


/*
    reg_slope_con_eng    :    [0x0, 0xffff],            bits : 15_0
    reg_slope_hld_con_eng:    [0x0, 0xffff],            bits : 31_16
*/
#define EDGE_REGION_EXTRACTION_REGISTER_5_OFS 0x009c
REGDEF_BEGIN(EDGE_REGION_EXTRACTION_REGISTER_5)
REGDEF_BIT(reg_slope_con_eng,        16)
REGDEF_BIT(reg_slope_hld_con_eng,        16)
REGDEF_END(EDGE_REGION_EXTRACTION_REGISTER_5)


/*
    reg_overshoot_en :    [0x0, 0x1],           bits : 0
    reg_wt_overshoot :    [0x0, 0xff],          bits : 11_4
    reg_wt_undershoot:    [0x0, 0xff],          bits : 23_16
*/
#define OVERSHOOTING_CONTROL_REGISTER_0_OFS 0x00a0
REGDEF_BEGIN(OVERSHOOTING_CONTROL_REGISTER_0)
REGDEF_BIT(reg_overshoot_en,        1)
REGDEF_BIT(,                        3)
REGDEF_BIT(reg_wt_overshoot,        8)
REGDEF_BIT(,                        4)
REGDEF_BIT(reg_wt_undershoot,       8)
REGDEF_END(OVERSHOOTING_CONTROL_REGISTER_0)


/*
    reg_th_overshoot     :    [0x0, 0xff],          bits : 7_0
    reg_th_undershoot    :    [0x0, 0xff],          bits : 15_8
    reg_th_undershoot_lum:    [0x0, 0xff],          bits : 23_16
    reg_th_undershoot_eng:    [0x0, 0xff],          bits : 31_24
*/
#define OVERSHOOTING_CONTROL_REGISTER_1_OFS 0x00a4
REGDEF_BEGIN(OVERSHOOTING_CONTROL_REGISTER_1)
REGDEF_BIT(reg_th_overshoot,             8)
REGDEF_BIT(reg_th_undershoot,            8)
REGDEF_BIT(reg_th_undershoot_lum,        8)
REGDEF_BIT(reg_th_undershoot_eng,        8)
REGDEF_END(OVERSHOOTING_CONTROL_REGISTER_1)


/*
    reg_clamp_wt_mod_lum:    [0x0, 0xff],           bits : 7_0
    reg_clamp_wt_mod_eng:    [0x0, 0xff],           bits : 15_8
    reg_strength_lum_eng:    [0x0, 0xff],           bits : 23_16
    reg_norm_lum_eng    :    [0x0, 0xff],           bits : 31_24
*/
#define OVERSHOOTING_CONTROL_REGISTER_2_OFS 0x00a8
REGDEF_BEGIN(OVERSHOOTING_CONTROL_REGISTER_2)
REGDEF_BIT(reg_clamp_wt_mod_lum,        8)
REGDEF_BIT(reg_clamp_wt_mod_eng,        8)
REGDEF_BIT(reg_strength_lum_eng,        8)
REGDEF_BIT(reg_norm_lum_eng,            8)
REGDEF_END(OVERSHOOTING_CONTROL_REGISTER_2)


/*
    reg_slope_overshoot :    [0x0, 0x7fff],         bits : 14_0
    reg_slope_undershoot:    [0x0, 0x7fff],         bits : 30_16
*/
#define OVERSHOOTING_CONTROL_REGISTER_3_OFS 0x00ac
REGDEF_BEGIN(OVERSHOOTING_CONTROL_REGISTER_3)
REGDEF_BIT(reg_slope_overshoot,         15)
REGDEF_BIT(,                             1)
REGDEF_BIT(reg_slope_undershoot,        15)
REGDEF_END(OVERSHOOTING_CONTROL_REGISTER_3)


/*
    reg_slope_undershoot_lum:    [0x0, 0x7fff],         bits : 14_0
    reg_slope_undershoot_eng:    [0x0, 0x7fff],         bits : 30_16
*/
#define OVERSHOOTING_CONTROL_REGISTER_4_OFS 0x00b0
REGDEF_BEGIN(OVERSHOOTING_CONTROL_REGISTER_4)
REGDEF_BIT(reg_slope_undershoot_lum,    15)
REGDEF_BIT(,                             1)
REGDEF_BIT(reg_slope_undershoot_eng,    15)
REGDEF_END(OVERSHOOTING_CONTROL_REGISTER_4)


/*
    reg_esthrl:    [0x0, 0x3ff],            bits : 9_0
    reg_esthrh:    [0x0, 0x3ff],            bits : 19_10
    reg_establ:    [0x0, 0x7],          bits : 22_20
    reg_estabh:    [0x0, 0x7],          bits : 26_24
*/
#define EDGE_LUMINANCE_PROCESS_REGISTER_0_OFS 0x00b4
REGDEF_BEGIN(EDGE_LUMINANCE_PROCESS_REGISTER_0)
REGDEF_BIT(reg_esthrl,        10)
REGDEF_BIT(reg_esthrh,        10)
REGDEF_BIT(reg_establ,         3)
REGDEF_BIT(,                   1)
REGDEF_BIT(reg_estabh,         3)
REGDEF_END(EDGE_LUMINANCE_PROCESS_REGISTER_0)


/*
    reg_eslutl_0:    [0x0, 0xff],            bits : 7_0
    reg_eslutl_1:    [0x0, 0xff],            bits : 15_8
    reg_eslutl_2:    [0x0, 0xff],            bits : 23_16
    reg_eslutl_3:    [0x0, 0xff],            bits : 31_24
*/
#define EDGE_LUMINANCE_PROCESS_REGISTER_1_OFS 0x00b8
REGDEF_BEGIN(EDGE_LUMINANCE_PROCESS_REGISTER_1)
REGDEF_BIT(reg_eslutl_0,        8)
REGDEF_BIT(reg_eslutl_1,        8)
REGDEF_BIT(reg_eslutl_2,        8)
REGDEF_BIT(reg_eslutl_3,        8)
REGDEF_END(EDGE_LUMINANCE_PROCESS_REGISTER_1)


/*
    reg_eslutl_4:    [0x0, 0xff],            bits : 7_0
    reg_eslutl_5:    [0x0, 0xff],            bits : 15_8
    reg_eslutl_6:    [0x0, 0xff],            bits : 23_16
    reg_eslutl_7:    [0x0, 0xff],            bits : 31_24
*/
#define EDGE_LUMINANCE_PROCESS_REGISTER_2_OFS 0x00bc
REGDEF_BEGIN(EDGE_LUMINANCE_PROCESS_REGISTER_2)
REGDEF_BIT(reg_eslutl_4,        8)
REGDEF_BIT(reg_eslutl_5,        8)
REGDEF_BIT(reg_eslutl_6,        8)
REGDEF_BIT(reg_eslutl_7,        8)
REGDEF_END(EDGE_LUMINANCE_PROCESS_REGISTER_2)


/*
    reg_esluth_0:    [0x0, 0xff],            bits : 7_0
    reg_esluth_1:    [0x0, 0xff],            bits : 15_8
    reg_esluth_2:    [0x0, 0xff],            bits : 23_16
    reg_esluth_3:    [0x0, 0xff],            bits : 31_24
*/
#define EDGE_LUMINANCE_PROCESS_REGISTER_3_OFS 0x00c0
REGDEF_BEGIN(EDGE_LUMINANCE_PROCESS_REGISTER_3)
REGDEF_BIT(reg_esluth_0,        8)
REGDEF_BIT(reg_esluth_1,        8)
REGDEF_BIT(reg_esluth_2,        8)
REGDEF_BIT(reg_esluth_3,        8)
REGDEF_END(EDGE_LUMINANCE_PROCESS_REGISTER_3)


/*
    reg_esluth_4:    [0x0, 0xff],            bits : 7_0
    reg_esluth_5:    [0x0, 0xff],            bits : 15_8
    reg_esluth_6:    [0x0, 0xff],            bits : 23_16
    reg_esluth_7:    [0x0, 0xff],            bits : 31_24
*/
#define EDGE_LUMINANCE_PROCESS_REGISTER_4_OFS 0x00c4
REGDEF_BEGIN(EDGE_LUMINANCE_PROCESS_REGISTER_4)
REGDEF_BIT(reg_esluth_4,        8)
REGDEF_BIT(reg_esluth_5,        8)
REGDEF_BIT(reg_esluth_6,        8)
REGDEF_BIT(reg_esluth_7,        8)
REGDEF_END(EDGE_LUMINANCE_PROCESS_REGISTER_4)


/*
    reg_edthrl :    [0x0, 0x3ff],           bits : 9_0
    reg_edthrh :    [0x0, 0x3ff],           bits : 19_10
    reg_edtabl :    [0x0, 0x7],         bits : 22_20
    reg_edtabh :    [0x0, 0x7],         bits : 26_24
    reg_edin_sel:    [0x0, 0x3],         bits : 29_28
*/
#define EDGE_DMAP_PROCESS_REGISTER_0_OFS 0x00c8
REGDEF_BEGIN(EDGE_DMAP_PROCESS_REGISTER_0)
REGDEF_BIT(reg_edthrl,        10)
REGDEF_BIT(reg_edthrh,        10)
REGDEF_BIT(reg_edtabl,        3)
REGDEF_BIT(,                  1)
REGDEF_BIT(reg_edtabh,        3)
REGDEF_BIT(,                  1)
REGDEF_BIT(reg_edin_sel,      2)
REGDEF_END(EDGE_DMAP_PROCESS_REGISTER_0)


/*
    reg_edlutl_0:    [0x0, 0xff],            bits : 7_0
    reg_edlutl_1:    [0x0, 0xff],            bits : 15_8
    reg_edlutl_2:    [0x0, 0xff],            bits : 23_16
    reg_edlutl_3:    [0x0, 0xff],            bits : 31_24
*/
#define EDGE_DMAP_PROCESS_REGISTER_1_OFS 0x00cc
REGDEF_BEGIN(EDGE_DMAP_PROCESS_REGISTER_1)
REGDEF_BIT(reg_edlutl_0,        8)
REGDEF_BIT(reg_edlutl_1,        8)
REGDEF_BIT(reg_edlutl_2,        8)
REGDEF_BIT(reg_edlutl_3,        8)
REGDEF_END(EDGE_DMAP_PROCESS_REGISTER_1)


/*
    reg_edlutl_4:    [0x0, 0xff],            bits : 7_0
    reg_edlutl_5:    [0x0, 0xff],            bits : 15_8
    reg_edlutl_6:    [0x0, 0xff],            bits : 23_16
    reg_edlutl_7:    [0x0, 0xff],            bits : 31_24
*/
#define EDGE_DMAP_PROCESS_REGISTER_2_OFS 0x00d0
REGDEF_BEGIN(EDGE_DMAP_PROCESS_REGISTER_2)
REGDEF_BIT(reg_edlutl_4,        8)
REGDEF_BIT(reg_edlutl_5,        8)
REGDEF_BIT(reg_edlutl_6,        8)
REGDEF_BIT(reg_edlutl_7,        8)
REGDEF_END(EDGE_DMAP_PROCESS_REGISTER_2)


/*
    reg_edluth_0:    [0x0, 0xff],            bits : 7_0
    reg_edluth_1:    [0x0, 0xff],            bits : 15_8
    reg_edluth_2:    [0x0, 0xff],            bits : 23_16
    reg_edluth_3:    [0x0, 0xff],            bits : 31_24
*/
#define EDGE_DMAP_PROCESS_REGISTER_3_OFS 0x00d4
REGDEF_BEGIN(EDGE_DMAP_PROCESS_REGISTER_3)
REGDEF_BIT(reg_edluth_0,        8)
REGDEF_BIT(reg_edluth_1,        8)
REGDEF_BIT(reg_edluth_2,        8)
REGDEF_BIT(reg_edluth_3,        8)
REGDEF_END(EDGE_DMAP_PROCESS_REGISTER_3)


/*
    reg_edluth_4:    [0x0, 0xff],            bits : 7_0
    reg_edluth_5:    [0x0, 0xff],            bits : 15_8
    reg_edluth_6:    [0x0, 0xff],            bits : 23_16
    reg_edluth_7:    [0x0, 0xff],            bits : 31_24
*/
#define EDGE_DMAP_PROCESS_REGISTER_4_OFS 0x00d8
REGDEF_BEGIN(EDGE_DMAP_PROCESS_REGISTER_4)
REGDEF_BIT(reg_edluth_4,        8)
REGDEF_BIT(reg_edluth_5,        8)
REGDEF_BIT(reg_edluth_6,        8)
REGDEF_BIT(reg_edluth_7,        8)
REGDEF_END(EDGE_DMAP_PROCESS_REGISTER_4)


/*
    localmax_statistics_max:    [0x0, 0x3ff],           bits : 9_0
    coneng_statistics_max  :    [0x0, 0x3ff],           bits : 19_10
    coneng_statistics_avg  :    [0x0, 0x3ff],           bits : 29_20
    reserved               :    [0x0, 0x3],         bits : 31_30
*/
#define EDGE_STATISTIC_REGISTER_OFS 0x00dc
REGDEF_BEGIN(EDGE_STATISTIC_REGISTER)
REGDEF_BIT(localmax_statistics_max,        10)
REGDEF_BIT(coneng_statistics_max,        10)
REGDEF_BIT(coneng_statistics_avg,        10)
REGDEF_BIT(reserved,        2)
REGDEF_END(EDGE_STATISTIC_REGISTER)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_EDGE_REGION_STRENGTH_REGISTER1_OFS 0x00e0
REGDEF_BEGIN(IPE_EDGE_REGION_STRENGTH_REGISTER1)
REGDEF_BIT(REGION_STR_EN,       1)
REGDEF_BIT(,                    15)
REGDEF_BIT(ENH_THIN,            8)
REGDEF_BIT(ENH_ROBUST,          8)
REGDEF_END(IPE_EDGE_REGION_STRENGTH_REGISTER1)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_EDGE_REGION_STRENGTH_REGISTER2_OFS 0x00e4
REGDEF_BEGIN(IPE_EDGE_REGION_STRENGTH_REGISTER2)
REGDEF_BIT(SLOPE_FLAT,        16)
REGDEF_BIT(SLOPE_EDGE,        16)
REGDEF_END(IPE_EDGE_REGION_STRENGTH_REGISTER2)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_EDGE_REGION_STRENGTH_REGISTER3_OFS 0x00e8
REGDEF_BEGIN(IPE_EDGE_REGION_STRENGTH_REGISTER3)
REGDEF_BIT(STR_FLAT,        8)
REGDEF_BIT(STR_EDGE,        8)
REGDEF_END(IPE_EDGE_REGION_STRENGTH_REGISTER3)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED5_OFS 0x00ec
REGDEF_BEGIN(IPE_RESERVED5)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED5)


/*
    reg_glpfw    :    [0x0, 0xf],           bits : 3_0
    reg_gsonlyw  :    [0x0, 0xf],           bits : 7_4
    reg_g_rangeth0:    [0x0, 0x3ff],         bits : 17_8
    reg_g_rangeth1:    [0x0, 0x3ff],         bits : 29_20
    reg_g_filtsize:    [0x0, 0x3],           bits : 31_30
*/
#define RGB_LPF_REGISTER_0_OFS 0x00f0
REGDEF_BEGIN(RGB_LPF_REGISTER_0)
REGDEF_BIT(reg_glpfw,             4)
REGDEF_BIT(reg_gsonlyw,           4)
REGDEF_BIT(reg_g_rangeth0,        10)
REGDEF_BIT(,                      2)
REGDEF_BIT(reg_g_rangeth1,        10)
REGDEF_BIT(reg_g_filtsize,        2)
REGDEF_END(RGB_LPF_REGISTER_0)


/*
    reg_rlpfw    :    [0x0, 0xf],           bits : 3_0
    reg_rsonlyw  :    [0x0, 0xf],           bits : 7_4
    reg_r_rangeth0:    [0x0, 0x3ff],         bits : 17_8
    reg_r_rangeth1:    [0x0, 0x3ff],         bits : 29_20
    reg_r_filtsize:    [0x0, 0x3],           bits : 31_30
*/
#define RGB_LPF_REGISTER_1_OFS 0x00f4
REGDEF_BEGIN(RGB_LPF_REGISTER_1)
REGDEF_BIT(reg_rlpfw,             4)
REGDEF_BIT(reg_rsonlyw,           4)
REGDEF_BIT(reg_r_rangeth0,        10)
REGDEF_BIT(,                      2)
REGDEF_BIT(reg_r_rangeth1,        10)
REGDEF_BIT(reg_r_filtsize,        2)
REGDEF_END(RGB_LPF_REGISTER_1)


/*
    reg_blpfw    :    [0x0, 0xf],           bits : 3_0
    reg_bsonlyw  :    [0x0, 0xf],           bits : 7_4
    reg_b_rangeth0:    [0x0, 0x3ff],         bits : 17_8
    reg_b_rangeth1:    [0x0, 0x3ff],         bits : 29_20
    reg_b_filtsize:    [0x0, 0x3],           bits : 31_30
*/
#define RGB_LPF_REGISTER_2_OFS 0x00f8
REGDEF_BEGIN(RGB_LPF_REGISTER_2)
REGDEF_BIT(reg_blpfw,             4)
REGDEF_BIT(reg_bsonlyw,           4)
REGDEF_BIT(reg_b_rangeth0,        10)
REGDEF_BIT(,                      2)
REGDEF_BIT(reg_b_rangeth1,        10)
REGDEF_BIT(reg_b_filtsize,        2)
REGDEF_END(RGB_LPF_REGISTER_2)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED6_OFS 0x00fc
REGDEF_BEGIN(IPE_RESERVED6)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED6)


/*
    reg_ccrange   :    [0x0, 0x3],          bits : 1_0
    reg_cc2_sel   :    [0x0, 0x1],          bits : 2
    reg_cc_gamsel :    [0x0, 0x1],          bits : 3
    reg_ccstab_sel:    [0x0, 0x3],          bits : 5_4
    reg_ccofs_sel :    [0x0, 0x3],          bits : 7_6
    reg_coef_rr   :    [0x0, 0xfff],            bits : 27_16
*/
#define COLOR_CORRECTION_REGISTER_0_OFS 0x0100
REGDEF_BEGIN(COLOR_CORRECTION_REGISTER_0)
REGDEF_BIT(reg_ccrange,          2)
REGDEF_BIT(reg_cc2_sel,          1)
REGDEF_BIT(reg_cc_gamsel,        1)
REGDEF_BIT(reg_ccstab_sel,       2)
REGDEF_BIT(reg_ccofs_sel,        2)
REGDEF_BIT(,                     8)
REGDEF_BIT(reg_coef_rr,          12)
REGDEF_END(COLOR_CORRECTION_REGISTER_0)


/*
    reg_coef_rg:    [0x0, 0xfff],           bits : 11_0
    reg_coef_rb:    [0x0, 0xfff],           bits : 27_16
*/
#define COLOR_CORRECTION_REGISTER_1_OFS 0x0104
REGDEF_BEGIN(COLOR_CORRECTION_REGISTER_1)
REGDEF_BIT(reg_coef_rg,        12)
REGDEF_BIT(,                    4)
REGDEF_BIT(reg_coef_rb,        12)
REGDEF_END(COLOR_CORRECTION_REGISTER_1)


/*
    reg_coef_gr:    [0x0, 0xfff],           bits : 11_0
    reg_coef_gg:    [0x0, 0xfff],           bits : 27_16
*/
#define COLOR_CORRECTION_REGISTER_2_OFS 0x0108
REGDEF_BEGIN(COLOR_CORRECTION_REGISTER_2)
REGDEF_BIT(reg_coef_gr,        12)
REGDEF_BIT(,                    4)
REGDEF_BIT(reg_coef_gg,        12)
REGDEF_END(COLOR_CORRECTION_REGISTER_2)


/*
    reg_coef_gb:    [0x0, 0xfff],           bits : 11_0
    reg_coef_br:    [0x0, 0xfff],           bits : 27_16
*/
#define COLOR_CORRECTION_REGISTER_3_OFS 0x010c
REGDEF_BEGIN(COLOR_CORRECTION_REGISTER_3)
REGDEF_BIT(reg_coef_gb,        12)
REGDEF_BIT(,                    4)
REGDEF_BIT(reg_coef_br,        12)
REGDEF_END(COLOR_CORRECTION_REGISTER_3)


/*
    reg_coef_bg:    [0x0, 0xfff],           bits : 11_0
    reg_coef_bb:    [0x0, 0xfff],           bits : 27_16
*/
#define COLOR_CORRECTION_REGISTER_4_OFS 0x0110
REGDEF_BEGIN(COLOR_CORRECTION_REGISTER_4)
REGDEF_BIT(reg_coef_bg,        12)
REGDEF_BIT(,                    4)
REGDEF_BIT(reg_coef_bb,        12)
REGDEF_END(COLOR_CORRECTION_REGISTER_4)


/*
    reg_fstab0:    [0x0, 0xff],         bits : 7_0
    reg_fstab1:    [0x0, 0xff],         bits : 15_8
    reg_fstab2:    [0x0, 0xff],         bits : 23_16
    reg_fstab3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CORRECTION_STAB_MAPPING_REGISTER_0_OFS 0x0114
REGDEF_BEGIN(COLOR_CORRECTION_STAB_MAPPING_REGISTER_0)
REGDEF_BIT(reg_fstab0,        8)
REGDEF_BIT(reg_fstab1,        8)
REGDEF_BIT(reg_fstab2,        8)
REGDEF_BIT(reg_fstab3,        8)
REGDEF_END(COLOR_CORRECTION_STAB_MAPPING_REGISTER_0)


/*
    reg_fstab4:    [0x0, 0xff],         bits : 7_0
    reg_fstab5:    [0x0, 0xff],         bits : 15_8
    reg_fstab6:    [0x0, 0xff],         bits : 23_16
    reg_fstab7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CORRECTION_STAB_MAPPING_REGISTER_1_OFS 0x0118
REGDEF_BEGIN(COLOR_CORRECTION_STAB_MAPPING_REGISTER_1)
REGDEF_BIT(reg_fstab4,        8)
REGDEF_BIT(reg_fstab5,        8)
REGDEF_BIT(reg_fstab6,        8)
REGDEF_BIT(reg_fstab7,        8)
REGDEF_END(COLOR_CORRECTION_STAB_MAPPING_REGISTER_1)


/*
    reg_fstab8 :    [0x0, 0xff],            bits : 7_0
    reg_fstab9 :    [0x0, 0xff],            bits : 15_8
    reg_fstab10:    [0x0, 0xff],            bits : 23_16
    reg_fstab11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CORRECTION_STAB_MAPPING_REGISTER_2_OFS 0x011c
REGDEF_BEGIN(COLOR_CORRECTION_STAB_MAPPING_REGISTER_2)
REGDEF_BIT(reg_fstab8,        8)
REGDEF_BIT(reg_fstab9,        8)
REGDEF_BIT(reg_fstab10,        8)
REGDEF_BIT(reg_fstab11,        8)
REGDEF_END(COLOR_CORRECTION_STAB_MAPPING_REGISTER_2)


/*
    reg_fstab12:    [0x0, 0xff],            bits : 7_0
    reg_fstab13:    [0x0, 0xff],            bits : 15_8
    reg_fstab14:    [0x0, 0xff],            bits : 23_16
    reg_fstab15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CORRECTION_STAB_MAPPING_REGISTER_3_OFS 0x0120
REGDEF_BEGIN(COLOR_CORRECTION_STAB_MAPPING_REGISTER_3)
REGDEF_BIT(reg_fstab12,        8)
REGDEF_BIT(reg_fstab13,        8)
REGDEF_BIT(reg_fstab14,        8)
REGDEF_BIT(reg_fstab15,        8)
REGDEF_END(COLOR_CORRECTION_STAB_MAPPING_REGISTER_3)


/*
    reg_fdtab0:    [0x0, 0xff],         bits : 7_0
    reg_fdtab1:    [0x0, 0xff],         bits : 15_8
    reg_fdtab2:    [0x0, 0xff],         bits : 23_16
    reg_fdtab3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0_OFS 0x0124
REGDEF_BEGIN(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0)
REGDEF_BIT(reg_fdtab0,        8)
REGDEF_BIT(reg_fdtab1,        8)
REGDEF_BIT(reg_fdtab2,        8)
REGDEF_BIT(reg_fdtab3,        8)
REGDEF_END(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0)


/*
    reg_fdtab4:    [0x0, 0xff],         bits : 7_0
    reg_fdtab5:    [0x0, 0xff],         bits : 15_8
    reg_fdtab6:    [0x0, 0xff],         bits : 23_16
    reg_fdtab7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CORRECTION_DTAB_MAPPING_REGISTER_1_OFS 0x0128
REGDEF_BEGIN(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_1)
REGDEF_BIT(reg_fdtab4,        8)
REGDEF_BIT(reg_fdtab5,        8)
REGDEF_BIT(reg_fdtab6,        8)
REGDEF_BIT(reg_fdtab7,        8)
REGDEF_END(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_1)


/*
    reg_fdtab8 :    [0x0, 0xff],            bits : 7_0
    reg_fdtab9 :    [0x0, 0xff],            bits : 15_8
    reg_fdtab10:    [0x0, 0xff],            bits : 23_16
    reg_fdtab11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CORRECTION_DTAB_MAPPING_REGISTER_2_OFS 0x012c
REGDEF_BEGIN(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_2)
REGDEF_BIT(reg_fdtab8,        8)
REGDEF_BIT(reg_fdtab9,        8)
REGDEF_BIT(reg_fdtab10,        8)
REGDEF_BIT(reg_fdtab11,        8)
REGDEF_END(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_2)


/*
    reg_fdtab12:    [0x0, 0xff],            bits : 7_0
    reg_fdtab13:    [0x0, 0xff],            bits : 15_8
    reg_fdtab14:    [0x0, 0xff],            bits : 23_16
    reg_fdtab15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CORRECTION_DTAB_MAPPING_REGISTER_3_OFS 0x0130
REGDEF_BEGIN(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_3)
REGDEF_BIT(reg_fdtab12,        8)
REGDEF_BIT(reg_fdtab13,        8)
REGDEF_BIT(reg_fdtab14,        8)
REGDEF_BIT(reg_fdtab15,        8)
REGDEF_END(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_3)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED7_OFS 0x0134
REGDEF_BEGIN(IPE_RESERVED7)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED7)


/*
    reg_coef_yr :    [0x0, 0x1ff],          bits : 8_0
    reg_coef_yg :    [0x0, 0x1ff],          bits : 17_9
    reg_coef_yb :    [0x0, 0x1ff],          bits : 26_18
    reg_cstp_rat:    [0x0, 0x1f],           bits : 31_27
*/
#define COLOR_SPACE_TRANSFORM_REGISTER0_OFS 0x0138
REGDEF_BEGIN(COLOR_SPACE_TRANSFORM_REGISTER0)
REGDEF_BIT(reg_coef_yr,        9)
REGDEF_BIT(reg_coef_yg,        9)
REGDEF_BIT(reg_coef_yb,        9)
REGDEF_BIT(reg_cstp_rat,        5)
REGDEF_END(COLOR_SPACE_TRANSFORM_REGISTER0)


/*
    reg_coef_ur   :    [0x0, 0x1ff],            bits : 8_0
    reg_coef_ug   :    [0x0, 0x1ff],            bits : 17_9
    reg_coef_ub   :    [0x0, 0x1ff],            bits : 26_18
    reg_cstoff_sel:    [0x0, 0x1],          bits : 28
*/
#define COLOR_SPACE_TRANSFORM_REGISTER1_OFS 0x013c
REGDEF_BEGIN(COLOR_SPACE_TRANSFORM_REGISTER1)
REGDEF_BIT(reg_coef_ur,        9)
REGDEF_BIT(reg_coef_ug,        9)
REGDEF_BIT(reg_coef_ub,        9)
REGDEF_BIT(,                   1)
REGDEF_BIT(reg_cstoff_sel,     1)
REGDEF_END(COLOR_SPACE_TRANSFORM_REGISTER1)


/*
    reg_coef_vr:    [0x0, 0x1ff],           bits : 8_0
    reg_coef_vg:    [0x0, 0x1ff],           bits : 17_9
    reg_coef_vb:    [0x0, 0x1ff],           bits : 26_18
*/
#define COLOR_SPACE_TRANSFORM_REGISTER2_OFS 0x0140
REGDEF_BEGIN(COLOR_SPACE_TRANSFORM_REGISTER2)
REGDEF_BIT(reg_coef_vr,        9)
REGDEF_BIT(reg_coef_vg,        9)
REGDEF_BIT(reg_coef_vb,        9)
REGDEF_END(COLOR_SPACE_TRANSFORM_REGISTER2)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED8_OFS 0x0144
REGDEF_BEGIN(IPE_RESERVED8)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED8)


/*
    reg_int_ofs   :    [0x0, 0xff],         bits : 7_0
    reg_sat_ofs   :    [0x0, 0xff],         bits : 15_8
    reg_chue_roten:    [0x0, 0x1],          bits : 16
    reg_chue_c2gen:    [0x0, 0x1],          bits : 17
    reg_cctrl_sel :    [0x0, 0x3],          bits : 21_20
    reg_vdet_div  :    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_REGISTER_OFS 0x0148
REGDEF_BEGIN(COLOR_CONTROL_REGISTER)
REGDEF_BIT(reg_int_ofs,        8)
REGDEF_BIT(reg_sat_ofs,        8)
REGDEF_BIT(reg_chue_roten,     1)
REGDEF_BIT(reg_chue_c2gen,     1)
REGDEF_BIT(,                   2)
REGDEF_BIT(reg_cctrl_sel,      2)
REGDEF_BIT(,                   2)
REGDEF_BIT(reg_vdet_div,       8)
REGDEF_END(COLOR_CONTROL_REGISTER)


/*
    reg_chuem0:    [0x0, 0xff],         bits : 7_0
    reg_chuem1:    [0x0, 0xff],         bits : 15_8
    reg_chuem2:    [0x0, 0xff],         bits : 23_16
    reg_chuem3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_0_OFS 0x014c
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_0)
REGDEF_BIT(reg_chuem0,        8)
REGDEF_BIT(reg_chuem1,        8)
REGDEF_BIT(reg_chuem2,        8)
REGDEF_BIT(reg_chuem3,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_0)


/*
    reg_chuem4:    [0x0, 0xff],         bits : 7_0
    reg_chuem5:    [0x0, 0xff],         bits : 15_8
    reg_chuem6:    [0x0, 0xff],         bits : 23_16
    reg_chuem7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_1_OFS 0x0150
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_1)
REGDEF_BIT(reg_chuem4,        8)
REGDEF_BIT(reg_chuem5,        8)
REGDEF_BIT(reg_chuem6,        8)
REGDEF_BIT(reg_chuem7,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_1)


/*
    reg_chuem8 :    [0x0, 0xff],            bits : 7_0
    reg_chuem9 :    [0x0, 0xff],            bits : 15_8
    reg_chuem10:    [0x0, 0xff],            bits : 23_16
    reg_chuem11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_2_OFS 0x0154
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_2)
REGDEF_BIT(reg_chuem8,        8)
REGDEF_BIT(reg_chuem9,        8)
REGDEF_BIT(reg_chuem10,        8)
REGDEF_BIT(reg_chuem11,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_2)


/*
    reg_chuem12:    [0x0, 0xff],            bits : 7_0
    reg_chuem13:    [0x0, 0xff],            bits : 15_8
    reg_chuem14:    [0x0, 0xff],            bits : 23_16
    reg_chuem15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_3_OFS 0x0158
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_3)
REGDEF_BIT(reg_chuem12,        8)
REGDEF_BIT(reg_chuem13,        8)
REGDEF_BIT(reg_chuem14,        8)
REGDEF_BIT(reg_chuem15,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_3)


/*
    reg_chuem16:    [0x0, 0xff],            bits : 7_0
    reg_chuem17:    [0x0, 0xff],            bits : 15_8
    reg_chuem18:    [0x0, 0xff],            bits : 23_16
    reg_chuem19:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_4_OFS 0x015c
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_4)
REGDEF_BIT(reg_chuem16,        8)
REGDEF_BIT(reg_chuem17,        8)
REGDEF_BIT(reg_chuem18,        8)
REGDEF_BIT(reg_chuem19,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_4)


/*
    reg_chuem20:    [0x0, 0xff],            bits : 7_0
    reg_chuem21:    [0x0, 0xff],            bits : 15_8
    reg_chuem22:    [0x0, 0xff],            bits : 23_16
    reg_chuem23:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_HUE_MAPPING_REGISTER_5_OFS 0x0160
REGDEF_BEGIN(COLOR_CONTROL_HUE_MAPPING_REGISTER_5)
REGDEF_BIT(reg_chuem20,        8)
REGDEF_BIT(reg_chuem21,        8)
REGDEF_BIT(reg_chuem22,        8)
REGDEF_BIT(reg_chuem23,        8)
REGDEF_END(COLOR_CONTROL_HUE_MAPPING_REGISTER_5)


/*
    reg_cintm0:    [0x0, 0xff],         bits : 7_0
    reg_cintm1:    [0x0, 0xff],         bits : 15_8
    reg_cintm2:    [0x0, 0xff],         bits : 23_16
    reg_cintm3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_0_OFS 0x0164
REGDEF_BEGIN(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_0)
REGDEF_BIT(reg_cintm0,        8)
REGDEF_BIT(reg_cintm1,        8)
REGDEF_BIT(reg_cintm2,        8)
REGDEF_BIT(reg_cintm3,        8)
REGDEF_END(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_0)


/*
    reg_cintm4:    [0x0, 0xff],         bits : 7_0
    reg_cintm5:    [0x0, 0xff],         bits : 15_8
    reg_cintm6:    [0x0, 0xff],         bits : 23_16
    reg_cintm7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_1_OFS 0x0168
REGDEF_BEGIN(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_1)
REGDEF_BIT(reg_cintm4,        8)
REGDEF_BIT(reg_cintm5,        8)
REGDEF_BIT(reg_cintm6,        8)
REGDEF_BIT(reg_cintm7,        8)
REGDEF_END(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_1)


/*
    reg_cintm8 :    [0x0, 0xff],            bits : 7_0
    reg_cintm9 :    [0x0, 0xff],            bits : 15_8
    reg_cintm10:    [0x0, 0xff],            bits : 23_16
    reg_cintm11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_2_OFS 0x016c
REGDEF_BEGIN(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_2)
REGDEF_BIT(reg_cintm8,        8)
REGDEF_BIT(reg_cintm9,        8)
REGDEF_BIT(reg_cintm10,        8)
REGDEF_BIT(reg_cintm11,        8)
REGDEF_END(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_2)


/*
    reg_cintm12:    [0x0, 0xff],            bits : 7_0
    reg_cintm13:    [0x0, 0xff],            bits : 15_8
    reg_cintm14:    [0x0, 0xff],            bits : 23_16
    reg_cintm15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_3_OFS 0x0170
REGDEF_BEGIN(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_3)
REGDEF_BIT(reg_cintm12,        8)
REGDEF_BIT(reg_cintm13,        8)
REGDEF_BIT(reg_cintm14,        8)
REGDEF_BIT(reg_cintm15,        8)
REGDEF_END(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_3)


/*
    reg_cintm16:    [0x0, 0xff],            bits : 7_0
    reg_cintm17:    [0x0, 0xff],            bits : 15_8
    reg_cintm18:    [0x0, 0xff],            bits : 23_16
    reg_cintm19:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_4_OFS 0x0174
REGDEF_BEGIN(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_4)
REGDEF_BIT(reg_cintm16,        8)
REGDEF_BIT(reg_cintm17,        8)
REGDEF_BIT(reg_cintm18,        8)
REGDEF_BIT(reg_cintm19,        8)
REGDEF_END(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_4)


/*
    reg_cintm20:    [0x0, 0xff],            bits : 7_0
    reg_cintm21:    [0x0, 0xff],            bits : 15_8
    reg_cintm22:    [0x0, 0xff],            bits : 23_16
    reg_cintm23:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_5_OFS 0x0178
REGDEF_BEGIN(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_5)
REGDEF_BIT(reg_cintm20,        8)
REGDEF_BIT(reg_cintm21,        8)
REGDEF_BIT(reg_cintm22,        8)
REGDEF_BIT(reg_cintm23,        8)
REGDEF_END(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_5)


/*
    reg_csatm0:    [0x0, 0xff],         bits : 7_0
    reg_csatm1:    [0x0, 0xff],         bits : 15_8
    reg_csatm2:    [0x0, 0xff],         bits : 23_16
    reg_csatm3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0_OFS 0x017c
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0)
REGDEF_BIT(reg_csatm0,        8)
REGDEF_BIT(reg_csatm1,        8)
REGDEF_BIT(reg_csatm2,        8)
REGDEF_BIT(reg_csatm3,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0)


/*
    reg_csatm4:    [0x0, 0xff],         bits : 7_0
    reg_csatm5:    [0x0, 0xff],         bits : 15_8
    reg_csatm6:    [0x0, 0xff],         bits : 23_16
    reg_csatm7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_1_OFS 0x0180
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_1)
REGDEF_BIT(reg_csatm4,        8)
REGDEF_BIT(reg_csatm5,        8)
REGDEF_BIT(reg_csatm6,        8)
REGDEF_BIT(reg_csatm7,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_1)


/*
    reg_csatm8 :    [0x0, 0xff],            bits : 7_0
    reg_csatm9 :    [0x0, 0xff],            bits : 15_8
    reg_csatm10:    [0x0, 0xff],            bits : 23_16
    reg_csatm11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_2_OFS 0x0184
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_2)
REGDEF_BIT(reg_csatm8,        8)
REGDEF_BIT(reg_csatm9,        8)
REGDEF_BIT(reg_csatm10,        8)
REGDEF_BIT(reg_csatm11,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_2)


/*
    reg_csatm12:    [0x0, 0xff],            bits : 7_0
    reg_csatm13:    [0x0, 0xff],            bits : 15_8
    reg_csatm14:    [0x0, 0xff],            bits : 23_16
    reg_csatm15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_3_OFS 0x0188
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_3)
REGDEF_BIT(reg_csatm12,        8)
REGDEF_BIT(reg_csatm13,        8)
REGDEF_BIT(reg_csatm14,        8)
REGDEF_BIT(reg_csatm15,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_3)


/*
    reg_csatm16:    [0x0, 0xff],            bits : 7_0
    reg_csatm17:    [0x0, 0xff],            bits : 15_8
    reg_csatm18:    [0x0, 0xff],            bits : 23_16
    reg_csatm19:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_4_OFS 0x018c
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_4)
REGDEF_BIT(reg_csatm16,        8)
REGDEF_BIT(reg_csatm17,        8)
REGDEF_BIT(reg_csatm18,        8)
REGDEF_BIT(reg_csatm19,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_4)


/*
    reg_csatm20:    [0x0, 0xff],            bits : 7_0
    reg_csatm21:    [0x0, 0xff],            bits : 15_8
    reg_csatm22:    [0x0, 0xff],            bits : 23_16
    reg_csatm23:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_SATURATION_MAPPING_REGISTER_5_OFS 0x0190
REGDEF_BEGIN(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_5)
REGDEF_BIT(reg_csatm20,        8)
REGDEF_BIT(reg_csatm21,        8)
REGDEF_BIT(reg_csatm22,        8)
REGDEF_BIT(reg_csatm23,        8)
REGDEF_END(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_5)


/*
    reg_cedgem0:    [0x0, 0xff],         bits : 7_0
    reg_cedgem1:    [0x0, 0xff],         bits : 15_8
    reg_cedgem2:    [0x0, 0xff],         bits : 23_16
    reg_cedgem3:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_EDGE_MAPPING_REGISTER_0_OFS 0x0194
REGDEF_BEGIN(COLOR_CONTROL_EDGE_MAPPING_REGISTER_0)
REGDEF_BIT(reg_cedgem0,        8)
REGDEF_BIT(reg_cedgem1,        8)
REGDEF_BIT(reg_cedgem2,        8)
REGDEF_BIT(reg_cedgem3,        8)
REGDEF_END(COLOR_CONTROL_EDGE_MAPPING_REGISTER_0)


/*
    reg_cedgem4:    [0x0, 0xff],         bits : 7_0
    reg_cedgem5:    [0x0, 0xff],         bits : 15_8
    reg_cedgem6:    [0x0, 0xff],         bits : 23_16
    reg_cedgem7:    [0x0, 0xff],         bits : 31_24
*/
#define COLOR_CONTROL_EDGE_MAPPING_REGISTER_1_OFS 0x0198
REGDEF_BEGIN(COLOR_CONTROL_EDGE_MAPPING_REGISTER_1)
REGDEF_BIT(reg_cedgem4,        8)
REGDEF_BIT(reg_cedgem5,        8)
REGDEF_BIT(reg_cedgem6,        8)
REGDEF_BIT(reg_cedgem7,        8)
REGDEF_END(COLOR_CONTROL_EDGE_MAPPING_REGISTER_1)


/*
    reg_cedgem8 :    [0x0, 0xff],            bits : 7_0
    reg_cedgem9 :    [0x0, 0xff],            bits : 15_8
    reg_cedgem10:    [0x0, 0xff],            bits : 23_16
    reg_cedgem11:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_EDGE_MAPPING_REGISTER_2_OFS 0x019c
REGDEF_BEGIN(COLOR_CONTROL_EDGE_MAPPING_REGISTER_2)
REGDEF_BIT(reg_cedgem8,        8)
REGDEF_BIT(reg_cedgem9,        8)
REGDEF_BIT(reg_cedgem10,        8)
REGDEF_BIT(reg_cedgem11,        8)
REGDEF_END(COLOR_CONTROL_EDGE_MAPPING_REGISTER_2)


/*
    reg_cedgem12:    [0x0, 0xff],            bits : 7_0
    reg_cedgem13:    [0x0, 0xff],            bits : 15_8
    reg_cedgem14:    [0x0, 0xff],            bits : 23_16
    reg_cedgem15:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_EDGE_MAPPING_REGISTER_3_OFS 0x01a0
REGDEF_BEGIN(COLOR_CONTROL_EDGE_MAPPING_REGISTER_3)
REGDEF_BIT(reg_cedgem12,        8)
REGDEF_BIT(reg_cedgem13,        8)
REGDEF_BIT(reg_cedgem14,        8)
REGDEF_BIT(reg_cedgem15,        8)
REGDEF_END(COLOR_CONTROL_EDGE_MAPPING_REGISTER_3)


/*
    reg_cedgem16:    [0x0, 0xff],            bits : 7_0
    reg_cedgem17:    [0x0, 0xff],            bits : 15_8
    reg_cedgem18:    [0x0, 0xff],            bits : 23_16
    reg_cedgem19:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_EDGE_MAPPING_REGISTER_4_OFS 0x01a4
REGDEF_BEGIN(COLOR_CONTROL_EDGE_MAPPING_REGISTER_4)
REGDEF_BIT(reg_cedgem16,        8)
REGDEF_BIT(reg_cedgem17,        8)
REGDEF_BIT(reg_cedgem18,        8)
REGDEF_BIT(reg_cedgem19,        8)
REGDEF_END(COLOR_CONTROL_EDGE_MAPPING_REGISTER_4)


/*
    reg_cedgem20:    [0x0, 0xff],            bits : 7_0
    reg_cedgem21:    [0x0, 0xff],            bits : 15_8
    reg_cedgem22:    [0x0, 0xff],            bits : 23_16
    reg_cedgem23:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_CONTROL_EDGE_MAPPING_REGISTER_5_OFS 0x01a8
REGDEF_BEGIN(COLOR_CONTROL_EDGE_MAPPING_REGISTER_5)
REGDEF_BIT(reg_cedgem20,        8)
REGDEF_BIT(reg_cedgem21,        8)
REGDEF_BIT(reg_cedgem22,        8)
REGDEF_BIT(reg_cedgem23,        8)
REGDEF_END(COLOR_CONTROL_EDGE_MAPPING_REGISTER_5)


/*
    reg_dds0:    [0x0, 0x3f],           bits : 5_0
    reg_dds1:    [0x0, 0x3f],           bits : 11_6
    reg_dds2:    [0x0, 0x3f],           bits : 17_12
    reg_dds3:    [0x0, 0x3f],           bits : 23_18
*/
#define COLOR_CONTROL_DDS_REGISTER_0_OFS 0x01ac
REGDEF_BEGIN(COLOR_CONTROL_DDS_REGISTER_0)
REGDEF_BIT(reg_dds0,        6)
REGDEF_BIT(reg_dds1,        6)
REGDEF_BIT(reg_dds2,        6)
REGDEF_BIT(reg_dds3,        6)
REGDEF_END(COLOR_CONTROL_DDS_REGISTER_0)


/*
    reg_dds4:    [0x0, 0x3f],           bits : 5_0
    reg_dds5:    [0x0, 0x3f],           bits : 11_6
    reg_dds6:    [0x0, 0x3f],           bits : 17_12
    reg_dds7:    [0x0, 0x3f],           bits : 23_18
*/
#define COLOR_CONTROL_DDS_REGISTER_1_OFS 0x01b0
REGDEF_BEGIN(COLOR_CONTROL_DDS_REGISTER_1)
REGDEF_BIT(reg_dds4,        6)
REGDEF_BIT(reg_dds5,        6)
REGDEF_BIT(reg_dds6,        6)
REGDEF_BIT(reg_dds7,        6)
REGDEF_END(COLOR_CONTROL_DDS_REGISTER_1)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED9_OFS 0x01b4
REGDEF_BEGIN(IPE_RESERVED9)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED9)


/*
    reg_y_enh_p :    [0x0, 0x3ff],          bits : 9_0
    reg_y_enh_n :    [0x0, 0x3ff],          bits : 19_10
    reg_y_einv_p:    [0x0, 0x1],            bits : 30
    reg_y_einv_n:    [0x0, 0x1],            bits : 31
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_0_OFS 0x01b8
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_0)
REGDEF_BIT(reg_y_enh_p,        10)
REGDEF_BIT(reg_y_enh_n,        10)
REGDEF_BIT(,                   10)
REGDEF_BIT(reg_y_einv_p,        1)
REGDEF_BIT(reg_y_einv_n,        1)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_0)


/*
    reg_y_con       :    [0x0, 0xff],           bits : 7_0
    reg_yc_randreset:    [0x0, 0x1],            bits : 8
    reg_y_rand_en   :    [0x0, 0x1],            bits : 9
    reg_y_rand      :    [0x0, 0x7],            bits : 14_12
    reg_y_ethy      :    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS 0x01bc
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1)
REGDEF_BIT(reg_y_con,            8)
REGDEF_BIT(reg_yc_randreset,     1)
REGDEF_BIT(reg_y_rand_en,        1)
REGDEF_BIT(,                     2)
REGDEF_BIT(reg_y_rand,           3)
REGDEF_BIT(,                     1)
REGDEF_BIT(reg_y_ethy,          10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1)


/*
    reg_y_yth1     :    [0x0, 0xff],            bits : 7_0
    reg_y_hit1sel  :    [0x0, 0x1],             bits : 12
    reg_y_nhit1sel :    [0x0, 0x1],             bits : 13
    reg_y_fix1_hit :    [0x0, 0xff],            bits : 23_16
    reg_y_fix1_nhit:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_2_OFS 0x01c0
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_2)
REGDEF_BIT(reg_y_yth1,            8)
REGDEF_BIT(,                      4)
REGDEF_BIT(reg_y_hit1sel,         1)
REGDEF_BIT(reg_y_nhit1sel,        1)
REGDEF_BIT(,                      2)
REGDEF_BIT(reg_y_fix1_hit,        8)
REGDEF_BIT(reg_y_fix1_nhit,       8)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_2)


/*
    reg_y_yth2     :    [0x0, 0xff],            bits : 7_0
    reg_y_hit2sel  :    [0x0, 0x1],             bits : 12
    reg_y_nhit2sel :    [0x0, 0x1],             bits : 13
    reg_y_fix2_hit :    [0x0, 0xff],            bits : 23_16
    reg_y_fix2_nhit:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_3_OFS 0x01c4
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_3)
REGDEF_BIT(reg_y_yth2,            8)
REGDEF_BIT(,                      4)
REGDEF_BIT(reg_y_hit2sel,         1)
REGDEF_BIT(reg_y_nhit2sel,        1)
REGDEF_BIT(,                      2)
REGDEF_BIT(reg_y_fix2_hit,        8)
REGDEF_BIT(reg_y_fix2_nhit,       8)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_3)


/*
    reg_ymask :    [0x0, 0xff],         bits : 7_0
    reg_cbmask:    [0x0, 0xff],         bits : 15_8
    reg_crmask:    [0x0, 0xff],         bits : 23_16
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_4_OFS 0x01c8
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_4)
REGDEF_BIT(reg_ymask,         8)
REGDEF_BIT(reg_cbmask,        8)
REGDEF_BIT(reg_crmask,        8)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_4)


/*
    reg_c_cbofs  :    [0x0, 0xff],          bits : 7_0
    reg_c_crofs  :    [0x0, 0xff],          bits : 15_8
    reg_c_con    :    [0x0, 0xff],          bits : 23_16
    reg_c_rand_en:    [0x0, 0x1],           bits : 27
    reg_c_rand   :    [0x0, 0x7],           bits : 30_28
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS 0x01cc
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5)
REGDEF_BIT(reg_c_cbofs,        8)
REGDEF_BIT(reg_c_crofs,        8)
REGDEF_BIT(reg_c_con,          8)
REGDEF_BIT(,                   3)
REGDEF_BIT(reg_c_rand_en,      1)
REGDEF_BIT(reg_c_rand,         3)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5)


/*
    reg_cb_fix_hit :    [0x0, 0xff],            bits : 7_0
    reg_cr_fix_hit :    [0x0, 0xff],            bits : 15_8
    reg_cb_fix_nhit:    [0x0, 0xff],            bits : 23_16
    reg_cr_fix_nhit:    [0x0, 0xff],            bits : 31_24
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_6_OFS 0x01d0
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_6)
REGDEF_BIT(reg_cb_fix_hit,         8)
REGDEF_BIT(reg_cr_fix_hit,         8)
REGDEF_BIT(reg_cb_fix_nhit,        8)
REGDEF_BIT(reg_cr_fix_nhit,        8)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_6)


/*
    reg_c_eth    :    [0x0, 0x3ff],         bits : 9_0
    reg_c_hitsel :    [0x0, 0x1],           bits : 12
    reg_c_nhitsel:    [0x0, 0x1],           bits : 13
    reg_c_yth_h  :    [0x0, 0xff],          bits : 23_16
    reg_c_yth_l  :    [0x0, 0xff],          bits : 31_24
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_7_OFS 0x01d4
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_7)
REGDEF_BIT(reg_c_eth,             10)
REGDEF_BIT(,                      2)
REGDEF_BIT(reg_c_hitsel,          1)
REGDEF_BIT(reg_c_nhitsel,         1)
REGDEF_BIT(,                      2)
REGDEF_BIT(reg_c_yth_h,           8)
REGDEF_BIT(reg_c_yth_l,           8)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_7)


/*
    reg_c_cbth_h:    [0x0, 0xff],           bits : 7_0
    reg_c_cbth_l:    [0x0, 0xff],           bits : 15_8
    reg_c_crth_h:    [0x0, 0xff],           bits : 23_16
    reg_c_crth_l:    [0x0, 0xff],           bits : 31_24
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_8_OFS 0x01d8
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_8)
REGDEF_BIT(reg_c_cbth_h,        8)
REGDEF_BIT(reg_c_cbth_l,        8)
REGDEF_BIT(reg_c_crth_h,        8)
REGDEF_BIT(reg_c_crth_l,        8)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_8)


/*
    reg_ccontab0:    [0x0, 0x3ff],          bits : 9_0
    reg_ccontab1:    [0x0, 0x3ff],          bits : 19_10
    reg_ccontab2:    [0x0, 0x3ff],          bits : 29_20
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_9_OFS 0x01dc
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_9)
REGDEF_BIT(reg_ccontab0,        10)
REGDEF_BIT(reg_ccontab1,        10)
REGDEF_BIT(reg_ccontab2,        10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_9)


/*
    reg_ccontab3:    [0x0, 0x3ff],          bits : 9_0
    reg_ccontab4:    [0x0, 0x3ff],          bits : 19_10
    reg_ccontab5:    [0x0, 0x3ff],          bits : 29_20
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_10_OFS 0x01e0
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_10)
REGDEF_BIT(reg_ccontab3,        10)
REGDEF_BIT(reg_ccontab4,        10)
REGDEF_BIT(reg_ccontab5,        10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_10)


/*
    reg_ccontab6:    [0x0, 0x3ff],          bits : 9_0
    reg_ccontab7:    [0x0, 0x3ff],          bits : 19_10
    reg_ccontab8:    [0x0, 0x3ff],          bits : 29_20
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_11_OFS 0x01e4
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_11)
REGDEF_BIT(reg_ccontab6,        10)
REGDEF_BIT(reg_ccontab7,        10)
REGDEF_BIT(reg_ccontab8,        10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_11)


/*
    reg_ccontab9 :    [0x0, 0x3ff],         bits : 9_0
    reg_ccontab10:    [0x0, 0x3ff],         bits : 19_10
    reg_ccontab11:    [0x0, 0x3ff],         bits : 29_20
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_12_OFS 0x01e8
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_12)
REGDEF_BIT(reg_ccontab9,        10)
REGDEF_BIT(reg_ccontab10,        10)
REGDEF_BIT(reg_ccontab11,        10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_12)


/*
    reg_ccontab12:    [0x0, 0x3ff],         bits : 9_0
    reg_ccontab13:    [0x0, 0x3ff],         bits : 19_10
    reg_ccontab14:    [0x0, 0x3ff],         bits : 29_20
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_13_OFS 0x01ec
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_13)
REGDEF_BIT(reg_ccontab12,        10)
REGDEF_BIT(reg_ccontab13,        10)
REGDEF_BIT(reg_ccontab14,        10)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_13)


/*
    reg_ccontab15  :    [0x0, 0x3ff],           bits : 9_0
    reg_ccontab16  :    [0x0, 0x3ff],           bits : 19_10
    reg_ccontab_sel:    [0x0, 0x1],         bits : 31
*/
#define COLOR_COMPONENT_ADJUSTMENT_REGISTER_14_OFS 0x01f0
REGDEF_BEGIN(COLOR_COMPONENT_ADJUSTMENT_REGISTER_14)
REGDEF_BIT(reg_ccontab15,        10)
REGDEF_BIT(reg_ccontab16,        10)
REGDEF_BIT(,        11)
REGDEF_BIT(reg_ccontab_sel,        1)
REGDEF_END(COLOR_COMPONENT_ADJUSTMENT_REGISTER_14)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED10_OFS 0x01f4
REGDEF_BEGIN(IPE_RESERVED10)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED10)


/*
    reg_edge_dbg_modesel:    [0x0, 0x3],            bits : 1_0
*/
#define EDGE_DEBUG_REGISTER_OFS 0x01f8
REGDEF_BEGIN(EDGE_DEBUG_REGISTER)
REGDEF_BIT(reg_edge_dbg_modesel,        2)
REGDEF_END(EDGE_DEBUG_REGISTER)


/*
    reg_ycurve_sel:    [0x0, 0x3],          bits : 1_0
*/
#define YCURVE_REFISTER_OFS 0x01fc
REGDEF_BEGIN(YCURVE_REFISTER)
REGDEF_BIT(reg_ycurve_sel,        2)
REGDEF_END(YCURVE_REFISTER)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED11_OFS 0x0200
REGDEF_BEGIN(IPE_RESERVED11)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED11)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED12_OFS 0x0204
REGDEF_BEGIN(IPE_RESERVED12)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED12)


/*
    reg_gamyrand_en   :    [0x0, 0x1],          bits : 0
    reg_gamyrand_reset:    [0x0, 0x1],          bits : 1
    reg_gamyrand_shf  :    [0x0, 0x1],          bits : 4
    reg_defogrnd_opt  :    [0x0, 0x3],          bits : 29_28
    reg_defogrnd_rst  :    [0x0, 0x1],          bits : 30
*/
#define GAMMA_YCURVE_REGISTER_OFS 0x0208
REGDEF_BEGIN(GAMMA_YCURVE_REGISTER)
REGDEF_BIT(reg_gamyrand_en,         1)
REGDEF_BIT(reg_gamyrand_reset,      1)
REGDEF_BIT(,                        2)
REGDEF_BIT(reg_gamyrand_shf,        1)
REGDEF_BIT(,                       23)
REGDEF_BIT(reg_defogrnd_opt,        2)
REGDEF_BIT(reg_defogrnd_rst,        1)
REGDEF_END(GAMMA_YCURVE_REGISTER)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED13_OFS 0x020c
REGDEF_BEGIN(IPE_RESERVED13)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED13)


/*
    reg_eth_outfmt:    [0x0, 0x1],          bits : 1
    reg_ethlow    :    [0x0, 0x3ff],            bits : 11_2
    reg_ethmid    :    [0x0, 0x3ff],            bits : 21_12
    reg_ethhigh   :    [0x0, 0x3ff],            bits : 31_22
*/
#define EDGE_THRESHOLD_REGISTER_0_OFS 0x0210
REGDEF_BEGIN(EDGE_THRESHOLD_REGISTER_0)
REGDEF_BIT(,                    1)
REGDEF_BIT(reg_eth_outfmt,      1)
REGDEF_BIT(reg_ethlow,         10)
REGDEF_BIT(reg_ethmid,         10)
REGDEF_BIT(reg_ethhigh,        10)
REGDEF_END(EDGE_THRESHOLD_REGISTER_0)


/*
    reg_hout_sel  :    [0x0, 0x1],          bits : 0
    reg_vout_sel  :    [0x0, 0x1],          bits : 1
    reg_eth_outsel:    [0x0, 0x3],          bits : 3_2
*/
#define EDGE_THRESHOLD_REGISTER_1_OFS 0x0214
REGDEF_BEGIN(EDGE_THRESHOLD_REGISTER_1)
REGDEF_BIT(reg_hout_sel,        1)
REGDEF_BIT(reg_vout_sel,        1)
REGDEF_BIT(reg_eth_outsel,        2)
REGDEF_END(EDGE_THRESHOLD_REGISTER_1)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED14_OFS 0x0218
REGDEF_BEGIN(IPE_RESERVED14)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED14)


/*
    reg_vdet_gh1_a      :    [0x0, 0x1f],         bits : 4_0
    reg_vdet_gh1_bcd_op :    [0x0, 0x1],          bits : 7
    reg_vdet_gh1_b      :    [0x0, 0x1f],         bits : 12_8
    reg_vdet_gh1_c      :    [0x0, 0xf],          bits : 19_16
    reg_vdet_gh1_d      :    [0x0, 0xf],          bits : 23_20
    reg_vdet_gh1_fsize  :    [0x0, 0x3],          bits : 25_24
    reg_vdet_gh1_div    :    [0x0, 0xf],          bits : 31_28
*/
#define VARIATION_DETECTION_GROUP1_REGISTER_0_OFS 0x021c
REGDEF_BEGIN(VARIATION_DETECTION_GROUP1_REGISTER_0)
REGDEF_BIT(reg_vdet_gh1_a,        5)
REGDEF_BIT(,                      2)
REGDEF_BIT(reg_vdet_gh1_bcd_op,   1)
REGDEF_BIT(reg_vdet_gh1_b,        5)
REGDEF_BIT(,                      3)
REGDEF_BIT(reg_vdet_gh1_c,        4)
REGDEF_BIT(reg_vdet_gh1_d,        4)
REGDEF_BIT(reg_vdet_gh1_fsize,    2)
REGDEF_BIT(,                      2)
REGDEF_BIT(reg_vdet_gh1_div,      4)
REGDEF_END(VARIATION_DETECTION_GROUP1_REGISTER_0)


/*
    reg_vdet_gv1_a      :    [0x0, 0x1f],         bits : 4_0
    reg_vdet_gv1_bcd_op :    [0x0, 0x1],          bits : 7
    reg_vdet_gv1_b      :    [0x0, 0x1f],         bits : 12_8
    reg_vdet_gv1_c      :    [0x0, 0xf],          bits : 19_16
    reg_vdet_gv1_d      :    [0x0, 0xf],          bits : 23_20
    reg_vdet_gv1_fsize  :    [0x0, 0x3],          bits : 25_24
    reg_vdet_gv1_div    :    [0x0, 0xf],          bits : 31_28
*/
#define VARIATION_DETECTION_GROUP1_REGISTER_1_OFS 0x0220
REGDEF_BEGIN(VARIATION_DETECTION_GROUP1_REGISTER_1)
REGDEF_BIT(reg_vdet_gv1_a,        5)
REGDEF_BIT(,                      2)
REGDEF_BIT(reg_vdet_gv1_bcd_op,   1)
REGDEF_BIT(reg_vdet_gv1_b,        5)
REGDEF_BIT(,                      3)
REGDEF_BIT(reg_vdet_gv1_c,        4)
REGDEF_BIT(reg_vdet_gv1_d,        4)
REGDEF_BIT(reg_vdet_gv1_fsize,    2)
REGDEF_BIT(,                      2)
REGDEF_BIT(reg_vdet_gv1_div,      4)
REGDEF_END(VARIATION_DETECTION_GROUP1_REGISTER_1)


/*
    reg_vdet_gh2_a      :    [0x0, 0x1f],         bits : 4_0
    reg_vdet_gh2_bcd_op :    [0x0, 0x1],          bits : 7
    reg_vdet_gh2_b      :    [0x0, 0x1f],         bits : 12_8
    reg_vdet_gh2_c      :    [0x0, 0xf],          bits : 19_16
    reg_vdet_gh2_d      :    [0x0, 0xf],          bits : 23_20
    reg_vdet_gh2_fsize  :    [0x0, 0x3],          bits : 25_24
    reg_vdet_gh2_div    :    [0x0, 0xf],          bits : 31_28
*/
#define VARIATION_DETECTION_GROUP2_REGISTER_0_OFS 0x0224
REGDEF_BEGIN(VARIATION_DETECTION_GROUP2_REGISTER_0)
REGDEF_BIT(reg_vdet_gh2_a,        5)
REGDEF_BIT(,                      2)
REGDEF_BIT(reg_vdet_gh2_bcd_op,   1)
REGDEF_BIT(reg_vdet_gh2_b,        5)
REGDEF_BIT(,                      3)
REGDEF_BIT(reg_vdet_gh2_c,        4)
REGDEF_BIT(reg_vdet_gh2_d,        4)
REGDEF_BIT(reg_vdet_gh2_fsize,    2)
REGDEF_BIT(,                      2)
REGDEF_BIT(reg_vdet_gh2_div,      4)
REGDEF_END(VARIATION_DETECTION_GROUP2_REGISTER_0)


/*
    reg_vdet_gv2_a      :    [0x0, 0x1f],         bits : 4_0
    reg_vdet_gv2_bcd_op :    [0x0, 0x1],          bits : 7
    reg_vdet_gv2_b      :    [0x0, 0x1f],         bits : 12_8
    reg_vdet_gv2_c      :    [0x0, 0xf],          bits : 19_16
    reg_vdet_gv2_d      :    [0x0, 0xf],          bits : 23_20
    reg_vdet_gv2_fsize  :    [0x0, 0x3],          bits : 25_24
    reg_vdet_gv2_div    :    [0x0, 0xf],          bits : 31_28
*/
#define VARIATION_DETECTION_GROUP2_REGISTER_1_OFS 0x0228
REGDEF_BEGIN(VARIATION_DETECTION_GROUP2_REGISTER_1)
REGDEF_BIT(reg_vdet_gv2_a,        5)
REGDEF_BIT(,                      2)
REGDEF_BIT(reg_vdet_gv2_bcd_op,   1)
REGDEF_BIT(reg_vdet_gv2_b,        5)
REGDEF_BIT(,                      3)
REGDEF_BIT(reg_vdet_gv2_c,        4)
REGDEF_BIT(reg_vdet_gv2_d,        4)
REGDEF_BIT(reg_vdet_gv2_fsize,    2)
REGDEF_BIT(,                      2)
REGDEF_BIT(reg_vdet_gv2_div,      4)
REGDEF_END(VARIATION_DETECTION_GROUP2_REGISTER_1)


/*
    reg_vacc_outsel:    [0x0, 0x1],         bits : 0
    reserved   :    [0x0, 0x7],         bits : 3_1
    reg_va_stx     :    [0x0, 0xfff],           bits : 15_4
    reg_va_sty     :    [0x0, 0xfff],           bits : 27_16
*/
#define VARIATION_ACCUMULATION_REGISTER_0_OFS 0x022c
REGDEF_BEGIN(VARIATION_ACCUMULATION_REGISTER_0)
REGDEF_BIT(reg_vacc_outsel,        1)
REGDEF_BIT(reserved,               3)
REGDEF_BIT(reg_va_stx,            12)
REGDEF_BIT(reg_va_sty,            12)
REGDEF_END(VARIATION_ACCUMULATION_REGISTER_0)


/*
    reg_va_g1h_thl:    [0x0, 0xff],          bits : 7_0
    reg_va_g1h_thh:    [0x0, 0xff],          bits : 15_8
    reg_va_g1v_thl:    [0x0, 0xff],          bits : 23_16
    reg_va_g1v_thh:    [0x0, 0xff],          bits : 31_24
*/
#define VARIATION_ACCUMULATION_REGISTER_1_OFS 0x0230
REGDEF_BEGIN(VARIATION_ACCUMULATION_REGISTER_1)
REGDEF_BIT(reg_va_g1h_thl,        8)
REGDEF_BIT(reg_va_g1h_thh,        8)
REGDEF_BIT(reg_va_g1v_thl,        8)
REGDEF_BIT(reg_va_g1v_thh,        8)
REGDEF_END(VARIATION_ACCUMULATION_REGISTER_1)


/*
    reg_va_g2h_thl:    [0x0, 0xff],          bits : 7_0
    reg_va_g2h_thh:    [0x0, 0xff],          bits : 15_8
    reg_va_g2v_thl:    [0x0, 0xff],          bits : 23_16
    reg_va_g2v_thh:    [0x0, 0xff],          bits : 31_24
*/
#define VARIATION_ACCUMULATION_REGISTER_2_OFS 0x0234
REGDEF_BEGIN(VARIATION_ACCUMULATION_REGISTER_2)
REGDEF_BIT(reg_va_g2h_thl,        8)
REGDEF_BIT(reg_va_g2h_thh,        8)
REGDEF_BIT(reg_va_g2v_thl,        8)
REGDEF_BIT(reg_va_g2v_thh,        8)
REGDEF_END(VARIATION_ACCUMULATION_REGISTER_2)


/*
    reg_va_win_szx         :    [0x0, 0x1ff],           bits : 8_0
    reg_va_win_szy         :    [0x0, 0x1ff],           bits : 17_9
    reg_va_g1_line_max_mode:    [0x0, 0x1],         bits : 18
    reg_va_g2_line_max_mode:    [0x0, 0x1],         bits : 19
    reg_va_g1_cnt_en       :    [0x0, 0x1],         bits : 20
    reg_va_g2_cnt_en       :    [0x0, 0x1],         bits : 21
*/
#define VARIATION_ACCUMULATION_REGISTER_3_OFS 0x0238
REGDEF_BEGIN(VARIATION_ACCUMULATION_REGISTER_3)
REGDEF_BIT(reg_va_win_szx,                 9)
REGDEF_BIT(reg_va_win_szy,                 9)
REGDEF_BIT(reg_va_g1_line_max_mode,        1)
REGDEF_BIT(reg_va_g2_line_max_mode,        1)
REGDEF_BIT(reg_va_g1_cnt_en,               1)
REGDEF_BIT(reg_va_g2_cnt_en,               1)
REGDEF_END(VARIATION_ACCUMULATION_REGISTER_3)


/*
    reg_va_win_numx :    [0x0, 0x7],            bits : 2_0
    reg_va_win_numy :    [0x0, 0x7],            bits : 6_4
    reg_va_win_skipx:    [0x0, 0x3f],           bits : 21_16
    reg_va_win_skipy:    [0x0, 0x1f],           bits : 28_24
*/
#define VARIATION_ACCUMULATION_REGISTER_4_OFS 0x023c
REGDEF_BEGIN(VARIATION_ACCUMULATION_REGISTER_4)
REGDEF_BIT(reg_va_win_numx,        3)
REGDEF_BIT(,                       1)
REGDEF_BIT(reg_va_win_numy,        3)
REGDEF_BIT(,                       9)
REGDEF_BIT(reg_va_win_skipx,       6)
REGDEF_BIT(,                       2)
REGDEF_BIT(reg_va_win_skipy,       5)
REGDEF_END(VARIATION_ACCUMULATION_REGISTER_4)


/*
    reg_win0_stx:    [0x0, 0xfff],          bits : 11_0
    reg_win0_sty:    [0x0, 0xfff],          bits : 27_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_0_OFS 0x0240
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_0)
REGDEF_BIT(reg_win0_stx,        12)
REGDEF_BIT(,                    4)
REGDEF_BIT(reg_win0_sty,        12)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_0)


/*
    reg_win0_g1_line_max_mode:    [0x0, 0x1],           bits : 0
    reg_win0_g2_line_max_mode:    [0x0, 0x1],           bits : 1
    reg_win0_hsz             :    [0x0, 0x1ff],         bits : 12_4
    reg_win0_vsz             :    [0x0, 0x1ff],         bits : 24_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_1_OFS 0x0244
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_1)
REGDEF_BIT(reg_win0_g1_line_max_mode,        1)
REGDEF_BIT(reg_win0_g2_line_max_mode,        1)
REGDEF_BIT(,                                 2)
REGDEF_BIT(reg_win0_hsz,                     9)
REGDEF_BIT(,                                 3)
REGDEF_BIT(reg_win0_vsz,                     9)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_1)


/*
    reg_win1_stx:    [0x0, 0xfff],          bits : 11_0
    reg_win1_sty:    [0x0, 0xfff],          bits : 27_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_1_REGISTER_0_OFS 0x0248
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_1_REGISTER_0)
REGDEF_BIT(reg_win1_stx,        12)
REGDEF_BIT(,                    4)
REGDEF_BIT(reg_win1_sty,        12)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_1_REGISTER_0)


/*
    reg_win1_g1_line_max_mode:    [0x0, 0x1],           bits : 0
    reg_win1_g2_line_max_mode:    [0x0, 0x1],           bits : 1
    reg_win1_hsz             :    [0x0, 0x1ff],         bits : 12_4
    reg_win1_vsz             :    [0x0, 0x1ff],         bits : 24_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_1_REGISTER_1_OFS 0x024c
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_1_REGISTER_1)
REGDEF_BIT(reg_win1_g1_line_max_mode,        1)
REGDEF_BIT(reg_win1_g2_line_max_mode,        1)
REGDEF_BIT(,                                 2)
REGDEF_BIT(reg_win1_hsz,                     9)
REGDEF_BIT(,                                 3)
REGDEF_BIT(reg_win1_vsz,                     9)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_1_REGISTER_1)


/*
    reg_win2_stx:    [0x0, 0xfff],          bits : 11_0
    reg_win2_sty:    [0x0, 0xfff],          bits : 27_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_2_REGISTER_0_OFS 0x0250
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_2_REGISTER_0)
REGDEF_BIT(reg_win2_stx,        12)
REGDEF_BIT(,                    4)
REGDEF_BIT(reg_win2_sty,        12)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_2_REGISTER_0)


/*
    reg_win2_g1_line_max_mode:    [0x0, 0x1],           bits : 0
    reg_win2_g2_line_max_mode:    [0x0, 0x1],           bits : 1
    reg_win2_hsz             :    [0x0, 0x1ff],         bits : 12_4
    reg_win2_vsz             :    [0x0, 0x1ff],         bits : 24_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_2_REGISTER_1_OFS 0x0254
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_2_REGISTER_1)
REGDEF_BIT(reg_win2_g1_line_max_mode,        1)
REGDEF_BIT(reg_win2_g2_line_max_mode,        1)
REGDEF_BIT(,                                 2)
REGDEF_BIT(reg_win2_hsz,                     9)
REGDEF_BIT(,                                 3)
REGDEF_BIT(reg_win2_vsz,                     9)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_2_REGISTER_1)


/*
    reg_win3_stx:    [0x0, 0xfff],          bits : 11_0
    reg_win3_sty:    [0x0, 0xfff],          bits : 27_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_3_REGISTER_0_OFS 0x0258
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_3_REGISTER_0)
REGDEF_BIT(reg_win3_stx,        12)
REGDEF_BIT(,                    4)
REGDEF_BIT(reg_win3_sty,        12)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_3_REGISTER_0)


/*
    reg_win3_g1_line_max_mode:    [0x0, 0x1],           bits : 0
    reg_win3_g2_line_max_mode:    [0x0, 0x1],           bits : 1
    reg_win3_hsz             :    [0x0, 0x1ff],         bits : 12_4
    reg_win3_vsz             :    [0x0, 0x1ff],         bits : 24_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_3_REGISTER_1_OFS 0x025c
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_3_REGISTER_1)
REGDEF_BIT(reg_win3_g1_line_max_mode,        1)
REGDEF_BIT(reg_win3_g2_line_max_mode,        1)
REGDEF_BIT(,                                 2)
REGDEF_BIT(reg_win3_hsz,                     9)
REGDEF_BIT(,                                 3)
REGDEF_BIT(reg_win3_vsz,                     9)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_3_REGISTER_1)


/*
    reg_win4_stx:    [0x0, 0xfff],          bits : 11_0
    reg_win4_sty:    [0x0, 0xfff],          bits : 27_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_4_REGISTER_0_OFS 0x0260
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_4_REGISTER_0)
REGDEF_BIT(reg_win4_stx,        12)
REGDEF_BIT(,                    4)
REGDEF_BIT(reg_win4_sty,        12)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_4_REGISTER_0)


/*
    reg_win4_g1_line_max_mode:    [0x0, 0x1],           bits : 0
    reg_win4_g2_line_max_mode:    [0x0, 0x1],           bits : 1
    reg_win4_hsz             :    [0x0, 0x1ff],         bits : 12_4
    reg_win4_vsz             :    [0x0, 0x1ff],         bits : 24_16
*/
#define INDEP_VARIATION_ACCUMULATION_WINDOW_4_REGISTER_1_OFS 0x0264
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_WINDOW_4_REGISTER_1)
REGDEF_BIT(reg_win4_g1_line_max_mode,        1)
REGDEF_BIT(reg_win4_g2_line_max_mode,        1)
REGDEF_BIT(,                                 2)
REGDEF_BIT(reg_win4_hsz,                     9)
REGDEF_BIT(,                                 3)
REGDEF_BIT(reg_win4_vsz,                     9)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_WINDOW_4_REGISTER_1)


/*
    reg_va_win0g1h_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win0g1h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_0_OFS 0x0268
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_0)
REGDEF_BIT(reg_va_win0g1h_vacc,         16)
REGDEF_BIT(reg_va_win0g1h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_0)


/*
    reg_va_win0g1v_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win0g1v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_1_OFS 0x026c
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_1)
REGDEF_BIT(reg_va_win0g1v_vacc,         16)
REGDEF_BIT(reg_va_win0g1v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_1)


/*
    reg_va_win0g2h_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win0g2h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_2_OFS 0x0270
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_2)
REGDEF_BIT(reg_va_win0g2h_vacc,         16)
REGDEF_BIT(reg_va_win0g2h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_2)


/*
    reg_va_win0g2v_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win0g2v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_3_OFS 0x0274
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_3)
REGDEF_BIT(reg_va_win0g2v_vacc,         16)
REGDEF_BIT(reg_va_win0g2v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_3)


/*
    reg_va_win1g1h_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win1g1h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_4_OFS 0x0278
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_4)
REGDEF_BIT(reg_va_win1g1h_vacc,         16)
REGDEF_BIT(reg_va_win1g1h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_4)


/*
    reg_va_win1g1v_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win1g1v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_5_OFS 0x027c
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_5)
REGDEF_BIT(reg_va_win1g1v_vacc,         16)
REGDEF_BIT(reg_va_win1g1v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_5)


/*
    reg_va_win1g2h_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win1g2h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_6_OFS 0x0280
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_6)
REGDEF_BIT(reg_va_win1g2h_vacc,         16)
REGDEF_BIT(reg_va_win1g2h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_6)


/*
    reg_va_win1g2v_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win1g2v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_7_OFS 0x0284
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_7)
REGDEF_BIT(reg_va_win1g2v_vacc,         16)
REGDEF_BIT(reg_va_win1g2v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_7)


/*
    reg_va_win2g1h_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win2g1h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_8_OFS 0x0288
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_8)
REGDEF_BIT(reg_va_win2g1h_vacc,         16)
REGDEF_BIT(reg_va_win2g1h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_8)


/*
    reg_va_win2g1v_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win2g1v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_9_OFS 0x028c
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_9)
REGDEF_BIT(reg_va_win2g1v_vacc,         16)
REGDEF_BIT(reg_va_win2g1v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_9)


/*
    reg_va_win2g2h_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win2g2h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_10_OFS 0x0290
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_10)
REGDEF_BIT(reg_va_win2g2h_vacc,         16)
REGDEF_BIT(reg_va_win2g2h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_10)


/*
    reg_va_win2g2v_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win2g2v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_11_OFS 0x0294
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_11)
REGDEF_BIT(reg_va_win2g2v_vacc,         16)
REGDEF_BIT(reg_va_win2g2v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_11)


/*
    reg_va_win3g1h_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win3g1h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_12_OFS 0x0298
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_12)
REGDEF_BIT(reg_va_win3g1h_vacc,         16)
REGDEF_BIT(reg_va_win3g1h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_12)


/*
    reg_va_win3g1v_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win3g1v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_13_OFS 0x029c
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_13)
REGDEF_BIT(reg_va_win3g1v_vacc,         16)
REGDEF_BIT(reg_va_win3g1v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_13)


/*
    reg_va_win3g2h_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win3g2h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_14_OFS 0x02a0
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_14)
REGDEF_BIT(reg_va_win3g2h_vacc,         16)
REGDEF_BIT(reg_va_win3g2h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_14)


/*
    reg_va_win3g2v_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win3g2v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_15_OFS 0x02a4
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_15)
REGDEF_BIT(reg_va_win3g2v_vacc,         16)
REGDEF_BIT(reg_va_win3g2v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_15)


/*
    reg_va_win4g1h_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win4g1h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_16_OFS 0x02a8
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_16)
REGDEF_BIT(reg_va_win4g1h_vacc,         16)
REGDEF_BIT(reg_va_win4g1h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_16)


/*
    reg_va_win4g1v_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win4g1v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_17_OFS 0x02ac
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_17)
REGDEF_BIT(reg_va_win4g1v_vacc,         16)
REGDEF_BIT(reg_va_win4g1v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_17)


/*
    reg_va_win4g2h_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win4g2h_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_18_OFS 0x02b0
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_18)
REGDEF_BIT(reg_va_win4g2h_vacc,         16)
REGDEF_BIT(reg_va_win4g2h_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_18)


/*
    reg_va_win4g2v_vacc :    [0x0, 0xffff],         bits : 15_0
    reg_va_win4g2v_vacnt:    [0x0, 0xffff],         bits : 31_16
*/
#define INDEP_VARIATION_ACCUMULATION_REGISTER_19_OFS 0x02b4
REGDEF_BEGIN(INDEP_VARIATION_ACCUMULATION_REGISTER_19)
REGDEF_BIT(reg_va_win4g2v_vacc,         16)
REGDEF_BIT(reg_va_win4g2v_vacnt,        16)
REGDEF_END(INDEP_VARIATION_ACCUMULATION_REGISTER_19)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED15_OFS 0x02b8
REGDEF_BEGIN(IPE_RESERVED15)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED15)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED16_OFS 0x02bc
REGDEF_BEGIN(IPE_RESERVED16)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED16)


/*
    reg_defog_subimg_width :    [0x0, 0x1f],            bits : 4_0
    reg_defog_subimg_height:    [0x0, 0x1f],            bits : 9_5
*/
#define DEFOG_SUBIMG_REGISTER_OFS 0x02c0
REGDEF_BEGIN(DEFOG_SUBIMG_REGISTER)
REGDEF_BIT(reg_defog_subimg_width,        5)
REGDEF_BIT(reg_defog_subimg_height,        5)
REGDEF_END(DEFOG_SUBIMG_REGISTER)


/*
    reg_defog_subimg_dramsai:    [0x0, 0x3fffffff],         bits : 31_2
*/
#define DMA_DEFOG_SUBIMG_INPUT_CHANNEL_REGISTER_OFS 0x02c4
REGDEF_BEGIN(DMA_DEFOG_SUBIMG_INPUT_CHANNEL_REGISTER)
REGDEF_BIT(,                                 2)
REGDEF_BIT(reg_defog_subimg_dramsai,        30)
REGDEF_END(DMA_DEFOG_SUBIMG_INPUT_CHANNEL_REGISTER)


/*
    reg_defog_subimg_lofsi:    [0x0, 0x3fff],           bits : 15_2
*/
#define DMA_DEFOG_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER_OFS 0x02c8
REGDEF_BEGIN(DMA_DEFOG_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER)
REGDEF_BIT(,                               2)
REGDEF_BIT(reg_defog_subimg_lofsi,        14)
REGDEF_END(DMA_DEFOG_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER)


/*
    reg_defog_subimg_dramsao:    [0x0, 0x3fffffff],         bits : 31_2
*/
#define DMA_DEFOG_SUBIMG_OUTPUT_CHANNEL_REGISTER_OFS 0x02cc
REGDEF_BEGIN(DMA_DEFOG_SUBIMG_OUTPUT_CHANNEL_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(reg_defog_subimg_dramsao,        30)
REGDEF_END(DMA_DEFOG_SUBIMG_OUTPUT_CHANNEL_REGISTER)


/*
    reg_defog_subimg_lofso:    [0x0, 0x3fff],           bits : 15_2
*/
#define DMA_DEFOG_SUBIMG_OUPTUPT_CHANNEL_LINEOFFSET_REGISTER_OFS 0x02d0
REGDEF_BEGIN(DMA_DEFOG_SUBIMG_OUPTUPT_CHANNEL_LINEOFFSET_REGISTER)
REGDEF_BIT(,                               2)
REGDEF_BIT(reg_defog_subimg_lofso,        14)
REGDEF_END(DMA_DEFOG_SUBIMG_OUPTUPT_CHANNEL_LINEOFFSET_REGISTER)


/*
    reg_defog_subimg_hfactor:    [0x0, 0xffff],         bits : 15_0
    reg_defog_subimg_vfactor:    [0x0, 0xffff],         bits : 31_16
*/
#define DEFOG_SUBIMG_SCALING_REGISTER_0_OFS 0x02d4
REGDEF_BEGIN(DEFOG_SUBIMG_SCALING_REGISTER_0)
REGDEF_BIT(reg_defog_subimg_hfactor,        16)
REGDEF_BIT(reg_defog_subimg_vfactor,        16)
REGDEF_END(DEFOG_SUBIMG_SCALING_REGISTER_0)


/*
    reg_defog_sub_blk_sizeh   :    [0x0, 0x3ff],            bits : 9_0
    reg_defog_blk_cent_hfactor:    [0x0, 0x3fffff],         bits : 31_10
*/
#define DEFOG_SUBOUT_REGISTER_0_OFS 0x02d8
REGDEF_BEGIN(DEFOG_SUBOUT_REGISTER_0)
REGDEF_BIT(reg_defog_sub_blk_sizeh,           10)
REGDEF_BIT(reg_defog_blk_cent_hfactor,        22)
REGDEF_END(DEFOG_SUBOUT_REGISTER_0)


/*
    reg_defog_sub_blk_sizev   :    [0x0, 0x3ff],            bits : 9_0
    reg_defog_blk_cent_vfactor:    [0x0, 0x3fffff],         bits : 31_10
*/
#define DEFOG_SUBOUT_REGISTER_1_OFS 0x02dc
REGDEF_BEGIN(DEFOG_SUBOUT_REGISTER_1)
REGDEF_BIT(reg_defog_sub_blk_sizev,           10)
REGDEF_BIT(reg_defog_blk_cent_vfactor,        22)
REGDEF_END(DEFOG_SUBOUT_REGISTER_1)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED17_OFS 0x02e0
REGDEF_BEGIN(IPE_RESERVED17)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED17)


/*
    reg_defog_input_bldrto0:    [0x0, 0xff],            bits : 7_0
    reg_defog_input_bldrto1:    [0x0, 0xff],            bits : 15_8
    reg_defog_input_bldrto2:    [0x0, 0xff],            bits : 23_16
    reg_defog_input_bldrto3:    [0x0, 0xff],            bits : 31_24
*/
#define DEFOG_INPUT_BLENDING_REGISTER_0_OFS 0x02e4
REGDEF_BEGIN(DEFOG_INPUT_BLENDING_REGISTER_0)
REGDEF_BIT(reg_defog_input_bldrto0,        8)
REGDEF_BIT(reg_defog_input_bldrto1,        8)
REGDEF_BIT(reg_defog_input_bldrto2,        8)
REGDEF_BIT(reg_defog_input_bldrto3,        8)
REGDEF_END(DEFOG_INPUT_BLENDING_REGISTER_0)


/*
    reg_defog_input_bldrto4:    [0x0, 0xff],            bits : 7_0
    reg_defog_input_bldrto5:    [0x0, 0xff],            bits : 15_8
    reg_defog_input_bldrto6:    [0x0, 0xff],            bits : 23_16
    reg_defog_input_bldrto7:    [0x0, 0xff],            bits : 31_24
*/
#define DEFOG_INPUT_BLENDING_REGISTER_1_OFS 0x02e8
REGDEF_BEGIN(DEFOG_INPUT_BLENDING_REGISTER_1)
REGDEF_BIT(reg_defog_input_bldrto4,        8)
REGDEF_BIT(reg_defog_input_bldrto5,        8)
REGDEF_BIT(reg_defog_input_bldrto6,        8)
REGDEF_BIT(reg_defog_input_bldrto7,        8)
REGDEF_END(DEFOG_INPUT_BLENDING_REGISTER_1)


/*
    reg_defog_input_bldrto8:    [0x0, 0xff],            bits : 7_0
*/
#define DEFOG_INPUT_BLENDING_REGISTER_2_OFS 0x02ec
REGDEF_BEGIN(DEFOG_INPUT_BLENDING_REGISTER_2)
REGDEF_BIT(reg_defog_input_bldrto8,        8)
REGDEF_END(DEFOG_INPUT_BLENDING_REGISTER_2)


/*
    reg_defog_lpf_c0:    [0x0, 0x7],            bits : 2_0
    reg_defog_lpf_c1:    [0x0, 0x7],            bits : 5_3
    reg_defog_lpf_c2:    [0x0, 0x7],            bits : 8_6
*/
#define DEFOG_SUBIMG_LPF_REGISTER_OFS 0x02f0
REGDEF_BEGIN(DEFOG_SUBIMG_LPF_REGISTER)
REGDEF_BIT(reg_defog_lpf_c0,        3)
REGDEF_BIT(reg_defog_lpf_c1,        3)
REGDEF_BIT(reg_defog_lpf_c2,        3)
REGDEF_END(DEFOG_SUBIMG_LPF_REGISTER)


/*
    reg_defog_target_lut_0:    [0x0, 0x3ff],            bits : 9_0
    reg_defog_target_lut_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define DEFOG_STRENGTH_CONTROL_REGISTER0_OFS 0x02f4
REGDEF_BEGIN(DEFOG_STRENGTH_CONTROL_REGISTER0)
REGDEF_BIT(reg_defog_target_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(reg_defog_target_lut_1,        10)
REGDEF_END(DEFOG_STRENGTH_CONTROL_REGISTER0)


/*
    reg_defog_target_lut_2:    [0x0, 0x3ff],            bits : 9_0
    reg_defog_target_lut_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define DEFOG_STRENGTH_CONTROL_REGISTER1_OFS 0x02f8
REGDEF_BEGIN(DEFOG_STRENGTH_CONTROL_REGISTER1)
REGDEF_BIT(reg_defog_target_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(reg_defog_target_lut_3,        10)
REGDEF_END(DEFOG_STRENGTH_CONTROL_REGISTER1)


/*
    reg_defog_target_lut_4:    [0x0, 0x3ff],            bits : 9_0
    reg_defog_target_lut_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define DEFOG_STRENGTH_CONTROL_REGISTER2_OFS 0x02fc
REGDEF_BEGIN(DEFOG_STRENGTH_CONTROL_REGISTER2)
REGDEF_BIT(reg_defog_target_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(reg_defog_target_lut_5,        10)
REGDEF_END(DEFOG_STRENGTH_CONTROL_REGISTER2)


/*
    reg_defog_target_lut_6:    [0x0, 0x3ff],            bits : 9_0
    reg_defog_target_lut_7:    [0x0, 0x3ff],            bits : 25_16
*/
#define DEFOG_STRENGTH_CONTROL_REGISTER3_OFS 0x0300
REGDEF_BEGIN(DEFOG_STRENGTH_CONTROL_REGISTER3)
REGDEF_BIT(reg_defog_target_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(reg_defog_target_lut_7,        10)
REGDEF_END(DEFOG_STRENGTH_CONTROL_REGISTER3)


/*
    reg_defog_target_lut_8:    [0x0, 0x3ff],            bits : 9_0
*/
#define DEFOG_STRENGTH_CONTROL_REGISTER4_OFS 0x0304
REGDEF_BEGIN(DEFOG_STRENGTH_CONTROL_REGISTER4)
REGDEF_BIT(reg_defog_target_lut_8,        10)
REGDEF_END(DEFOG_STRENGTH_CONTROL_REGISTER4)


/*
    reg_defog_mod_lut_0:    [0x0, 0x3ff],           bits : 9_0
    reg_defog_mod_lut_1:    [0x0, 0x3ff],           bits : 25_16
*/
#define DEFOG_FOG_MODIFY_REGISTER0_OFS 0x0308
REGDEF_BEGIN(DEFOG_FOG_MODIFY_REGISTER0)
REGDEF_BIT(reg_defog_mod_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(reg_defog_mod_lut_1,        10)
REGDEF_END(DEFOG_FOG_MODIFY_REGISTER0)


/*
    reg_defog_mod_lut_2:    [0x0, 0x3ff],           bits : 9_0
    reg_defog_mod_lut_3:    [0x0, 0x3ff],           bits : 25_16
*/
#define DEFOG_FOG_MODIFY_REGISTER1_OFS 0x030c
REGDEF_BEGIN(DEFOG_FOG_MODIFY_REGISTER1)
REGDEF_BIT(reg_defog_mod_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(reg_defog_mod_lut_3,        10)
REGDEF_END(DEFOG_FOG_MODIFY_REGISTER1)


/*
    reg_defog_mod_lut_4:    [0x0, 0x3ff],           bits : 9_0
    reg_defog_mod_lut_5:    [0x0, 0x3ff],           bits : 25_16
*/
#define DEFOG_FOG_MODIFY_REGISTER2_OFS 0x0310
REGDEF_BEGIN(DEFOG_FOG_MODIFY_REGISTER2)
REGDEF_BIT(reg_defog_mod_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(reg_defog_mod_lut_5,        10)
REGDEF_END(DEFOG_FOG_MODIFY_REGISTER2)


/*
    reg_defog_mod_lut_6:    [0x0, 0x3ff],           bits : 9_0
    reg_defog_mod_lut_7:    [0x0, 0x3ff],           bits : 25_16
*/
#define DEFOG_FOG_MODIFY_REGISTER3_OFS 0x0314
REGDEF_BEGIN(DEFOG_FOG_MODIFY_REGISTER3)
REGDEF_BIT(reg_defog_mod_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(reg_defog_mod_lut_7,        10)
REGDEF_END(DEFOG_FOG_MODIFY_REGISTER3)


/*
    reg_defog_mod_lut_8:    [0x0, 0x3ff],           bits : 9_0
    reg_defog_mod_lut_9:    [0x0, 0x3ff],           bits : 25_16
*/
#define DEFOG_FOG_MODIFY_REGISTER4_OFS 0x0318
REGDEF_BEGIN(DEFOG_FOG_MODIFY_REGISTER4)
REGDEF_BIT(reg_defog_mod_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(reg_defog_mod_lut_9,        10)
REGDEF_END(DEFOG_FOG_MODIFY_REGISTER4)


/*
    reg_defog_mod_lut_10:    [0x0, 0x3ff],          bits : 9_0
    reg_defog_mod_lut_11:    [0x0, 0x3ff],          bits : 25_16
*/
#define DEFOG_FOG_MODIFY_REGISTER5_OFS 0x031c
REGDEF_BEGIN(DEFOG_FOG_MODIFY_REGISTER5)
REGDEF_BIT(reg_defog_mod_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(reg_defog_mod_lut_11,        10)
REGDEF_END(DEFOG_FOG_MODIFY_REGISTER5)


/*
    reg_defog_mod_lut_12:    [0x0, 0x3ff],          bits : 9_0
    reg_defog_mod_lut_13:    [0x0, 0x3ff],          bits : 25_16
*/
#define DEFOG_FOG_MODIFY_REGISTER6_OFS 0x0320
REGDEF_BEGIN(DEFOG_FOG_MODIFY_REGISTER6)
REGDEF_BIT(reg_defog_mod_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(reg_defog_mod_lut_13,        10)
REGDEF_END(DEFOG_FOG_MODIFY_REGISTER6)


/*
    reg_defog_mod_lut_14:    [0x0, 0x3ff],          bits : 9_0
    reg_defog_mod_lut_15:    [0x0, 0x3ff],          bits : 25_16
*/
#define DEFOG_FOG_MODIFY_REGISTER7_OFS 0x0324
REGDEF_BEGIN(DEFOG_FOG_MODIFY_REGISTER7)
REGDEF_BIT(reg_defog_mod_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(reg_defog_mod_lut_15,        10)
REGDEF_END(DEFOG_FOG_MODIFY_REGISTER7)


/*
    reg_defog_mod_lut_16:    [0x0, 0x3ff],          bits : 9_0
*/
#define DEFOG_FOG_MODIFY_REGISTER8_OFS 0x0328
REGDEF_BEGIN(DEFOG_FOG_MODIFY_REGISTER8)
REGDEF_BIT(reg_defog_mod_lut_16,        10)
REGDEF_END(DEFOG_FOG_MODIFY_REGISTER8)


/*
    reg_defog_interp_diff_lut0:    [0x0, 0x3f],         bits : 5_0
    reg_defog_interp_diff_lut1:    [0x0, 0x3f],         bits : 11_6
    reg_defog_interp_diff_lut2:    [0x0, 0x3f],         bits : 17_12
    reg_defog_interp_diff_lut3:    [0x0, 0x3f],         bits : 23_18
    reg_defog_interp_diff_lut4:    [0x0, 0x3f],         bits : 29_24
*/
#define DEFOG_SUBIMG_SCALING_REGISTER1_OFS 0x032c
REGDEF_BEGIN(DEFOG_SUBIMG_SCALING_REGISTER1)
REGDEF_BIT(reg_defog_interp_diff_lut0,        6)
REGDEF_BIT(reg_defog_interp_diff_lut1,        6)
REGDEF_BIT(reg_defog_interp_diff_lut2,        6)
REGDEF_BIT(reg_defog_interp_diff_lut3,        6)
REGDEF_BIT(reg_defog_interp_diff_lut4,        6)
REGDEF_END(DEFOG_SUBIMG_SCALING_REGISTER1)


/*
    reg_defog_interp_diff_lut5:    [0x0, 0x3f],         bits : 5_0
    reg_defog_interp_diff_lut6:    [0x0, 0x3f],         bits : 11_6
    reg_defog_interp_diff_lut7:    [0x0, 0x3f],         bits : 17_12
    reg_defog_interp_diff_lut8:    [0x0, 0x3f],         bits : 23_18
    reg_defog_interp_diff_lut9:    [0x0, 0x3f],         bits : 29_24
*/
#define DEFOG_SUBIMG_SCALING_REGISTER2_OFS 0x0330
REGDEF_BEGIN(DEFOG_SUBIMG_SCALING_REGISTER2)
REGDEF_BIT(reg_defog_interp_diff_lut5,        6)
REGDEF_BIT(reg_defog_interp_diff_lut6,        6)
REGDEF_BIT(reg_defog_interp_diff_lut7,        6)
REGDEF_BIT(reg_defog_interp_diff_lut8,        6)
REGDEF_BIT(reg_defog_interp_diff_lut9,        6)
REGDEF_END(DEFOG_SUBIMG_SCALING_REGISTER2)


/*
    reg_defog_interp_diff_lut10:    [0x0, 0x3f],            bits : 5_0
    reg_defog_interp_diff_lut11:    [0x0, 0x3f],            bits : 11_6
    reg_defog_interp_diff_lut12:    [0x0, 0x3f],            bits : 17_12
    reg_defog_interp_diff_lut13:    [0x0, 0x3f],            bits : 23_18
    reg_defog_interp_diff_lut14:    [0x0, 0x3f],            bits : 29_24
*/
#define DEFOG_SUBIMG_SCALING_REGISTER3_OFS 0x0334
REGDEF_BEGIN(DEFOG_SUBIMG_SCALING_REGISTER3)
REGDEF_BIT(reg_defog_interp_diff_lut10,        6)
REGDEF_BIT(reg_defog_interp_diff_lut11,        6)
REGDEF_BIT(reg_defog_interp_diff_lut12,        6)
REGDEF_BIT(reg_defog_interp_diff_lut13,        6)
REGDEF_BIT(reg_defog_interp_diff_lut14,        6)
REGDEF_END(DEFOG_SUBIMG_SCALING_REGISTER3)


/*
    reg_defog_interp_diff_lut15:    [0x0, 0x3f],            bits : 5_0
    reg_defog_interp_diff_lut16:    [0x0, 0x3f],            bits : 11_6
*/
#define DEFOG_SUBIMG_SCALING_REGISTER4_OFS 0x0338
REGDEF_BEGIN(DEFOG_SUBIMG_SCALING_REGISTER4)
REGDEF_BIT(reg_defog_interp_diff_lut15,        6)
REGDEF_BIT(reg_defog_interp_diff_lut16,        6)
REGDEF_END(DEFOG_SUBIMG_SCALING_REGISTER4)


/*
    reg_defog_interp_wdist  :    [0x0, 0xf],            bits : 3_0
    reg_defog_interp_wout   :    [0x0, 0xf],            bits : 7_4
    reg_defog_interp_wcenter:    [0x0, 0x3f],           bits : 13_8
    reg_defog_interp_wsrc   :    [0x0, 0xff],           bits : 23_16
*/
#define DEFOG_SUBIMG_SCALING_REGISTER5_OFS 0x033c
REGDEF_BEGIN(DEFOG_SUBIMG_SCALING_REGISTER5)
REGDEF_BIT(reg_defog_interp_wdist,        4)
REGDEF_BIT(reg_defog_interp_wout,        4)
REGDEF_BIT(reg_defog_interp_wcenter,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(reg_defog_interp_wsrc,        8)
REGDEF_END(DEFOG_SUBIMG_SCALING_REGISTER5)


/*
    reg_defog_air0:    [0x0, 0x3ff],            bits : 9_0
    reg_defog_air1:    [0x0, 0x3ff],            bits : 25_16
*/
#define DEFOG_AIRLIGHT_REGISTER0_OFS 0x0340
REGDEF_BEGIN(DEFOG_AIRLIGHT_REGISTER0)
REGDEF_BIT(reg_defog_air0,        10)
REGDEF_BIT(,                       6)
REGDEF_BIT(reg_defog_air1,        10)
REGDEF_END(DEFOG_AIRLIGHT_REGISTER0)


/*
    reg_defog_air2:    [0x0, 0x3ff],            bits : 9_0
*/
#define DEFOG_AIRLIGHT_REGISTER1_OFS 0x0344
REGDEF_BEGIN(DEFOG_AIRLIGHT_REGISTER1)
REGDEF_BIT(reg_defog_air2,        10)
REGDEF_END(DEFOG_AIRLIGHT_REGISTER1)


/*
    reg_defog_statistics_pixcnt:    [0x0, 0xffffff],            bits : 23_0
*/
#define DEFOG_STATISTICS_REGISTER0_OFS 0x0348
REGDEF_BEGIN(DEFOG_STATISTICS_REGISTER0)
REGDEF_BIT(reg_defog_statistics_pixcnt,        24)
REGDEF_END(DEFOG_STATISTICS_REGISTER0)


/*
    reg_defog_statistics_air0:    [0x0, 0x3ff],         bits : 9_0
    reg_defog_statistics_air1:    [0x0, 0x3ff],         bits : 25_16
*/
#define DEFOG_STATISTICS_REGISTER1_OFS 0x034c
REGDEF_BEGIN(DEFOG_STATISTICS_REGISTER1)
REGDEF_BIT(reg_defog_statistics_air0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(reg_defog_statistics_air1,        10)
REGDEF_END(DEFOG_STATISTICS_REGISTER1)


/*
    reg_defog_statistics_air2:    [0x0, 0x3ff],         bits : 9_0
*/
#define DEFOG_STATISTICS_REGISTER2_OFS 0x0350
REGDEF_BEGIN(DEFOG_STATISTICS_REGISTER2)
REGDEF_BIT(reg_defog_statistics_air2,        10)
REGDEF_END(DEFOG_STATISTICS_REGISTER2)


/*
    reg_defog_air_normfactor:    [0x0, 0x1fffff],           bits : 20_0
*/
#define DEFOG_AIRLIGHT_REGISTER2_OFS 0x0354
REGDEF_BEGIN(DEFOG_AIRLIGHT_REGISTER2)
REGDEF_BIT(reg_defog_air_normfactor,        21)
REGDEF_END(DEFOG_AIRLIGHT_REGISTER2)


/*
    reg_defog_fog_rto    :    [0x0, 0xff],          bits : 7_0
    reg_defog_min_diff   :    [0x0, 0x3ff],         bits : 17_8
    reg_defog_dgain_ratio:    [0x0, 0xff],          bits : 27_20
    reg_defog_selfcmp_en :    [0x0, 0x1],           bits : 30
    reg_defog_mode_sel   :    [0x0, 0x1],           bits : 31
*/
#define DEFOG_STRENGTH_CONTROL_REGISTER5_OFS 0x0358
REGDEF_BEGIN(DEFOG_STRENGTH_CONTROL_REGISTER5)
REGDEF_BIT(reg_defog_fog_rto,           8)
REGDEF_BIT(reg_defog_min_diff,         10)
REGDEF_BIT(,                            2)
REGDEF_BIT(reg_defog_dgain_ratio,       8)
REGDEF_BIT(,                            2)
REGDEF_BIT(reg_defog_selfcmp_en,        1)
REGDEF_BIT(reg_defog_mode_sel,          1)
REGDEF_END(DEFOG_STRENGTH_CONTROL_REGISTER5)


/*
    reg_defog_gain_th:    [0x0, 0xff],          bits : 7_0
*/
#define DEFOG_STRENGTH_CONTROL_REGISTER6_OFS 0x035c
REGDEF_BEGIN(DEFOG_STRENGTH_CONTROL_REGISTER6)
REGDEF_BIT(reg_defog_gain_th,        8)
REGDEF_END(DEFOG_STRENGTH_CONTROL_REGISTER6)


/*
    reg_defog_outbld_lumwt0:    [0x0, 0xff],            bits : 7_0
    reg_defog_outbld_lumwt1:    [0x0, 0xff],            bits : 15_8
    reg_defog_outbld_lumwt2:    [0x0, 0xff],            bits : 23_16
    reg_defog_outbld_lumwt3:    [0x0, 0xff],            bits : 31_24
*/
#define DEFOG_OUTPUT_BLENDING_REGISTER0_OFS 0x0360
REGDEF_BEGIN(DEFOG_OUTPUT_BLENDING_REGISTER0)
REGDEF_BIT(reg_defog_outbld_lumwt0,        8)
REGDEF_BIT(reg_defog_outbld_lumwt1,        8)
REGDEF_BIT(reg_defog_outbld_lumwt2,        8)
REGDEF_BIT(reg_defog_outbld_lumwt3,        8)
REGDEF_END(DEFOG_OUTPUT_BLENDING_REGISTER0)


/*
    reg_defog_outbld_lumwt4:    [0x0, 0xff],            bits : 7_0
    reg_defog_outbld_lumwt5:    [0x0, 0xff],            bits : 15_8
    reg_defog_outbld_lumwt6:    [0x0, 0xff],            bits : 23_16
    reg_defog_outbld_lumwt7:    [0x0, 0xff],            bits : 31_24
*/
#define DEFOG_OUTPUT_BLENDING_REGISTER1_OFS 0x0364
REGDEF_BEGIN(DEFOG_OUTPUT_BLENDING_REGISTER1)
REGDEF_BIT(reg_defog_outbld_lumwt4,        8)
REGDEF_BIT(reg_defog_outbld_lumwt5,        8)
REGDEF_BIT(reg_defog_outbld_lumwt6,        8)
REGDEF_BIT(reg_defog_outbld_lumwt7,        8)
REGDEF_END(DEFOG_OUTPUT_BLENDING_REGISTER1)


/*
    reg_defog_outbld_lumwt8 :    [0x0, 0xff],           bits : 7_0
    reg_defog_outbld_lumwt9 :    [0x0, 0xff],           bits : 15_8
    reg_defog_outbld_lumwt10:    [0x0, 0xff],           bits : 23_16
    reg_defog_outbld_lumwt11:    [0x0, 0xff],           bits : 31_24
*/
#define DEFOG_OUTPUT_BLENDING_REGISTER2_OFS 0x0368
REGDEF_BEGIN(DEFOG_OUTPUT_BLENDING_REGISTER2)
REGDEF_BIT(reg_defog_outbld_lumwt8,         8)
REGDEF_BIT(reg_defog_outbld_lumwt9,         8)
REGDEF_BIT(reg_defog_outbld_lumwt10,        8)
REGDEF_BIT(reg_defog_outbld_lumwt11,        8)
REGDEF_END(DEFOG_OUTPUT_BLENDING_REGISTER2)


/*
    reg_defog_outbld_lumwt12:    [0x0, 0xff],           bits : 7_0
    reg_defog_outbld_lumwt13:    [0x0, 0xff],           bits : 15_8
    reg_defog_outbld_lumwt14:    [0x0, 0xff],           bits : 23_16
    reg_defog_outbld_lumwt15:    [0x0, 0xff],           bits : 31_24
*/
#define DEFOG_OUTPUT_BLENDING_REGISTER3_OFS 0x036c
REGDEF_BEGIN(DEFOG_OUTPUT_BLENDING_REGISTER3)
REGDEF_BIT(reg_defog_outbld_lumwt12,        8)
REGDEF_BIT(reg_defog_outbld_lumwt13,        8)
REGDEF_BIT(reg_defog_outbld_lumwt14,        8)
REGDEF_BIT(reg_defog_outbld_lumwt15,        8)
REGDEF_END(DEFOG_OUTPUT_BLENDING_REGISTER3)


/*
    reg_defog_outbld_lumwt16 :    [0x0, 0xff],          bits : 7_0
    reg_defog_wet_ref        :    [0x0, 0x1],           bits : 16
    reg_defog_local_outbld_en:    [0x0, 0x1],           bits : 17
*/
#define DEFOG_OUTPUT_BLENDING_REGISTER4_OFS 0x0370
REGDEF_BEGIN(DEFOG_OUTPUT_BLENDING_REGISTER4)
REGDEF_BIT(reg_defog_outbld_lumwt16,        8)
REGDEF_BIT(,        8)
REGDEF_BIT(reg_defog_wet_ref,        1)
REGDEF_BIT(reg_defog_local_outbld_en,        1)
REGDEF_END(DEFOG_OUTPUT_BLENDING_REGISTER4)


/*
    reg_defog_outbld_diffwt0:    [0x0, 0x3f],           bits : 5_0
    reg_defog_outbld_diffwt1:    [0x0, 0x3f],           bits : 11_6
    reg_defog_outbld_diffwt2:    [0x0, 0x3f],           bits : 17_12
    reg_defog_outbld_diffwt3:    [0x0, 0x3f],           bits : 23_18
    reg_defog_outbld_diffwt4:    [0x0, 0x3f],           bits : 29_24
*/
#define DEFOG_OUTPUT_BLENDING_REGISTER5_OFS 0x0374
REGDEF_BEGIN(DEFOG_OUTPUT_BLENDING_REGISTER5)
REGDEF_BIT(reg_defog_outbld_diffwt0,        6)
REGDEF_BIT(reg_defog_outbld_diffwt1,        6)
REGDEF_BIT(reg_defog_outbld_diffwt2,        6)
REGDEF_BIT(reg_defog_outbld_diffwt3,        6)
REGDEF_BIT(reg_defog_outbld_diffwt4,        6)
REGDEF_END(DEFOG_OUTPUT_BLENDING_REGISTER5)


/*
    reg_defog_outbld_diffwt5:    [0x0, 0x3f],           bits : 5_0
    reg_defog_outbld_diffwt6:    [0x0, 0x3f],           bits : 11_6
    reg_defog_outbld_diffwt7:    [0x0, 0x3f],           bits : 17_12
    reg_defog_outbld_diffwt8:    [0x0, 0x3f],           bits : 23_18
    reg_defog_outbld_diffwt9:    [0x0, 0x3f],           bits : 29_24
*/
#define DEFOG_OUTPUT_BLENDING_REGISTER6_OFS 0x0378
REGDEF_BEGIN(DEFOG_OUTPUT_BLENDING_REGISTER6)
REGDEF_BIT(reg_defog_outbld_diffwt5,        6)
REGDEF_BIT(reg_defog_outbld_diffwt6,        6)
REGDEF_BIT(reg_defog_outbld_diffwt7,        6)
REGDEF_BIT(reg_defog_outbld_diffwt8,        6)
REGDEF_BIT(reg_defog_outbld_diffwt9,        6)
REGDEF_END(DEFOG_OUTPUT_BLENDING_REGISTER6)


/*
    reg_defog_outbld_diffwt10:    [0x0, 0x3f],          bits : 5_0
    reg_defog_outbld_diffwt11:    [0x0, 0x3f],          bits : 11_6
    reg_defog_outbld_diffwt12:    [0x0, 0x3f],          bits : 17_12
    reg_defog_outbld_diffwt13:    [0x0, 0x3f],          bits : 23_18
    reg_defog_outbld_diffwt14:    [0x0, 0x3f],          bits : 29_24
*/
#define DEFOG_OUTPUT_BLENDING_REGISTER7_OFS 0x037c
REGDEF_BEGIN(DEFOG_OUTPUT_BLENDING_REGISTER7)
REGDEF_BIT(reg_defog_outbld_diffwt10,        6)
REGDEF_BIT(reg_defog_outbld_diffwt11,        6)
REGDEF_BIT(reg_defog_outbld_diffwt12,        6)
REGDEF_BIT(reg_defog_outbld_diffwt13,        6)
REGDEF_BIT(reg_defog_outbld_diffwt14,        6)
REGDEF_END(DEFOG_OUTPUT_BLENDING_REGISTER7)


/*
    reg_defog_outbld_diffwt15:    [0x0, 0x3f],          bits : 5_0
    reg_defog_outbld_diffwt16:    [0x0, 0x3f],          bits : 11_6
*/
#define DEFOG_OUTPUT_BLENDING_REGISTER8_OFS 0x0380
REGDEF_BEGIN(DEFOG_OUTPUT_BLENDING_REGISTER8)
REGDEF_BIT(reg_defog_outbld_diffwt15,        6)
REGDEF_BIT(reg_defog_outbld_diffwt16,        6)
REGDEF_END(DEFOG_OUTPUT_BLENDING_REGISTER8)


/*
    reg_lce_wt_diff_pos:    [0x0, 0xff],            bits : 7_0
    reg_lce_wt_diff_neg:    [0x0, 0xff],            bits : 15_8
    reg_lce_wt_diff_avg:    [0x0, 0xff],            bits : 23_16
*/
#define LCE_REGISTER_0_OFS 0x0384
REGDEF_BEGIN(LCE_REGISTER_0)
REGDEF_BIT(reg_lce_wt_diff_pos,        8)
REGDEF_BIT(reg_lce_wt_diff_neg,        8)
REGDEF_BIT(reg_lce_wt_diff_avg,        8)
REGDEF_END(LCE_REGISTER_0)


/*
    reg_lce_lum_adj_lut0:    [0x0, 0xff],           bits : 7_0
    reg_lce_lum_adj_lut1:    [0x0, 0xff],           bits : 15_8
    reg_lce_lum_adj_lut2:    [0x0, 0xff],           bits : 23_16
    reg_lce_lum_adj_lut3:    [0x0, 0xff],           bits : 31_24
*/
#define LCE_REGISTER_1_OFS 0x0388
REGDEF_BEGIN(LCE_REGISTER_1)
REGDEF_BIT(reg_lce_lum_adj_lut0,        8)
REGDEF_BIT(reg_lce_lum_adj_lut1,        8)
REGDEF_BIT(reg_lce_lum_adj_lut2,        8)
REGDEF_BIT(reg_lce_lum_adj_lut3,        8)
REGDEF_END(LCE_REGISTER_1)


/*
    reg_lce_lum_adj_lut4:    [0x0, 0xff],           bits : 7_0
    reg_lce_lum_adj_lut5:    [0x0, 0xff],           bits : 15_8
    reg_lce_lum_adj_lut6:    [0x0, 0xff],           bits : 23_16
    reg_lce_lum_adj_lut7:    [0x0, 0xff],           bits : 31_24
*/
#define LCE_REGISTER_2_OFS 0x038c
REGDEF_BEGIN(LCE_REGISTER_2)
REGDEF_BIT(reg_lce_lum_adj_lut4,        8)
REGDEF_BIT(reg_lce_lum_adj_lut5,        8)
REGDEF_BIT(reg_lce_lum_adj_lut6,        8)
REGDEF_BIT(reg_lce_lum_adj_lut7,        8)
REGDEF_END(LCE_REGISTER_2)


/*
    reg_lce_lum_adj_lut8:    [0x0, 0xff],           bits : 7_0
*/
#define LCE_REGISTER_3_OFS 0x0390
REGDEF_BEGIN(LCE_REGISTER_3)
REGDEF_BIT(reg_lce_lum_adj_lut8,        8)
REGDEF_END(LCE_REGISTER_3)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED32_OFS 0x0394
REGDEF_BEGIN(IPE_RESERVED32)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED32)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED33_OFS 0x0398
REGDEF_BEGIN(IPE_RESERVED33)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED33)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED34_OFS 0x039c
REGDEF_BEGIN(IPE_RESERVED34)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED34)


/*
    reg_pfr_uv_filt_en   :    [0x0, 0x1],           bits : 0
    reg_pfr_luma_level_en:    [0x0, 0x1],           bits : 1
    reg_pfr_set0_en      :    [0x0, 0x1],           bits : 2
    reg_pfr_set1_en      :    [0x0, 0x1],           bits : 3
    reg_pfr_set2_en      :    [0x0, 0x1],           bits : 4
    reg_pfr_set3_en      :    [0x0, 0x1],           bits : 5
    reg_pfr_out_wet      :    [0x0, 0xff],          bits : 23_16
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER0_OFS 0x03a0
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER0)
REGDEF_BIT(reg_pfr_uv_filt_en,     1)
REGDEF_BIT(reg_pfr_luma_level_en,  1)
REGDEF_BIT(reg_pfr_set0_en,        1)
REGDEF_BIT(reg_pfr_set1_en,        1)
REGDEF_BIT(reg_pfr_set2_en,        1)
REGDEF_BIT(reg_pfr_set3_en,        1)
REGDEF_BIT(,                      10)
REGDEF_BIT(reg_pfr_out_wet,        8)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER0)


/*
    reg_pfr_edge_th :    [0x0, 0xff],           bits : 7_0
    reg_pfr_edge_str:    [0x0, 0xff],           bits : 15_8
    reg_pfr_g_wet   :    [0x0, 0xff],           bits : 31_24
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER1_OFS 0x03a4
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER1)
REGDEF_BIT(reg_pfr_edge_th,        8)
REGDEF_BIT(reg_pfr_edge_str,       8)
REGDEF_BIT(,                       8)
REGDEF_BIT(reg_pfr_g_wet,          8)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER1)


/*
    reg_pfr_color_u0:    [0x0, 0xff],           bits : 7_0
    reg_pfr_color_v0:    [0x0, 0xff],           bits : 15_8
    reg_pfr_r_wet0  :    [0x0, 0xff],           bits : 23_16
    reg_pfr_b_wet0  :    [0x0, 0xff],           bits : 31_24
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER2_OFS 0x03a8
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER2)
REGDEF_BIT(reg_pfr_color_u0,        8)
REGDEF_BIT(reg_pfr_color_v0,        8)
REGDEF_BIT(reg_pfr_r_wet0,        8)
REGDEF_BIT(reg_pfr_b_wet0,        8)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER2)


/*
    reg_pfr_color_u1:    [0x0, 0xff],           bits : 7_0
    reg_pfr_color_v1:    [0x0, 0xff],           bits : 15_8
    reg_pfr_r_wet1  :    [0x0, 0xff],           bits : 23_16
    reg_pfr_b_wet1  :    [0x0, 0xff],           bits : 31_24
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER3_OFS 0x03ac
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER3)
REGDEF_BIT(reg_pfr_color_u1,        8)
REGDEF_BIT(reg_pfr_color_v1,        8)
REGDEF_BIT(reg_pfr_r_wet1,          8)
REGDEF_BIT(reg_pfr_b_wet1,          8)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER3)


/*
    reg_pfr_color_u2:    [0x0, 0xff],           bits : 7_0
    reg_pfr_color_v2:    [0x0, 0xff],           bits : 15_8
    reg_pfr_r_wet2  :    [0x0, 0xff],           bits : 23_16
    reg_pfr_b_wet2  :    [0x0, 0xff],           bits : 31_24
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER4_OFS 0x03b0
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER4)
REGDEF_BIT(reg_pfr_color_u2,        8)
REGDEF_BIT(reg_pfr_color_v2,        8)
REGDEF_BIT(reg_pfr_r_wet2,          8)
REGDEF_BIT(reg_pfr_b_wet2,          8)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER4)


/*
    reg_pfr_color_u3:    [0x0, 0xff],           bits : 7_0
    reg_pfr_color_v3:    [0x0, 0xff],           bits : 15_8
    reg_pfr_r_wet3  :    [0x0, 0xff],           bits : 23_16
    reg_pfr_b_wet3  :    [0x0, 0xff],           bits : 31_24
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER5_OFS 0x03b4
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER5)
REGDEF_BIT(reg_pfr_color_u3,        8)
REGDEF_BIT(reg_pfr_color_v3,        8)
REGDEF_BIT(reg_pfr_r_wet3,          8)
REGDEF_BIT(reg_pfr_b_wet3,          8)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER5)


/*
    reg_pfr_cdif_set0_lut0:    [0x0, 0xff],         bits : 7_0
    reg_pfr_cdif_set0_lut1:    [0x0, 0xff],         bits : 15_8
    reg_pfr_cdif_set0_lut2:    [0x0, 0xff],         bits : 23_16
    reg_pfr_cdif_set0_lut3:    [0x0, 0xff],         bits : 31_24
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER6_OFS 0x03b8
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER6)
REGDEF_BIT(reg_pfr_cdif_set0_lut0,        8)
REGDEF_BIT(reg_pfr_cdif_set0_lut1,        8)
REGDEF_BIT(reg_pfr_cdif_set0_lut2,        8)
REGDEF_BIT(reg_pfr_cdif_set0_lut3,        8)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER6)


/*
    reg_pfr_cdif_set0_lut4:    [0x0, 0xff],         bits : 7_0
    reg_pfr_cdif_set0_th  :    [0x0, 0xff],         bits : 23_16
    reg_pfr_cdif_set0_step:    [0x0, 0x7],          bits : 26_24
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER7_OFS 0x03bc
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER7)
REGDEF_BIT(reg_pfr_cdif_set0_lut4,        8)
REGDEF_BIT(,                              8)
REGDEF_BIT(reg_pfr_cdif_set0_th,          8)
REGDEF_BIT(reg_pfr_cdif_set0_step,        3)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER7)


/*
    reg_pfr_cdif_set1_lut0:    [0x0, 0xff],         bits : 7_0
    reg_pfr_cdif_set1_lut1:    [0x0, 0xff],         bits : 15_8
    reg_pfr_cdif_set1_lut2:    [0x0, 0xff],         bits : 23_16
    reg_pfr_cdif_set1_lut3:    [0x0, 0xff],         bits : 31_24
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER8_OFS 0x03c0
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER8)
REGDEF_BIT(reg_pfr_cdif_set1_lut0,        8)
REGDEF_BIT(reg_pfr_cdif_set1_lut1,        8)
REGDEF_BIT(reg_pfr_cdif_set1_lut2,        8)
REGDEF_BIT(reg_pfr_cdif_set1_lut3,        8)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER8)


/*
    reg_pfr_cdif_set1_lut4:    [0x0, 0xff],         bits : 7_0
    reg_pfr_cdif_set1_th  :    [0x0, 0xff],         bits : 23_16
    reg_pfr_cdif_set1_step:    [0x0, 0x7],          bits : 26_24
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER9_OFS 0x03c4
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER9)
REGDEF_BIT(reg_pfr_cdif_set1_lut4,        8)
REGDEF_BIT(,                              8)
REGDEF_BIT(reg_pfr_cdif_set1_th,          8)
REGDEF_BIT(reg_pfr_cdif_set1_step,        3)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER9)


/*
    reg_pfr_cdif_set2_lut0:    [0x0, 0xff],         bits : 7_0
    reg_pfr_cdif_set2_lut1:    [0x0, 0xff],         bits : 15_8
    reg_pfr_cdif_set2_lut2:    [0x0, 0xff],         bits : 23_16
    reg_pfr_cdif_set2_lut3:    [0x0, 0xff],         bits : 31_24
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER10_OFS 0x03c8
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER10)
REGDEF_BIT(reg_pfr_cdif_set2_lut0,        8)
REGDEF_BIT(reg_pfr_cdif_set2_lut1,        8)
REGDEF_BIT(reg_pfr_cdif_set2_lut2,        8)
REGDEF_BIT(reg_pfr_cdif_set2_lut3,        8)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER10)


/*
    reg_pfr_cdif_set2_lut4:    [0x0, 0xff],         bits : 7_0
    reg_pfr_cdif_set2_th  :    [0x0, 0xff],         bits : 23_16
    reg_pfr_cdif_set2_step:    [0x0, 0x7],          bits : 26_24
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER11_OFS 0x03cc
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER11)
REGDEF_BIT(reg_pfr_cdif_set2_lut4,        8)
REGDEF_BIT(,                              8)
REGDEF_BIT(reg_pfr_cdif_set2_th,          8)
REGDEF_BIT(reg_pfr_cdif_set2_step,        3)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER11)


/*
    reg_pfr_cdif_set3_lut0:    [0x0, 0xff],         bits : 7_0
    reg_pfr_cdif_set3_lut1:    [0x0, 0xff],         bits : 15_8
    reg_pfr_cdif_set3_lut2:    [0x0, 0xff],         bits : 23_16
    reg_pfr_cdif_set3_lut3:    [0x0, 0xff],         bits : 31_24
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER12_OFS 0x03d0
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER12)
REGDEF_BIT(reg_pfr_cdif_set3_lut0,        8)
REGDEF_BIT(reg_pfr_cdif_set3_lut1,        8)
REGDEF_BIT(reg_pfr_cdif_set3_lut2,        8)
REGDEF_BIT(reg_pfr_cdif_set3_lut3,        8)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER12)


/*
    reg_pfr_cdif_set3_lut4:    [0x0, 0xff],         bits : 7_0
    reg_pfr_cdif_set3_th  :    [0x0, 0xff],         bits : 23_16
    reg_pfr_cdif_set3_step:    [0x0, 0x7],          bits : 26_24
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER13_OFS 0x03d4
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER13)
REGDEF_BIT(reg_pfr_cdif_set3_lut4,        8)
REGDEF_BIT(,                              8)
REGDEF_BIT(reg_pfr_cdif_set3_th,          8)
REGDEF_BIT(reg_pfr_cdif_set3_step,        3)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER13)


/*
    reg_pfr_luma_level_0:    [0x0, 0xff],            bits : 7_0
    reg_pfr_luma_level_1:    [0x0, 0xff],            bits : 15_8
    reg_pfr_luma_level_2:    [0x0, 0xff],            bits : 23_16
    reg_pfr_luma_level_3:    [0x0, 0xff],            bits : 31_24
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER14_OFS 0x03d8
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER14)
REGDEF_BIT(reg_pfr_luma_level_0,        8)
REGDEF_BIT(reg_pfr_luma_level_1,        8)
REGDEF_BIT(reg_pfr_luma_level_2,        8)
REGDEF_BIT(reg_pfr_luma_level_3,        8)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER14)


/*
    reg_pfr_luma_level_4:    [0x0, 0xff],            bits : 7_0
    reg_pfr_luma_level_5:    [0x0, 0xff],            bits : 15_8
    reg_pfr_luma_level_6:    [0x0, 0xff],            bits : 23_16
    reg_pfr_luma_level_7:    [0x0, 0xff],            bits : 31_24
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER15_OFS 0x03dc
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER15)
REGDEF_BIT(reg_pfr_luma_level_4,        8)
REGDEF_BIT(reg_pfr_luma_level_5,        8)
REGDEF_BIT(reg_pfr_luma_level_6,        8)
REGDEF_BIT(reg_pfr_luma_level_7,        8)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER15)


/*
    reg_pfr_luma_level_8 :    [0x0, 0xff],           bits : 7_0
    reg_pfr_luma_level_9 :    [0x0, 0xff],           bits : 15_8
    reg_pfr_luma_level_10:    [0x0, 0xff],           bits : 23_16
    reg_pfr_luma_level_11:    [0x0, 0xff],           bits : 31_24
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER16_OFS 0x03e0
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER16)
REGDEF_BIT(reg_pfr_luma_level_8,        8)
REGDEF_BIT(reg_pfr_luma_level_9,        8)
REGDEF_BIT(reg_pfr_luma_level_10,        8)
REGDEF_BIT(reg_pfr_luma_level_11,        8)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER16)


/*
    reg_pfr_luma_level_12:    [0x0, 0xff],           bits : 7_0
    reg_pfr_luma_th     :    [0x0, 0x3ff],          bits : 25_16
*/
#define PURPLE_FRINGE_REDUCTION_REGISTER17_OFS 0x03e4
REGDEF_BEGIN(PURPLE_FRINGE_REDUCTION_REGISTER17)
REGDEF_BIT(reg_pfr_luma_level_12,        8)
REGDEF_BIT(,                             8)
REGDEF_BIT(reg_pfr_luma_th,             10)
REGDEF_END(PURPLE_FRINGE_REDUCTION_REGISTER17)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED53_OFS 0x03e8
REGDEF_BEGIN(IPE_RESERVED53)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED53)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED54_OFS 0x03ec
REGDEF_BEGIN(IPE_RESERVED54)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED54)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED55_OFS 0x03f0
REGDEF_BEGIN(IPE_RESERVED55)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED55)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED56_OFS 0x03f4
REGDEF_BEGIN(IPE_RESERVED56)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED56)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED57_OFS 0x03f8
REGDEF_BEGIN(IPE_RESERVED57)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED57)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED58_OFS 0x03fc
REGDEF_BEGIN(IPE_RESERVED58)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED58)


/*
    reg_r_lut_0:    [0x0, 0x3ff],            bits : 9_0
    reg_r_lut_1:    [0x0, 0x3ff],            bits : 19_10
    reg_r_lut_2:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_R_GAMMA0_OFS 0x0400
REGDEF_BEGIN(SRAM_R_GAMMA0)
REGDEF_BIT(reg_r_lut_0,        10)
REGDEF_BIT(reg_r_lut_1,        10)
REGDEF_BIT(reg_r_lut_2,        10)
REGDEF_END(SRAM_R_GAMMA0)


/*
    reg_r_lut_2:    [0x0, 0x3ff],            bits : 9_0
    reg_r_lut_3:    [0x0, 0x3ff],            bits : 19_10
    reg_r_lut_4:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_R_GAMMA1_OFS 0x0404
REGDEF_BEGIN(SRAM_R_GAMMA1)
REGDEF_BIT(reg_r_lut_2,        10)
REGDEF_BIT(reg_r_lut_3,        10)
REGDEF_BIT(reg_r_lut_4,        10)
REGDEF_END(SRAM_R_GAMMA1)


/*
    reg_r_lut_4:    [0x0, 0x3ff],            bits : 9_0
    reg_r_lut_5:    [0x0, 0x3ff],            bits : 19_10
    reg_r_lut_6:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_R_GAMMA2_OFS 0x0408
REGDEF_BEGIN(SRAM_R_GAMMA2)
REGDEF_BIT(reg_r_lut_4,        10)
REGDEF_BIT(reg_r_lut_5,        10)
REGDEF_BIT(reg_r_lut_6,        10)
REGDEF_END(SRAM_R_GAMMA2)


/*
    reg_r_lut_6:    [0x0, 0x3ff],            bits : 9_0
    reg_r_lut_7:    [0x0, 0x3ff],            bits : 19_10
    reg_r_lut_8:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_R_GAMMA3_OFS 0x040c
REGDEF_BEGIN(SRAM_R_GAMMA3)
REGDEF_BIT(reg_r_lut_6,        10)
REGDEF_BIT(reg_r_lut_7,        10)
REGDEF_BIT(reg_r_lut_8,        10)
REGDEF_END(SRAM_R_GAMMA3)


/*
    reg_r_lut_8 :    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_9 :    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_10:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA4_OFS 0x0410
REGDEF_BEGIN(SRAM_R_GAMMA4)
REGDEF_BIT(reg_r_lut_8,        10)
REGDEF_BIT(reg_r_lut_9,        10)
REGDEF_BIT(reg_r_lut_10,        10)
REGDEF_END(SRAM_R_GAMMA4)


/*
    reg_r_lut_10:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_11:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_12:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA5_OFS 0x0414
REGDEF_BEGIN(SRAM_R_GAMMA5)
REGDEF_BIT(reg_r_lut_10,        10)
REGDEF_BIT(reg_r_lut_11,        10)
REGDEF_BIT(reg_r_lut_12,        10)
REGDEF_END(SRAM_R_GAMMA5)


/*
    reg_r_lut_12:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_13:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_14:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA6_OFS 0x0418
REGDEF_BEGIN(SRAM_R_GAMMA6)
REGDEF_BIT(reg_r_lut_12,        10)
REGDEF_BIT(reg_r_lut_13,        10)
REGDEF_BIT(reg_r_lut_14,        10)
REGDEF_END(SRAM_R_GAMMA6)


/*
    reg_r_lut_14:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_15:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_16:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA7_OFS 0x041c
REGDEF_BEGIN(SRAM_R_GAMMA7)
REGDEF_BIT(reg_r_lut_14,        10)
REGDEF_BIT(reg_r_lut_15,        10)
REGDEF_BIT(reg_r_lut_16,        10)
REGDEF_END(SRAM_R_GAMMA7)


/*
    reg_r_lut_16:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_17:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_18:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA8_OFS 0x0420
REGDEF_BEGIN(SRAM_R_GAMMA8)
REGDEF_BIT(reg_r_lut_16,        10)
REGDEF_BIT(reg_r_lut_17,        10)
REGDEF_BIT(reg_r_lut_18,        10)
REGDEF_END(SRAM_R_GAMMA8)


/*
    reg_r_lut_18:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_19:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_20:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA9_OFS 0x0424
REGDEF_BEGIN(SRAM_R_GAMMA9)
REGDEF_BIT(reg_r_lut_18,        10)
REGDEF_BIT(reg_r_lut_19,        10)
REGDEF_BIT(reg_r_lut_20,        10)
REGDEF_END(SRAM_R_GAMMA9)


/*
    reg_r_lut_20:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_21:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_22:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA10_OFS 0x0428
REGDEF_BEGIN(SRAM_R_GAMMA10)
REGDEF_BIT(reg_r_lut_20,        10)
REGDEF_BIT(reg_r_lut_21,        10)
REGDEF_BIT(reg_r_lut_22,        10)
REGDEF_END(SRAM_R_GAMMA10)


/*
    reg_r_lut_22:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_23:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_24:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA11_OFS 0x042c
REGDEF_BEGIN(SRAM_R_GAMMA11)
REGDEF_BIT(reg_r_lut_22,        10)
REGDEF_BIT(reg_r_lut_23,        10)
REGDEF_BIT(reg_r_lut_24,        10)
REGDEF_END(SRAM_R_GAMMA11)


/*
    reg_r_lut_24:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_25:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_26:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA12_OFS 0x0430
REGDEF_BEGIN(SRAM_R_GAMMA12)
REGDEF_BIT(reg_r_lut_24,        10)
REGDEF_BIT(reg_r_lut_25,        10)
REGDEF_BIT(reg_r_lut_26,        10)
REGDEF_END(SRAM_R_GAMMA12)


/*
    reg_r_lut_26:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_27:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_28:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA13_OFS 0x0434
REGDEF_BEGIN(SRAM_R_GAMMA13)
REGDEF_BIT(reg_r_lut_26,        10)
REGDEF_BIT(reg_r_lut_27,        10)
REGDEF_BIT(reg_r_lut_28,        10)
REGDEF_END(SRAM_R_GAMMA13)


/*
    reg_r_lut_28:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_29:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_30:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA14_OFS 0x0438
REGDEF_BEGIN(SRAM_R_GAMMA14)
REGDEF_BIT(reg_r_lut_28,        10)
REGDEF_BIT(reg_r_lut_29,        10)
REGDEF_BIT(reg_r_lut_30,        10)
REGDEF_END(SRAM_R_GAMMA14)


/*
    reg_r_lut_30:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_31:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_32:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA15_OFS 0x043c
REGDEF_BEGIN(SRAM_R_GAMMA15)
REGDEF_BIT(reg_r_lut_30,        10)
REGDEF_BIT(reg_r_lut_31,        10)
REGDEF_BIT(reg_r_lut_32,        10)
REGDEF_END(SRAM_R_GAMMA15)


/*
    reg_r_lut_32:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_33:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_34:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA16_OFS 0x0440
REGDEF_BEGIN(SRAM_R_GAMMA16)
REGDEF_BIT(reg_r_lut_32,        10)
REGDEF_BIT(reg_r_lut_33,        10)
REGDEF_BIT(reg_r_lut_34,        10)
REGDEF_END(SRAM_R_GAMMA16)


/*
    reg_r_lut_34:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_35:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_36:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA17_OFS 0x0444
REGDEF_BEGIN(SRAM_R_GAMMA17)
REGDEF_BIT(reg_r_lut_34,        10)
REGDEF_BIT(reg_r_lut_35,        10)
REGDEF_BIT(reg_r_lut_36,        10)
REGDEF_END(SRAM_R_GAMMA17)


/*
    reg_r_lut_36:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_37:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_38:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA18_OFS 0x0448
REGDEF_BEGIN(SRAM_R_GAMMA18)
REGDEF_BIT(reg_r_lut_36,        10)
REGDEF_BIT(reg_r_lut_37,        10)
REGDEF_BIT(reg_r_lut_38,        10)
REGDEF_END(SRAM_R_GAMMA18)


/*
    reg_r_lut_38:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_39:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_40:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA19_OFS 0x044c
REGDEF_BEGIN(SRAM_R_GAMMA19)
REGDEF_BIT(reg_r_lut_38,        10)
REGDEF_BIT(reg_r_lut_39,        10)
REGDEF_BIT(reg_r_lut_40,        10)
REGDEF_END(SRAM_R_GAMMA19)


/*
    reg_r_lut_40:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_41:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_42:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA20_OFS 0x0450
REGDEF_BEGIN(SRAM_R_GAMMA20)
REGDEF_BIT(reg_r_lut_40,        10)
REGDEF_BIT(reg_r_lut_41,        10)
REGDEF_BIT(reg_r_lut_42,        10)
REGDEF_END(SRAM_R_GAMMA20)


/*
    reg_r_lut_42:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_43:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_44:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA21_OFS 0x0454
REGDEF_BEGIN(SRAM_R_GAMMA21)
REGDEF_BIT(reg_r_lut_42,        10)
REGDEF_BIT(reg_r_lut_43,        10)
REGDEF_BIT(reg_r_lut_44,        10)
REGDEF_END(SRAM_R_GAMMA21)


/*
    reg_r_lut_44:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_45:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_46:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA22_OFS 0x0458
REGDEF_BEGIN(SRAM_R_GAMMA22)
REGDEF_BIT(reg_r_lut_44,        10)
REGDEF_BIT(reg_r_lut_45,        10)
REGDEF_BIT(reg_r_lut_46,        10)
REGDEF_END(SRAM_R_GAMMA22)


/*
    reg_r_lut_46:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_47:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_48:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA23_OFS 0x045c
REGDEF_BEGIN(SRAM_R_GAMMA23)
REGDEF_BIT(reg_r_lut_46,        10)
REGDEF_BIT(reg_r_lut_47,        10)
REGDEF_BIT(reg_r_lut_48,        10)
REGDEF_END(SRAM_R_GAMMA23)


/*
    reg_r_lut_48:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_49:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_50:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA24_OFS 0x0460
REGDEF_BEGIN(SRAM_R_GAMMA24)
REGDEF_BIT(reg_r_lut_48,        10)
REGDEF_BIT(reg_r_lut_49,        10)
REGDEF_BIT(reg_r_lut_50,        10)
REGDEF_END(SRAM_R_GAMMA24)


/*
    reg_r_lut_50:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_51:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_52:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA25_OFS 0x0464
REGDEF_BEGIN(SRAM_R_GAMMA25)
REGDEF_BIT(reg_r_lut_50,        10)
REGDEF_BIT(reg_r_lut_51,        10)
REGDEF_BIT(reg_r_lut_52,        10)
REGDEF_END(SRAM_R_GAMMA25)


/*
    reg_r_lut_52:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_53:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_54:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA26_OFS 0x0468
REGDEF_BEGIN(SRAM_R_GAMMA26)
REGDEF_BIT(reg_r_lut_52,        10)
REGDEF_BIT(reg_r_lut_53,        10)
REGDEF_BIT(reg_r_lut_54,        10)
REGDEF_END(SRAM_R_GAMMA26)


/*
    reg_r_lut_54:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_55:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_56:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA27_OFS 0x046c
REGDEF_BEGIN(SRAM_R_GAMMA27)
REGDEF_BIT(reg_r_lut_54,        10)
REGDEF_BIT(reg_r_lut_55,        10)
REGDEF_BIT(reg_r_lut_56,        10)
REGDEF_END(SRAM_R_GAMMA27)


/*
    reg_r_lut_56:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_57:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_58:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA28_OFS 0x0470
REGDEF_BEGIN(SRAM_R_GAMMA28)
REGDEF_BIT(reg_r_lut_56,        10)
REGDEF_BIT(reg_r_lut_57,        10)
REGDEF_BIT(reg_r_lut_58,        10)
REGDEF_END(SRAM_R_GAMMA28)


/*
    reg_r_lut_58:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_59:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_60:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA29_OFS 0x0474
REGDEF_BEGIN(SRAM_R_GAMMA29)
REGDEF_BIT(reg_r_lut_58,        10)
REGDEF_BIT(reg_r_lut_59,        10)
REGDEF_BIT(reg_r_lut_60,        10)
REGDEF_END(SRAM_R_GAMMA29)


/*
    reg_r_lut_60:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_61:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_62:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA30_OFS 0x0478
REGDEF_BEGIN(SRAM_R_GAMMA30)
REGDEF_BIT(reg_r_lut_60,        10)
REGDEF_BIT(reg_r_lut_61,        10)
REGDEF_BIT(reg_r_lut_62,        10)
REGDEF_END(SRAM_R_GAMMA30)


/*
    reg_r_lut_62:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_63:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_64:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA31_OFS 0x047c
REGDEF_BEGIN(SRAM_R_GAMMA31)
REGDEF_BIT(reg_r_lut_62,        10)
REGDEF_BIT(reg_r_lut_63,        10)
REGDEF_BIT(reg_r_lut_64,        10)
REGDEF_END(SRAM_R_GAMMA31)


/*
    reg_r_lut_64:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_65:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_66:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA32_OFS 0x0480
REGDEF_BEGIN(SRAM_R_GAMMA32)
REGDEF_BIT(reg_r_lut_64,        10)
REGDEF_BIT(reg_r_lut_65,        10)
REGDEF_BIT(reg_r_lut_66,        10)
REGDEF_END(SRAM_R_GAMMA32)


/*
    reg_r_lut_66:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_67:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_68:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA33_OFS 0x0484
REGDEF_BEGIN(SRAM_R_GAMMA33)
REGDEF_BIT(reg_r_lut_66,        10)
REGDEF_BIT(reg_r_lut_67,        10)
REGDEF_BIT(reg_r_lut_68,        10)
REGDEF_END(SRAM_R_GAMMA33)


/*
    reg_r_lut_68:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_69:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_70:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA34_OFS 0x0488
REGDEF_BEGIN(SRAM_R_GAMMA34)
REGDEF_BIT(reg_r_lut_68,        10)
REGDEF_BIT(reg_r_lut_69,        10)
REGDEF_BIT(reg_r_lut_70,        10)
REGDEF_END(SRAM_R_GAMMA34)


/*
    reg_r_lut_70:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_71:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_72:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA35_OFS 0x048c
REGDEF_BEGIN(SRAM_R_GAMMA35)
REGDEF_BIT(reg_r_lut_70,        10)
REGDEF_BIT(reg_r_lut_71,        10)
REGDEF_BIT(reg_r_lut_72,        10)
REGDEF_END(SRAM_R_GAMMA35)


/*
    reg_r_lut_72:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_73:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_74:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA36_OFS 0x0490
REGDEF_BEGIN(SRAM_R_GAMMA36)
REGDEF_BIT(reg_r_lut_72,        10)
REGDEF_BIT(reg_r_lut_73,        10)
REGDEF_BIT(reg_r_lut_74,        10)
REGDEF_END(SRAM_R_GAMMA36)


/*
    reg_r_lut_74:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_75:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_76:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA37_OFS 0x0494
REGDEF_BEGIN(SRAM_R_GAMMA37)
REGDEF_BIT(reg_r_lut_74,        10)
REGDEF_BIT(reg_r_lut_75,        10)
REGDEF_BIT(reg_r_lut_76,        10)
REGDEF_END(SRAM_R_GAMMA37)


/*
    reg_r_lut_76:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_77:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_78:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA38_OFS 0x0498
REGDEF_BEGIN(SRAM_R_GAMMA38)
REGDEF_BIT(reg_r_lut_76,        10)
REGDEF_BIT(reg_r_lut_77,        10)
REGDEF_BIT(reg_r_lut_78,        10)
REGDEF_END(SRAM_R_GAMMA38)


/*
    reg_r_lut_78:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_79:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_80:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA39_OFS 0x049c
REGDEF_BEGIN(SRAM_R_GAMMA39)
REGDEF_BIT(reg_r_lut_78,        10)
REGDEF_BIT(reg_r_lut_79,        10)
REGDEF_BIT(reg_r_lut_80,        10)
REGDEF_END(SRAM_R_GAMMA39)


/*
    reg_r_lut_80:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_81:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_82:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA40_OFS 0x04a0
REGDEF_BEGIN(SRAM_R_GAMMA40)
REGDEF_BIT(reg_r_lut_80,        10)
REGDEF_BIT(reg_r_lut_81,        10)
REGDEF_BIT(reg_r_lut_82,        10)
REGDEF_END(SRAM_R_GAMMA40)


/*
    reg_r_lut_82:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_83:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_84:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA41_OFS 0x04a4
REGDEF_BEGIN(SRAM_R_GAMMA41)
REGDEF_BIT(reg_r_lut_82,        10)
REGDEF_BIT(reg_r_lut_83,        10)
REGDEF_BIT(reg_r_lut_84,        10)
REGDEF_END(SRAM_R_GAMMA41)


/*
    reg_r_lut_84:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_85:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_86:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA42_OFS 0x04a8
REGDEF_BEGIN(SRAM_R_GAMMA42)
REGDEF_BIT(reg_r_lut_84,        10)
REGDEF_BIT(reg_r_lut_85,        10)
REGDEF_BIT(reg_r_lut_86,        10)
REGDEF_END(SRAM_R_GAMMA42)


/*
    reg_r_lut_86:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_87:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_88:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA43_OFS 0x04ac
REGDEF_BEGIN(SRAM_R_GAMMA43)
REGDEF_BIT(reg_r_lut_86,        10)
REGDEF_BIT(reg_r_lut_87,        10)
REGDEF_BIT(reg_r_lut_88,        10)
REGDEF_END(SRAM_R_GAMMA43)


/*
    reg_r_lut_88:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_89:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_90:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA44_OFS 0x04b0
REGDEF_BEGIN(SRAM_R_GAMMA44)
REGDEF_BIT(reg_r_lut_88,        10)
REGDEF_BIT(reg_r_lut_89,        10)
REGDEF_BIT(reg_r_lut_90,        10)
REGDEF_END(SRAM_R_GAMMA44)


/*
    reg_r_lut_90:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_91:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_92:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA45_OFS 0x04b4
REGDEF_BEGIN(SRAM_R_GAMMA45)
REGDEF_BIT(reg_r_lut_90,        10)
REGDEF_BIT(reg_r_lut_91,        10)
REGDEF_BIT(reg_r_lut_92,        10)
REGDEF_END(SRAM_R_GAMMA45)


/*
    reg_r_lut_92:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_93:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_94:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA46_OFS 0x04b8
REGDEF_BEGIN(SRAM_R_GAMMA46)
REGDEF_BIT(reg_r_lut_92,        10)
REGDEF_BIT(reg_r_lut_93,        10)
REGDEF_BIT(reg_r_lut_94,        10)
REGDEF_END(SRAM_R_GAMMA46)


/*
    reg_r_lut_94:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_95:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_96:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA47_OFS 0x04bc
REGDEF_BEGIN(SRAM_R_GAMMA47)
REGDEF_BIT(reg_r_lut_94,        10)
REGDEF_BIT(reg_r_lut_95,        10)
REGDEF_BIT(reg_r_lut_96,        10)
REGDEF_END(SRAM_R_GAMMA47)


/*
    reg_r_lut_96:    [0x0, 0x3ff],           bits : 9_0
    reg_r_lut_97:    [0x0, 0x3ff],           bits : 19_10
    reg_r_lut_98:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_R_GAMMA48_OFS 0x04c0
REGDEF_BEGIN(SRAM_R_GAMMA48)
REGDEF_BIT(reg_r_lut_96,        10)
REGDEF_BIT(reg_r_lut_97,        10)
REGDEF_BIT(reg_r_lut_98,        10)
REGDEF_END(SRAM_R_GAMMA48)


/*
    reg_r_lut_98 :    [0x0, 0x3ff],          bits : 9_0
    reg_r_lut_99 :    [0x0, 0x3ff],          bits : 19_10
    reg_r_lut_100:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA49_OFS 0x04c4
REGDEF_BEGIN(SRAM_R_GAMMA49)
REGDEF_BIT(reg_r_lut_98,        10)
REGDEF_BIT(reg_r_lut_99,        10)
REGDEF_BIT(reg_r_lut_100,        10)
REGDEF_END(SRAM_R_GAMMA49)


/*
    reg_r_lut_100:    [0x0, 0x3ff],          bits : 9_0
    reg_r_lut_101:    [0x0, 0x3ff],          bits : 19_10
    reg_r_lut_102:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA50_OFS 0x04c8
REGDEF_BEGIN(SRAM_R_GAMMA50)
REGDEF_BIT(reg_r_lut_100,        10)
REGDEF_BIT(reg_r_lut_101,        10)
REGDEF_BIT(reg_r_lut_102,        10)
REGDEF_END(SRAM_R_GAMMA50)


/*
    reg_r_lut_102:    [0x0, 0x3ff],          bits : 9_0
    reg_r_lut_103:    [0x0, 0x3ff],          bits : 19_10
    reg_r_lut_104:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA51_OFS 0x04cc
REGDEF_BEGIN(SRAM_R_GAMMA51)
REGDEF_BIT(reg_r_lut_102,        10)
REGDEF_BIT(reg_r_lut_103,        10)
REGDEF_BIT(reg_r_lut_104,        10)
REGDEF_END(SRAM_R_GAMMA51)


/*
    reg_r_lut_104:    [0x0, 0x3ff],          bits : 9_0
    reg_r_lut_105:    [0x0, 0x3ff],          bits : 19_10
    reg_r_lut_106:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA52_OFS 0x04d0
REGDEF_BEGIN(SRAM_R_GAMMA52)
REGDEF_BIT(reg_r_lut_104,        10)
REGDEF_BIT(reg_r_lut_105,        10)
REGDEF_BIT(reg_r_lut_106,        10)
REGDEF_END(SRAM_R_GAMMA52)


/*
    reg_r_lut_106:    [0x0, 0x3ff],          bits : 9_0
    reg_r_lut_107:    [0x0, 0x3ff],          bits : 19_10
    reg_r_lut_108:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA53_OFS 0x04d4
REGDEF_BEGIN(SRAM_R_GAMMA53)
REGDEF_BIT(reg_r_lut_106,        10)
REGDEF_BIT(reg_r_lut_107,        10)
REGDEF_BIT(reg_r_lut_108,        10)
REGDEF_END(SRAM_R_GAMMA53)


/*
    reg_r_lut_108:    [0x0, 0x3ff],          bits : 9_0
    reg_r_lut_109:    [0x0, 0x3ff],          bits : 19_10
    reg_r_lut_110:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA54_OFS 0x04d8
REGDEF_BEGIN(SRAM_R_GAMMA54)
REGDEF_BIT(reg_r_lut_108,        10)
REGDEF_BIT(reg_r_lut_109,        10)
REGDEF_BIT(reg_r_lut_110,        10)
REGDEF_END(SRAM_R_GAMMA54)


/*
    reg_r_lut_110:    [0x0, 0x3ff],          bits : 9_0
    reg_r_lut_111:    [0x0, 0x3ff],          bits : 19_10
    reg_r_lut_112:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA55_OFS 0x04dc
REGDEF_BEGIN(SRAM_R_GAMMA55)
REGDEF_BIT(reg_r_lut_110,        10)
REGDEF_BIT(reg_r_lut_111,        10)
REGDEF_BIT(reg_r_lut_112,        10)
REGDEF_END(SRAM_R_GAMMA55)


/*
    reg_r_lut_112:    [0x0, 0x3ff],          bits : 9_0
    reg_r_lut_113:    [0x0, 0x3ff],          bits : 19_10
    reg_r_lut_114:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA56_OFS 0x04e0
REGDEF_BEGIN(SRAM_R_GAMMA56)
REGDEF_BIT(reg_r_lut_112,        10)
REGDEF_BIT(reg_r_lut_113,        10)
REGDEF_BIT(reg_r_lut_114,        10)
REGDEF_END(SRAM_R_GAMMA56)


/*
    reg_r_lut_114:    [0x0, 0x3ff],          bits : 9_0
    reg_r_lut_115:    [0x0, 0x3ff],          bits : 19_10
    reg_r_lut_116:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA57_OFS 0x04e4
REGDEF_BEGIN(SRAM_R_GAMMA57)
REGDEF_BIT(reg_r_lut_114,        10)
REGDEF_BIT(reg_r_lut_115,        10)
REGDEF_BIT(reg_r_lut_116,        10)
REGDEF_END(SRAM_R_GAMMA57)


/*
    reg_r_lut_116:    [0x0, 0x3ff],          bits : 9_0
    reg_r_lut_117:    [0x0, 0x3ff],          bits : 19_10
    reg_r_lut_118:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA58_OFS 0x04e8
REGDEF_BEGIN(SRAM_R_GAMMA58)
REGDEF_BIT(reg_r_lut_116,        10)
REGDEF_BIT(reg_r_lut_117,        10)
REGDEF_BIT(reg_r_lut_118,        10)
REGDEF_END(SRAM_R_GAMMA58)


/*
    reg_r_lut_118:    [0x0, 0x3ff],          bits : 9_0
    reg_r_lut_119:    [0x0, 0x3ff],          bits : 19_10
    reg_r_lut_120:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA59_OFS 0x04ec
REGDEF_BEGIN(SRAM_R_GAMMA59)
REGDEF_BIT(reg_r_lut_118,        10)
REGDEF_BIT(reg_r_lut_119,        10)
REGDEF_BIT(reg_r_lut_120,        10)
REGDEF_END(SRAM_R_GAMMA59)


/*
    reg_r_lut_120:    [0x0, 0x3ff],          bits : 9_0
    reg_r_lut_121:    [0x0, 0x3ff],          bits : 19_10
    reg_r_lut_122:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA60_OFS 0x04f0
REGDEF_BEGIN(SRAM_R_GAMMA60)
REGDEF_BIT(reg_r_lut_120,        10)
REGDEF_BIT(reg_r_lut_121,        10)
REGDEF_BIT(reg_r_lut_122,        10)
REGDEF_END(SRAM_R_GAMMA60)


/*
    reg_r_lut_122:    [0x0, 0x3ff],          bits : 9_0
    reg_r_lut_123:    [0x0, 0x3ff],          bits : 19_10
    reg_r_lut_124:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA61_OFS 0x04f4
REGDEF_BEGIN(SRAM_R_GAMMA61)
REGDEF_BIT(reg_r_lut_122,        10)
REGDEF_BIT(reg_r_lut_123,        10)
REGDEF_BIT(reg_r_lut_124,        10)
REGDEF_END(SRAM_R_GAMMA61)


/*
    reg_r_lut_124:    [0x0, 0x3ff],          bits : 9_0
    reg_r_lut_125:    [0x0, 0x3ff],          bits : 19_10
    reg_r_lut_126:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA62_OFS 0x04f8
REGDEF_BEGIN(SRAM_R_GAMMA62)
REGDEF_BIT(reg_r_lut_124,        10)
REGDEF_BIT(reg_r_lut_125,        10)
REGDEF_BIT(reg_r_lut_126,        10)
REGDEF_END(SRAM_R_GAMMA62)


/*
    reg_r_lut_126:    [0x0, 0x3ff],          bits : 9_0
    reg_r_lut_127:    [0x0, 0x3ff],          bits : 19_10
    reg_r_lut_128:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_R_GAMMA63_OFS 0x04fc
REGDEF_BEGIN(SRAM_R_GAMMA63)
REGDEF_BIT(reg_r_lut_126,        10)
REGDEF_BIT(reg_r_lut_127,        10)
REGDEF_BIT(reg_r_lut_128,        10)
REGDEF_END(SRAM_R_GAMMA63)


/*
    reg_g_lut_0:    [0x0, 0x3ff],            bits : 9_0
    reg_g_lut_1:    [0x0, 0x3ff],            bits : 19_10
    reg_g_lut_2:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_G_GAMMA0_OFS 0x0500
REGDEF_BEGIN(SRAM_G_GAMMA0)
REGDEF_BIT(reg_g_lut_0,        10)
REGDEF_BIT(reg_g_lut_1,        10)
REGDEF_BIT(reg_g_lut_2,        10)
REGDEF_END(SRAM_G_GAMMA0)


/*
    reg_g_lut_2:    [0x0, 0x3ff],            bits : 9_0
    reg_g_lut_3:    [0x0, 0x3ff],            bits : 19_10
    reg_g_lut_4:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_G_GAMMA1_OFS 0x0504
REGDEF_BEGIN(SRAM_G_GAMMA1)
REGDEF_BIT(reg_g_lut_2,        10)
REGDEF_BIT(reg_g_lut_3,        10)
REGDEF_BIT(reg_g_lut_4,        10)
REGDEF_END(SRAM_G_GAMMA1)


/*
    reg_g_lut_4:    [0x0, 0x3ff],            bits : 9_0
    reg_g_lut_5:    [0x0, 0x3ff],            bits : 19_10
    reg_g_lut_6:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_G_GAMMA2_OFS 0x0508
REGDEF_BEGIN(SRAM_G_GAMMA2)
REGDEF_BIT(reg_g_lut_4,        10)
REGDEF_BIT(reg_g_lut_5,        10)
REGDEF_BIT(reg_g_lut_6,        10)
REGDEF_END(SRAM_G_GAMMA2)


/*
    reg_g_lut_6:    [0x0, 0x3ff],            bits : 9_0
    reg_g_lut_7:    [0x0, 0x3ff],            bits : 19_10
    reg_g_lut_8:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_G_GAMMA3_OFS 0x050c
REGDEF_BEGIN(SRAM_G_GAMMA3)
REGDEF_BIT(reg_g_lut_6,        10)
REGDEF_BIT(reg_g_lut_7,        10)
REGDEF_BIT(reg_g_lut_8,        10)
REGDEF_END(SRAM_G_GAMMA3)


/*
    reg_g_lut_8 :    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_9 :    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_10:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA4_OFS 0x0510
REGDEF_BEGIN(SRAM_G_GAMMA4)
REGDEF_BIT(reg_g_lut_8,        10)
REGDEF_BIT(reg_g_lut_9,        10)
REGDEF_BIT(reg_g_lut_10,        10)
REGDEF_END(SRAM_G_GAMMA4)


/*
    reg_g_lut_10:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_11:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_12:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA5_OFS 0x0514
REGDEF_BEGIN(SRAM_G_GAMMA5)
REGDEF_BIT(reg_g_lut_10,        10)
REGDEF_BIT(reg_g_lut_11,        10)
REGDEF_BIT(reg_g_lut_12,        10)
REGDEF_END(SRAM_G_GAMMA5)


/*
    reg_g_lut_12:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_13:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_14:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA6_OFS 0x0518
REGDEF_BEGIN(SRAM_G_GAMMA6)
REGDEF_BIT(reg_g_lut_12,        10)
REGDEF_BIT(reg_g_lut_13,        10)
REGDEF_BIT(reg_g_lut_14,        10)
REGDEF_END(SRAM_G_GAMMA6)


/*
    reg_g_lut_14:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_15:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_16:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA7_OFS 0x051c
REGDEF_BEGIN(SRAM_G_GAMMA7)
REGDEF_BIT(reg_g_lut_14,        10)
REGDEF_BIT(reg_g_lut_15,        10)
REGDEF_BIT(reg_g_lut_16,        10)
REGDEF_END(SRAM_G_GAMMA7)


/*
    reg_g_lut_16:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_17:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_18:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA8_OFS 0x0520
REGDEF_BEGIN(SRAM_G_GAMMA8)
REGDEF_BIT(reg_g_lut_16,        10)
REGDEF_BIT(reg_g_lut_17,        10)
REGDEF_BIT(reg_g_lut_18,        10)
REGDEF_END(SRAM_G_GAMMA8)


/*
    reg_g_lut_18:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_19:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_20:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA9_OFS 0x0524
REGDEF_BEGIN(SRAM_G_GAMMA9)
REGDEF_BIT(reg_g_lut_18,        10)
REGDEF_BIT(reg_g_lut_19,        10)
REGDEF_BIT(reg_g_lut_20,        10)
REGDEF_END(SRAM_G_GAMMA9)


/*
    reg_g_lut_20:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_21:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_22:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA10_OFS 0x0528
REGDEF_BEGIN(SRAM_G_GAMMA10)
REGDEF_BIT(reg_g_lut_20,        10)
REGDEF_BIT(reg_g_lut_21,        10)
REGDEF_BIT(reg_g_lut_22,        10)
REGDEF_END(SRAM_G_GAMMA10)


/*
    reg_g_lut_22:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_23:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_24:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA11_OFS 0x052c
REGDEF_BEGIN(SRAM_G_GAMMA11)
REGDEF_BIT(reg_g_lut_22,        10)
REGDEF_BIT(reg_g_lut_23,        10)
REGDEF_BIT(reg_g_lut_24,        10)
REGDEF_END(SRAM_G_GAMMA11)


/*
    reg_g_lut_24:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_25:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_26:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA12_OFS 0x0530
REGDEF_BEGIN(SRAM_G_GAMMA12)
REGDEF_BIT(reg_g_lut_24,        10)
REGDEF_BIT(reg_g_lut_25,        10)
REGDEF_BIT(reg_g_lut_26,        10)
REGDEF_END(SRAM_G_GAMMA12)


/*
    reg_g_lut_26:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_27:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_28:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA13_OFS 0x0534
REGDEF_BEGIN(SRAM_G_GAMMA13)
REGDEF_BIT(reg_g_lut_26,        10)
REGDEF_BIT(reg_g_lut_27,        10)
REGDEF_BIT(reg_g_lut_28,        10)
REGDEF_END(SRAM_G_GAMMA13)


/*
    reg_g_lut_28:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_29:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_30:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA14_OFS 0x0538
REGDEF_BEGIN(SRAM_G_GAMMA14)
REGDEF_BIT(reg_g_lut_28,        10)
REGDEF_BIT(reg_g_lut_29,        10)
REGDEF_BIT(reg_g_lut_30,        10)
REGDEF_END(SRAM_G_GAMMA14)


/*
    reg_g_lut_30:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_31:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_32:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA15_OFS 0x053c
REGDEF_BEGIN(SRAM_G_GAMMA15)
REGDEF_BIT(reg_g_lut_30,        10)
REGDEF_BIT(reg_g_lut_31,        10)
REGDEF_BIT(reg_g_lut_32,        10)
REGDEF_END(SRAM_G_GAMMA15)


/*
    reg_g_lut_32:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_33:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_34:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA16_OFS 0x0540
REGDEF_BEGIN(SRAM_G_GAMMA16)
REGDEF_BIT(reg_g_lut_32,        10)
REGDEF_BIT(reg_g_lut_33,        10)
REGDEF_BIT(reg_g_lut_34,        10)
REGDEF_END(SRAM_G_GAMMA16)


/*
    reg_g_lut_34:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_35:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_36:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA17_OFS 0x0544
REGDEF_BEGIN(SRAM_G_GAMMA17)
REGDEF_BIT(reg_g_lut_34,        10)
REGDEF_BIT(reg_g_lut_35,        10)
REGDEF_BIT(reg_g_lut_36,        10)
REGDEF_END(SRAM_G_GAMMA17)


/*
    reg_g_lut_36:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_37:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_38:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA18_OFS 0x0548
REGDEF_BEGIN(SRAM_G_GAMMA18)
REGDEF_BIT(reg_g_lut_36,        10)
REGDEF_BIT(reg_g_lut_37,        10)
REGDEF_BIT(reg_g_lut_38,        10)
REGDEF_END(SRAM_G_GAMMA18)


/*
    reg_g_lut_38:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_39:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_40:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA19_OFS 0x054c
REGDEF_BEGIN(SRAM_G_GAMMA19)
REGDEF_BIT(reg_g_lut_38,        10)
REGDEF_BIT(reg_g_lut_39,        10)
REGDEF_BIT(reg_g_lut_40,        10)
REGDEF_END(SRAM_G_GAMMA19)


/*
    reg_g_lut_40:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_41:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_42:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA20_OFS 0x0550
REGDEF_BEGIN(SRAM_G_GAMMA20)
REGDEF_BIT(reg_g_lut_40,        10)
REGDEF_BIT(reg_g_lut_41,        10)
REGDEF_BIT(reg_g_lut_42,        10)
REGDEF_END(SRAM_G_GAMMA20)


/*
    reg_g_lut_42:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_43:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_44:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA21_OFS 0x0554
REGDEF_BEGIN(SRAM_G_GAMMA21)
REGDEF_BIT(reg_g_lut_42,        10)
REGDEF_BIT(reg_g_lut_43,        10)
REGDEF_BIT(reg_g_lut_44,        10)
REGDEF_END(SRAM_G_GAMMA21)


/*
    reg_g_lut_44:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_45:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_46:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA22_OFS 0x0558
REGDEF_BEGIN(SRAM_G_GAMMA22)
REGDEF_BIT(reg_g_lut_44,        10)
REGDEF_BIT(reg_g_lut_45,        10)
REGDEF_BIT(reg_g_lut_46,        10)
REGDEF_END(SRAM_G_GAMMA22)


/*
    reg_g_lut_46:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_47:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_48:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA23_OFS 0x055c
REGDEF_BEGIN(SRAM_G_GAMMA23)
REGDEF_BIT(reg_g_lut_46,        10)
REGDEF_BIT(reg_g_lut_47,        10)
REGDEF_BIT(reg_g_lut_48,        10)
REGDEF_END(SRAM_G_GAMMA23)


/*
    reg_g_lut_48:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_49:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_50:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA24_OFS 0x0560
REGDEF_BEGIN(SRAM_G_GAMMA24)
REGDEF_BIT(reg_g_lut_48,        10)
REGDEF_BIT(reg_g_lut_49,        10)
REGDEF_BIT(reg_g_lut_50,        10)
REGDEF_END(SRAM_G_GAMMA24)


/*
    reg_g_lut_50:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_51:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_52:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA25_OFS 0x0564
REGDEF_BEGIN(SRAM_G_GAMMA25)
REGDEF_BIT(reg_g_lut_50,        10)
REGDEF_BIT(reg_g_lut_51,        10)
REGDEF_BIT(reg_g_lut_52,        10)
REGDEF_END(SRAM_G_GAMMA25)


/*
    reg_g_lut_52:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_53:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_54:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA26_OFS 0x0568
REGDEF_BEGIN(SRAM_G_GAMMA26)
REGDEF_BIT(reg_g_lut_52,        10)
REGDEF_BIT(reg_g_lut_53,        10)
REGDEF_BIT(reg_g_lut_54,        10)
REGDEF_END(SRAM_G_GAMMA26)


/*
    reg_g_lut_54:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_55:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_56:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA27_OFS 0x056c
REGDEF_BEGIN(SRAM_G_GAMMA27)
REGDEF_BIT(reg_g_lut_54,        10)
REGDEF_BIT(reg_g_lut_55,        10)
REGDEF_BIT(reg_g_lut_56,        10)
REGDEF_END(SRAM_G_GAMMA27)


/*
    reg_g_lut_56:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_57:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_58:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA28_OFS 0x0570
REGDEF_BEGIN(SRAM_G_GAMMA28)
REGDEF_BIT(reg_g_lut_56,        10)
REGDEF_BIT(reg_g_lut_57,        10)
REGDEF_BIT(reg_g_lut_58,        10)
REGDEF_END(SRAM_G_GAMMA28)


/*
    reg_g_lut_58:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_59:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_60:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA29_OFS 0x0574
REGDEF_BEGIN(SRAM_G_GAMMA29)
REGDEF_BIT(reg_g_lut_58,        10)
REGDEF_BIT(reg_g_lut_59,        10)
REGDEF_BIT(reg_g_lut_60,        10)
REGDEF_END(SRAM_G_GAMMA29)


/*
    reg_g_lut_60:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_61:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_62:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA30_OFS 0x0578
REGDEF_BEGIN(SRAM_G_GAMMA30)
REGDEF_BIT(reg_g_lut_60,        10)
REGDEF_BIT(reg_g_lut_61,        10)
REGDEF_BIT(reg_g_lut_62,        10)
REGDEF_END(SRAM_G_GAMMA30)


/*
    reg_g_lut_62:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_63:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_64:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA31_OFS 0x057c
REGDEF_BEGIN(SRAM_G_GAMMA31)
REGDEF_BIT(reg_g_lut_62,        10)
REGDEF_BIT(reg_g_lut_63,        10)
REGDEF_BIT(reg_g_lut_64,        10)
REGDEF_END(SRAM_G_GAMMA31)


/*
    reg_g_lut_64:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_65:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_66:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA32_OFS 0x0580
REGDEF_BEGIN(SRAM_G_GAMMA32)
REGDEF_BIT(reg_g_lut_64,        10)
REGDEF_BIT(reg_g_lut_65,        10)
REGDEF_BIT(reg_g_lut_66,        10)
REGDEF_END(SRAM_G_GAMMA32)


/*
    reg_g_lut_66:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_67:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_68:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA33_OFS 0x0584
REGDEF_BEGIN(SRAM_G_GAMMA33)
REGDEF_BIT(reg_g_lut_66,        10)
REGDEF_BIT(reg_g_lut_67,        10)
REGDEF_BIT(reg_g_lut_68,        10)
REGDEF_END(SRAM_G_GAMMA33)


/*
    reg_g_lut_68:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_69:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_70:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA34_OFS 0x0588
REGDEF_BEGIN(SRAM_G_GAMMA34)
REGDEF_BIT(reg_g_lut_68,        10)
REGDEF_BIT(reg_g_lut_69,        10)
REGDEF_BIT(reg_g_lut_70,        10)
REGDEF_END(SRAM_G_GAMMA34)


/*
    reg_g_lut_70:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_71:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_72:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA35_OFS 0x058c
REGDEF_BEGIN(SRAM_G_GAMMA35)
REGDEF_BIT(reg_g_lut_70,        10)
REGDEF_BIT(reg_g_lut_71,        10)
REGDEF_BIT(reg_g_lut_72,        10)
REGDEF_END(SRAM_G_GAMMA35)


/*
    reg_g_lut_72:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_73:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_74:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA36_OFS 0x0590
REGDEF_BEGIN(SRAM_G_GAMMA36)
REGDEF_BIT(reg_g_lut_72,        10)
REGDEF_BIT(reg_g_lut_73,        10)
REGDEF_BIT(reg_g_lut_74,        10)
REGDEF_END(SRAM_G_GAMMA36)


/*
    reg_g_lut_74:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_75:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_76:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA37_OFS 0x0594
REGDEF_BEGIN(SRAM_G_GAMMA37)
REGDEF_BIT(reg_g_lut_74,        10)
REGDEF_BIT(reg_g_lut_75,        10)
REGDEF_BIT(reg_g_lut_76,        10)
REGDEF_END(SRAM_G_GAMMA37)


/*
    reg_g_lut_76:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_77:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_78:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA38_OFS 0x0598
REGDEF_BEGIN(SRAM_G_GAMMA38)
REGDEF_BIT(reg_g_lut_76,        10)
REGDEF_BIT(reg_g_lut_77,        10)
REGDEF_BIT(reg_g_lut_78,        10)
REGDEF_END(SRAM_G_GAMMA38)


/*
    reg_g_lut_78:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_79:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_80:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA39_OFS 0x059c
REGDEF_BEGIN(SRAM_G_GAMMA39)
REGDEF_BIT(reg_g_lut_78,        10)
REGDEF_BIT(reg_g_lut_79,        10)
REGDEF_BIT(reg_g_lut_80,        10)
REGDEF_END(SRAM_G_GAMMA39)


/*
    reg_g_lut_80:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_81:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_82:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA40_OFS 0x05a0
REGDEF_BEGIN(SRAM_G_GAMMA40)
REGDEF_BIT(reg_g_lut_80,        10)
REGDEF_BIT(reg_g_lut_81,        10)
REGDEF_BIT(reg_g_lut_82,        10)
REGDEF_END(SRAM_G_GAMMA40)


/*
    reg_g_lut_82:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_83:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_84:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA41_OFS 0x05a4
REGDEF_BEGIN(SRAM_G_GAMMA41)
REGDEF_BIT(reg_g_lut_82,        10)
REGDEF_BIT(reg_g_lut_83,        10)
REGDEF_BIT(reg_g_lut_84,        10)
REGDEF_END(SRAM_G_GAMMA41)


/*
    reg_g_lut_84:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_85:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_86:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA42_OFS 0x05a8
REGDEF_BEGIN(SRAM_G_GAMMA42)
REGDEF_BIT(reg_g_lut_84,        10)
REGDEF_BIT(reg_g_lut_85,        10)
REGDEF_BIT(reg_g_lut_86,        10)
REGDEF_END(SRAM_G_GAMMA42)


/*
    reg_g_lut_86:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_87:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_88:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA43_OFS 0x05ac
REGDEF_BEGIN(SRAM_G_GAMMA43)
REGDEF_BIT(reg_g_lut_86,        10)
REGDEF_BIT(reg_g_lut_87,        10)
REGDEF_BIT(reg_g_lut_88,        10)
REGDEF_END(SRAM_G_GAMMA43)


/*
    reg_g_lut_88:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_89:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_90:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA44_OFS 0x05b0
REGDEF_BEGIN(SRAM_G_GAMMA44)
REGDEF_BIT(reg_g_lut_88,        10)
REGDEF_BIT(reg_g_lut_89,        10)
REGDEF_BIT(reg_g_lut_90,        10)
REGDEF_END(SRAM_G_GAMMA44)


/*
    reg_g_lut_90:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_91:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_92:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA45_OFS 0x05b4
REGDEF_BEGIN(SRAM_G_GAMMA45)
REGDEF_BIT(reg_g_lut_90,        10)
REGDEF_BIT(reg_g_lut_91,        10)
REGDEF_BIT(reg_g_lut_92,        10)
REGDEF_END(SRAM_G_GAMMA45)


/*
    reg_g_lut_92:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_93:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_94:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA46_OFS 0x05b8
REGDEF_BEGIN(SRAM_G_GAMMA46)
REGDEF_BIT(reg_g_lut_92,        10)
REGDEF_BIT(reg_g_lut_93,        10)
REGDEF_BIT(reg_g_lut_94,        10)
REGDEF_END(SRAM_G_GAMMA46)


/*
    reg_g_lut_94:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_95:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_96:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA47_OFS 0x05bc
REGDEF_BEGIN(SRAM_G_GAMMA47)
REGDEF_BIT(reg_g_lut_94,        10)
REGDEF_BIT(reg_g_lut_95,        10)
REGDEF_BIT(reg_g_lut_96,        10)
REGDEF_END(SRAM_G_GAMMA47)


/*
    reg_g_lut_96:    [0x0, 0x3ff],           bits : 9_0
    reg_g_lut_97:    [0x0, 0x3ff],           bits : 19_10
    reg_g_lut_98:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_G_GAMMA48_OFS 0x05c0
REGDEF_BEGIN(SRAM_G_GAMMA48)
REGDEF_BIT(reg_g_lut_96,        10)
REGDEF_BIT(reg_g_lut_97,        10)
REGDEF_BIT(reg_g_lut_98,        10)
REGDEF_END(SRAM_G_GAMMA48)


/*
    reg_g_lut_98 :    [0x0, 0x3ff],          bits : 9_0
    reg_g_lut_99 :    [0x0, 0x3ff],          bits : 19_10
    reg_g_lut_100:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA49_OFS 0x05c4
REGDEF_BEGIN(SRAM_G_GAMMA49)
REGDEF_BIT(reg_g_lut_98,        10)
REGDEF_BIT(reg_g_lut_99,        10)
REGDEF_BIT(reg_g_lut_100,        10)
REGDEF_END(SRAM_G_GAMMA49)


/*
    reg_g_lut_100:    [0x0, 0x3ff],          bits : 9_0
    reg_g_lut_101:    [0x0, 0x3ff],          bits : 19_10
    reg_g_lut_102:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA50_OFS 0x05c8
REGDEF_BEGIN(SRAM_G_GAMMA50)
REGDEF_BIT(reg_g_lut_100,        10)
REGDEF_BIT(reg_g_lut_101,        10)
REGDEF_BIT(reg_g_lut_102,        10)
REGDEF_END(SRAM_G_GAMMA50)


/*
    reg_g_lut_102:    [0x0, 0x3ff],          bits : 9_0
    reg_g_lut_103:    [0x0, 0x3ff],          bits : 19_10
    reg_g_lut_104:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA51_OFS 0x05cc
REGDEF_BEGIN(SRAM_G_GAMMA51)
REGDEF_BIT(reg_g_lut_102,        10)
REGDEF_BIT(reg_g_lut_103,        10)
REGDEF_BIT(reg_g_lut_104,        10)
REGDEF_END(SRAM_G_GAMMA51)


/*
    reg_g_lut_104:    [0x0, 0x3ff],          bits : 9_0
    reg_g_lut_105:    [0x0, 0x3ff],          bits : 19_10
    reg_g_lut_106:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA52_OFS 0x05d0
REGDEF_BEGIN(SRAM_G_GAMMA52)
REGDEF_BIT(reg_g_lut_104,        10)
REGDEF_BIT(reg_g_lut_105,        10)
REGDEF_BIT(reg_g_lut_106,        10)
REGDEF_END(SRAM_G_GAMMA52)


/*
    reg_g_lut_106:    [0x0, 0x3ff],          bits : 9_0
    reg_g_lut_107:    [0x0, 0x3ff],          bits : 19_10
    reg_g_lut_108:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA53_OFS 0x05d4
REGDEF_BEGIN(SRAM_G_GAMMA53)
REGDEF_BIT(reg_g_lut_106,        10)
REGDEF_BIT(reg_g_lut_107,        10)
REGDEF_BIT(reg_g_lut_108,        10)
REGDEF_END(SRAM_G_GAMMA53)


/*
    reg_g_lut_108:    [0x0, 0x3ff],          bits : 9_0
    reg_g_lut_109:    [0x0, 0x3ff],          bits : 19_10
    reg_g_lut_110:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA54_OFS 0x05d8
REGDEF_BEGIN(SRAM_G_GAMMA54)
REGDEF_BIT(reg_g_lut_108,        10)
REGDEF_BIT(reg_g_lut_109,        10)
REGDEF_BIT(reg_g_lut_110,        10)
REGDEF_END(SRAM_G_GAMMA54)


/*
    reg_g_lut_110:    [0x0, 0x3ff],          bits : 9_0
    reg_g_lut_111:    [0x0, 0x3ff],          bits : 19_10
    reg_g_lut_112:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA55_OFS 0x05dc
REGDEF_BEGIN(SRAM_G_GAMMA55)
REGDEF_BIT(reg_g_lut_110,        10)
REGDEF_BIT(reg_g_lut_111,        10)
REGDEF_BIT(reg_g_lut_112,        10)
REGDEF_END(SRAM_G_GAMMA55)


/*
    reg_g_lut_112:    [0x0, 0x3ff],          bits : 9_0
    reg_g_lut_113:    [0x0, 0x3ff],          bits : 19_10
    reg_g_lut_114:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA56_OFS 0x05e0
REGDEF_BEGIN(SRAM_G_GAMMA56)
REGDEF_BIT(reg_g_lut_112,        10)
REGDEF_BIT(reg_g_lut_113,        10)
REGDEF_BIT(reg_g_lut_114,        10)
REGDEF_END(SRAM_G_GAMMA56)


/*
    reg_g_lut_114:    [0x0, 0x3ff],          bits : 9_0
    reg_g_lut_115:    [0x0, 0x3ff],          bits : 19_10
    reg_g_lut_116:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA57_OFS 0x05e4
REGDEF_BEGIN(SRAM_G_GAMMA57)
REGDEF_BIT(reg_g_lut_114,        10)
REGDEF_BIT(reg_g_lut_115,        10)
REGDEF_BIT(reg_g_lut_116,        10)
REGDEF_END(SRAM_G_GAMMA57)


/*
    reg_g_lut_116:    [0x0, 0x3ff],          bits : 9_0
    reg_g_lut_117:    [0x0, 0x3ff],          bits : 19_10
    reg_g_lut_118:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA58_OFS 0x05e8
REGDEF_BEGIN(SRAM_G_GAMMA58)
REGDEF_BIT(reg_g_lut_116,        10)
REGDEF_BIT(reg_g_lut_117,        10)
REGDEF_BIT(reg_g_lut_118,        10)
REGDEF_END(SRAM_G_GAMMA58)


/*
    reg_g_lut_118:    [0x0, 0x3ff],          bits : 9_0
    reg_g_lut_119:    [0x0, 0x3ff],          bits : 19_10
    reg_g_lut_120:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA59_OFS 0x05ec
REGDEF_BEGIN(SRAM_G_GAMMA59)
REGDEF_BIT(reg_g_lut_118,        10)
REGDEF_BIT(reg_g_lut_119,        10)
REGDEF_BIT(reg_g_lut_120,        10)
REGDEF_END(SRAM_G_GAMMA59)


/*
    reg_g_lut_120:    [0x0, 0x3ff],          bits : 9_0
    reg_g_lut_121:    [0x0, 0x3ff],          bits : 19_10
    reg_g_lut_122:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA60_OFS 0x05f0
REGDEF_BEGIN(SRAM_G_GAMMA60)
REGDEF_BIT(reg_g_lut_120,        10)
REGDEF_BIT(reg_g_lut_121,        10)
REGDEF_BIT(reg_g_lut_122,        10)
REGDEF_END(SRAM_G_GAMMA60)


/*
    reg_g_lut_122:    [0x0, 0x3ff],          bits : 9_0
    reg_g_lut_123:    [0x0, 0x3ff],          bits : 19_10
    reg_g_lut_124:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA61_OFS 0x05f4
REGDEF_BEGIN(SRAM_G_GAMMA61)
REGDEF_BIT(reg_g_lut_122,        10)
REGDEF_BIT(reg_g_lut_123,        10)
REGDEF_BIT(reg_g_lut_124,        10)
REGDEF_END(SRAM_G_GAMMA61)


/*
    reg_g_lut_124:    [0x0, 0x3ff],          bits : 9_0
    reg_g_lut_125:    [0x0, 0x3ff],          bits : 19_10
    reg_g_lut_126:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA62_OFS 0x05f8
REGDEF_BEGIN(SRAM_G_GAMMA62)
REGDEF_BIT(reg_g_lut_124,        10)
REGDEF_BIT(reg_g_lut_125,        10)
REGDEF_BIT(reg_g_lut_126,        10)
REGDEF_END(SRAM_G_GAMMA62)


/*
    reg_g_lut_126:    [0x0, 0x3ff],          bits : 9_0
    reg_g_lut_127:    [0x0, 0x3ff],          bits : 19_10
    reg_g_lut_128:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_G_GAMMA63_OFS 0x05fc
REGDEF_BEGIN(SRAM_G_GAMMA63)
REGDEF_BIT(reg_g_lut_126,        10)
REGDEF_BIT(reg_g_lut_127,        10)
REGDEF_BIT(reg_g_lut_128,        10)
REGDEF_END(SRAM_G_GAMMA63)


/*
    reg_b_lut_0:    [0x0, 0x3ff],            bits : 9_0
    reg_b_lut_1:    [0x0, 0x3ff],            bits : 19_10
    reg_b_lut_2:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_B_GAMMA0_OFS 0x0600
REGDEF_BEGIN(SRAM_B_GAMMA0)
REGDEF_BIT(reg_b_lut_0,        10)
REGDEF_BIT(reg_b_lut_1,        10)
REGDEF_BIT(reg_b_lut_2,        10)
REGDEF_END(SRAM_B_GAMMA0)


/*
    reg_b_lut_2:    [0x0, 0x3ff],            bits : 9_0
    reg_b_lut_3:    [0x0, 0x3ff],            bits : 19_10
    reg_b_lut_4:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_B_GAMMA1_OFS 0x0604
REGDEF_BEGIN(SRAM_B_GAMMA1)
REGDEF_BIT(reg_b_lut_2,        10)
REGDEF_BIT(reg_b_lut_3,        10)
REGDEF_BIT(reg_b_lut_4,        10)
REGDEF_END(SRAM_B_GAMMA1)


/*
    reg_b_lut_4:    [0x0, 0x3ff],            bits : 9_0
    reg_b_lut_5:    [0x0, 0x3ff],            bits : 19_10
    reg_b_lut_6:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_B_GAMMA2_OFS 0x0608
REGDEF_BEGIN(SRAM_B_GAMMA2)
REGDEF_BIT(reg_b_lut_4,        10)
REGDEF_BIT(reg_b_lut_5,        10)
REGDEF_BIT(reg_b_lut_6,        10)
REGDEF_END(SRAM_B_GAMMA2)


/*
    reg_b_lut_6:    [0x0, 0x3ff],            bits : 9_0
    reg_b_lut_7:    [0x0, 0x3ff],            bits : 19_10
    reg_b_lut_8:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_B_GAMMA3_OFS 0x060c
REGDEF_BEGIN(SRAM_B_GAMMA3)
REGDEF_BIT(reg_b_lut_6,        10)
REGDEF_BIT(reg_b_lut_7,        10)
REGDEF_BIT(reg_b_lut_8,        10)
REGDEF_END(SRAM_B_GAMMA3)


/*
    reg_b_lut_8 :    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_9 :    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_10:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA4_OFS 0x0610
REGDEF_BEGIN(SRAM_B_GAMMA4)
REGDEF_BIT(reg_b_lut_8,        10)
REGDEF_BIT(reg_b_lut_9,        10)
REGDEF_BIT(reg_b_lut_10,        10)
REGDEF_END(SRAM_B_GAMMA4)


/*
    reg_b_lut_10:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_11:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_12:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA5_OFS 0x0614
REGDEF_BEGIN(SRAM_B_GAMMA5)
REGDEF_BIT(reg_b_lut_10,        10)
REGDEF_BIT(reg_b_lut_11,        10)
REGDEF_BIT(reg_b_lut_12,        10)
REGDEF_END(SRAM_B_GAMMA5)


/*
    reg_b_lut_12:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_13:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_14:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA6_OFS 0x0618
REGDEF_BEGIN(SRAM_B_GAMMA6)
REGDEF_BIT(reg_b_lut_12,        10)
REGDEF_BIT(reg_b_lut_13,        10)
REGDEF_BIT(reg_b_lut_14,        10)
REGDEF_END(SRAM_B_GAMMA6)


/*
    reg_b_lut_14:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_15:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_16:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA7_OFS 0x061c
REGDEF_BEGIN(SRAM_B_GAMMA7)
REGDEF_BIT(reg_b_lut_14,        10)
REGDEF_BIT(reg_b_lut_15,        10)
REGDEF_BIT(reg_b_lut_16,        10)
REGDEF_END(SRAM_B_GAMMA7)


/*
    reg_b_lut_16:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_17:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_18:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA8_OFS 0x0620
REGDEF_BEGIN(SRAM_B_GAMMA8)
REGDEF_BIT(reg_b_lut_16,        10)
REGDEF_BIT(reg_b_lut_17,        10)
REGDEF_BIT(reg_b_lut_18,        10)
REGDEF_END(SRAM_B_GAMMA8)


/*
    reg_b_lut_18:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_19:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_20:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA9_OFS 0x0624
REGDEF_BEGIN(SRAM_B_GAMMA9)
REGDEF_BIT(reg_b_lut_18,        10)
REGDEF_BIT(reg_b_lut_19,        10)
REGDEF_BIT(reg_b_lut_20,        10)
REGDEF_END(SRAM_B_GAMMA9)


/*
    reg_b_lut_20:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_21:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_22:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA10_OFS 0x0628
REGDEF_BEGIN(SRAM_B_GAMMA10)
REGDEF_BIT(reg_b_lut_20,        10)
REGDEF_BIT(reg_b_lut_21,        10)
REGDEF_BIT(reg_b_lut_22,        10)
REGDEF_END(SRAM_B_GAMMA10)


/*
    reg_b_lut_22:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_23:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_24:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA11_OFS 0x062c
REGDEF_BEGIN(SRAM_B_GAMMA11)
REGDEF_BIT(reg_b_lut_22,        10)
REGDEF_BIT(reg_b_lut_23,        10)
REGDEF_BIT(reg_b_lut_24,        10)
REGDEF_END(SRAM_B_GAMMA11)


/*
    reg_b_lut_24:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_25:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_26:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA12_OFS 0x0630
REGDEF_BEGIN(SRAM_B_GAMMA12)
REGDEF_BIT(reg_b_lut_24,        10)
REGDEF_BIT(reg_b_lut_25,        10)
REGDEF_BIT(reg_b_lut_26,        10)
REGDEF_END(SRAM_B_GAMMA12)


/*
    reg_b_lut_26:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_27:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_28:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA13_OFS 0x0634
REGDEF_BEGIN(SRAM_B_GAMMA13)
REGDEF_BIT(reg_b_lut_26,        10)
REGDEF_BIT(reg_b_lut_27,        10)
REGDEF_BIT(reg_b_lut_28,        10)
REGDEF_END(SRAM_B_GAMMA13)


/*
    reg_b_lut_28:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_29:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_30:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA14_OFS 0x0638
REGDEF_BEGIN(SRAM_B_GAMMA14)
REGDEF_BIT(reg_b_lut_28,        10)
REGDEF_BIT(reg_b_lut_29,        10)
REGDEF_BIT(reg_b_lut_30,        10)
REGDEF_END(SRAM_B_GAMMA14)


/*
    reg_b_lut_30:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_31:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_32:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA15_OFS 0x063c
REGDEF_BEGIN(SRAM_B_GAMMA15)
REGDEF_BIT(reg_b_lut_30,        10)
REGDEF_BIT(reg_b_lut_31,        10)
REGDEF_BIT(reg_b_lut_32,        10)
REGDEF_END(SRAM_B_GAMMA15)


/*
    reg_b_lut_32:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_33:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_34:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA16_OFS 0x0640
REGDEF_BEGIN(SRAM_B_GAMMA16)
REGDEF_BIT(reg_b_lut_32,        10)
REGDEF_BIT(reg_b_lut_33,        10)
REGDEF_BIT(reg_b_lut_34,        10)
REGDEF_END(SRAM_B_GAMMA16)


/*
    reg_b_lut_34:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_35:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_36:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA17_OFS 0x0644
REGDEF_BEGIN(SRAM_B_GAMMA17)
REGDEF_BIT(reg_b_lut_34,        10)
REGDEF_BIT(reg_b_lut_35,        10)
REGDEF_BIT(reg_b_lut_36,        10)
REGDEF_END(SRAM_B_GAMMA17)


/*
    reg_b_lut_36:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_37:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_38:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA18_OFS 0x0648
REGDEF_BEGIN(SRAM_B_GAMMA18)
REGDEF_BIT(reg_b_lut_36,        10)
REGDEF_BIT(reg_b_lut_37,        10)
REGDEF_BIT(reg_b_lut_38,        10)
REGDEF_END(SRAM_B_GAMMA18)


/*
    reg_b_lut_38:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_39:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_40:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA19_OFS 0x064c
REGDEF_BEGIN(SRAM_B_GAMMA19)
REGDEF_BIT(reg_b_lut_38,        10)
REGDEF_BIT(reg_b_lut_39,        10)
REGDEF_BIT(reg_b_lut_40,        10)
REGDEF_END(SRAM_B_GAMMA19)


/*
    reg_b_lut_40:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_41:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_42:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA20_OFS 0x0650
REGDEF_BEGIN(SRAM_B_GAMMA20)
REGDEF_BIT(reg_b_lut_40,        10)
REGDEF_BIT(reg_b_lut_41,        10)
REGDEF_BIT(reg_b_lut_42,        10)
REGDEF_END(SRAM_B_GAMMA20)


/*
    reg_b_lut_42:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_43:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_44:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA21_OFS 0x0654
REGDEF_BEGIN(SRAM_B_GAMMA21)
REGDEF_BIT(reg_b_lut_42,        10)
REGDEF_BIT(reg_b_lut_43,        10)
REGDEF_BIT(reg_b_lut_44,        10)
REGDEF_END(SRAM_B_GAMMA21)


/*
    reg_b_lut_44:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_45:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_46:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA22_OFS 0x0658
REGDEF_BEGIN(SRAM_B_GAMMA22)
REGDEF_BIT(reg_b_lut_44,        10)
REGDEF_BIT(reg_b_lut_45,        10)
REGDEF_BIT(reg_b_lut_46,        10)
REGDEF_END(SRAM_B_GAMMA22)


/*
    reg_b_lut_46:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_47:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_48:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA23_OFS 0x065c
REGDEF_BEGIN(SRAM_B_GAMMA23)
REGDEF_BIT(reg_b_lut_46,        10)
REGDEF_BIT(reg_b_lut_47,        10)
REGDEF_BIT(reg_b_lut_48,        10)
REGDEF_END(SRAM_B_GAMMA23)


/*
    reg_b_lut_48:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_49:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_50:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA24_OFS 0x0660
REGDEF_BEGIN(SRAM_B_GAMMA24)
REGDEF_BIT(reg_b_lut_48,        10)
REGDEF_BIT(reg_b_lut_49,        10)
REGDEF_BIT(reg_b_lut_50,        10)
REGDEF_END(SRAM_B_GAMMA24)


/*
    reg_b_lut_50:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_51:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_52:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA25_OFS 0x0664
REGDEF_BEGIN(SRAM_B_GAMMA25)
REGDEF_BIT(reg_b_lut_50,        10)
REGDEF_BIT(reg_b_lut_51,        10)
REGDEF_BIT(reg_b_lut_52,        10)
REGDEF_END(SRAM_B_GAMMA25)


/*
    reg_b_lut_52:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_53:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_54:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA26_OFS 0x0668
REGDEF_BEGIN(SRAM_B_GAMMA26)
REGDEF_BIT(reg_b_lut_52,        10)
REGDEF_BIT(reg_b_lut_53,        10)
REGDEF_BIT(reg_b_lut_54,        10)
REGDEF_END(SRAM_B_GAMMA26)


/*
    reg_b_lut_54:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_55:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_56:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA27_OFS 0x066c
REGDEF_BEGIN(SRAM_B_GAMMA27)
REGDEF_BIT(reg_b_lut_54,        10)
REGDEF_BIT(reg_b_lut_55,        10)
REGDEF_BIT(reg_b_lut_56,        10)
REGDEF_END(SRAM_B_GAMMA27)


/*
    reg_b_lut_56:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_57:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_58:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA28_OFS 0x0670
REGDEF_BEGIN(SRAM_B_GAMMA28)
REGDEF_BIT(reg_b_lut_56,        10)
REGDEF_BIT(reg_b_lut_57,        10)
REGDEF_BIT(reg_b_lut_58,        10)
REGDEF_END(SRAM_B_GAMMA28)


/*
    reg_b_lut_58:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_59:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_60:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA29_OFS 0x0674
REGDEF_BEGIN(SRAM_B_GAMMA29)
REGDEF_BIT(reg_b_lut_58,        10)
REGDEF_BIT(reg_b_lut_59,        10)
REGDEF_BIT(reg_b_lut_60,        10)
REGDEF_END(SRAM_B_GAMMA29)


/*
    reg_b_lut_60:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_61:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_62:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA30_OFS 0x0678
REGDEF_BEGIN(SRAM_B_GAMMA30)
REGDEF_BIT(reg_b_lut_60,        10)
REGDEF_BIT(reg_b_lut_61,        10)
REGDEF_BIT(reg_b_lut_62,        10)
REGDEF_END(SRAM_B_GAMMA30)


/*
    reg_b_lut_62:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_63:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_64:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA31_OFS 0x067c
REGDEF_BEGIN(SRAM_B_GAMMA31)
REGDEF_BIT(reg_b_lut_62,        10)
REGDEF_BIT(reg_b_lut_63,        10)
REGDEF_BIT(reg_b_lut_64,        10)
REGDEF_END(SRAM_B_GAMMA31)


/*
    reg_b_lut_64:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_65:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_66:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA32_OFS 0x0680
REGDEF_BEGIN(SRAM_B_GAMMA32)
REGDEF_BIT(reg_b_lut_64,        10)
REGDEF_BIT(reg_b_lut_65,        10)
REGDEF_BIT(reg_b_lut_66,        10)
REGDEF_END(SRAM_B_GAMMA32)


/*
    reg_b_lut_66:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_67:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_68:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA33_OFS 0x0684
REGDEF_BEGIN(SRAM_B_GAMMA33)
REGDEF_BIT(reg_b_lut_66,        10)
REGDEF_BIT(reg_b_lut_67,        10)
REGDEF_BIT(reg_b_lut_68,        10)
REGDEF_END(SRAM_B_GAMMA33)


/*
    reg_b_lut_68:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_69:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_70:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA34_OFS 0x0688
REGDEF_BEGIN(SRAM_B_GAMMA34)
REGDEF_BIT(reg_b_lut_68,        10)
REGDEF_BIT(reg_b_lut_69,        10)
REGDEF_BIT(reg_b_lut_70,        10)
REGDEF_END(SRAM_B_GAMMA34)


/*
    reg_b_lut_70:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_71:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_72:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA35_OFS 0x068c
REGDEF_BEGIN(SRAM_B_GAMMA35)
REGDEF_BIT(reg_b_lut_70,        10)
REGDEF_BIT(reg_b_lut_71,        10)
REGDEF_BIT(reg_b_lut_72,        10)
REGDEF_END(SRAM_B_GAMMA35)


/*
    reg_b_lut_72:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_73:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_74:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA36_OFS 0x0690
REGDEF_BEGIN(SRAM_B_GAMMA36)
REGDEF_BIT(reg_b_lut_72,        10)
REGDEF_BIT(reg_b_lut_73,        10)
REGDEF_BIT(reg_b_lut_74,        10)
REGDEF_END(SRAM_B_GAMMA36)


/*
    reg_b_lut_74:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_75:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_76:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA37_OFS 0x0694
REGDEF_BEGIN(SRAM_B_GAMMA37)
REGDEF_BIT(reg_b_lut_74,        10)
REGDEF_BIT(reg_b_lut_75,        10)
REGDEF_BIT(reg_b_lut_76,        10)
REGDEF_END(SRAM_B_GAMMA37)


/*
    reg_b_lut_76:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_77:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_78:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA38_OFS 0x0698
REGDEF_BEGIN(SRAM_B_GAMMA38)
REGDEF_BIT(reg_b_lut_76,        10)
REGDEF_BIT(reg_b_lut_77,        10)
REGDEF_BIT(reg_b_lut_78,        10)
REGDEF_END(SRAM_B_GAMMA38)


/*
    reg_b_lut_78:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_79:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_80:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA39_OFS 0x069c
REGDEF_BEGIN(SRAM_B_GAMMA39)
REGDEF_BIT(reg_b_lut_78,        10)
REGDEF_BIT(reg_b_lut_79,        10)
REGDEF_BIT(reg_b_lut_80,        10)
REGDEF_END(SRAM_B_GAMMA39)


/*
    reg_b_lut_80:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_81:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_82:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA40_OFS 0x06a0
REGDEF_BEGIN(SRAM_B_GAMMA40)
REGDEF_BIT(reg_b_lut_80,        10)
REGDEF_BIT(reg_b_lut_81,        10)
REGDEF_BIT(reg_b_lut_82,        10)
REGDEF_END(SRAM_B_GAMMA40)


/*
    reg_b_lut_82:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_83:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_84:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA41_OFS 0x06a4
REGDEF_BEGIN(SRAM_B_GAMMA41)
REGDEF_BIT(reg_b_lut_82,        10)
REGDEF_BIT(reg_b_lut_83,        10)
REGDEF_BIT(reg_b_lut_84,        10)
REGDEF_END(SRAM_B_GAMMA41)


/*
    reg_b_lut_84:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_85:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_86:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA42_OFS 0x06a8
REGDEF_BEGIN(SRAM_B_GAMMA42)
REGDEF_BIT(reg_b_lut_84,        10)
REGDEF_BIT(reg_b_lut_85,        10)
REGDEF_BIT(reg_b_lut_86,        10)
REGDEF_END(SRAM_B_GAMMA42)


/*
    reg_b_lut_86:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_87:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_88:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA43_OFS 0x06ac
REGDEF_BEGIN(SRAM_B_GAMMA43)
REGDEF_BIT(reg_b_lut_86,        10)
REGDEF_BIT(reg_b_lut_87,        10)
REGDEF_BIT(reg_b_lut_88,        10)
REGDEF_END(SRAM_B_GAMMA43)


/*
    reg_b_lut_88:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_89:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_90:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA44_OFS 0x06b0
REGDEF_BEGIN(SRAM_B_GAMMA44)
REGDEF_BIT(reg_b_lut_88,        10)
REGDEF_BIT(reg_b_lut_89,        10)
REGDEF_BIT(reg_b_lut_90,        10)
REGDEF_END(SRAM_B_GAMMA44)


/*
    reg_b_lut_90:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_91:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_92:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA45_OFS 0x06b4
REGDEF_BEGIN(SRAM_B_GAMMA45)
REGDEF_BIT(reg_b_lut_90,        10)
REGDEF_BIT(reg_b_lut_91,        10)
REGDEF_BIT(reg_b_lut_92,        10)
REGDEF_END(SRAM_B_GAMMA45)


/*
    reg_b_lut_92:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_93:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_94:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA46_OFS 0x06b8
REGDEF_BEGIN(SRAM_B_GAMMA46)
REGDEF_BIT(reg_b_lut_92,        10)
REGDEF_BIT(reg_b_lut_93,        10)
REGDEF_BIT(reg_b_lut_94,        10)
REGDEF_END(SRAM_B_GAMMA46)


/*
    reg_b_lut_94:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_95:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_96:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA47_OFS 0x06bc
REGDEF_BEGIN(SRAM_B_GAMMA47)
REGDEF_BIT(reg_b_lut_94,        10)
REGDEF_BIT(reg_b_lut_95,        10)
REGDEF_BIT(reg_b_lut_96,        10)
REGDEF_END(SRAM_B_GAMMA47)


/*
    reg_b_lut_96:    [0x0, 0x3ff],           bits : 9_0
    reg_b_lut_97:    [0x0, 0x3ff],           bits : 19_10
    reg_b_lut_98:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_B_GAMMA48_OFS 0x06c0
REGDEF_BEGIN(SRAM_B_GAMMA48)
REGDEF_BIT(reg_b_lut_96,        10)
REGDEF_BIT(reg_b_lut_97,        10)
REGDEF_BIT(reg_b_lut_98,        10)
REGDEF_END(SRAM_B_GAMMA48)


/*
    reg_b_lut_98 :    [0x0, 0x3ff],          bits : 9_0
    reg_b_lut_99 :    [0x0, 0x3ff],          bits : 19_10
    reg_b_lut_100:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA49_OFS 0x06c4
REGDEF_BEGIN(SRAM_B_GAMMA49)
REGDEF_BIT(reg_b_lut_98,        10)
REGDEF_BIT(reg_b_lut_99,        10)
REGDEF_BIT(reg_b_lut_100,        10)
REGDEF_END(SRAM_B_GAMMA49)


/*
    reg_b_lut_100:    [0x0, 0x3ff],          bits : 9_0
    reg_b_lut_101:    [0x0, 0x3ff],          bits : 19_10
    reg_b_lut_102:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA50_OFS 0x06c8
REGDEF_BEGIN(SRAM_B_GAMMA50)
REGDEF_BIT(reg_b_lut_100,        10)
REGDEF_BIT(reg_b_lut_101,        10)
REGDEF_BIT(reg_b_lut_102,        10)
REGDEF_END(SRAM_B_GAMMA50)


/*
    reg_b_lut_102:    [0x0, 0x3ff],          bits : 9_0
    reg_b_lut_103:    [0x0, 0x3ff],          bits : 19_10
    reg_b_lut_104:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA51_OFS 0x06cc
REGDEF_BEGIN(SRAM_B_GAMMA51)
REGDEF_BIT(reg_b_lut_102,        10)
REGDEF_BIT(reg_b_lut_103,        10)
REGDEF_BIT(reg_b_lut_104,        10)
REGDEF_END(SRAM_B_GAMMA51)


/*
    reg_b_lut_104:    [0x0, 0x3ff],          bits : 9_0
    reg_b_lut_105:    [0x0, 0x3ff],          bits : 19_10
    reg_b_lut_106:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA52_OFS 0x06d0
REGDEF_BEGIN(SRAM_B_GAMMA52)
REGDEF_BIT(reg_b_lut_104,        10)
REGDEF_BIT(reg_b_lut_105,        10)
REGDEF_BIT(reg_b_lut_106,        10)
REGDEF_END(SRAM_B_GAMMA52)


/*
    reg_b_lut_106:    [0x0, 0x3ff],          bits : 9_0
    reg_b_lut_107:    [0x0, 0x3ff],          bits : 19_10
    reg_b_lut_108:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA53_OFS 0x06d4
REGDEF_BEGIN(SRAM_B_GAMMA53)
REGDEF_BIT(reg_b_lut_106,        10)
REGDEF_BIT(reg_b_lut_107,        10)
REGDEF_BIT(reg_b_lut_108,        10)
REGDEF_END(SRAM_B_GAMMA53)


/*
    reg_b_lut_108:    [0x0, 0x3ff],          bits : 9_0
    reg_b_lut_109:    [0x0, 0x3ff],          bits : 19_10
    reg_b_lut_110:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA54_OFS 0x06d8
REGDEF_BEGIN(SRAM_B_GAMMA54)
REGDEF_BIT(reg_b_lut_108,        10)
REGDEF_BIT(reg_b_lut_109,        10)
REGDEF_BIT(reg_b_lut_110,        10)
REGDEF_END(SRAM_B_GAMMA54)


/*
    reg_b_lut_110:    [0x0, 0x3ff],          bits : 9_0
    reg_b_lut_111:    [0x0, 0x3ff],          bits : 19_10
    reg_b_lut_112:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA55_OFS 0x06dc
REGDEF_BEGIN(SRAM_B_GAMMA55)
REGDEF_BIT(reg_b_lut_110,        10)
REGDEF_BIT(reg_b_lut_111,        10)
REGDEF_BIT(reg_b_lut_112,        10)
REGDEF_END(SRAM_B_GAMMA55)


/*
    reg_b_lut_112:    [0x0, 0x3ff],          bits : 9_0
    reg_b_lut_113:    [0x0, 0x3ff],          bits : 19_10
    reg_b_lut_114:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA56_OFS 0x06e0
REGDEF_BEGIN(SRAM_B_GAMMA56)
REGDEF_BIT(reg_b_lut_112,        10)
REGDEF_BIT(reg_b_lut_113,        10)
REGDEF_BIT(reg_b_lut_114,        10)
REGDEF_END(SRAM_B_GAMMA56)


/*
    reg_b_lut_114:    [0x0, 0x3ff],          bits : 9_0
    reg_b_lut_115:    [0x0, 0x3ff],          bits : 19_10
    reg_b_lut_116:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA57_OFS 0x06e4
REGDEF_BEGIN(SRAM_B_GAMMA57)
REGDEF_BIT(reg_b_lut_114,        10)
REGDEF_BIT(reg_b_lut_115,        10)
REGDEF_BIT(reg_b_lut_116,        10)
REGDEF_END(SRAM_B_GAMMA57)


/*
    reg_b_lut_116:    [0x0, 0x3ff],          bits : 9_0
    reg_b_lut_117:    [0x0, 0x3ff],          bits : 19_10
    reg_b_lut_118:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA58_OFS 0x06e8
REGDEF_BEGIN(SRAM_B_GAMMA58)
REGDEF_BIT(reg_b_lut_116,        10)
REGDEF_BIT(reg_b_lut_117,        10)
REGDEF_BIT(reg_b_lut_118,        10)
REGDEF_END(SRAM_B_GAMMA58)


/*
    reg_b_lut_118:    [0x0, 0x3ff],          bits : 9_0
    reg_b_lut_119:    [0x0, 0x3ff],          bits : 19_10
    reg_b_lut_120:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA59_OFS 0x06ec
REGDEF_BEGIN(SRAM_B_GAMMA59)
REGDEF_BIT(reg_b_lut_118,        10)
REGDEF_BIT(reg_b_lut_119,        10)
REGDEF_BIT(reg_b_lut_120,        10)
REGDEF_END(SRAM_B_GAMMA59)


/*
    reg_b_lut_120:    [0x0, 0x3ff],          bits : 9_0
    reg_b_lut_121:    [0x0, 0x3ff],          bits : 19_10
    reg_b_lut_122:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA60_OFS 0x06f0
REGDEF_BEGIN(SRAM_B_GAMMA60)
REGDEF_BIT(reg_b_lut_120,        10)
REGDEF_BIT(reg_b_lut_121,        10)
REGDEF_BIT(reg_b_lut_122,        10)
REGDEF_END(SRAM_B_GAMMA60)


/*
    reg_b_lut_122:    [0x0, 0x3ff],          bits : 9_0
    reg_b_lut_123:    [0x0, 0x3ff],          bits : 19_10
    reg_b_lut_124:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA61_OFS 0x06f4
REGDEF_BEGIN(SRAM_B_GAMMA61)
REGDEF_BIT(reg_b_lut_122,        10)
REGDEF_BIT(reg_b_lut_123,        10)
REGDEF_BIT(reg_b_lut_124,        10)
REGDEF_END(SRAM_B_GAMMA61)


/*
    reg_b_lut_124:    [0x0, 0x3ff],          bits : 9_0
    reg_b_lut_125:    [0x0, 0x3ff],          bits : 19_10
    reg_b_lut_126:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA62_OFS 0x06f8
REGDEF_BEGIN(SRAM_B_GAMMA62)
REGDEF_BIT(reg_b_lut_124,        10)
REGDEF_BIT(reg_b_lut_125,        10)
REGDEF_BIT(reg_b_lut_126,        10)
REGDEF_END(SRAM_B_GAMMA62)


/*
    reg_b_lut_126:    [0x0, 0x3ff],          bits : 9_0
    reg_b_lut_127:    [0x0, 0x3ff],          bits : 19_10
    reg_b_lut_128:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_B_GAMMA63_OFS 0x06fc
REGDEF_BEGIN(SRAM_B_GAMMA63)
REGDEF_BIT(reg_b_lut_126,        10)
REGDEF_BIT(reg_b_lut_127,        10)
REGDEF_BIT(reg_b_lut_128,        10)
REGDEF_END(SRAM_B_GAMMA63)


/*
    reg_y_lut_0:    [0x0, 0x3ff],            bits : 9_0
    reg_y_lut_1:    [0x0, 0x3ff],            bits : 19_10
    reg_y_lut_2:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_Y_CURVE0_OFS 0x0700
REGDEF_BEGIN(SRAM_Y_CURVE0)
REGDEF_BIT(reg_y_lut_0,        10)
REGDEF_BIT(reg_y_lut_1,        10)
REGDEF_BIT(reg_y_lut_2,        10)
REGDEF_END(SRAM_Y_CURVE0)


/*
    reg_y_lut_2:    [0x0, 0x3ff],            bits : 9_0
    reg_y_lut_3:    [0x0, 0x3ff],            bits : 19_10
    reg_y_lut_4:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_Y_CURVE1_OFS 0x0704
REGDEF_BEGIN(SRAM_Y_CURVE1)
REGDEF_BIT(reg_y_lut_2,        10)
REGDEF_BIT(reg_y_lut_3,        10)
REGDEF_BIT(reg_y_lut_4,        10)
REGDEF_END(SRAM_Y_CURVE1)


/*
    reg_y_lut_4:    [0x0, 0x3ff],            bits : 9_0
    reg_y_lut_5:    [0x0, 0x3ff],            bits : 19_10
    reg_y_lut_6:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_Y_CURVE2_OFS 0x0708
REGDEF_BEGIN(SRAM_Y_CURVE2)
REGDEF_BIT(reg_y_lut_4,        10)
REGDEF_BIT(reg_y_lut_5,        10)
REGDEF_BIT(reg_y_lut_6,        10)
REGDEF_END(SRAM_Y_CURVE2)


/*
    reg_y_lut_6:    [0x0, 0x3ff],            bits : 9_0
    reg_y_lut_7:    [0x0, 0x3ff],            bits : 19_10
    reg_y_lut_8:    [0x0, 0x3ff],            bits : 29_20
*/
#define SRAM_Y_CURVE3_OFS 0x070c
REGDEF_BEGIN(SRAM_Y_CURVE3)
REGDEF_BIT(reg_y_lut_6,        10)
REGDEF_BIT(reg_y_lut_7,        10)
REGDEF_BIT(reg_y_lut_8,        10)
REGDEF_END(SRAM_Y_CURVE3)


/*
    reg_y_lut_8 :    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_9 :    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_10:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE4_OFS 0x0710
REGDEF_BEGIN(SRAM_Y_CURVE4)
REGDEF_BIT(reg_y_lut_8,        10)
REGDEF_BIT(reg_y_lut_9,        10)
REGDEF_BIT(reg_y_lut_10,        10)
REGDEF_END(SRAM_Y_CURVE4)


/*
    reg_y_lut_10:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_11:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_12:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE5_OFS 0x0714
REGDEF_BEGIN(SRAM_Y_CURVE5)
REGDEF_BIT(reg_y_lut_10,        10)
REGDEF_BIT(reg_y_lut_11,        10)
REGDEF_BIT(reg_y_lut_12,        10)
REGDEF_END(SRAM_Y_CURVE5)


/*
    reg_y_lut_12:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_13:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_14:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE6_OFS 0x0718
REGDEF_BEGIN(SRAM_Y_CURVE6)
REGDEF_BIT(reg_y_lut_12,        10)
REGDEF_BIT(reg_y_lut_13,        10)
REGDEF_BIT(reg_y_lut_14,        10)
REGDEF_END(SRAM_Y_CURVE6)


/*
    reg_y_lut_14:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_15:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_16:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE7_OFS 0x071c
REGDEF_BEGIN(SRAM_Y_CURVE7)
REGDEF_BIT(reg_y_lut_14,        10)
REGDEF_BIT(reg_y_lut_15,        10)
REGDEF_BIT(reg_y_lut_16,        10)
REGDEF_END(SRAM_Y_CURVE7)


/*
    reg_y_lut_16:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_17:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_18:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE8_OFS 0x0720
REGDEF_BEGIN(SRAM_Y_CURVE8)
REGDEF_BIT(reg_y_lut_16,        10)
REGDEF_BIT(reg_y_lut_17,        10)
REGDEF_BIT(reg_y_lut_18,        10)
REGDEF_END(SRAM_Y_CURVE8)


/*
    reg_y_lut_18:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_19:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_20:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE9_OFS 0x0724
REGDEF_BEGIN(SRAM_Y_CURVE9)
REGDEF_BIT(reg_y_lut_18,        10)
REGDEF_BIT(reg_y_lut_19,        10)
REGDEF_BIT(reg_y_lut_20,        10)
REGDEF_END(SRAM_Y_CURVE9)


/*
    reg_y_lut_20:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_21:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_22:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE10_OFS 0x0728
REGDEF_BEGIN(SRAM_Y_CURVE10)
REGDEF_BIT(reg_y_lut_20,        10)
REGDEF_BIT(reg_y_lut_21,        10)
REGDEF_BIT(reg_y_lut_22,        10)
REGDEF_END(SRAM_Y_CURVE10)


/*
    reg_y_lut_22:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_23:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_24:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE11_OFS 0x072c
REGDEF_BEGIN(SRAM_Y_CURVE11)
REGDEF_BIT(reg_y_lut_22,        10)
REGDEF_BIT(reg_y_lut_23,        10)
REGDEF_BIT(reg_y_lut_24,        10)
REGDEF_END(SRAM_Y_CURVE11)


/*
    reg_y_lut_24:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_25:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_26:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE12_OFS 0x0730
REGDEF_BEGIN(SRAM_Y_CURVE12)
REGDEF_BIT(reg_y_lut_24,        10)
REGDEF_BIT(reg_y_lut_25,        10)
REGDEF_BIT(reg_y_lut_26,        10)
REGDEF_END(SRAM_Y_CURVE12)


/*
    reg_y_lut_26:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_27:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_28:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE13_OFS 0x0734
REGDEF_BEGIN(SRAM_Y_CURVE13)
REGDEF_BIT(reg_y_lut_26,        10)
REGDEF_BIT(reg_y_lut_27,        10)
REGDEF_BIT(reg_y_lut_28,        10)
REGDEF_END(SRAM_Y_CURVE13)


/*
    reg_y_lut_28:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_29:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_30:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE14_OFS 0x0738
REGDEF_BEGIN(SRAM_Y_CURVE14)
REGDEF_BIT(reg_y_lut_28,        10)
REGDEF_BIT(reg_y_lut_29,        10)
REGDEF_BIT(reg_y_lut_30,        10)
REGDEF_END(SRAM_Y_CURVE14)


/*
    reg_y_lut_30:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_31:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_32:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE15_OFS 0x073c
REGDEF_BEGIN(SRAM_Y_CURVE15)
REGDEF_BIT(reg_y_lut_30,        10)
REGDEF_BIT(reg_y_lut_31,        10)
REGDEF_BIT(reg_y_lut_32,        10)
REGDEF_END(SRAM_Y_CURVE15)


/*
    reg_y_lut_32:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_33:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_34:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE16_OFS 0x0740
REGDEF_BEGIN(SRAM_Y_CURVE16)
REGDEF_BIT(reg_y_lut_32,        10)
REGDEF_BIT(reg_y_lut_33,        10)
REGDEF_BIT(reg_y_lut_34,        10)
REGDEF_END(SRAM_Y_CURVE16)


/*
    reg_y_lut_34:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_35:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_36:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE17_OFS 0x0744
REGDEF_BEGIN(SRAM_Y_CURVE17)
REGDEF_BIT(reg_y_lut_34,        10)
REGDEF_BIT(reg_y_lut_35,        10)
REGDEF_BIT(reg_y_lut_36,        10)
REGDEF_END(SRAM_Y_CURVE17)


/*
    reg_y_lut_36:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_37:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_38:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE18_OFS 0x0748
REGDEF_BEGIN(SRAM_Y_CURVE18)
REGDEF_BIT(reg_y_lut_36,        10)
REGDEF_BIT(reg_y_lut_37,        10)
REGDEF_BIT(reg_y_lut_38,        10)
REGDEF_END(SRAM_Y_CURVE18)


/*
    reg_y_lut_38:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_39:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_40:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE19_OFS 0x074c
REGDEF_BEGIN(SRAM_Y_CURVE19)
REGDEF_BIT(reg_y_lut_38,        10)
REGDEF_BIT(reg_y_lut_39,        10)
REGDEF_BIT(reg_y_lut_40,        10)
REGDEF_END(SRAM_Y_CURVE19)


/*
    reg_y_lut_40:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_41:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_42:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE20_OFS 0x0750
REGDEF_BEGIN(SRAM_Y_CURVE20)
REGDEF_BIT(reg_y_lut_40,        10)
REGDEF_BIT(reg_y_lut_41,        10)
REGDEF_BIT(reg_y_lut_42,        10)
REGDEF_END(SRAM_Y_CURVE20)


/*
    reg_y_lut_42:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_43:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_44:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE21_OFS 0x0754
REGDEF_BEGIN(SRAM_Y_CURVE21)
REGDEF_BIT(reg_y_lut_42,        10)
REGDEF_BIT(reg_y_lut_43,        10)
REGDEF_BIT(reg_y_lut_44,        10)
REGDEF_END(SRAM_Y_CURVE21)


/*
    reg_y_lut_44:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_45:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_46:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE22_OFS 0x0758
REGDEF_BEGIN(SRAM_Y_CURVE22)
REGDEF_BIT(reg_y_lut_44,        10)
REGDEF_BIT(reg_y_lut_45,        10)
REGDEF_BIT(reg_y_lut_46,        10)
REGDEF_END(SRAM_Y_CURVE22)


/*
    reg_y_lut_46:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_47:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_48:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE23_OFS 0x075c
REGDEF_BEGIN(SRAM_Y_CURVE23)
REGDEF_BIT(reg_y_lut_46,        10)
REGDEF_BIT(reg_y_lut_47,        10)
REGDEF_BIT(reg_y_lut_48,        10)
REGDEF_END(SRAM_Y_CURVE23)


/*
    reg_y_lut_48:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_49:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_50:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE24_OFS 0x0760
REGDEF_BEGIN(SRAM_Y_CURVE24)
REGDEF_BIT(reg_y_lut_48,        10)
REGDEF_BIT(reg_y_lut_49,        10)
REGDEF_BIT(reg_y_lut_50,        10)
REGDEF_END(SRAM_Y_CURVE24)


/*
    reg_y_lut_50:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_51:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_52:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE25_OFS 0x0764
REGDEF_BEGIN(SRAM_Y_CURVE25)
REGDEF_BIT(reg_y_lut_50,        10)
REGDEF_BIT(reg_y_lut_51,        10)
REGDEF_BIT(reg_y_lut_52,        10)
REGDEF_END(SRAM_Y_CURVE25)


/*
    reg_y_lut_52:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_53:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_54:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE26_OFS 0x0768
REGDEF_BEGIN(SRAM_Y_CURVE26)
REGDEF_BIT(reg_y_lut_52,        10)
REGDEF_BIT(reg_y_lut_53,        10)
REGDEF_BIT(reg_y_lut_54,        10)
REGDEF_END(SRAM_Y_CURVE26)


/*
    reg_y_lut_54:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_55:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_56:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE27_OFS 0x076c
REGDEF_BEGIN(SRAM_Y_CURVE27)
REGDEF_BIT(reg_y_lut_54,        10)
REGDEF_BIT(reg_y_lut_55,        10)
REGDEF_BIT(reg_y_lut_56,        10)
REGDEF_END(SRAM_Y_CURVE27)


/*
    reg_y_lut_56:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_57:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_58:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE28_OFS 0x0770
REGDEF_BEGIN(SRAM_Y_CURVE28)
REGDEF_BIT(reg_y_lut_56,        10)
REGDEF_BIT(reg_y_lut_57,        10)
REGDEF_BIT(reg_y_lut_58,        10)
REGDEF_END(SRAM_Y_CURVE28)


/*
    reg_y_lut_58:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_59:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_60:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE29_OFS 0x0774
REGDEF_BEGIN(SRAM_Y_CURVE29)
REGDEF_BIT(reg_y_lut_58,        10)
REGDEF_BIT(reg_y_lut_59,        10)
REGDEF_BIT(reg_y_lut_60,        10)
REGDEF_END(SRAM_Y_CURVE29)


/*
    reg_y_lut_60:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_61:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_62:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE30_OFS 0x0778
REGDEF_BEGIN(SRAM_Y_CURVE30)
REGDEF_BIT(reg_y_lut_60,        10)
REGDEF_BIT(reg_y_lut_61,        10)
REGDEF_BIT(reg_y_lut_62,        10)
REGDEF_END(SRAM_Y_CURVE30)


/*
    reg_y_lut_62:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_63:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_64:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE31_OFS 0x077c
REGDEF_BEGIN(SRAM_Y_CURVE31)
REGDEF_BIT(reg_y_lut_62,        10)
REGDEF_BIT(reg_y_lut_63,        10)
REGDEF_BIT(reg_y_lut_64,        10)
REGDEF_END(SRAM_Y_CURVE31)


/*
    reg_y_lut_64:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_65:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_66:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE32_OFS 0x0780
REGDEF_BEGIN(SRAM_Y_CURVE32)
REGDEF_BIT(reg_y_lut_64,        10)
REGDEF_BIT(reg_y_lut_65,        10)
REGDEF_BIT(reg_y_lut_66,        10)
REGDEF_END(SRAM_Y_CURVE32)


/*
    reg_y_lut_66:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_67:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_68:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE33_OFS 0x0784
REGDEF_BEGIN(SRAM_Y_CURVE33)
REGDEF_BIT(reg_y_lut_66,        10)
REGDEF_BIT(reg_y_lut_67,        10)
REGDEF_BIT(reg_y_lut_68,        10)
REGDEF_END(SRAM_Y_CURVE33)


/*
    reg_y_lut_68:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_69:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_70:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE34_OFS 0x0788
REGDEF_BEGIN(SRAM_Y_CURVE34)
REGDEF_BIT(reg_y_lut_68,        10)
REGDEF_BIT(reg_y_lut_69,        10)
REGDEF_BIT(reg_y_lut_70,        10)
REGDEF_END(SRAM_Y_CURVE34)


/*
    reg_y_lut_70:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_71:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_72:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE35_OFS 0x078c
REGDEF_BEGIN(SRAM_Y_CURVE35)
REGDEF_BIT(reg_y_lut_70,        10)
REGDEF_BIT(reg_y_lut_71,        10)
REGDEF_BIT(reg_y_lut_72,        10)
REGDEF_END(SRAM_Y_CURVE35)


/*
    reg_y_lut_72:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_73:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_74:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE36_OFS 0x0790
REGDEF_BEGIN(SRAM_Y_CURVE36)
REGDEF_BIT(reg_y_lut_72,        10)
REGDEF_BIT(reg_y_lut_73,        10)
REGDEF_BIT(reg_y_lut_74,        10)
REGDEF_END(SRAM_Y_CURVE36)


/*
    reg_y_lut_74:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_75:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_76:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE37_OFS 0x0794
REGDEF_BEGIN(SRAM_Y_CURVE37)
REGDEF_BIT(reg_y_lut_74,        10)
REGDEF_BIT(reg_y_lut_75,        10)
REGDEF_BIT(reg_y_lut_76,        10)
REGDEF_END(SRAM_Y_CURVE37)


/*
    reg_y_lut_76:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_77:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_78:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE38_OFS 0x0798
REGDEF_BEGIN(SRAM_Y_CURVE38)
REGDEF_BIT(reg_y_lut_76,        10)
REGDEF_BIT(reg_y_lut_77,        10)
REGDEF_BIT(reg_y_lut_78,        10)
REGDEF_END(SRAM_Y_CURVE38)


/*
    reg_y_lut_78:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_79:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_80:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE39_OFS 0x079c
REGDEF_BEGIN(SRAM_Y_CURVE39)
REGDEF_BIT(reg_y_lut_78,        10)
REGDEF_BIT(reg_y_lut_79,        10)
REGDEF_BIT(reg_y_lut_80,        10)
REGDEF_END(SRAM_Y_CURVE39)


/*
    reg_y_lut_80:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_81:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_82:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE40_OFS 0x07a0
REGDEF_BEGIN(SRAM_Y_CURVE40)
REGDEF_BIT(reg_y_lut_80,        10)
REGDEF_BIT(reg_y_lut_81,        10)
REGDEF_BIT(reg_y_lut_82,        10)
REGDEF_END(SRAM_Y_CURVE40)


/*
    reg_y_lut_82:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_83:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_84:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE41_OFS 0x07a4
REGDEF_BEGIN(SRAM_Y_CURVE41)
REGDEF_BIT(reg_y_lut_82,        10)
REGDEF_BIT(reg_y_lut_83,        10)
REGDEF_BIT(reg_y_lut_84,        10)
REGDEF_END(SRAM_Y_CURVE41)


/*
    reg_y_lut_84:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_85:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_86:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE42_OFS 0x07a8
REGDEF_BEGIN(SRAM_Y_CURVE42)
REGDEF_BIT(reg_y_lut_84,        10)
REGDEF_BIT(reg_y_lut_85,        10)
REGDEF_BIT(reg_y_lut_86,        10)
REGDEF_END(SRAM_Y_CURVE42)


/*
    reg_y_lut_86:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_87:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_88:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE43_OFS 0x07ac
REGDEF_BEGIN(SRAM_Y_CURVE43)
REGDEF_BIT(reg_y_lut_86,        10)
REGDEF_BIT(reg_y_lut_87,        10)
REGDEF_BIT(reg_y_lut_88,        10)
REGDEF_END(SRAM_Y_CURVE43)


/*
    reg_y_lut_88:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_89:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_90:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE44_OFS 0x07b0
REGDEF_BEGIN(SRAM_Y_CURVE44)
REGDEF_BIT(reg_y_lut_88,        10)
REGDEF_BIT(reg_y_lut_89,        10)
REGDEF_BIT(reg_y_lut_90,        10)
REGDEF_END(SRAM_Y_CURVE44)


/*
    reg_y_lut_90:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_91:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_92:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE45_OFS 0x07b4
REGDEF_BEGIN(SRAM_Y_CURVE45)
REGDEF_BIT(reg_y_lut_90,        10)
REGDEF_BIT(reg_y_lut_91,        10)
REGDEF_BIT(reg_y_lut_92,        10)
REGDEF_END(SRAM_Y_CURVE45)


/*
    reg_y_lut_92:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_93:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_94:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE46_OFS 0x07b8
REGDEF_BEGIN(SRAM_Y_CURVE46)
REGDEF_BIT(reg_y_lut_92,        10)
REGDEF_BIT(reg_y_lut_93,        10)
REGDEF_BIT(reg_y_lut_94,        10)
REGDEF_END(SRAM_Y_CURVE46)


/*
    reg_y_lut_94:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_95:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_96:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE47_OFS 0x07bc
REGDEF_BEGIN(SRAM_Y_CURVE47)
REGDEF_BIT(reg_y_lut_94,        10)
REGDEF_BIT(reg_y_lut_95,        10)
REGDEF_BIT(reg_y_lut_96,        10)
REGDEF_END(SRAM_Y_CURVE47)


/*
    reg_y_lut_96:    [0x0, 0x3ff],           bits : 9_0
    reg_y_lut_97:    [0x0, 0x3ff],           bits : 19_10
    reg_y_lut_98:    [0x0, 0x3ff],           bits : 29_20
*/
#define SRAM_Y_CURVE48_OFS 0x07c0
REGDEF_BEGIN(SRAM_Y_CURVE48)
REGDEF_BIT(reg_y_lut_96,        10)
REGDEF_BIT(reg_y_lut_97,        10)
REGDEF_BIT(reg_y_lut_98,        10)
REGDEF_END(SRAM_Y_CURVE48)


/*
    reg_y_lut_98 :    [0x0, 0x3ff],          bits : 9_0
    reg_y_lut_99 :    [0x0, 0x3ff],          bits : 19_10
    reg_y_lut_100:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE49_OFS 0x07c4
REGDEF_BEGIN(SRAM_Y_CURVE49)
REGDEF_BIT(reg_y_lut_98,        10)
REGDEF_BIT(reg_y_lut_99,        10)
REGDEF_BIT(reg_y_lut_100,        10)
REGDEF_END(SRAM_Y_CURVE49)


/*
    reg_y_lut_100:    [0x0, 0x3ff],          bits : 9_0
    reg_y_lut_101:    [0x0, 0x3ff],          bits : 19_10
    reg_y_lut_102:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE50_OFS 0x07c8
REGDEF_BEGIN(SRAM_Y_CURVE50)
REGDEF_BIT(reg_y_lut_100,        10)
REGDEF_BIT(reg_y_lut_101,        10)
REGDEF_BIT(reg_y_lut_102,        10)
REGDEF_END(SRAM_Y_CURVE50)


/*
    reg_y_lut_102:    [0x0, 0x3ff],          bits : 9_0
    reg_y_lut_103:    [0x0, 0x3ff],          bits : 19_10
    reg_y_lut_104:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE51_OFS 0x07cc
REGDEF_BEGIN(SRAM_Y_CURVE51)
REGDEF_BIT(reg_y_lut_102,        10)
REGDEF_BIT(reg_y_lut_103,        10)
REGDEF_BIT(reg_y_lut_104,        10)
REGDEF_END(SRAM_Y_CURVE51)


/*
    reg_y_lut_104:    [0x0, 0x3ff],          bits : 9_0
    reg_y_lut_105:    [0x0, 0x3ff],          bits : 19_10
    reg_y_lut_106:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE52_OFS 0x07d0
REGDEF_BEGIN(SRAM_Y_CURVE52)
REGDEF_BIT(reg_y_lut_104,        10)
REGDEF_BIT(reg_y_lut_105,        10)
REGDEF_BIT(reg_y_lut_106,        10)
REGDEF_END(SRAM_Y_CURVE52)


/*
    reg_y_lut_106:    [0x0, 0x3ff],          bits : 9_0
    reg_y_lut_107:    [0x0, 0x3ff],          bits : 19_10
    reg_y_lut_108:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE53_OFS 0x07d4
REGDEF_BEGIN(SRAM_Y_CURVE53)
REGDEF_BIT(reg_y_lut_106,        10)
REGDEF_BIT(reg_y_lut_107,        10)
REGDEF_BIT(reg_y_lut_108,        10)
REGDEF_END(SRAM_Y_CURVE53)


/*
    reg_y_lut_108:    [0x0, 0x3ff],          bits : 9_0
    reg_y_lut_109:    [0x0, 0x3ff],          bits : 19_10
    reg_y_lut_110:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE54_OFS 0x07d8
REGDEF_BEGIN(SRAM_Y_CURVE54)
REGDEF_BIT(reg_y_lut_108,        10)
REGDEF_BIT(reg_y_lut_109,        10)
REGDEF_BIT(reg_y_lut_110,        10)
REGDEF_END(SRAM_Y_CURVE54)


/*
    reg_y_lut_110:    [0x0, 0x3ff],          bits : 9_0
    reg_y_lut_111:    [0x0, 0x3ff],          bits : 19_10
    reg_y_lut_112:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE55_OFS 0x07dc
REGDEF_BEGIN(SRAM_Y_CURVE55)
REGDEF_BIT(reg_y_lut_110,        10)
REGDEF_BIT(reg_y_lut_111,        10)
REGDEF_BIT(reg_y_lut_112,        10)
REGDEF_END(SRAM_Y_CURVE55)


/*
    reg_y_lut_112:    [0x0, 0x3ff],          bits : 9_0
    reg_y_lut_113:    [0x0, 0x3ff],          bits : 19_10
    reg_y_lut_114:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE56_OFS 0x07e0
REGDEF_BEGIN(SRAM_Y_CURVE56)
REGDEF_BIT(reg_y_lut_112,        10)
REGDEF_BIT(reg_y_lut_113,        10)
REGDEF_BIT(reg_y_lut_114,        10)
REGDEF_END(SRAM_Y_CURVE56)


/*
    reg_y_lut_114:    [0x0, 0x3ff],          bits : 9_0
    reg_y_lut_115:    [0x0, 0x3ff],          bits : 19_10
    reg_y_lut_116:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE57_OFS 0x07e4
REGDEF_BEGIN(SRAM_Y_CURVE57)
REGDEF_BIT(reg_y_lut_114,        10)
REGDEF_BIT(reg_y_lut_115,        10)
REGDEF_BIT(reg_y_lut_116,        10)
REGDEF_END(SRAM_Y_CURVE57)


/*
    reg_y_lut_116:    [0x0, 0x3ff],          bits : 9_0
    reg_y_lut_117:    [0x0, 0x3ff],          bits : 19_10
    reg_y_lut_118:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE58_OFS 0x07e8
REGDEF_BEGIN(SRAM_Y_CURVE58)
REGDEF_BIT(reg_y_lut_116,        10)
REGDEF_BIT(reg_y_lut_117,        10)
REGDEF_BIT(reg_y_lut_118,        10)
REGDEF_END(SRAM_Y_CURVE58)


/*
    reg_y_lut_118:    [0x0, 0x3ff],          bits : 9_0
    reg_y_lut_119:    [0x0, 0x3ff],          bits : 19_10
    reg_y_lut_120:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE59_OFS 0x07ec
REGDEF_BEGIN(SRAM_Y_CURVE59)
REGDEF_BIT(reg_y_lut_118,        10)
REGDEF_BIT(reg_y_lut_119,        10)
REGDEF_BIT(reg_y_lut_120,        10)
REGDEF_END(SRAM_Y_CURVE59)


/*
    reg_y_lut_120:    [0x0, 0x3ff],          bits : 9_0
    reg_y_lut_121:    [0x0, 0x3ff],          bits : 19_10
    reg_y_lut_122:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE60_OFS 0x07f0
REGDEF_BEGIN(SRAM_Y_CURVE60)
REGDEF_BIT(reg_y_lut_120,        10)
REGDEF_BIT(reg_y_lut_121,        10)
REGDEF_BIT(reg_y_lut_122,        10)
REGDEF_END(SRAM_Y_CURVE60)


/*
    reg_y_lut_122:    [0x0, 0x3ff],          bits : 9_0
    reg_y_lut_123:    [0x0, 0x3ff],          bits : 19_10
    reg_y_lut_124:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE61_OFS 0x07f4
REGDEF_BEGIN(SRAM_Y_CURVE61)
REGDEF_BIT(reg_y_lut_122,        10)
REGDEF_BIT(reg_y_lut_123,        10)
REGDEF_BIT(reg_y_lut_124,        10)
REGDEF_END(SRAM_Y_CURVE61)


/*
    reg_y_lut_124:    [0x0, 0x3ff],          bits : 9_0
    reg_y_lut_125:    [0x0, 0x3ff],          bits : 19_10
    reg_y_lut_126:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE62_OFS 0x07f8
REGDEF_BEGIN(SRAM_Y_CURVE62)
REGDEF_BIT(reg_y_lut_124,        10)
REGDEF_BIT(reg_y_lut_125,        10)
REGDEF_BIT(reg_y_lut_126,        10)
REGDEF_END(SRAM_Y_CURVE62)


/*
    reg_y_lut_126:    [0x0, 0x3ff],          bits : 9_0
    reg_y_lut_127:    [0x0, 0x3ff],          bits : 19_10
    reg_y_lut_128:    [0x0, 0x3ff],          bits : 29_20
*/
#define SRAM_Y_CURVE63_OFS 0x07fc
REGDEF_BEGIN(SRAM_Y_CURVE63)
REGDEF_BIT(reg_y_lut_126,        10)
REGDEF_BIT(reg_y_lut_127,        10)
REGDEF_BIT(reg_y_lut_128,        10)
REGDEF_END(SRAM_Y_CURVE63)


/*
    reg_edge_lut_0:    [0x0, 0x3ff],         bits : 9_0
    reg_edge_lut_1:    [0x0, 0x3ff],         bits : 19_10
    reg_edge_lut_2:    [0x0, 0x3ff],         bits : 29_20
*/
#define EDGE_GAMMA0_OFS 0x0800
REGDEF_BEGIN(EDGE_GAMMA0)
REGDEF_BIT(reg_edge_lut_0,        10)
REGDEF_BIT(reg_edge_lut_1,        10)
REGDEF_BIT(reg_edge_lut_2,        10)
REGDEF_END(EDGE_GAMMA0)


/*
    reg_edge_lut_3:    [0x0, 0x3ff],         bits : 9_0
    reg_edge_lut_4:    [0x0, 0x3ff],         bits : 19_10
    reg_edge_lut_5:    [0x0, 0x3ff],         bits : 29_20
*/
#define EDGE_GAMMA1_OFS 0x0804
REGDEF_BEGIN(EDGE_GAMMA1)
REGDEF_BIT(reg_edge_lut_3,        10)
REGDEF_BIT(reg_edge_lut_4,        10)
REGDEF_BIT(reg_edge_lut_5,        10)
REGDEF_END(EDGE_GAMMA1)


/*
    reg_edge_lut_6:    [0x0, 0x3ff],         bits : 9_0
    reg_edge_lut_7:    [0x0, 0x3ff],         bits : 19_10
    reg_edge_lut_8:    [0x0, 0x3ff],         bits : 29_20
*/
#define EDGE_GAMMA2_OFS 0x0808
REGDEF_BEGIN(EDGE_GAMMA2)
REGDEF_BIT(reg_edge_lut_6,        10)
REGDEF_BIT(reg_edge_lut_7,        10)
REGDEF_BIT(reg_edge_lut_8,        10)
REGDEF_END(EDGE_GAMMA2)


/*
    reg_edge_lut_9 :    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_10:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_11:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA3_OFS 0x080c
REGDEF_BEGIN(EDGE_GAMMA3)
REGDEF_BIT(reg_edge_lut_9,        10)
REGDEF_BIT(reg_edge_lut_10,        10)
REGDEF_BIT(reg_edge_lut_11,        10)
REGDEF_END(EDGE_GAMMA3)


/*
    reg_edge_lut_12:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_13:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_14:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA4_OFS 0x0810
REGDEF_BEGIN(EDGE_GAMMA4)
REGDEF_BIT(reg_edge_lut_12,        10)
REGDEF_BIT(reg_edge_lut_13,        10)
REGDEF_BIT(reg_edge_lut_14,        10)
REGDEF_END(EDGE_GAMMA4)


/*
    reg_edge_lut_15:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_16:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_17:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA5_OFS 0x0814
REGDEF_BEGIN(EDGE_GAMMA5)
REGDEF_BIT(reg_edge_lut_15,        10)
REGDEF_BIT(reg_edge_lut_16,        10)
REGDEF_BIT(reg_edge_lut_17,        10)
REGDEF_END(EDGE_GAMMA5)


/*
    reg_edge_lut_18:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_19:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_20:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA6_OFS 0x0818
REGDEF_BEGIN(EDGE_GAMMA6)
REGDEF_BIT(reg_edge_lut_18,        10)
REGDEF_BIT(reg_edge_lut_19,        10)
REGDEF_BIT(reg_edge_lut_20,        10)
REGDEF_END(EDGE_GAMMA6)


/*
    reg_edge_lut_21:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_22:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_23:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA7_OFS 0x081c
REGDEF_BEGIN(EDGE_GAMMA7)
REGDEF_BIT(reg_edge_lut_21,        10)
REGDEF_BIT(reg_edge_lut_22,        10)
REGDEF_BIT(reg_edge_lut_23,        10)
REGDEF_END(EDGE_GAMMA7)


/*
    reg_edge_lut_24:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_25:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_26:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA8_OFS 0x0820
REGDEF_BEGIN(EDGE_GAMMA8)
REGDEF_BIT(reg_edge_lut_24,        10)
REGDEF_BIT(reg_edge_lut_25,        10)
REGDEF_BIT(reg_edge_lut_26,        10)
REGDEF_END(EDGE_GAMMA8)


/*
    reg_edge_lut_27:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_28:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_29:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA9_OFS 0x0824
REGDEF_BEGIN(EDGE_GAMMA9)
REGDEF_BIT(reg_edge_lut_27,        10)
REGDEF_BIT(reg_edge_lut_28,        10)
REGDEF_BIT(reg_edge_lut_29,        10)
REGDEF_END(EDGE_GAMMA9)


/*
    reg_edge_lut_30:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_31:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_32:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA10_OFS 0x0828
REGDEF_BEGIN(EDGE_GAMMA10)
REGDEF_BIT(reg_edge_lut_30,        10)
REGDEF_BIT(reg_edge_lut_31,        10)
REGDEF_BIT(reg_edge_lut_32,        10)
REGDEF_END(EDGE_GAMMA10)


/*
    reg_edge_lut_33:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_34:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_35:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA11_OFS 0x082c
REGDEF_BEGIN(EDGE_GAMMA11)
REGDEF_BIT(reg_edge_lut_33,        10)
REGDEF_BIT(reg_edge_lut_34,        10)
REGDEF_BIT(reg_edge_lut_35,        10)
REGDEF_END(EDGE_GAMMA11)


/*
    reg_edge_lut_36:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_37:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_38:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA12_OFS 0x0830
REGDEF_BEGIN(EDGE_GAMMA12)
REGDEF_BIT(reg_edge_lut_36,        10)
REGDEF_BIT(reg_edge_lut_37,        10)
REGDEF_BIT(reg_edge_lut_38,        10)
REGDEF_END(EDGE_GAMMA12)


/*
    reg_edge_lut_39:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_40:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_41:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA13_OFS 0x0834
REGDEF_BEGIN(EDGE_GAMMA13)
REGDEF_BIT(reg_edge_lut_39,        10)
REGDEF_BIT(reg_edge_lut_40,        10)
REGDEF_BIT(reg_edge_lut_41,        10)
REGDEF_END(EDGE_GAMMA13)


/*
    reg_edge_lut_42:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_43:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_44:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA14_OFS 0x0838
REGDEF_BEGIN(EDGE_GAMMA14)
REGDEF_BIT(reg_edge_lut_42,        10)
REGDEF_BIT(reg_edge_lut_43,        10)
REGDEF_BIT(reg_edge_lut_44,        10)
REGDEF_END(EDGE_GAMMA14)


/*
    reg_edge_lut_45:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_46:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_47:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA15_OFS 0x083c
REGDEF_BEGIN(EDGE_GAMMA15)
REGDEF_BIT(reg_edge_lut_45,        10)
REGDEF_BIT(reg_edge_lut_46,        10)
REGDEF_BIT(reg_edge_lut_47,        10)
REGDEF_END(EDGE_GAMMA15)


/*
    reg_edge_lut_48:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_49:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_50:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA16_OFS 0x0840
REGDEF_BEGIN(EDGE_GAMMA16)
REGDEF_BIT(reg_edge_lut_48,        10)
REGDEF_BIT(reg_edge_lut_49,        10)
REGDEF_BIT(reg_edge_lut_50,        10)
REGDEF_END(EDGE_GAMMA16)


/*
    reg_edge_lut_51:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_52:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_53:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA17_OFS 0x0844
REGDEF_BEGIN(EDGE_GAMMA17)
REGDEF_BIT(reg_edge_lut_51,        10)
REGDEF_BIT(reg_edge_lut_52,        10)
REGDEF_BIT(reg_edge_lut_53,        10)
REGDEF_END(EDGE_GAMMA17)


/*
    reg_edge_lut_54:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_55:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_56:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA18_OFS 0x0848
REGDEF_BEGIN(EDGE_GAMMA18)
REGDEF_BIT(reg_edge_lut_54,        10)
REGDEF_BIT(reg_edge_lut_55,        10)
REGDEF_BIT(reg_edge_lut_56,        10)
REGDEF_END(EDGE_GAMMA18)


/*
    reg_edge_lut_57:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_58:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_59:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA19_OFS 0x084c
REGDEF_BEGIN(EDGE_GAMMA19)
REGDEF_BIT(reg_edge_lut_57,        10)
REGDEF_BIT(reg_edge_lut_58,        10)
REGDEF_BIT(reg_edge_lut_59,        10)
REGDEF_END(EDGE_GAMMA19)


/*
    reg_edge_lut_60:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_61:    [0x0, 0x3ff],            bits : 19_10
    reg_edge_lut_62:    [0x0, 0x3ff],            bits : 29_20
*/
#define EDGE_GAMMA20_OFS 0x0850
REGDEF_BEGIN(EDGE_GAMMA20)
REGDEF_BIT(reg_edge_lut_60,        10)
REGDEF_BIT(reg_edge_lut_61,        10)
REGDEF_BIT(reg_edge_lut_62,        10)
REGDEF_END(EDGE_GAMMA20)


/*
    reg_edge_lut_63:    [0x0, 0x3ff],            bits : 9_0
    reg_edge_lut_64:    [0x0, 0x3ff],            bits : 19_10
*/
#define EDGE_GAMMA21_OFS 0x0854
REGDEF_BEGIN(EDGE_GAMMA21)
REGDEF_BIT(reg_edge_lut_63,        10)
REGDEF_BIT(reg_edge_lut_64,        10)
REGDEF_END(EDGE_GAMMA21)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED59_OFS 0x0858
REGDEF_BEGIN(IPE_RESERVED59)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED59)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED60_OFS 0x085c
REGDEF_BEGIN(IPE_RESERVED60)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED60)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED61_OFS 0x0860
REGDEF_BEGIN(IPE_RESERVED61)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED61)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED62_OFS 0x0864
REGDEF_BEGIN(IPE_RESERVED62)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED62)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED63_OFS 0x0868
REGDEF_BEGIN(IPE_RESERVED63)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED63)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED64_OFS 0x086c
REGDEF_BEGIN(IPE_RESERVED64)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED64)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED65_OFS 0x0870
REGDEF_BEGIN(IPE_RESERVED65)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED65)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED66_OFS 0x0874
REGDEF_BEGIN(IPE_RESERVED66)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED66)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED67_OFS 0x0878
REGDEF_BEGIN(IPE_RESERVED67)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED67)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED68_OFS 0x087c
REGDEF_BEGIN(IPE_RESERVED68)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED68)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED69_OFS 0x0880
REGDEF_BEGIN(IPE_RESERVED69)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED69)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED70_OFS 0x0884
REGDEF_BEGIN(IPE_RESERVED70)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED70)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED71_OFS 0x0888
REGDEF_BEGIN(IPE_RESERVED71)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED71)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED72_OFS 0x088c
REGDEF_BEGIN(IPE_RESERVED72)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED72)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED73_OFS 0x0890
REGDEF_BEGIN(IPE_RESERVED73)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED73)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED74_OFS 0x0894
REGDEF_BEGIN(IPE_RESERVED74)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED74)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED75_OFS 0x0898
REGDEF_BEGIN(IPE_RESERVED75)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED75)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED76_OFS 0x089c
REGDEF_BEGIN(IPE_RESERVED76)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED76)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED77_OFS 0x08a0
REGDEF_BEGIN(IPE_RESERVED77)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED77)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED78_OFS 0x08a4
REGDEF_BEGIN(IPE_RESERVED78)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED78)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED79_OFS 0x08a8
REGDEF_BEGIN(IPE_RESERVED79)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED79)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED80_OFS 0x08ac
REGDEF_BEGIN(IPE_RESERVED80)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED80)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED81_OFS 0x08b0
REGDEF_BEGIN(IPE_RESERVED81)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED81)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED82_OFS 0x08b4
REGDEF_BEGIN(IPE_RESERVED82)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED82)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED83_OFS 0x08b8
REGDEF_BEGIN(IPE_RESERVED83)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED83)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED84_OFS 0x08bc
REGDEF_BEGIN(IPE_RESERVED84)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED84)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED85_OFS 0x08c0
REGDEF_BEGIN(IPE_RESERVED85)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED85)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED86_OFS 0x08c4
REGDEF_BEGIN(IPE_RESERVED86)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED86)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED87_OFS 0x08c8
REGDEF_BEGIN(IPE_RESERVED87)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED87)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED88_OFS 0x08cc
REGDEF_BEGIN(IPE_RESERVED88)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED88)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED89_OFS 0x08d0
REGDEF_BEGIN(IPE_RESERVED89)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED89)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED90_OFS 0x08d4
REGDEF_BEGIN(IPE_RESERVED90)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED90)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED91_OFS 0x08d8
REGDEF_BEGIN(IPE_RESERVED91)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED91)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED92_OFS 0x08dc
REGDEF_BEGIN(IPE_RESERVED92)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED92)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED93_OFS 0x08e0
REGDEF_BEGIN(IPE_RESERVED93)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED93)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED94_OFS 0x08e4
REGDEF_BEGIN(IPE_RESERVED94)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED94)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED95_OFS 0x08e8
REGDEF_BEGIN(IPE_RESERVED95)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED95)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED96_OFS 0x08ec
REGDEF_BEGIN(IPE_RESERVED96)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED96)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED97_OFS 0x08f0
REGDEF_BEGIN(IPE_RESERVED97)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED97)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED98_OFS 0x08f4
REGDEF_BEGIN(IPE_RESERVED98)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED98)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED99_OFS 0x08f8
REGDEF_BEGIN(IPE_RESERVED99)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED99)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IPE_RESERVED100_OFS 0x08fc
REGDEF_BEGIN(IPE_RESERVED100)
REGDEF_BIT(reserved,        32)
REGDEF_END(IPE_RESERVED100)


//---------------------------------------------------------------

typedef struct {

	union {
		struct {
			unsigned ipe_swrst:                   1;
			unsigned ipe_start:                   1;
			unsigned ipe_load_start:              1;
			unsigned ipe_load_fd:                 1;
			unsigned ipe_load_fs:                 1;
			unsigned :                                3;
			unsigned ipe_rwgamma:                 2;
			unsigned ipe_rwgamma_opt:             2;
			unsigned :                               16;
			unsigned ll_fire:                     1;
		} bit;
		UINT32 word;
	} reg_0; // 0x0000

	union {
		struct {
			unsigned ipe_mode:                    2;
			unsigned ipe_imeon:                   1;
			unsigned ipe_dmaon:                   1;
			unsigned dmao_sel:                    2;
			unsigned :                                2;
			unsigned ipe_imat:                    2;
			unsigned ipe_omat:                    2;
			unsigned ipe_subhsel:                 1;
			unsigned :                               14;
			unsigned ipe_mst_hovlp:               1;
			unsigned iny_burst_mode:              1;
			unsigned inc_burst_mode:              1;
			unsigned outy_burst_mode:             1;
			unsigned outc_burst_mode:             1;
		} bit;
		UINT32 word;
	} reg_1; // 0x0004

	union {
		struct {
			unsigned rgblpf_en:                   1;
			unsigned rgbgamma_en:                 1;
			unsigned ycurve_en:                   1;
			unsigned cr_en:                       1;
			unsigned defog_subimg_out_en:         1;
			unsigned defog_en:                    1;
			unsigned lce_en:                      1;
			unsigned cst_en:                      1;
			unsigned :                                1;
			unsigned ctrl_en:                     1;
			unsigned hadj_en:                     1;
			unsigned cadj_en:                     1;
			unsigned cadj_yenh_en:                1;
			unsigned cadj_ycon_en:                1;
			unsigned cadj_ccon_en:                1;
			unsigned cadj_ycth_en:                1;
			unsigned cstprotect_en:               1;
			unsigned edge_dbg_en:                 1;
			unsigned :                                1;
			unsigned vacc_en:                     1;
			unsigned win0_va_en:                  1;
			unsigned win1_va_en:                  1;
			unsigned win2_va_en:                  1;
			unsigned win3_va_en:                  1;
			unsigned win4_va_en:                  1;
			unsigned pfr_en:                      1;
		} bit;
		UINT32 word;
	} reg_2; // 0x0008

	union {
		struct {
			unsigned ipe_dram_out0_single_en:                1;
			unsigned ipe_dram_out1_single_en:                1;
			unsigned ipe_dram_out2_single_en:                1;
			unsigned ipe_dram_out3_single_en:                1;
			unsigned ipe_dram_out_mode:                      1;
		} bit;
		UINT32 word;
	} reg_3; // 0x000c

	union {
		struct {
			unsigned ll_terminate:                1;
		} bit;
		UINT32 word;
	} reg_4; // 0x0010

	union {
		struct {
			unsigned hn:                 11;
			unsigned :                        1;
			unsigned hl:                 11;
			unsigned :                        1;
			unsigned hm:                  4;
		} bit;
		UINT32 word;
	} reg_5; // 0x0014

	union {
		struct {
			unsigned vl:                13;
		} bit;
		UINT32 word;
	} reg_6; // 0x0018

	union {
		struct {
			unsigned :                                    2;
			unsigned dram_sai_ll:                    30;
		} bit;
		UINT32 word;
	} reg_7; // 0x001c

	union {
		struct {
			unsigned :                                    2;
			unsigned dram_sai_y:                     30;
		} bit;
		UINT32 word;
	} reg_8; // 0x0020

	union {
		struct {
			unsigned :                                    2;
			unsigned dram_sai_c:                     30;
		} bit;
		UINT32 word;
	} reg_9; // 0x0024

	union {
		struct {
			unsigned :                                    2;
			unsigned dram_sai_ycurve:                30;
		} bit;
		UINT32 word;
	} reg_10; // 0x0028

	union {
		struct {
			unsigned :                                   2;
			unsigned dram_sai_gamma:                30;
		} bit;
		UINT32 word;
	} reg_11; // 0x002c

	union {
		struct {
			unsigned :                                2;
			unsigned dram_ofsi_y:                14;
			unsigned :                               12;
			unsigned inrand_en:                   1;
			unsigned inrand_rst:                  1;
		} bit;
		UINT32 word;
	} reg_12; // 0x0030

	union {
		struct {
			unsigned :                                2;
			unsigned dram_ofsi_c:                14;
		} bit;
		UINT32 word;
	} reg_13; // 0x0034

	union {
		struct {
			unsigned :                               2;
			unsigned dram_sao_y:                30;
		} bit;
		UINT32 word;
	} reg_14; // 0x0038

	union {
		struct {
			unsigned :                               2;
			unsigned dram_sao_c:                30;
		} bit;
		UINT32 word;
	} reg_15; // 0x003c

	union {
		struct {
			unsigned :                                2;
			unsigned dram_ofso_y:                14;
		} bit;
		UINT32 word;
	} reg_16; // 0x0040

	union {
		struct {
			unsigned :                                2;
			unsigned dram_ofso_c:                14;
		} bit;
		UINT32 word;
	} reg_17; // 0x0044

	union {
		struct {
			unsigned :                                2;
			unsigned dram_sao_va:                30;
		} bit;
		UINT32 word;
	} reg_18; // 0x0048

	union {
		struct {
			unsigned :                                 2;
			unsigned dram_ofso_va:                14;
		} bit;
		UINT32 word;
	} reg_19; // 0x004c

	union {
		struct {
			unsigned :                                   1;
			unsigned inte_frmend:                    1;
			unsigned inte_st:                        1;
			unsigned inte_frmstart:                  1;
			unsigned inte_ycoutend:                  1;
			unsigned inte_dmain0end:                 1;
			unsigned inte_dfg_subout_end:            1;
			unsigned inte_vaoutend:                  1;
			unsigned inte_ll_done:                   1;
			unsigned inte_ll_job_end:                1;
			unsigned inte_ll_error:                  1;
			unsigned inte_ll_error2:                 1;
			unsigned inte_frame_err:                 1;
		} bit;
		UINT32 word;
	} reg_20; // 0x0050

	union {
		struct {
			unsigned :                                   1;
			unsigned int_frmend:                     1;
			unsigned int_st:                         1;
			unsigned int_frmstart:                   1;
			unsigned int_ycoutend:                   1;
			unsigned int_dmain0end:                  1;
			unsigned int_dfg_subout_end:             1;
			unsigned int_vaoutend:                   1;
			unsigned ints_ll_done:                   1;
			unsigned ints_ll_job_end:                1;
			unsigned ints_ll_error:                  1;
			unsigned ints_ll_error2:                 1;
			unsigned ints_frame_err:                 1;
		} bit;
		UINT32 word;
	} reg_21; // 0x0054

	union {
		struct {
			unsigned ipestatus:                  1;
			unsigned :                               3;
			unsigned hcnt:                       4;
			unsigned :                               4;
			unsigned status_y:                   1;
			unsigned status_c:                   1;
			unsigned :                               2;
			unsigned fstr_rst_disable:           1;
		} bit;
		UINT32 word;
	} reg_22; // 0x0058

	union {
		struct {
			unsigned ll_table_idx_0:                8;
			unsigned ll_table_idx_1:                8;
			unsigned ll_table_idx_2:                8;
			unsigned ll_table_idx_3:                8;
		} bit;
		UINT32 word;
	} reg_23; // 0x005c

	union {
		struct {
			unsigned ll_table_idx_4:                8;
			unsigned ll_table_idx_5:                8;
			unsigned ll_table_idx_6:                8;
			unsigned ll_table_idx_7:                8;
		} bit;
		UINT32 word;
	} reg_24; // 0x0060

	union {
		struct {
			unsigned ll_table_idx_8:                 8;
			unsigned ll_table_idx_9:                 8;
			unsigned ll_table_idx_10:                8;
			unsigned ll_table_idx_11:                8;
		} bit;
		UINT32 word;
	} reg_25; // 0x0064

	union {
		struct {
			unsigned ll_table_idx_12:                8;
			unsigned ll_table_idx_13:                8;
			unsigned ll_table_idx_14:                8;
			unsigned ll_table_idx_15:                8;
		} bit;
		UINT32 word;
	} reg_26; // 0x0068

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_27; // 0x006c

	union {
		struct {
			unsigned eext_chsel:                1;
			unsigned eext_gamsel:               1;
			unsigned :                              2;
			unsigned eextdiv_e7:                4;
			unsigned eext_enh_e7:               5;
			unsigned :                              3;
			unsigned eext0:                    10;
		} bit;
		UINT32 word;
	} reg_28; // 0x0070

	union {
		struct {
			unsigned eext1:                10;
			unsigned eext2:                10;
			unsigned eext3:                10;
		} bit;
		UINT32 word;
	} reg_29; // 0x0074

	union {
		struct {
			unsigned eext4:                10;
			unsigned eext5:                10;
			unsigned eext6:                10;
		} bit;
		UINT32 word;
	} reg_30; // 0x0078

	union {
		struct {
			unsigned eext7:                10;
			unsigned eext8:                10;
			unsigned eext9:                10;
		} bit;
		UINT32 word;
	} reg_31; // 0x007c

	union {
		struct {
			unsigned eext_enh_e3:                 6;
			unsigned :                                2;
			unsigned eext_enh_e5a:                6;
			unsigned :                                2;
			unsigned eext_enh_e5b:                6;
		} bit;
		UINT32 word;
	} reg_32; // 0x0080

	union {
		struct {
			unsigned eextdiv_e3:                 3;
			unsigned :                               1;
			unsigned eextdiv_e5a:                3;
			unsigned :                               1;
			unsigned eextdiv_e5b:                3;
			unsigned :                               1;
			unsigned eextdiv_eng:                4;
			unsigned eextdiv_con:                4;
		} bit;
		UINT32 word;
	} reg_33; // 0x0084

	union {
		struct {
			unsigned w_con_eng:                4;
			unsigned w_low:                    5;
			unsigned :                             3;
			unsigned w_high:                   5;
		} bit;
		UINT32 word;
	} reg_34; // 0x0088

	union {
		struct {
			unsigned w_ker_thin:                  4;
			unsigned w_ker_robust:                4;
			unsigned iso_ker_thin:                4;
			unsigned iso_ker_robust:              4;
		} bit;
		UINT32 word;
	} reg_35; // 0x008c

	union {
		struct {
			unsigned w_hld_low:                    5;
			unsigned :                                 3;
			unsigned w_hld_high:                   5;
			unsigned :                                 3;
			unsigned w_ker_thin_hld:               4;
			unsigned w_ker_robust_hld:             4;
			unsigned iso_ker_thin_hld:             4;
			unsigned iso_ker_robust_hld:           4;
		} bit;
		UINT32 word;
	} reg_36; // 0x0090

	union {
		struct {
			unsigned th_flat:                10;
			unsigned :                            2;
			unsigned th_edge:                10;
		} bit;
		UINT32 word;
	} reg_37; // 0x0094

	union {
		struct {
			unsigned th_flat_hld:                10;
			unsigned :                                2;
			unsigned th_edge_hld:                10;
			unsigned th_lum_hld:                 10;
		} bit;
		UINT32 word;
	} reg_38; // 0x0098

	union {
		struct {
			unsigned slope_con_eng:                16;
			unsigned slope_hld_con_eng:                16;
		} bit;
		UINT32 word;
	} reg_39; // 0x009c

	union {
		struct {
			unsigned overshoot_en:                1;
			unsigned :                                3;
			unsigned wt_overshoot:                8;
			unsigned :                                4;
			unsigned wt_undershoot:               8;
		} bit;
		UINT32 word;
	} reg_40; // 0x00a0

	union {
		struct {
			unsigned th_overshoot:                     8;
			unsigned th_undershoot:                    8;
			unsigned th_undershoot_lum:                8;
			unsigned th_undershoot_eng:                8;
		} bit;
		UINT32 word;
	} reg_41; // 0x00a4

	union {
		struct {
			unsigned clamp_wt_mod_lum:                8;
			unsigned clamp_wt_mod_eng:                8;
			unsigned strength_lum_eng:                8;
			unsigned norm_lum_eng:                    8;
		} bit;
		UINT32 word;
	} reg_42; // 0x00a8

	union {
		struct {
			unsigned slope_overshoot:                 15;
			unsigned :                                     1;
			unsigned slope_undershoot:                15;
		} bit;
		UINT32 word;
	} reg_43; // 0x00ac

	union {
		struct {
			unsigned slope_undershoot_lum:            15;
			unsigned :                                     1;
			unsigned slope_undershoot_eng:            15;
		} bit;
		UINT32 word;
	} reg_44; // 0x00b0

	union {
		struct {
			unsigned esthrl:                10;
			unsigned esthrh:                10;
			unsigned establ:                 3;
			unsigned :                           1;
			unsigned estabh:                 3;
		} bit;
		UINT32 word;
	} reg_45; // 0x00b4

	union {
		struct {
			unsigned eslutl_0:                8;
			unsigned eslutl_1:                8;
			unsigned eslutl_2:                8;
			unsigned eslutl_3:                8;
		} bit;
		UINT32 word;
	} reg_46; // 0x00b8

	union {
		struct {
			unsigned eslutl_4:                8;
			unsigned eslutl_5:                8;
			unsigned eslutl_6:                8;
			unsigned eslutl_7:                8;
		} bit;
		UINT32 word;
	} reg_47; // 0x00bc

	union {
		struct {
			unsigned esluth_0:                8;
			unsigned esluth_1:                8;
			unsigned esluth_2:                8;
			unsigned esluth_3:                8;
		} bit;
		UINT32 word;
	} reg_48; // 0x00c0

	union {
		struct {
			unsigned esluth_4:                8;
			unsigned esluth_5:                8;
			unsigned esluth_6:                8;
			unsigned esluth_7:                8;
		} bit;
		UINT32 word;
	} reg_49; // 0x00c4

	union {
		struct {
			unsigned edthrl:                10;
			unsigned edthrh:                10;
			unsigned edtabl:                3;
			unsigned :                          1;
			unsigned edtabh:                3;
			unsigned :                          1;
			unsigned edin_sel:              2;
		} bit;
		UINT32 word;
	} reg_50; // 0x00c8

	union {
		struct {
			unsigned edlutl_0:                8;
			unsigned edlutl_1:                8;
			unsigned edlutl_2:                8;
			unsigned edlutl_3:                8;
		} bit;
		UINT32 word;
	} reg_51; // 0x00cc

	union {
		struct {
			unsigned edlutl_4:                8;
			unsigned edlutl_5:                8;
			unsigned edlutl_6:                8;
			unsigned edlutl_7:                8;
		} bit;
		UINT32 word;
	} reg_52; // 0x00d0

	union {
		struct {
			unsigned edluth_0:                8;
			unsigned edluth_1:                8;
			unsigned edluth_2:                8;
			unsigned edluth_3:                8;
		} bit;
		UINT32 word;
	} reg_53; // 0x00d4

	union {
		struct {
			unsigned edluth_4:                8;
			unsigned edluth_5:                8;
			unsigned edluth_6:                8;
			unsigned edluth_7:                8;
		} bit;
		UINT32 word;
	} reg_54; // 0x00d8

	union {
		struct {
			unsigned localmax_statistics_max:                10;
			unsigned coneng_statistics_max:                10;
			unsigned coneng_statistics_avg:                10;
			unsigned reserved:                2;
		} bit;
		UINT32 word;
	} reg_55; // 0x00dc

	union {
		struct {
			unsigned region_str_en:        1;      // bits : 0
			unsigned :        15;      // bits : 15_1
			unsigned enh_thin:        8;      // bits : 23_16
			unsigned enh_robust:        8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_56; // 0x00e0

	union {
		struct {
			unsigned slope_flat: 16;      // bits : 15_0
			unsigned slope_edge: 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_57; // 0x00e4

	union {
		struct {
			unsigned str_flat                   : 8;      // bits : 7_0
			unsigned str_edge                   : 8;      // bits : 15_8
		} bit;
		UINT32 word;
	} reg_58; // 0x00e8

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_59; // 0x00ec

	union {
		struct {
			unsigned glpfw:                     4;
			unsigned gsonlyw:                   4;
			unsigned g_rangeth0:                10;
			unsigned :                              2;
			unsigned g_rangeth1:                10;
			unsigned g_filtsize:                2;
		} bit;
		UINT32 word;
	} reg_60; // 0x00f0

	union {
		struct {
			unsigned rlpfw:                     4;
			unsigned rsonlyw:                   4;
			unsigned r_rangeth0:                10;
			unsigned :                              2;
			unsigned r_rangeth1:                10;
			unsigned r_filtsize:                2;
		} bit;
		UINT32 word;
	} reg_61; // 0x00f4

	union {
		struct {
			unsigned blpfw:                     4;
			unsigned bsonlyw:                   4;
			unsigned b_rangeth0:                10;
			unsigned :                              2;
			unsigned b_rangeth1:                10;
			unsigned b_filtsize:                2;
		} bit;
		UINT32 word;
	} reg_62; // 0x00f8

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_63; // 0x00fc

	union {
		struct {
			unsigned ccrange:                  2;
			unsigned cc2_sel:                  1;
			unsigned cc_gamsel:                1;
			unsigned ccstab_sel:               2;
			unsigned ccofs_sel:                2;
			unsigned :                             8;
			unsigned coef_rr:                  12;
		} bit;
		UINT32 word;
	} reg_64; // 0x0100

	union {
		struct {
			unsigned coef_rg:                12;
			unsigned :                            4;
			unsigned coef_rb:                12;
		} bit;
		UINT32 word;
	} reg_65; // 0x0104

	union {
		struct {
			unsigned coef_gr:                12;
			unsigned :                            4;
			unsigned coef_gg:                12;
		} bit;
		UINT32 word;
	} reg_66; // 0x0108

	union {
		struct {
			unsigned coef_gb:                12;
			unsigned :                            4;
			unsigned coef_br:                12;
		} bit;
		UINT32 word;
	} reg_67; // 0x010c

	union {
		struct {
			unsigned coef_bg:                12;
			unsigned :                            4;
			unsigned coef_bb:                12;
		} bit;
		UINT32 word;
	} reg_68; // 0x0110

	union {
		struct {
			unsigned fstab0:                8;
			unsigned fstab1:                8;
			unsigned fstab2:                8;
			unsigned fstab3:                8;
		} bit;
		UINT32 word;
	} reg_69; // 0x0114

	union {
		struct {
			unsigned fstab4:                8;
			unsigned fstab5:                8;
			unsigned fstab6:                8;
			unsigned fstab7:                8;
		} bit;
		UINT32 word;
	} reg_70; // 0x0118

	union {
		struct {
			unsigned fstab8:                8;
			unsigned fstab9:                8;
			unsigned fstab10:                8;
			unsigned fstab11:                8;
		} bit;
		UINT32 word;
	} reg_71; // 0x011c

	union {
		struct {
			unsigned fstab12:                8;
			unsigned fstab13:                8;
			unsigned fstab14:                8;
			unsigned fstab15:                8;
		} bit;
		UINT32 word;
	} reg_72; // 0x0120

	union {
		struct {
			unsigned fdtab0:                8;
			unsigned fdtab1:                8;
			unsigned fdtab2:                8;
			unsigned fdtab3:                8;
		} bit;
		UINT32 word;
	} reg_73; // 0x0124

	union {
		struct {
			unsigned fdtab4:                8;
			unsigned fdtab5:                8;
			unsigned fdtab6:                8;
			unsigned fdtab7:                8;
		} bit;
		UINT32 word;
	} reg_74; // 0x0128

	union {
		struct {
			unsigned fdtab8:                8;
			unsigned fdtab9:                8;
			unsigned fdtab10:                8;
			unsigned fdtab11:                8;
		} bit;
		UINT32 word;
	} reg_75; // 0x012c

	union {
		struct {
			unsigned fdtab12:                8;
			unsigned fdtab13:                8;
			unsigned fdtab14:                8;
			unsigned fdtab15:                8;
		} bit;
		UINT32 word;
	} reg_76; // 0x0130

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_77; // 0x0134

	union {
		struct {
			unsigned coef_yr:                9;
			unsigned coef_yg:                9;
			unsigned coef_yb:                9;
			unsigned cstp_rat:                5;
		} bit;
		UINT32 word;
	} reg_78; // 0x0138

	union {
		struct {
			unsigned coef_ur:                9;
			unsigned coef_ug:                9;
			unsigned coef_ub:                9;
			unsigned :                           1;
			unsigned cstoff_sel:             1;
		} bit;
		UINT32 word;
	} reg_79; // 0x013c

	union {
		struct {
			unsigned coef_vr:                9;
			unsigned coef_vg:                9;
			unsigned coef_vb:                9;
		} bit;
		UINT32 word;
	} reg_80; // 0x0140

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_81; // 0x0144

	union {
		struct {
			unsigned int_ofs:                8;
			unsigned sat_ofs:                8;
			unsigned chue_roten:             1;
			unsigned chue_c2gen:             1;
			unsigned :                           2;
			unsigned cctrl_sel:              2;
			unsigned :                           2;
			unsigned vdet_div:               8;
		} bit;
		UINT32 word;
	} reg_82; // 0x0148

	union {
		struct {
			unsigned chuem0:                8;
			unsigned chuem1:                8;
			unsigned chuem2:                8;
			unsigned chuem3:                8;
		} bit;
		UINT32 word;
	} reg_83; // 0x014c

	union {
		struct {
			unsigned chuem4:                8;
			unsigned chuem5:                8;
			unsigned chuem6:                8;
			unsigned chuem7:                8;
		} bit;
		UINT32 word;
	} reg_84; // 0x0150

	union {
		struct {
			unsigned chuem8:                8;
			unsigned chuem9:                8;
			unsigned chuem10:                8;
			unsigned chuem11:                8;
		} bit;
		UINT32 word;
	} reg_85; // 0x0154

	union {
		struct {
			unsigned chuem12:                8;
			unsigned chuem13:                8;
			unsigned chuem14:                8;
			unsigned chuem15:                8;
		} bit;
		UINT32 word;
	} reg_86; // 0x0158

	union {
		struct {
			unsigned chuem16:                8;
			unsigned chuem17:                8;
			unsigned chuem18:                8;
			unsigned chuem19:                8;
		} bit;
		UINT32 word;
	} reg_87; // 0x015c

	union {
		struct {
			unsigned chuem20:                8;
			unsigned chuem21:                8;
			unsigned chuem22:                8;
			unsigned chuem23:                8;
		} bit;
		UINT32 word;
	} reg_88; // 0x0160

	union {
		struct {
			unsigned cintm0:                8;
			unsigned cintm1:                8;
			unsigned cintm2:                8;
			unsigned cintm3:                8;
		} bit;
		UINT32 word;
	} reg_89; // 0x0164

	union {
		struct {
			unsigned cintm4:                8;
			unsigned cintm5:                8;
			unsigned cintm6:                8;
			unsigned cintm7:                8;
		} bit;
		UINT32 word;
	} reg_90; // 0x0168

	union {
		struct {
			unsigned cintm8:                8;
			unsigned cintm9:                8;
			unsigned cintm10:                8;
			unsigned cintm11:                8;
		} bit;
		UINT32 word;
	} reg_91; // 0x016c

	union {
		struct {
			unsigned cintm12:                8;
			unsigned cintm13:                8;
			unsigned cintm14:                8;
			unsigned cintm15:                8;
		} bit;
		UINT32 word;
	} reg_92; // 0x0170

	union {
		struct {
			unsigned cintm16:                8;
			unsigned cintm17:                8;
			unsigned cintm18:                8;
			unsigned cintm19:                8;
		} bit;
		UINT32 word;
	} reg_93; // 0x0174

	union {
		struct {
			unsigned cintm20:                8;
			unsigned cintm21:                8;
			unsigned cintm22:                8;
			unsigned cintm23:                8;
		} bit;
		UINT32 word;
	} reg_94; // 0x0178

	union {
		struct {
			unsigned csatm0:                8;
			unsigned csatm1:                8;
			unsigned csatm2:                8;
			unsigned csatm3:                8;
		} bit;
		UINT32 word;
	} reg_95; // 0x017c

	union {
		struct {
			unsigned csatm4:                8;
			unsigned csatm5:                8;
			unsigned csatm6:                8;
			unsigned csatm7:                8;
		} bit;
		UINT32 word;
	} reg_96; // 0x0180

	union {
		struct {
			unsigned csatm8:                8;
			unsigned csatm9:                8;
			unsigned csatm10:                8;
			unsigned csatm11:                8;
		} bit;
		UINT32 word;
	} reg_97; // 0x0184

	union {
		struct {
			unsigned csatm12:                8;
			unsigned csatm13:                8;
			unsigned csatm14:                8;
			unsigned csatm15:                8;
		} bit;
		UINT32 word;
	} reg_98; // 0x0188

	union {
		struct {
			unsigned csatm16:                8;
			unsigned csatm17:                8;
			unsigned csatm18:                8;
			unsigned csatm19:                8;
		} bit;
		UINT32 word;
	} reg_99; // 0x018c

	union {
		struct {
			unsigned csatm20:                8;
			unsigned csatm21:                8;
			unsigned csatm22:                8;
			unsigned csatm23:                8;
		} bit;
		UINT32 word;
	} reg_100; // 0x0190

	union {
		struct {
			unsigned cedgem0:                8;
			unsigned cedgem1:                8;
			unsigned cedgem2:                8;
			unsigned cedgem3:                8;
		} bit;
		UINT32 word;
	} reg_101; // 0x0194

	union {
		struct {
			unsigned cedgem4:                8;
			unsigned cedgem5:                8;
			unsigned cedgem6:                8;
			unsigned cedgem7:                8;
		} bit;
		UINT32 word;
	} reg_102; // 0x0198

	union {
		struct {
			unsigned cedgem8:                8;
			unsigned cedgem9:                8;
			unsigned cedgem10:                8;
			unsigned cedgem11:                8;
		} bit;
		UINT32 word;
	} reg_103; // 0x019c

	union {
		struct {
			unsigned cedgem12:                8;
			unsigned cedgem13:                8;
			unsigned cedgem14:                8;
			unsigned cedgem15:                8;
		} bit;
		UINT32 word;
	} reg_104; // 0x01a0

	union {
		struct {
			unsigned cedgem16:                8;
			unsigned cedgem17:                8;
			unsigned cedgem18:                8;
			unsigned cedgem19:                8;
		} bit;
		UINT32 word;
	} reg_105; // 0x01a4

	union {
		struct {
			unsigned cedgem20:                8;
			unsigned cedgem21:                8;
			unsigned cedgem22:                8;
			unsigned cedgem23:                8;
		} bit;
		UINT32 word;
	} reg_106; // 0x01a8

	union {
		struct {
			unsigned dds0:                6;
			unsigned dds1:                6;
			unsigned dds2:                6;
			unsigned dds3:                6;
		} bit;
		UINT32 word;
	} reg_107; // 0x01ac

	union {
		struct {
			unsigned dds4:                6;
			unsigned dds5:                6;
			unsigned dds6:                6;
			unsigned dds7:                6;
		} bit;
		UINT32 word;
	} reg_108; // 0x01b0

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_109; // 0x01b4

	union {
		struct {
			unsigned y_enh_p:                10;
			unsigned y_enh_n:                10;
			unsigned :                           10;
			unsigned y_einv_p:                1;
			unsigned y_einv_n:                1;
		} bit;
		UINT32 word;
	} reg_110; // 0x01b8

	union {
		struct {
			unsigned y_con:                    8;
			unsigned yc_randreset:             1;
			unsigned y_rand_en:                1;
			unsigned :                             2;
			unsigned y_rand:                   3;
			unsigned :                             1;
			unsigned y_ethy:                  10;
		} bit;
		UINT32 word;
	} reg_111; // 0x01bc

	union {
		struct {
			unsigned y_yth1:                    8;
			unsigned :                              4;
			unsigned y_hit1sel:                 1;
			unsigned y_nhit1sel:                1;
			unsigned :                              2;
			unsigned y_fix1_hit:                8;
			unsigned y_fix1_nhit:               8;
		} bit;
		UINT32 word;
	} reg_112; // 0x01c0

	union {
		struct {
			unsigned y_yth2:                    8;
			unsigned :                              4;
			unsigned y_hit2sel:                 1;
			unsigned y_nhit2sel:                1;
			unsigned :                              2;
			unsigned y_fix2_hit:                8;
			unsigned y_fix2_nhit:               8;
		} bit;
		UINT32 word;
	} reg_113; // 0x01c4

	union {
		struct {
			unsigned ymask:                 8;
			unsigned cbmask:                8;
			unsigned crmask:                8;
		} bit;
		UINT32 word;
	} reg_114; // 0x01c8

	union {
		struct {
			unsigned c_cbofs:                8;
			unsigned c_crofs:                8;
			unsigned c_con:                  8;
			unsigned :                           3;
			unsigned c_rand_en:              1;
			unsigned c_rand:                 3;
		} bit;
		UINT32 word;
	} reg_115; // 0x01cc

	union {
		struct {
			unsigned cb_fix_hit:                 8;
			unsigned cr_fix_hit:                 8;
			unsigned cb_fix_nhit:                8;
			unsigned cr_fix_nhit:                8;
		} bit;
		UINT32 word;
	} reg_116; // 0x01d0

	union {
		struct {
			unsigned c_eth:                     10;
			unsigned :                              2;
			unsigned c_hitsel:                  1;
			unsigned c_nhitsel:                 1;
			unsigned :                              2;
			unsigned c_yth_h:                   8;
			unsigned c_yth_l:                   8;
		} bit;
		UINT32 word;
	} reg_117; // 0x01d4

	union {
		struct {
			unsigned c_cbth_h:                8;
			unsigned c_cbth_l:                8;
			unsigned c_crth_h:                8;
			unsigned c_crth_l:                8;
		} bit;
		UINT32 word;
	} reg_118; // 0x01d8

	union {
		struct {
			unsigned ccontab0:                10;
			unsigned ccontab1:                10;
			unsigned ccontab2:                10;
		} bit;
		UINT32 word;
	} reg_119; // 0x01dc

	union {
		struct {
			unsigned ccontab3:                10;
			unsigned ccontab4:                10;
			unsigned ccontab5:                10;
		} bit;
		UINT32 word;
	} reg_120; // 0x01e0

	union {
		struct {
			unsigned ccontab6:                10;
			unsigned ccontab7:                10;
			unsigned ccontab8:                10;
		} bit;
		UINT32 word;
	} reg_121; // 0x01e4

	union {
		struct {
			unsigned ccontab9:                10;
			unsigned ccontab10:                10;
			unsigned ccontab11:                10;
		} bit;
		UINT32 word;
	} reg_122; // 0x01e8

	union {
		struct {
			unsigned ccontab12:                10;
			unsigned ccontab13:                10;
			unsigned ccontab14:                10;
		} bit;
		UINT32 word;
	} reg_123; // 0x01ec

	union {
		struct {
			unsigned ccontab15:                10;
			unsigned ccontab16:                10;
			unsigned :                11;
			unsigned ccontab_sel:                1;
		} bit;
		UINT32 word;
	} reg_124; // 0x01f0

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_125; // 0x01f4

	union {
		struct {
			unsigned edge_dbg_modesel:                2;
		} bit;
		UINT32 word;
	} reg_126; // 0x01f8

	union {
		struct {
			unsigned ycurve_sel:                2;
		} bit;
		UINT32 word;
	} reg_127; // 0x01fc

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_128; // 0x0200

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_129; // 0x0204

	union {
		struct {
			unsigned gamyrand_en:                 1;
			unsigned gamyrand_reset:              1;
			unsigned :                                2;
			unsigned gamyrand_shf:                1;
			unsigned :                               23;
			unsigned defogrnd_opt:                2;
			unsigned defogrnd_rst:                1;
		} bit;
		UINT32 word;
	} reg_130; // 0x0208

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_131; // 0x020c

	union {
		struct {
			unsigned :                            1;
			unsigned eth_outfmt:              1;
			unsigned ethlow:                 10;
			unsigned ethmid:                 10;
			unsigned ethhigh:                10;
		} bit;
		UINT32 word;
	} reg_132; // 0x0210

	union {
		struct {
			unsigned hout_sel:                1;
			unsigned vout_sel:                1;
			unsigned eth_outsel:                2;
		} bit;
		UINT32 word;
	} reg_133; // 0x0214

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_134; // 0x0218

	union {
		struct {
			unsigned vdet_gh1_a:                5;
			unsigned :                              2;
			unsigned vdet_gh1_bcd_op:           1;
			unsigned vdet_gh1_b:                5;
			unsigned :                              3;
			unsigned vdet_gh1_c:                4;
			unsigned vdet_gh1_d:                4;
			unsigned vdet_gh1_fsize:            2;
			unsigned :                              2;
			unsigned vdet_gh1_div:              4;
		} bit;
		UINT32 word;
	} reg_135; // 0x021c

	union {
		struct {
			unsigned vdet_gv1_a:                5;
			unsigned :                              2;
			unsigned vdet_gv1_bcd_op:           1;
			unsigned vdet_gv1_b:                5;
			unsigned :                              3;
			unsigned vdet_gv1_c:                4;
			unsigned vdet_gv1_d:                4;
			unsigned vdet_gv1_fsize:            2;
			unsigned :                              2;
			unsigned vdet_gv1_div:              4;
		} bit;
		UINT32 word;
	} reg_136; // 0x0220

	union {
		struct {
			unsigned vdet_gh2_a:                5;
			unsigned :                              2;
			unsigned vdet_gh2_bcd_op:           1;
			unsigned vdet_gh2_b:                5;
			unsigned :                              3;
			unsigned vdet_gh2_c:                4;
			unsigned vdet_gh2_d:                4;
			unsigned vdet_gh2_fsize:            2;
			unsigned :                              2;
			unsigned vdet_gh2_div:              4;
		} bit;
		UINT32 word;
	} reg_137; // 0x0224

	union {
		struct {
			unsigned vdet_gv2_a:                5;
			unsigned :                              2;
			unsigned vdet_gv2_bcd_op:           1;
			unsigned vdet_gv2_b:                5;
			unsigned :                              3;
			unsigned vdet_gv2_c:                4;
			unsigned vdet_gv2_d:                4;
			unsigned vdet_gv2_fsize:            2;
			unsigned :                              2;
			unsigned vdet_gv2_div:              4;
		} bit;
		UINT32 word;
	} reg_138; // 0x0228

	union {
		struct {
			unsigned vacc_outsel:                1;
			unsigned reserved:                       3;
			unsigned va_stx:                    12;
			unsigned va_sty:                    12;
		} bit;
		UINT32 word;
	} reg_139; // 0x022c

	union {
		struct {
			unsigned va_g1h_thl:                8;
			unsigned va_g1h_thh:                8;
			unsigned va_g1v_thl:                8;
			unsigned va_g1v_thh:                8;
		} bit;
		UINT32 word;
	} reg_140; // 0x0230

	union {
		struct {
			unsigned va_g2h_thl:                8;
			unsigned va_g2h_thh:                8;
			unsigned va_g2v_thl:                8;
			unsigned va_g2v_thh:                8;
		} bit;
		UINT32 word;
	} reg_141; // 0x0234

	union {
		struct {
			unsigned va_win_szx:                         9;
			unsigned va_win_szy:                         9;
			unsigned va_g1_line_max_mode:                1;
			unsigned va_g2_line_max_mode:                1;
			unsigned va_g1_cnt_en:                       1;
			unsigned va_g2_cnt_en:                       1;
		} bit;
		UINT32 word;
	} reg_142; // 0x0238

	union {
		struct {
			unsigned va_win_numx:                3;
			unsigned :                               1;
			unsigned va_win_numy:                3;
			unsigned :                               9;
			unsigned va_win_skipx:               6;
			unsigned :                               2;
			unsigned va_win_skipy:               5;
		} bit;
		UINT32 word;
	} reg_143; // 0x023c

	union {
		struct {
			unsigned win0_stx:                12;
			unsigned :                            4;
			unsigned win0_sty:                12;
		} bit;
		UINT32 word;
	} reg_144; // 0x0240

	union {
		struct {
			unsigned win0_g1_line_max_mode:                1;
			unsigned win0_g2_line_max_mode:                1;
			unsigned :                                         2;
			unsigned win0_hsz:                             9;
			unsigned :                                         3;
			unsigned win0_vsz:                             9;
		} bit;
		UINT32 word;
	} reg_145; // 0x0244

	union {
		struct {
			unsigned win1_stx:                12;
			unsigned :                            4;
			unsigned win1_sty:                12;
		} bit;
		UINT32 word;
	} reg_146; // 0x0248

	union {
		struct {
			unsigned win1_g1_line_max_mode:                1;
			unsigned win1_g2_line_max_mode:                1;
			unsigned :                                         2;
			unsigned win1_hsz:                             9;
			unsigned :                                         3;
			unsigned win1_vsz:                             9;
		} bit;
		UINT32 word;
	} reg_147; // 0x024c

	union {
		struct {
			unsigned win2_stx:                12;
			unsigned :                            4;
			unsigned win2_sty:                12;
		} bit;
		UINT32 word;
	} reg_148; // 0x0250

	union {
		struct {
			unsigned win2_g1_line_max_mode:                1;
			unsigned win2_g2_line_max_mode:                1;
			unsigned :                                         2;
			unsigned win2_hsz:                             9;
			unsigned :                                         3;
			unsigned win2_vsz:                             9;
		} bit;
		UINT32 word;
	} reg_149; // 0x0254

	union {
		struct {
			unsigned win3_stx:                12;
			unsigned :                            4;
			unsigned win3_sty:                12;
		} bit;
		UINT32 word;
	} reg_150; // 0x0258

	union {
		struct {
			unsigned win3_g1_line_max_mode:                1;
			unsigned win3_g2_line_max_mode:                1;
			unsigned :                                         2;
			unsigned win3_hsz:                             9;
			unsigned :                                         3;
			unsigned win3_vsz:                             9;
		} bit;
		UINT32 word;
	} reg_151; // 0x025c

	union {
		struct {
			unsigned win4_stx:                12;
			unsigned :                            4;
			unsigned win4_sty:                12;
		} bit;
		UINT32 word;
	} reg_152; // 0x0260

	union {
		struct {
			unsigned win4_g1_line_max_mode:                1;
			unsigned win4_g2_line_max_mode:                1;
			unsigned :                                         2;
			unsigned win4_hsz:                             9;
			unsigned :                                         3;
			unsigned win4_vsz:                             9;
		} bit;
		UINT32 word;
	} reg_153; // 0x0264

	union {
		struct {
			unsigned va_win0g1h_vacc:                 16;
			unsigned va_win0g1h_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_154; // 0x0268

	union {
		struct {
			unsigned va_win0g1v_vacc:                 16;
			unsigned va_win0g1v_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_155; // 0x026c

	union {
		struct {
			unsigned va_win0g2h_vacc:                 16;
			unsigned va_win0g2h_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_156; // 0x0270

	union {
		struct {
			unsigned va_win0g2v_vacc:                 16;
			unsigned va_win0g2v_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_157; // 0x0274

	union {
		struct {
			unsigned va_win1g1h_vacc:                 16;
			unsigned va_win1g1h_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_158; // 0x0278

	union {
		struct {
			unsigned va_win1g1v_vacc:                 16;
			unsigned va_win1g1v_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_159; // 0x027c

	union {
		struct {
			unsigned va_win1g2h_vacc:                 16;
			unsigned va_win1g2h_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_160; // 0x0280

	union {
		struct {
			unsigned va_win1g2v_vacc:                 16;
			unsigned va_win1g2v_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_161; // 0x0284

	union {
		struct {
			unsigned va_win2g1h_vacc:                 16;
			unsigned va_win2g1h_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_162; // 0x0288

	union {
		struct {
			unsigned va_win2g1v_vacc:                 16;
			unsigned va_win2g1v_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_163; // 0x028c

	union {
		struct {
			unsigned va_win2g2h_vacc:                 16;
			unsigned va_win2g2h_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_164; // 0x0290

	union {
		struct {
			unsigned va_win2g2v_vacc:                 16;
			unsigned va_win2g2v_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_165; // 0x0294

	union {
		struct {
			unsigned va_win3g1h_vacc:                 16;
			unsigned va_win3g1h_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_166; // 0x0298

	union {
		struct {
			unsigned va_win3g1v_vacc:                 16;
			unsigned va_win3g1v_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_167; // 0x029c

	union {
		struct {
			unsigned va_win3g2h_vacc:                 16;
			unsigned va_win3g2h_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_168; // 0x02a0

	union {
		struct {
			unsigned va_win3g2v_vacc:                 16;
			unsigned va_win3g2v_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_169; // 0x02a4

	union {
		struct {
			unsigned va_win4g1h_vacc:                 16;
			unsigned va_win4g1h_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_170; // 0x02a8

	union {
		struct {
			unsigned va_win4g1v_vacc:                 16;
			unsigned va_win4g1v_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_171; // 0x02ac

	union {
		struct {
			unsigned va_win4g2h_vacc:                 16;
			unsigned va_win4g2h_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_172; // 0x02b0

	union {
		struct {
			unsigned va_win4g2v_vacc:                 16;
			unsigned va_win4g2v_vacnt:                16;
		} bit;
		UINT32 word;
	} reg_173; // 0x02b4

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_174; // 0x02b8

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_175; // 0x02bc

	union {
		struct {
			unsigned defog_subimg_width:                5;
			unsigned defog_subimg_height:                5;
		} bit;
		UINT32 word;
	} reg_176; // 0x02c0

	union {
		struct {
			unsigned :                                         2;
			unsigned defog_subimg_dramsai:                30;
		} bit;
		UINT32 word;
	} reg_177; // 0x02c4

	union {
		struct {
			unsigned :                                       2;
			unsigned defog_subimg_lofsi:                14;
		} bit;
		UINT32 word;
	} reg_178; // 0x02c8

	union {
		struct {
			unsigned :                2;
			unsigned defog_subimg_dramsao:                30;
		} bit;
		UINT32 word;
	} reg_179; // 0x02cc

	union {
		struct {
			unsigned :                                       2;
			unsigned defog_subimg_lofso:                14;
		} bit;
		UINT32 word;
	} reg_180; // 0x02d0

	union {
		struct {
			unsigned defog_subimg_hfactor:                16;
			unsigned defog_subimg_vfactor:                16;
		} bit;
		UINT32 word;
	} reg_181; // 0x02d4

	union {
		struct {
			unsigned defog_sub_blk_sizeh:                   10;
			unsigned defog_blk_cent_hfactor:                22;
		} bit;
		UINT32 word;
	} reg_182; // 0x02d8

	union {
		struct {
			unsigned defog_sub_blk_sizev:                   10;
			unsigned defog_blk_cent_vfactor:                22;
		} bit;
		UINT32 word;
	} reg_183; // 0x02dc

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_184; // 0x02e0

	union {
		struct {
			unsigned defog_input_bldrto0:                8;
			unsigned defog_input_bldrto1:                8;
			unsigned defog_input_bldrto2:                8;
			unsigned defog_input_bldrto3:                8;
		} bit;
		UINT32 word;
	} reg_185; // 0x02e4

	union {
		struct {
			unsigned defog_input_bldrto4:                8;
			unsigned defog_input_bldrto5:                8;
			unsigned defog_input_bldrto6:                8;
			unsigned defog_input_bldrto7:                8;
		} bit;
		UINT32 word;
	} reg_186; // 0x02e8

	union {
		struct {
			unsigned defog_input_bldrto8:                8;
		} bit;
		UINT32 word;
	} reg_187; // 0x02ec

	union {
		struct {
			unsigned defog_lpf_c0:                3;
			unsigned defog_lpf_c1:                3;
			unsigned defog_lpf_c2:                3;
		} bit;
		UINT32 word;
	} reg_188; // 0x02f0

	union {
		struct {
			unsigned defog_target_lut_0:                10;
			unsigned :                6;
			unsigned defog_target_lut_1:                10;
		} bit;
		UINT32 word;
	} reg_189; // 0x02f4

	union {
		struct {
			unsigned defog_target_lut_2:                10;
			unsigned :                6;
			unsigned defog_target_lut_3:                10;
		} bit;
		UINT32 word;
	} reg_190; // 0x02f8

	union {
		struct {
			unsigned defog_target_lut_4:                10;
			unsigned :                6;
			unsigned defog_target_lut_5:                10;
		} bit;
		UINT32 word;
	} reg_191; // 0x02fc

	union {
		struct {
			unsigned defog_target_lut_6:                10;
			unsigned :                6;
			unsigned defog_target_lut_7:                10;
		} bit;
		UINT32 word;
	} reg_192; // 0x0300

	union {
		struct {
			unsigned defog_target_lut_8:                10;
		} bit;
		UINT32 word;
	} reg_193; // 0x0304

	union {
		struct {
			unsigned defog_mod_lut_0:                10;
			unsigned :                6;
			unsigned defog_mod_lut_1:                10;
		} bit;
		UINT32 word;
	} reg_194; // 0x0308

	union {
		struct {
			unsigned defog_mod_lut_2:                10;
			unsigned :                6;
			unsigned defog_mod_lut_3:                10;
		} bit;
		UINT32 word;
	} reg_195; // 0x030c

	union {
		struct {
			unsigned defog_mod_lut_4:                10;
			unsigned :                6;
			unsigned defog_mod_lut_5:                10;
		} bit;
		UINT32 word;
	} reg_196; // 0x0310

	union {
		struct {
			unsigned defog_mod_lut_6:                10;
			unsigned :                6;
			unsigned defog_mod_lut_7:                10;
		} bit;
		UINT32 word;
	} reg_197; // 0x0314

	union {
		struct {
			unsigned defog_mod_lut_8:                10;
			unsigned :                6;
			unsigned defog_mod_lut_9:                10;
		} bit;
		UINT32 word;
	} reg_198; // 0x0318

	union {
		struct {
			unsigned defog_mod_lut_10:                10;
			unsigned :                6;
			unsigned defog_mod_lut_11:                10;
		} bit;
		UINT32 word;
	} reg_199; // 0x031c

	union {
		struct {
			unsigned defog_mod_lut_12:                10;
			unsigned :                6;
			unsigned defog_mod_lut_13:                10;
		} bit;
		UINT32 word;
	} reg_200; // 0x0320

	union {
		struct {
			unsigned defog_mod_lut_14:                10;
			unsigned :                6;
			unsigned defog_mod_lut_15:                10;
		} bit;
		UINT32 word;
	} reg_201; // 0x0324

	union {
		struct {
			unsigned defog_mod_lut_16:                10;
		} bit;
		UINT32 word;
	} reg_202; // 0x0328

	union {
		struct {
			unsigned defog_interp_diff_lut0:                6;
			unsigned defog_interp_diff_lut1:                6;
			unsigned defog_interp_diff_lut2:                6;
			unsigned defog_interp_diff_lut3:                6;
			unsigned defog_interp_diff_lut4:                6;
		} bit;
		UINT32 word;
	} reg_203; // 0x032c

	union {
		struct {
			unsigned defog_interp_diff_lut5:                6;
			unsigned defog_interp_diff_lut6:                6;
			unsigned defog_interp_diff_lut7:                6;
			unsigned defog_interp_diff_lut8:                6;
			unsigned defog_interp_diff_lut9:                6;
		} bit;
		UINT32 word;
	} reg_204; // 0x0330

	union {
		struct {
			unsigned defog_interp_diff_lut10:                6;
			unsigned defog_interp_diff_lut11:                6;
			unsigned defog_interp_diff_lut12:                6;
			unsigned defog_interp_diff_lut13:                6;
			unsigned defog_interp_diff_lut14:                6;
		} bit;
		UINT32 word;
	} reg_205; // 0x0334

	union {
		struct {
			unsigned defog_interp_diff_lut15:                6;
			unsigned defog_interp_diff_lut16:                6;
		} bit;
		UINT32 word;
	} reg_206; // 0x0338

	union {
		struct {
			unsigned defog_interp_wdist:                4;
			unsigned defog_interp_wout:                4;
			unsigned defog_interp_wcenter:                6;
			unsigned :                2;
			unsigned defog_interp_wsrc:                8;
		} bit;
		UINT32 word;
	} reg_207; // 0x033c

	union {
		struct {
			unsigned defog_air0:                10;
			unsigned :                               6;
			unsigned defog_air1:                10;
		} bit;
		UINT32 word;
	} reg_208; // 0x0340

	union {
		struct {
			unsigned defog_air2:                10;
		} bit;
		UINT32 word;
	} reg_209; // 0x0344

	union {
		struct {
			unsigned defog_statistics_pixcnt:                24;
		} bit;
		UINT32 word;
	} reg_210; // 0x0348

	union {
		struct {
			unsigned defog_statistics_air0:                10;
			unsigned :                6;
			unsigned defog_statistics_air1:                10;
		} bit;
		UINT32 word;
	} reg_211; // 0x034c

	union {
		struct {
			unsigned defog_statistics_air2:                10;
		} bit;
		UINT32 word;
	} reg_212; // 0x0350

	union {
		struct {
			unsigned defog_air_normfactor:                21;
		} bit;
		UINT32 word;
	} reg_213; // 0x0354

	union {
		struct {
			unsigned defog_fog_rto:                   8;
			unsigned defog_min_diff:                 10;
			unsigned :                                    2;
			unsigned defog_dgain_ratio:               8;
			unsigned :                                    2;
			unsigned defog_selfcmp_en:                1;
			unsigned defog_mode_sel:                  1;
		} bit;
		UINT32 word;
	} reg_214; // 0x0358

	union {
		struct {
			unsigned defog_gain_th:                8;
		} bit;
		UINT32 word;
	} reg_215; // 0x035c

	union {
		struct {
			unsigned defog_outbld_lumwt0:                8;
			unsigned defog_outbld_lumwt1:                8;
			unsigned defog_outbld_lumwt2:                8;
			unsigned defog_outbld_lumwt3:                8;
		} bit;
		UINT32 word;
	} reg_216; // 0x0360

	union {
		struct {
			unsigned defog_outbld_lumwt4:                8;
			unsigned defog_outbld_lumwt5:                8;
			unsigned defog_outbld_lumwt6:                8;
			unsigned defog_outbld_lumwt7:                8;
		} bit;
		UINT32 word;
	} reg_217; // 0x0364

	union {
		struct {
			unsigned defog_outbld_lumwt8:                 8;
			unsigned defog_outbld_lumwt9:                 8;
			unsigned defog_outbld_lumwt10:                8;
			unsigned defog_outbld_lumwt11:                8;
		} bit;
		UINT32 word;
	} reg_218; // 0x0368

	union {
		struct {
			unsigned defog_outbld_lumwt12:                8;
			unsigned defog_outbld_lumwt13:                8;
			unsigned defog_outbld_lumwt14:                8;
			unsigned defog_outbld_lumwt15:                8;
		} bit;
		UINT32 word;
	} reg_219; // 0x036c

	union {
		struct {
			unsigned defog_outbld_lumwt16:                8;
			unsigned :                8;
			unsigned defog_wet_ref:                1;
			unsigned defog_local_outbld_en:                1;
		} bit;
		UINT32 word;
	} reg_220; // 0x0370

	union {
		struct {
			unsigned defog_outbld_diffwt0:                6;
			unsigned defog_outbld_diffwt1:                6;
			unsigned defog_outbld_diffwt2:                6;
			unsigned defog_outbld_diffwt3:                6;
			unsigned defog_outbld_diffwt4:                6;
		} bit;
		UINT32 word;
	} reg_221; // 0x0374

	union {
		struct {
			unsigned defog_outbld_diffwt5:                6;
			unsigned defog_outbld_diffwt6:                6;
			unsigned defog_outbld_diffwt7:                6;
			unsigned defog_outbld_diffwt8:                6;
			unsigned defog_outbld_diffwt9:                6;
		} bit;
		UINT32 word;
	} reg_222; // 0x0378

	union {
		struct {
			unsigned defog_outbld_diffwt10:                6;
			unsigned defog_outbld_diffwt11:                6;
			unsigned defog_outbld_diffwt12:                6;
			unsigned defog_outbld_diffwt13:                6;
			unsigned defog_outbld_diffwt14:                6;
		} bit;
		UINT32 word;
	} reg_223; // 0x037c

	union {
		struct {
			unsigned defog_outbld_diffwt15:                6;
			unsigned defog_outbld_diffwt16:                6;
		} bit;
		UINT32 word;
	} reg_224; // 0x0380

	union {
		struct {
			unsigned lce_wt_diff_pos:                8;
			unsigned lce_wt_diff_neg:                8;
			unsigned lce_wt_diff_avg:                8;
		} bit;
		UINT32 word;
	} reg_225; // 0x0384

	union {
		struct {
			unsigned lce_lum_adj_lut0:                8;
			unsigned lce_lum_adj_lut1:                8;
			unsigned lce_lum_adj_lut2:                8;
			unsigned lce_lum_adj_lut3:                8;
		} bit;
		UINT32 word;
	} reg_226; // 0x0388

	union {
		struct {
			unsigned lce_lum_adj_lut4:                8;
			unsigned lce_lum_adj_lut5:                8;
			unsigned lce_lum_adj_lut6:                8;
			unsigned lce_lum_adj_lut7:                8;
		} bit;
		UINT32 word;
	} reg_227; // 0x038c

	union {
		struct {
			unsigned lce_lum_adj_lut8:                8;
		} bit;
		UINT32 word;
	} reg_228; // 0x0390

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_229; // 0x0394

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_230; // 0x0398

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_231; // 0x039c

	union {
		struct {
			unsigned pfr_uv_filt_en:             1;
			unsigned pfr_luma_level_en:          1;
			unsigned pfr_set0_en:                1;
			unsigned pfr_set1_en:                1;
			unsigned pfr_set2_en:                1;
			unsigned pfr_set3_en:                1;
			unsigned :                              10;
			unsigned pfr_out_wet:                8;
		} bit;
		UINT32 word;
	} reg_232; // 0x03a0

	union {
		struct {
			unsigned pfr_edge_th:                8;
			unsigned pfr_edge_str:               8;
			unsigned :                               8;
			unsigned pfr_g_wet:                  8;
		} bit;
		UINT32 word;
	} reg_233; // 0x03a4

	union {
		struct {
			unsigned pfr_color_u0:                8;
			unsigned pfr_color_v0:                8;
			unsigned pfr_r_wet0:                8;
			unsigned pfr_b_wet0:                8;
		} bit;
		UINT32 word;
	} reg_234; // 0x03a8

	union {
		struct {
			unsigned pfr_color_u1:                8;
			unsigned pfr_color_v1:                8;
			unsigned pfr_r_wet1:                  8;
			unsigned pfr_b_wet1:                  8;
		} bit;
		UINT32 word;
	} reg_235; // 0x03ac

	union {
		struct {
			unsigned pfr_color_u2:                8;
			unsigned pfr_color_v2:                8;
			unsigned pfr_r_wet2:                  8;
			unsigned pfr_b_wet2:                  8;
		} bit;
		UINT32 word;
	} reg_236; // 0x03b0

	union {
		struct {
			unsigned pfr_color_u3:                8;
			unsigned pfr_color_v3:                8;
			unsigned pfr_r_wet3:                  8;
			unsigned pfr_b_wet3:                  8;
		} bit;
		UINT32 word;
	} reg_237; // 0x03b4

	union {
		struct {
			unsigned pfr_cdif_set0_lut0:                8;
			unsigned pfr_cdif_set0_lut1:                8;
			unsigned pfr_cdif_set0_lut2:                8;
			unsigned pfr_cdif_set0_lut3:                8;
		} bit;
		UINT32 word;
	} reg_238; // 0x03b8

	union {
		struct {
			unsigned pfr_cdif_set0_lut4:                8;
			unsigned :                                      8;
			unsigned pfr_cdif_set0_th:                  8;
			unsigned pfr_cdif_set0_step:                3;
		} bit;
		UINT32 word;
	} reg_239; // 0x03bc

	union {
		struct {
			unsigned pfr_cdif_set1_lut0:                8;
			unsigned pfr_cdif_set1_lut1:                8;
			unsigned pfr_cdif_set1_lut2:                8;
			unsigned pfr_cdif_set1_lut3:                8;
		} bit;
		UINT32 word;
	} reg_240; // 0x03c0

	union {
		struct {
			unsigned pfr_cdif_set1_lut4:                8;
			unsigned :                                      8;
			unsigned pfr_cdif_set1_th:                  8;
			unsigned pfr_cdif_set1_step:                3;
		} bit;
		UINT32 word;
	} reg_241; // 0x03c4

	union {
		struct {
			unsigned pfr_cdif_set2_lut0:                8;
			unsigned pfr_cdif_set2_lut1:                8;
			unsigned pfr_cdif_set2_lut2:                8;
			unsigned pfr_cdif_set2_lut3:                8;
		} bit;
		UINT32 word;
	} reg_242; // 0x03c8

	union {
		struct {
			unsigned pfr_cdif_set2_lut4:                8;
			unsigned :                                      8;
			unsigned pfr_cdif_set2_th:                  8;
			unsigned pfr_cdif_set2_step:                3;
		} bit;
		UINT32 word;
	} reg_243; // 0x03cc

	union {
		struct {
			unsigned pfr_cdif_set3_lut0:                8;
			unsigned pfr_cdif_set3_lut1:                8;
			unsigned pfr_cdif_set3_lut2:                8;
			unsigned pfr_cdif_set3_lut3:                8;
		} bit;
		UINT32 word;
	} reg_244; // 0x03d0

	union {
		struct {
			unsigned pfr_cdif_set3_lut4:                8;
			unsigned :                                      8;
			unsigned pfr_cdif_set3_th:                  8;
			unsigned pfr_cdif_set3_step:                3;
		} bit;
		UINT32 word;
	} reg_245; // 0x03d4

	union {
		struct {
			unsigned pfr_luma_level_0:                8;
			unsigned pfr_luma_level_1:                8;
			unsigned pfr_luma_level_2:                8;
			unsigned pfr_luma_level_3:                8;
		} bit;
		UINT32 word;
	} reg_246; // 0x03d8

	union {
		struct {
			unsigned pfr_luma_level_4:                8;
			unsigned pfr_luma_level_5:                8;
			unsigned pfr_luma_level_6:                8;
			unsigned pfr_luma_level_7:                8;
		} bit;
		UINT32 word;
	} reg_247; // 0x03dc

	union {
		struct {
			unsigned pfr_luma_level_8:                8;
			unsigned pfr_luma_level_9:                8;
			unsigned pfr_luma_level_10:                8;
			unsigned pfr_luma_level_11:                8;
		} bit;
		UINT32 word;
	} reg_248; // 0x03e0

	union {
		struct {
			unsigned pfr_luma_level_12:                8;
			unsigned :                                     8;
			unsigned pfr_luma_th:                     10;
		} bit;
		UINT32 word;
	} reg_249; // 0x03e4

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_250; // 0x03e8

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_251; // 0x03ec

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_252; // 0x03f0

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_253; // 0x03f4

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_254; // 0x03f8

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_255; // 0x03fc

	union {
		struct {
			unsigned r_lut_0:                10;
			unsigned r_lut_1:                10;
			unsigned r_lut_2:                10;
		} bit;
		UINT32 word;
	} reg_256; // 0x0400

	union {
		struct {
			unsigned r_lut_2:                10;
			unsigned r_lut_3:                10;
			unsigned r_lut_4:                10;
		} bit;
		UINT32 word;
	} reg_257; // 0x0404

	union {
		struct {
			unsigned r_lut_4:                10;
			unsigned r_lut_5:                10;
			unsigned r_lut_6:                10;
		} bit;
		UINT32 word;
	} reg_258; // 0x0408

	union {
		struct {
			unsigned r_lut_6:                10;
			unsigned r_lut_7:                10;
			unsigned r_lut_8:                10;
		} bit;
		UINT32 word;
	} reg_259; // 0x040c

	union {
		struct {
			unsigned r_lut_8:                10;
			unsigned r_lut_9:                10;
			unsigned r_lut_10:                10;
		} bit;
		UINT32 word;
	} reg_260; // 0x0410

	union {
		struct {
			unsigned r_lut_10:                10;
			unsigned r_lut_11:                10;
			unsigned r_lut_12:                10;
		} bit;
		UINT32 word;
	} reg_261; // 0x0414

	union {
		struct {
			unsigned r_lut_12:                10;
			unsigned r_lut_13:                10;
			unsigned r_lut_14:                10;
		} bit;
		UINT32 word;
	} reg_262; // 0x0418

	union {
		struct {
			unsigned r_lut_14:                10;
			unsigned r_lut_15:                10;
			unsigned r_lut_16:                10;
		} bit;
		UINT32 word;
	} reg_263; // 0x041c

	union {
		struct {
			unsigned r_lut_16:                10;
			unsigned r_lut_17:                10;
			unsigned r_lut_18:                10;
		} bit;
		UINT32 word;
	} reg_264; // 0x0420

	union {
		struct {
			unsigned r_lut_18:                10;
			unsigned r_lut_19:                10;
			unsigned r_lut_20:                10;
		} bit;
		UINT32 word;
	} reg_265; // 0x0424

	union {
		struct {
			unsigned r_lut_20:                10;
			unsigned r_lut_21:                10;
			unsigned r_lut_22:                10;
		} bit;
		UINT32 word;
	} reg_266; // 0x0428

	union {
		struct {
			unsigned r_lut_22:                10;
			unsigned r_lut_23:                10;
			unsigned r_lut_24:                10;
		} bit;
		UINT32 word;
	} reg_267; // 0x042c

	union {
		struct {
			unsigned r_lut_24:                10;
			unsigned r_lut_25:                10;
			unsigned r_lut_26:                10;
		} bit;
		UINT32 word;
	} reg_268; // 0x0430

	union {
		struct {
			unsigned r_lut_26:                10;
			unsigned r_lut_27:                10;
			unsigned r_lut_28:                10;
		} bit;
		UINT32 word;
	} reg_269; // 0x0434

	union {
		struct {
			unsigned r_lut_28:                10;
			unsigned r_lut_29:                10;
			unsigned r_lut_30:                10;
		} bit;
		UINT32 word;
	} reg_270; // 0x0438

	union {
		struct {
			unsigned r_lut_30:                10;
			unsigned r_lut_31:                10;
			unsigned r_lut_32:                10;
		} bit;
		UINT32 word;
	} reg_271; // 0x043c

	union {
		struct {
			unsigned r_lut_32:                10;
			unsigned r_lut_33:                10;
			unsigned r_lut_34:                10;
		} bit;
		UINT32 word;
	} reg_272; // 0x0440

	union {
		struct {
			unsigned r_lut_34:                10;
			unsigned r_lut_35:                10;
			unsigned r_lut_36:                10;
		} bit;
		UINT32 word;
	} reg_273; // 0x0444

	union {
		struct {
			unsigned r_lut_36:                10;
			unsigned r_lut_37:                10;
			unsigned r_lut_38:                10;
		} bit;
		UINT32 word;
	} reg_274; // 0x0448

	union {
		struct {
			unsigned r_lut_38:                10;
			unsigned r_lut_39:                10;
			unsigned r_lut_40:                10;
		} bit;
		UINT32 word;
	} reg_275; // 0x044c

	union {
		struct {
			unsigned r_lut_40:                10;
			unsigned r_lut_41:                10;
			unsigned r_lut_42:                10;
		} bit;
		UINT32 word;
	} reg_276; // 0x0450

	union {
		struct {
			unsigned r_lut_42:                10;
			unsigned r_lut_43:                10;
			unsigned r_lut_44:                10;
		} bit;
		UINT32 word;
	} reg_277; // 0x0454

	union {
		struct {
			unsigned r_lut_44:                10;
			unsigned r_lut_45:                10;
			unsigned r_lut_46:                10;
		} bit;
		UINT32 word;
	} reg_278; // 0x0458

	union {
		struct {
			unsigned r_lut_46:                10;
			unsigned r_lut_47:                10;
			unsigned r_lut_48:                10;
		} bit;
		UINT32 word;
	} reg_279; // 0x045c

	union {
		struct {
			unsigned r_lut_48:                10;
			unsigned r_lut_49:                10;
			unsigned r_lut_50:                10;
		} bit;
		UINT32 word;
	} reg_280; // 0x0460

	union {
		struct {
			unsigned r_lut_50:                10;
			unsigned r_lut_51:                10;
			unsigned r_lut_52:                10;
		} bit;
		UINT32 word;
	} reg_281; // 0x0464

	union {
		struct {
			unsigned r_lut_52:                10;
			unsigned r_lut_53:                10;
			unsigned r_lut_54:                10;
		} bit;
		UINT32 word;
	} reg_282; // 0x0468

	union {
		struct {
			unsigned r_lut_54:                10;
			unsigned r_lut_55:                10;
			unsigned r_lut_56:                10;
		} bit;
		UINT32 word;
	} reg_283; // 0x046c

	union {
		struct {
			unsigned r_lut_56:                10;
			unsigned r_lut_57:                10;
			unsigned r_lut_58:                10;
		} bit;
		UINT32 word;
	} reg_284; // 0x0470

	union {
		struct {
			unsigned r_lut_58:                10;
			unsigned r_lut_59:                10;
			unsigned r_lut_60:                10;
		} bit;
		UINT32 word;
	} reg_285; // 0x0474

	union {
		struct {
			unsigned r_lut_60:                10;
			unsigned r_lut_61:                10;
			unsigned r_lut_62:                10;
		} bit;
		UINT32 word;
	} reg_286; // 0x0478

	union {
		struct {
			unsigned r_lut_62:                10;
			unsigned r_lut_63:                10;
			unsigned r_lut_64:                10;
		} bit;
		UINT32 word;
	} reg_287; // 0x047c

	union {
		struct {
			unsigned r_lut_64:                10;
			unsigned r_lut_65:                10;
			unsigned r_lut_66:                10;
		} bit;
		UINT32 word;
	} reg_288; // 0x0480

	union {
		struct {
			unsigned r_lut_66:                10;
			unsigned r_lut_67:                10;
			unsigned r_lut_68:                10;
		} bit;
		UINT32 word;
	} reg_289; // 0x0484

	union {
		struct {
			unsigned r_lut_68:                10;
			unsigned r_lut_69:                10;
			unsigned r_lut_70:                10;
		} bit;
		UINT32 word;
	} reg_290; // 0x0488

	union {
		struct {
			unsigned r_lut_70:                10;
			unsigned r_lut_71:                10;
			unsigned r_lut_72:                10;
		} bit;
		UINT32 word;
	} reg_291; // 0x048c

	union {
		struct {
			unsigned r_lut_72:                10;
			unsigned r_lut_73:                10;
			unsigned r_lut_74:                10;
		} bit;
		UINT32 word;
	} reg_292; // 0x0490

	union {
		struct {
			unsigned r_lut_74:                10;
			unsigned r_lut_75:                10;
			unsigned r_lut_76:                10;
		} bit;
		UINT32 word;
	} reg_293; // 0x0494

	union {
		struct {
			unsigned r_lut_76:                10;
			unsigned r_lut_77:                10;
			unsigned r_lut_78:                10;
		} bit;
		UINT32 word;
	} reg_294; // 0x0498

	union {
		struct {
			unsigned r_lut_78:                10;
			unsigned r_lut_79:                10;
			unsigned r_lut_80:                10;
		} bit;
		UINT32 word;
	} reg_295; // 0x049c

	union {
		struct {
			unsigned r_lut_80:                10;
			unsigned r_lut_81:                10;
			unsigned r_lut_82:                10;
		} bit;
		UINT32 word;
	} reg_296; // 0x04a0

	union {
		struct {
			unsigned r_lut_82:                10;
			unsigned r_lut_83:                10;
			unsigned r_lut_84:                10;
		} bit;
		UINT32 word;
	} reg_297; // 0x04a4

	union {
		struct {
			unsigned r_lut_84:                10;
			unsigned r_lut_85:                10;
			unsigned r_lut_86:                10;
		} bit;
		UINT32 word;
	} reg_298; // 0x04a8

	union {
		struct {
			unsigned r_lut_86:                10;
			unsigned r_lut_87:                10;
			unsigned r_lut_88:                10;
		} bit;
		UINT32 word;
	} reg_299; // 0x04ac

	union {
		struct {
			unsigned r_lut_88:                10;
			unsigned r_lut_89:                10;
			unsigned r_lut_90:                10;
		} bit;
		UINT32 word;
	} reg_300; // 0x04b0

	union {
		struct {
			unsigned r_lut_90:                10;
			unsigned r_lut_91:                10;
			unsigned r_lut_92:                10;
		} bit;
		UINT32 word;
	} reg_301; // 0x04b4

	union {
		struct {
			unsigned r_lut_92:                10;
			unsigned r_lut_93:                10;
			unsigned r_lut_94:                10;
		} bit;
		UINT32 word;
	} reg_302; // 0x04b8

	union {
		struct {
			unsigned r_lut_94:                10;
			unsigned r_lut_95:                10;
			unsigned r_lut_96:                10;
		} bit;
		UINT32 word;
	} reg_303; // 0x04bc

	union {
		struct {
			unsigned r_lut_96:                10;
			unsigned r_lut_97:                10;
			unsigned r_lut_98:                10;
		} bit;
		UINT32 word;
	} reg_304; // 0x04c0

	union {
		struct {
			unsigned r_lut_98:                10;
			unsigned r_lut_99:                10;
			unsigned r_lut_100:                10;
		} bit;
		UINT32 word;
	} reg_305; // 0x04c4

	union {
		struct {
			unsigned r_lut_100:                10;
			unsigned r_lut_101:                10;
			unsigned r_lut_102:                10;
		} bit;
		UINT32 word;
	} reg_306; // 0x04c8

	union {
		struct {
			unsigned r_lut_102:                10;
			unsigned r_lut_103:                10;
			unsigned r_lut_104:                10;
		} bit;
		UINT32 word;
	} reg_307; // 0x04cc

	union {
		struct {
			unsigned r_lut_104:                10;
			unsigned r_lut_105:                10;
			unsigned r_lut_106:                10;
		} bit;
		UINT32 word;
	} reg_308; // 0x04d0

	union {
		struct {
			unsigned r_lut_106:                10;
			unsigned r_lut_107:                10;
			unsigned r_lut_108:                10;
		} bit;
		UINT32 word;
	} reg_309; // 0x04d4

	union {
		struct {
			unsigned r_lut_108:                10;
			unsigned r_lut_109:                10;
			unsigned r_lut_110:                10;
		} bit;
		UINT32 word;
	} reg_310; // 0x04d8

	union {
		struct {
			unsigned r_lut_110:                10;
			unsigned r_lut_111:                10;
			unsigned r_lut_112:                10;
		} bit;
		UINT32 word;
	} reg_311; // 0x04dc

	union {
		struct {
			unsigned r_lut_112:                10;
			unsigned r_lut_113:                10;
			unsigned r_lut_114:                10;
		} bit;
		UINT32 word;
	} reg_312; // 0x04e0

	union {
		struct {
			unsigned r_lut_114:                10;
			unsigned r_lut_115:                10;
			unsigned r_lut_116:                10;
		} bit;
		UINT32 word;
	} reg_313; // 0x04e4

	union {
		struct {
			unsigned r_lut_116:                10;
			unsigned r_lut_117:                10;
			unsigned r_lut_118:                10;
		} bit;
		UINT32 word;
	} reg_314; // 0x04e8

	union {
		struct {
			unsigned r_lut_118:                10;
			unsigned r_lut_119:                10;
			unsigned r_lut_120:                10;
		} bit;
		UINT32 word;
	} reg_315; // 0x04ec

	union {
		struct {
			unsigned r_lut_120:                10;
			unsigned r_lut_121:                10;
			unsigned r_lut_122:                10;
		} bit;
		UINT32 word;
	} reg_316; // 0x04f0

	union {
		struct {
			unsigned r_lut_122:                10;
			unsigned r_lut_123:                10;
			unsigned r_lut_124:                10;
		} bit;
		UINT32 word;
	} reg_317; // 0x04f4

	union {
		struct {
			unsigned r_lut_124:                10;
			unsigned r_lut_125:                10;
			unsigned r_lut_126:                10;
		} bit;
		UINT32 word;
	} reg_318; // 0x04f8

	union {
		struct {
			unsigned r_lut_126:                10;
			unsigned r_lut_127:                10;
			unsigned r_lut_128:                10;
		} bit;
		UINT32 word;
	} reg_319; // 0x04fc

	union {
		struct {
			unsigned g_lut_0:                10;
			unsigned g_lut_1:                10;
			unsigned g_lut_2:                10;
		} bit;
		UINT32 word;
	} reg_320; // 0x0500

	union {
		struct {
			unsigned g_lut_2:                10;
			unsigned g_lut_3:                10;
			unsigned g_lut_4:                10;
		} bit;
		UINT32 word;
	} reg_321; // 0x0504

	union {
		struct {
			unsigned g_lut_4:                10;
			unsigned g_lut_5:                10;
			unsigned g_lut_6:                10;
		} bit;
		UINT32 word;
	} reg_322; // 0x0508

	union {
		struct {
			unsigned g_lut_6:                10;
			unsigned g_lut_7:                10;
			unsigned g_lut_8:                10;
		} bit;
		UINT32 word;
	} reg_323; // 0x050c

	union {
		struct {
			unsigned g_lut_8:                10;
			unsigned g_lut_9:                10;
			unsigned g_lut_10:                10;
		} bit;
		UINT32 word;
	} reg_324; // 0x0510

	union {
		struct {
			unsigned g_lut_10:                10;
			unsigned g_lut_11:                10;
			unsigned g_lut_12:                10;
		} bit;
		UINT32 word;
	} reg_325; // 0x0514

	union {
		struct {
			unsigned g_lut_12:                10;
			unsigned g_lut_13:                10;
			unsigned g_lut_14:                10;
		} bit;
		UINT32 word;
	} reg_326; // 0x0518

	union {
		struct {
			unsigned g_lut_14:                10;
			unsigned g_lut_15:                10;
			unsigned g_lut_16:                10;
		} bit;
		UINT32 word;
	} reg_327; // 0x051c

	union {
		struct {
			unsigned g_lut_16:                10;
			unsigned g_lut_17:                10;
			unsigned g_lut_18:                10;
		} bit;
		UINT32 word;
	} reg_328; // 0x0520

	union {
		struct {
			unsigned g_lut_18:                10;
			unsigned g_lut_19:                10;
			unsigned g_lut_20:                10;
		} bit;
		UINT32 word;
	} reg_329; // 0x0524

	union {
		struct {
			unsigned g_lut_20:                10;
			unsigned g_lut_21:                10;
			unsigned g_lut_22:                10;
		} bit;
		UINT32 word;
	} reg_330; // 0x0528

	union {
		struct {
			unsigned g_lut_22:                10;
			unsigned g_lut_23:                10;
			unsigned g_lut_24:                10;
		} bit;
		UINT32 word;
	} reg_331; // 0x052c

	union {
		struct {
			unsigned g_lut_24:                10;
			unsigned g_lut_25:                10;
			unsigned g_lut_26:                10;
		} bit;
		UINT32 word;
	} reg_332; // 0x0530

	union {
		struct {
			unsigned g_lut_26:                10;
			unsigned g_lut_27:                10;
			unsigned g_lut_28:                10;
		} bit;
		UINT32 word;
	} reg_333; // 0x0534

	union {
		struct {
			unsigned g_lut_28:                10;
			unsigned g_lut_29:                10;
			unsigned g_lut_30:                10;
		} bit;
		UINT32 word;
	} reg_334; // 0x0538

	union {
		struct {
			unsigned g_lut_30:                10;
			unsigned g_lut_31:                10;
			unsigned g_lut_32:                10;
		} bit;
		UINT32 word;
	} reg_335; // 0x053c

	union {
		struct {
			unsigned g_lut_32:                10;
			unsigned g_lut_33:                10;
			unsigned g_lut_34:                10;
		} bit;
		UINT32 word;
	} reg_336; // 0x0540

	union {
		struct {
			unsigned g_lut_34:                10;
			unsigned g_lut_35:                10;
			unsigned g_lut_36:                10;
		} bit;
		UINT32 word;
	} reg_337; // 0x0544

	union {
		struct {
			unsigned g_lut_36:                10;
			unsigned g_lut_37:                10;
			unsigned g_lut_38:                10;
		} bit;
		UINT32 word;
	} reg_338; // 0x0548

	union {
		struct {
			unsigned g_lut_38:                10;
			unsigned g_lut_39:                10;
			unsigned g_lut_40:                10;
		} bit;
		UINT32 word;
	} reg_339; // 0x054c

	union {
		struct {
			unsigned g_lut_40:                10;
			unsigned g_lut_41:                10;
			unsigned g_lut_42:                10;
		} bit;
		UINT32 word;
	} reg_340; // 0x0550

	union {
		struct {
			unsigned g_lut_42:                10;
			unsigned g_lut_43:                10;
			unsigned g_lut_44:                10;
		} bit;
		UINT32 word;
	} reg_341; // 0x0554

	union {
		struct {
			unsigned g_lut_44:                10;
			unsigned g_lut_45:                10;
			unsigned g_lut_46:                10;
		} bit;
		UINT32 word;
	} reg_342; // 0x0558

	union {
		struct {
			unsigned g_lut_46:                10;
			unsigned g_lut_47:                10;
			unsigned g_lut_48:                10;
		} bit;
		UINT32 word;
	} reg_343; // 0x055c

	union {
		struct {
			unsigned g_lut_48:                10;
			unsigned g_lut_49:                10;
			unsigned g_lut_50:                10;
		} bit;
		UINT32 word;
	} reg_344; // 0x0560

	union {
		struct {
			unsigned g_lut_50:                10;
			unsigned g_lut_51:                10;
			unsigned g_lut_52:                10;
		} bit;
		UINT32 word;
	} reg_345; // 0x0564

	union {
		struct {
			unsigned g_lut_52:                10;
			unsigned g_lut_53:                10;
			unsigned g_lut_54:                10;
		} bit;
		UINT32 word;
	} reg_346; // 0x0568

	union {
		struct {
			unsigned g_lut_54:                10;
			unsigned g_lut_55:                10;
			unsigned g_lut_56:                10;
		} bit;
		UINT32 word;
	} reg_347; // 0x056c

	union {
		struct {
			unsigned g_lut_56:                10;
			unsigned g_lut_57:                10;
			unsigned g_lut_58:                10;
		} bit;
		UINT32 word;
	} reg_348; // 0x0570

	union {
		struct {
			unsigned g_lut_58:                10;
			unsigned g_lut_59:                10;
			unsigned g_lut_60:                10;
		} bit;
		UINT32 word;
	} reg_349; // 0x0574

	union {
		struct {
			unsigned g_lut_60:                10;
			unsigned g_lut_61:                10;
			unsigned g_lut_62:                10;
		} bit;
		UINT32 word;
	} reg_350; // 0x0578

	union {
		struct {
			unsigned g_lut_62:                10;
			unsigned g_lut_63:                10;
			unsigned g_lut_64:                10;
		} bit;
		UINT32 word;
	} reg_351; // 0x057c

	union {
		struct {
			unsigned g_lut_64:                10;
			unsigned g_lut_65:                10;
			unsigned g_lut_66:                10;
		} bit;
		UINT32 word;
	} reg_352; // 0x0580

	union {
		struct {
			unsigned g_lut_66:                10;
			unsigned g_lut_67:                10;
			unsigned g_lut_68:                10;
		} bit;
		UINT32 word;
	} reg_353; // 0x0584

	union {
		struct {
			unsigned g_lut_68:                10;
			unsigned g_lut_69:                10;
			unsigned g_lut_70:                10;
		} bit;
		UINT32 word;
	} reg_354; // 0x0588

	union {
		struct {
			unsigned g_lut_70:                10;
			unsigned g_lut_71:                10;
			unsigned g_lut_72:                10;
		} bit;
		UINT32 word;
	} reg_355; // 0x058c

	union {
		struct {
			unsigned g_lut_72:                10;
			unsigned g_lut_73:                10;
			unsigned g_lut_74:                10;
		} bit;
		UINT32 word;
	} reg_356; // 0x0590

	union {
		struct {
			unsigned g_lut_74:                10;
			unsigned g_lut_75:                10;
			unsigned g_lut_76:                10;
		} bit;
		UINT32 word;
	} reg_357; // 0x0594

	union {
		struct {
			unsigned g_lut_76:                10;
			unsigned g_lut_77:                10;
			unsigned g_lut_78:                10;
		} bit;
		UINT32 word;
	} reg_358; // 0x0598

	union {
		struct {
			unsigned g_lut_78:                10;
			unsigned g_lut_79:                10;
			unsigned g_lut_80:                10;
		} bit;
		UINT32 word;
	} reg_359; // 0x059c

	union {
		struct {
			unsigned g_lut_80:                10;
			unsigned g_lut_81:                10;
			unsigned g_lut_82:                10;
		} bit;
		UINT32 word;
	} reg_360; // 0x05a0

	union {
		struct {
			unsigned g_lut_82:                10;
			unsigned g_lut_83:                10;
			unsigned g_lut_84:                10;
		} bit;
		UINT32 word;
	} reg_361; // 0x05a4

	union {
		struct {
			unsigned g_lut_84:                10;
			unsigned g_lut_85:                10;
			unsigned g_lut_86:                10;
		} bit;
		UINT32 word;
	} reg_362; // 0x05a8

	union {
		struct {
			unsigned g_lut_86:                10;
			unsigned g_lut_87:                10;
			unsigned g_lut_88:                10;
		} bit;
		UINT32 word;
	} reg_363; // 0x05ac

	union {
		struct {
			unsigned g_lut_88:                10;
			unsigned g_lut_89:                10;
			unsigned g_lut_90:                10;
		} bit;
		UINT32 word;
	} reg_364; // 0x05b0

	union {
		struct {
			unsigned g_lut_90:                10;
			unsigned g_lut_91:                10;
			unsigned g_lut_92:                10;
		} bit;
		UINT32 word;
	} reg_365; // 0x05b4

	union {
		struct {
			unsigned g_lut_92:                10;
			unsigned g_lut_93:                10;
			unsigned g_lut_94:                10;
		} bit;
		UINT32 word;
	} reg_366; // 0x05b8

	union {
		struct {
			unsigned g_lut_94:                10;
			unsigned g_lut_95:                10;
			unsigned g_lut_96:                10;
		} bit;
		UINT32 word;
	} reg_367; // 0x05bc

	union {
		struct {
			unsigned g_lut_96:                10;
			unsigned g_lut_97:                10;
			unsigned g_lut_98:                10;
		} bit;
		UINT32 word;
	} reg_368; // 0x05c0

	union {
		struct {
			unsigned g_lut_98:                10;
			unsigned g_lut_99:                10;
			unsigned g_lut_100:                10;
		} bit;
		UINT32 word;
	} reg_369; // 0x05c4

	union {
		struct {
			unsigned g_lut_100:                10;
			unsigned g_lut_101:                10;
			unsigned g_lut_102:                10;
		} bit;
		UINT32 word;
	} reg_370; // 0x05c8

	union {
		struct {
			unsigned g_lut_102:                10;
			unsigned g_lut_103:                10;
			unsigned g_lut_104:                10;
		} bit;
		UINT32 word;
	} reg_371; // 0x05cc

	union {
		struct {
			unsigned g_lut_104:                10;
			unsigned g_lut_105:                10;
			unsigned g_lut_106:                10;
		} bit;
		UINT32 word;
	} reg_372; // 0x05d0

	union {
		struct {
			unsigned g_lut_106:                10;
			unsigned g_lut_107:                10;
			unsigned g_lut_108:                10;
		} bit;
		UINT32 word;
	} reg_373; // 0x05d4

	union {
		struct {
			unsigned g_lut_108:                10;
			unsigned g_lut_109:                10;
			unsigned g_lut_110:                10;
		} bit;
		UINT32 word;
	} reg_374; // 0x05d8

	union {
		struct {
			unsigned g_lut_110:                10;
			unsigned g_lut_111:                10;
			unsigned g_lut_112:                10;
		} bit;
		UINT32 word;
	} reg_375; // 0x05dc

	union {
		struct {
			unsigned g_lut_112:                10;
			unsigned g_lut_113:                10;
			unsigned g_lut_114:                10;
		} bit;
		UINT32 word;
	} reg_376; // 0x05e0

	union {
		struct {
			unsigned g_lut_114:                10;
			unsigned g_lut_115:                10;
			unsigned g_lut_116:                10;
		} bit;
		UINT32 word;
	} reg_377; // 0x05e4

	union {
		struct {
			unsigned g_lut_116:                10;
			unsigned g_lut_117:                10;
			unsigned g_lut_118:                10;
		} bit;
		UINT32 word;
	} reg_378; // 0x05e8

	union {
		struct {
			unsigned g_lut_118:                10;
			unsigned g_lut_119:                10;
			unsigned g_lut_120:                10;
		} bit;
		UINT32 word;
	} reg_379; // 0x05ec

	union {
		struct {
			unsigned g_lut_120:                10;
			unsigned g_lut_121:                10;
			unsigned g_lut_122:                10;
		} bit;
		UINT32 word;
	} reg_380; // 0x05f0

	union {
		struct {
			unsigned g_lut_122:                10;
			unsigned g_lut_123:                10;
			unsigned g_lut_124:                10;
		} bit;
		UINT32 word;
	} reg_381; // 0x05f4

	union {
		struct {
			unsigned g_lut_124:                10;
			unsigned g_lut_125:                10;
			unsigned g_lut_126:                10;
		} bit;
		UINT32 word;
	} reg_382; // 0x05f8

	union {
		struct {
			unsigned g_lut_126:                10;
			unsigned g_lut_127:                10;
			unsigned g_lut_128:                10;
		} bit;
		UINT32 word;
	} reg_383; // 0x05fc

	union {
		struct {
			unsigned b_lut_0:                10;
			unsigned b_lut_1:                10;
			unsigned b_lut_2:                10;
		} bit;
		UINT32 word;
	} reg_384; // 0x0600

	union {
		struct {
			unsigned b_lut_2:                10;
			unsigned b_lut_3:                10;
			unsigned b_lut_4:                10;
		} bit;
		UINT32 word;
	} reg_385; // 0x0604

	union {
		struct {
			unsigned b_lut_4:                10;
			unsigned b_lut_5:                10;
			unsigned b_lut_6:                10;
		} bit;
		UINT32 word;
	} reg_386; // 0x0608

	union {
		struct {
			unsigned b_lut_6:                10;
			unsigned b_lut_7:                10;
			unsigned b_lut_8:                10;
		} bit;
		UINT32 word;
	} reg_387; // 0x060c

	union {
		struct {
			unsigned b_lut_8:                10;
			unsigned b_lut_9:                10;
			unsigned b_lut_10:                10;
		} bit;
		UINT32 word;
	} reg_388; // 0x0610

	union {
		struct {
			unsigned b_lut_10:                10;
			unsigned b_lut_11:                10;
			unsigned b_lut_12:                10;
		} bit;
		UINT32 word;
	} reg_389; // 0x0614

	union {
		struct {
			unsigned b_lut_12:                10;
			unsigned b_lut_13:                10;
			unsigned b_lut_14:                10;
		} bit;
		UINT32 word;
	} reg_390; // 0x0618

	union {
		struct {
			unsigned b_lut_14:                10;
			unsigned b_lut_15:                10;
			unsigned b_lut_16:                10;
		} bit;
		UINT32 word;
	} reg_391; // 0x061c

	union {
		struct {
			unsigned b_lut_16:                10;
			unsigned b_lut_17:                10;
			unsigned b_lut_18:                10;
		} bit;
		UINT32 word;
	} reg_392; // 0x0620

	union {
		struct {
			unsigned b_lut_18:                10;
			unsigned b_lut_19:                10;
			unsigned b_lut_20:                10;
		} bit;
		UINT32 word;
	} reg_393; // 0x0624

	union {
		struct {
			unsigned b_lut_20:                10;
			unsigned b_lut_21:                10;
			unsigned b_lut_22:                10;
		} bit;
		UINT32 word;
	} reg_394; // 0x0628

	union {
		struct {
			unsigned b_lut_22:                10;
			unsigned b_lut_23:                10;
			unsigned b_lut_24:                10;
		} bit;
		UINT32 word;
	} reg_395; // 0x062c

	union {
		struct {
			unsigned b_lut_24:                10;
			unsigned b_lut_25:                10;
			unsigned b_lut_26:                10;
		} bit;
		UINT32 word;
	} reg_396; // 0x0630

	union {
		struct {
			unsigned b_lut_26:                10;
			unsigned b_lut_27:                10;
			unsigned b_lut_28:                10;
		} bit;
		UINT32 word;
	} reg_397; // 0x0634

	union {
		struct {
			unsigned b_lut_28:                10;
			unsigned b_lut_29:                10;
			unsigned b_lut_30:                10;
		} bit;
		UINT32 word;
	} reg_398; // 0x0638

	union {
		struct {
			unsigned b_lut_30:                10;
			unsigned b_lut_31:                10;
			unsigned b_lut_32:                10;
		} bit;
		UINT32 word;
	} reg_399; // 0x063c

	union {
		struct {
			unsigned b_lut_32:                10;
			unsigned b_lut_33:                10;
			unsigned b_lut_34:                10;
		} bit;
		UINT32 word;
	} reg_400; // 0x0640

	union {
		struct {
			unsigned b_lut_34:                10;
			unsigned b_lut_35:                10;
			unsigned b_lut_36:                10;
		} bit;
		UINT32 word;
	} reg_401; // 0x0644

	union {
		struct {
			unsigned b_lut_36:                10;
			unsigned b_lut_37:                10;
			unsigned b_lut_38:                10;
		} bit;
		UINT32 word;
	} reg_402; // 0x0648

	union {
		struct {
			unsigned b_lut_38:                10;
			unsigned b_lut_39:                10;
			unsigned b_lut_40:                10;
		} bit;
		UINT32 word;
	} reg_403; // 0x064c

	union {
		struct {
			unsigned b_lut_40:                10;
			unsigned b_lut_41:                10;
			unsigned b_lut_42:                10;
		} bit;
		UINT32 word;
	} reg_404; // 0x0650

	union {
		struct {
			unsigned b_lut_42:                10;
			unsigned b_lut_43:                10;
			unsigned b_lut_44:                10;
		} bit;
		UINT32 word;
	} reg_405; // 0x0654

	union {
		struct {
			unsigned b_lut_44:                10;
			unsigned b_lut_45:                10;
			unsigned b_lut_46:                10;
		} bit;
		UINT32 word;
	} reg_406; // 0x0658

	union {
		struct {
			unsigned b_lut_46:                10;
			unsigned b_lut_47:                10;
			unsigned b_lut_48:                10;
		} bit;
		UINT32 word;
	} reg_407; // 0x065c

	union {
		struct {
			unsigned b_lut_48:                10;
			unsigned b_lut_49:                10;
			unsigned b_lut_50:                10;
		} bit;
		UINT32 word;
	} reg_408; // 0x0660

	union {
		struct {
			unsigned b_lut_50:                10;
			unsigned b_lut_51:                10;
			unsigned b_lut_52:                10;
		} bit;
		UINT32 word;
	} reg_409; // 0x0664

	union {
		struct {
			unsigned b_lut_52:                10;
			unsigned b_lut_53:                10;
			unsigned b_lut_54:                10;
		} bit;
		UINT32 word;
	} reg_410; // 0x0668

	union {
		struct {
			unsigned b_lut_54:                10;
			unsigned b_lut_55:                10;
			unsigned b_lut_56:                10;
		} bit;
		UINT32 word;
	} reg_411; // 0x066c

	union {
		struct {
			unsigned b_lut_56:                10;
			unsigned b_lut_57:                10;
			unsigned b_lut_58:                10;
		} bit;
		UINT32 word;
	} reg_412; // 0x0670

	union {
		struct {
			unsigned b_lut_58:                10;
			unsigned b_lut_59:                10;
			unsigned b_lut_60:                10;
		} bit;
		UINT32 word;
	} reg_413; // 0x0674

	union {
		struct {
			unsigned b_lut_60:                10;
			unsigned b_lut_61:                10;
			unsigned b_lut_62:                10;
		} bit;
		UINT32 word;
	} reg_414; // 0x0678

	union {
		struct {
			unsigned b_lut_62:                10;
			unsigned b_lut_63:                10;
			unsigned b_lut_64:                10;
		} bit;
		UINT32 word;
	} reg_415; // 0x067c

	union {
		struct {
			unsigned b_lut_64:                10;
			unsigned b_lut_65:                10;
			unsigned b_lut_66:                10;
		} bit;
		UINT32 word;
	} reg_416; // 0x0680

	union {
		struct {
			unsigned b_lut_66:                10;
			unsigned b_lut_67:                10;
			unsigned b_lut_68:                10;
		} bit;
		UINT32 word;
	} reg_417; // 0x0684

	union {
		struct {
			unsigned b_lut_68:                10;
			unsigned b_lut_69:                10;
			unsigned b_lut_70:                10;
		} bit;
		UINT32 word;
	} reg_418; // 0x0688

	union {
		struct {
			unsigned b_lut_70:                10;
			unsigned b_lut_71:                10;
			unsigned b_lut_72:                10;
		} bit;
		UINT32 word;
	} reg_419; // 0x068c

	union {
		struct {
			unsigned b_lut_72:                10;
			unsigned b_lut_73:                10;
			unsigned b_lut_74:                10;
		} bit;
		UINT32 word;
	} reg_420; // 0x0690

	union {
		struct {
			unsigned b_lut_74:                10;
			unsigned b_lut_75:                10;
			unsigned b_lut_76:                10;
		} bit;
		UINT32 word;
	} reg_421; // 0x0694

	union {
		struct {
			unsigned b_lut_76:                10;
			unsigned b_lut_77:                10;
			unsigned b_lut_78:                10;
		} bit;
		UINT32 word;
	} reg_422; // 0x0698

	union {
		struct {
			unsigned b_lut_78:                10;
			unsigned b_lut_79:                10;
			unsigned b_lut_80:                10;
		} bit;
		UINT32 word;
	} reg_423; // 0x069c

	union {
		struct {
			unsigned b_lut_80:                10;
			unsigned b_lut_81:                10;
			unsigned b_lut_82:                10;
		} bit;
		UINT32 word;
	} reg_424; // 0x06a0

	union {
		struct {
			unsigned b_lut_82:                10;
			unsigned b_lut_83:                10;
			unsigned b_lut_84:                10;
		} bit;
		UINT32 word;
	} reg_425; // 0x06a4

	union {
		struct {
			unsigned b_lut_84:                10;
			unsigned b_lut_85:                10;
			unsigned b_lut_86:                10;
		} bit;
		UINT32 word;
	} reg_426; // 0x06a8

	union {
		struct {
			unsigned b_lut_86:                10;
			unsigned b_lut_87:                10;
			unsigned b_lut_88:                10;
		} bit;
		UINT32 word;
	} reg_427; // 0x06ac

	union {
		struct {
			unsigned b_lut_88:                10;
			unsigned b_lut_89:                10;
			unsigned b_lut_90:                10;
		} bit;
		UINT32 word;
	} reg_428; // 0x06b0

	union {
		struct {
			unsigned b_lut_90:                10;
			unsigned b_lut_91:                10;
			unsigned b_lut_92:                10;
		} bit;
		UINT32 word;
	} reg_429; // 0x06b4

	union {
		struct {
			unsigned b_lut_92:                10;
			unsigned b_lut_93:                10;
			unsigned b_lut_94:                10;
		} bit;
		UINT32 word;
	} reg_430; // 0x06b8

	union {
		struct {
			unsigned b_lut_94:                10;
			unsigned b_lut_95:                10;
			unsigned b_lut_96:                10;
		} bit;
		UINT32 word;
	} reg_431; // 0x06bc

	union {
		struct {
			unsigned b_lut_96:                10;
			unsigned b_lut_97:                10;
			unsigned b_lut_98:                10;
		} bit;
		UINT32 word;
	} reg_432; // 0x06c0

	union {
		struct {
			unsigned b_lut_98:                10;
			unsigned b_lut_99:                10;
			unsigned b_lut_100:                10;
		} bit;
		UINT32 word;
	} reg_433; // 0x06c4

	union {
		struct {
			unsigned b_lut_100:                10;
			unsigned b_lut_101:                10;
			unsigned b_lut_102:                10;
		} bit;
		UINT32 word;
	} reg_434; // 0x06c8

	union {
		struct {
			unsigned b_lut_102:                10;
			unsigned b_lut_103:                10;
			unsigned b_lut_104:                10;
		} bit;
		UINT32 word;
	} reg_435; // 0x06cc

	union {
		struct {
			unsigned b_lut_104:                10;
			unsigned b_lut_105:                10;
			unsigned b_lut_106:                10;
		} bit;
		UINT32 word;
	} reg_436; // 0x06d0

	union {
		struct {
			unsigned b_lut_106:                10;
			unsigned b_lut_107:                10;
			unsigned b_lut_108:                10;
		} bit;
		UINT32 word;
	} reg_437; // 0x06d4

	union {
		struct {
			unsigned b_lut_108:                10;
			unsigned b_lut_109:                10;
			unsigned b_lut_110:                10;
		} bit;
		UINT32 word;
	} reg_438; // 0x06d8

	union {
		struct {
			unsigned b_lut_110:                10;
			unsigned b_lut_111:                10;
			unsigned b_lut_112:                10;
		} bit;
		UINT32 word;
	} reg_439; // 0x06dc

	union {
		struct {
			unsigned b_lut_112:                10;
			unsigned b_lut_113:                10;
			unsigned b_lut_114:                10;
		} bit;
		UINT32 word;
	} reg_440; // 0x06e0

	union {
		struct {
			unsigned b_lut_114:                10;
			unsigned b_lut_115:                10;
			unsigned b_lut_116:                10;
		} bit;
		UINT32 word;
	} reg_441; // 0x06e4

	union {
		struct {
			unsigned b_lut_116:                10;
			unsigned b_lut_117:                10;
			unsigned b_lut_118:                10;
		} bit;
		UINT32 word;
	} reg_442; // 0x06e8

	union {
		struct {
			unsigned b_lut_118:                10;
			unsigned b_lut_119:                10;
			unsigned b_lut_120:                10;
		} bit;
		UINT32 word;
	} reg_443; // 0x06ec

	union {
		struct {
			unsigned b_lut_120:                10;
			unsigned b_lut_121:                10;
			unsigned b_lut_122:                10;
		} bit;
		UINT32 word;
	} reg_444; // 0x06f0

	union {
		struct {
			unsigned b_lut_122:                10;
			unsigned b_lut_123:                10;
			unsigned b_lut_124:                10;
		} bit;
		UINT32 word;
	} reg_445; // 0x06f4

	union {
		struct {
			unsigned b_lut_124:                10;
			unsigned b_lut_125:                10;
			unsigned b_lut_126:                10;
		} bit;
		UINT32 word;
	} reg_446; // 0x06f8

	union {
		struct {
			unsigned b_lut_126:                10;
			unsigned b_lut_127:                10;
			unsigned b_lut_128:                10;
		} bit;
		UINT32 word;
	} reg_447; // 0x06fc

	union {
		struct {
			unsigned y_lut_0:                10;
			unsigned y_lut_1:                10;
			unsigned y_lut_2:                10;
		} bit;
		UINT32 word;
	} reg_448; // 0x0700

	union {
		struct {
			unsigned y_lut_2:                10;
			unsigned y_lut_3:                10;
			unsigned y_lut_4:                10;
		} bit;
		UINT32 word;
	} reg_449; // 0x0704

	union {
		struct {
			unsigned y_lut_4:                10;
			unsigned y_lut_5:                10;
			unsigned y_lut_6:                10;
		} bit;
		UINT32 word;
	} reg_450; // 0x0708

	union {
		struct {
			unsigned y_lut_6:                10;
			unsigned y_lut_7:                10;
			unsigned y_lut_8:                10;
		} bit;
		UINT32 word;
	} reg_451; // 0x070c

	union {
		struct {
			unsigned y_lut_8:                10;
			unsigned y_lut_9:                10;
			unsigned y_lut_10:                10;
		} bit;
		UINT32 word;
	} reg_452; // 0x0710

	union {
		struct {
			unsigned y_lut_10:                10;
			unsigned y_lut_11:                10;
			unsigned y_lut_12:                10;
		} bit;
		UINT32 word;
	} reg_453; // 0x0714

	union {
		struct {
			unsigned y_lut_12:                10;
			unsigned y_lut_13:                10;
			unsigned y_lut_14:                10;
		} bit;
		UINT32 word;
	} reg_454; // 0x0718

	union {
		struct {
			unsigned y_lut_14:                10;
			unsigned y_lut_15:                10;
			unsigned y_lut_16:                10;
		} bit;
		UINT32 word;
	} reg_455; // 0x071c

	union {
		struct {
			unsigned y_lut_16:                10;
			unsigned y_lut_17:                10;
			unsigned y_lut_18:                10;
		} bit;
		UINT32 word;
	} reg_456; // 0x0720

	union {
		struct {
			unsigned y_lut_18:                10;
			unsigned y_lut_19:                10;
			unsigned y_lut_20:                10;
		} bit;
		UINT32 word;
	} reg_457; // 0x0724

	union {
		struct {
			unsigned y_lut_20:                10;
			unsigned y_lut_21:                10;
			unsigned y_lut_22:                10;
		} bit;
		UINT32 word;
	} reg_458; // 0x0728

	union {
		struct {
			unsigned y_lut_22:                10;
			unsigned y_lut_23:                10;
			unsigned y_lut_24:                10;
		} bit;
		UINT32 word;
	} reg_459; // 0x072c

	union {
		struct {
			unsigned y_lut_24:                10;
			unsigned y_lut_25:                10;
			unsigned y_lut_26:                10;
		} bit;
		UINT32 word;
	} reg_460; // 0x0730

	union {
		struct {
			unsigned y_lut_26:                10;
			unsigned y_lut_27:                10;
			unsigned y_lut_28:                10;
		} bit;
		UINT32 word;
	} reg_461; // 0x0734

	union {
		struct {
			unsigned y_lut_28:                10;
			unsigned y_lut_29:                10;
			unsigned y_lut_30:                10;
		} bit;
		UINT32 word;
	} reg_462; // 0x0738

	union {
		struct {
			unsigned y_lut_30:                10;
			unsigned y_lut_31:                10;
			unsigned y_lut_32:                10;
		} bit;
		UINT32 word;
	} reg_463; // 0x073c

	union {
		struct {
			unsigned y_lut_32:                10;
			unsigned y_lut_33:                10;
			unsigned y_lut_34:                10;
		} bit;
		UINT32 word;
	} reg_464; // 0x0740

	union {
		struct {
			unsigned y_lut_34:                10;
			unsigned y_lut_35:                10;
			unsigned y_lut_36:                10;
		} bit;
		UINT32 word;
	} reg_465; // 0x0744

	union {
		struct {
			unsigned y_lut_36:                10;
			unsigned y_lut_37:                10;
			unsigned y_lut_38:                10;
		} bit;
		UINT32 word;
	} reg_466; // 0x0748

	union {
		struct {
			unsigned y_lut_38:                10;
			unsigned y_lut_39:                10;
			unsigned y_lut_40:                10;
		} bit;
		UINT32 word;
	} reg_467; // 0x074c

	union {
		struct {
			unsigned y_lut_40:                10;
			unsigned y_lut_41:                10;
			unsigned y_lut_42:                10;
		} bit;
		UINT32 word;
	} reg_468; // 0x0750

	union {
		struct {
			unsigned y_lut_42:                10;
			unsigned y_lut_43:                10;
			unsigned y_lut_44:                10;
		} bit;
		UINT32 word;
	} reg_469; // 0x0754

	union {
		struct {
			unsigned y_lut_44:                10;
			unsigned y_lut_45:                10;
			unsigned y_lut_46:                10;
		} bit;
		UINT32 word;
	} reg_470; // 0x0758

	union {
		struct {
			unsigned y_lut_46:                10;
			unsigned y_lut_47:                10;
			unsigned y_lut_48:                10;
		} bit;
		UINT32 word;
	} reg_471; // 0x075c

	union {
		struct {
			unsigned y_lut_48:                10;
			unsigned y_lut_49:                10;
			unsigned y_lut_50:                10;
		} bit;
		UINT32 word;
	} reg_472; // 0x0760

	union {
		struct {
			unsigned y_lut_50:                10;
			unsigned y_lut_51:                10;
			unsigned y_lut_52:                10;
		} bit;
		UINT32 word;
	} reg_473; // 0x0764

	union {
		struct {
			unsigned y_lut_52:                10;
			unsigned y_lut_53:                10;
			unsigned y_lut_54:                10;
		} bit;
		UINT32 word;
	} reg_474; // 0x0768

	union {
		struct {
			unsigned y_lut_54:                10;
			unsigned y_lut_55:                10;
			unsigned y_lut_56:                10;
		} bit;
		UINT32 word;
	} reg_475; // 0x076c

	union {
		struct {
			unsigned y_lut_56:                10;
			unsigned y_lut_57:                10;
			unsigned y_lut_58:                10;
		} bit;
		UINT32 word;
	} reg_476; // 0x0770

	union {
		struct {
			unsigned y_lut_58:                10;
			unsigned y_lut_59:                10;
			unsigned y_lut_60:                10;
		} bit;
		UINT32 word;
	} reg_477; // 0x0774

	union {
		struct {
			unsigned y_lut_60:                10;
			unsigned y_lut_61:                10;
			unsigned y_lut_62:                10;
		} bit;
		UINT32 word;
	} reg_478; // 0x0778

	union {
		struct {
			unsigned y_lut_62:                10;
			unsigned y_lut_63:                10;
			unsigned y_lut_64:                10;
		} bit;
		UINT32 word;
	} reg_479; // 0x077c

	union {
		struct {
			unsigned y_lut_64:                10;
			unsigned y_lut_65:                10;
			unsigned y_lut_66:                10;
		} bit;
		UINT32 word;
	} reg_480; // 0x0780

	union {
		struct {
			unsigned y_lut_66:                10;
			unsigned y_lut_67:                10;
			unsigned y_lut_68:                10;
		} bit;
		UINT32 word;
	} reg_481; // 0x0784

	union {
		struct {
			unsigned y_lut_68:                10;
			unsigned y_lut_69:                10;
			unsigned y_lut_70:                10;
		} bit;
		UINT32 word;
	} reg_482; // 0x0788

	union {
		struct {
			unsigned y_lut_70:                10;
			unsigned y_lut_71:                10;
			unsigned y_lut_72:                10;
		} bit;
		UINT32 word;
	} reg_483; // 0x078c

	union {
		struct {
			unsigned y_lut_72:                10;
			unsigned y_lut_73:                10;
			unsigned y_lut_74:                10;
		} bit;
		UINT32 word;
	} reg_484; // 0x0790

	union {
		struct {
			unsigned y_lut_74:                10;
			unsigned y_lut_75:                10;
			unsigned y_lut_76:                10;
		} bit;
		UINT32 word;
	} reg_485; // 0x0794

	union {
		struct {
			unsigned y_lut_76:                10;
			unsigned y_lut_77:                10;
			unsigned y_lut_78:                10;
		} bit;
		UINT32 word;
	} reg_486; // 0x0798

	union {
		struct {
			unsigned y_lut_78:                10;
			unsigned y_lut_79:                10;
			unsigned y_lut_80:                10;
		} bit;
		UINT32 word;
	} reg_487; // 0x079c

	union {
		struct {
			unsigned y_lut_80:                10;
			unsigned y_lut_81:                10;
			unsigned y_lut_82:                10;
		} bit;
		UINT32 word;
	} reg_488; // 0x07a0

	union {
		struct {
			unsigned y_lut_82:                10;
			unsigned y_lut_83:                10;
			unsigned y_lut_84:                10;
		} bit;
		UINT32 word;
	} reg_489; // 0x07a4

	union {
		struct {
			unsigned y_lut_84:                10;
			unsigned y_lut_85:                10;
			unsigned y_lut_86:                10;
		} bit;
		UINT32 word;
	} reg_490; // 0x07a8

	union {
		struct {
			unsigned y_lut_86:                10;
			unsigned y_lut_87:                10;
			unsigned y_lut_88:                10;
		} bit;
		UINT32 word;
	} reg_491; // 0x07ac

	union {
		struct {
			unsigned y_lut_88:                10;
			unsigned y_lut_89:                10;
			unsigned y_lut_90:                10;
		} bit;
		UINT32 word;
	} reg_492; // 0x07b0

	union {
		struct {
			unsigned y_lut_90:                10;
			unsigned y_lut_91:                10;
			unsigned y_lut_92:                10;
		} bit;
		UINT32 word;
	} reg_493; // 0x07b4

	union {
		struct {
			unsigned y_lut_92:                10;
			unsigned y_lut_93:                10;
			unsigned y_lut_94:                10;
		} bit;
		UINT32 word;
	} reg_494; // 0x07b8

	union {
		struct {
			unsigned y_lut_94:                10;
			unsigned y_lut_95:                10;
			unsigned y_lut_96:                10;
		} bit;
		UINT32 word;
	} reg_495; // 0x07bc

	union {
		struct {
			unsigned y_lut_96:                10;
			unsigned y_lut_97:                10;
			unsigned y_lut_98:                10;
		} bit;
		UINT32 word;
	} reg_496; // 0x07c0

	union {
		struct {
			unsigned y_lut_98:                10;
			unsigned y_lut_99:                10;
			unsigned y_lut_100:                10;
		} bit;
		UINT32 word;
	} reg_497; // 0x07c4

	union {
		struct {
			unsigned y_lut_100:                10;
			unsigned y_lut_101:                10;
			unsigned y_lut_102:                10;
		} bit;
		UINT32 word;
	} reg_498; // 0x07c8

	union {
		struct {
			unsigned y_lut_102:                10;
			unsigned y_lut_103:                10;
			unsigned y_lut_104:                10;
		} bit;
		UINT32 word;
	} reg_499; // 0x07cc

	union {
		struct {
			unsigned y_lut_104:                10;
			unsigned y_lut_105:                10;
			unsigned y_lut_106:                10;
		} bit;
		UINT32 word;
	} reg_500; // 0x07d0

	union {
		struct {
			unsigned y_lut_106:                10;
			unsigned y_lut_107:                10;
			unsigned y_lut_108:                10;
		} bit;
		UINT32 word;
	} reg_501; // 0x07d4

	union {
		struct {
			unsigned y_lut_108:                10;
			unsigned y_lut_109:                10;
			unsigned y_lut_110:                10;
		} bit;
		UINT32 word;
	} reg_502; // 0x07d8

	union {
		struct {
			unsigned y_lut_110:                10;
			unsigned y_lut_111:                10;
			unsigned y_lut_112:                10;
		} bit;
		UINT32 word;
	} reg_503; // 0x07dc

	union {
		struct {
			unsigned y_lut_112:                10;
			unsigned y_lut_113:                10;
			unsigned y_lut_114:                10;
		} bit;
		UINT32 word;
	} reg_504; // 0x07e0

	union {
		struct {
			unsigned y_lut_114:                10;
			unsigned y_lut_115:                10;
			unsigned y_lut_116:                10;
		} bit;
		UINT32 word;
	} reg_505; // 0x07e4

	union {
		struct {
			unsigned y_lut_116:                10;
			unsigned y_lut_117:                10;
			unsigned y_lut_118:                10;
		} bit;
		UINT32 word;
	} reg_506; // 0x07e8

	union {
		struct {
			unsigned y_lut_118:                10;
			unsigned y_lut_119:                10;
			unsigned y_lut_120:                10;
		} bit;
		UINT32 word;
	} reg_507; // 0x07ec

	union {
		struct {
			unsigned y_lut_120:                10;
			unsigned y_lut_121:                10;
			unsigned y_lut_122:                10;
		} bit;
		UINT32 word;
	} reg_508; // 0x07f0

	union {
		struct {
			unsigned y_lut_122:                10;
			unsigned y_lut_123:                10;
			unsigned y_lut_124:                10;
		} bit;
		UINT32 word;
	} reg_509; // 0x07f4

	union {
		struct {
			unsigned y_lut_124:                10;
			unsigned y_lut_125:                10;
			unsigned y_lut_126:                10;
		} bit;
		UINT32 word;
	} reg_510; // 0x07f8

	union {
		struct {
			unsigned y_lut_126:                10;
			unsigned y_lut_127:                10;
			unsigned y_lut_128:                10;
		} bit;
		UINT32 word;
	} reg_511; // 0x07fc

	union {
		struct {
			unsigned edge_lut_0:                10;
			unsigned edge_lut_1:                10;
			unsigned edge_lut_2:                10;
		} bit;
		UINT32 word;
	} reg_512; // 0x0800

	union {
		struct {
			unsigned edge_lut_3:                10;
			unsigned edge_lut_4:                10;
			unsigned edge_lut_5:                10;
		} bit;
		UINT32 word;
	} reg_513; // 0x0804

	union {
		struct {
			unsigned edge_lut_6:                10;
			unsigned edge_lut_7:                10;
			unsigned edge_lut_8:                10;
		} bit;
		UINT32 word;
	} reg_514; // 0x0808

	union {
		struct {
			unsigned edge_lut_9:                10;
			unsigned edge_lut_10:                10;
			unsigned edge_lut_11:                10;
		} bit;
		UINT32 word;
	} reg_515; // 0x080c

	union {
		struct {
			unsigned edge_lut_12:                10;
			unsigned edge_lut_13:                10;
			unsigned edge_lut_14:                10;
		} bit;
		UINT32 word;
	} reg_516; // 0x0810

	union {
		struct {
			unsigned edge_lut_15:                10;
			unsigned edge_lut_16:                10;
			unsigned edge_lut_17:                10;
		} bit;
		UINT32 word;
	} reg_517; // 0x0814

	union {
		struct {
			unsigned edge_lut_18:                10;
			unsigned edge_lut_19:                10;
			unsigned edge_lut_20:                10;
		} bit;
		UINT32 word;
	} reg_518; // 0x0818

	union {
		struct {
			unsigned edge_lut_21:                10;
			unsigned edge_lut_22:                10;
			unsigned edge_lut_23:                10;
		} bit;
		UINT32 word;
	} reg_519; // 0x081c

	union {
		struct {
			unsigned edge_lut_24:                10;
			unsigned edge_lut_25:                10;
			unsigned edge_lut_26:                10;
		} bit;
		UINT32 word;
	} reg_520; // 0x0820

	union {
		struct {
			unsigned edge_lut_27:                10;
			unsigned edge_lut_28:                10;
			unsigned edge_lut_29:                10;
		} bit;
		UINT32 word;
	} reg_521; // 0x0824

	union {
		struct {
			unsigned edge_lut_30:                10;
			unsigned edge_lut_31:                10;
			unsigned edge_lut_32:                10;
		} bit;
		UINT32 word;
	} reg_522; // 0x0828

	union {
		struct {
			unsigned edge_lut_33:                10;
			unsigned edge_lut_34:                10;
			unsigned edge_lut_35:                10;
		} bit;
		UINT32 word;
	} reg_523; // 0x082c

	union {
		struct {
			unsigned edge_lut_36:                10;
			unsigned edge_lut_37:                10;
			unsigned edge_lut_38:                10;
		} bit;
		UINT32 word;
	} reg_524; // 0x0830

	union {
		struct {
			unsigned edge_lut_39:                10;
			unsigned edge_lut_40:                10;
			unsigned edge_lut_41:                10;
		} bit;
		UINT32 word;
	} reg_525; // 0x0834

	union {
		struct {
			unsigned edge_lut_42:                10;
			unsigned edge_lut_43:                10;
			unsigned edge_lut_44:                10;
		} bit;
		UINT32 word;
	} reg_526; // 0x0838

	union {
		struct {
			unsigned edge_lut_45:                10;
			unsigned edge_lut_46:                10;
			unsigned edge_lut_47:                10;
		} bit;
		UINT32 word;
	} reg_527; // 0x083c

	union {
		struct {
			unsigned edge_lut_48:                10;
			unsigned edge_lut_49:                10;
			unsigned edge_lut_50:                10;
		} bit;
		UINT32 word;
	} reg_528; // 0x0840

	union {
		struct {
			unsigned edge_lut_51:                10;
			unsigned edge_lut_52:                10;
			unsigned edge_lut_53:                10;
		} bit;
		UINT32 word;
	} reg_529; // 0x0844

	union {
		struct {
			unsigned edge_lut_54:                10;
			unsigned edge_lut_55:                10;
			unsigned edge_lut_56:                10;
		} bit;
		UINT32 word;
	} reg_530; // 0x0848

	union {
		struct {
			unsigned edge_lut_57:                10;
			unsigned edge_lut_58:                10;
			unsigned edge_lut_59:                10;
		} bit;
		UINT32 word;
	} reg_531; // 0x084c

	union {
		struct {
			unsigned edge_lut_60:                10;
			unsigned edge_lut_61:                10;
			unsigned edge_lut_62:                10;
		} bit;
		UINT32 word;
	} reg_532; // 0x0850

	union {
		struct {
			unsigned edge_lut_63:                10;
			unsigned edge_lut_64:                10;
		} bit;
		UINT32 word;
	} reg_533; // 0x0854

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_534; // 0x0858

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_535; // 0x085c

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_536; // 0x0860

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_537; // 0x0864

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_538; // 0x0868

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_539; // 0x086c

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_540; // 0x0870

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_541; // 0x0874

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_542; // 0x0878

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_543; // 0x087c

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_544; // 0x0880

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_545; // 0x0884

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_546; // 0x0888

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_547; // 0x088c

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_548; // 0x0890

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_549; // 0x0894

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_550; // 0x0898

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_551; // 0x089c

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_552; // 0x08a0

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_553; // 0x08a4

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_554; // 0x08a8

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_555; // 0x08ac

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_556; // 0x08b0

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_557; // 0x08b4

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_558; // 0x08b8

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_559; // 0x08bc

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_560; // 0x08c0

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_561; // 0x08c4

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_562; // 0x08c8

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_563; // 0x08cc

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_564; // 0x08d0

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_565; // 0x08d4

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_566; // 0x08d8

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_567; // 0x08dc

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_568; // 0x08e0

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_569; // 0x08e4

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_570; // 0x08e8

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_571; // 0x08ec

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_572; // 0x08f0

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_573; // 0x08f4

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_574; // 0x08f8

	union {
		struct {
			unsigned reserved:                32;
		} bit;
		UINT32 word;
	} reg_575; // 0x08fc

} NT98520_IPE_REGISTER_STRUCT;



#endif


