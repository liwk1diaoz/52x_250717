/**
    @file       UIFrameworkTsk.c

    @ingroup    mIAPPExtUIFrmwk

    @brief      UI framework task
                Implement UI framework functions


    Copyright   Novatek Microelectronics Corp. 2007.  All rights reserved.

*/

#include "NvtUser/NvtUser.h"
#include "NvtUserInternal.h"
#include "NvtMbx.h"
#include <kwrap/task.h>
#include <stdarg.h>
#include <string.h>


/**
@addtogroup mIAPPExtUIFrmwk
@{
*/

#if 0
static INT32 Ux_DispatchMessage(NVTEVT evt, UINT32 paramNum, UINT32 *paramArray);
#endif
// ----------
//UINT32 pUIParamArray[MAX_MESSAGE_PARAM_NUM];
BOOL bFrameworkOpen = FALSE;
// ----------
#define NO_USE_TASK_ID      0xffff
ID gFrameworkRunningTskID = NO_USE_TASK_ID;
ID gFrameworkCallingLevel = 0;
//NVTRET (*gpfPostProc)(void) = 0;
//INT32 (*gpfEventProc)(NVTEVT evt, UINT32 paramNum, UINT32 *paramArray) = 0;
void (*gpfMainProc)(void) = 0;
void (*gpfNvtuserCB)(UXUSER_CB_TYPE evt, UINT32 p1, UINT32 p2, UINT32 p3) = 0;
static THREAD_HANDLE ui_task_hdl;

THREAD_RETTYPE UIFrameworkTsk(void);

// ----------

ID Ux_GetTaskID(void)
{
    return (ID)ui_task_hdl;
}

/**
@name UI Framework Open Functions
@{
*/
#define PRI_NVTUSER            10
#define STKSIZE_NVTUSER        (1024*8)

ER Ux_Open(UXUSER_OBJ *param)
{
	DBG_FUNC_BEGIN("\r\n");

	if (bFrameworkOpen) {
		return E_SYS;
	}

	bFrameworkOpen        = TRUE;
	gpfMainProc = param->pfMainProc;
	gpfNvtuserCB = param->pfNvtUserCB;

    nvt_user_mbx_init();
	Ux_InitMessage();

    ui_task_hdl = vos_task_create(UIFrameworkTsk, 0, "nvt_user", PRI_NVTUSER, STKSIZE_NVTUSER);
	if (0 == ui_task_hdl) {
		DBG_ERR("nvt_user create failed\r\n");
        return E_SYS;
	}
    vos_task_resume(ui_task_hdl);
    DBG_FUNC_END("\r\n");
	return E_OK;
}

BOOL Ux_IsOpen(void)
{
	return bFrameworkOpen;
}

/**
@name UI Framework Close Functions
@{
*/

ER Ux_Close(void)
{
#if 1
	if (!bFrameworkOpen) {
		return E_SYS;
	}
	Ux_UnInitMessage();
  	vos_task_destroy(ui_task_hdl);

#else
	FLGPTN uiFlag;

	if (!bFrameworkOpen) {
		return E_SYS;
	}

	wai_flg(&uiFlag, gFrameworkFlgId, FLGFRAMEWORK_IDLE, TWF_ANDW);//wait until idle

	ter_tsk(gFrameworkTaskID);

	gFrameworkFlgId = 0;
	gFrameworkWindowSemId = 0;
	gFrameworkDrawSemId = 0;
	gFrameworkTaskID = 0;
	bFrameworkOpen = FALSE;
#endif
	return E_OK;
}

#if 1 //move to project register main process
THREAD_RETTYPE UIFrameworkTsk(void)
{
	DBG_FUNC_BEGIN("\n");

	THREAD_ENTRY();
	if (gpfMainProc) {
		gpfMainProc();
	}

	bFrameworkOpen = FALSE;
	THREAD_RETURN(0);
}
#else
void UIFrameworkTsk(void)
{
	NVTEVT evt;
	UINT32 paramNum;
	FLGPTN flag;
	UINT32 pUIParamArray[MAX_MESSAGE_PARAM_NUM];
	//clr_flg(gFrameworkFlgId, 0xffffffff);
	INT32 result = NVTEVT_PASS;

	while (1) {
#if (!UX_POSTEVT)
		// This task run only when at least one windows is created.
		wai_flg(&flag, gFrameworkFlgId, FLGFRAMEWORK_WINDOW_CREATED, TWF_ANDW);
#endif

		while (1) {
			set_flg(gFrameworkFlgId, FLGFRAMEWORK_IDLE); //idle
			Ux_WaitEvent(&evt, &paramNum, pUIParamArray);
			clr_flg(gFrameworkFlgId, FLGFRAMEWORK_IDLE); //busy

			DBG_IND("MSG: get event %08x!\r\n", evt);
			if (evt) {
#if (!UX_POSTEVT)
				ID callerTskId;
				//wai_sem(gFrameworkWindowSemId);
				get_tid(&callerTskId);
#endif

#if (!UX_POSTEVT)
				if (gFrameworkRunningTskID == NO_USE_TASK_ID ||
					callerTskId != gFrameworkRunningTskID) {
					wai_sem(gFrameworkWindowSemId);
					gFrameworkRunningTskID = callerTskId;
				}
				gFrameworkCallingLevel++;
#endif


				result = Ux_DispatchMessage(evt, paramNum, pUIParamArray);
				DBG_IND("MSG: result %s\r\n", (result == NVTEVT_CONSUME) ? "consume" : "pass");

#if (!UX_POSTEVT)
				gFrameworkCallingLevel--;
				if (gFrameworkCallingLevel == 0) {
					gFrameworkRunningTskID = NO_USE_TASK_ID;
					sig_sem(gFrameworkWindowSemId);
				}
#endif
			}
		}
	}
}
#endif

#if 0
//
static INT32 Ux_DispatchMessage(NVTEVT evt, UINT32 paramNum, UINT32 *paramArray)
{
	INT32 result = NVTEVT_PASS;
	//VControl *pCtrl = 0;
	//UINT8 wndIndex;
	VControl *pAppCtrl = NULL;

	pAppCtrl = Ux_GetActiveApp();
	if (pAppCtrl && (IS_APP_EVENT(evt))) {
		//1.SEND TO APP
		DBG_IND("MSG: send to App %s!\r\n", pAppCtrl->Name);
		result = Ux_SendEventInternal(pAppCtrl, evt, paramNum, paramArray);
	}
	if (result != NVTEVT_CONSUME) {
		//2.SEND TO UI
		DBG_IND("MSG: send to UI!\r\n");
		if (gpfEventProc) {
			result = gpfEventProc(evt, paramNum, paramArray);
		}
	}
	if (gpfPostProc) {
		//3.POST PROCESS
		DBG_IND("MSG: Post Process!\r\n");
		gpfPostProc();
	}

	// A special case.
	// UI background is busy until finishing the event handler of BACKGROUND_DONE
	if (evt == NVTEVT_BACKGROUND_DONE) {
		BKG_Done();
	}
	return result;
}
#endif

void Ux_DumpEvents(void)
{
	NVTMSG_DumpStatus();
	Ux_DumpEventHistory();
}


//@}

