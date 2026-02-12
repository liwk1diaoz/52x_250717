
#include "PrjInc.h"
#include "UIFlowLVGL/UIFlowLVGL.h"
#include "UIFlowLVGL/UIFlowWrnMsg/UIFlowWrnMsgAPI.h"
#include "UIFlowPhotoParams.h"
#include "UIFlowPhotoFuncs.h"
#include "ImageApp/ImageApp_Photo.h"
#include "comm/timer.h"
#if (CALIBRATION_FUNC == ENABLE)
#include "EngineerMode.h"
#endif
#include "UIApp/Photo/UIAppPhoto.h"
#include "exif/Exif.h"
#include <kwrap/util.h>

#if (USE_DCF == ENABLE)
#include "DCF.h"
#endif

#define IDE_FD_MAX_NUM               8

#define FD_FRAME_RATE               3//10   //(10frame/30fps) = 333ms = update time

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UIFlowPhoto
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

//static UINT32 g_uiSelfTimerID       = NULL_TIMER;
//static UINT32 g_uiQviewTimerID      = NULL_TIMER;
static volatile BOOL g_bRedLEDOn    = FALSE;
//static void UIFlowPhoto_SetToQuickView(AlgMsgInfo *pAlgInfo);
static UINT32  g_uiFreePicNum = 0;
static CHAR    g_cSelftimerCntStr[8]   = {0};
static lv_group_t* gp = NULL;
static lv_task_t* task_selftimer = NULL;
static lv_task_t* task_qview = NULL;

static void task_selftimer_cb(lv_task_t* task);
static void task_qview_cb(lv_task_t* task);


typedef struct _UIFlowInfoTypePhoto {
	BOOL                       bIsPreview;
} UIFlowInfoTypePhoto;


UIFlowInfoTypePhoto UIFlowInfoPhotoInitVal = {
	TRUE,                           //bIsPreview;
};

static UIFlowInfoTypePhoto  UIFlowInfoPhoto;
static UIFlowInfoTypePhoto *localInfo = &UIFlowInfoPhoto;

static void set_indev_keypad_group(lv_obj_t* obj)
{
	if(gp == NULL){
		gp = lv_group_create();
		lv_group_add_obj(gp, obj);
	}

	lv_indev_t* indev = lv_plugin_find_indev_by_type(LV_INDEV_TYPE_KEYPAD);
	lv_indev_set_group(indev, gp);
}

static CHAR *Get_SelftimerCntString(UINT32 uiCount)
{
	snprintf(g_cSelftimerCntStr, sizeof(g_cSelftimerCntStr), "%ld", uiCount);
	return g_cSelftimerCntStr;
}

static void update_selftimer(void)
{
	static lv_plugin_res_id res[] = {
			LV_PLUGIN_RES_ID_NONE,
			LV_PLUGIN_IMG_ID_ICON_SELFTIMER_2SEC,
			LV_PLUGIN_IMG_ID_ICON_SELFTIMER_5SEC,
			LV_PLUGIN_IMG_ID_ICON_SELFTIMER_10SEC
	};


	lv_plugin_res_id img_id = res[SysGetFlag(FL_SELFTIMER)];
	bool is_task_selftimer_active;


	/**************************************************************************
	 * To disable task_selftimer, task coulde be deleted or set priority off.
	 * Both condition should be checked when updating the icon.
	 **************************************************************************/
	if(task_selftimer && task_selftimer->prio != LV_TASK_PRIO_OFF){
		is_task_selftimer_active = true;
	}
	else{
		is_task_selftimer_active = false;
	}


	if(is_task_selftimer_active){
		lv_obj_set_hidden(label_self_timer_cnt_scr_uiflowphoto, false);
		lv_obj_set_hidden(image_self_timer_scr_uiflowphoto, true);
	}
	else{
		lv_obj_set_hidden(label_self_timer_cnt_scr_uiflowphoto, true);

		if(img_id != LV_PLUGIN_RES_ID_NONE){
			lv_obj_set_hidden(image_self_timer_scr_uiflowphoto, false);
			lv_plugin_img_set_src(image_self_timer_scr_uiflowphoto, img_id);
		}
		else{
			lv_obj_set_hidden(image_self_timer_scr_uiflowphoto, true);
		}
	}
}

static void update_size(void)
{
	lv_label_set_text(label_size_scr_uiflowphoto, Get_SizeString(SysGetFlag(FL_PHOTO_SIZE)));
}

static void update_quality(void)
{
	static lv_plugin_res_id res[] = {
			LV_PLUGIN_IMG_ID_ICON_QUALITY_FINE,
			LV_PLUGIN_IMG_ID_ICON_QUALITY_NORMAL,
			LV_PLUGIN_IMG_ID_ICON_QUALITY_BASIC,
	};

	lv_plugin_img_set_src(image_quality_scr_uiflowphoto, res[SysGetFlag(FL_QUALITY)]);
}

static void update_free_pic_number(void)
{
#if (FS_FUNC == ENABLE)
	UIStorageCheck(STORAGE_CHECK_FULL, &g_uiFreePicNum);
#endif
	lv_label_set_text(label_free_pic_scr_uiflowphoto, Get_FreePicNumString(g_uiFreePicNum));
}

static void update_card(void)
{

	/* Update status item data */
	if (System_GetState(SYS_STATE_CARD)  == CARD_REMOVED) {
		lv_plugin_img_set_src(image_storage_scr_uiflowphoto,LV_PLUGIN_IMG_ID_ICON_INTERNAL_FLASH);

	} else if (System_GetState(SYS_STATE_CARD)  == CARD_LOCKED) {
			lv_plugin_img_set_src(image_storage_scr_uiflowphoto,LV_PLUGIN_IMG_ID_ICON_SD_LOCK);
	} else {
		lv_plugin_img_set_src(image_storage_scr_uiflowphoto,LV_PLUGIN_IMG_ID_ICON_SD_CARD);
	}
}

