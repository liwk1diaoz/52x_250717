/**
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       WiFiIpcID.h
    @ingroup    mISYSFileSysIPC

    @brief

    @note       Nothing.

    @date       2014/06/13
*/

/** \addtogroup mISYSFileSysIPC */
//@{

#ifndef _WIFIIPCDEBUG_H
#define _WIFIIPCDEBUG_H

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          WiFiIpc
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#define __DBGFLT__          "[API]"
//#define __DBGFLT__          "[MSG]"
//#define __DBGFLT__          "[TSK]"
//#define __DBGFLT__          "[UTIL]"
#include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////

#endif //_WIFIIPCDEBUG_H
