/*
    This file is the LogFile naming rule.

    @file       LogFileNaming.c
    @date       2016/02/03

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "kwrap/error_no.h"
#include "FileSysTsk.h"
#include "LogFileNaming.h"
#include "LogFile.h"
#include <time.h>
#include <comm/hwclock.h>

#define __MODULE__          LogFileNameing
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
extern unsigned int LogFileNameing_debug_level;

LOGFILE_NAME_OBJ  gLogNameObj[LOGFILE_NAME_TYPE_MAX];

LOGFILE_NAME_OBJ *LogFileNaming_GetObj(LOGFILE_NAME_TYPE name_type)
{
	if (name_type >= LOGFILE_NAME_TYPE_MAX)
		return NULL;
	return &gLogNameObj[name_type];
}



/**
  Comparing two filename if name1>name2 then return TRUE.

  @param char* name1: the name1 need to be compared
  @param char* name2: the name2 need to be compared
  @return return TRUE if name1>name2, else return FALSE
*/
static BOOL LogFileNaming_CompareName(const char *name1, const char *name2)
{
	UINT32 i = 0;
	CHAR   c1, c2;

	while (i <= MAX_FILE_NAME_LEN) {
		c1 = name1[i];
		c2 = name2[i];
		if (c1 >= 'a' && c1 <= 'z') {
			c1 -= 0x20;
		}
		if (c2 >= 'a' && c2 <= 'z') {
			c2 -= 0x20;
		}
		if (c1 == 0 && c2 == 0) {
			return FALSE;
		}
		if (c1 > c2) {
			return TRUE;
		} else if (c1 < c2) {
			return FALSE;
		}
		i++;
	}
	return FALSE;
}



/**
  get a free entry from the file list, if all entries are used, it will return the oldest file entry.
*/
static CHAR *LogFileNaming_GetFreeEntry(LOGFILE_NAME_OBJ *pObj)
{
	UINT32 i;
	CHAR  *curOldest = NULL;

	if (pObj->MaxFileCount > LOGFILE_MAX_FILENUM) {
		DBG_ERR("gMaxFileCount %d > limit %d\r\n", pObj->MaxFileCount, LOGFILE_MAX_FILENUM);
		return NULL;
	}
	// find a emtpy entry
	for (i = 0; i < pObj->MaxFileCount; i++) {
		if (pObj->FileNameTbl[i][0] == 0) {
			return pObj->FileNameTbl[i];
		}
	}
	// find oldest file
	curOldest = pObj->FileNameTbl[0];
	for (i = 1; i < pObj->MaxFileCount; i++) {
		// smallest name means oldest
		if (LogFileNaming_CompareName(curOldest, pObj->FileNameTbl[i]) == TRUE) {
			curOldest = pObj->FileNameTbl[i];
		}
	}
	return curOldest;
}


static void LogFileNaming_FileSysDirCB(FIND_DATA *findDir, BOOL *bContinue, UINT16 *cLongname, UINT32 Param)
{
	CHAR             *filepath;
	LOGFILE_NAME_OBJ *pObj = (LOGFILE_NAME_OBJ *)Param;

	if (0 == strncmp(findDir->filename, ".", 1) || 0 == strncmp(findDir->filename, "..", 2)) {
		return;
	}
	if (M_IsVolumeLabel(findDir->attrib) || M_IsDirectory(findDir->attrib)) {
		return;
	}
	if (pObj->FileIdx >= pObj->MaxFileCount) {
		DBG_WRN("Exceeds file limit %d\r\n", pObj->MaxFileCount);
		*bContinue = FALSE;
		return;
	}
	filepath = pObj->FileNameTbl[pObj->FileIdx];
	if (cLongname[0] != 0) {
		UINT32 dsti, srci, len = 0, isNormalFinish = FALSE;

		//len = snprintf(filepath,MAX_FILE_NAME_LEN+1,"%s",gPathRoot);
		srci = 0;
		for (dsti = len; dsti < MAX_FILE_NAME_LEN; dsti++) {
			if (cLongname[srci] == 0) {
				isNormalFinish = TRUE;
				filepath[dsti] = 0;
				break;
			}
			// ascii value
			if (cLongname[srci] < 128) {
				filepath[dsti] = (CHAR)cLongname[srci];
				srci++;
			}
			// has non-ascii value, need to use short name instead
			else {
				break;
			}
		}
		if (!isNormalFinish) {
			strncpy(filepath, findDir->filename, MAX_FILE_NAME_LEN);
		}
	} else {
		strncpy(filepath, findDir->filename, MAX_FILE_NAME_LEN);
	}
	DBG_IND("file %s\r\n", filepath);
	pObj->FileIdx++;
}

