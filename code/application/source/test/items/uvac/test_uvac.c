#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <usb2dev.h>
#include "UVAC.h"
#include "comm/timer.h"
#include "test_uvac_data.c"
#include "kwrap/stdio.h"
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/examsys.h"
#include "kwrap/sxcmd.h"
#include <kwrap/cmdsys.h>
#include "kwrap/error_no.h"
#include "hd_common.h"
#include "FileSysTsk.h"
#include "hd_gfx.h"
#include <sys/stat.h>

#define __MODULE__          test_uvac
#define __DBGLVL__          7 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"

#define BULK_PACKET_SIZE    512

#define EVB_EXAM_TEST                   1
#define UVAC_STRMRDY_TEST               1
#define UVAC_STRMRDY_TEST_2V2A          0

#define NEW_H264_TEST_PATTERN   1

#define USB_VID                         0x0603
#define USB_PID_PCCAM                   0x8612 // not support pc cam
#define USB_PID_WRITE                   0x8614
#define USB_PID_PRINT                   0x8613
#define USB_PID_MSDC                    0x8611

#define USB_PRODUCT_REVISION            '1', '.', '0', '0'
#define USB_VENDER_DESC_STRING          'N', 0x00,'O', 0x00,'V', 0x00,'A', 0x00,'T', 0x00,'E', 0x00,'K', 0x00, 0x20, 0x00,0x00, 0x00 // NULL
#define USB_VENDER_DESC_STRING_LEN      0x09
#define USB_PRODUCT_DESC_STRING         'D', 0x00,'E', 0x00,'M', 0x00,'O', 0x00,'1', 0x00, 0x20, 0x00,0x00, 0x00 // NULL
#define USB_PRODUCT_DESC_STRING_LEN     0x07
#define USB_PRODUCT_STRING              'N','v','t','-','D','S','C'
#define USB_SERIAL_NUM_STRING           '9', '8', '5', '2', '0','0', '0', '0', '0', '0','0', '0', '1', '0', '0'
#define USB_VENDER_STRING               'N','O','V','A','T','E','K'
#define USB_VENDER_SIDC_DESC_STRING     'N', 0x00,'O', 0x00,'V', 0x00,'A', 0x00,'T', 0x00,'E', 0x00,'K', 0x00, 0x20, 0x00,0x00, 0x00 // NULL
#define USB_VENDER_SIDC_DESC_STRING_LEN 0x09

#if UVAC_STRMRDY_TEST
#if UVAC_STRMRDY_TEST_2V2A
#define UVAC_STRMRDY_AUD_MEMSIZE        0x1000000     //0x1D00000    //29M
#define UVAC_STRMRDY_VID_MEMSIZE        0x2000000     //128M //0x4000000    //64M   //0xB000000    //176M
#define UVAC_STRMRDY_AUD_DEVCNT         2
#define UVAC_STRMRDY_VID_DEVCNT         2
#else
#define UVAC_STRMRDY_AUD_MEMSIZE        0x1D00000    //29M  //0x1000000     //16M  //0x1D00000    //29M
#define UVAC_STRMRDY_VID_MEMSIZE        0x8000000    //128M   0x2000000    //32M  ////0xB000000    //176M
#define UVAC_STRMRDY_AUD_DEVCNT         1
#define UVAC_STRMRDY_VID_DEVCNT         1
#endif
#define NVT_UI_UVAC_VENDCMD_CURCMD      0x01

UINT8  gTestH264HdrValue[] = {
	0x00, 0x00, 0x00, 0x01, 0x67, 0x4D, 0x00, 0x32, 0x8D, 0xA4, 0x05, 0x01, 0xEC, 0x80,
	0x00, 0x00, 0x00, 0x01, 0x68, 0xEE, 0x3C, 0x80
};

typedef struct _CDCLineCoding {
	UINT32   uiBaudRateBPS; ///< Data terminal rate, in bits per second.
	UINT8    uiCharFormat;  ///< Stop bits.
	UINT8    uiParityType;  ///< Parity.
	UINT8    uiDataBits;    ///< Data bits (5, 6, 7, 8 or 16).
} CDCLineCoding;

typedef enum _CDC_PSTN_REQUEST {
	REQ_SET_LINE_CODING         =    0x20,
	REQ_GET_LINE_CODING         =    0x21,
	REQ_SET_CONTROL_LINE_STATE  =    0x22,
	REQ_SEND_BREAK              =    0x23,
	ENUM_DUMMY4WORD(CDC_PSTN_REQUEST)
} CDC_PSTN_REQUEST;

typedef enum _RESOLUTION_IDX {
	RESOLUTION_VGA,
	RESOLUTION_FULL_HD,
	RESOLUTION_IDX_MAX,
	ENUM_DUMMY4WORD(RESOLUTION_IDX)
} RESOLUTION_IDX;

static UVAC_VID_RESO gUIUvacVidReso[RESOLUTION_IDX_MAX] = {
	{ 640,   480,   1,      30,      0,      0},        //4:3
	{ 1920,   1080,   1,    30,      0,      0},
};
static UVAC_VID_RESO gUvc2MjpgReso[] = {
	{ 640,   480,   1,      6,      0,      0},
	{ 1920,   1080,   1,    6,      0,      0},
};
static UVAC_VID_RESO gUvc2H264Reso[] = {
	{ 640,   480,   1,      30,      0,      0},
	{ 1920,   1080,   1,    30,      0,      0},
};

#define CDC_FUNC 0

static RESOLUTION_IDX curr_reso_idx = RESOLUTION_IDX_MAX;

#define NVT_UI_UVAC_AUD_SAMPLERATE_CNT                  1
#define NVT_UI_FREQUENCY_48K                    0x00BB80   //48k, stereo, 16bit
#define AUDIO_CHANNEL_NUM      2
#define AUDIO_BYTE_PER_SAMPLE  2//16bit
#define AUDIO_STREAM_LEN     4096
UINT32 gUIUvacAudSampleRate[NVT_UI_UVAC_AUD_SAMPLERATE_CNT] = {
	NVT_UI_FREQUENCY_48K
};

