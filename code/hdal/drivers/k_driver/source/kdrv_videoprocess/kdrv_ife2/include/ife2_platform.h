/*
    IFE2 module driver

    NT98520 IFE2 internal header file.

    @file       ife2_platform.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _IFE2_PLATFORM_H_
#define _IFE2_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif

#define IFE2_ALIGN_FLOOR(value, base)  ((value) & ~((base)-1))                   ///< Align Floor
#define IFE2_ALIGN_ROUND(value, base)  IFE2_ALIGN_FLOOR((value) + ((base)/2), base)   ///< Align Round
#define IFE2_ALIGN_CEIL(value, base)   IFE2_ALIGN_FLOOR((value) + ((base)-1), base)   ///< Align Ceil

#define IFE2_ALIGN_ROUND_64(a)       IFE2_ALIGN_ROUND(a, 64)  ///< Round Off to 64
#define IFE2_ALIGN_ROUND_32(a)       IFE2_ALIGN_ROUND(a, 32)  ///< Round Off to 32
#define IFE2_ALIGN_ROUND_16(a)       IFE2_ALIGN_ROUND(a, 16)  ///< Round Off to 16
#define IFE2_ALIGN_ROUND_8(a)        IFE2_ALIGN_ROUND(a, 8)   ///< Round Off to 8
#define IFE2_ALIGN_ROUND_4(a)        IFE2_ALIGN_ROUND(a, 4)   ///< Round Off to 4

#define IFE2_ALIGN_CEIL_64(a)        IFE2_ALIGN_CEIL(a, 64)   ///< Round Up to 64
#define IFE2_ALIGN_CEIL_32(a)        IFE2_ALIGN_CEIL(a, 32)   ///< Round Up to 32
#define IFE2_ALIGN_CEIL_16(a)        IFE2_ALIGN_CEIL(a, 16)   ///< Round Up to 16
#define IFE2_ALIGN_CEIL_8(a)         IFE2_ALIGN_CEIL(a, 8)    ///< Round Up to 8
#define IFE2_ALIGN_CEIL_4(a)         IFE2_ALIGN_CEIL(a, 4)    ///< Round Up to 4

#define IFE2_ALIGN_FLOOR_64(a)       IFE2_ALIGN_FLOOR(a, 64)  ///< Round down to 64
#define IFE2_ALIGN_FLOOR_32(a)       IFE2_ALIGN_FLOOR(a, 32)  ///< Round down to 32
#define IFE2_ALIGN_FLOOR_16(a)       IFE2_ALIGN_FLOOR(a, 16)  ///< Round down to 16
#define IFE2_ALIGN_FLOOR_8(a)        IFE2_ALIGN_FLOOR(a, 8)   ///< Round down to 8
#define IFE2_ALIGN_FLOOR_4(a)        IFE2_ALIGN_FLOOR(a, 4)   ///< Round down to 4


//-------------------------------------------------------------------------
#include "plat/top.h"


#if defined (__KERNEL__)

#include <linux/clk.h>
#include <linux/spinlock.h>


#include <linux/module.h>
#include <linux/export.h>
#include <linux/dma-mapping.h> // header file Dma(cache handle)

#include "mach/rcw_macro.h"
#include "mach/nvt-io.h"
#include <mach/fmem.h>
#include <mach/rcw_macro.h>

#include "kwrap/type.h"//a header for basic variable type
#include "kwrap/task.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/spinlock.h"
#include <kwrap/cpu.h>



#include <plat-na51055/nvt-sramctl.h>

#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/kdrv_ipp_builtin.h"

#include "ife2_dbg.h"
#include "ife2_drv.h"

#define pmc_turnonPower(a)
#define pmc_turnoffPower(a)

#define dma_getNonCacheAddr(parm) 0
#define dma_getPhyAddr(parm) 0
#define dma_flushWriteCache(parm,parm2)
#define dma_flushReadCache(parm,parm2)
#define dma_flushReadCacheWidthNEQLineOffset(parm,parm2)


#define pll_enableClock(x)
#define pll_disableClock(x)
#define pll_setClockRate(x,y)
#define pll_getPLLEn(x,y)
#define pll_setPLLEn(x)

extern UINT32 _IFE2_REGIOBASE;

#define _IFE2_REG_BASE_ADDR      _IFE2_REGIOBASE
#define IFE2_REG_ADDR(ofs)        (_IFE2_REG_BASE_ADDR+(ofs))
#define IFE2_SETREG(ofs, value)   kdrv_ipp_set_reg(KDRV_IPP_BUILTIN_IFE2, _IFE2_REG_BASE_ADDR, ofs, value)
#define IFE2_GETREG(ofs)          ioread32((void*)(_IFE2_REG_BASE_ADDR + ofs))

#define IFE2_SET_32BIT_VALUE(addr, value)    iowrite32(value, (void*)addr)


extern struct clk *ife2_clk[IFE2_CLK_NUM];
extern struct clk *ife2_pclk[IFE2_CLK_NUM];


//=========================================================================
#elif defined (__FREERTOS)

#include "string.h"
#include <stdlib.h>
#include "rcw_macro.h"
#include "io_address.h"

#if defined(_BSP_NA51055_)
#include "nvt-sramctl.h"
#endif

#include "interrupt.h"
#include "pll.h"
#include "pll_protected.h"
#include "dma_protected.h"
#include "kwrap/type.h"
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include <kwrap/spinlock.h>
#include <kwrap/cpu.h>
#include <kwrap/nvt_type.h>
#include "kdrv_builtin/kdrv_ipp_builtin.h"
#include "ife2_dbg.h"


#define pmc_turnonPower(a)
#define pmc_turnoffPower(a)

#define _IFE2_REG_BASE_ADDR       IOADDR_IFE2_REG_BASE  //(0xF0C40000)
#define IFE2_REG_ADDR(ofs)        (_IFE2_REG_BASE_ADDR+(ofs))
#define IFE2_SETREG(ofs, value)   OUTW((_IFE2_REG_BASE_ADDR + ofs), value)
#define IFE2_GETREG(ofs)          INW(_IFE2_REG_BASE_ADDR + ofs)

#define IFE2_SET_32BIT_VALUE(addr, value)    (*(volatile UINT32*)(addr) = (UINT32)(value))


//=========================================================================
#elif defined (__UITRON) || defined (__ECOS)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Type.h"
#include "ErrorNo.h"
#include "Debug.h"
#include "dma.h"

#include "DrvCommon.h"
#include "interrupt.h"
//#include "pll.h"
#include "pll_protected.h"
#include "top.h"
#include "Utility.h"
#include "Memory.h"
#include "ist.h"


#include "dal_ipp_utility.h"


#define __MODULE__    dal_ime
#define __DBGLVL__    2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*" // *=All, [mark1]=CustomClass
#include "DebugModule.h"


#define _IFE2_REG_BASE_ADDR       IOADDR_IFE2_REG_BASE  //(0xF0C40000)
#define IFE2_REG_ADDR(ofs)        (_IFE2_REG_BASE_ADDR+(ofs))
#define IFE2_SETREG(ofs, value)   OUTW((_IFE2_REG_BASE_ADDR + ofs), value)
#define IFE2_GETREG(ofs)          INW(_IFE2_REG_BASE_ADDR + ofs)

#define IFE2_SET_32BIT_VALUE(addr, value)    (*(volatile UINT32*)(addr) = (UINT32)(value))

//=========================================================================
#else


#endif


//-------------------------------------------------------------------------

#define DRV_SUPPORT_IST  0
#define _EMULATION_      0

#define FLGPTN_IFE2_FRAMEEND    FLGPTN_BIT(0)
#define FLGPTN_IFE2_LLEND       FLGPTN_BIT(1)
#define FLGPTN_IFE2_JEND        FLGPTN_BIT(2)


extern BOOL ife2_ctrl_flow_to_do;
extern BOOL fw_ife2_power_saving_en;

extern ER ife2_platform_flg_clear(FLGPTN flg);

extern ER ife2_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg);

extern ER ife2_platform_flg_set(FLGPTN flg);

extern ER ife2_platform_sem_wait(VOID);

extern ER ife2_platform_sem_signal(VOID);

extern unsigned long ife2_platform_spin_lock(VOID);

extern VOID ife2_platform_spin_unlock(unsigned long loc_status);

extern VOID ife2_platform_int_enable(VOID);

extern VOID ife2_platform_int_disable(VOID);

extern VOID ife2_platform_enable_clk(VOID);

extern VOID ife2_platform_disable_clk(VOID);

extern VOID ife2_platform_prepare_clk(VOID);

extern VOID ife2_platform_unprepare_clk(VOID);

extern VOID ife2_platform_disable_sram_shutdown(VOID);

extern VOID ife2_platform_enable_sram_shutdown(VOID);

extern ER ife2_platform_set_clk_rate(UINT32 source_clk);

extern UINT32 ife2_platform_get_clk_rate(VOID);

extern UINT32 ife2_platform_va2pa(UINT32 addr);


extern UINT32 ife2_platform_dma_is_cacheable(UINT32 addr);

extern UINT32 ife2_platform_dma_flush_mem2dev_for_video_mode(UINT32 addr, UINT32 size);

extern UINT32 ife2_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size);


extern UINT32 ife2_platform_dma_flush_dev2mem_width_neq_loff(UINT32 addr, UINT32 size);

extern UINT32 ife2_platform_dma_flush_dev2mem_for_video_mode(UINT32 addr, UINT32 size);

extern UINT32 ife2_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size);

extern UINT32 ife2_platform_dma_post_flush_dev2mem(UINT32 addr, UINT32 size);

extern VOID ife2_platform_set_init_status(BOOL set_status);
extern BOOL ife2_platform_get_init_status(VOID);



#if !(defined __UITRON || defined __ECOS)
extern VOID ife2_isr(VOID);
#if defined __FREERTOS
extern VOID ife2_platform_create_resource(VOID);
#else
extern VOID ife2_platform_create_resource(IFE2_INFO *pmodule_info);
#endif
extern VOID ife2_platform_release_resource(VOID);
#endif



#ifdef __cplusplus
}
#endif


#endif // _IME_PM_REG_

