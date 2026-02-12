#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

#if _TODO
#include "GxVideo.h"
#include "GxDisplay.h"
#endif

UINT32 gPbViewState = PB_VIEW_STATE_NONE;
UINT32 gUserState = 0;//PB_DISPSRV_DRAW|PB_DISPSRV_FLIP;
static BOOL flipped = FALSE;

void PB_CoordSysConvert(PCOORDSYS_CONV pCSConv)
{
	pCSConv->prcOut->x = pCSConv->uiClientCoord_OriX + pCSConv->uiGlobalCoordX * pCSConv->uiClientCoord_RngW / pCSConv->uiGlobalCoord_RngW;
	pCSConv->prcOut->y = pCSConv->uiClientCoord_OriY + pCSConv->uiGlobalCoordY * pCSConv->uiClientCoord_RngH / pCSConv->uiGlobalCoord_RngH;
	pCSConv->prcOut->w = pCSConv->uiGlobalCoordW * pCSConv->uiClientCoord_RngW / pCSConv->uiGlobalCoord_RngW;
	pCSConv->prcOut->h = pCSConv->uiGlobalCoordH * pCSConv->uiClientCoord_RngH / pCSConv->uiGlobalCoord_RngH;
}

BOOL PB_LockThumb(UINT32 uiIdx)
{
	FLGPTN Flag;

	if (kchk_flg(FLG_ID_PLAYBACK, FLGPB_IMG_STOP)) {
		return E_NOEXS;
	}
	// Thumb view is pull mode
	if (gPbViewState == PB_VIEW_STATE_THUMB) {
		wai_flg(&Flag, FLG_ID_PLAYBACK, ((FLGPB_IMG_0) << uiIdx), TWF_ORW | TWF_CLR);
	}

	return E_OK;
}

BOOL PB_UnlockThumb(UINT32 uiIdx)
{
	if (gPbViewState == PB_VIEW_STATE_THUMB) {
		set_flg(FLG_ID_PB_DISP, (FLGPB_IMG_0) << uiIdx);
		return E_OK;
	} else {
		//Push Unlock is fake. just return E_OK
		set_flg(FLG_PB, FLGPB_PUSH_FLIPPED);
		return E_OK;
	}
}

static void xPB_DispSrv_Reset(void)
{
	gPbPushSrcIdx = 0;
	gPbViewState = PB_VIEW_STATE_NONE;
	gUserState = 0;
}


void PB_DispSrv_Cfg(void)
{
	// reset variable
	xPB_DispSrv_Reset();
}

void PB_DispSrv_SetDrawCb(PB_VIEW_STATE ViewState)
{
	// suspend state
	if (gUserState & PB_DISPSRV_SUSPEND) {
		return;
	}

	if (gPbViewState == ViewState) {
		return;
	}

	flipped = FALSE;
	gPbViewState = ViewState;

}

void PB_DispSrv_Trigger(void)
{
	FLGPTN Flag;

	if (g_fpCfg4NextCB) {
		g_fpCfg4NextCB();
	}

	// suspend should not trigger anymore
	if (gUserState & PB_DISPSRV_SUSPEND) {
		return;
	}

	// Thumb view is pull mode
	if (gPbViewState == PB_VIEW_STATE_THUMB) {
		if (gUserState & PB_DISPSRV_PUSH) {
			if (g_fpDispTrigCB) {
				g_fpDispTrigCB(&gPBImgBuf[gPbPushSrcIdx], FALSE);
			}
			return;
		}
		if (gUserState & PB_DISPSRV_DRAW) {
			//set_flg(FLG_ID_PB_DISP, FLGPB_IMG_0 | FLGPB_IMG_1);
			clr_flg(FLG_ID_PLAYBACK, FLGPB_IMG_0 | FLGPB_IMG_1);
			clr_flg(FLG_PB, FLGPB_PULL_DRAWEND);
			gUserState |= (PB_DISPSRV_PUSH & PB_DISPSRV_DRAW_MASK);
		}
		if (gUserState & PB_DISPSRV_FLIP) {
			clr_flg(FLG_PB, FLGPB_PULL_FLIPPED);
		}
		if (g_fpDispTrigCB) {
			g_fpDispTrigCB(0, TRUE);
		}

		if (gUserState & PB_DISPSRV_FLIP) {
			wai_flg(&Flag, FLG_PB, FLGPB_PULL_FLIPPED, TWF_ORW | TWF_CLR);
		}
	}
	// others view is push mode
	else {
		if (flipped) {
			return;
		} else {
			flipped = TRUE;
		}
		clr_flg(FLG_PB, FLGPB_PUSH_FLIPPED);
		if (g_fpDispTrigCB) {
			g_fpDispTrigCB(&gPBImgBuf[gPbPushSrcIdx], TRUE);
		}

		wai_flg(&Flag, FLG_PB, FLGPB_PUSH_FLIPPED, TWF_ORW | TWF_CLR);
	}
}

