#ifndef _WIFIIPCMSG_H_
#define _WIFIIPCMSG_H_

#if defined(_NETWORK_ON_CPU2_)

#include <kwrap/type.h>
#include "nvtwifi.h"

ER WiFiIpc_Msg_Init(void);
ER WiFiIpc_Msg_Uninit(void);

ER WiFiIpc_Msg_WaitCmd(NVTWIFI_MSG *pRcvMsg);
ER WiFiIpc_Msg_AckCmd(INT32 AckValue);

ER WiFiIpc_Msg_SysCallReq(CHAR *szCmd);
ER WiFiIpc_Msg_SendCmd(NVTWIFI_CMDID CmdId, int *pOutRet);

UINT32 WiFiIpc_Msg_WaitAck(void);
void WiFiIpc_Msg_SetAck(INT32 msgAck);

#endif

#endif //_WIFIIPCMSG_H_