static  UVAC_VIDEO_FORMAT m_VideoFmt[UVAC_VID_DEV_CNT_MAX] = {0};

static  BOOL stream_enable[UVAC_VID_DEV_CNT_MAX] = {0};

#define UVAC_TEST_VID_OFS   4

#define UVAC_TEST_AUD_FRM_SIZE      0x1A00  //6.5k
#define UVAC_TEST_SET_AUD_FRM_CNT   3

#endif
static BOOL m_bIsStaticPattern = FALSE;

#define MAX_TEST_FRAME_NUM  60
static MEM_RANGE H264Hdr = {0};
static MEM_RANGE H264Frm[MAX_TEST_FRAME_NUM] = {0};
static MEM_RANGE MjpgFrm[MAX_TEST_FRAME_NUM] = {0};

#define MAX_TEST_AUD_NUM  94
static MEM_RANGE AudStream[MAX_TEST_AUD_NUM] = {0};




static MEM_RANGE m_VideoBuf;

static UINT32 uigFrameIdx = 0, uigAudIdx = 0;

#define UVAC_HW_PAYLOAD         0
#define UVAC_HW_PAYLOAD_2       0

#define UNPLUG_USB_PWR_OFF      1

#define PRI_USBCMD                 10
#define STKSIZE_USBCMD             4096
#define STKSIZE_USBCMD_DOWNLOAD    2048

#if CDC_FUNC	
static VK_TASK_HANDLE USBCMDTSK_ID = 0;
static VK_TASK_HANDLE USBCMDTSK2_ID = 0;
#endif
static BOOL xExam_EnableVidSteam(unsigned char argc, char **argv);
static BOOL xExam_EnableAudSteam(unsigned char argc, char **argv);
static void xExam_UVAC_InitStream(UINT32 uiBufAddr, UINT32 uiBufSize, RESOLUTION_IDX ResoIndex);

#define POOL_SIZE_USER_DEFINIED  0x4000000

static UINT32 user_va = 0;
static UINT32 user_pa = 0;
static HD_COMMON_MEM_VB_BLK blk;
static UINT32 work_pa = 0;
static UINT32 work_va = 0;

static UINT32 user_va_to_pa(UINT32 va)
{
	UINT32 pa = 0;

	if (va < user_va + POOL_SIZE_USER_DEFINIED) {
		pa = user_pa + (va - user_va);
		if ((pa < user_pa) || (pa >= (user_pa + POOL_SIZE_USER_DEFINIED))) {
			DBG_ERR("invalid value va=0x%X, pa=0x%X\r\n", va, pa);
		}
	} else {
		DBG_ERR("invalid value va=0x%X, pa=0x%X\r\n", va, pa);
	}
	return pa;
}
static int mem_init(void)
{
	HD_RESULT                 ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	mem_cfg.pool_info[0].type = HD_COMMON_MEM_USER_DEFINIED_POOL;
	mem_cfg.pool_info[0].blk_size = POOL_SIZE_USER_DEFINIED;
	mem_cfg.pool_info[0].blk_cnt = 1;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
		return ret;
	}
	blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_DEFINIED_POOL, POOL_SIZE_USER_DEFINIED, DDR_ID0);
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		DBG_ERR("hd_common_mem_get_block fail\r\n");
		return HD_ERR_NG;
	}
	user_pa = hd_common_mem_blk2pa(blk);
	if (user_pa == 0) {
		DBG_ERR("not get buffer, pa=%08x\r\n", (int)user_pa);
		return HD_ERR_NG;
	}
	user_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, user_pa, POOL_SIZE_USER_DEFINIED);
	/* Release buffer */
	if (user_va == 0) {
		DBG_ERR("mem map fail\r\n");
		ret = hd_common_mem_munmap((void *)user_va, POOL_SIZE_USER_DEFINIED);
		if (ret != HD_OK) {
			DBG_ERR("mem unmap fail\r\n");
			return ret;
		}
		return HD_ERR_NG;
	}
	if(hd_common_mem_alloc("uvac_work", &work_pa, (void **)&work_va, UVAC_GetNeedMemSize(), DDR_ID0) != HD_OK) {
		DBG_ERR("hd_common_mem_alloc failed\r\n");
		work_va = 0;
		work_pa = 0;
	}
	return HD_OK;
}
static HD_RESULT mem_uninit(void)
{
	HD_RESULT ret = HD_OK;

	if (user_va) {
		ret = hd_common_mem_munmap((void *)user_va, POOL_SIZE_USER_DEFINIED);
		if (ret != HD_OK) {
			DBG_ERR("mem_uninit : (user_va)hd_common_mem_munmap fail.\r\n");
			return ret;
		}
	}
	ret = hd_common_mem_release_block(blk);
	if (ret != HD_OK) {
		DBG_ERR("mem_uninit : hd_common_mem_release_block fail.\r\n");
		return ret;
	}
	return hd_common_mem_uninit();
}
static UINT32 GetUserPoolPA(void)
{
	DBG_DUMP("user pa=0x%X size=0x%X\r\n", user_pa, POOL_SIZE_USER_DEFINIED);
	return user_pa;
}
static UINT32 GetUserPoolVA(void)
{
	DBG_DUMP("user va=0x%X size=0x%X\r\n", user_va, POOL_SIZE_USER_DEFINIED);
	return user_va;
}
#if CDC_FUNC
static void DumpMem(UINT32 Addr, UINT32 Size, UINT32 Alignment)
{
	UINT32 i;
	UINT8 *pBuf = (UINT8 *)Addr;
	for (i = 0; i < Size; i++) {
		if (i > 0 && i % Alignment == 0) {
			DBG_MSG("\r\n");
		}
		DBG_MSG("0x%02X ", *(pBuf + i));
	}
	DBG_MSG("\r\n");
}
THREAD_DECLARE(UsbCmdTsk, arglist)
{
	ER  Ret;
	UINT32 Temp[512 / 4]; //just get a 4-byte-aligned buffer
	UINT32 DataLen;

	THREAD_ENTRY();
	DBG_FUNC_BEGIN("\r\n");
	while (USBCMDTSK_ID) {
		DataLen = sizeof(Temp);
		Ret = UVAC_ReadCdcData(CDC_COM_1ST, (UINT8 *)Temp, &DataLen);

		if (E_OK == Ret) {
			DBG_DUMP("Receive CDC_COM_1ST: %d B\r\n", DataLen);
			if (DataLen < 100) {
				DumpMem((UINT32)Temp, DataLen, 16);
			}
		} else if (E_RSATR == Ret) {
			DBG_IND("Aobrt ReadCdc to exit.\r\n");
			break;
		}
	}
	DBG_FUNC_END("\r\n");
	THREAD_RETURN(0);
}

