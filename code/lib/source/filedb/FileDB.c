/**
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    @file       FileDB.c
    @ingroup    FileDB

    @brief      File Database Library

    @note       Nothing.

    @version    V0.00.001
    @author     Lincy Lin
    @date       2007/02/05
*/

/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#if defined (__UITRON) || defined (__ECOS)
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "FileDB.h"
#include "Utility.h"
#include "ErrorNo.h"
#include "FileDBID.h"
#include "FileSysTsk.h"
#include "dma_protected.h"
#include "efuse_protected.h"
#include "NvtVerInfo.h"

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          FileDB
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#define __DBGFLT__          "[scan]" // scan files
//#define __DBGFLT__          "[search]" // search files
//#define __DBGFLT__          "[buff]" // search files
#include "DebugModule.h"
#else
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "FileDB.h"
#include "hd_common.h"
#include "kwrap/cpu.h"
#include "kwrap/error_no.h"
#include "kwrap/util.h"
#include "Utility/avl.h"

#define THIS_DBGLVL         NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          FileDB
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*"
#include "kwrap/debug.h"
unsigned int FileDB_debug_level = THIS_DBGLVL;

// INCLUDE_PROTECTED
#include "FileDBID.h"
#endif

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions & Types Declarations                             */
/*-----------------------------------------------------------------------------*/
typedef BOOL (*CHKLONGNAME_CB)(UINT16 *longName, UINT16 nameLen);

//#NT#2009/08/11#Lincy Lin -begin
//#Enhance File number and index from UINT16 to UINT32
//------------------------------------------------------------------------------------------------------------
#define FILEDB_INITED_VAR           MAKEFOURCC('F','D','B','T') ///< a key value 'F','D','B','T' for indicating FileDB initial.
#define FILEDB_HANDLE1_TAG          MAKEFOURCC('F','D','B','1')
#define FILEDB_HANDLE2_TAG          MAKEFOURCC('F','D','B','2')
#define FILEDB_HANDLE3_TAG          MAKEFOURCC('F','D','B','3')

#if defined(_BSP_NA51000_)
#if defined (__UITRON) || defined (__ECOS)
#define RANDOM_RANGE(n)             ((randomUINT32() % (n)))
#else
#define RANDOM_RANGE(n)             (rand() % (n))
#endif
#else
#if defined (__LINUX_USER__)
#define RANDOM_RANGE(n)             (rand() % (n))
#else
#include "trng.h"
#define RANDOM_RANGE(n)             ((randomUINT32() % (n)))
#endif
#endif

#define TEST_FUNC_DEEP               0

// define values
#define _SUPPORT_LONG_FILENAME_IN_FILEDB_      0
//#define FILEDB_MAX_LONG_FILENAME_LEN          30
#define FILEDB_NAME_MAX_LENG                  128
#define FILEDB_ROOT_PATH_LEN                   3


//#define FILEDB_NUM                             2
#define FILE_FILTER_MAX_LEN                   (4*FILEDB_SUPPORT_FILE_TYPE_NUM)

#define FILE_EXT_LEN                           3
#define FILEDB_MAX_FILE_NUM_DEFAULT            5000
#define FILEDB_RESERVE_FILE_ENTRY_NUM          900


#define QUICK_SORT_DITHER_STEP                 5
#define QUICK_SORT_SAMPLE_STEP                 (8*QUICK_SORT_DITHER_STEP)
#define INSERT_SORT_UNIT                       20
#define QUICK_SORT_RECURSIVE_FUNC_DEEP_MAX     60
#define SORT_METHOD_NONE                       0
#define SORT_METHOD_INSERT                     1
#define SORT_METHOD_QUICK                      2

#define SORT_METHOD_STRBASE                    10//16

// DCF releated define
#define FILEDB_MAX_DCF_FILENUM       9999
#define FILEDB_MAX_DIFF_DCF_FOLDER   900
#define DCF_FILE_ROOT_PATH_LEN       8
#define DCF_FILE_PATH_LEN            22
#define DCF_FOLDER_ID_INDEX          8
#define DCF_FOLDER_ID_LEN            3
#define DCF_FILE_ID_INDEX            21
#define DCF_FILE_ID_LEN              4
#define DCF_FILE_ID_OTHER_INDEX      17
#define DCF_FILE_ID_OTHER_LEN        4
#define DCF_DIR_NAME_LEN             5      ///<  DCF dir free chars length, ex: NVTIM
#define DCF_FILE_NAME_LEN            4      ///<  DCF file free chars length, ex: IMAG
#define DCF_FULL_FILE_PATH_LEN       30     ///<  DCF full file path legnth, ex: "A:\DCIM\100NVTIM\IMAG0001.JPG" & '\0'
#define MIN_DCF_DIR_NUM             (100)   ///<  Minimum DCF DIR ID
#define MAX_DCF_DIR_NUM             (999)   ///<  Maximum DCF DIR ID
#define MIN_DCF_FILE_NUM            (1)     ///<  Minimum DCF File ID
#define MAX_DCF_FILE_NUM            (9999)  ///<  Maximum DCF File ID

#define FILEDB_SHORT_NAME_LEN       12      ///<  IMAG0001.JPG


#define FILEDB_SORT_WEIGHT_MIN                0x1010
#define FILEDB_ROOT_SORT_WEIGHT               0x1020
#define FILEDB_DEFAULT_FOLDER_SORT_WEIGHT     0x1040
#define FILEDB_SORT_WEIGHT_MAX                0x1080



#define FILEDB_STATIC_MEM_MAX_FILE_NUM        10

#define SORTSN_MAX_DELIM_CHAR_NUM             7
#define SORTSN_MAX_CHAR_NUM_OF_SN             15

#define SPECIFIC_FILE_INDEX_INIT     0xFFFFFFFF
//static UINT32   gSortMethod;

static const CHAR    gPathDCIM[] = "A:\\DCIM\\";

#define FileDB_ExchangeIndex(pFileDBInfo,i,j) { \
		UINT32 u32temp; \
		u32temp = *(pFileDBInfo->pSortIndex+(i)); \
		(*(pFileDBInfo->pSortIndex+(i))) = (*(pFileDBInfo->pSortIndex+(j))); \
		(*(pFileDBInfo->pSortIndex+(j))) = u32temp; \
	}

#define FDB_GET_DAY_FROM_DATE(x)     (x & 0x1F)
#define FDB_GET_MONTH_FROM_DATE(x)   ((x >> 5) & 0x0F)
#define FDB_GET_YEAR_FROM_DATE(x)    (((x >> 9) & 0x7F)+1980)


typedef struct _FILEDB_FILE_INT_ATTR {
	UINT16               creTime;                              ///< File created time. bit0~4: seconds/2, bit5~10: minutes, bit11~15: hours.
	UINT16               creDate;                              ///< File created date. bit0~4: day(1-31), bit5~8: month(1-12), bit9~15: year(0-127) add 1980 to convert
	UINT16               lastWriteTime;                        ///< The latest time of write file.
	UINT16               lastWriteDate;                        ///< The latest date of write file.
	UINT64               fileSize64;                           ///< File size in bytes. (UINT64)
	UINT16               fileType;                             ///< File type, the value could be FILEDB_FMT_JPG, FILEDB_FMT_MP3 or FILEDB_FMT_AVI ...
	UINT8                attrib;                               ///< File attribute.
	UINT8                userData;                             ///< The user data that can keep in each file entry
	CHAR                *filename;                             ///< File Name
	CHAR                *filePath;                             ///< File full path
	//  reserved data for internal use
	UINT32               sortWeight;                           ///< Sort weighting value
} FILEDB_FILE_INT_ATTR, *PFILEDB_FILE_INT_ATTR;

STATIC_ASSERT(sizeof(FILEDB_FILE_INT_ATTR) == sizeof(FILEDB_FILE_ATTR));

typedef struct {
	UINT32               HeadTag;                              ///<  Head Tag to check if memory is overwrit
	BOOL                 bIsRecursive;                         ///<  If recursive search this path
	BOOL                 bIsSkipDirForNonRecursive;            ///<  If skip folder for non-recursive case
	BOOL                 bIsSkipHidden;                        ///<  If skip hidden items
	BOOL                 bIsCyclic;                            ///<  If cyclic search
	BOOL                 bIsSortLargeFirst;
	char                 rootPath[FILEDB_NAME_MAX_LENG + 1];   ///<  Root path
	char                 defaultfolder[FILEDB_NAME_MAX_LENG + 1]; ///<  default folder
	char                 filterfolder[FILEDB_NAME_MAX_LENG + 1]; ///<  filter folder
	BOOL                 bIsDCFFileOnly;
	BOOL                 bIsFilterOutSameDCFNumFolder;
	char                 OurDCFDirName[5];
	char                 OurDCFFileName[4];
	UINT8                bIsGetFile;
	UINT8                bIsFilterOutSameDCFNumFile;
	BOOL                 bIsChkHasFile;                        ///<  If just want to check if there any matched file inside file system
	BOOL                 bIsFindMatchFile;                     ///<  If already find the matched file, when this flag sets, FileDB will return
	FILEDB_SORT_TYPE     sortType;                             ///<  The sort type of this database
	UINT32               u32TotalFilesNum;                     ///<  Total items count in this directory
	UINT32               u32CurrentFileIndex;                  ///<  Current file Index
	UINT32               u32CurDirStartFileIndex;              ///<  The start file index of current directory
	UINT32               u32MaxFileNum;
	UINT32               u32MaxFilePathLen;
	UINT32               u32SpecificFileIndex;                 ///<  Specific file Index
	UINT8               *pDCFDirTbl;                           ///<  The DCF dir table
	UINT32               DCFDirTblSize;                        ///<  The DCF dir table size
	UINT8               *pDCFFileTbl;                          ///<  The DCF file table
	UINT32               DCFFileTblSize;                       ///<  The DCF file table size
	UINT32              *pSortIndex;                           ///<  The sort index starting address
	UINT32               SortIndexBufSize;                     ///<  The sort index total buffer size
	UINT32               pFileInfoAddr;                        ///<  The file info starting address
	UINT32               filesNum[FILEDB_SUPPORT_FILE_TYPE_NUM];  ///<  The files count of one file type
	UINT32              *GroupIndexMappTbl[FILEDB_SUPPORT_FILE_TYPE_NUM];          ///<  The Index mapping table of each grouped file type
	UINT32               u32GroupCurIdx[FILEDB_SUPPORT_FILE_TYPE_NUM];             ///<  The current file Index of each grouped file type
	UINT32               groupIndexMappTblSize[FILEDB_SUPPORT_FILE_TYPE_NUM];      ///<  The mapping table size
	UINT32               groupFileType[FILEDB_SUPPORT_FILE_TYPE_NUM];              ///<  The grouped file type
	UINT32               fileFilter;
	UINT32               u32MemAddr;                           ///<  The Dynamic Memory address
	UINT32               u32MemSize;                           ///<  The Dynamic Memory size
	UINT32               u32MaxStoreItemsNum;                  ///<  The Max Number of Files Info can be sotred in the Dynamic Memory
	UINT32               u32FileItemSize;                      ///<  The size of each file item
	UINT32               u32IndexItemSize;                     ///<  The index size of each item
#if (_SUPPORT_LONG_FILENAME_IN_FILEDB_)
	CHKLONGNAME_CB       fpChkLongname;
#endif
	CHKABORT_CB          fpChkAbort;
	UINT32               sortWeight;                           ///<  set sort weight of current folder
	UINT32               folderCount;
	BOOL                 bIsSupportLongName;
	UINT32               u32RootDepth;                         ///<  the root path deepth
	UINT32               u32RootPathLen;                       ///<  the root path length
	UINT32               u32CurrScanDirDepth;                  ///<  the current scan dir deepth
	UINT32               u32CurrScanDirLength;                 ///<  the current scan dir path length
	CHAR                 currentDir[FILEDB_NAME_MAX_LENG + 1]; ///<  the current scan dir
	UINT32               recursiveFuncCurDeep;                 ///<  recursive function call current deep
	UINT32               recursiveFuncMaxDeep;                 ///<  recursive function call max deep
	CHAR                 SortSN_Delim[SORTSN_MAX_DELIM_CHAR_NUM + 1]; ///<  The delimiter string, e.g. underline "_", "AA"
	UINT8                SortSN_DelimLen;                      ///<  The delimiter string length, calculated by SortSN_Delim
	INT32                SortSN_DelimCount;                    ///<  The delimiter count to find the serial number
	INT32                SortSN_CharNumOfSN;                   ///<  The character number of the serial number
	UINT32               EndTag;                               ///<  End Tag to check if memory is overwrit
} FILEDB_INFO, *PFILEDB_INFO;


typedef struct {
	UINT32               InitTag;
	PFILEDB_INFO         pInfo;
} FILEDB_HANDLE_INFO, *PFILEDB_HANDLE_INFO;


/* --------------------------Buffer Management -------------------------------------

                 pInfo       |--------------------------------------------------------
                             |
                             |  Size =  sizeof(FILEDB_INFO)
                 pDCFDirTbl  |--------------------------------------------------------
                             |
                             |  DCFDirTblSize = ALIGN4(FILEDB_MAX_DIFF_DCF_FOLDER)
                             |
                 pDCFFileTbl |--------------------------------------------------------
                             |
                             |  DCFFileTblSize = ALIGN4(FILEDB_MAX_DCF_FILENUM)
                             |
                 pSortIndex  |--------------------------------------------------------
                             |
                             |
                             |
                             |  SortIndexBufSize = u32MaxStoreItemsNum*u32IndexItemSize
                             |
                             |
               pFileInfoAddr |--------------------------------------------------------
                             |    file info 1 ,  each file item size = u32FileItemSize
                             |  ------------------------------------------------------
                             |    file info 2
                             |  ------------------------------------------------------
                             |   ...
                             |

 ------------------------------------------------------------------------------- */


// Global Variable
//-------------------------------------------------------------------------------------------------------------------------------
static FILEDB_HANDLE_INFO   gFileDBInfo[FILEDB_NUM] = {0};
static UINT32               gFileDBHandleTag[FILEDB_NUM] = {FILEDB_HANDLE1_TAG, FILEDB_HANDLE2_TAG, FILEDB_HANDLE3_TAG};
#if (_SUPPORT_LONG_FILENAME_IN_FILEDB_)
static UINT16             g_TempLongname[FILEDB_MAX_LONG_FILENAME_LEN];   ///< Long File Name
#endif

typedef struct {
	UINT32          fileType;
	char            filterStr[FILE_EXT_LEN + 1];
} FILEDB_FILE_TYPE;

// File Format Filter
static FILEDB_FILE_TYPE fileDbFiletype[FILEDB_SUPPORT_FILE_TYPE_NUM] = {
	{FILEDB_FMT_JPG, "JPG"},
	{FILEDB_FMT_MOV, "MOV"},
	{FILEDB_FMT_MP4, "MP4"},
	{FILEDB_FMT_MPG, "MPG"},
	{FILEDB_FMT_AVI, "AVI"},
	{FILEDB_FMT_RAW, "RAW"},
	{FILEDB_FMT_MP3, "MP3"},
	{FILEDB_FMT_ASF, "ASF"},
	{FILEDB_FMT_WAV, "WAV"},
	{FILEDB_FMT_BMP, "BMP"},
	{FILEDB_FMT_TS, "TS "},
	{FILEDB_FMT_AAC, "AAC"}
};

//#Add new sort type - sort by stroke number for FileDB
#if (_SUPPORT_LONG_FILENAME_IN_FILEDB_)
static const UINT16 AsciiCodeSortSeq[128] = {
	/*
	0 1 2 3 4 5 6 7 8 9 space ! " # $ % & ' ( ) * +  , - . / : ; < = > ? @ [ \ ] ^ _ ` { | } ~
	a A b B c C d D e E f F g G h H i I j J k K l L m M n N o O p P q Q r R s S t T u U v V w W x X y Y z Z
	*/
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x000B, 0x000C, 0x000D, 0x000E, 0x000F, 0x0010, 0x0011, 0x0012,
	0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001A,
	0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008,
	0x0009, 0x000A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F, 0x0020,
	0x0021, 0x002D, 0x002F, 0x0031, 0x0033, 0x0035, 0x0037, 0x0039,
	0x003B, 0x003D, 0x003F, 0x0041, 0x0043, 0x0045, 0x0047, 0x0049,
	0x004B, 0x004D, 0x004F, 0x0051, 0x0053, 0x0055, 0x0057, 0x0059,
	0x005B, 0x005D, 0x005F, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026,
	0x0027, 0x002C, 0x002E, 0x0030, 0x0032, 0x0034, 0x0036, 0x0038,
	0x003A, 0x003C, 0x003E, 0x0040, 0x0042, 0x0044, 0x0046, 0x0048,
	0x004A, 0x004C, 0x004E, 0x0050, 0x0052, 0x0054, 0x0056, 0x0058,
	0x005A, 0x005C, 0x005E, 0x0028, 0x0029, 0x002A, 0x002B, 0x0000
};
#endif
//---------------------FileDB MISC-------------------------

//---------------------FileDB Prototype Declaration  ---------------------------
static void   FileDB_FileSysDirCB(FIND_DATA *findDir, BOOL *bContinue, UINT16 *cLongname, UINT32 Param);
static void   FileDB_RemoveDir(FILEDB_HANDLE fileDbHandle);
static void   FileDB_GetFileFilterStr(UINT32 fileFilter, char *filterStr, UINT16 u16StrMaxLen);
static UINT32 FileDB_GetFileType(const char *ExtName);
static PFILEDB_INFO FileDB_GetInfoByHandle(FILEDB_HANDLE fileDbHandle);
//#NT#2015/02/25#Nestor Yang -begin
//#NT# Callback will get FileDB info from Param(FileDB Handle)
//static PFILEDB_INFO FileDB_GetInfoByPath(CHAR* path);
//#NT#2015/02/25#Nestor Yang -end
static PFILEDB_FILE_ATTR    FileDB_GetFileAttr(PFILEDB_INFO pFileDBInfo, UINT32 u32FileIndex);
static BOOL   FileDB_MatchNameUpperWithLen(const char *name1, const char *name2, UINT32 len);
static BOOL   FileDB_MatchNameUpper(const char *name1, const char *name2);
static BOOL   FileDB_CompareName(const char *name1, const char *name2);
static void   FileDB_AddFilesNum(PFILEDB_INFO pFileDBInfo, UINT32 fileType);
static UINT32 FileDB_LOG2(UINT32 i);
static UINT32 FileDB_CalcSortWeight(PFILEDB_INFO pFileDBInfo, CHAR *path);


static int     FileDB_IsOurDCFFolderName(PFILEDB_INFO pFileDBInfo, const char *pFolderName);
static int     FileDB_IsOurDCFFileName(PFILEDB_INFO pFileDBInfo, const char *pFileName);
static void    FileDB_FilterOutSameDCFnumFile(PFILEDB_INFO   pFileDBInfo);

static UINT32 FileDB_GetCurDirDeepth(const char *filePath, UINT32 filePathLen);
static void FileDB_FilterOutSameDCFnumFolder(PFILEDB_INFO   pFileDBInfo);
static void FileDB_internalSortBy(FILEDB_HANDLE fileDbHandle, FILEDB_SORT_TYPE sortType, BOOL bIsSortLargeFirst);
static void FileDB_QuickSortByCreDate(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);
static void FileDB_QuickSortByModDate(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);
static void FileDB_QuickSortBySize(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);
static void FileDB_QuickSortByFileType(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);
static void FileDB_QuickSortByName(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);
static void FileDB_QuickSortByDCFName(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);
static void FileDB_QuickSortByPath(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);
static void FileDB_QuickSortBySN(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);
static void FileDB_QuickSortByTypeCretimeSize(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right); //sort by the sequence (1) File type, (2) File Cretate time, (3) file size
static void FileDB_QuickSortByCretimeName(PFILEDB_INFO pFileDBInfo, INT32 left, INT32 right); //sort by the sequence (1) File create time, (2) File name
static UINT32 FileDB_ChooseSortMethod(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);
static void FileDB_InsertSort(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);
static void FileDB_InsertSortByCreDate(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);
static void FileDB_InsertSortByModDate(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);
static void FileDB_InsertSortBySize(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);
static void FileDB_InsertSortByFileType(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);
static void FileDB_InsertSortByName(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);
static void FileDB_InsertSortByDCFName(PFILEDB_INFO  pFileDBInfo,  INT32 left, INT32 right);
static void FileDB_InsertSortByPath(PFILEDB_INFO   pFileDBInfo,  INT32 left, INT32 right);
static void FileDB_InsertSortByTypeCretimeSize(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);  //sort by the sequence (1) File type, (2) File Cretate time, (3) file size
static void FileDB_InsertSortByCretimeName(PFILEDB_INFO  pFileDBInfo,  INT32 left, INT32 right);  //sort by the sequence (1) File create time, (2) File name
static void FileDB_InsertSortBySN(PFILEDB_INFO pFileDBInfo, INT32 left, INT32 right);
//static void FileDB_DitherIndex(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right);
#if (_SUPPORT_LONG_FILENAME_IN_FILEDB_)
static void FileDB_QuickSortByStrokeNum(INT32 left, INT32 right);
static BOOL FileDB_ChkIfChiWordInside(char *filename);
#endif
static UINT32 FileDB_GetMappTblIdxByFileType(UINT32 fileType);

static INT32 fdb_GetSNByName(PFILEDB_INFO pFileDBInfo, char *fileName);

PFILEDB_FILE_ATTR FileDB_SpecificFile(FILEDB_HANDLE fileDbHandle, INT32 i32FileIndex);

void FileDB_ClrSpecificFileIndex(FILEDB_HANDLE fileDbHandle);
void FileDB_SetSpecificFileIndex(FILEDB_HANDLE fileDbHandle, UINT32 u32SpecFileIndex);
void FileDB_SetFileMappTblByFileType(FILEDB_HANDLE fileDbHandle, UINT32 fileType, UINT32 *TblAddr, UINT32 TblSize);
PFILEDB_FILE_ATTR FileDB_NextFileByType(FILEDB_HANDLE fileDbHandle, UINT32 fileType);
PFILEDB_FILE_ATTR FileDB_PrevFileByType(FILEDB_HANDLE fileDbHandle, UINT32 fileType);
PFILEDB_FILE_ATTR FileDB_CurrFileByType(FILEDB_HANDLE fileDbHandle, UINT32 fileType);
PFILEDB_FILE_ATTR FileDB_SearhFileByType(FILEDB_HANDLE fileDbHandle, INT32 i32FileIndex, UINT32 fileType);
PFILEDB_FILE_ATTR FileDB_SearhFileByType2(FILEDB_HANDLE fileDbHandle, INT32 i32FileIndex, UINT32 fileType);
PFILEDB_FILE_ATTR FileDB_SpecificFileByType(FILEDB_HANDLE fileDbHandle, INT32 i32FileIndex, UINT32 fileType);
UINT32 FileDB_GetCurrFileIndexByType(FILEDB_HANDLE fileDbHandle, UINT32 fileType);
BOOL FileDB_DeleteFileByType(FILEDB_HANDLE fileDbHandle, UINT32 u32FileIndex, UINT32 fileType);

static void FileDB_lockComm(void)
{
#if defined (__UITRON) || defined (__ECOS)
	wai_sem(SEMID_FILEDB_COMM);
#else
	vos_sem_wait(SEMID_FILEDB_COMM);
#endif
}

static void FileDB_unlockComm(void)
{
#if defined (__UITRON) || defined (__ECOS)
	sig_sem(SEMID_FILEDB_COMM);
#else
	vos_sem_sig(SEMID_FILEDB_COMM);
#endif
}


static void FileDB_lock(FILEDB_HANDLE fileDbHandle)
{
	if (fileDbHandle < 0 ||  fileDbHandle >= FILEDB_NUM) {
		return;
	}
#if defined (__UITRON) || defined (__ECOS)
	wai_sem(SEMID_FILEDB[fileDbHandle]);
#else
	vos_sem_wait(SEMID_FILEDB[fileDbHandle]);
#endif
}

static void FileDB_unlock(FILEDB_HANDLE fileDbHandle)
{
	if (fileDbHandle < 0 ||  fileDbHandle >= FILEDB_NUM) {
		return;
	}
#if defined (__UITRON) || defined (__ECOS)
	sig_sem(SEMID_FILEDB[fileDbHandle]);
#else
	vos_sem_sig(SEMID_FILEDB[fileDbHandle]);
#endif
}

static INT32 FileDB_IsValidDCFFile(char *pFileName, UINT32 *pType)
{
	char tmpStr[DCF_FILE_NAME_LEN + 1];
	int FileId = 0, i, j;
	// check whether is jpg extension name
	memcpy(tmpStr, pFileName + 4, DCF_FILE_NAME_LEN);
	tmpStr[DCF_FILE_NAME_LEN] = '\0';

	DBG_IND("[name]%11s\r\n", pFileName);
	//*pType = DCF_FILE_TYPE_NOFILE;
	//check 4 digital number
	for (j = 0 ; j < DCF_FILE_NAME_LEN; j++) {
		if (!isdigit(tmpStr[j])) {
			return 0;
		} else {
			FileId += (tmpStr[j] - 0x30);
		}
		if (j < DCF_FILE_NAME_LEN - 1) {
			FileId *= 10;
		}
	}
	//sscanf(tmpStr, "%d", &FileId);
	{
		if ((MIN_DCF_FILE_NUM <= FileId) && (FileId <= MAX_DCF_FILE_NUM)) {
			for (i = 0; i < DCF_FILE_NAME_LEN; i++) {
				if ((!isalpha(pFileName[i])) && (pFileName[i] != '_') && (!isdigit(pFileName[i]))) {
					return 0;
				}
			}
			//*pType = DCFGetTypeByExt((pFileName+8));
			//if (*pType == DCF_FILE_TYPE_NOFILE)
			//    return 0;
			//else
			return FileId;
		}
	}

	return 0;
}


