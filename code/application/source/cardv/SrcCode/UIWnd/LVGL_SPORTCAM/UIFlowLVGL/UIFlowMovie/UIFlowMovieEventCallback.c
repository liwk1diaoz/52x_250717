#include "PrjInc.h"
#include "GxTime.h"
#include "GxStrg.h"
#include "UIFlowLVGL/UIFlowLVGL.h"
#include "UIFlowLVGL/UIFlowWrnMsg/UIFlowWrnMsgAPI.h"
#include <kwrap/debug.h>
#include "SysMain.h"
#if (USE_DCF == ENABLE)
#include "DCF.h"
#endif


extern BOOL FlowMovie_IsStorageErr2(lv_obj_t* parent,BOOL IsCheckFull);

static lv_group_t* gp = NULL;
static lv_task_t* task_1sec_period = NULL;
//#NT#2021/09/13#Philex Lin - begin
static lv_task_t* 	gUIMotionDetTimerID = NULL;
static BOOL    	g_uiRecordIngMotionDet = TRUE;
//#NT#2021/09/13#Philex Lin - end

uint16_t warn_msgbox_auto_close_ms = 6000;
uint32_t warn_msgbox_auto_infinite_ms = 0xffffffff;

//MOVIE_SIZE_TAG
static CHAR    *resolution_Buf[MOVIE_SIZE_ID_MAX] = {
	[MOVIE_SIZE_FRONT_2880x2160P50] = "UHD P50",
	[MOVIE_SIZE_FRONT_3840x2160P30] = "UHD P30",
	[MOVIE_SIZE_FRONT_2880x2160P24] = "UHD P24",
	[MOVIE_SIZE_FRONT_2704x2032P60] = "2.7K P60",
	[MOVIE_SIZE_FRONT_2560x1440P80] = "QHD P80",
	[MOVIE_SIZE_FRONT_2560x1440P60] = "QHD P60",
	[MOVIE_SIZE_FRONT_2560x1440P30] = "QHD P30",
	[MOVIE_SIZE_FRONT_2304x1296P30] = "3MHD P30",
	[MOVIE_SIZE_FRONT_1920x1080P120] = "FHD P120",
	[MOVIE_SIZE_FRONT_1920x1080P96] = "FHD P96",
	[MOVIE_SIZE_FRONT_1920x1080P60] = "FHD P60",
	[MOVIE_SIZE_FRONT_1920x1080P30] = "FHD P30",
	[MOVIE_SIZE_FRONT_1280x720P120] = "HD P120",
	[MOVIE_SIZE_FRONT_1280x720P240] = "HD P240",
	[MOVIE_SIZE_FRONT_1280x720P60] = "HD P60",
	[MOVIE_SIZE_FRONT_1280x720P30] = "HD P30",
	[MOVIE_SIZE_FRONT_848x480P30] = "WVGA P30",
	[MOVIE_SIZE_FRONT_640x480P240] = "VGA P240",
	[MOVIE_SIZE_FRONT_640x480P30] = "VGA P30",
	[MOVIE_SIZE_FRONT_320x240P30] = "QVGA P30",
	[MOVIE_SIZE_DUAL_2560x1440P30_1280x720P30] = "QHD P30+HD P30",
	[MOVIE_SIZE_DUAL_2560x1440P30_1920x1080P30] = "QHD P30+FHD P30",
	[MOVIE_SIZE_DUAL_2304x1296P30_1280x720P30] = "3MHD P30+HD P30",
	[MOVIE_SIZE_DUAL_1920x1080P30_1920x1080P30] = "FHD P30+FHD P30",
	[MOVIE_SIZE_DUAL_1920x1080P30_1280x720P30] = "FHD P30+HD P30",
	[MOVIE_SIZE_DUAL_1920x1080P30_848x480P30] = "FHD P30+WVGA P30",
	[MOVIE_SIZE_CLONE_1920x1080P30_1920x1080P30] = "FHD P30+FHD P30",
	[MOVIE_SIZE_CLONE_1920x1080P30_1280x720P30] = "FHD P30+HD P30",
	[MOVIE_SIZE_CLONE_2560x1440P30_848x480P30] = "QHD P30+WVGA P30",
	[MOVIE_SIZE_CLONE_2304x1296P30_848x480P30] = "3MHD P30+WVGA P30",
	[MOVIE_SIZE_CLONE_1920x1080P60_848x480P30] = "FHD P60+WVGA P30",
	[MOVIE_SIZE_CLONE_1920x1080P60_640x360P30] = "FHD P60+VGA P30",
	[MOVIE_SIZE_CLONE_1920x1080P30_848x480P30] = "FHD P30+WVGA P30",
	[MOVIE_SIZE_CLONE_2048x2048P30_480x480P30] = "2048x2048 P30 + 480x480 P30",
};

/****************************************************************
 * do not access variable directly, use get_rec_status() instead
 **** ***********************************************************/
static bool rec_status = false;
static bool pim_status = false;

static void update_icons(void);

static bool get_pim_status(void)
{
	return pim_status;
}

static void set_pim_status(bool status)
{
	pim_status = status;
}

static bool get_rec_status(void)
{
	return rec_status;
}

