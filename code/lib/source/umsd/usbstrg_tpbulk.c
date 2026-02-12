/*
    USB MSDC Task Bulk Transfer Commands

    Functions to handle the SCSI command set used for the USB MSDC Bulk-Only Transport.
    This file also contains the main commands handshake flow of the mass storage task.

    @file       UsbTpBulk.c
    @ingroup    mILibUsbMSDC
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "usbstrg_task.h"
#include "usbstrg_tpbulk.h"
#include "umsd.h"
#include "usb2dev.h"


static PCBW                	pTPBulk_CommandBlock;   // a pointer of CBW data structure
static PCSW                	pTPBulk_CommandStatus;  // a pointer of CSW data structure
static MSDCVendorParam     	VendorParam;
UINT32              		*BulkINData;         	// data buffer for Bulk in
UINT32              		*BulkINData_pa;         // data buffer for Bulk in
UINT32              		*BulkCSWData;        	// data buffer for CSW
UINT32              		*BulkCSWData_pa;        // data buffer for CSW

NVTIB               		gU2MsdRwInfoBlock;        // information block for SCSI command decoding operations
UINT32              	   *gpuiU2BulkCBWData;           // data buffer for CBW
UINT32              	   *gpuiU2BulkCBWData_pa;        // data buffer for CBW

MASS_STORAGE        		gU2MsdInfo[MAX_LUN];      // data structures for each MSDC logic LUN

UINT8 *SenseData;//[20] = {              // data buffer for Sense data
//	0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A,
//	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//	0x00, 0x00, 0x00, 0x00
//};

UINT8 *SenseData_pa;

#if 1
/*
static UINT8 Msdc_CalcBits(UINT32 dwSize)
{
	UINT8 i = 0;

	if (dwSize == 0) {
		return 0;
	}

	while ((dwSize & 0x01) == 0) {
		i++;
		dwSize = dwSize >> 1;
	}

	return i ;
}
*/

static void Msdc_PhaseErrorHandler(void)
{
	FLGPTN  uiFlag = 0;
	BOOL    MSDCResetGot = FALSE;
	UINT8   CLRFEATUREGot = 0;

	DBG_WRN("Waiting for the reset recovery\r\n");

	// Modify to pass USB-CV MSC Test
	usb2dev_unmask_ep_interrupt(gU2MsdcEpOUT);
	U2Msdc_ClearFlag(FLGMSDC_USBRESET);
	usb2dev_set_ep_stall(gU2MsdcEpOUT);

	while (1) {
		U2Msdc_WaitFlag(&uiFlag, FLGMSDC_CLRFEATURE | FLGMSDC_MSDCRESET | FLGMSDC_USBRESET);

		if (uiFlag & FLGMSDC_USBRESET) {
			g_uiU2MSDCState = MSDC_USBRESETED;
			break;
		} else if (uiFlag & FLGMSDC_MSDCRESET) {
			MSDCResetGot = TRUE;
		} else if (uiFlag & FLGMSDC_CLRFEATURE) {
			if (MSDCResetGot == TRUE) {
				CLRFEATUREGot++;
				/*
				    The reset recovery is composed of
				    (a) a Bulk-Only Mass Storage Reset
				    (b) a Clear Feature HALT to the Bulk-In endpoint
				    (c) a Clear Feature HALT to the Bulk-Out endpoint
				*/
				if (CLRFEATUREGot == 2) {
					DBG_WRN("Ready for the next CBW\r\n");
					g_uiU2MSDCState = MSDC_COMMANDOK;

					usb2dev_clear_ep_fifo(gU2MsdcEpIN);
					usb2dev_clear_ep_fifo(gU2MsdcEpOUT);
					U2Msdc_ClearFlag(FLGMSDC_BULKOUT);

					break;
				}
			} else {
				usb2dev_set_ep_stall(gU2MsdcEpIN);
				usb2dev_set_ep_stall(gU2MsdcEpOUT);
			}
		}
	}

	usb2dev_mask_ep_interrupt(gU2MsdcEpOUT);
	return;
}

static void Msdc_EndpointStallHandler(USB_EP EpNum)
{
	FLGPTN          uiFlag = 0;

	usb2dev_unmask_ep_interrupt(gU2MsdcEpOUT);

	if (EpNum == gU2MsdcEpIN) {
		// stall data in endpoint
		usb2dev_set_ep_stall(EpNum);
	} else {
		//Don't stall the OUT endpoint until one data packet (either short or MaxPacketSize's packet) is completed
		U2Msdc_WaitFlag(&uiFlag, FLGMSDC_BULKOUT);

		// Modify to pass USB-CV MSC Test
		// stall data out endpoint
		usb2dev_set_ep_stall(EpNum);
	}

	U2Msdc_ClearFlag(FLGMSDC_CLRFEATURE | FLGMSDC_USBRESET);
	U2Msdc_WaitFlag(&uiFlag, FLGMSDC_CLRFEATURE | FLGMSDC_USBRESET);


	if (uiFlag & FLGMSDC_USBRESET) {
		g_uiU2MSDCState = MSDC_USBRESETED;
	} else if (uiFlag & FLGMSDC_CLRFEATURE) {
		// Even we had stalled the OUT endpoint, we still have to read the received data out of the OUT FIFO, then discard them
		if (EpNum == gU2MsdcEpOUT) {
			if (usb2dev_get_ep_bytecount(EpNum)) {
				usb2dev_clear_ep_fifo(EpNum);
			}

			U2Msdc_ClearFlag(FLGMSDC_BULKOUT);
		}
	}

	usb2dev_mask_ep_interrupt(gU2MsdcEpOUT);

}

static void Msdc_BuildSenseData(UINT8 LUN, UINT8 Type, UINT8 ASC, UINT8 ASCQ)
{
	gU2MsdInfo[LUN].RequestSense.bValid = TRUE;
	gU2MsdInfo[LUN].RequestSense.bSenseKey = Type;
	gU2MsdInfo[LUN].RequestSense.bASC = ASC;
	gU2MsdInfo[LUN].RequestSense.bASCQ = ASCQ;
}

static void Msdc_BuildCSWData(UINT8 Status, UINT32 DevXferCount)
{
	pTPBulk_CommandStatus->dCSW_DataResidue = DevXferCount;
	pTPBulk_CommandStatus->bCSW_Status = Status;
	pTPBulk_CommandStatus->dCSW_Signature = CSW_SIGNATURE;
	pTPBulk_CommandStatus->dCSW_Tag = pTPBulk_CommandBlock->dCBW_Tag;
}

static void Msdc_FailedCommandHandler(void)
{
	FLGPTN  uiFlag;

	usb2dev_unmask_ep_interrupt(gU2MsdcEpOUT);

	if (pTPBulk_CommandBlock->dCBW_DataXferLen > 0) {
		if (pTPBulk_CommandBlock->bCBW_Flag & CBW_FLAG_IN) {
			Msdc_EndpointStallHandler(gU2MsdcEpIN);

			if (g_uiU2MSDCState == MSDC_PHASEERROR) {
				Msdc_BuildCSWData(CSW_PHASE_ERROR, pTPBulk_CommandBlock->dCBW_DataXferLen);
			} else {
				Msdc_BuildCSWData(CSW_FAIL, pTPBulk_CommandBlock->dCBW_DataXferLen);
			}
		} else {
			//Stall the EP2 only if the CBW_DataXferLen is larger than 2 X U2EP2_PACKET_SIZE(ping-pong buffer)
			if (pTPBulk_CommandBlock->dCBW_DataXferLen > (U2EP2_PACKET_SIZE * 2)) {
				Msdc_EndpointStallHandler(gU2MsdcEpOUT);

				if (g_uiU2MSDCState == MSDC_PHASEERROR) {
					Msdc_BuildCSWData(CSW_PHASE_ERROR, pTPBulk_CommandBlock->dCBW_DataXferLen - (U2EP2_PACKET_SIZE * 2));
				} else {
					Msdc_BuildCSWData(CSW_FAIL, pTPBulk_CommandBlock->dCBW_DataXferLen - (U2EP2_PACKET_SIZE * 2));
				}
			} else {
				//Wait until one data packet (either short or MaxPacketSize's packet) is completed
				U2Msdc_WaitFlag(&uiFlag, FLGMSDC_BULKOUT);

				// Fix CV Case 9/11 error due to false bulkout event flag
				usb2dev_mask_ep_interrupt(gU2MsdcEpOUT);

				U2Msdc_ClearFlag(FLGMSDC_BULKOUT);

				//Read them out and discard
				U2Msdc_SetIdle();
				if(usb2dev_read_endpoint(gU2MsdcEpOUT, (UINT8 *)(guiU2MsdcBufAddr[guiU2MsdcBufIdx]), &(pTPBulk_CommandBlock->dCBW_DataXferLen)) != E_OK) {
					DBG_WRN("READ ERR\r\n");
				}
				U2Msdc_ClearIdle();

				// Fix CV Case 9/11 error due to false bulkout event flag
				usb2dev_unmask_ep_interrupt(gU2MsdcEpOUT);

				//The data of CBW_DataXferLen has been all received, so we don't need to stall the EP2
				if (g_uiU2MSDCState == MSDC_PHASEERROR) {
					Msdc_BuildCSWData(CSW_PHASE_ERROR, 0);
				} else {
					Msdc_BuildCSWData(CSW_FAIL, 0);
				}
			}
		}
	} else {
		if (g_uiU2MSDCState == MSDC_PHASEERROR) {
			Msdc_BuildCSWData(CSW_PHASE_ERROR, 0);
		} else {
			Msdc_BuildCSWData(CSW_FAIL, 0);
		}
	}

	usb2dev_mask_ep_interrupt(gU2MsdcEpOUT);

}

static ER Msdc_FlushWriteCacheBuffer(UINT32 NVT_LUN, UINT32 SectorBits)
{
	ER Result;
	MSDC_CACHE_INFO CacheInfo;

	if((guiU2MsdcWCSize-guiU2MsdcWCCurSz) > 0) {
		CacheInfo.Dir       = MSDC_CACHE_WRITE;
		CacheInfo.uiLUN     = NVT_LUN;
		CacheInfo.uiAddr    = guiU2MsdcWCBufAddr[guiU2MsdcWCBufIdx];
		CacheInfo.uiLBA     = guiU2MsdcWCNextLBA - ((guiU2MsdcWCSize-guiU2MsdcWCCurSz)>>SectorBits);
		CacheInfo.uiSecNum  = ((guiU2MsdcWCSize-guiU2MsdcWCCurSz)>>SectorBits);
		dma_flushWriteCache(guiU2MsdcWCBufAddr[guiU2MsdcWCBufIdx], (guiU2MsdcWCSize-guiU2MsdcWCCurSz));
		Result = U2Msdc_TriggerCache(&CacheInfo);

		guiU2MsdcWCBufIdx = !guiU2MsdcWCBufIdx;
		guiU2MsdcWCCurSz  = guiU2MsdcWCSize;
		guiU2MsdcWCNextLBA= 0xFFFFFFFF;
	} else {
		Result = E_OK;
	}
	return Result;
}
#endif

