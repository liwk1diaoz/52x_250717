/**
    MsdcNvt Resource ID

    @file       msdcnvt_id.h
    @ingroup    mMsdcNvt

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#ifndef _MSDCNVTID_H
#define _MSDCNVTID_H

#include "msdcnvt_int.h"

extern THREAD_HANDLE _SECTION(".kercfg_data") m_msdcnvt_tsk_id; ///< task id
extern FLGPTN _SECTION(".kercfg_data") m_msdcnvt_flg_id; ///< flag id
extern SEM_HANDLE _SECTION(".kercfg_data") m_msdcnvt_sem_id; ///< semaphore id
extern SEM_HANDLE _SECTION(".kercfg_data") m_msdcnvt_sem_cb_id; ///< semaphore id to avoid racing between NET and USB
extern THREAD_HANDLE _SECTION(".kercfg_data") m_msdcnvt_tsk_ipc_id; ///< task id

#endif //_MSDCNVTID_H