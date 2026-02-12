#ifndef _WIFICMDPARSERINT_H_
#define _WIFICMDPARSERINT_H_
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>

#if 0
#include "SysKer.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          WifiCmdParser
#define __DBGLVL__          6 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////
#endif
extern ID _SECTION(".kercfg_data") FLG_ID_WIFICMD;
extern SEM_HANDLE _SECTION(".kercfg_data") WIFICMD_SEM_ID;
extern SEM_HANDLE _SECTION(".kercfg_data") WIFISTR_SEM_ID;

#endif //_WIFICMDPARSERINT_H_

