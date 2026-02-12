/*
    Copyright   Novatek Microelectronics Corp. 2004.  All rights reserved.

    @file       UvacVideoTsk.c
    @ingroup    mISYSUVAC

    @brief      UVAC Video Task.
                UVAC Video Task.

*/
#include <stdio.h>
#include <string.h>
#include "timer.h"
#include "U2UvacVideoTsk.h"
#include "U2UvacVideoTsk_api.h"
#include "U2UvacReqVendor.h"
#include "U2UvacReqClass.h"
#include "UVAC.h"
#include "U2UvacDesc.h"
#include "U2UvacDbg.h"
#include "U2UvacIsoInTsk.h"
#include "U2UvacID.h"
#include "hd_gfx.h"
#include "hd_common.h"
#include "vendor_gfx.h"
#if 0
#include "U2UvacSIDCCmd.h"
#include "U2UvacPTP_DataTransfer.h"
#include "U2UvacPTP_CreateObjectList.h"
#include "U2UvacPTPDef.h"
#include "U2UvacMTP_Operation.h"
#include "U2UvacPTP_Operation.h"
#include "U2UvacPTP_Operation.h"
#include "FileSysTsk.h"
#include "U2UvacSIDCUtility.h"
#endif

#define UVC_MAKE_PAYLOAD_PERF 0


#define MAX_FRAME_SIZE_MARGIN   15 //%

#define UVAC_BUF_ALIGN  64
BOOL gU2UvacMtpEnabled = FALSE;
#if 0
SIDC_INTRCMD g_U2UvcSidcIntrCmd;
UINT32      guiU2UvcSidcBufAddr, guiU2UvcSidcBufSize;
static UINT32      g_uiUvcSidcPTPUpdateFWAddr, g_uiUvcSidcPTPUpdateFWSize;
static UINT16      gUvcSidcCardLock = 0;
#endif

typedef enum {
	UVAC_BUF_TYPE_WORK = 0,
	UVAC_BUF_TYPE_VID,
	UVAC_BUF_TYPE_VID2,
} UVAC_BUF_TYPE;

typedef struct _MEMRANGE {
	UINT32  va;
	UINT32  pa;
	UINT32  size;
} MEMRANGE, *PMEMRANGE;

static BOOL uvc_direct_trigger = TRUE;
//------ Global variable declare ------//
UINT32 gU2UacAudStart[UVAC_AUD_DEV_CNT_MAX];
UINT32 gU2UvcVidStart[UVAC_VID_DEV_CNT_MAX];
FLGPTN gUvacRunningFlag[UVAC_VID_DEV_CNT_MAX] = {0};
//If an endpoint is set to hw-payload, then it also be auto-hdr-eof.
UINT32 gU2UvcHWPayload[UVAC_VID_DEV_CNT_MAX] = {FALSE};
static BOOL   gUvacOpened = FALSE;
static UVAC_VEND_DEV_DESC *gpImgUnitUvacVendDevDesc = 0;
static UVAC_VID_STRM_INFO gUvacVidStrmInfo[UVAC_VID_DEV_CNT_MAX];
UINT32 gU2UvacChannel = UVAC_CHANNEL_1V1A;
UINT32 gU2UvcCapM3Enable = FALSE;
static UINT32 gUvcH264TBR = UVAC_H264_TBR_DEF;
static UINT32 gUvcMJPGTBR = UVAC_MJPG_TBR_DEF;
UINT32 gU2UvcStrmMinBitRate = 0x02000000;  //32M  bps =  4M  byte
UINT32 gU2UvcStrmMaxBitRate = 0x10000000;  //256M bps = 32M byte
UINT32 gU2UvcMJPGMaxTBR = UVAC_MJPG_TBR_MAX;
static UINT32 guiUvacBufAddr, guiUvacBufTotalSize, guiUvacBufAddr_pa;
static MEMRANGE gUvcVidBuf[UVAC_VID_DEV_CNT_MAX] = {0};
static UINT8  *gpUvacHSConfigDesc;
static TIMER_ID gUvacTimerID[UVAC_VID_DEV_CNT_MAX] = {TIMER_NUM};
static UVAC_VIDEO_FORMAT gUvacCodecType[UVAC_VID_DEV_CNT_MAX];
static UINT32  gUVCHeadPTS = 0x200000;//0x61D4EB38;
static UINT32  gUVCHeadSCR = 0x3AB000;//0x63EC62E8;
static UINT16  gSOFTokenNum = 0x0001;
UINT32  gU2UVCIsoinTxfUnitSize[UVAC_VID_DEV_CNT_MAX] = {0};  //must be > PAYLOAD_LEN
BOOL gU2UvacWinIntrfEnable = FALSE;
UINT32 gU2UVcTrIgEnabled = 0;
UINT8 gU2UvacAudFmtType = UAC_FMT_TYPE_I;
UVAC_VIDEO_FORMAT_TYPE gU2UvcVideoFmtType = UVAC_VIDEO_FORMAT_H264_MJPEG;
BOOL gU2UvacCdcEnabled = FALSE;
USB_EP_BLKNUM gU2UvcIsoInBandWidth = BLKNUM_SINGLE;
UINT32 gU2UvcIsoInHsPacketSize = UVC_ISOIN_HS_PACKET_SIZE;
static UINT32 gUvcMaxVideoFmtSize = UVAC_MAX_PAYLOAD_FRAME_SIZE;
UINT16 gUacMaxPacketSize = 0;
UINT8 gUacInterval = 0;
UINT32 gU2UacChNum = UAC_NUM_PHYSICAL_CHANNEL;
UINT32 gU2UacITOutChCfg = UAC_IT_OUT_CHANNEL_CONFIG;
UVAC_CDC_PSTN_REQUEST_CB gfpU2CdcPstnReqCB = NULL;
static UVAC_STARTVIDEOCB  g_fpStartVideo = NULL;
static UVAC_STOPVIDEOCB   g_fpStopVideo = NULL;
static UVAC_STARTAUDIOCB  g_fpStartAudio = NULL;
static UINT32 gToggleFid[UVAC_VID_DEV_CNT_MAX] = {0};
static MEM_RANGE gVidTxBuf[UVAC_VID_DEV_CNT_MAX] = {0};
//static MEM_RANGE gH264Header[UVAC_VID_DEV_CNT_MAX] = {0};
static UVAC_STRM_INFO  gStrmInfo[UVAC_VID_DEV_CNT_MAX] = {0};
static UVAC_STRM_INFO  gAudStrmInfo[UVAC_AUD_DEV_CNT_MAX] = {0};
static USB_MNG gUSBMng;
UVAC_HWCOPYCB g_fpU2UvacHWCopyCB = NULL;
UVAC_VID_RESO_ARY gUvcYuvFrmInfo = {0};
UVAC_VID_RESO_ARY gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_MAX] = {0, NULL, UVC_VSFMT_DEF_FRAMEINDEX};
UVAC_VID_RESO_ARY gUvcH264FrmInfo[UVAC_VID_DEV_CNT_MAX] = {0, NULL, UVC_VSFMT_DEF_FRAMEINDEX};
BOOL gDisableUac = FALSE;
TIMER_ID gUacTimerID[UVAC_AUD_DEV_CNT_MAX] = {TIMER_NUM};
static UINT32 user_data_size = 0;
UINT32 ct_controls = UVC_IT_CONTROLS;
UINT32 pu_controls = UVC_PU_CONTROLS;
////////////////////////////////////////////////////////////////////////////////
//UAC out
static MEMRANGE gAudRxBuf[1] = {0};
BOOL gU2UvacUacRxEnabled = FALSE;
static UINT32 gU2UvacUacBlockSize = 4096;
static UINT32 gU2UvacUacBufSize = UVAC_AUD_RX_BUF_SIZE;
static UVAC_AUD_RAWQ uac_raw_que = {0};

void _U2UVAC_init_queue(void);
BOOL _U2UVAC_GetRaw(UINT32 *addr, UINT32 size);
void _U2UVAC_unlock_pullqueue(UVAC_STRM_PATH path);
BOOL _U2UVAC_put_pullque(UINT32 addr, UINT32 size);
BOOL _U2UVAC_release_all_pullque(UVAC_STRM_PATH path);
////////////////////////////////////////////////////////////////////////////////

extern UVAC_WINUSBCLSREQCB gU2UvcWinUSBReqCB;
extern UINT8 gU2UvacIntfIdx_WinUsb;
extern UINT8 gU2UvacIntfIdx_VC[UVAC_VID_DEV_CNT_MAX];
extern UINT8 gU2UvacIntfIdx_VS[UVAC_VID_DEV_CNT_MAX];
extern UINT8 gU2UvacIntfIdx_AC[UVAC_AUD_DEV_CNT_MAX];
extern UINT8 gU2UvacIntfIdx_AS[UVAC_AUD_DEV_CNT_MAX];
extern UINT8 gU2UvacIntfIdx_vidAryCurrIdx; //one video control map single video stream
extern UINT8 gU2UvacIntfIdx_audAryCurrIdx; //one audio control map single audio stream
extern UVAC_SETVOLCB gU2UacSetVolCB;

extern UVAC_UNIT_CB g_fpUvcCT_CB;
extern UVAC_UNIT_CB g_fpUvcPU_CB;
extern UVAC_UNIT_CB g_fpUvcXU_CB;
UVAC_DFU_INFO g_dfu_info = {0};

UVAC_HID_INFO g_hid_info = {0};
extern UVAC_EU_DESC eu_desc[2];

UVAC_MSDC_INFO g_msdc_info = {0};
USB_GENERIC_CB g_u2uvac_msdc_device_evt_cb = NULL;
FP g_u2uvac_msdc_class_req_cb = NULL;

#define FREE_STRING_INDEX_BEGIN 4
UINT8 free_string_idx = FREE_STRING_INDEX_BEGIN;
UINT8 uvc_string_idx = 0;
UINT8 uac_string_idx = 0;
UINT8 dfu_string_idx = 0;
UINT8 hid_string_idx = 0;
USB_STRING_DESC *p_uvc_string_desc = NULL;
USB_STRING_DESC *p_uac_string_desc = NULL;

#if (ISF_AUDIO_LATENCY_DEBUG)
extern void U2UVAC_DbgDmp_Aud_TimestampNumReset(void);
#endif
#define loc_cpu() wai_sem(SEMID_U2UVC_QUEUE)
#define unl_cpu() sig_sem(SEMID_U2UVC_QUEUE)

#if 0
static void DumpMem(UINT32 Addr, UINT32 Size, UINT32 Alignment)
{
	UINT32 i;
	UINT8 *pBuf = (UINT8 *)Addr;
	for (i = 0; i < Size; i++) {
		if (i > 0 && i % Alignment == 0) {
			DBG_DUMP("\r\n");
		}
		DBG_DUMP("0x%02X ", *(pBuf + i));
	}
	DBG_DUMP("\r\n");
}
#endif

//Discrete fps: fps must be in a decreasing order and 0 shall be put to the last one.
#if 0
UINT32 gUvcVidResoCnt = UVC_VSFMT_FRAMENUM1_HD;
UVAC_VID_RESO gUvcVidResoAry[UVAC_VID_RESO_MAX_CNT] = {
	{USB_UVC_HD_WIDTH, USB_UVC_HD_HEIGHT, 3, UVC_FRMRATE_20, UVC_FRMRATE_10, UVC_FRMRATE_5 },
	{USB_UVC_1024x768_WIDTH, USB_UVC_1024x768_HEIGHT, 3, UVC_FRMRATE_30, UVC_FRMRATE_15, UVC_FRMRATE_8 },
	{USB_UVC_VGA_WIDTH, USB_UVC_VGA_HEIGHT, 3, UVC_FRMRATE_30, UVC_FRMRATE_20, UVC_FRMRATE_10},
	{USB_UVC_QVGA_WIDTH, USB_UVC_QVGA_HEIGHT, 3, UVC_FRMRATE_30, UVC_FRMRATE_25, UVC_FRMRATE_12},
	{USB_UVC_1024x576_WIDTH, USB_UVC_1024x576_HEIGHT, 3, UVC_FRMRATE_60, UVC_FRMRATE_30, UVC_FRMRATE_15},
	{USB_UVC_800x600_WIDTH, USB_UVC_800x600_HEIGHT, 3, UVC_FRMRATE_60, UVC_FRMRATE_30, UVC_FRMRATE_20},
	{USB_UVC_800x480_WIDTH, USB_UVC_800x480_HEIGHT, 2, UVC_FRMRATE_60, UVC_FRMRATE_25, 0},
	{USB_UVC_FULL_HD_WIDTH, USB_UVC_FULL_HD_HEIGHT, 2, UVC_FRMRATE_60, UVC_FRMRATE_30, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0}
};
#else
UINT32 gU2UvcVidResoCnt = UVAC_VID_RESO_INTERNAL_TBL_CNT;
UVAC_VID_RESO gU2UvcVidResoAry[UVAC_VID_RESO_MAX_CNT] = {
	{1920,  1080,   1,      UVC_FRMRATE_30,      0,      0},
	{1280,   720,   1,      UVC_FRMRATE_30,      0,      0},
	{ 848,   480,   1,      UVC_FRMRATE_30,      0,      0},
	{ 640,   480,   1,      UVC_FRMRATE_30,      0,      0},
	{ 320,   240,   1,      UVC_FRMRATE_30,      0,      0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0}
};
#endif
static UVAC_PARAM gUvcParamAry[UVAC_VID_DEV_CNT_MAX] = {
	{UVC_VSFMT_DEF_FRM_IDX, UVC_FRMRATE_30},
	{UVC_VSFMT_DEF_FRM_IDX, UVC_FRMRATE_30}
};
UINT32 gU2UvacAudSampleRate[UVAC_AUD_SAMPLE_RATE_MAX_CNT] = {
	UAC_FREQUENCY_16K,
	0,
	0,
};


//====== extern variable ======
extern UINT32 gU2UacSampleRate;
extern UINT32 gU2UvcMaxTxfSizeForUsb[UVAC_TXF_QUE_MAX];
extern const USB_EP U2UVAC_USB_EP[UVAC_TXF_QUE_MAX];
extern const USB_EP U2UVAC_USB_RX_EP[UVAC_RXF_QUE_MAX];
extern UINT32 gU2UvcUsbDMAAbord;
extern UINT32 gU2UacUsbDMAAbord;
extern UINT32 gU2UvcNoVidStrm;
extern UVAC_EUVENDCMDCB gU2UvacEUVendCmdCB[UVAC_EU_VENDCMD_CNT];

UINT32 gU2UvcCapImgAddr = 0;
UINT32 gU2UvcCapImgSize = 0;

extern UVC_STILL_PROBE_COMMIT gU2UvcStillProbeCommit;
extern UINT8 gU2UvacStillImgTrigSts;

extern UINT32 gUvacUvcVer;

/**
    \addtogroup mISYSUVAC
@{
*/

extern UINT32 guiVideoBufAddr, FirstSize;

static UINT32 uvac_va_to_pa(UVAC_BUF_TYPE type, UINT32 va)
{
	UINT32 pa = 0;
	if (type == UVAC_BUF_TYPE_WORK) {
		if (va >= guiUvacBufAddr) {
			pa = guiUvacBufAddr_pa + (va - guiUvacBufAddr);
			if (pa <= (guiUvacBufAddr_pa + guiUvacBufTotalSize)) {
				//DBG_DUMP("work va=0x%X -> pa=0x%X", va, pa);
				return pa;
			}
		}
	} else if (type == UVAC_BUF_TYPE_VID){
		if (gUvcVidBuf[0].pa == 0) {
			return va;
		} else if (va >= gUvcVidBuf[0].va) {
			pa = gUvcVidBuf[0].pa + (va - gUvcVidBuf[0].va);
			if (pa <= (gUvcVidBuf[0].pa + gUvcVidBuf[0].size)) {
				//DBG_DUMP("vid1 va=0x%X -> pa=0x%X", va, pa);
				return pa;
			}
		}
	} else {
		if (gUvcVidBuf[1].pa == 0) {
			return va;
		} else if (va >= gUvcVidBuf[1].va) {
			pa = gUvcVidBuf[1].pa + (va - gUvcVidBuf[1].va);
			if (pa <= (gUvcVidBuf[1].pa + gUvcVidBuf[1].size)) {
				//DBG_DUMP("vid2 va=0x%X -> pa=0x%X", va, pa);
				return pa;
			}
		}
	}
	DBG_ERR("invalid value type=%d, va=0x%X, pa=0x%X\r\n", type, va, pa);
	return pa;
}
static UINT32 uvac_pa_to_va(UVAC_BUF_TYPE type, UINT32 pa)
{
	UINT32 va = 0;
	if (type == UVAC_BUF_TYPE_WORK) {
		if (pa >= guiUvacBufAddr_pa) {
			va = guiUvacBufAddr + (pa - guiUvacBufAddr_pa);
			if (va <= (guiUvacBufAddr + guiUvacBufTotalSize)) {
				//DBG_DUMP("work pa=0x%X -> va=0x%X", pa, va);
				return va;
			}
		}
	} else if (type == UVAC_BUF_TYPE_VID){
		if (gUvcVidBuf[0].va == 0) {
			return pa;
		} else if (pa >= gUvcVidBuf[0].pa) {
			va = gUvcVidBuf[0].va + (pa - gUvcVidBuf[0].pa);
			if (va <= (gUvcVidBuf[0].va + gUvcVidBuf[0].size)) {
				//DBG_DUMP("vid1 pa=0x%X -> va=0x%X", pa, va);
				return va;
			}
		}
	} else {
		if (gUvcVidBuf[1].va == 0) {
			return pa;
		} else if (pa >= gUvcVidBuf[1].pa) {
			va = gUvcVidBuf[1].va + (pa - gUvcVidBuf[1].pa);
			if (va <= (gUvcVidBuf[1].va + gUvcVidBuf[1].size)) {
				//DBG_DUMP("vid2 pa=0x%X -> va=0x%X", pa, va);
				return va;
			}
		}
	}
	DBG_ERR("invalid value type=%d, va=0x%X, pa=0x%X\r\n", type, va, pa);
	return va;
}
static ER hwmem_open(void)
{
#if 0
	HD_RESULT    ret;
    ret = hd_gfx_init();
    if(ret != HD_OK) {
        DBG_ERR("open fail=%d\n", ret);
        return E_SYS;
    }
#endif
	return E_OK;
}

static ER hwmem_close(void)
{
#if 0
	HD_RESULT    ret;
    ret = hd_gfx_uninit();
    if(ret != HD_OK) {
        DBG_ERR("close fail=%d\n", ret);
        return E_SYS;
    }
#endif
	return E_OK;
}
static void hwmem_memcpy(UVAC_VID_DEV_CNT  thisVidIdx, UINT32 uiDst, UINT32 uiSrc, UINT32 uiSize)
{
	//hd_gfx_memcpy(uiDst, uiSrc, uiSize);
	if (g_fpU2UvacHWCopyCB) {
		UINT32 dst, src;
		dst = uvac_va_to_pa(UVAC_BUF_TYPE_WORK, uiDst);
		if (thisVidIdx == UVAC_VID_DEV_CNT_1) {
			src = uvac_va_to_pa(UVAC_BUF_TYPE_VID, uiSrc);
		} else {
			src = uvac_va_to_pa(UVAC_BUF_TYPE_VID2, uiSrc);
		}
		g_fpU2UvacHWCopyCB(dst, src, uiSize);
	} else {
		memcpy((void *)uiDst, (void *)uiSrc, uiSize);
		hd_common_mem_flush_cache((void *) uiDst, uiSize);
	}
}
//====== debug ======
#if (_UVC_DBG_LVL_ > _UVC_DBG_NONE_)
static void UVAC_DbgDmp_StrDesc(UVAC_STRING_DESC *pStrDesc)
{
	UINT32 len = 0;
	UINT8 *ptr = 0;
	if (pStrDesc) {
		len = pStrDesc->bLength;
		len = (len > 2) ? (len - 2) : 0;
		DBG_DUMP("\r\nStrDesc: 0x%x, 0x%x\r\n", len, pStrDesc->bDescriptorType);
		ptr = pStrDesc->bString;
		while (len--) {
			DBG_DUMP("  %c", *ptr++);
		}
	} else {
		DBG_DUMP("\r\nStrDesc NULL\r\n");
	}
	DBG_DUMP("\r\n");
}
static void UVAC_DbgDmp_VendDevDesc(PUVAC_VEND_DEV_DESC pVendDevDesc)
{
	DBG_DUMP("%s ==>\r\n", __func__);
	if (0 == pVendDevDesc) {
		DBG_ERR("Input NULL\r\n");
		return;
	}
	DBG_DUMP("VID=0x%x, PID=0x%x\r\n", pVendDevDesc->VID, pVendDevDesc->PID);
	UVAC_DbgDmp_StrDesc(pVendDevDesc->pManuStringDesc);
	UVAC_DbgDmp_StrDesc(pVendDevDesc->pProdStringDesc);
	UVAC_DbgDmp_StrDesc(pVendDevDesc->pSerialStringDesc);
	DBG_DUMP("\r\nfpIQVendReqCB         =0x%x\r\n", pVendDevDesc->fpIQVendReqCB);
	DBG_DUMP("fpVendReqCB           =0x%x\r\n", pVendDevDesc->fpVendReqCB);
	DBG_DUMP("<===========\r\n");
}
#else
static void UVAC_DbgDmp_StrDesc(UVAC_STRING_DESC *pStrDesc) {}
static void UVAC_DbgDmp_VendDevDesc(PUVAC_VEND_DEV_DESC pVendDevDesc) {}
#endif

#if (_UVC_DBG_LVL_ > _UVC_DBG_NONE_)
static void UVAC_DbgDmp_STRM_INFO(UVAC_STRM_INFO *pStrmInfo)
{
	DBG_DUMP("%s ==>\r\n", __func__);
	DBG_DUMP("pStrmHdr      =0x%x\r\n", pStrmInfo->pStrmHdr);
	DBG_DUMP("strmHdrSize   =0x%x\r\n", pStrmInfo->strmHdrSize);
	DBG_DUMP("strmPath      =0x%x\r\n", pStrmInfo->strmPath);
	DBG_DUMP("strmWidth     =0x%x\r\n", pStrmInfo->strmWidth);
	DBG_DUMP("strmHeight    =0x%x\r\n", pStrmInfo->strmHeight);
	DBG_DUMP("strmFps       =0x%x\r\n", pStrmInfo->strmFps);
	DBG_DUMP("strmCodec     =0x%x\r\n", pStrmInfo->strmCodec);
	DBG_DUMP("<===========\r\n");
}
static void UVAC_DbgDmp_UvacInfo(UVAC_INFO *pClassInfo)
{
	DBG_DUMP("%s ==>\r\n", __func__);
	if (0 == pClassInfo) {
		DBG_ERR("Input NULL\r\n");
		return;
	}
	DBG_DUMP("UvacMemAdr=0x%x\r\n", pClassInfo->UvacMemAdr);
	DBG_DUMP("UvacMemSize=0x%x\r\n", pClassInfo->UvacMemSize);
	DBG_DUMP("hwPayload=0x%x, 0x%x\r\n", pClassInfo->hwPayload[0], pClassInfo->hwPayload[1]);
	DBG_DUMP("channel=0x%x\r\n", pClassInfo->channel);
	UVAC_DbgDmp_STRM_INFO(&(pClassInfo->strmInfo));
	DBG_DUMP("fpStartVideoCB=0x%x\r\n", pClassInfo->fpStartVideoCB);
	DBG_DUMP("fpStopVideoCB=0x%x\r\n", pClassInfo->fpStopVideoCB);

	DBG_DUMP("fpVendReqCB           =0x%x\r\n", g_fpU2UvacVendorReqCB);
	DBG_DUMP("fpIQVendReqCB         =0x%x\r\n", g_fpU2UvacVendorReqIQCB);
	DBG_DUMP("<===========\r\n");
}
static void UVAC_DbgDmp_MemLayout(void)
{
	UINT32 tmpIdx = 0;
	DbgMsg_UVC(("%s:sAdr=0x%x,size=0x%x,desc=0x%x\r\n", __func__, guiUvacBufAddr, guiUvacBufTotalSize, gpUvacHSConfigDesc));
	for (tmpIdx = 0; tmpIdx < UVAC_VID_DEV_CNT_MAX; tmpIdx++) {
		DbgMsg_UVC(("gVidTxBuf[%d]=0x%x,Size=0x%x\r\n", tmpIdx, gVidTxBuf[tmpIdx].addr, gVidTxBuf[tmpIdx].size));
	}
}
static void UVAC_DbgDmp_EU_INFO(UVAC_EU_DESC *p_eu_desc)
{
	UINT32 i;
	DBG_DUMP("== Extension Unit Descriptor ==\r\n");
	DBG_DUMP("bLength       =%d\r\n", p_eu_desc->bLength);
	DBG_DUMP("bDescriptorType       =0x%X\r\n", p_eu_desc->bDescriptorType);
	DBG_DUMP("bDescriptorSubtype       =0x%X\r\n", p_eu_desc->bDescriptorSubtype);
	DBG_DUMP("bUnitID       =%d\r\n", p_eu_desc->bUnitID);

	DBG_DUMP("GUID=");
	for (i = 0; i < 16; i++) {
		DBG_DUMP("%X ", p_eu_desc->guidExtensionCode[i]);
	}
	DBG_DUMP("\r\n");

	DBG_DUMP("bNumControls       =%d\r\n", p_eu_desc->bNumControls);
	DBG_DUMP("bNrInPins       =%d\r\n", p_eu_desc->bNrInPins);

	DBG_DUMP("baSourceID=");
	for (i = 0; i < p_eu_desc->bNrInPins; i++) {
		DBG_DUMP("%d ", p_eu_desc->baSourceID[i]);
	}
	DBG_DUMP("\r\n");

	DBG_DUMP("bControlSize       =%d\r\n", p_eu_desc->bControlSize);

	DBG_DUMP("bmControls=");
	for (i = 0; i < p_eu_desc->bControlSize; i++) {
		DBG_DUMP("%d ", p_eu_desc->bmControls[i]);
	}
	DBG_DUMP("\r\n");
	DBG_DUMP("iExtension       =%d\r\n", p_eu_desc->iExtension);
}
static void UVAC_DbgDmp_HID_INFO(UVAC_HID_INFO *p_hid_info)
{
	DBG_DUMP("--== HID info ==--\r\n");
	DBG_DUMP("en=%d, cb=0x%X\r\n", p_hid_info->en, (UINT32)p_hid_info->cb);
	DBG_DUMP("bLength =%d\r\n", p_hid_info->hid_desc.bLength);
	DBG_DUMP("bHidDescType  =0x%X\r\n", p_hid_info->hid_desc.bHidDescType);
	DBG_DUMP("bcdHID  =0x%X\r\n", p_hid_info->hid_desc.bcdHID);
	DBG_DUMP("bCountryCode  =0x%X\r\n", p_hid_info->hid_desc.bCountryCode);
	DBG_DUMP("bNumDescriptors  =%d\r\n", p_hid_info->hid_desc.bNumDescriptors);
	DBG_DUMP("bDescriptorType  =0x%X\r\n", p_hid_info->hid_desc.bDescriptorType);
	DBG_DUMP("wDescriptorLength  =%d\r\n", p_hid_info->hid_desc.wDescriptorLength);

	if (p_hid_info->hid_desc.bNumDescriptors > 1) {
		UINT8 i;
		UINT8 *p_temp = p_hid_info->hid_desc.p_desc;

		for (i = 0; i < p_hid_info->hid_desc.bNumDescriptors - 1; i++) {
			DBG_DUMP("bDescriptorType  =0x%X\r\n", *p_temp++);
			DBG_DUMP("wDescriptorLength  =%d\r\n", *p_temp + *(p_temp+1)*256);
			p_temp+=2;
		}
	}
}
#ifdef __LINUX_USER__
static void UVAC_DbgDmp_MSDC_INFO(UVAC_MSDC_INFO *p_msdc_info)
{
	DBG_DUMP("--== MSDC info ==--\r\n");
	DBG_DUMP("en=%d\r\n", p_msdc_info->en);
}
#endif
#else
static __inline void UVAC_DbgDmp_STRM_INFO(UVAC_STRM_INFO *pStrmInfo) {}
static __inline void UVAC_DbgDmp_UvacInfo(UVAC_INFO *pClassInfo) {}
static __inline void UVAC_DbgDmp_MemLayout(void) {}
static __inline void UVAC_DbgDmp_EU_INFO(UVAC_EU_DESC *p_eu_desc) {}
static __inline void UVAC_DbgDmp_HID_INFO(UVAC_HID_INFO *p_hid_info) {}
static __inline void UVAC_DbgDmp_MSDC_INFO(UVAC_MSDC_INFO *p_msdc_info) {}
#endif

