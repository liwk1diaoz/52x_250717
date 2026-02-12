#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFileList.h"

void PB_Hdl_EditImage(UINT32 PlayCMD)
{
	INT32  ErrCheck;
	UINT64 tmpFileSize;
	PPBX_FLIST_OBJ pFlist = g_PBSetting.pFileListObj;

	// must decode primary; avoid file being closed, can not be read-continue
	PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC, (INT8 *)guiPlayFileBuf, 0x200, 0, 1);
	// do not display primary again
	pFlist->GetInfo(PBX_FLIST_GETINFO_FILESIZE64, &tmpFileSize, NULL);
	ErrCheck = PB_DecodePrimaryHandler((UINT32)tmpFileSize, gMenuPlayInfo.CurFileFormat);
	if (ErrCheck > 0) {
		gMenuPlayInfo.DecodePrimaryOK = TRUE;
	} else {
		DBG_ERR("Decode Primary image NG!\r\n");
		gMenuPlayInfo.DecodePrimaryOK = FALSE;
	}

	switch (PlayCMD) {
	case PB_CMMD_RE_SIZE:
		ErrCheck = PB_ReResoultHandle(g_bPBSaveOverWrite);
		break;

	case PB_CMMD_RE_QTY:
		ErrCheck = PB_ReQualityHandle(g_bPBSaveOverWrite);
		break;

	case PB_CMMD_UPDATE_EXIF:
		ErrCheck = PB_UpdateEXIFHandle(g_bPBSaveOverWrite);
		if (gPB_EXIFOri.bDecodeIMG == FALSE) {
			if (ErrCheck == E_OK) {
				PB_SetFinishFlag(DECODE_JPG_DONE);
			} else {
				PB_SetFinishFlag(DECODE_JPG_WRITEERROR);
			}

			return;
		}
		break;

	case PB_CMMD_CROP_SAVE:
		ErrCheck = PB_CropSaveHandle(g_bPBSaveOverWrite);
		break;

	case PB_CMMD_CUSTOMIZE_EDIT:
		ErrCheck = PB_UserEditHandle(g_bPBSaveOverWrite);
		break;
	}

	if (ErrCheck == E_OK) {

		if (g_bPBSaveOverWrite == FALSE) {
			// decode max-id file
			//#NT#2013/02/20#Lincy Lin -begin
			//#NT#
			//gMenuPlayInfo.DispFileName[0] = PBXFile_GetInfo(PBX_FILEINFO_MAXFILEID_INDIR, 0);
			//gMenuPlayInfo.DispDirName[0]  = PBXFile_GetInfo(PBX_FILEINFO_DIRID, 0);
			//#NT#2013/02/20#Lincy Lin -end
		}

		if (PlayCMD == PB_CMMD_CROP_SAVE) {
			PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC, (INT8 *)guiPlayFileBuf, 0x200, 0, 1);
			PB_SetFinishFlag(DECODE_JPG_DONE);
		} else {
			gPBCmdObjPlaySingle.PlayCommand = (PB_SINGLE_CURR | PB_SINGLE_PRIMARY);
			guiCurrPlayCmmd = PB_CMMD_SINGLE;
			set_flg(FLG_PB, FLGPB_COMMAND);
		}
	} else {
		PB_SetFinishFlag(DECODE_JPG_WRITEERROR);
	}
}

