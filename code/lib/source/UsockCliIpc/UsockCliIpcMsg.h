#ifndef _USOCKCLIIPCMSG_H_
#define _USOCKCLIIPCMSG_H_

#include "Type.h"
#include "usocket_cli_ipc.h"

ER UsockCliIpc_Msg_Init(void);
ER UsockCliIpc_Msg_Uninit(void);

ER UsockCliIpc_Msg_WaitCmd(USOCKETCLI_MSG *pRcvMsg);
ER UsockCliIpc_Msg_AckCmd(INT32 AckValue);

ER UsockCliIpc_Msg_SysCallReq(CHAR* szCmd);
ER UsockCliIpc_Msg_SendCmd(USOCKETCLI_CMDID CmdId, int *pOutRet);

UINT32 UsockCliIpc_Msg_WaitAck(void);
void UsockCliIpc_Msg_SetAck(INT32 msgAck);

#endif //_USOCKCLIIPCMSG_H_

