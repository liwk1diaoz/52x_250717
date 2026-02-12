#include "PrjInc.h"
#include "GxTime.h"
#include "GxStrg.h"
#include "UIFlowLVGL/UIFlowLVGL.h"
#include <kwrap/debug.h>
#include "GxVideoFile.h"
#include "FileDB.h"
#include "exif/Exif.h"
#include "exif/ExifDef.h"
#include "UIApp/ExifVendor.h"
#include "BinaryFormat.h"

#include "ImageApp/ImageApp_MoviePlay.h"
#include "UIWnd/LVGL_SPORTCAM/UIInfo/UIInfo.h"
#include "UIFlowLVGL/UIFlowWrnMsg/UIFlowWrnMsgAPI.h"
#include "UIFlowLVGL/UIFlowMenuCommonConfirm/UIFlowMenuCommonConfirmAPI.h"

#if (USE_DCF == ENABLE)
#include "DCF.h"
#endif

#define PLAY_KEY_PRESS_MASK        (FLGKEY_KEY_MASK_DEFAULT)
#define PLAY_KEY_RELEASE_MASK      FLGKEY_KEY_MASK_NULL//(FLGKEY_UP | FLGKEY_DOWN | FLGKEY_LEFT | FLGKEY_RIGHT)
#define PLAY_KEY_CONTINUE_MASK     FLGKEY_KEY_CONT_MASK_DEFAULT


UINT32         g_uiUIFlowWndPlayCurrenSpeed     = SMEDIAPLAY_SPEED_NORMAL;
UINT32         g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
BOOL           g_bUIFlowWndPlayNoImgWndOpened   = FALSE;
FST_FILE       gphUIFlowMovPlay_Filehdl         = NULL;
extern VControl CustomMoviePlayObjCtrl;
extern void PBView_DrawErrorView(void);

static lv_group_t* gp_btns = NULL;
extern uint16_t warn_msgbox_auto_close_ms;

#define PLB_BTN_FWD     (1)
#define PLB_BTN_CUR     (0)
#define PLB_BTN_BWD     (-1)

static void UIFlowPlay_UpdataFileName(BOOL bShow);

static void UIFlowPlayBtnEventCallback(lv_obj_t* obj, lv_event_t event);

static void UIFlowPlay_IconFileAttri(BOOL bShow)
{
    UINT32 uiLockStatus;
    if (bShow == false)
    {
        lv_obj_set_hidden(image_file_attri_scr_uiflowplay,true);
        return;
    }


    lv_obj_set_hidden(image_file_attri_scr_uiflowplay,false);

    PB_GetParam(PBPRMID_FILE_ATTR_LOCK, &uiLockStatus);
    if (uiLockStatus)
    {
        lv_plugin_img_set_src(image_file_attri_scr_uiflowplay, LV_PLUGIN_IMG_ID_ICON_LOCK);
    }
    else
    {
        lv_obj_set_hidden(image_file_attri_scr_uiflowplay,true);
    }
}

static void UIFlowPlay_IconImageSize(BOOL bShow)
{
	lv_obj_t* label_size = label_file_size_scr_uiflowplay;

	GXVIDEO_INFO MovieInfo = {0};
	static char item1_Buf[64];

	UINT32 uiFileFmt = 0;
	HD_VIDEO_FRAME hd_vdoframe_info = {0};
	EXIF_GETTAG exifTag;
	BOOL bIsLittleEndian = TRUE;
	UINT32 OriImgWidth, OriImgHeight;
	PB_GetParam(PBPRMID_CURR_FILEFMT, &uiFileFmt);
	PB_GetParam(PBPRMID_INFO_IMG, (UINT32 *)&hd_vdoframe_info);
	EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_BYTEORDER, &bIsLittleEndian, sizeof(bIsLittleEndian));
	exifTag.uiTagIfd = EXIF_IFD_EXIF;

	exifTag.uiTagId = TAG_ID_PIXEL_X_DIMENSION;
	if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag)) {
		OriImgWidth = Get32BitsData(exifTag.uiTagDataAddr, bIsLittleEndian);
	} else {
		OriImgWidth = (UINT32)hd_vdoframe_info.dim.w;
	}
	exifTag.uiTagId = TAG_ID_PIXEL_Y_DIMENSION;
	if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag)) {
		OriImgHeight = Get32BitsData(exifTag.uiTagDataAddr, bIsLittleEndian);
	} else {
		OriImgHeight = (UINT32)hd_vdoframe_info.dim.h;
	}

	if (bShow == FALSE) {
		lv_obj_set_hidden(label_file_size_scr_uiflowplay, true);
		return;
	}

	if (uiFileFmt & (PBFMT_MOVMJPG | PBFMT_AVI | PBFMT_MP4 | PBFMT_TS)) {

		char res[16] = {'\0'};
		char fps[16] = {'\0'};

		PB_GetParam(PBPRMID_INFO_VDO, (UINT32 *)&MovieInfo);

		snprintf(fps, sizeof(fps), "P%lu", MovieInfo.uiVidRate);

		switch (MovieInfo.uiVidWidth) {
		case 3840:
		case 2880: //2880X2160P24
			snprintf(res, sizeof(res), "UHD");
			break;
		case 2560: // 2560x1440P30
			snprintf(res, sizeof(res),"QHD");
			break;
		case 2304: // 2304x1296P30
			snprintf(res, sizeof(res),"3MHD");
			break;
		case 1920:
			snprintf(res, sizeof(res),"FHD");
			break;
		case 1280:
			snprintf(res, sizeof(res),"HD");
			break;
		case 848:
			snprintf(res, sizeof(res),"WVGA");
			break;
		case 640:
			snprintf(res, sizeof(res),"VGA");
			break;
		case 320:
			snprintf(res, sizeof(res),"QVGA");
			break;
		default:
			snprintf(res, sizeof(res), "%lux%lu", OriImgWidth, OriImgHeight);
		}

		snprintf(item1_Buf, sizeof(item1_Buf), "%s %s", res, fps);
	} else {
		snprintf(item1_Buf, sizeof(item1_Buf), "%lux%lu", OriImgWidth, OriImgHeight);
	}

	lv_label_set_text(label_size, item1_Buf);
	lv_obj_set_hidden(label_file_size_scr_uiflowplay, false);
}

