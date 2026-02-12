/*
    IFE module register header

    NT98520 IFE module register header

    @file       ife_reg.h
    @ingroup    mIDrvIPP_IFE

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _IFE_NT98520_H_
#define _IFE_NT98520_H_

#if 1
#include "ife_platform.h"
#else
#include "mach/rcw_macro.h"
#include "kwrap/type.h"
#endif

//#include "DrvCommon.h"
/*
#define IFE_REG_ADDR(ofs)        (_IFE_REG_BASE_ADDR+(ofs))
#define IFE_SETREG(ofs, value)   OUTW((_IFE_REG_BASE_ADDR + ofs), value)
#define IFE_GETREG(ofs)          INW(_IFE_REG_BASE_ADDR + ofs)
*/
/*
    ife_sw_rst       :    [0x0, 0x1],           bits : 0
    ife_start        :    [0x0, 0x1],           bits : 1
    ife_load_start   :    [0x0, 0x1],           bits : 2
    ife_load_fd      :    [0x0, 0x1],           bits : 3
    ife_load_frmstart:    [0x0, 0x1],           bits : 4
    ife_ll_fire      :    [0x0, 0x1],           bits : 28
    ife_ll_terminate :    [0x0, 0x1],           bits : 29
*/
#define FILTER_OPERATION_CONTROL_REGISTER_OFS 0x0000
REGDEF_BEGIN(FILTER_OPERATION_CONTROL_REGISTER)
REGDEF_BIT(ife_sw_rst,        1)
REGDEF_BIT(ife_start,        1)
REGDEF_BIT(ife_load_start,        1)
REGDEF_BIT(ife_load_fd,        1)
REGDEF_BIT(ife_load_frmstart,        1)
REGDEF_BIT(,        23)
REGDEF_BIT(ife_ll_fire,        1)
REGDEF_BIT(ife_ll_terminate,        1)
REGDEF_END(FILTER_OPERATION_CONTROL_REGISTER)


/*
    IFE_MODE           :    [0x0, 0x3],         bits : 1_0
    INBIT_DEPTH        :    [0x0, 0x3],         bits : 6_5
    OUTBIT_DEPTH       :    [0x0, 0x3],         bits : 8_7
    CFAPAT             :    [0x0, 0x7],         bits : 11_9
    IFE_FILT_MODE      :    [0x0, 0x1],         bits : 12
    IFE_OUTL_EN        :    [0x0, 0x1],         bits : 13
    IFE_FILTER_EN      :    [0x0, 0x1],         bits : 14
    IFE_CGAIN_EN       :    [0x0, 0x1],         bits : 15
    IFE_VIG_EN         :    [0x0, 0x1],         bits : 16
    IFE_GBAL_EN        :    [0x0, 0x1],         bits : 17
    IFE_BINNING        :    [0x0, 0x7],         bits : 20_18
    BAYER_FORMAT       :    [0x0, 0x1],         bits : 21
    IFE_RGBIR_RB_NRFILL:    [0x0, 0x1],         bits : 22
    IFE_BILAT_TH_EN    :    [0x0, 0x1],         bits : 23
    IFE_F_NRS_EN       :    [0x0, 0x1],         bits : 24
    IFE_F_CG_EN        :    [0x0, 0x1],         bits : 25
    IFE_F_FUSION_EN    :    [0x0, 0x1],         bits : 26
    IFE_F_FUSION_FNUM  :    [0x0, 0x3],         bits : 28_27
    IFE_F_FC_EN        :    [0x0, 0x1],         bits : 29
    MIRROR_EN          :    [0x0, 0x1],         bits : 30
    IFE_R_DECODE_EN    :    [0x0, 0x1],         bits : 31
*/
#define CONTROL_REGISTER_OFS 0x0004
REGDEF_BEGIN(CONTROL_REGISTER)
REGDEF_BIT(ife_mode,        2)
REGDEF_BIT(,        3)
REGDEF_BIT(inbit_depth,        2)
REGDEF_BIT(outbit_depth,        2)
REGDEF_BIT(cfapat,        3)
REGDEF_BIT(filt_mode,        1)
REGDEF_BIT(outl_en,        1)
REGDEF_BIT(filter_en,        1)
REGDEF_BIT(cgain_en,        1)
REGDEF_BIT(vig_en,        1)
REGDEF_BIT(gbal_en,        1)
REGDEF_BIT(binning,        3)
REGDEF_BIT(bayer_fmt,        1)
REGDEF_BIT(rgbir_rb_nrfill,        1)
REGDEF_BIT(bilat_th_en,        1)
REGDEF_BIT(f_nrs_en,        1)
REGDEF_BIT(f_cg_en,        1)
REGDEF_BIT(f_fusion_en,        1)
REGDEF_BIT(f_fusion_fnum,        2)
REGDEF_BIT(f_fc_en,        1)
REGDEF_BIT(mirror_en,        1)
REGDEF_BIT(r_decode_en,        1)
REGDEF_END(CONTROL_REGISTER)


/*
    inte_frmend         :    [0x0, 0x1],            bits : 0
    inte_ife_r_dec1_err :    [0x0, 0x1],            bits : 1
    inte_ife_r_dec2_err :    [0x0, 0x1],            bits : 2
    inte_llend          :    [0x0, 0x1],            bits : 3
    inte_llerror        :    [0x0, 0x1],            bits : 4
    inte_llerror2       :    [0x0, 0x1],            bits : 5
    inte_lljobend       :    [0x0, 0x1],            bits : 6
    inte_ife_bufovfl    :    [0x0, 0x1],            bits : 7
    inte_ife_ringbuf_err:    [0x0, 0x1],            bits : 8
*/
#define FILTER_INTERRUPT_ENABLE_REGISTER_OFS 0x0008
REGDEF_BEGIN(FILTER_INTERRUPT_ENABLE_REGISTER)
REGDEF_BIT(inte_frmend,        1)
REGDEF_BIT(inte_ife_r_dec1_err,        1)
REGDEF_BIT(inte_ife_r_dec2_err,        1)
REGDEF_BIT(inte_llend,        1)
REGDEF_BIT(inte_llerror,        1)
REGDEF_BIT(inte_llerror2,        1)
REGDEF_BIT(inte_lljobend,        1)
REGDEF_BIT(inte_ife_bufovfl,        1)
REGDEF_BIT(inte_ife_ringbuf_err,        1)
REGDEF_END(FILTER_INTERRUPT_ENABLE_REGISTER)


/*
    int_frmend         :    [0x0, 0x1],         bits : 0
    int_ife_r_dec1_err :    [0x0, 0x1],         bits : 1
    int_ife_r_dec2_err :    [0x0, 0x1],         bits : 2
    int_llend          :    [0x0, 0x1],         bits : 3
    int_llerror        :    [0x0, 0x1],         bits : 4
    int_llerror2       :    [0x0, 0x1],         bits : 5
    int_lljobend       :    [0x0, 0x1],         bits : 6
    int_ife_bufovfl    :    [0x0, 0x1],         bits : 7
    int_ife_ringbuf_err:    [0x0, 0x1],         bits : 8
*/
#define FILTER_INTERRUPT_STATUS_REGISTER_OFS 0x000c
REGDEF_BEGIN(FILTER_INTERRUPT_STATUS_REGISTER)
REGDEF_BIT(int_frmend,        1)
REGDEF_BIT(int_ife_r_dec1_err,        1)
REGDEF_BIT(int_ife_r_dec2_err,        1)
REGDEF_BIT(int_llend,        1)
REGDEF_BIT(int_llerror,        1)
REGDEF_BIT(int_llerror2,        1)
REGDEF_BIT(int_lljobend,        1)
REGDEF_BIT(int_ife_bufovfl,        1)
REGDEF_BIT(int_ife_ringbuf_err,        1)
REGDEF_END(FILTER_INTERRUPT_STATUS_REGISTER)


/*
    ife_busy:    [0x0, 0x1],            bits : 0
    reserved:    [0x0, 0x7fffffff],     bits : 18_1
    ife_dma_wait_sie2_start_disable:    [0x0, 0x1],   bits : 19
*/
#define DEBUG_REGISTER_OFS 0x0010
REGDEF_BEGIN(DEBUG_REGISTER)
REGDEF_BIT(ife_busy,        1)
REGDEF_BIT(,        18)
REGDEF_BIT(ife_dma_wait_sie2_start_disable,        1)
REGDEF_END(DEBUG_REGISTER)


/*
    ife_hn :    [0x0, 0x3ff],           bits : 9_0
    ife_hl :    [0x0, 0x3ff],           bits : 21_12
    ife_hm :    [0x0, 0xf],         bits : 27_24
*/
#define STRIPE_REGISTER_OFS 0x0014
REGDEF_BEGIN(STRIPE_REGISTER)
REGDEF_BIT(ife_hn,        11)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_hl,        11)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_hm,        4)
REGDEF_END(STRIPE_REGISTER)

/*
    width :    [0x0, 0x3fff],           bits : 15_2
    height:    [0x0, 0x7fff],           bits : 31_17
*/
#define SOURCE_SIZE_REGISTER_0_OFS 0x0020
REGDEF_BEGIN(SOURCE_SIZE_REGISTER_0)
REGDEF_BIT(,        2)
REGDEF_BIT(width,        14)
REGDEF_BIT(,        1)
REGDEF_BIT(height,        15)
REGDEF_END(SOURCE_SIZE_REGISTER_0)


/*
    crop_width :    [0x0, 0x3fff],          bits : 15_2
    crop_height:    [0x0, 0x7fff],          bits : 31_17
*/
#define SOURCE_SIZE_REGISTER_1_OFS 0x0024
REGDEF_BEGIN(SOURCE_SIZE_REGISTER_1)
REGDEF_BIT(,        2)
REGDEF_BIT(crop_width,        14)
REGDEF_BIT(,        1)
REGDEF_BIT(crop_height,        15)
REGDEF_END(SOURCE_SIZE_REGISTER_1)


/*
    crop_hpos:    [0x0, 0xffff],            bits : 15_0
    crop_vpos:    [0x0, 0xffff],            bits : 31_16
*/
#define SOURCE_SIZE_REGISTER_2_OFS 0x0028
REGDEF_BEGIN(SOURCE_SIZE_REGISTER_2)
REGDEF_BIT(crop_hpos,        16)
REGDEF_BIT(crop_vpos,        16)
REGDEF_END(SOURCE_SIZE_REGISTER_2)

/*
    dram_sai0:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define SOURCE_ADDRESS_REGISTER_0_OFS 0x0030
REGDEF_BEGIN(SOURCE_ADDRESS_REGISTER_0)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_sai0,        30)
REGDEF_END(SOURCE_ADDRESS_REGISTER_0)


/*
    dram_ofsi0   :    [0x0, 0x3fff],            bits : 15_2
    h_start_shift:    [0x0, 0xf],           bits : 19_16
*/
#define SOURCE_LINE_OFFSET_REGISTER_0_OFS 0x0034
REGDEF_BEGIN(SOURCE_LINE_OFFSET_REGISTER_0)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_ofsi0,        14)
REGDEF_BIT(h_start_shift,        4)
REGDEF_END(SOURCE_LINE_OFFSET_REGISTER_0)


/*
    dram_sai1:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define SOURCE_ADDRESS_REGISTER_1_OFS 0x0038
REGDEF_BEGIN(SOURCE_ADDRESS_REGISTER_1)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_sai1,        30)
REGDEF_END(SOURCE_ADDRESS_REGISTER_1)


/*
    dram_ofsi1:    [0x0, 0x3fff],           bits : 15_2
*/
#define SOURCE_LINE_OFFSET_REGISTER_1_OFS 0x003c
REGDEF_BEGIN(SOURCE_LINE_OFFSET_REGISTER_1)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_ofsi1,        14)
REGDEF_END(SOURCE_LINE_OFFSET_REGISTER_1)

/*
    dram_saill:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define SOURCE_ADDRESS_REGISTER_2_OFS 0x0050
REGDEF_BEGIN(SOURCE_ADDRESS_REGISTER_2)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_saill,        30)
REGDEF_END(SOURCE_ADDRESS_REGISTER_2)


/*
    dram_sao:    [0x0, 0x3fffffff],         bits : 31_2
*/
#define DESTINATION_ADDRESS_REGISTER_OFS 0x0054
REGDEF_BEGIN(DESTINATION_ADDRESS_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_sao,        30)
REGDEF_END(DESTINATION_ADDRESS_REGISTER)


/*
    dram_ofso:    [0x0, 0x3fff],            bits : 15_2
*/
#define DESTINATION_LINE_OFFSET_REGISTER_OFS 0x0058
REGDEF_BEGIN(DESTINATION_LINE_OFFSET_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_ofso,        14)
REGDEF_END(DESTINATION_LINE_OFFSET_REGISTER)


/*
    input_burst_mode :    [0x0, 0x1],           bits : 0
    output_burst_mode:    [0x0, 0x1],           bits : 4
    dmaloop_line     :    [0x0, 0x3ff],         bits : 21_12
    dmaloop_en       :    [0x0, 0x1],           bits : 24
*/
#define DRAM_SETTINGS_OFS 0x005c
REGDEF_BEGIN(DRAM_SETTINGS)
REGDEF_BIT(input_burst_mode,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(output_burst_mode,        1)
REGDEF_BIT(,        7)
REGDEF_BIT(dmaloop_line,        11)
REGDEF_BIT(,        1)
REGDEF_BIT(dmaloop_en,        1)
REGDEF_END(DRAM_SETTINGS)

/*
    cgain_inv        :    [0x0, 0x1],           bits : 0
    cgain_hinv       :    [0x0, 0x1],           bits : 1
    cgain_range      :    [0x0, 0x1],           bits : 2
    ife_f_cgain_range:    [0x0, 0x1],           bits : 3
    cgain_mask       :    [0x0, 0xfff],         bits : 19_8
*/
#define COLOR_GAIN_REGISTER_0_OFS 0x0070
REGDEF_BEGIN(COLOR_GAIN_REGISTER_0)
REGDEF_BIT(cgain_inv,        1)
REGDEF_BIT(cgain_hinv,        1)
REGDEF_BIT(cgain_range,        1)
REGDEF_BIT(ife_f_cgain_range,        1)
REGDEF_BIT(,        4)
REGDEF_BIT(cgain_mask,        12)
REGDEF_END(COLOR_GAIN_REGISTER_0)


/*
    ife_cgain_r :    [0x0, 0x3ff],          bits : 9_0
    ife_cgain_gr:    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_GAIN_REGISTER_1_OFS 0x0074
REGDEF_BEGIN(COLOR_GAIN_REGISTER_1)
REGDEF_BIT(ife_cgain_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_cgain_gr,        10)
REGDEF_END(COLOR_GAIN_REGISTER_1)


/*
    ife_cgain_gb:    [0x0, 0x3ff],          bits : 9_0
    ife_cgain_b :    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_GAIN_REGISTER_2_OFS 0x0078
REGDEF_BEGIN(COLOR_GAIN_REGISTER_2)
REGDEF_BIT(ife_cgain_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_cgain_b,        10)
REGDEF_END(COLOR_GAIN_REGISTER_2)


/*
    ife_cgain_ir:    [0x0, 0x3ff],          bits : 9_0
*/
#define COLOR_GAIN_REGISTER_3_OFS 0x007c
REGDEF_BEGIN(COLOR_GAIN_REGISTER_3)
REGDEF_BIT(ife_cgain_ir,        10)
REGDEF_END(COLOR_GAIN_REGISTER_3)


/*
    ife_f_p0_cgain_r :    [0x0, 0x3ff],         bits : 9_0
    ife_f_p0_cgain_gr:    [0x0, 0x3ff],         bits : 25_16
*/
#define COLOR_GAIN_REGISTER_4_OFS 0x0080
REGDEF_BEGIN(COLOR_GAIN_REGISTER_4)
REGDEF_BIT(ife_f_p0_cgain_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p0_cgain_gr,        10)
REGDEF_END(COLOR_GAIN_REGISTER_4)


/*
    ife_f_p0_cgain_gb:    [0x0, 0x3ff],         bits : 9_0
    ife_f_p0_cgain_b :    [0x0, 0x3ff],         bits : 25_16
*/
#define COLOR_GAIN_REGISTER_5_OFS 0x0084
REGDEF_BEGIN(COLOR_GAIN_REGISTER_5)
REGDEF_BIT(ife_f_p0_cgain_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p0_cgain_b,        10)
REGDEF_END(COLOR_GAIN_REGISTER_5)


/*
    ife_f_p0_cgain_ir:    [0x0, 0x3ff],         bits : 9_0
*/
#define COLOR_GAIN_REGISTER_6_OFS 0x0088
REGDEF_BEGIN(COLOR_GAIN_REGISTER_6)
REGDEF_BIT(ife_f_p0_cgain_ir,        10)
REGDEF_END(COLOR_GAIN_REGISTER_6)


/*
    ife_f_p1_cgain_r :    [0x0, 0x3ff],         bits : 9_0
    ife_f_p1_cgain_gr:    [0x0, 0x3ff],         bits : 25_16
*/
#define COLOR_GAIN_REGISTER_7_OFS 0x008c
REGDEF_BEGIN(COLOR_GAIN_REGISTER_7)
REGDEF_BIT(ife_f_p1_cgain_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p1_cgain_gr,        10)
REGDEF_END(COLOR_GAIN_REGISTER_7)


/*
    ife_f_p1_cgain_gb:    [0x0, 0x3ff],         bits : 9_0
    ife_f_p1_cgain_b :    [0x0, 0x3ff],         bits : 25_16
*/
#define COLOR_GAIN_REGISTER_8_OFS 0x0090
REGDEF_BEGIN(COLOR_GAIN_REGISTER_8)
REGDEF_BIT(ife_f_p1_cgain_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p1_cgain_b,        10)
REGDEF_END(COLOR_GAIN_REGISTER_8)


/*
    ife_f_p1_cgain_ir:    [0x0, 0x3ff],         bits : 9_0
*/
#define COLOR_GAIN_REGISTER_9_OFS 0x0094
REGDEF_BEGIN(COLOR_GAIN_REGISTER_9)
REGDEF_BIT(ife_f_p1_cgain_ir,        10)
REGDEF_END(COLOR_GAIN_REGISTER_9)

/*
    ife_cofs_r :    [0x0, 0x3ff],           bits : 9_0
    ife_cofs_gr:    [0x0, 0x3ff],           bits : 25_16
*/
#define COLOR_OFFSET_REGISTER_0_OFS 0x00b0
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_0)
REGDEF_BIT(ife_cofs_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_cofs_gr,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_0)


/*
    ife_cofs_gb:    [0x0, 0x3ff],           bits : 9_0
    ife_cofs_b :    [0x0, 0x3ff],           bits : 25_16
*/
#define COLOR_OFFSET_REGISTER_1_OFS 0x00b4
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_1)
REGDEF_BIT(ife_cofs_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_cofs_b,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_1)


/*
    ife_cofs_ir:    [0x0, 0x3ff],           bits : 9_0
*/
#define COLOR_OFFSET_REGISTER_2_OFS 0x00b8
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_2)
REGDEF_BIT(ife_cofs_ir,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_2)


/*
    ife_f_p0_cofs_r :    [0x0, 0x3ff],          bits : 9_0
    ife_f_p0_cofs_gr:    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_OFFSET_REGISTER_3_OFS 0x00bc
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_3)
REGDEF_BIT(ife_f_p0_cofs_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p0_cofs_gr,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_3)


/*
    ife_f_p0_cofs_gb:    [0x0, 0x3ff],          bits : 9_0
    ife_f_p0_cofs_b :    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_OFFSET_REGISTER_4_OFS 0x00c0
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_4)
REGDEF_BIT(ife_f_p0_cofs_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p0_cofs_b,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_4)


/*
    ife_f_p0_cofs_ir:    [0x0, 0x3ff],          bits : 9_0
*/
#define COLOR_OFFSET_REGISTER_5_OFS 0x00c4
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_5)
REGDEF_BIT(ife_f_p0_cofs_ir,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_5)


/*
    ife_f_p1_cofs_r :    [0x0, 0x3ff],          bits : 9_0
    ife_f_p1_cofs_gr:    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_OFFSET_REGISTER_6_OFS 0x00c8
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_6)
REGDEF_BIT(ife_f_p1_cofs_r,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p1_cofs_gr,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_6)


/*
    ife_f_p1_cofs_gb:    [0x0, 0x3ff],          bits : 9_0
    ife_f_p1_cofs_b :    [0x0, 0x3ff],          bits : 25_16
*/
#define COLOR_OFFSET_REGISTER_7_OFS 0x00cc
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_7)
REGDEF_BIT(ife_f_p1_cofs_gb,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_f_p1_cofs_b,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_7)


/*
    ife_f_p1_cofs_ir:    [0x0, 0x3ff],          bits : 9_0
*/
#define COLOR_OFFSET_REGISTER_8_OFS 0x00d0
REGDEF_BEGIN(COLOR_OFFSET_REGISTER_8)
REGDEF_BIT(ife_f_p1_cofs_ir,        10)
REGDEF_END(COLOR_OFFSET_REGISTER_8)

/*
    ife_clamp_th :    [0x0, 0xfff],         bits : 11_0
    ife_clamp_mul:    [0x0, 0xff],          bits : 19_12
    ife_clamp_dlt:    [0x0, 0xfff],         bits : 31_20
*/
#define OUTPUT_FILTER_REGISTER_OFS 0x0100
REGDEF_BEGIN(OUTPUT_FILTER_REGISTER)
REGDEF_BIT(ife_clamp_th,        12)
REGDEF_BIT(ife_clamp_mul,        8)
REGDEF_BIT(ife_clamp_dlt,        12)
REGDEF_END(OUTPUT_FILTER_REGISTER)


/*
    ife_bilat_w  :    [0x0, 0xf],           bits : 3_0
    ife_rth_w    :    [0x0, 0xf],           bits : 7_4
    ife_bilat_th1:    [0x0, 0x3ff],         bits : 17_8
    ife_bilat_th2:    [0x0, 0x3ff],         bits : 27_18
*/
#define RANGE_FILTER_REGISTER_0_OFS 0x0104
REGDEF_BEGIN(RANGE_FILTER_REGISTER_0)
REGDEF_BIT(ife_bilat_w,        4)
REGDEF_BIT(ife_rth_w,        4)
REGDEF_BIT(ife_bilat_th1,        10)
REGDEF_BIT(ife_bilat_th2,        10)
REGDEF_END(RANGE_FILTER_REGISTER_0)


