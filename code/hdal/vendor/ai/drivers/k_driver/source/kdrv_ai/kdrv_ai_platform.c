/**
	@brief Source file of vendor net flow sample.

	@file net_flow_platform.c

	@ingroup net_flow_platform

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "kwrap/cpu.h"
#include "kdrv_ai_platform.h"
#include "../../../k_driver/include/kdrv_ai_version.h"


#if defined(__LINUX)
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include "mach/fmem.h"
#include <asm/io.h>  /* for ioremap and iounmap */
#include <plat-na51055/top.h>
#include "kflow_common/nvtmpp.h"
#elif defined(__FREERTOS)
#include "rtos_na51055/nvt-sramctl.h"
#include "rtos_na51055/top.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#else
#endif


UINT32 kdrv_ai_pa2va_remap(UINT32 pa, UINT32 sz)
{
	UINT32 va = 0;
#if defined(__LINUX)
	if (sz == 0) {
		return va;
	}
	if (pfn_valid(__phys_to_pfn(pa))) {
		va = (UINT32)__va(pa);
	} else {
		va = (UINT32)nvtmpp_sys_pa2va(pa);
		if (va == 0) {
			va = (UINT32)ioremap(pa, PAGE_ALIGN(sz));
		}
	}
#else
	va = pa;
#endif
	if (va > 0) {
		vos_cpu_dcache_sync(va, sz, VOS_DMA_TO_DEVICE);
	}
	
	return va;
}

UINT32 kdrv_ai_pa2va_remap_wo_sync(UINT32 pa, UINT32 sz)
{
	UINT32 va = 0;
#if defined(__LINUX)
	if (sz == 0) {
		return va;
	}
	if (pfn_valid(__phys_to_pfn(pa))) {
		va = (UINT32)__va(pa);
	} else {
		va = (UINT32)nvtmpp_sys_pa2va(pa);
		if (va == 0) {
			va = (UINT32)ioremap(pa, PAGE_ALIGN(sz));
		}
	}
#else
	va = pa;
#endif

	return va;
}

VOID kdrv_ai_pa2va_unmap(UINT32 va, UINT32 pa)
{
	if (va == 0) {
		return;
	}
#if defined(__LINUX)
	if (!pfn_valid(__phys_to_pfn(pa))) {
		UINT32 tmp_va = (UINT32)nvtmpp_sys_pa2va(pa);
		if (tmp_va == 0) {
			iounmap((VOID *)va);
		}
	}
#endif
}

#if defined(__LINUX)
MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("AI kdrv");
MODULE_LICENSE("GPL");
MODULE_VERSION(KDRV_AI_IMPL_VERSION);
#endif
