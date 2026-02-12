/**
    @file       UIBackgroundTsk.c

    @ingroup    mIAPPExtUIFrmwk

    @brief      UI background task
                Implement UI background functions


    Copyright   Novatek Microelectronics Corp. 2007.  All rights reserved.

*/

/**
@addtogroup mIAPPExtUIFrmwk


@{

*/

#include "NvtMbx.h"
#include "MessageElement.h"
#include "NvtBackInternal.h"
#include "NvtUser/NvtUser.h"
#include "NvtUser/NVTEvent.h"
#include "NvtUser/NVTReturn.h"
#include "NvtUser/NvtBack.h"
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////
#ifdef __DBGLVL__
#undef __DBGLVL__
#define __DBGLVL__          1 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#endif
#include <kwrap/debug.h>

static void BKG_DispatchMessage(NVTEVT evt);
static void BKG_WaitEvent(NVTEVT *evt);
static THREAD_RETTYPE UIBackgroundTsk(void);


static BKG_JOB_ENTRY     g_DefExecuteTable[1] = {NVTEVT_NULL, NULL};
static BKG_JOB_ENTRY     *g_pBKGExecuteTable = g_DefExecuteTable;
static NVTEVT            uiCurrentCmd = NVTEVT_NULL;
static ID NVTBACK_FLG_ID = 0;
static THREAD_HANDLE nvtback_task_hdl;
static BOOL bBkOpen = FALSE;
static UINT32  back_done_event = 0;  //prj register UI evnet when finish job


#define PRI_NVTBACK            10
#define STKSIZE_NVTBACK        8192


/**
    Initialize Message for UI framework and UI background

    @param UIBackgroundMbxID Mailbox ID for UI background message queue

    @note The mailbox must be configured well in syscfg.c and SysCfg.h already.
    @note Message is usually initialized in system init stage or
          the beginning of key scan. Message must be initialized
          before UI framework and UI background started.
*/
void Bk_InitMessage(void)
{
	DBG_FUNC_BEGIN("\r\n");
	if (!NVTMSG_IsInit()) {
		NVTMSG_Init();
	}
    DBG_FUNC_END("\r\n");
}

/**
    Check the UI framework message is initialized or not.

    @return TRUE or FALSE
*/
BOOL Bk_IsInitMessage(void)
{
	return ((NVTBACK_MBX_ID != NO_USE_MAILBOX_ID));
}


ER BKG_Open(BKG_OBJ *param)
{
	T_CFLG cflg ;

    DBG_FUNC_BEGIN("\r\n");

	memset(&cflg, 0, sizeof(T_CFLG));

	if (bBkOpen) {
		return E_SYS;
	}

	BKG_SetExecFuncTable(param->pDefaultFuncTbl);
    nvt_back_mbx_init();
	Bk_InitMessage();

    if(param->done_event==0) {
		DBG_ERR("plz reg NVTEVT_BACKGROUND_DONE!!\r\n");
        return E_SYS;
    } else {
        back_done_event = param->done_event;
    }

	// create the flag with an initial flag bits
	if (E_OK != vos_flag_create(&NVTBACK_FLG_ID, &cflg, "NVTBACK_FLG")) {
		DBG_ERR("NVTBACK_FLG_ID sfail\r\n");
        return E_SYS;
	}

	nvtback_task_hdl = vos_task_create(UIBackgroundTsk, 0, "nvt_back", PRI_NVTBACK, STKSIZE_NVTBACK);
	if (0 == nvtback_task_hdl) {
		DBG_ERR("nvtback_task create failed\r\n");
        return E_SYS;
	}
    vos_task_resume(nvtback_task_hdl);

	bBkOpen        = TRUE;
    DBG_FUNC_END("\n");
	return E_OK;
}

