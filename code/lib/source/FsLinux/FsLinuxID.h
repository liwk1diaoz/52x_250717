/**
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       FsLinuxID.h
    @ingroup    mISYSFsLinux

    @brief

    @note       Nothing.

    @date       2014/06/13
*/

/** \addtogroup mISYSFsLinux */
//@{

#ifndef _FSLINUXID_H
#define _FSLINUXID_H

#include "FsLinuxAPI.h"
#include "FsLinuxDebug.h"

#if FSLINUX_USE_IPC
#define FSLINUX_COMMON_FLGBIT_TSK_READY         FLGPTN_BIT(0)
#define FSLINUX_COMMON_FLGBIT_TSK_START_WAIT    FLGPTN_BIT(1)
#define FSLINUX_COMMON_FLGBIT_TSK_EXITED        FLGPTN_BIT(2)
#endif

#define FSLINUX_FLGBIT_API_OPENED       FLGPTN_BIT(0)
#define FSLINUX_FLGBIT_BUF_ARG_FREE     FLGPTN_BIT(1)
#define FSLINUX_FLGBIT_BUF_INFO_FREE    FLGPTN_BIT(2)
#define FSLINUX_FLGBIT_BUF_TMP_FREE     FLGPTN_BIT(3)
#if FSLINUX_USE_IPC
#define FSLINUX_FLGBIT_IPC_RECEIVED     FLGPTN_BIT(4)
#define FSLINUX_FLGBIT_IPC_INFO_RCVED   FLGPTN_BIT(5)
#endif

typedef struct {
	ID FlagID;
	SEM_HANDLE SemID;
} FSLINUX_CTRL_OBJ, *PFSLINUX_CTRL_OBJ;

#if FSLINUX_USE_IPC
extern ID _SECTION(".kercfg_data") FSLINUX_COMMON_FLG_ID;
extern ID _SECTION(".kercfg_data") FSLINUX_COMMON_TSK_ID;
#endif
extern FSLINUX_CTRL_OBJ _SECTION(".kercfg_data") gFsLinuxCtrlObj[FSLINUX_DRIVE_NUM];

PFSLINUX_CTRL_OBJ FsLinux_GetCtrlObjByDrive(CHAR Drive);

#endif //_FSLINUXID_H