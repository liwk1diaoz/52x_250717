#include "PBXFileList_FileDB.h"
#include "PBXFileList_FileDB_Int.h"
#include "FileDB.h"
#include "FileSysTsk.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          pbxflist_filedb
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass

#include "kwrap/debug.h"
unsigned int pbxflist_filedb_debug_level = NVT_DBG_WRN;
///////////////////////////////////////////////////////////////////////////////


//NVTVER_VERSION_ENTRY(PBXFList_FDB, 1, 00, 000, 00)

#define DUMP_INFO           0


#define DCF_FOLDER_ID_INDEX          8
#define DCF_FILE_ID_INDEX            21


static FILEDB_HANDLE gPBXFDBHdl = FILEDB_CREATE_ERROR;
static CHAR          gRootPath[PBX_FLIST_NAME_MAX_LENG + 1] = "A:\\";
static CHAR          gDefaultFolder[PBX_FLIST_NAME_MAX_LENG + 1] = {0};
static CHAR          gFilterFolder[PBX_FLIST_NAME_MAX_LENG + 1] = {0};
static UINT32        gSortType = PBX_FLIST_SORT_BY_NONE;
static BOOL          gbIsSupportLongName = FALSE;
static BOOL          gbIsSortLargeFirst = FALSE;

static FILEDB_INIT_OBJ gPBXFDBInitObj = {
	NULL,  //rootPath
	NULL,  //defaultfolder
	NULL,  //filterfolder
	TRUE,  //bIsRecursive
	TRUE,  //bIsCyclic
	TRUE,  //bIsMoveToLastFile
	FALSE, //bIsSupportLongName
	FALSE, //bIsDCFFileOnly
	TRUE,  //bIsFilterOutSameDCFNumFolder
	{'N', 'V', 'T', 'I', 'M'}, //OurDCFDirName[5]
	{'I', 'M', 'A', 'G'}, //OurDCFFileName[4]
	FALSE,  //bIsFilterOutSameDCFNumFile
	FALSE, //bIsChkHasFile
	50,    //u32MaxFilePathLen
	10000,  //u32MaxFileNum
	FILEDB_FMT_ANY,       //fileFilter
	0,     //u32MemAddr
	0,     //u32MemSize
	NULL,  //fpChkAbort
	0,     //bIsSkipDirForNonRecursive
	1,     //bIsSkipHidden
	NULL,  //SortSN_Delim
	0,     //SortSN_DelimCount
	0,     //SortSN_CharNumOfSN
};

static PBX_FLIST_OBJ gFDBListObj = {
	PBXFList_FDB_Config,
	PBXFList_FDB_Init,
	PBXFList_FDB_UnInit,
	PBXFList_FDB_GetInfo,
	PBXFList_FDB_SetInfo,
	PBXFList_FDB_SeekIndex,
	PBXFList_FDB_MakeFilePath,
	PBXFList_FDB_AddFile,
	PBXFList_FDB_Refresh,
};


static UINT32 xFDBFileType2PBXFileType(UINT32  fileType)
{
	UINT32 rtnType = 0;

	if (fileType & FILEDB_FMT_JPG) {
		rtnType |= PBX_FLIST_FILE_TYPE_JPG;
	}
	if (fileType & FILEDB_FMT_AVI) {
		rtnType |= PBX_FLIST_FILE_TYPE_AVI;
	}
	if (fileType & FILEDB_FMT_WAV) {
		rtnType |= PBX_FLIST_FILE_TYPE_WAV;
	}
	if (fileType & FILEDB_FMT_RAW) {
		rtnType |= PBX_FLIST_FILE_TYPE_RAW;
	}
	/*
	if (fileType & FILEDB_FMT_TIF)
	    rtnType |=PBX_FLIST_FILE_TYPE_TIF;
	if (fileType & FILEDB_FMT_MPO)
	    rtnType |=PBX_FLIST_FILE_TYPE_MPO;
	*/
	if (fileType & FILEDB_FMT_MOV) {
		rtnType |= PBX_FLIST_FILE_TYPE_MOV;
	}
	if (fileType & FILEDB_FMT_MP4) {
		rtnType |= PBX_FLIST_FILE_TYPE_MP4;
	}
	if (fileType & FILEDB_FMT_MPG) {
		rtnType |= PBX_FLIST_FILE_TYPE_MPG;
	}
	if (fileType & FILEDB_FMT_ASF) {
		rtnType |= PBX_FLIST_FILE_TYPE_ASF;
	}
	if (fileType & FILEDB_FMT_TS) {
		rtnType |= PBX_FLIST_FILE_TYPE_TS;
	}

	return rtnType;
}

