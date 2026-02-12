#include "FsLinuxBuf.h"
#include "FsLinuxDebug.h"
#include "FsLinuxID.h"
#include "FsLinuxUtil.h"

#define FSLINUX_FOURCC(DrvIdx) MAKEFOURCC('F','S','L',(DrvIdx+1))

static FSLINUX_DRIVE_HANDLE gFsLinuxDrvHandle[FSLINUX_DRIVE_NUM] = {0};

ER FsLinux_Buf_Init(CHAR Drive, FS_HANDLE StrgDXH, FILE_TSK_INIT_PARAM *pInitParam)
{
	FSLINUX_DRIVE_INFO  *pDriveInfo;
	FS_INIT_PARAM       *pFSParam = &pInitParam->FSParam;
	UINT32  DrvIdx;
	UINT32  MountPathLen;
	UINT32  OpenListSize;
	UINT32  NonCacheBufAddr;
	CHAR    *pMountPath;

	DBG_FUNC_BEGIN("[BUF]\r\n");

	if (pFSParam->WorkBufSize < FSLINUX_POOLSIZE_MIN + pFSParam->MaxOpenedFileNum * FSLINUX_ONE_OPENLIST_SIZE) {
		DBG_ERR("MemPoolSize(%d) not enough, min = %d\r\n",
				pFSParam->WorkBufSize,
				FSLINUX_POOLSIZE_MIN + pFSParam->MaxOpenedFileNum * FSLINUX_ONE_OPENLIST_SIZE);
		return E_PAR;
	}

	if (Drive < FSLINUX_DRIVE_NAME_FIRST || Drive > FSLINUX_DRIVE_NAME_LAST) {
		DBG_ERR("Drive(%c) is invalid. (Shoule be '%c'~'%c')\r\n", Drive, FSLINUX_DRIVE_NAME_FIRST, FSLINUX_DRIVE_NAME_LAST);
		return E_PAR;
	}

	DrvIdx = Drive - FSLINUX_DRIVE_NAME_FIRST;//e.g. A: Idx = 0, B: Idx = 1

	if (FSLINUX_TAG_DRVHANDLE == gFsLinuxDrvHandle[DrvIdx].FsLinuxHandleTag) {
		DBG_ERR("Drive(%c) is opened\r\n", Drive);
		return E_SYS;
	}

	pMountPath = pFSParam->szMountPath;
	if ((pMountPath[0] != '/') ||
		(!FsLinux_IsPathNullTerm(pMountPath, KFS_LONGNAME_PATH_MAX_LENG))) {
		DBG_ERR("Invalid szMountPath\r\n");
		return E_PAR;
	}
	MountPathLen = strlen(pMountPath);

	if (pMountPath[MountPathLen - 1] != '/') {
		MountPathLen++;
	}

	if (MountPathLen >= KFS_LONGNAME_PATH_MAX_LENG) {
		DBG_ERR("MountPathLen(%d) >= MAX(%d)\r\n", MountPathLen, KFS_LONGNAME_PATH_MAX_LENG);
		return E_PAR;
	}
#if 0
	dma_flushWriteCache(pFSParam->WorkBuf, pFSParam->WorkBufSize);//invalidate the buffer
	NonCacheBufAddr = dma_getNonCacheAddr(pFSParam->WorkBuf);
#else
	//vos_cpu_dcache_sync(pFSParam->WorkBuf, pFSParam->WorkBufSize, VOS_DMA_TO_DEVICE);
	NonCacheBufAddr = pFSParam->WorkBuf;
#endif

	//set the Drive Handle
	gFsLinuxDrvHandle[DrvIdx].FsLinuxHandleTag = FSLINUX_TAG_DRVHANDLE;
	gFsLinuxDrvHandle[DrvIdx].pDrvInfo = (FSLINUX_DRIVE_INFO *)NonCacheBufAddr;//use non-cached address

	//set the Drive Info
	pDriveInfo = gFsLinuxDrvHandle[DrvIdx].pDrvInfo;
	memset(pDriveInfo, 0, FSLINUX_DRIVEINFO_SIZE);

	//(head tag)
	pDriveInfo->HeadTag = FSLINUX_FOURCC(DrvIdx);

	//(open init param)
	pDriveInfo->Drive = Drive;
	pDriveInfo->StrgDXH = StrgDXH;
	memcpy(&(pDriveInfo->FsInitParam), pInitParam, sizeof(FILE_TSK_INIT_PARAM));
	if (pDriveInfo->FsInitParam.FSParam.szMountPath[MountPathLen - 1] != '/') {
		//make sure the end of szMountPath is '/'
		pDriveInfo->FsInitParam.FSParam.szMountPath[MountPathLen - 1] = '/';
		pDriveInfo->FsInitParam.FSParam.szMountPath[MountPathLen] = '\0';
	}

	//(file handle)
	//memset(&(pDriveInfo->OpenFileList), 0, sizeof(pDriveInfo->OpenFileList));

	//(search handle)
	memset(&(pDriveInfo->OpenSearchList), 0, sizeof(pDriveInfo->OpenSearchList));

	//(RcvIpc)
	memset(&(pDriveInfo->RcvIpc), 0, sizeof(pDriveInfo->RcvIpc));

	//(file handle)
	OpenListSize = pDriveInfo->FsInitParam.FSParam.MaxOpenedFileNum * FSLINUX_ONE_OPENLIST_SIZE;
	pDriveInfo->pOpenFileList = (FSLINUX_FILE_INNER *)ALIGN_CEIL_4((UINT32)pDriveInfo + FSLINUX_DRIVEINFO_SIZE);
	memset((void *)pDriveInfo->pOpenFileList, 0, OpenListSize);

	//(arg buf)
	//pDriveInfo->ArgBuf.BufAddr = ALIGN_CEIL_16((UINT32)pDriveInfo + FSLINUX_DRIVEINFO_SIZE);
	pDriveInfo->ArgBuf.BufAddr = ALIGN_CEIL_16((UINT32)pDriveInfo->pOpenFileList + OpenListSize);
	pDriveInfo->ArgBuf.BufSize = FSLINUX_ARGBUF_SIZE;

	//(info buf)
	pDriveInfo->InfoBuf.BufAddr = ALIGN_CEIL_16(pDriveInfo->ArgBuf.BufAddr + pDriveInfo->ArgBuf.BufSize);
	pDriveInfo->InfoBuf.BufSize = FSLINUX_INFOBUF_SIZE;

	//(tmp buf)
	pDriveInfo->TmpBuf.BufAddr = ALIGN_CEIL_16(pDriveInfo->InfoBuf.BufAddr + pDriveInfo->InfoBuf.BufSize);
	pDriveInfo->TmpBuf.BufSize = (UINT32)NonCacheBufAddr + pFSParam->WorkBufSize - pDriveInfo->TmpBuf.BufAddr - FSLINUX_ENDTAG_SIZE;
	//check the TmpBuf(the last buffer) size
	if (pDriveInfo->TmpBuf.BufSize < FSLINUX_TMPBUF_SIZE_MIN) {
		DBG_ERR("TmpBuf size (%d) < min (%d)\r\n", pDriveInfo->TmpBuf.BufSize, FSLINUX_TMPBUF_SIZE_MIN);
		return E_PAR;
	}

	//(end tag)
	pDriveInfo->pEndTag = (UINT32 *)ALIGN_CEIL_4(pDriveInfo->TmpBuf.BufAddr + pDriveInfo->TmpBuf.BufSize);
	*pDriveInfo->pEndTag = FSLINUX_FOURCC(DrvIdx);

	// check whether the buffer mapping exceeds the working buffer boundary or not
	if ((UINT32)pDriveInfo->pEndTag + FSLINUX_ENDTAG_SIZE > (UINT32)NonCacheBufAddr + pFSParam->WorkBufSize) {
		DBG_ERR("Last fs buf(0x%X) exceeds WorkBuf end(0x%X)\r\n",
				(UINT32)pDriveInfo->pEndTag + FSLINUX_ENDTAG_SIZE,
				(UINT32)NonCacheBufAddr + pFSParam->WorkBufSize);
	}

	DBG_IND("Drive = %c, MountPath = %s\r\n", pDriveInfo->Drive, pDriveInfo->FsInitParam.FSParam.szMountPath);
	DBG_IND("pDriveInfo:    Addr = 0x%X, Size = 0x%X\r\n", pDriveInfo, FSLINUX_DRIVEINFO_SIZE);
	DBG_IND("pOpenFileList: Addr = 0x%X, Size = 0x%X\r\n", pDriveInfo->pOpenFileList, OpenListSize);
	DBG_IND("ArgBuf:        Addr = 0x%X, Size = 0x%X\r\n", pDriveInfo->ArgBuf.BufAddr, pDriveInfo->ArgBuf.BufSize);
	DBG_IND("InfoBuf:       Addr = 0x%X, Size = 0x%X\r\n", pDriveInfo->InfoBuf.BufAddr, pDriveInfo->InfoBuf.BufSize);
	DBG_IND("TmpBuf:        Addr = 0x%X, Size = 0x%X\r\n", pDriveInfo->TmpBuf.BufAddr, pDriveInfo->TmpBuf.BufSize);
	DBG_IND("pEndTag:       Addr = 0x%X, Size = 0x%X\r\n", pDriveInfo->pEndTag, FSLINUX_ENDTAG_SIZE);

	DBG_FUNC_END("[BUF]\r\n");
	return E_OK;
}

