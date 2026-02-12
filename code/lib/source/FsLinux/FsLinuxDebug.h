/**
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       FsLinuxID.h
    @ingroup    mISYSFsLinux

    @brief

    @note       Nothing.

    @date       2014/06/13
*/

/** \addtogroup mISYSFsLinux */
//@{

#ifndef _FSLINUXDEBUG_H
#define _FSLINUXDEBUG_H

#ifndef __LINUX_USER
#include <string.h>
#include <kwrap/cpu.h>
#include <kwrap/dev.h>
#include <kwrap/ioctl.h>
#include <kwrap/mailbox.h>
#include <kwrap/mem.h>
#endif

#include "kwrap/error_no.h"
#include <kwrap/file.h>
#include <kwrap/flag.h>
#include <kwrap/perf.h>
#include <kwrap/semaphore.h>
#include <kwrap/stdio.h>
#include <kwrap/task.h>
#include "kwrap/type.h"
#include <kwrap/util.h>

#define THIS_DBGLVL         NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          FsLinux
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#define __DBGFLT__          "[API]"
//#define __DBGFLT__          "[BUF]"
//#define __DBGFLT__          "[IPC]"
//#define __DBGFLT__          "[TSK]"
//#define __DBGFLT__          "[UTIL]"
#include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////
extern unsigned int FsLinux_debug_level;

#endif //_FSLINUXDEBUG_H