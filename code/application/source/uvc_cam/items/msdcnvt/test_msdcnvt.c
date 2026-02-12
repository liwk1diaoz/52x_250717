#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kwrap/cmdsys.h>
#include <usb2dev.h>
#include <umsd.h>
#include <sdio.h>
#include <strg_def.h>
#include <hdal.h>
#include <hd_common.h>
#include <msdcnvt/MsdcNvtApi.h>
#include "MsdcNvtCbCom.h"
#include "sys_mempool.h"

#if defined(__FREERTOS)
#include <uart.h>
#else
#include "uart.h"
#endif

#define CFG_MSDCNVT_COMPOSITE 1

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __MODULE__          MsdcNvtCb
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
unsigned int MsdcNvtCb_debug_level = THIS_DBGLVL;
#include <kwrap/debug.h>

//------------------------------------------------------------------------------
// Multi Module Selection
//------------------------------------------------------------------------------
#define CFG_MSDC_NVT_CB_PHOTO   DISABLE  //!< Preview / Capture
#define CFG_MSDC_NVT_CB_DISP    DISABLE  //!< Display Engine Adjustment (IDE Gamma)
#define CFG_MSDC_NVT_CB_FILE    DISABLE  //!< Remote File System
#define CFG_MSDC_NVT_CB_ADJ     ENABLE  //!< Universal Adjustment
#define CFG_MSDC_NVT_CB_UPDFW   DISABLE  //!< Update Fw
#define CFG_MSDC_NVT_CB_EXAM    DISABLE
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Single Module Selection
//------------------------------------------------------------------------------
#define SI_MODULE_NONE          0  //!< No Single Module
#define SI_MODULE_CUSTOM_SI     1  //!< Customer Develop Module
#define SI_MODULE_ISP           2
#define SI_MODULE_AE            3
#define SI_MODULE_TIME_SI       4
#define SI_MODULE_CALUSB        5
#define SI_MODULE_FRNN          6
//Configure your Single Module. Module can be SI_MODULE_NONE / SI_MODULE_CUSTOM_SI / SI_MODULE_ISP..etc
#define CFG_SI_MODULE SI_MODULE_ISP
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Cache Memory Selection
//------------------------------------------------------------------------------
#define CACHE_USE_LOCAL             0
#define CACHE_USE_MEM_POOL          1

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Config Multi Module Include Headers
//------------------------------------------------------------------------------
#if(CFG_MSDC_NVT_CB_PHOTO==ENABLE)
#include "MsdcNvtCb_Photo.h"
#endif
#if(CFG_MSDC_NVT_CB_DISP==ENABLE)
#include "MsdcNvtCb_Disp.h"
#endif
#if(CFG_MSDC_NVT_CB_FILE==ENABLE)
#include "MsdcNvtCb_File.h"
#endif
#if(CFG_MSDC_NVT_CB_ADJ==ENABLE)
#include "MsdcNvtCb_Adj.h"
#endif
#if(CFG_MSDC_NVT_CB_UPDFW==ENABLE)
#include "MsdcNvtCb_UpdFw.h"
#endif
#if(CFG_MSDC_NVT_CB_EXAM==ENABLE)
#include "MsdcNvtCb_Exam.h"
#endif
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Config Single Module Include Headers
//------------------------------------------------------------------------------
#if(CFG_SI_MODULE==SI_MODULE_CUSTOM_SI)
#include "MsdcNvtCb_CustomSi.h"
#elif (CFG_SI_MODULE==SI_MODULE_ISP)
#include "MsdcNvtCb_ISP.h"
#elif (CFG_SI_MODULE==SI_MODULE_AE)
#include "ae_alg.h"
#elif (CFG_SI_MODULE==SI_MODULE_TIME_SI)
#include "MsdcNvtCb_TimeSi.h"
#elif (CFG_SI_MODULE==SI_MODULE_FRNN)
#include "MsdcNvtCb_FRNN.h"
#endif

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

static BOOL m_bInit = FALSE;
extern BOOL m_IsUsbConnected;
extern BOOL m_IsComConnected;

static void make_msdcinfo(USB_MSDC_INFO *pUSBMSDCInfo)
{
	pUSBMSDCInfo->pManuStringDesc = (USB_STRING_DESC *)myUSBManuStrDesc;
	pUSBMSDCInfo->pProdStringDesc = (USB_STRING_DESC *)myUSBMSDCProdStrDesc;
	pUSBMSDCInfo->pSerialStringDesc = (USB_STRING_DESC *)myUSBSerialStrDesc3;

	pUSBMSDCInfo->VID = 0x07B4;
	pUSBMSDCInfo->PID = 0x0109;

	pUSBMSDCInfo->pInquiryData = (UINT8 *)&InquiryData[0];
}