void U2Msdc_CBWHandler(void)
{
	UINT32  CBWsize;
	UINT32  len;

	CBWsize = (UINT8)usb2dev_get_ep_bytecount(gU2MsdcEpOUT);
	//msdc_debug(("!!!!!Command Size = %d\r\n",CBWsize));

	U2Msdc_SetIdle();
	vos_cpu_dcache_sync((VOS_ADDR)gpuiU2BulkCBWData, CBWsize, VOS_DMA_BIDIRECTIONAL);
	if(usb2dev_read_endpoint(gU2MsdcEpOUT, (UINT8 *)(gpuiU2BulkCBWData_pa), &CBWsize) != E_OK) {
		DBG_WRN("READ ERR\r\n");
	}
	vos_cpu_dcache_sync((VOS_ADDR)gpuiU2BulkCBWData, CBWsize, VOS_DMA_BIDIRECTIONAL);
	U2Msdc_ClearIdle();

	// Move CBW to non-cache buffer to support DMA-WP for FW region
	pTPBulk_CommandBlock    = (PCBW) gpuiU2BulkCBWData;
	pTPBulk_CommandStatus   = (PCSW) BulkCSWData;

	U2Msdc_ClearFlag(FLGMSDC_CLRFEATURE | FLGMSDC_USBRESET | FLGMSDC_MSDCRESET);

	if ((CBWsize == CBW_VALID_SIZE) &&
		(pTPBulk_CommandBlock->dCBW_Signature == CBW_SIGNATURE) &&
		(pTPBulk_CommandBlock->bCBW_LUN < g_uiU2TotalLUNs) &&
		(pTPBulk_CommandBlock->bCBW_CDBLen <= MAX_CDBLEN)) {
		g_uiU2MSDCState = MSDC_COMMANDOK;

		U2Msdc_CommandHandler();

		if (g_uiU2MSDCState != MSDC_USBRESETED) {
			//Returns the 13 bytes of CSW data
			len = CSW_SIZE;

			U2Msdc_SetIdle();
			vos_cpu_dcache_sync((VOS_ADDR)BulkCSWData, len, VOS_DMA_BIDIRECTIONAL);
			usb2dev_write_endpoint(gU2MsdcEpIN, (UINT8 *)(BulkCSWData_pa), &len);
			vos_cpu_dcache_sync((VOS_ADDR)BulkCSWData, len, VOS_DMA_BIDIRECTIONAL);
			U2Msdc_ClearIdle();
		}

		if (g_uiU2MSDCState == MSDC_PHASEERROR) {
			Msdc_PhaseErrorHandler();
		}
	} else {
		DBG_WRN("Invalid CBW received\r\n");
		g_uiU2MSDCState = MSDC_PHASEERROR;
		Msdc_PhaseErrorHandler();
	}

}

