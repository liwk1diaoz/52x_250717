#include "PrjInc.h"
#include "sys_mempool.h"
#include "Dx.h"
#include "FileDB.h"
#if _TODO
#include <plat/sdio.h>
#include <plat/strg_def.h>
#include <plat/nand.h>
#endif
#if (LOGFILE_FUNC==ENABLE)
#include "LogFile.h"
#endif
#if (UPDFW_MODE == ENABLE)
#include "Mode/UIModeUpdFw.h"
#endif
#include "UIApp/Network/UIAppNetwork.h"

#include <comm/hwclock.h>

#include "EthCam/EthsockCliIpcAPI.h"
#include "EthCam/EthCamSocket.h"
#include "UIApp/Network/EthCamAppSocket.h"
#include "UIApp/Network/EthCamAppNetwork.h"
#include "UIApp/Network/EthCamAppCmd.h"
#if (USE_DCF == ENABLE)
#include "DCF.h"
#endif
#include "Utility/SwTimer.h"
#include "SysMain.h"
//#NT#2016/05/30#Lincy Lin -end
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UIBKGObj
#define __DBGLVL__          1 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////
#define PB_COPY2CARD_NEW_DIR            0x00
#define PB_COPY2CARD_APPEND             0x01
#define PB_COPY2CARD_NO_RENAME          0x02
#define PB_COPY2CARD_NEW_DIR_ORG_FILE   0x03

static UINT32 Background_ProtectAll(BOOL bLock); //for any mode protect/unproteck all
static INT32 Background_DeleteAll(void); //for any mode delete all
static UINT32 BackgroundDelAll(void);
static UINT32 BackgroundCopy2Card(void);
#if(COPYCARD2CARD_FUNCTION == ENABLE)
static UINT32 BackgroundCopyCard1ToCard2(void);
static UINT32 BackgroundCopyCard2ToCard1(void);
#endif
static UINT32 BackgroundFormat(void);
static UINT32 BackgroundFormatCard(void);
static UINT32 BackgroundFormatNand(void);
static UINT32 BackgroundNumReset(void);
#if (USE_DPOF==ENABLE)
static UINT32 BackgroundSetDPOF(void);
#endif
static UINT32 BackgroundSetProtect(void);
static UINT32 BackgroundDummyWait(void);
static UINT32 BackgroundFWUpdate(void);
#if (PHOTO_MODE==ENABLE)
static UINT32 BackgroundDZoomIn(void);
static UINT32 BackgroundDZoomOut(void);
#endif
static UINT32 BackgroundPIMProcess(void);
//#NT#2012/10/30#Calvin Chang#Generate Audio NR pattern by noises of zoom, focus and iris -begin
static UINT32 BackgroundAudiNRLensAction(void);
//#NT#2012/10/30#Calvin Chang -end
static UINT32 BackgroundWiFiOn(void);
#if(WIFI_AP_FUNC==ENABLE)
static UINT32 BackgroundWiFiClearACL(void);
#endif
static UINT32 BackgroundStopRecProcess(void);
#if (defined(_NVT_ETHREARCAM_TX_) || defined(_NVT_ETHREARCAM_RX_))
static UINT32 BackgroundEthCamSocketOpen(void);
static UINT32 BackgroundEthCamSocketCliCmdOpen(void);
static UINT32 BackgroundEthCamSocketCliDataOpen(void);
static UINT32 BackgroundEthCamGetTxInfo(void);
static UINT32 BackgroundEthCamTriggerThumb(void);
static UINT32 BackgroundEthCamTxStreamStart(void);
static UINT32 BackgroundEthCamTxDecInfo(void);
static UINT32 BackgroundEthCamTxRecInfo(void);
static UINT32 BackgroundEthCamTxDispOpenSocketStreamStart(void);
static UINT32 BackgroundEthCamTxDispOpenSocketStream(void);
static UINT32 BackgroundEthCamTxRecOpenSocketStreamStart(void);
static UINT32 BackgroundEthCamSyncTime(void);
static UINT32 BackgroundEthCamDecErr(void);
static UINT32 BackgroundEthCamRawEncodeResult(void);
static UINT32 BackgroundEthCamSetTxSysInfo(void);
static UINT32 BackgroundEthCamCheckPortReady(void);
static UINT32 BackgroundEthCamUpdateUIInfo(void);
static UINT32 BackgroundEthCamIperfTest(void);
#endif

static UINT32 g_uiDpofOPMode = PLAYDPOF_SETONE;
static UINT32 g_uiDpofPrtNum = 0;
static UINT32 g_uiDpofDateOn = FALSE;
static UINT32 g_uiProtectType = SETUP_PROTECT_ALL;
static UINT32 g_uiWaitTime = 1;
static UINT32 *g_pDzoomStop = NULL;
static UINT32 g_uiEthcamTriggerThumbPathId[ETHCAM_PATH_ID_MAX] = {0};
static UINT32 g_uiEthcamDecErrPathId = 0;
static UINT32 g_uiEthcamCheckPortReadyIPAddr = 0;

UINT32 gUIStopBckGndDummyWait = FALSE;
#if USE_FILEDB
static FILEDB_INIT_OBJ gBKFDBInitObj = {
	"A:\\",  //rootPath
	NULL,  //defaultfolder
	NULL,  //filterfolder
	TRUE,  //bIsRecursive
	TRUE,  //bIsCyclic
	TRUE,  //bIsMoveToLastFile
	TRUE, //bIsSupportLongName
	FALSE, //bIsDCFFileOnly
	TRUE,  //bIsFilterOutSameDCFNumFolder
	{'N', 'V', 'T', 'I', 'M'}, //OurDCFDirName[5]
	{'I', 'M', 'A', 'G'}, //OurDCFFileName[4]
	FALSE,  //bIsFilterOutSameDCFNumFile
	FALSE, //bIsChkHasFile
	60,    //u32MaxFilePathLen
	5000,  //u32MaxFileNum
	(FILEDB_FMT_JPG | FILEDB_FMT_AVI | FILEDB_FMT_MOV | FILEDB_FMT_MP4 | FILEDB_FMT_TS), //fileFilter
	0,     //u32MemAddr
	0,     //u32MemSize
	NULL   //fpChkAbort

};
#endif
BKG_JOB_ENTRY gBackgroundExtFuncTable[] = {
	{NVTEVT_BKW_DELALL,             BackgroundDelAll},
	{NVTEVT_BKW_COPY2CARD,          BackgroundCopy2Card},
#if(COPYCARD2CARD_FUNCTION == ENABLE)
	{NVTEVT_BKW_COPYCARD1ToCARD2,   BackgroundCopyCard1ToCard2},
	{NVTEVT_BKW_COPYCARD2ToCARD1,   BackgroundCopyCard2ToCard1},
#endif
	{NVTEVT_BKW_FORMAT,             BackgroundFormat},
	{NVTEVT_BKW_FORMAT_CARD,        BackgroundFormatCard},
	{NVTEVT_BKW_FORMAT_NAND,        BackgroundFormatNand},
	{NVTEVT_BKW_NUM_RESET,          BackgroundNumReset},
#if (USB_MODE==ENABLE)
#if (USE_DPOF==ENABLE)
	{NVTEVT_BKW_SETDPOF,            BackgroundSetDPOF},
#endif
#endif
	{NVTEVT_BKW_SETPROTECT,         BackgroundSetProtect},
	{NVTEVT_BKW_DUMMY_WAIT,         BackgroundDummyWait},
	{NVTEVT_BKW_FW_UPDATE,          BackgroundFWUpdate},
#if (PHOTO_MODE==ENABLE)
	{NVTEVT_BKW_DZOOM_IN,           BackgroundDZoomIn},
	{NVTEVT_BKW_DZOOM_OUT,          BackgroundDZoomOut},
#endif
	{NVTEVT_BKW_PIM_PROCESS,        BackgroundPIMProcess},
	//#NT#2012/10/30#Calvin Chang#Generate Audio NR pattern by noises of zoom, focus and iris -begin
	{NVTEVT_BKW_ANR_LENS_ACTION,    BackgroundAudiNRLensAction},
	//#NT#2012/10/30#Calvin Chang -end
	{NVTEVT_BKW_WIFI_ON,            BackgroundWiFiOn},
#if(WIFI_AP_FUNC==ENABLE)
	{NVTEVT_BKW_WIFI_CLEARACL,      BackgroundWiFiClearACL},
#endif
	{NVTEVT_BKW_STOPREC_PROCESS,    BackgroundStopRecProcess},
#if (defined(_NVT_ETHREARCAM_TX_) || defined(_NVT_ETHREARCAM_RX_))
	{NVTEVT_BKW_ETHCAM_SOCKET_OPEN,    BackgroundEthCamSocketOpen},
	{NVTEVT_BKW_ETHCAM_SOCKETCLI_CMD_OPEN,    BackgroundEthCamSocketCliCmdOpen},
	{NVTEVT_BKW_ETHCAM_SOCKETCLI_DATA_OPEN,    BackgroundEthCamSocketCliDataOpen},
	{NVTEVT_BKW_GET_ETHCAM_TX_INFO,    BackgroundEthCamGetTxInfo},
	{NVTEVT_BKW_TRIGGER_THUMB,    BackgroundEthCamTriggerThumb},
	{NVTEVT_BKW_TX_STREAM_START,    BackgroundEthCamTxStreamStart},


	{NVTEVT_BKW_GET_ETHCAM_TX_DECINFO,    					BackgroundEthCamTxDecInfo},
	{NVTEVT_BKW_ETHCAM_SOCKETCLI_DISP_DATA_OPEN_START,    	BackgroundEthCamTxDispOpenSocketStreamStart},
	{NVTEVT_BKW_ETHCAM_SOCKETCLI_DISP_DATA_OPEN,    	BackgroundEthCamTxDispOpenSocketStream},
	{NVTEVT_BKW_GET_ETHCAM_TX_RECINFO,    					BackgroundEthCamTxRecInfo},
	{NVTEVT_BKW_ETHCAM_SOCKETCLI_REC_DATA_OPEN_START,    	BackgroundEthCamTxRecOpenSocketStreamStart},
	{NVTEVT_BKW_ETHCAM_SYNC_TIME,    						BackgroundEthCamSyncTime},

	{NVTEVT_BKW_ETHCAM_DEC_ERR,    BackgroundEthCamDecErr},
	{NVTEVT_BKW_ETHCAM_RAW_ENCODE_RESULT,    BackgroundEthCamRawEncodeResult},
	{NVTEVT_BKW_ETHCAM_SET_TX_SYSINFO,    BackgroundEthCamSetTxSysInfo},
	{NVTEVT_BKW_ETHCAM_CHECKPORT_READY,    BackgroundEthCamCheckPortReady},
	{NVTEVT_BKW_ETHCAM_UPDATE_UI,    BackgroundEthCamUpdateUIInfo},
	{NVTEVT_BKW_ETHCAM_IPERF_TEST,    BackgroundEthCamIperfTest},

#endif

	{0,                             0},
};


