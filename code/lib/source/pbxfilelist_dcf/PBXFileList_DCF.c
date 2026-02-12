#include "PBXFileList_DCF.h"
#include "PBXFileList_DCF_Int.h"
#if 1
#include "DCF.h"
#include "FileSysTsk.h"
#endif

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          pbxflist_dcf
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass

#include "kwrap/debug.h"
unsigned int pbxflist_dcf_debug_level = NVT_DBG_WRN;
///////////////////////////////////////////////////////////////////////////////

//NVTVER_VERSION_ENTRY(PBXFList_DCF, 1, 00, 000, 00)

static BOOL     gbPBXNoFile = TRUE;
#if 1
/**
    delete file command.
*/
typedef enum _PBX_FLIST_DELETE_CMD {
	PBX_FLIST_DELETE_CURR = 0,                   ///< delete current file
	PBX_FLIST_DELETE_ALL,                        ///< delete all files in the file list
	PBX_FLIST_DELETE_MAX_ID,                     ///< total delete command numbers
	ENUM_DUMMY4WORD(PBX_FLIST_DELETE_CMD)
} PBX_FLIST_DELETE_CMD;


/**
    Protect file command.
*/
typedef enum _PBX_FLIST_PROTECT_CMD {
	PBX_FLIST_PROTECT_CURR = 0,                   ///< Protect current file
	PBX_FLIST_PROTECT_ALL,                        ///< Protect all files in the file list
	PBX_FLIST_UNPROTECT_CURR,                     ///< Un-Protect current file
	PBX_FLIST_UNPROTECT_ALL,                      ///< Un-Protect all files in the file list
	PBX_FLIST_PROTECT_MAX_ID,                     ///< total protect command numbers
	ENUM_DUMMY4WORD(PBX_FLIST_PROTECT_CMD)
} PBX_FLIST_PROTECT_CMD;
#endif

static PBX_FLIST_OBJ gDCFListObj = {
	PBXFList_DCF_Config,
	PBXFList_DCF_Init,
	PBXFList_DCF_UnInit,
	PBXFList_DCF_GetInfo,
	PBXFList_DCF_SetInfo,
	PBXFList_DCF_SeekIndex,
	PBXFList_DCF_MakeFilePath,
	PBXFList_DCF_AddFile,
	PBXFList_DCF_Refresh,
};



PPBX_FLIST_OBJ PBXFList_DCF_getObject(void)
{
	return &gDCFListObj;
}

ER PBXFList_DCF_Config(UINT32 ConfigID, UINT32 param1, UINT32 param2)
{
	DBG_IND("ConfigID =  0x%x, param1 = %d, param2 = %d\r\n", ConfigID, param1, param2);
	switch (ConfigID) {
	default:
		break;

	}
	return E_OK;
}

ER PBXFList_DCF_Init(void)
{
	return E_OK;
}

ER PBXFList_DCF_UnInit(void)
{
	return E_OK;
}

#if 1
static UINT32 xDCFFileType2PBXFileType(UINT32  fileType)
{
	UINT32 rtnType = 0;

	if (fileType & DCF_FILE_TYPE_JPG) {
		rtnType |= PBX_FLIST_FILE_TYPE_JPG;
	}
	if (fileType & DCF_FILE_TYPE_AVI) {
		rtnType |= PBX_FLIST_FILE_TYPE_AVI;
	}
	if (fileType & DCF_FILE_TYPE_WAV) {
		rtnType |= PBX_FLIST_FILE_TYPE_WAV;
	}
	if (fileType & DCF_FILE_TYPE_RAW) {
		rtnType |= PBX_FLIST_FILE_TYPE_RAW;
	}
	if (fileType & DCF_FILE_TYPE_TIF) {
		rtnType |= PBX_FLIST_FILE_TYPE_TIF;
	}
	if (fileType & DCF_FILE_TYPE_MPO) {
		rtnType |= PBX_FLIST_FILE_TYPE_MPO;
	}
	if (fileType & DCF_FILE_TYPE_MOV) {
		rtnType |= PBX_FLIST_FILE_TYPE_MOV;
	}
	if (fileType & DCF_FILE_TYPE_MP4) {
		rtnType |= PBX_FLIST_FILE_TYPE_MP4;
	}
	if (fileType & DCF_FILE_TYPE_MPG) {
		rtnType |= PBX_FLIST_FILE_TYPE_MPG;
	}
	if (fileType & DCF_FILE_TYPE_ASF) {
		rtnType |= PBX_FLIST_FILE_TYPE_ASF;
	}

	return rtnType;
}
#endif

