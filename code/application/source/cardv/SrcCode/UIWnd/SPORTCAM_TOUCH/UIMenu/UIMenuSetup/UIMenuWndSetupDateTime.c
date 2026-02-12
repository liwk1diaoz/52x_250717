#include "UIWnd/UIFlow.h"
#include "UIMenuWndSetupDateTimeRes.c"
#include "UIMenuWndSetupDateTime.h"
#include "PrjInc.h"
#include "GxTime.h"
#if(defined(_NVT_ETHREARCAM_RX_))
#include "UIApp/Network/EthCamAppCmd.h"
#include "UIApp/Network/EthCamAppNetwork.h"
#endif
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UIMenuWndSetupDateTime
#define __DBGLVL__          1 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

//---------------------UIMenuWndSetupDateTimeCtrl Debug Definition -----------------------------
#define _UIMENUWNDSETUPDATETIME_ERROR_MSG        1
#define _UIMENUWNDSETUPDATETIME_TRACE_MSG        0

#if (_UIMENUWNDSETUPDATETIME_ERROR_MSG&(PRJ_DBG_LVL>=PRJ_DBG_LVL_ERR))
#define UIMenuWndSetupDateTimeErrMsg(...)            debug_msg ("^R UIMenuWndSetupDateTime: "__VA_ARGS__)
#else
#define UIMenuWndSetupDateTimeErrMsg(...)
#endif

#if (_UIMENUWNDSETUPDATETIME_TRACE_MSG&(PRJ_DBG_LVL>=PRJ_DBG_LVL_TRC))
#define UIMenuWndSetupDateTimeTraceMsg(...)          debug_msg ("^B UIMenuWndSetupDateTime: "__VA_ARGS__)
#else
#define UIMenuWndSetupDateTimeTraceMsg(...)
#endif

//---------------------UIMenuWndSetupDateTimeCtrl Global Variables -----------------------------

//---------------------UIMenuWndSetupDateTimeCtrl Prototype Declaration  -----------------------

//---------------------UIMenuWndSetupDateTimeCtrl Public API  ----------------------------------

//---------------------UIMenuWndSetupDateTimeCtrl Private API  ---------------------------------
#define DATETIME_MAX_YEAR       2050
#define DATETIME_DEFAULT_YEAR   2021//2000
#define DATETIME_DEFAULT_MONTH     1
#define DATETIME_DEFAULT_DAY       1
#define DATETIME_DEFAULT_HOUR      0
#define DATETIME_DEFAULT_MINUTE    0
#define DATETIME_DEFAULT_SECOND    0

/* (57, 53) is the parent's start (x,y). */
#define DATETIME_START_X        57
#define DATETIME_START_Y        53

#define DATETIME_SHIFT_X        20

typedef enum {
	UI_DATETIME_IDX_Y,
	UI_DATETIME_IDX_Y2,
	UI_DATETIME_IDX_Y3,
	UI_DATETIME_IDX_Y4,
	UI_DATETIME_IDX_M,
	UI_DATETIME_IDX_M2,
	UI_DATETIME_IDX_D,
	UI_DATETIME_IDX_D2,
	UI_DATETIME_IDX_HR,
	UI_DATETIME_IDX_HR2,
	UI_DATETIME_IDX_MIN,
	UI_DATETIME_IDX_MIN2,
	UI_DATETIME_IDX_SEC,
	UI_DATETIME_IDX_SWITCH
} _UI_DATETIME_IDX_;
//#NT#2010/06/01#Chris Chung -end

static UINT32 g_uiBlinkTimerID = NULL_TIMER;
static VControl* g_pActiveSettingCtrl = NULL;
static BOOL ShowActiveSetting = TRUE;

//---------------------UIMenuWndSetupDateTimeCtrl Prototype Declaration  -----------------------

//---------------------UIMenuWndSetupDateTimeCtrl Control List---------------------------
CTRL_LIST_BEGIN(UIMenuWndSetupDateTime)
CTRL_LIST_ITEM(UIMenuWndSetupDateTime_Tab)
CTRL_LIST_END
//----------------------UIMenuWndSetupDateTimeCtrl Event---------------------------
INT32 UIMenuWndSetupDateTime_OnOpen(VControl *, UINT32, UINT32 *);
INT32 UIMenuWndSetupDateTime_OnClose(VControl *, UINT32, UINT32 *);
INT32 UIMenuWndSetupDateTime_OnKeyMenu(VControl *, UINT32, UINT32 *);
INT32 UIMenuWndSetupDateTime_OnKeyMode(VControl *, UINT32, UINT32 *);
INT32 UIMenuWndSetupDateTime_OnTimer(VControl *, UINT32, UINT32 *);
INT32 UIMenuWndSetupDateTime_OnTouchPanelSlideUp(VControl *, UINT32, UINT32 *);
INT32 UIMenuWndSetupDateTime_OnTouchPanelSlideDown(VControl *, UINT32, UINT32 *);
EVENT_BEGIN(UIMenuWndSetupDateTime)
EVENT_ITEM(NVTEVT_OPEN_WINDOW, UIMenuWndSetupDateTime_OnOpen)
EVENT_ITEM(NVTEVT_CLOSE_WINDOW, UIMenuWndSetupDateTime_OnClose)
EVENT_ITEM(NVTEVT_KEY_MENU, UIMenuWndSetupDateTime_OnKeyMenu)
EVENT_ITEM(NVTEVT_KEY_MODE, UIMenuWndSetupDateTime_OnKeyMode)
EVENT_ITEM(NVTEVT_TIMER,UIMenuWndSetupDateTime_OnTimer)
EVENT_ITEM(NVTEVT_SLIDE_UP,UIMenuWndSetupDateTime_OnTouchPanelSlideUp)
EVENT_ITEM(NVTEVT_SLIDE_DOWN,UIMenuWndSetupDateTime_OnTouchPanelSlideDown)
EVENT_END

