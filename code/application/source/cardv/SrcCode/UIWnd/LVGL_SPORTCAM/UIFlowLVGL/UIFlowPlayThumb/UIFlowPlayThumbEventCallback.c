#include "UIFlowLVGL/UIFlowLVGL.h"
#include "PlaybackTsk.h"
#include "GxVideo.h"
#include "PrjInc.h"
#include "GxTime.h"
#include "GxStrg.h"
#include <kwrap/debug.h>
#include "NVTUserCommand.h"
//#include "UIFlowPlayFuncs.h"
//#include "UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMovie/UIFlowPlayFuncs.h"
//UIWnd\LVGL_SPORTCAM\UIFlowLVGL\UIFlowMovie

#define UINT16  unsigned short

void UIFlowPlayThumbEventCallback(lv_obj_t* obj, lv_event_t event);

static lv_group_t* gp = NULL;

//#NT#2012/8/13#Philex - begin
//Add Thumbnail play data structure
//typedef struct _THUMBNAIL_CONFIG {
//	UINT16  wThumbPerLine;          // Number of thumbnails per line
//	UINT16  wThumbPerColumn;        // Number of thumbnails per Column
//	UINT16  wFirstHGap;             // The Horizontal gap size to the first thumbnail
//	UINT16  wHGap;                  // The Horizontal gap size between thumbnails
//	UINT16  wLastHGap;              // The Horizontal gap size after the last thumbnail
//	UINT16  wFirstVGap;             // The Vertical gap size to the first thumbnail
//	UINT16  wVGap;                  // The Vertical gap between thumbnails
//	UINT16  wLastVGap;              // The Vertical gap size after the last thumbnail
//	UINT16  wFrColor;               // Color of select frame
//	UINT16  wBgColor;               // Color of background
//} THUMBNAIL_CONFIG;
#define PBX_SLIDE_EFFECT_NONE                       0

#define FLOWPB_THUMB_MODE_3_3           0
#define FLOWPB_THUMB_MODE_3_2           1
#define FLOWPB_THUMB_MODE_5_5           2
#define FLOWPB_THUMB_MODE_4_2           3
#define FLOWPB_THUMB_MODE_5_2           4
#define FLOWPB_THUMB_MODE_2_2           5
#define FLOWPB_THUMB_MODE               FLOWPB_THUMB_MODE_3_3

#if (FLOWPB_THUMB_MODE == FLOWPB_THUMB_MODE_3_3)
#define FLOWPB_THUMB_H_NUM2             3
#define FLOWPB_THUMB_V_NUM2             3
#elif (FLOWPB_THUMB_MODE == FLOWPB_THUMB_MODE_3_2)
#define FLOWPB_THUMB_H_NUM2             3
#define FLOWPB_THUMB_V_NUM2             2
#elif (FLOWPB_THUMB_MODE == FLOWPB_THUMB_MODE_5_5)
#define FLOWPB_THUMB_H_NUM2             5
#define FLOWPB_THUMB_V_NUM2             5
#elif (FLOWPB_THUMB_MODE == FLOWPB_THUMB_MODE_4_2)
#define FLOWPB_THUMB_H_NUM2             4
#define FLOWPB_THUMB_V_NUM2             2
#elif (FLOWPB_THUMB_MODE == FLOWPB_THUMB_MODE_5_2)
#define FLOWPB_THUMB_H_NUM2             5
#define FLOWPB_THUMB_V_NUM2             2
#elif (FLOWPB_THUMB_MODE == FLOWPB_THUMB_MODE_2_2)
#define FLOWPB_THUMB_H_NUM2             2
#define FLOWPB_THUMB_V_NUM2             2
#endif
//#NT#2012/8/13#Philex - end

//#NT#2012/8/13#Philex - begin
//Add Thumbnail play data structure
static PLAY_BROWSER_OBJ     FlowPBBrowserObj;
PURECT   g_pFlowPBThumbRect;
URECT    g_FlowPBThumbRect02[FLOWPB_THUMB_H_NUM2 * FLOWPB_THUMB_V_NUM2];
//#NT#2012/8/13#Philex - end

#if PLAY_THUMB_AND_MOVIE // play thumbnail and movie together
#include "UIApp/Play/UIAppMoviePlay.h"
#define FULL_FILE_PATH_LEN      64
extern FST_FILE       gphUIFlowMovPlay_Filehdl;
#endif

static void UIFlowPBThumb_UpdataThumbItem(BOOL bShow);


