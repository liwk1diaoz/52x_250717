#include "FileSysTsk.h"
#include "FileSysInt.h"
//#include "FileSysAPI.h"
//#include "../FsUitron/FsUitronAPI.h"
#include "../FsLinux/FsLinuxAPI.h"

#define THIS_DBGLVL         NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          filesys
#define __DBGLVL__          THIS_DBGLVL // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int filesys_debug_level = THIS_DBGLVL;

#if (THIS_DBGLVL > 2)
#include <time.h>
#define FILESYS_API_BEGIN(dummy)    UINT32 APIBeginTime = clock()
#define FILESYS_API_END(str)        DBG_DUMP("^GFSDBG %s %d\r\n", #str, (clock()-APIBeginTime)/1000)
#else
#define FILESYS_API_BEGIN(dummy)
#define FILESYS_API_END(str)
#endif

static FILESYS_OPS *g_pCurFs;
#if 0
static FILESYS_OPS g_FsUitron _SECTION(".kercfg_data") = {
	FST_FS_TYPE_UITRON,
	FsUitron_Open,
	FsUitron_Close,
	FsUitron_GetStrgObj,
	FsUitron_ChangeDisk,
	FsUitron_GetParam,
	FsUitron_SetParam,
	FsUitron_FormatAndLabel,
	FsUitron_GetDiskInfo,
	FsUitron_GetLongName,
	FsUitron_WaitFinish,
	FsUitron_MakeDir,
	FsUitron_ScanDir,
	FsUitron_DelDirFiles,
	FsUitron_LockDirFiles,
	FsUitron_GetAttrib,
	FsUitron_SetAttrib,
	FsUitron_GetDateTime,
	FsUitron_SetDateTime,
	FsUitron_DeleteFile,
	FsUitron_DeleteDir,
	FsUitron_RenameFile,
	FsUitron_RenameDir,
	FsUitron_MoveFile,
	FsUitron_GetFileLen,
	FsUitron_RegisterCB,
	FsUitron_OpenFile,
	FsUitron_CloseFile,
	FsUitron_ReadFile,
	FsUitron_WriteFile,
	FsUitron_SeekFile,
	FsUitron_TellFile,
	FsUitron_FlushFile,
	FsUitron_StatFile,
	FsUitron_TruncFile,
	FsUitron_CopyToByName,
	FsUitron_Benchmark,
	FsUitron_InstallID,
	FsUitron_GetCwd,
	FsUitron_SetCwd,
	FsUitron_GetParentDir,
	FsUitron_ChDir,
	FsUitron_SearchFileOpen,
	FsUitron_SearchFile,
	FsUitron_SearchFileClose,
	FsUitron_SearchFileRewind,
	FsUitron_AllocFile,
	FsUitron_GetLabel,
	FsUitron_DeleteMultiFiles,
};
#endif

static FILESYS_OPS g_FsLinux _SECTION(".kercfg_data") = {
	FST_FS_TYPE_LINUX,
	FsLinux_Open,
	FsLinux_Close,
	FsLinux_GetStrgObj,
	FsLinux_ChangeDisk,
	FsLinux_GetParam,
	FsLinux_SetParam,
	FsLinux_FormatAndLabel,
	FsLinux_GetDiskInfo,
	FsLinux_GetLongName,
	FsLinux_WaitFinish,
	FsLinux_MakeDir,
	FsLinux_ScanDir,
	FsLinux_DelDirFiles,
	FsLinux_LockDirFiles,
	FsLinux_GetAttrib,
	FsLinux_SetAttrib,
	FsLinux_GetDateTime,
	FsLinux_SetDateTime,
	FsLinux_DeleteFile,
	FsLinux_DeleteDir,
	FsLinux_RenameFile,
	FsLinux_RenameDir,
	FsLinux_MoveFile,
	FsLinux_GetFileLen,
	FsLinux_RegisterCB,
	FsLinux_OpenFile,
	FsLinux_CloseFile,
	FsLinux_ReadFile,
	FsLinux_WriteFile,
	FsLinux_SeekFile,
	FsLinux_TellFile,
	FsLinux_FlushFile,
	FsLinux_StatFile,
	FsLinux_TruncFile,
	FsLinux_CopyToByName,
	FsLinux_Benchmark,
	FsLinux_InstallID,
	FsLinux_GetCwd,
	FsLinux_SetCwd,
	FsLinux_GetParentDir,
	FsLinux_ChDir,
	FsLinux_SearchFileOpen,
	FsLinux_SearchFile,
	FsLinux_SearchFileClose,
	FsLinux_SearchFileRewind,
	FsLinux_AllocFile,
	FsLinux_GetLabel,
	FsLinux_DeleteMultiFiles,
};

