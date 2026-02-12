/*
    Name Rule _FileDB.

    This file is the fileDB naming rule.

    @file       NameRule_FileDB.c
    @ingroup    mIMPENC
    @version    V1.01.001
    @date       2013/03/12

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/

#define  SUPPORTED_HWCLOCK   ENABLE

#include <stdio.h>
#include <string.h>
#include "kwrap/error_no.h"
#include "kwrap/type.h"
//#include "kernel.h"
#include "FileDB.h"
#include "avfile/MediaWriteLib.h"
#include "avfile/MediaReadLib.h"
#include <time.h>
#include <comm/hwclock.h>

//#include "Perf.h"

#include "NamingRule/NameRule_FileDB.h"

#define __MODULE__          NameRuleFileDB
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include <kwrap/debug.h>


//temp hardcode============SSS
#define Perf_Open()
#define Perf_Mark()
#define Perf_Close()
#define Perf_GetDuration()

//temp hardcode============EEE

#define NAMEHDL_FILEDB_CHECKID  0x666c6462 ///< 'fldb' as checkID
#define NAMEHDL_TIMENUM_MAX 999 //2013/06/25 Meg

//static CHAR gpCarDVname[CARDV_PATHLEN];
CHAR    gPathDefRoot[] = "CarDV";
CHAR    gPathDefNormal[] = "MOVIE";
CHAR    gPathDefCrash[] = "RO";
CHAR    gPathDefPhoto[] = "PHOTO";
CHAR    gPathDefEMR[] = "EMR";
CHAR    *gpPathDefRoot ;
CHAR    gPathA[] = "A:\\";
//CHAR    gPathCarDV[]= "A:\\%s\\MOVIE\\";
//CHAR    gPathCarDVCrash[]= "A:\\%s\\MOVIE\\";
//CHAR    gPathCarDVCrashNew[5];
//CHAR    gPathCarDVCrashPath[CARDV_PATHLEN];
CHAR    gPathCarDVLast[CARDV_PATHLEN]   = {0};
CHAR    gPathCarDVTemp1[CARDV_PATHLEN]  = {0};
CHAR    gPathCarDVTemp2[CARDV_PATHLEN]  = {0};
//CHAR    gPathTempRoot[CARDV_PATHLEN];
CHAR    gPathNewRoot[CARDV_NEWROOTLEN]  = {0};
CHAR    gPathNewNormal[CARDV_NEWROOTLEN] = {0};
CHAR    gPathNewROPath[CARDV_NEWROOTLEN] = {0};
CHAR    gPathNewJPGPath[CARDV_NEWROOTLEN] = {0};
CHAR    gPathNewEMRPath[CARDV_NEWROOTLEN] = {0};
//UINT32 testdid = 0, testfid = 0, testtype =0;
UINT8 gNH_filedb_CrashSameDid = 0, gNH_ChangeRoot = 0;
UINT8 gNH_ChangeNormal = 0, gNH_ChangeROPath = 0, gNH_ChangeJPGPath = 0, gNH_ChangeEMRPath = 0 ;
UINT32 gtimenum = 0;//2013/05/31 Meg
UINT32 gNH_filedb_lastIndex = 0;
UINT32 gNH_filedb_filetype = 0;
UINT32 gNR_filedb_validMovType = (FILEDB_FMT_MOV | FILEDB_FMT_MP4 | FILEDB_FMT_AVI | FILEDB_FMT_TS);
FILEDB_HANDLE gNHFileDB_id = 0;
UINT32 gNH_filedb_msg = 0;
UINT32 gNH_fd_LastIndex = 0;
UINT32 gNH_alwaysAdd2Last = 0;

char    gNH_fd_endChar = 0;
void NH_FileDB_MakeDirPath(CHAR *pFirst, CHAR *pSecond, CHAR *pPath, UINT32 pathlen);
static ER NH_FileDB_ChangeSecond2EMRPath(CHAR *pPath);

#if 0
UINT32 NH_FileDBUti_StrToInt(char *pPath, char *frontDirHeader)
{
	UINT32 a1, a2, a3, a4, num, len_dh;

	//char newFolder[] = "A:\\CarDV\\%04dxxxx.AVI\0";
	// 012 345678 90123456789012345678

	len_dh = strlen(frontDirHeader);
	a1 = pPath[len_dh] - '0';
	a2 = pPath[len_dh + 1] - '0';
	a3 = pPath[len_dh + 2] - '0';
	a4 = pPath[len_dh + 3] - '0';
	num = a1 * 1000 + a2 * 100 + a3 * 10 + a4;

	DBG_DUMP("StrToInt: path = %s, num= %d!\r\n", pPath, num);

	return num;
}
#endif


BOOL NH_FileDBUti_MakeObjPath(NM_NAMEINFO *ptimeinfo, UINT32 fileType, CHAR *filePath, CHAR *frontDir)
{
	char pExtName[4]={0};
	//char *pFileFreeChars;
	char pAvi[4] = "AVI\0";
	char pMov[4] = "MOV\0";
	char pJpg[4] = "JPG\0";
	char pRaw[4] = "RAW\0";
	char pMp4[4] = "MP4\0";
	char pTs [4] = "TS\0";
	if (gNH_filedb_msg) {
		DBG_IND("MakeObjPath!\r\n");
	}
	if (ptimeinfo->uiMin > 59) {
		DBG_ERR("min err(%d)\r\n", ptimeinfo->uiMin);
		return FALSE;
	} else if (ptimeinfo->uiSec > 59) {
		DBG_ERR("SEC err(%d)\r\n", ptimeinfo->uiSec);
		return FALSE;
	}
	if (fileType == MEDIA_FILEFORMAT_AVI) {
		memcpy((CHAR *)pExtName, pAvi, 4); ;
	} else if (fileType == MEDIA_FILEFORMAT_MOV) {
		memcpy((CHAR *)pExtName, pMov, 4); ;
	} else if (fileType == NAMERULE_FMT_JPG) {
		memcpy((CHAR *)pExtName, pJpg, 4); ;
	} else if (fileType == MEDIA_FILEFORMAT_MP4) {
		memcpy((CHAR *)pExtName, pMp4, 4); ;
	} else if (fileType == NAMERULE_FMT_RAW) {
		memcpy((CHAR *)pExtName, pRaw, 4); ;
	} else if (fileType == MEDIA_FILEFORMAT_TS) {
		memcpy((CHAR *)pExtName, pTs, 3); ;
	}
#if 1
	ptimeinfo->ucChar = gNH_fd_endChar;//'A';
	if (ptimeinfo->SecValid) { //for Photo, NAMEHDL_TIMENUM_MAX
		if (ptimeinfo->ucChar) {
			snprintf(filePath, CARDV_PATHLEN,
					 "%s%04d_%02d%02d_%02d%02d%02d_%03d%c.%s", frontDir,
					 (int)(ptimeinfo->uiYear),
					 (int)(ptimeinfo->uiMonth),
					 (int)(ptimeinfo->uiDate),
					 (int)(ptimeinfo->uiHour),
					 (int)(ptimeinfo->uiMin),
					 (int)(ptimeinfo->uiSec),
					 (int)(ptimeinfo->uiNumber),
					 (char)(ptimeinfo->ucChar),
					 ((pExtName[0])?(pExtName):("")));
		} else {
			snprintf(filePath, CARDV_PATHLEN,
					 "%s%04d_%02d%02d_%02d%02d%02d_%03d.%s", frontDir,
					 (int)(ptimeinfo->uiYear),
					 (int)(ptimeinfo->uiMonth),
					 (int)(ptimeinfo->uiDate),
					 (int)(ptimeinfo->uiHour),
					 (int)(ptimeinfo->uiMin),
					 (int)(ptimeinfo->uiSec),
					 (int)(ptimeinfo->uiNumber),
					 ((pExtName[0])?(pExtName):("")));
		}
	} else {
		if (ptimeinfo->ucChar) {
			snprintf(filePath, CARDV_PATHLEN,
					 "%s%04d_%02d%02d_%02d%02d_%05d%c.%s", frontDir,
					 (int)(ptimeinfo->uiYear),
					 (int)(ptimeinfo->uiMonth),
					 (int)(ptimeinfo->uiDate),
					 (int)(ptimeinfo->uiHour),
					 (int)(ptimeinfo->uiMin),
					 (int)(ptimeinfo->uiNumber),
					 (char)(ptimeinfo->ucChar),
					 ((pExtName[0])?(pExtName):("")));
		} else {
			snprintf(filePath, CARDV_PATHLEN,
					 "%s%04d_%02d%02d_%02d%02d_%05d.%s", frontDir,
					 (int)(ptimeinfo->uiYear),
					 (int)(ptimeinfo->uiMonth),
					 (int)(ptimeinfo->uiDate),
					 (int)(ptimeinfo->uiHour),
					 (int)(ptimeinfo->uiMin),
					 (int)(ptimeinfo->uiNumber),
					 ((pExtName[0])?(pExtName):("")));
		}
	}
#else
	sprintf(filePath, "%s2013_0303_0506_%05d.%s", frontDir,
			(int)(ptimeinfo->uiNumber),
			pExtName);
#endif
	if (gNH_filedb_msg) {
		DBG_IND("%s\r\n", (char *)filePath);
	}
	return TRUE;
}

static BOOL NH_FileDBUti_ReplaceStr(CHAR *oldfront, CHAR *newfront, CHAR *oldPath, CHAR *newPath)
{
	//char *start;
	UINT32 oldlen, newlen, oldalllen;
	//example
	//oldfront: A:\FRONT\MOVIE\  ("\" needed!)
	//newfront: A:\FRONT\RO\     ("\" needed!)
	//oldpath:  A:\FRONT\MOVIE\01.MOV
	//newpath:  A:\FRONT\RO\01.MOV

	oldlen = strlen(oldfront);
	newlen = strlen(newfront);
	oldalllen = strlen(oldPath);
	{
		memcpy(newPath, newfront, newlen);
		if (gNH_filedb_msg) {
			DBG_IND("newPath1 = %s!\r\n", newPath);
		}
		memcpy(newPath + (newlen - 1), oldPath + (oldlen - 1), oldalllen - oldlen + 1);
		if (gNH_filedb_msg) {
			DBG_IND("newPath2 = %s!\r\n", newPath);
		}
	}

	return 0;
}

static BOOL NH_FileDBUti_Check(CHAR *pPath)
{
	INT32 num;

	num = FileDB_GetIndexByPath(gNHFileDB_id, pPath);
	if (gNH_filedb_msg) {
		DBG_IND("num = 0x%lx!\r\n", num);
	}
	if (num == FILEDB_SEARCH_ERR) {
		return FALSE;
	}

	else {
		return TRUE;
	}
}
#if 0
static void NH_FileDB_SetSameCrashID(UINT8 yesOrNot)
{
	gNH_filedb_CrashSameDid = yesOrNot;

}

static void NH_FileDB_Set1stCrashID(void)//remerber 1st crash did,fid
{
	if (gNH_filedb_CrashSameDid) { //using old
		//todo: get old folder??
	} else { //next be crash folder
		//set crash folder, gPathCarDVCrash
	}
}

static void NH_FileDB_SetCrashFolderName(char *pChar)
{
	memset(gPathCarDVCrashNew, 0, 5);
	memcpy(gPathCarDVCrashNew, pChar, 5);

	//gPathCarDVCrashNew

	//sprintf(gPathCarDVCrashPath, gPathCarDVCrashNew,pChar);
	if (gNH_filedb_msg) {
		DBG_DUMP("NEW CRASH FOLDER NAME = %s!!\r\n", gPathCarDVCrashNew);
	}
}
#endif

static void NH_FileDB_SetInfo(MEDIANAMING_SETINFO_TYPE type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	if (gNH_filedb_msg) {
		DBG_IND("Set Info type = %d!! p1= %d!\r\n", type, p1);
	}
	switch (type) {
	//case MEDIANAMING_SETINFO_SAMECRASHFOLDER:
	//    NH_FileDB_SetSameCrashID(p1);
	//    break;

	//case MEDIANAMING_SETINFO_SET1STCRASH:
	//    NH_FileDB_Set1stCrashID();
	//    break;

	//case MEDIANAMING_SETINFO_SETCRASHNAME:
	//    NH_FileDB_SetCrashFolderName((char *)p1);
	//    break;

	case MEDIANAMING_SETINFO_WORKINGHDL:
		gNHFileDB_id = (FILEDB_HANDLE)p1;
		break;

	//case MEDIANAMING_SETINFO_ROOTFOLDERNAME:
	//    gNH_ChangeRoot = 1;
	//    memset(gPathNewRoot, 0, CARDV_NEWROOTLEN);
	//    memcpy(gPathNewRoot, (char *)p1, CARDV_NEWROOTLEN);
	//    break;

	case MEDIANAMING_SETINFO_PATH_ENDCHAR:
		gNH_fd_endChar = (char)p1;
		break;
	default:
		break;
	}
}


static ER NH_FileDB_CreateAndGetPath(UINT32 filetype, CHAR *pPath)
{
	//UINT32 DirID, FileID, FileFormat=0;
	BOOL cr_exist = TRUE;
	//CHAR *path;
	//RTC_TIME newtime;
	NM_NAMEINFO timeinfo;
	//RTC_DATE newdate;
    struct tm Curr_DateTime;
#if (SUPPORTED_HWCLOCK == ENABLE)
    Curr_DateTime = hwclock_get_time(TIME_ID_CURRENT);
#else
	memset(&Curr_DateTime, 0, sizeof(struct tm));
#endif

	//UINT32 copylen;
	CHAR *pFullPath, *pFirst, *pSecond;
	gNH_filedb_filetype = filetype;
	//newtime = rtc_getTime();
	timeinfo.uiHour = Curr_DateTime.tm_hour;
	timeinfo.uiMin = Curr_DateTime.tm_min;
	timeinfo.uiSec = Curr_DateTime.tm_sec;
	//newdate =  rtc_getDate();
	timeinfo.uiYear = Curr_DateTime.tm_year;
	timeinfo.uiMonth = Curr_DateTime.tm_mon;
	timeinfo.uiDate = Curr_DateTime.tm_mday;
	timeinfo.SecValid = 1;//2013/05/28 Meg, default with second
	gtimenum += 1;
	if (gtimenum > NAMEHDL_TIMENUM_MAX) { //2013/06/25 Meg, cyclic
		gtimenum = 1;
	}
	timeinfo.uiNumber = gtimenum;//todo: getLast in fileDB
	//FileDB_DumpInfo(gNHFileDB_id);
	memset(gPathCarDVTemp1, 0, CARDV_PATHLEN);

	if (gNH_ChangeRoot) {
		pFirst = gPathNewRoot;
	} else {
		pFirst = gPathDefRoot;
	}
	if (filetype == NAMERULE_FMT_JPG) {
		if (gNH_ChangeJPGPath) {
			pSecond = gPathNewJPGPath;
		} else {
			pSecond = gPathDefPhoto;
		}
	} else {
		if (gNH_ChangeNormal) {
			pSecond = gPathNewNormal;
		} else {
			pSecond = gPathDefNormal;
		}
	}
	NH_FileDB_MakeDirPath(pFirst, pSecond, gPathCarDVTemp1, CARDV_PATHLEN);
	pFullPath = gPathCarDVTemp1;
	/*
	copylen = CARDV_PATHLEN;
	strncat(pFullPath, gPathA, copylen-1);
	copylen = CARDV_PATHLEN - strlen(pFullPath);
	if (gNH_ChangeRoot)
	{
	    strncat(pFullPath, gPathNewRoot, copylen-1);
	}
	else
	{
	    strncat(pFullPath, gPathDefRoot, copylen-1);
	}
	copylen = CARDV_PATHLEN - strlen(pFullPath);
	strncat(pFullPath, "\\", copylen-1);

	if (filetype==NAMERULE_FMT_JPG)
	{
	    copylen = CARDV_PATHLEN - strlen(pFullPath);
	    strncat(pFullPath, "\\", copylen-1);
	}
	else
	{
	    copylen = CARDV_PATHLEN - strlen(pFullPath);
	    if (gNH_ChangeNormal)
	    {
	        strncat(pFullPath, gPathNewNormal, copylen-1);
	    }
	    else
	    {
	        strncat(pFullPath, gPathDefNormal, copylen-1);
	    }
	}

	copylen = CARDV_PATHLEN - strlen(pFullPath);
	strncat(pFullPath, "\\", copylen-1);
	*/
	if (gNH_filedb_msg) {
		DBG_IND("Uti_Print pDst = %s!!\r\n", pFullPath);
	}


	NH_FileDBUti_MakeObjPath(&timeinfo, filetype, pPath, pFullPath);

	while (cr_exist) {
		if (NH_FileDBUti_Check(pPath) == TRUE) {
			gtimenum += 1;
			timeinfo.uiNumber = gtimenum;
			NH_FileDBUti_MakeObjPath(&timeinfo, filetype, pPath, pFullPath);
		} else {
			if (gNH_filedb_msg) {
				DBG_IND("Name %s OK !\r\n", pPath);
			}
			cr_exist = FALSE;
		}

	}
	memcpy((char *)gPathCarDVLast, (char *)pPath, CARDV_PATHLEN);
	return E_OK;


}