static void update_ev(UINT32 value)
{
    static lv_plugin_res_id res[] =
    {
        LV_PLUGIN_IMG_ID_ICON_EV_M2P0,
        LV_PLUGIN_IMG_ID_ICON_EV_M1P6,
        LV_PLUGIN_IMG_ID_ICON_EV_M1P3,
        LV_PLUGIN_IMG_ID_ICON_EV_M1P0,
        LV_PLUGIN_IMG_ID_ICON_EV_M0P6,
        LV_PLUGIN_IMG_ID_ICON_EV_M0P3,
        LV_PLUGIN_IMG_ID_ICON_EV_P0P0,
        LV_PLUGIN_IMG_ID_ICON_EV_P0P3,
        LV_PLUGIN_IMG_ID_ICON_EV_P0P6,
        LV_PLUGIN_IMG_ID_ICON_EV_P1P0,
        LV_PLUGIN_IMG_ID_ICON_EV_P1P3,
        LV_PLUGIN_IMG_ID_ICON_EV_P1P6,
        LV_PLUGIN_IMG_ID_ICON_EV_P2P0
    };
    lv_plugin_img_set_src(image_file_ev_scr_uiflowplay, res[value]);
}
void UIFlowPlay_IconFileEV(BOOL bShow)
{
    UINT32           uiFlag;
    CHAR str[5] = {0};
    EXIF_GETTAG exifTag;
    BOOL bIsLittleEndian = true;
    UINT32 Data1, Data2;

    if (bShow == false)
    {
        lv_obj_set_hidden(image_file_ev_scr_uiflowplay,true);
        return;
    }

    EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_BYTEORDER, &bIsLittleEndian, sizeof(bIsLittleEndian));

    exifTag.uiTagIfd = EXIF_IFD_EXIF;
    exifTag.uiTagId = TAG_ID_EXPOSURE_BIAS_VALUE;
    if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag))
    {
        Data1 = Get32BitsData(exifTag.uiTagDataAddr, bIsLittleEndian);
        Data2 = Get32BitsData(exifTag.uiTagDataAddr + 4, bIsLittleEndian);
        ExposureBiasToStr((INT32)Data1, (INT32)Data2, (UINT8 *)str);
    }
    if (!strncmp(str, "-2.0", 4))
    {
        uiFlag = UIFlowPlay_EV_ICON_EV_M2P0;
    }
    else if (!strncmp(str, "-5/3", 4) || !strncmp(str, "-1.7", 4))
    {
        uiFlag = UIFlowPlay_EV_ICON_EV_M1P6;
    }
    else if (!strncmp(str, "-4/3", 4) || !strncmp(str, "-1.3", 4))
    {
        uiFlag = UIFlowPlay_EV_ICON_EV_M1P3;
    }
    else if (!strncmp(str, "-1.0", 4))
    {
        uiFlag = UIFlowPlay_EV_ICON_EV_M1P0;
    }
    else if (!strncmp(str, "-2/3", 4) || !strncmp(str, "-0.7", 4))
    {
        uiFlag = UIFlowPlay_EV_ICON_EV_M0P6;
    }
    else if (!strncmp(str, "-1/3", 4) || !strncmp(str, "-0.3", 4))
    {
        uiFlag = UIFlowPlay_EV_ICON_EV_M0P3;
    }
    else if (!strncmp(str, "+1/3", 4) || !strncmp(str, "+0.3", 4))
    {
        uiFlag = UIFlowPlay_EV_ICON_EV_P0P3;
    }
    else if (!strncmp(str, "+2/3", 4) || !strncmp(str, "+0.7", 4))
    {
        uiFlag = UIFlowPlay_EV_ICON_EV_P0P6;
    }
    else if (!strncmp(str, "+1.0", 4))
    {
        uiFlag = UIFlowPlay_EV_ICON_EV_P1P0;
    }
    else if (!strncmp(str, "+4/3", 4) || !strncmp(str, "+1.3", 4))
    {
        uiFlag = UIFlowPlay_EV_ICON_EV_P1P3;
    }
    else if (!strncmp(str, "+5/3", 4) || !strncmp(str, "+1.7", 4))
    {
        uiFlag = UIFlowPlay_EV_ICON_EV_P1P6;
    }
    else if (!strncmp(str, "+2.0", 4))
    {
        uiFlag = UIFlowPlay_EV_ICON_EV_P2P0;
    }
    else
    {
        uiFlag = UIFlowPlay_EV_ICON_EV_P0P0;
    }

    lv_obj_set_hidden(image_file_ev_scr_uiflowplay,false);
    update_ev(uiFlag);

}

static void UIFlowPlay_IconFileWB(BOOL bShow)
{
    lv_obj_set_hidden(image_file_wb_scr_uiflowplay, bShow == TRUE ? false : true);
}

static void UIFlowPlay_IconFileFlash(BOOL bShow)
{
    lv_obj_set_hidden(image_file_flash_scr_uiflowplay, bShow == TRUE? false: true);
}

CHAR    gchaFullName[60] = { 0 };
CHAR    gchaFileName[32] = { 0 };
static void UIFlowPlay_UpdataFileName(BOOL bShow)
{
    static char item1_Buf[32];
#if USE_FILEDB
//    UINT32 uiPBFileFmt = 0;
    //UINT32 rootFolderLength = 0;
#endif
    if (bShow == false)
    {
		lv_obj_set_hidden(label_file_name_scr_uiflowplay,true);
		return;
    }

    //show icon
    if(lv_obj_get_hidden(label_file_name_scr_uiflowplay))
        lv_obj_set_hidden(label_file_name_scr_uiflowplay,false);

#if USE_FILEDB

    PB_GetParam(PBPRMID_CURR_FILEPATH, (UINT32 *)&gchaFullName);

    //show icon
    if(strlen(gchaFullName)){

		UINT32 i, j, full_path_len, file_name_len;
		char *file_name_pos;

		// search last '\' in full file path
		full_path_len = strlen(gchaFullName);
		for (i = 0; i < full_path_len; i++)
		{
			if (gchaFullName[full_path_len - i] == '\\')
			{
				break;
			}
		}
		// search last '.' in full file path
		for (j = 0; j < full_path_len; j++)
		{
			if (gchaFullName[full_path_len - j] == '.')
			{
				break;
			}
		}

		file_name_pos = &gchaFullName[full_path_len - i + 1]; // string behind last '\'
		file_name_len = strlen(file_name_pos);
		if (file_name_len > j)
		{
			file_name_len -= j; // exclude extension name
		}
		if (file_name_len > 31)   // buffer limitation
		{
			file_name_len = 31;
		}
		strncpy(item1_Buf, file_name_pos, file_name_len);
		item1_Buf[file_name_len] = '\0';
    }

#else
    UINT32 uiDirID, uiFileID;
    PB_GetParam(PBPRMID_NAMEID_DIR, &uiDirID);
    PB_GetParam(PBPRMID_NAMEID_FILE, &uiFileID);
    snprintf(item1_Buf, 32, "%03ld-%04ld", uiDirID, uiFileID);

    DBG_DUMP("Play DCF File = %s\r\n", item1_Buf);
#endif

    lv_label_set_text_fmt(label_file_name_scr_uiflowplay, "%s",item1_Buf);
}

static void UIFlowPlay_UpdatePlaytime(BOOL bShow)
{
    if (bShow == false)
    {
		lv_obj_set_hidden(label_play_time_scr_uiflowplay,true);
		return;
    }

    if(lv_obj_get_hidden(label_play_time_scr_uiflowplay))
        lv_obj_set_hidden(label_play_time_scr_uiflowplay,false);

    if (System_GetState(SYS_STATE_CARD)  == CARD_REMOVED)
    {
        lv_obj_set_hidden(label_play_time_scr_uiflowplay,true);
    }
    else
    {
        UINT32 rec_sec = FlowPB_GetMovPlayTime();
        lv_label_set_text_fmt(label_play_time_scr_uiflowplay, "%02d:%02d:%02d", rec_sec / 3600, (rec_sec % 3600) / 60, (rec_sec % 3600) % 60);
    }
}
static void UIFlowPlay_UpdataBtnContainer(BOOL bShow)
{
    if (!bShow)
    {
        lv_obj_set_hidden(container_bt_bar_scr_uiflowplay,true);
    }
    else
    {
        lv_obj_set_hidden(container_bt_bar_scr_uiflowplay,false);
    }
}