/*
    IFE_S_WEIGHT0:    [0x0, 0x1f],          bits : 4_0
    IFE_S_WEIGHT1:    [0x0, 0x1f],          bits : 12_8
    IFE_S_WEIGHT2:    [0x0, 0x1f],          bits : 20_16
    IFE_S_WEIGHT3:    [0x0, 0x1f],          bits : 28_24
*/
#define SPATIAL_FILTER_REGISTER_1_OFS 0x0108
REGDEF_BEGIN(SPATIAL_FILTER_REGISTER_1)
REGDEF_BIT(ife_spatial_weight0,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_spatial_weight1,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_spatial_weight2,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_spatial_weight3,        5)
REGDEF_END(SPATIAL_FILTER_REGISTER_1)


/*
    IFE_S_WEIGHT4:    [0x0, 0x1f],          bits : 4_0
    IFE_S_WEIGHT5:    [0x0, 0x1f],          bits : 12_8
*/
#define SPATIAL_FILTER_REGISTER_2_OFS 0x010c
REGDEF_BEGIN(SPATIAL_FILTER_REGISTER_2)
REGDEF_BIT(ife_spatial_weight4,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_spatial_weight5,        5)
REGDEF_END(SPATIAL_FILTER_REGISTER_2)


/*
    ife_rth_nlm_c0_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_1_OFS 0x0110
REGDEF_BEGIN(RANGE_FILTER_REGISTER_1)
REGDEF_BIT(ife_rth_nlm_c0_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_1)


/*
    ife_rth_nlm_c0_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_2_OFS 0x0114
REGDEF_BEGIN(RANGE_FILTER_REGISTER_2)
REGDEF_BIT(ife_rth_nlm_c0_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_2)


/*
    ife_rth_nlm_c0_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_3_OFS 0x0118
REGDEF_BEGIN(RANGE_FILTER_REGISTER_3)
REGDEF_BIT(ife_rth_nlm_c0_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_3)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RANGE_FILTER_REGISTER_4_OFS 0x011c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_4)
REGDEF_BIT(reserved,        32)
REGDEF_END(RANGE_FILTER_REGISTER_4)


/*
    ife_rth_nlm_c1_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_5_OFS 0x0120
REGDEF_BEGIN(RANGE_FILTER_REGISTER_5)
REGDEF_BIT(ife_rth_nlm_c1_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_5)


/*
    ife_rth_nlm_c1_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_6_OFS 0x0124
REGDEF_BEGIN(RANGE_FILTER_REGISTER_6)
REGDEF_BIT(ife_rth_nlm_c1_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_6)


/*
    ife_rth_nlm_c1_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_7_OFS 0x0128
REGDEF_BEGIN(RANGE_FILTER_REGISTER_7)
REGDEF_BIT(ife_rth_nlm_c1_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_7)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RANGE_FILTER_REGISTER_8_OFS 0x012c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_8)
REGDEF_BIT(reserved,        32)
REGDEF_END(RANGE_FILTER_REGISTER_8)


/*
    ife_rth_nlm_c2_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_9_OFS 0x0130
REGDEF_BEGIN(RANGE_FILTER_REGISTER_9)
REGDEF_BIT(ife_rth_nlm_c2_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_9)


/*
    ife_rth_nlm_c2_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_10_OFS 0x0134
REGDEF_BEGIN(RANGE_FILTER_REGISTER_10)
REGDEF_BIT(ife_rth_nlm_c2_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_10)


/*
    ife_rth_nlm_c2_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_11_OFS 0x0138
REGDEF_BEGIN(RANGE_FILTER_REGISTER_11)
REGDEF_BIT(ife_rth_nlm_c2_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_11)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RANGE_FILTER_REGISTER_12_OFS 0x013c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_12)
REGDEF_BIT(reserved,        32)
REGDEF_END(RANGE_FILTER_REGISTER_12)


/*
    ife_rth_nlm_c3_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_13_OFS 0x0140
REGDEF_BEGIN(RANGE_FILTER_REGISTER_13)
REGDEF_BIT(ife_rth_nlm_c3_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_13)


/*
    ife_rth_nlm_c3_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_14_OFS 0x0144
REGDEF_BEGIN(RANGE_FILTER_REGISTER_14)
REGDEF_BIT(ife_rth_nlm_c3_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_14)


/*
    ife_rth_nlm_c3_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_15_OFS 0x0148
REGDEF_BEGIN(RANGE_FILTER_REGISTER_15)
REGDEF_BIT(ife_rth_nlm_c3_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_15)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RANGE_FILTER_REGISTER_16_OFS 0x014c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_16)
REGDEF_BIT(reserved,        32)
REGDEF_END(RANGE_FILTER_REGISTER_16)


/*
    ife_rth_bilat_c0_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_17_OFS 0x0150
REGDEF_BEGIN(RANGE_FILTER_REGISTER_17)
REGDEF_BIT(ife_rth_bilat_c0_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_17)


/*
    ife_rth_bilat_c0_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_18_OFS 0x0154
REGDEF_BEGIN(RANGE_FILTER_REGISTER_18)
REGDEF_BIT(ife_rth_bilat_c0_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_18)


/*
    ife_rth_bilat_c0_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_19_OFS 0x0158
REGDEF_BEGIN(RANGE_FILTER_REGISTER_19)
REGDEF_BIT(ife_rth_bilat_c0_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_19)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RANGE_FILTER_REGISTER_20_OFS 0x015c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_20)
REGDEF_BIT(reserved,        32)
REGDEF_END(RANGE_FILTER_REGISTER_20)


/*
    ife_rth_bilat_c1_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_21_OFS 0x0160
REGDEF_BEGIN(RANGE_FILTER_REGISTER_21)
REGDEF_BIT(ife_rth_bilat_c1_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_21)


/*
    ife_rth_bilat_c1_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_22_OFS 0x0164
REGDEF_BEGIN(RANGE_FILTER_REGISTER_22)
REGDEF_BIT(ife_rth_bilat_c1_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_22)


/*
    ife_rth_bilat_c1_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_23_OFS 0x0168
REGDEF_BEGIN(RANGE_FILTER_REGISTER_23)
REGDEF_BIT(ife_rth_bilat_c1_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_23)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RANGE_FILTER_REGISTER_24_OFS 0x016c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_24)
REGDEF_BIT(reserved,        32)
REGDEF_END(RANGE_FILTER_REGISTER_24)


/*
    ife_rth_bilat_c2_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_25_OFS 0x0170
REGDEF_BEGIN(RANGE_FILTER_REGISTER_25)
REGDEF_BIT(ife_rth_bilat_c2_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_25)


/*
    ife_rth_bilat_c2_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_26_OFS 0x0174
REGDEF_BEGIN(RANGE_FILTER_REGISTER_26)
REGDEF_BIT(ife_rth_bilat_c2_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_26)


/*
    ife_rth_bilat_c2_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_27_OFS 0x0178
REGDEF_BEGIN(RANGE_FILTER_REGISTER_27)
REGDEF_BIT(ife_rth_bilat_c2_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_27)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define RANGE_FILTER_REGISTER_28_OFS 0x017c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_28)
REGDEF_BIT(reserved,        32)
REGDEF_END(RANGE_FILTER_REGISTER_28)


/*
    ife_rth_bilat_c3_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_29_OFS 0x0180
REGDEF_BEGIN(RANGE_FILTER_REGISTER_29)
REGDEF_BIT(ife_rth_bilat_c3_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_29)


/*
    ife_rth_bilat_c3_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_30_OFS 0x0184
REGDEF_BEGIN(RANGE_FILTER_REGISTER_30)
REGDEF_BIT(ife_rth_bilat_c3_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_30)


/*
    ife_rth_bilat_c3_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_31_OFS 0x0188
REGDEF_BEGIN(RANGE_FILTER_REGISTER_31)
REGDEF_BIT(ife_rth_bilat_c3_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_31)


/*
    ife_outl_bright_ofs:    [0x0, 0xfff],           bits : 11_0
    ife_outl_dark_ofs  :    [0x0, 0xfff],           bits : 23_12
*/
#define OUTLIER_THRESHOLD_REGISTER_0_OFS 0x018c
REGDEF_BEGIN(OUTLIER_THRESHOLD_REGISTER_0)
REGDEF_BIT(ife_outl_bright_ofs,        12)
REGDEF_BIT(ife_outl_dark_ofs,        12)
REGDEF_END(OUTLIER_THRESHOLD_REGISTER_0)


/*
    ife_outlth_bri0 :    [0x0, 0xfff],          bits : 11_0
    ife_outlth_dark0:    [0x0, 0xfff],          bits : 23_12
*/
#define OUTLIER_THRESHOLD_REGISTER_1_OFS 0x0190
REGDEF_BEGIN(OUTLIER_THRESHOLD_REGISTER_1)
REGDEF_BIT(ife_outlth_bri0,        12)
REGDEF_BIT(ife_outlth_dark0,        12)
REGDEF_END(OUTLIER_THRESHOLD_REGISTER_1)


/*
    ife_outlth_bri1 :    [0x0, 0xfff],          bits : 11_0
    ife_outlth_dark1:    [0x0, 0xfff],          bits : 23_12
*/
#define OUTLIER_THRESHOLD_REGISTER_2_OFS 0x0194
REGDEF_BEGIN(OUTLIER_THRESHOLD_REGISTER_2)
REGDEF_BIT(ife_outlth_bri1,        12)
REGDEF_BIT(ife_outlth_dark1,        12)
REGDEF_END(OUTLIER_THRESHOLD_REGISTER_2)


/*
    ife_outlth_bri2 :    [0x0, 0xfff],          bits : 11_0
    ife_outlth_dark2:    [0x0, 0xfff],          bits : 23_12
*/
#define OUTLIER_THRESHOLD_REGISTER_3_OFS 0x0198
REGDEF_BEGIN(OUTLIER_THRESHOLD_REGISTER_3)
REGDEF_BIT(ife_outlth_bri2,        12)
REGDEF_BIT(ife_outlth_dark2,        12)
REGDEF_END(OUTLIER_THRESHOLD_REGISTER_3)


/*
    ife_outlth_bri3 :    [0x0, 0xfff],          bits : 11_0
    ife_outlth_dark3:    [0x0, 0xfff],          bits : 23_12
*/
#define OUTLIER_THRESHOLD_REGISTER_4_OFS 0x019c
REGDEF_BEGIN(OUTLIER_THRESHOLD_REGISTER_4)
REGDEF_BIT(ife_outlth_bri3,        12)
REGDEF_BIT(ife_outlth_dark3,        12)
REGDEF_END(OUTLIER_THRESHOLD_REGISTER_4)


/*
    ife_outlth_bri4 :    [0x0, 0xfff],          bits : 11_0
    ife_outlth_dark4:    [0x0, 0xfff],          bits : 23_12
*/
#define OUTLIER_THRESHOLD_REGISTER_5_OFS 0x01a0
REGDEF_BEGIN(OUTLIER_THRESHOLD_REGISTER_5)
REGDEF_BIT(ife_outlth_bri4,        12)
REGDEF_BIT(ife_outlth_dark4,        12)
REGDEF_END(OUTLIER_THRESHOLD_REGISTER_5)


/*
    ife_outl_avg_mode:    [0x0, 0x1],           bits : 0
    ife_outl_weight  :    [0x0, 0xff],          bits : 15_8
    ife_outl_cnt1    :    [0x0, 0x1f],          bits : 20_16
    ife_outl_cnt2    :    [0x0, 0x1f],          bits : 28_24
*/
#define OUTLIER_THRESHOLD_REGISTER_6_OFS 0x01a4
REGDEF_BEGIN(OUTLIER_THRESHOLD_REGISTER_6)
REGDEF_BIT(ife_outl_avg_mode,        1)
REGDEF_BIT(,        7)
REGDEF_BIT(ife_outl_weight,        8)
REGDEF_BIT(ife_outl_cnt1,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_outl_cnt2,        5)
REGDEF_END(OUTLIER_THRESHOLD_REGISTER_6)


/*
    IFE_ORD_RANGE_BRI :    [0x0, 0x7],          bits : 2_0
    ife_ord_range_dark:    [0x0, 0x7],          bits : 6_4
    ife_ord_protect_th:    [0x0, 0x3ff],            bits : 17_8
    IFE_ORD_BLEND_W   :    [0x0, 0xff],         bits : 27_20
*/
#define OUTLIER_ORDER_REGISTER_0_OFS 0x01a8
REGDEF_BEGIN(OUTLIER_ORDER_REGISTER_0)
REGDEF_BIT(ife_ord_range_bright,     3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_ord_range_dark,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_ord_protect_th,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_ord_blend_weight,      8)
REGDEF_END(OUTLIER_ORDER_REGISTER_0)


/*
    ife_ord_bright_weight_lut0 (0~8):    [0x0, 0xf],            bits : 3_0
    ife_ord_bright_weight_lut1 (0~8):    [0x0, 0xf],            bits : 7_4
    ife_ord_bright_weight_lut2 (0~8):    [0x0, 0xf],            bits : 11_8
    ife_ord_bright_weight_lut3 (0~8):    [0x0, 0xf],            bits : 15_12
    ife_ord_bright_weight_lut4 (0~8):    [0x0, 0xf],            bits : 19_16
    ife_ord_bright_weight_lut5 (0~8):    [0x0, 0xf],            bits : 23_20
    ife_ord_bright_weight_lut6 (0~8):    [0x0, 0xf],            bits : 27_24
    ife_ord_bright_weight_lut7 (0~8):    [0x0, 0xf],            bits : 31_28
*/
#define OUTLIER_ORDER_REGISTER_1_OFS 0x01ac
REGDEF_BEGIN(OUTLIER_ORDER_REGISTER_1)
REGDEF_BIT(ife_ord_bright_weight_lut0,        4)
REGDEF_BIT(ife_ord_bright_weight_lut1,        4)
REGDEF_BIT(ife_ord_bright_weight_lut2,        4)
REGDEF_BIT(ife_ord_bright_weight_lut3,        4)
REGDEF_BIT(ife_ord_bright_weight_lut4,        4)
REGDEF_BIT(ife_ord_bright_weight_lut5,        4)
REGDEF_BIT(ife_ord_bright_weight_lut6,        4)
REGDEF_BIT(ife_ord_bright_weight_lut7,        4)
REGDEF_END(OUTLIER_ORDER_REGISTER_1)


/*
    ife_ord_dark_weight_lut0 (0~8):    [0x0, 0xf],          bits : 3_0
    ife_ord_dark_weight_lut1 (0~8):    [0x0, 0xf],          bits : 7_4
    ife_ord_dark_weight_lut2 (0~8):    [0x0, 0xf],          bits : 11_8
    ife_ord_dark_weight_lut3 (0~8):    [0x0, 0xf],          bits : 15_12
    ife_ord_dark_weight_lut4 (0~8):    [0x0, 0xf],          bits : 19_16
    ife_ord_dark_weight_lut5 (0~8):    [0x0, 0xf],          bits : 23_20
    ife_ord_dark_weight_lut6 (0~8):    [0x0, 0xf],          bits : 27_24
    ife_ord_dark_weight_lut7 (0~8):    [0x0, 0xf],          bits : 31_28
*/
#define OUTLIER_ORDER_REGISTER_2_OFS 0x01b0
REGDEF_BEGIN(OUTLIER_ORDER_REGISTER_2)
REGDEF_BIT(ife_ord_dark_weight_lut0,        4)
REGDEF_BIT(ife_ord_dark_weight_lut1,        4)
REGDEF_BIT(ife_ord_dark_weight_lut2,        4)
REGDEF_BIT(ife_ord_dark_weight_lut3,        4)
REGDEF_BIT(ife_ord_dark_weight_lut4,        4)
REGDEF_BIT(ife_ord_dark_weight_lut5,        4)
REGDEF_BIT(ife_ord_dark_weight_lut6,        4)
REGDEF_BIT(ife_ord_dark_weight_lut7,        4)
REGDEF_END(OUTLIER_ORDER_REGISTER_2)


/*
    ife_gbal_edge_protect_en:    [0x0, 0x1],            bits : 0
    ife_gbal_diff_thr_str   :    [0x0, 0x3ff],          bits : 17_8
    ife_gbal_diff_w_max     :    [0x0, 0xf],            bits : 23_20
*/
#define GBAL_REGISTER_0_OFS 0x01b4
REGDEF_BEGIN(GBAL_REGISTER_0)
REGDEF_BIT(ife_gbal_edge_protect_en,        1)
REGDEF_BIT(,        7)
REGDEF_BIT(ife_gbal_diff_thr_str,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_diff_w_max,        4)
REGDEF_END(GBAL_REGISTER_0)


/*
    ife_gbal_edge_thr_1:    [0x0, 0x3ff],           bits : 9_0
    ife_gbal_edge_thr_0:    [0x0, 0x3ff],           bits : 25_16
*/
#define GBAL_REGISTER_1_OFS 0x01b8
REGDEF_BEGIN(GBAL_REGISTER_1)
REGDEF_BIT(ife_gbal_edge_thr_1,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_gbal_edge_thr_0,        10)
REGDEF_END(GBAL_REGISTER_1)


/*
    ife_gbal_edge_w_max:    [0x0, 0xff],            bits : 7_0
    ife_gbal_edge_w_min:    [0x0, 0xff],            bits : 15_8
*/
#define GBAL_REGISTER_2_OFS 0x01bc
REGDEF_BEGIN(GBAL_REGISTER_2)
REGDEF_BIT(ife_gbal_edge_w_max,        8)
REGDEF_BIT(ife_gbal_edge_w_min,        8)
REGDEF_END(GBAL_REGISTER_2)