static void update_bust(void)
{
	static lv_plugin_res_id res[] = {
			LV_PLUGIN_IMG_ID_ICON_BUST_OFF,
			LV_PLUGIN_IMG_ID_ICON_BUST_3SHOT,
			LV_PLUGIN_IMG_ID_ICON_BUST_CONTINUE,
	};

	if (SysGetFlag(FL_CONTINUE_SHOT) == CONTINUE_SHOT_OFF || SysGetFlag(FL_CONTINUE_SHOT) == CONTINUE_SHOT_SIDE)
	{
		lv_obj_set_hidden(image_bust_scr_uiflowphoto, true);
	} else {
		lv_plugin_img_set_src(image_bust_scr_uiflowphoto, res[SysGetFlag(FL_CONTINUE_SHOT)]);
		lv_obj_set_hidden(image_bust_scr_uiflowphoto, false);
	}
}

void UIFlowPhoto_update_selftimer_cnt(UINT32 time)
{
	lv_label_set_text(label_self_timer_cnt_scr_uiflowphoto, Get_SelftimerCntString(time));
}

static void update_wb(void)
{
	static lv_plugin_res_id res[] = {
			LV_PLUGIN_IMG_ID_ICON_WB_AUTO,
			LV_PLUGIN_IMG_ID_ICON_WB_DAYLIGHT,
			LV_PLUGIN_IMG_ID_ICON_WB_CLOUDY,
			LV_PLUGIN_IMG_ID_ICON_WB_TUNGSTEN,
			LV_PLUGIN_IMG_ID_ICON_WB_FLUORESCENT
	};


	lv_plugin_img_set_src(image_wb_scr_uiflowphoto, res[SysGetFlag(FL_WB)]);
}

static void update_hdr(void)
{
	static lv_plugin_res_id res[] = {
			LV_PLUGIN_IMG_ID_ICON_HDR_OFF,
			LV_PLUGIN_IMG_ID_ICON_HDR
	};

	lv_plugin_img_set_src(image_hdr_scr_uiflowphoto, res[SysGetFlag(FL_SHDR)]);

	if (SysGetFlag(FL_SHDR) == SHDR_OFF) {
		lv_obj_set_hidden(image_hdr_scr_uiflowphoto, true);
	}
	else{
		lv_obj_set_hidden(image_hdr_scr_uiflowphoto, false);
	}

}

static void update_ev(void)
{
	static lv_plugin_res_id res[] = {
			  LV_PLUGIN_IMG_ID_ICON_EV_P2P0,
		      LV_PLUGIN_IMG_ID_ICON_EV_P1P6,
		      LV_PLUGIN_IMG_ID_ICON_EV_P1P3,
		      LV_PLUGIN_IMG_ID_ICON_EV_P1P0,
		      LV_PLUGIN_IMG_ID_ICON_EV_P0P6,
		      LV_PLUGIN_IMG_ID_ICON_EV_P0P3,
		      LV_PLUGIN_IMG_ID_ICON_EV_P0P0,
		      LV_PLUGIN_IMG_ID_ICON_EV_M0P3,
		      LV_PLUGIN_IMG_ID_ICON_EV_M0P6,
		      LV_PLUGIN_IMG_ID_ICON_EV_M1P0,
		      LV_PLUGIN_IMG_ID_ICON_EV_M1P3,
		      LV_PLUGIN_IMG_ID_ICON_EV_M1P6,
		      LV_PLUGIN_IMG_ID_ICON_EV_M2P0,
	};

	lv_plugin_img_set_src(image_ev_scr_uiflowphoto, res[SysGetFlag(FL_EV)]);
}

static void update_iso(void)
{
	static lv_plugin_res_id res[] = {
			  LV_PLUGIN_IMG_ID_ICON_ISO_AUTO,
			  LV_PLUGIN_IMG_ID_ICON_ISO_100,
			  LV_PLUGIN_IMG_ID_ICON_ISO_200,
			  LV_PLUGIN_IMG_ID_ICON_ISO_400,
			  LV_PLUGIN_IMG_ID_ICON_ISO_800,
			  LV_PLUGIN_IMG_ID_ICON_ISO_1600
	};

	lv_plugin_img_set_src(image_iso_scr_uiflowphoto, res[SysGetFlag(FL_ISO)]);
}

static void update_battery(void)
{
	static lv_plugin_res_id res[] = {
			LV_PLUGIN_IMG_ID_ICON_BATTERY_FULL,
			LV_PLUGIN_IMG_ID_ICON_BATTERY_MED,
			LV_PLUGIN_IMG_ID_ICON_BATTERY_LOW,
			LV_PLUGIN_IMG_ID_ICON_BATTERY_EMPTY,
			LV_PLUGIN_IMG_ID_ICON_BATTERY_ZERO,
			LV_PLUGIN_IMG_ID_ICON_BATTERY_CHARGE
	};


	/* user should call a function to get battery level here */
	lv_plugin_img_set_src(image_battery_scr_uiflowphoto, res[GetBatteryLevel()]);
}

static void update_fd(void)
{
	static lv_plugin_res_id res[] = {
			LV_PLUGIN_IMG_ID_ICON_FACE_OFF,
			LV_PLUGIN_IMG_ID_ICON_FACE_ON
	};

	lv_plugin_img_set_src(image_fd_scr_uiflowphoto, res[SysGetFlag(FL_FD)]);

	if (SysGetFlag(FL_FD) != FD_OFF){
		lv_obj_set_hidden(image_fd_scr_uiflowphoto, false);

	}
	else{
		lv_obj_set_hidden(image_fd_scr_uiflowphoto, true);
	}
}

static void update_fd_frame(void)
{
	if (SysGetFlag(FL_FD) != FD_OFF){
		lv_obj_set_hidden(container_fd_frame_scr_uiflowphoto, false);
	}
	else{
		lv_obj_set_hidden(container_fd_frame_scr_uiflowphoto, true);
	}
}

