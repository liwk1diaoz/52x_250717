/*
    USB MSDC Task Main Control File

    This is the USB Mass Storage main task and control APIs.

    @file       UsbStrgTsk.c
    @ingroup    mILibUsbMSDC
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include    "usb2dev.h"
#include    "umsd.h"

#include    <string.h>
#include	<kwrap/task.h>
#include	<kwrap/util.h>

#include    "usbstrg_task.h"
#include    "usbstrg_tpbulk.h"
#include    "usbstrg_desc.h"
#include    "usbstrg_reqclass.h"
//#include    "DxStorage.h"
//#include    "DxApi.h"
//#include    "NvtVerInfo.h"

unsigned int rtos_devusb_msdc_debug_level = NVT_DBG_WRN;

/**
    @addtogroup mILibUsbMSDC
*/
//@{

//NVTVER_VERSION_ENTRY(Msdc, 1, 00, 000, /* branch version */ 00)

static UINT32       guiOnMsdMode;

UINT32              guiU2InquiryDataAddr,guiU2InquiryDataAddr_pa;
UINT32              guiU2MsdcBufIdx = 0;
UINT32              guiU2MsdcBufAddr[2];
UINT32              guiU2MsdcBufAddr_pa[2];
UINT32              guiU2MsdcVendorInBufAddr;
MSDC_Vendor_CB      guiU2MsdcVendor_cb;
MSDC_Verify_CB      guiU2MsdcCheck_cb;
MSDC_Led_CB         guiU2MsdcRWLed_cb;
MSDC_Led_CB         guiU2MsdcSuspendLed_cb;
MSDC_Led_CB         guiU2MsdcStopUnitLed_cb;
UINT32              U2EP2_PACKET_SIZE;
UINT8               g_uiU2TotalLUNs;
UINT32              g_uiU2MSDCState = MSDC_CLOSED_STATE;
BOOL                g_bU2MsdcCompositeDevice = FALSE;
BOOL                gbU2ReadCapNoStall = FALSE;
UINT32				guiU2MsdcDataBufSize;
UINT32				guiU2MsdcReadSlice;
UINT32				guiU2MsdcWCBufAddr[2],guiU2MsdcWCSize,guiU2MsdcWCCurSz,guiU2MsdcWCBufIdx,guiU2MsdcWCNextLBA,guiU2MsdcWCSlice=128;

USB_EP              gU2MsdcEpIN = USB_EP1, gU2MsdcEpOUT = USB_EP2;
UINT32				guiU2MsdcInterfaceIdx = 0;

static volatile     MSDC_CACHE_INFO  MsdcCacheInfo, MsdcCachedData;


ID FLG_ID_USBCLASS;
THREAD_HANDLE USBSTRGTSK_ID;
THREAD_HANDLE USBSTRGCACHETSK_ID;


THREAD_RETTYPE U2Msdc_StrgTsk(void *arglist);
THREAD_RETTYPE U2Msdc_CacheTsk(void *arglist);

/*
    USB Event Callback Handler for MSDC Task
*/
static void U2Msdc_EventCallback(UINT32 uiEvent)
{
	//msdc_debug(("Into U2Msdc_EventCallback(), uiEvent =(0x%X) \r\n",uiEvent));

	switch (uiEvent) {
	case USB_EVENT_EP1_RX:
	case USB_EVENT_EP2_RX:
	case USB_EVENT_EP3_RX:
	case USB_EVENT_EP4_RX:
	case USB_EVENT_EP5_RX:
	case USB_EVENT_EP6_RX:
	case USB_EVENT_EP7_RX:
	case USB_EVENT_EP8_RX:
	case USB_EVENT_EP9_RX:
	case USB_EVENT_EP10_RX:
	case USB_EVENT_EP11_RX:
	case USB_EVENT_EP12_RX:
	case USB_EVENT_EP13_RX:
	case USB_EVENT_EP14_RX:
	case USB_EVENT_EP15_RX:

		if ((uiEvent - USB_EVENT_EP0_RX) == gU2MsdcEpOUT) {
			usb2dev_mask_ep_interrupt(gU2MsdcEpOUT);
			U2Msdc_SetFlag(FLGMSDC_BULKOUT);
		}
		break;


	case USB_EVENT_CLRFEATURE:
		U2Msdc_SetFlag(FLGMSDC_CLRFEATURE);
		break;
	case USB_EVENT_RESET:
		U2Msdc_SetFlag(FLGMSDC_USBRESET);
		break;
	case USB_EVENT_SUSPEND:
		if (guiU2MsdcSuspendLed_cb != NULL) {
			guiU2MsdcSuspendLed_cb();
		}
		break;

	default:
		msdc_debug(("U2Msdc_EventCallback: Not the MSDC's event!   uiEvent =(0x%X) \r\n", (int)uiEvent));
		break;
	}

}

/*
    Enable/Disable FIFO interrupts of each USB endpoints.
*/
static void U2Msdc_OpenNeededFIFO(void)
{
	usb2dev_mask_ep_interrupt(gU2MsdcEpIN);
	usb2dev_unmask_ep_interrupt(gU2MsdcEpOUT);
}

/*
    Initialize each LUN.

    @param pClassInfo-Information about each LUN.
*/
static void U2Msdc_InitLUNs(USB_MSDC_INFO *pClassInfo)
{
	UINT8 LUNs;

	if (pClassInfo->LUNs > MAX_LUN) {
		pClassInfo->LUNs                        = MAX_LUN;
	}
	if (pClassInfo->LUNs < 1) {
		pClassInfo->LUNs                        = 1;
	}

	for (LUNs = 0; LUNs < pClassInfo->LUNs; LUNs++) {
		gU2MsdInfo[LUNs].MsdcType                 = pClassInfo->msdc_type[LUNs];

		gU2MsdInfo[LUNs].RequestSense.bValid      = 0x00;
		gU2MsdInfo[LUNs].RequestSense.bSenseKey   = 0x00;
		gU2MsdInfo[LUNs].RequestSense.bASC        = 0x00;
		gU2MsdInfo[LUNs].RequestSense.bASCQ       = 0x00;

		gU2MsdInfo[LUNs].MEMBusClosed             = FALSE;
		gU2MsdInfo[LUNs].StorageEject             = TRUE;
		gU2MsdInfo[LUNs].StorageChanged           = FALSE;
		gU2MsdInfo[LUNs].WriteProtect             = FALSE;
		gU2MsdInfo[LUNs].LastDetStrg              = FALSE;

		gU2MsdInfo[LUNs].pStrgDevObj = pClassInfo->pStrgHandle[LUNs];
		if (gU2MsdInfo[LUNs].pStrgDevObj == 0) {
			DBG_WRN("Get Lun Strg Obj Fail!(%d)\r\n", LUNs);
		}

		gU2MsdInfo[LUNs].DetStrg_cb               = pClassInfo->msdc_storage_detCB[LUNs];
		gU2MsdInfo[LUNs].DetStrgLock_cb           = pClassInfo->msdc_strgLock_detCB[LUNs];
	}

	gU2MsdRwInfoBlock.bNVT_LastOpenedLUN          = NO_OPENED_LUN;

	return;
}

