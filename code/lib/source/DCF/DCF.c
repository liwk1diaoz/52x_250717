/*
    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.

    @file       DCF.c
    @author     Lincy Lin
    @date       2011/12/30
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
//#include "SysKer.h"
#include "DCFID.h"
#include "DCF.h"
//#include "fs_file_op.h" // for Scan DCF objects
#include "FileSysTsk.h"

#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "hd_common.h"
#include "kwrap/cpu.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          DCF
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#define __PERFTM__          0
#define __PERFBM__          0
//#define __DBGFLT__          "[name]"
//#define __DBGFLT__          "[dump]"
//#define __DBGFLT__          "[lock]"
//#define __DBGFLT__          "[del]"
#include <kwrap/debug.h>

// --------------------------------------------------------------------
// Flag patterns
// --------------------------------------------------------------------
#define wai_sem vos_sem_wait
#define sig_sem vos_sem_sig

#define BM_VAL_DEFINE(name)
#define BM_BEGIN(name)
#define BM_END(name)
#define BM_CLEAR(name)
#define BM_VAL_DECLARE(name)
#define BM_DUMP_HEADER()
#define BM_DUMP(name)


#define FLGDCF_SET_NEXT_ID               FLGPTN_BIT(0) //0x00000001
#define FLGDCF_LAST_FILE_FIND            FLGPTN_BIT(1) //0x00000002
#define FLGDCF_SCAN_COMPLETE             FLGPTN_BIT(2) //0x00000004


#define DCF_INITED_TAG                   MAKEFOURCC('D','C','F','T') ///< a key value 'D','C','F','T' for indicating DCF initial.
#define DCF_HANDLE1_TAG                  MAKEFOURCC('D','C','F','1')
#define DCF_HANDLE2_TAG                  MAKEFOURCC('D','C','F','2')

#define     DCF_CHK_VALID_FMT      1
#define     DCF_IS_DIR             1
#define     DCF_IS_FILE            0

#define     DCF_IS_EXIST           1
#define     DCF_IS_EMPTY           0

#define     DCF_NEED_CACHE         1
#define     DCF_DONT_CACHE         0

#define     DCF_IS_FORWARD         1
#define     DCF_IS_BACKWARD        0

#define     DCF_INDEX_FIRST        1


#define     DCF_FILE_EXT_LEN            3
#define     DCF_PATH_DCIM_LENGTH        8       // "A:\DCIM\"
#define     DCF_PATH_DIR_FREECHAR_POS   11      // "A:\DCIM\100"
#define     DCF_PATH_DIR_LENGTH         17      // "A:\DCIM\100NVTIM\"
#define     DCF_PATH_FILE_LENGTH        26      // "A:\DCIM\100NVTIM\IMAG0001"



#define     DCF_CACHE_ELMT_SIZE     (32)          // sizeof(int) *8
#define     DCF_DIR_TYPE_BITS       (1)
#define     DCF_DIR_CACHE_BITS      0x00000001  //  every dir only have exist or empty state, we use one bit to represent it

#define     DCF_DIR_CACHE_SIZE      (((MAX_DCF_DIR_NUM  -MIN_DCF_DIR_NUM+31) / 32)+1)
#define     DCF_FILE_CACHE_SIZE     (((((MAX_DCF_FILE_NUM - MIN_DCF_FILE_NUM)+31) / 32)+1)*DCF_FILE_TYPE_BITS) // *2 is for we use 2 bit for one file

// local used global variables
//static const CHAR    gPathDCIM[]= "A:\\DCIM\\";

//static UINT32  gDCFDirTb[DCF_DIR_CACHE_SIZE];
//static UINT32  gDCFFileTb[DCF_FILE_CACHE_SIZE];
//static UINT16  gDCFIDNumTb[MAX_DCF_DIR_NUM-MIN_DCF_DIR_NUM+1];
//static CHAR    gDCFDirName[MAX_DCF_DIR_NUM-MIN_DCF_DIR_NUM+1][DCF_DIR_NAME_LEN];
//static UINT8   gDCFDirAttrib[MAX_DCF_DIR_NUM-MIN_DCF_DIR_NUM+1];
//static CHAR    gDCFFileName[MAX_DCF_FILE_NUM-MIN_DCF_FILE_NUM+1][DCF_FILE_NAME_LEN];
//static UINT32  gTotalFilesInFolder[DCF_FILE_TYPE_NUM]={0};
//static UINT32  gTotalFiles[DCF_FILE_TYPE_NUM]={0};
//#NT#2013/01/08#Nestor Yang -begin
//#NT# Rename guiDupDirID as gMaxInvalidDirID to fit hidden and duplicate purposes
//static UINT32  guiDupDirID = MIN_DCF_DIR_NUM - 1;
//static UINT32  gMaxInvalidDirID = MIN_DCF_DIR_NUM - 1;
//#NT#2013/01/08#Nestor Yang -end
//static UINT32  guiMaxSystemFileId = 0;


#define        DCF_INVALID_MARK    (0xFF)
#define        DCF_DEF_VALID_MASK   DCF_FILE_TYPE_ANYFORMAT

// user customized variables
static CHAR    gOurDCFDirName[DCF_DIR_NAME_LEN + 1] = "NVTIM";
static UINT32  gValidFileFormat = DCF_FILE_TYPE_ANYFORMAT;
static UINT32  gDepFileFormat = 0;
static BOOL    gbSkipDupDir  = TRUE;
static BOOL    gbSkipDupFile = TRUE;
static BOOL    gbSkipHidden  = FALSE;
static BOOL    gbInitIndexLast = TRUE;
//static UINT32  guiNextDirId   = 0;
//static UINT32  guiNextFileId  = 0;
//static BOOL    gbIsScan = FALSE;
//static DCF_CBMSG_FP gDcfCbFP = NULL;
//static BOOL    gIsContinueDCFnum = TRUE;


// DCF database info
typedef struct {
	CHAR        AccessFilename[DCF_FULL_FILE_PATH_LEN];     // this is a temp buffer, for full DCF file name(at least 30 bytes)
	UINT32      TolFileNum;             // how many DCF ID applied in in whole system
	UINT32      TolIDSequ;              // the sequence of ack ID in total ID
	UINT32      TolDirNum;              // how many DCF folder in DCIM
	UINT32      MaxDirID;               // maximun directory ID of this system
	UINT32      DirectoryID;            // current access directory number.
	UINT32      FilenameID;             // current access file number
	UINT32      DirFileSequ;            // the file sequence of current dir
	UINT32      DirFileNum;             // how many DCF file in folder
	UINT32      MaxFileID;              // [out] maximun file ID of current access directory, it may bigger then DirFileNum
	UINT32      FileFormat;             // [in/out] point out the access file format it should be FST_FMT_JPG, WAV or AVI
	UINT32      IdFormat;               // [out] file format, use to indicate how many file format exist in the DCF ID.
	UINT32      MaxSystemFileID;        // Max file ID of Max Dir
	UINT32      NextDirId;
	UINT32      NextFileId;
	BOOL        Is9999;                 // the existence of 999xxxxx\yyyy9999.zzz
} DCF_DB_INFO, *PDCF_DB_INFO;

//static DCF_DB_INFO gDCFDbInfo={0};

//#NT#2012/07/05#Lincy Lin -begin
//#NT#Add dcf file type mapping func
typedef struct {
	UINT32          fileType;
	CHAR            ExtStr[DCF_FILE_EXT_LEN + 1];
	CHAR            FreeChars[DCF_FILE_NAME_LEN + 1];
	//UINT32          MapType;
} SDCF_FILE_TYPE;

// DCF File Type Ext name
#if 1
static SDCF_FILE_TYPE gDcfFiletype[DCF_FILE_TYPE_NUM] = {
	{DCF_FILE_TYPE_JPG, "JPG", "IMAG"},
	{DCF_FILE_TYPE_AVI, "AVI", "IMAG"},
	{DCF_FILE_TYPE_WAV, "WAV", "IMAG"},
	{DCF_FILE_TYPE_RAW, "RAW", "IMAG"},
	{DCF_FILE_TYPE_TIF, "TIF", "IMAG"},
	{DCF_FILE_TYPE_MOV, "MOV", "IMAG"},
	{DCF_FILE_TYPE_MPO, "MPO", "IMAG"},
	{DCF_FILE_TYPE_MP4, "MP4", "IMAG"},
	{DCF_FILE_TYPE_MPG, "MPG", "IMAG"},
	{DCF_FILE_TYPE_ASF, "ASF", "IMAG"}

};
//#NT#2012/07/05#Lincy Lin -end
#endif
#define     DCF_SUPPORT_RESERV_DATA_STORE      0

#if DCF_SUPPORT_RESERV_DATA_STORE
typedef struct {
	UINT32  TotalDCFObjectCount;
	CHAR    LastObjPath[DCF_FULL_FILE_PATH_LEN];
} DCF_RESV_DATA;

//static DCF_RESV_DATA  gDcfResvData={0};
#endif


typedef struct {
	BOOL            bIsValid;
	UINT32          DirID;
	UINT32          FileID;
	UINT32          FileType;
	CHAR            DirName[DCF_DIR_NAME_LEN + 1];
	CHAR            FileName[DCF_FILE_NAME_LEN + 1];
} SDCF_OBJ;

//static SDCF_OBJ     gDcfLastObj ={0};

typedef struct {
	UINT32          HeadTag;                                ///<  Head Tag to check if memory is overwrit
	CHAR            Drive;                                  ///<  The Drive Name of File System
	CHAR            PathDCIM[DCF_PATH_DCIM_LENGTH + 1];
	CHAR            OurDCFDirName[DCF_DIR_NAME_LEN + 1]; //="NVTIM";
	UINT32          ValidFileFormat;// = DCF_FILE_TYPE_ANYFORMAT;
	UINT32          DepFileFormat;
	BOOL            bSkipDupDir;   //  = TRUE;
	BOOL            bSkipDupFile;  //  = TRUE;
	BOOL            bInitIndexLast;// = TRUE;
	BOOL            bSkipHidden;
	SDCF_FILE_TYPE *DcfFiletype;
	BOOL            bIsScan;
	UINT32          DCFDirTb[DCF_DIR_CACHE_SIZE];
	UINT32          DCFFileTb[DCF_FILE_CACHE_SIZE];
	UINT16          DCFIDNumTb[MAX_DCF_DIR_NUM - MIN_DCF_DIR_NUM + 1];
	CHAR            DCFDirName[MAX_DCF_DIR_NUM - MIN_DCF_DIR_NUM + 1][DCF_DIR_NAME_LEN];
	UINT8           DCFDirAttrib[MAX_DCF_DIR_NUM - MIN_DCF_DIR_NUM + 1];
	CHAR            DCFFileName[MAX_DCF_FILE_NUM - MIN_DCF_FILE_NUM + 1][DCF_FILE_NAME_LEN];
	UINT32          TotalFilesInFolder[DCF_FILE_TYPE_NUM];
	UINT32          TotalFiles[DCF_FILE_TYPE_NUM];
	UINT32          MaxInvalidDirID;
	UINT32          MaxSystemFileId;
	DCF_CBMSG_FP    DcfCbFP;
	DCF_DB_INFO     DCFDbInfo;
	SDCF_OBJ        DcfLastObj;
#if DCF_SUPPORT_RESERV_DATA_STORE
	DCF_RESV_DATA   DcfResvData;
#endif
	//   semapre, flag control
	UINT32          SemidDcf;
	UINT32          SemidDcfNextId;
	UINT32          Flagid;
	UINT32          EndTag;                                ///<  End Tag to check if memory is overwrit
} DCF_INFO, *PDCF_INFO;

typedef struct {
	UINT32          InitTag;
	PDCF_INFO       pDcfInfo;
} DCF_HANDLE_INFO, *PDCF_HANDLE_INFO;

//static DCF_INFO   gDCFInfo[DCF_HANDLE_NUM] = {0};
static DCF_HANDLE_INFO   gDCFHandleInfo[DCF_HANDLE_NUM] = {0};
static UINT32            gDCFHandleTag[DCF_HANDLE_NUM] = {DCF_HANDLE1_TAG, DCF_HANDLE2_TAG};

static ER   DCF_lock(PDCF_INFO  pInfo);
static void   DCF_unlock(PDCF_INFO  pInfo);
static void DCF_internalRefresh(PDCF_INFO  pInfo, BOOL bKeepIndex);
static void DCF_internalScanObj(PDCF_INFO  pInfo, BOOL bKeepIndex);
static BOOL DCF_internalSetCurIndex(PDCF_INFO  pInfo, UINT32 index);
static BOOL DCF_internalSetNextID(PDCF_INFO  pInfo, UINT32 uiFolderId, UINT32 uiFileId);
static PDCF_INFO DCF_GetInfoByHandle(DCF_HANDLE DcfHandle);

static void DCF_lockComm(void)
{
	DBG_FUNC_BEGIN("[lock]\r\n");
	wai_sem(SEMID_DCF_COMM);
	DBG_FUNC_END("[lock]\r\n");
}

static void DCF_unlockComm(void)
{
	DBG_FUNC_BEGIN("[lock]\r\n");
	sig_sem(SEMID_DCF_COMM);
	DBG_FUNC_END("[lock]\r\n");
}


static ER DCF_lock(PDCF_INFO   pInfo)
{
	ER erReturn;

	DBG_FUNC_BEGIN("[lock]\r\n");

	erReturn = wai_sem(pInfo->SemidDcf);

	DBG_FUNC_END("[lock]\r\n");

	return erReturn;
}

/**
    Unlock DCF resource.

    @return
        - @b E_OK: unlock success
        - @b Else: unlock fail
*/
static void DCF_unlock(PDCF_INFO   pInfo)
{
	//ER erReturn;

	DBG_FUNC_BEGIN("[lock]\r\n");
	//erReturn  = sig_sem(pInfo->SemidDcf);
	sig_sem(pInfo->SemidDcf);
	DBG_FUNC_END("[lock]\r\n");
	//return erReturn;
}



/*========================================================================
    routine name :
    function     :
    parameter    :
    return value :
========================================================================*/
#if 0
static void DCFMakeStrUpper(CHAR *Str)
{
	while (*Str != '\0') {
		if ((*Str >= 'a') && (*Str <= 'z')) {
			*Str -= 'a' - 'A';
		}
		Str++;
	}
}
#endif

//#NT#2012/07/05#Lincy Lin -begin
//#NT#Add dcf file type mapping func
#if 0
static void DCFCreateFileTypeMap(void)
{
	UINT32 ValidType, i, index;

	ValidType = pInfo->ValidFileFormat | pInfo->DepFileFormat;
	for (i = 0; i < DCF_FILE_TYPE_NUM; i++) {
		pInfo->DcfFiletype[i].MapType = DCF_FILE_TYPE_NOFILE;
		if (pInfo->DcfFiletype[i].fileType & ValidType) {
			pInfo->DcfFiletype[i].MapType = (1 << index);
			index++;
		}
		if (index >= DCF_FILE_TYPE_BITS) {
			DBG_ERR("ValidType =0x%x over limit count %d", ValidType, DCF_FILE_TYPE_BITS - 1);
			break;
		}
	}
}

static UINT32 pInfo->DcfFiletypeOutToIn(UINT32 fileType)
{
	UINT32 i, rtn = 0;
	for (i = 0; i < DCF_FILE_TYPE_NUM; i++) {
		if (pInfo->DcfFiletype[i].fileType & fileType) {
			rtn |= pInfo->DcfFiletype[i].MapType;
		}
	}
	return rtn;
}

static UINT32 pInfo->DcfFiletypeInToOut(UINT32 fileType)
{
	UINT32 i, rtn = 0;
	for (i = 0; i < DCF_FILE_TYPE_NUM; i++) {
		if (pInfo->DcfFiletype[i].MapType & fileType) {
			rtn |= pInfo->DcfFiletype[i].fileType;
		}
	}
	return rtn;
}
#endif
//#NT#2012/07/05#Lincy Lin -end



/*========================================================================
    routine name :
    function     :
    parameter    :
        [in] CHAR *pExt: point to 3 chars ignore null terminal
    return value :
        DCF_FILE_TYPE__NOFILE, DCF_FILE_TYPE__JPG, DCF_FILE_TYPE__AVI or DCF_FILE_TYPE__WAV
========================================================================*/
static UINT32 DCFGetTypeByExt(CHAR *pExt)
{
	UINT32 i;
	for (i = 0; i < DCF_FILE_TYPE_NUM; i++) {
		if (gDcfFiletype[i].ExtStr[0] == pExt[0] && gDcfFiletype[i].ExtStr[1] == pExt[1] && gDcfFiletype[i].ExtStr[2] == pExt[2]) {
			return gDcfFiletype[i].fileType;
		}
	}
	return 0;
}

static CHAR *DCFGetExtByType(UINT32 fileType)
{
	UINT32 i;

	for (i = 0; i < DCF_FILE_TYPE_NUM; i++) {
		if (fileType == gDcfFiletype[i].fileType) {
			return gDcfFiletype[i].ExtStr;
		}
	}

	return NULL;
}

static CHAR *DCFGetFreeCharsByType(UINT32 fileType)
{
	UINT32 i;

	for (i = 0; i < DCF_FILE_TYPE_NUM; i++) {
		if (fileType == gDcfFiletype[i].fileType) {
			return gDcfFiletype[i].FreeChars;
		}
	}

	return NULL;
}
/*========================================================================
    routine name : DCF_IsValidFile
    function     :
        [in] CHAR *pMainName: 8 chars , the main name
        [in] CHAR *pExtName: 3 chars , the external name
        [out] UINT32 *pType: return the file type, NO fle, JPG,WAV, or AVI
    parameter    :
    return value : return file ID ,0 for invalid file name
========================================================================*/
INT32 DCF_IsValidFile(CHAR *pMainName, CHAR *pExtName, UINT32 *pType)
{
	char tmpStr[DCF_FILE_NAME_LEN + 1];
	int FileId = 0, i, j;
	// check whether is jpg extension name
	memcpy(tmpStr, pMainName + 4, DCF_FILE_NAME_LEN);
	tmpStr[DCF_FILE_NAME_LEN] = '\0';

	DBG_IND("[name]%11s\r\n", pMainName);
	*pType = DCF_FILE_TYPE_NOFILE;

	//check 4 digital number
	for (j = 0; j < DCF_FILE_NAME_LEN; j++) {
		if (!isdigit(tmpStr[j])) {
			return 0;
		}

		FileId += (tmpStr[j] - 0x30);

		if (j < DCF_FILE_NAME_LEN - 1) {
			FileId *= 10;
		}
	}
	if (FileId < MIN_DCF_FILE_NUM || FileId > MAX_DCF_FILE_NUM) {
		return 0;
	}

	for (i = 0; i < DCF_FILE_NAME_LEN; i++) {
		if ((!isalpha(pMainName[i])) && (pMainName[i] != '_') && (!isdigit(pMainName[i]))) {
			return 0;
		}
	}

	*pType = DCFGetTypeByExt(pExtName);
	if (*pType == DCF_FILE_TYPE_NOFILE) {
		return 0;
	} else {
		return FileId;
	}
}


