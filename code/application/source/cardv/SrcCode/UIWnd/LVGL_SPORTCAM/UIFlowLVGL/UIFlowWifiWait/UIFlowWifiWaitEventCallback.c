#include "PrjInc.h"
#include "UIFlowLVGL/UIFlowLVGL.h"
#include <kwrap/debug.h>
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
BOOL bIsWiFiRecorded;

static void UIFlowWifiWait_NVTMSG(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{

	switch(msg->event)
	{

	case NVTEVT_BACKGROUND_DONE:
	{
		NVTEVT message = msg->paramArray[ONDONE_PARAM_INDEX_CMD];
		switch (message) {
        case NVTEVT_BKW_WIFI_ON: {
                //open wifi link
            if (UI_GetData(FL_WIFI_LINK) == WIFI_LINK_OK) {
			//change to wifi mode,would close wait wnd
			//change to wifi mode would auto start record movie
            	bIsWiFiRecorded =  FALSE;
#if (WIFI_FUNC==ENABLE)
                if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_MOVIE){
                    #if _TODO
                    Ux_PostEvent(NVTEVT_SYSTEM_MODE, 1, PRIMARY_MODE_MOVIE);
                    #endif
                }
                //Ux_PostEvent(NVTEVT_OPEN_WINDOW, 1, &UIMenuWndWiFiModuleLinkCtrl);
                lv_plugin_scr_open(UIFlowWifiLink, NULL);
                Ux_PostEvent(NVTEVT_EXE_MOVIE_STRM_START,0);

#endif
		    } else {
			    //Ux_CloseWindow(pCtrl, 0);
                lv_plugin_scr_close(obj, NULL);
#if (WIFI_AP_FUNC==ENABLE)
			    //Ux_OpenWindow(&UIMenuWndWiFiModuleLinkCtrl, 0); //show link fail
                lv_plugin_scr_open(UIFlowWifiLink, NULL);
#endif
		    }
            
            }
            break;

		default:
		    break;
		}
	}


	default:
		break;

	}
}

static void UIFlowWifiWait_ChildScrClose(lv_obj_t* obj, LV_USER_EVENT_NVTMSG_DATA* data)
{
	DBG_DUMP("%s\r\n", __func__);

	/* once child scr closed, set keypad group again */
	set_indev_keypad_group(obj);

	//Ux_DefaultEvent(pCtrl, NVTEVT_CHILD_CLOSE, paramNum, paramArray);
	//return NVTEVT_CONSUME;
}

void UIFlowWifiWaitEventCallback(lv_obj_t* obj, lv_event_t event)
{

	switch(event)
	{

	case LV_PLUGIN_EVENT_SCR_OPEN:
        DBG_DUMP("UIFlowWifiWait_ScrOpen\r\n");
        //if(gp == NULL){
        //    gp = lv_group_create();
        //    lv_group_add_obj(gp, obj);
        //}
        
        //lv_indev_t* indev = lv_plugin_find_indev_by_type(LV_INDEV_TYPE_KEYPAD);
        //lv_indev_set_group(indev, gp);
        set_indev_keypad_group(obj);
		break;

	case LV_PLUGIN_EVENT_SCR_CLOSE:
        DBG_DUMP("UIFlowWifiWait_ScrClose\r\n");
		break;

	case LV_PLUGIN_EVENT_CHILD_SCR_CLOSE:
		UIFlowWifiWait_ChildScrClose(obj, (LV_USER_EVENT_NVTMSG_DATA*)lv_event_get_data());
		break;

	case LV_EVENT_KEY:
	{
		uint32_t* key = (uint32_t*)lv_event_get_data();

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
		//LV_UNUSED(key); /* avoid compile error, please remove it manually */
		//break;;
	}

	/* handle nvt event except key event */
	case LV_USER_EVENT_NVTMSG:
	{
		const LV_USER_EVENT_NVTMSG_DATA* msg = (const LV_USER_EVENT_NVTMSG_DATA*)lv_event_get_data();

		UIFlowWifiWait_NVTMSG(obj, msg);
		break;
	}

	default:
		break;

	}

}