UINT32 Background_CopyTo(BOOL isCopyToCard, UINT32 copMode)
{
#if _TODO
	//#NT#2016/06/27#Lincy Lin -begin
	//#NT#Reduce IPCAM used memory pool size
	UINT32 uiTempBufAddr, uiTempBufSize;
	INT32  uiError_Code = FST_STA_OK;
	//HNVT_STRG pSrcObj, pDstObj, pOriObj;
	FS_HANDLE pSrcDXH = 0, pDstDXH = 0, pOriDXH = 0;
	COPYTO_BYNAME_INFO CopyInfo;
#if USE_FILEDB
	FILEDB_HANDLE fileDbHandle;
	UINT32        fileDbbufSize = 0x200000, fileDbbufAddr, FileNumber;
#endif

	uiTempBufAddr = SxCmd_GetTempMem(POOL_SIZE_APP);
	uiTempBufSize = POOL_SIZE_APP;

	// reserved 2MB for FileDB using
#if USE_FILEDB
	uiTempBufSize -= fileDbbufSize;
	fileDbbufAddr = uiTempBufAddr + uiTempBufSize;
#endif

	//debug_msg("uiTempBufAddr = 0x%x, uiTempBufSize = 0x%x\r\n",uiTempBufAddr,uiTempBufSize);
	//debug_msg("fileDbbufAddr = 0x%x, fileDbbufSize = 0x%x\r\n",fileDbbufAddr,fileDbbufSize);

	UI_SetData(FL_IsCopyToCarding, TRUE);

	if (isCopyToCard) {
		FileSys_GetStrgObj(&pDstDXH);
		pOriDXH = pDstDXH;
		// --> TODO:remove begin
#if (defined(_EMBMEM_NAND_) || defined(_EMBMEM_SPI_NAND_))
		{
			pSrcDXH = (FS_HANDLE)Dx_GetObject(DX_CLASS_STORAGE_EXT | USER_DX_TYPE_EMBMEM_FAT);
		}
#else
		//pSrcObj = (HNVT_STRG)sdio2_getStorageObject(STRG_OBJ_FAT1);
		//sdio2_setDetectCardExistHdl(LibFs_CheckCardInserted);
		//sdio2_setDetectCardProtectHdl(SLibFs_CheckCardWP);
#endif
		// --> TODO:remove end
	} else {
		//FileSys_GetStrgObj(&pOriObj);
		FileSys_GetStrgObj(&pOriDXH);
		//pSrcObj = pOriObj;
		pSrcDXH = pOriDXH;
		// --> TODO:remove begin
#if (defined(_EMBMEM_NAND_) || defined(_EMBMEM_SPI_NAND_))
		{
			pDstDXH = (FS_HANDLE)Dx_GetObject(DX_CLASS_STORAGE_EXT | USER_DX_TYPE_EMBMEM_FAT);
		}
#else
		//pDstObj = (HNVT_STRG)sdio2_getStorageObject(STRG_OBJ_FAT1);
		//sdio2_setDetectCardExistHdl(LibFs_CheckCardInserted);
		//sdio2_setDetectCardProtectHdl(SLibFs_CheckCardWP);
#endif
	}
	memset(&CopyInfo, 0, sizeof(CopyInfo));
	CopyInfo.pSrcStrg = pSrcDXH;
	CopyInfo.pDstStrg = pDstDXH;
	CopyInfo.pBuf = (char *)uiTempBufAddr;
	CopyInfo.uiBufSize = uiTempBufSize;

#if (USE_FILEDB == ENABLE)
	{
		if (pOriDXH != pSrcDXH) {
			if (FileSys_ChangeDisk(pSrcDXH) != FSS_OK) {
				DBG_ERR("ChangeDisk fail\r\n");
			}
		}
		//  create fileDB for copy
		{
			FILEDB_INIT_OBJ   FilDBInitObj = {0};
			CHAR              rootPath[20] = "A:\\";

			FilDBInitObj.rootPath = rootPath;
			FilDBInitObj.bIsRecursive = TRUE;
			FilDBInitObj.bIsCyclic = TRUE;
			FilDBInitObj.bIsMoveToLastFile = TRUE;
			FilDBInitObj.bIsSupportLongName = TRUE;
			FilDBInitObj.bIsDCFFileOnly = FALSE;
			FilDBInitObj.bIsChkHasFile = FALSE;
			FilDBInitObj.u32MaxFilePathLen = 60;
			FilDBInitObj.u32MaxFileNum = 5000;
			FilDBInitObj.fileFilter = FILEDB_FMT_JPG | FILEDB_FMT_AVI | FILEDB_FMT_MOV | FILEDB_FMT_MP4 | FILEDB_FMT_TS;
			FilDBInitObj.u32MemAddr = dma_getCacheAddr(fileDbbufAddr);
			FilDBInitObj.u32MemSize = fileDbbufSize;
			fileDbHandle = FileDB_Create(&FilDBInitObj);
		}
		// copy file one by one
		{
			UINT32                   i = 0, CurrIndex;
			PFILEDB_FILE_ATTR        pFileAttr = 0;

			CurrIndex = FileDB_GetCurrFileIndex(fileDbHandle);
			FileNumber = FileDB_GetTotalFileNum(fileDbHandle);
			for (i = 0; i < FileNumber; i++) {
				pFileAttr = FileDB_SearhFile(fileDbHandle, i);
				if (!pFileAttr) {
					DBG_ERR("no pFileAttr\r\n");
					break;
				}
				CopyInfo.pSrcPath = pFileAttr->filePath;
				CopyInfo.pDstPath = pFileAttr->filePath;
				debug_msg("Copy file %s\r\n", pFileAttr->filePath);
				uiError_Code = FileSys_CopyToByName(&CopyInfo);
				if (uiError_Code != FST_STA_OK) {
					DBG_ERR("err %d\r\n", uiError_Code);
					break;
				}
			}
			if (FileNumber) {
				FileDB_SearhFile(fileDbHandle, CurrIndex);
			}
		}
		FileDB_Release(fileDbHandle);
	}
#endif
	//#NT#2016/06/27#Lincy Lin -end
	if (pOriDXH != pSrcDXH) {
		if (FileSys_ChangeDisk(pOriDXH) != FSS_OK) {
			DBG_ERR("ChangeDisk fail\r\n");
		}
	}

#if (USE_FILEDB == ENABLE)
	{
		UINT32  curIndex;

		fileDbHandle = 0;
		FileDB_Refresh(fileDbHandle);
		FileNumber = FileDB_GetTotalFileNum(fileDbHandle);
		curIndex = FileNumber - 1;
		if (FileNumber) {
			FileDB_SearhFile(fileDbHandle, curIndex);
		}
#if (PLAY_MODE==ENABLE)
		if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PLAYBACK) {
			PB_OpenSpecFileBySeq(curIndex + 1, TRUE);
			PB_WaitCommandFinish(PB_WAIT_INFINITE);
		}
#endif
	}
#endif
#if (USE_DCF == ENABLE)
	{
		DCF_Refresh();
		DCF_SetCurIndex(DCF_GetDBInfo(DCF_INFO_TOL_FILE_COUNT));

#if (PLAY_MODE==ENABLE)
		if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PLAYBACK) {
			PB_OpenSpecFileBySeq(DCF_GetDBInfo(DCF_INFO_CUR_INDEX), TRUE);
			PB_WaitCommandFinish(PB_WAIT_INFINITE);
		}
#endif
	}
#endif

	UI_SetData(FL_IsCopyToCarding, FALSE);
    SxCmd_RelTempMem(uiTempBufAddr);
	return uiError_Code;
#else
    return 0;
#endif
}


UINT32 BackgroundDelAll(void)
{
	INT32 ret;
#if (PLAY_MODE==ENABLE)
	if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PLAYBACK) {
		UIPlay_Delete(PB_DELETE_ALL);
		ret = TRUE;//dummy return value
	} else
#endif
	{
		ret = Background_DeleteAll();
	}
	return (UINT32)ret;
}

//for any mode delete all,
//UI flow need to update orgianl FileDB data,check FL_DeleteAllFlag
INT32 Background_DeleteAll(void)
{
    INT32              ret = E_OK;
#if (USE_FILEDB == ENABLE)
	FILEDB_HANDLE      FileDBHdl = 0;
	PFILEDB_FILE_ATTR  FileAttr  = NULL;

	INT32              FileNum = 0, i = 0;
	PFILEDB_INIT_OBJ   pFDBInitObj = &gBKFDBInitObj;

	UI_SetData(FL_DeleteAllFlag, TRUE);

    pFDBInitObj->u32MemAddr = SxCmd_GetTempMem(POOL_SIZE_FILEDB);

	if (!pFDBInitObj->u32MemAddr) {
        DBG_ERR("Can't alloc mem\r\n");
        ret = E_NOMEM;
		return ret;
	}

	pFDBInitObj->u32MemSize = POOL_SIZE_FILEDB;
	FileDBHdl               = FileDB_Create(pFDBInitObj);

	FileNum = FileDB_GetTotalFileNum(FileDBHdl);
	for (i = FileNum - 1; i >= 0; i--) {
		FileAttr = FileDB_SearhFile(FileDBHdl, i);
		if (FileAttr) {
			if (M_IsReadOnly(FileAttr->attrib)) {
				continue;
			}
			ret = FileSys_DeleteFile(FileAttr->filePath);
			DBG_IND("i = %04d path=%s\r\n", i, FileAttr->filePath);
			if (ret != FST_STA_OK) {
                DBG_ERR("FSYS_Del File err %s\r\n",FileAttr->filePath);
				goto FDB_Delete_Err;
			} else {
				FileDB_DeleteFile(FileDBHdl, i);
			}
		} else {
		    DBG_ERR("FDB_search err\r\n");
			goto FDB_Delete_Err;
		}
	}
	FileDB_Release(FileDBHdl);
    SxCmd_RelTempMem(pFDBInitObj->u32MemAddr);
	if (UI_GetData(FL_IsUseFileDB)) {
		Ux_SendEvent(&UISetupObjCtrl, NVTEVT_FILEID_RESET, 0);
	}
	return ret;

FDB_Delete_Err:
	FileDB_Release(FileDBHdl);
    SxCmd_RelTempMem(pFDBInitObj->u32MemAddr);
	if (UI_GetData(FL_IsUseFileDB)) {
		Ux_SendEvent(&UISetupObjCtrl, NVTEVT_FILEID_RESET, 0);
	}
    ret = E_SYS;
	return ret;
#else
	DBG_ERR("not support!\r\n");
    ret = E_SYS;
	return ret;
#endif
}
UINT32 BackgroundCopy2Card(void)
{
	return Background_CopyTo(TRUE, PB_COPY2CARD_NEW_DIR);
}

UINT32 BackgroundCopy2Nand(void)
{
	return Background_CopyTo(FALSE, PB_COPY2CARD_NEW_DIR);
}

