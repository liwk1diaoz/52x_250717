////////////////////////////////////////////////////////////////////////////////
#include "PrjInc.h"

#if(WIFI_FUNC==ENABLE)
#include "UIModeWifi.h"
#endif

#if (CALIBRATION_FUNC == ENABLE)
#include "EngineerMode.h"
#endif

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UIMode
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////


extern INT32 MovieExe_OnOpen(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray);

int PRIMARY_MODE_MOVIE = -1;      ///< Movie

void ModeMovie_Open(void);
void ModeMovie_Close(void);

void ModeMovie_Open(void)
{
	Input_ResetMask();
	Ux_SetActiveApp(&CustomMovieObjCtrl);
	Ux_SendEvent(0, NVTEVT_EXE_OPEN, 0);

	UI_SetData(FL_ModeIndex, UI_GetData(FL_NextMode));
	/*
	    //disable video1
	    UI_Show(UI_SHOW_PREVIEW, TRUE);
	    //enable video2
	    UI_Show(UI_SHOW_QUICKVIEW, FALSE);
	*/
#if(UI_FUNC==ENABLE)
    if (System_GetState(SYS_STATE_CURRSUBMODE) == SYS_SUBMODE_NORMAL) {
#if (CALIBRATION_FUNC == ENABLE)
		if (IsEngineerModeOpened()) {
			EngineerMode_WndOpen();
		}
		else
#endif
		{
			#if defined(_UI_STYLE_LVGL_)
			lv_plugin_scr_open(UIFlowMovie, NULL);
			#else
			gMovData.State = MOV_ST_VIEW;
			Ux_OpenWindow((VControl *)(&UIFlowWndMovieCtrl), 0);
			#endif
			#if 1
			if (System_IsBootFromRtos() == TRUE) {
				static UINT32 movie_movieon_cnt = 0;
				if(movie_movieon_cnt==0){
					Ux_PostEvent(NVTEVT_KEY_SHUTTER2, 1, NVTEVT_KEY_PRESS);
					#if defined(_UI_STYLE_LVGL_)
					Ux_PostEvent(NVTEVT_KEY_SHUTTER2, 1, NVTEVT_KEY_RELEASE);
					#endif
				}
				movie_movieon_cnt++;
			}
			#endif
			//#NT#2016/03/07#KCHong -begin
			//#NT#Low power timelapse function
#if (TIMELAPSE_LPR_FUNCTION == ENABLE)
			if (MovieTLLPR_CheckHWRTStatus() == TL_HWRT_BOOT_ALARM) {
				MovieTLLPR_Process(TIMELAPSE_FROM_HWRT);
			}
#endif
			//#NT#2016/03/07#KCHong -end
		}
	} else {
	    SYS_SUBMODE *pCurSubMode = System_GetSubModeCtrl();
        if (pCurSubMode && pCurSubMode->pfSubMode_OnOpen) {
		    pCurSubMode->pfSubMode_OnOpen();
	    }
    }
#endif
}
void ModeMovie_Close(void)
{
#if(UI_FUNC==ENABLE)
    if (System_GetState(SYS_STATE_CURRSUBMODE) == SYS_SUBMODE_NORMAL) {
#if (CALIBRATION_FUNC == ENABLE)
		if (IsEngineerModeOpened()) {
			EngineerMode_WndClose();
		}
		else
#endif
		{
            #if defined(_UI_STYLE_LVGL_)
			lv_plugin_scr_close(UIFlowMovie, NULL);
            #else
			Ux_CloseWindow((VControl *)(&UIFlowWndMovieCtrl), 0);
            #endif
		}
	} else {
	    SYS_SUBMODE *pCurSubMode = System_GetSubModeCtrl();
        if (pCurSubMode && pCurSubMode->pfSubMode_OnClose) {
		    pCurSubMode->pfSubMode_OnClose();
        }
	}

#if (MOVIE_DIS == ENABLE)
	Ux_SendEvent(&CustomMovieObjCtrl, NVTEVT_EXE_MOVIEDIS_ENABLE, 1, 0);
#endif
#if 0
	{
    	UINT32 i;

		for (i = 0; i < 500; i++) {
			if (!BKG_GetTskBusy()) {
				break;
			}
			SwTimer_DelayMs(100);
		}
		if (i == 500) {
			debug_err(("^R Wait background task idle timeout \r\n"));
		}
	}
#else
	if (BKG_GetTskBusy()) {
		DBG_WRN("BKG_GetTskBusy\r\n");
	}
#endif
#endif
	Ux_SendEvent(0, NVTEVT_EXE_CLOSE, 0);
}

SYS_MODE gModeMovie = {
	"MOVIE",
	ModeMovie_Open,
	ModeMovie_Close,
	NULL,
	NULL,
	NULL,
	NULL
};

//-----------------------------------------------------------------------------
// Movie (Primary Mode) + UVC (Sub Mode)
//-----------------------------------------------------------------------------
#if (UVC_MULTIMEDIA_FUNC == ENABLE)
int SYS_SUBMODE_UVC = -1;		///< Movie + UVC

void SubModeUvc_Open(void)
{
	if (System_GetState(SYS_STATE_CURRSUBMODE) != System_GetState(SYS_STATE_PREVSUBMODE)) {
		
		    #if defined(_UI_STYLE_LVGL_)

            #else
			Ux_OpenWindow(&UIMenuWndWiFiMobileLinkOKCtrl, 0);
            #endif
	}
}

void SubModeUvc_Close(void)
{
	if (System_GetState(SYS_STATE_CURRSUBMODE) != System_GetState(SYS_STATE_NEXTSUBMODE)) {
		
			#if defined(_UI_STYLE_LVGL_)

            #else
			Ux_CloseWindow(&UIMenuWndWiFiMobileLinkOKCtrl, 0);
            #endif		
	}
}

SYS_SUBMODE gSubModeUvc = {
	"UVC",
	SubModeUvc_Open,
	SubModeUvc_Close,
};
#endif

