#ifndef _LVIEWNVT_NONIPC_H_
#define _LVIEWNVT_NONIPC_H_


void  LviewNvt_NonIpc_Open(LVIEWNVT_DAEMON_INFO   *pOpen);
void  LviewNvt_NonIpc_Close(void);
void  LviewNvt_NonIpc_PushFrame(LVIEWNVT_FRAME_INFO *frame_info);
#endif //_LVIEWNVT_NONIPC_H_