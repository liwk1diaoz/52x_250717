#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFileList.h"

/*
    Rotate image and then display it
    This is internal API.

    @return void
*/
void PB_RotateDisplayHandle(BOOL isFlip)
{
	UINT64         fileSize;
	PPBX_FLIST_OBJ pFlist = g_PBSetting.pFileListObj;

	// check if 1st ROTATE_DISPLAY command
	if (gMenuPlayInfo.ZoomIndex != 0) {
		DBG_ERR("Invalid mode (Zoom)!\r\n");
		return;
	}

	PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC, (INT8 *)guiPlayFileBuf, 0x200, 0, 1);

	if ((FALSE == gMenuPlayInfo.DecodePrimaryOK) && (gMenuPlayInfo.RotateDir == PLAY_ROTATE_DIR_UNKNOWN) &&
		!(gPBCmdObjPlaySingle.PlayCommand & PB_SINGLE_THUMB)) {
		UINT32 Orientation = JPEG_EXIF_ORI_DEFAULT;
		MAKERNOTE_INFO MakernoteInfo = {0};

		EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_MAKERNOTE, &MakernoteInfo, sizeof(MakernoteInfo));

		if (g_PBSetting.DispDirection == PB_DISPDIR_VERTICAL) {
			// Backup and restore jdparams.pExifInfo->Orientation for g_PBSetting.DispDirection == PB_VERTICAL
			if (g_bExifExist) {
				Orientation = g_uiExifOrientation;
			} else {
				Orientation = JPEG_EXIF_ORI_DEFAULT;
			}
		}

		if (MakernoteInfo.uiScreennailOffset != 0) {
			PB_DecodeWhichFile(PB_DEC_HIDDEN, gMenuPlayInfo.CurFileFormat);
		} else {
			INT32 iErrChk;

			pFlist->GetInfo(PBX_FLIST_GETINFO_FILESIZE64, &fileSize, NULL);
			iErrChk = PB_DecodePrimaryHandler((UINT32)fileSize, gMenuPlayInfo.CurFileFormat);
			if (iErrChk > 0) {
				gMenuPlayInfo.DecodePrimaryOK = TRUE;
			} else {
				gMenuPlayInfo.DecodePrimaryOK = FALSE;
			}
		}

		if (g_PBSetting.DispDirection == PB_DISPDIR_VERTICAL) {
			// Backup and restore jdparams.pExifInfo->Orientation for g_PBSetting.DispDirection == PB_VERTICAL
			if (g_bExifExist) {
				g_uiExifOrientation = Orientation;
			}
		}
	}

	if (g_uiPBRotateDir == PLAY_ROTATE_DIR_0) {
		gMenuPlayInfo.RotateDir = g_uiPBRotateDir;
		//#NT#2012/05/17#Lincy Lin -begin
		//#NT#Porting disply Srv
#if PB_USE_DISP_SRV
		//PB_Scale2Display(gMenuPlayInfo.pJPGInfo->imagewidth, gMenuPlayInfo.pJPGInfo->imageheight, 0, 0,
		//				 gMenuPlayInfo.pJPGInfo->fileformat, PLAYSYS_COPY2FBBUF);
		PB_DispSrv_Trigger();
#else
		//PB_Scale2Display(gMenuPlayInfo.pJPGInfo->imagewidth, gMenuPlayInfo.pJPGInfo->imageheight, 0, 0,
		//				 gMenuPlayInfo.pJPGInfo->fileformat, PLAYSYS_COPY2TMPBUF);
		PB_CopyImage2Display(PLAYSYS_COPY2TMPBUF, PLAYSYS_COPY2FBBUF);
#endif
		//#NT#2012/05/17#Lincy Lin -end
		gucRotatePrimary = FALSE;
		return;
	}

	PB_ReScaleForRotate2Display();

	gMenuPlayInfo.RotateDir = g_uiPBRotateDir;
	gucRotatePrimary = FALSE;
	PB_HWRotate2Display(PB_JPG_FMT_YUV420, g_uiPBRotateDir, isFlip);
}