static UINT32 xPBXFileType2FDBFileType(UINT32  fileType)
{
	UINT32 rtnType = 0;

	if (fileType & PBX_FLIST_FILE_TYPE_JPG) {
		rtnType |= FILEDB_FMT_JPG;
	}
	if (fileType & PBX_FLIST_FILE_TYPE_AVI) {
		rtnType |= FILEDB_FMT_AVI;
	}
	if (fileType & PBX_FLIST_FILE_TYPE_WAV) {
		rtnType |= FILEDB_FMT_WAV;
	}
	if (fileType & PBX_FLIST_FILE_TYPE_RAW) {
		rtnType |= FILEDB_FMT_RAW;
	}
	/*
	if (fileType & PBX_FLIST_FILE_TYPE_TIF)
	    rtnType |=FILEDB_FMT_TIF;
	if (fileType & PBX_FLIST_FILE_TYPE_MPO)
	    rtnType |=FILEDB_FMT_MPO;
	*/
	if (fileType & PBX_FLIST_FILE_TYPE_MOV) {
		rtnType |= FILEDB_FMT_MOV;
	}
	if (fileType & PBX_FLIST_FILE_TYPE_MP4) {
		rtnType |= FILEDB_FMT_MP4;
	}
	if (fileType & PBX_FLIST_FILE_TYPE_MPG) {
		rtnType |= FILEDB_FMT_MPG;
	}
	if (fileType & PBX_FLIST_FILE_TYPE_ASF) {
		rtnType |= FILEDB_FMT_ASF;
	}
	if (fileType & PBX_FLIST_FILE_TYPE_TS) {
		rtnType |= FILEDB_FMT_TS;
	}

	return rtnType;
}

static UINT32 xPBXSortType2FDBSortType(UINT32  sortType)
{
	if (sortType == PBX_FLIST_SORT_BY_NONE) {
		return FILEDB_SORT_BY_NONE;
	}
	if (sortType == PBX_FLIST_SORT_BY_CREDATE) {
		return FILEDB_SORT_BY_CREDATE;
	}
	if (sortType == PBX_FLIST_SORT_BY_MODDATE) {
		return FILEDB_SORT_BY_MODDATE;
	}
	if (sortType == PBX_FLIST_SORT_BY_NAME) {
		return FILEDB_SORT_BY_NAME;
	}
	if (sortType == PBX_FLIST_SORT_BY_FILEPATH) {
		return FILEDB_SORT_BY_FILEPATH;
	}
	if (sortType == PBX_FLIST_SORT_BY_SN) {
		return FILEDB_SORT_BY_SN;
	}
	DBG_ERR("sortType = %d\r\n", sortType);
	return FILEDB_SORT_BY_NONE;
}


PPBX_FLIST_OBJ PBXFList_FDB_getObject(void)
{
	return &gFDBListObj;
}



