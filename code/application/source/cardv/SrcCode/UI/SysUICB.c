/*
    System UI Callback

    System Callback for UI Module.

    @file       SysUICB.c
    @ingroup    mIPRJSYS
    @note       None

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include "UIWnd/UIFlow.h"
#include "GxDisplay.h"
#include "UIGraphics.h"
#include "UIBuffer.h"
#include "sys_mempool.h"

//---------------------SysUICB Debug Definition -----------------------------
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          SysUICB
#define __DBGLVL__          2
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////


//---------------------SysUICB Global Variables -----------------------------
extern VControl *gUxCtrlTypeList[];
extern VControl *gAppCtrlTypeList[];

//---------------------SysUICB Prototype Declaration  -----------------------
//---------------------SysUICB Public API  ----------------------------------
//---------------------SysUICB Private API ----------------------------------

///////////////////////////////////////////////////////////////////////////////
void UI_CustomDrawShowImage(UIScreen ScreenObj, ITEM_IMAGE *pShowObj);

#define icst_prom0  103
#define icst_prom1  88
#define icst_prom2  183
#define icst_prom3  198
#define YUV_GET_R(y,u,v)    ( ((INT32)y) + ((INT32)(v)-128) + ((icst_prom0*((INT32)(v)-128))>>8) )
#define YUV_GET_G(y,u,v)    ( ((INT32)y) - ((icst_prom1*((INT32)(u)-128))>>8) - ((icst_prom2*((INT32)(v)-128))>>8) )
#define YUV_GET_B(y,u,v)    ( ((INT32)y) + ((INT32)(u)-128) + ((icst_prom3*((INT32)(u)-128))>>8) )
#define CLAMP_TO_BYTE(v)    (((v) < 0)?0:((v>255)?255:(v)))
#define YUV_GET_RGB(y,u,v,r,g,b)    \
	{   \
		r = YUV_GET_R(y,u,v);   r = CLAMP_TO_BYTE(r);   \
		g = YUV_GET_G(y,u,v);   g = CLAMP_TO_BYTE(g);   \
		b = YUV_GET_B(y,u,v);   b = CLAMP_TO_BYTE(b);   \
	}

#if (DISPLAY_OSD_FMT == DISP_PXLFMT_ARGB8565_PK)

PALETTE_ITEM gDemoKit_Palette_RGBA5658[256];


UINT32 COLOR_YUV_2_RGB565(UINT32 yuv, UINT32 a)
{
	INT32 r, g, b;
	UINT32 argb8565;
	INT32 y, u, v;

	v = COLOR_YUVA_GET_V(yuv);
	u = COLOR_YUVA_GET_U(yuv);
	y = COLOR_YUVA_GET_Y(yuv);

	YUV_GET_RGB(y, u, v, r, g, b);
	argb8565 = COLOR_MAKE_RGB565(r, g, b, a);
	return argb8565;
}

void PALETTE_PREPARE_DATA(void)
{
	UINT32 i = 0;
	for (i = 0; i < 256; i++) {
		gDemoKit_Palette_RGBA5658[i] = COLOR_YUV_2_RGB565(gDemoKit_Palette_Palette[i], 0xff);
        //DBG_DUMP("rgba5658 = 0x%08x\r\n", gDemoKit_Palette_RGBA5658[i]);

	}
	gDemoKit_Palette_RGBA5658[0] = COLOR_YUV_2_RGB565(gDemoKit_Palette_Palette[0], 0x00);
}

UINT32 ColorTable[2] = {(UINT32)gDemoKit_Palette_RGBA5658, (UINT32)gDemoKit_Palette_RGBA5658};

#elif (DISPLAY_OSD_FMT == DISP_PXLFMT_ARGB8888_PK)

PALETTE_ITEM gDemoKit_Palette_RGBA8888[256];
UINT32 ColorTable[2] = {(UINT32)gDemoKit_Palette_RGBA8888, (UINT32)gDemoKit_Palette_RGBA8888};

UINT32 COLOR_YUV_2_RGB888(UINT32 yuv, UINT32 a)
{
	INT32 r, g, b;
	UINT32 argb888;
	INT32 y, u, v;

	v = COLOR_YUVA_GET_V(yuv);
	u = COLOR_YUVA_GET_U(yuv);
	y = COLOR_YUVA_GET_Y(yuv);

	YUV_GET_RGB(y, u, v, r, g, b);
	argb888 = COLOR_MAKE_RGBA(r, g, b, a);
	return argb888;
}

void PALETTE_PREPARE_DATA(void)
{
	UINT32 i = 0;

	for (i = 0; i < 256; i++) {
		gDemoKit_Palette_RGBA8888[i] = COLOR_YUV_2_RGB888(gDemoKit_Palette_Palette[i], 0xff);
	}
	gDemoKit_Palette_RGBA8888[0] = COLOR_YUV_2_RGB888(gDemoKit_Palette_Palette[0], 0x00);

}
#elif (DISPLAY_OSD_FMT == DISP_PXLFMT_ARGB4444_PK)

PALETTE_ITEM gDemoKit_Palette_RGBA4444[256];
UINT32 ColorTable[2] = {(UINT32)gDemoKit_Palette_RGBA4444, (UINT32)gDemoKit_Palette_RGBA4444};

UINT32 COLOR_YUV_2_RGB444(UINT32 yuv, UINT32 a)
{
	INT32 r, g, b;
	UINT32 argb8565;
	INT32 y, u, v;

	v = COLOR_YUVA_GET_V(yuv);
	u = COLOR_YUVA_GET_U(yuv);
	y = COLOR_YUVA_GET_Y(yuv);

	YUV_GET_RGB(y, u, v, r, g, b);
	argb8565 = COLOR_MAKE_ARGB4444(r, g, b, a);
	return argb8565;
}
void PALETTE_PREPARE_DATA(void)
{
	UINT32 i = 0;

	for (i = 0; i < 256; i++) {
		gDemoKit_Palette_RGBA4444[i] = COLOR_YUV_2_RGB444(gDemoKit_Palette_Palette[i], 0xff);
	}
	gDemoKit_Palette_RGBA4444[0] = COLOR_YUV_2_RGB444(gDemoKit_Palette_Palette[0], 0x00);
}
#elif (DISPLAY_OSD_FMT == DISP_PXLFMT_INDEX8)

UINT32 COLOR_YUV_2_RGB(UINT32 yuv)
{
	INT32 r, g, b ,a;
	UINT32 argb888;
	INT32 y, u, v;

	v = COLOR_YUVA_GET_V(yuv);
	u = COLOR_YUVA_GET_U(yuv);
	y = COLOR_YUVA_GET_Y(yuv);
	a = COLOR_YUVA_GET_A(yuv);

	YUV_GET_RGB(y, u, v, r, g, b);
	argb888 = COLOR_MAKE_RGBA(r, g, b, a);
	return argb888;
}


void PALETTE_PREPARE_DATA(void)
{
	UINT32 i = 0;
    PALETTE_ITEM gDemoKit_Palette_Index8[256];

	for (i = 0; i < 256; i++) {
		gDemoKit_Palette_Index8[i] = COLOR_YUV_2_RGB(gDemoKit_Palette_Palette[i]);
	}

	//copy convert palette to org palette table
    memcpy((void *)gDemoKit_Palette_Palette,(void *)gDemoKit_Palette_Index8,sizeof(UINT32)*256);
}
#endif


/////////////////////////////////////////////////////////////////////////
//
//  Customize Display Flow
//
/////////////////////////////////////////////////////////////////////////

//font table
FONT *UIFontTable[2] = {
	(FONT *)gDemoKit_Font,
	NULL
};

DC *pDrawDC = 0; //current draw buffer
DC DrawDC = {0};

void UI_GfxInit(void)
{
	UINT32              uiPoolAddr =0;

	//Initial all states, all buffers
	DBG_FUNC_BEGIN("\r\n");

	uiPoolAddr = mempool_gxgfx_temp;

	////////////////////////////////////////////////////////////////

	GxGfx_Config(CFG_STRING_BUF_SIZE, 256);
	//Init Gfx
	GxGfx_Init((UINT32 *)uiPoolAddr, POOL_SIZE_GFX_TEMP);     //initial Graphics

	//set default shape, text, image state for GxGfx
	GxGfx_SetAllDefault();
	//set custom image state for GxGfx
	GxGfx_SetImageStroke(ROP_KEY, IMAGEPARAM_DEFAULT);
	GxGfx_SetImageColor(IMAGEPALETTE_DEFAULT, IMAGEPARAM_DEFAULT);
	GxGfx_SetImageTable((const IMAGE_TABLE *)gDemoKit_Image);
	//set custom text state for GxGfx
	GxGfx_SetTextColor(TEXTFORECOLOR1_DEFAULT, TEXTFORECOLOR2_DEFAULT, TEXTFORECOLOR3_DEFAULT);
	GxGfx_SetTextStroke((const FONT *)gDemoKit_Font, FONTSTYLE_NORMAL, SCALE_1X);
	GxGfx_SetStringTable((const STRING_TABLE *)Get_LanguageTable());
}


void UI_DispInit(void)
{
	//Initial all states, all buffers
	DBG_FUNC_BEGIN("\r\n");

    UI_RegisterBuf();

	////////////////////////////////////////////////////////////////

	//Init UI show
	UI_SetLayoutSize(TOOL_LAYOUT_W, TOOL_LAYOUT_H);

#if ((DISPLAY_OSD_FMT == DISP_PXLFMT_ARGB8565_PK)||(DISPLAY_OSD_FMT == DISP_PXLFMT_ARGB8888_PK)||(DISPLAY_OSD_FMT == DISP_PXLFMT_ARGB4444_PK))
	Ux_SetShowObjFunc(CMD_Image, (SHOWOBJ_FUNC_PTR)UI_CustomDrawShowImage);
#endif
}

//Init renderer for UIControl show process
void UI_ControlShowInit(void)
{
	DBG_FUNC_BEGIN("\r\n");

	//set palette rendering-time switch table for Ux_Redraw()
	//Init UI show resource
#if (DISPLAY_OSD_FMT == DISP_PXLFMT_ARGB8888_PK)
	PALETTE_PREPARE_DATA();
	Ux_SetPaletteTable((MAPPING_ITEM **)ColorTable);
	Ux_SetGlobalColorKey(TRUE, COLOR_RGBA_GET_COLOR(gDemoKit_Palette_RGBA8888[0]));
#elif (DISPLAY_OSD_FMT == DISP_PXLFMT_ARGB8565_PK)
	PALETTE_PREPARE_DATA();
	Ux_SetPaletteTable((MAPPING_ITEM **)ColorTable);
	Ux_SetGlobalColorKey(TRUE, COLOR_RGB565_GET_COLOR(gDemoKit_Palette_RGBA5658[0]));
    #elif (DISPLAY_OSD_FMT == DISP_PXLFMT_ARGB4444_PK)
	PALETTE_PREPARE_DATA();
	Ux_SetPaletteTable((MAPPING_ITEM **)ColorTable);
	Ux_SetGlobalColorKey(TRUE, COLOR_RGB565_GET_COLOR(gDemoKit_Palette_RGBA4444[0]));

#elif (DISPLAY_OSD_FMT == DISP_PXLFMT_INDEX8)

    PALETTE_PREPARE_DATA();
	Ux_SetPaletteTable(0);
#else //(DISPLAY_OSD_FMT == DISP_PXLFMT_INDEX4)
	Ux_SetPaletteTable(0);
#endif
	//set colormap rendering-time switch table for Ux_Redraw()
	Ux_SetColorMapTable(0);
	//set font switch rendering-time table for Ux_Redraw()
	Ux_SetFontTable(UIFontTable);

	//Init UI framework render object
	Ux_SetRender(&demoRender);

}

UINT32 UI_GetOSDSize(void)
{
	UINT32 osd1_size = 0;
	//UI - manage by UIDisplay, buffer is fixed memory after OS start
    osd1_size =
          ALIGN_CEIL_32(GxGfx_CalcDCMemSize(DISPLAY_OSD_W, DISPLAY_OSD_H, TYPE_FB, DISPLAY_OSD_FMT));
    return osd1_size;
}
/////////////////////////////////////////////////////////////////////////////

void UI_CustomDrawShowImage(UIScreen ScreenObj, ITEM_IMAGE *pShowObj)
{
	PALETTE_ITEM *pPalette = Ux_GetPaletteTable(0);
	IMAGE  * pImage=0;
	UINT32 globalColorKey = 0;
	UINT32 bEnableGlobalKey = FALSE;

	if (pShowObj->Content != ICONID_NULL) {
		GxGfx_SetImageLayout(pShowObj->uiLayout, pShowObj->uiAlignment);

		Ux_GetGlobalColorKey(&bEnableGlobalKey,&globalColorKey);
		pImage = (IMAGE  *)GxGfx_GetImageID((UINT16)pShowObj->Content);

        if(!pImage){
            DBG_ERR("id %d is 0\r\n",pShowObj->Content);
            return ;
        }
		if(pImage->dc.uiPxlfmt == PXLFMT_INDEX8){
			if ((bEnableGlobalKey) && (pShowObj->uiBltROP == ROP_KEY))
				GxGfx_SetImageStroke(ROP_KEY, globalColorKey);
			else
				GxGfx_SetImageStroke(pShowObj->uiBltROP, pShowObj->uiParamROP);
		} else {
		    #if 0 //use UI tool choose imageEffect
		    //ARGB format,always transfer KEY to BLEND
			if(pShowObj->uiBltROP == ROP_KEY){
				pShowObj->uiBltROP = ROP_BLEND;
			}
            #endif
			GxGfx_SetImageStroke(pShowObj->uiBltROP, pShowObj->uiParamROP);
		}

		if (pPalette == 0) {
			GxGfx_SetImageColor(IMAGEPALETTE_DEFAULT, Ux_GetColorMapTable((UINT32)pShowObj->uiColorMapTable));
		} else {
			GxGfx_SetImageColor(pPalette, Ux_GetColorMapTable((UINT32)pShowObj->uiColorMapTable));
		}

		GxGfx_ImageInRect(((DC **)ScreenObj)[LAYER_OSD1], pShowObj->x1, pShowObj->y1, pShowObj->x2, pShowObj->y2, GxGfx_GetImageID((UINT16)pShowObj->Content));
	}
}