#if(COPYCARD2CARD_FUNCTION == ENABLE)
//#NT#2016/10/18#Niven Cho -begin
//#NT#mount sd-2 as "B:\\"
typedef enum {
	WARN_REMOVE_USB = 0,
	WARN_CONNECT_FAIL,
	WARN_UNKNOWN_FILE,
	WARN_NO_PICTURE,
	WARN_LENS_ERROR,
	WARN_CARD_ERROR,
	WARN_BUILT_IN_MEMORY_ERROR,
	WARN_CARD_UNFORMAT,
	WARN_WRITE_PROTECT,
	WARN_CARD_FULL,
	WARN_MEM_FULL,
	WARN_FILE_PROTECT,
	WARN_DEL_PROTECT,
	WARN_BATT_EMPTY,
	WARN_PAN_ERROR,
	WARN_COPY_CARD_FULL,
	WARN_COPY_CARD_PROTECT,
	WARN_COPY_NO_PICTURE,
	WRN_FW_UPDATE_COMPLETE,
	WARN_RECORD_SLOW_MEDIA,
	WRN_FW_UPDATING,
	WRN_FW_CHECK,
	WRN_NOT_DETECT_REDEYE,
	WRN_EXCEED_MAX_FOLDER,
	WRN_TOO_MANY_PIC,
	WARN_TYPE_MAX
} WARN_TYPE;
//#NT#2016/10/18#Niven Cho -end
UINT32 Background_CopyCardToCard(BOOL IsPrimaryCard, UINT32 copMode)
{
	UINT32 uiTempBufAddr, uiTempBufSize, FileNumber;
	INT32  uiError_Code = FST_STA_OK;
	//HNVT_STRG pSrcObj, pDstObj, pOriObj;
	FS_HANDLE pSrcDXH, pDstDXH, pOriDXH;
	COPYTO_BYNAME_INFO CopyInfo;
#if USE_FILEDB
	FILEDB_HANDLE fileDbHandle, fileDbbufSize = 0x200000, fileDbbufAddr;
#endif
#if (USE_DCF == ENABLE)
	UINT32 uiThisFileFormat;
	UINT32 DirID, FileID;
	UINT32 NewDirID, NewFileID, i;
	CHAR   srcPath[DCF_FULL_FILE_PATH_LEN], dstPath[DCF_FULL_FILE_PATH_LEN];
#endif


	uiTempBufAddr = SxCmd_GetTempMem(POOL_SIZE_APP);
	uiTempBufSize = POOL_SIZE_APP;

	// reserved 2MB for FileDB using
#if USE_FILEDB
	uiTempBufSize -= fileDbbufSize;
	fileDbbufAddr = uiTempBufAddr + uiTempBufSize;
#endif

	UI_SetData(FL_IsCopyToCarding, TRUE);

	if (IsPrimaryCard) {
		FileSys_GetStrgObj(&pDstDXH);
		pOriDXH = pDstDXH;
		{
			pSrcDXH = (FS_HANDLE)Dx_GetObject(DX_CLASS_STORAGE_EXT | DX_TYPE_CARD2);
		}
	} else {
		FileSys_GetStrgObj(&pOriDXH);
		pSrcDXH = pOriDXH;
		{
			pDstDXH = (FS_HANDLE)Dx_GetObject(DX_CLASS_STORAGE_EXT | DX_TYPE_CARD2);
		}
	}

	memset(&CopyInfo, 0, sizeof(CopyInfo));
	CopyInfo.pSrcStrg = pSrcDXH;
	CopyInfo.pDstStrg = pDstDXH;
	CopyInfo.pBuf = (char *)uiTempBufAddr;
	CopyInfo.uiBufSize = uiTempBufSize;

#if (USE_FILEDB == ENABLE)
	{
		if (pOriDXH != pSrcDXH) {
			if (FileSys_ChangeDisk(pSrcDXH) != FSS_OK) {
				DBG_ERR("ChangeDisk fail\r\n");
			}
		}
		//  create fileDB for copy
		{
			FILEDB_INIT_OBJ   FilDBInitObj = {0};
			CHAR              rootPath[20] = "A:\\";

			FilDBInitObj.rootPath = rootPath;
			FilDBInitObj.bIsRecursive = TRUE;
			FilDBInitObj.bIsCyclic = TRUE;
			FilDBInitObj.bIsMoveToLastFile = TRUE;
			FilDBInitObj.bIsSupportLongName = TRUE;
			FilDBInitObj.bIsDCFFileOnly = FALSE;
			FilDBInitObj.bIsChkHasFile = FALSE;
			FilDBInitObj.u32MaxFilePathLen = 60;
			FilDBInitObj.u32MaxFileNum = 5000;
			FilDBInitObj.fileFilter = FILEDB_FMT_JPG | FILEDB_FMT_AVI | FILEDB_FMT_MOV | FILEDB_FMT_MP4 | FILEDB_FMT_TS;
			FilDBInitObj.u32MemAddr = dma_getCacheAddr(fileDbbufAddr);
			FilDBInitObj.u32MemSize = fileDbbufSize;
			fileDbHandle = FileDB_Create(&FilDBInitObj);
		}
		// copy file one by one
		{
			UINT32                   i = 0, CurrIndex;
			PFILEDB_FILE_ATTR        pFileAttr;

			CurrIndex = FileDB_GetCurrFileIndex(fileDbHandle);
			FileNumber = FileDB_GetTotalFileNum(fileDbHandle);
			for (i = 0; i < FileNumber; i++) {
				pFileAttr = FileDB_SearhFile(fileDbHandle, i);
				CopyInfo.pSrcPath = pFileAttr->filePath;
				CopyInfo.pDstPath = pFileAttr->filePath;
				debug_msg("Copy file %s\r\n", pFileAttr->filePath);
				uiError_Code = FileSys_CopyToByName(&CopyInfo);

				if (uiError_Code != FST_STA_OK) {
					if (uiError_Code == FST_STA_NOFREE_SPACE) {
						uiError_Code = WARN_CARD_FULL;
					}
					break;
				}
			}
			if (FileNumber) {
				FileDB_SearhFile(fileDbHandle, CurrIndex);
			}
		}
		FileDB_Release(fileDbHandle);
	}
#endif
#if (USE_DCF == ENABLE)
	{
		//if (pOriObj==pSrcObj)
		if (pOriDXH == pSrcDXH) {
			// get copy file number
			FileNumber = DCF_GetDBInfo(DCF_INFO_TOL_FILE_COUNT);

			// get dst max ID
			//FileSys_ChangeDisk(pDstObj);
			if (FileSys_ChangeDisk(pDstDXH) != FSS_OK) {
				DBG_ERR("ChangeDisk fail\r\n");
			}
			DCF_ScanObj();
			DCF_GetNextID(&NewDirID, &NewFileID);
			//FileSys_ChangeDisk(pOriObj);
			if (FileSys_ChangeDisk(pOriDXH) != FSS_OK) {
				DBG_ERR("ChangeDisk fail\r\n");
			}
			DCF_ScanObj();
		}
		// pOriObj == pDstObj
		else {
			// get dst max ID
#if 0
			DCF_GetNextID(&NewDirID, &NewFileID);
#else // Rescan DCF if not scanned yet.
			if (!DCF_GetNextID(&NewDirID, &NewFileID)) {
				DCF_ScanObj();
				DCF_GetNextID(&NewDirID, &NewFileID);
			}
#endif
			// get copy file number
			//FileSys_ChangeDisk(pSrcObj);
			if (FileSys_ChangeDisk(pSrcDXH) != FSS_OK) {
				DBG_ERR("ChangeDisk fail\r\n");
			}
			DCF_ScanObj();

			FileNumber = DCF_GetDBInfo(DCF_INFO_TOL_FILE_COUNT);
		}
		if (NewDirID == ERR_DCF_DIR_NUM && NewFileID == ERR_DCF_FILE_NUM) {
			uiError_Code = WRN_EXCEED_MAX_FOLDER;
			goto L_CopyToCard_Exit;
		}
		switch (copMode) {
		case PB_COPY2CARD_NEW_DIR:
			if (NewFileID != MIN_DCF_FILE_NUM) {
				NewDirID++;
				NewFileID = MIN_DCF_FILE_NUM;
				if (NewDirID > MAX_DCF_DIR_NUM) {
					uiError_Code = WRN_EXCEED_MAX_FOLDER;
					goto L_CopyToCard_Exit;
				}
			}
		// continue run append code should not add break here
		case PB_COPY2CARD_APPEND:
			for (i = 1; i <= FileNumber; i++) {
				// get src path
				DCF_SetCurIndex(i);
				DCF_GetObjInfo(i, &DirID, &FileID, &uiThisFileFormat);
				DCF_GetObjPath(i, uiThisFileFormat, srcPath);
				CopyInfo.pSrcPath = srcPath;

				// get dst path
				DCF_MakeObjPath(NewDirID, NewFileID, uiThisFileFormat, dstPath);
				CopyInfo.pDstPath = dstPath;
				uiError_Code = FileSys_CopyToByName(&CopyInfo);
				if (uiError_Code != FST_STA_OK) {
					if (uiError_Code == FST_STA_NOFREE_SPACE) {
						uiError_Code = WARN_CARD_FULL;
					}
					break;
				}
				NewFileID++;
				if (NewFileID > MAX_DCF_FILE_NUM) {
					NewFileID = 1;
					NewDirID++;
					if (NewDirID > MAX_DCF_DIR_NUM) {
						uiError_Code = WRN_EXCEED_MAX_FOLDER;
						goto L_CopyToCard_Exit;
					}
				}
			}
			break;

		case PB_COPY2CARD_NEW_DIR_ORG_FILE: {
				UINT32 tmpDirID  = 0;

				// Fix bug 0050970: Show no picture warning message if no file in Nand when doing copy to card function.
				if (!FileNumber) {
					uiError_Code = WARN_COPY_NO_PICTURE ;
					goto L_CopyToCard_Exit;
				}

				for (i = 1; i <= FileNumber; i++) {
					// get src path
					DCF_SetCurIndex(i);
					DCF_GetObjInfo(i, &DirID, &FileID, &uiThisFileFormat);
					DCF_GetObjPath(i, uiThisFileFormat, srcPath);
					CopyInfo.pSrcPath = srcPath;

					// check if need to new dir
					if (tmpDirID != DirID) {
						tmpDirID = DirID;
						NewDirID++;
						if (NewDirID > MAX_DCF_DIR_NUM) {
							uiError_Code = WRN_EXCEED_MAX_FOLDER;
							goto L_CopyToCard_Exit;
						}
					}

					// get dst path
					DCF_MakeObjPath(NewDirID, FileID, uiThisFileFormat, dstPath);
					CopyInfo.pDstPath = dstPath;
					uiError_Code = FileSys_CopyToByName(&CopyInfo);
					if (uiError_Code != FST_STA_OK) {
						if (uiError_Code == FST_STA_NOFREE_SPACE) {
							uiError_Code = WARN_CARD_FULL;
						}

						break;
					}
				}
			}
			break;
		case PB_COPY2CARD_NO_RENAME:

		default:
			debug_msg("Not Support Copy command %d\r\n", copMode);
			break;
		}
	}
L_CopyToCard_Exit:
#endif

	if (pOriDXH != pSrcDXH) {
		if (FileSys_ChangeDisk(pOriDXH) != FSS_OK) {
			DBG_ERR("ChangeDisk fail\r\n");
		}
	}

#if (USE_FILEDB == ENABLE)
	{
		UINT32  curIndex;

		fileDbHandle = 0;
		FileDB_Refresh(fileDbHandle);
		FileNumber = FileDB_GetTotalFileNum(fileDbHandle);

		curIndex = FileNumber - 1;
		if (FileNumber) {
			FileDB_SearhFile(fileDbHandle, curIndex);
		}
#if (PLAY_MODE==ENABLE)
		if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PLAYBACK) {
			PB_OpenSpecFileBySeq(curIndex + 1, TRUE);
			PB_WaitCommandFinish(PB_WAIT_INFINITE);
		}
#endif
	}
