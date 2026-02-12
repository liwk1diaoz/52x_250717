#ifndef __IFE_PLATFORM_H_
#define __IFE_PLATFORM_H__

#if (defined __UITRON || defined __ECOS)
#include    "interrupt.h"
#include    "pll.h"
#include    "top.h"
#include    "DrvCommon.h"
#include    "nvtDrvProtected.h"
#include    "pad.h"
#include    "gpio.h"
#include    "ife_lib.h"

#define _IFE_REG_BASE_ADDR       IOADDR_IFE_REG_BASE
#define IFE_REG_ADDR(ofs)        (_IFE_REG_BASE_ADDR+(ofs))
#define IFE_SETREG(ofs, value)   OUTW((_IFE_REG_BASE_ADDR + ofs), (value))
#define IFE_GETREG(ofs)          INW(_IFE_REG_BASE_ADDR + ofs)
#elif defined(__FREERTOS)
#include    "string.h"
#include    <stdlib.h>
#include    "rcw_macro.h"
#include    "io_address.h"
#if defined(_BSP_NA51055_)
#include    "nvt-sramctl.h"
#endif
#include    "interrupt.h"
#include    "pll.h"
#include    "pll_protected.h"
#include    "dma_protected.h"
#include    "kwrap/type.h"
#include    <kwrap/semaphore.h>
#include    <kwrap/flag.h>
#include    <kwrap/spinlock.h>
#include    <kwrap/cpu.h>
#include    <kwrap/nvt_type.h>
#include    "kdrv_builtin/kdrv_ipp_builtin.h"
#include    "ife_dbg.h"

#define _IFE_REG_BASE_ADDR       IOADDR_IFE_REG_BASE
#define IFE_REG_ADDR(ofs)        (_IFE_REG_BASE_ADDR+(ofs))
#define IFE_SETREG(ofs, value)   OUTW((_IFE_REG_BASE_ADDR + ofs), (value))
#define IFE_GETREG(ofs)          INW(_IFE_REG_BASE_ADDR + ofs)
#else
#include    <linux/clk.h>
#include    <linux/spinlock.h>
#include    <mach/rcw_macro.h>
#include    <mach/fmem.h>
#include    <plat/nvt-sramctl.h>
#include    "kwrap/type.h"
#include    "kwrap/flag.h"
#include    "kwrap/semaphore.h"
#include    <kwrap/spinlock.h>
#include    <kwrap/cpu.h>
#include    <kwrap/nvt_type.h>

#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/kdrv_ipp_builtin.h"

#include    "ife_drv.h"
#include    "ife_dbg.h"

#define dma_getNonCacheAddr(addr) addr

extern UINT32 _IFE_REG_BASE_ADDR;
#define     IFE_REG_ADDR(ofs)        (_IFE_REG_BASE_ADDR+(ofs))
#define     IFE_SETREG(ofs, value)   kdrv_ipp_set_reg(KDRV_IPP_BUILTIN_IFE, _IFE_REG_BASE_ADDR, ofs, value)
#define     IFE_GETREG(ofs)          ioread32((void *)(_IFE_REG_BASE_ADDR + ofs))
#endif

#ifndef max
#define max(a, b)           (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)           (((a) < (b)) ? (a) : (b))
#endif
#ifndef clamp
#define clamp(in, lb, ub)     (((in) < (lb)) ? (lb) : (((in) > (ub)) ? (ub) : (in)))
#endif

#define FLGPTN_IFE_FRAMEEND      FLGPTN_BIT(0)
#define FLGPTN_IFE_DEC1_ERR      FLGPTN_BIT(1)
#define FLGPTN_IFE_DEC2_ERR      FLGPTN_BIT(2)
#define FLGPTN_IFE_LLEND         FLGPTN_BIT(3)
#define FLGPTN_IFE_LLERR         FLGPTN_BIT(4)
#define FLGPTN_IFE_LLERR2        FLGPTN_BIT(5)
#define FLGPTN_IFE_LLJOBEND      FLGPTN_BIT(6)
#define FLGPTN_IFE_BUFOVFL       FLGPTN_BIT(7)
#define FLGPTN_IFE_RING_BUF_ERR  FLGPTN_BIT(8)
#define FLGPTN_IFE_FRAME_ERR     FLGPTN_BIT(9)
#define FLGPTN_IFE_SIE_FRM_START FLGPTN_BIT(12)

extern BOOL ife_ctrl_flow_to_do;

extern ER ife_platform_sem_signal(void);
extern ER ife_platform_sem_wait(void);
extern ER ife_platform_flg_clear(FLGPTN flg);
extern ER ife_platform_flg_set(FLGPTN flg);
extern ER ife_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg);

extern void ife_platform_int_enable(void);
extern void ife_platform_int_disable(void);

extern void ife_platform_disable_sram_shutdown(void);
extern void ife_platform_enable_sram_shutdown(void);
extern UINT32 ife_platform_spin_lock(void);
extern void ife_platform_spin_unlock(UINT32 ife_spin_flag);

extern void ife_platform_enable_clk(void);
extern void ife_platform_disable_clk(void);
extern void ife_platform_prepare_clk(void);
extern void ife_platform_unprepare_clk(void);
extern ER   ife_platform_set_clk_rate(UINT32 uiClock);
extern UINT32 ife_platform_get_clk_rate(void);

UINT32 ife_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size, UINT32 type);
UINT32 ife_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size, UINT32 type);
UINT32 ife_platform_va2pa(UINT32 addr);

extern VOID ife_platform_set_init_status(BOOL set_status);
extern BOOL ife_platform_get_init_status(VOID);


#if !(defined __UITRON || defined __ECOS)
extern void ife_isr(void);
#if defined __FREERTOS
extern void ife_platform_create_resource(void);
#else
extern void ife_platform_create_resource(IFE_MODULE_INFO *pmodule_info);
#endif
extern void ife_platform_release_resource(void);
#endif

#endif

