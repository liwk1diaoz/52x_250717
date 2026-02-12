#ifndef _ISE_ENG_INT_REG_H_
#define _ISE_ENG_INT_REG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcw_macro.h"

/*
    ise_rst         :    [0x0, 0x1],            bits : 0
    ise_start       :    [0x0, 0x1],            bits : 1
    ise_ll_fire     :    [0x0, 0x1],            bits : 28
*/
#define ISE_ENGINE_CONTROL_REGISTER_OFS 0x0000
REGDEF_BEGIN(ISE_ENGINE_CONTROL_REGISTER)
REGDEF_BIT(ise_rst,        1)
REGDEF_BIT(ise_start,        1)
REGDEF_BIT(,        26)
REGDEF_BIT(ise_ll_fire,        1)
REGDEF_END(ISE_ENGINE_CONTROL_REGISTER)


/*
    ise_in_fmt       :    [0x0, 0xf],           bits : 3_0
    ISE_OUT_FMT      :    [0x0, 0xf],           bits : 7_4
    ise_scl_method   :    [0x0, 0x3],           bits : 9_8
    ise_argb_outmode :    [0x0, 0x3],           bits : 13_12
    ise_in_brt_lenth :    [0x0, 0x1],           bits : 16
    ise_out_brt_lenth:    [0x0, 0x1],           bits : 17
    ise_ovlap_mode   :    [0x0, 0x1],           bits : 18
*/
#define ISE_INPUT_CONTROL_REGISTER0_OFS 0x0004
REGDEF_BEGIN(ISE_INPUT_CONTROL_REGISTER0)
REGDEF_BIT(ise_in_fmt,        4)
REGDEF_BIT(,        4)
REGDEF_BIT(ise_scl_method,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ise_argb_outmode,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ise_in_brt_lenth,        1)
REGDEF_BIT(ise_out_brt_lenth,        1)
REGDEF_BIT(ise_ovlap_mode,        1)
REGDEF_END(ISE_INPUT_CONTROL_REGISTER0)


/*
    ise_inte_ll_end :    [0x0, 0x1],            bits : 0
    ise_inte_ll_err :    [0x0, 0x1],            bits : 1
    ise_inte_frm_end:    [0x0, 0x1],            bits : 31
*/
#define ISE_INTERRUPT_ENABLE_REGISTER_OFS 0x0008
REGDEF_BEGIN(ISE_INTERRUPT_ENABLE_REGISTER)
REGDEF_BIT(ise_inte_ll_end,        1)
REGDEF_BIT(ise_inte_ll_err,        1)
REGDEF_BIT(,        29)
REGDEF_BIT(ise_inte_frm_end,        1)
REGDEF_END(ISE_INTERRUPT_ENABLE_REGISTER)


/*
    ise_ints_ll_end :    [0x0, 0x1],            bits : 0
    ise_ints_ll_err :    [0x0, 0x1],            bits : 1
    ise_ints_frm_end:    [0x0, 0x1],            bits : 31
*/
#define ISE_INTERRUPT_STATUS_REGISTER_OFS 0x000c
REGDEF_BEGIN(ISE_INTERRUPT_STATUS_REGISTER)
REGDEF_BIT(ise_ints_ll_end,        1)
REGDEF_BIT(ise_ints_ll_err,        1)
REGDEF_BIT(,        29)
REGDEF_BIT(ise_ints_frm_end,        1)
REGDEF_END(ISE_INTERRUPT_STATUS_REGISTER)


/*
    ise_h_size:    [0x0, 0xffff],           bits : 15_0
    ise_v_size:    [0x0, 0xffff],           bits : 31_16
*/
#define ISE_INPUT_IMAGE_SIZE_REGISTER0_OFS 0x0010
REGDEF_BEGIN(ISE_INPUT_IMAGE_SIZE_REGISTER0)
REGDEF_BIT(ise_h_size,        16)
REGDEF_BIT(ise_v_size,        16)
REGDEF_END(ISE_INPUT_IMAGE_SIZE_REGISTER0)


