#ifndef _FSLINUX_IPC_H_
#define _FSLINUX_IPC_H_

#include "fslinux.h"
#include <nvtipc.h>

#if defined(_BSP_NA50902_)
#include <protected/nvtmem_protected.h>
#include <sys/cachectl.h>
#define FsLinux_GetNonCacheAddr(addr)	dma_getNonCacheAddr(addr)
#define FsLinux_GetCacheAddr(addr)		dma_getCacheAddr(addr)
#define FSLINUX_IPC_INVALID_ADDR		0
#else
#define FsLinux_GetNonCacheAddr(addr)	NvtIPC_GetNonCacheAddr(addr)
#define FsLinux_GetCacheAddr(addr)		NvtIPC_GetCacheAddr(addr)
#define FSLINUX_IPC_INVALID_ADDR		NVTIPC_MEM_INVALID_ADDR
#endif

#if FSLINUX_USE_IPC
#define FSLINUX_SEND_TARGET NVTIPC_SENDTO_CORE1

#ifdef FSLINUX_SIM
#define SIM_MSGID_DAEMON 0
#define SIM_MSGID_TESTER 1
#define SIM_SHMID    2364
#define SIM_SHMSZ    1024
#endif

int fslinux_ipc_Init(void);
void fslinux_ipc_Uninit(void);

int fslinux_ipc_WaitCmd(FSLINUX_IPCMSG *pRcvMsg);
int fslinux_ipc_AckCmd(FSLINUX_IPCMSG *pSendMsg);
#endif

#endif //_FSLINUX_IPC_H_