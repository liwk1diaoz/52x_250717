////////////////////////////////////////////////////////////////////////////////
#include "System/SysCommon.h"
#include "WiFiIpc/nvtwifi.h"
//#include "DateTimeInfo.h"
#include "UIDateImprint.h"
#include "UIDateImprintID.h"
//#include "Utility.h"
#include "FontConv/FontConv.h"
#include "Mode/UIMode.h"
#include "gximage/gximage.h"
//#include "nvtmpp.h"
#include "UIAppPhoto.h"
#include "GxVideoFile.h"
#include <kflow_common/nvtmpp.h>
#include "kwrap/type.h"
#include "UIApp/MovieStamp/MovieStamp.h"
#include "UIWnd/UIFlow.h"
#include "vf_gfx.h"

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UIDateImprint
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

#define  USE_DBG_STAMP          DISABLE

/****************************************************
 * IMPORTANT!!
 *
 * LVGL UI style only accept RGB color
 *
 ****************************************************/
#if defined(_UI_STYLE_LVGL_)

#include "UIApp/lv_user_font_conv/lv_user_font_conv.h"

#define DATE_COLOR_TRANSPARENT   lv_color_to32(LV_COLOR_OLIVE) /* it will be converted into yuv as color key */
#define DATE_COLOR_WHITE         lv_color_to32(LV_COLOR_WHITE)
#define DATE_COLOR_BLACK         lv_color_to32(LV_COLOR_BLACK)
#define DATE_COLOR_ORANGED       lv_color_to32(LV_COLOR_ORANGE)
#define DATE_COLOR_RED           lv_color_to32(LV_COLOR_RED)

#define LV_USER_FONT_CONV_ALIGN_W 8
#define LV_USER_FONT_CONV_ALIGN_H 2

static void _movie_stamp_lv_cfg_init(lv_user_font_conv_draw_cfg *draw_cfg)
{
	lv_user_font_conv_draw_cfg_init(draw_cfg);


	draw_cfg->radius = LV_USER_CFG_STAMP_RADIUS;
	draw_cfg->string.align = LV_USER_CFG_STAMP_TEXT_ALIGN;
	draw_cfg->string.letter_space = LV_USER_CFG_STAMP_LETTER_SPACE;
	draw_cfg->ext_w = LV_USER_CFG_STAMP_EXT_WIDTH;
	draw_cfg->ext_h = LV_USER_CFG_STAMP_EXT_HEIGHT;
	draw_cfg->border.width = LV_USER_CFG_STAMP_BORDER_WIDTH;

}

static UINT32 UIDateImprint_GetStampMemSize_LVGL(const lv_font_t *pFont, char* text)
{
	lv_user_font_conv_draw_cfg draw_cfg = {0};
	lv_user_font_conv_calc_buffer_size_result result = {0};

	_movie_stamp_lv_cfg_init(&draw_cfg);

	draw_cfg.string.font = (lv_font_t *)pFont;
	draw_cfg.string.text = text;
	draw_cfg.align_w = LV_USER_FONT_CONV_ALIGN_W;
	draw_cfg.align_h = LV_USER_FONT_CONV_ALIGN_H;

	if(PhotoExe_GetCapYUV420En()){
		draw_cfg.fmt = HD_VIDEO_PXLFMT_YUV420;//GX_IMAGE_PIXEL_FMT_YUV420_PACKED;
	}else{
		draw_cfg.fmt = HD_VIDEO_PXLFMT_YUV422;//GX_IMAGE_PIXEL_FMT_YUV422_PACKED;
	}

	lv_user_font_conv_calc_buffer_size(&draw_cfg, &result);

	return result.output_buffer_size;
}

static ER FontConv_LVGL(FONT_CONV_IN *pIn, FONT_CONV_OUT *pOut)
{
	HD_RESULT r;
	lv_user_font_conv_draw_cfg draw_cfg = {0};
	lv_user_font_conv_mem_cfg mem_cfg = {0};
	lv_user_font_conv_calc_buffer_size_result result = {0};
	lv_color32_t color32;

	_movie_stamp_lv_cfg_init(&draw_cfg);

	draw_cfg.fmt = pIn->Format;
	draw_cfg.string.font = (lv_font_t *) pIn->pFont;
	draw_cfg.string.text = pIn->pStr;
	draw_cfg.string.align = LV_USER_CFG_STAMP_TEXT_ALIGN;
	draw_cfg.align_w = LV_USER_FONT_CONV_ALIGN_W;
	draw_cfg.align_h = LV_USER_FONT_CONV_ALIGN_H;

	/* text color */
	color32.full = pIn->ciSolid;
	draw_cfg.string.color = LV_COLOR_MAKE(color32.ch.red, color32.ch.green, color32.ch.blue);
	draw_cfg.string.opa = LV_COLOR_GET_A32(color32);

	/* bg color */
	color32.full = pIn->ciTransparet;
	draw_cfg.bg.color = LV_COLOR_MAKE(color32.ch.red, color32.ch.green, color32.ch.blue);
	draw_cfg.bg.opa =  LV_COLOR_GET_A32(color32);

	/* border color */
	color32.full = pIn->ciFrame;
	draw_cfg.border.color = LV_COLOR_MAKE(color32.ch.red, color32.ch.green, color32.ch.blue);
	draw_cfg.border.opa = LV_COLOR_GET_A32(color32);
	draw_cfg.border.width = LV_USER_CFG_STAMP_BORDER_WIDTH;

	/****************************************************************************
	 * IMPORTANT!!
	 *
	 * do not set color key to zero because engine may ignore zero value.
	 ****************************************************************************/
	draw_cfg.key_y = 255;
	draw_cfg.key_u = 255;
	draw_cfg.key_v = 255;

	mem_cfg.output_buffer = (void*)pIn->MemAddr;
	mem_cfg.output_buffer_size = pIn->MemSize;


	lv_user_font_conv_calc_buffer_size(&draw_cfg, &result);
	lv_user_font_conv(&draw_cfg, &mem_cfg);

	UINT32 LineOffs[2];
	UINT32 PxlAddrs[2];

	HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};

	vir_meminfo.va = (void *)(mem_cfg.output_buffer);
	if ( hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) != HD_OK) {
		DBG_ERR("convert output_buffer to pa failed!\n");
		return -1;
	}

	LineOffs[0] = (float)result.width * (float)(result.bpp / 8);
	LineOffs[1] = LineOffs[0];
	PxlAddrs[0] = (UINT32) vir_meminfo.pa;
	PxlAddrs[1] = PxlAddrs[0] + (result.width * result.height); /* y plane is w * h for yuv420 & 422 */

	r =  vf_init_ex(&pOut->GenImg, result.width, result.height, pIn->Format, LineOffs, PxlAddrs);
	if (r != HD_OK) {
		DBG_ERR("vf_init_ex failed %d\r\n",r);
	}

	pOut->ColorKeyY = draw_cfg.key_y;
	pOut->ColorKeyCb = draw_cfg.key_u;
	pOut->ColorKeyCr = draw_cfg.key_v;

	return 0;
}