/*
    ise_st_size   :    [0x0, 0x1ff],            bits : 8_0
    ise_ovlap_size:    [0x0, 0x3],          bits : 17_16
*/
#define ISE_INPUT_STRIPE_SIZE_REGISTER0_OFS 0x0014
REGDEF_BEGIN(ISE_INPUT_STRIPE_SIZE_REGISTER0)
REGDEF_BIT(ise_st_size,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(ise_ovlap_size,        2)
REGDEF_END(ISE_INPUT_STRIPE_SIZE_REGISTER0)


/*
    ise_dram_ofsi:    [0x0, 0x3ffff],           bits : 19_2
*/
#define ISE_INPUT_DMA_LINEOFFSET_REGISTER0_OFS 0x0018
REGDEF_BEGIN(ISE_INPUT_DMA_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ise_dram_ofsi,        18)
REGDEF_END(ISE_INPUT_DMA_LINEOFFSET_REGISTER0)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RESERVED0_OFS 0x001c
REGDEF_BEGIN(RESERVED0)
REGDEF_BIT(reserved,        32)
REGDEF_END(RESERVED0)


/*
    ise_dram_sai:    [0x0, 0xffffffff],         bits : 31_0
*/
#define ISE_INPUT_DMA_CHANNEL_REGISTER0_OFS 0x0020
REGDEF_BEGIN(ISE_INPUT_DMA_CHANNEL_REGISTER0)
REGDEF_BIT(ise_dram_sai,        32)
REGDEF_END(ISE_INPUT_DMA_CHANNEL_REGISTER0)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RESERVED1_OFS 0x0024
REGDEF_BEGIN(RESERVED1)
REGDEF_BIT(reserved,        32)
REGDEF_END(RESERVED1)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define ISE_LINKED_LIST_CONTROL_REGISTER0_OFS 0x0028
REGDEF_BEGIN(ISE_LINKED_LIST_CONTROL_REGISTER0)
REGDEF_BIT(reserved,        29)
REGDEF_BIT(ise_ll_terminate,        1)
REGDEF_END(ISE_LINKED_LIST_CONTROL_REGISTER0)


/*
    ise_dram_ll_sai:    [0x0, 0xffffffff],          bits : 31_0
*/
#define ISE_INPUT_DMA_CHANNEL_REGISTER3_OFS 0x002c
REGDEF_BEGIN(ISE_INPUT_DMA_CHANNEL_REGISTER3)
REGDEF_BIT(ise_dram_ll_sai,        32)
REGDEF_END(ISE_INPUT_DMA_CHANNEL_REGISTER3)


/*
    h_ud      :    [0x0, 0x1],          bits : 0
    v_ud      :    [0x0, 0x1],          bits : 1
    h_dnrate  :    [0x0, 0x1f],         bits : 6_2
    v_dnrate  :    [0x0, 0x1f],         bits : 11_7
    h_filtmode:    [0x0, 0x1],          bits : 15
    h_filtcoef:    [0x0, 0x3f],         bits : 21_16
    v_filtmode:    [0x0, 0x1],          bits : 22
    v_filtcoef:    [0x0, 0x3f],         bits : 28_23
*/
#define ISE_OUTPUT_CONTROL_REGISTER0_OFS 0x0030
REGDEF_BEGIN(ISE_OUTPUT_CONTROL_REGISTER0)
REGDEF_BIT(h_ud,        1)
REGDEF_BIT(v_ud,        1)
REGDEF_BIT(h_dnrate,        5)
REGDEF_BIT(v_dnrate,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(h_filtmode,        1)
REGDEF_BIT(h_filtcoef,        6)
REGDEF_BIT(v_filtmode,        1)
REGDEF_BIT(v_filtcoef,        6)
REGDEF_END(ISE_OUTPUT_CONTROL_REGISTER0)


/*
    h_sfact:    [0x0, 0xffff],          bits : 15_0
    v_sfact:    [0x0, 0xffff],          bits : 31_16
*/
#define ISE_OUTPUT_CONTROL_REGISTER1_OFS 0x0034
REGDEF_BEGIN(ISE_OUTPUT_CONTROL_REGISTER1)
REGDEF_BIT(h_sfact,        16)
REGDEF_BIT(v_sfact,        16)
REGDEF_END(ISE_OUTPUT_CONTROL_REGISTER1)


/*
    h_osize:    [0x0, 0xffff],          bits : 15_0
    v_osize:    [0x0, 0xffff],          bits : 31_16
*/
#define ISE_OUTPUT_CONTROL_REGISTER2_OFS 0x0038
REGDEF_BEGIN(ISE_OUTPUT_CONTROL_REGISTER2)
REGDEF_BIT(h_osize,        16)
REGDEF_BIT(v_osize,        16)
REGDEF_END(ISE_OUTPUT_CONTROL_REGISTER2)


/*
    ise_hsft_int:    [0x0, 0xffff],         bits : 15_0
    ise_vsft_int:    [0x0, 0xffff],         bits : 31_16
*/
#define ISE_SCALING_START_OFFSET_REGISTER0_OFS 0x003c
REGDEF_BEGIN(ISE_SCALING_START_OFFSET_REGISTER0)
REGDEF_BIT(ise_hsft_int,        16)
REGDEF_BIT(ise_vsft_int,        16)
REGDEF_END(ISE_SCALING_START_OFFSET_REGISTER0)


/*
    ise_hsft_dec:    [0x0, 0xffff],         bits : 15_0
    ise_vsft_dec:    [0x0, 0xffff],         bits : 31_16
*/
#define ISE_SCALING_START_OFFSET_REGISTER1_OFS 0x0040
REGDEF_BEGIN(ISE_SCALING_START_OFFSET_REGISTER1)
REGDEF_BIT(ise_hsft_dec,        16)
REGDEF_BIT(ise_vsft_dec,        16)
REGDEF_END(ISE_SCALING_START_OFFSET_REGISTER1)


/*
    isd_h_base:    [0x0, 0x1fff],           bits : 12_0
    isd_v_base:    [0x0, 0x1fff],           bits : 28_16
*/
#define ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER3_OFS 0x0044
REGDEF_BEGIN(ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER3)
REGDEF_BIT(isd_h_base,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_v_base,        13)
REGDEF_END(ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER3)


/*
    isd_h_sfact0:    [0x0, 0x1fff],         bits : 12_0
    isd_v_sfact0:    [0x0, 0x1fff],         bits : 28_16
*/
#define ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER0_OFS 0x0048
REGDEF_BEGIN(ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER0)
REGDEF_BIT(isd_h_sfact0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_v_sfact0,        13)
REGDEF_END(ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER0)


/*
    isd_h_sfact1:    [0x0, 0x1fff],         bits : 12_0
    isd_v_sfact1:    [0x0, 0x1fff],         bits : 28_16
*/
#define ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER1_OFS 0x004c
REGDEF_BEGIN(ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER1)
REGDEF_BIT(isd_h_sfact1,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_v_sfact1,        13)
REGDEF_END(ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER1)


/*
    isd_h_sfact2:    [0x0, 0x1fff],         bits : 12_0
    isd_v_sfact2:    [0x0, 0x1fff],         bits : 28_16
*/
#define ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER2_OFS 0x0050
REGDEF_BEGIN(ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER2)
REGDEF_BIT(isd_h_sfact2,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_v_sfact2,        13)
REGDEF_END(ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER2)


/*
    ise_isd_mode      :    [0x0, 0x1],          bits : 0
    ise_isd_h_coef_num:    [0x0, 0x1f],         bits : 7_3
    ise_isd_v_coef_num:    [0x0, 0x1f],         bits : 12_8
*/
#define ISE_INTEGRATION_SCALE_USER_COEFFICIENTS_REGISTER0_OFS 0x0054
REGDEF_BEGIN(ISE_INTEGRATION_SCALE_USER_COEFFICIENTS_REGISTER0)
REGDEF_BIT(ise_isd_mode,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(ise_isd_h_coef_num,        5)
REGDEF_BIT(ise_isd_v_coef_num,        5)
REGDEF_END(ISE_INTEGRATION_SCALE_USER_COEFFICIENTS_REGISTER0)


/*
    ise_isd_h_coef0:    [0x0, 0xff],            bits : 7_0
    ise_isd_h_coef1:    [0x0, 0xff],            bits : 15_8
    ise_isd_h_coef2:    [0x0, 0xff],            bits : 23_16
    ise_isd_h_coef3:    [0x0, 0xff],            bits : 31_24
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER0_OFS 0x0058
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER0)
REGDEF_BIT(ise_isd_h_coef0,        8)
REGDEF_BIT(ise_isd_h_coef1,        8)
REGDEF_BIT(ise_isd_h_coef2,        8)
REGDEF_BIT(ise_isd_h_coef3,        8)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER0)


/*
    ise_isd_h_coef4:    [0x0, 0xff],            bits : 7_0
    ise_isd_h_coef5:    [0x0, 0xff],            bits : 15_8
    ise_isd_h_coef6:    [0x0, 0xff],            bits : 23_16
    ise_isd_h_coef7:    [0x0, 0xff],            bits : 31_24
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER1_OFS 0x005c
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER1)
REGDEF_BIT(ise_isd_h_coef4,        8)
REGDEF_BIT(ise_isd_h_coef5,        8)
REGDEF_BIT(ise_isd_h_coef6,        8)
REGDEF_BIT(ise_isd_h_coef7,        8)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER1)


/*
    ise_isd_h_coef8 :    [0x0, 0xff],           bits : 7_0
    ise_isd_h_coef9 :    [0x0, 0xff],           bits : 15_8
    ise_isd_h_coef10:    [0x0, 0xff],           bits : 23_16
    ise_isd_h_coef11:    [0x0, 0xff],           bits : 31_24
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER2_OFS 0x0060
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER2)
REGDEF_BIT(ise_isd_h_coef8,        8)
REGDEF_BIT(ise_isd_h_coef9,        8)
REGDEF_BIT(ise_isd_h_coef10,        8)
REGDEF_BIT(ise_isd_h_coef11,        8)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER2)


/*
    ise_isd_h_coef12:    [0x0, 0xff],           bits : 7_0
    ise_isd_h_coef13:    [0x0, 0xff],           bits : 15_8
    ise_isd_h_coef14:    [0x0, 0xff],           bits : 23_16
    ise_isd_h_coef15:    [0x0, 0xff],           bits : 31_24
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER3_OFS 0x0064
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER3)
REGDEF_BIT(ise_isd_h_coef12,        8)
REGDEF_BIT(ise_isd_h_coef13,        8)
REGDEF_BIT(ise_isd_h_coef14,        8)
REGDEF_BIT(ise_isd_h_coef15,        8)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER3)


/*
    ise_isd_h_coef16:    [0x0, 0xfff],          bits : 11_0
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER4_OFS 0x0068
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER4)
REGDEF_BIT(ise_isd_h_coef16,        12)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER4)


/*
    ise_isd_h_coef_msb0:    [0x0, 0xf],         bits : 3_0
    ise_isd_h_coef_msb1:    [0x0, 0xf],         bits : 7_4
    ise_isd_h_coef_msb2:    [0x0, 0xf],         bits : 11_8
    ise_isd_h_coef_msb3:    [0x0, 0xf],         bits : 15_12
    ise_isd_h_coef_msb4:    [0x0, 0xf],         bits : 19_16
    ise_isd_h_coef_msb5:    [0x0, 0xf],         bits : 23_20
    ise_isd_h_coef_msb6:    [0x0, 0xf],         bits : 27_24
    ise_isd_h_coef_msb7:    [0x0, 0xf],         bits : 31_28
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER5_OFS 0x006c
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER5)
REGDEF_BIT(ise_isd_h_coef_msb0,        4)
REGDEF_BIT(ise_isd_h_coef_msb1,        4)
REGDEF_BIT(ise_isd_h_coef_msb2,        4)
REGDEF_BIT(ise_isd_h_coef_msb3,        4)
REGDEF_BIT(ise_isd_h_coef_msb4,        4)
REGDEF_BIT(ise_isd_h_coef_msb5,        4)
REGDEF_BIT(ise_isd_h_coef_msb6,        4)
REGDEF_BIT(ise_isd_h_coef_msb7,        4)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER5)


/*
    ise_isd_h_coef_msb8 :    [0x0, 0xf],            bits : 3_0
    ise_isd_h_coef_msb9 :    [0x0, 0xf],            bits : 7_4
    ise_isd_h_coef_msb10:    [0x0, 0xf],            bits : 11_8
    ise_isd_h_coef_msb11:    [0x0, 0xf],            bits : 15_12
    ise_isd_h_coef_msb12:    [0x0, 0xf],            bits : 19_16
    ise_isd_h_coef_msb13:    [0x0, 0xf],            bits : 23_20
    ise_isd_h_coef_msb14:    [0x0, 0xf],            bits : 27_24
    ise_isd_h_coef_msb15:    [0x0, 0xf],            bits : 31_28
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER6_OFS 0x0070
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER6)
REGDEF_BIT(ise_isd_h_coef_msb8,        4)
REGDEF_BIT(ise_isd_h_coef_msb9,        4)
REGDEF_BIT(ise_isd_h_coef_msb10,        4)
REGDEF_BIT(ise_isd_h_coef_msb11,        4)
REGDEF_BIT(ise_isd_h_coef_msb12,        4)
REGDEF_BIT(ise_isd_h_coef_msb13,        4)
REGDEF_BIT(ise_isd_h_coef_msb14,        4)
REGDEF_BIT(ise_isd_h_coef_msb15,        4)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER6)


/*
    ise_isd_v_coef0:    [0x0, 0xff],            bits : 7_0
    ise_isd_v_coef1:    [0x0, 0xff],            bits : 15_8
    ise_isd_v_coef2:    [0x0, 0xff],            bits : 23_16
    ise_isd_v_coef3:    [0x0, 0xff],            bits : 31_24
*/
#define ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER0_OFS 0x0074
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER0)
REGDEF_BIT(ise_isd_v_coef0,        8)
REGDEF_BIT(ise_isd_v_coef1,        8)
REGDEF_BIT(ise_isd_v_coef2,        8)
REGDEF_BIT(ise_isd_v_coef3,        8)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER0)


/*
    ise_isd_v_coef4:    [0x0, 0xff],            bits : 7_0
    ise_isd_v_coef5:    [0x0, 0xff],            bits : 15_8
    ise_isd_v_coef6:    [0x0, 0xff],            bits : 23_16
    ise_isd_v_coef7:    [0x0, 0xff],            bits : 31_24
*/
#define ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER1_OFS 0x0078
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER1)
REGDEF_BIT(ise_isd_v_coef4,        8)
REGDEF_BIT(ise_isd_v_coef5,        8)
REGDEF_BIT(ise_isd_v_coef6,        8)
REGDEF_BIT(ise_isd_v_coef7,        8)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER1)