static void update_dzoom(void)
{
	#if (PHOTO_MODE==ENABLE)
	lv_label_set_text(label_dzoom_scr_uiflowphoto, Get_DZoomRatioString());
	#endif

	lv_obj_set_hidden(label_dzoom_scr_uiflowphoto, true);
}

static void update_icons(void)
{
	update_selftimer();
	update_size();
	update_quality();
	update_free_pic_number();
	update_card();
	update_bust();
	update_hdr();
	update_battery();
	update_wb();
	update_iso();
	update_ev();
	update_fd();
	update_dzoom();
}

static void FlowPhoto_InitStartupFuncs(void)
{
#if(PHOTO_MODE==ENABLE)
	UINT32 StartFunc = 0;

	StartFunc |= UIAPP_PHOTO_AE;
	StartFunc |= UIAPP_PHOTO_AWB;

	Ux_SendEvent(&CustomPhotoObjCtrl, NVTEVT_EXE_START_FUNC, 2, StartFunc, UIAPP_PHOTO_NOWAITIDLE);
#endif
}
#if(PHOTO_MODE==ENABLE)
static void FlowPhoto_SetPreview(void)
{
	DBG_IND("\r\n");

	/* reset state to PHOTO_ST_VIEW */
	gPhotoData.State = PHOTO_ST_VIEW;
	localInfo->bIsPreview = TRUE;
	//#NT#2010/08/20#Lincy Lin -begin
	//#NT#Fine Tune Shot to Shot
#if 0
	localInfo->bIsForceExitQuickView = FALSE;
#endif
	//#NT#2010/08/20#Lincy Lin -end
	// unlock AE/AWB
	FlowPhoto_InitStartupFuncs();


	/* Set to preview mode */
	FlowPhoto_UI_Show(UI_SHOW_PREVIEW, TRUE);

	/* close quick view image */
	FlowPhoto_UI_Show(UI_SHOW_QUICKVIEW, FALSE);


	/* Resume key after quick view completed */
//	Input_SetKeyMask(KEY_PRESS, g_uiMaskKeyPress);
//	Ux_FlushEventByRange(NVTEVT_KEY_EVT_START, NVTEVT_KEY_EVT_END);

	/* Set FD/SD functions againg after exiting Menu */
//	FlowPhoto_SetFdSdProc(TRUE);

	/* Update window info */
//	FlowPhoto_UpdateIcons(TRUE);
	update_icons();

	DBG_IND("{BackPreivew}\r\n");

}
#endif
static void UIFlowPhoto_BackPreviewHandle(void)
{
#if(PHOTO_MODE==ENABLE)

	BOOL isToPreview = FALSE;

	DBG_IND("\r\n");


    #if (PHOTO_EXE_FUNC)
	if ((gPhotoData.QuickViewCount == 0) && (!pPhotoExeInfo->isStartCapture)) {
		isToPreview = TRUE;
	}
    #else
    	isToPreview = FALSE;
    #endif
	//#NT#2010/08/20#Lincy Lin -begin
	//#NT#Fine Tune Shot to Shot
#if 0
	else if (localInfo->bIsForceExitQuickView && (!pPhotoExeInfo->isStartCapture)) {
		isToPreview = TRUE;
		if (localInfo->isQviewTimerStart) {
			GxTimer_StopTimer(&localInfo->uiQviewTimerID);
			localInfo->isQviewTimerStart = FALSE;
		}
	}
#endif
	//#NT#2010/08/20#Lincy Lin -end
	if ((!localInfo->bIsPreview) && isToPreview) {
		FlowPhoto_SetPreview();
	}
#endif
}
#if _FD_FUNC_
static void FlowPhoto_ClrFDRect(void)
{
	UINT32 i;
	DISPLAYER_PARAM   DispLyr = {0} ;
	PDISP_OBJ         pDispObj = NULL;

	pDispObj = disp_getDisplayObject(DISP_1);
	for (i = 0; i < IDE_FD_MAX_NUM; i++) {
		DispLyr.SEL.SET_FDEN.FD_NUM = DISPFD_NUM0 << i;
		DispLyr.SEL.SET_FDEN.bEn = FALSE;
		pDispObj->dispLyrCtrl(DISPLAYER_FD, DISPLAYER_OP_SET_FDEN, &DispLyr);
	}
	pDispObj->load(TRUE);
}
#endif

static void UIFlowPhoto_OnExeCaptureStop(lv_obj_t* obj)
{
#if(PHOTO_MODE==ENABLE)
	// need to stop infinite burst shot
	if (UI_GetData(FL_CONTINUE_SHOT) == CONTINUE_SHOT_BURST)
	{
		Ux_SendEvent(&CustomPhotoObjCtrl, NVTEVT_EXE_CAPTURE_STOP, 0);
	}
#endif
}

