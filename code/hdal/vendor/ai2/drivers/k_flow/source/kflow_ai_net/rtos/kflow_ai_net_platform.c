/**
	@brief Source file of vendor net flow sample.

	@file kflow_ai_net_platform.c

	@ingroup kflow ai net mem map

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/cpu.h"

#include "kflow_ai_net/kflow_ai_net_platform.h"

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#else
#include "mach/fmem.h" //for fmem_lookup_pa, PAGE_ALIGN
#include <asm/io.h>  /* for ioremap and iounmap, pfn_valid, __phys_to_pfn */
#endif

UINT32 nvt_ai_va2pa(UINT32 addr)
{
	if (addr == 0) {
		return addr;
	} else {
#if defined(_BSP_NA51055_) ||  defined(_BSP_NA51089_)
		return vos_cpu_get_phy_addr(addr);   //--> why this cause random fail or hang in nue2 layer of resnet18, fc_slice5
#else
		return fmem_lookup_pa(addr);
#endif
	}
}

UINT32 nvt_ai_pa2va_remap(UINT32 pa, UINT32 sz)
{
	UINT32 va = 0;
	if (sz == 0) {
		return va;
	}
#if defined(__LINUX)
	if (pfn_valid(__phys_to_pfn(pa))) {
		va = (UINT32)__va(pa);
	} else {
		va = (UINT32)ioremap(pa, PAGE_ALIGN(sz));
	}
#else
	va = pa;
#endif
	//fmem_dcache_sync((UINT32 *)va, sz, DMA_BIDIRECTIONAL);
	vos_cpu_dcache_sync(va, sz, VOS_DMA_TO_DEVICE); ///< cache clean - output to engine's input
	return va;
}

VOID nvt_ai_pa2va_unmap(UINT32 va, UINT32 pa)
{
	if (va == 0) {
		return;
	}
#if defined(_BSP_NA51055_) ||  defined(_BSP_NA51089_)
#else
	if (!pfn_valid(__phys_to_pfn(pa))) {
		iounmap((VOID *)va);
	}
#endif
}

#if defined (__LINUX)
#include <linux/vmalloc.h>
#define mem_alloc	vmalloc
#define mem_free	vfree
#else
#include <malloc.h>
#define mem_alloc	malloc
#define mem_free	free
#endif

VOID* nvt_ai_mem_alloc(UINT32 size)
{
	return mem_alloc(size);
}

VOID nvt_ai_mem_free(VOID* addr)
{
	mem_free(addr);
}