#if 1
static UINT32 xPBXFileType2DCFFileType(UINT32  fileType)
{
	UINT32 rtnType = 0;

	if (fileType & PBX_FLIST_FILE_TYPE_JPG) {
		rtnType |= DCF_FILE_TYPE_JPG;
	}
	if (fileType & PBX_FLIST_FILE_TYPE_AVI) {
		rtnType |= DCF_FILE_TYPE_AVI;
	}
	if (fileType & PBX_FLIST_FILE_TYPE_WAV) {
		rtnType |= DCF_FILE_TYPE_WAV;
	}
	if (fileType & PBX_FLIST_FILE_TYPE_RAW) {
		rtnType |= DCF_FILE_TYPE_RAW;
	}
	if (fileType & PBX_FLIST_FILE_TYPE_TIF) {
		rtnType |= DCF_FILE_TYPE_TIF;
	}
	if (fileType & PBX_FLIST_FILE_TYPE_MPO) {
		rtnType |= DCF_FILE_TYPE_MPO;
	}
	if (fileType & PBX_FLIST_FILE_TYPE_MOV) {
		rtnType |= DCF_FILE_TYPE_MOV;
	}
	if (fileType & PBX_FLIST_FILE_TYPE_MP4) {
		rtnType |= DCF_FILE_TYPE_MP4;
	}
	if (fileType & PBX_FLIST_FILE_TYPE_MPG) {
		rtnType |= DCF_FILE_TYPE_MPG;
	}
	if (fileType & PBX_FLIST_FILE_TYPE_ASF) {
		rtnType |= DCF_FILE_TYPE_ASF;
	}

	return rtnType;
}
#endif

