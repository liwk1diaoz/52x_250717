/**
    PartLoad, Task,Semaphore,Flag Id Install

    @file       PartLoadID.c
    @ingroup    mPARTLOAD

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "FwSrvInt.h"
#include "PartLoadID.h"

THREAD_HANDLE PARTLOAD_TSK_ID = (THREAD_HANDLE)NULL;
ID PARTLOAD_FLG_ID = 0;

void PartLoad_InstallID(void)
{
	OS_CONFIG_FLAG(PARTLOAD_FLG_ID);
}

void PartLoad_UnInstallID(void)
{
	vos_sem_destroy(PARTLOAD_FLG_ID);
}