#include "PrjInc.h"
#include "GxTime.h"
#include "UIFlowLVGL/UIFlowLVGL.h"
#include <kwrap/debug.h>

#define DATETIME_MAX_YEAR       2050
#define DATETIME_DEFAULT_YEAR   2000
#define DATETIME_DEFAULT_MONTH     1
#define DATETIME_DEFAULT_DAY       1
#define DATETIME_DEFAULT_HOUR      0
#define DATETIME_DEFAULT_MINUTE    0
#define DATETIME_DEFAULT_SECOND    0

static lv_obj_t* btnmatrix = NULL;

static lv_group_t* gp = NULL;

static void set_indev_keypad_group(lv_obj_t* obj)
{
	if(gp == NULL){
		gp = lv_group_create();
		lv_group_add_obj(gp, obj);
	}

	lv_indev_t* indev = lv_plugin_find_indev_by_type(LV_INDEV_TYPE_KEYPAD);
	lv_indev_set_group(indev, gp);
}

/***************************************************************************************
 * this enum is mapping to btnmatrix's btn map, not btn id (newline is not counted)
 ***************************************************************************************/
typedef enum {
	UI_DATETIME_IDX_Y,
	UI_DATETIME_IDX_Y2,
	UI_DATETIME_IDX_Y3,
	UI_DATETIME_IDX_Y4,
	UI_DATETIME_IDX_SEP1,
	UI_DATETIME_IDX_M,
	UI_DATETIME_IDX_M2,
	UI_DATETIME_IDX_SEP2,
	UI_DATETIME_IDX_D,
	UI_DATETIME_IDX_D2,
	UI_DATETIME_IDX_NEWLINE1,
	UI_DATETIME_IDX_SPACE1,
	UI_DATETIME_IDX_SPACE2,
	UI_DATETIME_IDX_SPACE3,
	UI_DATETIME_IDX_HR,
	UI_DATETIME_IDX_HR2,
	UI_DATETIME_IDX_COMMA1,
	UI_DATETIME_IDX_MIN,
	UI_DATETIME_IDX_MIN2,
	UI_DATETIME_IDX_SPACE4,
	UI_DATETIME_IDX_SPACE5,
	UI_DATETIME_IDX_SPACE6,
	UI_DATETIME_IDX_END,
	UI_DATETIME_IDX_NUM,
	UI_DATETIME_IDX_INVALID
} UI_DATETIME_IDX;

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
static char   itemNewline_Buf[4] = "\n";
static char   itemSpace_Buf[4] = " ";
static char   itemComma_Buf[4] = ":";
static char   itemEnd_Buf[4] = "";
static char   itemBackslash_Buf[4] = "/";

/*****************************************************
 * newline: buttons after newline will be positioned to the next line
 * space: padding
 * empty string: indicate end of cmap (lvgl btnmatrix)
 *
 *****************************************************/
static const char* cmap[UI_DATETIME_IDX_NUM] =
{
		itemY_Buf,
		itemY2_Buf,
		itemY3_Buf,
		itemY4_Buf,
		itemBackslash_Buf,
		itemM_Buf,
		itemM2_Buf,
		itemBackslash_Buf,
		itemD_Buf,
		itemD2_Buf,
		itemNewline_Buf,
		itemSpace_Buf,
		itemSpace_Buf,
		itemSpace_Buf,
		itemHR_Buf,
		itemHR2_Buf,
		itemComma_Buf,
		itemMIN_Buf,
		itemMIN2_Buf,
		itemSpace_Buf,
		itemSpace_Buf,
		itemSpace_Buf,
		itemEnd_Buf
};

/* indicate the ui idx is a valid btn id*/
static bool ui_idx_is_valid(UI_DATETIME_IDX idx)
{
	switch(idx)
	{
	case UI_DATETIME_IDX_Y:
	case UI_DATETIME_IDX_Y2:
	case UI_DATETIME_IDX_Y3:
	case UI_DATETIME_IDX_Y4:
	case UI_DATETIME_IDX_M:
	case UI_DATETIME_IDX_M2:
	case UI_DATETIME_IDX_D:
	case UI_DATETIME_IDX_D2:
	case UI_DATETIME_IDX_HR:
	case UI_DATETIME_IDX_HR2:
	case UI_DATETIME_IDX_MIN:
	case UI_DATETIME_IDX_MIN2:
		return true;

	default:
		return false;
	}
}

