#ifndef _VPE_NT98528_H_
#define _VPE_NT98528_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined (__KERNEL__)

#include "mach/rcw_macro.h"
#include "kwrap/type.h"//a header for basic variable type

//=========================================================================
#elif defined (__FREERTOS)

#include "rcw_macro.h"
#include "kwrap/type.h"
#include <kwrap/nvt_type.h>

//=========================================================================
#elif defined (__UITRON) || defined (__ECOS)

#else


#endif



#if 0
#include "DrvCommon.h"

#define _VPE_REG_BASE_ADDR       0xF0CD0000

#define VPE_REG_ADDR(ofs)        (_VPE_REG_BASE_ADDR+(ofs))
#define VPE_SETREG(ofs, value)   OUTW((_VPE_REG_BASE_ADDR + ofs), value)
#define VPE_GETREG(ofs)          INW(_VPE_REG_BASE_ADDR + ofs)
#endif

/*
    VPE_SW_RST    :    [0x0, 0x1],          bits : 0
    VPE_DMA_SW_RST:    [0x0, 0x1],          bits : 1
    VPE_START     :    [0x0, 0x1],          bits : 30
    VPE_LL_FIRE   :    [0x0, 0x1],          bits : 31
*/
#define GLOBAL_CONTROL_REGISTER0_OFS 0x0000
REGDEF_BEGIN(GLOBAL_CONTROL_REGISTER0)
REGDEF_BIT(VPE_SW_RST,        1)
REGDEF_BIT(VPE_DMA_SW_RST,        1)
REGDEF_BIT(,        28)
REGDEF_BIT(VPE_START,        1)
REGDEF_BIT(VPE_LL_FIRE,        1)
REGDEF_END(GLOBAL_CONTROL_REGISTER0)


/*
    VPE_SHP_EN         :    [0x0, 0x1],         bits : 2
    DCE_EN             :    [0x0, 0x1],         bits : 3
    VPE_DCTG_EN        :    [0x0, 0x1],         bits : 4
    VPE_DCTG_GMA_LUT_EN:    [0x0, 0x1],         bits : 5
    DCE_2D_LUT_EN      :    [0x0, 0x1],         bits : 6
    VPE_DEBUG_TYPE     :    [0x0, 0xf],         bits : 23_20
    VPE_COL_NUM        :    [0x0, 0x7],         bits : 26_24
    VPE_LL_TERMINATE   :    [0x0, 0x1],         bits : 31
*/
#define GLOBAL_CONTROL_REGISTER1_OFS 0x0004
REGDEF_BEGIN(GLOBAL_CONTROL_REGISTER1)
REGDEF_BIT(,        2)
REGDEF_BIT(VPE_SHP_EN,        1)
REGDEF_BIT(DCE_EN,        1)
REGDEF_BIT(VPE_DCTG_EN,        1)
REGDEF_BIT(VPE_DCTG_GMA_LUT_EN,        1)
REGDEF_BIT(DCE_2D_LUT_EN,        1)
REGDEF_BIT(,        13)
REGDEF_BIT(VPE_DEBUG_TYPE,        4)
REGDEF_BIT(VPE_COL_NUM,        3)
REGDEF_BIT(,        4)
REGDEF_BIT(VPE_LL_TERMINATE,        1)
REGDEF_END(GLOBAL_CONTROL_REGISTER1)


/*
    VPE_SRC_FMT    :    [0x0, 0x7],         bits : 12_10
    VPE_SHP_OUT_SEL:    [0x0, 0x1],         bits : 17
    VPE_SRC_DRT    :    [0x0, 0x3],         bits : 21_20
    VPE_SRC_UV_SWAP:    [0x0, 0x1],         bits : 24
*/
#define GLOBAL_CONTROL_REGISTER2_OFS 0x0008
REGDEF_BEGIN(GLOBAL_CONTROL_REGISTER2)
REGDEF_BIT(,        10)
REGDEF_BIT(VPE_SRC_FMT,        3)
REGDEF_BIT(,        4)
REGDEF_BIT(VPE_SHP_OUT_SEL,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(VPE_SRC_DRT,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(VPE_SRC_UV_SWAP,        1)
REGDEF_END(GLOBAL_CONTROL_REGISTER2)


/*
    VPE_Reserved_0:    [0x0, 0xffffffff],           bits : 31_0
*/
#define VPE_RESERVED_0_OFS 0x000c
REGDEF_BEGIN(VPE_RESERVED_0)
REGDEF_BIT(VPE_Reserved_0,        32)
REGDEF_END(VPE_RESERVED_0)


/*
    VPE_SRC_BUF_WIDTH :    [0x0, 0x3fff],           bits : 13_0
    VPE_SRC_BUF_HEIGHT:    [0x0, 0x1fff],           bits : 28_16
*/
#define GLOBAL_SIZE_REGISTER1_OFS 0x0010
REGDEF_BEGIN(GLOBAL_SIZE_REGISTER1)
REGDEF_BIT(VPE_SRC_BUF_WIDTH,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(VPE_SRC_BUF_HEIGHT,        13)
REGDEF_END(GLOBAL_SIZE_REGISTER1)


/*
    VPE_Reserved_1:    [0x0, 0xffffffff],           bits : 31_0
*/
#define VPE_RESERVED_1_OFS 0x0014
REGDEF_BEGIN(VPE_RESERVED_1)
REGDEF_BIT(VPE_Reserved_1,        32)
REGDEF_END(VPE_RESERVED_1)


/*
    VPE_Reserved_2:    [0x0, 0xffffffff],           bits : 31_0
*/
#define VPE_RESERVED_2_OFS 0x0018
REGDEF_BEGIN(VPE_RESERVED_2)
REGDEF_BIT(VPE_Reserved_2,        32)
REGDEF_END(VPE_RESERVED_2)


/*
    VPE_PROC_HEIGHT   :    [0x0, 0x1fff],           bits : 12_0
    PRESCA_MERGE_WIDTH:    [0x0, 0x1fff],           bits : 28_16
*/
#define GLOBAL_SIZE_REGISTER4_OFS 0x001c
REGDEF_BEGIN(GLOBAL_SIZE_REGISTER4)
REGDEF_BIT(VPE_PROC_HEIGHT,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(PRESCA_MERGE_WIDTH,        13)
REGDEF_END(GLOBAL_SIZE_REGISTER4)


/*
    VPE_PROC_Y_START:    [0x0, 0x1fff],         bits : 12_0
*/
#define GLOBAL_SIZE_REGISTER5_OFS 0x0020
REGDEF_BEGIN(GLOBAL_SIZE_REGISTER5)
REGDEF_BIT(VPE_PROC_Y_START,        13)
REGDEF_END(GLOBAL_SIZE_REGISTER5)


/*
    VPE_Reserved_2:    [0x0, 0xffffffff],           bits : 31_0
*/
#define VPE_RESERVED_9999_OFS 0x0024
REGDEF_BEGIN(VPE_RESERVED_9999)
REGDEF_BIT(VPE_Reserved_2,        32)
REGDEF_END(VPE_RESERVED_9999)


/*
    VPE_Reserved_3:    [0x0, 0xffffffff],           bits : 31_0
*/
#define VPE_RESERVED_3_OFS 0x0028
REGDEF_BEGIN(VPE_RESERVED_3)
REGDEF_BIT(VPE_Reserved_3,        32)
REGDEF_END(VPE_RESERVED_3)


/*
    VPE_Reserved_4:    [0x0, 0xffffffff],           bits : 31_0
*/
#define VPE_RESERVED_4_OFS 0x002c
REGDEF_BEGIN(VPE_RESERVED_4)
REGDEF_BIT(VPE_Reserved_4,        32)
REGDEF_END(VPE_RESERVED_4)


/*
    DCE_WIDTH :    [0x0, 0x1fff],           bits : 12_0
    DCE_HEIGHT:    [0x0, 0x1fff],           bits : 28_16
*/
#define GLOBAL_SIZE_REGISTER9_OFS 0x0030
REGDEF_BEGIN(GLOBAL_SIZE_REGISTER9)
REGDEF_BIT(DCE_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCE_HEIGHT,        13)
REGDEF_END(GLOBAL_SIZE_REGISTER9)


/*
    VPE_Reserved_5:    [0x0, 0xffffffff],           bits : 31_0
*/
#define VPE_RESERVED_5_OFS 0x0034
REGDEF_BEGIN(VPE_RESERVED_5)
REGDEF_BIT(VPE_Reserved_5,        32)
REGDEF_END(VPE_RESERVED_5)


/*
    VPE_Reserved_6:    [0x0, 0xffffffff],           bits : 31_0
*/
#define VPE_RESERVED_6_OFS 0x0038
REGDEF_BEGIN(VPE_RESERVED_6)
REGDEF_BIT(VPE_Reserved_6,        32)
REGDEF_END(VPE_RESERVED_6)


/*
    VPE_Reserved_7:    [0x0, 0xffffffff],           bits : 31_0
*/
#define VPE_RESERVED_7_OFS 0x003c
REGDEF_BEGIN(VPE_RESERVED_7)
REGDEF_BIT(VPE_Reserved_7,        32)
REGDEF_END(VPE_RESERVED_7)


/*
    VPE_Reserved_8:    [0x0, 0xffffffff],           bits : 31_0
*/
#define VPE_RESERVED_8_OFS 0x0040
REGDEF_BEGIN(VPE_RESERVED_8)
REGDEF_BIT(VPE_Reserved_8,        32)
REGDEF_END(VPE_RESERVED_8)


/*
    VPE_Reserved_9:    [0x0, 0xffffffff],           bits : 31_0
*/
#define VPE_RESERVED_9_OFS 0x0044
REGDEF_BEGIN(VPE_RESERVED_9)
REGDEF_BIT(VPE_Reserved_9,        32)
REGDEF_END(VPE_RESERVED_9)


/*
    VPE_Reserved_10:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_10_OFS 0x0048
REGDEF_BEGIN(VPE_RESERVED_10)
REGDEF_BIT(VPE_Reserved_10,        32)
REGDEF_END(VPE_RESERVED_10)


/*
    VPE_Reserved_11:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_11_OFS 0x004c
REGDEF_BEGIN(VPE_RESERVED_11)
REGDEF_BIT(VPE_Reserved_11,        32)
REGDEF_END(VPE_RESERVED_11)


/*
    VPE_Reserved_12:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_12_OFS 0x0050
REGDEF_BEGIN(VPE_RESERVED_12)
REGDEF_BIT(VPE_Reserved_12,        32)
REGDEF_END(VPE_RESERVED_12)


/*
    VPE_Reserved_13:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_13_OFS 0x0054
REGDEF_BEGIN(VPE_RESERVED_13)
REGDEF_BIT(VPE_Reserved_13,        32)
REGDEF_END(VPE_RESERVED_13)


/*
    VPE_Reserved_14:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_14_OFS 0x0058
REGDEF_BEGIN(VPE_RESERVED_14)
REGDEF_BIT(VPE_Reserved_14,        32)
REGDEF_END(VPE_RESERVED_14)


/*
    VPE_Reserved_15:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_15_OFS 0x005c
REGDEF_BEGIN(VPE_RESERVED_15)
REGDEF_BIT(VPE_Reserved_15,        32)
REGDEF_END(VPE_RESERVED_15)


/*
    VPE_Reserved_16:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_16_OFS 0x0060
REGDEF_BEGIN(VPE_RESERVED_16)
REGDEF_BIT(VPE_Reserved_16,        32)
REGDEF_END(VPE_RESERVED_16)


/*
    VPE_Reserved_17:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_17_OFS 0x0064
REGDEF_BEGIN(VPE_RESERVED_17)
REGDEF_BIT(VPE_Reserved_17,        32)
REGDEF_END(VPE_RESERVED_17)


/*
    VPE_Reserved_18:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_18_OFS 0x0068
REGDEF_BEGIN(VPE_RESERVED_18)
REGDEF_BIT(VPE_Reserved_18,        32)
REGDEF_END(VPE_RESERVED_18)


/*
    VPE_Reserved_19:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_19_OFS 0x006c
REGDEF_BEGIN(VPE_RESERVED_19)
REGDEF_BIT(VPE_Reserved_19,        32)
REGDEF_END(VPE_RESERVED_19)


/*
    VPE_Reserved_20:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_20_OFS 0x0070
REGDEF_BEGIN(VPE_RESERVED_20)
REGDEF_BIT(VPE_Reserved_20,        32)
REGDEF_END(VPE_RESERVED_20)


/*
    VPE_Reserved_21:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_21_OFS 0x0074
REGDEF_BEGIN(VPE_RESERVED_21)
REGDEF_BIT(VPE_Reserved_21,        32)
REGDEF_END(VPE_RESERVED_21)


/*
    VPE_Reserved_22:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_22_OFS 0x0078
REGDEF_BEGIN(VPE_RESERVED_22)
REGDEF_BIT(VPE_Reserved_22,        32)
REGDEF_END(VPE_RESERVED_22)


/*
    VPE_Reserved_23:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_23_OFS 0x007c
REGDEF_BEGIN(VPE_RESERVED_23)
REGDEF_BIT(VPE_Reserved_23,        32)
REGDEF_END(VPE_RESERVED_23)


/*
    VPE_Reserved_24:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_24_OFS 0x0080
REGDEF_BEGIN(VPE_RESERVED_24)
REGDEF_BIT(VPE_Reserved_24,        32)
REGDEF_END(VPE_RESERVED_24)


/*
    VPE_Reserved_25:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_25_OFS 0x0084
REGDEF_BEGIN(VPE_RESERVED_25)
REGDEF_BIT(VPE_Reserved_25,        32)
REGDEF_END(VPE_RESERVED_25)


/*
    VPE_Reserved_26:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_26_OFS 0x0088
REGDEF_BEGIN(VPE_RESERVED_26)
REGDEF_BIT(VPE_Reserved_26,        32)
REGDEF_END(VPE_RESERVED_26)


/*
    VPE_Reserved_27:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_27_OFS 0x008c
REGDEF_BEGIN(VPE_RESERVED_27)
REGDEF_BIT(VPE_Reserved_27,        32)
REGDEF_END(VPE_RESERVED_27)


/*
    VPE_Reserved_28:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_28_OFS 0x0090
REGDEF_BEGIN(VPE_RESERVED_28)
REGDEF_BIT(VPE_Reserved_28,        32)
REGDEF_END(VPE_RESERVED_28)


/*
    VPE_Reserved_29:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_29_OFS 0x0094
REGDEF_BEGIN(VPE_RESERVED_29)
REGDEF_BIT(VPE_Reserved_29,        32)
REGDEF_END(VPE_RESERVED_29)


/*
    VPE_Reserved_30:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_30_OFS 0x0098
REGDEF_BEGIN(VPE_RESERVED_30)
REGDEF_BIT(VPE_Reserved_30,        32)
REGDEF_END(VPE_RESERVED_30)


/*
    VPE_Reserved_31:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_31_OFS 0x009c
REGDEF_BEGIN(VPE_RESERVED_31)
REGDEF_BIT(VPE_Reserved_31,        32)
REGDEF_END(VPE_RESERVED_31)


/*
    VPE_Reserved_32:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_32_OFS 0x00a0
REGDEF_BEGIN(VPE_RESERVED_32)
REGDEF_BIT(VPE_Reserved_32,        32)
REGDEF_END(VPE_RESERVED_32)


/*
    VPE_Reserved_33:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_33_OFS 0x00a4
REGDEF_BEGIN(VPE_RESERVED_33)
REGDEF_BIT(VPE_Reserved_33,        32)
REGDEF_END(VPE_RESERVED_33)


/*
    VPE_Reserved_34:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_34_OFS 0x00a8
REGDEF_BEGIN(VPE_RESERVED_34)
REGDEF_BIT(VPE_Reserved_34,        32)
REGDEF_END(VPE_RESERVED_34)


/*
    VPE_Reserved_35:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_35_OFS 0x00ac
REGDEF_BEGIN(VPE_RESERVED_35)
REGDEF_BIT(VPE_Reserved_35,        32)
REGDEF_END(VPE_RESERVED_35)


/*
    VCH_IDX_SHT:    [0x0, 0xf],         bits : 7_4
    VCH_IDX_SET:    [0x0, 0x1],         bits : 12
    VCH_IDX_OFT:    [0x0, 0xffff],          bits : 31_16
*/
#define VCACHE_REGISTER1_OFS 0x00b0
REGDEF_BEGIN(VCACHE_REGISTER1)
REGDEF_BIT(,        4)
REGDEF_BIT(VCH_IDX_SHT,        4)
REGDEF_BIT(,        4)
REGDEF_BIT(VCH_IDX_SET,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(VCH_IDX_OFT,        16)
REGDEF_END(VCACHE_REGISTER1)


/*
    VCH_OFT_MODE:    [0x0, 0xf],            bits : 3_0
    VCH_OFT_SHT :    [0x0, 0x3],            bits : 5_4
*/
#define VCACHE_REGISTER2_OFS 0x00b4
REGDEF_BEGIN(VCACHE_REGISTER2)
REGDEF_BIT(VCH_OFT_MODE,        4)
REGDEF_BIT(VCH_OFT_SHT,        2)
REGDEF_END(VCACHE_REGISTER2)


/*
    VCH_SLOT_WIDTH :    [0x0, 0x1ff],           bits : 8_0
    VCH_SLOT_HEIGHT:    [0x0, 0x1ff],           bits : 24_16
*/
#define VCACHE_REGISTER3_OFS 0x00b8
REGDEF_BEGIN(VCACHE_REGISTER3)
REGDEF_BIT(VCH_SLOT_WIDTH,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(VCH_SLOT_HEIGHT,        9)
REGDEF_END(VCACHE_REGISTER3)


/*
    VCH_OFT_WIDTH :    [0x0, 0x1ff],            bits : 8_0
    VCH_OFT_HEIGHT:    [0x0, 0x1ff],            bits : 24_16
*/
#define VCACHE_REGISTER4_OFS 0x00bc
REGDEF_BEGIN(VCACHE_REGISTER4)
REGDEF_BIT(VCH_OFT_WIDTH,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(VCH_OFT_HEIGHT,        9)
REGDEF_END(VCACHE_REGISTER4)


/*
    DCE_MODE    :    [0x0, 0x3],            bits : 1_0
    DCE_LUT2D_SZ:    [0x0, 0x7],            bits : 4_2
    DCE_LSB_RAND:    [0x0, 0x1],            bits : 5
*/
#define DCE_REGISTER1_OFS 0x00c0
REGDEF_BEGIN(DCE_REGISTER1)
REGDEF_BIT(DCE_MODE,        2)
REGDEF_BIT(DCE_LUT2D_SZ,        3)
REGDEF_BIT(DCE_LSB_RAND,        1)
REGDEF_END(DCE_REGISTER1)


/*
    DCE_FOV_BOUND:    [0x0, 0x1],           bits : 0
    DCE_BOUND_Y  :    [0x0, 0x3ff],         bits : 25_16
*/
#define DCE_REGISTER2_OFS 0x00c4
REGDEF_BEGIN(DCE_REGISTER2)
REGDEF_BIT(DCE_FOV_BOUND,        1)
REGDEF_BIT(,        15)
REGDEF_BIT(DCE_BOUND_Y,        10)
REGDEF_END(DCE_REGISTER2)


/*
    DCE_BOUND_U:    [0x0, 0x3ff],           bits : 9_0
    DCE_BOUND_V:    [0x0, 0x3ff],           bits : 25_16
*/
#define DCE_REGISTER3_OFS 0x00c8
REGDEF_BEGIN(DCE_REGISTER3)
REGDEF_BIT(DCE_BOUND_U,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCE_BOUND_V,        10)
REGDEF_END(DCE_REGISTER3)


/*
    DCE_CENT_X_S:    [0x0, 0x3fff],         bits : 13_0
    DCE_CENT_Y_S:    [0x0, 0x3fff],         bits : 29_16
*/
#define DCE_REGISTER4_OFS 0x00cc
REGDEF_BEGIN(DCE_REGISTER4)
REGDEF_BIT(DCE_CENT_X_S,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(DCE_CENT_Y_S,        14)
REGDEF_END(DCE_REGISTER4)


/*
    DCE_XDIST:    [0x0, 0xfff],         bits : 11_0
    DCE_YDIST:    [0x0, 0xfff],         bits : 27_16
*/
#define DCE_REGISTER5_OFS 0x00d0
REGDEF_BEGIN(DCE_REGISTER5)
REGDEF_BIT(DCE_XDIST,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(DCE_YDIST,        12)
REGDEF_END(DCE_REGISTER5)


/*
    DCE_NORM_FACT:    [0x0, 0x3ff],         bits : 9_0
    DCE_NORM_BIT :    [0x0, 0x1f],          bits : 16_12
    DCE_FOV_GAIN :    [0x0, 0xfff],         bits : 31_20
*/
#define DCE_REGISTER6_OFS 0x00d4
REGDEF_BEGIN(DCE_REGISTER6)
REGDEF_BIT(DCE_NORM_FACT,        10)
REGDEF_BIT(,        2)
REGDEF_BIT(DCE_NORM_BIT,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(DCE_FOV_GAIN,        12)
REGDEF_END(DCE_REGISTER6)


/*
    DCE_HFACT:    [0x0, 0xffffff],          bits : 23_0
*/
#define DCE_REGISTER7_OFS 0x00d8
REGDEF_BEGIN(DCE_REGISTER7)
REGDEF_BIT(DCE_HFACT,        24)
REGDEF_END(DCE_REGISTER7)


/*
    DCE_VFACT:    [0x0, 0xffffff],          bits : 23_0
*/
#define DCE_REGISTER8_OFS 0x00dc
REGDEF_BEGIN(DCE_REGISTER8)
REGDEF_BIT(DCE_VFACT,        24)
REGDEF_END(DCE_REGISTER8)


/*
    DCE_XOFS_INT:    [0x0, 0x1ff],          bits : 8_0
    DCE_YOFS_INT:    [0x0, 0x1ff],          bits : 24_16
*/
#define DCE_REGISTER9_OFS 0x00e0
REGDEF_BEGIN(DCE_REGISTER9)
REGDEF_BIT(DCE_XOFS_INT,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(DCE_YOFS_INT,        9)
REGDEF_END(DCE_REGISTER9)


/*
    DCE_XOFS_FRC:    [0x0, 0xffffff],           bits : 23_0
*/
#define DCE_REGISTER10_OFS 0x00e4
REGDEF_BEGIN(DCE_REGISTER10)
REGDEF_BIT(DCE_XOFS_FRC,        24)
REGDEF_END(DCE_REGISTER10)


/*
    DCE_YOFS_FRC:    [0x0, 0xffffff],           bits : 23_0
*/
#define DCE_REGISTER11_OFS 0x00e8
REGDEF_BEGIN(DCE_REGISTER11)
REGDEF_BIT(DCE_YOFS_FRC,        24)
REGDEF_END(DCE_REGISTER11)


/*
    VPE_Reserved_36:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_36_OFS 0x00ec
REGDEF_BEGIN(VPE_RESERVED_36)
REGDEF_BIT(VPE_Reserved_36,        32)
REGDEF_END(VPE_RESERVED_36)


/*
    VPE_Reserved_37:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_37_OFS 0x00f0
REGDEF_BEGIN(VPE_RESERVED_37)
REGDEF_BIT(VPE_Reserved_37,        32)
REGDEF_END(VPE_RESERVED_37)


/*
    VPE_Reserved_38:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_38_OFS 0x00f4
REGDEF_BEGIN(VPE_RESERVED_38)
REGDEF_BIT(VPE_Reserved_38,        32)
REGDEF_END(VPE_RESERVED_38)


/*
    VPE_Reserved_39:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_39_OFS 0x00f8
REGDEF_BEGIN(VPE_RESERVED_39)
REGDEF_BIT(VPE_Reserved_39,        32)
REGDEF_END(VPE_RESERVED_39)


/*
    VPE_Reserved_40:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_40_OFS 0x00fc
REGDEF_BEGIN(VPE_RESERVED_40)
REGDEF_BIT(VPE_Reserved_40,        32)
REGDEF_END(VPE_RESERVED_40)


/*
    VPE_Reserved_41:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_41_OFS 0x0100
REGDEF_BEGIN(VPE_RESERVED_41)
REGDEF_BIT(VPE_Reserved_41,        32)
REGDEF_END(VPE_RESERVED_41)


/*
    VPE_Reserved_42:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_42_OFS 0x0104
REGDEF_BEGIN(VPE_RESERVED_42)
REGDEF_BIT(VPE_Reserved_42,        32)
REGDEF_END(VPE_RESERVED_42)


/*
    VPE_Reserved_43:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_43_OFS 0x0108
REGDEF_BEGIN(VPE_RESERVED_43)
REGDEF_BIT(VPE_Reserved_43,        32)
REGDEF_END(VPE_RESERVED_43)


/*
    VPE_Reserved_44:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_44_OFS 0x010c
REGDEF_BEGIN(VPE_RESERVED_44)
REGDEF_BIT(VPE_Reserved_44,        32)
REGDEF_END(VPE_RESERVED_44)


/*
    VPE_Reserved_45:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_45_OFS 0x0110
REGDEF_BEGIN(VPE_RESERVED_45)
REGDEF_BIT(VPE_Reserved_45,        32)
REGDEF_END(VPE_RESERVED_45)


/*
    VPE_Reserved_46:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_46_OFS 0x0114
REGDEF_BEGIN(VPE_RESERVED_46)
REGDEF_BIT(VPE_Reserved_46,        32)
REGDEF_END(VPE_RESERVED_46)


/*
    VPE_Reserved_47:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_47_OFS 0x0118
REGDEF_BEGIN(VPE_RESERVED_47)
REGDEF_BIT(VPE_Reserved_47,        32)
REGDEF_END(VPE_RESERVED_47)


/*
    VPE_Reserved_48:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_48_OFS 0x011c
REGDEF_BEGIN(VPE_RESERVED_48)
REGDEF_BIT(VPE_Reserved_48,        32)
REGDEF_END(VPE_RESERVED_48)


/*
    VPE_Reserved_49:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_49_OFS 0x0120
REGDEF_BEGIN(VPE_RESERVED_49)
REGDEF_BIT(VPE_Reserved_49,        32)
REGDEF_END(VPE_RESERVED_49)


/*
    VPE_Reserved_50:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_50_OFS 0x0124
REGDEF_BEGIN(VPE_RESERVED_50)
REGDEF_BIT(VPE_Reserved_50,        32)
REGDEF_END(VPE_RESERVED_50)


/*
    VPE_Reserved_51:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_51_OFS 0x0128
REGDEF_BEGIN(VPE_RESERVED_51)
REGDEF_BIT(VPE_Reserved_51,        32)
REGDEF_END(VPE_RESERVED_51)


/*
    VPE_Reserved_52:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_52_OFS 0x012c
REGDEF_BEGIN(VPE_RESERVED_52)
REGDEF_BIT(VPE_Reserved_52,        32)
REGDEF_END(VPE_RESERVED_52)


/*
    VPE_Reserved_53:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_53_OFS 0x0130
REGDEF_BEGIN(VPE_RESERVED_53)
REGDEF_BIT(VPE_Reserved_53,        32)
REGDEF_END(VPE_RESERVED_53)


/*
    VPE_Reserved_54:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_54_OFS 0x0134
REGDEF_BEGIN(VPE_RESERVED_54)
REGDEF_BIT(VPE_Reserved_54,        32)
REGDEF_END(VPE_RESERVED_54)


/*
    VPE_Reserved_55:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_55_OFS 0x0138
REGDEF_BEGIN(VPE_RESERVED_55)
REGDEF_BIT(VPE_Reserved_55,        32)
REGDEF_END(VPE_RESERVED_55)


/*
    VPE_Reserved_56:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_56_OFS 0x013c
REGDEF_BEGIN(VPE_RESERVED_56)
REGDEF_BIT(VPE_Reserved_56,        32)
REGDEF_END(VPE_RESERVED_56)


/*
    VPE_Reserved_57:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_57_OFS 0x0140
REGDEF_BEGIN(VPE_RESERVED_57)
REGDEF_BIT(VPE_Reserved_57,        32)
REGDEF_END(VPE_RESERVED_57)


/*
    VPE_Reserved_58:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_58_OFS 0x0144
REGDEF_BEGIN(VPE_RESERVED_58)
REGDEF_BIT(VPE_Reserved_58,        32)
REGDEF_END(VPE_RESERVED_58)


/*
    VPE_Reserved_59:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_59_OFS 0x0148
REGDEF_BEGIN(VPE_RESERVED_59)
REGDEF_BIT(VPE_Reserved_59,        32)
REGDEF_END(VPE_RESERVED_59)


/*
    VPE_Reserved_60:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_60_OFS 0x014c
REGDEF_BEGIN(VPE_RESERVED_60)
REGDEF_BIT(VPE_Reserved_60,        32)
REGDEF_END(VPE_RESERVED_60)


/*
    VPE_Reserved_61:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_61_OFS 0x0150
REGDEF_BEGIN(VPE_RESERVED_61)
REGDEF_BIT(VPE_Reserved_61,        32)
REGDEF_END(VPE_RESERVED_61)


/*
    VPE_Reserved_62:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_62_OFS 0x0154
REGDEF_BEGIN(VPE_RESERVED_62)
REGDEF_BIT(VPE_Reserved_62,        32)
REGDEF_END(VPE_RESERVED_62)


/*
    VPE_Reserved_63:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_63_OFS 0x0158
REGDEF_BEGIN(VPE_RESERVED_63)
REGDEF_BIT(VPE_Reserved_63,        32)
REGDEF_END(VPE_RESERVED_63)


/*
    VPE_Reserved_64:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_64_OFS 0x015c
REGDEF_BEGIN(VPE_RESERVED_64)
REGDEF_BIT(VPE_Reserved_64,        32)
REGDEF_END(VPE_RESERVED_64)


/*
    VPE_Reserved_65:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_65_OFS 0x0160
REGDEF_BEGIN(VPE_RESERVED_65)
REGDEF_BIT(VPE_Reserved_65,        32)
REGDEF_END(VPE_RESERVED_65)


/*
    VPE_Reserved_66:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_66_OFS 0x0164
REGDEF_BEGIN(VPE_RESERVED_66)
REGDEF_BIT(VPE_Reserved_66,        32)
REGDEF_END(VPE_RESERVED_66)


/*
    VPE_Reserved_67:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_67_OFS 0x0168
REGDEF_BEGIN(VPE_RESERVED_67)
REGDEF_BIT(VPE_Reserved_67,        32)
REGDEF_END(VPE_RESERVED_67)


/*
    VPE_Reserved_68:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_68_OFS 0x016c
REGDEF_BEGIN(VPE_RESERVED_68)
REGDEF_BIT(VPE_Reserved_68,        32)
REGDEF_END(VPE_RESERVED_68)


/*
    VPE_Reserved_69:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_69_OFS 0x0170
REGDEF_BEGIN(VPE_RESERVED_69)
REGDEF_BIT(VPE_Reserved_69,        32)
REGDEF_END(VPE_RESERVED_69)


/*
    VPE_Reserved_70:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_70_OFS 0x0174
REGDEF_BEGIN(VPE_RESERVED_70)
REGDEF_BIT(VPE_Reserved_70,        32)
REGDEF_END(VPE_RESERVED_70)


/*
    VPE_Reserved_71:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_71_OFS 0x0178
REGDEF_BEGIN(VPE_RESERVED_71)
REGDEF_BIT(VPE_Reserved_71,        32)
REGDEF_END(VPE_RESERVED_71)


/*
    VPE_Reserved_72:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_72_OFS 0x017c
REGDEF_BEGIN(VPE_RESERVED_72)
REGDEF_BIT(VPE_Reserved_72,        32)
REGDEF_END(VPE_RESERVED_72)


/*
    VPE_Reserved_73:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_73_OFS 0x0180
REGDEF_BEGIN(VPE_RESERVED_73)
REGDEF_BIT(VPE_Reserved_73,        32)
REGDEF_END(VPE_RESERVED_73)


/*
    VPE_Reserved_74:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_74_OFS 0x0184
REGDEF_BEGIN(VPE_RESERVED_74)
REGDEF_BIT(VPE_Reserved_74,        32)
REGDEF_END(VPE_RESERVED_74)


/*
    VPE_Reserved_75:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_75_OFS 0x0188
REGDEF_BEGIN(VPE_RESERVED_75)
REGDEF_BIT(VPE_Reserved_75,        32)
REGDEF_END(VPE_RESERVED_75)


/*
    VPE_Reserved_76:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_76_OFS 0x018c
REGDEF_BEGIN(VPE_RESERVED_76)
REGDEF_BIT(VPE_Reserved_76,        32)
REGDEF_END(VPE_RESERVED_76)


/*
    VPE_Reserved_77:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_77_OFS 0x0190
REGDEF_BEGIN(VPE_RESERVED_77)
REGDEF_BIT(VPE_Reserved_77,        32)
REGDEF_END(VPE_RESERVED_77)


/*
    VPE_Reserved_78:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_78_OFS 0x0194
REGDEF_BEGIN(VPE_RESERVED_78)
REGDEF_BIT(VPE_Reserved_78,        32)
REGDEF_END(VPE_RESERVED_78)


/*
    VPE_Reserved_79:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_79_OFS 0x0198
REGDEF_BEGIN(VPE_RESERVED_79)
REGDEF_BIT(VPE_Reserved_79,        32)
REGDEF_END(VPE_RESERVED_79)


/*
    VPE_Reserved_80:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_80_OFS 0x019c
REGDEF_BEGIN(VPE_RESERVED_80)
REGDEF_BIT(VPE_Reserved_80,        32)
REGDEF_END(VPE_RESERVED_80)


/*
    VPE_Reserved_81:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_81_OFS 0x01a0
REGDEF_BEGIN(VPE_RESERVED_81)
REGDEF_BIT(VPE_Reserved_81,        32)
REGDEF_END(VPE_RESERVED_81)


/*
    VPE_Reserved_82:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_82_OFS 0x01a4
REGDEF_BEGIN(VPE_RESERVED_82)
REGDEF_BIT(VPE_Reserved_82,        32)
REGDEF_END(VPE_RESERVED_82)


/*
    VPE_Reserved_83:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_83_OFS 0x01a8
REGDEF_BEGIN(VPE_RESERVED_83)
REGDEF_BIT(VPE_Reserved_83,        32)
REGDEF_END(VPE_RESERVED_83)


/*
    VPE_Reserved_84:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_84_OFS 0x01ac
REGDEF_BEGIN(VPE_RESERVED_84)
REGDEF_BIT(VPE_Reserved_84,        32)
REGDEF_END(VPE_RESERVED_84)


/*
    VPE_Reserved_85:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_85_OFS 0x01b0
REGDEF_BEGIN(VPE_RESERVED_85)
REGDEF_BIT(VPE_Reserved_85,        32)
REGDEF_END(VPE_RESERVED_85)


/*
    VPE_Reserved_86:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_86_OFS 0x01b4
REGDEF_BEGIN(VPE_RESERVED_86)
REGDEF_BIT(VPE_Reserved_86,        32)
REGDEF_END(VPE_RESERVED_86)


/*
    VPE_Reserved_87:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_87_OFS 0x01b8
REGDEF_BEGIN(VPE_RESERVED_87)
REGDEF_BIT(VPE_Reserved_87,        32)
REGDEF_END(VPE_RESERVED_87)


/*
    VPE_Reserved_88:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_88_OFS 0x01bc
REGDEF_BEGIN(VPE_RESERVED_88)
REGDEF_BIT(VPE_Reserved_88,        32)
REGDEF_END(VPE_RESERVED_88)


/*
    VPE_Reserved_89:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_89_OFS 0x01c0
REGDEF_BEGIN(VPE_RESERVED_89)
REGDEF_BIT(VPE_Reserved_89,        32)
REGDEF_END(VPE_RESERVED_89)


/*
    VPE_Reserved_90:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_90_OFS 0x01c4
REGDEF_BEGIN(VPE_RESERVED_90)
REGDEF_BIT(VPE_Reserved_90,        32)
REGDEF_END(VPE_RESERVED_90)


/*
    VPE_Reserved_91:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_91_OFS 0x01c8
REGDEF_BEGIN(VPE_RESERVED_91)
REGDEF_BIT(VPE_Reserved_91,        32)
REGDEF_END(VPE_RESERVED_91)


/*
    VPE_Reserved_92:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_92_OFS 0x01cc
REGDEF_BEGIN(VPE_RESERVED_92)
REGDEF_BIT(VPE_Reserved_92,        32)
REGDEF_END(VPE_RESERVED_92)


/*
    VPE_Reserved_93:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_93_OFS 0x01d0
REGDEF_BEGIN(VPE_RESERVED_93)
REGDEF_BIT(VPE_Reserved_93,        32)
REGDEF_END(VPE_RESERVED_93)


/*
    VPE_Reserved_94:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_94_OFS 0x01d4
REGDEF_BEGIN(VPE_RESERVED_94)
REGDEF_BIT(VPE_Reserved_94,        32)
REGDEF_END(VPE_RESERVED_94)


/*
    VPE_Reserved_95:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_95_OFS 0x01d8
REGDEF_BEGIN(VPE_RESERVED_95)
REGDEF_BIT(VPE_Reserved_95,        32)
REGDEF_END(VPE_RESERVED_95)


/*
    VPE_Reserved_96:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_96_OFS 0x01dc
REGDEF_BEGIN(VPE_RESERVED_96)
REGDEF_BIT(VPE_Reserved_96,        32)
REGDEF_END(VPE_RESERVED_96)


/*
    VPE_Reserved_97:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_97_OFS 0x01e0
REGDEF_BEGIN(VPE_RESERVED_97)
REGDEF_BIT(VPE_Reserved_97,        32)
REGDEF_END(VPE_RESERVED_97)


/*
    VPE_Reserved_98:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_98_OFS 0x01e4
REGDEF_BEGIN(VPE_RESERVED_98)
REGDEF_BIT(VPE_Reserved_98,        32)
REGDEF_END(VPE_RESERVED_98)


/*
    VPE_Reserved_99:    [0x0, 0xffffffff],          bits : 31_0
*/
#define VPE_RESERVED_99_OFS 0x01e8
REGDEF_BEGIN(VPE_RESERVED_99)
REGDEF_BIT(VPE_Reserved_99,        32)
REGDEF_END(VPE_RESERVED_99)


/*
    VPE_Reserved_100:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_100_OFS 0x01ec
REGDEF_BEGIN(VPE_RESERVED_100)
REGDEF_BIT(VPE_Reserved_100,        32)
REGDEF_END(VPE_RESERVED_100)


/*
    VPE_Reserved_101:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_101_OFS 0x01f0
REGDEF_BEGIN(VPE_RESERVED_101)
REGDEF_BIT(VPE_Reserved_101,        32)
REGDEF_END(VPE_RESERVED_101)


/*
    VPE_Reserved_102:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_102_OFS 0x01f4
REGDEF_BEGIN(VPE_RESERVED_102)
REGDEF_BIT(VPE_Reserved_102,        32)
REGDEF_END(VPE_RESERVED_102)


/*
    VPE_Reserved_103:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_103_OFS 0x01f8
REGDEF_BEGIN(VPE_RESERVED_103)
REGDEF_BIT(VPE_Reserved_103,        32)
REGDEF_END(VPE_RESERVED_103)


/*
    VPE_Reserved_104:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_104_OFS 0x01fc
REGDEF_BEGIN(VPE_RESERVED_104)
REGDEF_BIT(VPE_Reserved_104,        32)
REGDEF_END(VPE_RESERVED_104)


/*
    VPE_Reserved_105:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_105_OFS 0x0200
REGDEF_BEGIN(VPE_RESERVED_105)
REGDEF_BIT(VPE_Reserved_105,        32)
REGDEF_END(VPE_RESERVED_105)


/*
    VPE_Reserved_106:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_106_OFS 0x0204
REGDEF_BEGIN(VPE_RESERVED_106)
REGDEF_BIT(VPE_Reserved_106,        32)
REGDEF_END(VPE_RESERVED_106)


/*
    VPE_Reserved_107:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_107_OFS 0x0208
REGDEF_BEGIN(VPE_RESERVED_107)
REGDEF_BIT(VPE_Reserved_107,        32)
REGDEF_END(VPE_RESERVED_107)


/*
    VPE_Reserved_108:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_108_OFS 0x020c
REGDEF_BEGIN(VPE_RESERVED_108)
REGDEF_BIT(VPE_Reserved_108,        32)
REGDEF_END(VPE_RESERVED_108)


/*
    VPE_Reserved_109:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_109_OFS 0x0210
REGDEF_BEGIN(VPE_RESERVED_109)
REGDEF_BIT(VPE_Reserved_109,        32)
REGDEF_END(VPE_RESERVED_109)


/*
    VPE_Reserved_110:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_110_OFS 0x0214
REGDEF_BEGIN(VPE_RESERVED_110)
REGDEF_BIT(VPE_Reserved_110,        32)
REGDEF_END(VPE_RESERVED_110)


/*
    VPE_Reserved_111:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_111_OFS 0x0218
REGDEF_BEGIN(VPE_RESERVED_111)
REGDEF_BIT(VPE_Reserved_111,        32)
REGDEF_END(VPE_RESERVED_111)


/*
    VPE_Reserved_112:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_112_OFS 0x021c
REGDEF_BEGIN(VPE_RESERVED_112)
REGDEF_BIT(VPE_Reserved_112,        32)
REGDEF_END(VPE_RESERVED_112)


/*
    VPE_Reserved_113:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_113_OFS 0x0220
REGDEF_BEGIN(VPE_RESERVED_113)
REGDEF_BIT(VPE_Reserved_113,        32)
REGDEF_END(VPE_RESERVED_113)


/*
    VPE_Reserved_114:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_114_OFS 0x0224
REGDEF_BEGIN(VPE_RESERVED_114)
REGDEF_BIT(VPE_Reserved_114,        32)
REGDEF_END(VPE_RESERVED_114)


/*
    VPE_Reserved_115:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_115_OFS 0x0228
REGDEF_BEGIN(VPE_RESERVED_115)
REGDEF_BIT(VPE_Reserved_115,        32)
REGDEF_END(VPE_RESERVED_115)


/*
    VPE_Reserved_116:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_116_OFS 0x022c
REGDEF_BEGIN(VPE_RESERVED_116)
REGDEF_BIT(VPE_Reserved_116,        32)
REGDEF_END(VPE_RESERVED_116)


/*
    VPE_Reserved_117:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_117_OFS 0x0230
REGDEF_BEGIN(VPE_RESERVED_117)
REGDEF_BIT(VPE_Reserved_117,        32)
REGDEF_END(VPE_RESERVED_117)


/*
    VPE_Reserved_118:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_118_OFS 0x0234
REGDEF_BEGIN(VPE_RESERVED_118)
REGDEF_BIT(VPE_Reserved_118,        32)
REGDEF_END(VPE_RESERVED_118)


/*
    VPE_Reserved_119:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_119_OFS 0x0238
REGDEF_BEGIN(VPE_RESERVED_119)
REGDEF_BIT(VPE_Reserved_119,        32)
REGDEF_END(VPE_RESERVED_119)


/*
    VPE_Reserved_120:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_120_OFS 0x023c
REGDEF_BEGIN(VPE_RESERVED_120)
REGDEF_BIT(VPE_Reserved_120,        32)
REGDEF_END(VPE_RESERVED_120)


/*
    EG_WET_SRC_SEL:    [0x0, 0x1],          bits : 0
    EG_WET_TH     :    [0x0, 0xff],         bits : 15_8
    EG_WET_GAIN   :    [0x0, 0xff],         bits : 23_16
    NOISE_LEVEL   :    [0x0, 0xff],         bits : 31_24
*/
#define SHARPEN_REGISTER1_OFS 0x0240
REGDEF_BEGIN(SHARPEN_REGISTER1)
REGDEF_BIT(EG_WET_SRC_SEL,        1)
REGDEF_BIT(,        7)
REGDEF_BIT(EG_WET_TH,        8)
REGDEF_BIT(EG_WET_GAIN,        8)
REGDEF_BIT(NOISE_LEVEL,        8)
REGDEF_END(SHARPEN_REGISTER1)


/*
    EG_SHP_STR1 :    [0x0, 0xff],           bits : 7_0
    EG_SHP_STR2 :    [0x0, 0xff],           bits : 15_8
    FLAT_SHP_STR:    [0x0, 0xff],           bits : 23_16
*/
#define SHARPEN_REGISTER2_OFS 0x0244
REGDEF_BEGIN(SHARPEN_REGISTER2)
REGDEF_BIT(EG_SHP_STR1,        8)
REGDEF_BIT(EG_SHP_STR2,        8)
REGDEF_BIT(FLAT_SHP_STR,        8)
REGDEF_END(SHARPEN_REGISTER2)


/*
    CORING_TH       :    [0x0, 0xff],           bits : 7_0
    BLEND_INV_GMA   :    [0x0, 0xff],           bits : 15_8
    BRIGHT_HALO_CLIP:    [0x0, 0xff],           bits : 23_16
    DARK_HALO_CLIP  :    [0x0, 0xff],           bits : 31_24
*/
#define SHARPEN_REGISTER3_OFS 0x0248
REGDEF_BEGIN(SHARPEN_REGISTER3)
REGDEF_BIT(CORING_TH,        8)
REGDEF_BIT(BLEND_INV_GMA,        8)
REGDEF_BIT(BRIGHT_HALO_CLIP,        8)
REGDEF_BIT(DARK_HALO_CLIP,        8)
REGDEF_END(SHARPEN_REGISTER3)


/*
    NOISE_CURVE0:    [0x0, 0xff],           bits : 7_0
    NOISE_CURVE1:    [0x0, 0xff],           bits : 15_8
    NOISE_CURVE2:    [0x0, 0xff],           bits : 23_16
    NOISE_CURVE3:    [0x0, 0xff],           bits : 31_24
*/
#define SHARPEN_REGISTER4_OFS 0x024c
REGDEF_BEGIN(SHARPEN_REGISTER4)
REGDEF_BIT(NOISE_CURVE0,        8)
REGDEF_BIT(NOISE_CURVE1,        8)
REGDEF_BIT(NOISE_CURVE2,        8)
REGDEF_BIT(NOISE_CURVE3,        8)
REGDEF_END(SHARPEN_REGISTER4)


/*
    NOISE_CURVE4:    [0x0, 0xff],           bits : 7_0
    NOISE_CURVE5:    [0x0, 0xff],           bits : 15_8
    NOISE_CURVE6:    [0x0, 0xff],           bits : 23_16
    NOISE_CURVE7:    [0x0, 0xff],           bits : 31_24
*/
#define SHARPEN_REGISTER5_OFS 0x0250
REGDEF_BEGIN(SHARPEN_REGISTER5)
REGDEF_BIT(NOISE_CURVE4,        8)
REGDEF_BIT(NOISE_CURVE5,        8)
REGDEF_BIT(NOISE_CURVE6,        8)
REGDEF_BIT(NOISE_CURVE7,        8)
REGDEF_END(SHARPEN_REGISTER5)


/*
    NOISE_CURVE8 :    [0x0, 0xff],          bits : 7_0
    NOISE_CURVE9 :    [0x0, 0xff],          bits : 15_8
    NOISE_CURVE10:    [0x0, 0xff],          bits : 23_16
    NOISE_CURVE11:    [0x0, 0xff],          bits : 31_24
*/
#define SHARPEN_REGISTER6_OFS 0x0254
REGDEF_BEGIN(SHARPEN_REGISTER6)
REGDEF_BIT(NOISE_CURVE8,        8)
REGDEF_BIT(NOISE_CURVE9,        8)
REGDEF_BIT(NOISE_CURVE10,        8)
REGDEF_BIT(NOISE_CURVE11,        8)
REGDEF_END(SHARPEN_REGISTER6)


/*
    NOISE_CURVE12:    [0x0, 0xff],          bits : 7_0
    NOISE_CURVE13:    [0x0, 0xff],          bits : 15_8
    NOISE_CURVE14:    [0x0, 0xff],          bits : 23_16
    NOISE_CURVE15:    [0x0, 0xff],          bits : 31_24
*/
#define SHARPEN_REGISTER7_OFS 0x0258
REGDEF_BEGIN(SHARPEN_REGISTER7)
REGDEF_BIT(NOISE_CURVE12,        8)
REGDEF_BIT(NOISE_CURVE13,        8)
REGDEF_BIT(NOISE_CURVE14,        8)
REGDEF_BIT(NOISE_CURVE15,        8)
REGDEF_END(SHARPEN_REGISTER7)


/*
    NOISE_CURVE16:    [0x0, 0xff],          bits : 7_0
*/
#define SHARPEN_REGISTER8_OFS 0x025c
REGDEF_BEGIN(SHARPEN_REGISTER8)
REGDEF_BIT(NOISE_CURVE16,        8)
REGDEF_END(SHARPEN_REGISTER8)


/*
    PAL0_Y :    [0x0, 0xff],            bits : 7_0
    PAL0_CB:    [0x0, 0xff],            bits : 15_8
    PAL0_CR:    [0x0, 0xff],            bits : 23_16
*/
#define PALETTE_REGISTER1_OFS 0x0260
REGDEF_BEGIN(PALETTE_REGISTER1)
REGDEF_BIT(PAL0_Y,        8)
REGDEF_BIT(PAL0_CB,        8)
REGDEF_BIT(PAL0_CR,        8)
REGDEF_END(PALETTE_REGISTER1)


/*
    PAL1_Y :    [0x0, 0xff],            bits : 7_0
    PAL1_CB:    [0x0, 0xff],            bits : 15_8
    PAL1_CR:    [0x0, 0xff],            bits : 23_16
*/
#define PALETTE_REGISTER2_OFS 0x0264
REGDEF_BEGIN(PALETTE_REGISTER2)
REGDEF_BIT(PAL1_Y,        8)
REGDEF_BIT(PAL1_CB,        8)
REGDEF_BIT(PAL1_CR,        8)
REGDEF_END(PALETTE_REGISTER2)


/*
    PAL2_Y :    [0x0, 0xff],            bits : 7_0
    PAL2_CB:    [0x0, 0xff],            bits : 15_8
    PAL2_CR:    [0x0, 0xff],            bits : 23_16
*/
#define PALETTE_REGISTER3_OFS 0x0268
REGDEF_BEGIN(PALETTE_REGISTER3)
REGDEF_BIT(PAL2_Y,        8)
REGDEF_BIT(PAL2_CB,        8)
REGDEF_BIT(PAL2_CR,        8)
REGDEF_END(PALETTE_REGISTER3)


/*
    PAL3_Y :    [0x0, 0xff],            bits : 7_0
    PAL3_CB:    [0x0, 0xff],            bits : 15_8
    PAL3_CR:    [0x0, 0xff],            bits : 23_16
*/
#define PALETTE_REGISTER4_OFS 0x026c
REGDEF_BEGIN(PALETTE_REGISTER4)
REGDEF_BIT(PAL3_Y,        8)
REGDEF_BIT(PAL3_CB,        8)
REGDEF_BIT(PAL3_CR,        8)
REGDEF_END(PALETTE_REGISTER4)


/*
    PAL4_Y :    [0x0, 0xff],            bits : 7_0
    PAL4_CB:    [0x0, 0xff],            bits : 15_8
    PAL4_CR:    [0x0, 0xff],            bits : 23_16
*/
#define PALETTE_REGISTER5_OFS 0x0270
REGDEF_BEGIN(PALETTE_REGISTER5)
REGDEF_BIT(PAL4_Y,        8)
REGDEF_BIT(PAL4_CB,        8)
REGDEF_BIT(PAL4_CR,        8)
REGDEF_END(PALETTE_REGISTER5)


/*
    PAL5_Y :    [0x0, 0xff],            bits : 7_0
    PAL5_CB:    [0x0, 0xff],            bits : 15_8
    PAL5_CR:    [0x0, 0xff],            bits : 23_16
*/
#define PALETTE_REGISTER6_OFS 0x0274
REGDEF_BEGIN(PALETTE_REGISTER6)
REGDEF_BIT(PAL5_Y,        8)
REGDEF_BIT(PAL5_CB,        8)
REGDEF_BIT(PAL5_CR,        8)
REGDEF_END(PALETTE_REGISTER6)


/*
    PAL6_Y :    [0x0, 0xff],            bits : 7_0
    PAL6_CB:    [0x0, 0xff],            bits : 15_8
    PAL6_CR:    [0x0, 0xff],            bits : 23_16
*/
#define PALETTE_REGISTER7_OFS 0x0278
REGDEF_BEGIN(PALETTE_REGISTER7)
REGDEF_BIT(PAL6_Y,        8)
REGDEF_BIT(PAL6_CB,        8)
REGDEF_BIT(PAL6_CR,        8)
REGDEF_END(PALETTE_REGISTER7)


/*
    PAL7_Y :    [0x0, 0xff],            bits : 7_0
    PAL7_CB:    [0x0, 0xff],            bits : 15_8
    PAL7_CR:    [0x0, 0xff],            bits : 23_16
*/
#define PALETTE_REGISTER8_OFS 0x027c
REGDEF_BEGIN(PALETTE_REGISTER8)
REGDEF_BIT(PAL7_Y,        8)
REGDEF_BIT(PAL7_CB,        8)
REGDEF_BIT(PAL7_CR,        8)
REGDEF_END(PALETTE_REGISTER8)


/*
    VPE_Reserved_121:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_121_OFS 0x0280
REGDEF_BEGIN(VPE_RESERVED_121)
REGDEF_BIT(VPE_Reserved_121,        32)
REGDEF_END(VPE_RESERVED_121)


/*
    VPE_Reserved_122:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_122_OFS 0x0284
REGDEF_BEGIN(VPE_RESERVED_122)
REGDEF_BIT(VPE_Reserved_122,        32)
REGDEF_END(VPE_RESERVED_122)


/*
    VPE_Reserved_123:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_123_OFS 0x0288
REGDEF_BEGIN(VPE_RESERVED_123)
REGDEF_BIT(VPE_Reserved_123,        32)
REGDEF_END(VPE_RESERVED_123)


/*
    VPE_Reserved_124:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_124_OFS 0x028c
REGDEF_BEGIN(VPE_RESERVED_124)
REGDEF_BIT(VPE_Reserved_124,        32)
REGDEF_END(VPE_RESERVED_124)


/*
    VPE_Reserved_125:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_125_OFS 0x0290
REGDEF_BEGIN(VPE_RESERVED_125)
REGDEF_BIT(VPE_Reserved_125,        32)
REGDEF_END(VPE_RESERVED_125)


/*
    VPE_Reserved_126:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_126_OFS 0x0294
REGDEF_BEGIN(VPE_RESERVED_126)
REGDEF_BIT(VPE_Reserved_126,        32)
REGDEF_END(VPE_RESERVED_126)


/*
    VPE_Reserved_127:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_127_OFS 0x0298
REGDEF_BEGIN(VPE_RESERVED_127)
REGDEF_BIT(VPE_Reserved_127,        32)
REGDEF_END(VPE_RESERVED_127)


/*
    VPE_Reserved_128:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_128_OFS 0x029c
REGDEF_BEGIN(VPE_RESERVED_128)
REGDEF_BIT(VPE_Reserved_128,        32)
REGDEF_END(VPE_RESERVED_128)


/*
    VPE_Reserved_129:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_129_OFS 0x02a0
REGDEF_BEGIN(VPE_RESERVED_129)
REGDEF_BIT(VPE_Reserved_129,        32)
REGDEF_END(VPE_RESERVED_129)


/*
    VPE_Reserved_130:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_130_OFS 0x02a4
REGDEF_BEGIN(VPE_RESERVED_130)
REGDEF_BIT(VPE_Reserved_130,        32)
REGDEF_END(VPE_RESERVED_130)


/*
    VPE_Reserved_131:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_131_OFS 0x02a8
REGDEF_BEGIN(VPE_RESERVED_131)
REGDEF_BIT(VPE_Reserved_131,        32)
REGDEF_END(VPE_RESERVED_131)


/*
    VPE_Reserved_132:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_132_OFS 0x02ac
REGDEF_BEGIN(VPE_RESERVED_132)
REGDEF_BIT(VPE_Reserved_132,        32)
REGDEF_END(VPE_RESERVED_132)


/*
    VPE_Reserved_133:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_133_OFS 0x02b0
REGDEF_BEGIN(VPE_RESERVED_133)
REGDEF_BIT(VPE_Reserved_133,        32)
REGDEF_END(VPE_RESERVED_133)


/*
    VPE_Reserved_134:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_134_OFS 0x02b4
REGDEF_BEGIN(VPE_RESERVED_134)
REGDEF_BIT(VPE_Reserved_134,        32)
REGDEF_END(VPE_RESERVED_134)


/*
    VPE_Reserved_135:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_135_OFS 0x02b8
REGDEF_BEGIN(VPE_RESERVED_135)
REGDEF_BIT(VPE_Reserved_135,        32)
REGDEF_END(VPE_RESERVED_135)


/*
    VPE_Reserved_136:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_136_OFS 0x02bc
REGDEF_BEGIN(VPE_RESERVED_136)
REGDEF_BIT(VPE_Reserved_136,        32)
REGDEF_END(VPE_RESERVED_136)


/*
    VPE_Reserved_137:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_137_OFS 0x02c0
REGDEF_BEGIN(VPE_RESERVED_137)
REGDEF_BIT(VPE_Reserved_137,        32)
REGDEF_END(VPE_RESERVED_137)


/*
    VPE_Reserved_138:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_138_OFS 0x02c4
REGDEF_BEGIN(VPE_RESERVED_138)
REGDEF_BIT(VPE_Reserved_138,        32)
REGDEF_END(VPE_RESERVED_138)


/*
    VPE_Reserved_139:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_139_OFS 0x02c8
REGDEF_BEGIN(VPE_RESERVED_139)
REGDEF_BIT(VPE_Reserved_139,        32)
REGDEF_END(VPE_RESERVED_139)


/*
    VPE_Reserved_140:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_140_OFS 0x02cc
REGDEF_BEGIN(VPE_RESERVED_140)
REGDEF_BIT(VPE_Reserved_140,        32)
REGDEF_END(VPE_RESERVED_140)


/*
    VPE_Reserved_141:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_141_OFS 0x02d0
REGDEF_BEGIN(VPE_RESERVED_141)
REGDEF_BIT(VPE_Reserved_141,        32)
REGDEF_END(VPE_RESERVED_141)


/*
    VPE_Reserved_142:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_142_OFS 0x02d4
REGDEF_BEGIN(VPE_RESERVED_142)
REGDEF_BIT(VPE_Reserved_142,        32)
REGDEF_END(VPE_RESERVED_142)


/*
    VPE_Reserved_143:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_143_OFS 0x02d8
REGDEF_BEGIN(VPE_RESERVED_143)
REGDEF_BIT(VPE_Reserved_143,        32)
REGDEF_END(VPE_RESERVED_143)


/*
    VPE_Reserved_144:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_144_OFS 0x02dc
REGDEF_BEGIN(VPE_RESERVED_144)
REGDEF_BIT(VPE_Reserved_144,        32)
REGDEF_END(VPE_RESERVED_144)


/*
    VPE_Reserved_145:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_145_OFS 0x02e0
REGDEF_BEGIN(VPE_RESERVED_145)
REGDEF_BIT(VPE_Reserved_145,        32)
REGDEF_END(VPE_RESERVED_145)


/*
    VPE_Reserved_146:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_146_OFS 0x02e4
REGDEF_BEGIN(VPE_RESERVED_146)
REGDEF_BIT(VPE_Reserved_146,        32)
REGDEF_END(VPE_RESERVED_146)


/*
    VPE_Reserved_147:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_147_OFS 0x02e8
REGDEF_BEGIN(VPE_RESERVED_147)
REGDEF_BIT(VPE_Reserved_147,        32)
REGDEF_END(VPE_RESERVED_147)


/*
    VPE_Reserved_148:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_148_OFS 0x02ec
REGDEF_BEGIN(VPE_RESERVED_148)
REGDEF_BIT(VPE_Reserved_148,        32)
REGDEF_END(VPE_RESERVED_148)


/*
    VPE_Reserved_149:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_149_OFS 0x02f0
REGDEF_BEGIN(VPE_RESERVED_149)
REGDEF_BIT(VPE_Reserved_149,        32)
REGDEF_END(VPE_RESERVED_149)


/*
    VPE_Reserved_150:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_150_OFS 0x02f4
REGDEF_BEGIN(VPE_RESERVED_150)
REGDEF_BIT(VPE_Reserved_150,        32)
REGDEF_END(VPE_RESERVED_150)


/*
    VPE_Reserved_151:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_151_OFS 0x02f8
REGDEF_BEGIN(VPE_RESERVED_151)
REGDEF_BIT(VPE_Reserved_151,        32)
REGDEF_END(VPE_RESERVED_151)


/*
    VPE_Reserved_152:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_152_OFS 0x02fc
REGDEF_BEGIN(VPE_RESERVED_152)
REGDEF_BIT(VPE_Reserved_152,        32)
REGDEF_END(VPE_RESERVED_152)


/*
    LLC_ADDR:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DMA_1_REGISTER_OFS 0x0300
REGDEF_BEGIN(DMA_1_REGISTER)
REGDEF_BIT(LLC_ADDR,        32)
REGDEF_END(DMA_1_REGISTER)


/*
    SRC_ADDR:    [0x0, 0xffffffff],         bits : 31_0
*/
#define DMA_2_REGISTER_OFS 0x0304
REGDEF_BEGIN(DMA_2_REGISTER)
REGDEF_BIT(SRC_ADDR,        32)
REGDEF_END(DMA_2_REGISTER)


/*
    VPE_Reserved_153:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_153_OFS 0x0308
REGDEF_BEGIN(VPE_RESERVED_153)
REGDEF_BIT(VPE_Reserved_153,        32)
REGDEF_END(VPE_RESERVED_153)


/*
    VPE_Reserved_154:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_154_OFS 0x030c
REGDEF_BEGIN(VPE_RESERVED_154)
REGDEF_BIT(VPE_Reserved_154,        32)
REGDEF_END(VPE_RESERVED_154)


/*
    VPE_Reserved_155:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_155_OFS 0x0310
REGDEF_BEGIN(VPE_RESERVED_155)
REGDEF_BIT(VPE_Reserved_155,        32)
REGDEF_END(VPE_RESERVED_155)


/*
    VPE_Reserved_156:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_156_OFS 0x0314
REGDEF_BEGIN(VPE_RESERVED_156)
REGDEF_BIT(VPE_Reserved_156,        32)
REGDEF_END(VPE_RESERVED_156)


/*
    VPE_Reserved_157:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_157_OFS 0x0318
REGDEF_BEGIN(VPE_RESERVED_157)
REGDEF_BIT(VPE_Reserved_157,        32)
REGDEF_END(VPE_RESERVED_157)


/*
    DCTG_GAMMA_ADDR:    [0x0, 0xffffffff],          bits : 31_0
*/
#define DMA_8_REGISTER_OFS 0x031c
REGDEF_BEGIN(DMA_8_REGISTER)
REGDEF_BIT(DCTG_GAMMA_ADDR,        32)
REGDEF_END(DMA_8_REGISTER)


/*
    DCE_2DLUT_ADDR:    [0x0, 0xffffffff],           bits : 31_0
*/
#define DMA_9_REGISTER_OFS 0x0320
REGDEF_BEGIN(DMA_9_REGISTER)
REGDEF_BIT(DCE_2DLUT_ADDR,        32)
REGDEF_END(DMA_9_REGISTER)


/*
    VPE_Reserved_158:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_158_OFS 0x0324
REGDEF_BEGIN(VPE_RESERVED_158)
REGDEF_BIT(VPE_Reserved_158,        32)
REGDEF_END(VPE_RESERVED_158)


/*
    VPE_Reserved_159:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_159_OFS 0x0328
REGDEF_BEGIN(VPE_RESERVED_159)
REGDEF_BIT(VPE_Reserved_159,        32)
REGDEF_END(VPE_RESERVED_159)


/*
    VPE_Reserved_160:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_160_OFS 0x032c
REGDEF_BEGIN(VPE_RESERVED_160)
REGDEF_BIT(VPE_Reserved_160,        32)
REGDEF_END(VPE_RESERVED_160)


/*
    VPE_Reserved_161:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_161_OFS 0x0330
REGDEF_BEGIN(VPE_RESERVED_161)
REGDEF_BIT(VPE_Reserved_161,        32)
REGDEF_END(VPE_RESERVED_161)


/*
    VPE_Reserved_162:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_162_OFS 0x0334
REGDEF_BEGIN(VPE_RESERVED_162)
REGDEF_BIT(VPE_Reserved_162,        32)
REGDEF_END(VPE_RESERVED_162)


/*
    VPE_Reserved_163:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_163_OFS 0x0338
REGDEF_BEGIN(VPE_RESERVED_163)
REGDEF_BIT(VPE_Reserved_163,        32)
REGDEF_END(VPE_RESERVED_163)


/*
    VPE_Reserved_164:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_164_OFS 0x033c
REGDEF_BEGIN(VPE_RESERVED_164)
REGDEF_BIT(VPE_Reserved_164,        32)
REGDEF_END(VPE_RESERVED_164)


/*
    DES_RES0_DP0_ADDR:    [0x0, 0xffffffff],            bits : 31_0
*/
#define DMA_16_REGISTER_OFS 0x0340
REGDEF_BEGIN(DMA_16_REGISTER)
REGDEF_BIT(DES_RES0_DP0_ADDR,        32)
REGDEF_END(DMA_16_REGISTER)


/*
    VPE_Reserved_165:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_165_OFS 0x0344
REGDEF_BEGIN(VPE_RESERVED_165)
REGDEF_BIT(VPE_Reserved_165,        32)
REGDEF_END(VPE_RESERVED_165)


#define VPE_RESERVED_9998_OFS 0x0348
REGDEF_BEGIN(VPE_RESERVED_9998)
REGDEF_BIT(RESERVED,        32)
REGDEF_END(VPE_RESERVED_9998) //0x0348


/*
    DES_RES1_DP0_ADDR:    [0x0, 0xffffffff],            bits : 31_0
*/
#define DMA_19_REGISTER_OFS 0x034c
REGDEF_BEGIN(DMA_19_REGISTER)
REGDEF_BIT(DES_RES1_DP0_ADDR,        32)
REGDEF_END(DMA_19_REGISTER)


/*
    VPE_Reserved_166:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_166_OFS 0x0350
REGDEF_BEGIN(VPE_RESERVED_166)
REGDEF_BIT(VPE_Reserved_166,        32)
REGDEF_END(VPE_RESERVED_166)


/*
    VPE_Reserved_167:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_167_OFS 0x0354
REGDEF_BEGIN(VPE_RESERVED_167)
REGDEF_BIT(VPE_Reserved_167,        32)
REGDEF_END(VPE_RESERVED_167)


/*
    DES_RES2_DP0_ADDR:    [0x0, 0xffffffff],            bits : 31_0
*/
#define DMA_22_REGISTER_OFS 0x0358
REGDEF_BEGIN(DMA_22_REGISTER)
REGDEF_BIT(DES_RES2_DP0_ADDR,        32)
REGDEF_END(DMA_22_REGISTER)


/*
    VPE_Reserved_168:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_168_OFS 0x035c
REGDEF_BEGIN(VPE_RESERVED_168)
REGDEF_BIT(VPE_Reserved_168,        32)
REGDEF_END(VPE_RESERVED_168)


/*
    VPE_Reserved_563:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_563_OFS 0x0360
REGDEF_BEGIN(VPE_RESERVED_563)
REGDEF_BIT(VPE_Reserved_563,        32)
REGDEF_END(VPE_RESERVED_563)


/*
    DES_RES3_DP0_ADDR:    [0x0, 0xffffffff],            bits : 31_0
*/
#define DMA_25_REGISTER_OFS 0x0364
REGDEF_BEGIN(DMA_25_REGISTER)
REGDEF_BIT(DES_RES3_DP0_ADDR,        32)
REGDEF_END(DMA_25_REGISTER)


/*
    VPE_Reserved_169:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_169_OFS 0x0368
REGDEF_BEGIN(VPE_RESERVED_169)
REGDEF_BIT(VPE_Reserved_169,        32)
REGDEF_END(VPE_RESERVED_169)


/*
    VPE_Reserved_170:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_170_OFS 0x036c
REGDEF_BEGIN(VPE_RESERVED_170)
REGDEF_BIT(VPE_Reserved_170,        32)
REGDEF_END(VPE_RESERVED_170)


/*
    VPE_Reserved_171:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_171_OFS 0x0370
REGDEF_BEGIN(VPE_RESERVED_171)
REGDEF_BIT(VPE_Reserved_171,        32)
REGDEF_END(VPE_RESERVED_171)


/*
    VPE_Reserved_172:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_172_OFS 0x0374
REGDEF_BEGIN(VPE_RESERVED_172)
REGDEF_BIT(VPE_Reserved_172,        32)
REGDEF_END(VPE_RESERVED_172)


/*
    VPE_Reserved_173:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_173_OFS 0x0378
REGDEF_BEGIN(VPE_RESERVED_173)
REGDEF_BIT(VPE_Reserved_173,        32)
REGDEF_END(VPE_RESERVED_173)


/*
    DMA0_WCMD_WAIT_VALUE:    [0x0, 0xffff],         bits : 15_0
    DMA0_RCMD_WAIT_VALUE:    [0x0, 0xffff],         bits : 31_16
*/
#define DMA_31_REGISTER_OFS 0x037c
REGDEF_BEGIN(DMA_31_REGISTER)
REGDEF_BIT(DMA0_WCMD_WAIT_VALUE,        16)
REGDEF_BIT(DMA0_RCMD_WAIT_VALUE,        16)
REGDEF_END(DMA_31_REGISTER)


/*
    DMA1_WCMD_WAIT_VALUE:    [0x0, 0xffff],         bits : 15_0
    DMA1_RCMD_WAIT_VALUE:    [0x0, 0xffff],         bits : 31_16
*/
#define DMA_32_REGISTER_OFS 0x0380
REGDEF_BEGIN(DMA_32_REGISTER)
REGDEF_BIT(DMA1_WCMD_WAIT_VALUE,        16)
REGDEF_BIT(DMA1_RCMD_WAIT_VALUE,        16)
REGDEF_END(DMA_32_REGISTER)


/*
    VPE_Reserved_174:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_174_OFS 0x0384
REGDEF_BEGIN(VPE_RESERVED_174)
REGDEF_BIT(VPE_Reserved_174,        32)
REGDEF_END(VPE_RESERVED_174)


/*
    VPE_Reserved_175:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_175_OFS 0x0388
REGDEF_BEGIN(VPE_RESERVED_175)
REGDEF_BIT(VPE_Reserved_175,        32)
REGDEF_END(VPE_RESERVED_175)


/*
    VPE_Reserved_176:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_176_OFS 0x038c
REGDEF_BEGIN(VPE_RESERVED_176)
REGDEF_BIT(VPE_Reserved_176,        32)
REGDEF_END(VPE_RESERVED_176)


/*
    VPE_Reserved_177:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_177_OFS 0x0390
REGDEF_BEGIN(VPE_RESERVED_177)
REGDEF_BIT(VPE_Reserved_177,        32)
REGDEF_END(VPE_RESERVED_177)


/*
    VPE_Reserved_178:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_178_OFS 0x0394
REGDEF_BEGIN(VPE_RESERVED_178)
REGDEF_BIT(VPE_Reserved_178,        32)
REGDEF_END(VPE_RESERVED_178)


/*
    VPE_Reserved_179:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_179_OFS 0x0398
REGDEF_BEGIN(VPE_RESERVED_179)
REGDEF_BIT(VPE_Reserved_179,        32)
REGDEF_END(VPE_RESERVED_179)


/*
    VPE_Reserved_180:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_180_OFS 0x039c
REGDEF_BEGIN(VPE_RESERVED_180)
REGDEF_BIT(VPE_Reserved_180,        32)
REGDEF_END(VPE_RESERVED_180)


/*
    VPE_Reserved_181:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_181_OFS 0x03a0
REGDEF_BEGIN(VPE_RESERVED_181)
REGDEF_BIT(VPE_Reserved_181,        32)
REGDEF_END(VPE_RESERVED_181)


/*
    VPE_Reserved_182:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_182_OFS 0x03a4
REGDEF_BEGIN(VPE_RESERVED_182)
REGDEF_BIT(VPE_Reserved_182,        32)
REGDEF_END(VPE_RESERVED_182)


/*
    VPE_Reserved_183:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_183_OFS 0x03a8
REGDEF_BEGIN(VPE_RESERVED_183)
REGDEF_BIT(VPE_Reserved_183,        32)
REGDEF_END(VPE_RESERVED_183)


/*
    VPE_Reserved_184:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_184_OFS 0x03ac
REGDEF_BEGIN(VPE_RESERVED_184)
REGDEF_BIT(VPE_Reserved_184,        32)
REGDEF_END(VPE_RESERVED_184)


/*
    VPE_Reserved_185:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_185_OFS 0x03b0
REGDEF_BEGIN(VPE_RESERVED_185)
REGDEF_BIT(VPE_Reserved_185,        32)
REGDEF_END(VPE_RESERVED_185)


/*
    VPE_Reserved_186:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_186_OFS 0x03b4
REGDEF_BEGIN(VPE_RESERVED_186)
REGDEF_BIT(VPE_Reserved_186,        32)
REGDEF_END(VPE_RESERVED_186)


/*
    VPE_Reserved_187:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_187_OFS 0x03b8
REGDEF_BEGIN(VPE_RESERVED_187)
REGDEF_BIT(VPE_Reserved_187,        32)
REGDEF_END(VPE_RESERVED_187)


/*
    VPE_Reserved_188:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_188_OFS 0x03bc
REGDEF_BEGIN(VPE_RESERVED_188)
REGDEF_BIT(VPE_Reserved_188,        32)
REGDEF_END(VPE_RESERVED_188)


/*
    SHP_PPI_IDX:    [0x0, 0xf],         bits : 3_0
    SHP_PPO_IDX:    [0x0, 0xf],         bits : 7_4
*/
#define PIPE_REGISTER1_OFS 0x03c0
REGDEF_BEGIN(PIPE_REGISTER1)
REGDEF_BIT(SHP_PPI_IDX,        4)
REGDEF_BIT(SHP_PPO_IDX,        4)
REGDEF_END(PIPE_REGISTER1)


/*
    SRC_PPO_IDX:    [0x0, 0xf],         bits : 3_0
    SCA_PPI_IDX:    [0x0, 0xf],         bits : 7_4
*/
#define PIPE_REGISTER2_OFS 0x03c4
REGDEF_BEGIN(PIPE_REGISTER2)
REGDEF_BIT(SRC_PPO_IDX,        4)
REGDEF_BIT(SCA_PPI_IDX,        4)
REGDEF_END(PIPE_REGISTER2)


/*
    VPE_Reserved_189:    [0x0, 0x1fffffff],         bits : 31_3
*/
#define VPE_RESERVED_189_OFS 0x03c8
REGDEF_BEGIN(VPE_RESERVED_189)
REGDEF_BIT(,        3)
REGDEF_BIT(VPE_Reserved_189,        29)
REGDEF_END(VPE_RESERVED_189)


/*
    VPE_Reserved_190:    [0x0, 0x1fffffff],         bits : 31_3
*/
#define VPE_RESERVED_190_OFS 0x03cc
REGDEF_BEGIN(VPE_RESERVED_190)
REGDEF_BIT(,        3)
REGDEF_BIT(VPE_Reserved_190,        29)
REGDEF_END(VPE_RESERVED_190)


/*
    VPE_Reserved_191:    [0x0, 0x1fffffff],         bits : 31_3
*/
#define VPE_RESERVED_191_OFS 0x03d0
REGDEF_BEGIN(VPE_RESERVED_191)
REGDEF_BIT(,        3)
REGDEF_BIT(VPE_Reserved_191,        29)
REGDEF_END(VPE_RESERVED_191)


/*
    VPE_Reserved_192:    [0x0, 0x1fffffff],         bits : 31_3
*/
#define VPE_RESERVED_192_OFS 0x03d4
REGDEF_BEGIN(VPE_RESERVED_192)
REGDEF_BIT(,        3)
REGDEF_BIT(VPE_Reserved_192,        29)
REGDEF_END(VPE_RESERVED_192)


/*
    VPE_Reserved_193:    [0x0, 0x1fffffff],         bits : 31_3
*/
#define VPE_RESERVED_193_OFS 0x03d8
REGDEF_BEGIN(VPE_RESERVED_193)
REGDEF_BIT(,        3)
REGDEF_BIT(VPE_Reserved_193,        29)
REGDEF_END(VPE_RESERVED_193)


/*
    VPE_Reserved_194:    [0x0, 0x1fffffff],         bits : 31_3
*/
#define VPE_RESERVED_194_OFS 0x03dc
REGDEF_BEGIN(VPE_RESERVED_194)
REGDEF_BIT(,        3)
REGDEF_BIT(VPE_Reserved_194,        29)
REGDEF_END(VPE_RESERVED_194)


/*
    DCTG_MOUNT_TYPE:    [0x0, 0x1],         bits : 0
    DCTG_RA_EN     :    [0x0, 0x1],         bits : 1
    DCTG_LUT2D_SZ  :    [0x0, 0x3],         bits : 3_2
    DCTG_LENS_R    :    [0x0, 0x3fff],          bits : 29_16
*/
#define DCTG_1_REGISTER_OFS 0x03e0
REGDEF_BEGIN(DCTG_1_REGISTER)
REGDEF_BIT(DCTG_MOUNT_TYPE,        1)
REGDEF_BIT(DCTG_RA_EN,        1)
REGDEF_BIT(DCTG_LUT2D_SZ,        2)
REGDEF_BIT(,        12)
REGDEF_BIT(DCTG_LENS_R,        14)
REGDEF_END(DCTG_1_REGISTER)


/*
    DCTG_LENS_X_ST:    [0x0, 0x1fff],           bits : 12_0
    DCTG_LENS_Y_ST:    [0x0, 0x1fff],           bits : 28_16
*/
#define DCTG_2_REGISTER_OFS 0x03e4
REGDEF_BEGIN(DCTG_2_REGISTER)
REGDEF_BIT(DCTG_LENS_X_ST,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCTG_LENS_Y_ST,        13)
REGDEF_END(DCTG_2_REGISTER)


/*
    DCTG_THETA_ST:    [0x0, 0xfffff],           bits : 19_0
*/
#define DCTG_3_REGISTER_OFS 0x03e8
REGDEF_BEGIN(DCTG_3_REGISTER)
REGDEF_BIT(DCTG_THETA_ST,        20)
REGDEF_END(DCTG_3_REGISTER)


/*
    DCTG_THETA_ED:    [0x0, 0xfffff],           bits : 19_0
*/
#define DCTG_4_REGISTER_OFS 0x03ec
REGDEF_BEGIN(DCTG_4_REGISTER)
REGDEF_BIT(DCTG_THETA_ED,        20)
REGDEF_END(DCTG_4_REGISTER)


/*
    DCTG_PHI_ST:    [0x0, 0xfffff],         bits : 19_0
*/
#define DCTG_5_REGISTER_OFS 0x03f0
REGDEF_BEGIN(DCTG_5_REGISTER)
REGDEF_BIT(DCTG_PHI_ST,        20)
REGDEF_END(DCTG_5_REGISTER)


/*
    DCTG_PHI_ED:    [0x0, 0xfffff],         bits : 19_0
*/
#define DCTG_6_REGISTER_OFS 0x03f4
REGDEF_BEGIN(DCTG_6_REGISTER)
REGDEF_BIT(DCTG_PHI_ED,        20)
REGDEF_END(DCTG_6_REGISTER)


/*
    DCTG_ROT_Z:    [0x0, 0xfffff],          bits : 19_0
*/
#define DCTG_7_REGISTER_OFS 0x03f8
REGDEF_BEGIN(DCTG_7_REGISTER)
REGDEF_BIT(DCTG_ROT_Z,        20)
REGDEF_END(DCTG_7_REGISTER)


/*
    DCTG_ROT_Y:    [0x0, 0xfffff],          bits : 19_0
*/
#define DCTG_8_REGISTER_OFS 0x03fc
REGDEF_BEGIN(DCTG_8_REGISTER)
REGDEF_BIT(DCTG_ROT_Y,        20)
REGDEF_END(DCTG_8_REGISTER)


/*
    VPE_INTS_FRAME_DONE        :    [0x0, 0x1],         bits : 0
    VPE_INTS_LL_ERR            :    [0x0, 0x1],         bits : 1
    VPE_INTS_LL_DONE           :    [0x0, 0x1],         bits : 2
    VPE_INTS_RES0_NEAR_FRM_DONE:    [0x0, 0x1],         bits : 10
    VPE_INTS_RES1_NEAR_FRM_DONE:    [0x0, 0x1],         bits : 11
    VPE_INTS_RES2_NEAR_FRM_DONE:    [0x0, 0x1],         bits : 12
    VPE_INTS_RES3_NEAR_FRM_DONE:    [0x0, 0x1],         bits : 13
*/
#define VPE_INTERRUPT_STATUS_1_REGISTER_OFS 0x0400
REGDEF_BEGIN(VPE_INTERRUPT_STATUS_1_REGISTER)
REGDEF_BIT(VPE_INTS_FRAME_DONE,        1)
REGDEF_BIT(VPE_INTS_LL_ERR,        1)
REGDEF_BIT(VPE_INTS_LL_DONE,        1)
REGDEF_BIT(,        7)
REGDEF_BIT(VPE_INTS_RES0_NEAR_FRM_DONE,        1)
REGDEF_BIT(VPE_INTS_RES1_NEAR_FRM_DONE,        1)
REGDEF_BIT(VPE_INTS_RES2_NEAR_FRM_DONE,        1)
REGDEF_BIT(VPE_INTS_RES3_NEAR_FRM_DONE,        1)
REGDEF_END(VPE_INTERRUPT_STATUS_1_REGISTER)


/*
    VPE_INTE_FRAME_DONE        :    [0x0, 0x1],         bits : 0
    VPE_INTE_LL_ERR            :    [0x0, 0x1],         bits : 1
    VPE_INTE_LL_DONE           :    [0x0, 0x1],         bits : 2
    VPE_INTE_RES0_NEAR_FRM_DONE:    [0x0, 0x1],         bits : 10
    VPE_INTE_RES1_NEAR_FRM_DONE:    [0x0, 0x1],         bits : 11
    VPE_INTE_RES2_NEAR_FRM_DONE:    [0x0, 0x1],         bits : 12
    VPE_INTE_RES3_NEAR_FRM_DONE:    [0x0, 0x1],         bits : 13
*/
#define VPE_INTERRUPT_ENABLE_2_REGISTER_OFS 0x0404
REGDEF_BEGIN(VPE_INTERRUPT_ENABLE_2_REGISTER)
REGDEF_BIT(VPE_INTE_FRAME_DONE,        1)
REGDEF_BIT(VPE_INTE_LL_ERR,        1)
REGDEF_BIT(VPE_INTE_LL_DONE,        1)
REGDEF_BIT(,        7)
REGDEF_BIT(VPE_INTE_RES0_NEAR_FRM_DONE,        1)
REGDEF_BIT(VPE_INTE_RES1_NEAR_FRM_DONE,        1)
REGDEF_BIT(VPE_INTE_RES2_NEAR_FRM_DONE,        1)
REGDEF_BIT(VPE_INTE_RES3_NEAR_FRM_DONE,        1)
REGDEF_END(VPE_INTERRUPT_ENABLE_2_REGISTER)


/*
    RES0_NEAR_FRM_DONE_TH:    [0x0, 0xffff],            bits : 15_0
    RES1_NEAR_FRM_DONE_TH:    [0x0, 0xffff],            bits : 31_16
*/
#define STATUS_1_REGISTER_OFS 0x0408
REGDEF_BEGIN(STATUS_1_REGISTER)
REGDEF_BIT(RES0_NEAR_FRM_DONE_TH,        16)
REGDEF_BIT(RES1_NEAR_FRM_DONE_TH,        16)
REGDEF_END(STATUS_1_REGISTER)


/*
    RES2_NEAR_FRM_DONE_TH:    [0x0, 0xffff],            bits : 15_0
    RES3_NEAR_FRM_DONE_TH:    [0x0, 0xffff],            bits : 31_16
*/
#define STATUS_2_REGISTER_OFS 0x040c
REGDEF_BEGIN(STATUS_2_REGISTER)
REGDEF_BIT(RES2_NEAR_FRM_DONE_TH,        16)
REGDEF_BIT(RES3_NEAR_FRM_DONE_TH,        16)
REGDEF_END(STATUS_2_REGISTER)


/*
    VPE_MODE           :    [0x0, 0x1],         bits : 0
    DMA0_SHUT_DOWN_DONE:    [0x0, 0x1],         bits : 1
    HW_IDLE            :    [0x0, 0x1],         bits : 3
*/
#define STATUS_3_REGISTER_OFS 0x0410
REGDEF_BEGIN(STATUS_3_REGISTER)
REGDEF_BIT(VPE_MODE,        1)
REGDEF_BIT(DMA0_SHUT_DOWN_DONE,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(HW_IDLE,        1)
REGDEF_END(STATUS_3_REGISTER)


/*
    IP_VERSION:    [0x0, 0xffffffff],           bits : 31_0
*/
#define STATUS_4_REGISTER_OFS 0x0414
REGDEF_BEGIN(STATUS_4_REGISTER)
REGDEF_BIT(IP_VERSION,        32)
REGDEF_END(STATUS_4_REGISTER)


/*
    IP_INFO:    [0x0, 0xffffffff],          bits : 31_0
*/
#define STATUS_5_REGISTER_OFS 0x0418
REGDEF_BEGIN(STATUS_5_REGISTER)
REGDEF_BIT(IP_INFO,        32)
REGDEF_END(STATUS_5_REGISTER)


/*
    VPE_Reserved_195:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_195_OFS 0x041c
REGDEF_BEGIN(VPE_RESERVED_195)
REGDEF_BIT(VPE_Reserved_195,        32)
REGDEF_END(VPE_RESERVED_195)


/*
    VPE_Reserved_196:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_196_OFS 0x0420
REGDEF_BEGIN(VPE_RESERVED_196)
REGDEF_BIT(VPE_Reserved_196,        32)
REGDEF_END(VPE_RESERVED_196)


/*
    VPE_Reserved_197:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_197_OFS 0x0424
REGDEF_BEGIN(VPE_RESERVED_197)
REGDEF_BIT(VPE_Reserved_197,        32)
REGDEF_END(VPE_RESERVED_197)


/*
    VPE_Reserved_198:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_198_OFS 0x0428
REGDEF_BEGIN(VPE_RESERVED_198)
REGDEF_BIT(VPE_Reserved_198,        32)
REGDEF_END(VPE_RESERVED_198)


/*
    VPE_Reserved_199:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_199_OFS 0x042c
REGDEF_BEGIN(VPE_RESERVED_199)
REGDEF_BIT(VPE_Reserved_199,        32)
REGDEF_END(VPE_RESERVED_199)


/*
    FIRE_CNT      :    [0x0, 0xff],         bits : 7_0
    DONE_CNT      :    [0x0, 0xff],         bits : 15_8
    SYS_CS        :    [0x0, 0xf],          bits : 19_16
    DMA_R_SRC_DONE:    [0x0, 0x1],          bits : 20
    DMA_R_LUT_DONE:    [0x0, 0x1],          bits : 24
    DMA_W_DES_DONE:    [0x0, 0x1],          bits : 25
    COL_CNT       :    [0x0, 0x3],          bits : 31_30
*/
#define STATUS_7_REGISTER_OFS 0x0430
REGDEF_BEGIN(STATUS_7_REGISTER)
REGDEF_BIT(FIRE_CNT,        8)
REGDEF_BIT(DONE_CNT,        8)
REGDEF_BIT(SYS_CS,        4)
REGDEF_BIT(DMA_R_SRC_DONE,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(DMA_R_LUT_DONE,        1)
REGDEF_BIT(DMA_W_DES_DONE,        1)
REGDEF_BIT(,        4)
REGDEF_BIT(COL_CNT,        2)
REGDEF_END(STATUS_7_REGISTER)


/*
    VPE_Reserved_200:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_200_OFS 0x0434
REGDEF_BEGIN(VPE_RESERVED_200)
REGDEF_BIT(VPE_Reserved_200,        32)
REGDEF_END(VPE_RESERVED_200)


/*
    RES0_YVD       :    [0x0, 0x1],         bits : 0
    RES0_Y_BLOCK   :    [0x0, 0x1],         bits : 1
    RES0_Y_LINE_NUM:    [0x0, 0x7ff],           bits : 12_2
    RES0_CVD       :    [0x0, 0x1],         bits : 16
    RES0_C_BLOCK   :    [0x0, 0x1],         bits : 17
    RES0_C_LINE_NUM:    [0x0, 0x7ff],           bits : 28_18
*/
#define STATUS_9_REGISTER_OFS 0x0438
REGDEF_BEGIN(STATUS_9_REGISTER)
REGDEF_BIT(RES0_YVD,        1)
REGDEF_BIT(RES0_Y_BLOCK,        1)
REGDEF_BIT(RES0_Y_LINE_NUM,        11)
REGDEF_BIT(,        3)
REGDEF_BIT(RES0_CVD,        1)
REGDEF_BIT(RES0_C_BLOCK,        1)
REGDEF_BIT(RES0_C_LINE_NUM,        11)
REGDEF_END(STATUS_9_REGISTER)


/*
    RES1_YVD       :    [0x0, 0x1],         bits : 0
    RES1_Y_BLOCK   :    [0x0, 0x1],         bits : 1
    RES1_Y_LINE_NUM:    [0x0, 0x7ff],           bits : 12_2
    RES1_CVD       :    [0x0, 0x1],         bits : 16
    RES1_C_BLOCK   :    [0x0, 0x1],         bits : 17
    RES1_C_LINE_NUM:    [0x0, 0x7ff],           bits : 28_18
*/
#define STATUS_10_REGISTER_OFS 0x043c
REGDEF_BEGIN(STATUS_10_REGISTER)
REGDEF_BIT(RES1_YVD,        1)
REGDEF_BIT(RES1_Y_BLOCK,        1)
REGDEF_BIT(RES1_Y_LINE_NUM,        11)
REGDEF_BIT(,        3)
REGDEF_BIT(RES1_CVD,        1)
REGDEF_BIT(RES1_C_BLOCK,        1)
REGDEF_BIT(RES1_C_LINE_NUM,        11)
REGDEF_END(STATUS_10_REGISTER)


/*
    RES2_YVD       :    [0x0, 0x1],         bits : 0
    RES2_Y_BLOCK   :    [0x0, 0x1],         bits : 1
    RES2_Y_LINE_NUM:    [0x0, 0x7ff],           bits : 12_2
    RES2_CVD       :    [0x0, 0x1],         bits : 16
    RES2_C_BLOCK   :    [0x0, 0x1],         bits : 17
    RES2_C_LINE_NUM:    [0x0, 0x7ff],           bits : 28_18
*/
#define STATUS_11_REGISTER_OFS 0x0440
REGDEF_BEGIN(STATUS_11_REGISTER)
REGDEF_BIT(RES2_YVD,        1)
REGDEF_BIT(RES2_Y_BLOCK,        1)
REGDEF_BIT(RES2_Y_LINE_NUM,        11)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CVD,        1)
REGDEF_BIT(RES2_C_BLOCK,        1)
REGDEF_BIT(RES2_C_LINE_NUM,        11)
REGDEF_END(STATUS_11_REGISTER)


/*
    RES3_YVD       :    [0x0, 0x1],         bits : 0
    RES3_Y_BLOCK   :    [0x0, 0x1],         bits : 1
    RES3_Y_LINE_NUM:    [0x0, 0x7ff],           bits : 12_2
    RES3_CVD       :    [0x0, 0x1],         bits : 16
    RES3_C_BLOCK   :    [0x0, 0x1],         bits : 17
    RES3_C_LINE_NUM:    [0x0, 0x7ff],           bits : 28_18
*/
#define STATUS_12_REGISTER_OFS 0x0444
REGDEF_BEGIN(STATUS_12_REGISTER)
REGDEF_BIT(RES3_YVD,        1)
REGDEF_BIT(RES3_Y_BLOCK,        1)
REGDEF_BIT(RES3_Y_LINE_NUM,        11)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CVD,        1)
REGDEF_BIT(RES3_C_BLOCK,        1)
REGDEF_BIT(RES3_C_LINE_NUM,        11)
REGDEF_END(STATUS_12_REGISTER)


/*
    VPE_Reserved_201:    [0x0, 0x1fffffff],         bits : 31_3
*/
#define VPE_RESERVED_201_OFS 0x0448
REGDEF_BEGIN(VPE_RESERVED_201)
REGDEF_BIT(,        3)
REGDEF_BIT(VPE_Reserved_201,        29)
REGDEF_END(VPE_RESERVED_201)


/*
    WBUF_CNT    :    [0x0, 0x1f],           bits : 4_0
    DES0_FRM_END:    [0x0, 0x1],            bits : 8
    DES1_FRM_END:    [0x0, 0x1],            bits : 9
    DES2_FRM_END:    [0x0, 0x1],            bits : 10
    DES3_FRM_END:    [0x0, 0x1],            bits : 11
*/
#define STATUS_14_REGISTER_OFS 0x044c
REGDEF_BEGIN(STATUS_14_REGISTER)
REGDEF_BIT(WBUF_CNT,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(DES0_FRM_END,        1)
REGDEF_BIT(DES1_FRM_END,        1)
REGDEF_BIT(DES2_FRM_END,        1)
REGDEF_BIT(DES3_FRM_END,        1)
REGDEF_END(STATUS_14_REGISTER)


/*
    MISS_CNT:    [0x0, 0xffffffff],         bits : 31_0
*/
#define V_CACHE_MISS_COUNT_REGISTER_OFS 0x0450
REGDEF_BEGIN(V_CACHE_MISS_COUNT_REGISTER)
REGDEF_BIT(MISS_CNT,        32)
REGDEF_END(V_CACHE_MISS_COUNT_REGISTER)


/*
    CONFLICT_CNT:    [0x0, 0xffffffff],         bits : 31_0
*/
#define V_CACHE_CONFLICT_COUNT_REGISTER_OFS 0x0454
REGDEF_BEGIN(V_CACHE_CONFLICT_COUNT_REGISTER)
REGDEF_BIT(CONFLICT_CNT,        32)
REGDEF_END(V_CACHE_CONFLICT_COUNT_REGISTER)


/*
    VC_CS  :    [0x0, 0x7],         bits : 2_0
    MISS_CS:    [0x0, 0x7],         bits : 10_8
*/
#define STATUS_17_REGISTER_OFS 0x0458
REGDEF_BEGIN(STATUS_17_REGISTER)
REGDEF_BIT(VC_CS,        3)
REGDEF_BIT(,        5)
REGDEF_BIT(MISS_CS,        3)
REGDEF_END(STATUS_17_REGISTER)


/*
    VPE_Reserved_202:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_202_OFS 0x045c
REGDEF_BEGIN(VPE_RESERVED_202)
REGDEF_BIT(VPE_Reserved_202,        32)
REGDEF_END(VPE_RESERVED_202)


/*
    VPE_Reserved_203:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_203_OFS 0x0460
REGDEF_BEGIN(VPE_RESERVED_203)
REGDEF_BIT(VPE_Reserved_203,        32)
REGDEF_END(VPE_RESERVED_203)


/*
    VPE_Reserved_204:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_204_OFS 0x0464
REGDEF_BEGIN(VPE_RESERVED_204)
REGDEF_BIT(VPE_Reserved_204,        32)
REGDEF_END(VPE_RESERVED_204)


/*
    VPE_Reserved_205:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_205_OFS 0x0468
REGDEF_BEGIN(VPE_RESERVED_205)
REGDEF_BIT(VPE_Reserved_205,        32)
REGDEF_END(VPE_RESERVED_205)


/*
    JOB_EMPT    :    [0x0, 0x1],            bits : 0
    JOB_TYPE    :    [0x0, 0x1],            bits : 1
    SD_SYS_FSM  :    [0x0, 0x7f],           bits : 8_2
    TC_ENG_RFY  :    [0x0, 0x1],            bits : 9
    SD0_HSAC_FSM:    [0x0, 0x1f],           bits : 16_12
    SD1_HSAC_FSM:    [0x0, 0x1f],           bits : 21_17
    SD2_HSAC_FSM:    [0x0, 0x1f],           bits : 26_22
    SD3_HSAC_FSM:    [0x0, 0x1f],           bits : 31_27
*/
#define STATUS_21_REGISTER_OFS 0x046c
REGDEF_BEGIN(STATUS_21_REGISTER)
REGDEF_BIT(JOB_EMPT,        1)
REGDEF_BIT(JOB_TYPE,        1)
REGDEF_BIT(SD_SYS_FSM,        7)
REGDEF_BIT(TC_ENG_RFY,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(SD0_HSAC_FSM,        5)
REGDEF_BIT(SD1_HSAC_FSM,        5)
REGDEF_BIT(SD2_HSAC_FSM,        5)
REGDEF_BIT(SD3_HSAC_FSM,        5)
REGDEF_END(STATUS_21_REGISTER)


/*
    SD_SRC_FRM_SCAN_FIN:    [0x0, 0x1],         bits : 0
    SM_SCAN_DONE       :    [0x0, 0x1],         bits : 1
*/
#define STATUS_22_REGISTER_OFS 0x0470
REGDEF_BEGIN(STATUS_22_REGISTER)
REGDEF_BIT(SD_SRC_FRM_SCAN_FIN,        1)
REGDEF_BIT(SM_SCAN_DONE,        1)
REGDEF_END(STATUS_22_REGISTER)


/*
    VPE_Reserved_206:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_206_OFS 0x0474
REGDEF_BEGIN(VPE_RESERVED_206)
REGDEF_BIT(VPE_Reserved_206,        32)
REGDEF_END(VPE_RESERVED_206)


/*
    RES0_FSM_DONE_KEEP:    [0x0, 0x1],          bits : 0
    RES1_FSM_DONE_KEEP:    [0x0, 0x1],          bits : 1
    RES2_FSM_DONE_KEEP:    [0x0, 0x1],          bits : 2
    RES3_FSM_DONE_KEEP:    [0x0, 0x1],          bits : 3
    TC0_PROC_JOB      :    [0x0, 0x1],          bits : 4
    TC1_PROC_JOB      :    [0x0, 0x1],          bits : 5
    TC2_PROC_JOB      :    [0x0, 0x1],          bits : 6
    TC3_PROC_JOB      :    [0x0, 0x1],          bits : 7
    OUT0_FULL         :    [0x0, 0x1],          bits : 8
    OUT1_FULL         :    [0x0, 0x1],          bits : 9
    OUT2_FULL         :    [0x0, 0x1],          bits : 10
    OUT3_FULL         :    [0x0, 0x1],          bits : 11
    MAS_FSM           :    [0x0, 0x7],          bits : 15_13
*/
#define STATUS_24_REGISTER_OFS 0x0478
REGDEF_BEGIN(STATUS_24_REGISTER)
REGDEF_BIT(RES0_FSM_DONE_KEEP,        1)
REGDEF_BIT(RES1_FSM_DONE_KEEP,        1)
REGDEF_BIT(RES2_FSM_DONE_KEEP,        1)
REGDEF_BIT(RES3_FSM_DONE_KEEP,        1)
REGDEF_BIT(TC0_PROC_JOB,        1)
REGDEF_BIT(TC1_PROC_JOB,        1)
REGDEF_BIT(TC2_PROC_JOB,        1)
REGDEF_BIT(TC3_PROC_JOB,        1)
REGDEF_BIT(OUT0_FULL,        1)
REGDEF_BIT(OUT1_FULL,        1)
REGDEF_BIT(OUT2_FULL,        1)
REGDEF_BIT(OUT3_FULL,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(MAS_FSM,        3)
REGDEF_END(STATUS_24_REGISTER)


/*
    RA_DONE  :    [0x0, 0x1],           bits : 0
    DCTG_DONE:    [0x0, 0x1],           bits : 1
    L2D_DONE :    [0x0, 0x1],           bits : 2
    DP_STALL :    [0x0, 0x1],           bits : 3
    VMR_FSM  :    [0x0, 0x7],           bits : 6_4
    VMR_CNT  :    [0x0, 0x7],           bits : 10_8
    HMR_CNT  :    [0x0, 0x3],           bits : 13_12
    HFRAC    :    [0x0, 0x7f],          bits : 22_16
    VFRAC    :    [0x0, 0x7f],          bits : 30_24
*/
#define STATUS_25_REGISTER_OFS 0x047c
REGDEF_BEGIN(STATUS_25_REGISTER)
REGDEF_BIT(RA_DONE,        1)
REGDEF_BIT(DCTG_DONE,        1)
REGDEF_BIT(L2D_DONE,        1)
REGDEF_BIT(DP_STALL,        1)
REGDEF_BIT(VMR_FSM,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(VMR_CNT,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(HMR_CNT,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(HFRAC,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(VFRAC,        7)
REGDEF_END(STATUS_25_REGISTER)


/*
    COL_CNT:    [0x0, 0x7f],            bits : 14_8
    ROW_CNT:    [0x0, 0x7f],            bits : 22_16
*/
#define STATUS_26_REGISTER_OFS 0x0480
REGDEF_BEGIN(STATUS_26_REGISTER)
REGDEF_BIT(,        8)
REGDEF_BIT(COL_CNT,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ROW_CNT,        7)
REGDEF_END(STATUS_26_REGISTER)


/*
    L2D_DONE   :    [0x0, 0x1],         bits : 0
    DPPM_FULL  :    [0x0, 0x1],         bits : 1
    DCE_FM_DONE:    [0x0, 0x1],         bits : 2
    PHASE      :    [0x0, 0x1],         bits : 3
    SCAN_DONE  :    [0x0, 0x1],         bits : 4
    DWC_DONE   :    [0x0, 0x1],         bits : 5
    DCE_FSM    :    [0x0, 0x7],         bits : 10_8
    DST_X      :    [0x0, 0xffff],          bits : 31_16
*/
#define STATUS_27_REGISTER_OFS 0x0484
REGDEF_BEGIN(STATUS_27_REGISTER)
REGDEF_BIT(L2D_DONE,        1)
REGDEF_BIT(DPPM_FULL,        1)
REGDEF_BIT(DCE_FM_DONE,        1)
REGDEF_BIT(PHASE,        1)
REGDEF_BIT(SCAN_DONE,        1)
REGDEF_BIT(DWC_DONE,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(DCE_FSM,        3)
REGDEF_BIT(,        5)
REGDEF_BIT(DST_X,        16)
REGDEF_END(STATUS_27_REGISTER)


/*
    DST_Y      :    [0x0, 0xffff],          bits : 15_0
    CC_RDY     :    [0x0, 0x1],         bits : 16
    REUSE      :    [0x0, 0x1],         bits : 17
    HMAP_EQ    :    [0x0, 0x1],         bits : 18
    BUF_IDX    :    [0x0, 0x7f],            bits : 30_24
    VPOS_BIT_32:    [0x0, 0x1],         bits : 31
*/
#define STATUS_28_REGISTER_OFS 0x0488
REGDEF_BEGIN(STATUS_28_REGISTER)
REGDEF_BIT(DST_Y,        16)
REGDEF_BIT(CC_RDY,        1)
REGDEF_BIT(REUSE,        1)
REGDEF_BIT(HMAP_EQ,        1)
REGDEF_BIT(,        5)
REGDEF_BIT(BUF_IDX,        7)
REGDEF_BIT(VPOS_BIT_32,        1)
REGDEF_END(STATUS_28_REGISTER)


/*
    VPOS:    [0x0, 0xffffffff],         bits : 31_0
*/
#define STATUS_29_REGISTER_OFS 0x048c
REGDEF_BEGIN(STATUS_29_REGISTER)
REGDEF_BIT(VPOS,        32)
REGDEF_END(STATUS_29_REGISTER)


/*
    LL_TAB_STATUS00:    [0x0, 0xf],         bits : 3_0
    LL_TAB_STATUS01:    [0x0, 0xf],         bits : 7_4
    LL_TAB_STATUS02:    [0x0, 0xf],         bits : 11_8
    LL_TAB_STATUS03:    [0x0, 0xf],         bits : 15_12
    LL_TAB_STATUS04:    [0x0, 0xf],         bits : 19_16
    LL_TAB_STATUS05:    [0x0, 0xf],         bits : 23_20
    LL_TAB_STATUS06:    [0x0, 0xf],         bits : 27_24
    LL_TAB_STATUS07:    [0x0, 0xf],         bits : 31_28
*/
#define STATUS_30_REGISTER_OFS 0x0490
REGDEF_BEGIN(STATUS_30_REGISTER)
REGDEF_BIT(LL_TAB_STATUS00,        4)
REGDEF_BIT(LL_TAB_STATUS01,        4)
REGDEF_BIT(LL_TAB_STATUS02,        4)
REGDEF_BIT(LL_TAB_STATUS03,        4)
REGDEF_BIT(LL_TAB_STATUS04,        4)
REGDEF_BIT(LL_TAB_STATUS05,        4)
REGDEF_BIT(LL_TAB_STATUS06,        4)
REGDEF_BIT(LL_TAB_STATUS07,        4)
REGDEF_END(STATUS_30_REGISTER)


/*
    LL_TAB_STATUS08:    [0x0, 0xf],         bits : 3_0
    LL_TAB_STATUS09:    [0x0, 0xf],         bits : 7_4
    LL_TAB_STATUS10:    [0x0, 0xf],         bits : 11_8
    LL_TAB_STATUS11:    [0x0, 0xf],         bits : 15_12
    LL_TAB_STATUS12:    [0x0, 0xf],         bits : 19_16
    LL_TAB_STATUS13:    [0x0, 0xf],         bits : 23_20
    LL_TAB_STATUS14:    [0x0, 0xf],         bits : 27_24
    LL_TAB_STATUS15:    [0x0, 0xf],         bits : 31_28
*/
#define STATUS_31_REGISTER_OFS 0x0494
REGDEF_BEGIN(STATUS_31_REGISTER)
REGDEF_BIT(LL_TAB_STATUS08,        4)
REGDEF_BIT(LL_TAB_STATUS09,        4)
REGDEF_BIT(LL_TAB_STATUS10,        4)
REGDEF_BIT(LL_TAB_STATUS11,        4)
REGDEF_BIT(LL_TAB_STATUS12,        4)
REGDEF_BIT(LL_TAB_STATUS13,        4)
REGDEF_BIT(LL_TAB_STATUS14,        4)
REGDEF_BIT(LL_TAB_STATUS15,        4)
REGDEF_END(STATUS_31_REGISTER)


/*
    LL_TAB_STATUS16:    [0x0, 0xf],         bits : 3_0
    LL_TAB_STATUS17:    [0x0, 0xf],         bits : 7_4
    LL_TAB_STATUS18:    [0x0, 0xf],         bits : 11_8
    LL_TAB_STATUS19:    [0x0, 0xf],         bits : 15_12
    LL_TAB_STATUS20:    [0x0, 0xf],         bits : 19_16
    LL_TAB_STATUS21:    [0x0, 0xf],         bits : 23_20
    LL_TAB_STATUS22:    [0x0, 0xf],         bits : 27_24
    LL_TAB_STATUS23:    [0x0, 0xf],         bits : 31_28
*/
#define STATUS_32_REGISTER_OFS 0x0498
REGDEF_BEGIN(STATUS_32_REGISTER)
REGDEF_BIT(LL_TAB_STATUS16,        4)
REGDEF_BIT(LL_TAB_STATUS17,        4)
REGDEF_BIT(LL_TAB_STATUS18,        4)
REGDEF_BIT(LL_TAB_STATUS19,        4)
REGDEF_BIT(LL_TAB_STATUS20,        4)
REGDEF_BIT(LL_TAB_STATUS21,        4)
REGDEF_BIT(LL_TAB_STATUS22,        4)
REGDEF_BIT(LL_TAB_STATUS23,        4)
REGDEF_END(STATUS_32_REGISTER)


/*
    LL_TAB_STATUS24:    [0x0, 0xf],         bits : 3_0
    LL_TAB_STATUS25:    [0x0, 0xf],         bits : 7_4
    LL_TAB_STATUS26:    [0x0, 0xf],         bits : 11_8
    LL_TAB_STATUS27:    [0x0, 0xf],         bits : 15_12
    LL_TAB_STATUS28:    [0x0, 0xf],         bits : 19_16
    LL_TAB_STATUS29:    [0x0, 0xf],         bits : 23_20
    LL_TAB_STATUS30:    [0x0, 0xf],         bits : 27_24
    LL_TAB_STATUS31:    [0x0, 0xf],         bits : 31_28
*/
#define STATUS_33_REGISTER_OFS 0x049c
REGDEF_BEGIN(STATUS_33_REGISTER)
REGDEF_BIT(LL_TAB_STATUS24,        4)
REGDEF_BIT(LL_TAB_STATUS25,        4)
REGDEF_BIT(LL_TAB_STATUS26,        4)
REGDEF_BIT(LL_TAB_STATUS27,        4)
REGDEF_BIT(LL_TAB_STATUS28,        4)
REGDEF_BIT(LL_TAB_STATUS29,        4)
REGDEF_BIT(LL_TAB_STATUS30,        4)
REGDEF_BIT(LL_TAB_STATUS31,        4)
REGDEF_END(STATUS_33_REGISTER)


/*
    LL_TAB_STATUS32:    [0x0, 0xf],         bits : 3_0
    LL_TAB_STATUS33:    [0x0, 0xf],         bits : 7_4
    LL_TAB_STATUS34:    [0x0, 0xf],         bits : 11_8
    LL_TAB_STATUS35:    [0x0, 0xf],         bits : 15_12
    LL_TAB_STATUS36:    [0x0, 0xf],         bits : 19_16
    LL_TAB_STATUS37:    [0x0, 0xf],         bits : 23_20
    LL_TAB_STATUS38:    [0x0, 0xf],         bits : 27_24
    LL_TAB_STATUS39:    [0x0, 0xf],         bits : 31_28
*/
#define STATUS_34_REGISTER_OFS 0x04a0
REGDEF_BEGIN(STATUS_34_REGISTER)
REGDEF_BIT(LL_TAB_STATUS32,        4)
REGDEF_BIT(LL_TAB_STATUS33,        4)
REGDEF_BIT(LL_TAB_STATUS34,        4)
REGDEF_BIT(LL_TAB_STATUS35,        4)
REGDEF_BIT(LL_TAB_STATUS36,        4)
REGDEF_BIT(LL_TAB_STATUS37,        4)
REGDEF_BIT(LL_TAB_STATUS38,        4)
REGDEF_BIT(LL_TAB_STATUS39,        4)
REGDEF_END(STATUS_34_REGISTER)


/*
    LL_TAB_STATUS40:    [0x0, 0xf],         bits : 3_0
    LL_TAB_STATUS41:    [0x0, 0xf],         bits : 7_4
    LL_TAB_STATUS42:    [0x0, 0xf],         bits : 11_8
    LL_TAB_STATUS43:    [0x0, 0xf],         bits : 15_12
    LL_TAB_STATUS44:    [0x0, 0xf],         bits : 19_16
    LL_TAB_STATUS45:    [0x0, 0xf],         bits : 23_20
    LL_TAB_STATUS46:    [0x0, 0xf],         bits : 27_24
    LL_TAB_STATUS47:    [0x0, 0xf],         bits : 31_28
*/
#define STATUS_35_REGISTER_OFS 0x04a4
REGDEF_BEGIN(STATUS_35_REGISTER)
REGDEF_BIT(LL_TAB_STATUS40,        4)
REGDEF_BIT(LL_TAB_STATUS41,        4)
REGDEF_BIT(LL_TAB_STATUS42,        4)
REGDEF_BIT(LL_TAB_STATUS43,        4)
REGDEF_BIT(LL_TAB_STATUS44,        4)
REGDEF_BIT(LL_TAB_STATUS45,        4)
REGDEF_BIT(LL_TAB_STATUS46,        4)
REGDEF_BIT(LL_TAB_STATUS47,        4)
REGDEF_END(STATUS_35_REGISTER)


/*
    LL_TAB_STATUS48:    [0x0, 0xf],         bits : 3_0
    LL_TAB_STATUS49:    [0x0, 0xf],         bits : 7_4
    LL_TAB_STATUS50:    [0x0, 0xf],         bits : 11_8
    LL_TAB_STATUS51:    [0x0, 0xf],         bits : 15_12
    LL_TAB_STATUS52:    [0x0, 0xf],         bits : 19_16
    LL_TAB_STATUS53:    [0x0, 0xf],         bits : 23_20
    LL_TAB_STATUS54:    [0x0, 0xf],         bits : 27_24
    LL_TAB_STATUS55:    [0x0, 0xf],         bits : 31_28
*/
#define STATUS_36_REGISTER_OFS 0x04a8
REGDEF_BEGIN(STATUS_36_REGISTER)
REGDEF_BIT(LL_TAB_STATUS48,        4)
REGDEF_BIT(LL_TAB_STATUS49,        4)
REGDEF_BIT(LL_TAB_STATUS50,        4)
REGDEF_BIT(LL_TAB_STATUS51,        4)
REGDEF_BIT(LL_TAB_STATUS52,        4)
REGDEF_BIT(LL_TAB_STATUS53,        4)
REGDEF_BIT(LL_TAB_STATUS54,        4)
REGDEF_BIT(LL_TAB_STATUS55,        4)
REGDEF_END(STATUS_36_REGISTER)


/*
    LL_TAB_STATUS56:    [0x0, 0xf],         bits : 3_0
    LL_TAB_STATUS57:    [0x0, 0xf],         bits : 7_4
    LL_TAB_STATUS58:    [0x0, 0xf],         bits : 11_8
    LL_TAB_STATUS59:    [0x0, 0xf],         bits : 15_12
    LL_TAB_STATUS60:    [0x0, 0xf],         bits : 19_16
    LL_TAB_STATUS61:    [0x0, 0xf],         bits : 23_20
    LL_TAB_STATUS62:    [0x0, 0xf],         bits : 27_24
    LL_TAB_STATUS63:    [0x0, 0xf],         bits : 31_28
*/
#define STATUS_37_REGISTER_OFS 0x04ac
REGDEF_BEGIN(STATUS_37_REGISTER)
REGDEF_BIT(LL_TAB_STATUS56,        4)
REGDEF_BIT(LL_TAB_STATUS57,        4)
REGDEF_BIT(LL_TAB_STATUS58,        4)
REGDEF_BIT(LL_TAB_STATUS59,        4)
REGDEF_BIT(LL_TAB_STATUS60,        4)
REGDEF_BIT(LL_TAB_STATUS61,        4)
REGDEF_BIT(LL_TAB_STATUS62,        4)
REGDEF_BIT(LL_TAB_STATUS63,        4)
REGDEF_END(STATUS_37_REGISTER)


/*
    VPE_Reserved_207:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_207_OFS 0x04b0
REGDEF_BEGIN(VPE_RESERVED_207)
REGDEF_BIT(VPE_Reserved_207,        32)
REGDEF_END(VPE_RESERVED_207)


/*
    VPE_Reserved_208:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_208_OFS 0x04b4
REGDEF_BEGIN(VPE_RESERVED_208)
REGDEF_BIT(VPE_Reserved_208,        32)
REGDEF_END(VPE_RESERVED_208)


/*
    VPE_Reserved_209:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_209_OFS 0x04b8
REGDEF_BEGIN(VPE_RESERVED_209)
REGDEF_BIT(VPE_Reserved_209,        32)
REGDEF_END(VPE_RESERVED_209)


/*
    VPE_Reserved_210:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_210_OFS 0x04bc
REGDEF_BEGIN(VPE_RESERVED_210)
REGDEF_BIT(VPE_Reserved_210,        32)
REGDEF_END(VPE_RESERVED_210)


/*
    VPE_Reserved_211:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_211_OFS 0x04c0
REGDEF_BEGIN(VPE_RESERVED_211)
REGDEF_BIT(VPE_Reserved_211,        32)
REGDEF_END(VPE_RESERVED_211)


/*
    VPE_Reserved_212:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_212_OFS 0x04c4
REGDEF_BEGIN(VPE_RESERVED_212)
REGDEF_BIT(VPE_Reserved_212,        32)
REGDEF_END(VPE_RESERVED_212)


/*
    VPE_Reserved_213:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_213_OFS 0x04c8
REGDEF_BEGIN(VPE_RESERVED_213)
REGDEF_BIT(VPE_Reserved_213,        32)
REGDEF_END(VPE_RESERVED_213)


/*
    VPE_Reserved_214:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_214_OFS 0x04cc
REGDEF_BEGIN(VPE_RESERVED_214)
REGDEF_BIT(VPE_Reserved_214,        32)
REGDEF_END(VPE_RESERVED_214)


/*
    VPE_Reserved_215:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_215_OFS 0x04d0
REGDEF_BEGIN(VPE_RESERVED_215)
REGDEF_BIT(VPE_Reserved_215,        32)
REGDEF_END(VPE_RESERVED_215)


/*
    VPE_Reserved_216:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_216_OFS 0x04d4
REGDEF_BEGIN(VPE_RESERVED_216)
REGDEF_BIT(VPE_Reserved_216,        32)
REGDEF_END(VPE_RESERVED_216)


/*
    VPE_Reserved_217:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_217_OFS 0x04d8
REGDEF_BEGIN(VPE_RESERVED_217)
REGDEF_BIT(VPE_Reserved_217,        32)
REGDEF_END(VPE_RESERVED_217)


/*
    VPE_Reserved_218:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_218_OFS 0x04dc
REGDEF_BEGIN(VPE_RESERVED_218)
REGDEF_BIT(VPE_Reserved_218,        32)
REGDEF_END(VPE_RESERVED_218)


/*
    VPE_Reserved_219:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_219_OFS 0x04e0
REGDEF_BEGIN(VPE_RESERVED_219)
REGDEF_BIT(VPE_Reserved_219,        32)
REGDEF_END(VPE_RESERVED_219)


/*
    VPE_Reserved_220:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_220_OFS 0x04e4
REGDEF_BEGIN(VPE_RESERVED_220)
REGDEF_BIT(VPE_Reserved_220,        32)
REGDEF_END(VPE_RESERVED_220)


/*
    VPE_Reserved_221:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_221_OFS 0x04e8
REGDEF_BEGIN(VPE_RESERVED_221)
REGDEF_BIT(VPE_Reserved_221,        32)
REGDEF_END(VPE_RESERVED_221)


/*
    VPE_Reserved_222:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_222_OFS 0x04ec
REGDEF_BEGIN(VPE_RESERVED_222)
REGDEF_BIT(VPE_Reserved_222,        32)
REGDEF_END(VPE_RESERVED_222)


/*
    VPE_Reserved_223:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_223_OFS 0x04f0
REGDEF_BEGIN(VPE_RESERVED_223)
REGDEF_BIT(VPE_Reserved_223,        32)
REGDEF_END(VPE_RESERVED_223)


/*
    VPE_Reserved_224:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_224_OFS 0x04f4
REGDEF_BEGIN(VPE_RESERVED_224)
REGDEF_BIT(VPE_Reserved_224,        32)
REGDEF_END(VPE_RESERVED_224)


/*
    VPE_Reserved_225:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_225_OFS 0x04f8
REGDEF_BEGIN(VPE_RESERVED_225)
REGDEF_BIT(VPE_Reserved_225,        32)
REGDEF_END(VPE_RESERVED_225)


/*
    VPE_Reserved_226:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_226_OFS 0x04fc
REGDEF_BEGIN(VPE_RESERVED_226)
REGDEF_BIT(VPE_Reserved_226,        32)
REGDEF_END(VPE_RESERVED_226)


/*
    RES0_SCA_EN       :    [0x0, 0x1],          bits : 0
    RES0_SCA_BYPASS_EN:    [0x0, 0x1],          bits : 1
    RES0_SCA_CROP_EN  :    [0x0, 0x1],          bits : 2
    RES0_TC_EN        :    [0x0, 0x1],          bits : 3
    RES0_SCA_UV_SEL   :    [0x0, 0x1],          bits : 4
    RES0_DES_DRT      :    [0x0, 0x3],          bits : 9_8
    RES0_DES_YUV420   :    [0x0, 0x1],          bits : 11
    RES0_DES_UV_SWAP  :    [0x0, 0x1],          bits : 15
    RES0_OUT_BG_SEL   :    [0x0, 0x7],          bits : 18_16
*/
#define RES0_CONTROL_1_REGISTER_OFS 0x0500
REGDEF_BEGIN(RES0_CONTROL_1_REGISTER)
REGDEF_BIT(RES0_SCA_EN,        1)
REGDEF_BIT(RES0_SCA_BYPASS_EN,        1)
REGDEF_BIT(RES0_SCA_CROP_EN,        1)
REGDEF_BIT(RES0_TC_EN,        1)
REGDEF_BIT(RES0_SCA_UV_SEL,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(RES0_DES_DRT,        2)
REGDEF_BIT(,        1)
REGDEF_BIT(RES0_DES_YUV420,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(RES0_DES_UV_SWAP,        1)
REGDEF_BIT(RES0_OUT_BG_SEL,        3)
REGDEF_END(RES0_CONTROL_1_REGISTER)


/*
    RES0_DES_DP0_YCC_ENC_EN:    [0x0, 0x1],         bits : 0
    RES0_DES_DP0_CHRW      :    [0x0, 0x1],         bits : 4
    RES0_DES_DP0_FORMAT    :    [0x0, 0x3],         bits : 9_8
*/
#define RES0_CONTROL_2_REGISTER_OFS 0x0504
REGDEF_BEGIN(RES0_CONTROL_2_REGISTER)
REGDEF_BIT(RES0_DES_DP0_YCC_ENC_EN,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(RES0_DES_DP0_CHRW,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(RES0_DES_DP0_FORMAT,        2)
REGDEF_END(RES0_CONTROL_2_REGISTER)


/*
    RES0_SCA_HEIGHT:    [0x0, 0x1fff],          bits : 12_0
*/
#define RES0_SIZE_1_REGISTER_OFS 0x0508
REGDEF_BEGIN(RES0_SIZE_1_REGISTER)
REGDEF_BIT(RES0_SCA_HEIGHT,        13)
REGDEF_END(RES0_SIZE_1_REGISTER)


/*
    RES0_DES_BUF_WIDTH :    [0x0, 0x3fff],          bits : 13_0
    RES0_DES_BUF_HEIGHT:    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_SIZE_2_REGISTER_OFS 0x050c
REGDEF_BEGIN(RES0_SIZE_2_REGISTER)
REGDEF_BIT(RES0_DES_BUF_WIDTH,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(RES0_DES_BUF_HEIGHT,        13)
REGDEF_END(RES0_SIZE_2_REGISTER)


/*
    RES0_OUT_Y_START:    [0x0, 0x1fff],         bits : 12_0
    RES0_OUT_HEIGHT :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES0_SIZE_3_REGISTER_OFS 0x0510
REGDEF_BEGIN(RES0_SIZE_3_REGISTER)
REGDEF_BIT(RES0_OUT_Y_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES0_OUT_HEIGHT,        13)
REGDEF_END(RES0_SIZE_3_REGISTER)


/*
    RES0_RLT_Y_START:    [0x0, 0x1fff],         bits : 12_0
    RES0_RLT_HEIGHT :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES0_SIZE_4_REGISTER_OFS 0x0514
REGDEF_BEGIN(RES0_SIZE_4_REGISTER)
REGDEF_BIT(RES0_RLT_Y_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES0_RLT_HEIGHT,        13)
REGDEF_END(RES0_SIZE_4_REGISTER)


/*
    RES0_PIP_Y_START:    [0x0, 0x1fff],         bits : 12_0
    RES0_PIP_HEIGHT :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES0_SIZE_5_REGISTER_OFS 0x0518
REGDEF_BEGIN(RES0_SIZE_5_REGISTER)
REGDEF_BIT(RES0_PIP_Y_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES0_PIP_HEIGHT,        13)
REGDEF_END(RES0_SIZE_5_REGISTER)


/*
    RES0_SCA_CROP_Y_START:    [0x0, 0xfff],         bits : 11_0
    RES0_SCA_CROP_HEIGHT :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES0_SCA_CROP_REGISTER_OFS 0x051c
REGDEF_BEGIN(RES0_SCA_CROP_REGISTER)
REGDEF_BIT(RES0_SCA_CROP_Y_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES0_SCA_CROP_HEIGHT,        13)
REGDEF_END(RES0_SCA_CROP_REGISTER)


/*
    RES0_SCA_Y_LUMA_ALGO_EN  :    [0x0, 0x3],           bits : 1_0
    RES0_SCA_X_LUMA_ALGO_EN  :    [0x0, 0x3],           bits : 3_2
    RES0_SCA_Y_CHROMA_ALGO_EN:    [0x0, 0x3],           bits : 5_4
    RES0_SCA_X_CHROMA_ALGO_EN:    [0x0, 0x3],           bits : 7_6
    RES0_SCA_MAP_SEL         :    [0x0, 0x1],           bits : 8
*/
#define RES0_SCA_1_REGISTER_OFS 0x0520
REGDEF_BEGIN(RES0_SCA_1_REGISTER)
REGDEF_BIT(RES0_SCA_Y_LUMA_ALGO_EN,        2)
REGDEF_BIT(RES0_SCA_X_LUMA_ALGO_EN,        2)
REGDEF_BIT(RES0_SCA_Y_CHROMA_ALGO_EN,        2)
REGDEF_BIT(RES0_SCA_X_CHROMA_ALGO_EN,        2)
REGDEF_BIT(RES0_SCA_MAP_SEL,        1)
REGDEF_END(RES0_SCA_1_REGISTER)


/*
    RES0_SCA_HSTEP:    [0x0, 0x3fff],           bits : 13_0
    RES0_SCA_VSTEP:    [0x0, 0x3fff],           bits : 29_16
*/
#define RES0_SCA_2_REGISTER_OFS 0x0524
REGDEF_BEGIN(RES0_SCA_2_REGISTER)
REGDEF_BIT(RES0_SCA_HSTEP,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(RES0_SCA_VSTEP,        14)
REGDEF_END(RES0_SCA_2_REGISTER)


/*
    RES0_SCA_HSCA_DIVISOR:    [0x0, 0x1fff],            bits : 12_0
    RES0_SCA_HAVG_PXL_MSK:    [0x0, 0xff],          bits : 23_16
*/
#define RES0_SCA_3_REGISTER_OFS 0x0528
REGDEF_BEGIN(RES0_SCA_3_REGISTER)
REGDEF_BIT(RES0_SCA_HSCA_DIVISOR,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES0_SCA_HAVG_PXL_MSK,        8)
REGDEF_END(RES0_SCA_3_REGISTER)


/*
    RES0_SCA_VSCA_DIVISOR:    [0x0, 0x1fff],            bits : 12_0
    RES0_SCA_VAVG_PXL_MSK:    [0x0, 0xff],          bits : 23_16
*/
#define RES0_SCA_4_REGISTER_OFS 0x052c
REGDEF_BEGIN(RES0_SCA_4_REGISTER)
REGDEF_BIT(RES0_SCA_VSCA_DIVISOR,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES0_SCA_VAVG_PXL_MSK,        8)
REGDEF_END(RES0_SCA_4_REGISTER)


/*
    RES0_SCA_COEF_H0:    [0x0, 0x3ff],          bits : 9_0
    RES0_SCA_COEF_H1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RES0_SCA_5_REGISTER_OFS 0x0530
REGDEF_BEGIN(RES0_SCA_5_REGISTER)
REGDEF_BIT(RES0_SCA_COEF_H0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(RES0_SCA_COEF_H1,        10)
REGDEF_END(RES0_SCA_5_REGISTER)


/*
    RES0_SCA_COEF_H2:    [0x0, 0x3ff],          bits : 9_0
    RES0_SCA_COEF_H3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RES0_SCA_6_REGISTER_OFS 0x0534
REGDEF_BEGIN(RES0_SCA_6_REGISTER)
REGDEF_BIT(RES0_SCA_COEF_H2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(RES0_SCA_COEF_H3,        10)
REGDEF_END(RES0_SCA_6_REGISTER)


/*
    RES0_SCA_COEF_V0:    [0x0, 0x3ff],          bits : 9_0
    RES0_SCA_COEF_V1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RES0_SCA_7_REGISTER_OFS 0x0538
REGDEF_BEGIN(RES0_SCA_7_REGISTER)
REGDEF_BIT(RES0_SCA_COEF_V0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(RES0_SCA_COEF_V1,        10)
REGDEF_END(RES0_SCA_7_REGISTER)


/*
    RES0_SCA_COEF_V2:    [0x0, 0x3ff],          bits : 9_0
    RES0_SCA_COEF_V3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RES0_SCA_8_REGISTER_OFS 0x053c
REGDEF_BEGIN(RES0_SCA_8_REGISTER)
REGDEF_BIT(RES0_SCA_COEF_V2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(RES0_SCA_COEF_V3,        10)
REGDEF_END(RES0_SCA_8_REGISTER)


/*
    RES0_TC_CROP_Y_START:    [0x0, 0x1fff],         bits : 12_0
    RES0_TC_CROP_HEIGHT :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES0_TC_REGISTER_OFS 0x0540
REGDEF_BEGIN(RES0_TC_REGISTER)
REGDEF_BIT(RES0_TC_CROP_Y_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES0_TC_CROP_HEIGHT,        13)
REGDEF_END(RES0_TC_REGISTER)


/*
    VPE_Reserved_227:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_227_OFS 0x0544
REGDEF_BEGIN(VPE_RESERVED_227)
REGDEF_BIT(VPE_Reserved_227,        32)
REGDEF_END(VPE_RESERVED_227)


/*
    VPE_Reserved_228:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_228_OFS 0x0548
REGDEF_BEGIN(VPE_RESERVED_228)
REGDEF_BIT(VPE_Reserved_228,        32)
REGDEF_END(VPE_RESERVED_228)


/*
    VPE_Reserved_229:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_229_OFS 0x054c
REGDEF_BEGIN(VPE_RESERVED_229)
REGDEF_BIT(VPE_Reserved_229,        32)
REGDEF_END(VPE_RESERVED_229)


/*
    VPE_Reserved_230:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_230_OFS 0x0550
REGDEF_BEGIN(VPE_RESERVED_230)
REGDEF_BIT(VPE_Reserved_230,        32)
REGDEF_END(VPE_RESERVED_230)


/*
    VPE_Reserved_231:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_231_OFS 0x0554
REGDEF_BEGIN(VPE_RESERVED_231)
REGDEF_BIT(VPE_Reserved_231,        32)
REGDEF_END(VPE_RESERVED_231)


/*
    VPE_Reserved_232:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_232_OFS 0x0558
REGDEF_BEGIN(VPE_RESERVED_232)
REGDEF_BIT(VPE_Reserved_232,        32)
REGDEF_END(VPE_RESERVED_232)


/*
    VPE_Reserved_233:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_233_OFS 0x055c
REGDEF_BEGIN(VPE_RESERVED_233)
REGDEF_BIT(VPE_Reserved_233,        32)
REGDEF_END(VPE_RESERVED_233)


/*
    VPE_Reserved_234:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_234_OFS 0x0560
REGDEF_BEGIN(VPE_RESERVED_234)
REGDEF_BIT(VPE_Reserved_234,        32)
REGDEF_END(VPE_RESERVED_234)


/*
    VPE_Reserved_235:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_235_OFS 0x0564
REGDEF_BEGIN(VPE_RESERVED_235)
REGDEF_BIT(VPE_Reserved_235,        32)
REGDEF_END(VPE_RESERVED_235)


/*
    VPE_Reserved_236:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_236_OFS 0x0568
REGDEF_BEGIN(VPE_RESERVED_236)
REGDEF_BIT(VPE_Reserved_236,        32)
REGDEF_END(VPE_RESERVED_236)


/*
    VPE_Reserved_237:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_237_OFS 0x056c
REGDEF_BEGIN(VPE_RESERVED_237)
REGDEF_BIT(VPE_Reserved_237,        32)
REGDEF_END(VPE_RESERVED_237)


/*
    VPE_Reserved_238:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_238_OFS 0x0570
REGDEF_BEGIN(VPE_RESERVED_238)
REGDEF_BIT(VPE_Reserved_238,        32)
REGDEF_END(VPE_RESERVED_238)


/*
    VPE_Reserved_239:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_239_OFS 0x0574
REGDEF_BEGIN(VPE_RESERVED_239)
REGDEF_BIT(VPE_Reserved_239,        32)
REGDEF_END(VPE_RESERVED_239)


/*
    VPE_Reserved_240:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_240_OFS 0x0578
REGDEF_BEGIN(VPE_RESERVED_240)
REGDEF_BIT(VPE_Reserved_240,        32)
REGDEF_END(VPE_RESERVED_240)


/*
    VPE_Reserved_241:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_241_OFS 0x057c
REGDEF_BEGIN(VPE_RESERVED_241)
REGDEF_BIT(VPE_Reserved_241,        32)
REGDEF_END(VPE_RESERVED_241)


/*
    RES1_SCA_EN       :    [0x0, 0x1],          bits : 0
    RES1_SCA_BYPASS_EN:    [0x0, 0x1],          bits : 1
    RES1_SCA_CROP_EN  :    [0x0, 0x1],          bits : 2
    RES1_TC_EN        :    [0x0, 0x1],          bits : 3
    RES1_SCA_UV_SEL   :    [0x0, 0x1],          bits : 4
    RES1_DES_DRT      :    [0x0, 0x3],          bits : 9_8
    RES1_DES_YUV420   :    [0x0, 0x1],          bits : 11
    RES1_DES_UV_SWAP  :    [0x0, 0x1],          bits : 15
    RES1_OUT_BG_SEL   :    [0x0, 0x7],          bits : 18_16
*/
#define RES1_CONTROL_1_REGISTER_OFS 0x0580
REGDEF_BEGIN(RES1_CONTROL_1_REGISTER)
REGDEF_BIT(RES1_SCA_EN,        1)
REGDEF_BIT(RES1_SCA_BYPASS_EN,        1)
REGDEF_BIT(RES1_SCA_CROP_EN,        1)
REGDEF_BIT(RES1_TC_EN,        1)
REGDEF_BIT(RES1_SCA_UV_SEL,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(RES1_DES_DRT,        2)
REGDEF_BIT(,        1)
REGDEF_BIT(RES1_DES_YUV420,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(RES1_DES_UV_SWAP,        1)
REGDEF_BIT(RES1_OUT_BG_SEL,        3)
REGDEF_END(RES1_CONTROL_1_REGISTER)


/*
    RES1_DES_DP0_YCC_ENC_EN:    [0x0, 0x1],         bits : 0
    RES1_DES_DP0_CHRW      :    [0x0, 0x1],         bits : 4
    RES1_DES_DP0_FORMAT    :    [0x0, 0x3],         bits : 9_8
*/
#define RES1_CONTROL_2_REGISTER_OFS 0x0584
REGDEF_BEGIN(RES1_CONTROL_2_REGISTER)
REGDEF_BIT(RES1_DES_DP0_YCC_ENC_EN,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(RES1_DES_DP0_CHRW,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(RES1_DES_DP0_FORMAT,        2)
REGDEF_END(RES1_CONTROL_2_REGISTER)


/*
    RES1_SCA_HEIGHT:    [0x0, 0x1fff],          bits : 12_0
*/
#define RES1_SIZE_1_REGISTER_OFS 0x0588
REGDEF_BEGIN(RES1_SIZE_1_REGISTER)
REGDEF_BIT(RES1_SCA_HEIGHT,        13)
REGDEF_END(RES1_SIZE_1_REGISTER)


/*
    RES1_DES_BUF_WIDTH :    [0x0, 0x3fff],          bits : 13_0
    RES1_DES_BUF_HEIGHT:    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_SIZE_2_REGISTER_OFS 0x058c
REGDEF_BEGIN(RES1_SIZE_2_REGISTER)
REGDEF_BIT(RES1_DES_BUF_WIDTH,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(RES1_DES_BUF_HEIGHT,        13)
REGDEF_END(RES1_SIZE_2_REGISTER)


/*
    RES1_OUT_Y_START:    [0x0, 0x1fff],         bits : 12_0
    RES1_OUT_HEIGHT :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES1_SIZE_3_REGISTER_OFS 0x0590
REGDEF_BEGIN(RES1_SIZE_3_REGISTER)
REGDEF_BIT(RES1_OUT_Y_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES1_OUT_HEIGHT,        13)
REGDEF_END(RES1_SIZE_3_REGISTER)


/*
    RES1_RLT_Y_START:    [0x0, 0x1fff],         bits : 12_0
    RES1_RLT_HEIGHT :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES1_SIZE_4_REGISTER_OFS 0x0594
REGDEF_BEGIN(RES1_SIZE_4_REGISTER)
REGDEF_BIT(RES1_RLT_Y_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES1_RLT_HEIGHT,        13)
REGDEF_END(RES1_SIZE_4_REGISTER)


/*
    RES1_PIP_Y_START:    [0x0, 0x1fff],         bits : 12_0
    RES1_PIP_HEIGHT :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES1_SIZE_5_REGISTER_OFS 0x0598
REGDEF_BEGIN(RES1_SIZE_5_REGISTER)
REGDEF_BIT(RES1_PIP_Y_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES1_PIP_HEIGHT,        13)
REGDEF_END(RES1_SIZE_5_REGISTER)


/*
    RES1_SCA_CROP_Y_START:    [0x0, 0xfff],         bits : 11_0
    RES1_SCA_CROP_HEIGHT :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES1_SCA_CROP_REGISTER_OFS 0x059c
REGDEF_BEGIN(RES1_SCA_CROP_REGISTER)
REGDEF_BIT(RES1_SCA_CROP_Y_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES1_SCA_CROP_HEIGHT,        13)
REGDEF_END(RES1_SCA_CROP_REGISTER)


/*
    RES1_SCA_Y_LUMA_ALGO_EN  :    [0x0, 0x3],           bits : 1_0
    RES1_SCA_X_LUMA_ALGO_EN  :    [0x0, 0x3],           bits : 3_2
    RES1_SCA_Y_CHROMA_ALGO_EN:    [0x0, 0x3],           bits : 5_4
    RES1_SCA_X_CHROMA_ALGO_EN:    [0x0, 0x3],           bits : 7_6
    RES1_SCA_MAP_SEL         :    [0x0, 0x1],           bits : 8
*/
#define RES1_SCA_1_REGISTER_OFS 0x05a0
REGDEF_BEGIN(RES1_SCA_1_REGISTER)
REGDEF_BIT(RES1_SCA_Y_LUMA_ALGO_EN,        2)
REGDEF_BIT(RES1_SCA_X_LUMA_ALGO_EN,        2)
REGDEF_BIT(RES1_SCA_Y_CHROMA_ALGO_EN,        2)
REGDEF_BIT(RES1_SCA_X_CHROMA_ALGO_EN,        2)
REGDEF_BIT(RES1_SCA_MAP_SEL,        1)
REGDEF_END(RES1_SCA_1_REGISTER)


/*
    RES1_SCA_HSTEP:    [0x0, 0x3fff],           bits : 13_0
    RES1_SCA_VSTEP:    [0x0, 0x3fff],           bits : 29_16
*/
#define RES1_SCA_2_REGISTER_OFS 0x05a4
REGDEF_BEGIN(RES1_SCA_2_REGISTER)
REGDEF_BIT(RES1_SCA_HSTEP,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(RES1_SCA_VSTEP,        14)
REGDEF_END(RES1_SCA_2_REGISTER)


/*
    RES1_SCA_HSCA_DIVISOR:    [0x0, 0x1fff],            bits : 12_0
    RES1_SCA_HAVG_PXL_MSK:    [0x0, 0xff],          bits : 23_16
*/
#define RES1_SCA_3_REGISTER_OFS 0x05a8
REGDEF_BEGIN(RES1_SCA_3_REGISTER)
REGDEF_BIT(RES1_SCA_HSCA_DIVISOR,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES1_SCA_HAVG_PXL_MSK,        8)
REGDEF_END(RES1_SCA_3_REGISTER)


/*
    RES1_SCA_VSCA_DIVISOR:    [0x0, 0x1fff],            bits : 12_0
    RES1_SCA_VAVG_PXL_MSK:    [0x0, 0xff],          bits : 23_16
*/
#define RES1_SCA_4_REGISTER_OFS 0x05ac
REGDEF_BEGIN(RES1_SCA_4_REGISTER)
REGDEF_BIT(RES1_SCA_VSCA_DIVISOR,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES1_SCA_VAVG_PXL_MSK,        8)
REGDEF_END(RES1_SCA_4_REGISTER)


/*
    RES1_SCA_COEF_H0:    [0x0, 0x3ff],          bits : 9_0
    RES1_SCA_COEF_H1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RES1_SCA_5_REGISTER_OFS 0x05b0
REGDEF_BEGIN(RES1_SCA_5_REGISTER)
REGDEF_BIT(RES1_SCA_COEF_H0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(RES1_SCA_COEF_H1,        10)
REGDEF_END(RES1_SCA_5_REGISTER)


/*
    RES1_SCA_COEF_H2:    [0x0, 0x3ff],          bits : 9_0
    RES1_SCA_COEF_H3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RES1_SCA_6_REGISTER_OFS 0x05b4
REGDEF_BEGIN(RES1_SCA_6_REGISTER)
REGDEF_BIT(RES1_SCA_COEF_H2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(RES1_SCA_COEF_H3,        10)
REGDEF_END(RES1_SCA_6_REGISTER)


/*
    RES1_SCA_COEF_V0:    [0x0, 0x3ff],          bits : 9_0
    RES1_SCA_COEF_V1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RES1_SCA_7_REGISTER_OFS 0x05b8
REGDEF_BEGIN(RES1_SCA_7_REGISTER)
REGDEF_BIT(RES1_SCA_COEF_V0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(RES1_SCA_COEF_V1,        10)
REGDEF_END(RES1_SCA_7_REGISTER)


/*
    RES1_SCA_COEF_V2:    [0x0, 0x3ff],          bits : 9_0
    RES1_SCA_COEF_V3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RES1_SCA_8_REGISTER_OFS 0x05bc
REGDEF_BEGIN(RES1_SCA_8_REGISTER)
REGDEF_BIT(RES1_SCA_COEF_V2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(RES1_SCA_COEF_V3,        10)
REGDEF_END(RES1_SCA_8_REGISTER)


/*
    RES1_TC_CROP_Y_START:    [0x0, 0x1fff],         bits : 12_0
    RES1_TC_CROP_HEIGHT :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES1_TC_REGISTER_OFS 0x05c0
REGDEF_BEGIN(RES1_TC_REGISTER)
REGDEF_BIT(RES1_TC_CROP_Y_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES1_TC_CROP_HEIGHT,        13)
REGDEF_END(RES1_TC_REGISTER)


/*
    VPE_Reserved_242:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_242_OFS 0x05c4
REGDEF_BEGIN(VPE_RESERVED_242)
REGDEF_BIT(VPE_Reserved_242,        32)
REGDEF_END(VPE_RESERVED_242)


/*
    VPE_Reserved_243:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_243_OFS 0x05c8
REGDEF_BEGIN(VPE_RESERVED_243)
REGDEF_BIT(VPE_Reserved_243,        32)
REGDEF_END(VPE_RESERVED_243)


/*
    VPE_Reserved_244:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_244_OFS 0x05cc
REGDEF_BEGIN(VPE_RESERVED_244)
REGDEF_BIT(VPE_Reserved_244,        32)
REGDEF_END(VPE_RESERVED_244)


/*
    VPE_Reserved_245:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_245_OFS 0x05d0
REGDEF_BEGIN(VPE_RESERVED_245)
REGDEF_BIT(VPE_Reserved_245,        32)
REGDEF_END(VPE_RESERVED_245)


/*
    VPE_Reserved_246:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_246_OFS 0x05d4
REGDEF_BEGIN(VPE_RESERVED_246)
REGDEF_BIT(VPE_Reserved_246,        32)
REGDEF_END(VPE_RESERVED_246)


/*
    VPE_Reserved_247:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_247_OFS 0x05d8
REGDEF_BEGIN(VPE_RESERVED_247)
REGDEF_BIT(VPE_Reserved_247,        32)
REGDEF_END(VPE_RESERVED_247)


/*
    VPE_Reserved_248:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_248_OFS 0x05dc
REGDEF_BEGIN(VPE_RESERVED_248)
REGDEF_BIT(VPE_Reserved_248,        32)
REGDEF_END(VPE_RESERVED_248)


/*
    VPE_Reserved_249:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_249_OFS 0x05e0
REGDEF_BEGIN(VPE_RESERVED_249)
REGDEF_BIT(VPE_Reserved_249,        32)
REGDEF_END(VPE_RESERVED_249)


/*
    VPE_Reserved_250:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_250_OFS 0x05e4
REGDEF_BEGIN(VPE_RESERVED_250)
REGDEF_BIT(VPE_Reserved_250,        32)
REGDEF_END(VPE_RESERVED_250)


/*
    VPE_Reserved_251:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_251_OFS 0x05e8
REGDEF_BEGIN(VPE_RESERVED_251)
REGDEF_BIT(VPE_Reserved_251,        32)
REGDEF_END(VPE_RESERVED_251)


/*
    VPE_Reserved_252:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_252_OFS 0x05ec
REGDEF_BEGIN(VPE_RESERVED_252)
REGDEF_BIT(VPE_Reserved_252,        32)
REGDEF_END(VPE_RESERVED_252)


/*
    VPE_Reserved_253:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_253_OFS 0x05f0
REGDEF_BEGIN(VPE_RESERVED_253)
REGDEF_BIT(VPE_Reserved_253,        32)
REGDEF_END(VPE_RESERVED_253)


/*
    VPE_Reserved_254:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_254_OFS 0x05f4
REGDEF_BEGIN(VPE_RESERVED_254)
REGDEF_BIT(VPE_Reserved_254,        32)
REGDEF_END(VPE_RESERVED_254)


/*
    VPE_Reserved_255:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_255_OFS 0x05f8
REGDEF_BEGIN(VPE_RESERVED_255)
REGDEF_BIT(VPE_Reserved_255,        32)
REGDEF_END(VPE_RESERVED_255)


/*
    VPE_Reserved_256:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_256_OFS 0x05fc
REGDEF_BEGIN(VPE_RESERVED_256)
REGDEF_BIT(VPE_Reserved_256,        32)
REGDEF_END(VPE_RESERVED_256)


/*
    RS0_CL0_SCA_WIDTH    :    [0x0, 0x1fff],            bits : 12_0
    RS0_CL0_SCA_HSTEP_OFT:    [0x0, 0x7fff],            bits : 30_16
*/
#define RES0_COL0_SIZE_1_REGISTER_OFS 0x0600
REGDEF_BEGIN(RES0_COL0_SIZE_1_REGISTER)
REGDEF_BIT(RS0_CL0_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL0_SCA_HSTEP_OFT,        15)
REGDEF_END(RES0_COL0_SIZE_1_REGISTER)


/*
    RS0_CL0_OUT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL0_OUT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL0_SIZE_2_REGISTER_OFS 0x0604
REGDEF_BEGIN(RES0_COL0_SIZE_2_REGISTER)
REGDEF_BIT(RS0_CL0_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL0_OUT_WIDTH,        13)
REGDEF_END(RES0_COL0_SIZE_2_REGISTER)


/*
    RS0_CL0_RLT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL0_RLT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL0_SIZE_3_REGISTER_OFS 0x0608
REGDEF_BEGIN(RES0_COL0_SIZE_3_REGISTER)
REGDEF_BIT(RS0_CL0_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL0_RLT_WIDTH,        13)
REGDEF_END(RES0_COL0_SIZE_3_REGISTER)


/*
    RS0_CL0_PIP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL0_PIP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL0_SIZE_4_REGISTER_OFS 0x060c
REGDEF_BEGIN(RES0_COL0_SIZE_4_REGISTER)
REGDEF_BIT(RS0_CL0_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL0_PIP_WIDTH,        13)
REGDEF_END(RES0_COL0_SIZE_4_REGISTER)


/*
    RS0_CL0_SCA_CROP_X_START:    [0x0, 0xfff],          bits : 11_0
    RS0_CL0_SCA_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES0_COL0_SIZE_5_REGISTER_OFS 0x0610
REGDEF_BEGIN(RES0_COL0_SIZE_5_REGISTER)
REGDEF_BIT(RS0_CL0_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RS0_CL0_SCA_CROP_WIDTH,        13)
REGDEF_END(RES0_COL0_SIZE_5_REGISTER)


/*
    RS0_CL0_TC_CROP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL0_TC_CROP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
    RS0_CL0_TC_CROP_SKIP   :    [0x0, 0x1],         bits : 31
*/
#define RES0_COL0_SIZE_6_REGISTER_OFS 0x0614
REGDEF_BEGIN(RES0_COL0_SIZE_6_REGISTER)
REGDEF_BIT(RS0_CL0_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL0_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RS0_CL0_TC_CROP_SKIP,        1)
REGDEF_END(RES0_COL0_SIZE_6_REGISTER)


/*
    VPE_Reserved_257:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_257_OFS 0x0618
REGDEF_BEGIN(VPE_RESERVED_257)
REGDEF_BIT(VPE_Reserved_257,        32)
REGDEF_END(VPE_RESERVED_257)


/*
    VPE_Reserved_258:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_258_OFS 0x061c
REGDEF_BEGIN(VPE_RESERVED_258)
REGDEF_BIT(VPE_Reserved_258,        32)
REGDEF_END(VPE_RESERVED_258)


/*
    RS0_CL1_SCA_WIDTH    :    [0x0, 0x1fff],            bits : 12_0
    RS0_CL1_SCA_HSTEP_OFT:    [0x0, 0x7fff],            bits : 30_16
*/
#define RES0_COL1_SIZE_1_REGISTER_OFS 0x0620
REGDEF_BEGIN(RES0_COL1_SIZE_1_REGISTER)
REGDEF_BIT(RS0_CL1_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL1_SCA_HSTEP_OFT,        15)
REGDEF_END(RES0_COL1_SIZE_1_REGISTER)


/*
    RS0_CL1_OUT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL1_OUT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL1_SIZE_2_REGISTER_OFS 0x0624
REGDEF_BEGIN(RES0_COL1_SIZE_2_REGISTER)
REGDEF_BIT(RS0_CL1_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL1_OUT_WIDTH,        13)
REGDEF_END(RES0_COL1_SIZE_2_REGISTER)


/*
    RS0_CL1_RLT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL1_RLT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL1_SIZE_3_REGISTER_OFS 0x0628
REGDEF_BEGIN(RES0_COL1_SIZE_3_REGISTER)
REGDEF_BIT(RS0_CL1_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL1_RLT_WIDTH,        13)
REGDEF_END(RES0_COL1_SIZE_3_REGISTER)


/*
    RS0_CL1_PIP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL1_PIP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL1_SIZE_4_REGISTER_OFS 0x062c
REGDEF_BEGIN(RES0_COL1_SIZE_4_REGISTER)
REGDEF_BIT(RS0_CL1_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL1_PIP_WIDTH,        13)
REGDEF_END(RES0_COL1_SIZE_4_REGISTER)


/*
    RS0_CL1_SCA_CROP_X_START:    [0x0, 0xfff],          bits : 11_0
    RS0_CL1_SCA_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES0_COL1_SIZE_5_REGISTER_OFS 0x0630
REGDEF_BEGIN(RES0_COL1_SIZE_5_REGISTER)
REGDEF_BIT(RS0_CL1_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RS0_CL1_SCA_CROP_WIDTH,        13)
REGDEF_END(RES0_COL1_SIZE_5_REGISTER)


/*
    RS0_CL1_TC_CROP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL1_TC_CROP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
    RS0_CL1_TC_CROP_SKIP   :    [0x0, 0x1],         bits : 31
*/
#define RES0_COL1_SIZE_6_REGISTER_OFS 0x0634
REGDEF_BEGIN(RES0_COL1_SIZE_6_REGISTER)
REGDEF_BIT(RS0_CL1_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL1_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RS0_CL1_TC_CROP_SKIP,        1)
REGDEF_END(RES0_COL1_SIZE_6_REGISTER)


/*
    VPE_Reserved_259:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_259_OFS 0x0638
REGDEF_BEGIN(VPE_RESERVED_259)
REGDEF_BIT(VPE_Reserved_259,        32)
REGDEF_END(VPE_RESERVED_259)


/*
    VPE_Reserved_260:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_260_OFS 0x063c
REGDEF_BEGIN(VPE_RESERVED_260)
REGDEF_BIT(VPE_Reserved_260,        32)
REGDEF_END(VPE_RESERVED_260)


/*
    RS0_CL2_SCA_WIDTH    :    [0x0, 0x1fff],            bits : 12_0
    RS0_CL2_SCA_HSTEP_OFT:    [0x0, 0x7fff],            bits : 30_16
*/
#define RES0_COL2_SIZE_1_REGISTER_OFS 0x0640
REGDEF_BEGIN(RES0_COL2_SIZE_1_REGISTER)
REGDEF_BIT(RS0_CL2_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL2_SCA_HSTEP_OFT,        15)
REGDEF_END(RES0_COL2_SIZE_1_REGISTER)


/*
    RS0_CL2_OUT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL2_OUT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL2_SIZE_2_REGISTER_OFS 0x0644
REGDEF_BEGIN(RES0_COL2_SIZE_2_REGISTER)
REGDEF_BIT(RS0_CL2_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL2_OUT_WIDTH,        13)
REGDEF_END(RES0_COL2_SIZE_2_REGISTER)


/*
    RS0_CL2_RLT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL2_RLT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL2_SIZE_3_REGISTER_OFS 0x0648
REGDEF_BEGIN(RES0_COL2_SIZE_3_REGISTER)
REGDEF_BIT(RS0_CL2_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL2_RLT_WIDTH,        13)
REGDEF_END(RES0_COL2_SIZE_3_REGISTER)


/*
    RS0_CL2_PIP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL2_PIP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL2_SIZE_4_REGISTER_OFS 0x064c
REGDEF_BEGIN(RES0_COL2_SIZE_4_REGISTER)
REGDEF_BIT(RS0_CL2_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL2_PIP_WIDTH,        13)
REGDEF_END(RES0_COL2_SIZE_4_REGISTER)


/*
    RS0_CL2_SCA_CROP_X_START:    [0x0, 0xfff],          bits : 11_0
    RS0_CL2_SCA_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES0_COL2_SIZE_5_REGISTER_OFS 0x0650
REGDEF_BEGIN(RES0_COL2_SIZE_5_REGISTER)
REGDEF_BIT(RS0_CL2_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RS0_CL2_SCA_CROP_WIDTH,        13)
REGDEF_END(RES0_COL2_SIZE_5_REGISTER)


/*
    RS0_CL2_TC_CROP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL2_TC_CROP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
    RS0_CL2_TC_CROP_SKIP   :    [0x0, 0x1],         bits : 31
*/
#define RES0_COL2_SIZE_6_REGISTER_OFS 0x0654
REGDEF_BEGIN(RES0_COL2_SIZE_6_REGISTER)
REGDEF_BIT(RS0_CL2_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL2_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RS0_CL2_TC_CROP_SKIP,        1)
REGDEF_END(RES0_COL2_SIZE_6_REGISTER)


/*
    VPE_Reserved_261:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_261_OFS 0x0658
REGDEF_BEGIN(VPE_RESERVED_261)
REGDEF_BIT(VPE_Reserved_261,        32)
REGDEF_END(VPE_RESERVED_261)


/*
    VPE_Reserved_262:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_262_OFS 0x065c
REGDEF_BEGIN(VPE_RESERVED_262)
REGDEF_BIT(VPE_Reserved_262,        32)
REGDEF_END(VPE_RESERVED_262)


/*
    RS0_CL3_SCA_WIDTH    :    [0x0, 0x1fff],            bits : 12_0
    RS0_CL3_SCA_HSTEP_OFT:    [0x0, 0x7fff],            bits : 30_16
*/
#define RES0_COL3_SIZE_1_REGISTER_OFS 0x0660
REGDEF_BEGIN(RES0_COL3_SIZE_1_REGISTER)
REGDEF_BIT(RS0_CL3_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL3_SCA_HSTEP_OFT,        15)
REGDEF_END(RES0_COL3_SIZE_1_REGISTER)


/*
    RS0_CL3_OUT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL3_OUT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL3_SIZE_2_REGISTER_OFS 0x0664
REGDEF_BEGIN(RES0_COL3_SIZE_2_REGISTER)
REGDEF_BIT(RS0_CL3_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL3_OUT_WIDTH,        13)
REGDEF_END(RES0_COL3_SIZE_2_REGISTER)


/*
    RS0_CL3_RLT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL3_RLT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL3_SIZE_3_REGISTER_OFS 0x0668
REGDEF_BEGIN(RES0_COL3_SIZE_3_REGISTER)
REGDEF_BIT(RS0_CL3_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL3_RLT_WIDTH,        13)
REGDEF_END(RES0_COL3_SIZE_3_REGISTER)


/*
    RS0_CL3_PIP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL3_PIP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL3_SIZE_4_REGISTER_OFS 0x066c
REGDEF_BEGIN(RES0_COL3_SIZE_4_REGISTER)
REGDEF_BIT(RS0_CL3_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL3_PIP_WIDTH,        13)
REGDEF_END(RES0_COL3_SIZE_4_REGISTER)


/*
    RS0_CL3_SCA_CROP_X_START:    [0x0, 0xfff],          bits : 11_0
    RS0_CL3_SCA_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES0_COL3_SIZE_5_REGISTER_OFS 0x0670
REGDEF_BEGIN(RES0_COL3_SIZE_5_REGISTER)
REGDEF_BIT(RS0_CL3_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RS0_CL3_SCA_CROP_WIDTH,        13)
REGDEF_END(RES0_COL3_SIZE_5_REGISTER)


/*
    RS0_CL3_TC_CROP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL3_TC_CROP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
    RS0_CL3_TC_CROP_SKIP   :    [0x0, 0x1],         bits : 31
*/
#define RES0_COL3_SIZE_6_REGISTER_OFS 0x0674
REGDEF_BEGIN(RES0_COL3_SIZE_6_REGISTER)
REGDEF_BIT(RS0_CL3_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL3_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RS0_CL3_TC_CROP_SKIP,        1)
REGDEF_END(RES0_COL3_SIZE_6_REGISTER)


/*
    VPE_Reserved_263:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_263_OFS 0x0678
REGDEF_BEGIN(VPE_RESERVED_263)
REGDEF_BIT(VPE_Reserved_263,        32)
REGDEF_END(VPE_RESERVED_263)


/*
    VPE_Reserved_264:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_264_OFS 0x067c
REGDEF_BEGIN(VPE_RESERVED_264)
REGDEF_BIT(VPE_Reserved_264,        32)
REGDEF_END(VPE_RESERVED_264)


/*
    RS0_CL4_SCA_WIDTH    :    [0x0, 0x1fff],            bits : 12_0
    RS0_CL4_SCA_HSTEP_OFT:    [0x0, 0x7fff],            bits : 30_16
*/
#define RES0_COL4_SIZE_1_REGISTER_OFS 0x0680
REGDEF_BEGIN(RES0_COL4_SIZE_1_REGISTER)
REGDEF_BIT(RS0_CL4_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL4_SCA_HSTEP_OFT,        15)
REGDEF_END(RES0_COL4_SIZE_1_REGISTER)


/*
    RS0_CL4_OUT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL4_OUT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL4_SIZE_2_REGISTER_OFS 0x0684
REGDEF_BEGIN(RES0_COL4_SIZE_2_REGISTER)
REGDEF_BIT(RS0_CL4_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL4_OUT_WIDTH,        13)
REGDEF_END(RES0_COL4_SIZE_2_REGISTER)


/*
    RS0_CL4_RLT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL4_RLT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL4_SIZE_3_REGISTER_OFS 0x0688
REGDEF_BEGIN(RES0_COL4_SIZE_3_REGISTER)
REGDEF_BIT(RS0_CL4_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL4_RLT_WIDTH,        13)
REGDEF_END(RES0_COL4_SIZE_3_REGISTER)


/*
    RS0_CL4_PIP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL4_PIP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL4_SIZE_4_REGISTER_OFS 0x068c
REGDEF_BEGIN(RES0_COL4_SIZE_4_REGISTER)
REGDEF_BIT(RS0_CL4_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL4_PIP_WIDTH,        13)
REGDEF_END(RES0_COL4_SIZE_4_REGISTER)


/*
    RS0_CL4_SCA_CROP_X_START:    [0x0, 0xfff],          bits : 11_0
    RS0_CL4_SCA_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES0_COL4_SIZE_5_REGISTER_OFS 0x0690
REGDEF_BEGIN(RES0_COL4_SIZE_5_REGISTER)
REGDEF_BIT(RS0_CL4_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RS0_CL4_SCA_CROP_WIDTH,        13)
REGDEF_END(RES0_COL4_SIZE_5_REGISTER)


/*
    RS0_CL4_TC_CROP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL4_TC_CROP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
    RS0_CL4_TC_CROP_SKIP   :    [0x0, 0x1],         bits : 31
*/
#define RES0_COL4_SIZE_6_REGISTER_OFS 0x0694
REGDEF_BEGIN(RES0_COL4_SIZE_6_REGISTER)
REGDEF_BIT(RS0_CL4_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL4_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RS0_CL4_TC_CROP_SKIP,        1)
REGDEF_END(RES0_COL4_SIZE_6_REGISTER)


/*
    VPE_Reserved_265:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_265_OFS 0x0698
REGDEF_BEGIN(VPE_RESERVED_265)
REGDEF_BIT(VPE_Reserved_265,        32)
REGDEF_END(VPE_RESERVED_265)


/*
    VPE_Reserved_266:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_266_OFS 0x069c
REGDEF_BEGIN(VPE_RESERVED_266)
REGDEF_BIT(VPE_Reserved_266,        32)
REGDEF_END(VPE_RESERVED_266)


/*
    RS0_CL5_SCA_WIDTH    :    [0x0, 0x1fff],            bits : 12_0
    RS0_CL5_SCA_HSTEP_OFT:    [0x0, 0x7fff],            bits : 30_16
*/
#define RES0_COL5_SIZE_1_REGISTER_OFS 0x06a0
REGDEF_BEGIN(RES0_COL5_SIZE_1_REGISTER)
REGDEF_BIT(RS0_CL5_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL5_SCA_HSTEP_OFT,        15)
REGDEF_END(RES0_COL5_SIZE_1_REGISTER)


/*
    RS0_CL5_OUT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL5_OUT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL5_SIZE_2_REGISTER_OFS 0x06a4
REGDEF_BEGIN(RES0_COL5_SIZE_2_REGISTER)
REGDEF_BIT(RS0_CL5_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL5_OUT_WIDTH,        13)
REGDEF_END(RES0_COL5_SIZE_2_REGISTER)


/*
    RS0_CL5_RLT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL5_RLT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL5_SIZE_3_REGISTER_OFS 0x06a8
REGDEF_BEGIN(RES0_COL5_SIZE_3_REGISTER)
REGDEF_BIT(RS0_CL5_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL5_RLT_WIDTH,        13)
REGDEF_END(RES0_COL5_SIZE_3_REGISTER)


/*
    RS0_CL5_PIP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL5_PIP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL5_SIZE_4_REGISTER_OFS 0x06ac
REGDEF_BEGIN(RES0_COL5_SIZE_4_REGISTER)
REGDEF_BIT(RS0_CL5_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL5_PIP_WIDTH,        13)
REGDEF_END(RES0_COL5_SIZE_4_REGISTER)


/*
    RS0_CL5_SCA_CROP_X_START:    [0x0, 0xfff],          bits : 11_0
    RS0_CL5_SCA_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES0_COL5_SIZE_5_REGISTER_OFS 0x06b0
REGDEF_BEGIN(RES0_COL5_SIZE_5_REGISTER)
REGDEF_BIT(RS0_CL5_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RS0_CL5_SCA_CROP_WIDTH,        13)
REGDEF_END(RES0_COL5_SIZE_5_REGISTER)


/*
    RS0_CL5_TC_CROP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL5_TC_CROP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
    RS0_CL5_TC_CROP_SKIP   :    [0x0, 0x1],         bits : 31
*/
#define RES0_COL5_SIZE_6_REGISTER_OFS 0x06b4
REGDEF_BEGIN(RES0_COL5_SIZE_6_REGISTER)
REGDEF_BIT(RS0_CL5_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL5_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RS0_CL5_TC_CROP_SKIP,        1)
REGDEF_END(RES0_COL5_SIZE_6_REGISTER)


/*
    VPE_Reserved_267:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_267_OFS 0x06b8
REGDEF_BEGIN(VPE_RESERVED_267)
REGDEF_BIT(VPE_Reserved_267,        32)
REGDEF_END(VPE_RESERVED_267)


/*
    VPE_Reserved_268:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_268_OFS 0x06bc
REGDEF_BEGIN(VPE_RESERVED_268)
REGDEF_BIT(VPE_Reserved_268,        32)
REGDEF_END(VPE_RESERVED_268)


/*
    RS0_CL6_SCA_WIDTH    :    [0x0, 0x1fff],            bits : 12_0
    RS0_CL6_SCA_HSTEP_OFT:    [0x0, 0x7fff],            bits : 30_16
*/
#define RES0_COL6_SIZE_1_REGISTER_OFS 0x06c0
REGDEF_BEGIN(RES0_COL6_SIZE_1_REGISTER)
REGDEF_BIT(RS0_CL6_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL6_SCA_HSTEP_OFT,        15)
REGDEF_END(RES0_COL6_SIZE_1_REGISTER)


/*
    RS0_CL6_OUT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL6_OUT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL6_SIZE_2_REGISTER_OFS 0x06c4
REGDEF_BEGIN(RES0_COL6_SIZE_2_REGISTER)
REGDEF_BIT(RS0_CL6_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL6_OUT_WIDTH,        13)
REGDEF_END(RES0_COL6_SIZE_2_REGISTER)


/*
    RS0_CL6_RLT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL6_RLT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL6_SIZE_3_REGISTER_OFS 0x06c8
REGDEF_BEGIN(RES0_COL6_SIZE_3_REGISTER)
REGDEF_BIT(RS0_CL6_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL6_RLT_WIDTH,        13)
REGDEF_END(RES0_COL6_SIZE_3_REGISTER)


/*
    RS0_CL6_PIP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL6_PIP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL6_SIZE_4_REGISTER_OFS 0x06cc
REGDEF_BEGIN(RES0_COL6_SIZE_4_REGISTER)
REGDEF_BIT(RS0_CL6_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL6_PIP_WIDTH,        13)
REGDEF_END(RES0_COL6_SIZE_4_REGISTER)


/*
    RS0_CL6_SCA_CROP_X_START:    [0x0, 0xfff],          bits : 11_0
    RS0_CL6_SCA_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES0_COL6_SIZE_5_REGISTER_OFS 0x06d0
REGDEF_BEGIN(RES0_COL6_SIZE_5_REGISTER)
REGDEF_BIT(RS0_CL6_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RS0_CL6_SCA_CROP_WIDTH,        13)
REGDEF_END(RES0_COL6_SIZE_5_REGISTER)


/*
    RS0_CL6_TC_CROP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL6_TC_CROP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
    RS0_CL6_TC_CROP_SKIP   :    [0x0, 0x1],         bits : 31
*/
#define RES0_COL6_SIZE_6_REGISTER_OFS 0x06d4
REGDEF_BEGIN(RES0_COL6_SIZE_6_REGISTER)
REGDEF_BIT(RS0_CL6_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL6_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RS0_CL6_TC_CROP_SKIP,        1)
REGDEF_END(RES0_COL6_SIZE_6_REGISTER)


/*
    VPE_Reserved_269:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_269_OFS 0x06d8
REGDEF_BEGIN(VPE_RESERVED_269)
REGDEF_BIT(VPE_Reserved_269,        32)
REGDEF_END(VPE_RESERVED_269)


/*
    VPE_Reserved_270:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_270_OFS 0x06dc
REGDEF_BEGIN(VPE_RESERVED_270)
REGDEF_BIT(VPE_Reserved_270,        32)
REGDEF_END(VPE_RESERVED_270)


/*
    RS0_CL7_SCA_WIDTH    :    [0x0, 0x1fff],            bits : 12_0
    RS0_CL7_SCA_HSTEP_OFT:    [0x0, 0x7fff],            bits : 30_16
*/
#define RES0_COL7_SIZE_1_REGISTER_OFS 0x06e0
REGDEF_BEGIN(RES0_COL7_SIZE_1_REGISTER)
REGDEF_BIT(RS0_CL7_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL7_SCA_HSTEP_OFT,        15)
REGDEF_END(RES0_COL7_SIZE_1_REGISTER)


/*
    RS0_CL7_OUT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL7_OUT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL7_SIZE_2_REGISTER_OFS 0x06e4
REGDEF_BEGIN(RES0_COL7_SIZE_2_REGISTER)
REGDEF_BIT(RS0_CL7_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL7_OUT_WIDTH,        13)
REGDEF_END(RES0_COL7_SIZE_2_REGISTER)


/*
    RS0_CL7_RLT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL7_RLT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL7_SIZE_3_REGISTER_OFS 0x06e8
REGDEF_BEGIN(RES0_COL7_SIZE_3_REGISTER)
REGDEF_BIT(RS0_CL7_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL7_RLT_WIDTH,        13)
REGDEF_END(RES0_COL7_SIZE_3_REGISTER)


/*
    RS0_CL7_PIP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL7_PIP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES0_COL7_SIZE_4_REGISTER_OFS 0x06ec
REGDEF_BEGIN(RES0_COL7_SIZE_4_REGISTER)
REGDEF_BIT(RS0_CL7_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL7_PIP_WIDTH,        13)
REGDEF_END(RES0_COL7_SIZE_4_REGISTER)


/*
    RS0_CL7_SCA_CROP_X_START:    [0x0, 0xfff],          bits : 11_0
    RS0_CL7_SCA_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES0_COL7_SIZE_5_REGISTER_OFS 0x06f0
REGDEF_BEGIN(RES0_COL7_SIZE_5_REGISTER)
REGDEF_BIT(RS0_CL7_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RS0_CL7_SCA_CROP_WIDTH,        13)
REGDEF_END(RES0_COL7_SIZE_5_REGISTER)


/*
    RS0_CL7_TC_CROP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS0_CL7_TC_CROP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
    RS0_CL7_TC_CROP_SKIP   :    [0x0, 0x1],         bits : 31
*/
#define RES0_COL7_SIZE_6_REGISTER_OFS 0x06f4
REGDEF_BEGIN(RES0_COL7_SIZE_6_REGISTER)
REGDEF_BIT(RS0_CL7_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS0_CL7_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RS0_CL7_TC_CROP_SKIP,        1)
REGDEF_END(RES0_COL7_SIZE_6_REGISTER)


/*
    VPE_Reserved_271:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_271_OFS 0x06f8
REGDEF_BEGIN(VPE_RESERVED_271)
REGDEF_BIT(VPE_Reserved_271,        32)
REGDEF_END(VPE_RESERVED_271)


/*
    VPE_Reserved_272:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_272_OFS 0x06fc
REGDEF_BEGIN(VPE_RESERVED_272)
REGDEF_BIT(VPE_Reserved_272,        32)
REGDEF_END(VPE_RESERVED_272)


/*
    RS1_CL0_SCA_WIDTH    :    [0x0, 0x1fff],            bits : 12_0
    RS1_CL0_SCA_HSTEP_OFT:    [0x0, 0x7fff],            bits : 30_16
*/
#define RES1_COL0_SIZE_1_REGISTER_OFS 0x0700
REGDEF_BEGIN(RES1_COL0_SIZE_1_REGISTER)
REGDEF_BIT(RS1_CL0_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL0_SCA_HSTEP_OFT,        15)
REGDEF_END(RES1_COL0_SIZE_1_REGISTER)


/*
    RS1_CL0_OUT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL0_OUT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL0_SIZE_2_REGISTER_OFS 0x0704
REGDEF_BEGIN(RES1_COL0_SIZE_2_REGISTER)
REGDEF_BIT(RS1_CL0_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL0_OUT_WIDTH,        13)
REGDEF_END(RES1_COL0_SIZE_2_REGISTER)


/*
    RS1_CL0_RLT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL0_RLT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL0_SIZE_3_REGISTER_OFS 0x0708
REGDEF_BEGIN(RES1_COL0_SIZE_3_REGISTER)
REGDEF_BIT(RS1_CL0_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL0_RLT_WIDTH,        13)
REGDEF_END(RES1_COL0_SIZE_3_REGISTER)


/*
    RS1_CL0_PIP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL0_PIP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL0_SIZE_4_REGISTER_OFS 0x070c
REGDEF_BEGIN(RES1_COL0_SIZE_4_REGISTER)
REGDEF_BIT(RS1_CL0_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL0_PIP_WIDTH,        13)
REGDEF_END(RES1_COL0_SIZE_4_REGISTER)


/*
    RS1_CL0_SCA_CROP_X_START:    [0x0, 0xfff],          bits : 11_0
    RS1_CL0_SCA_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES1_COL0_SIZE_5_REGISTER_OFS 0x0710
REGDEF_BEGIN(RES1_COL0_SIZE_5_REGISTER)
REGDEF_BIT(RS1_CL0_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RS1_CL0_SCA_CROP_WIDTH,        13)
REGDEF_END(RES1_COL0_SIZE_5_REGISTER)


/*
    RS1_CL0_TC_CROP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL0_TC_CROP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
    RS1_CL0_TC_CROP_SKIP   :    [0x0, 0x1],         bits : 31
*/
#define RES1_COL0_SIZE_6_REGISTER_OFS 0x0714
REGDEF_BEGIN(RES1_COL0_SIZE_6_REGISTER)
REGDEF_BIT(RS1_CL0_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL0_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RS1_CL0_TC_CROP_SKIP,        1)
REGDEF_END(RES1_COL0_SIZE_6_REGISTER)


/*
    VPE_Reserved_273:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_273_OFS 0x0718
REGDEF_BEGIN(VPE_RESERVED_273)
REGDEF_BIT(VPE_Reserved_273,        32)
REGDEF_END(VPE_RESERVED_273)


/*
    VPE_Reserved_274:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_274_OFS 0x071c
REGDEF_BEGIN(VPE_RESERVED_274)
REGDEF_BIT(VPE_Reserved_274,        32)
REGDEF_END(VPE_RESERVED_274)


/*
    RS1_CL1_SCA_WIDTH    :    [0x0, 0x1fff],            bits : 12_0
    RS1_CL1_SCA_HSTEP_OFT:    [0x0, 0x7fff],            bits : 30_16
*/
#define RES1_COL1_SIZE_1_REGISTER_OFS 0x0720
REGDEF_BEGIN(RES1_COL1_SIZE_1_REGISTER)
REGDEF_BIT(RS1_CL1_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL1_SCA_HSTEP_OFT,        15)
REGDEF_END(RES1_COL1_SIZE_1_REGISTER)


/*
    RS1_CL1_OUT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL1_OUT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL1_SIZE_2_REGISTER_OFS 0x0724
REGDEF_BEGIN(RES1_COL1_SIZE_2_REGISTER)
REGDEF_BIT(RS1_CL1_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL1_OUT_WIDTH,        13)
REGDEF_END(RES1_COL1_SIZE_2_REGISTER)


/*
    RS1_CL1_RLT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL1_RLT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL1_SIZE_3_REGISTER_OFS 0x0728
REGDEF_BEGIN(RES1_COL1_SIZE_3_REGISTER)
REGDEF_BIT(RS1_CL1_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL1_RLT_WIDTH,        13)
REGDEF_END(RES1_COL1_SIZE_3_REGISTER)


/*
    RS1_CL1_PIP_X_START      :    [0x0, 0x1fff],            bits : 12_0
    RS1_CL1_RS1_CL1_PIP_WIDTH:    [0x0, 0x1fff],            bits : 28_16
*/
#define RES1_COL1_SIZE_4_REGISTER_OFS 0x072c
REGDEF_BEGIN(RES1_COL1_SIZE_4_REGISTER)
REGDEF_BIT(RS1_CL1_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL1_RS1_CL1_PIP_WIDTH,        13)
REGDEF_END(RES1_COL1_SIZE_4_REGISTER)


/*
    RS1_CL1_SCA_CROP_X_START:    [0x0, 0xfff],          bits : 11_0
    RS1_CL1_SCA_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES1_COL1_SIZE_5_REGISTER_OFS 0x0730
REGDEF_BEGIN(RES1_COL1_SIZE_5_REGISTER)
REGDEF_BIT(RS1_CL1_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RS1_CL1_SCA_CROP_WIDTH,        13)
REGDEF_END(RES1_COL1_SIZE_5_REGISTER)


/*
    RS1_CL1_TC_CROP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL1_TC_CROP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
    RS1_CL1_TC_CROP_SKIP   :    [0x0, 0x1],         bits : 31
*/
#define RES1_COL1_SIZE_6_REGISTER_OFS 0x0734
REGDEF_BEGIN(RES1_COL1_SIZE_6_REGISTER)
REGDEF_BIT(RS1_CL1_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL1_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RS1_CL1_TC_CROP_SKIP,        1)
REGDEF_END(RES1_COL1_SIZE_6_REGISTER)


/*
    VPE_Reserved_275:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_275_OFS 0x0738
REGDEF_BEGIN(VPE_RESERVED_275)
REGDEF_BIT(VPE_Reserved_275,        32)
REGDEF_END(VPE_RESERVED_275)


/*
    VPE_Reserved_276:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_276_OFS 0x073c
REGDEF_BEGIN(VPE_RESERVED_276)
REGDEF_BIT(VPE_Reserved_276,        32)
REGDEF_END(VPE_RESERVED_276)


/*
    RS1_CL2_SCA_WIDTH    :    [0x0, 0x1fff],            bits : 12_0
    RS1_CL2_SCA_HSTEP_OFT:    [0x0, 0x7fff],            bits : 30_16
*/
#define RES1_COL2_SIZE_1_REGISTER_OFS 0x0740
REGDEF_BEGIN(RES1_COL2_SIZE_1_REGISTER)
REGDEF_BIT(RS1_CL2_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL2_SCA_HSTEP_OFT,        15)
REGDEF_END(RES1_COL2_SIZE_1_REGISTER)


/*
    RS1_CL2_OUT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL2_OUT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL2_SIZE_2_REGISTER_OFS 0x0744
REGDEF_BEGIN(RES1_COL2_SIZE_2_REGISTER)
REGDEF_BIT(RS1_CL2_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL2_OUT_WIDTH,        13)
REGDEF_END(RES1_COL2_SIZE_2_REGISTER)


/*
    RS1_CL2_RLT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL2_RLT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL2_SIZE_3_REGISTER_OFS 0x0748
REGDEF_BEGIN(RES1_COL2_SIZE_3_REGISTER)
REGDEF_BIT(RS1_CL2_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL2_RLT_WIDTH,        13)
REGDEF_END(RES1_COL2_SIZE_3_REGISTER)


/*
    RS1_CL2_PIP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL2_PIP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL2_SIZE_4_REGISTER_OFS 0x074c
REGDEF_BEGIN(RES1_COL2_SIZE_4_REGISTER)
REGDEF_BIT(RS1_CL2_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL2_PIP_WIDTH,        13)
REGDEF_END(RES1_COL2_SIZE_4_REGISTER)


/*
    RS1_CL2_SCA_CROP_X_START:    [0x0, 0xfff],          bits : 11_0
    RS1_CL2_SCA_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES1_COL2_SIZE_5_REGISTER_OFS 0x0750
REGDEF_BEGIN(RES1_COL2_SIZE_5_REGISTER)
REGDEF_BIT(RS1_CL2_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RS1_CL2_SCA_CROP_WIDTH,        13)
REGDEF_END(RES1_COL2_SIZE_5_REGISTER)


/*
    RS1_CL2_TC_CROP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL2_TC_CROP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
    RS1_CL2_TC_CROP_SKIP   :    [0x0, 0x1],         bits : 31
*/
#define RES1_COL2_SIZE_6_REGISTER_OFS 0x0754
REGDEF_BEGIN(RES1_COL2_SIZE_6_REGISTER)
REGDEF_BIT(RS1_CL2_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL2_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RS1_CL2_TC_CROP_SKIP,        1)
REGDEF_END(RES1_COL2_SIZE_6_REGISTER)


/*
    VPE_Reserved_277:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_277_OFS 0x0758
REGDEF_BEGIN(VPE_RESERVED_277)
REGDEF_BIT(VPE_Reserved_277,        32)
REGDEF_END(VPE_RESERVED_277)


/*
    VPE_Reserved_278:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_278_OFS 0x075c
REGDEF_BEGIN(VPE_RESERVED_278)
REGDEF_BIT(VPE_Reserved_278,        32)
REGDEF_END(VPE_RESERVED_278)


/*
    RS1_CL3_SCA_WIDTH    :    [0x0, 0x1fff],            bits : 12_0
    RS1_CL3_SCA_HSTEP_OFT:    [0x0, 0x7fff],            bits : 30_16
*/
#define RES1_COL3_SIZE_1_REGISTER_OFS 0x0760
REGDEF_BEGIN(RES1_COL3_SIZE_1_REGISTER)
REGDEF_BIT(RS1_CL3_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL3_SCA_HSTEP_OFT,        15)
REGDEF_END(RES1_COL3_SIZE_1_REGISTER)


/*
    RS1_CL3_OUT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL3_OUT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL3_SIZE_2_REGISTER_OFS 0x0764
REGDEF_BEGIN(RES1_COL3_SIZE_2_REGISTER)
REGDEF_BIT(RS1_CL3_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL3_OUT_WIDTH,        13)
REGDEF_END(RES1_COL3_SIZE_2_REGISTER)


/*
    RS1_CL3_RLT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL3_RLT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL3_SIZE_3_REGISTER_OFS 0x0768
REGDEF_BEGIN(RES1_COL3_SIZE_3_REGISTER)
REGDEF_BIT(RS1_CL3_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL3_RLT_WIDTH,        13)
REGDEF_END(RES1_COL3_SIZE_3_REGISTER)


/*
    RS1_CL3_PIP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL3_PIP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL3_SIZE_4_REGISTER_OFS 0x076c
REGDEF_BEGIN(RES1_COL3_SIZE_4_REGISTER)
REGDEF_BIT(RS1_CL3_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL3_PIP_WIDTH,        13)
REGDEF_END(RES1_COL3_SIZE_4_REGISTER)


/*
    RS1_CL3_SCA_CROP_X_START:    [0x0, 0xfff],          bits : 11_0
    RS1_CL3_SCA_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES1_COL3_SIZE_5_REGISTER_OFS 0x0770
REGDEF_BEGIN(RES1_COL3_SIZE_5_REGISTER)
REGDEF_BIT(RS1_CL3_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RS1_CL3_SCA_CROP_WIDTH,        13)
REGDEF_END(RES1_COL3_SIZE_5_REGISTER)


/*
    RS1_CL3_TC_CROP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL3_TC_CROP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
    RS1_CL3_TC_CROP_SKIP   :    [0x0, 0x1],         bits : 31
*/
#define RES1_COL3_SIZE_6_REGISTER_OFS 0x0774
REGDEF_BEGIN(RES1_COL3_SIZE_6_REGISTER)
REGDEF_BIT(RS1_CL3_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL3_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RS1_CL3_TC_CROP_SKIP,        1)
REGDEF_END(RES1_COL3_SIZE_6_REGISTER)


/*
    VPE_Reserved_279:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_279_OFS 0x0778
REGDEF_BEGIN(VPE_RESERVED_279)
REGDEF_BIT(VPE_Reserved_279,        32)
REGDEF_END(VPE_RESERVED_279)


/*
    VPE_Reserved_280:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_280_OFS 0x077c
REGDEF_BEGIN(VPE_RESERVED_280)
REGDEF_BIT(VPE_Reserved_280,        32)
REGDEF_END(VPE_RESERVED_280)


/*
    RS1_CL4_SCA_WIDTH    :    [0x0, 0x1fff],            bits : 12_0
    RS1_CL4_SCA_HSTEP_OFT:    [0x0, 0x7fff],            bits : 30_16
*/
#define RES1_COL4_SIZE_1_REGISTER_OFS 0x0780
REGDEF_BEGIN(RES1_COL4_SIZE_1_REGISTER)
REGDEF_BIT(RS1_CL4_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL4_SCA_HSTEP_OFT,        15)
REGDEF_END(RES1_COL4_SIZE_1_REGISTER)


/*
    RS1_CL4_OUT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL4_OUT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL4_SIZE_2_REGISTER_OFS 0x0784
REGDEF_BEGIN(RES1_COL4_SIZE_2_REGISTER)
REGDEF_BIT(RS1_CL4_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL4_OUT_WIDTH,        13)
REGDEF_END(RES1_COL4_SIZE_2_REGISTER)


/*
    RS1_CL4_RLT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL4_RLT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL4_SIZE_3_REGISTER_OFS 0x0788
REGDEF_BEGIN(RES1_COL4_SIZE_3_REGISTER)
REGDEF_BIT(RS1_CL4_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL4_RLT_WIDTH,        13)
REGDEF_END(RES1_COL4_SIZE_3_REGISTER)


/*
    RS1_CL4_PIP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL4_PIP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL4_SIZE_4_REGISTER_OFS 0x078c
REGDEF_BEGIN(RES1_COL4_SIZE_4_REGISTER)
REGDEF_BIT(RS1_CL4_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL4_PIP_WIDTH,        13)
REGDEF_END(RES1_COL4_SIZE_4_REGISTER)


/*
    RS1_CL4_SCA_CROP_X_START:    [0x0, 0xfff],          bits : 11_0
    RS1_CL4_SCA_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES1_COL4_SIZE_5_REGISTER_OFS 0x0790
REGDEF_BEGIN(RES1_COL4_SIZE_5_REGISTER)
REGDEF_BIT(RS1_CL4_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RS1_CL4_SCA_CROP_WIDTH,        13)
REGDEF_END(RES1_COL4_SIZE_5_REGISTER)


/*
    RS1_CL4_TC_CROP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL4_TC_CROP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
    RS1_CL4_TC_CROP_SKIP   :    [0x0, 0x1],         bits : 31
*/
#define RES1_COL4_SIZE_6_REGISTER_OFS 0x0794
REGDEF_BEGIN(RES1_COL4_SIZE_6_REGISTER)
REGDEF_BIT(RS1_CL4_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL4_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RS1_CL4_TC_CROP_SKIP,        1)
REGDEF_END(RES1_COL4_SIZE_6_REGISTER)


/*
    VPE_Reserved_281:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_281_OFS 0x0798
REGDEF_BEGIN(VPE_RESERVED_281)
REGDEF_BIT(VPE_Reserved_281,        32)
REGDEF_END(VPE_RESERVED_281)


/*
    VPE_Reserved_282:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_282_OFS 0x079c
REGDEF_BEGIN(VPE_RESERVED_282)
REGDEF_BIT(VPE_Reserved_282,        32)
REGDEF_END(VPE_RESERVED_282)


/*
    RS1_CL5_SCA_WIDTH    :    [0x0, 0x1fff],            bits : 12_0
    RS1_CL5_SCA_HSTEP_OFT:    [0x0, 0x7fff],            bits : 30_16
*/
#define RES1_COL5_SIZE_1_REGISTER_OFS 0x07a0
REGDEF_BEGIN(RES1_COL5_SIZE_1_REGISTER)
REGDEF_BIT(RS1_CL5_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL5_SCA_HSTEP_OFT,        15)
REGDEF_END(RES1_COL5_SIZE_1_REGISTER)


/*
    RS1_CL5_OUT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL5_OUT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL5_SIZE_2_REGISTER_OFS 0x07a4
REGDEF_BEGIN(RES1_COL5_SIZE_2_REGISTER)
REGDEF_BIT(RS1_CL5_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL5_OUT_WIDTH,        13)
REGDEF_END(RES1_COL5_SIZE_2_REGISTER)


/*
    RS1_CL5_RLT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL5_RLT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL5_SIZE_3_REGISTER_OFS 0x07a8
REGDEF_BEGIN(RES1_COL5_SIZE_3_REGISTER)
REGDEF_BIT(RS1_CL5_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL5_RLT_WIDTH,        13)
REGDEF_END(RES1_COL5_SIZE_3_REGISTER)


/*
    RS1_CL5_PIP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL5_PIP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL5_SIZE_4_REGISTER_OFS 0x07ac
REGDEF_BEGIN(RES1_COL5_SIZE_4_REGISTER)
REGDEF_BIT(RS1_CL5_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL5_PIP_WIDTH,        13)
REGDEF_END(RES1_COL5_SIZE_4_REGISTER)


/*
    RS1_CL5_SCA_CROP_X_START:    [0x0, 0xfff],          bits : 11_0
    RS1_CL5_SCA_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES1_COL5_SIZE_5_REGISTER_OFS 0x07b0
REGDEF_BEGIN(RES1_COL5_SIZE_5_REGISTER)
REGDEF_BIT(RS1_CL5_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RS1_CL5_SCA_CROP_WIDTH,        13)
REGDEF_END(RES1_COL5_SIZE_5_REGISTER)


/*
    RS1_CL5_TC_CROP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL5_TC_CROP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
    RS1_CL5_TC_CROP_SKIP   :    [0x0, 0x1],         bits : 31
*/
#define RES1_COL5_SIZE_6_REGISTER_OFS 0x07b4
REGDEF_BEGIN(RES1_COL5_SIZE_6_REGISTER)
REGDEF_BIT(RS1_CL5_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL5_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RS1_CL5_TC_CROP_SKIP,        1)
REGDEF_END(RES1_COL5_SIZE_6_REGISTER)


/*
    VPE_Reserved_283:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_283_OFS 0x07b8
REGDEF_BEGIN(VPE_RESERVED_283)
REGDEF_BIT(VPE_Reserved_283,        32)
REGDEF_END(VPE_RESERVED_283)


/*
    VPE_Reserved_284:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_284_OFS 0x07bc
REGDEF_BEGIN(VPE_RESERVED_284)
REGDEF_BIT(VPE_Reserved_284,        32)
REGDEF_END(VPE_RESERVED_284)


/*
    RS1_CL6_SCA_WIDTH    :    [0x0, 0x1fff],            bits : 12_0
    RS1_CL6_SCA_HSTEP_OFT:    [0x0, 0x7fff],            bits : 30_16
*/
#define RES1_COL6_SIZE_1_REGISTER_OFS 0x07c0
REGDEF_BEGIN(RES1_COL6_SIZE_1_REGISTER)
REGDEF_BIT(RS1_CL6_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL6_SCA_HSTEP_OFT,        15)
REGDEF_END(RES1_COL6_SIZE_1_REGISTER)


/*
    RS1_CL6_OUT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL6_OUT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL6_SIZE_2_REGISTER_OFS 0x07c4
REGDEF_BEGIN(RES1_COL6_SIZE_2_REGISTER)
REGDEF_BIT(RS1_CL6_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL6_OUT_WIDTH,        13)
REGDEF_END(RES1_COL6_SIZE_2_REGISTER)


/*
    RS1_CL6_RLT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL6_RLT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL6_SIZE_3_REGISTER_OFS 0x07c8
REGDEF_BEGIN(RES1_COL6_SIZE_3_REGISTER)
REGDEF_BIT(RS1_CL6_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL6_RLT_WIDTH,        13)
REGDEF_END(RES1_COL6_SIZE_3_REGISTER)


/*
    RS1_CL6_PIP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL6_PIP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL6_SIZE_4_REGISTER_OFS 0x07cc
REGDEF_BEGIN(RES1_COL6_SIZE_4_REGISTER)
REGDEF_BIT(RS1_CL6_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL6_PIP_WIDTH,        13)
REGDEF_END(RES1_COL6_SIZE_4_REGISTER)


/*
    RS1_CL6_SCA_CROP_X_START:    [0x0, 0xfff],          bits : 11_0
    RS1_CL6_SCA_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES1_COL6_SIZE_5_REGISTER_OFS 0x07d0
REGDEF_BEGIN(RES1_COL6_SIZE_5_REGISTER)
REGDEF_BIT(RS1_CL6_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RS1_CL6_SCA_CROP_WIDTH,        13)
REGDEF_END(RES1_COL6_SIZE_5_REGISTER)


/*
    RS1_CL6_TC_CROP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL6_TC_CROP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
    RS1_CL6_TC_CROP_SKIP   :    [0x0, 0x1],         bits : 31
*/
#define RES1_COL6_SIZE_6_REGISTER_OFS 0x07d4
REGDEF_BEGIN(RES1_COL6_SIZE_6_REGISTER)
REGDEF_BIT(RS1_CL6_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL6_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RS1_CL6_TC_CROP_SKIP,        1)
REGDEF_END(RES1_COL6_SIZE_6_REGISTER)


/*
    VPE_Reserved_285:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_285_OFS 0x07d8
REGDEF_BEGIN(VPE_RESERVED_285)
REGDEF_BIT(VPE_Reserved_285,        32)
REGDEF_END(VPE_RESERVED_285)


/*
    VPE_Reserved_286:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_286_OFS 0x07dc
REGDEF_BEGIN(VPE_RESERVED_286)
REGDEF_BIT(VPE_Reserved_286,        32)
REGDEF_END(VPE_RESERVED_286)


/*
    RS1_CL7_SCA_WIDTH    :    [0x0, 0x1fff],            bits : 12_0
    RS1_CL7_SCA_HSTEP_OFT:    [0x0, 0x7fff],            bits : 30_16
*/
#define RES1_COL7_SIZE_1_REGISTER_OFS 0x07e0
REGDEF_BEGIN(RES1_COL7_SIZE_1_REGISTER)
REGDEF_BIT(RS1_CL7_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL7_SCA_HSTEP_OFT,        15)
REGDEF_END(RES1_COL7_SIZE_1_REGISTER)


/*
    RS1_CL7_OUT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL7_OUT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL7_SIZE_2_REGISTER_OFS 0x07e4
REGDEF_BEGIN(RES1_COL7_SIZE_2_REGISTER)
REGDEF_BIT(RS1_CL7_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL7_OUT_WIDTH,        13)
REGDEF_END(RES1_COL7_SIZE_2_REGISTER)


/*
    RS1_CL7_RLT_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL7_RLT_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL7_SIZE_3_REGISTER_OFS 0x07e8
REGDEF_BEGIN(RES1_COL7_SIZE_3_REGISTER)
REGDEF_BIT(RS1_CL7_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL7_RLT_WIDTH,        13)
REGDEF_END(RES1_COL7_SIZE_3_REGISTER)


/*
    RS1_CL7_PIP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL7_PIP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
*/
#define RES1_COL7_SIZE_4_REGISTER_OFS 0x07ec
REGDEF_BEGIN(RES1_COL7_SIZE_4_REGISTER)
REGDEF_BIT(RS1_CL7_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL7_PIP_WIDTH,        13)
REGDEF_END(RES1_COL7_SIZE_4_REGISTER)


/*
    RS1_CL7_SCA_CROP_X_START:    [0x0, 0xfff],          bits : 11_0
    RS1_CL7_SCA_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES1_COL7_SIZE_5_REGISTER_OFS 0x07f0
REGDEF_BEGIN(RES1_COL7_SIZE_5_REGISTER)
REGDEF_BIT(RS1_CL7_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RS1_CL7_SCA_CROP_WIDTH,        13)
REGDEF_END(RES1_COL7_SIZE_5_REGISTER)


/*
    RS1_CL7_TC_CROP_X_START:    [0x0, 0x1fff],          bits : 12_0
    RS1_CL7_TC_CROP_WIDTH  :    [0x0, 0x1fff],          bits : 28_16
    RS1_CL7_TC_CROP_SKIP   :    [0x0, 0x1],         bits : 31
*/
#define RES1_COL7_SIZE_6_REGISTER_OFS 0x07f4
REGDEF_BEGIN(RES1_COL7_SIZE_6_REGISTER)
REGDEF_BIT(RS1_CL7_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RS1_CL7_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RS1_CL7_TC_CROP_SKIP,        1)
REGDEF_END(RES1_COL7_SIZE_6_REGISTER)


/*
    VPE_Reserved_287:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_287_OFS 0x07f8
REGDEF_BEGIN(VPE_RESERVED_287)
REGDEF_BIT(VPE_Reserved_287,        32)
REGDEF_END(VPE_RESERVED_287)


/*
    VPE_Reserved_288:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_288_OFS 0x07fc
REGDEF_BEGIN(VPE_RESERVED_288)
REGDEF_BIT(VPE_Reserved_288,        32)
REGDEF_END(VPE_RESERVED_288)


/*
    CL0_PROC_WIDTH  :    [0x0, 0xfff],          bits : 11_0
    CL0_PROC_X_START:    [0x0, 0x1fff],         bits : 28_16
*/
#define COL0_SIZE_1_REGISTER_OFS 0x0800
REGDEF_BEGIN(COL0_SIZE_1_REGISTER)
REGDEF_BIT(CL0_PROC_WIDTH,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(CL0_PROC_X_START,        13)
REGDEF_END(COL0_SIZE_1_REGISTER)


/*
    VPE_Reserved_289:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_289_OFS 0x0804
REGDEF_BEGIN(VPE_RESERVED_289)
REGDEF_BIT(VPE_Reserved_289,        32)
REGDEF_END(VPE_RESERVED_289)


/*
    VPE_Reserved_290:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_290_OFS 0x0808
REGDEF_BEGIN(VPE_RESERVED_290)
REGDEF_BIT(VPE_Reserved_290,        32)
REGDEF_END(VPE_RESERVED_290)


/*
    VPE_Reserved_291:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_291_OFS 0x080c
REGDEF_BEGIN(VPE_RESERVED_291)
REGDEF_BIT(VPE_Reserved_291,        32)
REGDEF_END(VPE_RESERVED_291)


/*
    VPE_Reserved_292:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_292_OFS 0x0810
REGDEF_BEGIN(VPE_RESERVED_292)
REGDEF_BIT(VPE_Reserved_292,        32)
REGDEF_END(VPE_RESERVED_292)


/*
    VPE_Reserved_293:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_293_OFS 0x0814
REGDEF_BEGIN(VPE_RESERVED_293)
REGDEF_BIT(VPE_Reserved_293,        32)
REGDEF_END(VPE_RESERVED_293)


/*
    CL0_COL_X_START:    [0x0, 0x1fff],          bits : 12_0
*/
#define COL0_SIZE_7_REGISTER_OFS 0x0818
REGDEF_BEGIN(COL0_SIZE_7_REGISTER)
REGDEF_BIT(CL0_COL_X_START,        13)
REGDEF_END(COL0_SIZE_7_REGISTER)


/*
    VPE_Reserved_294:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_294_OFS 0x081c
REGDEF_BEGIN(VPE_RESERVED_294)
REGDEF_BIT(VPE_Reserved_294,        32)
REGDEF_END(VPE_RESERVED_294)


/*
    CL1_PROC_WIDTH  :    [0x0, 0xfff],          bits : 11_0
    CL1_PROC_X_START:    [0x0, 0x1fff],         bits : 28_16
*/
#define COL1_SIZE_1_REGISTER_OFS 0x0820
REGDEF_BEGIN(COL1_SIZE_1_REGISTER)
REGDEF_BIT(CL1_PROC_WIDTH,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(CL1_PROC_X_START,        13)
REGDEF_END(COL1_SIZE_1_REGISTER)


/*
    VPE_Reserved_295:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_295_OFS 0x0824
REGDEF_BEGIN(VPE_RESERVED_295)
REGDEF_BIT(VPE_Reserved_295,        32)
REGDEF_END(VPE_RESERVED_295)


/*
    VPE_Reserved_296:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_296_OFS 0x0828
REGDEF_BEGIN(VPE_RESERVED_296)
REGDEF_BIT(VPE_Reserved_296,        32)
REGDEF_END(VPE_RESERVED_296)


/*
    VPE_Reserved_297:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_297_OFS 0x082c
REGDEF_BEGIN(VPE_RESERVED_297)
REGDEF_BIT(VPE_Reserved_297,        32)
REGDEF_END(VPE_RESERVED_297)


/*
    VPE_Reserved_298:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_298_OFS 0x0830
REGDEF_BEGIN(VPE_RESERVED_298)
REGDEF_BIT(VPE_Reserved_298,        32)
REGDEF_END(VPE_RESERVED_298)


/*
    VPE_Reserved_299:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_299_OFS 0x0834
REGDEF_BEGIN(VPE_RESERVED_299)
REGDEF_BIT(VPE_Reserved_299,        32)
REGDEF_END(VPE_RESERVED_299)


/*
    CL1_COL_X_START:    [0x0, 0x1fff],          bits : 12_0
*/
#define COL1_SIZE_7_REGISTER_OFS 0x0838
REGDEF_BEGIN(COL1_SIZE_7_REGISTER)
REGDEF_BIT(CL1_COL_X_START,        13)
REGDEF_END(COL1_SIZE_7_REGISTER)


/*
    VPE_Reserved_300:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_300_OFS 0x083c
REGDEF_BEGIN(VPE_RESERVED_300)
REGDEF_BIT(VPE_Reserved_300,        32)
REGDEF_END(VPE_RESERVED_300)


/*
    CL2_PROC_WIDTH  :    [0x0, 0xfff],          bits : 11_0
    CL2_PROC_X_START:    [0x0, 0x1fff],         bits : 28_16
*/
#define COL2_SIZE_1_REGISTER_OFS 0x0840
REGDEF_BEGIN(COL2_SIZE_1_REGISTER)
REGDEF_BIT(CL2_PROC_WIDTH,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(CL2_PROC_X_START,        13)
REGDEF_END(COL2_SIZE_1_REGISTER)


/*
    VPE_Reserved_301:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_301_OFS 0x0844
REGDEF_BEGIN(VPE_RESERVED_301)
REGDEF_BIT(VPE_Reserved_301,        32)
REGDEF_END(VPE_RESERVED_301)


/*
    VPE_Reserved_302:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_302_OFS 0x0848
REGDEF_BEGIN(VPE_RESERVED_302)
REGDEF_BIT(VPE_Reserved_302,        32)
REGDEF_END(VPE_RESERVED_302)


/*
    VPE_Reserved_303:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_303_OFS 0x084c
REGDEF_BEGIN(VPE_RESERVED_303)
REGDEF_BIT(VPE_Reserved_303,        32)
REGDEF_END(VPE_RESERVED_303)


/*
    VPE_Reserved_304:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_304_OFS 0x0850
REGDEF_BEGIN(VPE_RESERVED_304)
REGDEF_BIT(VPE_Reserved_304,        32)
REGDEF_END(VPE_RESERVED_304)


/*
    VPE_Reserved_305:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_305_OFS 0x0854
REGDEF_BEGIN(VPE_RESERVED_305)
REGDEF_BIT(VPE_Reserved_305,        32)
REGDEF_END(VPE_RESERVED_305)


/*
    CL2_COL_X_START:    [0x0, 0x1fff],          bits : 12_0
*/
#define COL2_SIZE_7_REGISTER_OFS 0x0858
REGDEF_BEGIN(COL2_SIZE_7_REGISTER)
REGDEF_BIT(CL2_COL_X_START,        13)
REGDEF_END(COL2_SIZE_7_REGISTER)


/*
    VPE_Reserved_307:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_307_OFS 0x085c
REGDEF_BEGIN(VPE_RESERVED_307)
REGDEF_BIT(VPE_Reserved_307,        32)
REGDEF_END(VPE_RESERVED_307)


/*
    CL3_PROC_WIDTH  :    [0x0, 0xfff],          bits : 11_0
    CL3_PROC_X_START:    [0x0, 0x1fff],         bits : 28_16
*/
#define COL3_SIZE_1_REGISTER_OFS 0x0860
REGDEF_BEGIN(COL3_SIZE_1_REGISTER)
REGDEF_BIT(CL3_PROC_WIDTH,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(CL3_PROC_X_START,        13)
REGDEF_END(COL3_SIZE_1_REGISTER)


/*
    VPE_Reserved_308:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_308_OFS 0x0864
REGDEF_BEGIN(VPE_RESERVED_308)
REGDEF_BIT(VPE_Reserved_308,        32)
REGDEF_END(VPE_RESERVED_308)


/*
    VPE_Reserved_309:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_309_OFS 0x0868
REGDEF_BEGIN(VPE_RESERVED_309)
REGDEF_BIT(VPE_Reserved_309,        32)
REGDEF_END(VPE_RESERVED_309)


/*
    VPE_Reserved_310:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_310_OFS 0x086c
REGDEF_BEGIN(VPE_RESERVED_310)
REGDEF_BIT(VPE_Reserved_310,        32)
REGDEF_END(VPE_RESERVED_310)


/*
    VPE_Reserved_311:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_311_OFS 0x0870
REGDEF_BEGIN(VPE_RESERVED_311)
REGDEF_BIT(VPE_Reserved_311,        32)
REGDEF_END(VPE_RESERVED_311)


/*
    VPE_Reserved_312:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_312_OFS 0x0874
REGDEF_BEGIN(VPE_RESERVED_312)
REGDEF_BIT(VPE_Reserved_312,        32)
REGDEF_END(VPE_RESERVED_312)


/*
    CL3_COL_X_START:    [0x0, 0x1fff],          bits : 12_0
*/
#define COL3_SIZE_7_REGISTER_OFS 0x0878
REGDEF_BEGIN(COL3_SIZE_7_REGISTER)
REGDEF_BIT(CL3_COL_X_START,        13)
REGDEF_END(COL3_SIZE_7_REGISTER)


/*
    VPE_Reserved_313:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_313_OFS 0x087c
REGDEF_BEGIN(VPE_RESERVED_313)
REGDEF_BIT(VPE_Reserved_313,        32)
REGDEF_END(VPE_RESERVED_313)


/*
    CL4_PROC_WIDTH  :    [0x0, 0xfff],          bits : 11_0
    CL4_PROC_X_START:    [0x0, 0x1fff],         bits : 28_16
*/
#define COL4_SIZE_1_REGISTER_OFS 0x0880
REGDEF_BEGIN(COL4_SIZE_1_REGISTER)
REGDEF_BIT(CL4_PROC_WIDTH,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(CL4_PROC_X_START,        13)
REGDEF_END(COL4_SIZE_1_REGISTER)


/*
    VPE_Reserved_314:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_314_OFS 0x0884
REGDEF_BEGIN(VPE_RESERVED_314)
REGDEF_BIT(VPE_Reserved_314,        32)
REGDEF_END(VPE_RESERVED_314)


/*
    VPE_Reserved_315:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_315_OFS 0x0888
REGDEF_BEGIN(VPE_RESERVED_315)
REGDEF_BIT(VPE_Reserved_315,        32)
REGDEF_END(VPE_RESERVED_315)


/*
    VPE_Reserved_316:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_316_OFS 0x088c
REGDEF_BEGIN(VPE_RESERVED_316)
REGDEF_BIT(VPE_Reserved_316,        32)
REGDEF_END(VPE_RESERVED_316)


/*
    VPE_Reserved_317:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_317_OFS 0x0890
REGDEF_BEGIN(VPE_RESERVED_317)
REGDEF_BIT(VPE_Reserved_317,        32)
REGDEF_END(VPE_RESERVED_317)


/*
    VPE_Reserved_318:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_318_OFS 0x0894
REGDEF_BEGIN(VPE_RESERVED_318)
REGDEF_BIT(VPE_Reserved_318,        32)
REGDEF_END(VPE_RESERVED_318)


/*
    CL4_COL_X_START:    [0x0, 0x1fff],          bits : 12_0
*/
#define COL4_SIZE_7_REGISTER_OFS 0x0898
REGDEF_BEGIN(COL4_SIZE_7_REGISTER)
REGDEF_BIT(CL4_COL_X_START,        13)
REGDEF_END(COL4_SIZE_7_REGISTER)


/*
    VPE_Reserved_319:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_319_OFS 0x089c
REGDEF_BEGIN(VPE_RESERVED_319)
REGDEF_BIT(VPE_Reserved_319,        32)
REGDEF_END(VPE_RESERVED_319)


/*
    CL5_PROC_WIDTH  :    [0x0, 0xfff],          bits : 11_0
    CL5_PROC_X_START:    [0x0, 0x1fff],         bits : 28_16
*/
#define COL5_SIZE_1_REGISTER_OFS 0x08a0
REGDEF_BEGIN(COL5_SIZE_1_REGISTER)
REGDEF_BIT(CL5_PROC_WIDTH,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(CL5_PROC_X_START,        13)
REGDEF_END(COL5_SIZE_1_REGISTER)


/*
    VPE_Reserved_320:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_320_OFS 0x08a4
REGDEF_BEGIN(VPE_RESERVED_320)
REGDEF_BIT(VPE_Reserved_320,        32)
REGDEF_END(VPE_RESERVED_320)


/*
    VPE_Reserved_321:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_321_OFS 0x08a8
REGDEF_BEGIN(VPE_RESERVED_321)
REGDEF_BIT(VPE_Reserved_321,        32)
REGDEF_END(VPE_RESERVED_321)


/*
    VPE_Reserved_322:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_322_OFS 0x08ac
REGDEF_BEGIN(VPE_RESERVED_322)
REGDEF_BIT(VPE_Reserved_322,        32)
REGDEF_END(VPE_RESERVED_322)


/*
    VPE_Reserved_323:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_323_OFS 0x08b0
REGDEF_BEGIN(VPE_RESERVED_323)
REGDEF_BIT(VPE_Reserved_323,        32)
REGDEF_END(VPE_RESERVED_323)


/*
    VPE_Reserved_324:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_324_OFS 0x08b4
REGDEF_BEGIN(VPE_RESERVED_324)
REGDEF_BIT(VPE_Reserved_324,        32)
REGDEF_END(VPE_RESERVED_324)


/*
    CL5_COL_X_START:    [0x0, 0x1fff],          bits : 12_0
*/
#define COL5_SIZE_7_REGISTER_OFS 0x08b8
REGDEF_BEGIN(COL5_SIZE_7_REGISTER)
REGDEF_BIT(CL5_COL_X_START,        13)
REGDEF_END(COL5_SIZE_7_REGISTER)


/*
    VPE_Reserved_325:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_325_OFS 0x08bc
REGDEF_BEGIN(VPE_RESERVED_325)
REGDEF_BIT(VPE_Reserved_325,        32)
REGDEF_END(VPE_RESERVED_325)


/*
    CL6_PROC_WIDTH  :    [0x0, 0xfff],          bits : 11_0
    CL6_PROC_X_START:    [0x0, 0x1fff],         bits : 28_16
*/
#define COL6_SIZE_1_REGISTER_OFS 0x08c0
REGDEF_BEGIN(COL6_SIZE_1_REGISTER)
REGDEF_BIT(CL6_PROC_WIDTH,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(CL6_PROC_X_START,        13)
REGDEF_END(COL6_SIZE_1_REGISTER)


/*
    VPE_Reserved_326:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_326_OFS 0x08c4
REGDEF_BEGIN(VPE_RESERVED_326)
REGDEF_BIT(VPE_Reserved_326,        32)
REGDEF_END(VPE_RESERVED_326)


/*
    VPE_Reserved_327:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_327_OFS 0x08c8
REGDEF_BEGIN(VPE_RESERVED_327)
REGDEF_BIT(VPE_Reserved_327,        32)
REGDEF_END(VPE_RESERVED_327)


/*
    VPE_Reserved_328:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_328_OFS 0x08cc
REGDEF_BEGIN(VPE_RESERVED_328)
REGDEF_BIT(VPE_Reserved_328,        32)
REGDEF_END(VPE_RESERVED_328)


/*
    VPE_Reserved_329:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_329_OFS 0x08d0
REGDEF_BEGIN(VPE_RESERVED_329)
REGDEF_BIT(VPE_Reserved_329,        32)
REGDEF_END(VPE_RESERVED_329)


/*
    VPE_Reserved_330:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_330_OFS 0x08d4
REGDEF_BEGIN(VPE_RESERVED_330)
REGDEF_BIT(VPE_Reserved_330,        32)
REGDEF_END(VPE_RESERVED_330)


/*
    CL6_COL_X_START:    [0x0, 0x1fff],          bits : 12_0
*/
#define COL6_SIZE_7_REGISTER_OFS 0x08d8
REGDEF_BEGIN(COL6_SIZE_7_REGISTER)
REGDEF_BIT(CL6_COL_X_START,        13)
REGDEF_END(COL6_SIZE_7_REGISTER)


/*
    VPE_Reserved_331:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_331_OFS 0x08dc
REGDEF_BEGIN(VPE_RESERVED_331)
REGDEF_BIT(VPE_Reserved_331,        32)
REGDEF_END(VPE_RESERVED_331)


/*
    CL7_PROC_WIDTH  :    [0x0, 0xfff],          bits : 11_0
    CL7_PROC_X_START:    [0x0, 0x1fff],         bits : 28_16
*/
#define COL7_SIZE_1_REGISTER_OFS 0x08e0
REGDEF_BEGIN(COL7_SIZE_1_REGISTER)
REGDEF_BIT(CL7_PROC_WIDTH,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(CL7_PROC_X_START,        13)
REGDEF_END(COL7_SIZE_1_REGISTER)


/*
    VPE_Reserved_332:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_332_OFS 0x08e4
REGDEF_BEGIN(VPE_RESERVED_332)
REGDEF_BIT(VPE_Reserved_332,        32)
REGDEF_END(VPE_RESERVED_332)


/*
    VPE_Reserved_333:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_333_OFS 0x08e8
REGDEF_BEGIN(VPE_RESERVED_333)
REGDEF_BIT(VPE_Reserved_333,        32)
REGDEF_END(VPE_RESERVED_333)


/*
    VPE_Reserved_334:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_334_OFS 0x08ec
REGDEF_BEGIN(VPE_RESERVED_334)
REGDEF_BIT(VPE_Reserved_334,        32)
REGDEF_END(VPE_RESERVED_334)


/*
    VPE_Reserved_335:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_335_OFS 0x08f0
REGDEF_BEGIN(VPE_RESERVED_335)
REGDEF_BIT(VPE_Reserved_335,        32)
REGDEF_END(VPE_RESERVED_335)


/*
    VPE_Reserved_336:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_336_OFS 0x08f4
REGDEF_BEGIN(VPE_RESERVED_336)
REGDEF_BIT(VPE_Reserved_336,        32)
REGDEF_END(VPE_RESERVED_336)


/*
    CL7_COL_X_START:    [0x0, 0x1fff],          bits : 12_0
*/
#define COL7_SIZE_7_REGISTER_OFS 0x08f8
REGDEF_BEGIN(COL7_SIZE_7_REGISTER)
REGDEF_BIT(CL7_COL_X_START,        13)
REGDEF_END(COL7_SIZE_7_REGISTER)


/*
    VPE_Reserved_337:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_337_OFS 0x08fc
REGDEF_BEGIN(VPE_RESERVED_337)
REGDEF_BIT(VPE_Reserved_337,        32)
REGDEF_END(VPE_RESERVED_337)


/*
    GEO_LUT_0:    [0x0, 0xffff],            bits : 15_0
    GEO_LUT_1:    [0x0, 0xffff],            bits : 31_16
*/
#define DCE_GEO_REGISTER0_OFS 0x0900
REGDEF_BEGIN(DCE_GEO_REGISTER0)
REGDEF_BIT(GEO_LUT_0,        16)
REGDEF_BIT(GEO_LUT_1,        16)
REGDEF_END(DCE_GEO_REGISTER0)


/*
    GEO_LUT_2:    [0x0, 0xffff],            bits : 15_0
    GEO_LUT_3:    [0x0, 0xffff],            bits : 31_16
*/
#define DCE_GEO_REGISTER1_OFS 0x0904
REGDEF_BEGIN(DCE_GEO_REGISTER1)
REGDEF_BIT(GEO_LUT_2,        16)
REGDEF_BIT(GEO_LUT_3,        16)
REGDEF_END(DCE_GEO_REGISTER1)


/*
    GEO_LUT_4:    [0x0, 0xffff],            bits : 15_0
    GEO_LUT_5:    [0x0, 0xffff],            bits : 31_16
*/
#define DCE_GEO_REGISTER2_OFS 0x0908
REGDEF_BEGIN(DCE_GEO_REGISTER2)
REGDEF_BIT(GEO_LUT_4,        16)
REGDEF_BIT(GEO_LUT_5,        16)
REGDEF_END(DCE_GEO_REGISTER2)


/*
    GEO_LUT_6:    [0x0, 0xffff],            bits : 15_0
    GEO_LUT_7:    [0x0, 0xffff],            bits : 31_16
*/
#define DCE_GEO_REGISTER3_OFS 0x090c
REGDEF_BEGIN(DCE_GEO_REGISTER3)
REGDEF_BIT(GEO_LUT_6,        16)
REGDEF_BIT(GEO_LUT_7,        16)
REGDEF_END(DCE_GEO_REGISTER3)


/*
    GEO_LUT_8:    [0x0, 0xffff],            bits : 15_0
    GEO_LUT_9:    [0x0, 0xffff],            bits : 31_16
*/
#define DCE_GEO_REGISTER4_OFS 0x0910
REGDEF_BEGIN(DCE_GEO_REGISTER4)
REGDEF_BIT(GEO_LUT_8,        16)
REGDEF_BIT(GEO_LUT_9,        16)
REGDEF_END(DCE_GEO_REGISTER4)


/*
    GEO_LUT_10:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_11:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER5_OFS 0x0914
REGDEF_BEGIN(DCE_GEO_REGISTER5)
REGDEF_BIT(GEO_LUT_10,        16)
REGDEF_BIT(GEO_LUT_11,        16)
REGDEF_END(DCE_GEO_REGISTER5)


/*
    GEO_LUT_12:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_13:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER6_OFS 0x0918
REGDEF_BEGIN(DCE_GEO_REGISTER6)
REGDEF_BIT(GEO_LUT_12,        16)
REGDEF_BIT(GEO_LUT_13,        16)
REGDEF_END(DCE_GEO_REGISTER6)


/*
    GEO_LUT_14:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_15:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER7_OFS 0x091c
REGDEF_BEGIN(DCE_GEO_REGISTER7)
REGDEF_BIT(GEO_LUT_14,        16)
REGDEF_BIT(GEO_LUT_15,        16)
REGDEF_END(DCE_GEO_REGISTER7)


/*
    GEO_LUT_16:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_17:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER8_OFS 0x0920
REGDEF_BEGIN(DCE_GEO_REGISTER8)
REGDEF_BIT(GEO_LUT_16,        16)
REGDEF_BIT(GEO_LUT_17,        16)
REGDEF_END(DCE_GEO_REGISTER8)


/*
    GEO_LUT_18:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_19:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER9_OFS 0x0924
REGDEF_BEGIN(DCE_GEO_REGISTER9)
REGDEF_BIT(GEO_LUT_18,        16)
REGDEF_BIT(GEO_LUT_19,        16)
REGDEF_END(DCE_GEO_REGISTER9)


/*
    GEO_LUT_20:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_21:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER10_OFS 0x0928
REGDEF_BEGIN(DCE_GEO_REGISTER10)
REGDEF_BIT(GEO_LUT_20,        16)
REGDEF_BIT(GEO_LUT_21,        16)
REGDEF_END(DCE_GEO_REGISTER10)


/*
    GEO_LUT_22:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_23:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER11_OFS 0x092c
REGDEF_BEGIN(DCE_GEO_REGISTER11)
REGDEF_BIT(GEO_LUT_22,        16)
REGDEF_BIT(GEO_LUT_23,        16)
REGDEF_END(DCE_GEO_REGISTER11)


/*
    GEO_LUT_24:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_25:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER12_OFS 0x0930
REGDEF_BEGIN(DCE_GEO_REGISTER12)
REGDEF_BIT(GEO_LUT_24,        16)
REGDEF_BIT(GEO_LUT_25,        16)
REGDEF_END(DCE_GEO_REGISTER12)


/*
    GEO_LUT_26:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_27:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER13_OFS 0x0934
REGDEF_BEGIN(DCE_GEO_REGISTER13)
REGDEF_BIT(GEO_LUT_26,        16)
REGDEF_BIT(GEO_LUT_27,        16)
REGDEF_END(DCE_GEO_REGISTER13)


/*
    GEO_LUT_28:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_29:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER14_OFS 0x0938
REGDEF_BEGIN(DCE_GEO_REGISTER14)
REGDEF_BIT(GEO_LUT_28,        16)
REGDEF_BIT(GEO_LUT_29,        16)
REGDEF_END(DCE_GEO_REGISTER14)


/*
    GEO_LUT_30:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_31:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER15_OFS 0x093c
REGDEF_BEGIN(DCE_GEO_REGISTER15)
REGDEF_BIT(GEO_LUT_30,        16)
REGDEF_BIT(GEO_LUT_31,        16)
REGDEF_END(DCE_GEO_REGISTER15)


/*
    GEO_LUT_32:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_33:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER16_OFS 0x0940
REGDEF_BEGIN(DCE_GEO_REGISTER16)
REGDEF_BIT(GEO_LUT_32,        16)
REGDEF_BIT(GEO_LUT_33,        16)
REGDEF_END(DCE_GEO_REGISTER16)


/*
    GEO_LUT_34:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_35:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER17_OFS 0x0944
REGDEF_BEGIN(DCE_GEO_REGISTER17)
REGDEF_BIT(GEO_LUT_34,        16)
REGDEF_BIT(GEO_LUT_35,        16)
REGDEF_END(DCE_GEO_REGISTER17)


/*
    GEO_LUT_36:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_37:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER18_OFS 0x0948
REGDEF_BEGIN(DCE_GEO_REGISTER18)
REGDEF_BIT(GEO_LUT_36,        16)
REGDEF_BIT(GEO_LUT_37,        16)
REGDEF_END(DCE_GEO_REGISTER18)


/*
    GEO_LUT_38:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_39:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER19_OFS 0x094c
REGDEF_BEGIN(DCE_GEO_REGISTER19)
REGDEF_BIT(GEO_LUT_38,        16)
REGDEF_BIT(GEO_LUT_39,        16)
REGDEF_END(DCE_GEO_REGISTER19)


/*
    GEO_LUT_40:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_41:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER20_OFS 0x0950
REGDEF_BEGIN(DCE_GEO_REGISTER20)
REGDEF_BIT(GEO_LUT_40,        16)
REGDEF_BIT(GEO_LUT_41,        16)
REGDEF_END(DCE_GEO_REGISTER20)


/*
    GEO_LUT_42:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_43:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER21_OFS 0x0954
REGDEF_BEGIN(DCE_GEO_REGISTER21)
REGDEF_BIT(GEO_LUT_42,        16)
REGDEF_BIT(GEO_LUT_43,        16)
REGDEF_END(DCE_GEO_REGISTER21)


/*
    GEO_LUT_44:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_45:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER22_OFS 0x0958
REGDEF_BEGIN(DCE_GEO_REGISTER22)
REGDEF_BIT(GEO_LUT_44,        16)
REGDEF_BIT(GEO_LUT_45,        16)
REGDEF_END(DCE_GEO_REGISTER22)


/*
    GEO_LUT_46:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_47:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER23_OFS 0x095c
REGDEF_BEGIN(DCE_GEO_REGISTER23)
REGDEF_BIT(GEO_LUT_46,        16)
REGDEF_BIT(GEO_LUT_47,        16)
REGDEF_END(DCE_GEO_REGISTER23)


/*
    GEO_LUT_48:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_49:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER24_OFS 0x0960
REGDEF_BEGIN(DCE_GEO_REGISTER24)
REGDEF_BIT(GEO_LUT_48,        16)
REGDEF_BIT(GEO_LUT_49,        16)
REGDEF_END(DCE_GEO_REGISTER24)


/*
    GEO_LUT_50:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_51:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER25_OFS 0x0964
REGDEF_BEGIN(DCE_GEO_REGISTER25)
REGDEF_BIT(GEO_LUT_50,        16)
REGDEF_BIT(GEO_LUT_51,        16)
REGDEF_END(DCE_GEO_REGISTER25)


/*
    GEO_LUT_52:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_53:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER26_OFS 0x0968
REGDEF_BEGIN(DCE_GEO_REGISTER26)
REGDEF_BIT(GEO_LUT_52,        16)
REGDEF_BIT(GEO_LUT_53,        16)
REGDEF_END(DCE_GEO_REGISTER26)


/*
    GEO_LUT_54:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_55:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER27_OFS 0x096c
REGDEF_BEGIN(DCE_GEO_REGISTER27)
REGDEF_BIT(GEO_LUT_54,        16)
REGDEF_BIT(GEO_LUT_55,        16)
REGDEF_END(DCE_GEO_REGISTER27)


/*
    GEO_LUT_56:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_57:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER28_OFS 0x0970
REGDEF_BEGIN(DCE_GEO_REGISTER28)
REGDEF_BIT(GEO_LUT_56,        16)
REGDEF_BIT(GEO_LUT_57,        16)
REGDEF_END(DCE_GEO_REGISTER28)


/*
    GEO_LUT_58:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_59:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER29_OFS 0x0974
REGDEF_BEGIN(DCE_GEO_REGISTER29)
REGDEF_BIT(GEO_LUT_58,        16)
REGDEF_BIT(GEO_LUT_59,        16)
REGDEF_END(DCE_GEO_REGISTER29)


/*
    GEO_LUT_60:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_61:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER30_OFS 0x0978
REGDEF_BEGIN(DCE_GEO_REGISTER30)
REGDEF_BIT(GEO_LUT_60,        16)
REGDEF_BIT(GEO_LUT_61,        16)
REGDEF_END(DCE_GEO_REGISTER30)


/*
    GEO_LUT_62:    [0x0, 0xffff],           bits : 15_0
    GEO_LUT_63:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER31_OFS 0x097c
REGDEF_BEGIN(DCE_GEO_REGISTER31)
REGDEF_BIT(GEO_LUT_62,        16)
REGDEF_BIT(GEO_LUT_63,        16)
REGDEF_END(DCE_GEO_REGISTER31)


/*
    GEO_LUT_64:    [0x0, 0xffff],           bits : 31_16
*/
#define DCE_GEO_REGISTER32_OFS 0x0980
REGDEF_BEGIN(DCE_GEO_REGISTER32)
REGDEF_BIT(GEO_LUT_64,        16)
REGDEF_BIT(,        16)
REGDEF_END(DCE_GEO_REGISTER32)


/*
    VPE_Reserved_338:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_338_OFS 0x0984
REGDEF_BEGIN(VPE_RESERVED_338)
REGDEF_BIT(VPE_Reserved_338,        32)
REGDEF_END(VPE_RESERVED_338)


/*
    VPE_Reserved_339:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_339_OFS 0x0988
REGDEF_BEGIN(VPE_RESERVED_339)
REGDEF_BIT(VPE_Reserved_339,        32)
REGDEF_END(VPE_RESERVED_339)


/*
    VPE_Reserved_340:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_340_OFS 0x098c
REGDEF_BEGIN(VPE_RESERVED_340)
REGDEF_BIT(VPE_Reserved_340,        32)
REGDEF_END(VPE_RESERVED_340)


/*
    VPE_Reserved_341:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_341_OFS 0x0990
REGDEF_BEGIN(VPE_RESERVED_341)
REGDEF_BIT(VPE_Reserved_341,        32)
REGDEF_END(VPE_RESERVED_341)


/*
    VPE_Reserved_342:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_342_OFS 0x0994
REGDEF_BEGIN(VPE_RESERVED_342)
REGDEF_BIT(VPE_Reserved_342,        32)
REGDEF_END(VPE_RESERVED_342)


/*
    VPE_Reserved_343:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_343_OFS 0x0998
REGDEF_BEGIN(VPE_RESERVED_343)
REGDEF_BIT(VPE_Reserved_343,        32)
REGDEF_END(VPE_RESERVED_343)


/*
    VPE_Reserved_344:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_344_OFS 0x099c
REGDEF_BEGIN(VPE_RESERVED_344)
REGDEF_BIT(VPE_Reserved_344,        32)
REGDEF_END(VPE_RESERVED_344)


/*
    VPE_Reserved_345:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_345_OFS 0x09a0
REGDEF_BEGIN(VPE_RESERVED_345)
REGDEF_BIT(VPE_Reserved_345,        32)
REGDEF_END(VPE_RESERVED_345)


/*
    VPE_Reserved_346:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_346_OFS 0x09a4
REGDEF_BEGIN(VPE_RESERVED_346)
REGDEF_BIT(VPE_Reserved_346,        32)
REGDEF_END(VPE_RESERVED_346)


/*
    VPE_Reserved_347:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_347_OFS 0x09a8
REGDEF_BEGIN(VPE_RESERVED_347)
REGDEF_BIT(VPE_Reserved_347,        32)
REGDEF_END(VPE_RESERVED_347)


/*
    VPE_Reserved_348:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_348_OFS 0x09ac
REGDEF_BEGIN(VPE_RESERVED_348)
REGDEF_BIT(VPE_Reserved_348,        32)
REGDEF_END(VPE_RESERVED_348)


/*
    VPE_Reserved_349:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_349_OFS 0x09b0
REGDEF_BEGIN(VPE_RESERVED_349)
REGDEF_BIT(VPE_Reserved_349,        32)
REGDEF_END(VPE_RESERVED_349)


/*
    VPE_Reserved_350:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_350_OFS 0x09b4
REGDEF_BEGIN(VPE_RESERVED_350)
REGDEF_BIT(VPE_Reserved_350,        32)
REGDEF_END(VPE_RESERVED_350)


/*
    VPE_Reserved_351:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_351_OFS 0x09b8
REGDEF_BEGIN(VPE_RESERVED_351)
REGDEF_BIT(VPE_Reserved_351,        32)
REGDEF_END(VPE_RESERVED_351)


/*
    VPE_Reserved_352:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_352_OFS 0x09bc
REGDEF_BEGIN(VPE_RESERVED_352)
REGDEF_BIT(VPE_Reserved_352,        32)
REGDEF_END(VPE_RESERVED_352)


/*
    VPE_Reserved_353:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_353_OFS 0x09c0
REGDEF_BEGIN(VPE_RESERVED_353)
REGDEF_BIT(VPE_Reserved_353,        32)
REGDEF_END(VPE_RESERVED_353)


/*
    VPE_Reserved_354:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_354_OFS 0x09c4
REGDEF_BEGIN(VPE_RESERVED_354)
REGDEF_BIT(VPE_Reserved_354,        32)
REGDEF_END(VPE_RESERVED_354)


/*
    VPE_Reserved_355:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_355_OFS 0x09c8
REGDEF_BEGIN(VPE_RESERVED_355)
REGDEF_BIT(VPE_Reserved_355,        32)
REGDEF_END(VPE_RESERVED_355)


/*
    VPE_Reserved_356:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_356_OFS 0x09cc
REGDEF_BEGIN(VPE_RESERVED_356)
REGDEF_BIT(VPE_Reserved_356,        32)
REGDEF_END(VPE_RESERVED_356)


/*
    VPE_Reserved_357:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_357_OFS 0x09d0
REGDEF_BEGIN(VPE_RESERVED_357)
REGDEF_BIT(VPE_Reserved_357,        32)
REGDEF_END(VPE_RESERVED_357)


/*
    VPE_Reserved_358:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_358_OFS 0x09d4
REGDEF_BEGIN(VPE_RESERVED_358)
REGDEF_BIT(VPE_Reserved_358,        32)
REGDEF_END(VPE_RESERVED_358)


/*
    VPE_Reserved_359:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_359_OFS 0x09d8
REGDEF_BEGIN(VPE_RESERVED_359)
REGDEF_BIT(VPE_Reserved_359,        32)
REGDEF_END(VPE_RESERVED_359)


/*
    VPE_Reserved_360:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_360_OFS 0x09dc
REGDEF_BEGIN(VPE_RESERVED_360)
REGDEF_BIT(VPE_Reserved_360,        32)
REGDEF_END(VPE_RESERVED_360)


/*
    VPE_Reserved_361:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_361_OFS 0x09e0
REGDEF_BEGIN(VPE_RESERVED_361)
REGDEF_BIT(VPE_Reserved_361,        32)
REGDEF_END(VPE_RESERVED_361)


/*
    VPE_Reserved_362:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_362_OFS 0x09e4
REGDEF_BEGIN(VPE_RESERVED_362)
REGDEF_BIT(VPE_Reserved_362,        32)
REGDEF_END(VPE_RESERVED_362)


/*
    VPE_Reserved_363:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_363_OFS 0x09e8
REGDEF_BEGIN(VPE_RESERVED_363)
REGDEF_BIT(VPE_Reserved_363,        32)
REGDEF_END(VPE_RESERVED_363)


/*
    VPE_Reserved_364:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_364_OFS 0x09ec
REGDEF_BEGIN(VPE_RESERVED_364)
REGDEF_BIT(VPE_Reserved_364,        32)
REGDEF_END(VPE_RESERVED_364)


/*
    VPE_Reserved_365:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_365_OFS 0x09f0
REGDEF_BEGIN(VPE_RESERVED_365)
REGDEF_BIT(VPE_Reserved_365,        32)
REGDEF_END(VPE_RESERVED_365)


/*
    VPE_Reserved_366:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_366_OFS 0x09f4
REGDEF_BEGIN(VPE_RESERVED_366)
REGDEF_BIT(VPE_Reserved_366,        32)
REGDEF_END(VPE_RESERVED_366)


/*
    VPE_Reserved_367:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_367_OFS 0x09f8
REGDEF_BEGIN(VPE_RESERVED_367)
REGDEF_BIT(VPE_Reserved_367,        32)
REGDEF_END(VPE_RESERVED_367)


/*
    VPE_Reserved_368:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_368_OFS 0x09fc
REGDEF_BEGIN(VPE_RESERVED_368)
REGDEF_BIT(VPE_Reserved_368,        32)
REGDEF_END(VPE_RESERVED_368)


/*
    VPE_Reserved_369:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_369_OFS 0x0a00
REGDEF_BEGIN(VPE_RESERVED_369)
REGDEF_BIT(VPE_Reserved_369,        32)
REGDEF_END(VPE_RESERVED_369)


/*
    VPE_Reserved_370:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_370_OFS 0x0a04
REGDEF_BEGIN(VPE_RESERVED_370)
REGDEF_BIT(VPE_Reserved_370,        32)
REGDEF_END(VPE_RESERVED_370)


/*
    VPE_Reserved_371:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_371_OFS 0x0a08
REGDEF_BEGIN(VPE_RESERVED_371)
REGDEF_BIT(VPE_Reserved_371,        32)
REGDEF_END(VPE_RESERVED_371)


/*
    VPE_Reserved_372:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_372_OFS 0x0a0c
REGDEF_BEGIN(VPE_RESERVED_372)
REGDEF_BIT(VPE_Reserved_372,        32)
REGDEF_END(VPE_RESERVED_372)


/*
    VPE_Reserved_373:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_373_OFS 0x0a10
REGDEF_BEGIN(VPE_RESERVED_373)
REGDEF_BIT(VPE_Reserved_373,        32)
REGDEF_END(VPE_RESERVED_373)


/*
    VPE_Reserved_374:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_374_OFS 0x0a14
REGDEF_BEGIN(VPE_RESERVED_374)
REGDEF_BIT(VPE_Reserved_374,        32)
REGDEF_END(VPE_RESERVED_374)


/*
    VPE_Reserved_375:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_375_OFS 0x0a18
REGDEF_BEGIN(VPE_RESERVED_375)
REGDEF_BIT(VPE_Reserved_375,        32)
REGDEF_END(VPE_RESERVED_375)


/*
    VPE_Reserved_376:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_376_OFS 0x0a1c
REGDEF_BEGIN(VPE_RESERVED_376)
REGDEF_BIT(VPE_Reserved_376,        32)
REGDEF_END(VPE_RESERVED_376)


/*
    VPE_Reserved_377:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_377_OFS 0x0a20
REGDEF_BEGIN(VPE_RESERVED_377)
REGDEF_BIT(VPE_Reserved_377,        32)
REGDEF_END(VPE_RESERVED_377)


/*
    VPE_Reserved_378:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_378_OFS 0x0a24
REGDEF_BEGIN(VPE_RESERVED_378)
REGDEF_BIT(VPE_Reserved_378,        32)
REGDEF_END(VPE_RESERVED_378)


/*
    VPE_Reserved_379:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_379_OFS 0x0a28
REGDEF_BEGIN(VPE_RESERVED_379)
REGDEF_BIT(VPE_Reserved_379,        32)
REGDEF_END(VPE_RESERVED_379)


/*
    VPE_Reserved_380:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_380_OFS 0x0a2c
REGDEF_BEGIN(VPE_RESERVED_380)
REGDEF_BIT(VPE_Reserved_380,        32)
REGDEF_END(VPE_RESERVED_380)


/*
    VPE_Reserved_381:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_381_OFS 0x0a30
REGDEF_BEGIN(VPE_RESERVED_381)
REGDEF_BIT(VPE_Reserved_381,        32)
REGDEF_END(VPE_RESERVED_381)


/*
    VPE_Reserved_382:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_382_OFS 0x0a34
REGDEF_BEGIN(VPE_RESERVED_382)
REGDEF_BIT(VPE_Reserved_382,        32)
REGDEF_END(VPE_RESERVED_382)


/*
    VPE_Reserved_383:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_383_OFS 0x0a38
REGDEF_BEGIN(VPE_RESERVED_383)
REGDEF_BIT(VPE_Reserved_383,        32)
REGDEF_END(VPE_RESERVED_383)


/*
    VPE_Reserved_384:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_384_OFS 0x0a3c
REGDEF_BEGIN(VPE_RESERVED_384)
REGDEF_BIT(VPE_Reserved_384,        32)
REGDEF_END(VPE_RESERVED_384)


/*
    VPE_Reserved_385:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_385_OFS 0x0a40
REGDEF_BEGIN(VPE_RESERVED_385)
REGDEF_BIT(VPE_Reserved_385,        32)
REGDEF_END(VPE_RESERVED_385)


/*
    VPE_Reserved_386:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_386_OFS 0x0a44
REGDEF_BEGIN(VPE_RESERVED_386)
REGDEF_BIT(VPE_Reserved_386,        32)
REGDEF_END(VPE_RESERVED_386)


/*
    VPE_Reserved_387:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_387_OFS 0x0a48
REGDEF_BEGIN(VPE_RESERVED_387)
REGDEF_BIT(VPE_Reserved_387,        32)
REGDEF_END(VPE_RESERVED_387)


/*
    VPE_Reserved_388:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_388_OFS 0x0a4c
REGDEF_BEGIN(VPE_RESERVED_388)
REGDEF_BIT(VPE_Reserved_388,        32)
REGDEF_END(VPE_RESERVED_388)


/*
    VPE_Reserved_389:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_389_OFS 0x0a50
REGDEF_BEGIN(VPE_RESERVED_389)
REGDEF_BIT(VPE_Reserved_389,        32)
REGDEF_END(VPE_RESERVED_389)


/*
    VPE_Reserved_390:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_390_OFS 0x0a54
REGDEF_BEGIN(VPE_RESERVED_390)
REGDEF_BIT(VPE_Reserved_390,        32)
REGDEF_END(VPE_RESERVED_390)


/*
    VPE_Reserved_391:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_391_OFS 0x0a58
REGDEF_BEGIN(VPE_RESERVED_391)
REGDEF_BIT(VPE_Reserved_391,        32)
REGDEF_END(VPE_RESERVED_391)


/*
    VPE_Reserved_392:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_392_OFS 0x0a5c
REGDEF_BEGIN(VPE_RESERVED_392)
REGDEF_BIT(VPE_Reserved_392,        32)
REGDEF_END(VPE_RESERVED_392)


/*
    VPE_Reserved_393:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_393_OFS 0x0a60
REGDEF_BEGIN(VPE_RESERVED_393)
REGDEF_BIT(VPE_Reserved_393,        32)
REGDEF_END(VPE_RESERVED_393)


/*
    VPE_Reserved_394:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_394_OFS 0x0a64
REGDEF_BEGIN(VPE_RESERVED_394)
REGDEF_BIT(VPE_Reserved_394,        32)
REGDEF_END(VPE_RESERVED_394)


/*
    VPE_Reserved_395:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_395_OFS 0x0a68
REGDEF_BEGIN(VPE_RESERVED_395)
REGDEF_BIT(VPE_Reserved_395,        32)
REGDEF_END(VPE_RESERVED_395)


/*
    VPE_Reserved_396:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_396_OFS 0x0a6c
REGDEF_BEGIN(VPE_RESERVED_396)
REGDEF_BIT(VPE_Reserved_396,        32)
REGDEF_END(VPE_RESERVED_396)


/*
    VPE_Reserved_397:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_397_OFS 0x0a70
REGDEF_BEGIN(VPE_RESERVED_397)
REGDEF_BIT(VPE_Reserved_397,        32)
REGDEF_END(VPE_RESERVED_397)


/*
    VPE_Reserved_398:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_398_OFS 0x0a74
REGDEF_BEGIN(VPE_RESERVED_398)
REGDEF_BIT(VPE_Reserved_398,        32)
REGDEF_END(VPE_RESERVED_398)


/*
    VPE_Reserved_399:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_399_OFS 0x0a78
REGDEF_BEGIN(VPE_RESERVED_399)
REGDEF_BIT(VPE_Reserved_399,        32)
REGDEF_END(VPE_RESERVED_399)


/*
    VPE_Reserved_400:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_400_OFS 0x0a7c
REGDEF_BEGIN(VPE_RESERVED_400)
REGDEF_BIT(VPE_Reserved_400,        32)
REGDEF_END(VPE_RESERVED_400)


/*
    VPE_Reserved_401:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_401_OFS 0x0a80
REGDEF_BEGIN(VPE_RESERVED_401)
REGDEF_BIT(VPE_Reserved_401,        32)
REGDEF_END(VPE_RESERVED_401)


/*
    VPE_Reserved_402:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_402_OFS 0x0a84
REGDEF_BEGIN(VPE_RESERVED_402)
REGDEF_BIT(VPE_Reserved_402,        32)
REGDEF_END(VPE_RESERVED_402)


/*
    VPE_Reserved_403:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_403_OFS 0x0a88
REGDEF_BEGIN(VPE_RESERVED_403)
REGDEF_BIT(VPE_Reserved_403,        32)
REGDEF_END(VPE_RESERVED_403)


/*
    VPE_Reserved_404:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_404_OFS 0x0a8c
REGDEF_BEGIN(VPE_RESERVED_404)
REGDEF_BIT(VPE_Reserved_404,        32)
REGDEF_END(VPE_RESERVED_404)


/*
    VPE_Reserved_405:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_405_OFS 0x0a90
REGDEF_BEGIN(VPE_RESERVED_405)
REGDEF_BIT(VPE_Reserved_405,        32)
REGDEF_END(VPE_RESERVED_405)


/*
    VPE_Reserved_406:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_406_OFS 0x0a94
REGDEF_BEGIN(VPE_RESERVED_406)
REGDEF_BIT(VPE_Reserved_406,        32)
REGDEF_END(VPE_RESERVED_406)


/*
    VPE_Reserved_407:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_407_OFS 0x0a98
REGDEF_BEGIN(VPE_RESERVED_407)
REGDEF_BIT(VPE_Reserved_407,        32)
REGDEF_END(VPE_RESERVED_407)


/*
    VPE_Reserved_408:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_408_OFS 0x0a9c
REGDEF_BEGIN(VPE_RESERVED_408)
REGDEF_BIT(VPE_Reserved_408,        32)
REGDEF_END(VPE_RESERVED_408)


/*
    VPE_Reserved_409:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_409_OFS 0x0aa0
REGDEF_BEGIN(VPE_RESERVED_409)
REGDEF_BIT(VPE_Reserved_409,        32)
REGDEF_END(VPE_RESERVED_409)


/*
    VPE_Reserved_410:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_410_OFS 0x0aa4
REGDEF_BEGIN(VPE_RESERVED_410)
REGDEF_BIT(VPE_Reserved_410,        32)
REGDEF_END(VPE_RESERVED_410)


/*
    VPE_Reserved_411:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_411_OFS 0x0aa8
REGDEF_BEGIN(VPE_RESERVED_411)
REGDEF_BIT(VPE_Reserved_411,        32)
REGDEF_END(VPE_RESERVED_411)


/*
    VPE_Reserved_412:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_412_OFS 0x0aac
REGDEF_BEGIN(VPE_RESERVED_412)
REGDEF_BIT(VPE_Reserved_412,        32)
REGDEF_END(VPE_RESERVED_412)


/*
    VPE_Reserved_413:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_413_OFS 0x0ab0
REGDEF_BEGIN(VPE_RESERVED_413)
REGDEF_BIT(VPE_Reserved_413,        32)
REGDEF_END(VPE_RESERVED_413)


/*
    VPE_Reserved_414:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_414_OFS 0x0ab4
REGDEF_BEGIN(VPE_RESERVED_414)
REGDEF_BIT(VPE_Reserved_414,        32)
REGDEF_END(VPE_RESERVED_414)


/*
    VPE_Reserved_415:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_415_OFS 0x0ab8
REGDEF_BEGIN(VPE_RESERVED_415)
REGDEF_BIT(VPE_Reserved_415,        32)
REGDEF_END(VPE_RESERVED_415)


/*
    VPE_Reserved_416:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_416_OFS 0x0abc
REGDEF_BEGIN(VPE_RESERVED_416)
REGDEF_BIT(VPE_Reserved_416,        32)
REGDEF_END(VPE_RESERVED_416)


/*
    VPE_Reserved_417:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_417_OFS 0x0ac0
REGDEF_BEGIN(VPE_RESERVED_417)
REGDEF_BIT(VPE_Reserved_417,        32)
REGDEF_END(VPE_RESERVED_417)


/*
    VPE_Reserved_418:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_418_OFS 0x0ac4
REGDEF_BEGIN(VPE_RESERVED_418)
REGDEF_BIT(VPE_Reserved_418,        32)
REGDEF_END(VPE_RESERVED_418)


/*
    VPE_Reserved_419:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_419_OFS 0x0ac8
REGDEF_BEGIN(VPE_RESERVED_419)
REGDEF_BIT(VPE_Reserved_419,        32)
REGDEF_END(VPE_RESERVED_419)


/*
    VPE_Reserved_420:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_420_OFS 0x0acc
REGDEF_BEGIN(VPE_RESERVED_420)
REGDEF_BIT(VPE_Reserved_420,        32)
REGDEF_END(VPE_RESERVED_420)


/*
    VPE_Reserved_421:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_421_OFS 0x0ad0
REGDEF_BEGIN(VPE_RESERVED_421)
REGDEF_BIT(VPE_Reserved_421,        32)
REGDEF_END(VPE_RESERVED_421)


/*
    VPE_Reserved_422:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_422_OFS 0x0ad4
REGDEF_BEGIN(VPE_RESERVED_422)
REGDEF_BIT(VPE_Reserved_422,        32)
REGDEF_END(VPE_RESERVED_422)


/*
    VPE_Reserved_423:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_423_OFS 0x0ad8
REGDEF_BEGIN(VPE_RESERVED_423)
REGDEF_BIT(VPE_Reserved_423,        32)
REGDEF_END(VPE_RESERVED_423)


/*
    VPE_Reserved_424:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_424_OFS 0x0adc
REGDEF_BEGIN(VPE_RESERVED_424)
REGDEF_BIT(VPE_Reserved_424,        32)
REGDEF_END(VPE_RESERVED_424)


/*
    VPE_Reserved_425:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_425_OFS 0x0ae0
REGDEF_BEGIN(VPE_RESERVED_425)
REGDEF_BIT(VPE_Reserved_425,        32)
REGDEF_END(VPE_RESERVED_425)


/*
    VPE_Reserved_426:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_426_OFS 0x0ae4
REGDEF_BEGIN(VPE_RESERVED_426)
REGDEF_BIT(VPE_Reserved_426,        32)
REGDEF_END(VPE_RESERVED_426)


/*
    VPE_Reserved_427:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_427_OFS 0x0ae8
REGDEF_BEGIN(VPE_RESERVED_427)
REGDEF_BIT(VPE_Reserved_427,        32)
REGDEF_END(VPE_RESERVED_427)


/*
    VPE_Reserved_428:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_428_OFS 0x0aec
REGDEF_BEGIN(VPE_RESERVED_428)
REGDEF_BIT(VPE_Reserved_428,        32)
REGDEF_END(VPE_RESERVED_428)


/*
    VPE_Reserved_429:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_429_OFS 0x0af0
REGDEF_BEGIN(VPE_RESERVED_429)
REGDEF_BIT(VPE_Reserved_429,        32)
REGDEF_END(VPE_RESERVED_429)


/*
    VPE_Reserved_430:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_430_OFS 0x0af4
REGDEF_BEGIN(VPE_RESERVED_430)
REGDEF_BIT(VPE_Reserved_430,        32)
REGDEF_END(VPE_RESERVED_430)


/*
    VPE_Reserved_431:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_431_OFS 0x0af8
REGDEF_BEGIN(VPE_RESERVED_431)
REGDEF_BIT(VPE_Reserved_431,        32)
REGDEF_END(VPE_RESERVED_431)


/*
    VPE_Reserved_432:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_432_OFS 0x0afc
REGDEF_BEGIN(VPE_RESERVED_432)
REGDEF_BIT(VPE_Reserved_432,        32)
REGDEF_END(VPE_RESERVED_432)


/*
    VPE_Reserved_433:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_433_OFS 0x0b00
REGDEF_BEGIN(VPE_RESERVED_433)
REGDEF_BIT(VPE_Reserved_433,        32)
REGDEF_END(VPE_RESERVED_433)


/*
    VPE_Reserved_434:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_434_OFS 0x0b04
REGDEF_BEGIN(VPE_RESERVED_434)
REGDEF_BIT(VPE_Reserved_434,        32)
REGDEF_END(VPE_RESERVED_434)


/*
    VPE_Reserved_435:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_435_OFS 0x0b08
REGDEF_BEGIN(VPE_RESERVED_435)
REGDEF_BIT(VPE_Reserved_435,        32)
REGDEF_END(VPE_RESERVED_435)


/*
    VPE_Reserved_436:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_436_OFS 0x0b0c
REGDEF_BEGIN(VPE_RESERVED_436)
REGDEF_BIT(VPE_Reserved_436,        32)
REGDEF_END(VPE_RESERVED_436)


/*
    VPE_Reserved_437:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_437_OFS 0x0b10
REGDEF_BEGIN(VPE_RESERVED_437)
REGDEF_BIT(VPE_Reserved_437,        32)
REGDEF_END(VPE_RESERVED_437)


/*
    VPE_Reserved_438:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_438_OFS 0x0b14
REGDEF_BEGIN(VPE_RESERVED_438)
REGDEF_BIT(VPE_Reserved_438,        32)
REGDEF_END(VPE_RESERVED_438)


/*
    VPE_Reserved_439:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_439_OFS 0x0b18
REGDEF_BEGIN(VPE_RESERVED_439)
REGDEF_BIT(VPE_Reserved_439,        32)
REGDEF_END(VPE_RESERVED_439)


/*
    VPE_Reserved_440:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_440_OFS 0x0b1c
REGDEF_BEGIN(VPE_RESERVED_440)
REGDEF_BIT(VPE_Reserved_440,        32)
REGDEF_END(VPE_RESERVED_440)


/*
    VPE_Reserved_441:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_441_OFS 0x0b20
REGDEF_BEGIN(VPE_RESERVED_441)
REGDEF_BIT(VPE_Reserved_441,        32)
REGDEF_END(VPE_RESERVED_441)


/*
    VPE_Reserved_442:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_442_OFS 0x0b24
REGDEF_BEGIN(VPE_RESERVED_442)
REGDEF_BIT(VPE_Reserved_442,        32)
REGDEF_END(VPE_RESERVED_442)


/*
    VPE_Reserved_443:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_443_OFS 0x0b28
REGDEF_BEGIN(VPE_RESERVED_443)
REGDEF_BIT(VPE_Reserved_443,        32)
REGDEF_END(VPE_RESERVED_443)


/*
    VPE_Reserved_444:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_444_OFS 0x0b2c
REGDEF_BEGIN(VPE_RESERVED_444)
REGDEF_BIT(VPE_Reserved_444,        32)
REGDEF_END(VPE_RESERVED_444)


/*
    VPE_Reserved_445:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_445_OFS 0x0b30
REGDEF_BEGIN(VPE_RESERVED_445)
REGDEF_BIT(VPE_Reserved_445,        32)
REGDEF_END(VPE_RESERVED_445)


/*
    VPE_Reserved_446:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_446_OFS 0x0b34
REGDEF_BEGIN(VPE_RESERVED_446)
REGDEF_BIT(VPE_Reserved_446,        32)
REGDEF_END(VPE_RESERVED_446)


/*
    VPE_Reserved_447:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_447_OFS 0x0b38
REGDEF_BEGIN(VPE_RESERVED_447)
REGDEF_BIT(VPE_Reserved_447,        32)
REGDEF_END(VPE_RESERVED_447)


/*
    VPE_Reserved_448:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_448_OFS 0x0b3c
REGDEF_BEGIN(VPE_RESERVED_448)
REGDEF_BIT(VPE_Reserved_448,        32)
REGDEF_END(VPE_RESERVED_448)


/*
    VPE_Reserved_449:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_449_OFS 0x0b40
REGDEF_BEGIN(VPE_RESERVED_449)
REGDEF_BIT(VPE_Reserved_449,        32)
REGDEF_END(VPE_RESERVED_449)


/*
    VPE_Reserved_450:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_450_OFS 0x0b44
REGDEF_BEGIN(VPE_RESERVED_450)
REGDEF_BIT(VPE_Reserved_450,        32)
REGDEF_END(VPE_RESERVED_450)


/*
    VPE_Reserved_451:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_451_OFS 0x0b48
REGDEF_BEGIN(VPE_RESERVED_451)
REGDEF_BIT(VPE_Reserved_451,        32)
REGDEF_END(VPE_RESERVED_451)


/*
    VPE_Reserved_452:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_452_OFS 0x0b4c
REGDEF_BEGIN(VPE_RESERVED_452)
REGDEF_BIT(VPE_Reserved_452,        32)
REGDEF_END(VPE_RESERVED_452)


/*
    VPE_Reserved_453:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_453_OFS 0x0b50
REGDEF_BEGIN(VPE_RESERVED_453)
REGDEF_BIT(VPE_Reserved_453,        32)
REGDEF_END(VPE_RESERVED_453)


/*
    VPE_Reserved_454:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_454_OFS 0x0b54
REGDEF_BEGIN(VPE_RESERVED_454)
REGDEF_BIT(VPE_Reserved_454,        32)
REGDEF_END(VPE_RESERVED_454)


/*
    VPE_Reserved_455:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_455_OFS 0x0b58
REGDEF_BEGIN(VPE_RESERVED_455)
REGDEF_BIT(VPE_Reserved_455,        32)
REGDEF_END(VPE_RESERVED_455)


/*
    VPE_Reserved_456:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_456_OFS 0x0b5c
REGDEF_BEGIN(VPE_RESERVED_456)
REGDEF_BIT(VPE_Reserved_456,        32)
REGDEF_END(VPE_RESERVED_456)


/*
    VPE_Reserved_457:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_457_OFS 0x0b60
REGDEF_BEGIN(VPE_RESERVED_457)
REGDEF_BIT(VPE_Reserved_457,        32)
REGDEF_END(VPE_RESERVED_457)


/*
    VPE_Reserved_458:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_458_OFS 0x0b64
REGDEF_BEGIN(VPE_RESERVED_458)
REGDEF_BIT(VPE_Reserved_458,        32)
REGDEF_END(VPE_RESERVED_458)


/*
    VPE_Reserved_459:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_459_OFS 0x0b68
REGDEF_BEGIN(VPE_RESERVED_459)
REGDEF_BIT(VPE_Reserved_459,        32)
REGDEF_END(VPE_RESERVED_459)


/*
    VPE_Reserved_460:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_460_OFS 0x0b6c
REGDEF_BEGIN(VPE_RESERVED_460)
REGDEF_BIT(VPE_Reserved_460,        32)
REGDEF_END(VPE_RESERVED_460)


/*
    VPE_Reserved_461:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_461_OFS 0x0b70
REGDEF_BEGIN(VPE_RESERVED_461)
REGDEF_BIT(VPE_Reserved_461,        32)
REGDEF_END(VPE_RESERVED_461)


/*
    VPE_Reserved_462:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_462_OFS 0x0b74
REGDEF_BEGIN(VPE_RESERVED_462)
REGDEF_BIT(VPE_Reserved_462,        32)
REGDEF_END(VPE_RESERVED_462)


/*
    VPE_Reserved_463:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_463_OFS 0x0b78
REGDEF_BEGIN(VPE_RESERVED_463)
REGDEF_BIT(VPE_Reserved_463,        32)
REGDEF_END(VPE_RESERVED_463)


/*
    VPE_Reserved_464:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_464_OFS 0x0b7c
REGDEF_BEGIN(VPE_RESERVED_464)
REGDEF_BIT(VPE_Reserved_464,        32)
REGDEF_END(VPE_RESERVED_464)


/*
    VPE_Reserved_465:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_465_OFS 0x0b80
REGDEF_BEGIN(VPE_RESERVED_465)
REGDEF_BIT(VPE_Reserved_465,        32)
REGDEF_END(VPE_RESERVED_465)


/*
    VPE_Reserved_466:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_466_OFS 0x0b84
REGDEF_BEGIN(VPE_RESERVED_466)
REGDEF_BIT(VPE_Reserved_466,        32)
REGDEF_END(VPE_RESERVED_466)


/*
    VPE_Reserved_467:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_467_OFS 0x0b88
REGDEF_BEGIN(VPE_RESERVED_467)
REGDEF_BIT(VPE_Reserved_467,        32)
REGDEF_END(VPE_RESERVED_467)


/*
    VPE_Reserved_468:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_468_OFS 0x0b8c
REGDEF_BEGIN(VPE_RESERVED_468)
REGDEF_BIT(VPE_Reserved_468,        32)
REGDEF_END(VPE_RESERVED_468)


/*
    VPE_Reserved_469:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_469_OFS 0x0b90
REGDEF_BEGIN(VPE_RESERVED_469)
REGDEF_BIT(VPE_Reserved_469,        32)
REGDEF_END(VPE_RESERVED_469)


/*
    VPE_Reserved_470:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_470_OFS 0x0b94
REGDEF_BEGIN(VPE_RESERVED_470)
REGDEF_BIT(VPE_Reserved_470,        32)
REGDEF_END(VPE_RESERVED_470)


/*
    VPE_Reserved_471:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_471_OFS 0x0b98
REGDEF_BEGIN(VPE_RESERVED_471)
REGDEF_BIT(VPE_Reserved_471,        32)
REGDEF_END(VPE_RESERVED_471)


/*
    VPE_Reserved_472:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_472_OFS 0x0b9c
REGDEF_BEGIN(VPE_RESERVED_472)
REGDEF_BIT(VPE_Reserved_472,        32)
REGDEF_END(VPE_RESERVED_472)


/*
    VPE_Reserved_473:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_473_OFS 0x0ba0
REGDEF_BEGIN(VPE_RESERVED_473)
REGDEF_BIT(VPE_Reserved_473,        32)
REGDEF_END(VPE_RESERVED_473)


/*
    VPE_Reserved_474:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_474_OFS 0x0ba4
REGDEF_BEGIN(VPE_RESERVED_474)
REGDEF_BIT(VPE_Reserved_474,        32)
REGDEF_END(VPE_RESERVED_474)


/*
    VPE_Reserved_475:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_475_OFS 0x0ba8
REGDEF_BEGIN(VPE_RESERVED_475)
REGDEF_BIT(VPE_Reserved_475,        32)
REGDEF_END(VPE_RESERVED_475)


/*
    VPE_Reserved_476:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_476_OFS 0x0bac
REGDEF_BEGIN(VPE_RESERVED_476)
REGDEF_BIT(VPE_Reserved_476,        32)
REGDEF_END(VPE_RESERVED_476)


/*
    VPE_Reserved_477:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_477_OFS 0x0bb0
REGDEF_BEGIN(VPE_RESERVED_477)
REGDEF_BIT(VPE_Reserved_477,        32)
REGDEF_END(VPE_RESERVED_477)


/*
    VPE_Reserved_478:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_478_OFS 0x0bb4
REGDEF_BEGIN(VPE_RESERVED_478)
REGDEF_BIT(VPE_Reserved_478,        32)
REGDEF_END(VPE_RESERVED_478)


/*
    VPE_Reserved_479:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_479_OFS 0x0bb8
REGDEF_BEGIN(VPE_RESERVED_479)
REGDEF_BIT(VPE_Reserved_479,        32)
REGDEF_END(VPE_RESERVED_479)


/*
    VPE_Reserved_480:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_480_OFS 0x0bbc
REGDEF_BEGIN(VPE_RESERVED_480)
REGDEF_BIT(VPE_Reserved_480,        32)
REGDEF_END(VPE_RESERVED_480)


/*
    VPE_Reserved_481:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_481_OFS 0x0bc0
REGDEF_BEGIN(VPE_RESERVED_481)
REGDEF_BIT(VPE_Reserved_481,        32)
REGDEF_END(VPE_RESERVED_481)


/*
    VPE_Reserved_482:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_482_OFS 0x0bc4
REGDEF_BEGIN(VPE_RESERVED_482)
REGDEF_BIT(VPE_Reserved_482,        32)
REGDEF_END(VPE_RESERVED_482)


/*
    VPE_Reserved_483:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_483_OFS 0x0bc8
REGDEF_BEGIN(VPE_RESERVED_483)
REGDEF_BIT(VPE_Reserved_483,        32)
REGDEF_END(VPE_RESERVED_483)


/*
    VPE_Reserved_484:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_484_OFS 0x0bcc
REGDEF_BEGIN(VPE_RESERVED_484)
REGDEF_BIT(VPE_Reserved_484,        32)
REGDEF_END(VPE_RESERVED_484)


/*
    VPE_Reserved_485:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_485_OFS 0x0bd0
REGDEF_BEGIN(VPE_RESERVED_485)
REGDEF_BIT(VPE_Reserved_485,        32)
REGDEF_END(VPE_RESERVED_485)


/*
    VPE_Reserved_486:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_486_OFS 0x0bd4
REGDEF_BEGIN(VPE_RESERVED_486)
REGDEF_BIT(VPE_Reserved_486,        32)
REGDEF_END(VPE_RESERVED_486)


/*
    VPE_Reserved_487:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_487_OFS 0x0bd8
REGDEF_BEGIN(VPE_RESERVED_487)
REGDEF_BIT(VPE_Reserved_487,        32)
REGDEF_END(VPE_RESERVED_487)


/*
    VPE_Reserved_488:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_488_OFS 0x0bdc
REGDEF_BEGIN(VPE_RESERVED_488)
REGDEF_BIT(VPE_Reserved_488,        32)
REGDEF_END(VPE_RESERVED_488)


/*
    VPE_Reserved_489:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_489_OFS 0x0be0
REGDEF_BEGIN(VPE_RESERVED_489)
REGDEF_BIT(VPE_Reserved_489,        32)
REGDEF_END(VPE_RESERVED_489)


/*
    VPE_Reserved_490:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_490_OFS 0x0be4
REGDEF_BEGIN(VPE_RESERVED_490)
REGDEF_BIT(VPE_Reserved_490,        32)
REGDEF_END(VPE_RESERVED_490)


/*
    VPE_Reserved_491:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_491_OFS 0x0be8
REGDEF_BEGIN(VPE_RESERVED_491)
REGDEF_BIT(VPE_Reserved_491,        32)
REGDEF_END(VPE_RESERVED_491)


/*
    VPE_Reserved_492:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_492_OFS 0x0bec
REGDEF_BEGIN(VPE_RESERVED_492)
REGDEF_BIT(VPE_Reserved_492,        32)
REGDEF_END(VPE_RESERVED_492)


/*
    VPE_Reserved_493:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_493_OFS 0x0bf0
REGDEF_BEGIN(VPE_RESERVED_493)
REGDEF_BIT(VPE_Reserved_493,        32)
REGDEF_END(VPE_RESERVED_493)


/*
    VPE_Reserved_494:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_494_OFS 0x0bf4
REGDEF_BEGIN(VPE_RESERVED_494)
REGDEF_BIT(VPE_Reserved_494,        32)
REGDEF_END(VPE_RESERVED_494)


/*
    VPE_Reserved_495:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_495_OFS 0x0bf8
REGDEF_BEGIN(VPE_RESERVED_495)
REGDEF_BIT(VPE_Reserved_495,        32)
REGDEF_END(VPE_RESERVED_495)


/*
    VPE_Reserved_496:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_496_OFS 0x0bfc
REGDEF_BEGIN(VPE_RESERVED_496)
REGDEF_BIT(VPE_Reserved_496,        32)
REGDEF_END(VPE_RESERVED_496)


/*
    RES2_SCA_EN        :    [0x0, 0x1],         bits : 0
    RES2_SCA_BYPASS_EN :    [0x0, 0x1],         bits : 1
    RES2_SCA_CROP_EN   :    [0x0, 0x1],         bits : 2
    RES2_TC_EN         :    [0x0, 0x1],         bits : 3
    RES2_SCA_CHROMA_SEL:    [0x0, 0x1],         bits : 4
    RES2_DES_DRT       :    [0x0, 0x3],         bits : 9_8
    RES2_DES_YUV420    :    [0x0, 0x1],         bits : 11
    RES2_DES_UV_SWAP   :    [0x0, 0x1],         bits : 15
    RES2_OUT_BG_SEL    :    [0x0, 0x7],         bits : 18_16
*/
#define RES2_CONTROL_1_REGISTER_OFS 0x0c00
REGDEF_BEGIN(RES2_CONTROL_1_REGISTER)
REGDEF_BIT(RES2_SCA_EN,        1)
REGDEF_BIT(RES2_SCA_BYPASS_EN,        1)
REGDEF_BIT(RES2_SCA_CROP_EN,        1)
REGDEF_BIT(RES2_TC_EN,        1)
REGDEF_BIT(RES2_SCA_CHROMA_SEL,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_DES_DRT,        2)
REGDEF_BIT(,        1)
REGDEF_BIT(RES2_DES_YUV420,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_DES_UV_SWAP,        1)
REGDEF_BIT(RES2_OUT_BG_SEL,        3)
REGDEF_END(RES2_CONTROL_1_REGISTER)


/*
    RES2_DES_DP0_CHRW  :    [0x0, 0x1],         bits : 4
    RES2_DES_DP0_FORMAT:    [0x0, 0x3],         bits : 9_8
*/
#define RES2_CONTROL_2_REGISTER_OFS 0x0c04
REGDEF_BEGIN(RES2_CONTROL_2_REGISTER)
REGDEF_BIT(,        4)
REGDEF_BIT(RES2_DES_DP0_CHRW,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_DES_DP0_FORMAT,        2)
REGDEF_END(RES2_CONTROL_2_REGISTER)


/*
    RES2_SCA_HEIGHT:    [0x0, 0x1fff],          bits : 12_0
*/
#define RES2_SIZE_1_REGISTER_OFS 0x0c08
REGDEF_BEGIN(RES2_SIZE_1_REGISTER)
REGDEF_BIT(RES2_SCA_HEIGHT,        13)
REGDEF_END(RES2_SIZE_1_REGISTER)


/*
    RES2_DES_BUF_WIDTH :    [0x0, 0x3fff],          bits : 13_0
    RES2_DES_BUF_HEIGHT:    [0x0, 0x1fff],          bits : 28_16
*/
#define RES2_SIZE_2_REGISTER_OFS 0x0c0c
REGDEF_BEGIN(RES2_SIZE_2_REGISTER)
REGDEF_BIT(RES2_DES_BUF_WIDTH,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(RES2_DES_BUF_HEIGHT,        13)
REGDEF_END(RES2_SIZE_2_REGISTER)


/*
    RES2_OUT_Y_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_OUT_HEIGHT :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_SIZE_3_REGISTER_OFS 0x0c10
REGDEF_BEGIN(RES2_SIZE_3_REGISTER)
REGDEF_BIT(RES2_OUT_Y_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_OUT_HEIGHT,        13)
REGDEF_END(RES2_SIZE_3_REGISTER)


/*
    RES2_RLT_Y_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_RLT_HEIGHT :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_SIZE_4_REGISTER_OFS 0x0c14
REGDEF_BEGIN(RES2_SIZE_4_REGISTER)
REGDEF_BIT(RES2_RLT_Y_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_RLT_HEIGHT,        13)
REGDEF_END(RES2_SIZE_4_REGISTER)


/*
    RES2_PIP_Y_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_PIP_HEIGHT :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_SIZE_5_REGISTER_OFS 0x0c18
REGDEF_BEGIN(RES2_SIZE_5_REGISTER)
REGDEF_BIT(RES2_PIP_Y_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_PIP_HEIGHT,        13)
REGDEF_END(RES2_SIZE_5_REGISTER)


/*
    RES2_SCA_CROP_Y_START:    [0x0, 0xfff],         bits : 11_0
    RES2_SCA_CROP_HEIGHT :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES2_SCA_CROP_REGISTER_OFS 0x0c1c
REGDEF_BEGIN(RES2_SCA_CROP_REGISTER)
REGDEF_BIT(RES2_SCA_CROP_Y_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES2_SCA_CROP_HEIGHT,        13)
REGDEF_END(RES2_SCA_CROP_REGISTER)


/*
    RES2_SCA_Y_LUMA_ALGO_EN  :    [0x0, 0x3],           bits : 1_0
    RES2_SCA_X_LUMA_ALGO_EN  :    [0x0, 0x3],           bits : 3_2
    RES2_SCA_Y_CHROMA_ALGO_EN:    [0x0, 0x3],           bits : 5_4
    RES2_SCA_X_CHROMA_ALGO_EN:    [0x0, 0x3],           bits : 7_6
    RES2_SCA_MAP_SEL         :    [0x0, 0x1],           bits : 8
*/
#define RES2_SCA_1_REGISTER_OFS 0x0c20
REGDEF_BEGIN(RES2_SCA_1_REGISTER)
REGDEF_BIT(RES2_SCA_Y_LUMA_ALGO_EN,        2)
REGDEF_BIT(RES2_SCA_X_LUMA_ALGO_EN,        2)
REGDEF_BIT(RES2_SCA_Y_CHROMA_ALGO_EN,        2)
REGDEF_BIT(RES2_SCA_X_CHROMA_ALGO_EN,        2)
REGDEF_BIT(RES2_SCA_MAP_SEL,        1)
REGDEF_END(RES2_SCA_1_REGISTER)


/*
    RES2_SCA_HSTEP:    [0x0, 0x3fff],           bits : 13_0
    RES2_SCA_VSTEP:    [0x0, 0x3fff],           bits : 29_16
*/
#define RES2_SCA_2_REGISTER_OFS 0x0c24
REGDEF_BEGIN(RES2_SCA_2_REGISTER)
REGDEF_BIT(RES2_SCA_HSTEP,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(RES2_SCA_VSTEP,        14)
REGDEF_END(RES2_SCA_2_REGISTER)


/*
    RES2_SCA_HSCA_DIVISOR:    [0x0, 0x1fff],            bits : 12_0
    RES2_SCA_HAVG_PXL_MSK:    [0x0, 0xff],          bits : 23_16
*/
#define RES2_SCA_3_REGISTER_OFS 0x0c28
REGDEF_BEGIN(RES2_SCA_3_REGISTER)
REGDEF_BIT(RES2_SCA_HSCA_DIVISOR,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_SCA_HAVG_PXL_MSK,        8)
REGDEF_END(RES2_SCA_3_REGISTER)


/*
    RES2_SCA_VSCA_DIVISOR:    [0x0, 0x1fff],            bits : 12_0
    RES2_SCA_VAVG_PXL_MSK:    [0x0, 0xff],          bits : 23_16
*/
#define RES2_SCA_4_REGISTER_OFS 0x0c2c
REGDEF_BEGIN(RES2_SCA_4_REGISTER)
REGDEF_BIT(RES2_SCA_VSCA_DIVISOR,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_SCA_VAVG_PXL_MSK,        8)
REGDEF_END(RES2_SCA_4_REGISTER)


/*
    RES2_SCA_COEF_H0:    [0x0, 0x3ff],          bits : 9_0
    RES2_SCA_COEF_H1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RES2_SCA_5_REGISTER_OFS 0x0c30
REGDEF_BEGIN(RES2_SCA_5_REGISTER)
REGDEF_BIT(RES2_SCA_COEF_H0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(RES2_SCA_COEF_H1,        10)
REGDEF_END(RES2_SCA_5_REGISTER)


/*
    RES2_SCA_COEF_H2:    [0x0, 0x3ff],          bits : 9_0
    RES2_SCA_COEF_H3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RES2_SCA_6_REGISTER_OFS 0x0c34
REGDEF_BEGIN(RES2_SCA_6_REGISTER)
REGDEF_BIT(RES2_SCA_COEF_H2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(RES2_SCA_COEF_H3,        10)
REGDEF_END(RES2_SCA_6_REGISTER)


/*
    RES2_SCA_COEF_V0:    [0x0, 0x3ff],          bits : 9_0
    RES2_SCA_COEF_V1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RES2_SCA_7_REGISTER_OFS 0x0c38
REGDEF_BEGIN(RES2_SCA_7_REGISTER)
REGDEF_BIT(RES2_SCA_COEF_V0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(RES2_SCA_COEF_V1,        10)
REGDEF_END(RES2_SCA_7_REGISTER)


/*
    RES2_SCA_COEF_V2:    [0x0, 0x3ff],          bits : 9_0
    RES2_SCA_COEF_V3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RES2_SCA_8_REGISTER_OFS 0x0c3c
REGDEF_BEGIN(RES2_SCA_8_REGISTER)
REGDEF_BIT(RES2_SCA_COEF_V2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(RES2_SCA_COEF_V3,        10)
REGDEF_END(RES2_SCA_8_REGISTER)


/*
    RES2_TC_CROP_Y_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_TC_CROP_HEIGHT :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_TC_REGISTER_OFS 0x0c40
REGDEF_BEGIN(RES2_TC_REGISTER)
REGDEF_BIT(RES2_TC_CROP_Y_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_TC_CROP_HEIGHT,        13)
REGDEF_END(RES2_TC_REGISTER)


/*
    VPE_Reserved_497:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_497_OFS 0x0c44
REGDEF_BEGIN(VPE_RESERVED_497)
REGDEF_BIT(VPE_Reserved_497,        32)
REGDEF_END(VPE_RESERVED_497)


/*
    VPE_Reserved_498:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_498_OFS 0x0c48
REGDEF_BEGIN(VPE_RESERVED_498)
REGDEF_BIT(VPE_Reserved_498,        32)
REGDEF_END(VPE_RESERVED_498)


/*
    VPE_Reserved_499:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_499_OFS 0x0c4c
REGDEF_BEGIN(VPE_RESERVED_499)
REGDEF_BIT(VPE_Reserved_499,        32)
REGDEF_END(VPE_RESERVED_499)


/*
    VPE_Reserved_500:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_500_OFS 0x0c50
REGDEF_BEGIN(VPE_RESERVED_500)
REGDEF_BIT(VPE_Reserved_500,        32)
REGDEF_END(VPE_RESERVED_500)


/*
    VPE_Reserved_501:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_501_OFS 0x0c54
REGDEF_BEGIN(VPE_RESERVED_501)
REGDEF_BIT(VPE_Reserved_501,        32)
REGDEF_END(VPE_RESERVED_501)


/*
    VPE_Reserved_502:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_502_OFS 0x0c58
REGDEF_BEGIN(VPE_RESERVED_502)
REGDEF_BIT(VPE_Reserved_502,        32)
REGDEF_END(VPE_RESERVED_502)


/*
    VPE_Reserved_503:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_503_OFS 0x0c5c
REGDEF_BEGIN(VPE_RESERVED_503)
REGDEF_BIT(VPE_Reserved_503,        32)
REGDEF_END(VPE_RESERVED_503)


/*
    VPE_Reserved_504:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_504_OFS 0x0c60
REGDEF_BEGIN(VPE_RESERVED_504)
REGDEF_BIT(VPE_Reserved_504,        32)
REGDEF_END(VPE_RESERVED_504)


/*
    VPE_Reserved_505:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_505_OFS 0x0c64
REGDEF_BEGIN(VPE_RESERVED_505)
REGDEF_BIT(VPE_Reserved_505,        32)
REGDEF_END(VPE_RESERVED_505)


/*
    VPE_Reserved_506:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_506_OFS 0x0c68
REGDEF_BEGIN(VPE_RESERVED_506)
REGDEF_BIT(VPE_Reserved_506,        32)
REGDEF_END(VPE_RESERVED_506)


/*
    VPE_Reserved_507:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_507_OFS 0x0c6c
REGDEF_BEGIN(VPE_RESERVED_507)
REGDEF_BIT(VPE_Reserved_507,        32)
REGDEF_END(VPE_RESERVED_507)


/*
    VPE_Reserved_508:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_508_OFS 0x0c70
REGDEF_BEGIN(VPE_RESERVED_508)
REGDEF_BIT(VPE_Reserved_508,        32)
REGDEF_END(VPE_RESERVED_508)


/*
    VPE_Reserved_509:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_509_OFS 0x0c74
REGDEF_BEGIN(VPE_RESERVED_509)
REGDEF_BIT(VPE_Reserved_509,        32)
REGDEF_END(VPE_RESERVED_509)


/*
    VPE_Reserved_510:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_510_OFS 0x0c78
REGDEF_BEGIN(VPE_RESERVED_510)
REGDEF_BIT(VPE_Reserved_510,        32)
REGDEF_END(VPE_RESERVED_510)


/*
    VPE_Reserved_511:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_511_OFS 0x0c7c
REGDEF_BEGIN(VPE_RESERVED_511)
REGDEF_BIT(VPE_Reserved_511,        32)
REGDEF_END(VPE_RESERVED_511)


/*
    RES3_SCA_EN        :    [0x0, 0x1],         bits : 0
    RES3_SCA_BYPASS_EN :    [0x0, 0x1],         bits : 1
    RES3_SCA_CROP_EN   :    [0x0, 0x1],         bits : 2
    RES3_TC_EN         :    [0x0, 0x1],         bits : 3
    RES3_SCA_CHROMA_SEL:    [0x0, 0x1],         bits : 4
    RES3_DES_DRT       :    [0x0, 0x3],         bits : 9_8
    DES_YUV420         :    [0x0, 0x1],         bits : 11
    RES3_DES_UV_SWAP   :    [0x0, 0x1],         bits : 15
    RES3_OUT_BG_SEL    :    [0x0, 0x7],         bits : 18_16
*/
#define RES3_CONTROL_1_REGISTER_OFS 0x0c80
REGDEF_BEGIN(RES3_CONTROL_1_REGISTER)
REGDEF_BIT(RES3_SCA_EN,        1)
REGDEF_BIT(RES3_SCA_BYPASS_EN,        1)
REGDEF_BIT(RES3_SCA_CROP_EN,        1)
REGDEF_BIT(RES3_TC_EN,        1)
REGDEF_BIT(RES3_SCA_CHROMA_SEL,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_DES_DRT,        2)
REGDEF_BIT(,        1)
REGDEF_BIT(DES_YUV420,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_DES_UV_SWAP,        1)
REGDEF_BIT(RES3_OUT_BG_SEL,        3)
REGDEF_END(RES3_CONTROL_1_REGISTER)


/*
    RES3_DES_DP0_CHRW  :    [0x0, 0x1],         bits : 4
    RES3_DES_DP0_FORMAT:    [0x0, 0x3],         bits : 9_8
*/
#define RES3_CONTROL_2_REGISTER_OFS 0x0c84
REGDEF_BEGIN(RES3_CONTROL_2_REGISTER)
REGDEF_BIT(,        4)
REGDEF_BIT(RES3_DES_DP0_CHRW,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_DES_DP0_FORMAT,        2)
REGDEF_END(RES3_CONTROL_2_REGISTER)


/*
    RES3_SCA_HEIGHT:    [0x0, 0x1fff],          bits : 12_0
*/
#define RES3_SIZE_1_REGISTER_OFS 0x0c88
REGDEF_BEGIN(RES3_SIZE_1_REGISTER)
REGDEF_BIT(RES3_SCA_HEIGHT,        13)
REGDEF_END(RES3_SIZE_1_REGISTER)


/*
    RES3_DES_BUF_WIDTH :    [0x0, 0x3fff],          bits : 13_0
    RES3_DES_BUF_HEIGHT:    [0x0, 0x1fff],          bits : 28_16
*/
#define RES3_SIZE_2_REGISTER_OFS 0x0c8c
REGDEF_BEGIN(RES3_SIZE_2_REGISTER)
REGDEF_BIT(RES3_DES_BUF_WIDTH,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(RES3_DES_BUF_HEIGHT,        13)
REGDEF_END(RES3_SIZE_2_REGISTER)


/*
    RES3_OUT_Y_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_OUT_HEIGHT :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_SIZE_3_REGISTER_OFS 0x0c90
REGDEF_BEGIN(RES3_SIZE_3_REGISTER)
REGDEF_BIT(RES3_OUT_Y_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_OUT_HEIGHT,        13)
REGDEF_END(RES3_SIZE_3_REGISTER)


/*
    RES3_RLT_Y_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_RLT_HEIGHT :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_SIZE_4_REGISTER_OFS 0x0c94
REGDEF_BEGIN(RES3_SIZE_4_REGISTER)
REGDEF_BIT(RES3_RLT_Y_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_RLT_HEIGHT,        13)
REGDEF_END(RES3_SIZE_4_REGISTER)


/*
    RES3_PIP_Y_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_PIP_HEIGHT :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_SIZE_5_REGISTER_OFS 0x0c98
REGDEF_BEGIN(RES3_SIZE_5_REGISTER)
REGDEF_BIT(RES3_PIP_Y_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_PIP_HEIGHT,        13)
REGDEF_END(RES3_SIZE_5_REGISTER)


/*
    RES3_SCA_CROP_Y_START:    [0x0, 0xfff],         bits : 11_0
    RES3_SCA_CROP_HEIGHT :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES3_SCA_CROP_REGISTER_OFS 0x0c9c
REGDEF_BEGIN(RES3_SCA_CROP_REGISTER)
REGDEF_BIT(RES3_SCA_CROP_Y_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES3_SCA_CROP_HEIGHT,        13)
REGDEF_END(RES3_SCA_CROP_REGISTER)


/*
    RES3_SCA_Y_LUMA_ALGO_EN  :    [0x0, 0x3],           bits : 1_0
    RES3_SCA_X_LUMA_ALGO_EN  :    [0x0, 0x3],           bits : 3_2
    RES3_SCA_Y_CHROMA_ALGO_EN:    [0x0, 0x3],           bits : 5_4
    RES3_SCA_X_CHROMA_ALGO_EN:    [0x0, 0x3],           bits : 7_6
    RES3_SCA_MAP_SEL         :    [0x0, 0x1],           bits : 8
*/
#define RES3_SCA_1_REGISTER_OFS 0x0ca0
REGDEF_BEGIN(RES3_SCA_1_REGISTER)
REGDEF_BIT(RES3_SCA_Y_LUMA_ALGO_EN,        2)
REGDEF_BIT(RES3_SCA_X_LUMA_ALGO_EN,        2)
REGDEF_BIT(RES3_SCA_Y_CHROMA_ALGO_EN,        2)
REGDEF_BIT(RES3_SCA_X_CHROMA_ALGO_EN,        2)
REGDEF_BIT(RES3_SCA_MAP_SEL,        1)
REGDEF_END(RES3_SCA_1_REGISTER)


/*
    RES3_SCA_HSTEP:    [0x0, 0x3fff],           bits : 13_0
    RES3_SCA_VSTEP:    [0x0, 0x3fff],           bits : 29_16
*/
#define RES3_SCA_2_REGISTER_OFS 0x0ca4
REGDEF_BEGIN(RES3_SCA_2_REGISTER)
REGDEF_BIT(RES3_SCA_HSTEP,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(RES3_SCA_VSTEP,        14)
REGDEF_END(RES3_SCA_2_REGISTER)


/*
    RES3_SCA_HSCA_DIVISOR:    [0x0, 0x1fff],            bits : 12_0
    RES3_SCA_HAVG_PXL_MSK:    [0x0, 0xff],          bits : 23_16
*/
#define RES3_SCA_3_REGISTER_OFS 0x0ca8
REGDEF_BEGIN(RES3_SCA_3_REGISTER)
REGDEF_BIT(RES3_SCA_HSCA_DIVISOR,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_SCA_HAVG_PXL_MSK,        8)
REGDEF_END(RES3_SCA_3_REGISTER)


/*
    RES3_SCA_VSCA_DIVISOR:    [0x0, 0x1fff],            bits : 12_0
    RES3_SCA_VAVG_PXL_MSK:    [0x0, 0xff],          bits : 23_16
*/
#define RES3_SCA_4_REGISTER_OFS 0x0cac
REGDEF_BEGIN(RES3_SCA_4_REGISTER)
REGDEF_BIT(RES3_SCA_VSCA_DIVISOR,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_SCA_VAVG_PXL_MSK,        8)
REGDEF_END(RES3_SCA_4_REGISTER)


/*
    RES3_SCA_COEF_H0:    [0x0, 0x3ff],          bits : 9_0
    RES3_SCA_COEF_H1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RES3_SCA_5_REGISTER_OFS 0x0cb0
REGDEF_BEGIN(RES3_SCA_5_REGISTER)
REGDEF_BIT(RES3_SCA_COEF_H0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(RES3_SCA_COEF_H1,        10)
REGDEF_END(RES3_SCA_5_REGISTER)


/*
    RES3_SCA_COEF_H2:    [0x0, 0x3ff],          bits : 9_0
    RES3_SCA_COEF_H3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RES3_SCA_6_REGISTER_OFS 0x0cb4
REGDEF_BEGIN(RES3_SCA_6_REGISTER)
REGDEF_BIT(RES3_SCA_COEF_H2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(RES3_SCA_COEF_H3,        10)
REGDEF_END(RES3_SCA_6_REGISTER)


/*
    RES3_SCA_COEF_V0:    [0x0, 0x3ff],          bits : 9_0
    RES3_SCA_COEF_V1:    [0x0, 0x3ff],          bits : 25_16
*/
#define RES3_SCA_7_REGISTER_OFS 0x0cb8
REGDEF_BEGIN(RES3_SCA_7_REGISTER)
REGDEF_BIT(RES3_SCA_COEF_V0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(RES3_SCA_COEF_V1,        10)
REGDEF_END(RES3_SCA_7_REGISTER)


/*
    RES3_SCA_COEF_V2:    [0x0, 0x3ff],          bits : 9_0
    RES3_SCA_COEF_V3:    [0x0, 0x3ff],          bits : 25_16
*/
#define RES3_SCA_8_REGISTER_OFS 0x0cbc
REGDEF_BEGIN(RES3_SCA_8_REGISTER)
REGDEF_BIT(RES3_SCA_COEF_V2,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(RES3_SCA_COEF_V3,        10)
REGDEF_END(RES3_SCA_8_REGISTER)


/*
    RES3_TC_CROP_Y_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_TC_CROP_HEIGHT :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_TC_REGISTER_OFS 0x0cc0
REGDEF_BEGIN(RES3_TC_REGISTER)
REGDEF_BIT(RES3_TC_CROP_Y_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_TC_CROP_HEIGHT,        13)
REGDEF_END(RES3_TC_REGISTER)


/*
    VPE_Reserved_512:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_512_OFS 0x0cc4
REGDEF_BEGIN(VPE_RESERVED_512)
REGDEF_BIT(VPE_Reserved_512,        32)
REGDEF_END(VPE_RESERVED_512)


/*
    VPE_Reserved_513:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_513_OFS 0x0cc8
REGDEF_BEGIN(VPE_RESERVED_513)
REGDEF_BIT(VPE_Reserved_513,        32)
REGDEF_END(VPE_RESERVED_513)


/*
    VPE_Reserved_514:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_514_OFS 0x0ccc
REGDEF_BEGIN(VPE_RESERVED_514)
REGDEF_BIT(VPE_Reserved_514,        32)
REGDEF_END(VPE_RESERVED_514)


/*
    VPE_Reserved_515:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_515_OFS 0x0cd0
REGDEF_BEGIN(VPE_RESERVED_515)
REGDEF_BIT(VPE_Reserved_515,        32)
REGDEF_END(VPE_RESERVED_515)


/*
    VPE_Reserved_516:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_516_OFS 0x0cd4
REGDEF_BEGIN(VPE_RESERVED_516)
REGDEF_BIT(VPE_Reserved_516,        32)
REGDEF_END(VPE_RESERVED_516)


/*
    VPE_Reserved_517:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_517_OFS 0x0cd8
REGDEF_BEGIN(VPE_RESERVED_517)
REGDEF_BIT(VPE_Reserved_517,        32)
REGDEF_END(VPE_RESERVED_517)


/*
    VPE_Reserved_518:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_518_OFS 0x0cdc
REGDEF_BEGIN(VPE_RESERVED_518)
REGDEF_BIT(VPE_Reserved_518,        32)
REGDEF_END(VPE_RESERVED_518)


/*
    VPE_Reserved_519:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_519_OFS 0x0ce0
REGDEF_BEGIN(VPE_RESERVED_519)
REGDEF_BIT(VPE_Reserved_519,        32)
REGDEF_END(VPE_RESERVED_519)


/*
    VPE_Reserved_520:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_520_OFS 0x0ce4
REGDEF_BEGIN(VPE_RESERVED_520)
REGDEF_BIT(VPE_Reserved_520,        32)
REGDEF_END(VPE_RESERVED_520)


/*
    VPE_Reserved_521:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_521_OFS 0x0ce8
REGDEF_BEGIN(VPE_RESERVED_521)
REGDEF_BIT(VPE_Reserved_521,        32)
REGDEF_END(VPE_RESERVED_521)


/*
    VPE_Reserved_522:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_522_OFS 0x0cec
REGDEF_BEGIN(VPE_RESERVED_522)
REGDEF_BIT(VPE_Reserved_522,        32)
REGDEF_END(VPE_RESERVED_522)


/*
    VPE_Reserved_523:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_523_OFS 0x0cf0
REGDEF_BEGIN(VPE_RESERVED_523)
REGDEF_BIT(VPE_Reserved_523,        32)
REGDEF_END(VPE_RESERVED_523)


/*
    VPE_Reserved_524:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_524_OFS 0x0cf4
REGDEF_BEGIN(VPE_RESERVED_524)
REGDEF_BIT(VPE_Reserved_524,        32)
REGDEF_END(VPE_RESERVED_524)


/*
    VPE_Reserved_525:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_525_OFS 0x0cf8
REGDEF_BEGIN(VPE_RESERVED_525)
REGDEF_BIT(VPE_Reserved_525,        32)
REGDEF_END(VPE_RESERVED_525)


/*
    VPE_Reserved_526:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_526_OFS 0x0cfc
REGDEF_BEGIN(VPE_RESERVED_526)
REGDEF_BIT(VPE_Reserved_526,        32)
REGDEF_END(VPE_RESERVED_526)


/*
    RES2_CL0_SCA_WIDTH    :    [0x0, 0x1fff],           bits : 12_0
    RES2_CL0_SCA_HSTEP_OFT:    [0x0, 0x7fff],           bits : 30_16
*/
#define RES2_COL0_SIZE_1_REGISTER_OFS 0x0d00
REGDEF_BEGIN(RES2_COL0_SIZE_1_REGISTER)
REGDEF_BIT(RES2_CL0_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL0_SCA_HSTEP_OFT,        15)
REGDEF_END(RES2_COL0_SIZE_1_REGISTER)


/*
    RES2_CL0_OUT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL0_OUT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL0_SIZE_2_REGISTER_OFS 0x0d04
REGDEF_BEGIN(RES2_COL0_SIZE_2_REGISTER)
REGDEF_BIT(RES2_CL0_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL0_OUT_WIDTH,        13)
REGDEF_END(RES2_COL0_SIZE_2_REGISTER)


/*
    RES2_CL0_RLT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL0_RLT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL0_SIZE_3_REGISTER_OFS 0x0d08
REGDEF_BEGIN(RES2_COL0_SIZE_3_REGISTER)
REGDEF_BIT(RES2_CL0_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL0_RLT_WIDTH,        13)
REGDEF_END(RES2_COL0_SIZE_3_REGISTER)


/*
    RES2_CL0_PIP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL0_PIP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL0_SIZE_4_REGISTER_OFS 0x0d0c
REGDEF_BEGIN(RES2_COL0_SIZE_4_REGISTER)
REGDEF_BIT(RES2_CL0_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL0_PIP_WIDTH,        13)
REGDEF_END(RES2_COL0_SIZE_4_REGISTER)


/*
    RES2_CL0_SCA_CROP_X_START:    [0x0, 0xfff],         bits : 11_0
    RES2_CL0_SCA_CROP_WIDTH  :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES2_COL0_SIZE_5_REGISTER_OFS 0x0d10
REGDEF_BEGIN(RES2_COL0_SIZE_5_REGISTER)
REGDEF_BIT(RES2_CL0_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES2_CL0_SCA_CROP_WIDTH,        13)
REGDEF_END(RES2_COL0_SIZE_5_REGISTER)


/*
    RES2_CL0_TC_CROP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL0_TC_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
    RES2_CL0_TC_CROP_SKIP   :    [0x0, 0x1],            bits : 31
*/
#define RES2_COL0_SIZE_6_REGISTER_OFS 0x0d14
REGDEF_BEGIN(RES2_COL0_SIZE_6_REGISTER)
REGDEF_BIT(RES2_CL0_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL0_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RES2_CL0_TC_CROP_SKIP,        1)
REGDEF_END(RES2_COL0_SIZE_6_REGISTER)


/*
    VPE_Reserved_527:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_527_OFS 0x0d18
REGDEF_BEGIN(VPE_RESERVED_527)
REGDEF_BIT(VPE_Reserved_527,        32)
REGDEF_END(VPE_RESERVED_527)


/*
    VPE_Reserved_528:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_528_OFS 0x0d1c
REGDEF_BEGIN(VPE_RESERVED_528)
REGDEF_BIT(VPE_Reserved_528,        32)
REGDEF_END(VPE_RESERVED_528)


/*
    RES2_CL1_SCA_WIDTH    :    [0x0, 0x1fff],           bits : 12_0
    RES2_CL1_SCA_HSTEP_OFT:    [0x0, 0x7fff],           bits : 30_16
*/
#define RES2_COL1_SIZE_1_REGISTER_OFS 0x0d20
REGDEF_BEGIN(RES2_COL1_SIZE_1_REGISTER)
REGDEF_BIT(RES2_CL1_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL1_SCA_HSTEP_OFT,        15)
REGDEF_END(RES2_COL1_SIZE_1_REGISTER)


/*
    RES2_CL1_OUT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL1_OUT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL1_SIZE_2_REGISTER_OFS 0x0d24
REGDEF_BEGIN(RES2_COL1_SIZE_2_REGISTER)
REGDEF_BIT(RES2_CL1_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL1_OUT_WIDTH,        13)
REGDEF_END(RES2_COL1_SIZE_2_REGISTER)


/*
    RES2_CL1_RLT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL1_RLT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL1_SIZE_3_REGISTER_OFS 0x0d28
REGDEF_BEGIN(RES2_COL1_SIZE_3_REGISTER)
REGDEF_BIT(RES2_CL1_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL1_RLT_WIDTH,        13)
REGDEF_END(RES2_COL1_SIZE_3_REGISTER)


/*
    RES2_CL1_PIP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL1_PIP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL1_SIZE_4_REGISTER_OFS 0x0d2c
REGDEF_BEGIN(RES2_COL1_SIZE_4_REGISTER)
REGDEF_BIT(RES2_CL1_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL1_PIP_WIDTH,        13)
REGDEF_END(RES2_COL1_SIZE_4_REGISTER)


/*
    RES2_CL1_SCA_CROP_X_START:    [0x0, 0xfff],         bits : 11_0
    RES2_CL1_SCA_CROP_WIDTH  :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES2_COL1_SIZE_5_REGISTER_OFS 0x0d30
REGDEF_BEGIN(RES2_COL1_SIZE_5_REGISTER)
REGDEF_BIT(RES2_CL1_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES2_CL1_SCA_CROP_WIDTH,        13)
REGDEF_END(RES2_COL1_SIZE_5_REGISTER)


/*
    RES2_CL1_TC_CROP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL1_TC_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
    RES2_CL1_TC_CROP_SKIP   :    [0x0, 0x1],            bits : 31
*/
#define RES2_COL1_SIZE_6_REGISTER_OFS 0x0d34
REGDEF_BEGIN(RES2_COL1_SIZE_6_REGISTER)
REGDEF_BIT(RES2_CL1_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL1_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RES2_CL1_TC_CROP_SKIP,        1)
REGDEF_END(RES2_COL1_SIZE_6_REGISTER)


/*
    VPE_Reserved_529:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_529_OFS 0x0d38
REGDEF_BEGIN(VPE_RESERVED_529)
REGDEF_BIT(VPE_Reserved_529,        32)
REGDEF_END(VPE_RESERVED_529)


/*
    VPE_Reserved_530:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_530_OFS 0x0d3c
REGDEF_BEGIN(VPE_RESERVED_530)
REGDEF_BIT(VPE_Reserved_530,        32)
REGDEF_END(VPE_RESERVED_530)


/*
    RES2_CL2_SCA_WIDTH    :    [0x0, 0x1fff],           bits : 12_0
    RES2_CL2_SCA_HSTEP_OFT:    [0x0, 0x7fff],           bits : 30_16
*/
#define RES2_COL2_SIZE_1_REGISTER_OFS 0x0d40
REGDEF_BEGIN(RES2_COL2_SIZE_1_REGISTER)
REGDEF_BIT(RES2_CL2_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL2_SCA_HSTEP_OFT,        15)
REGDEF_END(RES2_COL2_SIZE_1_REGISTER)


/*
    RES2_CL2_OUT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL2_OUT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL2_SIZE_2_REGISTER_OFS 0x0d44
REGDEF_BEGIN(RES2_COL2_SIZE_2_REGISTER)
REGDEF_BIT(RES2_CL2_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL2_OUT_WIDTH,        13)
REGDEF_END(RES2_COL2_SIZE_2_REGISTER)


/*
    RES2_CL2_RLT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL2_RLT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL2_SIZE_3_REGISTER_OFS 0x0d48
REGDEF_BEGIN(RES2_COL2_SIZE_3_REGISTER)
REGDEF_BIT(RES2_CL2_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL2_RLT_WIDTH,        13)
REGDEF_END(RES2_COL2_SIZE_3_REGISTER)


/*
    RES2_CL2_PIP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL2_PIP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL2_SIZE_4_REGISTER_OFS 0x0d4c
REGDEF_BEGIN(RES2_COL2_SIZE_4_REGISTER)
REGDEF_BIT(RES2_CL2_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL2_PIP_WIDTH,        13)
REGDEF_END(RES2_COL2_SIZE_4_REGISTER)


/*
    RES2_CL2_SCA_CROP_X_START:    [0x0, 0xfff],         bits : 11_0
    RES2_CL2_SCA_CROP_WIDTH  :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES2_COL2_SIZE_5_REGISTER_OFS 0x0d50
REGDEF_BEGIN(RES2_COL2_SIZE_5_REGISTER)
REGDEF_BIT(RES2_CL2_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES2_CL2_SCA_CROP_WIDTH,        13)
REGDEF_END(RES2_COL2_SIZE_5_REGISTER)


/*
    RES2_CL2_TC_CROP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL2_TC_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
    RES2_CL2_TC_CROP_SKIP   :    [0x0, 0x1],            bits : 31
*/
#define RES2_COL2_SIZE_6_REGISTER_OFS 0x0d54
REGDEF_BEGIN(RES2_COL2_SIZE_6_REGISTER)
REGDEF_BIT(RES2_CL2_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL2_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RES2_CL2_TC_CROP_SKIP,        1)
REGDEF_END(RES2_COL2_SIZE_6_REGISTER)


/*
    VPE_Reserved_531:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_531_OFS 0x0d58
REGDEF_BEGIN(VPE_RESERVED_531)
REGDEF_BIT(VPE_Reserved_531,        32)
REGDEF_END(VPE_RESERVED_531)


/*
    VPE_Reserved_532:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_532_OFS 0x0d5c
REGDEF_BEGIN(VPE_RESERVED_532)
REGDEF_BIT(VPE_Reserved_532,        32)
REGDEF_END(VPE_RESERVED_532)


/*
    RES2_CL3_SCA_WIDTH    :    [0x0, 0x1fff],           bits : 12_0
    RES2_CL3_SCA_HSTEP_OFT:    [0x0, 0x7fff],           bits : 30_16
*/
#define RES2_COL3_SIZE_1_REGISTER_OFS 0x0d60
REGDEF_BEGIN(RES2_COL3_SIZE_1_REGISTER)
REGDEF_BIT(RES2_CL3_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL3_SCA_HSTEP_OFT,        15)
REGDEF_END(RES2_COL3_SIZE_1_REGISTER)


/*
    RES2_CL3_OUT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL3_OUT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL3_SIZE_2_REGISTER_OFS 0x0d64
REGDEF_BEGIN(RES2_COL3_SIZE_2_REGISTER)
REGDEF_BIT(RES2_CL3_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL3_OUT_WIDTH,        13)
REGDEF_END(RES2_COL3_SIZE_2_REGISTER)


/*
    RES2_CL3_RLT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL3_RLT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL3_SIZE_3_REGISTER_OFS 0x0d68
REGDEF_BEGIN(RES2_COL3_SIZE_3_REGISTER)
REGDEF_BIT(RES2_CL3_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL3_RLT_WIDTH,        13)
REGDEF_END(RES2_COL3_SIZE_3_REGISTER)


/*
    RES2_CL3_PIP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL3_PIP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL3_SIZE_4_REGISTER_OFS 0x0d6c
REGDEF_BEGIN(RES2_COL3_SIZE_4_REGISTER)
REGDEF_BIT(RES2_CL3_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL3_PIP_WIDTH,        13)
REGDEF_END(RES2_COL3_SIZE_4_REGISTER)


/*
    RES2_CL3_SCA_CROP_X_START:    [0x0, 0xfff],         bits : 11_0
    RES2_CL3_SCA_CROP_WIDTH  :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES2_COL3_SIZE_5_REGISTER_OFS 0x0d70
REGDEF_BEGIN(RES2_COL3_SIZE_5_REGISTER)
REGDEF_BIT(RES2_CL3_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES2_CL3_SCA_CROP_WIDTH,        13)
REGDEF_END(RES2_COL3_SIZE_5_REGISTER)


/*
    RES2_CL3_TC_CROP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL3_TC_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
    RES2_CL3_TC_CROP_SKIP   :    [0x0, 0x1],            bits : 31
*/
#define RES2_COL3_SIZE_6_REGISTER_OFS 0x0d74
REGDEF_BEGIN(RES2_COL3_SIZE_6_REGISTER)
REGDEF_BIT(RES2_CL3_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL3_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RES2_CL3_TC_CROP_SKIP,        1)
REGDEF_END(RES2_COL3_SIZE_6_REGISTER)


/*
    VPE_Reserved_533:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_533_OFS 0x0d78
REGDEF_BEGIN(VPE_RESERVED_533)
REGDEF_BIT(VPE_Reserved_533,        32)
REGDEF_END(VPE_RESERVED_533)


/*
    VPE_Reserved_534:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_534_OFS 0x0d7c
REGDEF_BEGIN(VPE_RESERVED_534)
REGDEF_BIT(VPE_Reserved_534,        32)
REGDEF_END(VPE_RESERVED_534)


/*
    RES2_CL4_SCA_WIDTH    :    [0x0, 0x1fff],           bits : 12_0
    RES2_CL4_SCA_HSTEP_OFT:    [0x0, 0x7fff],           bits : 30_16
*/
#define RES2_COL4_SIZE_1_REGISTER_OFS 0x0d80
REGDEF_BEGIN(RES2_COL4_SIZE_1_REGISTER)
REGDEF_BIT(RES2_CL4_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL4_SCA_HSTEP_OFT,        15)
REGDEF_END(RES2_COL4_SIZE_1_REGISTER)


/*
    RES2_CL4_OUT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL4_OUT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL4_SIZE_2_REGISTER_OFS 0x0d84
REGDEF_BEGIN(RES2_COL4_SIZE_2_REGISTER)
REGDEF_BIT(RES2_CL4_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL4_OUT_WIDTH,        13)
REGDEF_END(RES2_COL4_SIZE_2_REGISTER)


/*
    RES2_CL4_RLT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL4_RLT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL4_SIZE_3_REGISTER_OFS 0x0d88
REGDEF_BEGIN(RES2_COL4_SIZE_3_REGISTER)
REGDEF_BIT(RES2_CL4_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL4_RLT_WIDTH,        13)
REGDEF_END(RES2_COL4_SIZE_3_REGISTER)


/*
    RES2_CL4_PIP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL4_PIP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL4_SIZE_4_REGISTER_OFS 0x0d8c
REGDEF_BEGIN(RES2_COL4_SIZE_4_REGISTER)
REGDEF_BIT(RES2_CL4_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL4_PIP_WIDTH,        13)
REGDEF_END(RES2_COL4_SIZE_4_REGISTER)


/*
    RES2_CL4_SCA_CROP_X_START:    [0x0, 0xfff],         bits : 11_0
    RES2_CL4_SCA_CROP_WIDTH  :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES2_COL4_SIZE_5_REGISTER_OFS 0x0d90
REGDEF_BEGIN(RES2_COL4_SIZE_5_REGISTER)
REGDEF_BIT(RES2_CL4_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES2_CL4_SCA_CROP_WIDTH,        13)
REGDEF_END(RES2_COL4_SIZE_5_REGISTER)


/*
    RES2_CL4_TC_CROP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL4_TC_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
    RES2_CL4_TC_CROP_SKIP   :    [0x0, 0x1],            bits : 31
*/
#define RES2_COL4_SIZE_6_REGISTER_OFS 0x0d94
REGDEF_BEGIN(RES2_COL4_SIZE_6_REGISTER)
REGDEF_BIT(RES2_CL4_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL4_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RES2_CL4_TC_CROP_SKIP,        1)
REGDEF_END(RES2_COL4_SIZE_6_REGISTER)


/*
    VPE_Reserved_535:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_535_OFS 0x0d98
REGDEF_BEGIN(VPE_RESERVED_535)
REGDEF_BIT(VPE_Reserved_535,        32)
REGDEF_END(VPE_RESERVED_535)


/*
    VPE_Reserved_536:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_536_OFS 0x0d9c
REGDEF_BEGIN(VPE_RESERVED_536)
REGDEF_BIT(VPE_Reserved_536,        32)
REGDEF_END(VPE_RESERVED_536)


/*
    RES2_CL5_SCA_WIDTH    :    [0x0, 0x1fff],           bits : 12_0
    RES2_CL5_SCA_HSTEP_OFT:    [0x0, 0x7fff],           bits : 30_16
*/
#define RES2_COL5_SIZE_1_REGISTER_OFS 0x0da0
REGDEF_BEGIN(RES2_COL5_SIZE_1_REGISTER)
REGDEF_BIT(RES2_CL5_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL5_SCA_HSTEP_OFT,        15)
REGDEF_END(RES2_COL5_SIZE_1_REGISTER)


/*
    RES2_CL5_OUT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL5_OUT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL5_SIZE_2_REGISTER_OFS 0x0da4
REGDEF_BEGIN(RES2_COL5_SIZE_2_REGISTER)
REGDEF_BIT(RES2_CL5_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL5_OUT_WIDTH,        13)
REGDEF_END(RES2_COL5_SIZE_2_REGISTER)


/*
    RES2_CL5_RLT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL5_RLT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL5_SIZE_3_REGISTER_OFS 0x0da8
REGDEF_BEGIN(RES2_COL5_SIZE_3_REGISTER)
REGDEF_BIT(RES2_CL5_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL5_RLT_WIDTH,        13)
REGDEF_END(RES2_COL5_SIZE_3_REGISTER)


/*
    RES2_CL5_PIP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL5_PIP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL5_SIZE_4_REGISTER_OFS 0x0dac
REGDEF_BEGIN(RES2_COL5_SIZE_4_REGISTER)
REGDEF_BIT(RES2_CL5_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL5_PIP_WIDTH,        13)
REGDEF_END(RES2_COL5_SIZE_4_REGISTER)


/*
    RES2_CL5_SCA_CROP_X_START:    [0x0, 0xfff],         bits : 11_0
    RES2_CL5_SCA_CROP_WIDTH  :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES2_COL5_SIZE_5_REGISTER_OFS 0x0db0
REGDEF_BEGIN(RES2_COL5_SIZE_5_REGISTER)
REGDEF_BIT(RES2_CL5_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES2_CL5_SCA_CROP_WIDTH,        13)
REGDEF_END(RES2_COL5_SIZE_5_REGISTER)


/*
    RES2_CL5_TC_CROP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL5_TC_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
    TC_CROP_SKIP            :    [0x0, 0x1],            bits : 31
*/
#define RES2_COL5_SIZE_6_REGISTER_OFS 0x0db4
REGDEF_BEGIN(RES2_COL5_SIZE_6_REGISTER)
REGDEF_BIT(RES2_CL5_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL5_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(TC_CROP_SKIP,        1)
REGDEF_END(RES2_COL5_SIZE_6_REGISTER)


/*
    VPE_Reserved_537:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_537_OFS 0x0db8
REGDEF_BEGIN(VPE_RESERVED_537)
REGDEF_BIT(VPE_Reserved_537,        32)
REGDEF_END(VPE_RESERVED_537)


/*
    VPE_Reserved_538:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_538_OFS 0x0dbc
REGDEF_BEGIN(VPE_RESERVED_538)
REGDEF_BIT(VPE_Reserved_538,        32)
REGDEF_END(VPE_RESERVED_538)


/*
    RES2_CL6_SCA_WIDTH    :    [0x0, 0x1fff],           bits : 12_0
    RES2_CL6_SCA_HSTEP_OFT:    [0x0, 0x7fff],           bits : 30_16
*/
#define RES2_COL6_SIZE_1_REGISTER_OFS 0x0dc0
REGDEF_BEGIN(RES2_COL6_SIZE_1_REGISTER)
REGDEF_BIT(RES2_CL6_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL6_SCA_HSTEP_OFT,        15)
REGDEF_END(RES2_COL6_SIZE_1_REGISTER)


/*
    RES2_CL6_OUT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL6_OUT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL6_SIZE_2_REGISTER_OFS 0x0dc4
REGDEF_BEGIN(RES2_COL6_SIZE_2_REGISTER)
REGDEF_BIT(RES2_CL6_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL6_OUT_WIDTH,        13)
REGDEF_END(RES2_COL6_SIZE_2_REGISTER)


/*
    RES2_CL6_RLT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL6_RLT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL6_SIZE_3_REGISTER_OFS 0x0dc8
REGDEF_BEGIN(RES2_COL6_SIZE_3_REGISTER)
REGDEF_BIT(RES2_CL6_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL6_RLT_WIDTH,        13)
REGDEF_END(RES2_COL6_SIZE_3_REGISTER)


/*
    RES2_CL6_PIP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL6_PIP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL6_SIZE_4_REGISTER_OFS 0x0dcc
REGDEF_BEGIN(RES2_COL6_SIZE_4_REGISTER)
REGDEF_BIT(RES2_CL6_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL6_PIP_WIDTH,        13)
REGDEF_END(RES2_COL6_SIZE_4_REGISTER)


/*
    RES2_CL6_SCA_CROP_X_START:    [0x0, 0xfff],         bits : 11_0
    RES2_CL6_SCA_CROP_WIDTH  :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES2_COL6_SIZE_5_REGISTER_OFS 0x0dd0
REGDEF_BEGIN(RES2_COL6_SIZE_5_REGISTER)
REGDEF_BIT(RES2_CL6_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES2_CL6_SCA_CROP_WIDTH,        13)
REGDEF_END(RES2_COL6_SIZE_5_REGISTER)


/*
    RES2_CL6_TC_CROP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL6_TC_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
    RES2_CL6_TC_CROP_SKIP   :    [0x0, 0x1],            bits : 31
*/
#define RES2_COL6_SIZE_6_REGISTER_OFS 0x0dd4
REGDEF_BEGIN(RES2_COL6_SIZE_6_REGISTER)
REGDEF_BIT(RES2_CL6_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL6_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RES2_CL6_TC_CROP_SKIP,        1)
REGDEF_END(RES2_COL6_SIZE_6_REGISTER)


/*
    VPE_Reserved_539:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_539_OFS 0x0dd8
REGDEF_BEGIN(VPE_RESERVED_539)
REGDEF_BIT(VPE_Reserved_539,        32)
REGDEF_END(VPE_RESERVED_539)


/*
    VPE_Reserved_540:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_540_OFS 0x0ddc
REGDEF_BEGIN(VPE_RESERVED_540)
REGDEF_BIT(VPE_Reserved_540,        32)
REGDEF_END(VPE_RESERVED_540)


/*
    RES2_CL7_SCA_WIDTH    :    [0x0, 0x1fff],           bits : 12_0
    RES2_CL7_SCA_HSTEP_OFT:    [0x0, 0x7fff],           bits : 30_16
*/
#define RES2_COL7_SIZE_1_REGISTER_OFS 0x0de0
REGDEF_BEGIN(RES2_COL7_SIZE_1_REGISTER)
REGDEF_BIT(RES2_CL7_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL7_SCA_HSTEP_OFT,        15)
REGDEF_END(RES2_COL7_SIZE_1_REGISTER)


/*
    RES2_CL7_OUT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL7_OUT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL7_SIZE_2_REGISTER_OFS 0x0de4
REGDEF_BEGIN(RES2_COL7_SIZE_2_REGISTER)
REGDEF_BIT(RES2_CL7_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL7_OUT_WIDTH,        13)
REGDEF_END(RES2_COL7_SIZE_2_REGISTER)


/*
    RES2_CL7_RLT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL7_RLT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL7_SIZE_3_REGISTER_OFS 0x0de8
REGDEF_BEGIN(RES2_COL7_SIZE_3_REGISTER)
REGDEF_BIT(RES2_CL7_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL7_RLT_WIDTH,        13)
REGDEF_END(RES2_COL7_SIZE_3_REGISTER)


/*
    RES2_CL7_PIP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL7_PIP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES2_COL7_SIZE_4_REGISTER_OFS 0x0dec
REGDEF_BEGIN(RES2_COL7_SIZE_4_REGISTER)
REGDEF_BIT(RES2_CL7_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL7_PIP_WIDTH,        13)
REGDEF_END(RES2_COL7_SIZE_4_REGISTER)


/*
    RES2_CL7_SCA_CROP_X_START:    [0x0, 0xfff],         bits : 11_0
    RES2_CL7_SCA_CROP_WIDTH  :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES2_COL7_SIZE_5_REGISTER_OFS 0x0df0
REGDEF_BEGIN(RES2_COL7_SIZE_5_REGISTER)
REGDEF_BIT(RES2_CL7_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES2_CL7_SCA_CROP_WIDTH,        13)
REGDEF_END(RES2_COL7_SIZE_5_REGISTER)


/*
    RES2_CL7_TC_CROP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES2_CL7_TC_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
    RES2_CL7_TC_CROP_SKIP   :    [0x0, 0x1],            bits : 31
*/
#define RES2_COL7_SIZE_6_REGISTER_OFS 0x0df4
REGDEF_BEGIN(RES2_COL7_SIZE_6_REGISTER)
REGDEF_BIT(RES2_CL7_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES2_CL7_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RES2_CL7_TC_CROP_SKIP,        1)
REGDEF_END(RES2_COL7_SIZE_6_REGISTER)


/*
    VPE_Reserved_541:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_541_OFS 0x0df8
REGDEF_BEGIN(VPE_RESERVED_541)
REGDEF_BIT(VPE_Reserved_541,        32)
REGDEF_END(VPE_RESERVED_541)


/*
    VPE_Reserved_542:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_542_OFS 0x0dfc
REGDEF_BEGIN(VPE_RESERVED_542)
REGDEF_BIT(VPE_Reserved_542,        32)
REGDEF_END(VPE_RESERVED_542)


/*
    RES3_CL0_SCA_WIDTH    :    [0x0, 0x1fff],           bits : 12_0
    RES3_CL0_SCA_HSTEP_OFT:    [0x0, 0x7fff],           bits : 30_16
*/
#define RES3_COL0_SIZE_1_REGISTER_OFS 0x0e00
REGDEF_BEGIN(RES3_COL0_SIZE_1_REGISTER)
REGDEF_BIT(RES3_CL0_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL0_SCA_HSTEP_OFT,        15)
REGDEF_END(RES3_COL0_SIZE_1_REGISTER)


/*
    RES3_CL0_OUT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL0_OUT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL0_SIZE_2_REGISTER_OFS 0x0e04
REGDEF_BEGIN(RES3_COL0_SIZE_2_REGISTER)
REGDEF_BIT(RES3_CL0_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL0_OUT_WIDTH,        13)
REGDEF_END(RES3_COL0_SIZE_2_REGISTER)


/*
    RES3_CL0_RLT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL0_RLT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL0_SIZE_3_REGISTER_OFS 0x0e08
REGDEF_BEGIN(RES3_COL0_SIZE_3_REGISTER)
REGDEF_BIT(RES3_CL0_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL0_RLT_WIDTH,        13)
REGDEF_END(RES3_COL0_SIZE_3_REGISTER)


/*
    RES3_CL0_PIP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL0_PIP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL0_SIZE_4_REGISTER_OFS 0x0e0c
REGDEF_BEGIN(RES3_COL0_SIZE_4_REGISTER)
REGDEF_BIT(RES3_CL0_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL0_PIP_WIDTH,        13)
REGDEF_END(RES3_COL0_SIZE_4_REGISTER)


/*
    RES3_CL0_SCA_CROP_X_START:    [0x0, 0xfff],         bits : 11_0
    RES3_CL0_SCA_CROP_WIDTH  :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES3_COL0_SIZE_5_REGISTER_OFS 0x0e10
REGDEF_BEGIN(RES3_COL0_SIZE_5_REGISTER)
REGDEF_BIT(RES3_CL0_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES3_CL0_SCA_CROP_WIDTH,        13)
REGDEF_END(RES3_COL0_SIZE_5_REGISTER)


/*
    RES3_CL0_TC_CROP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL0_TC_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
    RES3_CL0_TC_CROP_SKIP   :    [0x0, 0x1],            bits : 31
*/
#define RES3_COL0_SIZE_6_REGISTER_OFS 0x0e14
REGDEF_BEGIN(RES3_COL0_SIZE_6_REGISTER)
REGDEF_BIT(RES3_CL0_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL0_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RES3_CL0_TC_CROP_SKIP,        1)
REGDEF_END(RES3_COL0_SIZE_6_REGISTER)


/*
    VPE_Reserved_543:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_543_OFS 0x0e18
REGDEF_BEGIN(VPE_RESERVED_543)
REGDEF_BIT(VPE_Reserved_543,        32)
REGDEF_END(VPE_RESERVED_543)


/*
    VPE_Reserved_544:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_544_OFS 0x0e1c
REGDEF_BEGIN(VPE_RESERVED_544)
REGDEF_BIT(VPE_Reserved_544,        32)
REGDEF_END(VPE_RESERVED_544)


/*
    RES3_CL1_SCA_WIDTH    :    [0x0, 0x1fff],           bits : 12_0
    RES3_CL1_SCA_HSTEP_OFT:    [0x0, 0x7fff],           bits : 30_16
*/
#define RES3_COL1_SIZE_1_REGISTER_OFS 0x0e20
REGDEF_BEGIN(RES3_COL1_SIZE_1_REGISTER)
REGDEF_BIT(RES3_CL1_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL1_SCA_HSTEP_OFT,        15)
REGDEF_END(RES3_COL1_SIZE_1_REGISTER)


/*
    RES3_CL1_OUT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL1_OUT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL1_SIZE_2_REGISTER_OFS 0x0e24
REGDEF_BEGIN(RES3_COL1_SIZE_2_REGISTER)
REGDEF_BIT(RES3_CL1_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL1_OUT_WIDTH,        13)
REGDEF_END(RES3_COL1_SIZE_2_REGISTER)


/*
    RES3_CL1_RLT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL1_RLT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL1_SIZE_3_REGISTER_OFS 0x0e28
REGDEF_BEGIN(RES3_COL1_SIZE_3_REGISTER)
REGDEF_BIT(RES3_CL1_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL1_RLT_WIDTH,        13)
REGDEF_END(RES3_COL1_SIZE_3_REGISTER)


/*
    RES3_CL1_PIP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL1_PIP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL1_SIZE_4_REGISTER_OFS 0x0e2c
REGDEF_BEGIN(RES3_COL1_SIZE_4_REGISTER)
REGDEF_BIT(RES3_CL1_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL1_PIP_WIDTH,        13)
REGDEF_END(RES3_COL1_SIZE_4_REGISTER)


/*
    RES3_CL1_SCA_CROP_X_START:    [0x0, 0xfff],         bits : 11_0
    RES3_CL1_SCA_CROP_WIDTH  :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES3_COL1_SIZE_5_REGISTER_OFS 0x0e30
REGDEF_BEGIN(RES3_COL1_SIZE_5_REGISTER)
REGDEF_BIT(RES3_CL1_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES3_CL1_SCA_CROP_WIDTH,        13)
REGDEF_END(RES3_COL1_SIZE_5_REGISTER)


/*
    RES3_CL1_TC_CROP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL1_TC_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
    RES3_CL1_TC_CROP_SKIP   :    [0x0, 0x1],            bits : 31
*/
#define RES3_COL1_SIZE_6_REGISTER_OFS 0x0e34
REGDEF_BEGIN(RES3_COL1_SIZE_6_REGISTER)
REGDEF_BIT(RES3_CL1_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL1_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RES3_CL1_TC_CROP_SKIP,        1)
REGDEF_END(RES3_COL1_SIZE_6_REGISTER)


/*
    VPE_Reserved_545:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_545_OFS 0x0e38
REGDEF_BEGIN(VPE_RESERVED_545)
REGDEF_BIT(VPE_Reserved_545,        32)
REGDEF_END(VPE_RESERVED_545)


/*
    VPE_Reserved_546:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_546_OFS 0x0e3c
REGDEF_BEGIN(VPE_RESERVED_546)
REGDEF_BIT(VPE_Reserved_546,        32)
REGDEF_END(VPE_RESERVED_546)


/*
    RES3_CL2_SCA_WIDTH    :    [0x0, 0x1fff],           bits : 12_0
    RES3_CL2_SCA_HSTEP_OFT:    [0x0, 0x7fff],           bits : 30_16
*/
#define RES3_COL2_SIZE_1_REGISTER_OFS 0x0e40
REGDEF_BEGIN(RES3_COL2_SIZE_1_REGISTER)
REGDEF_BIT(RES3_CL2_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL2_SCA_HSTEP_OFT,        15)
REGDEF_END(RES3_COL2_SIZE_1_REGISTER)


/*
    RES3_CL2_OUT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL2_OUT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL2_SIZE_2_REGISTER_OFS 0x0e44
REGDEF_BEGIN(RES3_COL2_SIZE_2_REGISTER)
REGDEF_BIT(RES3_CL2_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL2_OUT_WIDTH,        13)
REGDEF_END(RES3_COL2_SIZE_2_REGISTER)


/*
    RES3_CL2_RLT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL2_RLT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL2_SIZE_3_REGISTER_OFS 0x0e48
REGDEF_BEGIN(RES3_COL2_SIZE_3_REGISTER)
REGDEF_BIT(RES3_CL2_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL2_RLT_WIDTH,        13)
REGDEF_END(RES3_COL2_SIZE_3_REGISTER)


/*
    RES3_CL2_PIP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL2_PIP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL2_SIZE_4_REGISTER_OFS 0x0e4c
REGDEF_BEGIN(RES3_COL2_SIZE_4_REGISTER)
REGDEF_BIT(RES3_CL2_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL2_PIP_WIDTH,        13)
REGDEF_END(RES3_COL2_SIZE_4_REGISTER)


/*
    RES3_CL2_SCA_CROP_X_START:    [0x0, 0xfff],         bits : 11_0
    RES3_CL2_SCA_CROP_WIDTH  :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES3_COL2_SIZE_5_REGISTER_OFS 0x0e50
REGDEF_BEGIN(RES3_COL2_SIZE_5_REGISTER)
REGDEF_BIT(RES3_CL2_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES3_CL2_SCA_CROP_WIDTH,        13)
REGDEF_END(RES3_COL2_SIZE_5_REGISTER)


/*
    RES3_CL2_TC_CROP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL2_TC_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
    RES3_CL2_TC_CROP_SKIP   :    [0x0, 0x1],            bits : 31
*/
#define RES3_COL2_SIZE_6_REGISTER_OFS 0x0e54
REGDEF_BEGIN(RES3_COL2_SIZE_6_REGISTER)
REGDEF_BIT(RES3_CL2_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL2_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RES3_CL2_TC_CROP_SKIP,        1)
REGDEF_END(RES3_COL2_SIZE_6_REGISTER)


/*
    VPE_Reserved_547:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_547_OFS 0x0e58
REGDEF_BEGIN(VPE_RESERVED_547)
REGDEF_BIT(VPE_Reserved_547,        32)
REGDEF_END(VPE_RESERVED_547)


/*
    VPE_Reserved_548:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_548_OFS 0x0e5c
REGDEF_BEGIN(VPE_RESERVED_548)
REGDEF_BIT(VPE_Reserved_548,        32)
REGDEF_END(VPE_RESERVED_548)


/*
    RES3_CL3_SCA_WIDTH    :    [0x0, 0x1fff],           bits : 12_0
    RES3_CL3_SCA_HSTEP_OFT:    [0x0, 0x7fff],           bits : 30_16
*/
#define RES3_COL3_SIZE_1_REGISTER_OFS 0x0e60
REGDEF_BEGIN(RES3_COL3_SIZE_1_REGISTER)
REGDEF_BIT(RES3_CL3_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL3_SCA_HSTEP_OFT,        15)
REGDEF_END(RES3_COL3_SIZE_1_REGISTER)


/*
    RES3_CL3_OUT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL3_OUT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL3_SIZE_2_REGISTER_OFS 0x0e64
REGDEF_BEGIN(RES3_COL3_SIZE_2_REGISTER)
REGDEF_BIT(RES3_CL3_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL3_OUT_WIDTH,        13)
REGDEF_END(RES3_COL3_SIZE_2_REGISTER)


/*
    R_RES3_CL3_LT_X_START:    [0x0, 0x1fff],            bits : 12_0
    RES3_CL3_RLT_WIDTH   :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES3_COL3_SIZE_3_REGISTER_OFS 0x0e68
REGDEF_BEGIN(RES3_COL3_SIZE_3_REGISTER)
REGDEF_BIT(R_RES3_CL3_LT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL3_RLT_WIDTH,        13)
REGDEF_END(RES3_COL3_SIZE_3_REGISTER)


/*
    RES3_CL3_PIP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL3_PIP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL3_SIZE_4_REGISTER_OFS 0x0e6c
REGDEF_BEGIN(RES3_COL3_SIZE_4_REGISTER)
REGDEF_BIT(RES3_CL3_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL3_PIP_WIDTH,        13)
REGDEF_END(RES3_COL3_SIZE_4_REGISTER)


/*
    RES3_CL3_SCA_CROP_X_START:    [0x0, 0xfff],         bits : 11_0
    RES3_CL3_SCA_CROP_WIDTH  :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES3_COL3_SIZE_5_REGISTER_OFS 0x0e70
REGDEF_BEGIN(RES3_COL3_SIZE_5_REGISTER)
REGDEF_BIT(RES3_CL3_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES3_CL3_SCA_CROP_WIDTH,        13)
REGDEF_END(RES3_COL3_SIZE_5_REGISTER)


/*
    RES3_CL3_TC_CROP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL3_TC_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
    RES3_CL3_TC_CROP_SKIP   :    [0x0, 0x1],            bits : 31
*/
#define RES3_COL3_SIZE_6_REGISTER_OFS 0x0e74
REGDEF_BEGIN(RES3_COL3_SIZE_6_REGISTER)
REGDEF_BIT(RES3_CL3_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL3_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RES3_CL3_TC_CROP_SKIP,        1)
REGDEF_END(RES3_COL3_SIZE_6_REGISTER)


/*
    VPE_Reserved_549:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_549_OFS 0x0e78
REGDEF_BEGIN(VPE_RESERVED_549)
REGDEF_BIT(VPE_Reserved_549,        32)
REGDEF_END(VPE_RESERVED_549)


/*
    VPE_Reserved_550:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_550_OFS 0x0e7c
REGDEF_BEGIN(VPE_RESERVED_550)
REGDEF_BIT(VPE_Reserved_550,        32)
REGDEF_END(VPE_RESERVED_550)


/*
    RES3_CL4_SCA_WIDTH    :    [0x0, 0x1fff],           bits : 12_0
    RES3_CL4_SCA_HSTEP_OFT:    [0x0, 0x7fff],           bits : 30_16
*/
#define RES3_COL4_SIZE_1_REGISTER_OFS 0x0e80
REGDEF_BEGIN(RES3_COL4_SIZE_1_REGISTER)
REGDEF_BIT(RES3_CL4_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL4_SCA_HSTEP_OFT,        15)
REGDEF_END(RES3_COL4_SIZE_1_REGISTER)


/*
    RES3_CL4_OUT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL4_OUT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL4_SIZE_2_REGISTER_OFS 0x0e84
REGDEF_BEGIN(RES3_COL4_SIZE_2_REGISTER)
REGDEF_BIT(RES3_CL4_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL4_OUT_WIDTH,        13)
REGDEF_END(RES3_COL4_SIZE_2_REGISTER)


/*
    RES3_CL4_RLT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL4_RLT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL4_SIZE_3_REGISTER_OFS 0x0e88
REGDEF_BEGIN(RES3_COL4_SIZE_3_REGISTER)
REGDEF_BIT(RES3_CL4_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL4_RLT_WIDTH,        13)
REGDEF_END(RES3_COL4_SIZE_3_REGISTER)


/*
    RES3_CL4_PIP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL4_PIP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL4_SIZE_4_REGISTER_OFS 0x0e8c
REGDEF_BEGIN(RES3_COL4_SIZE_4_REGISTER)
REGDEF_BIT(RES3_CL4_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL4_PIP_WIDTH,        13)
REGDEF_END(RES3_COL4_SIZE_4_REGISTER)


/*
    RES3_CL4_SCA_CROP_X_START:    [0x0, 0xfff],         bits : 11_0
    RES3_CL4_SCA_CROP_WIDTH  :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES3_COL4_SIZE_5_REGISTER_OFS 0x0e90
REGDEF_BEGIN(RES3_COL4_SIZE_5_REGISTER)
REGDEF_BIT(RES3_CL4_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES3_CL4_SCA_CROP_WIDTH,        13)
REGDEF_END(RES3_COL4_SIZE_5_REGISTER)


/*
    RES3_CL4_TC_CROP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL4_TC_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
    RES3_CL4_TC_CROP_SKIP   :    [0x0, 0x1],            bits : 31
*/
#define RES3_COL4_SIZE_6_REGISTER_OFS 0x0e94
REGDEF_BEGIN(RES3_COL4_SIZE_6_REGISTER)
REGDEF_BIT(RES3_CL4_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL4_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RES3_CL4_TC_CROP_SKIP,        1)
REGDEF_END(RES3_COL4_SIZE_6_REGISTER)


/*
    VPE_Reserved_551:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_551_OFS 0x0e98
REGDEF_BEGIN(VPE_RESERVED_551)
REGDEF_BIT(VPE_Reserved_551,        32)
REGDEF_END(VPE_RESERVED_551)


/*
    VPE_Reserved_552:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_552_OFS 0x0e9c
REGDEF_BEGIN(VPE_RESERVED_552)
REGDEF_BIT(VPE_Reserved_552,        32)
REGDEF_END(VPE_RESERVED_552)


/*
    RES3_CL5_SCA_WIDTH    :    [0x0, 0x1fff],           bits : 12_0
    RES3_CL5_SCA_HSTEP_OFT:    [0x0, 0x7fff],           bits : 30_16
*/
#define RES3_COL5_SIZE_1_REGISTER_OFS 0x0ea0
REGDEF_BEGIN(RES3_COL5_SIZE_1_REGISTER)
REGDEF_BIT(RES3_CL5_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL5_SCA_HSTEP_OFT,        15)
REGDEF_END(RES3_COL5_SIZE_1_REGISTER)


/*
    RES3_CL5_OUT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL5_OUT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL5_SIZE_2_REGISTER_OFS 0x0ea4
REGDEF_BEGIN(RES3_COL5_SIZE_2_REGISTER)
REGDEF_BIT(RES3_CL5_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL5_OUT_WIDTH,        13)
REGDEF_END(RES3_COL5_SIZE_2_REGISTER)


/*
    RES3_CL5_RLT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL5_RLT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL5_SIZE_3_REGISTER_OFS 0x0ea8
REGDEF_BEGIN(RES3_COL5_SIZE_3_REGISTER)
REGDEF_BIT(RES3_CL5_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL5_RLT_WIDTH,        13)
REGDEF_END(RES3_COL5_SIZE_3_REGISTER)


/*
    RES3_CL5_PIP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL5_PIP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL5_SIZE_4_REGISTER_OFS 0x0eac
REGDEF_BEGIN(RES3_COL5_SIZE_4_REGISTER)
REGDEF_BIT(RES3_CL5_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL5_PIP_WIDTH,        13)
REGDEF_END(RES3_COL5_SIZE_4_REGISTER)


/*
    RES3_CL5_SCA_CROP_X_START:    [0x0, 0xfff],         bits : 11_0
    RES3_CL5_SCA_CROP_WIDTH  :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES3_COL5_SIZE_5_REGISTER_OFS 0x0eb0
REGDEF_BEGIN(RES3_COL5_SIZE_5_REGISTER)
REGDEF_BIT(RES3_CL5_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES3_CL5_SCA_CROP_WIDTH,        13)
REGDEF_END(RES3_COL5_SIZE_5_REGISTER)


/*
    RES3_CL5_TC_CROP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL5_TC_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
    RES3_CL5_TC_CROP_SKIP   :    [0x0, 0x1],            bits : 31
*/
#define RES3_COL5_SIZE_6_REGISTER_OFS 0x0eb4
REGDEF_BEGIN(RES3_COL5_SIZE_6_REGISTER)
REGDEF_BIT(RES3_CL5_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL5_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RES3_CL5_TC_CROP_SKIP,        1)
REGDEF_END(RES3_COL5_SIZE_6_REGISTER)


/*
    VPE_Reserved_553:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_553_OFS 0x0eb8
REGDEF_BEGIN(VPE_RESERVED_553)
REGDEF_BIT(VPE_Reserved_553,        32)
REGDEF_END(VPE_RESERVED_553)


/*
    VPE_Reserved_554:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_554_OFS 0x0ebc
REGDEF_BEGIN(VPE_RESERVED_554)
REGDEF_BIT(VPE_Reserved_554,        32)
REGDEF_END(VPE_RESERVED_554)


/*
    RES3_CL6_SCA_WIDTH    :    [0x0, 0x1fff],           bits : 12_0
    RES3_CL6_SCA_HSTEP_OFT:    [0x0, 0x7fff],           bits : 30_16
*/
#define RES3_COL6_SIZE_1_REGISTER_OFS 0x0ec0
REGDEF_BEGIN(RES3_COL6_SIZE_1_REGISTER)
REGDEF_BIT(RES3_CL6_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL6_SCA_HSTEP_OFT,        15)
REGDEF_END(RES3_COL6_SIZE_1_REGISTER)


/*
    RES3_CL6_OUT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL6_OUT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL6_SIZE_2_REGISTER_OFS 0x0ec4
REGDEF_BEGIN(RES3_COL6_SIZE_2_REGISTER)
REGDEF_BIT(RES3_CL6_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL6_OUT_WIDTH,        13)
REGDEF_END(RES3_COL6_SIZE_2_REGISTER)


/*
    RES3_CL6_RLT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL6_RLT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL6_SIZE_3_REGISTER_OFS 0x0ec8
REGDEF_BEGIN(RES3_COL6_SIZE_3_REGISTER)
REGDEF_BIT(RES3_CL6_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL6_RLT_WIDTH,        13)
REGDEF_END(RES3_COL6_SIZE_3_REGISTER)


/*
    RES3_CL6_PIP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL6_PIP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL6_SIZE_4_REGISTER_OFS 0x0ecc
REGDEF_BEGIN(RES3_COL6_SIZE_4_REGISTER)
REGDEF_BIT(RES3_CL6_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL6_PIP_WIDTH,        13)
REGDEF_END(RES3_COL6_SIZE_4_REGISTER)


/*
    RES3_CL6_SCA_CROP_X_START:    [0x0, 0xfff],         bits : 11_0
    RES3_CL6_SCA_CROP_WIDTH  :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES3_COL6_SIZE_5_REGISTER_OFS 0x0ed0
REGDEF_BEGIN(RES3_COL6_SIZE_5_REGISTER)
REGDEF_BIT(RES3_CL6_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES3_CL6_SCA_CROP_WIDTH,        13)
REGDEF_END(RES3_COL6_SIZE_5_REGISTER)


/*
    RES3_CL6_TC_CROP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL6_TC_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
    RES3_CL6_TC_CROP_SKIP   :    [0x0, 0x1],            bits : 31
*/
#define RES3_COL6_SIZE_6_REGISTER_OFS 0x0ed4
REGDEF_BEGIN(RES3_COL6_SIZE_6_REGISTER)
REGDEF_BIT(RES3_CL6_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL6_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RES3_CL6_TC_CROP_SKIP,        1)
REGDEF_END(RES3_COL6_SIZE_6_REGISTER)


/*
    VPE_Reserved_555:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_555_OFS 0x0ed8
REGDEF_BEGIN(VPE_RESERVED_555)
REGDEF_BIT(VPE_Reserved_555,        32)
REGDEF_END(VPE_RESERVED_555)


/*
    VPE_Reserved_556:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_556_OFS 0x0edc
REGDEF_BEGIN(VPE_RESERVED_556)
REGDEF_BIT(VPE_Reserved_556,        32)
REGDEF_END(VPE_RESERVED_556)


/*
    RES3_CL7_SCA_WIDTH    :    [0x0, 0x1fff],           bits : 12_0
    RES3_CL7_SCA_HSTEP_OFT:    [0x0, 0x7fff],           bits : 30_16
*/
#define RES3_COL7_SIZE_1_REGISTER_OFS 0x0ee0
REGDEF_BEGIN(RES3_COL7_SIZE_1_REGISTER)
REGDEF_BIT(RES3_CL7_SCA_WIDTH,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL7_SCA_HSTEP_OFT,        15)
REGDEF_END(RES3_COL7_SIZE_1_REGISTER)


/*
    RES3_CL7_OUT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL7_OUT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL7_SIZE_2_REGISTER_OFS 0x0ee4
REGDEF_BEGIN(RES3_COL7_SIZE_2_REGISTER)
REGDEF_BIT(RES3_CL7_OUT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL7_OUT_WIDTH,        13)
REGDEF_END(RES3_COL7_SIZE_2_REGISTER)


/*
    RES3_CL7_RLT_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL7_RLT_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL7_SIZE_3_REGISTER_OFS 0x0ee8
REGDEF_BEGIN(RES3_COL7_SIZE_3_REGISTER)
REGDEF_BIT(RES3_CL7_RLT_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL7_RLT_WIDTH,        13)
REGDEF_END(RES3_COL7_SIZE_3_REGISTER)


/*
    RES3_CL7_PIP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL7_PIP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
*/
#define RES3_COL7_SIZE_4_REGISTER_OFS 0x0eec
REGDEF_BEGIN(RES3_COL7_SIZE_4_REGISTER)
REGDEF_BIT(RES3_CL7_PIP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL7_PIP_WIDTH,        13)
REGDEF_END(RES3_COL7_SIZE_4_REGISTER)


/*
    RES3_CL7_SCA_CROP_X_START:    [0x0, 0xfff],         bits : 11_0
    RES3_CL7_SCA_CROP_WIDTH  :    [0x0, 0x1fff],            bits : 28_16
*/
#define RES3_COL7_SIZE_5_REGISTER_OFS 0x0ef0
REGDEF_BEGIN(RES3_COL7_SIZE_5_REGISTER)
REGDEF_BIT(RES3_CL7_SCA_CROP_X_START,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(RES3_CL7_SCA_CROP_WIDTH,        13)
REGDEF_END(RES3_COL7_SIZE_5_REGISTER)


/*
    RES3_CL7_TC_CROP_X_START:    [0x0, 0x1fff],         bits : 12_0
    RES3_CL7_TC_CROP_WIDTH  :    [0x0, 0x1fff],         bits : 28_16
    RES3_CL7_TC_CROP_SKIP   :    [0x0, 0x1],            bits : 31
*/
#define RES3_COL7_SIZE_6_REGISTER_OFS 0x0ef4
REGDEF_BEGIN(RES3_COL7_SIZE_6_REGISTER)
REGDEF_BIT(RES3_CL7_TC_CROP_X_START,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(RES3_CL7_TC_CROP_WIDTH,        13)
REGDEF_BIT(,        2)
REGDEF_BIT(RES3_CL7_TC_CROP_SKIP,        1)
REGDEF_END(RES3_COL7_SIZE_6_REGISTER)


/*
    VPE_Reserved_557:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_557_OFS 0x0ef8
REGDEF_BEGIN(VPE_RESERVED_557)
REGDEF_BIT(VPE_Reserved_557,        32)
REGDEF_END(VPE_RESERVED_557)


/*
    VPE_Reserved_558:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_558_OFS 0x0efc
REGDEF_BEGIN(VPE_RESERVED_558)
REGDEF_BIT(VPE_Reserved_558,        32)
REGDEF_END(VPE_RESERVED_558)


/*
    RES0_ENC_SMODE_EN:    [0x0, 0x1],           bits : 4
    RES1_ENC_SMODE_EN:    [0x0, 0x1],           bits : 5
*/
#define RES_COMPRESSION_CONTROL_REGISTER0_OFS 0x0f00
REGDEF_BEGIN(RES_COMPRESSION_CONTROL_REGISTER0)
REGDEF_BIT(,        4)
REGDEF_BIT(RES0_ENC_SMODE_EN,        1)
REGDEF_BIT(RES1_ENC_SMODE_EN,        1)
REGDEF_END(RES_COMPRESSION_CONTROL_REGISTER0)


/*
    VPE_Reserved_559:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_559_OFS 0x0f04
REGDEF_BEGIN(VPE_RESERVED_559)
REGDEF_BIT(VPE_Reserved_559,        32)
REGDEF_END(VPE_RESERVED_559)


/*
    VPE_Reserved_560:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_560_OFS 0x0f08
REGDEF_BEGIN(VPE_RESERVED_560)
REGDEF_BIT(VPE_Reserved_560,        32)
REGDEF_END(VPE_RESERVED_560)


/*
    VPE_Reserved_561:    [0x0, 0xffffffff],         bits : 31_0
*/
#define VPE_RESERVED_561_OFS 0x0f0c
REGDEF_BEGIN(VPE_RESERVED_561)
REGDEF_BIT(VPE_Reserved_561,        32)
REGDEF_END(VPE_RESERVED_561)


/*
    DCT_QTBL0_ENC_P0:    [0x0, 0x1fff],         bits : 12_0
    DCT_QTBL0_ENC_P1:    [0x0, 0xfff],          bits : 27_16
*/
#define ENCODER_Q_TABLE_REGISTER0_OFS 0x0f10
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER0)
REGDEF_BIT(DCT_QTBL0_ENC_P0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCT_QTBL0_ENC_P1,        12)
REGDEF_END(ENCODER_Q_TABLE_REGISTER0)


/*
    DCT_QTBL0_ENC_P2:    [0x0, 0x7ff],          bits : 10_0
*/
#define ENCODER_Q_TABLE_REGISTER1_OFS 0x0f14
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER1)
REGDEF_BIT(DCT_QTBL0_ENC_P2,        11)
REGDEF_END(ENCODER_Q_TABLE_REGISTER1)


/*
    DCT_QTBL1_ENC_P0:    [0x0, 0x1fff],         bits : 12_0
    DCT_QTBL1_ENC_P1:    [0x0, 0xfff],          bits : 27_16
*/
#define ENCODER_Q_TABLE_REGISTER2_OFS 0x0f18
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER2)
REGDEF_BIT(DCT_QTBL1_ENC_P0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCT_QTBL1_ENC_P1,        12)
REGDEF_END(ENCODER_Q_TABLE_REGISTER2)


/*
    DCT_QTBL1_ENC_P2:    [0x0, 0x7ff],          bits : 10_0
*/
#define ENCODER_Q_TABLE_REGISTER3_OFS 0x0f1c
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER3)
REGDEF_BIT(DCT_QTBL1_ENC_P2,        11)
REGDEF_END(ENCODER_Q_TABLE_REGISTER3)


/*
    DCT_QTBL2_ENC_P0:    [0x0, 0x1fff],         bits : 12_0
    DCT_QTBL2_ENC_P1:    [0x0, 0xfff],          bits : 27_16
*/
#define ENCODER_Q_TABLE_REGISTER4_OFS 0x0f20
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER4)
REGDEF_BIT(DCT_QTBL2_ENC_P0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCT_QTBL2_ENC_P1,        12)
REGDEF_END(ENCODER_Q_TABLE_REGISTER4)


/*
    DCT_QTBL2_ENC_P2:    [0x0, 0x7ff],          bits : 10_0
*/
#define ENCODER_Q_TABLE_REGISTER5_OFS 0x0f24
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER5)
REGDEF_BIT(DCT_QTBL2_ENC_P2,        11)
REGDEF_END(ENCODER_Q_TABLE_REGISTER5)


/*
    DCT_QTBL3_ENC_P0:    [0x0, 0x1fff],         bits : 12_0
    DCT_QTBL3_ENC_P1:    [0x0, 0xfff],          bits : 27_16
*/
#define ENCODER_Q_TABLE_REGISTER6_OFS 0x0f28
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER6)
REGDEF_BIT(DCT_QTBL3_ENC_P0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCT_QTBL3_ENC_P1,        12)
REGDEF_END(ENCODER_Q_TABLE_REGISTER6)


/*
    DCT_QTBL3_ENC_P2:    [0x0, 0x7ff],          bits : 10_0
*/
#define ENCODER_Q_TABLE_REGISTER7_OFS 0x0f2c
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER7)
REGDEF_BIT(DCT_QTBL3_ENC_P2,        11)
REGDEF_END(ENCODER_Q_TABLE_REGISTER7)


/*
    DCT_QTBL4_ENC_P0:    [0x0, 0x1fff],         bits : 12_0
    DCT_QTBL4_ENC_P1:    [0x0, 0xfff],          bits : 27_16
*/
#define ENCODER_Q_TABLE_REGISTER8_OFS 0x0f30
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER8)
REGDEF_BIT(DCT_QTBL4_ENC_P0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCT_QTBL4_ENC_P1,        12)
REGDEF_END(ENCODER_Q_TABLE_REGISTER8)


/*
    DCT_QTBL4_ENC_P2:    [0x0, 0x7ff],          bits : 10_0
*/
#define ENCODER_Q_TABLE_REGISTER9_OFS 0x0f34
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER9)
REGDEF_BIT(DCT_QTBL4_ENC_P2,        11)
REGDEF_END(ENCODER_Q_TABLE_REGISTER9)


/*
    DCT_QTBL5_ENC_P0:    [0x0, 0x1fff],         bits : 12_0
    DCT_QTBL5_ENC_P1:    [0x0, 0xfff],          bits : 27_16
*/
#define ENCODER_Q_TABLE_REGISTER10_OFS 0x0f38
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER10)
REGDEF_BIT(DCT_QTBL5_ENC_P0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCT_QTBL5_ENC_P1,        12)
REGDEF_END(ENCODER_Q_TABLE_REGISTER10)


/*
    DCT_QTBL5_ENC_P2:    [0x0, 0x7ff],          bits : 10_0
*/
#define ENCODER_Q_TABLE_REGISTER11_OFS 0x0f3c
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER11)
REGDEF_BIT(DCT_QTBL5_ENC_P2,        11)
REGDEF_END(ENCODER_Q_TABLE_REGISTER11)


/*
    DCT_QTBL6_ENC_P0:    [0x0, 0x1fff],         bits : 12_0
    DCT_QTBL6_ENC_P1:    [0x0, 0xfff],          bits : 27_16
*/
#define ENCODER_Q_TABLE_REGISTER12_OFS 0x0f40
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER12)
REGDEF_BIT(DCT_QTBL6_ENC_P0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCT_QTBL6_ENC_P1,        12)
REGDEF_END(ENCODER_Q_TABLE_REGISTER12)


/*
    DCT_QTBL6_ENC_P2:    [0x0, 0x7ff],          bits : 10_0
*/
#define ENCODER_Q_TABLE_REGISTER13_OFS 0x0f44
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER13)
REGDEF_BIT(DCT_QTBL6_ENC_P2,        11)
REGDEF_END(ENCODER_Q_TABLE_REGISTER13)


/*
    DCT_QTBL7_ENC_P0:    [0x0, 0x1fff],         bits : 12_0
    DCT_QTBL7_ENC_P1:    [0x0, 0xfff],          bits : 27_16
*/
#define ENCODER_Q_TABLE_REGISTER14_OFS 0x0f48
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER14)
REGDEF_BIT(DCT_QTBL7_ENC_P0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCT_QTBL7_ENC_P1,        12)
REGDEF_END(ENCODER_Q_TABLE_REGISTER14)


/*
    DCT_QTBL7_ENC_P2:    [0x0, 0x7ff],          bits : 10_0
*/
#define ENCODER_Q_TABLE_REGISTER15_OFS 0x0f4c
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER15)
REGDEF_BIT(DCT_QTBL7_ENC_P2,        11)
REGDEF_END(ENCODER_Q_TABLE_REGISTER15)


/*
    DCT_QTBL8_ENC_P0:    [0x0, 0x1fff],         bits : 12_0
    DCT_QTBL8_ENC_P1:    [0x0, 0xfff],          bits : 27_16
*/
#define ENCODER_Q_TABLE_REGISTER16_OFS 0x0f50
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER16)
REGDEF_BIT(DCT_QTBL8_ENC_P0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCT_QTBL8_ENC_P1,        12)
REGDEF_END(ENCODER_Q_TABLE_REGISTER16)


/*
    DCT_QTBL8_ENC_P2:    [0x0, 0x7ff],          bits : 10_0
*/
#define ENCODER_Q_TABLE_REGISTER17_OFS 0x0f54
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER17)
REGDEF_BIT(DCT_QTBL8_ENC_P2,        11)
REGDEF_END(ENCODER_Q_TABLE_REGISTER17)


/*
    DCT_QTBL9_ENC_P0:    [0x0, 0x1fff],         bits : 12_0
    DCT_QTBL9_ENC_P1:    [0x0, 0xfff],          bits : 27_16
*/
#define ENCODER_Q_TABLE_REGISTER18_OFS 0x0f58
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER18)
REGDEF_BIT(DCT_QTBL9_ENC_P0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCT_QTBL9_ENC_P1,        12)
REGDEF_END(ENCODER_Q_TABLE_REGISTER18)


/*
    DCT_QTBL9_ENC_P2:    [0x0, 0x7ff],          bits : 10_0
*/
#define ENCODER_Q_TABLE_REGISTER19_OFS 0x0f5c
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER19)
REGDEF_BIT(DCT_QTBL9_ENC_P2,        11)
REGDEF_END(ENCODER_Q_TABLE_REGISTER19)


/*
    DCT_QTBL10_ENC_P0:    [0x0, 0x1fff],            bits : 12_0
    DCT_QTBL10_ENC_P1:    [0x0, 0xfff],         bits : 27_16
*/
#define ENCODER_Q_TABLE_REGISTER20_OFS 0x0f60
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER20)
REGDEF_BIT(DCT_QTBL10_ENC_P0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCT_QTBL10_ENC_P1,        12)
REGDEF_END(ENCODER_Q_TABLE_REGISTER20)


/*
    DCT_QTBL11_ENC_P2:    [0x0, 0x7ff],         bits : 10_0
*/
#define ENCODER_Q_TABLE_REGISTER21_OFS 0x0f64
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER21)
REGDEF_BIT(DCT_QTBL11_ENC_P2,        11)
REGDEF_END(ENCODER_Q_TABLE_REGISTER21)


/*
    DCT_QTBL11_ENC_P0:    [0x0, 0x1fff],            bits : 12_0
    DCT_QTBL11_ENC_P1:    [0x0, 0xfff],         bits : 27_16
*/
#define ENCODER_Q_TABLE_REGISTER22_OFS 0x0f68
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER22)
REGDEF_BIT(DCT_QTBL11_ENC_P0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCT_QTBL11_ENC_P1,        12)
REGDEF_END(ENCODER_Q_TABLE_REGISTER22)


/*
    DCT_QTBL11_ENC_P2:    [0x0, 0x7ff],         bits : 10_0
*/
#define ENCODER_Q_TABLE_REGISTER23_OFS 0x0f6c
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER23)
REGDEF_BIT(DCT_QTBL11_ENC_P2,        11)
REGDEF_END(ENCODER_Q_TABLE_REGISTER23)


/*
    DCT_QTBL12_ENC_P0:    [0x0, 0x1fff],            bits : 12_0
    DCT_QTBL12_ENC_P1:    [0x0, 0xfff],         bits : 27_16
*/
#define ENCODER_Q_TABLE_REGISTER24_OFS 0x0f70
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER24)
REGDEF_BIT(DCT_QTBL12_ENC_P0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCT_QTBL12_ENC_P1,        12)
REGDEF_END(ENCODER_Q_TABLE_REGISTER24)


/*
    DCT_QTBL12_ENC_P2:    [0x0, 0x7ff],         bits : 10_0
*/
#define ENCODER_Q_TABLE_REGISTER25_OFS 0x0f74
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER25)
REGDEF_BIT(DCT_QTBL12_ENC_P2,        11)
REGDEF_END(ENCODER_Q_TABLE_REGISTER25)


/*
    DCT_QTBL13_ENC_P0:    [0x0, 0x1fff],            bits : 12_0
    DCT_QTBL13_ENC_P1:    [0x0, 0xfff],         bits : 27_16
*/
#define ENCODER_Q_TABLE_REGISTER26_OFS 0x0f78
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER26)
REGDEF_BIT(DCT_QTBL13_ENC_P0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCT_QTBL13_ENC_P1,        12)
REGDEF_END(ENCODER_Q_TABLE_REGISTER26)


/*
    DCT_QTBL13_ENC_P2:    [0x0, 0x7ff],         bits : 10_0
*/
#define ENCODER_Q_TABLE_REGISTER27_OFS 0x0f7c
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER27)
REGDEF_BIT(DCT_QTBL13_ENC_P2,        11)
REGDEF_END(ENCODER_Q_TABLE_REGISTER27)


/*
    DCT_QTBL14_ENC_P0:    [0x0, 0x1fff],            bits : 12_0
    DCT_QTBL14_ENC_P1:    [0x0, 0xfff],         bits : 27_16
*/
#define ENCODER_Q_TABLE_REGISTER28_OFS 0x0f80
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER28)
REGDEF_BIT(DCT_QTBL14_ENC_P0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCT_QTBL14_ENC_P1,        12)
REGDEF_END(ENCODER_Q_TABLE_REGISTER28)


/*
    DCT_QTBL14_ENC_P2:    [0x0, 0x7ff],         bits : 10_0
*/
#define ENCODER_Q_TABLE_REGISTER29_OFS 0x0f84
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER29)
REGDEF_BIT(DCT_QTBL14_ENC_P2,        11)
REGDEF_END(ENCODER_Q_TABLE_REGISTER29)


/*
    DCT_QTBL15_ENC_P0:    [0x0, 0x1fff],            bits : 12_0
    DCT_QTBL15_ENC_P1:    [0x0, 0xfff],         bits : 27_16
*/
#define ENCODER_Q_TABLE_REGISTER30_OFS 0x0f88
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER30)
REGDEF_BIT(DCT_QTBL15_ENC_P0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(DCT_QTBL15_ENC_P1,        12)
REGDEF_END(ENCODER_Q_TABLE_REGISTER30)


/*
    DCT_QTBL15_ENC_P2:    [0x0, 0x7ff],         bits : 10_0
*/
#define ENCODER_Q_TABLE_REGISTER31_OFS 0x0f8c
REGDEF_BEGIN(ENCODER_Q_TABLE_REGISTER31)
REGDEF_BIT(DCT_QTBL15_ENC_P2,        11)
REGDEF_END(ENCODER_Q_TABLE_REGISTER31)


/*
    DCT_QTBL0_DCMAX:    [0x0, 0x1ff],           bits : 8_0
    DCT_QTBL1_DCMAX:    [0x0, 0x1ff],           bits : 24_16
*/
#define ENCODER_QUALITY_DC_REGISTER0_OFS 0x0f90
REGDEF_BEGIN(ENCODER_QUALITY_DC_REGISTER0)
REGDEF_BIT(DCT_QTBL0_DCMAX,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(DCT_QTBL1_DCMAX,        9)
REGDEF_END(ENCODER_QUALITY_DC_REGISTER0)


/*
    DCT_QTBL2_DCMAX:    [0x0, 0x1ff],           bits : 8_0
    DCT_QTBL3_DCMAX:    [0x0, 0x1ff],           bits : 24_16
*/
#define ENCODER_QUALITY_DC_REGISTER1_OFS 0x0f94
REGDEF_BEGIN(ENCODER_QUALITY_DC_REGISTER1)
REGDEF_BIT(DCT_QTBL2_DCMAX,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(DCT_QTBL3_DCMAX,        9)
REGDEF_END(ENCODER_QUALITY_DC_REGISTER1)


/*
    DCT_QTBL4_DCMAX:    [0x0, 0x1ff],           bits : 8_0
    DCT_QTBL5_DCMAX:    [0x0, 0x1ff],           bits : 24_16
*/
#define ENCODER_QUALITY_DC_REGISTER2_OFS 0x0f98
REGDEF_BEGIN(ENCODER_QUALITY_DC_REGISTER2)
REGDEF_BIT(DCT_QTBL4_DCMAX,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(DCT_QTBL5_DCMAX,        9)
REGDEF_END(ENCODER_QUALITY_DC_REGISTER2)


/*
    DCT_QTBL6_DCMAX:    [0x0, 0x1ff],           bits : 8_0
    DCT_QTBL7_DCMAX:    [0x0, 0x1ff],           bits : 24_16
*/
#define ENCODER_QUALITY_DC_REGISTER3_OFS 0x0f9c
REGDEF_BEGIN(ENCODER_QUALITY_DC_REGISTER3)
REGDEF_BIT(DCT_QTBL6_DCMAX,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(DCT_QTBL7_DCMAX,        9)
REGDEF_END(ENCODER_QUALITY_DC_REGISTER3)


/*
    DCT_QTBL8_DCMAX:    [0x0, 0x1ff],           bits : 8_0
    DCT_QTBL9_DCMAX:    [0x0, 0x1ff],           bits : 24_16
*/
#define ENCODER_QUALITY_DC_REGISTER4_OFS 0x0fa0
REGDEF_BEGIN(ENCODER_QUALITY_DC_REGISTER4)
REGDEF_BIT(DCT_QTBL8_DCMAX,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(DCT_QTBL9_DCMAX,        9)
REGDEF_END(ENCODER_QUALITY_DC_REGISTER4)


/*
    DCT_QTBL10_DCMAX:    [0x0, 0x1ff],          bits : 8_0
    DCT_QTBL11_DCMAX:    [0x0, 0x1ff],          bits : 24_16
*/
#define ENCODER_QUALITY_DC_REGISTER5_OFS 0x0fa4
REGDEF_BEGIN(ENCODER_QUALITY_DC_REGISTER5)
REGDEF_BIT(DCT_QTBL10_DCMAX,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(DCT_QTBL11_DCMAX,        9)
REGDEF_END(ENCODER_QUALITY_DC_REGISTER5)


/*
    DCT_QTBL12_DCMAX:    [0x0, 0x1ff],          bits : 8_0
    DCT_QTBL13_DCMAX:    [0x0, 0x1ff],          bits : 24_16
*/
#define ENCODER_QUALITY_DC_REGISTER6_OFS 0x0fa8
REGDEF_BEGIN(ENCODER_QUALITY_DC_REGISTER6)
REGDEF_BIT(DCT_QTBL12_DCMAX,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(DCT_QTBL13_DCMAX,        9)
REGDEF_END(ENCODER_QUALITY_DC_REGISTER6)


/*
    DCT_QTBL14_DCMAX:    [0x0, 0x1ff],          bits : 8_0
    DCT_QTBL15_DCMAX:    [0x0, 0x1ff],          bits : 24_16
*/
#define ENCODER_QUALITY_DC_REGISTER7_OFS 0x0fac
REGDEF_BEGIN(ENCODER_QUALITY_DC_REGISTER7)
REGDEF_BIT(DCT_QTBL14_DCMAX,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(DCT_QTBL15_DCMAX,        9)
REGDEF_END(ENCODER_QUALITY_DC_REGISTER7)


/*
    YRC_LNCNT_LFN0:    [0x0, 0x1fff],           bits : 12_0
    YRC_LNCNT_LFN1:    [0x0, 0xfff],            bits : 27_16
*/
#define YCC_COMPRESSION_DEBUG_REGISTER0_OFS 0x0fb0
REGDEF_BEGIN(YCC_COMPRESSION_DEBUG_REGISTER0)
REGDEF_BIT(YRC_LNCNT_LFN0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(YRC_LNCNT_LFN1,        12)
REGDEF_END(YCC_COMPRESSION_DEBUG_REGISTER0)


/*
    YRC_LNCNT_LFN2:    [0x0, 0x1fff],           bits : 12_0
    YRC_LNCNT_LFN3:    [0x0, 0xfff],            bits : 27_16
*/
#define YCC_COMPRESSION_DEBUG_REGISTER1_OFS 0x0fb4
REGDEF_BEGIN(YCC_COMPRESSION_DEBUG_REGISTER1)
REGDEF_BIT(YRC_LNCNT_LFN2,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(YRC_LNCNT_LFN3,        12)
REGDEF_END(YCC_COMPRESSION_DEBUG_REGISTER1)


/*
    YRC_LNCNT_LFN4:    [0x0, 0x1fff],           bits : 12_0
    YRC_LNCNT_LFN5:    [0x0, 0xfff],            bits : 27_16
*/
#define YCC_COMPRESSION_DEBUG_REGISTER2_OFS 0x0fb8
REGDEF_BEGIN(YCC_COMPRESSION_DEBUG_REGISTER2)
REGDEF_BIT(YRC_LNCNT_LFN4,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(YRC_LNCNT_LFN5,        12)
REGDEF_END(YCC_COMPRESSION_DEBUG_REGISTER2)


/*
    DCT_MAXDIST:    [0x0, 0xff],            bits : 7_0
*/
#define YCC_COMPRESSION_DEBUG_REGISTER3_OFS 0x0fbc
REGDEF_BEGIN(YCC_COMPRESSION_DEBUG_REGISTER3)
REGDEF_BIT(DCT_MAXDIST,        8)
REGDEF_END(YCC_COMPRESSION_DEBUG_REGISTER3)


/*
    DCT_QTBL0_DEC_P0:    [0x0, 0x3ff],          bits : 9_0
    DCT_QTBL0_DEC_P1:    [0x0, 0x3ff],          bits : 25_16
*/
#define DECODER_QUALITY_TABLE_0_REGISTER0_OFS 0x0fc0
REGDEF_BEGIN(DECODER_QUALITY_TABLE_0_REGISTER0)
REGDEF_BIT(DCT_QTBL0_DEC_P0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCT_QTBL0_DEC_P1,        10)
REGDEF_END(DECODER_QUALITY_TABLE_0_REGISTER0)


/*
    DCT_QTBL0_DEC_P2:    [0x0, 0x3ff],          bits : 9_0
*/
#define DECODER_QUALITY_TABLE_0_REGISTER1_OFS 0x0fc4
REGDEF_BEGIN(DECODER_QUALITY_TABLE_0_REGISTER1)
REGDEF_BIT(DCT_QTBL0_DEC_P2,        10)
REGDEF_END(DECODER_QUALITY_TABLE_0_REGISTER1)


/*
    DCT_QTBL1_DEC_P0:    [0x0, 0x3ff],          bits : 9_0
    DCT_QTBL1_DEC_P1:    [0x0, 0x3ff],          bits : 25_16
*/
#define DECODER_QUALITY_TABLE_1_REGISTER2_OFS 0x0fc8
REGDEF_BEGIN(DECODER_QUALITY_TABLE_1_REGISTER2)
REGDEF_BIT(DCT_QTBL1_DEC_P0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCT_QTBL1_DEC_P1,        10)
REGDEF_END(DECODER_QUALITY_TABLE_1_REGISTER2)


/*
    DCT_QTBL1_DEC_P2:    [0x0, 0x3ff],          bits : 9_0
*/
#define DECODER_QUALITY_TABLE_1_REGISTER3_OFS 0x0fcc
REGDEF_BEGIN(DECODER_QUALITY_TABLE_1_REGISTER3)
REGDEF_BIT(DCT_QTBL1_DEC_P2,        10)
REGDEF_END(DECODER_QUALITY_TABLE_1_REGISTER3)


/*
    DCT_QTBL2_DEC_P0:    [0x0, 0x3ff],          bits : 9_0
    DCT_QTBL2_DEC_P1:    [0x0, 0x3ff],          bits : 25_16
*/
#define DECODER_QUALITY_TABLE_2_REGISTER4_OFS 0x0fd0
REGDEF_BEGIN(DECODER_QUALITY_TABLE_2_REGISTER4)
REGDEF_BIT(DCT_QTBL2_DEC_P0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCT_QTBL2_DEC_P1,        10)
REGDEF_END(DECODER_QUALITY_TABLE_2_REGISTER4)


/*
    DCT_QTBL2_DEC_P2:    [0x0, 0x3ff],          bits : 9_0
*/
#define DECODER_QUALITY_TABLE_2_REGISTER5_OFS 0x0fd4
REGDEF_BEGIN(DECODER_QUALITY_TABLE_2_REGISTER5)
REGDEF_BIT(DCT_QTBL2_DEC_P2,        10)
REGDEF_END(DECODER_QUALITY_TABLE_2_REGISTER5)


/*
    DCT_QTBL3_DEC_P0:    [0x0, 0x3ff],          bits : 9_0
    DCT_QTBL3_DEC_P1:    [0x0, 0x3ff],          bits : 25_16
*/
#define DECODER_QUALITY_TABLE_3_REGISTER6_OFS 0x0fd8
REGDEF_BEGIN(DECODER_QUALITY_TABLE_3_REGISTER6)
REGDEF_BIT(DCT_QTBL3_DEC_P0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCT_QTBL3_DEC_P1,        10)
REGDEF_END(DECODER_QUALITY_TABLE_3_REGISTER6)


/*
    DCT_QTBL3_DEC_P2:    [0x0, 0x3ff],          bits : 9_0
*/
#define DECODER_QUALITY_TABLE_3_REGISTER7_OFS 0x0fdc
REGDEF_BEGIN(DECODER_QUALITY_TABLE_3_REGISTER7)
REGDEF_BIT(DCT_QTBL3_DEC_P2,        10)
REGDEF_END(DECODER_QUALITY_TABLE_3_REGISTER7)


/*
    DCT_QTBL4_DEC_P0:    [0x0, 0x3ff],          bits : 9_0
    DCT_QTBL4_DEC_P1:    [0x0, 0x3ff],          bits : 25_16
*/
#define DECODER_QUALITY_TABLE_4_REGISTER8_OFS 0x0fe0
REGDEF_BEGIN(DECODER_QUALITY_TABLE_4_REGISTER8)
REGDEF_BIT(DCT_QTBL4_DEC_P0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCT_QTBL4_DEC_P1,        10)
REGDEF_END(DECODER_QUALITY_TABLE_4_REGISTER8)


/*
    DCT_QTBL4_DEC_P2:    [0x0, 0x3ff],          bits : 9_0
*/
#define DECODER_QUALITY_TABLE_4_REGISTER9_OFS 0x0fe4
REGDEF_BEGIN(DECODER_QUALITY_TABLE_4_REGISTER9)
REGDEF_BIT(DCT_QTBL4_DEC_P2,        10)
REGDEF_END(DECODER_QUALITY_TABLE_4_REGISTER9)


/*
    DCT_QTBL5_DEC_P0:    [0x0, 0x3ff],          bits : 9_0
    DCT_QTBL5_DEC_P1:    [0x0, 0x3ff],          bits : 25_16
*/
#define DECODER_QUALITY_TABLE_5_REGISTER10_OFS 0x0fe8
REGDEF_BEGIN(DECODER_QUALITY_TABLE_5_REGISTER10)
REGDEF_BIT(DCT_QTBL5_DEC_P0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCT_QTBL5_DEC_P1,        10)
REGDEF_END(DECODER_QUALITY_TABLE_5_REGISTER10)


/*
    DCT_QTBL5_DEC_P2:    [0x0, 0x3ff],          bits : 9_0
*/
#define DECODER_QUALITY_TABLE_5_REGISTER11_OFS 0x0fec
REGDEF_BEGIN(DECODER_QUALITY_TABLE_5_REGISTER11)
REGDEF_BIT(DCT_QTBL5_DEC_P2,        10)
REGDEF_END(DECODER_QUALITY_TABLE_5_REGISTER11)


/*
    DCT_QTBL6_DEC_P0:    [0x0, 0x3ff],          bits : 9_0
    DCT_QTBL6_DEC_P1:    [0x0, 0x3ff],          bits : 25_16
*/
#define DECODER_QUALITY_TABLE_6_REGISTER12_OFS 0x0ff0
REGDEF_BEGIN(DECODER_QUALITY_TABLE_6_REGISTER12)
REGDEF_BIT(DCT_QTBL6_DEC_P0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCT_QTBL6_DEC_P1,        10)
REGDEF_END(DECODER_QUALITY_TABLE_6_REGISTER12)


/*
    DCT_QTBL6_DEC_P2:    [0x0, 0x3ff],          bits : 9_0
*/
#define DECODER_QUALITY_TABLE_6_REGISTER13_OFS 0x0ff4
REGDEF_BEGIN(DECODER_QUALITY_TABLE_6_REGISTER13)
REGDEF_BIT(DCT_QTBL6_DEC_P2,        10)
REGDEF_END(DECODER_QUALITY_TABLE_6_REGISTER13)


/*
    DCT_QTBL7_DEC_P0:    [0x0, 0x3ff],          bits : 9_0
    DCT_QTBL7_DEC_P1:    [0x0, 0x3ff],          bits : 25_16
*/
#define DECODER_QUALITY_TABLE_7_REGISTER14_OFS 0x0ff8
REGDEF_BEGIN(DECODER_QUALITY_TABLE_7_REGISTER14)
REGDEF_BIT(DCT_QTBL7_DEC_P0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCT_QTBL7_DEC_P1,        10)
REGDEF_END(DECODER_QUALITY_TABLE_7_REGISTER14)


/*
    DCT_QTBL7_DEC_P2:    [0x0, 0x3ff],          bits : 9_0
*/
#define DECODER_QUALITY_TABLE_7_REGISTER15_OFS 0x0ffc
REGDEF_BEGIN(DECODER_QUALITY_TABLE_7_REGISTER15)
REGDEF_BIT(DCT_QTBL7_DEC_P2,        10)
REGDEF_END(DECODER_QUALITY_TABLE_7_REGISTER15)


/*
    DCT_QTBL8_DEC_P0:    [0x0, 0x3ff],          bits : 9_0
    DCT_QTBL8_DEC_P1:    [0x0, 0x3ff],          bits : 25_16
*/
#define DECODER_QUALITY_TABLE_8_REGISTER16_OFS 0x1000
REGDEF_BEGIN(DECODER_QUALITY_TABLE_8_REGISTER16)
REGDEF_BIT(DCT_QTBL8_DEC_P0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCT_QTBL8_DEC_P1,        10)
REGDEF_END(DECODER_QUALITY_TABLE_8_REGISTER16)


/*
    DCT_QTBL8_DEC_P2:    [0x0, 0x3ff],          bits : 9_0
*/
#define DECODER_QUALITY_TABLE_8_REGISTER17_OFS 0x1004
REGDEF_BEGIN(DECODER_QUALITY_TABLE_8_REGISTER17)
REGDEF_BIT(DCT_QTBL8_DEC_P2,        10)
REGDEF_END(DECODER_QUALITY_TABLE_8_REGISTER17)


/*
    DCT_QTBL9_DEC_P0:    [0x0, 0x3ff],          bits : 9_0
    DCT_QTBL9_DEC_P1:    [0x0, 0x3ff],          bits : 25_16
*/
#define DECODER_QUALITY_TABLE_9_REGISTER18_OFS 0x1008
REGDEF_BEGIN(DECODER_QUALITY_TABLE_9_REGISTER18)
REGDEF_BIT(DCT_QTBL9_DEC_P0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCT_QTBL9_DEC_P1,        10)
REGDEF_END(DECODER_QUALITY_TABLE_9_REGISTER18)


/*
    DCT_QTBL9_DEC_P2:    [0x0, 0x3ff],          bits : 9_0
*/
#define DECODER_QUALITY_TABLE_9_REGISTER19_OFS 0x100c
REGDEF_BEGIN(DECODER_QUALITY_TABLE_9_REGISTER19)
REGDEF_BIT(DCT_QTBL9_DEC_P2,        10)
REGDEF_END(DECODER_QUALITY_TABLE_9_REGISTER19)


/*
    DCT_QTBL10_DEC_P0:    [0x0, 0x3ff],         bits : 9_0
    DCT_QTBL10_DEC_P1:    [0x0, 0x3ff],         bits : 25_16
*/
#define DECODER_QUALITY_TABLE_10_REGISTER20_OFS 0x1010
REGDEF_BEGIN(DECODER_QUALITY_TABLE_10_REGISTER20)
REGDEF_BIT(DCT_QTBL10_DEC_P0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCT_QTBL10_DEC_P1,        10)
REGDEF_END(DECODER_QUALITY_TABLE_10_REGISTER20)


/*
    DCT_QTBL10_DEC_P2:    [0x0, 0x3ff],         bits : 9_0
*/
#define DECODER_QUALITY_TABLE_10_REGISTER21_OFS 0x1014
REGDEF_BEGIN(DECODER_QUALITY_TABLE_10_REGISTER21)
REGDEF_BIT(DCT_QTBL10_DEC_P2,        10)
REGDEF_END(DECODER_QUALITY_TABLE_10_REGISTER21)


/*
    DCT_QTBL11_DEC_P0:    [0x0, 0x3ff],         bits : 9_0
    DCT_QTBL11_DEC_P1:    [0x0, 0x3ff],         bits : 25_16
*/
#define DECODER_QUALITY_TABLE_11_REGISTER22_OFS 0x1018
REGDEF_BEGIN(DECODER_QUALITY_TABLE_11_REGISTER22)
REGDEF_BIT(DCT_QTBL11_DEC_P0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCT_QTBL11_DEC_P1,        10)
REGDEF_END(DECODER_QUALITY_TABLE_11_REGISTER22)


/*
    DCT_QTBL11_DEC_P2:    [0x0, 0x3ff],         bits : 9_0
*/
#define DECODER_QUALITY_TABLE_11_REGISTER23_OFS 0x101c
REGDEF_BEGIN(DECODER_QUALITY_TABLE_11_REGISTER23)
REGDEF_BIT(DCT_QTBL11_DEC_P2,        10)
REGDEF_END(DECODER_QUALITY_TABLE_11_REGISTER23)


/*
    DCT_QTBL12_DEC_P0:    [0x0, 0x3ff],         bits : 9_0
    DCT_QTBL12_DEC_P1:    [0x0, 0x3ff],         bits : 25_16
*/
#define DECODER_QUALITY_TABLE_12_REGISTER24_OFS 0x1020
REGDEF_BEGIN(DECODER_QUALITY_TABLE_12_REGISTER24)
REGDEF_BIT(DCT_QTBL12_DEC_P0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCT_QTBL12_DEC_P1,        10)
REGDEF_END(DECODER_QUALITY_TABLE_12_REGISTER24)


/*
    DCT_QTBL12_DEC_P2:    [0x0, 0x3ff],         bits : 9_0
*/
#define DECODER_QUALITY_TABLE_12_REGISTER25_OFS 0x1024
REGDEF_BEGIN(DECODER_QUALITY_TABLE_12_REGISTER25)
REGDEF_BIT(DCT_QTBL12_DEC_P2,        10)
REGDEF_END(DECODER_QUALITY_TABLE_12_REGISTER25)


/*
    DCT_QTBL13_DEC_P0:    [0x0, 0x3ff],         bits : 9_0
    DCT_QTBL13_DEC_P1:    [0x0, 0x3ff],         bits : 25_16
*/
#define DECODER_QUALITY_TABLE_13_REGISTER26_OFS 0x1028
REGDEF_BEGIN(DECODER_QUALITY_TABLE_13_REGISTER26)
REGDEF_BIT(DCT_QTBL13_DEC_P0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCT_QTBL13_DEC_P1,        10)
REGDEF_END(DECODER_QUALITY_TABLE_13_REGISTER26)


/*
    DCT_QTBL13_DEC_P2:    [0x0, 0x3ff],         bits : 9_0
*/
#define DECODER_QUALITY_TABLE_13_REGISTER27_OFS 0x102c
REGDEF_BEGIN(DECODER_QUALITY_TABLE_13_REGISTER27)
REGDEF_BIT(DCT_QTBL13_DEC_P2,        10)
REGDEF_END(DECODER_QUALITY_TABLE_13_REGISTER27)


/*
    DCT_QTBL14_DEC_P0:    [0x0, 0x3ff],         bits : 9_0
    DCT_QTBL14_DEC_P1:    [0x0, 0x3ff],         bits : 25_16
*/
#define DECODER_QUALITY_TABLE_14_REGISTER28_OFS 0x1030
REGDEF_BEGIN(DECODER_QUALITY_TABLE_14_REGISTER28)
REGDEF_BIT(DCT_QTBL14_DEC_P0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCT_QTBL14_DEC_P1,        10)
REGDEF_END(DECODER_QUALITY_TABLE_14_REGISTER28)


/*
    DCT_QTBL14_DEC_P2:    [0x0, 0x3ff],         bits : 9_0
*/
#define DECODER_QUALITY_TABLE_14_REGISTER29_OFS 0x1034
REGDEF_BEGIN(DECODER_QUALITY_TABLE_14_REGISTER29)
REGDEF_BIT(DCT_QTBL14_DEC_P2,        10)
REGDEF_END(DECODER_QUALITY_TABLE_14_REGISTER29)


/*
    DCT_QTBL15_DEC_P0:    [0x0, 0x3ff],         bits : 9_0
    DCT_QTBL15_DEC_P1:    [0x0, 0x3ff],         bits : 25_16
*/
#define DECODER_QUALITY_TABLE_15_REGISTER30_OFS 0x1038
REGDEF_BEGIN(DECODER_QUALITY_TABLE_15_REGISTER30)
REGDEF_BIT(DCT_QTBL15_DEC_P0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(DCT_QTBL15_DEC_P1,        10)
REGDEF_END(DECODER_QUALITY_TABLE_15_REGISTER30)


/*
    DCT_QTBL15_DEC_P2:    [0x0, 0x3ff],         bits : 9_0
*/
#define DECODER_QUALITY_TABLE_15_REGISTER31_OFS 0x103c
REGDEF_BEGIN(DECODER_QUALITY_TABLE_15_REGISTER31)
REGDEF_BIT(DCT_QTBL15_DEC_P2,        10)
REGDEF_END(DECODER_QUALITY_TABLE_15_REGISTER31)


typedef struct _NT98528_VPE_REGISTER_STRUCT_ {

	union {
		struct {
			unsigned vpe_sw_rst            : 1;     // bits : 0
			unsigned vpe_dma_sw_rst        : 1;     // bits : 1
			unsigned                       : 28;
			unsigned vpe_start             : 1;     // bits : 30
			unsigned vpe_ll_fire           : 1;     // bits : 31
		} bit;
		UINT32 word;
	} reg_0; // 0x0000

	union {
		struct {
			unsigned                            : 2;
			unsigned vpe_shp_en                 : 1;        // bits : 2
			unsigned dce_en                     : 1;        // bits : 3
			unsigned vpe_dctg_en                : 1;        // bits : 4
			unsigned vpe_dctg_gma_lut_en        : 1;        // bits : 5
			unsigned dce_2d_lut_en              : 1;        // bits : 6
			unsigned                            : 13;
			unsigned vpe_debug_type             : 4;        // bits : 23_20
			unsigned vpe_col_num                : 3;        // bits : 26_24
			unsigned                            : 4;
			unsigned vpe_ll_terminate           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_1; // 0x0004

	union {
		struct {
			unsigned                        : 10;
			unsigned vpe_src_fmt            : 3;        // bits : 12_10
			unsigned                        : 4;
			unsigned vpe_shp_out_sel        : 1;        // bits : 17
			unsigned                        : 2;
			unsigned vpe_src_drt            : 2;        // bits : 21_20
			unsigned                        : 2;
			unsigned vpe_src_uv_swap        : 1;        // bits : 24
		} bit;
		UINT32 word;
	} reg_2; // 0x0008

	union {
		struct {
			unsigned vpe_reserved_0        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_3; // 0x000c

	union {
		struct {
			unsigned vpe_src_buf_width         : 14;        // bits : 13_0
			unsigned                           : 2;
			unsigned vpe_src_buf_height        : 13;        // bits : 28_16
		} bit;
		UINT32 word;
	} reg_4; // 0x0010

	union {
		struct {
			unsigned vpe_reserved_1        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_5; // 0x0014

	union {
		struct {
			unsigned vpe_reserved_2        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_6; // 0x0018

	union {
		struct {
			unsigned vpe_proc_height           : 13;        // bits : 12_0
			unsigned                           : 3;
			unsigned presca_merge_width        : 13;        // bits : 28_16
		} bit;
		UINT32 word;
	} reg_7; // 0x001c

	union {
		struct {
			unsigned vpe_proc_y_start        : 13;      // bits : 12_0
		} bit;
		UINT32 word;
	} reg_8; // 0x0020

	union {
		struct {
			unsigned vpe_reserved_2        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_9; // 0x0024

	union {
		struct {
			unsigned vpe_reserved_3        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_10; // 0x0028

	union {
		struct {
			unsigned vpe_reserved_4        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_11; // 0x002c

	union {
		struct {
			unsigned dce_width         : 13;        // bits : 12_0
			unsigned                   : 3;
			unsigned dce_height        : 13;        // bits : 28_16
		} bit;
		UINT32 word;
	} reg_12; // 0x0030

	union {
		struct {
			unsigned vpe_reserved_5        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_13; // 0x0034

	union {
		struct {
			unsigned vpe_reserved_6        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_14; // 0x0038

	union {
		struct {
			unsigned vpe_reserved_7        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_15; // 0x003c

	union {
		struct {
			unsigned vpe_reserved_8        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_16; // 0x0040

	union {
		struct {
			unsigned vpe_reserved_9        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_17; // 0x0044

	union {
		struct {
			unsigned vpe_reserved_10        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_18; // 0x0048

	union {
		struct {
			unsigned vpe_reserved_11        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_19; // 0x004c

	union {
		struct {
			unsigned vpe_reserved_12        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_20; // 0x0050

	union {
		struct {
			unsigned vpe_reserved_13        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_21; // 0x0054

	union {
		struct {
			unsigned vpe_reserved_14        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_22; // 0x0058

	union {
		struct {
			unsigned vpe_reserved_15        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_23; // 0x005c

	union {
		struct {
			unsigned vpe_reserved_16        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_24; // 0x0060

	union {
		struct {
			unsigned vpe_reserved_17        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_25; // 0x0064

	union {
		struct {
			unsigned vpe_reserved_18        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_26; // 0x0068

	union {
		struct {
			unsigned vpe_reserved_19        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_27; // 0x006c

	union {
		struct {
			unsigned vpe_reserved_20        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_28; // 0x0070

	union {
		struct {
			unsigned vpe_reserved_21        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_29; // 0x0074

	union {
		struct {
			unsigned vpe_reserved_22        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_30; // 0x0078

	union {
		struct {
			unsigned vpe_reserved_23        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_31; // 0x007c

	union {
		struct {
			unsigned vpe_reserved_24        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_32; // 0x0080

	union {
		struct {
			unsigned vpe_reserved_25        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_33; // 0x0084

	union {
		struct {
			unsigned vpe_reserved_26        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_34; // 0x0088

	union {
		struct {
			unsigned vpe_reserved_27        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_35; // 0x008c

	union {
		struct {
			unsigned vpe_reserved_28        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_36; // 0x0090

	union {
		struct {
			unsigned vpe_reserved_29        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_37; // 0x0094

	union {
		struct {
			unsigned vpe_reserved_30        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_38; // 0x0098

	union {
		struct {
			unsigned vpe_reserved_31        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_39; // 0x009c

	union {
		struct {
			unsigned vpe_reserved_32        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_40; // 0x00a0

	union {
		struct {
			unsigned vpe_reserved_33        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_41; // 0x00a4

	union {
		struct {
			unsigned vpe_reserved_34        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_42; // 0x00a8

	union {
		struct {
			unsigned vpe_reserved_35        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_43; // 0x00ac

	union {
		struct {
			unsigned                    : 4;
			unsigned vch_idx_sht        : 4;        // bits : 7_4
			unsigned                    : 4;
			unsigned vch_idx_set        : 1;        // bits : 12
			unsigned                    : 3;
			unsigned vch_idx_oft        : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_44; // 0x00b0

	union {
		struct {
			unsigned vch_oft_mode        : 4;       // bits : 3_0
			unsigned vch_oft_sht         : 2;       // bits : 5_4
		} bit;
		UINT32 word;
	} reg_45; // 0x00b4

	union {
		struct {
			unsigned vch_slot_width         : 9;        // bits : 8_0
			unsigned                        : 7;
			unsigned vch_slot_height        : 9;        // bits : 24_16
		} bit;
		UINT32 word;
	} reg_46; // 0x00b8

	union {
		struct {
			unsigned vch_oft_width         : 9;     // bits : 8_0
			unsigned                       : 7;
			unsigned vch_oft_height        : 9;     // bits : 24_16
		} bit;
		UINT32 word;
	} reg_47; // 0x00bc

	union {
		struct {
			unsigned dce_mode            : 2;       // bits : 1_0
			unsigned dce_lut2d_sz        : 3;       // bits : 4_2
			unsigned dce_lsb_rand        : 1;       // bits : 5
		} bit;
		UINT32 word;
	} reg_48; // 0x00c0

	union {
		struct {
			unsigned dce_fov_bound        : 1;      // bits : 0
			unsigned                      : 15;
			unsigned dce_bound_y          : 10;     // bits : 25_16
		} bit;
		UINT32 word;
	} reg_49; // 0x00c4

	union {
		struct {
			unsigned dce_bound_u        : 10;       // bits : 9_0
			unsigned                    : 6;
			unsigned dce_bound_v        : 10;       // bits : 25_16
		} bit;
		UINT32 word;
	} reg_50; // 0x00c8

	union {
		struct {
			unsigned dce_cent_x_s        : 14;      // bits : 13_0
			unsigned                     : 2;
			unsigned dce_cent_y_s        : 14;      // bits : 29_16
		} bit;
		UINT32 word;
	} reg_51; // 0x00cc

	union {
		struct {
			unsigned dce_xdist        : 12;     // bits : 11_0
			unsigned                  : 4;
			unsigned dce_ydist        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_52; // 0x00d0

	union {
		struct {
			unsigned dce_norm_fact        : 10;     // bits : 9_0
			unsigned                      : 2;
			unsigned dce_norm_bit         : 5;      // bits : 16_12
			unsigned                      : 3;
			unsigned dce_fov_gain         : 12;     // bits : 31_20
		} bit;
		UINT32 word;
	} reg_53; // 0x00d4

	union {
		struct {
			unsigned dce_hfact        : 24;     // bits : 23_0
		} bit;
		UINT32 word;
	} reg_54; // 0x00d8

	union {
		struct {
			unsigned dce_vfact        : 24;     // bits : 23_0
		} bit;
		UINT32 word;
	} reg_55; // 0x00dc

	union {
		struct {
			unsigned dce_xofs_int        : 9;       // bits : 8_0
			unsigned                     : 7;
			unsigned dce_yofs_int        : 9;       // bits : 24_16
		} bit;
		UINT32 word;
	} reg_56; // 0x00e0

	union {
		struct {
			unsigned dce_xofs_frc        : 24;      // bits : 23_0
		} bit;
		UINT32 word;
	} reg_57; // 0x00e4

	union {
		struct {
			unsigned dce_yofs_frc        : 24;      // bits : 23_0
		} bit;
		UINT32 word;
	} reg_58; // 0x00e8

	union {
		struct {
			unsigned vpe_reserved_36        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_59; // 0x00ec

	union {
		struct {
			unsigned vpe_reserved_37        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_60; // 0x00f0

	union {
		struct {
			unsigned vpe_reserved_38        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_61; // 0x00f4

	union {
		struct {
			unsigned vpe_reserved_39        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_62; // 0x00f8

	union {
		struct {
			unsigned vpe_reserved_40        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_63; // 0x00fc

	union {
		struct {
			unsigned vpe_reserved_41        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_64; // 0x0100

	union {
		struct {
			unsigned vpe_reserved_42        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_65; // 0x0104

	union {
		struct {
			unsigned vpe_reserved_43        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_66; // 0x0108

	union {
		struct {
			unsigned vpe_reserved_44        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_67; // 0x010c

	union {
		struct {
			unsigned vpe_reserved_45        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_68; // 0x0110

	union {
		struct {
			unsigned vpe_reserved_46        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_69; // 0x0114

	union {
		struct {
			unsigned vpe_reserved_47        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_70; // 0x0118

	union {
		struct {
			unsigned vpe_reserved_48        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_71; // 0x011c

	union {
		struct {
			unsigned vpe_reserved_49        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_72; // 0x0120

	union {
		struct {
			unsigned vpe_reserved_50        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_73; // 0x0124

	union {
		struct {
			unsigned vpe_reserved_51        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_74; // 0x0128

	union {
		struct {
			unsigned vpe_reserved_52        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_75; // 0x012c

	union {
		struct {
			unsigned vpe_reserved_53        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_76; // 0x0130

	union {
		struct {
			unsigned vpe_reserved_54        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_77; // 0x0134

	union {
		struct {
			unsigned vpe_reserved_55        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_78; // 0x0138

	union {
		struct {
			unsigned vpe_reserved_56        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_79; // 0x013c

	union {
		struct {
			unsigned vpe_reserved_57        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_80; // 0x0140

	union {
		struct {
			unsigned vpe_reserved_58        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_81; // 0x0144

	union {
		struct {
			unsigned vpe_reserved_59        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_82; // 0x0148

	union {
		struct {
			unsigned vpe_reserved_60        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_83; // 0x014c

	union {
		struct {
			unsigned vpe_reserved_61        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_84; // 0x0150

	union {
		struct {
			unsigned vpe_reserved_62        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_85; // 0x0154

	union {
		struct {
			unsigned vpe_reserved_63        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_86; // 0x0158

	union {
		struct {
			unsigned vpe_reserved_64        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_87; // 0x015c

	union {
		struct {
			unsigned vpe_reserved_65        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_88; // 0x0160

	union {
		struct {
			unsigned vpe_reserved_66        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_89; // 0x0164

	union {
		struct {
			unsigned vpe_reserved_67        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_90; // 0x0168

	union {
		struct {
			unsigned vpe_reserved_68        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_91; // 0x016c

	union {
		struct {
			unsigned vpe_reserved_69        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_92; // 0x0170

	union {
		struct {
			unsigned vpe_reserved_70        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_93; // 0x0174

	union {
		struct {
			unsigned vpe_reserved_71        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_94; // 0x0178

	union {
		struct {
			unsigned vpe_reserved_72        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_95; // 0x017c

	union {
		struct {
			unsigned vpe_reserved_73        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_96; // 0x0180

	union {
		struct {
			unsigned vpe_reserved_74        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_97; // 0x0184

	union {
		struct {
			unsigned vpe_reserved_75        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_98; // 0x0188

	union {
		struct {
			unsigned vpe_reserved_76        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_99; // 0x018c

	union {
		struct {
			unsigned vpe_reserved_77        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_100; // 0x0190

	union {
		struct {
			unsigned vpe_reserved_78        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_101; // 0x0194

	union {
		struct {
			unsigned vpe_reserved_79        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_102; // 0x0198

	union {
		struct {
			unsigned vpe_reserved_80        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_103; // 0x019c

	union {
		struct {
			unsigned vpe_reserved_81        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_104; // 0x01a0

	union {
		struct {
			unsigned vpe_reserved_82        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_105; // 0x01a4

	union {
		struct {
			unsigned vpe_reserved_83        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_106; // 0x01a8

	union {
		struct {
			unsigned vpe_reserved_84        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_107; // 0x01ac

	union {
		struct {
			unsigned vpe_reserved_85        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_108; // 0x01b0

	union {
		struct {
			unsigned vpe_reserved_86        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_109; // 0x01b4

	union {
		struct {
			unsigned vpe_reserved_87        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_110; // 0x01b8

	union {
		struct {
			unsigned vpe_reserved_88        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_111; // 0x01bc

	union {
		struct {
			unsigned vpe_reserved_89        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_112; // 0x01c0

	union {
		struct {
			unsigned vpe_reserved_90        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_113; // 0x01c4

	union {
		struct {
			unsigned vpe_reserved_91        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_114; // 0x01c8

	union {
		struct {
			unsigned vpe_reserved_92        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_115; // 0x01cc

	union {
		struct {
			unsigned vpe_reserved_93        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_116; // 0x01d0

	union {
		struct {
			unsigned vpe_reserved_94        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_117; // 0x01d4

	union {
		struct {
			unsigned vpe_reserved_95        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_118; // 0x01d8

	union {
		struct {
			unsigned vpe_reserved_96        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_119; // 0x01dc

	union {
		struct {
			unsigned vpe_reserved_97        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_120; // 0x01e0

	union {
		struct {
			unsigned vpe_reserved_98        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_121; // 0x01e4

	union {
		struct {
			unsigned vpe_reserved_99        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_122; // 0x01e8

	union {
		struct {
			unsigned vpe_reserved_100        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_123; // 0x01ec

	union {
		struct {
			unsigned vpe_reserved_101        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_124; // 0x01f0

	union {
		struct {
			unsigned vpe_reserved_102        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_125; // 0x01f4

	union {
		struct {
			unsigned vpe_reserved_103        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_126; // 0x01f8

	union {
		struct {
			unsigned vpe_reserved_104        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_127; // 0x01fc

	union {
		struct {
			unsigned vpe_reserved_105        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_128; // 0x0200

	union {
		struct {
			unsigned vpe_reserved_106        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_129; // 0x0204

	union {
		struct {
			unsigned vpe_reserved_107        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_130; // 0x0208

	union {
		struct {
			unsigned vpe_reserved_108        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_131; // 0x020c

	union {
		struct {
			unsigned vpe_reserved_109        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_132; // 0x0210

	union {
		struct {
			unsigned vpe_reserved_110        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_133; // 0x0214

	union {
		struct {
			unsigned vpe_reserved_111        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_134; // 0x0218

	union {
		struct {
			unsigned vpe_reserved_112        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_135; // 0x021c

	union {
		struct {
			unsigned vpe_reserved_113        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_136; // 0x0220

	union {
		struct {
			unsigned vpe_reserved_114        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_137; // 0x0224

	union {
		struct {
			unsigned vpe_reserved_115        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_138; // 0x0228

	union {
		struct {
			unsigned vpe_reserved_116        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_139; // 0x022c

	union {
		struct {
			unsigned vpe_reserved_117        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_140; // 0x0230

	union {
		struct {
			unsigned vpe_reserved_118        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_141; // 0x0234

	union {
		struct {
			unsigned vpe_reserved_119        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_142; // 0x0238

	union {
		struct {
			unsigned vpe_reserved_120        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_143; // 0x023c

	union {
		struct {
			unsigned eg_wet_src_sel        : 1;     // bits : 0
			unsigned                       : 7;
			unsigned eg_wet_th             : 8;     // bits : 15_8
			unsigned eg_wet_gain           : 8;     // bits : 23_16
			unsigned noise_level           : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_144; // 0x0240

	union {
		struct {
			unsigned eg_shp_str1         : 8;       // bits : 7_0
			unsigned eg_shp_str2         : 8;       // bits : 15_8
			unsigned flat_shp_str        : 8;       // bits : 23_16
		} bit;
		UINT32 word;
	} reg_145; // 0x0244

	union {
		struct {
			unsigned coring_th               : 8;       // bits : 7_0
			unsigned blend_inv_gma           : 8;       // bits : 15_8
			unsigned bright_halo_clip        : 8;       // bits : 23_16
			unsigned dark_halo_clip          : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_146; // 0x0248

	union {
		struct {
			unsigned noise_curve0        : 8;       // bits : 7_0
			unsigned noise_curve1        : 8;       // bits : 15_8
			unsigned noise_curve2        : 8;       // bits : 23_16
			unsigned noise_curve3        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_147; // 0x024c

	union {
		struct {
			unsigned noise_curve4        : 8;       // bits : 7_0
			unsigned noise_curve5        : 8;       // bits : 15_8
			unsigned noise_curve6        : 8;       // bits : 23_16
			unsigned noise_curve7        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_148; // 0x0250

	union {
		struct {
			unsigned noise_curve8         : 8;      // bits : 7_0
			unsigned noise_curve9         : 8;      // bits : 15_8
			unsigned noise_curve10        : 8;      // bits : 23_16
			unsigned noise_curve11        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_149; // 0x0254

	union {
		struct {
			unsigned noise_curve12        : 8;      // bits : 7_0
			unsigned noise_curve13        : 8;      // bits : 15_8
			unsigned noise_curve14        : 8;      // bits : 23_16
			unsigned noise_curve15        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_150; // 0x0258

	union {
		struct {
			unsigned noise_curve16        : 8;      // bits : 7_0
		} bit;
		UINT32 word;
	} reg_151; // 0x025c

	union {
		struct {
			unsigned pal0_y         : 8;        // bits : 7_0
			unsigned pal0_cb        : 8;        // bits : 15_8
			unsigned pal0_cr        : 8;        // bits : 23_16
		} bit;
		UINT32 word;
	} reg_152; // 0x0260

	union {
		struct {
			unsigned pal1_y         : 8;        // bits : 7_0
			unsigned pal1_cb        : 8;        // bits : 15_8
			unsigned pal1_cr        : 8;        // bits : 23_16
		} bit;
		UINT32 word;
	} reg_153; // 0x0264

	union {
		struct {
			unsigned pal2_y         : 8;        // bits : 7_0
			unsigned pal2_cb        : 8;        // bits : 15_8
			unsigned pal2_cr        : 8;        // bits : 23_16
		} bit;
		UINT32 word;
	} reg_154; // 0x0268

	union {
		struct {
			unsigned pal3_y         : 8;        // bits : 7_0
			unsigned pal3_cb        : 8;        // bits : 15_8
			unsigned pal3_cr        : 8;        // bits : 23_16
		} bit;
		UINT32 word;
	} reg_155; // 0x026c

	union {
		struct {
			unsigned pal4_y         : 8;        // bits : 7_0
			unsigned pal4_cb        : 8;        // bits : 15_8
			unsigned pal4_cr        : 8;        // bits : 23_16
		} bit;
		UINT32 word;
	} reg_156; // 0x0270

	union {
		struct {
			unsigned pal5_y         : 8;        // bits : 7_0
			unsigned pal5_cb        : 8;        // bits : 15_8
			unsigned pal5_cr        : 8;        // bits : 23_16
		} bit;
		UINT32 word;
	} reg_157; // 0x0274

	union {
		struct {
			unsigned pal6_y         : 8;        // bits : 7_0
			unsigned pal6_cb        : 8;        // bits : 15_8
			unsigned pal6_cr        : 8;        // bits : 23_16
		} bit;
		UINT32 word;
	} reg_158; // 0x0278

	union {
		struct {
			unsigned pal7_y         : 8;        // bits : 7_0
			unsigned pal7_cb        : 8;        // bits : 15_8
			unsigned pal7_cr        : 8;        // bits : 23_16
		} bit;
		UINT32 word;
	} reg_159; // 0x027c

	union {
		struct {
			unsigned vpe_reserved_121        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_160; // 0x0280

	union {
		struct {
			unsigned vpe_reserved_122        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_161; // 0x0284

	union {
		struct {
			unsigned vpe_reserved_123        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_162; // 0x0288

	union {
		struct {
			unsigned vpe_reserved_124        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_163; // 0x028c

	union {
		struct {
			unsigned vpe_reserved_125        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_164; // 0x0290

	union {
		struct {
			unsigned vpe_reserved_126        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_165; // 0x0294

	union {
		struct {
			unsigned vpe_reserved_127        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_166; // 0x0298

	union {
		struct {
			unsigned vpe_reserved_128        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_167; // 0x029c

	union {
		struct {
			unsigned vpe_reserved_129        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_168; // 0x02a0

	union {
		struct {
			unsigned vpe_reserved_130        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_169; // 0x02a4

	union {
		struct {
			unsigned vpe_reserved_131        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_170; // 0x02a8

	union {
		struct {
			unsigned vpe_reserved_132        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_171; // 0x02ac

	union {
		struct {
			unsigned vpe_reserved_133        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_172; // 0x02b0

	union {
		struct {
			unsigned vpe_reserved_134        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_173; // 0x02b4

	union {
		struct {
			unsigned vpe_reserved_135        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_174; // 0x02b8

	union {
		struct {
			unsigned vpe_reserved_136        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_175; // 0x02bc

	union {
		struct {
			unsigned vpe_reserved_137        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_176; // 0x02c0

	union {
		struct {
			unsigned vpe_reserved_138        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_177; // 0x02c4

	union {
		struct {
			unsigned vpe_reserved_139        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_178; // 0x02c8

	union {
		struct {
			unsigned vpe_reserved_140        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_179; // 0x02cc

	union {
		struct {
			unsigned vpe_reserved_141        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_180; // 0x02d0

	union {
		struct {
			unsigned vpe_reserved_142        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_181; // 0x02d4

	union {
		struct {
			unsigned vpe_reserved_143        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_182; // 0x02d8

	union {
		struct {
			unsigned vpe_reserved_144        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_183; // 0x02dc

	union {
		struct {
			unsigned vpe_reserved_145        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_184; // 0x02e0

	union {
		struct {
			unsigned vpe_reserved_146        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_185; // 0x02e4

	union {
		struct {
			unsigned vpe_reserved_147        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_186; // 0x02e8

	union {
		struct {
			unsigned vpe_reserved_148        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_187; // 0x02ec

	union {
		struct {
			unsigned vpe_reserved_149        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_188; // 0x02f0

	union {
		struct {
			unsigned vpe_reserved_150        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_189; // 0x02f4

	union {
		struct {
			unsigned vpe_reserved_151        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_190; // 0x02f8

	union {
		struct {
			unsigned vpe_reserved_152        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_191; // 0x02fc

	union {
		struct {
			unsigned llc_addr        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_192; // 0x0300

	union {
		struct {
			unsigned src_addr        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_193; // 0x0304

	union {
		struct {
			unsigned vpe_reserved_153        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_194; // 0x0308

	union {
		struct {
			unsigned vpe_reserved_154        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_195; // 0x030c

	union {
		struct {
			unsigned vpe_reserved_155        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_196; // 0x0310

	union {
		struct {
			unsigned vpe_reserved_156        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_197; // 0x0314

	union {
		struct {
			unsigned vpe_reserved_157        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_198; // 0x0318

	union {
		struct {
			unsigned dctg_gamma_addr        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_199; // 0x031c

	union {
		struct {
			unsigned dce_2dlut_addr        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_200; // 0x0320

	union {
		struct {
			unsigned vpe_reserved_158        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_201; // 0x0324

	union {
		struct {
			unsigned vpe_reserved_159        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_202; // 0x0328

	union {
		struct {
			unsigned vpe_reserved_160        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_203; // 0x032c

	union {
		struct {
			unsigned vpe_reserved_161        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_204; // 0x0330

	union {
		struct {
			unsigned vpe_reserved_162        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_205; // 0x0334

	union {
		struct {
			unsigned vpe_reserved_163        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_206; // 0x0338

	union {
		struct {
			unsigned vpe_reserved_164        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_207; // 0x033c

	union {
		struct {
			unsigned des_res0_dp0_addr        : 32;     // bits : 31_0
		} bit;
		UINT32 word;
	} reg_208; // 0x0340

	union {
		struct {
			unsigned vpe_reserved_165        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_209; // 0x0344

	union {
		struct {
			unsigned reserved         : 32;    // 31_0
		} bit;
		UINT32 word;
	} reg_210; // 0x0348

	union {
		struct {
			unsigned des_res1_dp0_addr        : 32;     // bits : 31_0
		} bit;
		UINT32 word;
	} reg_211; // 0x034c

	union {
		struct {
			unsigned vpe_reserved_166        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_212; // 0x0350

	union {
		struct {
			unsigned vpe_reserved_167        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_213; // 0x0354

	union {
		struct {
			unsigned des_res2_dp0_addr        : 32;     // bits : 31_0
		} bit;
		UINT32 word;
	} reg_214; // 0x0358

	union {
		struct {
			unsigned vpe_reserved_168        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_215; // 0x035c

	union {
		struct {
			unsigned vpe_reserved_563        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_216; // 0x0360

	union {
		struct {
			unsigned des_res3_dp0_addr        : 32;     // bits : 31_0
		} bit;
		UINT32 word;
	} reg_217; // 0x0364

	union {
		struct {
			unsigned vpe_reserved_169        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_218; // 0x0368

	union {
		struct {
			unsigned vpe_reserved_170        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_219; // 0x036c

	union {
		struct {
			unsigned vpe_reserved_171        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_220; // 0x0370

	union {
		struct {
			unsigned vpe_reserved_172        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_221; // 0x0374

	union {
		struct {
			unsigned vpe_reserved_173        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_222; // 0x0378

	union {
		struct {
			unsigned dma0_wcmd_wait_value        : 16;      // bits : 15_0
			unsigned dma0_rcmd_wait_value        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_223; // 0x037c

	union {
		struct {
			unsigned dma1_wcmd_wait_value        : 16;      // bits : 15_0
			unsigned dma1_rcmd_wait_value        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_224; // 0x0380

	union {
		struct {
			unsigned vpe_reserved_174        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_225; // 0x0384

	union {
		struct {
			unsigned vpe_reserved_175        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_226; // 0x0388

	union {
		struct {
			unsigned vpe_reserved_176        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_227; // 0x038c

	union {
		struct {
			unsigned vpe_reserved_177        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_228; // 0x0390

	union {
		struct {
			unsigned vpe_reserved_178        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_229; // 0x0394

	union {
		struct {
			unsigned vpe_reserved_179        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_230; // 0x0398

	union {
		struct {
			unsigned vpe_reserved_180        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_231; // 0x039c

	union {
		struct {
			unsigned vpe_reserved_181        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_232; // 0x03a0

	union {
		struct {
			unsigned vpe_reserved_182        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_233; // 0x03a4

	union {
		struct {
			unsigned vpe_reserved_183        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_234; // 0x03a8

	union {
		struct {
			unsigned vpe_reserved_184        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_235; // 0x03ac

	union {
		struct {
			unsigned vpe_reserved_185        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_236; // 0x03b0

	union {
		struct {
			unsigned vpe_reserved_186        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_237; // 0x03b4

	union {
		struct {
			unsigned vpe_reserved_187        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_238; // 0x03b8

	union {
		struct {
			unsigned vpe_reserved_188        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_239; // 0x03bc

	union {
		struct {
			unsigned shp_ppi_idx        : 4;        // bits : 3_0
			unsigned shp_ppo_idx        : 4;        // bits : 7_4
		} bit;
		UINT32 word;
	} reg_240; // 0x03c0

	union {
		struct {
			unsigned src_ppo_idx        : 4;        // bits : 3_0
			unsigned sca_ppi_idx        : 4;        // bits : 7_4
		} bit;
		UINT32 word;
	} reg_241; // 0x03c4

	union {
		struct {
			unsigned                         : 3;
			unsigned vpe_reserved_189        : 29;      // bits : 31_3
		} bit;
		UINT32 word;
	} reg_242; // 0x03c8

	union {
		struct {
			unsigned                         : 3;
			unsigned vpe_reserved_190        : 29;      // bits : 31_3
		} bit;
		UINT32 word;
	} reg_243; // 0x03cc

	union {
		struct {
			unsigned                         : 3;
			unsigned vpe_reserved_191        : 29;      // bits : 31_3
		} bit;
		UINT32 word;
	} reg_244; // 0x03d0

	union {
		struct {
			unsigned                         : 3;
			unsigned vpe_reserved_192        : 29;      // bits : 31_3
		} bit;
		UINT32 word;
	} reg_245; // 0x03d4

	union {
		struct {
			unsigned                         : 3;
			unsigned vpe_reserved_193        : 29;      // bits : 31_3
		} bit;
		UINT32 word;
	} reg_246; // 0x03d8

	union {
		struct {
			unsigned                         : 3;
			unsigned vpe_reserved_194        : 29;      // bits : 31_3
		} bit;
		UINT32 word;
	} reg_247; // 0x03dc

	union {
		struct {
			unsigned dctg_mount_type        : 1;        // bits : 0
			unsigned dctg_ra_en             : 1;        // bits : 1
			unsigned dctg_lut2d_sz          : 2;        // bits : 3_2
			unsigned                        : 12;
			unsigned dctg_lens_r            : 14;       // bits : 29_16
		} bit;
		UINT32 word;
	} reg_248; // 0x03e0

	union {
		struct {
			unsigned dctg_lens_x_st        : 13;        // bits : 12_0
			unsigned                       : 3;
			unsigned dctg_lens_y_st        : 13;        // bits : 28_16
		} bit;
		UINT32 word;
	} reg_249; // 0x03e4

	union {
		struct {
			unsigned dctg_theta_st        : 20;     // bits : 19_0
		} bit;
		UINT32 word;
	} reg_250; // 0x03e8

	union {
		struct {
			unsigned dctg_theta_ed        : 20;     // bits : 19_0
		} bit;
		UINT32 word;
	} reg_251; // 0x03ec

	union {
		struct {
			unsigned dctg_phi_st        : 20;       // bits : 19_0
		} bit;
		UINT32 word;
	} reg_252; // 0x03f0

	union {
		struct {
			unsigned dctg_phi_ed        : 20;       // bits : 19_0
		} bit;
		UINT32 word;
	} reg_253; // 0x03f4

	union {
		struct {
			unsigned dctg_rot_z        : 20;        // bits : 19_0
		} bit;
		UINT32 word;
	} reg_254; // 0x03f8

	union {
		struct {
			unsigned dctg_rot_y        : 20;        // bits : 19_0
		} bit;
		UINT32 word;
	} reg_255; // 0x03fc

	union {
		struct {
			unsigned vpe_ints_frame_done                : 1;        // bits : 0
			unsigned vpe_ints_ll_err                    : 1;        // bits : 1
			unsigned vpe_ints_ll_done                   : 1;        // bits : 2
			unsigned                                    : 7;
			unsigned vpe_ints_res0_near_frm_done        : 1;        // bits : 10
			unsigned vpe_ints_res1_near_frm_done        : 1;        // bits : 11
			unsigned vpe_ints_res2_near_frm_done        : 1;        // bits : 12
			unsigned vpe_ints_res3_near_frm_done        : 1;        // bits : 13
		} bit;
		UINT32 word;
	} reg_256; // 0x0400

	union {
		struct {
			unsigned vpe_inte_frame_done                : 1;        // bits : 0
			unsigned vpe_inte_ll_err                    : 1;        // bits : 1
			unsigned vpe_inte_ll_done                   : 1;        // bits : 2
			unsigned                                    : 7;
			unsigned vpe_inte_res0_near_frm_done        : 1;        // bits : 10
			unsigned vpe_inte_res1_near_frm_done        : 1;        // bits : 11
			unsigned vpe_inte_res2_near_frm_done        : 1;        // bits : 12
			unsigned vpe_inte_res3_near_frm_done        : 1;        // bits : 13
		} bit;
		UINT32 word;
	} reg_257; // 0x0404

	union {
		struct {
			unsigned res0_near_frm_done_th        : 16;     // bits : 15_0
			unsigned res1_near_frm_done_th        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_258; // 0x0408

	union {
		struct {
			unsigned res2_near_frm_done_th        : 16;     // bits : 15_0
			unsigned res3_near_frm_done_th        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_259; // 0x040c

	union {
		struct {
			unsigned vpe_mode                   : 1;        // bits : 0
			unsigned dma0_shut_down_done        : 1;        // bits : 1
			unsigned                            : 1;
			unsigned hw_idle                    : 1;        // bits : 3
		} bit;
		UINT32 word;
	} reg_260; // 0x0410

	union {
		struct {
			unsigned ip_version        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_261; // 0x0414

	union {
		struct {
			unsigned ip_info        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_262; // 0x0418

	union {
		struct {
			unsigned vpe_reserved_195        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_263; // 0x041c

	union {
		struct {
			unsigned vpe_reserved_196        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_264; // 0x0420

	union {
		struct {
			unsigned vpe_reserved_197        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_265; // 0x0424

	union {
		struct {
			unsigned vpe_reserved_198        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_266; // 0x0428

	union {
		struct {
			unsigned vpe_reserved_199        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_267; // 0x042c

	union {
		struct {
			unsigned fire_cnt              : 8;     // bits : 7_0
			unsigned done_cnt              : 8;     // bits : 15_8
			unsigned sys_cs                : 4;     // bits : 19_16
			unsigned dma_r_src_done        : 1;     // bits : 20
			unsigned                       : 3;
			unsigned dma_r_lut_done        : 1;     // bits : 24
			unsigned dma_w_des_done        : 1;     // bits : 25
			unsigned                       : 4;
			unsigned col_cnt               : 2;     // bits : 31_30
		} bit;
		UINT32 word;
	} reg_268; // 0x0430

	union {
		struct {
			unsigned vpe_reserved_200        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_269; // 0x0434

	union {
		struct {
			unsigned res0_yvd               : 1;        // bits : 0
			unsigned res0_y_block           : 1;        // bits : 1
			unsigned res0_y_line_num        : 11;       // bits : 12_2
			unsigned                        : 3;
			unsigned res0_cvd               : 1;        // bits : 16
			unsigned res0_c_block           : 1;        // bits : 17
			unsigned res0_c_line_num        : 11;       // bits : 28_18
		} bit;
		UINT32 word;
	} reg_270; // 0x0438

	union {
		struct {
			unsigned res1_yvd               : 1;        // bits : 0
			unsigned res1_y_block           : 1;        // bits : 1
			unsigned res1_y_line_num        : 11;       // bits : 12_2
			unsigned                        : 3;
			unsigned res1_cvd               : 1;        // bits : 16
			unsigned res1_c_block           : 1;        // bits : 17
			unsigned res1_c_line_num        : 11;       // bits : 28_18
		} bit;
		UINT32 word;
	} reg_271; // 0x043c

	union {
		struct {
			unsigned res2_yvd               : 1;        // bits : 0
			unsigned res2_y_block           : 1;        // bits : 1
			unsigned res2_y_line_num        : 11;       // bits : 12_2
			unsigned                        : 3;
			unsigned res2_cvd               : 1;        // bits : 16
			unsigned res2_c_block           : 1;        // bits : 17
			unsigned res2_c_line_num        : 11;       // bits : 28_18
		} bit;
		UINT32 word;
	} reg_272; // 0x0440

	union {
		struct {
			unsigned res3_yvd               : 1;        // bits : 0
			unsigned res3_y_block           : 1;        // bits : 1
			unsigned res3_y_line_num        : 11;       // bits : 12_2
			unsigned                        : 3;
			unsigned res3_cvd               : 1;        // bits : 16
			unsigned res3_c_block           : 1;        // bits : 17
			unsigned res3_c_line_num        : 11;       // bits : 28_18
		} bit;
		UINT32 word;
	} reg_273; // 0x0444

	union {
		struct {
			unsigned                         : 3;
			unsigned vpe_reserved_201        : 29;      // bits : 31_3
		} bit;
		UINT32 word;
	} reg_274; // 0x0448

	union {
		struct {
			unsigned wbuf_cnt            : 5;       // bits : 4_0
			unsigned                     : 3;
			unsigned des0_frm_end        : 1;       // bits : 8
			unsigned des1_frm_end        : 1;       // bits : 9
			unsigned des2_frm_end        : 1;       // bits : 10
			unsigned des3_frm_end        : 1;       // bits : 11
		} bit;
		UINT32 word;
	} reg_275; // 0x044c

	union {
		struct {
			unsigned miss_cnt        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_276; // 0x0450

	union {
		struct {
			unsigned conflict_cnt        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_277; // 0x0454

	union {
		struct {
			unsigned vc_cs          : 3;        // bits : 2_0
			unsigned                : 5;
			unsigned miss_cs        : 3;        // bits : 10_8
		} bit;
		UINT32 word;
	} reg_278; // 0x0458

	union {
		struct {
			unsigned vpe_reserved_202        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_279; // 0x045c

	union {
		struct {
			unsigned vpe_reserved_203        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_280; // 0x0460

	union {
		struct {
			unsigned vpe_reserved_204        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_281; // 0x0464

	union {
		struct {
			unsigned vpe_reserved_205        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_282; // 0x0468

	union {
		struct {
			unsigned job_empt            : 1;       // bits : 0
			unsigned job_type            : 1;       // bits : 1
			unsigned sd_sys_fsm          : 7;       // bits : 8_2
			unsigned tc_eng_rfy          : 1;       // bits : 9
			unsigned                     : 2;
			unsigned sd0_hsac_fsm        : 5;       // bits : 16_12
			unsigned sd1_hsac_fsm        : 5;       // bits : 21_17
			unsigned sd2_hsac_fsm        : 5;       // bits : 26_22
			unsigned sd3_hsac_fsm        : 5;       // bits : 31_27
		} bit;
		UINT32 word;
	} reg_283; // 0x046c

	union {
		struct {
			unsigned sd_src_frm_scan_fin        : 1;        // bits : 0
			unsigned sm_scan_done               : 1;        // bits : 1
		} bit;
		UINT32 word;
	} reg_284; // 0x0470

	union {
		struct {
			unsigned vpe_reserved_206        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_285; // 0x0474

	union {
		struct {
			unsigned res0_fsm_done_keep        : 1;     // bits : 0
			unsigned res1_fsm_done_keep        : 1;     // bits : 1
			unsigned res2_fsm_done_keep        : 1;     // bits : 2
			unsigned res3_fsm_done_keep        : 1;     // bits : 3
			unsigned tc0_proc_job              : 1;     // bits : 4
			unsigned tc1_proc_job              : 1;     // bits : 5
			unsigned tc2_proc_job              : 1;     // bits : 6
			unsigned tc3_proc_job              : 1;     // bits : 7
			unsigned out0_full                 : 1;     // bits : 8
			unsigned out1_full                 : 1;     // bits : 9
			unsigned out2_full                 : 1;     // bits : 10
			unsigned out3_full                 : 1;     // bits : 11
			unsigned                           : 1;
			unsigned mas_fsm                   : 3;     // bits : 15_13
		} bit;
		UINT32 word;
	} reg_286; // 0x0478

	union {
		struct {
			unsigned ra_done          : 1;      // bits : 0
			unsigned dctg_done        : 1;      // bits : 1
			unsigned l2d_done         : 1;      // bits : 2
			unsigned dp_stall         : 1;      // bits : 3
			unsigned vmr_fsm          : 3;      // bits : 6_4
			unsigned                  : 1;
			unsigned vmr_cnt          : 3;      // bits : 10_8
			unsigned                  : 1;
			unsigned hmr_cnt          : 2;      // bits : 13_12
			unsigned                  : 2;
			unsigned hfrac            : 7;      // bits : 22_16
			unsigned                  : 1;
			unsigned vfrac            : 7;      // bits : 30_24
		} bit;
		UINT32 word;
	} reg_287; // 0x047c

	union {
		struct {
			unsigned                : 8;
			unsigned col_cnt        : 7;        // bits : 14_8
			unsigned                : 1;
			unsigned row_cnt        : 7;        // bits : 22_16
		} bit;
		UINT32 word;
	} reg_288; // 0x0480

	union {
		struct {
			unsigned l2d_done           : 1;        // bits : 0
			unsigned dppm_full          : 1;        // bits : 1
			unsigned dce_fm_done        : 1;        // bits : 2
			unsigned phase              : 1;        // bits : 3
			unsigned scan_done          : 1;        // bits : 4
			unsigned dwc_done           : 1;        // bits : 5
			unsigned                    : 2;
			unsigned dce_fsm            : 3;        // bits : 10_8
			unsigned                    : 5;
			unsigned dst_x              : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_289; // 0x0484

	union {
		struct {
			unsigned dst_y              : 16;       // bits : 15_0
			unsigned cc_rdy             : 1;        // bits : 16
			unsigned reuse              : 1;        // bits : 17
			unsigned hmap_eq            : 1;        // bits : 18
			unsigned                    : 5;
			unsigned buf_idx            : 7;        // bits : 30_24
			unsigned vpos_bit_32        : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_290; // 0x0488

	union {
		struct {
			unsigned vpos        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_291; // 0x048c

	union {
		struct {
			unsigned ll_tab_status00        : 4;        // bits : 3_0
			unsigned ll_tab_status01        : 4;        // bits : 7_4
			unsigned ll_tab_status02        : 4;        // bits : 11_8
			unsigned ll_tab_status03        : 4;        // bits : 15_12
			unsigned ll_tab_status04        : 4;        // bits : 19_16
			unsigned ll_tab_status05        : 4;        // bits : 23_20
			unsigned ll_tab_status06        : 4;        // bits : 27_24
			unsigned ll_tab_status07        : 4;        // bits : 31_28
		} bit;
		UINT32 word;
	} reg_292; // 0x0490

	union {
		struct {
			unsigned ll_tab_status08        : 4;        // bits : 3_0
			unsigned ll_tab_status09        : 4;        // bits : 7_4
			unsigned ll_tab_status10        : 4;        // bits : 11_8
			unsigned ll_tab_status11        : 4;        // bits : 15_12
			unsigned ll_tab_status12        : 4;        // bits : 19_16
			unsigned ll_tab_status13        : 4;        // bits : 23_20
			unsigned ll_tab_status14        : 4;        // bits : 27_24
			unsigned ll_tab_status15        : 4;        // bits : 31_28
		} bit;
		UINT32 word;
	} reg_293; // 0x0494

	union {
		struct {
			unsigned ll_tab_status16        : 4;        // bits : 3_0
			unsigned ll_tab_status17        : 4;        // bits : 7_4
			unsigned ll_tab_status18        : 4;        // bits : 11_8
			unsigned ll_tab_status19        : 4;        // bits : 15_12
			unsigned ll_tab_status20        : 4;        // bits : 19_16
			unsigned ll_tab_status21        : 4;        // bits : 23_20
			unsigned ll_tab_status22        : 4;        // bits : 27_24
			unsigned ll_tab_status23        : 4;        // bits : 31_28
		} bit;
		UINT32 word;
	} reg_294; // 0x0498

	union {
		struct {
			unsigned ll_tab_status24        : 4;        // bits : 3_0
			unsigned ll_tab_status25        : 4;        // bits : 7_4
			unsigned ll_tab_status26        : 4;        // bits : 11_8
			unsigned ll_tab_status27        : 4;        // bits : 15_12
			unsigned ll_tab_status28        : 4;        // bits : 19_16
			unsigned ll_tab_status29        : 4;        // bits : 23_20
			unsigned ll_tab_status30        : 4;        // bits : 27_24
			unsigned ll_tab_status31        : 4;        // bits : 31_28
		} bit;
		UINT32 word;
	} reg_295; // 0x049c

	union {
		struct {
			unsigned ll_tab_status32        : 4;        // bits : 3_0
			unsigned ll_tab_status33        : 4;        // bits : 7_4
			unsigned ll_tab_status34        : 4;        // bits : 11_8
			unsigned ll_tab_status35        : 4;        // bits : 15_12
			unsigned ll_tab_status36        : 4;        // bits : 19_16
			unsigned ll_tab_status37        : 4;        // bits : 23_20
			unsigned ll_tab_status38        : 4;        // bits : 27_24
			unsigned ll_tab_status39        : 4;        // bits : 31_28
		} bit;
		UINT32 word;
	} reg_296; // 0x04a0

	union {
		struct {
			unsigned ll_tab_status40        : 4;        // bits : 3_0
			unsigned ll_tab_status41        : 4;        // bits : 7_4
			unsigned ll_tab_status42        : 4;        // bits : 11_8
			unsigned ll_tab_status43        : 4;        // bits : 15_12
			unsigned ll_tab_status44        : 4;        // bits : 19_16
			unsigned ll_tab_status45        : 4;        // bits : 23_20
			unsigned ll_tab_status46        : 4;        // bits : 27_24
			unsigned ll_tab_status47        : 4;        // bits : 31_28
		} bit;
		UINT32 word;
	} reg_297; // 0x04a4

	union {
		struct {
			unsigned ll_tab_status48        : 4;        // bits : 3_0
			unsigned ll_tab_status49        : 4;        // bits : 7_4
			unsigned ll_tab_status50        : 4;        // bits : 11_8
			unsigned ll_tab_status51        : 4;        // bits : 15_12
			unsigned ll_tab_status52        : 4;        // bits : 19_16
			unsigned ll_tab_status53        : 4;        // bits : 23_20
			unsigned ll_tab_status54        : 4;        // bits : 27_24
			unsigned ll_tab_status55        : 4;        // bits : 31_28
		} bit;
		UINT32 word;
	} reg_298; // 0x04a8

	union {
		struct {
			unsigned ll_tab_status56        : 4;        // bits : 3_0
			unsigned ll_tab_status57        : 4;        // bits : 7_4
			unsigned ll_tab_status58        : 4;        // bits : 11_8
			unsigned ll_tab_status59        : 4;        // bits : 15_12
			unsigned ll_tab_status60        : 4;        // bits : 19_16
			unsigned ll_tab_status61        : 4;        // bits : 23_20
			unsigned ll_tab_status62        : 4;        // bits : 27_24
			unsigned ll_tab_status63        : 4;        // bits : 31_28
		} bit;
		UINT32 word;
	} reg_299; // 0x04ac

	union {
		struct {
			unsigned vpe_reserved_207        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_300; // 0x04b0

	union {
		struct {
			unsigned vpe_reserved_208        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_301; // 0x04b4

	union {
		struct {
			unsigned vpe_reserved_209        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_302; // 0x04b8

	union {
		struct {
			unsigned vpe_reserved_210        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_303; // 0x04bc

	union {
		struct {
			unsigned vpe_reserved_211        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_304; // 0x04c0

	union {
		struct {
			unsigned vpe_reserved_212        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_305; // 0x04c4

	union {
		struct {
			unsigned vpe_reserved_213        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_306; // 0x04c8

	union {
		struct {
			unsigned vpe_reserved_214        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_307; // 0x04cc

	union {
		struct {
			unsigned vpe_reserved_215        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_308; // 0x04d0

	union {
		struct {
			unsigned vpe_reserved_216        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_309; // 0x04d4

	union {
		struct {
			unsigned vpe_reserved_217        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_310; // 0x04d8

	union {
		struct {
			unsigned vpe_reserved_218        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_311; // 0x04dc

	union {
		struct {
			unsigned vpe_reserved_219        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_312; // 0x04e0

	union {
		struct {
			unsigned vpe_reserved_220        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_313; // 0x04e4

	union {
		struct {
			unsigned vpe_reserved_221        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_314; // 0x04e8

	union {
		struct {
			unsigned vpe_reserved_222        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_315; // 0x04ec

	union {
		struct {
			unsigned vpe_reserved_223        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_316; // 0x04f0

	union {
		struct {
			unsigned vpe_reserved_224        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_317; // 0x04f4

	union {
		struct {
			unsigned vpe_reserved_225        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_318; // 0x04f8

	union {
		struct {
			unsigned vpe_reserved_226        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_319; // 0x04fc

	union {
		struct {
			unsigned res0_sca_en               : 1;     // bits : 0
			unsigned res0_sca_bypass_en        : 1;     // bits : 1
			unsigned res0_sca_crop_en          : 1;     // bits : 2
			unsigned res0_tc_en                : 1;     // bits : 3
			unsigned res0_sca_uv_sel           : 1;     // bits : 4
			unsigned                           : 3;
			unsigned res0_des_drt              : 2;     // bits : 9_8
			unsigned                           : 1;
			unsigned res0_des_yuv420           : 1;     // bits : 11
			unsigned                           : 3;
			unsigned res0_des_uv_swap          : 1;     // bits : 15
			unsigned res0_out_bg_sel           : 3;     // bits : 18_16
		} bit;
		UINT32 word;
	} reg_320; // 0x0500

	union {
		struct {
			unsigned res0_des_dp0_ycc_enc_en        : 1;        // bits : 0
			unsigned                                : 3;
			unsigned res0_des_dp0_chrw              : 1;        // bits : 4
			unsigned                                : 3;
			unsigned res0_des_dp0_format            : 2;        // bits : 9_8
		} bit;
		UINT32 word;
	} reg_321; // 0x0504

	union {
		struct {
			unsigned res0_sca_height        : 13;       // bits : 12_0
		} bit;
		UINT32 word;
	} reg_322; // 0x0508

	union {
		struct {
			unsigned res0_des_buf_width         : 14;       // bits : 13_0
			unsigned                            : 2;
			unsigned res0_des_buf_height        : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_323; // 0x050c

	union {
		struct {
			unsigned res0_out_y_start        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned res0_out_height         : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_324; // 0x0510

	union {
		struct {
			unsigned res0_rlt_y_start        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned res0_rlt_height         : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_325; // 0x0514

	union {
		struct {
			unsigned res0_pip_y_start        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned res0_pip_height         : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_326; // 0x0518

	union {
		struct {
			unsigned res0_sca_crop_y_start        : 12;     // bits : 11_0
			unsigned                              : 4;
			unsigned res0_sca_crop_height         : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_327; // 0x051c

	union {
		struct {
			unsigned res0_sca_y_luma_algo_en          : 2;      // bits : 1_0
			unsigned res0_sca_x_luma_algo_en          : 2;      // bits : 3_2
			unsigned res0_sca_y_chroma_algo_en        : 2;      // bits : 5_4
			unsigned res0_sca_x_chroma_algo_en        : 2;      // bits : 7_6
			unsigned res0_sca_map_sel                 : 1;      // bits : 8
		} bit;
		UINT32 word;
	} reg_328; // 0x0520

	union {
		struct {
			unsigned res0_sca_hstep        : 14;        // bits : 13_0
			unsigned                       : 2;
			unsigned res0_sca_vstep        : 14;        // bits : 29_16
		} bit;
		UINT32 word;
	} reg_329; // 0x0524

	union {
		struct {
			unsigned res0_sca_hsca_divisor        : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned res0_sca_havg_pxl_msk        : 8;      // bits : 23_16
		} bit;
		UINT32 word;
	} reg_330; // 0x0528

	union {
		struct {
			unsigned res0_sca_vsca_divisor        : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned res0_sca_vavg_pxl_msk        : 8;      // bits : 23_16
		} bit;
		UINT32 word;
	} reg_331; // 0x052c

	union {
		struct {
			unsigned res0_sca_coef_h0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned res0_sca_coef_h1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_332; // 0x0530

	union {
		struct {
			unsigned res0_sca_coef_h2        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned res0_sca_coef_h3        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_333; // 0x0534

	union {
		struct {
			unsigned res0_sca_coef_v0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned res0_sca_coef_v1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_334; // 0x0538

	union {
		struct {
			unsigned res0_sca_coef_v2        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned res0_sca_coef_v3        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_335; // 0x053c

	union {
		struct {
			unsigned res0_tc_crop_y_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res0_tc_crop_height         : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_336; // 0x0540

	union {
		struct {
			unsigned vpe_reserved_227        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_337; // 0x0544

	union {
		struct {
			unsigned vpe_reserved_228        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_338; // 0x0548

	union {
		struct {
			unsigned vpe_reserved_229        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_339; // 0x054c

	union {
		struct {
			unsigned vpe_reserved_230        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_340; // 0x0550

	union {
		struct {
			unsigned vpe_reserved_231        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_341; // 0x0554

	union {
		struct {
			unsigned vpe_reserved_232        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_342; // 0x0558

	union {
		struct {
			unsigned vpe_reserved_233        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_343; // 0x055c

	union {
		struct {
			unsigned vpe_reserved_234        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_344; // 0x0560

	union {
		struct {
			unsigned vpe_reserved_235        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_345; // 0x0564

	union {
		struct {
			unsigned vpe_reserved_236        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_346; // 0x0568

	union {
		struct {
			unsigned vpe_reserved_237        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_347; // 0x056c

	union {
		struct {
			unsigned vpe_reserved_238        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_348; // 0x0570

	union {
		struct {
			unsigned vpe_reserved_239        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_349; // 0x0574

	union {
		struct {
			unsigned vpe_reserved_240        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_350; // 0x0578

	union {
		struct {
			unsigned vpe_reserved_241        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_351; // 0x057c

	union {
		struct {
			unsigned res1_sca_en               : 1;     // bits : 0
			unsigned res1_sca_bypass_en        : 1;     // bits : 1
			unsigned res1_sca_crop_en          : 1;     // bits : 2
			unsigned res1_tc_en                : 1;     // bits : 3
			unsigned res1_sca_uv_sel           : 1;     // bits : 4
			unsigned                           : 3;
			unsigned res1_des_drt              : 2;     // bits : 9_8
			unsigned                           : 1;
			unsigned res1_des_yuv420           : 1;     // bits : 11
			unsigned                           : 3;
			unsigned res1_des_uv_swap          : 1;     // bits : 15
			unsigned res1_out_bg_sel           : 3;     // bits : 18_16
		} bit;
		UINT32 word;
	} reg_352; // 0x0580

	union {
		struct {
			unsigned res1_des_dp0_ycc_enc_en        : 1;        // bits : 0
			unsigned                                : 3;
			unsigned res1_des_dp0_chrw              : 1;        // bits : 4
			unsigned                                : 3;
			unsigned res1_des_dp0_format            : 2;        // bits : 9_8
		} bit;
		UINT32 word;
	} reg_353; // 0x0584

	union {
		struct {
			unsigned res1_sca_height        : 13;       // bits : 12_0
		} bit;
		UINT32 word;
	} reg_354; // 0x0588

	union {
		struct {
			unsigned res1_des_buf_width         : 14;       // bits : 13_0
			unsigned                            : 2;
			unsigned res1_des_buf_height        : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_355; // 0x058c

	union {
		struct {
			unsigned res1_out_y_start        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned res1_out_height         : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_356; // 0x0590

	union {
		struct {
			unsigned res1_rlt_y_start        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned res1_rlt_height         : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_357; // 0x0594

	union {
		struct {
			unsigned res1_pip_y_start        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned res1_pip_height         : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_358; // 0x0598

	union {
		struct {
			unsigned res1_sca_crop_y_start        : 12;     // bits : 11_0
			unsigned                              : 4;
			unsigned res1_sca_crop_height         : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_359; // 0x059c

	union {
		struct {
			unsigned res1_sca_y_luma_algo_en          : 2;      // bits : 1_0
			unsigned res1_sca_x_luma_algo_en          : 2;      // bits : 3_2
			unsigned res1_sca_y_chroma_algo_en        : 2;      // bits : 5_4
			unsigned res1_sca_x_chroma_algo_en        : 2;      // bits : 7_6
			unsigned res1_sca_map_sel                 : 1;      // bits : 8
		} bit;
		UINT32 word;
	} reg_360; // 0x05a0

	union {
		struct {
			unsigned res1_sca_hstep        : 14;        // bits : 13_0
			unsigned                       : 2;
			unsigned res1_sca_vstep        : 14;        // bits : 29_16
		} bit;
		UINT32 word;
	} reg_361; // 0x05a4

	union {
		struct {
			unsigned res1_sca_hsca_divisor        : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned res1_sca_havg_pxl_msk        : 8;      // bits : 23_16
		} bit;
		UINT32 word;
	} reg_362; // 0x05a8

	union {
		struct {
			unsigned res1_sca_vsca_divisor        : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned res1_sca_vavg_pxl_msk        : 8;      // bits : 23_16
		} bit;
		UINT32 word;
	} reg_363; // 0x05ac

	union {
		struct {
			unsigned res1_sca_coef_h0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned res1_sca_coef_h1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_364; // 0x05b0

	union {
		struct {
			unsigned res1_sca_coef_h2        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned res1_sca_coef_h3        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_365; // 0x05b4

	union {
		struct {
			unsigned res1_sca_coef_v0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned res1_sca_coef_v1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_366; // 0x05b8

	union {
		struct {
			unsigned res1_sca_coef_v2        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned res1_sca_coef_v3        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_367; // 0x05bc

	union {
		struct {
			unsigned res1_tc_crop_y_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res1_tc_crop_height         : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_368; // 0x05c0

	union {
		struct {
			unsigned vpe_reserved_242        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_369; // 0x05c4

	union {
		struct {
			unsigned vpe_reserved_243        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_370; // 0x05c8

	union {
		struct {
			unsigned vpe_reserved_244        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_371; // 0x05cc

	union {
		struct {
			unsigned vpe_reserved_245        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_372; // 0x05d0

	union {
		struct {
			unsigned vpe_reserved_246        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_373; // 0x05d4

	union {
		struct {
			unsigned vpe_reserved_247        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_374; // 0x05d8

	union {
		struct {
			unsigned vpe_reserved_248        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_375; // 0x05dc

	union {
		struct {
			unsigned vpe_reserved_249        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_376; // 0x05e0

	union {
		struct {
			unsigned vpe_reserved_250        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_377; // 0x05e4

	union {
		struct {
			unsigned vpe_reserved_251        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_378; // 0x05e8

	union {
		struct {
			unsigned vpe_reserved_252        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_379; // 0x05ec

	union {
		struct {
			unsigned vpe_reserved_253        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_380; // 0x05f0

	union {
		struct {
			unsigned vpe_reserved_254        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_381; // 0x05f4

	union {
		struct {
			unsigned vpe_reserved_255        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_382; // 0x05f8

	union {
		struct {
			unsigned vpe_reserved_256        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_383; // 0x05fc

	union {
		struct {
			unsigned rs0_cl0_sca_width            : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned rs0_cl0_sca_hstep_oft        : 15;     // bits : 30_16
		} bit;
		UINT32 word;
	} reg_384; // 0x0600

	union {
		struct {
			unsigned rs0_cl0_out_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl0_out_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_385; // 0x0604

	union {
		struct {
			unsigned rs0_cl0_rlt_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl0_rlt_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_386; // 0x0608

	union {
		struct {
			unsigned rs0_cl0_pip_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl0_pip_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_387; // 0x060c

	union {
		struct {
			unsigned rs0_cl0_sca_crop_x_start        : 12;      // bits : 11_0
			unsigned                                 : 4;
			unsigned rs0_cl0_sca_crop_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_388; // 0x0610

	union {
		struct {
			unsigned rs0_cl0_tc_crop_x_start        : 13;       // bits : 12_0
			unsigned                                : 3;
			unsigned rs0_cl0_tc_crop_width          : 13;       // bits : 28_16
			unsigned                                : 2;
			unsigned rs0_cl0_tc_crop_skip           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_389; // 0x0614

	union {
		struct {
			unsigned vpe_reserved_257        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_390; // 0x0618

	union {
		struct {
			unsigned vpe_reserved_258        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_391; // 0x061c

	union {
		struct {
			unsigned rs0_cl1_sca_width            : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned rs0_cl1_sca_hstep_oft        : 15;     // bits : 30_16
		} bit;
		UINT32 word;
	} reg_392; // 0x0620

	union {
		struct {
			unsigned rs0_cl1_out_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl1_out_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_393; // 0x0624

	union {
		struct {
			unsigned rs0_cl1_rlt_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl1_rlt_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_394; // 0x0628

	union {
		struct {
			unsigned rs0_cl1_pip_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl1_pip_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_395; // 0x062c

	union {
		struct {
			unsigned rs0_cl1_sca_crop_x_start        : 12;      // bits : 11_0
			unsigned                                 : 4;
			unsigned rs0_cl1_sca_crop_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_396; // 0x0630

	union {
		struct {
			unsigned rs0_cl1_tc_crop_x_start        : 13;       // bits : 12_0
			unsigned                                : 3;
			unsigned rs0_cl1_tc_crop_width          : 13;       // bits : 28_16
			unsigned                                : 2;
			unsigned rs0_cl1_tc_crop_skip           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_397; // 0x0634

	union {
		struct {
			unsigned vpe_reserved_259        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_398; // 0x0638

	union {
		struct {
			unsigned vpe_reserved_260        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_399; // 0x063c

	union {
		struct {
			unsigned rs0_cl2_sca_width            : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned rs0_cl2_sca_hstep_oft        : 15;     // bits : 30_16
		} bit;
		UINT32 word;
	} reg_400; // 0x0640

	union {
		struct {
			unsigned rs0_cl2_out_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl2_out_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_401; // 0x0644

	union {
		struct {
			unsigned rs0_cl2_rlt_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl2_rlt_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_402; // 0x0648

	union {
		struct {
			unsigned rs0_cl2_pip_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl2_pip_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_403; // 0x064c

	union {
		struct {
			unsigned rs0_cl2_sca_crop_x_start        : 12;      // bits : 11_0
			unsigned                                 : 4;
			unsigned rs0_cl2_sca_crop_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_404; // 0x0650

	union {
		struct {
			unsigned rs0_cl2_tc_crop_x_start        : 13;       // bits : 12_0
			unsigned                                : 3;
			unsigned rs0_cl2_tc_crop_width          : 13;       // bits : 28_16
			unsigned                                : 2;
			unsigned rs0_cl2_tc_crop_skip           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_405; // 0x0654

	union {
		struct {
			unsigned vpe_reserved_261        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_406; // 0x0658

	union {
		struct {
			unsigned vpe_reserved_262        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_407; // 0x065c

	union {
		struct {
			unsigned rs0_cl3_sca_width            : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned rs0_cl3_sca_hstep_oft        : 15;     // bits : 30_16
		} bit;
		UINT32 word;
	} reg_408; // 0x0660

	union {
		struct {
			unsigned rs0_cl3_out_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl3_out_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_409; // 0x0664

	union {
		struct {
			unsigned rs0_cl3_rlt_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl3_rlt_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_410; // 0x0668

	union {
		struct {
			unsigned rs0_cl3_pip_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl3_pip_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_411; // 0x066c

	union {
		struct {
			unsigned rs0_cl3_sca_crop_x_start        : 12;      // bits : 11_0
			unsigned                                 : 4;
			unsigned rs0_cl3_sca_crop_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_412; // 0x0670

	union {
		struct {
			unsigned rs0_cl3_tc_crop_x_start        : 13;       // bits : 12_0
			unsigned                                : 3;
			unsigned rs0_cl3_tc_crop_width          : 13;       // bits : 28_16
			unsigned                                : 2;
			unsigned rs0_cl3_tc_crop_skip           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_413; // 0x0674

	union {
		struct {
			unsigned vpe_reserved_263        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_414; // 0x0678

	union {
		struct {
			unsigned vpe_reserved_264        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_415; // 0x067c

	union {
		struct {
			unsigned rs0_cl4_sca_width            : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned rs0_cl4_sca_hstep_oft        : 15;     // bits : 30_16
		} bit;
		UINT32 word;
	} reg_416; // 0x0680

	union {
		struct {
			unsigned rs0_cl4_out_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl4_out_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_417; // 0x0684

	union {
		struct {
			unsigned rs0_cl4_rlt_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl4_rlt_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_418; // 0x0688

	union {
		struct {
			unsigned rs0_cl4_pip_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl4_pip_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_419; // 0x068c

	union {
		struct {
			unsigned rs0_cl4_sca_crop_x_start        : 12;      // bits : 11_0
			unsigned                                 : 4;
			unsigned rs0_cl4_sca_crop_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_420; // 0x0690

	union {
		struct {
			unsigned rs0_cl4_tc_crop_x_start        : 13;       // bits : 12_0
			unsigned                                : 3;
			unsigned rs0_cl4_tc_crop_width          : 13;       // bits : 28_16
			unsigned                                : 2;
			unsigned rs0_cl4_tc_crop_skip           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_421; // 0x0694

	union {
		struct {
			unsigned vpe_reserved_265        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_422; // 0x0698

	union {
		struct {
			unsigned vpe_reserved_266        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_423; // 0x069c

	union {
		struct {
			unsigned rs0_cl5_sca_width            : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned rs0_cl5_sca_hstep_oft        : 15;     // bits : 30_16
		} bit;
		UINT32 word;
	} reg_424; // 0x06a0

	union {
		struct {
			unsigned rs0_cl5_out_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl5_out_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_425; // 0x06a4

	union {
		struct {
			unsigned rs0_cl5_rlt_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl5_rlt_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_426; // 0x06a8

	union {
		struct {
			unsigned rs0_cl5_pip_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl5_pip_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_427; // 0x06ac

	union {
		struct {
			unsigned rs0_cl5_sca_crop_x_start        : 12;      // bits : 11_0
			unsigned                                 : 4;
			unsigned rs0_cl5_sca_crop_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_428; // 0x06b0

	union {
		struct {
			unsigned rs0_cl5_tc_crop_x_start        : 13;       // bits : 12_0
			unsigned                                : 3;
			unsigned rs0_cl5_tc_crop_width          : 13;       // bits : 28_16
			unsigned                                : 2;
			unsigned rs0_cl5_tc_crop_skip           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_429; // 0x06b4

	union {
		struct {
			unsigned vpe_reserved_267        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_430; // 0x06b8

	union {
		struct {
			unsigned vpe_reserved_268        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_431; // 0x06bc

	union {
		struct {
			unsigned rs0_cl6_sca_width            : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned rs0_cl6_sca_hstep_oft        : 15;     // bits : 30_16
		} bit;
		UINT32 word;
	} reg_432; // 0x06c0

	union {
		struct {
			unsigned rs0_cl6_out_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl6_out_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_433; // 0x06c4

	union {
		struct {
			unsigned rs0_cl6_rlt_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl6_rlt_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_434; // 0x06c8

	union {
		struct {
			unsigned rs0_cl6_pip_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl6_pip_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_435; // 0x06cc

	union {
		struct {
			unsigned rs0_cl6_sca_crop_x_start        : 12;      // bits : 11_0
			unsigned                                 : 4;
			unsigned rs0_cl6_sca_crop_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_436; // 0x06d0

	union {
		struct {
			unsigned rs0_cl6_tc_crop_x_start        : 13;       // bits : 12_0
			unsigned                                : 3;
			unsigned rs0_cl6_tc_crop_width          : 13;       // bits : 28_16
			unsigned                                : 2;
			unsigned rs0_cl6_tc_crop_skip           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_437; // 0x06d4

	union {
		struct {
			unsigned vpe_reserved_269        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_438; // 0x06d8

	union {
		struct {
			unsigned vpe_reserved_270        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_439; // 0x06dc

	union {
		struct {
			unsigned rs0_cl7_sca_width            : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned rs0_cl7_sca_hstep_oft        : 15;     // bits : 30_16
		} bit;
		UINT32 word;
	} reg_440; // 0x06e0

	union {
		struct {
			unsigned rs0_cl7_out_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl7_out_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_441; // 0x06e4

	union {
		struct {
			unsigned rs0_cl7_rlt_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl7_rlt_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_442; // 0x06e8

	union {
		struct {
			unsigned rs0_cl7_pip_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs0_cl7_pip_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_443; // 0x06ec

	union {
		struct {
			unsigned rs0_cl7_sca_crop_x_start        : 12;      // bits : 11_0
			unsigned                                 : 4;
			unsigned rs0_cl7_sca_crop_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_444; // 0x06f0

	union {
		struct {
			unsigned rs0_cl7_tc_crop_x_start        : 13;       // bits : 12_0
			unsigned                                : 3;
			unsigned rs0_cl7_tc_crop_width          : 13;       // bits : 28_16
			unsigned                                : 2;
			unsigned rs0_cl7_tc_crop_skip           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_445; // 0x06f4

	union {
		struct {
			unsigned vpe_reserved_271        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_446; // 0x06f8

	union {
		struct {
			unsigned vpe_reserved_272        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_447; // 0x06fc

	union {
		struct {
			unsigned rs1_cl0_sca_width            : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned rs1_cl0_sca_hstep_oft        : 15;     // bits : 30_16
		} bit;
		UINT32 word;
	} reg_448; // 0x0700

	union {
		struct {
			unsigned rs1_cl0_out_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl0_out_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_449; // 0x0704

	union {
		struct {
			unsigned rs1_cl0_rlt_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl0_rlt_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_450; // 0x0708

	union {
		struct {
			unsigned rs1_cl0_pip_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl0_pip_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_451; // 0x070c

	union {
		struct {
			unsigned rs1_cl0_sca_crop_x_start        : 12;      // bits : 11_0
			unsigned                                 : 4;
			unsigned rs1_cl0_sca_crop_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_452; // 0x0710

	union {
		struct {
			unsigned rs1_cl0_tc_crop_x_start        : 13;       // bits : 12_0
			unsigned                                : 3;
			unsigned rs1_cl0_tc_crop_width          : 13;       // bits : 28_16
			unsigned                                : 2;
			unsigned rs1_cl0_tc_crop_skip           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_453; // 0x0714

	union {
		struct {
			unsigned vpe_reserved_273        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_454; // 0x0718

	union {
		struct {
			unsigned vpe_reserved_274        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_455; // 0x071c

	union {
		struct {
			unsigned rs1_cl1_sca_width            : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned rs1_cl1_sca_hstep_oft        : 15;     // bits : 30_16
		} bit;
		UINT32 word;
	} reg_456; // 0x0720

	union {
		struct {
			unsigned rs1_cl1_out_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl1_out_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_457; // 0x0724

	union {
		struct {
			unsigned rs1_cl1_rlt_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl1_rlt_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_458; // 0x0728

	union {
		struct {
			unsigned rs1_cl1_pip_x_start              : 13;     // bits : 12_0
			unsigned                                  : 3;
			unsigned rs1_cl1_rs1_cl1_pip_width        : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_459; // 0x072c

	union {
		struct {
			unsigned rs1_cl1_sca_crop_x_start        : 12;      // bits : 11_0
			unsigned                                 : 4;
			unsigned rs1_cl1_sca_crop_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_460; // 0x0730

	union {
		struct {
			unsigned rs1_cl1_tc_crop_x_start        : 13;       // bits : 12_0
			unsigned                                : 3;
			unsigned rs1_cl1_tc_crop_width          : 13;       // bits : 28_16
			unsigned                                : 2;
			unsigned rs1_cl1_tc_crop_skip           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_461; // 0x0734

	union {
		struct {
			unsigned vpe_reserved_275        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_462; // 0x0738

	union {
		struct {
			unsigned vpe_reserved_276        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_463; // 0x073c

	union {
		struct {
			unsigned rs1_cl2_sca_width            : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned rs1_cl2_sca_hstep_oft        : 15;     // bits : 30_16
		} bit;
		UINT32 word;
	} reg_464; // 0x0740

	union {
		struct {
			unsigned rs1_cl2_out_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl2_out_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_465; // 0x0744

	union {
		struct {
			unsigned rs1_cl2_rlt_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl2_rlt_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_466; // 0x0748

	union {
		struct {
			unsigned rs1_cl2_pip_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl2_pip_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_467; // 0x074c

	union {
		struct {
			unsigned rs1_cl2_sca_crop_x_start        : 12;      // bits : 11_0
			unsigned                                 : 4;
			unsigned rs1_cl2_sca_crop_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_468; // 0x0750

	union {
		struct {
			unsigned rs1_cl2_tc_crop_x_start        : 13;       // bits : 12_0
			unsigned                                : 3;
			unsigned rs1_cl2_tc_crop_width          : 13;       // bits : 28_16
			unsigned                                : 2;
			unsigned rs1_cl2_tc_crop_skip           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_469; // 0x0754

	union {
		struct {
			unsigned vpe_reserved_277        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_470; // 0x0758

	union {
		struct {
			unsigned vpe_reserved_278        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_471; // 0x075c

	union {
		struct {
			unsigned rs1_cl3_sca_width            : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned rs1_cl3_sca_hstep_oft        : 15;     // bits : 30_16
		} bit;
		UINT32 word;
	} reg_472; // 0x0760

	union {
		struct {
			unsigned rs1_cl3_out_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl3_out_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_473; // 0x0764

	union {
		struct {
			unsigned rs1_cl3_rlt_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl3_rlt_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_474; // 0x0768

	union {
		struct {
			unsigned rs1_cl3_pip_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl3_pip_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_475; // 0x076c

	union {
		struct {
			unsigned rs1_cl3_sca_crop_x_start        : 12;      // bits : 11_0
			unsigned                                 : 4;
			unsigned rs1_cl3_sca_crop_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_476; // 0x0770

	union {
		struct {
			unsigned rs1_cl3_tc_crop_x_start        : 13;       // bits : 12_0
			unsigned                                : 3;
			unsigned rs1_cl3_tc_crop_width          : 13;       // bits : 28_16
			unsigned                                : 2;
			unsigned rs1_cl3_tc_crop_skip           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_477; // 0x0774

	union {
		struct {
			unsigned vpe_reserved_279        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_478; // 0x0778

	union {
		struct {
			unsigned vpe_reserved_280        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_479; // 0x077c

	union {
		struct {
			unsigned rs1_cl4_sca_width            : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned rs1_cl4_sca_hstep_oft        : 15;     // bits : 30_16
		} bit;
		UINT32 word;
	} reg_480; // 0x0780

	union {
		struct {
			unsigned rs1_cl4_out_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl4_out_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_481; // 0x0784

	union {
		struct {
			unsigned rs1_cl4_rlt_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl4_rlt_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_482; // 0x0788

	union {
		struct {
			unsigned rs1_cl4_pip_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl4_pip_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_483; // 0x078c

	union {
		struct {
			unsigned rs1_cl4_sca_crop_x_start        : 12;      // bits : 11_0
			unsigned                                 : 4;
			unsigned rs1_cl4_sca_crop_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_484; // 0x0790

	union {
		struct {
			unsigned rs1_cl4_tc_crop_x_start        : 13;       // bits : 12_0
			unsigned                                : 3;
			unsigned rs1_cl4_tc_crop_width          : 13;       // bits : 28_16
			unsigned                                : 2;
			unsigned rs1_cl4_tc_crop_skip           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_485; // 0x0794

	union {
		struct {
			unsigned vpe_reserved_281        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_486; // 0x0798

	union {
		struct {
			unsigned vpe_reserved_282        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_487; // 0x079c

	union {
		struct {
			unsigned rs1_cl5_sca_width            : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned rs1_cl5_sca_hstep_oft        : 15;     // bits : 30_16
		} bit;
		UINT32 word;
	} reg_488; // 0x07a0

	union {
		struct {
			unsigned rs1_cl5_out_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl5_out_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_489; // 0x07a4

	union {
		struct {
			unsigned rs1_cl5_rlt_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl5_rlt_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_490; // 0x07a8

	union {
		struct {
			unsigned rs1_cl5_pip_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl5_pip_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_491; // 0x07ac

	union {
		struct {
			unsigned rs1_cl5_sca_crop_x_start        : 12;      // bits : 11_0
			unsigned                                 : 4;
			unsigned rs1_cl5_sca_crop_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_492; // 0x07b0

	union {
		struct {
			unsigned rs1_cl5_tc_crop_x_start        : 13;       // bits : 12_0
			unsigned                                : 3;
			unsigned rs1_cl5_tc_crop_width          : 13;       // bits : 28_16
			unsigned                                : 2;
			unsigned rs1_cl5_tc_crop_skip           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_493; // 0x07b4

	union {
		struct {
			unsigned vpe_reserved_283        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_494; // 0x07b8

	union {
		struct {
			unsigned vpe_reserved_284        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_495; // 0x07bc

	union {
		struct {
			unsigned rs1_cl6_sca_width            : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned rs1_cl6_sca_hstep_oft        : 15;     // bits : 30_16
		} bit;
		UINT32 word;
	} reg_496; // 0x07c0

	union {
		struct {
			unsigned rs1_cl6_out_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl6_out_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_497; // 0x07c4

	union {
		struct {
			unsigned rs1_cl6_rlt_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl6_rlt_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_498; // 0x07c8

	union {
		struct {
			unsigned rs1_cl6_pip_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl6_pip_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_499; // 0x07cc

	union {
		struct {
			unsigned rs1_cl6_sca_crop_x_start        : 12;      // bits : 11_0
			unsigned                                 : 4;
			unsigned rs1_cl6_sca_crop_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_500; // 0x07d0

	union {
		struct {
			unsigned rs1_cl6_tc_crop_x_start        : 13;       // bits : 12_0
			unsigned                                : 3;
			unsigned rs1_cl6_tc_crop_width          : 13;       // bits : 28_16
			unsigned                                : 2;
			unsigned rs1_cl6_tc_crop_skip           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_501; // 0x07d4

	union {
		struct {
			unsigned vpe_reserved_285        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_502; // 0x07d8

	union {
		struct {
			unsigned vpe_reserved_286        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_503; // 0x07dc

	union {
		struct {
			unsigned rs1_cl7_sca_width            : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned rs1_cl7_sca_hstep_oft        : 15;     // bits : 30_16
		} bit;
		UINT32 word;
	} reg_504; // 0x07e0

	union {
		struct {
			unsigned rs1_cl7_out_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl7_out_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_505; // 0x07e4

	union {
		struct {
			unsigned rs1_cl7_rlt_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl7_rlt_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_506; // 0x07e8

	union {
		struct {
			unsigned rs1_cl7_pip_x_start        : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned rs1_cl7_pip_width          : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_507; // 0x07ec

	union {
		struct {
			unsigned rs1_cl7_sca_crop_x_start        : 12;      // bits : 11_0
			unsigned                                 : 4;
			unsigned rs1_cl7_sca_crop_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_508; // 0x07f0

	union {
		struct {
			unsigned rs1_cl7_tc_crop_x_start        : 13;       // bits : 12_0
			unsigned                                : 3;
			unsigned rs1_cl7_tc_crop_width          : 13;       // bits : 28_16
			unsigned                                : 2;
			unsigned rs1_cl7_tc_crop_skip           : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_509; // 0x07f4

	union {
		struct {
			unsigned vpe_reserved_287        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_510; // 0x07f8

	union {
		struct {
			unsigned vpe_reserved_288        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_511; // 0x07fc

	union {
		struct {
			unsigned cl0_proc_width          : 12;      // bits : 11_0
			unsigned                         : 4;
			unsigned cl0_proc_x_start        : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_512; // 0x0800

	union {
		struct {
			unsigned vpe_reserved_289        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_513; // 0x0804

	union {
		struct {
			unsigned vpe_reserved_290        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_514; // 0x0808

	union {
		struct {
			unsigned vpe_reserved_291        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_515; // 0x080c

	union {
		struct {
			unsigned vpe_reserved_292        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_516; // 0x0810

	union {
		struct {
			unsigned vpe_reserved_293        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_517; // 0x0814

	union {
		struct {
			unsigned cl0_col_x_start        : 13;       // bits : 12_0
		} bit;
		UINT32 word;
	} reg_518; // 0x0818

	union {
		struct {
			unsigned vpe_reserved_294        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_519; // 0x081c

	union {
		struct {
			unsigned cl1_proc_width          : 12;      // bits : 11_0
			unsigned                         : 4;
			unsigned cl1_proc_x_start        : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_520; // 0x0820

	union {
		struct {
			unsigned vpe_reserved_295        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_521; // 0x0824

	union {
		struct {
			unsigned vpe_reserved_296        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_522; // 0x0828

	union {
		struct {
			unsigned vpe_reserved_297        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_523; // 0x082c

	union {
		struct {
			unsigned vpe_reserved_298        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_524; // 0x0830

	union {
		struct {
			unsigned vpe_reserved_299        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_525; // 0x0834

	union {
		struct {
			unsigned cl1_col_x_start        : 13;       // bits : 12_0
		} bit;
		UINT32 word;
	} reg_526; // 0x0838

	union {
		struct {
			unsigned vpe_reserved_300        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_527; // 0x083c

	union {
		struct {
			unsigned cl2_proc_width          : 12;      // bits : 11_0
			unsigned                         : 4;
			unsigned cl2_proc_x_start        : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_528; // 0x0840

	union {
		struct {
			unsigned vpe_reserved_301        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_529; // 0x0844

	union {
		struct {
			unsigned vpe_reserved_302        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_530; // 0x0848

	union {
		struct {
			unsigned vpe_reserved_303        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_531; // 0x084c

	union {
		struct {
			unsigned vpe_reserved_304        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_532; // 0x0850

	union {
		struct {
			unsigned vpe_reserved_305        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_533; // 0x0854

	union {
		struct {
			unsigned cl2_col_x_start        : 13;       // bits : 12_0
		} bit;
		UINT32 word;
	} reg_534; // 0x0858

	union {
		struct {
			unsigned vpe_reserved_307        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_535; // 0x085c

	union {
		struct {
			unsigned cl3_proc_width          : 12;      // bits : 11_0
			unsigned                         : 4;
			unsigned cl3_proc_x_start        : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_536; // 0x0860

	union {
		struct {
			unsigned vpe_reserved_308        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_537; // 0x0864

	union {
		struct {
			unsigned vpe_reserved_309        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_538; // 0x0868

	union {
		struct {
			unsigned vpe_reserved_310        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_539; // 0x086c

	union {
		struct {
			unsigned vpe_reserved_311        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_540; // 0x0870

	union {
		struct {
			unsigned vpe_reserved_312        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_541; // 0x0874

	union {
		struct {
			unsigned cl3_col_x_start        : 13;       // bits : 12_0
		} bit;
		UINT32 word;
	} reg_542; // 0x0878

	union {
		struct {
			unsigned vpe_reserved_313        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_543; // 0x087c

	union {
		struct {
			unsigned cl4_proc_width          : 12;      // bits : 11_0
			unsigned                         : 4;
			unsigned cl4_proc_x_start        : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_544; // 0x0880

	union {
		struct {
			unsigned vpe_reserved_314        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_545; // 0x0884

	union {
		struct {
			unsigned vpe_reserved_315        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_546; // 0x0888

	union {
		struct {
			unsigned vpe_reserved_316        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_547; // 0x088c

	union {
		struct {
			unsigned vpe_reserved_317        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_548; // 0x0890

	union {
		struct {
			unsigned vpe_reserved_318        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_549; // 0x0894

	union {
		struct {
			unsigned cl4_col_x_start        : 13;       // bits : 12_0
		} bit;
		UINT32 word;
	} reg_550; // 0x0898

	union {
		struct {
			unsigned vpe_reserved_319        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_551; // 0x089c

	union {
		struct {
			unsigned cl5_proc_width          : 12;      // bits : 11_0
			unsigned                         : 4;
			unsigned cl5_proc_x_start        : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_552; // 0x08a0

	union {
		struct {
			unsigned vpe_reserved_320        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_553; // 0x08a4

	union {
		struct {
			unsigned vpe_reserved_321        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_554; // 0x08a8

	union {
		struct {
			unsigned vpe_reserved_322        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_555; // 0x08ac

	union {
		struct {
			unsigned vpe_reserved_323        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_556; // 0x08b0

	union {
		struct {
			unsigned vpe_reserved_324        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_557; // 0x08b4

	union {
		struct {
			unsigned cl5_col_x_start        : 13;       // bits : 12_0
		} bit;
		UINT32 word;
	} reg_558; // 0x08b8

	union {
		struct {
			unsigned vpe_reserved_325        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_559; // 0x08bc

	union {
		struct {
			unsigned cl6_proc_width          : 12;      // bits : 11_0
			unsigned                         : 4;
			unsigned cl6_proc_x_start        : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_560; // 0x08c0

	union {
		struct {
			unsigned vpe_reserved_326        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_561; // 0x08c4

	union {
		struct {
			unsigned vpe_reserved_327        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_562; // 0x08c8

	union {
		struct {
			unsigned vpe_reserved_328        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_563; // 0x08cc

	union {
		struct {
			unsigned vpe_reserved_329        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_564; // 0x08d0

	union {
		struct {
			unsigned vpe_reserved_330        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_565; // 0x08d4

	union {
		struct {
			unsigned cl6_col_x_start        : 13;       // bits : 12_0
		} bit;
		UINT32 word;
	} reg_566; // 0x08d8

	union {
		struct {
			unsigned vpe_reserved_331        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_567; // 0x08dc

	union {
		struct {
			unsigned cl7_proc_width          : 12;      // bits : 11_0
			unsigned                         : 4;
			unsigned cl7_proc_x_start        : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_568; // 0x08e0

	union {
		struct {
			unsigned vpe_reserved_332        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_569; // 0x08e4

	union {
		struct {
			unsigned vpe_reserved_333        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_570; // 0x08e8

	union {
		struct {
			unsigned vpe_reserved_334        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_571; // 0x08ec

	union {
		struct {
			unsigned vpe_reserved_335        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_572; // 0x08f0

	union {
		struct {
			unsigned vpe_reserved_336        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_573; // 0x08f4

	union {
		struct {
			unsigned cl7_col_x_start        : 13;       // bits : 12_0
		} bit;
		UINT32 word;
	} reg_574; // 0x08f8

	union {
		struct {
			unsigned vpe_reserved_337        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_575; // 0x08fc

	union {
		struct {
			unsigned geo_lut_0        : 16;     // bits : 15_0
			unsigned geo_lut_1        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_576; // 0x0900

	union {
		struct {
			unsigned geo_lut_2        : 16;     // bits : 15_0
			unsigned geo_lut_3        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_577; // 0x0904

	union {
		struct {
			unsigned geo_lut_4        : 16;     // bits : 15_0
			unsigned geo_lut_5        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_578; // 0x0908

	union {
		struct {
			unsigned geo_lut_6        : 16;     // bits : 15_0
			unsigned geo_lut_7        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_579; // 0x090c

	union {
		struct {
			unsigned geo_lut_8        : 16;     // bits : 15_0
			unsigned geo_lut_9        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_580; // 0x0910

	union {
		struct {
			unsigned geo_lut_10        : 16;        // bits : 15_0
			unsigned geo_lut_11        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_581; // 0x0914

	union {
		struct {
			unsigned geo_lut_12        : 16;        // bits : 15_0
			unsigned geo_lut_13        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_582; // 0x0918

	union {
		struct {
			unsigned geo_lut_14        : 16;        // bits : 15_0
			unsigned geo_lut_15        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_583; // 0x091c

	union {
		struct {
			unsigned geo_lut_16        : 16;        // bits : 15_0
			unsigned geo_lut_17        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_584; // 0x0920

	union {
		struct {
			unsigned geo_lut_18        : 16;        // bits : 15_0
			unsigned geo_lut_19        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_585; // 0x0924

	union {
		struct {
			unsigned geo_lut_20        : 16;        // bits : 15_0
			unsigned geo_lut_21        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_586; // 0x0928

	union {
		struct {
			unsigned geo_lut_22        : 16;        // bits : 15_0
			unsigned geo_lut_23        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_587; // 0x092c

	union {
		struct {
			unsigned geo_lut_24        : 16;        // bits : 15_0
			unsigned geo_lut_25        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_588; // 0x0930

	union {
		struct {
			unsigned geo_lut_26        : 16;        // bits : 15_0
			unsigned geo_lut_27        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_589; // 0x0934

	union {
		struct {
			unsigned geo_lut_28        : 16;        // bits : 15_0
			unsigned geo_lut_29        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_590; // 0x0938

	union {
		struct {
			unsigned geo_lut_30        : 16;        // bits : 15_0
			unsigned geo_lut_31        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_591; // 0x093c

	union {
		struct {
			unsigned geo_lut_32        : 16;        // bits : 15_0
			unsigned geo_lut_33        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_592; // 0x0940

	union {
		struct {
			unsigned geo_lut_34        : 16;        // bits : 15_0
			unsigned geo_lut_35        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_593; // 0x0944

	union {
		struct {
			unsigned geo_lut_36        : 16;        // bits : 15_0
			unsigned geo_lut_37        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_594; // 0x0948

	union {
		struct {
			unsigned geo_lut_38        : 16;        // bits : 15_0
			unsigned geo_lut_39        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_595; // 0x094c

	union {
		struct {
			unsigned geo_lut_40        : 16;        // bits : 15_0
			unsigned geo_lut_41        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_596; // 0x0950

	union {
		struct {
			unsigned geo_lut_42        : 16;        // bits : 15_0
			unsigned geo_lut_43        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_597; // 0x0954

	union {
		struct {
			unsigned geo_lut_44        : 16;        // bits : 15_0
			unsigned geo_lut_45        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_598; // 0x0958

	union {
		struct {
			unsigned geo_lut_46        : 16;        // bits : 15_0
			unsigned geo_lut_47        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_599; // 0x095c

	union {
		struct {
			unsigned geo_lut_48        : 16;        // bits : 15_0
			unsigned geo_lut_49        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_600; // 0x0960

	union {
		struct {
			unsigned geo_lut_50        : 16;        // bits : 15_0
			unsigned geo_lut_51        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_601; // 0x0964

	union {
		struct {
			unsigned geo_lut_52        : 16;        // bits : 15_0
			unsigned geo_lut_53        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_602; // 0x0968

	union {
		struct {
			unsigned geo_lut_54        : 16;        // bits : 15_0
			unsigned geo_lut_55        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_603; // 0x096c

	union {
		struct {
			unsigned geo_lut_56        : 16;        // bits : 15_0
			unsigned geo_lut_57        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_604; // 0x0970

	union {
		struct {
			unsigned geo_lut_58        : 16;        // bits : 15_0
			unsigned geo_lut_59        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_605; // 0x0974

	union {
		struct {
			unsigned geo_lut_60        : 16;        // bits : 15_0
			unsigned geo_lut_61        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_606; // 0x0978

	union {
		struct {
			unsigned geo_lut_62        : 16;        // bits : 15_0
			unsigned geo_lut_63        : 16;        // bits : 31_16
		} bit;
		UINT32 word;
	} reg_607; // 0x097c

	union {
		struct {
			unsigned geo_lut_64           : 16;      // bits : 15_0
			unsigned         : 16;
		} bit;
		UINT32 word;
	} reg_608; // 0x0980

	union {
		struct {
			unsigned vpe_reserved_338        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_609; // 0x0984

	union {
		struct {
			unsigned vpe_reserved_339        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_610; // 0x0988

	union {
		struct {
			unsigned vpe_reserved_340        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_611; // 0x098c

	union {
		struct {
			unsigned vpe_reserved_341        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_612; // 0x0990

	union {
		struct {
			unsigned vpe_reserved_342        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_613; // 0x0994

	union {
		struct {
			unsigned vpe_reserved_343        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_614; // 0x0998

	union {
		struct {
			unsigned vpe_reserved_344        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_615; // 0x099c

	union {
		struct {
			unsigned vpe_reserved_345        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_616; // 0x09a0

	union {
		struct {
			unsigned vpe_reserved_346        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_617; // 0x09a4

	union {
		struct {
			unsigned vpe_reserved_347        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_618; // 0x09a8

	union {
		struct {
			unsigned vpe_reserved_348        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_619; // 0x09ac

	union {
		struct {
			unsigned vpe_reserved_349        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_620; // 0x09b0

	union {
		struct {
			unsigned vpe_reserved_350        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_621; // 0x09b4

	union {
		struct {
			unsigned vpe_reserved_351        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_622; // 0x09b8

	union {
		struct {
			unsigned vpe_reserved_352        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_623; // 0x09bc

	union {
		struct {
			unsigned vpe_reserved_353        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_624; // 0x09c0

	union {
		struct {
			unsigned vpe_reserved_354        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_625; // 0x09c4

	union {
		struct {
			unsigned vpe_reserved_355        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_626; // 0x09c8

	union {
		struct {
			unsigned vpe_reserved_356        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_627; // 0x09cc

	union {
		struct {
			unsigned vpe_reserved_357        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_628; // 0x09d0

	union {
		struct {
			unsigned vpe_reserved_358        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_629; // 0x09d4

	union {
		struct {
			unsigned vpe_reserved_359        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_630; // 0x09d8

	union {
		struct {
			unsigned vpe_reserved_360        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_631; // 0x09dc

	union {
		struct {
			unsigned vpe_reserved_361        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_632; // 0x09e0

	union {
		struct {
			unsigned vpe_reserved_362        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_633; // 0x09e4

	union {
		struct {
			unsigned vpe_reserved_363        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_634; // 0x09e8

	union {
		struct {
			unsigned vpe_reserved_364        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_635; // 0x09ec

	union {
		struct {
			unsigned vpe_reserved_365        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_636; // 0x09f0

	union {
		struct {
			unsigned vpe_reserved_366        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_637; // 0x09f4

	union {
		struct {
			unsigned vpe_reserved_367        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_638; // 0x09f8

	union {
		struct {
			unsigned vpe_reserved_368        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_639; // 0x09fc

	union {
		struct {
			unsigned vpe_reserved_369        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_640; // 0x0a00

	union {
		struct {
			unsigned vpe_reserved_370        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_641; // 0x0a04

	union {
		struct {
			unsigned vpe_reserved_371        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_642; // 0x0a08

	union {
		struct {
			unsigned vpe_reserved_372        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_643; // 0x0a0c

	union {
		struct {
			unsigned vpe_reserved_373        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_644; // 0x0a10

	union {
		struct {
			unsigned vpe_reserved_374        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_645; // 0x0a14

	union {
		struct {
			unsigned vpe_reserved_375        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_646; // 0x0a18

	union {
		struct {
			unsigned vpe_reserved_376        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_647; // 0x0a1c

	union {
		struct {
			unsigned vpe_reserved_377        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_648; // 0x0a20

	union {
		struct {
			unsigned vpe_reserved_378        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_649; // 0x0a24

	union {
		struct {
			unsigned vpe_reserved_379        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_650; // 0x0a28

	union {
		struct {
			unsigned vpe_reserved_380        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_651; // 0x0a2c

	union {
		struct {
			unsigned vpe_reserved_381        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_652; // 0x0a30

	union {
		struct {
			unsigned vpe_reserved_382        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_653; // 0x0a34

	union {
		struct {
			unsigned vpe_reserved_383        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_654; // 0x0a38

	union {
		struct {
			unsigned vpe_reserved_384        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_655; // 0x0a3c

	union {
		struct {
			unsigned vpe_reserved_385        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_656; // 0x0a40

	union {
		struct {
			unsigned vpe_reserved_386        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_657; // 0x0a44

	union {
		struct {
			unsigned vpe_reserved_387        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_658; // 0x0a48

	union {
		struct {
			unsigned vpe_reserved_388        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_659; // 0x0a4c

	union {
		struct {
			unsigned vpe_reserved_389        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_660; // 0x0a50

	union {
		struct {
			unsigned vpe_reserved_390        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_661; // 0x0a54

	union {
		struct {
			unsigned vpe_reserved_391        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_662; // 0x0a58

	union {
		struct {
			unsigned vpe_reserved_392        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_663; // 0x0a5c

	union {
		struct {
			unsigned vpe_reserved_393        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_664; // 0x0a60

	union {
		struct {
			unsigned vpe_reserved_394        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_665; // 0x0a64

	union {
		struct {
			unsigned vpe_reserved_395        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_666; // 0x0a68

	union {
		struct {
			unsigned vpe_reserved_396        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_667; // 0x0a6c

	union {
		struct {
			unsigned vpe_reserved_397        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_668; // 0x0a70

	union {
		struct {
			unsigned vpe_reserved_398        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_669; // 0x0a74

	union {
		struct {
			unsigned vpe_reserved_399        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_670; // 0x0a78

	union {
		struct {
			unsigned vpe_reserved_400        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_671; // 0x0a7c

	union {
		struct {
			unsigned vpe_reserved_401        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_672; // 0x0a80

	union {
		struct {
			unsigned vpe_reserved_402        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_673; // 0x0a84

	union {
		struct {
			unsigned vpe_reserved_403        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_674; // 0x0a88

	union {
		struct {
			unsigned vpe_reserved_404        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_675; // 0x0a8c

	union {
		struct {
			unsigned vpe_reserved_405        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_676; // 0x0a90

	union {
		struct {
			unsigned vpe_reserved_406        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_677; // 0x0a94

	union {
		struct {
			unsigned vpe_reserved_407        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_678; // 0x0a98

	union {
		struct {
			unsigned vpe_reserved_408        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_679; // 0x0a9c

	union {
		struct {
			unsigned vpe_reserved_409        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_680; // 0x0aa0

	union {
		struct {
			unsigned vpe_reserved_410        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_681; // 0x0aa4

	union {
		struct {
			unsigned vpe_reserved_411        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_682; // 0x0aa8

	union {
		struct {
			unsigned vpe_reserved_412        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_683; // 0x0aac

	union {
		struct {
			unsigned vpe_reserved_413        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_684; // 0x0ab0

	union {
		struct {
			unsigned vpe_reserved_414        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_685; // 0x0ab4

	union {
		struct {
			unsigned vpe_reserved_415        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_686; // 0x0ab8

	union {
		struct {
			unsigned vpe_reserved_416        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_687; // 0x0abc

	union {
		struct {
			unsigned vpe_reserved_417        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_688; // 0x0ac0

	union {
		struct {
			unsigned vpe_reserved_418        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_689; // 0x0ac4

	union {
		struct {
			unsigned vpe_reserved_419        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_690; // 0x0ac8

	union {
		struct {
			unsigned vpe_reserved_420        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_691; // 0x0acc

	union {
		struct {
			unsigned vpe_reserved_421        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_692; // 0x0ad0

	union {
		struct {
			unsigned vpe_reserved_422        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_693; // 0x0ad4

	union {
		struct {
			unsigned vpe_reserved_423        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_694; // 0x0ad8

	union {
		struct {
			unsigned vpe_reserved_424        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_695; // 0x0adc

	union {
		struct {
			unsigned vpe_reserved_425        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_696; // 0x0ae0

	union {
		struct {
			unsigned vpe_reserved_426        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_697; // 0x0ae4

	union {
		struct {
			unsigned vpe_reserved_427        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_698; // 0x0ae8

	union {
		struct {
			unsigned vpe_reserved_428        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_699; // 0x0aec

	union {
		struct {
			unsigned vpe_reserved_429        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_700; // 0x0af0

	union {
		struct {
			unsigned vpe_reserved_430        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_701; // 0x0af4

	union {
		struct {
			unsigned vpe_reserved_431        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_702; // 0x0af8

	union {
		struct {
			unsigned vpe_reserved_432        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_703; // 0x0afc

	union {
		struct {
			unsigned vpe_reserved_433        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_704; // 0x0b00

	union {
		struct {
			unsigned vpe_reserved_434        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_705; // 0x0b04

	union {
		struct {
			unsigned vpe_reserved_435        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_706; // 0x0b08

	union {
		struct {
			unsigned vpe_reserved_436        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_707; // 0x0b0c

	union {
		struct {
			unsigned vpe_reserved_437        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_708; // 0x0b10

	union {
		struct {
			unsigned vpe_reserved_438        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_709; // 0x0b14

	union {
		struct {
			unsigned vpe_reserved_439        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_710; // 0x0b18

	union {
		struct {
			unsigned vpe_reserved_440        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_711; // 0x0b1c

	union {
		struct {
			unsigned vpe_reserved_441        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_712; // 0x0b20

	union {
		struct {
			unsigned vpe_reserved_442        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_713; // 0x0b24

	union {
		struct {
			unsigned vpe_reserved_443        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_714; // 0x0b28

	union {
		struct {
			unsigned vpe_reserved_444        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_715; // 0x0b2c

	union {
		struct {
			unsigned vpe_reserved_445        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_716; // 0x0b30

	union {
		struct {
			unsigned vpe_reserved_446        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_717; // 0x0b34

	union {
		struct {
			unsigned vpe_reserved_447        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_718; // 0x0b38

	union {
		struct {
			unsigned vpe_reserved_448        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_719; // 0x0b3c

	union {
		struct {
			unsigned vpe_reserved_449        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_720; // 0x0b40

	union {
		struct {
			unsigned vpe_reserved_450        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_721; // 0x0b44

	union {
		struct {
			unsigned vpe_reserved_451        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_722; // 0x0b48

	union {
		struct {
			unsigned vpe_reserved_452        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_723; // 0x0b4c

	union {
		struct {
			unsigned vpe_reserved_453        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_724; // 0x0b50

	union {
		struct {
			unsigned vpe_reserved_454        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_725; // 0x0b54

	union {
		struct {
			unsigned vpe_reserved_455        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_726; // 0x0b58

	union {
		struct {
			unsigned vpe_reserved_456        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_727; // 0x0b5c

	union {
		struct {
			unsigned vpe_reserved_457        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_728; // 0x0b60

	union {
		struct {
			unsigned vpe_reserved_458        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_729; // 0x0b64

	union {
		struct {
			unsigned vpe_reserved_459        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_730; // 0x0b68

	union {
		struct {
			unsigned vpe_reserved_460        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_731; // 0x0b6c

	union {
		struct {
			unsigned vpe_reserved_461        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_732; // 0x0b70

	union {
		struct {
			unsigned vpe_reserved_462        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_733; // 0x0b74

	union {
		struct {
			unsigned vpe_reserved_463        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_734; // 0x0b78

	union {
		struct {
			unsigned vpe_reserved_464        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_735; // 0x0b7c

	union {
		struct {
			unsigned vpe_reserved_465        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_736; // 0x0b80

	union {
		struct {
			unsigned vpe_reserved_466        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_737; // 0x0b84

	union {
		struct {
			unsigned vpe_reserved_467        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_738; // 0x0b88

	union {
		struct {
			unsigned vpe_reserved_468        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_739; // 0x0b8c

	union {
		struct {
			unsigned vpe_reserved_469        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_740; // 0x0b90

	union {
		struct {
			unsigned vpe_reserved_470        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_741; // 0x0b94

	union {
		struct {
			unsigned vpe_reserved_471        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_742; // 0x0b98

	union {
		struct {
			unsigned vpe_reserved_472        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_743; // 0x0b9c

	union {
		struct {
			unsigned vpe_reserved_473        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_744; // 0x0ba0

	union {
		struct {
			unsigned vpe_reserved_474        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_745; // 0x0ba4

	union {
		struct {
			unsigned vpe_reserved_475        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_746; // 0x0ba8

	union {
		struct {
			unsigned vpe_reserved_476        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_747; // 0x0bac

	union {
		struct {
			unsigned vpe_reserved_477        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_748; // 0x0bb0

	union {
		struct {
			unsigned vpe_reserved_478        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_749; // 0x0bb4

	union {
		struct {
			unsigned vpe_reserved_479        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_750; // 0x0bb8

	union {
		struct {
			unsigned vpe_reserved_480        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_751; // 0x0bbc

	union {
		struct {
			unsigned vpe_reserved_481        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_752; // 0x0bc0

	union {
		struct {
			unsigned vpe_reserved_482        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_753; // 0x0bc4

	union {
		struct {
			unsigned vpe_reserved_483        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_754; // 0x0bc8

	union {
		struct {
			unsigned vpe_reserved_484        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_755; // 0x0bcc

	union {
		struct {
			unsigned vpe_reserved_485        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_756; // 0x0bd0

	union {
		struct {
			unsigned vpe_reserved_486        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_757; // 0x0bd4

	union {
		struct {
			unsigned vpe_reserved_487        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_758; // 0x0bd8

	union {
		struct {
			unsigned vpe_reserved_488        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_759; // 0x0bdc

	union {
		struct {
			unsigned vpe_reserved_489        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_760; // 0x0be0

	union {
		struct {
			unsigned vpe_reserved_490        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_761; // 0x0be4

	union {
		struct {
			unsigned vpe_reserved_491        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_762; // 0x0be8

	union {
		struct {
			unsigned vpe_reserved_492        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_763; // 0x0bec

	union {
		struct {
			unsigned vpe_reserved_493        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_764; // 0x0bf0

	union {
		struct {
			unsigned vpe_reserved_494        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_765; // 0x0bf4

	union {
		struct {
			unsigned vpe_reserved_495        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_766; // 0x0bf8

	union {
		struct {
			unsigned vpe_reserved_496        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_767; // 0x0bfc

	union {
		struct {
			unsigned res2_sca_en                : 1;        // bits : 0
			unsigned res2_sca_bypass_en         : 1;        // bits : 1
			unsigned res2_sca_crop_en           : 1;        // bits : 2
			unsigned res2_tc_en                 : 1;        // bits : 3
			unsigned res2_sca_uv_sel        : 1;        // bits : 4
			unsigned                            : 3;
			unsigned res2_des_drt               : 2;        // bits : 9_8
			unsigned                            : 1;
			unsigned res2_des_yuv420            : 1;        // bits : 11
			unsigned                            : 3;
			unsigned res2_des_uv_swap           : 1;        // bits : 15
			unsigned res2_out_bg_sel            : 3;        // bits : 18_16
		} bit;
		UINT32 word;
	} reg_768; // 0x0c00

	union {
		struct {
			unsigned                            : 4;
			unsigned res2_des_dp0_chrw          : 1;        // bits : 4
			unsigned                            : 3;
			unsigned res2_des_dp0_format        : 2;        // bits : 9_8
		} bit;
		UINT32 word;
	} reg_769; // 0x0c04

	union {
		struct {
			unsigned res2_sca_height        : 13;       // bits : 12_0
		} bit;
		UINT32 word;
	} reg_770; // 0x0c08

	union {
		struct {
			unsigned res2_des_buf_width         : 14;       // bits : 13_0
			unsigned                            : 2;
			unsigned res2_des_buf_height        : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_771; // 0x0c0c

	union {
		struct {
			unsigned res2_out_y_start        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned res2_out_height         : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_772; // 0x0c10

	union {
		struct {
			unsigned res2_rlt_y_start        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned res2_rlt_height         : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_773; // 0x0c14

	union {
		struct {
			unsigned res2_pip_y_start        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned res2_pip_height         : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_774; // 0x0c18

	union {
		struct {
			unsigned res2_sca_crop_y_start        : 12;     // bits : 11_0
			unsigned                              : 4;
			unsigned res2_sca_crop_height         : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_775; // 0x0c1c

	union {
		struct {
			unsigned res2_sca_y_luma_algo_en          : 2;      // bits : 1_0
			unsigned res2_sca_x_luma_algo_en          : 2;      // bits : 3_2
			unsigned res2_sca_y_chroma_algo_en        : 2;      // bits : 5_4
			unsigned res2_sca_x_chroma_algo_en        : 2;      // bits : 7_6
			unsigned res2_sca_map_sel                 : 1;      // bits : 8
		} bit;
		UINT32 word;
	} reg_776; // 0x0c20

	union {
		struct {
			unsigned res2_sca_hstep        : 14;        // bits : 13_0
			unsigned                       : 2;
			unsigned res2_sca_vstep        : 14;        // bits : 29_16
		} bit;
		UINT32 word;
	} reg_777; // 0x0c24

	union {
		struct {
			unsigned res2_sca_hsca_divisor        : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned res2_sca_havg_pxl_msk        : 8;      // bits : 23_16
		} bit;
		UINT32 word;
	} reg_778; // 0x0c28

	union {
		struct {
			unsigned res2_sca_vsca_divisor        : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned res2_sca_vavg_pxl_msk        : 8;      // bits : 23_16
		} bit;
		UINT32 word;
	} reg_779; // 0x0c2c

	union {
		struct {
			unsigned res2_sca_coef_h0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned res2_sca_coef_h1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_780; // 0x0c30

	union {
		struct {
			unsigned res2_sca_coef_h2        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned res2_sca_coef_h3        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_781; // 0x0c34

	union {
		struct {
			unsigned res2_sca_coef_v0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned res2_sca_coef_v1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_782; // 0x0c38

	union {
		struct {
			unsigned res2_sca_coef_v2        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned res2_sca_coef_v3        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_783; // 0x0c3c

	union {
		struct {
			unsigned res2_tc_crop_y_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_tc_crop_height         : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_784; // 0x0c40

	union {
		struct {
			unsigned vpe_reserved_497        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_785; // 0x0c44

	union {
		struct {
			unsigned vpe_reserved_498        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_786; // 0x0c48

	union {
		struct {
			unsigned vpe_reserved_499        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_787; // 0x0c4c

	union {
		struct {
			unsigned vpe_reserved_500        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_788; // 0x0c50

	union {
		struct {
			unsigned vpe_reserved_501        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_789; // 0x0c54

	union {
		struct {
			unsigned vpe_reserved_502        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_790; // 0x0c58

	union {
		struct {
			unsigned vpe_reserved_503        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_791; // 0x0c5c

	union {
		struct {
			unsigned vpe_reserved_504        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_792; // 0x0c60

	union {
		struct {
			unsigned vpe_reserved_505        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_793; // 0x0c64

	union {
		struct {
			unsigned vpe_reserved_506        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_794; // 0x0c68

	union {
		struct {
			unsigned vpe_reserved_507        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_795; // 0x0c6c

	union {
		struct {
			unsigned vpe_reserved_508        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_796; // 0x0c70

	union {
		struct {
			unsigned vpe_reserved_509        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_797; // 0x0c74

	union {
		struct {
			unsigned vpe_reserved_510        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_798; // 0x0c78

	union {
		struct {
			unsigned vpe_reserved_511        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_799; // 0x0c7c

	union {
		struct {
			unsigned res3_sca_en                : 1;        // bits : 0
			unsigned res3_sca_bypass_en         : 1;        // bits : 1
			unsigned res3_sca_crop_en           : 1;        // bits : 2
			unsigned res3_tc_en                 : 1;        // bits : 3
			unsigned res3_sca_uv_sel            : 1;        // bits : 4
			unsigned                            : 3;
			unsigned res3_des_drt               : 2;        // bits : 9_8
			unsigned                            : 1;
			unsigned res3_des_yuv420            : 1;        // bits : 11
			unsigned                            : 3;
			unsigned res3_des_uv_swap           : 1;        // bits : 15
			unsigned res3_out_bg_sel            : 3;        // bits : 18_16
		} bit;
		UINT32 word;
	} reg_800; // 0x0c80

	union {
		struct {
			unsigned                            : 4;
			unsigned res3_des_dp0_chrw          : 1;        // bits : 4
			unsigned                            : 3;
			unsigned res3_des_dp0_format        : 2;        // bits : 9_8
		} bit;
		UINT32 word;
	} reg_801; // 0x0c84

	union {
		struct {
			unsigned res3_sca_height        : 13;       // bits : 12_0
		} bit;
		UINT32 word;
	} reg_802; // 0x0c88

	union {
		struct {
			unsigned res3_des_buf_width         : 14;       // bits : 13_0
			unsigned                            : 2;
			unsigned res3_des_buf_height        : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_803; // 0x0c8c

	union {
		struct {
			unsigned res3_out_y_start        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned res3_out_height         : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_804; // 0x0c90

	union {
		struct {
			unsigned res3_rlt_y_start        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned res3_rlt_height         : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_805; // 0x0c94

	union {
		struct {
			unsigned res3_pip_y_start        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned res3_pip_height         : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_806; // 0x0c98

	union {
		struct {
			unsigned res3_sca_crop_y_start        : 12;     // bits : 11_0
			unsigned                              : 4;
			unsigned res3_sca_crop_height         : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_807; // 0x0c9c

	union {
		struct {
			unsigned res3_sca_y_luma_algo_en          : 2;      // bits : 1_0
			unsigned res3_sca_x_luma_algo_en          : 2;      // bits : 3_2
			unsigned res3_sca_y_chroma_algo_en        : 2;      // bits : 5_4
			unsigned res3_sca_x_chroma_algo_en        : 2;      // bits : 7_6
			unsigned res3_sca_map_sel                 : 1;      // bits : 8
		} bit;
		UINT32 word;
	} reg_808; // 0x0ca0

	union {
		struct {
			unsigned res3_sca_hstep        : 14;        // bits : 13_0
			unsigned                       : 2;
			unsigned res3_sca_vstep        : 14;        // bits : 29_16
		} bit;
		UINT32 word;
	} reg_809; // 0x0ca4

	union {
		struct {
			unsigned res3_sca_hsca_divisor        : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned res3_sca_havg_pxl_msk        : 8;      // bits : 23_16
		} bit;
		UINT32 word;
	} reg_810; // 0x0ca8

	union {
		struct {
			unsigned res3_sca_vsca_divisor        : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned res3_sca_vavg_pxl_msk        : 8;      // bits : 23_16
		} bit;
		UINT32 word;
	} reg_811; // 0x0cac

	union {
		struct {
			unsigned res3_sca_coef_h0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned res3_sca_coef_h1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_812; // 0x0cb0

	union {
		struct {
			unsigned res3_sca_coef_h2        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned res3_sca_coef_h3        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_813; // 0x0cb4

	union {
		struct {
			unsigned res3_sca_coef_v0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned res3_sca_coef_v1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_814; // 0x0cb8

	union {
		struct {
			unsigned res3_sca_coef_v2        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned res3_sca_coef_v3        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_815; // 0x0cbc

	union {
		struct {
			unsigned res3_tc_crop_y_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_tc_crop_height         : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_816; // 0x0cc0

	union {
		struct {
			unsigned vpe_reserved_512        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_817; // 0x0cc4

	union {
		struct {
			unsigned vpe_reserved_513        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_818; // 0x0cc8

	union {
		struct {
			unsigned vpe_reserved_514        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_819; // 0x0ccc

	union {
		struct {
			unsigned vpe_reserved_515        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_820; // 0x0cd0

	union {
		struct {
			unsigned vpe_reserved_516        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_821; // 0x0cd4

	union {
		struct {
			unsigned vpe_reserved_517        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_822; // 0x0cd8

	union {
		struct {
			unsigned vpe_reserved_518        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_823; // 0x0cdc

	union {
		struct {
			unsigned vpe_reserved_519        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_824; // 0x0ce0

	union {
		struct {
			unsigned vpe_reserved_520        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_825; // 0x0ce4

	union {
		struct {
			unsigned vpe_reserved_521        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_826; // 0x0ce8

	union {
		struct {
			unsigned vpe_reserved_522        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_827; // 0x0cec

	union {
		struct {
			unsigned vpe_reserved_523        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_828; // 0x0cf0

	union {
		struct {
			unsigned vpe_reserved_524        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_829; // 0x0cf4

	union {
		struct {
			unsigned vpe_reserved_525        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_830; // 0x0cf8

	union {
		struct {
			unsigned vpe_reserved_526        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_831; // 0x0cfc

	union {
		struct {
			unsigned res2_cl0_sca_width            : 13;        // bits : 12_0
			unsigned                               : 3;
			unsigned res2_cl0_sca_hstep_oft        : 15;        // bits : 30_16
		} bit;
		UINT32 word;
	} reg_832; // 0x0d00

	union {
		struct {
			unsigned res2_cl0_out_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl0_out_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_833; // 0x0d04

	union {
		struct {
			unsigned res2_cl0_rlt_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl0_rlt_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_834; // 0x0d08

	union {
		struct {
			unsigned res2_cl0_pip_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl0_pip_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_835; // 0x0d0c

	union {
		struct {
			unsigned res2_cl0_sca_crop_x_start        : 12;     // bits : 11_0
			unsigned                                  : 4;
			unsigned res2_cl0_sca_crop_width          : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_836; // 0x0d10

	union {
		struct {
			unsigned res2_cl0_tc_crop_x_start        : 13;      // bits : 12_0
			unsigned                                 : 3;
			unsigned res2_cl0_tc_crop_width          : 13;      // bits : 28_16
			unsigned                                 : 2;
			unsigned res2_cl0_tc_crop_skip           : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_837; // 0x0d14

	union {
		struct {
			unsigned vpe_reserved_527        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_838; // 0x0d18

	union {
		struct {
			unsigned vpe_reserved_528        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_839; // 0x0d1c

	union {
		struct {
			unsigned res2_cl1_sca_width            : 13;        // bits : 12_0
			unsigned                               : 3;
			unsigned res2_cl1_sca_hstep_oft        : 15;        // bits : 30_16
		} bit;
		UINT32 word;
	} reg_840; // 0x0d20

	union {
		struct {
			unsigned res2_cl1_out_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl1_out_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_841; // 0x0d24

	union {
		struct {
			unsigned res2_cl1_rlt_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl1_rlt_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_842; // 0x0d28

	union {
		struct {
			unsigned res2_cl1_pip_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl1_pip_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_843; // 0x0d2c

	union {
		struct {
			unsigned res2_cl1_sca_crop_x_start        : 12;     // bits : 11_0
			unsigned                                  : 4;
			unsigned res2_cl1_sca_crop_width          : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_844; // 0x0d30

	union {
		struct {
			unsigned res2_cl1_tc_crop_x_start        : 13;      // bits : 12_0
			unsigned                                 : 3;
			unsigned res2_cl1_tc_crop_width          : 13;      // bits : 28_16
			unsigned                                 : 2;
			unsigned res2_cl1_tc_crop_skip           : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_845; // 0x0d34

	union {
		struct {
			unsigned vpe_reserved_529        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_846; // 0x0d38

	union {
		struct {
			unsigned vpe_reserved_530        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_847; // 0x0d3c

	union {
		struct {
			unsigned res2_cl2_sca_width            : 13;        // bits : 12_0
			unsigned                               : 3;
			unsigned res2_cl2_sca_hstep_oft        : 15;        // bits : 30_16
		} bit;
		UINT32 word;
	} reg_848; // 0x0d40

	union {
		struct {
			unsigned res2_cl2_out_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl2_out_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_849; // 0x0d44

	union {
		struct {
			unsigned res2_cl2_rlt_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl2_rlt_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_850; // 0x0d48

	union {
		struct {
			unsigned res2_cl2_pip_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl2_pip_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_851; // 0x0d4c

	union {
		struct {
			unsigned res2_cl2_sca_crop_x_start        : 12;     // bits : 11_0
			unsigned                                  : 4;
			unsigned res2_cl2_sca_crop_width          : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_852; // 0x0d50

	union {
		struct {
			unsigned res2_cl2_tc_crop_x_start        : 13;      // bits : 12_0
			unsigned                                 : 3;
			unsigned res2_cl2_tc_crop_width          : 13;      // bits : 28_16
			unsigned                                 : 2;
			unsigned res2_cl2_tc_crop_skip           : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_853; // 0x0d54

	union {
		struct {
			unsigned vpe_reserved_531        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_854; // 0x0d58

	union {
		struct {
			unsigned vpe_reserved_532        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_855; // 0x0d5c

	union {
		struct {
			unsigned res2_cl3_sca_width            : 13;        // bits : 12_0
			unsigned                               : 3;
			unsigned res2_cl3_sca_hstep_oft        : 15;        // bits : 30_16
		} bit;
		UINT32 word;
	} reg_856; // 0x0d60

	union {
		struct {
			unsigned res2_cl3_out_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl3_out_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_857; // 0x0d64

	union {
		struct {
			unsigned res2_cl3_rlt_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl3_rlt_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_858; // 0x0d68

	union {
		struct {
			unsigned res2_cl3_pip_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl3_pip_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_859; // 0x0d6c

	union {
		struct {
			unsigned res2_cl3_sca_crop_x_start        : 12;     // bits : 11_0
			unsigned                                  : 4;
			unsigned res2_cl3_sca_crop_width          : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_860; // 0x0d70

	union {
		struct {
			unsigned res2_cl3_tc_crop_x_start        : 13;      // bits : 12_0
			unsigned                                 : 3;
			unsigned res2_cl3_tc_crop_width          : 13;      // bits : 28_16
			unsigned                                 : 2;
			unsigned res2_cl3_tc_crop_skip           : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_861; // 0x0d74

	union {
		struct {
			unsigned vpe_reserved_533        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_862; // 0x0d78

	union {
		struct {
			unsigned vpe_reserved_534        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_863; // 0x0d7c

	union {
		struct {
			unsigned res2_cl4_sca_width            : 13;        // bits : 12_0
			unsigned                               : 3;
			unsigned res2_cl4_sca_hstep_oft        : 15;        // bits : 30_16
		} bit;
		UINT32 word;
	} reg_864; // 0x0d80

	union {
		struct {
			unsigned res2_cl4_out_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl4_out_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_865; // 0x0d84

	union {
		struct {
			unsigned res2_cl4_rlt_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl4_rlt_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_866; // 0x0d88

	union {
		struct {
			unsigned res2_cl4_pip_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl4_pip_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_867; // 0x0d8c

	union {
		struct {
			unsigned res2_cl4_sca_crop_x_start        : 12;     // bits : 11_0
			unsigned                                  : 4;
			unsigned res2_cl4_sca_crop_width          : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_868; // 0x0d90

	union {
		struct {
			unsigned res2_cl4_tc_crop_x_start        : 13;      // bits : 12_0
			unsigned                                 : 3;
			unsigned res2_cl4_tc_crop_width          : 13;      // bits : 28_16
			unsigned                                 : 2;
			unsigned res2_cl4_tc_crop_skip           : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_869; // 0x0d94

	union {
		struct {
			unsigned vpe_reserved_535        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_870; // 0x0d98

	union {
		struct {
			unsigned vpe_reserved_536        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_871; // 0x0d9c

	union {
		struct {
			unsigned res2_cl5_sca_width            : 13;        // bits : 12_0
			unsigned                               : 3;
			unsigned res2_cl5_sca_hstep_oft        : 15;        // bits : 30_16
		} bit;
		UINT32 word;
	} reg_872; // 0x0da0

	union {
		struct {
			unsigned res2_cl5_out_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl5_out_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_873; // 0x0da4

	union {
		struct {
			unsigned res2_cl5_rlt_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl5_rlt_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_874; // 0x0da8

	union {
		struct {
			unsigned res2_cl5_pip_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl5_pip_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_875; // 0x0dac

	union {
		struct {
			unsigned res2_cl5_sca_crop_x_start        : 12;     // bits : 11_0
			unsigned                                  : 4;
			unsigned res2_cl5_sca_crop_width          : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_876; // 0x0db0

	union {
		struct {
			unsigned res2_cl5_tc_crop_x_start        : 13;      // bits : 12_0
			unsigned                                 : 3;
			unsigned res2_cl5_tc_crop_width          : 13;      // bits : 28_16
			unsigned                                 : 2;
			unsigned tc_crop_skip                    : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_877; // 0x0db4

	union {
		struct {
			unsigned vpe_reserved_537        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_878; // 0x0db8

	union {
		struct {
			unsigned vpe_reserved_538        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_879; // 0x0dbc

	union {
		struct {
			unsigned res2_cl6_sca_width            : 13;        // bits : 12_0
			unsigned                               : 3;
			unsigned res2_cl6_sca_hstep_oft        : 15;        // bits : 30_16
		} bit;
		UINT32 word;
	} reg_880; // 0x0dc0

	union {
		struct {
			unsigned res2_cl6_out_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl6_out_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_881; // 0x0dc4

	union {
		struct {
			unsigned res2_cl6_rlt_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl6_rlt_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_882; // 0x0dc8

	union {
		struct {
			unsigned res2_cl6_pip_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl6_pip_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_883; // 0x0dcc

	union {
		struct {
			unsigned res2_cl6_sca_crop_x_start        : 12;     // bits : 11_0
			unsigned                                  : 4;
			unsigned res2_cl6_sca_crop_width          : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_884; // 0x0dd0

	union {
		struct {
			unsigned res2_cl6_tc_crop_x_start        : 13;      // bits : 12_0
			unsigned                                 : 3;
			unsigned res2_cl6_tc_crop_width          : 13;      // bits : 28_16
			unsigned                                 : 2;
			unsigned res2_cl6_tc_crop_skip           : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_885; // 0x0dd4

	union {
		struct {
			unsigned vpe_reserved_539        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_886; // 0x0dd8

	union {
		struct {
			unsigned vpe_reserved_540        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_887; // 0x0ddc

	union {
		struct {
			unsigned res2_cl7_sca_width            : 13;        // bits : 12_0
			unsigned                               : 3;
			unsigned res2_cl7_sca_hstep_oft        : 15;        // bits : 30_16
		} bit;
		UINT32 word;
	} reg_888; // 0x0de0

	union {
		struct {
			unsigned res2_cl7_out_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl7_out_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_889; // 0x0de4

	union {
		struct {
			unsigned res2_cl7_rlt_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl7_rlt_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_890; // 0x0de8

	union {
		struct {
			unsigned res2_cl7_pip_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res2_cl7_pip_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_891; // 0x0dec

	union {
		struct {
			unsigned res2_cl7_sca_crop_x_start        : 12;     // bits : 11_0
			unsigned                                  : 4;
			unsigned res2_cl7_sca_crop_width          : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_892; // 0x0df0

	union {
		struct {
			unsigned res2_cl7_tc_crop_x_start        : 13;      // bits : 12_0
			unsigned                                 : 3;
			unsigned res2_cl7_tc_crop_width          : 13;      // bits : 28_16
			unsigned                                 : 2;
			unsigned res2_cl7_tc_crop_skip           : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_893; // 0x0df4

	union {
		struct {
			unsigned vpe_reserved_541        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_894; // 0x0df8

	union {
		struct {
			unsigned vpe_reserved_542        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_895; // 0x0dfc

	union {
		struct {
			unsigned res3_cl0_sca_width            : 13;        // bits : 12_0
			unsigned                               : 3;
			unsigned res3_cl0_sca_hstep_oft        : 15;        // bits : 30_16
		} bit;
		UINT32 word;
	} reg_896; // 0x0e00

	union {
		struct {
			unsigned res3_cl0_out_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl0_out_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_897; // 0x0e04

	union {
		struct {
			unsigned res3_cl0_rlt_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl0_rlt_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_898; // 0x0e08

	union {
		struct {
			unsigned res3_cl0_pip_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl0_pip_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_899; // 0x0e0c

	union {
		struct {
			unsigned res3_cl0_sca_crop_x_start        : 12;     // bits : 11_0
			unsigned                                  : 4;
			unsigned res3_cl0_sca_crop_width          : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_900; // 0x0e10

	union {
		struct {
			unsigned res3_cl0_tc_crop_x_start        : 13;      // bits : 12_0
			unsigned                                 : 3;
			unsigned res3_cl0_tc_crop_width          : 13;      // bits : 28_16
			unsigned                                 : 2;
			unsigned res3_cl0_tc_crop_skip           : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_901; // 0x0e14

	union {
		struct {
			unsigned vpe_reserved_543        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_902; // 0x0e18

	union {
		struct {
			unsigned vpe_reserved_544        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_903; // 0x0e1c

	union {
		struct {
			unsigned res3_cl1_sca_width            : 13;        // bits : 12_0
			unsigned                               : 3;
			unsigned res3_cl1_sca_hstep_oft        : 15;        // bits : 30_16
		} bit;
		UINT32 word;
	} reg_904; // 0x0e20

	union {
		struct {
			unsigned res3_cl1_out_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl1_out_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_905; // 0x0e24

	union {
		struct {
			unsigned res3_cl1_rlt_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl1_rlt_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_906; // 0x0e28

	union {
		struct {
			unsigned res3_cl1_pip_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl1_pip_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_907; // 0x0e2c

	union {
		struct {
			unsigned res3_cl1_sca_crop_x_start        : 12;     // bits : 11_0
			unsigned                                  : 4;
			unsigned res3_cl1_sca_crop_width          : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_908; // 0x0e30

	union {
		struct {
			unsigned res3_cl1_tc_crop_x_start        : 13;      // bits : 12_0
			unsigned                                 : 3;
			unsigned res3_cl1_tc_crop_width          : 13;      // bits : 28_16
			unsigned                                 : 2;
			unsigned res3_cl1_tc_crop_skip           : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_909; // 0x0e34

	union {
		struct {
			unsigned vpe_reserved_545        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_910; // 0x0e38

	union {
		struct {
			unsigned vpe_reserved_546        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_911; // 0x0e3c

	union {
		struct {
			unsigned res3_cl2_sca_width            : 13;        // bits : 12_0
			unsigned                               : 3;
			unsigned res3_cl2_sca_hstep_oft        : 15;        // bits : 30_16
		} bit;
		UINT32 word;
	} reg_912; // 0x0e40

	union {
		struct {
			unsigned res3_cl2_out_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl2_out_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_913; // 0x0e44

	union {
		struct {
			unsigned res3_cl2_rlt_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl2_rlt_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_914; // 0x0e48

	union {
		struct {
			unsigned res3_cl2_pip_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl2_pip_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_915; // 0x0e4c

	union {
		struct {
			unsigned res3_cl2_sca_crop_x_start        : 12;     // bits : 11_0
			unsigned                                  : 4;
			unsigned res3_cl2_sca_crop_width          : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_916; // 0x0e50

	union {
		struct {
			unsigned res3_cl2_tc_crop_x_start        : 13;      // bits : 12_0
			unsigned                                 : 3;
			unsigned res3_cl2_tc_crop_width          : 13;      // bits : 28_16
			unsigned                                 : 2;
			unsigned res3_cl2_tc_crop_skip           : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_917; // 0x0e54

	union {
		struct {
			unsigned vpe_reserved_547        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_918; // 0x0e58

	union {
		struct {
			unsigned vpe_reserved_548        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_919; // 0x0e5c

	union {
		struct {
			unsigned res3_cl3_sca_width            : 13;        // bits : 12_0
			unsigned                               : 3;
			unsigned res3_cl3_sca_hstep_oft        : 15;        // bits : 30_16
		} bit;
		UINT32 word;
	} reg_920; // 0x0e60

	union {
		struct {
			unsigned res3_cl3_out_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl3_out_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_921; // 0x0e64

	union {
		struct {
			unsigned r_res3_cl3_lt_x_start        : 13;     // bits : 12_0
			unsigned                              : 3;
			unsigned res3_cl3_rlt_width           : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_922; // 0x0e68

	union {
		struct {
			unsigned res3_cl3_pip_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl3_pip_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_923; // 0x0e6c

	union {
		struct {
			unsigned res3_cl3_sca_crop_x_start        : 12;     // bits : 11_0
			unsigned                                  : 4;
			unsigned res3_cl3_sca_crop_width          : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_924; // 0x0e70

	union {
		struct {
			unsigned res3_cl3_tc_crop_x_start        : 13;      // bits : 12_0
			unsigned                                 : 3;
			unsigned res3_cl3_tc_crop_width          : 13;      // bits : 28_16
			unsigned                                 : 2;
			unsigned res3_cl3_tc_crop_skip           : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_925; // 0x0e74

	union {
		struct {
			unsigned vpe_reserved_549        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_926; // 0x0e78

	union {
		struct {
			unsigned vpe_reserved_550        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_927; // 0x0e7c

	union {
		struct {
			unsigned res3_cl4_sca_width            : 13;        // bits : 12_0
			unsigned                               : 3;
			unsigned res3_cl4_sca_hstep_oft        : 15;        // bits : 30_16
		} bit;
		UINT32 word;
	} reg_928; // 0x0e80

	union {
		struct {
			unsigned res3_cl4_out_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl4_out_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_929; // 0x0e84

	union {
		struct {
			unsigned res3_cl4_rlt_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl4_rlt_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_930; // 0x0e88

	union {
		struct {
			unsigned res3_cl4_pip_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl4_pip_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_931; // 0x0e8c

	union {
		struct {
			unsigned res3_cl4_sca_crop_x_start        : 12;     // bits : 11_0
			unsigned                                  : 4;
			unsigned res3_cl4_sca_crop_width          : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_932; // 0x0e90

	union {
		struct {
			unsigned res3_cl4_tc_crop_x_start        : 13;      // bits : 12_0
			unsigned                                 : 3;
			unsigned res3_cl4_tc_crop_width          : 13;      // bits : 28_16
			unsigned                                 : 2;
			unsigned res3_cl4_tc_crop_skip           : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_933; // 0x0e94

	union {
		struct {
			unsigned vpe_reserved_551        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_934; // 0x0e98

	union {
		struct {
			unsigned vpe_reserved_552        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_935; // 0x0e9c

	union {
		struct {
			unsigned res3_cl5_sca_width            : 13;        // bits : 12_0
			unsigned                               : 3;
			unsigned res3_cl5_sca_hstep_oft        : 15;        // bits : 30_16
		} bit;
		UINT32 word;
	} reg_936; // 0x0ea0

	union {
		struct {
			unsigned res3_cl5_out_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl5_out_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_937; // 0x0ea4

	union {
		struct {
			unsigned res3_cl5_rlt_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl5_rlt_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_938; // 0x0ea8

	union {
		struct {
			unsigned res3_cl5_pip_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl5_pip_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_939; // 0x0eac

	union {
		struct {
			unsigned res3_cl5_sca_crop_x_start        : 12;     // bits : 11_0
			unsigned                                  : 4;
			unsigned res3_cl5_sca_crop_width          : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_940; // 0x0eb0

	union {
		struct {
			unsigned res3_cl5_tc_crop_x_start        : 13;      // bits : 12_0
			unsigned                                 : 3;
			unsigned res3_cl5_tc_crop_width          : 13;      // bits : 28_16
			unsigned                                 : 2;
			unsigned res3_cl5_tc_crop_skip           : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_941; // 0x0eb4

	union {
		struct {
			unsigned vpe_reserved_553        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_942; // 0x0eb8

	union {
		struct {
			unsigned vpe_reserved_554        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_943; // 0x0ebc

	union {
		struct {
			unsigned res3_cl6_sca_width            : 13;        // bits : 12_0
			unsigned                               : 3;
			unsigned res3_cl6_sca_hstep_oft        : 15;        // bits : 30_16
		} bit;
		UINT32 word;
	} reg_944; // 0x0ec0

	union {
		struct {
			unsigned res3_cl6_out_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl6_out_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_945; // 0x0ec4

	union {
		struct {
			unsigned res3_cl6_rlt_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl6_rlt_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_946; // 0x0ec8

	union {
		struct {
			unsigned res3_cl6_pip_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl6_pip_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_947; // 0x0ecc

	union {
		struct {
			unsigned res3_cl6_sca_crop_x_start        : 12;     // bits : 11_0
			unsigned                                  : 4;
			unsigned res3_cl6_sca_crop_width          : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_948; // 0x0ed0

	union {
		struct {
			unsigned res3_cl6_tc_crop_x_start        : 13;      // bits : 12_0
			unsigned                                 : 3;
			unsigned res3_cl6_tc_crop_width          : 13;      // bits : 28_16
			unsigned                                 : 2;
			unsigned res3_cl6_tc_crop_skip           : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_949; // 0x0ed4

	union {
		struct {
			unsigned vpe_reserved_555        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_950; // 0x0ed8

	union {
		struct {
			unsigned vpe_reserved_556        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_951; // 0x0edc

	union {
		struct {
			unsigned res3_cl7_sca_width            : 13;        // bits : 12_0
			unsigned                               : 3;
			unsigned res3_cl7_sca_hstep_oft        : 15;        // bits : 30_16
		} bit;
		UINT32 word;
	} reg_952; // 0x0ee0

	union {
		struct {
			unsigned res3_cl7_out_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl7_out_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_953; // 0x0ee4

	union {
		struct {
			unsigned res3_cl7_rlt_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl7_rlt_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_954; // 0x0ee8

	union {
		struct {
			unsigned res3_cl7_pip_x_start        : 13;      // bits : 12_0
			unsigned                             : 3;
			unsigned res3_cl7_pip_width          : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_955; // 0x0eec

	union {
		struct {
			unsigned res3_cl7_sca_crop_x_start        : 12;     // bits : 11_0
			unsigned                                  : 4;
			unsigned res3_cl7_sca_crop_width          : 13;     // bits : 28_16
		} bit;
		UINT32 word;
	} reg_956; // 0x0ef0

	union {
		struct {
			unsigned res3_cl7_tc_crop_x_start        : 13;      // bits : 12_0
			unsigned                                 : 3;
			unsigned res3_cl7_tc_crop_width          : 13;      // bits : 28_16
			unsigned                                 : 2;
			unsigned res3_cl7_tc_crop_skip           : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_957; // 0x0ef4

	union {
		struct {
			unsigned vpe_reserved_557        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_958; // 0x0ef8

	union {
		struct {
			unsigned vpe_reserved_558        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_959; // 0x0efc

	union {
		struct {
			unsigned                          : 4;
			unsigned res0_enc_smode_en        : 1;      // bits : 4
			unsigned res1_enc_smode_en        : 1;      // bits : 5
		} bit;
		UINT32 word;
	} reg_960; // 0x0f00

	union {
		struct {
			unsigned vpe_reserved_559        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_961; // 0x0f04

	union {
		struct {
			unsigned vpe_reserved_560        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_962; // 0x0f08

	union {
		struct {
			unsigned vpe_reserved_561        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_963; // 0x0f0c

	union {
		struct {
			unsigned dct_qtbl0_enc_p0        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl0_enc_p1        : 12;      // bits : 27_16
		} bit;
		UINT32 word;
	} reg_964; // 0x0f10

	union {
		struct {
			unsigned dct_qtbl0_enc_p2        : 11;      // bits : 10_0
		} bit;
		UINT32 word;
	} reg_965; // 0x0f14

	union {
		struct {
			unsigned dct_qtbl1_enc_p0        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl1_enc_p1        : 12;      // bits : 27_16
		} bit;
		UINT32 word;
	} reg_966; // 0x0f18

	union {
		struct {
			unsigned dct_qtbl1_enc_p2        : 11;      // bits : 10_0
		} bit;
		UINT32 word;
	} reg_967; // 0x0f1c

	union {
		struct {
			unsigned dct_qtbl2_enc_p0        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl2_enc_p1        : 12;      // bits : 27_16
		} bit;
		UINT32 word;
	} reg_968; // 0x0f20

	union {
		struct {
			unsigned dct_qtbl2_enc_p2        : 11;      // bits : 10_0
		} bit;
		UINT32 word;
	} reg_969; // 0x0f24

	union {
		struct {
			unsigned dct_qtbl3_enc_p0        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl3_enc_p1        : 12;      // bits : 27_16
		} bit;
		UINT32 word;
	} reg_970; // 0x0f28

	union {
		struct {
			unsigned dct_qtbl3_enc_p2        : 11;      // bits : 10_0
		} bit;
		UINT32 word;
	} reg_971; // 0x0f2c

	union {
		struct {
			unsigned dct_qtbl4_enc_p0        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl4_enc_p1        : 12;      // bits : 27_16
		} bit;
		UINT32 word;
	} reg_972; // 0x0f30

	union {
		struct {
			unsigned dct_qtbl4_enc_p2        : 11;      // bits : 10_0
		} bit;
		UINT32 word;
	} reg_973; // 0x0f34

	union {
		struct {
			unsigned dct_qtbl5_enc_p0        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl5_enc_p1        : 12;      // bits : 27_16
		} bit;
		UINT32 word;
	} reg_974; // 0x0f38

	union {
		struct {
			unsigned dct_qtbl5_enc_p2        : 11;      // bits : 10_0
		} bit;
		UINT32 word;
	} reg_975; // 0x0f3c

	union {
		struct {
			unsigned dct_qtbl6_enc_p0        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl6_enc_p1        : 12;      // bits : 27_16
		} bit;
		UINT32 word;
	} reg_976; // 0x0f40

	union {
		struct {
			unsigned dct_qtbl6_enc_p2        : 11;      // bits : 10_0
		} bit;
		UINT32 word;
	} reg_977; // 0x0f44

	union {
		struct {
			unsigned dct_qtbl7_enc_p0        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl7_enc_p1        : 12;      // bits : 27_16
		} bit;
		UINT32 word;
	} reg_978; // 0x0f48

	union {
		struct {
			unsigned dct_qtbl7_enc_p2        : 11;      // bits : 10_0
		} bit;
		UINT32 word;
	} reg_979; // 0x0f4c

	union {
		struct {
			unsigned dct_qtbl8_enc_p0        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl8_enc_p1        : 12;      // bits : 27_16
		} bit;
		UINT32 word;
	} reg_980; // 0x0f50

	union {
		struct {
			unsigned dct_qtbl8_enc_p2        : 11;      // bits : 10_0
		} bit;
		UINT32 word;
	} reg_981; // 0x0f54

	union {
		struct {
			unsigned dct_qtbl9_enc_p0        : 13;      // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl9_enc_p1        : 12;      // bits : 27_16
		} bit;
		UINT32 word;
	} reg_982; // 0x0f58

	union {
		struct {
			unsigned dct_qtbl9_enc_p2        : 11;      // bits : 10_0
		} bit;
		UINT32 word;
	} reg_983; // 0x0f5c

	union {
		struct {
			unsigned dct_qtbl10_enc_p0        : 13;     // bits : 12_0
			unsigned                          : 3;
			unsigned dct_qtbl10_enc_p1        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_984; // 0x0f60

	union {
		struct {
			unsigned dct_qtbl11_enc_p2        : 11;     // bits : 10_0
		} bit;
		UINT32 word;
	} reg_985; // 0x0f64

	union {
		struct {
			unsigned dct_qtbl11_enc_p0        : 13;     // bits : 12_0
			unsigned                          : 3;
			unsigned dct_qtbl11_enc_p1        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_986; // 0x0f68

	union {
		struct {
			unsigned dct_qtbl11_enc_p2        : 11;     // bits : 10_0
		} bit;
		UINT32 word;
	} reg_987; // 0x0f6c

	union {
		struct {
			unsigned dct_qtbl12_enc_p0        : 13;     // bits : 12_0
			unsigned                          : 3;
			unsigned dct_qtbl12_enc_p1        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_988; // 0x0f70

	union {
		struct {
			unsigned dct_qtbl12_enc_p2        : 11;     // bits : 10_0
		} bit;
		UINT32 word;
	} reg_989; // 0x0f74

	union {
		struct {
			unsigned dct_qtbl13_enc_p0        : 13;     // bits : 12_0
			unsigned                          : 3;
			unsigned dct_qtbl13_enc_p1        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_990; // 0x0f78

	union {
		struct {
			unsigned dct_qtbl13_enc_p2        : 11;     // bits : 10_0
		} bit;
		UINT32 word;
	} reg_991; // 0x0f7c

	union {
		struct {
			unsigned dct_qtbl14_enc_p0        : 13;     // bits : 12_0
			unsigned                          : 3;
			unsigned dct_qtbl14_enc_p1        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_992; // 0x0f80

	union {
		struct {
			unsigned dct_qtbl14_enc_p2        : 11;     // bits : 10_0
		} bit;
		UINT32 word;
	} reg_993; // 0x0f84

	union {
		struct {
			unsigned dct_qtbl15_enc_p0        : 13;     // bits : 12_0
			unsigned                          : 3;
			unsigned dct_qtbl15_enc_p1        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_994; // 0x0f88

	union {
		struct {
			unsigned dct_qtbl15_enc_p2        : 11;     // bits : 10_0
		} bit;
		UINT32 word;
	} reg_995; // 0x0f8c

	union {
		struct {
			unsigned dct_qtbl0_dcmax        : 9;        // bits : 8_0
			unsigned                        : 7;
			unsigned dct_qtbl1_dcmax        : 9;        // bits : 24_16
		} bit;
		UINT32 word;
	} reg_996; // 0x0f90

	union {
		struct {
			unsigned dct_qtbl2_dcmax        : 9;        // bits : 8_0
			unsigned                        : 7;
			unsigned dct_qtbl3_dcmax        : 9;        // bits : 24_16
		} bit;
		UINT32 word;
	} reg_997; // 0x0f94

	union {
		struct {
			unsigned dct_qtbl4_dcmax        : 9;        // bits : 8_0
			unsigned                        : 7;
			unsigned dct_qtbl5_dcmax        : 9;        // bits : 24_16
		} bit;
		UINT32 word;
	} reg_998; // 0x0f98

	union {
		struct {
			unsigned dct_qtbl6_dcmax        : 9;        // bits : 8_0
			unsigned                        : 7;
			unsigned dct_qtbl7_dcmax        : 9;        // bits : 24_16
		} bit;
		UINT32 word;
	} reg_999; // 0x0f9c

	union {
		struct {
			unsigned dct_qtbl8_dcmax        : 9;        // bits : 8_0
			unsigned                        : 7;
			unsigned dct_qtbl9_dcmax        : 9;        // bits : 24_16
		} bit;
		UINT32 word;
	} reg_1000; // 0x0fa0

	union {
		struct {
			unsigned dct_qtbl10_dcmax        : 9;       // bits : 8_0
			unsigned                         : 7;
			unsigned dct_qtbl11_dcmax        : 9;       // bits : 24_16
		} bit;
		UINT32 word;
	} reg_1001; // 0x0fa4

	union {
		struct {
			unsigned dct_qtbl12_dcmax        : 9;       // bits : 8_0
			unsigned                         : 7;
			unsigned dct_qtbl13_dcmax        : 9;       // bits : 24_16
		} bit;
		UINT32 word;
	} reg_1002; // 0x0fa8

	union {
		struct {
			unsigned dct_qtbl14_dcmax        : 9;       // bits : 8_0
			unsigned                         : 7;
			unsigned dct_qtbl15_dcmax        : 9;       // bits : 24_16
		} bit;
		UINT32 word;
	} reg_1003; // 0x0fac

	union {
		struct {
			unsigned yrc_lncnt_lfn0        : 13;        // bits : 12_0
			unsigned                       : 3;
			unsigned yrc_lncnt_lfn1        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_1004; // 0x0fb0

	union {
		struct {
			unsigned yrc_lncnt_lfn2        : 13;        // bits : 12_0
			unsigned                       : 3;
			unsigned yrc_lncnt_lfn3        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_1005; // 0x0fb4

	union {
		struct {
			unsigned yrc_lncnt_lfn4        : 13;        // bits : 12_0
			unsigned                       : 3;
			unsigned yrc_lncnt_lfn5        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_1006; // 0x0fb8

	union {
		struct {
			unsigned dct_maxdist        : 8;        // bits : 7_0
		} bit;
		UINT32 word;
	} reg_1007; // 0x0fbc

	union {
		struct {
			unsigned dct_qtbl0_dec_p0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl0_dec_p1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_1008; // 0x0fc0

	union {
		struct {
			unsigned dct_qtbl0_dec_p2        : 10;      // bits : 9_0
		} bit;
		UINT32 word;
	} reg_1009; // 0x0fc4

	union {
		struct {
			unsigned dct_qtbl1_dec_p0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl1_dec_p1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_1010; // 0x0fc8

	union {
		struct {
			unsigned dct_qtbl1_dec_p2        : 10;      // bits : 9_0
		} bit;
		UINT32 word;
	} reg_1011; // 0x0fcc

	union {
		struct {
			unsigned dct_qtbl2_dec_p0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl2_dec_p1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_1012; // 0x0fd0

	union {
		struct {
			unsigned dct_qtbl2_dec_p2        : 10;      // bits : 9_0
		} bit;
		UINT32 word;
	} reg_1013; // 0x0fd4

	union {
		struct {
			unsigned dct_qtbl3_dec_p0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl3_dec_p1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_1014; // 0x0fd8

	union {
		struct {
			unsigned dct_qtbl3_dec_p2        : 10;      // bits : 9_0
		} bit;
		UINT32 word;
	} reg_1015; // 0x0fdc

	union {
		struct {
			unsigned dct_qtbl4_dec_p0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl4_dec_p1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_1016; // 0x0fe0

	union {
		struct {
			unsigned dct_qtbl4_dec_p2        : 10;      // bits : 9_0
		} bit;
		UINT32 word;
	} reg_1017; // 0x0fe4

	union {
		struct {
			unsigned dct_qtbl5_dec_p0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl5_dec_p1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_1018; // 0x0fe8

	union {
		struct {
			unsigned dct_qtbl5_dec_p2        : 10;      // bits : 9_0
		} bit;
		UINT32 word;
	} reg_1019; // 0x0fec

	union {
		struct {
			unsigned dct_qtbl6_dec_p0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl6_dec_p1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_1020; // 0x0ff0

	union {
		struct {
			unsigned dct_qtbl6_dec_p2        : 10;      // bits : 9_0
		} bit;
		UINT32 word;
	} reg_1021; // 0x0ff4

	union {
		struct {
			unsigned dct_qtbl7_dec_p0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl7_dec_p1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_1022; // 0x0ff8

	union {
		struct {
			unsigned dct_qtbl7_dec_p2        : 10;      // bits : 9_0
		} bit;
		UINT32 word;
	} reg_1023; // 0x0ffc

	union {
		struct {
			unsigned dct_qtbl8_dec_p0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl8_dec_p1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_1024; // 0x1000

	union {
		struct {
			unsigned dct_qtbl8_dec_p2        : 10;      // bits : 9_0
		} bit;
		UINT32 word;
	} reg_1025; // 0x1004

	union {
		struct {
			unsigned dct_qtbl9_dec_p0        : 10;      // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl9_dec_p1        : 10;      // bits : 25_16
		} bit;
		UINT32 word;
	} reg_1026; // 0x1008

	union {
		struct {
			unsigned dct_qtbl9_dec_p2        : 10;      // bits : 9_0
		} bit;
		UINT32 word;
	} reg_1027; // 0x100c

	union {
		struct {
			unsigned dct_qtbl10_dec_p0        : 10;     // bits : 9_0
			unsigned                          : 6;
			unsigned dct_qtbl10_dec_p1        : 10;     // bits : 25_16
		} bit;
		UINT32 word;
	} reg_1028; // 0x1010

	union {
		struct {
			unsigned dct_qtbl10_dec_p2        : 10;     // bits : 9_0
		} bit;
		UINT32 word;
	} reg_1029; // 0x1014

	union {
		struct {
			unsigned dct_qtbl11_dec_p0        : 10;     // bits : 9_0
			unsigned                          : 6;
			unsigned dct_qtbl11_dec_p1        : 10;     // bits : 25_16
		} bit;
		UINT32 word;
	} reg_1030; // 0x1018

	union {
		struct {
			unsigned dct_qtbl11_dec_p2        : 10;     // bits : 9_0
		} bit;
		UINT32 word;
	} reg_1031; // 0x101c

	union {
		struct {
			unsigned dct_qtbl12_dec_p0        : 10;     // bits : 9_0
			unsigned                          : 6;
			unsigned dct_qtbl12_dec_p1        : 10;     // bits : 25_16
		} bit;
		UINT32 word;
	} reg_1032; // 0x1020

	union {
		struct {
			unsigned dct_qtbl12_dec_p2        : 10;     // bits : 9_0
		} bit;
		UINT32 word;
	} reg_1033; // 0x1024

	union {
		struct {
			unsigned dct_qtbl13_dec_p0        : 10;     // bits : 9_0
			unsigned                          : 6;
			unsigned dct_qtbl13_dec_p1        : 10;     // bits : 25_16
		} bit;
		UINT32 word;
	} reg_1034; // 0x1028

	union {
		struct {
			unsigned dct_qtbl13_dec_p2        : 10;     // bits : 9_0
		} bit;
		UINT32 word;
	} reg_1035; // 0x102c

	union {
		struct {
			unsigned dct_qtbl14_dec_p0        : 10;     // bits : 9_0
			unsigned                          : 6;
			unsigned dct_qtbl14_dec_p1        : 10;     // bits : 25_16
		} bit;
		UINT32 word;
	} reg_1036; // 0x1030

	union {
		struct {
			unsigned dct_qtbl14_dec_p2        : 10;     // bits : 9_0
		} bit;
		UINT32 word;
	} reg_1037; // 0x1034

	union {
		struct {
			unsigned dct_qtbl15_dec_p0        : 10;     // bits : 9_0
			unsigned                          : 6;
			unsigned dct_qtbl15_dec_p1        : 10;     // bits : 25_16
		} bit;
		UINT32 word;
	} reg_1038; // 0x1038

	union {
		struct {
			unsigned dct_qtbl15_dec_p2        : 10;     // bits : 9_0
		} bit;
		UINT32 word;
	} reg_1039; // 0x103c

} NT98528_VPE_REGISTER_STRUCT;

#ifdef __cplusplus
}
#endif

#endif