static BOOL _is_iframe(UINT8 *pVidBuf)
{
	UINT8 value;

	value = pVidBuf[UVAC_VID_I_FRM_MARK_POS + user_data_size]&0x1F;

	// (BYTE & 0x1F)  = 5 (I-frame)  , 7(SPS)  ,  8(PPS) , 1(P-frame)
	if (value == 5 || value == 7) {
		return TRUE;
	} else {
		return FALSE;
	}
}
//====== internal function ======
static __inline void UVAC_SetupMem(void)
{
	DbgMsg_UVC(("Mem-Addr=0x%x(pa=0x%X), Size=0x%x, Desc=0x%x +\r\n", guiUvacBufAddr, guiUvacBufAddr_pa, guiUvacBufTotalSize, gpUvacHSConfigDesc));
	gpUvacHSConfigDesc = (UINT8 *)guiUvacBufAddr;
	memset((void *) guiUvacBufAddr, 0, UVAC_MEM_DESC_SIZE);

	gVidTxBuf[UVAC_VID_DEV_CNT_1].addr = ALIGN_CEIL(guiUvacBufAddr + UVAC_MEM_DESC_SIZE, UVAC_BUF_ALIGN);
	gVidTxBuf[UVAC_VID_DEV_CNT_1].size = ALIGN_FLOOR(gUvcMaxVideoFmtSize, UVAC_BUF_ALIGN);
	hd_common_mem_flush_cache((void *) gVidTxBuf[UVAC_VID_DEV_CNT_1].addr, gVidTxBuf[UVAC_VID_DEV_CNT_1].size);
	if (gU2UvacChannel > UVAC_CHANNEL_1V1A) {
		gVidTxBuf[UVAC_VID_DEV_CNT_2].addr = ALIGN_CEIL(gVidTxBuf[UVAC_VID_DEV_CNT_1].addr + gVidTxBuf[UVAC_VID_DEV_CNT_1].size, UVAC_BUF_ALIGN);
		gVidTxBuf[UVAC_VID_DEV_CNT_2].size = ALIGN_FLOOR(gUvcMaxVideoFmtSize, UVAC_BUF_ALIGN);
		hd_common_mem_flush_cache((void *) gVidTxBuf[UVAC_VID_DEV_CNT_2].addr, gVidTxBuf[UVAC_VID_DEV_CNT_2].size);
	}

	if (gU2UvacUacRxEnabled) {
		gAudRxBuf[0].va = ALIGN_CEIL(gVidTxBuf[UVAC_VID_DEV_CNT_1].addr + gVidTxBuf[UVAC_VID_DEV_CNT_1].size, UVAC_BUF_ALIGN);
		gAudRxBuf[0].pa = uvac_va_to_pa(UVAC_BUF_TYPE_WORK, gAudRxBuf[0].va);
		gAudRxBuf[0].size = ALIGN_FLOOR(gU2UvacUacBufSize, UVAC_BUF_ALIGN);
		hd_common_mem_flush_cache((void *) gAudRxBuf[0].va, gAudRxBuf[0].size);

		memset((void *)gAudRxBuf[0].va, 0, gAudRxBuf[0].size);
		vos_cpu_dcache_sync((VOS_ADDR)(gAudRxBuf[0].va), gAudRxBuf[0].size, VOS_DMA_BIDIRECTIONAL);
	}
}

/*
    UVAC_GetImageSize
*/
static void UVAC_GetImageSize(UVAC_VIDEO_FORMAT codec_type, UINT32 frm_idx, UINT32 vidDevIdx, UINT32 *pOutWidth, UINT32 *pOutHeight)
{
	UINT32 tmpIdx = frm_idx - 1;
	UVAC_VID_RESO_ARY *p_frm_info;
	DbgMsg_UVC(("GetImageSize:idx=%d, %d\r\n", frm_idx, tmpIdx));

	if (codec_type == UVAC_VIDEO_FORMAT_YUV) {
		p_frm_info = &gUvcYuvFrmInfo;
	} else if (codec_type == UVAC_VIDEO_FORMAT_MJPG) {
		p_frm_info = &gUvcMjpgFrmInfo[vidDevIdx];
	} else if (codec_type == UVAC_VIDEO_FORMAT_H264) {
		p_frm_info = &gUvcH264FrmInfo[vidDevIdx];
	} else {
		DBG_ERR("unknow codec type(%d)\r\n", codec_type);
		return;
	}

	if (frm_idx && (p_frm_info->aryCnt>= frm_idx)) {
		*pOutWidth = p_frm_info->pVidResAry[tmpIdx].width;
		*pOutHeight = p_frm_info->pVidResAry[tmpIdx].height;
	} else {
		DBG_ERR("Frame_Index=%d, num=%d\r\n", frm_idx, p_frm_info->aryCnt);
	}

	DbgMsg_UVC(("GetImageSize[%d]=%d, %d, codec type =%d\r\n", tmpIdx, *pOutWidth, *pOutHeight, codec_type));
}
static UINT16 UVAC_SetIntfString(void)
{
	if ((p_uvc_string_desc != NULL) && (p_uvc_string_desc->b_descriptor_type == 0x03)) {
		uvc_string_idx = free_string_idx++;
		gUSBMng.p_string_desc[uvc_string_idx] = (USB_STRING_DESC *)p_uvc_string_desc;
		UVAC_DbgDmp_StrDesc((UVAC_STRING_DESC *)gUSBMng.p_string_desc[uvc_string_idx]);
	}
	if ((p_uac_string_desc != NULL) && (p_uac_string_desc->b_descriptor_type == 0x03)) {
		uac_string_idx = free_string_idx++;
		gUSBMng.p_string_desc[uac_string_idx] = (USB_STRING_DESC *)p_uac_string_desc;
		UVAC_DbgDmp_StrDesc((UVAC_STRING_DESC *)gUSBMng.p_string_desc[uac_string_idx]);
	}

	if ((g_dfu_info.p_vendor_string != NULL) && (g_dfu_info.p_vendor_string->bDescriptorType == 0x03)) {
		dfu_string_idx = free_string_idx++;
		gUSBMng.p_string_desc[dfu_string_idx] = (USB_STRING_DESC *)(g_dfu_info.p_vendor_string);
		UVAC_DbgDmp_StrDesc((UVAC_STRING_DESC *)gUSBMng.p_string_desc[dfu_string_idx]);
	}

	if ((g_hid_info.p_vendor_string != NULL) && (g_hid_info.p_vendor_string->bDescriptorType == 0x03)) {
		hid_string_idx = free_string_idx++;
		gUSBMng.p_string_desc[hid_string_idx] = (USB_STRING_DESC *)(g_hid_info.p_vendor_string);
		UVAC_DbgDmp_StrDesc((UVAC_STRING_DESC *)gUSBMng.p_string_desc[hid_string_idx]);
	}
	return free_string_idx;
}
static __inline void UVAC_ResetParam(void)
{
	UINT32 i;
	UINT32 uiOutWidth = 0, uiOutHeight = 0;
	DBG_IND("++\r\n");

	guiUvacBufAddr = guiUvacBufAddr_pa = guiUvacBufTotalSize = 0;
	gpUvacHSConfigDesc = 0;
	for (i = 0; i < UVAC_VID_DEV_CNT_MAX; i++) {
		memset((void *)&gUvacVidStrmInfo[i], 0, sizeof(UVAC_VID_STRM_INFO));
		gUvacCodecType[i] = UVAC_VIDEO_FORMAT_H264;

		UVAC_GetImageSize(gUvacCodecType[i], gUvcParamAry[i].OutSizeID, i, &uiOutWidth, &uiOutHeight);
		gUvacVidStrmInfo[i].strmInfo.strmWidth = uiOutWidth;
		gUvacVidStrmInfo[i].strmInfo.strmHeight = uiOutHeight;
		gUvacVidStrmInfo[i].strmInfo.strmFps = 30;
		gUvacVidStrmInfo[i].strmInfo.strmCodec = gUvacCodecType[i];
		if (UVAC_VID_DEV_CNT_1 == i) {
			gUvacVidStrmInfo[i].strmInfo.strmPath = UVAC_STRM_VID;
		} else if (UVAC_VID_DEV_CNT_2 == i) {
			gUvacVidStrmInfo[i].strmInfo.strmPath = UVAC_STRM_VID2;
		} else {
			DBG_ERR("Unknown stream-path: %d\r\n", i);
		}

		gU2UvcVidStart[i] = FALSE;
		gUvacTimerID[i] = TIMER_NUM;
		gUvacRunningFlag[i] = 0;
		gU2UvacIntfIdx_VC[i] = UVAC_INTF_IDX_UNKNOWN;
		gU2UvacIntfIdx_VS[i] = UVAC_INTF_IDX_UNKNOWN;
		gVidTxBuf[i].addr = 0;
		gVidTxBuf[i].size = 0;

	}
	for (i = 0; i < UVAC_AUD_DEV_CNT_MAX; i++) {
		gU2UacAudStart[i] = FALSE;
		gU2UvacIntfIdx_AC[i] = UVAC_INTF_IDX_UNKNOWN;
		gU2UvacIntfIdx_AS[i] = UVAC_INTF_IDX_UNKNOWN;
	}
	gU2UVcTrIgEnabled = 0; //remove-able?
	gU2UvacIntfIdx_WinUsb = UVAC_INTF_IDX_UNKNOWN;
	gU2UvacIntfIdx_vidAryCurrIdx = 0;
	gU2UvacIntfIdx_audAryCurrIdx = 0;
	gU2UacSampleRate = gU2UvacAudSampleRate[0];
	gU2UvcNoVidStrm = FALSE;
	DBG_IND("--gU2UacSampleRate=%d\r\n", gU2UacSampleRate);
}
static ER UVC_TimeTrigSet(UINT32 intrVal, UINT32 idx)
{
	ER retV = E_OK;
	UINT32 intval = intrVal;
	DbgMsg_UVC(("++TimeTrig:idx=%d,intrVal=%d,en=%d,T1=0x%x,T2=0x%x,maxCh=%d\n\r", idx, intrVal, gU2UVcTrIgEnabled, gUvacTimerID[0], gUvacTimerID[1], gU2UvacChannel));

	//if (gU2UVcTrIgEnabled < UVAC_VID_DEV_CNT)
	if (gU2UVcTrIgEnabled < UVAC_VID_DEV_CNT_MAX) {
		gU2UVcTrIgEnabled ++;
		if (FALSE == uvc_direct_trigger) {
			if (0 == intval) {
				intval = 1000000 / UVC_FRMRATE_30;
				DbgMsg_UVC(("Time-intrVal zero,set to %d fps, %d\r\n", __func__, UVC_FRMRATE_30, intval));
			}
			if (UVAC_VID_DEV_CNT_1 == idx) {
				if (gUvacTimerID[UVAC_VID_DEV_CNT_1] != TIMER_NUM) {
					DBG_ERR("Timer1 already open:%d,%d\n\r", (int)gUvacTimerID, TIMER_NUM);
				}
				retV = timer_open(&gUvacTimerID[UVAC_VID_DEV_CNT_1], NULL);
				if (retV != E_OK) {
					DBG_ERR("open timer fail=%d\n\r", retV);
					return retV;
				}
				retV = timer_cfg(gUvacTimerID[UVAC_VID_DEV_CNT_1], intval, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
				if (retV != E_OK) {
					DBG_ERR("set timer fail=%d,intval=%d\n\r", retV, intval);
					return retV;
				}
			} else if ((gU2UvacChannel > UVAC_CHANNEL_1V1A) && (UVAC_VID_DEV_CNT_2 == idx)) {
				if (gUvacTimerID[UVAC_VID_DEV_CNT_2] != TIMER_NUM) {
					DBG_ERR("Timer[%d] already open\n\r", UVAC_VID_DEV_CNT_2);
				}
				retV = timer_open(&gUvacTimerID[UVAC_VID_DEV_CNT_2], NULL);
				if (retV != E_OK) {
					DBG_ERR("open timer[%d] fail=%d\n\r", UVAC_VID_DEV_CNT_2, retV);
					return retV;
				}
				retV = timer_cfg(gUvacTimerID[UVAC_VID_DEV_CNT_2], intval, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
				if (retV != E_OK) {
					DBG_ERR("set timer[%d] fail=%d,intval=%d\n\r", UVAC_VID_DEV_CNT_2, retV, intval);
					return retV;
				}
			} else {
				DBG_ERR("Not Support MaxCh=%d or idx=%d\n\r", gU2UvacChannel, idx);
				return E_PAR;
			}
		}
	} else {
		DBG_ERR("Already Trig Both=%d\r\n", gU2UVcTrIgEnabled);
		return E_PAR;
	}
	DbgMsg_UVCIO(("--TimeTrig:idx=%d,intrVal=%d,en=%d,T1=0x%x,T2=0x%x,maxCh=%d\n\r", idx, intrVal, gU2UVcTrIgEnabled, gUvacTimerID[UVAC_VID_DEV_CNT_1], gUvacTimerID[UVAC_VID_DEV_CNT_2], gU2UvacChannel));

	return retV;
}

static ER UVC_TimeTrigClose(UINT32 idx)
{
	ER retV = E_OK;
	DbgMsg_UVC(("+%s:MaxCh=%d,idx=%d,cnt=%d,start=%d,%d\n\r", __func__, gU2UvacChannel, idx, gU2UVcTrIgEnabled, gU2UacAudStart[0], gU2UacAudStart[1]));

	if (gU2UVcTrIgEnabled) {
		gU2UVcTrIgEnabled --;
		if (FALSE == uvc_direct_trigger) {
			if (UVAC_VID_DEV_CNT_1 == idx) {
				DbgMsg_UVC(("MaxCh=%d,idx=%d,AudStart[0]=%d\r\n", gU2UvacChannel, idx, gU2UacAudStart[UVAC_VID_DEV_CNT_1]));
				retV = timer_close(gUvacTimerID[UVAC_VID_DEV_CNT_1]);
				gUvacTimerID[UVAC_VID_DEV_CNT_1] = TIMER_NUM;
			}
			DbgMsg_UVC(("MaxCh=%d,idx=%d,AudStart[1]=%d\r\n", gU2UvacChannel, idx, gU2UacAudStart[UVAC_VID_DEV_CNT_2]));
			//if ((UVAC_VID_DEV_CNT_2 == idx) && (FALSE == gU2UacAudStart[UVAC_VID_DEV_CNT_2]))
			if (UVAC_VID_DEV_CNT_2 == idx) {
				retV = timer_close(gUvacTimerID[UVAC_VID_DEV_CNT_2]);
				gUvacTimerID[UVAC_VID_DEV_CNT_2] = TIMER_NUM;
			}
		}
		#if 0
		if (FALSE == gU2UacAudStart[UVAC_AUD_DEV_CNT_1]) {
			U2UVAC_RemoveTxfInfo(UVAC_TXF_QUE_A1);
		}
		if (FALSE == gU2UacAudStart[UVAC_AUD_DEV_CNT_2]) {
			U2UVAC_RemoveTxfInfo(UVAC_TXF_QUE_A2);
		}
		#endif
		if (0 == gU2UVcTrIgEnabled) {
			U2UVAC_IsoInTxfStateMachine(UVC_ISOIN_TXF_ACT_STOP);
		}
	} else {
		DBG_ERR("^RTrig Already CLose\r\n");
	}

	DbgMsg_UVCIO(("-%s:MaxCh=%d,idx=%d,cnt=%d,start=%d,%d, retV=%d\n\r", __func__, gU2UvacChannel, idx, gU2UVcTrIgEnabled, gU2UacAudStart[UVAC_VID_DEV_CNT_1], gU2UacAudStart[UVAC_VID_DEV_CNT_2], retV));
	return retV;
}

/*
    Start UVAC data streaming.
*/
static int UVAC_Start(UVAC_VID_DEV_CNT vidDevIdx)
{
	DbgMsg_UVC(("^R+-%s:vidDevIdx=%d\r\n", __func__, vidDevIdx));
	gU2UvcUsbDMAAbord = FALSE;
	if (UVAC_VID_DEV_CNT_1 == vidDevIdx) {
		clr_flg(FLG_ID_U2UVAC, FLGUVAC_STOP | FLGUVAC_VIDEO_STOP);
		set_flg(FLG_ID_U2UVAC, FLGUVAC_START);
	} else if (UVAC_VID_DEV_CNT_2 == vidDevIdx) {
		clr_flg(FLG_ID_U2UVAC, FLGUVAC_STOP2 | FLGUVAC_VIDEO2_STOP);
		set_flg(FLG_ID_U2UVAC, FLGUVAC_START2);
	} else {
		DBG_ERR("Unknown VidDevIdx=%d\r\n", vidDevIdx);
	}
	if (gU2UVcTrIgEnabled) {
		DbgMsg_UVC(("^RVidStart[%d]:%d,V1=%d,%d,A1=%d,%d\r\n", vidDevIdx, gU2UVcTrIgEnabled, gU2UvcVidStart[0], gU2UvcVidStart[1], gU2UacAudStart[0], gU2UacAudStart[1]));
		set_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_TXF);
	}
	return E_OK;
}

/*
    Stop UVAC data streaming.

    @note  Important!! Important!! Important!! Important!! Important!! Important!!
         Here we assume the stop command will send after the whole image return!!
         We don't handle stop command in FDMA running!!
         If we should support that case, we should stop/flush FDMA channel-B/USB code.

*/
static int UVAC_StopAll(void)
{
	FLGPTN uiFlag;

	uiFlag = kchk_flg(FLG_ID_U2UVAC, (FLGUVAC_START | FLGUVAC_START2));
	DbgMsg_UVC(("^R+%s:0x%x\r\n", __func__, uiFlag));
	//#NT#2017/05/04#Ben Wang -begin
	//#NT#Fix that UVC_MULTIMEDIA_FUNC with MJPEG will hang up in UVAC_StopAll().
	if (uiFlag & (FLGUVAC_START | FLGUVAC_START2)) {
		FLGPTN uiWaitReadyFlg = 0;
		if (uiFlag & FLGUVAC_START) {
			uiWaitReadyFlg |= FLGUVAC_RDY;
		}
		if (uiFlag & FLGUVAC_START2) {
			uiWaitReadyFlg |= FLGUVAC_RDY2;
		}
		gU2UvcNoVidStrm = TRUE;
		clr_flg(FLG_ID_U2UVAC, (FLGUVAC_START | FLGUVAC_START2));
		set_flg(FLG_ID_U2UVAC, (FLGUVAC_STOP | FLGUVAC_STOP2));
		if (uvc_direct_trigger) {
			clr_flg(FLG_ID_U2UVAC, (FLGUVAC_VIDEO_TXF | FLGUVAC_VIDEO2_TXF));
			set_flg(FLG_ID_U2UVAC, (FLGUVAC_VIDEO_STOP | FLGUVAC_VIDEO2_STOP));
		}
		gUvacRunningFlag[UVAC_VID_DEV_CNT_1] |= FLGUVAC_STOP;
		gUvacRunningFlag[UVAC_VID_DEV_CNT_2] |= FLGUVAC_STOP2;

		vos_flag_set(FLG_ID_U2UVAC_FRM, (FLGUVAC_FRM_V1 | FLGUVAC_FRM_V2 | FLGUVAC_FRM_A1 | FLGUVAC_FRM_A2));

		DbgMsg_UVC(("+UVAC STOP All \r\n"));
		wai_flg(&uiFlag, FLG_ID_U2UVAC, uiWaitReadyFlg, TWF_ANDW);
		DbgMsg_UVC(("-UVAC STOP All \r\n"));
		return E_OK;
	}
	//#NT#2017/05/04#Ben Wang -end
	DbgMsg_UVC(("^R-%s\r\n", __func__));
	return E_SYS;

}
static int _UVC_Stop(UVAC_VID_DEV_CNT vidDevIdx)
{
	FLGPTN uiFlag = 0, chkFlag = 0, setFlag = 0, rdyFlag = 0;

	if (UVAC_VID_DEV_CNT_1 == vidDevIdx) {
		chkFlag = FLGUVAC_START;
		setFlag = FLGUVAC_STOP;
		rdyFlag = FLGUVAC_RDY;
		if (uvc_direct_trigger) {
			setFlag |= FLGUVAC_VIDEO_STOP;
		}
	} else if (UVAC_VID_DEV_CNT_2 == vidDevIdx) {
		chkFlag = FLGUVAC_START2;
		setFlag = FLGUVAC_STOP2;
		rdyFlag = FLGUVAC_RDY2;
		if (uvc_direct_trigger) {
			setFlag |= FLGUVAC_VIDEO2_STOP;
		}
	} else {
		DBG_ERR("Unknown VidDevIdx=%d\r\n", vidDevIdx);
		return E_SYS;
	}
	DbgMsg_UVC(("^R+%s vid[%d]:chkFlag=0x%x,setFlag=0x%x,rdyFlag=0x%x\r\n", __func__, vidDevIdx, chkFlag, setFlag, rdyFlag));
	if (kchk_flg(FLG_ID_U2UVAC, chkFlag) & chkFlag) {
		clr_flg(FLG_ID_U2UVAC, chkFlag);
		gUvacRunningFlag[vidDevIdx] |= setFlag;
		set_flg(FLG_ID_U2UVAC, setFlag);

		DbgMsg_UVC(("+UVAC[%d] STOP Run \r\n", vidDevIdx));
		wai_flg(&uiFlag, FLG_ID_U2UVAC, rdyFlag, TWF_ANDW);
		DbgMsg_UVC(("-UVAC[%d] STOP Run \r\n", vidDevIdx));

		return E_OK;
	}

	DbgMsg_UVC(("^R-%s\r\n", __func__));
	return E_SYS;
}
#if 1
static int _UAC_Stop(UVAC_AUD_DEV_CNT audDevIdx)
{
	FLGPTN uiFlag = 0, rdyFlag = 0;

	if (UVAC_AUD_DEV_CNT_1 == audDevIdx) {
		rdyFlag = FLGUVAC_ISOIN_AUD_READY;
	} else if (UVAC_AUD_DEV_CNT_2 == audDevIdx) {
		rdyFlag = FLGUVAC_ISOIN_AUD2_READY;
	} else {
		DBG_ERR("Unknown VidDevIdx=%d\r\n", audDevIdx);
		return E_SYS;
	}
	DbgMsg_UVC(("+UAC[%d] STOP Run \r\n", audDevIdx));
	vos_flag_wait_timeout(&uiFlag, FLG_ID_U2UVAC, rdyFlag, TWF_ANDW, vos_util_msec_to_tick(10));
	DbgMsg_UVC(("-UAC[%d] STOP Run \r\n", audDevIdx));

	return E_OK;

}
#endif
//INterface, AlternatSetting are highly depends on the definition of USB device driver
static BOOL UVAC_GetIntfAltsetFromCB(UINT32 uEvent, UINT8 *pIntf, UINT8 *pAltSet)
{
	if (uEvent) {
		*pIntf = uEvent >> 16;
		*pAltSet = (UINT8)uEvent;
		return TRUE;
	} else {
		return FALSE;
	}
}

static void UVAC_InterfaceCB(UINT32 uEvent)
{
	UINT32 i = 0;
	ER retV = E_OK;
	UINT32 tmpV = 0, txfQueIdx;
	UINT8 intf = 0, altSet = 0;
	BOOL found = FALSE;
	DbgMsg_UVCIO(("%s:0x%x\r\n", __func__, uEvent));
	if (UVAC_GetIntfAltsetFromCB(uEvent, &intf, &altSet)) {
		DbgMsg_UVC(("^RUSBDrv-CB:0x%x,0x%x,0x%x\r\n", uEvent, intf, altSet));
	} else {
		DBG_DUMP("!!Unknown:0x%x,uvac channel=%d\r\n", uEvent, gU2UvacChannel);
		return;
	}
	for (i = 0; i < UVAC_VID_DEV_CNT_MAX; i++) {
		if (intf == gU2UvacIntfIdx_VS[i]) {
			//Clear FIFO when setting interface
			if (gUvacOpened) {
#if 0 //Interrupt
				usb_clrEPFIFO(USB_EP5);
				usb_clrEPFIFO(USB_EP6);
#endif
				//usb_clrEPFIFO(USB_EP1);
				if (gU2UvacChannel > UVAC_CHANNEL_1V1A) {
					//usb_clrEPFIFO(USB_EP2);
				}
				if (gU2UvcCapM3Enable) {
					//usb_clrEPFIFO(USB_EP7);
				}
			}
			if (altSet) {
				DbgMsg_UVC(("Vid[%d] Start[%d/%d]:evt=0x%x\r\n", i, intf, altSet, uEvent));
				loc_cpu();
				gU2UvcVidStart[i] = TRUE;
				unl_cpu();
				UVAC_Start(i);
				found = TRUE;
				break;
			} else {
				DbgMsg_UVC(("Vid[%d] Stop[%d/%d]:evt=0x%x,stillTrig=%d, Ori=%d\r\n", i, intf, altSet, uEvent, gU2UvacStillImgTrigSts, gU2UvcVidStart[i]));
				loc_cpu();
				gU2UvcVidStart[i] = FALSE;
				unl_cpu();
				if (i == UVAC_VID_DEV_CNT_1) {
					txfQueIdx = UVAC_TXF_QUE_V1;
				} else { // if(i == UVAC_VID_DEV_CNT_2)
					txfQueIdx = UVAC_TXF_QUE_V2;
				}

				_UVC_Stop(i);
				U2UVAC_RemoveTxfInfo(txfQueIdx);

				if (gU2UvacStillImgTrigSts == UVC_STILLIMG_TRIG_CTRL_TRANSMIT_BULK) { //2
					//U2UVC_TrIgStIlImg(gU2UvcCapImgAddr, gU2UvcCapImgSize);
				}
				found = TRUE;
				break;
			}
		}
	}
	if (FALSE == found) {
		for (i = 0; i < UVAC_AUD_DEV_CNT_MAX; i++) {
			if (intf == gU2UvacIntfIdx_AS[i] && i == UVAC_AUD_DEV_CNT_3) {
				DbgMsg_UVC(("Aud[%d] [inf: %d/alt: %d]:evt=0x%x\r\n", i, intf, altSet, uEvent));
				found = TRUE;
				//usb_clrEPFIFO(U2UVAC_USB_RX_EP[0]);

				if (altSet) {
					//usb_clrEPFIFO(U2UVAC_USB_RX_EP[0]);
					_U2UVAC_init_queue();
					loc_cpu();
					gU2UacAudStart[i] = TRUE;
					unl_cpu();
					gU2UacUsbDMAAbord = FALSE;
					break;
				} else {
					//usb_clrEPFIFO(U2UVAC_USB_RX_EP[0]);
					tmpV = gU2UacAudStart[i];
					loc_cpu();
					gU2UacAudStart[i] = FALSE;
					unl_cpu();

					if (TRUE == tmpV) {
						retV = usb_abortEndpoint(U2UVAC_USB_RX_EP[0]);
						if (retV != E_OK) {
							DBG_ERR("Abort error\r\n");
						}
					}
					break;
				}
				break;
			} else if (intf == gU2UvacIntfIdx_AS[i]) {
				//Clear FIFO when setting interface
				if (gUvacOpened) {
					//usb_clrEPFIFO(USB_EP3);
					if (gU2UvacChannel > UVAC_CHANNEL_1V1A) {
						//usb_clrEPFIFO(USB_EP4);
					}
				}
				if (altSet) {
					DbgMsg_UVC(("Aud[%d] Start[%d/%d]:evt=0x%x\r\n", i, intf, altSet, uEvent));

					#if ISF_AUDIO_LATENCY_DEBUG
					U2UVAC_DbgDmp_Aud_TimestampNumReset();
					#endif

					txfQueIdx = i + UVAC_VID_DEV_CNT_MAX;//UVAC_TXF_QUE_A1, UVAC_TXF_QUE_A2
					//U2UVAC_RemoveTxfInfo(txfQueIdx);
					loc_cpu();
					gU2UacAudStart[i] = TRUE;
					unl_cpu();
					found = TRUE;

					if (g_fpStartAudio) {
						memset((void *)&gAudStrmInfo[i], 0, sizeof(UVAC_STRM_INFO));
						gAudStrmInfo[i].strmPath = i;
						gAudStrmInfo[i].isAudStrmOn = TRUE;
						g_fpStartAudio(i, &gAudStrmInfo[i]);
					}
					break;
				} else {
					char  *chip_name = getenv("NVT_CHIP_ID");

					DbgMsg_UVC(("Aud[%d] Stop[%d/%d]:evt=0x%x, Ori=%d\r\n", i, intf, altSet, uEvent, gU2UacAudStart[i]));
					loc_cpu();
					tmpV = gU2UacAudStart[i];
					gU2UacAudStart[i] = FALSE;
					unl_cpu();
					txfQueIdx = i + UVAC_VID_DEV_CNT_MAX;//UVAC_TXF_QUE_A1, UVAC_TXF_QUE_A2
					if (g_fpStartAudio) {
						gAudStrmInfo[i].isAudStrmOn = FALSE;
						g_fpStartAudio(i, &gAudStrmInfo[i]);
					}

					if (!(chip_name != NULL && strcmp(chip_name, "CHIP_NA51084") == 0)) {
						if (TRUE == tmpV) {
							retV = usb_abortEndpoint(U2UVAC_USB_EP[txfQueIdx]);
							DbgMsg_UVC(("Abort Aud-EP[%d/%d]=%d, retV=%d\n\r", i, txfQueIdx, U2UVAC_USB_EP[txfQueIdx], retV));
							if (retV != E_OK) {
								DBG_ERR("Abort error\r\n");
							}
						}
					}

					_UAC_Stop(i);
					U2UVAC_RemoveTxfInfo(txfQueIdx);
					if (i == UVAC_AUD_DEV_CNT_1) {
						vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_A1);
					} else {
						vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_A2);
					}
					found = TRUE;
					break;
				}
			}
		}
	}
	if (FALSE == found) {
		DBG_DUMP("!!Unknown CB evt=0x%x, Intf=%d, alt=%d,uvac channel=%d\r\n", uEvent, intf, altSet, gU2UvacChannel);
		return;
	}
}

