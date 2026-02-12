#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFile.h"
#include "PBXFileList.h"

/*
    Write file by file-system-task.
    This is internal API.

    @param[in] Buffer starting address of new file
    @param[in] New file size
    @param[in] TRUE: over-write file, FALSE: create new file
    @param[in] TRUE: Sync date & time with original file, FALSE: No sync time
    @return The process result
*/
INT32 PB_WriteFileByFileSys(UINT32 uiNewFileStartAddr, UINT64 ui64NewFileSZ, BOOL bIfOverWrite, BOOL bIfSyncOriTime)
{
	//#NT#2013/02/20#Lincy Lin -begin
	//#NT#Support Filelist plugin

	UINT64 ui64StorageRemainSize;
	UINT64 uiOriFileSZ;
	UINT32 uiErrCheck;
	UINT32 uiCreDateTime[6], uiModDateTime[6];
	CHAR   cFileName[PBX_FLIST_NAME_MAX_LENG];
	PBXFile_Access_Info64  fileAccess = {0};
	PPBX_FLIST_OBJ       pFlist = g_PBSetting.pFileListObj;

	pFlist->GetInfo(PBX_FLIST_GETINFO_FILESIZE64, &uiOriFileSZ, NULL);

	// get disk-info, remain storage size
	ui64StorageRemainSize = PBXFile_GetFreeSpace();
	if (bIfOverWrite == FALSE) {
		UINT32 fileSize;

		if (ui64StorageRemainSize <= ui64NewFileSZ) {
			DBG_ERR("Write File NG1!\r\n");
			return E_SYS;
		}

		if (bIfSyncOriTime) {
			pFlist->GetInfo(PBX_FLIST_GETINFO_FILEPATH, (UINT32 *)cFileName, NULL);
			PBXFile_GetTime(cFileName, uiCreDateTime, uiModDateTime);
		}
		fileSize = ui64NewFileSZ;
		uiErrCheck = pFlist->MakeFilePath(PBX_FLIST_FILE_TYPE_JPG, cFileName);
		if (uiErrCheck != E_OK) {
			DBG_ERR("MakePath fail!\r\n");
			return E_SYS;
		}
		fileAccess.fileCmd = PBX_FILE_CMD_WRITE;
		fileAccess.fileName = (UINT8 *)cFileName;
		fileAccess.pBuf = (UINT8 *)uiNewFileStartAddr;
		fileAccess.bufSize = fileSize;
		fileAccess.filePos = 0;
		uiErrCheck = PBXFile_Access64(&fileAccess);
		if (uiErrCheck == E_OK) {
			pFlist->AddFile(cFileName);
			//#NT#2012/06/25#Lincy Lin -begin
			//#NT#Fix read wrong file bug
			pFlist->GetInfo(PBX_FLIST_GETINFO_FILESEQ, &gMenuPlayInfo.CurIndexOfTotal, NULL);
			pFlist->GetInfo(PBX_FLIST_GETINFO_DIRID, &gMenuPlayInfo.DispDirName[gMenuPlayInfo.CurFileIndex - 1], NULL);
			pFlist->GetInfo(PBX_FLIST_GETINFO_FILEID, &gMenuPlayInfo.DispFileName[gMenuPlayInfo.CurFileIndex - 1], NULL);
			gMenuPlayInfo.DispFileSeq[gMenuPlayInfo.CurFileIndex - 1] = gMenuPlayInfo.CurIndexOfTotal;
			//#NT#2012/06/25#Lincy Lin -end
			if (bIfSyncOriTime) {
				PBXFile_SetTime(cFileName, uiCreDateTime, uiModDateTime);
			}
		}
	} else {
		if ((uiOriFileSZ + ui64StorageRemainSize) <= ui64NewFileSZ) {
			DBG_ERR("Write File NG2!\r\n");
			return E_SYS;
		}
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILEPATH, (UINT32 *)cFileName, NULL);
		fileAccess.fileCmd = PBX_FILE_CMD_WRITE;
		fileAccess.fileName = (UINT8 *)cFileName;
		fileAccess.pBuf = (UINT8 *)uiNewFileStartAddr;
		fileAccess.bufSize = ui64NewFileSZ;
		fileAccess.filePos = 0;
		uiErrCheck = PBXFile_Access64(&fileAccess);
	}
	return uiErrCheck;
	//#NT#2013/02/20#Lincy Lin -end
}

