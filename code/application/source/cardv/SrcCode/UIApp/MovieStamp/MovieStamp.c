#include <stdio.h>
#include <string.h>

#include "PrjInc.h"
#include "MovieStamp.h"
#include "MovieStampAPI.h"
#include "Utility/SwTimer.h"
#include "kwrap/type.h"

#include <kflow_common/nvtmpp.h>
#include "FontConv/FontConv.h"
#include "ImageApp/ImageApp_MovieMulti.h"
#include "ImageApp/ImageApp_Photo.h"

#include "vf_gfx.h"
#include "GxTime.h"
#include "hd_type.h"

#if MOVIE_ISP_LOG
#include "vendor_isp.h"
#endif
//#NT#2016/10/17#Bin Xiao -end
#define __MODULE__              MovieStamp
#define __DBGLVL__              2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__              "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>


#define COLOR_ID_BG             0
#define COLOR_ID_FR             1
#define COLOR_ID_FG             2

#define STAMP_WIDTH_TOLERANCE   8           // total font width error tolerance
#define STAMP_LOGO_GAP          8           // date stamp and water logo position gap
//#define VIDEO_IN_MAX            12//8//13//5
#define VIDEO_IN_MAX            16//8//13//5
#if MOVIE_ISP_LOG
#define MOVIE_STAMP_CHK_TIME    50          // 50ms check once
#else
#define MOVIE_STAMP_CHK_TIME    1000//50          // 50ms check once
#endif
#define MOVIE_STAMP_MAX_LEN     256
#define VSFONT_BUF_RATIO  (550)
#define OSG_BUF_RATIO  (400)
#if(defined(_NVT_ETHREARCAM_RX_) && (ETHCAM_EIS ==ENABLE))
#define VENC_OUT_PORTID_MAX  (SENSOR_CAPS_COUNT*(2+1)+1 + ETH_REARCAM_CAPS_COUNT)
#else
#define VENC_OUT_PORTID_MAX  (SENSOR_CAPS_COUNT*(2+1)+1)
#endif
//variable declare
static STAMP_POS    g_MovieStampPos[VIDEO_IN_MAX] = {0};
#if defined (MOVIE_MULTISTAMP_FUNC) && (MOVIE_MULTISTAMP_FUNC == ENABLE)
static UPOINT    g_MovieGpsStampPos[VIDEO_IN_MAX] = {0};
#endif
char  g_cMovieStampStr[VIDEO_IN_MAX][MOVIE_STAMP_MAX_LEN]={0};
static UINT32 g_uiMovieStampSetup[VIDEO_IN_MAX] = {
	STAMP_OFF, STAMP_OFF, STAMP_OFF, STAMP_OFF,
	STAMP_OFF, STAMP_OFF, STAMP_OFF, STAMP_OFF,
	STAMP_OFF, STAMP_OFF, STAMP_OFF, STAMP_OFF,
	STAMP_OFF, STAMP_OFF, STAMP_OFF, STAMP_OFF,
};
static STAMP_INFO   g_MovieStampInfo[VIDEO_IN_MAX];
static struct tm    g_CurDateTime;
static UINT32       g_uiMovieStampYAddr[VIDEO_IN_MAX][2] = {0}; // movie stamp Y address (2 means double buffer)
static UINT32       g_uiWaterLogoYAddr[VIDEO_IN_MAX] = {0}, g_uiWaterLogoUVAddr[VIDEO_IN_MAX] = {0};
static WATERLOGO_BUFFER g_MovieWaterInfo[VIDEO_IN_MAX] = {0};
static BOOL         g_bWaterLogoEnable[VIDEO_IN_MAX] = {0};

static STAMP_ADDR_INFO g_MovieStampPoolAddr[VIDEO_IN_MAX]={0};
static UINT32 g_uiVsStampAddr[VIDEO_IN_MAX] = {0};
static UINT32 g_uiVsFontAddr[VIDEO_IN_MAX] = {0};
static UINT32 g_VsStampEn=FALSE;
UINT32 g_VsOsdWidth[VIDEO_IN_MAX] = {0};
UINT32 g_VsOsdHight[VIDEO_IN_MAX] = {0};
static UINT32 g_uiVsStampSize[VIDEO_IN_MAX] = {0};
static UINT32 g_uiVsStampPa[VIDEO_IN_MAX] = {0};
static UINT32 g_uiVsFontSize[VIDEO_IN_MAX] = {0};
FONT_CONV_IN g_VsFontIn[VIDEO_IN_MAX]={0};
FONT_CONV_OUT g_VsFontOut[VIDEO_IN_MAX]={0};
UINT32 g_pVsFontDataAddr[VIDEO_IN_MAX];
static UINT32 g_VEncHDPathId[VENC_OUT_PORTID_MAX]={0};			//hd_videoenc_open(HD_STAMP_0), Encport diff--> date stamp: main , clone, rawenc, wifi, 0, 1, 2, 3. sen1:main/clone 0,1, rawenc 5; sen2:main/clone 2,3 , rawenc 5, wifi 4
#if defined (WATERLOGO_FUNCTION) && (WATERLOGO_FUNCTION == ENABLE)
static UINT32 g_WaterlogoVEncHDPathId[VENC_OUT_PORTID_MAX]={0}; //hd_videoenc_open(HD_STAMP_1), Encport diff--> waterlogo: main , clone, rawenc, wifi, 0, 1, 2, 3
#endif

//date(HD_STAMP_0)
//waterlogo(HD_STAMP_1)
//multistamp1(HD_STAMP_2), multistamp2 (HD_STAMP_3), multistamp3 (HD_STAMP_4), multistamp4 (HD_STAMP_5), multistamp5 (HD_STAMP_6)
//multiewaterlogo1(HD_STAMP_7), multiwaterlogo2 (HD_STAMP_8), multiwaterlogo3 (HD_STAMP_9), multiwaterlogo4 (HD_STAMP_10), multiwaterlogo5 (HD_STAMP_11)
#define VSSTAMP_MAX_VIDEOENC_PATH  12// 4
#define VSSTAMP_DATE_HD_STAMP_ID  				(HD_STAMP_0)
#define VSSTAMP_WATERLOGO_HD_STAMP_ID  			(HD_STAMP_1)
#define VSSTAMP_MULTISTAMP_HD_STAMP_ID  			(HD_STAMP_2)
#define VSSTAMP_MULTIWATERLOGO_HD_STAMP_ID  	(HD_STAMP_6)

static UINT32 g_VsStampStart[VENC_OUT_PORTID_MAX][VSSTAMP_MAX_VIDEOENC_PATH]={0}; //date stamp, main , clone, rawenc , wifi , 0, 1, 2, 3
static UINT32 isUI_GfxInitLiteOpened = FALSE;
static STAMP_ADDR_INFO g_GfxInitPoolAddr={0};
static BOOL         g_bMovieStampSwTimerOpen = FALSE;
static SWTIMER_ID   g_MovieStampSwTimerID;

//IC HW limitation
#if defined(_BSP_NA51055_)
#define MOVIE_STAMP_MAX_REGION_PER_LAYER  16
#define MOVIE_STAMP_MAX_LAYER  2
#define MOVIE_STAMP_LAYER1  0
#define MOVIE_STAMP_LAYER2  1
#elif defined(_BSP_NA51089_)
#define MOVIE_STAMP_MAX_REGION_PER_LAYER  10
#define MOVIE_STAMP_MAX_LAYER  1
#define MOVIE_STAMP_LAYER1  0
#define MOVIE_STAMP_LAYER2  0
#endif

#define MOVIE_MULTI_STAMP_CNT_MAX     3
#define MOVIE_MULTI_WATERLOGO_CNT_MAX           5
#if((2+MOVIE_MULTI_STAMP_CNT_MAX + MOVIE_MULTI_WATERLOGO_CNT_MAX) >(MOVIE_STAMP_MAX_REGION_PER_LAYER*MOVIE_STAMP_MAX_LAYER)) // 2 = date + normal waterlogo
	#error "total stamp count is overflow"
#endif
#if defined (MOVIE_MULTIWATERLOGO_FUNC) && (MOVIE_MULTIWATERLOGO_FUNC == ENABLE)
static  WATERLOGO_BUFFER    g_sMultiWaterLogo[VIDEO_IN_MAX][MOVIE_MULTI_WATERLOGO_CNT_MAX];
static UINT32 g_MultiWaterLogoVEncHDPathId[VENC_OUT_PORTID_MAX][MOVIE_MULTI_WATERLOGO_CNT_MAX]={0};
#endif

#define VS_DATESTAMP_REGION 		(MOVIE_STAMP_MAX_REGION_PER_LAYER -1 )
#define VS_WATERLOGO_REGION 		(VS_DATESTAMP_REGION -1)
#define VS_MULTISTAMP_REGION 		(VS_WATERLOGO_REGION- MOVIE_MULTI_STAMP_CNT_MAX)
#define VS_MULTIWATERLOGO_REGION 	(VS_MULTISTAMP_REGION- MOVIE_MULTI_WATERLOGO_CNT_MAX)

const UINT16 g_ucStamp_Null[8*8] = {0};
#if defined(_UI_STYLE_LVGL_)

#include "UIApp/lv_user_font_conv/lv_user_font_conv.h"

#define LV_USER_FONT_CONV_ALIGN_W 8
#define LV_USER_FONT_CONV_ALIGN_H 2



static inline uint32_t lv_color4444_to32(lv_user_color4444_t color)
{
    lv_color32_t ret;
    LV_COLOR_SET_R32(ret, LV_USER_COLOR_GET_R16(color) * 17); /*(2^8 - 1)/(2^4 - 1) = 255/15 = 17*/
    LV_COLOR_SET_G32(ret, LV_USER_COLOR_GET_G16(color) * 17); /*(2^8 - 1)/(2^4 - 1) = 255/15 = 17*/
    LV_COLOR_SET_B32(ret, LV_USER_COLOR_GET_B16(color) * 17); /*(2^8 - 1)/(2^4 - 1) = 255/15 = 17*/
    LV_COLOR_SET_A32(ret, LV_USER_COLOR_GET_A16(color) * 17); /*(2^8 - 1)/(2^4 - 1) = 255/15 = 17*/

    return ret.full;
}

static ISIZE MovieStamp_GetStampDataWidth_LVGL(PSTAMP_INFO pStampInfo, const lv_font_t *pFont);

static void _movie_stamp_lv_cfg_init(lv_user_font_conv_draw_cfg *draw_cfg)
{
	lv_user_font_conv_draw_cfg_init(draw_cfg);

	draw_cfg->fmt = HD_VIDEO_PXLFMT_ARGB4444;
	draw_cfg->radius = LV_USER_CFG_STAMP_RADIUS;
	draw_cfg->string.align = LV_USER_CFG_STAMP_TEXT_ALIGN;
	draw_cfg->string.letter_space = LV_USER_CFG_STAMP_LETTER_SPACE;
	draw_cfg->ext_w = LV_USER_CFG_STAMP_EXT_WIDTH;
	draw_cfg->ext_h = LV_USER_CFG_STAMP_EXT_HEIGHT;
	draw_cfg->border.width = LV_USER_CFG_STAMP_BORDER_WIDTH;

}

static ER FontConv_LVGL(FONT_CONV_IN *pIn, FONT_CONV_OUT *pOut)
{
	HD_RESULT r;
	lv_user_font_conv_draw_cfg draw_cfg = {0};
	lv_user_font_conv_mem_cfg mem_cfg = {0};
	lv_user_font_conv_calc_buffer_size_result result = {0};
	lv_user_color4444_t color4444;
	lv_color32_t color32;

	_movie_stamp_lv_cfg_init(&draw_cfg);

	if(draw_cfg.fmt != pIn->Format){
		DBG_ERR("Unexpected pIn->Format(%lx), it's supposed to be %lx\r\n", pIn->Format, draw_cfg.fmt);
		return -1;
	}

	draw_cfg.fmt = pIn->Format;
	draw_cfg.string.font = (lv_font_t *) pIn->pFont;
	draw_cfg.string.text = pIn->pStr;
	draw_cfg.align_w = LV_USER_FONT_CONV_ALIGN_W;
	draw_cfg.align_h = LV_USER_FONT_CONV_ALIGN_H;
	draw_cfg.string.align = LV_USER_CFG_STAMP_TEXT_ALIGN;

	/************************************************
	 * Color setting convertion flow
	 * lv user format argb4444 -> lv format 8888 -> lv format by LV_COLOR_DEPTH (332 or 8888)
	 ************************************************/

	/* text color */
	color4444.full = pIn->ciSolid;
	color32.full = lv_color4444_to32(color4444);
	draw_cfg.string.color = LV_COLOR_MAKE(color32.ch.red, color32.ch.green, color32.ch.blue);
	draw_cfg.string.opa = LV_COLOR_GET_A32(color32);

	/* bg color */
	color4444.full = pIn->ciTransparet;
	color32.full = lv_color4444_to32(color4444);
	draw_cfg.bg.color = LV_COLOR_MAKE(color32.ch.red, color32.ch.green, color32.ch.blue);
	draw_cfg.bg.opa =  LV_COLOR_GET_A32(color32);

	/* border color */
	color4444.full = pIn->ciFrame;
	color32.full = lv_color4444_to32(color4444);
	draw_cfg.border.color = LV_COLOR_MAKE(color32.ch.red, color32.ch.green, color32.ch.blue);
	draw_cfg.border.opa = LV_COLOR_GET_A32(color32);
	draw_cfg.border.width = LV_USER_CFG_STAMP_BORDER_WIDTH;

	mem_cfg.output_buffer = (void*)pIn->MemAddr;
	mem_cfg.output_buffer_size = pIn->MemSize;


	lv_user_font_conv_calc_buffer_size(&draw_cfg, &result);
	lv_user_font_conv(&draw_cfg, &mem_cfg);

	UINT32 LineOffs = result.width * (float)(result.bpp / 8);
	UINT32 PxlAddrs = (UINT32) mem_cfg.output_buffer; /* osg needs virtual address not physical address */

	r =  vf_init_ex(&pOut->GenImg, result.width, result.height, pIn->Format, &LineOffs, &PxlAddrs);
	if (r != HD_OK) {
		DBG_ERR("vf_init_ex failed %d\r\n",r);
	}

	return 0;
}

static ISIZE MovieStamp_GetStampDataWidth_LVGL(PSTAMP_INFO pStampInfo, const lv_font_t *pFont)
{
	ISIZE 	FontSize={0};
	lv_user_font_conv_draw_cfg draw_cfg = {0};
	lv_user_font_conv_calc_buffer_size_result result = {0};

	_movie_stamp_lv_cfg_init(&draw_cfg);

	draw_cfg.string.font = (lv_font_t *)pFont;
	draw_cfg.string.text = pStampInfo->pi8Str;
	draw_cfg.align_w = LV_USER_FONT_CONV_ALIGN_W;
	draw_cfg.align_h = LV_USER_FONT_CONV_ALIGN_H;

	lv_user_font_conv_calc_buffer_size(&draw_cfg, &result);
	FontSize.w = result.width;
	FontSize.h = result.height;

	return FontSize;//uiDataWidth;
}
#else

#include "DateStampFontTbl36x60.h"
#include "DateStampFontTbl10x16.h"
#include "DateStampFontTbl12x20.h"
#include "DateStampFontTbl18x30.h"
#include "DateStampFontTbl20x44.h"
#include "DateStampFontTbl26x44.h"