static lv_obj_t** btn_thumb_items[] = {
		NULL,
		&button_thumb_item1_scr_uiflowplaythumb,
		&button_thumb_item2_scr_uiflowplaythumb,
		&button_thumb_item3_scr_uiflowplaythumb,
		&button_thumb_item4_scr_uiflowplaythumb,
		&button_thumb_item5_scr_uiflowplaythumb,
		&button_thumb_item6_scr_uiflowplaythumb,
		&button_thumb_item7_scr_uiflowplaythumb,
		&button_thumb_item8_scr_uiflowplaythumb,
		&button_thumb_item9_scr_uiflowplaythumb,

};

static const uint8_t btn_thumb_item_num = sizeof(btn_thumb_items)/sizeof(lv_obj_t**);

static lv_obj_t* get_btn_thumb_items(uint8_t idx)
{
	if(idx >= btn_thumb_item_num)
	{
		return NULL;
	}

	if(btn_thumb_items[idx])
		return *btn_thumb_items[idx];
	else
		return NULL;
}


static void UIFlowPBThumb_ShowItems(void)
{
	UINT32 uiBrowseIdx=1;
	PB_GetParam(PBPRMID_THUMB_CURR_IDX, &uiBrowseIdx);

	lv_obj_t* btn = get_btn_thumb_items(uiBrowseIdx);

	if(btn){
		lv_obj_set_hidden(btn, false);
        _lv_obj_set_style_local_opa(btn, LV_OBJ_PART_MAIN, LV_STYLE_BORDER_OPA, LV_OPA_COVER);
	}
}

static void UIFlowPBThumb_HiddenAllWidgets(void)
{

	for(uint8_t idx=0 ; idx<btn_thumb_item_num ; idx++)
	{
		lv_obj_t* btn = get_btn_thumb_items(idx);

		/* ************************************************************************
		 * images are button childs and will be invisible if parent(button) is hidden,
		 ****************************************************************************/
		if(btn){
			lv_obj_set_hidden(btn, true);
			_lv_obj_set_style_local_opa(btn, LV_OBJ_PART_MAIN, LV_STYLE_BORDER_OPA, LV_OPA_TRANSP);
		}
	}

    lv_obj_set_hidden(label_page_info_scr_uiflowplaythumb,true);
    lv_obj_set_hidden(label_file_name_scr_uiflowplaythumb,true);

}

static void UIFlowPBThumb_ShowItemsSubWidgets(void)
{
	UINT32  i, loopcnts=0, CurrFileFormat, uiParamVal;
	UINT16  *pCurrFileFormat;   // Hideo@0503: variable type should be UINT16

	UIFlowPBThumb_ShowItems();

	UIFlowPBThumb_UpdataThumbItem(true);
	PB_GetParam(PBPRMID_THUMB_FMT_ARRAY, &uiParamVal);
	pCurrFileFormat = (UINT16 *)uiParamVal;

	PB_GetParam(PBPRMID_THUMB_CURR_NUM, &loopcnts);
	for (i = 0; i < loopcnts; i++) {
		CurrFileFormat  = *pCurrFileFormat++;

		lv_obj_t* btn = get_btn_thumb_items(i + 1);

		if(btn == NULL) continue;

		lv_obj_set_hidden(btn, false);

		/* *********************************************************
		 * IMPORTANT!!
		 *
		 * child order is determined by builder
		 * lv_plugin_find_child_by_type return the youngest child start from start child param
		 * *********************************************************/
		lv_obj_t* img_filefmt = lv_plugin_find_child_by_type(btn, NULL, lv_plugin_img_type_name());
		lv_obj_t* img_lock = lv_plugin_find_child_by_type(btn, img_filefmt, lv_plugin_img_type_name());

		if(img_lock){

			if (CurrFileFormat & PBFMT_READONLY) {
				lv_obj_set_hidden(img_lock, false);
			}
			else{
				lv_obj_set_hidden(img_lock, true);
			}
		}

		if(img_filefmt){

			if (CurrFileFormat & (PBFMT_MOVMJPG | PBFMT_AVI | PBFMT_MP4 | PBFMT_TS)) {
				lv_obj_set_hidden(img_filefmt, false);
			}
			else{
				lv_obj_set_hidden(img_filefmt, true);
			}
		}
	}
}