/*========================================================================
    routine name :
    function     :
    parameter    :
    return value : return directory ID,  return 0 for invalid folder name
========================================================================*/
INT32 DCF_IsValidDir(char *pDirName)
{
	char tmpStr[4];
	int FolderId;
	int i;

	if(strnlen(pDirName, 32) < 8)
		return 0;

	if(strncmp(gOurDCFDirName, pDirName + 3, 5) != 0)
		return 0;

	DBG_IND("[name]%8s\r\n", pDirName);
	memcpy(tmpStr, pDirName, 3);
	tmpStr[3] = '\0';
	FolderId = atoi(tmpStr);
	if ((MIN_DCF_DIR_NUM <= FolderId) && (FolderId <= MAX_DCF_DIR_NUM)) {
		for (i = 3; i < 8; i++) {
			if ((!isalpha(pDirName[i])) && (pDirName[i] != '_') && (!isdigit(pDirName[i]))) {
				return 0;
			}
		}
	} else {
		return 0;
	}

	return FolderId;

}
/*========================================================================
    routine name : ToDCFElmtOffset
    function     : DCF ID to element and offset  in DCF Cache
    parameter    :
    return value :
        E_OK or E_SYS
========================================================================*/
static INT32  ToDCFElmtOffset(UINT32 WhichID, UINT32 *pElmt, UINT32 *pOffset, BOOL IsFolder)
{
	UINT32 tmp;
	if (IsFolder) {
		if ((WhichID >= MIN_DCF_DIR_NUM) && (WhichID <= MAX_DCF_DIR_NUM)) {
			tmp = (WhichID - MIN_DCF_DIR_NUM) * DCF_DIR_TYPE_BITS;
			*pElmt = tmp / 32;
			*pOffset = tmp % 32;
			return E_OK;
		} else {
			*pElmt = *pOffset = 0;
			return E_SYS;
		}
	} else {
		// if file
		if ((WhichID >= MIN_DCF_FILE_NUM) && (WhichID <= MAX_DCF_FILE_NUM)) {
			tmp = (WhichID - MIN_DCF_FILE_NUM) * DCF_FILE_TYPE_BITS;
			*pElmt = tmp / 32;
			*pOffset = tmp % 32;

			return E_OK;

		} else {
			*pElmt = *pOffset = 0;
			return E_SYS;
		}
	}
}
/*========================================================================
    routine name : SetDCFNumCache
    function     : set DCF cache value
    parameter    :
        [in] UINT32 WhichID: which ID
        [in] UINT32 Type: this input param is determine by IsFolder param,
            if IsFolder is true. it shuld only be 1 or 0
            else it would be DCF_FILE_TYPE__NOFILE, DCF_FILE_TYPE__JPG, DCF_FILE_TYPE__WAV or DCF_FILE_TYPE__AVI
        [in] IsFolder : indicate folder or file
========================================================================*/
static void SetDCFNumCache(PDCF_INFO  pInfo, UINT32 WhichID, UINT32 Type, BOOL IsFolder)
{
	UINT32  elmt, offset ;
	UINT32  uwTmp;

	if (ToDCFElmtOffset(WhichID,  &elmt, &offset, IsFolder) == E_OK) {
		if (IsFolder) {
			uwTmp = pInfo->DCFDirTb[elmt];
			uwTmp |= (DCF_DIR_CACHE_BITS << offset);
			pInfo->DCFDirTb[elmt] = uwTmp;
		} else {
			uwTmp = pInfo->DCFFileTb[elmt];
			uwTmp |= (Type << offset);
#if DCF_CHK_VALID_FMT
			if (Type & pInfo->ValidFileFormat) {
				uwTmp |= (DCF_FMT_VALID_BIT << offset);
			}
#endif
			pInfo->DCFFileTb[elmt] = uwTmp;
		}
	}
}

/*========================================================================
    routine name : ClearDCFNumCache
    function     : can delete any DCF format
    parameter    : UINT32 WhichID, UINT32 Type, BOOL IsFolder
    return value :
========================================================================*/
static void ClearDCFNumCache(PDCF_INFO   pInfo, UINT32 WhichID, UINT32 Type, BOOL IsFolder)
{
	UINT32 elmt, offset, uwTmp;
	if (ToDCFElmtOffset(WhichID,  &elmt, &offset, IsFolder) == E_OK) {
		if (IsFolder) {
			uwTmp = pInfo->DCFDirTb[elmt];
			uwTmp &= (~(DCF_DIR_CACHE_BITS << offset));
			pInfo->DCFDirTb[elmt] = uwTmp;
		} else {
			uwTmp = pInfo->DCFFileTb[elmt];
			uwTmp &= (~((Type & DCF_FILE_CACHE_BITS) << offset));
#if DCF_CHK_VALID_FMT
			if (!(uwTmp & (pInfo->ValidFileFormat << offset))) {
				uwTmp &= ~(DCF_FMT_VALID_BIT << offset);
			}
#endif
			pInfo->DCFFileTb[elmt] = uwTmp;
		}
	}
}

/*========================================================================
    routine name : GetDCFNumCache
    function     : get which ID's value in cache
    parameter    :
        [in] int *pCache :
        [in] UINT32 WhichID: which ID,
        [in] BOOL IsFolder: is folder or file
    return value :
        return DCF_FILE_TYPE_NOFILE, DCF_FILE_TYPE_JPG, DCF_FILE_TYPE_WAV or DCF_FILE_TYPE_AVI
========================================================================*/
static UINT32 GetDCFNumCache(PDCF_INFO  pInfo, UINT32 WhichID, BOOL IsFolder)
{
	UINT32 elmt, offset ;


	if (ToDCFElmtOffset(WhichID,  &elmt, &offset, IsFolder) == E_OK) {
		if (!IsFolder) {
			// is file
#if DCF_CHK_VALID_FMT
			if ((pInfo->DCFFileTb[elmt] >> offset) & DCF_FMT_VALID_BIT) {
				return (pInfo->DCFFileTb[elmt] >> offset)& DCF_FILE_CACHE_BITS;
			}
#else
			return (pInfo->DCFFileTb[elmt] >> offset)&DCF_FILE_CACHE_BITS;
#endif
		} else {
			// is fodler
			return (pInfo->DCFDirTb[elmt] >> offset)&DCF_DIR_CACHE_BITS;
		}
	}
	return DCF_FILE_TYPE_NOFILE;
}


static UINT32 GetDCFNumCacheAll(PDCF_INFO  pInfo, UINT32 WhichID, BOOL IsFolder)
{
	UINT32 elmt, offset ;


	if (ToDCFElmtOffset(WhichID,  &elmt, &offset, IsFolder) == E_OK) {
		if (!IsFolder) {
			// is file
			return (pInfo->DCFFileTb[elmt] >> offset)&DCF_FILE_CACHE_BITS;
		} else {
			// is fodler
			return (pInfo->DCFDirTb[elmt] >> offset)&DCF_FILE_CACHE_BITS;
		}
	}
	return 0;
}


/*========================================================================
    routine name : ToDCFID
    function     : element and offset to DCF ID in DCF Cache
    parameter    :
        [in] UINT32 WhichID:
    return value :
        DCF ID value
========================================================================*/
static UINT32 ToDCFID(UINT32 Element, UINT32 Offset, BOOL IsFolder)
{
	UINT32 result;

	result = Element * DCF_CACHE_ELMT_SIZE + Offset;

	if (IsFolder) {
		return (UINT32)((result / DCF_DIR_TYPE_BITS) + MIN_DCF_DIR_NUM);
	} else {
		return (UINT32)((result / DCF_FILE_TYPE_BITS) + MIN_DCF_FILE_NUM);
	}

}

#if 1
static INT32 DCFCountFolder(PDCF_INFO   pInfo, UINT32 *pTolDirNum, UINT32 *pMaxDirID)
{
	FS_SEARCH_HDL pSrhDir;
	FIND_DATA     findDir = {0};
	UINT32        numDir, FolderId, maxDir;
	int           result;

	maxDir = 0;
	numDir = 0;
	pSrhDir = NULL;
	pSrhDir = fs_SearchFileOpen((char *)pInfo->PathDCIM, &findDir, KFS_FORWARD_SEARCH, NULL);
	pInfo->MaxInvalidDirID = MIN_DCF_DIR_NUM - 1;

	while (pSrhDir) {
		if (M_IsDirectory(findDir.attrib)) {
			FolderId = DCF_IsValidDir(findDir.FATMainName);
			if (FolderId) {
				if (pInfo->bSkipDupDir) {
					// skip dir id that mark as invalid
					if (pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM][0] == DCF_INVALID_MARK) {
						goto L_DCFCountFolder_NEXT;
					}

					if (pInfo->bSkipHidden && M_IsHidden(findDir.attrib)) {
						goto L_DCFCountFolder_NEXT;
					}

					if (numDir > 0 && pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM][0] != 0) {
						DBG_ERR("Duplicate folderId: %11s\r\n", findDir.filename);
						// both duplicate dir are not counted
						ClearDCFNumCache(pInfo, FolderId, DCF_FILE_TYPE_ANYFORMAT, DCF_IS_DIR);
						memset(&pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM], 0, DCF_DIR_NAME_LEN);
						// mark invalid for later checking
						pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM][0] = DCF_INVALID_MARK;

						numDir--;

						if (FolderId > pInfo->MaxInvalidDirID) {
							pInfo->MaxInvalidDirID = FolderId;
						}
					} else {
						memcpy(&pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM], &findDir.filename[3], DCF_DIR_NAME_LEN);
						pInfo->DCFDirAttrib[FolderId - MIN_DCF_DIR_NUM] = findDir.attrib;
						numDir++;
						SetDCFNumCache(pInfo, FolderId, DCF_IS_EXIST, DCF_IS_DIR);
					}
					if (FolderId > maxDir) {
						maxDir = FolderId;
					}
				} else {
					if (pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM][0] != 0) {
						DBG_ERR("Duplicate folderId: %11s\r\n", findDir.filename);
						//#NT#2014/10/03#Nestor Yang -begin
						//#NT# Update to our dir name or reserve the original one
						//if(strcmp(&findDir.filename[3],pInfo->DCFDirName[FolderId-MIN_DCF_DIR_NUM])>0)
						if (0 == strcmp(&findDir.filename[3], gOurDCFDirName))
							//#NT#2014/10/03#Nestor Yang -end
						{
							memcpy(pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM], &findDir.filename[3], DCF_DIR_NAME_LEN);
							pInfo->DCFDirAttrib[FolderId - MIN_DCF_DIR_NUM] = findDir.attrib;
						}
					} else {
						memcpy(pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM], &findDir.filename[3], DCF_DIR_NAME_LEN);
						pInfo->DCFDirAttrib[FolderId - MIN_DCF_DIR_NUM] = findDir.attrib;
					}
					numDir++;
					SetDCFNumCache(pInfo, FolderId, DCF_IS_EXIST, DCF_IS_DIR);
					if (FolderId > maxDir) {
						maxDir = FolderId;
					}
				}
			}
		}
L_DCFCountFolder_NEXT:
		result = fs_SearchFile(pSrhDir, &findDir, KFS_FORWARD_SEARCH, NULL);
		if (result != FSS_OK) {
			fs_SearchFileClose(pSrhDir);
			break; // break out do loop
		}
	}

	*pTolDirNum = numDir;
	*pMaxDirID = maxDir;
	return E_OK;
}
#else
static void CountFolder_ScanCB(FIND_DATA *findDir, BOOL *bContinue, UINT16 *cLongname, UINT32 Param)
{
	UINT32  FolderId;

	if (M_IsDirectory(findDir->attrib)) {
		FolderId = DCF_IsValidDir(findDir->FATMainName);
		if (FolderId) {
			if (pInfo->bSkipDupDir) {
				// skip dir id that mark as invalid
				if (pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM][0] == DCF_INVALID_MARK) {
					return;
				} else if (pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM][0] != 0) {
					DBG_IND("There are the duplicate folderId! %11s\r\n", findDir->filename);
					// both duplicate dir are not counted
					ClearDCFNumCache(pInfo, FolderId, DCF_FILE_TYPE_ANYFORMAT, DCF_IS_DIR);
					memset(&pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM], 0, DCF_DIR_NAME_LEN);
					// mark invalid for later checking
					pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM][0] = DCF_INVALID_MARK;
					gScanNumDir--;
					if (FolderId > pInfo->MaxInvalidDirID) {
						pInfo->MaxInvalidDirID = FolderId;
					}
				} else {
					memcpy(&pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM], &findDir->filename[3], DCF_DIR_NAME_LEN);
					pInfo->DCFDirAttrib[FolderId - MIN_DCF_DIR_NUM] = findDir->attrib;
					gScanNumDir++;
					SetDCFNumCache(pInfo, FolderId, DCF_IS_EXIST, DCF_IS_DIR);
				}
				if (FolderId > gScanMaxDir) {
					gScanMaxDir = FolderId;
				}
			} else {
				if (pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM][0] != 0) {
					DBG_IND("There are the duplicate folderId! %11s\r\n", findDir->filename);
					if (strcmp(&findDir->filename[3], pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM]) > 0) {
						memcpy(pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM], &findDir->filename[3], DCF_DIR_NAME_LEN);
						pInfo->DCFDirAttrib[FolderId - MIN_DCF_DIR_NUM] = findDir->attrib;
					}
				} else {
					memcpy(pInfo->DCFDirName[FolderId - MIN_DCF_DIR_NUM], &findDir->filename[3], DCF_DIR_NAME_LEN);
					pInfo->DCFDirAttrib[FolderId - MIN_DCF_DIR_NUM] = findDir->attrib;
				}
				gScanNumDir++;
				SetDCFNumCache(pInfo, FolderId, DCF_IS_EXIST, DCF_IS_DIR);
				if (FolderId > gScanMaxDir) {
					gScanMaxDir = FolderId;
				}
			}
		}
	}
}

static INT32 DCFCountFolder(UINT32 *pTolDirNum, UINT32 *pMaxDirID)
{
	gScanMaxDir = 0;
	gScanNumDir = 0;
	pInfo->MaxInvalidDirID = MIN_DCF_DIR_NUM - 1;
	FileSys_ScanDir((char *)pInfo->PathDCIM, CountFolder_ScanCB, FALSE);
	*pTolDirNum = gScanNumDir;
	*pMaxDirID = gScanMaxDir;
	return E_OK;
}
#endif