#else

#include "UIApp/MovieStamp/DateStampFontTbl56x96.h"
#include "UIApp/MovieStamp/DateStampFontTbl42x72.h"
#include "UIApp/MovieStamp/DateStampFontTbl36x60.h"
#include "UIApp/MovieStamp/DateStampFontTbl26x44.h"
#include "UIApp/MovieStamp/DateStampFontTbl20x44.h"
#include "UIApp/MovieStamp/DateStampFontTbl18x30.h"
#include "UIApp/MovieStamp/DateStampFontTbl12x20.h"
#include "UIApp/MovieStamp/DateStampFontTbl10x16.h"

/****************************************************
 * YUV
 ****************************************************/
#define DATE_COLOR_TRANSPARENT  0x00808000
#define DATE_COLOR_WHITE        0x008080FF
#define DATE_COLOR_BLACK        0x00818101
#define DATE_COLOR_ORANGED      0x00D4328A
#define DATE_COLOR_RED          0x00FF554E

#endif

#define FONT_BUF_RATIO  (450)

typedef enum {
	DATE_IMPRINT_EVENT_QV = 0,            ///<quick view event
	DATE_IMPRINT_EVENT_SCR,               ///<screennail event
	DATE_IMPRINT_EVENT_PRI,               ///<primary image event
	DATE_IMPRINT_EVENT_MAX,               ///<Maximun value
} DATE_IMPRINT_EVENT;


typedef struct{
    UINT32  PosX;
    UINT32  PosY;
    //IMG_BUF Img;
    HD_VIDEO_FRAME  Img;
    UINT32  ColorKey;                ///< format 0x00YYUUVV
    UINT32  StampWeight;             ///< 0 ~ 255
} DS_STAMP_INFOR;


typedef struct {
	IMG_CAP_DATASTAMP_INFO *pCapInfo;
	UINT32                 MemAddr;
	UINT32                 MemSize;
	FONT_CONV_IN           tYuvDesc;
	FONT_CONV_OUT          tYuvInfo;
	DS_STAMP_INFOR         StampInfo;
	//GX_IMAGE_CP_ENG        copyEngine;
	GXIMG_CP_ENG  	copyEngine;
	UINT32                 lockptn;
	BOOL                   isStrDirty;
	CHAR                   StrBuf[64];
	UINT32                 pic_cnt;
} DATE_IMPRINT_INFO;


typedef struct {
	DATE_IMPRINT_INFO     info[DATE_IMPRINT_EVENT_MAX];

} DATE_IMPRINT_CTRL;

DATE_IMPRINT_CTRL  gDateImprintCtrl = {0};
DATE_IMPRINT_CTRL  *pCtrl = &gDateImprintCtrl;
//static char               gUiDateImprint_StrBuf[64] = {0};
//static NVTMPP_VB_POOL g_DateImprintPool[DATE_IMPRINT_EVENT_MAX]={NVTMPP_VB_INVALID_POOL, NVTMPP_VB_INVALID_POOL, NVTMPP_VB_INVALID_POOL};
static STAMP_ADDR_INFO g_DateImprintPool[DATE_IMPRINT_EVENT_MAX]={0};
void UiDateImprint_GetStampFont(UINT32 uiImageWidth, char **font);
char  *UiDateImprint_InitStrBuf(void);

void UiDateImprint_Lock(UINT32 ptn)
{
	FLGPTN       uiFlag = 0;
	vos_flag_wait(&uiFlag, UI_DATEIMPRINT_FLG_ID, ptn, TWF_CLR);
}

void UiDateImprint_UnLock(UINT32 ptn)
{
	vos_flag_set(UI_DATEIMPRINT_FLG_ID, ptn);
}