static void UIFlowPlay_IconStorage(BOOL bShow)
{
	if (bShow == FALSE) {
		lv_obj_set_hidden(image_storage_scr_uiflowplay, true);
		return;
	}
	switch (System_GetState(SYS_STATE_CARD)) {
	case CARD_REMOVED:
		lv_plugin_img_set_src(image_storage_scr_uiflowplay, LV_PLUGIN_IMG_ID_ICON_INTERNAL_FLASH);
		break;
	default:
		lv_plugin_img_set_src(image_storage_scr_uiflowplay, LV_PLUGIN_IMG_ID_ICON_SD_CARD);
		break;
	}

	lv_obj_set_hidden(image_storage_scr_uiflowplay, false);
}

static void UIFlowPlay_IconDate(BOOL bShow)
{
	UINT32  creDateTime[6], modDateTime[6] = {0};
	static char item1_Buf[34];

	if (bShow == FALSE) {
		lv_obj_set_hidden(label_file_date_scr_uiflowplay, true);
		return;
	}

	FileSys_WaitFinish();
	PB_GetParam(PBPRMID_CURR_FILEPATH, (UINT32 *)&gchaFullName);

	if(strlen(gchaFullName)){
		if (FST_STA_OK == FileSys_GetDateTime(gchaFullName, creDateTime, modDateTime)) {
			modDateTime[0] %= 100;

			if (modDateTime[0] > 50) {
				modDateTime[0] = 50;
			}
			if (modDateTime[1] > 12) {
				modDateTime[0] = 12;
			}
			if (modDateTime[2] > 31) {
				modDateTime[0] = 31;
			}
			snprintf(item1_Buf, 20, "%02lu/%02lu/%02lu", modDateTime[0], modDateTime[1], modDateTime[2]);
			lv_label_set_text(label_file_date_scr_uiflowplay, item1_Buf);
			lv_obj_set_hidden(label_file_date_scr_uiflowplay, false);
		}
	}
}


static void UIFlowPlay_IconTime(BOOL bShow)
{
	UINT32  creDateTime[6], modDateTime[6] = {0};
	static char item1_Buf[34];

	if (bShow == FALSE) {

		lv_obj_set_hidden(label_file_time_scr_uiflowplay, true);
		return;
	}

	FileSys_WaitFinish();
	PB_GetParam(PBPRMID_CURR_FILEPATH, (UINT32 *)&gchaFullName);

	if(strlen(gchaFullName)){

		if (FST_STA_OK == FileSys_GetDateTime(gchaFullName, creDateTime, modDateTime)) {
			if (modDateTime[3] > 23) {
				modDateTime[3] = 0;
			}
			if (modDateTime[4] > 59) {
				modDateTime[4] = 0;
			}

			snprintf(item1_Buf, 20, "%02lu:%02lu", modDateTime[3], modDateTime[4]);
			lv_label_set_text(label_file_time_scr_uiflowplay, item1_Buf);
			lv_obj_set_hidden(label_file_time_scr_uiflowplay, false);
		}
	}
}

static void UIFlowPlay_IconImageQuality(BOOL bShow)
{
	if(bShow)
		lv_obj_set_hidden(image_quality_scr_uiflowplay, false);
	else
		lv_obj_set_hidden(image_quality_scr_uiflowplay, true);
}

static void UIFlowPlay_IconBattery(BOOL bShow)
{

	if(bShow)
		lv_obj_set_hidden(image_battery_scr_uiflowplay, false);
	else
		lv_obj_set_hidden(image_battery_scr_uiflowplay, true);
}

static void UIFlowPlay_IconSharpness(BOOL bShow)
{
	if(bShow)
		lv_obj_set_hidden(image_sharpness_scr_uiflowplay, false);
	else
		lv_obj_set_hidden(image_sharpness_scr_uiflowplay, true);
}

static void FlowPB_IconDrawDSCMode(BOOL bShow)
{
	UINT32 uiFlag;
	UINT32 uiFileFmt = 0;
	PB_GetParam(PBPRMID_CURR_FILEFMT, &uiFileFmt);

	if (bShow == FALSE) {

		lv_obj_set_hidden(image_mode_playback_scr_uiflowplay, true);
		return;
	}

	if (uiFileFmt & (PBFMT_MOVMJPG | PBFMT_AVI | PBFMT_MP4 | PBFMT_TS)) {
		uiFlag = LV_PLUGIN_IMG_ID_ICON_FILE_VIDEO;
	} else {
		uiFlag = LV_PLUGIN_IMG_ID_ICON_MODE_PLAYBACK;
	}

	lv_plugin_img_set_src(image_mode_playback_scr_uiflowplay, uiFlag);
	lv_obj_set_hidden(image_mode_playback_scr_uiflowplay, false);
}


static void update_playicons(UINT8 state)
{
	bool is_show;

	switch(state)
	{
	case PLB_ST_MAGNIFY:
		UIFlowPlay_UpdatePlaytime(false);
		UIFlowPlay_UpdataFileName(true);
		is_show = false;
		break;

	case PLB_ST_IDLE:
	case PLB_ST_THUMB:
		UIFlowPlay_UpdatePlaytime(false);
		UIFlowPlay_UpdataFileName(false);
		is_show = false;
		break;

	case PLB_ST_FULL:
	case PLB_ST_MENU:
		UIFlowPlay_UpdatePlaytime(false);
		UIFlowPlay_UpdataFileName(true);
		is_show = true;
		break;

	default: /* play state */
		UIFlowPlay_UpdatePlaytime(true);
		UIFlowPlay_UpdataFileName(false);
		is_show = false;
		break;
	}

	FlowPB_IconDrawDSCMode(is_show);
    UIFlowPlay_IconFileAttri(is_show);
    UIFlowPlay_UpdataBtnContainer(is_show);
    UIFlowPlay_IconBattery(is_show);
    UIFlowPlay_IconImageSize(is_show);
    UIFlowPlay_IconStorage(is_show);
    UIFlowPlay_IconDate(is_show);
    UIFlowPlay_IconTime(is_show);

	UINT32 uiCurrMode;
	PB_GetParam(PBPRMID_PLAYBACK_MODE, &uiCurrMode);

	if (uiCurrMode == PLAYMODE_PRIMARY) {
		UIFlowPlay_IconFileWB(is_show);
		UIFlowPlay_IconImageQuality(is_show);
		UIFlowPlay_IconSharpness(is_show);
		UIFlowPlay_IconFileFlash(is_show);
		UIFlowPlay_IconFileEV(is_show);
    }
	else{
		UIFlowPlay_IconFileWB(false);
		UIFlowPlay_IconImageQuality(false);
		UIFlowPlay_IconSharpness(false);
		UIFlowPlay_IconFileFlash(false);
		UIFlowPlay_IconFileEV(false);
	}
}