static void UIFlowPBThumb_UpdataThumbItem(BOOL bShow)
{
	static char item1_Buf[32];
	UINT32  *pThumbSeqID;
	UINT32 uiFileNum = 0;
	UINT32 uiBrowseIdx=0, uiParamVal;
	if (bShow == FALSE) {
        lv_obj_set_hidden(label_page_info_scr_uiflowplaythumb,true);
		return;
	}
	PB_GetParam(PBPRMID_THUMB_SEQ_ARRAY, &uiParamVal);
	pThumbSeqID = (UINT32 *)uiParamVal;
	PB_GetParam(PBPRMID_TOTAL_FILE_COUNT, &uiFileNum);
	PB_GetParam(PBPRMID_THUMB_CURR_IDX, &uiBrowseIdx);
	snprintf(item1_Buf, 32, "%lu/%lu", *(pThumbSeqID + uiBrowseIdx - 1), uiFileNum);

    lv_obj_set_hidden(label_page_info_scr_uiflowplaythumb,false);
    lv_label_set_text_fmt(label_page_info_scr_uiflowplaythumb, "%s",item1_Buf);
}


#if 1
static void UIFlowWndPlayThumb_NaviKey(UINT32 key)
{
	BROWSE_NAVI_INFO BrowseNavi;
    
	UIFlowPBThumb_HiddenAllWidgets();
	SxTimer_SetFuncActive(SX_TIMER_DET_KEY_ID, FALSE);
	BrowseNavi.NaviKey = key;
	BrowseNavi.HorNums = FLOWPB_THUMB_H_NUM2;
	BrowseNavi.VerNums = FLOWPB_THUMB_V_NUM2;

	BrowseThumbNaviKey(&BrowseNavi);

	UIFlowPBThumb_ShowItemsSubWidgets();
	SxTimer_SetFuncActive(SX_TIMER_DET_KEY_ID, TRUE);
}

//#NT#2012/8/13#Philex - begin
// add Thumbnail play functions
static BOOL UIFlowThumb_Config(THUMBNAIL_CONFIG *pThumbCfg)
{
	UINT32  i;
	UINT32  uiDispW, uiDispH;
	UINT16  wThumbWTotal, wThumbHTotal;
	UINT16  wThumbW, wThumbH;
	ISIZE   DevSize = {0};;

	DevSize = GxVideo_GetDeviceSize(DOUT1);

	// don't need to change w, h for rotation on 57x platform
	if (0) {//(SysVideo_GetDirbyID(DOUT1) == HD_VIDEO_DIR_ROTATE_90 || SysVideo_GetDirbyID(DOUT1) == HD_VIDEO_DIR_ROTATE_270) {
		uiDispW = DevSize.h;
		uiDispH = DevSize.w;
	} else {
		uiDispW = DevSize.w;
		uiDispH = DevSize.h;
	}

	//DBG_DUMP("%s: Display w = %d, h = %d\r\n", __func__, uiDispW, uiDispH);

	// support maximum 5 x 5 thumbnails
	if (pThumbCfg->wThumbPerLine > 5 || pThumbCfg->wThumbPerColumn > 5) {
		return (FALSE);
	}

	// at least we need 4 pixels for thumbnail frame
	if (pThumbCfg->wFirstHGap < 4) {
		pThumbCfg->wFirstHGap = 4;
	}
	if (pThumbCfg->wHGap < 4) {
		pThumbCfg->wHGap = 4;
	}
	if (pThumbCfg->wLastHGap < 4) {
		pThumbCfg->wLastHGap = 4;
	}
	if (pThumbCfg->wFirstVGap < 4) {
		pThumbCfg->wFirstVGap = 4;
	}
	if (pThumbCfg->wVGap < 4) {
		pThumbCfg->wVGap = 4;
	}
	if (pThumbCfg->wLastVGap < 4) {
		pThumbCfg->wLastVGap = 4;
	}

	wThumbWTotal = pThumbCfg->wFirstHGap + pThumbCfg->wLastHGap +
				   pThumbCfg->wHGap * (pThumbCfg->wThumbPerLine - 1);

	if (wThumbWTotal >= uiDispW) {
		DBG_ERR("Thumb setting error! wThumbWTotal=%d > uiDispW=%d\r\n", wThumbWTotal, uiDispW);
		return (FALSE);
	}

	wThumbHTotal = pThumbCfg->wFirstVGap + pThumbCfg->wLastVGap +
				   pThumbCfg->wVGap * (pThumbCfg->wThumbPerColumn - 1);

	if (wThumbHTotal >= uiDispH) {
		DBG_ERR("Thumb setting error! wThumbWTotal=%d > uiDispW=%d\r\n", wThumbWTotal, uiDispW);
		return (FALSE);
	}

	wThumbW = (uiDispW - wThumbWTotal) / pThumbCfg->wThumbPerLine;
	wThumbH = (uiDispH - wThumbHTotal) / pThumbCfg->wThumbPerColumn;

	for (i = 0; i < (UINT32)(pThumbCfg->wThumbPerLine * pThumbCfg->wThumbPerColumn); i++) {
		g_FlowPBThumbRect02[i].x = pThumbCfg->wFirstHGap +
								   (i % pThumbCfg->wThumbPerLine) * (wThumbW + pThumbCfg->wHGap);
		g_FlowPBThumbRect02[i].y = pThumbCfg->wFirstVGap +
								   (i / pThumbCfg->wThumbPerLine) * (wThumbH + pThumbCfg->wVGap);
		g_FlowPBThumbRect02[i].w = wThumbW;
		g_FlowPBThumbRect02[i].h = wThumbH;
        
		DBG_DUMP("ThumbRect[%d]: x %d, y %d, w %d, h %d\r\n", i, g_FlowPBThumbRect02[i].x, g_FlowPBThumbRect02[i].y, g_FlowPBThumbRect02[i].w, g_FlowPBThumbRect02[i].h);
	}

	FlowPBBrowserObj.HorNums = pThumbCfg->wThumbPerLine;
	FlowPBBrowserObj.VerNums = pThumbCfg->wThumbPerColumn;
	//uiThumbFrameColor = pThumbCfg->wFrColor;

	return (TRUE);
}

