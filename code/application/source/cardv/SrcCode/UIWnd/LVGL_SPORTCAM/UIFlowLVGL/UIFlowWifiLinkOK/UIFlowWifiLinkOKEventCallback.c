#include "PrjInc.h"
#include "UIFlowLVGL/UIFlowLVGL.h"
#include <kwrap/debug.h>
#include "UIApp/Network/WifiAppCmd.h"
#include "UIApp/Network/UIAppNetwork.h"
#include "UIApp/Network/UIAppWiFiCmdMovie.h"

static lv_group_t* gp_btns = NULL;
char StringTmpBuf[32] = {0};
static void task_delay_close(lv_task_t* task);
static void task_disconnect_last(lv_task_t* task);
static void set_indev_keypad_group(lv_obj_t* obj);

typedef struct {

	int* mode;
	lv_obj_t** scr;

} LV_USER_MODE_SCR_MAP;

static LV_USER_MODE_SCR_MAP lv_user_mode_scr_map_entry[] = {

		{&PRIMARY_MODE_MOVIE, &UIFlowMovie},
		{&PRIMARY_MODE_PHOTO, &UIFlowPhoto},
		{&PRIMARY_MODE_PLAYBACK, &UIFlowPlay},
};

static void UIFlowWiFiLinkOK_DHCP_REQ(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	char *pMAC = 0;
	//if (paramNum) {
		pMAC = (char *)msg->paramArray[0];
	//}
	//#NT#2016/12/27#YongChang Qui -begin
	//#NT#don't display mac address on UI if mac is 0:0:0:0:0:0:0
	if (pMAC && (pMAC[0] || pMAC[1] || pMAC[2] || pMAC[3] || pMAC[4] || pMAC[5]))
		//#NT#2016/12/27#YongChang Qui -end
	{
		snprintf(StringTmpBuf, sizeof(StringTmpBuf), "%02x%02x%02x%02x%02x%02x", pMAC[0], pMAC[1], pMAC[2]\
				 , pMAC[3], pMAC[4], pMAC[5]);
		//UxStatic_SetData(&UIMenuWndWiFiMobileLinkOK_StaticTXT_MACCtrl, STATIC_VALUE, Txt_Pointer(StringTmpBuf));
        //lvgl api
        lv_label_set_text_static(label_mac_scr_uiflowwifilinkok, StringTmpBuf);
	}
}

static void UIFlowWiFiLinkOK_DEAUTHENTICATED(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
UINT32 Live555_rtcp_support;

	// stop live555 service
	ImageApp_Common_GetParam(0,IACOMMON_PARAM_SUPPORT_RTCP,&Live555_rtcp_support);
	if (Live555_rtcp_support==0)
		ImageApp_Common_RtspStop(0);

//#NT#2016/03/23#Isiah Chang -begin
//#NT#add new Wi-Fi UI flow.
#if (WIFI_UI_FLOW_VER == WIFI_UI_VER_2_0)
	Ux_PostEvent(NVTEVT_SYSTEM_MODE, 2, System_GetState(SYS_STATE_CURRMODE), SYS_SUBMODE_NORMAL);
#else
	snprintf(StringTmpBuf, sizeof(StringTmpBuf), "No connect");
	//UxStatic_SetData(&UIMenuWndWiFiMobileLinkOK_StaticTXT_MACCtrl, STATIC_VALUE, Txt_Pointer(StringTmpBuf));
    //lvgl api
    lv_label_set_text_static(label_mac_scr_uiflowwifilinkok, StringTmpBuf);
#if !defined(_NVT_SDIO_WIFI_NONE_) || !defined(_NVT_USB_WIFI_NONE_)
	#if _TODO
	UINet_CliReConnect(1);
	#endif
#endif
#endif
//#NT#2016/03/23#Isiah Chang -end
	//Ux_CloseWindow(pCtrl,0);

	//return NVTEVT_CONSUME;
}

