/**
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    @file       EthsockCliIpcInt.h
    @ingroup    mISYSEthsockCliIpc

    @brief

    @note       Nothing.

    @date       2019/05/06
*/

/** \addtogroup mISYSEthsockCliIpc */
//@{

#ifndef _ETHSOCKCLIIPCDEBUG_H
#define _ETHSOCKCLIIPCDEBUG_H

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          EthsockCliIpc
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#define __DBGFLT__          "[API]"
//#define __DBGFLT__          "[MSG]"
//#define __DBGFLT__          "[TSK]"
//#define __DBGFLT__          "[UTIL]"
#include <kwrap/debug.h>

///////////////////////////////////////////////////////////////////////////////

#include "EthCam/EthsockCliIpcAPI.h"

typedef struct _ETHSOCKIPCCLI_CTRL {
	UINT32 ui_init_key;  ///< indicate module is initail
	BOOL   b_opened;     ///< indicate task is open
	ID     task_id;      ///< given a task id
	ID     sem_id;       ///< given a semaphore id
	ID     flag_id;      ///< given a flag id
	char   *token_path;   ///< ipc queue path
} ETHSOCKIPCCLI_CTRL;

#define MAX_ETHSOCKETCLI_NUM     (3)
#define MAX_ETH_PATH_NUM     (2)
ETHSOCKIPCCLI_CTRL *EthsockCliIpc_GetCtrl(UINT32 path_id, ETHSOCKIPCCLI_ID id);

#endif //_ETHSOCKCLIIPCDEBUG_H