/*
    Set USB MSDC Task's Event Flag.
*/
ER U2Msdc_SetFlag(FLGPTN flagsID)
{
	return set_flg(FLG_ID_USBCLASS, flagsID);
}

/*
    Clear USB MSDC Task's Event Flag.
*/
ER U2Msdc_ClearFlag(FLGPTN flagsID)
{
	return clr_flg(FLG_ID_USBCLASS, flagsID);
}

/*
    Wait mode idle flag.
*/
//static ER U2Msdc_WaitIdle(void)
//{
//	FLGPTN  uiFlag;
//	return vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBCLASS, FLGMSDC_IDLE, TWF_ORW);
//}

/*
    Flag waiting function with the mode idle flag checking mechanism.
*/
ER U2Msdc_WaitFlag(PFLGPTN puiFlag, FLGPTN WaitFlags)
{
	U2Msdc_SetIdle();
	vos_flag_wait_interruptible(puiFlag, FLG_ID_USBCLASS, WaitFlags, TWF_ORW | TWF_CLR);
	U2Msdc_ClearIdle();
	return E_OK;
}

/*
    Wait the Cache Task Idle and then Clear the Cache Contents.
*/
ER U2Msdc_FlushCache(void)
{
	FLGPTN      uiFlag;

	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBCLASS, FLGMSDC_CACHE_IDLE, TWF_ORW | TWF_CLR);

	// Clear Cache Content
	MsdcCachedData.uiSecNum = 0;

	return U2Msdc_SetFlag(FLGMSDC_CACHE_IDLE);
}

/*
    Wait the Cache Task Idle and then Clear the Cache Contents.
*/
ER U2Msdc_FlushReadCache(void)
{
	FLGPTN      uiFlag;

	if (MsdcCacheInfo.Dir != MSDC_CACHE_WRITE) {
		vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBCLASS, FLGMSDC_CACHE_IDLE, TWF_ORW | TWF_CLR);

		// Clear Cache Content
		MsdcCachedData.uiSecNum = 0;

		return U2Msdc_SetFlag(FLGMSDC_CACHE_IDLE);
	} else {
		return E_OK;
	}
}

/*
    Trigger MSDC Cache Task Read or Write Operation
*/
ER U2Msdc_TriggerCache(PMSDC_CACHE_INFO pCacheInfo)
{
	FLGPTN      uiFlag;

	U2Msdc_WaitFlag(&uiFlag, FLGMSDC_CACHE_IDLE);

	if (pCacheInfo->Dir == MSDC_CACHE_READ) {
		U2Msdc_ClearFlag(FLGMSDC_CACHE_READDONE);

		if ((pCacheInfo->uiLUN == MsdcCachedData.uiLUN) && (pCacheInfo->uiLBA == MsdcCachedData.uiLBA) && (pCacheInfo->uiSecNum <= MsdcCachedData.uiSecNum)) {
			// Cache Hit
			MsdcCachedData.Dir = MSDC_CACHE_READ_HIT;

#if !MSDC_CACHEREAD_L2_HANDLE
			U2Msdc_SetFlag(FLGMSDC_CACHE_IDLE);
			return E_OK;
#endif
		} else {
			// Cache Miss
			MsdcCachedData.Dir      = MSDC_CACHE_READ;

			// Clear Cache Content immediately if the next access is not cache hit.
			// If not, we may read error because the host read randomly.
			MsdcCachedData.uiSecNum = 0;
		}
	} else {
		// Clear Cache Content immediately if the next access is not cache hit.
		// If not, we may read error because the host read randomly.
		MsdcCachedData.uiSecNum = 0;
	}

	memcpy((void *)&MsdcCacheInfo, (void *)pCacheInfo, sizeof(MSDC_CACHE_INFO));

	if (MsdcCachedData.Dir == MSDC_CACHE_READ_HIT) {
		MsdcCacheInfo.Dir = MSDC_CACHE_READ_HIT;
	}

	return  U2Msdc_SetFlag(FLGMSDC_CACHE_TRIGGER);
}

/*
    Check if the Cache Read is done and if the READ Cache is Hit or Miss
*/
ER U2Msdc_GetCacheReadData(PMSDC_CACHE_INFO pCacheInfo)
{
	FLGPTN uiFlag;

	// Check cache content
	if (MsdcCachedData.Dir == MSDC_CACHE_READ_HIT) {
		// Cache Hit
		memcpy((void *)pCacheInfo, (void *)&MsdcCachedData, sizeof(MSDC_CACHE_INFO));

		MsdcCachedData.Dir = MSDC_CACHE_READ;// Cache Cleared
		return E_OK;
	} else {
		// Cache Miss
		pCacheInfo->Dir     = MSDC_CACHE_READ;

		return U2Msdc_WaitFlag(&uiFlag, FLGMSDC_CACHE_READDONE);
	}

}

/*
    USB MSDC Main Task

    Handles the USB MSDC Bulk-Only Transport
*/
//void U2Msdc_StrgTsk(void)
THREAD_DECLARE(U2Msdc_StrgTsk, arglist)
{
	FLGPTN      uiFlag;
	static BOOL detectFlag = FALSE;

	//kent_tsk();

	msdc_debug(("U2Msdc_StrgTsk: MSDC Task Start!!\r\n"));

	//coverity[no_escape]
	while (1) {
		U2Msdc_SetIdle();
		PROFILE_TASK_IDLE();
		U2Msdc_WaitFlag(&uiFlag, FLGMSDC_BULKOUT|FLGMSDC_BULKOUT_STOP);
		PROFILE_TASK_BUSY();
		usb2dev_mask_ep_interrupt(gU2MsdcEpOUT);
		U2Msdc_ClearIdle();

		if(uiFlag & FLGMSDC_BULKOUT_STOP) {
			THREAD_RETURN(0);
		}

		// detect USB connection is HS or FS
		if (detectFlag == FALSE) {
			detectFlag = TRUE;

			if (usb2dev_is_highspeed_enabled()) {
				// HS case
				U2EP2_PACKET_SIZE = HS_PACKET_SIZE;
			} else {
				// FS case
				U2EP2_PACKET_SIZE = FS_PACKET_SIZE;
			}
		}

		U2Msdc_CBWHandler();
		U2Msdc_ClearFlag(FLGMSDC_BULKOUT);
		usb2dev_unmask_ep_interrupt(gU2MsdcEpOUT);
	}
}