static ISIZE MovieStamp_GetStampDataWidth(PSTAMP_INFO pStampInfo, const IMAGE_TABLE *pTable);

ISIZE MovieStamp_GetStampDataWidth(PSTAMP_INFO pStampInfo, const IMAGE_TABLE *pTable)
{
	UINT32  i;
	UINT32  uiStrLen;
	UINT32  uiDataWidth;
	UINT32  uiIconID;
	ISIZE 	FontSize={0};

	uiStrLen = strlen(pStampInfo->pi8Str);
	uiDataWidth = 0;
	for (i = 0; i < uiStrLen; i++) {
		//get icon database
		uiIconID = pStampInfo->pi8Str[i];
		IMAGE_GetSizeFromTable(pTable, uiIconID, &FontSize);
		uiDataWidth += FontSize.w;
	}
	FontSize.w=uiDataWidth;
	return FontSize;//uiDataWidth;
}

#endif

#if MOVIE_ISP_LOG
static void MovieStamp_get_isp_status(UINT32 id, char* Buf, UINT32 BufLen)
{
	AET_STATUS_INFO ae_status = {0};
	AWBT_STATUS awb_status = {0};
	IQT_WDR_PARAM wdr = {0};
	IQT_DEFOG_PARAM defog = {0};

	ae_status.id = id;
	vendor_isp_get_ae(AET_ITEM_STATUS, &ae_status);
	awb_status.id = id;
	vendor_isp_get_awb(AWBT_ITEM_STATUS, &awb_status);
	wdr.id = id;
	vendor_isp_get_iq(IQT_ITEM_WDR_PARAM, &wdr);
	defog.id = id;
	vendor_isp_get_iq(IQT_ITEM_DEFOG_PARAM, &defog);
	snprintf(Buf, BufLen, "%3d %4d %4d %6d %6d %4d %4d %d %d %4d %4d %4d\0",
												ae_status.status_info.lv/100000,
												ae_status.status_info.lum,
												ae_status.status_info.expect_lum,
												ae_status.status_info.expotime[0],
												ae_status.status_info.iso_gain[0],
												ae_status.status_info.overexp_adj,
												ae_status.status_info.overexp_cnt,
												wdr.wdr.enable,
												defog.defog.enable,
												awb_status.status.cur_r_gain,
												awb_status.status.cur_b_gain,
												awb_status.status.cur_ct
												);
	//DBG_DUMP("isp Buf=%s\r\n",Buf);

	return;
}
#endif
UINT32 MovieStamp_TriggerUpdateChk(void)
{

	struct tm   CurDateTime;
	GxTime_GetTime(&CurDateTime);
	// check time varying
	if ((g_CurDateTime.tm_sec  != CurDateTime.tm_sec)  ||
		(g_CurDateTime.tm_min  != CurDateTime.tm_min)  ||
		(g_CurDateTime.tm_hour != CurDateTime.tm_hour) ||
		(g_CurDateTime.tm_mday != CurDateTime.tm_mday) ||
		(g_CurDateTime.tm_mon  != CurDateTime.tm_mon)  ||
		(g_CurDateTime.tm_year != CurDateTime.tm_year))	{

		g_CurDateTime = CurDateTime;
		// time varied, update stamp database
		return 1;
	}
	return 0;
}

static void MovieStamp_SwTimerHdl(UINT32 uiEvent)
{
	MovieStampTsk_TrigUpdate();
}

static void MovieStamp_SwTimerOpen(void)
{
	if (g_bMovieStampSwTimerOpen == FALSE) {
		if (SwTimer_Open(&g_MovieStampSwTimerID, MovieStamp_SwTimerHdl) != E_OK) {
			DBG_ERR("Sw timer open failed!\r\n");
			return;
		}

		SwTimer_Cfg(g_MovieStampSwTimerID, MOVIE_STAMP_CHK_TIME, SWTIMER_MODE_FREE_RUN);
		SwTimer_Start(g_MovieStampSwTimerID);
		g_bMovieStampSwTimerOpen = TRUE;
	}
}

static void MovieStamp_SwTimerClose(void)
{
	if (g_bMovieStampSwTimerOpen) {
		SwTimer_Stop(g_MovieStampSwTimerID);
		SwTimer_Close(g_MovieStampSwTimerID);
		g_bMovieStampSwTimerOpen = FALSE;
	}
}

void MovieStamp_Enable(void)
{
	struct tm   CurDateTime;
	GxTime_GetTime(&CurDateTime);
	g_CurDateTime = CurDateTime;

	MovieStamp_UpdateData();

	// register movie stamp update callback
	MovieStampTsk_RegUpdateCB(MovieStamp_UpdateData);
	MovieStampTsk_RegTrigUpdateChkCB(MovieStamp_TriggerUpdateChk);

	// open movie stamp task to wait for stamp update flag
	MovieStampTsk_Open();

	// use SW timer to check current time
	MovieStamp_SwTimerOpen();
}

void MovieStamp_Disable(void)
{
	UINT32 i;

	// close SW timer
	MovieStamp_SwTimerClose();

	// close movie stamp task
#if defined(_UI_STYLE_LVGL_)

	/*****************************************************************************
	 * MovieStampTsk_Close might cause deadlock if invoked by ui task
	 *
	 * UI task:
	 * 1. lock -> ui flow
	 * 2. wait movie stamp task idle (stuck in here)
	 * 3. unlock
	 *
	 * MovieStamp task:
	 * 1. busy
	 * 2. lock (stuck in here)
	 * 3. idle
	 *
	 * lv_user_task_handler_temp_release will unlock temporarily until MovieStampTsk_Close finished
	 ****************************************************************************/

	lv_user_task_handler_temp_release(MovieStampTsk_Close);

#else
	MovieStampTsk_Close();
#endif

	MovieStamp_VsClose();
	for (i = 0; i < VIDEO_IN_MAX; i++) {
		MovieStamp_Setup(
			i,
			STAMP_OFF,
			0,
			0,
			NULL);
	}
}

//-------------------------------------------------------------------------------------------------
void MovieStamp_Setup(UINT32 uiVEncOutPortId, UINT32 uiFlag, UINT32 uiImageWidth, UINT32 uiImageHeight, WATERLOGO_BUFFER *pWaterLogoBuf)
{
    PSTAMP_INFO     pStampInfo;
    UINT32          uiIconID;
    ISIZE 	FontSize;
#if MOVIE_ISP_LOG
	UINT16 i=0;
	for(i=0;i<SENSOR_CAPS_COUNT;i++){
		if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetVdoEncPort(_CFG_CLONE_ID_1+i)) - 1)){
			return;
		}
		if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetVdoEncPort(_CFG_STRM_ID_1+i)) - 1)){
			return;
		}
		//raw enc
		if(MovieStamp_IsRawEncVirPort(uiVEncOutPortId)){
			return;
		}
	}
#endif

    g_uiMovieStampSetup[uiVEncOutPortId] = uiFlag;

    if ((uiFlag & STAMP_SWITCH_MASK) == STAMP_OFF) {
        return;
    }
#if 0//
    UI_GfxInitLite();
#endif

    g_bWaterLogoEnable[uiVEncOutPortId] = pWaterLogoBuf ? TRUE : FALSE;

/* water logo information recording*/
#if defined (WATERLOGO_FUNCTION) && (WATERLOGO_FUNCTION == ENABLE)
    {
        if (g_bWaterLogoEnable[uiVEncOutPortId]) {
            // setup water logo
            memcpy((void *)&g_MovieWaterInfo[uiVEncOutPortId], (const void *)pWaterLogoBuf, sizeof(WATERLOGO_BUFFER));
        }
    }
    //printf("%s[%d]---------------->vedio[%d*%d] logo w*h [%d*%d]\n",__func__,__LINE__,uiImageWidth,uiImageHeight,pWaterLogoBuf->uiWidth,pWaterLogoBuf->uiHeight);

#endif

	pStampInfo = &g_MovieStampInfo[uiVEncOutPortId];
	pStampInfo->pi8Str = &g_cMovieStampStr[uiVEncOutPortId][0];

/**************************************
  1.choice front data source;
  2.scale water logo width and height
***************************************/
    // set date stamp font data base

#if defined(_UI_STYLE_LVGL_)

	lv_plugin_res_id red_id;

//    switch (uiImageWidth) {
//    case 3840:  // 3840x2160
//    case 2880:  // 2880x2160 (DAR 16:9)
//    	red_id = LV_USER_CFG_STAMP_FONT_ID_XXL;
//        break;
//
//
//    case 2592:  // 2592x1944
//    case 2560:  // 2560x1440
//    case 2304:  // 2304x1296
//    	red_id = LV_USER_CFG_STAMP_FONT_ID_XL;
//    	break;
//
//    case 1920:  // 1920x1080
//    case 1536:  // 1536x1536
//    case 1728:  // 1728x1296 (DAR 16:9)
//    case 1440:  // 1440x1080 (DAR 16:9)
//    case 1280:  // 1280x720
//    	red_id = LV_USER_CFG_STAMP_FONT_ID_LARGE;
//        break;
//
//    case 848:    //848*480 wifi
//    case 640:    // VGA & others
//    	red_id = LV_USER_CFG_STAMP_FONT_ID_MEDIUM;
//        break;
//
//    case 320:   // QVGA
//    	red_id = LV_USER_CFG_STAMP_FONT_ID_SMALL;
//        break;
//
//    default:    // VGA & others
//    	red_id = LV_USER_CFG_STAMP_FONT_ID_MEDIUM;
//        break;
//    }

	if (uiImageWidth >= 3840) {
		red_id = LV_USER_CFG_STAMP_FONT_ID_XXL;
	}
	else if(uiImageWidth >=3600) {
		red_id = LV_USER_CFG_STAMP_FONT_ID_XXL;
	}
	else if(uiImageWidth >=3200) {
		red_id = LV_USER_CFG_STAMP_FONT_ID_XL;
	}
	else if(uiImageWidth >=2880) {
		red_id = LV_USER_CFG_STAMP_FONT_ID_XL;
	}
	else if(uiImageWidth >=1920) {
		red_id = LV_USER_CFG_STAMP_FONT_ID_LARGE;
	}
	else if(uiImageWidth >=1080) {
		red_id = LV_USER_CFG_STAMP_FONT_ID_MEDIUM;
	}
	else if(uiImageWidth >=640) {
		red_id = LV_USER_CFG_STAMP_FONT_ID_SMALL;
	}
	else if(uiImageWidth >=320) {
		red_id = LV_USER_CFG_STAMP_FONT_ID_SMALL;
	}
	else {
		DBG_WRN("image_width(%lu) is too small!\n", uiImageWidth);
		red_id = LV_USER_CFG_STAMP_FONT_ID_SMALL;
	}

    g_VsFontIn[uiVEncOutPortId].pFont=(FONT *) lv_plugin_get_font(red_id)->font;

#else

    switch (uiImageWidth) {
    case 3840:  // 3840x2160
        g_VsFontIn[uiVEncOutPortId].pFont=(FONT *)gDateStampFontTbl26x44;
        break;
    case 2592:  // 2592x1944
    case 2560:  // 2560x1440
    case 2304:  // 2304x1296
    case 1920:  // 1920x1080
    case 1536:  // 1536x1536
        g_VsFontIn[uiVEncOutPortId].pFont=(FONT *)gDateStampFontTbl26x44;
        break;

    case 2880:  // 2880x2160 (DAR 16:9)
    case 1728:  // 1728x1296 (DAR 16:9)
    case 1440:  // 1440x1080 (DAR 16:9)
        g_VsFontIn[uiVEncOutPortId].pFont=(FONT *)gDateStampFontTbl20x44;
        break;

    case 1280:  // 1280x720
        g_VsFontIn[uiVEncOutPortId].pFont=(FONT *)gDateStampFontTbl18x30;
        break;

    case 848:    //848*480 wifi
    case 640:    // VGA & others
        g_VsFontIn[uiVEncOutPortId].pFont=(FONT *)gDateStampFontTbl12x20;
        break;

    case 320:   // QVGA
        g_VsFontIn[uiVEncOutPortId].pFont=(FONT *)gDateStampFontTbl10x16;
        break;

    default:    // VGA & others
        g_VsFontIn[uiVEncOutPortId].pFont=(FONT *)gDateStampFontTbl12x20;
        break;
    }

#endif

#if MOVIE_ISP_LOG
        g_VsFontIn[uiVEncOutPortId].pFont=(FONT *)gDateStampFontTbl12x20;
#endif
/* do water logo scaling*/
#if defined (WATERLOGO_FUNCTION) && (WATERLOGO_FUNCTION == ENABLE)
	{
		if (g_bWaterLogoEnable[uiVEncOutPortId]) {
			g_MovieWaterInfo[uiVEncOutPortId].uiWidth = ALIGN_CEIL_8(pWaterLogoBuf->uiWidth);
			g_MovieWaterInfo[uiVEncOutPortId].uiHeight = ALIGN_CEIL_4(pWaterLogoBuf->uiHeight);
		}
	}
#endif

    // set stamp string (for calculating stamp position)
    switch (uiFlag & STAMP_DATE_TIME_MASK) {
    case STAMP_DATE_TIME_AMPM:
        //sprintf(pStampInfo->pi8Str, "0000/00/00 00:00:00 AM");
        snprintf(pStampInfo->pi8Str, MOVIE_STAMP_MAX_LEN, "0000/00/00 00:00:00 AM");
        break;

    case STAMP_DATE: // date only is not suitable for movie stamp (it's suitable for still image stamp)
    case STAMP_TIME:
        //sprintf(pStampInfo->pi8Str, "00:00:00");
        snprintf(pStampInfo->pi8Str, MOVIE_STAMP_MAX_LEN, "00:00:00");
        break;

    case STAMP_DATE_TIME:
    default:
            snprintf(pStampInfo->pi8Str, MOVIE_STAMP_MAX_LEN, "0000/00/00 00:00:00");
        break;
    }
#if MOVIE_ISP_LOG
	UINT16 Ipl_id=0;

	HD_RESULT hd_ret;
	UINT32 isp_ver = 0;
	if (vendor_isp_get_common(ISPT_ITEM_VERSION, &isp_ver) == HD_ERR_UNINIT) {
		if ((hd_ret = vendor_isp_init()) != HD_OK) {
			DBG_ERR("vendor_isp_init fail(%d)\n", hd_ret);
		}
	}

	Ipl_id=ImageApp_MovieMulti_VePort2Imglink(uiVEncOutPortId);
	//DBG_DUMP("setup Ipl_id=%d\r\n",Ipl_id);

    MovieStamp_get_isp_status(Ipl_id, &g_cMovieStampStr[uiVEncOutPortId][0], 256);
#endif


#if defined(_UI_STYLE_LVGL_)

	LV_UNUSED(FontSize);
	LV_UNUSED(uiIconID);

	ISIZE sz = MovieStamp_GetStampDataWidth_LVGL(pStampInfo, (const lv_font_t *)g_VsFontIn[uiVEncOutPortId].pFont);

	pStampInfo->ui32FontWidth = sz.w / strlen(pStampInfo->pi8Str);
	pStampInfo->ui32FontHeight = sz.h;

	DBG_DUMP("pStampInfo = {%u, %u}\r\n", pStampInfo->ui32FontWidth, pStampInfo->ui32FontHeight);

