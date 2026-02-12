#ifndef _LVIEWNVT_IPC_H_
#define _LVIEWNVT_IPC_H_


void  LviewNvt_Ipc_Open(LVIEWNVT_DAEMON_INFO   *pOpen);
void  LviewNvt_Ipc_Close(void);
void  LviewNvt_Ipc_PushFrame(LVIEWNVT_FRAME_INFO *frame_info);
void  LviewNvt_Ipc_RcvTsk(void);
#endif //_LVIEWNVT_IPC_H_