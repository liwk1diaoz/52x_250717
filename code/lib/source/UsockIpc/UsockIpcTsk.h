#ifndef _USOCKIPCTSK_H_
#define _USOCKIPCTSK_H_

void UsockIpc_Tsk_0(void);
void UsockIpc_Tsk_1(void);
INT32 UsockIpc_CmdId_Notify(USOCKIPC_ID id);
INT32 UsockIpc_CmdId_Receive(USOCKIPC_ID id);
INT32 UsockIpc_CmdId_Udp_Notify(USOCKIPC_ID id);
INT32 UsockIpc_CmdId_Udp_Receive(USOCKIPC_ID id);


#endif //_USOCKIPCTSK_H_