/*
    ife_rth_nlm_c0_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_32_OFS 0x01c0
REGDEF_BEGIN(RANGE_FILTER_REGISTER_32)
REGDEF_BIT(ife_rth_nlm_c0_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_32)


/*
    ife_rth_nlm_c0_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_33_OFS 0x01c4
REGDEF_BEGIN(RANGE_FILTER_REGISTER_33)
REGDEF_BIT(ife_rth_nlm_c0_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_33)


/*
    ife_rth_nlm_c0_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_34_OFS 0x01c8
REGDEF_BEGIN(RANGE_FILTER_REGISTER_34)
REGDEF_BIT(ife_rth_nlm_c0_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_34)


/*
    ife_rth_nlm_c0_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_35_OFS 0x01cc
REGDEF_BEGIN(RANGE_FILTER_REGISTER_35)
REGDEF_BIT(ife_rth_nlm_c0_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_7,        10)
REGDEF_END(RANGE_FILTER_REGISTER_35)


/*
    ife_rth_nlm_c0_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c0_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_36_OFS 0x01d0
REGDEF_BEGIN(RANGE_FILTER_REGISTER_36)
REGDEF_BIT(ife_rth_nlm_c0_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_9,        10)
REGDEF_END(RANGE_FILTER_REGISTER_36)


/*
    ife_rth_nlm_c0_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c0_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_37_OFS 0x01d4
REGDEF_BEGIN(RANGE_FILTER_REGISTER_37)
REGDEF_BIT(ife_rth_nlm_c0_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_11,        10)
REGDEF_END(RANGE_FILTER_REGISTER_37)


/*
    ife_rth_nlm_c0_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c0_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_38_OFS 0x01d8
REGDEF_BEGIN(RANGE_FILTER_REGISTER_38)
REGDEF_BIT(ife_rth_nlm_c0_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_13,        10)
REGDEF_END(RANGE_FILTER_REGISTER_38)


/*
    ife_rth_nlm_c0_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c0_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_39_OFS 0x01dc
REGDEF_BEGIN(RANGE_FILTER_REGISTER_39)
REGDEF_BIT(ife_rth_nlm_c0_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c0_lut_15,        10)
REGDEF_END(RANGE_FILTER_REGISTER_39)


/*
    ife_rth_nlm_c0_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define RANGE_FILTER_REGISTER_40_OFS 0x01e0
REGDEF_BEGIN(RANGE_FILTER_REGISTER_40)
REGDEF_BIT(ife_rth_nlm_c0_lut_16,        10)
REGDEF_END(RANGE_FILTER_REGISTER_40)


/*
    ife_rth_nlm_c1_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_41_OFS 0x01e4
REGDEF_BEGIN(RANGE_FILTER_REGISTER_41)
REGDEF_BIT(ife_rth_nlm_c1_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_41)


/*
    ife_rth_nlm_c1_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_42_OFS 0x01e8
REGDEF_BEGIN(RANGE_FILTER_REGISTER_42)
REGDEF_BIT(ife_rth_nlm_c1_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_42)


/*
    ife_rth_nlm_c1_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_43_OFS 0x01ec
REGDEF_BEGIN(RANGE_FILTER_REGISTER_43)
REGDEF_BIT(ife_rth_nlm_c1_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_43)


/*
    ife_rth_nlm_c1_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_44_OFS 0x01f0
REGDEF_BEGIN(RANGE_FILTER_REGISTER_44)
REGDEF_BIT(ife_rth_nlm_c1_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_7,        10)
REGDEF_END(RANGE_FILTER_REGISTER_44)


/*
    ife_rth_nlm_c1_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c1_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_45_OFS 0x01f4
REGDEF_BEGIN(RANGE_FILTER_REGISTER_45)
REGDEF_BIT(ife_rth_nlm_c1_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_9,        10)
REGDEF_END(RANGE_FILTER_REGISTER_45)


/*
    ife_rth_nlm_c1_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c1_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_46_OFS 0x01f8
REGDEF_BEGIN(RANGE_FILTER_REGISTER_46)
REGDEF_BIT(ife_rth_nlm_c1_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_11,        10)
REGDEF_END(RANGE_FILTER_REGISTER_46)


/*
    ife_rth_nlm_c1_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c1_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_47_OFS 0x01fc
REGDEF_BEGIN(RANGE_FILTER_REGISTER_47)
REGDEF_BIT(ife_rth_nlm_c1_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_13,        10)
REGDEF_END(RANGE_FILTER_REGISTER_47)


/*
    ife_rth_nlm_c1_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c1_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_48_OFS 0x0200
REGDEF_BEGIN(RANGE_FILTER_REGISTER_48)
REGDEF_BIT(ife_rth_nlm_c1_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c1_lut_15,        10)
REGDEF_END(RANGE_FILTER_REGISTER_48)


/*
    ife_rth_nlm_c1_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define RANGE_FILTER_REGISTER_49_OFS 0x0204
REGDEF_BEGIN(RANGE_FILTER_REGISTER_49)
REGDEF_BIT(ife_rth_nlm_c1_lut_16,        10)
REGDEF_END(RANGE_FILTER_REGISTER_49)


/*
    ife_rth_nlm_c2_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_50_OFS 0x0208
REGDEF_BEGIN(RANGE_FILTER_REGISTER_50)
REGDEF_BIT(ife_rth_nlm_c2_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_50)


/*
    ife_rth_nlm_c2_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_51_OFS 0x020c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_51)
REGDEF_BIT(ife_rth_nlm_c2_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_51)


/*
    ife_rth_nlm_c2_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_52_OFS 0x0210
REGDEF_BEGIN(RANGE_FILTER_REGISTER_52)
REGDEF_BIT(ife_rth_nlm_c2_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_52)


/*
    ife_rth_nlm_c2_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_53_OFS 0x0214
REGDEF_BEGIN(RANGE_FILTER_REGISTER_53)
REGDEF_BIT(ife_rth_nlm_c2_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_7,        10)
REGDEF_END(RANGE_FILTER_REGISTER_53)


/*
    ife_rth_nlm_c2_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c2_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_54_OFS 0x0218
REGDEF_BEGIN(RANGE_FILTER_REGISTER_54)
REGDEF_BIT(ife_rth_nlm_c2_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_9,        10)
REGDEF_END(RANGE_FILTER_REGISTER_54)


/*
    ife_rth_nlm_c2_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c2_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_55_OFS 0x021c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_55)
REGDEF_BIT(ife_rth_nlm_c2_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_11,        10)
REGDEF_END(RANGE_FILTER_REGISTER_55)


/*
    ife_rth_nlm_c2_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c2_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_56_OFS 0x0220
REGDEF_BEGIN(RANGE_FILTER_REGISTER_56)
REGDEF_BIT(ife_rth_nlm_c2_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_13,        10)
REGDEF_END(RANGE_FILTER_REGISTER_56)


/*
    ife_rth_nlm_c2_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c2_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_57_OFS 0x0224
REGDEF_BEGIN(RANGE_FILTER_REGISTER_57)
REGDEF_BIT(ife_rth_nlm_c2_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c2_lut_15,        10)
REGDEF_END(RANGE_FILTER_REGISTER_57)


/*
    ife_rth_nlm_c2_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define RANGE_FILTER_REGISTER_58_OFS 0x0228
REGDEF_BEGIN(RANGE_FILTER_REGISTER_58)
REGDEF_BIT(ife_rth_nlm_c2_lut_16,        10)
REGDEF_END(RANGE_FILTER_REGISTER_58)


/*
    ife_rth_nlm_c3_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_59_OFS 0x022c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_59)
REGDEF_BIT(ife_rth_nlm_c3_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_59)


/*
    ife_rth_nlm_c3_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_60_OFS 0x0230
REGDEF_BEGIN(RANGE_FILTER_REGISTER_60)
REGDEF_BIT(ife_rth_nlm_c3_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_60)


/*
    ife_rth_nlm_c3_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_61_OFS 0x0234
REGDEF_BEGIN(RANGE_FILTER_REGISTER_61)
REGDEF_BIT(ife_rth_nlm_c3_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_61)


/*
    ife_rth_nlm_c3_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_62_OFS 0x0238
REGDEF_BEGIN(RANGE_FILTER_REGISTER_62)
REGDEF_BIT(ife_rth_nlm_c3_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_7,        10)
REGDEF_END(RANGE_FILTER_REGISTER_62)


/*
    ife_rth_nlm_c3_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_rth_nlm_c3_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define RANGE_FILTER_REGISTER_63_OFS 0x023c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_63)
REGDEF_BIT(ife_rth_nlm_c3_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_9,        10)
REGDEF_END(RANGE_FILTER_REGISTER_63)


/*
    ife_rth_nlm_c3_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c3_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_64_OFS 0x0240
REGDEF_BEGIN(RANGE_FILTER_REGISTER_64)
REGDEF_BIT(ife_rth_nlm_c3_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_11,        10)
REGDEF_END(RANGE_FILTER_REGISTER_64)


/*
    ife_rth_nlm_c3_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c3_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_65_OFS 0x0244
REGDEF_BEGIN(RANGE_FILTER_REGISTER_65)
REGDEF_BIT(ife_rth_nlm_c3_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_13,        10)
REGDEF_END(RANGE_FILTER_REGISTER_65)


/*
    ife_rth_nlm_c3_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_rth_nlm_c3_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define RANGE_FILTER_REGISTER_66_OFS 0x0248
REGDEF_BEGIN(RANGE_FILTER_REGISTER_66)
REGDEF_BIT(ife_rth_nlm_c3_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_nlm_c3_lut_15,        10)
REGDEF_END(RANGE_FILTER_REGISTER_66)


/*
    ife_rth_nlm_c3_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define RANGE_FILTER_REGISTER_67_OFS 0x024c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_67)
REGDEF_BIT(ife_rth_nlm_c3_lut_16,        10)
REGDEF_END(RANGE_FILTER_REGISTER_67)


/*
    ife_rth_bilat_c0_lut_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_lut_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_68_OFS 0x0250
REGDEF_BEGIN(RANGE_FILTER_REGISTER_68)
REGDEF_BIT(ife_rth_bilat_c0_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_68)


/*
    ife_rth_bilat_c0_lut_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_lut_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_69_OFS 0x0254
REGDEF_BEGIN(RANGE_FILTER_REGISTER_69)
REGDEF_BIT(ife_rth_bilat_c0_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_69)


/*
    ife_rth_bilat_c0_lut_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_lut_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_70_OFS 0x0258
REGDEF_BEGIN(RANGE_FILTER_REGISTER_70)
REGDEF_BIT(ife_rth_bilat_c0_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_70)


/*
    ife_rth_bilat_c0_lut_6:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_lut_7:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_71_OFS 0x025c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_71)
REGDEF_BIT(ife_rth_bilat_c0_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_7,        10)
REGDEF_END(RANGE_FILTER_REGISTER_71)


/*
    ife_rth_bilat_c0_lut_8:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c0_lut_9:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_72_OFS 0x0260
REGDEF_BEGIN(RANGE_FILTER_REGISTER_72)
REGDEF_BIT(ife_rth_bilat_c0_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_9,        10)
REGDEF_END(RANGE_FILTER_REGISTER_72)


/*
    ife_rth_bilat_c0_lut_10:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c0_lut_11:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_73_OFS 0x0264
REGDEF_BEGIN(RANGE_FILTER_REGISTER_73)
REGDEF_BIT(ife_rth_bilat_c0_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_11,        10)
REGDEF_END(RANGE_FILTER_REGISTER_73)


/*
    ife_rth_bilat_c0_lut_12:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c0_lut_13:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_74_OFS 0x0268
REGDEF_BEGIN(RANGE_FILTER_REGISTER_74)
REGDEF_BIT(ife_rth_bilat_c0_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_13,        10)
REGDEF_END(RANGE_FILTER_REGISTER_74)


/*
    ife_rth_bilat_c0_lut_14:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c0_lut_15:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_75_OFS 0x026c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_75)
REGDEF_BIT(ife_rth_bilat_c0_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c0_lut_15,        10)
REGDEF_END(RANGE_FILTER_REGISTER_75)


/*
    ife_rth_bilat_c0_lut_16:    [0x0, 0x3ff],           bits : 9_0
*/
#define RANGE_FILTER_REGISTER_76_OFS 0x0270
REGDEF_BEGIN(RANGE_FILTER_REGISTER_76)
REGDEF_BIT(ife_rth_bilat_c0_lut_16,        10)
REGDEF_END(RANGE_FILTER_REGISTER_76)


/*
    ife_rth_bilat_c1_lut_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_lut_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_77_OFS 0x0274
REGDEF_BEGIN(RANGE_FILTER_REGISTER_77)
REGDEF_BIT(ife_rth_bilat_c1_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_77)


/*
    ife_rth_bilat_c1_lut_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_lut_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_78_OFS 0x0278
REGDEF_BEGIN(RANGE_FILTER_REGISTER_78)
REGDEF_BIT(ife_rth_bilat_c1_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_78)


/*
    ife_rth_bilat_c1_lut_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_lut_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_79_OFS 0x027c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_79)
REGDEF_BIT(ife_rth_bilat_c1_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_79)


/*
    ife_rth_bilat_c1_lut_6:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_lut_7:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_80_OFS 0x0280
REGDEF_BEGIN(RANGE_FILTER_REGISTER_80)
REGDEF_BIT(ife_rth_bilat_c1_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_7,        10)
REGDEF_END(RANGE_FILTER_REGISTER_80)


/*
    ife_rth_bilat_c1_lut_8:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c1_lut_9:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_81_OFS 0x0284
REGDEF_BEGIN(RANGE_FILTER_REGISTER_81)
REGDEF_BIT(ife_rth_bilat_c1_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_9,        10)
REGDEF_END(RANGE_FILTER_REGISTER_81)


/*
    ife_rth_bilat_c1_lut_10:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c1_lut_11:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_82_OFS 0x0288
REGDEF_BEGIN(RANGE_FILTER_REGISTER_82)
REGDEF_BIT(ife_rth_bilat_c1_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_11,        10)
REGDEF_END(RANGE_FILTER_REGISTER_82)


/*
    ife_rth_bilat_c1_lut_12:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c1_lut_13:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_83_OFS 0x028c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_83)
REGDEF_BIT(ife_rth_bilat_c1_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_13,        10)
REGDEF_END(RANGE_FILTER_REGISTER_83)


/*
    ife_rth_bilat_c1_lut_14:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c1_lut_15:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_84_OFS 0x0290
REGDEF_BEGIN(RANGE_FILTER_REGISTER_84)
REGDEF_BIT(ife_rth_bilat_c1_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c1_lut_15,        10)
REGDEF_END(RANGE_FILTER_REGISTER_84)


/*
    ife_rth_bilat_c1_lut_16:    [0x0, 0x3ff],           bits : 9_0
*/
#define RANGE_FILTER_REGISTER_85_OFS 0x0294
REGDEF_BEGIN(RANGE_FILTER_REGISTER_85)
REGDEF_BIT(ife_rth_bilat_c1_lut_16,        10)
REGDEF_END(RANGE_FILTER_REGISTER_85)


/*
    ife_rth_bilat_c2_lut_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_lut_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_86_OFS 0x0298
REGDEF_BEGIN(RANGE_FILTER_REGISTER_86)
REGDEF_BIT(ife_rth_bilat_c2_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_86)


/*
    ife_rth_bilat_c2_lut_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_lut_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_87_OFS 0x029c
REGDEF_BEGIN(RANGE_FILTER_REGISTER_87)
REGDEF_BIT(ife_rth_bilat_c2_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_87)


/*
    ife_rth_bilat_c2_lut_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_lut_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_88_OFS 0x02a0
REGDEF_BEGIN(RANGE_FILTER_REGISTER_88)
REGDEF_BIT(ife_rth_bilat_c2_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_88)


/*
    ife_rth_bilat_c2_lut_6:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_lut_7:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_89_OFS 0x02a4
REGDEF_BEGIN(RANGE_FILTER_REGISTER_89)
REGDEF_BIT(ife_rth_bilat_c2_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_7,        10)
REGDEF_END(RANGE_FILTER_REGISTER_89)


/*
    ife_rth_bilat_c2_lut_8:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c2_lut_9:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_90_OFS 0x02a8
REGDEF_BEGIN(RANGE_FILTER_REGISTER_90)
REGDEF_BIT(ife_rth_bilat_c2_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_9,        10)
REGDEF_END(RANGE_FILTER_REGISTER_90)


/*
    ife_rth_bilat_c2_lut_10:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c2_lut_11:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_91_OFS 0x02ac
REGDEF_BEGIN(RANGE_FILTER_REGISTER_91)
REGDEF_BIT(ife_rth_bilat_c2_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_11,        10)
REGDEF_END(RANGE_FILTER_REGISTER_91)


/*
    ife_rth_bilat_c2_lut_12:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c2_lut_13:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_92_OFS 0x02b0
REGDEF_BEGIN(RANGE_FILTER_REGISTER_92)
REGDEF_BIT(ife_rth_bilat_c2_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_13,        10)
REGDEF_END(RANGE_FILTER_REGISTER_92)


/*
    ife_rth_bilat_c2_lut_14:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c2_lut_15:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_93_OFS 0x02b4
REGDEF_BEGIN(RANGE_FILTER_REGISTER_93)
REGDEF_BIT(ife_rth_bilat_c2_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c2_lut_15,        10)
REGDEF_END(RANGE_FILTER_REGISTER_93)


/*
    ife_rth_bilat_c2_lut_16:    [0x0, 0x3ff],           bits : 9_0
*/
#define RANGE_FILTER_REGISTER_94_OFS 0x02b8
REGDEF_BEGIN(RANGE_FILTER_REGISTER_94)
REGDEF_BIT(ife_rth_bilat_c2_lut_16,        10)
REGDEF_END(RANGE_FILTER_REGISTER_94)


/*
    ife_rth_bilat_c3_lut_0:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_lut_1:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_95_OFS 0x02bc
REGDEF_BEGIN(RANGE_FILTER_REGISTER_95)
REGDEF_BIT(ife_rth_bilat_c3_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_1,        10)
REGDEF_END(RANGE_FILTER_REGISTER_95)


/*
    ife_rth_bilat_c3_lut_2:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_lut_3:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_96_OFS 0x02c0
REGDEF_BEGIN(RANGE_FILTER_REGISTER_96)
REGDEF_BIT(ife_rth_bilat_c3_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_3,        10)
REGDEF_END(RANGE_FILTER_REGISTER_96)


/*
    ife_rth_bilat_c3_lut_4:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_lut_5:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_97_OFS 0x02c4
REGDEF_BEGIN(RANGE_FILTER_REGISTER_97)
REGDEF_BIT(ife_rth_bilat_c3_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_5,        10)
REGDEF_END(RANGE_FILTER_REGISTER_97)


/*
    ife_rth_bilat_c3_lut_6:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_lut_7:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_98_OFS 0x02c8
REGDEF_BEGIN(RANGE_FILTER_REGISTER_98)
REGDEF_BIT(ife_rth_bilat_c3_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_7,        10)
REGDEF_END(RANGE_FILTER_REGISTER_98)


/*
    ife_rth_bilat_c3_lut_8:    [0x0, 0x3ff],            bits : 9_0
    ife_rth_bilat_c3_lut_9:    [0x0, 0x3ff],            bits : 25_16
*/
#define RANGE_FILTER_REGISTER_99_OFS 0x02cc
REGDEF_BEGIN(RANGE_FILTER_REGISTER_99)
REGDEF_BIT(ife_rth_bilat_c3_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_9,        10)
REGDEF_END(RANGE_FILTER_REGISTER_99)


/*
    ife_rth_bilat_c3_lut_10:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c3_lut_11:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_100_OFS 0x02d0
REGDEF_BEGIN(RANGE_FILTER_REGISTER_100)
REGDEF_BIT(ife_rth_bilat_c3_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_11,        10)
REGDEF_END(RANGE_FILTER_REGISTER_100)


/*
    ife_rth_bilat_c3_lut_12:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c3_lut_13:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_101_OFS 0x02d4
REGDEF_BEGIN(RANGE_FILTER_REGISTER_101)
REGDEF_BIT(ife_rth_bilat_c3_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_13,        10)
REGDEF_END(RANGE_FILTER_REGISTER_101)


/*
    ife_rth_bilat_c3_lut_14:    [0x0, 0x3ff],           bits : 9_0
    ife_rth_bilat_c3_lut_15:    [0x0, 0x3ff],           bits : 25_16
*/
#define RANGE_FILTER_REGISTER_102_OFS 0x02d8
REGDEF_BEGIN(RANGE_FILTER_REGISTER_102)
REGDEF_BIT(ife_rth_bilat_c3_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_rth_bilat_c3_lut_15,        10)
REGDEF_END(RANGE_FILTER_REGISTER_102)


/*
    ife_rth_bilat_c3_lut_16:    [0x0, 0x3ff],           bits : 9_0
*/
#define RANGE_FILTER_REGISTER_103_OFS 0x02dc
REGDEF_BEGIN(RANGE_FILTER_REGISTER_103)
REGDEF_BIT(ife_rth_bilat_c3_lut_16,        10)
REGDEF_END(RANGE_FILTER_REGISTER_103)


/*
    ife_distvgtx_c0:    [0x0, 0x3fff],          bits : 13_0
    ife_distvgty_c0:    [0x0, 0x3fff],          bits : 29_16
    ife_distgain   :    [0x0, 0x3],         bits : 31_30
*/
#define VIGNETTE_SETTING_REGISTER_0_OFS 0x02e0
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_0)
REGDEF_BIT(ife_distvgtx_c0,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_distvgty_c0,        14)
REGDEF_BIT(ife_distgain,        2)
REGDEF_END(VIGNETTE_SETTING_REGISTER_0)


/*
    ife_distvgtx_c1:    [0x0, 0x3fff],          bits : 13_0
    ife_distvgty_c1:    [0x0, 0x3fff],          bits : 29_16
*/
#define VIGNETTE_SETTING_REGISTER_1_OFS 0x02e4
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_1)
REGDEF_BIT(ife_distvgtx_c1,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_distvgty_c1,        14)
REGDEF_END(VIGNETTE_SETTING_REGISTER_1)


/*
    ife_distvgtx_c2:    [0x0, 0x3fff],          bits : 13_0
    ife_distvgty_c2:    [0x0, 0x3fff],          bits : 29_16
*/
#define VIGNETTE_SETTING_REGISTER_2_OFS 0x02e8
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_2)
REGDEF_BIT(ife_distvgtx_c2,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_distvgty_c2,        14)
REGDEF_END(VIGNETTE_SETTING_REGISTER_2)


/*
    ife_distvgtx_c3:    [0x0, 0x3fff],          bits : 13_0
    ife_distvgty_c3:    [0x0, 0x3fff],          bits : 29_16
*/
#define VIGNETTE_SETTING_REGISTER_3_OFS 0x02ec
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_3)
REGDEF_BIT(ife_distvgtx_c3,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_distvgty_c3,        14)
REGDEF_END(VIGNETTE_SETTING_REGISTER_3)


/*
    ife_distvgxdiv:    [0x0, 0xfff],            bits : 11_0
    ife_distvgydiv:    [0x0, 0xfff],            bits : 23_12
*/
#define VIGNETTE_SETTING_REGISTER_4_OFS 0x02f0
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_4)
REGDEF_BIT(ife_distvgxdiv,        12)
REGDEF_BIT(ife_distvgydiv,        12)
REGDEF_END(VIGNETTE_SETTING_REGISTER_4)