static void FlowPlay_CheckFileAndRecovery(lv_obj_t* obj)
{
    UINT32 uiStatus, uiCurrMode;

    uiStatus = PB_WaitCommandFinish(PB_WAIT_INFINITE);
    PB_GetParam(PBPRMID_PLAYBACK_MODE, &uiCurrMode);

    // Decode Error & Read Error
    if (uiStatus & (PB_STA_ERR_FILE | PB_STA_ERR_DECODE))
    {
        if (uiCurrMode == PLAYMODE_AVI || uiCurrMode == PLAYMODE_MOVMJPG)
        {
            UINT32	uiBuffAddr=0, uiBuffSize=0;
            CHAR	chaFullName[64] = { 0 };

            PB_GetParam(PBPRMID_DATABUF_ADDR, &uiBuffAddr);
            PB_GetParam(PBPRMID_DATABUF_SIZE, &uiBuffSize);
            PB_GetParam(PBPRMID_CURR_FILEPATH, (UINT32 *)&chaFullName);
            SMediaPlay_FileRecovery(chaFullName, uiBuffAddr, uiBuffSize);
            FileDB_Refresh(0);//2016/04/11
            DBG_IND("SMediaPlay_FileRecovery is skipped.\r\n");
            uiStatus = UIPlay_PlaySingle(PB_SINGLE_CURR);
            if (uiStatus == PB_STA_DONE)
            {
                return;
            }
        }
        PBView_DrawErrorView();
        UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_PICTUREERROR, warn_msgbox_auto_close_ms);
    }
}

static void UIFlowPlay_OpenFile(lv_obj_t* obj)
{
    DBG_DUMP("UIFlowPlay_OpenFile\r\n");
    UINT32 uiFileNum = 0;
    PLAY_SINGLE_OBJ FlowPlaySingleObj;
    UINT32  uiStatus;

    PB_WaitCommandFinish(PB_WAIT_INFINITE);  // Will this affect the OSD screen? By KS Hung on 20190805.
    //After playback ready, point to the last file
    PB_GetParam(PBPRMID_TOTAL_FILE_COUNT, &uiFileNum);
    PB_OpenSpecFileBySeq(uiFileNum, TRUE);
    FlowPlaySingleObj.PlayCommand = PB_SINGLE_CURR;
    FlowPlaySingleObj.PlayCommand |= PB_SINGLE_PRIMARY;
    FlowPlaySingleObj.JumpOffset  = 1;
    PB_PlaySingleMode(&FlowPlaySingleObj);
    uiStatus = PB_WaitCommandFinish(PB_WAIT_INFINITE);    // Will this affect the OSD screen? By KS Hung on 20190805.

    //disable video2 and enable video 1
    Display_ShowPreview();

    if (uiStatus & PB_STA_NOIMAGE)
    {
    	g_PlbData.State = PLB_ST_IDLE;
        update_playicons(g_PlbData.State);
        UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_NO_IMAGE, warn_msgbox_auto_close_ms);
        g_bUIFlowWndPlayNoImgWndOpened = TRUE;
    }
    else
    {
        g_PlbData.VideoPBSpeed = PLB_FWD_MOV_1x;
        g_PlbData.State = PLB_ST_FULL;
        FlowPlay_CheckFileAndRecovery(obj);

        /* init all icons */
        update_playicons(g_PlbData.State);
        
    }
    
	Ux_FlushEventByRange(NVTEVT_KEY_EVT_START, NVTEVT_KEY_EVT_END);

}

static void set_indev_keypad_group(lv_obj_t* obj)
{
    if(gp_btns == NULL)
    {
        gp_btns = lv_group_create();

        lv_group_add_obj(gp_btns, button_up_bg_scr_uiflowplay);
        lv_group_add_obj(gp_btns, button_left_bg_scr_uiflowplay);
        lv_group_add_obj(gp_btns, button_select_bg_scr_uiflowplay);
        lv_group_add_obj(gp_btns, button_right_bg_scr_uiflowplay);
        lv_group_add_obj(gp_btns, button_down_bg_scr_uiflowplay);

        lv_group_set_wrap(gp_btns, true);
        lv_group_focus_obj(button_select_bg_scr_uiflowplay);
    }

    lv_indev_t* indev = lv_plugin_find_indev_by_type(LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_group(indev, gp_btns);
}


static void UIFlowPlay_ScrOpen(lv_obj_t* obj)
{
    DBG_IND("UIFlowPlay_ScrOpen\r\n");

    set_indev_keypad_group(obj);

    /* open all files */
    UIFlowPlay_OpenFile(obj);
}

static void UIFlowPlay_ChildScrClose(lv_obj_t* obj, const void * data)
{
    UINT32 uiStatus;

	DBG_DUMP("%s\r\n", __func__);

	/* do nothing if current system mode is not matched */
	if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_PLAYBACK){
		DBG_WRN("system current mode is not playback\r\n");
		return;
	}

	/* do nothing if play screen is ready to be closed (menu mode change) */
	if (lv_plugin_scr_is_ready_to_be_closed(obj)) {
		g_PlbData.State = PLB_ST_FULL;
		return;
	}

	/* once child scr closed, set keypad group again */
	set_indev_keypad_group(obj);

    if(data){

    	LV_USER_EVENT_NVTMSG_DATA* msg = (LV_USER_EVENT_NVTMSG_DATA*)data;

        //Return from thumbnail, magnify or delete mode and play current image again.
        switch (msg->event) {
        case NVTRET_THUMBNAIL:
        case NVTRET_MAGNIFY:
        	g_PlbData.State = PLB_ST_FULL;
            UIPlay_PlaySingle(PB_SINGLE_CURR);
            FlowPlay_CheckFileAndRecovery(obj);
            break;
        case NVTRET_DELETE:
        case NVTRET_DELETEALL:
            uiStatus = PB_WaitCommandFinish(PB_WAIT_INFINITE);

            if (uiStatus & PB_STA_NOIMAGE)
            {
            	g_PlbData.State = PLB_ST_IDLE;
                if (!g_bUIFlowWndPlayNoImgWndOpened) {
                    UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_NO_IMAGE, warn_msgbox_auto_close_ms);
                    g_bUIFlowWndPlayNoImgWndOpened = TRUE;
                }
            } else {
            	g_PlbData.State = PLB_ST_FULL;
                UIPlay_PlaySingle(PB_SINGLE_CURR);
                FlowPlay_CheckFileAndRecovery(obj);
            }
            break;
        case NVTRET_ENTER_MENU:

        	DBG_DUMP("NVTRET_ENTER_MENU");
            SysSetFlag(FL_PROTECT, PROTECT_ONE);
            lv_plugin_scr_open(UIFlowMenuCommonItem, NULL);
            g_bUIFlowWndPlayNoImgWndOpened = FALSE;
            break;
        case NVTRET_WARNING:

        	DBG_DUMP("NVTRET_WARNING");
//            Ux_PostEvent(paramArray[1], 0);
            break;
        }
    } else {
        UIPlay_PlaySingle(PB_SINGLE_CURR);
        uiStatus = PB_WaitCommandFinish(PB_WAIT_INFINITE);

        if (uiStatus & PB_STA_NOIMAGE)
        {
            if (!g_bUIFlowWndPlayNoImgWndOpened) {
            	g_PlbData.State = PLB_ST_IDLE;
                UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_NO_IMAGE, warn_msgbox_auto_close_ms);
                g_bUIFlowWndPlayNoImgWndOpened = TRUE;
            } else {
                g_PlbData.State = PLB_ST_MENU;
                SysSetFlag(FL_PROTECT, PROTECT_ONE);
        		lv_plugin_scr_open(UIFlowMenuCommonItem, NULL);
            }
        } else {
        	g_PlbData.State = PLB_ST_FULL;
        }
    }

    update_playicons(g_PlbData.State);
    KeyScan_EnableMisc(TRUE);
}