void UiDateImprint_InitBuff(void)
{

	DATE_IMPRINT_INFO   *pInfo;
	UINT32               i,genMaxw, genMaxh;
	//NVTMPP_VB_BLK  blk;
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	CHAR pool_name[20] ={0};
	ISIZE szStamp;
	UINT32 FontWidth = 0;
	UINT32 FontHight = 0;
	FONT  *pFont;

#if defined(_UI_STYLE_LVGL_)
	char* str = NULL;
#endif

#if defined(_UI_STYLE_LVGL_)
	LV_UNUSED(FontWidth);
	LV_UNUSED(FontHight);
	LV_UNUSED(szStamp);
#endif

	DBG_IND("\r\n");
	if (!UI_DATEIMPRINT_FLG_ID) {
		DBG_ERR("ID is not installed\r\n");
		return;
	}

#if defined(_UI_STYLE_LVGL_)
	DateTime_Load();
	str = DateTime_MakeYMDHMS(); /* calculate max buffer */
#endif


	for (i=0;i<DATE_IMPRINT_EVENT_MAX;i++) {
		pInfo = &pCtrl->info[i];
		if ( i == DATE_IMPRINT_EVENT_QV && g_DateImprintPool[i].pool_va == 0) {
			ISIZE size = GxVideo_GetDeviceSize(DOUT1);

			UiDateImprint_GetStampFont(size.w,(char**)&pFont);

#if defined(_UI_STYLE_LVGL_)
			pInfo->MemSize = UIDateImprint_GetStampMemSize_LVGL((const lv_font_t *)pFont, str);
#else
			GxGfx_SetTextStroke((const FONT *)pFont, FONTSTYLE_NORMAL, SCALE_1X);
			GxGfx_Text(0, 0, 0, UiDateImprint_InitStrBuf()); //not really draw
			szStamp = GxGfx_GetTextLastSize(); //just get text size

			FontWidth=ALIGN_CEIL(szStamp.w ,8);
			FontHight=ALIGN_CEIL(szStamp.h, 2);
			pInfo->MemSize = (FontWidth*FontHight*FONT_BUF_RATIO)/100;//0x5000;//0x80000;
#endif

			DBG_IND("QV w=%d, %d, MemSize=0x%x\r\n", FontWidth,FontHight,pInfo->MemSize);

			sprintf(pool_name,"DateImprint_QV");

			ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, pInfo->MemSize, ddr_id);

			if (ret != HD_OK) {
				DBG_ERR("QV alloc fail size 0x%x, ddr %d\r\n", pInfo->MemSize, ddr_id);
				return;
			}
			DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
			g_DateImprintPool[i].pool_va=(UINT32)va;
			g_DateImprintPool[i].pool_pa=(UINT32)pa;
			memset(va, 0, pInfo->MemSize);

	    		pInfo->MemAddr = g_DateImprintPool[i].pool_va;
			pInfo->copyEngine = GXIMG_CP_ENG1;
			pInfo->lockptn = FLGPTN_BIT(i); //i
		}
		else if ( i == DATE_IMPRINT_EVENT_SCR &&  g_DateImprintPool[i].pool_va == 0) {
				pInfo->MemSize = 0x80000;

				UiDateImprint_GetStampFont(CFG_SCREENNAIL_W,(char**)&pFont);

#if defined(_UI_STYLE_LVGL_)
				pInfo->MemSize = UIDateImprint_GetStampMemSize_LVGL((const lv_font_t *)pFont, str);
#else
				GxGfx_SetTextStroke((const FONT *)pFont, FONTSTYLE_NORMAL, SCALE_1X);
				GxGfx_Text(0, 0, 0, UiDateImprint_InitStrBuf()); //not really draw
				szStamp = GxGfx_GetTextLastSize(); //just get text size

				FontWidth=ALIGN_CEIL(szStamp.w ,8);
				FontHight=ALIGN_CEIL(szStamp.h, 2);
				pInfo->MemSize = (FontWidth*FontHight*FONT_BUF_RATIO)/100;//0x5000;//0x80000;
#endif


				DBG_IND("SRC w=%d, %d, MemSize=0x%x\r\n", FontWidth,FontHight,pInfo->MemSize);

				sprintf(pool_name,"DateImprint_SCR");

				ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, pInfo->MemSize, ddr_id);

				if (ret != HD_OK) {
					DBG_ERR("QV alloc fail size 0x%x, ddr %d\r\n", pInfo->MemSize, ddr_id);
					return;
				}
				DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
				g_DateImprintPool[i].pool_va=(UINT32)va;
				g_DateImprintPool[i].pool_pa=(UINT32)pa;
				memset(va, 0, pInfo->MemSize);

	        	pInfo->MemAddr = g_DateImprintPool[i].pool_va;
				pInfo->copyEngine = GXIMG_CP_ENG2;
				pInfo->lockptn = FLGPTN_BIT(i); //i
		}
		else if ( i == DATE_IMPRINT_EVENT_PRI &&  g_DateImprintPool[i].pool_va == 0) {
			pInfo->MemSize = 0x80000;
			UiDateImprint_GetStampFont(GetPhotoSizeWidth(PHOTO_MAX_CAP_SIZE),(char**)&pFont);

#if defined(_UI_STYLE_LVGL_)
			pInfo->MemSize = UIDateImprint_GetStampMemSize_LVGL((const lv_font_t *)pFont, str);
#else
			GxGfx_SetTextStroke((const FONT *)pFont, FONTSTYLE_NORMAL, SCALE_1X);
			GxGfx_Text(0, 0, 0, UiDateImprint_InitStrBuf()); //not really draw
			szStamp = GxGfx_GetTextLastSize(); //just get text size

			FontWidth=ALIGN_CEIL(szStamp.w ,8);
			FontHight=ALIGN_CEIL(szStamp.h, 2);
			pInfo->MemSize = (FontWidth*FontHight*FONT_BUF_RATIO)/100;//0x80000;
#endif

			DBG_IND("PRI w=%d, %d, MemSize=0x%x\r\n", FontWidth,FontHight,pInfo->MemSize);


			sprintf(pool_name,"DateImprint_PRI");

			ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, pInfo->MemSize, ddr_id);

			if (ret != HD_OK) {
				DBG_ERR("QV alloc fail size 0x%x, ddr %d\r\n", pInfo->MemSize, ddr_id);
				return;
			}
			DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
			g_DateImprintPool[i].pool_va=(UINT32)va;
			g_DateImprintPool[i].pool_pa=(UINT32)pa;
			memset(va, 0, pInfo->MemSize);

			pInfo->MemAddr = g_DateImprintPool[i].pool_va;
			pInfo->copyEngine = GXIMG_CP_ENG2;
			pInfo->lockptn =FLGPTN_BIT(i); //i
		}
		pInfo->isStrDirty = TRUE;
		pInfo->StrBuf[0] = 0;
		pInfo->pic_cnt = 0;
		genMaxw = GetPhotoSizeWidth(0) / 2;
		genMaxh = GetPhotoSizeHeight(0) * 0.04;
		pInfo->StampInfo.Img.loff[0] = genMaxw;
		pInfo->StampInfo.Img.loff[1] = genMaxw;
		pInfo->StampInfo.Img.pw[0] = genMaxw;
		pInfo->StampInfo.Img.ph[0] = genMaxh;
		if(PhotoExe_GetCapYUV420En()){
			pInfo->StampInfo.Img.pxlfmt = HD_VIDEO_PXLFMT_YUV420;//GX_IMAGE_PIXEL_FMT_YUV420_PACKED;
		}else{
			pInfo->StampInfo.Img.pxlfmt = HD_VIDEO_PXLFMT_YUV422;//GX_IMAGE_PIXEL_FMT_YUV422_PACKED;
		}
	}
	vos_flag_set(UI_DATEIMPRINT_FLG_ID, FLGPTN_BIT_ALL);
}
void UiDateImprint_DestroyBuff(void)
{
	UINT32 i, ret;
	for (i=0;i<DATE_IMPRINT_EVENT_MAX;i++) {

		if (g_DateImprintPool[i].pool_va != 0) {
			ret = hd_common_mem_free((UINT32)g_DateImprintPool[i].pool_pa, (void *)g_DateImprintPool[i].pool_va);
			if (ret != HD_OK) {
				DBG_ERR("FileIn release blk failed! (%d)\r\n", ret);
				break;
			}
			g_DateImprintPool[i].pool_va = 0;
			g_DateImprintPool[i].pool_pa = 0;
		}
	}
}