void U2Msdc_CommandHandler(void)
{
	UINT32 CBW_LUN, CDB_cmdid, CBW_DataXferLen;
	UINT32 CDB_DataXferLen;
	BOOL   result = FALSE;
	UINT32 OutDataBuf;
	ER     CheckOK;
	BOOL   CheckStrg;
	BOOL   NewDetStrg;
	UINT32 len;

	VendorParam.VendorCmdBuf = (UINT32)pTPBulk_CommandBlock;
	VendorParam.VendorCSWBuf = (UINT32)pTPBulk_CommandStatus;

	CDB_cmdid = pTPBulk_CommandBlock->CBW_CDB[0];
	CBW_LUN   = pTPBulk_CommandBlock->bCBW_LUN;
	CBW_DataXferLen = pTPBulk_CommandBlock->dCBW_DataXferLen;

	if (gU2MsdRwInfoBlock.bNVT_LastOpenedLUN != CBW_LUN) {
		U2Msdc_FlushCache();
	}

	if (gU2MsdInfo[CBW_LUN].DetStrg_cb != NULL) {
		NewDetStrg = gU2MsdInfo[CBW_LUN].DetStrg_cb();
	} else {
		NewDetStrg = TRUE;
	}

	if (NewDetStrg != gU2MsdInfo[CBW_LUN].LastDetStrg) {
		gU2MsdInfo[CBW_LUN].StorageEject = !(NewDetStrg);
		gU2MsdInfo[CBW_LUN].LastDetStrg = NewDetStrg;

		if (NewDetStrg == TRUE) {
			gU2MsdInfo[CBW_LUN].StorageChanged = TRUE;

			// Close the last opened LUN
			if (gU2MsdRwInfoBlock.bNVT_LastOpenedLUN != NO_OPENED_LUN) {
				if (gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].DetStrg_cb != NULL) {
					CheckStrg = gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].DetStrg_cb();
				} else {
					CheckStrg = TRUE;
				}

				// If the storage is not present,
				// then we CAN'T do the CloseMemBus()/OpenMemBus()
				if (((gU2MsdInfo[CBW_LUN].pStrgDevObj->uiStrgType) == (gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].pStrgDevObj->uiStrgType))
					|| (gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].pStrgDevObj->CloseMemBus == NULL)
					|| (CheckStrg == FALSE)) {

					// Flush WCB
					if((guiU2MsdcWCSize>0) && (guiU2MsdcWCBufAddr[1]>0)) {
						Msdc_FlushWriteCacheBuffer(gU2MsdRwInfoBlock.bNVT_LastOpenedLUN, gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].bSectorBits);
					}

					// Wait the Cache Operation done and Also Clear Cache Content before Close the previous LUN.
					U2Msdc_FlushCache();

					gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].pStrgDevObj->Close();
					gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].MEMBusClosed = FALSE;
				} else {

					// Flush WCB
					if((guiU2MsdcWCSize>0) && (guiU2MsdcWCBufAddr[1]>0)) {
						Msdc_FlushWriteCacheBuffer(gU2MsdRwInfoBlock.bNVT_LastOpenedLUN, gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].bSectorBits);
					}

					// Wait the Cache Operation done and Also Clear Cache Content before Close the previous LUN.
					U2Msdc_FlushCache();

					if (gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].pStrgDevObj->CloseMemBus() == E_OK) {
						gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].MEMBusClosed = TRUE;
					} else {
						gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].MEMBusClosed = FALSE;
					}
				}
			}

			// Open the newly inserted storage
			CheckOK = gU2MsdInfo[CBW_LUN].pStrgDevObj->Open();

			//if (!(((gU2MsdInfo[CBW_LUN].pStrgDevObj->uiStrgStatus & STORAGE_STATUS_MASK) == (STORAGE_CHANGED | STORAGE_READY)) ||
			//	  ((gU2MsdInfo[CBW_LUN].pStrgDevObj->uiStrgStatus & STORAGE_STATUS_MASK) == STORAGE_READY))) {
			//	DBG_ERR("Strg State Err\r\n");
			//	CheckOK = E_SYS;
			//}

			if (CheckOK == E_OK) {
				INT32        Cap=0;

				gU2MsdRwInfoBlock.bNVT_LastOpenedLUN = CBW_LUN;

				result = gU2MsdInfo[CBW_LUN].pStrgDevObj->GetParam(STRG_GET_DEVICE_PHY_SECTORS, (UINT32)&Cap, 0);
				if (result == E_OK) {
					gU2MsdInfo[CBW_LUN].dwSectorSize  = 512;
					gU2MsdInfo[CBW_LUN].bSectorBits   = 9;
					gU2MsdInfo[CBW_LUN].dwMAX_LBA     = Cap - 1;
				}

				if (gU2MsdInfo[CBW_LUN].DetStrgLock_cb != NULL) {
					gU2MsdInfo[CBW_LUN].WriteProtect = gU2MsdInfo[CBW_LUN].DetStrgLock_cb();
				} else {
					gU2MsdInfo[CBW_LUN].WriteProtect = FALSE;
				}

			} else {
				DBG_ERR("LUN %d: Failed to open changed storage\r\n", CBW_LUN);

				gU2MsdRwInfoBlock.bNVT_LastOpenedLUN  = NO_OPENED_LUN;
				gU2MsdInfo[CBW_LUN].StorageEject      = TRUE;
				gU2MsdInfo[CBW_LUN].StorageChanged    = FALSE;
			}
		} else {

			// Flush WCB
			if((guiU2MsdcWCSize>0) && (guiU2MsdcWCBufAddr[1]>0)) {
				Msdc_FlushWriteCacheBuffer(CBW_LUN, gU2MsdInfo[CBW_LUN].bSectorBits);
			}

			// Wait the Cache Operation done and Also Clear Cache Content before Close the previous LUN.
			U2Msdc_FlushCache();

			gU2MsdInfo[CBW_LUN].pStrgDevObj->Close();
		}

	}

	//Storage changed, generate the UNIT ATTENTION condition
	if (gU2MsdInfo[CBW_LUN].StorageChanged == TRUE) {
		Msdc_BuildSenseData(CBW_LUN, UNIT_ATTENTION, 0x28, 0x00); // NOT READY TO READY CHANGE

		if (CDB_cmdid == SPC_CMD_REQUESTSENSE) {
			DBG_IND("LUN %d is ready\r\n", CBW_LUN);

			//Reset storage status
			gU2MsdInfo[CBW_LUN].StorageChanged = FALSE;
		} else if ((CDB_cmdid >= VENDOR_CMD_MIN) && (CDB_cmdid <= VENDOR_CMD_MAX)) {
			;//Back-Door for Customization FW Tool
		} else if (CDB_cmdid != SPC_CMD_INQUIRY) {
			Msdc_FailedCommandHandler();
			return;
		}
	}


	//
	//  SBC commands
	//

	switch (CDB_cmdid) {
	case SBC_CMD_WRITE6: //0x0A
	case SBC_CMD_READ6:  //0x08
	case SBC_CMD_WRITE10://0x2A, Mandatory for SBC command
	case SBC_CMD_READ10: { //0x28, Mandatory for SBC command
			if ((CDB_cmdid == SBC_CMD_READ6) || (CDB_cmdid == SBC_CMD_WRITE6)) {
				gU2MsdRwInfoBlock.dNVT_LBA     =  pTPBulk_CommandBlock->CBW_CDB[2] << 8;
				gU2MsdRwInfoBlock.dNVT_LBA    |=  pTPBulk_CommandBlock->CBW_CDB[3];
				gU2MsdRwInfoBlock.dNVT_LBALEN  =  pTPBulk_CommandBlock->CBW_CDB[4];
			} else {
				gU2MsdRwInfoBlock.dNVT_LBA     =  pTPBulk_CommandBlock->CBW_CDB[2] << 24;
				gU2MsdRwInfoBlock.dNVT_LBA    |=  pTPBulk_CommandBlock->CBW_CDB[3] << 16;
				gU2MsdRwInfoBlock.dNVT_LBA    |=  pTPBulk_CommandBlock->CBW_CDB[4] << 8;
				gU2MsdRwInfoBlock.dNVT_LBA    |=  pTPBulk_CommandBlock->CBW_CDB[5];

				gU2MsdRwInfoBlock.dNVT_LBALEN  =  pTPBulk_CommandBlock->CBW_CDB[7] << 8;
				gU2MsdRwInfoBlock.dNVT_LBALEN |=  pTPBulk_CommandBlock->CBW_CDB[8];
			}


			CDB_DataXferLen = gU2MsdRwInfoBlock.dNVT_LBALEN * gU2MsdInfo[CBW_LUN].dwSectorSize;

			//Case(1) : Hn = Dn
			if ((CBW_DataXferLen == 0) && (CDB_DataXferLen == 0)) {
				Msdc_BuildCSWData(CSW_GOOD, CBW_DataXferLen);
				break;
			}
			// case(2): Hn < Di , case(3): Hn < Do
			else if ((CBW_DataXferLen == 0) && (CDB_DataXferLen > 0)) {
				g_uiU2MSDCState = MSDC_PHASEERROR;
				CheckOK = E_SYS;
			}
			// case(8): Hi <> Do
			else if ((CDB_cmdid == SBC_CMD_WRITE10) && ((pTPBulk_CommandBlock->bCBW_Flag & CBW_FLAG_IN) != 0x00)) {
				g_uiU2MSDCState = MSDC_PHASEERROR;
				CheckOK = E_SYS;
			}
			// case(10): Ho <> Di
			else if ((CDB_cmdid == SBC_CMD_READ10)  && ((pTPBulk_CommandBlock->bCBW_Flag & CBW_FLAG_IN) == 0x00)) {
				g_uiU2MSDCState = MSDC_PHASEERROR;
				CheckOK = E_SYS;
			}
			// case(4): Hi>Dn, case(5): hi>Di, case(11): Ho>Do
			else if (CBW_DataXferLen > CDB_DataXferLen) {
				Msdc_BuildSenseData(CBW_LUN, ILLEGAL_REQUEST, 0x4A, 0x00);  // 4a,00 command phase error
				CheckOK = E_SYS;
			}
			// case(7): Hi<Di, case(13): Ho < Do
			else if (CBW_DataXferLen < CDB_DataXferLen) {
				g_uiU2MSDCState = MSDC_PHASEERROR;
				CheckOK = E_SYS;
			} else if ((gU2MsdInfo[CBW_LUN].dwMAX_LBA + 1) < (gU2MsdRwInfoBlock.dNVT_LBA + gU2MsdRwInfoBlock.dNVT_LBALEN)) {
				Msdc_BuildSenseData(CBW_LUN, ILLEGAL_REQUEST, 0x21, 0x00);  // LOGICAL BLOCK ADDRESS OUT OF RANGE
				CheckOK = E_SYS;
			} else if (gU2MsdInfo[CBW_LUN].StorageEject == TRUE) {
				Msdc_BuildSenseData(CBW_LUN, NOT_READY, 0x3A, 0x00);  // MEDIUM NOT PRESENT
				CheckOK = E_SYS;
			} else {
				Msdc_BuildCSWData(CSW_GOOD, 0);
				CheckOK = E_OK;
			}

			if (CheckOK == E_OK) {
				//Close the last opened LUN
				if ((gU2MsdRwInfoBlock.bNVT_LastOpenedLUN != NO_OPENED_LUN)
					&& ((gU2MsdInfo[CBW_LUN].pStrgDevObj->uiStrgType)
						!= (gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].pStrgDevObj->uiStrgType))) {
					// If the last opened storage is not present,
					// then we CAN'T do the Close/OpenMemBus();
					if (gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].DetStrg_cb != NULL) {
						CheckStrg = gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].DetStrg_cb();
					} else {
						CheckStrg = TRUE;
					}

					if ((CheckStrg == FALSE) || (gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].pStrgDevObj->CloseMemBus == NULL)) {

						// Flush WCB
						if((guiU2MsdcWCSize>0) && (guiU2MsdcWCBufAddr[1]>0)) {
							Msdc_FlushWriteCacheBuffer(gU2MsdRwInfoBlock.bNVT_LastOpenedLUN, gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].bSectorBits);
						}

						// Wait the Cache Operation done and Also Clear Cache Content before Close the previous LUN.
						U2Msdc_FlushCache();

						gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].pStrgDevObj->Close();
						gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].MEMBusClosed = FALSE;
					} else {

						// Flush WCB
						if((guiU2MsdcWCSize>0) && (guiU2MsdcWCBufAddr[1]>0)) {
							Msdc_FlushWriteCacheBuffer(gU2MsdRwInfoBlock.bNVT_LastOpenedLUN, gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].bSectorBits);
						}

						U2Msdc_FlushCache();
						if (gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].pStrgDevObj->CloseMemBus() == E_OK) {
							gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].MEMBusClosed = TRUE;
						} else {
							gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].MEMBusClosed = FALSE;
						}
					}
				}

				// Open the current LUN
				if ((gU2MsdRwInfoBlock.bNVT_LastOpenedLUN == NO_OPENED_LUN)
					|| ((gU2MsdInfo[CBW_LUN].pStrgDevObj->uiStrgType)
						!= (gU2MsdInfo[gU2MsdRwInfoBlock.bNVT_LastOpenedLUN].pStrgDevObj->uiStrgType))) {
					if (gU2MsdInfo[CBW_LUN].MEMBusClosed == TRUE) {
						CheckOK = gU2MsdInfo[CBW_LUN].pStrgDevObj->OpenMemBus();
					} else {
						CheckOK = gU2MsdInfo[CBW_LUN].pStrgDevObj->Open();
					}

					if (CheckOK != E_OK) {
						DBG_ERR("Failed to open LUN %d\r\n", CBW_LUN);
						gU2MsdInfo[CBW_LUN].StorageEject = TRUE;
						gU2MsdRwInfoBlock.bNVT_LastOpenedLUN = NO_OPENED_LUN;
						Msdc_BuildSenseData(CBW_LUN, NOT_READY, 0x3A, 0x00);  // MEDIUM NOT PRESENT
						Msdc_FailedCommandHandler();
						break;
					} else {
						gU2MsdRwInfoBlock.bNVT_LastOpenedLUN = CBW_LUN;
					}
				}


				// normal operations for READ10/WRITE10/READ6/WRITE6
				if ((CDB_cmdid == SBC_CMD_READ10) || (CDB_cmdid == SBC_CMD_READ6)) {
					U2Msdc_SbcREAD();
				} else {
					U2Msdc_SbcWRITE();
				}

			} else {
				Msdc_FailedCommandHandler();
			}
		}
		break;



	/*
	    Commands below normally have a data-In phase from the device to the host.
	*/
	case SBC_CMD_READCAPACITY:          //0x25, Mandatory for SBC command
	case SBC_CMD_READCAPACITY16:        //0x9E, Mandatory for SBC command
	case SPC_CMD_INQUIRY:               //0x12, Mandatory for SPC command
	case SPC_CMD_REQUESTSENSE:          //0x03, Mandatory for SPC command
	case SPC_CMD_MODESENSE6:            //0x1A,  Optional for SPC command
	case SPC_CMD_MODESENSE10:           //0x5A,  Optional for SPC command
	case MMC_CMD_GETEVTSTSNOTIFI:
	case MMC_CMD_READTOCPMAATIP:
	case SPC_CMD_REPORT_SUP_OPCODE: {
			if (CBW_DataXferLen == 0) {
				// case(2) Hn < Di
				Msdc_BuildCSWData(CSW_GOOD, CBW_DataXferLen);
				break;
			}
			// Case(10) Ho <> Di
			else if ((pTPBulk_CommandBlock->bCBW_Flag & CBW_FLAG_IN) == 0x00) {
				g_uiU2MSDCState = MSDC_PHASEERROR;
				result = FALSE;
			} else {
				gU2MsdRwInfoBlock.dNVT_RamAddr = (UINT32)BulkINData;
				gU2MsdRwInfoBlock.dNVT_RamAddr_pa = (UINT32)BulkINData_pa;

				if (CDB_cmdid == SBC_CMD_READCAPACITY) {
					result = U2Msdc_SbcREADCAPACITY();
				} else if (CDB_cmdid == SBC_CMD_READCAPACITY16) {
					result = U2Msdc_SbcREADCAPACITY16();
				} else if (CDB_cmdid == SPC_CMD_INQUIRY) {
					result = U2Msdc_SpcINQUIRY();
				} else if (CDB_cmdid == SPC_CMD_REQUESTSENSE) {
					result = U2Msdc_SpcREQUESTSENSE();
				} else if (CDB_cmdid == SPC_CMD_MODESENSE6) {
					result = U2Msdc_SpcMODESENSE6();
				} else if (CDB_cmdid == SPC_CMD_MODESENSE10) {
					result = U2Msdc_SpcMODESENSE10();
				} else if (CDB_cmdid == SPC_CMD_REPORT_SUP_OPCODE) {
					result = U2Msdc_SpcREPORTSUPPORTEDOPCODE();
				} else if (CDB_cmdid == MMC_CMD_GETEVTSTSNOTIFI) {
					result = U2Msdc_MmcGETEVTSTSNOTIFI();
				} else if (CDB_cmdid == MMC_CMD_READTOCPMAATIP) {
					result = U2Msdc_MmcREADTOCPMAATIP();
				}

			}

			if ((result == TRUE) && ((CDB_cmdid == SPC_CMD_MODESENSE10) || (CDB_cmdid == SPC_CMD_MODESENSE6) || (CDB_cmdid == MMC_CMD_READTOCPMAATIP)) && (gU2MsdInfo[pTPBulk_CommandBlock->bCBW_LUN].MsdcType == MSDC_CDROM)) {
				//
				// For supporting CDROM in MAC Device.
				//

				Msdc_BuildCSWData(CSW_GOOD, 0);

				len = gU2MsdRwInfoBlock.dNVT_DataSize;

				U2Msdc_SetIdle();
				vos_cpu_dcache_sync((VOS_ADDR)gU2MsdRwInfoBlock.dNVT_RamAddr, len, VOS_DMA_BIDIRECTIONAL);
				usb2dev_write_endpoint(gU2MsdcEpIN, (UINT8 *)gU2MsdRwInfoBlock.dNVT_RamAddr_pa, &len);
				vos_cpu_dcache_sync((VOS_ADDR)gU2MsdRwInfoBlock.dNVT_RamAddr, len, VOS_DMA_BIDIRECTIONAL);
				U2Msdc_ClearIdle();
			} else if (result == TRUE) {
				//case(7) Hi < Di  ||  case(6) Hi = Di
				if ((CBW_DataXferLen < gU2MsdRwInfoBlock.dNVT_DataSize) || (CBW_DataXferLen == gU2MsdRwInfoBlock.dNVT_DataSize)) {
					gU2MsdRwInfoBlock.dNVT_DataSize = CBW_DataXferLen;
				}
#if 0
				//case(5) Hi > Di
				else if (CBW_DataXferLen > gMsdRwInfoBlock.dNVT_DataSize) {
					Msdc_BuildCSWData(CSW_GOOD, CBW_DataXferLen - gMsdRwInfoBlock.dNVT_DataSize);
					gMsdRwInfoBlock.dNVT_DataSize = CBW_DataXferLen;  // Fill data to pad up to CBW_DataXferLen
				}
#endif

				if ((gU2MsdInfo[CBW_LUN].StorageEject == TRUE) && (gbU2ReadCapNoStall == TRUE) && (CDB_cmdid == SBC_CMD_READCAPACITY)) {
					Msdc_BuildCSWData(CSW_FAIL, 0);
				} else {
					Msdc_BuildCSWData(CSW_GOOD, 0);
					Msdc_BuildSenseData(pTPBulk_CommandBlock->bCBW_LUN, NO_SENSE, 0x00, 0x00);
				}


				len = gU2MsdRwInfoBlock.dNVT_DataSize;

				U2Msdc_SetIdle();
				vos_cpu_dcache_sync((VOS_ADDR)gU2MsdRwInfoBlock.dNVT_RamAddr, len, VOS_DMA_BIDIRECTIONAL);
				usb2dev_write_endpoint(gU2MsdcEpIN, (UINT8 *)gU2MsdRwInfoBlock.dNVT_RamAddr_pa, &len);
				vos_cpu_dcache_sync((VOS_ADDR)gU2MsdRwInfoBlock.dNVT_RamAddr, len, VOS_DMA_BIDIRECTIONAL);
				U2Msdc_ClearIdle();
			} else {
				Msdc_FailedCommandHandler();
			}
		}
		break;

	/*
	    Commands below normally don't have a data transfer phase.
	*/
	case SPC_CMD_TESTUNITREADY:                 //0x00,
	case SPC_CMD_PRVENTALLOWMEDIUMREMOVAL:      //0x1E,
	case SBC_CMD_STARTSTOPUNIT:                 //0x1b,
	case MMC_SET_CD_SPEED: {                    //0xbb,
			if (CBW_DataXferLen != 0) {
				//case(4) Hi > Dn, case(9) Ho > Dn
				Msdc_BuildSenseData(CBW_LUN, ILLEGAL_REQUEST, 0x4A, 0x00);  // 4a,00 command phase error
				result = FALSE;
			} else {
				if (CDB_cmdid == SBC_CMD_STARTSTOPUNIT) {
					result = U2Msdc_SbcSTARTSTOPUNIT();
				} else if (CDB_cmdid == SPC_CMD_TESTUNITREADY) {
					result = U2Msdc_SpcTESTUNITREADY();
				} else if (CDB_cmdid == SPC_CMD_PRVENTALLOWMEDIUMREMOVAL) {
					result = U2Msdc_SpcPRVENTALLOWMEDIUMREMOVAL();
				} else if (CDB_cmdid == MMC_SET_CD_SPEED) {
					result = U2Msdc_MmcSETCDSPEED();
				}
			}

			if (result == TRUE) {
				// case(1) Hn = Dn
				Msdc_BuildCSWData(CSW_GOOD, 0);
			} else {
				Msdc_FailedCommandHandler();
			}
		}
		break;


	/*
	    Commands below may or may not have a data transfer phase.
	*/
	case SBC_CMD_VERIFY10: {                    //0x2F,
			if (CDB_cmdid == SBC_CMD_VERIFY10) {
				result = U2Msdc_SbcVERIFY10();
			}

			if (result == TRUE) {
				Msdc_BuildCSWData(CSW_GOOD, 0);
			} else {
				Msdc_FailedCommandHandler();
			}
		}
		break;


	/*
	    Command that has Data OUT phase, but we just read it out and then discard it.
	*/
	case SPC_CMD_MODESELECT6:                   //0x15,
	case SBC_CMD_FORMATUNIT: {                  //0x04,
			if (CBW_DataXferLen == 0) {
				// No Data Phase, Return Good.
				Msdc_BuildCSWData(CSW_GOOD, 0);
			} else if ((pTPBulk_CommandBlock->bCBW_Flag & CBW_FLAG_IN) == CBW_FLAG_IN) {
				// Phase Err, This should be OUT.
				g_uiU2MSDCState = MSDC_PHASEERROR;
				Msdc_FailedCommandHandler();
			} else {
				FLGPTN uiFlag = 0;

				usb2dev_unmask_ep_interrupt(gU2MsdcEpOUT);
				U2Msdc_WaitFlag(&uiFlag, FLGMSDC_BULKOUT | FLGMSDC_USBRESET);
				usb2dev_mask_ep_interrupt(gU2MsdcEpOUT);

				if (uiFlag & FLGMSDC_USBRESET) {
					g_uiU2MSDCState = MSDC_USBRESETED;
					break;
				}

				Msdc_BuildCSWData(CSW_GOOD, 0);

				usb2dev_mask_ep_interrupt(gU2MsdcEpOUT);

				// Read Out the Data and then discard it.
				U2Msdc_SetIdle();
				vos_cpu_dcache_sync((VOS_ADDR)(guiU2MsdcBufAddr[guiU2MsdcBufIdx]), CBW_DataXferLen, VOS_DMA_BIDIRECTIONAL);
				if(usb2dev_read_endpoint(gU2MsdcEpOUT, (UINT8 *)(guiU2MsdcBufAddr_pa[guiU2MsdcBufIdx]), (UINT32 *)&CBW_DataXferLen) != E_OK) {
					DBG_WRN("READ ERR\r\n");
				}
				vos_cpu_dcache_sync((VOS_ADDR)(guiU2MsdcBufAddr[guiU2MsdcBufIdx]), CBW_DataXferLen, VOS_DMA_BIDIRECTIONAL);
				U2Msdc_ClearIdle();

				usb2dev_unmask_ep_interrupt(gU2MsdcEpOUT);
			}
		}
		break;


	default : {
			if ((guiU2MsdcCheck_cb != NULL) && (guiU2MsdcVendor_cb != NULL) && (CDB_cmdid >= VENDOR_CMD_MIN)) {
				result = guiU2MsdcCheck_cb(VendorParam.VendorCmdBuf, (UINT32 *) &OutDataBuf);
			} else {
				result = FALSE;
			}

			if (result == TRUE) {
				if (pTPBulk_CommandBlock->bCBW_Flag & CBW_FLAG_IN) {
					//For IN vendor commands
					U2Msdc_SbcVENDORCMD_DIRIN(&VendorParam);
				} else {
					//For OUT vendor commands
					U2Msdc_SbcVENDORCMD_DIROUT(&VendorParam, &OutDataBuf, CBW_DataXferLen);
				}
			} else {
				//For unsupported commands
				Msdc_BuildSenseData(CBW_LUN, ILLEGAL_REQUEST, 0x20, 0x00); // 20,00 Invalid command operation code
				Msdc_FailedCommandHandler();
			}
		}
		break;

	}
}

