#include "UsockCliIpc/UsockCliIpcAPI.h"

#if (USE_IPC)
#include "SysKer.h"
#include "UsockCliIpcAPI.h"
#include "UsockCliIpcID.h"
#include "UsockCliIpcTsk.h"

#define PRI_USOCKCLIIPC                10
#define STKSIZE_USOCKCLIIPC            4096

UINT32 USOCKCLIIPC_FLG_ID = 0;
UINT32 USOCKCLIIPC_SEM_ID = 0;
UINT32 USOCKCLIIPC_TSK_ID = 0;

void UsockCliIpc_InstallID(void)
{
    OS_CONFIG_FLAG(USOCKCLIIPC_FLG_ID);
    OS_CONFIG_SEMPHORE(USOCKCLIIPC_SEM_ID, 0, 1, 1);
    OS_CONFIG_TASK(USOCKCLIIPC_TSK_ID, PRI_USOCKCLIIPC,  STKSIZE_USOCKCLIIPC,  UsockCliIpc_Tsk);
}
#else
void UsockIpc_InstallID(void)
{
}
#endif