static uint16_t btnmatrix_get_btn_id(UI_DATETIME_IDX idx)
{
	uint16_t i, newline_cnt = 0, ret = LV_BTNMATRIX_BTN_NONE;

	for(i=0 ; i<UI_DATETIME_IDX_NUM ; i++)
	{
		if(strlen(cmap[i]) == 0)
		{
			ret = LV_BTNMATRIX_BTN_NONE;
			break;
		}
		else if(strcmp(cmap[i], itemNewline_Buf) == 0)
		{
			newline_cnt++;
		}
		else if(idx == i){
			ret = i - newline_cnt;
			break;
		}
	}

	return ret;
}

static UI_DATETIME_IDX btnmatrix_btn_id_to_ui_idx(uint16_t btn_id)
{
	UI_DATETIME_IDX ret = UI_DATETIME_IDX_INVALID;
	uint16_t i, newline_cnt = 0;

	for(i=0 ; i<UI_DATETIME_IDX_NUM ; i++)
	{
		if(strlen(cmap[i]) == 0)
		{
			ret = UI_DATETIME_IDX_INVALID;
			break;
		}
		else if(strcmp(cmap[i], itemNewline_Buf) == 0)
		{
			newline_cnt++;
		}
		else if((i-newline_cnt) == btn_id){
			ret = i;
			break;
		}
	}

	return ret;
}


static uint16_t btnmatrix_focus_prev_btn(void)
{
	uint16_t id = lv_btnmatrix_get_focused_btn(btnmatrix);


	if(id > 0){

		do{

			DBG_DUMP("%s id=%u\r\n", __func__, id);

			if(lv_btnmatrix_get_btn_ctrl(btnmatrix, --id, LV_BTNMATRIX_CTRL_DISABLED))
				continue;

			lv_btnmatrix_set_focused_btn(btnmatrix, id);

			if(lv_btnmatrix_get_focused_btn(btnmatrix) != id)
				return false;
			else
				return true;

		} while( id > 0);

		return false;
	}

	return false;

}

static bool btnmatrix_focus_next_btn(void)
{
	uint16_t id = lv_btnmatrix_get_focused_btn(btnmatrix);

	do{
		if(lv_btnmatrix_get_btn_ctrl(btnmatrix, ++id, LV_BTNMATRIX_CTRL_DISABLED))
			continue;

		lv_btnmatrix_set_focused_btn(btnmatrix, id);

		if(lv_btnmatrix_get_focused_btn(btnmatrix) != id){
			return false;
		}
		else{
			return true;
		}

	} while(true);

	DBG_DUMP("%s return false1\r\n", __func__);
	return false;
}

_ALIGNED(4) static const UINT8 DayOfMonth[2][12] = {
	// Not leap year
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	// Leap year
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

static void btnmatrix_setup(void)
{
	lv_obj_t* obj = btnmatrix;

	lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_NO_REPEAT);
    lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_DISABLED);
    lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECK_STATE);
    lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CLICK_TRIG);


    for(uint16_t i=0 ; i<UI_DATETIME_IDX_NUM ; i++)
    {

    	if(!ui_idx_is_valid(i))
    	{
    		lv_btnmatrix_set_btn_ctrl(btnmatrix, btnmatrix_get_btn_id(i), LV_BTNMATRIX_CTRL_DISABLED);
    	}
    }
}

static void UIMenuWndSetupDateTime_UpdateInfo(void)
{
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

	lv_btnmatrix_set_map(btnmatrix, cmap);
}


