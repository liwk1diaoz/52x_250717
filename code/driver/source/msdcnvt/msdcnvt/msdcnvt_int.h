#ifndef _MSDCNVT_NVT_H
#define _MSDCNVT_NVT_H

#if defined (__UITRON)
#include "SysKer.h"
#include "uart.h"
#include "DxStorage.h"
#include "DxCommon.h"
#include "StrgDef.h"
#include <string.h>
#include "dma_protected.h"
#include "NvtIpcAPI.h"
#include "Utility.h"
#include "SysKer.h"
#include <string.h>
#include "NvtVerInfo.h"
#include "msdcnvt_api.h"
#include "msdcnvt_callback.h"
#if defined(_NETWORK_ON_CPU1_)
#include <cyg/msdcnvt/msdcnvt.h>
#endif
#else
#include <kwrap/semaphore.h>
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/dma-direction.h>
#include <asm/cacheflush.h>
#include <asm/io.h>  /* for ioremap and iounmap */
#include <msdcnvt/msdcnvt_api.h>
#include <msdcnvt/msdcnvt_callback.h>
#endif
#include "msdcnvt_ipc.h"

//Configure
#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define CFG_MSDCNVT_INIT_KEY    MAKEFOURCC('M', 'S', 'D', 'C') ///< a key value 'M','S','D','C' for indicating system initial.
#define CFG_DBGSYS_MSG_PAYLOAD_NUM      (1<<7)      ///< Must be 2^n
#if 1 //defined(_BSP_NA51000_) || defined(_BSP_NA51055_)
#define CFG_REGISTER_BEGIN_ADDR         0xF0000000  ///< Mem Read/Write over this indicate to Register Access
#define CFG_VIRTUAL_MEM_MAP_MASK        0xF0000000  ///< for MSDCNVT_HANDLER_TYPE_VIRTUAL_MEM_MAP
#define CFG_VIRTUAL_MEM_MAP_ADDR        0xE0000000  ///< for MSDCNVT_HANDLER_TYPE_VIRTUAL_MEM_MAP
#elif defined(_BSP_NA51023_)
#define CFG_REGISTER_BEGIN_ADDR         0xC0000000  ///< Mem Read/Write over this indicate to Register Access
#define CFG_VIRTUAL_MEM_MAP_MASK        0xF0000000  ///< for MSDCNVT_HANDLER_TYPE_VIRTUAL_MEM_MAP
#define CFG_VIRTUAL_MEM_MAP_ADDR        0xF0000000  ///< for MSDCNVT_HANDLER_TYPE_VIRTUAL_MEM_MAP
#else
#error  "unknown bsp"
#endif

#if !defined (__UITRON)
#define dma_flushreadcache(virt_addr, size)
#define dma_flushwritecache(virt_addr, size)
#endif

#if defined(__UITRON)
#if defined(_NETWORK_ON_CPU1_)
#define MSDCNVT_PHY_ADDR(addr)      (addr)
#define MSDCNVT_NONCACHE_ADDR(addr) (addr)
#define MSDCNVT_CACHE_ADDR(addr)    (addr)
#else
/* #define MSDCNVT_PHY_ADDR(addr)      Nvipc_GetPhyAddr(addr) */
/* #define MSDCNVT_NONCACHE_ADDR(addr) Nvipc_GetNonCacheAddr(addr) */
/* #define MSDCNVT_CACHE_ADDR(addr)    Nvipc_GetCacheAddr(addr) */
#endif
#else // pure linux
#define MSDCNVT_PHY_ADDR(addr)      msdcnvt_get_phy(addr)
#define MSDCNVT_NONCACHE_ADDR(addr) msdcnvt_get_noncache(addr)
#define MSDCNVT_CACHE_ADDR(addr)    msdcnvt_get_cache(addr)
#endif

//Configure Working Memory Pool
#define CFG_NVTMSDC_SIZE_CBW_DATA (sizeof(UINT32)*16) ///< //Msdc System will use front memory to be CBW data
#define CFG_NVTMSDC_SIZE_IPC_DATA (sizeof(MSDCNVT_IPC_CFG))
#define CFG_MIN_HOST_MEM_SIZE           0x10000     ///< Front-End:0x10000
#define CFG_MIN_BACKGROUND_MEM_SIZE     0x10000     ///< Back-End:0x10000
#define CFG_MIN_DBGSYS_MSG_MEM          0x04000     ///< Msg queue
#define CFG_MIN_WORKING_BUF_SIZE        (CFG_MIN_HOST_MEM_SIZE+CFG_MIN_BACKGROUND_MEM_SIZE+CFG_MIN_DBGSYS_MSG_MEM+CFG_NVTMSDC_SIZE_CBW_DATA+CFG_NVTMSDC_SIZE_IPC_DATA)