ER PBXFList_FDB_Config(UINT32 ConfigID, UINT32 param1, UINT32 param2)
{
	PFILEDB_INIT_OBJ   pFDBInitObj = &gPBXFDBInitObj;

	DBG_IND("ConfigID =  0x%x, param1 = %d, param2 = %d\r\n", ConfigID, param1, param2);
	switch (ConfigID) {
	case PBX_FLIST_CONFIG_MEM:
		pFDBInitObj->u32MemAddr = param1;
		pFDBInitObj->u32MemSize = param2;
		break;
	case PBX_FLIST_CONFIG_MAX_FILENUM:
		pFDBInitObj->u32MaxFileNum = param1;
		break;
	case PBX_FLIST_CONFIG_MAX_FILEPATH_LEN:
		pFDBInitObj->u32MaxFilePathLen = param1;
		break;
	case PBX_FLIST_CONFIG_VALID_FILETYPE:
		pFDBInitObj->fileFilter = xPBXFileType2FDBFileType(param1);
		break;
	case PBX_FLIST_CONFIG_DEP_FILETYPE:
		// don't care
		break;
	case PBX_FLIST_CONFIG_DCF_ONLY:
		pFDBInitObj->bIsDCFFileOnly = param1;
		break;
	case PBX_FLIST_CONFIG_SORT_TYPE:
		gSortType = param1;
		break;
	case PBX_FLIST_CONFIG_ROOT_PATH:
		if (param1) {
			strncpy(gRootPath, (CHAR *)param1, PBX_FLIST_NAME_MAX_LENG);
			DBG_IND("RootPath =  %s\r\n", gRootPath);
		} else {
			DBG_ERR("param1 is NULL\r\n");
		}
		break;
	case PBX_FLIST_CONFIG_DEFAULT_FOLDER:
		if (param1) {
			strncpy(gDefaultFolder, (CHAR *)param1, PBX_FLIST_NAME_MAX_LENG);
			DBG_IND("defaulFolder =  %s\r\n", gDefaultFolder);
		} else {
			DBG_ERR("param1 is NULL\r\n");
		}
		break;

	case PBX_FLIST_CONFIG_FILTER_FOLDER:
		if (param1) {
			strncpy(gFilterFolder, (CHAR *)param1, PBX_FLIST_NAME_MAX_LENG);
			DBG_IND("defaulFolder =  %s\r\n", gFilterFolder);
		} else {
			DBG_ERR("param1 is NULL\r\n");
		}
		break;
	case PBX_FLIST_CONFIG_SUPPORT_LONGNAME:
		gbIsSupportLongName = param1;
		break;

	case PBX_FLIST_CONFIG_SORT_LARGEFIRST:
		gbIsSortLargeFirst = param1;
		break;

	case PBX_FLIST_CONFIG_RECURSIVE:
		pFDBInitObj->bIsRecursive = param1;
		break;

	case PBX_FLIST_CONFIG_SKIPDIR_FOR_NONRECURSIVE:
		pFDBInitObj->bIsSkipDirForNonRecursive = param1;
		break;

	case PBX_FLIST_CONFIG_SKIPHIDDEN:
		pFDBInitObj->bIsSkipHidden = param1;
		break;

	case PBX_FLIST_CONFIG_SORT_BYSN_DELIMSTR:
		pFDBInitObj->SortSN_Delim = (CHAR *)param1;
		break;

	case PBX_FLIST_CONFIG_SORT_BYSN_DELIMNUM:
		pFDBInitObj->SortSN_DelimCount = param1;
		break;

	case PBX_FLIST_CONFIG_SORT_BYSN_NUMOFSN:
		pFDBInitObj->SortSN_CharNumOfSN = param1;
		break;

	default:
		DBG_ERR("Not Support ConfigID = %d\r\n", ConfigID);
		break;

	}
	return E_OK;
}

