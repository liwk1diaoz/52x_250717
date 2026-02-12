/**
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       UsockCliIpcID.h
    @ingroup    mISYSFileSysIPC

    @brief

    @note       Nothing.

    @date       2014/06/13
*/

/** \addtogroup mISYSFileSysIPC */
//@{

#ifndef _USOCKCLIIPCDEBUG_H
#define _USOCKCLIIPCDEBUG_H

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UsockCliIpc
#define __DBGLVL__          2 //0=OFF, 1=ERROR, 2=TRACE
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#define __DBGFLT__          "[API]"
//#define __DBGFLT__          "[MSG]"
//#define __DBGFLT__          "[TSK]"
//#define __DBGFLT__          "[UTIL]"
#include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////

#endif //_USOCKCLIIPCDEBUG_H