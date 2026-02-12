#ifndef __IPE_PLATFORM_H_
#define __IPE_PLATFORM_H__


#include "plat/top.h"


#define IPE_ALIGN_FLOOR(value, base)  ((value) & ~((base)-1))                   ///< Align Floor
#define IPE_ALIGN_ROUND(value, base)  IPE_ALIGN_FLOOR((value) + ((base)/2), base)   ///< Align Round
#define IPE_ALIGN_CEIL(value, base)   IPE_ALIGN_FLOOR((value) + ((base)-1), base)   ///< Align Ceil

#define IPE_ALIGN_ROUND_64(a)       IPE_ALIGN_ROUND(a, 64)  ///< Round Off to 64
#define IPE_ALIGN_ROUND_32(a)       IPE_ALIGN_ROUND(a, 32)  ///< Round Off to 32
#define IPE_ALIGN_ROUND_16(a)       IPE_ALIGN_ROUND(a, 16)  ///< Round Off to 16
#define IPE_ALIGN_ROUND_8(a)        IPE_ALIGN_ROUND(a, 8)   ///< Round Off to 8
#define IPE_ALIGN_ROUND_4(a)        IPE_ALIGN_ROUND(a, 4)   ///< Round Off to 4

#define IPE_ALIGN_CEIL_64(a)        IPE_ALIGN_CEIL(a, 64)   ///< Round Up to 64
#define IPE_ALIGN_CEIL_32(a)        IPE_ALIGN_CEIL(a, 32)   ///< Round Up to 32
#define IPE_ALIGN_CEIL_16(a)        IPE_ALIGN_CEIL(a, 16)   ///< Round Up to 16
#define IPE_ALIGN_CEIL_8(a)         IPE_ALIGN_CEIL(a, 8)    ///< Round Up to 8
#define IPE_ALIGN_CEIL_4(a)         IPE_ALIGN_CEIL(a, 4)    ///< Round Up to 4

#define IPE_ALIGN_FLOOR_64(a)       IPE_ALIGN_FLOOR(a, 64)  ///< Round down to 64
#define IPE_ALIGN_FLOOR_32(a)       IPE_ALIGN_FLOOR(a, 32)  ///< Round down to 32
#define IPE_ALIGN_FLOOR_16(a)       IPE_ALIGN_FLOOR(a, 16)  ///< Round down to 16
#define IPE_ALIGN_FLOOR_8(a)        IPE_ALIGN_FLOOR(a, 8)   ///< Round down to 8
#define IPE_ALIGN_FLOOR_4(a)        IPE_ALIGN_FLOOR(a, 4)   ///< Round down to 4



#if (defined __UITRON || defined __ECOS)
#include "interrupt.h"
#include "pll.h"
#include "top.h"
#include "DrvCommon.h"
#include "nvtDrvProtected.h"
#include "pad.h"
#include "gpio.h"
#include "ipe_lib.h"

#define _IPE_REG_BASE_ADDR       IOADDR_IPE_REG_BASE
#define IPE_REG_ADDR(ofs)        (_IPE_REG_BASE_ADDR+(ofs))
#define IPE_SETREG(ofs, value)   OUTW((_IPE_REG_BASE_ADDR + ofs), (value))
#define IPE_GETREG(ofs)          INW(_IPE_REG_BASE_ADDR + ofs)

#elif defined(__FREERTOS)

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
#include "ipe_dbg.h"
#include <stdio.h>
#include "malloc.h"
#include <kwrap/task.h>
#include "kdrv_builtin/kdrv_ipp_builtin.h"

#define _IPE_REG_BASE_ADDR       IOADDR_IPE_REG_BASE
#define IPE_REG_ADDR(ofs)        (_IPE_REG_BASE_ADDR+(ofs))
#define IPE_SETREG(ofs, value)   OUTW((_IPE_REG_BASE_ADDR + ofs), (value))
#define IPE_GETREG(ofs)          INW(_IPE_REG_BASE_ADDR + ofs)


#else