static void UIFlowWiFiLinkOK_Authorized_OK(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
  UINT32 bNewUser = 0 ;
  UINT32 Live555_rtcp_support;

 	// start live555 service
	ImageApp_Common_GetParam(0,IACOMMON_PARAM_SUPPORT_RTCP,&Live555_rtcp_support);
	if (Live555_rtcp_support==0)
		ImageApp_Common_RtspStart(0);

	//if (paramNum) {
		bNewUser = msg->paramArray[0];
	//}
#if 1  //print MAC when client get IP
	snprintf(StringTmpBuf, sizeof(StringTmpBuf), "%s", UINet_GetConnectedMAC());
    lv_label_set_text_static(label_mac_scr_uiflowwifilinkok, StringTmpBuf);
#endif
	if (bNewUser) {
		//delay 500 ms,disconnect last user and
		//in window,use GxTimer replace Timer Delay,it would occupat UI task
//		GxTimer_StartTimer(TIMER_HALF_SEC, NVTEVT_DISCONNECT_LAST_TIMER, ONE_SHOT);
		lv_task_once(lv_task_create(task_disconnect_last, 500, LV_TASK_PRIO_MID, NULL));
	}

#if 0 // Moved to UINet_DhcpSrvCBCliConn.
	//make sure, connec to RTSP mode
	if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_MOVIE) {
		Ux_PostEvent(NVTEVT_SYSTEM_MODE, 1, PRIMARY_MODE_MOVIE);
	}
#endif
}

static void UIFlowWiFiLinkOK_BAK_EVT(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	NVTEVT event;
	UINT32 status;

	event = msg->paramArray[ONDONE_PARAM_INDEX_CMD];
	status = msg->paramArray[ONDONE_PARAM_INDEX_RET];
	DBG_DUMP("%s event = 0x%x, status=%d\r\n", __FUNCTION__, event, status);

	switch (event) {
	case NVTEVT_BKW_FORMAT_CARD: {
			WifiCmd_Done(WIFIFLAG_FORMAT_DONE, WIFIAPP_RET_OK);
			//Rsvd
			break;
		}

	case NVTEVT_BKW_FORMAT_NAND: {
			WifiCmd_Done(WIFIFLAG_FORMAT_DONE, WIFIAPP_RET_OK);
			//Rsvd
			break;
		}

	case NVTEVT_BKW_FW_UPDATE: {
			if (status) {
				DBG_ERR("%s:update FW fail %d\r\n", __FUNCTION__, status);
				WifiCmd_Done(WIFIFLAG_UPDATE_DONE, status + WIFIAPP_RET_FW_OFFSET);
			} else {
				//#NT#2016/03/23#Isiah Chang -begin
				//#NT#add new Wi-Fi UI flow.
				INT32 ret;

				WifiCmd_Done(WIFIFLAG_UPDATE_DONE, WIFIAPP_RET_OK);

				ret = FileSys_DeleteFile(FW_UPDATE_NAME);
				if (ret != FST_STA_OK) {
					DBG_ERR("%s:delete FW fail %d\r\n", __FUNCTION__, status);
				}
				//#NT#2016/03/23#Isiah Chang -end
				Delay_DelayMs(2000);
				// Should power off immediatelly
				System_PowerOff(SYS_POWEROFF_NORMAL);
			}
			break;
		}
      
	case NVTEVT_BKW_WIFI_ON: {
		if (UI_GetData(FL_WIFI_LINK) == WIFI_LINK_OK) {
			//change to wifi mode,would close wait wnd
			//change to wifi mode would auto start record movie
#if (WIFI_FUNC==ENABLE)
		    if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_MOVIE){
				#if _TODO
			    Ux_PostEvent(NVTEVT_SYSTEM_MODE, 1, PRIMARY_MODE_MOVIE);
				#endif
			}
			//Ux_PostEvent(NVTEVT_OPEN_WINDOW, 1, &UIMenuWndWiFiModuleLinkCtrl);
        	Ux_PostEvent(NVTEVT_EXE_MOVIE_STRM_START,0);

#endif
		} else {
			//Ux_CloseWindow(pCtrl, 0);
            lv_plugin_scr_close(UIFlowWifiLinkOK, NULL);
#if (WIFI_AP_FUNC==ENABLE)
			//Ux_OpenWindow(&UIMenuWndWiFiModuleLinkCtrl, 0); //show link fail
            lv_plugin_scr_open(UIFlowWifiLink, NULL);
#endif
		}
		break;
	}	
	default:
		DBG_ERR("%s:Unknown event 0x%x\r\n", __FUNCTION__, event);
		break;
	}

}

