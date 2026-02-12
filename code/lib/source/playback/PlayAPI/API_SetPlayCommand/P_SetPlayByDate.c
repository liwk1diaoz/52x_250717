#include "kwrap/stdio.h"
#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "FileSysTsk.h"
#include "FileDB.h"

FILEDB_HANDLE bkpFileHandle = FILEDB_CREATE_ERROR;
UINT16 SetSeqNum = 0;
BOOL   isEnableThumbWithFolder = FALSE;
extern UINT16 SearchMonthIndex;
extern UINT16 SearchYearIndex;
extern UINT16 TotalDays;
extern UINT16 TotalMonths;
extern UINT16 TotalYears;

static UINT32 g_uiFileDBAddr = 0;
static UINT32 g_uiFileDBMaxFileNum = 0;


/*
    Init working buffer for FileDB

    @param[in] u32MemAddr the begining address of FileDB
    @param[in] u32MaxFileNum the max file number of FileDB
    @return NeededBuffer, the needed buffer size which the max file number is u32MaxFileNum

    Example:
@code
{
    UINT32 NeedBufSize;
    NeedBufSize = PB_SetBuffForFileDB(0x1500000, 60000);

}
@endcode
*/
UINT32 PB_SetBuffForFileDB(UINT32 u32MemAddr, UINT32 u32MaxFileNum)
{
	g_uiFileDBAddr = u32MemAddr;
	g_uiFileDBMaxFileNum = u32MaxFileNum;
	return FileDB_CalcBuffSize(u32MaxFileNum, 40);
}

/*
    Do play file by date initial

    @return E_SYS if failed, E_OK if the job success
*/
INT32 PB_PlayByDateInit(void)
{
	FILEDB_INIT_OBJ fileDbInitObj;
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 LastDay = 0, LastMonth = 0, LastYear = 0;
	UINT16 bkpFileId = 0, bkpDirId = 0;
	UINT16 searchFileId = 0, searchDirId = 0;
	UINT32 i;

	if (g_uiFileDBAddr == 0) {
		DBG_ERR("Buffer Need to be allocated first!!! [PB_SetBuffForFileDB]\r\n");
		return E_SYS;
	}
	if (bkpFileHandle != FILEDB_CREATE_ERROR) {
		DBG_ERR("PlayByDate has been inited!!!\r\n");
		return E_SYS;
	}
	bkpFileId = xPB_GetCurFileNameId();
	bkpDirId = xPB_GetCurFileDirId();

	DBG_IND("bkpFileId %d bkpDirId %d \r\n", bkpFileId, bkpDirId);
	DBG_IND("gMenuPlayInfo.CurFileIndex %d \r\n", gMenuPlayInfo.CurFileIndex);

	memset(&fileDbInitObj, 0x00, sizeof(fileDbInitObj));

	fileDbInitObj.rootPath = "A:\\DCIM\\";
	if (gusPlayFileFormat & PBFMT_JPG) {
		fileDbInitObj.fileFilter |= FILEDB_FMT_JPG;
	}

	if (gusPlayFileFormat & PBFMT_AVI) {
		fileDbInitObj.fileFilter |= FILEDB_FMT_AVI;
	}

	if (gusPlayFileFormat & PBFMT_MOVMJPG) {
		fileDbInitObj.fileFilter |= FILEDB_FMT_MOV;//MOV_MJPG
	}

	if (gusPlayFileFormat & PBFMT_MP4) {
		fileDbInitObj.fileFilter |= FILEDB_FMT_MP4;
	}

	if (gusPlayFileFormat & PBFMT_ASF) {
		fileDbInitObj.fileFilter |= FILEDB_FMT_ASF;
	}
	fileDbInitObj.bIsRecursive = TRUE;
	fileDbInitObj.bIsCyclic = TRUE;
	//fileDbInitObj.bIsUseDynaMem = TRUE;
	fileDbInitObj.bIsDCFFileOnly = TRUE;
	fileDbInitObj.bIsFilterOutSameDCFNumFolder = FALSE;
	fileDbInitObj.bIsFilterOutSameDCFNumFile = FALSE;
	fileDbInitObj.u32MemAddr = g_uiFileDBAddr;
	fileDbInitObj.u32MaxFileNum = g_uiFileDBMaxFileNum;
	fileDbInitObj.u32MemSize = FileDB_CalcBuffSize(g_uiFileDBMaxFileNum, 40);
	DBG_IND("FileDB supports file# = %d, MemSize = %d\r\n", g_uiFileDBMaxFileNum, fileDbInitObj.u32MemSize);
	g_PBSetting.FileHandle = FileDB_Create(&fileDbInitObj);

	if (g_PBSetting.FileHandle == FILEDB_CREATE_ERROR) {
		DBG_ERR("File DB init error\r\n");
		return E_SYS;
	} else {
		bkpFileHandle = g_PBSetting.FileHandle;
		g_PBSetting.EnableFlags |= (PB_ENABLE_SEARCH_FILE_WITHOUT_DCF | PB_ENABLE_PLAY_FILE_BY_DATE);
		FileDB_SortBy(g_PBSetting.FileHandle, FILEDB_SORT_BY_MODDATE, FALSE);
	}

	if (g_PBSetting.EnableFlags & PB_ENABLE_THUMB_WITH_DIFF_FOLDER) {
		isEnableThumbWithFolder = TRUE;
		g_PBSetting.EnableFlags &= ~(PB_ENABLE_THUMB_WITH_DIFF_FOLDER);
	}


	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, 0);

	for (i = 0; i < FileDB_GetTotalFileNum(g_PBSetting.FileHandle); i++) {
		if (FileAttr->lastWriteDate != LastDay) {
			TotalDays++;
			LastDay = FileAttr->lastWriteDate;
		}

		if ((PB_GET_YEAR_FROM_DATE(FileAttr->lastWriteDate) != PB_GET_YEAR_FROM_DATE(LastMonth)) || (PB_GET_MONTH_FROM_DATE(FileAttr->lastWriteDate) != PB_GET_MONTH_FROM_DATE(LastMonth))) {
			TotalMonths++;
			LastMonth = FileAttr->lastWriteDate;
		}

		if (PB_GET_YEAR_FROM_DATE(FileAttr->lastWriteDate) != PB_GET_YEAR_FROM_DATE(LastYear)) {
			TotalYears++;
			LastYear = FileAttr->lastWriteDate;
		}


		sscanf_s(FileAttr->filePath + 8, "%3d", (INT32 *) & (searchDirId));
		sscanf_s(FileAttr->filePath + 21, "%4d", (INT32 *) & (searchFileId));
		if ((searchDirId == bkpDirId) && (searchFileId == bkpFileId)) {
			SetSeqNum = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);
			DBG_IND("SetSeqNum = %d \r\n", SetSeqNum);
		}

		FileAttr = FileDB_NextFile(g_PBSetting.FileHandle);
	}
	//update playback status
	if (TotalDays) {
		gusCurrPlayStatus = PB_STA_UNKNOWN;
	} else {
		gusCurrPlayStatus = PB_STA_NOIMAGE;
	}

	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, SetSeqNum);
	//init current date info
	PB_SetPlayDate(PLAY_FILE_BY_DAY, FileAttr->lastWriteDate, PLAYDATE_INDEX_TO_CURR);

	return E_OK;
}