/*========================================================================
    routine name :
    function     :
    parameter    :
    return value : return directory ID,  return 0 for invalid folder name
========================================================================*/
static INT32 FileDB_IsValidDCFDir(char *pDirName)
{
	char tmpStr[4];
	int FolderId;
	int i;

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

static FILEDB_HANDLE FileDB_GetFreeHandle(void)
{
	UINT32 i;
	for (i = 0; i < FILEDB_NUM; i++) {
		if (!(gFileDBInfo[i].InitTag == FILEDB_INITED_VAR)) {
			return i;
		}
	}
	return FILEDB_CREATE_ERROR;
}

static INT32 FileDB_Scan(FILEDB_HANDLE fileDbHandle)
{
	UINT32             j, curDeepth;
	PFILEDB_FILE_ATTR  pFileDbFileAttr;
	ER                 Error_check;
	PFILEDB_INFO       pFileDBInfo;


	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return E_SYS;
	}

	strncpy(pFileDBInfo->currentDir, pFileDBInfo->rootPath, FILEDB_NAME_MAX_LENG);
	pFileDBInfo->u32RootPathLen = strlen(pFileDBInfo->rootPath);
	curDeepth = FileDB_GetCurDirDeepth(pFileDBInfo->rootPath, strlen(pFileDBInfo->rootPath));
	pFileDBInfo->u32RootDepth = curDeepth;
	pFileDBInfo->u32CurrScanDirDepth = curDeepth;
	pFileDBInfo->u32CurrScanDirLength = strlen(pFileDBInfo->currentDir);
	if (curDeepth > KFS_MAX_DIRECTORY_DEEP + 1) {
		DBG_ERR("Directory Deepth over limit\r\n");
		pFileDBInfo->u32TotalFilesNum = 0;
		return E_OK;
	}
	//Fit our DCF rule
	if (pFileDBInfo->bIsDCFFileOnly == TRUE) {
		pFileDBInfo->bIsGetFile = FALSE;

	}
	// calculate sort weight
	pFileDBInfo->sortWeight = FileDB_CalcSortWeight(pFileDBInfo, pFileDBInfo->rootPath);
	DBG_IND("[scan]Scan Dir (%s)\r\n", pFileDBInfo->rootPath);

	//FileDB_lockComm();
	//gFileDbHandle = fileDbHandle;
	Error_check  = FileSys_ScanDir(pFileDBInfo->rootPath, FileDB_FileSysDirCB, pFileDBInfo->bIsSupportLongName, fileDbHandle);
	//FileDB_unlockComm();
	if (Error_check != E_OK) {
		DBG_ERR("Filesys Error");
		goto L_Scan_Error;
	}
	//For check if no files only
	else if (pFileDBInfo->bIsChkHasFile && pFileDBInfo->bIsFindMatchFile) {
		//#NT#2015/01/16#Nestor Yang -begin
		//#NT# Since bIsChkHasFile is set, the scan is skipped when one file is found
		//pFileDBInfo->u32TotalFilesNum = pFileDBInfo->u32CurrentFileIndex;
		pFileDBInfo->u32TotalFilesNum = 1;
		//#NT#2015/01/16#Nestor Yang -end
		goto L_Scan_FindFile;
	} else if (pFileDBInfo->fpChkAbort && pFileDBInfo->fpChkAbort()) {
		DBG_ERR("Abort !!\r\n");
		goto L_Scan_Error;
	}
	pFileDBInfo->u32TotalFilesNum = pFileDBInfo->u32CurrentFileIndex;
	// Recursive get file in directory
	if (pFileDBInfo->bIsRecursive == TRUE) {
		//#NT#2015/04/13#Nestor Yang -begin
		//#NT# Skip if FileDB Index is full
		UINT32 RecurMaxIndex = pFileDBInfo->folderCount + pFileDBInfo->u32MaxFileNum;

		if (pFileDBInfo->bIsDCFFileOnly && pFileDBInfo->bIsFilterOutSameDCFNumFolder) {
			FileDB_FilterOutSameDCFnumFolder(pFileDBInfo);
		}
		//for (j=0;j<pFileDBInfo->u32TotalFilesNum;j++)
		for (j = 0; j < pFileDBInfo->u32TotalFilesNum && pFileDBInfo->u32CurrentFileIndex < RecurMaxIndex; j++)
			//#NT#2015/04/13#Nestor Yang -end
		{
			pFileDbFileAttr = FileDB_GetFileAttr(pFileDBInfo, j);
			if (M_IsDirectory(pFileDbFileAttr->attrib)) {
				if ((pFileDBInfo->bIsDCFFileOnly == FALSE) || (pFileDBInfo->bIsDCFFileOnly == TRUE && FileDB_IsValidDCFDir(pFileDbFileAttr->filename))) {
					memset(pFileDBInfo->currentDir, 0x00, FILEDB_NAME_MAX_LENG);
					strncpy(pFileDBInfo->currentDir, (char *)pFileDbFileAttr->filePath, FILEDB_NAME_MAX_LENG - 1);
					strncat(pFileDBInfo->currentDir, "\\", (FILEDB_NAME_MAX_LENG - 1) - strlen(pFileDBInfo->currentDir));
					pFileDBInfo->u32CurrScanDirLength = strlen(pFileDBInfo->currentDir);
					curDeepth = FileDB_GetCurDirDeepth(pFileDBInfo->currentDir, pFileDBInfo->u32CurrScanDirLength);
					pFileDBInfo->u32CurrScanDirDepth = curDeepth;
					if (curDeepth > KFS_MAX_DIRECTORY_DEEP + 1) {
						DBG_ERR("Directory Deepth over limit\r\n");
						continue;
					}
					//Fit our DCF rule
					else if (pFileDBInfo->bIsDCFFileOnly == TRUE && curDeepth > 3) {
						DBG_ERR("Directory Deepth over DCF limit\r\n");
						continue;
					}
					if (pFileDBInfo->bIsDCFFileOnly == TRUE) {
						pFileDBInfo->bIsGetFile = TRUE;
						pFileDBInfo->u32CurDirStartFileIndex = pFileDBInfo->u32CurrentFileIndex;
					}
					// calculate sort weight
					pFileDBInfo->sortWeight = FileDB_CalcSortWeight(pFileDBInfo, pFileDBInfo->currentDir);
					DBG_IND("[scan]Recursive Search (%s), curDeepth=%d\r\n", pFileDBInfo->currentDir, curDeepth);
					//FileDB_lockComm();
					//gFileDbHandle = fileDbHandle;
					Error_check  = FileSys_ScanDir(pFileDBInfo->currentDir, FileDB_FileSysDirCB, pFileDBInfo->bIsSupportLongName, fileDbHandle);
					//FileDB_unlockComm();
					if (Error_check != E_OK) {
						DBG_ERR("Filesys Error");
						goto L_Scan_Error;
					}
					//For check if no files only
					else if (pFileDBInfo->bIsChkHasFile && pFileDBInfo->bIsFindMatchFile) {
						//#NT#2015/01/16#Nestor Yang -begin
						//#NT# Since bIsChkHasFile is set, the scan is skipped when one file is found
						//pFileDBInfo->u32TotalFilesNum = pFileDBInfo->u32CurrentFileIndex;
						pFileDBInfo->u32TotalFilesNum = 1;
						//#NT#2015/01/16#Nestor Yang -end
						goto L_Scan_FindFile;
					} else if (pFileDBInfo->fpChkAbort != NULL && pFileDBInfo->fpChkAbort()) {
						DBG_ERR("Abort !!\r\n");
						goto L_Scan_Error;
					}
					pFileDBInfo->u32TotalFilesNum = pFileDBInfo->u32CurrentFileIndex;
					//Fit our DCF rule
					if (pFileDBInfo->bIsDCFFileOnly && pFileDBInfo->bIsFilterOutSameDCFNumFile) {
						FileDB_FilterOutSameDCFnumFile(pFileDBInfo);
					}

				}
			}
		}
		FileDB_RemoveDir(fileDbHandle);
	}
L_Scan_FindFile:
	return E_OK;
L_Scan_Error:
	return E_SYS;
}

/**
  Create a File DataBase

*/
FILEDB_HANDLE FileDB_Create(PFILEDB_INIT_OBJ pfileDbInitObj)
{
	FILEDB_HANDLE      fileDbHandle;
	PFILEDB_INFO       pFileDBInfo;

#if 1
	if(avl_check_available(__xstring(__section_name__))!=TRUE)
	{
	    DBG_FATAL("Not Support .\r\n");
		vos_util_delay_ms(100);         // add delay to show dbg_fatal message in linux
		vos_debug_halt();
	}
#endif

	// check if flag initialized
	if (!FLG_ID_FILEDB) {
		DBG_ERR("FLG_ID is not installed\r\n");
		return FILEDB_CREATE_ERROR;
	}
	if (!pfileDbInitObj->rootPath) {
		DBG_ERR("rootPath is NULL\r\n");
		return FILEDB_CREATE_ERROR;
	}
	if (!pfileDbInitObj->u32MemAddr) {
		DBG_ERR("u32MemAddr is NULL\r\n");
		return FILEDB_CREATE_ERROR;
	}
	if (pfileDbInitObj->u32MemAddr % 4) {
		DBG_ERR("u32MemAddr (0x%x) not 4 bytes align\r\n", pfileDbInitObj->u32MemAddr);
		return FILEDB_CREATE_ERROR;
	}
	if (!pfileDbInitObj->u32MemSize) {
		DBG_ERR("u32MemSize is 0\r\n");
		return FILEDB_CREATE_ERROR;
	}
	if (pfileDbInitObj->u32MaxFilePathLen <= FILEDB_ROOT_PATH_LEN) {
		DBG_ERR("u32MaxFilePathLen (%d) is to short\r\n", pfileDbInitObj->u32MaxFilePathLen);
		return FILEDB_CREATE_ERROR;
	}
	if (pfileDbInitObj->u32MaxFilePathLen > FILEDB_NAME_MAX_LENG) {
		DBG_ERR("u32MaxFilePathLen (%d) over limit (%d)\r\n", pfileDbInitObj->u32MaxFilePathLen, FILEDB_NAME_MAX_LENG);
		return FILEDB_CREATE_ERROR;
	}
	if (!pfileDbInitObj->u32MaxFileNum) {
		DBG_ERR("u32MaxFileNum is 0\r\n");
		return FILEDB_CREATE_ERROR;
	}
	if (pfileDbInitObj->u32MemSize < FileDB_CalcBuffSize(pfileDbInitObj->u32MaxFileNum, pfileDbInitObj->u32MaxFilePathLen)) {
		DBG_ERR("u32MemSize %d < need %d\r\n", pfileDbInitObj->u32MemSize, FileDB_CalcBuffSize(pfileDbInitObj->u32MaxFileNum, pfileDbInitObj->u32MaxFilePathLen));
		return FILEDB_CREATE_ERROR;
	}
	// check non-Used FileDB
	FileDB_lockComm();
	fileDbHandle = FileDB_GetFreeHandle();
	if (fileDbHandle == FILEDB_CREATE_ERROR) {
		DBG_ERR("FileDB handle are all used\r\n");
		FileDB_unlockComm();
		return FILEDB_CREATE_ERROR;
	}
	gFileDBInfo[fileDbHandle].InitTag = FILEDB_INITED_VAR;
	gFileDBInfo[fileDbHandle].pInfo = (PFILEDB_INFO)pfileDbInitObj->u32MemAddr;
	pFileDBInfo = gFileDBInfo[fileDbHandle].pInfo;
	memset(pFileDBInfo, 0x00, sizeof(FILEDB_INFO));
	pFileDBInfo->HeadTag = gFileDBHandleTag[fileDbHandle];
	pFileDBInfo->EndTag = gFileDBHandleTag[fileDbHandle];
	FileDB_unlockComm();
	FileDB_lock(fileDbHandle);
	pFileDBInfo->u32CurrentFileIndex = 0;
	pFileDBInfo->u32SpecificFileIndex = SPECIFIC_FILE_INDEX_INIT;
	pFileDBInfo->bIsCyclic = pfileDbInitObj->bIsCyclic;
	pFileDBInfo->bIsRecursive = pfileDbInitObj->bIsRecursive;
	pFileDBInfo->bIsSkipDirForNonRecursive = pfileDbInitObj->bIsSkipDirForNonRecursive;
	pFileDBInfo->bIsSkipHidden = pfileDbInitObj->bIsSkipHidden;
	pFileDBInfo->fileFilter = pfileDbInitObj->fileFilter;

	//For check if no files only
	pFileDBInfo->bIsChkHasFile = pfileDbInitObj->bIsChkHasFile;
	pFileDBInfo->sortType = FILEDB_SORT_BY_NONE;

	strncpy(pFileDBInfo->rootPath, pfileDbInitObj->rootPath, FILEDB_NAME_MAX_LENG);
	if (pfileDbInitObj->defaultfolder) {
		strncpy(pFileDBInfo->defaultfolder, pfileDbInitObj->defaultfolder, FILEDB_NAME_MAX_LENG);
	}
	if (pfileDbInitObj->filterfolder) {
		strncpy(pFileDBInfo->filterfolder, pfileDbInitObj->filterfolder, FILEDB_NAME_MAX_LENG);
	}
	pFileDBInfo->bIsDCFFileOnly = pfileDbInitObj->bIsDCFFileOnly;
	pFileDBInfo->bIsSupportLongName = pfileDbInitObj->bIsSupportLongName;
	if (pFileDBInfo->bIsDCFFileOnly) {
		strncpy(pFileDBInfo->rootPath, gPathDCIM, FILEDB_NAME_MAX_LENG);
		pFileDBInfo->bIsFilterOutSameDCFNumFolder = pfileDbInitObj->bIsFilterOutSameDCFNumFolder;
		memcpy((UINT8 *)pFileDBInfo->OurDCFDirName, pfileDbInitObj->OurDCFDirName, DCF_DIR_NAME_LEN);
		memcpy((UINT8 *)pFileDBInfo->OurDCFFileName, pfileDbInitObj->OurDCFFileName, DCF_FILE_NAME_LEN);
		pFileDBInfo->bIsFilterOutSameDCFNumFile = pfileDbInitObj->bIsFilterOutSameDCFNumFile;
	}

	pFileDBInfo->u32MemAddr = pfileDbInitObj->u32MemAddr;
	pFileDBInfo->u32MemSize = pfileDbInitObj->u32MemSize;
	pFileDBInfo->u32MaxFileNum = pfileDbInitObj->u32MaxFileNum;
	pFileDBInfo->u32MaxFilePathLen = ALIGN_CEIL_4(pfileDbInitObj->u32MaxFilePathLen);
	pFileDBInfo->u32FileItemSize = sizeof(FILEDB_FILE_ATTR) + pFileDBInfo->u32MaxFilePathLen;
	pFileDBInfo->u32IndexItemSize = 4;

	pFileDBInfo->pDCFDirTbl = (UINT8 *)pFileDBInfo->u32MemAddr + ALIGN_CEIL_4(sizeof(FILEDB_INFO));
	pFileDBInfo->DCFDirTblSize = ALIGN_CEIL_4(FILEDB_MAX_DIFF_DCF_FOLDER);
	pFileDBInfo->pDCFFileTbl = (UINT8 *)((UINT32)pFileDBInfo->pDCFDirTbl + pFileDBInfo->DCFDirTblSize);
	pFileDBInfo->DCFFileTblSize = ALIGN_CEIL_4(FILEDB_MAX_DCF_FILENUM);
	pFileDBInfo->pSortIndex = (UINT32 *)((UINT32)pFileDBInfo->pDCFFileTbl + pFileDBInfo->DCFFileTblSize);
	pFileDBInfo->u32MaxStoreItemsNum = FILEDB_RESERVE_FILE_ENTRY_NUM + pFileDBInfo->u32MaxFileNum;
	pFileDBInfo->SortIndexBufSize = pFileDBInfo->u32MaxStoreItemsNum * pFileDBInfo->u32IndexItemSize;
	pFileDBInfo->pFileInfoAddr = ((UINT32)pFileDBInfo->pSortIndex + pFileDBInfo->SortIndexBufSize);

	if (pfileDbInitObj->SortSN_Delim) {
		pFileDBInfo->SortSN_DelimLen = strlen(pfileDbInitObj->SortSN_Delim);
		if (pFileDBInfo->SortSN_DelimLen > SORTSN_MAX_DELIM_CHAR_NUM || pFileDBInfo->SortSN_DelimLen < 1) {
			DBG_ERR("SortSN_DelimLen length(%d) should be 1 ~ MAX(%d)\r\n", pFileDBInfo->SortSN_DelimLen, SORTSN_MAX_DELIM_CHAR_NUM);
			goto L_Create_Error;
		}

		strncpy(pFileDBInfo->SortSN_Delim, pfileDbInitObj->SortSN_Delim, SORTSN_MAX_DELIM_CHAR_NUM);
		pFileDBInfo->SortSN_Delim[SORTSN_MAX_DELIM_CHAR_NUM] = '\0';

		pFileDBInfo->SortSN_DelimCount = pfileDbInitObj->SortSN_DelimCount;
		if (pFileDBInfo->SortSN_DelimCount < 1) {
			DBG_ERR("Invalid SortSN_DelimCount(%d)\r\n", pFileDBInfo->SortSN_DelimCount);
			goto L_Create_Error;
		}

		pFileDBInfo->SortSN_CharNumOfSN = pfileDbInitObj->SortSN_CharNumOfSN;
		if (pFileDBInfo->SortSN_CharNumOfSN > SORTSN_MAX_CHAR_NUM_OF_SN) {
			DBG_ERR("SortSN_CharNumOfSN > MAX(%d)\r\n", SORTSN_MAX_CHAR_NUM_OF_SN);
			goto L_Create_Error;
		}

		DBG_IND("SortSN_Delim = %s\r\n", pFileDBInfo->SortSN_Delim);
		DBG_IND("SortSN_DelimLen = %d\r\n", pFileDBInfo->SortSN_DelimLen);
		DBG_IND("SortSN_DelimCount = %d\r\n", pFileDBInfo->SortSN_DelimCount);
		DBG_IND("SortSN_CharNumOfSN = %d\r\n", pFileDBInfo->SortSN_CharNumOfSN);
	}

	DBG_IND("[buff] pDCFDirTbl Addr=0x%x, size = 0x%x\r\n", pFileDBInfo->pDCFDirTbl, pFileDBInfo->DCFDirTblSize);
	DBG_IND("[buff] pDCFFileTbl Addr=0x%x, size = 0x%x\r\n", pFileDBInfo->pDCFFileTbl, pFileDBInfo->DCFFileTblSize);
	DBG_IND("[buff] pSortIndex Addr=0x%x, size = 0x%x\r\n", pFileDBInfo->pSortIndex, pFileDBInfo->SortIndexBufSize);
	DBG_IND("[buff] pFileInfoAddr Addr=0x%x , u32MaxStoreItemsNum = %d\r\n", pFileDBInfo->pFileInfoAddr, pFileDBInfo->u32MaxStoreItemsNum);

	DBG_IND(" FILE_ATTR size=%d, max stored num= %d\r\n", pFileDBInfo->u32FileItemSize, pFileDBInfo->u32MaxStoreItemsNum);
#if (_SUPPORT_LONG_FILENAME_IN_FILEDB_)
	pFileDBInfo->fpChkLongname = pfileDbInitObj->fpChkLongname;
#endif
	pFileDBInfo->fpChkAbort = pfileDbInitObj->fpChkAbort;

	if (FileDB_Scan(fileDbHandle) != E_OK) {
		goto L_Create_Error;
	}
	if (pfileDbInitObj->bIsMoveToLastFile) {
		//  Set current index to last item
		pFileDBInfo->u32CurrentFileIndex = pFileDBInfo->u32TotalFilesNum - 1;
	} else {
		//  Set current index to first item
		pFileDBInfo->u32CurrentFileIndex = 0;
	}
	FileDB_unlock(fileDbHandle);
	return fileDbHandle;

L_Create_Error:
	FileDB_Release(fileDbHandle);
	FileDB_unlock(fileDbHandle);
	return FILEDB_CREATE_ERROR;

}

/**
  Remove the Directory Items from File DataBase.

  Remove the Directory Items from File DataBase. When caller calls FileDB_Create() to create a File DataBase
  and set the parameter bIsRecursive to TRUE, in which case we need to remove the directory items from database

  @param FILEDB_HANDLE fileDbHandle: refer to FileDB.h, the possible value should be >=0 and <FILEDB_NUM
  @return void
*/
static void FileDB_RemoveDir(FILEDB_HANDLE fileDbHandle)
{
	UINT32               i, u32FileCount, nameOffset;
	PFILEDB_FILE_ATTR    pFileDbFileAttr, pTempFileDbFileAttr;
	PFILEDB_INFO         pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return;
	}

	u32FileCount = 0;
	for (i = 0; i < pFileDBInfo->u32TotalFilesNum; i++) {
		pFileDbFileAttr = FileDB_GetFileAttr(pFileDBInfo, i);
		if (!M_IsDirectory(pFileDbFileAttr->attrib)) {
			*(pFileDBInfo->pSortIndex + u32FileCount) = i;
			u32FileCount++;
			if (u32FileCount >= pFileDBInfo->u32MaxFileNum) {
				break;
			}
		}
	}
	pFileDBInfo->u32TotalFilesNum = u32FileCount;
	for (i = 0; i < u32FileCount; i++) {
		pFileDbFileAttr = FileDB_GetFileAttr(pFileDBInfo, i);
		pTempFileDbFileAttr = FileDB_GetFileAttr(pFileDBInfo, *(pFileDBInfo->pSortIndex + i));
		if (pFileDbFileAttr != pTempFileDbFileAttr) {
			memcpy((void *)pFileDbFileAttr, (void *)pTempFileDbFileAttr, pFileDBInfo->u32FileItemSize);
			nameOffset = (UINT32)pFileDbFileAttr->filename - (UINT32)pFileDbFileAttr->filePath;
			pFileDbFileAttr->filePath = (CHAR *)((UINT32)pFileDbFileAttr + sizeof(FILEDB_FILE_ATTR));
			pFileDbFileAttr->filename = &pFileDbFileAttr->filePath[nameOffset];
		}
	}
	// Set the sort Index again
	for (i = 0; i < u32FileCount; i++) {
		*(pFileDBInfo->pSortIndex + i) = i;
	}
}

/**
  Get The File Filter String.

  Get The File Filter String.

  @param[in] UINT32 fileFilter   : It can be the combine value of FILEDB_FMT_JPG, FILEDB_FMT_MP3, FILEDB_FMT_AVI.. refer to FileDB.h
  @param[out] char *filterStr    : The file filter string, ex: "JPG MP3 AVI"
  @param[in] UINT16 u16StrMaxLen : The max string len of filterStr
  @return void
*/
static void FileDB_GetFileFilterStr(UINT32 fileFilter, char *filterStr, UINT16 u16StrMaxLen)
{
	UINT16 i;
	memset(filterStr, 0x00, u16StrMaxLen);
	for (i = 0; i < FILEDB_SUPPORT_FILE_TYPE_NUM; i++) {
		if (fileDbFiletype[i].fileType & fileFilter) {
			strncat(filterStr, fileDbFiletype[i].filterStr, u16StrMaxLen - strlen(filterStr));
			strncat(filterStr, " ", u16StrMaxLen - strlen(filterStr));
		}
	}
}