#if 0
UINT32 FileSys_GetOPS_uITRON(void)
{
	FILESYS_API_BEGIN();

	FILESYS_API_END(GetOPS_U);
	return (UINT32)&g_FsUitron;
}
#endif

UINT32 FileSys_GetOPS_Linux(void)
{
	FILESYS_API_BEGIN();

	FILESYS_API_END(GetOPS_L);
	return (UINT32)&g_FsLinux;
}

INT32 FileSys_Init(UINT32 OpsHdl)
{
	FILESYS_API_BEGIN();

	if (0 == OpsHdl) {
		DBG_ERR("pFsOPS is NULL\r\n");
		return FST_STA_ERROR;
	}

	g_pCurFs = (FILESYS_OPS *)OpsHdl;

	FILESYS_API_END(Init);
	return FST_STA_OK;
}

FST_FS_TYPE FileSys_GetType(UINT32 OpsHdl)
{
	FILESYS_API_BEGIN();

	if (0 == OpsHdl) {
		DBG_ERR("pFsOPS is NULL\r\n");
		return FST_STA_ERROR;
	}

	FILESYS_API_END(GetType);

	return ((FILESYS_OPS *)OpsHdl)->FsType;
}


INT32 FileSys_OpenEx(CHAR Drive, FS_HANDLE TarStrgDXH, FILE_TSK_INIT_PARAM *pInitParam)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_ERR("FileSys not init\r\n");
		return FST_STA_ERROR;
	}

	Result =  g_pCurFs->Open(Drive, TarStrgDXH, pInitParam);

	//xFileSys_InstallCmd();

	FILESYS_API_END(OpenM);
	return Result;
}

INT32 FileSys_CloseEx(CHAR Drive, UINT32 TimeOut)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->Close(Drive, TimeOut);

	FILESYS_API_END(CloseM);
	return Result;
}

INT32 FileSys_GetStrgObjEx(CHAR Drive, FS_HANDLE *pStrgDXH)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->GetStrgObj(Drive, pStrgDXH);

	FILESYS_API_END(GetStrg);
	return Result;
}

INT32 FileSys_FormatAndLabel(CHAR Drive, FS_HANDLE StrgDXH, BOOL ChgDisk, CHAR *pLabelName)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->FormatAndLabel(Drive, StrgDXH, ChgDisk, pLabelName);

	FILESYS_API_END(FmtLabel);
	return Result;
}

INT32 FileSys_GetLabel(CHAR Drive, CHAR *pLabel)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->GetLabel(Drive, pLabel);

	FILESYS_API_END(GetLabel);
	return Result;
}

void FileSys_RegisterCBEx(CHAR Drive, FileSys_CB CB)
{
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return;
	}

	FILESYS_API_END(RegCB);
	return g_pCurFs->RegisterCB(Drive, CB);
}

INT32 FileSys_WaitFinishEx(CHAR Drive)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->WaitFinish(Drive);

	FILESYS_API_END(WaitFin);
	return Result;
}

INT32 FileSys_ChangeDiskEx(CHAR Drive, FS_HANDLE StrgDXH)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->ChangeDisk(Drive, StrgDXH);

	FILESYS_API_END(ChDisk);
	return Result;
}

INT32 FileSys_DeleteFile(char *pPath)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->DeleteFile(pPath);

	FILESYS_API_END(DelFile);
	return Result;
}

INT32 FileSys_DeleteMultiFiles(FST_MULTI_FILES *pMultiFiles)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->DeleteMultiFiles(pMultiFiles);

	FILESYS_API_END(DelMulFile);
	return Result;
}

INT32 FileSys_DeleteDir(char *pPath)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->DeleteDir(pPath);

	FILESYS_API_END(DelDir);
	return Result;
}

INT32 FileSys_GetAttrib(char *pPath, UINT8 *pAttrib)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->GetAttrib(pPath, pAttrib);

	FILESYS_API_END(GetAttr);
	return Result;
}

INT32 FileSys_GetLongName(char *pPath, UINT16 *wFileName)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->GetLongName(pPath, wFileName);

	FILESYS_API_END(GetLFN);
	return Result;
}

