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

#ifndef _WIFIIPCID_H
#define _WIFIIPCID_H

#if defined(_NETWORK_ON_CPU2_)

#include "SysKer.h"

#define WIFIIPC_FLGBIT_TSK_READY      FLGPTN_BIT(0)
#define WIFIIPC_FLGBIT_SYSREQ_GOTACK  FLGPTN_BIT(1)

extern UINT32 _SECTION(".kercfg_data") WIFIIPC_FLG_ID;
extern UINT32 _SECTION(".kercfg_data") WIFIIPC_SEM_ID;
extern UINT32 _SECTION(".kercfg_data") WIFIIPC_TSK_ID;

#endif

#endif //_WIFIIPCID_H