static void set_rec_status(bool status)
{
	rec_status = status;

	if(status){
		lv_obj_set_hidden(image_rec_ellipse_scr_uiflowmovie, false);
		FlowMovie_StartRec();
		gMovData.State = MOV_ST_REC;
	}
	else{
		gMovData.State = MOV_ST_VIEW;
		FlowMovie_StopRec();
		update_icons();
	}

}

static void toggle_rec_status(void)
{
	set_rec_status(!get_rec_status());
}

//static void  warn_msgbox(lv_obj_t* parent, const char* message, uint16_t auto_close_time_ms)
//{
//	lv_obj_t* msgbox = lv_msgbox_create(parent, message_box_1_scr_2);
//	lv_msgbox_set_text(msgbox, message);
//	lv_msgbox_start_auto_close(msgbox, auto_close_time_ms);
//
//	lv_group_t* gp = lv_obj_get_group(parent);
//
//	lv_group_add_obj(gp, msgbox);
//	lv_group_focus_obj(msgbox);
//}
//
//void  warn_msgbox_string_id(lv_obj_t* parent, lv_plugin_res_id id, uint16_t auto_close_time_ms)
//{
//	const lv_plugin_string_t* string = lv_plugin_get_string(id);
//	warn_msgbox(parent, string->ptr, auto_close_time_ms);
//}

static void update_date_time(void)
{
    struct tm Curr_DateTime;
    GxTime_GetTime(&Curr_DateTime);

	lv_label_set_text_fmt(label_date_scr_uiflowmovie, "%04d/%02d/%02d", Curr_DateTime.tm_year, Curr_DateTime.tm_mon, Curr_DateTime.tm_mday);
	lv_label_set_text_fmt(label_time_scr_uiflowmovie, "%02d:%02d:%02d", Curr_DateTime.tm_hour, Curr_DateTime.tm_min, Curr_DateTime.tm_sec);
}

static void update_rec_ellipse(void)
{
	if (get_rec_status()){
		bool is_hidden = lv_obj_get_hidden(image_rec_ellipse_scr_uiflowmovie);
		lv_obj_set_hidden(image_rec_ellipse_scr_uiflowmovie, !is_hidden);
	}
	else{
		lv_obj_set_hidden(image_rec_ellipse_scr_uiflowmovie, true);
	}
}

static void update_pim(void)
{
	if (get_pim_status()){
		bool is_hidden = lv_obj_get_hidden(image_pim_scr_uiflowmovie);
		lv_obj_set_hidden(image_pim_scr_uiflowmovie, !is_hidden);
	}
	else{
		lv_obj_set_hidden(image_pim_scr_uiflowmovie, true);
	}
}

static void update_max_rec_time(void)
{

	lv_obj_set_hidden(label_rec_time_scr_uiflowmovie,true);
	lv_obj_set_hidden(label_maxtime_scr_uiflowmovie,false);
	if (System_GetState(SYS_STATE_CARD)  == CARD_REMOVED) {

		lv_label_set_text_fmt(label_maxtime_scr_uiflowmovie, "%02d:%02d:%02d", 0, 0, 0);

	} else {
		UINT32 free_sec = Movie_GetFreeSec();

		if (free_sec <= 2) {

			FlowMovie_SetRecMaxTime(0);

		} else if (free_sec > 86399) {
			///23:59:59
			FlowMovie_SetRecMaxTime(86399);
		} else {
			FlowMovie_SetRecMaxTime(free_sec);
		}
		lv_label_set_text_fmt(label_maxtime_scr_uiflowmovie, "%02d:%02d:%02d", free_sec / 3600, (free_sec % 3600) / 60, (free_sec % 3600) % 60);
	}
}

static void update_rec_time(void)
{
   UINT32 rec_sec = FlowMovie_GetRecCurrTime();

	if(lv_obj_get_hidden(label_rec_time_scr_uiflowmovie))
	{
		lv_obj_set_hidden(label_rec_time_scr_uiflowmovie,false);
		lv_obj_set_hidden(label_maxtime_scr_uiflowmovie,true);
	}

	lv_label_set_text_fmt(label_rec_time_scr_uiflowmovie, "%02d:%02d:%02d", rec_sec / 3600, (rec_sec % 3600) / 60, (rec_sec % 3600) % 60);
}

static void update_size(void)
{
	lv_label_set_text(label_size_scr_uiflowmovie, resolution_Buf[SysGetFlag(FL_MOVIE_SIZE)]);
}

static void update_hdr(void)
{
	lv_obj_set_hidden(image_hdr_scr_uiflowmovie,!SysGetFlag(FL_MOVIE_HDR));
}

static void update_motionDet(void)
{
	lv_obj_set_hidden(image_motiondetect_scr_uiflowmovie,!SysGetFlag(FL_MOVIE_MOTION_DET));
}


/**************************************************************************************
 *
 * example how to update icon by flag , it's not like nvt gui framework's status icon,
 * resource must be specified in the user code.
 *
 * resource id is aligned with enumerate in the UIInfo.h,
 * out of array index is not checked here, user should be careful.
 *
 **************************************************************************************/