//#NT#2010/06/01#Chris Chung -begin
static UINT32 g_year = DATETIME_DEFAULT_YEAR, g_month = DATETIME_DEFAULT_MONTH, g_day = DATETIME_DEFAULT_DAY;
static UINT32 g_hour = DATETIME_DEFAULT_HOUR, g_min = DATETIME_DEFAULT_MINUTE, g_second = DATETIME_DEFAULT_SECOND;
static char   itemY_Buf[32] = "0";
static char   itemY2_Buf[32] = "0";
static char   itemY3_Buf[32] = "0";
static char   itemY4_Buf[32] = "0";
static char   itemM_Buf[32] = "0";
static char   itemM2_Buf[32] = "0";
static char   itemD_Buf[32] = "0";
static char   itemD2_Buf[32] = "0";
static char   itemHR_Buf[32] = "0";
static char   itemHR2_Buf[32] = "0";
static char   itemMIN_Buf[32] = "0";
static char   itemMIN2_Buf[32] = "0";
static char   itemSEC_Buf[32] = "0";
static char   itemOther0_Buf[32] = "/";
static char   itemOther2_Buf[32] = ":";

_ALIGNED(4) static const UINT8 DayOfMonth[2][12] = {
	// Not leap year
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	// Leap year
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};


static void UIMenuWndSetupDateTime_UpdateInfo(void)
{
#if 0
	/* rect1_x: YYYY MM DD position */
	Ux_RECT rect1_1_1 = { 96 - DATETIME_START_X,  61 - DATETIME_START_Y, 163 - DATETIME_START_X, 117 - DATETIME_START_Y};
	Ux_RECT rect1_1_2 = {116 - DATETIME_START_X,  61 - DATETIME_START_Y, 163 - DATETIME_START_X, 117 - DATETIME_START_Y};
	Ux_RECT rect1_2   = {173 - DATETIME_START_X,  61 - DATETIME_START_Y, 212 - DATETIME_START_X, 117 - DATETIME_START_Y};
	Ux_RECT rect1_3   = {229 - DATETIME_START_X,  61 - DATETIME_START_Y, 268 - DATETIME_START_X, 117 - DATETIME_START_Y};
#endif

	if (g_day > DayOfMonth[GxTime_IsLeapYear(g_year)][g_month - 1]) {
		g_day = DayOfMonth[GxTime_IsLeapYear(g_year)][g_month - 1];
	}

	snprintf(itemY_Buf, 32, "%1ld", g_year / 1000);
	snprintf(itemY2_Buf, 32, "%01ld", (g_year / 100) % 10);
	snprintf(itemY3_Buf, 32, "%01ld", (g_year / 10) % 10);
	snprintf(itemY4_Buf, 32, "%ld", g_year % 10);
	snprintf(itemM_Buf, 32, "%01ld", g_month / 10);
	snprintf(itemM2_Buf, 32, "%ld", g_month % 10);
	snprintf(itemD_Buf, 32, "%01ld", g_day / 10);
	snprintf(itemD2_Buf, 32, "%ld", g_day % 10);
	snprintf(itemHR_Buf, 32, "%01ld", g_hour / 10);
	snprintf(itemHR2_Buf, 32, "%ld", g_hour % 10);
	snprintf(itemMIN_Buf, 32, "%01ld", g_min / 10);
	snprintf(itemMIN2_Buf, 32, "%ld", g_min % 10);
	snprintf(itemSEC_Buf, 32, "%02ld", g_second);
    #if 1//defined(YQCONFIG_TOUCH_FUNCTION_SUPPORT)
    UxStatic_SetData(&UIMenuWndSetupDateTimeTouch_YMD_YCtrl, STATIC_VALUE, Txt_Pointer(itemY_Buf));
    UxStatic_SetData(&UIMenuWndSetupDateTimeTouch_YMD_Y2Ctrl, STATIC_VALUE, Txt_Pointer(itemY2_Buf));
    UxStatic_SetData(&UIMenuWndSetupDateTimeTouch_YMD_Y3Ctrl, STATIC_VALUE, Txt_Pointer(itemY3_Buf));
    UxStatic_SetData(&UIMenuWndSetupDateTimeTouch_YMD_Y4Ctrl, STATIC_VALUE, Txt_Pointer(itemY4_Buf));
    UxStatic_SetData(&UIMenuWndSetupDateTimeTouch_YMD_MCtrl, STATIC_VALUE, Txt_Pointer(itemM_Buf));
    UxStatic_SetData(&UIMenuWndSetupDateTimeTouch_YMD_M2Ctrl, STATIC_VALUE, Txt_Pointer(itemM2_Buf));
    UxStatic_SetData(&UIMenuWndSetupDateTimeTouch_YMD_DCtrl, STATIC_VALUE, Txt_Pointer(itemD_Buf));
    UxStatic_SetData(&UIMenuWndSetupDateTimeTouch_YMD_D2Ctrl, STATIC_VALUE, Txt_Pointer(itemD2_Buf));
    UxStatic_SetData(&UIMenuWndSetupDateTimeTouch_YMD_HRCtrl, STATIC_VALUE, Txt_Pointer(itemHR_Buf));
    UxStatic_SetData(&UIMenuWndSetupDateTimeTouch_YMD_HR2Ctrl, STATIC_VALUE, Txt_Pointer(itemHR2_Buf));
    UxStatic_SetData(&UIMenuWndSetupDateTimeTouch_YMD_MINCtrl, STATIC_VALUE, Txt_Pointer(itemMIN_Buf));
    UxStatic_SetData(&UIMenuWndSetupDateTimeTouch_YMD_MIN2Ctrl, STATIC_VALUE, Txt_Pointer(itemMIN2_Buf));
    #else
	UxButton_SetItemData(&UIMenuWndSetupDateTime_YMD_YCtrl, 0, BTNITM_STRID, Txt_Pointer(itemY_Buf));
	UxButton_SetItemData(&UIMenuWndSetupDateTime_YMD_Y2Ctrl, 0, BTNITM_STRID, Txt_Pointer(itemY2_Buf));
	UxButton_SetItemData(&UIMenuWndSetupDateTime_YMD_Y3Ctrl, 0, BTNITM_STRID, Txt_Pointer(itemY3_Buf));
	UxButton_SetItemData(&UIMenuWndSetupDateTime_YMD_Y4Ctrl, 0, BTNITM_STRID, Txt_Pointer(itemY4_Buf));
	UxButton_SetItemData(&UIMenuWndSetupDateTime_YMD_MCtrl, 0, BTNITM_STRID, Txt_Pointer(itemM_Buf));
	UxButton_SetItemData(&UIMenuWndSetupDateTime_YMD_M2Ctrl, 0, BTNITM_STRID, Txt_Pointer(itemM2_Buf));
	UxButton_SetItemData(&UIMenuWndSetupDateTime_YMD_DCtrl, 0, BTNITM_STRID, Txt_Pointer(itemD_Buf));
	UxButton_SetItemData(&UIMenuWndSetupDateTime_YMD_D2Ctrl, 0, BTNITM_STRID, Txt_Pointer(itemD2_Buf));
	UxButton_SetItemData(&UIMenuWndSetupDateTime_YMD_HRCtrl, 0, BTNITM_STRID, Txt_Pointer(itemHR_Buf));
	UxButton_SetItemData(&UIMenuWndSetupDateTime_YMD_HR2Ctrl, 0, BTNITM_STRID, Txt_Pointer(itemHR2_Buf));
	UxButton_SetItemData(&UIMenuWndSetupDateTime_YMD_MINCtrl, 0, BTNITM_STRID, Txt_Pointer(itemMIN_Buf));
	UxButton_SetItemData(&UIMenuWndSetupDateTime_YMD_MIN2Ctrl, 0, BTNITM_STRID, Txt_Pointer(itemMIN2_Buf));
    #endif
	UxStatic_SetData(&UIMenuWndSetupDateTime_YMD_VALUE_Other0Ctrl, STATIC_VALUE, Txt_Pointer(itemOther0_Buf));
	UxStatic_SetData(&UIMenuWndSetupDateTime_YMD_VALUE_Other1Ctrl, STATIC_VALUE, Txt_Pointer(itemOther0_Buf));
	UxStatic_SetData(&UIMenuWndSetupDateTime_YMD_VALUE_Other2Ctrl, STATIC_VALUE, Txt_Pointer(itemOther2_Buf));
    #if 1//defined(YQCONFIG_TOUCH_FUNCTION_SUPPORT)
	ShowActiveSetting = TRUE;
    UxCtrl_SetShow(g_pActiveSettingCtrl, TRUE);
    #endif
	UxCtrl_SetDirty(&UIMenuWndSetupDateTime_TabCtrl, TRUE);
}
//#NT#2010/06/01#Chris Chung -end