static ER NH_FileDB_AddPath(CHAR *pPath, UINT32 attrib)
{
	UINT32 num = 0;
	PFILEDB_FILE_ATTR  pfile;
	if (gNH_alwaysAdd2Last) {
		if (FileDB_AddFileToLast(gNHFileDB_id, pPath) == TRUE) {
			if (gNH_filedb_msg) {
				DBG_IND("AddPath %s OK!!!\r\n", pPath);
			}
			//num = NH_FileDBUti_StrToInt(pPath);

			//DBG_DUMP("^R  Num add =  %d !!!!!\r\n", num);

			//num = FileDB_GetCurrFileIndex(gNHFileDB_id);
			num = FileDB_GetIndexByPath(gNHFileDB_id, pPath);//2014/04/03 fixed
			if (attrib == NAMERULE_ATTRIB_OR_READONLY) {
				pfile = FileDB_SearhFile2(gNHFileDB_id, num);
				if (pfile) { //coverity 43788
					pfile->attrib |= FST_ATTRIB_READONLY;
				}
			}
			gNH_fd_LastIndex = num;


			//FileDB_DumpInfo(gNHFileDB_id);
			return E_OK;

		} else {
			DBG_ERR("AddPath %s fails!!!\r\n", pPath);
			return E_SYS;
		}
	} else {
		if (FileDB_AddFile(gNHFileDB_id, pPath) == TRUE) {
			if (gNH_filedb_msg) {
				DBG_DUMP("AddPath %s OK!!!\r\n", pPath);
			}
			//num = NH_FileDBUti_StrToInt(pPath);

			//DBG_DUMP("^R  Num add =  %d !!!!!\r\n", num);

			//num = FileDB_GetCurrFileIndex(gNHFileDB_id);
			num = FileDB_GetIndexByPath(gNHFileDB_id, pPath);//2014/04/03 fixed
			if (attrib == NAMERULE_ATTRIB_OR_READONLY) {
				pfile = FileDB_SearhFile2(gNHFileDB_id, num);
				if (pfile) { //coverity 43788
					pfile->attrib |= FST_ATTRIB_READONLY;
				}
			}
			gNH_fd_LastIndex = num;


			//FileDB_DumpInfo(gNHFileDB_id);
			return E_OK;

		} else {
			DBG_ERR("AddPath %s fails!!!\r\n", pPath);
			return E_SYS;
		}
	}

}

