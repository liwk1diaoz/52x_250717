#include "PrjInc.h"
#include "UIFlowLVGL/UIFlowLVGL.h"
#include "UIFlowLVGL/UIFlowWaitMoment/UIFlowWaitMomentAPI.h"
#include <kwrap/debug.h>

//static UINT32 g_uiWaitMomentMsg     = 0;
static UINT32 g_uiRestart_Rec     = 0;
//static char g_StringTmpBuf[64] = {0};
//static UINT32  g_UIStopRecTimerID = NULL_TIMER;
static BOOL g_bRecStopFinish     = 0;
static UINT32 g_uiRecStopTimerCnt     = 0;

void  UIFlowWaitMomentAPI_Set_Text(UIFlowWaitMomentAPI_Set_Text_Param param)
{
	lv_obj_t* label = label_1_scr_uiflowwaitmoment;

	if(param.text){
		DBG_IND("set text with custom text [%s]\r\n", param.text);

		/* update font without string id check */
		lv_plugin_label_set_text(label, LV_PLUGIN_RES_ID_NONE);

		lv_label_set_text(label, param.text);
		lv_plugin_label_update_font(label, LV_LABEL_PART_MAIN);
	}
	else if(param.string_id != LV_PLUGIN_RES_ID_NONE){
		DBG_IND("set text with string_id[%lu]\r\n", param.string_id);
		lv_plugin_label_set_text(label, param.string_id);
		lv_plugin_label_update_font(label, LV_LABEL_PART_MAIN);
	}
	else{
		DBG_ERR("can't set wait moment label text!\r\n");
	}
}

void UIFlowWaitMoment_OnOpen(lv_obj_t* obj, const void *data)
{
	lv_obj_t* scr = lv_plugin_scr_act();
	lv_group_t* gp = lv_obj_get_group(scr);

	lv_group_add_obj(gp, obj);
	lv_group_focus_obj(obj);

	if(data){

		UIFlowWaitMomentAPI_Set_Text_Param* param = (UIFlowWaitMomentAPI_Set_Text_Param*)data;

		if(param){
			UIFlowWaitMomentAPI_Set_Text(*param);
		}
		else{
			DBG_WRN("open screen without event data\r\n");
		}
	}
}

void UIFlowWaitMoment_OnCloseOpen(lv_obj_t* obj)
{
	DBG_DUMP("%s", __func__);

	lv_group_remove_obj(obj);

}