static void update_cyclic_rec(void)
{
	static lv_plugin_res_id res[] = {
			LV_PLUGIN_RES_ID_NONE,
			LV_PLUGIN_IMG_ID_ICON_CYCLIC_REC_1MIN,
			LV_PLUGIN_IMG_ID_ICON_CYCLIC_REC_3MIN,
			LV_PLUGIN_IMG_ID_ICON_CYCLIC_REC_5MIN,
			LV_PLUGIN_IMG_ID_ICON_CYCLIC_REC_10MIN,
	};


	lv_plugin_res_id img_id = res[SysGetFlag(FL_MOVIE_CYCLIC_REC)];


	/* hide cyclic rec icon if disabled */
	if(img_id != LV_PLUGIN_RES_ID_NONE){
		lv_obj_set_hidden(image_cyclic_rec_scr_uiflowmovie, false);
		lv_plugin_img_set_src(image_cyclic_rec_scr_uiflowmovie, img_id);
	}
	else{
		lv_obj_set_hidden(image_cyclic_rec_scr_uiflowmovie, true);
	}
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
	lv_plugin_img_set_src(image_battery_scr_uiflowmovie, res[0]);

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
	/* user should call a function to get battery level here */
	lv_plugin_img_set_src(image_ev_scr_uiflowmovie, res[SysGetFlag(FL_EV)]);
}

static void update_card(void)
{

	/* Update status item data */
	if (System_GetState(SYS_STATE_CARD)  == CARD_REMOVED) {
		lv_plugin_img_set_src(image_storage_scr_uiflowmovie,LV_PLUGIN_IMG_ID_ICON_INTERNAL_FLASH);

	} else if (System_GetState(SYS_STATE_CARD)  == CARD_LOCKED) {
			lv_plugin_img_set_src(image_storage_scr_uiflowmovie,LV_PLUGIN_IMG_ID_ICON_SD_LOCK);
	} else {
		lv_plugin_img_set_src(image_storage_scr_uiflowmovie,LV_PLUGIN_IMG_ID_ICON_SD_CARD);
	}
}

static void update_wifi(void)
{
	static lv_plugin_res_id res[] = {
			LV_PLUGIN_IMG_ID_ICON_WIFI_OFF,
		      LV_PLUGIN_IMG_ID_ICON_WIFI_ON,
		      LV_PLUGIN_IMG_ID_ICON_CLOUD_ON,
	};

	lv_plugin_img_set_src(image_wifi_scr_uiflowmovie, res[SysGetFlag(FL_WIFI)]);
}


void UIFlowMovie_update_icons(void)
{
	update_icons();
}

static void update_icons(void)
{
	update_date_time();
	update_rec_ellipse();
	update_pim();
	update_size();
	update_ev();
	update_card();
	update_wifi();
	update_hdr();
	update_motionDet();
	update_cyclic_rec();
	update_battery();
	update_max_rec_time();

}

static void UIFlowMovie_MotionDetect(void)
{
	static UINT32  uiMotionDetGo = 0;
	static UINT32  uiMotionDetStop = 0;
	static BOOL    bMotionDetRec = FALSE; // TRUE: trigger record by MD
	static UINT32  uiMotionDetRet = 0;
	Ux_SendEvent(0, NVTEVT_EXE_MOTION_DET_RUN, 1, (UINT32)&uiMotionDetRet);

	if (uiMotionDetRet == TRUE)	{
		uiMotionDetGo++;
		if (uiMotionDetGo >= 2) {
			uiMotionDetStop = 0;
			// Recording of modtion detection in pure CarDV path
			if (!((gMovData.State == MOV_ST_REC) || (gMovData.State == (MOV_ST_REC | MOV_ST_ZOOM)))) {
				// reset uiMotionDetGo
				uiMotionDetGo = 0;
				bMotionDetRec = TRUE;
				// record video
				set_rec_status(true);
			}
		}
	} else {
		if (bMotionDetRec == TRUE) {
			uiMotionDetStop++;
			if (uiMotionDetStop >= 2) { // 1 sec
				uiMotionDetGo = 0;
			}
			if (uiMotionDetStop >= 20) { // 10 Sec
				uiMotionDetStop = 0;
				if (FlowMovie_GetRecCurrTime() >= 1) {
					if (gMovData.State == MOV_ST_REC || gMovData.State == (MOV_ST_REC | MOV_ST_ZOOM)) {
						set_rec_status(false);
						// update ui window icon
						update_icons();
						bMotionDetRec = FALSE;
					}
				}
			}
		}
	}
}

static void task_1sec_period_cb(lv_task_t* task)
{
//	update_icons();
	update_date_time();
}

static void task_motionDet_cb(lv_task_t* task)
{
	if (g_uiRecordIngMotionDet)
		UIFlowMovie_MotionDetect();
}

