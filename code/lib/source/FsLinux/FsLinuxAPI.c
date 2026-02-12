#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FsLinuxAPI.h"
#include "FsLinuxBuf.h"
#include "FsLinuxDebug.h"
#include "FsLinuxID.h"
#include "FsLinuxLock.h"
#if FSLINUX_USE_IPC
#include "FsLinuxIpc.h"
#endif
#include "FsLinuxSxCmd.h"
#include "FsLinuxTsk.h"
#include "FsLinuxUtil.h"

#define FSLINUX_FIXED_ITEMNUM   0 //for test and debug
#define FSLINUX_DROP_CACHES     1 //for drop clean caches

#define FSLINUX_API_ENTRY()     do{ DBG_FUNC_BEGIN("[api]\r\n"); } while(0)
#define FSLINUX_API_RETURN(ret) do{ DBG_FUNC_END("[api]\r\n"); return ret;} while(0)
#define FSLINUX_API_RETURN_VOID() do{ DBG_FUNC_END("[api]\r\n"); return;} while(0)

unsigned int FsLinux_debug_level = THIS_DBGLVL;

static CHAR g_cwd[KFS_LONGNAME_PATH_MAX_LENG] = {0};

UINT32 g_OpenedDriveNum = 0;

static FSLINUX_FILE_INNER *fslnx_openfile(CHAR *pPath, UINT32 flag);
static INT32 fslnx_closefile(FSLINUX_FILE_INNER *pFileInner);
static UINT32 fslnx_readfile(FSLINUX_FILE_INNER *pFileInner, UINT8 *pBuf, UINT32 BufSize, UINT32 Flag, FileSys_CB CB);
static UINT32 fslnx_writefile(FSLINUX_FILE_INNER *pFileInner, UINT8 *pBuf, UINT32 BufSize, UINT32 Flag, FileSys_CB CB);
static INT32 fslnx_deletefile(CHAR *pPath);

static FSLINUX_SEARCH_INNER *fslnx_searchfileopen(CHAR *pPath, FSLINUX_SEARCHFILE_BUFINFO *pSearchBuf, INT32 Direction);
static INT32 fslnx_searchfile(FSLINUX_SEARCH_INNER *pSearchInner, UINT32 ItemNum);
static INT32 fslnx_searchfiledel(FSLINUX_SEARCH_INNER *pSearchInner, UINT32 ItemNum);
static INT32 fslnx_searchfilelock(FSLINUX_SEARCH_INNER *pSearchInner, UINT32 ItemNum, BOOL bLock);
static INT32 fslnx_searchfileclose(FSLINUX_SEARCH_INNER *pSearchInner);

static FSLINUX_FILE_INNER *fslnx_FindEmptyFileHandle(CHAR Drive);
static UINT32 fslnx_ReleaseFileHandle(FSLINUX_FILE_INNER *pFileInner);
static UINT32 fslnx_IsValidFileHandle(FSLINUX_FILE_INNER *pFileInner);

static INT32 fslnx_SendPureCmd(FSLINUX_CMDID CmdId, CHAR Drive);

static INT32 fslnx_SendPureCmd(FSLINUX_CMDID CmdId, CHAR Drive)
{
	FSLINUX_IPCMSG IpcMsg;

	//set ipc msg
	IpcMsg.CmdId = CmdId;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = 0;

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		return FST_STA_OK;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		return FST_STA_ERROR;
	}
}


INT32 FsLinux_Open(CHAR Drive, FS_HANDLE StrgDXH, FILE_TSK_INIT_PARAM *pInitParam)
{
	FSLINUX_CTRL_OBJ *pCtrlObj;

	FSLINUX_API_ENTRY();

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Already opened\r\n");
		FSLINUX_API_RETURN(FST_STA_OK);
	}

	if (!pInitParam) {
		DBG_ERR("pInitParam is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (FALSE == FsLinux_IsDefSync()) {
		DBG_ERR("Def no sync\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//For backward compatible. If users did not define this, set to the default value
	if (0 == pInitParam->FSParam.MaxOpenedFileNum) {
		pInitParam->FSParam.MaxOpenedFileNum = KFS_FOPEN_MAX_NUM;
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	if (E_OK != FsLinux_Buf_Init(Drive, StrgDXH, pInitParam)) {
		DBG_ERR("FsLinux_Buf_Init ERR\r\n");
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
#if FSLINUX_USE_IPC
	if (0 == kchk_flg(FSLINUX_COMMON_FLG_ID, FSLINUX_COMMON_FLGBIT_TSK_READY)) {
		//create the API task
		sta_tsk(FSLINUX_COMMON_TSK_ID, 0);
	}
#endif
	//install SxCmd
	//xFsLinux_InstallCmd();

	//init flag status
#if FSLINUX_USE_IPC
	clr_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_IPC_INFO_RCVED);
#endif
	set_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_BUF_ARG_FREE);
	set_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_BUF_INFO_FREE);
	set_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_BUF_TMP_FREE);

	g_OpenedDriveNum++;
	set_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED);

	// reset
	//if (FsLinux_Util_IsSDIOErr(Drive))
	{
		FsLinux_Util_ClrSDIOErr(Drive);
	}

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	//For FsUitron, the disk callback may return error status
	//For FsLinux, all errors are returned before, so OK is always returned by the callback function
	if (pInitParam->pDiskErrCB) {
		pInitParam->pDiskErrCB(FST_STA_OK, 0, 0, 0);
	}

	FSLINUX_API_RETURN(FST_STA_OK);
}

INT32 FsLinux_Close(CHAR Drive, UINT32 TimeOut)
{
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FLGPTN FlgPtn;

	FSLINUX_API_ENTRY();

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Already closed\r\n");
		FSLINUX_API_RETURN(FST_STA_OK);
	}

	if (FST_TIME_NO_WAIT == TimeOut) {
		DBG_WRN("No wait not supported\r\n");
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	//wait other APIs done and lock them
	vos_flag_wait_interruptible(&FlgPtn, pCtrlObj->FlagID, FSLINUX_FLGBIT_BUF_ARG_FREE, (TWF_ORW | TWF_CLR));
	vos_flag_wait_interruptible(&FlgPtn, pCtrlObj->FlagID, FSLINUX_FLGBIT_BUF_TMP_FREE, (TWF_ORW | TWF_CLR));

	//set ipc msg
	if (FST_STA_OK != fslnx_SendPureCmd(FSLINUX_CMDID_FINISH, Drive)) {
		DBG_ERR("fslnx_SendPureCmd(%d, %c) failed\r\n", FSLINUX_CMDID_FINISH, Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

#if FSLINUX_USE_IPC
	if (1 == g_OpenedDriveNum) { //last drive
		//wait tsk exit
		DBG_IND("Send IPC Down cmd\r\n");
		if (FST_STA_OK != fslnx_SendPureCmd(FSLINUX_CMDID_IPC_DOWN, Drive)) {
			DBG_ERR("fslnx_SendPureCmd(%d, %c) failed\r\n", FSLINUX_CMDID_FINISH, Drive);
			FsLinux_Unlock(pCtrlObj);
			FSLINUX_API_RETURN(FST_STA_ERROR);
		}

		wai_flg(&FlgPtn, FSLINUX_COMMON_FLG_ID, FSLINUX_COMMON_FLGBIT_TSK_EXITED, TWF_ORW);
	}
#endif

	if (E_OK != FsLinux_Buf_Uninit(Drive)) {
		DBG_ERR("FsLinux_Buf_Uninit\r\n");
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	g_OpenedDriveNum--;
	clr_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED);
#if FSLINUX_USE_IPC
	set_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_IPC_INFO_RCVED); //set flg to prevent blocking GetDiskInfo API
#endif

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(FST_STA_OK);
}

static FSLINUX_FILE_INNER *fslnx_FindEmptyFileHandle(CHAR Drive)
{
	FSLINUX_DRIVE_INFO *pDrvInfo;
	FSLINUX_FILE_INNER *pCurFile;
	UINT32 i;
	UINT32 MaxOpenedFileNum;

	pDrvInfo = FsLinux_Buf_GetDrvInfo(Drive);
	if (NULL == pDrvInfo) {
		DBG_ERR("pDrvInfo is NULL\r\n");
		return NULL;
	}
	MaxOpenedFileNum = pDrvInfo->FsInitParam.FSParam.MaxOpenedFileNum;

	pCurFile = pDrvInfo->pOpenFileList;
	if (NULL == pCurFile) {
		DBG_ERR("pCurFile is NULL\r\n");
		return NULL;
	}

	for (i = 0; i < MaxOpenedFileNum; i++) {
		if (0 == pCurFile->Linux_fd) {
			pCurFile->Linux_fd = (UINT32)(-1);//mark as used
			return pCurFile;
		}

		pCurFile++;
	}

	DBG_WRN("No empty file handle\r\n");
	return NULL;
}

static UINT32 fslnx_ReleaseFileHandle(FSLINUX_FILE_INNER *pFileInner)
{
	memset(pFileInner, 0, sizeof(FSLINUX_FILE_INNER));
	return 0;
}

static UINT32 fslnx_IsValidFileHandle(FSLINUX_FILE_INNER *pFileInner)
{
	UINT32 Target_fd;
	FSLINUX_DRIVE_INFO *pDrvInfo;
	FSLINUX_FILE_INNER *pCurFile;
	UINT32 i;
	UINT32 MaxOpenedFileNum;

	if (NULL == pFileInner) {
		return 0;
	}

	Target_fd = pFileInner->Linux_fd;

	pDrvInfo = FsLinux_Buf_GetDrvInfo(pFileInner->Drive);
	if (NULL == pDrvInfo) {
		DBG_ERR("pDrvInfo is NULL\r\n");
		return 0;
	}
	MaxOpenedFileNum = pDrvInfo->FsInitParam.FSParam.MaxOpenedFileNum;

	pCurFile = pDrvInfo->pOpenFileList;
	if (NULL == pCurFile) {
		DBG_ERR("pCurFile is NULL\r\n");
		return 0;
	}

	for (i = 0; i < MaxOpenedFileNum; i++) {
		if (pCurFile->Linux_fd == Target_fd) {
			return 1;//valid
		}

		pCurFile++;
	}

	return 0;//invalid
}

static FSLINUX_SEARCH_INNER *fslnx_FindEmptySearchHandle(CHAR Drive)
{
	FSLINUX_DRIVE_INFO *pDrvInfo;
	FSLINUX_SEARCH_INNER *pCurSearch;
	UINT32 i;

	pDrvInfo = FsLinux_Buf_GetDrvInfo(Drive);
	if (NULL == pDrvInfo) {
		DBG_ERR("pDrvInfo is NULL\r\n");
		return NULL;
	}

	pCurSearch = pDrvInfo->OpenSearchList;

	for (i = 0; i < KFS_FSEARCH_MAX_NUM; i++) {
		if (0 == pCurSearch->Linux_dirfd) {
			pCurSearch->Linux_dirfd = (UINT32)(-1);//mark as used
			return pCurSearch;
		}

		pCurSearch++;
	}

	DBG_WRN("No empty search handle\r\n");
	return NULL;
}

static UINT32 fslnx_ReleaseSearchHandle(FSLINUX_SEARCH_INNER *pSearchInner)
{
	memset(pSearchInner, 0, sizeof(FSLINUX_SEARCH_INNER));
	return 0;
}

static UINT32 fslnx_IsValidSearchHandle(FSLINUX_SEARCH_INNER *pSearchInner)
{
	UINT32 Target_dirfd;
	FSLINUX_DRIVE_INFO *pDrvInfo;
	FSLINUX_SEARCH_INNER *pCurSearch;
	UINT32 i;

	if (NULL == pSearchInner) {
		return 0;
	}

	Target_dirfd = pSearchInner->Linux_dirfd;

	pDrvInfo = FsLinux_Buf_GetDrvInfo(pSearchInner->Drive);
	if (NULL == pDrvInfo) {
		DBG_ERR("pDrvInfo is NULL\r\n");
		return 0;
	}
	pCurSearch = pDrvInfo->OpenSearchList;

	for (i = 0; i < KFS_FSEARCH_MAX_NUM; i++) {
		if (pCurSearch->Linux_dirfd == Target_dirfd) {
			return 1;//valid
		}

		pCurSearch++;
	}

	return 0;//invalid
}

static FSLINUX_FILE_INNER *fslnx_openfile(CHAR *pPath, UINT32 flag)
{
	CHAR Drive = *pPath;
	FSLINUX_ARG_OPENFILE *pSharedArg;
	FSLINUX_FILE_INNER *pFileInner;
	FSLINUX_IPCMSG IpcMsg;

	pSharedArg = (FSLINUX_ARG_OPENFILE *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		return NULL;
	}

	//get an empty file handle
	pFileInner = fslnx_FindEmptyFileHandle(Drive);
	if (NULL == pFileInner) {
		DBG_ERR("No empty handles\r\n");
		FsLinux_Buf_UnlockArgBuf(Drive);
		return NULL;
	}

	//set shared arguments
	FsLinux_Conv2LinuxPath(pSharedArg->szPath, sizeof(pSharedArg->szPath), pPath);
	pSharedArg->flag = flag;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_OPENFILE;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//fslinux_cmd_DoCmd(&IpcMsg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK != FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK != FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		fslnx_ReleaseFileHandle(pFileInner);
		FsLinux_Buf_UnlockArgBuf(Drive);
		return NULL;
	}

	if (-1 == pSharedArg->Linux_fd) {
		fslnx_ReleaseFileHandle(pFileInner);
		FsLinux_Buf_UnlockArgBuf(Drive);
		return NULL;
	}

	pFileInner->Linux_fd = pSharedArg->Linux_fd;
	pFileInner->Drive = *pPath;//set the drive name, e.g. 'A'

	FsLinux_Buf_UnlockArgBuf(Drive);
	//----------unlock----------

	return pFileInner;
}

