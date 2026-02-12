#ifndef _MSDCNVT_NVT_H
#define _MSDCNVT_NVT_H

#include <stdio.h>
#include <string.h>
#include <kwrap/nvt_type.h>
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include <kwrap/task.h>
#include <kwrap/cpu.h>
#include <kwrap/util.h>
#include <kwrap/verinfo.h>
#include <umsd.h>
#include <msdcnvt/MsdcNvtApi.h>
#include "msdcnvt_ipc.h"
#if defined(__FREERTOS)
#include <uart.h>
#include <console.h>
#else
#include <hdal.h>
#include <hd_common.h>
#endif
//Configure
#define CFG_MSDCNVT_INIT_KEY    MAKEFOURCC('M','S','D','C') ///< a key value 'M','S','D','C' for indicating system initial.
#define CFG_DBGSYS_MSG_PAYLOAD_NUM      (1<<7)      ///< Must be 2^n
#if defined(_BSP_NA51000_) || defined(_BSP_NA51055_)
#define CFG_REGISTER_BEGIN_ADDR         0xF0000000  ///< Mem Read/Write over this indicate to Register Access
#define CFG_VIRTUAL_MEM_MAP_MASK        0xF0000000  ///< for MSDCNVT_HANDLER_TYPE_VIRTUAL_MEM_MAP
#define CFG_VIRTUAL_MEM_MAP_ADDR        0xE0000000  ///< for MSDCNVT_HANDLER_TYPE_VIRTUAL_MEM_MAP
#else
#error  "unknown bsp"
#endif

#define _NETWORK_ON_CPU1_

#if defined(__FREERTOS)
#define CFG_DBGSYS ENABLE
#else
#define CFG_DBGSYS DISABLE
// for compiling, because has no console.h
typedef struct _CONSOLE {
	void (*hook)(void); ///< start this object
	void (*unhook)(void); ///< terminal this object
	void (*puts)(const char *s); ///< console input funciton pointer
	int (*gets)(char *s, int len); ///< console output function point
} CONSOLE;
#endif

#if defined(_NETWORK_ON_CPU1_)
#define MSDCNVT_PHY_ADDR(addr)      (addr)
#define MSDCNVT_NONCACHE_ADDR(addr) (addr)
#else
#define MSDCNVT_PHY_ADDR(addr)      NvtIPC_GetPhyAddr(addr)
#define MSDCNVT_NONCACHE_ADDR(addr) NvtIPC_GetNonCacheAddr(addr)
#endif

//Configure Working Memory Pool
#define CFG_NVTMSDC_SIZE_CBW_DATA (sizeof(UINT32)*16) ///< //Msdc System will use front memory to be CBW data
#define CFG_NVTMSDC_SIZE_IPC_DATA (sizeof(MSDCNVT_IPC_CFG))
#define CFG_MIN_HOST_MEM_SIZE           0x10000     ///< Front-End:0x10000
#define CFG_MIN_BACKGROUND_MEM_SIZE     0x10000     ///< Back-End:0x10000
#define CFG_MIN_DBGSYS_MSG_MEM          0x04000     ///< Msg Queue
#define CFG_MIN_WORKING_BUF_SIZE        (CFG_MIN_HOST_MEM_SIZE+CFG_MIN_BACKGROUND_MEM_SIZE+CFG_MIN_DBGSYS_MSG_MEM+CFG_NVTMSDC_SIZE_CBW_DATA+CFG_NVTMSDC_SIZE_IPC_DATA)

#define __MODULE__          MsdcNvt
#include <kwrap/debug.h>

//------------------------------------------------------------------------------
// Novatek Tag
//------------------------------------------------------------------------------
#define CDB_01_NVT_TAG                          0x07 ///< Novatek Tag in CDB_01

//------------------------------------------------------------------------------
// Host SCSI Extern Command
//------------------------------------------------------------------------------
#define SBC_CMD_MSDCNVT_READ                 0xC3 ///< Read Memory / Registers
#define SBC_CMD_MSDCNVT_WRITE                0xC4 ///< Write Memory / Registers
#define SBC_CMD_MSDCNVT_FUNCTION             0xC5 ///< Call Function in Firmware Side

//------------------------------------------------------------------------------
// SBC_CMD_MSDCNVT_FUNCTION Message
//------------------------------------------------------------------------------
#define CDB_10_FUNCTION_UNKNOWN                 0x00 ///< Invalid Type
#define CDB_10_FUNCTION_BI_CALL                 0x01 ///< Bi-Direction Function Type
#define CDB_10_FUNCTION_SI_CALL                 0x02 ///< Single-Direction Function Type
#define CDB_10_FUNCTION_DBGSYS                  0x03 ///< Debug System by Msdc
#define CDB_10_FUNCTION_MISC                    0xFF ///< Misc (Must be Last)

