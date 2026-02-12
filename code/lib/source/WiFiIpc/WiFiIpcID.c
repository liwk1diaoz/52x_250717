#include "SysKer.h"
#include "WiFiIpcAPI.h"
#include "WiFiIpcID.h"
#include "WiFiIpcTsk.h"

#if defined(_NETWORK_ON_CPU2_)
#define PRI_WIFIIPC                10
#define STKSIZE_WIFIIPC            2048

UINT32 WIFIIPC_FLG_ID = 0;
UINT32 WIFIIPC_SEM_ID = 0;
UINT32 WIFIIPC_TSK_ID = 0;

void WiFiIpc_InstallID(void)
{
	OS_CONFIG_FLAG(WIFIIPC_FLG_ID);
	OS_CONFIG_SEMPHORE(WIFIIPC_SEM_ID, 0, 1, 1);
	OS_CONFIG_TASK(WIFIIPC_TSK_ID, PRI_WIFIIPC,  STKSIZE_WIFIIPC,  WiFiIpc_Tsk);
}
#else
void WiFiIpc_InstallID(void)
{
}
#endif
