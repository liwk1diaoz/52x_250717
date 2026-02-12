#ifndef _USOCKIPCMSG_H_
#define _USOCKIPCMSG_H_

#include "Type.h"
#include "usocket_ipc.h"

ER UsockIpc_Msg_Init(USOCKIPC_ID id);
ER UsockIpc_Msg_Uninit(USOCKIPC_ID id);

ER UsockIpc_Msg_WaitCmd(USOCKIPC_ID id,USOCKET_MSG *pRcvMsg);
ER UsockIpc_Msg_AckCmd(USOCKIPC_ID id,INT32 AckValue);

ER UsockIpc_Msg_SysCallReq(USOCKIPC_ID id,CHAR *szCmd);
ER UsockIpc_Msg_SendCmd(USOCKIPC_ID id,USOCKET_CMDID CmdId, int *pOutRet);

UINT32 UsockIpc_Msg_WaitAck(USOCKIPC_ID id);
void UsockIpc_Msg_SetAck(USOCKIPC_ID id,INT32 msgAck);

#endif //_USOCKIPCMSG_H_