/*
    UVAC_OpenNeededFIFO
*/
static void UVAC_OpenNeededFIFO(void)
{
	usb_maskEPINT(USB_EP1);
	if (gDisableUac == FALSE) {
		usb_maskEPINT(USB_EP3);
	}
	if (gU2UvacCdcEnabled) {
#if 0//USB_CDC_IF_COMM_NUMBER_EP
		usb_maskEPINT(CDC_COMM_IN_EP);
#if USB_2ND_CDC
		usb_maskEPINT(CDC_COMM2_IN_EP);
#endif
#endif
		usb_maskEPINT(CDC_DATA_IN_EP);
		usb_unmaskEPINT(CDC_DATA_OUT_EP);
#if USB_2ND_CDC
		usb_maskEPINT(CDC_DATA2_IN_EP);
		usb_unmaskEPINT(CDC_DATA2_OUT_EP);
#endif
	}
	if (g_hid_info.en) {
		usb_maskEPINT(HID_INTRIN_EP);
		if (g_hid_info.intr_out) {
			usb_unmaskEPINT(HID_INTROUT_EP);
		}
	}
	if (gU2UvacUacRxEnabled) {
		usb_unmaskEPINT(U2UVAC_USB_RX_EP[0]);
	}

	if (g_msdc_info.en) {
		usb_maskEPINT(MSDC_IN_EP);
		usb_unmaskEPINT(MSDC_OUT_EP);
	}
}
void U2UVC_SetTestImg(UINT32 imgAddr, UINT32 imgSize)
{
	DbgMsg_UVC(("imgAddr=0x%x, imgSize=0x%x\r\n", __func__, imgAddr, imgSize));
	gU2UvcCapImgAddr = imgAddr;
	gU2UvcCapImgSize = imgSize;
	gU2UvacStillImgTrigSts = UVC_STILLIMG_TRIG_CTRL_TRANSMIT_BULK;
	gU2UvcUsbDMAAbord = FALSE;
	U2UVC_TrIgStIlImg(gU2UvcCapImgAddr, gU2UvcCapImgSize);
}
static UINT32 uvac_ep_to_rx_event(USB_EP EPn)
{
	UINT32 event = USB_EVENT_NONE;

	if (EPn > USB_EP0 && EPn < USB_EP8) {
		event = USB_EVENT_EP0_RX + EPn;
	}
	return event;
}
static void UVAC_Callback(UINT32 uEvent)
{
	DbgMsg_UVCIO(("^G%s:0x%x\r\n", __func__, uEvent));

	switch (uEvent) {
	case USB_EVENT_OUT0:
		set_flg(FLG_ID_U2UVAC, FLGUVAC_OUT0);
		break;
	case USB_EVENT_RESET:
		if (g_dfu_info.en && g_dfu_info.cb) {
			g_dfu_info.cb(UVAC_DFU_USB_RESET, 0, 0, 0, 0);
		}
		break;
	case USB_EVENT_EP7_RX:
		usb_maskEPINT(U2UVAC_USB_RX_EP[0]);
		set_flg(FLG_ID_U2UVAC_UAC_RX, FLGUVAC_AUD_DATA_OUT);
		break;
	case USB_EVENT_EP8_RX:
		if (gU2UvacCdcEnabled){
			set_flg(FLG_ID_U2UVAC, FLGUVAC_CDC_DATA_OUT);
			usb_maskEPINT(CDC_DATA_OUT_EP);
			break;
		}
	default:
		if (g_hid_info.en && g_hid_info.intr_out) {
			if (uEvent == uvac_ep_to_rx_event(HID_INTROUT_EP)) {
				set_flg(FLG_ID_U2UVAC, FLGUVAC_HID_DATA_OUT);
				usb_maskEPINT(HID_INTROUT_EP);
			}
		}

		if (g_msdc_info.en && g_u2uvac_msdc_device_evt_cb) {
			g_u2uvac_msdc_device_evt_cb(uEvent);
		}

		break;
	}
}
void U2UVAC_SetEachStrmInfo(PUVAC_STRM_FRM pStrmFrm)
{
	UINT32 devIdx = 0;
	UINT32 tmpIdx = 0;
	UVAC_VID_STRM_INFO *pStrmQueInfo = 0;
	UVAC_STRM_PATH strmPath;
	UVAC_TXF_INFO txfInfo;

	if (FALSE == gUvacOpened)
		return;

	if (0 == pStrmFrm) {
		DBG_ERR("Input pStrmFrm NULL\r\n");
		return;
	}

	strmPath = pStrmFrm->path;
	if ((m_uiU2UvacDbgVid & UVAC_DBG_VID_RCV_ALL)) {
		DBG_DUMP("Strm[%d]=0x%x,%d\r\n", pStrmFrm->path, pStrmFrm->addr, pStrmFrm->size);
		DBG_DUMP("UVAC-Ch=%d,v1=%d,a1=%d,v2=%d,a2=%d, abort=%d %d\r\n", gU2UvacChannel, gU2UvcVidStart[0], gU2UacAudStart[0], gU2UvcVidStart[1], gU2UacAudStart[1], gU2UvcUsbDMAAbord, gU2UvcNoVidStrm);
	}
	if (0 == pStrmFrm->size || 0 == pStrmFrm->addr) {
		DBG_WRN("[%d]zero stream:0x%x,0x%x\r\n", strmPath, pStrmFrm->addr, pStrmFrm->size);
		return;
	}
	//DbgMsg_UVCIO(("Strm[%d]=0x%x, 0x%x\r\n", pStrmFrm->path, pStrmFrm->addr, pStrmFrm->size));
	//DbgMsg_UVCIO(("UVAC-Ch=%d,v1=%d,a1=%d,v2=%d,a2=%d, abort=%d %d\r\n", gU2UvacChannel, gU2UvcVidStart[0], gU2UacAudStart[0], gU2UvcVidStart[1], gU2UacAudStart[1], gU2UvcUsbDMAAbord, gU2UvcNoVidStrm));

	loc_cpu();
	if ((TRUE == gU2UvcVidStart[UVAC_VID_DEV_CNT_1]) && (UVAC_STRM_VID == strmPath)) {
		vos_flag_clr(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V1);
		unl_cpu();
		devIdx = UVAC_VID_DEV_CNT_1;
		if (pStrmFrm->size > gVidTxBuf[devIdx].size*98/100) { //keep for payload header
			DBG_WRN("skip! frame(%dKB)> TxBuf (%dKB)\r\n", pStrmFrm->size/1024, gVidTxBuf[devIdx].size*98/100/1024);
			vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V1);
			return;
		}
		#if defined(__LINUX_USER__)
		if (0 == gUvcVidBuf[devIdx].size && 0 == pStrmFrm->va) {
			DBG_ERR("UVAC_CONFIG_VID_BUF_INFO not configured!\r\n");
		}
		#endif
		pStrmQueInfo = &gUvacVidStrmInfo[devIdx];
		tmpIdx = pStrmQueInfo->idxProducer;
		DbgMsg_UVCIO(("VidQue[%d] new[%d]=0x%x, 0x%x\r\n", devIdx, tmpIdx, pStrmFrm->addr, pStrmFrm->size));
		if (((tmpIdx + 1) & UVAC_VID_INFO_QUE_MAX_IDX) == pStrmQueInfo->idxConsumer) {
			DBG_ERR("VidQue[%d] is Full:%d, %d\r\n", devIdx, pStrmQueInfo->idxConsumer, pStrmQueInfo->idxProducer);
		}
		if (pStrmFrm->va) {
			pStrmQueInfo->addr[tmpIdx] = pStrmFrm->va;
		} else {
			pStrmQueInfo->addr[tmpIdx] = uvac_pa_to_va(UVAC_BUF_TYPE_VID, pStrmFrm->addr);
		}
		pStrmQueInfo->size[tmpIdx] = pStrmFrm->size;
		pStrmQueInfo->idxProducer = (tmpIdx + 1) & UVAC_VID_INFO_QUE_MAX_IDX;
		if (pStrmFrm->pStrmHdr) {
			if (gUvcVidBuf[UVAC_VID_DEV_CNT_1].pa) {
				pStrmQueInfo->strmInfo.pStrmHdr = (UINT8 *)uvac_pa_to_va(UVAC_BUF_TYPE_VID,  (UINT32)pStrmFrm->pStrmHdr);
			} else {
				pStrmQueInfo->strmInfo.pStrmHdr = (UINT8 *)pStrmFrm->pStrmHdr;
			}
		} else {
			pStrmQueInfo->strmInfo.pStrmHdr = 0;
		}
		pStrmQueInfo->strmInfo.strmHdrSize= pStrmFrm->strmHdrSize;
		pStrmQueInfo->timestamp = pStrmFrm->timestamp;

		if (uvc_direct_trigger) {
			set_flg(FLG_ID_U2UVAC, FLGUVAC_VIDEO_TXF);
		}
		if (m_uiU2UvacDbgVid & UVAC_DBG_VID_RCV_V1) {
			//UINT8 *pVidBuf;
			//pVidBuf = (UINT8 *)(pStrmQueInfo->addr[tmpIdx] + UVAC_VID_I_FRM_MARK_POS + user_data_size);
			DBG_DUMP("V[0][%d]=0x%X(va0x%X),%d,hdr=0x%X\r\n", tmpIdx, pStrmFrm->addr, pStrmFrm->va, pStrmFrm->size, pStrmFrm->pStrmHdr);
			//DBG_DUMP("I@0x%X=%02X\n", (UINT32)pVidBuf, *pVidBuf);
		}
	} else if ((TRUE == gU2UacAudStart[UVAC_AUD_DEV_CNT_1]) && (UVAC_STRM_AUD == strmPath)) {
		vos_flag_clr(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_A1);
		unl_cpu();
		txfInfo.sAddr = txfInfo.oriAddr = pStrmFrm->addr;
		txfInfo.size = pStrmFrm->size;
		txfInfo.usbEP = U2UVAC_USB_EP[UVAC_TXF_QUE_A1];
		txfInfo.streamType = UVAC_TXF_STREAM_AUD;
		txfInfo.timestamp = pStrmFrm->timestamp;
		DbgMsg_UVCIO(("^YA-EP%d:0x%x\r\n", txfInfo.usbEP, txfInfo.size));
		U2UVAC_AddIsoInTxfInfo(&txfInfo, UVAC_TXF_QUE_A1);
		//set_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_TXF);
		if (m_uiU2UvacDbgVid & UVAC_DBG_VID_RCV_A1) {
			DBG_DUMP("A[0]0x%x, %d\r\n", pStrmFrm->addr, pStrmFrm->size);
		}
	} else if ((TRUE == gU2UvcVidStart[UVAC_VID_DEV_CNT_2]) && (UVAC_STRM_VID2 == strmPath)) {
		vos_flag_clr(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V2);
		unl_cpu();
		devIdx = UVAC_VID_DEV_CNT_2;
		if (pStrmFrm->size > gVidTxBuf[devIdx].size*98/100) { //keep for payload header
			DBG_WRN("skip! frame(%dKB)> TxBuf (%dKB)\r\n", pStrmFrm->size/1024, gVidTxBuf[devIdx].size*98/100/1024);
			vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V2);
			return;
		}
		#if defined(__LINUX_USER__)
		if (0 == gUvcVidBuf[devIdx].size && 0 == pStrmFrm->va) {
			DBG_ERR("UVAC_CONFIG_VID_BUF_INFO not configured!\r\n");
		}
		#endif

		if (gU2UvacChannel == UVAC_CHANNEL_1V1A) {
			DBG_ERR("1V1A only\r\n");
			vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V2);
			return;
		}

		pStrmQueInfo = &gUvacVidStrmInfo[devIdx];

		tmpIdx = pStrmQueInfo->idxProducer;
		DbgMsg_UVCIO(("VidQue[%d] new[%d]=0x%x, 0x%x\r\n", devIdx, tmpIdx, pStrmFrm->addr, pStrmFrm->size));
		if (((tmpIdx + 1) & UVAC_VID_INFO_QUE_MAX_IDX) == pStrmQueInfo->idxConsumer) {
			DBG_ERR("VidQue[%d] is Full:%d, %d\r\n", devIdx, pStrmQueInfo->idxConsumer, pStrmQueInfo->idxProducer);
		}
		if (pStrmFrm->va) {
			pStrmQueInfo->addr[tmpIdx] = pStrmFrm->va;
		} else {
			pStrmQueInfo->addr[tmpIdx] = uvac_pa_to_va(UVAC_BUF_TYPE_VID2, pStrmFrm->addr);
		}
		pStrmQueInfo->size[tmpIdx] = pStrmFrm->size;
		pStrmQueInfo->idxProducer = (tmpIdx + 1) & UVAC_VID_INFO_QUE_MAX_IDX;
		if (pStrmFrm->pStrmHdr) {
			if (gUvcVidBuf[UVAC_VID_DEV_CNT_2].pa) {
				pStrmQueInfo->strmInfo.pStrmHdr = (UINT8 *)uvac_pa_to_va(UVAC_BUF_TYPE_VID2,  (UINT32)pStrmFrm->pStrmHdr);
			} else {
				pStrmQueInfo->strmInfo.pStrmHdr = (UINT8 *)pStrmFrm->pStrmHdr;
			}
		} else {
			pStrmQueInfo->strmInfo.pStrmHdr = 0;
		}
		pStrmQueInfo->strmInfo.strmHdrSize= pStrmFrm->strmHdrSize;
		pStrmQueInfo->timestamp = pStrmFrm->timestamp;

		if (uvc_direct_trigger) {
			set_flg(FLG_ID_U2UVAC, FLGUVAC_VIDEO2_TXF);
		}
		if (m_uiU2UvacDbgVid & UVAC_DBG_VID_RCV_V2) {
			DBG_DUMP("V[1][%d]=0x%X(va0x%X),%d,hdr=0x%X\r\n", tmpIdx, pStrmFrm->addr, pStrmFrm->va, pStrmFrm->size, pStrmFrm->pStrmHdr);
		}
	} else if ((TRUE == gU2UacAudStart[UVAC_AUD_DEV_CNT_2]) && (UVAC_STRM_AUD2 == strmPath)) {
		vos_flag_clr(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_A2);
		unl_cpu();
		if (gU2UvacChannel == UVAC_CHANNEL_1V1A) {
			DBG_ERR("1V1A only\r\n");
			return;
		}
		txfInfo.sAddr = txfInfo.oriAddr = pStrmFrm->addr;
		txfInfo.size = pStrmFrm->size;
		txfInfo.usbEP = U2UVAC_USB_EP[UVAC_TXF_QUE_A2];
		txfInfo.streamType = UVAC_TXF_STREAM_AUD;
		txfInfo.timestamp = pStrmFrm->timestamp;
		DbgMsg_UVCIO(("^Y2A-EP%d:0x%x\r\n", txfInfo.usbEP, txfInfo.size));
		U2UVAC_AddIsoInTxfInfo(&txfInfo, UVAC_TXF_QUE_A2);
		//set_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_TXF);
		if (m_uiU2UvacDbgVid & UVAC_DBG_VID_RCV_A2) {
			DBG_DUMP("A[1]0x%x, %d\r\n", pStrmFrm->addr, pStrmFrm->size);
		}
	} else {
		unl_cpu();
		//DbgMsg_UVCIO(("Strm=%d,en=%d,%d,%d,%d\r\n", strmPath, gU2UvcVidStart[UVAC_VID_DEV_CNT_1], gU2UvcVidStart[UVAC_VID_DEV_CNT_2], gU2UacAudStart[UVAC_AUD_DEV_CNT_1], gU2UacAudStart[UVAC_AUD_DEV_CNT_2]));
	}
	//DbgMsg_UVCIO(("-%s:type=%d,enable=%d,%d,%d,%d\r\n", __func__, strmPath, gU2UvcVidStart[UVAC_VID_DEV_CNT_1], gU2UvcVidStart[UVAC_VID_DEV_CNT_2], gU2UacAudStart[UVAC_AUD_DEV_CNT_1], gU2UacAudStart[UVAC_AUD_DEV_CNT_2]));
}
static UINT32 UVCUtil_MakeHeadScrPts(UINT8 *pH264DecAddr, UINT32 fid)
{
	*pH264DecAddr = PAYLOAD_LEN;
	*(pH264DecAddr + 1) = PAYLOAD_SCR | PAYLOAD_PTS | PAYLOAD_EOH;

	if (fid) {
		*(pH264DecAddr + 1) |= PAYLOAD_FID;
	}

	*(pH264DecAddr + 2) = gUVCHeadPTS & 0xFF;
	*(pH264DecAddr + 3) = (gUVCHeadPTS >> 8) & 0xFF;
	*(pH264DecAddr + 4) = (gUVCHeadPTS >> 16) & 0xFF;
	*(pH264DecAddr + 5) = (gUVCHeadPTS >> 24) & 0xFF;

	*(pH264DecAddr + 6) = gUVCHeadSCR & 0xFF;
	*(pH264DecAddr + 7) = (gUVCHeadSCR >> 8) & 0xFF;
	*(pH264DecAddr + 8) = (gUVCHeadSCR >> 16) & 0xFF;
	*(pH264DecAddr + 9) = (gUVCHeadSCR >> 24) & 0xFF;

	gSOFTokenNum = usb_getSOF();
	*(pH264DecAddr + 10) = gSOFTokenNum & 0xFF;
	*(pH264DecAddr + 11) = (gSOFTokenNum >> 8) & 0xFF;

	gUVCHeadPTS += 0xF3BBA;
	gUVCHeadSCR += 3940;
	return PAYLOAD_LEN;
}

static void _uac_timerhdl(void)
{
	vos_flag_set(FLG_ID_U2UVAC_UAC, FLGUVAC_AUD1_CHK_BUSY);
}

static void _uac_timerhdl2(void)
{
	vos_flag_set(FLG_ID_U2UVAC_UAC, FLGUVAC_AUD2_CHK_BUSY);
}

ER U2UVAC_ConfigVidReso(PUVAC_VID_RESO pVidReso, UINT32 cnt)
{
	ER retV = E_OK;
	DbgMsg_UVC(("+ %s:pVidReso=0x%x cnt=%d, limit=%d\r\n", __func__, pVidReso, cnt, UVAC_VID_RESO_MAX_CNT));
	if ((UVAC_VID_RESO_MAX_CNT >= cnt) && pVidReso && cnt) {
		memset((void *)gU2UvcVidResoAry, 0, (sizeof(UVAC_VID_RESO)* UVAC_VID_RESO_MAX_CNT));
		memcpy((void *)gU2UvcVidResoAry, pVidReso, (sizeof(UVAC_VID_RESO)* cnt));
		gU2UvcVidResoCnt = cnt;
		//just for backwark compatible
		gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_1].aryCnt = gUvcH264FrmInfo[UVAC_VID_DEV_CNT_1].aryCnt= cnt;
		gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_1].pVidResAry = gUvcH264FrmInfo[UVAC_VID_DEV_CNT_1].pVidResAry = gU2UvcVidResoAry;

#if (_UVC_DBG_LVL_ > _UVC_DBG_CHK_)
		{
			UINT32 i = 0, j = 0;
			for (i = 0; i < gU2UvcVidResoCnt; i++) {
				DBG_DUMP("VidReso[%d]=%d, %d, fpsCnt=%d: \r\n", i, gU2UvcVidResoAry[i].width, gU2UvcVidResoAry[i].height, gU2UvcVidResoAry[i].fpsCnt);
				for (j = 0; j < UVAC_VID_RESO_FPS_MAX_CNT; j++) {
					DBG_DUMP("  fps[%d]=%d", j, gU2UvcVidResoAry[i].fps[j]);
				}
				DBG_DUMP("\r\n");
			}
			if (i < UVAC_VID_RESO_MAX_CNT) {
				DBG_DUMP("VidReso[%d]=%d, %d, fpsCnt=%d: \r\n", i, gU2UvcVidResoAry[i].width, gU2UvcVidResoAry[i].height, gU2UvcVidResoAry[i].fpsCnt);
				for (j = 0; j < UVAC_VID_RESO_FPS_MAX_CNT; j++) {
					DBG_DUMP("  fps[%d]=%d", j, gU2UvcVidResoAry[i].fps[j]);
				}
				DBG_DUMP("\r\n");
			}
		}