static INT32 fslnx_closefile(FSLINUX_FILE_INNER *pFileInner)
{
	CHAR Drive = pFileInner->Drive;
	FSLINUX_ARG_CLOSEFILE *pSharedArg;
	FSLINUX_IPCMSG IpcMsg;
	INT32 ret_fslnx;

	pSharedArg = (FSLINUX_ARG_CLOSEFILE *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		return FST_STA_ERROR;
	}

	//set shared arguments
	pSharedArg->Linux_fd = pFileInner->Linux_fd;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_CLOSEFILE;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		ret_fslnx = (INT32)pSharedArg->ret_fslnx;
		fslnx_ReleaseFileHandle(pFileInner);
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		ret_fslnx = FST_STA_ERROR;
	}

	if (FST_STA_SDIO_ERR == pSharedArg->ret_fslnx) {
		FsLinux_Util_SetSDIOErr(Drive);
	}

	FsLinux_Buf_UnlockArgBuf(Drive);
	//----------unlock----------

	return ret_fslnx;
}

static UINT32 fslnx_readfile(FSLINUX_FILE_INNER *pFileInner, UINT8 *pBuf, UINT32 BufSize, UINT32 Flag, FileSys_CB CB)
{
	CHAR Drive = pFileInner->Drive;
	FSLINUX_ARG_READFILE *pSharedArg;
	FSLINUX_IPCMSG IpcMsg;
	UINT32 ret_size = 0;

	if (!FsLinux_FlushReadCache((UINT32)pBuf, BufSize)) {
		DBG_WRN("FsLinux_FlushReadCache failed\r\n");
		return 0;
	}

	pSharedArg = (FSLINUX_ARG_READFILE *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		return 0;
	}

	//set shared arguments
	pSharedArg->Linux_fd = pFileInner->Linux_fd;
	pSharedArg->phy_pBuf = (void *)ipc_getPhyAddr((UINT32)pBuf);
	pSharedArg->rwsize = BufSize;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_READFILE;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//if sync mode, return OK without unlock
	if (Flag & FST_FLAG_ASYNC_MODE) {
		//FsLinux_Ipc_PostMsg(&IpcMsg);
		//FsLinux_Tsk_WaitForAsyncCmd(FSLINUX_CMDID_READFILE, CB, Drive);
		//return BufSize;
		DBG_WRN("Not supported now\r\n");
		FsLinux_Buf_UnlockArgBuf(Drive);
		return 0;
	}

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		if (FST_STA_OK == pSharedArg->ret_fslnx) {
			ret_size = pSharedArg->rwsize;
		}
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		ret_size = 0;
	}

	if (FST_STA_SDIO_ERR == pSharedArg->ret_fslnx) {
		FsLinux_Util_SetSDIOErr(Drive);
	}

	FsLinux_Buf_UnlockArgBuf(Drive);
	//----------unlock----------

	return ret_size;
}

static UINT32 fslnx_writefile(FSLINUX_FILE_INNER *pFileInner, UINT8 *pBuf, UINT32 BufSize, UINT32 Flag, FileSys_CB CB)
{
	CHAR Drive = pFileInner->Drive;
	FSLINUX_ARG_WRITEFILE *pSharedArg;
	FSLINUX_IPCMSG IpcMsg;
	UINT32 ret_size = 0;

	if (!FsLinux_FlushWriteCache((UINT32)pBuf, BufSize)) {
		DBG_WRN("FsLinux_FlushWriteCache\r\n");
		return 0;
	}

	pSharedArg = (FSLINUX_ARG_WRITEFILE *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		return 0;
	}

	//set shared arguments
	pSharedArg->Linux_fd = pFileInner->Linux_fd;
	pSharedArg->phy_pBuf = (void *)ipc_getPhyAddr((UINT32)pBuf);
	pSharedArg->rwsize = BufSize;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_WRITEFILE;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//if sync mode, return OK without setting ipc available
	if (Flag & FST_FLAG_ASYNC_MODE) {
		//FsLinux_Ipc_PostMsg(&IpcMsg);
		//FsLinux_Tsk_WaitForAsyncCmd(FSLINUX_CMDID_WRITEFILE, CB, Drive);
		//FSLINUX_API_RETURN(FST_STA_OK);
		DBG_WRN("Not supported now\r\n");
		FsLinux_Buf_UnlockArgBuf(Drive);
		return 0;
	}

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		if (FST_STA_OK == pSharedArg->ret_fslnx) {
			ret_size = pSharedArg->rwsize;
		}
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		ret_size = 0;
	}

	if (FST_STA_SDIO_ERR == pSharedArg->ret_fslnx) {
		FsLinux_Util_SetSDIOErr(Drive);
	}

	FsLinux_Buf_UnlockArgBuf(Drive);
	//----------unlock----------

	return ret_size;
}


static INT32 fslnx_deletefile(CHAR *pPath)
{
	CHAR Drive;
	FSLINUX_ARG_DELETEFILE *pSharedArg;
	FSLINUX_IPCMSG IpcMsg;
	INT32 ret_fslnx;

	if (NULL == pPath) {
		DBG_ERR("pPath is NULL\r\n");
		return FST_STA_ERROR;
	}
	Drive = *pPath;

	pSharedArg = (FSLINUX_ARG_DELETEFILE *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		return FST_STA_ERROR;
	}

	//set shared arguments
	FsLinux_Conv2LinuxPath(pSharedArg->szPath, sizeof(pSharedArg->szPath), pPath);

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_DELETEFILE;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		ret_fslnx = pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		ret_fslnx = FST_STA_ERROR;
	}

	if (FST_STA_SDIO_ERR == pSharedArg->ret_fslnx) {
		FsLinux_Util_SetSDIOErr(Drive);
	}

	FsLinux_Buf_UnlockArgBuf(Drive);
	//----------unlock----------

	return ret_fslnx;
}