// return value is current file count
UINT32 LogFileNaming_Init(LOGFILE_NAME_OBJ *pObj, UINT32 MaxFileCount, CHAR *rootDir)
{
	UINT32 i;
	ER     Error_check;

	if (MaxFileCount > LOGFILE_MAX_FILENUM) {
		pObj->MaxFileCount = LOGFILE_MAX_FILENUM;
		DBG_WRN("MaxFileCount > limit %d\r\n", LOGFILE_MAX_FILENUM);
	} else {
		pObj->MaxFileCount = MaxFileCount;
	}
	if (rootDir && rootDir[0]) {
		snprintf(pObj->PathRoot, sizeof(pObj->PathRoot), "%s", rootDir);
	}
	DBG_IND("MaxFileCount %d, rootDir %s\r\n", MaxFileCount, rootDir);
	// clear variable
	pObj->FileIdx = 0;
	for (i = 0; i < pObj->MaxFileCount; i++) {
		pObj->FileNameTbl[i][0] = 0;
	}
	// scan folder
	Error_check = FileSys_ScanDir(pObj->PathRoot, LogFileNaming_FileSysDirCB, TRUE, (UINT32)pObj);
	if (Error_check != E_OK) {
		DBG_ERR("Filesys Error\r\n");
	}
	DBG_IND("FileIdx %d\r\n", pObj->FileIdx);
	return pObj->FileIdx;
}

INT32 LogFileNaming_PreAllocFiles(LOGFILE_NAME_OBJ *pObj, UINT32 MaxFileCount, UINT32 maxFileSize)
{
	UINT32    i;
	INT32     result;
	CHAR      filename[LOGFILE_ROOT_DIR_MAX_LEN + MAX_FILE_NAME_LEN + 1];
	FST_FILE  filehdl = NULL;

	for (i = 0; i < MaxFileCount; i++) {
		snprintf(pObj->FileNameTbl[i], sizeof(pObj->FileNameTbl[i]), "%04d_dummy.log", (int)i);
		snprintf(filename, sizeof(filename), "%s%s", pObj->PathRoot, pObj->FileNameTbl[i]);

		filehdl = FileSys_OpenFile(filename, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (!filehdl) {
			DBG_ERR("Open file %s fail\r\n", filename);
			return -1;
		}
		DBG_IND("Alloc file %s, size %d\r\n", filename, maxFileSize);
		result = FileSys_AllocFile(filehdl, maxFileSize, 0, 0);
		if (result != FST_STA_OK) {
			DBG_ERR("Alloc size %d fail, %s, need to stop\r\n", maxFileSize, filename);
			FileSys_CloseFile(filehdl);
			return -1;
		}
		FileSys_CloseFile(filehdl);
		pObj->FileIdx++;
	}
	return 0;
}


BOOL LogFileNaming_GetNewPath(LOGFILE_NAME_OBJ *pObj, CHAR *path, UINT32 pathlen)
{
	struct tm   CurDateTime;
	CHAR    *pEntry = NULL;
	CHAR     OldFilePath[LOGFILE_ROOT_DIR_MAX_LEN + MAX_FILE_NAME_LEN + 1];
	BOOL     isFileExist = FALSE;

	if (path == NULL) {
		DBG_ERR("path is NULL\r\n");
		return FALSE;
	}
	CurDateTime = hwclock_get_time(TIME_ID_CURRENT);
	pEntry = LogFileNaming_GetFreeEntry(pObj);
	if (pEntry == NULL) {
		DBG_ERR("Can't get FreeEntry\r\n");
		path[0] = 0;
		return FALSE;
	}
	// file exist
	if (pEntry[0] != 0) {
		isFileExist = TRUE;
		// get oldest file path
		snprintf(OldFilePath, sizeof(OldFilePath), "%s%s", pObj->PathRoot, pEntry);
	}
	snprintf(pEntry, MAX_FILE_NAME_LEN + 1, "%04d%02d%02d_%02d%02d%02d.log", CurDateTime.tm_year, CurDateTime.tm_mon, CurDateTime.tm_mday, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
	snprintf(path, pathlen, "%s%s", pObj->PathRoot, pEntry);

	// file exist , need to delete it or rename it
	if (isFileExist) {

#if 0
		if (FST_STA_OK != FileSys_DeleteFile(OldFilePath)) {
			DBG_ERR("FileSys_DeleteFile %s\r\n", OldFilePath);
		}
		DBG_IND("delete oldest %s\r\n", OldFilePath);
#else
		if (FST_STA_OK != FileSys_RenameFile(pEntry, OldFilePath, TRUE)) {
			DBG_ERR("FileSys_RenameFile %s -> %s\r\n", OldFilePath, path);
		}
		DBG_IND("Rename %s -> %s\r\n", OldFilePath, path);
#endif
	}
	DBG_IND("path %s\n", path);
	return TRUE;
}


