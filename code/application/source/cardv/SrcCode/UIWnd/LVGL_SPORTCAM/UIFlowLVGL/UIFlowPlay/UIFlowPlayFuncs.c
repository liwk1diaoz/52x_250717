//#include "UIFlow.h"
#include "UIFlowPlayFuncs.h"
#include <kwrap/nvt_type.h>
#include <kwrap/platform.h>
#include "UIWnd/UIFlow.h"
#include "PlaybackTsk.h"
#include "UIApp/UIAppCommon.h"
#include <kwrap/debug.h>

#include "UIApp/AppDisp_PBView.h"
#include "vf_gfx.h"
#include "SizeConvert.h"
//SizeConvert.h

static UINT32 g_u32MovPlayTime = 0;


PLB_TASK_DATA g_PlbData = {0};

//#NT#2012/8/13#Philex - begin
//Add Thumbnail play data structure
//static PLAY_BROWSER_OBJ     FlowPBBrowserObj;
//PURECT   g_pFlowPBThumbRect;
//URECT    g_FlowPBThumbRect02[FLOWPB_THUMB_H_NUM2 * FLOWPB_THUMB_V_NUM2];
//#NT#2012/8/13#Philex - end

UINT8 FlowPB_GetPlbDataState(void)
{
	return g_PlbData.State;
}

void FlowPB_PlayVideo(void)
{
#if _TODO
	//#NT#2012/7/4#Philex - begin
	//KeyScan_EnableAutoPoweroff(FALSE); //disable auto poweroff,while playing video
	//KeyScan_EnableUSBDet(FALSE);       //disable USB detect,while playing video
	//#NT#2012/7/4#Philex - end
	g_PlbData.State = PLB_ST_PLAY_MOV;
	g_PlbData.VolCount = AUDIO_VOL_63;  //AUDIO_VOL_63
	//#NT#2012/7/4#Philex - begin
	//Ux_SendEvent(&UIPlayObjCtrl,NVTEVT_AUD_VOLUME,1,g_PlbData.VolCount);
	//Ux_SendEvent(&UIPlayObjCtrl,NVTEVT_MOVPLAY,1,UIFlowMoviePlayCBFunc);
	//#NT#2012/7/4#Philex - end
#endif
}

void FlowPB_PauseVideoPlaying(void)
{
	Ux_FlushEventByRange(NVTEVT_KEY_EVT_START, NVTEVT_KEY_EVT_END);
	//#NT#2012/7/4#Philex - begin
	//KeyScan_EnableAutoPoweroff(FALSE); //disable auto poweroff,while playing video
	//KeyScan_EnableUSBDet(FALSE);       //disable USB detect,while playing video
	//#NT#2012/7/4#Philex - end
	g_PlbData.State = PLB_ST_PAUSE_MOV;
	//Ux_SendEvent(&UIPlayObjCtrl,NVTEVT_MOVPAUSE,0);
}

void FlowPB_ResumeVideoPlaying(void)
{
	Ux_FlushEventByRange(NVTEVT_KEY_EVT_START, NVTEVT_KEY_EVT_END);
	//#NT#2012/7/4#Philex - begin
	//KeyScan_EnableAutoPoweroff(FALSE); //disable auto poweroff,while playing video
	//KeyScan_EnableUSBDet(FALSE);       //disable USB detect,while playing video
	//#NT#2012/7/4#Philex - end
	g_PlbData.State = PLB_ST_PLAY_MOV;
	//Ux_SendEvent(&UIPlayObjCtrl,NVTEVT_MOVRESUME,0);
}

void FlowPB_StopVideoPlaying(void)
{
	g_PlbData.State = PLB_ST_FULL;
	g_PlbData.VideoPBSpeed = PLB_FWD_MOV_1x;
	//#NT#2012/7/4#Philex - begin
	//Ux_SendEvent(&UIPlayObjCtrl,NVTEVT_MOVSTOP,0);
	//KeyScan_EnableAutoPoweroff(TRUE);
	//KeyScan_EnableUSBDet(TRUE);
	//#NT#2012/7/4#Philex - end
}

#if 0
//#NT#2012/8/13#Philex - begin
// add Thumbnail play functions
static BOOL FlowPB_ThumbConfig(THUMBNAIL_CONFIG *pThumbCfg)
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
		//DBG_DUMP("ThumbRect[%d]: x %d, y %d, w %d, h %d\r\n", i, g_FlowPBThumbRect02[i].x, g_FlowPBThumbRect02[i].y, g_FlowPBThumbRect02[i].w, g_FlowPBThumbRect02[i].h);
	}

	FlowPBBrowserObj.HorNums = pThumbCfg->wThumbPerLine;
	FlowPBBrowserObj.VerNums = pThumbCfg->wThumbPerColumn;
	//uiThumbFrameColor = pThumbCfg->wFrColor;

	return (TRUE);
}

PURECT FlowPB_ThumbDisplay(void)
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
	FlowPB_ThumbConfig(&ThumbCfg);
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

void FlowPB_SetMovPlayTime(UINT32 u32PlayTime)
{
	g_u32MovPlayTime = u32PlayTime;
}

UINT32 FlowPB_GetMovPlayTime(void)
{
   return g_u32MovPlayTime;
}

//#NT#2012/8/13#Philex - end