#else

    uiIconID = pStampInfo->pi8Str[0]; // 1st font
    DBG_DUMP("[%d]##############uiIconID=%d, pi8Str=%s\n",uiVEncOutPortId,uiIconID,pStampInfo->pi8Str);


    RESULT ret;

    if((ret=IMAGE_GetSizeFromTable((const IMAGE_TABLE *)g_VsFontIn[uiVEncOutPortId].pFont, uiIconID, &FontSize))==0){
	    pStampInfo->ui32FontWidth = FontSize.w;
	    pStampInfo->ui32FontHeight = FontSize.h;
    }else{
            DBG_ERR("IMAGE_GetSizeFromTable ,ret=%d\n",ret);
            return;
    }

#endif

    pStampInfo->ui32DstHeight  = pStampInfo->ui32FontHeight; // no scaling

    //printf("%s[%d]----------------> w*h[%d*%d]\n",__func__,__LINE__,FontSize.w,FontSize.h);
        UINT32  uiStampWidth=0;// = (pStampInfo->ui32DstHeight * pStampInfo->ui32FontWidth) / pStampInfo->ui32FontHeight;
    // Set date stamp position
    if ((uiFlag & STAMP_OPERATION_MASK) == STAMP_AUTO) {
        //UINT32  uiStampWidth=0;// = (pStampInfo->ui32DstHeight * pStampInfo->ui32FontWidth) / pStampInfo->ui32FontHeight;
        if(pStampInfo->ui32FontHeight){
        	uiStampWidth = (pStampInfo->ui32DstHeight * pStampInfo->ui32FontWidth) / pStampInfo->ui32FontHeight;
        }else{
            DBG_ERR("ui32FontHeight zero!\n");
            return;
        }
        switch (uiFlag & STAMP_POSITION_MASK) {
        case STAMP_TOP_LEFT:
            if ((uiFlag & STAMP_POS_END_MASK) == STAMP_POS_END) {
                g_MovieStampPos[uiVEncOutPortId].uiX = 0;
                g_MovieStampPos[uiVEncOutPortId].uiY = 0;
            } else {
                g_MovieStampPos[uiVEncOutPortId].uiX = uiStampWidth; // 1 font width gap
                g_MovieStampPos[uiVEncOutPortId].uiY = pStampInfo->ui32DstHeight / 2; // 1/2 font height gap
            }
#if defined (WATERLOGO_FUNCTION) && (WATERLOGO_FUNCTION == ENABLE)
            if (g_bWaterLogoEnable[uiVEncOutPortId]) {
                if (g_MovieWaterInfo[uiVEncOutPortId].uiXPos == WATERLOGO_AUTO_POS && g_MovieWaterInfo[uiVEncOutPortId].uiYPos == WATERLOGO_AUTO_POS) {
                    g_MovieWaterInfo[uiVEncOutPortId].uiXPos = g_MovieStampPos[uiVEncOutPortId].uiX;
                    g_MovieWaterInfo[uiVEncOutPortId].uiYPos = g_MovieStampPos[uiVEncOutPortId].uiY;
                    g_MovieStampPos[uiVEncOutPortId].uiX += (g_MovieWaterInfo[uiVEncOutPortId].uiWidth + STAMP_LOGO_GAP);
                }
            }
#endif
            break;

        case STAMP_TOP_RIGHT:
            if ((uiFlag & STAMP_POS_END_MASK) == STAMP_POS_END) {
                g_MovieStampPos[uiVEncOutPortId].uiX = uiImageWidth - uiStampWidth * strlen(pStampInfo->pi8Str) - STAMP_WIDTH_TOLERANCE;
                g_MovieStampPos[uiVEncOutPortId].uiY = 0;
            } else {
                g_MovieStampPos[uiVEncOutPortId].uiX = uiImageWidth - uiStampWidth * (strlen(pStampInfo->pi8Str) + 1); // 1 font width gap
                g_MovieStampPos[uiVEncOutPortId].uiY = pStampInfo->ui32DstHeight / 2; // 1/2 font height gap
            }
#if defined (WATERLOGO_FUNCTION) && (WATERLOGO_FUNCTION == ENABLE)
            if (g_bWaterLogoEnable[uiVEncOutPortId]) {
                if (g_MovieWaterInfo[uiVEncOutPortId].uiXPos == WATERLOGO_AUTO_POS && g_MovieWaterInfo[uiVEncOutPortId].uiYPos == WATERLOGO_AUTO_POS) {
                    g_MovieWaterInfo[uiVEncOutPortId].uiXPos = g_MovieStampPos[uiVEncOutPortId].uiX - g_MovieWaterInfo[uiVEncOutPortId].uiWidth - STAMP_LOGO_GAP;
                    g_MovieWaterInfo[uiVEncOutPortId].uiYPos = g_MovieStampPos[uiVEncOutPortId].uiY;
                }
            }
#endif
            break;

        case STAMP_BOTTOM_LEFT:
            if ((uiFlag & STAMP_POS_END_MASK) == STAMP_POS_END) {
                g_MovieStampPos[uiVEncOutPortId].uiX = 0;
                g_MovieStampPos[uiVEncOutPortId].uiY = uiImageHeight - pStampInfo->ui32DstHeight;
            } else {
                g_MovieStampPos[uiVEncOutPortId].uiX = uiStampWidth; // 1 font width gap
                g_MovieStampPos[uiVEncOutPortId].uiY = uiImageHeight - (pStampInfo->ui32DstHeight * 3) / 2; // 1/2 font height gap
            }
#if defined (WATERLOGO_FUNCTION) && (WATERLOGO_FUNCTION == ENABLE)
            if (g_bWaterLogoEnable[uiVEncOutPortId]) {
                if (g_MovieWaterInfo[uiVEncOutPortId].uiXPos == WATERLOGO_AUTO_POS && g_MovieWaterInfo[uiVEncOutPortId].uiYPos == WATERLOGO_AUTO_POS) {
                    g_MovieWaterInfo[uiVEncOutPortId].uiXPos = g_MovieStampPos[uiVEncOutPortId].uiX;
                    g_MovieWaterInfo[uiVEncOutPortId].uiYPos = g_MovieStampPos[uiVEncOutPortId].uiY + pStampInfo->ui32DstHeight - g_MovieWaterInfo[uiVEncOutPortId].uiHeight;
                    g_MovieStampPos[uiVEncOutPortId].uiX += (g_MovieWaterInfo[uiVEncOutPortId].uiWidth + STAMP_LOGO_GAP);
                }
            }
#endif
            break;

        case STAMP_BOTTOM_RIGHT:
        default:
#if 0
            if ((uiFlag & STAMP_POS_END_MASK) == STAMP_POS_END) {
			g_MovieStampPos[uiVEncOutPortId].uiX = uiImageWidth - uiStampWidth * strlen(pStampInfo->pi8Str) - STAMP_WIDTH_TOLERANCE;
			g_MovieStampPos[uiVEncOutPortId].uiY = uiImageHeight - pStampInfo->ui32DstHeight;
            } else {
			g_MovieStampPos[uiVEncOutPortId].uiX = uiImageWidth - uiStampWidth * (strlen(pStampInfo->pi8Str) + 1); // 1 font width gap
			g_MovieStampPos[uiVEncOutPortId].uiY = uiImageHeight - (pStampInfo->ui32DstHeight * 3) / 2; // 1/2 font height gap
            }
#else
            g_MovieStampPos[uiVEncOutPortId].uiX = ALIGN_FLOOR(uiImageWidth - (pStampInfo->ui32FontWidth * strlen(pStampInfo->pi8Str)) - pStampInfo->ui32FontHeight, 2);
            g_MovieStampPos[uiVEncOutPortId].uiY = ALIGN_FLOOR(uiImageHeight - (pStampInfo->ui32FontHeight) * 2, 2);
#endif



#if defined (WATERLOGO_FUNCTION) && (WATERLOGO_FUNCTION == ENABLE)
		if (g_bWaterLogoEnable[uiVEncOutPortId]) {
			if (g_MovieWaterInfo[uiVEncOutPortId].uiXPos == WATERLOGO_AUTO_POS && g_MovieWaterInfo[uiVEncOutPortId].uiYPos == WATERLOGO_AUTO_POS) {
				g_MovieWaterInfo[uiVEncOutPortId].uiXPos = g_MovieStampPos[uiVEncOutPortId].uiX - g_MovieWaterInfo[uiVEncOutPortId].uiWidth - STAMP_LOGO_GAP;
				g_MovieWaterInfo[uiVEncOutPortId].uiYPos = g_MovieStampPos[uiVEncOutPortId].uiY + pStampInfo->ui32DstHeight - g_MovieWaterInfo[uiVEncOutPortId].uiHeight;
			}
		}
            //g_MovieWaterInfo[uiVEncOutPortId].uiYPos = g_MovieStampPos[uiVEncOutPortId].uiY + pStampInfo->ui32DstHeight - g_MovieWaterInfo[uiVEncOutPortId].uiHeight;
#endif
            break;
        }
    }

	g_MovieStampPos[uiVEncOutPortId].uiX = ALIGN_FLOOR_4(g_MovieStampPos[uiVEncOutPortId].uiX);
	g_MovieStampPos[uiVEncOutPortId].uiY = ALIGN_FLOOR_4(g_MovieStampPos[uiVEncOutPortId].uiY);

#if defined (WATERLOGO_FUNCTION) && (WATERLOGO_FUNCTION == ENABLE)
	if (g_bWaterLogoEnable[uiVEncOutPortId]) {
		g_MovieWaterInfo[uiVEncOutPortId].uiXPos = ALIGN_FLOOR_4(g_MovieWaterInfo[uiVEncOutPortId].uiXPos);
		g_MovieWaterInfo[uiVEncOutPortId].uiYPos = ALIGN_FLOOR_4(g_MovieWaterInfo[uiVEncOutPortId].uiYPos);
	}
#endif

    //printf("%s[%d]---------------->font x*y[%d*%d]\n",__func__,__LINE__,g_MovieStampPos[uiVEncOutPortId].uiX,g_MovieStampPos[uiVEncOutPortId].uiY);
    //printf("%s[%d]---------------->logo x*y[%d*%d]\n",__func__,__LINE__,g_MovieWaterInfo[uiVEncOutPortId].uiXPos,g_MovieWaterInfo[uiVEncOutPortId].uiYPos);


	// Reset reference time
	g_CurDateTime.tm_sec = 0;//61;

	ISIZE szStamp;

#if defined(_UI_STYLE_LVGL_)
	szStamp=MovieStamp_GetStampDataWidth_LVGL(pStampInfo, (const lv_font_t *)g_VsFontIn[uiVEncOutPortId].pFont);
	g_VsOsdWidth[uiVEncOutPortId] = szStamp.w;
	g_VsOsdHight[uiVEncOutPortId] = szStamp.h;
#else
	szStamp=MovieStamp_GetStampDataWidth(pStampInfo, (const IMAGE_TABLE *)g_VsFontIn[uiVEncOutPortId].pFont);
	g_VsOsdWidth[uiVEncOutPortId]=ALIGN_CEIL(szStamp.w ,8);
	g_VsOsdHight[uiVEncOutPortId]=ALIGN_CEIL(szStamp.h, 2);
#endif
	//DBG_ERR("VsOsdWidth[%d]=%d, %d\r\n",uiVidEncId,g_VsOsdWidth[uiVidEncId], g_VsOsdHight[uiVidEncId] );

	MovieStamp_VsConfig(uiVEncOutPortId, uiImageWidth, STAMP_HEIGHT_MAX,pWaterLogoBuf); //from MovieStamp_CalcBufSize(), uiImageWidth may be 2560
	MovieStamp_VsAllocWaterLogoOsgBuf(uiVEncOutPortId, g_MovieWaterInfo[uiVEncOutPortId].uiWidth, g_MovieWaterInfo[uiVEncOutPortId].uiHeight);
	MovieStamp_VsFontConfig(uiVEncOutPortId);

	MovieStamp_VsAllocFontBuf(uiVEncOutPortId, uiImageWidth, STAMP_HEIGHT_MAX);

#if defined (MOVIE_MULTISTAMP_FUNC) && (MOVIE_MULTISTAMP_FUNC == ENABLE)
        // set main path 2nd movie stamp
	{
	    g_MovieGpsStampPos[uiVEncOutPortId].x = uiStampWidth; // 1 font width gap
	    g_MovieGpsStampPos[uiVEncOutPortId].y = uiImageHeight - (pStampInfo->ui32DstHeight * 3) / 2; // 1/2 font height gap

	    char Name[32] = {"120    121.0000 30.0000"};
	    MovieStamp_DrawMultiStamp(uiVEncOutPortId, 0, &g_MovieGpsStampPos[uiVEncOutPortId], Name, TRUE);
	}
#endif
#if defined (MOVIE_MULTIWATERLOGO_FUNC) && (MOVIE_MULTIWATERLOGO_FUNC == ENABLE)
	{
		UINT32 j;
		for(j=0;j<MOVIE_MULTI_WATERLOGO_CNT_MAX;j++){
			MovieStamp_GetMultiWaterLogoSource(j, &g_sMultiWaterLogo[uiVEncOutPortId][j]);
			MovieStamp_DrawMultiWaterLogo(uiVEncOutPortId, j, &g_sMultiWaterLogo[uiVEncOutPortId][j], TRUE);
		}
	}
#endif
}


void MovieStamp_SetColor(UINT32 uiVEncOutPortId, PSTAMP_COLOR pStampColorBg, PSTAMP_COLOR pStampColorFr, PSTAMP_COLOR pStampColorFg)
{

	// Stamp background color
	g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_BG].ucY = pStampColorBg->ucY;
	g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_BG].ucU = pStampColorBg->ucU;
	g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_BG].ucV = pStampColorBg->ucV;

	// Stamp frame color
	g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_FR].ucY = pStampColorFr->ucY;
	g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_FR].ucU = pStampColorFr->ucU;
	g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_FR].ucV = pStampColorFr->ucV;

	// Stamp foreground color (text body)
	g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_FG].ucY = pStampColorFg->ucY;
	g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_FG].ucU = pStampColorFg->ucU;
	g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_FG].ucV = pStampColorFg->ucV;

}

