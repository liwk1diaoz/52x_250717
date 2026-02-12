#ifndef _FSLINUXBUF_H_
#define _FSLINUXBUF_H_

#include "FsLinuxAPI.h"

#define FSLINUX_DRV_A_IDX   0
#define FSLINUX_DRV_B_IDX   1

#define FSLINUX_TAG_DRVHANDLE       MAKEFOURCC('F','S','L','H')
#define FSLINUX_TAG_DRVINFO1        MAKEFOURCC('F','S','L','1')
#define FSLINUX_TAG_DRVINFO2        MAKEFOURCC('F','S','L','2')

#define FSLINUX_DRIVEINFO_SIZE      sizeof(FSLINUX_DRIVE_INFO)
#define FSLINUX_ONE_OPENLIST_SIZE   sizeof(FSLINUX_FILE_INNER) //at least one open file handle
#define FSLINUX_ARGBUF_SIZE         (1*1024)
#define FSLINUX_INFOBUF_SIZE        (512)
#define FSLINUX_TMPBUF_SIZE_MIN     (10*1024)
#define FSLINUX_ENDTAG_SIZE         sizeof(UINT32)
#define FSLINUX_POOLSIZE_MIN        (FSLINUX_DRIVEINFO_SIZE + FSLINUX_ARGBUF_SIZE + FSLINUX_TMPBUF_SIZE_MIN) //the pool size without the open lists

typedef struct {
	UINT32 Linux_fd;
	CHAR Drive;
} FSLINUX_FILE_INNER;

//FSLINUX_SEARCHFILE_BUFINFO is used for reading mutiple entries at one time
typedef struct {
	UINT32 TotalItems; //total items that can be read in the buffer
	UINT32 ReadItems; //the read item count which already been read
	UINT32 bLastRound;
	FSLINUX_FIND_DATA_ITEM FDItem[];
} FSLINUX_SEARCHFILE_BUFINFO;

//FSLINUX_SEARCH_INNER is the minimal info to list a directory
typedef struct {
	UINT32 Linux_dirfd;
	FSLINUX_SEARCHFILE_BUFINFO *pSearchBuf;
	BOOL bUseIoctl;
	CHAR Drive;
	CHAR szLinuxPath[FSLINUX_LONGNAME_PATH_MAX_LENG];
} FSLINUX_SEARCH_INNER;

typedef struct _FSLINUX_BUFF_INFO {
	UINT32 BufAddr;
	UINT32 BufSize;
} FSLINUX_BUFF_INFO, *PFSLINUX_BUFF_INFO;

typedef struct _FSLINUX_DRIVE_INFO {
	UINT32                  HeadTag;                                ///<  Head Tag to check if memory is valid
	CHAR                    Drive;
	FS_HANDLE               StrgDXH;
	FILE_TSK_INIT_PARAM     FsInitParam;
	FSLINUX_FILE_INNER      *pOpenFileList;
	FSLINUX_SEARCH_INNER    OpenSearchList[KFS_FSEARCH_MAX_NUM];
	FSLINUX_IPCMSG          RcvIpc;
	FSLINUX_BUFF_INFO       ArgBuf;
	FSLINUX_BUFF_INFO       InfoBuf;
	FSLINUX_BUFF_INFO       TmpBuf;
	FileSys_CB              pWriteCB;
	UINT32                  *pEndTag;                                 ///<  End Tag to check if memory is overwrit
} FSLINUX_DRIVE_INFO, *PFSLINUX_DRIVE_INFO;

typedef struct {
	UINT32              FsLinuxHandleTag;
	FSLINUX_DRIVE_INFO  *pDrvInfo;
} FSLINUX_DRIVE_HANDLE;


ER FsLinux_Buf_Init(CHAR Drive, FS_HANDLE StrgDXH, FILE_TSK_INIT_PARAM *pInitParam);
ER FsLinux_Buf_Uninit(CHAR Drive);

UINT32 FsLinux_Buf_LocknGetArgBuf(CHAR Drive);
void FsLinux_Buf_UnlockArgBuf(CHAR Drive);

UINT32 FsLinux_Buf_LocknGetInfoBuf(CHAR Drive);
void FsLinux_Buf_UnlockInfoBuf(CHAR Drive);

UINT32 FsLinux_Buf_GetTmpSize(CHAR Drive);
UINT32 FsLinux_Buf_LocknGetTmpBuf(CHAR Drive);
void FsLinux_Buf_UnlockTmpBuf(CHAR Drive);


PFSLINUX_DRIVE_INFO FsLinux_Buf_GetDrvInfo(CHAR Drive);

#endif //_FSLINUXBUF_H_

