/**
    Firmware Service  Resource ID

    @file       FwSrvID.h
    @ingroup    mFWSRV

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#ifndef _FWSRVID_H
#define _FWSRVID_H

/**
     Install FwSrv task,flag and semaphore id

     This is default install API. User can create own task id, semphore id,
     flag id to adjust own stack size and task priority as following:

     @code
     {
        #define PRI_FWSRV             10
        #define STKSIZE_FWSRV         2048

        UINT32 FWSRV_TSK_ID = 0;
        UINT32 FWSRV_FLG_ID = 0;
        UINT32 FWSRV_SEM_ID = 0;

        void Install_UserFwSrvID(void)
        {
            OS_CONFIG_TASK(FWSRV_TSK_ID,PRI_FWSRV,STKSIZE_FWSRV,FwSrvTsk);
            OS_CONFIG_FLAG(FWSRV_FLG_ID);
            OS_CONFIG_SEMPHORE(FWSRV_SEM_ID,0,1,1);
        }
     }
     @endcode
*/
extern void FwSrv_InstallID(void);
extern void FwSrv_UnInstallID(void);

extern VK_TASK_HANDLE FWSRV_TSK_ID; ///< task id
extern ID FWSRV_FLG_ID; ///< flag id
extern ID FWSRV_SEM_ID; ///< semaphore id

#endif //_FWSRVID_H