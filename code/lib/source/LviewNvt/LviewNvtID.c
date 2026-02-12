#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "LviewNvtID.h"
#include "LviewNvtInt.h"
#include "LviewNvtIpc.h"
#include "LviewNvt/LviewNvtAPI.h"

#define PRI_LVIEWD_RCV             3
#define STKSIZE_LVIEWD_RCV         1024

ID              FLG_ID_LVIEWNVT;
ID              LVIEWD_FLG_ID;
VK_TASK_HANDLE  LVIEWD_RCV_TSK_ID;
SEM_HANDLE      LVIEWD_SEM_ID;

void LviewNvt_InstallID(void)
{
	OS_CONFIG_FLAG(FLG_ID_LVIEWNVT);
	OS_CONFIG_FLAG(LVIEWD_FLG_ID);
	OS_CONFIG_SEMPHORE(LVIEWD_SEM_ID, 0, 1, 1);
	#if LVIEW_USE_IPC
	OS_CONFIG_TASK(LVIEWD_RCV_TSK_ID,      PRI_LVIEWD_RCV,      STKSIZE_LVIEWD_RCV,      LviewNvt_Ipc_RcvTsk);
	#endif
}

void LviewNvt_UnInstallID(void)
{
	vos_flag_destroy(FLG_ID_LVIEWNVT);
	vos_flag_destroy(LVIEWD_FLG_ID);
	SEM_DESTROY(LVIEWD_SEM_ID);
}

