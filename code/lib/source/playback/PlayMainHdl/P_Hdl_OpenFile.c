#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFileList.h"

void PB_Hdl_OpenFile(UINT32 PlayCMD)
{
	INT32  ReadSts = E_SYS;
	BOOL   isReadOnly;
	PPBX_FLIST_OBJ  pFlist = g_PBSetting.pFileListObj;

	switch (PlayCMD) {
	case PB_CMMD_OPEN_NEXT:
		ReadSts = PB_ReadFileByFileSys(PB_FILE_READ_NEXT, (INT8 *)guiPlayFileBuf, 0x200, 0, 1);
		break;

	case PB_CMMD_OPEN_PREV:
		ReadSts = PB_ReadFileByFileSys(PB_FILE_READ_PREV, (INT8 *)guiPlayFileBuf, 0x200, 0, 1);
		break;

	case PB_CMMD_OPEN_SPECFILE:
		gMenuPlayInfo.DispFileName[gMenuPlayInfo.CurFileIndex - 1] = gPBCmdObjPlaySingle.FileId;
		gMenuPlayInfo.DispDirName[gMenuPlayInfo.CurFileIndex - 1]  = gPBCmdObjPlaySingle.DirId;
		ReadSts = PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC, (INT8 *)guiPlayFileBuf, 0x200, 0, 1);
		pFlist->GetInfo(PBX_FLIST_GETINFO_READONLY, (UINT32 *)&isReadOnly, NULL);
		if (isReadOnly) {
			gMenuPlayInfo.AllThumbFormat[gMenuPlayInfo.CurFileIndex - 1] |= PBFMT_READONLY;
		} else {
			gMenuPlayInfo.AllThumbFormat[gMenuPlayInfo.CurFileIndex - 1] &= ~PBFMT_READONLY;
		}
		break;
	}

	//if(PB_GetFileSysStatus() >= 0)
	if (ReadSts >= 0) {
		PB_SetFinishFlag(DECODE_JPG_DONE);
	} else {
		PB_SetFinishFlag(DECODE_JPG_READERROR);
	}
}

#if 0
#pragma mark -
#endif

INT32 PB_SearchValidFile(UINT32 CommandID, INT32 *ReadFileSts)
{
	UINT32 i, j, uiAllFileNums;
	PPBX_FLIST_OBJ  pFlist = g_PBSetting.pFileListObj;

	pFlist->GetInfo(PBX_FLIST_GETINFO_FILENUMS, &uiAllFileNums, NULL);
	PB_GetDefaultFileBufAddr();

	j = 1;

	*ReadFileSts = E_SYS;
	if (gPBCmdObjPlaySingle.JumpOffset == 0) {
		gPBCmdObjPlaySingle.JumpOffset = 1;
	}
	//#NT#2009/09/07#Ben Wang#[0010573]-begin
	//reset CurFileFormat
	gMenuPlayInfo.CurFileFormat = PBFMT_UNKNOWN;
	//#NT#2009/09/07#Ben Wang#[0010573] -end
	for (i = 0; i < uiAllFileNums; i++) {
		// Read next file
		if (j == gPBCmdObjPlaySingle.JumpOffset) {
			*ReadFileSts = PB_ReadFileByFileSys(CommandID, (INT8 *)guiPlayFileBuf, FST_READ_THUMB_BUF_SIZE, 0, 1);
		} else {
			*ReadFileSts = PB_ReadFileByFileSys(CommandID, (INT8 *)guiPlayFileBuf, 0, 0, 1);
		}

		gMenuPlayInfo.CurFileFormat = PB_ParseFileFormat();
		if (gMenuPlayInfo.CurFileFormat & gusPlayFileFormat) {
			if (j == gPBCmdObjPlaySingle.JumpOffset) {
				break;
			}
			j++;
		}
		if (CommandID == PB_FILE_READ_SPECIFIC) {
			CommandID = PB_FILE_READ_NEXT;
		}
	}

	if (!(gMenuPlayInfo.CurFileFormat & gusPlayFileFormat)) {
		return E_SYS;
	} else {
		return E_OK;
	}
}

INT32 PB_SearchMaxFileId(void)
{
	UINT32          i, uiAllFileNums;
	PPBX_FLIST_OBJ  pFlist = g_PBSetting.pFileListObj;

	// playback task decode last-file in card
	pFlist->GetInfo(PBX_FLIST_GETINFO_FILENUMS, &uiAllFileNums, NULL);
	PB_GetDefaultFileBufAddr();

	PB_ReadFileByFileSys(PB_FILE_READ_LAST, (INT8 *)guiPlayFileBuf, FST_READ_THUMB_BUF_SIZE, 0, 1);
	gMenuPlayInfo.CurFileFormat = PB_ParseFileFormat();
	if (!(gMenuPlayInfo.CurFileFormat & gusPlayFileFormat)) {
		for (i = 0; i < uiAllFileNums; i++) {
			PB_ReadFileByFileSys(PB_FILE_READ_PREV, (INT8 *)guiPlayFileBuf, FST_READ_THUMB_BUF_SIZE, 0, 1);
			gMenuPlayInfo.CurFileFormat = PB_ParseFileFormat();
			if (gMenuPlayInfo.CurFileFormat & gusPlayFileFormat) {
				break;
			}
		}
	}

	if (!(gMenuPlayInfo.CurFileFormat & gusPlayFileFormat)) {
		return E_SYS;
	} else {
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILEID, &gMenuPlayInfo.DispFileName[0], NULL);
		pFlist->GetInfo(PBX_FLIST_GETINFO_DIRID, &gMenuPlayInfo.DispDirName[0], NULL);
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILESEQ, &gMenuPlayInfo.DispFileSeq[0], NULL);
		return E_OK;
	}
}