static void UIFlowPlay_ScrClose(lv_obj_t* obj)
{
    DBG_DUMP("%s\r\n", __func__);

	/* do nothing if current system mode is not matched */
	if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_PLAYBACK){
		DBG_WRN("system current mode is not playback\r\n");
		return;
	}
}


///================================key event start==================================================================================

void UIFlowMoviePlay_SetSpeed(INT16 uiChangeSpeedLevel)
{
    switch (uiChangeSpeedLevel)
    {
    default:
    case PLB_FWD_MOV_1x:
    case PLB_BWD_MOV_1x:
        g_uiUIFlowWndPlayCurrenSpeed = SMEDIAPLAY_SPEED_NORMAL;
        break;

    case PLB_FWD_MOV_2x:
    case PLB_BWD_MOV_2x:
        g_uiUIFlowWndPlayCurrenSpeed = SMEDIAPLAY_SPEED_2X;
        break;

    case PLB_FWD_MOV_4x:
    case PLB_BWD_MOV_4x:
        g_uiUIFlowWndPlayCurrenSpeed = SMEDIAPLAY_SPEED_4X;
        break;

    case PLB_FWD_MOV_8x:
    case PLB_BWD_MOV_8x:
        g_uiUIFlowWndPlayCurrenSpeed = SMEDIAPLAY_SPEED_8X;
        break;

    }
}

void button_home_event_callback(lv_obj_t* obj, lv_event_t event)
{
	UIFlowPlayBtnEventCallback(obj, event);
}

void button_next_event_callback(lv_obj_t* obj, lv_event_t event)
{
	UIFlowPlayBtnEventCallback(obj, event);
}

void button_prev_event_callback(lv_obj_t* obj, lv_event_t event)
{
	UIFlowPlayBtnEventCallback(obj, event);
}

void button_del_event_callback(lv_obj_t* obj, lv_event_t event)
{
	UIFlowPlayBtnEventCallback(obj, event);
}

void button_sel_event_callback(lv_obj_t* obj, lv_event_t event)
{
	UIFlowPlayBtnEventCallback(obj, event);
}


static void UIFlowPlay_OnKeySelect(lv_obj_t* obj)
{
    DBG_DUMP("%s !!!\r\n", __func__);
    UINT32 Tabfocus = 0xFF;
    char   pFilePath[FULL_FILE_PATH_LEN] = {0};
    UINT32 uiPBFileFmt = PBFMT_MP4;
    UINT32 uiPBFileSize = 0;

    lv_obj_t* focused_obj = lv_group_get_focused(gp_btns);

    if(focused_obj == button_up_bg_scr_uiflowplay){

    	Tabfocus = (UINT32)PLB_BTN_SEL_UP;
    }
    else if(focused_obj == button_left_bg_scr_uiflowplay){

    	Tabfocus = (UINT32)PLB_BTN_SEL_LEFT;
    }
    else if(focused_obj == button_select_bg_scr_uiflowplay){

    	Tabfocus = (UINT32)PLB_BTN_SEL_SELECT;
    }
    else if(focused_obj == button_right_bg_scr_uiflowplay){

    	Tabfocus = (UINT32)PLB_BTN_SEL_RIGHT;
    }
    else if(focused_obj == button_down_bg_scr_uiflowplay){

    	Tabfocus = (UINT32)PLB_BTN_SEL_DOWN;
    }
    else{

    	return;
    }

    switch (g_PlbData.State)
    {
    case PLB_ST_FULL:
        switch (Tabfocus)
        {
        case PLB_BTN_SEL_SELECT:
            DBG_IND("PLB_BTN_SEL_SELECT\r\n");
#if 1//_TODO
            PB_GetParam(PBPRMID_CURR_FILEFMT, &uiPBFileFmt);
            if (uiPBFileFmt & (PBFMT_MOVMJPG | PBFMT_AVI | PBFMT_MP4 | PBFMT_TS))
            {
                UINT32 u32CurrPbStatus = 0;

                PB_GetParam(PBPRMID_PLAYBACK_STATUS, &u32CurrPbStatus);
                if (u32CurrPbStatus != PB_STA_DONE)
                {
                    return;
                }

                // Open Video File
                if (gphUIFlowMovPlay_Filehdl)
                {
                    FileSys_CloseFile(gphUIFlowMovPlay_Filehdl);
                    gphUIFlowMovPlay_Filehdl = NULL;
                }
                if (uiPBFileFmt & PBFMT_TS)
                {
                    PB_GetParam(PBPRMID_CURR_FILESIZE, &uiPBFileSize);
                    if (uiPBFileSize <= 0x10000)
                    {
                        DBG_DUMP("Wrong video file format!! \r\n");
                        break;
                    }
                }
                // Get Current index
                PB_GetParam(PBPRMID_CURR_FILEPATH, (UINT32 *)pFilePath);

                // Open Test Media File
                gphUIFlowMovPlay_Filehdl = FileSys_OpenFile(pFilePath, FST_OPEN_READ);

                if (!gphUIFlowMovPlay_Filehdl)
                {
                    DBG_DUMP("UIFlowWndPlay_OnKeySelect: Can't open Video file!\r\n");
                    break;
                }
                KeyScan_EnableMisc(FALSE);
#if 0

                UIFlowMoviePlay_SetSpeed(g_PlbData.VideoPBSpeed);
                g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
#endif
                update_playicons(PLB_ST_PLAY_MOV);
                Ux_SendEvent(0, NVTEVT_EXE_CLOSE, 0); //CustomPlayObjCmdMap

                //flush event first
                Ux_FlushEventByRange(NVTEVT_KEY_EVT_START, NVTEVT_KEY_EVT_END);

                //stop scan
#if 0
                //SxTimer_SetFuncActive(SX_TIMER_DET_STRG_ID, FALSE);
                SxTimer_SetFuncActive(SX_TIMER_DET_SYSTEM_BUSY_ID, FALSE);
#endif

#if 0
                ImageApp_MoviePlay_Open();
                ImageApp_MoviePlay_Start();
#else			// Wake up CustomMoviePlayObjCtrl obj.
                Ux_SetActiveApp(&CustomMoviePlayObjCtrl);
                Ux_SendEvent(0, NVTEVT_EXE_OPENPLAY, 1, (UINT32)gphUIFlowMovPlay_Filehdl);
                Ux_SendEvent(0, NVTEVT_EXE_STARTPLAY, 0);
#endif

                //set movie volumn
                Ux_SendEvent(&CustomMoviePlayObjCtrl, NVTEVT_EXE_MOVIEAUDPLAYVOLUME, 2, UI_GetData(FL_MovieAudioPlayIndex), 1);
                g_PlbData.State = PLB_ST_PLAY_MOV;
            }
            else
            {
                g_PlbData.State = PLB_ST_MAGNIFY;

                /* PLB_ST_MAGNIFY only shows file name */
                update_playicons(g_PlbData.State);
            }
#endif
            break;
        case PLB_BTN_SEL_LEFT:
            DBG_IND("PLB_BTN_SEL_LEFT\r\n");
            UIPlay_PlaySingle(PB_SINGLE_PREV);
            FlowPlay_CheckFileAndRecovery(obj);
            update_playicons(g_PlbData.State);
            break;
        case PLB_BTN_SEL_RIGHT:
            DBG_IND("PLB_BTN_SEL_RIGHT\r\n");
            UIPlay_PlaySingle(PB_SINGLE_NEXT);
            FlowPlay_CheckFileAndRecovery(obj);
            update_playicons(g_PlbData.State);
            break;
        case PLB_BTN_SEL_UP:
            DBG_IND("PLB_BTN_SEL_UP\r\n");
            UIFlowMenuCommonConfirmAPI_Open(IDM_DELETE_THIS);
            break;
        case PLB_BTN_SEL_DOWN:
            DBG_IND("PLB_BTN_SEL_DOWN\r\n");
            g_PlbData.State = PLB_ST_MENU;
            // Reset specific menu items
            SysSetFlag(FL_PROTECT, PROTECT_ONE);
            // Set Tab menu to Playback menu
            // Open common item menu
            lv_plugin_scr_open(UIFlowMenuCommonItem, NULL);
            break;
        default:
            break;
        }
        break;
    case PLB_ST_MAGNIFY:
        g_PlbData.State = PLB_ST_FULL;
        update_playicons(g_PlbData.State);
        break;
    case PLB_ST_PLAY_MOV:
    case PLB_ST_PAUSE_MOV:
#if 1//_TODO
        g_PlbData.State        = PLB_ST_FULL;
        g_PlbData.VideoPBSpeed = PLB_FWD_MOV_1x;

        // Close MoviePlay module.
        Ux_SendEvent(0, NVTEVT_EXE_CLOSEPLAY, 0);
        Ux_SetActiveApp(&CustomPlayObjCtrl);

        // Wakeup playback task and ImageApp_Play module.
        Ux_SendEvent(0, NVTEVT_EXE_OPEN, 0);
        KeyScan_EnableMisc(TRUE);
#if 0
        if (SxTimer_GetFuncActive(SX_TIMER_DET_SYSTEM_BUSY_ID) == 0)
        {
            SxTimer_SetFuncActive(SX_TIMER_DET_SYSTEM_BUSY_ID, TRUE);
        }
#endif

        if (gphUIFlowMovPlay_Filehdl)
        {
            FileSys_CloseFile(gphUIFlowMovPlay_Filehdl);
            gphUIFlowMovPlay_Filehdl = NULL;
        }
        PBView_DrawErrorView();

        // Play 1st video frame image
        UIPlay_PlaySingle(PB_SINGLE_CURR);
        update_playicons(g_PlbData.State);
#endif
        break;
    case PLB_ST_FWD_MOV:
    case PLB_ST_BWD_MOV:
        KeyScan_EnableMisc(FALSE);
#if _TODO
        // Set to Normal Play
        g_PlbData.VideoPBSpeed = PLB_FWD_MOV_1x;
        UIFlowMoviePlay_SetSpeed(g_PlbData.VideoPBSpeed);
        g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;

        // Start Play
        vdo_idx = AppDisp_MoviePlayView_GetDispIdx(); // get current display video index
        ImageApp_MoviePlay_FilePlay_UpdateSpeedDirect(g_uiUIFlowWndPlayCurrenSpeed, g_uiUIFlowWndPlayCurrenDirection, vdo_idx);
        g_PlbData.State = PLB_ST_PLAY_MOV;
#endif
        break;
    }
}