/**
  Get The file type by file extension name.

  Get The file type by file extension name.

  @param[in] char *ExtName : The file extension of this file type EX:"MP3", refer to FILEDB_FILE_TYPE in FileDB.c
  @return UINT32, it can be FILEDB_FMT_JPG, FILEDB_FMT_MP3 or FILEDB_FMT_AVI ...refer to FileDB.h
*/
static UINT32 FileDB_GetFileType(const char *ExtName)
{
	UINT32 i;
	UINT32 fileType = 0;
	CHAR   Ext[FILE_EXT_LEN + 1];

	for (i = 0; i < FILE_EXT_LEN; i++) {
		Ext[i] = ExtName[i];
		if (Ext[i] >= 'a' && Ext[i] <= 'z') {
			Ext[i] -= 0x20;
		}
	}
	Ext[i] = 0;
	for (i = 0; i < FILEDB_SUPPORT_FILE_TYPE_NUM; i++) {
		if (strstr(fileDbFiletype[i].filterStr, Ext) != 0) {
			return fileDbFiletype[i].fileType;
		}
	}
	return fileType;
}
// root is "A:\" , filter folder is "DCIM|CARDV"
static BOOL FileDB_ChkFilterFolder(PFILEDB_INFO  pFileDBInfo, const CHAR *currPath, const CHAR *folderName)
{
#define MAX_FILTER_FOLDER_CNT             (10)
	UINT32  filterFolderOffset[MAX_FILTER_FOLDER_CNT];
	UINT32  filterFolderCount = 0;

	if (pFileDBInfo->u32RootDepth == pFileDBInfo->u32CurrScanDirDepth) {
		if (FileDB_MatchNameUpper(pFileDBInfo->rootPath, currPath)) {
			UINT32 i = 0, strlen = 0;


			// get offset of each folder name  , filter folder example is "DCIM CARDV"
			while (i < FILEDB_NAME_MAX_LENG && filterFolderCount < MAX_FILTER_FOLDER_CNT) {
				if (pFileDBInfo->filterfolder[i] == '|' || pFileDBInfo->filterfolder[i] == 0) {
					if (strlen) {
						filterFolderOffset[filterFolderCount] = i - strlen;
						filterFolderCount++;
						i++;
						strlen = 0;
						if (pFileDBInfo->filterfolder[i]) {
							continue;
						} else {
							break;
						}
					}
				}
				strlen++;
				i++;
			}
			//
			for (i = 0; i < filterFolderCount; i++) {
				DBG_IND("[scan]folderName (%s) filterFolder (%s) \r\n", folderName, &pFileDBInfo->filterfolder[filterFolderOffset[i]]);
				if (FileDB_MatchNameUpper(&pFileDBInfo->filterfolder[filterFolderOffset[i]], folderName)) {
					return TRUE;
				}
			}
			return FALSE;
		}
	}
	return TRUE;
}
/**
  The Callback Function of FileSys_ScanDir().

  The Callback Function of FileSys_ScanDir().

  @param FIND_DATA *findDir  : The find file info, structure refer to fs_file_op.h
  @param BOOL *bContinue     : If want to continue the file search in fuction FileSys_ScanDir().
  @param char *cLongname     : The Long file name of this file
  @param UINT32 Param        : no used here
  @return void
*/
static void FileDB_FileSysDirCB(FIND_DATA *findDir, BOOL *bContinue, UINT16 *cLongname, UINT32 Param)
{
	CHAR          ExtName[FILE_EXT_LEN + 1];
	UINT32        u32CurrIndex;
	CHAR          filterStr[FILE_FILTER_MAX_LEN + 1];
	PFILEDB_FILE_INT_ATTR  pFileDbFileAttr;
	UINT32        uiFileType;
	PFILEDB_INFO  pFileDBInfo;
	UINT32        filename_len;
	//#NT#2015/02/25#Nestor Yang -begin
	//#NT# Get the handle from Param
	//CHAR          *path = (CHAR*)Param;
	//pFileDBInfo = FileDB_GetInfoByPath(path);
	pFileDBInfo = FileDB_GetInfoByHandle((FILEDB_HANDLE)Param);
	//#NT#2015/02/25#Nestor Yang -end
	if (!pFileDBInfo) {
		return;
	}

	u32CurrIndex = pFileDBInfo->u32CurrentFileIndex;
	if ((u32CurrIndex >= pFileDBInfo->u32MaxStoreItemsNum)) {
		*bContinue = FALSE;
		DBG_ERR("Index(%d) Exceeds Max Item \r\n", pFileDBInfo->u32CurrentFileIndex);
		return;
	} else if (pFileDBInfo->bIsRecursive && (u32CurrIndex >= (pFileDBInfo->folderCount + pFileDBInfo->u32MaxFileNum))) {
		*bContinue = FALSE;
		DBG_ERR("Index(%d) Exceeds Max Item \r\n", pFileDBInfo->u32CurrentFileIndex);
		return;
	} else if ((pFileDBInfo->bIsRecursive == FALSE) && (u32CurrIndex >= pFileDBInfo->u32MaxFileNum)) {
		*bContinue = FALSE;
		DBG_ERR("Index(%d) Exceeds Max Item \r\n", pFileDBInfo->u32CurrentFileIndex);
		return;
	}

	pFileDbFileAttr = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, u32CurrIndex);
	if (pFileDbFileAttr == NULL) {
		*bContinue = FALSE;
		DBG_ERR("Index(%d) GetFileAttr Error \r\n", pFileDBInfo->u32CurrentFileIndex);
		return;
	}

	//skip '.' and '..' and label entries but keep continued
	filename_len = strlen(findDir->filename);
	if ((0 == strncmp(findDir->filename, ".", 1) && filename_len == 1) || 0 == strncmp(findDir->filename, "..", 2) || M_IsVolumeLabel(findDir->attrib)) {
		return;
	}

	//skip folders for non-recursive case but keep continued
	if (FALSE == pFileDBInfo->bIsRecursive && TRUE == pFileDBInfo->bIsSkipDirForNonRecursive && M_IsDirectory(findDir->attrib)) {
		return;
	}

	//skip hidden items
	if (TRUE == pFileDBInfo->bIsSkipHidden && M_IsHidden(findDir->attrib)) {
		return;
	}

	// Clear the memory
	memset(pFileDbFileAttr, 0x00, pFileDBInfo->u32FileItemSize);
	FileDB_GetFileFilterStr(pFileDBInfo->fileFilter, filterStr, FILE_FILTER_MAX_LEN + 1);
	memset(ExtName, 0x00, sizeof(ExtName));
	strncpy(ExtName, findDir->FATExtName, FILE_EXT_LEN);

	if (M_IsDirectory(findDir->attrib) || (pFileDBInfo->fileFilter == FILEDB_FMT_ANY) || (ExtName[0] && (strstr(filterStr, ExtName) != 0))) {
		//Fit our DCF rule
		if ((pFileDBInfo->bIsDCFFileOnly == FALSE) ||
			((pFileDBInfo->bIsDCFFileOnly == TRUE)  && (M_IsDirectory(findDir->attrib)) && FileDB_IsValidDCFDir(findDir->filename) && (pFileDBInfo->bIsGetFile == FALSE)) ||
			((pFileDBInfo->bIsDCFFileOnly == TRUE)  && (!M_IsDirectory(findDir->attrib)) && FileDB_IsValidDCFFile(findDir->FATMainName, &uiFileType)  && (pFileDBInfo->bIsGetFile == TRUE))
		   ) {
			UINT32 filePathlen;
			//For check if no files only
			if (pFileDBInfo->bIsChkHasFile && (!M_IsDirectory(findDir->attrib))) {
				*bContinue = FALSE;
				u32CurrIndex++;
				pFileDBInfo->u32CurrentFileIndex = u32CurrIndex;
				pFileDBInfo->bIsFindMatchFile = TRUE;
			}
			// check if file path exceed MaxFilePathLen
			//if ((pFileDBInfo->u32CurrScanDirLength+FILEDB_SHORT_NAME_LEN > pFileDBInfo->u32MaxFilePathLen-1) && (!M_IsDirectory(findDir->attrib)) )
			if ((pFileDBInfo->u32CurrScanDirLength + strlen(findDir->filename) > pFileDBInfo->u32MaxFilePathLen - 1)) {
				DBG_ERR("%s%s exceeds MaxFilePathLen\r\n", pFileDBInfo->currentDir, findDir->filename);
				*bContinue = FALSE;
				return;
			}
			// file path
			pFileDbFileAttr->filePath = (CHAR *)((UINT32)pFileDbFileAttr + sizeof(FILEDB_FILE_ATTR));

			strncpy(pFileDbFileAttr->filePath, pFileDBInfo->currentDir, pFileDBInfo->u32MaxFilePathLen - 1);
			filePathlen = strlen(pFileDbFileAttr->filePath);
			pFileDbFileAttr->filename = &pFileDbFileAttr->filePath[filePathlen];
			// has long name
			if (cLongname[0] != 0) {
				UINT32 i, isNormalFinish = FALSE;

				for (i = 0; i < pFileDBInfo->u32MaxFilePathLen - filePathlen; i++) {
					if (cLongname[i] == 0) {
						isNormalFinish = TRUE;
						break;
					}

					// ascii value
					if (cLongname[i] < 128) {
						pFileDbFileAttr->filename[i] = (CHAR)cLongname[i];
					}
					// has non ascii value, need to use short name instead
					else {
						break;
					}
				}
				if (!isNormalFinish) {
					pFileDbFileAttr->filename[0] = '\0';
					strncpy(pFileDbFileAttr->filename, findDir->filename, pFileDBInfo->u32MaxFilePathLen - 1 - filePathlen);
					if (i == pFileDBInfo->u32MaxFilePathLen - filePathlen) {
						DBG_ERR("%s exceeds MaxFilePathLen\r\n", pFileDbFileAttr->filePath);
					}

				}
			}
			// no long name
			else {
				strncpy(pFileDbFileAttr->filename, findDir->filename, pFileDBInfo->u32MaxFilePathLen - 1 - filePathlen);
			}
			// file name
#if (_SUPPORT_LONG_FILENAME_IN_FILEDB_)
			if (cLongname[0] == 0) {
				for (i = 0; i < FILEDB_MAX_LONG_FILENAME_LEN; i++) {
					pFileDbFileAttr->longName[i] = pFileDbFileAttr->filename[i];
				}
				pFileDbFileAttr->bIsNeedRename = FileDB_ChkIfChiWordInside(pFileDbFileAttr->filename);
			} else {
				memcpy(pFileDbFileAttr->longName, cLongname, (FILEDB_MAX_LONG_FILENAME_LEN << 1));
			}
			if (pFileDBInfo->fpChkLongname) {
				if (cLongname[0] != 0) {
					pFileDbFileAttr->bIsNeedRename = pFileDBInfo->fpChkLongname(cLongname, KFS_LONGFILENAME_MAX_LENG);
				}
			}
#endif
			// This is a folder
			if (M_IsDirectory(findDir->attrib)) {
				// check filter folder
				if (pFileDBInfo->filterfolder[0]) {
					// folder not match filter folder , so this folder can't be added
					if (!FileDB_ChkFilterFolder(pFileDBInfo, pFileDBInfo->currentDir, pFileDbFileAttr->filename)) {
						DBG_IND("[scan]name =%s filter out\r\n", pFileDbFileAttr->filename);
						return;
					}
				}
				// Add folder count
				pFileDBInfo->folderCount++;
			}
			// This is a file
			else {
				// file type
				pFileDbFileAttr->fileType = FileDB_GetFileType(ExtName);

				// Add file count for different file type
				FileDB_AddFilesNum(pFileDBInfo, pFileDbFileAttr->fileType);
			}
			// file attribute
			pFileDbFileAttr->attrib = findDir->attrib;
			// file size
			pFileDbFileAttr->fileSize64 = (UINT64)findDir->fileSize + ((UINT64)findDir->fileSizeHI << 32);
			// file created date
			pFileDbFileAttr->creDate = findDir->creDate;
			pFileDbFileAttr->creTime = findDir->creTime;
			pFileDbFileAttr->lastWriteTime = findDir->lastWriteTime;
			pFileDbFileAttr->lastWriteDate = findDir->lastWriteDate;
			// set file sort weight
			pFileDbFileAttr->sortWeight = pFileDBInfo->sortWeight;
			// Set the sort index initial value
			*(pFileDBInfo->pSortIndex + u32CurrIndex) = u32CurrIndex;
			// file index
			u32CurrIndex++;
			pFileDBInfo->u32CurrentFileIndex = u32CurrIndex;
			DBG_IND("[scan]name =%s , file =(%s) w=0x%x\r\n", findDir->filename, pFileDbFileAttr->filePath, pFileDbFileAttr->sortWeight);
		}
	}
}

/**
  Release a File DataBase.

  Release a File DataBase.

  @param FILEDB_HANDLE fileDbHandle: refer to FileDB.h, the possible value should be >=0 and <FILEDB_NUM
  @return void
*/
void FileDB_Release(FILEDB_HANDLE fileDbHandle)
{
	PFILEDB_INFO            pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return;
	}

	FileDB_lockComm();

	gFileDBInfo[fileDbHandle].InitTag = 0;
	gFileDBInfo[fileDbHandle].pInfo = 0;
	pFileDBInfo->HeadTag = 0;
	pFileDBInfo->EndTag = 0;

	//#NT#2015/11/27#Nestor Yang#[0088253] -begin
	//#NT# Flush to prevent the racing issue of non-cache and cache data
#if defined (__UITRON) || defined (__ECOS)
	dma_flushWriteCache(pFileDBInfo->u32MemAddr, pFileDBInfo->u32MemSize);
#else
	vos_cpu_dcache_sync(pFileDBInfo->u32MemAddr, pFileDBInfo->u32MemSize, VOS_DMA_TO_DEVICE);
#endif
	//#NT#2015/11/27#Nestor Yang#[0088253] -end

	FileDB_unlockComm();

	DBG_IND("handle %d ok!!", fileDbHandle);
}

/**
  Get total files number from a File DataBase.

  Get total files number from a File DataBase.

  @param FILEDB_HANDLE fileDbHandle: refer to FileDB.h, the possible value should be >=0 and <FILEDB_NUM
  @return UINT16 The total files number of this File DataBase
*/
UINT32 FileDB_GetTotalFileNum(FILEDB_HANDLE fileDbHandle)
{
	PFILEDB_INFO            pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return 0;
	}
	return pFileDBInfo->u32TotalFilesNum;
}

static BOOL FileDB_ChkFileAttr(PFILEDB_FILE_ATTR pFileAttr)
{
	PFILEDB_FILE_INT_ATTR    pFileIntAttr;

	pFileIntAttr = (PFILEDB_FILE_INT_ATTR)pFileAttr;
	if (pFileIntAttr && pFileIntAttr->sortWeight < FILEDB_SORT_WEIGHT_MAX && pFileIntAttr->sortWeight > FILEDB_SORT_WEIGHT_MIN) {
		return TRUE;
	}
	return FALSE;
}


static PFILEDB_FILE_ATTR FileDB_internalSearhFile(FILEDB_HANDLE fileDbHandle, INT32 i32FileIndex)
{
	UINT32             u32Index;
	PFILEDB_FILE_ATTR  pFileAttr;
	PFILEDB_INFO       pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return NULL;
	}
	if (pFileDBInfo->u32TotalFilesNum == 0) {
		DBG_ERR("u32TotalFilesNum=0\r\n");
		return NULL;
	}
	if (i32FileIndex >= (INT32)pFileDBInfo->u32TotalFilesNum) {
		if (pFileDBInfo->bIsCyclic == TRUE) {
			u32Index = (UINT32)i32FileIndex % pFileDBInfo->u32TotalFilesNum;
		} else {
			u32Index = pFileDBInfo->u32TotalFilesNum - 1;
		}
	} else if (i32FileIndex < 0) {
		if (pFileDBInfo->bIsCyclic == TRUE) {
			u32Index = (UINT32)i32FileIndex + pFileDBInfo->u32TotalFilesNum;
		} else {
			u32Index = 0;
		}
	} else {
		u32Index = (UINT32)i32FileIndex;
	}
	if (u32Index >= pFileDBInfo->u32TotalFilesNum) {
		DBG_ERR("[search]fileDbHandle=%d, i32FileIndex=%d, u32Index=%d \r\n", fileDbHandle, i32FileIndex, u32Index);
		return NULL;
	}
	DBG_IND("[search]fileDbHandle=%d, i32FileIndex=%d, u32Index=%d \r\n", fileDbHandle, i32FileIndex, u32Index);
	pFileDBInfo->u32CurrentFileIndex = u32Index;

	pFileAttr =  FileDB_GetFileAttr(pFileDBInfo, u32Index);
	if (!FileDB_ChkFileAttr(pFileAttr)) {
		DBG_ERR("file %04d data is OverWrite\r\n", pFileDBInfo->u32CurrentFileIndex);
	}
	return pFileAttr;

}

PFILEDB_FILE_ATTR FileDB_SearhFile(FILEDB_HANDLE fileDbHandle, INT32 i32FileIndex)
{
	PFILEDB_FILE_ATTR  pFileAttr;

	FileDB_lock(fileDbHandle);
	pFileAttr = FileDB_internalSearhFile(fileDbHandle, i32FileIndex);
	FileDB_unlock(fileDbHandle);
	return pFileAttr;

}
/**
  Use FileIndex to get one file's info.

  Use FileIndex to get one file's info. The current index of this File DataBase will not move.
  (This is the major difference between FileDB_SearhFile2 & FileDB_SearhFile)

  @param FILEDB_HANDLE fileDbHandle: refer to FileDB.h, the possible value should be >=0 and <FILEDB_NUM
  @param INT16 i16FileIndex        :
  @return PFILEDB_FILE_ATTR structure refer to FileDB.h
*/
PFILEDB_FILE_ATTR FileDB_SearhFile2(FILEDB_HANDLE fileDbHandle, INT32 i32FileIndex)
{
	UINT32             u32TmpIndex;
	PFILEDB_FILE_ATTR  pFileAttr;
	PFILEDB_INFO       pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return NULL;
	}
	FileDB_lock(fileDbHandle);
	u32TmpIndex = pFileDBInfo->u32CurrentFileIndex;
	pFileAttr = FileDB_internalSearhFile(fileDbHandle, i32FileIndex);
	pFileDBInfo->u32CurrentFileIndex = u32TmpIndex;
	if (!FileDB_ChkFileAttr(pFileAttr)) {
		DBG_ERR("file %04d data is OverWrite\r\n", pFileDBInfo->u32CurrentFileIndex);
	}
	FileDB_unlock(fileDbHandle);
	return pFileAttr;
}

/**
  Get the file info of current index.

  Get the file info of current index.

  @param FILEDB_HANDLE fileDbHandle: refer to FileDB.h, the possible value should be >=0 and <FILEDB_NUM
  @return PFILEDB_FILE_ATTR structure refer to FileDB.h
*/
PFILEDB_FILE_ATTR FileDB_CurrFile(FILEDB_HANDLE fileDbHandle)
{
	PFILEDB_FILE_ATTR  pFileAttr;
	PFILEDB_INFO       pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return NULL;
	}
	if (pFileDBInfo->u32TotalFilesNum == 0) {
		DBG_ERR("u32TotalFilesNum=0\r\n");
		return NULL;
	}
	pFileAttr = FileDB_GetFileAttr(pFileDBInfo, pFileDBInfo->u32CurrentFileIndex);
	if (!FileDB_ChkFileAttr(pFileAttr)) {
		DBG_ERR("file %04d data is OverWrite\r\n", pFileDBInfo->u32CurrentFileIndex);
	}
	return pFileAttr;
}

/**
  Get the file info of next file index.

  Move the current file index to next item and return the file info.

  @param FILEDB_HANDLE fileDbHandle: refer to FileDB.h, the possible value should be >=0 and <FILEDB_NUM
  @return PFILEDB_FILE_ATTR structure refer to FileDB.h
*/
PFILEDB_FILE_ATTR FileDB_NextFile(FILEDB_HANDLE fileDbHandle)
{
	PFILEDB_FILE_ATTR  pFileAttr;
	PFILEDB_INFO       pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return NULL;
	}
	if (pFileDBInfo->u32TotalFilesNum == 0) {
		DBG_ERR("u32TotalFilesNum=0\r\n");
		return NULL;
	}
	FileDB_lock(fileDbHandle);
	if (pFileDBInfo->u32CurrentFileIndex >= pFileDBInfo->u32TotalFilesNum - 1) {
		if (pFileDBInfo->bIsCyclic == TRUE) {
			pFileDBInfo->u32CurrentFileIndex = 0;
		} else {
			DBG_ERR("already last index\r\n");
			pFileAttr = NULL;
			goto L_NextFile_Error;
		}
	} else {
		pFileDBInfo->u32CurrentFileIndex++;
	}
	pFileAttr = FileDB_GetFileAttr(pFileDBInfo, pFileDBInfo->u32CurrentFileIndex);
	if (!FileDB_ChkFileAttr(pFileAttr)) {
		DBG_ERR("file %04d data is OverWrite\r\n", pFileDBInfo->u32CurrentFileIndex);
	}
L_NextFile_Error:
	FileDB_unlock(fileDbHandle);
	return pFileAttr;
}

/**
  Get the file info of previous file index.

  Move the current file index to previous item and return the file info.

  @param FILEDB_HANDLE fileDbHandle: refer to FileDB.h, the possible value should be >=0 and <FILEDB_NUM
  @return PFILEDB_FILE_ATTR structure refer to FileDB.h
*/

PFILEDB_FILE_ATTR FileDB_PrevFile(FILEDB_HANDLE fileDbHandle)
{
	PFILEDB_FILE_ATTR  pFileAttr;
	PFILEDB_INFO       pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return NULL;
	}
	if (pFileDBInfo->u32TotalFilesNum == 0) {
		DBG_ERR("u32TotalFilesNum=0\r\n");
		return NULL;
	}
	FileDB_lock(fileDbHandle);
	if (pFileDBInfo->u32CurrentFileIndex == 0) {
		if (pFileDBInfo->bIsCyclic == TRUE) {
			pFileDBInfo->u32CurrentFileIndex = pFileDBInfo->u32TotalFilesNum - 1;
		} else {
			DBG_ERR("already first index\r\n");
			pFileAttr = NULL;
			goto L_PrevFile_Error;
		}
	} else {
		pFileDBInfo->u32CurrentFileIndex--;
	}
	pFileAttr = FileDB_GetFileAttr(pFileDBInfo, pFileDBInfo->u32CurrentFileIndex);
	if (!FileDB_ChkFileAttr(pFileAttr)) {
		DBG_ERR("file %04d data is OverWrite\r\n", pFileDBInfo->u32CurrentFileIndex);
	}
L_PrevFile_Error:
	FileDB_unlock(fileDbHandle);
	return pFileAttr;
}

/**
  Get the Current file index.

  Get the Current file index.

  @param FILEDB_HANDLE fileDbHandle: refer to FileDB.h, the possible value should be >=0 and <FILEDB_NUM
  @return UINT32: the current file index
*/
UINT32 FileDB_GetCurrFileIndex(FILEDB_HANDLE fileDbHandle)
{
	PFILEDB_INFO            pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return 0;
	}
	return pFileDBInfo->u32CurrentFileIndex;
}

/**
  Check if this fileDbHandle is valid.

  Check if this fileDbHandle is valid.

  @param FILEDB_HANDLE fileDbHandle
  @return Return TRUE if this fileDbHandle is valid else return FALSE.
*/
static PFILEDB_INFO FileDB_GetInfoByHandle(FILEDB_HANDLE fileDbHandle)
{
	if (fileDbHandle < 0 || fileDbHandle >= FILEDB_NUM) {
		DBG_ERR("Exceeds FILEDB_NUM(%d)\r\n", fileDbHandle);
		return NULL;
	}
	if (!(gFileDBInfo[fileDbHandle].InitTag == FILEDB_INITED_VAR)) {
		DBG_ERR("This Handle is not created(%d)\r\n", fileDbHandle);
		return NULL;
	}
	if (!(gFileDBInfo[fileDbHandle].pInfo->HeadTag == gFileDBHandleTag[fileDbHandle])) {
		DBG_ERR("FileDB Handle %d Data may be overwritted, Address = 0x%x, Headtag =0x%x\r\n", fileDbHandle, (unsigned int)&gFileDBInfo[fileDbHandle].pInfo->HeadTag, gFileDBInfo[fileDbHandle].pInfo->HeadTag);
		return NULL;
	}
	if (!(gFileDBInfo[fileDbHandle].pInfo->EndTag == gFileDBHandleTag[fileDbHandle])) {
		DBG_ERR("FileDB Handle %d Data maybe overwritted, Address = 0x%x, Endtag =0x%x\r\n", fileDbHandle, (unsigned int)&gFileDBInfo[fileDbHandle].pInfo->EndTag, gFileDBInfo[fileDbHandle].pInfo->EndTag);
		return NULL;
	}
	return gFileDBInfo[fileDbHandle].pInfo;
}

//#NT#2015/02/25#Nestor Yang -begin
//#NT# Callback will get FileDB info from Param(FileDB Handle)
#if 0
static PFILEDB_INFO FileDB_GetInfoByPath(CHAR *path)
{
	PFILEDB_INFO   pInfo;
	UINT32        i;

	for (i = 0; i < FILEDB_NUM; i++) {
		pInfo = gFileDBInfo[i].pInfo;
		if ((gFileDBInfo[i].InitTag == FILEDB_INITED_VAR) && (*path == *pInfo->rootPath)) {
			return pInfo;
		}
	}
	DBG_ERR("Can't find (%s)\r\n", path);
	return NULL;
}
#endif
//#NT#2015/02/25#Nestor Yang -end


/**
  Get the file info of file index.

  Get the file info of file index.

  @param FILEDB_HANDLE fileDbHandle: refer to FileDB.h, the possible value should be >=0 and <FILEDB_NUM
  @param UINT32 u32FileIndex       : The file index
  @return PFILEDB_FILE_ATTR structure refer to FileDB.h
*/
static PFILEDB_FILE_ATTR    FileDB_GetFileAttr(PFILEDB_INFO pFileDBInfo, UINT32 u32FileIndex)
{
	if (pFileDBInfo->sortType != FILEDB_SORT_BY_NONE) {
		return (PFILEDB_FILE_ATTR)(pFileDBInfo->pFileInfoAddr + (pFileDBInfo->u32FileItemSize * (*(pFileDBInfo->pSortIndex + u32FileIndex))));
	} else {
		return (PFILEDB_FILE_ATTR)(pFileDBInfo->pFileInfoAddr + (pFileDBInfo->u32FileItemSize * u32FileIndex));
	}

}