void UIFlowWndMovie_Initparam(void)
{
#if(PHOTO_MODE==ENABLE)
	// The same effect as Photo mode
	Ux_SendEvent(&CustomPhotoObjCtrl,	NVTEVT_EXE_WB,					1,	SysGetFlag(FL_WB));

	// The other settings
	Ux_SendEvent(&CustomPhotoObjCtrl,	NVTEVT_EXE_COLOR,				1,	MOVIE_COLOR_NORMAL);
#endif
	/* Video resolution setting must be set after other IQ settings */
	//Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_MOVIESIZE,			1,	SysGetFlag(FL_MOVIE_SIZE));

	/* Cyclic recording/record with mute or sound/DateImptint/Motion Detect */
	Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_CYCLIC_REC,			1,	SysGetFlag(FL_MOVIE_CYCLIC_REC));
	Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_MOTION_DET,			1,	SysGetFlag(FL_MOVIE_MOTION_DET));
	Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_MOVIE_DATE_IMPRINT,	1,	SysGetFlag(FL_MOVIE_DATEIMPRINT));
	Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_MOVIE_AUDIO,			1,	SysGetFlag(FL_MOVIE_AUDIO));
#if(PHOTO_MODE==ENABLE)
	Ux_SendEvent(&CustomPhotoObjCtrl,	NVTEVT_EXE_EV,					1,	SysGetFlag(FL_EV));
#endif
	Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_MOVIE_MCTF,			1,	SysGetFlag(FL_MovieMCTFIndex));
	//Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_MOVIE_WDR,			1,	SysGetFlag(FL_MOVIE_WDR));
	Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_MOVIE_HDR,			1,	SysGetFlag(FL_MOVIE_HDR));
	Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_MOVIE_DEFOG,			1,	SysGetFlag(FL_MOVIE_DEFOG));
	Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_GSENSOR,				1,	SysGetFlag(FL_GSENSOR));
	Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_MOVIE_SENSOR_ROTATE,	1,	SysGetFlag(FL_MOVIE_SENSOR_ROTATE));
	Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_MOVIE_IR_CUT,		1,	SysGetFlag(FL_MOVIE_IR_CUT));
	Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_MOVIE_PROTECT_AUTO,	1,	SysGetFlag(FL_MOVIE_URGENT_PROTECT_AUTO));
	Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_MOVIE_PROTECT_MANUAL,	1,	SysGetFlag(FL_MOVIE_URGENT_PROTECT_MANUAL));
	Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_MOVIE_LDWS,			1,	SysGetFlag(FL_MOVIE_LDWS));
	Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_MOVIE_FCW,			1,	SysGetFlag(FL_MOVIE_FCW));
	Ux_SendEvent(&UISetupObjCtrl,		NVTEVT_EXE_FREQ,				1,	SysGetFlag(FL_FREQUENCY));
#if (MOVIE_RSC == ENABLE)
	Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_MOVIE_RSC,			1,	MOVIE_RSC_ON);
#endif
#if MOVIE_DIS
	Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_MOVIEDIS,			1,	MOVIE_DIS_ON);
#endif
#if SHDR_FUNC
	//Ux_SendEvent(&CustomMovieObjCtrl,	NVTEVT_EXE_SHDR,				1,	MOVIE_HDR_ON);
#endif
}


static void set_indev_keypad_group(lv_obj_t* obj)
{
	if(gp == NULL){
		gp = lv_group_create();
		lv_group_add_obj(gp, obj);
	}

	lv_indev_t* indev = lv_plugin_find_indev_by_type(LV_INDEV_TYPE_KEYPAD);
	lv_indev_set_group(indev, gp);
}


static void UIFlowMovie_ScrOpen(lv_obj_t* obj)
{
	DBG_DUMP("%s\r\n", __func__);

	set_indev_keypad_group(obj);

//	if(gp == NULL){
//		gp = lv_group_create();
//		lv_group_add_obj(gp, obj);
//	}
//
//	lv_indev_t* indev = lv_plugin_find_indev_by_type(LV_INDEV_TYPE_KEYPAD);
//	lv_indev_set_group(indev, gp);

	/* init all flags */
	UIFlowWndMovie_Initparam();

	gMovData.State = MOV_ST_VIEW;

	/* init all icons */
	update_icons();

	/* update icons periodically*/
	if(task_1sec_period == NULL)
	{
		task_1sec_period = lv_task_create(task_1sec_period_cb, 1000, LV_TASK_PRIO_MID, NULL);
	}

	//#NT#2021/9/13#Philex Lin - begin
	g_uiRecordIngMotionDet = (SysGetFlag(FL_MOVIE_MOTION_DET) == MOVIE_MOTIONDET_ON) ? TRUE : FALSE;
	if(gUIMotionDetTimerID == NULL)
	{
		gUIMotionDetTimerID = lv_task_create(task_motionDet_cb, 500, LV_TASK_PRIO_MID, NULL);
	}
	//#NT#2021/9/13#Philex Lin - end


	//#NT#2018/08/10#KCHong -begin
	//#NT#Fixed Jira NA51000-1230
	if (gMovData.State == MOV_ST_VIEW || gMovData.State == MOV_ST_REC) {
		update_icons();
		if (ImageApp_MovieMulti_IsStreamRunning(_CFG_REC_ID_1)
#if(SENSOR_CAPS_COUNT >=2)
			|| ImageApp_MovieMulti_IsStreamRunning(_CFG_REC_ID_2)
#endif
		) {
			DBG_DUMP("gMovData.State = MOV_ST_REC;\r\n");
			gMovData.State = MOV_ST_REC;
			set_rec_status(true);
		} else {
			DBG_DUMP("gMovData.State = MOV_ST_VIEW;\r\n");
			gMovData.State = MOV_ST_VIEW;
			set_rec_status(false);
		}
	}
	//#NT#2018/08/10#KCHong -end

}