/*
    MSDC Read/Write Cache Task
*/
//void U2Msdc_CacheTsk(void)
THREAD_DECLARE(U2Msdc_CacheTsk, arglist)
{
	FLGPTN      uiFlag;
	ER          Ret;
	UINT32      uiCacheAddr, uiCacheLBA, uiCacheSecNum;
	UINT32      uiPreCond;

	//kent_tsk();

	/*
	    In current Buffer Allocation, Each ping-pong buffer is 128KB.(256 Sectors)
	    In Windosw system, the OS always issue no larger than 64KB in READ Command.(128 Sectors)
	    So, we use the lower 64KB as the Cache Buffer location for each ping-pong buffer.
	    The upper 64KB usage is as the READ prediction Cache.

	    So, in the current design, the uiCacheSecNum can only use the value 0x80.
	    Because windows OS always issue 0x80 sectors in each READ command.
	    When we received 0x80 Sectors READ command, we will pre-Load next 0x80 sectors.
	    This Pre-Load prcoss would be run simutanously with the USB write EP.
	    This predict condition would be very useful in the LARGE-sized file transmission.
	*/

	//coverity[no_escape]
	while (1) {
		U2Msdc_SetFlag(FLGMSDC_CACHE_IDLE);
		PROFILE_TASK_IDLE();
		vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBCLASS, FLGMSDC_CACHE_TRIGGER|FLGMSDC_CACHE_STOP, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		if(uiFlag & FLGMSDC_CACHE_STOP) {
			THREAD_RETURN(0);
		}

		uiPreCond     = guiU2MsdcReadSlice; // Predict Condition
		uiCacheSecNum = guiU2MsdcReadSlice; // Pre-Load Size

		if (MsdcCacheInfo.Dir == MSDC_CACHE_WRITE) {
			Ret = gU2MsdInfo[MsdcCacheInfo.uiLUN].pStrgDevObj->WrSectors((INT8 *)MsdcCacheInfo.uiAddr, MsdcCacheInfo.uiLBA, MsdcCacheInfo.uiSecNum);

			if (Ret != E_OK) {
				DBG_ERR("write error!\r\n");
			}

		} else if (MsdcCacheInfo.Dir == MSDC_CACHE_READ) {

			Ret = gU2MsdInfo[MsdcCacheInfo.uiLUN].pStrgDevObj->RdSectors((INT8 *)MsdcCacheInfo.uiAddr, MsdcCacheInfo.uiLBA, MsdcCacheInfo.uiSecNum);

			if (Ret != E_OK) {
				DBG_ERR("read error 1!\r\n");
			}


			//
			// Make the Main task USB return the READ Data first
			//
			U2Msdc_SetFlag(FLGMSDC_CACHE_READDONE);


			//
			// Load Prediction Cache Sectors
			//
			if (MsdcCacheInfo.uiSecNum == uiPreCond) {
				uiCacheAddr     = (MsdcCacheInfo.uiAddr + (uiCacheSecNum * 512));
				uiCacheLBA      =  MsdcCacheInfo.uiLBA + uiCacheSecNum;

				if ((gU2MsdInfo[MsdcCacheInfo.uiLUN].dwMAX_LBA + 1) >= (uiCacheLBA + uiCacheSecNum)) {
					Ret = gU2MsdInfo[MsdcCacheInfo.uiLUN].pStrgDevObj->RdSectors((INT8 *)uiCacheAddr, uiCacheLBA, uiCacheSecNum);

					if (Ret != E_OK) {
						DBG_ERR("read error 2!\r\n");
					} else {
						MsdcCachedData.Dir      = MSDC_CACHE_READ;
						MsdcCachedData.uiLUN    = MsdcCacheInfo.uiLUN;
						MsdcCachedData.uiAddr   = uiCacheAddr;
						MsdcCachedData.uiLBA    = uiCacheLBA;
						MsdcCachedData.uiSecNum = uiCacheSecNum;
					}
				}
			}
		}
#if MSDC_CACHEREAD_L2_HANDLE
		else if ((MsdcCacheInfo.Dir == MSDC_CACHE_READ_HIT) && (MsdcCachedData.Dir != MSDC_CACHE_READ_HIT)) {
			if (MsdcCacheInfo.uiSecNum == uiPreCond) {
				if (MsdcCacheInfo.uiAddr != MsdcCachedData.uiAddr) {
					uiCacheAddr     =  MsdcCacheInfo.uiAddr;
				} else {
					uiCacheAddr     = (MsdcCacheInfo.uiAddr + (uiCacheSecNum * 512));
				}

				uiCacheLBA      =  MsdcCacheInfo.uiLBA + uiCacheSecNum;

				if ((gU2MsdInfo[MsdcCacheInfo.uiLUN].dwMAX_LBA + 1) >= (uiCacheLBA + uiCacheSecNum)) {
					Ret = gU2MsdInfo[MsdcCacheInfo.uiLUN].pStrgDevObj->RdSectors((INT8 *)uiCacheAddr, uiCacheLBA, uiCacheSecNum);

					if (Ret != E_OK) {
						DBG_ERR("read error 3!\r\n");
					} else {
						MsdcCachedData.Dir      = MSDC_CACHE_READ;
						MsdcCachedData.uiLUN    = MsdcCacheInfo.uiLUN;
						MsdcCachedData.uiAddr   = uiCacheAddr;
						MsdcCachedData.uiLBA    = uiCacheLBA;
						MsdcCachedData.uiSecNum = uiCacheSecNum;
					}
				}
			}
		}
#endif

	}

}
#if 1