#endif

	} else {
		DBG_ERR("pVidReso=0x%x NULL -or- cnt=%d, max = %d\r\n", (unsigned int)pVidReso, cnt, UVAC_VID_RESO_MAX_CNT);
		retV = E_SYS;
	}
	DbgMsg_UVC(("- %s:retV=0x%x cnt=%d, limit=%d\r\n", __func__, retV, cnt, UVAC_VID_RESO_MAX_CNT));
	return retV;
}
void U2UVAC_SetConfig(UVAC_CONFIG_ID ConfigID, UINT32 Value)
{
	//DBG_DUMP("ConfigID=%d, Value=0x%x, opened=%d\r\n",ConfigID, Value, gUvacOpened);
	#if 0
	if (gUvacOpened) {
		if (UVAC_CONFIG_MFK_REC2LIVEVIEW == ConfigID) {
			gU2UvcNoVidStrm = TRUE;
		} else if (UVAC_CONFIG_MFK_LIVEVIEW2REC == ConfigID) {
			UINT32 i;
			gU2UvcNoVidStrm = FALSE;
			for (i = UVAC_VID_DEV_CNT_1; i < UVAC_VID_DEV_CNT_MAX; i++) {
				if (TRUE == gU2UvcVidStart[i]) {
					UVAC_Start(i);
				}
			}
		} else {
			DBG_ERR("UVAC is opened, Can Not set ConfigID=%d, Value=0x%x\r\n", ConfigID, Value);
		}
		return;
	}
	#endif
	switch (ConfigID) {
	case UVAC_CONFIG_AUD_FMT_TYPE:
		if (UVAC_AUD_FMT_TYPE_PCM == Value) {
			gU2UvacAudFmtType = UAC_FMT_TYPE_I;
		} else {
			gU2UvacAudFmtType = UAC_FMT_TYPE_II;
		}
		break;
	case UVAC_CONFIG_AUD_SAMPLERATE: {
			PUVAC_AUD_SAMPLERATE_ARY pAudAry = (PUVAC_AUD_SAMPLERATE_ARY)Value;
			if (pAudAry && pAudAry->pAudSampleRateAry) {
				UINT32 maxCnt = 0;
				memset((void *)&gU2UvacAudSampleRate[0], 0, UVAC_AUD_SAMPLE_RATE_MAX_CNT * sizeof(UINT32));
				maxCnt = (pAudAry->aryCnt > UVAC_AUD_SAMPLE_RATE_MAX_CNT) ? UVAC_AUD_SAMPLE_RATE_MAX_CNT : pAudAry->aryCnt;
				memcpy((void *)&gU2UvacAudSampleRate[0], (void *)pAudAry->pAudSampleRateAry, maxCnt * sizeof(UINT32));
			} else {
				DBG_ERR("no pAudSampleRateAry\r\n");
			}
		}
		break;
#if 0
	case UVAC_CONFIG_WINUSB_ENABLE:
		gU2UvacWinIntrfEnable = Value;
		break;
	case UVAC_CONFIG_WINUSB_CB:
		gU2UvcWinUSBReqCB = (UVAC_WINUSBCLSREQCB)Value;
		break;
#endif
	case UVAC_CONFIG_VEND_DEV_DESC:
		if (0 == Value) {
			DBG_ERR("ConfigID=%d, Value NULL\r\n", ConfigID);
			return;
		}
		gpImgUnitUvacVendDevDesc = (UVAC_VEND_DEV_DESC *)Value;
		UVAC_DbgDmp_VendDevDesc(gpImgUnitUvacVendDevDesc);
		if (gpImgUnitUvacVendDevDesc->VID != 0) {
			g_U2UVACDevDesc.id_vendor = gpImgUnitUvacVendDevDesc->VID;
		}
		if (gpImgUnitUvacVendDevDesc->PID != 0) {
			g_U2UVACDevDesc.id_product = gpImgUnitUvacVendDevDesc->PID;
		}
		if (0 == gpImgUnitUvacVendDevDesc->fpIQVendReqCB) {
			DBG_IND("IQ VendReqCB NULL\r\n");
		}
		if (0 == gpImgUnitUvacVendDevDesc->fpVendReqCB) {
			DBG_IND("VendReqCB NULL\r\n");
		}
		break;
#if 0
	case UVAC_CONFIG_UVAC_CAP_M3:
		gU2UvcCapM3Enable = Value;
		break;
#endif
	case UVAC_CONFIG_H264_TARGET_SIZE:
		gUvcH264TBR = Value;
		break;
	case UVAC_CONFIG_MJPG_TARGET_SIZE:
		gUvcMJPGTBR = Value;
		break;
	case UVAC_CONFIG_VENDOR_CALLBACK:
		g_fpU2UvacVendorReqCB = (UVAC_VENDOR_REQ_CB)Value;
		break;
	case UVAC_CONFIG_VENDOR_IQ_CALLBACK:
		g_fpU2UvacVendorReqIQCB = (UVAC_VENDOR_REQ_CB)Value;
		break;
	case UVAC_CONFIG_MFK_LIVEVIEW2REC:
		gU2UvcNoVidStrm = FALSE;          //necessary for continue loopTxf
		break;
	case UVAC_CONFIG_EU_VENDCMDCB_ID01:
	case UVAC_CONFIG_EU_VENDCMDCB_ID02:
	case UVAC_CONFIG_EU_VENDCMDCB_ID03:
	case UVAC_CONFIG_EU_VENDCMDCB_ID04:
	case UVAC_CONFIG_EU_VENDCMDCB_ID05:
	case UVAC_CONFIG_EU_VENDCMDCB_ID06:
	case UVAC_CONFIG_EU_VENDCMDCB_ID07:
	case UVAC_CONFIG_EU_VENDCMDCB_ID08:
		gU2UvacEUVendCmdCB[ConfigID - UVAC_CONFIG_EU_VENDCMDCB_START] = (UVAC_EUVENDCMDCB)Value;
		break;
	case UVAC_CONFIG_XU_CTRL:
		g_fpUvcXU_CB = (UVAC_UNIT_CB)Value;
		break;
	case UVAC_CONFIG_VIDEO_FORMAT_TYPE:
		gU2UvcVideoFmtType = Value;
		break;
	case UVAC_CONFIG_CDC_ENABLE:
		if (g_msdc_info.en)
			DBG_ERR("CDC can't encable with MSDC at the same time\r\n");
		else
			gU2UvacCdcEnabled = Value;
		break;
	case UVAC_CONFIG_CDC_PSTN_REQUEST_CB:
		gfpU2CdcPstnReqCB = (UVAC_CDC_PSTN_REQUEST_CB)Value;
		break;
#if 0//gU2UvacMtpEnabled
	case UVAC_CONFIG_MTP_ENABLE:
		gU2UvacMtpEnabled = (BOOL)Value;
		UVAC_MTPEnable((UINT8)Value);
		break;
	case UVAC_CONFIG_MTP_DEVICE_FREIEDLYNAME:
		UVAC_MTPSetDeviceFriendlyName((char *)Value);
		break;
	case UVAC_CONFIG_MTP_VENDSPECI_DEVINFO:
		UVAC_MTPSetMTPVendorSpeciDevInfo((UINT8)Value);
		break;
	case UVAC_CONFIG_MTP_DATATX_CB:
		UVAC_SidcRegDataTxfCB((UVAC_USIDCDataTxf_CB)Value);
		break;
	case UVAC_CONFIG_PTP_DEVDATETIME_CB:
		UVAC_PTPRegistryDevDateTimeCallBack((UVAC_PTPDevDateTime_CB)Value);
		break;
	case UVAC_CONFIG_PTP_BATTERY_LEVEL:
		UVAC_PTPSetBatteryLevel((UINT8)Value);
		break;
	case UVAC_CONFIG_PTP_STORAGETYPE_CB:
		UVAC_PTPRegistryStorTypeCB((UVAC_PTPStorType_CB)Value);
		break;
	case UVAC_CONFIG_PTP_STORAGEID:
		UVAC_PTPSetStorageId((UINT32)Value);
		break;
#endif
	case UVAC_CONFIG_MAX_FRAME_SIZE:
		//add margin for BRC and payload header
		gUvcMaxVideoFmtSize = ALIGN_CEIL_32(Value + Value * MAX_FRAME_SIZE_MARGIN / 100);
		if (gUvcMaxVideoFmtSize > gU2UvcMJPGMaxTBR) {
			gU2UvcMJPGMaxTBR = gUvcMaxVideoFmtSize;
		}
		break;
	case UVAC_CONFIG_AUD_CHANNEL_NUM:
		if (Value == 0) {
			gU2UacChNum = UAC_NUM_PHYSICAL_CHANNEL;
			gU2UacITOutChCfg = UAC_IT_OUT_CHANNEL_CONFIG;
			DBG_WRN("Invalid channel number = %d. Set to 2\r\n", Value);
		} else if (Value > 2) {
			gU2UacChNum = Value;
		} else {
			gU2UacChNum = Value;
			if (gU2UacChNum == 2) {
				gU2UacITOutChCfg = UAC_IT_OUT_CHANNEL_CONFIG;
			} else {
				gU2UacITOutChCfg = 0;
			}
		}
		break;
	case UVAC_CONFIG_UVC_STRING:
		p_uvc_string_desc = (USB_STRING_DESC *)Value;
		break;
	case UVAC_CONFIG_UAC_STRING:
		p_uac_string_desc = (USB_STRING_DESC *)Value;
		break;
	case UVAC_CONFIG_EU_DESC:
		memcpy(&eu_desc[0], (UVAC_EU_DESC *)Value, sizeof(UVAC_EU_DESC));
		UVAC_DbgDmp_EU_INFO(&eu_desc[0]);
		if (eu_desc[0].bUnitID < UVC_TERMINAL_ID_EXTE_UNIT) {
			DBG_ERR("Only support EU UnitID  >= %d\r\n", UVC_TERMINAL_ID_EXTE_UNIT);
			eu_desc[0].bUnitID = UVC_TERMINAL_ID_EXTE_UNIT;
		}
		break;
	case UVAC_CONFIG_HID_INFO:
		{
			UVAC_HID_INFO *p_user = (UVAC_HID_INFO *)Value;

			UVAC_DbgDmp_HID_INFO(p_user);
			if (p_user->en) {
				memcpy((void *)&g_hid_info, (const void *)p_user, sizeof(UVAC_HID_INFO));
			} else {
				memset((void *)&g_hid_info, 0, sizeof(UVAC_HID_INFO));
			}
			if (g_hid_info.intr_interval == 0) {
				g_hid_info.intr_interval = HID_EP_INTERRUPT_INTERVAL;
			}
		}
		break;
	case UVAC_CONFIG_CT_CB:
		g_fpUvcCT_CB = (UVAC_UNIT_CB)Value;
		break;
	case UVAC_CONFIG_PU_CB:
		g_fpUvcPU_CB = (UVAC_UNIT_CB)Value;
		break;
	case UVAC_CONFIG_YUV_FRM_INFO:
		{
			UVAC_VID_RESO_ARY *p_user = (UVAC_VID_RESO_ARY *)Value;
			if (p_user->aryCnt && NULL == p_user->pVidResAry) {
				DBG_ERR("param error, no array\r\n");
				gUvcYuvFrmInfo.aryCnt = 0;
				gUvcYuvFrmInfo.pVidResAry = NULL;
			} else {
				gUvcYuvFrmInfo.aryCnt = p_user->aryCnt;
				gUvcYuvFrmInfo.pVidResAry = p_user->pVidResAry;
				if (p_user->bDefaultFrameIndex > p_user->aryCnt) {
					DBG_ERR("bDefaultFrameIndex should <= aryCnt\r\n", p_user->bDefaultFrameIndex,  p_user->aryCnt);
					gUvcYuvFrmInfo.bDefaultFrameIndex = UVC_VSFMT_DEF_FRAMEINDEX;
				} else if (p_user->bDefaultFrameIndex == 0) {
					gUvcYuvFrmInfo.bDefaultFrameIndex = UVC_VSFMT_DEF_FRAMEINDEX;
				} else {
					gUvcYuvFrmInfo.bDefaultFrameIndex = p_user->bDefaultFrameIndex;
				}
			}
		}
		break;
	case UVAC_CONFIG_UVC_MJPG_FRM_INFO:
		{
			UVAC_VID_RESO_ARY *p_user = (UVAC_VID_RESO_ARY *)Value;
			if (p_user->aryCnt && NULL == p_user->pVidResAry) {
				DBG_ERR("param error, no array\r\n");
				gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_1].aryCnt = 0;
				gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_1].pVidResAry = NULL;
			} else {
				gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_1].aryCnt = p_user->aryCnt;
				gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_1].pVidResAry = p_user->pVidResAry;
				if (p_user->bDefaultFrameIndex > p_user->aryCnt) {
					DBG_ERR("bDefaultFrameIndex should <= aryCnt\r\n", p_user->bDefaultFrameIndex,  p_user->aryCnt);
					gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_1].bDefaultFrameIndex = UVC_VSFMT_DEF_FRAMEINDEX;
				} else if (p_user->bDefaultFrameIndex == 0) {
					gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_1].bDefaultFrameIndex = UVC_VSFMT_DEF_FRAMEINDEX;
				} else {
					gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_1].bDefaultFrameIndex = p_user->bDefaultFrameIndex;
				}
			}
		}
		break;
	case UVAC_CONFIG_UVC_H264_FRM_INFO:
		{
			UVAC_VID_RESO_ARY *p_user = (UVAC_VID_RESO_ARY *)Value;
			if (p_user->aryCnt && NULL == p_user->pVidResAry) {
				DBG_ERR("param error, no array\r\n");
				gUvcH264FrmInfo[UVAC_VID_DEV_CNT_1].aryCnt = 0;
				gUvcH264FrmInfo[UVAC_VID_DEV_CNT_1].pVidResAry = NULL;
			} else {
				gUvcH264FrmInfo[UVAC_VID_DEV_CNT_1].aryCnt = p_user->aryCnt;
				gUvcH264FrmInfo[UVAC_VID_DEV_CNT_1].pVidResAry = p_user->pVidResAry;
				if (p_user->bDefaultFrameIndex > p_user->aryCnt) {
					DBG_ERR("bDefaultFrameIndex should <= aryCnt\r\n", p_user->bDefaultFrameIndex,  p_user->aryCnt);
					gUvcH264FrmInfo[UVAC_VID_DEV_CNT_1].bDefaultFrameIndex = UVC_VSFMT_DEF_FRAMEINDEX;
				} else if (p_user->bDefaultFrameIndex == 0) {
					gUvcH264FrmInfo[UVAC_VID_DEV_CNT_1].bDefaultFrameIndex = UVC_VSFMT_DEF_FRAMEINDEX;
				} else {
					gUvcH264FrmInfo[UVAC_VID_DEV_CNT_1].bDefaultFrameIndex = p_user->bDefaultFrameIndex;
				}
			}
		}
		break;
	case UVAC_CONFIG_UVC2_MJPG_FRM_INFO:
		{
			UVAC_VID_RESO_ARY *p_user = (UVAC_VID_RESO_ARY *)Value;
			if (p_user->aryCnt && NULL == p_user->pVidResAry) {
				DBG_ERR("param error, no array\r\n");
				gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_2].aryCnt = 0;
				gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_2].pVidResAry = NULL;
			} else {
				gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_2].aryCnt = p_user->aryCnt;
				gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_2].pVidResAry = p_user->pVidResAry;
				if (p_user->bDefaultFrameIndex > p_user->aryCnt) {
					DBG_ERR("bDefaultFrameIndex should <= aryCnt\r\n", p_user->bDefaultFrameIndex,  p_user->aryCnt);
					gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_2].bDefaultFrameIndex = UVC_VSFMT_DEF_FRAMEINDEX;
				} else if (p_user->bDefaultFrameIndex == 0) {
					gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_2].bDefaultFrameIndex = UVC_VSFMT_DEF_FRAMEINDEX;
				} else {
					gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_2].bDefaultFrameIndex = p_user->bDefaultFrameIndex;
				}
			}
		}
		break;
	case UVAC_CONFIG_UVC2_H264_FRM_INFO:
		{
			UVAC_VID_RESO_ARY *p_user = (UVAC_VID_RESO_ARY *)Value;
			if (p_user->aryCnt && NULL == p_user->pVidResAry) {
				DBG_ERR("param error, no array\r\n");
				gUvcH264FrmInfo[UVAC_VID_DEV_CNT_2].aryCnt = 0;
				gUvcH264FrmInfo[UVAC_VID_DEV_CNT_2].pVidResAry = NULL;
			} else {
				gUvcH264FrmInfo[UVAC_VID_DEV_CNT_2].aryCnt = p_user->aryCnt;
				gUvcH264FrmInfo[UVAC_VID_DEV_CNT_2].pVidResAry = p_user->pVidResAry;
				if (p_user->bDefaultFrameIndex > p_user->aryCnt) {
					DBG_ERR("bDefaultFrameIndex should <= aryCnt\r\n", p_user->bDefaultFrameIndex,  p_user->aryCnt);
					gUvcH264FrmInfo[UVAC_VID_DEV_CNT_2].bDefaultFrameIndex = UVC_VSFMT_DEF_FRAMEINDEX;
				} else if (p_user->bDefaultFrameIndex == 0) {
					gUvcH264FrmInfo[UVAC_VID_DEV_CNT_2].bDefaultFrameIndex = UVC_VSFMT_DEF_FRAMEINDEX;
				} else {
					gUvcH264FrmInfo[UVAC_VID_DEV_CNT_2].bDefaultFrameIndex = p_user->bDefaultFrameIndex;
				}
			}
		}
		break;
	case UVAC_CONFIG_HW_COPY_CB:
		g_fpU2UvacHWCopyCB = (UVAC_HWCOPYCB)Value;
		break;
	case UVAC_CONFIG_VID_BUF_INFO:
		{
			if (gUvcVidBuf[UVAC_VID_DEV_CNT_1].size){
				hd_common_mem_munmap((void *)gUvcVidBuf[UVAC_VID_DEV_CNT_1].va, gUvcVidBuf[UVAC_VID_DEV_CNT_1].size);
			}
			if (gUvcVidBuf[UVAC_VID_DEV_CNT_2].size){
				hd_common_mem_munmap((void *)gUvcVidBuf[UVAC_VID_DEV_CNT_2].va, gUvcVidBuf[UVAC_VID_DEV_CNT_2].size);
			}

			PUVAC_VID_BUF_INFO p_vid_buf = (PUVAC_VID_BUF_INFO)Value;

			if (p_vid_buf->vid_buf_pa) {
				gUvcVidBuf[UVAC_VID_DEV_CNT_1].pa = p_vid_buf->vid_buf_pa;
				gUvcVidBuf[UVAC_VID_DEV_CNT_1].size = p_vid_buf->vid_buf_size;
				gUvcVidBuf[UVAC_VID_DEV_CNT_1].va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, p_vid_buf->vid_buf_pa, p_vid_buf->vid_buf_size);
			}
			if (p_vid_buf->vid2_buf_pa) {
				gUvcVidBuf[UVAC_VID_DEV_CNT_2].pa = p_vid_buf->vid2_buf_pa;
				gUvcVidBuf[UVAC_VID_DEV_CNT_2].size = p_vid_buf->vid2_buf_size;
				gUvcVidBuf[UVAC_VID_DEV_CNT_2].va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, p_vid_buf->vid2_buf_pa, p_vid_buf->vid2_buf_size);
			}
			printf("VID_BUF_INFO[0] pa=0x%X,size=0x%X,va=0x%X\r\n", gUvcVidBuf[UVAC_VID_DEV_CNT_1].pa, gUvcVidBuf[UVAC_VID_DEV_CNT_1].size,gUvcVidBuf[UVAC_VID_DEV_CNT_1].va);
			printf("VID_BUF_INFO[1] pa=0x%X,size=0x%X,va=0x%X\r\n", gUvcVidBuf[UVAC_VID_DEV_CNT_2].pa, gUvcVidBuf[UVAC_VID_DEV_CNT_2].size,gUvcVidBuf[UVAC_VID_DEV_CNT_2].va);
		}
		break;
	case UVAC_CONFIG_DFU_INFO:
		{
			UVAC_DFU_INFO *p_user = (UVAC_DFU_INFO *)Value;

			if (p_user->en) {
				memcpy((void *)&g_dfu_info, (const void *)p_user, sizeof(UVAC_DFU_INFO));
			} else {
				memset((void *)&g_dfu_info, 0, sizeof(UVAC_DFU_INFO));
			}
		}
		break;
	case UVAC_CONFIG_DISABLE_UAC:
		gDisableUac = Value;
		break;
	case UVAC_CONFIG_UAC_RX_ENABLE:
		gU2UvacUacRxEnabled = (Value == 0)? FALSE : TRUE;
		break;
	case UVAC_CONFIG_UAC_RX_BLK_SIZE:
		gU2UvacUacBlockSize = ALIGN_CEIL(Value, UVAC_BUF_ALIGN);
		break;
	case UVAC_CONFIG_UAC_RX_BUF_SIZE:
		gU2UvacUacBufSize = ALIGN_CEIL(Value, UVAC_BUF_ALIGN);
		break;
	case UVAC_CONFIG_AUD_START_CB:
		g_fpStartAudio = (UVAC_STARTAUDIOCB)Value;
		break;
	case UVAC_CONFIG_VID_USER_DATA_SIZE:
		user_data_size = Value;
		break;
	case UVAC_CONFIG_MJPG_DEF_FRM_IDX:
		gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_1].bDefaultFrameIndex = Value;
		break;
	case UVAC_CONFIG_H264_DEF_FRM_IDX:
		gUvcH264FrmInfo[UVAC_VID_DEV_CNT_1].bDefaultFrameIndex = Value;
		break;
#ifdef __LINUX_USER__
	case UVAC_CONFIG_MSDC_INFO:
		{
			UVAC_MSDC_INFO *p_user = (UVAC_MSDC_INFO *)Value;
			UVAC_DbgDmp_MSDC_INFO(p_user);
			if (p_user->en) {
				if (gU2UvacCdcEnabled)
					DBG_ERR("MSDC can't enable with CDC at the same time\r\n");
				else
					memcpy((void *)&g_msdc_info, (const void *)p_user, sizeof(UVAC_MSDC_INFO));
			} else {
				memset((void *)&g_msdc_info, 0, sizeof(UVAC_MSDC_INFO));
			}
		}
		break;
#endif
	case UVAC_CONFIG_EU_DESC_2ND:
		memcpy(&eu_desc[1], (UVAC_EU_DESC *)Value, sizeof(UVAC_EU_DESC));
		UVAC_DbgDmp_EU_INFO(&eu_desc[1]);
		if (eu_desc[1].bUnitID < UVC_TERMINAL_ID_EXTE_UNIT) {
			DBG_ERR("Only support EU UnitID  >= %d\r\n", UVC_TERMINAL_ID_EXTE_UNIT);
			eu_desc[1].bUnitID = UVC_TERMINAL_ID_EXTE_UNIT;
		}
		break;
	case UVAC_CONFIG_UAC_PACKETSIZE:
		if (Value > 512) {
			DBG_ERR("Uac wMaxPacketSize(%d) should < 512\r\n", Value);
		} else {
			gUacMaxPacketSize = Value;
		}
		break;
	case UVAC_CONFIG_UAC_INTERVAL:
		gUacInterval = Value;
		break;


	case UVAC_CONFIG_UVC_VER:
		gUvacUvcVer = Value;
		break;
	case UVAC_CONFIG_CT_CONTROLS:
		ct_controls = Value;
		break;
	case UVAC_CONFIG_PU_CONTROLS:
		pu_controls = Value;
		break;
	case UVAC_CONFIG_AUD_CHANNEL_CONFIG:
		gU2UacITOutChCfg = Value;
		break;
	default:
		DBG_ERR("Not Support the config:[%d]=0x%x\r\n", ConfigID, Value);
		break;
	}
}
void U2UVAC_SetCodec(UVAC_VIDEO_FORMAT CodecType, UINT32 vidIdx)
{
	DbgMsg_UVC(("PrvCodec[%d]=%d,SetTo=%d\r\n", vidIdx, gUvacCodecType[vidIdx], CodecType));
	gUvacCodecType[vidIdx] = CodecType;
}
UINT32 U2UVAC_GetNeedMemSize(void)
{
	UINT32 memSize;
	memSize = ALIGN_CEIL(gUvcMaxVideoFmtSize, UVAC_BUF_ALIGN) * gU2UvacChannel + ALIGN_CEIL(UVAC_MEM_DESC_SIZE, UVAC_BUF_ALIGN);

	if (gU2UvacUacRxEnabled) {
		 memSize += ALIGN_CEIL(gU2UvacUacBufSize, UVAC_BUF_ALIGN);
	}

	#if 0//gU2UvacMtpEnabled
	if (gU2UvacMtpEnabled) {
		memSize += SIDC_WORKING_BUFFER + MIN_BUFFER_SIZE_FOR_SENDOBJ;
	}
	#endif
	DbgMsg_UVC(("NeedMem=0x%x, ch=%d, Max Payload Frame=0x%x, DescMem=0x%x\n\r", memSize, gU2UvacChannel, gUvcMaxVideoFmtSize, UVAC_MEM_DESC_SIZE));
	return memSize;
}
static UINT32 UVCIsoIn_ConcateH264Header(UVAC_VID_DEV_CNT  thisVidIdx, PUVC_PAYL_MAKE_COMP pUvcPaylMakeComp)
{
	UINT8 *pSrcAddr = 0;
	UINT8 *pDstAddr = 0;
	UINT8 *pHdrAddr = 0;
	UINT32 vidSize = 0;
	UINT32 hdrSize = 0;
	UINT32 totalSize = 0;
	UINT8 *pFirstVidData = 0;

	if (0 == pUvcPaylMakeComp) {
		DBG_ERR("Input NULL.Do nothing!!\r\n");
		return totalSize;
	} else {
		pDstAddr = pUvcPaylMakeComp->pDst;
		if (0 == pDstAddr) {
			DBG_ERR("NULL DST!!\r\n");
			return totalSize;
		}
		pSrcAddr = pUvcPaylMakeComp->pVidSrc;
		vidSize = pUvcPaylMakeComp->vidSize;
		pHdrAddr = pUvcPaylMakeComp->pHdrAddr;
		hdrSize = pUvcPaylMakeComp->hdrSize;
		DbgMsg_UVCIO(("%s:src=0x%x,dst=0x%x,len=0x%x,hdr=0x%x,0x%x\r\n", __func__, pSrcAddr, pDstAddr, vidSize, pHdrAddr, hdrSize));
	}
//DBG_DUMP("%s:src=0x%x,dst=0x%x,len=0x%x,hdr=0x%x,0x%x\r\n", __func__, pSrcAddr, pDstAddr, vidSize, pHdrAddr, hdrSize);
	hwmem_open();
	if (hdrSize && pHdrAddr) {
		//hwmem_memcpy((UINT32)pDstAddr, (UINT32)pHdrAddr, hdrSize);
		memcpy((void *)pDstAddr, (const void *)pHdrAddr, hdrSize);
		pDstAddr = (UINT8 *)((UINT32)pDstAddr + hdrSize);
		totalSize += hdrSize;
	}
	hwmem_memcpy(thisVidIdx, (UINT32)pDstAddr, (UINT32)pSrcAddr, vidSize);
	totalSize += vidSize;
	hwmem_close();
	pFirstVidData = pDstAddr;
	if (pUvcPaylMakeComp->pVidSrc) {
		DbgMsg_UVCIO(("UPDATE first 4 byte:0x%x,0x%x,0x%x,0x%x\r\n", *pFirstVidData, *(pFirstVidData + 1), *(pFirstVidData + 2), *(pFirstVidData + 3)));
		*pFirstVidData = 0;
		*(pFirstVidData + 1) = 0;
		*(pFirstVidData + 2) = 0;
		*(pFirstVidData + 3) = 1;
	}
	DbgMsg_UVCIO(("dst=0x%x/0x%x,src=0x%x/0x%x,tol=0x%x\r\n", pUvcPaylMakeComp->pDst, pDstAddr, pUvcPaylMakeComp->pVidSrc, pSrcAddr, totalSize));
	return totalSize;
}
#if 0//SW copy only for FPGA
static UINT32 UVCIsoIn_MakeIsoPayloadTxfData(UVAC_VID_DEV_CNT  thisVidIdx, PUVC_PAYL_MAKE_COMP pUvcPaylMakeComp)
{
	UINT32 toggleFid = 0;
	UINT8 *pSrcAddr = 0;
	UINT8 *pDstAddr = 0;
	UINT8 *pHdrAddr = 0;
	UINT32 vidSize = 0;
	UINT32 hdrSize = 0;
	UINT32 remainSize = 0;
	UINT32 totalSize = 0;
	UINT32 tmpSize = 0;
	const UINT32 unitSize = gU2UVCIsoinTxfUnitSize - PAYLOAD_LEN;
	UINT8 *pLastPayloadHdrAddr = 0;
#if (_UVC_DBG_LVL_ > _UVC_DBG_CHK_)
	if (gU2UVCIsoinTxfUnitSize <= PAYLOAD_LEN) {
		DBG_ERR("Fail Unit Size=%d, Payload hdr size = %d\r\n", gU2UVCIsoinTxfUnitSize, PAYLOAD_LEN);
	}
#endif

	if (0 == pUvcPaylMakeComp) {
		DBG_ERR("Input NULL.Do nothing!!\r\n");
		return totalSize;
	} else {
		pDstAddr = pUvcPaylMakeComp->pDst;
		if (0 == pDstAddr) {
			DBG_ERR("NULL DST!!\r\n");
			return totalSize;
		}
		pSrcAddr = pUvcPaylMakeComp->pVidSrc;
		vidSize = pUvcPaylMakeComp->vidSize;
		toggleFid = pUvcPaylMakeComp->toggleFid;
		pHdrAddr = pUvcPaylMakeComp->pHdrAddr;
		hdrSize = pUvcPaylMakeComp->hdrSize;
		DbgMsg_UVCIO(("%s:src=0x%x,dst=0x%x,len=0x%x,fid=%d,hdr=0x%x,0x%x,unitSize=0x%x\r\n", __func__, pSrcAddr, pDstAddr, vidSize, toggleFid, pHdrAddr, hdrSize, unitSize));
	}

//DBG_DUMP("%s:src=0x%x,dst=0x%x,len=0x%x,fid=%d,hdr=0x%x,0x%x,unitSize=0x%x\r\n", __func__, pSrcAddr, pDstAddr, vidSize, toggleFid, pHdrAddr, hdrSize, unitSize);
	tmpSize = UVCUtil_MakeHeadScrPts(pDstAddr, toggleFid);
	pLastPayloadHdrAddr = pDstAddr;
	pDstAddr = (UINT8 *)((UINT32)pDstAddr + tmpSize);
	totalSize = tmpSize;
	if (hdrSize && pHdrAddr) {
#if (_UVC_DBG_LVL_ >= _UVC_DBG_CHK_)
		if (gU2UVCIsoinTxfUnitSize <= (hdrSize + PAYLOAD_LEN)) {
			DBG_ERR("Fail make payload, hdrSize/%d+%d=%d > txf-unit-size=%d\r\n", hdrSize, PAYLOAD_LEN, (hdrSize + PAYLOAD_LEN), gU2UVCIsoinTxfUnitSize);
		}
#endif
		//hwmem_memcpy((UINT32)pDstAddr, (UINT32)pHdrAddr, hdrSize);
		memcpy((void *)pDstAddr, (const void *)pHdrAddr, hdrSize);
		pDstAddr = (UINT8 *)((UINT32)pDstAddr + hdrSize);
		remainSize = unitSize - hdrSize;
		totalSize += hdrSize;
	} else {
		DbgMsg_UVCIO(("Null Head=0x%x,0x%x\r\n", pHdrAddr, hdrSize));
		remainSize = unitSize;
	}
	while (vidSize) {
		remainSize = (vidSize > remainSize) ? remainSize : vidSize;
		//hwmem_memcpy(thisVidIdx, (UINT32)pDstAddr, (UINT32)pSrcAddr, remainSize);
		memcpy((void *)pDstAddr, (const void *)pSrcAddr, remainSize);
		vidSize -= remainSize;
		totalSize += remainSize;
		pDstAddr = (UINT8 *)((UINT32)pDstAddr + remainSize);
		pSrcAddr = (UINT8 *)((UINT32)pSrcAddr + remainSize);
		remainSize = unitSize;
		if (vidSize) {
			tmpSize = UVCUtil_MakeHeadScrPts(pDstAddr, toggleFid);
			pLastPayloadHdrAddr = pDstAddr;
			pDstAddr = (UINT8 *)((UINT32)pDstAddr + tmpSize);
			totalSize += tmpSize;
		}
	}
	*(pLastPayloadHdrAddr + 1) |= PAYLOAD_EOF;
	hd_common_mem_flush_cache((void *)pUvcPaylMakeComp->pDst, totalSize);
	DbgMsg_UVCIO(("dst=0x%x/0x%x,src=0x%x/0x%x,tol=0x%x\r\n", pUvcPaylMakeComp->pDst, pDstAddr, pUvcPaylMakeComp->pVidSrc, pSrcAddr, totalSize));
	return totalSize;
}
#else
static UINT32 UVCIsoIn_MakeIsoPayloadTxfData(UVAC_VID_DEV_CNT  thisVidIdx, PUVC_PAYL_MAKE_COMP pUvcPaylMakeComp)
{
	UINT32 toggleFid = 0;
	UINT8 *pSrcAddr = 0;
	UINT8 *pDstAddr = 0;
	UINT8 *pHdrAddr = 0;
	UINT32 vidSize = 0;
	UINT32 hdrSize = 0;
	UINT32 remainSize = 0;
	UINT32 totalSize = 0;
	UINT32 tmpSize = 0;
	const UINT32 unitSize = gU2UVCIsoinTxfUnitSize[thisVidIdx] - PAYLOAD_LEN;
	UINT8 *pLastPayloadHdrAddr = 0;
#if UVC_MAKE_PAYLOAD_PERF
	UINT64 h1 = 0, h2 = 0, d1 = 0, d2 = 0, in = 0, out = 0;
	static UINT32 frm_cnt =0;

	frm_cnt++;
	in = hd_gettime_us();
#endif
#if (_UVC_DBG_LVL_ > _UVC_DBG_CHK_)
	if (gU2UVCIsoinTxfUnitSize[thisVidIdx] <= PAYLOAD_LEN) {
		DBG_ERR("Fail Unit Size=%d, Payload hdr size = %d\r\n", gU2UVCIsoinTxfUnitSize[thisVidIdx], PAYLOAD_LEN);
	}
#endif

	if (0 == pUvcPaylMakeComp) {
		DBG_ERR("Input NULL.Do nothing!!\r\n");
		return totalSize;
	} else {
		pDstAddr = pUvcPaylMakeComp->pDst;
		if (0 == pDstAddr) {
			DBG_ERR("NULL DST!!\r\n");
			return totalSize;
		}
		pSrcAddr = pUvcPaylMakeComp->pVidSrc;
		vidSize = pUvcPaylMakeComp->vidSize;
		toggleFid = pUvcPaylMakeComp->toggleFid;
		pHdrAddr = pUvcPaylMakeComp->pHdrAddr;
		hdrSize = pUvcPaylMakeComp->hdrSize;
		DbgMsg_UVCIO(("%s:src=0x%x,dst=0x%x,len=0x%x,fid=%d,hdr=0x%x,0x%x,unitSize=0x%x\r\n", __func__, pSrcAddr, pDstAddr, vidSize, toggleFid, pHdrAddr, hdrSize, unitSize));
	}

	if (pUvcPaylMakeComp->bIsH264 || vidSize <= unitSize) {
	//DBG_DUMP("%s:src=0x%x,dst=0x%x,len=0x%x,fid=%d,hdr=0x%x,0x%x,unitSize=0x%x\r\n", __func__, pSrcAddr, pDstAddr, vidSize, toggleFid, pHdrAddr, hdrSize, unitSize);
		tmpSize = UVCUtil_MakeHeadScrPts(pDstAddr, toggleFid);
		pLastPayloadHdrAddr = pDstAddr;
		pDstAddr = (UINT8 *)((UINT32)pDstAddr + tmpSize);
		totalSize = tmpSize;
		if (hdrSize && pHdrAddr) {
#if (_UVC_DBG_LVL_ >= _UVC_DBG_CHK_)
			if (gU2UVCIsoinTxfUnitSize[thisVidIdx] <= (hdrSize + PAYLOAD_LEN)) {
				DBG_ERR("Fail make payload, hdrSize/%d+%d=%d > txf-unit-size=%d\r\n", hdrSize, PAYLOAD_LEN, (hdrSize + PAYLOAD_LEN), gU2UVCIsoinTxfUnitSize);
			}
#endif
			if (user_data_size) {
				memcpy((void *)pDstAddr, (const void *)pSrcAddr, user_data_size);
				pDstAddr = (UINT8 *)((UINT32)pDstAddr + user_data_size);
				pSrcAddr = (UINT8 *)((UINT32)pSrcAddr + user_data_size);
			}
			//hwmem_memcpy((UINT32)pDstAddr, (UINT32)pHdrAddr, hdrSize);
			memcpy((void *)pDstAddr, (const void *)pHdrAddr, hdrSize);
			pDstAddr = (UINT8 *)((UINT32)pDstAddr + hdrSize);
			remainSize = unitSize - hdrSize - user_data_size;
			totalSize += hdrSize;
		} else {
			DbgMsg_UVCIO(("Null Head=0x%x,0x%x\r\n", pHdrAddr, hdrSize));
			remainSize = unitSize;
		}
		while (vidSize) {
			remainSize = (vidSize > remainSize) ? remainSize : vidSize;
			memcpy((void *)pDstAddr, (const void *)pSrcAddr, remainSize);
			vidSize -= remainSize;
			totalSize += remainSize;
			pDstAddr = (UINT8 *)((UINT32)pDstAddr + remainSize);
			pSrcAddr = (UINT8 *)((UINT32)pSrcAddr + remainSize);
			remainSize = unitSize;
			if (vidSize) {
				tmpSize = UVCUtil_MakeHeadScrPts(pDstAddr, toggleFid);
				pLastPayloadHdrAddr = pDstAddr;
				pDstAddr = (UINT8 *)((UINT32)pDstAddr + tmpSize);
				totalSize += tmpSize;
			}
		}
		*(pLastPayloadHdrAddr + 1) |= PAYLOAD_EOF;
		hd_common_mem_flush_cache((void *)pUvcPaylMakeComp->pDst, totalSize);
	} else { //MJPEG or YUV
		HD_GFX_COPY param = {0};
		UINT32 payload_cnt, i, src_img_pa;

#if UVC_MAKE_PAYLOAD_PERF
		h1 = hd_gettime_us();
#endif
		payload_cnt = (vidSize+(unitSize-1))/unitSize;
		//prepare payload header except last one
		for(i = 0; i < (payload_cnt-1); i++) {
			UVCUtil_MakeHeadScrPts(pDstAddr, toggleFid);
			hd_common_mem_flush_cache((void *)pDstAddr, PAYLOAD_LEN);
			pDstAddr += gU2UVCIsoinTxfUnitSize[thisVidIdx];
		}
		//the last payload header
		UVCUtil_MakeHeadScrPts(pDstAddr, toggleFid);
		*(pDstAddr + 1) |= PAYLOAD_EOF;
		hd_common_mem_flush_cache((void *)pDstAddr, PAYLOAD_LEN);

#if UVC_MAKE_PAYLOAD_PERF
		d1 = h2 = hd_gettime_us();
#endif
		{
			HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};
			vir_meminfo.va = (void *)pUvcPaylMakeComp->pVidSrc;
			if (hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) != HD_OK) {
				DBG_ERR("fail to convert va(0x%X) to pa.\r\n", (unsigned int)vir_meminfo.va);
				return 0;
			}
			src_img_pa = vir_meminfo.pa;
		}

		//hw-memcopy for payload data
		param.src_img.dim.w            = unitSize;
		param.src_img.dim.h            = payload_cnt;
		param.src_img.format           = HD_VIDEO_PXLFMT_Y8;
		param.src_img.p_phy_addr[0]    = src_img_pa;
		param.src_img.lineoffset[0]    = unitSize;
		param.dst_img.dim.w            = unitSize;
		param.dst_img.dim.h            = payload_cnt;
		param.dst_img.format           = HD_VIDEO_PXLFMT_Y8;
		param.dst_img.p_phy_addr[0]    = uvac_va_to_pa(UVAC_BUF_TYPE_WORK, (UINT32)pUvcPaylMakeComp->pDst + PAYLOAD_LEN);
		param.dst_img.lineoffset[0]    = unitSize + PAYLOAD_LEN;
		param.src_region.x             = 0;
		param.src_region.y             = 0;
		param.src_region.w             = param.src_img.dim.w;
		param.src_region.h             = param.src_img.dim.h;
		param.dst_pos.x                = 0;
		param.dst_pos.y                = 0;
		param.colorkey                 = 0;
		param.alpha                    = 255;
		//hd_gfx_copy(&param);
		vendor_gfx_copy_no_flush(&param);