ER BKG_Close(void)
{
	FLGPTN uiFlag;

    DBG_FUNC_BEGIN("\n");

	if (!bBkOpen) {
		return E_SYS;
	}

	// after wai_flg with TWF_CLR, bit should be cleared
	uiFlag = FLGFRAMEWORK_BACKGROUND_MASK;
	if (E_OK != vos_flag_wait(&uiFlag, NVTBACK_FLG_ID, FLGFRAMEWORK_BACKGROUND_IDLE, TWF_ANDW)) {
		DBG_ERR("vos_flag_wait fail\r\n");
		return E_OK;
	}
	vos_task_destroy(nvtback_task_hdl);

	if (NVTBACK_FLG_ID) {
		vos_flag_destroy(NVTBACK_FLG_ID);
	}

	bBkOpen = FALSE;
    DBG_FUNC_END("\r\n");
	return E_OK;
}
/**
@}
*/


THREAD_RETTYPE UIBackgroundTsk(void)
{
	NVTEVT  evt = 0;

	THREAD_ENTRY();

	DBG_IND("Back worker task start %d\n\r",NVTBACK_FLG_ID);
#if 0
	get_tid(&backgroundTaskID);
	DBG_IND("Background Task ID: %d\n\r", backgroundTaskID);
#endif
	if (E_OK != vos_flag_clr(NVTBACK_FLG_ID, FLGFRAMEWORK_BACKGROUND_BUSY)) {
		DBG_ERR("clr busy fail\r\n");
		THREAD_RETURN(-1);
	}

	while (1) {
    	if (E_OK != vos_flag_set(NVTBACK_FLG_ID, FLGFRAMEWORK_BACKGROUND_IDLE)) {
    		DBG_ERR("set idle fail\r\n");
    		THREAD_RETURN(-1);
    	}

		PROFILE_TASK_IDLE();
		BKG_WaitEvent(&evt);

    	if (E_OK != vos_flag_clr(NVTBACK_FLG_ID, FLGFRAMEWORK_BACKGROUND_IDLE)) {
    		DBG_ERR("clr idle fail\r\n");
    		THREAD_RETURN(-1);
    	}

		PROFILE_TASK_BUSY();
		BKG_DispatchMessage(evt);
		uiCurrentCmd = NVTEVT_NULL;
	}

	THREAD_RETURN(0);

}

void BKG_Done(void)
{
	vos_flag_set(NVTBACK_FLG_ID, FLGFRAMEWORK_BACKGROUND_DONE);
}


void BKG_PostEvent(NVTEVT evt)
{
	NVTMSG *sndMsg;
	ER ret;
    DBG_FUNC_BEGIN("\r\n");
    DBG_FUNC("evt = %d\n\r", evt);

	if (NVTBACK_MBX_ID == NO_USE_MAILBOX_ID) {
		DBG_ERR("Mailbox ID incorrect!\n\r");
		return;
	}

	ret = NVTMSG_GetElement(&sndMsg);
    DBG_FUNC("call NVTMSG_GetElement() to get the ret=%d[NVTRET_OK==0]\r\n",ret);
	if (ret == NVTRET_OK) {
		sndMsg->event = evt;
		ret = nvt_snd_msg(NVTBACK_MBX_ID, 0, (UINT8 *)sndMsg, sizeof(NVTMSG));
        DBG_FUNC("call nvt_snd_msg()to get the ret=%d[E_OK==0]\r\n",ret);
		if (ret != E_OK) {
			DBG_ERR("snd ail %d\r\n",ret);
		}
	} else {
		DBG_ERR("get %d\n\r",ret);
	}
    DBG_FUNC_END("\r\n");
}

/**
    Wait event in background task

    @param[out] evt Command waited

    @note This function is called in BackgroundTsk. Programmers do not call this function.
*/
static void BKG_WaitEvent(NVTEVT *evt)
{
	UINT32 event_id;
	NVTMSG *rcvMsg;
	UINT32 size;
    DBG_FUNC_BEGIN("\r\n");
	if (NVTBACK_MBX_ID == NO_USE_MAILBOX_ID) {
		DBG_ERR("Mailbox ID incorrect!\n\r");
		return;
	}

	nvt_rcv_msg(NVTBACK_MBX_ID, &event_id, (UINT8 **)(&rcvMsg), &size);
	if (event_id == 0 && size == sizeof(NVTMSG)) {
		*evt = rcvMsg->event;
		NVTMSG_FreeElement(rcvMsg);
	} else {
		DBG_ERR("event_id = %d, size = %d\n\r", event_id, size);
	}
    DBG_FUNC_END("\r\n");
}