PURECT UIFlowThumb_Display(void)
{
	THUMBNAIL_CONFIG ThumbCfg;

	if (KeyScan_GetPlugDev() == PLUG_TV) {
		//#For LCD panel with 16:9 ratio (but 4:3 pixel resolution)
		if (SysGetFlag(FL_TV_MODE) == TV_MODE_NTSC) {
			ThumbCfg.wFirstHGap         = 22 * (640) / (320);
			ThumbCfg.wHGap              = 22 * (640) / (320);
			ThumbCfg.wLastHGap          = 24 * (640) / (320);
			ThumbCfg.wFirstVGap         = 6 * (448) / (240);
			ThumbCfg.wVGap              = 6 * (448) / (240);
			ThumbCfg.wLastVGap          = 30 * (448) / (240);
		} else {
			ThumbCfg.wFirstHGap         = 22 * (640) / (320);
			ThumbCfg.wHGap              = 22 * (640) / (320);
			ThumbCfg.wLastHGap          = 22 * (640) / (320);
			ThumbCfg.wFirstVGap         = 7 * (528) / (240);
			ThumbCfg.wVGap              = 6 * (528) / (240);
			ThumbCfg.wLastVGap          = 30 * (528) / (240);
		}
	} else {
		if (KeyScan_GetPlugDev() == PLUG_HDMI) {
			//HDMI
			ThumbCfg.wFirstHGap         = 20 * (1920) / (320);
			ThumbCfg.wHGap              = 22 * (1920) / (320);
			ThumbCfg.wLastHGap          = 20 * (1920) / (320);
			ThumbCfg.wFirstVGap         = 6 * (1080) / (240);
			ThumbCfg.wVGap              = 6 * (1080) / (240);
			ThumbCfg.wLastVGap          = 30 * (1080) / (240);
		} else {
			//LCD
#if (FLOWPB_THUMB_MODE == FLOWPB_THUMB_MODE_3_3)
			ThumbCfg.wFirstHGap         = 18;//16;
			ThumbCfg.wHGap              = 17;//16;
			ThumbCfg.wLastHGap          = 18;//32;
			ThumbCfg.wFirstVGap         = 6;
			ThumbCfg.wVGap              = 6;
			ThumbCfg.wLastVGap          = 30;
#elif (FLOWPB_THUMB_MODE == FLOWPB_THUMB_MODE_3_2)
			ThumbCfg.wFirstHGap         = 18;
			ThumbCfg.wHGap              = 16;
			ThumbCfg.wLastHGap          = 18;
			ThumbCfg.wFirstVGap         = 50;
			ThumbCfg.wVGap              = 12;
			ThumbCfg.wLastVGap          = 50;
#elif (FLOWPB_THUMB_MODE == FLOWPB_THUMB_MODE_5_5)
			//Todo: fine tune 5x5 location
			ThumbCfg.wFirstHGap         = 18;
			ThumbCfg.wHGap              = 16;
			ThumbCfg.wLastHGap          = 18;
			ThumbCfg.wFirstVGap         = 6;
			ThumbCfg.wVGap              = 6;
			ThumbCfg.wLastVGap          = 30;
#elif (FLOWPB_THUMB_MODE == FLOWPB_THUMB_MODE_4_2)
			ThumbCfg.wFirstHGap 		= 88;
			ThumbCfg.wHGap				= 48;
			ThumbCfg.wLastHGap			= 88;
			ThumbCfg.wFirstVGap 		= 24;
			ThumbCfg.wVGap				= 20;
			ThumbCfg.wLastVGap			= 24;
#elif (FLOWPB_THUMB_MODE == FLOWPB_THUMB_MODE_5_2)
			ThumbCfg.wFirstHGap 		= 16;
			ThumbCfg.wHGap				= 12;
			ThumbCfg.wLastHGap			= 16;
			ThumbCfg.wFirstVGap 		= 16;
			ThumbCfg.wVGap				= 16;
			ThumbCfg.wLastVGap			= 16;
#elif (FLOWPB_THUMB_MODE == FLOWPB_THUMB_MODE_2_2)
			ThumbCfg.wFirstHGap 		= 88;
			ThumbCfg.wHGap				= 28;
			ThumbCfg.wLastHGap			= 76 + 640;
			ThumbCfg.wFirstVGap 		= 22;
			ThumbCfg.wVGap				= 22;
			ThumbCfg.wLastVGap			= 24;
#endif
		}
	}

	ThumbCfg.wThumbPerLine          = FLOWPB_THUMB_H_NUM2;
	ThumbCfg.wThumbPerColumn        = FLOWPB_THUMB_V_NUM2;
	//ThumbCfg.wFrColor               = SHOWOSD_COLOR_WHITE;//SHOWOSD_COLOR_RED;
	UIFlowThumb_Config(&ThumbCfg);
//#NT#2012/8/13#Philex - begin
//mark code
#if 0
	FlowPB_ShowThumbBG();
	FlowPBBrowserObj.BrowserCommand = PB_BROWSER_CURR;
	FlowPBBrowserObj.MoveImgMode    = SLIDE_EFFECT_NONE;
	FlowPBBrowserObj.JumpOffset     = 0;
	PB_PlayBrowserMode(&FlowPBBrowserObj);
	g_pFlowPBThumbRect = &g_FlowPBThumbRect02[0];
	PB_SetThumbRect(g_pFlowPBThumbRect);
#endif
//#NT#2012/8/13#Philex - end
	return &g_FlowPBThumbRect02[0];
}
#endif