/*
    Do play file by date uninitial.

    @return void
*/
void PB_PlayByDateUnInit(void)
{
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 tmpFileName, tmpFolderName;

	if (bkpFileHandle != FILEDB_CREATE_ERROR) {
		if (isEnableThumbWithFolder == TRUE) {
			g_PBSetting.EnableFlags |= PB_ENABLE_THUMB_WITH_DIFF_FOLDER;
			isEnableThumbWithFolder = FALSE;
		}

		DBG_IND("gMenuPlayInfo.CurFileIndex=%d \r\n", gMenuPlayInfo.CurFileIndex);
		DBG_IND("gMenuPlayInfo.DispFileSeq[ gMenuPlayInfo.CurFileIndex -1]=%d \r\n", gMenuPlayInfo.DispFileSeq[ gMenuPlayInfo.CurFileIndex - 1]);
		DBG_IND("gMenuPlayInfo.uiFirstFileIndex=%d \r\n", gMenuPlayInfo.uiFirstFileIndex);
		FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, gMenuPlayInfo.DispFileSeq[ gMenuPlayInfo.CurFileIndex - 1] + gMenuPlayInfo.uiFirstFileIndex - 1);
		if (NULL == FileAttr) {
			DBG_ERR("FileAttr NG@PB_PlayByDateUnInit!\r\n");
		} else {
			sscanf_s(FileAttr->filePath + 8, "%3d", (INT32 *) & (tmpFolderName));
			sscanf_s(FileAttr->filePath + 21, "%4d", (INT32 *) & (tmpFileName));
			gMenuPlayInfo.DispDirName[0] = tmpFolderName;
			gMenuPlayInfo.DispFileName[0] = tmpFileName;
			gMenuPlayInfo.CurFileIndex = 1;
			DBG_IND("gMenuPlayInfo.DispDirName[0] %d \r\n", gMenuPlayInfo.DispDirName[0]);
			DBG_IND("gMenuPlayInfo.DispFileName[0] %d \r\n", gMenuPlayInfo.DispFileName[0]);

			g_PBSetting.EnableFlags &= ~(PB_ENABLE_SEARCH_FILE_WITHOUT_DCF);
			FileDB_Release(g_PBSetting.FileHandle);
			g_PBSetting.FileHandle = FILEDB_CREATE_ERROR;

			bkpFileHandle = FILEDB_CREATE_ERROR;
			SearchMonthIndex = 0;
			SearchYearIndex = 0;
			TotalDays = 0;
			TotalMonths = 0;
			TotalYears = 0;
			SetSeqNum = 0;
			g_PBSetting.EnableFlags &= ~(PB_ENABLE_PLAY_FILE_BY_DATE);
			gMenuPlayInfo.PlayDateType = 0;
			gMenuPlayInfo.uiPlayDate = 0;
			gMenuPlayInfo.uiFirstFileIndex = 0;
			gMenuPlayInfo.uiTotalFilesInSpecDate = 0;
		}
	}
}

