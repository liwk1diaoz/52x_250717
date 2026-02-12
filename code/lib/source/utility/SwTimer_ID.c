/*
    Install kernel resource ID for software timer

    Install kernel resource ID for software timer

    @file       SwTimer_ID.c
    @ingroup    mIUtilSwTimer
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "SwTimer_int.h"

// Task, Flag and semaphore ID
THREAD_HANDLE uiSwTimerTaskID;
ID uiSwTimerSemID      = 0;
ID uiSwTimerFlagID[SWTIMER_GROUP_CNT]  = { 0 };


/**
    @addtogroup mIUtilSwTimer
*/
//@{

/**
    Install kernel resource ID for software timer module

    Install kernel resource ID for software timer module. Software timer module
    required 1 task ID, 1 semaphore ID and two flag IDs now.

    @note   This is the software timer module's 1st API that system should call.
            And must call once only.

    @return void
*/
void SwTimer_InstallID(void)
{
	UINT32 i;

	OS_CONFIG_SEMPHORE(uiSwTimerSemID, 0, 1, 1);
	for (i = 0; i < (ALIGN_CEIL(SWTIMER_NUM, 64) >> 5); i++) {
		OS_CONFIG_FLAG(uiSwTimerFlagID[i]);
	}
}

//@}