/**
    Open USB MSDC(Mass-Storage-Device-Class) Task

    Open USB MSDC(Mass-Storage-Device-Class) Task. Before using this API to open MSDC task, the user must use Msdc_InstallID()
    to retrieve the system task ID and the flag ID for MSDC Task usage to keep the task normal working.

    @param[in] pClassInfo Information needed for opening USB MSDC Task. The user must prepare all the information needed.

    @return
     - @b E_OK:  The MSDC Task open done and success.
     - @b E_SYS: The MSDC Task is already opened or the Msdc_InstallID() has not be invoked first.
*/
ER U2Msdc_Open(PUSB_MSDC_INFO pClassInfo)
{
	UINT32  i;
	USB_MNG USBMng;

	vos_flag_create(&FLG_ID_USBCLASS,		NULL,	"FLG_ID_USBCLASS");

	if (guiOnMsdMode) {
		return E_SYS;
	}

	if (g_bU2MsdcCompositeDevice) {

		//guiU2InquiryDataAddr          = (UINT32)pClassInfo->pInquiryData;

		if(pClassInfo->uiMsdcBufSize > MSDC_MAX_BUFFER_SIZE)
			pClassInfo->uiMsdcBufSize = MSDC_MAX_BUFFER_SIZE;

		gpuiU2BulkCBWData             = (UINT32 *)(pClassInfo->uiMsdcBufAddr_va);
		gpuiU2BulkCBWData_pa          = (UINT32 *)(pClassInfo->uiMsdcBufAddr_pa);
		BulkCSWData					  = (UINT32 *)(pClassInfo->uiMsdcBufAddr_va+MSDC_CBW_BUFSIZE);
		BulkCSWData_pa				  = (UINT32 *)(pClassInfo->uiMsdcBufAddr_pa+MSDC_CBW_BUFSIZE);
		BulkINData					  = (UINT32 *)(pClassInfo->uiMsdcBufAddr_va+MSDC_CBW_BUFSIZE+64);
		BulkINData_pa				  = (UINT32 *)(pClassInfo->uiMsdcBufAddr_pa+MSDC_CBW_BUFSIZE+64);
		guiU2InquiryDataAddr		  = (UINT32  )(pClassInfo->uiMsdcBufAddr_va+MSDC_CBW_BUFSIZE+64+256);
		guiU2InquiryDataAddr_pa		  = (UINT32  )(pClassInfo->uiMsdcBufAddr_pa+MSDC_CBW_BUFSIZE+64+256);
		SenseData					  = (UINT8  *)(pClassInfo->uiMsdcBufAddr_va+MSDC_CBW_BUFSIZE+64+256+64);
		SenseData_pa				  = (UINT8  *)(pClassInfo->uiMsdcBufAddr_pa+MSDC_CBW_BUFSIZE+64+256+64);

		guiU2MsdcBufAddr[0]           = (UINT32)(pClassInfo->uiMsdcBufAddr_va    + MSDC_CBW_BUFSIZE+64+256+64+64);
		guiU2MsdcBufAddr_pa[0]           = (UINT32)(pClassInfo->uiMsdcBufAddr_pa + MSDC_CBW_BUFSIZE+64+256+64+64);
		guiU2MsdcDataBufSize 		  = ((pClassInfo->uiMsdcBufSize - (guiU2MsdcBufAddr[0] - pClassInfo->uiMsdcBufAddr_va)) & (~MSDC_MINBUFSIZE_MASK))>>1;
		guiU2MsdcBufAddr[1]           = (UINT32)(guiU2MsdcBufAddr[0] + guiU2MsdcDataBufSize);
		guiU2MsdcBufAddr_pa[1]           = (UINT32)(guiU2MsdcBufAddr_pa[0] + guiU2MsdcDataBufSize);

		guiU2MsdcReadSlice 			  = 64;
		guiU2MsdcWCCurSz			  = guiU2MsdcWCSize;
		guiU2MsdcWCBufIdx			  = 0;
		guiU2MsdcWCNextLBA			  = 0xFFFFFFFF;

		memcpy((void *)guiU2InquiryDataAddr, pClassInfo->pInquiryData, 36);
		memset((void *)SenseData, 0x00, 64);
		SenseData[0] = 0x70;
		SenseData[7] = 0x0A;

		msdc_debug(("CBW=0x%08X   BUF0=0x%08X  BUF1=0x%08X guiU2MsdcDataBufSize=0x%08X\r\n", (int)gpuiU2BulkCBWData, (int)guiU2MsdcBufAddr[0], (int)guiU2MsdcBufAddr[1], (int)guiU2MsdcDataBufSize));

		// Buffer must be larger than 256KB + 64B.
		if ((!guiU2MsdcBufAddr[0]) || (pClassInfo->uiMsdcBufSize < MSDC_MIN_BUFFER_SIZE)) {
			DBG_ERR("Buffer Size Not enough!\r\n");
			return E_SYS;
		}

		guiU2MsdcVendorInBufAddr    = 0;//Default
		guiU2MsdcVendor_cb          = pClassInfo->msdc_vendor_cb;
		guiU2MsdcCheck_cb           = pClassInfo->msdc_check_cb;


		// IN EP Address
		gUSBHSStrgConfigDesc[USB_CFG_LENGTH + USB_IF_LENGTH + 2] = (USB_EP_IN_ADDRESS  | gU2MsdcEpIN);
		gUSBFSStrgConfigDesc[USB_CFG_LENGTH + USB_IF_LENGTH + 2] = (USB_EP_IN_ADDRESS  | gU2MsdcEpIN);
		// OUT EP Address
		gUSBHSStrgConfigDesc[USB_CFG_LENGTH + USB_IF_LENGTH + USB_BULK_EP_LENGTH + 2] = (USB_EP_OUT_ADDRESS | gU2MsdcEpOUT);
		gUSBFSStrgConfigDesc[USB_CFG_LENGTH + USB_IF_LENGTH + USB_BULK_EP_LENGTH + 2] = (USB_EP_OUT_ADDRESS | gU2MsdcEpOUT);

		//For the USB device mode require to know device enter suspend mode.
		//If the USB device don't care suspend mode (most case), set this callback function as NULL.
		guiU2MsdcRWLed_cb                      = pClassInfo->msdc_RW_Led_CB;
		guiU2MsdcStopUnitLed_cb                = pClassInfo->msdc_StopUnit_Led_CB;
		guiU2MsdcSuspendLed_cb                 = pClassInfo->msdc_UsbSuspend_Led_CB;

		g_uiU2TotalLUNs                           = pClassInfo->LUNs;

		U2Msdc_InitLUNs(pClassInfo);


		USBSTRGCACHETSK_ID = vos_task_create(U2Msdc_CacheTsk,  0, "U2Msdc_CacheTsk",   PRI_USBSTRG_CACHE,	STKSIZE_USBSTRGCACHE);
		vos_task_resume(USBSTRGCACHETSK_ID);
		USBSTRGTSK_ID = vos_task_create(U2Msdc_StrgTsk,  0, "U2Msdc_StrgTsk",   PRI_USBSTRG,	STKSIZE_USBSTRGVND);
		vos_task_resume(USBSTRGTSK_ID);

		guiOnMsdMode = TRUE;

	} else {
		//U2Msdc_ClearFlag(0xFFFFFFFF);
		usb2dev_init_management(&USBMng);

		usb2dev_set_callback(USB_CALLBACK_CX_CLASS_REQUEST, (USB_GENERIC_CB) U2Msdc_ClassRequestHandler);

		gUSB2HSStrgDevDesc.id_vendor  = pClassInfo->VID;
		gUSB2HSStrgDevDesc.id_product = pClassInfo->PID;

		//guiU2InquiryDataAddr          = (UINT32)pClassInfo->pInquiryData;

		if(pClassInfo->uiMsdcBufSize > MSDC_MAX_BUFFER_SIZE)
			pClassInfo->uiMsdcBufSize = MSDC_MAX_BUFFER_SIZE;

		gpuiU2BulkCBWData             = (UINT32 *)(pClassInfo->uiMsdcBufAddr_va);
		gpuiU2BulkCBWData_pa          = (UINT32 *)(pClassInfo->uiMsdcBufAddr_pa);
		BulkCSWData					  = (UINT32 *)(pClassInfo->uiMsdcBufAddr_va+MSDC_CBW_BUFSIZE);
		BulkCSWData_pa				  = (UINT32 *)(pClassInfo->uiMsdcBufAddr_pa+MSDC_CBW_BUFSIZE);
		BulkINData					  = (UINT32 *)(pClassInfo->uiMsdcBufAddr_va+MSDC_CBW_BUFSIZE+64);
		BulkINData_pa				  = (UINT32 *)(pClassInfo->uiMsdcBufAddr_pa+MSDC_CBW_BUFSIZE+64);
		guiU2InquiryDataAddr		  = (UINT32  )(pClassInfo->uiMsdcBufAddr_va+MSDC_CBW_BUFSIZE+64+256);
		guiU2InquiryDataAddr_pa		  = (UINT32  )(pClassInfo->uiMsdcBufAddr_pa+MSDC_CBW_BUFSIZE+64+256);
		SenseData					  = (UINT8  *)(pClassInfo->uiMsdcBufAddr_va+MSDC_CBW_BUFSIZE+64+256+64);
		SenseData_pa				  = (UINT8  *)(pClassInfo->uiMsdcBufAddr_pa+MSDC_CBW_BUFSIZE+64+256+64);

		guiU2MsdcBufAddr[0]           = (UINT32)(pClassInfo->uiMsdcBufAddr_va    + MSDC_CBW_BUFSIZE+64+256+64+64);
		guiU2MsdcBufAddr_pa[0]           = (UINT32)(pClassInfo->uiMsdcBufAddr_pa + MSDC_CBW_BUFSIZE+64+256+64+64);
		guiU2MsdcDataBufSize 		  = ((pClassInfo->uiMsdcBufSize - (guiU2MsdcBufAddr[0] - pClassInfo->uiMsdcBufAddr_va)) & (~MSDC_MINBUFSIZE_MASK))>>1;
		guiU2MsdcBufAddr[1]           = (UINT32)(guiU2MsdcBufAddr[0] + guiU2MsdcDataBufSize);
		guiU2MsdcBufAddr_pa[1]           = (UINT32)(guiU2MsdcBufAddr_pa[0] + guiU2MsdcDataBufSize);

		guiU2MsdcReadSlice 			  = 64;
		guiU2MsdcWCCurSz			  = guiU2MsdcWCSize;
		guiU2MsdcWCBufIdx			  = 0;
		guiU2MsdcWCNextLBA			  = 0xFFFFFFFF;

		memcpy((void *)guiU2InquiryDataAddr, pClassInfo->pInquiryData, 36);
		memset((void *)SenseData, 0x00, 64);
		SenseData[0] = 0x70;
		SenseData[7] = 0x0A;

		msdc_debug(("CBW=0x%08X   BUF0=0x%08X  BUF1=0x%08X guiU2MsdcDataBufSize=0x%08X\r\n", (int)gpuiU2BulkCBWData, (int)guiU2MsdcBufAddr[0], (int)guiU2MsdcBufAddr[1], (int)guiU2MsdcDataBufSize));

		// Buffer must be larger than 256KB + 64B.
		if ((!guiU2MsdcBufAddr[0]) || (pClassInfo->uiMsdcBufSize < MSDC_MIN_BUFFER_SIZE)) {
			DBG_ERR("Buffer Size Not enough!\r\n");
			return E_SYS;
		}

		guiU2MsdcVendorInBufAddr    = 0;//Default
		guiU2MsdcVendor_cb          = pClassInfo->msdc_vendor_cb;
		guiU2MsdcCheck_cb           = pClassInfo->msdc_check_cb;


		// IN EP Address
		gUSBHSStrgConfigDesc[USB_CFG_LENGTH + USB_IF_LENGTH + 2] = (USB_EP_IN_ADDRESS  | gU2MsdcEpIN);
		gUSBFSStrgConfigDesc[USB_CFG_LENGTH + USB_IF_LENGTH + 2] = (USB_EP_IN_ADDRESS  | gU2MsdcEpIN);
		// OUT EP Address
		gUSBHSStrgConfigDesc[USB_CFG_LENGTH + USB_IF_LENGTH + USB_BULK_EP_LENGTH + 2] = (USB_EP_OUT_ADDRESS | gU2MsdcEpOUT);
		gUSBFSStrgConfigDesc[USB_CFG_LENGTH + USB_IF_LENGTH + USB_BULK_EP_LENGTH + 2] = (USB_EP_OUT_ADDRESS | gU2MsdcEpOUT);

		// initialize USB device description
		USBMng.p_dev_desc          = (USB_DEVICE_DESC *)&gUSB2HSStrgDevDesc;

		USBMng.p_config_desc_hs       = (USB_CONFIG_DESC *)&gUSBHSStrgConfigDesc;
		USBMng.p_config_desc_fs       = (USB_CONFIG_DESC *)&gUSBFSStrgConfigDesc;
		USBMng.p_config_desc_fs_other  = (USB_CONFIG_DESC *)&gUSBFSOtherStrgConfigDesc;
		USBMng.p_config_desc_hs_other  = (USB_CONFIG_DESC *)&gUSBHSOtherStrgConfigDesc;

		USBMng.p_dev_quali_desc       = (USB_DEVICE_DESC *)&gUSB2HSStrgDevQualiDesc;
		USBMng.num_of_configurations  = 1;
		USBMng.num_of_strings         = 4;
		USBMng.p_string_desc[0]      = (USB_STRING_DESC *)&USBStrgStrDesc0;

		if ((pClassInfo->pManuStringDesc) && (pClassInfo->pManuStringDesc->b_descriptor_type == 0x3)) {
			USBMng.p_string_desc[1]  = (USB_STRING_DESC *)(pClassInfo->pManuStringDesc);
		} else {
			USBMng.p_string_desc[1]  = (USB_STRING_DESC *)&USBMSDCManuStrDesc1;
		}

		if ((pClassInfo->pProdStringDesc) && (pClassInfo->pProdStringDesc->b_descriptor_type == 0x3)) {
			USBMng.p_string_desc[2]  = (USB_STRING_DESC *)(pClassInfo->pProdStringDesc);
		} else {
			USBMng.p_string_desc[2]  = (USB_STRING_DESC *)&USBMSDCProdStrDesc2;
		}

		if (pClassInfo->pSerialStringDesc) {
			USBMng.p_string_desc[3]  = (USB_STRING_DESC *)(pClassInfo->pSerialStringDesc);
		} else {
			USBMng.p_string_desc[3]  = (USB_STRING_DESC *)&USBStrgStrDesc3;
		}

		//
		//  Configure USB Endpoint allocation
		//
		for (i = 0; i < 8; i++) {
			USBMng.ep_config_hs[i].enable      = FALSE;
			USBMng.ep_config_fs[i].enable      = FALSE;
		}

		// Config High Speed endpoint usage.
		USBMng.ep_config_hs[gU2MsdcEpIN - 1].enable             = TRUE; //endpoint IN
		USBMng.ep_config_hs[gU2MsdcEpIN - 1].blk_size            = 512;
		USBMng.ep_config_hs[gU2MsdcEpIN - 1].blk_num             = BLKNUM_TRIPLE;
		USBMng.ep_config_hs[gU2MsdcEpIN - 1].direction          = EP_DIR_IN;
		USBMng.ep_config_hs[gU2MsdcEpIN - 1].trnsfer_type        = EP_TYPE_BULK;
		USBMng.ep_config_hs[gU2MsdcEpIN - 1].max_pkt_size         = 512;
		USBMng.ep_config_hs[gU2MsdcEpIN - 1].high_bandwidth      = HBW_NOT;

		USBMng.ep_config_hs[gU2MsdcEpOUT - 1].enable            = TRUE; //endpoint OUT
		USBMng.ep_config_hs[gU2MsdcEpOUT - 1].blk_size           = 512;
		USBMng.ep_config_hs[gU2MsdcEpOUT - 1].blk_num            = BLKNUM_TRIPLE;
		USBMng.ep_config_hs[gU2MsdcEpOUT - 1].direction         = EP_DIR_OUT;
		USBMng.ep_config_hs[gU2MsdcEpOUT - 1].trnsfer_type       = EP_TYPE_BULK;
		USBMng.ep_config_hs[gU2MsdcEpOUT - 1].max_pkt_size        = 512;
		USBMng.ep_config_hs[gU2MsdcEpOUT - 1].high_bandwidth     = HBW_NOT;

		// Config Full Speed endpoint usage.
		USBMng.ep_config_fs[gU2MsdcEpIN - 1].enable             = TRUE; //endpoint IN
		USBMng.ep_config_fs[gU2MsdcEpIN - 1].blk_size            = 512;
		USBMng.ep_config_fs[gU2MsdcEpIN - 1].blk_num             = BLKNUM_DOUBLE;
		USBMng.ep_config_fs[gU2MsdcEpIN - 1].direction          = EP_DIR_IN;
		USBMng.ep_config_fs[gU2MsdcEpIN - 1].trnsfer_type        = EP_TYPE_BULK;
		USBMng.ep_config_fs[gU2MsdcEpIN - 1].max_pkt_size         = 64;
		USBMng.ep_config_fs[gU2MsdcEpIN - 1].high_bandwidth      = HBW_NOT;

		USBMng.ep_config_fs[gU2MsdcEpOUT - 1].enable            = TRUE; //endpoint OUT
		USBMng.ep_config_fs[gU2MsdcEpOUT - 1].blk_size           = 512;
		USBMng.ep_config_fs[gU2MsdcEpOUT - 1].blk_num            = BLKNUM_DOUBLE;
		USBMng.ep_config_fs[gU2MsdcEpOUT - 1].direction         = EP_DIR_OUT;
		USBMng.ep_config_fs[gU2MsdcEpOUT - 1].trnsfer_type       = EP_TYPE_BULK;
		USBMng.ep_config_fs[gU2MsdcEpOUT - 1].max_pkt_size        = 64;
		USBMng.ep_config_fs[gU2MsdcEpOUT - 1].high_bandwidth     = HBW_NOT;


		USBMng.fp_open_needed_fifo              = U2Msdc_OpenNeededFIFO;
		USBMng.usb_type                         = USB_MSD;

		//For the USB device mode require to know device enter suspend mode.
		//If the USB device don't care suspend mode (most case), set this callback function as NULL.
		guiU2MsdcRWLed_cb                      = pClassInfo->msdc_RW_Led_CB;
		guiU2MsdcStopUnitLed_cb                = pClassInfo->msdc_StopUnit_Led_CB;
		guiU2MsdcSuspendLed_cb                 = pClassInfo->msdc_UsbSuspend_Led_CB;

		// Hook class event callback
		USBMng.fp_event_callback                      = U2Msdc_EventCallback;

		usb2dev_set_management(&USBMng);

		g_uiU2TotalLUNs                           = pClassInfo->LUNs;

		U2Msdc_InitLUNs(pClassInfo);


		USBSTRGCACHETSK_ID = vos_task_create(U2Msdc_CacheTsk,  0, "U2Msdc_CacheTsk",   PRI_USBSTRG_CACHE,	STKSIZE_USBSTRGCACHE);
		vos_task_resume(USBSTRGCACHETSK_ID);
		USBSTRGTSK_ID = vos_task_create(U2Msdc_StrgTsk,  0, "U2Msdc_StrgTsk",   PRI_USBSTRG,	STKSIZE_USBSTRGVND);
		vos_task_resume(USBSTRGTSK_ID);

		if (g_bU2MsdcCompositeDevice == FALSE) {
			//Detect whether connecting to a real USB host
			if (usb2dev_open() != E_OK) {
				DBG_ERR("USB open error\r\n");
				return E_CTX;
			}
		}

		guiOnMsdMode = TRUE;
	}

	return E_OK;
}