/*
    Set play date for play file by date uninitial

    @param[in] DateType
    @param[in] Date
    @param[in] CurrIdxTo
    @return void
*/
void PB_SetPlayDate(UINT32 DateType, UINT16 Date, PLAYDATE_INDEX CurrIdxTo)
{
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 tmpFileName, tmpFolderName, totalFiles = 0;
	UINT16 CurrFileIndex = 0;
	UINT16 OriFileIndex = 0;
	UINT32 i;
	UINT32 TotalSearchFile;
	DBG_IND("YMD=%04d-%02d-%02d\r\n", PB_GET_YEAR_FROM_DATE(Date),
			PB_GET_MONTH_FROM_DATE(Date),
			PB_GET_DAY_FROM_DATE(Date));

	OriFileIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);
	//To do ... fine tune performance
#if 0
	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, 0);

	//DBG_IND("FileAttr->filePath %s \r\n", FileAttr->filePath);

	for (i = 0; i < FileDB_GetTotalFileNum(g_PBSetting.FileHandle); i++) {
		if ((FileAttr == NULL) || (FileAttr->lastWriteDate == Date)) {
			break;
		}
		FileAttr = FileDB_NextFile(g_PBSetting.FileHandle);

	}
#else
	FileAttr = FileDB_CurrFile(g_PBSetting.FileHandle);
	if (FileAttr == NULL || FileDB_GetTotalFileNum(g_PBSetting.FileHandle) == 0) {
		DBG_ERR("Current FileAttr is NULL!\r\n");
		return;
	}
	TotalSearchFile = FileDB_GetTotalFileNum(g_PBSetting.FileHandle) - 1;
	if (Date > FileAttr->lastWriteDate) {
		for (i = 0; i < TotalSearchFile; i++) {
			FileAttr = FileDB_NextFile(g_PBSetting.FileHandle);
			if ((FileAttr == NULL) || (FileAttr->lastWriteDate == Date)) {
				break;
			}

		}
	} else { // if(Date <= FileAttr->lastWriteDate)
		//backward find target date
		for (i = 0; i < TotalSearchFile; i++) {
			if ((FileAttr == NULL) || (FileAttr->lastWriteDate == Date)) {
				break;
			}
			FileAttr = FileDB_PrevFile(g_PBSetting.FileHandle);
		}
		//find first index in target date
		for (i = 0; i < TotalSearchFile; i++) {
			if (FileDB_GetCurrFileIndex(g_PBSetting.FileHandle) == 0) {
				break;
			}
			FileAttr = FileDB_PrevFile(g_PBSetting.FileHandle);
			if ((FileAttr == NULL) || (FileAttr->lastWriteDate != Date)) {
				FileAttr = FileDB_NextFile(g_PBSetting.FileHandle);
				break;
			}

		}
	}