#define __MODULE__          msdcnvt
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass

#if defined (__UITRON)
#include "DebugModule.h"
#else
#include "kwrap/debug.h"
#endif
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

//Bit Mask with T_MSDCNVT_CSW.tag
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
#define FLGMSDCNVT_IPC_STOPPED FLGPTN_BIT(29)
#define FLGMSDCNVT_UNKNOWN     FLGPTN_BIT(30)
//@}

typedef struct _MSDCNVT_HOST_INFO {
	//SCSI Command Info
	MSDCNVT_CBW    *p_cbw;            ///< SCSI Command Block Wrapper
	MSDCNVT_CSW    *p_csw;            ///< SCSI Command Status Wrapper
	//Calculated Private Data by MSDCVendor_Verify_CB parsed
	UINT32  cmd_id;                   ///< SBC Command ID, SBC_CMD_MSDCNVT_XXXX
	UINT32  lb_address;               ///< Logical Block Address of SCSI Command
	UINT32  lb_length;                ///< Transfer Length of SCSI Command
	//Universal Common Data Swap Pool (Host <-> Device)
	UINT8  *p_pool;                   ///< Host<->Device Swap Common Memory Pool
	UINT32  pool_size;                ///< Host<->Device Common Memory Pool Size (Unit: Bytes)
} MSDCNVT_HOST_INFO;

typedef struct _MSDCNVT_FUNCTION_CMD_HANDLE {
	UINT32 call_id;                   ///< Calling Type (CDB_11 value)
	void (*fp_call)(void *p_info);    ///< Callback for Handler
} MSDCNVT_FUNCTION_CMD_HANDLE;

//------------------------------------------------------------------------------
// Bi-Direction Call for CDB_10_FUNCTION_BI_CALL
//------------------------------------------------------------------------------
//Bi-Direction Function Control
typedef struct _MSDCNVT_BI_CALL_CTRL {
	void (*fp_call)(void *p_data);        ///< Current Function Call (BiDirection)
	MSDCNVT_BI_CALL_UNIT       *p_head;  ///< Callbacks Description - Link List Head
	MSDCNVT_BI_CALL_UNIT       *p_last;  ///< Callbacks Description - Link List Last
} MSDCNVT_BI_CALL_CTRL;

//------------------------------------------------------------------------------
// Single-Direction Call for CDB_10_FUNCTION_SI_CALL
//------------------------------------------------------------------------------
//Single-Direction Function Control
typedef struct _MSDCNVT_SI_CALL_CTRL {
	FP     *fp_tbl_get; ///< PC Get Functions Mapping Talbe(Single Direction)
	UINT8   num_gets;   ///< Number of PC Get Functions Mapping Talbe(Single Direction)
	FP     *fp_tbl_set; ///< PC Set Functions Mapping Talbe(Single Direction)
	UINT8   num_sets;   ///< Number of PC Set Functions Mapping Talbe(Single Direction)
	UINT32  call_id;    ///< Current Call ID
} MSDCNVT_SI_CALL_CTRL;


//------------------------------------------------------------------------------
// Debug System for CDB_10_FUNCTION_DBGSYS
//------------------------------------------------------------------------------
typedef struct _MSDCNVT_DBGSYS_UNIT {
	UINT32 skip_cnt; ///< Count this payload Msg has been skiped how many times
	UINT32 byte_cnt; ///< Used Msg Length
	UINT8 *p_msg;    ///< Msg Buffer
} MSDCNVT_DBGSYS_UNIT;

//Background Ctrl
typedef struct _MSDCNVT_BACKGROUND_CTRL {
	THREAD_HANDLE task_id;            ///< Working Task ID for Command Send
	UINT8  *p_pool;                   ///< Device<->Background Common Memory Pool
	UINT32  pool_size;                ///< Device<->Background Common Memory Pool Size (Unit: Bytes)
	UINT32  trans_size;               ///< Effect Data Size of this Call
	void (*fp_call)(void);            ///< Device<->Background Thread Action Callback Function
	BOOL    b_service_lock;           ///< Indicate Service is Lock / Unlock by Host
	BOOL    b_cmd_running;            ///< Indicate Background Thread is Running
} MSDCNVT_BACKGROUND_CTRL;