INT32 UIMenuWndSetupDateTime_OnOpen(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    DBG_FUNC_BEGIN("\r\n");
	//#NT#2010/06/01#Chris Chung -begin
    struct tm Curr_DateTime;
    GxTime_GetTime(&Curr_DateTime);

    g_year = Curr_DateTime.tm_year;
	g_month = Curr_DateTime.tm_mon;
	g_day = Curr_DateTime.tm_mday;
	g_hour = Curr_DateTime.tm_hour;
	g_min = Curr_DateTime.tm_min;
	g_second = Curr_DateTime.tm_sec;
    DBG_FUNC("Y=%d, M=%d D=%d\r\n", g_year,g_month, g_day);
    #if 1//defined(YQCONFIG_TOUCH_FUNCTION_SUPPORT)
    g_pActiveSettingCtrl = &UIMenuWndSetupDateTime_YMD_PNL_YCtrl;

    g_uiBlinkTimerID = GxTimer_StartTimer (TIMER_HALF_SEC, NVTEVT_05SEC_TIMER, CONTINUE);
	
    UxCtrl_SetShow(&UIMenuWndSetupDateTime_YMD_PNL_YCtrl, TRUE);
    UxCtrl_SetShow(&UIMenuWndSetupDateTime_YMD_PNL_MCtrl, TRUE);
    UxCtrl_SetShow(&UIMenuWndSetupDateTime_YMD_PNL_DCtrl, TRUE);
    UxCtrl_SetShow(&UIMenuWndSetupDateTime_YMD_PNL_HRCtrl, TRUE);
    UxCtrl_SetShow(&UIMenuWndSetupDateTime_YMD_PNL_MINCtrl, TRUE);
    #else
    UxTab_SetData(&UIMenuWndSetupDateTime_TabCtrl, TAB_FOCUS, UI_DATETIME_IDX_Y3);
    Input_SetKeyMask(KEY_RELEASE, FLGKEY_KEY_MASK_NULL);
    #endif
	UIMenuWndSetupDateTime_UpdateInfo();
	//#NT#2010/06/01#Chris Chung -end
	Ux_DefaultEvent(pCtrl, NVTEVT_OPEN_WINDOW, paramNum, paramArray);
    DBG_FUNC_END("\r\n");
	return NVTEVT_CONSUME;
}
INT32 UIMenuWndSetupDateTime_OnClose(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    DBG_FUNC_BEGIN("\r\n");
	#if 1//defined(YQCONFIG_TOUCH_FUNCTION_SUPPORT)
	if(g_uiBlinkTimerID != NULL_TIMER) {
        GxTimer_StopTimer(&g_uiBlinkTimerID);
	}
	#endif
	/*
    struct tm Curr_DateTime ={0};

	Curr_DateTime.tm_year = g_year ;
	Curr_DateTime.tm_mon = g_month ;
	Curr_DateTime.tm_mday = g_day ;
	Curr_DateTime.tm_hour = g_hour ;
	Curr_DateTime.tm_min = g_min ;
	Curr_DateTime.tm_sec = g_second ;

    GxTime_SetTime(Curr_DateTime);
#if((NVT_ETHREARCAM_RX==ENABLE))
	UINT32 i;
	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP){
			//sync time
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SYNC_TIME, 0);
			EthCam_SendXMLData(i, (UINT8 *)&Curr_DateTime, sizeof(struct tm));
		}
	}
#endif
	*/
	Ux_DefaultEvent(pCtrl, NVTEVT_CLOSE_WINDOW, paramNum, paramArray);
    DBG_FUNC_END("\r\n");
	return NVTEVT_CONSUME;
}
INT32 UIMenuWndSetupDateTime_OnKeyMenu(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    DBG_FUNC_BEGIN("\r\n");
    struct tm Curr_DateTime = {0};

	Curr_DateTime.tm_year = g_year ;
	Curr_DateTime.tm_mon = g_month ;
	Curr_DateTime.tm_mday = g_day ;
	Curr_DateTime.tm_hour = g_hour ;
	Curr_DateTime.tm_min = g_min ;
	Curr_DateTime.tm_sec = g_second ;

    GxTime_SetTime(Curr_DateTime);
#if(defined(_NVT_ETHREARCAM_RX_))
	UINT32 i;
	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP){
			//sync time
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SYNC_TIME, 0);
			EthCam_SendXMLData(i, (UINT8 *)&Curr_DateTime, sizeof(struct tm));
		}
	}
