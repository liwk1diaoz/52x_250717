#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

void PB_HWRotate2Display(UINT32 CurrFBFormat, UINT32 RotateDir, BOOL isFlip)
{
#if _TODO	
	UINT32 GxRotateDir;

	GxRotateDir = PB_GetGxImgRotateDir(RotateDir);
	// do rotate
	gximg_rotate_data(g_pPlayTmpImgBuf, g_uiPBIMETmpBuf, guiPlayIMETmpBufSize, GxRotateDir, g_pPlayIMETmpImgBuf);

	gPbPushSrcIdx = PB_DISP_SRC_IMETMP;
	//gSrcWidth  = gMenuPlayInfo.pJPGInfo->imagewidth;
	//gSrcHeight = gMenuPlayInfo.pJPGInfo->imageheight;
	gSrcRegion.x = 0;
	gSrcRegion.y = 0;
//#NT#2015/03/03#Scottie -begin
//#NT#Modify for Auto-Rotated (180 deg) images
#if 0
	gSrcRegion.w = gSrcWidth;
	gSrcRegion.h = gSrcHeight;
#else
	gSrcRegion.w = g_pPlayIMETmpImgBuf->size.w;
	gSrcRegion.h = g_pPlayIMETmpImgBuf->size.h;
#endif
//#NT#2015/03/03#Scottie -end
#endif

	PB_DispSrv_SetDrawCb(PB_VIEW_STATE_ROTATE);
	if (isFlip) {
		PB_DispSrv_SetDrawAct(PB_DISPSRV_DRAW | PB_DISPSRV_FLIP);
	} else {
		PB_DispSrv_SetDrawAct(PB_DISPSRV_DRAW);
	}
	PB_DispSrv_Trigger();
}