INT32 FileSys_SetAttrib(char *pPath, UINT8 ubAttrib, BOOL bSet)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->SetAttrib(pPath, ubAttrib, bSet);

	FILESYS_API_END(SetAttr);
	return Result;
}

INT32 FileSys_GetDateTime(char *pPath, UINT32  creDateTime[FST_FILE_DATETIME_MAX_ID], UINT32  modDateTime[FST_FILE_DATETIME_MAX_ID])
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->GetDateTime(pPath, creDateTime, modDateTime);

	FILESYS_API_END(GetDT);
	return Result;
}

INT32 FileSys_SetDateTime(char *pPath, UINT32  creDateTime[FST_FILE_DATETIME_MAX_ID], UINT32  modDateTime[FST_FILE_DATETIME_MAX_ID])
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->SetDateTime(pPath, creDateTime, modDateTime);

	FILESYS_API_END(SetDT);
	return Result;
}


INT32 FileSys_CopyToByName(COPYTO_BYNAME_INFO *pCopyInfo)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->CopyToByName(pCopyInfo);

	FILESYS_API_END(CpByName);
	return Result;
}

INT32 FileSys_RenameFile(char *pNewname, char *pPath, BOOL bIsOverwrite)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->RenameFile(pNewname, pPath, bIsOverwrite);

	FILESYS_API_END(RenFile);
	return Result;
}

INT32 FileSys_RenameDir(char *pNewname, char *pPath, BOOL bIsOverwrite)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->RenameDir(pNewname, pPath, bIsOverwrite);

	FILESYS_API_END(RenDir);
	return Result;
}

INT32 FileSys_MoveFile(char *pSrcPath, char *pDstPath)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->MoveFile(pSrcPath, pDstPath);

	FILESYS_API_END(MvFile);
	return Result;
}

INT32 FileSys_ScanDir(char *pPath, FileSys_ScanDirCB DirCB, BOOL bGetlong, UINT32 CBArg)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->ScanDir(pPath, DirCB, bGetlong, CBArg);

	FILESYS_API_END(ScanDir);
	return Result;
}

FST_FILE FileSys_OpenFile(char *filename, UINT32 flag)
{
	FST_FILE Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return NULL;
	}

	Result = g_pCurFs->OpenFile(filename, flag);

	FILESYS_API_END(OpenF);
	return Result;
}

INT32 FileSys_CloseFile(FST_FILE pFile)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->CloseFile(pFile);

	FILESYS_API_END(CloseF);
	return Result;
}

INT32 FileSys_ReadFile(FST_FILE pFile, UINT8 *pBuf, UINT32 *pBufSize, UINT32 Flag, FileSys_CB CB)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->ReadFile(pFile, pBuf, pBufSize, Flag, CB);

	FILESYS_API_END(Read);
	return Result;
}

INT32 FileSys_WriteFile(FST_FILE pFile, UINT8 *pBuf, UINT32 *pBufSize, UINT32 Flag, FileSys_CB CB)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->WriteFile(pFile, pBuf, pBufSize, Flag, CB);

	FILESYS_API_END(Write);
	return Result;
}

INT32 FileSys_SeekFile(FST_FILE pFile, UINT64 Pos, FST_SEEK_CMD fromwhere)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->SeekFile(pFile, Pos, fromwhere);

	FILESYS_API_END(Seek);
	return Result;
}

UINT64 FileSys_TellFile(FST_FILE pFile)
{
	UINT64 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->TellFile(pFile);

	FILESYS_API_END(Tell);
	return Result;
}

INT32 FileSys_FlushFile(FST_FILE pFile)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->FlushFile(pFile);

	FILESYS_API_END(Flush);
	return Result;
}

INT32 FileSys_StatFile(FST_FILE pFile, FST_FILE_STATUS *pFileStat)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->StatFile(pFile, pFileStat);

	FILESYS_API_END(Stat);
	return Result;
}

INT32 FileSys_TruncFile(FST_FILE pFile, UINT64 NewSize)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->TruncFile(pFile, NewSize);

	FILESYS_API_END(Trunc);
	return Result;
}

INT32 FileSys_AllocFile(FST_FILE pFile, UINT64 NewSize, UINT32 Reserved1, UINT32 Reserved2)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->AllocFile(pFile, NewSize, Reserved1, Reserved2);

	FILESYS_API_END(Alloc);
	return Result;
}

