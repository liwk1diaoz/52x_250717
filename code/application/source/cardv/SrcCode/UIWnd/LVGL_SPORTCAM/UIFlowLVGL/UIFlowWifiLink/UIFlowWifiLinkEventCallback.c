#include "PrjInc.h"
#include "UIApp/Network/UIAppNetwork.h"
#include "UIFlowLVGL/UIFlowLVGL.h"
#include <kwrap/debug.h>
#include "UIApp/Network/WifiAppCmd.h"
#include "UIApp/Network/UIAppNetwork.h"
#include "UIApp/Network/UIAppWiFiCmdMovie.h"

static lv_group_t* gp_btns = NULL;

typedef enum {
	BTN_REFRESH = 0,
	BTN_WIFI_OFF,
} WIFI_LINK_BTN_FLAG;

typedef struct {

	int* mode;
	lv_obj_t** scr;

} LV_USER_MODE_SCR_MAP;

static LV_USER_MODE_SCR_MAP lv_user_mode_scr_map_entry[] = {

		{&PRIMARY_MODE_MOVIE, &UIFlowMovie},
		{&PRIMARY_MODE_PHOTO, &UIFlowPhoto},
		{&PRIMARY_MODE_PLAYBACK, &UIFlowPlay},
};

static void set_indev_keypad_group(lv_obj_t* obj)
{
    if(gp_btns == NULL)
    {
        gp_btns = lv_group_create();

        lv_group_add_obj(gp_btns, button_ref_scr_uiflowwifilink);
        lv_group_add_obj(gp_btns, button_wifioff_scr_uiflowwifilink);

        lv_group_set_wrap(gp_btns, true);
        lv_group_focus_obj(button_ref_scr_uiflowwifilink);
    }
        
    lv_indev_t* indev = lv_plugin_find_indev_by_type(LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_group(indev, gp_btns);
}

static void UIFlowWiFiLink_UpdateData(void)
{
	static char buf1[32], buf2[32];
#if (MAC_APPEN_SSID==ENABLE)
	UINT8 *pMacAddr;
#endif
	UIMenuStoreInfo *ptMenuStoreInfo = UI_GetMenuInfo();

	if (UI_GetData(FL_WIFI_LINK) == WIFI_LINK_OK) {
		if (UI_GetData(FL_NetWorkMode) == NET_AP_MODE || UI_GetData(FL_NetWorkMode) == NET_WPS_AP_PBC_MODE || UI_GetData(FL_NetWorkMode) == NET_STATION_MODE) {
			if (ptMenuStoreInfo->strSSID[0] == 0) {
#if (MAC_APPEN_SSID==ENABLE)
				pMacAddr = (UINT8 *)UINet_GetMAC();
				snprintf(buf1, sizeof(buf1), "%s%02x%02x%02x%02x%02x%02x", UINet_GetSSID(), (pMacAddr[0] & 0xFF), (pMacAddr[1] & 0xFF), (pMacAddr[2] & 0xFF)
						 , (pMacAddr[3] & 0xFF), (pMacAddr[4] & 0xFF), (pMacAddr[5] & 0xFF));
#else
				snprintf(buf1, sizeof(buf1), "%s", UINet_GetSSID());
#endif
			} else {
				snprintf(buf1, sizeof(buf1), "%s", UINet_GetSSID());
			}


			snprintf(buf2, sizeof(buf2), "%s", UINet_GetPASSPHRASE());
            lv_label_set_text_static(label_ssid_scr_uiflowwifilink,buf1);
            lv_label_set_text_static(label_pwa2_scr_uiflowwifilink,buf2);

		}
	} else {
		snprintf(buf1, sizeof(buf1), "%s:Fail", "Conntecting");
        lv_label_set_text_static(label_ssid_scr_uiflowwifilink,buf1);
        lv_label_set_text_static(label_pwa2_scr_uiflowwifilink,NULL);
	}
}

static uint32_t btn_sta = BTN_REFRESH;
static INT32 UIFlowWiFiLink_OnOpen(lv_obj_t* obj, lv_event_t event)
{
    //UINT16 i;
    //after enter link wnd ,set wifi sub mode,avoid wifi on change normal movie mode
	System_ChangeSubMode(SYS_SUBMODE_WIFI);
	UIFlowWiFiLink_UpdateData();
    //button checked
    //

#if defined(_NVT_ETHERNET_EQOS_) || defined(_CPU2_LINUX_)
	//#NT#2016/03/17#YongChang Qui -begin
	//#NT#This flow was for testing purpose and should be removed once cardv linux is added
#if (NETWORK_4G_MODULE == ENABLE)
	Ux_PostEvent(NVTEVT_WIFI_AUTHORIZED_OK, 1, 0);
#endif
	//#NT#2016/03/17#YongChang Qui -end
#endif
	return NVTEVT_CONSUME;
}


static void UIFlowWiFiLink_OnKeyEnter(void)
{
    lv_obj_t* focused_obj = lv_group_get_focused(gp_btns);

    if (focused_obj == button_ref_scr_uiflowwifilink) {
        btn_sta = BTN_REFRESH;
    }
    else if (focused_obj == button_wifioff_scr_uiflowwifilink) {
        btn_sta = BTN_WIFI_OFF;
    }
    else {
    }
    DBG_IND("menu item index %x\r\n",btn_sta);
    //return 0;
    if (btn_sta == BTN_WIFI_OFF) {
        if (UI_GetData(FL_WIFI_LINK) == WIFI_LINK_OK) {
        //#NT#2016/03/08#Niven Cho -begin
        //#NT#because it is long time closing the linux-wifi, we don't change mode to movie video ahead.
        //#NT#use Ux_SendEvent(0,NVTEVT_EXE_WIFI_STOP, 0),not post event
#if (WIFI_FUNC == ENABLE)
            INT32 curMode = System_GetState(SYS_STATE_CURRMODE);
//            if ( curMode!= PRIMARY_MODE_MOVIE) {
//                Ux_SendEvent(0,NVTEVT_EXE_WIFI_STOP, 0);
//                //Ux_CloseWindow(&UIFlowWndMovieCtrl,0);
//                lv_plugin_scr_close(UIFlowMovie, NULL);
//                Ux_PostEvent(NVTEVT_SYSTEM_MODE, 2, curMode,SYS_SUBMODE_NORMAL);
//            } else {
//                //VControl *pWnd =0 ;
//                //Ux_GetWindowByIndex(&pWnd, 1);
//
//            	lv_obj_t* scr = lv_plugin_scr_by_index(1);
//
//                Ux_SendEvent(0,NVTEVT_EXE_WIFI_STOP, 0);
//                System_ChangeSubMode(SYS_SUBMODE_NORMAL);
//                //Ux_CloseWindow(pWnd, 0);
//                lv_plugin_scr_close(scr, NULL);
//            }

            Ux_SendEvent(0,NVTEVT_EXE_WIFI_STOP, 0);

        	lv_obj_t* target_scr = NULL;
        	lv_obj_t* scr = lv_plugin_scr_by_index(1);

        	lv_plugin_scr_close(scr, NULL);

        	scr = lv_plugin_scr_act();

        	/* find target screen */
        	for(unsigned int i=0 ; i<(sizeof(lv_user_mode_scr_map_entry) / sizeof(LV_USER_MODE_SCR_MAP)) ; i++)
        	{
        		if(*lv_user_mode_scr_map_entry[i].mode == curMode){
        			target_scr = *lv_user_mode_scr_map_entry[i].scr;
        			break;
        		}
        	}

        	if(target_scr != scr){
        		lv_plugin_scr_close(scr, NULL);
        		lv_plugin_scr_open(target_scr, NULL);
        	}

        	System_ChangeSubMode(SYS_SUBMODE_NORMAL);

#endif
        //#NT#2016/03/08#Niven Cho -end
            //#NT#2016/03/08#Niven Cho -end
        } else {
            //Ux_CloseWindow(&UIMenuWndWiFiModuleLinkCtrl, 0);
            lv_plugin_scr_close(UIFlowWifiLink, NULL);
        }
    }
}

static void UIFlowWiFiLink_Authorized_OK(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
    UINT32 Live555_rtcp_support;

 	// start live555 service
	ImageApp_Common_GetParam(0,IACOMMON_PARAM_SUPPORT_RTCP,&Live555_rtcp_support);
	if (Live555_rtcp_support==0)
		ImageApp_Common_RtspStart(0);

    lv_plugin_scr_open(UIFlowWifiLinkOK, NULL);
}



static void UIFlowWifiLink_Key(lv_obj_t* obj, uint32_t key)
{
	switch(key)
	{
        case LV_USER_KEY_NEXT:
        {
            lv_group_focus_next(gp_btns);
            break;
        }

        case LV_USER_KEY_PREV:
        {
            lv_group_focus_prev(gp_btns);
            break;
        }

        case LV_USER_KEY_SHUTTER2:
        {
            UIFlowWiFiLink_OnKeyEnter();
            break;
        }
	}
}

static INT32 UIFlowWiFiLink_OnWiFiState(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	
    if (UI_GetData(FL_NetWorkMode) == NET_AP_MODE || UI_GetData(FL_NetWorkMode) == NET_WPS_AP_PBC_MODE) {
        DBG_DUMP("ap  state %d result %d\r\n", msg->paramArray[0], msg->paramArray[1]);
        UINT32 state = msg->paramArray[ONDONE_PARAM_INDEX_CMD];
        INT32 result = msg->paramArray[ONDONE_PARAM_INDEX_RET];
        // wifi phase status
        if (state == NET_STATE_WIFI_CB) {
            if (result == NVT_WIFI_AP_READY) {
                DBG_DUMP("ap mode OK\r\n");
            } else { //if(result == NVT_WIFI_STA_STATUS_PORT_AUTHORIZED)
                DBG_DUMP("ap state %d result %d\r\n", msg->paramArray[0], msg->paramArray[1]);
            }

        }
    } else { // station mode
        DBG_DUMP("sta  state %d result %d\r\n", msg->paramArray[0], msg->paramArray[1]);
        UINT32 state = msg->paramArray[0];
        INT32 result = msg->paramArray[1];

        if (state == NET_STATE_DHCP_START) {
            if (result == 0) {
                DBG_DUMP("station mode OK\r\n");
            } else {
                DBG_DUMP("DHCP fail or wifi module failed\r\n");
            }
        } else if (state == NET_STATE_WIFI_CB) {
            if (result == NVT_WIFI_LINK_STATUS_CONNECTED) {

            } else {

            }
        } else if (state == NET_STATE_WIFI_CONNECTING) {
            if (result == NET_CONNECTING_TIMEOUT) {
                DBG_DUMP("wifi module failed\r\n");
            }
        }
    }

	return NVTEVT_CONSUME;
}

static INT32 UIFlowWifiLink_OnMovieFull(void)
{
	return WiFiCmd_OnMovieFull(NULL, 0, NULL);
}
static INT32 UIFlowWifiLink_OnMovieWrError(void)
{
	return WiFiCmd_OnMovieWrError(NULL, 0, NULL);
}
static INT32 UIFlowWifiLink_OnStorageSlow(void)
{
	return WiFiCmd_OnStorageSlow(NULL, 0, NULL);
}

static void UIFlowWifiLink_NVTMSG(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{

	switch(msg->event)
	{

    case NVTEVT_WIFI_STATE:
    {
        UIFlowWiFiLink_OnWiFiState(obj, msg);
        break;
    }

    case NVTEVT_WIFI_AUTHORIZED_OK:
    {
        UIFlowWiFiLink_Authorized_OK(obj, msg);
        break;
    }

    case NVTEVT_CB_MOVIE_OVERTIME:
    case NVTEVT_CB_MOVIE_FULL:
    case NVTEVT_CB_MOVIE_LOOPREC_FULL:
    {
        UIFlowWifiLink_OnMovieFull();
        break;
    }

    case NVTEVT_CB_MOVIE_WR_ERROR:
    {
        UIFlowWifiLink_OnMovieWrError();
        break;
    }  

    case NVTEVT_CB_MOVIE_SLOW:
    {
        UIFlowWifiLink_OnStorageSlow();
        break;
    }  

	default:
		break;

	}
}


static void UIFlowWifiLink_ChildScrClose(lv_obj_t* obj, LV_USER_EVENT_NVTMSG_DATA* data)
{
	DBG_DUMP("%s\r\n", __func__);

	/* once child scr closed, set keypad group again */
	set_indev_keypad_group(obj);

	UIFlowWiFiLink_UpdateData();
	//Ux_DefaultEvent(pCtrl, NVTEVT_CHILD_CLOSE, paramNum, paramArray);
	//return NVTEVT_CONSUME;
}


static void UIFlowWifiLinkBtnEventCallback(lv_obj_t* obj, lv_event_t event)
{
    DBG_IND("%s:%d\r\n", __func__, event);

    switch(event)
    {

	case LV_EVENT_CLICKED:
	{
		UIFlowWiFiLink_OnKeyEnter();
		break;
	}


	case LV_EVENT_KEY:
	{
		uint32_t* key = (uint32_t*)lv_event_get_data();

		if(*key != LV_KEY_ENTER) {

       		 UIFlowWifiLink_Key(obj, *key);
			/***********************************************************************************
			 * IMPORTANT!!
			 *
			 * calling lv_indev_wait_release to avoid duplicate event in long pressed key state
			 * the event will not be sent again until released
			 *
			 ***********************************************************************************/
 		 	lv_indev_wait_release(lv_indev_get_act());
		}

		break;
	}

    case LV_EVENT_FOCUSED:
    {
    	lv_obj_t* child_label = lv_obj_get_child(obj, NULL);

    	if(child_label){

    		lv_obj_set_state(child_label, LV_STATE_FOCUSED);

    	}

    	break;
    }

    case LV_EVENT_DEFOCUSED:
    {
    	lv_obj_t* child_label = lv_obj_get_child(obj, NULL);

    	if(child_label){

    		lv_obj_set_state(child_label, LV_STATE_DEFAULT);

    	}
    	break;
    }

    }
}

void btn_wifioff_event_callback(lv_obj_t* obj, lv_event_t event)
{
	UIFlowWifiLinkBtnEventCallback(obj, event);
}

void btn_ref_event_callback(lv_obj_t* obj, lv_event_t event)
{
	UIFlowWifiLinkBtnEventCallback(obj, event);
}

void UIFlowWifiLinkEventCallback(lv_obj_t* obj, lv_event_t event)
{

	switch(event)
	{

	case LV_PLUGIN_EVENT_SCR_OPEN:
        DBG_DUMP("UIFlowWifiWLink_ScrOpen\r\n");

        set_indev_keypad_group(obj);
        UIFlowWiFiLink_OnOpen(obj, event);
		break;

	case LV_PLUGIN_EVENT_SCR_CLOSE:
        DBG_DUMP("UIFlowWifiLink_ScrClose\r\n");
		break;

	case LV_PLUGIN_EVENT_CHILD_SCR_CLOSE:
		UIFlowWifiLink_ChildScrClose(obj, (LV_USER_EVENT_NVTMSG_DATA*)lv_event_get_data());
		break;


	/* handle nvt event except key event */
	case LV_USER_EVENT_NVTMSG:
	{
		const LV_USER_EVENT_NVTMSG_DATA* msg = (const LV_USER_EVENT_NVTMSG_DATA*)lv_event_get_data();

		UIFlowWifiLink_NVTMSG(obj, msg);
		break;
	}

	default:
		break;

	}

}