#if UVC_MAKE_PAYLOAD_PERF
		d2 = hd_gettime_us();
#endif
		totalSize = vidSize + payload_cnt*PAYLOAD_LEN;
	}
#if UVC_MAKE_PAYLOAD_PERF
	out = hd_gettime_us();
	if (frm_cnt%150 ==0){
		DBG_DUMP("%dKB H[%llu]D[%llu]IO[%llu]us\r\n",totalSize/1024, h2-h1, d2-d1,out-in);
	}
#endif
	DbgMsg_UVCIO(("dst=0x%x/0x%x,src=0x%x/0x%x,tol=0x%x\r\n", pUvcPaylMakeComp->pDst, pDstAddr, pUvcPaylMakeComp->pVidSrc, pSrcAddr, totalSize));
	return totalSize;
}
#endif
void U2UVC_MakePayloadFmt(UVAC_VID_DEV_CNT  thisVidIdx, UVAC_TXF_INFO *pTxfInfo)
{
	UVC_PAYL_MAKE_COMP  uvcPaylMakeCom;
	UINT32 DMALen = 0;
	UVAC_VID_STRM_INFO *pStrmQueInfo = 0;

	if (pTxfInfo->sAddr == 0) {
		DBG_ERR("Stream NULL\r\n");
		return;
	}

	if (pTxfInfo->size == 0) {
		return;
	}
//DBG_DUMP("[%d]%X %X\r\n",__LINE__, *(UINT8 *)(guiVideoBufAddr+FirstSize-2),*(UINT8 *)(guiVideoBufAddr+FirstSize-1));
	if (pTxfInfo->size*105/100 > gVidTxBuf[thisVidIdx].size) {
		DBG_ERR("stream (%dKB) > TxBuf (%dKB)\r\n", pTxfInfo->size*105/100, gVidTxBuf[thisVidIdx].size / 1024);
		pTxfInfo->size = 0;
		return;
	}
	uvcPaylMakeCom.pVidSrc = (UINT8 *)pTxfInfo->sAddr;
	uvcPaylMakeCom.vidSize = pTxfInfo->size;
	uvcPaylMakeCom.pDst = (UINT8 *)gVidTxBuf[thisVidIdx].addr;
	uvcPaylMakeCom.toggleFid = gToggleFid[thisVidIdx];
	gToggleFid[thisVidIdx] ^= 1;
	#if 0
	if ((m_uiU2UvacDbgVid & UVAC_DBG_VID_QUE_CNT)) {
		DumpMem((UINT32)uvcPaylMakeCom.pVidSrc, 32, 16);
	}
	#endif
	//H264
	if (_is_iframe(uvcPaylMakeCom.pVidSrc)) {
		pStrmQueInfo = &gUvacVidStrmInfo[thisVidIdx];
		uvcPaylMakeCom.pHdrAddr = pStrmQueInfo->strmInfo.pStrmHdr;
		uvcPaylMakeCom.hdrSize = pStrmQueInfo->strmInfo.strmHdrSize;
	} else {
		uvcPaylMakeCom.pHdrAddr = 0;
		uvcPaylMakeCom.hdrSize = 0;
	}
	if (gU2UvcHWPayload[thisVidIdx]) {
		if (UVAC_VIDEO_FORMAT_H264 == gUvacCodecType[thisVidIdx]) {
			//0.update the first 4 byte to be 0x00 0x00 0x00 0x01
			//1.make the first payload, H264 header only
			DMALen = UVCIsoIn_ConcateH264Header(thisVidIdx, &uvcPaylMakeCom);
		} else {
			//MJPG

			hwmem_open();
			hwmem_memcpy(thisVidIdx, (UINT32)uvcPaylMakeCom.pDst, (UINT32)uvcPaylMakeCom.pVidSrc, uvcPaylMakeCom.vidSize);
			hwmem_close();
			DMALen = uvcPaylMakeCom.vidSize;
#if 0
			UINT8 *pTmpU8 = 0;
			pTmpU8 = uvcPaylMakeCom.pDst;

			if (*pTmpU8 != 0xFF || *(pTmpU8 + 1) != 0xD8 || *(pTmpU8 + DMALen - 2) != 0xFF || *(pTmpU8 + DMALen - 1) != 0xD9) {
				DBG_WRN("0x%X:%X %X %X %X\r\n", pTxfInfo->sAddr, *pTmpU8, *(pTmpU8 + 1), *(pTmpU8 + DMALen - 2), *(pTmpU8 + DMALen - 1));
			}
#endif
		}
	} else {
		if (UVAC_VIDEO_FORMAT_H264 == gUvacCodecType[thisVidIdx]) {
			uvcPaylMakeCom.bIsH264 = TRUE;
		} else {
			uvcPaylMakeCom.bIsH264 = FALSE;
		}
		DMALen = UVCIsoIn_MakeIsoPayloadTxfData(thisVidIdx, &uvcPaylMakeCom);
	}
//DBG_DUMP("[%d]%X %X\r\n",__LINE__, *(UINT8 *)(guiVideoBufAddr+FirstSize-2),*(UINT8 *)(guiVideoBufAddr+FirstSize-1));
	pTxfInfo->sAddr = pTxfInfo->oriAddr = uvac_va_to_pa(UVAC_BUF_TYPE_WORK, (UINT32)uvcPaylMakeCom.pDst);
	pTxfInfo->size = DMALen;
	if (DMALen > gVidTxBuf[thisVidIdx].size) {
		DBG_ERR("Payload Frame (%dKB) > TxBuf (%dKB)\r\n", DMALen / 1024, gVidTxBuf[thisVidIdx].size / 1024);
	}
}
/**
    UVAC Video Task.
*/
THREAD_DECLARE(U2UVAC_VideoTsk, arglist)
{
	UINT32              uiOutWidth = 0, uiOutHeight = 0, uiFps = 0;
	FLGPTN              uiFlag = 0;
	UINT32              totalFrmCnt = 0;
	UINT32              toggleFid = 0;
	UVAC_STRM_INFO      strmInfo = {0};
	ER                  retV = E_OK;
	UVAC_TXF_INFO       txfInfo;
	UINT32              tmpIdx = 0;
	UINT8               *pVidBuf;
	UINT32              vidSize;
	UINT32              thisVidIdx = UVAC_VID_DEV_CNT_1;
	UINT32              thisTxfQueIdx = UVAC_TXF_QUE_V1;
	PUVAC_VID_STRM_INFO pStrmQueInfo = &gUvacVidStrmInfo[thisVidIdx];
	UINT32 uiQueCnt;
	VOS_TICK tt1 = 0, tt2 = 0;

	vos_task_enter();
	while (1) {
		DbgMsg_UVC(("UVAC Tsk Init..\r\n"));
		set_flg(FLG_ID_U2UVAC, FLGUVAC_RDY);
		wai_flg(&uiFlag, FLG_ID_U2UVAC, FLGUVAC_START, TWF_ORW);        //Note we don't clear start flag!!!
		clr_flg(FLG_ID_U2UVAC, FLGUVAC_RDY);
		UVAC_GetImageSize(gUvacCodecType[thisVidIdx], gUvcParamAry[thisVidIdx].OutSizeID, thisVidIdx, &uiOutWidth, &uiOutHeight);
		uiFps = gUvcParamAry[thisVidIdx].FrameRate;

		DbgMsg_UVC(("^GVidTsk:W=%d,H=%d,Fr=%d,Fmt=%d,%d\r\n", uiOutWidth, uiOutHeight, uiFps, gUvacCodecType[thisVidIdx], gU2UvcNoVidStrm));
		//U2UVAC_DbgDmp_TxfInfoByQue(thisTxfQueIdx);
		UVAC_DbgDmp_MemLayout();

		UVC_TimeTrigSet(1000000 / uiFps, thisVidIdx);

		//CB to change video resolution, codec, fps
		if (g_fpStartVideo) {
			memset((void *)&gStrmInfo[thisVidIdx], 0, sizeof(UVAC_STRM_INFO));
			gStrmInfo[thisVidIdx].strmPath = UVAC_STRM_VID;

			pStrmQueInfo->strmInfo.strmCodec = gStrmInfo[thisVidIdx].strmCodec = gUvacCodecType[thisVidIdx];
			pStrmQueInfo->strmInfo.strmWidth = gStrmInfo[thisVidIdx].strmWidth = uiOutWidth;
			pStrmQueInfo->strmInfo.strmHeight = gStrmInfo[thisVidIdx].strmHeight = uiOutHeight;
			pStrmQueInfo->strmInfo.strmFps = gStrmInfo[thisVidIdx].strmFps = uiFps;
			pStrmQueInfo->strmInfo.strmResoIdx = gStrmInfo[thisVidIdx].strmResoIdx = gUvcParamAry[thisVidIdx].OutSizeID - 1;
			pStrmQueInfo->strmInfo.isStrmOn = gStrmInfo[thisVidIdx].isStrmOn = TRUE;
			if (UVAC_VIDEO_FORMAT_H264 == gUvacCodecType[thisVidIdx]) {
				pStrmQueInfo->strmInfo.strmTBR = gStrmInfo[thisVidIdx].strmTBR = gUvcH264TBR;
			} else {
				pStrmQueInfo->strmInfo.strmTBR = gStrmInfo[thisVidIdx].strmTBR = gUvcMJPGTBR;
			}

			U2UVAC_RemoveTxfInfo(thisTxfQueIdx);
			if (E_OK != g_fpStartVideo(thisVidIdx, &gStrmInfo[thisVidIdx])) {
				//Get the data of H264 Header,and check resolution is workale or not
				DBG_ERR("StartVid Fail, Reso=%d/%d, Codec=%d\r\n", uiOutWidth, uiOutHeight, gUvacCodecType[thisVidIdx]);
				clr_flg(FLG_ID_U2UVAC, FLGUVAC_START);
				UVC_TimeTrigClose(thisVidIdx);
				continue;
			}
		} else {
			U2UVAC_RemoveTxfInfo(thisTxfQueIdx);
			DBG_ERR("CB NULL. Can NOT change video resolution,codec,fps\r\n");
		}
		if (gUvacRunningFlag[thisVidIdx] & FLGUVAC_STOP) {
			DbgMsg_UVC(("Stop C...\r\n"));
			goto L_UVAC_END;
		}

		//U2UVAC_RemoveTxfInfo(thisTxfQueIdx);
		totalFrmCnt = toggleFid = 0;
		gToggleFid[thisVidIdx] = 0;
		uiQueCnt = 0;
		while (FALSE == gU2UvcNoVidStrm) {
			if (uvc_direct_trigger) {
				wai_flg(&uiFlag, FLG_ID_U2UVAC, FLGUVAC_VIDEO_TXF|FLGUVAC_VIDEO_STOP, (TWF_CLR | TWF_ORW));
				if (uiFlag & FLGUVAC_VIDEO_STOP) {
					DbgMsg_UVC(("^GVdoTsk1 Stop\r\n"));
					break;
				}
			} else {
				timer_wait_timeup(gUvacTimerID[thisVidIdx]);
			}
			DbgMsg_UVCIO(("11Uvac-timer:%d, %d\r\n", totalFrmCnt, toggleFid));
V1_SKIP_TIMER:
			//Send the first frame and it must be I frame
			tmpIdx = pStrmQueInfo->idxConsumer;
			DbgMsg_UVCIO(("StrmQue[%d]:cons=%d,prod=%d\r\n", thisVidIdx, pStrmQueInfo->idxConsumer, pStrmQueInfo->idxProducer));
			if (tmpIdx == pStrmQueInfo->idxProducer) {
				if (m_uiU2UvacDbgVid & UVAC_DBG_VID_START) {
					DBG_WRN("Empty StrmQue[%d]:con=%d,prod=%d, start=%d, NoStrm=%d\r\n", thisVidIdx, pStrmQueInfo->idxConsumer, pStrmQueInfo->idxProducer, gU2UvcVidStart[thisTxfQueIdx], gU2UvcNoVidStrm);
				}
				if (TRUE == gU2UvcVidStart[thisTxfQueIdx]) {
					continue;
				} else {
					break;
				}
			} else {
				DbgCode_UVC(if (0 == totalFrmCnt) {
				DbgMsg_UVC(("FirstFrm[%d]:cons=%d,prod=%d\r\n", thisVidIdx, pStrmQueInfo->idxConsumer, pStrmQueInfo->idxProducer));
				})
			}
			if (0 == totalFrmCnt && (m_uiU2UvacDbgVid & UVAC_DBG_VID_START)) {
				DBG_DUMP("idxProducer=%d, idxConsumer=%d\r\n", pStrmQueInfo->idxProducer, pStrmQueInfo->idxConsumer);
			}
			pVidBuf = (UINT8 *)(pStrmQueInfo->addr[tmpIdx]);
			vidSize = pStrmQueInfo->size[tmpIdx];
			if (0 == totalFrmCnt) {
				if (UVAC_VIDEO_FORMAT_MJPG == gUvacCodecType[thisVidIdx] || UVAC_VIDEO_FORMAT_YUV== gUvacCodecType[thisVidIdx]) {
					//point to the latest frame index
					pStrmQueInfo->idxConsumer = (pStrmQueInfo->idxProducer + UVAC_VID_INFO_QUE_MAX_CNT - 1) % UVAC_VID_INFO_QUE_MAX_CNT;
					tmpIdx = pStrmQueInfo->idxConsumer;
				} else { //UVAC_VIDEO_FORMAT_H264
					if ((0 == vidSize) || (FALSE == _is_iframe(pVidBuf))) {
						DbgMsg_UVCIO(("Chk I-Frm [%d]addr=0x%x,0x%x,cnt=0x%x\r\n", tmpIdx, (UINT32)pVidBuf, vidSize, totalFrmCnt));
						if ((m_uiU2UvacDbgVid & UVAC_DBG_VID_START)) {
							DBG_WRN("Wait 1st I-frm:%d, vidStrm exist=%d\r\n", tmpIdx, gU2UvcNoVidStrm);
						}
						pStrmQueInfo->idxConsumer = (tmpIdx + 1) & UVAC_VID_INFO_QUE_MAX_IDX;

						vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V1);
						continue;
					} else {
						//Get H264 header after the first frame;Send h264's header only
						/*if (g_fpGetH264Header) {
							g_fpGetH264Header(&gStrmInfo[thisVidIdx]);
							gH264Header[thisVidIdx].addr = (UINT32)gStrmInfo[thisVidIdx].pStrmHdr;
							gH264Header[thisVidIdx].size = gStrmInfo[thisVidIdx].strmHdrSize;
							if ((0 == gStrmInfo[thisVidIdx].pStrmHdr) || (0 == gStrmInfo[thisVidIdx].strmHdrSize)) {
								DBG_ERR("!!!H264 Header NULL:path=%d;\r\n", gStrmInfo[thisVidIdx].strmPath);
								break;
							}
						} else {
							DBG_ERR("fpGetH264HeaderCB NULL. Stop UVAC\r\n");
							break;
						}*/
					}
				}
			}
			pStrmQueInfo->idxConsumer = (tmpIdx + 1) & UVAC_VID_INFO_QUE_MAX_IDX;

			txfInfo.sAddr = txfInfo.oriAddr = (UINT32)pVidBuf;
			txfInfo.size = vidSize;
			txfInfo.usbEP = U2UVAC_USB_EP[thisTxfQueIdx];
			txfInfo.txfCnt = 0;
			txfInfo.streamType = UVAC_TXF_STREAM_VID;
			txfInfo.timestamp = pStrmQueInfo->timestamp;
			U2UVAC_AddIsoInTxfInfo(&txfInfo, thisTxfQueIdx);
			U2UVAC_IsoInTxfStateMachine(UVC_ISOIN_TXF_ACT_TXF);
			totalFrmCnt++;

			if ((m_uiU2UvacDbgVid & UVAC_DBG_VID_QUEUE)) {
				vos_perf_mark(&tt2);
				DBG_DUMP("[0]0x%X:%dKB %dms\r\n", txfInfo.sAddr, txfInfo.size/1024, vos_perf_duration(tt1, tt2)/ 1000);
				vos_perf_mark(&tt1);
			}
			if (gUvacRunningFlag[thisVidIdx] & FLGUVAC_STOP) {
				DbgMsg_UVC(("Stop B ...\r\n"));
				break;
			}

			if (usb_getControllerState() == USB_CONTROLLER_STATE_SUSPEND) {
				if ((usb_getControllerState() == USB_CONTROLLER_STATE_SUSPEND)) {
					DbgMsg_UVC(("Clr StartFlag!!\r\n"));
					gU2UvcUsbDMAAbord = TRUE;
					clr_flg(FLG_ID_U2UVAC, FLGUVAC_START);
				}
				DbgMsg_UVC(("Stop E ...\r\n"));
				break;
			}
			uiQueCnt = pStrmQueInfo->idxProducer + UVAC_VID_INFO_QUE_MAX_CNT - pStrmQueInfo->idxConsumer;
			if (uiQueCnt >= UVAC_VID_INFO_QUE_MAX_CNT) {
				uiQueCnt -= UVAC_VID_INFO_QUE_MAX_CNT;
			}
			if (m_uiU2UvacDbgVid & UVAC_DBG_VID_QUE_CNT) {
				DBG_DUMP("V1 Q=%d\r\n", uiQueCnt);
			}
			#if 0
			if (uiQueCnt > 1 && ((UVAC_VIDEO_FORMAT_MJPG == gUvacCodecType[thisVidIdx]) || (UVAC_VID_I_FRM_MARK == pVidBuf[UVAC_VID_I_FRM_MARK_POS]))) {
				if (m_uiU2UvacDbgVid & UVAC_DBG_VID_START) {
					DBG_DUMP("V1[%d] race!\r\n", totalFrmCnt);
				}
				goto V1_SKIP_TIMER;
			}
			#else
			if (uiQueCnt) {
				goto V1_SKIP_TIMER;
			}
			#endif

		}// end of wait timer //end of while (FALSE == gUvcNoVidStrm)

L_UVAC_END:
		clr_flg(FLG_ID_U2UVAC, FLGUVAC_START);
		DbgMsg_UVC(("+UVAC end:Fmt[0]=%d\n\r", gUvacCodecType[thisVidIdx]));
		//retV = usb_abortEndpoint(U2UVAC_USB_EP[thisTxfQueIdx]);
		DbgMsg_UVC(("--Abort EP#%d, retV=%d\n\r", U2UVAC_USB_EP[thisTxfQueIdx], retV));
		if (retV != E_OK) {
			DBG_ERR("Abort failed\n\r");
		}
		UVC_TimeTrigClose(thisVidIdx);
		DbgMsg_UVC(("+wait isoTsk :0x%x\n\r", uiFlag));
		//wai_flg(&uiFlag, FLG_ID_U2UVAC, FLGUVAC_ISOIN_READY, (TWF_CLR | TWF_ORW));

		if (0 == kchk_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_VDO_READY)) {
			vos_util_delay_ms(70);
		}

		{
			UINT32 count = 0;
			while (1) {
				if (count < 1000) {
					if (0 == kchk_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_VDO_READY)) {
						retV = usb_abortEndpoint(U2UVAC_USB_EP[thisTxfQueIdx]);
						if (retV != E_OK) {
							DBG_ERR("Abort failed\n\r");
						}
						DbgMsg_UVC(("\n\r#### abort EP[%d] ####\n\r\n\r", U2UVAC_USB_EP[thisTxfQueIdx]));
						vos_util_delay_ms(10);
					} else {
						break;
					}
				} else {
					DBG_ERR("wait ISOIN_VDO_READY failed\r\n");
					break;
				}
				count++;
			}
		}
		DbgMsg_UVC(("-wait isoTsk :0x%x\n\r", uiFlag));
		gUvacRunningFlag[thisVidIdx] = 0;
		clr_flg(FLG_ID_U2UVAC, FLGUVAC_STOP);
		//reset stream queue index
		pStrmQueInfo->idxProducer = 0;
		pStrmQueInfo->idxConsumer = 0;

		if (g_fpStopVideo) {
			g_fpStopVideo(thisVidIdx);
			pStrmQueInfo->strmInfo.isStrmOn = strmInfo.isStrmOn = FALSE;

		}
		#if ISF_LATENCY_DEBUG
		U2UVAC_DbgDmp_TimestampNumReset();
		#endif
		DbgMsg_UVC(("-UVAC end\n\r"));
	} // end for while(1)
	THREAD_RETURN(0);
}



/*
    Set USB UVAC frame rate.
*/
int U2UVAC_SetFrameRate(UINT16 uhFrameRate, UINT32 vidDevIdx)
{
	UVAC_PARAM *pUvacParam = &gUvcParamAry[vidDevIdx];
	DbgMsg_UVC(("+FRate=%d,%d, vidDevIdx=%d\r\n", uhFrameRate, pUvacParam->FrameRate, vidDevIdx));
	//pUvacParam->FrameRate = (uhFrameRate > UVC_FRMRATE_60) ? UVC_FRMRATE_60 : uhFrameRate;
	pUvacParam->FrameRate = uhFrameRate;
	DbgMsg_UVC(("-FRate=%d,%d\r\n", uhFrameRate, pUvacParam->FrameRate));
	return E_OK;
}


/*
    Set UVAC frame size.

    @param uhSize Frame size ID.
*/
int U2UVAC_SetImageSize(UINT16 frmIdx, UINT32 thisVidDevIdx)
{
	UINT32 uhSize = (UVC_VSFMT_PREV_FRM_IDX == frmIdx) ? gUvcParamAry[thisVidDevIdx].OutSizeID : frmIdx;
	DbgMsg_UVC(("^G+-SetImgSize=%d/%d,Prev=%d,thisVidDevIdx=%d\r\n", frmIdx, uhSize, gUvcParamAry[thisVidDevIdx].OutSizeID, thisVidDevIdx));
	gUvcParamAry[thisVidDevIdx].OutSizeID = uhSize;
	return E_OK;
}