static void task_delay_close(lv_task_t* task)
{
#if (WIFI_FUNC == ENABLE)

	INT32 curMode = System_GetState(SYS_STATE_CURRMODE);

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

}

static void task_disconnect_last(lv_task_t* task)
{
	UINet_AddACLTable();
}


static INT32 UIFlowWiFiLinkOK_OnTimer(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	UINT32  uiEvent;
	uiEvent = msg->paramArray[0];

	switch (uiEvent) {
//	case NVTEVT_DISCONNECT_LAST_TIMER:
//		UINet_AddACLTable();
//		return NVTEVT_CONSUME;
//	case NVTEVT_DELAY_CLOSE_TIMER: {
//			//#NT#2016/03/08#Niven Cho -begin
//			//#NT#because it is long time closing the linux-wifi, we don't change mode to movie video ahead.
//			//#NT#use Ux_SendEvent(0,NVTEVT_EXE_WIFI_STOP, 0),not post event
//#if (WIFI_FUNC == ENABLE)
//				INT32 curMode = System_GetState(SYS_STATE_CURRMODE);
//				if ( curMode!= PRIMARY_MODE_MOVIE) {
//			    	Ux_SendEvent(0,NVTEVT_EXE_WIFI_STOP, 0);
//					//Ux_CloseWindow(&UIFlowWndMovieCtrl,0);
//                    //lvgl api
////                    lv_plugin_scr_close(UIFlowMovie, NULL);
//					lv_obj_t* scr = lv_plugin_scr_by_index(0);
//                    lv_plugin_scr_close(scr, NULL);
//			    	Ux_PostEvent(NVTEVT_SYSTEM_MODE, 2, curMode,SYS_SUBMODE_NORMAL);
//				} else {
//					//VControl *pWnd =0 ;
//					//Ux_GetWindowByIndex(&pWnd, 1);
//					Ux_SendEvent(0,NVTEVT_EXE_WIFI_STOP, 0);
//					System_ChangeSubMode(SYS_SUBMODE_NORMAL);
//					//Ux_CloseWindow(pWnd, 0);
//
//					lv_obj_t* scr = lv_plugin_scr_by_index(1);
//                    lv_plugin_scr_close(scr, NULL);
//
//				}
//#endif
//			//#NT#2016/03/08#Niven Cho -end
//			return NVTEVT_CONSUME;
//		}

//#NT#2017/01/03#Isiah Chang -begin
//#NT#Add WiFiCmd Bitrate control.
#if(WIFI_UI_FLOW_VER == WIFI_UI_VER_2_0)
	case NVTEVT_1SEC_TIMER: {
			WiFiCmd_HBOneSec();
			return NVTEVT_CONSUME;
		}
#endif
//#NT#2017/01/03#Isiah Chang -end
	case NVTEVT_05SEC_TIMER: {
			WiFiCmd_OnMotionDetect();
			return NVTEVT_CONSUME;
		}
	default:
		return NVTEVT_CONSUME;
	}
}

static void UIFlowWiFiLinkOK_OnUpdateInfo(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
#if(PHOTO_MODE==ENABLE)
	extern void UIFlowPhoto_UpdateInfo(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg);
	return UIFlowPhoto_UpdateInfo(obj, msg);
#else
	return;
#endif
}