/*
    ise_isd_v_coef8 :    [0x0, 0xff],           bits : 7_0
    ise_isd_v_coef9 :    [0x0, 0xff],           bits : 15_8
    ise_isd_v_coef10:    [0x0, 0xff],           bits : 23_16
    ise_isd_v_coef11:    [0x0, 0xff],           bits : 31_24
*/
#define ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER2_OFS 0x007c
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER2)
REGDEF_BIT(ise_isd_v_coef8,        8)
REGDEF_BIT(ise_isd_v_coef9,        8)
REGDEF_BIT(ise_isd_v_coef10,        8)
REGDEF_BIT(ise_isd_v_coef11,        8)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER2)


/*
    ise_isd_v_coef12:    [0x0, 0xff],           bits : 7_0
    ise_isd_v_coef13:    [0x0, 0xff],           bits : 15_8
    ise_isd_v_coef14:    [0x0, 0xff],           bits : 23_16
    ise_isd_v_coef15:    [0x0, 0xff],           bits : 31_24
*/
#define ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER3_OFS 0x0080
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER3)
REGDEF_BIT(ise_isd_v_coef12,        8)
REGDEF_BIT(ise_isd_v_coef13,        8)
REGDEF_BIT(ise_isd_v_coef14,        8)
REGDEF_BIT(ise_isd_v_coef15,        8)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER3)


