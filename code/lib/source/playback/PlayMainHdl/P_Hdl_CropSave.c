#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#if _TODO
#include "PBXEdit.h"
#include "PBXFileList.h"
#endif

INT32 PB_CropSaveHandle(UINT32 IfOverWrite)
{
	#if _TODO
	UINT64 tmpFileSize;
	UINT32 uiScaleRatio;
	UINT32 SrcStartX = 0, SrcStartY = 0;
	UINT32 SrcWidth = 0, SrcHeight = 0;
	PBX_EDIT_PARAM  EditParam = {0};
	UINT32 Param[1] = {0};
	IRECT  CropRegion = {0};
	UINT32 DecStartAddr, DecEndAddr = 0;
	//#NT#2010/11/30#Ben Wang -begin
	//#NT#transform coordinate for non default orientation
	UINT32 uiOrientation = g_uiExifOrientation;
	//#NT#2010/11/30#Ben Wang -end
	PPBX_FLIST_OBJ pFlist = g_PBSetting.pFileListObj;

	pFlist->GetInfo(PBX_FLIST_GETINFO_FILESIZE64, &tmpFileSize, NULL);
	if (((gPBCmdObjCropSave.DisplayStart_X + gPBCmdObjCropSave.DisplayWidth) > g_PBDispInfo.uiDisplayW) ||
		((gPBCmdObjCropSave.DisplayStart_Y + gPBCmdObjCropSave.DisplayHeight) > g_PBDispInfo.uiDisplayH)) {
		return E_PAR;
	} else {
		SrcStartX = gMenuPlayInfo.PanSrcStartX + gMenuPlayInfo.PanSrcWidth * gPBCmdObjCropSave.DisplayStart_X / g_PBDispInfo.uiDisplayW;
		SrcStartY = gMenuPlayInfo.PanSrcStartY + gMenuPlayInfo.PanSrcHeight * gPBCmdObjCropSave.DisplayStart_Y / g_PBDispInfo.uiDisplayH;

		if ((gPBCmdObjCropSave.DisplayWidth == 0) && (gMenuPlayInfo.PanSrcWidth == 0)) {
			SrcWidth = gMenuPlayInfo.pJPGInfo->imagewidth;
		} else if (gPBCmdObjCropSave.DisplayWidth == 0) {
			SrcWidth = gMenuPlayInfo.PanSrcWidth;
		} else {
			SrcWidth = gMenuPlayInfo.PanSrcWidth * gPBCmdObjCropSave.DisplayWidth / g_PBDispInfo.uiDisplayW;
		}

		if ((gPBCmdObjCropSave.DisplayHeight == 0) && (gMenuPlayInfo.PanSrcHeight == 0)) {
			SrcHeight = gMenuPlayInfo.pJPGInfo->imageheight;
		} else if (gPBCmdObjCropSave.DisplayHeight == 0) {
			SrcHeight = gMenuPlayInfo.PanSrcHeight;
		} else {
			SrcHeight = gMenuPlayInfo.PanSrcHeight * gPBCmdObjCropSave.DisplayHeight / g_PBDispInfo.uiDisplayH;
		}
	}
	//#NT#2010/11/30#Ben Wang -begin
	//#NT#transform coordinate for non default orientation
	if (uiOrientation == JPEG_EXIF_ORI_ROTATE_180) {
		if (gMenuPlayInfo.pJPGInfo->imagewidth > (SrcStartX + SrcWidth)) {
			SrcStartX = gMenuPlayInfo.pJPGInfo->imagewidth - SrcStartX - SrcWidth;
		} else {
			SrcStartX = 0;
		}

		if (gMenuPlayInfo.pJPGInfo->imageheight > (SrcStartY + SrcHeight)) {
			SrcStartY = gMenuPlayInfo.pJPGInfo->imageheight - SrcStartY - SrcHeight;
		} else {
			SrcStartY = 0;
		}
	}
	//#NT#2010/11/30#Ben Wang -end

	// Restore the related image info.
	uiScaleRatio = gMenuPlayInfo.pJPGInfo->ori_imagewidth / gMenuPlayInfo.pJPGInfo->imagewidth;
	SrcStartX *= uiScaleRatio;
	SrcStartY *= uiScaleRatio;
	SrcWidth  *= uiScaleRatio;
	SrcHeight *= uiScaleRatio;


	////////////////////////////////////////////////////////////////////////////
	// Alignment for JPEG Decode Crop function (with YUV422 format)
	////////////////////////////////////////////////////////////////////////////
	if ((SrcWidth % 16) != 0) {
		SrcWidth = ALIGN_CEIL_16(SrcWidth);
	}
	if ((SrcHeight % 8) != 0) {
		SrcHeight = ALIGN_CEIL_8(SrcHeight);
	}
	if ((SrcStartX % 16) != 0) {
		SrcStartX = ALIGN_CEIL_16(SrcStartX);
	}
	if ((SrcStartY % 8) != 0) {
		SrcStartY = ALIGN_CEIL_8(SrcStartY);
	}
	GxImg_GetBufAddr(g_pDecodedImgBuf, &DecStartAddr, &DecEndAddr);
	EditParam.pSrcImg = g_pDecodedImgBuf;
	EditParam.pOutImg = g_pPlayIMETmpImgBuf;
	EditParam.workBuff = DecEndAddr;
	EditParam.workBufSize = guiPlayFileBuf - DecEndAddr;
	EditParam.paramNum = 1;
	CropRegion.x = SrcStartX;
	CropRegion.y = SrcStartY;
	CropRegion.w = SrcWidth;
	CropRegion.h = SrcHeight;
	Param[0] = (UINT32)&CropRegion;
	EditParam.param = Param;
	PBXEdit_SetFunc(EDIT_Crop);
	if (PBXEdit_Apply(&EditParam) != E_OK) {
		DBG_ERR("Buffer is NOT enough for Crop!!! \r\n");
		return E_SYS;
	}
	memcpy(g_pDecodedImgBuf, g_pPlayIMETmpImgBuf, sizeof(IMG_BUF));

	return PB_SaveCurrRawBuf(0, 0, SrcWidth, SrcHeight, IfOverWrite);
	#else
	return 0;
	#endif
}

