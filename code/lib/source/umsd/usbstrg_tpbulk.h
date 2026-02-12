/*
    USB MSDC Task Bulk Commands Internal Header File

    Header file for functions to handle the SCSI command set used for the USB MSDC Bulk-Only Transport.

    @file       UsbTpBulk.c
    @ingroup    mILibUsbMSDC
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _USBTPBULK_H
#define _USBTPBULK_H

#include "umsd.h"
#include "usb2dev.h"
#include "usb_define.h"
#include "usb_desc.h"


#define SWAP4(x)        ((((x) & 0xFF) << 24) | (((x) & 0xFF00) << 8) | (((x) & 0xFF0000) >> 8) | (((x) & 0xFF000000) >> 24))
#define SWAP2(x)        ((((x) & 0xFF) << 24) | (((x) & 0xFF00) << 8))


#define CSW_GOOD        0x00
#define CSW_FAIL        0x01
#define CSW_PHASE_ERROR 0x02
#define CSW_SIZE        0x0D
#define CSW_SIGNATURE   0x53425355

#define CBW_SIGNATURE   0x43425355
#define CBW_FLAG_IN     0x80
#define CBW_VALID_SIZE  0x1F

#define NO_OPENED_LUN   0xFF

#define MAX_CDBLEN      0x10

typedef struct _COMMAND_BLOCK_WRAPPER {
	UINT32   dCBW_Signature;
	UINT32   dCBW_Tag;
	UINT32   dCBW_DataXferLen;
	UINT8    bCBW_Flag;
	UINT8    bCBW_LUN ;
	UINT8    bCBW_CDBLen ;
	UINT8    CBW_CDB[MAX_CDBLEN];
} CBW, *PCBW;

typedef struct _COMMAND_STATUS_WRAPPER {
	UINT32   dCSW_Signature;
	UINT32   dCSW_Tag;
	UINT32   dCSW_DataResidue;
	UINT8    bCSW_Status;
} CSW, *PCSW;

typedef struct _NVT_INFO_BLOCK {
	UINT32  dNVT_DataSize;
	UINT32  dNVT_LBA;
	UINT32  dNVT_LBALEN;
	UINT32  dNVT_RamAddr;
	UINT32  dNVT_RamAddr_pa;
	UINT8   bNVT_LastOpenedLUN;
} NVTIB, *PNVTIB;

typedef struct _MASS_STORAGE {
	MSDC_TYPE           MsdcType;
	PSTRG_TAB           pStrgDevObj;
	BOOL                WriteProtect;
	BOOL                StorageEject;
	BOOL                StorageChanged;
	BOOL                MEMBusClosed;
	BOOL                LastDetStrg;
	MSDC_StorageDet_CB  DetStrg_cb;
	MSDC_StrgLockDet_CB DetStrgLock_cb;
	UINT8               bSectorBits;
	UINT16              dwSectorSize;
	UINT32              dwMAX_LBA;
	struct  REQUESTSENSE {
		UINT8 bValid;
		UINT8 bSenseKey;
		UINT8 bASC;
		UINT8 bASCQ;
	} RequestSense;

} MASS_STORAGE, *PMASS_STORAGE;


//
//  MSDC Command Set Definition
//

/* SBC commands */
#define SBC_CMD_READ6                       0x08
#define SBC_CMD_READ10                      0x28
#define SBC_CMD_READCAPACITY                0x25
#define SBC_CMD_READCAPACITY16              0x9E
#define SBC_CMD_STARTSTOPUNIT               0x1B
#define SBC_CMD_VERIFY10                    0x2F
#define SBC_CMD_WRITE6                      0x0A
#define SBC_CMD_WRITE10                     0x2A
#define SBC_CMD_FORMATUNIT                  0x04

/* SPC commands */
#define SPC_CMD_INQUIRY                     0x12
#define SPC_CMD_MODESELECT6                 0x15
#define SPC_CMD_MODESENSE6                  0x1A
#define SPC_CMD_PERSISTANTRESERVIN          0x5E
#define SPC_CMD_PERSISTANTRESERVOUT         0x5F
#define SPC_CMD_PRVENTALLOWMEDIUMREMOVAL    0x1E
#define SPC_CMD_RELEASE6                    0x17
#define SPC_CMD_REQUESTSENSE                0x03
#define SPC_CMD_RESERVE6                    0x16
#define SPC_CMD_TESTUNITREADY               0x00
#define SPC_CMD_WRITEBUFFER                 0x3B
#define SPC_CMD_MODESENSE10                 0x5A
#define SPC_CMD_REPORT_SUP_OPCODE           0xA3