#if 1
/*
    Valid for SBC READ10 or READ6
*/
void U2Msdc_SbcREAD(void)
{
	UINT32          XferBlock, XferRamAddr, XferRamAddr_pa, NVT_LUN, NVT_LBA;
	ER              CheckOK;
	UINT32          XferDataSize;
	UINT32          CardBuf;
	UINT32          read_bytes;
	UINT32          data_cycle;
#if MSDC_CACHEREAD_HANDLE
	MSDC_CACHE_INFO CacheInfo;
#endif

	g_uiU2MSDCState = MSDC_READ_BUSY_STATE;

	NVT_LUN = pTPBulk_CommandBlock->bCBW_LUN;
	NVT_LBA = gU2MsdRwInfoBlock.dNVT_LBA;

	XferRamAddr    = (UINT32) guiU2MsdcBufAddr[guiU2MsdcBufIdx];
	XferRamAddr_pa = (UINT32) guiU2MsdcBufAddr_pa[guiU2MsdcBufIdx];

	// We must ping-pong the Read Buffer,
	// Because if the read cache is delayed during the multi-tasking,
	// The read buffer (layer-2) may conflict with the SbcWRITE's starting USB write to DRAM buffer.
	guiU2MsdcBufIdx = !guiU2MsdcBufIdx;

	XferDataSize = gU2MsdRwInfoBlock.dNVT_LBALEN << gU2MsdInfo[NVT_LUN].bSectorBits;

	if(((gU2MsdRwInfoBlock.dNVT_LBALEN>>1) > guiU2MsdcReadSlice) && (guiU2MsdcDataBufSize >= XferDataSize)) {
		INT32 i;

		guiU2MsdcReadSlice = gU2MsdRwInfoBlock.dNVT_LBALEN>>1;

		for(i=15;i>=0;i--) {
			if(guiU2MsdcReadSlice & (0x1<<i)) {
				guiU2MsdcReadSlice = 0x1<<i;
				break;
			}
		}
		//DBG_DUMP("guiU2MsdcReadSlice = %d\r\n",guiMsdcReadSlice);
	}

	msdc_debug(("U2Msdc_SbcREAD: Lun(%d) BlkSecSize(%d)\r\n", (int)NVT_LUN, (int)gU2MsdRwInfoBlock.dNVT_LBALEN));

	data_cycle = guiU2MsdcReadSlice * 512;

	if (XferDataSize > data_cycle) {
		read_bytes = data_cycle;
	} else {
		read_bytes = XferDataSize;
	}

	XferBlock = (read_bytes >> gU2MsdInfo[NVT_LUN].bSectorBits);

	//read the first block
#if !MSDC_CACHEREAD_HANDLE
	CheckOK = gU2MsdInfo[NVT_LUN].pStrgDevObj->RdSectors((INT8 *)XferRamAddr, NVT_LBA, XferBlock);
#else
	CacheInfo.Dir       = MSDC_CACHE_READ;
	CacheInfo.uiLUN     = NVT_LUN;
	CacheInfo.uiAddr    = XferRamAddr;
	CacheInfo.uiLBA     = NVT_LBA;
	CacheInfo.uiSecNum  = XferBlock;

	CheckOK = U2Msdc_TriggerCache(&CacheInfo);
#endif


	if (guiU2MsdcRWLed_cb != NULL) {
		guiU2MsdcRWLed_cb();
	}


	NVT_LBA += XferBlock;
	CardBuf = XferRamAddr;
	//XferRamAddr +=  read_bytes;

	if (CheckOK == E_OK) {
		XferDataSize -=  read_bytes;
	}

	while ((XferDataSize > 0) && (CheckOK == E_OK)) {
#if MSDC_CACHEREAD_HANDLE
		U2Msdc_GetCacheReadData(&CacheInfo);
		if (CacheInfo.Dir == MSDC_CACHE_READ_HIT) {
			CardBuf = CacheInfo.uiAddr;
		}
#endif

		// Start USB RW
		U2Msdc_SetIdle();
		vos_cpu_dcache_sync((VOS_ADDR)(CardBuf), read_bytes, VOS_DMA_TO_DEVICE);
		CheckOK = usb2dev_write_endpoint(gU2MsdcEpIN, (UINT8 *)XferRamAddr_pa, &read_bytes);
		//vos_cpu_dcache_sync((VOS_ADDR)(CardBuf), read_bytes, VOS_DMA_BIDIRECTIONAL);

		if (CheckOK != E_OK) {
			U2Msdc_ClearIdle();
			DBG_ERR("READ10 error phase 2\r\n");
			break;
		}
		U2Msdc_ClearIdle();



		if (XferDataSize < data_cycle) {
			read_bytes = XferDataSize;
			XferBlock = (read_bytes >> gU2MsdInfo[NVT_LUN].bSectorBits);
		}

		//Read the next block
#if !MSDC_CACHEREAD_HANDLE
		CheckOK = gU2MsdInfo[NVT_LUN].pStrgDevObj->RdSectors((INT8 *)XferRamAddr, NVT_LBA,  XferBlock);
#else
		CacheInfo.Dir       = MSDC_CACHE_READ;
		CacheInfo.uiLUN     = NVT_LUN;
		CacheInfo.uiAddr    = XferRamAddr;
		CacheInfo.uiLBA     = NVT_LBA;
		CacheInfo.uiSecNum  = XferBlock;

		CheckOK = U2Msdc_TriggerCache(&CacheInfo);
#endif


		if (guiU2MsdcRWLed_cb != NULL) {
			guiU2MsdcRWLed_cb();
		}

		NVT_LBA += XferBlock;
		CardBuf = XferRamAddr;
		//XferRamAddr +=  read_bytes;

		if (CheckOK == E_OK) {
			XferDataSize -=  read_bytes;
		}


	}


	if (CheckOK != E_OK) {
		Msdc_BuildCSWData(CSW_FAIL, XferDataSize);
		Msdc_BuildSenseData(NVT_LUN, MEDIUM_ERROR, 0x11, 0x00);  // UNRECOVERED_READ_ERROR
		Msdc_EndpointStallHandler(gU2MsdcEpIN);
		DBG_ERR("READ10 storage error\r\n");
		return;
	}

#if MSDC_CACHEREAD_HANDLE
	U2Msdc_GetCacheReadData(&CacheInfo);
	if (CacheInfo.Dir == MSDC_CACHE_READ_HIT) {
		CardBuf = CacheInfo.uiAddr;
	}
#endif

	U2Msdc_SetIdle();

	vos_cpu_dcache_sync((VOS_ADDR)(CardBuf), read_bytes, VOS_DMA_TO_DEVICE);
	CheckOK = usb2dev_write_endpoint(gU2MsdcEpIN, (UINT8 *)XferRamAddr_pa, &read_bytes);
	//vos_cpu_dcache_sync((VOS_ADDR)(CardBuf), read_bytes, VOS_DMA_BIDIRECTIONAL);

	if (CheckOK != E_OK) {
		DBG_ERR("READ10 error phase 3\r\n");
	}
	U2Msdc_ClearIdle();

	g_uiU2MSDCState = MSDC_COMMANDOK;

}