/*
    ise_isd_v_coef16:    [0x0, 0xfff],          bits : 11_0
*/
#define ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER4_OFS 0x0084
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER4)
REGDEF_BIT(ise_isd_v_coef16,        12)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER4)


/*
    ise_isd_v_coef_msb0:    [0x0, 0xf],         bits : 3_0
    ise_isd_v_coef_msb1:    [0x0, 0xf],         bits : 7_4
    ise_isd_v_coef_msb2:    [0x0, 0xf],         bits : 11_8
    ise_isd_v_coef_msb3:    [0x0, 0xf],         bits : 15_12
    ise_isd_v_coef_msb4:    [0x0, 0xf],         bits : 19_16
    ise_isd_v_coef_msb5:    [0x0, 0xf],         bits : 23_20
    ise_isd_v_coef_msb6:    [0x0, 0xf],         bits : 27_24
    ise_isd_v_coef_msb7:    [0x0, 0xf],         bits : 31_28
*/
#define ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER5_OFS 0x0088
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER5)
REGDEF_BIT(ise_isd_v_coef_msb0,        4)
REGDEF_BIT(ise_isd_v_coef_msb1,        4)
REGDEF_BIT(ise_isd_v_coef_msb2,        4)
REGDEF_BIT(ise_isd_v_coef_msb3,        4)
REGDEF_BIT(ise_isd_v_coef_msb4,        4)
REGDEF_BIT(ise_isd_v_coef_msb5,        4)
REGDEF_BIT(ise_isd_v_coef_msb6,        4)
REGDEF_BIT(ise_isd_v_coef_msb7,        4)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER5)


