#ifndef _ETHSOCKIPCMSG_H_
#define _ETHSOCKIPCMSG_H_

#include "ethsocket_ipc.h"
#include "kwrap/type.h"
#include "EthCam/EthsockIpcAPI.h"


ER EthsockIpc_Msg_Init(ETHSOCKIPC_ID id);
ER EthsockIpc_Msg_Uninit(ETHSOCKIPC_ID id);

ER EthsockIpc_Msg_WaitCmd(ETHSOCKIPC_ID id, ETHSOCKET_MSG *pRcvMsg);
ER EthsockIpc_Msg_AckCmd(ETHSOCKIPC_ID id, INT32 AckValue);

ER EthsockIpc_Msg_SysCallReq(ETHSOCKIPC_ID id, CHAR *szCmd);
ER EthsockIpc_Msg_SendCmd(ETHSOCKIPC_ID id, ETHSOCKET_CMDID CmdId, int *pOutRet);

INT32 EthsockIpc_Msg_WaitAck(ETHSOCKIPC_ID id);
void EthsockIpc_Msg_SetAck(ETHSOCKIPC_ID id, INT32 msgAck);

#endif //_ETHSOCKIPCMSG_H_