static void UIFlowSetupDateTime_OnOpen(lv_obj_t* obj)
{
	DBG_DUMP("%s\r\n", __func__);
	set_indev_keypad_group(obj);

	btnmatrix = button_matrix_datetime_scr_uiflowsetupdatetime;

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

    DBG_DUMP("btnmatrix_get_btn_id(UI_DATETIME_IDX_Y3) = %u\r\n", btnmatrix_get_btn_id(UI_DATETIME_IDX_Y3));

    lv_btnmatrix_set_focused_btn(btnmatrix, btnmatrix_get_btn_id(UI_DATETIME_IDX_Y3));

//    UxTab_SetData(&UIMenuWndSetupDateTime_TabCtrl, TAB_FOCUS, UI_DATETIME_IDX_Y3);
//	Input_SetKeyMask(KEY_RELEASE, FLGKEY_KEY_MASK_NULL);
	UIMenuWndSetupDateTime_UpdateInfo();

	btnmatrix_setup();
	//#NT#2010/06/01#Chris Chung -end
//	Ux_DefaultEvent(pCtrl, NVTEVT_OPEN_WINDOW, paramNum, paramArray);
//    DBG_FUNC_END("\r\n");
//	return NVTEVT_CONSUME;

}

void UIFlowSetupDateTime_OnClose(lv_obj_t* obj)
{
	DBG_DUMP("%s\r\n", __func__);
    DBG_FUNC_BEGIN("\r\n");
    struct tm Curr_DateTime ={0};

	Curr_DateTime.tm_year = g_year ;
	Curr_DateTime.tm_mon = g_month ;
	Curr_DateTime.tm_mday = g_day ;
	Curr_DateTime.tm_hour = g_hour ;
	Curr_DateTime.tm_min = g_min ;
	Curr_DateTime.tm_sec = g_second ;

    GxTime_SetTime(Curr_DateTime);

//	Ux_DefaultEvent(pCtrl, NVTEVT_CLOSE_WINDOW, paramNum, paramArray);
    DBG_FUNC_END("\r\n");
//	return NVTEVT_CONSUME;
}

void UIMenuWndSetupDateTime_OnKeyMenu(lv_obj_t* obj)
{
	DBG_DUMP("%s\r\n", __func__);
    DBG_FUNC_BEGIN("\r\n");
    struct tm Curr_DateTime = {0};

	Curr_DateTime.tm_year = g_year ;
	Curr_DateTime.tm_mon = g_month ;
	Curr_DateTime.tm_mday = g_day ;
	Curr_DateTime.tm_hour = g_hour ;
	Curr_DateTime.tm_min = g_min ;
	Curr_DateTime.tm_sec = g_second ;

    GxTime_SetTime(Curr_DateTime);
//	Ux_CloseWindow(&MenuCommonItemCtrl, 0); // close whole tab menu
	DBG_FUNC_END("\r\n");
//	return NVTEVT_CONSUME;
}

static void  UIMenuWndSetupDateTime_OnKeyNext(lv_obj_t* obj)
{
	DBG_DUMP("%s\r\n", __func__);
    DBG_FUNC_BEGIN("\r\n");

//    uint16_t focused_btn = lv_btnmatrix_get_focused_btn(btn_matrix);
//    UI_DATETIME_IDX idx = btnmatrix_btn_id_to_ui_idx(focused_btn);
//
//    if(idx == UI_DATETIME_IDX_MIN2){
//    	lv_plugin_scr_close(obj);
//    }
//    else{
//
//    }

    if(!btnmatrix_focus_next_btn()){
    	lv_plugin_scr_close(obj, NULL);
    }

//    UIMenuWndSetupDateTime_UpdateInfo();

//	if (UxTab_GetData(&UIMenuWndSetupDateTime_TabCtrl, TAB_FOCUS) == UI_DATETIME_IDX_MIN2) {
//		Ux_CloseWindow(&UIMenuWndSetupDateTimeCtrl, 0);
//	} else {
//		Ux_SendEvent(pCtrl, NVTEVT_NEXT_ITEM, 0);
//	}

    DBG_FUNC_END("\r\n");
//	return NVTEVT_CONSUME;
}

static void  UIMenuWndSetupDateTime_OnKeyShutter2(lv_obj_t* obj)
{
	DBG_DUMP("%s\r\n", __func__);

    lv_plugin_scr_close(obj, NULL);
}