//return the offset of ExtName
static UINT32 FileDB_FindExtNameOffset(const char *filePath, UINT32 len)
{
	UINT32 i = 0;

	for (i = len; i > 0; i--) {
		if (filePath[i] == '.') {
			return i + 1;
		}
	}
	return 0;
}

//return the offset of FileName
static UINT32 FileDB_FindFileNameOffset(const char *filePath, UINT32 len)
{
	UINT32 i = 0;

	for (i = len; i > 0; i--) {
		if (filePath[i] == '\\') {
			return i + 1;
		}
	}
	return 0;
}


static BOOL FileDB_MatchNameUpperWithLen(const char *name1, const char *name2, UINT32 len)
{
	UINT32 i = 0;
	CHAR   c1, c2;

	while (i < len) {
		c1 = name1[i];
		c2 = name2[i];
		if (c1 >= 'a' && c1 <= 'z') {
			c1 -= 0x20;
		}
		if (c2 >= 'a' && c2 <= 'z') {
			c2 -= 0x20;
		}
		if (c1 != c2) {
			return FALSE;
		}
		i++;
	}
	return TRUE;
}

static BOOL FileDB_MatchNameUpper(const char *name1, const char *name2)
{
	UINT32 i = 0;
	CHAR   c1, c2;

	while (i < FILEDB_NAME_MAX_LENG) {
		c1 = name1[i];
		c2 = name2[i];
		if (c1 >= 'a' && c1 <= 'z') {
			c1 -= 0x20;
		}
		if (c2 >= 'a' && c2 <= 'z') {
			c2 -= 0x20;
		}
		if (((c1 == 0) || (c1 == '|')) && ((c2 == 0) || (c2 == '|'))) {
			return TRUE;
		}
		if (c1 != c2) {
			return FALSE;
		}
		i++;
	}
	return TRUE;
}

/**
  Comparing two filename if name1>name2 then return TRUE.

  @param char* name1: the name1 need to be compared
  @param char* name2: the name2 need to be compared
  @return return TRUE if name1>name2, else return FALSE
*/
static BOOL FileDB_CompareName(const char *name1, const char *name2)
{
	UINT32 i = 0;
	CHAR   c1, c2;

	while (i <= FILEDB_NAME_MAX_LENG) {
		c1 = name1[i];
		c2 = name2[i];
		if (c1 >= 'a' && c1 <= 'z') {
			c1 -= 0x20;
		}
		if (c2 >= 'a' && c2 <= 'z') {
			c2 -= 0x20;
		}
		//if (c1==0 || c2==0)
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


static void FileDB_internalSortBy(FILEDB_HANDLE fileDbHandle, FILEDB_SORT_TYPE sortType, BOOL bIsSortLargeFirst)
{

	UINT32         i, j, u32SortMethod;
	PFILEDB_INFO   pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		DBG_ERR("fileDbHandle=%d\r\n", fileDbHandle);
		return;
	}
	// Save the sort type
	pFileDBInfo->sortType = sortType;
	if (pFileDBInfo->u32TotalFilesNum == 0) {
		DBG_IND("u32TotalFilesNum=0\r\n");
		return;
	}
	// reset recusive function call deepth
	pFileDBInfo->recursiveFuncCurDeep = 0;
	pFileDBInfo->recursiveFuncMaxDeep = 0;
	DBG_IND("fileDbHandle=%d, sortType=%d, bIsSortLargeFirst=%d\r\n", fileDbHandle, sortType, bIsSortLargeFirst);
	// Reset the sort index
	for (i = 0; i < pFileDBInfo->u32TotalFilesNum; i++) {
		*(pFileDBInfo->pSortIndex + i) = i;

	}
	pFileDBInfo->bIsSortLargeFirst = bIsSortLargeFirst;
	// Sorting
	u32SortMethod = FileDB_ChooseSortMethod(pFileDBInfo, 0, (INT32)pFileDBInfo->u32TotalFilesNum - 1);
	if (SORT_METHOD_QUICK == u32SortMethod) {
		if (pFileDBInfo->sortType == FILEDB_SORT_BY_CREDATE) {
			FileDB_QuickSortByCreDate(pFileDBInfo, 0, (INT32)pFileDBInfo->u32TotalFilesNum - 1);
		} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_MODDATE) {
			FileDB_QuickSortByModDate(pFileDBInfo, 0, (INT32)pFileDBInfo->u32TotalFilesNum - 1);
		} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_SIZE) {
			FileDB_QuickSortBySize(pFileDBInfo, 0, (INT32)pFileDBInfo->u32TotalFilesNum - 1);
		} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_FILETYPE) {
			FileDB_QuickSortByFileType(pFileDBInfo, 0, (INT32)pFileDBInfo->u32TotalFilesNum - 1);
		} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_TYPE_CRETIME_SIZE) {
			FileDB_QuickSortByTypeCretimeSize(pFileDBInfo, 0, (INT32)pFileDBInfo->u32TotalFilesNum - 1);
		} else if(pFileDBInfo->sortType == FILEDB_SORT_BY_CRETIME_NAME) {
			FileDB_QuickSortByCretimeName(pFileDBInfo, 0, (INT32)pFileDBInfo->u32TotalFilesNum - 1);
		} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_NAME) {
			if (pFileDBInfo->bIsDCFFileOnly) {
				FileDB_QuickSortByDCFName(pFileDBInfo, 0, (INT32)pFileDBInfo->u32TotalFilesNum - 1);
			} else {
				FileDB_QuickSortByName(pFileDBInfo, 0, (INT32)pFileDBInfo->u32TotalFilesNum - 1);
			}
		} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_FILEPATH) {
			FileDB_QuickSortByPath(pFileDBInfo, 0, (INT32)pFileDBInfo->u32TotalFilesNum - 1);
		} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_SN) {
			FileDB_QuickSortBySN(pFileDBInfo, 0, (INT32)pFileDBInfo->u32TotalFilesNum - 1);
		}
#if (_SUPPORT_LONG_FILENAME_IN_FILEDB_)
		else if (pFileDBInfo->sortType == FILEDB_SORT_BY_STROKENUM) {
			FileDB_QuickSortByStrokeNum(pFileDBInfo, 0, (INT32)pFileDBInfo->u32TotalFilesNum - 1);
		}
#endif

		// Inverse the sort
		if (bIsSortLargeFirst == TRUE) {
			i = 0;
			j = pFileDBInfo->u32TotalFilesNum - 1;
			while (i < j) {
				FileDB_ExchangeIndex(pFileDBInfo, i, j);
				i++;
				j--;
			}
		}
	} else if (SORT_METHOD_INSERT == u32SortMethod) {
		FileDB_InsertSort(pFileDBInfo, 0, (INT32)pFileDBInfo->u32TotalFilesNum - 1);
		// Inverse the sort
		if (bIsSortLargeFirst == TRUE) {
			i = 0;
			j = pFileDBInfo->u32TotalFilesNum - 1;
			while (i < j) {
				FileDB_ExchangeIndex(pFileDBInfo, i, j);
				i++;
				j--;
			}
		}
	}
}

void FileDB_SortBy(FILEDB_HANDLE fileDbHandle, FILEDB_SORT_TYPE sortType, BOOL bIsSortLargeFirst)
{
	FileDB_lock(fileDbHandle);
	FileDB_internalSortBy(fileDbHandle, sortType, bIsSortLargeFirst);
	FileDB_unlock(fileDbHandle);
}

/**
  Add files number(+1) by file type.

  Add files number(+1) by file type.

  @param[in] FILEDB_HANDLE fileDbHandle: refer to FileDB.h, the possible value should be >=0 and <FILEDB_NUM
  @param[in] UINT32 fileType   : It can be the value of FILEDB_FMT_JPG, FILEDB_FMT_MP3 or FILEDB_FMT_AVI.. refer to FileDB.h
  @return void
*/
static void FileDB_AddFilesNum(PFILEDB_INFO pFileDBInfo, UINT32 fileType)
{
	UINT32             i, u32Index;
	for (i = 0; i < FILEDB_SUPPORT_FILE_TYPE_NUM; i++) {
		if (fileDbFiletype[i].fileType & fileType) {
			u32Index = FileDB_LOG2(fileDbFiletype[i].fileType);
			if (u32Index < FILEDB_SUPPORT_FILE_TYPE_NUM) {
				pFileDBInfo->filesNum[u32Index]++;
				break;
			} else {
				DBG_ERR("Wrong Index(%d)\r\n", u32Index);
			}
		}
	}
}

/**
  Get files number by file type.

  Get files number by file type.

  @param[in] FILEDB_HANDLE fileDbHandle: refer to FileDB.h, the possible value should be >=0 and <FILEDB_NUM
  @param[in] UINT32 fileType   : It can be the combine value of FILEDB_FMT_JPG, FILEDB_FMT_MP3, FILEDB_FMT_AVI.. refer to FileDB.h
  @return UINT16 The total files number of fileType
*/
UINT32 FileDB_GetFilesNumByFileType(FILEDB_HANDLE fileDbHandle, UINT32 fileType)
{
	UINT32    i, u32Index, u32FilesNum;
	PFILEDB_INFO            pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		DBG_ERR("Invalid handle 0x%X\r\n", fileDbHandle);
		return 0;
	}
	u32FilesNum = 0;
	for (i = 0; i < FILEDB_SUPPORT_FILE_TYPE_NUM; i++) {
		if (fileDbFiletype[i].fileType & fileType) {
			u32Index = FileDB_LOG2(fileDbFiletype[i].fileType);
			if (u32Index < FILEDB_SUPPORT_FILE_TYPE_NUM) {
				u32FilesNum += pFileDBInfo->filesNum[u32Index];
			} else {
				DBG_ERR("Wrong Index(%d)\r\n", u32Index);
			}
		}
	}
	return u32FilesNum;
}


static UINT32 FileDB_LOG2(UINT32 i)
{
	// returns n where n = log2(2^n) = log2(2^(n+1)-1)
	UINT32 iLog2 = 0;
	while ((i >> iLog2) > 1) {
		iLog2++;
	}
	return iLog2;
}

static UINT32 FileDB_CalcSortWeight(PFILEDB_INFO pFileDBInfo, CHAR *path)
{
	UINT32         curDeepth, sortWeight, defaultFolderLen;

	curDeepth = FileDB_GetCurDirDeepth(path, strlen(path));
	// set sort weight = file path deepth
	//sortWeight = FILEDB_ROOT_SORT_WEIGHT - pFileDBInfo->u32CurrScanDirDepth;//curDeepth;
	sortWeight = FILEDB_ROOT_SORT_WEIGHT - curDeepth;
	if (pFileDBInfo->defaultfolder[0]) {
		defaultFolderLen = strlen(pFileDBInfo->defaultfolder);
		DBG_IND("path=%s, defaultfolder=%s \r\n", path, pFileDBInfo->defaultfolder);
		if (FileDB_MatchNameUpperWithLen(path, pFileDBInfo->defaultfolder, defaultFolderLen)) {
			sortWeight |= FILEDB_DEFAULT_FOLDER_SORT_WEIGHT;
		}
	}
	return sortWeight;
}



INT32 FileDB_Refresh(FILEDB_HANDLE fileDbHandle)
{
	UINT32             u32OrgCurrIndex;
	FILEDB_SORT_TYPE   orgSortType;
	PFILEDB_INFO       pFileDBInfo;
	INT32              ret = E_OK;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return E_SYS;
	}
	FileDB_lock(fileDbHandle);
	// Keep Original index
	u32OrgCurrIndex = pFileDBInfo->u32CurrentFileIndex;
	pFileDBInfo->u32CurrentFileIndex = 0;
	// Reset some values
	orgSortType = pFileDBInfo->sortType;
	pFileDBInfo->sortType = FILEDB_SORT_BY_NONE;
	memset(&pFileDBInfo->filesNum[0], 0x00, sizeof(pFileDBInfo->filesNum));
	pFileDBInfo->u32TotalFilesNum = 0;
	pFileDBInfo->folderCount = 0;

	if (FileDB_Scan(fileDbHandle) != E_OK) {
		ret = E_SYS;
		goto L_Refresh_Error;
	}
	//  Set current index to original index
	pFileDBInfo->u32CurrentFileIndex = u32OrgCurrIndex;
	if (pFileDBInfo->u32CurrentFileIndex > pFileDBInfo->u32TotalFilesNum - 1) {
		pFileDBInfo->u32CurrentFileIndex = pFileDBInfo->u32TotalFilesNum - 1;
	}
	//  Sorting
	pFileDBInfo->sortType = orgSortType;
	if (pFileDBInfo->sortType != FILEDB_SORT_BY_NONE) {
		FileDB_internalSortBy(fileDbHandle, pFileDBInfo->sortType, pFileDBInfo->bIsSortLargeFirst);
	}
L_Refresh_Error:
	FileDB_unlock(fileDbHandle);
	return ret;
}

static UINT32 FileDB_GetCurDirDeepth(const char *filePath, UINT32 filePathLen)
{
	UINT32 ret, i;

	ret = 0;
	for (i = 0; i < filePathLen; i++) {
		if (*(filePath + i) == '\\') {
			ret++;
		}
	}
	return ret;
}


static void FileDB_FilterOutSameDCFnumFolder(PFILEDB_INFO   pFileDBInfo)
{
	char   tmpStr[4];
	int    FolderId, i, count;
	UINT32 nameOffset;
	PFILEDB_FILE_ATTR  pFileDbFileAttr, pFileDbFileAttr2;

	//count = 0;
	memset(pFileDBInfo->pDCFDirTbl, 0x00, pFileDBInfo->DCFDirTblSize);
	// Get all different DCF Num Folder
	for (i = 0; i < (int)pFileDBInfo->u32TotalFilesNum; i++) {
		pFileDbFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		if (M_IsDirectory(pFileDbFileAttr->attrib)) {
			memcpy(tmpStr, (char *)pFileDbFileAttr->filename, 3);
			tmpStr[3] = '\0';
			FolderId = atoi(tmpStr);
			pFileDBInfo->pDCFDirTbl[FolderId - 100]++;
		}
	}
	// Mark same DCF Num Folder
	memset(pFileDBInfo->pSortIndex, 0x00, pFileDBInfo->SortIndexBufSize);
	for (i = 0; i < (int)pFileDBInfo->u32TotalFilesNum; i++) {
		pFileDbFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		if (M_IsDirectory(pFileDbFileAttr->attrib)) {
			memcpy(tmpStr, (char *)pFileDbFileAttr->filename, 3);
			tmpStr[3] = '\0';
			FolderId = atoi(tmpStr);
			if (pFileDBInfo->pDCFDirTbl[FolderId - 100] > 1 || (!FileDB_IsOurDCFFolderName(pFileDBInfo, pFileDbFileAttr->filename))) {
				*(pFileDBInfo->pSortIndex + i) = 1;
			}
			DBG_IND("Dir name=%s\r\n", pFileDbFileAttr->filename);
		}
	}
	count = 0;
	for (i = 0; i < (int)pFileDBInfo->u32TotalFilesNum; i++) {
		if ((*(pFileDBInfo->pSortIndex + i)) == 1) {
			// mark counter
			count++;
		} else {
			if (count) {
				pFileDbFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)(i - count));
				pFileDbFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
				if (pFileDbFileAttr) {
					memcpy((char *)pFileDbFileAttr, (char *)pFileDbFileAttr2, pFileDBInfo->u32FileItemSize);
					// Re Assign File path & file name
					nameOffset = (UINT32)pFileDbFileAttr->filename - (UINT32)pFileDbFileAttr->filePath;
					pFileDbFileAttr->filePath = (CHAR *)((UINT32)pFileDbFileAttr + sizeof(FILEDB_FILE_ATTR));
					pFileDbFileAttr->filename = &pFileDbFileAttr->filePath[nameOffset];
				}
			}
		}
	}
	pFileDBInfo->u32TotalFilesNum -= (UINT32)count;
	pFileDBInfo->u32CurrentFileIndex = pFileDBInfo->u32TotalFilesNum;
	// reset values
	memset(pFileDBInfo->pSortIndex, 0x00, pFileDBInfo->SortIndexBufSize);

}
static void FileDB_FilterOutSameDCFnumFile(PFILEDB_INFO pFileDBInfo)
{
	char tmpStr[5];
	int  FileId, i, count;
	UINT32 nameOffset;
	PFILEDB_FILE_ATTR  pFileDbFileAttr, pFileDbFileAttr2;

	//count = 0;
	memset(pFileDBInfo->pDCFFileTbl, 0x00, pFileDBInfo->DCFFileTblSize);
	// Get all different DCF Num Folder
	for (i = (int)pFileDBInfo->u32CurDirStartFileIndex; i < (int)pFileDBInfo->u32TotalFilesNum; i++) {
		pFileDbFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		if (!M_IsDirectory(pFileDbFileAttr->attrib)) {
			memcpy(tmpStr, (char *)(pFileDbFileAttr->filename + 4), 4);
			tmpStr[4] = '\0';
			FileId = atoi(tmpStr);
			pFileDBInfo->pDCFFileTbl[FileId - 1]++;
		}

	}
	// Mark same DCF Num Folder
	memset(pFileDBInfo->pSortIndex, 0x00, pFileDBInfo->SortIndexBufSize);
	for (i = (int)pFileDBInfo->u32CurDirStartFileIndex; i < (int)pFileDBInfo->u32TotalFilesNum; i++) {
		pFileDbFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		if (!M_IsDirectory(pFileDbFileAttr->attrib)) {
			memcpy(tmpStr, (char *)(pFileDbFileAttr->filename + 4), 4);
			tmpStr[4] = '\0';
			FileId = atoi(tmpStr);
			if (pFileDBInfo->pDCFFileTbl[FileId - 1] > 1 || (!FileDB_IsOurDCFFileName(pFileDBInfo, pFileDbFileAttr->filename))) {
				*(pFileDBInfo->pSortIndex + i) = 1;
			}
			DBG_IND("File name=%s\r\n", pFileDbFileAttr->filename);
		}
	}
	count = 0;
	for (i = (int)pFileDBInfo->u32CurDirStartFileIndex; i < (int)pFileDBInfo->u32TotalFilesNum; i++) {
		if ((*(pFileDBInfo->pSortIndex + i)) == 1) {
			// mark counter
			count++;
		} else {
			if (count) {
				pFileDbFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)(i - count));
				pFileDbFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
				if (pFileDbFileAttr && pFileDbFileAttr2) {
					memcpy((char *)pFileDbFileAttr, (char *)pFileDbFileAttr2, pFileDBInfo->u32FileItemSize);
					// Re Assign File path & file name
					nameOffset = (UINT32)pFileDbFileAttr->filename - (UINT32)pFileDbFileAttr->filePath;
					pFileDbFileAttr->filePath = (CHAR *)((UINT32)pFileDbFileAttr + sizeof(FILEDB_FILE_ATTR));
					pFileDbFileAttr->filename = &pFileDbFileAttr->filePath[nameOffset];
				}
			}
		}
	}
	pFileDBInfo->u32TotalFilesNum -= (UINT32)count;
	pFileDBInfo->u32CurrentFileIndex = pFileDBInfo->u32TotalFilesNum;
	memset(pFileDBInfo->pSortIndex, 0x00, pFileDBInfo->SortIndexBufSize);

}

static int     FileDB_IsOurDCFFolderName(PFILEDB_INFO pFileDBInfo, const char *pFolderName)
{
	if (pFileDBInfo->OurDCFDirName[0]) {
		return (strncmp((char *)(pFolderName + 3), pFileDBInfo->OurDCFDirName, 5) == 0);
	} else {
		return TRUE;
	}
}


static int     FileDB_IsOurDCFFileName(PFILEDB_INFO pFileDBInfo, const char *pFileName)
{
	if (pFileDBInfo->OurDCFFileName[0]) {
		return  !(strncmp((char *)pFileName, pFileDBInfo->OurDCFFileName, 4) == 0);
	} else {
		return TRUE;
	}
}


#if 0
static INT32 FileDB_QuickSortChoosePivot(PFILEDB_INFO pFileDBInfo, INT32 left, INT32 right)
{
	return (left + RANDOM_RANGE(right - left));
#if 0
	switch (pFileDBInfo->sortType) {
	case FILEDB_SORT_BY_FILEPATH: {

			PFILEDB_FILE_INT_ATTR       pTempFileAttr1, pTempFileAttr2, pTempFileAttr3;
			pTempFileAttr1 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
			pTempFileAttr2 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)(left + right) >> 1);
			pTempFileAttr3 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)right);
			if (pTempFileAttr2->sortWeight > pTempFileAttr1->sortWeight) {
				goto Lab2L1;
			} else if (pTempFileAttr2->sortWeight == pTempFileAttr1->sortWeight) {
				if (FileDB_CompareName(pTempFileAttr2->filePath + pFileDBInfo->u32RootPathLen, pTempFileAttr1->filePath + pFileDBInfo->u32RootPathLen)) {
					goto Lab2L1;
				}
			}
			goto Lab1L2;

Lab2L1:
			if (pTempFileAttr3->sortWeight > pTempFileAttr2->sortWeight) {
				goto Lab3L2L1;
			} else if (pTempFileAttr3->sortWeight == pTempFileAttr2->sortWeight) {
				if (FileDB_CompareName(pTempFileAttr3->filePath + pFileDBInfo->u32RootPathLen, pTempFileAttr2->filePath + pFileDBInfo->u32RootPathLen)) {
					goto Lab3L2L1;
				}
			}
			if (pTempFileAttr1->sortWeight > pTempFileAttr3->sortWeight) {
				goto Lab2L1L3;
			} else if (pTempFileAttr1->sortWeight == pTempFileAttr3->sortWeight) {
				if (FileDB_CompareName(pTempFileAttr1->filePath + pFileDBInfo->u32RootPathLen, pTempFileAttr3->filePath + pFileDBInfo->u32RootPathLen)) {
					goto Lab2L1L3;
				}
			}
			goto Lab2L3L1;

Lab1L2:
			if (pTempFileAttr3->sortWeight > pTempFileAttr1->sortWeight) {
				goto Lab3L1L2;
			} else if (pTempFileAttr3->sortWeight == pTempFileAttr1->sortWeight) {
				if (FileDB_CompareName(pTempFileAttr3->filePath + pFileDBInfo->u32RootPathLen, pTempFileAttr1->filePath + pFileDBInfo->u32RootPathLen)) {
					goto Lab3L1L2;
				}
			}
			if (pTempFileAttr2->sortWeight > pTempFileAttr3->sortWeight) {
				goto Lab1L2L3;
			} else if (pTempFileAttr2->sortWeight == pTempFileAttr3->sortWeight) {
				if (FileDB_CompareName(pTempFileAttr2->filePath + pFileDBInfo->u32RootPathLen, pTempFileAttr3->filePath + pFileDBInfo->u32RootPathLen)) {
					goto Lab1L2L3;
				}
			}
			goto Lab1L3L2;
Lab1L2L3:
Lab3L2L1:
			DBG_DUMP("m");
			return (UINT32)(left + right) >> 1;

Lab2L1L3:
Lab3L1L2:
			DBG_DUMP("l");
			return left;
Lab1L3L2:
Lab2L3L1:
			DBG_DUMP("r");
			return right;

		}

	case FILEDB_SORT_BY_CREDATE:
		return (UINT32)(left + right) >> 1;

	case FILEDB_SORT_BY_MODDATE:
		return (UINT32)(left + right) >> 1;

	case FILEDB_SORT_BY_NAME:
		return (UINT32)(left + right) >> 1;

	case FILEDB_SORT_BY_STROKENUM:
		return (UINT32)(left + right) >> 1;


	case FILEDB_SORT_BY_SIZE:
		return (UINT32)(left + right) >> 1;

	case FILEDB_SORT_BY_FILETYPE:
		return (UINT32)(left + right) >> 1;

	case FILEDB_SORT_BY_TYPE_CRETIME_SIZE:
		return (UINT32)(left + right) >> 1;

	default:
		return (UINT32)(left + right) >> 1;
	}
#endif
}
#endif