//Bit Mask with T_MSDCNVT_CSW.dCSWTag
#define SBC_CMD_DIR_MASK  0x80
#define SBC_CMD_DIR_IN    0x80
#define SBC_CMD_DIR_OUT   0x00

/**
    Flag Pattern
*/
//@{
//Task Operation
#define FLGDBGSYS_CMD_ARRIVAL   FLGPTN_BIT(0)
#define FLGDBGSYS_CMD_ABORT     FLGPTN_BIT(1)
#define FLGDBGSYS_CMD_FINISH    FLGPTN_BIT(2)
//Task Finish State
#define FLGMSDCNVT_IPC_STOPPED FLGPTN_BIT(22)
#define FLGMSDCNVT_UNKNOWN     FLGPTN_BIT(23)
//@}

typedef struct _MSDCNVT_HOST_INFO {
	//SCSI Command Info
	MSDCNVT_CBW    *pCBW;            ///< SCSI Command Block Wrapper
	MSDCNVT_CSW    *pCSW;            ///< SCSI Command Status Wrapper
	//Calculated Private Data by MSDCVendor_Verify_CB parsed
	UINT32  uiCmd;                      ///< SBC Command ID, SBC_CMD_MSDCNVT_XXXX
	UINT32  uiLB_Address;               ///< Logical Block Address of SCSI Command
	UINT32  uiLB_Length;                ///< Transfer Length of SCSI Command
	//Universal Common Data Swap Pool (Host <-> Device)
	UINT8  *pPoolMem;                   ///< Host<->Device Swap Common Memory Pool (va)
	UINT8  *pPoolMem_pa;                ///< Host<->Device Swap Common Memory Pool (pa)
	UINT32  uiPoolSize;                 ///< Host<->Device Common Memory Pool Size (Unit: Bytes)
} MSDCNVT_HOST_INFO;

typedef struct _MSDCNVT_FUNCTION_CMD_HANDLE {
	UINT32 uiCall;                   ///< Calling Type (CDB_11 Value)
	void (*fpCall)(void *pInfo);     ///< Callback for Handler
} MSDCNVT_FUNCTION_CMD_HANDLE;

//------------------------------------------------------------------------------
// Bi-Direction Call for CDB_10_FUNCTION_BI_CALL
//------------------------------------------------------------------------------
//Bi-Direction Function Control
typedef struct _MSDCNVT_BI_CALL_CTRL {
	void (*fpCall)(void *pData);                        ///< Current Function Call (BiDirection)
	MSDCNVT_BI_CALL_UNIT       *pHead;                  ///< Callbacks Description - Link List Head
	MSDCNVT_BI_CALL_UNIT       *pLast;                  ///< Callbacks Description - Link List Last
} MSDCNVT_BI_CALL_CTRL;

//------------------------------------------------------------------------------
// Single-Direction Call for CDB_10_FUNCTION_SI_CALL
//------------------------------------------------------------------------------
//Single-Direction Function Control
typedef struct _MSDCNVT_SI_CALL_CTRL {
	FP     *fpTblGet;                   ///< PC Get Functions Mapping Talbe(Single Direction)
	UINT8   uiGets;                     ///< Number of PC Get Functions Mapping Talbe(Single Direction)
	FP     *fpTblSet;                   ///< PC Set Functions Mapping Talbe(Single Direction)
	UINT8   uiSets;                     ///< Number of PC Set Functions Mapping Talbe(Single Direction)
	UINT32  uiCall;                     ///< Current Call ID
} MSDCNVT_SI_CALL_CTRL;


//------------------------------------------------------------------------------
// Debug System for CDB_10_FUNCTION_DBGSYS
//------------------------------------------------------------------------------
typedef struct _MSDCNVT_DBGSYS_UNIT {
	UINT32 uiSkip;      ///< Count this payload Msg has been skiped how many times
	UINT32 uiUsedBytes; ///< Used Msg Length
	UINT8 *pMsg;        ///< Msg Buffer
} MSDCNVT_DBGSYS_UNIT;

//Background Ctrl
typedef struct _MSDCNVT_BACKGROUND_CTRL {
	VK_TASK_HANDLE TaskID;              ///< Working Task ID for Command Send
	UINT8  *pPoolMem;                   ///< Device<->Background Common Memory Pool (va)
	UINT8  *pPoolMem_pa;                   ///< Device<->Background Common Memory Pool (pa)
	UINT32  uiPoolSize;                 ///< Device<->Background Common Memory Pool Size (Unit: Bytes)
	UINT32  uiTransmitSize;             ///< Effect Data Size of this Call
	void (*fpCall)(void);               ///< Device<->Background Thread Action Callback Function
	BOOL    bServiceLock;               ///< Indicate Service is Lock / Unlock by Host
	BOOL    bCmdRunning;                ///< Indicate Background Thread is Running
} MSDCNVT_BACKGROUND_CTRL;

