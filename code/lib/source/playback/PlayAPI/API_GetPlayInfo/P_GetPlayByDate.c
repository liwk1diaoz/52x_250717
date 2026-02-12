#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "FileDB.h"

//#NT#2007/06/25#Corey Ke -begin
//#Support play files sort by date

//#NT#2010/09/10#Ben Wang -begin
//#NT#Refine code for sort by date only
UINT16 SearchMonthIndex = 0;
UINT16 SearchYearIndex = 0;
UINT16 TotalDays = 0;
UINT16 TotalMonths = 0;
UINT16 TotalYears = 0;

/*
    Get total play day.

    @return date
*/
UINT16 PB_GetTotalPlayDay(void)
{
	return TotalDays;
}

/*
    Get total play month.

    @return date
*/
UINT16 PB_GetTotalPlayMonth(void)
{
	return TotalMonths;
}

/*
    Get total play year.

    @return date
*/
UINT16 PB_GetTotalPlayYear(void)
{
	return TotalYears;
}

/*
    Get first play day.

    @return date
*/
UINT16 PB_GetFirstPlayDay(void)
{
	PFILEDB_FILE_ATTR FileAttr;
	INT32 FileIndex = (INT32)FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);


	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, 0);
	FileDB_SearhFile(g_PBSetting.FileHandle, FileIndex);

	if (FileAttr != NULL) {
		return FileAttr->lastWriteDate;
	} else {
		return 0;
	}
}

/*
    Get last play day.

    @return date
*/
UINT16 PB_GetLastPlayDay(void)
{
	PFILEDB_FILE_ATTR FileAttr;
	INT32 LastIndex;
	INT32 FileIndex = (INT32)FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);

	LastIndex = (INT32)FileDB_GetTotalFileNum(g_PBSetting.FileHandle) - 1;

	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, LastIndex);
	FileDB_SearhFile(g_PBSetting.FileHandle, FileIndex);

	if (FileAttr != NULL) {
		return FileAttr->lastWriteDate;
	} else {
		return 0;
	}
}

/*
    Get next play day.

    @return date
*/
UINT16 PB_GetNextPlayDay(void)
{
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 LastDate;
	INT32 FileIndex = (INT32)FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);
	UINT32 i;

	FileAttr = FileDB_CurrFile(g_PBSetting.FileHandle);
	if (NULL == FileAttr) {
		DBG_ERR("FileAttr NG@PB_GetNextPlayDay!\r\n");
		return 0;
	}
	LastDate = FileAttr->lastWriteDate;

	for (i = 0; i < FileDB_GetTotalFileNum(g_PBSetting.FileHandle); i++) {
		FileAttr = FileDB_NextFile(g_PBSetting.FileHandle);
		if ((FileAttr == NULL) || (FileAttr->lastWriteDate != LastDate)) {
			break;
		}
	}

	FileDB_SearhFile(g_PBSetting.FileHandle, FileIndex);

	if (FileAttr == NULL) {
		//#NT#2010/10/08#Ben Wang -begin
		//#NT#fix bug for there is only one date
		return LastDate;
		//#NT#2010/10/08#Ben Wang -end
	} else {
		return FileAttr->lastWriteDate;
	}
}

/*
    Get prev play day.

    @return date
*/
UINT16 PB_GetPrevPlayDay(void)
{
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 LastDate;
	INT32 FileIndex = (INT32)FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);
	UINT32 i;

	FileAttr = FileDB_CurrFile(g_PBSetting.FileHandle);
	if (NULL == FileAttr) {
		DBG_ERR("FileAttr NG@PB_GetPrevPlayDay!\r\n");
		return 0;
	}
	LastDate = FileAttr->lastWriteDate;

	for (i = 0; i < FileDB_GetTotalFileNum(g_PBSetting.FileHandle); i++) {
		FileAttr = FileDB_PrevFile(g_PBSetting.FileHandle);
		if ((FileAttr == NULL) || (FileAttr->lastWriteDate != LastDate)) {
			break;
		}
	}

	FileDB_SearhFile(g_PBSetting.FileHandle, FileIndex);

	if ((FileAttr == NULL) || (FileAttr->lastWriteDate == LastDate)) {
		//#NT#2010/10/08#Ben Wang -begin
		//#NT#fix bug for there is only one date
		return LastDate;
		//#NT#2010/10/08#Ben Wang -end
	} else {
		return FileAttr->lastWriteDate;
	}
}