THREAD_DECLARE(UsbCmdTsk2, arglist)
{
	ER  Ret;
	UINT32 Temp[512 / 4]; //just get a 4-byte-aligned buffer
	UINT32 DataLen;

	THREAD_ENTRY();
	DBG_FUNC_BEGIN("\r\n");
	while (USBCMDTSK2_ID) {
		DataLen = sizeof(Temp);
		Ret = UVAC_ReadCdcData(CDC_COM_2ND, (UINT8 *)Temp, &DataLen);

		if (E_OK == Ret) {
			DBG_DUMP("Receive CDC_COM_2ND: %d B\r\n", DataLen);
			if (DataLen < 100) {
				DumpMem((UINT32)Temp, DataLen, 16);
			}
		} else if (E_RSATR == Ret) {
			DBG_IND("Aobrt ReadCdc to exit.\r\n");
			break;
		}
	}
	DBG_FUNC_END("\r\n");
	THREAD_RETURN(0);
}

static BOOL CdcPstnReqCB(CDC_COM_ID ComID, UINT8 Code, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL bSupported = TRUE;
	CDCLineCoding LineCoding;
	DBGD(ComID);
	switch (Code) {
	case REQ_GET_LINE_CODING:
		DBG_DUMP("Get Line Coding\r\n");
		LineCoding.uiBaudRateBPS = 115200;
		LineCoding.uiCharFormat = 0;//CDC_LINEENCODING_OneStopBit;
		LineCoding.uiParityType = 0;//CDC_PARITY_None;
		LineCoding.uiDataBits = 8;
		*pDataLen = sizeof(LineCoding);
		memcpy(pData, &LineCoding, *pDataLen);
		break;
	case REQ_SET_LINE_CODING:
		DBG_DUMP("Set Line Coding\r\n");
		if (*pDataLen == sizeof(LineCoding)) {
			memcpy(&LineCoding, pData, *pDataLen);
		} else {
			bSupported = FALSE;
		}
		break;
	case REQ_SET_CONTROL_LINE_STATE:
		DBG_DUMP("Control Line State = 0x%X\r\n", *(UINT16 *)pData);
		//debug console test
		if (*(UINT16 *)pData == 0x3) { //console ready
		}
		break;
	default:
		bSupported = FALSE;
		break;
	}
	return bSupported;
}
#endif
//====================================================================================
_ALIGNED(4) static UINT16 m_UVACSerialStrDesc3[] = {
	0x0320,                             // 20: size of String Descriptor = 32 bytes
	// 03: String Descriptor type
	'5', '1', '0', '5', '5',            // 96611-00000-001 (default)
	'0', '0', '0', '0', '0',
	'0', '0', '1', '0', '0'
};
_ALIGNED(4) const static UINT8 m_UVACManuStrDesc[] = {
	USB_VENDER_DESC_STRING_LEN * 2 + 2, // size of String Descriptor = 6 bytes
	0x03,                       // 03: String Descriptor type
	USB_VENDER_DESC_STRING
};

_ALIGNED(4) const static UINT8 m_UVACProdStrDesc[] = {
	USB_PRODUCT_DESC_STRING_LEN * 2 + 2, // size of String Descriptor = 6 bytes
	0x03,                       // 03: String Descriptor type
	USB_PRODUCT_DESC_STRING
};
static void xUSBMakerInit_UVAC(UVAC_VEND_DEV_DESC *pUVACDevDesc)
{
	pUVACDevDesc->pManuStringDesc = (UVAC_STRING_DESC *)m_UVACManuStrDesc;
	pUVACDevDesc->pProdStringDesc = (UVAC_STRING_DESC *)m_UVACProdStrDesc;

	pUVACDevDesc->pSerialStringDesc = (UVAC_STRING_DESC *)m_UVACSerialStrDesc3;

	pUVACDevDesc->VID = USB_VID;
	pUVACDevDesc->PID = USB_PID_PCCAM;

	//pUVACDevDesc->fpVendReqCB = xUvacVendReqCB;
	// pUVACDevDesc->fpIQVendReqCB = xUvacVendReqIQCB;

}
static void xUvacTriggerVidStream(UVAC_VID_DEV_CNT VidDev, UINT32 FrameIdx)
{
	UVAC_STRM_FRM strmFrm = {0};
	if (stream_enable[VidDev] == FALSE) {
		return;
	}
	//DBG_IND("\r\n");
	if ((FrameIdx & 0x1) == 0) {
		if (m_VideoFmt[VidDev] == UVAC_VIDEO_FORMAT_H264) {
			strmFrm.addr = (UINT32)H264Frm0;
			strmFrm.size = sizeof(H264Frm0);
			strmFrm.pStrmHdr = (UINT8 *)gTestH264HdrValue;
			strmFrm.strmHdrSize = sizeof(gTestH264HdrValue);
		} else {
			strmFrm.addr = (UINT32)MjpgFrm0;
			strmFrm.size = sizeof(MjpgFrm0);
		}
	} else {
		if (m_VideoFmt[VidDev] == UVAC_VIDEO_FORMAT_H264) {
			strmFrm.addr = (UINT32)H264Frm1;
			strmFrm.size = sizeof(H264Frm1);
			strmFrm.pStrmHdr = (UINT8 *)gTestH264HdrValue;
			strmFrm.strmHdrSize = sizeof(gTestH264HdrValue);
		} else {
			strmFrm.addr = (UINT32)MjpgFrm1;
			strmFrm.size = sizeof(MjpgFrm1);
		}
	}
	if (VidDev == UVAC_VID_DEV_CNT_1) {
		strmFrm.path = UVAC_STRM_VID;
	} else {
		strmFrm.path = UVAC_STRM_VID2;
	}

	UVAC_SetEachStrmInfo(&strmFrm);
}

