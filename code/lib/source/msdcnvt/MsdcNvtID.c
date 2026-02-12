/**
    MsdcNvt, Task,Semaphore Id Install

    @file       MsdcNvtID.c
    @ingroup    mMSDCNVT

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "MsdcNvtInt.h"
#include "MsdcNvtID.h"

#define PRI_MSDCNVT            10
#define STKSIZE_MSDCNVT        4096
#define PRI_MSDCNVT_IPC        8
#define STKSIZE_MSDCNVT_IPC    2048

VK_TASK_HANDLE MSDCNVT_TSK_ID = 0;
ID MSDCNVT_FLG_ID = 0;
ID MSDCNVT_SEM_ID = 0;
ID MSDCNVT_SEM_CB_ID = 0;

#if !defined(_NETWORK_ON_CPU1_)
VK_TASK_HANDLE MSDCNVT_TSK_IPC_ID = 0;
#endif

void MsdcNvt_InstallID(void)
{
	//OS_CONFIG_TASK(MSDCNVT_TSK_ID, PRI_MSDCNVT, STKSIZE_MSDCNVT, MsdcNvtTsk);
	//OS_CONFIG_TASK(MSDCNVT_TSK_IPC_ID, PRI_MSDCNVT_IPC, STKSIZE_MSDCNVT_IPC, xMsdcNvtTsk_Ipc);
#if !defined(_NETWORK_ON_CPU1_)
	MSDCNVT_TSK_IPC_ID = vos_task_create(xMsdcNvtTsk_Ipc,  0, "msdcnvt_ipc",   PRI_MSDCNVT_IPC,	STKSIZE_MSDCNVT_IPC);
#endif
	vos_flag_create(&MSDCNVT_FLG_ID, NULL, "MSDCNVT_FLG_ID");
	vos_sem_create(&MSDCNVT_SEM_ID, 1, "MSDCNVT_SEM_ID");
	vos_sem_create(&MSDCNVT_SEM_CB_ID, 1, "MSDCNVT_SEM_ID");
}

void MsdcNvt_UnInstallID(void)
{
#if !defined(_NETWORK_ON_CPU1_)
	vos_task_destroy(MSDCNVT_TSK_IPC_ID);
#endif
	vos_flag_destroy(MSDCNVT_FLG_ID);
	vos_sem_destroy(MSDCNVT_SEM_ID);
	vos_sem_destroy(MSDCNVT_SEM_CB_ID);
}