static void UIFlowPhoto_OnExeCaptureStart(lv_obj_t* obj)
{

	switch (gPhotoData.State) {
		case PHOTO_ST_VIEW:
      //#NT#2018/12/13#hilex Lin -begin
      // add one more case for capturing
		case PHOTO_ST_ZOOM|PHOTO_ST_VIEW:
      //#NT#2018/12/13#hilex Lin - end
				/* Check if in quick review process */
			if (SysGetFlag(FL_QUICK_REVIEW) != QUICK_REVIEW_0SEC) {
				if (task_qview != NULL && task_qview->prio != LV_TASK_PRIO_OFF) {
//					GxTimer_StopTimer(&g_uiQviewTimerID);
					//g_bQviewTimerStart = FALSE;
					lv_task_set_prio(task_qview, LV_TASK_PRIO_OFF);

					// unlock AE/AWB
					FlowPhoto_InitStartupFuncs();

					/* Set to preview mode */
					FlowPhoto_UI_Show(UI_SHOW_PREVIEW, TRUE);

					/* close quick view image */
					FlowPhoto_UI_Show(UI_SHOW_QUICKVIEW, FALSE);

					/* Resume key after quick view completed */
//					Input_SetKeyMask(KEY_PRESS, g_uiMaskKeyPress);
//					Ux_FlushEventByRange(NVTEVT_KEY_EVT_START, NVTEVT_KEY_EVT_END);

					/* Update window info */
//					FlowPhoto_UpdateIcons(TRUE);
					update_icons();
					return;
				}
			}

			/* Check capture prerequisite */
			if (FlowPhoto_IsStorageErr() == TRUE) {
				DBG_ERR("UIFlowPhoto_OnKeyShutter2: Card or memory full!\r\n");
				gPhotoData.State = PHOTO_ST_WARNING_MENU;
				return;
			}
#if _TODO
			if (GetBatteryLevel() == BATTERY_EXHAUSTED) {
				DBG_ERR("UIFlowPhoto_OnKeyShutter2: Battery is too low!\r\n");
				return NVTEVT_CONSUME;
			}
#endif
			#if 0
			if (UI_GetData(FL_SHDR) == SHDR_ON) {
				DBG_WRN("HDR not support Capture\r\n");
				return NVTEVT_CONSUME;
			}
			#endif

				// check free pic number
			if (SysGetFlag(FL_CONTINUE_SHOT) != CONTINUE_SHOT_OFF) {
				if (FlowPhoto_GetFreePicNum() < 2) {
#if (FS_FUNC == ENABLE)
//					UINT32 uiMsg = 0;
					lv_plugin_res_id id;

					id = (System_GetState(SYS_STATE_CARD)  == CARD_REMOVED) ? LV_PLUGIN_STRING_ID_STRID_PLEASE_INSERT_SD : LV_PLUGIN_STRING_ID_STRID_CARD_FULL;
					gPhotoData.State = PHOTO_ST_WARNING_MENU;
//					Ux_OpenWindow(&UIFlowWndWrnMsgCtrl, 2, uiMsg, FLOWWRNMSG_TIMER_2SEC);
					UIFlowWrnMsgAPI_Open_StringID(id, 2000);
					return;
#endif
				}
			}

			switch (SysGetFlag(FL_SELFTIMER)) {        // set capture mode by system flag
			case SELFTIMER_2SEC:
				/* Suspend all keys, except S2 key while selftimer started */
//				Input_SetKeyMask(KEY_PRESS, FLGKEY_ENTER);
//				FlowPhoto_IconDrawSelftimerTime(&UIFlowPhoto_StaticTXT_SelftimerCntCtrl, 2);
				UIFlowPhoto_update_selftimer_cnt(2);
				gPhotoData.SelfTimerCount = 20;              // set time counter
				gPhotoData.State = PHOTO_ST_SELFTIMER;  // set working state to self-timer state
				//KeyScan_EnableLEDToggle(KEYSCAN_LED_GREEN, TRUE);
				UISound_Play(DEMOSOUND_SOUND_KEY_TONE);

//				if (g_uiSelfTimerID == NULL_TIMER) {
//					g_uiSelfTimerID = GxTimer_StartTimer(100, NVTEVT_01SEC_TIMER, CONTINUE);
//				}

				if(task_selftimer == NULL){
					task_selftimer = lv_task_create(task_selftimer_cb, 100, LV_TASK_PRIO_MID, NULL);
				}
				else{
					lv_task_set_prio(task_selftimer, LV_TASK_PRIO_MID);
				}

				update_selftimer();

				break;
			case SELFTIMER_5SEC:
				/* Suspend all keys, except S2 key while selftimer started */
//				Input_SetKeyMask(KEY_PRESS, FLGKEY_ENTER);
//				FlowPhoto_IconDrawSelftimerTime(&UIFlowPhoto_StaticTXT_SelftimerCntCtrl, 5);
				UIFlowPhoto_update_selftimer_cnt(5);
				gPhotoData.SelfTimerCount = 50;         // set time counter
				gPhotoData.State = PHOTO_ST_SELFTIMER;  // set working state to self-timer state
				//KeyScan_EnableLEDToggle(KEYSCAN_LED_GREEN, TRUE);
				UISound_Play(DEMOSOUND_SOUND_KEY_TONE);

//				if (g_uiSelfTimerID == NULL_TIMER) {
//					g_uiSelfTimerID = GxTimer_StartTimer(100, NVTEVT_01SEC_TIMER, CONTINUE);
//				}
				if(task_selftimer == NULL){
					task_selftimer = lv_task_create(task_selftimer_cb, 100, LV_TASK_PRIO_MID, NULL);
				}
				else{
					lv_task_set_prio(task_selftimer, LV_TASK_PRIO_MID);
				}

				update_selftimer();

				break;
			case SELFTIMER_10SEC:
				/* Suspend all keys, except S2 key while selftimer started */
//				Input_SetKeyMask(KEY_PRESS, FLGKEY_ENTER);
//				FlowPhoto_IconDrawSelftimerTime(&UIFlowPhoto_StaticTXT_SelftimerCntCtrl, 10);
				UIFlowPhoto_update_selftimer_cnt(10);
				gPhotoData.SelfTimerCount = 100;        // set time counter
				gPhotoData.State = PHOTO_ST_SELFTIMER;  // set working state to self-timer state
				//#NT#2012/8/1#Philex - begin
				//KeyScan_EnableLEDToggle(KEYSCAN_LED_GREEN, TRUE);
				UISound_Play(DEMOSOUND_SOUND_KEY_TONE);
				//#NT#2012/8/1#Philex - begin

//				if (g_uiSelfTimerID == NULL_TIMER) {
//					g_uiSelfTimerID = GxTimer_StartTimer(100, NVTEVT_01SEC_TIMER, CONTINUE);
//				}
				if(task_selftimer == NULL){
					task_selftimer = lv_task_create(task_selftimer_cb, 100, LV_TASK_PRIO_MID, NULL);
				}
				else{
					lv_task_set_prio(task_selftimer, LV_TASK_PRIO_MID);
				}

				update_selftimer();

				break;
			default:
				/* Suspend all keys before sending capture command */
//				Input_SetKeyMask(KEY_PRESS, FLGKEY_KEY_MASK_NULL);
//				Ux_FlushEventByRange(NVTEVT_KEY_EVT_START, NVTEVT_KEY_EVT_END);

				//#NT#2016/04/28#Lincy Lin -begin
				//#NT#Fix sidebyside capture quickview will have preview image on background
				FlowPhoto_UI_Show(UI_SHOW_PREVIEW, FALSE);
				//#NT#2016/04/28#Lincy Lin -end

				/* Clear the whole OSD screen */
//				UxCtrl_SetAllChildShow(pCtrl, FALSE);

				gPhotoData.State = PHOTO_ST_CAPTURE;    // enter capture state
				FlowPhoto_DoCapture();                  // do capture directly
				break;
			}
			break;

		case PHOTO_ST_SELFTIMER:

			/* check is task finished */
			if(task_selftimer == NULL){

			gPhotoData.State = PHOTO_ST_VIEW;
			gPhotoData.SelfTimerCount = 0;
			//KeyScan_EnableLEDToggle(KEYSCAN_LED_GREEN, FALSE);
//			FlowPhoto_IconHideSelftimer(&UIFlowPhoto_StaticTXT_SelftimerCntCtrl);
			update_selftimer();
			}

			// resume press mask to default
//			Input_SetKeyMask(KEY_PRESS, g_uiMaskKeyPress);
			break;
	}
}

