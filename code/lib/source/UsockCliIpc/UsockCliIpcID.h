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

#ifndef _USOCKCLIIPCID_H
#define _USOCKCLIIPCID_H

#include "SysKer.h"

#define USOCKCLIIPC_FLGBIT_TSK_READY      FLGPTN_BIT(0)
#define USOCKCLIIPC_FLGBIT_SYSREQ_GOTACK  FLGPTN_BIT(1)

extern UINT32 _SECTION(".kercfg_data") USOCKCLIIPC_FLG_ID;
extern UINT32 _SECTION(".kercfg_data") USOCKCLIIPC_SEM_ID;
extern UINT32 _SECTION(".kercfg_data") USOCKCLIIPC_TSK_ID;

#endif //_USOCKCLIIPCID_H