//DbgSys Ctrl
typedef struct MSDCNVT_DBGSYS_CTRL {
	BOOL   b_init;                   ///< Indicate DbgSys Init
	SEM_HANDLE sem_id;               ///< Semaphore ID for lock / unlock Msg Receiver
	UINT32 msg_in_idx;               ///< Msg Index for rtos putting
	UINT32 msg_out_idx;              ///< Msg Index for pc   getting
	UINT32 msg_cnt_mask;             ///< Mask for Msg Count Add 1
	UINT32 payload_num;              ///< Total Payload
	UINT32 payload_size;             ///< Each tMSDCNVT_MSG_UNIT.p_msg Size
	BOOL   b_no_uart_output;         ///< Disable Output to UART
	unsigned int (*fp_uart_put_string)(CHAR *); ///< default real uart output (uart_putString or uart2_putString)
	MSDCNVT_DBGSYS_UNIT queue[CFG_DBGSYS_MSG_PAYLOAD_NUM]; ///< Message queue
} MSDCNVT_DBGSYS_CTRL;

//IPC Ctrl
typedef struct _MSDCNVT_IPC_CTRL {
	BOOL b_net_running;
	int ipc_msgid;
	MSDCNVT_IPC_CFG *p_cfg; ///< IPC Buffer
	char cmd_line[40]; ///< linux shell command such as 'msdcnvt 0x%08X 0x%08X'
} MSDCNVT_IPC_CTRL;

//User Handler Ctrl
typedef struct _MSDCNVT_HANLDER_CTRL {
	MSDCNVT_CB_VIRTUAL_MEM_READ fp_virt_mem_map;
} MSDCNVT_HANLDER_CTRL;

//------------------------------------------------------------------------------
// Global Control Manager
//------------------------------------------------------------------------------
typedef struct _MSDCNVT_CTRL {
	INT32                   init_key;  ///< indicate module is initail
	ID                      flag_id;   ///< flag_id (used by DbgSys currently)
	MSDC_OBJ                *p_msdc;   ///< via Msdc Obj
	MSDCNVT_HOST_INFO       host_info; ///< Host Information
	MSDCNVT_SI_CALL_CTRL    sicall;    ///< Single-Direction Call Ctrl
	MSDCNVT_BI_CALL_CTRL    bicall;    ///< BiDirection Call Ctrl
	MSDCNVT_DBGSYS_CTRL     dbgsys;    ///< Debug System Ctrl
	MSDCNVT_BACKGROUND_CTRL bk;        ///< Background Thread Ctrl
	MSDCNVT_CB_EVENT        fp_event;  ///< Event Report
	MSDCNVT_IPC_CTRL        ipc;       ///< IPC Ctrl
	MSDCNVT_HANLDER_CTRL    handler;   ///< User Handler Ctrl
} MSDCNVT_CTRL;

//------------------------------------------------------------------------------
// Internal APIs
//------------------------------------------------------------------------------
MSDCNVT_CTRL  *msdcnvt_get_ctrl(void);                     ///< Get Global Control
MSDCNVT_ICALL_TBL *msdcnvt_get_ipc_call_tbl(UINT32 *p_cnt); ///< Get IPC Call Table
void           msdcnvt_mem_push_host_to_bk(void);  ///< Copy MSDCNVT_HOST_INFO::p_pool -> MSDCNVT_BACKGROUND_CTRL::p_pool
void           msdcnvt_mem_pop_bk_to_host(void);   ///< Copy MSDCNVT_BACKGROUND_CTRL::p_pool -> MSDCNVT_HOST_INFO::p_pool
BOOL           msdcnvt_bk_run_cmd(void (*p_call)(void)); ///< Run Callback in Background Thread
BOOL           msdcnvt_bk_is_finish(void);              ///< Query Background Thread is Finish
BOOL           msdcnvt_bk_host_lock(void);              ///< Lock Bakcgorund Service by Host
BOOL           msdcnvt_bk_host_unlock(void);            ///< UnLock Bakcgorund Service by Host
BOOL           msdcnvt_bk_host_is_lock(void);           ///< Query Bakcgorund Service is Lock

//------------------------------------------------------------------------------
// This task is used for do background command, currently.
//------------------------------------------------------------------------------
extern THREAD_DECLARE(msdcnvt_tsk, arglist);

//Vendor Command (CDB[10]) Handler
void msdcnvt_function_bicall(void);       ///< Bi Direction Call
void msdcnvt_function_sicall(void);       ///< Single Direction Call
void msdcnvt_function_dbgsys(void);       ///< Debug System Vendor Command
void msdcnvt_function_misc(void);         ///< Debug System Vendor Command

//IPC
void msdcnvt_icmd(MSDCNVT_ICMD *p_cmd); ///< IPC Command
//void xmsdcnvt_tsk_ipc(void);
THREAD_DECLARE(xmsdcnvt_tsk_ipc, arglist);

//Pure Linux
extern UINT32 msdcnvt_get_phy(UINT32 addr);
extern UINT32 msdcnvt_get_noncache(UINT32 addr);
extern UINT32 msdcnvt_get_cache(UINT32 addr);

#endif