#if 1
/*========================================================================
    function     : count how many file in specify foler
    parameter    :
        [in] char *pPath: DCF folder
        [out] int *pFileTbl: point of file cache
        [in] BOOL NeedCache: Specified wheter need to cache file number status to cache
        [in] *pResrInfo:
            [out] FilenameID: always 0
            [out] DirFileNum: how many DCF file in folder
            [out] MaxFileID: the maximum DCF number in folder, 0 means there is no DCF file in folder
    return value :
========================================================================*/
static INT32 DCFCountFile(PDCF_INFO   pInfo, char *pPath, UINT32 *pFileTbl, BOOL NeedCache, UINT32 *pDirFileNum, UINT32 *pMaxFileID)
{
	FS_SEARCH_HDL pSrhFile;
	FIND_DATA     findFile = {0};
	UINT32        numFile, tmp, maxFile;
	int           result;
	UINT32        FileType, i;
	UINT32        tmpCache, tmpCacheAll;
	//UINT32    CheckPattern = DCF_FMT_VALID_BIT|(DCF_FILE_TYPE_WAV|DCF_FILE_TYPE_JPG|DCF_FILE_TYPE_MPO&(pInfo->ValidFileFormat|pInfo->DepFileFormat));
	UINT32        CheckPattern = (DCF_FMT_VALID_BIT | pInfo->DepFileFormat);
	BM_VAL_DEFINE(SearchNext);
	BM_VAL_DEFINE(dcf);
	BM_VAL_DEFINE(dcf1);
	BM_VAL_DEFINE(dcf2);
	BM_VAL_DEFINE(dcf3);

	memset(pFileTbl, 0, sizeof(UINT32)*DCF_FILE_CACHE_SIZE);
	memset(&pInfo->DCFFileName, 0, sizeof(pInfo->DCFFileName));
	memset(&pInfo->TotalFilesInFolder, 0, sizeof(pInfo->TotalFilesInFolder));

#if DCF_CHK_VALID_FMT
	pInfo->MaxSystemFileId = 0;
#endif

	numFile = 0;
	maxFile = 0;
	pSrhFile = NULL;
	pSrhFile = fs_SearchFileOpen(pPath, &findFile, KFS_FORWARD_SEARCH, NULL);
	while (pSrhFile) {
		BM_CLEAR(dcf);
		BM_CLEAR(dcf1);
		BM_CLEAR(dcf2);
		BM_CLEAR(dcf3);
		BM_BEGIN(dcf);
		if (!M_IsDirectory(findFile.attrib)) {
			BM_BEGIN(dcf1);
			tmp = DCF_IsValidFile(findFile.FATMainName, findFile.FATExtName, &FileType);
			BM_END(dcf1);
			BM_DUMP(dcf1);
			if (tmp > pInfo->MaxSystemFileId) {
				//update the max file ID in this folder
				pInfo->MaxSystemFileId = tmp;
			}

			if (tmp && FileType != DCF_FILE_TYPE_NOFILE) {
				// is valid file ID
				if (tmp > maxFile) {
#if DCF_CHK_VALID_FMT
					//#NT#2012/10/22#Lincy Lin -begin
					//#NT#Fix max id bug
					//pInfo->MaxSystemFileId = tmp;
					//#NT#2012/10/22#Lincy Lin -end
					if (FileType & (pInfo->ValidFileFormat | pInfo->DepFileFormat)) {
						maxFile = tmp;
					}
#else
					maxFile = tmp;
#endif
				}
				if (pInfo->bSkipDupFile) {
					// skip file id that mark as invalid
					if (pInfo->DCFFileName[tmp - MIN_DCF_FILE_NUM][0] == DCF_INVALID_MARK) {
						goto L_DCFCountFile_NEXT;
					}

					if (pInfo->bSkipHidden && M_IsHidden(findFile.attrib)) {
						goto L_DCFCountFile_NEXT;
					}

					BM_BEGIN(dcf2);
					tmpCache = GetDCFNumCache(pInfo, tmp, DCF_IS_FILE);
					BM_END(dcf2);
					BM_DUMP(dcf2);
					if (tmpCache == DCF_FILE_TYPE_NOFILE && (FileType & pInfo->ValidFileFormat)) {
						numFile++;    // no the same ID , add this file
					}

					tmpCacheAll = GetDCFNumCacheAll(pInfo, tmp, DCF_IS_FILE);

					if ((pInfo->DCFFileName[tmp - MIN_DCF_FILE_NUM][0] != 0) &&
						(((FileType | tmpCacheAll) & (~CheckPattern)) || (FileType & tmpCacheAll))) {
						//if file already exist, skip invalid file type or duplicate file type
						//#NT#2013/01/08#Nestor Yang -end
						DBG_IND("There are the duplicate fileId! %11s  %x %x \r\n", findFile.FATMainName, FileType, GetDCFNumCache(pInfo, tmp, DCF_IS_FILE));
						// both duplicate file are not counted
						ClearDCFNumCache(pInfo, tmp, DCF_FILE_TYPE_ANYFORMAT, DCF_IS_FILE);
						memset(&pInfo->DCFFileName[tmp - MIN_DCF_FILE_NUM], 0, DCF_FILE_NAME_LEN);
						// mark invalid for later checking
						pInfo->DCFFileName[tmp - MIN_DCF_FILE_NUM][0] = DCF_INVALID_MARK;
						numFile--;
					} else {
						BM_BEGIN(dcf3);
						SetDCFNumCache(pInfo, tmp, FileType, DCF_IS_FILE);
						for (i = 0; i < DCF_FILE_TYPE_NUM; i++) {
							if (FileType == pInfo->DcfFiletype[i].fileType) {
								pInfo->TotalFilesInFolder[i]++;
							}
						}
						//duplicated fileID checking avoid check wav
						if ((FileType & (pInfo->ValidFileFormat | pInfo->DepFileFormat))) {
							memcpy(&pInfo->DCFFileName[tmp - MIN_DCF_FILE_NUM], findFile.FATMainName, DCF_FILE_NAME_LEN);
						}
						BM_END(dcf3);
						BM_DUMP(dcf3);
					}
				} else {
					if (GetDCFNumCache(pInfo, tmp, DCF_IS_FILE) == DCF_FILE_TYPE_NOFILE && (FileType & pInfo->ValidFileFormat)) {
						numFile++;    // the same ID exist
					}
					SetDCFNumCache(pInfo, tmp, FileType, DCF_IS_FILE);


					for (i = 0; i < DCF_FILE_TYPE_NUM; i++) {
						if (FileType == pInfo->DcfFiletype[i].fileType) {
							pInfo->TotalFilesInFolder[i]++;
						}
					}
					//if(M_IsReadOnly(findFile.attrib))
					//readOnlyFilesInFolder++;
					memcpy(pInfo->DCFFileName[tmp - MIN_DCF_FILE_NUM], findFile.FATMainName, DCF_FILE_NAME_LEN);
				}
			}
		}
L_DCFCountFile_NEXT:
		BM_END(dcf);
		BM_DUMP(dcf);
		BM_CLEAR(SearchNext);
		BM_BEGIN(SearchNext);
		result = fs_SearchFile(pSrhFile, &findFile, KFS_FORWARD_SEARCH, NULL);
		BM_END(SearchNext);
		BM_DUMP(SearchNext);
		if (result != FSS_OK) {
			fs_SearchFileClose(pSrhFile);
			break; // break out do loop
		}
	}   // end of while loop

	*pDirFileNum = numFile;
	*pMaxFileID = maxFile;
	return E_OK;

}
#else

static void CountFile_SacnCB(FIND_DATA *findFile, BOOL *bContinue, UINT16 *cLongname, UINT32 Param)
{
	UINT32    tmp;
	UINT32    FileType, i;
	UINT32    tmpCache, tmpCacheAll;
	UINT32    CheckPattern = (DCF_FMT_VALID_BIT | pInfo->DepFileFormat);

	if (!M_IsDirectory(findFile->attrib)) {
		tmp = DCF_IsValidFile(findFile->FATMainName, findFile->FATExtName, &FileType);
		if (tmp > pInfo->MaxSystemFileId) {
			pInfo->MaxSystemFileId = tmp;
		}

		if (tmp && FileType != DCF_FILE_TYPE_NOFILE) {
			// is valid file ID
			if (tmp > gScanMaxFile) {
#if DCF_CHK_VALID_FMT
				pInfo->MaxSystemFileId = tmp;
				if (FileType & (pInfo->ValidFileFormat | pInfo->DepFileFormat)) {
					gScanMaxFile = tmp;
				}
#else
				gScanMaxFile = tmp;
#endif
			}
			if (pInfo->bSkipDupFile) {
				// skip file id that mark as invalid
				if (pInfo->DCFFileName[tmp - MIN_DCF_FILE_NUM][0] == DCF_INVALID_MARK) {
					return;
				}

				tmpCache = GetDCFNumCache(pInfo, tmp, DCF_IS_FILE);
				if (tmpCache == DCF_FILE_TYPE_NOFILE && (FileType & pInfo->ValidFileFormat)) {
					gScanNumFile++;    // no the same ID , add this file
				}

				tmpCacheAll = GetDCFNumCacheAll(pInfo, tmp, DCF_IS_FILE);
				if ((pInfo->DCFFileName[tmp - MIN_DCF_FILE_NUM][0] != 0) &&
					(((FileType | tmpCacheAll) & (~CheckPattern)) || (FileType & tmpCacheAll)))

				{
					DBG_IND("There are the duplicate fileId! %11s  %x %x \r\n", findFile->FATMainName, FileType, GetDCFNumCache(pInfo, tmp, DCF_IS_FILE));
					// both duplicate file are not counted
					ClearDCFNumCache(pInfo, tmp, DCF_FILE_TYPE_ANYFORMAT, DCF_IS_FILE);
					memset(&pInfo->DCFFileName[tmp - MIN_DCF_FILE_NUM], 0, DCF_FILE_NAME_LEN);
					// mark invalid for later checking
					pInfo->DCFFileName[tmp - MIN_DCF_FILE_NUM][0] = DCF_INVALID_MARK;
					gScanNumFile--;
				} else {
					SetDCFNumCache(pInfo, tmp, FileType, DCF_IS_FILE);
					for (i = 0; i < DCF_FILE_TYPE_NUM; i++) {
						if (FileType == pInfo->DcfFiletype[i].fileType) {
							pInfo->TotalFilesInFolder[i]++;
						}
					}
					//duplicated fileID checking avoid check wav
					if ((FileType & (pInfo->ValidFileFormat | pInfo->DepFileFormat))) {
						memcpy(&pInfo->DCFFileName[tmp - MIN_DCF_FILE_NUM], findFile->FATMainName, DCF_FILE_NAME_LEN);
					}
				}
			} else {
				if (GetDCFNumCache(pInfo, tmp, DCF_IS_FILE) == DCF_FILE_TYPE_NOFILE && (FileType & pInfo->ValidFileFormat)) {
					gScanNumFile++;    // the same ID exist
				}
				SetDCFNumCache(pInfo, tmp, FileType, DCF_IS_FILE);


				for (i = 0; i < DCF_FILE_TYPE_NUM; i++) {
					if (FileType == pInfo->DcfFiletype[i].fileType) {
						pInfo->TotalFilesInFolder[i]++;
					}
				}
				//if(M_IsReadOnly(findFile->attrib))
				//readOnlyFilesInFolder++;
				memcpy(pInfo->DCFFileName[tmp - MIN_DCF_FILE_NUM], findFile->FATMainName, DCF_FILE_NAME_LEN);
			}
		}
	}
}

/*========================================================================
    function     : count how many file in specify foler
    parameter    :
        [in] char *pPath: DCF folder
        [out] int *pFileTbl: point of file cache
        [in] BOOL NeedCache: Specified wheter need to cache file number status to cache
        [in] *pResrInfo:
            [out] FilenameID: always 0
            [out] DirFileNum: how many DCF file in folder
            [out] MaxFileID: the maximum DCF number in folder, 0 means there is no DCF file in folder
    return value :
========================================================================*/
static INT32 DCFCountFile(char *pPath, UINT32 *pFileTbl, BOOL NeedCache, UINT32 *pDirFileNum, UINT32 *pMaxFileID)
{
	memset(pFileTbl, 0, sizeof(UINT32)*DCF_FILE_CACHE_SIZE);
	memset(&pInfo->DCFFileName, 0, sizeof(pInfo->DCFFileName));
	memset(&pInfo->TotalFilesInFolder, 0, sizeof(pInfo->TotalFilesInFolder));

	gScanNumFile = 0;
	gScanMaxFile = 0;
#if DCF_CHK_VALID_FMT
	pInfo->MaxSystemFileId = 0;
#endif
	FileSys_ScanDir((char *)pPath, CountFile_SacnCB, FALSE);

	*pDirFileNum = gScanNumFile;
	*pMaxFileID = gScanMaxFile;
	return E_OK;

}
#endif

static INT32 GetDCFFolderName(PDCF_INFO   pInfo, CHAR *FolderName, UINT32 FolderID)
{
	char tmpDirName[DCF_DIR_NAME_LEN + 1];

	if (FolderID < MIN_DCF_DIR_NUM) {
		DBG_ERR("FolderID < MIN(%d)\r\n", MIN_DCF_DIR_NUM);
		FolderName[0] = '\0';
		return 0;
	}

	if (pInfo->DCFDirName[FolderID - MIN_DCF_DIR_NUM][0] == 0) {
		memcpy(pInfo->DCFDirName[FolderID - MIN_DCF_DIR_NUM], pInfo->OurDCFDirName, DCF_DIR_NAME_LEN);
	}

	memset(tmpDirName, 0, DCF_DIR_NAME_LEN + 1);
	memcpy(tmpDirName, pInfo->DCFDirName[FolderID - MIN_DCF_DIR_NUM], DCF_DIR_NAME_LEN);
	sprintf(FolderName, "%s%03d%s\\", pInfo->PathDCIM, (int)(FolderID), tmpDirName);
	FolderName[DCF_PATH_DIR_LENGTH] = '\0';
	DBG_IND("%s \r\n", (char *)FolderName);
	return (INT32)strlen(FolderName);
}

/*========================================================================
    routine name :
    function     :
    parameter    :
    return value : return 0 or the number of characters written in the array,
========================================================================*/
static INT32 GetDCFFileName(PDCF_INFO   pInfo, char *pFileName, UINT32 FolderID, UINT32 FileID, UINT32 Type)
{
	int ret;
	char *pExtName;
	char tmpDirName[6];
	char tmpFileName[5];

	pExtName = DCFGetExtByType(Type);
	if ((pExtName == NULL) || (pInfo->DCFDirName[FolderID - MIN_DCF_DIR_NUM][0] == 0) || (pInfo->DCFFileName[FileID - MIN_DCF_FILE_NUM][0] == 0)) {
		DBG_ERR("FolderID=%d FileID=%d Type=0x%x\r\n", FolderID, FileID, Type);
		*pFileName = '\0';
		return 0;
	}
	//memset(tmpDirName,0,DCF_DIR_NAME_LEN+1);
	memcpy(tmpDirName, pInfo->DCFDirName[FolderID - MIN_DCF_DIR_NUM], DCF_DIR_NAME_LEN);
	tmpDirName[DCF_DIR_NAME_LEN] = '\0';
	//memset(tmpFileName,0,DCF_FILE_NAME_LEN+1);
	memcpy(tmpFileName, pInfo->DCFFileName[FileID - MIN_DCF_FILE_NUM], DCF_FILE_NAME_LEN);
	tmpFileName[DCF_FILE_NAME_LEN] = '\0';

	sprintf(pFileName, "%s%03d%s\\%s%04d.", pInfo->PathDCIM,
			(int)(FolderID),
			tmpDirName,
			tmpFileName,
			(int)(FileID));
	memcpy(pFileName + DCF_PATH_FILE_LENGTH, pExtName, DCF_FILE_EXT_LEN);
	*(pFileName + DCF_PATH_FILE_LENGTH + DCF_FILE_EXT_LEN)  = '\0';
	//#NT#2012/10/23#Lincy Lin -begin
	//#NT#
	//ret = (int)strlen(pFileName);
	ret = DCF_PATH_FILE_LENGTH + DCF_FILE_EXT_LEN;
	//#NT#2012/10/23#Lincy Lin -end
	DBG_IND("%s\r\n", (char *)pFileName);
	return  ret;
}

static void DCFSetIDNumInDir(PDCF_INFO   pInfo, UINT32 WhichDir, UINT32 Num)
{
	pInfo->DCFIDNumTb[WhichDir - MIN_DCF_DIR_NUM] = Num;
}

static UINT32 DCFGetIDNumInDir(PDCF_INFO   pInfo, UINT32 WhichDir)
{
	return pInfo->DCFIDNumTb[WhichDir - MIN_DCF_DIR_NUM];
}


/*========================================================================
    routine name :
    function     : the api will return the sequence of specific file, Note: the sequence
        may exist or empty, it is determined by input file ID status.
    parameter    :
        [in]const int *pCache: DCF file cache
        [in] UINT32 WhichID: which file,
    return value :  return the sequence in folder it should be 1~MaxID
========================================================================*/
static UINT32 DCFGetFileSequInFolder(PDCF_INFO  pInfo, UINT32 *pCache, UINT32 WhichID)
{
	UINT32 elmt, offset, tmp;
	UINT32 /*allFit, */ result = 0;
	UINT32 uwBitMask;

	if (ToDCFElmtOffset(WhichID,  &elmt, &offset, DCF_IS_FILE) != E_OK) {
		return 0;
	}

#if DCF_CHK_VALID_FMT
	uwBitMask = DCF_FILE_CACHE_BITS & pInfo->ValidFileFormat;
#else
	uwBitMask = DCF_FILE_CACHE_BITS;
#endif

	tmp = pCache[elmt];
#if DCF_CHK_VALID_FMT
	//do not know why check file exist then do else, remove else.
	result = 0;
	if (tmp & (uwBitMask << offset)) {
		// file exist
		result = 1; // orignal one
	}
	//#NT#2012/07/13#Lincy Lin -begin
	//#NT#
	if (result) {
		// search in the same element
		while (offset) {
			offset -= DCF_FILE_TYPE_BITS;
			if (tmp & (uwBitMask << offset)) {
				result++;
			}
		};

		//elmt++;
		//allFit = 0xFFFFFFFF;
		while (elmt) {
			elmt--;
			tmp = pCache[elmt];
			/*
			if(tmp == allFit)
			{
			    result += (DCF_CACHE_ELMT_SIZE /DCF_FILE_TYPE_BITS);
			}
			else
			*/
			if (tmp != 0) {
				/*
				offset = DCF_CACHE_ELMT_SIZE;
				while(offset)
				{
				    offset-=DCF_FILE_TYPE_BITS;
				    if(tmp&(uwBitMask << offset))
				        result++;
				};
				*/
				if (tmp & (uwBitMask << DCF_FILE_TYPE_BITS)) {
					result++;
				}
				if (tmp & uwBitMask) {
					result++;
				}

			}
		}
	}
#else
	if (tmp & (uwBitMask << offset)) {
		// file exist
		result = 1; // orignal one
		// search in the same element
		while (offset) {
			offset -= DCF_FILE_TYPE_BITS;
			if (tmp & (uwBitMask << offset)) {
				result++;
			}
		};

		//elmt++;
		allFit = 0xFFFFFFFF;
		while (elmt) {
			elmt--;
			tmp = pCache[elmt];
			if (tmp == allFit) {
				result += (DCF_CACHE_ELMT_SIZE / DCF_FILE_TYPE_BITS);
			} else {
				offset = DCF_CACHE_ELMT_SIZE;
				while (offset) {
					offset -= DCF_FILE_TYPE_BITS;
					if (tmp & (uwBitMask << offset)) {
						result++;
					}
				};
			}
		}
	} else {
		// the specified ID is nonused
		allFit = 0x00000000;
		result = 1;

		// search in the same element
		tmp = pCache[elmt];
		while (offset) {
			offset -= DCF_FILE_TYPE_BITS;
			if (!(tmp & (uwBitMask << offset))) {
				result++;
			}
		};

		while (elmt) {
			elmt--;
			tmp = pCache[elmt];
			if (tmp == allFit) {
				result += (DCF_CACHE_ELMT_SIZE / DCF_FILE_TYPE_BITS);
			} else {
				offset = DCF_CACHE_ELMT_SIZE;
				while (offset) {
					offset -= DCF_FILE_TYPE_BITS;
					if (!(tmp & (uwBitMask << offset))) {
						result++;
					}
				};
			}
		}
	}
#endif
	return result;

}


static UINT32 DCFGetIDSequ(PDCF_INFO  pInfo, UINT32 DirId, UINT32 FileId, UINT32 *pDirSequ)
{
	UINT32 i, endDir, idSequ;

	idSequ = 0;
	endDir = DirId - MIN_DCF_DIR_NUM;
	for (i = 0; i < endDir; i++) {
		idSequ += pInfo->DCFIDNumTb[i];
	}
	if (pInfo->DCFIDNumTb[endDir]) {
		i = DCFGetFileSequInFolder(pInfo, pInfo->DCFFileTb, FileId);
	} else {
		i = 0;
	}
	if (i) {
		*pDirSequ = i;
		idSequ += i;
	} else {
		idSequ = 0;
	}
	return idSequ;
}

/*========================================================================
    routine name : DCFChangeWorkingDir
    function     : change wording folder and update cache
    parameter    :
        [in] UINT32 DirID:
        [out]int *pFileCache:
        [out] char *pDirName: at lease 32 bytes
            [out] UINT32 *DirFileNum    : output file number in this folder
            [out] UINT32 *MaxFileID : output max DCF file ID in this folder
    return value :
        DCF file number in this folder
========================================================================*/
static UINT32 DCFChangeWorkingDir(PDCF_INFO  pInfo, UINT32 DirID, UINT32 *pFileCache, PDCF_DB_INFO pDcfDbInfo)
{
	DBG_IND("%d \r\n", DirID);
	GetDCFFolderName(pInfo, pDcfDbInfo->AccessFilename, DirID);
	DCFCountFile(pInfo, pDcfDbInfo->AccessFilename, pFileCache, DCF_NEED_CACHE, &(pDcfDbInfo->DirFileNum), &(pDcfDbInfo->MaxFileID));
	DCFSetIDNumInDir(pInfo, DirID, pDcfDbInfo->DirFileNum);
	pDcfDbInfo->DirectoryID = DirID;
	return pDcfDbInfo->DirFileNum;
}

