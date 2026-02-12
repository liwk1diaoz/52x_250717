/*
    SDE module driver

    NT98528 SDE internal header file.

    @file       sde_platform.h
    @ingroup    mIIPPISE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _SDE_PLATFORM_H_
#define _SDE_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif


#define DRV_SUPPORT_IST  0
#define _EMULATION_      0




//-------------------------------------------------------------------------
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

//#include "kwrap/type.h"//a header for basic variable type
//#include "kwrap/task.h"
//#include "kwrap/semaphore.h"
//#include "kwrap/flag.h"
//#include "kwrap/spinlock.h"


#include    "kwrap/type.h"
#include    "kwrap/flag.h"
#include    "kwrap/semaphore.h"
#include    <kwrap/spinlock.h>
#include    <kwrap/cpu.h>
#include    <kwrap/nvt_type.h>




#include <plat-na51055/nvt-sramctl.h>

#include "sde_dbg.h"
#include "sde_drv.h"

//#include "comm/hwclock.h"

#define pmc_turnonPower(a)
#define pmc_turnoffPower(a)

//----------------------------------------------
//For uitron.
#define dma_getNonCacheAddr(parm) parm //wrong
#define dma_getPhyAddr(parm) parm  //wrong
//#define dma_flushWriteCache(parm,parm2)
//#define dma_flushReadCache(parm,parm2)
//#define dma_flushReadCacheWidthNEQLineOffset(parm,parm2)
//----------------------------------------------



//#define pll_enableClock(x)
//#define pll_disableClock(x)
//#define pll_setClockRate(x,y)
//#define pll_getPLLEn(x,y)
//#define pll_setPLLEn(x)


//#define CHKPNT                          printk(KERN_INFO  "\033[37mCHK: %d, %s\033[0m\r\n",__LINE__,__func__)
extern UINT32  _SDE_REG_BASE_ADDR;
#define SDE_REG_ADDR(ofs)        (_SDE_REG_BASE_ADDR+(ofs))
#define SDE_SETREG(ofs, value)   iowrite32(value, (void *)(_SDE_REG_BASE_ADDR + ofs))
#define SDE_GETREG(ofs)          ioread32((void *)(_SDE_REG_BASE_ADDR + ofs))


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

#include "sde_dbg.h"

#define pmc_turnonPower(a)
#define pmc_turnoffPower(a)


#define _SDE_REG_BASE_ADDR       (IOADDR_SDE_REG_BASE)

#define SDE_REG_ADDR(ofs)        (_SDE_REG_BASE_ADDR+(ofs))
#define SDE_SETREG(ofs, value)   OUTW((_SDE_REG_BASE_ADDR + ofs), value)
#define SDE_GETREG(ofs)          INW(_SDE_REG_BASE_ADDR + ofs)


//#define CHKPNT    DBG_ERR("\033[37mCHK: %d, %s\033[0m\r\n",__LINE__,__func__)


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


#define __MODULE__    dal_sde
#define __DBGLVL__    2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*" // *=All, [mark1]=CustomClass
#include "DebugModule.h"


#define _SDE_REG_BASE_ADDR       (IOADDR_SDE_REG_BASE)
#define SDE_REG_ADDR(ofs)        (_SDE_REG_BASE_ADDR+(ofs))
#define SDE_SETREG(ofs, value)   OUTW((_SDE_REG_BASE_ADDR + ofs), value)
#define SDE_GETREG(ofs)          INW(_SDE_REG_BASE_ADDR + ofs)

//=========================================================================
#else

#endif


extern ID     g_SdeLockStatus;



//-------------------------------------------------------------------------
#define SDE_CLOCK_240        240
#define SDE_CLOCK_320        320
#define SDE_CLOCK_360        360
#define SDE_CLOCK_480        480







#define FLGPTN_SDE_FRAMEEND     FLGPTN_BIT(0)
#define FLGPTN_SDE_LLEND        FLGPTN_BIT(1)
#define FLGPTN_SDE_LL_ERROR     FLGPTN_BIT(2)
#define FLGPTN_SDE_JOB_END      FLGPTN_BIT(3)


extern UINT32 _sde_reg_io_base[2];

extern ER sde_platform_flg_clear(FLGPTN flg);

extern ER sde_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg);

extern ER sde_platform_flg_set(FLGPTN flg);

extern ER sde_platform_sem_wait(VOID);

extern ER sde_platform_sem_signal(VOID);

extern VOID sde_platform_spin_lock(VOID);

extern VOID sde_platform_spin_unlock(VOID);

extern VOID sde_platform_int_enable(VOID);

extern VOID sde_platform_int_disable(VOID);

extern VOID sde_platform_enable_clk(VOID);

extern VOID sde_platform_disable_clk(VOID);

extern VOID sde_platform_prepare_clk(VOID);

extern VOID sde_platform_unprepare_clk(VOID);

extern VOID sde_platform_disable_sram_shutdown(VOID);

extern VOID sde_platform_enable_sram_shutdown(VOID);

extern ER sde_platform_set_clk_rate(UINT32 source_clk);

extern UINT32 sde_platform_get_clk_rate(VOID);

extern UINT32 sde_platform_va2pa(UINT32 addr);


extern UINT32 sde_platform_dma_is_cacheable(UINT32 addr);

extern UINT32 sde_platform_dma_flush_mem2dev_for_video_mode(UINT32 addr, UINT32 size);

extern UINT32 sde_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size);


extern UINT32 sde_platform_dma_flush_dev2mem_width_neq_loff(UINT32 addr, UINT32 size);

extern UINT32 sde_platform_dma_flush_dev2mem_for_video_mode(UINT32 addr, UINT32 size);

extern UINT32 sde_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size);

extern UINT32 sde_platform_dma_post_flush_dev2mem(UINT32 addr, UINT32 size);


#if !(defined __UITRON || defined __ECOS)
extern VOID sde_isr(VOID);
#if defined __FREERTOS
extern VOID sde_platform_create_resource(VOID);
#else
extern VOID sde_platform_create_resource(SDE_INFO *pmodule_info);
#endif
extern VOID sde_platform_release_resource(VOID);
#endif



#ifdef __cplusplus
}
#endif


#endif // _IME_PM_REG_

