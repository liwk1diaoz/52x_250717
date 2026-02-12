#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "HfsNvt/HfsNvtAPI.h"
#include "HfsNvtID.h"
#include "HfsNvtTsk.h"
#include "HfsNvtIpc.h"

#define PRI_HSFNVT_MAIN               7
#define STKSIZE_HFSNVT_MAIN           2048
#define PRI_HSFNVT_RCV                6
#define STKSIZE_HFSNVT_RCV            4096


ID             HFSNVT_FLG_ID = 0;
VK_TASK_HANDLE HFSNVT_TSK_RCV_ID = 0;
SEM_HANDLE     HFSNVT_SEM_ID = 0;

void HfsNvt_InstallID(void)
{
	OS_CONFIG_SEMPHORE(HFSNVT_SEM_ID, 0, 1, 1);
	OS_CONFIG_FLAG(HFSNVT_FLG_ID);
	#if HFS_USE_IPC
	OS_CONFIG_TASK(HFSNVT_TSK_RCV_ID,    PRI_HSFNVT_RCV,   STKSIZE_HFSNVT_RCV,   HfsNvt_Ipc_RcvTsk);
	#endif
}

void HfsNvt_UnInstallID(void)
{
	SEM_DESTROY(HFSNVT_SEM_ID);
	rel_flg(HFSNVT_FLG_ID);
}