UINT32 MovieStamp_GetBufAddr(UINT32 uiVEncOutPortId, UINT32 blk_size)
{
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	CHAR pool_name[20] ={0};
	UINT32 path_id=uiVEncOutPortId;
	if(g_MovieStampPoolAddr[path_id].pool_va == 0)  {

		sprintf(pool_name,"MovieStamp_%d",(int)path_id);

		ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, blk_size, ddr_id);
		if (ret != HD_OK) {
			DBG_ERR("alloc fail size 0x%x, ddr %d\r\n", blk_size, ddr_id);
			return 0;
		}
		DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
		g_MovieStampPoolAddr[path_id].pool_va=(UINT32)va;
		g_MovieStampPoolAddr[path_id].pool_pa=(UINT32)pa;
		memset(va, 0, blk_size);
	}

	if(g_MovieStampPoolAddr[path_id].pool_va == 0)
		DBG_ERR("get buf addr err\r\n");
	return g_MovieStampPoolAddr[path_id].pool_va;

}
void MovieStamp_DestroyBuff(void)
{
	UINT32 i, ret;
	for (i=0;i<VIDEO_IN_MAX;i++) {

		g_uiVsStampPa[i]=0;
		g_uiVsStampSize[i]=0;

		if (g_MovieStampPoolAddr[i].pool_va != 0) {
			ret = hd_common_mem_free((UINT32)g_MovieStampPoolAddr[i].pool_pa, (void *)g_MovieStampPoolAddr[i].pool_va);
			if (ret != HD_OK) {
				DBG_ERR("FileIn release blk failed! (%d)\r\n", ret);
				break;
			}
			g_MovieStampPoolAddr[i].pool_va = 0;
			g_MovieStampPoolAddr[i].pool_pa = 0;
		}
	}
#if(MOVIE_MULTISTAMP_FUNC == ENABLE)
	MovieStamp_DestroyMultiStampBuff();
#endif
#if defined (MOVIE_MULTIWATERLOGO_FUNC) && (MOVIE_MULTIWATERLOGO_FUNC == ENABLE)
	MovieStamp_DestroyMultiWaterLogoBuff();
#endif
}
void MovieStamp_SetBuffer(UINT32 uiVEncOutPortId, UINT32 uiAddr, UINT32 uiSize, UINT32 Width, UINT32 Height)
{
	UINT32 path_id=uiVEncOutPortId;
    UINT32 waterLogoBufSize = ALIGN_CEIL_8(Width*Height*4);
	g_uiWaterLogoYAddr[path_id]     = uiAddr;
	g_uiWaterLogoUVAddr[path_id]    = g_uiWaterLogoYAddr[path_id] + waterLogoBufSize / 2; // YUV422, UV-packed


	g_uiMovieStampYAddr[path_id][0] = uiAddr + waterLogoBufSize;
	g_uiMovieStampYAddr[path_id][1] = g_uiMovieStampYAddr[path_id][0] + (uiAddr + uiSize - g_uiMovieStampYAddr[path_id][0]) / 2;

    	g_uiVsStampAddr[path_id]=g_uiMovieStampYAddr[path_id][0];
    	g_uiVsFontAddr[path_id]=g_uiVsStampAddr[path_id] + (uiSize-(waterLogoBufSize ))*OSG_BUF_RATIO/(OSG_BUF_RATIO+VSFONT_BUF_RATIO);
}
UINT32 MovieStamp_OsgQueryBufSize(UINT32 Width, UINT32 Height)
{
	HD_VIDEO_FRAME frame = {0};
	UINT32 stamp_size;

	frame.sign   = MAKEFOURCC('O','S','G','P');
			frame.dim.w  = Width;
			frame.dim.h  = Height;
	frame.pxlfmt = HD_VIDEO_PXLFMT_ARGB4444;

	//get required buffer size for a single image
	stamp_size = hd_common_mem_calc_buf_size(&frame);
	if(!stamp_size){
		DBG_ERR("fail to query buffer size\n");
		return -1;
	}

	//ping pong buffer needs double size
	stamp_size *= 2;
	//DBG_DUMP("Width=%dx%d, stamp_size=%d\n",Width, Height, stamp_size);

	return stamp_size;
}