/**
    Close USB MSDC Task.
*/
void U2Msdc_Close(void)
{
	UINT32  i;
	//FLGPTN  uiFlag;

	if (!guiOnMsdMode) {
		return;
	}

	//U2Msdc_WaitIdle();
	//vos_task_destroy(USBSTRGTSK_ID);

	//U2Msdc_WaitFlag(&uiFlag, FLGMSDC_CACHE_IDLE);
	//vos_task_destroy(USBSTRGCACHETSK_ID);

	U2Msdc_SetFlag(FLGMSDC_BULKOUT_STOP);
	U2Msdc_SetFlag(FLGMSDC_CACHE_STOP);
	vos_util_delay_ms(20);

	if (g_bU2MsdcCompositeDevice == FALSE) {
		usb2dev_close();
	}

	if (MSDC_CLOSED_STATE != g_uiU2MSDCState) {
		for (i = 0; i < g_uiU2TotalLUNs; i++) {
			if (gU2MsdInfo[i].pStrgDevObj != NULL) {
				gU2MsdInfo[i].pStrgDevObj->Close();
				gU2MsdInfo[i].pStrgDevObj = NULL;
			}
		}
	} else {
		DBG_WRN("state is close!\r\n");
	}

	vos_flag_destroy(FLG_ID_USBCLASS);

	guiOnMsdMode    = FALSE;
	g_uiU2MSDCState = MSDC_CLOSED_STATE;

}

