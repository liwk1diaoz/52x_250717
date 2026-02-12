#ifndef _ETHSOCKIPCTSK_H_
#define _ETHSOCKIPCTSK_H_
#include "EthCam/EthsockIpcAPI.h"
#include "ethsocket_ipc.h"

void EthsockIpc_Tsk_0(void);
void EthsockIpc_Tsk_1(void);
void EthsockIpc_Tsk_2(void);
INT32 EthsockIpc_CmdId_Notify(ETHSOCKIPC_ID id);
INT32 EthsockIpc_CmdId_Receive(ETHSOCKIPC_ID id);
INT32 EthsockIpc_CmdId_Udp_Notify(ETHSOCKIPC_ID id);
INT32 EthsockIpc_CmdId_Udp_Receive(ETHSOCKIPC_ID id);
void EthsockIpc_HandleCmd(ETHSOCKET_CMDID CmdId, unsigned int id);

#endif //_ETHSOCKIPCTSK_H_

