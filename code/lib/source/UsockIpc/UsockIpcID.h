/**
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       UsockIpcID.h
    @ingroup    mISYSFileSysIPC

    @brief

    @note       Nothing.

    @date       2014/06/13
*/

/** \addtogroup mISYSFileSysIPC */
//@{

#ifndef _USOCKIPCID_H
#define _USOCKIPCID_H
#if (USE_IPC)

#include "SysKer.h"

#define USOCKIPC_FLGBIT_TSK_READY      FLGPTN_BIT(0)
#define USOCKIPC_FLGBIT_SYSREQ_GOTACK  FLGPTN_BIT(1)

extern UINT32 _SECTION(".kercfg_data") USOCKIPC_FLG_ID_0;
extern UINT32 _SECTION(".kercfg_data") USOCKIPC_SEM_ID_0;
extern UINT32 _SECTION(".kercfg_data") USOCKIPC_TSK_ID_0;
extern UINT32 _SECTION(".kercfg_data") USOCKIPC_FLG_ID_1;
extern UINT32 _SECTION(".kercfg_data") USOCKIPC_SEM_ID_1;
extern UINT32 _SECTION(".kercfg_data") USOCKIPC_TSK_ID_1;
#endif
#endif //_USOCKIPCID_H