BOOL UiDateImprint_UpdateDate(char *StrBuf, UINT32 buff_len)
{
	char  *str;
	DBG_IND("\r\n");

	DateTime_Load();
	switch (UI_GetData(FL_DATE_STAMP)) {
		case DATEIMPRINT_DATE:
			str = DateTime_MakeYMD();
			DBG_IND("DATEIMPRINT_DATE, str=%s\r\n", str);
			if (strncmp(str,StrBuf,strlen(str))) {
				strncpy(StrBuf, str, buff_len - 1);
				return TRUE;
			}
			break;

		case DATEIMPRINT_DATE_TIME:
			str = DateTime_MakeYMDHMS();
			DBG_IND("DATEIMPRINT_DATE_TIME, str=%s\r\n", str);

			if (strncmp(str,StrBuf,strlen(str))) {
				strncpy(StrBuf, str, buff_len - 1);
				return TRUE;
			}
			break;
		default:
			StrBuf[0] = 0; //Empty String
	}
	return FALSE;
}
char  * UiDateImprint_InitStrBuf(void)
{
	char  *str=NULL;
	DateTime_Load();
	switch (UI_GetData(FL_DATE_STAMP)) {
		case DATEIMPRINT_DATE:
			str=DateTime_MakeYMD();
			break;

		case DATEIMPRINT_DATE_TIME:
			str=DateTime_MakeYMDHMS();
			break;
		default:
			break;
	}
	return str;
}

UINT32 UiDateImprint_GetMaxWorkBufSize(DS_STAMP_INFOR *stampInfo)
{
    return 0;//gximg_calc_require_size(stampInfo->Img.pw[0],stampInfo->Img.pw[0],stampInfo->Img.pxlfmt,stampInfo->Img.loff[0]);
}

void UiDateImprint_SelStampFont(UINT32 uiImageWidth, char **font, UINT32 *ScaleFactor)
{
#if defined(_UI_STYLE_LVGL_)
	*ScaleFactor = 65536;

	lv_plugin_res_id red_id;

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
		red_id = LV_USER_CFG_STAMP_FONT_ID_XXL;
	}

	*font = (char *) lv_plugin_get_font(red_id)->font;

#else
	if (uiImageWidth >= 3840) {
		*font = (char *)gDateStampFontTbl56x96;
		*ScaleFactor = 65536;
	}
	else if(uiImageWidth >=3600) {
		*font = (char *)&gDateStampFontTbl56x96;
		*ScaleFactor = 65536*95/100;
	}
	else if(uiImageWidth >=3200) {
		*font = (char *)&gDateStampFontTbl56x96;
		*ScaleFactor = 65536*90/100;
	}
	else if(uiImageWidth >=2880) {
		*font = (char *)&gDateStampFontTbl42x72;
		*ScaleFactor = 65536;
	}
	else if(uiImageWidth >=1920) {
		*font = (char *)&gDateStampFontTbl36x60;
		*ScaleFactor = 65536;
	}
	else if(uiImageWidth >=1080) {
		*font = (char *)&gDateStampFontTbl26x44;
		*ScaleFactor = 65536;
	}
	else if(uiImageWidth >=640) {
		*font = (char *)&gDateStampFontTbl12x20;
		*ScaleFactor = 65536;
	}
	else if(uiImageWidth >=320) {
		*font = (char *)&gDateStampFontTbl10x16;
		*ScaleFactor = 65536;
	}
	else {
		*font = (char *)&gDateStampFontTbl10x16;
		*ScaleFactor = 65536/3;
	}
#endif


}
void UiDateImprint_GetStampFont(UINT32 uiImageWidth, char **font)
{
#if defined(_UI_STYLE_LVGL_)

		lv_plugin_res_id red_id;

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
			red_id = LV_USER_CFG_STAMP_FONT_ID_XXL;
		}

		*font = (char *) lv_plugin_get_font(red_id)->font;

