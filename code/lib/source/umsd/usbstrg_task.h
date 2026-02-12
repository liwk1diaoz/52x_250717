/*
    USB MSDC Task Header File

    This is the MSDC Task internal header file

    @file       UsbStrgTsk.h
    @ingroup    mILibUsbMSDC
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _USBSTRGTSK_H
#define _USBSTRGTSK_H

#include <string.h>
#include <stdio.h>

#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/cpu.h"

#include "umsd.h"
#include "usb2dev.h"
#ifndef __KERNEL__
//#include "dma_protected.h"
#endif
#include "strg_def.h"

#define __MODULE__    rtos_devusb_msdc
#include <kwrap/debug.h>

extern unsigned int rtos_devusb_msdc_debug_level;

#ifdef _NVT_EMULATION_
#define _EMULATION_				ENABLE
#else
#define _EMULATION_				DISABLE
#endif

#if 1//def __KERNEL__
#define dma_getCacheAddr(adr) (adr)
#define dma_flushWriteCache(addr, size)
#define dma_flushReadCache(addr, size)

#endif



#define MSDC_DEBUG              DISABLE
#if MSDC_DEBUG
#define msdc_debug(msg)         DBG_DUMP msg
#else
#define msdc_debug(msg)
#endif

#define MSDC_CACHEWRITE_HANDLE      DISABLE  // Write Cache Enable/Disable
#define MSDC_CACHEREAD_HANDLE       DISABLE  // Read  Cache Enable/Disable
#define MSDC_CACHEREAD_L2_HANDLE    DISABLE  // Layer-2 Read  Cache Enable/Disable

#define SS_PACKET_SIZE          1024
#define HS_PACKET_SIZE          512
#define FS_PACKET_SIZE          64

#define PRI_USBSTRG                 8
#define PRI_USBSTRG_CACHE           10
#define STKSIZE_USBSTRGVND          2048
#define STKSIZE_USBSTRGCACHE        1024

// MSDC Flag define
#define FLGMSDC_BULKIN          0x00000001
#define FLGMSDC_BULKOUT         0x00000002
#define FLGMSDC_CLRFEATURE      0x00000004
#define FLGMSDC_MSDCRESET       0x00000008
#define FLGMSDC_USBRESET        0x00000010
#define FLGMSDC_IDLE            0x00000100
#define FLGMSDC_CACHE_IDLE      0x00010000
#define FLGMSDC_CACHE_TRIGGER   0x00020000
#define FLGMSDC_CACHE_READDONE  0x00040000
#define FLGMSDC_CACHE_NOPENDING 0x00080000
#define FLGMSDC_BULKOUT_STOP    0x00100000
#define FLGMSDC_CACHE_STOP      0x00200000


#define MSDC_CBW_BUFSIZE        64
#define MSDC_MINBUFSIZE_MASK    (0x1FFFF)


/*
    Cache R/W Direction ID
*/
typedef enum {
	MSDC_CACHE_READ,            // Cache Read
	MSDC_CACHE_WRITE,           // Cache Write

	MSDC_CACHE_READ_HIT,        // Cache HIT. Use new buffer address

	ENUM_DUMMY4WORD(MSDC_CACHE_DIR)
} MSDC_CACHE_DIR;

/*
    MSDC Cache Command Configurations
*/
typedef struct {
	MSDC_CACHE_DIR  Dir;        // Cache Direction

	UINT32          uiLUN;      // Logical Unit Number
	UINT32          uiAddr;     // Buffer Address
	UINT32          uiLBA;      // Logical Block Address
	UINT32          uiSecNum;   // RW Sector Number

} MSDC_CACHE_INFO, *PMSDC_CACHE_INFO;