/*========================================================================
    routine name : GetPreObjID
    function     : Get previous free or exist DCF object ID from current ID
    parameter    :
        [in] const int *pCache: cache of DCF table.
        [in] UINT32 StartID : which ID to start, if 0 it will return the max ID in cache
        [in] BOOL IsExist: to find free or used ID.
        [in] BOOL IsFoldr: to file folder or file.
    return value : return the ID, for reach the begin of cache and
        doesn't find any one matcht it  return 0
========================================================================*/
static UINT32 GetPreObjID(const UINT32 *pCache, UINT32 StartID, BOOL IsExist, BOOL IsFolder)
{
	UINT32 elmt, offset, offsetUnit;
	UINT32 tmp, noMatch;
	UINT32  uiBitMask;

	if (StartID == 0) {
		// reach the begin return the last one
		if (IsFolder) {
			elmt = DCF_DIR_CACHE_SIZE - 1;
		} else {
			elmt = DCF_FILE_CACHE_SIZE - 1;
		}
		offset = DCF_CACHE_ELMT_SIZE;
	} else {
		if (ToDCFElmtOffset(StartID,  &elmt, &offset, IsFolder) != E_OK) {
			return 0;
		}
	}

	if (IsFolder) {
		uiBitMask = DCF_DIR_CACHE_BITS;
		offsetUnit = DCF_DIR_TYPE_BITS;
	} else {
#if DCF_CHK_VALID_FMT
		uiBitMask = DCF_FMT_VALID_BIT;
#else
		uiBitMask = DCF_FILE_CACHE_BITS ;
#endif
		offsetUnit = DCF_FILE_TYPE_BITS;
	}

	// search in the same element
	if (offset) {
		do {
			offset -= offsetUnit;
			tmp = pCache[elmt] & (uiBitMask << offset);

			if (IsExist) {
				// find exist and exist one
				if (tmp) {
					return ToDCFID(elmt, offset, IsFolder);
				}
			} else {
				// find free and find free only
				if (!tmp) {
					return ToDCFID(elmt, offset, IsFolder);
				}
			}
		} while (offset);
	}

	if (elmt == 0) { // elmt ==0 should finished in previous code
		return 0;
	}

	// search in previosu element
	if (IsExist) {
		noMatch = 0;
	} else {
		noMatch = 0xffffffff;
	}

	do {
		elmt--;
		tmp = pCache[elmt];
		if (tmp != noMatch) {
			offset = DCF_CACHE_ELMT_SIZE;
			do {
				offset  -= offsetUnit;
				if (IsExist) {
					if (tmp & (uiBitMask << offset)) {
						return ToDCFID(elmt, offset, IsFolder);
					}
				} else {
					if (!(tmp & (uiBitMask << offset))) {
						return ToDCFID(elmt, offset, IsFolder);
					}
				}
			} while (offset);
		}
	} while (elmt);
	return 0;

}
/*========================================================================
    routine name : GetNextObjID
    function     : Get Next free or exist DCF object ID from current ID
    parameter    :
        [in] const int *pCache: cache of DCF table.
        [in] UINT32 StartID : which ID to start. 0 means to find first ID
        [in] BOOL IsExist: to find free or used ID.
        [in] BOOL IsFoldr: to file folder or file.
    return value : return the ID, return 0 for reach the end of cache and
        doesn't find any one match
========================================================================*/
static UINT32 GetNextObjID(const UINT32 *pCache, UINT32 StartID, BOOL IsExist, BOOL IsFolder)
{
	UINT32 elmt, offset;
	UINT32 tmp, maxElmt, noMatch;
	UINT32 uiBitMask, offsetUnit;


	if (StartID == 0) {
		// find
		if (IsFolder) {
			elmt = 0;
			maxElmt = DCF_DIR_CACHE_SIZE - 1; // minus 1 is because in back of code we add first then compare
			uiBitMask = DCF_DIR_CACHE_BITS;
			offsetUnit = DCF_DIR_TYPE_BITS;
		} else {
			elmt = 0;
			maxElmt = DCF_FILE_CACHE_SIZE - 1;
#if DCF_CHK_VALID_FMT
			uiBitMask = DCF_FMT_VALID_BIT;
#else
			uiBitMask = DCF_FILE_CACHE_BITS ;
#endif
			offsetUnit = DCF_FILE_TYPE_BITS;
		}
		offset = 0;
	} else {
		if (IsFolder) {
			maxElmt = DCF_DIR_CACHE_SIZE - 1;
			uiBitMask = DCF_DIR_CACHE_BITS;
			offsetUnit = DCF_DIR_TYPE_BITS;
			if (StartID == MAX_DCF_DIR_NUM) {
				return 0;
			}
		} else {
			maxElmt = DCF_FILE_CACHE_SIZE - 1;
#if DCF_CHK_VALID_FMT
			uiBitMask = DCF_FMT_VALID_BIT;
#else
			uiBitMask = DCF_FILE_CACHE_BITS ;
#endif
			offsetUnit = DCF_FILE_TYPE_BITS;
			if (StartID == MAX_DCF_FILE_NUM) {
				return 0;
			}
		}
		if (ToDCFElmtOffset(StartID,  &elmt, &offset, IsFolder) != E_OK) {
			return 0;
		}
		offset += offsetUnit;
	}

	// search in the same element

	while (offset < DCF_CACHE_ELMT_SIZE) {
		tmp = pCache[elmt] & (uiBitMask << offset);

		if (IsExist) {
			// find exist and exist one
			if (tmp) {
				return ToDCFID(elmt, offset, IsFolder);
			}
		} else {
			// find free and find free only one
			if (!tmp) {
				return ToDCFID(elmt, offset, IsFolder);
			}
		}
		offset += offsetUnit;
	};

	if (elmt >= maxElmt) { // reach the end of cache
		return 0;
	}

	// search in next element
	if (IsExist) {
		noMatch = 0;
	} else {
		noMatch  = 0xffffffff;
	}

	do {
		elmt++;
		tmp = pCache[elmt];
		if (tmp != noMatch) {
			offset = 0;
			do {
				if (IsExist) {
					if (tmp & (uiBitMask << offset)) {
						return ToDCFID(elmt, offset, IsFolder);
					}
				} else {
					if (!(tmp & (uiBitMask << offset))) {
						return ToDCFID(elmt, offset, IsFolder);
					}
				}
				offset += offsetUnit;
			} while (offset < DCF_CACHE_ELMT_SIZE);
		}
	} while (elmt < maxElmt);
	return 0;
}

/*========================================================================
    function     : the function will change working directory,
        if there is no other file and dir in system
            it will return 0 to both ackDirID and ackFileID
        if there is no other file but have some empty dirs in system
            it will return 0 to ackFileID and minmum DirID in these empty dir
    parameter    :
        [in] pResrInfo:
            [in] DirectoryID: it will not update this field
            [in] FilenameID : it will not update this field
            [out] DirFileNum: this will be change only if *pChgDir is true;
            [out] MaxFileID : this will be change only if *pChgDir is true;
        [in] IsForward: DCF_IS_FORWARD
        [in] IsExist:
        [in] Offset:
        [out]
            UINT32* pAckDirID, UINT32*pAckFileID , BOOL *pChgDir
    return value :
========================================================================*/
static INT32 DCFGetAckID(PDCF_INFO   pInfo, BOOL IsForward, BOOL IsExist, UINT32 Offset,
						 UINT32 *pAckDirID, UINT32 *pAckFileID, BOOL *pChgDir)
{

	UINT32 ackDir, ackFile, orgDir, orgFile, i;
	BOOL   bFindWhat;
	PDCF_DB_INFO pDcfDbInfo = &pInfo->DCFDbInfo;


	UINT32(*FN_GetDCFObjID)(const UINT32 *, UINT32, BOOL, BOOL);

	if (IsForward) {
		FN_GetDCFObjID = GetNextObjID;
	} else {
		FN_GetDCFObjID = GetPreObjID;
	}

	orgDir = pDcfDbInfo->DirectoryID;
	orgFile = pDcfDbInfo->FilenameID;
	ackDir = orgDir;
	ackFile = orgFile;
	bFindWhat = IsExist ;
	*pChgDir = FALSE;
	if (Offset) {

		while (0 < Offset) {
			//find the next File ID
			ackFile = FN_GetDCFObjID(pInfo->DCFFileTb, orgFile, bFindWhat, DCF_IS_FILE);
			if (!ackFile) {
				*pChgDir = TRUE;
				for (i = 0; i <= pDcfDbInfo->TolDirNum; i++) {
					//find the next DIR ID
					ackDir = FN_GetDCFObjID(pInfo->DCFDirTb, ackDir, bFindWhat, DCF_IS_DIR);
					if (ackDir >= MIN_DCF_DIR_NUM) {
						if (ackDir == orgDir) {
							DCFChangeWorkingDir(pInfo, ackDir, pInfo->DCFFileTb, pDcfDbInfo);
							break;
						}
						if (bFindWhat == DCF_IS_EXIST) {
							if (DCFChangeWorkingDir(pInfo, ackDir, pInfo->DCFFileTb, pDcfDbInfo)) {
								break;
							}
						} else {
							// find empty
							if (!DCFChangeWorkingDir(pInfo, ackDir, pInfo->DCFFileTb, pDcfDbInfo)) {
								break;
							}
						}
					}

				}
				ackFile = FN_GetDCFObjID(pInfo->DCFFileTb, 0, bFindWhat, DCF_IS_FILE);
			}
			Offset--;
			orgDir = ackDir;
			orgFile = ackFile;
		}// end of while(0 < offset)
	} else {
		ackFile = pDcfDbInfo->FilenameID;
		ackDir = pDcfDbInfo->DirectoryID;
	}
	*pAckDirID = ackDir;
	*pAckFileID = ackFile;

	return E_OK;
}

static INT32 DCFGetIDBySequ(PDCF_INFO  pInfo, UINT32 *pDirId, UINT32 *pFileId, UINT32 TotalSequ, UINT32 *pDirFileSequ)
{
	UINT32 i, endDir, idSequ;
	BOOL    bChgDir;
	PDCF_DB_INFO pDcfDbInfo = &pInfo->DCFDbInfo;

	if (!TotalSequ) {
		return E_SYS;    // no file
	}

	idSequ = TotalSequ;
	endDir = MAX_DCF_DIR_NUM - MIN_DCF_DIR_NUM + 1;
	for (i = 0; i < endDir; i++) {
		if (idSequ > pInfo->DCFIDNumTb[i]) {
			idSequ -= pInfo->DCFIDNumTb[i];
		} else {
			break;
		}
	}

	if (i == endDir) {
		return E_SYS;    // no file
	}

	if ((MIN_DCF_DIR_NUM + i) != pDcfDbInfo->DirectoryID) {
		DCFChangeWorkingDir(pInfo, (MIN_DCF_DIR_NUM + i), pInfo->DCFFileTb, &pInfo->DCFDbInfo);
	}

	pDcfDbInfo->FilenameID = 0;

	DCFGetAckID(pInfo, DCF_IS_FORWARD, DCF_IS_EXIST, idSequ, pDirId, pFileId, &bChgDir);
	*pDirFileSequ = idSequ;

	return E_OK;
}
static void DCFAddOneTypeNum(PDCF_INFO  pInfo, UINT32 fileType)
{
	UINT32 i;
	for (i = 0; i < DCF_FILE_TYPE_NUM; i++) {
		if (fileType & pInfo->DcfFiletype[i].fileType) {
			pInfo->TotalFiles[i]++;
		}
	}
}

static void DCFDelOneTypeNum(PDCF_INFO  pInfo, UINT32 fileType)
{
	UINT32 i;
	for (i = 0; i < DCF_FILE_TYPE_NUM; i++) {
		if (fileType & pInfo->DcfFiletype[i].fileType) {
			pInfo->TotalFiles[i]--;
		}
	}
}

/*========================================================================
    routine name : DCFGetMaxIdInCache
    function     : find the maxmum exist ID in cache
    parameter    :
        [in]const int *pCache: DCF file cache
        [in]BOOL IsFolder: is file or folder
    return value :  return the max ID, it will return 0 if no exist ID
========================================================================*/
static UINT32 DCFGetMaxIdInCache(const int *pCache, BOOL IsFolder, UINT32 fromWhichID)
{
	UINT32 elmt, offset, tmp;
	UINT32 uwBitMask, shtbits;;

	if (IsFolder) {
		elmt = DCF_DIR_CACHE_SIZE - 1;
		shtbits = DCF_DIR_TYPE_BITS;
		uwBitMask = DCF_DIR_CACHE_BITS;
	} else {
		//#NT#2012/10/29#Lincy Lin -begin
		//#NT#Fine tune performance
		//elmt = DCF_FILE_CACHE_SIZE-1;
		elmt = (DCF_FILE_TYPE_BITS * (fromWhichID - 1) / 32);
		//#NT#2012/10/29#Lincy Lin -end
		shtbits = DCF_FILE_TYPE_BITS;
#if DCF_CHK_VALID_FMT
		uwBitMask = DCF_FMT_VALID_BIT; //only find valid format
#else
		uwBitMask = DCF_FILE_CACHE_BITS;
#endif
	}
	do {
		tmp = (UINT32)pCache[elmt];

		if (tmp) {
			// find it
			offset = DCF_CACHE_ELMT_SIZE;
			while (offset) {
				offset -= shtbits;
				if (tmp & (uwBitMask << offset)) {
					// get the offset
					return ToDCFID(elmt, offset, IsFolder);
				}
			}
		}
	} while (elmt--); // end of while

	return 0;
}


/*========================================================================
    routine name : DCFUpdateDirInfo
    function     :  udpate member of reserved info
    parameter    :
        pResrInfo
            [out] DirFileSequ,
                DirectoryID,
                FilenameID ,
                FileFormat ,
                "DirFileNum" : only update while ackDir doesn't equal to original DirectoryID
                MaxFileID
            [in] ackDir
            [in] ackFile
                 fileFormat,

    return value :
========================================================================*/
static void DCFUpdateDirInfo(PDCF_INFO  pInfo, UINT32 ackDir, UINT32 ackFile, UINT32 fileFormat)
{
	UINT32 fileID, dirID;
	PDCF_DB_INFO pDcfDbInfo = &pInfo->DCFDbInfo;

	dirID = ackDir;
	fileID = ackFile;
	if ((ackDir < MIN_DCF_DIR_NUM) || (ackDir > MAX_DCF_DIR_NUM)) {
		// valid folde ID
		dirID = MIN_DCF_DIR_NUM;
	}
	if ((ackFile < MIN_DCF_FILE_NUM) || (ackFile > MAX_DCF_FILE_NUM)) {
		fileID = 0; // note here we set act file as zero, no file exist the ack file is zero
	}

	if (dirID != pDcfDbInfo->DirectoryID) {
		// here we will get MaxfileID and file number in dir
		DCFChangeWorkingDir(pInfo, dirID, pInfo->DCFFileTb, pDcfDbInfo);
	} else {
		// in the same folder we need to update maxmum ID
		pDcfDbInfo->MaxFileID = DCFGetMaxIdInCache((const int *)pInfo->DCFFileTb, DCF_IS_FILE, MAX_DCF_FILE_NUM);
	}
	pDcfDbInfo->DirectoryID = dirID;
	pDcfDbInfo->FilenameID = fileID;
	pDcfDbInfo->TolIDSequ = DCFGetIDSequ(pInfo, dirID, fileID, &(pDcfDbInfo->DirFileSequ));

	// this line need to change, but for project time, we left it here
	pDcfDbInfo->FileFormat = fileFormat;

#if DCF_CHK_VALID_FMT
	pDcfDbInfo->IdFormat = (GetDCFNumCache(pInfo, fileID, DCF_IS_FILE) & (~DCF_FMT_VALID_BIT));
#else
	pDcfDbInfo->IdFormat = GetDCFNumCache(pInfo, fileID, DCF_IS_FILE);
#endif
}

void DCF_DumpInfoEx(DCF_HANDLE DcfHandle)
{
	UINT32 i, tmp;
	CHAR   tmpName[DCF_FULL_FILE_PATH_LEN];
	char   tmpDirName[DCF_DIR_NAME_LEN + 1];
	CHAR   tmpStr[DCF_FILE_TYPE_NUM * 12] = {0};
	PDCF_DB_INFO pDcfDbInfo;
	PDCF_INFO   pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return;
	}
	pDcfDbInfo = &pInfo->DCFDbInfo;
	DBG_DUMP("[dump]----------------DCF DB info begin----------------\r\n");
	DBG_DUMP("[dump]----\tAccessFilename=%s\r\n", pDcfDbInfo->AccessFilename);
	DBG_DUMP("[dump]----\tTolFileNum=%d\tTolDirNum=%d\r\n", pDcfDbInfo->TolFileNum, pDcfDbInfo->TolDirNum);
	DBG_DUMP("[dump]----\tTolIDSequ=%d\r\n", pDcfDbInfo->TolIDSequ);
	DBG_DUMP("[dump]----\tNextDirID=%d\tNextFileID=%d\r\n", pDcfDbInfo->NextDirId, pDcfDbInfo->NextFileId);
	DBG_DUMP("[dump]----\tMaxDirID=%d\tMaxSysFileID=%d\r\n", pDcfDbInfo->MaxDirID, pDcfDbInfo->MaxSystemFileID);
	DBG_DUMP("[dump]----\tCurDirID=%d\tCurFileID=%d\tMaxFileID=%d\r\n", pDcfDbInfo->DirectoryID, pDcfDbInfo->FilenameID, pDcfDbInfo->MaxFileID);
	DBG_DUMP("[dump]----\tDirFileNum=%d\tDirFileSequ=%d\r\n", pDcfDbInfo->DirFileNum, pDcfDbInfo->DirFileSequ);
	DBG_DUMP("[dump]----\tIs9999=%d\r\n", pDcfDbInfo->Is9999);
	DBG_DUMP("[dump]----\tFileFormat=0x%x\r\n", pDcfDbInfo->FileFormat);
	DBG_DUMP("[dump]----\tIdFormat=0x%x\r\n", pDcfDbInfo->IdFormat);
	DBG_DUMP("[dump]----\tValidFileType=0x%x\tDepFileFormat=0x%x\r\n", pInfo->ValidFileFormat, pInfo->DepFileFormat);
	DBG_DUMP("[dump]----\tOurDirName=%s\t\r\n", pInfo->OurDCFDirName);
	sprintf(tmpStr, "%s=%s", pInfo->DcfFiletype[0].ExtStr, pInfo->DcfFiletype[0].FreeChars);
	#if 0//mark for CID 130820
	for (i = 1; i < DCF_FILE_TYPE_NUM; i++) {
		sprintf(tmpStr, "%s,%s=%s", tmpStr, pInfo->DcfFiletype[i].ExtStr, pInfo->DcfFiletype[i].FreeChars);
	}
	sprintf(tmpStr, "%s\r\n", tmpStr);
	DBG_DUMP("[dump]----\tOurFileName:%s", tmpStr);

	sprintf(tmpStr, "%s=%ld", pInfo->DcfFiletype[0].ExtStr, pInfo->TotalFiles[0]);
	for (i = 1; i < DCF_FILE_TYPE_NUM; i++) {
		sprintf(tmpStr, "%s,%s=%ld", tmpStr, pInfo->DcfFiletype[i].ExtStr, pInfo->TotalFiles[i]);
	}
	sprintf(tmpStr, "%s\r\n", tmpStr);
	#endif
	DBG_DUMP("[dump]----\t%s", tmpStr);

