/**
    FwSrv, Task,Semaphore,Flag Id Install

    @file       FwSrvID.c
    @ingroup    mFWSRV

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "FwSrvInt.h"
#include "FwSrvID.h"

ID FWSRV_FLG_ID = 0;
ID FWSRV_SEM_ID = 0;

void FwSrv_InstallID(void)
{
    OS_CONFIG_FLAG(FWSRV_FLG_ID);
    OS_CONFIG_SEMPHORE(FWSRV_SEM_ID,0,1,1);
}

void FwSrv_UnInstallID(void)
{
    vos_flag_destroy(FWSRV_FLG_ID);
    vos_sem_destroy(FWSRV_SEM_ID);
}