static void FileDB_QuickSortByCreDate(PFILEDB_INFO pFileDBInfo, INT32 left, INT32 right)
{
	INT32              j;

	if (SORT_METHOD_INSERT == FileDB_ChooseSortMethod(pFileDBInfo,  left, right)) {
		return FileDB_InsertSortByCreDate(pFileDBInfo,  left, right);
	}
	pFileDBInfo->recursiveFuncCurDeep++;
#if TEST_FUNC_DEEP
	if (pFileDBInfo->recursiveFuncCurDeep > pFileDBInfo->recursiveFuncMaxDeep) {
		pFileDBInfo->recursiveFuncMaxDeep = pFileDBInfo->recursiveFuncCurDeep;
		DBG_DUMP("MaxDeep = %d, left = %d, right = %d, random=%d\r\n", pFileDBInfo->recursiveFuncMaxDeep, left, right, (UINT32)left + RANDOM_RANGE(right - left));
	}
#endif
	if (left < right) {
		{
			INT32                   i;
			UINT32                  tempS;
			PFILEDB_FILE_ATTR       pTempFileAttr;

			// random to choose Pivot ==> left+RANDOM_RANGE(right-left)
			i = left + RANDOM_RANGE(right - left);
			FileDB_ExchangeIndex(pFileDBInfo, left, i);
			pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
			tempS = ((UINT32)pTempFileAttr->creDate << 16) + pTempFileAttr->creTime;
			i = left;
			j = right + 1;
			while (1) {
				// looking from left to right
				while (i < right) {
					i++;
					pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
					if ((((UINT32)pTempFileAttr->creDate << 16) + pTempFileAttr->creTime) > tempS) {
						break;
					}
				}
				// looking from right to left
				while (j  > left) {
					j--;
					pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j);
					if ((((UINT32)pTempFileAttr->creDate << 16) + pTempFileAttr->creTime) < tempS) {
						break;
					}
				}
				if (i >= j) {
					break;
				}
				FileDB_ExchangeIndex(pFileDBInfo, i, j);
			}
		}
		FileDB_ExchangeIndex(pFileDBInfo, left, j);
		if (left < j - 1) {
			FileDB_QuickSortByCreDate(pFileDBInfo, left, j - 1); // Rescursive Left part
		}
		if (j + 1 < right) {
			FileDB_QuickSortByCreDate(pFileDBInfo, j + 1, right); // Rescursive right part
		}
	}
	pFileDBInfo->recursiveFuncCurDeep--;
}

static void FileDB_QuickSortByModDate(PFILEDB_INFO pFileDBInfo, INT32 left, INT32 right)
{
	INT32              j;

	if (SORT_METHOD_INSERT == FileDB_ChooseSortMethod(pFileDBInfo,  left, right)) {
		return FileDB_InsertSortByModDate(pFileDBInfo,  left, right);
	}
	pFileDBInfo->recursiveFuncCurDeep++;
#if TEST_FUNC_DEEP
	if (pFileDBInfo->recursiveFuncCurDeep > pFileDBInfo->recursiveFuncMaxDeep) {
		pFileDBInfo->recursiveFuncMaxDeep = pFileDBInfo->recursiveFuncCurDeep;
		DBG_DUMP("MaxDeep = %d, left = %d, right = %d, random=%d\r\n", pFileDBInfo->recursiveFuncMaxDeep, left, right, (UINT32)left + RANDOM_RANGE(right - left));
	}
#endif
	if (left < right) {
		{
			INT32                   i;
			UINT32                  tempS;
			PFILEDB_FILE_ATTR       pTempFileAttr;

			// random to choose Pivot ==> left+RANDOM_RANGE(right-left)
			i = left + RANDOM_RANGE(right - left);
			FileDB_ExchangeIndex(pFileDBInfo, left, i);
			pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);//s = number[left];
			tempS = ((UINT32)pTempFileAttr->lastWriteDate << 16) + pTempFileAttr->lastWriteTime;
			i = left;
			j = right + 1;
			while (1) {
				// looking from left to right
				while (i < right) {
					i++;
					pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
					if ((((UINT32)pTempFileAttr->lastWriteDate << 16) + pTempFileAttr->lastWriteTime) > tempS) {
						break;
					}
				}
				// looking from right to left
				while (j  > left) {
					j--;
					pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j);
					if ((((UINT32)pTempFileAttr->lastWriteDate << 16) + pTempFileAttr->lastWriteTime) < tempS) {
						break;
					}
				}
				if (i >= j) {
					break;
				}
				FileDB_ExchangeIndex(pFileDBInfo, i, j);
			}
		}
		FileDB_ExchangeIndex(pFileDBInfo, left, j);
		if (left < j - 1) {
			FileDB_QuickSortByModDate(pFileDBInfo, left, j - 1); // Rescursive Left part
		}
		if (j + 1 < right) {
			FileDB_QuickSortByModDate(pFileDBInfo, j + 1, right); // Rescursive right part
		}
	}
	pFileDBInfo->recursiveFuncCurDeep--;
}

static void FileDB_QuickSortBySize(PFILEDB_INFO pFileDBInfo, INT32 left, INT32 right)
{
	INT32              j;

	if (SORT_METHOD_INSERT == FileDB_ChooseSortMethod(pFileDBInfo,  left, right)) {
		return FileDB_InsertSortBySize(pFileDBInfo,  left, right);
	}
	pFileDBInfo->recursiveFuncCurDeep++;
#if TEST_FUNC_DEEP
	if (pFileDBInfo->recursiveFuncCurDeep > pFileDBInfo->recursiveFuncMaxDeep) {
		pFileDBInfo->recursiveFuncMaxDeep = pFileDBInfo->recursiveFuncCurDeep;
		DBG_DUMP("MaxDeep = %d, left = %d, right = %d, random=%d\r\n", pFileDBInfo->recursiveFuncMaxDeep, left, right, (UINT32)left + RANDOM_RANGE(right - left));
	}
#endif
	if (left < right) {
		{
			INT32                   i;
			//#NT#2016/03/29#Nestor Yang -begin
			//#NT# Support fileSize larger than 4GB
			//UINT32                  tempS;
			UINT64                  tempS;
			//#NT#2016/03/29#Nestor Yang -end
			PFILEDB_FILE_ATTR       pTempFileAttr;

			// random to choose Pivot ==> left+RANDOM_RANGE(right-left)
			i = left + RANDOM_RANGE(right - left);
			FileDB_ExchangeIndex(pFileDBInfo, left, i);
			pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
			//#NT#2016/03/29#Nestor Yang -begin
			//#NT# Support fileSize larger than 4GB
			//tempS = pTempFileAttr->fileSize;
			tempS = pTempFileAttr->fileSize64;
			//#NT#2016/03/29#Nestor Yang -end
			i = left;
			j = right + 1;
			while (1) {
				// looking from left to right
				while (i < right) {
					i++;
					pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
					//#NT#2016/03/29#Nestor Yang -begin
					//#NT# Support fileSize larger than 4GB
					//if (pTempFileAttr->fileSize > tempS)
					if (pTempFileAttr->fileSize64 > tempS) {
						break;
					}
					//#NT#2016/03/29#Nestor Yang -end
				}
				// looking from right to left
				while (j  > left) {
					j--;
					pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j);
					//#NT#2016/03/29#Nestor Yang -begin
					//#NT# Support fileSize larger than 4GB
					//if (pTempFileAttr->fileSize < tempS)
					if (pTempFileAttr->fileSize64 < tempS) {
						break;
					}
					//#NT#2016/03/29#Nestor Yang -end
				}
				if (i >= j) {
					break;
				}
				FileDB_ExchangeIndex(pFileDBInfo, i, j);
			}
		}
		FileDB_ExchangeIndex(pFileDBInfo, left, j);
		if (left < j - 1) {
			FileDB_QuickSortBySize(pFileDBInfo, left, j - 1);
		}
		if (j + 1 < right) {
			FileDB_QuickSortBySize(pFileDBInfo, j + 1, right);
		}
	}
	pFileDBInfo->recursiveFuncCurDeep--;
}

static void FileDB_QuickSortByFileType(PFILEDB_INFO pFileDBInfo, INT32 left, INT32 right)
{
	INT32              j;

	if (SORT_METHOD_INSERT == FileDB_ChooseSortMethod(pFileDBInfo,  left, right)) {
		return FileDB_InsertSortByFileType(pFileDBInfo,  left, right);
	}
	pFileDBInfo->recursiveFuncCurDeep++;
#if TEST_FUNC_DEEP
	if (pFileDBInfo->recursiveFuncCurDeep > pFileDBInfo->recursiveFuncMaxDeep) {
		pFileDBInfo->recursiveFuncMaxDeep = pFileDBInfo->recursiveFuncCurDeep;
		DBG_DUMP("MaxDeep = %d, left = %d, right = %d, random=%d\r\n", pFileDBInfo->recursiveFuncMaxDeep, left, right, (UINT32)left + RANDOM_RANGE(right - left));
	}
#endif
	if (left < right) {
		{
			INT32                   i;
			UINT32                  tempS;
			PFILEDB_FILE_ATTR       pTempFileAttr;

			// random to choose Pivot ==> left+RANDOM_RANGE(right-left)
			i = left + RANDOM_RANGE(right - left);
			FileDB_ExchangeIndex(pFileDBInfo, left, i);
			pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
			tempS = pTempFileAttr->fileType;
			i = left;
			j = right + 1;
			while (1) {
				//
				while (i < right) {
					i++;
					pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
					if (pTempFileAttr->fileType > tempS) {
						break;
					}
				}
				//
				while (j  > left) {
					j--;
					pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j);
					if (pTempFileAttr->fileType < tempS) {
						break;
					}
				}
				if (i >= j) {
					break;
				}
				FileDB_ExchangeIndex(pFileDBInfo, i, j);
			}
		}
		FileDB_ExchangeIndex(pFileDBInfo, left, j);
		//Avoid quick sort worst case cause stack overflow problem
		if (left < j - 1) {
			FileDB_QuickSortByFileType(pFileDBInfo, left, j - 1);
		}
		if (j + 1 < right) {
			FileDB_QuickSortByFileType(pFileDBInfo, j + 1, right);
		}
	}
	pFileDBInfo->recursiveFuncCurDeep--;
}

static void FileDB_QuickSortByTypeCretimeSize(PFILEDB_INFO pFileDBInfo, INT32 left, INT32 right)
{
	INT32              j;

	if (SORT_METHOD_INSERT == FileDB_ChooseSortMethod(pFileDBInfo,  left, right)) {
		return FileDB_InsertSortByTypeCretimeSize(pFileDBInfo,  left, right);
	}
	pFileDBInfo->recursiveFuncCurDeep++;
#if TEST_FUNC_DEEP
	if (pFileDBInfo->recursiveFuncCurDeep > pFileDBInfo->recursiveFuncMaxDeep) {
		pFileDBInfo->recursiveFuncMaxDeep = pFileDBInfo->recursiveFuncCurDeep;
		DBG_DUMP("MaxDeep = %d, left = %d, right = %d, random=%d\r\n", pFileDBInfo->recursiveFuncMaxDeep, left, right, (UINT32)left + RANDOM_RANGE(right - left));
	}
#endif
	if (left < right) {
		{
			INT32                   i;
			//#NT#2016/03/29#Nestor Yang -begin
			//#NT# Support fileSize larger than 4GB
			//UINT32                  tempType,tempDateTime,tempSize;
			UINT32                  tempType, tempDateTime;
			UINT64                  tempSize;
			//#NT#2016/03/29#Nestor Yang -end
			PFILEDB_FILE_ATTR       pTempFileAttr;


			// random to choose Pivot ==> left+RANDOM_RANGE(right-left)
#if 0
			if (ismax) {
				i = (UINT32)left + RANDOM_RANGE(right - left);
				pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
				DBG_DUMP("[dump]-- file %04d CreTime=%ld, path=(%s) \r\n", left, (UINT32)(pTempFileAttr->creDate << 16) + pTempFileAttr->creTime, pTempFileAttr->filePath);
				pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
				DBG_DUMP("[dump]-- file %04d CreTime=%ld, path=(%s) \r\n", i, (UINT32)(pTempFileAttr->creDate << 16) + pTempFileAttr->creTime, pTempFileAttr->filePath);
			}
#endif
			i = left + RANDOM_RANGE(right - left);
			FileDB_ExchangeIndex(pFileDBInfo, left, i);
			pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
			tempType = pTempFileAttr->fileType;
			tempDateTime = ((UINT32)pTempFileAttr->creDate << 16) + pTempFileAttr->creTime;
			//#NT#2016/03/29#Nestor Yang -begin
			//#NT# Support fileSize larger than 4GB
			//tempSize = pTempFileAttr->fileSize;
			tempSize = pTempFileAttr->fileSize64;
			//#NT#2016/03/29#Nestor Yang -end
			i = left;
			j = right + 1;
			while (1) {
				//
				while (i < right) {
					i++;
					pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
					if (pTempFileAttr->fileType > tempType) {
						break;
					} else if (pTempFileAttr->fileType == tempType) {
						if ((((UINT32)pTempFileAttr->creDate << 16) + pTempFileAttr->creTime) > tempDateTime) {
							break;
						} else if ((((UINT32)pTempFileAttr->creDate << 16) + pTempFileAttr->creTime) == tempDateTime) {
							//#NT#2016/03/29#Nestor Yang -begin
							//#NT# Support fileSize larger than 4GB
							//if (pTempFileAttr->fileSize > tempSize)
							if (pTempFileAttr->fileSize64 > tempSize)
								//#NT#2016/03/29#Nestor Yang -end
							{
								break;
							}
						}

					}
				}
				//
				while (j  > left) {
					j--;
					pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j);
					if (pTempFileAttr->fileType < tempType) {
						break;
					} else if (pTempFileAttr->fileType == tempType) {
						if ((((UINT32)pTempFileAttr->creDate << 16) + pTempFileAttr->creTime) < tempDateTime) {
							break;
						} else if ((((UINT32)pTempFileAttr->creDate << 16) + pTempFileAttr->creTime) == tempDateTime) {
							//#NT#2016/03/29#Nestor Yang -begin
							//#NT# Support fileSize larger than 4GB
							//if (pTempFileAttr->fileSize < tempSize)
							if (pTempFileAttr->fileSize64 < tempSize)
								//#NT#2016/03/29#Nestor Yang -end
							{
								break;
							}
						}

					}
				}
				if (i >= j) {
					break;
				}
				FileDB_ExchangeIndex(pFileDBInfo, i, j);
			}
		}
		FileDB_ExchangeIndex(pFileDBInfo, left, j);
		if (left < j - 1) {
			FileDB_QuickSortByTypeCretimeSize(pFileDBInfo, left, j - 1);
		}
		if (j + 1 < right) {
			FileDB_QuickSortByTypeCretimeSize(pFileDBInfo, j + 1, right);
		}
	}
	pFileDBInfo->recursiveFuncCurDeep--;
}

static void FileDB_QuickSortByName(PFILEDB_INFO pFileDBInfo, INT32 left, INT32 right)
{
	INT32                   j;

	if (SORT_METHOD_INSERT == FileDB_ChooseSortMethod(pFileDBInfo,  left, right)) {
		return FileDB_InsertSortByName(pFileDBInfo,  left, right);
	}
	pFileDBInfo->recursiveFuncCurDeep++;
#if TEST_FUNC_DEEP
	if (pFileDBInfo->recursiveFuncCurDeep > pFileDBInfo->recursiveFuncMaxDeep) {
		pFileDBInfo->recursiveFuncMaxDeep = pFileDBInfo->recursiveFuncCurDeep;
		DBG_DUMP("MaxDeep = %d, left = %d, right = %d, random=%d\r\n", pFileDBInfo->recursiveFuncMaxDeep, left, right, (UINT32)left + RANDOM_RANGE(right - left));
	}
#endif
	if (left < right) {
		{
			INT32                   i;
			PFILEDB_FILE_ATTR       pTempFileAttr1, pTempFileAttr2;

			// random to choose Pivot ==> left+RANDOM_RANGE(right-left)
			i = left + RANDOM_RANGE(right - left);
			FileDB_ExchangeIndex(pFileDBInfo, left, i);
			pTempFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
			i = left;
			j = right + 1;
			while (1) {
				//
				while (i < right) {
					i++;
					pTempFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
					if (FileDB_CompareName(pTempFileAttr2->filename, pTempFileAttr1->filename)) {
						break;
					}
				}
				//
				while (j  > left) {
					j--;
					pTempFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j);
					if (FileDB_CompareName(pTempFileAttr1->filename, pTempFileAttr2->filename)) {
						break;
					}
				}
				if (i >= j) {
					break;
				}
				FileDB_ExchangeIndex(pFileDBInfo, i, j);
			}
		}
		FileDB_ExchangeIndex(pFileDBInfo, left, j);
		if (left < j - 1) {
			FileDB_QuickSortByName(pFileDBInfo, left, j - 1);
		}
		if (j + 1 < right) {
			FileDB_QuickSortByName(pFileDBInfo, j + 1, right);
		}
	}
	pFileDBInfo->recursiveFuncCurDeep--;
}

static void FileDB_QuickSortByPath(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right)
{
	INT32                   j;

	if (SORT_METHOD_INSERT == FileDB_ChooseSortMethod(pFileDBInfo,  left, right)) {
		return FileDB_InsertSortByPath(pFileDBInfo,  left, right);
	}
	pFileDBInfo->recursiveFuncCurDeep++;
#if TEST_FUNC_DEEP
	if (pFileDBInfo->recursiveFuncCurDeep > pFileDBInfo->recursiveFuncMaxDeep) {
		pFileDBInfo->recursiveFuncMaxDeep = pFileDBInfo->recursiveFuncCurDeep;
		DBG_DUMP("MaxDeep = %d, left = %d, right = %d, random=%d\r\n", pFileDBInfo->recursiveFuncMaxDeep, left, right, (UINT32)left + RANDOM_RANGE(right - left));
	}
#endif

	if (left < right) {
		{
			INT32                       i;
			PFILEDB_FILE_INT_ATTR       pTempFileAttr1, pTempFileAttr2;

			// random to choose Pivot ==> left+RANDOM_RANGE(right-left)
			i = left + RANDOM_RANGE(right - left);
			FileDB_ExchangeIndex(pFileDBInfo, left, i);
			pTempFileAttr1 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
			i = left;
			j = right + 1;
			while (1) {
				//
				while (i < right) {
					i++;
					pTempFileAttr2 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
					if (pTempFileAttr2->sortWeight > pTempFileAttr1->sortWeight) {
						break;
					} else if (pTempFileAttr2->sortWeight == pTempFileAttr1->sortWeight) {
						if (FileDB_CompareName(pTempFileAttr2->filePath + pFileDBInfo->u32RootPathLen, pTempFileAttr1->filePath + pFileDBInfo->u32RootPathLen)) {
							break;
						}
					}
				}
				//
				while (j  > left) {
					j--;
					pTempFileAttr2 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)j);
					if (pTempFileAttr2->sortWeight < pTempFileAttr1->sortWeight) {
						break;
					} else if (pTempFileAttr2->sortWeight == pTempFileAttr1->sortWeight) {
						if (FileDB_CompareName(pTempFileAttr1->filePath + pFileDBInfo->u32RootPathLen, pTempFileAttr2->filePath + pFileDBInfo->u32RootPathLen)) {
							break;
						}
					}
				}
				if (i >= j) {
					break;
				}
				FileDB_ExchangeIndex(pFileDBInfo, i, j);
			}
		}
		FileDB_ExchangeIndex(pFileDBInfo, left, j);
		if (left < j - 1) {
			FileDB_QuickSortByPath(pFileDBInfo, left, j - 1);
		}
		if (j + 1 < right) {
			FileDB_QuickSortByPath(pFileDBInfo, j + 1, right);
		}
	}
	pFileDBInfo->recursiveFuncCurDeep--;
}

static void FileDB_QuickSortBySN(PFILEDB_INFO pFileDBInfo, INT32 left, INT32 right)
{
	INT32              j;

	if (SORT_METHOD_INSERT == FileDB_ChooseSortMethod(pFileDBInfo,  left, right)) {
		return FileDB_InsertSortBySN(pFileDBInfo,  left, right);
	}
	pFileDBInfo->recursiveFuncCurDeep++;

#if TEST_FUNC_DEEP
	if (pFileDBInfo->recursiveFuncCurDeep > pFileDBInfo->recursiveFuncMaxDeep) {
		pFileDBInfo->recursiveFuncMaxDeep = pFileDBInfo->recursiveFuncCurDeep;
		DBG_DUMP("MaxDeep = %d, left = %d, right = %d, random=%d\r\n", pFileDBInfo->recursiveFuncMaxDeep, left, right, (UINT32)left + RANDOM_RANGE(right - left));
	}
#endif

	if (left < right) {
		INT32                   i;
		INT32                   SN;
		PFILEDB_FILE_ATTR       pTempFileAttr;

		// random to choose Pivot ==> left+RANDOM_RANGE(right-left)
		i = left + RANDOM_RANGE(right - left);

		FileDB_ExchangeIndex(pFileDBInfo, left, i);
		pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
		SN = fdb_GetSNByName(pFileDBInfo, pTempFileAttr->filename);

		i = left;
		j = right + 1;
		while (1) {
			// looking from left to right
			while (i < right) {
				i++;
				pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
				if (fdb_GetSNByName(pFileDBInfo, pTempFileAttr->filename) > SN) {
					break;
				}
			}
			// looking from right to left
			while (j  > left) {
				j--;
				pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j);
				if (fdb_GetSNByName(pFileDBInfo, pTempFileAttr->filename) < SN) {
					break;
				}
			}
			if (i >= j) {
				break;
			}
			FileDB_ExchangeIndex(pFileDBInfo, i, j);
		}

		FileDB_ExchangeIndex(pFileDBInfo, left, j);
		if (left < j - 1) {
			FileDB_QuickSortBySN(pFileDBInfo, left, j - 1);
		}
		if (j + 1 < right) {
			FileDB_QuickSortBySN(pFileDBInfo, j + 1, right);
		}
	}
	pFileDBInfo->recursiveFuncCurDeep--;
}

//#Support sort by DCF file path
/**
  Comparing two filename if name1>name2 then return TRUE.

  Comparing two filename if name1>name2 then return TRUE.

  @param char* name1: the name1 need to be compared
  @param char* name2: the name2 need to be compared
  @return return TRUE if name1>name2, else return FALSE
*/
static BOOL FileDB_CompareDCFName(const char *name1, const char *name2)
{
	UINT32              i;
#if 0
	for (i = 0; i < DCF_FILE_PATH_LEN; i++) {
		if (name1[i] > name2[i]) {
			return TRUE;
		} else if (name1[i] < name2[i]) {
			return FALSE;
		}
	}
#else
	for (i = DCF_FOLDER_ID_INDEX; i < DCF_FOLDER_ID_INDEX + DCF_FOLDER_ID_LEN; i++) {
		if (name1[i] > name2[i]) {
			return TRUE;
		} else if (name1[i] < name2[i]) {
			return FALSE;
		}
	}
	for (i = DCF_FILE_ID_INDEX; i < DCF_FILE_ID_INDEX + DCF_FILE_ID_LEN; i++) {
		if (name1[i] > name2[i]) {
			return TRUE;
		} else if (name1[i] < name2[i]) {
			return FALSE;
		}
	}
	for (i = DCF_FILE_ID_OTHER_INDEX; i < DCF_FILE_ID_OTHER_INDEX + DCF_FILE_ID_OTHER_LEN; i++) {
		if (name1[i] > name2[i]) {
			return TRUE;
		} else if (name1[i] < name2[i]) {
			return FALSE;
		}
	}
#endif

	return FALSE;
}

static void FileDB_QuickSortByDCFName(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right)
{

	INT32               j;

	if (SORT_METHOD_INSERT == FileDB_ChooseSortMethod(pFileDBInfo,  left, right)) {
		return FileDB_InsertSortByDCFName(pFileDBInfo,  left, right);
	}
	pFileDBInfo->recursiveFuncCurDeep++;
#if TEST_FUNC_DEEP
	if (pFileDBInfo->recursiveFuncCurDeep > pFileDBInfo->recursiveFuncMaxDeep) {
		pFileDBInfo->recursiveFuncMaxDeep = pFileDBInfo->recursiveFuncCurDeep;
		DBG_DUMP("MaxDeep = %d, left = %d, right = %d, random=%d\r\n", pFileDBInfo->recursiveFuncMaxDeep, left, right, (UINT32)left + RANDOM_RANGE(right - left));
	}
#endif
	if (left < right) {
		{
			INT32               i;
			PFILEDB_FILE_ATTR   pTempFileAttr1, pTempFileAttr2;

			// random to choose Pivot ==> left+RANDOM_RANGE(right-left)
			i = left + RANDOM_RANGE(right - left);
			FileDB_ExchangeIndex(pFileDBInfo, left, i);
			pTempFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
			i = left;
			j = right + 1;
			while (1) {
				//
				while (i < right) {
					i++;
					pTempFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
					if (FileDB_CompareDCFName(pTempFileAttr2->filePath, pTempFileAttr1->filePath)) {
						break;
					}
				}
				//
				while (j  > left) {
					j--;
					pTempFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j);
					if (FileDB_CompareDCFName(pTempFileAttr1->filePath, pTempFileAttr2->filePath)) {
						break;
					}
				}
				if (i >= j) {
					break;
				}
				FileDB_ExchangeIndex(pFileDBInfo, i, j);
			}
		}
		FileDB_ExchangeIndex(pFileDBInfo, left, j);
		if (left < j - 1) {
			FileDB_QuickSortByDCFName(pFileDBInfo, left, j - 1);
		}
		if (j + 1 < right) {
			FileDB_QuickSortByDCFName(pFileDBInfo, j + 1, right);
		}
	}
	pFileDBInfo->recursiveFuncCurDeep--;
}