#if DCF_SUPPORT_RESERV_DATA_STORE
	DBG_DUMP("[dump]----\tResvData Total=%d  LastObj=%s\r\n", pInfo->DcfResvData.TotalDCFObjectCount, pInfo->DcfResvData.LastObjPath);
#endif
	for (i = MIN_DCF_DIR_NUM; i <= MAX_DCF_DIR_NUM; i++) {
		tmp = DCFGetIDNumInDir(pInfo, i);
		if (tmp) {
			//GetDCFFolderName(pInfo, tmpName, i);
			if (pInfo->DCFDirName[i - MIN_DCF_DIR_NUM][0] == 0) {
				memcpy(pInfo->DCFDirName[i - MIN_DCF_DIR_NUM], pInfo->OurDCFDirName, DCF_DIR_NAME_LEN);
			}
			memset(tmpDirName, 0, DCF_DIR_NAME_LEN + 1);
			memcpy(tmpDirName, pInfo->DCFDirName[i - MIN_DCF_DIR_NUM], DCF_DIR_NAME_LEN);
			sprintf(tmpName, "%03d%s", (int)(i), tmpDirName);
			DBG_DUMP("[dump]----\t Dir (%s) fileNum = %d\r\n", tmpName, tmp);
		}
	}
	DBG_DUMP("[dump]----------------DCF DB info end----------------\r\n");
}



BOOL DCF_SetParm(DCF_PRMID parmID, UINT32 value)
{
	BOOL ret = TRUE;

	//DCF_lock(pInfo);

	DBG_IND("parmID=%d,value=%d\r\n", parmID, value);
	switch (parmID) {
	case DCF_PRMID_REMOVE_DUPLICATE_FOLDER:
		gbSkipDupDir = value;
		break;

	case DCF_PRMID_REMOVE_DUPLICATE_FILE:
		gbSkipDupFile = value;
		break;

	case DCF_PRMID_INIT_INDEX_TO_LAST:
		gbInitIndexLast = value;
		break;

	case DCF_PRMID_SET_VALID_FILE_FMT:
		if (value > DCF_FILE_TYPE_ANYFORMAT) {
			DBG_ERR("parmID=%d,value=%d\r\n", parmID, value);
			ret = FALSE;
		} else {
			gValidFileFormat = value;
		}
		break;
	case DCF_PRMID_SET_DEP_FILE_FMT:
		if (value > DCF_FILE_TYPE_ANYFORMAT) {
			DBG_ERR("parmID=%d,value=%d\r\n", parmID, value);
			ret =  FALSE;
		} else {
			gDepFileFormat = value;
		}
		break;

	case DCF_PRMID_REMOVE_HIDDEN_ITEM:
		gbSkipHidden = value;
		break;

	default:
		DBG_ERR("parmID(%d)\r\n", parmID);
		ret =  FALSE;
		break;
	}
	//DCF_unlock(pInfo);

	return ret;
}

void DCF_SetDirFreeChars(CHAR baChars[])
{
	UINT32 i;
	//DCF_lock(pInfo);
	for (i = 0; i < DCF_DIR_NAME_LEN; i++) {
		if ((!isalpha(baChars[i])) && (baChars[i] != '_') && (!isdigit(baChars[i]))) {
			DBG_ERR("baChars[i]='%c' is invalid\r\n", baChars[i]);
			//DCF_unlock(pInfo);
			return;
		}
	}
	memcpy(gOurDCFDirName, baChars, DCF_DIR_NAME_LEN);
	//DCF_unlock(pInfo);
}

void DCF_SetFileFreeChars(UINT32 fileType, CHAR baChars[])
{
	UINT32 i;
	//DCF_lock(pInfo);

	if (baChars) {
		for (i = 0; i < DCF_FILE_NAME_LEN; i++) {
			if ((!isalpha(baChars[i])) && (baChars[i] != '_') && (!isdigit(baChars[i]))) {
				DBG_ERR("baChars[i]='%c' is invalid\r\n", baChars[i]);
				//DCF_unlock(pInfo);
				return;
			}
		}
	}
	for (i = 0; i < DCF_FILE_TYPE_NUM; i++) {
		if (fileType & gDcfFiletype[i].fileType) {
			if (baChars) {
				memcpy(gDcfFiletype[i].FreeChars, baChars, DCF_FILE_NAME_LEN);
			} else {
				memset(gDcfFiletype[i].FreeChars, 0x00, DCF_FILE_NAME_LEN);
			}
		}
	}
	//DCF_unlock(pInfo);
}

static DCF_HANDLE DCF_GetFreeHandle(void)
{
	UINT32 i;
	for (i = 0; i < DCF_HANDLE_NUM; i++) {
		if (!(gDCFHandleInfo[i].InitTag == DCF_INITED_TAG)) {
			return i;
		}
	}
	return DCF_OPEN_ERROR;
}


static PDCF_INFO DCF_GetInfoByHandle(DCF_HANDLE DcfHandle)
{
	if (DcfHandle < 0 || DcfHandle >= DCF_HANDLE_NUM) {
		DBG_ERR("DcfHandle (%d) Exceeds DCF_HANDLE_NUM\r\n", DcfHandle);
		return NULL;
	}
	if (!(gDCFHandleInfo[DcfHandle].InitTag == DCF_INITED_TAG)) {
		DBG_ERR("DcfHandle (%d) is not opend, tag =0x%x\r\n", DcfHandle, gDCFHandleInfo[DcfHandle].InitTag);
		return NULL;
	}
	if (!(gDCFHandleInfo[DcfHandle].pDcfInfo->HeadTag == gDCFHandleTag[DcfHandle])) {
		DBG_ERR("Dcf Handle %d Data may be overwritted, Address = 0x%x, Headtag =0x%x\r\n", DcfHandle, (unsigned int)&gDCFHandleInfo[DcfHandle].pDcfInfo->HeadTag, gDCFHandleInfo[DcfHandle].pDcfInfo->HeadTag);
		return NULL;
	}
	if (!(gDCFHandleInfo[DcfHandle].pDcfInfo->EndTag == gDCFHandleTag[DcfHandle])) {
		DBG_ERR("Dcf Handle %d Data may be overwritted, Address = 0x%x, Endtag =0x%x\r\n", DcfHandle, (unsigned int)&gDCFHandleInfo[DcfHandle].pDcfInfo->EndTag, gDCFHandleInfo[DcfHandle].pDcfInfo->EndTag);
		return NULL;
	}
	return gDCFHandleInfo[DcfHandle].pDcfInfo;
}



DCF_HANDLE DCF_Open(PDCF_OPEN_PARM pParm)
{
	DCF_HANDLE  DcfHandle;
	PDCF_INFO   pInfo;


	if (!SEMID_DCF_COMM) {
		DBG_ERR("FLG_ID is not installed\r\n");
		return DCF_OPEN_ERROR;
	}
	if (!pParm) {
		DBG_ERR("pParm is NULL\r\n");
		return DCF_OPEN_ERROR;
	}
	if (!pParm->WorkbuffAddr) {
		DBG_ERR("WorkbuffAddr is NULL\r\n");
		return DCF_OPEN_ERROR;
	}
	if (pParm->WorkbuffSize < sizeof(DCF_INFO)) {
		DBG_ERR("WorkbuffSize %d < need %d\r\n", pParm->WorkbuffSize, sizeof(DCF_INFO));
		return DCF_OPEN_ERROR;
	}
	if (pParm->Drive < 'A' || pParm->Drive > 'Z') {
		DBG_ERR("Drive %c is invalid\r\n", pParm->Drive);
		return DCF_OPEN_ERROR;
	}
	DCF_lockComm();
	DcfHandle = DCF_GetFreeHandle();
	if (DcfHandle == DCF_OPEN_ERROR) {
		DBG_ERR("All Handles are all used\r\n");
		DCF_unlockComm();
		return DCF_OPEN_ERROR;
	}
	DBG_IND("[api] DcfHandle=%d, Drive=%c, workBuf = 0x%x, size = 0x%x\r\n", DcfHandle, pParm->Drive, pParm->WorkbuffAddr, pParm->WorkbuffSize);
	gDCFHandleInfo[DcfHandle].InitTag = DCF_INITED_TAG;
	gDCFHandleInfo[DcfHandle].pDcfInfo = (PDCF_INFO)pParm->WorkbuffAddr;
	pInfo = gDCFHandleInfo[DcfHandle].pDcfInfo;
	pInfo->HeadTag = gDCFHandleTag[DcfHandle];
	pInfo->EndTag = gDCFHandleTag[DcfHandle];
	pInfo->Drive = pParm->Drive;
	pInfo->bIsScan = FALSE;
	memset(&pInfo->DCFDbInfo, 0, sizeof(DCF_DB_INFO));
	sprintf(pInfo->PathDCIM, "%c:\\DCIM\\", pParm->Drive);
	pInfo->DcfCbFP = NULL;
	pInfo->SemidDcf = SEMID_DCF[DcfHandle];
	pInfo->SemidDcfNextId = SEMID_DCFNEXTID[DcfHandle];
	pInfo->Flagid = FLG_ID_DCF[DcfHandle];
	DCF_unlockComm();
	return DcfHandle;
}

void DCF_Close(DCF_HANDLE DcfHandle)
{
	PDCF_INFO   pInfo;

	DBG_IND("[api] DcfHandle=%d\r\n", DcfHandle);

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return;
	}
	DCF_lockComm();
	gDCFHandleInfo[DcfHandle].InitTag = 0;
	gDCFHandleInfo[DcfHandle].pDcfInfo = 0;
	pInfo->HeadTag = 0;
	pInfo->EndTag = 0;
	DCF_unlockComm();
	DBG_IND("handle %d ok!!", DcfHandle);
}



static void DCF_internalScanObj(PDCF_INFO   pInfo, BOOL keepIndex)
{
	UINT32        i, j;
	BOOL          bIsFindLast = FALSE;
	BOOL          bIsSetNextID = FALSE;
	DCF_DB_INFO  *pDCFDbInfo = &pInfo->DCFDbInfo;
	SDCF_OBJ     *pDcfLastObj = &pInfo->DcfLastObj;


	clr_flg(pInfo->Flagid, FLGDCF_SET_NEXT_ID | FLGDCF_LAST_FILE_FIND | FLGDCF_SCAN_COMPLETE);
	// set initial value
	pInfo->bIsScan = TRUE;
	memset(pDCFDbInfo, 0, sizeof(DCF_DB_INFO));
	memset(pInfo->DCFDirTb, 0, sizeof(pInfo->DCFDirTb));
	memset(pInfo->DCFFileTb, 0, sizeof(pInfo->DCFFileTb));
	memset(pInfo->DCFIDNumTb, 0, sizeof(pInfo->DCFIDNumTb));
	memset(pInfo->DCFDirName, 0, sizeof(pInfo->DCFDirName));
	memset(pInfo->DCFDirAttrib, 0, sizeof(pInfo->DCFDirAttrib));
	memset(pInfo->DCFFileName, 0, sizeof(pInfo->DCFFileName));
	memset(pInfo->TotalFiles, 0, sizeof(pInfo->TotalFiles));
	memset(pDcfLastObj, 0, sizeof(SDCF_OBJ));
	pInfo->ValidFileFormat = gValidFileFormat;
	pInfo->DepFileFormat = gDepFileFormat;
	pInfo->bSkipDupDir = gbSkipDupDir;
	pInfo->bSkipDupFile = gbSkipDupFile;
	pInfo->bInitIndexLast = gbInitIndexLast;
	pInfo->bSkipHidden = gbSkipHidden;
	pInfo->DcfFiletype = gDcfFiletype;

	//pInfo->OurDCFDirName = gOurDCFDirName;
	memcpy(pInfo->OurDCFDirName, gOurDCFDirName, sizeof(pInfo->OurDCFDirName));

	// count how manay DCF folder in system
	DCFCountFolder(pInfo, &pDCFDbInfo->TolDirNum, &pDCFDbInfo->MaxDirID);

	if (pDCFDbInfo->MaxDirID) {
		for (i = pDCFDbInfo->MaxDirID ; i >= MIN_DCF_DIR_NUM ; i--) {
			if (GetDCFNumCache(pInfo, (UINT32)i, DCF_IS_DIR)) {
				GetDCFFolderName(pInfo, pDCFDbInfo->AccessFilename, i);
				DCFCountFile(pInfo, pDCFDbInfo->AccessFilename, pInfo->DCFFileTb, DCF_DONT_CACHE, &(pDCFDbInfo->DirFileNum), &(pDCFDbInfo->MaxFileID));
				DCFSetIDNumInDir(pInfo, i, pDCFDbInfo->DirFileNum);
				pDCFDbInfo->DirectoryID = i;

				pDCFDbInfo->TolFileNum += pDCFDbInfo->DirFileNum;

				for (j = 0; j < DCF_FILE_TYPE_NUM; j++) {
					pInfo->TotalFiles[j] += pInfo->TotalFilesInFolder[j];
				}
				// set next ID when find last object , the last obejct may be not valid file type
				// if ((!bIsSetNextID && pInfo->MaxSystemFileId) || (!bIsSetNextID && i == MIN_DCF_DIR_NUM) )

				// set next id when max folder scan complete
				if (!bIsSetNextID) {
					bIsSetNextID = TRUE;
					//#NT#2013/01/08#Nestor Yang -begin
					//#NT# remove unused code, since pDCFDbInfo->MaxDirID is always the largest
					//if (pInfo->MaxInvalidDirID > pDCFDbInfo->MaxDirID)
					//{
					//    pDCFDbInfo->MaxDirID = pInfo->MaxInvalidDirID;
					//}
					//#NT#2013/01/08#Nestor Yang -end
					pDCFDbInfo->MaxSystemFileID = pInfo->MaxSystemFileId;
					if (pDCFDbInfo->MaxDirID == MAX_DCF_DIR_NUM) {
						if (pInfo->MaxInvalidDirID == MAX_DCF_DIR_NUM) {
							DBG_IND("the 999 folder is invalid folder\r\n");
							pDCFDbInfo->Is9999 = TRUE;
							DCF_internalSetNextID(pInfo, ERR_DCF_DIR_NUM, ERR_DCF_FILE_NUM);
						} else if (strncmp(pInfo->DCFDirName[pDCFDbInfo->MaxDirID - MIN_DCF_DIR_NUM], pInfo->OurDCFDirName, DCF_DIR_NAME_LEN) != 0) {
							DBG_IND("the 999 folder is not our folder name\r\n");
							pDCFDbInfo->Is9999 = TRUE;
							DCF_internalSetNextID(pInfo, ERR_DCF_DIR_NUM, ERR_DCF_FILE_NUM);
						} else if (pDCFDbInfo->MaxSystemFileID == MAX_DCF_FILE_NUM) {
							DBG_IND("the 999-9999 file exist\r\n");
							pDCFDbInfo->Is9999 = TRUE;
							DCF_internalSetNextID(pInfo, ERR_DCF_DIR_NUM, ERR_DCF_FILE_NUM);
						} else {
							DCF_internalSetNextID(pInfo, pDCFDbInfo->MaxDirID, pDCFDbInfo->MaxSystemFileID + 1);
						}
					} else {
						if (pInfo->MaxInvalidDirID == pDCFDbInfo->MaxDirID) {
							DBG_IND("the Max folder is invalid folder\r\n");
							DCF_internalSetNextID(pInfo, pDCFDbInfo->MaxDirID + 1, MIN_DCF_FILE_NUM);
						} else if (strncmp(pInfo->DCFDirName[pDCFDbInfo->MaxDirID - MIN_DCF_DIR_NUM], pInfo->OurDCFDirName, DCF_DIR_NAME_LEN) != 0) {
							DBG_IND("the Max folder is not our folder name\r\n");
							DCF_internalSetNextID(pInfo, pDCFDbInfo->MaxDirID + 1, MIN_DCF_FILE_NUM);
						} else if (pDCFDbInfo->MaxSystemFileID == MAX_DCF_FILE_NUM) {
							DBG_IND("the 9999 file exist\r\n");
							DCF_internalSetNextID(pInfo, pDCFDbInfo->MaxDirID + 1, MIN_DCF_FILE_NUM);
						} else {
							DCF_internalSetNextID(pInfo, pDCFDbInfo->MaxDirID, pDCFDbInfo->MaxSystemFileID + 1);
						}
					}
					set_flg(pInfo->Flagid, FLGDCF_SET_NEXT_ID);
				}
				// Find last obejct & this object is valid file type
				if (!bIsFindLast && pDCFDbInfo->DirFileNum) {
					bIsFindLast = TRUE;
					// copy last object info begin
					pDcfLastObj->bIsValid = TRUE;
					pDcfLastObj->DirID = i;
					pDcfLastObj->FileID = pDCFDbInfo->MaxFileID;
					memcpy(pDcfLastObj->DirName, pInfo->DCFDirName[i - MIN_DCF_DIR_NUM], DCF_DIR_NAME_LEN);
					memcpy(pDcfLastObj->FileName, pInfo->DCFFileName[pDcfLastObj->FileID - MIN_DCF_FILE_NUM], DCF_FILE_NAME_LEN);
					pDcfLastObj->FileType = GetDCFNumCache(pInfo, pDcfLastObj->FileID, DCF_IS_FILE);
					// copy last object info end
					set_flg(pInfo->Flagid, FLGDCF_LAST_FILE_FIND);
					if (pInfo->DcfCbFP) {
						pInfo->DcfCbFP(DCF_CBMSG_LAST_FILE_FIND, NULL);
					}
				}
			}
		}
		if (!keepIndex) {
			if (pDCFDbInfo->TolFileNum) {
				if (pInfo->bInitIndexLast) {
					DCF_internalSetCurIndex(pInfo, pDCFDbInfo->TolFileNum);
				} else {
					DCF_internalSetCurIndex(pInfo, DCF_INDEX_FIRST);
				}
			}
		}
	} else {
		DBG_IND("No DCF folder in system.\r\n");
		pDCFDbInfo->MaxDirID = MIN_DCF_DIR_NUM;
		DCF_internalSetNextID(pInfo, MIN_DCF_DIR_NUM, MIN_DCF_FILE_NUM);
	}
	//DCFDumpInfo();
	set_flg(pInfo->Flagid, FLGDCF_SET_NEXT_ID | FLGDCF_LAST_FILE_FIND | FLGDCF_SCAN_COMPLETE);
	if (pInfo->DcfCbFP) {
		pInfo->DcfCbFP(DCF_CBMSG_SCAN_COMPLETE, NULL);
	}
}