static void xUvacTriggerStream(UINT32 uiEvent)
{
	//DBG_IND("\r\n");
	xUvacTriggerVidStream(UVAC_VID_DEV_CNT_1, uigFrameIdx);
	xUvacTriggerVidStream(UVAC_VID_DEV_CNT_2, uigFrameIdx);
	uigFrameIdx++;
}
static void xUvacTriggerVidFrame(UVAC_VID_DEV_CNT VidDev, UINT32 FrameIdx)
{
	UVAC_STRM_FRM strmFrm = {0};
	UINT32 Frm;

	if (stream_enable[VidDev] == FALSE) {
		return;
	}
	Frm = FrameIdx % MAX_TEST_FRAME_NUM;
	//DBG_IND("\r\n");
	if (m_VideoFmt[VidDev] == UVAC_VIDEO_FORMAT_H264) {
		strmFrm.addr = user_va_to_pa(H264Frm[Frm].addr);		
		strmFrm.size = H264Frm[Frm].size;
		strmFrm.va = H264Frm[Frm].addr;
		//strmFrm.pStrmHdr = (UINT8 *)user_va_to_pa(H264Hdr.addr);
		//strmFrm.strmHdrSize = H264Hdr.size;
	} else {
		strmFrm.addr = user_va_to_pa(MjpgFrm[Frm].addr);
		strmFrm.size = MjpgFrm[Frm].size;
		strmFrm.va = MjpgFrm[Frm].addr;
	}

	if (VidDev == UVAC_VID_DEV_CNT_1) {
		strmFrm.path = UVAC_STRM_VID;
	} else {
		strmFrm.path = UVAC_STRM_VID2;
	}

	UVAC_SetEachStrmInfo(&strmFrm);
}
static void xUvacTriggerFrame(UINT32 uiEvent)
{
	//DBG_IND("\r\n");
	xUvacTriggerVidFrame(UVAC_VID_DEV_CNT_1, uigFrameIdx);
	xUvacTriggerVidFrame(UVAC_VID_DEV_CNT_2, uigFrameIdx);
	uigFrameIdx++;
}
static void xUvacTriggerAud(UINT32 uiEvent)
{
	UVAC_STRM_FRM strmFrm = {0};
	UINT32 Frm;

	Frm = uigAudIdx % MAX_TEST_AUD_NUM;
	strmFrm.addr = user_va_to_pa(AudStream[Frm].addr);
	strmFrm.size = AudStream[Frm].size;
	strmFrm.path = UVAC_STRM_AUD;
	UVAC_SetEachStrmInfo(&strmFrm);
	strmFrm.path = UVAC_STRM_AUD2;
	UVAC_SetEachStrmInfo(&strmFrm);
	uigAudIdx++;
}
#if 0
static UINT32 xUvacGetH264HeaderCB(UVAC_STRM_INFO *pStrmInfo)
{
	UINT32 strmHdrAddr;
	UINT32 strmHdrSize;

	DBG_IND("FrmFile:UVAC Vid W=%d,H=%d,Fmt=%d,fps=%d\r\n", pStrmInfo->strmWidth, pStrmInfo->strmHeight, pStrmInfo->strmCodec, pStrmInfo->strmFps);
	if (m_bIsStaticPattern) {
		strmHdrAddr = (UINT32)&gTestH264HdrValue[0];
		strmHdrSize = sizeof(gTestH264HdrValue);
	} else {
		strmHdrAddr = H264Hdr.addr;
		strmHdrSize = H264Hdr.size;
	}

	pStrmInfo->pStrmHdr = (UINT8 *)strmHdrAddr;
	pStrmInfo->strmHdrSize = strmHdrSize;

	return E_OK;
}
#endif

static UINT32 xUvacStartVideoCB(UVAC_VID_DEV_CNT vidDevIdx, UVAC_STRM_INFO *pStrmInfo)
{
	if (pStrmInfo) {
		DBG_DUMP("UVAC Start[%d]++resoIdx=%d,W=%d,H=%d,codec=%d,fps=%d,path=%d,tbr=0x%x\r\n", vidDevIdx, pStrmInfo->strmResoIdx, pStrmInfo->strmWidth, pStrmInfo->strmHeight, pStrmInfo->strmCodec, pStrmInfo->strmFps, pStrmInfo->strmPath, pStrmInfo->strmTBR);
		stream_enable[vidDevIdx] = FALSE;
		m_VideoFmt[vidDevIdx] = pStrmInfo->strmCodec;
		xExam_UVAC_InitStream(GetUserPoolVA(), POOL_SIZE_USER_DEFINIED, pStrmInfo->strmResoIdx);
		stream_enable[vidDevIdx] = TRUE;
		uigFrameIdx = 0;
		DBG_DUMP("UVAC Start[%d]--\r\n", vidDevIdx);
	}
	return E_OK;
}

static UINT32 xUvacStopVideoCB(UVAC_VID_DEV_CNT vidDevIdx)
{
	DBG_DUMP("UVAC STOP[%d]\r\n", vidDevIdx);
	stream_enable[vidDevIdx] = FALSE;
	return E_OK;
}