#else
	if (uiImageWidth >= 3840) {
		*font = (char *)&gDateStampFontTbl56x96;
	}
	else if(uiImageWidth >=3600) {
		*font = (char *)&gDateStampFontTbl56x96;
	}
	else if(uiImageWidth >=3200) {
		*font = (char *)&gDateStampFontTbl56x96;
	}
	else if(uiImageWidth >=2880) {
		*font = (char *)&gDateStampFontTbl42x72;
	}
	else if(uiImageWidth >=1920) {
		*font = (char *)&gDateStampFontTbl36x60;
	}
	else if(uiImageWidth >=1080) {
		*font = (char *)&gDateStampFontTbl26x44;
	}
	else if(uiImageWidth >=640) {
		*font = (char *)&gDateStampFontTbl12x20;
	}
	else if(uiImageWidth >=320) {
		*font = (char *)&gDateStampFontTbl10x16;
	}
	else {
		*font = (char *)&gDateStampFontTbl10x16;
	}
#endif
}

DATE_IMPRINT_INFO   *UiDateImprint_Event2Info(IMG_CAP_DATASTAMP_EVENT  event)
{
	DATE_IMPRINT_INFO   *pInfo;
	if (event == CAP_DS_EVENT_QV)
		pInfo = &pCtrl->info[DATE_IMPRINT_EVENT_QV];
	else if (event == CAP_DS_EVENT_SCR)
		pInfo = &pCtrl->info[DATE_IMPRINT_EVENT_SCR];
	else if (event == CAP_DS_EVENT_PRI)
		pInfo = &pCtrl->info[DATE_IMPRINT_EVENT_PRI];
    else
		pInfo = NULL;
	return pInfo;
}


INT32 UiDateImprint_GenYuvData(DATE_IMPRINT_INFO   *pInfo)
{
#if 1
	FONT_CONV_IN  *pIn =  &pInfo->tYuvDesc;
	FONT_CONV_OUT *pOut = &pInfo->tYuvInfo;
	UINT32 photo_w = pInfo->pCapInfo->ImgInfo.ch[0].width;
	//UINT32 photo_h = pInfo->pCapInfo->ImgInfo.ch[0].height;

	DBG_IND("\r\n");
	pIn->MemAddr = pInfo->MemAddr;
	pIn->MemSize = pInfo->MemSize;
	pIn->pStr = pInfo->StrBuf;

#if defined(_UI_STYLE_LVGL_)
	pIn->ciSolid = LV_USER_CFG_STAMP_COLOR_TEXT;
	pIn->ciFrame = LV_USER_CFG_STAMP_COLOR_FRAME;
	pIn->ciTransparet = LV_USER_CFG_STAMP_COLOR_BACKGROUND;
#else
	pIn->ciSolid = DATE_COLOR_RED;
	pIn->ciFrame = DATE_COLOR_TRANSPARENT;
	pIn->ciTransparet = DATE_COLOR_TRANSPARENT;
#endif
	if(PhotoExe_GetCapYUV420En()){
		pIn->Format = HD_VIDEO_PXLFMT_YUV420;//GX_IMAGE_PIXEL_FMT_YUV420_PACKED;
	}else{
		pIn->Format = HD_VIDEO_PXLFMT_YUV422;//GX_IMAGE_PIXEL_FMT_YUV422_PACKED;
	}
	pIn->bEnableSmooth = FALSE;
	//ImgSize = photo_w*photo_h;
#if 0
	if (photo_w * photo_h <= 160 * 120) {
		//thumbnail Size
		pIn->pFont = (FONT *)gDemo_SmallFont;
		pIn->ScaleFactor = 65536 / 3; //0.33x
	}
	else if (photo_w * photo_h <= 640 * 480) {
		//VGA Size
		pIn->pFont = (FONT *)gDemo_SmallFont;
		pIn->ScaleFactor = 65536; //1.0x
	} else {
		UINT32 font_h, stamp_ratio;
		pIn->pFont = (FONT *)gDemo_BigFont;

		font_h = 190;
		stamp_ratio = 0x00000A00; //0.04x
		pIn->ScaleFactor = photo_h * stamp_ratio / font_h;
	}
#else
	DBG_IND("photo_w=%d\r\n",photo_w);
	UiDateImprint_SelStampFont(photo_w, (char**)&pIn->pFont, &pIn->ScaleFactor);
#endif

#if defined(_UI_STYLE_LVGL_)
	if (FontConv_LVGL(pIn, pOut) != E_OK) {
		DBG_ERR("FontConv err\r\n");
		return -1;
	}
#else
	if (FontConv(pIn, pOut) != E_OK) {
		DBG_ERR("FontConv err\r\n");
		return -1;
	}
#endif



#endif
	return 0;
}

INT32 UiDateImprint_CalcPosition(DATE_IMPRINT_INFO   *pInfo, UPOINT *pos)
{
#if 1
	IMG_CAP_DATASTAMP_INFO *pCapInfo = pInfo->pCapInfo;
	FONT_CONV_OUT *pYuvInfo = &pInfo->tYuvInfo;

	UINT32 photo_w = pCapInfo->ImgInfo.ch[0].width;
	UINT32 photo_h = pCapInfo->ImgInfo.ch[0].height;

	//if (pYuvInfo->GenImg.Width > photo_w)
	if ((pYuvInfo->GenImg.pw[0]) > photo_w)
	{
		DBG_ERR("event=%d, GenImg w %d> photo_w %d\r\n",pCapInfo->event,(pYuvInfo->GenImg.pw[0]),photo_w);
		return -1;
	}
	//if (pYuvInfo->GenImg.Height > photo_h)
	if (pYuvInfo->GenImg.ph[0] > photo_h)
	{
		DBG_ERR("GenImg h %d> photo_h %d\r\n",pYuvInfo->GenImg.ph[0],photo_h);
		return -1;
	}
	DBG_IND("photo_w =%d, photo_h=%d, gen w=%d, h=%d\r\n",photo_w,photo_h,(pYuvInfo->GenImg.pw[0]),pYuvInfo->GenImg.ph[0]);
	pos->x = ALIGN_ROUND_4(photo_w - (pYuvInfo->GenImg.pw[0]) - pYuvInfo->GenImg.ph[0]);

	/* check overflow */
	if(photo_h < ALIGN_ROUND_4(photo_h - pYuvInfo->GenImg.ph[0] - pYuvInfo->GenImg.ph[0])){
		pos->y = 0;
	}
	else{
		pos->y = ALIGN_ROUND_4(photo_h - pYuvInfo->GenImg.ph[0] - pYuvInfo->GenImg.ph[0]);
	}

	DBG_IND("pos->x =%d, pos->y=%d\r\n",pos->x, pos->y);

#endif
	return 0;
}

