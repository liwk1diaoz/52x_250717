

#ifndef _IFE2_REG_H_
#define _IFE2_REG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include    "ife2_platform.h"


#if 0
#ifdef __KERNEL__

#include "mach/rcw_macro.h"
#include "kwrap/type.h"
#include "ife2_int.h"

#include "ife2_lib.h"

extern UINT32 _IFE2_REGIOBASE;

#define IOADDR_IFE2_REG_BASE      _IFE2_REGIOBASE
#define IFE2_REG_ADDR(ofs)        (IOADDR_IFE2_REG_BASE+(ofs))
#define IFE2_SETREG(ofs, value)   iowrite32(value, (void*)(IOADDR_IFE2_REG_BASE + ofs))
#define IFE2_GETREG(ofs)          ioread32((void*)(IOADDR_IFE2_REG_BASE + ofs))

#else

#include "DrvCommon.h"

#define _IFE2_REG_BASE_ADDR       IOADDR_IFE2_REG_BASE // in IOAddress.h

#define IFE2_REG_ADDR(ofs)        (_IFE2_REG_BASE_ADDR+(ofs))
#define IFE2_SETREG(ofs, value)   OUTW((_IFE2_REG_BASE_ADDR + ofs), value)
#define IFE2_GETREG(ofs)          INW(_IFE2_REG_BASE_ADDR + ofs)

#endif // end #ifdef __KERNEL__

#endif

/*
    ife2_rst           :    [0x0, 0x1],         bits : 0
    ife2_start         :    [0x0, 0x1],         bits : 1
    ife2_start_load    :    [0x0, 0x1],         bits : 2
    IFE2_FRAMEEND_LOAD :    [0x0, 0x1],         bits : 3
    ife2_drt_start_load:    [0x0, 0x1],         bits : 4
    ife2_ll_fire       :    [0x0, 0x1],         bits : 28
    ife2_ll_terminate  :    [0x0, 0x1],         bits : 29
*/
#define IFE2_ENGINE_CONTROL_REGISTER_OFS 0x0000
REGDEF_BEGIN(IFE2_ENGINE_CONTROL_REGISTER)
REGDEF_BIT(ife2_rst,        1)
REGDEF_BIT(ife2_start,        1)
REGDEF_BIT(ife2_start_aload,        1)
REGDEF_BIT(ife2_frameend_aload,        1)
REGDEF_BIT(ife2_drt_start_load,        1)
REGDEF_BIT(ife2_frm_start_reset,       1)
REGDEF_BIT(,        22)
REGDEF_BIT(ife2_ll_fire,        1)
REGDEF_END(IFE2_ENGINE_CONTROL_REGISTER)