void DCF_ScanObjEx(DCF_HANDLE DcfHandle)
{
	PDCF_INFO   pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return;
	}
	DCF_lock(pInfo);
	DCF_internalScanObj(pInfo, FALSE);
	DCF_unlock(pInfo);
}

/*========================================================================
    routine name : DCF_GetNextID
    function     : get write next id
    parameter    : UINT32 *uiFolderId,UINT32 *uiFileId

    return value :
========================================================================*/
BOOL DCF_GetNextIDEx(DCF_HANDLE DcfHandle, UINT32 *DirID, UINT32 *FileID)
{
	FLGPTN uiFlag;
	PDCF_DB_INFO pDcfDbInfo;
	PDCF_INFO    pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return FALSE;
	}
	pDcfDbInfo = &pInfo->DCFDbInfo;
	if (pInfo->bIsScan) {
		// should wait last file find , then can get next ID
		// the last file find flag is set in DCF scan
		wai_flg(&uiFlag, pInfo->Flagid, FLGDCF_SET_NEXT_ID, TWF_ORW);
		*DirID =  pDcfDbInfo->NextDirId ;
		*FileID = pDcfDbInfo->NextFileId ;
		DBG_IND("uiDirId=%d,uiFileId=%d\r\n", pDcfDbInfo->NextDirId, pDcfDbInfo->NextFileId);
		if (pDcfDbInfo->NextDirId == ERR_DCF_DIR_NUM && pDcfDbInfo->NextFileId == ERR_DCF_FILE_NUM) {
			DBG_ERR("uiDirId=%d,uiFileId=%d\r\n", pDcfDbInfo->NextDirId, pDcfDbInfo->NextFileId);
			return FALSE;
		} else {
			return TRUE;
		}
	} else {
		DBG_ERR("DCF is not Scanned\r\n");
		return FALSE;
	}
}

/*========================================================================
    routine name : DCF_SetNextID
    function     : set write next id
    parameter    : UINT32 uiFolderId,UINT32 uiFileId

    return value :
========================================================================*/
BOOL DCF_SetNextIDEx(DCF_HANDLE DcfHandle, UINT32 DirID, UINT32 FileID)
{
	BOOL ret;
	PDCF_INFO    pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return FALSE;
	}
	DCF_lock(pInfo);
	ret = DCF_internalSetNextID(pInfo, DirID, FileID);
	DCF_unlock(pInfo);
	return ret;
}

BOOL DCF_LockNextIDEx(DCF_HANDLE DcfHandle, UINT32 *DirID, UINT32 *FileID)
{
	BOOL rtn;
	PDCF_INFO    pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return FALSE;
	}

	DBG_FUNC_BEGIN("[lock]\r\n");
	wai_sem(pInfo->SemidDcfNextId);
	rtn = DCF_GetNextID(DirID, FileID);
	DBG_FUNC_END("[lock]\r\n");
	return rtn;
}

void DCF_UnlockNextIDEx(DCF_HANDLE DcfHandle)
{
	PDCF_INFO    pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return;
	}
	DBG_IND("[lock]\r\n");
	sig_sem(pInfo->SemidDcfNextId);
}


static BOOL DCF_internalSetNextID(PDCF_INFO  pInfo, UINT32 DirID, UINT32 FileID)
{
	BOOL ret = TRUE;
	PDCF_DB_INFO pDcfDbInfo = &pInfo->DCFDbInfo;

	if (DirID == ERR_DCF_DIR_NUM && FileID == ERR_DCF_FILE_NUM) {
		pDcfDbInfo->NextDirId = DirID;
		pDcfDbInfo->NextFileId = FileID;
		DBG_ERR("DCF out of ID\r\n");
	} else if (DirID < MIN_DCF_DIR_NUM || DirID > MAX_DCF_DIR_NUM) {
		DBG_ERR("uiFolderId(%d)\r\n", DirID);
		ret = FALSE;
	} else if (FileID < MIN_DCF_FILE_NUM || FileID > MAX_DCF_FILE_NUM) {
		DBG_ERR("uiFileId(%d)\r\n", FileID);
		ret = FALSE;
	} else {
		pDcfDbInfo->NextDirId = DirID;
		pDcfDbInfo->NextFileId = FileID;
	}

	DBG_IND("DirID=%d,FileID=%d\r\n", pDcfDbInfo->NextDirId, pDcfDbInfo->NextFileId);
	return ret;
}





UINT32 DCF_GetDBInfoEx(DCF_HANDLE DcfHandle, DCF_INFOID infoID)
{
	UINT32 value = 0;
	PDCF_DB_INFO pDcfDbInfo;
	PDCF_INFO   pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return 0;
	}
	pDcfDbInfo = &pInfo->DCFDbInfo;
	switch (infoID) {
	case DCF_INFO_TOL_FILE_COUNT:
		value = pDcfDbInfo->TolFileNum;
		break;
	case DCF_INFO_TOL_DIR_COUNT:
		value =  pDcfDbInfo->TolDirNum;
		break;
	case DCF_INFO_CUR_INDEX:
		value =  pDcfDbInfo->TolIDSequ;
		break;
	case DCF_INFO_CUR_DIR_ID:
		value =  pDcfDbInfo->DirectoryID;
		break;
	case DCF_INFO_CUR_FILE_ID:
		value =  pDcfDbInfo->FilenameID;
		break;
	case DCF_INFO_MAX_DIR_ID:
		value =  pDcfDbInfo->MaxDirID;
		break;
	case DCF_INFO_MAX_FILE_ID:
		value =  pDcfDbInfo->MaxFileID;
		break;
	case DCF_INFO_CUR_FILE_TYPE:
		value =  pDcfDbInfo->IdFormat;
		break;
	case DCF_INFO_MAX_SYS_FILE_ID:
		value =  pDcfDbInfo->MaxSystemFileID;
		break;
	case DCF_INFO_IS_9999:
		value =  pDcfDbInfo->Is9999;
		break;
	case DCF_INFO_DIR_FILE_SEQ:
		value =  pDcfDbInfo->DirFileSequ;
		break;
	case DCF_INFO_DIR_FILE_NUM:
		value =  pDcfDbInfo->DirFileNum;
		break;
	case DCF_INFO_VALID_FILE_FMT:
		value =  pInfo->ValidFileFormat;
		break;
	case DCF_INFO_DEP_FILE_FMT:
		value =  pInfo->DepFileFormat;
		break;
	case DCF_INFO_DIR_FREE_CHARS:
		value = (UINT32)pInfo->OurDCFDirName;
		break;
	case DCF_INFO_FILE_FREE_CHARS:
		value = (UINT32)&pInfo->DcfFiletype[0].FreeChars;
		break;
	default:
		DBG_ERR("infoID(%d)\r\n", infoID);
		break;
	}
	return value;
}


UINT32 DCF_GetOneTypeFileCountEx(DCF_HANDLE DcfHandle, UINT32 fileType)
{
	UINT32 i, count = 0;
	BOOL   hasFound = FALSE;
	PDCF_INFO   pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return 0;
	}
	for (i = 0; i < DCF_FILE_TYPE_NUM; i++) {
		if (fileType == pInfo->DcfFiletype[i].fileType) {
			count = pInfo->TotalFiles[i];
			hasFound = TRUE;
			break;
		}
	}
	if (!hasFound) {
		DBG_ERR("fileType=0x%x\r\n", fileType);
	}
	DBG_IND("fileType=0x%x,count=%d\r\n", fileType, count);
	return count;
}
/*
UINT32 DCF_GetCurIndex(void)
{
    return pDcfDbInfo->TolIDSequ;
}
*/
BOOL DCF_internalSetCurIndex(PDCF_INFO   pInfo, UINT32 index)
{
	UINT32 DirID, FileID;
	INT32  offset;
	INT32  ret = E_OK;
	BOOL   result = TRUE, bChgDir = FALSE, isJump = FALSE;
	PDCF_DB_INFO pDcfDbInfo = &pInfo->DCFDbInfo;

	DBG_IND("index=%d\r\n", index);
	if (index > pDcfDbInfo->TolFileNum || index <= 0) {
		DBG_ERR("index(%d)\r\n", index);
		result = FALSE;
	}

	if (pDcfDbInfo->TolIDSequ != index) {
		//#NT#2012/10/23#Lincy Lin -begin
		//#NT#Fine tune seek performance
		//ret = DCFGetIDBySequ(&DirID, &FileID, index, &(pDcfDbInfo->DirFileSequ));
		offset = index - pDcfDbInfo->TolIDSequ;
		//#NT#2014/04/28#Nestor Yang -begin
		//#NT# Offset to the next ID or the previous ID only when the offset is 1 or -1
		if (1 == offset) {
			ret = DCFGetAckID(pInfo, DCF_IS_FORWARD, DCF_IS_EXIST, 1, &DirID, &FileID, &bChgDir);
		} else if (-1 == offset) {
			ret = DCFGetAckID(pInfo, DCF_IS_BACKWARD, DCF_IS_EXIST, 1, &DirID, &FileID, &bChgDir);
		} else {
			ret = DCFGetIDBySequ(pInfo, &DirID, &FileID, index, &(pDcfDbInfo->DirFileSequ));
			isJump = TRUE;
		}
		//#NT#2014/04/28#Nestor Yang -end
		//#NT#2012/10/23#Lincy Lin -end
		if (ret == E_OK) {
#if 0
			DCFUpdateDirInfo(pInfo, DirID, FileID, GetDCFNumCache(pInfo, FileID, DCF_IS_FILE));
#else
			if (bChgDir) {
				pDcfDbInfo->MaxFileID = DCFGetMaxIdInCache((const int *)pInfo->DCFFileTb, DCF_IS_FILE, MAX_DCF_FILE_NUM);
				pDcfDbInfo->DirectoryID = DirID;
			}
			pDcfDbInfo->FilenameID = FileID;
			if (isJump) {
				pDcfDbInfo->TolIDSequ = index;
			} else {
				pDcfDbInfo->TolIDSequ = DCFGetIDSequ(pInfo, DirID, FileID, &(pDcfDbInfo->DirFileSequ));
			}
			pDcfDbInfo->FileFormat = GetDCFNumCache(pInfo, FileID, DCF_IS_FILE);
			pDcfDbInfo->IdFormat = (pDcfDbInfo->FileFormat & (~DCF_FMT_VALID_BIT));
#endif
			result = TRUE;
		} else {
			DBG_ERR("index(%d)\r\n", index);
			result = FALSE;
		}
		//DCFDumpInfo();
	}
	return result;
}

BOOL DCF_SetCurIndexEx(DCF_HANDLE DcfHandle, UINT32 index)
{
	BOOL ret;
	PDCF_INFO   pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return FALSE;
	}
	DCF_lock(pInfo);
	ret = DCF_internalSetCurIndex(pInfo, index);
	DCF_unlock(pInfo);
	return ret;
}

UINT32 DCF_SeekIndexEx(DCF_HANDLE DcfHandle, INT32 offset, DCF_SEEK_CMD seekCmd)
{
	UINT32 index;
	INT32  lastIndex;
	BOOL   ret;
	INT32  tmpIndex;
	PDCF_INFO   pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return 0;
	}
	if (seekCmd >= DCF_SEEK_MAX_ID) {
		DBG_ERR("offset=%d,seekCmd=%d\r\n", offset, seekCmd);
		ret = FALSE;
	}
	DCF_lock(pInfo);
	index = DCF_GetDBInfo(DCF_INFO_CUR_INDEX);
	lastIndex = DCF_GetDBInfo(DCF_INFO_TOL_FILE_COUNT);

	if (seekCmd == DCF_SEEK_SET) {
		ret = DCF_internalSetCurIndex(pInfo, offset);
		index = offset;
	} else {
		if (seekCmd == DCF_SEEK_CUR) {
			tmpIndex = (INT32)index;
		} else {
			tmpIndex = (INT32)lastIndex;
		}
		tmpIndex += offset;
		if (tmpIndex < DCF_INDEX_FIRST) {
			tmpIndex += lastIndex;
			if (tmpIndex < DCF_INDEX_FIRST || tmpIndex > lastIndex) {
				DBG_ERR("offset=%d,seekCmd=%d\r\n", offset, seekCmd);
				ret = FALSE;
			} else {
				ret = DCF_internalSetCurIndex(pInfo, tmpIndex);
			}
		} else if (tmpIndex > lastIndex) {
			tmpIndex -= lastIndex;
			if (tmpIndex < DCF_INDEX_FIRST || tmpIndex > lastIndex) {
				DBG_ERR("offset=%d,seekCmd=%d\r\n", offset, seekCmd);
				ret = FALSE;
			} else {
				ret = DCF_internalSetCurIndex(pInfo, tmpIndex);
			}
		} else {
			ret = DCF_internalSetCurIndex(pInfo, tmpIndex);
		}
		index = (UINT32)tmpIndex;
	}

	if (ret != TRUE) {
		index = 0;
	}
	DCF_unlock(pInfo);
	return index;
}

UINT32 DCF_SeekIndexByIDEx(DCF_HANDLE DcfHandle, UINT32 DirID, UINT32 FileID)
{
	UINT32 index;
	BOOL   ret = FALSE;
	PDCF_INFO   pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return 0;
	}
	index = DCF_GetIndexByID(DirID, FileID);
	if (index) {
		DCF_lock(pInfo);
		ret = DCF_internalSetCurIndex(pInfo, index);
		DCF_unlock(pInfo);
	}
	return (ret == TRUE) ? index : 0;
}



BOOL DCF_GetObjInfoEx(DCF_HANDLE DcfHandle, UINT32  index, UINT32 *DirID, UINT32 *FileID, UINT32 *fileType)
{
	UINT32 OriIndex;
	BOOL   ret = TRUE;
	PDCF_DB_INFO pDcfDbInfo;
	PDCF_INFO    pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return FALSE;
	}
	pDcfDbInfo = &pInfo->DCFDbInfo;
	if (index == 0 || index > pDcfDbInfo->TolFileNum) {
		DBG_ERR("index=%d,TolFileNum=%d\r\n", index, pDcfDbInfo->TolFileNum);
		return FALSE;
	}
	DCF_lock(pInfo);
	OriIndex = pDcfDbInfo->TolIDSequ;
	if (index != OriIndex) {
		ret = DCF_internalSetCurIndex(pInfo, index);
	}
	if (ret == TRUE) {
		*DirID = pDcfDbInfo->DirectoryID;
		*FileID = pDcfDbInfo->FilenameID;
		*fileType = pDcfDbInfo->IdFormat;
		DBG_IND("index=%d,DirID=%d,fileID=%d,fileType=0x%x\r\n", index, *DirID, *FileID, *fileType);
	} else {
		DBG_ERR("index=%d\r\n", index);
	}
	if (index != OriIndex) {
		DCF_internalSetCurIndex(pInfo, OriIndex);
	}
	DCF_unlock(pInfo);
	return ret;
}
BOOL DCF_GetObjPathEx(DCF_HANDLE DcfHandle, UINT32  index, UINT32 fileType, CHAR *filePath)
{
	UINT32 OriIndex, tmpFileType;
	BOOL   ret = TRUE;
	INT32  len;
	PDCF_DB_INFO pDcfDbInfo;
	PDCF_INFO    pInfo;

	DBG_IND("index=%d,fileType=0x%x\r\n", index, fileType);
	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return FALSE;
	}
	pDcfDbInfo = &pInfo->DCFDbInfo;
	OriIndex = pDcfDbInfo->TolIDSequ;
	if (index == 0 || index > pDcfDbInfo->TolFileNum) {
		DBG_ERR("index=%d,TolFileNum=%d\r\n", index, pDcfDbInfo->TolFileNum);
		return FALSE;
	}

	DCF_lock(pInfo);

	if (index != OriIndex) {
		ret = DCF_internalSetCurIndex(pInfo, index);
	}
	if (ret == TRUE) {
#if DCF_CHK_VALID_FMT
		tmpFileType = GetDCFNumCache(pInfo, pDcfDbInfo->FilenameID, DCF_IS_FILE) & (~DCF_FMT_VALID_BIT);
#else
		tmpFileType = GetDCFNumCache(pInfo, pDcfDbInfo->FilenameID, DCF_IS_FILE);
#endif
		if ((tmpFileType & fileType) == DCF_FILE_TYPE_NOFILE) {
			DBG_ERR("fileType=0x%x,curType=0x%x\r\n", fileType, tmpFileType);
			ret = FALSE;
		} else {
			len = GetDCFFileName(pInfo, filePath, pDcfDbInfo->DirectoryID, pDcfDbInfo->FilenameID, fileType);
			if (len) {
				DBG_IND("path=%s\r\n", filePath);
			} else {
				DBG_ERR("fileType=0x%x\r\n", fileType);
				ret = FALSE;
			}
		}
	} else {
		DBG_ERR("index=%d\r\n", index);
		ret = FALSE;
	}
	if (index != OriIndex) {
		DCF_internalSetCurIndex(pInfo, OriIndex);
	}

	DCF_unlock(pInfo);
	return ret;
}