#if 1
static void UIFlowPhoto_OnExeZoomInStart(lv_obj_t* obj)
{
#if(PHOTO_MODE==ENABLE && DZOOM_FUNC)



		switch (gPhotoData.State) {

		case PHOTO_ST_VIEW:
		case PHOTO_ST_VIEW|PHOTO_ST_ZOOM:
//			Input_SetKeyMask(KEY_PRESS, FLGKEY_KEY_MASK_NULL);
			gPhotoData.State |= PHOTO_ST_ZOOM;
			Ux_SendEvent(&CustomPhotoObjCtrl, NVTEVT_EXE_ZOOM, 1, UI_ZOOM_CTRL_IN);
			break;

		default:
			break;
		}

#endif
	return;
}

static void UIFlowPhoto_OnExeZoomInStop(lv_obj_t* obj)
{
	Ux_SendEvent(&CustomPhotoObjCtrl, NVTEVT_EXE_ZOOM, 1, UI_ZOOM_CTRL_STOP);
	gPhotoData.State &= ~PHOTO_ST_ZOOM;
//	Input_SetKeyMask(KEY_PRESS, g_uiMaskKeyPress);

	return;
}

static void UIFlowPhoto_OnExeZoomOutStart(lv_obj_t* obj)
{
#if(PHOTO_MODE==ENABLE)

		switch (gPhotoData.State) {

		case PHOTO_ST_VIEW:
		case PHOTO_ST_VIEW|PHOTO_ST_ZOOM:
			Input_SetKeyMask(KEY_PRESS, FLGKEY_KEY_MASK_NULL);
			gPhotoData.State |= PHOTO_ST_ZOOM;
			Ux_SendEvent(&CustomPhotoObjCtrl, NVTEVT_EXE_ZOOM, 1, UI_ZOOM_CTRL_OUT);
			break;

		default:
			break;
		}
#endif

	return;
}

static void UIFlowPhoto_OnExeZoomOutStop(lv_obj_t* obj)
{
#if(PHOTO_MODE==ENABLE)

		Ux_SendEvent(&CustomPhotoObjCtrl, NVTEVT_EXE_ZOOM, 1, UI_ZOOM_CTRL_STOP);
		gPhotoData.State &= ~PHOTO_ST_ZOOM;
		/* Resume keys after ZoomIn released */
//		Input_SetKeyMask(KEY_PRESS, g_uiMaskKeyPress);

#endif

	return;
}

#endif

void UIFlowPhoto_OnKeyMenu(lv_obj_t* obj)
{
	UINT32  uiSoundMask;

	switch (gPhotoData.State) {
	case PHOTO_ST_VIEW:			// enable shutter2 sound (shutter2 as OK key in menu)
	case PHOTO_ST_VIEW|PHOTO_ST_ZOOM:			// enable shutter2 sound (shutter2 as OK key in menu)
		uiSoundMask = Input_GetKeySoundMask(KEY_PRESS);
		uiSoundMask |= FLGKEY_SHUTTER2;
		Input_SetKeySoundMask(KEY_PRESS, uiSoundMask);

		/* Force to lock FD/SD functions before opening Menu */
		FlowPhoto_ForceLockFdSd();
//		Input_SetKeyMask(KEY_RELEASE, FLGKEY_KEY_MASK_NULL);

		// Open common mix (Item + Option) menu
//		Ux_OpenWindow((VControl *)(&MenuCommonItemCtrl), 0);
		lv_plugin_scr_open(UIFlowMenuCommonItem, NULL);
		gPhotoData.State = PHOTO_ST_MENU;
		break;
	}

	return ;
}

void UIFlowPhoto_OnChildScrClose(lv_obj_t* obj, LV_USER_EVENT_NVTMSG_DATA* data)
{
	UINT32  uiSoundMask;

	DBG_DUMP("%s\r\n", __func__);

	/* do nothing if current system mode is not matched */
	if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_PHOTO){
		DBG_WRN("system current mode is not photo\r\n");
		return;
	}

	set_indev_keypad_group(obj);

	/* Set key mask to self-original state */