/*
    Get curr play day.

    @return date
*/
UINT16 PB_GetCurrPlayDay(void)
{
	PFILEDB_FILE_ATTR FileAttr;

	FileAttr = FileDB_CurrFile(g_PBSetting.FileHandle);

	if (FileAttr == NULL) {
		return 0;
	} else {
		return FileAttr->lastWriteDate;
	}
}
//#NT#2010/09/10#Ben Wang -end
/*
    Get curr play day seq.

    @return date
*/
UINT16 PB_GetCurrPlayDaySeq(void)
{
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 LastDate, SpecDate;
	UINT16 tmpDayNum = 0;
	UINT16 FileIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);

	FileAttr = FileDB_CurrFile(g_PBSetting.FileHandle);
	if (FileAttr == NULL) {
		return 0;
	}
	SpecDate = FileAttr->lastWriteDate;

	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, 0);
	if (FileAttr == NULL) {
		return 0;
	}
	LastDate = FileAttr->lastWriteDate;

	while (SpecDate != FileAttr->lastWriteDate) {
		FileAttr = FileDB_NextFile(g_PBSetting.FileHandle);
		if (FileAttr == NULL) {
			break;
		}

		if (LastDate != FileAttr->lastWriteDate) {
			tmpDayNum++;
			LastDate = FileAttr->lastWriteDate;
		}
	}
	tmpDayNum++;
	FileDB_SearhFile(g_PBSetting.FileHandle, FileIndex);

	return tmpDayNum;
}

/*
    Get curr play year seq.

    @return date
*/
UINT16 PB_GetCurrPlayYearSeq(void)
{
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 LastDate, SpecDate;
	UINT16 tmpYearNum = 0;
	UINT16 FileIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);

	FileAttr = FileDB_CurrFile(g_PBSetting.FileHandle);
	if (NULL == FileAttr) {
		DBG_ERR("FileAttr NG (3)!\r\n");
		return 0;
	}
	SpecDate = FileAttr->lastWriteDate;

	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, 0);
	if (NULL == FileAttr) {
		DBG_ERR("FileAttr NG (4)!\r\n");
		return 0;
	}
	LastDate = FileAttr->lastWriteDate;

	while (PB_GET_YEAR_FROM_DATE(SpecDate) != PB_GET_YEAR_FROM_DATE(FileAttr->lastWriteDate)) {
		FileAttr = FileDB_NextFile(g_PBSetting.FileHandle);
		if (NULL == FileAttr) {
			break;
		}

		if (PB_GET_YEAR_FROM_DATE(LastDate) != PB_GET_YEAR_FROM_DATE(FileAttr->lastWriteDate)) {
			tmpYearNum++;
			LastDate = FileAttr->lastWriteDate;
		}
	}
	tmpYearNum++;
	FileDB_SearhFile(g_PBSetting.FileHandle, FileIndex);

	return tmpYearNum;
}