ER PBXFList_DCF_GetInfo(UINT32  InfoID, VOID *pparam1, VOID *pparam2)
{
	DBG_IND("InfoID = 0x%x, ", InfoID);
	if (!pparam1) {
		DBG_ERR("pparam1 is NULL \r\n");
		return E_PAR;
	}
	#if 1
	switch (InfoID) {
	case PBX_FLIST_GETINFO_FILESEQ:
		*(UINT32 *)pparam1 = DCF_GetDBInfo(DCF_INFO_CUR_INDEX);
		DBG_MSG("index = %d \r\n", *(UINT32 *)pparam1);
		break;
	case PBX_FLIST_GETINFO_FILENUMS:
		*(UINT32 *)pparam1 = DCF_GetDBInfo(DCF_INFO_TOL_FILE_COUNT);
		DBG_MSG("TolFileNum = %d \r\n", *(UINT32 *)pparam1);
		break;
	case PBX_FLIST_GETINFO_READONLY: {

			UINT32     CurIndex, DirectoryID = 0, FilenameID = 0, uiThisFileFormat = 0, fileCount;
			UINT8      attrib = 0;
			int        ret;
			CHAR       filePath[DCF_FULL_FILE_PATH_LEN];

			fileCount = DCF_GetDBInfo(DCF_INFO_TOL_FILE_COUNT);
			if (!fileCount) {
				goto DCF_FileCount_Zero;
			}
			CurIndex = DCF_GetCurIndex();
			DCF_GetObjInfo(CurIndex, &DirectoryID, &FilenameID, &uiThisFileFormat);
			if (uiThisFileFormat & DCF_FILE_TYPE_JPG) {
				uiThisFileFormat = DCF_FILE_TYPE_JPG;
			}
			DCF_GetObjPath(CurIndex, uiThisFileFormat, filePath);
			ret = FileSys_GetAttrib(filePath, &attrib);
			if ((ret == E_OK) && M_IsReadOnly(attrib) == TRUE) {
				*(UINT32 *)pparam1 = TRUE;
			} else {
				*(UINT32 *)pparam1 = FALSE;
			}
			DBG_MSG("isReadOnly = %d \r\n", *(UINT32 *)pparam1);
		}
		break;
	case PBX_FLIST_GETINFO_FILESIZE64: {
			UINT32     CurIndex, DirectoryID = 0, FilenameID = 0, uiThisFileFormat = 0, fileCount;
			CHAR       filePath[DCF_FULL_FILE_PATH_LEN];

			fileCount = DCF_GetDBInfo(DCF_INFO_TOL_FILE_COUNT);
			if (!fileCount) {
				goto DCF_FileCount_Zero;
			}
			CurIndex = DCF_GetDBInfo(DCF_INFO_CUR_INDEX);
			DCF_GetObjInfo(CurIndex, &DirectoryID, &FilenameID, &uiThisFileFormat);
			if (uiThisFileFormat & DCF_FILE_TYPE_JPG) {
				uiThisFileFormat = DCF_FILE_TYPE_JPG;
			}
			DCF_GetObjPath(CurIndex, uiThisFileFormat, filePath);
			*(UINT64 *)pparam1 = FileSys_GetFileLen(filePath);
			DBG_MSG("fileSize = %lld \r\n", *(UINT64 *)pparam1);
		}
		break;
	case PBX_FLIST_GETINFO_FILETYPE:
		*(UINT32 *)pparam1 = xDCFFileType2PBXFileType(DCF_GetDBInfo(DCF_INFO_CUR_FILE_TYPE));
		DBG_MSG("fileType = 0x%x \r\n", *(UINT32 *)pparam1);
		break;
	case PBX_FLIST_GETINFO_FILEPATH: {
			UINT32     CurIndex, DirectoryID = 0, FilenameID = 0, uiThisFileFormat = 0, fileCount;

			fileCount = DCF_GetDBInfo(DCF_INFO_TOL_FILE_COUNT);
			if (!fileCount) {
				goto DCF_FileCount_Zero;
			}

			CurIndex = DCF_GetCurIndex();
			DCF_GetObjInfo(CurIndex, &DirectoryID, &FilenameID, &uiThisFileFormat);
			if (uiThisFileFormat & DCF_FILE_TYPE_JPG) {
				uiThisFileFormat = DCF_FILE_TYPE_JPG;
			}
			DCF_GetObjPath(CurIndex, uiThisFileFormat, (CHAR *)pparam1);
			DBG_MSG("filePath = %s \r\n", (CHAR *)pparam1);
		}
		break;

	case PBX_FLIST_GETINFO_ISWITHMEMO: {
			UINT32 uiThisFileFormat = 0;
			uiThisFileFormat = DCF_GetDBInfo(DCF_INFO_CUR_FILE_TYPE);
			if ((uiThisFileFormat & DCF_FILE_TYPE_WAV) && (uiThisFileFormat & DCF_FILE_TYPE_JPG)) {
				*(BOOL *)pparam1 = TRUE;
			} else {
				*(BOOL *)pparam1 = FALSE;
			}
			DBG_MSG("isWithMemo = %d \r\n", *(BOOL *)pparam1);
		}
		break;
	case PBX_FLIST_GETINFO_FILEID:
		*(UINT32 *)pparam1 = DCF_GetDBInfo(DCF_INFO_CUR_FILE_ID);
		DBG_MSG("fileID = %d \r\n", *(UINT32 *)pparam1);
		break;
	case PBX_FLIST_GETINFO_DIRID:
		*(UINT32 *)pparam1 = DCF_GetDBInfo(DCF_INFO_CUR_DIR_ID);
		DBG_MSG("DirID = %d \r\n", *(UINT32 *)pparam1);
		break;
	case PBX_FLIST_GETINFO_VALID_FILETYPE:
		*(UINT32 *)pparam1 = xDCFFileType2PBXFileType(DCF_GetDBInfo(DCF_INFO_VALID_FILE_FMT));
		DBG_MSG("ValidfileType = 0x%x \r\n", *(UINT32 *)pparam1);
		break;
	case PBX_FLIST_GETINFO_FILENUMS_INDIR:
		*(UINT32 *)pparam1 = DCF_GetDBInfo(DCF_INFO_DIR_FILE_NUM);
		DBG_MSG("fileNuminDir = %d \r\n", *(UINT32 *)pparam1);
		break;
	case PBX_FLIST_GETINFO_FILESEQ_INDIR:
		*(UINT32 *)pparam1 = DCF_GetDBInfo(DCF_INFO_DIR_FILE_SEQ);
		DBG_MSG("fileSeqInDir = %d \r\n", *(UINT32 *)pparam1);
		break;

	default:
		*(UINT32 *)pparam1 = 0;
		DBG_ERR("Not Support InfoID = 0x%x\r\n", InfoID);
		return E_PAR;

	}
	return E_OK;
DCF_FileCount_Zero:
	*(UINT32 *)pparam1 = 0;
	DBG_ERR("InfoID = 0x%x, FileNums=0\r\n", InfoID);
	#endif
	return E_SYS;
}