static ER NH_FileDB_DelPath(CHAR *pPath)
{
	INT32    index;
	//INT32 ret;
	Perf_Mark();
#if 1
	index = FileDB_GetIndexByPath(gNHFileDB_id, pPath);
	if (index != FILEDB_SEARCH_ERR) {
		FileDB_DeleteFile(gNHFileDB_id, index);
	}
#else
	//ret = FileSys_DeleteFile(pPath);
	//if (ret)
	//
	//{
	//    DBG_DUMP("FS delete %s fail!!!!!\r\n",pPath);
	//    return E_SYS;
	//}

	FileDB_DeleteFile(gNHFileDB_H, gNH_filedb_lastIndex);

#endif
	if (gNH_filedb_msg) {
		DBG_IND("DelPath = %s ,index =%d ,delTime = %07d us \r\n", pPath, index, Perf_GetDuration());
	}
	//FileDB_DumpInfo(gNHFileDB_H);//testing

	return E_OK;
}


static ER NH_FileDB_GetPathBySeq(UINT32 uiSequ, CHAR *pPath)
{
	FILEDB_FILE_ATTR *pFileAttr;

	pFileAttr = FileDB_SearhFile(gNHFileDB_id, uiSequ - 1); //Media count from 1, but FileDB from 0
	if (pFileAttr == NULL) {
		DBG_ERR("GetPathBySeq fails..!!!\r\n");
		return E_SYS;
	}
	if (gNH_filedb_msg) {
		DBG_IND(" getpath %s \r\n", pFileAttr->filePath);
	}
	if ((gNR_filedb_validMovType & pFileAttr->fileType) == 0) {

		gNH_filedb_lastIndex = uiSequ - 1;
		return E_PAR;
	}
	memcpy((char *)pPath, (char *)pFileAttr->filePath, CARDV_PATHLEN);
	gNH_filedb_lastIndex = uiSequ - 1;
	return E_OK;

}
#if 0
static ER NH_FileDB_CalcNextID(void)
{
	//gtimenum +=1;
	return E_OK;
}
#endif
static ER NH_FileDB_GetLastPath(CHAR *pPath)
{
	//UINT32 totalfiles;
	//PFILEDB_FILE_ATTR pFileAttr;
#if 0
	totalfiles = FileDB_GetTotalFileNum(gNHFileDB_H);
	pFileAttr = FileDB_SearhFile(gNHFileDB_H, totalfiles - 1);
	if (pFileAttr == NULL) {
		DBG_ERR("GetLastPath fails..!!!\r\n");
		return E_SYS;
	}
	DBG_DUMP(" last %s \r\n", pFileAttr->filePath);
	memcpy((char *)pPath, (char *)pFileAttr->filePath, CARDV_PATHLEN);
#else
	//actually get lastest working path
	memcpy((char *)pPath, (char *)gPathCarDVLast, CARDV_PATHLEN);

#endif
	return E_OK;

}
#if 0
static ER NH_FileDB_GetNewCrashPath(CHAR *pPath, UINT8 setActive)
{
	CHAR newpath[CARDV_PATHLEN];

	memset(newpath, 0, CARDV_PATHLEN);
	memset(gPathCarDVTemp1, 0, CARDV_PATHLEN);
	sprintf(gPathCarDVTemp1, gPathCarDV, gpPathDefRoot);

	memset(gPathCarDVTemp2, 0, CARDV_PATHLEN);
	sprintf(gPathCarDVTemp2, gPathCarDVCrash, gpPathDefRoot);

	NH_FileDBUti_ReplaceStr(gPathCarDVTemp1, gPathCarDVTemp2, gPathCarDVLast, newpath);
	memcpy((unsigned char *)pPath, newpath, CARDV_PATHLEN);
	return E_OK;
}
#endif
static ER NH_FileDB_ChangeFrontPath(CHAR *pPath)
{
	CHAR newpath[CARDV_PATHLEN];
	CHAR oldpath[CARDV_PATHLEN];
	CHAR *pFirst, *pSecond;
	memcpy(oldpath, pPath, CARDV_PATHLEN);

	memset(newpath, 0, CARDV_PATHLEN);
	if (gNH_ChangeRoot) {
		pFirst = gPathNewRoot;
	} else {
		pFirst = gPathDefRoot;
	}
	if (gNH_ChangeNormal) {
		pSecond = gPathNewNormal;
	} else {
		pSecond = gPathDefNormal;
	}

	NH_FileDB_MakeDirPath(pFirst, pSecond, gPathCarDVTemp1, CARDV_PATHLEN);
	if (gNH_ChangeROPath) {
		pSecond = gPathNewROPath;
	} else {
		pSecond = gPathDefCrash;
	}

	NH_FileDB_MakeDirPath(pFirst, pSecond, gPathCarDVTemp2, CARDV_PATHLEN);

	NH_FileDBUti_ReplaceStr(gPathCarDVTemp1, gPathCarDVTemp2, oldpath, newpath);
	//DBG_DUMP("newpath=%s\r\n", newpath);
	memcpy((unsigned char *)pPath, newpath, CARDV_PATHLEN);
	return E_OK;
}