INT32 FileSys_BenchmarkEx(CHAR Drive, FS_HANDLE StrgDXH, UINT8 *pBuf, UINT32 BufSize)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->Benchmark(Drive, StrgDXH, pBuf, BufSize);

	FILESYS_API_END(Bench);
	return Result;
}

ER FileSys_SetParamEx(CHAR Drive, FST_PARM_ID param_id, UINT32 value)
{
	ER Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->SetParam(Drive, param_id, value);

	FILESYS_API_END(SetParm);
	return Result;
}

UINT64 FileSys_GetDiskInfoEx(CHAR Drive, FST_INFO_ID info_id)
{
	UINT64 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return 0;
	}

	Result = g_pCurFs->GetDiskInfo(Drive, info_id);

	FILESYS_API_END(GetInfo);
	return Result;
}

UINT32   FileSys_GetParamEx(CHAR Drive, FST_PARM_ID param_id, UINT32 parm1)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return 0;
	}

	Result = g_pCurFs->GetParam(Drive, param_id, parm1);

	FILESYS_API_END(GetParm);
	return Result;
}

INT32 FileSys_DelDirFiles(char *pPath, FileSys_DelDirCB CB)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->DelDirFiles(pPath, CB);

	FILESYS_API_END(DelDirF);
	return Result;
}

INT32 FileSys_LockDirFiles(char *pPath, BOOL isLock, FileSys_LockDirCB CB)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->LockDirFiles(pPath, isLock, CB);

	FILESYS_API_END(LockDirF);
	return Result;
}

UINT64 FileSys_GetFileLen(char *pPath)
{
	UINT64 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return 0;
	}

	Result = g_pCurFs->GetFileLen(pPath);

	FILESYS_API_END(GetFileLen);
	return Result;
}

INT32 FileSys_MakeDir(char *pPath)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->MakeDir(pPath);

	FILESYS_API_END(MkDir);
	return Result;
}

void FileSys_InstallID(UINT32 OpsHdl)
{
	FILESYS_API_BEGIN();

	if (0 == OpsHdl) {
		DBG_ERR("pFsOPS is NULL\r\n");
		return;
	}

	((FILESYS_OPS *)OpsHdl)->InstallID();

	FILESYS_API_END(InstallID);
}

CHAR *FileSys_GetCwd(VOID)
{
	CHAR *Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return NULL;
	}

	Result = g_pCurFs->GetCwd();

	FILESYS_API_END(GetCwd);
	return Result;
}

VOID FileSys_SetCwd(const CHAR *pPath)
{
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return;
	}

	FILESYS_API_END(SetCwd);
	return g_pCurFs->SetCwd(pPath);
}

int FileSys_GetParentDir(const CHAR *pPath, CHAR *parentDir)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->GetParentDir(pPath, parentDir);

	FILESYS_API_END(GetParDir);
	return Result;
}

int FileSys_ChDir(CHAR *pPath)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->Chdir(pPath);

	FILESYS_API_END(ChDir);
	return Result;
}

FS_SEARCH_HDL FileSys_SearchFileOpen(CHAR *pPath, FIND_DATA *pFindData, int Direction, UINT16 *pLongFilename)
{
	FS_SEARCH_HDL Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return NULL;
	}

	Result = g_pCurFs->SearchFileOpen(pPath, pFindData, Direction, pLongFilename);

	FILESYS_API_END(SrhOpen);
	return Result;
}

int FileSys_SearchFile(FS_SEARCH_HDL pSearch, FIND_DATA *pFindData, int Direction, UINT16 *pLongFilename)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->SearchFile(pSearch, pFindData, Direction, pLongFilename);

	FILESYS_API_END(SrhFile);
	return Result;
}

int FileSys_SearchFileClose(FS_SEARCH_HDL pSearch)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->SearchFileClose(pSearch);

	FILESYS_API_END(SrhClose);
	return Result;
}

int FileSys_SearchFileRewind(FS_SEARCH_HDL pSearch)
{
	INT32 Result;
	FILESYS_API_BEGIN();

	if (NULL == g_pCurFs) {
		DBG_WRN("FileSys_Init not ready\r\n");
		return FST_STA_ERROR;
	}

	Result = g_pCurFs->SearchFileRewind(pSearch);

	FILESYS_API_END(SrhRewind);
	return Result;
}