static void UIFlowPlay_OnKeyPrev(lv_obj_t* obj)
{

    INT32 vdo_idx = -1;

    DBG_DUMP("%s: g_PlbData.State = %d\r\n", __func__, g_PlbData.State);
    switch (g_PlbData.State)
    {
    case PLB_ST_FULL:
        DBG_DUMP("UIFlowPlay_OnKeyPrev PLB_ST_FULL\r\n");
        lv_group_focus_prev(gp_btns);

        break;
    case PLB_ST_PLAY_MOV:
    case PLB_ST_FWD_MOV:
    case PLB_ST_BWD_MOV:
        DBG_DUMP("OnKeyPrev g_PlbData.VideoPBSpeed %x, g_uiUIFlowWndPlayCurrenDirection %d\r\n", g_PlbData.VideoPBSpeed, g_uiUIFlowWndPlayCurrenDirection);
        if(g_PlbData.VideoPBSpeed > PLB_BWD_MOV_8x)
        {
            if (g_PlbData.VideoPBSpeed == PLB_FWD_MOV_1x)
            {
                g_PlbData.VideoPBSpeed -= 2;
            }
            else
            {
                g_PlbData.VideoPBSpeed--;
            }

            Ux_FlushEventByRange(NVTEVT_KEY_EVT_START,NVTEVT_KEY_EVT_END);

            if(g_PlbData.VideoPBSpeed > PLB_FWD_MOV_1x)
            {
                g_PlbData.State = PLB_ST_FWD_MOV;
                g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
            }
            else if(g_PlbData.VideoPBSpeed < PLB_FWD_MOV_1x)
            {
                g_PlbData.State = PLB_ST_BWD_MOV;
                g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_BACKWARD;
            }
            else   //uiCurrSpeedIndex == PLAYMOV_SPEED_FWD_1X
            {
                g_PlbData.State = PLB_ST_PLAY_MOV;
                g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
            }

            // Set speed and direction
            UIFlowMoviePlay_SetSpeed(g_PlbData.VideoPBSpeed);
            ImageApp_MoviePlay_FilePlay_UpdateSpeedDirect(g_uiUIFlowWndPlayCurrenSpeed, g_uiUIFlowWndPlayCurrenDirection, vdo_idx);
        }
        break;
    case PLB_ST_PAUSE_MOV:
        break;
    default:
        break;
    }

}