/*
    Valid for SBC WRITE10 or WRITE6
*/
void U2Msdc_SbcWRITE(void)
{
	FLGPTN  uiFlag = 0;

	UINT32  RamAddrFromUSB,RamAddrFromUSB_pa, BytesReadFromUSB;
	UINT32  SectorNumToCard, RamAddrToCard;
	UINT32  XferDataSize, data_cycle;
	UINT8  	NVT_LUN;
	UINT32 	NVT_LBA;
	ER     	Result, CheckUSBDMAOK;
	static	UINT32 WriteSlice = 256;

	g_uiU2MSDCState = MSDC_WRITE_BUSY_STATE;

	// Flush Read Cache
	U2Msdc_FlushReadCache();

	U2Msdc_ClearFlag(FLGMSDC_BULKOUT | FLGMSDC_USBRESET);

	NVT_LUN = pTPBulk_CommandBlock->bCBW_LUN;

	XferDataSize = gU2MsdRwInfoBlock.dNVT_LBALEN << gU2MsdInfo[NVT_LUN].bSectorBits ;

	msdc_debug(("U2Msdc_SbcWRITE: Lun(%d) BlkSecSize(%d)\r\n", (int)NVT_LUN, (int)gU2MsdRwInfoBlock.dNVT_LBALEN));

	if((gU2MsdRwInfoBlock.dNVT_LBALEN > WriteSlice) && (guiU2MsdcDataBufSize >= XferDataSize)) {
		INT32 i;

		WriteSlice = gU2MsdRwInfoBlock.dNVT_LBALEN;

		for(i=15;i>=0;i--) {
			if(WriteSlice & (0x1<<i)) {
				WriteSlice = 0x1<<i;
				break;
			}
		}
		//DBG_DUMP("WriteSlice = %d\r\n",WriteSlice);
	}

	if((guiU2MsdcWCSize>0) && (guiU2MsdcWCBufAddr[1]>0)) {
		WriteSlice = guiU2MsdcWCSlice;
	}

	data_cycle = WriteSlice * 512;

	// Update information for the next USB-DMA transfer
	RamAddrFromUSB  = (UINT32)guiU2MsdcBufAddr[guiU2MsdcBufIdx];
	RamAddrFromUSB_pa  = (UINT32)guiU2MsdcBufAddr_pa[guiU2MsdcBufIdx];
	guiU2MsdcBufIdx = !guiU2MsdcBufIdx;


	if (XferDataSize > data_cycle) {
		BytesReadFromUSB = data_cycle;
	} else {
		BytesReadFromUSB = XferDataSize;
	}


	// Update information  for the next WrSectors function
	NVT_LBA         = gU2MsdRwInfoBlock.dNVT_LBA;
	SectorNumToCard = BytesReadFromUSB >> gU2MsdInfo[NVT_LUN].bSectorBits ;
	RamAddrToCard   = RamAddrFromUSB;

	usb2dev_unmask_ep_interrupt(gU2MsdcEpOUT);
	U2Msdc_WaitFlag(&uiFlag, FLGMSDC_BULKOUT | FLGMSDC_USBRESET);
	usb2dev_mask_ep_interrupt(gU2MsdcEpOUT);

	if (uiFlag & FLGMSDC_USBRESET) {
		g_uiU2MSDCState = MSDC_USBRESETED;
		return;
	}

	usb2dev_mask_ep_interrupt(gU2MsdcEpOUT);

	U2Msdc_SetIdle();
	vos_cpu_dcache_sync((VOS_ADDR)(RamAddrFromUSB), BytesReadFromUSB, VOS_DMA_TO_DEVICE);
	CheckUSBDMAOK = usb2dev_read_endpoint(gU2MsdcEpOUT, (UINT8 *) RamAddrFromUSB_pa, &BytesReadFromUSB);
	vos_cpu_dcache_sync((VOS_ADDR)(RamAddrFromUSB), BytesReadFromUSB, VOS_DMA_FROM_DEVICE);
	if (CheckUSBDMAOK != E_OK) {
		DBG_ERR("WRITE10 error phase 1\r\n");
		CheckUSBDMAOK = E_SYS;
	}
	U2Msdc_ClearIdle();

	XferDataSize -= BytesReadFromUSB;

	while (XferDataSize > 0) {
		if (gU2MsdInfo[NVT_LUN].WriteProtect == TRUE) {
			Msdc_BuildCSWData(CSW_FAIL, 0);
			Msdc_BuildSenseData(NVT_LUN, DATA_PROTECT, 0x27, 0x00);  //WRITE PROTECTED
			DBG_ERR("STRG write protected\r\n");
		} else if (CheckUSBDMAOK != E_OK) {
			Msdc_BuildCSWData(CSW_FAIL, 0);
			Msdc_BuildSenseData(NVT_LUN, MEDIUM_ERROR, 0x03, 0x00);  //PERIPHERAL_DEVICE_WRITE_FAULT
		} else {
			//Write the previous block of data to the storage.
#if !MSDC_CACHEWRITE_HANDLE
			Result = gU2MsdInfo[NVT_LUN].pStrgDevObj->WrSectors((INT8 *)RamAddrToCard, NVT_LBA, SectorNumToCard);
#else
		if((guiU2MsdcWCSize>0) && (guiU2MsdcWCBufAddr[1]>0)) {
			/*
				WCB Enabled
			*/
			if(BytesReadFromUSB == data_cycle) {
				if((guiU2MsdcWCCurSz >= BytesReadFromUSB) &&
					((guiU2MsdcWCNextLBA == NVT_LBA)||(guiU2MsdcWCNextLBA == 0xFFFFFFFF))) {

					dma_flushReadCache(dma_getCacheAddr(RamAddrToCard), BytesReadFromUSB);
					memcpy((void *)(guiU2MsdcWCBufAddr[guiU2MsdcWCBufIdx]+(guiU2MsdcWCSize-guiU2MsdcWCCurSz)), (void *)dma_getCacheAddr(RamAddrToCard), BytesReadFromUSB);

					guiU2MsdcWCCurSz  -= BytesReadFromUSB;
					guiU2MsdcWCNextLBA = NVT_LBA + SectorNumToCard;
					Result = E_OK;
				} else {
					Result = Msdc_FlushWriteCacheBuffer(NVT_LUN, gU2MsdInfo[NVT_LUN].bSectorBits);
					if (Result != E_OK) {
						DBG_ERR("FlushWCB Err\r\n");
					}
					dma_flushReadCache(dma_getCacheAddr(RamAddrToCard), BytesReadFromUSB);
					memcpy((void *)(guiU2MsdcWCBufAddr[guiU2MsdcWCBufIdx]+(guiU2MsdcWCSize-guiU2MsdcWCCurSz)), (void *)dma_getCacheAddr(RamAddrToCard), BytesReadFromUSB);

					guiU2MsdcWCCurSz  -= BytesReadFromUSB;
					guiU2MsdcWCNextLBA = NVT_LBA + SectorNumToCard;
					Result = E_OK;

				}

				if(guiU2MsdcWCCurSz == 0) {
					Result = Msdc_FlushWriteCacheBuffer(NVT_LUN, gU2MsdInfo[NVT_LUN].bSectorBits);
				}

			} else {
				// Check if something inside WCB, if yes, flush it.
				Result = Msdc_FlushWriteCacheBuffer(NVT_LUN, gU2MsdInfo[NVT_LUN].bSectorBits);
				if (Result != E_OK) {
					DBG_ERR("FlushWCB Err\r\n");
				}

				// Write down the smaller size
				{
					MSDC_CACHE_INFO CacheInfo;

					CacheInfo.Dir       = MSDC_CACHE_WRITE;
					CacheInfo.uiLUN     = NVT_LUN;
					CacheInfo.uiAddr    = RamAddrToCard;
					CacheInfo.uiLBA     = NVT_LBA;
					CacheInfo.uiSecNum  = SectorNumToCard;

					Result = U2Msdc_TriggerCache(&CacheInfo);
				}

			}
		} else {
			MSDC_CACHE_INFO CacheInfo;

			CacheInfo.Dir       = MSDC_CACHE_WRITE;
			CacheInfo.uiLUN     = NVT_LUN;
			CacheInfo.uiAddr    = RamAddrToCard;
			CacheInfo.uiLBA     = NVT_LBA;
			CacheInfo.uiSecNum  = SectorNumToCard;

			Result = U2Msdc_TriggerCache(&CacheInfo);
		}
#endif

			if (Result != E_OK) {
				Msdc_BuildCSWData(CSW_FAIL, 0);
				Msdc_BuildSenseData(NVT_LUN, MEDIUM_ERROR, 0x03, 0x00);  //PERIPHERAL_DEVICE_WRITE_FAULT
				DBG_ERR("WRITE10 storage error 2\r\n");
			}
		}



		// Update information for the next USB-DMA transfer
		RamAddrFromUSB     = (UINT32)guiU2MsdcBufAddr[guiU2MsdcBufIdx];
		RamAddrFromUSB_pa  = (UINT32)guiU2MsdcBufAddr_pa[guiU2MsdcBufIdx];
		guiU2MsdcBufIdx = !guiU2MsdcBufIdx;

		if (XferDataSize > data_cycle) {
			BytesReadFromUSB = data_cycle;
		} else {
			BytesReadFromUSB = XferDataSize;
		}

		U2Msdc_SetIdle();
		vos_cpu_dcache_sync((VOS_ADDR)(RamAddrFromUSB), BytesReadFromUSB, VOS_DMA_TO_DEVICE);
		CheckUSBDMAOK = usb2dev_read_endpoint(gU2MsdcEpOUT, (UINT8 *) RamAddrFromUSB_pa, &BytesReadFromUSB);
		vos_cpu_dcache_sync((VOS_ADDR)(RamAddrFromUSB), BytesReadFromUSB, VOS_DMA_FROM_DEVICE);
		if (CheckUSBDMAOK != E_OK) {
			DBG_ERR("WRITE10 error phase 2\r\n");
			CheckUSBDMAOK = E_SYS;
		}
		U2Msdc_ClearIdle();


		// Update information  for the next WrSectors function
		NVT_LBA         += SectorNumToCard;
		SectorNumToCard = BytesReadFromUSB >> gU2MsdInfo[NVT_LUN].bSectorBits ;
		RamAddrToCard   = RamAddrFromUSB;

		XferDataSize -= BytesReadFromUSB;
	}


	if (gU2MsdInfo[NVT_LUN].WriteProtect == TRUE) {
		Msdc_BuildCSWData(CSW_FAIL, 0);
		Msdc_BuildSenseData(NVT_LUN, DATA_PROTECT, 0x27, 0x00);  //WRITE PROTECTED
		DBG_ERR("STRG write protected\r\n");
	} else if (CheckUSBDMAOK != E_OK) {
		Msdc_BuildCSWData(CSW_FAIL, 0);
		Msdc_BuildSenseData(NVT_LUN, MEDIUM_ERROR, 0x03, 0x00);  //PERIPHERAL_DEVICE_WRITE_FAULT
	} else {
		//write the last block of data to the storage.
#if !MSDC_CACHEWRITE_HANDLE
		Result = gU2MsdInfo[NVT_LUN].pStrgDevObj->WrSectors((INT8 *)RamAddrToCard, NVT_LBA, SectorNumToCard);
#else
		if((guiU2MsdcWCSize>0) && (guiU2MsdcWCBufAddr[1]>0)) {
			/*
				WCB Enabled
			*/
			if(BytesReadFromUSB == data_cycle) {
				if((guiU2MsdcWCCurSz >= BytesReadFromUSB) &&
					((guiU2MsdcWCNextLBA == NVT_LBA)||(guiU2MsdcWCNextLBA == 0xFFFFFFFF))) {

					dma_flushReadCache(dma_getCacheAddr(RamAddrToCard), BytesReadFromUSB);
					memcpy((void *)(guiU2MsdcWCBufAddr[guiU2MsdcWCBufIdx]+(guiU2MsdcWCSize-guiU2MsdcWCCurSz)), (void *)dma_getCacheAddr(RamAddrToCard), BytesReadFromUSB);

					guiU2MsdcWCCurSz  -= BytesReadFromUSB;
					guiU2MsdcWCNextLBA = NVT_LBA + SectorNumToCard;
					Result = E_OK;
				} else {
					Result = Msdc_FlushWriteCacheBuffer(NVT_LUN, gU2MsdInfo[NVT_LUN].bSectorBits);
					if (Result != E_OK) {
						DBG_ERR("FlushWCB Err\r\n");
					}

					dma_flushReadCache(dma_getCacheAddr(RamAddrToCard), BytesReadFromUSB);
					memcpy((void *)(guiU2MsdcWCBufAddr[guiU2MsdcWCBufIdx]+(guiU2MsdcWCSize-guiU2MsdcWCCurSz)), (void *)dma_getCacheAddr(RamAddrToCard), BytesReadFromUSB);

					guiU2MsdcWCCurSz  -= BytesReadFromUSB;
					guiU2MsdcWCNextLBA = NVT_LBA + SectorNumToCard;
					Result = E_OK;

				}

				if(guiU2MsdcWCCurSz == 0) {
					Result = Msdc_FlushWriteCacheBuffer(NVT_LUN, gU2MsdInfo[NVT_LUN].bSectorBits);
				}

			} else {
				// Check if something inside WCB, if yes, flush it.
				Result = Msdc_FlushWriteCacheBuffer(NVT_LUN, gU2MsdInfo[NVT_LUN].bSectorBits);
				if (Result != E_OK) {
					DBG_ERR("FlushWCB Err\r\n");
				}

				// Write down the smaller size
				{
					MSDC_CACHE_INFO CacheInfo;

					CacheInfo.Dir       = MSDC_CACHE_WRITE;
					CacheInfo.uiLUN     = NVT_LUN;
					CacheInfo.uiAddr    = RamAddrToCard;
					CacheInfo.uiLBA     = NVT_LBA;
					CacheInfo.uiSecNum  = SectorNumToCard;

					Result = U2Msdc_TriggerCache(&CacheInfo);
				}

			}
		} else {
			MSDC_CACHE_INFO CacheInfo;

			CacheInfo.Dir       = MSDC_CACHE_WRITE;
			CacheInfo.uiLUN     = NVT_LUN;
			CacheInfo.uiAddr    = RamAddrToCard;
			CacheInfo.uiLBA     = NVT_LBA;
			CacheInfo.uiSecNum  = SectorNumToCard;

			Result = U2Msdc_TriggerCache(&CacheInfo);
		}
#endif

		if (Result != E_OK) {
			Msdc_BuildCSWData(CSW_FAIL, 0);
			Msdc_BuildSenseData(NVT_LUN, MEDIUM_ERROR, 0x03, 0x00);  //PERIPHERAL_DEVICE_WRITE_FAULT
			DBG_ERR("WRITE10 storage error 1\r\n");
		}
	}

	if (guiU2MsdcRWLed_cb != NULL) {
		guiU2MsdcRWLed_cb();
	}

	usb2dev_unmask_ep_interrupt(gU2MsdcEpOUT);

	g_uiU2MSDCState = MSDC_COMMANDOK;

}

