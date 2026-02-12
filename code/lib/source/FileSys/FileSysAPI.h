/*
    Copyright (c) 2014  Novatek Microelectronics Corporation

    @file FsLinuxAPI.h

    @version

    @date
*/

#ifndef _FSLINUXAPI_H
#define _FSLINUXAPI_H
#include "FileSysTsk.h"

/**
    Install FsLinux flag and semaphore id.

    @return void.
*/
extern void FsLinux_InstallID(void) _SECTION(".kercfg_text");

/**
    Open FsLinux task.

    This is FsLinux initialization function.

    @return
        - @b FST_STA_OK if success, others fail
*/
extern INT32 FsLinux_Open(CHAR Drive, FS_HANDLE StrgDXH, FILE_TSK_INIT_PARAM *pInitParam);

/**
    Close FsLinux task.

    @return
        - @b FST_STA_OK if success, others fail
*/
extern INT32 FsLinux_Close(CHAR Drive, UINT32 TimeOut);


extern FST_FILE FsLinux_OpenFile(CHAR *pPath, UINT32 flag);
extern INT32 FsLinux_CloseFile(FST_FILE pFile);
extern INT32 FsLinux_ReadFile(FST_FILE pFile, UINT8 *pBuf, UINT32 *pBufSize, UINT32 Flag, FileSys_CB CB);
extern INT32 FsLinux_WriteFile(FST_FILE pFile, UINT8 *pBuf, UINT32 *pBufSize, UINT32 Flag, FileSys_CB CB);
extern INT32 FsLinux_WaitFinish(CHAR Drive);

extern INT32 FsLinux_SeekFile(FST_FILE pFile, UINT64 offset, FST_SEEK_CMD fromwhere);
extern UINT64 FsLinux_TellFile(FST_FILE pFile);
extern INT32 FsLinux_DeleteFile(CHAR *pPath);
extern INT32 FsLinux_DeleteMultiFiles(FST_MULTI_FILES *pMultiFiles);
extern INT32 FsLinux_StatFile(FST_FILE pFile, FST_FILE_STATUS *pFileStat);
extern INT32 FsLinux_FlushFile(FST_FILE pFile);
extern INT32 FsLinux_TruncFile(FST_FILE pFile, UINT64 Size);
extern INT32 FsLinux_AllocFile(FST_FILE pFile, UINT64 Size, UINT32 Reserved1, UINT32 Reserved2);
extern UINT64 FsLinux_GetFileLen(char *pPath);

extern INT32 FsLinux_GetDateTime(char *pPath, UINT32 creDateTime[FST_FILE_DATETIME_MAX_ID], UINT32 modDateTime[FST_FILE_DATETIME_MAX_ID]);
extern INT32 FsLinux_SetDateTime(char *pPath, UINT32 creDateTime[FST_FILE_DATETIME_MAX_ID], UINT32 modDateTime[FST_FILE_DATETIME_MAX_ID]);

extern INT32 FsLinux_MakeDir(CHAR *pPath);
extern INT32 FsLinux_DeleteDir(CHAR *pPath);

extern INT32 FsLinux_MoveFile(CHAR *pSrcPath, CHAR *pDstPath);
extern INT32 FsLinux_RenameFile(CHAR *pNewname, CHAR *pPath, BOOL bIsOverwrite);
extern INT32 FsLinux_RenameDir(CHAR *pNewname, CHAR *pPath, BOOL bIsOverwrite);

extern FS_SEARCH_HDL FsLinux_SearchFileOpen(CHAR *pPath, FIND_DATA *pFindData, int Direction, UINT16 *pLongFilename);
extern int FsLinux_SearchFile(FS_SEARCH_HDL pSearch, FIND_DATA *pFindData, int Direction, UINT16 *pLongFilename);
extern int FsLinux_SearchFileClose(FS_SEARCH_HDL pSearch);
extern int FsLinux_SearchFileRewind(FS_SEARCH_HDL pSearch);

extern INT32 FsLinux_GetAttrib(CHAR *pPath, UINT8 *pAttrib);
extern INT32 FsLinux_SetAttrib(CHAR *pPath, UINT8 Attrib, BOOL bSet);

extern INT32 FsLinux_CopyToByName(COPYTO_BYNAME_INFO *pCopyInfo);

extern CHAR *FsLinux_GetCwd(void);
extern VOID FsLinux_SetCwd(const CHAR *pPath);
extern int FsLinux_GetParentDir(const CHAR *pPath, CHAR *parentDir);
extern int FsLinux_ChDir(CHAR *pPath);

extern UINT32 FsLinux_GetParam(CHAR Drive, FST_PARM_ID param_id, UINT32 parm1);

extern INT32 FsLinux_ScanDir(char *pPath, FileSys_ScanDirCB DirCB, BOOL bGetlong, UINT32 CBArg);
extern INT32 FsLinux_DelDirFiles(char *pPath, FileSys_DelDirCB DelCB);
extern INT32 FsLinux_LockDirFiles(char *pPath, BOOL isLock, FileSys_LockDirCB ApplyCB);

extern UINT64 FsLinux_GetDiskInfo(CHAR Drive, FST_INFO_ID InfoId);
extern INT32 FsLinux_Benchmark(CHAR Drive, FS_HANDLE StrgDXH, UINT8 *pBuf, UINT32 BufSize);
extern INT32 FsLinux_GetStrgObj(CHAR Drive, FS_HANDLE *pStrgDXH);
extern INT32 FsLinux_ChangeDisk(CHAR Drive, FS_HANDLE StrgDXH);
extern INT32 FsLinux_FormatAndLabel(CHAR Drive, FS_HANDLE pStrgDXH, BOOL ChgDisk, CHAR *pLabelName);
extern INT32 FsLinux_GetLabel(CHAR Drive, CHAR *pLabel);

//obselete functions
extern ER    FsLinux_SetParam(CHAR Drive, FST_PARM_ID param_id, UINT32 value);
extern INT32 FsLinux_GetLongName(CHAR *pPath, UINT16 *wFileName);
extern void  FsLinux_RegisterCB(CHAR Drive, FileSys_CB CB);

#endif /* _FSLINUXAPI_H  */