static void UIFlowPlay_OnKeyNext(lv_obj_t* obj)
{

    INT32 vdo_idx = -1;

    switch (g_PlbData.State)
    {
    case PLB_ST_FULL:
        lv_group_focus_next(gp_btns);
        break;
    case PLB_ST_PLAY_MOV:
    case PLB_ST_FWD_MOV:
    case PLB_ST_BWD_MOV:
        if(g_PlbData.VideoPBSpeed < PLB_FWD_MOV_8x)
        {
            g_PlbData.VideoPBSpeed ++;

            Ux_FlushEventByRange(NVTEVT_KEY_EVT_START,NVTEVT_KEY_EVT_END);

            if(g_PlbData.VideoPBSpeed > PLB_FWD_MOV_1x)
            {
                g_PlbData.State = PLB_ST_FWD_MOV;
                g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
            }
            else if(g_PlbData.VideoPBSpeed < PLB_FWD_MOV_1x)
            {
                if (g_PlbData.VideoPBSpeed == PLB_BWD_MOV_1x)
                {
                    g_PlbData.VideoPBSpeed = PLB_FWD_MOV_1x;
                    g_PlbData.State = PLB_ST_PLAY_MOV;
                    g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
                }
                else
                {
                    g_PlbData.State = PLB_ST_BWD_MOV;
                    g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_BACKWARD;
                }
            }
            else
            {
                g_PlbData.State = PLB_ST_PLAY_MOV;
                g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
            }

            UIFlowMoviePlay_SetSpeed(g_PlbData.VideoPBSpeed);
            ImageApp_MoviePlay_FilePlay_UpdateSpeedDirect(g_uiUIFlowWndPlayCurrenSpeed, g_uiUIFlowWndPlayCurrenDirection, vdo_idx);
        }
        break;
    case PLB_ST_PAUSE_MOV:
        break;
    default:
        break;
    }

}

static void UIFlowPlay_OnKeyZoomIn(lv_obj_t* obj)
{
#if 1//_TODO
    UINT32 vdo_idx = 0;
#endif

    DBG_DUMP("%s: g_PlbData.State = %d\r\n", __func__, g_PlbData.State);
    switch (g_PlbData.State)
    {
    case PLB_ST_PLAY_MOV:
    case PLB_ST_FWD_MOV:
    case PLB_ST_BWD_MOV:
        if(g_PlbData.VideoPBSpeed < PLB_FWD_MOV_8x)
        {
            g_PlbData.VideoPBSpeed ++;

            Ux_FlushEventByRange(NVTEVT_KEY_EVT_START,NVTEVT_KEY_EVT_END);
//            Input_SetKeyMask(KEY_PRESS, FLGKEY_KEY_MASK_NULL);

            if(g_PlbData.VideoPBSpeed > PLB_FWD_MOV_1x)
            {
                g_PlbData.State = PLB_ST_FWD_MOV;
                g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
            }
            else if(g_PlbData.VideoPBSpeed < PLB_FWD_MOV_1x)
            {
                if (g_PlbData.VideoPBSpeed == PLB_BWD_MOV_1x)
                {
                    g_PlbData.VideoPBSpeed = PLB_FWD_MOV_1x;
                    g_PlbData.State = PLB_ST_PLAY_MOV;
                    g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
                }
                else
                {
                    g_PlbData.State = PLB_ST_BWD_MOV;
                    g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_BACKWARD;
                }
            }
            else   //uiCurrSpeedIndex == PLAYMOV_SPEED_FWD_1X
            {
                g_PlbData.State = PLB_ST_PLAY_MOV;
                g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
            }

            // Set speed and direction
            UIFlowMoviePlay_SetSpeed(g_PlbData.VideoPBSpeed);
#if _TODO
            vdo_idx = AppDisp_MoviePlayView_GetDispIdx(); // get current display video index
#endif
            ImageApp_MoviePlay_FilePlay_UpdateSpeedDirect(g_uiUIFlowWndPlayCurrenSpeed, g_uiUIFlowWndPlayCurrenDirection, vdo_idx);

            //FlowPB_IconDrawMovSpeed();
//            Input_SetKeyMask(KEY_PRESS, FLGKEY_KEY_MASK_DEFAULT);
        }
        break;
    default:
        DBG_DUMP("%s: g_PlbData.State = %d can't play by speed or step !!!\r\n", __func__, g_PlbData.State);
        break;
    }
}

static void UIFlowPlay_OnKeyZoomOut(lv_obj_t* obj)
{
#if 1//_TODO
    UINT32 vdo_idx = 0;
#endif

    DBG_DUMP("%s: g_PlbData.State = %d\r\n", __func__, g_PlbData.State);
    switch (g_PlbData.State)
    {
    case PLB_ST_PLAY_MOV:
    case PLB_ST_FWD_MOV:
    case PLB_ST_BWD_MOV:
        DBG_DUMP("OnKeyPrev g_PlbData.VideoPBSpeed %x, g_uiUIFlowWndPlayCurrenDirection %d\r\n", g_PlbData.VideoPBSpeed, g_uiUIFlowWndPlayCurrenDirection);
        if(g_PlbData.VideoPBSpeed > PLB_BWD_MOV_8x)
        {
            if (g_PlbData.VideoPBSpeed == PLB_FWD_MOV_1x)
            {
                g_PlbData.VideoPBSpeed -= 2;
            }
            else
            {
                g_PlbData.VideoPBSpeed--;
            }

            Ux_FlushEventByRange(NVTEVT_KEY_EVT_START,NVTEVT_KEY_EVT_END);
//            Input_SetKeyMask(KEY_PRESS, FLGKEY_KEY_MASK_NULL);

            if(g_PlbData.VideoPBSpeed > PLB_FWD_MOV_1x)
            {
                g_PlbData.State = PLB_ST_FWD_MOV;
                g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
            }
            else if(g_PlbData.VideoPBSpeed < PLB_FWD_MOV_1x)
            {
                g_PlbData.State = PLB_ST_BWD_MOV;
                g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_BACKWARD;
            }
            else   //uiCurrSpeedIndex == PLAYMOV_SPEED_FWD_1X
            {
                g_PlbData.State = PLB_ST_PLAY_MOV;
                g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
            }

            // Set speed and direction
            UIFlowMoviePlay_SetSpeed(g_PlbData.VideoPBSpeed);
#if _TODO
            vdo_idx = AppDisp_MoviePlayView_GetDispIdx(); // get current display video index
#endif
            ImageApp_MoviePlay_FilePlay_UpdateSpeedDirect(g_uiUIFlowWndPlayCurrenSpeed, g_uiUIFlowWndPlayCurrenDirection, vdo_idx);


            //FlowPB_IconDrawMovSpeed();
//            Input_SetKeyMask(KEY_PRESS, FLGKEY_KEY_MASK_DEFAULT);
        }
        break;

    default:
        DBG_DUMP("%s: g_PlbData.State = %d can't play by speed or step !!!\r\n", __func__, g_PlbData.State);
        break;
    }

}


static void UIFlowPlay_OnKeyMode(lv_obj_t* obj)
{

}

static void UIFlowPlay_OnKeyShutter2(lv_obj_t* obj)
{
	switch (g_PlbData.State) {
	case PLB_ST_PLAY_MOV:
		Ux_SendEvent(0, NVTEVT_EXE_PAUSEPLAY, 0);
		g_PlbData.State = PLB_ST_PAUSE_MOV;
		KeyScan_EnableMisc(TRUE);
		break;
	case PLB_ST_PAUSE_MOV:

        KeyScan_EnableMisc(TRUE);
		Ux_SendEvent(&CustomMoviePlayObjCtrl, NVTEVT_EXE_MOVIEAUDPLAYVOLUME, 2, UI_GetData(FL_MovieAudioPlayIndex), 1);
		Ux_SendEvent(0, NVTEVT_EXE_RESUMEPLAY, 0);
		g_PlbData.State = PLB_ST_PLAY_MOV;

		break;

	case PLB_ST_FULL:
		g_PlbData.State = PLB_ST_THUMB;
		update_playicons(g_PlbData.State);
		lv_plugin_scr_open(UIFlowPlayThumb, NULL);
		break;

	default:
		break;
	}
}