static void  UIMenuWndSetupDateTime_OnKeyPrev(lv_obj_t* obj)
{
	DBG_DUMP("%s\r\n", __func__);

    if(!btnmatrix_focus_prev_btn()){
    	lv_plugin_scr_close(obj, NULL);
    }

}

static void  UIMenuWndSetupDateTime_OnKeyMode(lv_obj_t* obj)
{
	DBG_DUMP("%s\r\n", __func__);

    DBG_FUNC_BEGIN("\r\n");
    struct tm Curr_DateTime = {0};

	Curr_DateTime.tm_year = g_year ;
	Curr_DateTime.tm_mon = g_month ;
	Curr_DateTime.tm_mday = g_day ;
	Curr_DateTime.tm_hour = g_hour ;
	Curr_DateTime.tm_min = g_min ;
	Curr_DateTime.tm_sec = g_second ;

    GxTime_SetTime(Curr_DateTime);

	Ux_SendEvent(&UISetupObjCtrl, NVTEVT_EXE_CHANGEDSCMODE, 1, DSCMODE_CHGTO_NEXT);
    DBG_FUNC_END("\r\n");
//	return NVTEVT_CONSUME;
}

static void  UIMenuSetupDateTime_OnKey(lv_obj_t* obj, uint32_t key)
{
	DBG_DUMP("%s\r\n", __func__);


	switch(key)
	{
	case LV_USER_KEY_MODE:
		UIMenuWndSetupDateTime_OnKeyMode(obj);
		break;

	case LV_USER_KEY_MENU:
		UIMenuWndSetupDateTime_OnKeyMenu(obj);
		break;

	case LV_USER_KEY_NEXT:
		UIMenuWndSetupDateTime_OnKeyNext(obj);
		break;

	case LV_USER_KEY_PREV:
		UIMenuWndSetupDateTime_OnKeyPrev(obj);
		break;

	case LV_USER_KEY_SHUTTER2:
		UIMenuWndSetupDateTime_OnKeyShutter2(obj);
		break;

	default:
		break;
	}

}

void UIMenuWndSetupDateTime_OnKeySelect(lv_obj_t* obj)
{
	DBG_DUMP("%s\r\n", __func__);
    DBG_FUNC_BEGIN("\r\n");
	//#NT#2010/05/19#Chris Chung -begin
//    btnmatrix_btn_id_to_ui_idx
//	switch (UxTab_GetData(&UIMenuWndSetupDateTime_TabCtrl, TAB_FOCUS)) {


    uint16_t focused_btn = lv_btnmatrix_get_focused_btn(btnmatrix);
    UI_DATETIME_IDX idx = btnmatrix_btn_id_to_ui_idx(focused_btn);

    DBG_DUMP("%s focused ui idx = %u\r\n", __func__, idx);

    switch (idx) {
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

	default:
		break;
	}

	UIMenuWndSetupDateTime_UpdateInfo();

	lv_btnmatrix_set_focused_btn(btnmatrix, focused_btn);

	//#NT#2010/05/19#Chris Chung -end
	DBG_FUNC_END("\r\n");
//	return NVTEVT_CONSUME;
}

void UIFlowSetupDateTimeEventCallback(lv_obj_t* obj, lv_event_t event)
{

	switch(event)
	{

	case LV_PLUGIN_EVENT_SCR_OPEN:
		UIFlowSetupDateTime_OnOpen(obj);
		break;

	case LV_PLUGIN_EVENT_SCR_CLOSE:
		UIFlowSetupDateTime_OnClose(obj);
		break;

	case LV_EVENT_KEY:
	{
		uint32_t* key = (uint32_t*)lv_event_get_data();

		if(key){
			UIMenuSetupDateTime_OnKey(obj, *key);
		}

		/***********************************************************************************
		 * IMPORTANT!!
		 *
		 * calling lv_indev_wait_release to avoid duplicate event in long pressed key state
		 * the event will not be sent again until released
		 *
		 ***********************************************************************************/
		if(*key != LV_KEY_ENTER)
			lv_indev_wait_release(lv_indev_get_act());

		break;
	}

	case LV_EVENT_CLICKED:
		UIMenuWndSetupDateTime_OnKeySelect(obj);
		break;

	default:
		break;

	}

}

