#include "dis_alg_platform.h"


UINT32 dis_alg_platform_va2pa(UINT32 addr)
{
	VOS_ADDR rt;
    rt = vos_cpu_get_phy_addr(addr);
	if (rt == VOS_ADDR_INVALID) {
		DBG_ERR("addr(0x%.8x) conv fail\r\n", (int)addr);
		return 0;
	}
	return rt;

}

UINT32 dis_alg_platform_pa2va_remap(UINT32 pa, UINT32 sz, UINT8 is_mem2dev)
{
    UINT32 va = 0;

#if defined(__FREERTOS)
    va = pa;
#else

    if (sz == 0) {
        return va;
    }
    if (pfn_valid(__phys_to_pfn(pa))) {
        va = (UINT32)__va(pa);
    } else {
        va = (UINT32)ioremap(pa, PAGE_ALIGN(sz));
	}
	//fmem_dcache_sync((UINT32 *)va, sz, DMA_BIDIRECTIONAL);
	if (is_mem2dev == 1) {
    	vos_cpu_dcache_sync(va, sz, VOS_DMA_TO_DEVICE); ///< cache clean - output to engine's input
	} else if (is_mem2dev == 0) {
		vos_cpu_dcache_sync(va, sz, VOS_DMA_FROM_DEVICE); ///< cache clean - output from engine
	} else {
		//Do not cache sync.
	}
#endif
    return va;
}

VOID dis_alg_platform_pa2va_unmap(UINT32 va, UINT32 pa)
{
#if defined(__FREERTOS)
	return;
#else
    if (va == 0) {
        return;
    }

    if (!pfn_valid(__phys_to_pfn(pa))) {
        iounmap((VOID *)va);
    }
#endif
}