static ER NH_FileDB_ChangeSecond2EMRPath(CHAR *pPath)
{
	CHAR newpath[CARDV_PATHLEN];
	CHAR oldpath[CARDV_PATHLEN];
	CHAR *pFirst, *pSecond;
	memcpy(oldpath, pPath, CARDV_PATHLEN);

	memset(newpath, 0, CARDV_PATHLEN);
	if (gNH_ChangeRoot) {
		pFirst = gPathNewRoot;
	} else {
		pFirst = gPathDefRoot;
	}
	if (gNH_ChangeNormal) {
		pSecond = gPathNewNormal;
	} else {
		pSecond = gPathDefNormal;
	}

	NH_FileDB_MakeDirPath(pFirst, pSecond, gPathCarDVTemp1, CARDV_PATHLEN);
	if (gNH_ChangeEMRPath) {
		pSecond = gPathNewEMRPath;
	} else {
		pSecond = gPathDefEMR;
	}

	NH_FileDB_MakeDirPath(pFirst, pSecond, gPathCarDVTemp2, CARDV_PATHLEN);

	NH_FileDBUti_ReplaceStr(gPathCarDVTemp1, gPathCarDVTemp2, oldpath, newpath);
	//DBG_DUMP("newpath=%s\r\n", newpath);
	memcpy((unsigned char *)pPath, newpath, CARDV_PATHLEN);
	return E_OK;
}