BOOL U2Msdc_SbcREADCAPACITY(void)
{
	UINT32 NVT_LUN;
	UINT32 LBA;

	msdc_debug(("U2Msdc_SbcREADCAPACITY\r\n"));

	NVT_LUN = pTPBulk_CommandBlock->bCBW_LUN;

	LBA =  pTPBulk_CommandBlock->CBW_CDB[2] << 24;
	LBA |= pTPBulk_CommandBlock->CBW_CDB[3] << 16;
	LBA |= pTPBulk_CommandBlock->CBW_CDB[4] << 8;
	LBA |= pTPBulk_CommandBlock->CBW_CDB[5];

	if ((LBA != 0x00) &&
		((pTPBulk_CommandBlock->CBW_CDB[1] & 0x01) != 0x00) &&    //RelAdr must be zero
		((pTPBulk_CommandBlock->CBW_CDB[8] & 0x01) != 0x00)) {    //PMI must be zero
		Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
		return FALSE;
	} else if ((LBA != 0x00) && ((pTPBulk_CommandBlock->CBW_CDB[8] & 0x01) == 0x00)) {
		Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
		return FALSE;
	} else if (gU2MsdInfo[NVT_LUN].StorageEject == TRUE) {
		Msdc_BuildSenseData(NVT_LUN, NOT_READY, 0x3A, 0x00); // MEDIUM NOT PRESENT

		if (gbU2ReadCapNoStall == FALSE) {
			return FALSE;
		}
	}

	BulkINData[0] = SWAP4(gU2MsdInfo[NVT_LUN].dwMAX_LBA);
	BulkINData[1] = (UINT32)SWAP2(gU2MsdInfo[NVT_LUN].dwSectorSize);

	gU2MsdRwInfoBlock.dNVT_DataSize = 8;
	return TRUE;
}

BOOL U2Msdc_SbcREADCAPACITY16(void)
{
	UINT32 NVT_LUN, allocate;
	UINT64 LBA;

	msdc_debug(("U2Msdc_SbcREADCAPACITY16\r\n"));

	NVT_LUN = pTPBulk_CommandBlock->bCBW_LUN;

	LBA = (UINT64)pTPBulk_CommandBlock->CBW_CDB[2] << 56;
	LBA |= (UINT64)pTPBulk_CommandBlock->CBW_CDB[3] << 48;
	LBA |= (UINT64)pTPBulk_CommandBlock->CBW_CDB[4] << 40;
	LBA |= (UINT64)pTPBulk_CommandBlock->CBW_CDB[5] << 32;
	LBA |= (UINT64)pTPBulk_CommandBlock->CBW_CDB[6] << 24;
	LBA |= (UINT64)pTPBulk_CommandBlock->CBW_CDB[7] << 16;
	LBA |= (UINT64)pTPBulk_CommandBlock->CBW_CDB[8] << 8;
	LBA |= (UINT64)pTPBulk_CommandBlock->CBW_CDB[9];

	allocate  = pTPBulk_CommandBlock->CBW_CDB[10] << 24;
	allocate |= pTPBulk_CommandBlock->CBW_CDB[11] << 16;
	allocate |= pTPBulk_CommandBlock->CBW_CDB[12] << 8;
	allocate |= pTPBulk_CommandBlock->CBW_CDB[13];


	if ((LBA != 0x00) &&
		((pTPBulk_CommandBlock->CBW_CDB[14] & 0x01) != 0x00)) {    //PMI must be zero
		Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
		return FALSE;
	} else if ((LBA != 0x00) && ((pTPBulk_CommandBlock->CBW_CDB[14] & 0x01) == 0x00)) {
		Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
		return FALSE;
	} else if (gU2MsdInfo[NVT_LUN].StorageEject == TRUE) {
		Msdc_BuildSenseData(NVT_LUN, NOT_READY, 0x3A, 0x00); // MEDIUM NOT PRESENT
		return FALSE;
	}

	BulkINData[0] = 0;
	BulkINData[1] = SWAP4(gU2MsdInfo[NVT_LUN].dwMAX_LBA);
	BulkINData[2] = (UINT32)SWAP2(gU2MsdInfo[NVT_LUN].dwSectorSize);
	BulkINData[3] = 0;
	BulkINData[4] = 0;
	BulkINData[5] = 0;
	BulkINData[6] = 0;
	BulkINData[7] = 0;

	gU2MsdRwInfoBlock.dNVT_DataSize = 32;

	if (gU2MsdRwInfoBlock.dNVT_DataSize > allocate) {
		gU2MsdRwInfoBlock.dNVT_DataSize = allocate;
	}

	return TRUE;
}

BOOL U2Msdc_SbcSTARTSTOPUNIT(void)
{
	UINT32 NVT_LUN;

	msdc_debug(("U2Msdc_SbcSTARTSTOPUNIT\r\n"));

	NVT_LUN = pTPBulk_CommandBlock->bCBW_LUN;

	if ((pTPBulk_CommandBlock->CBW_CDB[5] != 0x00)) {           // not supported field
		Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
		return FALSE;
	}

	if ((pTPBulk_CommandBlock->CBW_CDB[4] & 0x03) == 0x02) {    //LoEj=1  START=0

		gU2MsdInfo[NVT_LUN].StorageEject = TRUE;

		Msdc_BuildSenseData(NVT_LUN, NOT_READY, 0x3A, 0x00);

		if (guiU2MsdcStopUnitLed_cb != NULL) {
			guiU2MsdcStopUnitLed_cb();
		}
	} else if ((pTPBulk_CommandBlock->CBW_CDB[4] & 0x03) == 0x03) { //LoEj=1  START=1
		gU2MsdInfo[NVT_LUN].StorageEject = FALSE;

		Msdc_BuildSenseData(NVT_LUN, NO_SENSE, 0x00, 0x00);
	}

	return TRUE;
}

BOOL U2Msdc_SbcVERIFY10(void)
{
	UINT32 NVT_LUN;
	UINT32 LBA, LENGTH;

	msdc_debug(("U2Msdc_SbcVERIFY10\r\n"));

	gU2MsdRwInfoBlock.dNVT_LBA =  pTPBulk_CommandBlock->CBW_CDB[2] << 24;
	gU2MsdRwInfoBlock.dNVT_LBA |= pTPBulk_CommandBlock->CBW_CDB[3] << 16;
	gU2MsdRwInfoBlock.dNVT_LBA |= pTPBulk_CommandBlock->CBW_CDB[4] << 8;
	gU2MsdRwInfoBlock.dNVT_LBA |= pTPBulk_CommandBlock->CBW_CDB[5];

	gU2MsdRwInfoBlock.dNVT_LBALEN =  pTPBulk_CommandBlock->CBW_CDB[7] << 8;
	gU2MsdRwInfoBlock.dNVT_LBALEN |= pTPBulk_CommandBlock->CBW_CDB[8];

	NVT_LUN = pTPBulk_CommandBlock->bCBW_LUN;
	LBA     = gU2MsdRwInfoBlock.dNVT_LBA;
	LENGTH  = gU2MsdRwInfoBlock.dNVT_LBALEN;

	if (pTPBulk_CommandBlock->CBW_CDB[1] & 0x2) {
		//BYTCHK = 1 means that the host requests a byte-to-byte comparison,
		//but we don't support this function yet.
		Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
		return FALSE;
	} else if (pTPBulk_CommandBlock->CBW_CDB[9] != 0x00) {
		Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
		return FALSE;
	} else if ((gU2MsdInfo[NVT_LUN].dwMAX_LBA + 1) < (LBA + LENGTH)) {
		Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x21, 0x00);  // LOGICAL BLOCK ADDRESS OUT OF RANGE
		return FALSE;
	} else if (gU2MsdInfo[NVT_LUN].StorageEject == TRUE) {
		Msdc_BuildSenseData(NVT_LUN, NOT_READY, 0x3A, 0x00); // MEDIUM NOT PRESENT
		return FALSE;
	}

	//Actually, when BYTCHK = 0, we have to test the specified sectors,
	//here we just report OK without checking anything.
	return TRUE;

}

/*
    Processes Vendor Commands which direction is OUT
*/
BOOL U2Msdc_SbcVENDORCMD_DIROUT(PMSDCVendorParam pVendorParam, UINT32 *pDataOutBuf, UINT32 CBW_DataXferLen)
{
	UINT32 XferRamAddr;
	FLGPTN uiFlag = 0;

	msdc_debug(("U2Msdc_SbcVENDORCMD_DIROUT\r\n"));

	pVendorParam->OutDataBufLen = CBW_DataXferLen;
	XferRamAddr  = *pDataOutBuf;

	usb2dev_unmask_ep_interrupt(gU2MsdcEpOUT);
	U2Msdc_WaitFlag(&uiFlag, FLGMSDC_BULKOUT | FLGMSDC_USBRESET);
	usb2dev_mask_ep_interrupt(gU2MsdcEpOUT);

	if (uiFlag & FLGMSDC_USBRESET) {
		g_uiU2MSDCState = MSDC_USBRESETED;
		return FALSE;
	}

	usb2dev_mask_ep_interrupt(gU2MsdcEpOUT);

	U2Msdc_SetIdle();
	if(usb2dev_read_endpoint(gU2MsdcEpOUT, (UINT8 *)XferRamAddr, (UINT32 *) &CBW_DataXferLen) != E_OK) {
		DBG_WRN("READ ERR\r\n");
	}
	U2Msdc_ClearIdle();

	// call CB function
	guiU2MsdcVendor_cb((PMSDCVendorParam)pVendorParam);

	usb2dev_unmask_ep_interrupt(gU2MsdcEpOUT);
	return TRUE;
}