/**
    Open USB PCC Task.

    @param[in] pClassInfo USB PCC task open parameters.

*/
UINT32 U2UVAC_Open(UVAC_INFO *pClassInfo)
{
	UINT32 i;
	UINT32 UvcBlkSize;
	UINT32 UacPacketSize = gU2UvacAudSampleRate[0] * gU2UacChNum * UAC_BIT_PER_SECOND / 8 / 1000; //Audio bit rate(byte per ms).
	char  *chip_name = getenv("NVT_CHIP_ID");

	UacPacketSize += 4;	//fix for compatibility with MacOS
	if (UacPacketSize < 36) {
		UacPacketSize = 36;
	}
	if (gUacMaxPacketSize){
		UacPacketSize = gUacMaxPacketSize;
	}
	DbgMsg_UVCIO(("+%s:0x%x\r\n", __func__, pClassInfo));
	UVAC_DbgDmp_VendDevDesc(gpImgUnitUvacVendDevDesc);
	UVAC_DbgDmp_UvacInfo(pClassInfo);

	if (NULL == pClassInfo) {
		DBG_ERR("!!!!!!Input NULL\r\n");
		DBG_ERR("!!!!!!Input NULL\r\n");
		return E_PAR;
	}
	U2UVAC_InstallID();

	if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51084") == 0) {
		if (timer_open((TIMER_ID *)&gUacTimerID[UVAC_AUD_DEV_CNT_1], (DRV_CB)_uac_timerhdl) != E_OK) {
			DBG_ERR("UAC timer open failed!\r\n");
			return E_SYS;
		}

		if (gU2UvacChannel == UVAC_CHANNEL_2V2A) {
			if (timer_open((TIMER_ID *)&gUacTimerID[UVAC_AUD_DEV_CNT_2], (DRV_CB)_uac_timerhdl2) != E_OK) {
				DBG_ERR("UAC2 timer open failed!\r\n");
				return E_SYS;
			}
		}
	}

	//xU2UVAC_InstallCmd();
	UVAC_ResetParam();
	guiUvacBufAddr = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pClassInfo->UvacMemAdr, pClassInfo->UvacMemSize);
	guiUvacBufAddr_pa = pClassInfo->UvacMemAdr;
	guiUvacBufTotalSize = pClassInfo->UvacMemSize;
	if (!guiUvacBufAddr || (guiUvacBufTotalSize < UVAC_GetNeedMemSize())) {
		DBG_ERR("!!!!!!!addr=0x%x,size=0x%x\r\n", guiUvacBufAddr, guiUvacBufTotalSize);
		DBG_ERR("!!!!!!!addr=0x%x,size=0x%x\r\n", guiUvacBufAddr, guiUvacBufTotalSize);
		return E_PAR;
	}
	if (guiUvacBufAddr % UVAC_BUF_ALIGN) {
		DBG_ERR("Input MemAddr not %d-byte alignment = 0x%x\r\n", UVAC_BUF_ALIGN, guiUvacBufAddr);
		guiUvacBufAddr += (UVAC_BUF_ALIGN-1);
		guiUvacBufAddr &= (UVAC_BUF_ALIGN-1);
		guiUvacBufTotalSize = (pClassInfo->UvacMemAdr + pClassInfo->UvacMemSize) - guiUvacBufAddr;
	}
	gU2UvcHWPayload[UVAC_VID_DEV_CNT_1] = pClassInfo->hwPayload[UVAC_VID_DEV_CNT_1];
	gU2UvcHWPayload[UVAC_VID_DEV_CNT_2] = pClassInfo->hwPayload[UVAC_VID_DEV_CNT_2];
	gU2UvacChannel = pClassInfo->channel;
	g_fpStartVideo = pClassInfo->fpStartVideoCB;
	g_fpStopVideo = pClassInfo->fpStopVideoCB;
	gU2UacSetVolCB = pClassInfo->fpSetVolCB;
	if (gU2UvacChannel > UVAC_CHANNEL_1V1A) {
		if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51084") == 0) {
			gU2UvcIsoInHsPacketSize = UVC_USB_FIFO_UNIT_SIZE;
			if (gU2UvacCdcEnabled) {
				gU2UvcIsoInBandWidth = BLKNUM_SINGLE;
			} else {
				gU2UvcIsoInBandWidth = BLKNUM_TRIPLE;
			}
		} else {
			gU2UvcIsoInHsPacketSize = UVC_ISOIN_HS_PACKET_SIZE;
			gU2UvcIsoInBandWidth = BLKNUM_DOUBLE;
		}
	} else if (gU2UvacChannel == UVAC_CHANNEL_1V1A && !gU2UvacCdcEnabled) {
		gU2UvcIsoInHsPacketSize = UVC_ISOIN_HS_PACKET_SIZE;
		gU2UvcIsoInBandWidth = BLKNUM_DOUBLE;
	} else {
		gU2UvcIsoInHsPacketSize = UVC_ISOIN_HS_PACKET_SIZE;
		gU2UvcIsoInBandWidth = BLKNUM_SINGLE;
	}
	UvcBlkSize = gU2UvcIsoInHsPacketSize;
	UVAC_SetupMem();
	U2UVAC_ResetTxfPara();//must be called after gU2UvcHWPayload and gUvacChannel

	DbgMsg_UVC(("UVAC:ch=%d,codec=%d,%d,adr=0x%x,size=0x%x,HWPL=%d,%d\r\n", pClassInfo->channel, gUvacCodecType[UVAC_VID_DEV_CNT_1], gUvacCodecType[UVAC_VID_DEV_CNT_2], guiUvacBufAddr, guiUvacBufTotalSize, gU2UvcHWPayload[UVAC_VID_DEV_CNT_1], gU2UvcHWPayload[UVAC_VID_DEV_CNT_2]));

	usb_initManagement(&gUSBMng);
	gUSBMng.num_of_strings = UVAC_SetIntfString();//should be prior to UVAC_MakeHSConfigDesc()
	U2UVAC_MakeHSConfigDesc(gpUvacHSConfigDesc, UVAC_MEM_DESC_SIZE);
	U2UVC_InitProbeCommitData();
	U2UVC_InitStillProbeCommitData(&gU2UvcStillProbeCommit);

	usb_setCallBack(USB_CALLBACK_CX_VENDOR_REQUEST, (USB_GENERIC_CB)U2UvacVendorRequestHandler);
	usb_setCallBack(USB_CALLBACK_CX_CLASS_REQUEST, (USB_GENERIC_CB)U2UvacClassRequestHandler);
	clr_flg(FLG_ID_U2UVAC, (FLGUVAC_START | FLGUVAC_STOP | FLGUVAC_START2 | FLGUVAC_STOP2));

	if (g_hid_info.en) {
		usb_setCallBack(USB_CALLBACK_STD_UNKNOWN_REQ, (USB_GENERIC_CB)UvacStdRequestHandler);
	}
	if (uvc_direct_trigger) {
		clr_flg(FLG_ID_U2UVAC, (FLGUVAC_VIDEO_TXF | FLGUVAC_VIDEO_STOP | FLGUVAC_VIDEO2_TXF | FLGUVAC_VIDEO2_STOP));
	}
	if (gU2UvacCdcEnabled) {
		clr_flg(FLG_ID_U2UVAC, (FLGUVAC_CDC_DATA_OUT | FLGUVAC_CDC_ABORT_READ | FLGUVAC_CDC_DATA2_OUT | FLGUVAC_CDC_ABORT_READ2));
	}

	vos_flag_set(FLG_ID_U2UVAC_FRM, (FLGUVAC_FRM_V1 | FLGUVAC_FRM_V2 | FLGUVAC_FRM_A1 | FLGUVAC_FRM_A2));

	gUSBMng.num_of_configurations = 1;
	gUSBMng.p_dev_desc = (USB_DEVICE_DESC *)&g_U2UVACDevDesc;
	gUSBMng.p_config_desc_hs = (USB_CONFIG_DESC *)gpUvacHSConfigDesc;

	gUSBMng.p_config_desc_fs = (USB_CONFIG_DESC *)&g_U2UVACFSConfigDescFBH264;
	gUSBMng.p_config_desc_fs_other = (USB_CONFIG_DESC *)&g_U2UVACFSOtherConfigDescFBH264;
	gUSBMng.p_config_desc_hs_other = (USB_CONFIG_DESC *)&g_U2UVACHSOtherConfigDescFBH264;
	gUSBMng.p_dev_quali_desc = (USB_DEVICE_DESC *)&g_U2UVACDevQualiDesc;
	gUSBMng.p_string_desc[0] = (USB_STRING_DESC *)&U2UvacStrDesc0;
	if (0 == gpImgUnitUvacVendDevDesc) {
		DBG_IND("Poj Not set string, use internal\r\n");
		gUSBMng.p_string_desc[1] = (USB_STRING_DESC *)&U2UvacStrDesc1;
		gUSBMng.p_string_desc[2] = (USB_STRING_DESC *)&U2UvacStrDesc2;
		gUSBMng.p_string_desc[3] = (USB_STRING_DESC *)&U2UvacStrDesc3;
	} else {
		if ((gpImgUnitUvacVendDevDesc->pManuStringDesc != NULL) && (gpImgUnitUvacVendDevDesc->pManuStringDesc->bDescriptorType == 0x03)) {
			gUSBMng.p_string_desc[1] = (USB_STRING_DESC *)(gpImgUnitUvacVendDevDesc->pManuStringDesc);
			UVAC_DbgDmp_StrDesc((UVAC_STRING_DESC *)gUSBMng.p_string_desc[1]);
		}
		if ((gpImgUnitUvacVendDevDesc->pProdStringDesc != NULL) && (gpImgUnitUvacVendDevDesc->pProdStringDesc->bDescriptorType == 0x03)) {
			gUSBMng.p_string_desc[2] = (USB_STRING_DESC *)(gpImgUnitUvacVendDevDesc->pProdStringDesc);
			UVAC_DbgDmp_StrDesc((UVAC_STRING_DESC *)gUSBMng.p_string_desc[2]);
		}
		if ((gpImgUnitUvacVendDevDesc->pSerialStringDesc != NULL) && (gpImgUnitUvacVendDevDesc->pSerialStringDesc->bDescriptorType == 0x03)) {
			gUSBMng.p_string_desc[3] = (USB_STRING_DESC *)(gpImgUnitUvacVendDevDesc->pSerialStringDesc);
			UVAC_DbgDmp_StrDesc((UVAC_STRING_DESC *)gUSBMng.p_string_desc[3]);
		}
	}

	//Clear all endpoints' setting
	for (i = 0; i < UVC_USB_MAX_ENDPOINT_IDX; i++) {
		gUSBMng.ep_config_hs[i].enable     = FALSE;
		gUSBMng.ep_config_fs[i].enable     = FALSE;
	}
	//HighBandWidth
	//EP1/Vid1,EP2/Vid2 or upload image,EP3/Aud1,EP4/Aud2,EP5/Intr-intf0,EP6/Intr-intf3,EP7/Cap-Img
	//EP1-UVC1
	gUSBMng.ep_config_fs[0].enable = TRUE;
	gUSBMng.ep_config_fs[0].blk_size = UVC_ISOIN_FS_PACKET_SIZE;
	gUSBMng.ep_config_fs[0].blk_num = UVC_ISOIN_BANDWIDTH;
	gUSBMng.ep_config_fs[0].direction = EP_DIR_IN;
	gUSBMng.ep_config_fs[0].trnsfer_type = EP_TYPE_ISOCHRONOUS;
	gUSBMng.ep_config_fs[0].max_pkt_size = UVC_ISOIN_FS_PACKET_SIZE;
	gUSBMng.ep_config_fs[0].high_bandwidth = UVC_ISOIN_BANDWIDTH;

	gUSBMng.ep_config_hs[0].blk_size         = UvcBlkSize;
	gUSBMng.ep_config_hs[0].blk_num          = gU2UvcIsoInBandWidth;
#if UVC_USB_UVC_FIFO_PINGPONG
	if (BLKNUM_SINGLE == gUSBMng.ep_config_hs[0].blk_num) {
		gUSBMng.ep_config_hs[0].blk_num *= 2;
	}
#endif
	gUSBMng.ep_config_hs[0].enable = TRUE;
	gUSBMng.ep_config_hs[0].direction = EP_DIR_IN;
	gUSBMng.ep_config_hs[0].trnsfer_type = EP_TYPE_ISOCHRONOUS;
	gUSBMng.ep_config_hs[0].max_pkt_size = UvcBlkSize;
	gUSBMng.ep_config_hs[0].high_bandwidth = gU2UvcIsoInBandWidth;
	DbgMsg_UVC(("\r\nEP1/Vid PackSize=0x%x,HB=%d,blkNum=%d,En=%d\r\n", UvcBlkSize, gU2UvcIsoInBandWidth, gUSBMng.ep_config_hs[0].blk_num, gUSBMng.ep_config_hs[0].enable));

	if (gDisableUac == FALSE) {
		//EP3-UAC1
		gUSBMng.ep_config_fs[2].enable = TRUE;
		gUSBMng.ep_config_fs[2].blk_size = UVC_ISOIN_AUD_FS_PACKET_SIZE;
		gUSBMng.ep_config_fs[2].blk_num = UVC_ISOIN_AUD_BANDWIDTH;
		gUSBMng.ep_config_fs[2].direction = EP_DIR_IN;
		gUSBMng.ep_config_fs[2].trnsfer_type = EP_TYPE_ISOCHRONOUS;
		gUSBMng.ep_config_fs[2].max_pkt_size = UVC_ISOIN_AUD_FS_PACKET_SIZE;
		gUSBMng.ep_config_fs[2].high_bandwidth = UVC_ISOIN_AUD_BANDWIDTH;
		gUSBMng.ep_config_hs[2].blk_num        = UVC_ISOIN_AUD_BANDWIDTH;
		gUSBMng.ep_config_hs[2].blk_size       = UVC_ISOIN_AUD_HS_PACKET_SIZE;
#if UVC_USB_UAC_FIFO_PINGPONG
		if (BLKNUM_SINGLE == gUSBMng.ep_config_hs[2].blk_num) {
			gUSBMng.ep_config_hs[2].blk_num *= 2;
		}
#endif
		gUSBMng.ep_config_hs[2].enable = TRUE;
		gUSBMng.ep_config_hs[2].direction = EP_DIR_IN;
		gUSBMng.ep_config_hs[2].trnsfer_type = EP_TYPE_ISOCHRONOUS;
		gUSBMng.ep_config_hs[2].max_pkt_size = UacPacketSize;
		gUSBMng.ep_config_hs[2].high_bandwidth = UVC_ISOIN_AUD_BANDWIDTH;
		DbgMsg_UVC(("^MEP3/Aud PackSize=0x%x,blk=%d,HB=%d,En=%d\r\n", UVC_ISOIN_AUD_HS_PACKET_SIZE, gUSBMng.ep_config_hs[2].blk_num, UVC_ISOIN_AUD_BANDWIDTH, gUSBMng.ep_config_hs[2].enable));
	}
	if (gU2UvacChannel > UVAC_CHANNEL_1V1A) {
		//EP2-UVC2
		gUSBMng.ep_config_fs[1].enable = TRUE;
		gUSBMng.ep_config_fs[1].blk_size = UVC_ISOIN_FS_PACKET_SIZE;
		gUSBMng.ep_config_fs[1].blk_num = UVC_ISOIN_BANDWIDTH;
		gUSBMng.ep_config_fs[1].direction = EP_DIR_IN;
		gUSBMng.ep_config_fs[1].trnsfer_type = EP_TYPE_ISOCHRONOUS;
		gUSBMng.ep_config_fs[1].max_pkt_size = UVC_ISOIN_FS_PACKET_SIZE;
		gUSBMng.ep_config_fs[1].high_bandwidth = UVC_ISOIN_BANDWIDTH;

		gUSBMng.ep_config_hs[1].blk_size         = UVC_USB_FIFO_UNIT_SIZE;
		gUSBMng.ep_config_hs[1].blk_num          = BLKNUM_SINGLE;
		gUSBMng.ep_config_hs[1].enable = TRUE;
		gUSBMng.ep_config_hs[1].direction = EP_DIR_IN;
		gUSBMng.ep_config_hs[1].trnsfer_type = EP_TYPE_ISOCHRONOUS;
		gUSBMng.ep_config_hs[1].max_pkt_size = UVC_USB_FIFO_UNIT_SIZE;
		gUSBMng.ep_config_hs[1].high_bandwidth = UVC_ISOIN_BANDWIDTH;
#if UVC_USB_UVC_FIFO_PINGPONG
		if (BLKNUM_SINGLE == gUSBMng.ep_config_hs[1].blk_num) {
			gUSBMng.ep_config_hs[1].blk_num *= 2;
		}
#endif
		DbgMsg_UVC(("^MEP2/Vid PackSize=0x%x,HB=%d,En=%d\r\n", UVC_USB_FIFO_UNIT_SIZE, UVC_ISOIN_BANDWIDTH, gUSBMng.ep_config_hs[1].enable));
	}
	if (gU2UvacChannel == UVAC_CHANNEL_2V2A && gDisableUac == FALSE) {
		if (g_hid_info.intr_out) {
			DBG_ERR("Not support 2V2A + HID intr_out\r\n");
			return E_SYS;
		}
	//EP4-UAC2
		gUSBMng.ep_config_fs[3].enable = TRUE;
		gUSBMng.ep_config_fs[3].blk_size = UVC_ISOIN_AUD_FS_PACKET_SIZE;
		gUSBMng.ep_config_fs[3].blk_num = UVC_ISOIN_AUD_BANDWIDTH;
		gUSBMng.ep_config_fs[3].direction = EP_DIR_IN;
		gUSBMng.ep_config_fs[3].trnsfer_type = EP_TYPE_ISOCHRONOUS;
		gUSBMng.ep_config_fs[3].max_pkt_size = UVC_ISOIN_AUD_FS_PACKET_SIZE;
		gUSBMng.ep_config_fs[3].high_bandwidth = UVC_ISOIN_AUD_BANDWIDTH;
		gUSBMng.ep_config_hs[3].blk_num        = UVC_ISOIN_AUD_BANDWIDTH;
		gUSBMng.ep_config_hs[3].blk_size       = UVC_ISOIN_AUD_HS_PACKET_SIZE;
#if UVC_USB_UAC_FIFO_PINGPONG
		if (BLKNUM_SINGLE == gUSBMng.ep_config_hs[3].blk_num) {
			gUSBMng.ep_config_hs[3].blk_num *= 2;
		}
#endif
		gUSBMng.ep_config_hs[3].enable = TRUE;
		gUSBMng.ep_config_hs[3].direction = EP_DIR_IN;
		gUSBMng.ep_config_hs[3].trnsfer_type = EP_TYPE_ISOCHRONOUS;
		gUSBMng.ep_config_hs[3].max_pkt_size = UacPacketSize;
		gUSBMng.ep_config_hs[3].high_bandwidth = UVC_ISOIN_AUD_BANDWIDTH;
		DbgMsg_UVC(("^MEP4/Aud PackSize=0x%x,HB=%d,blk=%d,En=%d\r\n", UVC_ISOIN_AUD_HS_PACKET_SIZE, UVC_ISOIN_AUD_BANDWIDTH, gUSBMng.ep_config_hs[3].blk_num, gUSBMng.ep_config_hs[3].enable));
	}
#if 0
	//EP5-Intr
	gUSBMng.ep_config_fs[4].enable = FALSE;
	gUSBMng.ep_config_fs[4].blk_size = 64;
	gUSBMng.ep_config_fs[4].blk_num = BLKNUM_NOT_USE;//BLKNUM_SINGLE;
	gUSBMng.ep_config_fs[4].direction = EP_DIR_IN;
	gUSBMng.ep_config_fs[4].trnsfer_type = EP_TYPE_INTERRUPT;
	gUSBMng.ep_config_fs[4].max_pkt_size = 64;
	gUSBMng.ep_config_fs[4].high_bandwidth = HBW_NOT;

	gUSBMng.ep_config_hs[4].enable = FALSE;
	gUSBMng.ep_config_hs[4].blk_size = 64;
	gUSBMng.ep_config_hs[4].blk_num = BLKNUM_NOT_USE;//BLKNUM_SINGLE;
	gUSBMng.ep_config_hs[4].direction = EP_DIR_IN;
	gUSBMng.ep_config_hs[4].trnsfer_type = EP_TYPE_INTERRUPT;
	gUSBMng.ep_config_hs[4].max_pkt_size = 64;
	gUSBMng.ep_config_hs[4].high_bandwidth = HBW_NOT;

	if (gUvacChannel > UVAC_CHANNEL_1V1A) {
		//EP6-Intr
		gUSBMng.ep_config_fs[5].enable = FALSE;
		gUSBMng.ep_config_fs[5].blk_size = 64;
		gUSBMng.ep_config_fs[5].blk_num = BLKNUM_NOT_USE;//BLKNUM_SINGLE;
		gUSBMng.ep_config_fs[5].direction = EP_DIR_IN;
		gUSBMng.ep_config_fs[5].trnsfer_type = EP_TYPE_INTERRUPT;
		gUSBMng.ep_config_fs[5].max_pkt_size = 64;
		gUSBMng.ep_config_fs[5].high_bandwidth = HBW_NOT;

		gUSBMng.ep_config_hs[5].enable = FALSE;
		gUSBMng.ep_config_hs[5].blk_size = 64;
		gUSBMng.ep_config_hs[5].blk_num = BLKNUM_NOT_USE;//BLKNUM_SINGLE;
		gUSBMng.ep_config_hs[5].direction = EP_DIR_IN;
		gUSBMng.ep_config_hs[5].trnsfer_type = EP_TYPE_INTERRUPT;
		gUSBMng.ep_config_hs[5].max_pkt_size = 64;
		gUSBMng.ep_config_hs[5].high_bandwidth = HBW_NOT;
	}
