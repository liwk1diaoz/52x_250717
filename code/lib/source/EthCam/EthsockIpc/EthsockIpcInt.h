/**
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    @file       EthsockIpcInt.h
    @ingroup    mIEthsockIpcAPI

    @brief

    @note       Nothing.

    @date       2019/05/06
*/

/** \addtogroup mIEthsockIpcAPI */
//@{

#ifndef _ETHSOCKIPCDEBUG_H
#define _ETHSOCKIPCDEBUG_H
#include "ethsocket_ipc.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          EthsockIpc
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#define __DBGFLT__          "[API]"
//#define __DBGFLT__          "[MSG]"
//#define __DBGFLT__          "[TSK]"
//#define __DBGFLT__          "[UTIL]"
#include <kwrap/debug.h>

///////////////////////////////////////////////////////////////////////////////

#include "EthCam/EthsockIpcAPI.h"
#include "kwrap/task.h"

typedef INT32(*ETHSOCKET_CMDID_FP)(ETHSOCKIPC_ID id);

typedef struct {
	ETHSOCKET_CMDID CmdId;
	ETHSOCKET_CMDID_FP pFunc;
} ETHSOCKET_CMDID_SET;

/**
*   control condition
*/
typedef struct _ETHSOCKIPC_CTRL {
	UINT32 ui_init_key;  ///< indicate module is initail
	BOOL   b_opened;     ///< indicate task is open
	THREAD_HANDLE     task_id;      ///< given a task id
	ID     sem_id;       ///< given a semaphore id
	ID     flag_id;      ///< given a flag id
	char   *token_path;   ///< ipc queue path
} ETHSOCKIPC_CTRL;

#define MAX_ETHSOCKET_NUM     (3)
ETHSOCKIPC_CTRL *EthsockIpc_GetCtrl(ETHSOCKIPC_ID id);
extern int id;
#endif //_ETHSOCKIPCDEBUG_H