#endif

	if (FileAttr != NULL && FileAttr->lastWriteDate == Date) {
		//DBG_IND("FileAttr->filePath %s \r\n", FileAttr->filePath);
		g_PBSetting.EnableFlags |= PB_ENABLE_PLAY_FILE_BY_DATE;
		gMenuPlayInfo.PlayDateType = DateType;
		gMenuPlayInfo.uiPlayDate = Date;
		gMenuPlayInfo.uiFirstFileIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);

		//DBG_IND("OriFileIndex=%d, gMenuPlayInfo.uiFirstFileIndex %d, bCurrIdxTo1st=%d\r\n",OriFileIndex, gMenuPlayInfo.uiFirstFileIndex, bCurrIdxTo1st);

		do {
			//DBG_IND("\r\n FileAttr->filePath %s \r\n", FileAttr->filePath);
			totalFiles++;
			FileAttr = FileDB_NextFile(g_PBSetting.FileHandle);
			CurrFileIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);
		} while (FileAttr != NULL && FileAttr->lastWriteDate == Date && CurrFileIndex > gMenuPlayInfo.uiFirstFileIndex);

		gMenuPlayInfo.uiTotalFilesInSpecDate = totalFiles;
		DBG_IND("gMenuPlayInfo.uiTotalFilesInSpecDate %d \r\n", gMenuPlayInfo.uiTotalFilesInSpecDate);

		if (PLAYDATE_INDEX_TO_1ST == CurrIdxTo || OriFileIndex != SetSeqNum) {
			gMenuPlayInfo.DispFileSeq[0] = 1;
			FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, gMenuPlayInfo.uiFirstFileIndex);
		} else if (PLAYDATE_INDEX_TO_LAST == CurrIdxTo) {
			gMenuPlayInfo.DispFileSeq[0] = totalFiles;
			FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, gMenuPlayInfo.uiFirstFileIndex + totalFiles - 1);
		} else { //if (PLAYDATE_INDEX_TO_CURR == CurrIdxTo)
			gMenuPlayInfo.DispFileSeq[0] = SetSeqNum - gMenuPlayInfo.uiFirstFileIndex + 1;
			FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, OriFileIndex);
		}
		gMenuPlayInfo.CurFileIndex = 1;

		if (NULL != FileAttr) {
			sscanf_s(FileAttr->filePath + 8, "%3d", (INT32 *) & (tmpFolderName));
			sscanf_s(FileAttr->filePath + 21, "%4d", (INT32 *) & (tmpFileName));
			gMenuPlayInfo.DispDirName[0] = tmpFolderName;
			gMenuPlayInfo.DispFileName[0] = tmpFileName;
		}
	} else {
		DBG_ERR("No Such date ! YMD=%04d-%02d-%02d\r\n", PB_GET_YEAR_FROM_DATE(Date),
				PB_GET_MONTH_FROM_DATE(Date),
				PB_GET_DAY_FROM_DATE(Date));
	}
}

void PB_SetPlayDateToNext(void)
{
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 tmpFileName, tmpFolderName, totalFiles = 0;
	UINT32 uiTargetIndex, uiTotalFileNum;

	if (TotalDays <= 1) {
		return;
	}

	FileAttr = FileDB_CurrFile(g_PBSetting.FileHandle);
	if (FileAttr != NULL) {
		DBG_IND("Original YMD=%04d-%02d-%02d\r\n", PB_GET_YEAR_FROM_DATE(FileAttr->lastWriteDate),
				PB_GET_MONTH_FROM_DATE(FileAttr->lastWriteDate),
				PB_GET_DAY_FROM_DATE(FileAttr->lastWriteDate));
	}

	uiTotalFileNum = FileDB_GetTotalFileNum(g_PBSetting.FileHandle);
	if (0 == uiTotalFileNum) {
		return;
	} else {
		uiTargetIndex = (gMenuPlayInfo.uiFirstFileIndex + gMenuPlayInfo.uiTotalFilesInSpecDate) % uiTotalFileNum;
	}

	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, uiTargetIndex);
	if (FileAttr != NULL) {
		DBG_IND("Current YMD=%04d-%02d-%02d\r\n", PB_GET_YEAR_FROM_DATE(FileAttr->lastWriteDate),
				PB_GET_MONTH_FROM_DATE(FileAttr->lastWriteDate),
				PB_GET_DAY_FROM_DATE(FileAttr->lastWriteDate));


		g_PBSetting.EnableFlags |= PB_ENABLE_PLAY_FILE_BY_DATE;
		gMenuPlayInfo.PlayDateType = PLAY_FILE_BY_DAY;
		gMenuPlayInfo.uiPlayDate = FileAttr->lastWriteDate;
		gMenuPlayInfo.uiFirstFileIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);

		DBG_IND("gMenuPlayInfo.uiFirstFileIndex %d\r\n", gMenuPlayInfo.uiFirstFileIndex);

		do {
			totalFiles++;
			FileAttr = FileDB_NextFile(g_PBSetting.FileHandle);
		} while (FileAttr != NULL && FileAttr->lastWriteDate == gMenuPlayInfo.uiPlayDate);

		gMenuPlayInfo.uiTotalFilesInSpecDate = totalFiles;
		DBG_IND("gMenuPlayInfo.uiTotalFilesInSpecDate %d \r\n", gMenuPlayInfo.uiTotalFilesInSpecDate);
		FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, gMenuPlayInfo.uiFirstFileIndex);
		if (NULL == FileAttr) {
			return;
		}

		gMenuPlayInfo.DispFileSeq[0] = 1;
		gMenuPlayInfo.CurFileIndex = 1;
		sscanf_s(FileAttr->filePath + 8, "%3d", (INT32 *) & (tmpFolderName));
		sscanf_s(FileAttr->filePath + 21, "%4d", (INT32 *) & (tmpFileName));
		gMenuPlayInfo.DispDirName[0] = tmpFolderName;
		gMenuPlayInfo.DispFileName[0] = tmpFileName;
	} else {
		DBG_ERR("Set To Next Date Failed!\r\n");
	}
}