void BKG_FlushEvent(void)
{
	UINT32 event_id;
	NVTMSG *rcvMsg;
	UINT32 size;
    DBG_FUNC_BEGIN("\r\n");
	if (NVTBACK_MBX_ID == NO_USE_MAILBOX_ID) {
		DBG_ERR("Mailbox ID incorrect!\n\r");
		return;
	}

	while (nvt_chk_msg(NVTBACK_MBX_ID)) {
		nvt_rcv_msg(NVTBACK_MBX_ID, &event_id, (UINT8 **)(&rcvMsg), &size);
		if (event_id == 0 && size == sizeof(NVTMSG)) {
			NVTMSG_FreeElement(rcvMsg);
		}
	}
    DBG_FUNC_END("\r\n");
}

static void BKG_DispatchMessage(NVTEVT evt)
{
	int i = 0;
	UINT32 ret;

    DBG_FUNC_BEGIN("evt = 0x%x\n\r", evt);

	if (!g_pBKGExecuteTable) {
		DBG_ERR("no Background Table\r\n");
		return ;
	}
	while (g_pBKGExecuteTable[i].event != NVTEVT_NULL) {
		if (evt == g_pBKGExecuteTable[i].event) {
			uiCurrentCmd = evt;
			//A flag to determine background task is busy or not
			vos_flag_set(NVTBACK_FLG_ID, FLGFRAMEWORK_BACKGROUND_BUSY);
			vos_flag_clr(NVTBACK_FLG_ID, FLGFRAMEWORK_BACKGROUND_DONE);
			ret = g_pBKGExecuteTable[i].pfn();
			//Send back original event when background task finished
			Ux_PostEvent(back_done_event, 2, g_pBKGExecuteTable[i].event, ret);
			// Wait finishing of background done event handler
			//#NT#2013/03/15#Janice Huang -begin
			//#NT#background should not wait UI done
			//wai_flg(&flag, NVTBACK_FLG_ID, FLGFRAMEWORK_BACKGROUND_DONE, TWF_ANDW | TWF_CLR);
			vos_flag_set(NVTBACK_FLG_ID, FLGFRAMEWORK_BACKGROUND_DONE);
			//#NT#2013/03/15#Janice Huang -end
			vos_flag_clr(NVTBACK_FLG_ID, FLGFRAMEWORK_BACKGROUND_BUSY);
			break;
			//If execute function end, break, not need to continue find event table
		}
		i++;
	}
    DBG_FUNC_END("\r\n");
}

/**
@name UI Background Functions
@{

*/


void BKG_SetExecFuncTable(BKG_JOB_ENTRY *pBackgroundFuncTbl)
{
	if (pBackgroundFuncTbl) {
		g_pBKGExecuteTable = pBackgroundFuncTbl;
	} else {
		g_pBKGExecuteTable = g_DefExecuteTable;
	}
}


/**
    Get execution table

    @return pBackgroundFuncTbl

*/
BKG_JOB_ENTRY *BKG_GetExecFuncTable(void)
{
	return g_pBKGExecuteTable ;
}

void BKG_ResetTsk(void)
{
    #if 0
	FLGPTN uiFlag;

	if (!bBkOpen) {
		return ;
	}

	BKG_FlushEvent();

	wai_flg(&uiFlag, NVTBACK_FLG_ID, FLGFRAMEWORK_BACKGROUND_IDLE, TWF_ANDW);

	ter_tsk(NVTBACK_TSK_ID);
	sta_tsk(NVTBACK_TSK_ID, 0);
    #else
    DBG_ERR("not sup\r\n");
    #endif
}

/**
    Get if background task is busy or not

    @return TRUE : BackgroundTsk is busy
            FALSE: BackgroundTsk is not busy

*/
BOOL BKG_GetTskBusy(void)
{
	return (vos_flag_chk(NVTBACK_FLG_ID, FLGFRAMEWORK_BACKGROUND_BUSY) == FLGFRAMEWORK_BACKGROUND_BUSY);
}


NVTEVT BKG_GetExeuteCommand(void)
{
	return uiCurrentCmd;
}
//@}
//@}