ER UiDateImprint_UpdateData(DATE_IMPRINT_INFO   *pInfo)
{
#if 1
	DS_STAMP_INFOR  *pStampInfo = &pInfo->StampInfo;
	FONT_CONV_OUT   *pYuvInfo = &pInfo->tYuvInfo;
	UPOINT           pos = {0};

	if (UiDateImprint_GenYuvData(pInfo)< 0){
		DBG_ERR("GenYuvData fail\r\n");
		return E_SYS;
	}
	if (UiDateImprint_CalcPosition(pInfo,&pos) < 0){
		DBG_ERR("CalcPosition fail\r\n");
		return E_SYS;
	}
	pStampInfo->PosX = pos.x;
	pStampInfo->PosY = pos.y;
	pStampInfo->Img = pYuvInfo->GenImg;

	//memcpy(&pStampInfo->Img, &pYuvInfo->GenImg, sizeof(VDO_FRAME));

	pStampInfo->ColorKey = COLOR_MAKE_YUVD(pYuvInfo->ColorKeyY, pYuvInfo->ColorKeyCb, pYuvInfo->ColorKeyCr);
	pStampInfo->StampWeight = 255;
	DBG_IND("MemMax Use = %d bytes, MemMin Use=%d bytes\r\n", pYuvInfo->UsedMaxMemSize, pYuvInfo->UsedMemSize);
#endif
	return E_OK;
}

