/**
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       PBXFile.c
    @ingroup    mISYSPlaySS

    @brief      All function for Playback file handle
                This module can be a plug-in to Application Playback

    @note       Nothing.

    @version    V0.00.001
    @author     Lincy Lin
    @date       2012/3/20
*/

/** \addtogroup mISYSPlaySS */
//@{

#include <kwrap/error_no.h>
#include "FileSysTsk.h"
#include "PBXFile.h"
#if _TODO
#include "DCF.h"
#endif
//#include "Debug.h"

#if defined __UITRON || defined __ECOS || defined __FREERTOS
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#endif

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          pbxfile
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int pbxfile_debug_level = NVT_DBG_WRN;

BOOL PBXFile_WaitInit(void)
{
	return FileSys_GetParam(FST_PARM_TASK_INIT_RDY, 0);
}


#if 0
/**
     Get file status.

     @return ths status of current file
         - @b E_SYS:   File not exist or file size is 0 or file read write error
         - @b E_OK:    File operation success
*/
ER  PBX_File_GetStatus(void)
{
	HNVT_FILE *pFile = NULL;
	//pFile = (HNVT_FILE *)FileSys_GetParam(FST_PARM_CURR_FILE, 0);
	if (pFile && pFile->fileSize == 0) {
		// 0 KB file Error
		return E_SYS;
	} else {
		return FileSys_GetParam(FST_PARM_TASK_STS, 0);
	}

}
#endif
UINT32 PBXFile_GetInfo(PBX_FILEINFO  WhichInfo, UINT32 parm1)
{
	UINT32 ReturnValue;

	switch (WhichInfo) {
#if _TODO
	case PBX_FILEINFO_FILEID:
		ReturnValue = DCF_GetDBInfo(DCF_INFO_CUR_FILE_ID);
		break;

	case PBX_FILEINFO_FILESIZE: {
			UINT32     CurIndex, DirectoryID = 0, FilenameID = 0, uiThisFileFormat = 0;
			CHAR       filePath[DCF_FULL_FILE_PATH_LEN];

			CurIndex = DCF_GetDBInfo(DCF_INFO_CUR_INDEX);
			DCF_GetObjInfo(CurIndex, &DirectoryID, &FilenameID, &uiThisFileFormat);
			if (uiThisFileFormat & DCF_FILE_TYPE_JPG) {
				uiThisFileFormat = DCF_FILE_TYPE_JPG;
			}
			DCF_GetObjPath(CurIndex, uiThisFileFormat, filePath);
			ReturnValue = FileSys_GetFileLen(filePath);
		}
		break;

	case PBX_FILEINFO_FILEFORMAT:
		ReturnValue = DCF_GetDBInfo(DCF_INFO_CUR_FILE_TYPE);
		break;

	case PBX_FILEINFO_FILESEQ:
		ReturnValue = DCF_GetDBInfo(DCF_INFO_CUR_INDEX);
		break;

	case PBX_FILEINFO_FILENUMS:
		ReturnValue = DCF_GetDBInfo(DCF_INFO_TOL_FILE_COUNT);
		break;

	case PBX_FILEINFO_DIRNUMS:
		ReturnValue = DCF_GetDBInfo(DCF_INFO_TOL_DIR_COUNT);
		break;

	case PBX_FILEINFO_MAXDIRID:
		ReturnValue = DCF_GetDBInfo(DCF_INFO_MAX_DIR_ID);
		break;

	case PBX_FILEINFO_DIRID:
		ReturnValue = DCF_GetDBInfo(DCF_INFO_CUR_DIR_ID);
		break;

	case PBX_FILEINFO_FILENUMS_INDIR:
		ReturnValue = DCF_GetDBInfo(DCF_INFO_DIR_FILE_NUM);
		break;

	case PBX_FILEINFO_FILESEQ_INDIR:
		ReturnValue = DCF_GetDBInfo(DCF_INFO_DIR_FILE_SEQ);
		break;

	case PBX_FILEINFO_MAXFILEID_INDIR:
		ReturnValue = DCF_GetDBInfo(DCF_INFO_MAX_FILE_ID);
		break;

	case PBX_FILEINFO_ISFILEREADONLY: {

			UINT32     CurIndex, DirectoryID = 0, FilenameID = 0, uiThisFileFormat = 0;
			UINT8      attrib = 0;
			int        ret;
			CHAR       filePath[DCF_FULL_FILE_PATH_LEN];

			CurIndex = DCF_GetDBInfo(DCF_INFO_CUR_INDEX);
			DCF_GetObjInfo(CurIndex, &DirectoryID, &FilenameID, &uiThisFileFormat);
			if (uiThisFileFormat & DCF_FILE_TYPE_JPG) {
				uiThisFileFormat = DCF_FILE_TYPE_JPG;
			}
			DCF_GetObjPath(CurIndex, uiThisFileFormat, filePath);
			ret = FileSys_GetAttrib(filePath, &attrib);
			if ((ret == E_OK) && M_IsReadOnly(attrib) == TRUE) {
				ReturnValue = TRUE;
			} else {
				ReturnValue = FALSE;
			}

		}
		break;

	case PBX_FILEINFO_ISWITHMENO: {
			UINT32 uiThisFileFormat;
			uiThisFileFormat = DCF_GetDBInfo(DCF_INFO_CUR_FILE_TYPE);
			if ((uiThisFileFormat & DCF_FILE_TYPE_WAV) && (uiThisFileFormat & DCF_FILE_TYPE_JPG)) {
				ReturnValue = TRUE;
			} else {
				ReturnValue = FALSE;
			}
		}
		break;
#endif
	default:
		DBG_ERR("%d\r\n", WhichInfo);
		ReturnValue = FALSE;
		break;
	}
	return (ReturnValue);
}