#endif
	if (g_hid_info.en) {
		if (gU2UvacChannel > UVAC_CHANNEL_1V1A) {
			DBG_ERR("Not support HID out with dual UVAC\r\n");
			return E_SYS;
		}
		gUSBMng.ep_config_hs[HID_INTRIN_EP - 1].enable           = TRUE;
		gUSBMng.ep_config_hs[HID_INTRIN_EP - 1].blk_size          = HID_EP_INTR_PACKET_SIZE;
		gUSBMng.ep_config_hs[HID_INTRIN_EP - 1].blk_num           = SINGLE_BLK;
		gUSBMng.ep_config_hs[HID_INTRIN_EP - 1].direction        = EP_DIR_IN;
		gUSBMng.ep_config_hs[HID_INTRIN_EP - 1].trnsfer_type      = EP_TYPE_INTERRUPT;
		gUSBMng.ep_config_hs[HID_INTRIN_EP - 1].max_pkt_size       = HID_EP_INTR_PACKET_SIZE;
		gUSBMng.ep_config_hs[HID_INTRIN_EP - 1].high_bandwidth    = HBW_NOT;

		gUSBMng.ep_config_fs[HID_INTRIN_EP - 1].enable           = TRUE;
		gUSBMng.ep_config_fs[HID_INTRIN_EP - 1].blk_size          = HID_EP_INTR_PACKET_SIZE;
		gUSBMng.ep_config_fs[HID_INTRIN_EP - 1].blk_num           = SINGLE_BLK;
		gUSBMng.ep_config_fs[HID_INTRIN_EP - 1].direction        = EP_DIR_IN;
		gUSBMng.ep_config_fs[HID_INTRIN_EP - 1].trnsfer_type      = EP_TYPE_INTERRUPT;
		gUSBMng.ep_config_fs[HID_INTRIN_EP - 1].max_pkt_size       = 64;
		gUSBMng.ep_config_fs[HID_INTRIN_EP - 1].high_bandwidth    = HBW_NOT;
		if (g_hid_info.intr_out) {
			gUSBMng.ep_config_hs[HID_INTROUT_EP - 1].enable           = TRUE;
			gUSBMng.ep_config_hs[HID_INTROUT_EP - 1].blk_size          = HID_EP_INTR_PACKET_SIZE;
			gUSBMng.ep_config_hs[HID_INTROUT_EP - 1].blk_num           = SINGLE_BLK;
			gUSBMng.ep_config_hs[HID_INTROUT_EP - 1].direction        = EP_DIR_OUT;
			gUSBMng.ep_config_hs[HID_INTROUT_EP - 1].trnsfer_type      = EP_TYPE_INTERRUPT;
			gUSBMng.ep_config_hs[HID_INTROUT_EP - 1].max_pkt_size       = HID_EP_INTR_PACKET_SIZE;
			gUSBMng.ep_config_hs[HID_INTROUT_EP - 1].high_bandwidth    = HBW_NOT;

			gUSBMng.ep_config_fs[HID_INTROUT_EP - 1].enable           = TRUE;
			gUSBMng.ep_config_fs[HID_INTROUT_EP - 1].blk_size          = HID_EP_INTR_PACKET_SIZE;
			gUSBMng.ep_config_fs[HID_INTROUT_EP - 1].blk_num           = SINGLE_BLK;
			gUSBMng.ep_config_fs[HID_INTROUT_EP - 1].direction        = EP_DIR_OUT;
			gUSBMng.ep_config_fs[HID_INTROUT_EP - 1].trnsfer_type      = EP_TYPE_INTERRUPT;
			gUSBMng.ep_config_fs[HID_INTROUT_EP - 1].max_pkt_size       = 64;
			gUSBMng.ep_config_fs[HID_INTROUT_EP - 1].high_bandwidth    = HBW_NOT;
		}
	}
	if (gU2UvacCdcEnabled) {
		// Config High Speed endpoint usage.
#if 0//USB_CDC_IF_COMM_NUMBER_EP
		gUSBMng.ep_config_hs[CDC_COMM_IN_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_hs[CDC_COMM_IN_EP - 1].blk_size         = 64;
		gUSBMng.ep_config_hs[CDC_COMM_IN_EP - 1].blk_num          = SINGLE_BLK;
		gUSBMng.ep_config_hs[CDC_COMM_IN_EP - 1].direction       = EP_DIR_IN;
		gUSBMng.ep_config_hs[CDC_COMM_IN_EP - 1].trnsfer_type     = EP_TYPE_INTERRUPT;
		gUSBMng.ep_config_hs[CDC_COMM_IN_EP - 1].max_pkt_size      = 64;
		gUSBMng.ep_config_hs[CDC_COMM_IN_EP - 1].high_bandwidth   = HBW_NOT;
#endif

		gUSBMng.ep_config_hs[CDC_DATA_IN_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_hs[CDC_DATA_IN_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_hs[CDC_DATA_IN_EP - 1].blk_num          = BLKNUM_SINGLE; //BLKNUM_DOUBLE;
		gUSBMng.ep_config_hs[CDC_DATA_IN_EP - 1].direction       = EP_DIR_IN;
		gUSBMng.ep_config_hs[CDC_DATA_IN_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_hs[CDC_DATA_IN_EP - 1].max_pkt_size      = 512;
		gUSBMng.ep_config_hs[CDC_DATA_IN_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_hs[CDC_DATA_OUT_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_hs[CDC_DATA_OUT_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_hs[CDC_DATA_OUT_EP - 1].blk_num          = BLKNUM_SINGLE; //BLKNUM_DOUBLE;
		gUSBMng.ep_config_hs[CDC_DATA_OUT_EP - 1].direction       = EP_DIR_OUT;
		gUSBMng.ep_config_hs[CDC_DATA_OUT_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_hs[CDC_DATA_OUT_EP - 1].max_pkt_size      = 512;
		gUSBMng.ep_config_hs[CDC_DATA_OUT_EP - 1].high_bandwidth   = HBW_NOT;

#if USB_2ND_CDC
#if 0//USB_CDC_IF_COMM_NUMBER_EP
		gUSBMng.ep_config_hs[CDC_COMM2_IN_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_hs[CDC_COMM2_IN_EP - 1].blk_size         = 64;
		gUSBMng.ep_config_hs[CDC_COMM2_IN_EP - 1].blk_num          = SINGLE_BLK;
		gUSBMng.ep_config_hs[CDC_COMM2_IN_EP - 1].direction       = EP_DIR_IN;
		gUSBMng.ep_config_hs[CDC_COMM2_IN_EP - 1].trnsfer_type     = EP_TYPE_INTERRUPT;
		gUSBMng.ep_config_hs[CDC_COMM2_IN_EP - 1].max_pkt_size      = 64;
		gUSBMng.ep_config_hs[CDC_COMM2_IN_EP - 1].high_bandwidth   = HBW_NOT;
#endif

		gUSBMng.ep_config_hs[CDC_DATA2_IN_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_hs[CDC_DATA2_IN_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_hs[CDC_DATA2_IN_EP - 1].blk_num          = BLKNUM_SINGLE; //BLKNUM_DOUBLE;
		gUSBMng.ep_config_hs[CDC_DATA2_IN_EP - 1].direction       = EP_DIR_IN;
		gUSBMng.ep_config_hs[CDC_DATA2_IN_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_hs[CDC_DATA2_IN_EP - 1].max_pkt_size      = 512;
		gUSBMng.ep_config_hs[CDC_DATA2_IN_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_hs[CDC_DATA2_OUT_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_hs[CDC_DATA2_OUT_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_hs[CDC_DATA2_OUT_EP - 1].blk_num          = BLKNUM_SINGLE; //BLKNUM_DOUBLE;
		gUSBMng.ep_config_hs[CDC_DATA2_OUT_EP - 1].direction       = EP_DIR_OUT;
		gUSBMng.ep_config_hs[CDC_DATA2_OUT_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_hs[CDC_DATA2_OUT_EP - 1].max_pkt_size      = 512;
		gUSBMng.ep_config_hs[CDC_DATA2_OUT_EP - 1].high_bandwidth   = HBW_NOT;
#endif
		// Config Full Speed endpoint usage.
		gUSBMng.ep_config_fs[CDC_COMM_IN_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_fs[CDC_COMM_IN_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_fs[CDC_COMM_IN_EP - 1].blk_num          = BLKNUM_DOUBLE;
		gUSBMng.ep_config_fs[CDC_COMM_IN_EP - 1].direction       = EP_DIR_OUT;
		gUSBMng.ep_config_fs[CDC_COMM_IN_EP - 1].trnsfer_type     = EP_TYPE_INTERRUPT;
		gUSBMng.ep_config_fs[CDC_COMM_IN_EP - 1].max_pkt_size      = 64;
		gUSBMng.ep_config_fs[CDC_COMM_IN_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_fs[CDC_DATA_IN_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_fs[CDC_DATA_IN_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_fs[CDC_DATA_IN_EP - 1].blk_num          = BLKNUM_DOUBLE;
		gUSBMng.ep_config_fs[CDC_DATA_IN_EP - 1].direction       = EP_DIR_IN;
		gUSBMng.ep_config_fs[CDC_DATA_IN_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_fs[CDC_DATA_IN_EP - 1].max_pkt_size      = 64;
		gUSBMng.ep_config_fs[CDC_DATA_IN_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_fs[CDC_DATA_OUT_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_fs[CDC_DATA_OUT_EP - 1].blk_size         = 512;
		gUSBMng.ep_config_fs[CDC_DATA_OUT_EP - 1].blk_num          = BLKNUM_DOUBLE;
		gUSBMng.ep_config_fs[CDC_DATA_OUT_EP - 1].direction       = EP_DIR_OUT;
		gUSBMng.ep_config_fs[CDC_DATA_OUT_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_fs[CDC_DATA_OUT_EP - 1].max_pkt_size      = 64;
		gUSBMng.ep_config_fs[CDC_DATA_OUT_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_fs[10].enable = FALSE;
		gUSBMng.ep_config_hs[10].enable = FALSE;
	#if 0//gU2UvacMtpEnabled
	} else if (gU2UvacMtpEnabled) {
		gUSBMng.ep_config_hs[SIDC_BULKIN_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_hs[SIDC_BULKIN_EP - 1].blk_size         = SIDC_EP_BULKIN_BLKSIZE;
		gUSBMng.ep_config_hs[SIDC_BULKIN_EP - 1].blk_num          = BLKNUM_SINGLE; //BLKNUM_DOUBLE;
		gUSBMng.ep_config_hs[SIDC_BULKIN_EP - 1].direction       = EP_DIR_IN;
		gUSBMng.ep_config_hs[SIDC_BULKIN_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_hs[SIDC_BULKIN_EP - 1].max_pkt_size      = SIDC_EP_IN_MAX_PACKET_SIZE;
		gUSBMng.ep_config_hs[SIDC_BULKIN_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_hs[SIDC_BULKOUT_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_hs[SIDC_BULKOUT_EP - 1].blk_size         = SIDC_EP_BULKOUT_BLKSIZE;
		gUSBMng.ep_config_hs[SIDC_BULKOUT_EP - 1].blk_num          = BLKNUM_SINGLE; //BLKNUM_DOUBLE;
		gUSBMng.ep_config_hs[SIDC_BULKOUT_EP - 1].direction       = EP_DIR_OUT;
		gUSBMng.ep_config_hs[SIDC_BULKOUT_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_hs[SIDC_BULKOUT_EP - 1].max_pkt_size      = SIDC_EP_OUT_MAX_PACKET_SIZE;
		gUSBMng.ep_config_hs[SIDC_BULKOUT_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_hs[SIDC_INTRIN_EP - 1].enable           = TRUE; //endpoint 3
		gUSBMng.ep_config_hs[SIDC_INTRIN_EP - 1].blk_size          = SIDC_EP_INTRIN_BLKSIZE;
		gUSBMng.ep_config_hs[SIDC_INTRIN_EP - 1].blk_num           = SINGLE_BLK;
		gUSBMng.ep_config_hs[SIDC_INTRIN_EP - 1].direction        = EP_DIR_IN;
		gUSBMng.ep_config_hs[SIDC_INTRIN_EP - 1].trnsfer_type      = EP_TYPE_INTERRUPT;
		gUSBMng.ep_config_hs[SIDC_INTRIN_EP - 1].max_pkt_size       = SIDC_EP_INTR_PACKET_SIZE;
		gUSBMng.ep_config_hs[SIDC_INTRIN_EP - 1].high_bandwidth    = HBW_NOT;

		// Config Full Speed endpoint usage.
		gUSBMng.ep_config_fs[SIDC_BULKIN_EP - 1].enable           = TRUE;
		gUSBMng.ep_config_fs[SIDC_BULKIN_EP - 1].blk_size          = SIDC_EP_BULKIN_BLKSIZE;
		gUSBMng.ep_config_fs[SIDC_BULKIN_EP - 1].blk_num           = BLKNUM_SINGLE; //BLKNUM_DOUBLE;
		gUSBMng.ep_config_fs[SIDC_BULKIN_EP - 1].direction        = EP_DIR_IN;
		gUSBMng.ep_config_fs[SIDC_BULKIN_EP - 1].trnsfer_type      = EP_TYPE_BULK;
		gUSBMng.ep_config_fs[SIDC_BULKIN_EP - 1].max_pkt_size       = 64;
		gUSBMng.ep_config_fs[SIDC_BULKIN_EP - 1].high_bandwidth    = HBW_NOT;

		gUSBMng.ep_config_fs[SIDC_BULKOUT_EP - 1].enable          = TRUE;
		gUSBMng.ep_config_fs[SIDC_BULKOUT_EP - 1].blk_size         = SIDC_EP_BULKOUT_BLKSIZE;
		gUSBMng.ep_config_fs[SIDC_BULKOUT_EP - 1].blk_num          = BLKNUM_SINGLE; //BLKNUM_DOUBLE;
		gUSBMng.ep_config_fs[SIDC_BULKOUT_EP - 1].direction       = EP_DIR_OUT;
		gUSBMng.ep_config_fs[SIDC_BULKOUT_EP - 1].trnsfer_type     = EP_TYPE_BULK;
		gUSBMng.ep_config_fs[SIDC_BULKOUT_EP - 1].max_pkt_size      = 64;
		gUSBMng.ep_config_fs[SIDC_BULKOUT_EP - 1].high_bandwidth   = HBW_NOT;

		gUSBMng.ep_config_fs[SIDC_INTRIN_EP - 1].enable           = TRUE; //endpoint 3
		gUSBMng.ep_config_fs[SIDC_INTRIN_EP - 1].blk_size          = SIDC_EP_INTRIN_BLKSIZE;
		gUSBMng.ep_config_fs[SIDC_INTRIN_EP - 1].blk_num           = SINGLE_BLK;
		gUSBMng.ep_config_fs[SIDC_INTRIN_EP - 1].direction        = EP_DIR_IN;
		gUSBMng.ep_config_fs[SIDC_INTRIN_EP - 1].trnsfer_type      = EP_TYPE_INTERRUPT;
		gUSBMng.ep_config_fs[SIDC_INTRIN_EP - 1].max_pkt_size       = 64;
		gUSBMng.ep_config_fs[SIDC_INTRIN_EP - 1].high_bandwidth    = HBW_NOT;
	#endif
	} else {
		gUSBMng.ep_config_fs[7].enable = FALSE;
		gUSBMng.ep_config_hs[7].enable = FALSE;
	}

	if (gU2UvacUacRxEnabled) {
		UacPacketSize = gU2UvacAudSampleRate[0] * gU2UacChNum * UAC_BIT_PER_SECOND / 8 / 1000;

		gUSBMng.ep_config_fs[U2UVAC_USB_RX_EP[0]-1].enable = TRUE;
		gUSBMng.ep_config_fs[U2UVAC_USB_RX_EP[0]-1].blk_size = UVC_ISOIN_AUD_FS_PACKET_SIZE;
		gUSBMng.ep_config_fs[U2UVAC_USB_RX_EP[0]-1].blk_num = UVC_ISOIN_AUD_BANDWIDTH;
		gUSBMng.ep_config_fs[U2UVAC_USB_RX_EP[0]-1].direction = EP_DIR_OUT;
		gUSBMng.ep_config_fs[U2UVAC_USB_RX_EP[0]-1].trnsfer_type = EP_TYPE_ISOCHRONOUS;
		gUSBMng.ep_config_fs[U2UVAC_USB_RX_EP[0]-1].max_pkt_size = UVC_ISOIN_AUD_FS_PACKET_SIZE;
		gUSBMng.ep_config_fs[U2UVAC_USB_RX_EP[0]-1].high_bandwidth = UVC_ISOIN_AUD_BANDWIDTH;

		gUSBMng.ep_config_hs[U2UVAC_USB_RX_EP[0]-1].blk_num        = UVC_ISOIN_AUD_BANDWIDTH;
		gUSBMng.ep_config_hs[U2UVAC_USB_RX_EP[0]-1].blk_size       = UVC_ISOIN_AUD_HS_PACKET_SIZE;
		gUSBMng.ep_config_hs[U2UVAC_USB_RX_EP[0]-1].enable = TRUE;
		gUSBMng.ep_config_hs[U2UVAC_USB_RX_EP[0]-1].direction = EP_DIR_OUT;
		gUSBMng.ep_config_hs[U2UVAC_USB_RX_EP[0]-1].trnsfer_type = EP_TYPE_ISOCHRONOUS;
		gUSBMng.ep_config_hs[U2UVAC_USB_RX_EP[0]-1].max_pkt_size = UacPacketSize;
		gUSBMng.ep_config_hs[U2UVAC_USB_RX_EP[0]-1].high_bandwidth = UVC_ISOIN_AUD_BANDWIDTH;
	}

	if (g_msdc_info.en) {
		gUSBMng.ep_config_fs[MSDC_IN_EP-1].enable = TRUE;
		gUSBMng.ep_config_fs[MSDC_IN_EP-1].blk_size = 512;
		gUSBMng.ep_config_fs[MSDC_IN_EP-1].blk_num = BLKNUM_SINGLE;
		gUSBMng.ep_config_fs[MSDC_IN_EP-1].direction = EP_DIR_IN;
		gUSBMng.ep_config_fs[MSDC_IN_EP-1].trnsfer_type = EP_TYPE_BULK;
		gUSBMng.ep_config_fs[MSDC_IN_EP-1].max_pkt_size = 64;
		gUSBMng.ep_config_fs[MSDC_IN_EP-1].high_bandwidth = HBW_NOT;

		gUSBMng.ep_config_hs[MSDC_IN_EP-1].blk_num        = BLKNUM_SINGLE;
		gUSBMng.ep_config_hs[MSDC_IN_EP-1].blk_size       = 512;
		gUSBMng.ep_config_hs[MSDC_IN_EP-1].enable = TRUE;
		gUSBMng.ep_config_hs[MSDC_IN_EP-1].direction = EP_DIR_IN;
		gUSBMng.ep_config_hs[MSDC_IN_EP-1].trnsfer_type = EP_TYPE_BULK;
		gUSBMng.ep_config_hs[MSDC_IN_EP-1].max_pkt_size = 512;
		gUSBMng.ep_config_hs[MSDC_IN_EP-1].high_bandwidth = HBW_NOT;

		gUSBMng.ep_config_fs[MSDC_OUT_EP-1].enable = TRUE;
		gUSBMng.ep_config_fs[MSDC_OUT_EP-1].blk_size = 512;
		gUSBMng.ep_config_fs[MSDC_OUT_EP-1].blk_num = BLKNUM_SINGLE;
		gUSBMng.ep_config_fs[MSDC_OUT_EP-1].direction = EP_DIR_OUT;
		gUSBMng.ep_config_fs[MSDC_OUT_EP-1].trnsfer_type = EP_TYPE_BULK;
		gUSBMng.ep_config_fs[MSDC_OUT_EP-1].max_pkt_size = 64;
		gUSBMng.ep_config_fs[MSDC_OUT_EP-1].high_bandwidth = HBW_NOT;

		gUSBMng.ep_config_hs[MSDC_OUT_EP-1].blk_num        = BLKNUM_SINGLE;
		gUSBMng.ep_config_hs[MSDC_OUT_EP-1].blk_size       = 512;
		gUSBMng.ep_config_hs[MSDC_OUT_EP-1].enable = TRUE;
		gUSBMng.ep_config_hs[MSDC_OUT_EP-1].direction = EP_DIR_OUT;
		gUSBMng.ep_config_hs[MSDC_OUT_EP-1].trnsfer_type = EP_TYPE_BULK;
		gUSBMng.ep_config_hs[MSDC_OUT_EP-1].max_pkt_size = 512;
		gUSBMng.ep_config_hs[MSDC_OUT_EP-1].high_bandwidth = HBW_NOT;
	}

	gUSBMng.fp_event_callback = UVAC_Callback;
	gUSBMng.fp_usb_suspend = NULL;
	gUSBMng.fp_open_needed_fifo = UVAC_OpenNeededFIFO;
	gUSBMng.usb_type = USB_PCC;//USB_TEST;
	usb_setCallBack(USB_CALLBACK_SET_INTERFACE, UVAC_InterfaceCB);
	usb_setManagement(&gUSBMng);

	DBG_IND("Open USB Driver\r\n");
	if (E_OK == usb_open()) {
		U2UVACVIDEOTSK_ID = vos_task_create(U2UVAC_VideoTsk, 0, "UVAC_VideoTsk", PRI_UVACVIDEO, STKSIZE_UVACVIDEO);
		if (0 == U2UVACVIDEOTSK_ID) {
			DBG_ERR("VideoTsk create failed\r\n");
	        return E_SYS;
		}
		#if 0
		U2UVACISOINTSK_ID = vos_task_create(U2UVAC_IsoInTsk, 0, "UVAC_IsoInTsk", PRI_UVACISOIN, STKSIZE_UVACISOIN);
		if (0 == U2UVACISOINTSK_ID) {
			DBG_ERR("VideoTsk create failed\r\n");
	        return E_SYS;
		}
		vos_task_resume(U2UVACISOINTSK_ID);
		#endif
		U2UVAC_TX_VDO1_ID = vos_task_create(U2UVAC_IsoInVdo1Tsk, 0, "UVAC_IsoInVdo1Tsk", PRI_UVACISOIN, STKSIZE_UVACISOIN_VID);
		if (0 == U2UVAC_TX_VDO1_ID) {
			DBG_ERR("U2UVAC_IsoInVdo1Tsk create failed\r\n");
	        return E_SYS;
		}
		vos_task_resume(U2UVAC_TX_VDO1_ID);
		U2UVAC_TX_AUD1_ID = vos_task_create(U2UVAC_IsoInAud1Tsk, 0, "UVAC_IsoInAud1Tsk", PRI_UVACISOIN, STKSIZE_UVACISOIN_AUD);
		if (0 == U2UVAC_TX_AUD1_ID) {
			DBG_ERR("U2UVAC_IsoInAud1Tsk create failed\r\n");
	        return E_SYS;
		}
		vos_task_resume(U2UVAC_TX_AUD1_ID);

		vos_task_resume(U2UVACVIDEOTSK_ID);
		if (gU2UvacChannel > UVAC_CHANNEL_1V1A) {
			U2UVACVIDEOTSK_ID2 = vos_task_create(U2UVAC_VideoTsk2, 0, "UVAC_VideoTsk2", PRI_UVACVIDEO, STKSIZE_UVACVIDEO);
			if (0 == U2UVACVIDEOTSK_ID2) {
				DBG_ERR("VideoTsk create failed\r\n");
		        return E_SYS;
			}
			vos_task_resume(U2UVACVIDEOTSK_ID2);

			U2UVAC_TX_VDO2_ID = vos_task_create(U2UVAC_IsoInVdo2Tsk, 0, "UVAC_IsoInVdo2Tsk", PRI_UVACISOIN, STKSIZE_UVACISOIN_VID);
			if (0 == U2UVAC_TX_VDO2_ID) {
				DBG_ERR("U2UVAC_IsoInVdo2Tsk create failed\r\n");
		        return E_SYS;
			}
			vos_task_resume(U2UVAC_TX_VDO2_ID);
			if (gU2UvacChannel == UVAC_CHANNEL_2V2A) {
				U2UVAC_TX_AUD2_ID = vos_task_create(U2UVAC_IsoInAud2Tsk, 0, "UVAC_IsoInAud2Tsk", PRI_UVACISOIN, STKSIZE_UVACISOIN_AUD);
				if (0 == U2UVAC_TX_AUD2_ID) {
					DBG_ERR("U2UVAC_IsoInAud2Tsk create failed\r\n");
			        return E_SYS;
				}
				vos_task_resume(U2UVAC_TX_AUD2_ID);
			}
		}
		#if 0//gU2UvacMtpEnabled
		if (gU2UvacMtpEnabled) {
			sta_tsk(U2UVACSIDCTSK_ID, 0);
			sta_tsk(U2UVACSIDCINTRTSK_ID, 0);
		}
		#endif

		if (gU2UvacUacRxEnabled) {
			U2UVAC_UAC_RX_ID = vos_task_create(U2UVAC_AudioRxTsk, 0, "UVAC_AudRxTsk", PRI_UVACISOOUT, STKSIZE_UVACISOOUT);
			if (0 == U2UVAC_UAC_RX_ID) {
				DBG_ERR("AudRxTsk create failed\r\n");
		        return E_SYS;
			}
			vos_task_resume(U2UVAC_UAC_RX_ID);
		}

		gUvacOpened = TRUE;
		DbgMsg_UVCIO(("-%s\r\n", __func__));
		return E_OK;
	} else {
		U2UVAC_UnInstallID();
		return E_SYS;
	}
}

/**
    Close USB PCC Task.
*/

void U2UVAC_Close(void)
{
	DbgMsg_UVC((":0x%x++\n\r", gUvacOpened));

	if (gUvacOpened) {
		gUvacOpened = FALSE;
	} else {
		DBG_ERR("UVAC not opened\r\n");
		return;                          //task aren't running. so we forget this request...
	}
	wai_sem(SEMID_U2UVC_WRITE_CDC);
	UVAC_StopAll();

	if (gU2UvacUacRxEnabled) {
		FLGPTN uiFlag = 0;

		usb_abortEndpoint(U2UVAC_USB_RX_EP[0]);

		_U2UVAC_unlock_pullqueue(UVAC_STRM_AUDRX);
		set_flg(FLG_ID_U2UVAC_UAC_RX, FLGUVAC_AUD_EXIT);
		wai_flg(&uiFlag, FLG_ID_U2UVAC_UAC_RX, FLGUVAC_AUD_IDLE, (TWF_CLR | TWF_ORW));
	}

	usb_close();

	memset((void *)gU2UvacEUVendCmdCB, 0, (sizeof(UVAC_EUVENDCMDCB) * UVAC_EU_VENDCMD_CNT));
	//#NT#2017/08/15#Ben Wang -begin
	//#NT#Patch for ImageUnit_UsbUVAC "restart" function, these flags didn't need reset also.
	//gpImgUnitUvacVendDevDesc = 0;
	gU2UvacWinIntrfEnable = FALSE;
	//gUvcVideoFmtType = UVAC_VIDEO_FORMAT_H264_MJPEG;
	//#NT#2017/08/15#Ben Wang -end
	if (gU2UvacCdcEnabled) {
		set_flg(FLG_ID_U2UVAC, FLGUVAC_CDC_ABORT_READ);
		set_flg(FLG_ID_U2UVAC, FLGUVAC_CDC_ABORT_READ2);
		//gU2UvacCdcEnabled = FALSE;
	}
	sig_sem(SEMID_U2UVC_WRITE_CDC);


	if (gUacTimerID[UVAC_AUD_DEV_CNT_1] != TIMER_NUM) {
		timer_close(gUacTimerID[UVAC_AUD_DEV_CNT_1]);
		gUacTimerID[UVAC_AUD_DEV_CNT_1] = TIMER_NUM;
	}

	if (gU2UvacChannel == UVAC_CHANNEL_1V1A) {
		if (gUacTimerID[UVAC_AUD_DEV_CNT_2] != TIMER_NUM) {
			timer_close(gUacTimerID[UVAC_AUD_DEV_CNT_2]);
			gUacTimerID[UVAC_AUD_DEV_CNT_2] = TIMER_NUM;
		}
	}

	hd_common_mem_munmap((void *)guiUvacBufAddr, guiUvacBufTotalSize);
	if (gUvcVidBuf[UVAC_VID_DEV_CNT_1].size){
		hd_common_mem_munmap((void *)gUvcVidBuf[UVAC_VID_DEV_CNT_1].va, gUvcVidBuf[UVAC_VID_DEV_CNT_1].size);
	}
	if (gUvcVidBuf[UVAC_VID_DEV_CNT_2].size){
		hd_common_mem_munmap((void *)gUvcVidBuf[UVAC_VID_DEV_CNT_2].va, gUvcVidBuf[UVAC_VID_DEV_CNT_2].size);
	}
	U2UVAC_UnInstallID();
	DbgMsg_UVC(("-%s:0x%x,trig=%d,ch=%d\n\r", __func__, gUvacOpened, gU2UVcTrIgEnabled, gU2UvacChannel));
}
/**
    UVAC Video Task 2.
*/
THREAD_DECLARE(U2UVAC_VideoTsk2, arglist)
{
	UINT32              uiOutWidth = 0, uiOutHeight = 0, uiFps = 0;
	FLGPTN              uiFlag = 0;
	UINT32              totalFrmCnt = 0;
	UINT32              toggleFid = 0;
	UVAC_STRM_INFO      strmInfo = {0};
	ER                  retV = E_OK;
	UVAC_TXF_INFO       txfInfo;
	UINT32              tmpIdx = 0;
	UINT8               *pVidBuf;
	UINT32              vidSize;
	UINT32              thisVidIdx = UVAC_VID_DEV_CNT_2;
	UINT32              thisTxfQueIdx = UVAC_TXF_QUE_V2;
	PUVAC_VID_STRM_INFO  pStrmQueInfo = &gUvacVidStrmInfo[thisVidIdx];
	UINT32 uiQueCnt;
	VOS_TICK tt1 = 0, tt2 = 0;

	vos_task_enter();
	while (1) {
		DbgMsg_UVC(("UVAC Tsk2 Init..\r\n"));
		set_flg(FLG_ID_U2UVAC, FLGUVAC_RDY2);
		wai_flg(&uiFlag, FLG_ID_U2UVAC, FLGUVAC_START2, TWF_ORW);        //Note we don't clear start flag!!!
		clr_flg(FLG_ID_U2UVAC, FLGUVAC_RDY2);
		UVAC_GetImageSize(gUvacCodecType[thisVidIdx], gUvcParamAry[thisVidIdx].OutSizeID, thisVidIdx, &uiOutWidth, &uiOutHeight);
		uiFps = gUvcParamAry[thisVidIdx].FrameRate;

		DBG_DUMP("^GVidTsk2:W=%d,H=%d,Fr=%d,Fmt=%d,%d, HWP=%d\r\n", uiOutWidth, uiOutHeight, uiFps, gUvacCodecType[thisVidIdx], gU2UvcNoVidStrm, gU2UvcHWPayload[thisVidIdx]);
		//U2UVAC_DbgDmp_TxfInfoByQue(thisTxfQueIdx);
		UVAC_DbgDmp_MemLayout();

		UVC_TimeTrigSet(1000000 / uiFps, thisVidIdx);

		//CB to change video resolution, codec, fps
		if (g_fpStartVideo) {
			memset((void *)&gStrmInfo[thisVidIdx], 0, sizeof(UVAC_STRM_INFO));
			gStrmInfo[thisVidIdx].strmPath = UVAC_STRM_VID2;

			pStrmQueInfo->strmInfo.strmCodec = gStrmInfo[thisVidIdx].strmCodec = gUvacCodecType[thisVidIdx];
			pStrmQueInfo->strmInfo.strmWidth = gStrmInfo[thisVidIdx].strmWidth = uiOutWidth;
			pStrmQueInfo->strmInfo.strmHeight = gStrmInfo[thisVidIdx].strmHeight = uiOutHeight;
			pStrmQueInfo->strmInfo.strmFps = gStrmInfo[thisVidIdx].strmFps = uiFps;
			pStrmQueInfo->strmInfo.strmResoIdx = gStrmInfo[thisVidIdx].strmResoIdx = gUvcParamAry[thisVidIdx].OutSizeID - 1;
			pStrmQueInfo->strmInfo.isStrmOn = gStrmInfo[thisVidIdx].isStrmOn = TRUE;
			if (UVAC_VIDEO_FORMAT_H264 == gUvacCodecType[thisVidIdx]) {
				pStrmQueInfo->strmInfo.strmTBR = gStrmInfo[thisVidIdx].strmTBR = gUvcH264TBR;
			} else {
				pStrmQueInfo->strmInfo.strmTBR = gStrmInfo[thisVidIdx].strmTBR = gUvcMJPGTBR;
			}

			U2UVAC_RemoveTxfInfo(thisTxfQueIdx);
			if (E_OK != g_fpStartVideo(thisVidIdx, &gStrmInfo[thisVidIdx])) {
				//Get the data of H264 Header,and check resolution is workale or not
				DBG_ERR("StartVid Fail, Reso=%d/%d, Codec=%d\r\n", uiOutWidth, uiOutHeight, gUvacCodecType[thisVidIdx]);
				clr_flg(FLG_ID_U2UVAC, FLGUVAC_START2);
				UVC_TimeTrigClose(thisVidIdx);
				continue;
			}
		} else {
			U2UVAC_RemoveTxfInfo(thisTxfQueIdx);
			DBG_ERR("CB NULL. Can NOT change video resolution,codec,fps\r\n");
		}

		if (gUvacRunningFlag[thisVidIdx] & FLGUVAC_STOP2) {
			DbgMsg_UVC(("Tsk2 Stop C...\r\n"));
			goto L_UVAC_END2;
		}

		//U2UVAC_RemoveTxfInfo(thisTxfQueIdx);
		totalFrmCnt = toggleFid = 0;
		gToggleFid[thisVidIdx] = 0;
		uiQueCnt = 0;
		while (FALSE == gU2UvcNoVidStrm) {
			if (uvc_direct_trigger) {
				wai_flg(&uiFlag, FLG_ID_U2UVAC, FLGUVAC_VIDEO2_TXF|FLGUVAC_VIDEO2_STOP, (TWF_CLR | TWF_ORW));
				if (uiFlag & FLGUVAC_VIDEO2_STOP) {
					DbgMsg_UVC(("^GVdoTsk2 Stop\r\n"));
					break;
				}
			} else {
				timer_wait_timeup(gUvacTimerID[thisVidIdx]);
			}
			DbgMsg_UVCIO(("22Uvac-timer:%d\r\n", totalFrmCnt));
V2_SKIP_TIMER:
			//Send the first frame and it must be I frame
			tmpIdx = pStrmQueInfo->idxConsumer;
			DbgMsg_UVCIO(("22StrmQue[%d]:cons=%d,prod=%d,codec=%d\r\n", thisVidIdx, pStrmQueInfo->idxConsumer, pStrmQueInfo->idxProducer, gUvacCodecType[thisVidIdx]));
			if (tmpIdx == pStrmQueInfo->idxProducer) {
				if (m_uiU2UvacDbgVid & UVAC_DBG_VID_START) {
					DBG_WRN("22Empty StrmQue[%d]:con=%d,prod=%d, start=%d, NoStrm=%d\r\n", thisVidIdx, pStrmQueInfo->idxConsumer, pStrmQueInfo->idxProducer, gU2UvcVidStart[thisTxfQueIdx], gU2UvcNoVidStrm);
				}
				if (TRUE == gU2UvcVidStart[thisTxfQueIdx]) {
					continue;
				} else {
					break;
				}
			} else {
				DbgCode_UVC(if (0 == totalFrmCnt) {
				DbgMsg_UVC(("22FirstFrm[%d]:cons=%d,prod=%d\r\n", thisVidIdx, pStrmQueInfo->idxConsumer, pStrmQueInfo->idxProducer));
				})
			}
			if (0 == totalFrmCnt && (m_uiU2UvacDbgVid & UVAC_DBG_VID_START)) {
				DBG_DUMP("idxProducer=%d, idxConsumer=%d\r\n", pStrmQueInfo->idxProducer, pStrmQueInfo->idxConsumer);
			}
			pVidBuf = (UINT8 *)(pStrmQueInfo->addr[tmpIdx]);
			vidSize = pStrmQueInfo->size[tmpIdx];
			if (0 == totalFrmCnt) {
				if (UVAC_VIDEO_FORMAT_MJPG == gUvacCodecType[thisVidIdx] || UVAC_VIDEO_FORMAT_YUV== gUvacCodecType[thisVidIdx]) {
					//point to the latest frame index
					pStrmQueInfo->idxConsumer = (pStrmQueInfo->idxProducer + UVAC_VID_INFO_QUE_MAX_CNT - 1) % UVAC_VID_INFO_QUE_MAX_CNT;
					tmpIdx = pStrmQueInfo->idxConsumer;
				} else { //UVAC_VIDEO_FORMAT_H264
					if ((0 == vidSize) || (FALSE == _is_iframe(pVidBuf))) {
						DbgMsg_UVCIO(("Chk I-Frm [%d]addr=0x%x,0x%x,cnt=0x%x\r\n", tmpIdx, (UINT32)pVidBuf, vidSize, totalFrmCnt));
						if ((m_uiU2UvacDbgVid & UVAC_DBG_VID_START)) {
							DBG_WRN("Wait 1st I-frm:%d, vidStrm exist=%d\r\n", tmpIdx, gU2UvcNoVidStrm);
						}
						pStrmQueInfo->idxConsumer = (tmpIdx + 1) & UVAC_VID_INFO_QUE_MAX_IDX;

						vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V2);
						continue;
					} /*else {
						//Get H264 header after the first frame;Send h264's header only
						if (g_fpGetH264Header) {
							g_fpGetH264Header(&gStrmInfo[thisVidIdx]);
							gH264Header[thisVidIdx].addr = (UINT32)gStrmInfo[thisVidIdx].pStrmHdr;
							gH264Header[thisVidIdx].size = gStrmInfo[thisVidIdx].strmHdrSize;
							if ((0 == gStrmInfo[thisVidIdx].pStrmHdr) || (0 == gStrmInfo[thisVidIdx].strmHdrSize)) {
								DBG_ERR("!!!H264 Header NULL:path=%d;\r\n", gStrmInfo[thisVidIdx].strmPath);
								break;
							}
						} else {
							DBG_ERR("fpGetH264HeaderCB NULL. Stop UVAC\r\n");
							break;
						}
					}*/
				}
			}
			pStrmQueInfo->idxConsumer = (tmpIdx + 1) & UVAC_VID_INFO_QUE_MAX_IDX;

			txfInfo.sAddr = txfInfo.oriAddr = (UINT32)pVidBuf;
			txfInfo.size = vidSize;
			txfInfo.usbEP = U2UVAC_USB_EP[thisTxfQueIdx];
			txfInfo.txfCnt = 0;
			txfInfo.streamType = UVAC_TXF_STREAM_VID;
			txfInfo.timestamp = pStrmQueInfo->timestamp;

			U2UVAC_AddIsoInTxfInfo(&txfInfo, thisTxfQueIdx);
			U2UVAC_IsoInTxfStateMachine(UVC_ISOIN_TXF_ACT_TXF);
			totalFrmCnt++;
			if ((m_uiU2UvacDbgVid & UVAC_DBG_VID_QUEUE)) {
				vos_perf_mark(&tt2);
				DBG_DUMP("[1]0x%X:%dKB %dms\r\n", txfInfo.sAddr, txfInfo.size/1024, vos_perf_duration(tt1, tt2)/ 1000);
				vos_perf_mark(&tt1);
			}
			if (gUvacRunningFlag[thisVidIdx] & FLGUVAC_STOP2) {
				DbgMsg_UVC(("Tsk2 Stop B ...\r\n"));
				break;
			}

			if (usb_getControllerState() == USB_CONTROLLER_STATE_SUSPEND) {
				if ((usb_getControllerState() == USB_CONTROLLER_STATE_SUSPEND)) {
					DbgMsg_UVC(("Clr StartFlag2!!\r\n"));
					gU2UvcUsbDMAAbord = TRUE;
					clr_flg(FLG_ID_U2UVAC, FLGUVAC_START2);
				}
				DbgMsg_UVC(("Tsk2 Stop E ...\r\n"));
				break;
			}
			uiQueCnt = pStrmQueInfo->idxProducer + UVAC_VID_INFO_QUE_MAX_CNT - pStrmQueInfo->idxConsumer;
			if (uiQueCnt >= UVAC_VID_INFO_QUE_MAX_CNT) {
				uiQueCnt -= UVAC_VID_INFO_QUE_MAX_CNT;
			}
			if (m_uiU2UvacDbgVid & UVAC_DBG_VID_QUE_CNT) {
				DBG_DUMP("V2 Q=%d\r\n", uiQueCnt);
			}
			#if 0
			if (uiQueCnt > 1 && ((UVAC_VIDEO_FORMAT_MJPG == gUvacCodecType[thisVidIdx]) || (UVAC_VID_I_FRM_MARK == pVidBuf[UVAC_VID_I_FRM_MARK_POS]))) {
				if (m_uiU2UvacDbgVid & UVAC_DBG_VID_START) {
					DBG_DUMP("V2[%d] race!\r\n", totalFrmCnt);
				}
				goto V2_SKIP_TIMER;
			}
			#else
			if (uiQueCnt) {
				goto V2_SKIP_TIMER;
			}
			#endif

		}// end of wait timer2 //while (FALSE == gUvcNoVidStrm)

L_UVAC_END2:
		clr_flg(FLG_ID_U2UVAC, FLGUVAC_START2);
		DbgMsg_UVC(("+2UVAC end:Fmt[0]=%d\n\r", gUvacCodecType[thisVidIdx]));
		//retV = usb_abortEndpoint(U2UVAC_USB_EP[thisTxfQueIdx]);
		DbgMsg_UVC(("-2Abort EP#%d, retV=%d\n\r", U2UVAC_USB_EP[thisTxfQueIdx], retV));
		if (retV != E_OK) {
			DBG_ERR("Abort failed\n\r");
		}
		UVC_TimeTrigClose(thisVidIdx);
		DbgMsg_UVC(("+2wait isoTsk :0x%x\n\r", uiFlag));
		//wai_flg(&uiFlag, FLG_ID_U2UVAC, FLGUVAC_ISOIN_READY, (TWF_CLR | TWF_ORW));

		if (0 == kchk_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_VDO2_READY)) {
			vos_util_delay_ms(70);
		}
		{
			UINT32 count = 0;
			while (1) {
				if (count < 1000) {
					if (0 == kchk_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_VDO2_READY)) {
						retV = usb_abortEndpoint(U2UVAC_USB_EP[thisTxfQueIdx]);
						if (retV != E_OK) {
							DBG_ERR("Abort failed\n\r");
						}
						DbgMsg_UVC(("\n\r#### abort EP[%d] ####\n\r\n\r", U2UVAC_USB_EP[thisTxfQueIdx]));
						vos_util_delay_ms(10);
					} else {
						break;
					}
				} else {
					DBG_ERR("wait ISOIN_VDO2_READY failed\r\n");
					break;
				}
				count++;
			}
		}
		DbgMsg_UVC(("-2wait isoTsk :0x%x\n\r", uiFlag));
		gUvacRunningFlag[thisVidIdx] = 0;
		clr_flg(FLG_ID_U2UVAC, FLGUVAC_STOP2);
		//reset stream queue index
		pStrmQueInfo->idxProducer = 0;
		pStrmQueInfo->idxConsumer = 0;
		if (g_fpStopVideo) {
			g_fpStopVideo(thisVidIdx);
			pStrmQueInfo->strmInfo.isStrmOn = strmInfo.isStrmOn = FALSE;
		}
		DbgMsg_UVC(("-UVAC2 end\n\r"));
	} // end for while(1)
	THREAD_RETURN(0);
}

ER U2UVAC_WaitStrmDone(UVAC_STRM_PATH path)
{
	FLGPTN uiFlag = 0;
	FLGPTN wait_flg;
	UINT32 timeout;
	int ret = 0;

	if (FLG_ID_U2UVAC_FRM == 0) {
		return E_OK;
	}

	switch (path) {
	case UVAC_STRM_VID:
		wait_flg = FLGUVAC_FRM_V1;
		if (gStrmInfo[UVAC_VID_DEV_CNT_1].strmFps != 0) {
			timeout = (10000/gStrmInfo[UVAC_VID_DEV_CNT_1].strmFps);
		} else {
			timeout = 500;
		}
		break;
	case UVAC_STRM_VID2:
		wait_flg = FLGUVAC_FRM_V2;
		if (gStrmInfo[UVAC_VID_DEV_CNT_2].strmFps != 0) {
			timeout = (10000/gStrmInfo[UVAC_VID_DEV_CNT_2].strmFps);
		} else {
			timeout = 500;
		}
		break;
	case UVAC_STRM_AUD:
		wait_flg = FLGUVAC_FRM_A1;
		wait_flg = FLGUVAC_FRM_A2;
		if (gU2UacSampleRate != 0) {
			timeout = (10240000/gU2UacSampleRate);
		} else {
			timeout = 500;
		}
		break;
	case UVAC_STRM_AUD2:
		wait_flg = FLGUVAC_FRM_A2;
		if (gU2UacSampleRate != 0) {
			timeout = (10240000/gU2UacSampleRate);
		} else {
			timeout = 500;
		}
		break;
	default:
		DBG_ERR("invalid path = %d\r\n", path);
		return E_SYS;
		break;
	}

	ret = vos_flag_wait_timeout(&uiFlag, FLG_ID_U2UVAC_FRM, wait_flg, TWF_ORW, vos_util_msec_to_tick(timeout));

	if (ret != 0) {
		DBG_WRN("wait path %d flag err = %d\r\n", path, ret);
	}

	return E_OK;
}

static BOOL IsCdcNotOpened(void)
{
	if (FALSE == gUvacOpened || FALSE == gU2UvacCdcEnabled) {
		DBG_ERR("CDC not opened!\r\n");
		return TRUE;
	} else {
		return FALSE;
	}
}

UINT32 U2UVAC_GetCdcDataCount(CDC_COM_ID ComID)
{
	if (IsCdcNotOpened()) {
		return 0;
	}
	if (CDC_COM_1ST == ComID) {
		return usb_getEPByteCount(CDC_DATA_OUT_EP);
	} else {
		return usb_getEPByteCount(CDC_DATA2_OUT_EP);
	}
}

INT32 U2UVAC_ReadCdcData(CDC_COM_ID ComID, void *p_buf, UINT32 buffer_size, INT32 timeout)
{
	UINT32  dataSize;
	FLGPTN      uiFlag = 0;
	UINT32 SemID;
	FLGPTN waiptn;
	USB_EP DataOutEP;
	INT32 ret_value = -1;

	if (IsCdcNotOpened()) {
		return ret_value;
	}

	if (CDC_COM_1ST == ComID) {
		SemID = SEMID_U2UVC_READ_CDC;
		waiptn = FLGUVAC_CDC_DATA_OUT | FLGUVAC_CDC_ABORT_READ;
		DataOutEP = CDC_DATA_OUT_EP;
	} else {
		SemID = SEMID_U2UVC_READ2_CDC;
		waiptn = FLGUVAC_CDC_DATA2_OUT | FLGUVAC_CDC_ABORT_READ2;
		DataOutEP = CDC_DATA2_OUT_EP;
	}

	wai_sem(SemID);

	vos_flag_wait_timeout(&uiFlag, FLG_ID_U2UVAC, waiptn, TWF_ORW | TWF_CLR, vos_util_msec_to_tick(timeout));
	if (uiFlag & (FLGUVAC_CDC_ABORT_READ | FLGUVAC_CDC_ABORT_READ2)) {
		sig_sem(SemID);
		return ret_value;
	}

	if (uiFlag & waiptn) {
	//usb_maskEPINT(CDC_DATA_OUT_EP); already mask in UVAC_Callback()
	dataSize = U2UVAC_GetCdcDataCount(ComID);
		if (dataSize > buffer_size) {
			dataSize = buffer_size;
	}
		usb_readEndpoint(DataOutEP, (UINT8 *) p_buf, &dataSize);
		ret_value = (INT32)dataSize;
		usb_unmaskEPINT(DataOutEP);
		}

	DBG_MSG("CDC read %d byte.\r\n", dataSize);
	sig_sem(SemID);
	return ret_value;
}

void U2UVAC_AbortCdcRead(CDC_COM_ID ComID)
{
	if (FALSE == IsCdcNotOpened()) {
		if (CDC_COM_1ST == ComID) {
			set_flg(FLG_ID_U2UVAC, FLGUVAC_CDC_ABORT_READ);
		} else {
			set_flg(FLG_ID_U2UVAC, FLGUVAC_CDC_ABORT_READ2);
		}
	}
}
#define MAX_CDC_RETRY_CNT 1000
INT32 U2UVAC_WriteCdcData(CDC_COM_ID ComID, void *p_buf, UINT32 buffer_size, INT32 timeout)
{
	UINT32 SemID;
	USB_EP DataInEP;
	UINT32 BufSize;
	VOS_TICK begin = 0, current = 0;
	UINT32 elapsed = 0, remain;
	INT32 ret_value = -1;

	if (IsCdcNotOpened()) {
		return ret_value;
	}

	if (buffer_size > USB_MAX_DMA_LENGTH) {
		DBG_ERR("Max data length is 0x%X\r\n", USB_MAX_DMA_LENGTH);
		return ret_value;
	}

	if (CDC_COM_1ST == ComID) {
		SemID = SEMID_U2UVC_WRITE_CDC;
		DataInEP = CDC_DATA_IN_EP;
	} else {
		SemID = SEMID_U2UVC_WRITE2_CDC;
		DataInEP = CDC_DATA2_IN_EP;
	}

	//In 2V2A mode, CDC used the preivous FIFOs which share only one DMA channel.
	if (gU2UvacChannel > UVAC_CHANNEL_1V1A) {
		SemID = SEMID_U2UVC_WRITE_CDC;
	}

	wai_sem(SemID);

	vos_perf_mark(&begin);
	while(usb_chkEPBusy(DataInEP)) {
		vos_perf_mark(&current);
		elapsed = vos_perf_duration(begin, current)/ 1000;

		if (timeout == 0 || (elapsed > (UINT32)timeout)) {
			sig_sem(SemID);
			return ret_value;
		}
		vos_util_delay_ms(10);
				}

	if ((UINT32)timeout > elapsed) {
		remain = (UINT32)timeout - elapsed;
			} else {
		remain = 0;
				}

	if (buffer_size) {
		BufSize = buffer_size;
		usb_writeEndpointTimeout(DataInEP, p_buf, &BufSize, remain);
		ret_value = (INT32)BufSize;
	} else {
		usb_setTX0Byte(DataInEP);
		ret_value = 0;
	}
	sig_sem(SemID);

	return ret_value;
}


////////////////////////////////////////////////////////////////////////////////
void _U2UVAC_init_queue(void)
{
	PUVAC_AUD_RAWQ pObj;

	pObj = &uac_raw_que;

	//memset((void*)pObj, 0, sizeof(UVAC_AUD_RAWQ));

	pObj->last_addr = gAudRxBuf[0].pa;
	pObj->lock_addr= gAudRxBuf[0].pa;
	pObj->Front = 0;
	pObj->Rear = 0;
}

BOOL _U2UVAC_GetRaw(UINT32 *addr, UINT32 size)
{
	UINT32 front, rear, next, next_end;
	PUVAC_AUD_RAWQ pObj;
	BOOL bResult = FALSE;

	pObj = &uac_raw_que;
	front = pObj->last_addr;
	rear = pObj->lock_addr;

	if (front != rear) {
		if ((front + size) > gAudRxBuf[0].pa + gAudRxBuf[0].size) {
			next = gAudRxBuf[0].pa;

		} else {
			next = front;
		}

		next_end = next + size;

		if (rear > next_end) {
			*addr = next;
			pObj->last_addr = next_end;
			bResult = TRUE;
		} else if (rear < next_end && (next_end - rear) > size) {
			*addr = next;
			pObj->last_addr = next_end;
			bResult = TRUE;
		} else {
			bResult = FALSE;
		}
	} else {
		//empty
		if ((front + size) > gAudRxBuf[0].pa + gAudRxBuf[0].size) {
			next = gAudRxBuf[0].pa;
		} else {
			next = front;
		}

		next_end = next + size;

		*addr = next;
		pObj->last_addr = next_end;
		bResult = TRUE; //no lock bs
	}

	if (pObj->last_addr == (gAudRxBuf[0].pa + gAudRxBuf[0].size)) {
		pObj->last_addr = gAudRxBuf[0].pa;
	}

	return bResult;
}


/*static BOOL _U2UVAC_is_pullque_empty(UINT32 pathID)
{
	PUVAC_AUD_RAWQ p_obj;
	BOOL is_empty = FALSE;
	unsigned long flags;

	p_obj = &uac_raw_que;
	is_empty = ((p_obj->Front == p_obj->Rear) && (p_obj->bQFull == FALSE));

	return is_empty;
}

static BOOL _U2UVAC_is_pullque_full(UINT32 pathID)
{
	PUVAC_AUD_RAWQ p_obj;
	BOOL is_full = FALSE;
	unsigned long flags;

	p_obj = &uac_raw_que;
	is_full = ((p_obj->Front == p_obj->Rear) && (p_obj->bQFull == TRUE));

	return is_full;
}*/

void _U2UVAC_unlock_pullqueue(UVAC_STRM_PATH path)
{
	if (vos_sem_wait_timeout(SEMID_U2UVC_UAC_QUEUE, vos_util_msec_to_tick(0))) {
		DBG_IND("[ISF_AUDCAP][%d] no data in pull queue, auto unlock pull blocking mode !!\r\n", pathID);
	}

	// [case_1]   no data in pullQ, then this 0 ->   0   -> 1 to unlock HDAL pull blocking mode (fake semaphore.... but it's OK, because _ISF_AudDec_Get_PullQueue() will return FALSE, there's no actual data in queue )
	// [case_2] have data in pullQ, then this n -> (n-1) -> n to recover the semaphore count
	vos_sem_sig(SEMID_U2UVC_UAC_QUEUE);
}

BOOL _U2UVAC_put_pullque(UINT32 addr, UINT32 size)
{
	PUVAC_AUD_RAWQ pObj;

	pObj = &uac_raw_que;

	if ((pObj->Front != 0) && (pObj->Front - 1) == pObj->Rear) {
		//DBG_ERR("[ISF_AUDCAP][%d] Pull Queue is Full!\r\n", pathID);
		return FALSE;
	} else if ((pObj->Front == 0) && ((pObj->Rear+1) == UVAC_AUD_RAWQ_MAX)) {
		//DBG_ERR("[ISF_AUDCAP][%d] Pull Queue is Full!\r\n", pathID);
		return FALSE;
	} else {
		pObj->queue[pObj->Rear].addr = addr;
		pObj->queue[pObj->Rear].size = size;

		pObj->Rear = (pObj->Rear + 1) % UVAC_AUD_RAWQ_MAX;

		vos_sem_sig(SEMID_U2UVC_UAC_QUEUE); // PULLQ + 1
		return TRUE;
	}
}

BOOL _U2UVAC_release_all_pullque(UVAC_STRM_PATH path)
{
	UVAC_STRM_FRM strm_frm;

	//consume all pull queue
	while (E_OK == U2UVAC_PullOutStrm(&strm_frm, 0));

	return TRUE;
}

ER U2UVAC_PullOutStrm(PUVAC_STRM_FRM pStrmFrm, INT32 wait_ms)
{
	PUVAC_AUD_RAWQ pObj;

	if (wait_ms < 0) {
		// blocking (wait until data available) , if success PULLQ - 1 , else wait forever (or until signal interrupt and return FALSE)
		if (vos_sem_wait_interruptible(SEMID_U2UVC_UAC_QUEUE)) {
			return E_NOEXS;
		}
	} else  {
		// non-blocking (wait_ms=0) , timeout (wait_ms > 0). If success PULLQ - 1 , else just return FALSE
		if (vos_sem_wait_timeout(SEMID_U2UVC_UAC_QUEUE, vos_util_msec_to_tick(wait_ms))) {
			DBG_IND("[ISF_AUDCAP][%d] Pull Queue Semaphore timeout!\r\n");
			return E_TMOUT;
		}
	}

	// check state
	if (gUvacOpened == FALSE) {
		return E_OBJ;
	}

	pObj = &uac_raw_que;

	if ((pObj->Front == pObj->Rear)) {
		//DBG_ERR("[ISF_AUDCAP][%d] Pull Queue is Empty !!!\r\n", pathID);   // This should normally never happen !! If so, BUG exist. ( Because we already check & wait data available at upper code. )
		return E_NOEXS;
	} else {
		pStrmFrm->path = UVAC_STRM_AUDRX;
		pStrmFrm->addr = pObj->queue[pObj->Front].addr;
		pStrmFrm->size = pObj->queue[pObj->Front].size;

		pObj->Front= (pObj->Front + 1) % UVAC_AUD_RAWQ_MAX;

		return E_OK;
	}
}

ER U2UVAC_ReleaseOutStrm(PUVAC_STRM_FRM pStrmFrm)
{
	PUVAC_AUD_RAWQ pObj;

	pObj = &uac_raw_que;

	pObj->lock_addr = pStrmFrm->addr + pStrmFrm->size;

	if (pObj->lock_addr == (gAudRxBuf[0].pa + gAudRxBuf[0].size)) {
		pObj->lock_addr = gAudRxBuf[0].pa;
	}

	return E_OK;
}

THREAD_DECLARE(U2UVAC_AudioRxTsk, arglist)
{
	FLGPTN uiFlag = 0;
	ER     ret;
	UINT32 recv_size;
	UINT32 recv_addr;
	UINT64 t1, t2;
	UINT64 block_time = 0;
	BOOL add_queue;

	//coverity[no_escape]
	while (1) {

		wai_flg(&uiFlag, FLG_ID_U2UVAC_UAC_RX, FLGUVAC_AUD_DATA_OUT | FLGUVAC_AUD_EXIT, TWF_ORW | TWF_CLR);

		if (block_time == 0) {
			block_time = (gU2UvacUacBlockSize*1000000) / (gU2UvacAudSampleRate[0] * gU2UacChNum * UAC_BIT_PER_SECOND / 8);
			DBG_IND("block_time = %llu\r\n", block_time);
		}

		if (uiFlag & FLGUVAC_AUD_EXIT) {
			break;
		}

		if (uiFlag & FLGUVAC_AUD_DATA_OUT) {

			if (!gU2UacAudStart[UVAC_AUD_DEV_CNT_3] || !gU2UvcVidStart[0]) {
				usb2dev_unmask_ep_interrupt(U2UVAC_USB_RX_EP[0]);
				continue;
			}

			if ( usb2dev_check_ep_busy(U2UVAC_USB_RX_EP[0])) {
				DBG_ERR("EP busy\r\n");
				usb2dev_unmask_ep_interrupt(U2UVAC_USB_RX_EP[0]);
				continue;
			}

			recv_size = gU2UvacUacBlockSize;

			if (!_U2UVAC_GetRaw(&recv_addr, recv_size)) {
				DBG_ERR("No buffer\r\n");
				usb2dev_unmask_ep_interrupt(U2UVAC_USB_RX_EP[0]);
				continue;
			}

			//vos_cpu_dcache_sync((VOS_ADDR)(gAudRxBuf[0].va + read_size), recv_size, VOS_DMA_BIDIRECTIONAL);
			t1 = hd_gettime_us();

			ret = usb2dev_read_endpoint(U2UVAC_USB_RX_EP[0], (UINT8 *) recv_addr, &recv_size);

			vos_cpu_dcache_sync((VOS_ADDR)uvac_pa_to_va(UVAC_BUF_TYPE_WORK, recv_addr), recv_size, VOS_DMA_FROM_DEVICE);
			t2 = hd_gettime_us();

			if ((t2 - t1) > block_time*2) {
				DBG_IND("%llu\r\n", t2 - t1);
				add_queue = FALSE;
			} else {
				add_queue = TRUE;
			}

			if (recv_size != gU2UvacUacBlockSize) {
				DBG_IND("recv_size = %d\r\n", recv_size);
			} else if (add_queue) {
				if(!_U2UVAC_put_pullque(recv_addr, recv_size)) {
					DBG_ERR("UAC RX put queue failed\r\n");
				}
			}

			if (ret != E_OK) {
				DBG_ERR("read error\r\n");
			}

			usb2dev_unmask_ep_interrupt(U2UVAC_USB_RX_EP[0]);
		}

	}

	set_flg(FLG_ID_U2UVAC_UAC_RX, FLGUVAC_AUD_IDLE);

	THREAD_RETURN(0);
}
////////////////////////////////////////////////////////////////////////////////

//// for internal test only ///////
void _UVAC_stream_enable(UVAC_STRM_PATH stream, BOOL enable)
{
	UINT32 event = 0;

	switch (stream) {
	case UVAC_STRM_VID:
		event = (gU2UvacIntfIdx_VS[0] << 16) | enable;
		break;
	case UVAC_STRM_AUD:
		event = (gU2UvacIntfIdx_AS[0] << 16) | enable;
		break;
	case UVAC_STRM_VID2:
		event = (gU2UvacIntfIdx_VS[1] << 16) | enable;
		break;
	case UVAC_STRM_AUD2:
		event = (gU2UvacIntfIdx_AS[1] << 16) | enable;
		break;
	default:
		break;
	}
	UVAC_InterfaceCB(event);
}

INT32 UVAC_WriteHidData(void *p_buf, UINT32 buffer_size, INT32 timeout)
{
	UINT32 SemID;
	USB_EP DataInEP;
	UINT32 BufSize;
	INT32 ret_value = -1;
	VOS_TICK begin = 0, current = 0;
	UINT32 elapsed = 0, remain;

	if (FALSE == g_hid_info.en) {
		return ret_value;
	}

	if (buffer_size > USB_MAX_DMA_LENGTH) {
		DBG_ERR("Max data length is 0x%X\r\n", USB_MAX_DMA_LENGTH);
		return ret_value;
	}

	SemID = SEMID_U2UVC_WRITE_HID;
	DataInEP = HID_INTRIN_EP;

	wai_sem(SemID);

	vos_perf_mark(&begin);
	while(usb_chkEPBusy(DataInEP)) {
		vos_perf_mark(&current);
		elapsed = vos_perf_duration(begin, current)/ 1000;

		if (timeout == 0 || (elapsed > (UINT32)timeout)) {
			sig_sem(SemID);
			return ret_value;
		}
		vos_util_delay_ms(10);
	}
	if ((UINT32)timeout > elapsed) {
		remain = (UINT32)timeout - elapsed;
	} else {
		remain = 0;
	}
	if (buffer_size) {
		BufSize = buffer_size;
		usb_writeEndpointTimeout(DataInEP, p_buf, &BufSize, remain);
		ret_value = (INT32)BufSize;
	}
	sig_sem(SemID);
	return ret_value;
}
INT32 UVAC_ReadHidData(void *p_buf, UINT32 buffer_size, INT32 timeout)
{
	UINT32  dataSize;
	UINT32 SemID;
	USB_EP DataOutEP;
	INT32 ret_value = -1;
	FLGPTN      uiFlag = 0;

	if (FALSE == g_hid_info.en) {
		return ret_value;
	}

	SemID = SEMID_U2UVC_READ_HID;
	DataOutEP = HID_INTROUT_EP;

	wai_sem(SemID);
	vos_flag_wait_timeout(&uiFlag, FLG_ID_U2UVAC, FLGUVAC_HID_DATA_OUT, TWF_CLR | TWF_ORW, vos_util_msec_to_tick(timeout));
	if (uiFlag & FLGUVAC_HID_DATA_OUT) {
		dataSize = usb_getEPByteCount(DataOutEP);
		if (dataSize > buffer_size) {
			dataSize = buffer_size;
		}
		//usb_readEndpointTimeout(DataOutEP, (UINT8 *) p_buf, &dataSize, remain);
		usb_readEndpoint(DataOutEP, (UINT8 *) p_buf, &dataSize);
		ret_value = (INT32)dataSize;
		usb_unmaskEPINT(DataOutEP);
	}
	sig_sem(SemID);

	return ret_value;
}
//@}