void PB_DispSrv_SetDrawAct(UINT32 DrawAct)
{
	// suspend state
	if (gUserState & PB_DISPSRV_SUSPEND) {
		return;
	}

	flipped = FALSE;
	gUserState = (gUserState & PB_DISPSRV_SUS_MASK) | (DrawAct & PB_DISPSRV_DRAW_MASK);
}

void PB_DispSrv_Suspend(void)
{
	// suspend state
	if (gUserState & PB_DISPSRV_SUSPEND) {
		return;
	}
	gUserState = ((gUserState & PB_DISPSRV_DRAW_MASK) | PB_DISPSRV_SUSPEND);
	set_flg(FLG_ID_PLAYBACK, FLGPB_IMG_STOP);
	if (gPbViewState == PB_VIEW_STATE_THUMB) {
		set_flg(FLG_ID_PB_DISP, FLGPB_IMG_0 | FLGPB_IMG_1);
	}
}

void PB_DispSrv_Resume(void)
{
	gUserState = (gUserState & PB_DISPSRV_DRAW_MASK);
	clr_flg(FLG_ID_PLAYBACK, FLGPB_IMG_STOP);
}

void xPB_DispSrv_Init4PB(void)
{
	#if _TODO
	ISIZE size = GxVideo_GetDeviceSize(DOUT1);
	USIZE aspect = GxVideo_GetDeviceAspect(DOUT1);
	#else
	ISIZE size = {0};
	USIZE aspect = {0};
	#endif

    size.w = 960;
	size.h = 240;
	aspect.w = 4;
	aspect.h = 3;
	g_PBVdoWIN[PBDISP_IDX_PRI].x = 0;
	g_PBVdoWIN[PBDISP_IDX_PRI].y = 0;
	g_PBVdoWIN[PBDISP_IDX_PRI].w = size.w;
	g_PBVdoWIN[PBDISP_IDX_PRI].h = size.h;

	g_PB1stVdoRect[PBDISP_IDX_PRI].x = 0;
	g_PB1stVdoRect[PBDISP_IDX_PRI].y = 0;
	g_PB1stVdoRect[PBDISP_IDX_PRI].w = size.w;
	g_PB1stVdoRect[PBDISP_IDX_PRI].h = size.h;

	if (gDualDisp) {

		#if _TODO
		ISIZE size2 = GxVideo_GetDeviceSize(DOUT2);
		USIZE aspect2 = GxVideo_GetDeviceAspect(DOUT2);
		#else
		ISIZE size2 = {0};
		USIZE aspect2 = {0};
		#endif

		g_PBVdoWIN[PBDISP_IDX_SEC].x = 0;
		g_PBVdoWIN[PBDISP_IDX_SEC].y = 0;
		g_PBVdoWIN[PBDISP_IDX_SEC].w = size2.w;
		g_PBVdoWIN[PBDISP_IDX_SEC].h = size2.h;

		g_PB1stVdoRect[PBDISP_IDX_SEC].x = 0;
		g_PB1stVdoRect[PBDISP_IDX_SEC].y = 0;
		g_PB1stVdoRect[PBDISP_IDX_SEC].w = size2.w;
		g_PB1stVdoRect[PBDISP_IDX_SEC].h = size2.h;

		g_PBDispInfo.uiDisplayW = size2.w;
		g_PBDispInfo.uiDisplayH = size2.h;
		g_PBDispInfo.uiARatioW  = aspect2.w;
		g_PBDispInfo.uiARatioH  = aspect2.h;

	} else {
		g_PBDispInfo.uiDisplayW = size.w;
		g_PBDispInfo.uiDisplayH = size.h;
		g_PBDispInfo.uiARatioW  = aspect.w;
		g_PBDispInfo.uiARatioH  = aspect.h;
	}
}

void PB_SetPBFlag(PB_SET_FLG flag)
{
	switch (flag) {
	case PB_SET_FLG_BROWSER_END:
		set_flg(FLG_PB, FLGPB_PULL_DRAWEND);
		break;

	case PB_SET_FLG_TRIGGER:
		set_flg(FLG_ID_PB_DRAW, FLGPB_ONDRAW);
		break;
	case PB_SET_FLG_DRAW_END:
		set_flg(FLG_PB, FLGPB_PUSH_FLIPPED);
		break;
	default:
		break;
	}
}

