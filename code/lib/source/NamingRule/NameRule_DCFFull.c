/*
    Media Plugin - Naming Rule DCFFull.

    This file is the media plugin library - Naming Rule DCFFull.

    @file       NameRule_DCFFull.c
    @ingroup    mIMPENC
    @note       1st version.
    @version    V1.01.0001
    @date       2013/02/26

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/
#include "avfile/MediaWriteLib.h"
#include "avfile/MediaReadLib.h"
#include <stdio.h>
#include <string.h>
#include "kwrap/error_no.h"
#include "kwrap/type.h"
//#include "kernel.h"
#include "DCF.h"
#include "NamingRule/NameRule_DCFFull.h"

#define __MODULE__          NameRuleDCFFull
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include <kwrap/debug.h>

////===================================================================start
#define NAMERULE_DCFFULL_CHECKID  0x44434646 ///< 'DCFF' as checkID
#define DCF_FULL_FILE_PATH_LEN       30     ///<  DCF full file path legnth, ex: "A:\DCIM\100NVTIM\IMAG0001.JPG" & '\0'

static CHAR gpDCFFSname[DCF_FULL_FILE_PATH_LEN];

UINT32 gMediaDCFDid = 0, gMediaDCFFid = 0, gMediaDCFType = 0;
UINT8  gMediaDCFCrashSameDid = 0, gDCFFullMsg = 0;
UINT32 gMediaDCF1stCrashDid = 0, gMediaDCF1stCrashFid = 0;
char       gMediaDCFCrashFoldername[7] = "CRASH\0";//2011/09/15
UINT32 gMediaNewDidAfterCrash = 0, gMediaNewFidAfterCrash = 0;

static void NameRuleDCFFullUti_SetSameCrashID(UINT8 yesOrNot)
{
	gMediaDCFCrashSameDid = yesOrNot;

}

static void NameRuleDCFFullUti_Set1stCrashID(void)
{
	if (gMediaDCFCrashSameDid) { //using old
		if (gMediaDCF1stCrashDid == 0) {
			if (gMediaDCFDid != 999) {
				gMediaDCF1stCrashDid = gMediaDCFDid + 1;
				gMediaDCF1stCrashFid = gMediaDCFFid;
				if (gDCFFullMsg) {
					DBG_IND("1st did=%d, fid=%d!\r\n", gMediaDCF1stCrashDid, gMediaDCF1stCrashFid);
				}
			} else {
				// gMediaDCF1stCrashDid = 0; invalid crash folder
				DBG_ERR("set 1st crash ID FAILS!!! now already 999 !!!\r\n");
			}
		} else {
			gMediaDCF1stCrashFid = gMediaDCFFid;
			if (gDCFFullMsg) {

				DBG_IND("1st2 did=%d, fid=%d!\r\n", gMediaDCF1stCrashDid, gMediaDCF1stCrashFid);
			}
		}
	} else { //next be crash folder
		if (gMediaDCFDid != 999) {
			gMediaDCF1stCrashDid = gMediaDCFDid + 1;
			gMediaDCF1stCrashFid = gMediaDCFFid;
		} else {
			// gMediaDCF1stCrashDid = 0; invalid crash folder
			DBG_ERR("set crash ID FAILS!!! now already 999 !!!\r\n");
		}
	}
}

//static void NameRuleDCFFullUti_SetCrashFolderName(char *pChar)
//{
//    memcpy(gMediaDCFCrashFoldername, pChar, 5);
//    if (gDCFFullMsg)
//    {

//        DBG_DUMP("NEW CRASH FOLDER NAME = %s!!\r\n", gMediaDCFCrashFoldername);
//    }
//}

static ER NameRuleDCFFullUti_SetNextID(UINT32 DirID, UINT32 FileID, BOOL bCreateFolder)
{
	CHAR filePath[DCF_FULL_FILE_PATH_LEN];
	BOOL ret;
	INT32 fsret;
	if (gDCFFullMsg) {

		DBG_IND("MediaFS_SetNextID did=%d,fid=%d!\r\n", DirID, FileID);
	}
	ret = DCF_SetNextID(DirID, FileID);
	if (ret == FALSE) {
		DBG_ERR("[REC]DCF setNextID FAILS, did=%d! fid=%d!\r\n", DirID, FileID);
		return E_SYS;
	}
	if (bCreateFolder) {
		//DCF_MakeObjPath(UINT32 DirID, UINT32 FileID, UINT32 fileType, CHAR * filePath);

		DBG_ERR("[REC]create new Folder!");
		ret = DCF_MakeDirPath(DirID, filePath); //2012/11/01 Meg
		if (ret) {
			fsret = FileSys_MakeDir(filePath);
			if (fsret) {
				DBG_ERR("[REC]FS make DIR FAILS, path=%s!\r\n", filePath);
				return E_SYS;
			} else {
				return E_OK;
			}
		} else {
			DBG_ERR("[REC]DCF get folderpath FAILS 2, dirID=%d!\r\n", DirID);
			return E_SYS;
		}
	}
	return E_OK;
}


static ER NameRuleDCFFull_CreateAndGetPath(UINT32 filetype, CHAR *pPath)
{
	UINT32 DirID = 0, FileID = 0, FileFormat = 0; //coverity 44575
	BOOL ret;//, set1;
	CHAR *path;
	if (filetype == MEDIA_FILEFORMAT_AVI) {
		FileFormat = DCF_FILE_TYPE_AVI;
	} else if (filetype == MEDIA_FILEFORMAT_MOV) {
		FileFormat = DCF_FILE_TYPE_MOV;
	} else if (filetype == MEDIA_FILEFORMAT_MP4) {
		FileFormat = DCF_FILE_TYPE_MP4;
	}

	ret = DCF_GetNextID(&DirID, &FileID);

	path = gpDCFFSname;
	if (ret) {
		DCF_MakeObjPath(DirID, FileID, FileFormat, path);
		gMediaDCFDid = DirID;
		gMediaDCFFid = FileID;
		gMediaDCFType = FileFormat;
		if ((gMediaDCF1stCrashDid == 0) || (gMediaDCFCrashSameDid == 0)) {
			if (gMediaDCFDid == 999) {
				gMediaDCF1stCrashDid = 100;
			} else {
				gMediaDCF1stCrashDid = gMediaDCFDid + 1;
			}
		}
		//else
		//{
		//    no need to update gMediaDCF1stCrashDid
		//}
		memcpy((unsigned char *)pPath, path, DCF_FULL_FILE_PATH_LEN);
		//DBG_DUMP("[REC]DCF GetNextid OK %s!\r\n",pPath);

		if (FileID < 999) { //add for multi-recording
			//set1= DCF_SetNextID(DirID, FileID+1);
			DCF_SetNextID(DirID, FileID + 1);
			//DBG_DUMP("set1=0x%lx newFileID=%d\r\n", set1, FileID+1);
		} else {
			//set1= DCF_SetNextID(DirID+1,1);
			DCF_SetNextID(DirID + 1, 1);
		}
		//DBG_DUMP("gMediaDCF1stCrashDid11=%d\r\n", gMediaDCF1stCrashDid);

		return E_OK;
	} else {
		DBG_ERR("[REC]DCF GetNextid Fails..did=%d, fid=%d!\r\n", DirID, FileID);
		return E_SYS;
	}

}
static ER NameRuleDCFFull_AddPath(CHAR *pPath, UINT32 attrib)
{
	if (DCF_AddDBfile(pPath) == TRUE) {
		if (gDCFFullMsg) {
			DBG_IND("AddPath %s OK!!!\r\n", pPath);
		}
		return E_OK;

	} else {
		DBG_ERR("AddPath %s fails!!!\r\n", pPath);
		return E_SYS;
	}
}

static ER NameRuleDCFFull_DelPath(CHAR *pPath)
{
	if (DCF_DelDBfile(pPath) == TRUE) {
		if (gDCFFullMsg) {
			DBG_DUMP("DelPath %s OK!!!\r\n", pPath);
		}
		return E_OK;

	} else {
		DBG_ERR("DelPath %s fails!!!\r\n", pPath);
		return E_SYS;
	}
}

static ER NameRuleDCFFull_GetPathBySeq(UINT32 uiSequ, CHAR *pPath)
{
	UINT32 did, fid, type;
	BOOL ret, getfilepath;
	if (gDCFFullMsg) {

		DBG_IND("GetPathBySeq id = %d!!\r\n", uiSequ);
	}

	ret = DCF_GetObjInfo(uiSequ, &did, &fid, &type);
	if (ret == TRUE) {
		if (gDCFFullMsg) {

			DBG_IND("[REC]DCF GetidBySeq OK..seq=%d, dirID = %d, fileID = %d!%d!\r\n", uiSequ, did, fid, type);
		}
		getfilepath = DCF_GetObjPathByID(did, fid, type, pPath);
		if (getfilepath == FALSE) {
			DBG_ERR("[REC]DCF path Fails..did=%d, fid = %d, type = %d!\r\n", did, fid, type);
			return E_SYS;
		}
	} else {
		DBG_ERR("[REC]DCF info Fails..seq=%d!\r\n", uiSequ);
		return E_SYS;
	}
	if (gDCFFullMsg) {

		DBG_IND("[GetPathbySeq] name = %s!\r\n", pPath);
	}
	return E_OK;
}

static ER NameRuleDCFFull_CalcNextID(void)
{

	//UINT32 nowFid, nowDid;
	//UINT32 newFid;


	if (gMediaDCFCrashSameDid) {
		if (gMediaDCF1stCrashDid == (gMediaDCFDid + 1)) { //the first crash
			if (gMediaDCF1stCrashDid < 999) {
				NameRuleDCFFullUti_SetNextID(gMediaDCF1stCrashDid + 1, gMediaDCF1stCrashFid + 1, TRUE);
			} else {
				DBG_ERR("no folder lasting...1111\r\n");
				return E_SYS;
			}
		} else { //the second crash
			//go back folder, filenum +1
			NameRuleDCFFullUti_SetNextID(gMediaDCFDid, gMediaDCFFid + 1, FALSE);

			//in order to avoid many empty folders
			//just continue, do not change folder , 2011/09/16 Meg
			//FilesysSetWriteNextFileId(nowDid+1, newFid, TRUE);
		}

	} else {
		if (gMediaDCF1stCrashDid < 999) {
			NameRuleDCFFullUti_SetNextID(gMediaDCF1stCrashDid + 1, 1, TRUE);
		} else {
			DBG_ERR("no folder lasting...2222\r\n");
			return E_SYS;
		}

	}
	return E_OK;
}


static ER NameRuleDCFFull_GetLastPath(CHAR *pPath)
{
	UINT32 FileFormat = 0;
	//BOOL ret;
	CHAR *path;
	FileFormat = gMediaDCFType;
	path = gpDCFFSname;

	DCF_MakeObjPath(gMediaDCFDid, gMediaDCFFid, FileFormat, path);
	memcpy((unsigned char *)pPath, path, DCF_FULL_FILE_PATH_LEN);
	if (gDCFFullMsg) {

		DBG_IND("LastPath = %s!!!\r\n", pPath);
	}
	return E_OK;
}

static ER NameRuleDCFFull_GetNewCrashPath(CHAR *pPath, UINT8 setActive)
{
	UINT32 FileFormat = 0;
	CHAR *path;

	//char newFolder[] = "A:\\DCIM\\%03dCRASH\\IMAG%04d.AVI\0";
	// 012 34567 890 123456 789012345678

	FileFormat = gMediaDCFType;
	path = gpDCFFSname;

	DCF_MakeObjPath(gMediaDCF1stCrashDid, gMediaDCF1stCrashFid, FileFormat, path);

	memcpy((unsigned char *)path + 11, gMediaDCFCrashFoldername, 5);
	//DBG_DUMP("NewCrashPath11 = %s!!!\r\n", path);
	memcpy((unsigned char *)pPath, path, DCF_FULL_FILE_PATH_LEN);
	if (gDCFFullMsg) {

		DBG_IND("NewCrashPath22 = %s!!!\r\n", pPath);
	}
	//if (setActive)
	//{
	//    gMediaDCFDid = gMediaDCF1stCrashDid;
	//    gMediaDCFFid = gMediaDCF1stCrashFid;
	//}
	return E_OK;
}


static void NameRuleDCFFull_SetInfo(MEDIANAMING_SETINFO_TYPE type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	if (gDCFFullMsg) {

		DBG_IND("Set Info type = %d!! p1= %d!\r\n", type, p1);
	}
	switch (type) {
	case MEDIANAMING_SETINFO_SAMECRASHFOLDER:
		NameRuleDCFFullUti_SetSameCrashID(p1);
		break;
	case MEDIANAMING_SETINFO_SET1STCRASH:
		NameRuleDCFFullUti_Set1stCrashID();
		break;
	//case MEDIANAMING_SETINFO_SETCRASHNAME:
	//    NameRuleDCFFullUti_SetCrashFolderName((char *)p1);
	//    break;

	default:
		break;
	}
}

void NameRuleDCFFull_OpenDumpMsg(UINT8 on)
{
	gDCFFullMsg = on;
}

static ER NameRuleDCFFullUti_ChangeFrontPath(CHAR *pPath)//2013/09/27 Meg
{


	UINT32 DirID;
	CHAR *path;
	//char newFolder[] = "A:\\DCIM\\%03dCRASH\\IMAG%04d.AVI\0";
	// 012 34567 890 123456 789012345678
	UINT32 oldFileid;
	UINT32 a, b, c, d;
	a = pPath[24] - '0';
	b = pPath[23] - '0';
	c = pPath[22] - '0';
	d = pPath[21] - '0';
	oldFileid = d * 1000 + c * 100 + b * 10 + a;
	a = pPath[10] - '0';
	b = pPath[9] - '0';
	c = pPath[8] - '0';
	DirID = c * 100 + b * 10 + a;

	DBG_IND("gMediaDCF1stCrashDid22=%d\r\n", gMediaDCF1stCrashDid);
	path = gpDCFFSname;

	{
		DCF_MakeObjPath(gMediaDCF1stCrashDid, oldFileid, gMediaDCFType, path);
		memcpy((unsigned char *)pPath, path, DCF_FULL_FILE_PATH_LEN);
		//DBG_DUMP("[REC]DCF GetNextid OK %s!\r\n",pPath);

		if (oldFileid < 999) { //add for multi-recording
			if (gMediaDCF1stCrashDid > DirID) {
				gMediaNewDidAfterCrash = gMediaDCF1stCrashDid + 1;
				gMediaNewFidAfterCrash = oldFileid + 1;
			} else { //same crash, old =116, 1st =115=> next 117
				gMediaNewDidAfterCrash = DirID + 1;
				gMediaNewFidAfterCrash = oldFileid + 1;
			}
			//DBG_DUMP("set1=0x%lx newFileID=%d\r\n", set1, FileID+1);
		} else {
			if (gMediaDCF1stCrashDid > DirID) {
				gMediaNewDidAfterCrash = gMediaDCF1stCrashDid + 1;
				gMediaNewFidAfterCrash = 1;

			} else {
				gMediaNewDidAfterCrash = DirID + 1;
				gMediaNewFidAfterCrash = 1;
			}
		}
		return E_OK;
	}
	DBG_IND("next =%d, %d!\r\n", gMediaNewDidAfterCrash, gMediaNewFidAfterCrash);
	DBG_IND("newpath=%s\r\n", pPath);
	//memcpy((unsigned char *)pPath, newpath, CARDV_PATHLEN);
	return E_OK;
}

static ER NameRuleDCFFull_customizeFunc(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	ER returnV = E_SYS;
	CHAR *pPath;
	UINT32 *p32int;
	UINT32 uiSequ, did, fid, filetype = gMediaDCFType;
	BOOL ret, getfilepath;
	UINT64 filesize;
	INT32 get;
	UINT8 attrib = 0; //coverity 44577
	CHAR tempFilepath[30] = {0};
	//UINT32 newattrib=0;
	FST_FILE  filehdl;
	FST_FILE_STATUS filestatus = {0}; //coverity 44578

	switch (type) {
	case MEDIANR_CUSTOM_CHANGECRASH:
		if (p1 == 0) {
			DBG_ERR("MEDIANR_CUSTOM_CHANGECRASH NULL\r\n");
			return returnV;
		}
		pPath = (CHAR *)p1;

		NameRuleDCFFullUti_ChangeFrontPath(pPath);
		returnV = E_OK;
		break;
	case MEDIANR_CUSTOM_UPDATECRASH:
		DCF_SetNextID(gMediaNewDidAfterCrash, gMediaNewFidAfterCrash);

		returnV = E_OK;
		break;
	case MEDIANR_CUSTOM_GETFILES://2013/11/08
		p32int = (UINT32 *)p1;
		*p32int = DCF_GetDBInfo(DCF_INFO_TOL_FILE_COUNT);
		returnV = E_OK;
		break;


	case MEDIANR_CUSTOM_GETFILESIZE_BYSEQ://p1=seq, p2=(out)filesize, p3=(out)attrib
		p32int = (UINT32 *)p2;
		uiSequ = p1;
		ret = DCF_GetObjInfo(uiSequ, &did, &fid, &filetype);
		if (ret == TRUE) { //
			pPath = (CHAR *)tempFilepath;
			//getfilepath = DCF_GetObjPathByID(did,fid, filetype, pPath);
			getfilepath = DCF_GetObjPath(uiSequ, filetype, pPath);
			if (getfilepath == FALSE) {
				DBG_ERR("[REC]DCF path Fails..did=%d, fid = %d, type = %d!\r\n", did, fid, type);
				*p32int = 0;
				p32int = (UINT32 *)p3;
				*p32int = 0;
				return E_SYS;
			}
			filehdl = FileSys_OpenFile(pPath, FST_OPEN_READ);
			if (filehdl) {
				FileSys_StatFile(filehdl, &filestatus);
				filesize = filestatus.uiFileSize;
				attrib = filestatus.uiAttrib;
				FileSys_CloseFile(filehdl);
			} else {
				filesize = 0;
				attrib = 0;
			}
			//filesize = FileSys_GetFileLen(pPath);
			*p32int = (UINT32)filesize;
			//get = FileSys_GetAttrib(pPath, &attrib);

			p32int = (UINT32 *)p3;
			*p32int = (UINT32)attrib;
			DBG_IND("[%d] %s , size=0x%lx, attrib = 0x%x\r\n", uiSequ,  pPath, (UINT32)filesize, attrib);

		} else {
			*p32int = 0;
			p32int = (UINT32 *)p3;
			*p32int = 0;
		}
		//NameRuleDCFFull_customizeFunc(MEDIANR_CUSTOM_GET_ATTRIB, (UINT32)pPath, (UINT32)&newattrib,0);
		break;

	case MEDIANR_CUSTOM_SET_FILESIZE:
	case MEDIANR_CUSTOM_UPDATE_ATTRIB:
		//donothing
		break;

	case MEDIANR_CUSTOM_GET_FILESIZE://p1: CHAR *pPath (in), p2: UINT32 *filesize (out)
		if (p1 == 0) {
			DBG_ERR("MEDIANR_CUSTOM_GET_FILESIZE NULL\r\n");
			return returnV;
		}
		pPath = (CHAR *)p1;

		filesize = FileSys_GetFileLen(pPath);
		p32int = (UINT32 *)p2;
		*p32int = (UINT32)filesize;
		break;

	case MEDIANR_CUSTOM_GET_ATTRIB://p1: CHAR *pPath (in), p2: UINT32 attrib (in)
		if (p1 == 0) {
			DBG_ERR("MEDIANR_CUSTOM_GET_ATTRIB NULL\r\n");
			return returnV;
		}
		pPath = (CHAR *)p1;
		p32int = (UINT32 *)p2;

		get = FileSys_GetAttrib(pPath, &attrib);
		if (get != E_OK) {
			DBG_ERR("[REC]get attrib fails..%s!\r\n", pPath);
			*p32int = 0;
			return E_SYS;
		}
		DBG_IND("[REC]get attrib ok.%s! 0x%x\r\n", pPath, attrib);
		*p32int = (UINT32)attrib;
		break;

	default:
		break;
	}

	return returnV;
}

MEDIANAMINGRULE  gDCFNamingHdl = {
	NameRuleDCFFull_CreateAndGetPath, //CreateAndGetPath
	NameRuleDCFFull_AddPath,          //AddPath
	NameRuleDCFFull_DelPath,          //DelPath
	NameRuleDCFFull_GetPathBySeq,     //GetPathBySeq
	NameRuleDCFFull_CalcNextID,//CalcNextID
	NameRuleDCFFull_GetLastPath,//GetLastPath
	NameRuleDCFFull_GetNewCrashPath,//GetNewCrashPath
	NameRuleDCFFull_customizeFunc,
	NameRuleDCFFull_SetInfo,        //SetInfo
	NAMERULE_DCFFULL_CHECKID
};

PMEDIANAMINGRULE NameRule_getDCFFull(void)
{
	MEDIANAMINGRULE *pHdl;


	pHdl = &gDCFNamingHdl;

	return pHdl;
}

////===================================================================end DCF naming hdl