static ER NH_FileDB_customizeFunc(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	ER returnV = E_SYS;
	CHAR *pPath;
	UINT64 *p64int = 0;
	UINT32 *p32int = 0, *p33int = 0;

	switch (type) {
	case MEDIANR_CUSTOM_CHANGECRASH://CAR\MOVIE-> CAR\RO
		if (p1 == 0) {
			DBG_ERR("MEDIANR_CUSTOM_CHANGECRASH NULL\r\n");
			return returnV;
		}
		pPath = (CHAR *)p1;

		NH_FileDB_ChangeFrontPath(pPath);
		returnV = E_OK;
		break;

	case MEDIANR_CUSTOM_CHANGE_TO_EMR://CAR\MOVIE-> CAR\EMR
		if (p1 == 0) {
			DBG_ERR("MEDIANR_CUSTOM_CHANGE_TO_EMR NULL\r\n");
			return returnV;
		}
		pPath = (CHAR *)p1;
		//pathid = p2;
		NH_FileDB_ChangeSecond2EMRPath(pPath);
		returnV = E_OK;
		break;

	case MEDIANR_CUSTOM_GETFILES://2013/11/08
		p32int = (UINT32 *)p1;
		*p32int = FileDB_GetTotalFileNum(gNHFileDB_id);
		returnV = E_OK;
		break;

	case MEDIANR_CUSTOM_GETFILESIZE_BYSEQ: {
			FILEDB_FILE_ATTR *pFileAttr;
			p64int = (UINT64 *)p2;
			p33int = (UINT32 *)p3;
			pFileAttr = FileDB_SearhFile(gNHFileDB_id, p1 - 1); //Media count from 1, but FileDB from 0
			if (pFileAttr == NULL) {
				DBG_ERR("GetPathBySeq fails..!!!\r\n");
				*p64int = 0;
				return E_SYS;
			}
			if (1) { //gNH_filedb_msg)
				//DBG_DUMP(" getpath %s ", pFileAttr->filePath);
				//DBG_DUMP(" size = %ld \r\n", pFileAttr->fileSize);
			}

			*p64int = pFileAttr->fileSize64;
			*p33int = (UINT32)pFileAttr->attrib;
			returnV = E_OK;
		}
		break;

	case MEDIANR_CUSTOM_SET_FILESIZE: {
			FILEDB_FILE_ATTR *pFileAttr;
			INT32 num;
			UINT64 size64;
			pPath = (CHAR *)p1;

			num = FileDB_GetIndexByPath(gNHFileDB_id, pPath);
			pFileAttr = FileDB_SearhFile(gNHFileDB_id, num);
			if (pFileAttr) { //coverity 43789
				if (p3) {
					size64 = (UINT64)p2 + ((UINT64)p3 << 32);
				} else {
					size64 = (UINT64)p2;
				}

				if (FALSE == FileDB_SetFileSize(gNHFileDB_id, num, size64)) {
					DBG_IND("^RSetpathFAIL=%s,size=0x%llx!\r\n", pPath, size64);
				} else {
					DBG_IND("^M setfilesize2=0x%llx\r\n", size64);
				}
			} else {
				DBG_IND("^R pFileNULL!\r\n");
			}
			//DBG_DUMP("Setpath=%s,size=0x%lx!\r\n", pPath, p2);
			returnV = E_OK;
		}
		break;

	case MEDIANR_CUSTOM_GET_FILESIZE: {
			FILEDB_FILE_ATTR *pFileAttr;
			INT32 num;

			pPath = (CHAR *)p1;
			p32int = (UINT32 *)p2;
			if (p3) {
				p33int = (UINT32 *)p3;
			}

			num = FileDB_GetIndexByPath(gNHFileDB_id, pPath);
			pFileAttr = FileDB_SearhFile(gNHFileDB_id, num);
			if (pFileAttr == NULL) {
				DBG_ERR("GET_FILESIZE fails..!!!\r\n");
				*p32int = 0;
				if (p33int) {
					*p33int = 0;
				}
				return E_SYS;
			}

			*p32int  = (UINT32)pFileAttr->fileSize64;
			if (p33int) {
				*p33int = (UINT32)(pFileAttr->fileSize64 >> 32);
			}
			//DBG_DUMP("getpath=%s,filesize=0x%lx!\r\n", pPath, pFileAttr->fileSize);
			returnV = E_OK;
		}
		break;

	case MEDIANR_CUSTOM_GET_ATTRIB: {
			FILEDB_FILE_ATTR *pFileAttr;
			INT32 num;
			pPath = (CHAR *)p1;
			p32int = (UINT32 *)p2;

			num = FileDB_GetIndexByPath(gNHFileDB_id, pPath);
			pFileAttr = FileDB_SearhFile(gNHFileDB_id, num);
			if (pFileAttr == NULL) {
				DBG_ERR("GET_ATTRIB fails..!!!\r\n");
				*p32int = 0;
				return E_SYS;
			}
			*p32int = (UINT32)pFileAttr->attrib;
			returnV = E_OK;
		}
		break;

	case MEDIANR_CUSTOM_UPDATE_ATTRIB: {
			FILEDB_FILE_ATTR *pFileAttr;
			INT32 num;
			pPath = (CHAR *)p1;

			num = FileDB_GetIndexByPath(gNHFileDB_id, pPath);
			pFileAttr = FileDB_SearhFile(gNHFileDB_id, num);

			if ((p2 == NAMERULE_ATTRIB_CLR_READONLY) && (pFileAttr)) {
				pFileAttr->attrib &= ~FS_ATTRIB_READ;
				DBG_IND("Setpath=%s,attr=0x%lx!\r\n", pPath, pFileAttr->attrib);
			}
			returnV = E_OK;
		}
		break;

	default:
		break;
	}

	return returnV;
}