ER PBXFList_DCF_SetInfo(UINT32  InfoID, UINT32 param1, UINT32 param2)
{
	#if 1
	UINT32 fileCount;
	DBG_IND("InfoID = 0x%x, param1= %d\r\n", InfoID, param1);

	switch (InfoID) {
	case PBX_FLIST_SETINFO_CURINDEX:
		fileCount = DCF_GetDBInfo(DCF_INFO_TOL_FILE_COUNT);
		if (!fileCount) {
			DBG_ERR("InfoID = 0x%x, FileNums=0\r\n", InfoID);
			break;
		}
		DCF_SetCurIndex(param1);
		break;
	default:
		DBG_ERR("Not Support InfoID = 0x%x\r\n", InfoID);
		return E_PAR;

	}
	#endif
	return E_OK;
}

ER PBXFList_DCF_SeekIndex(INT32 offset, PBX_FLIST_SEEK_CMD seekCmd)
{
	DBG_IND("offset=%d,seekCmd=%d\r\n", offset, seekCmd);

	#if 1
	UINT32 index, fileCount;


	fileCount = DCF_GetDBInfo(DCF_INFO_TOL_FILE_COUNT);
	if (!fileCount) {
		DBG_ERR("FileNums=0\r\n");
		return E_SYS;
	}
	if (seekCmd == PBX_FLIST_SEEK_SET) {
		index = DCF_SeekIndex(offset, DCF_SEEK_SET);
	} else if (seekCmd == PBX_FLIST_SEEK_CUR) {
		index = DCF_SeekIndex(offset, DCF_SEEK_CUR);
	} else if (seekCmd == PBX_FLIST_SEEK_END) {
		index = DCF_SeekIndex(offset, DCF_SEEK_END);
	} else {
		DBG_ERR("Not Support seekCmd = %d\r\n", seekCmd);
		return E_PAR;
	}
	if (index) {
		return E_OK;
	} else {
		DBG_ERR("offset=%d,seekCmd=%d\r\n", offset, seekCmd);
		return E_PAR;
	}
	#else
	return E_SYS;
	#endif
}

ER PBXFList_DCF_MakeFilePath(UINT32  fileType, CHAR *path)
{
	#if 1
	UINT32 DirID = 0, FileID = 0;

	DBG_IND("fileType=0x%x\r\n", fileType);
	if (DCF_GetNextID(&DirID, &FileID) == FALSE) {
		return E_SYS;
	}
	if (DCF_MakeObjPath(DirID, FileID, xPBXFileType2DCFFileType(fileType), path)) {
		return E_OK;
	} else {
		return E_SYS;
	}
	#else
	return E_SYS;
	#endif
}

