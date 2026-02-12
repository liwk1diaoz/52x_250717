/**
    vdodisp, Task,Semaphore,Flag Id Install

    @file       vdodisp_id.c
    @ingroup    mVDODISP

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "vdodisp_id.h"
#include "vdodisp_int.h"

#define STKSIZE_VDODISP         2048

extern THREAD_DECLARE(vdodisp_tsk_0, arglist);
extern THREAD_DECLARE(vdodisp_ide_0, arglist);
extern THREAD_DECLARE(vdodisp_tsk_1, arglist);
extern THREAD_DECLARE(vdodisp_ide_1, arglist);


THREAD_HANDLE VDODISP_TSK_ID_0 = 0;
THREAD_HANDLE VDODISP_IDE_ID_0 = 0;
ID VDODISP_FLG_ID_0 = 0;
SEM_HANDLE VDODISP_SEM_ID_0 = {0};

#if (DEVICE_COUNT >= 2)
THREAD_HANDLE VDODISP_TSK_ID_1 = 0;
THREAD_HANDLE VDODISP_IDE_ID_1 = 0;
ID VDODISP_FLG_ID_1 = 0;
SEM_HANDLE VDODISP_SEM_ID_1 = {0};
#endif

ER vdodisp_install_id(VDODISP_DEVID id)
{
    if(id == VDODISP_DEVID_0) {
	THREAD_CREATE(VDODISP_TSK_ID_0, vdodisp_tsk_0, NULL, "vdodisp_tsk_0");
    if(VDODISP_TSK_ID_0 == 0){
        return VDODISP_ER_SYS;
    }
    #if (IDE_TASK==ENABLE)
	THREAD_CREATE(VDODISP_IDE_ID_0, vdodisp_ide_0, NULL, "vdodisp_ide_0");
    if(VDODISP_IDE_ID_0 == 0) {
        return VDODISP_ER_SYS;
    }
    #endif
	SEM_CREATE(VDODISP_SEM_ID_0,1);
   	OS_CONFIG_FLAG(VDODISP_FLG_ID_0);

    }
#if (DEVICE_COUNT >= 2)
    else {
	THREAD_CREATE(VDODISP_TSK_ID_1, vdodisp_tsk_1, NULL, "vdodisp_tsk_1");
    if(VDODISP_TSK_ID_1 == 0){
        return VDODISP_ER_SYS;
    }
    #if (IDE_TASK==ENABLE)
	THREAD_CREATE(VDODISP_IDE_ID_1, vdodisp_ide_1, NULL, "vdodisp_ide_1");
    if(VDODISP_IDE_ID_1 == 0){
        return VDODISP_ER_SYS;
    }
    #endif
	SEM_CREATE(VDODISP_SEM_ID_1,1);
	OS_CONFIG_FLAG(VDODISP_FLG_ID_1);


    }
#endif

    return VDODISP_ER_OK;
}

ER vdodisp_uninstall_id(VDODISP_DEVID id)
{
    if(id == VDODISP_DEVID_0) {
		SEM_DESTROY(VDODISP_SEM_ID_0);
    	return rel_flg(VDODISP_FLG_ID_0);
    }
#if (DEVICE_COUNT >= 2)
    else {
    	SEM_DESTROY(VDODISP_SEM_ID_1);
    	return rel_flg(VDODISP_FLG_ID_1);
    }
#endif
    return VDODISP_ER_SYS;
}