MEDIANAMINGRULE  gNamingFileDBHdl = {
	NH_FileDB_CreateAndGetPath, //CreateAndGetPath
	NH_FileDB_AddPath,          //AddPath
	NH_FileDB_DelPath,          //DelPath
	NH_FileDB_GetPathBySeq,     //GetPathBySeq
	NULL, //NH_FileDB_CalcNextID,//CalcNextID
	NH_FileDB_GetLastPath,//GetLastPath
	NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
	NH_FileDB_customizeFunc,
	NH_FileDB_SetInfo,        //SetInfo
	NAMEHDL_FILEDB_CHECKID
};

PMEDIANAMINGRULE NameRule_getFileDB(void)
{
	MEDIANAMINGRULE *pHdl;


	pHdl = &gNamingFileDBHdl;
	if (gNH_ChangeRoot) {
		gpPathDefRoot = &gPathNewRoot[0];
	} else {

		gpPathDefRoot = &gPathDefRoot[0];
	}
	if (gNH_filedb_msg) {
		DBG_DUMP("^G NameRule path = %s!\r\n", gpPathDefRoot);
	}
	return pHdl;
}


void NH_FileDB_OpenMsg(UINT8 on)
{
	gNH_filedb_msg = on;
}

void NH_FileDB_SetRootFolder(char *pChar)
{
	gNH_ChangeRoot = 1;
	memset(gPathNewRoot, 0, CARDV_NEWROOTLEN);
	memcpy(gPathNewRoot, (char *)pChar, CARDV_NEWROOTLEN);
}