static void UIFlowPlayThumb_BrowseFile(void)
{
    PURECT          pThumbRect;
    PLAY_BROWSER_OBJ     FlowPBBrowserObj = {0};
    UINT32 FileNumsInDir = 0, NumsPerPage;
    UINT8  HorNums, VerNums;
    UINT32 uiStatus = 0;//E_OK;

    UIFlowPBThumb_HiddenAllWidgets();
    pThumbRect = UIFlowThumb_Display();
    HorNums = FLOWPB_THUMB_H_NUM2;
    VerNums = FLOWPB_THUMB_V_NUM2;
    PB_SetParam(PBPRMID_THUMB_LAYOUT_ARRAY, (UINT32)pThumbRect);

    NumsPerPage = VerNums * VerNums;
    PB_GetParam(PBPRMID_TOTAL_FILE_COUNT, &FileNumsInDir);
    FlowPBBrowserObj.BrowserCommand = PB_BROWSER_CURR;
    FlowPBBrowserObj.HorNums        = HorNums;
    FlowPBBrowserObj.VerNums        = VerNums;
    FlowPBBrowserObj.slideEffectFunc = PBX_SLIDE_EFFECT_NONE;
    if (FileNumsInDir <= NumsPerPage) { // means only one page
        FlowPBBrowserObj.bReDecodeImages = FALSE;
    } else {
        FlowPBBrowserObj.bReDecodeImages = TRUE;
    }
    PB_PlayBrowserMode(&FlowPBBrowserObj);
    uiStatus = PB_WaitCommandFinish(PB_WAIT_INFINITE);
    DBG_DUMP("UIFlowWndPlayThumb_OnOpen: uiStatus 0x%X\r\n", uiStatus);

#if 0//PLAY_THUMB_AND_MOVIE // play thumbnail and movie together
    UINT32 idx;
    PB_GetParam(PBPRMID_THUMB_CURR_IDX, &idx);
    //DBG_DUMP("UIFlowWndPlayThumb_NaviKey: PBPRMID_THUMB_CURR_IDX %d\r\n", idx);
    PBView_DrawThumbFrame(idx-1, THUMB_DRAW_TMP_BUFFER);
#endif

    UIFlowPBThumb_ShowItemsSubWidgets();

}

