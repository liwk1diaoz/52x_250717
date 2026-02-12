#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/*
    Digital zoom pan mode.
    This is internal API.

    @return void
*/
void PB_DigitalZoomMoving(void)
{
#if _TODO
	UINT8  ucZoomIndex;
	UINT32 uiOriSrcWidth, uiOriSrcHeight;
	UINT16 usSrcWidth, usSrcHeight, usSrcStart_X, usSrcStart_Y;
	UINT16 usDstWidth, usDstHeight;

	ucZoomIndex = gMenuPlayInfo.ZoomIndex;
	if ((gMenuPlayInfo.RotateDir == PLAY_ROTATE_DIR_90) || (gMenuPlayInfo.RotateDir == PLAY_ROTATE_DIR_270)) {
		uiOriSrcWidth  = gMenuPlayInfo.pJPGInfo->imageheight;
		uiOriSrcHeight = gMenuPlayInfo.pJPGInfo->imagewidth;
	} else {
		uiOriSrcWidth  = gMenuPlayInfo.pJPGInfo->imagewidth;
		uiOriSrcHeight = gMenuPlayInfo.pJPGInfo->imageheight;
	}

	usSrcWidth   = gMenuPlayInfo.PanSrcWidth;
	usSrcHeight  = gMenuPlayInfo.PanSrcHeight;
	usSrcStart_X = gMenuPlayInfo.PanDstStartX;
	usSrcStart_Y = gMenuPlayInfo.PanDstStartY;

	usDstWidth  = gMenuPlayInfo.PanDstWidth;
	usDstHeight = gMenuPlayInfo.PanDstHeight;

	if (uiOriSrcHeight < uiOriSrcWidth) {
		// normal
		PB_Scale2Display(usSrcWidth, usSrcHeight, usSrcStart_X, usSrcStart_Y,
						 gMenuPlayInfo.pJPGInfo->fileformat, PLAYSYS_COPY2TMPBUF);
	}
//#NT#2012/10/30#Scottie -begin
//#NT#Modify, support 1:1 image ratio
//    else if(uiOriSrcHeight > uiOriSrcWidth)
	else// if(uiOriSrcHeight >= uiOriSrcWidth)
//#NT#2012/10/30#Scottie -end
	{
		if (ucZoomIndex == 0) {
			// 1X
			usSrcStart_X = 0;
			usSrcStart_Y = 0;
			PB_Scale2Display(usSrcWidth, usSrcHeight, usSrcStart_X, usSrcStart_Y,
							 gMenuPlayInfo.pJPGInfo->fileformat, PLAYSYS_COPY2TMPBUF);
		} else {
			// another scale
			//#NT#2012/05/17#Lincy Lin -begin
			//#NT#Porting disply Srv
			gSrcRegion.w = usSrcWidth;
			gSrcRegion.h = usSrcHeight;
			gSrcRegion.x = usSrcStart_X;
			gSrcRegion.y = usSrcStart_Y;
			gDstRegion.w = usDstWidth;
			gDstRegion.h = usDstHeight;
			gPbPushSrcIdx = PB_DISP_SRC_DEC;
			PB_DispSrv_SetDrawCb(PB_VIEW_STATE_SPEZOOM);
			PB_DispSrv_SetDrawAct(PB_DISPSRV_DRAW);
			PB_DispSrv_Trigger();
			//#NT#2012/05/17#Lincy Lin -end
		}
	}

	//#NT#2011/01/31#Ben Wang -begin
	//#NT#Add screen control to sync display with OSD
	if (PB_FLUSH_SCREEN == gScreenControlType) {
		//#NT#2012/05/17#Lincy Lin -begin
		//#NT#Porting disply Srv
		PB_DispSrv_SetDrawAct(PB_DISPSRV_FLIP);
		PB_DispSrv_Trigger();
	} else {
		gScreenControlType = PB_FLUSH_SCREEN;
	}
	//#NT#2011/01/31#Ben Wang -end

	// restore PanSrcStartX & PanSrcStartY
	gMenuPlayInfo.PanSrcStartX = usSrcStart_X;
	gMenuPlayInfo.PanSrcStartY = usSrcStart_Y;
#endif //_TODO	
}

