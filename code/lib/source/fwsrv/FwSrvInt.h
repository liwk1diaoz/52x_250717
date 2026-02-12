/**
    FwSrv, Internal function declare

    @file       FwSrvInt.c
    @ingroup    mFWSRV

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _FWSRVINT_H
#define _FWSRVINT_H

#include <stdio.h>
#include <string.h>
#include <kwrap/nvt_type.h>
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include <kwrap/task.h>
#include <kwrap/cpu.h>
#include <kwrap/verinfo.h>
#include <comm/bin_info.h>
#include <comm/shm_info.h>
#include <BinaryFormat.h>
#include <MemCheck.h>
#include <lz.h>
#include <zlib.h>
#include <FwSrvApi.h>
#include <PartLoad.h>
#include <nvtpack.h>
#include <compiler.h>
#include <libfdt.h>
#include <rtosfdt.h>
#include "FwSrvCmd.h"

/**
*   Internal configuration
*/
//@{
#define THIS_DBGLVL            NVT_DBG_WRN
#define CFG_FWSRV_INIT_KEY     MAKEFOURCC('F','W','S','V') ///< a key value 'F','W','S','V' for indicating system initial.
//@}

/**
*   Partial load internal configuration
*/
//@{
#define CFG_FWSRV_PL_MAX_SECTIONS 20 ///< allow maximum section number
#define CFG_FWSRV_GZ_BLK_SIZE 0x8000 //ecah gz uncompress interval size
//@}

#define __MODULE__          FwSrv
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

#define PRI_FWSRV             10
#define STKSIZE_FWSRV         4096

/**
    Flag Pattern
*/
//@{
//Task Operation
#define FLGFWSRV_OPEN      FLGPTN_BIT(0)   //not used, currently.
#define FLGFWSRV_CLOSE     FLGPTN_BIT(1)   //notify task don't do any command for task close
#define FLGFWSRV_SUSPEND   FLGPTN_BIT(2)   //notify task don't do any command for task suspend
#define FLGFWSRV_RESUME    FLGPTN_BIT(3)   //notify task start to receive command
#define FLGFWSRV_CMD       FLGPTN_BIT(4)   //notify task there's command coming
#define FLGFWSRV_IDLE      FLGPTN_BIT(5)   //task is idle or just finish his job
#define FLGFWSRV_POLLING   FLGPTN_BIT(6)   //safe polling enter idle
#define FLGFWSRV_FASTLOAD_NEW  FLGPTN_BIT(7)   //fastload new block has loaded
#define FLGFWSRV_FASTLOAD_DONE FLGPTN_BIT(8)   //fastloadtsk has finished
//Task Finish State
#define FLGFWSRV_OPENED    FLGPTN_BIT(28)  //indicate task can start to recevie command
#define FLGFWSRV_STOPPED   FLGPTN_BIT(29)  //indicate task start to skip all command to wait terminate task
#define FLGFWSRV_SUSPENDED FLGPTN_BIT(30)  //indicate task start to skip all command
#define FLGFWSRV_RESUMED   FLGPTN_BIT(31)  //task start to receive command
//@}

typedef struct _HEADER_BFC{
    UINT32 uiFourCC;    ///< FourCC = BCL1
    UINT16 uiChkSum16;  ///< 16bit check sum value
    UINT16 uiAlgorithm; ///< algorithm always is 9
    UINT32 uiSizeUnComp;///< big endian uncompressed size
    UINT32 uiSizeComp;  ///< big endian compressed size
}HEADER_BFC,*PHEADER_BFC;

/**
*   Task condition
*/
typedef struct _FWSRV_TASK_CONDITION{
    UINT32 uiInitKey;   ///< indicate module is initail
    BOOL   bOpened;     ///< indicate task is open
    BOOL   bSuspended;  ///< indicate task is suppend
    BOOL   bStopped;    ///< indicate task is close
}FWSRV_TASK_CONDITION;