static void UIFlowMovie_ChildScrClose(lv_obj_t* obj, LV_USER_EVENT_NVTMSG_DATA* data)
{
	DBG_DUMP("%s\r\n", __func__);

	/* do nothing if current system mode is not matched */
	if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_MOVIE){
		DBG_WRN("current system mode is not movie\r\n");
		return;
	}

	/* once child scr closed, set keypad group again */
	set_indev_keypad_group(obj);

	switch (gMovData.State) {
	case MOV_ST_WARNING_MENU:
		if (data) {
			if ((data->paramNum>0) && (data->paramArray[0] ==NVTRET_ENTER_MENU))
			{
				/* Create Menu window */
				gMovData.State = MOV_ST_MENU;
				lv_plugin_scr_open(UIFlowMenuCommonItem, NULL);
				return;
			}
		}
		gMovData.State = MOV_ST_VIEW;
		break;

	case MOV_ST_MENU:
		#if _TODO
		// disable shutter2 sound
		uiSoundMask = Input_GetKeySoundMask(KEY_PRESS);
		uiSoundMask &= ~FLGKEY_ENTER;
		Input_SetKeySoundMask(KEY_PRESS, uiSoundMask);

		g_uiMaskKeyPress = MOVIE_KEY_PRESS_MASK;
		g_uiMaskKeyRelease = MOVIE_KEY_RELEASE_MASK;
		Input_SetKeyMask(KEY_PRESS, g_uiMaskKeyPress);
		Input_SetKeyMask(KEY_RELEASE, g_uiMaskKeyRelease);
		#endif
		update_icons();

		/* start timer again when exiting menu */
		if(task_1sec_period == NULL)
		{
			task_1sec_period = lv_task_create(task_1sec_period_cb, 1000, LV_TASK_PRIO_MID, NULL);
		}

		//#NT#2021/9/13#Philex Lin - begin
		g_uiRecordIngMotionDet = (SysGetFlag(FL_MOVIE_MOTION_DET) == MOVIE_MOTIONDET_ON) ? TRUE : FALSE;
		if(gUIMotionDetTimerID == NULL)
		{
			gUIMotionDetTimerID = lv_task_create(task_motionDet_cb, 500, LV_TASK_PRIO_MID, NULL);
		}
		//#NT#2021/9/13#Philex Lin - end

		gMovData.State = MOV_ST_VIEW;
		break;

	case MOV_ST_VIEW:

		if(data){

			if (data->paramNum > 0) {
				if ((data->paramNum == 2) && (data->paramArray[0] == NVTRET_WAITMOMENT)) {
					if (data->paramArray[1] == NVTRET_RESTART_REC) {
						#if defined(_KEY_METHOD_4KEY_)
						Ux_PostEvent(NVTEVT_KEY_SHUTTER2, 1, NVTEVT_KEY_PRESS);
						#else
						Ux_PostEvent(NVTEVT_KEY_SELECT, 1, NVTEVT_KEY_PRESS);
						#endif
					} else {
	//					FlowMovie_UpdateIcons(TRUE);
						update_icons();
					}
				}
			}
		}
		break;
	}

	//#NT#2018/08/10#KCHong -begin
	//#NT#Fixed Jira NA51000-1230
	if (gMovData.State == MOV_ST_VIEW || gMovData.State == MOV_ST_REC) {
		update_icons();
		if (ImageApp_MovieMulti_IsStreamRunning(_CFG_REC_ID_1)
#if(SENSOR_CAPS_COUNT >=2)
			|| ImageApp_MovieMulti_IsStreamRunning(_CFG_REC_ID_2)
#endif
		) {
			DBG_DUMP("gMovData.State = MOV_ST_REC;\r\n");
			gMovData.State = MOV_ST_REC;
			set_rec_status(true);
		} else {
			DBG_DUMP("gMovData.State = MOV_ST_VIEW;\r\n");
			gMovData.State = MOV_ST_VIEW;
			set_rec_status(false);
		}
	}
	//#NT#2018/08/10#KCHong -end
}

static void UIFlowMovie_ScrClose(lv_obj_t* obj)
{
	DBG_DUMP("%s\r\n", __func__);

	/* do nothing if current system mode is not matched */
	if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_MOVIE){
		DBG_WRN("current system mode is not movie\r\n");
		return;
	}

	if(task_1sec_period)
	{
		lv_task_del(task_1sec_period);
		task_1sec_period = NULL;
	}

	if(gUIMotionDetTimerID)
	{
		lv_task_del(gUIMotionDetTimerID);
		gUIMotionDetTimerID = NULL;
	}

}


