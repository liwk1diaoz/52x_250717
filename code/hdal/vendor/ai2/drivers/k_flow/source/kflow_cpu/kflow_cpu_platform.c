/**
	@brief Source file of vendor net flow sample.

	@file kflow_cpu_platform.c

	@ingroup kflow ai net mem map

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include"kflow_cpu/kflow_cpu_platform.h"

#if defined (__LINUX)
#include <linux/vmalloc.h>
#define mem_alloc	vmalloc
#define mem_free	vfree
#else
#include <malloc.h>
#define mem_alloc	malloc
#define mem_free	free
#endif

void* nvt_ai_mem_alloc_cpu(unsigned size)
{
	return mem_alloc(size);
}
//EXPORT_SYMBOL(nvt_ai_mem_alloc);

void nvt_ai_mem_free_cpu(void* addr)
{
	mem_free(addr);
}
//EXPORT_SYMBOL(nvt_ai_mem_free);