ER PBXFList_DCF_AddFile(CHAR *path)
{
	#if 1
	DBG_IND("path=%s\r\n", path);
	if (DCF_AddDBfile(path)) {
		return E_OK;
	} else {
		return E_SYS;
	}
	#else
	return E_SYS;
	#endif
}

#if 1
static void xPBXFList_Filesys_DelCB(FIND_DATA *pFindData, BOOL *bDelete, UINT32 Param1, UINT32 Param2)
{
	char    *pFileName;
	char    *pExtName;
	INT32    fileNum;
	UINT32   fileType;
	UINT32   filterType = (DCF_GetDBInfo(DCF_INFO_VALID_FILE_FMT) | DCF_GetDBInfo(DCF_INFO_DEP_FILE_FMT));
	UINT8     attrib;

	pFileName = pFindData->FATMainName;
	pExtName = pFindData->FATExtName;
	attrib    = pFindData->attrib;

	DBG_IND("pFileName = %s\r\n", pFileName);

	fileNum = DCF_IsValidFile(pFileName, pExtName, &fileType);

	if (!M_IsReadOnly(attrib) && fileNum && (filterType & fileType)) {
		*bDelete = TRUE;
	} else {
		*bDelete = FALSE;
		gbPBXNoFile = FALSE;
	}
}

static void xPBXFList_DCF_DelAll(void)
{
	SDCFDIRINFO dirinfo;
	char path[DCF_FULL_FILE_PATH_LEN];
	BOOL ret;
	UINT32 i, uiMaxDirNum;

	uiMaxDirNum = DCF_GetDBInfo(DCF_INFO_MAX_DIR_ID);
	for (i = 100; i <= uiMaxDirNum; i++) {
		// check if folder has file
		ret = DCF_GetDirInfo(i, &dirinfo);
		if (ret /*&& dirinfo.uiNumOfDcfObj*/) {
			// delete all in one folder
			DCF_GetDirPath(i, path);
			gbPBXNoFile = TRUE;
			FileSys_DelDirFiles(path, xPBXFList_Filesys_DelCB);
			// delete empty folder
			if (gbPBXNoFile) {
				FileSys_DeleteDir(path);
			}
		}
	}
	DCF_Refresh();
}



ER PBXFList_DCF_DelFile(UINT32  cmdID, UINT32 param1)
{
	DBG_IND("cmdID=%d\r\n", cmdID);
	Perf_Mark();
	if (cmdID > PBX_FLIST_DELETE_MAX_ID) {
		DBG_ERR("cmdID=%d\r\n", cmdID);
		return E_PAR;
	}
	if (cmdID == PBX_FLIST_DELETE_CURR) {
		UINT32 index, uiThisFileFormat, i, tmpFileType = 1;
		CHAR   filePath[DCF_FULL_FILE_PATH_LEN];

		index = DCF_GetCurIndex();
		uiThisFileFormat = DCF_GetDBInfo(DCF_INFO_CUR_FILE_TYPE);
		for (i = 0; i <= DCF_FILE_TYPE_NUM; i++) {
			tmpFileType = 0x0001 << i;
			if (uiThisFileFormat & tmpFileType) {
				DCF_GetObjPath(index, uiThisFileFormat, filePath);
				FileSys_DeleteFile(filePath);
				DCF_DelDBfile(filePath);
			}
		}
	} else if (cmdID == PBX_FLIST_DELETE_ALL) {
		char        filePath[DCF_FULL_FILE_PATH_LEN];
		BOOL        ret;
		UINT8       attrib;
		UINT32      DirID, FileID, Index, uiThisFileFormat;
		BOOL        isCurrFileReadOnly = FALSE;

		Index = DCF_GetDBInfo(DCF_INFO_CUR_INDEX);
		DCF_GetObjInfo(Index, &DirID, &FileID, &uiThisFileFormat);
		DCF_GetObjPath(Index, uiThisFileFormat, filePath);

		ret = FileSys_GetAttrib(filePath, &attrib);
		if ((ret == E_OK) && M_IsReadOnly(attrib) == TRUE) {
			isCurrFileReadOnly = TRUE;
		}

		// ---------  delete all files  -----------------------
		xPBXFList_DCF_DelAll();
		if (isCurrFileReadOnly) {
			Index = DCF_GetIndexByID(DirID, FileID);
			DCF_SetCurIndex(Index);
		}
		DCF_Refresh();
	}
	DBG_IND("DelTime = %04d ms \r\n", Perf_GetDuration() / 1000);
	return E_OK;

}