UINT32 MovieStamp_CalcBufSize(UINT32 Width, UINT32 Height, WATERLOGO_BUFFER *pWaterLogoBuf)
{
    UINT32 BufSize=0;
    UINT32 FontHeight=0;
    UINT32 waterLogoBufSize = pWaterLogoBuf ? ALIGN_CEIL_8(pWaterLogoBuf->uiWidth*pWaterLogoBuf->uiHeight*4) : 0;

    FontHeight =STAMP_HEIGHT_MAX;
    BufSize = waterLogoBufSize +
	         MovieStamp_OsgQueryBufSize(Width, FontHeight) +
	         Width*FontHeight*VSFONT_BUF_RATIO/100;
	//DBG_DUMP("Width=%d, FontHeight=%d, BufSize=%d\n",Width,FontHeight,BufSize);

	return BufSize;
}
void MovieStamp_UpdateData(void)
{
	UINT32          i;
#if (MOVIE_ISP_LOG == 0)
	struct tm       CurDateTime;

	CurDateTime = g_CurDateTime;
#endif
	ER FontConvRet=0;
	for (i = 0; i < VIDEO_IN_MAX; i++) {
		if ((g_uiMovieStampSetup[i] & STAMP_SWITCH_MASK) == STAMP_ON) {

#if (MOVIE_ISP_LOG == 0)

			// Prepare date-time string
			if ((g_uiMovieStampSetup[i] & STAMP_DATE_TIME_MASK) == STAMP_DATE_TIME) {
				switch (g_uiMovieStampSetup[i] & STAMP_DATE_FORMAT_MASK) {
				case STAMP_DD_MM_YY:
					//sprintf(&g_cMovieStampStr[i][0], "%02d/%02d/%04d %02d:%02d:%02d", CurDateTime.tm_mday, CurDateTime.tm_mon, CurDateTime.tm_year, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
					snprintf(&g_cMovieStampStr[i][0], MOVIE_STAMP_MAX_LEN, "%02d/%02d/%04d %02d:%02d:%02d", CurDateTime.tm_mday, CurDateTime.tm_mon, CurDateTime.tm_year, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
					break;
				case STAMP_MM_DD_YY:
					//sprintf(&g_cMovieStampStr[i][0], "%02d/%02d/%04d %02d:%02d:%02d", CurDateTime.tm_mon, CurDateTime.tm_mday, CurDateTime.tm_year, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
					snprintf(&g_cMovieStampStr[i][0], MOVIE_STAMP_MAX_LEN, "%02d/%02d/%04d %02d:%02d:%02d", CurDateTime.tm_mon, CurDateTime.tm_mday, CurDateTime.tm_year, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
					break;
				default:
					//sprintf(&g_cMovieStampStr[i][0], "%04d/%02d/%02d %02d:%02d:%02d", CurDateTime.tm_year, CurDateTime.tm_mon, CurDateTime.tm_mday, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
					snprintf(&g_cMovieStampStr[i][0], MOVIE_STAMP_MAX_LEN, "%04d/%02d/%02d %02d:%02d:%02d", CurDateTime.tm_year, CurDateTime.tm_mon, CurDateTime.tm_mday, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
					break;
				}
			} else if ((g_uiMovieStampSetup[i] & STAMP_DATE_TIME_MASK) == STAMP_DATE_TIME_AMPM) {
				if (CurDateTime.tm_hour >= 12) {
					if (CurDateTime.tm_hour > 12) {
						CurDateTime.tm_hour -= 12;
					}

					switch (g_uiMovieStampSetup[i] & STAMP_DATE_FORMAT_MASK) {
					case STAMP_DD_MM_YY:
						//sprintf(&g_cMovieStampStr[i][0], "%02d/%02d/%04d %02d:%02d:%02d PM", CurDateTime.tm_mday, CurDateTime.tm_mon, CurDateTime.tm_year, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
						snprintf(&g_cMovieStampStr[i][0], MOVIE_STAMP_MAX_LEN, "%02d/%02d/%04d %02d:%02d:%02d PM", CurDateTime.tm_mday, CurDateTime.tm_mon, CurDateTime.tm_year, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
						break;
					case STAMP_MM_DD_YY:
						//sprintf(&g_cMovieStampStr[i][0], "%02d/%02d/%04d %02d:%02d:%02d PM", CurDateTime.tm_mon, CurDateTime.tm_mday, CurDateTime.tm_year, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
						snprintf(&g_cMovieStampStr[i][0], MOVIE_STAMP_MAX_LEN, "%02d/%02d/%04d %02d:%02d:%02d PM", CurDateTime.tm_mon, CurDateTime.tm_mday, CurDateTime.tm_year, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
						break;
					default:
						//sprintf(&g_cMovieStampStr[i][0], "%04d/%02d/%02d %02d:%02d:%02d PM", CurDateTime.tm_year, CurDateTime.tm_mon, CurDateTime.tm_mday, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
						snprintf(&g_cMovieStampStr[i][0], MOVIE_STAMP_MAX_LEN, "%04d/%02d/%02d %02d:%02d:%02d PM", CurDateTime.tm_year, CurDateTime.tm_mon, CurDateTime.tm_mday, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
						break;
					}
				} else {
					switch (g_uiMovieStampSetup[i] & STAMP_DATE_FORMAT_MASK) {
					case STAMP_DD_MM_YY:
						//sprintf(&g_cMovieStampStr[i][0], "%02d/%02d/%04d %02d:%02d:%02d AM", CurDateTime.tm_mday, CurDateTime.tm_mon, CurDateTime.tm_year, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
						snprintf(&g_cMovieStampStr[i][0], MOVIE_STAMP_MAX_LEN, "%02d/%02d/%04d %02d:%02d:%02d AM", CurDateTime.tm_mday, CurDateTime.tm_mon, CurDateTime.tm_year, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
						break;
					case STAMP_MM_DD_YY:
						//sprintf(&g_cMovieStampStr[i][0], "%02d/%02d/%04d %02d:%02d:%02d AM", CurDateTime.tm_mon, CurDateTime.tm_mday, CurDateTime.tm_year, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
						snprintf(&g_cMovieStampStr[i][0], MOVIE_STAMP_MAX_LEN, "%02d/%02d/%04d %02d:%02d:%02d AM", CurDateTime.tm_mon, CurDateTime.tm_mday, CurDateTime.tm_year, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
						break;
					default:
						//sprintf(&g_cMovieStampStr[i][0], "%04d/%02d/%02d %02d:%02d:%02d AM", CurDateTime.tm_year, CurDateTime.tm_mon, CurDateTime.tm_mday, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
						snprintf(&g_cMovieStampStr[i][0], MOVIE_STAMP_MAX_LEN, "%04d/%02d/%02d %02d:%02d:%02d AM", CurDateTime.tm_year, CurDateTime.tm_mon, CurDateTime.tm_mday, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
						break;
					}
				}
			} else {
				//sprintf(&g_cMovieStampStr[i][0], "%02d:%02d:%02d", CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
				snprintf(&g_cMovieStampStr[i][0], MOVIE_STAMP_MAX_LEN, "%02d:%02d:%02d", CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
			}
#endif

			#if MOVIE_ISP_LOG
			UINT16 Ipl_id=0;
			Ipl_id=ImageApp_MovieMulti_VePort2Imglink(i);
			//DBG_DUMP("update Ipl_id=%d\r\n",Ipl_id);
			MovieStamp_get_isp_status(Ipl_id, &g_cMovieStampStr[i][0], 256);
			#endif

			g_VsFontIn[i].pStr=g_MovieStampInfo[i].pi8Str;


			if(MovieStamp_IsRawEncVirPort(i) && g_VsStampStart[i][(VSSTAMP_DATE_HD_STAMP_ID-HD_STAMP_BASE)]){
				//DBG_DUMP("Update i=%d, start=0x%x, return\r\n",i, g_VsStampStart[i][(HD_STAMP_0-HD_STAMP_BASE)]);
				continue;
			}


#if defined(_UI_STYLE_LVGL_)
			if ((FontConvRet=FontConv_LVGL(&g_VsFontIn[i], &g_VsFontOut[i])) != E_OK) {
				DBG_ERR("FontConv_LVGL err, Ret=%d\r\n",FontConvRet);
				return;
			}
#else
			if ((FontConvRet=FontConv(&g_VsFontIn[i], &g_VsFontOut[i])) != E_OK) {
				DBG_ERR("FontConv err, Ret=%d\r\n",FontConvRet);
				return;
			}
#endif
			//hd_common_mem_flush_cache((void*)g_VsFontOut[i].GenImg.phy_addr[0], g_VsFontOut[i].GenImg.loff[0]* g_VsFontOut[i].GenImg.ph[0]);
			//g_pVsFontDataAddr[i]=(UINT16 *)g_VsFontOut[i].GenImg.PxlAddr[0];
			g_pVsFontDataAddr[i]=g_VsFontOut[i].GenImg.phy_addr[0];
			//DBG_ERR("update pi8Str[%d]=%s\r\n", i,g_MovieStampInfo[i].pi8Str);

			g_VsOsdWidth[i]=g_VsFontOut[i].GenImg.pw[0]/2;
			g_VsOsdHight[i]=g_VsFontOut[i].GenImg.ph[0];
			//DBG_DUMP("i=%d, w=%d, %d, %d\r\n", i,g_VsOsdWidth[i], g_VsOsdHight[i],g_VsFontOut[i].GenImg.loff[0]);
			//MovieStamp_VsUpdateOsd(i, TRUE, 1, g_MovieStampPos[i].uiX, g_MovieStampPos[i].uiY, g_VsOsdWidth[i], g_VsOsdHight[i], (void*)g_pVsFontDataAddr[i]);
			UINT32 SoutEn = MOVIE_IMGCAP_SOUT_NONE;
			if(MovieStamp_IsRawEncVirPort(i)){
		            HD_PATH_ID      uiVEncOutPortId = HD_GET_OUT(ImageApp_MovieMulti_GetRawEncPort(_CFG_REC_ID_1)) - 1;
		            UINT32 rec_id= _CFG_REC_ID_1 + i- uiVEncOutPortId;
		            DBG_DUMP("[%d]rec_id=%d\r\n",i, rec_id);
		            if(ImageApp_MovieMulti_GetParam(rec_id, MOVIEMULTI_PARAM_IMGCAP_SOUT_BUF_TYPE, &SoutEn)==E_OK){
		                if(SoutEn!= MOVIE_IMGCAP_SOUT_NONE){
			                g_pVsFontDataAddr[i]=(UINT32)&g_ucStamp_Null[0];
			                g_VsOsdWidth[i]=8;
			                g_VsOsdHight[i]=8;
			                DBG_DUMP("[%d]SoutEn=%d\r\n",i, SoutEn);
		                }else{
			                DBG_DUMP("[%d]MOVIE_IMGCAP_SOUT_NONE\r\n",i);
		                }
		            }
			}
			// update 1st stamp
			if(MovieStamp_VsUpdateOsd(g_VEncHDPathId[i], TRUE, MOVIE_STAMP_LAYER1, VS_DATESTAMP_REGION, g_MovieStampPos[i].uiX, g_MovieStampPos[i].uiY, g_VsOsdWidth[i], g_VsOsdHight[i], (void*)g_pVsFontDataAddr[i])){
				MovieStamp_VsSwapOsd(i, (VSSTAMP_DATE_HD_STAMP_ID-HD_STAMP_BASE),g_VEncHDPathId[i]);
			}else{
				MovieStamp_VsStop(i, (VSSTAMP_DATE_HD_STAMP_ID-HD_STAMP_BASE));
			}

			// update waterlogo
#if defined (WATERLOGO_FUNCTION) && (WATERLOGO_FUNCTION == ENABLE)
			if (g_bWaterLogoEnable[i]) {
				DBG_IND("water Width=%dx%d\r\n",g_MovieWaterInfo[i].uiWidth, g_MovieWaterInfo[i].uiHeight);
				if(MovieStamp_IsRawEncVirPort(i)){
					if(SoutEn!= MOVIE_IMGCAP_SOUT_NONE){
					    g_MovieWaterInfo[i].uiWaterLogoAddr=(UINT32)&g_ucStamp_Null[0];
					    g_MovieWaterInfo[i].uiWidth=8;
					    g_MovieWaterInfo[i].uiHeight=8;
					}
				}

				//MovieStamp_VsUpdateOsd(i, TRUE, 2, g_MovieWaterInfo[i].uiXPos, g_MovieWaterInfo[i].uiYPos, g_MovieWaterInfo[i].uiWidth, g_MovieWaterInfo[i].uiHeight, (void*)g_MovieWaterInfo[i].uiWaterLogoYAddr);
				if(MovieStamp_VsUpdateOsd(g_WaterlogoVEncHDPathId[i], TRUE, MOVIE_STAMP_LAYER1, VS_WATERLOGO_REGION, g_MovieWaterInfo[i].uiXPos, g_MovieWaterInfo[i].uiYPos, g_MovieWaterInfo[i].uiWidth, g_MovieWaterInfo[i].uiHeight, (void*)g_MovieWaterInfo[i].uiWaterLogoAddr)){
					MovieStamp_VsSwapOsd(i, (VSSTAMP_WATERLOGO_HD_STAMP_ID-HD_STAMP_BASE),g_WaterlogoVEncHDPathId[i]);
				}else{
					MovieStamp_VsStop(i, (VSSTAMP_WATERLOGO_HD_STAMP_ID-HD_STAMP_BASE));
				}
			}
#endif
#if defined (MOVIE_MULTISTAMP_FUNC) && (MOVIE_MULTISTAMP_FUNC == ENABLE)
			// update 2nd stamp
            {
                // follows are test codes, please update them by your GPS data
                char Name[32] = {"120    121.0000 30.0000"};

                MovieStamp_DrawMultiStamp(i, 0, &g_MovieGpsStampPos[i], Name , FALSE);
            }
#endif
#if defined (MOVIE_MULTIWATERLOGO_FUNC) && (MOVIE_MULTIWATERLOGO_FUNC == ENABLE)
		{
			UINT32 j;
			for(j=0;j<MOVIE_MULTI_WATERLOGO_CNT_MAX;j++){
				MovieStamp_DrawMultiWaterLogo(i, j, &g_sMultiWaterLogo[i][j], FALSE);
			}

		}
#endif

		}
	}
}


void MovieStamp_VsConfigOsd(HD_OSG_STAMP_IMG *image, HD_OSG_STAMP_ATTR *attr, HD_PATH_ID path_id, UINT32 layer, UINT32 region, UINT32 enable, UINT32 x, UINT32 y, UINT32 w, UINT32 h, void *data)
{
	UINT32 Result=0;

	memset(image, 0, sizeof(HD_OSG_STAMP_IMG));
	image->fmt     = HD_VIDEO_PXLFMT_ARGB4444;
	image->dim.w    = w;
	image->dim.h         = h;
	image->p_addr         = (UINT32)data;
	Result=hd_videoenc_set(path_id, HD_VIDEOENC_PARAM_IN_STAMP_IMG, image);
	if(Result != HD_OK){
		DBG_ERR("fail to set stamp image, Result=%d\n",Result);
		return;
	}

	memset(attr, 0, sizeof(HD_OSG_STAMP_ATTR));
	attr->layer     = layer;
	attr->region    = region;
	attr->position.x         = x;
	attr->position.y         = y;
	attr->alpha     = 255;
	Result=hd_videoenc_set(path_id, HD_VIDEOENC_PARAM_IN_STAMP_ATTR, attr);
	if(Result != HD_OK){
		DBG_ERR("fail to set stamp attr, path_id=0x%x, Result=%d\n",path_id,Result);
		return;
	}
}
ER MovieStamp_VsAllocWaterLogoOsgBuf(UINT32 uiVEncOutPortId, UINT32 WaterLogoWidth, UINT32 WaterLogoHeight)
{
#if defined (WATERLOGO_FUNCTION) && (WATERLOGO_FUNCTION == ENABLE)
	UINT32 Result=0;
	HD_OSG_STAMP_BUF  buf;
	UINT32 uiOsgSize;
	UINT32 uiOsgPa=0;
	if (g_bWaterLogoEnable[uiVEncOutPortId]) {

		DBG_IND("Water %d, height=%d\r\n", WaterLogoWidth, WaterLogoHeight);
		uiOsgSize = MovieStamp_OsgQueryBufSize(WaterLogoWidth, WaterLogoHeight);

		uiOsgPa=g_MovieStampPoolAddr[uiVEncOutPortId].pool_pa+g_uiWaterLogoYAddr[uiVEncOutPortId]-g_MovieStampPoolAddr[uiVEncOutPortId].pool_va;
		if(uiOsgPa==0){
			DBG_ERR("uiVsStampPa fail\n");
			return -1;
		}
		memset(&buf, 0, sizeof(HD_OSG_STAMP_BUF));

		buf.type      = HD_OSG_BUF_TYPE_PING_PONG;
		buf.p_addr    = uiOsgPa;
		buf.size      = uiOsgSize;
		Result=hd_videoenc_set(g_WaterlogoVEncHDPathId[uiVEncOutPortId], HD_VIDEOENC_PARAM_IN_STAMP_BUF, &buf);
		if(Result != HD_OK){
			DBG_ERR("waterlogo fail to set stamp buffer, [%d]Result=%d\n",uiVEncOutPortId,Result);
			return -1;
		}
	}
#endif
	return 0;
}
ER MovieStamp_VsAllocOsdBuf(UINT32 uiVEncOutPortId, UINT32 width, UINT32 height)
{
	UINT32 Result=0;
	HD_OSG_STAMP_BUF  buf;
	if(MovieStamp_IsRawEncVirPort(uiVEncOutPortId) && (uiVEncOutPortId!=(HD_GET_OUT(ImageApp_MovieMulti_GetRawEncPort(_CFG_REC_ID_1)) - 1))){
		return Result;
	}
	//DBG_DUMP("path_id=0x%x, width=%d, height=%d\r\n",path_id, width, height);
	g_uiVsStampSize[uiVEncOutPortId] = MovieStamp_OsgQueryBufSize(width, height);

	//vir_meminfo.va = (void *)((UINT32)g_uiVsStampAddr[uiVidEncId]);
	//if (hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo)== HD_OK) {
	//	g_uiVsStampPa=vir_meminfo.pa;
	//}
	g_uiVsStampPa[uiVEncOutPortId]=g_MovieStampPoolAddr[uiVEncOutPortId].pool_pa+g_uiVsStampAddr[uiVEncOutPortId]-g_MovieStampPoolAddr[uiVEncOutPortId].pool_va;
	if(g_uiVsStampPa[uiVEncOutPortId]==0){
		DBG_ERR("g_uiVsStampPa fail\n");
		return -1;
	}
	memset(&buf, 0, sizeof(HD_OSG_STAMP_BUF));

	buf.type      = HD_OSG_BUF_TYPE_PING_PONG;
	buf.p_addr    = g_uiVsStampPa[uiVEncOutPortId];
	buf.size      = g_uiVsStampSize[uiVEncOutPortId];
	Result=hd_videoenc_set(g_VEncHDPathId[uiVEncOutPortId], HD_VIDEOENC_PARAM_IN_STAMP_BUF, &buf);
	if(Result != HD_OK){
		DBG_ERR("fail to set stamp buffer, Result=%d, PortId=0x%x, HDPathId=0x%x\n",Result,uiVEncOutPortId,g_VEncHDPathId[uiVEncOutPortId]);
		return -1;
	}
	return Result;
}

ER MovieStamp_VsConfig(UINT32 uiVEncOutPortId, UINT32 uiOSDWidth, UINT32 uiOSDHeight,WATERLOGO_BUFFER *pWaterLogoBuf)
{
	g_VsStampEn =TRUE;
	HD_RESULT ret =-1;
	MOVIE_CFG_REC_ID rec_id=_CFG_REC_ID_1;
	UINT32 i;
	UINT32 is_normal_enc=0;
	UINT32 is_raw_enc=0;

	//_CFG_REC_ID_1, VencPort=1, main
	//date(HD_STAMP_0)
	//waterlogo(HD_STAMP_1)
	//multistamp1(HD_STAMP_2), multistamp2 (HD_STAMP_3), multistamp3 (HD_STAMP_4), multistamp4 (HD_STAMP_5), multistamp5 (HD_STAMP_6)
	//multiewaterlogo1(HD_STAMP_7), multiwaterlogo2 (HD_STAMP_8), multiwaterlogo3 (HD_STAMP_9), multiwaterlogo4 (HD_STAMP_10), multiwaterlogo5 (HD_STAMP_11)

	//_CFG_REC_ID_1, VencPort=4, raw enc
	//date(HD_STAMP_0)
	//waterlogo(HD_STAMP_1)
	//multistamp1(HD_STAMP_2), multistamp2 (HD_STAMP_3), multistamp3 (HD_STAMP_4), multistamp4 (HD_STAMP_5), multistamp5 (HD_STAMP_6)
	//multiewaterlogo1(HD_STAMP_7), multiwaterlogo2 (HD_STAMP_8), multiwaterlogo3 (HD_STAMP_9), multiwaterlogo4 (HD_STAMP_10), multiwaterlogo5 (HD_STAMP_11)

	//UINT32 path_id_exe;//=ImageApp_MovieMulti_GetVdoEncPort(0);
	//DBG_DUMP("MovieMulti_GetVdoEncPort=0x%x, 0x%x\r\n",ImageApp_MovieMulti_GetVdoEncPort(0),HD_VIDEOENC_0_IN_0);
	for(i=0;i<(SENSOR_CAPS_COUNT & SENSOR_ON_MASK);i++){
		if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_MOVIE || System_GetState(SYS_STATE_NEXTMODE) == PRIMARY_MODE_MOVIE) {
			if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetVdoEncPort(_CFG_REC_ID_1+i)) - 1)){
				rec_id=_CFG_REC_ID_1+i;
				is_normal_enc=1;
				break;
			}
			if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetVdoEncPort(_CFG_CLONE_ID_1+i)) - 1)){
				rec_id=_CFG_CLONE_ID_1+i;
				is_normal_enc=1;
				break;
			}
			if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetVdoEncPort(_CFG_STRM_ID_1+i)) - 1)){
				rec_id=_CFG_STRM_ID_1+i;
				is_normal_enc=1;
				break;
			}
		}else if(System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PHOTO || System_GetState(SYS_STATE_NEXTMODE) == PRIMARY_MODE_PHOTO){
			break;
		}else{
			DBG_ERR("not support mode %d\r\n",System_GetState(SYS_STATE_CURRMODE));
			return ret;
		}

	#if(defined(_NVT_ETHREARCAM_RX_) && (ETHCAM_EIS ==ENABLE))
	for (i = 0; i < ETH_REARCAM_CAPS_COUNT; i ++) {
		if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetVdoEncPort(_CFG_ETHCAM_ID_1+i)) - 1)){
			rec_id=_CFG_ETHCAM_ID_1+i;
			is_normal_enc=1;
			break;
		}
	}
	#endif
	}
	//DBG_DUMP("rec_id=%d, is_normal_enc=%d\r\n",rec_id,is_normal_enc);

	if((System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_MOVIE || System_GetState(SYS_STATE_NEXTMODE) == PRIMARY_MODE_MOVIE) && is_normal_enc && (ret = hd_videoenc_open(ImageApp_MovieMulti_GetVdoEncPort(rec_id), VSSTAMP_DATE_HD_STAMP_ID, &g_VEncHDPathId[uiVEncOutPortId])) != HD_OK){
		DBG_ERR("movie hd_videoenc_open fail uiVEncOutPortId=%d, ret=%d, GetVdoEncPort=%d\r\n",uiVEncOutPortId,ret,ImageApp_MovieMulti_GetVdoEncPort(rec_id));
		return ret;
	}else if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PHOTO || System_GetState(SYS_STATE_NEXTMODE) == PRIMARY_MODE_PHOTO){
		PHOTO_STRM_INFO *p_strm_info = (PHOTO_STRM_INFO*)ImageApp_Photo_GetConfig(PHOTO_CFG_STRM_INFO, 0);
		if((ret = hd_videoenc_open(p_strm_info->venc_p[0], VSSTAMP_DATE_HD_STAMP_ID, &g_VEncHDPathId[uiVEncOutPortId])) != HD_OK){
			DBG_ERR("photo hd_videoenc_open fail uiVEncOutPortId=%d, ret=%d, GetVdoEncPort=%d\r\n",uiVEncOutPortId,ret,p_strm_info->venc_p[0]);
			return ret;
		}
	}
	#if defined (WATERLOGO_FUNCTION) && (WATERLOGO_FUNCTION == ENABLE)
	if (g_bWaterLogoEnable[uiVEncOutPortId]) {
		if(is_normal_enc && (ret = hd_videoenc_open(ImageApp_MovieMulti_GetVdoEncPort(rec_id), VSSTAMP_WATERLOGO_HD_STAMP_ID, &g_WaterlogoVEncHDPathId[uiVEncOutPortId])) != HD_OK){
			DBG_ERR("waterlogo hd_videoenc_open fail uiVEncOutPortId=%d, ret=%d, GetVdoEncPort=%d\r\n",uiVEncOutPortId,ret,ImageApp_MovieMulti_GetVdoEncPort(rec_id));
			return ret;
		}
	}
	#endif

	for(i=0;i<SENSOR_CAPS_COUNT;i++){
		//raw enc
		if(MovieStamp_IsRawEncVirPort(uiVEncOutPortId)){
			rec_id=_CFG_REC_ID_1+i;
			is_raw_enc=1;
			break;
		}
		#if 0
		//raw enc
		if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetRawEncPort(_CFG_CLONE_ID_1+i)) - 1)){
			rec_id=_CFG_CLONE_ID_1+i;
			is_raw_enc=1;
			break;
		}
		#endif
	}
	if(is_raw_enc){
		g_VsStampStart[uiVEncOutPortId][(VSSTAMP_DATE_HD_STAMP_ID-HD_STAMP_BASE)]=0;
	}
	if(is_raw_enc && (ret = hd_videoenc_open(ImageApp_MovieMulti_GetRawEncPort(rec_id), VSSTAMP_DATE_HD_STAMP_ID, &g_VEncHDPathId[uiVEncOutPortId])) != HD_OK){
		DBG_ERR("hd_videoenc_open fail uiVEncOutPortId=%d, ret=%d, GetRawEncPort=%d\r\n",uiVEncOutPortId,ret,ImageApp_MovieMulti_GetRawEncPort(rec_id));
		return ret;
	}
	#if defined (WATERLOGO_FUNCTION) && (WATERLOGO_FUNCTION == ENABLE)
	if (g_bWaterLogoEnable[uiVEncOutPortId]) {
		if(is_raw_enc && (ret = hd_videoenc_open(ImageApp_MovieMulti_GetRawEncPort(rec_id), VSSTAMP_WATERLOGO_HD_STAMP_ID, &g_WaterlogoVEncHDPathId[uiVEncOutPortId])) != HD_OK){
			DBG_ERR("waterlogo hd_videoenc_open fail uiVEncOutPortId=%d, ret=%d, GetRawEncPort=%d\r\n",uiVEncOutPortId,ret,ImageApp_MovieMulti_GetRawEncPort(rec_id));
			return ret;
		}
	}
	#endif
	//DBG_DUMP("normal_enc=%d, raw_enc=%d, PortId=0x%x, HDPathId=0x%x\r\n",is_normal_enc,is_raw_enc,uiVEncOutPortId,g_VEncHDPathId[uiVEncOutPortId]);





	if (g_VEncHDPathId[uiVEncOutPortId] && MovieStamp_VsAllocOsdBuf(uiVEncOutPortId, uiOSDWidth, uiOSDHeight)) {
	    DBG_ERR("fail to allocate buffer for BTN image uiVEncOutPortId=%d\r\n",uiVEncOutPortId);
	    return -1;
	}
	return ret;
}
INT32 MovieStamp_VsUpdateOsd(HD_PATH_ID path_id, UINT32 enable, UINT32 layer, UINT32 region, UINT32 uiX, UINT32 uiY, UINT32 uiOSDWidth, UINT32 uiOSDHeight, void *picture)
{
	HD_OSG_STAMP_IMG image;
	HD_OSG_STAMP_ATTR attr;
	//DBG_ERR("g_VsHDPathId=%d\r\n",g_VsHDPathId);
	//DBG_DUMP("uiX=%d, uiY=%d\r\n",uiX,uiY);
	MovieStamp_VsConfigOsd(&image, &attr,  path_id, layer, region, enable, uiX, uiY, uiOSDWidth, uiOSDHeight, picture);
	return enable;
}
void MovieStamp_VsFontConfig(UINT32 uiVEncOutPortId)
{
	INT32 r, g, b;
	INT32 y, u, v;
	y = g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_FG].ucY;
	u = g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_FG].ucU;
	v = g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_FG].ucV;
	YUV_GET_RGB(y, u, v, r, g, b);