static void xExam_UVAC_InitStream(UINT32 uiBufAddr, UINT32 uiBufSize, RESOLUTION_IDX ResoIndex)
{
	UINT32 i;
	CHAR         tempfilename[128];
	int     filehdl;
	UINT32  FreeBufAddr = ALIGN_CEIL_4(uiBufAddr);
	struct stat st;

	if (curr_reso_idx == ResoIndex) {
		return;
	}
	//h264 header
	#if (NEW_H264_TEST_PATTERN)
	FILE *bs_fd = 0, *bslen_fd = 0;
	INT bs_size = 0, read_len = 0;

	if (ResoIndex == RESOLUTION_VGA) {
		snprintf(tempfilename, sizeof(tempfilename), "/mnt/sd/UVAC/video_bs_640_480_h264.dat");
	} else {
		snprintf(tempfilename, sizeof(tempfilename), "/mnt/sd/UVAC/video_bs_1920_1080_h264.dat");
	}
	if ((bs_fd = fopen(tempfilename, "rb")) == NULL) {
		printf("open file (%s) fail !!....\r\nPlease copy test pattern to SD Card !!\r\n\r\n", tempfilename);
		return;
	}
	printf("bs file: [%s]\n", tempfilename);
	if (ResoIndex == RESOLUTION_VGA) {
		snprintf(tempfilename, sizeof(tempfilename), "/mnt/sd/UVAC/video_bs_640_480_h264.len");
	} else {
		snprintf(tempfilename, sizeof(tempfilename), "/mnt/sd/UVAC/video_bs_1920_1080_h264.len");
	}
	if ((bslen_fd = fopen(tempfilename, "rb")) == NULL) {
		printf("open file (%s) fail !!....\r\nPlease copy test pattern to SD Card !!\r\n\r\n", tempfilename);
		fclose(bs_fd);
		return;
	}
	printf("bslen file: [%s]\n", tempfilename);

	#endif
	if (ResoIndex == RESOLUTION_VGA) {
		snprintf(tempfilename, sizeof(tempfilename), "/mnt/sd/UVAC/Stream.hdr");
	} else {
		snprintf(tempfilename, sizeof(tempfilename), "/mnt/sd/UVAC/Stream_1080p.hdr");
	}
	if (stat(tempfilename, &st) != 0) {
		printf("failed to get %s, err = %d \n", tempfilename, errno);
		return;
	}
	filehdl = open(tempfilename, O_RDONLY);
	if (filehdl) {
		H264Hdr.addr = FreeBufAddr;
		H264Hdr.size = st.st_size;
		read(filehdl, (unsigned char *)H264Hdr.addr, st.st_size);
		close(filehdl);
		FreeBufAddr += ALIGN_CEIL_4(H264Hdr.size);
		//DBG_DUMP("H264Hdr @0x%X, %d\r\n",H264Hdr.addr,H264Hdr.size);
	} else {
		DBG_ERR("fail to open %s\r\n", tempfilename);
	}

	//read video stream to buffer
	for (i = 0; i < MAX_TEST_FRAME_NUM; i++) {
		//h264 frame
		#if NEW_H264_TEST_PATTERN
		// get bs size
		if (fscanf(bslen_fd, "%d\n", &bs_size) == EOF) {
			// reach EOF, read from the beginning
			fseek(bs_fd, 0, SEEK_SET);
			fseek(bslen_fd, 0, SEEK_SET);
			if (fscanf(bslen_fd, "%d\n", &bs_size) == EOF) {
				DBG_ERR("fscanf error\n");
			}
		}
		//DBGD(bs_size);
		if (bs_size == 0) {
			printf("Invalid bs_size(%d)\n", bs_size);
			continue;
		}

		H264Frm[i].addr = FreeBufAddr;
		H264Frm[i].size = bs_size;
		// read bs from file
		read_len = fread((void *)H264Frm[i].addr, 1, bs_size, bs_fd);
		if (read_len != bs_size) {
			DBG_ERR("reading error\n");
		}
		FreeBufAddr += ALIGN_CEIL_4(H264Frm[i].size);


		#else
		snprintf(tempfilename, sizeof(tempfilename), "A:\\UVAC\\UVC_%d.264", (int)i);
		filehdl = FileSys_OpenFile(tempfilename, FST_OPEN_READ);
		if (filehdl) {
			H264Frm[i].addr = FreeBufAddr;
			H264Frm[i].size = FileSys_GetFileLen(tempfilename);
			if (FST_STA_OK != FileSys_ReadFile(filehdl, (UINT8 *)H264Frm[i].addr, &H264Frm[i].size, 0, NULL)) {
				DBG_ERR("read %s error!\r\n", tempfilename);
			}
			FileSys_CloseFile(filehdl);
			FreeBufAddr += ALIGN_CEIL_4(H264Frm[i].size);
		} else {
			DBG_ERR("fail to open %s\r\n", tempfilename);
			break;
		}
		#endif
		//DBG_DUMP("H264Frm[%02d] @0x%X, %d\r\n",i,H264Frm[i].addr,H264Frm[i].size);
		//MJPEG frame
		if (ResoIndex == RESOLUTION_VGA) {
			snprintf(tempfilename, sizeof(tempfilename), "/mnt/sd/UVAC/UVC_%d.jpg", (int)i);
		} else if (ResoIndex == RESOLUTION_FULL_HD) {
			snprintf(tempfilename, sizeof(tempfilename), "/mnt/sd/UVAC/1080p/UVC_%d.jpg", (int)i);
		} else {
			DBG_ERR("ResoIndex(%d) error!\r\n", ResoIndex);
		}
		if (stat(tempfilename, &st) != 0) {
			printf("failed to get %s, err = %d \n", tempfilename, errno);
			return;
		}
		filehdl = open(tempfilename, O_RDONLY);

		if (filehdl) {
			MjpgFrm[i].addr = FreeBufAddr;
			MjpgFrm[i].size = st.st_size;
			read(filehdl, (unsigned char *)MjpgFrm[i].addr, st.st_size);
			close(filehdl);
			FreeBufAddr += ALIGN_CEIL_4(MjpgFrm[i].size);
		} else {
			DBG_ERR("fail to open %s\r\n", tempfilename);
			break;
		}
		if (FreeBufAddr > uiBufAddr + uiBufSize) {
			DBG_ERR("Insufficent buffer size!\r\n");
			break;
		}
		//DBG_DUMP("MjpgFrm[%02d] @0x%X, %d\r\n",i,MjpgFrm[i].addr,MjpgFrm[i].size);
	}
	#if NEW_H264_TEST_PATTERN
	fclose(bs_fd);
	fclose(bslen_fd);
	#endif

	//read audio stream to buffer
	snprintf(tempfilename, sizeof(tempfilename), "/mnt/sd/UVAC/AUD.pcm");
	if (stat(tempfilename, &st) != 0) {
		printf("failed to get %s, err = %d \n", tempfilename, errno);
		return;
	}
	filehdl = open(tempfilename, O_RDONLY);


	if (filehdl) {
		read(filehdl, (unsigned char *)FreeBufAddr, st.st_size);
		close(filehdl);
		for (i = 0; i < MAX_TEST_AUD_NUM; i++) {
			AudStream[i].addr = FreeBufAddr;
			AudStream[i].size = AUDIO_STREAM_LEN;
			FreeBufAddr += AUDIO_STREAM_LEN;
			//DBG_DUMP("AudStream[%02d] @0x%X, %d\r\n",i,AudStream[i].addr,AudStream[i].size);
		}
	} else {
		DBG_ERR("fail to open %s\r\n", tempfilename);
	}
	if (FreeBufAddr > uiBufAddr + uiBufSize) {
		DBG_ERR("Insufficent buffer size!\r\n");
	}
	curr_reso_idx = ResoIndex;
}