//	Ux_FlushEventByRange(NVTEVT_KEY_EVT_START, NVTEVT_KEY_EVT_END);
//	Input_SetKeyMask(KEY_PRESS, g_uiMaskKeyPress);
//	Input_SetKeyMask(KEY_RELEASE, g_uiMaskKeyRelease);
//	Input_SetKeyMask(KEY_CONTINUE, g_uiMaskKeyContinue);

	switch (gPhotoData.State) {
	case PHOTO_ST_WARNING_MENU:

		if(data){
			if (data->paramNum > 0) {
				if (data->paramArray[0] == NVTRET_ENTER_MENU) {
					/* Force to lock FD/SD functions before opening Menu */
					FlowPhoto_ForceLockFdSd();
					/* Create Menu window */
					gPhotoData.State = PHOTO_ST_MENU;
//					Ux_OpenWindow(&MenuCommonItemCtrl, 0);
					lv_plugin_scr_open(UIFlowMenuCommonItem, NULL);
					return;
				}
			}
		}
		gPhotoData.State = PHOTO_ST_VIEW;
		break;

	case PHOTO_ST_MENU:
		// disable shutter2 sound
		uiSoundMask = Input_GetKeySoundMask(KEY_PRESS);
		uiSoundMask &= ~FLGKEY_SHUTTER2;
		Input_SetKeySoundMask(KEY_PRESS, uiSoundMask);

		gPhotoData.State = PHOTO_ST_VIEW;

		FlowPhoto_InitCfgSetting();
		// set image ratio here
		//Ux_SendEvent(&CustomPhotoObjCtrl,NVTEVT_EXE_IMAGE_RATIO,1,GetPhotoSizeRatio(UI_GetData(FL_PHOTO_SIZE)));

		// unlock AE/AWB
		FlowPhoto_InitStartupFuncs();

        /* Set to preview mode */
		FlowPhoto_UI_Show(UI_SHOW_PREVIEW, TRUE);

		/* close quick view image */
		FlowPhoto_UI_Show(UI_SHOW_QUICKVIEW, FALSE);

		/* Resume key after normal capture completed */
//		Input_SetKeyMask(KEY_PRESS, g_uiMaskKeyPress);

		/* Set FD/SD functions againg after exiting Menu */
//		FlowPhoto_SetFdSdProc(TRUE);
		/* Update window info */
//		FlowPhoto_UpdateIcons(TRUE);
		update_icons();

		break;
	}

//	Ux_DefaultEvent(pCtrl, NVTEVT_CHILD_CLOSE, paramNum, paramArray);
	return;
}

void UIFlowPhoto_OnStorageInit(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
#if (CALIBRATION_FUNC == ENABLE)
	{
		// check if enter engineer mode
		if (EngineerMode_CheckEng())
		{
			//#NT#2016/06/23#Niven Cho -begin
			//#NT#Enter calibration by cgi event or command event
			Ux_PostEvent(NVTEVT_KEY_CALIBRATION, 0);
			//#NT#2016/06/23#Niven Cho -end
		}
	}
#endif
	return;
}

void UIFlowPhoto_OnStorageChange(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	//#NT#2016/10/04#Lincy Lin -begin
	//#NT#Support SD hot plug function
#if (SDHOTPLUG_FUNCTION == ENABLE)
	Ux_PostEvent(NVTEVT_SYSTEM_MODE, 1, System_GetState(SYS_STATE_CURRMODE));
#endif
	//#NT#2016/10/04#Lincy Lin -end
	return;
}
void UIFlowPhoto_OnBattery(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	static volatile BOOL bBatteryOn = FALSE;

//	UxState_SetData(&UIFlowWndPhoto_StatusICN_BatteryCtrl, STATE_CURITEM, GetBatteryLevel());
	update_battery();

	if (KeyScan_IsACIn()) {

		bBatteryOn = !bBatteryOn;
//		UxCtrl_SetShow(&UIFlowWndPhoto_StatusICN_BatteryCtrl, bBatteryOn);
		lv_obj_set_hidden(image_battery_scr_uiflowphoto, !bBatteryOn);

	} else {

//		UxCtrl_SetShow(&UIFlowWndPhoto_StatusICN_BatteryCtrl, TRUE);
		lv_obj_set_hidden(image_battery_scr_uiflowphoto, false);
	}
	return ;
}
void UIFlowPhoto_OnBatteryLow(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
//	Ux_OpenWindow(&UIFlowWndWrnMsgCtrl, 2, UIFlowWndWrnMsg_StatusTXT_Msg_STRID_BATTERY_LOW, FLOWWRNMSG_TIMER_2SEC);
	UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_BATTERY_LOW, 2);
	return ;
}


static void UIFlowPhoto_ScrOpen(lv_obj_t* obj)
{

	set_indev_keypad_group(obj);
	update_fd_frame();

	//#NT#2016/10/04#Lincy Lin -begin
	//#NT#Support SD hot plug function
#if (SDHOTPLUG_FUNCTION == ENABLE)
	if (UIStorageCheck(STORAGE_CHECK_ERROR, NULL) == TRUE) {

		/* never closed */
		UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_PLEASE_INSERT_SD, 0);
		return;
	}
#endif
	//#NT#2016/10/04#Lincy Lin -end
	UIFlowInfoPhoto = UIFlowInfoPhotoInitVal;
	FlowPhoto_InitCfgSetting();
	FlowPhoto_InitStartupFuncs();

	/* Init window key mask variables & set key press/release/continue mask */
	gPhotoData.State = PHOTO_ST_VIEW;
//	g_uiMaskKeyPress = PHOTO_KEY_PRESS_MASK;
//	g_uiMaskKeyRelease = PHOTO_KEY_RELEASE_MASK;
//	g_uiMaskKeyContinue = PHOTO_KEY_CONTINUE_MASK;
	Ux_FlushEventByRange(NVTEVT_KEY_EVT_START, NVTEVT_KEY_EVT_END);

//	Input_SetKeyMask(KEY_PRESS, g_uiMaskKeyPress);
//	Input_SetKeyMask(KEY_RELEASE, g_uiMaskKeyRelease);
//	Input_SetKeyMask(KEY_CONTINUE, g_uiMaskKeyContinue);

	gPhotoData.SysTimeCount = 0;
	/* Update window info */
//	FlowPhoto_UpdateIcons(TRUE);
	update_icons();


	/* set FD/SD feature */
//	FlowPhoto_SetFdSdProc(TRUE);

	return;
}

