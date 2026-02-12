
// UI Graphics的防止閃爍策略
//
//  1.配合UI Framework, 集中在BeginScreen()/EndScreen()中間, 進行畫面Buffer的Redraw 動作
//
//  2.配合UI Framework, 集中在EndScreen()的兩次WaitVD中間, 進行GxDisplay的Setup 動作
//


#include "UIFramework.h"
#include "UIFrameworkExt.h"
#include "GxDisplay.h"
#include "UIGraphics.h"
#include <kwrap/semaphore.h>
#include "UIView.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UIGfx
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////
#define BENCH_GFX   DISABLE

//Device Context
#define GxGfx_OSD 0
#define GxGfx_OSD2 1
#define GxGfx_VDO 2
#define GxGfx_VDO2 3


extern BOOL g_bDualUI;

UINT32 gDrawSemID = 0;

UINT32 layout_w = 0;
UINT32 layout_h = 0;

DC *_sys_pDC[4];
IPOINT _sys_Orig[4];
typedef DC **DCLIST;

#if (BENCH_GFX == ENABLE)
UINT32 A1 = 0, A2 = 0, A3 = 0, A4 = 0;
UINT32 B1 = 0, B2 = 0, B3 = 0, B4 = 0;
#endif


/////////////////////////////////////////////////////////////////////////
//
//  Draw Dirty
//
/////////////////////////////////////////////////////////////////////////
#define DIRTY_MASK      0x00ff
VControl *pScreenWnd = 0;
BOOL UI_IsDrawDirty(void);

BOOL UI_IsDrawDirty(void)
{
	Ux_GetFocusedWindow(&pScreenWnd);
	if ((pScreenWnd) && (pScreenWnd->flag & DIRTY_MASK)) {
		return TRUE;
	}
	return FALSE;
}



/////////////////////////////////////////////////////////////////////////
//
//  Flip Control
//
/////////////////////////////////////////////////////////////////////////
static BOOL g_bFlip = TRUE;
void UI_SetDisplayFlip(BOOL bEn);
BOOL UI_GetDisplayFlip(void);

BOOL UI_GetDisplayFlip(void)
{
	return g_bFlip;
}
//set next redraw not change buffer
//after next redraw ,bNotFlip would be set true
void UI_SetDisplayFlip(BOOL bEn)
{
	g_bFlip = bEn;
}

/////////////////////////////////////////////////////////////////////////
//
//  Dirty Control
//
/////////////////////////////////////////////////////////////////////////
BOOL bScreenDirty = FALSE;
void UI_SetDisplayDirty(BOOL bEn);

//set force dirty (even do not modify show or draw)
void UI_SetDisplayDirty(BOOL bEn)
{
	bScreenDirty = bEn;
}


/////////////////////////////////////////////////////////////////////////
//
//  Customize Display Flow
//
/////////////////////////////////////////////////////////////////////////

void UI_BeginShow(void)
{
}

void UI_EndShow(void)
{
}

void UI_SetLayoutSize(UINT32 Width, UINT32 Height)
{
	layout_w = Width;
	layout_h = Height;
}
extern DC *pDrawDC ; //current draw buffer

UIScreen UI_BeginScreen(void)
{
	DBG_FUNC_BEGIN("\r\n");

    _sys_pDC[0] = GxDisplay_BeginDraw(LAYER_OSD1);
#if (OSD_USE_ROTATE_BUFFER == ENABLE)
	_sys_pDC[0] = View_Window_BeginRotate(_sys_pDC[0]); //copy show to paint with rotate
#endif

	_sys_Orig[0] = GxGfx_GetOrigin(_sys_pDC[0]);

	GxGfx_SetCoord(_sys_pDC[0], layout_w, layout_h);
	GxGfx_SetWindow(_sys_pDC[0], 0, 0, layout_w - 1, layout_h - 1);

    GxGfx_Lock(); //for UI and FontCont share font table
	return (UIScreen)(DCLIST)(_sys_pDC);
}
#include "hdal.h"

void UI_EndScreen(UIScreen ScreenObj)
{
   	DBG_FUNC_BEGIN("\r\n");

    GxGfx_Unlock(); //for UI and FontCont share font table

	GxGfx_SetOrigin(_sys_pDC[0], _sys_Orig[0].x, _sys_Orig[0].y);
#if (OSD_USE_ROTATE_BUFFER == ENABLE)
	View_Window_EndRotate(); //copy paint to show with rotate
#endif
    GxDisplay_EndDraw(LAYER_OSD1, _sys_pDC[0]);

    GxDisplay_FlushEx(LAYER_OSD1, FALSE);
	//swap buffer
	GxDisplay_Flip(FALSE); //buffer flip

}

void UI_BeginWindow(VControl *pWnd, UIScreen ScreenObj)
{
	DC **pDCList = (DC **)ScreenObj;
	//set relative position by Window
	GxGfx_SetOrigin(pDCList[GxGfx_OSD],
					pWnd->rect.x1,
					pWnd->rect.y1);

}

void UI_EndWindow(VControl *pWnd, UIScreen ScreenObj)
{
	DBG_FUNC_BEGIN("\r\n");

}

void UI_BeginControl(VControl *pParent, VControl *pControl, UIScreen ScreenObj)
{
	DC **pDCList = (DC **)ScreenObj;
	Ux_RECT  rect = {0, 0, 0, 0};
	rect.x1 =  pControl->rect.x1;
	rect.y1 =  pControl->rect.y1;
	while (pParent) {
		rect.x1 += pParent->rect.x1;
		rect.y1 += pParent->rect.y1;
		pParent = pParent->pParent;
	}

	GxGfx_SetOrigin(pDCList[GxGfx_OSD], rect.x1, rect.y1);
}

void UI_EndControl(VControl *pCtrl, VControl *pControl, UIScreen ScreenObj)
{
   	DBG_FUNC_BEGIN("\r\n");

}

void UI_ClearScreen(UIScreen ScreenObj)
{
	DC **pDCList = (DC **)ScreenObj;

	DBG_MSG("^YClear OSD1\r\n");
	GxGfx_Clear(pDCList[GxGfx_OSD]);
}

UIRender demoRender = {
	UI_BeginScreen,
	UI_EndScreen,
	UI_BeginWindow,
	UI_EndWindow,
	UI_BeginControl,
	UI_EndControl,
	UI_ClearScreen
};


/////////////////////////////////////////////////////////////////////////////