ER PBXFList_FDB_Init(void)
{
	PFILEDB_INIT_OBJ   pFDBInitObj = &gPBXFDBInitObj;

	if (gRootPath[0]) {
		pFDBInitObj->rootPath = gRootPath;
	}
	if (gDefaultFolder[0]) {
		pFDBInitObj->defaultfolder = gDefaultFolder;
	}
	if (gFilterFolder[0]) {
		pFDBInitObj->filterfolder = gFilterFolder;
	}
	pFDBInitObj->bIsSupportLongName = gbIsSupportLongName;
	if (gPBXFDBHdl != FILEDB_CREATE_ERROR) {
		DBG_ERR("FileDBHdl is used, please uninit firstly\r\n");
	} else {
		Perf_Mark();
		gPBXFDBHdl = FileDB_Create(pFDBInitObj);
		DBG_IND("createTime = %04d ms \r\n", Perf_GetDuration() / 1000);
		Perf_Mark();
		if (FileDB_GetTotalFileNum(gPBXFDBHdl)) {
			FileDB_SortBy(gPBXFDBHdl, xPBXSortType2FDBSortType(gSortType), gbIsSortLargeFirst);
		}
		DBG_IND("sortTime = %04d ms \r\n", Perf_GetDuration() / 1000);
	}
#if DUMP_INFO
FileDB_DumpInfo(gPBXFDBHdl);
#endif
return E_OK;
}

	ER PBXFList_FDB_UnInit(void)
	{
		FileDB_Release(gPBXFDBHdl);
		gPBXFDBHdl = FILEDB_CREATE_ERROR;
		return E_OK;
	}


	ER PBXFList_FDB_GetInfo(UINT32  InfoID, VOID *pparam1, VOID *pparam2)
	{
		FILEDB_HANDLE     FileDBHdl = gPBXFDBHdl;
		PFILEDB_FILE_ATTR FileAttr = NULL;

		if (!pparam1) {
			DBG_ERR("pparam1 is NULL \r\n");
			return E_PAR;
		}
		DBG_IND("InfoID = 0x%x, ", InfoID);
		switch (InfoID) {
		case PBX_FLIST_GETINFO_FILESEQ:
			*(UINT32 *)pparam1 = (FileDB_GetCurrFileIndex(FileDBHdl) + 1);
			DBG_MSG("index = %d \r\n", *(UINT32 *)pparam1);
			break;
		case PBX_FLIST_GETINFO_FILENUMS:
			*(UINT32 *)pparam1 = FileDB_GetTotalFileNum(FileDBHdl);
			DBG_MSG("TolFileNum = %d \r\n", *(UINT32 *)pparam1);
			break;
		case PBX_FLIST_GETINFO_READONLY:
			FileAttr = FileDB_CurrFile(FileDBHdl);
			if (!FileAttr) {
				goto FDB_GetInfo_err;
			}
			*(UINT32 *)pparam1 = M_IsReadOnly(FileAttr->attrib);
			DBG_MSG("isReadOnly = %d \r\n", *(UINT32 *)pparam1);
			break;
		case PBX_FLIST_GETINFO_FILESIZE64:
			FileAttr = FileDB_CurrFile(FileDBHdl);
			if (!FileAttr) {
				goto FDB_GetInfo_err;
			}
			*(UINT64 *)pparam1 = FileAttr->fileSize64;
			DBG_MSG("fileSize = %lld \r\n", *(UINT64 *)pparam1);
			break;
		case PBX_FLIST_GETINFO_FILETYPE:
			FileAttr = FileDB_CurrFile(FileDBHdl);
			if (!FileAttr) {
				goto FDB_GetInfo_err;
			}
			*(UINT32 *)pparam1 = xFDBFileType2PBXFileType((UINT32)FileAttr->fileType);
			DBG_MSG("fileType = 0x%x \r\n", *(UINT32 *)pparam1);
			break;
		case PBX_FLIST_GETINFO_FILEPATH:
			FileAttr = FileDB_CurrFile(FileDBHdl);
			if (!FileAttr) {
				goto FDB_GetInfo_err;
			}
			strncpy((CHAR *)pparam1, FileAttr->filePath, PBX_FLIST_NAME_MAX_LENG);
			DBG_MSG("filePath = %s \r\n", (CHAR *)pparam1);
			break;

		case PBX_FLIST_GETINFO_ISWITHMEMO: {
				UINT32 uiThisFileFormat;
				FileAttr = FileDB_CurrFile(FileDBHdl);
				if (!FileAttr) {
					goto FDB_GetInfo_err;
				}
				uiThisFileFormat = FileAttr->fileType;
				if ((uiThisFileFormat & FILEDB_FMT_WAV) && (uiThisFileFormat & FILEDB_FMT_JPG)) {
					*(BOOL *)pparam1 = TRUE;
				} else {
					*(BOOL *)pparam1 = FALSE;
				}
				DBG_MSG("isWithMemo = %d \r\n", *(BOOL *)pparam1);
			}
			break;
		case PBX_FLIST_GETINFO_FILEID: {
				PFILEDB_INIT_OBJ   pFDBInitObj = &gPBXFDBInitObj;

				if (pFDBInitObj->bIsDCFFileOnly) {
					FileAttr = FileDB_CurrFile(FileDBHdl);
					if (!FileAttr) {
						goto FDB_GetInfo_err;
					}
					sscanf_s(FileAttr->filePath + DCF_FILE_ID_INDEX, "%4d", (INT32 *)pparam1);
				} else {
					*(INT32 *)pparam1 = 0;
				}
				DBG_MSG("fileID = %d \r\n", *(INT32 *)pparam1);
			}
			break;
		case PBX_FLIST_GETINFO_DIRID: {
				PFILEDB_INIT_OBJ   pFDBInitObj = &gPBXFDBInitObj;

				if (pFDBInitObj->bIsDCFFileOnly) {
					FileAttr = FileDB_CurrFile(FileDBHdl);
					if (!FileAttr) {
						goto FDB_GetInfo_err;
					}
					sscanf_s(FileAttr->filePath + DCF_FOLDER_ID_INDEX, "%3d", (INT32 *)pparam1);
				} else {
					*(INT32 *)pparam1 = 0;
				}
				DBG_MSG("DirID = %d \r\n", *(INT32 *)pparam1);
			}
			break;
		case PBX_FLIST_GETINFO_VALID_FILETYPE: {
				PFILEDB_INIT_OBJ   pFDBInitObj = &gPBXFDBInitObj;

				*(UINT32 *)pparam1 = xFDBFileType2PBXFileType(pFDBInitObj->fileFilter);
				DBG_MSG("ValidFileType = 0x%x \r\n", *(UINT32 *)pparam1);
			}
			break;
		case PBX_FLIST_GETINFO_MODDATE:
			FileAttr = FileDB_CurrFile(FileDBHdl);
			if (!FileAttr) {
				goto FDB_GetInfo_err;
			}
			*(UINT32 *)pparam1 = ((UINT32)FileAttr->lastWriteDate << 16) + FileAttr->lastWriteTime;
			DBG_MSG("fileType = 0x%x \r\n", *(UINT32 *)pparam1);
			break;
		case PBX_FLIST_GETINFO_DB_HANDLE:
			*(UINT32 *)pparam1 = FileDBHdl;
			DBG_MSG("DB_HANDLE = 0x%x \r\n", *(UINT32 *)pparam1);
			break;
		case PBX_FLIST_GETINFO_FILENUMS_INDIR:
		case PBX_FLIST_GETINFO_FILESEQ_INDIR:
		default:
			*(UINT32 *)pparam1 = 0;
			DBG_ERR("Not Support InfoID = 0x%x\r\n", InfoID);
			return E_PAR;

		}
		return E_OK;
FDB_GetInfo_err:
		*(UINT32 *)pparam1 = 0;
		DBG_ERR("InfoID = 0x%x\r\n", InfoID);
		return E_SYS;

	}

	ER PBXFList_FDB_SetInfo(UINT32  InfoID, UINT32 param1, UINT32 param2)
	{
		FILEDB_HANDLE     FileDBHdl = gPBXFDBHdl;
		PFILEDB_FILE_ATTR FileAttr = NULL;

		DBG_IND("InfoID = 0x%x, param1= %d\r\n", InfoID, param1);
		switch (InfoID) {
		case PBX_FLIST_SETINFO_CURINDEX:
			FileAttr = FileDB_SearhFile(FileDBHdl, param1);
			if (!FileAttr) {
				DBG_ERR("InfoID = 0x%x, param1= %d\r\n", InfoID, param1);
				return E_PAR;
			}
			break;
		default:
			DBG_ERR("Not Support InfoID = 0x%x\r\n", InfoID);
			return E_PAR;

		}
		return E_OK;
	}

	ER PBXFList_FDB_SeekIndex(INT32 offset, PBX_FLIST_SEEK_CMD seekCmd)
	{
		FILEDB_HANDLE     FileDBHdl = gPBXFDBHdl;
		PFILEDB_FILE_ATTR FileAttr = NULL;
		INT32             index;
		UINT32            fileCount;

		DBG_IND("offset=%d,seekCmd=%d\r\n", offset, seekCmd);

		fileCount = FileDB_GetTotalFileNum(FileDBHdl);
		if (!fileCount) {
			DBG_ERR("FileNums=0\r\n");
			return E_SYS;
		}
		if (seekCmd == PBX_FLIST_SEEK_SET) {
			index = offset - 1;
			FileAttr = FileDB_SearhFile(FileDBHdl, index);
		} else if (seekCmd == PBX_FLIST_SEEK_CUR) {
			index = (INT32)FileDB_GetCurrFileIndex(FileDBHdl) + offset;
			FileAttr = FileDB_SearhFile(FileDBHdl, index);
		} else if (seekCmd == PBX_FLIST_SEEK_END) {
			index = (INT32)FileDB_GetTotalFileNum(FileDBHdl) - 1 + offset;
			FileAttr = FileDB_SearhFile(FileDBHdl, index);
		} else {
			DBG_ERR("Not Support seekCmd = %d\r\n", seekCmd);
			return E_PAR;
		}
		DBG_IND("index=%d\r\n", index);

#if DUMP_INFO
FileDB_DumpInfo(FileDBHdl);
#endif
if (FileAttr) {
return E_OK;
} else {
DBG_ERR("offset=%d,seekCmd=%d\r\n", offset, seekCmd);
return E_PAR;
}
}

		ER PBXFList_FDB_MakeFilePath(UINT32  fileType, CHAR *path)
		{
			DBG_ERR("Not Support\r\n");
			return E_SYS;
		}

		ER PBXFList_FDB_AddFile(CHAR *path)
		{
			DBG_IND("path=%s\r\n", path);
			if (FileDB_AddFile(gPBXFDBHdl, path)) {
				return E_OK;
			} else {
				DBG_ERR("path=%s\r\n", path);
				return E_SYS;
			}
		}