static void UIFlowPhoto_ScrClose(lv_obj_t* obj)
{
	DBG_DUMP("%s\r\n", __func__);

	/* do nothing if current system mode is not matched */
	if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_PHOTO){
		DBG_WRN("system current mode is not photo\r\n");
		return;
	}

#if _FD_FUNC_
	FlowPhoto_ClrFDRect();
#endif
	// stop SelTimerID/QViewTimerID
//	if (g_uiSelfTimerID != NULL_TIMER) {
//		GxTimer_StopTimer(&g_uiSelfTimerID);
//	}

//	if (g_uiQviewTimerID != NULL_TIMER) {
//		GxTimer_StopTimer(&g_uiQviewTimerID);
//	}

	if(task_qview)
	{
		lv_task_del(task_qview);
		task_qview = NULL;
	}

	if(task_selftimer){
		lv_task_del(task_selftimer);
		task_selftimer = NULL;
		update_selftimer();
	}

	/* Once close photo window, reset selftimer to off state */
//    SysSetFlag(FL_SELFTIMER, SELFTIMER_OFF);

	g_bRedLEDOn = FALSE;
	//KeyScan_TurnOffLED(KEYSCAN_LED_RED);

	gPhotoData.QuickViewCount = 0;
	gPhotoData.SelfTimerCount = 0;

	FlowPhoto_InitStartupFuncs();

    //#NT#2016/10/04#Lincy Lin -begin
	//#NT#Support SD hot plug function
	/* Set to preview mode */
	FlowPhoto_UI_Show(UI_SHOW_PREVIEW, TRUE);

	/* close quick view image */
	FlowPhoto_UI_Show(UI_SHOW_QUICKVIEW, FALSE);
	//#NT#2016/10/04#Lincy Lin -end

	/* Reset key press/release/continue mask to default */
	Ux_FlushEventByRange(NVTEVT_KEY_EVT_START, NVTEVT_KEY_EVT_END);

	return;
}

static void UIFlowPhoto_OnQVStart(void)
{
	/* Set to quick review mode */
	FlowPhoto_UI_Show(UI_SHOW_QUICKVIEW, TRUE);

	switch (SysGetFlag(FL_QUICK_REVIEW)) {
	case QUICK_REVIEW_2SEC:
		gPhotoData.QuickViewCount = 15;
		break;
	case QUICK_REVIEW_5SEC:
		gPhotoData.QuickViewCount = 45;
		break;
	case QUICK_REVIEW_0SEC:
		gPhotoData.QuickViewCount = 1;
		break;
	}

	if (task_qview == NULL) {
		/* Resume only S2 key while quick view timer started */
//		Input_SetKeyMask(KEY_PRESS, FLGKEY_SHUTTER2);
//		g_uiQviewTimerID = GxTimer_StartTimer(100, NVTEVT_01SEC_TIMER, CONTINUE);
		task_qview = lv_task_create(task_qview_cb, 100, LV_TASK_PRIO_MID, NULL);
	}
	else{
		lv_task_set_prio(task_qview, LV_TASK_PRIO_MID);
	}
}