static void UIFlowMovie_OnExeRecord(lv_obj_t* obj)
{
	if (System_GetState(SYS_STATE_POWERON) == SYSTEM_POWERON_SAFE) {

		if (System_GetState(SYS_STATE_CARD)  == CARD_REMOVED) {

			UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_PLEASE_INSERT_SD, warn_msgbox_auto_close_ms);
			return;
		}

	} else if (System_GetState(SYS_STATE_POWERON) == SYSTEM_POWERON_NORMAL) {

		if (GxStrg_GetDeviceCtrl(0, CARD_READONLY)) { // card lock

			UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_CARD_LOCKED, warn_msgbox_auto_close_ms);
			return;

		}

		if (System_GetState(SYS_STATE_CARD)  == CARD_REMOVED) {

			UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_PLEASE_INSERT_SD, warn_msgbox_auto_close_ms);
			return;
		}
	}


	toggle_rec_status();

	//#NT#2021/10/06#Philex Lin - begin
	// fix IVOT_N12079_CO-32
	if ((gMovData.State == MOV_ST_VIEW)&&(SysGetFlag(FL_MOVIE_MOTION_DET) == MOVIE_MOTIONDET_ON))
	{
		if (g_uiRecordIngMotionDet == TRUE) {
			g_uiRecordIngMotionDet = FALSE;
			UI_SetData(FL_MOVIE_MOTION_DET, MOVIE_MOTIONDET_OFF);
			update_motionDet();
		}
	}
	//#NT#2021/10/06#Philex Lin - end


}

static void UIFlowMovie_OnExePIM(lv_obj_t* obj)
{
	if(get_rec_status() && (SysGetFlag(FL_MOVIE_PIM) == MOVIE_PIM_ON))
	{
		set_pim_status(true);
		//show RawEnc icon
		update_pim();
		// Send RawEnc event
		Ux_SendEvent(&CustomMovieObjCtrl, NVTEVT_EXE_MOVIE_REC_RAWENC, 0);
	}
}

static void UIFlowMovie_OnKeyZoomIn(lv_obj_t* obj)
{

}

static void UIFlowMovie_OnKeyZoomOut(lv_obj_t* obj)
{

}

static void UIFlowMovie_OnKeyMode(lv_obj_t* obj)
{

}

static void UIFlowMovie_OnKeyEnter(lv_obj_t* obj)
{

}

static void UIFlowMovie_OnKeyMenu(lv_obj_t* obj)
{
UINT32  uiSoundMask;

	switch (gMovData.State) {
		case MOV_ST_VIEW:
		case MOV_ST_VIEW|MOV_ST_ZOOM:

		if(task_1sec_period)
		{
			lv_task_del(task_1sec_period);
			task_1sec_period = NULL;
		}

		if(gUIMotionDetTimerID)
		{
			lv_task_del(gUIMotionDetTimerID);
			gUIMotionDetTimerID = NULL;
		}

		// enable shutter2 sound (shutter2 as OK key in menu)
		uiSoundMask = Input_GetKeySoundMask(KEY_PRESS);
		uiSoundMask |= FLGKEY_ENTER;
		Input_SetKeySoundMask(KEY_PRESS, uiSoundMask);
		Input_SetKeyMask(KEY_RELEASE, FLGKEY_KEY_MASK_NULL);

		// Open common mix (Item + Option) menu
		lv_plugin_scr_open(UIFlowMenuCommonItem, NULL);
  		gMovData.State = MOV_ST_MENU;
		break;

		case MOV_ST_REC:
		case MOV_ST_REC|MOV_ST_ZOOM:
		if (SysGetFlag(FL_MOVIE_URGENT_PROTECT_MANUAL) == MOVIE_URGENT_PROTECT_MANUAL_ON) {
		#if (_BOARD_DRAM_SIZE_ > 0x04000000)
		if (GetMovieRecType_2p(UI_GetData(FL_MOVIE_SIZE)) == MOVIE_REC_TYPE_FRONT) {
			if(gMovie_Rec_Option.emr_on == _CFG_EMR_SET_CRASH){
				ImageApp_MovieMulti_SetCrash(_CFG_REC_ID_1, TRUE);
			}else{
				ImageApp_MovieMulti_TrigEMR(_CFG_REC_ID_1);
			}
		} else
		#endif
		{
			UINT32 i, mask, movie_rec_mask;

			movie_rec_mask = Movie_GetMovieRecMask();
			mask = 1;
			for (i = 0; i < SENSOR_CAPS_COUNT; i++) {
				if (movie_rec_mask & mask) {
					ImageApp_MovieMulti_SetCrash(_CFG_REC_ID_1 + i, TRUE);
				}
				mask <<= 1;
			}
		}
		}

		break;

	}
}

static void UIFlowMovie_OnKeyUp(lv_obj_t* obj)
{

}

static void UIFlowMovie_FULL(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
//	FlowMovie_StopRec();
	set_rec_status(false);
	gMovData.State = MOV_ST_WARNING_MENU;
	UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_CARD_FULL, warn_msgbox_auto_close_ms);
	update_icons();
	lv_obj_set_hidden(image_rec_ellipse_scr_uiflowmovie, true);
}