#include <linux/clk.h>
#include <linux/spinlock.h>
#include <mach/rcw_macro.h>
#include <mach/fmem.h>
#include <plat/nvt-sramctl.h>
#include "kwrap/type.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include <kwrap/spinlock.h>
#include <kwrap/cpu.h>


#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/kdrv_ipp_builtin.h"

#include "ipe_drv.h"
#include "ipe_dbg.h"

extern UINT32 _IPE_REG_BASE_ADDR;
#define IPE_REG_ADDR(ofs)        (_IPE_REG_BASE_ADDR+(ofs))
#define IPE_SETREG(ofs, value)   kdrv_ipp_set_reg(KDRV_IPP_BUILTIN_IPE, _IPE_REG_BASE_ADDR, ofs, value)
#define IPE_GETREG(ofs)          ioread32((void *)(_IPE_REG_BASE_ADDR + ofs))

extern struct clk *ipe_clk[MODULE_CLK_NUM];
extern struct clk *ipe_pclk[MODULE_CLK_NUM];

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

#define FLGPTN_IPE_FRAMEEND     FLGPTN_BIT(0)
#define FLGPTN_IPE_STRPEND      FLGPTN_BIT(1)
#define FLGPTN_IPE_FRMSTART     FLGPTN_BIT(2)
#define FLGPTN_IPE_YCOUTEND     FLGPTN_BIT(3)
#define FLGPTN_IPE_DMAIN0END    FLGPTN_BIT(4)
#define FLGPTN_IPE_DMAIN1END    FLGPTN_BIT(5)
#define FLGPTN_IPE_GAMERR       FLGPTN_BIT(6)
#define FLGPTN_IPE_3DCCERR      FLGPTN_BIT(7)
#define FLGPTN_IPE_VAOUTEND     FLGPTN_BIT(8)
#define FLGPTN_IPE_LL_JOB_END   FLGPTN_BIT(9)
#define FLGPTN_IPE_LL_DONE      FLGPTN_BIT(10)

extern BOOL ipe_ctrl_flow_to_do;
extern BOOL fw_ipe_power_saving_en;

extern ER ipe_platform_sem_wait(void);
extern ER ipe_platform_sem_signal(void);
extern ER ipe_platform_flg_clear(FLGPTN flg);
extern ER ipe_platform_flg_set(FLGPTN flg);
extern ER ipe_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg);
extern unsigned long ipe_platform_spin_lock(void);
extern void ipe_platform_spin_unlock(unsigned long flag);
extern void ipe_platform_int_enable(void);
extern void ipe_platform_int_disable(void);
extern void ipe_platform_set_pinmux(UINT32 id, BOOL en);
extern int ipe_platform_is_err_clk(void);
extern void ipe_platform_disable_clk(void);
extern void ipe_platform_enable_clk(void);
extern void ipe_platform_prepare_clk(void);
extern void ipe_platform_unprepare_clk(void);
extern void ipe_platform_set_clk_rate(UINT32 source_clk);
extern UINT32 ipe_platform_get_clk_rate(void);
extern UINT32 ipe_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size);
extern UINT32 ipe_platform_dma_flush_dev2mem_video_mode(UINT32 addr, UINT32 size);
extern UINT32 ipe_platform_dma_flush_dev2mem_width_neq_lofs(UINT32 addr, UINT32 size);
extern UINT32 ipe_platform_dma_post_flush_dev2mem(UINT32 addr, UINT32 size);
extern UINT32 ipe_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size);
extern UINT32 ipe_platform_va2pa(UINT32 addr);
extern void ipe_platform_disable_sram_shutdown(void);
extern void ipe_platform_enable_sram_shutdown(void);

extern void ipe_platform_set_init_status(BOOL set_status);
extern BOOL ipe_platform_get_init_status(void);


#if !(defined __UITRON || defined __ECOS)
extern void ipe_isr(void);
#if defined __FREERTOS
extern void ipe_platform_create_resource(void);
#else
extern void ipe_platform_create_resource(IPE_INFO *pmodule_info);
#endif
extern void ipe_platform_release_resource(void);
#endif


#endif