/**
    Set USB MSDC configuration

    Assign new configuration of the specified ConfigID to MSDC Task.

    @param[in] ConfigID         Configuration identifier
    @param[in] uiCfgValue       Configuration context for ConfigID

    @return
     - @b E_OK: Set configuration success
     - @b E_NOSPT: ConfigID is not supported
*/
ER U2Msdc_SetConfig(USBMSDC_CONFIG_ID ConfigID, UINT32 uiCfgValue)
{

	switch (ConfigID) {
	case USBMSDC_CONFIG_ID_COMPOSITE: {
			g_bU2MsdcCompositeDevice = uiCfgValue > 0;
		}
		break;

	case USBMSDC_CONFIG_ID_CHGVENINBUGADDR: {
			guiU2MsdcVendorInBufAddr = uiCfgValue;
		}
		break;

	case USBMSDC_CONFIG_ID_SELECT_POWER: {
			if (uiCfgValue == USBMSDC_POW_SELFPOWER) {
				gUSBHSStrgConfigDesc[USB_STRG_CFGA_CFG_ATTRIBUES_IDX]       = USB_STRG_CFGA_CFG_ATTRIBUES_SELF;
				gUSBHSStrgConfigDesc[USB_STRG_CFGA_MAX_POWER_IDX]           = USB_STRG_CFGA_MAX_POWER_SELF;

				gUSBFSStrgConfigDesc[USB_STRG_CFGA_CFG_ATTRIBUES_IDX]       = USB_STRG_CFGA_CFG_ATTRIBUES_SELF;
				gUSBFSStrgConfigDesc[USB_STRG_CFGA_MAX_POWER_IDX]           = USB_STRG_CFGA_MAX_POWER_SELF;

				gUSBHSOtherStrgConfigDesc[USB_STRG_CFGA_CFG_ATTRIBUES_IDX]  = USB_STRG_CFGA_CFG_ATTRIBUES_SELF;
				gUSBHSOtherStrgConfigDesc[USB_STRG_CFGA_MAX_POWER_IDX]      = USB_STRG_CFGA_MAX_POWER_SELF;

				gUSBFSOtherStrgConfigDesc[USB_STRG_CFGA_CFG_ATTRIBUES_IDX]  = USB_STRG_CFGA_CFG_ATTRIBUES_SELF;
				gUSBFSOtherStrgConfigDesc[USB_STRG_CFGA_MAX_POWER_IDX]      = USB_STRG_CFGA_MAX_POWER_SELF;
			} else if (uiCfgValue == USBMSDC_POW_BUSPOWER) {
				gUSBHSStrgConfigDesc[USB_STRG_CFGA_CFG_ATTRIBUES_IDX]       = USB_STRG_CFGA_CFG_ATTRIBUES_BUS;
				gUSBHSStrgConfigDesc[USB_STRG_CFGA_MAX_POWER_IDX]           = USB_STRG_CFGA_MAX_POWER_BUS;

				gUSBFSStrgConfigDesc[USB_STRG_CFGA_CFG_ATTRIBUES_IDX]       = USB_STRG_CFGA_CFG_ATTRIBUES_BUS;
				gUSBFSStrgConfigDesc[USB_STRG_CFGA_MAX_POWER_IDX]           = USB_STRG_CFGA_MAX_POWER_BUS;

				gUSBHSOtherStrgConfigDesc[USB_STRG_CFGA_CFG_ATTRIBUES_IDX]  = USB_STRG_CFGA_CFG_ATTRIBUES_BUS;
				gUSBHSOtherStrgConfigDesc[USB_STRG_CFGA_MAX_POWER_IDX]      = USB_STRG_CFGA_MAX_POWER_BUS;

				gUSBFSOtherStrgConfigDesc[USB_STRG_CFGA_CFG_ATTRIBUES_IDX]  = USB_STRG_CFGA_CFG_ATTRIBUES_BUS;
				gUSBFSOtherStrgConfigDesc[USB_STRG_CFGA_MAX_POWER_IDX]      = USB_STRG_CFGA_MAX_POWER_BUS;
			}
		}
		break;

	case USBMSDC_CONFIG_ID_SERIALNO_STRING_EN: {
			if (uiCfgValue) {
				gUSB2HSStrgDevDesc.i_serial_number = 3;
			} else {
				gUSB2HSStrgDevDesc.i_serial_number = 0;
			}
		}
		break;

	case USBMSDC_CONFIG_ID_RC_DISABLESTALL: {
			gbU2ReadCapNoStall = uiCfgValue;
		}
		break;

	case USBMSDC_CONFIG_ID_SPEED: {
		}
		break;

	case USBMSDC_CONFIG_ID_WCACHE_ADDR: {
			if (guiOnMsdMode) {
				DBG_ERR("MSDC opened. WCB setup fail\r\n");
				break;
			}

			guiU2MsdcWCBufAddr[0] = dma_getCacheAddr(uiCfgValue);

			if(guiU2MsdcWCSize && (guiU2MsdcWCBufAddr[1]==0)) {
				dma_flushReadCache(guiU2MsdcWCBufAddr[0], guiU2MsdcWCSize);
				dma_flushWriteCache(guiU2MsdcWCBufAddr[0], guiU2MsdcWCSize);

				guiU2MsdcWCBufAddr[1] = guiU2MsdcWCBufAddr[0]+(guiU2MsdcWCSize>>1);
				guiU2MsdcWCSize = guiU2MsdcWCSize>>1;
				guiU2MsdcWCCurSz= guiU2MsdcWCSize;
			}
		}
		break;
	case USBMSDC_CONFIG_ID_WCACHE_SIZE: {
			if (guiOnMsdMode) {
				DBG_ERR("MSDC opened. WCB setup fail\r\n");
				break;
			}

			if(uiCfgValue >= MSDC_WCACHE_MIN_BUFFER_SIZE) {
				// For USB2 controller, 1MB is ok.
				guiU2MsdcWCSize = MSDC_WCACHE_MIN_BUFFER_SIZE;

				if(guiU2MsdcWCBufAddr[0]) {
					dma_flushReadCache(guiU2MsdcWCBufAddr[0], guiU2MsdcWCSize);
					dma_flushWriteCache(guiU2MsdcWCBufAddr[0], guiU2MsdcWCSize);

					guiU2MsdcWCBufAddr[1] = guiU2MsdcWCBufAddr[0]+(guiU2MsdcWCSize>>1);
					guiU2MsdcWCSize = guiU2MsdcWCSize>>1;
					guiU2MsdcWCCurSz= guiU2MsdcWCSize;
				}
			}
		}
		break;
	case USBMSDC_CONFIG_ID_WCACHE_SLICE: {
			guiU2MsdcWCSlice = uiCfgValue;
		}
		break;

#if 1
	case USBMSDC_CONFIG_ID_RESERVED: {
			if (guiOnMsdMode && (g_bU2MsdcCompositeDevice == FALSE)) {
				DBG_ERR("MSDC opened. Not Allow to change EP number mapping\r\n");
			} else {
				if (uiCfgValue) {
					gU2MsdcEpIN = uiCfgValue & 0xF;

					gU2MsdcEpOUT = gU2MsdcEpIN + 1;
					if (gU2MsdcEpOUT == USB_EP_MAX) {
						gU2MsdcEpOUT = USB_EP1;
					}
				}
			}

			DBG_WRN("MSDC IN= EP%d   OUT=EP%d\r\n", gU2MsdcEpIN, gU2MsdcEpOUT);
		}
		break;
#endif
	case USBMSDC_CONFIG_ID_INTERFACE_IDX: {
			guiU2MsdcInterfaceIdx = uiCfgValue;
		}
		break;

	default: {
			return E_NOSPT;
		}

	}

	return E_OK;
}

