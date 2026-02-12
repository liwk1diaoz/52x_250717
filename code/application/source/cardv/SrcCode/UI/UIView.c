#include "PrjInc.h"
#include "GxDisplay.h"
#include "GxVideo.h"
#include "sys_mempool.h"
#if (OSD_USE_ROTATE_BUFFER == ENABLE)
#include "UIView.h"
#endif

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UIView
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

#if (OSD_USE_ROTATE_BUFFER == ENABLE)
static UINT32 gbOsdRotate[2] = {0};  // 0:DOUT1,1:DOUT2
static DC LocalDC;
static DC *pPaintDC = &LocalDC;
static DC *pShowDC = 0;

DC *View_Window_BeginRotate(DC *pDC)
{
	if (gbOsdRotate[0] == OSD_ROTATE_90_DEGREE) {
		DBG_MSG("case b-1\r\n");
		GxDisplay_WaitVD();
		pShowDC = pDC;
		GxGfx_CopyEx(pPaintDC, pShowDC, ROP_ROTATE | SRC_RCCW_90, 0); //copy show to paint with rotate CCW 90
		DBG_MSG("^YBegin Rotate CCW 90\r\n");
		return pPaintDC;
	} else if (gbOsdRotate[0] == OSD_ROTATE_270_DEGREE) {
		DBG_MSG("case b-2\r\n");
		GxDisplay_WaitVD();
		pShowDC = pDC;
		GxGfx_CopyEx(pPaintDC, pShowDC, ROP_ROTATE | SRC_RCCW_270, 0); //copy show to paint with rotate CCW 90
		DBG_MSG("^YBegin Rotate CCW 270\r\n");
		return pPaintDC;
	}
    return pDC;
}

void View_Window_EndRotate(void)
{
	if (gbOsdRotate[0] == OSD_ROTATE_90_DEGREE) {
		DBG_MSG("case e-1\r\n");
		GxDisplay_WaitVD();
		GxGfx_CopyEx(pShowDC, pPaintDC, ROP_ROTATE | SRC_RCW_90, 0); //copy paint to show with rotate CW 90
		pShowDC = 0;
		DBG_MSG("^YEnd Rotate CW 90\r\n");
	} else if (gbOsdRotate[0] == OSD_ROTATE_270_DEGREE) {
		DBG_MSG("case e-2\r\n");
		GxDisplay_WaitVD();
		GxGfx_CopyEx(pShowDC, pPaintDC, ROP_ROTATE | SRC_RCW_270, 0); //copy paint to show with rotate CW 90
		pShowDC = 0;
		DBG_MSG("^YEnd Rotate CW 270\r\n");
	}
}

#endif

