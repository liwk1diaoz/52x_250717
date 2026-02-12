/**
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       UsockIpcID.h
    @ingroup    mIUsockIpcAPI

    @brief

    @note       Nothing.

    @date       2014/06/13
*/

/** \addtogroup mISYSFileSysIPC */
//@{

#ifndef _USOCKIPCDEBUG_H
#define _USOCKIPCDEBUG_H
#include "usocket_ipc.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UsockIpc
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#define __DBGFLT__          "[API]"
//#define __DBGFLT__          "[MSG]"
//#define __DBGFLT__          "[TSK]"
//#define __DBGFLT__          "[UTIL]"
#include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////

typedef INT32(*USOCKET_CMDID_FP)(USOCKIPC_ID id);

typedef struct {
	USOCKET_CMDID CmdId;
	USOCKET_CMDID_FP pFunc;
} USOCKET_CMDID_SET;

/**
*   control condition
*/
typedef struct _USOCKIPC_CTRL {
	UINT32 ui_init_key;  ///< indicate module is initail
	BOOL   b_opened;     ///< indicate task is open
	ID     task_id;      ///< given a task id
	ID     sem_id;       ///< given a semaphore id
	ID     flag_id;      ///< given a flag id
	char   *token_path;   ///< ipc queue path
} USOCKIPC_CTRL;

#define MAX_USOCKET_NUM     (5)
USOCKIPC_CTRL *UsockIpc_GetCtrl(USOCKIPC_ID id);
extern int id;
#endif //_USOCKIPCDEBUG_H