#endif

	Ux_CloseWindow(&MenuCommonItemCtrl, 0); // close whole tab menu
	DBG_FUNC_END("\r\n");
	return NVTEVT_CONSUME;
}
INT32 UIMenuWndSetupDateTime_OnKeyMode(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    DBG_FUNC_BEGIN("\r\n");
    struct tm Curr_DateTime = {0};

	Curr_DateTime.tm_year = g_year ;
	Curr_DateTime.tm_mon = g_month ;
	Curr_DateTime.tm_mday = g_day ;
	Curr_DateTime.tm_hour = g_hour ;
	Curr_DateTime.tm_min = g_min ;
	Curr_DateTime.tm_sec = g_second ;

    GxTime_SetTime(Curr_DateTime);
#if(defined(_NVT_ETHREARCAM_RX_))
	UINT32 i;
	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP){
			//sync time
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SYNC_TIME, 0);
			EthCam_SendXMLData(i, (UINT8 *)&Curr_DateTime, sizeof(struct tm));
		}
	}
#endif

	Ux_SendEvent(&UISetupObjCtrl, NVTEVT_EXE_CHANGEDSCMODE, 1, DSCMODE_CHGTO_NEXT);
    DBG_FUNC_END("\r\n");
	return NVTEVT_CONSUME;
}
#if 1//defined(YQCONFIG_TOUCH_FUNCTION_SUPPORT)
INT32 UIMenuWndSetupDateTime_OnTimer(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    UINT32  uiEvent;

    uiEvent = paramNum ? paramArray[0] : 0;

    switch(uiEvent) {
    case NVTEVT_05SEC_TIMER:
        UxCtrl_SetShow(g_pActiveSettingCtrl, ShowActiveSetting);
        ShowActiveSetting = ShowActiveSetting?FALSE:TRUE;
        break;
    default:
        break;
    }
    return NVTEVT_CONSUME;
}
INT32 UIMenuWndSetupDateTime_OnTouchPanelSlideUp(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    Ux_CloseWindow(&UIMenuWndSetupDateTimeCtrl,0);
    return NVTEVT_CONSUME;
}
INT32 UIMenuWndSetupDateTime_OnTouchPanelSlideDown(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    Ux_CloseWindow(&UIMenuWndSetupDateTimeCtrl,0);
    return NVTEVT_CONSUME;
}

#endif
//----------------------UIMenuWndSetupDateTime_TabCtrl Event---------------------------
INT32 UIMenuWndSetupDateTime_Tab_OnKeySelect(VControl *, UINT32, UINT32 *);
INT32 UIMenuWndSetupDateTime_Tab_OnKeyPrev(VControl *, UINT32, UINT32 *);
INT32 UIMenuWndSetupDateTime_Tab_OnKeyNext(VControl *, UINT32, UINT32 *);
INT32 UIMenuWndSetupDateTime_Tab_OnKeyExit(VControl *, UINT32, UINT32 *);
EVENT_BEGIN(UIMenuWndSetupDateTime_Tab)
EVENT_ITEM(NVTEVT_KEY_NEXT, UIMenuWndSetupDateTime_Tab_OnKeyNext)
EVENT_ITEM(NVTEVT_KEY_PREV, UIMenuWndSetupDateTime_Tab_OnKeyPrev)
EVENT_ITEM(NVTEVT_KEY_SELECT, UIMenuWndSetupDateTime_Tab_OnKeySelect)
EVENT_ITEM(NVTEVT_KEY_SHUTTER2, UIMenuWndSetupDateTime_Tab_OnKeyExit)