/*
    distdthr_en :    [0x0, 0x1],            bits : 0
    distdthr_rst:    [0x0, 0x1],            bits : 4
    distthr     :    [0x0, 0x3ff],          bits : 17_8
*/
#define VIGNETTE_SETTING_REGISTER_5_OFS 0x02f4
REGDEF_BEGIN(VIGNETTE_SETTING_REGISTER_5)
REGDEF_BIT(distdthr_en,        1)
REGDEF_BIT(ife_vig_fisheye_gain_en, 1)
REGDEF_BIT(,        2)
REGDEF_BIT(distdthr_rst,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(distthr,        10)
REGDEF_BIT(ife_vig_fisheye_slope,        4)
REGDEF_BIT(ife_vig_fisheye_radius,      10)
REGDEF_END(VIGNETTE_SETTING_REGISTER_5)


/*
    ife_vig_c0_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c0_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_0_OFS 0x02f8
REGDEF_BEGIN(VIGNETTE_REGISTER_0)
REGDEF_BIT(ife_vig_c0_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_1,        10)
REGDEF_END(VIGNETTE_REGISTER_0)


/*
    ife_vig_c0_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c0_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_1_OFS 0x02fc
REGDEF_BEGIN(VIGNETTE_REGISTER_1)
REGDEF_BIT(ife_vig_c0_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_3,        10)
REGDEF_END(VIGNETTE_REGISTER_1)


/*
    ife_vig_c0_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c0_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_2_OFS 0x0300
REGDEF_BEGIN(VIGNETTE_REGISTER_2)
REGDEF_BIT(ife_vig_c0_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_5,        10)
REGDEF_END(VIGNETTE_REGISTER_2)


/*
    ife_vig_c0_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c0_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_3_OFS 0x0304
REGDEF_BEGIN(VIGNETTE_REGISTER_3)
REGDEF_BIT(ife_vig_c0_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_7,        10)
REGDEF_END(VIGNETTE_REGISTER_3)


/*
    ife_vig_c0_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c0_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_4_OFS 0x0308
REGDEF_BEGIN(VIGNETTE_REGISTER_4)
REGDEF_BIT(ife_vig_c0_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_9,        10)
REGDEF_END(VIGNETTE_REGISTER_4)


/*
    ife_vig_c0_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c0_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_5_OFS 0x030c
REGDEF_BEGIN(VIGNETTE_REGISTER_5)
REGDEF_BIT(ife_vig_c0_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_11,        10)
REGDEF_END(VIGNETTE_REGISTER_5)


/*
    ife_vig_c0_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c0_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_6_OFS 0x0310
REGDEF_BEGIN(VIGNETTE_REGISTER_6)
REGDEF_BIT(ife_vig_c0_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_13,        10)
REGDEF_END(VIGNETTE_REGISTER_6)


/*
    ife_vig_c0_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c0_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_7_OFS 0x0314
REGDEF_BEGIN(VIGNETTE_REGISTER_7)
REGDEF_BIT(ife_vig_c0_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c0_lut_15,        10)
REGDEF_END(VIGNETTE_REGISTER_7)


/*
    ife_vig_c0_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define VIGNETTE_REGISTER_8_OFS 0x0318
REGDEF_BEGIN(VIGNETTE_REGISTER_8)
REGDEF_BIT(ife_vig_c0_lut_16,        10)
REGDEF_END(VIGNETTE_REGISTER_8)


/*
    ife_vig_c1_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c1_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_9_OFS 0x031c
REGDEF_BEGIN(VIGNETTE_REGISTER_9)
REGDEF_BIT(ife_vig_c1_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_1,        10)
REGDEF_END(VIGNETTE_REGISTER_9)


/*
    ife_vig_c1_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c1_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_10_OFS 0x0320
REGDEF_BEGIN(VIGNETTE_REGISTER_10)
REGDEF_BIT(ife_vig_c1_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_3,        10)
REGDEF_END(VIGNETTE_REGISTER_10)


/*
    ife_vig_c1_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c1_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_11_OFS 0x0324
REGDEF_BEGIN(VIGNETTE_REGISTER_11)
REGDEF_BIT(ife_vig_c1_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_5,        10)
REGDEF_END(VIGNETTE_REGISTER_11)


/*
    ife_vig_c1_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c1_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_12_OFS 0x0328
REGDEF_BEGIN(VIGNETTE_REGISTER_12)
REGDEF_BIT(ife_vig_c1_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_7,        10)
REGDEF_END(VIGNETTE_REGISTER_12)


/*
    ife_vig_c1_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c1_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_13_OFS 0x032c
REGDEF_BEGIN(VIGNETTE_REGISTER_13)
REGDEF_BIT(ife_vig_c1_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_9,        10)
REGDEF_END(VIGNETTE_REGISTER_13)


/*
    ife_vig_c1_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c1_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_14_OFS 0x0330
REGDEF_BEGIN(VIGNETTE_REGISTER_14)
REGDEF_BIT(ife_vig_c1_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_11,        10)
REGDEF_END(VIGNETTE_REGISTER_14)


/*
    ife_vig_c1_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c1_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_15_OFS 0x0334
REGDEF_BEGIN(VIGNETTE_REGISTER_15)
REGDEF_BIT(ife_vig_c1_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_13,        10)
REGDEF_END(VIGNETTE_REGISTER_15)


/*
    ife_vig_c1_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c1_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_16_OFS 0x0338
REGDEF_BEGIN(VIGNETTE_REGISTER_16)
REGDEF_BIT(ife_vig_c1_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c1_lut_15,        10)
REGDEF_END(VIGNETTE_REGISTER_16)


/*
    ife_vig_c1_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define VIGNETTE_REGISTER_17_OFS 0x033c
REGDEF_BEGIN(VIGNETTE_REGISTER_17)
REGDEF_BIT(ife_vig_c1_lut_16,        10)
REGDEF_END(VIGNETTE_REGISTER_17)


/*
    ife_vig_c2_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c2_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_18_OFS 0x0340
REGDEF_BEGIN(VIGNETTE_REGISTER_18)
REGDEF_BIT(ife_vig_c2_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_1,        10)
REGDEF_END(VIGNETTE_REGISTER_18)


/*
    ife_vig_c2_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c2_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_19_OFS 0x0344
REGDEF_BEGIN(VIGNETTE_REGISTER_19)
REGDEF_BIT(ife_vig_c2_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_3,        10)
REGDEF_END(VIGNETTE_REGISTER_19)


/*
    ife_vig_c2_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c2_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_20_OFS 0x0348
REGDEF_BEGIN(VIGNETTE_REGISTER_20)
REGDEF_BIT(ife_vig_c2_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_5,        10)
REGDEF_END(VIGNETTE_REGISTER_20)


/*
    ife_vig_c2_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c2_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_21_OFS 0x034c
REGDEF_BEGIN(VIGNETTE_REGISTER_21)
REGDEF_BIT(ife_vig_c2_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_7,        10)
REGDEF_END(VIGNETTE_REGISTER_21)


/*
    ife_vig_c2_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c2_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_22_OFS 0x0350
REGDEF_BEGIN(VIGNETTE_REGISTER_22)
REGDEF_BIT(ife_vig_c2_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_9,        10)
REGDEF_END(VIGNETTE_REGISTER_22)


/*
    ife_vig_c2_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c2_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_23_OFS 0x0354
REGDEF_BEGIN(VIGNETTE_REGISTER_23)
REGDEF_BIT(ife_vig_c2_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_11,        10)
REGDEF_END(VIGNETTE_REGISTER_23)


/*
    ife_vig_c2_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c2_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_24_OFS 0x0358
REGDEF_BEGIN(VIGNETTE_REGISTER_24)
REGDEF_BIT(ife_vig_c2_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_13,        10)
REGDEF_END(VIGNETTE_REGISTER_24)


/*
    ife_vig_c2_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c2_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_25_OFS 0x035c
REGDEF_BEGIN(VIGNETTE_REGISTER_25)
REGDEF_BIT(ife_vig_c2_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c2_lut_15,        10)
REGDEF_END(VIGNETTE_REGISTER_25)


/*
    ife_vig_c2_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define VIGNETTE_REGISTER_26_OFS 0x0360
REGDEF_BEGIN(VIGNETTE_REGISTER_26)
REGDEF_BIT(ife_vig_c2_lut_16,        10)
REGDEF_END(VIGNETTE_REGISTER_26)


/*
    ife_vig_c3_lut_0:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c3_lut_1:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_27_OFS 0x0364
REGDEF_BEGIN(VIGNETTE_REGISTER_27)
REGDEF_BIT(ife_vig_c3_lut_0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_1,        10)
REGDEF_END(VIGNETTE_REGISTER_27)


/*
    ife_vig_c3_lut_2:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c3_lut_3:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_28_OFS 0x0368
REGDEF_BEGIN(VIGNETTE_REGISTER_28)
REGDEF_BIT(ife_vig_c3_lut_2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_3,        10)
REGDEF_END(VIGNETTE_REGISTER_28)


/*
    ife_vig_c3_lut_4:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c3_lut_5:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_29_OFS 0x036c
REGDEF_BEGIN(VIGNETTE_REGISTER_29)
REGDEF_BIT(ife_vig_c3_lut_4,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_5,        10)
REGDEF_END(VIGNETTE_REGISTER_29)


/*
    ife_vig_c3_lut_6:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c3_lut_7:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_30_OFS 0x0370
REGDEF_BEGIN(VIGNETTE_REGISTER_30)
REGDEF_BIT(ife_vig_c3_lut_6,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_7,        10)
REGDEF_END(VIGNETTE_REGISTER_30)


/*
    ife_vig_c3_lut_8:    [0x0, 0x3ff],          bits : 9_0
    ife_vig_c3_lut_9:    [0x0, 0x3ff],          bits : 25_16
*/
#define VIGNETTE_REGISTER_31_OFS 0x0374
REGDEF_BEGIN(VIGNETTE_REGISTER_31)
REGDEF_BIT(ife_vig_c3_lut_8,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_9,        10)
REGDEF_END(VIGNETTE_REGISTER_31)


/*
    ife_vig_c3_lut_10:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c3_lut_11:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_32_OFS 0x0378
REGDEF_BEGIN(VIGNETTE_REGISTER_32)
REGDEF_BIT(ife_vig_c3_lut_10,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_11,        10)
REGDEF_END(VIGNETTE_REGISTER_32)


/*
    ife_vig_c3_lut_12:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c3_lut_13:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_33_OFS 0x037c
REGDEF_BEGIN(VIGNETTE_REGISTER_33)
REGDEF_BIT(ife_vig_c3_lut_12,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_13,        10)
REGDEF_END(VIGNETTE_REGISTER_33)


/*
    ife_vig_c3_lut_14:    [0x0, 0x3ff],         bits : 9_0
    ife_vig_c3_lut_15:    [0x0, 0x3ff],         bits : 25_16
*/
#define VIGNETTE_REGISTER_34_OFS 0x0380
REGDEF_BEGIN(VIGNETTE_REGISTER_34)
REGDEF_BIT(ife_vig_c3_lut_14,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(ife_vig_c3_lut_15,        10)
REGDEF_END(VIGNETTE_REGISTER_34)


/*
    ife_vig_c3_lut_16:    [0x0, 0x3ff],         bits : 9_0
*/
#define VIGNETTE_REGISTER_35_OFS 0x0384
REGDEF_BEGIN(VIGNETTE_REGISTER_35)
REGDEF_BIT(ife_vig_c3_lut_16,        10)
REGDEF_END(VIGNETTE_REGISTER_35)


/*
    ife_gbal_ofs_lut_00:    [0x0, 0x3f],            bits : 5_0
    ife_gbal_ofs_lut_01:    [0x0, 0x3f],            bits : 13_8
    ife_gbal_ofs_lut_02:    [0x0, 0x3f],            bits : 21_16
    ife_gbal_ofs_lut_03:    [0x0, 0x3f],            bits : 29_24
*/
#define GBAL_REGISTER_3_OFS 0x0388
REGDEF_BEGIN(GBAL_REGISTER_3)
REGDEF_BIT(ife_gbal_ofs_lut_00,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_01,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_02,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_03,        6)
REGDEF_END(GBAL_REGISTER_3)


/*
    ife_gbal_ofs_lut_04:    [0x0, 0x3f],            bits : 5_0
    ife_gbal_ofs_lut_05:    [0x0, 0x3f],            bits : 13_8
    ife_gbal_ofs_lut_06:    [0x0, 0x3f],            bits : 21_16
    ife_gbal_ofs_lut_07:    [0x0, 0x3f],            bits : 29_24
*/
#define GBAL_REGISTER_4_OFS 0x038c
REGDEF_BEGIN(GBAL_REGISTER_4)
REGDEF_BIT(ife_gbal_ofs_lut_04,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_05,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_06,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_07,        6)
REGDEF_END(GBAL_REGISTER_4)


/*
    ife_gbal_ofs_lut_08:    [0x0, 0x3f],            bits : 5_0
    ife_gbal_ofs_lut_09:    [0x0, 0x3f],            bits : 13_8
    ife_gbal_ofs_lut_10:    [0x0, 0x3f],            bits : 21_16
    ife_gbal_ofs_lut_11:    [0x0, 0x3f],            bits : 29_24
*/
#define GBAL_REGISTER_5_OFS 0x0390
REGDEF_BEGIN(GBAL_REGISTER_5)
REGDEF_BIT(ife_gbal_ofs_lut_08,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_09,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_10,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_11,        6)
REGDEF_END(GBAL_REGISTER_5)


/*
    ife_gbal_ofs_lut_12:    [0x0, 0x3f],            bits : 5_0
    ife_gbal_ofs_lut_13:    [0x0, 0x3f],            bits : 13_8
    ife_gbal_ofs_lut_14:    [0x0, 0x3f],            bits : 21_16
    ife_gbal_ofs_lut_15:    [0x0, 0x3f],            bits : 29_24
*/
#define GBAL_REGISTER_6_OFS 0x0394
REGDEF_BEGIN(GBAL_REGISTER_6)
REGDEF_BIT(ife_gbal_ofs_lut_12,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_13,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_14,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_gbal_ofs_lut_15,        6)
REGDEF_END(GBAL_REGISTER_6)


/*
    ife_gbal_ofs_lut_16:    [0x0, 0x3f],            bits : 5_0
*/
#define GBAL_REGISTER_7_OFS 0x0398
REGDEF_BEGIN(GBAL_REGISTER_7)
REGDEF_BIT(ife_gbal_ofs_lut_16,        6)
REGDEF_END(GBAL_REGISTER_7)


/*
    ife_rbluma00:    [0x0, 0x1f],           bits : 4_0
    ife_rbluma01:    [0x0, 0x1f],           bits : 12_8
    ife_rbluma02:    [0x0, 0x1f],           bits : 20_16
    ife_rbluma03:    [0x0, 0x1f],           bits : 28_24
*/
#define RB_LUMINANCE_REGISTER_0_OFS 0x039c
REGDEF_BEGIN(RB_LUMINANCE_REGISTER_0)
REGDEF_BIT(ife_rbluma00,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma01,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma02,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma03,        5)
REGDEF_END(RB_LUMINANCE_REGISTER_0)


/*
    ife_rbluma04:    [0x0, 0x1f],           bits : 4_0
    ife_rbluma05:    [0x0, 0x1f],           bits : 12_8
    ife_rbluma06:    [0x0, 0x1f],           bits : 20_16
    ife_rbluma07:    [0x0, 0x1f],           bits : 28_24
*/
#define RB_LUMINANCE_REGISTER_1_OFS 0x03a0
REGDEF_BEGIN(RB_LUMINANCE_REGISTER_1)
REGDEF_BIT(ife_rbluma04,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma05,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma06,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma07,        5)
REGDEF_END(RB_LUMINANCE_REGISTER_1)


/*
    ife_rbluma08:    [0x0, 0x1f],           bits : 4_0
    ife_rbluma09:    [0x0, 0x1f],           bits : 12_8
    ife_rbluma10:    [0x0, 0x1f],           bits : 20_16
    ife_rbluma11:    [0x0, 0x1f],           bits : 28_24
*/
#define RB_LUMINANCE_REGISTER_2_OFS 0x03a4
REGDEF_BEGIN(RB_LUMINANCE_REGISTER_2)
REGDEF_BIT(ife_rbluma08,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma09,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma10,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma11,        5)
REGDEF_END(RB_LUMINANCE_REGISTER_2)


/*
    ife_rbluma12:    [0x0, 0x1f],           bits : 4_0
    ife_rbluma13:    [0x0, 0x1f],           bits : 12_8
    ife_rbluma14:    [0x0, 0x1f],           bits : 20_16
    ife_rbluma15:    [0x0, 0x1f],           bits : 28_24
*/
#define RB_LUMINANCE_REGISTER_3_OFS 0x03a8
REGDEF_BEGIN(RB_LUMINANCE_REGISTER_3)
REGDEF_BIT(ife_rbluma12,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma13,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma14,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbluma15,        5)
REGDEF_END(RB_LUMINANCE_REGISTER_3)


/*
    ife_rbluma16:    [0x0, 0x1f],           bits : 4_0
*/
#define RB_LUMINANCE_REGISTER_4_OFS 0x03ac
REGDEF_BEGIN(RB_LUMINANCE_REGISTER_4)
REGDEF_BIT(ife_rbluma16,        5)
REGDEF_END(RB_LUMINANCE_REGISTER_4)

/*
    ife_rbratio00:    [0x0, 0x1f],          bits : 4_0
    ife_rbratio01:    [0x0, 0x1f],          bits : 12_8
    ife_rbratio02:    [0x0, 0x1f],          bits : 20_16
    ife_rbratio03:    [0x0, 0x1f],          bits : 28_24
*/
#define RB_RATIO_REGISTER_0_OFS 0x03c0
REGDEF_BEGIN(RB_RATIO_REGISTER_0)
REGDEF_BIT(ife_rbratio00,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio01,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio02,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio03,        5)
REGDEF_END(RB_RATIO_REGISTER_0)


/*
    ife_rbratio04:    [0x0, 0x1f],          bits : 4_0
    ife_rbratio05:    [0x0, 0x1f],          bits : 12_8
    ife_rbratio06:    [0x0, 0x1f],          bits : 20_16
    ife_rbratio07:    [0x0, 0x1f],          bits : 28_24
*/
#define RB_RATIO_REGISTER_1_OFS 0x03c4
REGDEF_BEGIN(RB_RATIO_REGISTER_1)
REGDEF_BIT(ife_rbratio04,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio05,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio06,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio07,        5)
REGDEF_END(RB_RATIO_REGISTER_1)


/*
    ife_rbratio08:    [0x0, 0x1f],          bits : 4_0
    ife_rbratio09:    [0x0, 0x1f],          bits : 12_8
    ife_rbratio10:    [0x0, 0x1f],          bits : 20_16
    ife_rbratio11:    [0x0, 0x1f],          bits : 28_24
*/
#define RB_RATIO_REGISTER_2_OFS 0x03c8
REGDEF_BEGIN(RB_RATIO_REGISTER_2)
REGDEF_BIT(ife_rbratio08,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio09,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio10,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio11,        5)
REGDEF_END(RB_RATIO_REGISTER_2)


/*
    ife_rbratio12:    [0x0, 0x1f],          bits : 4_0
    ife_rbratio13:    [0x0, 0x1f],          bits : 12_8
    ife_rbratio14:    [0x0, 0x1f],          bits : 20_16
    ife_rbratio15:    [0x0, 0x1f],          bits : 28_24
*/
#define RB_RATIO_REGISTER_3_OFS 0x03cc
REGDEF_BEGIN(RB_RATIO_REGISTER_3)
REGDEF_BIT(ife_rbratio12,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio13,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio14,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio15,        5)
REGDEF_END(RB_RATIO_REGISTER_3)


/*
    ife_rbratio16:    [0x0, 0x1f],          bits : 4_0
    ife_rbratio17:    [0x0, 0x1f],          bits : 12_8
    ife_rbratio18:    [0x0, 0x1f],          bits : 20_16
    ife_rbratio19:    [0x0, 0x1f],          bits : 28_24
*/
#define RB_RATIO_REGISTER_4_OFS 0x03d0
REGDEF_BEGIN(RB_RATIO_REGISTER_4)
REGDEF_BIT(ife_rbratio16,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio17,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio18,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio19,        5)
REGDEF_END(RB_RATIO_REGISTER_4)


/*
    ife_rbratio20:    [0x0, 0x1f],          bits : 4_0
    ife_rbratio21:    [0x0, 0x1f],          bits : 12_8
    ife_rbratio22:    [0x0, 0x1f],          bits : 20_16
    ife_rbratio23:    [0x0, 0x1f],          bits : 28_24
*/
#define RB_RATIO_REGISTER_5_OFS 0x03d4
REGDEF_BEGIN(RB_RATIO_REGISTER_5)
REGDEF_BIT(ife_rbratio20,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio21,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio22,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio23,        5)
REGDEF_END(RB_RATIO_REGISTER_5)


/*
    ife_rbratio24:    [0x0, 0x1f],          bits : 4_0
    ife_rbratio25:    [0x0, 0x1f],          bits : 12_8
    ife_rbratio26:    [0x0, 0x1f],          bits : 20_16
    ife_rbratio27:    [0x0, 0x1f],          bits : 28_24
*/
#define RB_RATIO_REGISTER_6_OFS 0x03d8
REGDEF_BEGIN(RB_RATIO_REGISTER_6)
REGDEF_BIT(ife_rbratio24,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio25,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio26,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio27,        5)
REGDEF_END(RB_RATIO_REGISTER_6)


/*
    ife_rbratio28     :    [0x0, 0x1f],         bits : 4_0
    ife_rbratio29     :    [0x0, 0x1f],         bits : 12_8
    ife_rbratio30     :    [0x0, 0x1f],         bits : 20_16
    ife_rbratio31     :    [0x0, 0x1f],         bits : 28_24
    rbratio_mode (0~2):    [0x0, 0x3],          bits : 30_29
*/
#define RB_RATIO_REGISTER_7_OFS 0x03dc
REGDEF_BEGIN(RB_RATIO_REGISTER_7)
REGDEF_BIT(ife_rbratio28,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio29,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio30,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_rbratio31,        5)
REGDEF_BIT(rbratio_mode,        2)
REGDEF_END(RB_RATIO_REGISTER_7)

/*
    IFE_F_NRS_O_EN            :    [0x0, 0x1],          bits : 0
    IFE_F_NRS_B_EN            :    [0x0, 0x1],          bits : 1
    IFE_F_NRS_G_EN            :    [0x0, 0x1],          bits : 2
    IFE_F_NRS_B_STRENGTH (0~8):    [0x0, 0xf],          bits : 19_16
    IFE_F_NRS_G_STRENGTH (0~8):    [0x0, 0xf],          bits : 23_20
    IFE_F_NRS_G_WEIGHT        :    [0x0, 0xf],          bits : 27_24
*/
#define NRS_REGISTER_OFS 0x0500
REGDEF_BEGIN(NRS_REGISTER)
REGDEF_BIT(ife_f_nrs_ord_en,        1)
REGDEF_BIT(ife_f_nrs_bilat_en,        1)
REGDEF_BIT(ife_f_nrs_gbilat_en,        1)
REGDEF_BIT(,       13)
REGDEF_BIT(ife_f_nrs_bilat_strength,        4)
REGDEF_BIT(ife_f_nrs_gbilat_strength,        4)
REGDEF_BIT(ife_f_nrs_gbilat_weight,        4)
REGDEF_END(NRS_REGISTER)


/*
    IFE_F_NRS_ORD_RANGE_BRI :    [0x0, 0x7],            bits : 2_0
    ife_f_nrs_ord_range_dark:    [0x0, 0x7],            bits : 6_4
    ife_f_nrs_ord_diff_thr  :    [0x0, 0x3ff],          bits : 17_8
*/
#define RHE_NRS_S_REGISTER_OFS 0x0504
REGDEF_BEGIN(RHE_NRS_S_REGISTER)
REGDEF_BIT(ife_f_nrs_ord_range_bright,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_f_nrs_ord_range_dark,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_f_nrs_ord_diff_thr,        10)
REGDEF_END(RHE_NRS_S_REGISTER)


/*
    ife_f_nrs_ord_dark_weight_lut0 (0~8):    [0x0, 0xf],            bits : 3_0
    ife_f_nrs_ord_dark_weight_lut1 (0~8):    [0x0, 0xf],            bits : 7_4
    ife_f_nrs_ord_dark_weight_lut2 (0~8):    [0x0, 0xf],            bits : 11_8
    ife_f_nrs_ord_dark_weight_lut3 (0~8):    [0x0, 0xf],            bits : 15_12
    ife_f_nrs_ord_dark_weight_lut4 (0~8):    [0x0, 0xf],            bits : 19_16
    ife_f_nrs_ord_dark_weight_lut5 (0~8):    [0x0, 0xf],            bits : 23_20
    ife_f_nrs_ord_dark_weight_lut6 (0~8):    [0x0, 0xf],            bits : 27_24
    ife_f_nrs_ord_dark_weight_lut7 (0~8):    [0x0, 0xf],            bits : 31_28
*/
#define RHE_NRS_ORDER_LUT_DARK_WEIGHT_OFS 0x0508
REGDEF_BEGIN(RHE_NRS_ORDER_LUT_DARK)
REGDEF_BIT(ife_f_nrs_ord_dark_weight_lut0,        4)
REGDEF_BIT(ife_f_nrs_ord_dark_weight_lut1,        4)
REGDEF_BIT(ife_f_nrs_ord_dark_weight_lut2,        4)
REGDEF_BIT(ife_f_nrs_ord_dark_weight_lut3,        4)
REGDEF_BIT(ife_f_nrs_ord_dark_weight_lut4,        4)
REGDEF_BIT(ife_f_nrs_ord_dark_weight_lut5,        4)
REGDEF_BIT(ife_f_nrs_ord_dark_weight_lut6,        4)
REGDEF_BIT(ife_f_nrs_ord_dark_weight_lut7,        4)
REGDEF_END(RHE_NRS_ORDER_LUT_DARK_WEIGHT)


/*
    ife_f_nrs_ord_bright_weight_lut0 (0~8):    [0x0, 0xf],          bits : 3_0
    ife_f_nrs_ord_bright_weight_lut1 (0~8):    [0x0, 0xf],          bits : 7_4
    ife_f_nrs_ord_bright_weight_lut2 (0~8):    [0x0, 0xf],          bits : 11_8
    ife_f_nrs_ord_bright_weight_lut3 (0~8):    [0x0, 0xf],          bits : 15_12
    ife_f_nrs_ord_bright_weight_lut4 (0~8):    [0x0, 0xf],          bits : 19_16
    ife_f_nrs_ord_bright_weight_lut5 (0~8):    [0x0, 0xf],          bits : 23_20
    ife_f_nrs_ord_bright_weight_lut6 (0~8):    [0x0, 0xf],          bits : 27_24
    ife_f_nrs_ord_bright_weight_lut7 (0~8):    [0x0, 0xf],          bits : 31_28
*/
#define RHE_NRS_ORDER_LUT_BRIGHT_WEIGHT_OFS 0x050c
REGDEF_BEGIN(RHE_NRS_ORDER_LUT_BRI)
REGDEF_BIT(ife_f_nrs_ord_bright_weight_lut0,        4)
REGDEF_BIT(ife_f_nrs_ord_bright_weight_lut1,        4)
REGDEF_BIT(ife_f_nrs_ord_bright_weight_lut2,        4)
REGDEF_BIT(ife_f_nrs_ord_bright_weight_lut3,        4)
REGDEF_BIT(ife_f_nrs_ord_bright_weight_lut4,        4)
REGDEF_BIT(ife_f_nrs_ord_bright_weight_lut5,        4)
REGDEF_BIT(ife_f_nrs_ord_bright_weight_lut6,        4)
REGDEF_BIT(ife_f_nrs_ord_bright_weight_lut7,        4)
REGDEF_END(RHE_NRS_ORDER_LUT_BRIGHT_WEIGHT)


/*
    IFE_F_NRS_B_LUT_OFS0:    [0x0, 0xff],           bits : 7_0
    IFE_F_NRS_B_LUT_OFS1:    [0x0, 0xff],           bits : 15_8
    IFE_F_NRS_B_LUT_OFS2:    [0x0, 0xff],           bits : 23_16
    IFE_F_NRS_B_LUT_OFS3:    [0x0, 0xff],           bits : 31_24
*/
#define RHE_NRS_BILATERAL_LUT_OFFSET0_OFS 0x0510
REGDEF_BEGIN(RHE_NRS_BILATERAL_LUT_OFFSET0)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset0,        8)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset1,        8)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset2,        8)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset3,        8)
REGDEF_END(RHE_NRS_BILATERAL_LUT_OFFSET0)