static void UIFlowPhoto_OnDZoomStepChange(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{

	/* Update Digital zoom icon */
//	FlowPhoto_IconDrawDZoom(&UIFlowPhoto_StaticTXT_DZoomCtrl);
	update_dzoom();

	return;
}

static void UIFlowPhoto_OnFocusEnd(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	return;
}

static void UIFlowPhoto_OnFdEnd(void)
{
	if (SysGetFlag(FL_FD) != FD_OFF) {
		if ((gPhotoData.State != PHOTO_ST_CAPTURE) && (gPhotoData.QuickViewCount == 0)) {
			/* Enable FD frame show */
			lv_obj_set_hidden(image_fd_scr_uiflowphoto, false);
//			UxCtrl_SetShow(&UIFlowPhoto_PNL_FDFrameCtrl, TRUE);
		}
	}
}

void UIFlowPhoto_OnKeyCalibration(lv_obj_t* obj)
{
#if (CALIBRATION_FUNC == ENABLE)
	{
		Ux_SendEvent(0, NVTEVT_SYSTEM_MODE, 1, PRIMARY_MODE_MAIN);
		EngineerMode_Open();
	}
#endif
	return;
}

static void task_qview_cb(lv_task_t* task)
{
	if (gPhotoData.QuickViewCount != 0) {
		gPhotoData.QuickViewCount--;
		if (gPhotoData.QuickViewCount == 0) {

			lv_task_set_prio(task, LV_TASK_PRIO_OFF);
			lv_obj_set_hidden(UIFlowPhoto, false);
			UIFlowPhoto_BackPreviewHandle();

			/* set DirID/FileID */
#if !USE_FILEDB
#if (USE_DCF)
			{
				UINT32 uhFilesysDirNum, uhFilesysFileNum;

				DCF_GetNextID(&uhFilesysDirNum, &uhFilesysFileNum);
				SysSetFlag(FL_DCF_DIR_ID, uhFilesysDirNum);
				SysSetFlag(FL_DCF_FILE_ID, uhFilesysFileNum);
			}
#endif
#endif
		}
	}

	return;
}

static void task_selftimer_cb(lv_task_t* task)
{
	if (gPhotoData.SelfTimerCount != 0) {
		gPhotoData.SelfTimerCount--;
		if (gPhotoData.SelfTimerCount == 0) {
			lv_task_set_prio(task, LV_TASK_PRIO_OFF);
		}
		FlowPhoto_OnTimer01SecIndex();
	}

	return;
}

void UIFlowPhoto_UpdateInfo(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	if(msg == NULL){
		DBG_ERR("msg is NULL!\r\n");
		return;
	}

	if (msg->paramNum != 1) {
		DBG_ERR("paramNum = %d\r\n", msg->paramNum);
		return;
	}

	switch (msg->paramArray[0]) {
	case UIAPPPHOTO_CB_PREVIEW_STABLE:

		break;
	case UIAPPPHOTO_CB_CAPSTART:

		if(obj == UIFlowPhoto)
			lv_obj_set_hidden(obj, true);

		localInfo->bIsPreview = FALSE;
		break;
	case UIAPPPHOTO_CB_QVSTART:
		UIFlowPhoto_OnQVStart();
		break;
	case UIAPPPHOTO_CB_JPGOK:
		break;
	case UIAPPPHOTO_CB_FSTOK:
		/* Update window info */
		update_icons();
//		FlowPhoto_UpdateIcons(TRUE);
		break;
	case UIAPPPHOTO_CB_CAPTUREEND:

		if (SysGetFlag(FL_QUICK_REVIEW) == QUICK_REVIEW_0SEC) {
			if(obj == UIFlowPhoto)
				lv_obj_set_hidden(obj, false);
			UIFlowPhoto_BackPreviewHandle();
		}

		break;

	case UIAPPPHOTO_CB_FDEND:
		UIFlowPhoto_OnFdEnd();
		break;

	case UIAPPPHOTO_CB_SDEND:
		if (SysGetFlag(FL_FD) == FD_SMILE) {
			if ((gPhotoData.State != PHOTO_ST_CAPTURE) && (gPhotoData.QuickViewCount == 0)) {
				/* Enable FD frame show */
				lv_obj_set_hidden(container_fd_frame_scr_uiflowphoto, false);
//				UxCtrl_SetShow(&UIFlowPhoto_PNL_FDFrameCtrl, TRUE);
				#if(OSD2_FUNC)
				FlowPhoto_UI_Show(UI_SHOW_INFO, FALSE); //FD layer
				#endif
//				Input_SetKeyMask(KEY_PRESS, FLGKEY_KEY_MASK_NULL);

			}
		}
		break;
	case UIAPPPHOTO_CB_UCTRL_UPDATE_UI:
		FlowPhoto_InitCfgSetting();
//		FlowPhoto_UpdateIcons(TRUE);
		update_icons();
//		DBG_IND("PhotoUpdateUI\r\n");
		break;
	}

	return ;
}

static void UIFlowPhoto_NVTMSG(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{

	switch(msg->event)
	{
	case NVTEVT_UPDATE_INFO:
		UIFlowPhoto_UpdateInfo(obj, msg);
		break;

	case NVTEVT_CB_ZOOM:
		UIFlowPhoto_OnDZoomStepChange(obj, msg);
		break;

	case NVTEVT_ALGMSG_FOCUSEND:
		UIFlowPhoto_OnFocusEnd(obj, msg);
		break;

	case NVTEVT_STORAGE_INIT:
		UIFlowPhoto_OnStorageInit(obj, msg);
		break;

	case NVTEVT_STORAGE_CHANGE:
		UIFlowPhoto_OnStorageChange(obj, msg);
		break;

	case NVTEVT_BATTERY:
		UIFlowPhoto_OnBattery(obj, msg);
		break;

	case NVTEVT_BATTERY_LOW:
		UIFlowPhoto_OnBatteryLow(obj, msg);
		break;

	default:
		break;

	}
}

static void UIFlowPhoto_Key(lv_obj_t* obj, uint32_t key)
{
	switch(key)
	{
	case LV_USER_KEY_SHUTTER2:
	{
		UIFlowPhoto_OnExeCaptureStart(obj);
		break;
	}

	case LV_USER_KEY_ZOOMIN:
	{
		UIFlowPhoto_OnExeZoomInStart(obj);
		break;
	}

	case LV_USER_KEY_ZOOMOUT:
	{
		UIFlowPhoto_OnExeZoomOutStart(obj);
		break;
	}

	case LV_USER_KEY_MENU:
	{
		UIFlowPhoto_OnKeyMenu(obj);
		break;
	}

	case LV_USER_KEY_CALIBRATION:
		UIFlowPhoto_OnKeyCalibration(obj);
		break;

	default:
		break;
	}
}

static void UIFlowPhoto_KeyRelease(lv_obj_t* obj, uint32_t key)
{
	switch(key)
	{
	case LV_USER_KEY_SHUTTER2:
	{
		UIFlowPhoto_OnExeCaptureStop(obj);
		break;
	}

	case LV_USER_KEY_ZOOMIN:
	{
		UIFlowPhoto_OnExeZoomInStop(obj);
		break;
	}

	case LV_USER_KEY_ZOOMOUT:
	{
		UIFlowPhoto_OnExeZoomOutStop(obj);
		break;
	}

	default:
		break;
	}
}

void UIFlowPhotoEventCallback(lv_obj_t* obj, lv_event_t event)
{

	switch(event)
	{
	case LV_PLUGIN_EVENT_SCR_OPEN:
		UIFlowPhoto_ScrOpen(obj);
		break;

	case LV_PLUGIN_EVENT_SCR_CLOSE:
		UIFlowPhoto_ScrClose(obj);
		break;

	case LV_PLUGIN_EVENT_CHILD_SCR_CLOSE:
		UIFlowPhoto_OnChildScrClose(obj, (LV_USER_EVENT_NVTMSG_DATA*)lv_event_get_data());
		break;

	case LV_EVENT_CLICKED:
		UIFlowPhoto_OnKeyMenu(obj);
		break;

	case LV_EVENT_KEY:
	{
		uint32_t* key = (uint32_t*)lv_event_get_data();

		/* handle key event */
		UIFlowPhoto_Key(obj, *key);

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

	case LV_USER_EVENT_KEY_RELEASE:
	{
		uint32_t* key = (uint32_t*)lv_event_get_data();
		UIFlowPhoto_KeyRelease(obj, *key);

		break;
	}


	/* handle nvt event except key event */
	case LV_USER_EVENT_NVTMSG:
	{
		const LV_USER_EVENT_NVTMSG_DATA* msg = (const LV_USER_EVENT_NVTMSG_DATA*)lv_event_get_data();

		UIFlowPhoto_NVTMSG(obj, msg);
		break;
	}
	}
}