EVENT_END

INT32 UIMenuWndSetupDateTime_Tab_OnKeySelect(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    DBG_FUNC_BEGIN("\r\n");
	//#NT#2010/05/19#Chris Chung -begin
	switch (UxTab_GetData(&UIMenuWndSetupDateTime_TabCtrl, TAB_FOCUS)) {
	case UI_DATETIME_IDX_Y3:
		if (g_year >= DATETIME_MAX_YEAR / 10 * 10) {
			g_year = g_year % 10;
		} else {
			g_year = g_year + 10;
			if (g_year > DATETIME_MAX_YEAR) {
				g_year = (g_year / 10) * 10;
			}
		}

		if (g_year < DATETIME_DEFAULT_YEAR) {
			g_year = DATETIME_DEFAULT_YEAR;
		}
		break;
	case UI_DATETIME_IDX_Y4:
		if (g_year % 10 == 9) {
			g_year = (g_year / 10) * 10;
		} else {
			g_year++;
			if (g_year > DATETIME_MAX_YEAR) {
				g_year = (g_year / 10) * 10;
			}
		}

		if (g_year < DATETIME_DEFAULT_YEAR) {
			g_year = DATETIME_DEFAULT_YEAR;
		}

		break;
	case UI_DATETIME_IDX_M:
		if (g_month >= 10) {
			g_month = g_month % 10;
		} else {
			g_month = g_month + 10;
			if (g_month > 12) {
				g_month = (g_month / 10) * 10;
			}
		}
		if (g_month < 1) {
			g_month = 1;
		}
		break;
	case UI_DATETIME_IDX_M2:
		if (g_month % 10 == 9) {
			g_month = (g_month / 10) * 10;
		} else {
			g_month++;
			if (g_month > 12) {
				g_month = (g_month / 10) * 10;
			}
		}
		if (g_month < 1) {
			g_month = 1;
		}

		break;
	case UI_DATETIME_IDX_D:
		DBGD(((DayOfMonth[GxTime_IsLeapYear(g_year)][g_month - 1] / 10) * 10));
		if (g_day >= ((DayOfMonth[GxTime_IsLeapYear(g_year)][g_month - 1] / 10) * 10)) {
			g_day = g_day % 10;
		} else {
			g_day = g_day + 10;
			if (g_day > DayOfMonth[GxTime_IsLeapYear(g_year)][g_month - 1]) {
				g_day = (g_day / 10) * 10;
			}
		}
		if (g_day < 1) {
			g_day = 1;
		}
		break;
	case UI_DATETIME_IDX_D2:
		if (g_day % 10 == 9) {
			g_day = (g_day / 10) * 10;
		} else {
			g_day++;
			if (g_day > DayOfMonth[GxTime_IsLeapYear(g_year)][g_month - 1]) {
				g_day = (g_day / 10) * 10;
			}
		}
		if (g_day < 1) {
			g_day = 1;
		}
		break;
	case UI_DATETIME_IDX_HR:
		if (g_hour >= 20) {
			g_hour = g_hour % 10;
		} else {
			g_hour = g_hour + 10;
			if (g_hour > 23) {
				g_hour = (g_hour / 10) * 10;
			}
		}
		break;
	case UI_DATETIME_IDX_HR2:
		if (g_hour % 10 == 9) {
			g_hour = (g_hour / 10) * 10;
		} else {
			g_hour++;
			if (g_hour > 23) {
				g_hour = (g_hour / 10) * 10;
			}
		}
		break;
	case UI_DATETIME_IDX_MIN:
		if (g_min >= 50) {
			g_min = g_min % 10;
		} else {
			g_min = g_min + 10;
			if (g_min > 59) {
				g_min = (g_min / 10) * 10;
			}
		}
		break;
	case UI_DATETIME_IDX_MIN2:
		if (g_min % 10 == 9) {
			g_min = (g_min / 10) * 10;
		} else {
			g_min++;
			if (g_min > 59) {
				g_min = (g_min / 10) * 10;
			}
		}
		break;
	}

	UIMenuWndSetupDateTime_UpdateInfo();
	//#NT#2010/05/19#Chris Chung -end
	DBG_FUNC_END("\r\n");
	return NVTEVT_CONSUME;
}

INT32 UIMenuWndSetupDateTime_Tab_OnKeyNext(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    DBG_FUNC_BEGIN("\r\n");
	if (UxTab_GetData(&UIMenuWndSetupDateTime_TabCtrl, TAB_FOCUS) == UI_DATETIME_IDX_MIN2) {
		Ux_CloseWindow(&UIMenuWndSetupDateTimeCtrl, 0);
	} else {
		Ux_SendEvent(pCtrl, NVTEVT_NEXT_ITEM, 0);
	}

	UIMenuWndSetupDateTime_UpdateInfo();
    DBG_FUNC_END("\r\n");
	return NVTEVT_CONSUME;
}

INT32 UIMenuWndSetupDateTime_Tab_OnKeyPrev(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    DBG_FUNC_BEGIN("\r\n");
	if (UxTab_GetData(&UIMenuWndSetupDateTime_TabCtrl, TAB_FOCUS) == UI_DATETIME_IDX_Y) {
		Ux_CloseWindow(&UIMenuWndSetupDateTimeCtrl, 0);
	} else {
		Ux_SendEvent(pCtrl, NVTEVT_PREVIOUS_ITEM, 0);
	}

	UIMenuWndSetupDateTime_UpdateInfo();
    DBG_FUNC_END("\r\n");
	return NVTEVT_CONSUME;
}
INT32 UIMenuWndSetupDateTime_Tab_OnKeyExit(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    DBG_FUNC_BEGIN("\r\n");
	Ux_CloseWindow(&MenuCommonItemCtrl, 0);
    DBG_FUNC_END("\r\n");
	return NVTEVT_CONSUME;
}

