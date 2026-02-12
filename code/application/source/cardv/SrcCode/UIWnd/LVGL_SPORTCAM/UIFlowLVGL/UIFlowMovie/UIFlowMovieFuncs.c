#include "PrjInc.h"
//#include "UIWnd/UIFlow.h"

#define __MODULE__          UIMovieFuncs
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

#define MOVIE_REC_TIME_MIN				10
#define MOVIE_REC_MIN_CLUSTER_SIZE		(0x8000)    // 32KB
#define MOVIE_REC_SD_CLASS				(4)         // class 4

MOV_TASK_DATA   gMovData = { 0 };

static UINT32  g_MovRecMaxTime = 0;
static UINT32  g_MovRecCurrTime = 0;
static UINT32  g_MovRecSelfTimerSec = 0;
static UINT32  g_MovRecSelfTimerID = NULL_TIMER;

extern uint16_t warn_msgbox_auto_close_ms;
extern uint32_t warn_msgbox_auto_infinite_ms;
extern void UIFlowWrnMsgAPI_Open_StringID(lv_plugin_res_id id, uint16_t auto_close_time_ms);

UINT8 FlowMovie_GetMovDataState(void)
{
	return gMovData.State;
}

void FlowMovie_StartRec(void)
{
	if (g_MovRecSelfTimerSec == 0) {
		g_MovRecCurrTime = 0;

		if (System_GetState(SYS_STATE_POWERON) == SYSTEM_POWERON_SAFE) {
			// wait playing sound stop
			////GxSound_WaitStop();
		}
		Ux_SendEvent(&CustomMovieObjCtrl, NVTEVT_EXE_MOVIE_REC_START, 0);

		// disable auto power off/USB detect timer
		////KeyScan_EnableMisc(FALSE);
		////gMovData.State = MOV_ST_REC;
	} else {
		DBG_ERR("not yet, g_MovSelfTimerSec=%d\r\n", g_MovRecSelfTimerSec);
	}
}

void FlowMovie_StopRec(void)
{
#if (_ADAS_FUNC_ == ENABLE)
	// Fixed icon disappear issue when stop record during ADAS warning window
	UxCtrl_SetShow(&UIFlowWndMovie_Panel_Normal_DisplayCtrl, TRUE);
	UxCtrl_SetShow(&UIFlowWndMovie_ADAS_Alert_DisplayCtrl, FALSE);
#endif  // #if (_ADAS_FUNC_ == ENABLE)

	////UxState_SetData(&UIFlowWndMovie_Status_RECCtrl, STATE_CURITEM, UIFlowWndMovie_Status_REC_ICON_REC_TRANSPAENT);
	Ux_SendEvent(&CustomMovieObjCtrl, NVTEVT_EXE_MOVIE_REC_STOP, 0);

	// enable auto power off/USB detect timer
	////KeyScan_EnableMisc(TRUE);
	gMovData.State = MOV_ST_VIEW;
}

void FlowMovie_StopRecSelfTimer(void)
{
	if (g_MovRecSelfTimerID != NULL_TIMER) {
		GxTimer_StopTimer(&g_MovRecSelfTimerID);
		g_MovRecSelfTimerID = NULL_TIMER;
	}
	g_MovRecSelfTimerSec = 0;
}

UINT8 FlowMovie_GetDataState(void)
{
	return gMovData.State;
}

void FlowMovie_SetRecMaxTime(UINT32 RecMaxTime)
{
	g_MovRecMaxTime = RecMaxTime;
}

UINT32 FlowMovie_GetRecMaxTime(void)
{
	return g_MovRecMaxTime;
}

void FlowMovie_SetRecCurrTime(UINT32 RecCurrTime)
{
	g_MovRecCurrTime = RecCurrTime;
}

UINT32 FlowMovie_GetRecCurrTime(void)
{
	return g_MovRecCurrTime;
}

BOOL FlowMovie_IsStorageErr2(lv_obj_t* parent,BOOL IsCheckFull)
{
#if (MOVIE_ENSURE_SD_32KCLUSTER == ENABLE)
	UINT32  uiClusterSize;
#endif
#if (MOVIE_ENSURE_SD_CLASS4 == ENABLE)
	PSDIO_MISC_INFORMATION pMiscInfo;
#endif

	// check card inserted
	if (System_GetState(SYS_STATE_CARD)  == CARD_REMOVED) {
		UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_PLEASE_INSERT_SD, warn_msgbox_auto_close_ms);
		return TRUE;
	}

	// check storage error
	if (UIStorageCheck(STORAGE_CHECK_ERROR, NULL) == TRUE) {
		UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_MEMORYERROR, warn_msgbox_auto_infinite_ms);
		return TRUE;
	}


	// check storage lock or directory read only
	if (UIStorageCheck(STORAGE_CHECK_LOCKED, NULL) == TRUE ||
		UIStorageCheck(STORAGE_CHECK_DCIM_READONLY, NULL) == TRUE) {
		UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_CARD_LOCKED, warn_msgbox_auto_close_ms);
		return TRUE;
	}

	// check storage full
	if (TRUE == IsCheckFull) {
		g_MovRecMaxTime = Movie_GetFreeSec();
		if (g_MovRecMaxTime <= MOVIE_REC_TIME_MIN) {
			g_MovRecMaxTime = 0;
			UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_CARD_FULL, warn_msgbox_auto_close_ms);
			return TRUE;
		}
	}

	// check folder full
	if (UIStorageCheck(STORAGE_CHECK_FOLDER_FULL, NULL) == TRUE) {
		UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_CARD_FULL, warn_msgbox_auto_close_ms);
		return TRUE;
	}

	// check serial number full
	if (MovieExe_CheckSNFull()) {
		UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_CARD_FULL, warn_msgbox_auto_close_ms);
		return TRUE;
	}

#if (MOVIE_ENSURE_SD_CLASS4 == ENABLE)
	// check sd card whether faster than class 4
	pMiscInfo = sdio_getMiscInfo();
	if (pMiscInfo->uiWriteRate < MOVIE_REC_SD_CLASS) {
		UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_SD_CLASS4, warn_msgbox_auto_close_ms);
		return TRUE;
	}
#endif

#if (MOVIE_ENSURE_SD_32KCLUSTER == ENABLE)
	// Check cluster size, need to be larger than 32KB
	uiClusterSize = FileSys_GetDiskInfo(FST_INFO_CLUSTER_SIZE);
	if (uiClusterSize < MOVIE_REC_MIN_CLUSTER_SIZE) {
		UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_CLUSTER_WRONG, warn_msgbox_auto_close_ms);
		return TRUE;
	}
#endif

	return FALSE;
}

UINT32 FlowMovie_GetSelfTimerID(void)
{
	return g_MovRecSelfTimerID;
}

void FlowMovie_OnTimer1SecIndex(void)
{

	switch (gMovData.State) {
	case MOV_ST_VIEW:
	case MOV_ST_VIEW | MOV_ST_ZOOM:
	case MOV_ST_REC:
	case MOV_ST_REC | MOV_ST_ZOOM:
		gMovData.SysTimeCount++;
        #if 0
		if (UxCtrl_IsShow(&UIFlowWndMovie_YMD_StaticCtrl)) {
			FlowMovie_IconDrawDateTime();
		}
        #endif
		break;

	}
}