/**
    Get USB MSDC configuration

    Retrieve current configuration of the specified ConfigID from MSDC Task.

    @param[in] ConfigID         Configuration identifier

    @return  Configuration context for ConfigID
*/
UINT32 U2Msdc_GetConfig(USBMSDC_CONFIG_ID ConfigID)
{
	UINT32 Ret = 0;

	switch (ConfigID) {
	case USBMSDC_CONFIG_ID_COMPOSITE: {
			Ret = g_bU2MsdcCompositeDevice;
		}
		break;

	case USBMSDC_CONFIG_ID_CHGVENINBUGADDR: {
			if (guiU2MsdcVendorInBufAddr) {
				Ret = guiU2MsdcVendorInBufAddr;
			} else {
				Ret = guiU2MsdcBufAddr[guiU2MsdcBufIdx];
			}
		}
		break;

	case USBMSDC_CONFIG_ID_EVT_CB: {
			Ret = (UINT32)U2Msdc_EventCallback;
		}
		break;
	case USBMSDC_CONFIG_ID_CLASS_CB: {
			Ret = (UINT32)U2Msdc_ClassRequestHandler;
		}
		break;

	default:
		DBG_WRN("Get Config ID err\r\n");
		break;
	}

	return Ret;
}

/**
    Set new configuration to specified Logical Unit Number(LUN)

    Assign new configuration to specified Logical Unit Number(LUN).

    @param[in] uiLun        Specified Logical Unit Number
    @param[in] ConfigID     Function select ID
    @param[in] uiCfgValue   Configuration context for ConfigID

    @return
     - @b E_OK: Set configuration success
     - @b E_NOSPT: ConfigID is not supported
*/
ER U2Msdc_SetLunConfig(UINT32 uiLun, USBMSDC_LUN_CONFIG_ID ConfigID, UINT32 uiCfgValue)
{

	switch (ConfigID) {
	case USBMSDC_LUN_CONFIG_ID_FORCE_EJECT: {
			gU2MsdInfo[uiLun].StorageEject = uiCfgValue > 0;
		}
		break;

	case USBMSDC_LUN_CONFIG_ID_WRITEPROTECT: {
			gU2MsdInfo[uiLun].WriteProtect = uiCfgValue > 0;
		}
		break;

	default: {
			return E_NOSPT;
		}

	}

	return E_OK;
}

