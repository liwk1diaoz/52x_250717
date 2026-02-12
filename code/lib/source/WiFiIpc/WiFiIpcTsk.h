#ifndef _WIFIIPCTSK_H_
#define _WIFIIPCTSK_H_

#if defined(_NETWORK_ON_CPU2_)

void WiFiIpc_Tsk(void);
INT32 WiFiIpc_CmdId_StaCB(void);
INT32 WiFiIpc_CmdId_LinkCB(void);
INT32 WiFiIpc_CmdId_WSCCB(void);
INT32 WiFiIpc_CmdId_WSC_FlashCB(void);
INT32 WiFiIpc_CmdId_P2PCB(void);

#endif

#endif //_WIFIIPCTSK_H_