#endif
#if (USE_DCF == ENABLE)
	{
		DCF_Refresh();
		DCF_SetCurIndex(DCF_GetDBInfo(DCF_INFO_TOL_FILE_COUNT));
#if (PLAY_MODE==ENABLE)
		if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PLAYBACK) {
			PB_OpenSpecFileBySeq(DCF_GetDBInfo(DCF_INFO_CUR_INDEX), TRUE);
			PB_WaitCommandFinish(PB_WAIT_INFINITE);
		}
#endif
	}
#endif

	UI_SetData(FL_IsCopyToCarding, FALSE);
	SxCmd_RelTempMem(uiTempBufAddr);
	return uiError_Code;
}

UINT32 BackgroundCopyCard1ToCard2(void)
{
	return Background_CopyCardToCard(FALSE, PB_COPY2CARD_NEW_DIR);
}

UINT32 BackgroundCopyCard2ToCard1(void)
{
	return Background_CopyCardToCard(TRUE, PB_COPY2CARD_NEW_DIR);
}
#endif


UINT32 BackgroundFormat(void)
{
	int ret=0;

	FS_HANDLE   pStrgDXH = 0;
    DBG_FUNC_BEGIN("\r\n");
	FileSys_GetStrgObj(&pStrgDXH);
	//#NT#2016/05/30#Lincy Lin -begin
	//#NT#Support logfile function
#if (LOGFILE_FUNC==ENABLE)
	LogFile_Suspend();
	LogFile_Complete();
#endif
	//#NT#2016/05/30#Lincy Lin -end
	ret = FileSys_FormatDisk(pStrgDXH, FALSE);
	DBG_FUNC("Call FileSys_FormatDisk() ret=%d\r\n", ret);
	if (ret == FST_STA_OK) {
		// reset file ID (for FileDB)
#if USE_FILEDB
		if (UI_GetData(FL_IsUseFileDB)) {
			Ux_SendEvent(&UISetupObjCtrl, NVTEVT_FILEID_RESET, 0);
		}
#endif
#if (USE_DCF == ENABLE)
		//reset next id 100 , 1
		DCF_SetNextID(MIN_DCF_DIR_NUM, MIN_DCF_FILE_NUM);
		UI_SetData(FL_DCF_DIR_ID, MIN_DCF_DIR_NUM);
		UI_SetData(FL_DCF_FILE_ID, MIN_DCF_FILE_NUM);

		UI_SetData(FL_IsDCIMReadOnly, FALSE);
#endif
#if (PLAY_MODE==ENABLE)
		if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PLAYBACK) {
			//UIPlay_PlaySingle(PB_SINGLE_CURR);
		}
#endif
		//#NT#2016/05/30#Lincy Lin -begin
		//#NT#Support logfile function
#if (LOGFILE_FUNC==ENABLE)
		LogFile_ReOpen();
#endif
		//#NT#2016/05/30#Lincy Lin -end
	}
    #if 1 //To do
    DBG_ERR("To Do Setup_Menu Call MovieExe_ResetFileSN()\r\n");
    #else //To do
	MovieExe_ResetFileSN();
    #endif//To do
    DBG_FUNC_END("\r\n");
	return (UINT32)ret;
}

UINT32 BackgroundFormatCard(void)
{
	int ret;
    DBG_FUNC_BEGIN("\r\n");
	//DX_HANDLE pStrgDev = Dx_GetObject(DX_CLASS_STORAGE_EXT|DX_TYPE_CARD1);
	//UINT32 hStrgObj = Dx_Getcaps(pStrgDev, STORAGE_CAPS_HANDLE, 0);
	//ret = FileSys_FormatDisk((HNVT_STRG)hStrgObj, FALSE);
	FS_HANDLE pStrgDXH = (FS_HANDLE)Dx_GetObject(DX_CLASS_STORAGE_EXT | DX_TYPE_CARD1);
    //FS_HANDLE pStrgDXH = (FS_HANDLE)sdio_getStorageObject(STRG_OBJ_FAT1);
    DBG_FUNC("pStrgDXH=0x%08X\r\n",pStrgDXH);
	//#NT#2016/05/30#Lincy Lin -begin
	//#NT#Support logfile function
#if (LOGFILE_FUNC==ENABLE)
	LogFile_Suspend();
	LogFile_Complete();
#endif
	//#NT#2016/05/30#Lincy Lin -end
	ret = FileSys_FormatDisk(pStrgDXH, FALSE);
	DBG_FUNC("Call FileSys_FormatDisk() ret=%d\r\n", ret);

	if (ret == FST_STA_OK) {
		// reset file ID (for FileDB)
#if USE_FILEDB
		if (UI_GetData(FL_IsUseFileDB)) {
			Ux_SendEvent(&UISetupObjCtrl, NVTEVT_FILEID_RESET, 0);
		}
#endif
#if (USE_DCF == ENABLE)
		//reset next id 100 , 1
		DCF_SetNextID(MIN_DCF_DIR_NUM, MIN_DCF_FILE_NUM);
		UI_SetData(FL_DCF_DIR_ID, MIN_DCF_DIR_NUM);
		UI_SetData(FL_DCF_FILE_ID, MIN_DCF_FILE_NUM);
		UI_SetData(FL_IsDCIMReadOnly, FALSE);
#endif
#if (PLAY_MODE==ENABLE)
		if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PLAYBACK) {
			UIPlay_PlaySingle(PB_SINGLE_CURR);
		}
#endif
		//#NT#2016/05/30#Lincy Lin -begin
		//#NT#Support logfile function
#if (LOGFILE_FUNC==ENABLE)
		LogFile_ReOpen();
#endif
		//#NT#2016/05/30#Lincy Lin -end
		vos_util_delay_ms(1000);
        SysMain_system("sync");
	}
	MovieExe_ResetFileSN();
    DBG_FUNC_END("\r\n");
	return (UINT32)ret;
}
UINT32 BackgroundFormatNand(void)
{
	int ret;
    DBG_FUNC_BEGIN("\r\n");
	//DX_HANDLE pStrgDev = Dx_GetObject(DX_CLASS_STORAGE_EXT|DX_TYPE_EMBMEM1);
	//UINT32 hStrgObj = Dx_Getcaps(pStrgDev, STORAGE_CAPS_HANDLE, 0);
	//ret = FileSys_FormatDisk((HNVT_STRG)hStrgObj, FALSE);
	FS_HANDLE pStrgDXH = (FS_HANDLE)Dx_GetObject(DX_CLASS_STORAGE_EXT | USER_DX_TYPE_EMBMEM_FAT);
    //FS_HANDLE pStrgDXH = (FS_HANDLE)nand_getStorageObject(STRG_OBJ_FAT1);
	//#NT#2016/05/30#Lincy Lin -begin
	//#NT#Support logfile function
#if (LOGFILE_FUNC==ENABLE)
	LogFile_Suspend();
	LogFile_Complete();
#endif
	//#NT#2016/05/30#Lincy Lin -end
	ret = FileSys_FormatDisk(pStrgDXH, FALSE);
	DBG_FUNC("call FileSys_FormatDisk() ret=%d\r\n", ret);

	if (ret == FST_STA_OK) {
		// reset file ID (for FileDB)
#if USE_FILEDB
		if (UI_GetData(FL_IsUseFileDB)) {
			Ux_SendEvent(&UISetupObjCtrl, NVTEVT_FILEID_RESET, 0);
		}
#endif
#if (USE_DCF == ENABLE)
		//reset next id 100 , 1
		DCF_SetNextID(MIN_DCF_DIR_NUM, MIN_DCF_FILE_NUM);
		UI_SetData(FL_DCF_DIR_ID, MIN_DCF_DIR_NUM);
		UI_SetData(FL_DCF_FILE_ID, MIN_DCF_FILE_NUM);
		UI_SetData(FL_IsDCIMReadOnly, FALSE);
#endif
#if (PLAY_MODE==ENABLE)
		if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PLAYBACK) {
			UIPlay_PlaySingle(PB_SINGLE_CURR);
		}
#endif
		//#NT#2016/05/30#Lincy Lin -begin
		//#NT#Support logfile function
#if (LOGFILE_FUNC==ENABLE)
		LogFile_ReOpen();
#endif
		//#NT#2016/05/30#Lincy Lin -end
	}
    MovieExe_ResetFileSN();
   DBG_FUNC("call FileSys_FormatDisk() ret=%d\r\n", ret);
	return (UINT32)ret;
}

UINT32 BackgroundNumReset(void)
{
    DBG_FUNC_BEGIN("\r\n");
#if (USE_DCF == ENABLE)
	UINT32 fid = 0, did = 0;

	//#Get DCF Folder/FIle ID
	DCF_GetNextID((UINT32 *)&did, (UINT32 *)&fid);
	DBG_ERR("SetupExe_OnNumReset(), DirNum = %d, FileNum = %d\r\n", did, fid);

	if (did < MAX_DCF_DIR_NUM) {
		//reset
		DCF_SetNextID(did + 1, 1);

		//#Get DCF Folder/FIle ID
		DCF_GetNextID((UINT32 *)&did, (UINT32 *)&fid);

		SwTimer_DelayMs(0x2000); //for ui show wait window for 2 second
		DBG_ERR("SetupExe_OnNumReset(), DirNum = %d, FileNum = %d\r\n", did, fid);
        DBG_FUNC_END(" ret TRUE\r\n");
		return TRUE;
	} else {
		DBG_ERR("did over %d\r\n", MAX_DCF_DIR_NUM);
        DBG_FUNC_END(" ret FALSE\r\n");
		return FALSE;
	}
#else
    DBG_FUNC_END(" ret TRUE\r\n");
	return TRUE;
#endif
}
#if (USE_DPOF == ENABLE)
UINT32 BackgroundSetDPOF(void)
{
	INT32 ret;
    DBG_FUNC_BEGIN("\r\n");
	if (g_uiDpofOPMode == PLAYDPOF_SETONE)
		//Ux_SendEvent(&UIPlayObjCtrl,NVTEVT_SET_DPOFONE,2, g_uiDpofPrtNum, g_uiDpofDateOn);
	{
		DpofVendor_SetDPOFQty(DPOF_OP_SET_CURR, g_uiDpofPrtNum, g_uiDpofDateOn);
	} else {
		//#NT#2010/05/31#[0010985]Lily Kao -begin
		//#NT#DPOF-All from the current or the first picture
		//Ux_SendEvent(&UIPlayObjCtrl,NVTEVT_SET_DPOFALL_FROM_FIRST,2, g_uiDpofPrtNum, g_uiDpofDateOn);
		//#NT#2010/05/31#[0010985]Lily Kao -end
		DpofVendor_SetDPOFQty(DPOF_OP_SET_ALL, g_uiDpofPrtNum, g_uiDpofDateOn);
	}
	ret = TRUE;//dummy return value
	DBG_FUNC_END(" ret TRUE\r\n");
	return (UINT32)ret;
}
#endif
UINT32 BackgroundSetProtect(void)
{
        DBG_FUNC_BEGIN("\r\n");
#if (PLAY_MODE==ENABLE)
	if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PLAYBACK) {
		INT32 ret;
		UIPlay_Protect(g_uiProtectType);
		ret = TRUE;//dummy return value
		return (UINT32)ret;
	} else