#if defined(_UI_STYLE_LVGL_)
	g_VsFontIn[uiVEncOutPortId].ciSolid =  ((LV_COLOR_GET_A32((lv_color32_t){.full = LV_USER_CFG_STAMP_COLOR_TEXT}) >> 4) << 12) | ((r>>4)<<8) | ((g>>4)<<4) | (b>>4); //0xFFFF;
#else
	g_VsFontIn[uiVEncOutPortId].ciSolid = 0xF000 | ((r>>4)<<8) | ((g>>4)<<4) | (b>>4); //0xFFFF;
#endif

	y = g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_FR].ucY;
	u = g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_FR].ucU;
	v = g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_FR].ucV;
	YUV_GET_RGB(y, u, v, r, g, b);

#if defined(_UI_STYLE_LVGL_)
	g_VsFontIn[uiVEncOutPortId].ciFrame = ((LV_COLOR_GET_A32((lv_color32_t){.full = LV_USER_CFG_STAMP_COLOR_FRAME}) >> 4) << 12) | ((r>>4)<<8) | ((g>>4)<<4) | (b>>4); //0xF000;
#else
	g_VsFontIn[uiVEncOutPortId].ciFrame = 0xF000 | ((r>>4)<<8) | ((g>>4)<<4) | (b>>4); //0xF000;
#endif

	y = g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_BG].ucY;
	u = g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_BG].ucU;
	v = g_MovieStampInfo[uiVEncOutPortId].Color[COLOR_ID_BG].ucV;
	YUV_GET_RGB(y, u, v, r, g, b);

#if defined(_UI_STYLE_LVGL_)
	g_VsFontIn[uiVEncOutPortId].ciTransparet = ((LV_COLOR_GET_A32((lv_color32_t){.full = LV_USER_CFG_STAMP_COLOR_BACKGROUND}) >> 4) << 12) | ((g>>4)<<4) | (b>>4); //0x0000;
#else
	g_VsFontIn[uiVEncOutPortId].ciTransparet = ((r>>4)<<8) | ((g>>4)<<4) | (b>>4); //0x0000;
#endif

	g_VsFontIn[uiVEncOutPortId].Format = HD_VIDEO_PXLFMT_ARGB4444;//GX_IMAGE_PIXEL_FMT_ARGB4444_PACKED;
	g_VsFontIn[uiVEncOutPortId].bEnableSmooth = FALSE;
	g_VsFontIn[uiVEncOutPortId].ScaleFactor = 65536; //1.0x
}
void MovieStamp_VsAllocFontBuf(UINT32 uiVEncOutPortId, UINT32 width, UINT32 height)
{
    g_uiVsFontSize[uiVEncOutPortId] = width*height*VSFONT_BUF_RATIO/100; //5.5xWxH
     //DBG_DUMP("font vid=%d, YAddr=0x%x, size=0x%x, end=0x%x\r\n",uiVidEncId,g_uiVsFontAddr[uiVidEncId],g_uiVsFontSize, (g_uiVsFontAddr[uiVidEncId]+g_uiVsFontSize));

    g_VsFontIn[uiVEncOutPortId].MemAddr = g_uiVsFontAddr[uiVEncOutPortId];
    g_VsFontIn[uiVEncOutPortId].MemSize = g_uiVsFontSize[uiVEncOutPortId];
}

void MovieStamp_VsClose(void)
{
	HD_RESULT ret;
	UINT32 i, j;

	for (i = 0; i < SENSOR_CAPS_COUNT*(2+1)+1; i++) {
		for (j = 0; j < VSSTAMP_MAX_VIDEOENC_PATH; j++) {
			if (g_VsStampStart[i][j]) {
				if (!(g_VsStampStart[i][j] & 0x80000000)) {
					ret = hd_videoenc_stop(g_VsStampStart[i][j]);
				}
				if ((ret = hd_videoenc_close(g_VsStampStart[i][j] & ~0x80000000)) != HD_OK) {
					DBG_ERR("[%d][%d]hd_videoenc_close fail ,ret=%d\r\n", i, j, ret);
				}
				g_VsStampStart[i][j] = 0;
			}
		}
	}
	g_VsStampEn = FALSE;
}
void MovieStamp_VsStop(UINT32 uiVEncOutPortId, UINT32 StampPath)
{
	HD_RESULT ret;

	if (g_VsStampStart[uiVEncOutPortId][StampPath]) {
		if (!(g_VsStampStart[uiVEncOutPortId][StampPath] & 0x80000000)) { //never stop before
			if ((ret = hd_videoenc_stop(g_VsStampStart[uiVEncOutPortId][StampPath])) != HD_OK) {
				DBG_ERR("[%d][%d]hd_videoenc_stop fail ,ret=%d\r\n", uiVEncOutPortId, StampPath, ret);
				return;
			}
			g_VsStampStart[uiVEncOutPortId][StampPath] |= 0x80000000;//stop
		}
	}
}

void MovieStamp_EncodeStampEn(UINT32 uiVEncOutPortId, UINT32 mask, UINT32 Enable)
{
	HD_RESULT ret;
	UINT32 j;
    UINT32 uiVirPortId =MovieStamp_GetRawEncVirtualPort(uiVEncOutPortId);
	uiVEncOutPortId=HD_GET_OUT(ImageApp_MovieMulti_GetRawEncPort(uiVEncOutPortId)) - 1;
	//DBG_DUMP("====>>>uiVEncOutPortId=0x%x, mask=0x%x, en=%d, VirPortId=%d\r\n", uiVEncOutPortId, mask, Enable, uiVirPortId);

	for (j = 0; j < VSSTAMP_MAX_VIDEOENC_PATH; j++) {
		if (mask & 0x00000001) {
			if (g_VsStampStart[uiVEncOutPortId][j] && Enable == 0) {
				//DBG_DUMP("====>>>EncStamp disable: g_VsStampStart[%x][%d]=%x, en=%d\r\n", uiVEncOutPortId, j, g_VsStampStart[uiVEncOutPortId][j], Enable);
				ret = hd_videoenc_stop(g_VsStampStart[uiVEncOutPortId][j] & (~(0x80000000)));
				if (ret != HD_OK) {
					DBG_ERR("start enc stamp fail=%d\r\n", ret);
					return;
				}
				g_VsStampStart[uiVEncOutPortId][j] |= 0x80000000;//stop
			} else if(g_VsStampStart[uiVEncOutPortId][j] && Enable) {
				//DBG_DUMP("====>>>EncStamp enable: g_VsStampStart[%x][%d]=%x, en=%d\r\n", uiVEncOutPortId, j, g_VsStampStart[uiVEncOutPortId][j], Enable);
				ret = hd_videoenc_start(g_VsStampStart[uiVEncOutPortId][j] & (~(0x80000000)));
				if (ret != HD_OK) {
					DBG_ERR("start enc stamp fail VEncOutPortId=%d, ret=%d\r\n",uiVEncOutPortId, ret);
					return;
				}
				ER FontConvRet=0;
				UINT32 i=uiVirPortId;


#if defined(_UI_STYLE_LVGL_)
				if ((FontConvRet=FontConv_LVGL(&g_VsFontIn[i], &g_VsFontOut[i])) != E_OK) {
					DBG_ERR("FontConv err, Ret=%d\r\n",FontConvRet);
					return;
				}
#else
				if ((FontConvRet=FontConv(&g_VsFontIn[i], &g_VsFontOut[i])) != E_OK) {
					DBG_ERR("FontConv err, Ret=%d\r\n",FontConvRet);
					return;
				}
#endif

				g_pVsFontDataAddr[i]=g_VsFontOut[i].GenImg.phy_addr[0];
				g_VsOsdWidth[i]=g_VsFontOut[i].GenImg.pw[0]/2;
				g_VsOsdHight[i]=g_VsFontOut[i].GenImg.ph[0];

				UINT32 SoutEn = MOVIE_IMGCAP_SOUT_NONE;
				if(MovieStamp_IsRawEncVirPort(i)){
			            HD_PATH_ID      uiVEncOutPortId = HD_GET_OUT(ImageApp_MovieMulti_GetRawEncPort(_CFG_REC_ID_1)) - 1;
			            UINT32 rec_id= _CFG_REC_ID_1 + i- uiVEncOutPortId;
			            DBG_DUMP("[%d]rec_id=%d\r\n",i, rec_id);
			            if(ImageApp_MovieMulti_GetParam(rec_id, MOVIEMULTI_PARAM_IMGCAP_SOUT_BUF_TYPE, &SoutEn)==E_OK){
			                if(SoutEn!= MOVIE_IMGCAP_SOUT_NONE){
				                g_pVsFontDataAddr[i]=(UINT32)&g_ucStamp_Null[0];
				                g_VsOsdWidth[i]=8;
				                g_VsOsdHight[i]=8;
				                DBG_DUMP("[%d]SoutEn=%d\r\n",i, SoutEn);
			                }else{
				                DBG_DUMP("[%d]MOVIE_IMGCAP_SOUT_NONE\r\n",i);
			                }
			            }
				}

				if(MovieStamp_VsUpdateOsd(g_VEncHDPathId[uiVEncOutPortId], TRUE, MOVIE_STAMP_LAYER1, VS_DATESTAMP_REGION, g_MovieStampPos[i].uiX, g_MovieStampPos[i].uiY,  g_VsOsdWidth[i], g_VsOsdHight[i], (void*)g_pVsFontDataAddr[i])){
					MovieStamp_VsSwapOsd(i, (VSSTAMP_DATE_HD_STAMP_ID-HD_STAMP_BASE),g_VEncHDPathId[uiVEncOutPortId]);
				}else{
					MovieStamp_VsStop(i, (VSSTAMP_DATE_HD_STAMP_ID-HD_STAMP_BASE));
				}
#if defined (WATERLOGO_FUNCTION) && (WATERLOGO_FUNCTION == ENABLE)
				if (g_bWaterLogoEnable[i]) {
					DBG_IND("water Width=%dx%d\r\n",g_MovieWaterInfo[i].uiWidth, g_MovieWaterInfo[i].uiHeight);
					if(MovieStamp_IsRawEncVirPort(i)){
						if(SoutEn!= MOVIE_IMGCAP_SOUT_NONE){
						    g_MovieWaterInfo[i].uiWaterLogoAddr=(UINT32)&g_ucStamp_Null[0];
						    g_MovieWaterInfo[i].uiWidth=8;
						    g_MovieWaterInfo[i].uiHeight=8;
						}
					}

					//MovieStamp_VsUpdateOsd(i, TRUE, 2, g_MovieWaterInfo[i].uiXPos, g_MovieWaterInfo[i].uiYPos, g_MovieWaterInfo[i].uiWidth, g_MovieWaterInfo[i].uiHeight, (void*)g_MovieWaterInfo[i].uiWaterLogoYAddr);
					if(MovieStamp_VsUpdateOsd(g_WaterlogoVEncHDPathId[uiVEncOutPortId], TRUE, MOVIE_STAMP_LAYER1, VS_WATERLOGO_REGION, g_MovieWaterInfo[i].uiXPos, g_MovieWaterInfo[i].uiYPos, g_MovieWaterInfo[i].uiWidth, g_MovieWaterInfo[i].uiHeight, (void*)g_MovieWaterInfo[i].uiWaterLogoAddr)){
						MovieStamp_VsSwapOsd(i, (VSSTAMP_WATERLOGO_HD_STAMP_ID-HD_STAMP_BASE),g_WaterlogoVEncHDPathId[uiVEncOutPortId]);
					}else{
						MovieStamp_VsStop(i, (VSSTAMP_WATERLOGO_HD_STAMP_ID-HD_STAMP_BASE));
					}
				}
#endif
#if defined (MOVIE_MULTISTAMP_FUNC) && (MOVIE_MULTISTAMP_FUNC == ENABLE)
				// update 2nd stamp
		            {
		                // follows are test codes, please update them by your GPS data
		                char Name[32] = {"120    121.0000 30.0000"};

		                MovieStamp_DrawMultiStamp(i, 0, &g_MovieGpsStampPos[i], Name, FALSE);
		            }
#endif
#if defined (MOVIE_MULTIWATERLOGO_FUNC) && (MOVIE_MULTIWATERLOGO_FUNC == ENABLE)
				{
					UINT32 k;
					for(k=0;k<MOVIE_MULTI_WATERLOGO_CNT_MAX;k++){
						MovieStamp_DrawMultiWaterLogo(i, k, &g_sMultiWaterLogo[i][k], FALSE);
					}
				}
#endif
				g_VsStampStart[uiVEncOutPortId][j] &= (~(0x80000000));
			} else {
				//DBG_DUMP("==>RawEncStamp out of range: g_VsStampStart[%x][%d]=%x, en=%d\r\n", uiVEncOutPortId, j, g_VsStampStart[uiVEncOutPortId][j], Enable);
			}
		}
		mask >>= 1;
	}
}


