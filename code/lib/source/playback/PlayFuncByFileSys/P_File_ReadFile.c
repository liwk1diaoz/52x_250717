#include "FileSysTsk.h"
#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFile.h"
#include "PBXFileList.h"

/*
    Read file by file-system-task.
    This is internal API.

    @param[in] CommandID read next/prev/current/continue DCF file
    @param[in] pFileBuf buffer to store file
    @param[in] BufSize read file size
    @param[in] FilePosition read file from FilePosition
    @param[in] JumpOffset Jump file numbers when read next/prev file
    @return void
*/
ER PB_ReadFileByFileSys(UINT32 CommandID, INT8 *pFileBuf, UINT32 BufSize, UINT64 FilePosition, UINT32 JumpOffset)
{
	UINT32 CurIndex, NewIndex;
	UINT64 fileSize;
	CHAR   filePath[PBX_FLIST_NAME_MAX_LENG];
	UINT32 uiThisFileFormat;
	INT32  i32offset;
	PBXFile_Access_Info64   fileAccess = {0};
	PPBX_FLIST_OBJ        pFlist = g_PBSetting.pFileListObj;
	UINT32 u32DebugMsg = FALSE;

    PB_GetParam(PBPRMID_ENABLE_DEBUG_MSG, &u32DebugMsg);

	CurIndex = gMenuPlayInfo.CurIndexOfTotal;
	pFlist->SeekIndex(CurIndex, PBX_FLIST_SEEK_SET);
	switch (CommandID) {
	case PB_FILE_READ_SPECIFIC:
		NewIndex = gMenuPlayInfo.DispFileSeq[gMenuPlayInfo.CurFileIndex - 1];
		pFlist->SeekIndex(NewIndex, PBX_FLIST_SEEK_SET);
		//Restore correct gMenuPlayInfo.CurIndexOfTotal after leaving Browser mode.
		if (PB_CMMD_SINGLE == guiCurrPlayCmmd) {
			gMenuPlayInfo.CurIndexOfTotal =  NewIndex;
		}
		break;

	case PB_FILE_READ_NEXT:
		i32offset = JumpOffset;
		pFlist->SeekIndex(i32offset, PBX_FLIST_SEEK_CUR);
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILESEQ, &gMenuPlayInfo.CurIndexOfTotal, NULL);
		break;

	case PB_FILE_READ_PREV:
		i32offset = -JumpOffset;
		pFlist->SeekIndex(i32offset, PBX_FLIST_SEEK_CUR);
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILESEQ, &gMenuPlayInfo.CurIndexOfTotal, NULL);
		break;

	case PB_FILE_READ_LAST:
		pFlist->SeekIndex(0, PBX_FLIST_SEEK_END);
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILESEQ, &gMenuPlayInfo.CurIndexOfTotal, NULL);
		break;

	case PB_FILE_READ_CONTINUE:
	default:
		break;
	}
	pFlist->GetInfo(PBX_FLIST_GETINFO_FILEPATH, (UINT32 *)filePath, NULL);

	if ((FilePosition == 0) && (u32DebugMsg)) {
		DBG_DUMP("%s: File path=%s\r\n", __func__, filePath);
	}

	if (CommandID == PB_FILE_READ_CONTINUE) {
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILESIZE64, &fileSize, NULL);

		if ((BufSize + FilePosition) >  fileSize) {
			BufSize = fileSize - FilePosition;
		}
	}
	pFlist->GetInfo(PBX_FLIST_GETINFO_FILETYPE, &uiThisFileFormat, NULL);
	gusPlayThisFileFormat = uiThisFileFormat;

	fileAccess.fileCmd = PBX_FILE_CMD_READ;
	fileAccess.fileName = (UINT8 *)filePath;
	fileAccess.pBuf = (UINT8 *)pFileBuf;
	fileAccess.bufSize = BufSize;
	fileAccess.filePos = FilePosition;
	return PBXFile_Access64(&fileAccess);
}