void UIFlowWaitMoment_OnNVTMSG_BackgroundDone(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	NVTEVT event;
	UINT32 uiReturn = 0;
	UINT32 status;

	event = msg->paramArray[ONDONE_PARAM_INDEX_CMD];
	status = msg->paramArray[ONDONE_PARAM_INDEX_RET];
	DBG_FUNC_BEGIN("event = 0x%x, status=%d\r\n", event, status);

	switch (event) {
	case NVTEVT_BKW_COPY2CARD: {
			switch (status) {
			case 1: // TRUE
				uiReturn = NVTRET_COPY2CARD;
				break;
			case 0:
				uiReturn = NVTRET_CANCEL;
			}
			//Rsvd
			break;
		}

	case NVTEVT_BKW_DELALL: {
			uiReturn = NVTRET_DELETEALL;
			//Rsvd
			break;
		}


	case NVTEVT_BKW_FORMAT_CARD: {
			uiReturn = NVTRET_FORMAT;
			//Rsvd
			break;
		}

	case NVTEVT_BKW_FORMAT_NAND: {
			uiReturn = NVTRET_FORMAT;
			//Rsvd
			break;
		}

	case NVTEVT_BKW_FORMAT: {
			uiReturn = NVTRET_FORMAT;
			//Rsvd
			break;
		}

	case NVTEVT_BKW_SETPROTECT: {
			uiReturn = NVTRET_PROTECT;
			//Rsvd
			break;
		}
	case NVTEVT_BKW_WIFI_ON: {
#if (WIFI_FUNC==ENABLE)
			Ux_PostEvent(NVTEVT_SYSTEM_MODE, 2, PRIMARY_MODE_MOVIE, SYS_SUBMODE_WIFI);
#endif
//			return NVTEVT_CONSUME;
			return;
		}
	case NVTEVT_BKW_FW_UPDATE: {
			if (status) {
				DBG_ERR("update fail and status=%d\r\n", status);
			} else {
			    DBG_DUMP("FW UPDATE OK and Delay 2000ms to System_PowerOff\r\n");
				Delay_DelayMs(2000);
				// Should power off immediatelly
				System_PowerOff(SYS_POWEROFF_NORMAL);
			}
			break;
		}
	case NVTEVT_BKW_STOPREC_PROCESS: {
			//debug_err(("NVTEVT_BKW_STOPREC_PROCESS, Restart_Rec=%d\r\n",g_uiRestart_Rec));
			//Delay_DelayMs(300);
			//#NT#2018/09/14#KCHong -begin
			//#NT# retore key mask when stop record flow done
			Input_SetKeyMask(KEY_PRESS, (FLGKEY_SHUTTER2|FLGKEY_RIGHT|FLGKEY_LEFT|FLGKEY_CUSTOM1|FLGKEY_UP|FLGKEY_DOWN));
			//#NT#2018/09/14#KCHong -end
			if(g_uiRestart_Rec){
        			uiReturn = NVTRET_RESTART_REC;
			}else{
        			uiReturn = 0;
        			g_bRecStopFinish=1;
        			if(g_uiRecStopTimerCnt){
//        			    return NVTEVT_CONSUME;
        				return;
        			}
			}
		}
			break;
	default:
		uiReturn = NVTRET_ERROR;
		DBG_ERR("Unknown event 0x%x\r\n", event);
		break;
	}

	lv_plugin_scr_close(obj, gen_nvtmsg_data(NVTRET_WAITMOMENT, 1, uiReturn));
//	Ux_CloseWindow(pCtrl, 2, NVTRET_WAITMOMENT, uiReturn);

    DBG_FUNC_END("\r\n");

}

void UIFlowWndWaitMoment_OnKeyShutter2(lv_obj_t* obj)
{
//	UINT32 uiKeyAct;
//	UINT32 uiState;
//
//	DBG_FUNC_BEGIN("\r\n");
//	uiKeyAct = paramNum ? paramArray[0] : 0;
//
//	if (paramNum >= 3) {
//		uiState = paramArray[2];
//	} else {
//		uiState = 0;
//	}
//
//	DBG_FUNC("uiKeyAct=%d, uiState=0x%x\r\n",uiKeyAct,uiState);
//	switch (uiKeyAct) {
//	case NVTEVT_KEY_PRESS:
//		DBG_IND("NVTEVT_KEY_PRESS\r\n");
//		if (uiState == UIFlowWndMovie_Restart_Rec) {
//			g_uiRestart_Rec = 1;
//		} else {
//			g_uiRestart_Rec = 0;
//		}
//	break;
//		}
//    DBG_FUNC_END("\r\n");

}


static void UIFlowWaitMoment_OnNVTMSG(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{

	switch(msg->event)
	{
	case NVTEVT_BACKGROUND_DONE:
		UIFlowWaitMoment_OnNVTMSG_BackgroundDone(obj, msg);
		break;
	}
}

void UIFlowWaitMomentEventCallback(lv_obj_t* obj, lv_event_t event)
{

	switch(event)
	{

	case LV_PLUGIN_EVENT_SCR_OPEN:
		UIFlowWaitMoment_OnOpen(obj, lv_event_get_data());
		break;

	case LV_PLUGIN_EVENT_SCR_CLOSE:
		UIFlowWaitMoment_OnCloseOpen(obj);
		break;

	case LV_EVENT_KEY:
	{
		uint32_t* key = (uint32_t*)lv_event_get_data();

		LV_UNUSED(key); /* avoid compile error, please remove it manually */
		break;
	}

	case LV_USER_EVENT_NVTMSG:
	{
		const LV_USER_EVENT_NVTMSG_DATA* msg = (const LV_USER_EVENT_NVTMSG_DATA*)lv_event_get_data();

		UIFlowWaitMoment_OnNVTMSG(obj, msg);
		break;
	}

	default:
		break;

	}

}

