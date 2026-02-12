/**
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    @file       EthsockIpcID.h
    @ingroup    mIEthsockIpcAPI

    @brief

    @note       Nothing.

    @date       2019/05/06
*/

/** \addtogroup mIEthsockIpcAPI */
//@{

#ifndef _ETHSOCKIPCID_H
#define _ETHSOCKIPCID_H

//#include "SysKer.h"
#include "EthsockIpcInt.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"


#define ETHSOCKIPC_FLGBIT_TSK_READY      FLGPTN_BIT(0)
#define ETHSOCKIPC_FLGBIT_SYSREQ_GOTACK  FLGPTN_BIT(1)

extern ID _SECTION(".kercfg_data") ETHSOCKIPC_FLG_ID_0;
extern ID _SECTION(".kercfg_data") ETHSOCKIPC_SEM_ID_0;
extern THREAD_HANDLE _SECTION(".kercfg_data") ETHSOCKIPC_TSK_ID_0;
extern ID _SECTION(".kercfg_data") ETHSOCKIPC_FLG_ID_1;
extern ID _SECTION(".kercfg_data") ETHSOCKIPC_SEM_ID_1;
extern THREAD_HANDLE _SECTION(".kercfg_data") ETHSOCKIPC_TSK_ID_1;
extern ID _SECTION(".kercfg_data") ETHSOCKIPC_FLG_ID_2;
extern ID _SECTION(".kercfg_data") ETHSOCKIPC_SEM_ID_2;
extern THREAD_HANDLE _SECTION(".kercfg_data") ETHSOCKIPC_TSK_ID_2;

#endif //_ETHSOCKIPCID_H