/*
    Processes Vendor Commands which direction is IN
*/
BOOL U2Msdc_SbcVENDORCMD_DIRIN(PMSDCVendorParam pVendorParam)
{
	UINT32 XferRamAddr;
	UINT32 CBW_DataXferLen;

	msdc_debug(("U2Msdc_SbcVENDORCMD_DIRIN\r\n"));

	if (guiU2MsdcVendorInBufAddr) {
		XferRamAddr  = guiU2MsdcVendorInBufAddr;
		pVendorParam->InDataBuf = guiU2MsdcVendorInBufAddr;
	} else {
		XferRamAddr  = guiU2MsdcBufAddr[guiU2MsdcBufIdx];
		pVendorParam->InDataBuf = guiU2MsdcBufAddr[guiU2MsdcBufIdx];
	}


	// call CB function
	guiU2MsdcVendor_cb(pVendorParam);

	CBW_DataXferLen = pVendorParam->InDataBufLen;

	U2Msdc_SetIdle();
	usb2dev_write_endpoint(gU2MsdcEpIN, (UINT8 *)XferRamAddr, (UINT32 *) &CBW_DataXferLen);
	U2Msdc_ClearIdle();


	return TRUE;
}

#if 1
BOOL U2Msdc_SpcMODESENSE6(void)
{
	UINT32 NVT_LUN;

	msdc_debug(("U2Msdc_SpcMODESENSE6\r\n"));

	NVT_LUN = pTPBulk_CommandBlock->bCBW_LUN;

	if ((pTPBulk_CommandBlock->CBW_CDB[2] & 0xC0) == 0xC0) {    //Save value is not supported
		Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x39, 0x00);
		return FALSE;
	} else if ((pTPBulk_CommandBlock->CBW_CDB[5] != 0x00) || (pTPBulk_CommandBlock->CBW_CDB[3] != 0x00)) { //field non-supported
		Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
		return FALSE;
	}

	// Build Block descriptor
	BulkINData[1] = SWAP4(gU2MsdInfo[NVT_LUN].dwMAX_LBA + 1);
	BulkINData[2] = (UINT32)SWAP2(gU2MsdInfo[NVT_LUN].dwSectorSize);

	if ((pTPBulk_CommandBlock->CBW_CDB[2] & 0xC0) == 0x40) {
		/* Changeable values */

		switch (pTPBulk_CommandBlock->CBW_CDB[2] & 0x3F) {
		case 0x3F: {
				// Return all mode pages
				gU2MsdRwInfoBlock.dNVT_DataSize = 4 + 8 + 20 + 12;

				BulkINData[0] =  0x08000000 ;// Header
				BulkINData[0] += (gU2MsdRwInfoBlock.dNVT_DataSize - 1);

				// Caching Page
				BulkINData[3] = 0x00001208 ;//Page Code 0x08; PageLength 0x12
				BulkINData[4] = 0x00000000 ;
				BulkINData[5] = 0x00000000 ;
				BulkINData[6] = 0x00000000 ;
				BulkINData[7] = 0x00000000 ;

				// Informational Exception Page
				BulkINData[8] = 0x00000A1C ;
				BulkINData[9] = 0x00000000 ;
				BulkINData[10] = 0x00000000 ;
			}
			break;

		case 0x08: {
				gU2MsdRwInfoBlock.dNVT_DataSize = 4 + 8 + 20;

				BulkINData[0] =  0x08000000 ;// Header
				BulkINData[0] += (gU2MsdRwInfoBlock.dNVT_DataSize - 1);

				// Caching Page
				BulkINData[3] = 0x00001208 ;//Page Code 0x08; PageLength 0x12
				BulkINData[4] = 0x00000000 ;
				BulkINData[5] = 0x00000000 ;
				BulkINData[6] = 0x00000000 ;
				BulkINData[7] = 0x00000000 ;
			}
			break;

		case 0x1C: {
				gU2MsdRwInfoBlock.dNVT_DataSize = 4 + 8 + 12;

				BulkINData[0] =  0x08000000 ;// Header
				BulkINData[0] += (gU2MsdRwInfoBlock.dNVT_DataSize - 1);

				// Informational exceptions control Mode Page
				BulkINData[3] = 0x00000A1C ;
				BulkINData[4] = 0x00000000 ;
				BulkINData[5] = 0x00000000 ;
			}
			break;

		case 0x1A:  //  For supporting CDROM in MAC Device.
		case 0x2A:  //  For supporting CDROM in MAC Device.
		case 0x31: { //  For supporting CDROM in MAC Device.

				if (gU2MsdInfo[NVT_LUN].MsdcType == MSDC_CDROM) {
					gU2MsdRwInfoBlock.dNVT_DataSize = 0x4;

					BulkINData[0] = 0x00800000;// Header
					BulkINData[0] += (gU2MsdRwInfoBlock.dNVT_DataSize - 1);
				} else {
					Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
					return FALSE;
				}

			}
			break;

		default:
			Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
			return FALSE;
			break;
		}
	} else {
		/* Default & Current   values */

		switch (pTPBulk_CommandBlock->CBW_CDB[2] & 0x3F) {
		case 0x3F: {
				// Return all mode pages
				gU2MsdRwInfoBlock.dNVT_DataSize = 4 + 8 + 20 + 12;

				BulkINData[0] = 0x08000000 ;// Header
				BulkINData[0] += (gU2MsdRwInfoBlock.dNVT_DataSize - 1);

				// Caching Page
				BulkINData[3] = 0x00011208 ;//Page Code 0x08; PageLength 0x12
				BulkINData[4] = 0x00000000 ;
				BulkINData[5] = 0x00000000 ;
				BulkINData[6] = 0x00000000 ;
				BulkINData[7] = 0x00000000 ;

				// Informational exception control Mode Page
				BulkINData[8] = 0x00000A1C ;
				BulkINData[9] = 0x00000000 ;
				BulkINData[10] = 0x00000000 ;
			}
			break;

		case 0x08: { // Caching Page
				gU2MsdRwInfoBlock.dNVT_DataSize = 4 + 8 + 20;

				BulkINData[0] = 0x08000000 ;// Header
				BulkINData[0] += (gU2MsdRwInfoBlock.dNVT_DataSize - 1);

				// Caching Page
				BulkINData[3] = 0x00011208 ;//Page Code 0x08; PageLength 0x12
				BulkINData[4] = 0x00000000 ;
				BulkINData[5] = 0x00000000 ;
				BulkINData[6] = 0x00000000 ;
				BulkINData[7] = 0x00000000 ;
			}
			break;

		case 0x1C: {
				gU2MsdRwInfoBlock.dNVT_DataSize = 4 + 8 + 12;

				BulkINData[0] = 0x08000000 ;// Header
				BulkINData[0] += (gU2MsdRwInfoBlock.dNVT_DataSize - 1);

				// Informational exception control Mode Page
				BulkINData[3] = 0x00000A1C ;
				BulkINData[4] = 0x00000000 ;
				BulkINData[5] = 0x00000000 ;
			}
			break;

		case 0x1A:  //  For supporting CDROM in MAC Device.
		case 0x2A:  //  For supporting CDROM in MAC Device.
		case 0x31: { //  For supporting CDROM in MAC Device.

				if (gU2MsdInfo[NVT_LUN].MsdcType == MSDC_CDROM) {
					gU2MsdRwInfoBlock.dNVT_DataSize = 0x4;

					BulkINData[0] = 0x00800000;// Header
					BulkINData[0] += (gU2MsdRwInfoBlock.dNVT_DataSize - 1);
				} else {
					Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
					return FALSE;
				}

			}
			break;

		default: {
				Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
				return FALSE;
			}
			break;

		}
	}


	if (gU2MsdInfo[NVT_LUN].WriteProtect == TRUE) {
		BulkINData[0] |= 0x00800000;
	}


	// DBD(Disable Block Descriptor) is set: Clear the block descriptor
	if ((pTPBulk_CommandBlock->CBW_CDB[1] & 0x08) && (BulkINData[0] >> 24)) {
		UINT32 temp, i;

		temp = BulkINData[0];
		gU2MsdRwInfoBlock.dNVT_DataSize -= (BulkINData[0] >> 24);
		BulkINData[0] &= ~0xFF0000FF;
		BulkINData[0] += gU2MsdRwInfoBlock.dNVT_DataSize - 1;

		temp = (temp >> 24) / 4;
		for (i = (temp + 1); i < 64; i++) {
			BulkINData[i - 2] = BulkINData[i];
		}
	}

	return TRUE;
}

BOOL U2Msdc_SpcINQUIRY(void)
{
	UINT32 NVT_LUN;

	msdc_debug(("U2Msdc_SpcINQUIRY\r\n"));

	NVT_LUN = pTPBulk_CommandBlock->bCBW_LUN;

	if (((pTPBulk_CommandBlock->CBW_CDB[1] & 0x01) == 0x01) &&
		(pTPBulk_CommandBlock->CBW_CDB[5] == 0x00)       &&
		((pTPBulk_CommandBlock->CBW_CDB[2] == 0x83) || (pTPBulk_CommandBlock->CBW_CDB[2] == 0x00) || (pTPBulk_CommandBlock->CBW_CDB[2] == 0x80))) {
		/* Case for EPVD = 1 */

		BulkINData[0] = 0x00000000;
		BulkINData[0] += (pTPBulk_CommandBlock->CBW_CDB[2] << 8);

		if (pTPBulk_CommandBlock->CBW_CDB[2] == 0x80) {
			BulkINData[0]                  += (0x01 << 24);
			BulkINData[1]                   = 0x20202020;
			gU2MsdRwInfoBlock.dNVT_DataSize   = 5;
		} else if (pTPBulk_CommandBlock->CBW_CDB[2] == 0x83) {
			gU2MsdRwInfoBlock.dNVT_DataSize   = 0x4 + 0xC;
			BulkINData[0]                  += (0x0C << 24);
			BulkINData[1]                   = 0x08000201;//EUI-64
			BulkINData[2]                   = 0x00130100;//IEEE Company_ID: 00.01.13 OLYMPUS Corp.
			BulkINData[3]                   = 0x00000000;//Vendor Specific Extension
		} else {
			gU2MsdRwInfoBlock.dNVT_DataSize   = 4;
		}

		if (gU2MsdInfo[NVT_LUN].MsdcType == MSDC_CDROM) {
			((UINT8 *)gU2MsdRwInfoBlock.dNVT_RamAddr)[0] = 0x05;//MMC
		}

		return TRUE;

	} else if ((pTPBulk_CommandBlock->CBW_CDB[2] != 0x00) ||
			   ((pTPBulk_CommandBlock->CBW_CDB[1] & 0x01) != 0x00) ||
			   (pTPBulk_CommandBlock->CBW_CDB[5] != 0x00)) {
		Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
		return FALSE;
	}

	/* Case for EPVD = 0 */
	gU2MsdRwInfoBlock.dNVT_DataSize = 36;
	gU2MsdRwInfoBlock.dNVT_RamAddr = (UINT32)guiU2InquiryDataAddr;
	gU2MsdRwInfoBlock.dNVT_RamAddr_pa = (UINT32)guiU2InquiryDataAddr_pa;

	if (gU2MsdInfo[NVT_LUN].MsdcType == MSDC_STRG) {
		((UINT8 *)gU2MsdRwInfoBlock.dNVT_RamAddr)[0] = 0x00;    //SBC
	} else if (gU2MsdInfo[NVT_LUN].MsdcType == MSDC_CDROM) {
		((UINT8 *)gU2MsdRwInfoBlock.dNVT_RamAddr)[0] = 0x05;    //MMC
	}

	return TRUE;
}