#endif
	{
        DBG_FUNC("g_uiProtectType=%d\r\n",g_uiProtectType);
		if (g_uiProtectType == SETUP_PROTECT_ALL) {
            DBG_FUNC_END("ret Background_ProtectAll(TRUE)\r\n");
			return Background_ProtectAll(TRUE);
		} else {
		    DBG_FUNC_END("ret Background_ProtectAll(FALSE)\r\n");
			return Background_ProtectAll(FALSE);
		}
	}
}

UINT32 Background_ProtectAll(BOOL bLock)
{
    DBG_FUNC_BEGIN("\r\n");
#if (USE_FILEDB == ENABLE)
	FILEDB_HANDLE      FileDBHdl = 0;
	PFILEDB_FILE_ATTR  FileAttr = NULL;
	INT32              ret;
	INT32              FileNum, i;
	PFILEDB_INIT_OBJ   pFDBInitObj = &gBKFDBInitObj;

	pFDBInitObj->u32MemAddr = SxCmd_GetTempMem(POOL_SIZE_FILEDB);

	if (!pFDBInitObj->u32MemAddr) {
		return FALSE;
	}

	pFDBInitObj->u32MemSize = POOL_SIZE_FILEDB;
	FileDBHdl = FileDB_Create(pFDBInitObj);
	//FileDB_DumpInfo(FileDBHdl);
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
	FileDB_Release(FileDBHdl);
    SxCmd_RelTempMem(pFDBInitObj->u32MemAddr);
	if (UI_GetData(FL_IsUseFileDB)) {
		Ux_SendEvent(&UISetupObjCtrl, NVTEVT_FILEID_RESET, 0);
	}
	return E_OK;

FDB_Protect_Err:
	DBG_ERR("bLock=%d err\r\n", bLock);
	FileDB_Release(FileDBHdl);
    SxCmd_RelTempMem(pFDBInitObj->u32MemAddr);
	if (UI_GetData(FL_IsUseFileDB)) {
		Ux_SendEvent(&UISetupObjCtrl, NVTEVT_FILEID_RESET, 0);
	}
    DBG_FUNC_END("\r\n");
	return FALSE;
#else
	DBG_ERR("not support!\r\n");
    DBG_FUNC_END("\r\n");
	return FALSE;
#endif
}

UINT32 BackgroundDummyWait_Stop(void)
{
    DBG_FUNC_BEGIN("\r\n");
	DBG_ERR("BackgroundDummyWait_Stop:%d,%d\r\n", gUIStopBckGndDummyWait, g_uiWaitTime);
	if (g_uiWaitTime) {
		gUIStopBckGndDummyWait = TRUE;
	}
    DBG_FUNC_END("ret g_uiWaitTime=%d\r\n",g_uiWaitTime);
	return g_uiWaitTime;
}
//#NT#2010/06/23#Lily Kao -begin
UINT32 BackgroundDummyWait(void)
{
	UINT32 i = g_uiWaitTime;
    DBG_FUNC_BEGIN("\r\n");
	gUIStopBckGndDummyWait = FALSE;
	while (i) {
		//debug_msg("BackgroundDummyWait:%d,%d,%d\r\n", gUIStopBckGndDummyWait, g_uiWaitTime,i);
		Delay_DelayMs(1);
		i--;
		if (gUIStopBckGndDummyWait == TRUE) {
			//debug_msg("BackgroundDummyWait break:%d,%d\r\n", gUIStopBckGndDummyWait, g_uiWaitTime);
			gUIStopBckGndDummyWait = FALSE;
			g_uiWaitTime = 0;
			break;
		}
	}
    DBG_FUNC_END("\r\n");
	return 1;
}

extern UINT32   System_OnStrg_UploadFW(UINT32 uiStorage);
//static HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
static UINT32 BackgroundFWUpdate(void)
{
    DBG_FUNC_BEGIN("\r\n");
#if (IPCAM_FUNC != ENABLE)
	UINT32 result = 0;
#if (UPDFW_MODE == ENABLE)
	INT curMode = System_GetState(SYS_STATE_CURRMODE) ;

	System_ChangeSubMode(SYS_SUBMODE_UPDFW);
	Ux_SendEvent(0, NVTEVT_SYSTEM_MODE, 1, PRIMARY_MODE_UPDFW);
    //DBG_ERR("To Do call System_OnStrg_UploadFW()\r\n");
    // config mem pool for filein
    /*HD_RESULT hd_result = HD_OK;
	mem_cfg.pool_info[0].type       = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size   = 16 * 1024 * 1024;  // default at least 16MBytes for filein bitstream block.
	mem_cfg.pool_info[0].blk_cnt    = 1;
	mem_cfg.pool_info[0].ddr_id     = DDR_ID0;
    if ((hd_result = hd_common_mem_init(&mem_cfg)) != HD_OK) {
		DBG_ERR("hd_common_mem_init fail(%d)\r\n", hd_result);
    }*/

	result = System_OnStrg_UploadFW(0);
    if (result != 0)
        DBG_ERR("System_OnStrg_UploadFW() fail(%d)\r\n",result);

    /*if ((hd_result = hd_common_mem_uninit()) != HD_OK) {
		DBG_ERR("hd_common_mem_uninit fail(%d)\r\n", hd_result);
	}*/

	if (result != 0) {
		Ux_SendEvent(0, NVTEVT_SYSTEM_MODE, 1, curMode);
		System_ChangeSubMode(SYS_SUBMODE_NORMAL);

	}
#endif
#if (defined(_NVT_ETHREARCAM_TX_) || defined(_NVT_ETHREARCAM_RX_))
	if(sEthCamFwUd.FwAddr && sEthCamFwUd.FwSize ){
		if (result != 0) {
			EthCam_SendXMLStatus(sEthCamFwUd.path_id,sEthCamFwUd.port_type,sEthCamFwUd.cmd, result + ETHCAM_RET_FW_OFFSET);
		}else{
			EthCam_SendXMLStatus(sEthCamFwUd.path_id,sEthCamFwUd.port_type,sEthCamFwUd.cmd, ETHCAM_RET_FW_OK);
			Delay_DelayMs(2000);
			// reboot
			GxPower_SetPwrKey(GXPWR_KEY_SWRESET, 0xff);
			GxPower_Close();
			System_PowerOff(SYS_POWEROFF_NORMAL);
		}
	}
#endif
    DBG_FUNC_END("\r\n");
	return result;
#else
	debug_msg("_TODO -> %s:%d, task(%x,%s) caller(%pF)\r\n", __FUNCTION__, __LINE__,
			  OS_GetTaskID(), OS_GetTaskName(), __builtin_return_address(0));

	return 0;
#endif
}

#if (PHOTO_MODE==ENABLE)
extern void PhotoExe_DZoomInBK(void);
extern void PhotoExe_DZoomOutBK(void);

static UINT32 BackgroundDZoomIn(void)
{
	PhotoExe_DZoomInBK();
	return TRUE;
}

static UINT32 BackgroundDZoomOut(void)
{
	PhotoExe_DZoomOutBK();
	return TRUE;
}
#endif

//#NT#2012/10/23#Hideo Lin -begin
//#NT#For picture in movie processing
static UINT32 BackgroundPIMProcess(void)
{
	Ux_SendEvent(&CustomMovieObjCtrl, NVTEVT_EXE_MOVIE_REC_STOP, 0);
	return NVTRET_OK;
}
//#NT#2012/10/23#Hideo Lin -end

//#NT#2012/10/30#Calvin Chang#Generate Audio NR pattern by noises of zoom, focus and iris -begin
static UINT32 BackgroundAudiNRLensAction(void)
{
#if (LENS_FUNCTION == ENABLE)
	UINT32 i, uiLoop;

	Lens_ZoomSetSpeed(ZOOM_SPEED_LOW);

	Delay_DelayMs(3000); // Wait 2 seconds

	// 1. Zoom In/out + Focus in 3 seconds
	debug_err(("Zoom In/out + Focus in 3 seconds!\r\n"));
	uiLoop = 1;
	for (i = 0; i < uiLoop; i++) {
		Lens_ZoomIn();
		Lens_FocusDoAction(50); // position = 45 ~ 600;
		Lens_FocusDoAction(600); // position = 45 ~ 600;
		Delay_DelayMs(150);
		Lens_ZoomStop();
		Delay_DelayMs(25);
		Lens_ZoomOut();
		Lens_FocusDoAction(50); // position = 45 ~ 600;
		Lens_FocusDoAction(600); // position = 45 ~ 600;
		Delay_DelayMs(150);
		Lens_ZoomStop();
		Delay_DelayMs(25);
	}

	Delay_DelayMs(6000); // Wait 6 seconds

	//2. Focus Forward N steps in 3 seconds
	debug_err(("Focus Forward N steps in 3 seconds!\r\n"));
	uiLoop = 6;
	for (i = 0; i < uiLoop; i++) {
		Lens_FocusDoAction(50); // position = 45 ~ 600;
		Lens_FocusDoAction(450); // position = 45 ~ 600;

	}

	Delay_DelayMs(7000); // Wait 7 seconds

	//3. Iris switch in 3 seconds
	debug_err(("Iris switch in 3 seconds!\r\n"));
	uiLoop = 15;
	for (i = 0; i < uiLoop; i++) {
		Lens_ApertureSetPosition(IRIS_POS_SMALL);
		Delay_DelayMs(50);
		Lens_ApertureSetPosition(IRIS_POS_BIG);
		Delay_DelayMs(50);
	}

	Delay_DelayMs(4000);

	Lens_ZoomSetSpeed(ZOOM_SPEED_HIGH);

	// Record stop!
	Ux_SendEvent(&CustomMovieObjCtrl, NVTEVT_EXE_MOVIE_REC_STOP, 0);

	debug_err(("Record Stop!\r\n"));
#endif

	return NVTRET_OK;
}
//#NT#2012/10/30#Calvin Chang -end
static UINT32 BackgroundWiFiOn(void)
{
#if (WIFI_FUNC == ENABLE)
	Ux_SendEvent(0, NVTEVT_EXE_WIFI_START, 0);
#endif
	return NVTRET_OK;
}
#if(WIFI_AP_FUNC==ENABLE)

static UINT32 BackgroundWiFiClearACL(void)
{
	Delay_DelayMs(ACL_TIME);
	UINet_ClearACLTable();
	return NVTRET_OK;
}
#endif
static UINT32 BackgroundStopRecProcess(void)
{
	//debug_err(("StopRec\r\n\n\n\n\n"));
    #if defined(_UI_STYLE_LVGL_)
	Ux_SendEvent(&CustomMovieObjCtrl, NVTEVT_EXE_MOVIE_REC_STOP, 0);
    #else
	FlowMovie_StopRec();
    #endif
    return NVTRET_OK;
}

#if (defined(_NVT_ETHREARCAM_TX_) || defined(_NVT_ETHREARCAM_RX_))
static UINT32 BackgroundEthCamSocketOpen(void)
{
#if (defined(_NVT_ETHREARCAM_TX_))
	socketEthCmd_Open();

	while(MovieExe_GetCommonMemInitFinish()==0){
		Delay_DelayMs(5);
	}

	socketEthData_Open(ETHSOCKIPC_ID_0);
#if 1//(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
	socketEthData_Open(ETHSOCKIPC_ID_1);
#endif
#endif
	return NVTRET_OK;
}