static void UIFlowMovie_OneSec(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	switch (gMovData.State) {
	case MOV_ST_REC:
	case MOV_ST_REC|MOV_ST_ZOOM:
		if (msg->paramNum) {
			update_rec_ellipse();
			//lv_obj_set_hidden(label_maxtime_scr_uiflowmovie,true);
			FlowMovie_SetRecCurrTime(msg->paramArray[0]);
			if(get_rec_status())
				update_rec_time();
		}
		if (get_pim_status())
		{
			set_pim_status(false);
			update_pim();
		}
		break;
	}
}

static void UIFlowMovie_REC_FINISH(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	#if (USE_DCF == ENABLE)
	UINT32  uiFolderId = 0, uiFileId = 0;
	#endif
	BOOL    CheckStorageErr;
	//UINT32  gUIAviRecMaxTime;

	switch (gMovData.State) {
	case MOV_ST_REC:
	case MOV_ST_REC|MOV_ST_ZOOM:

		//#NT#2016/09/20#Bob Huang -begin
		//#NT#Support HDMI Display with 3DNR Out
		//call stop rec first before starting to rec, keep rec mode
#if (_3DNROUT_FUNC == ENABLE)
		if (!gb3DNROut)
#endif
//#NT#2016/09/20#Bob Huang -end
		{
			gMovData.State = MOV_ST_VIEW;
		}

		//#NT#2012/10/23#Philex Lin - begin
		// enable auto power off/USB detect timer
		KeyScan_EnableMisc(TRUE);
		//#NT#2012/10/23#Philex Lin - end
		//lv_obj_set_hidden(label_rec_time_scr_uiflowmovie, true);
		update_icons();
		lv_obj_set_hidden(image_rec_ellipse_scr_uiflowmovie, true);

		//if (FlowMovie_ChkDrawStoreFullFolderFull() == FALSE)
		if (SysGetFlag(FL_MOVIE_CYCLIC_REC) == MOVIE_CYCLICREC_OFF) {
			CheckStorageErr = FlowMovie_IsStorageErr2(obj,TRUE);
		} else {
			CheckStorageErr = FlowMovie_IsStorageErr2(obj,FALSE);
		}
		if (CheckStorageErr == FALSE) {
			#if (USE_DCF == ENABLE)
			DCF_GetNextID(&uiFolderId, &uiFileId);
			SysSetFlag(FL_DCF_DIR_ID, uiFolderId);
			SysSetFlag(FL_DCF_FILE_ID, uiFileId);
			#endif
			//#NT#2016/03/07#KCHong -begin
			//#NT#Low power timelapse function
#if (TIMELAPSE_LPR_FUNCTION == ENABLE)
			if (UI_GetData(FL_MOVIE_TIMELAPSE_REC) < TL_LPR_TIME_MIN_PERIOD)
#endif
//#NT#2016/03/07#KCHong -end
			update_icons();
			#if _TODO
			Input_SetKeyMask(KEY_PRESS, MOVIE_KEY_PRESS_MASK);
			#endif
		}
		break;

	//The flow here may be only for APC3 stop record than lock file function.
	//To be careful that gMovData have changed in UIFlowMovie_Stop.
	case MOV_ST_VIEW:
		#if _TODO
		// Enable key if user pressed shutter2 key to stop recording.
		Input_SetKeyMask(KEY_PRESS, MOVIE_KEY_PRESS_MASK);
		#endif
		break;
	}

}


static void UIFlowMovie_WR_ERROR(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	if ((gMovData.State == MOV_ST_REC) || (gMovData.State == (MOV_ST_REC | MOV_ST_ZOOM))) {
//		FlowMovie_StopRec();
		set_rec_status(false);
		update_max_rec_time();
		lv_obj_set_hidden(image_rec_ellipse_scr_uiflowmovie, true);
		if (System_GetState(SYS_STATE_CARD)  == CARD_LOCKED) {
			gMovData.State = MOV_ST_WARNING_MENU;
			UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_IMG_ID_ICON_SD_LOCK, warn_msgbox_auto_close_ms);
		} else {
			gMovData.State = MOV_ST_WARNING_MENU;
			UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_MEMORYERROR, warn_msgbox_auto_close_ms);
		}
		update_icons();
	} else {
		// dummy
	}
}

static void UIFlowMovie_SLOW(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
		// stop recoding when slow card event is happening
		set_rec_status(false);
		update_icons();

		// restart recording again
		set_rec_status(true);
}

static void UIFlowMovie_EMR_COMPLETED(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
#if (__DBGLVL__ >= 6)
	UINT32  uiPathId = 0;

	if (paramNum)
	{
		uiPathId = paramArray[0];
	}

	DBG_IND("EMR(%d) completed!\r\n", uiPathId);
#endif

	if((msg->paramNum == 2)&&(msg->paramArray[1] == MOVIE_USER_CB_EVENT_EMR_FILE_COMPLETED))
	{
		DBG_IND("EMR(%d) completed!\r\n", uiPathId);
	}
	else if((msg->paramNum == 2)&&(msg->paramArray[1] == MOVIE_USER_CB_EVENT_CARSH_FILE_COMPLETED))
	{
		DBG_IND("Crash (%d) completed!\r\n", uiPathId);
	}
	else if((msg->paramNum == 2)&&(msg->paramArray[1] == MOVIE_USER_CB_EVENT_PREV_CARSH_FILE_COMPLETED))
	{
		DBG_IND("Crash (%d) completed!\r\n", uiPathId);
	}
}