/*
    ise_isd_v_coef_msb8 :    [0x0, 0xf],            bits : 3_0
    ise_isd_v_coef_msb9 :    [0x0, 0xf],            bits : 7_4
    ise_isd_v_coef_msb10:    [0x0, 0xf],            bits : 11_8
    ise_isd_v_coef_msb11:    [0x0, 0xf],            bits : 15_12
    ise_isd_v_coef_msb12:    [0x0, 0xf],            bits : 19_16
    ise_isd_v_coef_msb13:    [0x0, 0xf],            bits : 23_20
    ise_isd_v_coef_msb14:    [0x0, 0xf],            bits : 27_24
    ise_isd_v_coef_msb15:    [0x0, 0xf],            bits : 31_28
*/
#define ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER6_OFS 0x008c
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER6)
REGDEF_BIT(ise_isd_v_coef_msb8,        4)
REGDEF_BIT(ise_isd_v_coef_msb9,        4)
REGDEF_BIT(ise_isd_v_coef_msb10,        4)
REGDEF_BIT(ise_isd_v_coef_msb11,        4)
REGDEF_BIT(ise_isd_v_coef_msb12,        4)
REGDEF_BIT(ise_isd_v_coef_msb13,        4)
REGDEF_BIT(ise_isd_v_coef_msb14,        4)
REGDEF_BIT(ise_isd_v_coef_msb15,        4)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER6)