void UiDateImprint_ChkUpdateData(DATE_IMPRINT_INFO   *pInfo)
{
#if 1
	UINT32 uiImageSize, ScreenNailSize;
	UINT32 bScreenNailEn=0;
	UINT32 bGenEn=0;

	ScreenNailSize = CFG_SCREENNAIL_SIZE;
	uiImageSize = UI_GetData(FL_PHOTO_SIZE);
	if (uiImageSize < ScreenNailSize) {
		bScreenNailEn=1;
	}

	if(pInfo->pCapInfo->event == CAP_DS_EVENT_SCR && bScreenNailEn){
		bGenEn=1;
	}else if(pInfo->pCapInfo->event == CAP_DS_EVENT_PRI && bScreenNailEn==0){
		bGenEn=1;
	}
	DBG_IND("event=%d, bScreenNailEn=%d,  bGenEn=%d\r\n",pInfo->pCapInfo->event,bScreenNailEn,bGenEn);

	//if (pInfo->pCapInfo->event == CAP_DS_EVENT_QV || pInfo->pCapInfo->event == CAP_DS_EVENT_SCR) {
	if (pInfo->pCapInfo->event == CAP_DS_EVENT_QV || bGenEn) {
 		if ((UI_GetData(FL_CONTINUE_SHOT) == CONTINUE_SHOT_BURST_3)) {
			if (pInfo->pic_cnt == 0)
				pInfo->isStrDirty = UiDateImprint_UpdateDate(pInfo->StrBuf,sizeof(pInfo->StrBuf));
			else
				pInfo->isStrDirty = FALSE;
		}
		else {
			pInfo->isStrDirty = UiDateImprint_UpdateDate(pInfo->StrBuf,sizeof(pInfo->StrBuf));
		}
	}
	#if 0
	// need to update primary info to screenail
	if (pInfo->pCapInfo->event == CAP_DS_EVENT_PRI)
	{
		DATE_IMPRINT_INFO   *pScrInfo;

		pScrInfo= UiDateImprint_Event2Info(CAP_DS_EVENT_SCR);
		pScrInfo->isStrDirty = pInfo->isStrDirty;
		if (pInfo->isStrDirty == TRUE){
			strncpy(pScrInfo->StrBuf,pInfo->StrBuf,sizeof(pScrInfo->StrBuf));
		}
	}
	#else
	// need to update primary info to screenail
	//if (pInfo->pCapInfo->event == CAP_DS_EVENT_SCR)
	if (bGenEn && pInfo->pCapInfo->event == CAP_DS_EVENT_SCR)
	{
		DATE_IMPRINT_INFO   *pScrInfo;
		pScrInfo= UiDateImprint_Event2Info(CAP_DS_EVENT_PRI);

		pScrInfo->isStrDirty = pInfo->isStrDirty;
		if (pInfo->isStrDirty == TRUE){
			strncpy(pScrInfo->StrBuf,pInfo->StrBuf,sizeof(pScrInfo->StrBuf));
		}
	}
	#endif
	pInfo->pic_cnt++;
#endif
}
ER UiDateImprint_CopyData(DS_STAMP_INFOR *stampInfo,HD_VIDEO_FRAME* Img, UINT32  copyEngine)
{
	ER     ret =E_OK;
#if 1
	//UINT32          BufSize;
	//IMG_BUF         tmpImg = {0};
	IRECT  srcRegion;
	IPOINT DstLocation;


	DBG_IND("\r\n");

#if 0
	BufSize = UiDateImprint_GetMaxWorkBufSize(stampInfo);
	if (stampInfo->StampWeight < 255 && BufSize > WorkBufSize)
	{
	    DBG_ERR("WorkBufSize too small 0x%.8x > 0x%.8x\r\n", BufSize, WorkBufSize);
	    return E_SYS;
	}
#endif


	// the StampWeight is a alphaPlane Pointer
	if (stampInfo->StampWeight > 255)
	{
		//UINT8*    alphaPlane = (UINT8*)stampInfo->StampWeight;

		DstLocation.x = stampInfo->PosX;
		DstLocation.y = stampInfo->PosY;
        	VF_GFX_COPY param ={0};
		param.src_img=stampInfo->Img;
		param.src_region.x=0;
		param.src_region.y=0;
		param.src_region.w=stampInfo->Img.loff[0];
		param.src_region.h=stampInfo->Img.ph[0];
		param.dst_img=*Img;
		param.dst_pos.x=DstLocation.x;
		param.dst_pos.y=DstLocation.y;
		param.alpha=stampInfo->StampWeight;
		param.colorkey=stampInfo->ColorKey;
		param.engine=copyEngine;
		//ret = GxImg_CopyBlendingDataEx(&stampInfo->Img,REGION_MATCH_IMG,Img,&DstLocation, alphaPlane);
		//ret = gximg_copy_blend_data_ex(&stampInfo->Img,GXIMG_REGION_MATCH_IMG,Img,&DstLocation, alphaPlane);

		ret = vf_gfx_copy(&param);
	}
	else if (stampInfo->StampWeight == 255)
	{
		srcRegion.x = stampInfo->PosX;
		srcRegion.y = stampInfo->PosY;
		srcRegion.w = stampInfo->Img.loff[0];
		srcRegion.h = stampInfo->Img.ph[0];
		//DstLocation.x = stampInfo->PosX;
		//DstLocation.y = stampInfo->PosY;
        	VF_GFX_COPY param ={0};
		param.src_img=stampInfo->Img;
		param.src_region.x=0;
		param.src_region.y=0;
		param.src_region.w=srcRegion.w;
		param.src_region.h=srcRegion.h;

		param.dst_img=*Img;
		param.dst_pos.x=srcRegion.x;
		param.dst_pos.y=srcRegion.y;
		param.colorkey=stampInfo->ColorKey;
		param.engine=copyEngine;
		//DBG_ERR("src_img=0x%x, 0x%x, 0x%x, 0x%x\r\n",&param.src_img,param.src_img,Img,*Img);

		//ret = GxImg_CopyColorkeyData(Img,&srcRegion,&stampInfo->Img,REGION_MATCH_IMG,stampInfo->ColorKey,FALSE, copyEngine);
		//ret = gximg_copy_color_key_data(Img,&srcRegion,&stampInfo->Img,GXIMG_REGION_MATCH_IMG,stampInfo->ColorKey,FALSE, copyEngine);
		//ret = vf_gfx_copy(&param);
		//ret = vf_gfx_I8_colorkey(&param);
		ret = vf_gfx_copy(&param);
	}
	else
	{
		DBG_ERR("Not support StampWeight = %d\r\n",stampInfo->StampWeight);
		/*
		srcRegion.x = stampInfo->PosX;
		srcRegion.y = stampInfo->PosY;
		srcRegion.w = stampInfo->Img.Width;
		srcRegion.h = stampInfo->Img.Height;
		DstLocation.x = stampInfo->PosX;
		DstLocation.y = stampInfo->PosY;
		GxImg_InitBuf(&tmpImg,stampInfo->Img.Width,stampInfo->Img.Height,stampInfo->Img.PxlFmt,stampInfo->Img.Width,WorkBuf,WorkBufSize);
		DBG_MSG("w= %d, h=%d, colorkey=0x%x\r\n",tmpImg.Width,tmpImg.Height,stampInfo->ColorKey);
		ret = GxImg_CopyData(Img,&srcRegion,&tmpImg,REGION_MATCH_IMG);
		if (ret == E_OK)
		{
		//(B == KEY) ? A:B -> destination with color key (=)
		ret = GxImg_CopyColorkeyData(&tmpImg,REGION_MATCH_IMG,&stampInfo->Img,REGION_MATCH_IMG,stampInfo->ColorKey,FALSE, copyEngine);
		if (ret == E_OK)
		{
		ret = GxImg_CopyBlendingData(&tmpImg,REGION_MATCH_IMG,Img, &DstLocation,stampInfo->StampWeight, copyEngine);
		}
		}
		*/
	}
#endif
    return ret;
}