ER FsLinux_Buf_Uninit(CHAR Drive)
{
	FSLINUX_CTRL_OBJ *pCtrlObj;
	UINT32 DrvIdx = Drive - FSLINUX_DRIVE_NAME_FIRST;

	DBG_FUNC_BEGIN("[BUF]\r\n");

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		return E_SYS;
	}

	memset(&gFsLinuxDrvHandle[DrvIdx], 0, sizeof(FSLINUX_DRIVE_HANDLE));

	clr_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_BUF_ARG_FREE);
	clr_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_BUF_TMP_FREE);

	DBG_FUNC_END("[BUF]\r\n");
	return E_OK;
}

PFSLINUX_DRIVE_INFO FsLinux_Buf_GetDrvInfo(CHAR Drive)
{
	UINT32 DrvIdx;
	FSLINUX_DRIVE_INFO *pDriveInfo;

	if (Drive < FSLINUX_DRIVE_NAME_FIRST || Drive > FSLINUX_DRIVE_NAME_LAST) {
		DBG_ERR("Drive(%c) is invalid. (Shoule be '%c'~'%c')\r\n", Drive, FSLINUX_DRIVE_NAME_FIRST, FSLINUX_DRIVE_NAME_LAST);
		return NULL;
	}
	DrvIdx = Drive - FSLINUX_DRIVE_NAME_FIRST;

	if (gFsLinuxDrvHandle[DrvIdx].FsLinuxHandleTag != FSLINUX_TAG_DRVHANDLE) {
		DBG_ERR("Drive %c not initialized\r\n", Drive);
		return NULL;
	}

	pDriveInfo = (FSLINUX_DRIVE_INFO *)(gFsLinuxDrvHandle[DrvIdx].pDrvInfo);

	if (pDriveInfo->HeadTag != FSLINUX_FOURCC(DrvIdx)) {
		DBG_ERR("Drive info corrupted, &HeadTag = 0x%X, HeadTag = %d\r\n", (unsigned int)&pDriveInfo->HeadTag, pDriveInfo->HeadTag);
		return NULL;
	}

	if (pDriveInfo->pEndTag != NULL) {
		if (*pDriveInfo->pEndTag != FSLINUX_FOURCC(DrvIdx)) {
			DBG_ERR("Drive info corrupted, pEndTag = 0x%X, *pEndTag = %d\r\n", (unsigned int)pDriveInfo->pEndTag, *pDriveInfo->pEndTag);
			return NULL;
		}
	}

	return pDriveInfo;
}