static UINT32 BackgroundEthCamSocketCliCmdOpen(void)
{
#if (defined(_NVT_ETHREARCAM_RX_))
	UINT32 i;

	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		//DBG_ERR("LinkStatus[%d]=%d, %d\r\n",i,EthCamNet_GetEthLinkStatus(i),EthCamNet_GetPrevEthLinkStatus(i));
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
			socketCliEthCmd_Open(i);
		}
	}
#endif
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamSocketCliDataOpen(void)
{
#if (defined(_NVT_ETHREARCAM_RX_))
	UINT32 i;
	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		if(socketCliEthCmd_IsConn(i) && sEthCamTxDecInfo[i].bStarupOK && EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			socketCliEthData_Open(i, ETHSOCKIPCCLI_ID_1);
#endif
			socketCliEthData_Open(i, ETHSOCKIPCCLI_ID_0);
		}
	}
#endif
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamTxDecInfo(void)
{
#if(defined(_NVT_ETHREARCAM_RX_))
	ETHCAM_PORT_TYPE port_type=ETHCAM_PORT_DATA1;
	UINT32 i;//, j=0;
	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
			#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			port_type =ETHCAM_PORT_DATA2;
			#else
			port_type =ETHCAM_PORT_DATA1;
			#endif

#if 0
	MOVIEMULTI_IME_CROP_INFO CropInfo ={0};
	CropInfo.IMESize.w = 1280;
	CropInfo.IMESize.h = 720;
	CropInfo.IMEWin.x = 0;
	CropInfo.IMEWin.w = 1280;
	CropInfo.IMEWin.h = 320;
	CropInfo.IMEWin.y = 200;
	DBG_DUMP("CropInfo.IMEWin.y=%d\r\n",CropInfo.IMEWin.y);
	EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_TX_DISP_CROP, 0);
	EthCam_SendXMLData(i, (UINT8 *)&CropInfo, (UINT32)sizeof(MOVIEMULTI_IME_CROP_INFO));
#endif


			EthCam_SendXMLCmd(i, port_type ,ETHCAM_CMD_GET_TX_DEC_INFO, 0);
			//DBG_DUMP("i=%d, dec_info, fps=%d, tbr=%d,w=%d, h=%d, GOP=%d, Codec=%d\r\n",i,sEthCamTxDecInfo[i].Fps,sEthCamTxDecInfo[i].Tbr,sEthCamTxDecInfo[i].Width ,sEthCamTxDecInfo[i].Height,sEthCamTxDecInfo[i].Gop,sEthCamTxDecInfo[i].Codec);
			EthCam_SendXMLCmd(i, port_type ,ETHCAM_CMD_GET_TX_SPS_PPS, 0);


			EthCamSocketCli_SetDescSize(i, ETHSOCKIPCCLI_ID_1,  sEthCamTxDecInfo[i].DescSize);
			sEthCamTxDecInfo[i].bStarupOK=1;
			//get tx dec info
			MOVIEMULTI_ETHCAM_DEC_INFO dec_info={0};

			dec_info.width = sEthCamTxDecInfo[i].Width;
			dec_info.height = sEthCamTxDecInfo[i].Height;
			//dec_info.gop = sEthCamTxDecInfo[i].Gop;
			dec_info.codec = sEthCamTxDecInfo[i].Codec;
			//DBG_ERR("################Desc=0x%x, DescSize=%d,%d\r\n",sEthCamTxDecInfo[i].Desc,sEthCamTxDecInfo[i].DescSize);
			//DBG_ERR("################Desc=0x%x, DescSize=%d,%d\r\n",sEthCamTxDecInfo[i].Desc,sEthCamTxDecInfo[i].DescSize);
			DBG_DUMP("1Desc=0x%x, DescSize=%d ,%d\r\n",sEthCamTxDecInfo[i].Desc,sEthCamTxDecInfo[i].DescSize,dec_info.header_size);

			dec_info.header_size = sEthCamTxDecInfo[i].DescSize;
			DBG_DUMP("2Desc=0x%x, DescSize=%d ,%d\r\n",sEthCamTxDecInfo[i].Desc,sEthCamTxDecInfo[i].DescSize,dec_info.header_size);

			dec_info.header = sEthCamTxDecInfo[i].Desc;
			//DBG_ERR("i=%d, dec_info, w=%d, h=%d, GOP=%d, Codec=%d\r\n",i,dec_info.width ,dec_info.height,dec_info.gop,dec_info.codec);
			//DBG_ERR("dec_info, w=%d, h=%d, GOP=%d, Codec=%d\r\n",dec_info.width ,dec_info.height,dec_info.gop,dec_info.codec);
			//DBG_ERR("dec_info, w=%d, h=%d, GOP=%d, Codec=%d\r\n",dec_info.width ,dec_info.height,dec_info.gop,dec_info.codec);
			ImageApp_MovieMulti_SetParam(_CFG_ETHCAM_ID_1+i, MOVIEMULTI_PRARM_ETHCAM_DEC_INFO, (UINT32)&dec_info);
			//ImageApp_MovieMulti_EthCamLinkForDisp(_CFG_ETHCAM_ID_1, ENABLE, TRUE);

			#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			socketCliEthData2_ConfigRecvBuf(i);
			#else
			socketCliEthData1_ConfigRecvBuf(i);
			#endif
		}
	}
#endif
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamTxRecInfo(void)
{
#if(defined(_NVT_ETHREARCAM_RX_))
	UINT32 i;
	UINT32 addrPa=0, sizePa=0;
	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_GET_TX_REC_INFO, 0);

			EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_GET_TX_SPS_PPS, 0);
			EthCamSocketCli_SetDescSize(i, ETHSOCKIPCCLI_ID_0,  sEthCamTxRecInfo[i].DescSize);

			DBG_DUMP("rec_format[%d]=%d\r\n",i,sEthCamTxRecInfo[i].rec_format);
			#if 0 //temp
			MOVIEMULTI_ETHCAM_REC_INFO EthCamRecInfo={0};
			memcpy(&EthCamRecInfo, &sEthCamTxRecInfo[i], sizeof(MOVIEMULTI_ETHCAM_REC_INFO));

			EthCamRecInfo.rec_format=_CFG_FILE_FORMAT_MP4;
			ImageApp_MovieMulti_SetParam(_CFG_ETHCAM_ID_1+i, MOVIEMULTI_PRARM_ETHCAM_REC_INFO, (UINT32)&EthCamRecInfo);
			#endif
			#if 1//(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			socketCliEthData1_ConfigRecvBuf(i);
			#endif
			socketCliEthData1_GetBsBufInfo(i, &addrPa, &sizePa);
			sEthCamTxRecInfo[i].aud_codec=gEthcamRecInfoAudCodec;//ETHCAM_AUDIO_FORMAT;
			sEthCamTxRecInfo[i].rec_format=gEthcamRecInfoRecFormat;//ETHCAM_FILE_FORMAT;
			MOVIEMULTI_ETHCAM_REC_INFO EthCamRecInfo = {
				.width = sEthCamTxRecInfo[i].width,
				.height = sEthCamTxRecInfo[i].height,
				.vfr = sEthCamTxRecInfo[i].vfr,
				.tbr = sEthCamTxRecInfo[i].tbr,
				.ar = sEthCamTxRecInfo[i].ar,
				.gop = sEthCamTxRecInfo[i].gop,
				.codec = (sEthCamTxRecInfo[i].codec == HD_CODEC_TYPE_H265)?_CFG_CODEC_H265: _CFG_CODEC_H264,//_CFG_CODEC_H265,
				.aud_codec = sEthCamTxRecInfo[i].aud_codec,
				.rec_mode = sEthCamTxRecInfo[i].rec_mode,
				.rec_format = sEthCamTxRecInfo[i].rec_format,//_CFG_FILE_FORMAT_MP4,
				.buf_pa = addrPa,
				.buf_size = sizePa,
				.aq_info  = {1, 3, 36, 8, -8, 0},
				.cbr_info = {1, 4, sEthCamTxRecInfo[i].vfr, sEthCamTxRecInfo[i].tbr, sEthCamTxRecInfo[i].gop, 26, 10, 40, 26, 10, 40, 0, 1, 8, 4},
			};
			ImageApp_MovieMulti_SetParam(_CFG_ETHCAM_ID_1 + i, MOVIEMULTI_PRARM_ETHCAM_REC_INFO, (UINT32)&EthCamRecInfo);
		}
	}