BOOL   DCF_GetObjPathByIDEx(DCF_HANDLE DcfHandle, UINT32 DirID, UINT32 FileID, UINT32 fileType, CHAR *filePath)
{
	INT32 len;
	BOOL   ret = TRUE;
	PDCF_INFO   pInfo;
	//#NT#2014/04/25#Nestor Yang -begin
	//#NT# DCF id disorder
	UINT32 OriWorkDir = 0;
	//#NT#2014/04/25#Nestor Yang -end

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return FALSE;
	}

	if (DirID < MIN_DCF_DIR_NUM ||
		DirID > MAX_DCF_DIR_NUM ||
		pInfo->DCFDbInfo.DirectoryID < MIN_DCF_DIR_NUM ||
		pInfo->DCFDbInfo.DirectoryID > MAX_DCF_DIR_NUM) {
		return FALSE;
	}

	DCF_lock(pInfo);

	if (pInfo->DCFDbInfo.DirectoryID != DirID) {
		// here we will get MaxfileID and file number in dir
		OriWorkDir = pInfo->DCFDbInfo.DirectoryID;//store the current DirID
		DCFChangeWorkingDir(pInfo, DirID, pInfo->DCFFileTb, &pInfo->DCFDbInfo);
	}
	len = GetDCFFileName(pInfo, filePath, DirID, FileID, fileType);
	if (len) {
		DBG_IND("path=%s\r\n", filePath);
	} else {
		DBG_ERR("fileType=0x%x\r\n", fileType);
		ret = FALSE;
	}
	//#NT#2014/04/25#Nestor Yang -begin
	//#NT# DCF id disorder
	if (OriWorkDir) {
		DCFChangeWorkingDir(pInfo, OriWorkDir, pInfo->DCFFileTb, &pInfo->DCFDbInfo);
	}
	//#NT#2014/04/25#Nestor Yang -end

	DCF_unlock(pInfo);

	return ret;
}

BOOL DCF_GetLastObjInfoEx(DCF_HANDLE DcfHandle, UINT32 *DirID, UINT32 *FileID, UINT32 *fileType)
{
	FLGPTN      uiFlag;
	SDCF_OBJ   *pDcfLastObj;
	PDCF_INFO   pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return FALSE;
	}
	pDcfLastObj = &pInfo->DcfLastObj;
	if (pInfo->bIsScan) {
		// should wait last file find , then can get Last object
		// the last file find flag is set in DCF scan
		wai_flg(&uiFlag, pInfo->Flagid, FLGDCF_LAST_FILE_FIND, TWF_ORW);
		if (pDcfLastObj->bIsValid) {
			*DirID = pDcfLastObj->DirID;
			*FileID = pDcfLastObj->FileID;
			*fileType = (pDcfLastObj->FileType & (~DCF_FMT_VALID_BIT));
			return TRUE;
		} else {
			return FALSE;
		}
	} else {
		DBG_ERR("DCF is not Scanned\r\n");
		return FALSE;
	}
}

BOOL DCF_GetLastObjPathEx(DCF_HANDLE DcfHandle, UINT32 fileType, CHAR *filePath)
{
	CHAR       *pExtName;
	FLGPTN      uiFlag;
	SDCF_OBJ   *pDcfLastObj;
	PDCF_INFO   pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return FALSE;
	}
	pDcfLastObj = &pInfo->DcfLastObj;
	if (pInfo->bIsScan) {
		// should wait last file find , then can get Last object
		// the last file find flag is set in DCF scan
		wai_flg(&uiFlag, pInfo->Flagid, FLGDCF_LAST_FILE_FIND, TWF_ORW);
		if (pDcfLastObj->bIsValid) {
			pExtName = DCFGetExtByType(fileType);
			if (pExtName == NULL) {
				DBG_ERR("fileType=0x%x\r\n", fileType);
				*filePath = '\0';
				return FALSE;
			}
			sprintf(filePath, "%s%03d%s\\%s%04d.", pInfo->PathDCIM,
					(int)(pDcfLastObj->DirID),
					pDcfLastObj->DirName,
					pDcfLastObj->FileName,
					(int)(pDcfLastObj->FileID));
			memcpy(filePath + DCF_PATH_FILE_LENGTH, pExtName, DCF_FILE_EXT_LEN);
			*(filePath + DCF_PATH_FILE_LENGTH + DCF_FILE_EXT_LEN)  = '\0';
			return TRUE;
		} else {
			return FALSE;
		}
	} else {
		DBG_ERR("DCF is not Scanned\r\n");
		return FALSE;
	}
}





UINT32 DCF_GetIndexByIDEx(DCF_HANDLE DcfHandle, UINT32 DirNum, UINT32 FileNum)
{
	UINT32 DirSeq, OriDirNum, index;
	PDCF_INFO   pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return 0;
	}
	if (DirNum < MIN_DCF_DIR_NUM || DirNum > MAX_DCF_DIR_NUM) {
		DBG_ERR("DirNum(%d)\r\n", DirNum);
		return 0;
	} else if (FileNum < MIN_DCF_FILE_NUM || FileNum > MAX_DCF_FILE_NUM) {
		DBG_ERR("FileNum(%d)\r\n", FileNum);
		return 0;
	}
	DCF_lock(pInfo);
	OriDirNum = pInfo->DCFDbInfo.DirectoryID;
	if (pInfo->DCFDbInfo.DirectoryID != DirNum) {
		// here we will get MaxfileID and file number in dir
		DCFChangeWorkingDir(pInfo, DirNum, pInfo->DCFFileTb, &pInfo->DCFDbInfo);
	}
	index = DCFGetIDSequ(pInfo, DirNum, FileNum, &DirSeq);
	if (!index) {
		DBG_ERR("Not found DirNum(%d), FileNum(%d)\r\n", DirNum, FileNum);
	}
	if (OriDirNum != DirNum) {
		DCFChangeWorkingDir(pInfo, OriDirNum, pInfo->DCFFileTb, &pInfo->DCFDbInfo); // here will update our dir id
	}
	DCF_unlock(pInfo);
	return index;
}

BOOL DCF_DelDBDirEx(DCF_HANDLE DcfHandle, CHAR *filePath)
{
	UINT32 DirNum;
	BOOL   ret = TRUE;

	DBG_IND("filePath=%s\r\n", filePath);

	PDCF_INFO   pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return FALSE;
	}
	DCF_lock(pInfo);
	DirNum = DCF_IsValidDir(&filePath[DCF_PATH_DCIM_LENGTH]);
	if (DirNum < MIN_DCF_DIR_NUM || DirNum > MAX_DCF_DIR_NUM) {
		DBG_ERR("DirNum(%d)\r\n", DirNum);
		ret = FALSE;
		goto L_DCF_DelDBDir_Err;
	}
	if (DCFGetIDNumInDir(pInfo, DirNum) == 0) {
		// compare folder free chars name
		if (memcmp(pInfo->DCFDirName[DirNum - MIN_DCF_DIR_NUM], &filePath[DCF_PATH_DIR_FREECHAR_POS], DCF_DIR_NAME_LEN) != 0) {
			DBG_ERR("input folder name not match %s\r\n", filePath);
			ret =  FALSE;
			goto L_DCF_DelDBDir_Err;
		}
		memset(pInfo->DCFDirName[DirNum - MIN_DCF_DIR_NUM], 0x00, DCF_DIR_NAME_LEN);
	} else {
		DBG_ERR("(%s) is not empty\r\n", filePath);
		ret = FALSE;
		goto L_DCF_DelDBDir_Err;
	}
L_DCF_DelDBDir_Err:
	DCF_unlock(pInfo);

	DBG_FUNC_END("\r\n");
	return ret;
}

//BOOL DCF_DelDBfile(UINT32  DirNum, UINT32 FileNum, UINT32 fileType)
BOOL DCF_DelDBfileEx(DCF_HANDLE DcfHandle, CHAR *filePath)
{
	UINT32 /*OriIndex,NewIndex,*/DelIndex, DirFileSequ, tmpFileType, newDir, newFile, OriMaxFileID;
	UINT32 DirNum, FileNum, fileType;
	BOOL   bDelLastFile = FALSE, isChangDir = FALSE, isUpdateDirFileNum = TRUE;
	BOOL   ret = TRUE, bChgDir = FALSE, bBackward = FALSE;
	CHAR   tmpFileName[12] = {0}; // XXXX0001JPG
	PDCF_DB_INFO pDcfDbInfo;
	PDCF_INFO    pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return FALSE;
	}
	pDcfDbInfo = &pInfo->DCFDbInfo;
	DBG_IND("[del]filePath=%s\r\n", filePath);
	DCF_lock(pInfo);

	if (memcmp(pInfo->PathDCIM, filePath, DCF_PATH_DCIM_LENGTH) != 0) {
		DBG_ERR("path %s error\r\n", filePath);
		ret =  FALSE;
		goto L_DCF_DelDBfile_Err;
	}
	DirNum = DCF_IsValidDir(&filePath[DCF_PATH_DCIM_LENGTH]);
	// truncat '.' for   DCF_IsValidFile input filename   XXXX0001.JPG --> XXXX0001JPG
	memcpy(&tmpFileName[0], &filePath[DCF_PATH_DIR_LENGTH], 8);
	memcpy(&tmpFileName[8], &filePath[DCF_PATH_DIR_LENGTH + 9], 3);
	FileNum = DCF_IsValidFile(tmpFileName, tmpFileName + 8, &fileType);
	if (DirNum == 0 || FileNum == 0) {
		DBG_IND("%s,DirNum=%d,FileNum=%d\r\n", filePath, DirNum, FileNum);
		ret =  FALSE;
		goto L_DCF_DelDBfile_Err;
	} else if (DirNum < MIN_DCF_DIR_NUM || DirNum > MAX_DCF_DIR_NUM) {
		DBG_ERR("DirNum(%d)\r\n", DirNum);
		ret = FALSE;
		goto L_DCF_DelDBfile_Err;
	} else if (FileNum < MIN_DCF_FILE_NUM || FileNum > MAX_DCF_FILE_NUM) {
		DBG_ERR("FileNum(%d)\r\n", FileNum);
		ret = FALSE;
		goto L_DCF_DelDBfile_Err;
	} else if (memcmp(pInfo->DCFDirName[DirNum - MIN_DCF_DIR_NUM], &filePath[DCF_PATH_DIR_FREECHAR_POS], DCF_DIR_NAME_LEN) != 0) {
		DBG_ERR("input folder name not match %s\r\n", filePath);
		ret =  FALSE;
		goto L_DCF_DelDBfile_Err;
	}

	if (pInfo->DCFDbInfo.DirectoryID != DirNum) {
		// here we will get MaxfileID and file number in dir
		DCFChangeWorkingDir(pInfo, DirNum, pInfo->DCFFileTb, pDcfDbInfo);
		isChangDir = TRUE;
	}
	//OriIndex = pDcfDbInfo->TolIDSequ;
	//NewIndex = OriIndex;
	if (pDcfDbInfo->DirectoryID == DirNum) {
#if DCF_CHK_VALID_FMT
		tmpFileType = GetDCFNumCache(pInfo, FileNum, DCF_IS_FILE) & (~DCF_FMT_VALID_BIT);
#else
		tmpFileType = GetDCFNumCache(pInfo, FileNum, DCF_IS_FILE);
#endif
		if ((tmpFileType & fileType) == DCF_FILE_TYPE_NOFILE) {
			if (isChangDir) {
				// because the folder is rescan and the DirFileNum is already correct
				isUpdateDirFileNum = FALSE;
			} else {
				DBG_ERR("No This File Dir=%d,File=%d,fileType=0x%x\r\n", DirNum, FileNum, fileType);
				ret = FALSE;
				goto L_DCF_DelDBfile_Err;
			}
		}
		//#NT#2012/10/23#Lincy Lin -begin
		//#NT#Fine tune delete file performance
		if (DirNum == pDcfDbInfo->DirectoryID && FileNum == pDcfDbInfo->FilenameID) {
			DelIndex = pDcfDbInfo->TolIDSequ;
		} else
			//#NT#2012/10/23#Lincy Lin -end
		{
			DelIndex = DCFGetIDSequ(pInfo, DirNum, FileNum, &DirFileSequ);
		}
		DBG_IND("[del]index=%d,fileType=0x%x\r\n", DelIndex, tmpFileType);
		ClearDCFNumCache(pInfo, FileNum, fileType, DCF_IS_FILE);
		DCFDelOneTypeNum(pInfo, fileType);

		DBG_IND("[del]isChangDir = %d, isUpdateDirFileNum =%d\r\n", isChangDir, isUpdateDirFileNum);
		if (GetDCFNumCache(pInfo, FileNum, DCF_IS_FILE) == DCF_FILE_TYPE_NOFILE) {
			if (pDcfDbInfo->TolFileNum == DelIndex) {
				bDelLastFile = TRUE;
				DBG_IND("[del]del last\r\n");
			}
			if (pDcfDbInfo->TolFileNum == pDcfDbInfo->TolIDSequ) {
				bBackward = TRUE;
				DBG_IND("[del]backward\r\n");
			}
			memset(pInfo->DCFFileName[FileNum - MIN_DCF_FILE_NUM], 0, DCF_FILE_NAME_LEN);
			pDcfDbInfo->TolFileNum--;
			if (isUpdateDirFileNum) {
				pDcfDbInfo->DirFileNum--;
				DCFSetIDNumInDir(pInfo, DirNum, pDcfDbInfo->DirFileNum);
			}
			OriMaxFileID = pDcfDbInfo->MaxFileID;
			if (bDelLastFile) {
				if (DirNum == pDcfDbInfo->MaxDirID && FileNum == pDcfDbInfo->MaxSystemFileID) {
					// update 9999
					if (DirNum == MAX_DCF_DIR_NUM && FileNum == MAX_DCF_FILE_NUM) {
						pDcfDbInfo->Is9999 = FALSE;
					}
					// update MaxSystemFileID
					if ((pDcfDbInfo->DirFileNum == 0) && pDcfDbInfo->TolFileNum) {
						pDcfDbInfo->MaxSystemFileID = 0;
					} else {
						pDcfDbInfo->MaxSystemFileID = DCFGetMaxIdInCache((const int *)pInfo->DCFFileTb, DCF_IS_FILE, OriMaxFileID);
					}
					// DCF Numbering is not continue, so set next ID
					//if (!gIsContinueDCFnum)
					if (pDcfDbInfo->MaxDirID >= pDcfDbInfo->NextDirId) {
						DCF_internalSetNextID(pInfo, pDcfDbInfo->MaxDirID, pDcfDbInfo->MaxSystemFileID + 1);
					}
				}
			}
			if (pDcfDbInfo->TolFileNum == 0) {
				pDcfDbInfo->TolIDSequ = 0;
				pDcfDbInfo->MaxFileID = 0;
				pDcfDbInfo->DirectoryID = 0;
				pDcfDbInfo->FilenameID = 0;
				pDcfDbInfo->FileFormat = 0;
				pDcfDbInfo->IdFormat = 0;
			} else {
				if (pDcfDbInfo->TolFileNum == 1) {
					DCFGetIDBySequ(pInfo, &newDir, &newFile, 1, &(pDcfDbInfo->DirFileSequ));
					if (newDir != pDcfDbInfo->DirectoryID) {
						bChgDir = TRUE;
					}
				} else if (bBackward) {
					DCFGetAckID(pInfo, DCF_IS_BACKWARD, DCF_IS_EXIST, 1, &newDir, &newFile, &bChgDir);
				} else {
					DCFGetAckID(pInfo, DCF_IS_FORWARD, DCF_IS_EXIST, 1, &newDir, &newFile, &bChgDir);
				}
				DBG_IND("[del]newDir=%d, newFile=%d, bChgDir=%d\r\n", newDir, newFile, bChgDir);

				if (bChgDir) {
					DCFUpdateDirInfo(pInfo, newDir, newFile, GetDCFNumCache(pInfo, newFile, DCF_IS_FILE));
				} else {
					pDcfDbInfo->MaxFileID = DCFGetMaxIdInCache((const int *)pInfo->DCFFileTb, DCF_IS_FILE, OriMaxFileID);
					pDcfDbInfo->DirectoryID = newDir;
					pDcfDbInfo->FilenameID = newFile;
					pDcfDbInfo->TolIDSequ = DCFGetIDSequ(pInfo, newDir, newFile, &(pDcfDbInfo->DirFileSequ));
					pDcfDbInfo->FileFormat = GetDCFNumCache(pInfo, newFile, DCF_IS_FILE);
					pDcfDbInfo->IdFormat = (pDcfDbInfo->FileFormat & (~DCF_FMT_VALID_BIT));
				}
			}
		}
	}
	// Set Current index will update DCF dir info
	//if (NewIndex > pInfo->DCFDbInfo.TolFileNum)
	//    NewIndex = pInfo->DCFDbInfo.TolFileNum;
	//DCF_internalSetCurIndex(pInfo, NewIndex);

L_DCF_DelDBfile_Err:
	DCF_unlock(pInfo);

	DBG_FUNC_END("[del]\r\n");
	return ret;
}
BOOL DCF_GetDirInfoEx(DCF_HANDLE DcfHandle,  UINT32 DirID, PSDCFDIRINFO psDirInfo)
{
	BOOL ret = TRUE;
	PDCF_INFO   pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return FALSE;
	}
	if (DirID < MIN_DCF_DIR_NUM || DirID > pInfo->DCFDbInfo.MaxDirID) {
		DBG_ERR("DirNum(%d)\r\n", DirID);
		return FALSE;
	} else if (psDirInfo == NULL) {
		DBG_ERR("psDirInfo is NULL\r\n");
		return FALSE;
	}

	memset(psDirInfo->szDirFreeChars, 0, DCF_DIR_NAME_LEN + 1);
	memcpy(psDirInfo->szDirFreeChars, pInfo->DCFDirName[DirID - MIN_DCF_DIR_NUM], DCF_DIR_NAME_LEN);
	if (!psDirInfo->szDirFreeChars[0]) {
		ret = FALSE;
	}
	psDirInfo->uiNumOfDcfObj = DCFGetIDNumInDir(pInfo, DirID);
	psDirInfo->ucAttrib = pInfo->DCFDirAttrib[DirID - MIN_DCF_DIR_NUM];
	DBG_IND("DirID=%d,uiNumOfDcfObj=%d,szDirFreeChars=%s,ucAttributes=0x%x\r\n", DirID, psDirInfo->uiNumOfDcfObj, psDirInfo->szDirFreeChars, psDirInfo->ucAttrib);
	return ret;
}