static void UIFlowPlay_OnKeyMenu(lv_obj_t* obj)
{
    UINT32  uiSoundMask;

    switch (gMovData.State)
    {
    case MOV_ST_VIEW:
    case MOV_ST_VIEW|MOV_ST_ZOOM:

        // enable shutter2 sound (shutter2 as OK key in menu)
        uiSoundMask = Input_GetKeySoundMask(KEY_PRESS);
        uiSoundMask |= FLGKEY_ENTER;
        Input_SetKeySoundMask(KEY_PRESS, uiSoundMask);
//        Input_SetKeyMask(KEY_RELEASE, FLGKEY_KEY_MASK_NULL);

        // Open common mix (Item + Option) menu
        lv_plugin_scr_open(UIFlowMenuCommonItem, NULL);
        gMovData.State = MOV_ST_MENU;
        break;

    default:
        DBG_DUMP("%s: g_PlbData.State = %d can't open menu setting !!!\r\n", __func__, g_PlbData.State);
        break;

    }
}
///================================key event end==================================================================================


///================================callback event start===========================================================================
static void UIFlowPlay_CB_Finish(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
    switch (g_PlbData.State) {
    case PLB_ST_FULL:
        break;

    case PLB_ST_PLAY_MOV:
    case PLB_ST_PAUSE_MOV:
    case PLB_ST_FWD_MOV:
    case PLB_ST_BWD_MOV:

		g_PlbData.State        = PLB_ST_FULL;
		g_PlbData.VideoPBSpeed = PLB_FWD_MOV_1x;

		Ux_SendEvent(0, NVTEVT_EXE_CLOSEPLAY, 0);
		Ux_SetActiveApp(&CustomPlayObjCtrl);

		// Wakeup playback task and ImageApp_Play module.
		Ux_SendEvent(0, NVTEVT_EXE_OPEN, 0);

		KeyScan_EnableMisc(TRUE);

		if (gphUIFlowMovPlay_Filehdl)
		{
			FileSys_CloseFile(gphUIFlowMovPlay_Filehdl);
			gphUIFlowMovPlay_Filehdl = NULL;
		}

		// Play 1st video frame image
		UIPlay_PlaySingle(PB_SINGLE_CURR);

		update_playicons(g_PlbData.State);

		exam_msg("OnMovieFinish\r\n");

		break;

    default:
        break;
    }

}

static void UIFlowPlay_CB_OneSec(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
    UIFlowPlay_UpdatePlaytime(true);
}


static void UIFlowPlay_Battery(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{

}

static void UIFlowPlay_StorageChange(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{

}

static void UIFlowPlay_CB_Error(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
	UIFlowWrnMsgAPI_Open_StringID(LV_PLUGIN_STRING_ID_STRID_PICTUREERROR, warn_msgbox_auto_close_ms);
}
///================================callback event end============================================================================================



static void UIFlowPlay_Key(lv_obj_t* obj, uint32_t key)
{

    switch(key)
    {
    case LV_USER_KEY_SELECT:
    {
        UIFlowPlay_OnKeySelect(obj);
        break;
        break;
    }

    case LV_USER_KEY_SHUTTER2:
    {
    	UIFlowPlay_OnKeyShutter2(obj);
        break;
    }

    case LV_USER_KEY_UP:
    case LV_USER_KEY_PREV:
    {
        UIFlowPlay_OnKeyPrev(obj);
        break;
    }
    case LV_USER_KEY_DOWN:
    case LV_USER_KEY_NEXT:
    {
        UIFlowPlay_OnKeyNext(obj);
        break;
    }

    case LV_USER_KEY_ZOOMIN:
    {
        UIFlowPlay_OnKeyZoomIn(obj);
        break;
    }

    case LV_USER_KEY_ZOOMOUT:
    {
        UIFlowPlay_OnKeyZoomOut(obj);
        break;
    }

    case LV_USER_KEY_MENU:
    {
        UIFlowPlay_OnKeyMenu(obj);
        break;
    }

    case LV_USER_KEY_MODE:
    {
        UIFlowPlay_OnKeyMode(obj);
        break;
    }

    }

}

static void UIFlowPlay_NVTMSG(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{

    switch(msg->event)
    {
    case NVTEVT_CB_MOVIE_FINISH:
    {
        UIFlowPlay_CB_Finish(obj,msg);
        break;
    }

    case NVTEVT_CB_MOVIE_ONE_SEC:
    {
        UIFlowPlay_CB_OneSec(obj,msg);
        break;
    }

    case NVTEVT_CB_MOVIE_ERR:
    {
    	UIFlowPlay_CB_Error(obj,msg);
        break;
    }


    case NVTEVT_BATTERY:
    {
        UIFlowPlay_Battery(obj,msg);
        break;
    }

    case NVTEVT_STORAGE_CHANGE:
    {
        UIFlowPlay_StorageChange(obj,msg);
        break;
    }

    case NVTEVT_BACKGROUND_DONE:
    {
        NVTEVT message=msg->paramArray[ONDONE_PARAM_INDEX_CMD];
        switch (message)
        {
        case NVTEVT_BKW_STOPREC_PROCESS:
        {
            DBG_DUMP("NVTEVT_BKW_STOPREC_PROCESS:%d\r\n",message);
            break;
        }

        default:
            break;
        }

        break;
    }
    default:
        break;

    }
}

static void UIFlowPlayBtnEventCallback(lv_obj_t* obj, lv_event_t event)
{
    DBG_IND("%s:%d\r\n", __func__, event);

    switch(event)
    {
    case LV_EVENT_CLICKED:
    {
    	UIFlowPlay_OnKeySelect(obj);
    	break;
    }

    case LV_EVENT_KEY:
    {
        uint32_t* key = (uint32_t*)lv_event_get_data();

        /* handle key event */
        UIFlowPlay_Key(obj, *key);

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

void UIFlowPlayEventCallback(lv_obj_t* obj, lv_event_t event)
{
    DBG_IND("UIFlowPlayEventCallback:%d\r\n",event);

    switch(event)
    {
    case LV_PLUGIN_EVENT_SCR_OPEN:
        UIFlowPlay_ScrOpen(obj);
        break;

    case LV_PLUGIN_EVENT_SCR_CLOSE:
        UIFlowPlay_ScrClose(obj);
        break;
        
    case LV_PLUGIN_EVENT_CHILD_SCR_CLOSE:
        UIFlowPlay_ChildScrClose(obj, (LV_USER_EVENT_NVTMSG_DATA*)lv_event_get_data());
        break;

	/* handle nvt event except key event */
	case LV_USER_EVENT_NVTMSG:
	{
		const LV_USER_EVENT_NVTMSG_DATA* msg = (const LV_USER_EVENT_NVTMSG_DATA*)lv_event_get_data();

		UIFlowPlay_NVTMSG(obj, msg);
		break;
	}

    }
}

