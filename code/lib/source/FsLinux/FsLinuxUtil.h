#ifndef _FSLINUXUTIL_H_
#define _FSLINUXUTIL_H_
#if defined (__UITRON) || defined (__ECOS)
#include "dma_protected.h"

//temp definition
#define ipc_getPhyAddr(addr) dma_getPhyAddr(addr)
#else
#if FSLINUX_USE_IPC
#define ipc_getPhyAddr(addr) vos_cpu_get_phy_addr(addr)
#else
#define ipc_getPhyAddr(addr) (addr)
#endif
#endif
#define FSLINUX_STRCPY(dst, src, dst_size)  do { strncpy(dst, src, dst_size-1); dst[dst_size-1] = '\0'; } while(0)

BOOL FsLinux_IsDefSync(VOID);

UINT32 FsLinux_Date_Dos2Unix(UINT16 DosTime, UINT16 DosDate);
void FsLinux_Date_Unix2Dos(UINT32 UnixTimeStamp, UINT16 *OutDosTime, UINT16 *OutDosDate);

UINT32 FsLinux_wcslen(const UINT16 *wcs);
BOOL FsLinux_ConvToSingleByte(char *pDstName, UINT16 *pSrcName);

BOOL FsLinux_Conv2LinuxPath(CHAR *pPath_Dst, UINT32 DstSize, const CHAR *pPath_Src);
BOOL FsLinux_IsPathNullTerm(CHAR *pPath, UINT32 MaxLen);

BOOL FsLinux_FlushReadCache(UINT32 Addr, UINT32 Size);
BOOL FsLinux_FlushWriteCache(UINT32 Addr, UINT32 Size);

BOOL FsLinux_Util_ClrSDIOErr(CHAR Drive);
BOOL FsLinux_Util_SetSDIOErr(CHAR Drive);
BOOL FsLinux_Util_IsSDIOErr(CHAR Drive);

ER FsLinux_Util_SendMsgDirect(const FSLINUX_IPCMSG *pSendMsg);
ER FsLinux_Util_ReqInfoDirect(const FSLINUX_IPCMSG *pSendMsg);

#endif //_FSLINUXUTIL_H_