ER PBXFile_GetTime(char *szFileName, UINT32  creDateTime[6], UINT32  modDateTime[6])
{
	return FileSys_GetDateTime(szFileName, creDateTime, modDateTime);
}

ER PBXFile_SetTime(char *szFileName, UINT32  creDateTime[6], UINT32  modDateTime[6])
{
	UINT32 ReturnValue;
	ReturnValue = FileSys_SetDateTime(szFileName, creDateTime, modDateTime);
	return ReturnValue;
}

ER PBXFile_Access64(PPBXFile_Access_Info64 pFileAccess64)
{
	INT32 FSRet;
	FST_FILE filehdl = NULL;

	if (NULL == pFileAccess64) {
		DBG_ERR("pFileAccess64 is NULL\r\n");
		return E_PAR;
	}

	if (pFileAccess64->fileCmd == PBX_FILE_CMD_READ) {
		filehdl = FileSys_OpenFile((char *)pFileAccess64->fileName, FST_OPEN_READ);
		if (NULL == filehdl) {
			FSRet = FST_STA_ERROR;
			goto L_PBXFile_Access64_END;
		}

		FSRet = FileSys_SeekFile(filehdl, pFileAccess64->filePos, FST_SEEK_SET);
		if (FST_STA_OK != FSRet) {
			goto L_PBXFile_Access64_END;
		}

		FSRet = FileSys_ReadFile(filehdl, (UINT8 *)pFileAccess64->pBuf, &pFileAccess64->bufSize, 0, NULL);
		if (FST_STA_OK != FSRet) {
			goto L_PBXFile_Access64_END;
		}
	} else if (pFileAccess64->fileCmd == PBX_FILE_CMD_WRITE) {
		filehdl = FileSys_OpenFile((char *)pFileAccess64->fileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (NULL == filehdl) {
			FSRet = FST_STA_ERROR;
			goto L_PBXFile_Access64_END;
		}

		FSRet = FileSys_SeekFile(filehdl, pFileAccess64->filePos, FST_SEEK_SET);
		if (FST_STA_OK != FSRet) {
			goto L_PBXFile_Access64_END;
		}

		FSRet = FileSys_WriteFile(filehdl, (UINT8 *)pFileAccess64->pBuf, &pFileAccess64->bufSize, 0, NULL);
		if (FST_STA_OK != FSRet) {
			goto L_PBXFile_Access64_END;
		}
	} else if (pFileAccess64->fileCmd == PBX_FILE_CMD_UPDATE) {
		filehdl = FileSys_OpenFile((char *)pFileAccess64->fileName, FST_OPEN_ALWAYS | FST_OPEN_WRITE);
		if (NULL == filehdl) {
			FSRet = FST_STA_ERROR;
			goto L_PBXFile_Access64_END;
		}

		FSRet = FileSys_SeekFile(filehdl, pFileAccess64->filePos, FST_SEEK_SET);
		if (FST_STA_OK != FSRet) {
			goto L_PBXFile_Access64_END;
		}

		FSRet = FileSys_WriteFile(filehdl, (UINT8 *)pFileAccess64->pBuf, &pFileAccess64->bufSize, 0, NULL);
		if (FST_STA_OK != FSRet) {
			goto L_PBXFile_Access64_END;
		}
	} else {
		DBG_ERR("cmd=%d\r\n", pFileAccess64->fileCmd);
		FSRet = FST_STA_ERROR;
	}

L_PBXFile_Access64_END:
	if (NULL != filehdl) {
		FileSys_CloseFile(filehdl);
	}

	if (FSRet != FST_STA_OK) {
		DBG_ERR("Access failed\r\n");
		return E_PAR;
	}

	return E_OK;
}

UINT64 PBXFile_GetFreeSpace(void)
{
	return FileSys_GetDiskInfo(FST_INFO_FREE_SPACE);
}