BOOL xExam_UVAC_enable(unsigned char argc, char **argv)
{

	UVAC_INFO       UvacInfo = {0};
	UINT32          uiPoolAddr;
	UINT32          retV = 0;
	UINT32          uvacNeedMem;
	UVAC_VEND_DEV_DESC UIUvacDevDesc = {0};
	UVAC_AUD_SAMPLERATE_ARY uvacAudSampleRateAry = {0};
	UINT32 ChannelNum = UVAC_CHANNEL_1V1A;
	UINT32 bVidFmtType = UVAC_VIDEO_FORMAT_H264_MJPEG;
	m_bIsStaticPattern = FALSE;
	UINT32 bEnableCDC = 0;	

	//sscanf_s(strCmd, "%d %d %d %d", &ChannelNum, &bEnableCDC, &bVidFmtType, &m_bIsStaticPattern);
	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&ChannelNum);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%d", (int *)&bVidFmtType);
	}
	if (argc >= 3) {
		sscanf(argv[2], "%d", (int *)&m_bIsStaticPattern);
	}
	if (argc >= 4) {
		sscanf(argv[3], "%d", (int *)&bEnableCDC);
	}

	DBG_IND("###Real Open UVAC-lib\r\n");

	uiPoolAddr = GetUserPoolPA();
	uvacNeedMem = UVAC_GetNeedMemSize();

	if (ChannelNum > UVAC_CHANNEL_1V1A) {
		uvacNeedMem *= 2;
	}
	
	UvacInfo.UvacMemAdr    = work_pa;
	UvacInfo.UvacMemSize   = uvacNeedMem;

	xExam_UVAC_InitStream(GetUserPoolVA(), POOL_SIZE_USER_DEFINIED, RESOLUTION_VGA);
	DBG_DUMP("AppMemStart PA=0x%x,VA=0x%x,0x%x,uvac=0x%x,0x%x\r\n", uiPoolAddr, GetUserPoolVA(), POOL_SIZE_USER_DEFINIED, UvacInfo.UvacMemAdr, uvacNeedMem);
	#if 0
	{
		UVAC_VID_BUF_INFO vid_buf_info = {0};
		vid_buf_info.vid_buf_pa = uiPoolAddr;
		vid_buf_info.vid_buf_size = POOL_SIZE_USER_DEFINIED;
		UVAC_SetConfig(UVAC_CONFIG_VID_BUF_INFO, (UINT32)&vid_buf_info);
	}
	#endif

	UvacInfo.fpStartVideoCB  = (UVAC_STARTVIDEOCB)xUvacStartVideoCB;
	//Don't need anymore
	//UvacInfo.fpGetH264HeaderCB = (UVAC_GETH264HEADERCB)xUvacGetH264HeaderCB;
	UvacInfo.fpStopVideoCB  = (UVAC_STOPVIDEOCB)xUvacStopVideoCB;
	xUSBMakerInit_UVAC(&UIUvacDevDesc);
	UVAC_SetConfig(UVAC_CONFIG_VEND_DEV_DESC, (UINT32)&UIUvacDevDesc);

	DBG_IND("%s:uvacAddr=0x%x,s=0x%x;IPLAddr=0x%x,s=0x%x\r\n", __func__, UvacInfo.UvacMemAdr, UvacInfo.UvacMemSize, m_VideoBuf.addr, m_VideoBuf.size);
#if UVAC_HW_PAYLOAD
	UvacInfo.hwPayload[0] = TRUE;
#else
	UvacInfo.hwPayload[0] = FALSE;
#endif
#if UVAC_HW_PAYLOAD_2
	UvacInfo.hwPayload[1] = TRUE;
#else
	UvacInfo.hwPayload[1] = FALSE;
#endif

	//w=1280,720,30,2
	UVAC_ConfigVidReso(gUIUvacVidReso, RESOLUTION_IDX_MAX);
	uvacAudSampleRateAry.aryCnt = NVT_UI_UVAC_AUD_SAMPLERATE_CNT;
	uvacAudSampleRateAry.pAudSampleRateAry = &gUIUvacAudSampleRate[0];
	UVAC_SetConfig(UVAC_CONFIG_AUD_SAMPLERATE, (UINT32)&uvacAudSampleRateAry);
	UvacInfo.channel = ChannelNum;
	DBG_IND("w=%d,h=%d,fps=%d,codec=%d\r\n", UvacInfo.strmInfo.strmWidth, UvacInfo.strmInfo.strmHeight, UvacInfo.strmInfo.strmFps, UvacInfo.strmInfo.strmCodec);

	if (ChannelNum > UVAC_CHANNEL_1V1A) {
		UVAC_VID_RESO_ARY Uvc2MjpgResoAry = {0};
		UVAC_VID_RESO_ARY Uvc2H264ResoAry = {0};

		Uvc2MjpgResoAry.aryCnt = sizeof(gUvc2MjpgReso)/sizeof(UVAC_VID_RESO);
		Uvc2MjpgResoAry.pVidResAry = &gUvc2MjpgReso[0];
		//set default frame index, starting from 1
		Uvc2MjpgResoAry.bDefaultFrameIndex = 1;
		UVAC_SetConfig(UVAC_CONFIG_UVC2_MJPG_FRM_INFO, (UINT32)&Uvc2MjpgResoAry);

		Uvc2H264ResoAry.aryCnt = sizeof(gUvc2H264Reso)/sizeof(UVAC_VID_RESO);
		Uvc2H264ResoAry.pVidResAry = &gUvc2H264Reso[0];
		//set default frame index, starting from 1
		Uvc2H264ResoAry.bDefaultFrameIndex = 1;
		UVAC_SetConfig(UVAC_CONFIG_UVC2_H264_FRM_INFO, (UINT32)&Uvc2H264ResoAry);
	}
