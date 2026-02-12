#ifndef _MSDCNVT_NVT_H
#define _MSDCNVT_NVT_H

#if defined (__UITRON)
#include "SysKer.h"
#include "uart.h"
#include "DxStorage.h"
#include "DxCommon.h"
#include "StrgDef.h"
#include <string.h>
#include "dma_protected.h"
#include "NvtIpcAPI.h"
#include "Utility.h"
#include "SysKer.h"
#include <string.h>
#include "NvtVerInfo.h"
#include "msdcnvt_api.h"
#include "msdcnvt_callback.h"
#if defined(_NETWORK_ON_CPU1_)
#include <cyg/msdcnvt/msdcnvt.h>
#endif
#else
#include <kwrap/semaphore.h>
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/dma-direction.h>
#include <asm/cacheflush.h>
#include <asm/io.h>  /* for ioremap and iounmap */
#include <msdcnvt/msdcnvt_api.h>
#include <msdcnvt/msdcnvt_callback.h>
#endif

//Configure
#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER

#define __MODULE__          msdcnvt_adj
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass

#if defined (__UITRON)
#include "DebugModule.h"
#else
#include "kwrap/debug.h"
#endif
#endif