/*
    Get curr play day seq in this year.

    @return date
*/
UINT16 PB_GetCurrPlayDaySeqInThisYear(void)
{
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 LastDate, SpecDate;
	UINT16 tmpDayNum = 0;
	UINT16 FileIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);

	FileAttr = FileDB_CurrFile(g_PBSetting.FileHandle);
	if (NULL == FileAttr) {
		DBG_ERR("FileAttr NG (1)!\r\n");
		return 0;
	}
	SpecDate = FileAttr->lastWriteDate;

	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, 0);
	if (NULL == FileAttr) {
		DBG_ERR("FileAttr NG (2)!\r\n");
		return 0;
	}

	while (PB_GET_YEAR_FROM_DATE(FileAttr->lastWriteDate) != PB_GET_YEAR_FROM_DATE(SpecDate)) {
		FileAttr = FileDB_NextFile(g_PBSetting.FileHandle);
		if (NULL == FileAttr) {
			break;
		}
	}
	if (NULL != FileAttr) {
		LastDate = FileAttr->lastWriteDate;

		while (SpecDate != FileAttr->lastWriteDate) {
			FileAttr = FileDB_NextFile(g_PBSetting.FileHandle);
			if (NULL == FileAttr) {
				break;
			}

			if (LastDate != FileAttr->lastWriteDate) {
				tmpDayNum++;
				LastDate = FileAttr->lastWriteDate;
			}
		}
	}

	tmpDayNum++;
	FileDB_SearhFile(g_PBSetting.FileHandle, FileIndex);

	return tmpDayNum;
}

/*
    Get play days number in spec year.

    @return date
*/
UINT16 PB_GetPlayDaysNuminYear(UINT16 SpecYear)
{
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 LastDate = 0;
	UINT16 tmpDayNum = 0;
	UINT16 FileIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);

	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, 0);

	while ((FileAttr != NULL) && (PB_GET_YEAR_FROM_DATE(FileAttr->lastWriteDate) != PB_GET_YEAR_FROM_DATE(SpecYear))) {
		FileAttr = FileDB_NextFile(g_PBSetting.FileHandle);
	}

	if (FileAttr != NULL) {
		do {
			if (LastDate != FileAttr->lastWriteDate) {
				tmpDayNum++;
				LastDate = FileAttr->lastWriteDate;
			}
			FileAttr = FileDB_NextFile(g_PBSetting.FileHandle);
		} while ((FileAttr != NULL) && (PB_GET_YEAR_FROM_DATE(SpecYear) == PB_GET_YEAR_FROM_DATE(FileAttr->lastWriteDate)));
	}

	FileDB_SearhFile(g_PBSetting.FileHandle, FileIndex);

	return tmpDayNum;
}


/*
    Get first play month.

    @return date
*/
UINT16 PB_GetFirstPlayMonth(void)
{
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 FileIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);

	SearchMonthIndex = 0;

	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, SearchMonthIndex);
	FileDB_SearhFile(g_PBSetting.FileHandle, FileIndex);

	if (FileAttr != NULL) {
		return FileAttr->lastWriteDate;
	} else {
		return 0;
	}
}

/*
    Get next play month.

    @return date
*/
UINT16 PB_GetNextPlayMonth(void)
{
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 LastDate;
	UINT16 FileIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);

	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, SearchMonthIndex);
	if (NULL == FileAttr) {
		DBG_ERR("FileAttr NG@PB_GetNextPlayMonth!\r\n");
		return 0;
	}
	LastDate = FileAttr->lastWriteDate;

	do {
		FileAttr = FileDB_NextFile(g_PBSetting.FileHandle);

	} while ((FileAttr != NULL) && ((PB_GET_YEAR_FROM_DATE(FileAttr->lastWriteDate) == PB_GET_YEAR_FROM_DATE(LastDate)) && (PB_GET_MONTH_FROM_DATE(FileAttr->lastWriteDate) == PB_GET_MONTH_FROM_DATE(LastDate))));

	SearchMonthIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);
	FileDB_SearhFile(g_PBSetting.FileHandle, FileIndex);

	if ((FileAttr == NULL) || (FileAttr->lastWriteDate == LastDate)) {
		return 0;
	} else {
		return FileAttr->lastWriteDate;
	}
}