static void FileDB_QuickSortByCretimeName(PFILEDB_INFO pFileDBInfo, INT32 left, INT32 right)
{
	INT32              j;
	if (SORT_METHOD_INSERT == FileDB_ChooseSortMethod(pFileDBInfo, left, right)) {
		return FileDB_InsertSortByCretimeName(pFileDBInfo, left, right);
	}
	pFileDBInfo->recursiveFuncCurDeep++;
#if TEST_FUNC_DEEP
	if (pFileDBInfo->recursiveFuncCurDeep > pFileDBInfo->recursiveFuncMaxDeep) {
		pFileDBInfo->recursiveFuncMaxDeep = pFileDBInfo->recursiveFuncCurDeep;
		DBG_DUMP("MaxDeep = %d, left = %d, right = %d, random=%d\r\n", pFileDBInfo->recursiveFuncMaxDeep, left, right, (UINT32)left + RANDOM_RANGE(right - left));
	}
#endif
	if(left < right) {
		{
			INT32               i;
			UINT32              temp1, temp2;
			PFILEDB_FILE_ATTR   pTempFileAttr1, pTempFileAttr2;

			// random to choose Pivot ==> left+RANDOM_RANGE(right-left)
			i = left + RANDOM_RANGE(right - left);
			FileDB_ExchangeIndex(pFileDBInfo, left, i);
			pTempFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
			temp1 = ((UINT32)pTempFileAttr1->creDate <<16) + pTempFileAttr1->creTime;
			i = left;
			j = right + 1;
			while(1) {
				// looking from left to right
				while (i < right) {
					i++;
					pTempFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
					temp2 = ((UINT32)pTempFileAttr2->creDate <<16) + pTempFileAttr2->creTime;
					if (temp2 > temp1) {
						break;
					} else if (temp2 == temp1) {
						if (pFileDBInfo->bIsDCFFileOnly) {
							if (FileDB_CompareDCFName(pTempFileAttr2->filename, pTempFileAttr1->filename)) {
								break;
							}
						} else {
							if (FileDB_CompareName(pTempFileAttr2->filename, pTempFileAttr1->filename)) {
								break;
							}
						}
					}
				}
				// looking from right to left
				while(j  > left) {
					j--;
					pTempFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j);
					temp2 = ((UINT32)pTempFileAttr2->creDate <<16) + pTempFileAttr2->creTime;
					if (temp2 < temp1) {
						break;
					} else if (temp2 == temp1) {
						if (pFileDBInfo->bIsDCFFileOnly) {
							if (FileDB_CompareDCFName(pTempFileAttr1->filename, pTempFileAttr2->filename)) {
								break;
							}
						} else {
							if (FileDB_CompareName(pTempFileAttr1->filename, pTempFileAttr2->filename)) {
								break;
							}
						}
					}
				}
				if(i >= j) {
					break;
				}
				FileDB_ExchangeIndex(pFileDBInfo, i, j);
			}
		}
		FileDB_ExchangeIndex(pFileDBInfo, left, j);
		if (left < j - 1) {
			FileDB_QuickSortByCretimeName(pFileDBInfo, left, j - 1);   // Rescursive Left part
		}
		if (j + 1 < right) {
			FileDB_QuickSortByCretimeName(pFileDBInfo, j + 1, right);  // Rescursive right part
		}
	}
	pFileDBInfo->recursiveFuncCurDeep--;
}

#if (_SUPPORT_LONG_FILENAME_IN_FILEDB_)
static BOOL FileDB_CmpLongNameByStokeNum(UINT16 *name1, UINT16 *name2)
{
	UINT16 i = 0;
	UINT32 u32SortSeq1, u32SortSeq2;
	UINT8  highByte, lowByte;

	while (i <= FILEDB_MAX_LONG_FILENAME_LEN) {
		if (name1[i] == 0x0000 || name2[i] == 0x0000) {
			return FALSE;
		}

		highByte = ((name1[i] & 0xFF00) >> 8);
		lowByte = (name1[i] & 0x00FF);
		if (highByte > 0) {
			u32SortSeq1 = (((0x01FF & Big5GetStrokeNum(ConvertUnicodeToBig5(name1[i]))) << 16) | name1[i]);
		} else {
			u32SortSeq1 = AsciiCodeSortSeq[lowByte];
		}

		highByte = ((name2[i] & 0xFF00) >> 8);
		lowByte = (name2[i] & 0x00FF);
		if (highByte > 0) {
			u32SortSeq2 = (((0x01FF & Big5GetStrokeNum(ConvertUnicodeToBig5(name2[i]))) << 16) | name2[i]);
		} else {
			u32SortSeq2 = AsciiCodeSortSeq[lowByte];
		}

		if (u32SortSeq1 > u32SortSeq2) {
			return TRUE;
		} else if (u32SortSeq1 < u32SortSeq2) {
			return FALSE;
		}
		i++;
	}
	return FALSE;
}

static void FileDB_QuickSortByStrokeNum(PFILEDB_INFO   pFileDBInfo, INT32 left, INT32 right)
{
	INT32              j;

	if (left < right) {
		{
			INT32               i;
			PFILEDB_FILE_ATTR   pTempFileAttr;

			// random to choose Pivot ==> left+RANDOM_RANGE(right-left)
			i = RANDOM_RANGE(right - left);
			FileDB_ExchangeIndex(pFileDBInfo, left, i);
			pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, left);
			memcpy(g_TempLongname, pTempFileAttr->longName, (FILEDB_MAX_LONG_FILENAME_LEN << 1));
			i = left;
			j = right + 1;
			while (1) {
				//
				while (i < right) {
					i++;
					pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, i);
					if (FileDB_CmpLongNameByStokeNum(pTempFileAttr->longName, g_TempLongname)) {
						break;
					}
				}
				//
				while (j  > left) {
					j--;
					pTempFileAttr = FileDB_GetFileAttr(pFileDBInfo, j);
					if (FileDB_CmpLongNameByStokeNum(g_TempLongname, pTempFileAttr->longName)) {
						break;
					}
				}
				if (i >= j) {
					break;
				}
				FileDB_ExchangeIndex(pFileDBInfo, i, j);
			}
		}
		FileDB_ExchangeIndex(pFileDBInfo, left, j);
		if (left < j - 1) {
			FileDB_QuickSortByStrokeNum(left, j - 1);
		}
		if (j + 1 < right) {
			FileDB_QuickSortByStrokeNum(j + 1, right);
		}
	}
}
#endif



static void FileDB_InsertOneByCreDate(PFILEDB_INFO  pFileDBInfo,  INT32 Index)
{
	INT32                 i;
	BOOL                  isNeedChange = FALSE;
	PFILEDB_FILE_INT_ATTR pFileDbFileAttr1, pFileDbFileAttr2;

	pFileDbFileAttr1 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)Index);

	//  find the index to insert
	for (i = Index - 1; i >= 0; i--) {
		pFileDbFileAttr2 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		if (!pFileDBInfo->bIsSortLargeFirst) {
			if ((((UINT32)pFileDbFileAttr1->creDate << 16) + pFileDbFileAttr1->creTime) > (((UINT32)pFileDbFileAttr2->creDate << 16) + pFileDbFileAttr2->creTime)) {
				break;
			}
		} else {
			if ((((UINT32)pFileDbFileAttr2->creDate << 16) + pFileDbFileAttr2->creTime) > (((UINT32)pFileDbFileAttr1->creDate << 16) + pFileDbFileAttr1->creTime)) {
				break;
			}
		}
		*(pFileDBInfo->pSortIndex + i + 1) = *(pFileDBInfo->pSortIndex + i);
		isNeedChange = TRUE;
	}
	// the one should insert to index i
	if (isNeedChange) {
		*(pFileDBInfo->pSortIndex + i + 1) = Index;
	}
}

static void FileDB_InsertOneByModDate(PFILEDB_INFO  pFileDBInfo,  INT32 Index)
{
	INT32                 i;
	BOOL                  isNeedChange = FALSE;
	PFILEDB_FILE_INT_ATTR pFileDbFileAttr1, pFileDbFileAttr2;

	pFileDbFileAttr1 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)Index);

	//  find the index to insert
	for (i = Index - 1; i >= 0; i--) {
		pFileDbFileAttr2 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		if (!pFileDBInfo->bIsSortLargeFirst) {
			if ((((UINT32)pFileDbFileAttr1->lastWriteDate << 16) + pFileDbFileAttr1->lastWriteTime) > (((UINT32)pFileDbFileAttr2->lastWriteDate << 16) + pFileDbFileAttr2->lastWriteTime)) {
				break;
			}
		} else {
			if ((((UINT32)pFileDbFileAttr2->lastWriteDate << 16) + pFileDbFileAttr2->lastWriteTime) > (((UINT32)pFileDbFileAttr1->lastWriteDate << 16) + pFileDbFileAttr1->lastWriteTime)) {
				break;
			}
		}
		*(pFileDBInfo->pSortIndex + i + 1) = *(pFileDBInfo->pSortIndex + i);
		isNeedChange = TRUE;
	}
	// the one should insert to index i
	if (isNeedChange) {
		*(pFileDBInfo->pSortIndex + i + 1) = Index;
	}
}

static void FileDB_InsertOneBySize(PFILEDB_INFO  pFileDBInfo,  INT32 Index)
{
	INT32                 i;
	BOOL                  isNeedChange = FALSE;
	PFILEDB_FILE_INT_ATTR pFileDbFileAttr1, pFileDbFileAttr2;

	pFileDbFileAttr1 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)Index);

	//  find the index to insert
	for (i = Index - 1; i >= 0; i--) {
		pFileDbFileAttr2 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		if (!pFileDBInfo->bIsSortLargeFirst) {
			//#NT#2016/03/30#Nestor Yang -begin
			//#NT# Support fileSize larger than 4GB
			//if (pFileDbFileAttr1->fileSize > pFileDbFileAttr2->fileSize)
			if (pFileDbFileAttr1->fileSize64 > pFileDbFileAttr2->fileSize64) {
				break;
			}
			//#NT#2016/03/30#Nestor Yang -end
		} else {
			//#NT#2016/03/30#Nestor Yang -begin
			//#NT# Support fileSize larger than 4GB
			//if (pFileDbFileAttr2->fileSize > pFileDbFileAttr1->fileSize)
			if (pFileDbFileAttr2->fileSize64 > pFileDbFileAttr1->fileSize64) {
				break;
			}
			//#NT#2016/03/30#Nestor Yang -end
		}
		*(pFileDBInfo->pSortIndex + i + 1) = *(pFileDBInfo->pSortIndex + i);
		isNeedChange = TRUE;
	}
	// the one should insert to index i
	if (isNeedChange) {
		*(pFileDBInfo->pSortIndex + i + 1) = Index;
	}
}

static void FileDB_InsertOneByFileType(PFILEDB_INFO  pFileDBInfo,  INT32 Index)
{
	INT32                 i;
	BOOL                  isNeedChange = FALSE;
	PFILEDB_FILE_INT_ATTR pFileDbFileAttr1, pFileDbFileAttr2;

	pFileDbFileAttr1 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)Index);

	//  find the index to insert
	for (i = Index - 1; i >= 0; i--) {
		pFileDbFileAttr2 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		if (!pFileDBInfo->bIsSortLargeFirst) {
			if (pFileDbFileAttr1->fileType > pFileDbFileAttr2->fileType) {
				break;
			}
		} else {
			if (pFileDbFileAttr2->fileType > pFileDbFileAttr1->fileType) {
				break;
			}
		}
		*(pFileDBInfo->pSortIndex + i + 1) = *(pFileDBInfo->pSortIndex + i);
		isNeedChange = TRUE;
	}
	// the one should insert to index i
	if (isNeedChange) {
		*(pFileDBInfo->pSortIndex + i + 1) = Index;
	}
}

static void FileDB_InsertOneByTypeCretimeSize(PFILEDB_INFO  pFileDBInfo,  INT32 Index)
{
	INT32                 i;
	BOOL                  isNeedChange = FALSE;
	PFILEDB_FILE_INT_ATTR pFileDbFileAttr1, pFileDbFileAttr2;
	UINT32                u32DateTime1, u32DateTime2;

	pFileDbFileAttr1 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)Index);
	u32DateTime1 = ((UINT32)pFileDbFileAttr1->creDate << 16) + pFileDbFileAttr1->creTime;

	//  find the index to insert
	for (i = Index - 1; i >= 0; i--) {
		pFileDbFileAttr2 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		if (!pFileDBInfo->bIsSortLargeFirst) {
			if (pFileDbFileAttr1->fileType > pFileDbFileAttr2->fileType) {
				break;
			} else if (pFileDbFileAttr1->fileType == pFileDbFileAttr2->fileType) {
				u32DateTime2 = ((UINT32)pFileDbFileAttr2->creDate << 16) + pFileDbFileAttr2->creTime;

				if (u32DateTime1 > u32DateTime2) {
					break;
				} else if (u32DateTime1 == u32DateTime2) {
					//#NT#2016/03/30#Nestor Yang -begin
					//#NT# Support fileSize larger than 4GB
					//if (pFileDbFileAttr1->fileSize >= pFileDbFileAttr2->fileSize)
					if (pFileDbFileAttr1->fileSize64 >= pFileDbFileAttr2->fileSize64)
						//#NT#2016/03/30#Nestor Yang -end
					{
						break;
					}
				}
			}
		} else {
			if (pFileDbFileAttr2->fileType > pFileDbFileAttr1->fileType) {
				break;
			} else if (pFileDbFileAttr2->fileType == pFileDbFileAttr1->fileType) {
				u32DateTime2 = ((UINT32)pFileDbFileAttr2->creDate << 16) + pFileDbFileAttr2->creTime;

				if (u32DateTime2 > u32DateTime1) {
					break;
				} else if (u32DateTime2 == u32DateTime1) {
					//#NT#2016/03/30#Nestor Yang -begin
					//#NT# Support fileSize larger than 4GB
					//if (pFileDbFileAttr2->fileSize >= pFileDbFileAttr1->fileSize)
					if (pFileDbFileAttr2->fileSize64 >= pFileDbFileAttr1->fileSize64)
						//#NT#2016/03/30#Nestor Yang -end
					{
						break;
					}
				}
			}
		}
		*(pFileDBInfo->pSortIndex + i + 1) = *(pFileDBInfo->pSortIndex + i);
		isNeedChange = TRUE;
	}
	// the one should insert to index i
	if (isNeedChange) {
		*(pFileDBInfo->pSortIndex + i + 1) = Index;
	}
}

static void FileDB_InsertOneByCretimeName(PFILEDB_INFO  pFileDBInfo,  INT32 Index)
{
	INT32                 i;
	BOOL                  isNeedChange = FALSE;
	PFILEDB_FILE_INT_ATTR pFileDbFileAttr1, pFileDbFileAttr2;

	pFileDbFileAttr1 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)Index);

	//  find the index to insert
	for(i = Index-1; i >= 0; i--) {
		pFileDbFileAttr2 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		if (!pFileDBInfo->bIsSortLargeFirst) {
			if ((((UINT32)pFileDbFileAttr1->creDate <<16) + pFileDbFileAttr1->creTime) > (((UINT32)pFileDbFileAttr2->creDate <<16) + pFileDbFileAttr2->creTime)) {
				break;
			} else if ((((UINT32)pFileDbFileAttr1->creDate <<16) + pFileDbFileAttr1->creTime) == (((UINT32)pFileDbFileAttr2->creDate <<16) + pFileDbFileAttr2->creTime)) {
				if (pFileDBInfo->bIsDCFFileOnly) {
					if (FileDB_CompareDCFName(pFileDbFileAttr1->filename, pFileDbFileAttr2->filename)) {
						break;
					}
				} else {
					if (FileDB_CompareName(pFileDbFileAttr1->filename, pFileDbFileAttr2->filename)) {
						break;
					}
				}
			}
		} else {
			if ((((UINT32)pFileDbFileAttr2->creDate <<16) + pFileDbFileAttr2->creTime) > (((UINT32)pFileDbFileAttr1->creDate <<16) + pFileDbFileAttr1->creTime)) {
				break;
			} else if ((((UINT32)pFileDbFileAttr2->creDate <<16) + pFileDbFileAttr2->creTime) == (((UINT32)pFileDbFileAttr1->creDate <<16) + pFileDbFileAttr1->creTime)) {
				if (pFileDBInfo->bIsDCFFileOnly) {
					if (FileDB_CompareDCFName(pFileDbFileAttr2->filename, pFileDbFileAttr1->filename)) {
						break;
					}
				} else {
					if (FileDB_CompareName(pFileDbFileAttr2->filename, pFileDbFileAttr1->filename)) {
						break;
					}
				}
			}
		}
		*(pFileDBInfo->pSortIndex + i + 1) = *(pFileDBInfo->pSortIndex + i);
		isNeedChange = TRUE;
	}
	// the one should insert to index i
	if (isNeedChange) {
		*(pFileDBInfo->pSortIndex + i + 1) = Index;
	}
}

static void FileDB_InsertOneByName(PFILEDB_INFO  pFileDBInfo,  INT32 Index)
{
	INT32                 i;
	BOOL                  isNeedChange = FALSE;
	PFILEDB_FILE_INT_ATTR pFileDbFileAttr1, pFileDbFileAttr2;

	pFileDbFileAttr1 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)Index);

	//  find the index to insert
	for (i = Index - 1; i >= 0; i--) {
		pFileDbFileAttr2 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		if (!pFileDBInfo->bIsSortLargeFirst) {
			if (FileDB_CompareName(pFileDbFileAttr1->filename, pFileDbFileAttr2->filename)) {
				break;
			}
		} else {
			if (FileDB_CompareName(pFileDbFileAttr2->filename, pFileDbFileAttr1->filename)) {
				break;
			}
		}
		*(pFileDBInfo->pSortIndex + i + 1) = *(pFileDBInfo->pSortIndex + i);
		isNeedChange = TRUE;
	}
	// the one should insert to index i
	if (isNeedChange) {
		*(pFileDBInfo->pSortIndex + i + 1) = Index;
	}
}

static void FileDB_InsertOneByDCFName(PFILEDB_INFO  pFileDBInfo,  INT32 Index)
{
	INT32                 i;
	BOOL                  isNeedChange = FALSE;
	PFILEDB_FILE_INT_ATTR pFileDbFileAttr1, pFileDbFileAttr2;

	pFileDbFileAttr1 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)Index);

	//  find the index to insert
	for (i = Index - 1; i >= 0; i--) {
		pFileDbFileAttr2 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		if (!pFileDBInfo->bIsSortLargeFirst) {
			if (FileDB_CompareDCFName(pFileDbFileAttr1->filePath, pFileDbFileAttr2->filePath)) {
				break;
			}
		} else {
			if (FileDB_CompareDCFName(pFileDbFileAttr2->filePath, pFileDbFileAttr1->filePath)) {
				break;
			}
		}
		*(pFileDBInfo->pSortIndex + i + 1) = *(pFileDBInfo->pSortIndex + i);
		isNeedChange = TRUE;
	}
	// the one should insert to index i
	if (isNeedChange) {
		*(pFileDBInfo->pSortIndex + i + 1) = Index;
	}
}

static void FileDB_InsertOneByPath(PFILEDB_INFO  pFileDBInfo,  INT32 Index)
{
	INT32                 i;
	BOOL                  isNeedChange = FALSE;
	PFILEDB_FILE_INT_ATTR pFileDbFileAttr1, pFileDbFileAttr2;

	pFileDbFileAttr1 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)Index);

	//  find the index to insert
	for (i = Index - 1; i >= 0; i--) {
		pFileDbFileAttr2 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		if (!pFileDBInfo->bIsSortLargeFirst) {
			if (pFileDbFileAttr1->sortWeight > pFileDbFileAttr2->sortWeight) {
				break;
			} else if (pFileDbFileAttr1->sortWeight == pFileDbFileAttr2->sortWeight) {
				if (FileDB_CompareName(pFileDbFileAttr1->filePath + FILEDB_ROOT_PATH_LEN, pFileDbFileAttr2->filePath + FILEDB_ROOT_PATH_LEN)) {
					break;
				}
			}
		} else {
			if (pFileDbFileAttr2->sortWeight > pFileDbFileAttr1->sortWeight) {
				break;
			} else if (pFileDbFileAttr2->sortWeight == pFileDbFileAttr1->sortWeight) {
				if (FileDB_CompareName(pFileDbFileAttr2->filePath + FILEDB_ROOT_PATH_LEN, pFileDbFileAttr1->filePath + FILEDB_ROOT_PATH_LEN)) {
					break;
				}
			}
		}
		*(pFileDBInfo->pSortIndex + i + 1) = *(pFileDBInfo->pSortIndex + i);
		isNeedChange = TRUE;
	}
	// the one should insert to index i
	if (isNeedChange) {
		*(pFileDBInfo->pSortIndex + i + 1) = Index;
	}
}

static void FileDB_InsertOneBySN(PFILEDB_INFO pFileDBInfo, INT32 Index)
{
	INT32                 i;
	BOOL                  isNeedChange = FALSE;
	PFILEDB_FILE_INT_ATTR pFileDbFileAttr1, pFileDbFileAttr2;
	INT32 SN1, SN2;

	pFileDbFileAttr1 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)Index);
	SN1 = fdb_GetSNByName(pFileDBInfo, pFileDbFileAttr1->filename);

	//  find the index to insert
	for (i = Index - 1; i >= 0; i--) {
		pFileDbFileAttr2 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		SN2 = fdb_GetSNByName(pFileDBInfo, pFileDbFileAttr2->filename);

		if (!pFileDBInfo->bIsSortLargeFirst) {
			if (SN1 > SN2) {
				break;
			}
		} else {
			if (SN2 > SN1) {
				break;
			}
		}
		*(pFileDBInfo->pSortIndex + i + 1) = *(pFileDBInfo->pSortIndex + i);
		isNeedChange = TRUE;
	}
	// the one should insert to index i
	if (isNeedChange) {
		*(pFileDBInfo->pSortIndex + i + 1) = Index;
	}
}

static void FileDB_InsertOneFile(FILEDB_HANDLE fileDbHandle,  INT32 Index)
{
	PFILEDB_INFO   pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return;
	}

	switch (pFileDBInfo->sortType) {
	case FILEDB_SORT_BY_CREDATE:
		FileDB_InsertOneByCreDate(pFileDBInfo, Index);
		break;
	case FILEDB_SORT_BY_MODDATE:
		FileDB_InsertOneByModDate(pFileDBInfo, Index);
		break;

	case FILEDB_SORT_BY_NAME:
		if (pFileDBInfo->bIsDCFFileOnly) {
			FileDB_InsertOneByDCFName(pFileDBInfo, Index);
		} else {
			FileDB_InsertOneByName(pFileDBInfo, Index);
		}
		break;
	case FILEDB_SORT_BY_FILEPATH:
		FileDB_InsertOneByPath(pFileDBInfo, Index);
		break;
	case FILEDB_SORT_BY_SIZE:
		FileDB_InsertOneBySize(pFileDBInfo, Index);
		break;
	case FILEDB_SORT_BY_FILETYPE:
		FileDB_InsertOneByFileType(pFileDBInfo, Index);
		break;
	case FILEDB_SORT_BY_TYPE_CRETIME_SIZE:
		FileDB_InsertOneByTypeCretimeSize(pFileDBInfo, Index);
		break;
	case FILEDB_SORT_BY_CRETIME_NAME:
		FileDB_InsertOneByCretimeName(pFileDBInfo, Index);
		break;
	case FILEDB_SORT_BY_SN:
		FileDB_InsertOneBySN(pFileDBInfo, Index);
		break;

	default:
		DBG_ERR("sortType=%d\r\n", pFileDBInfo->sortType);
	}
}