/*
    ife2_ifmt         :    [0x0, 0x3],          bits : 1_0
    ife2_ofmt         :    [0x0, 0x3],          bits : 3_2
    ife2_dest         :    [0x0, 0x3],          bits : 5_4
    ife2_yftr_en      :    [0x0, 0x1],          bits : 6
    ife2_dram_out_en  :    [0x0, 0x1],          bits : 7
    ife2_fsize_sel    :    [0x0, 0x3],          bits : 17_16
    ife2_eksize_sel   :    [0x0, 0x3],          bits : 21_20
    ife2_in_brt_lenth :    [0x0, 0x1],          bits : 28
    ife2_out_brt_lenth:    [0x0, 0x1],          bits : 30
*/
#define IFE2_INPUT_CONTROL_REGISTER0_OFS 0x0004
REGDEF_BEGIN(IFE2_INPUT_CONTROL_REGISTER0)
REGDEF_BIT(ife2_ifmt,        2)
REGDEF_BIT(ife2_ofmt,        2)
REGDEF_BIT(ife2_dest,        2)
REGDEF_BIT(ife2_yftr_en,        1)
REGDEF_BIT(ife2_dram_out_en,        1)
REGDEF_BIT(,        8)
REGDEF_BIT(ife2_fsize_sel,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ife2_eksize_sel,        2)
REGDEF_BIT(,        6)
REGDEF_BIT(ife2_in_brt_lenth,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(ife2_out_brt_lenth,        1)
REGDEF_END(IFE2_INPUT_CONTROL_REGISTER0)


/*
    IFE2_INTE_LL_END     :    [0x0, 0x1],           bits : 0
    IFE2_INTE_LL_ERR     :    [0x0, 0x1],           bits : 1
    IFE2_INTE_LL_RED_LATE:    [0x0, 0x1],           bits : 2
    IFE2_INTE_LL_JOB_END :    [0x0, 0x1],           bits : 3
    IFE2_INTE_OUT_OVF    :    [0x0, 0x1],           bits : 30
    IFE2_INTE_FRM_END    :    [0x0, 0x1],           bits : 31
*/
#define IFE2_INTERRUPT_ENABLE_REGISTER_OFS 0x0008
REGDEF_BEGIN(IFE2_INTERRUPT_ENABLE_REGISTER)
REGDEF_BIT(ife2_intpe_ll_end,        1)
REGDEF_BIT(ife2_intpe_ll_err,        1)
REGDEF_BIT(ife2_intpe_ll_red_late,        1)
REGDEF_BIT(ife2_intpe_ll_job_end,        1)
REGDEF_BIT(,        25)
REGDEF_BIT(ife2_intpe_frm_err,        1)
REGDEF_BIT(ife2_intpe_out_ovf,        1)
REGDEF_BIT(ife2_intpe_frm_end,        1)
REGDEF_END(IFE2_INTERRUPT_ENABLE_REGISTER)


/*
    IFE2_INTS_LL_END     :    [0x0, 0x1],           bits : 0
    IFE2_INTS_LL_ERR     :    [0x0, 0x1],           bits : 1
    IFE2_INTS_LL_RED_LATE:    [0x0, 0x1],           bits : 2
    IFE2_INTS_LL_JOB_END :    [0x0, 0x1],           bits : 3
    IFE2_INT_OUT_OVF     :    [0x0, 0x1],           bits : 30
    IFE2_INT_FRM_END     :    [0x0, 0x1],           bits : 31
*/
#define IFE2_INTERRUPT_STATUS_REGISTER_OFS 0x000c
REGDEF_BEGIN(IFE2_INTERRUPT_STATUS_REGISTER)
REGDEF_BIT(ife2_intps_ll_end,        1)
REGDEF_BIT(ife2_intps_ll_err,        1)
REGDEF_BIT(ife2_intps_ll_red_late,        1)
REGDEF_BIT(ife2_intps_ll_job_end,        1)
REGDEF_BIT(,        25)
REGDEF_BIT(ife2_intps_frm_err,        1)
REGDEF_BIT(ife2_intps_out_ovf,        1)
REGDEF_BIT(ife2_intps_frm_end,        1)
REGDEF_END(IFE2_INTERRUPT_STATUS_REGISTER)


/*
    IFE2_DRAM_OUT_SINGLE_EN:    [0x0, 0x1],         bits : 0
    IFE2_DRAM_OUT_MODE     :    [0x0, 0x1],         bits : 31
*/
#define IFE2_LINKED_LIST_REGISTER_OFS 0x0010
REGDEF_BEGIN(IFE2_LINKED_LIST_REGISTER)
REGDEF_BIT(ife2_ll_terminate,        1)
REGDEF_BIT(,        31)
REGDEF_END(IFE2_LINKED_LIST_REGISTER)


/*
    ife2_h_size:    [0x0, 0x3fff],          bits : 13_0
    ife2_v_size:    [0x0, 0x3fff],          bits : 29_16
*/
#define IFE2_INPUT_IMAGE_SIZE_REGISTER0_OFS 0x0014
REGDEF_BEGIN(IFE2_INPUT_IMAGE_SIZE_REGISTER0)
REGDEF_BIT(ife2_h_size,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ife2_v_size,        14)
REGDEF_END(IFE2_INPUT_IMAGE_SIZE_REGISTER0)


/*
    ife2_egd_en   :    [0x0, 0x1],          bits : 29
    ife2_rc_en    :    [0x0, 0x1],          bits : 30
    ife2_filter_en:    [0x0, 0x1],          bits : 31
*/
#define DEBUG_REGISTER0_OFS 0x0018
REGDEF_BEGIN(DEBUG_REGISTER0)
REGDEF_BIT(,        29)
REGDEF_BIT(ife2_egd_en,        1)
REGDEF_BIT(ife2_rc_en,        1)
REGDEF_BIT(ife2_filter_en,        1)
REGDEF_END(DEBUG_REGISTER0)


/*
    ife2_dram_y_ofsi:    [0x0, 0x3ffff],            bits : 19_2
*/
#define IFE2_DMA_INPUT_LINEOFFSET_REGISTER0_OFS 0x001c
REGDEF_BEGIN(IFE2_DMA_INPUT_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ife2_dram_y_ofsi,        18)
REGDEF_END(IFE2_DMA_INPUT_LINEOFFSET_REGISTER0)


/*
    ife2_dram_y_sai:    [0x0, 0xffffffff],          bits : 31_0
*/
#define IFE2_INPUT_DMA_ADDRESS_REGISTER0_OFS 0x0020
REGDEF_BEGIN(IFE2_INPUT_DMA_ADDRESS_REGISTER0)
REGDEF_BIT(ife2_dram_y_sai,        32)
REGDEF_END(IFE2_INPUT_DMA_ADDRESS_REGISTER0)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IFE2_RESERVED1_OFS 0x0024
REGDEF_BEGIN(IFE2_RESERVED1)
REGDEF_BIT(reserved,        32)
REGDEF_END(IFE2_RESERVED1)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IFE2_RESERVED2_OFS 0x0028
REGDEF_BEGIN(IFE2_RESERVED2)
REGDEF_BIT(reserved,        32)
REGDEF_END(IFE2_RESERVED2)


/*
    ife2_dram_ll_sai:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IFE2LINKED_LIST_INPUT_DMA_CHANNEL_REGISTER3_OFS 0x002c
REGDEF_BEGIN(IFE2LINKED_LIST_INPUT_DMA_CHANNEL_REGISTER3)
REGDEF_BIT(,        2)
REGDEF_BIT(ife2_dram_ll_sai,        30)
REGDEF_END(IFE2LINKED_LIST_INPUT_DMA_CHANNEL_REGISTER3)


/*
    ife2_dram_y_ofso:    [0x0, 0x3ffff],            bits : 19_2
*/
#define IFE2_DMA_OUTPUT_LINEOFFSET_REGISTER0_OFS 0x0030
REGDEF_BEGIN(IFE2_DMA_OUTPUT_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ife2_dram_y_ofso,        18)
REGDEF_END(IFE2_DMA_OUTPUT_LINEOFFSET_REGISTER0)


/*
    ife2_dram_y_sao:    [0x0, 0xffffffff],          bits : 31_0
*/
#define IFE2_OUTPUT_DMA_ADDRESS_REGISTER0_OFS 0x0034
REGDEF_BEGIN(IFE2_OUTPUT_DMA_ADDRESS_REGISTER0)
REGDEF_BIT(ife2_dram_y_sao,        32)
REGDEF_END(IFE2_OUTPUT_DMA_ADDRESS_REGISTER0)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IFE2_RESERVED3_OFS 0x0038
REGDEF_BEGIN(IFE2_RESERVED3)
REGDEF_BIT(reserved,        32)
REGDEF_END(IFE2_RESERVED3)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IFE2_RESERVED4_OFS 0x003c
REGDEF_BEGIN(IFE2_RESERVED4)
REGDEF_BIT(reserved,        32)
REGDEF_END(IFE2_RESERVED4)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IFE2_RESERVED5_OFS 0x0040
REGDEF_BEGIN(IFE2_RESERVED5)
REGDEF_BIT(reserved,        32)
REGDEF_END(IFE2_RESERVED5)


/*
    ife2_y_rcth0:    [0x0, 0xff],           bits : 7_0
    ife2_y_rcth1:    [0x0, 0xff],           bits : 15_8
    ife2_y_rcth2:    [0x0, 0xff],           bits : 23_16
    ife2_y_cwt  :    [0x0, 0x1f],           bits : 28_24
*/
#define IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER0_OFS 0x0044
REGDEF_BEGIN(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER0)
REGDEF_BIT(ife2_y_rcth0,        8)
REGDEF_BIT(ife2_y_rcth1,        8)
REGDEF_BIT(ife2_y_rcth2,        8)
REGDEF_BIT(ife2_y_cwt,        5)
REGDEF_END(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER0)


/*
    ife2_y_rcwt0  :    [0x0, 0xf],          bits : 3_0
    IFE2_Y_ RCWT1 :    [0x0, 0xf],          bits : 7_4
    IFE2_Y_ RCWT2 :    [0x0, 0xf],          bits : 11_8
    IFE2_Y_ RCWT3 :    [0x0, 0xf],          bits : 15_12
    ife2_y_outl_th:    [0x0, 0x7],          bits : 18_16
*/
#define IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER1_OFS 0x0048
REGDEF_BEGIN(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER1)
REGDEF_BIT(ife2_y_rcwt0,        4)
REGDEF_BIT(ife2_y_rcwt1,        4)
REGDEF_BIT(ife2_y_rcwt2,        4)
REGDEF_BIT(ife2_y_rcwt3,        4)
REGDEF_BIT(ife2_y_outl_th,        3)
REGDEF_END(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER1)


/*
    ife2_uv_rcth0:    [0x0, 0xff],          bits : 7_0
    ife2_uv_rcth1:    [0x0, 0xff],          bits : 15_8
    ife2_uv_rcth2:    [0x0, 0xff],          bits : 23_16
    ife2_uv_cwt  :    [0x0, 0x1f],          bits : 28_24
*/
#define IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER2_OFS 0x004c
REGDEF_BEGIN(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER2)
REGDEF_BIT(ife2_uv_rcth0,        8)
REGDEF_BIT(ife2_uv_rcth1,        8)
REGDEF_BIT(ife2_uv_rcth2,        8)
REGDEF_BIT(ife2_uv_cwt,        5)
REGDEF_END(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER2)


/*
    ife2_uv_rcwt0  :    [0x0, 0xf],         bits : 3_0
    IFE2_UV_ RCWT1 :    [0x0, 0xf],         bits : 7_4
    IFE2_UV_ RCWT2 :    [0x0, 0xf],         bits : 11_8
    IFE2_UV_ RCWT3 :    [0x0, 0xf],         bits : 15_12
    ife2_uv_outl_th:    [0x0, 0x7],         bits : 18_16
*/
#define IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER3_OFS 0x0050
REGDEF_BEGIN(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER3)
REGDEF_BIT(ife2_uv_rcwt0,        4)
REGDEF_BIT(ife2_uv_rcwt1,        4)
REGDEF_BIT(ife2_uv_rcwt2,        4)
REGDEF_BIT(ife2_uv_rcwt3,        4)
REGDEF_BIT(ife2_uv_outl_th,        3)
REGDEF_END(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER3)


/*
    ife2_y_outl_dth:    [0x0, 0xff],            bits : 7_0
    ife2_u_outl_dth:    [0x0, 0xff],            bits : 15_8
    ife2_v_outl_dth:    [0x0, 0xff],            bits : 23_16
*/
#define IFE2_REFERENCE_CENTER_OUTLIER_DIFFERENCE_THRESHOLD_REGISTER0_OFS 0x0054
REGDEF_BEGIN(IFE2_REFERENCE_CENTER_OUTLIER_DIFFERENCE_THRESHOLD_REGISTER0)
REGDEF_BIT(ife2_y_outl_dth,        8)
REGDEF_BIT(ife2_u_outl_dth,        8)
REGDEF_BIT(ife2_v_outl_dth,        8)
REGDEF_END(IFE2_REFERENCE_CENTER_OUTLIER_DIFFERENCE_THRESHOLD_REGISTER0)


/*
    ife2_ed_pn_th:    [0x0, 0xff],          bits : 7_0
    ife2_ed_hv_th:    [0x0, 0xff],          bits : 15_8
*/
#define IFE2_EDGE_DIRECTION_THRESHOLD_REGISTER0_OFS 0x0058
REGDEF_BEGIN(IFE2_EDGE_DIRECTION_THRESHOLD_REGISTER0)
REGDEF_BIT(ife2_ed_pn_th,        8)
REGDEF_BIT(ife2_ed_hv_th,        8)
REGDEF_END(IFE2_EDGE_DIRECTION_THRESHOLD_REGISTER0)


/*
    ife2_y_fth0:    [0x0, 0xff],            bits : 7_0
    ife2_y_fth1:    [0x0, 0xff],            bits : 15_8
    ife2_y_fth2:    [0x0, 0xff],            bits : 23_16
    ife2_y_fth3:    [0x0, 0xff],            bits : 31_24
*/
#define IFE2_FILTER_COMPUTATION_REGISTER0_OFS 0x005c
REGDEF_BEGIN(IFE2_FILTER_COMPUTATION_REGISTER0)
REGDEF_BIT(ife2_y_fth0,        8)
REGDEF_BIT(ife2_y_fth1,        8)
REGDEF_BIT(ife2_y_fth2,        8)
REGDEF_BIT(ife2_y_fth3,        8)
REGDEF_END(IFE2_FILTER_COMPUTATION_REGISTER0)


/*
    ife2_y_fth4:    [0x0, 0xff],            bits : 7_0
*/
#define IFE2_FILTER_COMPUTATION_REGISTER1_OFS 0x0060
REGDEF_BEGIN(IFE2_FILTER_COMPUTATION_REGISTER1)
REGDEF_BIT(ife2_y_fth4,        8)
REGDEF_END(IFE2_FILTER_COMPUTATION_REGISTER1)


/*
    ife2_y_fwt0:    [0x0, 0x1f],            bits : 4_0
    ife2_y_fwt1:    [0x0, 0x1f],            bits : 9_5
    ife2_y_fwt2:    [0x0, 0x1f],            bits : 14_10
    ife2_y_fwt3:    [0x0, 0x1f],            bits : 19_15
    ife2_y_fwt4:    [0x0, 0x1f],            bits : 24_20
    ife2_y_fwt5:    [0x0, 0x1f],            bits : 29_25
*/
#define IFE2_FILTER_COMPUTATION_REGISTER2_OFS 0x0064
REGDEF_BEGIN(IFE2_FILTER_COMPUTATION_REGISTER2)
REGDEF_BIT(ife2_y_fwt0,        5)
REGDEF_BIT(ife2_y_fwt1,        5)
REGDEF_BIT(ife2_y_fwt2,        5)
REGDEF_BIT(ife2_y_fwt3,        5)
REGDEF_BIT(ife2_y_fwt4,        5)
REGDEF_BIT(ife2_y_fwt5,        5)
REGDEF_END(IFE2_FILTER_COMPUTATION_REGISTER2)


/*
    ife2_u_fth0:    [0x0, 0xff],            bits : 7_0
    ife2_u_fth1:    [0x0, 0xff],            bits : 15_8
    ife2_u_fth2:    [0x0, 0xff],            bits : 23_16
    ife2_u_fth3:    [0x0, 0xff],            bits : 31_24
*/
#define IFE2_FILTER_COMPUTATION_REGISTER3_OFS 0x0068
REGDEF_BEGIN(IFE2_FILTER_COMPUTATION_REGISTER3)
REGDEF_BIT(ife2_u_fth0,        8)
REGDEF_BIT(ife2_u_fth1,        8)
REGDEF_BIT(ife2_u_fth2,        8)
REGDEF_BIT(ife2_u_fth3,        8)
REGDEF_END(IFE2_FILTER_COMPUTATION_REGISTER3)


/*
    ife2_u_fth4:    [0x0, 0xff],            bits : 7_0
*/
#define IFE2_FILTER_COMPUTATION_REGISTER4_OFS 0x006c
REGDEF_BEGIN(IFE2_FILTER_COMPUTATION_REGISTER4)
REGDEF_BIT(ife2_u_fth4,        8)
REGDEF_END(IFE2_FILTER_COMPUTATION_REGISTER4)


/*
    ife2_u_fwt0:    [0x0, 0x1f],            bits : 4_0
    ife2_u_fwt1:    [0x0, 0x1f],            bits : 9_5
    ife2_u_fwt2:    [0x0, 0x1f],            bits : 14_10
    ife2_u_fwt3:    [0x0, 0x1f],            bits : 19_15
    ife2_u_fwt4:    [0x0, 0x1f],            bits : 24_20
    ife2_u_fwt5:    [0x0, 0x1f],            bits : 29_25
*/
#define IFE2_FILTER_COMPUTATION_REGISTER5_OFS 0x0070
REGDEF_BEGIN(IFE2_FILTER_COMPUTATION_REGISTER5)
REGDEF_BIT(ife2_u_fwt0,        5)
REGDEF_BIT(ife2_u_fwt1,        5)
REGDEF_BIT(ife2_u_fwt2,        5)
REGDEF_BIT(ife2_u_fwt3,        5)
REGDEF_BIT(ife2_u_fwt4,        5)
REGDEF_BIT(ife2_u_fwt5,        5)
REGDEF_END(IFE2_FILTER_COMPUTATION_REGISTER5)


/*
    ife2_v_fth0:    [0x0, 0xff],            bits : 7_0
    ife2_v_fth1:    [0x0, 0xff],            bits : 15_8
    ife2_v_fth2:    [0x0, 0xff],            bits : 23_16
    ife2_v_fth3:    [0x0, 0xff],            bits : 31_24
*/
#define IFE2_FILTER_COMPUTATION_REGISTER6_OFS 0x0074
REGDEF_BEGIN(IFE2_FILTER_COMPUTATION_REGISTER6)
REGDEF_BIT(ife2_v_fth0,        8)
REGDEF_BIT(ife2_v_fth1,        8)
REGDEF_BIT(ife2_v_fth2,        8)
REGDEF_BIT(ife2_v_fth3,        8)
REGDEF_END(IFE2_FILTER_COMPUTATION_REGISTER6)


/*
    ife2_v_fth4:    [0x0, 0xff],            bits : 7_0
*/
#define IFE2_FILTER_COMPUTATION_REGISTER7_OFS 0x0078
REGDEF_BEGIN(IFE2_FILTER_COMPUTATION_REGISTER7)
REGDEF_BIT(ife2_v_fth4,        8)
REGDEF_END(IFE2_FILTER_COMPUTATION_REGISTER7)


/*
    ife2_v_fwt0:    [0x0, 0x1f],            bits : 4_0
    ife2_v_fwt1:    [0x0, 0x1f],            bits : 9_5
    ife2_v_fwt2:    [0x0, 0x1f],            bits : 14_10
    ife2_v_fwt3:    [0x0, 0x1f],            bits : 19_15
    ife2_v_fwt4:    [0x0, 0x1f],            bits : 24_20
    ife2_v_fwt5:    [0x0, 0x1f],            bits : 29_25
*/
#define IFE2_FILTER_COMPUTATION_REGISTER8_OFS 0x007c
REGDEF_BEGIN(IFE2_FILTER_COMPUTATION_REGISTER8)
REGDEF_BIT(ife2_v_fwt0,        5)
REGDEF_BIT(ife2_v_fwt1,        5)
REGDEF_BIT(ife2_v_fwt2,        5)
REGDEF_BIT(ife2_v_fwt3,        5)
REGDEF_BIT(ife2_v_fwt4,        5)
REGDEF_BIT(ife2_v_fwt5,        5)
REGDEF_END(IFE2_FILTER_COMPUTATION_REGISTER8)


/*
    ife2_stl_u_th0:    [0x0, 0xff],         bits : 7_0
    ife2_stl_u_th1:    [0x0, 0xff],         bits : 15_8
*/
#define IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0_OFS 0x0080
REGDEF_BEGIN(IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0)
REGDEF_BIT(ife2_stl_u_th0,        8)
REGDEF_BIT(ife2_stl_u_th1,        8)
REGDEF_END(IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0)


/*
    ife2_stl_u_sum0:    [0x0, 0x7ffff],         bits : 18_0
*/
#define IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1_OFS 0x0084
REGDEF_BEGIN(IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1)
REGDEF_BIT(ife2_stl_u_sum0,        19)
REGDEF_END(IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1)


/*
    ife2_stl_u_sum1:    [0x0, 0x3ffff],         bits : 17_0
*/
#define IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2_OFS 0x0088
REGDEF_BEGIN(IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2)
REGDEF_BIT(ife2_stl_u_sum1,        18)
REGDEF_END(IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2)


/*
    ife2_stl_v_th0:    [0x0, 0xff],         bits : 7_0
    ife2_stl_v_th1:    [0x0, 0xff],         bits : 15_8
*/
#define IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0_OFS 0x008c
REGDEF_BEGIN(IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0)
REGDEF_BIT(ife2_stl_v_th0,        8)
REGDEF_BIT(ife2_stl_v_th1,        8)
REGDEF_END(IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0)


/*
    ife2_stl_v_sum0:    [0x0, 0x7ffff],         bits : 18_0
*/
#define IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1_OFS 0x0090
REGDEF_BEGIN(IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1)
REGDEF_BIT(ife2_stl_v_sum0,        19)
REGDEF_END(IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1)


/*
    ife2_stl_v_sum1:    [0x0, 0x3ffff],         bits : 17_0
*/
#define IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2_OFS 0x0094
REGDEF_BEGIN(IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2)
REGDEF_BIT(ife2_stl_v_sum1,        18)
REGDEF_END(IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2)


/*
    ife2_stl_cnt:    [0x0, 0xfffffff],          bits : 27_0
*/
#define IFE2_GRAY_STATISTICAL_INFORMATION_COUNT_REGISTER_OFS 0x0098
REGDEF_BEGIN(IFE2_GRAY_STATISTICAL_INFORMATION_COUNT_REGISTER)
REGDEF_BIT(ife2_stl_cnt,        28)
REGDEF_END(IFE2_GRAY_STATISTICAL_INFORMATION_COUNT_REGISTER)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IFE2_RESERVED6_OFS 0x009c
REGDEF_BEGIN(IFE2_RESERVED6)
REGDEF_BIT(reserved,        32)
REGDEF_END(IFE2_RESERVED6)


/*
    ife2_ll_tab0:    [0x0, 0xff],           bits : 7_0
    ife2_ll_tab1:    [0x0, 0xff],           bits : 15_8
    ife2_ll_tab2:    [0x0, 0xff],           bits : 23_16
    ife2_ll_tab3:    [0x0, 0xff],           bits : 31_24
*/
#define IFE2_LINKED_LIST_TABLE_INDEX_REGISTER0_OFS 0x00a0
REGDEF_BEGIN(IFE2_LINKED_LIST_TABLE_INDEX_REGISTER0)
REGDEF_BIT(ife2_ll_tab0,        8)
REGDEF_BIT(ife2_ll_tab1,        8)
REGDEF_BIT(ife2_ll_tab2,        8)
REGDEF_BIT(ife2_ll_tab3,        8)
REGDEF_END(IFE2_LINKED_LIST_TABLE_INDEX_REGISTER0)


/*
    ife2_ll_tab4:    [0x0, 0xff],           bits : 7_0
    ife2_ll_tab5:    [0x0, 0xff],           bits : 15_8
    ife2_ll_tab6:    [0x0, 0xff],           bits : 23_16
    ife2_ll_tab7:    [0x0, 0xff],           bits : 31_24
*/
#define IFE2_LINKED_LIST_TABLE_INDEX_REGISTER1_OFS 0x00a4
REGDEF_BEGIN(IFE2_LINKED_LIST_TABLE_INDEX_REGISTER1)
REGDEF_BIT(ife2_ll_tab4,        8)
REGDEF_BIT(ife2_ll_tab5,        8)
REGDEF_BIT(ife2_ll_tab6,        8)
REGDEF_BIT(ife2_ll_tab7,        8)
REGDEF_END(IFE2_LINKED_LIST_TABLE_INDEX_REGISTER1)


/*
    ife2_ll_tab8 :    [0x0, 0xff],          bits : 7_0
    ife2_ll_tab9 :    [0x0, 0xff],          bits : 15_8
    ife2_ll_tab10:    [0x0, 0xff],          bits : 23_16
    ife2_ll_tab11:    [0x0, 0xff],          bits : 31_24
*/
#define IFE2_LINKED_LIST_TABLE_INDEX_REGISTER2_OFS 0x00a8
REGDEF_BEGIN(IFE2_LINKED_LIST_TABLE_INDEX_REGISTER2)
REGDEF_BIT(ife2_ll_tab8,        8)
REGDEF_BIT(ife2_ll_tab9,        8)
REGDEF_BIT(ife2_ll_tab10,        8)
REGDEF_BIT(ife2_ll_tab11,        8)
REGDEF_END(IFE2_LINKED_LIST_TABLE_INDEX_REGISTER2)


/*
    ife2_ll_tab12:    [0x0, 0xff],          bits : 7_0
    ife2_ll_tab13:    [0x0, 0xff],          bits : 15_8
    ife2_ll_tab14:    [0x0, 0xff],          bits : 23_16
    ife2_ll_tab15:    [0x0, 0xff],          bits : 31_24
*/
#define IFE2_LINKED_LIST_TABLE_INDEX_REGISTER3_OFS 0x00ac
REGDEF_BEGIN(IFE2_LINKED_LIST_TABLE_INDEX_REGISTER3)
REGDEF_BIT(ife2_ll_tab12,        8)
REGDEF_BIT(ife2_ll_tab13,        8)
REGDEF_BIT(ife2_ll_tab14,        8)
REGDEF_BIT(ife2_ll_tab15,        8)
REGDEF_END(IFE2_LINKED_LIST_TABLE_INDEX_REGISTER3)

//---------------------------------------------------------------

typedef struct {

	union {
		struct {
			unsigned ife2_rst                   : 1;      // bits : 0
			unsigned ife2_start                 : 1;      // bits : 1
			unsigned ife2_start_aload            : 1;      // bits : 2
			unsigned ife2_frameend_aload         : 1;      // bits : 3
			unsigned ife2_drt_start_aload        : 1;      // bits : 4
			unsigned ife2_frm_start_reset       : 1;      // bits : 5
			unsigned                            : 22;
			unsigned ife2_ll_fire               : 1;      // bits : 28
		} bit;
		UINT32 word;
	} reg_0; // 0x0000

	union {
		struct {
			unsigned ife2_ifmt                 : 2;       // bits : 1_0
			unsigned ife2_ofmt                 : 2;       // bits : 3_2
			unsigned ife2_dest                 : 2;       // bits : 5_4
			unsigned ife2_yftr_en              : 1;       // bits : 6
			unsigned ife2_dram_out_en          : 1;       // bits : 7
			unsigned                           : 8;
			unsigned ife2_fsize_sel            : 2;       // bits : 17_16
			unsigned                           : 2;
			unsigned ife2_eksize_sel           : 2;       // bits : 21_20
			unsigned                           : 6;
			unsigned ife2_in_brt_lenth         : 1;       // bits : 28
			unsigned                           : 1;
			unsigned ife2_out_brt_lenth        : 1;       // bits : 30
		} bit;
		UINT32 word;
	} reg_1; // 0x0004

	union {
		struct {
			unsigned ife2_intpe_ll_end             : 1;        // bits : 0
			unsigned ife2_intpe_ll_err             : 1;        // bits : 1
			unsigned ife2_intpe_ll_red_late        : 1;        // bits : 2
			unsigned ife2_intpe_ll_job_end         : 1;        // bits : 3
			unsigned                              : 25;
			unsigned ife2_intpe_frm_err            : 1;        // bits : 29
			unsigned ife2_intpe_out_ovf            : 1;        // bits : 30
			unsigned ife2_intpe_frm_end            : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_2; // 0x0008

	union {
		struct {
			unsigned ife2_intps_ll_end             : 1;        // bits : 0
			unsigned ife2_intps_ll_err             : 1;        // bits : 1
			unsigned ife2_intps_ll_red_late        : 1;        // bits : 2
			unsigned ife2_intps_ll_job_end         : 1;        // bits : 3
			unsigned                              : 25;
			unsigned ife2_intps_frm_err             : 1;        // bits : 29
			unsigned ife2_intps_out_ovf             : 1;        // bits : 30
			unsigned ife2_intps_frm_end             : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_3; // 0x000c

	union {
		struct {
			unsigned ife2_ll_terminate              : 1;      // bits : 0
			unsigned                                : 30;
			unsigned ife2_dram_out_mode             : 1;      // bits : 31
		} bit;
		UINT32 word;
	} reg_4; // 0x0010

	union {
		struct {
			unsigned ife2_h_size        : 14;      // bits : 13_0
			unsigned                       : 2;
			unsigned ife2_v_size        : 14;      // bits : 29_16
		} bit;
		UINT32 word;
	} reg_5; // 0x0014

	union {
		struct {
			unsigned                       : 29;
			unsigned ife2_egd_en           : 1;       // bits : 29
			unsigned ife2_rc_en            : 1;       // bits : 30
			unsigned ife2_filter_en        : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_6; // 0x0018

	union {
		struct {
			unsigned                         : 2;
			unsigned ife2_dram_y_ofsi        : 18;        // bits : 19_2
		} bit;
		UINT32 word;
	} reg_7; // 0x001c

	union {
		struct {
			unsigned ife2_dram_y_sai        : 32;     // bits : 31_0
		} bit;
		UINT32 word;
	} reg_8; // 0x0020

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_9; // 0x0024

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_10; // 0x0028

	union {
		struct {
			unsigned                         : 2;
			unsigned ife2_dram_ll_sai        : 30;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_11; // 0x002c

	union {
		struct {
			unsigned                         : 2;
			unsigned ife2_dram_y_ofso        : 18;        // bits : 19_2
		} bit;
		UINT32 word;
	} reg_12; // 0x0030

	union {
		struct {
			unsigned ife2_dram_y_sao        : 32;     // bits : 31_0
		} bit;
		UINT32 word;
	} reg_13; // 0x0034

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_14; // 0x0038

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_15; // 0x003c

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_16; // 0x0040

	union {
		struct {
			unsigned ife2_y_rcth0        : 8;     // bits : 7_0
			unsigned ife2_y_rcth1        : 8;     // bits : 15_8
			unsigned ife2_y_rcth2        : 8;     // bits : 23_16
			unsigned ife2_y_cwt          : 5;     // bits : 28_24
		} bit;
		UINT32 word;
	} reg_17; // 0x0044

	union {
		struct {
			unsigned ife2_y_rcwt0          : 4;       // bits : 3_0
			unsigned ife2_y_rcwt1         : 4;       // bits : 7_4
			unsigned ife2_y_rcwt2         : 4;       // bits : 11_8
			unsigned ife2_y_rcwt3         : 4;       // bits : 15_12
			unsigned ife2_y_outl_th        : 3;       // bits : 18_16
		} bit;
		UINT32 word;
	} reg_18; // 0x0048

	union {
		struct {
			unsigned ife2_uv_rcth0        : 8;        // bits : 7_0
			unsigned ife2_uv_rcth1        : 8;        // bits : 15_8
			unsigned ife2_uv_rcth2        : 8;        // bits : 23_16
			unsigned ife2_uv_cwt          : 5;        // bits : 28_24
		} bit;
		UINT32 word;
	} reg_19; // 0x004c

	union {
		struct {
			unsigned ife2_uv_rcwt0          : 4;      // bits : 3_0
			unsigned ife2_uv_rcwt1         : 4;      // bits : 7_4
			unsigned ife2_uv_rcwt2         : 4;      // bits : 11_8
			unsigned ife2_uv_rcwt3         : 4;      // bits : 15_12
			unsigned ife2_uv_outl_th        : 3;      // bits : 18_16
		} bit;
		UINT32 word;
	} reg_20; // 0x0050

	union {
		struct {
			unsigned ife2_y_outl_dth        : 8;      // bits : 7_0
			unsigned ife2_u_outl_dth        : 8;      // bits : 15_8
			unsigned ife2_v_outl_dth        : 8;      // bits : 23_16
		} bit;
		UINT32 word;
	} reg_21; // 0x0054

	union {
		struct {
			unsigned ife2_ed_pn_th        : 8;        // bits : 7_0
			unsigned ife2_ed_hv_th        : 8;        // bits : 15_8
		} bit;
		UINT32 word;
	} reg_22; // 0x0058

	union {
		struct {
			unsigned ife2_y_fth0        : 8;      // bits : 7_0
			unsigned ife2_y_fth1        : 8;      // bits : 15_8
			unsigned ife2_y_fth2        : 8;      // bits : 23_16
			unsigned ife2_y_fth3        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_23; // 0x005c

	union {
		struct {
			unsigned ife2_y_fth4        : 8;      // bits : 7_0
		} bit;
		UINT32 word;
	} reg_24; // 0x0060

	union {
		struct {
			unsigned ife2_y_fwt0        : 5;      // bits : 4_0
			unsigned ife2_y_fwt1        : 5;      // bits : 9_5
			unsigned ife2_y_fwt2        : 5;      // bits : 14_10
			unsigned ife2_y_fwt3        : 5;      // bits : 19_15
			unsigned ife2_y_fwt4        : 5;      // bits : 24_20
			unsigned ife2_y_fwt5        : 5;      // bits : 29_25
		} bit;
		UINT32 word;
	} reg_25; // 0x0064

	union {
		struct {
			unsigned ife2_u_fth0        : 8;      // bits : 7_0
			unsigned ife2_u_fth1        : 8;      // bits : 15_8
			unsigned ife2_u_fth2        : 8;      // bits : 23_16
			unsigned ife2_u_fth3        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_26; // 0x0068

	union {
		struct {
			unsigned ife2_u_fth4        : 8;      // bits : 7_0
		} bit;
		UINT32 word;
	} reg_27; // 0x006c

	union {
		struct {
			unsigned ife2_u_fwt0        : 5;      // bits : 4_0
			unsigned ife2_u_fwt1        : 5;      // bits : 9_5
			unsigned ife2_u_fwt2        : 5;      // bits : 14_10
			unsigned ife2_u_fwt3        : 5;      // bits : 19_15
			unsigned ife2_u_fwt4        : 5;      // bits : 24_20
			unsigned ife2_u_fwt5        : 5;      // bits : 29_25
		} bit;
		UINT32 word;
	} reg_28; // 0x0070

	union {
		struct {
			unsigned ife2_v_fth0        : 8;      // bits : 7_0
			unsigned ife2_v_fth1        : 8;      // bits : 15_8
			unsigned ife2_v_fth2        : 8;      // bits : 23_16
			unsigned ife2_v_fth3        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_29; // 0x0074

	union {
		struct {
			unsigned ife2_v_fth4        : 8;      // bits : 7_0
		} bit;
		UINT32 word;
	} reg_30; // 0x0078

	union {
		struct {
			unsigned ife2_v_fwt0        : 5;      // bits : 4_0
			unsigned ife2_v_fwt1        : 5;      // bits : 9_5
			unsigned ife2_v_fwt2        : 5;      // bits : 14_10
			unsigned ife2_v_fwt3        : 5;      // bits : 19_15
			unsigned ife2_v_fwt4        : 5;      // bits : 24_20
			unsigned ife2_v_fwt5        : 5;      // bits : 29_25
		} bit;
		UINT32 word;
	} reg_31; // 0x007c

	union {
		struct {
			unsigned ife2_stl_u_th0        : 8;       // bits : 7_0
			unsigned ife2_stl_u_th1        : 8;       // bits : 15_8
		} bit;
		UINT32 word;
	} reg_32; // 0x0080

	union {
		struct {
			unsigned ife2_stl_u_sum0        : 19;     // bits : 18_0
		} bit;
		UINT32 word;
	} reg_33; // 0x0084

	union {
		struct {
			unsigned ife2_stl_u_sum1        : 18;     // bits : 17_0
		} bit;
		UINT32 word;
	} reg_34; // 0x0088

	union {
		struct {
			unsigned ife2_stl_v_th0        : 8;       // bits : 7_0
			unsigned ife2_stl_v_th1        : 8;       // bits : 15_8
		} bit;
		UINT32 word;
	} reg_35; // 0x008c

	union {
		struct {
			unsigned ife2_stl_v_sum0        : 19;     // bits : 18_0
		} bit;
		UINT32 word;
	} reg_36; // 0x0090

	union {
		struct {
			unsigned ife2_stl_v_sum1        : 18;     // bits : 17_0
		} bit;
		UINT32 word;
	} reg_37; // 0x0094

	union {
		struct {
			unsigned ife2_stl_cnt        : 28;        // bits : 27_0
		} bit;
		UINT32 word;
	} reg_38; // 0x0098

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_39; // 0x009c

	union {
		struct {
			unsigned ife2_ll_tab0        : 8;     // bits : 7_0
			unsigned ife2_ll_tab1        : 8;     // bits : 15_8
			unsigned ife2_ll_tab2        : 8;     // bits : 23_16
			unsigned ife2_ll_tab3        : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_40; // 0x00a0

	union {
		struct {
			unsigned ife2_ll_tab4        : 8;     // bits : 7_0
			unsigned ife2_ll_tab5        : 8;     // bits : 15_8
			unsigned ife2_ll_tab6        : 8;     // bits : 23_16
			unsigned ife2_ll_tab7        : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_41; // 0x00a4

	union {
		struct {
			unsigned ife2_ll_tab8         : 8;        // bits : 7_0
			unsigned ife2_ll_tab9         : 8;        // bits : 15_8
			unsigned ife2_ll_tab10        : 8;        // bits : 23_16
			unsigned ife2_ll_tab11        : 8;        // bits : 31_24
		} bit;
		UINT32 word;
	} reg_42; // 0x00a8

	union {
		struct {
			unsigned ife2_ll_tab12        : 8;        // bits : 7_0
			unsigned ife2_ll_tab13        : 8;        // bits : 15_8
			unsigned ife2_ll_tab14        : 8;        // bits : 23_16
			unsigned ife2_ll_tab15        : 8;        // bits : 31_24
		} bit;
		UINT32 word;
	} reg_43; // 0x00ac

} NT98520_IFE2_REGISTER_STRUCT;



#ifdef __cplusplus
}
#endif


#endif  // #ifndef _IFE2_NT96680_H_