#if CDC_FUNC	
	UVAC_SetConfig(UVAC_CONFIG_CDC_PSTN_REQUEST_CB, (UINT32)CdcPstnReqCB);
#endif		
	UVAC_SetConfig(UVAC_CONFIG_CDC_ENABLE, bEnableCDC);
	UVAC_SetConfig(UVAC_CONFIG_VIDEO_FORMAT_TYPE, bVidFmtType);
	retV = UVAC_Open(&UvacInfo);
	if (retV != E_OK) {
		DBG_ERR("Error open UVAC task\r\n");
	}
#if CDC_FUNC		
	if (bEnableCDC) {
		vos_task_resume(USBCMDTSK_ID);
		vos_task_resume(USBCMDTSK2_ID);
	}
#endif
	//default enable video and audio sream
	//xExam_EnableVidSteam("1");
	//xExam_EnableAudSteam("1");
	return TRUE;
}
static BOOL xExam_UVAC_open(unsigned char argc, char **argv)
{
	HD_RESULT ret;

	#ifdef __LINUX
	#else
	usb2dev_power_on_init(TRUE);
	#endif
#if CDC_FUNC	
	USBCMDTSK_ID = vos_task_create(UsbCmdTsk, 0, "UsbCmdTsk", PRI_USBCMD, STKSIZE_USBCMD);
	if (0 == USBCMDTSK_ID) {
		DBG_ERR("UsbCmdTsk create failed\r\n");
	}
	USBCMDTSK2_ID = vos_task_create(UsbCmdTsk2, 0, "UsbCmdTsk2", PRI_USBCMD, STKSIZE_USBCMD);
	if (0 == USBCMDTSK2_ID) {
		DBG_ERR("UsbCmdTsk2 create failed\r\n");
	}
#endif	
	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		DBG_ERR("common fail=%d\n", ret);
		return FALSE;
	}
	ret = hd_gfx_init();
    if(ret != HD_OK) {
        printf("init gfx fail=%d\n", ret);
        return FALSE;
    }
	ret = mem_init();
	if (ret != HD_OK) {
		DBG_ERR("mem fail=%d\n", ret);
		return FALSE;
	}
	return TRUE;
}
static BOOL xExam_UVAC_disable(unsigned char argc, char **argv)
{
#if CDC_FUNC
	UVAC_AbortCdcRead(CDC_COM_1ST);
	UVAC_AbortCdcRead(CDC_COM_2ND);
#endif	
	UVAC_Close();
	return TRUE;
}
static BOOL xExam_UVAC_close(unsigned char argc, char **argv)
{
	DBG_IND(" ++\r\n");
#if CDC_FUNC		
	vos_task_destroy(USBCMDTSK_ID);
	vos_task_destroy(USBCMDTSK2_ID);
#endif	
	mem_uninit();
	if (hd_common_uninit() != HD_OK) {
		printf("hd_common_uninit fail\n");
	}
	DBG_IND(" --\r\n");
	return TRUE;
}
static BOOL xExam_EnableSteam(unsigned char argc, char **argv)
{
	ER retV = E_OK;
	UINT32 FrameRate = 30, Enable = 1;
	UINT32 intval;
	static TIMER_ID gStreamTimerID = TIMER_NUM;

	//sscanf_s(strCmd, "%d %d", &Enable, &FrameRate);
	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&Enable);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%d", (int *)&FrameRate);
	}
	if (Enable) {
		uigFrameIdx = 0;
		intval = 1000000 / FrameRate;
		retV = timer_open(&gStreamTimerID, xUvacTriggerStream);
		if (retV != E_OK) {
			DBG_ERR("open timer fail=%d\n\r", retV);
			return TRUE;
		}
		retV = timer_cfg(gStreamTimerID, intval, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
		if (retV != E_OK) {
			DBG_ERR("set timer fail=%d,intval=%d\n\r", retV, intval);
			return TRUE;
		}
	} else {
		if (gStreamTimerID != TIMER_NUM) {
			timer_close(gStreamTimerID);
		}
	}
	return TRUE;
}
static BOOL xExam_EnableVidSteam(unsigned char argc, char **argv)
{
	ER retV = E_OK;
	UINT32 FrameRate = 30, Enable = 1;
	UINT32 intval;
	static TIMER_ID gStreamTimerID = TIMER_NUM;

	//sscanf_s(strCmd, "%d %d", &Enable, &FrameRate);
	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&Enable);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%d", (int *)&FrameRate);
	}
	DBG_DUMP("VID stream en=%d, fps=%d\n\r", Enable, FrameRate);
	if (Enable && (gStreamTimerID == TIMER_NUM)) {
		uigFrameIdx = 0;
		intval = 1000000 / FrameRate;
		retV = timer_open(&gStreamTimerID, xUvacTriggerFrame);
		if (retV != E_OK) {
			DBG_ERR("open timer fail=%d\n\r", retV);
			return TRUE;
		}
		retV = timer_cfg(gStreamTimerID, intval, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
		if (retV != E_OK) {
			DBG_ERR("set timer fail=%d,intval=%d\n\r", retV, intval);
			return TRUE;
		}
	} else {
		if (gStreamTimerID != TIMER_NUM) {
			timer_close(gStreamTimerID);
			gStreamTimerID = TIMER_NUM;
		}
	}
	return TRUE;
}
static BOOL xExam_EnableAudSteam(unsigned char argc, char **argv)
{
	ER retV = E_OK;
	UINT32 Enable = 1;
	UINT32 intval;
	static TIMER_ID gAudTimerID = TIMER_NUM;
	UINT32 slow_rate = 1;
	//sscanf_s(strCmd, "%d %d", &Enable, &FrameRate);
	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&Enable);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%d", (int *)&slow_rate);
	}
	if (Enable) {
		uigFrameIdx = 0;
		intval = 1000000/AUDIO_CHANNEL_NUM/AUDIO_BYTE_PER_SAMPLE*AUDIO_STREAM_LEN/NVT_UI_FREQUENCY_48K;
		intval *= slow_rate;
		retV = timer_open(&gAudTimerID, xUvacTriggerAud);
		if (retV != E_OK) {
			DBG_ERR("open timer fail=%d\n\r", retV);
			return TRUE;
		}
		retV = timer_cfg(gAudTimerID, intval, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
		if (retV != E_OK) {
			DBG_ERR("set timer fail=%d,intval=%d\n\r", retV, intval);
			return TRUE;
		}
	} else {
		if (gAudTimerID != TIMER_NUM) {
			timer_close(gAudTimerID);
		}
	}
	return TRUE;
}
#if CDC_FUNC	
static BOOL xExam_WriteCdc(unsigned char argc, char **argv)
{
	UINT32 CdcChannel = CDC_COM_1ST;
	UINT32 DataSize = BULK_PACKET_SIZE;
	ER retV;
	//sscanf_s(strCmd, "%d %d", &CdcChannel, &DataSize);
	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&CdcChannel);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%d", (int *)&DataSize);
	}

	DBG_DUMP("Send data %d byte to COM_%d.\n\r", DataSize, CdcChannel);
	retV = UVAC_WriteCdcData(CdcChannel, (UINT8 *)ALIGN_CEIL(MjpgFrm[0].addr, 64), &DataSize);
	if (retV != E_OK) {
		DBG_ERR("Send data failed.\n\r");
		return TRUE;
	}
	if (DataSize % BULK_PACKET_SIZE == 0) {
		DataSize = 0;
		retV = UVAC_WriteCdcData(CdcChannel, (UINT8 *)MjpgFrm[0].addr, &DataSize);
		if (retV != E_OK) {
			DBG_ERR("Send zero packet failed.\n\r");
		}
	}
	return TRUE;
}
static BOOL xExam_WriteCdcStr(unsigned char argc, char **argv)
{
	UINT32 CdcChannel = CDC_COM_1ST;
	CHAR  String[512] = {0};
	UINT32 DataSize;
	ER retV;
	//sscanf_s(strCmd, "%d %s", &CdcChannel, &String, sizeof(String));
	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&CdcChannel);
	}
	if (argc >= 2) {
		strncpy(String, argv[1], sizeof(String)-1);
	}

	DataSize = strlen(String);

	retV = UVAC_WriteCdcData(CdcChannel, (UINT8 *)String, &DataSize);
	if (retV != E_OK) {
		DBG_ERR("Send data failed.\n\r");
		return TRUE;
	}
	if (DataSize % BULK_PACKET_SIZE == 0) {
		DataSize = 0;
		retV = UVAC_WriteCdcData(CdcChannel, (UINT8 *)MjpgFrm[0].addr, &DataSize);
		if (retV != E_OK) {
			DBG_ERR("Send zero packet failed.\n\r");
		}
	}
	return TRUE;
}
#endif
extern void _UVAC_stream_enable(UVAC_STRM_PATH stream, BOOL enable);

