#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kwrap/examsys.h>
#include <usb2dev.h>
#include <umsd.h>
#include <sdio.h>
#include <strg_def.h>

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __MODULE__          msdc
#define __DBGLVL__          THIS_DBGLVL // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

#if defined(__FREERTOS)

static _ALIGNED(4) const UINT16 myUSBManuStrDesc[] = {
	0x0310,                             // 10: size of String Descriptor = 16 bytes
	// 03: String Descriptor type
	'N', 'O', 'V', 'A', 'T', 'E', 'K'   // NOVATEK
};

static _ALIGNED(4) const UINT16 myUSBMSDCProdStrDesc[] = {
	0x0336,                             // 36: size of String Descriptor = 54 bytes
	// 03: String Descriptor type
	'U', 'S', 'B', ' ', 'M', 'a',       // USB Mass Storage
	's', 's', ' ', 'S', 't', 'o',
	'r', 'a', 'g', 'e', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' '
};

//Serial number string descriptor, the content should be updated according to serial number
static _ALIGNED(4) UINT16 myUSBSerialStrDesc3[] = {
	0x0320,                             // 20: size of String Descriptor = 32 bytes
	// 03: String Descriptor type
	'9', '6', '6', '5', '0',            // 96650-00000-001 (default)
	'0', '0', '0', '0', '0',
	'0', '0', '1', '0', '0'
};

static _ALIGNED(64) UINT8 InquiryData[36] = {
	0x00, 0x80, 0x05, 0x02, 0x20, 0x00, 0x00, 0x00,
//    //Vendor identification, PREMIER
	'N', 'O', 'V', 'A', 'T', 'E', 'K', '-',
//    //product identification, DC8365
	'N', 'T', '9', '6', '6', '8', '0', '-',
	'D', 'S', 'P', ' ', 'I', 'N', 'S', 'I',
//    //product revision level, 1.00
	'D', 'E', ' ', ' '
};

static UINT8* m_pMsdcNvtCache = NULL;

static void make_msdcinfo(USB_MSDC_INFO *pUSBMSDCInfo)
{
	pUSBMSDCInfo->pManuStringDesc = (USB_STRING_DESC *)myUSBManuStrDesc;
	pUSBMSDCInfo->pProdStringDesc = (USB_STRING_DESC *)myUSBMSDCProdStrDesc;
	pUSBMSDCInfo->pSerialStringDesc = (USB_STRING_DESC *)myUSBSerialStrDesc3;

	pUSBMSDCInfo->VID = 0x07B4;
	pUSBMSDCInfo->PID = 0x0109;

	pUSBMSDCInfo->pInquiryData = (UINT8 *)&InquiryData[0];
}

static BOOL msdc_strg_detect(void)
{
	return 1;
}

static void msdc_open(void)
{
	USB_MSDC_INFO MSDCInfo;
	MSDC_OBJ *p_msdc;

	usb2dev_power_on_init(TRUE);

	p_msdc = Msdc_getObject(MSDC_ID_USB20);
	p_msdc->SetConfig(USBMSDC_CONFIG_ID_SELECT_POWER,  USBMSDC_POW_SELFPOWER);

	if (m_pMsdcNvtCache == NULL) {
		m_pMsdcNvtCache = (UINT8*)malloc(MSDC_MIN_BUFFER_SIZE);
	}

	MSDCInfo.uiMsdcBufAddr = (UINT32)(m_pMsdcNvtCache);
	if (MSDCInfo.uiMsdcBufAddr == 0) {
		printf("malloc buffer failed\n");
	}
	MSDCInfo.uiMsdcBufSize = MSDC_MIN_BUFFER_SIZE;
	make_msdcinfo(&MSDCInfo);

	MSDCInfo.msdc_check_cb = NULL;
	MSDCInfo.msdc_vendor_cb = NULL;

	MSDCInfo.msdc_RW_Led_CB = NULL;
	MSDCInfo.msdc_StopUnit_Led_CB = NULL;
	MSDCInfo.msdc_UsbSuspend_Led_CB = NULL;

	//The callback functions for the MSDC Vendor command.
	//If project doesn't need the MSDC Vendor command, set this callback function as NULL.

	//Assign a Null Lun
	MSDCInfo.pStrgHandle[0] = sdio_getStorageObject(STRG_OBJ_FAT1);
	MSDCInfo.msdc_type[0] = MSDC_STRG;
	MSDCInfo.msdc_storage_detCB[0] = msdc_strg_detect;
	MSDCInfo.msdc_strgLock_detCB[0] = NULL;
	MSDCInfo.LUNs = 1;

	if (p_msdc->Open(&MSDCInfo) != 0) {
		printf("msdc open failed\r\n");
	}
}

static void msdc_close(void)
{
	MSDC_OBJ *p_msdc = Msdc_getObject(MSDC_ID_USB20);
	p_msdc->Close();
	if (m_pMsdcNvtCache) {
		free(m_pMsdcNvtCache);
		m_pMsdcNvtCache = NULL;
	}
}

EXAMFUNC_ENTRY(test_msdc, argc, argv)
{
	if (argc < 2) {
		printf("usage:\n");
		printf("\t %s open\n", argv[0]);
		printf("\t %s close\n", argv[0]);
		return 0;
	}

	if (strncmp(argv[1],"open",4) == 0) {
		msdc_open();
	} else if (strncmp(argv[1],"close",5) == 0) {
		msdc_close();
	} else {
		printf("unknown argument: %s\r\n", argv[1]);
	}

    return 0;
}

#endif