void PB_SetPlayDateToPrev(void)
{
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 tmpFileName, tmpFolderName, totalFiles = 0;
	UINT32 uiTargetIndex;

	if (TotalDays <= 1) {
		return;
	}

	FileAttr = FileDB_CurrFile(g_PBSetting.FileHandle);
	if (FileAttr != NULL) {
		DBG_IND("Original YMD=%04d-%02d-%02d\r\n", PB_GET_YEAR_FROM_DATE(FileAttr->lastWriteDate),
				PB_GET_MONTH_FROM_DATE(FileAttr->lastWriteDate),
				PB_GET_DAY_FROM_DATE(FileAttr->lastWriteDate));
	}

	if (0 == gMenuPlayInfo.uiFirstFileIndex) {
		uiTargetIndex = FileDB_GetTotalFileNum(g_PBSetting.FileHandle) - 1;
	} else {
		uiTargetIndex = gMenuPlayInfo.uiFirstFileIndex - 1;
	}

	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, uiTargetIndex);
	if (FileAttr != NULL) {
		DBG_IND("Current YMD=%04d-%02d-%02d\r\n", PB_GET_YEAR_FROM_DATE(FileAttr->lastWriteDate),
				PB_GET_MONTH_FROM_DATE(FileAttr->lastWriteDate),
				PB_GET_DAY_FROM_DATE(FileAttr->lastWriteDate));

		g_PBSetting.EnableFlags |= PB_ENABLE_PLAY_FILE_BY_DATE;
		gMenuPlayInfo.PlayDateType = PLAY_FILE_BY_DAY;
		gMenuPlayInfo.uiPlayDate = FileAttr->lastWriteDate;

		do {
			totalFiles++;
			FileAttr = FileDB_PrevFile(g_PBSetting.FileHandle);
		} while (FileAttr != NULL && FileAttr->lastWriteDate == gMenuPlayInfo.uiPlayDate);

		//go back to the first index in target day
		FileAttr = FileDB_NextFile(g_PBSetting.FileHandle);
		if (NULL != FileAttr) {
			gMenuPlayInfo.uiTotalFilesInSpecDate = totalFiles;
			gMenuPlayInfo.uiFirstFileIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);
			DBG_IND("gMenuPlayInfo.uiTotalFilesInSpecDate %d \r\n", gMenuPlayInfo.uiTotalFilesInSpecDate);
			DBG_IND("gMenuPlayInfo.uiFirstFileIndex %d\r\n", gMenuPlayInfo.uiFirstFileIndex);

			gMenuPlayInfo.DispFileSeq[0] = 1;
			gMenuPlayInfo.CurFileIndex = 1;
			sscanf_s(FileAttr->filePath + 8, "%3d", (INT32 *) & (tmpFolderName));
			sscanf_s(FileAttr->filePath + 21, "%4d", (INT32 *) & (tmpFileName));
			gMenuPlayInfo.DispDirName[0] = tmpFolderName;
			gMenuPlayInfo.DispFileName[0] = tmpFileName;
		}
	} else {
		DBG_ERR("Set To Prev Date Failed!\r\n");

	}
}