void NH_FileDB_SetNormalFolder(char *pChar)
{
	gNH_ChangeNormal = 1;
	memset(gPathNewNormal, 0, CARDV_NEWROOTLEN);
	memcpy(gPathNewNormal, (char *)pChar, CARDV_NEWROOTLEN);
}

void NH_FileDB_SetReadOnlyFolder(char *pChar)
{
	gNH_ChangeROPath = 1;
	memset(gPathNewROPath, 0, CARDV_NEWROOTLEN);
	memcpy(gPathNewROPath, (char *)pChar, CARDV_NEWROOTLEN);
}

void NH_FileDB_SetJPGFolder(char *pChar)
{
	gNH_ChangeJPGPath = 1;
	memset(gPathNewJPGPath, 0, CARDV_NEWROOTLEN);
	memcpy(gPathNewJPGPath, (char *)pChar, CARDV_NEWROOTLEN);
}

void NH_FileDB_SetEMRFolder(char *pChar)
{
	gNH_ChangeEMRPath = 1;
	memset(gPathNewEMRPath, 0, CARDV_NEWROOTLEN);
	memcpy(gPathNewEMRPath, (char *)pChar, CARDV_NEWROOTLEN);
}

void NH_FileDB_SetFileID(UINT32 uiFileID)
{
	gtimenum = uiFileID;
}

void NH_FileDB_MakeDirPath(CHAR *pFirst, CHAR *pSecond, CHAR *pPath, UINT32 pathlen)
{
	UINT32 copylen = 0;
	memset(pPath, 0, pathlen);

	copylen = pathlen;
	strncat(pPath, gPathA, copylen - 1); //A:
	copylen = pathlen - strlen(pPath);
	strncat(pPath, pFirst, copylen - 1); //A:\CarDV
	copylen = pathlen - strlen(pPath);
	strncat(pPath, "\\", copylen - 1);
	copylen = pathlen - strlen(pPath);
	strncat(pPath, pSecond, copylen - 1); //A:\CarDV\MOVIE
	copylen = pathlen - strlen(pPath);
	strncat(pPath, "\\", copylen - 1);
}

void NH_FileDB_SetMOVIEfiletype(UINT32 type)
{
	gNR_filedb_validMovType = type;

}

void NH_FileDB_Set_AlwaysAdd2Last(UINT32 value)
{
	gNH_alwaysAdd2Last = value;

}