//----------------------UIMenuWndSetupDateTime_YMD_YCtrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_Y)
EVENT_END

//----------------------UIMenuWndSetupDateTime_YMD_Y2Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_Y2)
EVENT_END

//----------------------UIMenuWndSetupDateTime_YMD_Y3Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_Y3)
EVENT_END

//----------------------UIMenuWndSetupDateTime_YMD_Y4Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_Y4)
EVENT_END

//----------------------UIMenuWndSetupDateTime_YMD_MCtrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_M)
EVENT_END

//----------------------UIMenuWndSetupDateTime_YMD_M2Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_M2)
EVENT_END

//----------------------UIMenuWndSetupDateTime_YMD_DCtrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_D)
EVENT_END

//----------------------UIMenuWndSetupDateTime_YMD_D2Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_D2)
EVENT_END

//----------------------UIMenuWndSetupDateTime_YMD_HRCtrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_HR)
EVENT_END

//----------------------UIMenuWndSetupDateTime_YMD_HR2Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_HR2)
EVENT_END

//----------------------UIMenuWndSetupDateTime_YMD_MINCtrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_MIN)
EVENT_END

//----------------------UIMenuWndSetupDateTime_YMD_MIN2Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_MIN2)
EVENT_END

//----------------------UIMenuWndSetupDateTime_Tab_YMD_VALUECtrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTime_Tab_YMD_VALUE)
EVENT_END

//----------------------UIMenuWndSetupDateTime_YMD_VALUE_Other0Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_VALUE_Other0)
EVENT_END

//----------------------UIMenuWndSetupDateTime_YMD_VALUE_Other1Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_VALUE_Other1)
EVENT_END

//----------------------UIMenuWndSetupDateTime_YMD_VALUE_Other2Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_VALUE_Other2)
EVENT_END

#if 1//defined(YQCONFIG_TOUCH_FUNCTION_SUPPORT)
//----------------------UIMenuWndSetupDateTime_YMD_OKCtrl Event---------------------------
INT32 UIMenuWndSetupDateTime_YMD_OK_OnTouchPanelClick(VControl *, UINT32, UINT32 *);
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_OK)
EVENT_ITEM(NVTEVT_CLICK,UIMenuWndSetupDateTime_YMD_OK_OnTouchPanelClick)
EVENT_END

INT32 UIMenuWndSetupDateTime_YMD_OK_OnTouchPanelClick(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	struct tm Curr_DateTime ={0};

	Curr_DateTime.tm_year = g_year ;
	Curr_DateTime.tm_mon = g_month ;
	Curr_DateTime.tm_mday = g_day ;
	Curr_DateTime.tm_hour = g_hour ;
	Curr_DateTime.tm_min = g_min ;
	Curr_DateTime.tm_sec = g_second ;

	GxTime_SetTime(Curr_DateTime);
#if(defined(_NVT_ETHREARCAM_RX_))
	UINT32 i;
	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP){
			//sync time
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SYNC_TIME, 0);
			EthCam_SendXMLData(i, (UINT8 *)&Curr_DateTime, sizeof(struct tm));
		}
	}
#endif


    Ux_CloseWindow(&UIMenuWndSetupDateTimeCtrl,0);
    return NVTEVT_CONSUME;
}
//----------------------UIMenuWndSetupDateTime_YMD_INCCtrl Event---------------------------
INT32 UIMenuWndSetupDateTime_YMD_INC_OnTouchPanelHold(VControl *, UINT32, UINT32 *);
INT32 UIMenuWndSetupDateTime_YMD_INC_OnTouchPanelClick(VControl *, UINT32, UINT32 *);
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_INC)
EVENT_ITEM(NVTEVT_HOLD,UIMenuWndSetupDateTime_YMD_INC_OnTouchPanelHold)
EVENT_ITEM(NVTEVT_CLICK,UIMenuWndSetupDateTime_YMD_INC_OnTouchPanelClick)
EVENT_END

INT32 UIMenuWndSetupDateTime_YMD_INC_OnTouchPanelHold(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    UIMenuWndSetupDateTime_YMD_INC_OnTouchPanelClick(pCtrl, paramNum, paramArray);
    return NVTEVT_CONSUME;
}