void MovieStamp_VsSwapOsd(UINT32 uiVEncOutPortId, UINT32 StampPath, UINT32 VsHDPathId)
{
	HD_RESULT ret;
	//DBG_DUMP("VsSwapOs  uiVEncOutPortId=0x%x, StampPath=0x%x, VsHDPathId=0x%x\r\n", uiVEncOutPortId, StampPath, VsHDPathId);
	//if(g_VsStampStart[uiVEncOutPortId][StampPath]==0){
	if(g_VsStampStart[uiVEncOutPortId][StampPath]==0 || (g_VsStampStart[uiVEncOutPortId][StampPath] & 0x80000000)){
		//DBG_ERR("uiVidEncId=%d\r\n", uiVidEncId);

		ret = hd_videoenc_start(VsHDPathId);
		if (ret != HD_OK) {
			DBG_ERR("start enc stamp fail=%d\r\n", ret);
			return;
		}
		g_VsStampStart[uiVEncOutPortId][StampPath]=VsHDPathId;
	}
}
//#NT#2018/03/22#Brain Yen -end
BOOL MovieStamp_IsEnable(void)
{
    return ((BOOL)g_VsStampEn);
}
static UINT32   g_MovieStampRawEncodeVirPort[(SENSOR_CAPS_COUNT*2)]={0};
static UINT32   g_MovieStampRawEncodeVirPortCont=0;
UINT32 MovieStamp_GetRawEncVirtualPort(UINT32 RecId)
{
	HD_PATH_ID      uiVEncOutPortId = 0;
	uiVEncOutPortId = HD_GET_OUT(ImageApp_MovieMulti_GetRawEncPort(_CFG_REC_ID_1)) - 1;
	if(g_MovieStampRawEncodeVirPort[RecId]==0){
		g_MovieStampRawEncodeVirPort[RecId]=uiVEncOutPortId + g_MovieStampRawEncodeVirPortCont;
		g_MovieStampRawEncodeVirPortCont++;
	}
	//DBG_DUMP("GetVirPort [%d]EncodeVirPort=%d, VEncOutPortId=%d, VirPortCont=%d\r\n", RecId, g_MovieStampRawEncodeVirPort[RecId],uiVEncOutPortId,g_MovieStampRawEncodeVirPortCont);
	//DBG_DUMP("SENSOR_CAPS_COUNT=%d ,%d,%d,%d\r\n", SENSOR_CAPS_COUNT,g_MovieStampRawEncodeVirPort[0],g_MovieStampRawEncodeVirPort[1],g_MovieStampRawEncodeVirPort[2]);

	return g_MovieStampRawEncodeVirPort[RecId];
}
UINT32 MovieStamp_IsRawEncVirPort(UINT32 PortId)
{
	HD_PATH_ID      uiVEncOutPortId = 0;
	UINT32 i=0;
	UINT32 Result=0;
	if (!(System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_MOVIE || System_GetState(SYS_STATE_NEXTMODE) == PRIMARY_MODE_MOVIE)) {
		//DBG_ERR("not movie mode\r\n");
		return Result;
	}

	uiVEncOutPortId = HD_GET_OUT(ImageApp_MovieMulti_GetRawEncPort(_CFG_REC_ID_1)) - 1;
	for(i=0;i<g_MovieStampRawEncodeVirPortCont;i++){
		if((uiVEncOutPortId+i) ==PortId ){
			Result=1;
			break;
		}
	}
	//DBG_DUMP("IsVirPort PortId=%d, Result=%d ,VirPortCont=%d\r\n", PortId, Result ,g_MovieStampRawEncodeVirPortCont);
	//DBG_DUMP("VirPort[0]=%d,%d,%d\r\n", g_MovieStampRawEncodeVirPort[0],g_MovieStampRawEncodeVirPort[1],g_MovieStampRawEncodeVirPort[2]);

	return Result;
}
#if defined (MOVIE_MULTISTAMP_FUNC) && (MOVIE_MULTISTAMP_FUNC == ENABLE)
//#define MOVIE_MULTI_STAMP_CNT_MAX           (VSSTAMP_MAX_VIDEOENC_PATH-1)//5
#define MOVIE_MULTI_STAMP_MAX_NAMECHAR_CNT  30

static STAMP_ADDR_INFO g_MovieMultiStampPoolAddr[VENC_OUT_PORTID_MAX][MOVIE_MULTI_STAMP_CNT_MAX]={0};

FONT_CONV_IN g_MultiStampVsFontIn[VENC_OUT_PORTID_MAX][MOVIE_MULTI_STAMP_CNT_MAX]={0};
FONT_CONV_OUT g_MultiStampVsFontOut[VENC_OUT_PORTID_MAX][MOVIE_MULTI_STAMP_CNT_MAX]={0};
static UINT32 g_MultiStampVEncHDPathId[VENC_OUT_PORTID_MAX][MOVIE_MULTI_STAMP_CNT_MAX]={0};

UINT32 MovieStamp_GetMultiBufAddr(UINT32 uiVEncOutPortId, UINT32 Id, UINT32 blk_size)
{
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	CHAR pool_name[30] ={0};
	if(g_MovieMultiStampPoolAddr[uiVEncOutPortId][Id].pool_va == 0)  {

            sprintf(pool_name,"MovieMultiStamp_%d_%d",(int)uiVEncOutPortId,(int)Id);

		ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, blk_size, ddr_id);
		if (ret != HD_OK) {
			DBG_ERR("alloc fail size 0x%x, ddr %d\r\n", blk_size, ddr_id);
			return 0;
		}
		DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
		g_MovieMultiStampPoolAddr[uiVEncOutPortId][Id].pool_va=(UINT32)va;
		g_MovieMultiStampPoolAddr[uiVEncOutPortId][Id].pool_pa=(UINT32)pa;
		memset(va, 0, blk_size);
	}

	if(g_MovieMultiStampPoolAddr[uiVEncOutPortId][Id].pool_va == 0)
		DBG_ERR("get buf addr err\r\n");
	return g_MovieMultiStampPoolAddr[uiVEncOutPortId][Id].pool_va;

}
void MovieStamp_DestroyMultiStampBuff(void)
{
	UINT32 i, j, ret;
	for (i=0;i<(SENSOR_CAPS_COUNT*(2+1)+1);i++) {
		for (j=0;j<MOVIE_MULTI_STAMP_CNT_MAX;j++) {
			if (g_MovieMultiStampPoolAddr[i][j].pool_va != 0) {
				ret = hd_common_mem_free((UINT32)g_MovieMultiStampPoolAddr[i][j].pool_pa, (void *)g_MovieMultiStampPoolAddr[i][j].pool_va);
				if (ret != HD_OK) {
					DBG_ERR("FileIn release blk failed! (%d)\r\n", ret);
					break;
				}
				g_MovieMultiStampPoolAddr[i][j].pool_va = 0;
				g_MovieMultiStampPoolAddr[i][j].pool_pa = 0;
			}
		}
	}

}
//every frame has 5 stamp, and  every stamp has MAX 30 charaters
//uiVidEncId: Video path
//CntId: Stamp cnt, MAX is 5, from 0 to 4
//*Pos: Stamp position
//*pStr: Stamp data
void MovieStamp_DrawMultiStamp(UINT32 uiVEncOutPortId, UINT32 CntId, UPOINT *Pos, char *pStr, UINT32 bInitStart)
{
	// temporary return multistamp due to some error here.

	STAMP_INFO   sMovieStampInfo;
	UINT16 uiFontWidth=0;
	UINT16 uiFontHeight;
	ISIZE FontSize;
	UINT32 VsMultiFontDataAddr[VENC_OUT_PORTID_MAX][MOVIE_MULTI_STAMP_CNT_MAX];
	UINT32 uiVsStampSize = 0;
	UINT32 MemSize;
	UINT32 MemAddr[MOVIE_MULTI_STAMP_CNT_MAX] = {0};
	MOVIE_CFG_REC_ID rec_id=_CFG_REC_ID_1;
	UINT32 i;
	UINT32 is_raw_enc=0;
	ER FontConvRet;
	HD_RESULT ret;

	UINT32 Result=0;
	HD_OSG_STAMP_BUF  buf;
	static UINT32 uiVsMultiStampPa = 0;
	//DBG_ERR("uiVidEncId=%d\n",uiVidEncId);
	if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_MOVIE) {
		DBG_ERR("not movie mode\r\n");
		return NVTEVT_CONSUME;
	}

	if(pStr == NULL){
		DBG_ERR("str NULL\r\n");
		return;
	}
	if(strlen(pStr) == 0){
		DBG_ERR("str len 0\r\n");
		return;
	}
	if(strlen(pStr) > MOVIE_MULTI_STAMP_MAX_NAMECHAR_CNT){
		DBG_ERR("str len reach MAX %d\r\n",strlen(pStr) );
		return;
	}
	if(CntId>=MOVIE_MULTI_STAMP_CNT_MAX){
		DBG_ERR("CntId reach MAX %d\r\n",CntId);
		return;
	}
	if ((g_uiMovieStampSetup[uiVEncOutPortId] & STAMP_SWITCH_MASK) != STAMP_ON) {
		DBG_ERR("uiVidEncId(%d) is OFF\r\n",uiVEncOutPortId);
		return;
	}
	sMovieStampInfo.pi8Str=pStr;
	//sMovieStampInfo.pDataBase=g_MovieStampInfo[uiVidEncId].pDataBase;
	//icon width must be even
	FontSize=MovieStamp_GetStampDataWidth(&sMovieStampInfo, (const IMAGE_TABLE *)g_VsFontIn[uiVEncOutPortId].pFont);
	uiFontWidth=FontSize.w;
	uiFontWidth=ALIGN_CEIL_8(uiFontWidth);
	if(uiFontWidth > MULTISTAMP_WIDTH_MAX){
		DBG_ERR("uiFontWidth reach MAX %d\r\n", uiFontWidth);
		return;
	}
	uiFontWidth=MULTISTAMP_WIDTH_MAX;
	uiFontHeight=STAMP_HEIGHT_MAX;//sMovieStampInfo.pDataBase->pIconHeader[0x20].uiHeight;

	//DBG_DUMP("FontWidth=%d, FontHeight=%d, x=%d, y=%d\r\n",uiFontWidth,uiFontHeight,Pos->x, Pos->y);
	if(bInitStart){
		//MemSize =videosprite_query_size(VDS_PHASE_BTN, VDS_BUF_TYPE_PING_PONG, uiFontWidth, uiFontHeight)+ uiFontWidth*uiFontHeight*VSFONT_BUF_RATIO/100;
		//MovieStamp_OsgQueryBufSize(uiFontWidth, uiFontHeight);
		MemSize =MovieStamp_OsgQueryBufSize(uiFontWidth, uiFontHeight)+ uiFontWidth*uiFontHeight*VSFONT_BUF_RATIO/100;
		MemAddr[CntId]=MovieStamp_GetMultiBufAddr(uiVEncOutPortId, CntId, MemSize);

		memcpy(&g_MultiStampVsFontIn[uiVEncOutPortId][CntId], &g_VsFontIn[uiVEncOutPortId], sizeof(FONT_CONV_IN));
		g_MultiStampVsFontIn[uiVEncOutPortId][CntId].MemAddr = MemAddr[CntId]+ MemSize*OSG_BUF_RATIO/(OSG_BUF_RATIO+VSFONT_BUF_RATIO);
		g_MultiStampVsFontIn[uiVEncOutPortId][CntId].MemSize = uiFontWidth*uiFontHeight*VSFONT_BUF_RATIO/100;
		//g_MultiStampVsFontIn[CntId].pStr = pStr;

	      //if ((FontConvRet=FontConv(&g_MultiStampVsFontIn[CntId], &g_MultiStampVsFontOut[CntId])) != E_OK) {
	      //     DBG_ERR("FontConv err, ret=%d\r\n",FontConvRet);
	      //     return;
	      //}
	       //pVsMultiFontDataAddr[CntId]=(UINT16 *)VsFontOut[CntId].GenImg.phy_addr[0];
	       //VsMultiFontDataAddr[CntId]=g_MultiStampVsFontOut[CntId].GenImg.phy_addr[0];
	       //DBG_DUMP("Str=%s\r\n",pStr);
	       //DBG_DUMP("CntId=%d, w=%d, %d\r\n", CntId,VsFontOut[CntId].GenImg.Width, VsFontOut[CntId].GenImg.Height);
		//uiVsStampSize = videosprite_query_size(VDS_PHASE_BTN, VDS_BUF_TYPE_PING_PONG, uiFontWidth, uiFontHeight);
		uiVsStampSize=MovieStamp_OsgQueryBufSize(uiFontWidth, uiFontHeight);
		//videosprite_config_ping_pong_buf(VDS_PHASE_BTN, (VS_MULTISTAMP_ID+CntId), MemAddr[CntId], uiVsStampSize);

		//DBG_DUMP("uiVsStampSize=%d\r\n",uiVsStampSize);

		//UINT32 VsHDPathId=0;
		//HD_RESULT ret;

		//UINT32 Result=0;
		//HD_OSG_STAMP_BUF  buf;
		//static UINT32 uiVsMultiStampPa = 0;

		uiVsMultiStampPa=g_MovieMultiStampPoolAddr[uiVEncOutPortId][CntId].pool_pa;
		//DBG_DUMP("[%d]uiVsMultiStampPa=%d\r\n",CntId,uiVsMultiStampPa);

		if(uiVsMultiStampPa==0){
			DBG_ERR("uiVsMultiStampPa fail\n");
			return;
		}
		memset(&buf, 0, sizeof(HD_OSG_STAMP_BUF));

		//UINT32 path_id_exe;//=ImageApp_MovieMulti_GetVdoEncPort(0);
		//DBG_DUMP("MovieMulti_GetVdoEncPort=0x%x, 0x%x\r\n",ImageApp_MovieMulti_GetVdoEncPort(0),HD_VIDEOENC_0_IN_0);
		for(i=0;i<SENSOR_CAPS_COUNT;i++){
			//main
			if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetVdoEncPort(_CFG_REC_ID_1+i)) - 1)){
				rec_id=_CFG_REC_ID_1+i;
				break;
			}
			//clone
			if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetVdoEncPort(_CFG_CLONE_ID_1+i)) - 1)){
				rec_id=_CFG_CLONE_ID_1+i;
				break;
			}
			//wifi
			if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetVdoEncPort(_CFG_STRM_ID_1+i)) - 1)){
				rec_id=_CFG_STRM_ID_1+i;
				break;
			}

			//main raw enc
			if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetRawEncPort(_CFG_REC_ID_1+i)) - 1)){
				rec_id=_CFG_REC_ID_1+i;
				is_raw_enc=1;
				break;
			}
			#if 0
			//clone raw enc
			if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetRawEncPort(_CFG_CLONE_ID_1+i)) - 1)){
				rec_id=_CFG_CLONE_ID_1+i;
				is_raw_enc=1;
				break;
			}
			#endif
		}

		if(is_raw_enc){
			if((ret = hd_videoenc_open(ImageApp_MovieMulti_GetRawEncPort(rec_id), (VSSTAMP_MULTISTAMP_HD_STAMP_ID+CntId), &g_MultiStampVEncHDPathId[uiVEncOutPortId][CntId])) != HD_OK){
				DBG_ERR("hd_videoenc_open fail uiVEncOutPortId=%d, ret=%d, GetRawEncPort=%d\r\n",uiVEncOutPortId,ret,ImageApp_MovieMulti_GetRawEncPort(rec_id));
			}
		}else{
			//DBG_ERR("VidEncId=%d, REC_ID=%d, 0x%x\r\n",uiVidEncId,id,ImageApp_MovieMulti_GetVdoEncPort(id));

			//HD_VIDEOENC_OUT(0, CntId);
			if((ret = hd_videoenc_open(ImageApp_MovieMulti_GetVdoEncPort(rec_id) , (VSSTAMP_MULTISTAMP_HD_STAMP_ID+CntId), &g_MultiStampVEncHDPathId[uiVEncOutPortId][CntId])) != HD_OK){
			    DBG_ERR("hd_videoenc_open fail ret=%d\r\n",ret);
			}
		}
		//DBG_ERR("VsHDPathId=%d\r\n",VsHDPathId);
		if(MovieStamp_IsRawEncVirPort(uiVEncOutPortId) && (uiVEncOutPortId!=(HD_GET_OUT(ImageApp_MovieMulti_GetRawEncPort(_CFG_REC_ID_1)) - 1))){
			return;
		}
		buf.type      = HD_OSG_BUF_TYPE_PING_PONG;
		buf.p_addr    = uiVsMultiStampPa;
		buf.size      = uiVsStampSize;
		Result=hd_videoenc_set(g_MultiStampVEncHDPathId[uiVEncOutPortId][CntId], HD_VIDEOENC_PARAM_IN_STAMP_BUF, &buf);
		if(Result != HD_OK){
			DBG_ERR("fail to set stamp buffer, Result=0x%x, Pa=0x%x, Sz=0x%x\n",Result, uiVsMultiStampPa,uiVsStampSize);
			return;
		}
	}
	g_MultiStampVsFontIn[uiVEncOutPortId][CntId].pStr = pStr;
       if ((FontConvRet=FontConv(&g_MultiStampVsFontIn[uiVEncOutPortId][CntId], &g_MultiStampVsFontOut[uiVEncOutPortId][CntId])) != E_OK) {
            DBG_ERR("FontConv err, ret=%d\r\n",FontConvRet);
            return;
       }
       VsMultiFontDataAddr[uiVEncOutPortId][CntId]=g_MultiStampVsFontOut[uiVEncOutPortId][CntId].GenImg.phy_addr[0];
	i=uiVEncOutPortId;
	UINT32 RawEncVEncPort = 0;
	RawEncVEncPort = HD_GET_OUT(ImageApp_MovieMulti_GetRawEncPort(_CFG_REC_ID_1)) - 1;
	if(MovieStamp_IsRawEncVirPort(uiVEncOutPortId)){
		uiVEncOutPortId=RawEncVEncPort;
	}

	//DBG_DUMP("VsHDPathId=%d\r\n",VsHDPathId);
	if(MovieStamp_VsUpdateOsd(g_MultiStampVEncHDPathId[uiVEncOutPortId][CntId], TRUE, MOVIE_STAMP_LAYER2, (VS_MULTISTAMP_REGION+CntId),  Pos->x, Pos->y, g_MultiStampVsFontOut[i][CntId].GenImg.pw[0]/2, g_MultiStampVsFontOut[i][CntId].GenImg.ph[0], (void*)VsMultiFontDataAddr[i][CntId])){
		MovieStamp_VsSwapOsd(i, (VSSTAMP_MULTISTAMP_HD_STAMP_ID+CntId-HD_STAMP_BASE),g_MultiStampVEncHDPathId[uiVEncOutPortId][CntId]);
	}else{
		MovieStamp_VsStop(i, (VSSTAMP_MULTISTAMP_HD_STAMP_ID+CntId-HD_STAMP_BASE));
	}
}
#endif