BOOL U2Msdc_SpcREQUESTSENSE(void)
{
	UINT32 NVT_LUN;

	msdc_debug(("U2Msdc_SpcREQUESTSENSE\r\n"));

	NVT_LUN = pTPBulk_CommandBlock->bCBW_LUN;

	if ((pTPBulk_CommandBlock->CBW_CDB[1] & 0x01) || (pTPBulk_CommandBlock->CBW_CDB[5] != 0x00)) {  //field non-supported
		Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
		return FALSE;
	}

	if ((gU2MsdInfo[NVT_LUN].RequestSense.bValid == TRUE)) {
		SenseData[2] = gU2MsdInfo[NVT_LUN].RequestSense.bSenseKey;
		SenseData[12] = gU2MsdInfo[NVT_LUN].RequestSense.bASC;
		SenseData[13] = gU2MsdInfo[NVT_LUN].RequestSense.bASCQ;
		gU2MsdInfo[NVT_LUN].RequestSense.bValid = FALSE ;
	} else {
		SenseData[2] = NO_SENSE;
		SenseData[12] = 0x00;
		SenseData[13] = 0x00;
	}

	gU2MsdRwInfoBlock.dNVT_DataSize = 18;
	gU2MsdRwInfoBlock.dNVT_RamAddr    = (UINT32)SenseData;
	gU2MsdRwInfoBlock.dNVT_RamAddr_pa = (UINT32)SenseData_pa;
	return TRUE;

}

BOOL U2Msdc_SpcMODESENSE10(void)
{
	UINT32 NVT_LUN;

	msdc_debug(("U2Msdc_SpcMODESENSE10\r\n"));

	NVT_LUN = pTPBulk_CommandBlock->bCBW_LUN;

	if ((pTPBulk_CommandBlock->CBW_CDB[2] & 0xC0) == 0xC0) {    //Save value is not supported
		Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x39, 0x00);
		return FALSE;
	} else if ((pTPBulk_CommandBlock->CBW_CDB[9] != 0x00) || (pTPBulk_CommandBlock->CBW_CDB[3] != 0x00)) { //field non-supported
		Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
		return FALSE;
	}

	//Build Block descriptor
	BulkINData[2] = SWAP4(gU2MsdInfo[NVT_LUN].dwMAX_LBA + 1);
	BulkINData[3] = (UINT32)SWAP2(gU2MsdInfo[NVT_LUN].dwSectorSize);

	if ((pTPBulk_CommandBlock->CBW_CDB[2] & 0xC0) == 0x40) { // Changeable values
		switch (pTPBulk_CommandBlock->CBW_CDB[2] & 0x3F) {

		case 0x1C: { // Informational exceptions control mode page.
				//Build Mode Parameter Header
				BulkINData[0] = 0x00001A00;
				BulkINData[1] = 0x08000000;
				gU2MsdRwInfoBlock.dNVT_DataSize = 0x1C;
				// Block descriptor
				// Mode page
				BulkINData[4] = 0x00000A1C ;
				BulkINData[5] = 0x00000000 ;
				BulkINData[6] = 0x00000000 ;
			}
			break;

		case 0x3F: { // Return all mode pages
				//Build Mode Parameter Header
				BulkINData[0] = 0x00001A00;
				BulkINData[1] = 0x08000000;
				gU2MsdRwInfoBlock.dNVT_DataSize = 0x1C;
				// Block descriptor
				// Mode page
				BulkINData[4] = 0x00000A1C ;
				BulkINData[5] = 0x00000000 ;
				BulkINData[6] = 0x00000000 ;
			}
			break;

		default : {
				Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
				return FALSE;
			}
			break;

		}
	} else {
		switch (pTPBulk_CommandBlock->CBW_CDB[2] & 0x3F) {

		case 0x1C: { // Informational exceptions control mode page.
				//Build Mode Parameter Header
				BulkINData[0] = 0x00001A00;
				BulkINData[1] = 0x08000000;
				gU2MsdRwInfoBlock.dNVT_DataSize = 0x1C;
				// Block descriptor
				// Mode page
				BulkINData[4] = 0x00000A1C ;
				BulkINData[5] = 0x00000000 ;
				BulkINData[6] = 0x00000000 ;
			}
			break;

		case 0x3F: { // Return all mode pages
				//Build Mode Parameter Header
				BulkINData[0] = 0x00001A00;
				BulkINData[1] = 0x08000000;
				gU2MsdRwInfoBlock.dNVT_DataSize = 0x1C;
				// Block descriptor
				// Mode page
				BulkINData[4] = 0x00000A1C ;
				BulkINData[5] = 0x00000000 ;
				BulkINData[6] = 0x00000000 ;
			}
			break;

		case 0x1A:
		case 0x2A:
		case 0x31: {
				//
				//  For supporting CDROM in MAC Device.
				//

				if (gU2MsdInfo[NVT_LUN].MsdcType == MSDC_CDROM) {
					BulkINData[0] = 0x80000600;
					BulkINData[1] = 0x00000000;
					gU2MsdRwInfoBlock.dNVT_DataSize = 0x8;
				} else {
					Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
					return FALSE;
				}
			}
			break;

		default: {
				Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
				return FALSE;
			}
			break;

		}
	}
	if (gU2MsdInfo[NVT_LUN].WriteProtect == TRUE) {
		BulkINData[0] |= 0x80000000;
	}

	return TRUE;

}

BOOL U2Msdc_SpcTESTUNITREADY(void)
{
	UINT32 NVT_LUN;

	//msdc_debug(("U2Msdc_SpcTESTUNITREADY\r\n"));

	NVT_LUN = pTPBulk_CommandBlock->bCBW_LUN;

	if ((pTPBulk_CommandBlock->CBW_CDB[5] != 0x00)) {  // not supported field
		Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
		return FALSE;
	}

	if ((gU2MsdInfo[NVT_LUN].StorageEject == TRUE)) {
		Msdc_BuildSenseData(NVT_LUN, NOT_READY, 0x3A, 0x00); // MEDIUM NOT PRESENT
		return FALSE;
	} else {
		return TRUE;
	}
}

BOOL U2Msdc_SpcPRVENTALLOWMEDIUMREMOVAL(void)
{
	UINT32 NVT_LUN;
	UINT8  PREVENT;

	msdc_debug(("U2Msdc_SpcPRVENTALLOWMEDIUMREMOVAL\r\n"));

	NVT_LUN = pTPBulk_CommandBlock->bCBW_LUN;

	PREVENT = pTPBulk_CommandBlock->CBW_CDB[4] & 0x3;

	if ((pTPBulk_CommandBlock->CBW_CDB[5] != 0x00) || ((PREVENT == 0x02) || (PREVENT == 0x03))) { // not supported field
		Msdc_BuildSenseData(NVT_LUN, ILLEGAL_REQUEST, 0x24, 0x00);
		return FALSE;
	} else if (gU2MsdInfo[NVT_LUN].StorageEject == TRUE) {
		Msdc_BuildSenseData(NVT_LUN, NOT_READY, 0x3A, 0x00); // MEDIUM NOT PRESENT
		return FALSE;
	}

	return TRUE;
}

BOOL U2Msdc_SpcREPORTSUPPORTEDOPCODE(void)
{
	msdc_debug(("U2Msdc_SpcREPORTSUPPORTEDOPCODE\r\n"));

	BulkINData[0] = 0x00000000;
	gU2MsdRwInfoBlock.dNVT_DataSize = 0x4;

	return TRUE;
}

#endif

BOOL U2Msdc_MmcREADFORMATCAPACITIES(void)
{
	UINT32 NVT_LUN;

	msdc_debug(("U2Msdc_MmcREADFORMATCAPACITIES\r\n"));
	NVT_LUN = pTPBulk_CommandBlock->bCBW_LUN;

	if (gU2MsdInfo[NVT_LUN].StorageEject == TRUE) {
		BulkINData[0] = 0x08000000;
		BulkINData[1] = 0xFFFFFFFF;
		BulkINData[2] = 0x00020003;
	} else {
		BulkINData[0] = 0x08000000;
		BulkINData[1] = SWAP4(gU2MsdInfo[NVT_LUN].dwMAX_LBA + 1);
		BulkINData[2] = (UINT32)SWAP2(gU2MsdInfo[NVT_LUN].dwSectorSize);
		BulkINData[2] |= 0x02;
	}

	gU2MsdRwInfoBlock.dNVT_DataSize = 12;

	return TRUE;
}

/*
    processing of MMC Command Read TOC/PMA/ATIP
*/
BOOL U2Msdc_MmcREADTOCPMAATIP(void)
{
	msdc_debug(("U2Msdc_MmcREADTOCPMAATIP\r\n"));

	if (gU2MsdInfo[pTPBulk_CommandBlock->bCBW_LUN].MsdcType == MSDC_CDROM) {
		//
		//  For supporting CDROM in MAC Device.
		//

		if (pTPBulk_CommandBlock->CBW_CDB[8] == 0x04) {
			gU2MsdRwInfoBlock.dNVT_DataSize = 0x04;

			BulkINData[0] = 0x01010200;
		} else {
			gU2MsdRwInfoBlock.dNVT_DataSize = 48;

			BulkINData[0]  = 0x01012E00;
			BulkINData[1]  = 0xA0001401;
			BulkINData[2]  = 0x00000000;
			BulkINData[3]  = 0x01000001;
			BulkINData[4]  = 0x00A10014;
			BulkINData[5]  = 0x01000000;
			BulkINData[6]  = 0x14010000;
			BulkINData[7]  = 0x0000A200;
			BulkINData[8]  = 0x08000000;
			BulkINData[9]  = 0x00140100;
			BulkINData[10] = 0x00000001;
			BulkINData[11] = 0x00020000;
		}

	} else {
		gU2MsdRwInfoBlock.dNVT_DataSize = 0x0C;

		//Data Length = 0x0A
		BulkINData[0] = 0x00000A00;
		//First Track = 0x01 / Last Track No = 0x01
		BulkINData[0] |= 0x01010000;
		//ADR/CONTROL = 0x14(all other type)
		BulkINData[1] = 0x00001400;
		//Track Number = 0x01
		BulkINData[1] |= 0x00010000;
		//Track Start Address = 0x0
		BulkINData[2] = 0x00000000;
	}

	return TRUE;
}

/*
    processing of MMC Command Get Event Status Notification
*/
BOOL U2Msdc_MmcGETEVTSTSNOTIFI(void)
{
	gU2MsdRwInfoBlock.dNVT_DataSize = 0x04;

	//Event Descriptor length = 0x0
	BulkINData[0] = 0x0000;

	//NEA = 1 (Supports none of the requested notification classes)
	//Notification Class = 0 (No request Event Classes are supported)
	BulkINData[0] |= (0x80 << 16);

	//Support Event Class = 0 (No requested Event Classes are supported)
	BulkINData[0] |= (0x00 << 24);

	return TRUE;
}

/*
    For supporting CDROM in MAC Device.
*/
BOOL U2Msdc_MmcSETCDSPEED(void)
{
	if (gU2MsdInfo[pTPBulk_CommandBlock->bCBW_LUN].MsdcType == MSDC_CDROM) {
		return TRUE;
	} else {
		Msdc_BuildSenseData(pTPBulk_CommandBlock->bCBW_LUN, ILLEGAL_REQUEST, 0x20, 0x00);
		return FALSE;
	}
}
#endif