INT32 UIMenuWndSetupDateTime_YMD_INC_OnTouchPanelClick(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    if (g_pActiveSettingCtrl == &UIMenuWndSetupDateTime_YMD_PNL_YCtrl)
    {
        g_year++;
        if (g_year > DATETIME_MAX_YEAR)
        {
            g_year = DATETIME_DEFAULT_YEAR;
        }
    }
    else if (g_pActiveSettingCtrl == &UIMenuWndSetupDateTime_YMD_PNL_MCtrl)
    {
        g_month++;
        if (g_month > 12)
        {
            g_month = 1;
        }
    }
    else if (g_pActiveSettingCtrl == &UIMenuWndSetupDateTime_YMD_PNL_DCtrl)
    {
        g_day++;
        if (g_day > DayOfMonth[GxTime_IsLeapYear(g_year)][g_month-1])
        {
            g_day = 1;
        }
    }
    else if (g_pActiveSettingCtrl == &UIMenuWndSetupDateTime_YMD_PNL_HRCtrl)
    {
        g_hour++;
        if (g_hour > 23)
        {
            g_hour = 0;
        }
    }
    else if (g_pActiveSettingCtrl == &UIMenuWndSetupDateTime_YMD_PNL_MINCtrl)
    {
        g_min++;
        if (g_min > 59)
        {
            g_min = 0;
        }
    }
    UIMenuWndSetupDateTime_UpdateInfo();
    return NVTEVT_CONSUME;
}
//----------------------UIMenuWndSetupDateTime_YMD_DECCtrl Event---------------------------
INT32 UIMenuWndSetupDateTime_YMD_DEC_OnTouchPanelHold(VControl *, UINT32, UINT32 *);
INT32 UIMenuWndSetupDateTime_YMD_DEC_OnTouchPanelClick(VControl *, UINT32, UINT32 *);
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_DEC)
EVENT_ITEM(NVTEVT_HOLD,UIMenuWndSetupDateTime_YMD_DEC_OnTouchPanelHold)
EVENT_ITEM(NVTEVT_CLICK,UIMenuWndSetupDateTime_YMD_DEC_OnTouchPanelClick)
EVENT_END

INT32 UIMenuWndSetupDateTime_YMD_DEC_OnTouchPanelHold(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    UIMenuWndSetupDateTime_YMD_DEC_OnTouchPanelClick(pCtrl, paramNum, paramArray);
    return NVTEVT_CONSUME;
}

INT32 UIMenuWndSetupDateTime_YMD_DEC_OnTouchPanelClick(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    if (g_pActiveSettingCtrl == &UIMenuWndSetupDateTime_YMD_PNL_YCtrl)
    {
        g_year--;
        if (g_year < DATETIME_DEFAULT_YEAR)
        {
            g_year = DATETIME_MAX_YEAR;
        }
    }
    else if (g_pActiveSettingCtrl == &UIMenuWndSetupDateTime_YMD_PNL_MCtrl)
    {
        g_month--;
        if (g_month < 1)
        {
            g_month = 12;
        }
    }
    else if (g_pActiveSettingCtrl == &UIMenuWndSetupDateTime_YMD_PNL_DCtrl)
    {
        g_day--;
        if (g_day < 1)
        {
            g_day = DayOfMonth[GxTime_IsLeapYear(g_year)][g_month-1];
        }
    }
    else if (g_pActiveSettingCtrl == &UIMenuWndSetupDateTime_YMD_PNL_HRCtrl)
    {
        if (g_hour == 0)
        {
            g_hour = 23;
        }
        else
        {
            g_hour--;
        }
    }
    else if (g_pActiveSettingCtrl == &UIMenuWndSetupDateTime_YMD_PNL_MINCtrl)
    {
        if (g_min == 0)
        {
            g_min = 59;
        }
        else
        {
            g_min--;
        }
    }
    UIMenuWndSetupDateTime_UpdateInfo();
    return NVTEVT_CONSUME;
}
//---------------------UIMenuWndSetupDateTime_YMD_PNL_YCtrl Control List---------------------------
CTRL_LIST_BEGIN(UIMenuWndSetupDateTime_YMD_PNL_Y)
CTRL_LIST_ITEM(UIMenuWndSetupDateTimeTouch_YMD_Y)
CTRL_LIST_ITEM(UIMenuWndSetupDateTimeTouch_YMD_Y2)
CTRL_LIST_ITEM(UIMenuWndSetupDateTimeTouch_YMD_Y3)
CTRL_LIST_ITEM(UIMenuWndSetupDateTimeTouch_YMD_Y4)
CTRL_LIST_END

//----------------------UIMenuWndSetupDateTime_YMD_PNL_YCtrl Event---------------------------
INT32 UIMenuWndSetupDateTime_YMD_PNL_Y_OnTouchPanelClick(VControl *, UINT32, UINT32 *);
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_PNL_Y)
EVENT_ITEM(NVTEVT_CLICK,UIMenuWndSetupDateTime_YMD_PNL_Y_OnTouchPanelClick)
EVENT_END

INT32 UIMenuWndSetupDateTime_YMD_PNL_Y_OnTouchPanelClick(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    UxCtrl_SetShow(g_pActiveSettingCtrl, TRUE);
    g_pActiveSettingCtrl = pCtrl;
    UxCtrl_SetShow(g_pActiveSettingCtrl, FALSE);
    return NVTEVT_CONSUME;
}
//----------------------UIMenuWndSetupDateTimeTouch_YMD_YCtrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTimeTouch_YMD_Y)
EVENT_END

//----------------------UIMenuWndSetupDateTimeTouch_YMD_Y2Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTimeTouch_YMD_Y2)
EVENT_END

//----------------------UIMenuWndSetupDateTimeTouch_YMD_Y3Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTimeTouch_YMD_Y3)
EVENT_END

//----------------------UIMenuWndSetupDateTimeTouch_YMD_Y4Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTimeTouch_YMD_Y4)
EVENT_END

//---------------------UIMenuWndSetupDateTime_YMD_PNL_MCtrl Control List---------------------------
CTRL_LIST_BEGIN(UIMenuWndSetupDateTime_YMD_PNL_M)
CTRL_LIST_ITEM(UIMenuWndSetupDateTimeTouch_YMD_M)
CTRL_LIST_ITEM(UIMenuWndSetupDateTimeTouch_YMD_M2)
CTRL_LIST_END

//----------------------UIMenuWndSetupDateTime_YMD_PNL_MCtrl Event---------------------------
INT32 UIMenuWndSetupDateTime_YMD_PNL_M_OnTouchPanelClick(VControl *, UINT32, UINT32 *);
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_PNL_M)
EVENT_ITEM(NVTEVT_CLICK,UIMenuWndSetupDateTime_YMD_PNL_M_OnTouchPanelClick)
EVENT_END

