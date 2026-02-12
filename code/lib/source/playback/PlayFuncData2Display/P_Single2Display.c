#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#if _TODO
#include "Color.h"
#endif

void PB_Scale2Display(UINT32 SrcWidth, UINT32 SrcHeight, UINT32 SrcStart_X, UINT32 SrcStart_Y, UINT32 FileFormat, UINT32 IfCopy2TmpBuf)
{
#if _TODO
	UINT32 DstWidth, DstHeight;
	UINT32 ImeTmpWidth, ImeTmpHeight;
	UINT32 usPBDisplayWidth, usPBDisplayHeight;

	usPBDisplayWidth  = g_PBDispInfo.uiDisplayW;
	usPBDisplayHeight = g_PBDispInfo.uiDisplayH;

	if ((usPBDisplayWidth > PLAY_IMETMP_WIDTH) || (usPBDisplayHeight > PLAY_IMETMP_HEIGHT)) {
		// Use double CURRENT display size instead of IMETmp size for improving display quality
		ImeTmpWidth  = usPBDisplayWidth * 2;
		ImeTmpHeight = usPBDisplayHeight * 2;
	} else {
		ImeTmpWidth  = PLAY_IMETMP_WIDTH;
		ImeTmpHeight = PLAY_IMETMP_HEIGHT;
	}

	////////////////////////////////////////////////////////////////////////////
	// Alignment for IME Scaling function
	////////////////////////////////////////////////////////////////////////////
	ImeTmpWidth  = ALIGN_CEIL_8(ImeTmpWidth);
	ImeTmpHeight = ALIGN_CEIL_8(ImeTmpHeight);
	DstWidth  = usPBDisplayWidth;
	DstHeight = usPBDisplayHeight;

	// default source is decode img buff
	gPbPushSrcIdx = PB_DISP_SRC_DEC;
	gSrcWidth  = SrcWidth;
	gSrcHeight = SrcHeight;
	gSrcRegion.x = SrcStart_X;
	gSrcRegion.y = SrcStart_Y;
	gSrcRegion.w = SrcWidth;
	gSrcRegion.h = SrcHeight;

	if (SrcWidth > SrcHeight) {
		//
		// 1. Better scale quality for large image
		// 2. Destination width or height is less than temp buffer width or height
		//    (If destination >= temp, don't scale to temp. Scale to destination directly)
		//
		if (((SrcWidth > ImeTmpWidth) && (SrcHeight > ImeTmpHeight)) &&
			((DstWidth < ImeTmpWidth) || (DstHeight < ImeTmpHeight)) &&
			gScaleTwice) {
			gximg_init_buf(g_pPlayIMETmpImgBuf, ImeTmpWidth, ImeTmpHeight, VDO_PXLFMT_YUV422, GXIMG_LINEOFFSET_ALIGN(8), g_uiPBIMETmpBuf, ImeTmpWidth * ImeTmpHeight * 2);
			gximg_fill_data(g_pPlayIMETmpImgBuf, NULL, COLOR_YUV_BLACK);
			gximg_scale_data_fine(g_pDecodedImgBuf, &gSrcRegion, g_pPlayIMETmpImgBuf, GXIMG_REGION_MATCH_IMG);

			gPbPushSrcIdx = PB_DISP_SRC_IMETMP;
			gSrcRegion.w = ImeTmpWidth;
			gSrcRegion.h = ImeTmpHeight;
			gSrcRegion.x = 0;
			gSrcRegion.y = 0;
		}
	} else {
		//
		// 1. Better scale quality for large image
		// 2. Destination width or height is less than temp buffer width or height
		//    (If destination >= temp, don't scale to temp. Scale to destination directly)
		//
		if (((SrcHeight > ImeTmpWidth) && (SrcWidth > ImeTmpHeight)) &&
			((DstWidth < ImeTmpWidth) && (DstHeight < ImeTmpHeight)) &&
			gScaleTwice) {
			gximg_init_buf(g_pPlayIMETmpImgBuf, ImeTmpHeight, ImeTmpWidth, VDO_PXLFMT_YUV422, GXIMG_LINEOFFSET_ALIGN(8), g_uiPBIMETmpBuf, ImeTmpWidth * ImeTmpHeight * 2);
			gximg_fill_data(g_pPlayIMETmpImgBuf, NULL, COLOR_YUV_BLACK);
			gximg_scale_data_fine(g_pDecodedImgBuf, &gSrcRegion, g_pPlayIMETmpImgBuf, GXIMG_REGION_MATCH_IMG);

			gPbPushSrcIdx = PB_DISP_SRC_IMETMP;
			gSrcRegion.w = ImeTmpHeight;
			gSrcRegion.h = ImeTmpWidth;
			gSrcRegion.x = 0;
			gSrcRegion.y = 0;
		}
	}
#endif

	PB_DispSrv_SetDrawCb(PB_VIEW_STATE_SINGLE);

	if (IfCopy2TmpBuf == PLAYSYS_COPY2FBBUF) {
		PB_DispSrv_SetDrawAct(PB_DISPSRV_DRAW | PB_DISPSRV_FLIP);
	} else {
		PB_DispSrv_SetDrawAct(PB_DISPSRV_DRAW);
	}

	//PB_DispSrv_Trigger();
}

