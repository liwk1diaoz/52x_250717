/*
    Name Rule _Custom.

    This file is the custom naming rule. (need fileDB)

    @file       NameRule_Custom.c
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
#include "avfile/MediaWriteLib.h"
#include "avfile/MediaReadLib.h"
#include "FileDB.h"
//#include "HwClock.h"
#include "NamingRule/NameRule_Custom.h"
#include "NameRule_Custom_def.h"
//#include "MediaRecAPI.h"
//#include "Perf.h"
#include <kwrap/verinfo.h>

#define __MODULE__          NameRuleCustom
#define __DBGLVL__          1 // 0=OFF, 1=ERROR, 2=TRACE
#include <kwrap/debug.h>
#include <time.h>
#include <comm/hwclock.h>

//temp hardcode============SSS
#define Perf_Open()
#define Perf_Mark()
#define Perf_Close()
#define Perf_GetDuration()

//temp hardcode============EEE



#define NMC_MAX_PATH  16
#define NAMEHDL_CUS_TIMENUM_MAX 999 //2016/07/07 Meg

UINT32  gNH_custom_msg = 0;
FILEDB_HANDLE gNHCustom_id[NMC_MAX_PATH] = {0, 1};
UINT32  gNHC_timenum = 0;//2013/05/31 Meg
UINT8   gNHC_usemethod1 = 0;
UINT8   gNHC_usemethod2 = 0;
CHAR    gNHC_Root[NMC_MAX_PATH][NMC_ROOT_MAX_LEN] = {0};
CHAR    gNHC_RO[NMC_MAX_PATH][NMC_OTHERS_MAX_LEN] = {0};
CHAR    gNHC_Movie[NMC_MAX_PATH][NMC_OTHERS_MAX_LEN] = {0};
CHAR    gNHC_Photo[NMC_MAX_PATH][NMC_OTHERS_MAX_LEN] = {0};
CHAR    gNHC_EMR[NMC_MAX_PATH][NMC_OTHERS_MAX_LEN] = {0};
CHAR    gNHC_Ev1[NMC_MAX_PATH][NMC_OTHERS_MAX_LEN] = {0};
CHAR    gNHC_Ev2[NMC_MAX_PATH][NMC_OTHERS_MAX_LEN] = {0};
CHAR    gNHC_Ev3[NMC_MAX_PATH][NMC_OTHERS_MAX_LEN] = {0};
CHAR    gNHC_Def_Root[]   = "A:\\";
CHAR    gNHC_Def_RO[]     = "RO1";
CHAR    gNHC_Def_RORear[]     = "RO2";
CHAR    gNHC_Def_Movie[]  = "MOVIE1";
CHAR    gNHC_Def_Photo1[]  = "PHOTO1";
CHAR    gNHC_Def_Photo2[]  = "PHOTO2";
CHAR    gNHC_Def_EMR[]    = "EMR";
CHAR    gNHC_Def_Rear[]   = "MOVIE2";//default front/rear the same folder,
// using endchar to distinguish
CHAR    gNHC_Def_Event1[] = "EV1";
CHAR    gNHC_Def_Event2[] = "EV2";
CHAR    gNHC_Def_Event3[] = "EV3";
UINT8   gNHC_ChangeRoot[NMC_MAX_PATH] = {0};
UINT8   gNHC_ChangeRO[NMC_MAX_PATH] = {0};
UINT8   gNHC_ChangeMovie[NMC_MAX_PATH] = {0};
UINT8   gNHC_ChangePhoto[NMC_MAX_PATH] = {0};
UINT8   gNHC_ChangeEMR[NMC_MAX_PATH] = {0};
UINT8   gNHC_ChangeEv1[NMC_MAX_PATH] = {0};
UINT8   gNHC_ChangeEv2[NMC_MAX_PATH] = {0};
UINT8   gNHC_ChangeEv3[NMC_MAX_PATH] = {0};
UINT32  gNHC_filetype = 0;
UINT32  gNHC_activePathid = 0;
UINT32  gNHC_activeEMRPathid = 0, gNHC_activeEMRfilesavetype = NMC_FILESAVE_PART1;
UINT32  gNHC_active2ndfilesavetype = NMC_FILESAVE_PART1;
CHAR    gPathNMCTemp1[NMC_TOTALFILEPATH_MAX_LEN];
CHAR    gPathNMCTemp2[NMC_MAX_PATH][NMC_TOTALFILEPATH_MAX_LEN];
CHAR    gPathNMCTemp3[NMC_TOTALFILEPATH_MAX_LEN];
CHAR    gPathNMCTemp4[NMC_TOTALFILEPATH_MAX_LEN];

CHAR    gPathNMCTemp5[NMC_TOTALFILEPATH_MAX_LEN] = {0};
NMC_TIMEINFO gnmc_timeinfo;
UINT32  gNR_cus_validMovType = (FILEDB_FMT_MOV | FILEDB_FMT_MP4 | FILEDB_FMT_AVI | FILEDB_FMT_TS);
UINT32  gNHC_lastIndex = 0;
CHAR    gNHC_LastPath[NMC_TOTALFILEPATH_MAX_LEN];
UINT32  gNHC_filepathMethod = NMC_FILEPATH_METHOD_3;
NMC_NAMEINFO gNHC_Nameinfo[NMC_MAX_PATH] = {0};
CHAR    gNHC_TempEnd[NMC_MAX_PATH][NMC_OTHERS_MAX_LEN] = {0};
ER NH_Custom_CreateAndGetPath(UINT32 filetype, CHAR *pPath, UINT32 activeid);
void NH_Custom_SetInfo(MEDIANAMING_SETINFO_TYPE type, UINT32 p1, UINT32 p2, UINT32 p3);
static ER NH_Custom_AddPath1(CHAR *pPath, UINT32 attrib);
static ER NH_Custom_AddPath2(CHAR *pPath, UINT32 attrib);
static ER NH_Custom_DelPath1(CHAR *pPath);
static ER NH_Custom_DelPath2(CHAR *pPath);
static ER NH_Custom_GetPathBySeq1(UINT32 uiSequ, CHAR *pPath);
static ER NH_Custom_GetPathBySeq2(UINT32 uiSequ, CHAR *pPath);
static ER NH_Custom_customizeFunc1(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3);
static ER NH_Custom_customizeFunc2(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3);

NMC_FOLDER_INFO   gNHC_Folder_Setting[NMC_MAX_PATH] = {
	{
		0,
		gNHC_Def_RO,
		gNHC_Def_Movie,
		gNHC_Def_Photo1,
		gNHC_Def_EMR,
		gNHC_Def_Event1,
		gNHC_Def_Event2,
		gNHC_Def_Event3
	},

	{
		1,
		gNHC_Def_RO,
		gNHC_Def_Rear,
		gNHC_Def_Photo2,
		gNHC_Def_EMR,
		gNHC_Def_Event1,
		gNHC_Def_Event2,
		gNHC_Def_Event3
	}
};

BOOL NH_CustomUti_MakeObjPath(NMC_TIMEINFO *ptimeinfo, NMC_NAMEINFO *pInfo, CHAR *filePath, CHAR *frontDir)
{
	char pExtName[4] = {0}; //coverity 44573
	//char *pFileFreeChars;
	char pAvi[4] = "AVI\0";
	char pMov[4] = "MOV\0";
	char pJpg[4] = "JPG\0";
	char pRaw[4] = "RAW\0";
	char pMp4[4] = "MP4\0";
	char pTs [4] = "TS\0";
	if (gNH_custom_msg) {
		DBG_DUMP("MakeObjPath! ");
	}
	if (pInfo->timetype != NAMERULECUS_TIMETYPE_GIVE) {
		if (ptimeinfo->uiMin > 59) {
			DBG_ERR("min err(%d)\r\n", ptimeinfo->uiMin);
			return FALSE;
		} else if (ptimeinfo->uiSec > 59) {
			DBG_ERR("SEC err(%d)\r\n", ptimeinfo->uiSec);
			return FALSE;
		}
	}
	if (pInfo->filetype == MEDIA_FILEFORMAT_AVI) {
		memcpy((CHAR *)pExtName, pAvi, 4); ;
	} else if (pInfo->filetype == MEDIA_FILEFORMAT_MOV) {
		memcpy((CHAR *)pExtName, pMov, 4); ;
	} else if ((pInfo->filetype == NAMERULE_FMT_JPG)
			   || (pInfo->filetype == MEDIA_FILEFORMAT_JPG)) { //2017/05/24
		memcpy((CHAR *)pExtName, pJpg, 4);
	} else if (pInfo->filetype == MEDIA_FILEFORMAT_MP4) {
		memcpy((CHAR *)pExtName, pMp4, 4); ;
	} else if (pInfo->filetype == NAMERULE_FMT_RAW) {
		memcpy((CHAR *)pExtName, pRaw, 4); ;
	} else if (pInfo->filetype == MEDIA_FILEFORMAT_TS) {
		memcpy((CHAR *)pExtName, pTs, 3); ;
	}

	switch (pInfo->timetype) {
	default:
	case NAMERULECUS_TIMETYPE_1:
		if (gNH_custom_msg) {
			DBG_DUMP("timetype1  (%d)\r\n", pInfo->ymdtype);
		}
		if (pInfo->en_count) {
			if (pInfo->ymdtype == NAMERULECUS_YMDTYPE_MDY) {

				snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN,
						 "%s%02d%02d%04d_%02d%02d%02d_%03d",
						 frontDir,
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec),
						 (int)(ptimeinfo->uiNumber));
			} else if (pInfo->ymdtype == NAMERULECUS_YMDTYPE_DMY) {

				snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN,
						 "%s%02d%02d%04d_%02d%02d%02d_%03d",
						 frontDir,
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec),
						 (int)(ptimeinfo->uiNumber));
			} else { //ymd

				snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN,
						 "%s%04d%02d%02d_%02d%02d%02d_%03d",
						 frontDir,
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec),
						 (int)(ptimeinfo->uiNumber));
			}

		} else {
			if (pInfo->ymdtype == NAMERULECUS_YMDTYPE_MDY) {

				snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN,
						 "%s%02d%02d%04d_%02d%02d%02d",
						 frontDir,
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec));
			} else if (pInfo->ymdtype == NAMERULECUS_YMDTYPE_DMY) {

				snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN,
						 "%s%02d%02d%04d_%02d%02d%02d",
						 frontDir,
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec));
			} else { //ymd

				snprintf(filePath,
						 NMC_TOTALFILEPATH_MAX_LEN, "%s%04d%02d%02d_%02d%02d%02d",
						 frontDir,
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec));
			}
		}
		break;
	case NAMERULECUS_TIMETYPE_2://yyyy_mmdd
		if (gNH_custom_msg) {
			DBG_DUMP("timetype2  (%d)\r\n", pInfo->ymdtype);
		}
		if (pInfo->en_count) {
			if (pInfo->ymdtype == NAMERULECUS_YMDTYPE_MDY) {

				snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN,
						 "%s%02d%02d_%04d_%02d%02d%02d_%03d",
						 frontDir,
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec),
						 (int)(ptimeinfo->uiNumber));
			} else if (pInfo->ymdtype == NAMERULECUS_YMDTYPE_DMY) {

				snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN,
						 "%s%02d%02d_%04d_%02d%02d%02d_%03d",
						 frontDir,
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec),
						 (int)(ptimeinfo->uiNumber));
			} else { //ymd

				snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN,
						 "%s%04d_%02d%02d_%02d%02d%02d_%03d",
						 frontDir,
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec),
						 (int)(ptimeinfo->uiNumber));
			}

		} else {
			if (pInfo->ymdtype == NAMERULECUS_YMDTYPE_MDY) {

				snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN,
						 "%s%02d%02d_%04d_%02d%02d%02d",
						 frontDir,
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec));
			} else if (pInfo->ymdtype == NAMERULECUS_YMDTYPE_DMY) {

				snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN,
						 "%s%02d%02d_%04d_%02d%02d%02d",
						 frontDir,
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec));
			} else { //ymd

				snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN,
						 "%s%04d_%02d%02d_%02d%02d%02d",
						 frontDir,
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec));
			}
		}
		break;
	case NAMERULECUS_TIMETYPE_3:
		ptimeinfo->uiYear -= 2000;
		if (gNH_custom_msg) {
			DBG_DUMP("timetype3  (%d)\r\n", pInfo->ymdtype);
		}
		if (pInfo->en_count) {
			if (pInfo->ymdtype == NAMERULECUS_YMDTYPE_MDY) {

				snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN,
						 "%s%02d-%02d-%02d_%02d%02d%02d_%03d",
						 frontDir,
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec),
						 (int)(ptimeinfo->uiNumber));
			} else if (pInfo->ymdtype == NAMERULECUS_YMDTYPE_DMY) {

				snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN,
						 "%s%02d-%02d-%02d_%02d%02d%02d_%03d",
						 frontDir,
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec),
						 (int)(ptimeinfo->uiNumber));
			} else { //ymd

				snprintf(filePath,
						 NMC_TOTALFILEPATH_MAX_LEN, "%s%02d-%02d-%02d_%02d%02d%02d_%03d",
						 frontDir,
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec),
						 (int)(ptimeinfo->uiNumber));
			}

		} else {
			if (pInfo->ymdtype == NAMERULECUS_YMDTYPE_MDY) {

				snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN,
						 "%s%02d-%02d-%02d_%02d%02d%02d",
						 frontDir,
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec));
			} else if (pInfo->ymdtype == NAMERULECUS_YMDTYPE_DMY) {

				snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN,
						 "%s%02d-%02d-%02d_%02d%02d%02d",
						 frontDir,
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec));
			} else { //ymd

				snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN,
						 "%s%02d-%02d-%02d_%02d%02d%02d",
						 frontDir,
						 (int)(ptimeinfo->uiYear),
						 (int)(ptimeinfo->uiMonth),
						 (int)(ptimeinfo->uiDate),
						 (int)(ptimeinfo->uiHour),
						 (int)(ptimeinfo->uiMin),
						 (int)(ptimeinfo->uiSec));
			}
		}
		break;
	case NAMERULECUS_TIMETYPE_GIVE:
		snprintf(filePath, NMC_TOTALFILEPATH_MAX_LEN, "%s", frontDir);
		if (pInfo->pGiveName) {
			strncat(filePath, pInfo->pGiveName, 40); //A:
		}
		break;
	}
	//(int)(ptimeinfo->uiNumber),
	if (pInfo->en_endchar) {
		strncat(filePath, pInfo->pEndChar, 5); //A:
	}
	strncat(filePath, ".", 1);
	if(pExtName[0]){
		strncat(filePath, &pExtName[0], 3); //A:
	}
	if (gNH_custom_msg) {
		DBG_DUMP("%s\r\n", (char *)filePath);
	}
	return TRUE;
}

void NH_CustomUti_MakePhotoPath(UINT32 pathid, UINT32 filetype, CHAR *pOutputName)
{
	//RTC_TIME newtime;
	//RTC_DATE newdate;
    struct tm Curr_DateTime;
#if (SUPPORTED_HWCLOCK == ENABLE)
    Curr_DateTime = hwclock_get_time(TIME_ID_CURRENT);
#else
	memset(&Curr_DateTime, 0, sizeof(struct tm));
#endif
	NMC_NAMEINFO Nameinfo = {0}; //coverity 44574
	NMC_TIMEINFO Timeinfo = {0};
	UINT32 sectype = NMC_SECONDTYPE_PHOTO;

	//NMC_FULLPATH_INFO FullInfo;
	if (pathid > NMC_MAX_PATH) {
		DBG_ERR("setFolderPath id error %d!\r\n", pathid);
		return;
	} else if (pathid == NMC_MAX_PATH) { //2016/10/11
		DBG_DUMP("pathid=%d equal to MAX_PATH!\r\n", pathid);
		//sometimes, sensor id=2, but pathid = 1, should work correctly
		pathid = 1;
		//return;
	}

	//FullInfo.pathid = pathid;
	memset(gPathNMCTemp3, 0, NMC_TOTALFILEPATH_MAX_LEN);

	NH_Custom_SetWholeDirPath(pathid, pOutputName, sectype);

	{
		//newtime = rtc_getTime();
		Timeinfo.uiHour = Curr_DateTime.tm_hour;
		Timeinfo.uiMin = Curr_DateTime.tm_min;
		Timeinfo.uiSec = Curr_DateTime.tm_sec;
		//newdate =  rtc_getDate();
		Timeinfo.uiYear = Curr_DateTime.tm_year;
		Timeinfo.uiMonth = Curr_DateTime.tm_mon;
		Timeinfo.uiDate = Curr_DateTime.tm_mday;
		Timeinfo.SecValid = 1;
		Timeinfo.ucChar = 0;
		gNHC_timenum += 1; //2016/07/07
		if (gNHC_timenum > NAMEHDL_CUS_TIMENUM_MAX) { //2016/07/07
			gNHC_timenum = 1;
		}
		Timeinfo.uiNumber = gNHC_timenum;//todo: getLast in fileDB
		//FullInfo.ptimeinfo =  &Timeinfo;
	}

	Nameinfo.filetype = filetype;
	Nameinfo.ymdtype  = gNHC_Nameinfo[pathid].ymdtype;
	Nameinfo.timetype = gNHC_Nameinfo[pathid].timetype;
	Nameinfo.en_count = 1;//2017/08/02 default: on gNHC_Nameinfo[pathid].en_count;
	Nameinfo.en_endchar = gNHC_Nameinfo[pathid].en_endchar;
	Nameinfo.pEndChar = gNHC_Nameinfo[pathid].pEndChar;
	Nameinfo.pGiveName  = 0;
	NH_CustomUti_MakeObjPath(&Timeinfo, &Nameinfo, gPathNMCTemp4, pOutputName);
	memcpy(pOutputName, gPathNMCTemp4, NMC_TOTALFILEPATH_MAX_LEN);
	//DBG_DUMP("photo %s!!\r\n", pOutputName);
}






//==============================================================

void NH_Custom_SetInfo(MEDIANAMING_SETINFO_TYPE type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	if (gNH_custom_msg) {
		DBG_DUMP("Set Info type = %d!! p1= %d!\r\n", type, p1);
	}
	switch (type) {
	case MEDIANAMING_SETINFO_WORKINGHDL:
		//gNHCustom_id = (FILEDB_HANDLE)p1;
		break;

	case MEDIANAMING_SETINFO_OPENDEBUGMSG:
		if (p1) {
			gNH_custom_msg = 1;
		} else {
			gNH_custom_msg = 0;
		}
		break;

	case MEDIANAMING_SETINFO_SETFILETYPE:
		gNHC_filetype = p1;
		if (gNH_custom_msg)	{
			DBG_DUMP("gNHC_filetype = %d!\r\n", gNHC_filetype);
		}
		break;

	case MEDIANAMING_SETINFO_SETNOWPATHID:
#if 0
		if (p2) { //emr on
			if (gNHC_activeEMRfilesavetype == NMC_FILESAVE_PART2) {
				DBG_DUMP("^R force to PATH2 \r\n");
				gNHC_activePathid = 1;
			} else {
				gNHC_activePathid = p1;
			}
		} else if (p3) { //only path1 but save card0
			if (gNHC_active2ndfilesavetype == NMC_FILESAVE_PART1) {
				DBG_DUMP("^R force to 1stpath \r\n");
				gNHC_activePathid = 0;
			} else {
				gNHC_activePathid = p1;
			}
		} else {
			gNHC_activePathid = p1;
		}
#endif
		break;
	case MEDIANAMING_SETINFO_SETEMRPATHID:
		gNHC_activeEMRPathid = p1;
		break;
	case MEDIANAMING_SETINFO_SETEMRSAVETYPE:
		gNHC_activeEMRfilesavetype = p1;
		break;
	case MEDIANAMING_SETINFO_SET2NDSAVETYPE:
		gNHC_active2ndfilesavetype = p1;
		break;
	default:
		break;
	}
}

ER NH_Custom_CreateAndGetPath(UINT32 filetype, CHAR *pPath, UINT32 activeid)
{
#if 0
	//UINT32 DirID, FileID, FileFormat=0;
	BOOL cr_exist = TRUE;
	//CHAR *path;
	NMC_TIMEINFO timeinfo;
	//UINT32 copylen;
	CHAR *pFullPath, *pFirst, *pSecond;
	gNHC_filetype = filetype;

	//FileDB_DumpInfo(gNHFileDB_id);
	memset(gPathNMCTemp1, 0, NMC_TOTALFILEPATH_MAX_LEN);

	if (gNHC_ChangeRoot) {
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
	if (1) { //gNH_filedb_msg)
		DBG_DUMP("Uti_Print pDst = %s!!\r\n", pFullPath);
	}


	NH_FileDBUti_MakeObjPath(&timeinfo, filetype, pPath, pFullPath);

	while (cr_exist) {
		if (NH_FileDBUti_Check(pPath) == TRUE) {
			gtimenum += 1;
			timeinfo.uiNumber = gtimenum;
			NH_FileDBUti_MakeObjPath(&timeinfo, filetype, pPath, pFullPath);
		} else {
			if (gNH_filedb_msg) {
				DBG_DUMP("Name %s OK !\r\n", pPath);
			}
			cr_exist = FALSE;
		}

	}
#endif
	if (gNHC_usemethod1) {
		if (gNH_custom_msg) {
			DBG_DUMP("usemethod1 = %s\r\n", gPathNMCTemp1);
		}
		memcpy((char *)pPath, (char *)gPathNMCTemp1, NMC_TOTALFILEPATH_MAX_LEN);
		gNHC_usemethod1 = 0;
	} else if (gNHC_usemethod2) {
		if (gNH_custom_msg) {
			DBG_DUMP("usemethod2 = %s\r\n",  (char *)gPathNMCTemp2);
		}
		memcpy((char *)pPath, (char *)gPathNMCTemp2, NMC_TOTALFILEPATH_MAX_LEN);
		gNHC_usemethod2 = 0;
	} else if (gNHC_filepathMethod == NMC_FILEPATH_METHOD_2) {
		gNHC_activePathid = activeid;
		NH_Custom_BuildFullPath_Auto(&gNHC_Nameinfo[activeid], &gPathNMCTemp2[activeid][0]);

		if (gNH_custom_msg) {
			DBG_DUMP("usemethod2_2 = %s\r\n",  (char *)gPathNMCTemp2);
		}
		memcpy((char *)pPath, (char *)&gPathNMCTemp2[activeid][0], NMC_TOTALFILEPATH_MAX_LEN);
	} else {
		//char temp3[NMC_TOTALFILEPATH_MAX_LEN];
		NH_CustomUti_MakeDefaultPath(activeid, NMC_SECONDTYPE_MOVIE, gNHC_filetype, pPath);

		//memcpy((char *)pPath, (char *)gPathNMCTemp2, CARDV_PATHLEN);
	}
	//memcpy((char *)gPathNMCTemp2, (char *)pPath, CARDV_PATHLEN);

	memcpy((char *)gNHC_LastPath, (char *)pPath, NMC_TOTALFILEPATH_MAX_LEN);

	return E_OK;
}

ER NH_Custom_CreateAndGetPath1(UINT32 filetype, CHAR *pPath)
{
	return NH_Custom_CreateAndGetPath(filetype, pPath, NRCUS_ID_1);
}

ER NH_Custom_CreateAndGetPath2(UINT32 filetype, CHAR *pPath)
{
	return NH_Custom_CreateAndGetPath(filetype, pPath, NRCUS_ID_2);
}

ER NH_Custom_CreateAndGetPath3(UINT32 filetype, CHAR *pPath)
{
	return NH_Custom_CreateAndGetPath(filetype, pPath, NRCUS_ID_3);
}

ER NH_Custom_CreateAndGetPath4(UINT32 filetype, CHAR *pPath)
{
	return NH_Custom_CreateAndGetPath(filetype, pPath, NRCUS_ID_4);
}

ER NH_Custom_CreateAndGetPath5(UINT32 filetype, CHAR *pPath)
{
	return NH_Custom_CreateAndGetPath(filetype, pPath, NRCUS_ID_5);
}

ER NH_Custom_CreateAndGetPath6(UINT32 filetype, CHAR *pPath)
{
	return NH_Custom_CreateAndGetPath(filetype, pPath, NRCUS_ID_6);
}

ER NH_Custom_CreateAndGetPath7(UINT32 filetype, CHAR *pPath)
{
	return NH_Custom_CreateAndGetPath(filetype, pPath, NRCUS_ID_7);
}

ER NH_Custom_CreateAndGetPath8(UINT32 filetype, CHAR *pPath)
{
	return NH_Custom_CreateAndGetPath(filetype, pPath, NRCUS_ID_8);
}
ER NH_Custom_CreateAndGetPath9(UINT32 filetype, CHAR *pPath)
{
	return NH_Custom_CreateAndGetPath(filetype, pPath, NRCUS_ID_9);
}

ER NH_Custom_CreateAndGetPath10(UINT32 filetype, CHAR *pPath)
{
	return NH_Custom_CreateAndGetPath(filetype, pPath, NRCUS_ID_10);
}

ER NH_Custom_CreateAndGetPath11(UINT32 filetype, CHAR *pPath)
{
	return NH_Custom_CreateAndGetPath(filetype, pPath, NRCUS_ID_11);
}

ER NH_Custom_CreateAndGetPath12(UINT32 filetype, CHAR *pPath)
{
	return NH_Custom_CreateAndGetPath(filetype, pPath, NRCUS_ID_12);
}

ER NH_Custom_CreateAndGetPath13(UINT32 filetype, CHAR *pPath)
{
	return NH_Custom_CreateAndGetPath(filetype, pPath, NRCUS_ID_13);
}

ER NH_Custom_CreateAndGetPath14(UINT32 filetype, CHAR *pPath)
{
	return NH_Custom_CreateAndGetPath(filetype, pPath, NRCUS_ID_14);
}

ER NH_Custom_CreateAndGetPath15(UINT32 filetype, CHAR *pPath)
{
	return NH_Custom_CreateAndGetPath(filetype, pPath, NRCUS_ID_15);
}

ER NH_Custom_CreateAndGetPath16(UINT32 filetype, CHAR *pPath)
{
	return NH_Custom_CreateAndGetPath(filetype, pPath, NRCUS_ID_16);
}

static ER NH_Custom_AddPath(CHAR *pPath, UINT32 attrib, UINT32 activeid)
{
	UINT32 num = 0;
	PFILEDB_FILE_ATTR  pfile;
	if (FileDB_AddFileToLast(gNHCustom_id[activeid], pPath) == TRUE) {
		if (gNH_custom_msg) {
			DBG_DUMP("AddPath %s OK!!!\r\n", pPath);
		}
		//DBG_DUMP("^R  Num add =  %d !!!!!\r\n", num);

		//num = FileDB_GetCurrFileIndex(gNHFileDB_id);
		num = FileDB_GetIndexByPath(gNHCustom_id[activeid], pPath);//2014/04/03 fixed
		if (attrib == NAMERULE_ATTRIB_OR_READONLY) {
			pfile = FileDB_SearhFile2(gNHCustom_id[activeid], num);
			if (pfile) { //coverity 43786
				pfile->attrib |= FST_ATTRIB_READONLY;
			}
		}
		gNHC_lastIndex = num;
		//FileDB_DumpInfo(gNHCustom_id);
		return E_OK;
	} else {
		DBG_ERR("AddPath %s fails!!!\r\n", pPath);
		return E_SYS;
	}
}

static ER NH_Custom_AddPath1(CHAR *pPath, UINT32 attrib)
{
	return NH_Custom_AddPath(pPath, attrib, NRCUS_ID_1);
}

static ER NH_Custom_AddPath2(CHAR *pPath, UINT32 attrib)
{
	return NH_Custom_AddPath(pPath, attrib, NRCUS_ID_2);
}

static ER NH_Custom_AddPath3(CHAR *pPath, UINT32 attrib)
{
	return NH_Custom_AddPath(pPath, attrib, NRCUS_ID_3);
}

static ER NH_Custom_AddPath4(CHAR *pPath, UINT32 attrib)
{
	return NH_Custom_AddPath(pPath, attrib, NRCUS_ID_4);
}

static ER NH_Custom_AddPath5(CHAR *pPath, UINT32 attrib)
{
	return NH_Custom_AddPath(pPath, attrib, NRCUS_ID_5);
}

static ER NH_Custom_AddPath6(CHAR *pPath, UINT32 attrib)
{
	return NH_Custom_AddPath(pPath, attrib, NRCUS_ID_6);
}

static ER NH_Custom_AddPath7(CHAR *pPath, UINT32 attrib)
{
	return NH_Custom_AddPath(pPath, attrib, NRCUS_ID_7);
}

static ER NH_Custom_AddPath8(CHAR *pPath, UINT32 attrib)
{
	return NH_Custom_AddPath(pPath, attrib, NRCUS_ID_8);
}
static ER NH_Custom_AddPath9(CHAR *pPath, UINT32 attrib)
{
	return NH_Custom_AddPath(pPath, attrib, NRCUS_ID_9);
}

static ER NH_Custom_AddPath10(CHAR *pPath, UINT32 attrib)
{
	return NH_Custom_AddPath(pPath, attrib, NRCUS_ID_10);
}

static ER NH_Custom_AddPath11(CHAR *pPath, UINT32 attrib)
{
	return NH_Custom_AddPath(pPath, attrib, NRCUS_ID_11);
}

static ER NH_Custom_AddPath12(CHAR *pPath, UINT32 attrib)
{
	return NH_Custom_AddPath(pPath, attrib, NRCUS_ID_12);
}

static ER NH_Custom_AddPath13(CHAR *pPath, UINT32 attrib)
{
	return NH_Custom_AddPath(pPath, attrib, NRCUS_ID_13);
}

static ER NH_Custom_AddPath14(CHAR *pPath, UINT32 attrib)
{
	return NH_Custom_AddPath(pPath, attrib, NRCUS_ID_14);
}

static ER NH_Custom_AddPath15(CHAR *pPath, UINT32 attrib)
{
	return NH_Custom_AddPath(pPath, attrib, NRCUS_ID_15);
}

static ER NH_Custom_AddPath16(CHAR *pPath, UINT32 attrib)
{
	return NH_Custom_AddPath(pPath, attrib, NRCUS_ID_16);
}
static ER NH_Custom_DelPath(CHAR *pPath, UINT32 activeid)
{
	INT32    index;
	//INT32 ret;
	Perf_Mark();
	index = FileDB_GetIndexByPath(gNHCustom_id[activeid], pPath);
	if (index != FILEDB_SEARCH_ERR) {
		FileDB_DeleteFile(gNHCustom_id[activeid], index);
		if (gNH_custom_msg) {
			DBG_IND("DelPath = %s ,index =%d ,delTime = %07d us \r\n", pPath, index, Perf_GetDuration());
		}
	} else {
		DBG_ERR("DelPath = %s ,but search fail!!!!! \r\n", pPath);
	}
	//FileDB_DumpInfo(gNHFileDB_H);//testing

	return E_OK;
}

static ER NH_Custom_DelPath1(CHAR *pPath)
{
	return NH_Custom_DelPath(pPath, NRCUS_ID_1);
}

static ER NH_Custom_DelPath2(CHAR *pPath)
{
	return NH_Custom_DelPath(pPath, NRCUS_ID_2);
}

static ER NH_Custom_DelPath3(CHAR *pPath)
{
	return NH_Custom_DelPath(pPath, NRCUS_ID_3);
}

static ER NH_Custom_DelPath4(CHAR *pPath)
{
	return NH_Custom_DelPath(pPath, NRCUS_ID_4);
}

static ER NH_Custom_DelPath5(CHAR *pPath)
{
	return NH_Custom_DelPath(pPath, NRCUS_ID_5);
}

static ER NH_Custom_DelPath6(CHAR *pPath)
{
	return NH_Custom_DelPath(pPath, NRCUS_ID_6);
}

static ER NH_Custom_DelPath7(CHAR *pPath)
{
	return NH_Custom_DelPath(pPath, NRCUS_ID_7);
}

static ER NH_Custom_DelPath8(CHAR *pPath)
{
	return NH_Custom_DelPath(pPath, NRCUS_ID_8);
}
static ER NH_Custom_DelPath9(CHAR *pPath)
{
	return NH_Custom_DelPath(pPath, NRCUS_ID_9);
}

static ER NH_Custom_DelPath10(CHAR *pPath)
{
	return NH_Custom_DelPath(pPath, NRCUS_ID_10);
}

static ER NH_Custom_DelPath11(CHAR *pPath)
{
	return NH_Custom_DelPath(pPath, NRCUS_ID_11);
}

static ER NH_Custom_DelPath12(CHAR *pPath)
{
	return NH_Custom_DelPath(pPath, NRCUS_ID_12);
}

static ER NH_Custom_DelPath13(CHAR *pPath)
{
	return NH_Custom_DelPath(pPath, NRCUS_ID_13);
}

static ER NH_Custom_DelPath14(CHAR *pPath)
{
	return NH_Custom_DelPath(pPath, NRCUS_ID_14);
}

static ER NH_Custom_DelPath15(CHAR *pPath)
{
	return NH_Custom_DelPath(pPath, NRCUS_ID_15);
}

static ER NH_Custom_DelPath16(CHAR *pPath)
{
	return NH_Custom_DelPath(pPath, NRCUS_ID_16);
}

static ER NH_Custom_GetPathBySeq(UINT32 uiSequ, CHAR *pPath, UINT32 activeid)
{

	FILEDB_FILE_ATTR *pFileAttr;
	//DBG_DUMP("NH_Custom_GetPathBySeq seq=%d!\r\n", uiSequ);
	pFileAttr = FileDB_SearhFile(gNHCustom_id[activeid], uiSequ - 1); //Media count from 1, but FileDB from 0
	if (pFileAttr == NULL) {
		DBG_ERR("GetPathBySeq fails..!!!\r\n");
		return E_SYS;
	}
	if (gNH_custom_msg) {
		DBG_DUMP(" getpathbySeq %s \r\n", pFileAttr->filePath);
	}
	if ((gNR_cus_validMovType & pFileAttr->fileType) == 0) {

		gNHC_lastIndex = uiSequ - 1;
		return E_PAR;
	}
	memcpy((char *)pPath, (char *)pFileAttr->filePath, NMC_TOTALFILEPATH_MAX_LEN);
	gNHC_lastIndex = uiSequ - 1;
	return E_OK;
}

static ER NH_Custom_GetPathBySeq1(UINT32 uiSequ, CHAR *pPath)
{
	return NH_Custom_GetPathBySeq(uiSequ, pPath, NRCUS_ID_1);
}

static ER NH_Custom_GetPathBySeq2(UINT32 uiSequ, CHAR *pPath)
{
	return NH_Custom_GetPathBySeq(uiSequ, pPath, NRCUS_ID_2);
}

static ER NH_Custom_GetPathBySeq3(UINT32 uiSequ, CHAR *pPath)
{
	return NH_Custom_GetPathBySeq(uiSequ, pPath, NRCUS_ID_3);
}

static ER NH_Custom_GetPathBySeq4(UINT32 uiSequ, CHAR *pPath)
{
	return NH_Custom_GetPathBySeq(uiSequ, pPath, NRCUS_ID_4);
}

static ER NH_Custom_GetPathBySeq5(UINT32 uiSequ, CHAR *pPath)
{
	return NH_Custom_GetPathBySeq(uiSequ, pPath, NRCUS_ID_5);
}

static ER NH_Custom_GetPathBySeq6(UINT32 uiSequ, CHAR *pPath)
{
	return NH_Custom_GetPathBySeq(uiSequ, pPath, NRCUS_ID_6);
}

static ER NH_Custom_GetPathBySeq7(UINT32 uiSequ, CHAR *pPath)
{
	return NH_Custom_GetPathBySeq(uiSequ, pPath, NRCUS_ID_7);
}

static ER NH_Custom_GetPathBySeq8(UINT32 uiSequ, CHAR *pPath)
{
	return NH_Custom_GetPathBySeq(uiSequ, pPath, NRCUS_ID_8);
}
static ER NH_Custom_GetPathBySeq9(UINT32 uiSequ, CHAR *pPath)
{
	return NH_Custom_GetPathBySeq(uiSequ, pPath, NRCUS_ID_9);
}

static ER NH_Custom_GetPathBySeq10(UINT32 uiSequ, CHAR *pPath)
{
	return NH_Custom_GetPathBySeq(uiSequ, pPath, NRCUS_ID_10);
}

static ER NH_Custom_GetPathBySeq11(UINT32 uiSequ, CHAR *pPath)
{
	return NH_Custom_GetPathBySeq(uiSequ, pPath, NRCUS_ID_11);
}

static ER NH_Custom_GetPathBySeq12(UINT32 uiSequ, CHAR *pPath)
{
	return NH_Custom_GetPathBySeq(uiSequ, pPath, NRCUS_ID_12);
}

static ER NH_Custom_GetPathBySeq13(UINT32 uiSequ, CHAR *pPath)
{
	return NH_Custom_GetPathBySeq(uiSequ, pPath, NRCUS_ID_13);
}

static ER NH_Custom_GetPathBySeq14(UINT32 uiSequ, CHAR *pPath)
{
	return NH_Custom_GetPathBySeq(uiSequ, pPath, NRCUS_ID_14);
}

static ER NH_Custom_GetPathBySeq15(UINT32 uiSequ, CHAR *pPath)
{
	return NH_Custom_GetPathBySeq(uiSequ, pPath, NRCUS_ID_15);
}

static ER NH_Custom_GetPathBySeq16(UINT32 uiSequ, CHAR *pPath)
{
	return NH_Custom_GetPathBySeq(uiSequ, pPath, NRCUS_ID_16);
}

static ER NH_Custom_GetLastPath(CHAR *pPath)
{
	memcpy((char *)pPath, (char *)gNHC_LastPath, NMC_TOTALFILEPATH_MAX_LEN);
	return E_OK;
}

static ER NH_Custom_customizeFunc(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3, UINT32 activeid)
{
	ER returnV = E_SYS;
	CHAR *pPath;
	UINT64 *p64int = 0;
	UINT32 *p32int = 0, *p33int = 0, pathid, crashtype;
	UINT64 size64;
	BOOL bset;

	switch (type) {
	case MEDIANR_CUSTOM_CHANGECRASH://CAR\MOVIE-> CAR\RO
		if (p1 == 0) {
			DBG_ERR("MEDIANR_CUSTOM_CHANGECRASH NULL\r\n");
			return returnV;
		}
		pPath = (CHAR *)p1;
		//pathid = p2;
		pathid = activeid;
		NH_CustomUti_ChangeFrontPath(pPath, pathid);
		returnV = E_OK;
		break;

	case MEDIANR_CUSTOM_GETFILES://2013/11/08
		p32int = (UINT32 *)p1;
		*p32int = FileDB_GetTotalFileNum(gNHCustom_id[activeid]);
		//DBG_DUMP("^Y [%d][%d]gettotalfiles%d\r\n",gNHC_activePathid,gNHCustom_id[gNHC_activePathid],*p32int );
		returnV = E_OK;
		break;

	case MEDIANR_CUSTOM_GETFILESIZE_BYSEQ: {
			FILEDB_FILE_ATTR *pFileAttr;
			p64int = (UINT64 *)p2;
			p33int = (UINT32 *)p3;
			pFileAttr = FileDB_SearhFile(gNHCustom_id[activeid], p1 - 1); //Media count from 1, but FileDB from 0
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
			pPath = (CHAR *)p1;

			num = FileDB_GetIndexByPath(gNHCustom_id[activeid], pPath);
			pFileAttr = FileDB_SearhFile(gNHCustom_id[activeid], num);


			if (pFileAttr) {
				size64 = (UINT64)p2 ;
				if (p3) {
					size64 = (UINT64)p2 + ((UINT64)p3 << 32);
				}
				bset = FileDB_SetFileSize(gNHCustom_id[activeid], num, size64);
				if (bset == FALSE) {
					DBG_DUMP("^RSetpathFAIL=%s,size=0x%llx!\r\n", pPath, size64);
				} else {
					if (1) { //gNH_custom_msg)
						//DBG_DUMP("activeid=%d\r\n", activeid);
						DBG_DUMP("Setpath=%s,size=0x%llx!\r\n", pPath, size64);
					}
				}
			} else {
				DBG_DUMP("^R pFileNULL!\r\n");
			}
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

			num = FileDB_GetIndexByPath(gNHCustom_id[activeid], pPath);
			pFileAttr = FileDB_SearhFile(gNHCustom_id[activeid], num);
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

			num = FileDB_GetIndexByPath(gNHCustom_id[activeid], pPath);
			pFileAttr = FileDB_SearhFile(gNHCustom_id[activeid], num);
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

			num = FileDB_GetIndexByPath(gNHCustom_id[activeid], pPath);
			pFileAttr = FileDB_SearhFile(gNHCustom_id[activeid], num);

			if (p2 == NAMERULE_ATTRIB_CLR_READONLY) {
				if (pFileAttr) { //coverity 43787
					bset = FileDB_SetFileRO(gNHCustom_id[activeid], num, FALSE);
					if (bset == FALSE) {
						DBG_DUMP("^R Setpathfail=%s, clrRO!\r\n", pPath);
					} else {

						//pFileAttr->attrib &= ~FS_ATTRIB_READ;
						DBG_DUMP("Setpath=%s, clrRO!\r\n", pPath);
					}
				}
			}
			returnV = E_OK;
		}
		break;

	case MEDIANR_CUSTOM_CHANGE_TO_EMR://CAR\MOVIE-> CAR\EMR
		if (p1 == 0) {
			DBG_ERR("MEDIANR_CUSTOM_CHANGE_TO_EMR NULL\r\n");
			return returnV;
		}
		pPath = (CHAR *)p1;
		//pathid = p2;
		pathid = activeid;
		NH_CustomUti_ChangeFrontPath2EMR(pPath, pathid);
		returnV = E_OK;
		break;

	case MEDIANR_CUSTOM_CHANGE_TO_EV://CAR\MOVIE-> CAR\EV
		if (p1 == 0) {
			DBG_ERR("MEDIANR_CUSTOM_CHANGE_TO_EMR NULL\r\n");
			return returnV;
		}
		pPath = (CHAR *)p1;
		crashtype = p2;
		pathid = activeid;
		NH_CustomUti_ChangeFrontPath2EV(pPath, pathid, crashtype);
		returnV = E_OK;
		break;
	default:
		break;
	}

	return returnV;
}

static ER NH_Custom_customizeFunc1(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	return NH_Custom_customizeFunc(type, p1, p2, p3, NRCUS_ID_1);
}

static ER NH_Custom_customizeFunc2(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	return NH_Custom_customizeFunc(type, p1, p2, p3, NRCUS_ID_2);
}

static ER NH_Custom_customizeFunc3(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	return NH_Custom_customizeFunc(type, p1, p2, p3, NRCUS_ID_3);
}

static ER NH_Custom_customizeFunc4(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	return NH_Custom_customizeFunc(type, p1, p2, p3, NRCUS_ID_4);
}

static ER NH_Custom_customizeFunc5(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	return NH_Custom_customizeFunc(type, p1, p2, p3, NRCUS_ID_5);
}

static ER NH_Custom_customizeFunc6(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	return NH_Custom_customizeFunc(type, p1, p2, p3, NRCUS_ID_6);
}

static ER NH_Custom_customizeFunc7(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	return NH_Custom_customizeFunc(type, p1, p2, p3, NRCUS_ID_7);
}

static ER NH_Custom_customizeFunc8(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	return NH_Custom_customizeFunc(type, p1, p2, p3, NRCUS_ID_8);
}
static ER NH_Custom_customizeFunc9(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	return NH_Custom_customizeFunc(type, p1, p2, p3, NRCUS_ID_9);
}

static ER NH_Custom_customizeFunc10(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	return NH_Custom_customizeFunc(type, p1, p2, p3, NRCUS_ID_10);
}

static ER NH_Custom_customizeFunc11(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	return NH_Custom_customizeFunc(type, p1, p2, p3, NRCUS_ID_11);
}

static ER NH_Custom_customizeFunc12(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	return NH_Custom_customizeFunc(type, p1, p2, p3, NRCUS_ID_12);
}

static ER NH_Custom_customizeFunc13(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	return NH_Custom_customizeFunc(type, p1, p2, p3, NRCUS_ID_13);
}

static ER NH_Custom_customizeFunc14(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	return NH_Custom_customizeFunc(type, p1, p2, p3, NRCUS_ID_14);
}

static ER NH_Custom_customizeFunc15(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	return NH_Custom_customizeFunc(type, p1, p2, p3, NRCUS_ID_15);
}

static ER NH_Custom_customizeFunc16(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	return NH_Custom_customizeFunc(type, p1, p2, p3, NRCUS_ID_16);
}
MEDIANAMINGRULE  gNamingCustomHdl_byid[NMC_MAX_PATH] = {
	{
		NH_Custom_CreateAndGetPath1, //CreateAndGetPath
		NH_Custom_AddPath1,          //AddPath
		NH_Custom_DelPath1,          //DelPath
		NH_Custom_GetPathBySeq1,     //GetPathBySeq
		NULL, //NH_FileDB_CalcNextID,//CalcNextID
		NH_Custom_GetLastPath,//GetLastPath
		NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
		NH_Custom_customizeFunc1,
		NH_Custom_SetInfo,        //SetInfo
		NAMEHDL_CUSTOM_CHECKID
	},
	{
		NH_Custom_CreateAndGetPath2, //CreateAndGetPath
		NH_Custom_AddPath2,          //AddPath
		NH_Custom_DelPath2,          //DelPath
		NH_Custom_GetPathBySeq2,     //GetPathBySeq
		NULL, //NH_FileDB_CalcNextID,//CalcNextID
		NH_Custom_GetLastPath,//GetLastPath
		NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
		NH_Custom_customizeFunc2,
		NH_Custom_SetInfo,        //SetInfo
		NAMEHDL_CUSTOM_CHECKID
	},
	{
		NH_Custom_CreateAndGetPath3, //CreateAndGetPath
		NH_Custom_AddPath3,          //AddPath
		NH_Custom_DelPath3,          //DelPath
		NH_Custom_GetPathBySeq3,     //GetPathBySeq
		NULL, //NH_FileDB_CalcNextID,//CalcNextID
		NH_Custom_GetLastPath,//GetLastPath
		NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
		NH_Custom_customizeFunc3,
		NH_Custom_SetInfo,        //SetInfo
		NAMEHDL_CUSTOM_CHECKID
	},
	{
		NH_Custom_CreateAndGetPath4, //CreateAndGetPath
		NH_Custom_AddPath4,          //AddPath
		NH_Custom_DelPath4,          //DelPath
		NH_Custom_GetPathBySeq4,     //GetPathBySeq
		NULL, //NH_FileDB_CalcNextID,//CalcNextID
		NH_Custom_GetLastPath,//GetLastPath
		NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
		NH_Custom_customizeFunc4,
		NH_Custom_SetInfo,        //SetInfo
		NAMEHDL_CUSTOM_CHECKID
	},
	{
		NH_Custom_CreateAndGetPath5, //CreateAndGetPath
		NH_Custom_AddPath5,          //AddPath
		NH_Custom_DelPath5,          //DelPath
		NH_Custom_GetPathBySeq5,     //GetPathBySeq
		NULL, //NH_FileDB_CalcNextID,//CalcNextID
		NH_Custom_GetLastPath,//GetLastPath
		NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
		NH_Custom_customizeFunc5,
		NH_Custom_SetInfo,        //SetInfo
		NAMEHDL_CUSTOM_CHECKID
	},
	{
		NH_Custom_CreateAndGetPath6, //CreateAndGetPath
		NH_Custom_AddPath6,          //AddPath
		NH_Custom_DelPath6,          //DelPath
		NH_Custom_GetPathBySeq6,     //GetPathBySeq
		NULL, //NH_FileDB_CalcNextID,//CalcNextID
		NH_Custom_GetLastPath,//GetLastPath
		NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
		NH_Custom_customizeFunc6,
		NH_Custom_SetInfo,        //SetInfo
		NAMEHDL_CUSTOM_CHECKID
	},
	{
		NH_Custom_CreateAndGetPath7, //CreateAndGetPath
		NH_Custom_AddPath7,          //AddPath
		NH_Custom_DelPath7,          //DelPath
		NH_Custom_GetPathBySeq7,     //GetPathBySeq
		NULL, //NH_FileDB_CalcNextID,//CalcNextID
		NH_Custom_GetLastPath,//GetLastPath
		NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
		NH_Custom_customizeFunc7,
		NH_Custom_SetInfo,        //SetInfo
		NAMEHDL_CUSTOM_CHECKID
	},
	{
		NH_Custom_CreateAndGetPath8, //CreateAndGetPath
		NH_Custom_AddPath8,          //AddPath
		NH_Custom_DelPath8,          //DelPath
		NH_Custom_GetPathBySeq8,     //GetPathBySeq
		NULL, //NH_FileDB_CalcNextID,//CalcNextID
		NH_Custom_GetLastPath,//GetLastPath
		NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
		NH_Custom_customizeFunc8,
		NH_Custom_SetInfo,        //SetInfo
		NAMEHDL_CUSTOM_CHECKID
	},
	{
		NH_Custom_CreateAndGetPath9, //CreateAndGetPath
		NH_Custom_AddPath9,          //AddPath
		NH_Custom_DelPath9,          //DelPath
		NH_Custom_GetPathBySeq9,     //GetPathBySeq
		NULL, //NH_FileDB_CalcNextID,//CalcNextID
		NH_Custom_GetLastPath,//GetLastPath
		NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
		NH_Custom_customizeFunc9,
		NH_Custom_SetInfo,        //SetInfo
		NAMEHDL_CUSTOM_CHECKID
	},
	{
		NH_Custom_CreateAndGetPath10, //CreateAndGetPath
		NH_Custom_AddPath10,          //AddPath
		NH_Custom_DelPath10,          //DelPath
		NH_Custom_GetPathBySeq10,     //GetPathBySeq
		NULL, //NH_FileDB_CalcNextID,//CalcNextID
		NH_Custom_GetLastPath,//GetLastPath
		NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
		NH_Custom_customizeFunc10,
		NH_Custom_SetInfo,        //SetInfo
		NAMEHDL_CUSTOM_CHECKID
	},
	{
		NH_Custom_CreateAndGetPath11, //CreateAndGetPath
		NH_Custom_AddPath11,          //AddPath
		NH_Custom_DelPath11,          //DelPath
		NH_Custom_GetPathBySeq11,     //GetPathBySeq
		NULL, //NH_FileDB_CalcNextID,//CalcNextID
		NH_Custom_GetLastPath,//GetLastPath
		NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
		NH_Custom_customizeFunc11,
		NH_Custom_SetInfo,        //SetInfo
		NAMEHDL_CUSTOM_CHECKID
	},
	{
		NH_Custom_CreateAndGetPath12, //CreateAndGetPath
		NH_Custom_AddPath12,          //AddPath
		NH_Custom_DelPath12,          //DelPath
		NH_Custom_GetPathBySeq12,     //GetPathBySeq
		NULL, //NH_FileDB_CalcNextID,//CalcNextID
		NH_Custom_GetLastPath,//GetLastPath
		NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
		NH_Custom_customizeFunc12,
		NH_Custom_SetInfo,        //SetInfo
		NAMEHDL_CUSTOM_CHECKID
	},
	{
		NH_Custom_CreateAndGetPath13, //CreateAndGetPath
		NH_Custom_AddPath13,          //AddPath
		NH_Custom_DelPath13,          //DelPath
		NH_Custom_GetPathBySeq13,     //GetPathBySeq
		NULL, //NH_FileDB_CalcNextID,//CalcNextID
		NH_Custom_GetLastPath,//GetLastPath
		NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
		NH_Custom_customizeFunc13,
		NH_Custom_SetInfo,        //SetInfo
		NAMEHDL_CUSTOM_CHECKID
	},
	{
		NH_Custom_CreateAndGetPath14, //CreateAndGetPath
		NH_Custom_AddPath14,          //AddPath
		NH_Custom_DelPath14,          //DelPath
		NH_Custom_GetPathBySeq14,     //GetPathBySeq
		NULL, //NH_FileDB_CalcNextID,//CalcNextID
		NH_Custom_GetLastPath,//GetLastPath
		NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
		NH_Custom_customizeFunc14,
		NH_Custom_SetInfo,        //SetInfo
		NAMEHDL_CUSTOM_CHECKID
	},
	{
		NH_Custom_CreateAndGetPath15, //CreateAndGetPath
		NH_Custom_AddPath15,          //AddPath
		NH_Custom_DelPath15,          //DelPath
		NH_Custom_GetPathBySeq15,     //GetPathBySeq
		NULL, //NH_FileDB_CalcNextID,//CalcNextID
		NH_Custom_GetLastPath,//GetLastPath
		NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
		NH_Custom_customizeFunc15,
		NH_Custom_SetInfo,        //SetInfo
		NAMEHDL_CUSTOM_CHECKID
	},
	{
		NH_Custom_CreateAndGetPath16, //CreateAndGetPath
		NH_Custom_AddPath16,          //AddPath
		NH_Custom_DelPath16,          //DelPath
		NH_Custom_GetPathBySeq16,     //GetPathBySeq
		NULL, //NH_FileDB_CalcNextID,//CalcNextID
		NH_Custom_GetLastPath,//GetLastPath
		NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
		NH_Custom_customizeFunc16,
		NH_Custom_SetInfo,        //SetInfo
		NAMEHDL_CUSTOM_CHECKID
	},
};


PMEDIANAMINGRULE NameRule_getCustom(void)
{
	MEDIANAMINGRULE *pHdl;


	pHdl = &gNamingCustomHdl_byid[0];
	//DBG_DUMP("yes CUSTOM!!\r\n");
	return pHdl;
}

PMEDIANAMINGRULE NameRule_getCustom_byid(UINT32 g_nr_id)
{
	return &gNamingCustomHdl_byid[g_nr_id];
}


void NH_Custom_SetFileID(UINT32 uiFileID)
{
	gNHC_timenum = uiFileID;
}

void NH_Custom_OpenMsg(UINT8 on)
{
	gNH_custom_msg = on;
}

void NH_Custom_SetFilepathMethod(UINT32 type)
{
	if ((type) && (type < NMC_FILEPATH_METHOD_MAX)) {
		gNHC_filepathMethod = type;
	}


}

void NH_Custom_MakeDirPath(CHAR *pFirst, CHAR *pSecond, CHAR *pPath, UINT32 pathlen)
{
	UINT32 copylen = 0;
	memset(pPath, 0, pathlen);

	copylen = pathlen;

	strncat(pPath, pFirst, copylen - 1); //A:\CarDV
	copylen = pathlen - strlen(pPath);
	//strncat(pPath, "\\", copylen-1);
	//copylen = pathlen - strlen(pPath);

	strncat(pPath, pSecond, copylen - 1); //A:\CarDV\MOVIE
	copylen = pathlen - strlen(pPath);
	strncat(pPath, "\\", copylen - 1);
}


void NH_Custom_SetFolderPath(NMC_FOLDER_INFO *pInfo)
{
	UINT32 wantI = pInfo->pathid;
	if (wantI >= NMC_MAX_PATH) {
		DBG_ERR("setFolderPath id error %d!\r\n", wantI);
		return;
	}
	char *pChar;
	if (pInfo->pRO) {
		pChar = &gNHC_RO[wantI][0];
		memset(pChar, 0, NMC_OTHERS_MAX_LEN);
		memcpy(pChar, pInfo->pRO, NMC_ROOT_MAX_LEN);
		gNHC_ChangeRO[wantI] = 1;
	}
	if (pInfo->pMovie) {
		pChar = &gNHC_Movie[wantI][0];
		memset(pChar, 0, NMC_OTHERS_MAX_LEN);
		memcpy(pChar, pInfo->pMovie, NMC_ROOT_MAX_LEN);
		gNHC_ChangeMovie[wantI] = 1;
	}
	if (pInfo->pPhoto) {
		pChar = &gNHC_Photo[wantI][0];
		memset(pChar, 0, NMC_OTHERS_MAX_LEN);
		memcpy(pChar, pInfo->pPhoto, NMC_ROOT_MAX_LEN);
		gNHC_ChangePhoto[wantI] = 1;
	}
	if (pInfo->pEMR) {
		pChar = &gNHC_EMR[wantI][0];
		memset(pChar, 0, NMC_OTHERS_MAX_LEN);
		memcpy(pChar, pInfo->pEMR, NMC_ROOT_MAX_LEN);
		gNHC_ChangeEMR[wantI] = 1;
	}
	if (pInfo->pEvent1) {
		pChar = &gNHC_Ev1[wantI][0];
		memset(pChar, 0, NMC_OTHERS_MAX_LEN);
		memcpy(pChar, pInfo->pEvent1, NMC_ROOT_MAX_LEN);
		gNHC_ChangeEv1[wantI] = 1;
	}
	if (pInfo->pEvent2) {
		pChar = &gNHC_Ev2[wantI][0];
		memset(pChar, 0, NMC_OTHERS_MAX_LEN);
		memcpy(pChar, pInfo->pEvent2, NMC_ROOT_MAX_LEN);
		gNHC_ChangeEv2[wantI] = 1;
	}
	if (pInfo->pEvent3) {
		pChar = &gNHC_Ev3[wantI][0];
		memset(pChar, 0, NMC_OTHERS_MAX_LEN);
		memcpy(pChar, pInfo->pEvent3, NMC_ROOT_MAX_LEN);
		gNHC_ChangeEv3[wantI] = 1;
	}

}

void NH_Custom_SetRootPath(CHAR *pPath)
{
	UINT8 i;
	for (i = 0; i < NMC_MAX_PATH; i++) {
		memset(&gNHC_Root[i][0], 0, NMC_ROOT_MAX_LEN);
		memcpy(&gNHC_Root[i][0], pPath, NMC_ROOT_MAX_LEN);
		gNHC_ChangeRoot[i] = 1;
	}
}

void NH_Custom_SetRootPathByPath(UINT32 pathid, CHAR *pPath)
{
	if (pathid >= NMC_MAX_PATH) {
		DBG_ERR("NH_Custom_SetRootPathByPath wrong id %d\r\n", pathid);
		return;
	}
	memset(&gNHC_Root[pathid][0], 0, NMC_ROOT_MAX_LEN);
	memcpy(&gNHC_Root[pathid][0], pPath, NMC_ROOT_MAX_LEN);
	DBG_DUMP("root[%d] %s\r\n", pathid, gNHC_Root[pathid]);
	gNHC_ChangeRoot[pathid] = 1;
}

void NH_Custom_SetFileHandleID(UINT32 value)
{
	gNHCustom_id[0] = value;
}

void NH_Custom_SetFileHandleIDByPath(UINT32 value, UINT32 pathid)
{
	gNHCustom_id[pathid] = value;
}

void NH_Custom_SetWholeDirPath(UINT32 pathid, CHAR *pPath, UINT32 sectype)
{
	CHAR *pFirst, *pSecond;
	if (pathid >= NMC_MAX_PATH) {
		DBG_ERR("setFolderPath id error %d!\r\n", pathid);
		return;
	}

	memset(pPath, 0, NMC_ROOT_MAX_LEN + NMC_OTHERS_MAX_LEN);

	if (gNHC_ChangeRoot[pathid]) {
		pFirst = &gNHC_Root[pathid][0];
	} else {
		pFirst = gNHC_Def_Root;
	}
	//DBG_DUMP("path=%d, root=%s\r\n",pathid, pFirst);
	switch (sectype) {
	case NMC_SECONDTYPE_RO:
		if (gNHC_ChangeRO[pathid]) {
			pSecond = &gNHC_RO[pathid][0];
		} else {
			if (pathid == 0) {
				pSecond = gNHC_Def_RO;
			} else {
				pSecond = gNHC_Def_RORear;
			}
			DBG_DUMP("pSecond= %s!", pSecond);
		}
		break;
	default:
	case NMC_SECONDTYPE_MOVIE:
		if (gNHC_ChangeMovie[pathid]) {
			pSecond = &gNHC_Movie[pathid][0];
		} else {
			if (pathid == 0) {
				pSecond = gNHC_Def_Movie;
			} else {
				pSecond = gNHC_Def_Rear;
			}
			//DBG_DUMP("pSecond= %s!", pSecond);
		}
		break;
	case NMC_SECONDTYPE_PHOTO:
		if (gNHC_ChangePhoto[pathid]) {
			pSecond = &gNHC_Photo[pathid][0];
		} else {
			if (pathid == 0) {
				pSecond = gNHC_Def_Photo1;
			} else {
				pSecond = gNHC_Def_Photo2;
			}
			DBG_DUMP("pSecond= %s!", pSecond);
		}
		break;
	case NMC_SECONDTYPE_EMR:
		if (gNHC_ChangeEMR[pathid]) {
			pSecond = &gNHC_EMR[pathid][0];
		} else {
			pSecond = gNHC_Def_EMR;
		}
		break;

	case NMC_SECONDTYPE_EV1:
		if (gNHC_ChangeEv1[pathid]) {
			pSecond = &gNHC_Ev1[pathid][0];
		} else {
			pSecond = gNHC_Def_Event1;
		}
		break;
	case NMC_SECONDTYPE_EV2:
		if (gNHC_ChangeEv2[pathid]) {
			pSecond = &gNHC_Ev2[pathid][0];
		} else {
			pSecond = gNHC_Def_Event2;
		}
		break;
	case NMC_SECONDTYPE_EV3:
		if (gNHC_ChangeEv3[pathid]) {
			pSecond = &gNHC_Ev3[pathid][0];
		} else {
			pSecond = gNHC_Def_Event3;
		}
		break;
	}

	NH_Custom_MakeDirPath(pFirst, pSecond, pPath, NMC_ROOT_MAX_LEN + NMC_OTHERS_MAX_LEN);
	//NH_FileDBUti_ReplaceStr(gPathCarDVTemp1, gPathCarDVTemp2, oldpath, newpath);
	//DBG_DUMP("newpath=%s\r\n", pPath);
	//memcpy((unsigned char *)pPath, newpath, CARDV_PATHLEN);

}

void NH_Custom_BuildFullPath(NMC_FULLPATH_INFO *pFullInfo, CHAR *pOutputName)
{
	UINT32 id;
	//RTC_TIME newtime;
	//RTC_DATE newdate;
    struct tm Curr_DateTime;
#if (SUPPORTED_HWCLOCK == ENABLE)
    Curr_DateTime = hwclock_get_time(TIME_ID_CURRENT);
#else
	memset(&Curr_DateTime, 0, sizeof(struct tm));
#endif


	char *pSavePath;

	if (pFullInfo->pathid >= NMC_MAX_PATH) {
		DBG_ERR("setFolderPath id error %d!\r\n", pFullInfo->pathid);
		return;
	}
	//create without no fileid, so use [0]
	//pSavePath = &gPathNMCTemp2[pFullInfo->pathid][0];
	pSavePath = &gPathNMCTemp2[0][0];
	memset(pSavePath, 0, NMC_TOTALFILEPATH_MAX_LEN);

	NH_Custom_SetWholeDirPath(pFullInfo->pathid, pOutputName, pFullInfo->secondType);
	//flen = strlen(pOutputName);

	if (pFullInfo->ptimeinfo == 0) {
		//newtime = rtc_getTime();
		gnmc_timeinfo.uiHour = Curr_DateTime.tm_hour;
		gnmc_timeinfo.uiMin = Curr_DateTime.tm_min;
		gnmc_timeinfo.uiSec = Curr_DateTime.tm_sec;
		//newdate =  rtc_getDate();
		gnmc_timeinfo.uiYear = Curr_DateTime.tm_year;
		gnmc_timeinfo.uiMonth = Curr_DateTime.tm_mon;
		gnmc_timeinfo.uiDate = Curr_DateTime.tm_mday;
		gnmc_timeinfo.SecValid = 1;
		gnmc_timeinfo.ucChar = 0;
		gNHC_timenum += 1;
		if (gNHC_timenum > NAMEHDL_CUS_TIMENUM_MAX) { //2016/07/07
			gNHC_timenum = 1;
		}

		gnmc_timeinfo.uiNumber = gNHC_timenum;//todo: getLast in fileDB
		pFullInfo->ptimeinfo =  &gnmc_timeinfo;
	}
	id = pFullInfo->pathid;
	gNHC_Nameinfo[id].ymdtype  = pFullInfo->pnameinfo->ymdtype;
	gNHC_Nameinfo[id].timetype = pFullInfo->pnameinfo->timetype;
	gNHC_Nameinfo[id].en_count = pFullInfo->pnameinfo->en_count;
	gNHC_Nameinfo[id].en_endchar = pFullInfo->pnameinfo->en_endchar;
	memset(gNHC_TempEnd[id], 0, NMC_OTHERS_MAX_LEN);
	memcpy(gNHC_TempEnd[id], pFullInfo->pnameinfo->pEndChar, NMC_OTHERS_MAX_LEN);
	gNHC_Nameinfo[id].pEndChar = gNHC_TempEnd[id];

	NH_CustomUti_MakeObjPath(pFullInfo->ptimeinfo, pFullInfo->pnameinfo, pSavePath, pOutputName);
	memcpy(pOutputName, pSavePath, NMC_TOTALFILEPATH_MAX_LEN);
	//DBG_DUMP("gPathNMCTemp2[0] = %s\r\n", gPathNMCTemp2);
	gNHC_usemethod2 = 1;
}

void NH_Custom_BuildFullPath_Auto(NMC_NAMEINFO *pNameInfo, CHAR *pOutputName)
{
	UINT32 id;
	//RTC_TIME newtime;
	//RTC_DATE newdate;
    struct tm Curr_DateTime;
#if (SUPPORTED_HWCLOCK == ENABLE)
    Curr_DateTime = hwclock_get_time(TIME_ID_CURRENT);
#else
	memset(&Curr_DateTime, 0, sizeof(struct tm));
#endif

	char *pSavePath;

	id = gNHC_activePathid;
	if (id >= NMC_MAX_PATH) {
		DBG_ERR("active id error %d!\r\n", id);
		return;
	}
	//create without no fileid, so use [0]
	//pSavePath = &gPathNMCTemp2[pFullInfo->pathid][0];
	pSavePath = &gPathNMCTemp5[0];//2016/04/13 use temp3 for temp2 used by others
	memset(pSavePath, 0, NMC_TOTALFILEPATH_MAX_LEN);

	NH_Custom_SetWholeDirPath(id, pOutputName, NMC_SECONDTYPE_MOVIE);
	//flen = strlen(pOutputName);

	{
		//newtime = rtc_getTime();
		gnmc_timeinfo.uiHour = Curr_DateTime.tm_hour;
		gnmc_timeinfo.uiMin = Curr_DateTime.tm_min;
		gnmc_timeinfo.uiSec = Curr_DateTime.tm_sec;
		//newdate =  rtc_getDate();
		gnmc_timeinfo.uiYear = Curr_DateTime.tm_year;
		gnmc_timeinfo.uiMonth = Curr_DateTime.tm_mon;
		gnmc_timeinfo.uiDate = Curr_DateTime.tm_mday;
		gnmc_timeinfo.SecValid = 1;
		gnmc_timeinfo.ucChar = 0;
		gNHC_timenum += 1;
		if (gNHC_timenum > NAMEHDL_CUS_TIMENUM_MAX) { //2016/07/07
			gNHC_timenum = 1;
		}

		gnmc_timeinfo.uiNumber = gNHC_timenum;//todo: getLast in fileDB
	}
	pNameInfo->filetype = gNHC_filetype;

	NH_CustomUti_MakeObjPath(&gnmc_timeinfo, pNameInfo, pSavePath, pOutputName);
	memcpy(pOutputName, pSavePath, NMC_TOTALFILEPATH_MAX_LEN);
	//DBG_DUMP("gPathNMCTemp2[0] = %s\r\n", gPathNMCTemp2);
}

void NH_Custom_BuildFullPath_GiveFileName(UINT32 pathid, UINT32 sectype, CHAR *pFileName, CHAR *pOutputName)
{
	NMC_NAMEINFO Nameinfo = {0};
	NMC_TIMEINFO Timeinfo = {0};
	if (pathid >= NMC_MAX_PATH) {
		DBG_ERR("setFolderPath id error %d!\r\n", pathid);
		return;
	}


	NH_Custom_SetWholeDirPath(pathid, pOutputName, sectype);

	Nameinfo.filetype = gNHC_filetype;
	Nameinfo.ymdtype  = NAMERULECUS_YMDTYPE_YMD;
	Nameinfo.timetype = NAMERULECUS_TIMETYPE_GIVE;
	Nameinfo.en_count = 0;
	Nameinfo.en_endchar = 0;
	Nameinfo.pEndChar = 0;
	Nameinfo.pGiveName = pFileName;

	memset(gPathNMCTemp1, 0, NMC_TOTALFILEPATH_MAX_LEN);
	NH_CustomUti_MakeObjPath(&Timeinfo, &Nameinfo, gPathNMCTemp1, pOutputName);
	memcpy(pOutputName, gPathNMCTemp1, NMC_TOTALFILEPATH_MAX_LEN);

	gNHC_usemethod1 = 1;
}

void NH_Custom_BuildFullPath_GiveFileNameWithFiletype(UINT32 pathid, UINT32 sectype, UINT32 filetype, CHAR *pFileName, CHAR *pOutputName)
{
	UINT32 oldtype = gNHC_filetype;
	gNHC_filetype = filetype;
	NH_Custom_BuildFullPath_GiveFileName(pathid, sectype, pFileName,  pOutputName);
	gNHC_filetype = oldtype;
}

void NH_Custom_SetDefaultFiletype(UINT32 filetype)
{
	gNHC_filetype = filetype;
}

void NH_CustomUti_MakeDefaultPath(UINT32 pathid, UINT32 sectype, UINT32 filetype, CHAR *pOutputName)
{
	//RTC_TIME newtime;
	//RTC_DATE newdate;
    struct tm Curr_DateTime;
#if (SUPPORTED_HWCLOCK == ENABLE)
    Curr_DateTime = hwclock_get_time(TIME_ID_CURRENT);
#else
	memset(&Curr_DateTime, 0, sizeof(struct tm));
#endif

	NMC_NAMEINFO Nameinfo;
	NMC_TIMEINFO Timeinfo;
	char tempEnd[] = {"A\0"};
	char tempEnd2[] = {"B\0"};

	//NMC_FULLPATH_INFO FullInfo;
	if (pathid >= NMC_MAX_PATH) {
		DBG_ERR("setFolderPath id error %d!\r\n", pathid);
		return;
	}
	//FullInfo.pathid = pathid;
	memset(gPathNMCTemp3, 0, NMC_TOTALFILEPATH_MAX_LEN);

	NH_Custom_SetWholeDirPath(pathid, pOutputName, sectype);

	{
		//newtime = rtc_getTime();
		Timeinfo.uiHour = Curr_DateTime.tm_hour;
		Timeinfo.uiMin = Curr_DateTime.tm_min;
		Timeinfo.uiSec = Curr_DateTime.tm_sec;
		//newdate =  rtc_getDate();
		Timeinfo.uiYear = Curr_DateTime.tm_year;
		Timeinfo.uiMonth = Curr_DateTime.tm_mon;
		Timeinfo.uiDate = Curr_DateTime.tm_mday;
		Timeinfo.SecValid = 1;
		Timeinfo.ucChar = 0;
		gNHC_timenum += 1;
		if (gNHC_timenum > NAMEHDL_CUS_TIMENUM_MAX) { //2016/07/07
			gNHC_timenum = 1;
		}

		Timeinfo.uiNumber = gNHC_timenum;//todo: getLast in fileDB
		//FullInfo.ptimeinfo =  &Timeinfo;
	}

	Nameinfo.filetype = filetype;
	Nameinfo.ymdtype  = NAMERULECUS_YMDTYPE_YMD;
	Nameinfo.timetype = NAMERULECUS_TIMETYPE_1;
	Nameinfo.en_count = 1;
	Nameinfo.en_endchar = 1;
	if (pathid == 0) {
		Nameinfo.pEndChar = tempEnd;
	} else {
		Nameinfo.pEndChar = tempEnd2;
	}
	NH_CustomUti_MakeObjPath(&Timeinfo, &Nameinfo, gPathNMCTemp3, pOutputName);
	memcpy(pOutputName, gPathNMCTemp3, NMC_TOTALFILEPATH_MAX_LEN);
	//DBG_DUMP("method3 %s!!\r\n", pOutputName);

	gNHC_Nameinfo[pathid].ymdtype  = Nameinfo.ymdtype;
	gNHC_Nameinfo[pathid].timetype = Nameinfo.timetype;
	gNHC_Nameinfo[pathid].en_count = Nameinfo.en_count;
	gNHC_Nameinfo[pathid].en_endchar = Nameinfo.en_endchar;
	memset(gNHC_TempEnd[pathid], 0, NMC_OTHERS_MAX_LEN);
	//covertiy 43896
	strncpy((&gNHC_TempEnd[pathid][0]), Nameinfo.pEndChar, 2);//coverity
	gNHC_Nameinfo[pathid].pEndChar = gNHC_TempEnd[pathid];


}

static BOOL NH_CustomUti_ReplaceStr(CHAR *oldfront, CHAR *newfront, CHAR *oldPath, CHAR *newPath)
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


	if (strstr(oldPath, oldfront) == 0) {
		DBG_DUMP("not old, wrong!!");
		return 1;
	}
	{
		memcpy(newPath, newfront, newlen);
		if (gNH_custom_msg) {
			DBG_DUMP("Rep:newPath1 = %s!\r\n", newPath);
		}
		memcpy(newPath + (newlen - 1), oldPath + (oldlen - 1), oldalllen - oldlen + 1);
		if (gNH_custom_msg) {
			DBG_DUMP("Rep:newPath2 = %s!\r\n", newPath);
		}
	}

	return 0;
}

ER NH_CustomUti_ChangeFrontPath(CHAR *pPath, UINT32 pathid)
{
	//if (pathid == 0)
	{
		return NH_CustomUti_ChangeFrontPath2nd(NMC_SECONDTYPE_MOVIE, NMC_SECONDTYPE_RO, pPath, pathid);
	}
	//else
	//{
	//    return NH_CustomUti_ChangeFrontPath2nd(NMC_SECONDTYPE_REAR, NMC_SECONDTYPE_RO, pPath, pathid);
	//}
}

ER NH_CustomUti_ChangeFrontPath2EMR(CHAR *pPath, UINT32 pathid)
{
	//if (pathid == 0)
	{
		return NH_CustomUti_ChangeFrontPath2nd(NMC_SECONDTYPE_MOVIE, NMC_SECONDTYPE_EMR, pPath, pathid);
	}
	//else
	//{
	//    return NH_CustomUti_ChangeFrontPath2nd(NMC_SECONDTYPE_REAR, NMC_SECONDTYPE_EMR, pPath, pathid);
	//}
}

ER NH_CustomUti_ChangeFrontPath2EV(CHAR *pPath, UINT32 pathid, UINT32 sectype)
{
	//if (pathid == 0)
	{
		return NH_CustomUti_ChangeFrontPath2nd(NMC_SECONDTYPE_MOVIE, sectype, pPath, pathid);
	}
	//else
	//{
	//    return NH_CustomUti_ChangeFrontPath2nd(NMC_SECONDTYPE_REAR, sectype, pPath, pathid);
	//}
}

ER NH_CustomUti_ChangeFrontPath2nd(UINT32 oldtype, UINT32 newtype, CHAR *pPath, UINT32 pathid)
{
	CHAR newpath[NMC_TOTALFILEPATH_MAX_LEN];
	CHAR oldpath[NMC_TOTALFILEPATH_MAX_LEN];
	CHAR temp1[NMC_TOTALFILEPATH_MAX_LEN];
	CHAR temp2[NMC_TOTALFILEPATH_MAX_LEN];
	memcpy(oldpath, pPath, NMC_TOTALFILEPATH_MAX_LEN);
	if (gNH_custom_msg) {
		DBG_DUMP("Change:oldpath=%s\r\n", oldpath);
	}
	memset(newpath, 0, NMC_TOTALFILEPATH_MAX_LEN);


	NH_Custom_SetWholeDirPath(pathid, temp1, oldtype);
	//DBG_DUMP("olddir=%s\r\n", temp1);
	NH_Custom_SetWholeDirPath(pathid, temp2, newtype);
	//DBG_DUMP("newdir=%s\r\n", temp2);

	if (NH_CustomUti_ReplaceStr(temp1, temp2, oldpath, newpath) == 0) {
		if (gNH_custom_msg) {
			DBG_DUMP("Change:newpath=%s\r\n", newpath);
		}
		memcpy((unsigned char *)pPath, newpath, NMC_TOTALFILEPATH_MAX_LEN);
	} else {
		return E_SYS;
	}
	return E_OK;
}

void NH_Custom_SetMethod2Param(NMC_METHOD2_INFO *pInfo)
{
	UINT32 id;
	id = pInfo->pathid;
	gNHC_Nameinfo[id].timetype = pInfo->timetype;
	gNHC_Nameinfo[id].ymdtype  = pInfo->ymdtype;

	gNHC_Nameinfo[id].en_count = pInfo->en_count;
	gNHC_Nameinfo[id].en_endchar = pInfo->en_endChar;
	memset(gNHC_TempEnd[id], 0, NMC_OTHERS_MAX_LEN);
	memcpy(gNHC_TempEnd[id], pInfo->pEndChar, NMC_OTHERS_MAX_LEN);
	gNHC_Nameinfo[id].pEndChar = gNHC_TempEnd[id];
}



VOS_MODULE_VERSION(NameRuleCustom, 1, 00, 016, 00)

