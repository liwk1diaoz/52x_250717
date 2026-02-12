/**
    MsdcNvt Resource ID

    @file       MsdcNvtID.h
    @ingroup    mMsdcNvt

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include <hd_type.h>

#ifndef _MSDCNVTID_H
#define _MSDCNVTID_H

extern VK_TASK_HANDLE  MSDCNVT_TSK_ID; ///< task id
extern ID  MSDCNVT_FLG_ID; ///< flag id
extern ID  MSDCNVT_SEM_ID; ///< semaphore id
extern ID  MSDCNVT_SEM_CB_ID; ///< semaphore id to avoid racing between NET and USB
extern VK_TASK_HANDLE  MSDCNVT_TSK_IPC_ID; ///< task id

#endif //_MSDCNVTID_H