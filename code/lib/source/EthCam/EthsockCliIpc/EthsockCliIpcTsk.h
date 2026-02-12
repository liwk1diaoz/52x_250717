#ifndef _ETHSOCKCLIIPCTSK_H_
#define _ETHSOCKCLIIPCTSK_H_
#include "EthCam/EthsockCliIpcAPI.h"
#include "Ethsocket_cli_ipc.h"

void EthsockCliIpc_Tsk_Path0_0(void);
void EthsockCliIpc_Tsk_Path0_1(void);
void EthsockCliIpc_Tsk_Path0_2(void);
void EthsockCliIpc_Tsk_Path1_0(void);
void EthsockCliIpc_Tsk_Path1_1(void);
void EthsockCliIpc_Tsk_Path1_2(void);

INT32 EthsockCliIpc_CmdId_Notify(UINT32 path_id, ETHSOCKIPCCLI_ID id);
INT32 EthsockCliIpc_CmdId_Receive(UINT32 path_id, ETHSOCKIPCCLI_ID id);
void EthsockCliIpc_HandleCmd(ETHSOCKETCLI_CMDID CmdId, unsigned int path_id, unsigned int id);

#endif //_ETHSOCKCLIIPCTSK_H_

