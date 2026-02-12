/**
    MsdcNvt, Task,Semaphore Id Install

    @file       msdcnvt_id.c
    @ingroup    mMSDCNVT

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "msdcnvt_id.h"
#include "msdcnvt_int.h"

#define PRI_MSDCNVT            10
#define STKSIZE_MSDCNVT        4096
#define PRI_MSDCNVT_IPC        8
#define STKSIZE_MSDCNVT_IPC    2048

THREAD_HANDLE m_msdcnvt_tsk_id = 0;
THREAD_HANDLE m_msdcnvt_tsk_ipc_id = 0;
FLGPTN m_msdcnvt_flg_id = 0;
#if defined(__UITRON)
SEM_HANDLE m_msdcnvt_sem_id = 0;
SEM_HANDLE m_msdcnvt_sem_cb_id = 0;
#else
SEM_HANDLE m_msdcnvt_sem_id;
SEM_HANDLE m_msdcnvt_sem_cb_id;
#endif

void msdcnvt_install_id(void)
{
#if defined(__UITRON)
	OS_CONFIG_TASK(m_msdcnvt_tsk_id, PRI_MSDCNVT, STKSIZE_MSDCNVT, msdcnvt_tsk);
	OS_CONFIG_TASK(m_msdcnvt_tsk_ipc_id, PRI_MSDCNVT_IPC, STKSIZE_MSDCNVT_IPC, xmsdcnvt_tsk_ipc);
	OS_CONFIG_FLAG(m_msdcnvt_flg_id);
	OS_CONFIG_SEMPHORE(m_msdcnvt_sem_id, 0, 1, 1);
	OS_CONFIG_SEMPHORE(m_msdcnvt_sem_cb_id, 0, 1, 1);
#else
	//THREAD_CREATE(m_msdcnvt_tsk_id, msdcnvt_tsk, NULL, "msdcnvt_tsk");
	//THREAD_CREATE(m_msdcnvt_tsk_ipc_id, xmsdcnvt_tsk_Ipc, NULL, "xmsdcnvt_tsk_Ipc");
	OS_CONFIG_FLAG(m_msdcnvt_flg_id);
	SEM_CREATE(m_msdcnvt_sem_id, 1);
	SEM_CREATE(m_msdcnvt_sem_cb_id, 1);
#endif
}

void msdcnvt_uninstall_id(void)
{
#if defined(__UITRON)
#else
	rel_flg(m_msdcnvt_flg_id);
	SEM_DESTROY(m_msdcnvt_sem_id);
        SEM_DESTROY(m_msdcnvt_sem_cb_id);
#endif
}