static BOOL FileDB_internalAddFile(FILEDB_HANDLE fileDbHandle, CHAR *filePath, BOOL isSort)
{
	PFILEDB_INFO          pFileDBInfo;
	PFILEDB_FILE_INT_ATTR pFileDbFileAttr = NULL;
	CHAR                  ExtName[FILE_EXT_LEN + 1];
	UINT32                u32CurrIndex, filePathlen, offset, fileType;
	FST_FILE_STATUS       FileStat = {0};
	FST_FILE              filehdl;
	BOOL                  isFileExist = FALSE;
	INT32                 ret;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return FALSE;
	}
	if (!filePath) {
		DBG_ERR("filePath is NULL\r\n");
		return FALSE;
	}
	if (pFileDBInfo->u32TotalFilesNum >= pFileDBInfo->u32MaxFileNum) {
		DBG_ERR("Total=%d,MaxNum=%d \r\n", pFileDBInfo->u32TotalFilesNum, pFileDBInfo->u32MaxFileNum);
		return FALSE;
	}
	// check if root path match
	if (!FileDB_MatchNameUpperWithLen(pFileDBInfo->rootPath, filePath, strlen(pFileDBInfo->rootPath))) {
		DBG_ERR("root = %s, filePath =%s\r\n", pFileDBInfo->rootPath, filePath);
		return FALSE;
	}

	if (pFileDBInfo->sortType == FILEDB_SORT_BY_FILEPATH && isSort == FALSE) {
		//this combination will cause the sort result disorderly
		DBG_ERR("SORT_BY_FILEPATH can not combine with AddToLast\r\n");
		return FALSE;
	}

	// check if extension name math
	memset(ExtName, 0x00, sizeof(ExtName));
	filePathlen = strlen(filePath);
	offset = FileDB_FindExtNameOffset(filePath, filePathlen);
	strncpy(ExtName, &filePath[offset], FILE_EXT_LEN);
	fileType = FileDB_GetFileType(ExtName);
	if (!fileType) {
		DBG_ERR("filePath =%s\r\n", filePath);
		return FALSE;
	}
	filehdl = FileSys_OpenFile(filePath, FST_OPEN_READ);
	if (filehdl != NULL) {
		ret = FileSys_StatFile(filehdl, &FileStat);
		if (FST_STA_OK != ret) {
			DBG_ERR("stat %s fail\r\n", filePath);
		}
		ret = FileSys_CloseFile(filehdl);
		if (FST_STA_OK != ret) {
			DBG_ERR("close %s fail\r\n", filePath);
		}
		isFileExist = TRUE;
	}
	FileDB_lock(fileDbHandle);
	// add file count
	pFileDBInfo->u32TotalFilesNum++;
	// move index to last file
	u32CurrIndex = pFileDBInfo->u32TotalFilesNum - 1;
	pFileDBInfo->u32CurrentFileIndex = u32CurrIndex;
	// set sort index
	*(pFileDBInfo->pSortIndex + u32CurrIndex) = u32CurrIndex;
	pFileDbFileAttr = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, u32CurrIndex);

	// Clear the memory
	memset(pFileDbFileAttr, 0x00, pFileDBInfo->u32FileItemSize);
	// file path
	pFileDbFileAttr->filePath = (CHAR *)((UINT32)pFileDbFileAttr + sizeof(FILEDB_FILE_ATTR));
	strncpy(pFileDbFileAttr->filePath, filePath, pFileDBInfo->u32MaxFilePathLen - 1);
	// file name
	offset = FileDB_FindFileNameOffset(filePath, filePathlen);
	pFileDbFileAttr->filename = &pFileDbFileAttr->filePath[offset];
	// file attribute
	pFileDbFileAttr->attrib = FileStat.uiAttrib;
	pFileDbFileAttr->fileSize64 = FileStat.uiFileSize;
	// file type
	pFileDbFileAttr->fileType = fileType;
	// Add file count for different file type
	FileDB_AddFilesNum(pFileDBInfo, pFileDbFileAttr->fileType);
	// file created datetime
	pFileDbFileAttr->creDate = FileStat.uiCreatedDate;
	pFileDbFileAttr->creTime = FileStat.uiCreatedTime;
	// file modify datetime
	pFileDbFileAttr->lastWriteDate = FileStat.uiModifiedDate;
	pFileDbFileAttr->lastWriteTime = FileStat.uiModifiedTime;
	// set file sort weight
	pFileDbFileAttr->sortWeight = FileDB_CalcSortWeight(pFileDBInfo, pFileDbFileAttr->filePath);
	// if sort
	if (isSort && ((pFileDBInfo->sortType == FILEDB_SORT_BY_FILEPATH) || (isFileExist && (pFileDBInfo->sortType != FILEDB_SORT_BY_NONE)))) {
		FileDB_InsertOneFile(fileDbHandle, u32CurrIndex);
	}
	FileDB_unlock(fileDbHandle);
	return TRUE;
}

BOOL FileDB_AddFile(FILEDB_HANDLE fileDbHandle, CHAR *filePath)
{
	return FileDB_internalAddFile(fileDbHandle, filePath, TRUE);
}
BOOL FileDB_AddFileToLast(FILEDB_HANDLE fileDbHandle, CHAR *filePath)
{
	return FileDB_internalAddFile(fileDbHandle, filePath, FALSE);
}

BOOL FileDB_DeleteFile(FILEDB_HANDLE fileDbHandle, UINT32 u32FileIndex)
{
	PFILEDB_FILE_ATTR     pFileDbFileAttr;//,pFileDbFileAttr2;
	UINT32                u32PhysicalIndex, i;
	UINT32                nameOffset, MoveFileCount;
	PFILEDB_INFO          pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return FALSE;
	}
	if (u32FileIndex >= pFileDBInfo->u32TotalFilesNum) {
		DBG_ERR("FileIndex=%d, TotalFilesNum=%d\r\n", u32FileIndex, pFileDBInfo->u32TotalFilesNum);
		return FALSE;
	}
	FileDB_lock(fileDbHandle);
	u32PhysicalIndex = *(pFileDBInfo->pSortIndex + u32FileIndex);
	// Re Arrange physical data
	pFileDbFileAttr = FileDB_GetFileAttr(pFileDBInfo, u32FileIndex);
	if (pFileDbFileAttr) {

		MoveFileCount = pFileDBInfo->u32TotalFilesNum - u32PhysicalIndex;

#if 1
		memmove((void *)pFileDbFileAttr, (void *)((UINT32)pFileDbFileAttr + pFileDBInfo->u32FileItemSize), pFileDBInfo->u32FileItemSize * MoveFileCount);
#else
		if (pFileDBInfo->u32FileItemSize <= 64 || (pFileDBInfo->u32FileItemSize * MoveFileCount < 8000)) {
			memmove((void *)pFileDbFileAttr, (void *)((UINT32)pFileDbFileAttr + pFileDBInfo->u32FileItemSize), pFileDBInfo->u32FileItemSize * MoveFileCount);
		} else {
			hwmem_open();
			hwmem_memcpy((void *)pFileDbFileAttr, (void *)((UINT32)pFileDbFileAttr + pFileDBInfo->u32FileItemSize), pFileDBInfo->u32FileItemSize * MoveFileCount);
			hwmem_close();
		}
#endif
	}
	// Re Arrange sorting index
	for (i = u32FileIndex; i < pFileDBInfo->u32TotalFilesNum - 1; i++) {
		*(pFileDBInfo->pSortIndex + i) = *(pFileDBInfo->pSortIndex + i + 1);
	}
	*(pFileDBInfo->pSortIndex + pFileDBInfo->u32TotalFilesNum - 1) = 0;
	pFileDBInfo->u32TotalFilesNum --;
	for (i = 0; i < pFileDBInfo->u32TotalFilesNum; i++) {
		if (*(pFileDBInfo->pSortIndex + i) > u32PhysicalIndex) {
			// Re assing index
			(*(pFileDBInfo->pSortIndex + i))--;
			// Re Assign File path & file name
			pFileDbFileAttr = FileDB_GetFileAttr(pFileDBInfo, i);
			nameOffset = (UINT32)pFileDbFileAttr->filename - (UINT32)pFileDbFileAttr->filePath;
			pFileDbFileAttr->filePath = (CHAR *)((UINT32)pFileDbFileAttr + sizeof(FILEDB_FILE_ATTR));
			pFileDbFileAttr->filename = &pFileDbFileAttr->filePath[nameOffset];
		}

	}
	if (pFileDBInfo->u32CurrentFileIndex >= pFileDBInfo->u32TotalFilesNum - 1) {
		pFileDBInfo->u32CurrentFileIndex = pFileDBInfo->u32TotalFilesNum - 1;
	}
	FileDB_unlock(fileDbHandle);
	return TRUE;

}

static UINT32 FileDB_ChooseSortMethod(PFILEDB_INFO  pFileDBInfo,  INT32 left, INT32 right)
{
	INT32                i;
	PFILEDB_FILE_ATTR    pFileDbFileAttr1, pFileDbFileAttr2;

	if (pFileDBInfo->recursiveFuncCurDeep >= QUICK_SORT_RECURSIVE_FUNC_DEEP_MAX) {
		DBG_WRN("recursiveFuncCurDeep %d Over limit\r\n", pFileDBInfo->recursiveFuncCurDeep);
		return SORT_METHOD_INSERT;

	} else if (right -  left > INSERT_SORT_UNIT) {
		if (pFileDBInfo->sortType == FILEDB_SORT_BY_CREDATE) {
			UINT32             u32DateTime1, u32DateTime2;

			pFileDbFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
			u32DateTime1 = ((UINT32)pFileDbFileAttr1->creDate << 16) + pFileDbFileAttr1->creTime;

			for (i = left ; i <= right ; i += QUICK_SORT_SAMPLE_STEP) {
				pFileDbFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
				u32DateTime2 = ((UINT32)pFileDbFileAttr2->creDate << 16) + pFileDbFileAttr2->creTime;
				if (u32DateTime1 != u32DateTime2) {
					return SORT_METHOD_QUICK;
				}
			}
			return SORT_METHOD_INSERT;
		} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_MODDATE) {
			UINT32             u32DateTime1, u32DateTime2;

			pFileDbFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
			u32DateTime1 = ((UINT32)pFileDbFileAttr1->lastWriteDate << 16) + pFileDbFileAttr1->lastWriteTime;

			for (i = left; i <= right; i += QUICK_SORT_SAMPLE_STEP) {
				pFileDbFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
				u32DateTime2 = ((UINT32)pFileDbFileAttr2->lastWriteDate << 16) + pFileDbFileAttr2->lastWriteTime;
				if (u32DateTime1 != u32DateTime2) {
					return SORT_METHOD_QUICK;
				}
			}
			return SORT_METHOD_INSERT;
		} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_SIZE) {
			//#NT#2016/03/29#Nestor Yang -begin
			//#NT# Support fileSize larger than 4GB
			//UINT32    fileSize1, fileSize2;
			UINT64    fileSize1, fileSize2;
			//#NT#2016/03/29#Nestor Yang -end

			pFileDbFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
			//#NT#2016/03/29#Nestor Yang -begin
			//#NT# Support fileSize larger than 4GB
			//fileSize1 = pFileDbFileAttr1->fileSize;
			fileSize1 = pFileDbFileAttr1->fileSize64;
			//#NT#2016/03/29#Nestor Yang -end
			for (i = left; i <= right; i += QUICK_SORT_SAMPLE_STEP) {
				pFileDbFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
				//#NT#2016/03/29#Nestor Yang -begin
				//#NT# Support fileSize larger than 4GB
				//fileSize2 = pFileDbFileAttr2->fileSize;
				fileSize2 = pFileDbFileAttr2->fileSize64;
				//#NT#2016/03/29#Nestor Yang -end
				if (fileSize1 != fileSize2) {
					return SORT_METHOD_QUICK;
				}
			}
			return SORT_METHOD_INSERT;
		} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_FILETYPE) {
			UINT32    fileType1, fileType2;

			pFileDbFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
			fileType1 = pFileDbFileAttr1->fileType;
			for (i = left; i <= right; i += QUICK_SORT_SAMPLE_STEP) {
				pFileDbFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
				fileType2 = pFileDbFileAttr2->fileType;
				if (fileType1 != fileType2) {
					return SORT_METHOD_QUICK;
				}
			}
			return SORT_METHOD_INSERT;
		} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_SN) {
			INT32 SN1, SN2;

			pFileDbFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
			SN1 = fdb_GetSNByName(pFileDBInfo, pFileDbFileAttr1->filePath);

			for (i = left; i <= right; i += QUICK_SORT_SAMPLE_STEP) {
				pFileDbFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
				SN2 = fdb_GetSNByName(pFileDBInfo, pFileDbFileAttr2->filePath);
				if (SN1 != SN2) {
					return SORT_METHOD_QUICK;
				}
			}
			return SORT_METHOD_INSERT;
		} else {
			return SORT_METHOD_QUICK;
		}
	} else {
		return SORT_METHOD_INSERT;
	}
}

static void FileDB_InsertSort(PFILEDB_INFO  pFileDBInfo, INT32 left, INT32 right)
{

	if (pFileDBInfo->sortType == FILEDB_SORT_BY_SIZE) {
		FileDB_InsertSortBySize(pFileDBInfo, left, right);
	} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_FILETYPE) {
		FileDB_InsertSortByFileType(pFileDBInfo, left, right);
	} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_TYPE_CRETIME_SIZE) {
		FileDB_InsertSortByTypeCretimeSize(pFileDBInfo, left, right);
	} else if(pFileDBInfo->sortType == FILEDB_SORT_BY_CRETIME_NAME) {
		FileDB_InsertSortByCretimeName(pFileDBInfo, left, right);
	} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_CREDATE) {
		FileDB_InsertSortByCreDate(pFileDBInfo, left, right);
	} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_MODDATE) {
		FileDB_InsertSortByModDate(pFileDBInfo, left, right);
	} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_NAME) {
		if (pFileDBInfo->bIsDCFFileOnly) {
			FileDB_InsertSortByDCFName(pFileDBInfo, left, right);
		} else {
			FileDB_InsertSortByName(pFileDBInfo, left, right);
		}
	} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_FILEPATH) {
		FileDB_InsertSortByPath(pFileDBInfo, left, right);
	} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_SN) {
		FileDB_InsertSortBySN(pFileDBInfo, left, right);
	} else {
		DBG_ERR("Insert sort type %d\r\n", pFileDBInfo->sortType);
	}
}

static void FileDB_InsertSortBySize(PFILEDB_INFO  pFileDBInfo,  INT32 left, INT32 right)
{

	INT32 i, j;
	PFILEDB_FILE_ATTR pFileDbFileAttr1, pFileDbFileAttr2;

	for (i = left + 1; i < right + 1; i++) {
		pFileDbFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		for (j = i; j > left; j--) {
			pFileDbFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j - 1);
			//#NT#2016/03/29#Nestor Yang -begin
			//#NT# Support fileSize larger than 4GB
			//if (pFileDbFileAttr1->fileSize >= pFileDbFileAttr2->fileSize)
			if (pFileDbFileAttr1->fileSize64 >= pFileDbFileAttr2->fileSize64)
				//#NT#2016/03/29#Nestor Yang -end
			{
				break;
			}
			FileDB_ExchangeIndex(pFileDBInfo, j - 1, j);
		}
	}
}

static void FileDB_InsertSortByFileType(PFILEDB_INFO  pFileDBInfo,  INT32 left, INT32 right)
{

	INT32 i, j;
	PFILEDB_FILE_ATTR pFileDbFileAttr1, pFileDbFileAttr2;

	for (i = left + 1; i < right + 1; i++) {
		pFileDbFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		for (j = i; j > left; j--) {
			pFileDbFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j - 1);
			if (pFileDbFileAttr1->fileType >= pFileDbFileAttr2->fileType) {
				break;
			}
			FileDB_ExchangeIndex(pFileDBInfo, j - 1, j);
		}
	}
}


static void FileDB_InsertSortByTypeCretimeSize(PFILEDB_INFO  pFileDBInfo,  INT32 left, INT32 right)
{
	INT32 i, j;
	PFILEDB_FILE_ATTR pFileDbFileAttr1, pFileDbFileAttr2;
	UINT32            u32DateTime1, u32DateTime2;

	for (i = left + 1; i < right + 1; i++) {
		pFileDbFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		u32DateTime1 = ((UINT32)pFileDbFileAttr1->creDate << 16) + pFileDbFileAttr1->creTime;
		for (j = i; j > left; j--) {
			pFileDbFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j - 1);
			if (pFileDbFileAttr1->fileType > pFileDbFileAttr2->fileType) {
				break;
			} else if (pFileDbFileAttr1->fileType == pFileDbFileAttr2->fileType) {
				u32DateTime2 = ((UINT32)pFileDbFileAttr2->creDate << 16) + pFileDbFileAttr2->creTime;

				if (u32DateTime1 > u32DateTime2) {
					break;
				} else if (u32DateTime1 == u32DateTime2) {
					//#NT#2016/03/29#Nestor Yang -begin
					//#NT# Support fileSize larger than 4GB
					//if (pFileDbFileAttr1->fileSize >= pFileDbFileAttr2->fileSize)
					if (pFileDbFileAttr1->fileSize64 >= pFileDbFileAttr2->fileSize64)
						//#NT#2016/03/29#Nestor Yang -end
					{
						break;
					}
				}

			}
			FileDB_ExchangeIndex(pFileDBInfo, j - 1, j);
		}
	}
}

static void FileDB_InsertSortByCretimeName(PFILEDB_INFO  pFileDBInfo,  INT32 left, INT32 right)
{
	INT32 i, j;
	PFILEDB_FILE_ATTR pFileDbFileAttr1, pFileDbFileAttr2;
	UINT32            u32DateTime1, u32DateTime2;

	for(i = left+1; i < right+1; i++) {
		pFileDbFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		u32DateTime1 = ((UINT32)pFileDbFileAttr1->creDate <<16) + pFileDbFileAttr1->creTime;

		for(j = i; j > left; j--) {
			pFileDbFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j - 1);
			u32DateTime2 = ((UINT32)pFileDbFileAttr2->creDate <<16) + pFileDbFileAttr2->creTime;

			if (u32DateTime1 > u32DateTime2) {
				break;
			} else if (u32DateTime1 == u32DateTime2) {
				if (pFileDBInfo->bIsDCFFileOnly) {
					if (FileDB_CompareDCFName(pFileDbFileAttr1->filename, pFileDbFileAttr2->filename)) {
						break;
					}
				} else {
					if (FileDB_CompareName(pFileDbFileAttr1->filename, pFileDbFileAttr2->filename)) {
						break;
					}
				}
			}
			FileDB_ExchangeIndex(pFileDBInfo, j - 1, j);
		}
	}
}

static void FileDB_InsertSortByCreDate(PFILEDB_INFO  pFileDBInfo,  INT32 left, INT32 right)
{

	INT32 i, j;
	PFILEDB_FILE_ATTR pFileDbFileAttr1, pFileDbFileAttr2;
	UINT32            u32DateTime1, u32DateTime2;


	for (i = left + 1; i < right + 1; i++) {
		pFileDbFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		u32DateTime1 = ((UINT32)pFileDbFileAttr1->creDate << 16) + pFileDbFileAttr1->creTime;
		for (j = i; j > left; j--) {
			pFileDbFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j - 1);

			u32DateTime2 = ((UINT32)pFileDbFileAttr2->creDate << 16) + pFileDbFileAttr2->creTime;

			if (u32DateTime1 >= u32DateTime2) {
				break;
			}
			FileDB_ExchangeIndex(pFileDBInfo, j - 1, j);
		}
	}
}

static void FileDB_InsertSortByModDate(PFILEDB_INFO  pFileDBInfo,  INT32 left, INT32 right)
{

	INT32             i, j;
	PFILEDB_FILE_ATTR pFileDbFileAttr1, pFileDbFileAttr2;
	UINT32            u32DateTime1, u32DateTime2;

	for (i = left + 1; i < right + 1; i++) {
		pFileDbFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		u32DateTime1 = ((UINT32)pFileDbFileAttr1->lastWriteDate << 16) + pFileDbFileAttr1->lastWriteTime;
		for (j = i; j > left; j--) {
			pFileDbFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j - 1);

			u32DateTime2 = ((UINT32)pFileDbFileAttr2->lastWriteDate << 16) + pFileDbFileAttr2->lastWriteTime;

			if (u32DateTime1 >= u32DateTime2) {
				break;
			}
			FileDB_ExchangeIndex(pFileDBInfo, j - 1, j);
		}
	}
}

static void FileDB_InsertSortByName(PFILEDB_INFO  pFileDBInfo,  INT32 left, INT32 right)
{

	INT32 i, j;
	PFILEDB_FILE_ATTR pFileDbFileAttr1, pFileDbFileAttr2;

	for (i = left + 1; i < right + 1; i++) {
		pFileDbFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		for (j = i; j > left; j--) {
			pFileDbFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j - 1);
			if (FileDB_CompareName(pFileDbFileAttr1->filename, pFileDbFileAttr2->filename)) {
				break;
			}
			FileDB_ExchangeIndex(pFileDBInfo, j - 1, j);
		}
	}
}

static void FileDB_InsertSortByDCFName(PFILEDB_INFO  pFileDBInfo,  INT32 left, INT32 right)
{

	INT32 i, j;
	PFILEDB_FILE_ATTR pFileDbFileAttr1, pFileDbFileAttr2;

	for (i = left + 1; i < right + 1; i++) {
		pFileDbFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		for (j = i; j > left; j--) {
			pFileDbFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j - 1);
			if (FileDB_CompareDCFName(pFileDbFileAttr1->filePath, pFileDbFileAttr2->filePath)) {
				break;
			}
			FileDB_ExchangeIndex(pFileDBInfo, j - 1, j);
		}
	}
}


static void FileDB_InsertSortByPath(PFILEDB_INFO  pFileDBInfo,  INT32 left, INT32 right)
{
	INT32 i, j;
	PFILEDB_FILE_INT_ATTR pFileDbFileAttr1, pFileDbFileAttr2;

	for (i = left + 1; i < right + 1; i++) {
		pFileDbFileAttr1 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);

		for (j = i; j > left; j--) {
			pFileDbFileAttr2 = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)j - 1);
			if (pFileDbFileAttr1->sortWeight > pFileDbFileAttr2->sortWeight) {
				break;
			} else if (pFileDbFileAttr1->sortWeight == pFileDbFileAttr2->sortWeight) {
				if (FileDB_CompareName(pFileDbFileAttr1->filePath + FILEDB_ROOT_PATH_LEN, pFileDbFileAttr2->filePath + FILEDB_ROOT_PATH_LEN)) {
					break;
				}
			}
			FileDB_ExchangeIndex(pFileDBInfo, j - 1, j);
		}
	}
}

static void FileDB_InsertSortBySN(PFILEDB_INFO pFileDBInfo, INT32 left, INT32 right)
{
	INT32 i, j;
	PFILEDB_FILE_ATTR pFileDbFileAttr1, pFileDbFileAttr2;
	INT32 SN1, SN2;

	for (i = left + 1; i < right + 1; i++) {
		pFileDbFileAttr1 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)i);
		SN1 = fdb_GetSNByName(pFileDBInfo, pFileDbFileAttr1->filename);

		for (j = i; j > left; j--) {
			pFileDbFileAttr2 = FileDB_GetFileAttr(pFileDBInfo, (UINT32)j - 1);
			SN2 = fdb_GetSNByName(pFileDBInfo, pFileDbFileAttr2->filename);
			if (SN1 >= SN2) {
				break;
			}
			FileDB_ExchangeIndex(pFileDBInfo, j - 1, j);
		}
	}
}

#if 0
static void FileDB_DitherIndex(PFILEDB_INFO  pFileDBInfo, INT32 left, INT32 right)
{

	UINT32 i;

	if (right - left > QUICK_SORT_DITHER_STEP) {
		for (i = (UINT32)left; i < ((UINT32)(left + right) >> 1); i += QUICK_SORT_DITHER_STEP) {
			FileDB_ExchangeIndex(pFileDBInfo, i, (UINT32)(right + left) - i);
		}
	}
}
#endif

#if (_SUPPORT_LONG_FILENAME_IN_FILEDB_)
static BOOL FileDB_ChkIfChiWordInside(char *filename)
{
	UINT16 i;
	for (i = 0; i < 8; i += 2) {
		if (fs_IsInCharSet(filename[i + 1], filename[i], KFS_CHARSET_BIG5)) {
			return TRUE;
		}
		if (fs_IsInCharSet(filename[i + 1], filename[i], KFS_CHARSET_GB2312)) {
			return TRUE;
		}
	}
	return FALSE;
}
#endif

static INT32 FileDB_BinarySearchByDCFName(PFILEDB_INFO  pFileDBInfo, INT32 left, INT32 right, const char *filePath)
{
	INT32              Middle = 0;
	PFILEDB_FILE_ATTR  pTempFileDbFileAttr;

	while (left < right) {
		Middle = ((UINT32)(left + right) >> 1);
		pTempFileDbFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)Middle);
		DBG_IND("[search]index =%d, path:%s\r\n", Middle, pTempFileDbFileAttr->filePath);
		if (FileDB_CompareDCFName(filePath, pTempFileDbFileAttr->filePath)) {
			left = Middle + 1;
		} else {
			right = Middle;
		}
	}
	pTempFileDbFileAttr = FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
	if (FileDB_CompareDCFName(filePath, pTempFileDbFileAttr->filePath) == FALSE &&
		FileDB_CompareDCFName(pTempFileDbFileAttr->filePath, filePath) == FALSE
	   ) {
		DBG_IND("[search]index =%d, path:%s\r\n", Middle, pTempFileDbFileAttr->filePath);
		return left;
	} else {
		return FILEDB_SEARCH_ERR;
	}
}