/*
    Get prev play month.

    @return date
*/
UINT16 PB_GetPrevPlayMonth(void)
{
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 LastDate;
	UINT16 FileIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);

	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, SearchMonthIndex);
	if (NULL == FileAttr) {
		DBG_ERR("FileAttr NG@PB_GetPrevPlayMonth!\r\n");
		return 0;
	}
	LastDate = FileAttr->lastWriteDate;

	do {
		FileAttr = FileDB_PrevFile(g_PBSetting.FileHandle);

	} while ((FileAttr != NULL) && ((PB_GET_YEAR_FROM_DATE(FileAttr->lastWriteDate) == PB_GET_YEAR_FROM_DATE(LastDate)) && (PB_GET_MONTH_FROM_DATE(FileAttr->lastWriteDate) == PB_GET_MONTH_FROM_DATE(LastDate))));

	SearchMonthIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);
	FileDB_SearhFile(g_PBSetting.FileHandle, FileIndex);

	if ((FileAttr == NULL) || (FileAttr->lastWriteDate == LastDate)) {
		return 0;
	} else {
		return FileAttr->lastWriteDate;
	}
}

/*
    Get first play year.

    @return date
*/
UINT16 PB_GetFirstPlayYear(void)
{
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 FileIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);

	SearchYearIndex = 0;

	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, SearchYearIndex);
	FileDB_SearhFile(g_PBSetting.FileHandle, FileIndex);

	if (FileAttr != NULL) {
		return FileAttr->lastWriteDate;
	} else {
		return 0;
	}
}

/*
    Get next play year.

    @return date
*/
UINT16 PB_GetNextPlayYear(void)
{
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 LastDate;
	UINT16 FileIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);

	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, SearchYearIndex);
	if (NULL == FileAttr) {
		DBG_ERR("FileAttr NG@PB_GetNextPlayYear!\r\n");
		return 0;
	}
	LastDate = FileAttr->lastWriteDate;

	do {
		FileAttr = FileDB_NextFile(g_PBSetting.FileHandle);

	} while ((FileAttr != NULL) && (PB_GET_YEAR_FROM_DATE(FileAttr->lastWriteDate) == PB_GET_YEAR_FROM_DATE(LastDate)));

	SearchYearIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);
	FileDB_SearhFile(g_PBSetting.FileHandle, FileIndex);

	if ((FileAttr == NULL) || (FileAttr->lastWriteDate == LastDate)) {
		return 0;
	} else {
		return FileAttr->lastWriteDate;
	}
}

/*
    Get prev play year.

    @return date
*/
UINT16 PB_GetPrevPlayYear(void)
{
	PFILEDB_FILE_ATTR FileAttr;
	UINT16 LastDate;
	UINT16 FileIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);

	FileAttr = FileDB_SearhFile(g_PBSetting.FileHandle, SearchYearIndex);
	if (NULL == FileAttr) {
		DBG_ERR("FileAttr NG@PB_GetPrevPlayYear!\r\n");
		return 0;
	}
	LastDate = FileAttr->lastWriteDate;

	do {
		FileAttr = FileDB_PrevFile(g_PBSetting.FileHandle);

	} while ((FileAttr != NULL) && (PB_GET_YEAR_FROM_DATE(FileAttr->lastWriteDate) == PB_GET_YEAR_FROM_DATE(LastDate)));

	SearchYearIndex = FileDB_GetCurrFileIndex(g_PBSetting.FileHandle);
	FileDB_SearhFile(g_PBSetting.FileHandle, FileIndex);

	if ((FileAttr == NULL) || (FileAttr->lastWriteDate == LastDate)) {
		return 0;
	} else {
		return FileAttr->lastWriteDate;
	}
}

/*
    Get total files of specific date.

    @return date
*/
UINT16 PB_GetTotalFilesNumOfSpecDate(void)
{
	return gMenuPlayInfo.uiTotalFilesInSpecDate;
}
//#NT#2007/06/25#Corey Ke -end