#endif
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamGetTxInfo(void)
{
#if(defined(_NVT_ETHREARCAM_RX_))
	UINT32 i;
	UINT32 addrPa=0, sizePa=0;

	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		if(socketCliEthCmd_IsConn(i) && (socketCliEthData1_IsConn(i)==0) && EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
			//DBG_DUMP("BackgroundEthCamGetTxInfo, path_id=%d\r\n",i);
			sEthCamTxDecInfo[i].bStarupOK=0;
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT,ETHCAM_CMD_STARTUP, 0);
			if(sEthCamTxDecInfo[i].bStarupOK){
				#if 1
				//sync time
				//EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SYNC_TIME, 0);
				struct tm Curr_DateTime ={0};
				Curr_DateTime = hwclock_get_time(TIME_ID_CURRENT);
				//EthCam_SendXMLData(i, (UINT8 *)&Curr_DateTime, sizeof(struct tm));
				EthCam_SendXMLCmdData(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SYNC_TIME, 0, (UINT8 *)&Curr_DateTime, sizeof(struct tm));
				#endif

				#if 1
				//sync menu
				//EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SYNC_MENU_SETTING, 0);
				ETHCAM_MENU_SETTING sEthCamMenuSetting;
				#if (ETH_REARCAM_CAPS_COUNT>=2)
				sEthCamMenuSetting.Size=MOVIE_SIZE_FRONT_1920x1080P30;
				#else
				sEthCamMenuSetting.Size=MOVIE_SIZE_CLONE_1920x1080P30_1280x720P30;
				#endif
				sEthCamMenuSetting.WDR=UI_GetData(FL_MOVIE_WDR);
				sEthCamMenuSetting.EV=UI_GetData(FL_EV);
				sEthCamMenuSetting.DateImprint=UI_GetData(FL_MOVIE_DATEIMPRINT);
				sEthCamMenuSetting.SensorRotate=UI_GetData(FL_MOVIE_SENSOR_ROTATE);
				//sEthCamMenuSetting.Codec=MOVIE_CODEC_H265;//UI_GetData(FL_MOVIE_CODEC);//MOVIE_CODEC_H264;//UI_GetData(FL_MOVIE_CODEC);
				sEthCamMenuSetting.Codec=UI_GetData(FL_MOVIE_CODEC);//MOVIE_CODEC_H264;//UI_GetData(FL_MOVIE_CODEC);
#if defined(_NVT_ETHREARCAM_CAPS_COUNT_1_)
				sEthCamMenuSetting.TimeLapse=UI_GetData(FL_MOVIE_TIMELAPSE_REC);
#else
				sEthCamMenuSetting.TimeLapse=MOVIE_TIMELAPSEREC_OFF;
#endif
				//EthCam_SendXMLData(i, (UINT8 *)&sEthCamMenuSetting, sizeof(ETHCAM_MENU_SETTING));
				EthCam_SendXMLCmdData(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SYNC_MENU_SETTING, 0, (UINT8 *)&sEthCamMenuSetting, sizeof(ETHCAM_MENU_SETTING));
				#endif

				ETHCAM_PORT_TYPE port_type=ETHCAM_PORT_DATA1;
				#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
				port_type =ETHCAM_PORT_DATA2;
				#else
				port_type =ETHCAM_PORT_DATA1;
				#endif

				EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_GET_TX_REC_INFO, 0);

				EthCam_SendXMLCmd(i, port_type ,ETHCAM_CMD_GET_TX_DEC_INFO, 0);

				EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_GET_TX_SPS_PPS, 0);
				EthCamSocketCli_SetDescSize(i, ETHSOCKIPCCLI_ID_0,  sEthCamTxRecInfo[i].DescSize);

				#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
				EthCam_SendXMLCmd(i, port_type ,ETHCAM_CMD_GET_TX_SPS_PPS, 0);
				EthCamSocketCli_SetDescSize(i, ETHSOCKIPCCLI_ID_1,  sEthCamTxDecInfo[i].DescSize);
				#else
				EthCamSocketCli_SetDescSize(i, ETHSOCKIPCCLI_ID_0,  sEthCamTxDecInfo[i].DescSize);
				#endif

				#if 0 //temp
				//get tx dec info
				MOVIEMULTI_ETHCAM_DEC_INFO dec_info;

				dec_info.width = sEthCamTxDecInfo[i].Width;
				dec_info.height = sEthCamTxDecInfo[i].Height;
				dec_info.gop = sEthCamTxDecInfo[i].Gop;
				dec_info.codec = sEthCamTxDecInfo[i].Codec;
				dec_info.header_size = sEthCamTxDecInfo[i].DescSize;
				dec_info.header = sEthCamTxDecInfo[i].Desc;
				//DBG_IND("[%d]dec_info, w=%d, h=%d, GOP=%d, Codec=%d\r\n",i,dec_info.width ,dec_info.height,dec_info.gop,dec_info.codec);
				//DBG_IND("[%d]Desc=0x%x, DescSize=%d\r\n",i,sEthCamTxDecInfo[i].Desc,sEthCamTxDecInfo[i].DescSize);
				DBG_DUMP("[%d]dec_info DescSize=%d, Desc[0]=%x,%x,%x,%x,%x\r\n",i,sEthCamTxDecInfo[i].DescSize, sEthCamTxDecInfo[i].Desc[0],sEthCamTxDecInfo[i].Desc[1],sEthCamTxDecInfo[i].Desc[2],sEthCamTxDecInfo[i].Desc[3],sEthCamTxDecInfo[i].Desc[4]);
				if(sEthCamTxDecInfo[i].DescSize==0){
					DBG_ERR("[%d]TxDecInfo DescSize 0!!\r\n",i,sEthCamTxDecInfo[i].DescSize);
					return NVTRET_ERROR;
				}
				ImageApp_MovieMulti_SetParam((_CFG_ETHCAM_ID_1 + i), MOVIEMULTI_PRARM_ETHCAM_DEC_INFO, (UINT32)&dec_info);

				//EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_GET_TX_REC_INFO, 0);
				MOVIEMULTI_ETHCAM_REC_INFO EthCamRecInfo={0};
				memcpy(&EthCamRecInfo, &sEthCamTxRecInfo[i], sizeof(MOVIEMULTI_ETHCAM_REC_INFO));
				//EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_GET_TX_SPS_PPS, 0);

				//DBG_IND("rec_format=%d\r\n",sEthCamTxRecInfo[i].rec_format);
				if(sEthCamTxRecInfo[i].DescSize==0){
					DBG_ERR("[%d]RecInfo DescSize 0!!\r\n",i,sEthCamTxRecInfo[i].DescSize);
					return NVTRET_ERROR;
				}
				//DBG_IND("[%d]RecInfo Desc=0x%x, DescSize=%d\r\n",i,sEthCamTxRecInfo[i].Desc,sEthCamTxRecInfo[i].DescSize);
				DBG_DUMP("[%d]RecInfo DescSize=%d, Desc[0]=%x,%x,%x,%x,%x\r\n",i,sEthCamTxRecInfo[i].DescSize,sEthCamTxRecInfo[i].Desc[0],sEthCamTxRecInfo[i].Desc[1],sEthCamTxRecInfo[i].Desc[2],sEthCamTxRecInfo[i].Desc[3],sEthCamTxRecInfo[i].Desc[4]);

				EthCamRecInfo.rec_format=_CFG_FILE_FORMAT_MP4;
				//temp
				//ImageApp_MovieMulti_SetParam((_CFG_ETHCAM_ID_1 + i), MOVIEMULTI_PRARM_ETHCAM_REC_INFO, (UINT32)&EthCamRecInfo);
				ImageApp_MovieMulti_EthCamLinkForDisp((_CFG_ETHCAM_ID_1 + i), ENABLE, TRUE);
				#else
				//Dec
				MOVIEMULTI_ETHCAM_DEC_INFO dec_info={0};

				dec_info.width = sEthCamTxDecInfo[i].Width;
				dec_info.height = sEthCamTxDecInfo[i].Height;
				dec_info.codec = sEthCamTxDecInfo[i].Codec;
				DBG_DUMP("1Desc=0x%x, DescSize=%d ,%d\r\n",sEthCamTxDecInfo[i].Desc,sEthCamTxDecInfo[i].DescSize,dec_info.header_size);

				dec_info.header_size = sEthCamTxDecInfo[i].DescSize;
				DBG_DUMP("2Desc=0x%x, DescSize=%d ,%d\r\n",sEthCamTxDecInfo[i].Desc,sEthCamTxDecInfo[i].DescSize,dec_info.header_size);

				dec_info.header = sEthCamTxDecInfo[i].Desc;
				ImageApp_MovieMulti_SetParam(_CFG_ETHCAM_ID_1+i, MOVIEMULTI_PRARM_ETHCAM_DEC_INFO, (UINT32)&dec_info);

				//Rec
				MovieExe_SetEthcamEncBufSec(ETHCAM_PATH_ID_1+i, 5);

				socketCliEthData1_ConfigRecvBuf(i);
				socketCliEthData2_ConfigRecvBuf(i);
				socketCliEthData1_GetBsBufInfo(i, &addrPa, &sizePa);
				sEthCamTxRecInfo[i].aud_codec=gEthcamRecInfoAudCodec;//ETHCAM_AUDIO_FORMAT;
				sEthCamTxRecInfo[i].rec_format=gEthcamRecInfoRecFormat;//ETHCAM_FILE_FORMAT;
				MOVIEMULTI_ETHCAM_REC_INFO EthCamRecInfo = {
					.width = sEthCamTxRecInfo[i].width,
					.height = sEthCamTxRecInfo[i].height,
					.vfr = sEthCamTxRecInfo[i].vfr,
					.tbr = sEthCamTxRecInfo[i].tbr,
					.ar = sEthCamTxRecInfo[i].ar,
					.gop = sEthCamTxRecInfo[i].gop,
					.codec = (sEthCamTxRecInfo[i].codec == HD_CODEC_TYPE_H265)?_CFG_CODEC_H265: _CFG_CODEC_H264,//_CFG_CODEC_H265,
					.aud_codec = sEthCamTxRecInfo[i].aud_codec,
					.rec_mode = sEthCamTxRecInfo[i].rec_mode,
					.rec_format = sEthCamTxRecInfo[i].rec_format,//_CFG_FILE_FORMAT_MP4,
					.buf_pa = addrPa,
					.buf_size = sizePa,
					.aq_info  = {1, 3, 36, 8, -8, 0},
					.cbr_info = {1, 4, sEthCamTxRecInfo[i].vfr, sEthCamTxRecInfo[i].tbr, sEthCamTxRecInfo[i].gop, 26, 10, 40, 26, 10, 40, 0, 1, 8, 4},
				};
				ImageApp_MovieMulti_SetParam(_CFG_ETHCAM_ID_1 + i, MOVIEMULTI_PRARM_ETHCAM_REC_INFO, (UINT32)&EthCamRecInfo);

				#endif
				//MovieExe_SetEthcamEncBufSec(ETHCAM_PATH_ID_1+i, 5);

				//socketCliEthData1_ConfigRecvBuf(i);
				//socketCliEthData2_ConfigRecvBuf(i);

			}else{
				DBG_ERR("bStarupOK Fail, path_id=%d\r\n",i);
			}
		}
	}
#endif
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamTriggerThumb(void)
{
#if(defined(_NVT_ETHREARCAM_RX_))
	UINT32 i;
	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		//DBG_DUMP("[%d]BsPort2Recid=%d, %d\r\n",i,g_uiThumbPathId,ImageApp_MovieMulti_BsPort2EthCamlink(g_uiThumbPathId));
		if ((EthCamNet_GetEthLinkStatus(i) == ETHCAM_LINK_UP) && (g_uiEthcamTriggerThumbPathId[i] == (_CFG_ETHCAM_ID_1 + i))) {
			DBG_DUMP("BK Trigger Thumb[%d][%d]\r\n", i, g_uiEthcamTriggerThumbPathId[i]);
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_GET_TX_MOVIE_THUMB, 0);
		}
	}
#endif
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamTxDispOpenSocketStreamStart(void)
{
#if(defined(_NVT_ETHREARCAM_RX_))
	UINT32 i;
	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			socketCliEthData_Open(i, ETHSOCKIPCCLI_ID_1);
			ImageApp_MovieMulti_EthCamLinkForDisp(_CFG_ETHCAM_ID_1+i, ENABLE, TRUE);
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA2 ,ETHCAM_CMD_TX_STREAM_START, 0);
#else
			socketCliEthData_Open(i, ETHSOCKIPCCLI_ID_0);
			ImageApp_MovieMulti_EthCamLinkForDisp(_CFG_ETHCAM_ID_1+i, ENABLE, TRUE);
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_TX_STREAM_START, 0);
#endif
		}
	}
#endif
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamTxDispOpenSocketStream(void)
{
#if(defined(_NVT_ETHREARCAM_RX_))
	UINT32 i;
	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			socketCliEthData_Open(i, ETHSOCKIPCCLI_ID_1);
#else
			socketCliEthData_Open(i, ETHSOCKIPCCLI_ID_0);
#endif
		}
	}
#endif
	return NVTRET_OK;
}

static UINT32 BackgroundEthCamTxRecOpenSocketStreamStart(void)
{
#if(defined(_NVT_ETHREARCAM_RX_))
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
	UINT32 i;
	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
			socketCliEthData_Open(i, ETHSOCKIPCCLI_ID_0);
			EthCam_SendXMLCmd(i ,ETHCAM_PORT_DATA1 ,ETHCAM_CMD_TX_STREAM_START, 0);
			EthCamNet_SetPrevEthLinkStatus(i, ETHCAM_LINK_UP);
		}
	}
#endif
#endif
	return NVTRET_OK;
}