static void UIFlowPlayThumb_ScrOpen(lv_obj_t* obj)
{
    DBG_IND("UIFlowPlayThumb_ScrOpen\r\n");

    if(gp == NULL)
    {
        gp = lv_group_create();
    }

    lv_group_add_obj(gp, obj);
    lv_indev_t* indev = lv_plugin_find_indev_by_type(LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_group(indev, gp);

    /* open file and browse */
    UIFlowPlayThumb_BrowseFile();
}
static void UIFlowPlayThumb_ScrClose(lv_obj_t* obj)
{
    DBG_DUMP("UIFlowPlayThumb_ScrClose\r\n");
}

///================================key event start==================================================================================
#if PLAY_THUMB_AND_MOVIE // play thumbnail and movie together
static void UIFlowPlayThumb_OnKeySelect(lv_obj_t* obj)
{
    //INT32  uiActKey;
    char   pFilePath[FULL_FILE_PATH_LEN] = {0};
    UINT32 uiPBFileFmt = PBFMT_MP4;
    UINT32 uiPBFileSize = 0;

    switch (g_PlbData.State) {
    case PLB_ST_FULL:
        PB_SeekCurrent(); // seek current file after leaving thumbnail mode

        PB_GetParam(PBPRMID_CURR_FILEFMT, &uiPBFileFmt);
        if (uiPBFileFmt & (PBFMT_MOVMJPG | PBFMT_AVI | PBFMT_MP4 | PBFMT_TS)) {
            UINT32 u32CurrPbStatus = 0;

            PB_GetParam(PBPRMID_PLAYBACK_STATUS, &u32CurrPbStatus);
            if (u32CurrPbStatus != PB_STA_DONE){
                return NVTEVT_CONSUME;
            }

            // Open Video File
            if (gphUIFlowMovPlay_Filehdl) {
                FileSys_CloseFile(gphUIFlowMovPlay_Filehdl);
                gphUIFlowMovPlay_Filehdl = NULL;
            }
            if (uiPBFileFmt & PBFMT_TS) {
                PB_GetParam(PBPRMID_CURR_FILESIZE, &uiPBFileSize);
                if (uiPBFileSize <= 0x10000) {
                    DBG_DUMP("Wrong video file format!! \r\n");
                    break;
                }
            }
            // Get Current index
            PB_GetParam(PBPRMID_CURR_FILEPATH, (UINT32 *)pFilePath);
            DBG_DUMP("play file path: %s\r\n", pFilePath);

            // Open Test Media File
            gphUIFlowMovPlay_Filehdl = FileSys_OpenFile(pFilePath, FST_OPEN_READ);

            if (!gphUIFlowMovPlay_Filehdl) {
                DBG_DUMP("UIFlowWndPlay_OnKeySelect: Can't open Video file!\r\n");
                break;
            }
		#if 0
            KeyScan_EnableMisc(FALSE);
            UIFlowMoviePlay_SetSpeed(g_PlbData.VideoPBSpeed);
            g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
		#endif
            Ux_SendEvent(0, NVTEVT_EXE_CLOSE, 0); //CustomPlayObjCmdMap

            //flush event first
            Ux_FlushEventByRange(NVTEVT_KEY_EVT_START, NVTEVT_KEY_EVT_END);

            //stop scan
		#if 0
            //SxTimer_SetFuncActive(SX_TIMER_DET_STRG_ID, FALSE);
            SxTimer_SetFuncActive(SX_TIMER_DET_SYSTEM_BUSY_ID, FALSE);
		#endif

            Ux_SetActiveApp(&CustomMoviePlayObjCtrl);
            Ux_SendEvent(0, NVTEVT_EXE_OPENPLAY, 1, (UINT32)gphUIFlowMovPlay_Filehdl);
            Ux_SendEvent(0, NVTEVT_EXE_STARTPLAY, 0);

            //set movie volumn
            Ux_SendEvent(&CustomMoviePlayObjCtrl, NVTEVT_EXE_MOVIEAUDPLAYVOLUME, 2, UI_GetData(FL_MovieAudioPlayIndex), 1);
//            FlowPB_UpdateIcons(0);
            g_PlbData.State = PLB_ST_PLAY_MOV;
            //FlowPB_IconDrawLeftBtn(TRUE);
            //FlowPB_IconDrawRightBtn(TRUE);
            FlowPB_IconDrawMovPlayTime(TRUE);
        }
        break;

    case PLB_ST_PLAY_MOV:
    case PLB_ST_PAUSE_MOV:
        g_PlbData.State        = PLB_ST_FULL;
        g_PlbData.VideoPBSpeed = PLB_FWD_MOV_1x;
	#if 0
        UIFlowMoviePlay_SetSpeed(g_PlbData.VideoPBSpeed);
        g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
	#endif

        // Close MoviePlay module.
        Ux_SendEvent(0, NVTEVT_EXE_CLOSEPLAY, 0);
        Ux_SetActiveApp(&CustomPlayObjCtrl);

	#if 0
        if (gMovie_Play_Info.event_cb) {
            gMovie_Play_Info.event_cb(MOVIEPLAY_EVENT_STOP);
        }
        if (UIStorageCheck(STORAGE_CHECK_ERROR, NULL) == TRUE) {
            return NVTEVT_CONSUME;
        }
	#endif

        // Wakeup playback task and ImageApp_Play module.
        Ux_SendEvent(0, NVTEVT_EXE_OPEN, 0);

	#if 0
        KeyScan_EnableMisc(TRUE);

        if (SxTimer_GetFuncActive(SX_TIMER_DET_SYSTEM_BUSY_ID) == 0) {
            SxTimer_SetFuncActive(SX_TIMER_DET_SYSTEM_BUSY_ID, TRUE);
        }
	#endif

        if (gphUIFlowMovPlay_Filehdl) {
            FileSys_CloseFile(gphUIFlowMovPlay_Filehdl);
            gphUIFlowMovPlay_Filehdl = NULL;
        }

        UIFlowWndPlayThumb_NaviKey(NVTEVT_KEY_ENTER); // recall current thumbnail
        break;

    }

}
#endif

static void UIFlowPlayThumb_OnKeyUp(lv_obj_t* obj)
{
    DBG_DUMP("--up in!!!\r\n");
    UIFlowWndPlayThumb_NaviKey(NVTEVT_KEY_UP);
}

static void UIFlowPlayThumb_OnKeyDown(lv_obj_t* obj)
{
    DBG_DUMP("--down in!!!\r\n");
    UIFlowWndPlayThumb_NaviKey(NVTEVT_KEY_DOWN);
}

static void UIFlowPlayThumb_OnKeyPrev(lv_obj_t* obj)
{
    DBG_DUMP("--left/prev in!!!\r\n");
    UIFlowWndPlayThumb_NaviKey(NVTEVT_KEY_LEFT);
}

static void UIFlowPlayThumb_OnKeyNext(lv_obj_t* obj)
{
    DBG_DUMP("--right/next in!!!\r\n");
    UIFlowWndPlayThumb_NaviKey(NVTEVT_KEY_RIGHT);
}

static void UIFlowPlayThumb_OnKeyZoomIn(lv_obj_t* obj)
{
    UIFlowPBThumb_HiddenAllWidgets();
	lv_plugin_scr_close(UIFlowPlayThumb, gen_nvtmsg_data(NVTRET_THUMBNAIL, 0));
}

static void UIFlowPlayThumb_OnKeyZoomOut(lv_obj_t* obj)
{
    UIFlowPBThumb_HiddenAllWidgets();
	lv_plugin_scr_close(UIFlowPlayThumb, gen_nvtmsg_data(NVTRET_THUMBNAIL, 0));
}

static void UIFlowPlayThumb_OnKeyMode(lv_obj_t* obj)
{

}

static void UIFlowPlayThumb_OnKeyShutter2(lv_obj_t* obj)
{
    DBG_DUMP("close UIFlowPlay %d !!!\r\n", UIFlowPlay);
    DBG_DUMP("close UIFlowPlayThumb %d !!!\r\n", UIFlowPlayThumb);
    DBG_DUMP("close UIFlowPlayThumb %d !!!\r\n", obj->parent);
    UIFlowPBThumb_HiddenAllWidgets();
	lv_plugin_scr_close(UIFlowPlayThumb, gen_nvtmsg_data(NVTRET_THUMBNAIL, 0));
//    lv_plugin_scr_close(obj, NULL);
}

static void UIFlowPlayThumb_OnKeyMenu(lv_obj_t* obj)
{

}
///================================key event end==================================================================================

///================================callback event end==================================================================================

#if PLAY_THUMB_AND_MOVIE // play thumbnail and movie together
static void UIFlowPlayThumb_CB_Finish(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{
    g_PlbData.State        = PLB_ST_FULL;
    g_PlbData.VideoPBSpeed = PLB_FWD_MOV_1x;

#if 0
    UIFlowMoviePlay_SetSpeed(g_PlbData.VideoPBSpeed);
    g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
#endif

    Ux_SendEvent(0, NVTEVT_EXE_CLOSEPLAY, 0);
    Ux_SetActiveApp(&CustomPlayObjCtrl);

    // Wakeup playback task and ImageApp_Play module.
    Ux_SendEvent(0, NVTEVT_EXE_OPEN, 0);

#if 0
    KeyScan_EnableMisc(TRUE);

    if (SxTimer_GetFuncActive(SX_TIMER_DET_SYSTEM_BUSY_ID) == 0) {
        SxTimer_SetFuncActive(SX_TIMER_DET_SYSTEM_BUSY_ID, TRUE);
    }
#endif

    if (gphUIFlowMovPlay_Filehdl) {
        FileSys_CloseFile(gphUIFlowMovPlay_Filehdl);
        gphUIFlowMovPlay_Filehdl = NULL;
    }

    UIFlowWndPlayThumb_NaviKey(NVTEVT_KEY_ENTER); // recall current thumbnail
}
#endif


///================================callback event end==================================================================================

static void UIFlowPlayThumb_Key(lv_obj_t* obj, uint32_t key)
{

    switch(key)
    {
    case LV_USER_KEY_SHUTTER2:
    {
    	UIFlowPlayThumb_OnKeyShutter2(obj);
        break;
    }
    case LV_USER_KEY_UP:
    {
        UIFlowPlayThumb_OnKeyUp(obj);
        break;
    }
    case LV_USER_KEY_DOWN:
    {
        UIFlowPlayThumb_OnKeyDown(obj);
        break;
    }    
    case LV_USER_KEY_PREV:
    {
        UIFlowPlayThumb_OnKeyPrev(obj);
        break;
    }

    case LV_USER_KEY_NEXT:
    {
        UIFlowPlayThumb_OnKeyNext(obj);
        break;
    }

    case LV_USER_KEY_ZOOMIN:
    {
        UIFlowPlayThumb_OnKeyZoomIn(obj);
        break;
    }

    case LV_USER_KEY_ZOOMOUT:
    {
        UIFlowPlayThumb_OnKeyZoomOut(obj);
        break;
    }

    case LV_USER_KEY_MENU:
    {
        UIFlowPlayThumb_OnKeyMenu(obj);
        break;
    }

    case LV_USER_KEY_MODE:
    {
        UIFlowPlayThumb_OnKeyMode(obj);
        break;
    }

//    case LV_KEY_ENTER:
//    {
//        UIFlowPlayThumb_OnKeyEnter(obj);
//        break;
//    }

    }

}

static void UIFlowPlayThumb_NVTMSG(lv_obj_t* obj, const LV_USER_EVENT_NVTMSG_DATA* msg)
{

    switch(msg->event)
    {
    case NVTEVT_BATTERY:
    {
        break;
    }

    case NVTEVT_CB_MOVIE_FINISH:
    {
#if PLAY_THUMB_AND_MOVIE // play thumbnail and movie together
        UIFlowPlayThumb_CB_Finish(obj,msg);
#endif
        break;
    }

    case NVTEVT_BACKGROUND_DONE:
    {
        NVTEVT message=msg->paramArray[ONDONE_PARAM_INDEX_CMD];
        switch (message)
        {
        case NVTEVT_BKW_STOPREC_PROCESS:
        {
            //DBG_DUMP("NVTEVT_BKW_STOPREC_PROCESS:%d\r\n",message);
		    //update_playicons();
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


void UIFlowPlayThumbEventCallback(lv_obj_t* obj, lv_event_t event)
{

	switch(event)
	{

	case LV_PLUGIN_EVENT_SCR_OPEN:
        UIFlowPlayThumb_ScrOpen(obj);
		break;

	case LV_PLUGIN_EVENT_SCR_CLOSE:
        UIFlowPlayThumb_ScrClose(obj);
		break;


	case LV_EVENT_CLICKED:
	{
#if PLAY_THUMB_AND_MOVIE // play thumbnail and movie together
        UIFlowPlayThumb_OnKeySelect(obj);
#endif
		break;
	}

    case LV_EVENT_KEY:
    {
        uint32_t* key = (uint32_t*)lv_event_get_data();

        /* handle key event */
        UIFlowPlayThumb_Key(obj, *key);

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

        UIFlowPlayThumb_NVTMSG(obj, msg);
        break;
    }
   }

}