static INT32 UIFlowWifiLinkOK_OnMovieOneSec(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	return WiFiCmd_OnMovieOneSec(NULL, msg->paramNum, msg->paramArray);
}

static INT32 UIFlowWifiLinkOK_OnExeMovieRawEncJpgOKCB(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	return WiFiCmd_OnMovieRawEncJpgOKCB(NULL, msg->paramNum, msg->paramArray);
}

static INT32 UIFlowWifiLinkOK_OnExeMovieRawEncErr(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	return WiFiCmd_OnMovieRawEncErr(NULL, msg->paramNum, msg->paramArray);
}

static INT32 UIFlowWifiLinkOK_OnBatteryLow(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	DBG_DUMP("%s \r\n", __FUNCTION__);

	//snprintf(StringTmpBuf,sizeof(StringTmpBuf),"Battery Low");
	//UxStatic_SetData(&UIFlowWifiLinkOK_StaticTXT_MACCtrl,STATIC_VALUE,Txt_Pointer(StringTmpBuf));

	WifiApp_SendCmd(WIFIAPP_CMD_NOTIFY_STATUS, WIFIAPP_RET_BATTERY_LOW);
	return NVTEVT_CONSUME;
}

static INT32 UIFlowWiFiLinkOK_OnStorageInit(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
#if (SDHOTPLUG_FUNCTION == ENABLE)
	if (UIStorageCheck(STORAGE_CHECK_ERROR, NULL) == TRUE) {
		DBG_DUMP("card err,removed\r\n");
	} else {
		DBG_DUMP("card insert\r\n");
		if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PLAYBACK) {
			Ux_SendEvent(0, NVTEVT_SYSTEM_MODE, 1, System_GetState(SYS_STATE_CURRMODE));
		} else {
			ImageUnit_SetParam(&ISF_CamFile, CAMFILE_PARAM_FILEDB_REOPEN, 0);
		}
		WifiApp_SendCmd(WIFIAPP_CMD_NOTIFY_STATUS, WIFIAPP_RET_CARD_INSERT);
	}
#endif
	return NVTEVT_CONSUME;
}

static INT32 UIFlowWiFiLinkOK_OnStorageChange(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
#if (SDHOTPLUG_FUNCTION == ENABLE)
	DBG_DUMP("card removed\r\n");
	if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_PLAYBACK) {
		ImageUnit_SetParam(&ISF_CamFile, CAMFILE_PARAM_FILEDB_CLOSE, 0);
	}
	WifiApp_SendCmd(WIFIAPP_CMD_NOTIFY_STATUS, WIFIAPP_RET_CARD_REMOVE);
#endif
	return NVTEVT_CONSUME;
}

static INT32 UIFlowWifiLinkOK_OnExeMovieVedioReady(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	return WiFiCmd_OnExeMovieVedioReady(NULL, msg->paramNum, msg->paramArray);
}

static INT32 UIFlowWifiLinkOK_OnMovieFull(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	return WiFiCmd_OnMovieFull(NULL, msg->paramNum, msg->paramArray);
}
static INT32 UIFlowWifiLinkOK_OnMovieWrError(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	return WiFiCmd_OnMovieWrError(NULL, msg->paramNum, msg->paramArray);
}
static INT32 UIFlowWifiLinkOK_OnStorageSlow(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	return WiFiCmd_OnStorageSlow(NULL, msg->paramNum, msg->paramArray);
}