/*
    IFE_F_NRS_B_LUT_OFS4:    [0x0, 0xff],           bits : 7_0
    IFE_F_NRS_B_LUT_OFS5:    [0x0, 0xff],           bits : 15_8
*/
#define RHE_NRS_BILATERAL_LUT_OFFSET1_OFS 0x0514
REGDEF_BEGIN(RHE_NRS_BILATERAL_LUT_OFFSET1)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset4,        8)
REGDEF_BIT(ife_f_nrs_bilat_lut_offset5,        8)
REGDEF_END(RHE_NRS_BILATERAL_LUT_OFFSET1)


/*
    IFE_F_NRS_B_LUT_W0:    [0x0, 0x1f],         bits : 4_0
    IFE_F_NRS_B_LUT_W1:    [0x0, 0x1f],         bits : 9_5
    IFE_F_NRS_B_LUT_W2:    [0x0, 0x1f],         bits : 14_10
    IFE_F_NRS_B_LUT_W3:    [0x0, 0x1f],         bits : 19_15
    IFE_F_NRS_B_LUT_W4:    [0x0, 0x1f],         bits : 24_20
    IFE_F_NRS_B_LUT_W5:    [0x0, 0x1f],         bits : 29_25
*/
#define RHE_NRS_BILATERAL_LUT_WEIGHT_OFS 0x0518
REGDEF_BEGIN(RHE_NRS_BILATERAL_LUT_WEIGHT)
REGDEF_BIT(ife_f_nrs_bilat_lut_weight0,        5)
REGDEF_BIT(ife_f_nrs_bilat_lut_weight1,        5)
REGDEF_BIT(ife_f_nrs_bilat_lut_weight2,        5)
REGDEF_BIT(ife_f_nrs_bilat_lut_weight3,        5)
REGDEF_BIT(ife_f_nrs_bilat_lut_weight4,        5)
REGDEF_BIT(ife_f_nrs_bilat_lut_weight5,        5)
REGDEF_END(RHE_NRS_BILATERAL_LUT_WEIGHT)


/*
    IFE_F_NRS_B_LUT_RANGE1:    [0x0, 0xfff],            bits : 11_0
    IFE_F_NRS_B_LUT_RANGE2:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_NRS_BILATERAL_LUT_RANGE0_OFS 0x051c
REGDEF_BEGIN(RHE_NRS_BILATERAL_LUT_RANGE0)
REGDEF_BIT(ife_f_nrs_bilat_lut_range1,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_nrs_bilat_lut_range2,        12)
REGDEF_END(RHE_NRS_BILATERAL_LUT_RANGE0)


/*
    IFE_F_NRS_B_LUT_RANGE3:    [0x0, 0xfff],            bits : 11_0
    IFE_F_NRS_B_LUT_RANGE4:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_NRS_BILATERAL_LUT_RANGE1_OFS 0x0520
REGDEF_BEGIN(RHE_NRS_BILATERAL_LUT_RANGE1)
REGDEF_BIT(ife_f_nrs_bilat_lut_range3,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_nrs_bilat_lut_range4,        12)
REGDEF_END(RHE_NRS_BILATERAL_LUT_RANGE1)


/*
    IFE_F_NRS_B_LUT_RANGE5 :    [0x0, 0xfff],           bits : 11_0
    IFE_F_NRS_B_LUT_TH1:    [0x0, 0x3],         bits : 17_16
    IFE_F_NRS_B_LUT_TH2:    [0x0, 0x3],         bits : 19_18
    IFE_F_NRS_B_LUT_TH3:    [0x0, 0x3],         bits : 21_20
    IFE_F_NRS_B_LUT_TH4:    [0x0, 0x3],         bits : 23_22
    IFE_F_NRS_B_LUT_TH5:    [0x0, 0x3],         bits : 25_24
*/
#define RHE_NRS_BILATERAL_LUT_RANGE2_OFS 0x0524
REGDEF_BEGIN(RHE_NRS_BILATERAL_LUT_RANGE2)
REGDEF_BIT(ife_f_nrs_bilat_lut_range5,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_nrs_bilat_lut_th1,        2)
REGDEF_BIT(ife_f_nrs_bilat_lut_th2,        2)
REGDEF_BIT(ife_f_nrs_bilat_lut_th3,        2)
REGDEF_BIT(ife_f_nrs_bilat_lut_th4,        2)
REGDEF_BIT(ife_f_nrs_bilat_lut_th5,        2)
REGDEF_END(RHE_NRS_BILATERAL_LUT_RANGE2)


/*
    IFE_F_NRS_G_LUT_OFS0:    [0x0, 0xff],           bits : 7_0
    IFE_F_NRS_G_LUT_OFS1:    [0x0, 0xff],           bits : 15_8
    IFE_F_NRS_G_LUT_OFS2:    [0x0, 0xff],           bits : 23_16
    IFE_F_NRS_G_LUT_OFS3:    [0x0, 0xff],           bits : 31_24
*/
#define RHE_NRS_GBILATERAL_LUT_OFFSET0_OFS 0x0528
REGDEF_BEGIN(RHE_NRS_GBILATERAL_LUT_OFFSET0)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset0,        8)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset1,        8)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset2,        8)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset3,        8)
REGDEF_END(RHE_NRS_GBILATERAL_LUT_OFFSET0)


/*
    IFE_F_NRS_G_LUT_OFS4:    [0x0, 0xff],           bits : 7_0
    IFE_F_NRS_G_LUT_OFS5:    [0x0, 0xff],           bits : 15_8
*/
#define RHE_NRS_GBILATERAL_LUT_OFFSET1_OFS 0x052c
REGDEF_BEGIN(RHE_NRS_GBILATERAL_LUT_OFFSET1)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset4,        8)
REGDEF_BIT(ife_f_nrs_gbilat_lut_offset5,        8)
REGDEF_END(RHE_NRS_GBILATERAL_LUT_OFFSET1)


/*
    IFE_F_NRS_G_LUT_W0:    [0x0, 0x1f],         bits : 4_0
    IFE_F_NRS_G_LUT_W1:    [0x0, 0x1f],         bits : 9_5
    IFE_F_NRS_G_LUT_W2:    [0x0, 0x1f],         bits : 14_10
    IFE_F_NRS_G_LUT_W3:    [0x0, 0x1f],         bits : 19_15
    IFE_F_NRS_G_LUT_W4:    [0x0, 0x1f],         bits : 24_20
    IFE_F_NRS_G_LUT_W5:    [0x0, 0x1f],         bits : 29_25
*/
#define RHE_NRS_GBILATERAL_LUT_WEIGHT_OFS 0x0530
REGDEF_BEGIN(RHE_NRS_GBILATERAL_LUT_WEIGHT)
REGDEF_BIT(ife_f_nrs_gbilat_lut_weight0,        5)
REGDEF_BIT(ife_f_nrs_gbilat_lut_weight1,        5)
REGDEF_BIT(ife_f_nrs_gbilat_lut_weight2,        5)
REGDEF_BIT(ife_f_nrs_gbilat_lut_weight3,        5)
REGDEF_BIT(ife_f_nrs_gbilat_lut_weight4,        5)
REGDEF_BIT(ife_f_nrs_gbilat_lut_weight5,        5)
REGDEF_END(RHE_NRS_GBILATERAL_LUT_WEIGHT)


/*
    IFE_F_NRS_G_LUT_RANGE1:    [0x0, 0xfff],            bits : 11_0
    IFE_F_NRS_G_LUT_RANGE2:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_NRS_GBILATERAL_LUT_RANGE0_OFS 0x0534
REGDEF_BEGIN(RHE_NRS_GBILATERAL_LUT_RANGE0)
REGDEF_BIT(ife_f_nrs_gbilat_lut_range1,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_nrs_gbilat_lut_range2,        12)
REGDEF_END(RHE_NRS_GBILATERAL_LUT_RANGE0)


/*
    IFE_F_NRS_G_LUT_RANGE3:    [0x0, 0xfff],            bits : 11_0
    IFE_F_NRS_G_LUT_RANGE4:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_NRS_GBILATERAL_LUT_RANGE1_OFS 0x0538
REGDEF_BEGIN(RHE_NRS_GBILATERAL_LUT_RANGE1)
REGDEF_BIT(ife_f_nrs_gbilat_lut_range3,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_nrs_gbilat_lut_range4,        12)
REGDEF_END(RHE_NRS_GBILATERAL_LUT_RANGE1)


/*
    IFE_F_NRS_G_LUT_RANGE5 :    [0x0, 0xfff],           bits : 11_0
    IFE_F_NRS_G_LUT_TH1:    [0x0, 0x3],         bits : 17_16
    IFE_F_NRS_G_LUT_TH2:    [0x0, 0x3],         bits : 19_18
    IFE_F_NRS_G_LUT_TH3:    [0x0, 0x3],         bits : 21_20
    IFE_F_NRS_G_LUT_TH4:    [0x0, 0x3],         bits : 23_22
    IFE_F_NRS_G_LUT_TH5:    [0x0, 0x3],         bits : 25_24
*/
#define RHE_NRS_GBILATERAL_LUT_RANGE2_OFS 0x053c
REGDEF_BEGIN(RHE_NRS_GBILATERAL_LUT_RANGE2)
REGDEF_BIT(ife_f_nrs_gbilat_lut_range5,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_nrs_gbilat_lut_th1,        2)
REGDEF_BIT(ife_f_nrs_gbilat_lut_th2,        2)
REGDEF_BIT(ife_f_nrs_gbilat_lut_th3,        2)
REGDEF_BIT(ife_f_nrs_gbilat_lut_th4,        2)
REGDEF_BIT(ife_f_nrs_gbilat_lut_th5,        2)
REGDEF_END(RHE_NRS_GBILATERAL_LUT_RANGE2)

/*
    IFE_F_FUSION_YM_SEL       :    [0x0, 0x3],          bits : 1_0
    IFE_F_FUSION_BCN_SEL (0~2):    [0x0, 0x3],          bits : 5_4
    IFE_F_FUSION_BCD_SEL (0~2):    [0x0, 0x3],          bits : 9_8
    ife_f_fusion_mode         :    [0x0, 0x3],          bits : 13_12
    ife_f_fusion_evratio      :    [0x0, 0xff],         bits : 23_16
*/
#define RHE_FUSION_REGISTER_OFS 0x0550
REGDEF_BEGIN(RHE_FUSION_REGISTER)
REGDEF_BIT(ife_f_fusion_ymean_sel,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fusion_normal_blend_curve_sel,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fusion_diff_blend_curve_sel,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fusion_mode,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fusion_evratio,        8)
REGDEF_END(RHE_FUSION_REGISTER)


/*
    IFE_F_FUSION_BCNL_P0          :    [0x0, 0xfff],            bits : 11_0
    IFE_F_FUSION_BCNL_RANGE (0~12):    [0x0, 0xf],          bits : 15_12
    IFE_F_FUSION_BCNL_P1          :    [0x0, 0xfff],            bits : 27_16
    IFE_F_FUSION_BCNL_WEDGE       :    [0x0, 0x1],          bits : 31
*/
#define RHE_FUSION_LONG_EXP_NORMAL_BLEND_CURVE_OFS 0x0554
REGDEF_BEGIN(RHE_FUSION_LONG_EXPOSURE_NORMAL_BLENDING_CURVE)
REGDEF_BIT(ife_f_fusion_long_exp_normal_blend_curve_knee_point0,        12)
REGDEF_BIT(ife_f_fusion_long_exp_normal_blend_curve_range,        4)
REGDEF_BIT(ife_f_fusion_long_exp_normal_blend_curve_knee_point1,        12)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_f_fusion_long_exp_normal_blend_curve_wedge,        1)
REGDEF_END(RHE_FUSION_LONG_EXP_NORMAL_BLEND_CURVE)


/*
    IFE_F_FUSION_BCNS_P0          :    [0x0, 0xfff],            bits : 11_0
    IFE_F_FUSION_BCNS_RANGE (0~12):    [0x0, 0xf],          bits : 15_12
    IFE_F_FUSION_BCNS_P1          :    [0x0, 0xfff],            bits : 27_16
    IFE_F_FUSION_BCNS_WEDGE       :    [0x0, 0x1],          bits : 31
*/
#define RHE_FUSION_SHORT_EXP_NORMAL_BLEND_CURVE_OFS 0x0558
REGDEF_BEGIN(RHE_FUSION_SHORT_EXP_NORMAL_BLEND_CURVE)
REGDEF_BIT(ife_f_fusion_short_exp_normal_blend_curve_knee_point0,        12)
REGDEF_BIT(ife_f_fusion_short_exp_normal_blend_curve_range,        4)
REGDEF_BIT(ife_f_fusion_short_exp_normal_blend_curve_knee_point1,        12)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_f_fusion_short_exp_normal_blend_curve_wedge,        1)
REGDEF_END(RHE_FUSION_SHORT_EXP_NORMAL_BLEND_CURVE)


/*
    IFE_F_FUSION_BCDL_P0          :    [0x0, 0xfff],            bits : 11_0
    IFE_F_FUSION_BCDL_RANGE (0~12):    [0x0, 0xf],          bits : 15_12
    IFE_F_FUSION_BCDL_P1          :    [0x0, 0xfff],            bits : 27_16
    IFE_F_FUSION_BCDL_WEDGE       :    [0x0, 0x1],          bits : 31
*/
#define RHE_FUSION_LONG_EXP_DIFF_BLEND_CURVE_OFS 0x055c
REGDEF_BEGIN(RHE_FUSION_LONG_EXP_DIFF_BLEND_CURVE)
REGDEF_BIT(ife_f_fusion_long_exp_diff_blend_curve_knee_point0,        12)
REGDEF_BIT(ife_f_fusion_long_exp_diff_blend_curve_range,        4)
REGDEF_BIT(ife_f_fusion_long_exp_diff_blend_curve_knee_point1,        12)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_f_fusion_long_exp_diff_blend_curve_wedge,        1)
REGDEF_END(RHE_FUSION_LONG_EXP_DIFF_BLEND_CURVE)


/*
    IFE_F_FUSION_BCDS_P0          :    [0x0, 0xfff],            bits : 11_0
    IFE_F_FUSION_BCDS_RANGE (0~12):    [0x0, 0xf],          bits : 15_12
    IFE_F_FUSION_BCDS_P1          :    [0x0, 0xfff],            bits : 27_16
    IFE_F_FUSION_BCDS_WEDGE       :    [0x0, 0x1],          bits : 31
*/
#define RHE_FUSION_SHORT_EXP_DIFF_BLEND_CURVE_OFS 0x0560
REGDEF_BEGIN(RHE_FUSION_SHORT_EXP_DIFF_BLEND_CURVE)
REGDEF_BIT(ife_f_fusion_short_exp_diff_blend_curve_knee_point0,        12)
REGDEF_BIT(ife_f_fusion_short_exp_diff_blend_curve_range,        4)
REGDEF_BIT(ife_f_fusion_short_exp_diff_blend_curve_knee_point1,        12)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_f_fusion_short_exp_diff_blend_curve_wedge,        1)
REGDEF_END(RHE_FUSION_SHORT_EXP_DIFF_BLEND_CURVE)

/*
    ife_f_fusion_mc_lumthr    :    [0x0, 0xfff],            bits : 11_0
    ife_f_fusion_mc_diff_ratio:    [0x0, 0x3],          bits : 13_12
*/
#define RHE_FUSION_MOTION_COMPENSATION_OFS 0x0570
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION)
REGDEF_BIT(ife_f_fusion_mc_lumthr,        12)
REGDEF_BIT(ife_f_fusion_mc_diff_ratio,        2)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION)


/*
    ife_f_fusion_mc_lut_positive_diff_weight0 (0~16):    [0x0, 0x1f],           bits : 4_0
    ife_f_fusion_mc_lut_positive_diff_weight1 (0~16):    [0x0, 0x1f],           bits : 9_5
    ife_f_fusion_mc_lut_positive_diff_weight2 (0~16):    [0x0, 0x1f],           bits : 14_10
    ife_f_fusion_mc_lut_positive_diff_weight3 (0~16):    [0x0, 0x1f],           bits : 19_15
    ife_f_fusion_mc_lut_positive_diff_weight4 (0~16):    [0x0, 0x1f],           bits : 24_20
    ife_f_fusion_mc_lut_positive_diff_weight5 (0~16):    [0x0, 0x1f],           bits : 29_25
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_0_OFS 0x0574
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_0)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight0,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight1,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight2,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight3,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight4,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight5,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_0)


/*
    ife_f_fusion_mc_lut_positive_diff_weight6 (0~16) :    [0x0, 0x1f],          bits : 4_0
    ife_f_fusion_mc_lut_positive_diff_weight7 (0~16) :    [0x0, 0x1f],          bits : 9_5
    ife_f_fusion_mc_lut_positive_diff_weight8 (0~16) :    [0x0, 0x1f],          bits : 14_10
    ife_f_fusion_mc_lut_positive_diff_weight9 (0~16) :    [0x0, 0x1f],          bits : 19_15
    ife_f_fusion_mc_lut_positive_diff_weight10 (0~16):    [0x0, 0x1f],          bits : 24_20
    ife_f_fusion_mc_lut_positive_diff_weight11 (0~16):    [0x0, 0x1f],          bits : 29_25
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_1_OFS 0x0578
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_1)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight6,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight7,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight8,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight9,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight10,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight11,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_1)


/*
    ife_f_fusion_mc_lut_positive_diff_weight12 (0~16):    [0x0, 0x1f],          bits : 4_0
    ife_f_fusion_mc_lut_positive_diff_weight13 (0~16):    [0x0, 0x1f],          bits : 9_5
    ife_f_fusion_mc_lut_positive_diff_weight14 (0~16):    [0x0, 0x1f],          bits : 14_10
    ife_f_fusion_mc_lut_positive_diff_weight15 (0~16):    [0x0, 0x1f],          bits : 19_15
    IFE_F_FUSION_MC_LUT_DWD (0~16)  :    [0x0, 0x1f],           bits : 24_20
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_2_OFS 0x057c
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_2)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight12,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight13,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight14,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_positive_diff_weight15,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_difflumth_diff_weight,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_2)


/*
    ife_f_fusion_mc_lut_negative_diff_weight0 (0~16):    [0x0, 0x1f],           bits : 4_0
    ife_f_fusion_mc_lut_negative_diff_weight1 (0~16):    [0x0, 0x1f],           bits : 9_5
    ife_f_fusion_mc_lut_negative_diff_weight2 (0~16):    [0x0, 0x1f],           bits : 14_10
    ife_f_fusion_mc_lut_negative_diff_weight3 (0~16):    [0x0, 0x1f],           bits : 19_15
    ife_f_fusion_mc_lut_negative_diff_weight4 (0~16):    [0x0, 0x1f],           bits : 24_20
    ife_f_fusion_mc_lut_negative_diff_weight5 (0~16):    [0x0, 0x1f],           bits : 29_25
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_3_OFS 0x0580
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_3)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight0,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight1,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight2,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight3,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight4,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight5,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_3)


/*
    ife_f_fusion_mc_lut_negative_diff_weight6 (0~16) :    [0x0, 0x1f],          bits : 4_0
    ife_f_fusion_mc_lut_negative_diff_weight7 (0~16) :    [0x0, 0x1f],          bits : 9_5
    ife_f_fusion_mc_lut_negative_diff_weight8 (0~16) :    [0x0, 0x1f],          bits : 14_10
    ife_f_fusion_mc_lut_negative_diff_weight9 (0~16) :    [0x0, 0x1f],          bits : 19_15
    ife_f_fusion_mc_lut_negative_diff_weight10 (0~16):    [0x0, 0x1f],          bits : 24_20
    ife_f_fusion_mc_lut_negative_diff_weight11 (0~16):    [0x0, 0x1f],          bits : 29_25
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_4_OFS 0x0584
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_4)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight6,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight7,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight8,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight9,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight10,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight11,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_4)


/*
    ife_f_fusion_mc_lut_negative_diff_weight12 (0~16):    [0x0, 0x1f],          bits : 4_0
    ife_f_fusion_mc_lut_negative_diff_weight13 (0~16):    [0x0, 0x1f],          bits : 9_5
    ife_f_fusion_mc_lut_negative_diff_weight14 (0~16):    [0x0, 0x1f],          bits : 14_10
    ife_f_fusion_mc_lut_negative_diff_weight15 (0~16):    [0x0, 0x1f],          bits : 19_15
*/
#define RHE_FUSION_MOTION_COMPENSATION_LUT_5_OFS 0x0588
REGDEF_BEGIN(RHE_FUSION_MOTION_COMPENSATION_LUT_5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight12,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight13,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight14,        5)
REGDEF_BIT(ife_f_fusion_mc_lut_negative_diff_weight15,        5)
REGDEF_END(RHE_FUSION_MOTION_COMPENSATION_LUT_5)