UINT32 FsLinux_Buf_LocknGetArgBuf(CHAR Drive)
{
	FLGPTN FlgPtn;
	FSLINUX_DRIVE_INFO *pDriveInfo;
	FSLINUX_CTRL_OBJ *pCtrlObj;

	DBG_FUNC_BEGIN("[BUF]\r\n");

	pDriveInfo = FsLinux_Buf_GetDrvInfo(Drive);
	if (NULL == pDriveInfo) {
		DBG_ERR("FsLinux_Buf_GetDrvInfo failed\r\n");
		return 0;
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		return 0;
	}

	vos_flag_wait_interruptible(&FlgPtn, pCtrlObj->FlagID, FSLINUX_FLGBIT_BUF_ARG_FREE, TWF_ORW | TWF_CLR);

	DBG_FUNC_END("[BUF]\r\n");
	return pDriveInfo->ArgBuf.BufAddr;
}


void FsLinux_Buf_UnlockArgBuf(CHAR Drive)
{
	FSLINUX_CTRL_OBJ *pCtrlObj;

	DBG_FUNC_BEGIN("[BUF]\r\n");

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		return;
	}

	set_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_BUF_ARG_FREE);

	DBG_FUNC_END("[BUF]\r\n");
}

UINT32 FsLinux_Buf_LocknGetInfoBuf(CHAR Drive)
{
	FLGPTN FlgPtn;
	FSLINUX_DRIVE_INFO *pDriveInfo;
	FSLINUX_CTRL_OBJ *pCtrlObj;

	DBG_FUNC_BEGIN("[BUF]\r\n");

	pDriveInfo = FsLinux_Buf_GetDrvInfo(Drive);
	if (NULL == pDriveInfo) {
		DBG_ERR("FsLinux_Buf_GetDrvInfo failed\r\n");
		return 0;
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		return 0;
	}

	vos_flag_wait_interruptible(&FlgPtn, pCtrlObj->FlagID, FSLINUX_FLGBIT_BUF_INFO_FREE, TWF_ORW | TWF_CLR);

	DBG_FUNC_END("[BUF]\r\n");
	return pDriveInfo->InfoBuf.BufAddr;
}


