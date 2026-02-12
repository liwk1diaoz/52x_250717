#ifndef __DIS_ALG_PLATFORM_H__
#define __DIS_ALG_PLATFORM_H__


#if (defined __UITRON || defined __ECOS)
#define __MODULE__    kdrv_dis
#define __DBGLVL__    2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*" // *=All, [mark1]=CustomClass
#include "DebugModule.h"
#include "frammap/frammap_if.h"
#include "interrupt.h"
#include "top.h"
#include "dma.h"
#include "SysKer.h"
#include "Type.h"

#elif defined(__FREERTOS)
#include "pll_protected.h"
#include "interrupt.h"
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "rcw_macro.h"
#include "kwrap/error_no.h"
#include "dma_protected.h"
#include "FileSysTsk.h"
#if defined(_BSP_NA51055_)
#include "rtos_na51055/nvt-sramctl.h"
#include "rtos_na51055/top.h"
#endif
#define EXPORT_SYMBOL(a)
#define MODULE_AUTHOR(a)
#define MODULE_LICENSE(a)

#elif defined(__LINUX)
#include <comm/nvtmem.h>
#include <mach/rcw_macro.h>

#include <plat-na51055/nvt-sramctl.h>
#include <plat-na51055/top.h>

#endif

#include "kwrap/debug.h"
#include "kdrv_type.h"
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/cpu.h"
#include "kwrap/task.h"
#include "kwrap/platform.h"
#include "kwrap/util.h"
#include "kwrap/cpu.h"
#include "kwrap/mem.h"
#include "kwrap/file.h"


UINT32 dis_alg_platform_va2pa(UINT32 addr);
UINT32 dis_alg_platform_pa2va_remap(UINT32 pa, UINT32 sz, UINT8 is_mem2dev);
VOID dis_alg_platform_pa2va_unmap(UINT32 va, UINT32 pa);

#endif