/*
    IFE_F_DS0_TH  :    [0x0, 0xfff],            bits : 11_0
    IFE_F_DS0_STEP:    [0x0, 0xff],         bits : 23_16
    IFE_F_DS0_LB  :    [0x0, 0xff],         bits : 31_24
*/
#define RHE_FUSION_PATH0_DARK_SATURATION_REDUCTION_OFS 0x0590
REGDEF_BEGIN(RHE_FUSION_PATH0_DSR)
REGDEF_BIT(ife_f_dark_sat_reduction0_th,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_dark_sat_reduction0_step,        8)
REGDEF_BIT(ife_f_dark_sat_reduction0_low_bound,        8)
REGDEF_END(RHE_FUSION_PATH0_DARK_SATURATION_REDUCTION)


/*
    IFE_F_DS1_TH  :    [0x0, 0xfff],            bits : 11_0
    IFE_F_DS1_STEP:    [0x0, 0xff],         bits : 23_16
    IFE_F_DS1_LB  :    [0x0, 0xff],         bits : 31_24
*/
#define RHE_FUSION_PATH1_DARK_SATURATION_REDUCTION_OFS 0x0594
REGDEF_BEGIN(RHE_FUSION_PATH1_DSR)
REGDEF_BIT(ife_f_dark_sat_reduction1_th,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_dark_sat_reduction1_step,        8)
REGDEF_BIT(ife_f_dark_sat_reduction1_low_bound,        8)
REGDEF_END(RHE_FUSION_PATH1_DARK_SATURATION_REDUCTION)

/*
    ife_f_fusion_short_exposure_compress_knee_point0:    [0x0, 0xfff],          bits : 11_0
    ife_f_fusion_short_exposure_compress_knee_point1:    [0x0, 0xfff],          bits : 27_16
*/
#define RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_0_OFS 0x05a0
REGDEF_BEGIN(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_0)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_knee_point0,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_knee_point1,        12)
REGDEF_END(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_0)


/*
    ife_f_fusion_short_exposure_compress_knee_point2:    [0x0, 0xfff],          bits : 11_0
    IFE_f_fusion_short_exposure_compress_en :    [0x0, 0x1],            bits : 16
*/
#define RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_1_OFS 0x05a4
REGDEF_BEGIN(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_1)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_knee_point2,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_enable,        1)
REGDEF_END(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_1)


/*
    ife_f_fusion_short_exposure_compress_subtract_point0:    [0x0, 0xfff],          bits : 11_0
    ife_f_fusion_short_exposure_compress_subtract_point1:    [0x0, 0xfff],          bits : 27_16
*/
#define RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_2_OFS 0x05a8
REGDEF_BEGIN(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_2)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_subtract_point0,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_subtract_point1,        12)
REGDEF_END(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_2)


/*
    ife_f_fusion_short_exposure_compress_subtract_point2:    [0x0, 0xfff],          bits : 11_0
    ife_f_fusion_short_exposure_compress_subtract_point3:    [0x0, 0xfff],          bits : 27_16
*/
#define RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_3_OFS 0x05ac
REGDEF_BEGIN(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_3)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_subtract_point2,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_subtract_point3,        12)
REGDEF_END(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_3)


/*
    ife_f_fusion_short_exposure_compress_shift_bit0:    [0x0, 0x7],         bits : 2_0
    ife_f_fusion_short_exposure_compress_shift_bit1:    [0x0, 0x7],         bits : 6_4
    ife_f_fusion_short_exposure_compress_shift_bit2:    [0x0, 0x7],         bits : 10_8
    ife_f_fusion_short_exposure_compress_shift_bit3:    [0x0, 0x7],         bits : 14_12
*/
#define RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_4_OFS 0x05b0
REGDEF_BEGIN(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_4)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_shift_bit0,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_shift_bit1,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_shift_bit2,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ife_f_fusion_short_exposure_compress_shift_bit3,        3)
REGDEF_END(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_4)

/*
    IFE_F_FCURVE_YM_SEL (0~2)  :    [0x0, 0x3],         bits : 1_0
    ife_f_fcurve_yvweight (0~8):    [0x0, 0xf],         bits : 5_2
*/
#define RHE_FCURVE_CTRL_OFS 0x0600
REGDEF_BEGIN(RHE_FCURVE_CTRL)
REGDEF_BIT(ife_f_fcurve_ymean_select,        2)
REGDEF_BIT(ife_f_fcurve_yvweight,        4)
REGDEF_END(RHE_FCURVE_CTRL)


/*
    ife_f_fcurve_yweight_lut0:    [0x0, 0xff],          bits : 7_0
    ife_f_fcurve_yweight_lut1:    [0x0, 0xff],          bits : 15_8
    ife_f_fcurve_yweight_lut2:    [0x0, 0xff],          bits : 23_16
    ife_f_fcurve_yweight_lut3:    [0x0, 0xff],          bits : 31_24
*/
#define RHE_FCURVE_Y_WEIGHT_REGISTER0_OFS 0x0604
REGDEF_BEGIN(RHE_FCURVE_Y_WEIGHT_REGISTER0)
REGDEF_BIT(ife_f_fcurve_yweight_lut0,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut1,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut2,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut3,        8)
REGDEF_END(RHE_FCURVE_Y_WEIGHT_REGISTER0)


/*
    ife_f_fcurve_yweight_lut4:    [0x0, 0xff],          bits : 7_0
    ife_f_fcurve_yweight_lut5:    [0x0, 0xff],          bits : 15_8
    ife_f_fcurve_yweight_lut6:    [0x0, 0xff],          bits : 23_16
    ife_f_fcurve_yweight_lut7:    [0x0, 0xff],          bits : 31_24
*/
#define RHE_FCURVE_Y_WEIGHT_REGISTER1_OFS 0x0608
REGDEF_BEGIN(RHE_FCURVE_Y_WEIGHT_REGISTER1)
REGDEF_BIT(ife_f_fcurve_yweight_lut4,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut5,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut6,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut7,        8)
REGDEF_END(RHE_FCURVE_Y_WEIGHT_REGISTER1)


/*
    ife_f_fcurve_yweight_lut8 :    [0x0, 0xff],         bits : 7_0
    ife_f_fcurve_yweight_lut9 :    [0x0, 0xff],         bits : 15_8
    ife_f_fcurve_yweight_lut10:    [0x0, 0xff],         bits : 23_16
    ife_f_fcurve_yweight_lut11:    [0x0, 0xff],         bits : 31_24
*/
#define RHE_FCURVE_Y_WEIGHT_REGISTER2_OFS 0x060c
REGDEF_BEGIN(RHE_FCURVE_Y_WEIGHT_REGISTER2)
REGDEF_BIT(ife_f_fcurve_yweight_lut8,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut9,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut10,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut11,        8)
REGDEF_END(RHE_FCURVE_Y_WEIGHT_REGISTER2)


/*
    ife_f_fcurve_yweight_lut12:    [0x0, 0xff],         bits : 7_0
    ife_f_fcurve_yweight_lut13:    [0x0, 0xff],         bits : 15_8
    ife_f_fcurve_yweight_lut14:    [0x0, 0xff],         bits : 23_16
    ife_f_fcurve_yweight_lut15:    [0x0, 0xff],         bits : 31_24
*/
#define RHE_FCURVE_Y_WEIGHT_REGISTER3_OFS 0x0610
REGDEF_BEGIN(RHE_FCURVE_Y_WEIGHT_REGISTER3)
REGDEF_BIT(ife_f_fcurve_yweight_lut12,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut13,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut14,        8)
REGDEF_BIT(ife_f_fcurve_yweight_lut15,        8)
REGDEF_END(RHE_FCURVE_Y_WEIGHT_REGISTER3)


/*
    ife_f_fcurve_yweight_lut16:    [0x0, 0xff],         bits : 7_0
*/
#define RHE_FCURVE_Y_WEIGHT_REGISTER4_OFS 0x0614
REGDEF_BEGIN(RHE_FCURVE_Y_WEIGHT_REGISTER4)
REGDEF_BIT(ife_f_fcurve_yweight_lut16,        8)
REGDEF_END(RHE_FCURVE_Y_WEIGHT_REGISTER4)


/*
    ife_f_fcurve_index_lut0:    [0x0, 0x3f],            bits : 5_0
    ife_f_fcurve_index_lut1:    [0x0, 0x3f],            bits : 13_8
    ife_f_fcurve_index_lut2:    [0x0, 0x3f],            bits : 21_16
    ife_f_fcurve_index_lut3:    [0x0, 0x3f],            bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER0_OFS 0x0618
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER0)
REGDEF_BIT(ife_f_fcurve_index_lut0,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut2,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut3,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER0)


/*
    ife_f_fcurve_index_lut4:    [0x0, 0x3f],            bits : 5_0
    ife_f_fcurve_index_lut5:    [0x0, 0x3f],            bits : 13_8
    ife_f_fcurve_index_lut6:    [0x0, 0x3f],            bits : 21_16
    ife_f_fcurve_index_lut7:    [0x0, 0x3f],            bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER1_OFS 0x061c
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER1)
REGDEF_BIT(ife_f_fcurve_index_lut4,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut5,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut6,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut7,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER1)


/*
    ife_f_fcurve_index_lut8 :    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut9 :    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut10:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut11:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER2_OFS 0x0620
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER2)
REGDEF_BIT(ife_f_fcurve_index_lut8,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut9,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut10,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut11,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER2)


/*
    ife_f_fcurve_index_lut12:    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut13:    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut14:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut15:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER3_OFS 0x0624
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER3)
REGDEF_BIT(ife_f_fcurve_index_lut12,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut13,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut14,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut15,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER3)


/*
    ife_f_fcurve_index_lut16:    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut17:    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut18:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut19:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER4_OFS 0x0628
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER4)
REGDEF_BIT(ife_f_fcurve_index_lut16,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut17,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut18,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut19,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER4)


/*
    ife_f_fcurve_index_lut20:    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut21:    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut22:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut23:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER5_OFS 0x062c
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER5)
REGDEF_BIT(ife_f_fcurve_index_lut20,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut21,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut22,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut23,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER5)


/*
    ife_f_fcurve_index_lut24:    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut25:    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut26:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut27:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER6_OFS 0x0630
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER6)
REGDEF_BIT(ife_f_fcurve_index_lut24,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut25,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut26,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut27,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER6)


/*
    ife_f_fcurve_index_lut28:    [0x0, 0x3f],           bits : 5_0
    ife_f_fcurve_index_lut29:    [0x0, 0x3f],           bits : 13_8
    ife_f_fcurve_index_lut30:    [0x0, 0x3f],           bits : 21_16
    ife_f_fcurve_index_lut31:    [0x0, 0x3f],           bits : 29_24
*/
#define RHE_FCURVE_INDEX_REGISTER7_OFS 0x0634
REGDEF_BEGIN(RHE_FCURVE_INDEX_REGISTER7)
REGDEF_BIT(ife_f_fcurve_index_lut28,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut29,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut30,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_f_fcurve_index_lut31,        6)
REGDEF_END(RHE_FCURVE_INDEX_REGISTER7)


/*
    ife_f_fcurve_split_lut0 :    [0x0, 0x3],            bits : 1_0
    ife_f_fcurve_split_lut1 :    [0x0, 0x3],            bits : 3_2
    ife_f_fcurve_split_lut2 :    [0x0, 0x3],            bits : 5_4
    ife_f_fcurve_split_lut3 :    [0x0, 0x3],            bits : 7_6
    ife_f_fcurve_split_lut4 :    [0x0, 0x3],            bits : 9_8
    ife_f_fcurve_split_lut5 :    [0x0, 0x3],            bits : 11_10
    ife_f_fcurve_split_lut6 :    [0x0, 0x3],            bits : 13_12
    ife_f_fcurve_split_lut7 :    [0x0, 0x3],            bits : 15_14
    ife_f_fcurve_split_lut8 :    [0x0, 0x3],            bits : 17_16
    ife_f_fcurve_split_lut9 :    [0x0, 0x3],            bits : 19_18
    ife_f_fcurve_split_lut10:    [0x0, 0x3],            bits : 21_20
    ife_f_fcurve_split_lut11:    [0x0, 0x3],            bits : 23_22
    ife_f_fcurve_split_lut12:    [0x0, 0x3],            bits : 25_24
    ife_f_fcurve_split_lut13:    [0x0, 0x3],            bits : 27_26
    ife_f_fcurve_split_lut14:    [0x0, 0x3],            bits : 29_28
    ife_f_fcurve_split_lut15:    [0x0, 0x3],            bits : 31_30
*/
#define RHE_FCURVE_SPLIT_REGISTER0_OFS 0x0638
REGDEF_BEGIN(RHE_FCURVE_SPLIT_REGISTER0)
REGDEF_BIT(ife_f_fcurve_split_lut0,        2)
REGDEF_BIT(ife_f_fcurve_split_lut1,        2)
REGDEF_BIT(ife_f_fcurve_split_lut2,        2)
REGDEF_BIT(ife_f_fcurve_split_lut3,        2)
REGDEF_BIT(ife_f_fcurve_split_lut4,        2)
REGDEF_BIT(ife_f_fcurve_split_lut5,        2)
REGDEF_BIT(ife_f_fcurve_split_lut6,        2)
REGDEF_BIT(ife_f_fcurve_split_lut7,        2)
REGDEF_BIT(ife_f_fcurve_split_lut8,        2)
REGDEF_BIT(ife_f_fcurve_split_lut9,        2)
REGDEF_BIT(ife_f_fcurve_split_lut10,        2)
REGDEF_BIT(ife_f_fcurve_split_lut11,        2)
REGDEF_BIT(ife_f_fcurve_split_lut12,        2)
REGDEF_BIT(ife_f_fcurve_split_lut13,        2)
REGDEF_BIT(ife_f_fcurve_split_lut14,        2)
REGDEF_BIT(ife_f_fcurve_split_lut15,        2)
REGDEF_END(RHE_FCURVE_SPLIT_REGISTER0)


/*
    ife_f_fcurve_split_lut16:    [0x0, 0x3],            bits : 1_0
    ife_f_fcurve_split_lut17:    [0x0, 0x3],            bits : 3_2
    ife_f_fcurve_split_lut18:    [0x0, 0x3],            bits : 5_4
    ife_f_fcurve_split_lut19:    [0x0, 0x3],            bits : 7_6
    ife_f_fcurve_split_lut20:    [0x0, 0x3],            bits : 9_8
    ife_f_fcurve_split_lut21:    [0x0, 0x3],            bits : 11_10
    ife_f_fcurve_split_lut22:    [0x0, 0x3],            bits : 13_12
    ife_f_fcurve_split_lut23:    [0x0, 0x3],            bits : 15_14
    ife_f_fcurve_split_lut24:    [0x0, 0x3],            bits : 17_16
    ife_f_fcurve_split_lut25:    [0x0, 0x3],            bits : 19_18
    ife_f_fcurve_split_lut26:    [0x0, 0x3],            bits : 21_20
    ife_f_fcurve_split_lut27:    [0x0, 0x3],            bits : 23_22
    ife_f_fcurve_split_lut28:    [0x0, 0x3],            bits : 25_24
    ife_f_fcurve_split_lut29:    [0x0, 0x3],            bits : 27_26
    ife_f_fcurve_split_lut30:    [0x0, 0x3],            bits : 29_28
    ife_f_fcurve_split_lut31:    [0x0, 0x3],            bits : 31_30
*/
#define RHE_FCURVE_SPLIT_REGISTER1_OFS 0x063c
REGDEF_BEGIN(RHE_FCURVE_SPLIT_REGISTER1)
REGDEF_BIT(ife_f_fcurve_split_lut16,        2)
REGDEF_BIT(ife_f_fcurve_split_lut17,        2)
REGDEF_BIT(ife_f_fcurve_split_lut18,        2)
REGDEF_BIT(ife_f_fcurve_split_lut19,        2)
REGDEF_BIT(ife_f_fcurve_split_lut20,        2)
REGDEF_BIT(ife_f_fcurve_split_lut21,        2)
REGDEF_BIT(ife_f_fcurve_split_lut22,        2)
REGDEF_BIT(ife_f_fcurve_split_lut23,        2)
REGDEF_BIT(ife_f_fcurve_split_lut24,        2)
REGDEF_BIT(ife_f_fcurve_split_lut25,        2)
REGDEF_BIT(ife_f_fcurve_split_lut26,        2)
REGDEF_BIT(ife_f_fcurve_split_lut27,        2)
REGDEF_BIT(ife_f_fcurve_split_lut28,        2)
REGDEF_BIT(ife_f_fcurve_split_lut29,        2)
REGDEF_BIT(ife_f_fcurve_split_lut30,        2)
REGDEF_BIT(ife_f_fcurve_split_lut31,        2)
REGDEF_END(RHE_FCURVE_SPLIT_REGISTER1)