static void xMsdcNvtCb_Init(void)
{
	MSDCNVT_INIT        MsdcNvtInit = {0};

	if (m_bInit == FALSE) {
		MsdcNvtInit.uiApiVer         = MSDCNVT_API_VERSION;
		MsdcNvtInit.pMemCache        = (UINT8*)mempool_msdcnvt;
#if !defined(__FREERTOS)
		MsdcNvtInit.pMemCache_pa     = (UINT8*)mempool_msdcnvt_pa;
#endif
		MsdcNvtInit.uiSizeCache      = MSDCNVT_REQUIRE_MIN_SIZE;
		MsdcNvtInit.bHookDbgMsg      = TRUE;
		MsdcNvtInit.uiUartIdx        = 0;
		MsdcNvtInit.fpEvent          = NULL;
		MsdcNvtInit.pMsdc            = NULL;
		if (!MsdcNvt_Init(&MsdcNvtInit)) {
			DBG_ERR("MsdcNvt_Init() Failed!\r\n");
			return;
		}

		//Register Bi - Direction Functions
#if(CFG_MSDC_NVT_CB_PHOTO==ENABLE)
		MsdcNvtRegBi_Photo();
#endif
#if(CFG_MSDC_NVT_CB_DISP==ENABLE)
		MsdcNvtRegBi_Disp();
#endif
#if(CFG_MSDC_NVT_CB_FILE==ENABLE)
		{
			MSDCNVTCBFILE_INIT Init = {0};
			Init.uiWorkingAddr = OS_GetMempoolAddr(POOL_ID_APP);
			Init.uiWorkingSize = POOL_SIZE_APP;
			MsdcNvtCbFile_Init(&Init);
			MsdcNvtRegBi_File();
		}
#endif
#if(CFG_MSDC_NVT_CB_ADJ==ENABLE)
		MsdcNvtRegBi_Adj();
#endif
#if(CFG_MSDC_NVT_CB_UPDFW==ENABLE)
		MsdcNvtRegBi_UpdFw();
#endif
#if(CFG_MSDC_NVT_CB_EXAM==ENABLE)
		MSDCNVTCBEXAM_INIT ExamInit = {0};
#if(CFG_CACHE_USE==CACHE_USE_LOCAL)
		ExamInit.uiWorkingAddr = (UINT32)m_MsdcNvtExam;
		ExamInit.uiWorkingSize = sizeof(m_MsdcNvtExam);
#elif(CFG_CACHE_USE==CACHE_USE_MEM_POOL)
		ExamInit.uiWorkingAddr = OS_GetMempoolAddr(POOL_ID_MSDCNVT_EXAM);
		ExamInit.uiWorkingSize = OS_GetMempoolSize(POOL_ID_MSDCNVT_EXAM);
#endif
		MsdcNvtCbExam_Init(&ExamInit);
		//set it as "MONITOR" for auto-test server
		MsdcNvtCb_ExamSetName("TARGET");
		MsdcNvtRegBi_Exam();
		//set key map (ks, ki, ...)
		extern BOOL User_CommandFilter(CHAR * pcCmdStr, int len);
		MsdcNvtCb_RegCmdFilterCB(User_CommandFilter);
		//#NT#2016/12/13#Niven Cho -end
#endif
		//#NT#2016/11/10#Niven Cho -end

		//Register Single Direction Functions
#if(CFG_SI_MODULE==SI_MODULE_CUSTOM_SI)
		MsdcNvtRegSi_CustomSi();
#elif (CFG_SI_MODULE==SI_MODULE_ISP)
		msdc_nvt_reg_si_isp();
#elif (CFG_SI_MODULE==SI_MODULE_AE)
		AEAlg2_MsdcCB();
#elif (CFG_SI_MODULE==SI_MODULE_TIME_SI)
		MsdcNvtRegSi_TimeSi();
#elif (CFG_SI_MODULE==SI_MODULE_CALUSB)
		CalUSB_MsdcCB();
#elif (CFG_SI_MODULE==SI_MODULE_FRNN)
		MsdcNvtRegSi_FRNN();
#endif
		m_bInit = TRUE;
	}
}