FST_FILE FsLinux_OpenFile(CHAR *pPath, UINT32 flag)
{
	FSLINUX_FILE_INNER *pFileInner;
	FSLINUX_CTRL_OBJ *pCtrlObj;

	FSLINUX_API_ENTRY();

	if (NULL == pPath) {
		DBG_WRN("filename is NULL\r\n");
		FSLINUX_API_RETURN(NULL);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(*pPath);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(NULL);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(NULL);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(*pPath)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(NULL);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pFileInner = fslnx_openfile(pPath, flag);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN((FST_FILE)pFileInner);
}

INT32 FsLinux_CloseFile(FST_FILE pFile)
{
	CHAR Drive;
	FSLINUX_FILE_INNER *pFileInner = (FSLINUX_FILE_INNER *)pFile;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (0 == fslnx_IsValidFileHandle(pFileInner)) {
		DBG_WRN("Invalid file handle 0x%X\r\n", (unsigned int)pFileInner);
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = pFileInner->Drive;

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	// close the file to avoid remount/recovery failure
	#if 0
	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}
	#endif

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	Ret_FsApi = fslnx_closefile(pFileInner);

	//check sdio status (curr)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("sdio err is set\r\n");
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_ReadFile(FST_FILE pFile, UINT8 *pBuf, UINT32 *pBufSize, UINT32 Flag, FileSys_CB CB)
{
	FSLINUX_FILE_INNER *pFileInner = (FSLINUX_FILE_INNER *)pFile;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	UINT32 ReqSize, ActualSize;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (0 == fslnx_IsValidFileHandle(pFileInner)) {
		DBG_WRN("Invalid file handle 0x%X\r\n", (unsigned int)pFileInner);
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (NULL == pBuf) {
		DBG_WRN("pBuf is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (NULL == pBufSize) {
		DBG_WRN("pBufSize is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (0 == *pBufSize) {
		DBG_WRN("Read 0 bytes\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(pFileInner->Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	ReqSize = *pBufSize;

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(pFileInner->Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	//if sync mode, return OK without unlock
	if (Flag & FST_FLAG_ASYNC_MODE) {
		/*
		fslnx_readfile(pFileInner, pBuf, ReqSize, Flag, CB);
		//FsLinux_UnlockArg();//unlock by FsLinuxTsk
		FSLINUX_API_RETURN(FST_STA_OK);
		*/
		DBG_WRN("Not supported now\r\n");
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	ActualSize = fslnx_readfile(pFileInner, pBuf, ReqSize, Flag, CB);
	if (ActualSize > 0) { //we block read 0 bytes before, so we should at least get one byte
		*pBufSize = ActualSize;
		Ret_FsApi = FST_STA_OK;
	} else {
		*pBufSize = 0;
		Ret_FsApi = FST_STA_ERROR;
	}

	//check sdio status (curr)
	if (FsLinux_Util_IsSDIOErr(pFileInner->Drive)) {
		DBG_ERR("sdio err is set\r\n");
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_WriteFile(FST_FILE pFile, UINT8 *pBuf, UINT32 *pBufSize, UINT32 Flag, FileSys_CB CB)
{
	FSLINUX_FILE_INNER *pFileInner = (FSLINUX_FILE_INNER *)pFile;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	UINT32 ReqSize, ActualSize;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (0 == fslnx_IsValidFileHandle(pFileInner)) {
		DBG_WRN("Invalid file handle 0x%X\r\n", (unsigned int)pFileInner);
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (NULL == pBuf) {
		DBG_WRN("pBuf is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (NULL == pBufSize) {
		DBG_WRN("pBufSize is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (0 == *pBufSize) {
		DBG_WRN("Write 0 bytes\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(pFileInner->Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	ReqSize = *pBufSize;

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(pFileInner->Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	//if sync mode, return OK without setting ipc available
	if (Flag & FST_FLAG_ASYNC_MODE) {
		/*
		fslnx_writefile(pFileInner, pBuf, ReqSize, Flag, CB);
		//FsLinux_UnlockArg();//unlock by FsLinuxTsk
		FSLINUX_API_RETURN(FST_STA_OK);
		*/
		DBG_WRN("Not supported now\r\n");
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	ActualSize = fslnx_writefile(pFileInner, pBuf, ReqSize, Flag, CB);
	if (ActualSize == ReqSize) { //we do not allow partially write
		*pBufSize = ActualSize;
		Ret_FsApi = FST_STA_OK;
	} else {
		*pBufSize = 0;
		Ret_FsApi = FST_STA_ERROR;
	}

	//check sdio status (curr)
	if (FsLinux_Util_IsSDIOErr(pFileInner->Drive)) {
		DBG_ERR("sdio err is set\r\n");
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_WaitFinish(CHAR Drive)
{
	FSLINUX_CTRL_OBJ *pCtrlObj;

	FSLINUX_API_ENTRY();

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//try to lock to make sure the previous API is done
	FsLinux_Lock(pCtrlObj);
	FsLinux_Unlock(pCtrlObj);

	FSLINUX_API_RETURN(FST_STA_OK);
}

INT32 FsLinux_SeekFile(FST_FILE pFile, UINT64 offset, FST_SEEK_CMD fromwhere)
{
	CHAR Drive;
	FSLINUX_FILE_INNER *pFileInner = (FSLINUX_FILE_INNER *)pFile;
	FSLINUX_ARG_SEEKFILE *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (0 == fslnx_IsValidFileHandle(pFileInner)) {
		DBG_WRN("Invalid file handle 0x%X\r\n", (unsigned int)pFileInner);
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = pFileInner->Drive;

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_SEEKFILE *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	pSharedArg->Linux_fd = pFileInner->Linux_fd;
	pSharedArg->offset = offset;
	pSharedArg->whence = fromwhere;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_SEEKFILE;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = (INT32)pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

UINT64 FsLinux_TellFile(FST_FILE pFile)
{
	CHAR Drive;
	FSLINUX_FILE_INNER *pFileInner = (FSLINUX_FILE_INNER *)pFile;
	FSLINUX_ARG_TELLFILE *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	UINT64 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (0 == fslnx_IsValidFileHandle(pFileInner)) {
		DBG_WRN("Invalid file handle 0x%X\r\n", (unsigned int)pFileInner);
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = pFileInner->Drive;

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_TELLFILE *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	pSharedArg->Linux_fd = pFileInner->Linux_fd;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_TELLFILE;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = 0;
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_DeleteFile(CHAR *pPath)
{
	FSLINUX_CTRL_OBJ *pCtrlObj;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (NULL == pPath) {
		DBG_WRN("pPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(*pPath);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(*pPath)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	Ret_FsApi = fslnx_deletefile(pPath);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_DeleteMultiFiles(FST_MULTI_FILES *pMultiFiles)
{
	FSLINUX_CTRL_OBJ *pCtrlObj;
	INT32 Ret_FsApi = FST_STA_OK;
	CHAR Drive = 0;
	INT32 FileIdx;
	CHAR *pCurPath;

	FSLINUX_API_ENTRY();

	if (NULL == pMultiFiles) {
		DBG_WRN("pMultiFiles is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	for (FileIdx = 0; FileIdx < KFS_MULTI_FILES_MAX_NUM; FileIdx++) {
		pCurPath = pMultiFiles->pFilePath[FileIdx];

		if (NULL == pCurPath || 0 == *pCurPath) {
			continue;
		}

		if (0 == Drive) {
			Drive = *pCurPath;
		} else if (Drive != *pCurPath) {
			DBG_ERR("Drive not match %d != %d\r\n", Drive, *pCurPath);
			FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
		}
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	for (FileIdx = 0; FileIdx < KFS_MULTI_FILES_MAX_NUM; FileIdx++) {
		pCurPath = pMultiFiles->pFilePath[FileIdx];

		if (NULL == pCurPath || 0 == *pCurPath) {
			continue;
		}

		Ret_FsApi = fslnx_deletefile(pCurPath);
		if (FST_STA_OK != Ret_FsApi) {
			DBG_ERR("delete %s failed\r\n", pCurPath);
			break;
		}
	}

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_StatFile(FST_FILE pFile, FST_FILE_STATUS *pFileStat)
{
	CHAR Drive;
	FSLINUX_FILE_INNER *pFileInner = (FSLINUX_FILE_INNER *)pFile;
	FSLINUX_ARG_STATFILE *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (0 == fslnx_IsValidFileHandle(pFileInner)) {
		DBG_WRN("Invalid file handle 0x%X\r\n", (unsigned int)pFileInner);
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = pFileInner->Drive;

	if (NULL == pFileStat) {
		DBG_WRN("pFileStat is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_STATFILE *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	pSharedArg->Linux_fd = pFileInner->Linux_fd;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_STATFILE;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = (INT32)pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	if (FST_STA_OK == Ret_FsApi) {
		memcpy(pFileStat, &(pSharedArg->OutStat), sizeof(pSharedArg->OutStat));
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_TruncFile(FST_FILE pFile, UINT64 NewSize)
{
	CHAR Drive;
	FSLINUX_FILE_INNER *pFileInner = (FSLINUX_FILE_INNER *)pFile;
	FSLINUX_ARG_TRUNCFILE *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (0 == fslnx_IsValidFileHandle(pFileInner)) {
		DBG_WRN("Invalid file handle 0x%X\r\n", (unsigned int)pFileInner);
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = pFileInner->Drive;

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_TRUNCFILE *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	pSharedArg->Linux_fd = pFileInner->Linux_fd;
	pSharedArg->NewSize = NewSize;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_TRUNCFILE;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = (INT32)pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	if (FST_STA_SDIO_ERR == pSharedArg->ret_fslnx) {
		FsLinux_Util_SetSDIOErr(Drive);
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	//check sdio status (curr)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_AllocFile(FST_FILE pFile, UINT64 Size, UINT32 Reserved1, UINT32 Reserved2)
{
	FSLINUX_API_ENTRY();

#if (USE_FALLOCATE == DISABLE)
	DBG_ERR("Not supported\r\n");

	FSLINUX_API_RETURN(FST_STA_ERROR);

#else
	//Note:
	//Do not support because "fallocate" of Linux fs/fat is not implemented
	//The Linux 4.5 patch of fs/fat may corrupt the file system when SD plugged out

	CHAR Drive;
	FSLINUX_FILE_INNER *pFileInner = (FSLINUX_FILE_INNER *)pFile;
	FSLINUX_ARG_ALLOCFILE *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if(0 == fslnx_IsValidFileHandle(pFileInner))
	{
	    DBG_WRN("Invalid file handle 0x%X\r\n", (unsigned int)pFileInner);
	    FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = pFileInner->Drive;

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if(NULL == pCtrlObj)
	{
	    DBG_ERR("Get CtrlObj failed\r\n");
	    FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if(0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED))
	{
	    DBG_WRN("Drv not opened\r\n");
	    FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_ALLOCFILE *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if(NULL == pSharedArg)
	{
	    DBG_ERR("Lock ArgBuf %c\r\n", Drive);
	    FsLinux_Unlock(pCtrlObj);
	    FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	pSharedArg->Linux_fd = pFileInner->Linux_fd;
	pSharedArg->mode = 0x01; //FALLOC_FL_KEEP_SIZE;
	pSharedArg->offset = 0;
	pSharedArg->len = Size;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_ALLOCFILE;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if(E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg))
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg))
#endif
	{
	    Ret_FsApi = (INT32)pSharedArg->ret_fslnx;
	}
	else
	{
	    DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
	    Ret_FsApi = FST_STA_ERROR;
	}

	if (FST_STA_SDIO_ERR == Ret_FsApi) {
		FsLinux_Util_SetSDIOErr(Drive);
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	//check sdio status (curr)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("sdio err is set\r\n");
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
#endif
}


INT32 FsLinux_FlushFile(FST_FILE pFile)
{
	CHAR Drive;
	FSLINUX_FILE_INNER *pFileInner = (FSLINUX_FILE_INNER *)pFile;
	FSLINUX_ARG_FLUSHFILE *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (0 == fslnx_IsValidFileHandle(pFileInner)) {
		DBG_WRN("Invalid file handle 0x%X\r\n", (unsigned int)pFileInner);
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = pFileInner->Drive;

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_FLUSHFILE *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	pSharedArg->Linux_fd = pFileInner->Linux_fd;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_FLUSHFILE;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = (INT32)pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	if (FST_STA_SDIO_ERR == pSharedArg->ret_fslnx) {
		FsLinux_Util_SetSDIOErr(Drive);
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	//check sdio status
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("sdio err is set\r\n");
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

UINT64 FsLinux_GetFileLen(char *pPath)
{
	CHAR Drive;
	FSLINUX_ARG_GETFILELEN *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	UINT64 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (NULL == pPath) {
		DBG_WRN("pPath is NULL\r\n");
		FSLINUX_API_RETURN(0);
	}
	Drive = *pPath;

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_GETFILELEN *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	FsLinux_Conv2LinuxPath(pSharedArg->szPath, sizeof(pSharedArg->szPath), pPath);

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_GETFILELEN;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_GetDateTime(char *pPath, UINT32 creDateTime[FST_FILE_DATETIME_MAX_ID], UINT32 modDateTime[FST_FILE_DATETIME_MAX_ID])
{
	CHAR Drive;
	FSLINUX_ARG_GETDATETIME *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (NULL == pPath) {
		DBG_WRN("pPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = *pPath;

	if (NULL == creDateTime) {
		DBG_WRN("creDateTime is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (NULL == modDateTime) {
		DBG_WRN("modDateTime is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_GETDATETIME *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	FsLinux_Conv2LinuxPath(pSharedArg->szPath, sizeof(pSharedArg->szPath), pPath);

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_GETDATETIME;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	if (FST_STA_OK == Ret_FsApi) {
		memcpy(creDateTime, pSharedArg->DT_Created, FST_FILE_DATETIME_MAX_ID * sizeof(UINT32));
		memcpy(modDateTime, pSharedArg->DT_Modified, FST_FILE_DATETIME_MAX_ID * sizeof(UINT32));
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_SetDateTime(char *pPath, UINT32 creDateTime[FST_FILE_DATETIME_MAX_ID], UINT32 modDateTime[FST_FILE_DATETIME_MAX_ID])
{
	CHAR Drive;
	FSLINUX_ARG_SETDATETIME *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (NULL == pPath) {
		DBG_WRN("pPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = *pPath;

	if (NULL == creDateTime) {
		DBG_WRN("creDateTime is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	DBG_WRN("creDateTime setting not supported\r\n");

	if (NULL == modDateTime) {
		DBG_WRN("modDateTime is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_SETDATETIME *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	FsLinux_Conv2LinuxPath(pSharedArg->szPath, sizeof(pSharedArg->szPath), pPath);
	memcpy(pSharedArg->DT_Created, creDateTime, FST_FILE_DATETIME_MAX_ID * sizeof(UINT32));
	memcpy(pSharedArg->DT_Modified, modDateTime, FST_FILE_DATETIME_MAX_ID * sizeof(UINT32));

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_SETDATETIME;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_MakeDir(CHAR *pPath)
{
	CHAR Drive;
	FSLINUX_ARG_MAKEDIR *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (NULL == pPath) {
		DBG_WRN("pPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = *pPath;

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_MAKEDIR *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	FsLinux_Conv2LinuxPath(pSharedArg->szPath, sizeof(pSharedArg->szPath), pPath);

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_MAKEDIR;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_DeleteDir(CHAR *pPath)
{
	CHAR Drive;
	FSLINUX_ARG_DELETEDIR *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (NULL == pPath) {
		DBG_WRN("pPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = *pPath;

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_DELETEDIR *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	FsLinux_Conv2LinuxPath(pSharedArg->szPath, sizeof(pSharedArg->szPath), pPath);

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_DELETEDIR;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}


INT32 FsLinux_RenameDir(CHAR *pNewname, CHAR *pPath, BOOL bIsOverwrite)
{
	CHAR Drive;
	FSLINUX_ARG_RENAMEDIR *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (NULL == pNewname) {
		DBG_WRN("pNewname is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (NULL == pPath) {
		DBG_WRN("pPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = *pPath;

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_RENAMEDIR *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	FSLINUX_STRCPY(pSharedArg->szNewName, pNewname, FSLINUX_LONGNAME_PATH_MAX_LENG);
	FsLinux_Conv2LinuxPath(pSharedArg->szPath, sizeof(pSharedArg->szPath), pPath);
	pSharedArg->bIsOverwrite = (unsigned int)bIsOverwrite;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_RENAMEDIR;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_RenameFile(CHAR *pNewname, CHAR *pPath, BOOL bIsOverwrite)
{
	CHAR Drive;
	FSLINUX_ARG_RENAMEFILE *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (NULL == pNewname) {
		DBG_WRN("pNewname is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (NULL == pPath) {
		DBG_WRN("pPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = *pPath;

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_RENAMEFILE *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	FSLINUX_STRCPY(pSharedArg->szNewName, pNewname, FSLINUX_LONGNAME_PATH_MAX_LENG);
	FsLinux_Conv2LinuxPath(pSharedArg->szPath, sizeof(pSharedArg->szPath), pPath);
	pSharedArg->bIsOverwrite = (unsigned int)bIsOverwrite;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_RENAMEFILE;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_MoveFile(CHAR *pSrcPath, CHAR *pDstPath)
{
	FSLINUX_ARG_MOVEFILE *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObjSrc, *pCtrlObjDst;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (NULL == pSrcPath) {
		DBG_WRN("pSrcPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (NULL == pDstPath) {
		DBG_WRN("pDstPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	pCtrlObjSrc = FsLinux_GetCtrlObjByDrive(*pSrcPath);
	if (NULL == pCtrlObjSrc) {
		DBG_WRN("Get CtrlObjSrc failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObjSrc->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv(Src) not opend\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	pCtrlObjDst = FsLinux_GetCtrlObjByDrive(*pDstPath);
	if (NULL == pCtrlObjDst) {
		DBG_ERR("Get CtrlObjDst failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObjDst->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv(Dst) not opend\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(*pSrcPath)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObjSrc);

	pSharedArg = (FSLINUX_ARG_MOVEFILE *)FsLinux_Buf_LocknGetArgBuf(*pSrcPath);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", *pSrcPath);
		FsLinux_Unlock(pCtrlObjSrc);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (pCtrlObjSrc != pCtrlObjDst) {
		FsLinux_Lock(pCtrlObjDst);
	}

	//set shared arguments
	FsLinux_Conv2LinuxPath(pSharedArg->szSrcPath, sizeof(pSharedArg->szSrcPath), pSrcPath);
	FsLinux_Conv2LinuxPath(pSharedArg->szDstPath, sizeof(pSharedArg->szDstPath), pDstPath);

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_MOVEFILE;
	IpcMsg.Drive = *pSrcPath;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	if (pCtrlObjSrc != pCtrlObjDst) {
		FsLinux_Unlock(pCtrlObjDst);
	}

	FsLinux_Buf_UnlockArgBuf(*pSrcPath);

	FsLinux_Unlock(pCtrlObjSrc);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

static FSLINUX_SEARCH_INNER *fslnx_searchfileopen(CHAR *pPath, FSLINUX_SEARCHFILE_BUFINFO *pSearchBuf, INT32 Direction)
{
	CHAR Drive = *pPath;
	FSLINUX_ARG_OPENDIR *pSharedArg = NULL;
	FSLINUX_IPCMSG IpcMsg;
	FSLINUX_SEARCH_INNER *pSearchInner;

	//get an empty file handle
	pSearchInner = fslnx_FindEmptySearchHandle(Drive);
	if (NULL == pSearchInner) {
		return NULL;
	}

	pSharedArg = (FSLINUX_ARG_OPENDIR *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		fslnx_ReleaseSearchHandle(pSearchInner);
		return NULL;
	}

	//reset the tmp buffer of searchfile
	pSearchBuf->TotalItems = 0;
	pSearchBuf->ReadItems = 0;
	pSearchBuf->bLastRound = 0;

	//set shared arguments
	FsLinux_Conv2LinuxPath(pSharedArg->szPath, sizeof(pSharedArg->szPath), pPath);
	pSharedArg->phy_pItemBuf = (unsigned int *)ipc_getPhyAddr((UINT32)(pSearchBuf->FDItem));//non-cached
	pSharedArg->direction = Direction;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_OPENDIR;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK != FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK != FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		pSharedArg->Linux_dirfd = 0;
	}

	if (0 == pSharedArg->Linux_dirfd) {
		//linux opendir failed
		fslnx_ReleaseSearchHandle(pSearchInner);
		FsLinux_Buf_UnlockArgBuf(Drive);
		return NULL;
	}

	//setup the search handle
	pSearchInner->Linux_dirfd = pSharedArg->Linux_dirfd;
	pSearchInner->pSearchBuf = pSearchBuf;
	pSearchInner->bUseIoctl = pSharedArg->bUseIoctl;
	pSearchInner->Drive = Drive;
	FSLINUX_STRCPY(pSearchInner->szLinuxPath, pSharedArg->szPath, FSLINUX_LONGNAME_PATH_MAX_LENG);

	FsLinux_Buf_UnlockArgBuf(Drive);

	return pSearchInner;
}

static INT32 fslnx_searchfile(FSLINUX_SEARCH_INNER *pSearchInner, UINT32 ItemNum)
{
	CHAR Drive = pSearchInner->Drive;
	FSLINUX_SEARCHFILE_BUFINFO *pSearchBuf = pSearchInner->pSearchBuf;
	FSLINUX_ARG_READDIR *pSharedArg = NULL;
	FSLINUX_IPCMSG IpcMsg;
	INT32 ret_fslnx;

	pSharedArg = (FSLINUX_ARG_READDIR *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		return FST_STA_ERROR;
	}

	//set shared arguments
	pSharedArg->Linux_dirfd = pSearchInner->Linux_dirfd;
	pSharedArg->phy_pItemBuf = (unsigned int *)ipc_getPhyAddr((UINT32)(pSearchBuf->FDItem));//non-cached
	pSharedArg->ItemNum = ItemNum;
	pSharedArg->bUseIoctl = (unsigned int)pSearchInner->bUseIoctl;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_READDIR;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		pSearchBuf->TotalItems = pSharedArg->ItemNum;
		pSearchBuf->ReadItems = 0;
		if (FST_STA_FS_NO_MORE_FILES == pSharedArg->ret_fslnx) {
			pSearchBuf->bLastRound = 1;
		}

		ret_fslnx = (INT32)pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		ret_fslnx = FST_STA_ERROR;
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	return ret_fslnx;
}


static INT32 fslnx_searchfiledel(FSLINUX_SEARCH_INNER *pSearchInner, UINT32 ItemNum)
{
	CHAR Drive = pSearchInner->Drive;
	FSLINUX_SEARCHFILE_BUFINFO *pSearchBuf = pSearchInner->pSearchBuf;
	FSLINUX_ARG_UNLINKATDIR *pSharedArg = NULL;
	FSLINUX_IPCMSG IpcMsg;
	INT32 ret_fslnx;

	pSharedArg = (FSLINUX_ARG_UNLINKATDIR *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		return FST_STA_ERROR;
	}

	//set shared arguments
	pSharedArg->Linux_dirfd = pSearchInner->Linux_dirfd;
	pSharedArg->phy_pItemBuf = (unsigned int *)ipc_getPhyAddr((UINT32)(pSearchBuf->FDItem));//non-cached
	pSharedArg->ItemNum = ItemNum;
	pSharedArg->bUseIoctl = (unsigned int)pSearchInner->bUseIoctl;
	FSLINUX_STRCPY(pSharedArg->szPath, pSearchInner->szLinuxPath, FSLINUX_LONGNAME_PATH_MAX_LENG);

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_UNLINKATDIR;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		ret_fslnx = (INT32)pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		ret_fslnx = FST_STA_ERROR;
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	return ret_fslnx;
}

static INT32 fslnx_searchfilelock(FSLINUX_SEARCH_INNER *pSearchInner, UINT32 ItemNum, BOOL bLock)
{
	CHAR Drive = pSearchInner->Drive;
	FSLINUX_SEARCHFILE_BUFINFO *pSearchBuf = pSearchInner->pSearchBuf;
	FSLINUX_ARG_LOCKDIR *pSharedArg = NULL;
	FSLINUX_IPCMSG IpcMsg;
	INT32 ret_fslnx;

	pSharedArg = (FSLINUX_ARG_LOCKDIR *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		return FST_STA_ERROR;
	}

	//set shared arguments
	pSharedArg->Linux_dirfd = pSearchInner->Linux_dirfd;
	pSharedArg->phy_pItemBuf = (unsigned int *)ipc_getPhyAddr((UINT32)(pSearchBuf->FDItem));//non-cached
	pSharedArg->ItemNum = ItemNum;
	pSharedArg->bLock = (int)bLock;
	pSharedArg->bUseIoctl = (unsigned int)pSearchInner->bUseIoctl;
	FSLINUX_STRCPY(pSharedArg->szPath, pSearchInner->szLinuxPath, FSLINUX_LONGNAME_PATH_MAX_LENG);

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_LOCKDIR;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		ret_fslnx = (INT32)pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		ret_fslnx = FST_STA_ERROR;
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	return ret_fslnx;
}

static INT32 fslnx_searchfileclose(FSLINUX_SEARCH_INNER *pSearchInner)
{
	CHAR Drive = pSearchInner->Drive;
	FSLINUX_ARG_CLOSEDIR *pSharedArg = NULL;
	FSLINUX_IPCMSG IpcMsg;
	INT32 ret_fslnx;

	pSharedArg = (FSLINUX_ARG_CLOSEDIR *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		return FST_STA_ERROR;
	}

	//set shared arguments
	pSharedArg->Linux_dirfd = pSearchInner->Linux_dirfd;
	pSharedArg->bUseIoctl = (unsigned int)pSearchInner->bUseIoctl;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_CLOSEDIR;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		ret_fslnx = (INT32)pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		ret_fslnx = FST_STA_ERROR;
	}

	fslnx_ReleaseSearchHandle(pSearchInner);

	FsLinux_Buf_UnlockArgBuf(Drive);

	return ret_fslnx;
}

FS_SEARCH_HDL FsLinux_SearchFileOpen(CHAR *pPath, FIND_DATA *pFindData, int Direction, UINT16 *pLongFilename)
{
	FSLINUX_SEARCHFILE_BUFINFO *pSearchBuf = NULL;
	FSLINUX_SEARCH_INNER *pSearchInner;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	CHAR Drive;

	FSLINUX_API_ENTRY();

	if (NULL == pPath) {
		DBG_WRN("pPath is NULL\r\n");
		FSLINUX_API_RETURN(NULL);
	}
	Drive = *pPath;

	if (NULL == pFindData && Direction != KFS_HANDLEONLY_SEARCH) {
		DBG_WRN("pFindData is NULL\r\n");
		FSLINUX_API_RETURN(NULL);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(NULL);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(NULL);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(NULL);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	//lock the tmp buffer when opening, unlock when closing
	pSearchBuf = (FSLINUX_SEARCHFILE_BUFINFO *)FsLinux_Buf_LocknGetTmpBuf(Drive);
	if (NULL == pSearchBuf) {
		DBG_ERR("Get pSearchBuf failed\r\n");
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(NULL);
	}

	pSearchInner = fslnx_searchfileopen(pPath, pSearchBuf, Direction);
	if (pSearchInner != NULL) {
		if (pLongFilename) {
			*pLongFilename = 0;    //not implemented, set to 0
		}

		if (Direction != KFS_HANDLEONLY_SEARCH) {
			memcpy(pFindData, &(pSearchBuf->FDItem[0].FindData), sizeof(FIND_DATA));    //copy the first data
		}
	}

	if (NULL == pSearchInner) {
		FsLinux_Buf_UnlockTmpBuf(Drive);
	}

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN((FS_SEARCH_HDL)pSearchInner);
}

int FsLinux_SearchFile(FS_SEARCH_HDL pSearch, FIND_DATA *pFindData, int Direction, UINT16 *pLongFilename)
{
	FSLINUX_SEARCH_INNER *pSearchInner = (FSLINUX_SEARCH_INNER *)pSearch;
	FSLINUX_SEARCHFILE_BUFINFO *pSearchBuf = NULL;
	FSLINUX_FIND_DATA_ITEM *pCurItem;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	UINT32 ItemNum_Req;
	UINT32 TmpBufSize;
	INT32 ret_fslnx;

	FSLINUX_API_ENTRY();

	if (0 == fslnx_IsValidSearchHandle(pSearchInner)) {
		DBG_WRN("Invalid search handle 0x%X\r\n", (unsigned int)pSearchInner);
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (NULL == pFindData) {
		DBG_WRN("pFindData is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (KFS_FORWARD_SEARCH != Direction) {
		DBG_WRN("unknown direction %d\r\n", Direction);
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(pSearchInner->Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(pSearchInner->Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSearchBuf = pSearchInner->pSearchBuf;//get buf from the handle, DO NOT lock again
	if (pSearchBuf->bLastRound && pSearchBuf->ReadItems == pSearchBuf->TotalItems) {
		//Reach the dir end, just return error without any messages
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	DBG_IND("ReadItems/TotalItems = %d/%d\r\n", pSearchBuf->ReadItems, pSearchBuf->TotalItems);

	//read multiple items if we reach the buffer end
	if (pSearchBuf->ReadItems >= pSearchBuf->TotalItems) {
		TmpBufSize = FsLinux_Buf_GetTmpSize(pSearchInner->Drive);
		if (TmpBufSize < sizeof(FSLINUX_SEARCHFILE_BUFINFO) + sizeof(FSLINUX_FIND_DATA_ITEM)) {
			//Except for BufInfo, at least one item space
			DBG_ERR("TmpBufSize %d not enough\r\n", TmpBufSize);
			FsLinux_Unlock(pCtrlObj);
			FSLINUX_API_RETURN(FST_STA_ERROR);
		}

		ItemNum_Req = (TmpBufSize - sizeof(FSLINUX_SEARCHFILE_BUFINFO)) / sizeof(FSLINUX_FIND_DATA_ITEM);

#if FSLINUX_FIXED_ITEMNUM
		DBG_ERR("ItemNum_Req = %d, fixed to %d\r\n", ItemNum_Req, FSLINUX_FIXED_ITEMNUM);
		ItemNum_Req = FSLINUX_FIXED_ITEMNUM;
#endif

		ret_fslnx = fslnx_searchfile(pSearchInner, ItemNum_Req);
		if (FST_STA_FS_NO_MORE_FILES == ret_fslnx) {
			pSearchBuf->bLastRound = 1;
		} else if (FST_STA_ERROR == ret_fslnx) {
			FsLinux_Unlock(pCtrlObj);
			FSLINUX_API_RETURN(FST_STA_ERROR);
		}
	}

	//fill the FindData
	if (pLongFilename) {
		*pLongFilename = 0;    //not implemented, set to 0
	}

	pCurItem = pSearchBuf->FDItem + pSearchBuf->ReadItems;
	memcpy(pFindData, &(pCurItem->FindData), sizeof(FIND_DATA));

	//offset to the next entry
	pSearchBuf->ReadItems++;

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(FST_STA_OK);
}

int FsLinux_SearchFileClose(FS_SEARCH_HDL pSearch)
{
	FSLINUX_SEARCH_INNER *pSearchInner = (FSLINUX_SEARCH_INNER *)pSearch;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	INT32 Ret_FsApi;
	CHAR Drive;

	FSLINUX_API_ENTRY();

	if (0 == fslnx_IsValidSearchHandle(pSearchInner)) {
		DBG_WRN("Invalid search handle 0x%X\r\n", (unsigned int)pSearchInner);
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = pSearchInner->Drive;

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	Ret_FsApi = fslnx_searchfileclose(pSearchInner);

	FsLinux_Buf_UnlockTmpBuf(Drive);//lock the tmp buffer when opening, unlock when closing

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

int FsLinux_SearchFileRewind(FS_SEARCH_HDL pSearch)
{
	CHAR Drive;
	FSLINUX_SEARCH_INNER *pSearchInner = (FSLINUX_SEARCH_INNER *)pSearch;
	FSLINUX_ARG_REWINDDIR *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (0 == fslnx_IsValidSearchHandle(pSearchInner)) {
		DBG_WRN("Invalid search handle 0x%X\r\n", (unsigned int)pSearchInner);
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = pSearchInner->Drive;

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_REWINDDIR *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	pSharedArg->Linux_dirfd = (unsigned int)pSearchInner->Linux_dirfd;
	pSharedArg->bUseIoctl = (unsigned int)pSearchInner->bUseIoctl;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_REWINDDIR;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = (INT32)pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	//reset the search buf
	pSearchInner->pSearchBuf->TotalItems = 0;
	pSearchInner->pSearchBuf->ReadItems = 0;
	pSearchInner->pSearchBuf->bLastRound = 0;

	FsLinux_Buf_UnlockArgBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_GetAttrib(CHAR *pPath, UINT8 *pAttrib)
{
	CHAR Drive;
	FSLINUX_ARG_GETATTRIB *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (NULL == pPath) {
		DBG_WRN("pPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = *pPath;

	if (NULL == pAttrib) {
		DBG_WRN("pAttrib is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_GETATTRIB *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	FsLinux_Conv2LinuxPath(pSharedArg->szPath, sizeof(pSharedArg->szPath), pPath);

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_GETATTRIB;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = (INT32)pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	if (FST_STA_OK == Ret_FsApi) {
		*pAttrib = pSharedArg->Attrib;
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_SetAttrib(CHAR *pPath, UINT8 Attrib, BOOL bSet)
{
	CHAR Drive;
	FSLINUX_ARG_SETATTRIB *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	if (NULL == pPath) {
		DBG_WRN("pPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = *pPath;

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_SETATTRIB *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	FsLinux_Conv2LinuxPath(pSharedArg->szPath, sizeof(pSharedArg->szPath), pPath);
	pSharedArg->Attrib = Attrib;
	pSharedArg->bSet = (int)bSet;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_SETATTRIB;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = (INT32)pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_CopyToByName(COPYTO_BYNAME_INFO *pCopyInfo)
{
	FSLINUX_ARG_COPYTOBYNAME *pSharedArg = NULL;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;
	FSLINUX_CTRL_OBJ *pCtrlObjSrc, *pCtrlObjDst;

	FSLINUX_API_ENTRY();

	if (NULL == pCopyInfo) {
		DBG_WRN("pCopyInfo is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (NULL == pCopyInfo->pSrcPath) {
		DBG_WRN("pCopyInfo.pSrcPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (NULL == pCopyInfo->pDstPath) {
		DBG_ERR("pCopyInfo.pDstPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	pCtrlObjSrc = FsLinux_GetCtrlObjByDrive(*(pCopyInfo->pSrcPath));
	if (NULL == pCtrlObjSrc) {
		DBG_ERR("Get CtrlObjSrc failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObjSrc->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv(Src) not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	pCtrlObjDst = FsLinux_GetCtrlObjByDrive(*(pCopyInfo->pDstPath));
	if (NULL == pCtrlObjDst) {
		DBG_ERR("Get CtrlObjDst failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObjDst->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv(Dst) not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(*(pCopyInfo->pSrcPath))) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(*(pCopyInfo->pDstPath))) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObjSrc);

	pSharedArg = (FSLINUX_ARG_COPYTOBYNAME *)FsLinux_Buf_LocknGetArgBuf(*pCopyInfo->pSrcPath);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", *pCopyInfo->pSrcPath);
		FsLinux_Unlock(pCtrlObjSrc);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (pCtrlObjSrc != pCtrlObjDst) {
		FsLinux_Lock(pCtrlObjDst);
	}

	//set shared arguments
	FsLinux_Conv2LinuxPath(pSharedArg->szSrcPath, sizeof(pSharedArg->szSrcPath), pCopyInfo->pSrcPath);
	FsLinux_Conv2LinuxPath(pSharedArg->szDstPath, sizeof(pSharedArg->szDstPath), pCopyInfo->pDstPath);
	pSharedArg->bDelete = pCopyInfo->bDelete;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_COPYTOBYNAME;
	IpcMsg.Drive = *(pCopyInfo->pSrcPath);
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = (INT32)pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	if (pCtrlObjSrc != pCtrlObjDst) {
		FsLinux_Unlock(pCtrlObjDst);
	}

	FsLinux_Buf_UnlockArgBuf(*pCopyInfo->pSrcPath);

	FsLinux_Unlock(pCtrlObjSrc);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

CHAR *FsLinux_GetCwd(void)
{
	FSLINUX_API_ENTRY();

	FSLINUX_API_RETURN(g_cwd);
}

VOID FsLinux_SetCwd(const CHAR *pPath)
{
	FSLINUX_API_ENTRY();

	FsLinux_ChDir((CHAR *)pPath);

	FSLINUX_API_RETURN_VOID();
}

int FsLinux_GetParentDir(const CHAR *pPath, CHAR *parentDir)
{
	FSLINUX_CTRL_OBJ *pCtrlObj;
	UINT32 uiSrcLen;
	const CHAR *pChar;
	const CHAR SlashSymbol = '\\';

	FSLINUX_API_ENTRY();

	if (NULL == pPath) {
		DBG_WRN("pPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (NULL == parentDir) {
		DBG_WRN("parentDir is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(*pPath);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	uiSrcLen = strlen(pPath);

	if (SlashSymbol == pPath[uiSrcLen - 1]) {
		pChar = pPath + uiSrcLen - 1;
	} else {
		pChar = pPath + uiSrcLen;
	}

	while (--pChar > pPath) {
		if (SlashSymbol == *pChar) {
			UINT32 uiParLen = (UINT32)pChar - (UINT32)pPath + 1;//with SlashSymbol
			memcpy(parentDir, pPath, uiParLen);//Ex: parentDir = "A:\BC\"
			parentDir[uiParLen] = '\0';
			DBG_IND("parentDir = %s, len = %d\r\n", parentDir, uiParLen);
			FsLinux_Unlock(pCtrlObj);
			FSLINUX_API_RETURN(FST_STA_OK);
		}
	}

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(FST_STA_ERROR);
}

int FsLinux_ChDir(CHAR *pPath)
{
	FSLINUX_CTRL_OBJ *pCtrlObj;

	FSLINUX_API_ENTRY();

	if (NULL == pPath) {
		DBG_WRN("pPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	if (strlen(pPath) > KFS_LONGNAME_PATH_MAX_LENG - 1) {
		DBG_WRN("strlen(pPath) = %d is too long\r\n", strlen(pPath));
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(*pPath);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	FSLINUX_STRCPY(g_cwd, pPath, KFS_LONGNAME_PATH_MAX_LENG);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(FST_STA_OK);
}

UINT32 FsLinux_GetParam(CHAR Drive, FST_PARM_ID param_id, UINT32 parm1)
{
	FSLINUX_CTRL_OBJ *pCtrlObj;
	UINT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(0);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	switch (param_id) {
	case FST_PARM_TASK_STS:
		if (kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_BUF_ARG_FREE)) {
			Ret_FsApi = FST_STA_OK;
		} else {
			Ret_FsApi = (UINT32)FST_STA_BUSY;
		}
		break;
	case FST_PARM_TASK_INIT_RDY:
		if (kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
			Ret_FsApi = TRUE;
		} else {
			Ret_FsApi = FALSE;
		}
		break;
	default:
		DBG_WRN("Unknown param_id(%d)\r\n", param_id);
		Ret_FsApi = 0;
	}

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_ScanDir(char *pPath, FileSys_ScanDirCB DirCB, BOOL bGetlong, UINT32 CBArg)
{
	FSLINUX_SEARCHFILE_BUFINFO *pSearchBuf = NULL;
	FSLINUX_SEARCH_INNER *pSearchInner;
	FSLINUX_FIND_DATA_ITEM *pCurItem;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	UINT32 ItemNum_Req, ItemLeft;
	INT32 ret_fslnx;
	INT32 Ret_FsApi = FST_STA_OK;//assume OK unless encounter errors
	UINT32 TmpBufSize;
	UINT16 DummyName = 0;
	CHAR Drive;
	BOOL bContinue = TRUE;
	BOOL bLastRound = FALSE;

	FSLINUX_API_ENTRY();

	if (NULL == pPath) {
		DBG_WRN("pPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = *pPath;

	if (NULL == DirCB) {
		DBG_WRN("DirCB is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	TmpBufSize = FsLinux_Buf_GetTmpSize(Drive);
	if (TmpBufSize < sizeof(FSLINUX_SEARCHFILE_BUFINFO) + sizeof(FSLINUX_FIND_DATA_ITEM)) {
		//Except for BufInfo, at least one item space
		DBG_ERR("TmpBufSize %d not enough\r\n", TmpBufSize);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	ItemNum_Req = (TmpBufSize - sizeof(FSLINUX_SEARCHFILE_BUFINFO)) / sizeof(FSLINUX_FIND_DATA_ITEM);

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	//try to lock the tmp buffer before opening a dir
	pSearchBuf = (FSLINUX_SEARCHFILE_BUFINFO *)FsLinux_Buf_LocknGetTmpBuf(Drive);
	if (NULL == pSearchBuf) {
		DBG_ERR("Get pSearchBuf failed\r\n");
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

#if FSLINUX_FIXED_ITEMNUM
	DBG_ERR("ItemNum_Req = %d, fixed to %d\r\n", ItemNum_Req, FSLINUX_FIXED_ITEMNUM);
	ItemNum_Req = FSLINUX_FIXED_ITEMNUM;
#endif

	pSearchInner = fslnx_searchfileopen(pPath, pSearchBuf, KFS_HANDLEONLY_SEARCH);
	if (NULL == pSearchInner) {
		FsLinux_Buf_UnlockTmpBuf(Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_OK);//return OK for fileDB to know that FileSys is fine
	}

	while (bContinue && !bLastRound) {
		ret_fslnx = fslnx_searchfile(pSearchInner, ItemNum_Req);
		if (FST_STA_ERROR == ret_fslnx) {
			break;//empty folder
		} else if (FST_STA_FS_NO_MORE_FILES == ret_fslnx) {
			bLastRound = TRUE;
		}

		ItemLeft = pSearchBuf->TotalItems;
		pCurItem = pSearchBuf->FDItem;

		while (bContinue && ItemLeft--) {
			DirCB((FIND_DATA *)(&(pCurItem->FindData)), &bContinue, &DummyName, CBArg);
			pCurItem++;
		}
	}

	fslnx_searchfileclose(pSearchInner);

	FsLinux_Buf_UnlockTmpBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_DelDirFiles(char *pPath, FileSys_DelDirCB DelCB)
{
	FSLINUX_SEARCHFILE_BUFINFO *pSearchBuf = NULL;
	FSLINUX_SEARCH_INNER *pSearchInner;
	FSLINUX_FIND_DATA_ITEM *pCurItem;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	UINT32 ItemNum_Req, ItemLeft;
	UINT32 TmpBufSize;
	INT32 ret_fslnx;
	INT32 Ret_FsApi = FST_STA_OK;//assume OK unless encounter errors
	CHAR Drive;
	BOOL bDelete = FALSE;
	BOOL bLastRound = FALSE;

	FSLINUX_API_ENTRY();

	if (NULL == pPath) {
		DBG_WRN("pPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = *pPath;

	if (NULL == DelCB) {
		bDelete = TRUE;    //delete all files
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	TmpBufSize = FsLinux_Buf_GetTmpSize(Drive);
	if (TmpBufSize < sizeof(FSLINUX_SEARCHFILE_BUFINFO) + sizeof(FSLINUX_FIND_DATA_ITEM)) {
		//Except for BufInfo, at least one item space
		DBG_ERR("TmpBufSize %d not enough\r\n", TmpBufSize);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	ItemNum_Req = (TmpBufSize - sizeof(FSLINUX_SEARCHFILE_BUFINFO)) / sizeof(FSLINUX_FIND_DATA_ITEM);

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	//try to lock the tmp buffer before opening a dir
	pSearchBuf = (FSLINUX_SEARCHFILE_BUFINFO *)FsLinux_Buf_LocknGetTmpBuf(Drive);
	if (NULL == pSearchBuf) {
		DBG_ERR("Get pSearchBuf failed\r\n");
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

#if FSLINUX_FIXED_ITEMNUM
	DBG_ERR("ItemNum_Req = %d, fixed to %d\r\n", ItemNum_Req, FSLINUX_FIXED_ITEMNUM);
	ItemNum_Req = FSLINUX_FIXED_ITEMNUM;
#endif

	pSearchInner = fslnx_searchfileopen(pPath, pSearchBuf, KFS_HANDLEONLY_SEARCH);
	if (NULL == pSearchInner) {
		FsLinux_Buf_UnlockTmpBuf(Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	while (!bLastRound) {
		ret_fslnx = fslnx_searchfile(pSearchInner, ItemNum_Req);
		if (FST_STA_ERROR == ret_fslnx) {
			DBG_ERR("fslnx_searchfile failed\r\n");
			Ret_FsApi = FST_STA_ERROR;
			break;
		} else if (FST_STA_FS_NO_MORE_FILES == ret_fslnx) {
			bLastRound = TRUE;
		}

		ItemLeft = pSearchBuf->TotalItems;
		pCurItem = pSearchBuf->FDItem;

		while (ItemLeft--) {
			if (DelCB) {
				DelCB((FIND_DATA *)(&(pCurItem->FindData)), &bDelete, 0, 0);
			}

			if (bDelete) {
				pCurItem->flag = 1;
			}

			pCurItem++;
		}

		ret_fslnx = fslnx_searchfiledel(pSearchInner, pSearchBuf->TotalItems);
		if (FST_STA_ERROR == ret_fslnx) {
			DBG_ERR("fslnx_searchfiledel failed\r\n");
			Ret_FsApi = FST_STA_ERROR;
			break;
		}
	}

	fslnx_searchfileclose(pSearchInner);

	FsLinux_Buf_UnlockTmpBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_LockDirFiles(char *pPath, BOOL isLock, FileSys_LockDirCB ApplyCB)
{
	FSLINUX_SEARCHFILE_BUFINFO *pSearchBuf = NULL;
	FSLINUX_SEARCH_INNER *pSearchInner;
	FSLINUX_FIND_DATA_ITEM *pCurItem;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	UINT32 ItemNum_Req, ItemLeft;
	UINT32 TmpBufSize;
	INT32 ret_fslnx;
	INT32 Ret_FsApi = FST_STA_OK;//assume OK unless encounter errors
	CHAR Drive;
	BOOL bApply = FALSE;
	BOOL bLastRound = FALSE;

	FSLINUX_API_ENTRY();

	if (NULL == pPath) {
		DBG_WRN("pPath is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	Drive = *pPath;

	if (NULL == ApplyCB) {
		bApply = TRUE;    //apply to all files
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	TmpBufSize = FsLinux_Buf_GetTmpSize(Drive);
	if (TmpBufSize < sizeof(FSLINUX_SEARCHFILE_BUFINFO) + sizeof(FSLINUX_FIND_DATA_ITEM)) {
		//Except for BufInfo, at least one item space
		DBG_ERR("TmpBufSize %d not enough\r\n", TmpBufSize);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	ItemNum_Req = (TmpBufSize - sizeof(FSLINUX_SEARCHFILE_BUFINFO)) / sizeof(FSLINUX_FIND_DATA_ITEM);

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	//try to lock the tmp buffer before opening a dir
	pSearchBuf = (FSLINUX_SEARCHFILE_BUFINFO *)FsLinux_Buf_LocknGetTmpBuf(Drive);
	if (NULL == pSearchBuf) {
		DBG_ERR("Get pSearchBuf failed\r\n");
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

#if FSLINUX_FIXED_ITEMNUM
	DBG_ERR("ItemNum_Req = %d, fixed to %d\r\n", ItemNum_Req, FSLINUX_FIXED_ITEMNUM);
	ItemNum_Req = FSLINUX_FIXED_ITEMNUM;
#endif

	pSearchInner = fslnx_searchfileopen(pPath, pSearchBuf, KFS_HANDLEONLY_SEARCH);
	if (NULL == pSearchInner) {
		FsLinux_Buf_UnlockTmpBuf(Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	while (!bLastRound) {
		ret_fslnx = fslnx_searchfile(pSearchInner, ItemNum_Req);
		if (FST_STA_ERROR == ret_fslnx) {
			DBG_ERR("fslnx_searchfile failed\r\n");
			Ret_FsApi = FST_STA_ERROR;
			break;
		} else if (FST_STA_FS_NO_MORE_FILES == ret_fslnx) {
			bLastRound = TRUE;
		}

		ItemLeft = pSearchBuf->TotalItems;
		pCurItem = pSearchBuf->FDItem;

		while (ItemLeft--) {
			if (ApplyCB) {
				ApplyCB((FIND_DATA *)(&(pCurItem->FindData)), &bApply, 0, 0);
			}

			if (bApply) {
				pCurItem->flag = 1;
			}

			pCurItem++;
		}

		ret_fslnx = fslnx_searchfilelock(pSearchInner, pSearchBuf->TotalItems, isLock);
		if (FST_STA_ERROR == ret_fslnx) {
			DBG_ERR("fslnx_searchfilelock failed\r\n");
			Ret_FsApi = FST_STA_ERROR;
			break;
		}
	}

	fslnx_searchfileclose(pSearchInner);

	FsLinux_Buf_UnlockTmpBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}



UINT64 FsLinux_GetDiskInfo(CHAR Drive, FST_INFO_ID InfoId)
{
	FSLINUX_ARG_GETDISKINFO *pSharedArg = NULL;
	FSLINUX_DRIVE_INFO *pDrvInfo;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_IPCMSG IpcMsg;
	UINT64 Ret_FsApi;

	FSLINUX_API_ENTRY();

	pDrvInfo = FsLinux_Buf_GetDrvInfo(Drive);
	if (NULL == pDrvInfo) {
		DBG_ERR("Get DrvInfo(%c) failed\r\n", Drive);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		if (InfoId == FST_INFO_IS_SDIO_ERR) {
			FSLINUX_API_RETURN(1);
		} else {
			DBG_ERR("invalid due to I/O error\r\n");
			FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
		}
	}

	//--------------------
	//This API will not lock the module to prevent blocked by other APIs

	pSharedArg = (FSLINUX_ARG_GETDISKINFO *)FsLinux_Buf_LocknGetInfoBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	pSharedArg->ret_fslnx = FST_STA_ERROR;  //set to error first, because when FsLinux module is closing,
	//Fslinux tsk will set flg forcibly and the value will be invalid
	FSLINUX_STRCPY(pSharedArg->szPath, pDrvInfo->FsInitParam.FSParam.szMountPath, FSLINUX_LONGNAME_PATH_MAX_LENG);

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_GETDISKINFO;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//req info and wait
#if FSLINUX_USE_IPC
	if (FsLinux_Ipc_ReqInfoWaitAck(&IpcMsg) != E_OK) {
#else
	if (FsLinux_Util_ReqInfoDirect(&IpcMsg) != E_OK) {
#endif
		DBG_ERR("FsLinux_Ipc_ReqInfoWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		pSharedArg->ret_fslnx = FST_STA_ERROR;
	}

	if (FST_STA_SDIO_ERR == pSharedArg->ret_fslnx) {
		FsLinux_Util_SetSDIOErr(Drive);
	}

	if (pSharedArg->ret_fslnx != FST_STA_OK) {
		DBG_WRN("Get disk info(0x%X) failed\r\n", InfoId);
		FsLinux_Buf_UnlockInfoBuf(Drive);
		//check sdio status (curr)
		if (FsLinux_Util_IsSDIOErr(Drive)) {
			DBG_ERR("sdio err is set\r\n");
			if (InfoId == FST_INFO_IS_SDIO_ERR) {
				FSLINUX_API_RETURN(1);
			}
		}
		FSLINUX_API_RETURN(0);
	}

	switch (InfoId) {
	case FST_INFO_DISK_RDY:
		Ret_FsApi = TRUE;
		break;

	case FST_INFO_DISK_SIZE:
		Ret_FsApi = (UINT64)pSharedArg->TotalClus * (UINT64)pSharedArg->ClusSize;
		break;

	case FST_INFO_FREE_SPACE:
		Ret_FsApi = (UINT64)pSharedArg->FreeClus * (UINT64)pSharedArg->ClusSize;
		break;

	case FST_INFO_VOLUME_ID:
		DBG_WRN("FST_INFO_VOLUME_ID not supported\r\n");
		Ret_FsApi = 0;
		break;

	case FST_INFO_CLUSTER_SIZE:
		Ret_FsApi = (UINT64)pSharedArg->ClusSize;
		break;

	case FST_INFO_PARTITION_TYPE:
		Ret_FsApi = (UINT64)pSharedArg->FsType;
		break;

	case FST_INFO_IS_SDIO_ERR:
		Ret_FsApi = (UINT64)FsLinux_Util_IsSDIOErr(Drive);
		break;

	default:
		DBG_WRN("InfoId(%d) not support\r\n", InfoId);
		Ret_FsApi = 0;
		break;
	}

	FsLinux_Buf_UnlockInfoBuf(Drive);

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_Benchmark(CHAR Drive, FS_HANDLE StrgDXH, UINT8 *pBuf, UINT32 BufSize)
{
	typedef struct {
		UINT32 SizeByte;
		UINT32 Dur_ms_R;
		UINT32 Dur_ms_R_max;
		UINT32 Dur_ms_R_min;
		UINT32 Dur_ms_W;
		UINT32 Dur_ms_W_max;
		UINT32 Dur_ms_W_min;
	} FsLinuxBMData;

#define BM_MIN_SIZE     512*1024
#define BM_MAX_SIZE     16*1024*1024
#define BM_SCALE_NUM    6
#define BM_REPEAT_NUM   10
#define BM_FILENAME_STR "%c:\\BM%05dK.SPD"

	CHAR            szPath[KFS_LONGNAME_PATH_MAX_LENG] = {0};
	FsLinuxBMData   BMData[BM_SCALE_NUM] = {0};
	UINT32          idx;
	UINT32          TmpSizeByte;
	UINT32          RepeatCount;
	UINT32          TmpDur_ms;
	INT32           Ret_FsApi = FST_STA_OK;
	FSLINUX_FILE_INNER  *pFileInner;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	VOS_TICK        t1, t2;

	FSLINUX_API_ENTRY();

	if (!pBuf) {
		DBG_WRN("pBuf is NULL\r\n");
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}
	if (BufSize < BM_MAX_SIZE) {
		DBG_WRN("BufSize 0x%x < Needed 0x%x\r\n", BufSize, BM_MAX_SIZE);
		FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	for (idx = 0; idx < BM_SCALE_NUM; idx++) {
		if (0 == idx) {
			BMData[idx].SizeByte = BM_MIN_SIZE;
			continue;
		}

		TmpSizeByte = 2 * BMData[idx - 1].SizeByte;
		if (TmpSizeByte > BM_MAX_SIZE) {
			DBG_WRN("Skip the size\r\n");
			break;
		}

		BMData[idx].SizeByte = TmpSizeByte;
	}

	DBG_DUMP("Benchmark : Start ----- (Linux)\r\n");

	for (RepeatCount = 0; RepeatCount < BM_REPEAT_NUM; RepeatCount++) {
		//Write all files
		for (idx = 0; idx < BM_SCALE_NUM; idx++) {
			TmpSizeByte = BMData[idx].SizeByte;
			if (0 == TmpSizeByte) {
				break;    //no further data
			}

			snprintf(szPath, sizeof(szPath), BM_FILENAME_STR, Drive, (int)(TmpSizeByte / 1024));
			DBG_DUMP("Write %s %10d Bytes (%02d/%02d)", szPath, TmpSizeByte, RepeatCount + 1, BM_REPEAT_NUM);

			//benchmark start
			vos_perf_mark(&t1);

			pFileInner = fslnx_openfile(szPath, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
			if (pFileInner == NULL) {
				DBG_ERR("\r\nOpen %s Fail\r\n", szPath);
				Ret_FsApi = FST_STA_ERROR;
				break;
			}

			if (TmpSizeByte != fslnx_writefile(pFileInner, pBuf, TmpSizeByte, FST_FLAG_NONE, NULL)) {
				DBG_ERR("\r\nWrite %s Fail\r\n", szPath);
				Ret_FsApi = FST_STA_ERROR;
				fslnx_closefile(pFileInner);
				break;
			}

			if (FSLINUX_STA_OK != fslnx_closefile(pFileInner)) {
				DBG_ERR("\r\nClose %s Fail\r\n", szPath);
				Ret_FsApi = FST_STA_ERROR;
				break;
			}

			//benchmark stop
			vos_perf_mark(&t2);
			TmpDur_ms = vos_perf_duration(t1, t2) / 1000;
			DBG_DUMP(" %4d ms\r\n", TmpDur_ms);

			if (TmpDur_ms > BMData[idx].Dur_ms_W_max) {
				BMData[idx].Dur_ms_W_max = TmpDur_ms;
			}
			if (TmpDur_ms < BMData[idx].Dur_ms_W_min || 0 == BMData[idx].Dur_ms_W_min) {
				BMData[idx].Dur_ms_W_min = TmpDur_ms;
			}

			BMData[idx].Dur_ms_W += TmpDur_ms;
			if (0 == BMData[idx].Dur_ms_W) {
				BMData[idx].Dur_ms_W = 1;
			}
		}

#if FSLINUX_DROP_CACHES
		/*
		 * Since benchmark is an offline tool, we use the drop_caches util to
		 * ensure that cached objects are dropped and real requests are executed.
		 * ( ) To free pagecache
		 *     -> echo 1 > /proc/sys/vm/drop_caches
		 * ( ) To free reclaimable slab objects (includes dentries and inodes)
		 *     -> echo 2 > /proc/sys/vm/drop_caches
		 * (*) To free slab objects and pagecache
		 *     -> echo 3 > /proc/sys/vm/drop_caches
		 */
		system("echo 3 > /proc/sys/vm/drop_caches");
#endif

		//Read all files
		for (idx = 0; idx < BM_SCALE_NUM; idx++) {
			TmpSizeByte = BMData[idx].SizeByte;
			if (0 == TmpSizeByte) {
				break;    //no further data
			}

			snprintf(szPath, sizeof(szPath), BM_FILENAME_STR, Drive, (int)(TmpSizeByte / 1024));
			DBG_DUMP("Read  %s %10d Bytes (%02d/%02d)", szPath, TmpSizeByte, RepeatCount + 1, BM_REPEAT_NUM);

			vos_perf_mark(&t1);

			pFileInner = fslnx_openfile(szPath, FST_OPEN_READ);
			if (pFileInner == NULL) {
				DBG_ERR("\r\nOpen %s Fail\r\n", szPath);
				Ret_FsApi = FST_STA_ERROR;
				break;
			}

			if (TmpSizeByte != fslnx_readfile(pFileInner, pBuf, TmpSizeByte, FST_FLAG_NONE, NULL)) {
				DBG_ERR("\r\nRead %s Fail\r\n", szPath);
				Ret_FsApi = FST_STA_ERROR;
				fslnx_closefile(pFileInner);
				break;
			}

			if (FSLINUX_STA_OK != fslnx_closefile(pFileInner)) {
				DBG_ERR("\r\nClose %s Fail\r\n", szPath);
				Ret_FsApi = FST_STA_ERROR;
				break;
			}
			vos_perf_mark(&t2);
			TmpDur_ms = vos_perf_duration(t1, t2) / 1000;
			DBG_DUMP(" %4d ms\r\n", TmpDur_ms);

			if (TmpDur_ms > BMData[idx].Dur_ms_R_max) {
				BMData[idx].Dur_ms_R_max = TmpDur_ms;
			}
			if (TmpDur_ms < BMData[idx].Dur_ms_R_min || 0 == BMData[idx].Dur_ms_R_min) {
				BMData[idx].Dur_ms_R_min = TmpDur_ms;
			}

			BMData[idx].Dur_ms_R += TmpDur_ms;

			if (FSLINUX_STA_OK != fslnx_deletefile(szPath)) {
				DBG_ERR("\r\nDelete %s Fail\r\n", szPath);
				Ret_FsApi = FST_STA_ERROR;
				break;
			}

			if (0 == BMData[idx].Dur_ms_R) {
				BMData[idx].Dur_ms_R = 1;
			}
		}
	}

	//report speed
	for (idx = 0; idx < BM_SCALE_NUM; idx++) {
		TmpSizeByte = BMData[idx].SizeByte;
		if (0 == TmpSizeByte) {
			break;    //no further data
		}

		snprintf(szPath, sizeof(szPath), BM_FILENAME_STR, Drive, (int)(TmpSizeByte / 1024));

		DBG_DUMP("^M%s: (%d KB) %d times\r\n", szPath, TmpSizeByte / 1024, BM_REPEAT_NUM);

		if (BMData[idx].Dur_ms_W) {
			DBG_DUMP("W: Min %5.1f MB/s, Max %5.1f MB/s, Avg %5.1f MB/s\r\n",
					 ((FLOAT)TmpSizeByte) / (BMData[idx].Dur_ms_W_max) / 1024,
					 ((FLOAT)TmpSizeByte) / (BMData[idx].Dur_ms_W_min) / 1024,
					 ((FLOAT)TmpSizeByte) / (BMData[idx].Dur_ms_W / BM_REPEAT_NUM) / 1024);
		}

		if (BMData[idx].Dur_ms_R) {
			DBG_DUMP("R: Min %5.1f MB/s, Max %5.1f MB/s, Avg %5.1f MB/s\r\n",
					 ((FLOAT)TmpSizeByte) / (BMData[idx].Dur_ms_R_max) / 1024,
					 ((FLOAT)TmpSizeByte) / (BMData[idx].Dur_ms_R_min) / 1024,
					 ((FLOAT)TmpSizeByte) / (BMData[idx].Dur_ms_R / BM_REPEAT_NUM) / 1024);
		}
	}

	DBG_DUMP("Benchmark : End -----\r\n");

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_GetStrgObj(CHAR Drive, FS_HANDLE *pStrgDXH)
{
	FSLINUX_DRIVE_INFO *pDrvInfo;
	INT32 Ret_FsApi;
	FSLINUX_CTRL_OBJ *pCtrlObj;

	FSLINUX_API_ENTRY();

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pDrvInfo = FsLinux_Buf_GetDrvInfo(Drive);

	if (pDrvInfo != NULL) {
		*pStrgDXH = pDrvInfo->StrgDXH;
		Ret_FsApi = FST_STA_OK;
	} else {
		DBG_ERR("GetDrvInfo failed\r\n");
		Ret_FsApi = FST_STA_ERROR;
	}

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_ChangeDisk(CHAR Drive, FS_HANDLE StrgDXH)
{
	FSLINUX_DRIVE_INFO *pDrvInfo;
	INT32 Ret_FsApi;
	FSLINUX_CTRL_OBJ *pCtrlObj;

	FSLINUX_API_ENTRY();

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pDrvInfo = FsLinux_Buf_GetDrvInfo(Drive);

	if (pDrvInfo != NULL) {
		pDrvInfo->StrgDXH = StrgDXH;
		Ret_FsApi = FST_STA_OK;
	} else {
		DBG_ERR("GetDrvInfo failed\r\n");
		Ret_FsApi = FST_STA_ERROR;
	}

	//check sdio status (curr)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("sdio err is set\r\n");
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}

INT32 FsLinux_FormatAndLabel(CHAR Drive, FS_HANDLE pStrgDXH, BOOL ChgDisk, CHAR *pLabelName)
{
	FSLINUX_ARG_FORMATANDLABEL *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_DRIVE_INFO *pDrvInfo;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	pDrvInfo = FsLinux_Buf_GetDrvInfo(Drive);
	if (NULL == pDrvInfo) {
		DBG_ERR("Get DrvInfo(%c) failed\r\n", Drive);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (pLabelName != NULL) {
		UINT32 LabelLen;

		DBG_IND("pLabelName = %s\r\n", pLabelName);

		LabelLen = strlen(pLabelName);
		if (LabelLen < 4  || LabelLen > 14) {
			DBG_ERR("LabelLen(%d) should be 4~14\r\n", LabelLen);
			FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
		}
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_FORMATANDLABEL *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	FSLINUX_STRCPY(pSharedArg->szMountPath, pDrvInfo->FsInitParam.FSParam.szMountPath, FSLINUX_LONGNAME_PATH_MAX_LENG);
	pSharedArg->bIsSupportExfat = pDrvInfo->FsInitParam.FSParam.bSupportExFAT;
	if (pLabelName) {
		FSLINUX_STRCPY(pSharedArg->szNewLabel, pLabelName + 3, FSLINUX_FILENAME_MAX_LENG);    //shift out "A:\"
	} else {
		pSharedArg->szNewLabel[0] = 0;
	}

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_FORMATANDLABEL;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg)) {
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg)) {
#endif
		Ret_FsApi = (INT32)pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	if (FST_STA_OK == Ret_FsApi) {
		FsLinux_Util_ClrSDIOErr(Drive);
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	//For FsUitron, the disk callback may return error status
	//For FsLinux, all errors are returned before, so OK is always returned by the callback function
	// 1 to updated IsFormatted status
	if (pDrvInfo->FsInitParam.pDiskErrCB) {
		pDrvInfo->FsInitParam.pDiskErrCB(FST_STA_OK, 1, 0, 0);
	}

	FSLINUX_API_RETURN(Ret_FsApi);
}

/*
INT32 FsLinux_SetDiskLabel(char* pLabelPath)
{
    FSLINUX_ARG_SETDISKLABEL *pSharedArg = NULL;
    FSLINUX_DRIVE_INFO *pDrvInfo = NULL;
    FSLINUX_IPCMSG IpcMsg;
    CHAR Drive;
    INT32 Ret_FsApi;
    FSLINUX_CTRL_OBJ *pCtrlObj;

    FSLINUX_API_ENTRY();

    if(NULL == pLabelPath)
    {
        DBG_WRN("pLabelPath is NULL\r\n");
        FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
    }
    Drive = *pLabelPath;

    pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
    if(NULL == pCtrlObj)
    {
        DBG_ERR("Get CtrlObj failed\r\n");
        FSLINUX_API_RETURN(FST_STA_ERROR);
    }

    pDrvInfo = FsLinux_Buf_GetDrvInfo(Drive);
    if(NULL == pDrvInfo)
    {
        DBG_ERR("Get DrvInfo(%c) failed\r\n", Drive);
        FSLINUX_API_RETURN(FST_STA_ERROR);
    }

    //----------lock----------
    FsLinux_LockArg(pCtrlObj);

    //set shared arguments
    pSharedArg = (FSLINUX_ARG_SETDISKLABEL *)FsLinux_Buf_GetArgAddr(Drive);
    FSLINUX_STRCPY(pSharedArg->szMountPath, pDrvInfo->FsInitParam.FSParam.szMountPath, FSLINUX_LONGNAME_PATH_MAX_LENG);
    FSLINUX_STRCPY(pSharedArg->szNewLabel, pLabelPath+3, FSLINUX_FILENAME_MAX_LENG);//shift out "A:\"
    pSharedArg->bIsSupportExfat = pDrvInfo->FsInitParam.FSParam.bSupportExFAT;

    //set ipc msg
    IpcMsg.CmdId = FSLINUX_CMDID_SETDISKLABEL;
    IpcMsg.Drive = Drive;
    IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

    //send cmd and wait
    if(E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg))
    {
        Ret_FsApi = (INT32)pSharedArg->ret_fslnx;
    }
    else
    {
        DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
        Ret_FsApi = FST_STA_ERROR;
    }

    FsLinux_Buf_UnlockArgBuf(Drive);
    //----------unlock----------

    FSLINUX_API_RETURN(Ret_FsApi);
}
*/

INT32 FsLinux_GetLabel(CHAR Drive, CHAR *pLabel)
{
	FSLINUX_ARG_GETLABEL *pSharedArg = NULL;
    FSLINUX_DRIVE_INFO *pDrvInfo = NULL;
    FSLINUX_IPCMSG IpcMsg;
    INT32 Ret_FsApi;
    FSLINUX_CTRL_OBJ *pCtrlObj;

    FSLINUX_API_ENTRY();

    if(NULL == pLabel)
    {
        DBG_WRN("pLabel is NULL\r\n");
        FSLINUX_API_RETURN(FST_STA_PARAM_ERR);
    }

	pDrvInfo = FsLinux_Buf_GetDrvInfo(Drive);
	if (NULL == pDrvInfo) {
		DBG_ERR("Get DrvInfo(%c) failed\r\n", Drive);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//check sdio status (prev)
	if (FsLinux_Util_IsSDIOErr(Drive)) {
		DBG_ERR("invalid due to I/O error\r\n");
		FSLINUX_API_RETURN(FST_STA_SDIO_ERR);
	}

    //----------lock----------
    FsLinux_Lock(pCtrlObj);

    //set shared arguments
	pSharedArg = (FSLINUX_ARG_GETLABEL *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

    FSLINUX_STRCPY(pSharedArg->szMountPath, pDrvInfo->FsInitParam.FSParam.szMountPath, FSLINUX_LONGNAME_PATH_MAX_LENG);

    //set ipc msg
    IpcMsg.CmdId = FSLINUX_CMDID_GETLABEL;
    IpcMsg.Drive = Drive;
    IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

    //send cmd and wait
#if FSLINUX_USE_IPC
    if(E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg))
#else
    if(E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg))
#endif
    {
		FSLINUX_STRCPY(pLabel, pSharedArg->szLabel, KFS_FILENAME_MAX_LENG);
        Ret_FsApi = (INT32)pSharedArg->ret_fslnx;
    }
    else
    {
        DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
        Ret_FsApi = FST_STA_ERROR;
    }

	FsLinux_Buf_UnlockArgBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
}


//obselete
ER FsLinux_SetParam(CHAR Drive, FST_PARM_ID param_id, UINT32 value)
{
#if FSLINUX_SETPARAM
	FSLINUX_ARG_SETPARAM *pSharedArg = NULL;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FSLINUX_DRIVE_INFO *pDrvInfo;
	FSLINUX_IPCMSG IpcMsg;
	INT32 Ret_FsApi;

	FSLINUX_API_ENTRY();

	pDrvInfo = FsLinux_Buf_GetDrvInfo(Drive);
	if (NULL == pDrvInfo) {
		DBG_ERR("Get DrvInfo(%c) failed\r\n", Drive);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	if (0 == kchk_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_API_OPENED)) {
		DBG_WRN("Drv not opened\r\n");
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}
	FsLinux_Tsk_WaitTskReady();

	//----------lock----------
	FsLinux_Lock(pCtrlObj);

	pSharedArg = (FSLINUX_ARG_SETPARAM *)FsLinux_Buf_LocknGetArgBuf(Drive);
	if (NULL == pSharedArg) {
		DBG_ERR("Lock ArgBuf %c\r\n", Drive);
		FsLinux_Unlock(pCtrlObj);
		FSLINUX_API_RETURN(FST_STA_ERROR);
	}

	//set shared arguments
	FSLINUX_STRCPY(pSharedArg->szMountPath, pDrvInfo->FsInitParam.FSParam.szMountPath, FSLINUX_LONGNAME_PATH_MAX_LENG);
	pSharedArg->bIsSupportExfat = pDrvInfo->FsInitParam.FSParam.bSupportExFAT;
	pSharedArg->param_id = param_id;
	pSharedArg->value = value;

	//set ipc msg
	IpcMsg.CmdId = FSLINUX_CMDID_SETPARAM;
	IpcMsg.Drive = Drive;
	IpcMsg.phy_ArgAddr = ipc_getPhyAddr((UINT32)pSharedArg);

	//send cmd and wait
#if FSLINUX_USE_IPC
	if (E_OK == FsLinux_Ipc_SendMsgWaitAck(&IpcMsg))
#else
	if (E_OK == FsLinux_Util_SendMsgDirect(&IpcMsg))
#endif
	{
		Ret_FsApi = (INT32)pSharedArg->ret_fslnx;
	} else {
		DBG_ERR("FsLinux_Ipc_SendMsgWaitAck(%d, %c) failed\r\n", IpcMsg.CmdId, IpcMsg.Drive);
		Ret_FsApi = FST_STA_ERROR;
	}

	FsLinux_Buf_UnlockArgBuf(Drive);

	FsLinux_Unlock(pCtrlObj);
	//----------unlock----------

	FSLINUX_API_RETURN(Ret_FsApi);
#else
	DBG_WRN("This function is obselete \r\n");
	return E_SYS;
#endif
}

//obselete
INT32 FsLinux_GetLongName(CHAR *pPath, UINT16 *wFileName)
{
	DBG_WRN("This function is obselete \r\n");
	*wFileName = 0;

	return FST_STA_ERROR;
}

//obselete
void FsLinux_RegisterCB(CHAR Drive, FileSys_CB CB)
{
	DBG_WRN("This function is obselete \r\n");
	/*
	PFSLINUX_DRIVE_INFO pDrvInfo;

	FSLINUX_API_ENTRY();

	pDrvInfo = FsLinux_Buf_GetDrvInfo(DrvIdx);
	if (!pDrvInfo)
	{
	    DBG_ERR("DrvIdx %d is invalid\r\n", DrvIdx);
	    FSLINUX_API_RETURN_VOID();
	}

	pDrvInfo->pWriteCB = CB;

	FSLINUX_API_RETURN_VOID();
	*/
}