static INT32 FileDB_BinarySearchByPath(PFILEDB_INFO  pFileDBInfo, INT32 left, INT32 right, CHAR *filePath)
{
	INT32                  Middle;
	PFILEDB_FILE_INT_ATTR  pTempFileDbFileAttr;
	UINT32                 tmpSortWeight;



	tmpSortWeight = FileDB_CalcSortWeight(pFileDBInfo, filePath);
	while (left < right) {
		Middle = ((UINT32)(left + right) >> 1);
		pTempFileDbFileAttr = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)Middle);
		DBG_IND("left =%d, right =%d,[search]index =%d, path:%s\r\n", left, right, Middle, pTempFileDbFileAttr->filePath);
		if (tmpSortWeight > pTempFileDbFileAttr->sortWeight) {
			left = Middle + 1;
		} else if (tmpSortWeight == pTempFileDbFileAttr->sortWeight && FileDB_CompareName(filePath, pTempFileDbFileAttr->filePath)) {
			left = Middle + 1;
		} else {
			right = Middle;
		}
	}
	pTempFileDbFileAttr = (PFILEDB_FILE_INT_ATTR)FileDB_GetFileAttr(pFileDBInfo, (UINT32)left);
	if (FileDB_CompareName(filePath, pTempFileDbFileAttr->filePath) == FALSE &&
		FileDB_CompareName(pTempFileDbFileAttr->filePath, filePath) == FALSE
	   ) {
		DBG_IND("out [search]index =%d, path:%s\r\n", left, pTempFileDbFileAttr->filePath);
		return left;
	} else {
		return FILEDB_SEARCH_ERR;
	}
}
INT32 FileDB_GetIndexByPath(FILEDB_HANDLE fileDbHandle, char *filePath)
{
	PFILEDB_FILE_ATTR       pTempFileDbFileAttr;
	PFILEDB_INFO            pFileDBInfo;
	INT32                   ret;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return FILEDB_SEARCH_ERR;
	}
	if (pFileDBInfo->u32TotalFilesNum == 0) {
		DBG_WRN("u32TotalFilesNum=0\r\n");
		return FILEDB_SEARCH_ERR;
	}
	if (!filePath) {
		DBG_WRN("filePath is NULL\r\n");
		return FILEDB_SEARCH_ERR;
	}
	FileDB_lock(fileDbHandle);
	if (pFileDBInfo->sortType == FILEDB_SORT_BY_NAME && pFileDBInfo->bIsDCFFileOnly) {
		ret = FileDB_BinarySearchByDCFName(pFileDBInfo, 0, (INT32)pFileDBInfo->u32TotalFilesNum - 1, filePath);
	} else if (pFileDBInfo->sortType == FILEDB_SORT_BY_FILEPATH && (!pFileDBInfo->bIsDCFFileOnly)) {
		ret = FileDB_BinarySearchByPath(pFileDBInfo, 0, (INT32)pFileDBInfo->u32TotalFilesNum - 1, filePath);
	} else {
		UINT32 i, j;

		for (j = 0; j < pFileDBInfo->u32TotalFilesNum; j++) {
			pTempFileDbFileAttr = FileDB_GetFileAttr(pFileDBInfo, j);
			i = FILEDB_ROOT_PATH_LEN; /* skip A:\ */
			while (i < FILEDB_NAME_MAX_LENG + 1) {
				if (pTempFileDbFileAttr->filePath[i] != filePath[i]) {
					break;
				} else if (pTempFileDbFileAttr->filePath[i] == 0 && filePath[i] == 0) {
					ret = (INT32)j;
					goto GetIndex_end;
				}
				i++;
			}
		}
		//DBG_ERR("ret FILEDB_SEARCH_ERR\r\n");
		ret =  FILEDB_SEARCH_ERR;
	}
GetIndex_end:
	FileDB_unlock(fileDbHandle);
	return ret;
}

PFILEDB_FILE_ATTR FileDB_SpecificFile(FILEDB_HANDLE fileDbHandle, INT32 i32FileIndex)
{
	PFILEDB_INFO            pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return NULL;
	}
	if (pFileDBInfo->u32TotalFilesNum == 0) {
		return NULL;
	}

	if (pFileDBInfo->u32SpecificFileIndex == SPECIFIC_FILE_INDEX_INIT) {
		return FileDB_SearhFile(fileDbHandle, i32FileIndex);
	} else {
		return FileDB_SearhFile(fileDbHandle, (INT32)pFileDBInfo->u32SpecificFileIndex);
	}
}

void FileDB_ClrSpecificFileIndex(FILEDB_HANDLE fileDbHandle)
{
	PFILEDB_INFO            pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return;
	}
	pFileDBInfo->u32SpecificFileIndex = SPECIFIC_FILE_INDEX_INIT;
}

void FileDB_SetSpecificFileIndex(FILEDB_HANDLE fileDbHandle, UINT32 u32SpecFileIndex)
{
	PFILEDB_INFO            pFileDBInfo;
	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) && pFileDBInfo->u32TotalFilesNum != 0 && u32SpecFileIndex < pFileDBInfo->u32TotalFilesNum) {
		pFileDBInfo->u32SpecificFileIndex = u32SpecFileIndex;
	}
}


static UINT32 FileDB_GetMappTblIdxByFileType(UINT32 fileType)
{
	UINT32    i, u32Index;

	u32Index = 0;
	for (i = 0; i < FILEDB_SUPPORT_FILE_TYPE_NUM; i++) {
		if (fileDbFiletype[i].fileType & fileType) {
			u32Index = FileDB_LOG2(fileDbFiletype[i].fileType);
			return u32Index;
		}
	}
	return u32Index;
}


void FileDB_SetFileMappTblByFileType(FILEDB_HANDLE fileDbHandle, UINT32 fileType, UINT32 *TblAddr, UINT32 TblSize)
{
	UINT32                               i, u32Index;
	UINT32                                  tempSize;
	UINT32                                 *pTmpAddr;
	PFILEDB_FILE_ATTR                pFileDbFileAttr;
	PFILEDB_INFO                         pFileDBInfo;


	tempSize = 0;
	pTmpAddr = TblAddr;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return;
	}
	u32Index = FileDB_GetMappTblIdxByFileType(fileType);
	pFileDBInfo->GroupIndexMappTbl[u32Index] = TblAddr;
	pFileDBInfo->groupIndexMappTblSize[u32Index] = TblSize;
	pFileDBInfo->groupFileType[u32Index] = fileType;
	for (i = 0; i < pFileDBInfo->u32TotalFilesNum; i++) {
		pFileDbFileAttr = FileDB_GetFileAttr(pFileDBInfo, i);
		if (pFileDbFileAttr->fileType & fileType) {
			*pTmpAddr = i;
			pTmpAddr++;
			tempSize += 2;
		}
		if (tempSize >= TblSize) {
			DBG_ERR("TblSize too samll Need=0x%x,tblszie=0x%x\r\n", tempSize, TblSize);
		}
	}

}

PFILEDB_FILE_ATTR FileDB_NextFileByType(FILEDB_HANDLE fileDbHandle, UINT32 fileType)
{
	UINT32                   u32FileTypeIndex, u32FilesNum;
	UINT32                  *pGroupIndexMappTbl;
	PFILEDB_INFO             pFileDBInfo;


	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return NULL;
	}

	u32FilesNum = FileDB_GetFilesNumByFileType(fileDbHandle, fileType);
	u32FileTypeIndex = FileDB_GetMappTblIdxByFileType(fileType);
	pGroupIndexMappTbl = pFileDBInfo->GroupIndexMappTbl[u32FileTypeIndex];
	if (!pGroupIndexMappTbl) {
		return FileDB_NextFile(fileDbHandle);
	} else if (u32FilesNum != 0) {
		if (pFileDBInfo->u32GroupCurIdx[u32FileTypeIndex] >= u32FilesNum - 1) {
			if (pFileDBInfo->bIsCyclic == TRUE) {
				pFileDBInfo->u32GroupCurIdx[u32FileTypeIndex] = 0;
			} else {
				return NULL;
			}
		} else {
			pFileDBInfo->u32GroupCurIdx[u32FileTypeIndex]++;
		}
		return FileDB_GetFileAttr(pFileDBInfo, pGroupIndexMappTbl[pFileDBInfo->u32GroupCurIdx[u32FileTypeIndex]]);
	} else {
		return NULL;
	}
}

PFILEDB_FILE_ATTR FileDB_PrevFileByType(FILEDB_HANDLE fileDbHandle, UINT32 fileType)
{
	UINT32                   u32FileTypeIndex, u32FilesNum;
	UINT32                  *pGroupIndexMappTbl;
	PFILEDB_INFO             pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return NULL;
	}
	u32FilesNum = FileDB_GetFilesNumByFileType(fileDbHandle, fileType);
	u32FileTypeIndex = FileDB_GetMappTblIdxByFileType(fileType);
	pGroupIndexMappTbl = pFileDBInfo->GroupIndexMappTbl[u32FileTypeIndex];

	if (!pGroupIndexMappTbl) {
		return FileDB_PrevFile(fileDbHandle);
	} else if (u32FilesNum != 0) {
		if (pFileDBInfo->u32GroupCurIdx[u32FileTypeIndex] == 0) {
			if (pFileDBInfo->bIsCyclic == TRUE) {
				pFileDBInfo->u32GroupCurIdx[u32FileTypeIndex] = u32FilesNum - 1;
			} else {
				return NULL;
			}
		} else {
			pFileDBInfo->u32GroupCurIdx[u32FileTypeIndex]--;
		}
		return FileDB_GetFileAttr(pFileDBInfo, pGroupIndexMappTbl[pFileDBInfo->u32GroupCurIdx[u32FileTypeIndex]]);
	} else {
		return NULL;
	}
}

PFILEDB_FILE_ATTR FileDB_CurrFileByType(FILEDB_HANDLE fileDbHandle, UINT32 fileType)
{
	UINT32                   u32FileTypeIndex, u32FilesNum;
	UINT32                  *pGroupIndexMappTbl;
	PFILEDB_INFO             pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return NULL;
	}

	u32FilesNum = FileDB_GetFilesNumByFileType(fileDbHandle, fileType);
	u32FileTypeIndex = FileDB_GetMappTblIdxByFileType(fileType);
	pGroupIndexMappTbl = pFileDBInfo->GroupIndexMappTbl[u32FileTypeIndex];
	if (!pGroupIndexMappTbl) {
		return FileDB_CurrFile(fileDbHandle);
	} else if (u32FilesNum != 0) {
		return FileDB_GetFileAttr(pFileDBInfo, pGroupIndexMappTbl[pFileDBInfo->u32GroupCurIdx[u32FileTypeIndex]]);
	} else {
		return NULL;
	}
}

PFILEDB_FILE_ATTR FileDB_SearhFileByType(FILEDB_HANDLE fileDbHandle, INT32 i32FileIndex, UINT32 fileType)
{
	UINT32             u32Index, u32FileTypeIndex, u32FilesNum;
	UINT32                               *pGroupIndexMappTbl;
	PFILEDB_INFO                                 pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return NULL;
	}

	u32FilesNum = FileDB_GetFilesNumByFileType(fileDbHandle, fileType);
	u32FileTypeIndex = FileDB_GetMappTblIdxByFileType(fileType);
	pGroupIndexMappTbl = pFileDBInfo->GroupIndexMappTbl[u32FileTypeIndex];
	if (!pGroupIndexMappTbl) {
		return FileDB_SearhFile(fileDbHandle, i32FileIndex);
	} else if (u32FilesNum != 0) {
		if (i32FileIndex >= (INT32)u32FilesNum) {
			if (pFileDBInfo->bIsCyclic == TRUE) {
				u32Index = (UINT32)i32FileIndex % u32FilesNum;
			} else {
				u32Index = u32FilesNum - 1;
			}
		} else if (i32FileIndex < 0) {
			if (pFileDBInfo->bIsCyclic == TRUE) {
				u32Index = (UINT32)i32FileIndex + u32FilesNum;
			} else {
				u32Index = 0;
			}
		} else {
			u32Index = (UINT32)i32FileIndex;
		}
		pFileDBInfo->u32GroupCurIdx[u32FileTypeIndex] = u32Index;
		return FileDB_GetFileAttr(pFileDBInfo, pGroupIndexMappTbl[u32Index]);
	} else {
		return NULL;
	}
}

PFILEDB_FILE_ATTR FileDB_SearhFileByType2(FILEDB_HANDLE fileDbHandle, INT32 i32FileIndex, UINT32 fileType)
{
	UINT32             u32TmpIndex, u32FileTypeIndex;
	PFILEDB_FILE_ATTR  pFileDbFileAttr;
	PFILEDB_INFO           pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return NULL;
	}

	u32FileTypeIndex = FileDB_GetMappTblIdxByFileType(fileType);
	u32TmpIndex = pFileDBInfo->u32GroupCurIdx[u32FileTypeIndex];
	pFileDbFileAttr = FileDB_SearhFileByType(fileDbHandle, i32FileIndex, fileType);
	pFileDBInfo->u32GroupCurIdx[u32FileTypeIndex] = u32TmpIndex;
	return pFileDbFileAttr;
}

PFILEDB_FILE_ATTR FileDB_SpecificFileByType(FILEDB_HANDLE fileDbHandle, INT32 i32FileIndex, UINT32 fileType)
{
	PFILEDB_INFO              pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return NULL;
	}
	if (pFileDBInfo->u32TotalFilesNum != 0) {
		if (pFileDBInfo->u32SpecificFileIndex == SPECIFIC_FILE_INDEX_INIT) {
			return FileDB_SearhFileByType(fileDbHandle, i32FileIndex, fileType);
		} else {
			return FileDB_SearhFileByType(fileDbHandle, (INT32)pFileDBInfo->u32SpecificFileIndex, fileType);
		}
	} else {
		return NULL;
	}
}

UINT32 FileDB_GetCurrFileIndexByType(FILEDB_HANDLE fileDbHandle, UINT32 fileType)
{
	UINT32             u32FileTypeIndex;
	UINT32          *pGroupIndexMappTbl;
	PFILEDB_INFO            pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return 0;
	}

	u32FileTypeIndex = FileDB_GetMappTblIdxByFileType(fileType);
	pGroupIndexMappTbl = pFileDBInfo->GroupIndexMappTbl[u32FileTypeIndex];
	if (!pGroupIndexMappTbl) {
		return FileDB_GetCurrFileIndex(fileDbHandle);
	}
	return pFileDBInfo->u32GroupCurIdx[u32FileTypeIndex];
}


BOOL FileDB_DeleteFileByType(FILEDB_HANDLE fileDbHandle, UINT32 u32FileIndex, UINT32 fileType)
{
	UINT32                i, u32FileTypeIndex, u32FilesNum;
	UINT32               *pGroupIndexMappTbl;
	PFILEDB_INFO                 pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return FALSE;
	}

	u32FilesNum = FileDB_GetFilesNumByFileType(fileDbHandle, fileType);
	u32FileTypeIndex = FileDB_GetMappTblIdxByFileType(fileType);
	pGroupIndexMappTbl = pFileDBInfo->GroupIndexMappTbl[u32FileTypeIndex];
	if (!pGroupIndexMappTbl) {
		return FileDB_DeleteFile(fileDbHandle, u32FileIndex);
	} else if (u32FileIndex < u32FilesNum) {
		if (FileDB_DeleteFile(fileDbHandle, pGroupIndexMappTbl[u32FileIndex])) {
			for (i = 0; i < FILEDB_SUPPORT_FILE_TYPE_NUM; i++) {
				if (pFileDBInfo->GroupIndexMappTbl[i]) {
					FileDB_SetFileMappTblByFileType(fileDbHandle, pFileDBInfo->groupFileType[i], pFileDBInfo->GroupIndexMappTbl[i], pFileDBInfo->groupIndexMappTblSize[i]);
				}
			}
			return TRUE;
		} else {
			return FALSE;
		}
	} else {
		return FALSE;
	}
}


UINT32 FileDB_CalcBuffSize(UINT32 fileNum, UINT32 u32MaxFilePathLen)
{

	UINT32 u32FileItemSize, u32IndexItemSize;

	u32FileItemSize = sizeof(FILEDB_FILE_ATTR) + ALIGN_CEIL_4(u32MaxFilePathLen);
	u32IndexItemSize = 4;
	return (sizeof(FILEDB_INFO) + (u32FileItemSize + u32IndexItemSize) * (fileNum + FILEDB_RESERVE_FILE_ENTRY_NUM) + ALIGN_CEIL_4(FILEDB_MAX_DIFF_DCF_FOLDER) + ALIGN_CEIL_4(FILEDB_MAX_DCF_FILENUM));

}

void FileDB_DumpInfo(FILEDB_HANDLE fileDbHandle)
{
	UINT32                   fidx = 0, CurrIndex;
	PFILEDB_FILE_INT_ATTR    pFileAttr;
	PFILEDB_INFO             pFileDBInfo;


	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		return;
	}
	DBG_DUMP("[dump]----------------FileDB info begin----------------\r\n");
	DBG_DUMP("[dump]----\trootPath=\t\t%s\r\n", pFileDBInfo->rootPath);
	if (pFileDBInfo->defaultfolder[0]) {
		DBG_DUMP("[dump]----\tdefaultfolder=\t\t%s\r\n", pFileDBInfo->defaultfolder);
	} else {
		DBG_DUMP("[dump]----\tdefaultfolder=\t\tNULL\t\t\r\n");
	}
	if (pFileDBInfo->filterfolder[0]) {
		DBG_DUMP("[dump]----\tfilterfolder=\t\t%s\r\n", pFileDBInfo->filterfolder);
	} else {
		DBG_DUMP("[dump]----\tdfilterfolder=\t\tNULL\t\t\r\n");
	}
	DBG_DUMP("[dump]----\tfileFilter=\t\t0x%x\r\n", pFileDBInfo->fileFilter);
	DBG_DUMP("[dump]----\tsortType=\t\t%d\r\n", pFileDBInfo->sortType);
	DBG_DUMP("[dump]----\tRecursive=\t\t%d\r\n", pFileDBInfo->bIsRecursive);
	DBG_DUMP("[dump]----\tSkipDirForNonRecursive=\t%d\r\n", pFileDBInfo->bIsSkipDirForNonRecursive);
	DBG_DUMP("[dump]----\tSkipHidden=\t\t%d\r\n", pFileDBInfo->bIsSkipHidden);
	DBG_DUMP("[dump]----\tSupportLong=\t\t%d\r\n", pFileDBInfo->bIsSupportLongName);
	DBG_DUMP("[dump]----\tMemAddr=\t\t0x%x\r\n", pFileDBInfo->u32MemAddr);
	DBG_DUMP("[dump]----\tSortIndexAddr=\t\t0x%x\r\n", (unsigned int)pFileDBInfo->pSortIndex);
	DBG_DUMP("[dump]----\tSortIndexBufSize=\t0x%x\r\n", pFileDBInfo->SortIndexBufSize);
	DBG_DUMP("[dump]----\tFileInfoAddr=\t\t0x%x\r\n", pFileDBInfo->pFileInfoAddr);
	DBG_DUMP("[dump]----\tMaxStoreItemsNum=\t%d\r\n", pFileDBInfo->u32MaxStoreItemsNum);
	DBG_DUMP("[dump]----\tMaxFileNum=\t\t%d\r\n", pFileDBInfo->u32MaxFileNum);
	DBG_DUMP("[dump]----\tMaxFilePathLen=\t\t%d\r\n", pFileDBInfo->u32MaxFilePathLen);
	DBG_DUMP("[dump]----\tFileItemSize=\t\t%d\r\n", pFileDBInfo->u32FileItemSize);
	DBG_DUMP("[dump]----\tTolFileNum=\t\t%d\r\n", pFileDBInfo->u32TotalFilesNum);
	DBG_DUMP("[dump]----\tCurIndex=\t\t%d\r\n", pFileDBInfo->u32CurrentFileIndex);

	if (pFileDBInfo->SortSN_Delim[0]) {
		DBG_DUMP("[dump]----\tSortSN_Delim=\t\t%s\r\n", pFileDBInfo->SortSN_Delim);
		DBG_DUMP("[dump]----\tSortSN_DelimCount=\t%d\r\n", pFileDBInfo->SortSN_DelimCount);
		DBG_DUMP("[dump]----\tSortSN_CharNumOfSN=\t%d\r\n", pFileDBInfo->SortSN_CharNumOfSN);
	}

	//list all files if exists
	if (pFileDBInfo->u32TotalFilesNum) {
		CurrIndex = FileDB_GetCurrFileIndex(fileDbHandle);

		for (fidx = 0; fidx < pFileDBInfo->u32TotalFilesNum; fidx++) {
			pFileAttr = (PFILEDB_FILE_INT_ATTR)FileDB_internalSearhFile(fileDbHandle, fidx);
			if (FileDB_ChkFileAttr((PFILEDB_FILE_ATTR)pFileAttr)) {
				//DBG_DUMP("[dump]-- file %04d =(%s) w=0x%x \r\n",i,pFileAttr->filePath,pFileAttr->sortWeight);
				//#NT#2016/03/30#Nestor Yang -begin
				//#NT# Support fileSize larger than 4GB
				//DBG_DUMP("[dump]-- file %04d ,Type = 0x%x, CreTime=%ld, ModTime=%ld, Fsize=%ld, w=0x%x \r\n",i,pFileAttr->fileType,(UINT32)(pFileAttr->creDate <<16) + pFileAttr->creTime,(UINT32)(pFileAttr->lastWriteDate<<16) + pFileAttr->lastWriteTime,pFileAttr->fileSize, pFileAttr->sortWeight);
				DBG_DUMP("[dump]-- fidx %04d, wt 0x%X, type 0x%x, cre 0x%08lX, mod 0x%08lX, size %lld, attr 0x%X\r\n",
							fidx,
							pFileAttr->sortWeight,
							pFileAttr->fileType,
							(UINT32)(pFileAttr->creDate << 16) + pFileAttr->creTime,
							(UINT32)(pFileAttr->lastWriteDate << 16) + pFileAttr->lastWriteTime,
							pFileAttr->fileSize64,
							pFileAttr->attrib);
				//#NT#2016/03/30#Nestor Yang -end
				DBG_DUMP("[dump]--            path [%s]\r\n", pFileAttr->filePath);
			} else {
				DBG_ERR("file %04d , Data is OverWrite\r\n", fidx);
			}
		}

		FileDB_SearhFile(fileDbHandle, CurrIndex);
	}

	DBG_DUMP("[dump]----------------FileDB info end----------------\r\n");
}

BOOL FileDB_SetFileSize(FILEDB_HANDLE fileDbHandle, INT32 i32FileIndex, UINT64 fileSize64)
{
	PFILEDB_FILE_ATTR  pFileAttr;

	FileDB_lock(fileDbHandle);

	pFileAttr = FileDB_internalSearhFile(fileDbHandle, i32FileIndex);
	if (NULL == pFileAttr) {
		DBG_ERR("Get file idx %d failed\r\n", i32FileIndex);
		FileDB_unlock(fileDbHandle);
		return FALSE;
	}

	pFileAttr->fileSize64 = fileSize64;

	FileDB_unlock(fileDbHandle);

	return TRUE;
}

BOOL FileDB_SetFileRO(FILEDB_HANDLE fileDbHandle, INT32 i32FileIndex, BOOL bReadOnly)
{
	PFILEDB_FILE_ATTR  pFileAttr;

	FileDB_lock(fileDbHandle);

	pFileAttr = FileDB_internalSearhFile(fileDbHandle, i32FileIndex);
	if (NULL == pFileAttr) {
		DBG_ERR("Get file idx %d failed\r\n", i32FileIndex);
		FileDB_unlock(fileDbHandle);
		return FALSE;
	}

	if (bReadOnly) {
		pFileAttr->attrib |= FST_ATTRIB_READONLY;
	} else {
		pFileAttr->attrib &= (~FST_ATTRIB_READONLY);
	}

	FileDB_unlock(fileDbHandle);

	return TRUE;
}

static INT32 fdb_GetSNByName(PFILEDB_INFO pFileDBInfo, char *fileName)
{
	CHAR szSN[SORTSN_MAX_CHAR_NUM_OF_SN + 1] = {0};
	CHAR *pHead;
	INT32 i;

	pHead = fileName;

	if (0 == pFileDBInfo->SortSN_DelimLen ||
		0 == pFileDBInfo->SortSN_DelimCount ||
		0 == pFileDBInfo->SortSN_CharNumOfSN) {
		DBG_ERR("SortSN param is not set\r\n");
		return -1;
	}

	for (i = 0; i < pFileDBInfo->SortSN_DelimCount; i++) {
		pHead = strstr(pHead, pFileDBInfo->SortSN_Delim);
		if (NULL == pHead) {
			DBG_IND("%s szSN N/A\r\n", fileName);
			return -1;
		}
		pHead += pFileDBInfo->SortSN_DelimLen;
	}

	strncpy(szSN, pHead, pFileDBInfo->SortSN_CharNumOfSN);
	szSN[pFileDBInfo->SortSN_CharNumOfSN] = '\0';

	//DBG_IND("%s szSN %s SN %d\r\n", fileName, szSN, atol(szSN));

	//return atol(szSN);

	/* changed handling for hex, char limit (0~F) */
	DBG_IND("%s szSN %s SN %d\r\n", fileName, szSN, strtol(szSN, NULL, SORT_METHOD_STRBASE));

	return strtol(szSN, NULL, SORT_METHOD_STRBASE);
}

INT32 FileDB_GetSNByName(FILEDB_HANDLE fileDbHandle, char *fileName)
{
	PFILEDB_INFO pFileDBInfo;

	if ((pFileDBInfo = FileDB_GetInfoByHandle(fileDbHandle)) == NULL) {
		DBG_ERR("Invalid handle 0x%X\r\n", fileDbHandle);
		return -1;
	}

	if (NULL == fileName) {
		DBG_ERR("fileName is NULL\r\n");
		return -1;
	}

	return fdb_GetSNByName(pFileDBInfo, fileName);
}

INT32 FileDB_GetFileTypeByPath(char *filePath, UINT32 *fileType)
{
	CHAR          ExtName[FILE_EXT_LEN + 1];
	const CHAR    *pDot;
	UINT32        ExtLen = 0;
	UINT32        FileType = 0;

	//find dot
	pDot = strrchr(filePath, '.');

	//take ExtName
	memset(ExtName, 0x00, sizeof(ExtName));
	if (pDot) {
		ExtLen = strlen(filePath) - ((UINT32)pDot - (UINT32)filePath) - 1;//-1 is for '.'
	} else {
		ExtLen = 0;
	}
	if (ExtLen > 0) {
		memcpy(ExtName, pDot + 1, ExtLen);
	}

	//get fileType
	FileType = FileDB_GetFileType(ExtName);
	*fileType = FileType;

	return 0;
}