static void msdcnvt_open(void)
{
	USB_MSDC_INFO MSDCInfo;
	MSDC_OBJ *p_msdc;
	MSDCNVT_LUN *pLun = MsdcNvt_GetNullLun();

	xMsdcNvtCb_Init();

#if defined(__FREERTOS)
	usb2dev_power_on_init(TRUE);
#endif

	p_msdc = Msdc_getObject(MSDC_ID_USB20);
	p_msdc->SetConfig(USBMSDC_CONFIG_ID_SELECT_POWER,  USBMSDC_POW_SELFPOWER);
#if (CFG_MSDCNVT_COMPOSITE == 1)
	p_msdc->SetConfig(USBMSDC_CONFIG_ID_COMPOSITE,  TRUE);
#endif

#if defined(__FREERTOS)
	MSDCInfo.uiMsdcBufAddr = (UINT32)(mempool_msdcnvt_pa);
	if (MSDCInfo.uiMsdcBufAddr == 0) {
		printf("malloc buffer failed\n");
	}
#else
	MSDCInfo.uiMsdcBufAddr_va = mempool_msdcnvt;
	MSDCInfo.uiMsdcBufAddr_pa = mempool_msdcnvt_pa;
	if (MSDCInfo.uiMsdcBufAddr_va == 0) {
		printf("malloc buffer failed\n");
		return;
	}
#endif
	MSDCInfo.uiMsdcBufSize = MSDCNVT_REQUIRE_MIN_SIZE;
	make_msdcinfo(&MSDCInfo);

	MSDCInfo.msdc_check_cb = MsdcNvt_Verify_Cb;
	MSDCInfo.msdc_vendor_cb = MsdcNvt_Vendor_Cb;

	MSDCInfo.msdc_RW_Led_CB = NULL;
	MSDCInfo.msdc_StopUnit_Led_CB = NULL;
	MSDCInfo.msdc_UsbSuspend_Led_CB = NULL;

	MsdcNvt_SetMsdcObj(p_msdc);

	//The callback functions for the MSDC Vendor command.
	//If project doesn't need the MSDC Vendor command, set this callback function as NULL.

	//Assign a Null Lun
	MSDCInfo.pStrgHandle[0] = pLun->pStrg;
	MSDCInfo.msdc_type[0] = pLun->Type;
	MSDCInfo.msdc_storage_detCB[0] = pLun->fpStrgDetCb;
	MSDCInfo.msdc_strgLock_detCB[0] = pLun->fpStrgLockCb;
	MSDCInfo.LUNs = pLun->uiLUNs;

	if (p_msdc->Open(&MSDCInfo) != 0) {
		printf("msdc open failed\r\n");
	}
	m_IsUsbConnected = TRUE;
}

static void msdcnvt_close(void)
{
	MSDC_OBJ *p_msdc = Msdc_getObject(MSDC_ID_USB20);
	p_msdc->Close();
	MsdcNvt_Exit();
	m_IsUsbConnected = FALSE;
}

static void msdcnvt_bycom_open(void)
{
#if(CFG_SI_MODULE==SI_MODULE_CUSTOM_SI)
		MsdcNvtRegSi_CustomSi();
#elif (CFG_SI_MODULE==SI_MODULE_ISP)
		msdc_nvt_reg_si_isp();
#endif
	MsdcNvtCbCom_Open();
	m_IsComConnected = TRUE;
}

static void msdcnvt_bycom_close(void)
{
	MsdcNvtCbCom_Close();
	m_IsComConnected = FALSE;
}

MAINFUNC_ENTRY(msdcnvt, argc, argv)
{
	if (argc < 2) {
		printf("usage:\n");
		printf("\t %s open (via usb)\n", argv[0]);
		printf("\t %s close (via usb)\n", argv[0]);
		printf("\t %s uopen (via uart)\n", argv[0]);
		printf("\t %s uclose (via uart)\n", argv[0]);
		printf("\t %s tuart3 (test uart3 via 115200)\n", argv[0]);
		return 0;
	}

	if (strncmp(argv[1],"open",4) == 0) {
		if (m_IsUsbConnected || m_IsComConnected) {
			printf("connect twice, ignore open\n");
			return 0;
		}
#if !defined(__FREERTOS) && (!CFG_MSDCNVT_COMPOSITE)
		hd_common_init(0);
#endif
		msdcnvt_open();
	} else if (strncmp(argv[1],"close",5) == 0) {
		if (m_IsUsbConnected) {
			msdcnvt_close();
		} else {
			printf("no in usb mode, ignore close\n");
			return 0;
		}
	} else if (strncmp(argv[1],"uopen",5) == 0) {
		if (m_IsUsbConnected || m_IsComConnected) {
			printf("connect twice, ignore open\n");
			return 0;
		}

#if !defined(__FREERTOS)
		HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
		hd_common_init(0);
		hd_common_mem_init(&mem_cfg);
#endif
		if (argc > 2) {
			int baudrate = 3000000;
			sscanf(argv[2], "%d", &baudrate);
			printf("set baudrate: %d\n", baudrate);
			MsdcNvtCbCom_SetupBaudRate(baudrate);
		}
		msdcnvt_bycom_open();
	} else if (strncmp(argv[1],"uclose",6) == 0) {
		if (m_IsComConnected) {
			msdcnvt_bycom_close();
		} else {
			printf("no in com mode, ignore close\n");
		}
	} else if (strncmp(argv[1],"tuart3",6) == 0) {
		char str_test[] = "UART3 test OK.\r\n";
		uart3_open();
		while ((uart3_checkIntStatus() & UART_INT_STATUS_TX_EMPTY) == 0) {
			;
		}
		uart3_init(UART_BAUDRATE_115200, UART_LEN_L8_S1, UART_PARITY_NONE);
		uart3_write((UINT8*) str_test, sizeof(str_test));
		uart3_close();
	} else {
		printf("unknown argument: %s\r\n", argv[1]);
	}

    return 0;
}