BOOL UiDateImprint_GenData(IMG_CAP_DATASTAMP_INFO *pCapInfo)
{
	DATE_IMPRINT_INFO   *pInfo;
	//IMG_BUF Img = {0};
	HD_VIDEO_FRAME Img = {0};
	UINT32  LineOff[MAX_PLANE_NUM];
	UINT32  PxlAddr[MAX_PLANE_NUM];
#if USE_DBG_STAMP
	UINT32  time_start,time_end;
#endif
	HD_RESULT hd_ret;

	DBG_IND("event %d, width =%d, height =%d, line_ofs[0]=%d, line_ofs[1]=%d\r\n",pCapInfo->event,pCapInfo->ImgInfo.ch[0].width,pCapInfo->ImgInfo.ch[0].height,pCapInfo->ImgInfo.ch[0].line_ofs,pCapInfo->ImgInfo.ch[1].line_ofs);

	pInfo = UiDateImprint_Event2Info(pCapInfo->event);
	if (pInfo == NULL){
		DBG_ERR("Unknown event %d\r\n",pCapInfo->event);
		return FALSE;
	}
	UiDateImprint_Lock(pInfo->lockptn);
	pInfo->pCapInfo = pCapInfo;
	UiDateImprint_ChkUpdateData(pInfo);
	DBG_IND("isStrDirty %d\r\n",pInfo->isStrDirty);
#if USE_DBG_STAMP
	time_start = Perf_GetCurrent();
#endif
	if (pInfo->isStrDirty) {
		if (UiDateImprint_UpdateData(pInfo)!=E_OK){
			return FALSE;
		}
	}
#if 0
	GxImg_DumpBuf(&pYuvInfo->GenImg, TRUE);
#endif
	PxlAddr[0] = pCapInfo->ImgInfo.pixel_addr[0];
	PxlAddr[1] = pCapInfo->ImgInfo.pixel_addr[1];
	LineOff[0] = pCapInfo->ImgInfo.ch[0].line_ofs;
	LineOff[1] = pCapInfo->ImgInfo.ch[1].line_ofs;

	if(PhotoExe_GetCapYUV420En()){
		//gximg_init_buf_ex(&Img, pCapInfo->ImgInfo.ch[0].width, pCapInfo->ImgInfo.ch[0].height, GX_IMAGE_PIXEL_FMT_YUV420_PACKED, LineOff, PxlAddr);
		if ((hd_ret = vf_init_ex(&Img, pCapInfo->ImgInfo.ch[0].width, pCapInfo->ImgInfo.ch[0].height, HD_VIDEO_PXLFMT_YUV420, LineOff, PxlAddr)) != HD_OK) {
			DBG_ERR("vf_init_ex 420 failed(%d)\r\n", hd_ret);
		}
	}else{
		//gximg_init_buf_ex(&Img, pCapInfo->ImgInfo.ch[0].width, pCapInfo->ImgInfo.ch[0].height, GX_IMAGE_PIXEL_FMT_YUV422_PACKED, LineOff, PxlAddr);
		if ((hd_ret = vf_init_ex(&Img, pCapInfo->ImgInfo.ch[0].width, pCapInfo->ImgInfo.ch[0].height, HD_VIDEO_PXLFMT_YUV422, LineOff, PxlAddr)) != HD_OK) {
			DBG_ERR("vf_init_ex 422 failed(%d)\r\n", hd_ret);
		}
	}
	//DBG_ERR("Img.phy_addr=0x%x,0x%x,0x%x\r\n",Img.phy_addr[0],Img.phy_addr[1],Img.phy_addr[2]);

#if 1
	UiDateImprint_CopyData(&pInfo->StampInfo,&Img,pInfo->copyEngine);
#else
	IRECT  srcRegion;
	srcRegion.x = pInfo->StampInfo.PosX;
	srcRegion.y = pInfo->StampInfo.PosY;
	srcRegion.w = pInfo->StampInfo.Img.loff[0];
	srcRegion.h = pInfo->StampInfo.Img.ph[0];
    	VF_GFX_COPY param ={0};
	#if 0
	param.src_img=Img;
	param.src_region.x=srcRegion.x;
	param.src_region.y=srcRegion.y;
	param.src_region.w=srcRegion.w;
	param.src_region.h=srcRegion.h;
	param.dst_img=pInfo->StampInfo.Img;

	param.dst_pos.x=0;
	param.dst_pos.y=0;
	#else
	param.src_img=pInfo->StampInfo.Img;
	param.src_region.x=0;
	param.src_region.y=0;
	param.src_region.w=srcRegion.w;
	param.src_region.h=srcRegion.h;

	param.dst_img=Img;
	param.dst_pos.x=pInfo->StampInfo.PosX;
	param.dst_pos.y=pInfo->StampInfo.PosY;
	#endif
	param.colorkey=pInfo->StampInfo.ColorKey;
	param.engine=pInfo->copyEngine;

	//DBG_ERR("param.src_img.phy_addr=0x%x,0x%x,0x%x\r\n",param.src_img.phy_addr[0],param.src_img.phy_addr[1],param.src_img.phy_addr[2]);

#if 0
{
    		FST_FILE hFile = NULL;
    		UINT32 filesize;
            char filenamey[32]={0};
            static UINT32 count=0;
            sprintf(filenamey,"A:\\test_STASY_%d.RAW",count);
            count++;
    		DBG_DUMP("wxh=%dx%d,loff=%d,  %s...\r\n", param.dst_img.pw[0], param.dst_img.ph[0],param.dst_img.loff[0],filenamey);
    		filesize = param.dst_img.loff[0]*param.dst_img.ph[0];
    		hFile = FileSys_OpenFile(filenamey, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
    		FileSys_WriteFile(hFile, (UINT8 *)param.dst_img.phy_addr[0], &filesize, 0, NULL);
    		FileSys_CloseFile(hFile);
}
#endif
	//IRECT    dst_region={388,440,240,20};
	//gximg_fill_data((VDO_FRAME *)&(Img), GXIMG_REGION_MATCH_IMG, COLOR_YUV_RED);
	//gximg_fill_data((VDO_FRAME *)&(param.dst_img), GXIMG_REGION_MATCH_IMG, COLOR_YUV_RED);

	//ret = GxImg_CopyColorkeyData(Img,&srcRegion,&stampInfo->Img,REGION_MATCH_IMG,stampInfo->ColorKey,FALSE, copyEngine);
	//ret = gximg_copy_color_key_data(Img,&srcRegion,&stampInfo->Img,GXIMG_REGION_MATCH_IMG,stampInfo->ColorKey,FALSE, copyEngine);
	//vf_gfx_copy(&param);
	vf_gfx_I8_colorkey(&param);
#endif


#if USE_DBG_STAMP
	time_end = Perf_GetCurrent();
	DBG_DUMP("Time Use = %d ms\r\n", (time_end-time_start) / 1000);
#endif
	UiDateImprint_UnLock(pInfo->lockptn);
	return TRUE;
}




