#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "BinaryFormat.h"
#if _TODO
#include "GxImageFile.h"
#endif

void PB_Hdl_CaptureScreen()
{
#if _TODO
	UINT32 uiExpectFileSize = *g_stPBCSNInfo.puiFileSize;
	UINT32 uiEncodeMainAddr;
	VDO_FRAME TmpImg = {0};
	PVDO_FRAME pDstImg;
	UINT32 uiTmpBuf = guiPlayFileBuf - (2 * g_stPBCSNInfo.uiDstWidth * g_stPBCSNInfo.uiDstHeight);
	UINT32 uiDecBufEnd = g_pDecodedImgBuf->addr[0];
	MEM_RANGE PrimaryJPG = {0};

	if (VDO_PXLFMT_YUV420 == g_pDecodedImgBuf->pxlfmt) {
		uiDecBufEnd += (g_pDecodedImgBuf->loff[0] * g_pDecodedImgBuf->size.h * 3 / 2);
	} else {
		uiDecBufEnd += (g_pDecodedImgBuf->loff[0] * g_pDecodedImgBuf->size.h * 2);
	}

	if (uiTmpBuf < uiDecBufEnd) {
		DBG_ERR("CaptureSN NG1!\r\n");
		PB_SetFinishFlag(DECODE_JPG_DECODEERROR);

		return;
	} else {
		gximg_init_buf(&TmpImg, g_stPBCSNInfo.uiDstWidth, g_stPBCSNInfo.uiDstHeight, VDO_PXLFMT_YUV422,
					  g_stPBCSNInfo.uiDstWidth, uiTmpBuf, g_stPBCSNInfo.uiDstWidth * g_stPBCSNInfo.uiDstHeight * 2);
	}

	if (PLAYMODE_THUMB == gMenuPlayInfo.CurrentMode) {
		IRECT    TmpRegion = {0};

		// Decode CURRENT image into TmpBuf
		gPBCmdObjPlaySpecFile.bDisplayWoReDec  = FALSE;
		gPBCmdObjPlaySpecFile.PlayFileVideo    = PLAY_SPEC_FILE_IN_VIDEO_1;
		gPBCmdObjPlaySpecFile.PlayFileClearBuf = PLAY_SPEC_FILE_WITH_CLEAR_BUF;
		gPBCmdObjPlaySpecFile.PlayRect.x = 0;
		gPBCmdObjPlaySpecFile.PlayRect.y = 0;
		gPBCmdObjPlaySpecFile.PlayRect.w = g_stPBCSNInfo.uiDstWidth;
		gPBCmdObjPlaySpecFile.PlayRect.h = g_stPBCSNInfo.uiDstHeight;
		PB_Hdl_PlaySpecFileHandle();

		TmpRegion.x = 0;
		TmpRegion.y = 0;
		TmpRegion.w = g_stPBCSNInfo.uiDstWidth;
		TmpRegion.h = g_stPBCSNInfo.uiDstHeight;
		gximg_scale_data(g_pPlayTmpImgBuf, &TmpRegion, &TmpImg, GXIMG_REGION_MATCH_IMG);
		pDstImg = &TmpImg;
	} else {
		if ((g_stPBCSNInfo.uiDstWidth != g_PBDispInfo.uiDisplayW) || (g_stPBCSNInfo.uiDstHeight != g_PBDispInfo.uiDisplayH)) {
			// The resolution of LOGO image is user defined
			gximg_scale_data(&gPlayFBImgBuf, GXIMG_REGION_MATCH_IMG, &TmpImg, GXIMG_REGION_MATCH_IMG);
			pDstImg = &TmpImg;
		} else {
			// The resolution of LOGO image is panel size
			pDstImg = &gPlayFBImgBuf;
		}
	}

	//[Hint] Use IMETmpBuf for encoding screen image, 'cos the encoded file size shouldn't exceed 2x screen size
	uiEncodeMainAddr = g_uiPBIMETmpBuf;
#if _TODO
	if (uiExpectFileSize) {
		UINT32 uiCompressRatio;
		uiCompressRatio = GxImgFile_CalcCompressedRatio(pDstImg, uiExpectFileSize);
		PB_SetExpectJPEGSize(uiCompressRatio, PB_BRC_DEFAULT_UPBOUND_RATIO, PB_BRC_DEFAULT_LOWBOUND_RATIO, PB_BRC_DEFAULT_LIMIT_CNT);
	}
#endif
//#NT#2016/05/30#Scottie -begin
//#NT#Remove for Coverity: 61079 (Unused value)
#if 0
	else {
		uiExpectFileSize = 2 * g_uiMAXPanelSize;
	}
#endif
//#NT#2016/05/30#Scottie -end

	PrimaryJPG.addr = uiEncodeMainAddr;
	PrimaryJPG.size = guiPlayIMETmpBufSize;
	if (E_OK != PB_JpgBRCHandler(pDstImg, &PrimaryJPG)) {
		DBG_ERR("CaptureSN NG3!\r\n");

		PB_SetFinishFlag(DECODE_JPG_WRITEERROR);
	} else {
		// OK, update the captured screen image info
		*g_stPBCSNInfo.puiFileAddr = PrimaryJPG.addr;
		*g_stPBCSNInfo.puiFileSize = PrimaryJPG.size;

		PB_SetFinishFlag(DECODE_JPG_DONE);
	}
#endif	
}

