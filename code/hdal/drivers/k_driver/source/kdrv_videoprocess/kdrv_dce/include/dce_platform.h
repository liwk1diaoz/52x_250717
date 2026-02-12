#ifndef __DCE_PLATFORM_H_
#define __DCE_PLATFORM_H__

#if (defined __UITRON || defined __ECOS)
#include "interrupt.h"
#include "pll.h"
#include "top.h"
#include "DrvCommon.h"
#include "nvtDrvProtected.h"
#include "pad.h"
#include "gpio.h"
#include "dce_lib.h"
//#include "dce_api.h" //for log num

//modified:SC
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include "Type.h"
//#include "ErrorNo.h"
//#include "interrupt.h"
//#include "pll_protected.h"
//#include "top.h"
//#include "Utility.h"
//#include "dma.h"
//#include "ist.h"
//#include "uart.h"

#define _DCE_REG_BASE_ADDR       IOADDR_DCE_REG_BASE
#define DCE_REG_ADDR(ofs)        (_DCE_REG_BASE_ADDR+(ofs))
#define DCE_SETREG(ofs, value)   OUTW((_DCE_REG_BASE_ADDR + ofs), value)
#define DCE_GETREG(ofs)          INW(_DCE_REG_BASE_ADDR + ofs)

#define _CNN2_REG_BASE_ADDR       (_DCE_REG_BASE_ADDR - 0x00c20000 + 0x00d60000)  //IOADDR_CNN2_REG_BASE (not ready)
#define CNN2_SETREG(ofs, value)   OUTW((_CNN2_REG_BASE_ADDR + ofs), value)
#define CNN2_GETREG(ofs)          INW(_CNN2_REG_BASE_ADDR + ofs)
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
#include "dce_dbg.h"
#include <stdio.h>
#include "malloc.h"
#include <kwrap/task.h>
#include "kdrv_builtin/kdrv_ipp_builtin.h"
#include "top.h"

//#include "dce_api.h" //for log num

#define _DCE_REG_BASE_ADDR       IOADDR_DCE_REG_BASE
#define DCE_REG_ADDR(ofs)        (_DCE_REG_BASE_ADDR+(ofs))
#define DCE_SETREG(ofs, value)   OUTW((_DCE_REG_BASE_ADDR + ofs), value)
#define DCE_GETREG(ofs)          INW(_DCE_REG_BASE_ADDR + ofs)

#define _CNN2_REG_BASE_ADDR       (_DCE_REG_BASE_ADDR - 0x00c20000 + 0x00d60000)  //IOADDR_CNN2_REG_BASE (not ready)
#define CNN2_SETREG(ofs, value)   OUTW((_CNN2_REG_BASE_ADDR + ofs), value)
#define CNN2_GETREG(ofs)          INW(_CNN2_REG_BASE_ADDR + ofs)
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
#include <plat/top.h>

#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/kdrv_ipp_builtin.h"

#include "dce_drv.h"
#include "dce_dbg.h"

//modified:SC
//#include <linux/dma-mapping.h> // header file Dma(cache handle)
//#include "dce_main.h"
//#include "dce_api.h" //for log num

extern UINT32 _DCE_REG_BASE_ADDR;
#define DCE_REG_ADDR(ofs)        (_DCE_REG_BASE_ADDR+(ofs))
#define DCE_SETREG(ofs, value)   kdrv_ipp_set_reg(KDRV_IPP_BUILTIN_DCE, _DCE_REG_BASE_ADDR, ofs, value)
#define DCE_GETREG(ofs)          ioread32((void *)(_DCE_REG_BASE_ADDR + ofs))

#define _CNN2_REG_BASE_ADDR       (_DCE_REG_BASE_ADDR - 0x00c20000 + 0x00d60000)
#define CNN2_SETREG(ofs, value)   iowrite32(value,(void *)(_CNN2_REG_BASE_ADDR + ofs))
#define CNN2_GETREG(ofs)          ioread32((void *)(_CNN2_REG_BASE_ADDR + ofs))
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

#define KDRV_DCE_LOG_NUM 128
#define KDRV_DCE_LOG_MSG_LENGTH 64

#define KDRV_DCE_CEIL_64(n) ((((n) + 63) >> 6) << 6) // for memory 64 byte alignment

#if !defined (CONFIG_NVT_SMALL_HDAL)
extern char g_kdrv_dce_ctrl_api_log[KDRV_DCE_LOG_NUM][KDRV_DCE_LOG_MSG_LENGTH];
extern int log_idx;
extern unsigned int total_log_idx;
#endif

// interrupt flag
#define FLGPTN_DCE_FRAMESTART   FLGPTN_BIT(0)
#define FLGPTN_DCE_FRAMEEND     FLGPTN_BIT(1)
#define FLGPTN_DCE_CSTPEND      FLGPTN_BIT(2)
#define FLGPTN_DCE_OVFL1        FLGPTN_BIT(3)
#define FLGPTN_DCE_OVFL2        FLGPTN_BIT(4)
#define FLGPTN_DCE_LL_DONE      FLGPTN_BIT(5)
#define FLGPTN_DCE_LL_JOB_END   FLGPTN_BIT(6)

extern BOOL dce_ctrl_flow_to_do;

extern ER dce_platform_sem_wait(void);
extern ER dce_platform_sem_signal(void);
extern ER dce_platform_flg_clear(FLGPTN flg);
extern ER dce_platform_flg_set(FLGPTN flg);
extern ER dce_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg);
extern unsigned long dce_platform_spin_lock(void);
extern void dce_platform_spin_unlock(unsigned long flag);
extern void dce_platform_int_enable(void);
extern void dce_platform_int_disable(void);
extern void dce_platform_set_pinmux(UINT32 id, BOOL en);
extern int dce_platform_is_err_clk(void);
extern void dce_platform_disable_clk(void);
extern void dce_platform_enable_clk(void);
extern void dce_platform_prepare_clk(void);
extern void dce_platform_unprepare_clk(void);
extern void dce_platform_set_clk_rate(UINT32 source_clk);
extern UINT32 dce_platform_get_clk_rate(void);
#if 0
extern UINT32 dce_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size);
extern UINT32 dce_platform_dma_flush_dev2mem_width_neq_lofs(UINT32 addr, UINT32 size);
extern UINT32 dce_platform_dma_flush_dev2mem_for_video_mode(UINT32 addr, UINT32 size);
extern UINT32 dce_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size);
#endif
extern UINT32 dce_platform_va2pa(UINT32 addr);
extern void dce_platform_disable_sram_shutdown(void);
extern void dce_platform_enable_sram_shutdown(void);
extern void cnn2_platform_disable_sram_shutdown(void);

extern void dce_platform_set_init_status(BOOL set_status);
extern BOOL dce_platform_get_init_status(void);


#if !(defined __UITRON || defined __ECOS)
extern void dce_isr(void);
#if defined __FREERTOS
extern void dce_platform_create_resource(void);
#else
extern void dce_platform_create_resource(DCE_INFO *pmodule_info);
#endif
extern void dce_platform_release_resource(void);
#endif


#endif