static void UIFlowWifiLinkOK_NVTMSG(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{

	switch(msg->event)
	{

	case NVTEVT_BACKGROUND_DONE:
	{
		
        UIFlowWiFiLinkOK_BAK_EVT(obj, msg);
        break;
	}

    case NVTEVT_WIFI_DHCP_REQ:
    {
        UIFlowWiFiLinkOK_DHCP_REQ(obj, msg);
        break;
    }

    case NVTEVT_WIFI_AUTHORIZED_OK:
    {
        UIFlowWiFiLinkOK_Authorized_OK(obj, msg);
        break;
    }

    case NVTEVT_WIFI_DEAUTHENTICATED:
    {
        UIFlowWiFiLinkOK_DEAUTHENTICATED(obj, msg);
        break;
    }

    case NVTEVT_STORAGE_INIT:
    {
        UIFlowWiFiLinkOK_OnStorageInit(obj, msg);
        break;
    }

    case NVTEVT_STORAGE_CHANGE:
    {
        UIFlowWiFiLinkOK_OnStorageChange(obj, msg);
        break;
    }

    case NVTEVT_BATTERY_LOW:
    {
        UIFlowWifiLinkOK_OnBatteryLow(obj, msg);
        break;
    }  

    case NVTEVT_CB_MOVIE_VEDIO_READY:
    {
        UIFlowWifiLinkOK_OnExeMovieVedioReady(obj, msg);
        break;
    }  

    case NVTEVT_CB_RAWENC_DCF_FULL:
    case NVTEVT_CB_RAWENC_WRITE_ERR:
    case NVTEVT_CB_RAWENC_ERR:
    {
        UIFlowWifiLinkOK_OnExeMovieRawEncErr(obj, msg);
        break;
    }

    case NVTEVT_CB_MOVIE_REC_ONE_SEC:
    {
        UIFlowWifiLinkOK_OnMovieOneSec(obj, msg);
        break;
    }  

    case NVTEVT_CB_RAWENC_OK:
    {
        UIFlowWifiLinkOK_OnExeMovieRawEncJpgOKCB(obj, msg);
        break;
    }  

    case NVTEVT_CB_MOVIE_OVERTIME:
    case NVTEVT_CB_MOVIE_FULL:
    case NVTEVT_CB_MOVIE_LOOPREC_FULL:
    {
        UIFlowWifiLinkOK_OnMovieFull(obj, msg);
        break;
    }

    case NVTEVT_CB_MOVIE_WR_ERROR:
    {
        UIFlowWifiLinkOK_OnMovieWrError(obj, msg);
        break;
    }  

    case NVTEVT_CB_MOVIE_SLOW:
    {
        UIFlowWifiLinkOK_OnStorageSlow(obj, msg);
        break;
    }  


    case NVTEVT_TIMER:
    {
        ///timer
        //CHKPNT;
        UIFlowWiFiLinkOK_OnTimer(obj, msg);
        break;
    }

    case NVTEVT_UPDATE_INFO:
    {
    	UIFlowWiFiLinkOK_OnUpdateInfo(obj, msg);
    	break;
    }

	default:
		break;

	}
}


static void UIFlowWiFiLinkOK_OnOpen(lv_obj_t* obj)
{
	static char Buf1[32] = {0}, buf2[32] = {0} ;

	DBG_DUMP("%s\r\n", __func__);

	set_indev_keypad_group(obj);

	//Ux_FlushEventByRange(NVTEVT_KEY_EVT_START, NVTEVT_KEY_EVT_END);
	//Input_SetKeyMask(KEY_PRESS, FLGKEY_KEY_MASK_DEFAULT);
	//Input_SetKeyMask(KEY_RELEASE, 0);
	//Input_SetKeyMask(KEY_CONTINUE, 0);
//#NT#2016/05/31#Ben Wang -begin
//#NT#Add UVC multimedia function.
#if(UVC_MULTIMEDIA_FUNC == ENABLE)
	if (System_GetState(SYS_STATE_CURRSUBMODE) == SYS_SUBMODE_UVC) {
		if (ImageUnit_GetParam(&ISF_UsbUVAC, ISF_CTRL, UVAC_PARAM_CDC_ENABLE)) {
			MEM_RANGE WorkBuf;
			WorkBuf.Addr = OS_GetMempoolAddr(POOL_ID_USBCMD);
			WorkBuf.Size = POOL_SIZE_USBCMD;
			UsbCmd_Open(&WorkBuf);
		}
		snprintf(Buf1, sizeof(Buf1), "UVC Connected");
		//just don't show this string
		StringTmpBuf[0] = 0;
	} else
#endif
	{
#if !defined(_NVT_SDIO_WIFI_NONE_) || !defined(_NVT_USB_WIFI_NONE_)
		snprintf(Buf1, sizeof(Buf1), "WiFi Connected");
		snprintf(StringTmpBuf, sizeof(StringTmpBuf), "%s", UINet_GetConnectedMAC());
#else
		snprintf(Buf1, sizeof(Buf1), "Ethernet Connected");
		snprintf(StringTmpBuf, sizeof(StringTmpBuf), "%s", UINet_GetIP());
#endif
	}
//#NT#2016/05/31#Ben Wang -end
    lv_label_set_text_static(label_wifista_scr_uiflowwifilinkok, Buf1);
    lv_label_set_text_static(label_mac_scr_uiflowwifilinkok, StringTmpBuf);

	snprintf(buf2, sizeof(buf2), "Press Select to Disconnect");
    lv_label_set_text_static(label_select_msg_scr_uiflowwifilinkok, buf2);
//#NT#2016/05/31#Ben Wang -begin
//#NT#Add UVC multimedia function.
#if(UVC_MULTIMEDIA_FUNC == ENABLE)
	if (System_GetState(SYS_STATE_CURRSUBMODE) != SYS_SUBMODE_UVC)
#endif
	{
		//make sure, connec to RTSP mode
		if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_MOVIE) {
			Ux_PostEvent(NVTEVT_SYSTEM_MODE, 1, PRIMARY_MODE_MOVIE);
		}
	}
//#NT#2016/05/31#Ben Wang -end
//#NT#2016/03/23#Isiah Chang -begin
//#NT#add new Wi-Fi UI flow.
#if(WIFI_UI_FLOW_VER == WIFI_UI_VER_2_0)
	WiFiCmd_HBStart();
#endif
//#NT#2016/03/23#Isiah Chang -end
	WiFiCmd_MotionDetectStart();
}