void FsLinux_Buf_UnlockInfoBuf(CHAR Drive)
{
	FSLINUX_CTRL_OBJ *pCtrlObj;

	DBG_FUNC_BEGIN("[BUF]\r\n");

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		return;
	}

	set_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_BUF_INFO_FREE);

	DBG_FUNC_END("[BUF]\r\n");
}

/*
UINT32 FsLinux_Buf_GetArgAddr(CHAR Drive)
{
    FSLINUX_DRIVE_INFO *pDriveInfo = FsLinux_Buf_GetDrvInfo(Drive);

    if(NULL == pDriveInfo)
    {
        DBG_ERR("FsLinux_Buf_GetDrvInfo failed\r\n");
        return 0;
    }

    return pDriveInfo->ArgBuf.BufAddr;
}
*/

UINT32 FsLinux_Buf_GetTmpSize(CHAR Drive)
{
	FSLINUX_DRIVE_INFO *pDriveInfo = FsLinux_Buf_GetDrvInfo(Drive);

	if (NULL == pDriveInfo) {
		DBG_ERR("FsLinux_Buf_GetDrvInfo failed\r\n");
		return 0;
	}

	return pDriveInfo->TmpBuf.BufSize;
}

UINT32 FsLinux_Buf_LocknGetTmpBuf(CHAR Drive)
{
	FLGPTN FlgPtn;
	FSLINUX_DRIVE_INFO *pDriveInfo;
	FSLINUX_CTRL_OBJ *pCtrlObj;

	DBG_FUNC_BEGIN("[BUF]\r\n");

	pDriveInfo = FsLinux_Buf_GetDrvInfo(Drive);
	if (NULL == pDriveInfo) {
		DBG_ERR("FsLinux_Buf_GetDrvInfo failed\r\n");
		return 0;
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		return 0;
	}

	vos_flag_wait_interruptible(&FlgPtn, pCtrlObj->FlagID, FSLINUX_FLGBIT_BUF_TMP_FREE, TWF_ORW | TWF_CLR);

	DBG_FUNC_END("[BUF]\r\n");
	return pDriveInfo->TmpBuf.BufAddr;
}

void FsLinux_Buf_UnlockTmpBuf(CHAR Drive)
{
	FSLINUX_CTRL_OBJ *pCtrlObj;

	DBG_FUNC_BEGIN("[BUF]\r\n");
	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		return;
	}

	set_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_BUF_TMP_FREE);
	DBG_FUNC_END("[BUF]\r\n");
}