/*
    ife_f_fcurve_val_lut0:    [0x0, 0xfff],         bits : 11_0
    ife_f_fcurve_val_lut1:    [0x0, 0xfff],         bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER0_OFS 0x0640
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER0)
REGDEF_BIT(ife_f_fcurve_val_lut0,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut1,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER0)


/*
    ife_f_fcurve_val_lut2:    [0x0, 0xfff],         bits : 11_0
    ife_f_fcurve_val_lut3:    [0x0, 0xfff],         bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER1_OFS 0x0644
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER1)
REGDEF_BIT(ife_f_fcurve_val_lut2,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut3,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER1)


/*
    ife_f_fcurve_val_lut4:    [0x0, 0xfff],         bits : 11_0
    ife_f_fcurve_val_lut5:    [0x0, 0xfff],         bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER2_OFS 0x0648
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER2)
REGDEF_BIT(ife_f_fcurve_val_lut4,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut5,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER2)


/*
    ife_f_fcurve_val_lut6:    [0x0, 0xfff],         bits : 11_0
    ife_f_fcurve_val_lut7:    [0x0, 0xfff],         bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER3_OFS 0x064c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER3)
REGDEF_BIT(ife_f_fcurve_val_lut6,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut7,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER3)


/*
    ife_f_fcurve_val_lut8:    [0x0, 0xfff],         bits : 11_0
    ife_f_fcurve_val_lut9:    [0x0, 0xfff],         bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER4_OFS 0x0650
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER4)
REGDEF_BIT(ife_f_fcurve_val_lut8,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut9,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER4)


/*
    ife_f_fcurve_val_lut10:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut11:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER5_OFS 0x0654
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER5)
REGDEF_BIT(ife_f_fcurve_val_lut10,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut11,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER5)


/*
    ife_f_fcurve_val_lut12:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut13:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER6_OFS 0x0658
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER6)
REGDEF_BIT(ife_f_fcurve_val_lut12,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut13,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER6)


/*
    ife_f_fcurve_val_lut14:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut15:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER7_OFS 0x065c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER7)
REGDEF_BIT(ife_f_fcurve_val_lut14,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut15,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER7)


/*
    ife_f_fcurve_val_lut16:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut17:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER8_OFS 0x0660
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER8)
REGDEF_BIT(ife_f_fcurve_val_lut16,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut17,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER8)


/*
    ife_f_fcurve_val_lut18:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut19:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER9_OFS 0x0664
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER9)
REGDEF_BIT(ife_f_fcurve_val_lut18,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut19,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER9)


/*
    ife_f_fcurve_val_lut20:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut21:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER10_OFS 0x0668
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER10)
REGDEF_BIT(ife_f_fcurve_val_lut20,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut21,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER10)


/*
    ife_f_fcurve_val_lut22:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut23:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER11_OFS 0x066c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER11)
REGDEF_BIT(ife_f_fcurve_val_lut22,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut23,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER11)


/*
    ife_f_fcurve_val_lut24:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut25:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER12_OFS 0x0670
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER12)
REGDEF_BIT(ife_f_fcurve_val_lut24,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut25,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER12)


/*
    ife_f_fcurve_val_lut26:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut27:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER13_OFS 0x0674
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER13)
REGDEF_BIT(ife_f_fcurve_val_lut26,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut27,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER13)


/*
    ife_f_fcurve_val_lut28:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut29:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER14_OFS 0x0678
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER14)
REGDEF_BIT(ife_f_fcurve_val_lut28,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut29,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER14)


/*
    ife_f_fcurve_val_lut30:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut31:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER15_OFS 0x067c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER15)
REGDEF_BIT(ife_f_fcurve_val_lut30,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut31,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER15)


/*
    ife_f_fcurve_val_lut32:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut33:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER16_OFS 0x0680
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER16)
REGDEF_BIT(ife_f_fcurve_val_lut32,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut33,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER16)


/*
    ife_f_fcurve_val_lut34:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut35:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER17_OFS 0x0684
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER17)
REGDEF_BIT(ife_f_fcurve_val_lut34,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut35,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER17)


/*
    ife_f_fcurve_val_lut36:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut37:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER18_OFS 0x0688
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER18)
REGDEF_BIT(ife_f_fcurve_val_lut36,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut37,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER18)


/*
    ife_f_fcurve_val_lut38:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut39:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER19_OFS 0x068c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER19)
REGDEF_BIT(ife_f_fcurve_val_lut38,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut39,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER19)


/*
    ife_f_fcurve_val_lut40:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut41:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER20_OFS 0x0690
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER20)
REGDEF_BIT(ife_f_fcurve_val_lut40,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut41,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER20)


/*
    ife_f_fcurve_val_lut42:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut43:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER21_OFS 0x0694
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER21)
REGDEF_BIT(ife_f_fcurve_val_lut42,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut43,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER21)


/*
    ife_f_fcurve_val_lut44:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut45:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER22_OFS 0x0698
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER22)
REGDEF_BIT(ife_f_fcurve_val_lut44,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut45,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER22)


/*
    ife_f_fcurve_val_lut46:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut47:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER23_OFS 0x069c
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER23)
REGDEF_BIT(ife_f_fcurve_val_lut46,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut47,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER23)


/*
    ife_f_fcurve_val_lut48:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut49:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER24_OFS 0x06a0
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER24)
REGDEF_BIT(ife_f_fcurve_val_lut48,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut49,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER24)


/*
    ife_f_fcurve_val_lut50:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut51:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER25_OFS 0x06a4
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER25)
REGDEF_BIT(ife_f_fcurve_val_lut50,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut51,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER25)


/*
    ife_f_fcurve_val_lut52:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut53:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER26_OFS 0x06a8
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER26)
REGDEF_BIT(ife_f_fcurve_val_lut52,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut53,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER26)


/*
    ife_f_fcurve_val_lut54:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut55:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER27_OFS 0x06ac
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER27)
REGDEF_BIT(ife_f_fcurve_val_lut54,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut55,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER27)


/*
    ife_f_fcurve_val_lut56:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut57:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER28_OFS 0x06b0
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER28)
REGDEF_BIT(ife_f_fcurve_val_lut56,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut57,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER28)


/*
    ife_f_fcurve_val_lut58:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut59:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER29_OFS 0x06b4
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER29)
REGDEF_BIT(ife_f_fcurve_val_lut58,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut59,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER29)


/*
    ife_f_fcurve_val_lut60:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut61:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER30_OFS 0x06b8
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER30)
REGDEF_BIT(ife_f_fcurve_val_lut60,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut61,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER30)


/*
    ife_f_fcurve_val_lut62:    [0x0, 0xfff],            bits : 11_0
    ife_f_fcurve_val_lut63:    [0x0, 0xfff],            bits : 27_16
*/
#define RHE_FCURVE_VALUE_REGISTER31_OFS 0x06bc
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER31)
REGDEF_BIT(ife_f_fcurve_val_lut62,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_f_fcurve_val_lut63,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER31)


/*
    ife_f_fcurve_val_lut64:    [0x0, 0xfff],            bits : 11_0
*/
#define RHE_FCURVE_VALUE_REGISTER32_OFS 0x06c0
REGDEF_BEGIN(RHE_FCURVE_VALUE_REGISTER32)
REGDEF_BIT(ife_f_fcurve_val_lut64,        12)
REGDEF_END(RHE_FCURVE_VALUE_REGISTER32)

/*
    ife_r_degamma_en:    [0x0, 0x1],            bits : 0
    ife_r_dith_en   :    [0x0, 0x1],            bits : 4
    ife_r_segbitno  :    [0x0, 0x3],            bits : 9_8
    ife_r_dith_rst  :    [0x0, 0x1],            bits : 12
*/
#define RDE_CONTROL_OFS 0x0700
REGDEF_BEGIN(RDE_CONTROL)
REGDEF_BIT(ife_r_degamma_en,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_r_dith_en,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_r_segbitno,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ife_r_dith_rst,        1)
REGDEF_END(RDE_CONTROL)


/*
    ife_r_dct_qtbl0_idx:    [0x0, 0x1f],            bits : 4_0
    ife_r_dct_qtbl1_idx:    [0x0, 0x1f],            bits : 12_8
    ife_r_dct_qtbl2_idx:    [0x0, 0x1f],            bits : 20_16
    ife_r_dct_qtbl3_idx:    [0x0, 0x1f],            bits : 28_24
*/
#define DCT_QTBL_REGISTER0_OFS 0x0704
REGDEF_BEGIN(DCT_QTBL_REGISTER0)
REGDEF_BIT(ife_r_dct_qtbl0_idx,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_r_dct_qtbl1_idx,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_r_dct_qtbl2_idx,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_r_dct_qtbl3_idx,        5)
REGDEF_END(DCT_QTBL_REGISTER0)


/*
    ife_r_dct_qtbl4_idx:    [0x0, 0x1f],            bits : 4_0
    ife_r_dct_qtbl5_idx:    [0x0, 0x1f],            bits : 12_8
    ife_r_dct_qtbl6_idx:    [0x0, 0x1f],            bits : 20_16
    ife_r_dct_qtbl7_idx:    [0x0, 0x1f],            bits : 28_24
*/
#define DCT_QTBL_REGISTER1_OFS 0x0708
REGDEF_BEGIN(DCT_QTBL_REGISTER1)
REGDEF_BIT(ife_r_dct_qtbl4_idx,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_r_dct_qtbl5_idx,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_r_dct_qtbl6_idx,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ife_r_dct_qtbl7_idx,        5)
REGDEF_END(DCT_QTBL_REGISTER1)


/*
    ife_r_out_rand1_init1:    [0x0, 0xf],           bits : 3_0
    ife_r_out_rand1_init2:    [0x0, 0x7fff],            bits : 18_4
*/
#define DITHERING_INITIAL_REGISTER0_OFS 0x070c
REGDEF_BEGIN(DITHERING_INITIAL_REGISTER0)
REGDEF_BIT(ife_r_out_rand1_init1,        4)
REGDEF_BIT(ife_r_out_rand1_init2,        15)
REGDEF_END(DITHERING_INITIAL_REGISTER0)


/*
    ife_r_out_rand2_init1:    [0x0, 0xf],           bits : 3_0
    ife_r_out_rand2_init2:    [0x0, 0x7fff],            bits : 18_4
*/
#define DITHERING_INITIAL_REGISTER1_OFS 0x0710
REGDEF_BEGIN(DITHERING_INITIAL_REGISTER1)
REGDEF_BIT(ife_r_out_rand2_init1,        4)
REGDEF_BIT(ife_r_out_rand2_init2,        15)
REGDEF_END(DITHERING_INITIAL_REGISTER1)


/*
    ife_r_degamma_tbl0:    [0x0, 0xfff],            bits : 11_0
    ife_r_degamma_tbl1:    [0x0, 0xfff],            bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER0_OFS 0x0714
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER0)
REGDEF_BIT(ife_r_degamma_tbl0,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl1,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER0)


/*
    ife_r_degamma_tbl2:    [0x0, 0xfff],            bits : 11_0
    ife_r_degamma_tbl3:    [0x0, 0xfff],            bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER1_OFS 0x0718
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER1)
REGDEF_BIT(ife_r_degamma_tbl2,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl3,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER1)


/*
    ife_r_degamma_tbl4:    [0x0, 0xfff],            bits : 11_0
    ife_r_degamma_tbl5:    [0x0, 0xfff],            bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER2_OFS 0x071c
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER2)
REGDEF_BIT(ife_r_degamma_tbl4,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl5,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER2)


/*
    ife_r_degamma_tbl6:    [0x0, 0xfff],            bits : 11_0
    ife_r_degamma_tbl7:    [0x0, 0xfff],            bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER3_OFS 0x0720
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER3)
REGDEF_BIT(ife_r_degamma_tbl6,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl7,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER3)


/*
    ife_r_degamma_tbl8:    [0x0, 0xfff],            bits : 11_0
    ife_r_degamma_tbl9:    [0x0, 0xfff],            bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER4_OFS 0x0724
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER4)
REGDEF_BIT(ife_r_degamma_tbl8,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl9,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER4)


/*
    ife_r_degamma_tbl10:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl11:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER5_OFS 0x0728
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER5)
REGDEF_BIT(ife_r_degamma_tbl10,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl11,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER5)


/*
    ife_r_degamma_tbl12:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl13:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER6_OFS 0x072c
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER6)
REGDEF_BIT(ife_r_degamma_tbl12,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl13,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER6)


/*
    ife_r_degamma_tbl14:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl15:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER7_OFS 0x0730
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER7)
REGDEF_BIT(ife_r_degamma_tbl14,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl15,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER7)


/*
    ife_r_degamma_tbl16:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl17:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER8_OFS 0x0734
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER8)
REGDEF_BIT(ife_r_degamma_tbl16,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl17,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER8)


/*
    ife_r_degamma_tbl18:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl19:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER9_OFS 0x0738
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER9)
REGDEF_BIT(ife_r_degamma_tbl18,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl19,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER9)


/*
    ife_r_degamma_tbl20:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl21:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER10_OFS 0x073c
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER10)
REGDEF_BIT(ife_r_degamma_tbl20,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl21,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER10)


/*
    ife_r_degamma_tbl22:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl23:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER11_OFS 0x0740
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER11)
REGDEF_BIT(ife_r_degamma_tbl22,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl23,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER11)


/*
    ife_r_degamma_tbl24:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl25:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER12_OFS 0x0744
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER12)
REGDEF_BIT(ife_r_degamma_tbl24,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl25,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER12)


/*
    ife_r_degamma_tbl26:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl27:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER13_OFS 0x0748
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER13)
REGDEF_BIT(ife_r_degamma_tbl26,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl27,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER13)


/*
    ife_r_degamma_tbl28:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl29:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER14_OFS 0x074c
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER14)
REGDEF_BIT(ife_r_degamma_tbl28,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl29,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER14)


/*
    ife_r_degamma_tbl30:    [0x0, 0xfff],           bits : 11_0
    ife_r_degamma_tbl31:    [0x0, 0xfff],           bits : 27_16
*/
#define DEGAMMA_TBL_REGISTER15_OFS 0x0750
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER15)
REGDEF_BIT(ife_r_degamma_tbl30,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ife_r_degamma_tbl31,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER15)


/*
    IFE_R_DEGAMMA_TBL32:    [0x0, 0xfff],           bits : 11_0
*/
#define DEGAMMA_TBL_REGISTER16_OFS 0x0754
REGDEF_BEGIN(DEGAMMA_TBL_REGISTER16)
REGDEF_BIT(ife_r_degamma_tbl32,        12)
REGDEF_END(DEGAMMA_TBL_REGISTER16)