/**
    Get new configuration to specified Logical Unit Number(LUN)

    Retrieve current configuration of the specified Logical Unit Number(LUN).

    @param[in] uiLun        Specified Logical Unit Number
    @param[in] ConfigID     Function select ID

    @return Configuration context for ConfigID from the specified Logical Unit Number(LUN)
*/
UINT32 U2Msdc_GetLunConfig(UINT32 uiLun, USBMSDC_LUN_CONFIG_ID ConfigID)
{
	UINT32 Ret = 0;

	switch (ConfigID) {
	case USBMSDC_LUN_CONFIG_ID_FORCE_EJECT: {
			Ret = gU2MsdInfo[uiLun].StorageEject;
		}
		break;

	case USBMSDC_LUN_CONFIG_ID_WRITEPROTECT: {
			Ret = gU2MsdInfo[uiLun].WriteProtect;
		}
		break;

	default:
		break;
	}

	return Ret;
}

/**
    Change specified LUN's Storage Object

    This API can be used to change the specified LUN's Storage Object.
    This API is only allowed to use only if the specified LUN is in the detached state, such as the LUN is ejected or unplung.
    If the card is still plugged, the user can use Msdc_SetLunConfig() to forcing the card ejected and then using this
    API to change the storage object.

    @param[in] uiLun        Specified Logical Unit Number selected to change the storage object.
    @param[in] pStrgInfo    New storage information used to change the storage object.

    @return void
*/
void U2Msdc_ChgStrgObject(UINT32 uiLun, PUSB_MSDC_STRG_INFO pStrgInfo)
{
	if (!((gU2MsdInfo[uiLun].LastDetStrg == FALSE) || (gU2MsdInfo[uiLun].StorageEject == TRUE))) {
		DBG_ERR("Can only used for detached LUN!!\r\n");
		return;
	}

	if (uiLun >= g_uiU2TotalLUNs) {
		DBG_ERR("Error Lun Number!\r\n");
		return;
	}

	if (gU2MsdInfo[uiLun].pStrgDevObj != NULL) {
		gU2MsdInfo[uiLun].pStrgDevObj->Close();
	}

	gU2MsdInfo[uiLun].RequestSense.bValid     = 0x00;
	gU2MsdInfo[uiLun].RequestSense.bSenseKey  = 0x00;
	gU2MsdInfo[uiLun].RequestSense.bASC       = 0x00;
	gU2MsdInfo[uiLun].RequestSense.bASCQ      = 0x00;

	gU2MsdInfo[uiLun].MEMBusClosed            = FALSE;
	gU2MsdInfo[uiLun].StorageEject            = TRUE;
	gU2MsdInfo[uiLun].StorageChanged          = FALSE;
	gU2MsdInfo[uiLun].WriteProtect            = FALSE;
	gU2MsdInfo[uiLun].LastDetStrg             = FALSE;

	gU2MsdInfo[uiLun].pStrgDevObj = pStrgInfo->StrgHandle;
	if (gU2MsdInfo[uiLun].pStrgDevObj == 0) {
		DBG_WRN("Get Lun Strg Obj Fail!(%d)\r\n", uiLun);
	}

	gU2MsdInfo[uiLun].DetStrg_cb              = pStrgInfo->msdc_storage_detCB;
	gU2MsdInfo[uiLun].DetStrgLock_cb          = pStrgInfo->msdc_strgLock_detCB;


	//if (Dx_Close(pStrgInfo->StrgHandle) != DX_OK) {
	//	DBG_WRN("DX Handle Close Fail!\r\n");
	//}

}

/**
    Get the status of the MSDC Task.

    @return The returned MSDC Task status. Please refer to MSDC_TASK_STS for details.
*/
MSDC_TASK_STS U2Msdc_GetStatus(void)
{
	return g_uiU2MSDCState;
}

/**
    Check MSDC idle Flag

    Check the MSDC Task is running or in the IDLE state.

    @return
     - @b TRUE:   MSDC Task is IDLE.
     - @b FALSE:  MSDC Task is Running.
*/
BOOL U2Msdc_CheckIdle(void)
{
	if (kchk_flg(FLG_ID_USBCLASS, FLGMSDC_IDLE)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

//@}
#endif

#if 1
static MSDC_OBJ gpMsdcObject[MSDC_ID_MAX] = {
	{
		MSDC_ID_USB20,
		U2Msdc_Open,
		U2Msdc_Close,
		U2Msdc_SetConfig,
		U2Msdc_GetConfig,
		U2Msdc_SetLunConfig,
		U2Msdc_GetLunConfig,
		U2Msdc_ChgStrgObject,
		U2Msdc_GetStatus,
		U2Msdc_CheckIdle
	}
};

/**

*/
PMSDC_OBJ Msdc_getObject(MSDC_ID MsdcID)
{
	if (MsdcID >= MSDC_ID_MAX) {
		DBG_ERR("No such module.%d\r\n", MsdcID);
		return NULL;
	}

	return &gpMsdcObject[MsdcID];
}
#endif