void DCF_internalRefresh(PDCF_INFO  pInfo, BOOL bKeepIndex)
{
	UINT32 OriIndex;
	PDCF_DB_INFO pDcfDbInfo = &pInfo->DCFDbInfo;
	if (bKeepIndex) {
		OriIndex = pDcfDbInfo->TolIDSequ;
		DCF_internalScanObj(pInfo, bKeepIndex);
		if (pDcfDbInfo->TolFileNum == 0) {
			OriIndex = 0;
		} else if (OriIndex > pDcfDbInfo->TolFileNum) {
			OriIndex = pDcfDbInfo->TolFileNum;
		}
		if (OriIndex) {
			DCF_internalSetCurIndex(pInfo, OriIndex);
		}
		//DCFDumpInfo();
	} else {
		DCF_internalScanObj(pInfo, bKeepIndex);
	}
}


void DCF_RefreshEx(DCF_HANDLE DcfHandle)
{
	PDCF_INFO   pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return;
	}
	DCF_lock(pInfo);

	DCF_internalRefresh(pInfo, TRUE);

	DCF_unlock(pInfo);
}

BOOL DCF_MakeDirPathEx(DCF_HANDLE DcfHandle, UINT32  DirID, CHAR *Path)
{
	PDCF_INFO   pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return FALSE;
	}
	if (DirID < MIN_DCF_DIR_NUM || DirID > MAX_DCF_DIR_NUM) {
		DBG_ERR("DirID(%d)\r\n", DirID);
		return FALSE;
	}
	sprintf(Path, "%s%03d%s", pInfo->PathDCIM, (int)(DirID), pInfo->OurDCFDirName);
	DBG_IND("%s\r\n", (char *)Path);
	return TRUE;
}


BOOL DCF_MakeObjPathEx(DCF_HANDLE DcfHandle, UINT32  DirID, UINT32 FileID, UINT32 fileType, CHAR *filePath)
{
	char *pExtName;
	char *pFileFreeChars;
	PDCF_INFO   pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return FALSE;
	}
	if (DirID < MIN_DCF_DIR_NUM || DirID > MAX_DCF_DIR_NUM) {
		DBG_ERR("DirID(%d)\r\n", DirID);
		return FALSE;
	} else if (FileID < MIN_DCF_FILE_NUM || FileID > MAX_DCF_FILE_NUM) {
		DBG_ERR("FileID(%d)\r\n", FileID);
		return FALSE;
	}
	pExtName = DCFGetExtByType(fileType);
	if ((pExtName == NULL)) {
		DBG_ERR("Type 0x%x\r\n", fileType);
		filePath[0] = '\0';
		return FALSE;
	}

	pFileFreeChars = DCFGetFreeCharsByType(fileType);
	if (NULL == pFileFreeChars) {
		DBG_ERR("Type 0x%x\r\n", fileType);
		return FALSE;
	}

	// file free chars is emtpy , means the free chars is auto generated by folder
	if (pFileFreeChars[0] == 0) {
		sprintf(filePath, "%s%03d%s\\%03d_%04d.%s",
				pInfo->PathDCIM,
				(int)(DirID),
				pInfo->OurDCFDirName,
				(int)(DirID),
				(int)(FileID),
				pExtName);
	} else {
		sprintf(filePath, "%s%03d%s\\%s%04d.%s",
				pInfo->PathDCIM,
				(int)(DirID),
				pInfo->OurDCFDirName,
				pFileFreeChars,
				(int)(FileID),
				pExtName);
	}
	DBG_IND("%s\r\n", (char *)filePath);
	return TRUE;
}

BOOL DCF_AddDBfileEx(DCF_HANDLE DcfHandle, CHAR *filePath)
{
	UINT32  DirNum, FileNum, fileType, tmpFileType, i;
	BOOL    isAddOneFile = FALSE, isAddValidFmt = TRUE, ret = TRUE, isChangDir = FALSE, isUpdateDirFileNum = TRUE;
	CHAR    tmpFileName[12] = {0}; // XXXX0001JPG
	PDCF_DB_INFO pDcfDbInfo;
	PDCF_INFO    pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return FALSE;
	}
	pDcfDbInfo = &pInfo->DCFDbInfo;
	// "A:\\DCIM\\100XXXXX\\XXXX0001.JPG
	DCF_lock(pInfo);
	DBG_IND("%s\r\n", filePath);

	//check the parent folder is DCIM
	if (memcmp(pInfo->PathDCIM, filePath, DCF_PATH_DCIM_LENGTH) != 0) {
		DBG_ERR("path %s error\r\n", filePath);
		ret =  FALSE;
		goto L_DCF_AddDBfileErr;
	}

	//check the DirID and FileID
	DirNum = DCF_IsValidDir(&filePath[DCF_PATH_DCIM_LENGTH]);
	// truncat '.' for   DCF_IsValidFile input filename   XXXX0001.JPG --> XXXX0001JPG
	memcpy(&tmpFileName[0], &filePath[DCF_PATH_DIR_LENGTH], 8);
	memcpy(&tmpFileName[8], &filePath[DCF_PATH_DIR_LENGTH + 9], 3);
	FileNum = DCF_IsValidFile(tmpFileName, tmpFileName + 8, &fileType);
	//#NT#2016/06/15#Nestor Yang -begin
	//#NT# fix for coverity
	//if (DirNum == 0 || FileNum == 0)
	if (DirNum < MIN_DCF_DIR_NUM || FileNum == 0)
		//#NT#2016/06/15#Nestor Yang -end
	{
		DBG_ERR("%s,DirNum=%d,FileNum=%d\r\n", filePath, DirNum, FileNum);
		ret =  FALSE;
		goto L_DCF_AddDBfileErr;
	}

	//check change dir
	if (pDcfDbInfo->DirectoryID != DirNum) {
		// if the folder not exist , need to copy folder name from input filePath
		if (pInfo->DCFDirName[DirNum - MIN_DCF_DIR_NUM][0] == 0) {
			memcpy(pInfo->DCFDirName[DirNum - MIN_DCF_DIR_NUM], &filePath[DCF_PATH_DIR_FREECHAR_POS], DCF_DIR_NAME_LEN);
		} else {
			if (memcmp(pInfo->DCFDirName[DirNum - MIN_DCF_DIR_NUM], &filePath[DCF_PATH_DIR_FREECHAR_POS], DCF_DIR_NAME_LEN) != 0) {
				DBG_ERR("input folder name not match %s\r\n", filePath);
				ret =  FALSE;
				goto L_DCF_AddDBfileErr;
			}
		}
		// here we will get MaxfileID and file number in dir
		DCFChangeWorkingDir(pInfo, DirNum, pInfo->DCFFileTb, pDcfDbInfo);
		isChangDir = TRUE;
	}
	if (pDcfDbInfo->DirectoryID == DirNum) {
#if DCF_CHK_VALID_FMT
		tmpFileType = GetDCFNumCache(pInfo, FileNum, DCF_IS_FILE) & (~DCF_FMT_VALID_BIT);
#else
		tmpFileType = GetDCFNumCache(pInfo, FileNum, DCF_IS_FILE);
#endif
		if (tmpFileType == DCF_FILE_TYPE_NOFILE) {
			isAddOneFile = TRUE;
		} else if (tmpFileType & fileType) {
			if (isChangDir) {
				for (i = 0; i < DCF_FILE_TYPE_NUM; i++) {
					if (tmpFileType == pInfo->DcfFiletype[i].fileType) {
						isAddOneFile = TRUE;
						// because the folder is rescan and the DirFileNum is already correct
						isUpdateDirFileNum = FALSE;
						//DBG_IND("%s,not update Dir file Num\r\n",filePath);
						break;
					}
				}
			} else {
				// file is already exist and not change folder
				DBG_IND("%s,duplicate OriFileType=0x%x,NewFileType=0x%x\r\n", filePath, tmpFileType, fileType);
				ret =  FALSE;
				goto L_DCF_AddDBfileErr;
			}
		}
		SetDCFNumCache(pInfo, FileNum, fileType,  DCF_IS_FILE);
		SetDCFNumCache(pInfo, DirNum, DCF_IS_EXIST, DCF_IS_DIR);
		if (isAddOneFile) {
			if (DirNum > pDcfDbInfo->MaxDirID) {
				pDcfDbInfo->MaxDirID = DirNum;
				pDcfDbInfo->MaxSystemFileID = pDcfDbInfo->MaxFileID;
			}
			tmpFileType = GetDCFNumCache(pInfo, FileNum, DCF_IS_FILE);
			if (tmpFileType == DCF_FILE_TYPE_NOFILE) {
				isAddValidFmt = FALSE;
			}
#if 0
			if (isAddValidFmt) {
				if (isUpdateDirFileNum) {
					pDcfDbInfo->DirFileNum++;
					DCFSetIDNumInDir(pInfo, DirNum, pDcfDbInfo->DirFileNum);
				}
				pDcfDbInfo->TolFileNum++;
				DCFUpdateDirInfo(pInfo, DirNum, FileNum, tmpFileType);
				DCFAddOneTypeNum(pInfo, pDcfDbInfo->FileFormat);
				memcpy(pInfo->DCFFileName[FileNum - MIN_DCF_FILE_NUM], tmpFileName, DCF_FILE_NAME_LEN);
				if (pDcfDbInfo->MaxDirID == pDcfDbInfo->DirectoryID && pDcfDbInfo->MaxSystemFileID < pDcfDbInfo->MaxFileID) {
					pDcfDbInfo->MaxSystemFileID = pDcfDbInfo->MaxFileID;
				}
				if (pDcfDbInfo->MaxDirID == MAX_DCF_DIR_NUM && pDcfDbInfo->MaxSystemFileID == MAX_DCF_FILE_NUM) {
					pDcfDbInfo->Is9999 = TRUE;
					DCF_internalSetNextID(pInfo, ERR_DCF_DIR_NUM, ERR_DCF_FILE_NUM);
				} else if (pDcfDbInfo->MaxSystemFileID == MAX_DCF_FILE_NUM) {
					DCF_internalSetNextID(pInfo, pDcfDbInfo->MaxDirID + 1, MIN_DCF_FILE_NUM);
				} else {
					DCF_internalSetNextID(pInfo, pDcfDbInfo->MaxDirID, pDcfDbInfo->MaxSystemFileID + 1);
				}
			} else {
				DCF_internalRefresh(FALSE);
			}
#else
			if (isAddValidFmt) {
				if (isUpdateDirFileNum) {
					pDcfDbInfo->DirFileNum++;
					DCFSetIDNumInDir(pInfo, DirNum, pDcfDbInfo->DirFileNum);
				}
				pDcfDbInfo->TolFileNum++;
				DCFUpdateDirInfo(pInfo, DirNum, FileNum, tmpFileType);
				DCFAddOneTypeNum(pInfo, fileType);
				memcpy(pInfo->DCFFileName[FileNum - MIN_DCF_FILE_NUM], tmpFileName, DCF_FILE_NAME_LEN);
			} else {
				DCFAddOneTypeNum(pInfo, fileType);
				memcpy(pInfo->DCFFileName[FileNum - MIN_DCF_FILE_NUM], tmpFileName, DCF_FILE_NAME_LEN);
			}
			// set system file ID
			if (pDcfDbInfo->MaxDirID == pDcfDbInfo->DirectoryID && pDcfDbInfo->MaxSystemFileID < pDcfDbInfo->MaxFileID) {
				pDcfDbInfo->MaxSystemFileID = pDcfDbInfo->MaxFileID;
			} else if (pDcfDbInfo->MaxDirID == pDcfDbInfo->DirectoryID && pDcfDbInfo->MaxSystemFileID < FileNum) {
				pDcfDbInfo->MaxSystemFileID = FileNum;
			}

			// set next id
			if (pDcfDbInfo->MaxDirID == MAX_DCF_DIR_NUM && pDcfDbInfo->MaxSystemFileID == MAX_DCF_FILE_NUM) {
				pDcfDbInfo->Is9999 = TRUE;
				DCF_internalSetNextID(pInfo, ERR_DCF_DIR_NUM, ERR_DCF_FILE_NUM);
			} else if (pDcfDbInfo->MaxSystemFileID == MAX_DCF_FILE_NUM) {
				DCF_internalSetNextID(pInfo, pDcfDbInfo->MaxDirID + 1, MIN_DCF_FILE_NUM);
			} else {
				DCF_internalSetNextID(pInfo, pDcfDbInfo->MaxDirID, pDcfDbInfo->MaxSystemFileID + 1);
			}
#endif

		} else {
			pDcfDbInfo->FileFormat = GetDCFNumCache(pInfo, FileNum, DCF_IS_FILE);
#if DCF_CHK_VALID_FMT
			pDcfDbInfo->IdFormat = (pDcfDbInfo->FileFormat & (~DCF_FMT_VALID_BIT));
#else
			pDcfDbInfo->IdFormat = pDcfDbInfo->FileFormat;
#endif

		}
	}
	//DCFDumpInfo();

L_DCF_AddDBfileErr:
	DCF_unlock(pInfo);
	return ret;
}

BOOL DCF_GetDirPathEx(DCF_HANDLE DcfHandle, UINT32 DirID, CHAR *Path)
{
	PDCF_DB_INFO pDcfDbInfo;
	PDCF_INFO    pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return FALSE;
	}
	pDcfDbInfo = &pInfo->DCFDbInfo;
	DBG_IND("DirID(%d)\r\n", DirID);
	if (DirID < MIN_DCF_DIR_NUM || DirID > pDcfDbInfo->MaxDirID) {
		DBG_ERR("DirID(%d)\r\n", DirID);
		return FALSE;
	}
	DCF_lock(pInfo);
	GetDCFFolderName(pInfo, Path, DirID);
	// remove redundant '\'
	Path[DCF_PATH_DIR_LENGTH - 1] = '\0';
	DCF_unlock(pInfo);
	return TRUE;
}

void DCF_RegisterCBEx(DCF_HANDLE DcfHandle, DCF_CBMSG_FP CB)
{
	PDCF_INFO   pInfo;

	if ((pInfo = DCF_GetInfoByHandle(DcfHandle)) == NULL) {
		return;
	}
	pInfo->DcfCbFP = CB;
}

#if DCF_SUPPORT_RESERV_DATA_STORE
void DCF_ParseReservData(BOOL isValid, CHAR *pData)
{
	if (isValid) {
		memcpy(&pInfo->DcfResvData, pData, sizeof(pInfo->DcfResvData));
	}
}

void DCF_StoreReservData(void)
{
	extern int fs_StoreResvData(UINT32 DataLen, CHAR * pData);
	UINT32 CurIndex, DirID, FileID, FileType;
	PDCF_DB_INFO pDcfDbInfo = &pInfo->DCFDbInfo;

	memset(&pInfo->DcfResvData, 0, sizeof(pInfo->DcfResvData));
	if (pDcfDbInfo->TolFileNum) {
		pInfo->DcfResvData.TotalDCFObjectCount = pDcfDbInfo->TolFileNum;
		DCF_SeekIndex(0, DCF_SEEK_END);
		CurIndex = DCF_GetCurIndex();
		DCF_GetObjInfo(CurIndex, &DirID, &FileID, &FileType);
		DCF_GetObjPath(CurIndex, FileType, pInfo->DcfResvData.LastObjPath);
	}
	fs_StoreResvData(sizeof(pInfo->DcfResvData), (CHAR *)&pInfo->DcfResvData);
}
#endif


#if 0
//FST_CMD_FILTER_DELETE,(UINT32)pDirEntry,(UINT32)&bDelete,NULL;

static void DCFFilesys_DelCB(UINT32 MessageID, UINT32 Param1, UINT32 Param2, UINT32 Param3)
{
	BOOL    *bDelete;
	INT32    fileNum;
	UINT32   fileType;
	UINT32   filterType = (pInfo->ValidFileFormat | pInfo->DepFileFormat);
	FIND_DATA *pFindData;
	UINT8     attrib;

	pFindData = (FIND_DATA *)Param1;
	attrib    = pFindData->attrib;
	bDelete = (BOOL *)Param2;

	fileNum = DCF_IsValidFile(pFindData->FATMainName, pFindData->FATExtName, &fileType);
	if (!M_IsReadOnly(attrib) && fileNum && (filterType & fileType)) {
		*bDelete = TRUE;
	} else {
		*bDelete = FALSE;
		gbNoFile = FALSE;
	}
}

static void DCFFilesys_LockCB(UINT32 MessageID, UINT32 Param1, UINT32 Param2, UINT32 Param3)
{
	char    *pFileName;
	BOOL    *bLock;
	INT32    fileNum;
	UINT32   fileType;
	UINT32   filterType = (pInfo->ValidFileFormat | pInfo->DepFileFormat);

	pFileName = (char *)Param1;
	bLock = (BOOL *)Param2;

	fileNum = DCF_IsValidFile(pFileName, pFileName + 8, &fileType);
	if (fileNum && (filterType & fileType)) {
		*bLock = TRUE;
	} else {
		*bLock = FALSE;
	}
}

BOOL DCF_LockAllinDirNum(UINT32 uwDirNum, BOOL bLock)
{
	char path[DCF_FULL_FILE_PATH_LEN];

	DBG_IND("DirNum(%d), bLock=%d\r\n", uwDirNum, bLock);
	if (uwDirNum < MIN_DCF_DIR_NUM || uwDirNum > pDcfDbInfo->MaxDirID) {
		DBG_ERR("DirNum(%d)\r\n", uwDirNum);
		return FALSE;
	}
	DCF_lock(pInfo);
	GetDCFFolderName(pInfo, path, uwDirNum);
	FileSys_LockDirFiles(path, bLock, DCFFilesys_LockCB);
	//DCFDumpInfo();
	DCF_unlock(pInfo);
	return TRUE;
}
BOOL DCF_DelAllinDirNum(UINT32 uwDirNum)
{
	char path[DCF_FULL_FILE_PATH_LEN];

	DBG_IND("DirNum(%d)\r\n", uwDirNum);
	if (uwDirNum < MIN_DCF_DIR_NUM || uwDirNum > pDcfDbInfo->MaxDirID) {
		DBG_ERR("DirNum(%d)\r\n", uwDirNum);
		return FALSE;
	}
	DCF_lock(pInfo);
	GetDCFFolderName(pInfo, path, uwDirNum);
	gbNoFile = TRUE;
	FileSys_DelDirFiles(path, DCFFilesys_DelCB);
	if (gbNoFile) {
		FileSys_DeleteDir(path);
	}
	//DCFDumpInfo();
	DCF_unlock(pInfo);
	return TRUE;
}
#endif