#if defined (MOVIE_MULTIWATERLOGO_FUNC) && (MOVIE_MULTIWATERLOGO_FUNC == ENABLE)
static STAMP_ADDR_INFO g_MovieMultiWaterLogoPoolAddr[VENC_OUT_PORTID_MAX][MOVIE_MULTI_WATERLOGO_CNT_MAX]={0};
//static UINT32 g_MultiWaterLogoVEncHDPathId[VENC_OUT_PORTID_MAX][MOVIE_MULTI_WATERLOGO_CNT_MAX]={0};

UINT32 MovieStamp_GetMultiWaterLogoBufAddr(UINT32 uiVEncOutPortId, UINT32 Id, UINT32 blk_size)
{
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	CHAR pool_name[30] ={0};
	if(g_MovieMultiWaterLogoPoolAddr[uiVEncOutPortId][Id].pool_va == 0)  {

            sprintf(pool_name,"MovieMultiStamp_%d_%d",(int)uiVEncOutPortId,(int)Id);

		ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, blk_size, ddr_id);
		if (ret != HD_OK) {
			DBG_ERR("alloc fail size 0x%x, ddr %d\r\n", blk_size, ddr_id);
			return 0;
		}
		DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
		g_MovieMultiWaterLogoPoolAddr[uiVEncOutPortId][Id].pool_va=(UINT32)va;
		g_MovieMultiWaterLogoPoolAddr[uiVEncOutPortId][Id].pool_pa=(UINT32)pa;
		memset(va, 0, blk_size);
	}

	if(g_MovieMultiWaterLogoPoolAddr[uiVEncOutPortId][Id].pool_va == 0)
		DBG_ERR("get buf addr err\r\n");
	return g_MovieMultiWaterLogoPoolAddr[uiVEncOutPortId][Id].pool_va;

}
void MovieStamp_DestroyMultiWaterLogoBuff(void)
{
	UINT32 i, j, ret;
	for (i=0;i<(SENSOR_CAPS_COUNT*(2+1)+1);i++) {
		for (j=0;j<MOVIE_MULTI_WATERLOGO_CNT_MAX;j++) {
			if (g_MovieMultiWaterLogoPoolAddr[i][j].pool_va != 0) {
				ret = hd_common_mem_free((UINT32)g_MovieMultiWaterLogoPoolAddr[i][j].pool_pa, (void *)g_MovieMultiWaterLogoPoolAddr[i][j].pool_va);
				if (ret != HD_OK) {
					DBG_ERR("FileIn release blk failed! (%d)\r\n", ret);
					break;
				}
				g_MovieMultiWaterLogoPoolAddr[i][j].pool_va = 0;
				g_MovieMultiWaterLogoPoolAddr[i][j].pool_pa = 0;
			}
		}
	}

}
int MovieStamp_GetMultiWaterLogoSource(UINT32 CntId,WATERLOGO_BUFFER *waterSrc)
{
	switch(CntId){
		default:
		case 0:
		memcpy(waterSrc,&g_WaterLogo_640,sizeof(WATERLOGO_BUFFER));
		waterSrc->uiXPos=10;
		waterSrc->uiYPos=20;
		waterSrc->uilayer=MOVIE_STAMP_LAYER1;
		break;
		case 1:
		memcpy(waterSrc,&g_WaterLogo_1440,sizeof(WATERLOGO_BUFFER));
		waterSrc->uiXPos=16;
		waterSrc->uiYPos=20;
		waterSrc->uilayer=MOVIE_STAMP_LAYER2;
		break;
		case 2:
		memcpy(waterSrc,&g_WaterLogo_640,sizeof(WATERLOGO_BUFFER));
		waterSrc->uiXPos=400;
		waterSrc->uiYPos=420;
		break;
		case 3:
		memcpy(waterSrc,&g_WaterLogo_640,sizeof(WATERLOGO_BUFFER));
		waterSrc->uiXPos=600;
		waterSrc->uiYPos=620;
		break;
		case 4:
		memcpy(waterSrc,&g_WaterLogo_1440,sizeof(WATERLOGO_BUFFER));
		waterSrc->uiXPos=800;
		waterSrc->uiYPos=820;
		break;
	}
 	return E_OK;
}
//CntId: Stamp cnt, MAX is 5, from 0 to 4
void MovieStamp_DrawMultiWaterLogo(UINT32 uiVEncOutPortId, UINT32 CntId, WATERLOGO_BUFFER *sWaterLogo, UINT32 bInitStart)
{
	UINT32 uiMultiWaterLogoSize = 0;
	MOVIE_CFG_REC_ID rec_id=_CFG_REC_ID_1;
	UINT32 i;
	UINT32 is_raw_enc=0;
	HD_RESULT ret;

	UINT32 Result=0;
	HD_OSG_STAMP_BUF  buf;
	static UINT32 uiMultiWaterLogoPa = 0;
	if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_MOVIE) {
		DBG_ERR("not movie mode\r\n");
		return NVTEVT_CONSUME;
	}

	if(sWaterLogo == NULL){
		DBG_ERR("str NULL\r\n");
		return;
	}
	if(sWaterLogo->uiWaterLogoAddr== 0){
		DBG_ERR("data addr 0\r\n");
		return;
	}
	if(CntId>=MOVIE_MULTI_WATERLOGO_CNT_MAX){
		DBG_ERR("CntId reach MAX %d\r\n",CntId);
		return;
	}
	if ((g_uiMovieStampSetup[uiVEncOutPortId] & STAMP_SWITCH_MASK) != STAMP_ON) {
		DBG_ERR("uiVidEncId(%d) is OFF\r\n",uiVEncOutPortId);
		return;
	}
	//DBG_DUMP("MovieStamp_DrawMultiWaterLogo  uiVEncOutPortId=0x%x, CntId=0x%x, bInitStart=%d\r\n",uiVEncOutPortId,CntId,bInitStart);

	if(bInitStart){
		for(i=0;i<SENSOR_CAPS_COUNT;i++){
			//main
			if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetVdoEncPort(_CFG_REC_ID_1+i)) - 1)){
				rec_id=_CFG_REC_ID_1+i;
				break;
			}
			//clone
			if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetVdoEncPort(_CFG_CLONE_ID_1+i)) - 1)){
				rec_id=_CFG_CLONE_ID_1+i;
				break;
			}
			//wifi
			if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetVdoEncPort(_CFG_STRM_ID_1+i)) - 1)){
				rec_id=_CFG_STRM_ID_1+i;
				break;
			}

			//main raw enc
			if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetRawEncPort(_CFG_REC_ID_1+i)) - 1)){
				rec_id=_CFG_REC_ID_1+i;
				is_raw_enc=1;
				break;
			}
			#if 0
			//clone raw enc
			if(uiVEncOutPortId==(HD_GET_OUT(ImageApp_MovieMulti_GetRawEncPort(_CFG_CLONE_ID_1+i)) - 1)){
				rec_id=_CFG_CLONE_ID_1+i;
				is_raw_enc=1;
				break;
			}
			#endif
		}
		//DBG_DUMP("MovieStamp_DrawMultiWaterLogo  is_raw_enc=0x%x, rec_id=0x%x\r\n",is_raw_enc, rec_id);

		if(is_raw_enc){
			if((ret = hd_videoenc_open(ImageApp_MovieMulti_GetRawEncPort(rec_id), (VSSTAMP_MULTIWATERLOGO_HD_STAMP_ID+CntId), &g_MultiWaterLogoVEncHDPathId[uiVEncOutPortId][CntId])) != HD_OK){
				DBG_ERR("hd_videoenc_open fail uiVEncOutPortId=%d, ret=%d, GetRawEncPort=%d\r\n",uiVEncOutPortId,ret,ImageApp_MovieMulti_GetRawEncPort(rec_id));
			}
		}else{
			//DBG_ERR("VidEncId=%d, REC_ID=%d, 0x%x\r\n",uiVidEncId,id,ImageApp_MovieMulti_GetVdoEncPort(id));

			//HD_VIDEOENC_OUT(0, CntId);
			if((ret = hd_videoenc_open(ImageApp_MovieMulti_GetVdoEncPort(rec_id) , (VSSTAMP_MULTIWATERLOGO_HD_STAMP_ID+CntId), &g_MultiWaterLogoVEncHDPathId[uiVEncOutPortId][CntId])) != HD_OK){
			    DBG_ERR("hd_videoenc_open fail ret=%d\r\n",ret);
			}
		}
		//DBG_ERR("VsHDPathId=%d\r\n",VsHDPathId);
		if(MovieStamp_IsRawEncVirPort(uiVEncOutPortId) && (uiVEncOutPortId!=(HD_GET_OUT(ImageApp_MovieMulti_GetRawEncPort(_CFG_REC_ID_1)) - 1))){
			return;
		}


		uiMultiWaterLogoSize = MovieStamp_OsgQueryBufSize(sWaterLogo->uiWidth, sWaterLogo->uiHeight);
		MovieStamp_GetMultiWaterLogoBufAddr(uiVEncOutPortId, CntId, uiMultiWaterLogoSize);


		uiMultiWaterLogoPa=g_MovieMultiWaterLogoPoolAddr[uiVEncOutPortId][CntId].pool_pa;
		if(uiMultiWaterLogoPa==0){
			DBG_ERR("uiMultiWaterLogoPa fail\n");
			return;
		}
		memset(&buf, 0, sizeof(HD_OSG_STAMP_BUF));
		buf.type      = HD_OSG_BUF_TYPE_PING_PONG;
		buf.p_addr    = uiMultiWaterLogoPa;
		buf.size      = uiMultiWaterLogoSize;
		buf.ddr_id =0;
		Result=hd_videoenc_set(g_MultiWaterLogoVEncHDPathId[uiVEncOutPortId][CntId], HD_VIDEOENC_PARAM_IN_STAMP_BUF, &buf);
		if(Result != HD_OK){
			DBG_ERR("fail to set stamp buffer, Result=0x%x, Pa=0x%x, Sz=0x%x\n",Result, uiMultiWaterLogoPa,uiMultiWaterLogoSize);
			return;
		}
	}

	i=uiVEncOutPortId;
	UINT32 RawEncVEncPort = 0;
	RawEncVEncPort = HD_GET_OUT(ImageApp_MovieMulti_GetRawEncPort(_CFG_REC_ID_1)) - 1;
	if(MovieStamp_IsRawEncVirPort(uiVEncOutPortId)){
		uiVEncOutPortId=RawEncVEncPort;
	}

	//DBG_DUMP("multi water X=%d, Y=%d, Width=%dx%d\r\n",sWaterLogo->uiXPos,sWaterLogo->uiYPos,sWaterLogo->uiWidth, sWaterLogo->uiHeight);
	if(MovieStamp_VsUpdateOsd(g_MultiWaterLogoVEncHDPathId[uiVEncOutPortId][CntId], TRUE, sWaterLogo->uilayer, ( VS_MULTIWATERLOGO_REGION + CntId), sWaterLogo->uiXPos, sWaterLogo->uiYPos, sWaterLogo->uiWidth, sWaterLogo->uiHeight, (void*)sWaterLogo->uiWaterLogoAddr)){
		MovieStamp_VsSwapOsd(i, (VSSTAMP_MULTIWATERLOGO_HD_STAMP_ID+CntId-HD_STAMP_BASE),g_MultiWaterLogoVEncHDPathId[uiVEncOutPortId][CntId]);
	}else{
		MovieStamp_VsStop(i, (VSSTAMP_MULTIWATERLOGO_HD_STAMP_ID+CntId-HD_STAMP_BASE));
	}

}
#endif

UINT32 MovieStamp_GetGfxInitBufAddr(UINT32 blk_size)
{
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	CHAR pool_name[20] ={0};
	if(g_GfxInitPoolAddr.pool_va == 0)  {

		sprintf(pool_name,"gxgfx_temp");

		ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, blk_size, ddr_id);
		if (ret != HD_OK) {
			DBG_ERR("alloc fail size 0x%x, ddr %d\r\n", blk_size, ddr_id);
			return 0;
		}
		DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
		g_GfxInitPoolAddr.pool_va=(UINT32)va;
		g_GfxInitPoolAddr.pool_pa=(UINT32)pa;
		memset(va, 0, blk_size);
	}

	if(g_GfxInitPoolAddr.pool_va == 0)
		DBG_ERR("get buf addr err\r\n");
	return g_GfxInitPoolAddr.pool_va;

}

//for UI_GfxInit, for FontConv function
void UI_GfxInitLite(void)
{
	UINT32              uiPoolAddr;
	DBG_FUNC_BEGIN("\r\n");
	if(isUI_GfxInitLiteOpened==1){
		return;
	}
	isUI_GfxInitLiteOpened=1;

	MovieStamp_GetGfxInitBufAddr(ALIGN_CEIL_64(4096));

	uiPoolAddr =  (UINT32)g_GfxInitPoolAddr.pool_va;

	////////////////////////////////////////////////////////////////

	GxGfx_Config(CFG_STRING_BUF_SIZE, 256);
	//Init Gfx
	GxGfx_Init((UINT32 *)uiPoolAddr, ALIGN_CEIL_64(4096));     //initial Graphics

	//set default shape, text, image state for GxGfx
	GxGfx_SetAllDefault();
	//set custom image state for GxGfx
	GxGfx_SetImageStroke(ROP_KEY, IMAGEPARAM_DEFAULT);
	GxGfx_SetImageColor(IMAGEPALETTE_DEFAULT, IMAGEPARAM_DEFAULT);
	GxGfx_SetTextColor(TEXTFORECOLOR1_DEFAULT, TEXTFORECOLOR2_DEFAULT, TEXTFORECOLOR3_DEFAULT);
}