static UINT32 BackgroundEthCamTxStreamStart(void)
{
#if(defined(_NVT_ETHREARCAM_RX_))
	UINT32 i;
	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		//DBG_WRN("GetEthLinkStatus[%d]=%d, GetPrevEthLinkStatus[%d]=%d\r\n",i,EthCamNet_GetEthLinkStatus(i),i,EthCamNet_GetPrevEthLinkStatus(i));
		if(socketCliEthData1_IsConn(i) && EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
			DBG_DUMP("TxStream Open path id=%d\r\n",i);
  			ImageApp_MovieMulti_EthCamLinkForDisp((_CFG_ETHCAM_ID_1 + i), ENABLE, TRUE);
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA2 ,ETHCAM_CMD_TX_STREAM_START, 0);
#endif
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_TX_STREAM_START, 0);
			EthCamNet_SetPrevEthLinkStatus(i, ETHCAM_LINK_UP);
		}else{
#if (ETH_REARCAM_CAPS_COUNT>=2)
			if(socketCliEthData1_IsConn(i) && EthCamNet_GetEthHub_LinkStatus(i) && EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && socketCliEthData1_IsRecv(i)==0){
				DBG_DUMP("TxStream restart path id=%d\r\n",i);
				ImageApp_MovieMulti_EthCamLinkForDisp((_CFG_ETHCAM_ID_1 + i), ENABLE, TRUE);
				EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_TX_STREAM_START, 0);
			}
#endif
		}

		//ImageApp_MovieMulti_EthCamLinkForDisp((_CFG_ETHCAM_ID_1 + i), ENABLE, TRUE);

	}

	EthCamCmd_GetFrameTimerEn(1);
#endif
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamSyncTime(void)
{
#if(defined(_NVT_ETHREARCAM_RX_))
	UINT32 i;
	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
			//sync time
			//EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SYNC_TIME, 0);
			struct tm Curr_DateTime ={0};
			Curr_DateTime = hwclock_get_time(TIME_ID_CURRENT);
			//EthCam_SendXMLData(i, (UINT8 *)&Curr_DateTime, sizeof(struct tm));
			//EthCam_SendXMLData(i, (UINT8 *)&Curr_DateTime, sizeof(Curr_DateTime));
			EthCam_SendXMLCmdData(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SYNC_TIME, 0 ,(UINT8 *)&Curr_DateTime, sizeof(Curr_DateTime));

			//sync menu
			//EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SYNC_MENU_SETTING, 0);
			ETHCAM_MENU_SETTING sEthCamMenuSetting;
			#if (ETH_REARCAM_CAPS_COUNT>=2)
			sEthCamMenuSetting.Size=MOVIE_SIZE_FRONT_1920x1080P30;
			#else
			sEthCamMenuSetting.Size=MOVIE_SIZE_CLONE_1920x1080P30_1280x720P30;
			#endif
			sEthCamMenuSetting.WDR=UI_GetData(FL_MOVIE_WDR);
			sEthCamMenuSetting.EV=UI_GetData(FL_EV);
			sEthCamMenuSetting.DateImprint=UI_GetData(FL_MOVIE_DATEIMPRINT);
			sEthCamMenuSetting.SensorRotate=UI_GetData(FL_MOVIE_SENSOR_ROTATE);
			//sEthCamMenuSetting.Codec=MOVIE_CODEC_H265;//UI_GetData(FL_MOVIE_CODEC);
			sEthCamMenuSetting.Codec=UI_GetData(FL_MOVIE_CODEC);
			sEthCamMenuSetting.TimeLapse=UI_GetData(FL_MOVIE_TIMELAPSE_REC);
			//EthCam_SendXMLData(i, (UINT8 *)&sEthCamMenuSetting, sizeof(ETHCAM_MENU_SETTING));
			EthCam_SendXMLCmdData(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SYNC_MENU_SETTING, 0,(UINT8 *)&sEthCamMenuSetting, sizeof(ETHCAM_MENU_SETTING));
		}
	}
#endif
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamDecErr(void)
{
#if(defined(_NVT_ETHREARCAM_RX_))
	UINT32 DecErrPathId = g_uiEthcamDecErrPathId;
	EthCamCmd_GetFrameTimerEn(0);
	EthCam_SendXMLCmd(DecErrPathId, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_TX_STREAM_STOP, 0);
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
	EthCam_SendXMLCmd(DecErrPathId, ETHCAM_PORT_DATA2 ,ETHCAM_CMD_TX_STREAM_STOP, 0);
#endif
	EthCamCmd_GetFrameTimerEn(1);
#endif
	return NVTRET_OK;
}

static UINT32 BackgroundEthCamRawEncodeResult(void)
{
	if(!ImageApp_MovieMulti_IsStreamRunning(_CFG_REC_ID_1)
		#if (SENSOR_CAPS_COUNT>=2)
		&& !ImageApp_MovieMulti_IsStreamRunning(_CFG_REC_ID_2)
		#endif
	){
		if(ImageApp_MovieMulti_IsStreamRunning(_CFG_ETHCAM_ID_1)
			|| ImageApp_MovieMulti_IsStreamRunning(_CFG_ETHCAM_ID_2)){
			MovieExe_TriggerPIMResultManual(0);
		}
	}
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamSetTxSysInfo(void)
{
#if(defined(_NVT_ETHREARCAM_RX_))
	UINT32 i;
	UINT32 TxVdoEncBufSec=3; //default=3
	for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
		if(socketCliEthCmd_IsConn(i) && EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
			sEthCamTxSysInfo[i].bCmdOK=0;
			//EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SET_TX_SYS_INFO, TxVdoEncBufSec);
			//EthCam_SendXMLData(i, (UINT8 *)&sEthCamTxSysInfo[i], sizeof(ETHCAM_TX_SYS_INFO));
			EthCam_SendXMLCmdData(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SET_TX_SYS_INFO, TxVdoEncBufSec, (UINT8 *)&sEthCamTxSysInfo[i], sizeof(ETHCAM_TX_SYS_INFO));
			DBG_DUMP("bCmdOK[%d]=%d\r\n",i,sEthCamTxSysInfo[i].bCmdOK);

			if(sEthCamTxSysInfo[i].bCmdOK==1){
				//EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SET_TX_CODEC_SRCTYPE, 0);
				//EthCam_SendXMLData(i, (UINT8 *)&sEthCamCodecSrctype[i], sizeof(ETHCAM_TX_CODEC_SRCTYPE));
				EthCam_SendXMLCmdData(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SET_TX_CODEC_SRCTYPE, 0,(UINT8 *)&sEthCamCodecSrctype[i], sizeof(ETHCAM_TX_CODEC_SRCTYPE));

				//EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SET_TX_AUDCAP, g_EthCamCfgTxAudCap);
				//EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SET_TX_AUDINFO, 0);
				//MOVIEMULTI_AUDIO_INFO MovieAudInfo=MovieExe_GetAudInfo();
				//EthCam_SendXMLData(i, (UINT8 *)&MovieAudInfo, sizeof(MOVIEMULTI_AUDIO_INFO));

				#if (ETHCAM_EIS ==ENABLE)
				//EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_EIS_INFO, 0);
				//EthCam_SendXMLData(i, (UINT8 *)&sEthCamTxEISInfo[i], sizeof(ETHCAM_CMD_EISINFO));
				EthCam_SendXMLCmdData(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_EIS_INFO, 0, (UINT8 *)&sEthCamTxEISInfo[i], sizeof(ETHCAM_CMD_EISINFO));
				#endif
			}
		}
	}
#endif
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamCheckPortReady(void)
{
#if (ETHCAM_CHECK_PORTREADY == ENABLE)
#if(defined(_NVT_ETHREARCAM_RX_))

	char ipccmd[64]={0};
	//NVTIPC_SYS_MSG sysMsg;
	//NVTIPC_I32 ipcErr = 0;

	snprintf(ipccmd, sizeof(ipccmd) - 1, "ethcamcliconncheck %d",g_uiEthcamCheckPortReadyIPAddr);
	#if 0
	sysMsg.sysCmdID = NVTIPC_SYSCMD_SYSCALL_REQ;
	sysMsg.DataAddr = (UINT32)ipccmd;
	sysMsg.DataSize = strlen(ipccmd) + 1;
	DBG_DUMP("^CBKCheckPortReady ipccmd=%s\r\n",ipccmd);

	if ((ipcErr = NvtIPC_MsgSnd(NVTIPC_SYS_QUEUE_ID, NVTIPC_SENDTO_CORE2, &sysMsg, sizeof(sysMsg))) < 0) {
		DBG_ERR("Failed to NVTIPC_SYS_QUEUE_ID\r\n");
	}
	#else
	EthCamNet_PortReadyCheck(ipccmd);
	#endif
	EthCamNet_EthHubPortReadySendCmdTimerOpen();
#endif
#endif
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamUpdateUIInfo(void)
{
#if(defined(_NVT_ETHREARCAM_RX_))

	#if defined(_UI_STYLE_LVGL_)
	extern void UIFlowMovie_update_icons(void);
	lv_user_task_handler_lock();
	UIFlowMovie_update_icons();
	lv_user_task_handler_unlock();
	#else
	FlowMovie_UpdateIcons(1);
	#endif

#endif
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamIperfTest(void)
{
#if(defined(_NVT_ETHREARCAM_RX_))
	EthCamNet_IperfTest();
#endif
	return NVTRET_OK;
}

#endif

void AppBKW_SetData(BKW_DATA_SET attribute, UINT32 value)
{
	switch (attribute) {
	case BKW_DPOF_MODE:
		g_uiDpofOPMode = value;
		break;
	case BKW_DPOF_NUM:
		g_uiDpofPrtNum = value;
		break;
	case BKW_DPOF_DATE:
		g_uiDpofDateOn = value;
		break;
	case BKW_PROTECT_TYPE:
		g_uiProtectType = value;
		break;
	case BKW_WAIT_TIME:
		g_uiWaitTime = value;
		break;
	case BKW_DZOOM_STOP:
		g_pDzoomStop = (UINT32 *)value;
		break;
	case BKW_ETHCAM_TRIGGER_THUMB_PATHID_P0:
		g_uiEthcamTriggerThumbPathId[0] = value;
		break;
	case BKW_ETHCAM_TRIGGER_THUMB_PATHID_P1:
		g_uiEthcamTriggerThumbPathId[1] = value;
		break;
	case BKW_ETHCAM_DEC_ERR_PATHID:
		g_uiEthcamDecErrPathId = value;
		break;
	case BKW_ETHCAM_CHECK_PORT_READY_IP:
		DBG_DUMP("SET PortReadyIP =%d\r\n",value);
		g_uiEthcamCheckPortReadyIPAddr = value;
		break;

	default:
		DBG_ERR("[AppBKW_SetData]no this attribute");
		break;
	}

}
UINT32 AppBKW_GetData(BKW_DATA_SET attribute)
{
	UINT32 value=0xffff;
	switch (attribute) {
	case BKW_ETHCAM_TRIGGER_THUMB_PATHID_P0:
		value=g_uiEthcamTriggerThumbPathId[0];
		break;
	case BKW_ETHCAM_TRIGGER_THUMB_PATHID_P1:
		value=g_uiEthcamTriggerThumbPathId[1];
		break;
	default:
		DBG_ERR("[AppBKW_GetData]no this attribute");
		break;
	}
	return value;
}