typedef struct {
	//0x0000
	T_FILTER_OPERATION_CONTROL_REGISTER
	IFE_Register_0000;

	//0x0004
	T_CONTROL_REGISTER
	IFE_Register_0004;

	//0x0008
	T_FILTER_INTERRUPT_ENABLE_REGISTER
	IFE_Register_0008;

	//0x000c
	T_FILTER_INTERRUPT_STATUS_REGISTER
	IFE_Register_000c;

	//0x0010
	T_DEBUG_REGISTER
	IFE_Register_0010;

	//0x0014
	T_STRIPE_REGISTER
	IFE_Register_0014;

	//0x0020
	T_SOURCE_SIZE_REGISTER_0
	IFE_Register_0020;

	//0x0024
	T_SOURCE_SIZE_REGISTER_1
	IFE_Register_0024;

	//0x0028
	T_SOURCE_SIZE_REGISTER_2
	IFE_Register_0028;

	//0x0030
	T_SOURCE_ADDRESS_REGISTER_0
	IFE_Register_0030;

	//0x0034
	T_SOURCE_LINE_OFFSET_REGISTER_0
	IFE_Register_0034;

	//0x0038
	T_SOURCE_ADDRESS_REGISTER_1
	IFE_Register_0038;

	//0x003c
	T_SOURCE_LINE_OFFSET_REGISTER_1
	IFE_Register_003c;

	//0x0050
	T_SOURCE_ADDRESS_REGISTER_2
	IFE_Register_0050;

	//0x0054
	T_DESTINATION_ADDRESS_REGISTER
	IFE_Register_0054;

	//0x0058
	T_DESTINATION_LINE_OFFSET_REGISTER
	IFE_Register_0058;

	//0x005c
	T_DRAM_SETTINGS
	IFE_Register_005c;

	//0x0070
	T_COLOR_GAIN_REGISTER_0
	IFE_Register_0070;

	//0x0074
	T_COLOR_GAIN_REGISTER_1
	IFE_Register_0074;

	//0x0078
	T_COLOR_GAIN_REGISTER_2
	IFE_Register_0078;

	//0x007c
	T_COLOR_GAIN_REGISTER_3
	IFE_Register_007c;

	//0x0080
	T_COLOR_GAIN_REGISTER_4
	IFE_Register_0080;

	//0x0084
	T_COLOR_GAIN_REGISTER_5
	IFE_Register_0084;

	//0x0088
	T_COLOR_GAIN_REGISTER_6
	IFE_Register_0088;

	//0x008c
	T_COLOR_GAIN_REGISTER_7
	IFE_Register_008c;

	//0x0090
	T_COLOR_GAIN_REGISTER_8
	IFE_Register_0090;

	//0x0094
	T_COLOR_GAIN_REGISTER_9
	IFE_Register_0094;

	//0x00b0
	T_COLOR_OFFSET_REGISTER_0
	IFE_Register_00b0;

	//0x00b4
	T_COLOR_OFFSET_REGISTER_1
	IFE_Register_00b4;

	//0x00b8
	T_COLOR_OFFSET_REGISTER_2
	IFE_Register_00b8;

	//0x00bc
	T_COLOR_OFFSET_REGISTER_3
	IFE_Register_00bc;

	//0x00c0
	T_COLOR_OFFSET_REGISTER_4
	IFE_Register_00c0;

	//0x00c4
	T_COLOR_OFFSET_REGISTER_5
	IFE_Register_00c4;

	//0x00c8
	T_COLOR_OFFSET_REGISTER_6
	IFE_Register_00c8;

	//0x00cc
	T_COLOR_OFFSET_REGISTER_7
	IFE_Register_00cc;

	//0x00d0
	T_COLOR_OFFSET_REGISTER_8
	IFE_Register_00d0;

	//0x0100
	T_OUTPUT_FILTER_REGISTER
	IFE_Register_0100;

	//0x0104
	T_RANGE_FILTER_REGISTER_0
	IFE_Register_0104;

	//0x0108
	T_SPATIAL_FILTER_REGISTER_1
	IFE_Register_0108;

	//0x010c
	T_SPATIAL_FILTER_REGISTER_2
	IFE_Register_010c;

	//0x0110
	T_RANGE_FILTER_REGISTER_1
	IFE_Register_0110;

	//0x0114
	T_RANGE_FILTER_REGISTER_2
	IFE_Register_0114;

	//0x0118
	T_RANGE_FILTER_REGISTER_3
	IFE_Register_0118;

	//0x011c
	T_RANGE_FILTER_REGISTER_4
	IFE_Register_011c;

	//0x0120
	T_RANGE_FILTER_REGISTER_5
	IFE_Register_0120;

	//0x0124
	T_RANGE_FILTER_REGISTER_6
	IFE_Register_0124;

	//0x0128
	T_RANGE_FILTER_REGISTER_7
	IFE_Register_0128;

	//0x012c
	T_RANGE_FILTER_REGISTER_8
	IFE_Register_012c;

	//0x0130
	T_RANGE_FILTER_REGISTER_9
	IFE_Register_0130;

	//0x0134
	T_RANGE_FILTER_REGISTER_10
	IFE_Register_0134;

	//0x0138
	T_RANGE_FILTER_REGISTER_11
	IFE_Register_0138;

	//0x013c
	T_RANGE_FILTER_REGISTER_12
	IFE_Register_013c;

	//0x0140
	T_RANGE_FILTER_REGISTER_13
	IFE_Register_0140;

	//0x0144
	T_RANGE_FILTER_REGISTER_14
	IFE_Register_0144;

	//0x0148
	T_RANGE_FILTER_REGISTER_15
	IFE_Register_0148;

	//0x014c
	T_RANGE_FILTER_REGISTER_16
	IFE_Register_014c;

	//0x0150
	T_RANGE_FILTER_REGISTER_17
	IFE_Register_0150;

	//0x0154
	T_RANGE_FILTER_REGISTER_18
	IFE_Register_0154;

	//0x0158
	T_RANGE_FILTER_REGISTER_19
	IFE_Register_0158;

	//0x015c
	T_RANGE_FILTER_REGISTER_20
	IFE_Register_015c;

	//0x0160
	T_RANGE_FILTER_REGISTER_21
	IFE_Register_0160;

	//0x0164
	T_RANGE_FILTER_REGISTER_22
	IFE_Register_0164;

	//0x0168
	T_RANGE_FILTER_REGISTER_23
	IFE_Register_0168;

	//0x016c
	T_RANGE_FILTER_REGISTER_24
	IFE_Register_016c;

	//0x0170
	T_RANGE_FILTER_REGISTER_25
	IFE_Register_0170;

	//0x0174
	T_RANGE_FILTER_REGISTER_26
	IFE_Register_0174;

	//0x0178
	T_RANGE_FILTER_REGISTER_27
	IFE_Register_0178;

	//0x017c
	T_RANGE_FILTER_REGISTER_28
	IFE_Register_017c;

	//0x0180
	T_RANGE_FILTER_REGISTER_29
	IFE_Register_0180;

	//0x0184
	T_RANGE_FILTER_REGISTER_30
	IFE_Register_0184;

	//0x0188
	T_RANGE_FILTER_REGISTER_31
	IFE_Register_0188;

	//0x018c
	T_OUTLIER_THRESHOLD_REGISTER_0
	IFE_Register_018c;

	//0x0190
	T_OUTLIER_THRESHOLD_REGISTER_1
	IFE_Register_0190;

	//0x0194
	T_OUTLIER_THRESHOLD_REGISTER_2
	IFE_Register_0194;

	//0x0198
	T_OUTLIER_THRESHOLD_REGISTER_3
	IFE_Register_0198;

	//0x019c
	T_OUTLIER_THRESHOLD_REGISTER_4
	IFE_Register_019c;

	//0x01a0
	T_OUTLIER_THRESHOLD_REGISTER_5
	IFE_Register_01a0;

	//0x01a4
	T_OUTLIER_THRESHOLD_REGISTER_6
	IFE_Register_01a4;

	//0x01a8
	T_OUTLIER_ORDER_REGISTER_0
	IFE_Register_01a8;

	//0x01ac
	T_OUTLIER_ORDER_REGISTER_1
	IFE_Register_01ac;

	//0x01b0
	T_OUTLIER_ORDER_REGISTER_2
	IFE_Register_01b0;

	//0x01b4
	T_GBAL_REGISTER_0
	IFE_Register_01b4;

	//0x01b8
	T_GBAL_REGISTER_1
	IFE_Register_01b8;

	//0x01bc
	T_GBAL_REGISTER_2
	IFE_Register_01bc;

	//0x01c0
	T_RANGE_FILTER_REGISTER_32
	IFE_Register_01c0;

	//0x01c4
	T_RANGE_FILTER_REGISTER_33
	IFE_Register_01c4;

	//0x01c8
	T_RANGE_FILTER_REGISTER_34
	IFE_Register_01c8;

	//0x01cc
	T_RANGE_FILTER_REGISTER_35
	IFE_Register_01cc;

	//0x01d0
	T_RANGE_FILTER_REGISTER_36
	IFE_Register_01d0;

	//0x01d4
	T_RANGE_FILTER_REGISTER_37
	IFE_Register_01d4;

	//0x01d8
	T_RANGE_FILTER_REGISTER_38
	IFE_Register_01d8;

	//0x01dc
	T_RANGE_FILTER_REGISTER_39
	IFE_Register_01dc;

	//0x01e0
	T_RANGE_FILTER_REGISTER_40
	IFE_Register_01e0;

	//0x01e4
	T_RANGE_FILTER_REGISTER_41
	IFE_Register_01e4;

	//0x01e8
	T_RANGE_FILTER_REGISTER_42
	IFE_Register_01e8;

	//0x01ec
	T_RANGE_FILTER_REGISTER_43
	IFE_Register_01ec;

	//0x01f0
	T_RANGE_FILTER_REGISTER_44
	IFE_Register_01f0;

	//0x01f4
	T_RANGE_FILTER_REGISTER_45
	IFE_Register_01f4;

	//0x01f8
	T_RANGE_FILTER_REGISTER_46
	IFE_Register_01f8;

	//0x01fc
	T_RANGE_FILTER_REGISTER_47
	IFE_Register_01fc;

	//0x0200
	T_RANGE_FILTER_REGISTER_48
	IFE_Register_0200;

	//0x0204
	T_RANGE_FILTER_REGISTER_49
	IFE_Register_0204;

	//0x0208
	T_RANGE_FILTER_REGISTER_50
	IFE_Register_0208;

	//0x020c
	T_RANGE_FILTER_REGISTER_51
	IFE_Register_020c;

	//0x0210
	T_RANGE_FILTER_REGISTER_52
	IFE_Register_0210;

	//0x0214
	T_RANGE_FILTER_REGISTER_53
	IFE_Register_0214;

	//0x0218
	T_RANGE_FILTER_REGISTER_54
	IFE_Register_0218;

	//0x021c
	T_RANGE_FILTER_REGISTER_55
	IFE_Register_021c;

	//0x0220
	T_RANGE_FILTER_REGISTER_56
	IFE_Register_0220;

	//0x0224
	T_RANGE_FILTER_REGISTER_57
	IFE_Register_0224;

	//0x0228
	T_RANGE_FILTER_REGISTER_58
	IFE_Register_0228;

	//0x022c
	T_RANGE_FILTER_REGISTER_59
	IFE_Register_022c;

	//0x0230
	T_RANGE_FILTER_REGISTER_60
	IFE_Register_0230;

	//0x0234
	T_RANGE_FILTER_REGISTER_61
	IFE_Register_0234;

	//0x0238
	T_RANGE_FILTER_REGISTER_62
	IFE_Register_0238;

	//0x023c
	T_RANGE_FILTER_REGISTER_63
	IFE_Register_023c;

	//0x0240
	T_RANGE_FILTER_REGISTER_64
	IFE_Register_0240;

	//0x0244
	T_RANGE_FILTER_REGISTER_65
	IFE_Register_0244;

	//0x0248
	T_RANGE_FILTER_REGISTER_66
	IFE_Register_0248;

	//0x024c
	T_RANGE_FILTER_REGISTER_67
	IFE_Register_024c;

	//0x0250
	T_RANGE_FILTER_REGISTER_68
	IFE_Register_0250;

	//0x0254
	T_RANGE_FILTER_REGISTER_69
	IFE_Register_0254;

	//0x0258
	T_RANGE_FILTER_REGISTER_70
	IFE_Register_0258;

	//0x025c
	T_RANGE_FILTER_REGISTER_71
	IFE_Register_025c;

	//0x0260
	T_RANGE_FILTER_REGISTER_72
	IFE_Register_0260;

	//0x0264
	T_RANGE_FILTER_REGISTER_73
	IFE_Register_0264;

	//0x0268
	T_RANGE_FILTER_REGISTER_74
	IFE_Register_0268;

	//0x026c
	T_RANGE_FILTER_REGISTER_75
	IFE_Register_026c;

	//0x0270
	T_RANGE_FILTER_REGISTER_76
	IFE_Register_0270;

	//0x0274
	T_RANGE_FILTER_REGISTER_77
	IFE_Register_0274;

	//0x0278
	T_RANGE_FILTER_REGISTER_78
	IFE_Register_0278;

	//0x027c
	T_RANGE_FILTER_REGISTER_79
	IFE_Register_027c;

	//0x0280
	T_RANGE_FILTER_REGISTER_80
	IFE_Register_0280;

	//0x0284
	T_RANGE_FILTER_REGISTER_81
	IFE_Register_0284;

	//0x0288
	T_RANGE_FILTER_REGISTER_82
	IFE_Register_0288;

	//0x028c
	T_RANGE_FILTER_REGISTER_83
	IFE_Register_028c;

	//0x0290
	T_RANGE_FILTER_REGISTER_84
	IFE_Register_0290;

	//0x0294
	T_RANGE_FILTER_REGISTER_85
	IFE_Register_0294;

	//0x0298
	T_RANGE_FILTER_REGISTER_86
	IFE_Register_0298;

	//0x029c
	T_RANGE_FILTER_REGISTER_87
	IFE_Register_029c;

	//0x02a0
	T_RANGE_FILTER_REGISTER_88
	IFE_Register_02a0;

	//0x02a4
	T_RANGE_FILTER_REGISTER_89
	IFE_Register_02a4;

	//0x02a8
	T_RANGE_FILTER_REGISTER_90
	IFE_Register_02a8;

	//0x02ac
	T_RANGE_FILTER_REGISTER_91
	IFE_Register_02ac;

	//0x02b0
	T_RANGE_FILTER_REGISTER_92
	IFE_Register_02b0;

	//0x02b4
	T_RANGE_FILTER_REGISTER_93
	IFE_Register_02b4;

	//0x02b8
	T_RANGE_FILTER_REGISTER_94
	IFE_Register_02b8;

	//0x02bc
	T_RANGE_FILTER_REGISTER_95
	IFE_Register_02bc;

	//0x02c0
	T_RANGE_FILTER_REGISTER_96
	IFE_Register_02c0;

	//0x02c4
	T_RANGE_FILTER_REGISTER_97
	IFE_Register_02c4;

	//0x02c8
	T_RANGE_FILTER_REGISTER_98
	IFE_Register_02c8;

	//0x02cc
	T_RANGE_FILTER_REGISTER_99
	IFE_Register_02cc;

	//0x02d0
	T_RANGE_FILTER_REGISTER_100
	IFE_Register_02d0;

	//0x02d4
	T_RANGE_FILTER_REGISTER_101
	IFE_Register_02d4;

	//0x02d8
	T_RANGE_FILTER_REGISTER_102
	IFE_Register_02d8;

	//0x02dc
	T_RANGE_FILTER_REGISTER_103
	IFE_Register_02dc;

	//0x02e0
	T_VIGNETTE_SETTING_REGISTER_0
	IFE_Register_02e0;

	//0x02e4
	T_VIGNETTE_SETTING_REGISTER_1
	IFE_Register_02e4;

	//0x02e8
	T_VIGNETTE_SETTING_REGISTER_2
	IFE_Register_02e8;

	//0x02ec
	T_VIGNETTE_SETTING_REGISTER_3
	IFE_Register_02ec;

	//0x02f0
	T_VIGNETTE_SETTING_REGISTER_4
	IFE_Register_02f0;

	//0x02f4
	T_VIGNETTE_SETTING_REGISTER_5
	IFE_Register_02f4;

	//0x02f8
	T_VIGNETTE_REGISTER_0
	IFE_Register_02f8;

	//0x02fc
	T_VIGNETTE_REGISTER_1
	IFE_Register_02fc;

	//0x0300
	T_VIGNETTE_REGISTER_2
	IFE_Register_0300;

	//0x0304
	T_VIGNETTE_REGISTER_3
	IFE_Register_0304;

	//0x0308
	T_VIGNETTE_REGISTER_4
	IFE_Register_0308;

	//0x030c
	T_VIGNETTE_REGISTER_5
	IFE_Register_030c;

	//0x0310
	T_VIGNETTE_REGISTER_6
	IFE_Register_0310;

	//0x0314
	T_VIGNETTE_REGISTER_7
	IFE_Register_0314;

	//0x0318
	T_VIGNETTE_REGISTER_8
	IFE_Register_0318;

	//0x031c
	T_VIGNETTE_REGISTER_9
	IFE_Register_031c;

	//0x0320
	T_VIGNETTE_REGISTER_10
	IFE_Register_0320;

	//0x0324
	T_VIGNETTE_REGISTER_11
	IFE_Register_0324;

	//0x0328
	T_VIGNETTE_REGISTER_12
	IFE_Register_0328;

	//0x032c
	T_VIGNETTE_REGISTER_13
	IFE_Register_032c;

	//0x0330
	T_VIGNETTE_REGISTER_14
	IFE_Register_0330;

	//0x0334
	T_VIGNETTE_REGISTER_15
	IFE_Register_0334;

	//0x0338
	T_VIGNETTE_REGISTER_16
	IFE_Register_0338;

	//0x033c
	T_VIGNETTE_REGISTER_17
	IFE_Register_033c;

	//0x0340
	T_VIGNETTE_REGISTER_18
	IFE_Register_0340;

	//0x0344
	T_VIGNETTE_REGISTER_19
	IFE_Register_0344;

	//0x0348
	T_VIGNETTE_REGISTER_20
	IFE_Register_0348;

	//0x034c
	T_VIGNETTE_REGISTER_21
	IFE_Register_034c;

	//0x0350
	T_VIGNETTE_REGISTER_22
	IFE_Register_0350;

	//0x0354
	T_VIGNETTE_REGISTER_23
	IFE_Register_0354;

	//0x0358
	T_VIGNETTE_REGISTER_24
	IFE_Register_0358;

	//0x035c
	T_VIGNETTE_REGISTER_25
	IFE_Register_035c;

	//0x0360
	T_VIGNETTE_REGISTER_26
	IFE_Register_0360;

	//0x0364
	T_VIGNETTE_REGISTER_27
	IFE_Register_0364;

	//0x0368
	T_VIGNETTE_REGISTER_28
	IFE_Register_0368;

	//0x036c
	T_VIGNETTE_REGISTER_29
	IFE_Register_036c;

	//0x0370
	T_VIGNETTE_REGISTER_30
	IFE_Register_0370;

	//0x0374
	T_VIGNETTE_REGISTER_31
	IFE_Register_0374;

	//0x0378
	T_VIGNETTE_REGISTER_32
	IFE_Register_0378;

	//0x037c
	T_VIGNETTE_REGISTER_33
	IFE_Register_037c;

	//0x0380
	T_VIGNETTE_REGISTER_34
	IFE_Register_0380;

	//0x0384
	T_VIGNETTE_REGISTER_35
	IFE_Register_0384;

	//0x0388
	T_GBAL_REGISTER_3
	IFE_Register_0388;

	//0x038c
	T_GBAL_REGISTER_4
	IFE_Register_038c;

	//0x0390
	T_GBAL_REGISTER_5
	IFE_Register_0390;

	//0x0394
	T_GBAL_REGISTER_6
	IFE_Register_0394;

	//0x0398
	T_GBAL_REGISTER_7
	IFE_Register_0398;

	//0x039c
	T_RB_LUMINANCE_REGISTER_0
	IFE_Register_039c;

	//0x03a0
	T_RB_LUMINANCE_REGISTER_1
	IFE_Register_03a0;

	//0x03a4
	T_RB_LUMINANCE_REGISTER_2
	IFE_Register_03a4;

	//0x03a8
	T_RB_LUMINANCE_REGISTER_3
	IFE_Register_03a8;

	//0x03ac
	T_RB_LUMINANCE_REGISTER_4
	IFE_Register_03ac;

	//0x03c0
	T_RB_RATIO_REGISTER_0
	IFE_Register_03c0;

	//0x03c4
	T_RB_RATIO_REGISTER_1
	IFE_Register_03c4;

	//0x03c8
	T_RB_RATIO_REGISTER_2
	IFE_Register_03c8;

	//0x03cc
	T_RB_RATIO_REGISTER_3
	IFE_Register_03cc;

	//0x03d0
	T_RB_RATIO_REGISTER_4
	IFE_Register_03d0;

	//0x03d4
	T_RB_RATIO_REGISTER_5
	IFE_Register_03d4;

	//0x03d8
	T_RB_RATIO_REGISTER_6
	IFE_Register_03d8;

	//0x03dc
	T_RB_RATIO_REGISTER_7
	IFE_Register_03dc;

	//0x0500
	T_NRS_REGISTER
	IFE_Register_0500;

	//0x0504
	T_RHE_NRS_S_REGISTER
	IFE_Register_0504;

	//0x0508
	T_RHE_NRS_ORDER_LUT_DARK_WEIGHT
	IFE_Register_0508;

	//0x050c
	T_RHE_NRS_ORDER_LUT_BRIGHT_WEIGHT
	IFE_Register_050c;

	//0x0510
	T_RHE_NRS_BILATERAL_LUT_OFFSET0
	IFE_Register_0510;

	//0x0514
	T_RHE_NRS_BILATERAL_LUT_OFFSET1
	IFE_Register_0514;

	//0x0518
	T_RHE_NRS_BILATERAL_LUT_WEIGHT
	IFE_Register_0518;

	//0x051c
	T_RHE_NRS_BILATERAL_LUT_RANGE0
	IFE_Register_051c;

	//0x0520
	T_RHE_NRS_BILATERAL_LUT_RANGE1
	IFE_Register_0520;

	//0x0524
	T_RHE_NRS_BILATERAL_LUT_RANGE2
	IFE_Register_0524;

	//0x0528
	T_RHE_NRS_GBILATERAL_LUT_OFFSET0
	IFE_Register_0528;

	//0x052c
	T_RHE_NRS_GBILATERAL_LUT_OFFSET1
	IFE_Register_052c;

	//0x0530
	T_RHE_NRS_GBILATERAL_LUT_WEIGHT
	IFE_Register_0530;

	//0x0534
	T_RHE_NRS_GBILATERAL_LUT_RANGE0
	IFE_Register_0534;

	//0x0538
	T_RHE_NRS_GBILATERAL_LUT_RANGE1
	IFE_Register_0538;

	//0x053c
	T_RHE_NRS_GBILATERAL_LUT_RANGE2
	IFE_Register_053c;

	//0x0550
	T_RHE_FUSION_REGISTER
	IFE_Register_0550;

	//0x0554
	T_RHE_FUSION_LONG_EXP_NORMAL_BLEND_CURVE
	IFE_Register_0554;

	//0x0558
	T_RHE_FUSION_SHORT_EXP_NORMAL_BLEND_CURVE
	IFE_Register_0558;

	//0x055c
	T_RHE_FUSION_LONG_EXP_DIFF_BLEND_CURVE
	IFE_Register_055c;

	//0x0560
	T_RHE_FUSION_SHORT_EXP_DIFF_BLEND_CURVE
	IFE_Register_0560;

	//0x0570
	T_RHE_FUSION_MOTION_COMPENSATION
	IFE_Register_0570;

	//0x0574
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_0
	IFE_Register_0574;

	//0x0578
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_1
	IFE_Register_0578;

	//0x057c
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_2
	IFE_Register_057c;

	//0x0580
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_3
	IFE_Register_0580;

	//0x0584
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_4
	IFE_Register_0584;

	//0x0588
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_5
	IFE_Register_0588;

	//0x0590
	T_RHE_FUSION_PATH0_DARK_SATURATION_REDUCTION
	IFE_Register_0590;

	//0x0594
	T_RHE_FUSION_PATH1_DARK_SATURATION_REDUCTION
	IFE_Register_0594;

	//0x05a0
	T_RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_0
	IFE_Register_05a0;

	//0x05a4
	T_RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_1
	IFE_Register_05a4;

	//0x05a8
	T_RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_2
	IFE_Register_05a8;

	//0x05ac
	T_RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_3
	IFE_Register_05ac;

	//0x05b0
	T_RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_4
	IFE_Register_05b0;

	//0x0600
	T_RHE_FCURVE_CTRL
	IFE_Register_0600;

	//0x0604
	T_RHE_FCURVE_Y_WEIGHT_REGISTER0
	IFE_Register_0604;

	//0x0608
	T_RHE_FCURVE_Y_WEIGHT_REGISTER1
	IFE_Register_0608;

	//0x060c
	T_RHE_FCURVE_Y_WEIGHT_REGISTER2
	IFE_Register_060c;

	//0x0610
	T_RHE_FCURVE_Y_WEIGHT_REGISTER3
	IFE_Register_0610;

	//0x0614
	T_RHE_FCURVE_Y_WEIGHT_REGISTER4
	IFE_Register_0614;

	//0x0618
	T_RHE_FCURVE_INDEX_REGISTER0
	IFE_Register_0618;

	//0x061c
	T_RHE_FCURVE_INDEX_REGISTER1
	IFE_Register_061c;

	//0x0620
	T_RHE_FCURVE_INDEX_REGISTER2
	IFE_Register_0620;

	//0x0624
	T_RHE_FCURVE_INDEX_REGISTER3
	IFE_Register_0624;

	//0x0628
	T_RHE_FCURVE_INDEX_REGISTER4
	IFE_Register_0628;

	//0x062c
	T_RHE_FCURVE_INDEX_REGISTER5
	IFE_Register_062c;

	//0x0630
	T_RHE_FCURVE_INDEX_REGISTER6
	IFE_Register_0630;

	//0x0634
	T_RHE_FCURVE_INDEX_REGISTER7
	IFE_Register_0634;

	//0x0638
	T_RHE_FCURVE_SPLIT_REGISTER0
	IFE_Register_0638;

	//0x063c
	T_RHE_FCURVE_SPLIT_REGISTER1
	IFE_Register_063c;

	//0x0640
	T_RHE_FCURVE_VALUE_REGISTER0
	IFE_Register_0640;

	//0x0644
	T_RHE_FCURVE_VALUE_REGISTER1
	IFE_Register_0644;

	//0x0648
	T_RHE_FCURVE_VALUE_REGISTER2
	IFE_Register_0648;

	//0x064c
	T_RHE_FCURVE_VALUE_REGISTER3
	IFE_Register_064c;

	//0x0650
	T_RHE_FCURVE_VALUE_REGISTER4
	IFE_Register_0650;

	//0x0654
	T_RHE_FCURVE_VALUE_REGISTER5
	IFE_Register_0654;

	//0x0658
	T_RHE_FCURVE_VALUE_REGISTER6
	IFE_Register_0658;

	//0x065c
	T_RHE_FCURVE_VALUE_REGISTER7
	IFE_Register_065c;

	//0x0660
	T_RHE_FCURVE_VALUE_REGISTER8
	IFE_Register_0660;

	//0x0664
	T_RHE_FCURVE_VALUE_REGISTER9
	IFE_Register_0664;

	//0x0668
	T_RHE_FCURVE_VALUE_REGISTER10
	IFE_Register_0668;

	//0x066c
	T_RHE_FCURVE_VALUE_REGISTER11
	IFE_Register_066c;

	//0x0670
	T_RHE_FCURVE_VALUE_REGISTER12
	IFE_Register_0670;

	//0x0674
	T_RHE_FCURVE_VALUE_REGISTER13
	IFE_Register_0674;

	//0x0678
	T_RHE_FCURVE_VALUE_REGISTER14
	IFE_Register_0678;

	//0x067c
	T_RHE_FCURVE_VALUE_REGISTER15
	IFE_Register_067c;

	//0x0680
	T_RHE_FCURVE_VALUE_REGISTER16
	IFE_Register_0680;

	//0x0684
	T_RHE_FCURVE_VALUE_REGISTER17
	IFE_Register_0684;

	//0x0688
	T_RHE_FCURVE_VALUE_REGISTER18
	IFE_Register_0688;

	//0x068c
	T_RHE_FCURVE_VALUE_REGISTER19
	IFE_Register_068c;

	//0x0690
	T_RHE_FCURVE_VALUE_REGISTER20
	IFE_Register_0690;

	//0x0694
	T_RHE_FCURVE_VALUE_REGISTER21
	IFE_Register_0694;

	//0x0698
	T_RHE_FCURVE_VALUE_REGISTER22
	IFE_Register_0698;

	//0x069c
	T_RHE_FCURVE_VALUE_REGISTER23
	IFE_Register_069c;

	//0x06a0
	T_RHE_FCURVE_VALUE_REGISTER24
	IFE_Register_06a0;

	//0x06a4
	T_RHE_FCURVE_VALUE_REGISTER25
	IFE_Register_06a4;

	//0x06a8
	T_RHE_FCURVE_VALUE_REGISTER26
	IFE_Register_06a8;

	//0x06ac
	T_RHE_FCURVE_VALUE_REGISTER27
	IFE_Register_06ac;

	//0x06b0
	T_RHE_FCURVE_VALUE_REGISTER28
	IFE_Register_06b0;

	//0x06b4
	T_RHE_FCURVE_VALUE_REGISTER29
	IFE_Register_06b4;

	//0x06b8
	T_RHE_FCURVE_VALUE_REGISTER30
	IFE_Register_06b8;

	//0x06bc
	T_RHE_FCURVE_VALUE_REGISTER31
	IFE_Register_06bc;

	//0x06c0
	T_RHE_FCURVE_VALUE_REGISTER32
	IFE_Register_06c0;

	//0x0700
	T_RDE_CONTROL
	IFE_Register_0700;

	//0x0704
	T_DCT_QTBL_REGISTER0
	IFE_Register_0704;

	//0x0708
	T_DCT_QTBL_REGISTER1
	IFE_Register_0708;

	//0x070c
	T_DITHERING_INITIAL_REGISTER0
	IFE_Register_070c;

	//0x0710
	T_DITHERING_INITIAL_REGISTER1
	IFE_Register_0710;

	//0x0714
	T_DEGAMMA_TBL_REGISTER0
	IFE_Register_0714;

	//0x0718
	T_DEGAMMA_TBL_REGISTER1
	IFE_Register_0718;

	//0x071c
	T_DEGAMMA_TBL_REGISTER2
	IFE_Register_071c;

	//0x0720
	T_DEGAMMA_TBL_REGISTER3
	IFE_Register_0720;

	//0x0724
	T_DEGAMMA_TBL_REGISTER4
	IFE_Register_0724;

	//0x0728
	T_DEGAMMA_TBL_REGISTER5
	IFE_Register_0728;

	//0x072c
	T_DEGAMMA_TBL_REGISTER6
	IFE_Register_072c;

	//0x0730
	T_DEGAMMA_TBL_REGISTER7
	IFE_Register_0730;

	//0x0734
	T_DEGAMMA_TBL_REGISTER8
	IFE_Register_0734;

	//0x0738
	T_DEGAMMA_TBL_REGISTER9
	IFE_Register_0738;

	//0x073c
	T_DEGAMMA_TBL_REGISTER10
	IFE_Register_073c;

	//0x0740
	T_DEGAMMA_TBL_REGISTER11
	IFE_Register_0740;

	//0x0744
	T_DEGAMMA_TBL_REGISTER12
	IFE_Register_0744;

	//0x0748
	T_DEGAMMA_TBL_REGISTER13
	IFE_Register_0748;

	//0x074c
	T_DEGAMMA_TBL_REGISTER14
	IFE_Register_074c;

	//0x0750
	T_DEGAMMA_TBL_REGISTER15
	IFE_Register_0750;

	//0x0754
	T_DEGAMMA_TBL_REGISTER16
	IFE_Register_0754;

} NT98520_IFE_REG_STRUCT;

#endif