void UIDisplay_Init(UINT8 iDD, BOOL bClear, ISIZE *pDeviceSize)
{
	RESULT r;
	LAYER_INIT LayerInit={0};

	UINT32 uiBufAddr;
	//fixed buffer size
	UINT32 osd_w = DISPLAY_OSD_W;
	UINT32 osd_h = DISPLAY_OSD_H;
	LAYER_INIT *pLayerInit = (LAYER_INIT *)&LayerInit; //layer init parameter
	ISIZE DeviceSize = pDeviceSize[0];
#if (OSD_USE_ROTATE_BUFFER == ENABLE)
	UINT32 did = iDD >> 4;
#endif
	DBG_MSG("^YDOUT=%d\r\n", iDD >> 4);
#if (OSD_USE_ROTATE_BUFFER == ENABLE)
	DBG_MSG("^Yrotate[%d]=%d\r\n", did, gbOsdRotate[did]);
#endif
	switch (iDD & 0x0f) { //check layer
	case LAYER_OSD1:

		uiBufAddr = mempool_disp_osd1;//OS_GetMempoolAddr(POOL_ID_DISP_OSD1);

		pLayerInit->uiType = TYPE_FB;
		pLayerInit->uiPxlfmt = DISPLAY_OSD_FMT;
		pLayerInit->uiWidth = osd_w;
		pLayerInit->uiHeight = osd_h;

		switch (pLayerInit->uiPxlfmt) {
		case DISP_PXLFMT_INDEX8:
			pLayerInit->uiBufSize = (osd_w * osd_h) >> 0;
			break;
		case DISP_PXLFMT_ARGB8565_PK:
			pLayerInit->uiBufSize = (osd_w * osd_h) * 3;
			break;
		case DISP_PXLFMT_ARGB8888_PK:
			pLayerInit->uiBufSize = (osd_w * osd_h) * 4;
			break;
		case DISP_PXLFMT_ARGB4444_PK:
			pLayerInit->uiBufSize = (osd_w * osd_h) * 2;
			break;
		default:
			DBG_ERR("unknown pLayerInit->uiPxlfmt=%d\r\n", pLayerInit->uiPxlfmt);
            return ;
		}
		//dynamic window size
		pLayerInit->win.x = 0;
		pLayerInit->win.y = 0;
		pLayerInit->win.w = DeviceSize.w;
		pLayerInit->win.h = DeviceSize.h;
		pLayerInit->uiWinAttr = 0;
		//for dummy LCD, window is very small, avoid scale-down buffer too much
		if ((DeviceSize.w == 80) && (DeviceSize.h == 60)) { //if dummy LCD?
			if (pLayerInit->win.w < (INT32)(osd_w / 2)) {
				DBG_WRN("devSize.w=%d, osd w=%d, force osd.win w=%d\r\n", DeviceSize.w, osd_w, osd_w / 2);
				pLayerInit->win.w = (INT32)(osd_w / 2);
			}
			if (pLayerInit->win.h < (INT32)(osd_h / 2)) {
				DBG_WRN("devSize.h=%d, osd h=%d, force osd.win h=%d\r\n", DeviceSize.h, osd_h, osd_h / 2);
				pLayerInit->win.h = (INT32)(osd_h / 2);
			}
		}

#if (OSD_USE_ROTATE_BUFFER == ENABLE)
		{ //single display
			if (gbOsdRotate[did] == OSD_ROTATE_0_DEGREE) { //one normal display
				DBG_IND("case 1-1\r\n");
				pLayerInit->uiBufCount = 1;
				pLayerInit->uiSwapEffect = SWAPEFFECT_COPY; //use double buffer(for show DC and paint DC)
				pLayerInit->pBufAddr[0] = uiBufAddr; //1st buffer
				pLayerInit->pBufAddr[1] = uiBufAddr + pLayerInit->uiBufSize; //2nd buffer
				pLayerInit->pBufAddr[2] = 0;
			} else { //one rotate display
				DBG_IND("case 1-2\r\n");  //use single buffer,avoid rotate twice
				pLayerInit->uiBufCount = 0;
				pLayerInit->uiSwapEffect = SWAPEFFECT_DISCARD; //use single buffer(for show DC only)
				pLayerInit->pBufAddr[0] = uiBufAddr;    //1st buffer
				pLayerInit->pBufAddr[1] = 0;
				pLayerInit->pBufAddr[2] = 0;
				pLayerInit->uiSwapEffect |= SWAPEFFECT_XY;
			}


        if (gbOsdRotate[did] != OSD_ROTATE_0_DEGREE) {
			//prepare additional paint DC
			UINT8 *pBuffer = 0;

			//single display
			pBuffer = (UINT8 *)(uiBufAddr + pLayerInit->uiBufSize*2);    //2nd buffer

			memset(pPaintDC, 0, sizeof(DC));
			GxGfx_AttachDC(pPaintDC, TYPE_FB,
						   pLayerInit->uiPxlfmt, osd_w, osd_h, 0, pBuffer, 0, 0);
			DBG_MSG("paint buf=%08x, size=%08x\n\r", pBuffer, LayerInit.uiBufSize);
			DBG_MSG("paint buf.w=%d, buf.h=%d;\n\r", osd_w, osd_h);
		}


		}
#else
		pLayerInit->uiBufCount = 1;
		pLayerInit->uiSwapEffect = SWAPEFFECT_COPY; //use double buffer(for show DC and paint DC)
		pLayerInit->pBufAddr[0] = uiBufAddr; //1st buffer
		pLayerInit->pBufAddr[1] = uiBufAddr + pLayerInit->uiBufSize; //2nd buffer
		pLayerInit->pBufAddr[2] = 0;
#endif

		r = GxDisplay_InitLayer(iDD, &LayerInit, bClear);
		if (r != GX_OK) {
			DBG_ERR("GxDisplay_InitLayer %d\r\n", r);
            return ;
		}
		DBG_IND("OSD1 buf=%08x, size=%08x\n\r", LayerInit.pBufAddr[0], LayerInit.uiBufSize);
		DBG_IND("OSD1 buf.w=%d, buf.h=%d;\n\r", LayerInit.uiWidth, LayerInit.uiHeight);
		DBG_IND("OSD1 win.w=%d, win.h=%d;\n\r", LayerInit.win.w, LayerInit.win.h);

		break;
	}
}

#if (OSD_USE_ROTATE_BUFFER == ENABLE)
void View_Window_ConfigAttr(UINT32 did, UINT32 attr, UINT32 value)
{
	if (attr == OSD_ATTR_ROTATE) {
		DBG_MSG("...............Set rot[%d]=%x\r\n", did, value);
		gbOsdRotate[did] = value;
	}
}
#endif