/* MMC commands */
#define MMC_CMD_READTOCPMAATIP              0x43
#define MMC_CMD_GETEVTSTSNOTIFI             0x4A
#define MMC_SET_CD_SPEED                    0xBB

/* Vendor Command range*/
#define VENDOR_CMD_MIN                      0xC0
#define VENDOR_CMD_MAX                      0xFF


enum _SENSE_DATA {
	NO_SENSE = 0,
	RECOVERED_ERROR,        //1
	NOT_READY,      //2
	MEDIUM_ERROR,       //3
	HARDWARE_ERROR,         //4
	ILLEGAL_REQUEST, //5
	UNIT_ATTENTION,
	DATA_PROTECT,
	BLANK_CHECK,
	VENDOR_SPECIFIC,
	COPY_ABORTED,    //10
	ABORTED_COMMAND,
	__obsolete,
	VOLUME_OVERFLOW,
	MISCOMPARE
};



extern MASS_STORAGE     gMsdInfo[MAX_LUN],gU2MsdInfo[MAX_LUN];
extern NVTIB            gMsdRwInfoBlock,gU2MsdRwInfoBlock;
extern void             Msdc_CBWHandler(void);
extern void             U2Msdc_CBWHandler(void);
extern MSDC_Led_CB      guiMsdcRWLed_cb,guiU2MsdcRWLed_cb;
extern MSDC_Led_CB      guiMsdcStopUnitLed_cb,guiU2MsdcStopUnitLed_cb;

extern UINT8 *SenseData,*SenseData_pa;

//
//  SCSI command processing APIs
//
void Msdc_CommandHandler(void);
void U2Msdc_CommandHandler(void);

BOOL Msdc_SbcREADCAPACITY(void);
BOOL Msdc_SbcREADCAPACITY16(void);
BOOL Msdc_SbcSTARTSTOPUNIT(void);
BOOL Msdc_SbcVERIFY10(void);
void Msdc_SbcREAD(void);
void Msdc_SbcWRITE(void);
BOOL Msdc_SbcVENDORCMD_DIROUT(PMSDCVendorParam pVendorParam, UINT32 *pDataOutBuf, UINT32 CBW_DataXferLen);
BOOL Msdc_SbcVENDORCMD_DIRIN(PMSDCVendorParam pVendorParam);

BOOL Msdc_SpcMODESENSE6(void);
BOOL Msdc_SpcINQUIRY(void);
BOOL Msdc_SpcREQUESTSENSE(void);
BOOL Msdc_SpcMODESENSE10(void);
BOOL Msdc_SpcTESTUNITREADY(void);
BOOL Msdc_SpcPRVENTALLOWMEDIUMREMOVAL(void);
BOOL Msdc_SpcREPORTSUPPORTEDOPCODE(void);

BOOL Msdc_MmcREADFORMATCAPACITIES(void);
BOOL Msdc_MmcREADTOCPMAATIP(void);
BOOL Msdc_MmcGETEVTSTSNOTIFI(void);
BOOL Msdc_MmcSETCDSPEED(void);

BOOL U2Msdc_SbcREADCAPACITY(void);
BOOL U2Msdc_SbcREADCAPACITY16(void);
BOOL U2Msdc_SbcSTARTSTOPUNIT(void);
BOOL U2Msdc_SbcVERIFY10(void);
void U2Msdc_SbcREAD(void);
void U2Msdc_SbcWRITE(void);
BOOL U2Msdc_SbcVENDORCMD_DIROUT(PMSDCVendorParam pVendorParam, UINT32 *pDataOutBuf, UINT32 CBW_DataXferLen);
BOOL U2Msdc_SbcVENDORCMD_DIRIN(PMSDCVendorParam pVendorParam);

BOOL U2Msdc_SpcMODESENSE6(void);
BOOL U2Msdc_SpcINQUIRY(void);
BOOL U2Msdc_SpcREQUESTSENSE(void);
BOOL U2Msdc_SpcMODESENSE10(void);
BOOL U2Msdc_SpcTESTUNITREADY(void);
BOOL U2Msdc_SpcPRVENTALLOWMEDIUMREMOVAL(void);
BOOL U2Msdc_SpcREPORTSUPPORTEDOPCODE(void);

BOOL U2Msdc_MmcREADFORMATCAPACITIES(void);
BOOL U2Msdc_MmcREADTOCPMAATIP(void);
BOOL U2Msdc_MmcGETEVTSTSNOTIFI(void);
BOOL U2Msdc_MmcSETCDSPEED(void);


#endif