static BOOL xExam_SimulateHost(unsigned char argc, char **argv)
{
	UINT32 stream = UVAC_STRM_VID;
	UINT32 enable = 0;

	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&stream);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%d", (int *)&enable);
	}
	_UVAC_stream_enable((UVAC_STRM_PATH)stream, (BOOL)enable);

	return TRUE;
}
extern UINT32 m_uiU2UvacDbgIso;
extern UINT32 m_uiU2UvacDbgVid;
static BOOL xExam_UVAC_debug(unsigned char argc, char **argv)
{
	UINT32 iso_dbg = 0; //0 for vid; 1 for iso
	UINT32 value = 0;

	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&iso_dbg);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%x", (int *)&value);
	}

	if (iso_dbg) {
		m_uiU2UvacDbgIso = value;
	} else {
		m_uiU2UvacDbgVid = value;
	}
	return TRUE;
}
static SXCMD_BEGIN(m_items, "uvac")
SXCMD_ITEM("open", xExam_UVAC_open, "Open UVAC")
SXCMD_ITEM("ena %", xExam_UVAC_enable, "Enable UVAC")
SXCMD_ITEM("dis", xExam_UVAC_disable, "Disable UVAC")
SXCMD_ITEM("stream %", xExam_EnableSteam, "Enable/Disable Stream source.")
SXCMD_ITEM("vid %", xExam_EnableVidSteam, "Enable/Disable Video Stream source.")
SXCMD_ITEM("aud %", xExam_EnableAudSteam, "Enable/Disable Audio Stream source.")
#if CDC_FUNC	
SXCMD_ITEM("write %", xExam_WriteCdc, "Send data to host by CDC.")
SXCMD_ITEM("str %", xExam_WriteCdcStr, "Send string to host by CDC.")
#endif
SXCMD_ITEM("host %", xExam_SimulateHost, "Simulate Host start/stop stream")
SXCMD_ITEM("dbg %", xExam_UVAC_debug, "UVAC debug command")
SXCMD_ITEM("close", xExam_UVAC_close, "Close UVAC")
SXCMD_END()

static int uvac_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(m_items);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "test_uvac");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", m_items[loop].p_name, m_items[loop].p_desc);
	}
	return 0;
}
EXAMFUNC_ENTRY(test_uvac, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(m_items);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		uvac_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], m_items[loop].p_name, strlen(argv[1])) == 0) {
			ret = m_items[loop].p_func(argc - 2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		return -1;
	}
	return 0;
}