/**
*   Debug information
*/
typedef struct _FWSRV_DBGINFO{
    FWSRV_ER LastEr;               ///< last error code (if any success, this variable will not be clean)
    FWSRV_WR LastWr;               ///< last warning code (if any success, this variable will not be clean)
    FWSRV_CMD_IDX LastCmdIdx;      ///< last command index
    VK_TASK_HANDLE LastCallerTask;  ///< last command caller task id
    UINT32         LastCallerAddr;  ///< last command caller address
}FWSRV_DBGINFO;

/**
    Maxium Data.
*/
typedef union _FWSRV_CMD_MAXDATA{
    union{
        UINT32                  uiReserve;  ///< just reserve. for no any other structure
        FWSRV_PL_LOAD_BURST_IN  PlBurstIn;
		FWSRV_BIN_UPDATE_ALL_IN_ONE UpdateAllInOne;
		FWSRV_FASTLOAD          FastLoad;
    }MAXDATA;
}FWSRV_CMD_MAXDATA;

/**
    Command controller
*/
typedef struct _FWSRV_CMD_CTRL{
    const FWSRV_CMD_DESC*  pFuncTbl;   ///< command function table
    UINT32                 uiNumFunc;  ///< total items in command function table
    FWSRV_CMD              Cmd;        ///< received FwSrv_Cmd command data
    FWSRV_CMD_MAXDATA      MaxInBuf;   ///< duplicated command input data
    FWSRV_ER               erCmd;      ///< command error code after command finish
}FWSRV_CMD_CTRL;

/**
    Update Firmware controller
*/
typedef struct _FWSRV_UPDFW_CTRL{
    const char *pFdtNvtStrgBasePath;
}FWSRV_UPDFW_CTRL;

/**
    Pipe to load and decompress linux-kernel
*/
typedef struct _FWSRV_PIPE_UNCOMP {
	UINT32 uiAddrCompress;
	UINT32 uiSizeCompress;
	UINT32 uiAddrUnCompress;
	UINT32 uiSizeUnCompress;
	UINT32 uiLoadedSize; ///< main task loaded size
} FWSRV_PIPE_UNCOMP, *PFWSRV_PIPE_UNCOMP;

/*
*   FwSrv Manager Control Object
*/
typedef struct _FWSRV_CTRL{
    FWSRV_INIT             Init;       ///< duplicated initial data
    FWSRV_TASK_CONDITION   Cond;       ///< task condition object
    FWSRV_STATE            State;      ///< task state machine
    FWSRV_DBGINFO          DbgInfo;    ///< debug information
    FWSRV_CMD_CTRL         CmdCtrl;    ///< command controllor data
    FWSRV_UPDFW_CTRL       UpdFw;      ///< update firmware
    FWSRV_PIPE_UNCOMP      PipeUnComp; ///< load and decode linux
}FWSRV_CTRL;

FWSRV_CTRL*    xFwSrv_GetCtrl(void);
FWSRV_ER       xFwSrv_Err(FWSRV_ER er);
FWSRV_WR       xFwSrv_Wrn(FWSRV_WR wr);
FWSRV_ER       xFwSrv_Lock(void);
FWSRV_ER       xFwSrv_Unlock(void);
BOOL           xFwSrv_ChkIdle(void);
FWSRV_ER       xFwSrv_WaitIdle(void);
FWSRV_ER       xFwSrv_SetIdle(void);
FWSRV_ER       xFwSrv_SetState(FWSRV_STATE State);
FWSRV_STATE    xFwSrv_GetState(void);

FWSRV_ER       xFwSrv_RunOpen(void);
FWSRV_ER       xFwSrv_RunClose(void);
FWSRV_ER       xFwSrv_RunSuspend(void);
FWSRV_ER       xFwSrv_RunResume(void);
FWSRV_ER       xFwSrv_RunCmd(const FWSRV_CMD* pCmd);

void           xFwSrv_InstallCmd(void);
THREAD_DECLARE(FwSrvTsk, args);

#endif