/*
    ISE_OFSO:    [0x0, 0x3ffff],            bits : 19_2
*/
#define ISE_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS 0x0090
REGDEF_BEGIN(ISE_OUTPUT_DMA_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ise_dram_ofso,        18)
REGDEF_END(ISE_OUTPUT_DMA_LINEOFFSET_REGISTER0)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RESERVED3_OFS 0x0094
REGDEF_BEGIN(RESERVED3)
REGDEF_BIT(reserved,        32)
REGDEF_END(RESERVED3)


/*
    ise_dram_sao:    [0x0, 0xffffffff],         bits : 31_0
*/
#define ISE_OUTPUT_DMA_CHANNEL_REGISTER0_OFS 0x0098
REGDEF_BEGIN(ISE_OUTPUT_DMA_CHANNEL_REGISTER0)
REGDEF_BIT(ise_dram_sao,        32)
REGDEF_END(ISE_OUTPUT_DMA_CHANNEL_REGISTER0)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RESERVED4_OFS 0x009c
REGDEF_BEGIN(RESERVED4)
REGDEF_BIT(reserved,        32)
REGDEF_END(RESERVED4)


/*
    ise_isd_h_coef_all:    [0x0, 0x1ffff],          bits : 16_0
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALING_USER_COEFFICIENTS_REGISTER7_OFS 0x00a0
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALING_USER_COEFFICIENTS_REGISTER7)
REGDEF_BIT(ise_isd_h_coef_all,        17)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALING_USER_COEFFICIENTS_REGISTER7)


/*
    ise_isd_h_coef_half:    [0x0, 0x1ffff],         bits : 16_0
*/
#define ISE_INTEGRATION_HORIZONTAL_SCALING_USER_COEFFICIENTS_REGISTER8_OFS 0x00a4
REGDEF_BEGIN(ISE_INTEGRATION_HORIZONTAL_SCALING_USER_COEFFICIENTS_REGISTER8)
REGDEF_BIT(ise_isd_h_coef_half,        17)
REGDEF_END(ISE_INTEGRATION_HORIZONTAL_SCALING_USER_COEFFICIENTS_REGISTER8)


/*
    ise_isd_v_coef_all:    [0x0, 0x1ffff],          bits : 16_0
*/
#define ISE_INTEGRATION_VERTICAL_SCALING_USER_COEFFICIENTS_REGISTER7_OFS 0x00a8
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALING_USER_COEFFICIENTS_REGISTER7)
REGDEF_BIT(ise_isd_v_coef_all,        17)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALING_USER_COEFFICIENTS_REGISTER7)


/*
    ise_isd_v_coef_half:    [0x0, 0x1ffff],         bits : 16_0
*/
#define ISE_INTEGRATION_VERTICAL_SCALING_USER_COEFFICIENTS_REGISTER8_OFS 0x00ac
REGDEF_BEGIN(ISE_INTEGRATION_VERTICAL_SCALING_USER_COEFFICIENTS_REGISTER8)
REGDEF_BIT(ise_isd_v_coef_half,        17)
REGDEF_END(ISE_INTEGRATION_VERTICAL_SCALING_USER_COEFFICIENTS_REGISTER8)


#endif

