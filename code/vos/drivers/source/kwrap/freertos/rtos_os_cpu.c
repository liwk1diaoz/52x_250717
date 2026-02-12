/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>

#define __MODULE__    rtos_cpu
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/cpu.h>

#include <FreeRTOS.h>
#include <task.h> //FreeRTOS header file

#include "freertos_ext_kdrv.h"

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#if 0 //0 (disable the check), 1 (enable the check)
#define VOS_CPU_IS_ALIGNED(val) VOS_IS_ALIGNED(val)
#else
#define VOS_CPU_IS_ALIGNED(val) 1
#endif

//VOS_ALIGN_BYTES is not the same as the real cache line size
//make sure the defined value meets OS requirements
STATIC_ASSERT(VOS_ALIGN_BYTES >= (1 << VOS_CACHE_SET_NUMBER_SHIFT));

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_cpu_debug_level = NVT_DBG_WRN;

static unsigned int vb_cache_flush_en = 0;

//make sure the address size is correct
STATIC_ASSERT(sizeof(VOS_ADDR) == sizeof((void *)0));

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
static BOOL vos_cpu_is_cache_addr(VOS_ADDR addr)
{
	return (addr >= (VOS_ADDR)VOS_CPU_DRAM_OFFSET) ? FALSE : TRUE;
}

void rtos_cpu_init(void *param)
{
}

void rtos_cpu_exit(void)
{
}


VOS_ADDR vos_cpu_get_phy_addr(VOS_ADDR vaddr)
{
	return (vaddr >= (VOS_ADDR)VOS_CPU_DRAM_OFFSET) ? (vaddr - VOS_CPU_DRAM_OFFSET) : vaddr;
}

void vos_cpu_dcache_sync(VOS_ADDR vaddr, UINT32 len, VOS_DMA_DIRECTION dir)
{
	if (dir <= VOS_DMA_NONE) {
		if (!VOS_CPU_IS_ALIGNED(vaddr) || !VOS_CPU_IS_ALIGNED(len)) {
			DBG_ERR("vaddr(0x%X) or len(0x%X) not aligned %d\r\n", vaddr, len, VOS_ALIGN_BYTES);
			return;
		}
	}

	switch (dir) {
	case VOS_DMA_FROM_DEVICE:
	case VOS_DMA_FROM_DEVICE_NON_ALIGN:
		if (vos_cpu_is_cache_addr((vaddr)) == TRUE) {
			dma_flushReadCacheWithoutCheck(vaddr, len);
		}
		break;

	case VOS_DMA_TO_DEVICE:
	case VOS_DMA_TO_DEVICE_NON_ALIGN:
		if (vos_cpu_is_cache_addr(vaddr) == TRUE) {
			dma_flushWriteCacheWithoutCheck(vaddr, len);
		} else {
			__asm__ __volatile__("dsb\n\t");
		}
		(*((volatile UINT32 *)(0x60000000)));
		break;

	case VOS_DMA_BIDIRECTIONAL:
	case VOS_DMA_BIDIRECTIONAL_NON_ALIGN:
		if (vos_cpu_is_cache_addr(vaddr) == TRUE) {
			dma_flushReadCacheWidthNEQLineOffsetWithoutCheck(vaddr, len);
		} else {
			__asm__ __volatile__("dsb\n\t");
		}
		(*((volatile UINT32 *)(0x60000000)));
		break;

	default:
		DBG_ERR("Invalid dir %d\r\n", dir);
	}
}

void vos_cpu_dcache_sync_vb(VOS_ADDR vaddr, UINT32 len, VOS_DMA_DIRECTION dir)
{
	if (vb_cache_flush_en) {
		vos_cpu_dcache_sync(vaddr, len, dir);
	}
}