//DbgSys Ctrl
typedef struct MSDCNVT_DBGSYS_CTRL {
	BOOL   bInit;                       ///< Indicate DbgSys Init
	ID     SemID;                       ///< Semaphore ID for lock / unlock Msg Receiver
	UINT32 uiMsgIn;                     ///< Msg Index for rtos putting
	UINT32 uiMsgOut;                    ///< Msg Index for pc   getting
	UINT32 uiMsgCntMask;                ///< Mask for Msg Count Add 1
	UINT32 uiPayloadNum;                ///< Total Payload
	UINT32 uiPayloadSize;               ///< Each tMSDCNVT_MSG_UNIT.pMsg Size
	BOOL   bNoOutputUart;               ///< Disable Output to UART
	ER(*fpUartPutString)(CHAR *);       ///< default real uart output (uart_putString or uart2_putString)
	CONSOLE DefaultConsole;             ///< Original Console
	MSDCNVT_DBGSYS_UNIT Queue[CFG_DBGSYS_MSG_PAYLOAD_NUM]; ///< Message Queue
} MSDCNVT_DBGSYS_CTRL;

//IPC Ctrl
typedef struct _MSDCNVT_IPC_CTRL {
	BOOL bNetRunning;
	int ipc_msgid;
	MSDCNVT_IPC_CFG *pCfg;              ///< IPC Buffer
	char CmdLine[40]; //linux shell command such as 'msdcnvt 0x%08X 0x%08X'
} MSDCNVT_IPC_CTRL;

//User Handler Ctrl
typedef struct _MSDCNVT_HANLDER_CTRL {
	MSDCNVT_CB_VIRTUAL_MEM_READ fpVirtualMemMap;
} MSDCNVT_HANLDER_CTRL;

//------------------------------------------------------------------------------
// Global Control Manager
//------------------------------------------------------------------------------
typedef struct _MSDCNVT_CTRL {
	INT32                   uiInitKey;  ///< indicate module is initail
	ID                      FlagID;     ///< FlagID (used by DbgSys currently)
	MSDC_OBJ                *pMsdc;     ///< via Msdc Obj
	MSDCNVT_HOST_INFO       tHostInfo;  ///< Host Information
	MSDCNVT_SI_CALL_CTRL    tSiCall;    ///< Single-Direction Call Ctrl
	MSDCNVT_BI_CALL_CTRL    tBiCall;    ///< BiDirection Call Ctrl
	MSDCNVT_DBGSYS_CTRL     tDbgSys;    ///< Debug System Ctrl
	MSDCNVT_BACKGROUND_CTRL tBkCtrl;    ///< Background Thread Ctrl
	MSDCNVT_CB_EVENT        fpEvent;    ///< Event Report
	MSDCNVT_IPC_CTRL        tIPC;       ///< IPC Ctrl
	MSDCNVT_HANLDER_CTRL    tHandler;   ///< User Handler Ctrl
} MSDCNVT_CTRL;

//------------------------------------------------------------------------------
// Internal APIs
//------------------------------------------------------------------------------
MSDCNVT_CTRL  *xMsdcNvt_GetCtrl(void);                     ///< Get Global Control
MSDCNVT_ICALL_TBL *xMsdcNvt_GetIpcCallTbl(UINT32 *pCnt);   ///< Get IPC Call Table
void           xMsdcNvt_MemPushHostToBk(void);  ///< Copy MSDCNVT_HOST_INFO::pPoolMem -> MSDCNVT_BACKGROUND_CTRL::pPoolMem
void           xMsdcNvt_MemPopBkToHost(void);   ///< Copy MSDCNVT_BACKGROUND_CTRL::pPoolMem -> MSDCNVT_HOST_INFO::pPoolMem
BOOL           xMsdcNvt_Bk_RunCmd(void (*pCall)(void)); ///< Run Callback in Background Thread
BOOL           xMsdcNvt_Bk_IsFinish(void);              ///< Query Background Thread is Finish
BOOL           xMsdcNvt_Bk_HostLock(void);              ///< Lock Bakcgorund Service by Host
BOOL           xMsdcNvt_Bk_HostUnLock(void);            ///< UnLock Bakcgorund Service by Host
BOOL           xMsdcNvt_Bk_HostIsLock(void);            ///< Query Bakcgorund Service is Lock

//Vendor Command (CDB[10]) Handler
void xMsdcNvt_Function_BiCall(void);       ///< Bi Direction Call
void xMsdcNvt_Function_SiCall(void);       ///< Single Direction Call
void xMsdcNvt_Function_DbgSys(void);       ///< Debug System Vendor Command
void xMsdcNvt_Function_Misc(void);         ///< Debug System Vendor Command

//IPC
void xMsdcNvt_ICmd(MSDCNVT_ICMD *pCmd); ///< IPC Command
void xMsdcNvtTsk_Ipc(void);

/**
     Install and UnInstall task,flag and semaphore id
*/
extern void MsdcNvt_InstallID(void);
extern void MsdcNvt_UnInstallID(void);

#endif