extern UINT32                   U2EP2_PACKET_SIZE;
extern UINT8                    g_uiTotalLUNs,g_uiU2TotalLUNs;
extern UINT32                   guiU2InquiryDataAddr,guiU2InquiryDataAddr_pa,guiInquiryDataAddr, guiMsdcBufAddr[2],guiU2MsdcBufAddr[2],guiU2MsdcBufAddr_pa[2], guiMsdcVendorInBufAddr,guiU2MsdcVendorInBufAddr, guiMsdcBufIdx,guiU2MsdcBufIdx;
extern MSDC_Vendor_CB           guiMsdcVendor_cb,guiU2MsdcVendor_cb;
extern MSDC_Verify_CB           guiMsdcCheck_cb,guiU2MsdcCheck_cb;
extern UINT32                   g_uiUMSDCState,g_uiU2MSDCState;
extern UINT32                   *BulkCBWData,*gpuiU2BulkCBWData,*gpuiU2BulkCBWData_pa;
extern UINT32					*BulkCSWData,*BulkCSWData_pa;
extern UINT32					*BulkINData,*BulkINData_pa;
extern BOOL                     gbReadCapNoStall,gbU2ReadCapNoStall;
extern USB_EP                   gMsdcEpIN,gU2MsdcEpIN, gMsdcEpOUT,gU2MsdcEpOUT;
extern UINT32					guiMsdcDataBufSize,guiU2MsdcDataBufSize;
extern UINT32					guiMsdcReadSlice,guiU2MsdcReadSlice;
extern UINT32					guiMsdcWCBufAddr[2],guiMsdcWCSize,guiMsdcWCCurSz,guiMsdcWCBufIdx,guiMsdcWCNextLBA,guiMsdcWCSlice;
extern UINT32					guiU2MsdcWCBufAddr[2],guiU2MsdcWCSize,guiU2MsdcWCCurSz,guiU2MsdcWCBufIdx,guiU2MsdcWCNextLBA,guiU2MsdcWCSlice;


extern ER                       Msdc_WaitFlag(PFLGPTN puiFlag, FLGPTN WaitFlags);
extern ER                       U2Msdc_WaitFlag(PFLGPTN puiFlag, FLGPTN WaitFlags);
extern ER                       Msdc_SetFlag(FLGPTN flagsID);
extern ER                       U2Msdc_SetFlag(FLGPTN flagsID);
extern ER                       Msdc_ClearFlag(FLGPTN flagsID);
extern ER                       U2Msdc_ClearFlag(FLGPTN flagsID);
extern ER                       Msdc_TriggerCache(PMSDC_CACHE_INFO pCacheInfo);
extern ER                       U2Msdc_TriggerCache(PMSDC_CACHE_INFO pCacheInfo);
extern ER                       Msdc_GetCacheReadData(PMSDC_CACHE_INFO pCacheInfo);
extern ER                       U2Msdc_GetCacheReadData(PMSDC_CACHE_INFO pCacheInfo);
extern ER                       Msdc_FlushCache(void);
extern ER 						Msdc_FlushReadCache(void);
extern ER                       U2Msdc_FlushCache(void);
extern ER 						U2Msdc_FlushReadCache(void);


extern ER               		U2Msdc_Open(PUSB_MSDC_INFO pClassInfo);
extern void             		U2Msdc_Close(void);
extern ER               		U2Msdc_SetConfig(USBMSDC_CONFIG_ID ConfigID, UINT32 uiCfgValue);
extern UINT32           		U2Msdc_GetConfig(USBMSDC_CONFIG_ID ConfigID);
extern ER               		U2Msdc_SetLunConfig(UINT32 uiLun, USBMSDC_LUN_CONFIG_ID ConfigID, UINT32 uiCfgValue);
extern UINT32           		U2Msdc_GetLunConfig(UINT32 uiLun, USBMSDC_LUN_CONFIG_ID ConfigID);
extern void             		U2Msdc_ChgStrgObject(UINT32 uiLun, PUSB_MSDC_STRG_INFO pStrgInfo);
extern MSDC_TASK_STS    		U2Msdc_GetStatus(void);
extern BOOL             		U2Msdc_CheckIdle(void);



extern _ALIGNED(64) UINT8  		puiCbwBuf[64];

#define U2Msdc_SetIdle()        U2Msdc_SetFlag(FLGMSDC_IDLE)
#define U2Msdc_ClearIdle()      U2Msdc_ClearFlag(FLGMSDC_IDLE)

#define Msdc_SetIdle()          Msdc_SetFlag(FLGMSDC_IDLE)
#define Msdc_ClearIdle()        Msdc_ClearFlag(FLGMSDC_IDLE)

#endif