static void UIFlowWiFiLinkOK_OnKeyEnter(lv_obj_t* obj, uint32_t key)
{

    //return 0;
//#NT#2016/05/31#Ben Wang -begin
//#NT#Add UVC multimedia function.
#if(UVC_MULTIMEDIA_FUNC == ENABLE)
	if (System_GetState(SYS_STATE_CURRSUBMODE) == SYS_SUBMODE_UVC) {
		Ux_PostEvent(NVTEVT_SYSTEM_MODE, 2, System_GetState(SYS_STATE_CURRMODE), SYS_SUBMODE_NORMAL);
		return NVTEVT_CONSUME;
	}
#endif
//#NT#2016/05/31#Ben Wang -end
#if (WIFI_FUNC == ENABLE)
#if !defined(_NVT_SDIO_WIFI_NONE_) || !defined(_NVT_USB_WIFI_NONE_)
	//notify current connected socket,new connect not get IP and socket not create
	//cannot disconnet immediate,suggest after 500 ms

	WifiApp_SendCmd(WIFIAPP_CMD_NOTIFY_STATUS, WIFIAPP_RET_DISCONNECT);
	//delay 500 ms,and then close window
	//in window,use GxTimer replace Timer Delay,it would occupat UI task
//	GxTimer_StartTimer(TIMER_HALF_SEC, NVTEVT_DELAY_CLOSE_TIMER, ONE_SHOT);
	lv_task_once(lv_task_create(task_delay_close, 500, LV_TASK_PRIO_MID, NULL));

#else

	Ux_PostEvent(NVTEVT_SYSTEM_MODE, 2, System_GetState(SYS_STATE_CURRMODE), SYS_SUBMODE_NORMAL);
	//should close network application,then stop wifi
	Ux_PostEvent(NVTEVT_EXE_WIFI_STOP, 0);

#endif
#endif

}

