#ifndef _ETHSOCKCLIIPCMSG_H_
#define _ETHSOCKCLIIPCMSG_H_

#include "kwrap/type.h"
#include "Ethsocket_cli_ipc.h"
#include "EthCam/EthsockCliIpcAPI.h"

ER EthsockCliIpc_Msg_Init(UINT32 path_id, ETHSOCKIPCCLI_ID id);
ER EthsockCliIpc_Msg_Uninit(UINT32 path_id, ETHSOCKIPCCLI_ID id);

ER EthsockCliIpc_Msg_WaitCmd(UINT32 path_id, ETHSOCKIPCCLI_ID id, ETHSOCKETCLI_MSG *pRcvMsg);
ER EthsockCliIpc_Msg_AckCmd(UINT32 path_id, ETHSOCKIPCCLI_ID id, INT32 AckValue);

ER EthsockCliIpc_Msg_SysCallReq(UINT32 path_id, ETHSOCKIPCCLI_ID id, CHAR* szCmd);
ER EthsockCliIpc_Msg_SendCmd(UINT32 path_id, ETHSOCKIPCCLI_ID id, ETHSOCKETCLI_CMDID CmdId, int *pOutRet);

UINT32 EthsockCliIpc_Msg_WaitAck(UINT32 path_id, ETHSOCKIPCCLI_ID id);
void EthsockCliIpc_Msg_SetAck(UINT32 path_id, ETHSOCKIPCCLI_ID id, INT32 msgAck);

#endif //_ETHSOCKCLIIPCMSG_H_