#if 0
ER PBXFList_FDB_DelFile(UINT32  cmdID, UINT32 param1)
{
FILEDB_HANDLE     FileDBHdl = gPBXFDBHdl;
PFILEDB_FILE_ATTR FileAttr = NULL;
INT32             ret;
INT32             FileNum, i;

		Perf_Mark();
		DBG_IND("cmdID=%d\r\n", cmdID);
		if (cmdID > PBX_FLIST_DELETE_MAX_ID) {
			DBG_ERR("cmdID=%d\r\n", cmdID);
			return E_PAR;
		}
		if (cmdID == PBX_FLIST_DELETE_CURR) {
			FileAttr = FileDB_CurrFile(FileDBHdl);
			i =  FileDB_GetCurrFileIndex(FileDBHdl);
			if (FileAttr) {
				ret = FileSys_DeleteFile(FileAttr->filePath);
				if (ret == FST_STA_OK) {
					FileDB_DeleteFile(FileDBHdl, i);
				} else {
					goto FDB_Delete_Err;
				}
			} else {
				goto FDB_Delete_Err;
			}
		} else if (cmdID == PBX_FLIST_DELETE_ALL) {
			FileNum = FileDB_GetTotalFileNum(FileDBHdl);
			for (i = FileNum - 1; i >= 0; i--) {
				FileAttr = FileDB_SearhFile(FileDBHdl, i);
				if (FileAttr) {
					ret = FileSys_DeleteFile(FileAttr->filePath);
					if (ret == FST_STA_OK) {
						FileDB_DeleteFile(FileDBHdl, i);
					} else {
						goto FDB_Delete_Err;
					}
				} else {
					goto FDB_Delete_Err;
				}
			}
		}
		DBG_IND("DelTime = %04d ms \r\n", Perf_GetDuration() / 1000);
		return E_OK;
FDB_Delete_Err:
		DBG_ERR("cmdID=%d\r\n", cmdID);
		return E_SYS;

	}

	ER PBXFList_FDB_ProtectFile(UINT32  cmdID, UINT32 param1)
	{
		FILEDB_HANDLE     FileDBHdl = gPBXFDBHdl;
		PFILEDB_FILE_ATTR FileAttr = NULL;
		INT32             ret;
		UINT32            FileNum, i;
		BOOL              bLock;

		DBG_IND("cmdID=%d\r\n", cmdID);
		Perf_Mark();
		if (cmdID > PBX_FLIST_PROTECT_MAX_ID) {
			DBG_ERR("cmdID=%d\r\n", cmdID);
			return E_PAR;
		}
		if (cmdID == PBX_FLIST_PROTECT_CURR || cmdID == PBX_FLIST_UNPROTECT_CURR) {
			if (cmdID == PBX_FLIST_PROTECT_CURR) {
				bLock = TRUE;
			} else {
				bLock = FALSE;
			}
			FileAttr = FileDB_CurrFile(FileDBHdl);
			i =  FileDB_GetCurrFileIndex(FileDBHdl);
			if (FileAttr) {
				ret = FileSys_SetAttrib(FileAttr->filePath, FST_ATTRIB_READONLY, bLock);
				if (ret == FST_STA_OK) {
					if (bLock) {
						FileAttr->attrib |= FS_ATTRIB_READ;
					} else {
						FileAttr->attrib &= (~FS_ATTRIB_READ);
					}
				} else {
					goto FDB_Protect_Err;
				}
			} else {
				goto FDB_Protect_Err;
			}
		} else if (cmdID == PBX_FLIST_PROTECT_ALL || cmdID == PBX_FLIST_UNPROTECT_ALL) {
			if (cmdID == PBX_FLIST_PROTECT_ALL) {
				bLock = TRUE;
			} else {
				bLock = FALSE;
			}
			FileNum = FileDB_GetTotalFileNum(FileDBHdl);
			for (i = 0; i < FileNum; i++) {
				FileAttr = FileDB_SearhFile(FileDBHdl, i);
				if (FileAttr) {
					ret = FileSys_SetAttrib(FileAttr->filePath, FST_ATTRIB_READONLY, bLock);
					if (ret == FST_STA_OK) {
						if (bLock) {
							FileAttr->attrib |= FS_ATTRIB_READ;
						} else {
							FileAttr->attrib &= (~FS_ATTRIB_READ);
						}
					} else {
						goto FDB_Protect_Err;
					}
				} else {
					goto FDB_Protect_Err;
				}
			}
		}
		DBG_IND("ProtectTime = %04d ms \r\n", Perf_GetDuration() / 1000);
		return E_OK;
FDB_Protect_Err:
		DBG_ERR("cmdID=%d\r\n", cmdID);
		return E_OK;
	}
#endif

	ER PBXFList_FDB_Refresh(void)
	{
		DBG_IND("\r\n");
		FileDB_Refresh(gPBXFDBHdl);
		return E_OK;
	}