INT32 UIMenuWndSetupDateTime_YMD_PNL_M_OnTouchPanelClick(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    UxCtrl_SetShow(g_pActiveSettingCtrl, TRUE);
    g_pActiveSettingCtrl = pCtrl;
    UxCtrl_SetShow(g_pActiveSettingCtrl, FALSE);
    return NVTEVT_CONSUME;
}
//----------------------UIMenuWndSetupDateTimeTouch_YMD_MCtrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTimeTouch_YMD_M)
EVENT_END

//----------------------UIMenuWndSetupDateTimeTouch_YMD_M2Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTimeTouch_YMD_M2)
EVENT_END

//---------------------UIMenuWndSetupDateTime_YMD_PNL_DCtrl Control List---------------------------
CTRL_LIST_BEGIN(UIMenuWndSetupDateTime_YMD_PNL_D)
CTRL_LIST_ITEM(UIMenuWndSetupDateTimeTouch_YMD_D)
CTRL_LIST_ITEM(UIMenuWndSetupDateTimeTouch_YMD_D2)
CTRL_LIST_END

//----------------------UIMenuWndSetupDateTime_YMD_PNL_DCtrl Event---------------------------
INT32 UIMenuWndSetupDateTime_YMD_PNL_D_OnTouchPanelClick(VControl *, UINT32, UINT32 *);
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_PNL_D)
EVENT_ITEM(NVTEVT_CLICK,UIMenuWndSetupDateTime_YMD_PNL_D_OnTouchPanelClick)
EVENT_END

INT32 UIMenuWndSetupDateTime_YMD_PNL_D_OnTouchPanelClick(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    UxCtrl_SetShow(g_pActiveSettingCtrl, TRUE);
    g_pActiveSettingCtrl = pCtrl;
    UxCtrl_SetShow(g_pActiveSettingCtrl, FALSE);
    return NVTEVT_CONSUME;
}
//----------------------UIMenuWndSetupDateTimeTouch_YMD_DCtrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTimeTouch_YMD_D)
EVENT_END

//----------------------UIMenuWndSetupDateTimeTouch_YMD_D2Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTimeTouch_YMD_D2)
EVENT_END

//---------------------UIMenuWndSetupDateTime_YMD_PNL_HRCtrl Control List---------------------------
CTRL_LIST_BEGIN(UIMenuWndSetupDateTime_YMD_PNL_HR)
CTRL_LIST_ITEM(UIMenuWndSetupDateTimeTouch_YMD_HR)
CTRL_LIST_ITEM(UIMenuWndSetupDateTimeTouch_YMD_HR2)
CTRL_LIST_END

//----------------------UIMenuWndSetupDateTime_YMD_PNL_HRCtrl Event---------------------------
INT32 UIMenuWndSetupDateTime_YMD_PNL_HR_OnTouchPanelClick(VControl *, UINT32, UINT32 *);
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_PNL_HR)
EVENT_ITEM(NVTEVT_CLICK,UIMenuWndSetupDateTime_YMD_PNL_HR_OnTouchPanelClick)
EVENT_END

INT32 UIMenuWndSetupDateTime_YMD_PNL_HR_OnTouchPanelClick(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    UxCtrl_SetShow(g_pActiveSettingCtrl, TRUE);
    g_pActiveSettingCtrl = pCtrl;
    UxCtrl_SetShow(g_pActiveSettingCtrl, FALSE);
    return NVTEVT_CONSUME;
}
//----------------------UIMenuWndSetupDateTimeTouch_YMD_HRCtrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTimeTouch_YMD_HR)
EVENT_END

//----------------------UIMenuWndSetupDateTimeTouch_YMD_HR2Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTimeTouch_YMD_HR2)
EVENT_END

//---------------------UIMenuWndSetupDateTime_YMD_PNL_MINCtrl Control List---------------------------
CTRL_LIST_BEGIN(UIMenuWndSetupDateTime_YMD_PNL_MIN)
CTRL_LIST_ITEM(UIMenuWndSetupDateTimeTouch_YMD_MIN)
CTRL_LIST_ITEM(UIMenuWndSetupDateTimeTouch_YMD_MIN2)
CTRL_LIST_END

//----------------------UIMenuWndSetupDateTime_YMD_PNL_MINCtrl Event---------------------------
INT32 UIMenuWndSetupDateTime_YMD_PNL_MIN_OnTouchPanelClick(VControl *, UINT32, UINT32 *);
EVENT_BEGIN(UIMenuWndSetupDateTime_YMD_PNL_MIN)
EVENT_ITEM(NVTEVT_CLICK,UIMenuWndSetupDateTime_YMD_PNL_MIN_OnTouchPanelClick)
EVENT_END

INT32 UIMenuWndSetupDateTime_YMD_PNL_MIN_OnTouchPanelClick(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    UxCtrl_SetShow(g_pActiveSettingCtrl, TRUE);
    g_pActiveSettingCtrl = pCtrl;
    UxCtrl_SetShow(g_pActiveSettingCtrl, FALSE);
    return NVTEVT_CONSUME;
}
//----------------------UIMenuWndSetupDateTimeTouch_YMD_MINCtrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTimeTouch_YMD_MIN)
EVENT_END

//----------------------UIMenuWndSetupDateTimeTouch_YMD_MIN2Ctrl Event---------------------------
EVENT_BEGIN(UIMenuWndSetupDateTimeTouch_YMD_MIN2)
EVENT_END
#endif