static void xPBXFList_Filesys_LockCB(FIND_DATA *pFindData, BOOL *bApply, UINT32 Param1, UINT32 Param2)
{
	char    *pFileName;
	char    *pExtName;
	INT32    fileNum;
	UINT32   fileType;
	UINT32   filterType = (DCF_GetDBInfo(DCF_INFO_VALID_FILE_FMT) | DCF_GetDBInfo(DCF_INFO_DEP_FILE_FMT));

	pFileName = pFindData->FATMainName;
	pExtName  = pFindData->FATExtName;
	DBG_IND("pFileName = %s\r\n", pFileName);
	fileNum = DCF_IsValidFile(pFileName, pExtName, &fileType);
	if (fileNum && (filterType & fileType)) {
		*bApply = TRUE;
	} else {
		*bApply = FALSE;
	}
}

static void xPBXFList_DCF_LockAll(BOOL bLock)
{
	SDCFDIRINFO dirinfo;
	char path[DCF_FULL_FILE_PATH_LEN];
	UINT32 i, uiMaxDirNum;

	uiMaxDirNum = DCF_GetDBInfo(DCF_INFO_MAX_DIR_ID);
	for (i = 100; i <= uiMaxDirNum; i++) {
		// check if folder has file
		DCF_GetDirInfo(i, &dirinfo);
		if (dirinfo.uiNumOfDcfObj) {
			DCF_GetDirPath(i, path);
			FileSys_LockDirFiles(path, bLock, xPBXFList_Filesys_LockCB);
		}
	}
}


ER PBXFList_DCF_ProtectFile(UINT32  cmdID, UINT32 param1)
{
	char     filePath[DCF_FULL_FILE_PATH_LEN];
	UINT32   index, i, uiThisFileFormat, tmpFileType;
	BOOL     bLock;
	UINT32   uiMaxDirNum;

	Perf_Mark();
	DBG_IND("cmdID=%d\r\n", cmdID);
	if ((cmdID == PBX_FLIST_PROTECT_CURR) || (cmdID == PBX_FLIST_UNPROTECT_CURR)) {
		if (cmdID == PBX_FLIST_PROTECT_CURR) {
			bLock = TRUE;    // lock
		} else {
			bLock = FALSE;    // unlock
		}
		index = DCF_GetCurIndex();
		uiThisFileFormat = DCF_GetDBInfo(DCF_INFO_CUR_FILE_TYPE);
		for (i = 0; i <= DCF_FILE_TYPE_NUM; i++) {
			tmpFileType = 1;
			tmpFileType <<= i;
			if (uiThisFileFormat & tmpFileType) {
				DCF_GetObjPath(index, tmpFileType, filePath);
				FileSys_SetAttrib(filePath, FST_ATTRIB_READONLY, bLock);
			}
		}
	} else if ((cmdID == PBX_FLIST_PROTECT_ALL) || (cmdID == PBX_FLIST_UNPROTECT_ALL)) {
		if (cmdID == PBX_FLIST_PROTECT_ALL) {
			bLock  = TRUE;    // lock
		} else {
			bLock  = FALSE;    // unlock
		}
		uiMaxDirNum = DCF_GetDBInfo(DCF_INFO_MAX_DIR_ID);
		(void) uiMaxDirNum;
		xPBXFList_DCF_LockAll(bLock);
	}
	DBG_IND("ProtectTime = %04d ms \r\n", Perf_GetDuration() / 1000);
	return E_OK;
}
#endif

ER PBXFList_DCF_Refresh(void)
{
	#if 1
	DBG_IND("\r\n");
	DCF_Refresh();
	#endif
	return E_OK;
}



