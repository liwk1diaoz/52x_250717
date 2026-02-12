/**
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    @file       EthsockCliIpcID.h
    @ingroup    mISYSEthsockCliIpc

    @brief

    @note       Nothing.

    @date       2019/05/06
*/

/** \addtogroup mISYSEthsockCliIpc */
//@{

#ifndef _ETHSOCKCLIIPCID_H
#define _ETHSOCKCLIIPCID_H

//#include "SysKer.h"
#include "EthsockCliIpcInt.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"

#define ETHSOCKCLIIPC_FLGBIT_TSK_READY      FLGPTN_BIT(0)
#define ETHSOCKCLIIPC_FLGBIT_SYSREQ_GOTACK  FLGPTN_BIT(1)
#define ETHSOCKCLIIPC_FLGBIT_SYSREQ_TIMEOUT  FLGPTN_BIT(2)

extern ID _SECTION(".kercfg_data") ETHSOCKCLIIPC_FLG_ID_0[MAX_ETH_PATH_NUM];
extern ID _SECTION(".kercfg_data") ETHSOCKCLIIPC_SEM_ID_0[MAX_ETH_PATH_NUM];
extern THREAD_HANDLE _SECTION(".kercfg_data") ETHSOCKCLIIPC_TSK_ID_0[MAX_ETH_PATH_NUM];

extern ID _SECTION(".kercfg_data") ETHSOCKCLIIPC_FLG_ID_1[MAX_ETH_PATH_NUM];
extern ID _SECTION(".kercfg_data") ETHSOCKCLIIPC_SEM_ID_1[MAX_ETH_PATH_NUM];
extern THREAD_HANDLE _SECTION(".kercfg_data") ETHSOCKCLIIPC_TSK_ID_1[MAX_ETH_PATH_NUM];

extern ID _SECTION(".kercfg_data") ETHSOCKCLIIPC_FLG_ID_2[MAX_ETH_PATH_NUM];
extern ID _SECTION(".kercfg_data") ETHSOCKCLIIPC_SEM_ID_2[MAX_ETH_PATH_NUM];
extern THREAD_HANDLE _SECTION(".kercfg_data") ETHSOCKCLIIPC_TSK_ID_2[MAX_ETH_PATH_NUM];


#endif //_ETHSOCKCLIIPCID_H