static void UIFlowMovie_OZOOMSTEPCHG(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{

}

static void UIFlowMovie_DZOOMSTEPCHG(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{

}

static void UIFlowMovie_BATTERY(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{

}

static void UIFlowMovie_STORAGE_CHANGE(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{

}


static void UIFlowMovie_Key(lv_obj_t* obj, uint32_t key)
{

	switch(key)
	{
	case LV_USER_KEY_SHUTTER2:
	{
		UIFlowMovie_OnExeRecord(obj);
		break;
	}

	case LV_USER_KEY_NEXT:
	{
		UIFlowMovie_OnExePIM(obj);
		break;
	}

	//#NT#2021/09/10#Philex Lin--begin
	case LV_USER_KEY_ZOOMIN:
	{
		UIFlowMovie_OnKeyZoomIn(obj);
		break;
	}

	case LV_USER_KEY_ZOOMOUT:
	{
		UIFlowMovie_OnKeyZoomOut(obj);
		break;
	}

	case LV_USER_KEY_MENU:
	{
		UIFlowMovie_OnKeyMenu(obj);
		break;
	}

	case LV_USER_KEY_SELECT:
	{
		UIFlowMovie_OnKeyMenu(obj);
		break;
	}


	case LV_USER_KEY_MODE:
	{
		UIFlowMovie_OnKeyMode(obj);
		break;
	}

	case LV_KEY_ENTER:
	{
		UIFlowMovie_OnKeyEnter(obj);
		break;
	}
	//#NT#2021/09/10#Philex Lin--end

	case LV_KEY_UP:
	{
		UIFlowMovie_OnKeyUp(obj);
		break;
	}

	case LV_KEY_DOWN:
	{
		UIFlowMovie_OnExePIM(obj);
		break;
	}


	}

}

static void UIFlowMovie_NVTMSG(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{

	switch(msg->event)
	{
	case NVTEVT_CB_MOVIE_FULL:
		UIFlowMovie_FULL(obj,msg);
		break;

	case NVTEVT_CB_MOVIE_REC_ONE_SEC:
	{
		UIFlowMovie_OneSec(obj,msg);
		break;
	}

	//#NT#2021/09/10#Philex Lin--begin
	case NVTEVT_CB_MOVIE_REC_FINISH:
	{
		UIFlowMovie_REC_FINISH(obj,msg);
		break;
	}

	case NVTEVT_CB_MOVIE_WR_ERROR:
	{
		UIFlowMovie_WR_ERROR(obj,msg);
		break;
	}

	case NVTEVT_CB_MOVIE_SLOW:
	{
		UIFlowMovie_SLOW(obj,msg);
		break;
	}

	case NVTEVT_CB_EMR_COMPLETED:
	{
		UIFlowMovie_EMR_COMPLETED(obj,msg);
		break;
	}

	case NVTEVT_CB_OZOOMSTEPCHG:
	{
		UIFlowMovie_OZOOMSTEPCHG(obj,msg);
		break;
	}

	case NVTEVT_CB_DZOOMSTEPCHG:
	{
		UIFlowMovie_DZOOMSTEPCHG(obj,msg);
		break;
	}

	case NVTEVT_BATTERY:
	{
		UIFlowMovie_BATTERY(obj,msg);
		break;
	}

	case NVTEVT_STORAGE_CHANGE:
	{
		UIFlowMovie_STORAGE_CHANGE(obj,msg);
		break;
	}

	case NVTEVT_BACKGROUND_DONE:
	{
		NVTEVT message=msg->paramArray[ONDONE_PARAM_INDEX_CMD];
		switch (message) {
		case NVTEVT_BKW_STOPREC_PROCESS: {
		    update_icons();
		}
		break;
		default:
		break;
		}
	}

	//#NT#2021/09/10#Philex Lin--end

	default:
		break;

	}
}

void UIFlowMovieEventCallback(lv_obj_t* obj, lv_event_t event)
{

	switch(event)
	{
	case LV_PLUGIN_EVENT_SCR_OPEN:
		 UIFlowMovie_ScrOpen(obj);
		break;

	case LV_PLUGIN_EVENT_SCR_CLOSE:
		UIFlowMovie_ScrClose(obj);
		break;

	case LV_PLUGIN_EVENT_CHILD_SCR_CLOSE:
		UIFlowMovie_ChildScrClose(obj, (LV_USER_EVENT_NVTMSG_DATA*)lv_event_get_data());
		break;

	case LV_EVENT_CLICKED:
		UIFlowMovie_OnKeyMenu(obj);
		break;

	case LV_EVENT_KEY:
	{
		uint32_t* key = (uint32_t*)lv_event_get_data();

		/* handle key event */
		UIFlowMovie_Key(obj, *key);

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

	/* handle nvt event except key event */
	case LV_USER_EVENT_NVTMSG:
	{
		const LV_USER_EVENT_NVTMSG_DATA* msg = (const LV_USER_EVENT_NVTMSG_DATA*)lv_event_get_data();

		UIFlowMovie_NVTMSG(obj, msg);
		break;
	}
	}
}