static void UIFlowWifiLinkOKBtnEventCallback(lv_obj_t* obj, lv_event_t event)
{
    DBG_IND("%s:%d\r\n", __func__, event);

    switch(event)
    {

	case LV_EVENT_CLICKED:
	{
		uint32_t* key = (uint32_t*)lv_event_get_data();

		UIFlowWiFiLinkOK_OnKeyEnter(obj, *key);
		break;
	}

    case LV_EVENT_KEY:
    {
        uint32_t* key = (uint32_t*)lv_event_get_data();

        /* handle key event */
        //UIFlowWifiLinkOK_Key(obj, *key);

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

    }
}

void btn_sel_event_callback(lv_obj_t* obj, lv_event_t event)
{
	UIFlowWifiLinkOKBtnEventCallback(obj, event);
}

static void set_indev_keypad_group(lv_obj_t* obj)
{
    if(gp_btns == NULL)
    {
        gp_btns = lv_group_create();

        lv_group_add_obj(gp_btns, button_select_msg_scr_uiflowwifilinkok);

        lv_group_set_wrap(gp_btns, true);
        lv_group_focus_obj(button_select_msg_scr_uiflowwifilinkok);
    }
        
    lv_indev_t* indev = lv_plugin_find_indev_by_type(LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_group(indev, gp_btns);
}

static void UIFlowWiFiLinkOK_OnClose(lv_obj_t* obj)
{
	DBG_DUMP("%s\r\n", __func__);

	if (UI_GetData(FL_MOVIE_SIZE_MENU) != UI_GetData(FL_MOVIE_SIZE)) {
		Ux_SendEvent(&CustomMovieObjCtrl, NVTEVT_EXE_MOVIESIZE, 1, UI_GetData(FL_MOVIE_SIZE_MENU));
	}
//#NT#2016/03/23#Isiah Chang -begin
//#NT#add new Wi-Fi UI flow.
#if (WIFI_UI_FLOW_VER == WIFI_UI_VER_2_0)
	WiFiCmd_HBStop();
#endif
//#NT#2016/03/23#Isiah Chang -end
	WiFiCmd_MotionDetectStop();

//#NT#2016/05/31#Ben Wang -begin
//#NT#Add UVC multimedia function.
#if(UVC_MULTIMEDIA_FUNC == ENABLE)
	//if(System_GetState(SYS_STATE_CURRSUBMODE)==SYS_SUBMODE_UVC && System_GetState(SYS_STATE_NEXTSUBMODE)!=SYS_SUBMODE_UVC)
	if (System_GetState(SYS_STATE_CURRSUBMODE) == SYS_SUBMODE_UVC) {
		//if (ImageUnit_GetParam(&ISF_UsbUVAC, ISF_CTRL, UVAC_PARAM_CDC_ENABLE)) {
			UsbCmd_Close();
		//}
	}
#endif
//#NT#2016/05/31#Ben Wang -end
}

static void UIFlowWiFiLinkOK_OnChildScrClose(lv_obj_t* obj)
{
	DBG_DUMP("%s\r\n", __func__);
}

void UIFlowWifiLinkOKEventCallback(lv_obj_t* obj, lv_event_t event)
{

	switch(event)
	{

	case LV_PLUGIN_EVENT_SCR_OPEN:
        UIFlowWiFiLinkOK_OnOpen(obj);
		break;

	case LV_PLUGIN_EVENT_SCR_CLOSE:
        UIFlowWiFiLinkOK_OnClose(obj);
		break;

	case LV_PLUGIN_EVENT_CHILD_SCR_CLOSE:
		UIFlowWiFiLinkOK_OnChildScrClose(obj);
		break;

	/* handle nvt event except key event */
	case LV_USER_EVENT_NVTMSG:
	{
		const LV_USER_EVENT_NVTMSG_DATA* msg = (const LV_USER_EVENT_NVTMSG_DATA*)lv_event_get_data();

		UIFlowWifiLinkOK_NVTMSG(obj, msg);
		break;
	}    

	default:
		break;

	}

}

