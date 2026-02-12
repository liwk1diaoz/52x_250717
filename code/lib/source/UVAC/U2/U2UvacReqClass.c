#include <string.h>
#include "U2UvacVideoTsk.h"
#include "U2UvacDesc.h"
#include "UVAC.h"
#include "U2UvacDbg.h"
#include "U2UvacDesc.h"
#include "U2UvacReqClass.h"
#include "U2UvacIsoInTsk.h"
#include "U2UvacID.h"
#include "hd_util.h"
//#include "U2UvacSIDCCmd.h"
//#include "DxSound.h"


extern UINT32 gU2UvcUsbDMAAbord;
extern const USB_EP U2UVAC_USB_EP[UVAC_TXF_QUE_MAX];
extern UINT32 gU2UvcCapImgAddr;
extern UINT32 gU2UvcCapImgSize;
extern UINT8 gU2UvacIntfIdx_WinUsb;
extern BOOL gU2UvacWinIntrfEnable;
extern UINT8 gU2UvacIntfIdx_VC[UVAC_VID_DEV_CNT_MAX];
extern UINT8 gU2UvacIntfIdx_VS[UVAC_VID_DEV_CNT_MAX];
extern UINT8 gU2UvacIntfIdx_AC[UVAC_AUD_DEV_CNT_MAX];
extern UINT8 gU2UvacIntfIdx_AS[UVAC_AUD_DEV_CNT_MAX];
extern UINT32 gU2UvacAudSampleRate[UVAC_AUD_SAMPLE_RATE_MAX_CNT];
extern UVAC_VID_RESO gU2UvcVidResoAry[UVAC_VID_RESO_MAX_CNT];
extern UINT8 gU2UvacIntfIdx_CDC_COMM[];
extern UINT8 gU2UvacIntfIdx_CDC_DATA[];
extern UVAC_CDC_PSTN_REQUEST_CB gfpU2CdcPstnReqCB;
extern BOOL gU2UvacCdcEnabled;
extern UINT8 gU2UvacIntfIdx_SIDC;
extern BOOL gU2UvacMtpEnabled;
extern UINT32 gU2UvcMJPGMaxTBR;
extern UINT8 gUvacIntfIdx_DFU;
extern UVAC_VID_RESO_ARY gUvcYuvFrmInfo;
extern UVAC_VID_RESO_ARY gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_MAX];
extern UVAC_VID_RESO_ARY gUvcH264FrmInfo[UVAC_VID_DEV_CNT_MAX];
extern UINT32 gU2UvcVidResoCnt;
extern UVAC_VID_RESO gU2UvcVidResoAry[UVAC_VID_RESO_MAX_CNT];
extern UINT8 h264_fmt_index[UVAC_VID_DEV_CNT_MAX];
extern UINT8 mjpeg_fmt_index[UVAC_VID_DEV_CNT_MAX];
extern UINT8 yuv_fmt_index;
extern const USB_EP U2UVAC_USB_RX_EP[UVAC_RXF_QUE_MAX];
extern BOOL gU2UvacUacRxEnabled;
extern UINT32 gU2UvacChannel;

void U2UVC_DmpStillProbeCommitData(PUVC_STILL_PROBE_COMMIT pStilProbComm);
static void UVC_VS_Still_ProbeCtrl(UINT8 req, UINT8 *pData, UINT8 intrf);
static void UVC_VS_Still_CommitCtrl(UINT8 req, UINT8 *pData, UINT8 intrf);
static void UVC_VS_Still_ImgTrigCtrl(UINT8 req, UINT8 *pData, UINT8 intrf);
static void UVC_CapStilImgUpload(UINT8 *pData);


UVC_STILL_PROBE_COMMIT gU2UvcStillProbeCommit = {0};
UINT8 gU2UvacStillImgTrigSts = UVC_STILLIMG_TRIG_CTRL_NORMAL;
static UINT16 gUACRes = 0x0002;
static UVC_PROBE_COMMIT g_ProbeCommit[UVAC_VID_DEV_CNT_MAX] = {0};
static _ALIGNED(4) UINT8 gUvacClsReqDataBuf[USB_UVAC_CLS_DATABUF_LEN];
UINT32 gU2UacSampleRate = UAC_FREQUENCY_16K;
static UINT32 gUvacVendDataLen = USB_UVAC_CLS_DATABUF_LEN;
UVAC_EUVENDCMDCB gU2UvacEUVendCmdCB[UVAC_EU_VENDCMD_CNT] = {0};
UVAC_WINUSBCLSREQCB gU2UvcWinUSBReqCB = 0;
static UINT32 gUvcWinUsbDataLen = 0;
static BOOL gUacMute = FALSE;
static INT32 gUacVol = UAC_VOL_MAX;
UVAC_SETVOLCB gU2UacSetVolCB = 0;
UVAC_UNIT_CB g_fpUvcCT_CB = NULL;
UVAC_UNIT_CB g_fpUvcPU_CB = NULL;
UVAC_UNIT_CB g_fpUvcXU_CB = NULL;
extern UINT8 eu_unit_id[2];
extern UVAC_DFU_INFO g_dfu_info;
static MEM_RANGE dfu_remain_buf = {0};
extern UVAC_HID_INFO g_hid_info;
extern UINT8 gUvacIntfIdx_HID;
extern FP g_u2uvac_msdc_class_req_cb;
extern UVAC_MSDC_INFO g_msdc_info;
extern UVAC_EU_DESC eu_desc[2];
extern UINT32 gUvacUvcVer;

#define WAIT_OUT0_TIMEOUT 1000//ms
static UINT8 error_code = ERROR_CODE_INVALID_CONTROL;

static void _ret_usb_stall(void)
{
	usb_setEP0Stall();
	//usb_setEP0Done();
}

static ER _readEP0(UINT8 *p_data, UINT32 *pDMALen)
{
#ifdef __LINUX_USER__
	UINT32 begin = hd_gettime_ms();
	BOOL timeout = TRUE;

	if (0 == *pDMALen) {
		return E_OK;
	}
	do {
		if (usb2dev_get_config(USB_CONFIG_ID_CHECK_CXOUT)) {
			timeout = FALSE;
			break;
		}
	} while ((hd_gettime_ms() - begin) < WAIT_OUT0_TIMEOUT  );

	if (timeout) {
		DBG_WRN("wait OUT0 timeout!\r\n");
		return E_SYS;
	}
	return usb_readEndpoint(USB_EP0, p_data, pDMALen);
#else
	if (0 == *pDMALen) {
		return E_OK;
	} else {
		FLGPTN uiFlag;
		usb_unmaskEPINT(USB_EP0);
		wai_flg(&uiFlag, FLG_ID_U2UVAC, FLGUVAC_OUT0, TWF_CLR);
		usb_maskEPINT(USB_EP0);
		return usb_readEndpoint(USB_EP0, p_data, pDMALen);
	}
#endif
}

#define REQUEST_LEN CONTROL_DATA.device_request.w_length

static void ret_setup_data(UINT8 *p_data, UINT32 len)
{
	CONTROL_DATA.w_length  = (UINT16)len;
	CONTROL_DATA.p_data    = p_data;
	usb_RetSetupData();
}

static void UVC_Unit_Cmd(UVAC_UNIT_CB fp_UnitCB, UINT32 ControlCode, UINT8 request, UINT8 *p_data)
{
	DbgMsg_UVCIO(("ctrl=%d, request=0x%x, pData=0x%x\r\n", ControlCode, request, p_data));
	//DBG_DUMP("VendCmd: ctrl=%d, cs=0x%x, pData=0x%x\r\n", ControlCode, CS, pData);

	//ControlCode: 1, 2, ..., 8
	if (fp_UnitCB) {
		if (SET_CUR == request) {
			if (usb2dev_control_data.device_request.w_length > USB_UVAC_CLS_DATABUF_LEN) {
				gUvacVendDataLen = USB_UVAC_CLS_DATABUF_LEN;
			} else {
				gUvacVendDataLen = usb2dev_control_data.device_request.w_length;
			}
			memset(p_data, 0, USB_UVAC_CLS_DATABUF_LEN);
			if (_readEP0(p_data, &gUvacVendDataLen) != E_OK) {
				DBG_ERR("Read failed!\r\n");
				_ret_usb_stall();
				return;
			}
		}
		if (fp_UnitCB(ControlCode, request, p_data, &gUvacVendDataLen)) {
			DbgMsg_UVCIO(("VendCmd Ok, retLen=%d,ctrl=%d, request=0x%x, p_data=0x%x\r\n", gUvacVendDataLen, ControlCode, request, p_data));
			DbgMsg_UVCIO(("  ==> *p_data=0x%x\r\n", *p_data));
			//DBG_DUMP("VendCmd Ok, retLen=%d,ctrl=%d, cs=0x%x, p_data=0x%x", gUvacVendDataLen, ControlCode, CS, p_data);
			//DBG_DUMP("==> *p_data=0x%x\r\n", *p_data);
			if (gUvacVendDataLen) {
				if (gUvacVendDataLen > USB_UVAC_CLS_DATABUF_LEN) {
					UINT8 *p_addr = (UINT8 *)*(UINT32 *)p_data;
					ret_setup_data(p_addr, gUvacVendDataLen);
				} else {
				ret_setup_data(p_data, gUvacVendDataLen);
			}
		} else {
				usb_setEP0Done();
			}
		} else {
			DBG_IND("VendCmd Fail, retLen=%d,ctrl=%d, cs=0x%x, pData=0x%x\r\n", gUvacVendDataLen, ControlCode, CS, pData);
			if (gUvacVendDataLen && *p_data == ERROR_CODE_INVALID_REQUEST) {
				error_code = ERROR_CODE_INVALID_REQUEST;
			} else if (gUvacVendDataLen && *p_data == ERROR_CODE_WRONG_STATE) {
				error_code = ERROR_CODE_WRONG_STATE;
			} else if (gUvacVendDataLen && *p_data == ERROR_CODE_OUT_OF_RANGE) {
				error_code = ERROR_CODE_OUT_OF_RANGE;
			} else {
				error_code = ERROR_CODE_INVALID_CONTROL;
			}
			_ret_usb_stall();
		}
	} else {
		_ret_usb_stall();
	}

}

void U2UVC_WinUSB_ClassReq(UINT32 ControlCode, UINT8 CS, UINT8 *pData)
{
	UINT8 vendCBRet;
	DbgMsg_UVC(("+WinUSB_ClassReq: ctrl=%d, cs=0x%x, pData=0x%x, cb=0x%x\r\n", ControlCode, CS, pData, gU2UvcWinUSBReqCB));

	if (gU2UvcWinUSBReqCB) {
		vendCBRet = gU2UvcWinUSBReqCB(ControlCode, CS, pData, &gUvcWinUsbDataLen);
		if (TRUE == vendCBRet) {
			DbgMsg_UVC(("WinUSB_ClassReq Ok, retLen=%d,ctrl=%d, cs=0x%x, pData=0x%x\r\n", gUvcWinUsbDataLen, ControlCode, CS, pData));
			DbgMsg_UVC(("  ==> *pData=0x%x\r\n", *pData));
			gUvcWinUsbDataLen = (gUvcWinUsbDataLen > USB_UVAC_CLS_DATABUF_LEN) ? USB_UVAC_CLS_DATABUF_LEN : gUvcWinUsbDataLen;
			ret_setup_data(pData, gUvcWinUsbDataLen);
		} else {
			DBG_ERR("WinUSB_ClassReq Fail, retLen=%d,ctrl=%d, cs=0x%x, pData=0x%x\r\n", gUvcWinUsbDataLen, ControlCode, CS, (unsigned int)pData);
			_ret_usb_stall();
		}
	}
	DbgMsg_UVC(("-WinUSB_ClassReq: ctrl=%d, cs=0x%x, pData=0x%x\r\n", ControlCode, CS, pData));
}

/**
    UVC Select Unit control

    @param UINT8 CS       Control attribute
    @param UINT8 *pData   Data pointer
    @return void
*/
static void UVC_SU_SelectCtrl(UINT8 CS, UINT8 *pData)
{
	UINT8 *pDataAddr;
	UINT8 datasize;

	pDataAddr = pData;
	DbgMsg_UVCIO(("%s:%d,len=%d\r\n", __func__, CS, REQUEST_LEN));
	switch (CS) {
	case GET_INFO:
		*pData = 0x1;
		datasize = 1;
		ret_setup_data(pDataAddr, datasize);
		break;
	case SET_CUR:
	case GET_CUR:
	case GET_MIN:
	case GET_MAX:
	case GET_RES:
	case GET_LEN:
	case GET_DEF:
		//stall EP0
		_ret_usb_stall();
		DBG_ERR("UVC:[SU Select control] Not imp\r\n");
		break;
	}
}

/**
    UVC VideoControl interface -- Request error code control

    @param UINT8 CS       Control attribute
    @param UINT8 *pData   Data pointer
    @return void
*/
static void UVC_VC_ErrorCtrl(UINT8 CS, UINT8 *pData)
{
	UINT8 *pDataAddr;
	UINT8 datasize;

	pDataAddr = pData;
	DbgMsg_UVCIO(("%s:%d\r\n", __func__, CS));

	switch (CS) {
	case GET_CUR:
		DbgMsg_UVCIO(("UVC:[VC error control] --GET_CUR\r\n"));
		*pData = 0x06;
		datasize = 1;
		ret_setup_data(pDataAddr, datasize);
		break;
	case SET_CUR:
	case GET_INFO:
	case GET_MIN:
	case GET_MAX:
	case GET_RES:
	case GET_LEN:
	case GET_DEF:
		//stall EP0
		_ret_usb_stall();
		DbgMsg_UVCIO(("UVC:[VC error control] Not imp\r\n"));
		break;
	}

}
static void UVC_DmpProbeData(UINT32 srcId, UVC_PROBE_COMMIT *pProbeCommit)
{
#if (_UVC_DBG_LVL_ > _UVC_DBG_CHK_)
	DBG_DUMP("+%s:%d, pProbeCommit=0x%x\r\n", __func__, srcId, pProbeCommit);
	DBG_DUMP("bmHint=0x%x\r\n",                            pProbeCommit->bmHint);
	DBG_DUMP("bFormatIndex=0x%x\r\n",                      pProbeCommit->bFormatIndex);
	DBG_DUMP("bFrameIndex=0x%x\r\n",                       pProbeCommit->bFrameIndex);
	DBG_DUMP("dwFrameInterval=0x%x\r\n",                   pProbeCommit->dwFrameInterval);
	DBG_DUMP("wKeyFrameRate=0x%x\r\n",                     pProbeCommit->wKeyFrameRate);
	DBG_DUMP("wPFrameRate=0x%x\r\n",                       pProbeCommit->wPFrameRate);
	DBG_DUMP("wCompQuality=0x%x\r\n",                      pProbeCommit->wCompQuality);
	DBG_DUMP("wCompWindowSize=0x%x\r\n",                   pProbeCommit->wCompWindowSize);
	DBG_DUMP("wDelay=0x%x\r\n",                            pProbeCommit->wDelay);
	DBG_DUMP("dwMaxVideoFrameSize=0x%x\r\n",               pProbeCommit->dwMaxVideoFrameSize);
	DBG_DUMP("dwMaxPayloadTransferSize=0x%x\r\n",          pProbeCommit->dwMaxPayloadTransferSize);
	if (gUvacUvcVer >= UVAC_UVC_VER_110) {
		DBG_DUMP("dwClockFrequency=0x%x\r\n",              pProbeCommit->dwClockFrequency);
		DBG_DUMP("bmFramingInfo=0x%x\r\n",                 pProbeCommit->bmFramingInfo);
		DBG_DUMP("bPreferredVersion=0x%x\r\n",             pProbeCommit->bPreferredVersion);
		DBG_DUMP("bMinVersion=0x%x\r\n",                   pProbeCommit->bMinVersion);
		DBG_DUMP("bMaxVersion=0x%x\r\n",                   pProbeCommit->bMaxVersion);
	}
	DBG_DUMP("-%s\r\n", __func__);
#endif
}
static void UVC_GetCurProbeCommitData(UINT8 *pData, UVC_PROBE_COMMIT *pProbeCommit)
{
	pData += 2;
	*pData++ = pProbeCommit->bFormatIndex;
	*pData++ = pProbeCommit->bFrameIndex;

	*pData++ = pProbeCommit->dwFrameInterval & 0xFF;
	*pData++ = (pProbeCommit->dwFrameInterval >> 8) & 0xFF;
	*pData++ = (pProbeCommit->dwFrameInterval >> 16) & 0xFF;
	*pData++ = (pProbeCommit->dwFrameInterval >> 24) & 0xFF;

	pData += 10;

	*pData++ = pProbeCommit->dwMaxVideoFrameSize & 0xFF;
	*pData++ = (pProbeCommit->dwMaxVideoFrameSize >> 8) & 0xFF;
	*pData++ = (pProbeCommit->dwMaxVideoFrameSize >> 16) & 0xFF;
	*pData++ = (pProbeCommit->dwMaxVideoFrameSize >> 24) & 0xFF;

	*pData++ = (pProbeCommit->dwMaxPayloadTransferSize) & 0xFF;
	*pData++ = (pProbeCommit->dwMaxPayloadTransferSize >> 8) & 0xFF;
	*pData++ = (pProbeCommit->dwMaxPayloadTransferSize >> 16) & 0xFF;
	*pData++ = (pProbeCommit->dwMaxPayloadTransferSize >> 24) & 0xFF;

	if (gUvacUvcVer >= UVAC_UVC_VER_110) {
		*pData++= (pProbeCommit->dwClockFrequency) & 0xFF;
		*pData++= (pProbeCommit->dwClockFrequency>>8) & 0xFF;
		*pData++= (pProbeCommit->dwClockFrequency>>16) & 0xFF;
		*pData++= (pProbeCommit->dwClockFrequency>>24) & 0xFF;
		*pData++= pProbeCommit->bmFramingInfo;
		*pData++= pProbeCommit->bPreferredVersion;
		*pData++= pProbeCommit->bMinVersion;
		*pData++= pProbeCommit->bMaxVersion;
	}
}
void U2UVC_InitProbeCommitData(void)
{
	UINT32 i = 0, dev_cnt;
	UVC_PROBE_COMMIT *pProbeCommit = 0;
	memset((void *)g_ProbeCommit, 0, (sizeof(UVC_PROBE_COMMIT)* UVAC_VID_DEV_CNT_MAX));

	if (gU2UvacChannel > UVAC_CHANNEL_1V1A) {
		dev_cnt = 2;
	} else {
		dev_cnt = 1;
	}
	for (i = 0; i < dev_cnt; i++) {
		pProbeCommit = &g_ProbeCommit[i];
		pProbeCommit->bmHint = 0;
		pProbeCommit->bFormatIndex = UVC_VSFMT_FIRSTINDEX;
		pProbeCommit->bFrameIndex = UVC_VSFMT_DEF_FRM_IDX;
		pProbeCommit->dwFrameInterval = UVC_VSFRM_FRAMEINTERVAL_MIN;
		pProbeCommit->wKeyFrameRate = UVC_FRMRATE_30;//0
		pProbeCommit->wPFrameRate = 0;
		pProbeCommit->wCompQuality = 0;//0x3D;
		pProbeCommit->wCompWindowSize = 0;
		pProbeCommit->wDelay = 666;
		pProbeCommit->dwMaxPayloadTransferSize = gU2UvcIsoInHsPacketSize * gU2UvcIsoInBandWidth;
		if (pProbeCommit->bFormatIndex == h264_fmt_index[i]) {
			pProbeCommit->dwMaxVideoFrameSize = gU2UvcMJPGMaxTBR; //no use for h264
		} else if (pProbeCommit->bFormatIndex == mjpeg_fmt_index[i]) {
			pProbeCommit->dwMaxVideoFrameSize = gU2UvcVidResoAry[UVC_VSFMT_DEF_FRM_IDX-1].width*gU2UvcVidResoAry[UVC_VSFMT_DEF_FRM_IDX-1].height*2;
		} else if (pProbeCommit->bFormatIndex == yuv_fmt_index) {
			pProbeCommit->dwMaxVideoFrameSize = gUvcYuvFrmInfo.pVidResAry[UVC_VSFMT_DEF_FRM_IDX-1].width*gUvcYuvFrmInfo.pVidResAry[UVC_VSFMT_DEF_FRM_IDX-1].height*2;
		} else {
			DBG_ERR("[%d]unknow format index=%d, h264=%d, mjpeg=%d, yuv=%d\r\n", i, pProbeCommit->bFormatIndex, h264_fmt_index[i], mjpeg_fmt_index[i], yuv_fmt_index);
		}

		if (gUvacUvcVer >= UVAC_UVC_VER_110) {
			pProbeCommit->dwClockFrequency = UVC_VC_CLOCK_FREQUENCY;
			pProbeCommit->bmFramingInfo = 0;
			pProbeCommit->bPreferredVersion = 1;
			pProbeCommit->bMinVersion = 1;
			if (yuv_fmt_index != 0) {
				pProbeCommit->bMaxVersion = yuv_fmt_index;
			} else if (mjpeg_fmt_index != 0) {
				pProbeCommit->bMaxVersion = mjpeg_fmt_index[i];
			} else {
				pProbeCommit->bMaxVersion = h264_fmt_index[i];
			}
		}
	}
}
static void UVC_VS_ProbeCtrl(UINT8 CS, UINT8 *pData, UVC_PROBE_COMMIT *pProbeCommit, UINT32 vidDevIdx)
{
	UINT8 *pDataAddr;
	UINT8 datasize;
	UINT32 ProbeData;
	UINT32 idx;
	UINT32 probe_len = (gUvacUvcVer == UVAC_UVC_VER_100)? USB_UVC_PROBE_LENGTH : USB_UVC_110_PROBE_LENGTH;

	pDataAddr = pData;
	DbgMsg_UVC(("^B+%s:0x%x, pProbeCommit=0x%x\r\n", __func__, CS, pProbeCommit));

	switch (CS) {
	case GET_CUR:
		DbgMsg_UVCIO(("^BUVC:[VS Probe control --GET_CUR]:%d\r\n", REQUEST_LEN));
		UVC_DmpProbeData(1, pProbeCommit);
		memset(pData, 0, sizeof(UVC_PROBE_COMMIT));
		UVC_GetCurProbeCommitData(pData, pProbeCommit);
		datasize = probe_len;
		ret_setup_data(pDataAddr, datasize);
		break;

	case SET_CUR:
		DbgMsg_UVCIO(("^B+UVC:[VS Probe control --SET_CUR]\r\n"));
		memset(pProbeCommit, 0, sizeof(UVC_PROBE_COMMIT));

		ProbeData = probe_len;
		memset(pData, 0, probe_len);
		if (_readEP0(pData, &ProbeData) != E_OK) {
			DBG_ERR("Read failed!\r\n");
			_ret_usb_stall();
			return;
		}
		usb_setEP0Done();

		pProbeCommit->bFormatIndex = *(pData + 2);
		pProbeCommit->bFrameIndex = *(pData + 3);

		pProbeCommit->dwFrameInterval = *(pData + 4);
		pProbeCommit->dwFrameInterval |= (*(pData + 5) << 8);
		pProbeCommit->dwFrameInterval |= (*(pData + 6) << 16);
		pProbeCommit->dwFrameInterval |= (*(pData + 7) << 24);

		if (pProbeCommit->bFormatIndex == h264_fmt_index[vidDevIdx]) {
			if (pProbeCommit->bFrameIndex > gUvcH264FrmInfo[vidDevIdx].aryCnt) {
				DBG_ERR("frame index(%d) > resolution cnt(%d)\r\n", pProbeCommit->bFrameIndex, gUvcH264FrmInfo[vidDevIdx].aryCnt);
				pProbeCommit->bFrameIndex = UVC_VSFMT_DEF_FRM_IDX;//set to default
			}
			pProbeCommit->dwMaxVideoFrameSize = gU2UvcMJPGMaxTBR; //no use for h264
		} else if (pProbeCommit->bFormatIndex == mjpeg_fmt_index[vidDevIdx]) {
			if (pProbeCommit->bFrameIndex > gUvcMjpgFrmInfo[vidDevIdx].aryCnt) {
				DBG_ERR("frame index(%d) > resolution cnt(%d)\r\n", pProbeCommit->bFrameIndex, gUvcMjpgFrmInfo[vidDevIdx].aryCnt);
				pProbeCommit->bFrameIndex = UVC_VSFMT_DEF_FRM_IDX;//set to default
			}
			idx = pProbeCommit->bFrameIndex - 1;
			pProbeCommit->dwMaxVideoFrameSize = gUvcMjpgFrmInfo[vidDevIdx].pVidResAry[idx].width*gUvcMjpgFrmInfo[vidDevIdx].pVidResAry[idx].height*2;
		} else if (pProbeCommit->bFormatIndex == yuv_fmt_index) {
			if (pProbeCommit->bFrameIndex > gUvcYuvFrmInfo.aryCnt) {
				DBG_ERR("frame index(%d) > resolution cnt(%d)\r\n", pProbeCommit->bFrameIndex, gUvcYuvFrmInfo.aryCnt);
				pProbeCommit->bFrameIndex = UVC_VSFMT_DEF_FRM_IDX;//set to default
			}
			idx = pProbeCommit->bFrameIndex - 1;
			pProbeCommit->dwMaxVideoFrameSize = gUvcYuvFrmInfo.pVidResAry[idx].width*gUvcYuvFrmInfo.pVidResAry[idx].height*2;
		} else {
			DBG_ERR("unknow format index=%d, h264=%d, mjpeg=%d, yuv=%d\r\n", pProbeCommit->bFormatIndex, h264_fmt_index[vidDevIdx], mjpeg_fmt_index[vidDevIdx], yuv_fmt_index);
		}
		pProbeCommit->dwMaxPayloadTransferSize = gU2UvcIsoInHsPacketSize * gU2UvcIsoInBandWidth;
		
		if (gUvacUvcVer >= UVAC_UVC_VER_110) {
			pProbeCommit->dwClockFrequency = UVC_VC_CLOCK_FREQUENCY;
			pProbeCommit->bmFramingInfo = 0;
			pProbeCommit->bPreferredVersion = 1;
			pProbeCommit->bMinVersion = 1;
			pProbeCommit->bMaxVersion = 1;
		}
		UVC_DmpProbeData(2, pProbeCommit);

		DbgMsg_UVCIO(("UVC: Format index =%d Frame index =%d  Interval=0x%x \r\n",
					  pProbeCommit->bFormatIndex, pProbeCommit->bFrameIndex, pProbeCommit->dwFrameInterval));
		break;

	case GET_INFO:
		DbgMsg_UVCIO(("^BUVC:[VS Probe control --GET_INFO]\r\n"));
		*pData++ = 0x03;
		ret_setup_data(pDataAddr, 1);
		break;

	case GET_MIN:
		DbgMsg_UVCIO(("^BUVC:[VS Probe control --GET_MIN]\r\n"));
		memset(pData, 0, sizeof(UVC_PROBE_COMMIT));

		pData += 2;
		*pData++ = 1;
		*pData++ = 1;

		*pData++ = UVC_VSFRM_FRAMEINTERVAL_MIN & 0xFF;
		*pData++ = (UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xFF;
		*pData++ = (UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xFF;
		*pData++ = (UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xFF;

		pData += 10;
		*pData++ = pProbeCommit->dwMaxVideoFrameSize & 0xFF;
		*pData++ = (pProbeCommit->dwMaxVideoFrameSize >> 8) & 0xFF;
		*pData++ = (pProbeCommit->dwMaxVideoFrameSize >> 16) & 0xFF;
		*pData++ = (pProbeCommit->dwMaxVideoFrameSize >> 24) & 0xFF;

		*pData++ = (pProbeCommit->dwMaxPayloadTransferSize) & 0xFF;
		*pData++ = (pProbeCommit->dwMaxPayloadTransferSize >> 8) & 0xFF;
		*pData++ = (pProbeCommit->dwMaxPayloadTransferSize >> 16) & 0xFF;
		*pData++ = (pProbeCommit->dwMaxPayloadTransferSize >> 24) & 0xFF;

		if (gUvacUvcVer >= UVAC_UVC_VER_110) {
			*pData++= (pProbeCommit->dwClockFrequency) & 0xFF;
			*pData++= (pProbeCommit->dwClockFrequency>>8) & 0xFF;
			*pData++= (pProbeCommit->dwClockFrequency>>16) & 0xFF;
			*pData++= (pProbeCommit->dwClockFrequency>>24) & 0xFF;
			*pData++= pProbeCommit->bmFramingInfo;
			*pData++= pProbeCommit->bPreferredVersion;
			*pData++= pProbeCommit->bMinVersion;
			*pData++= pProbeCommit->bMaxVersion;
		}

		datasize = probe_len;
		DbgMsg_UVCIO(("^B-UVC:[VS Probe control --GET_MIN]:%d\r\n", REQUEST_LEN));
		ret_setup_data(pDataAddr, datasize);
		DbgMsg_UVCIO(("^B---UVC:[VS Probe control --GET_MIN]:%d\r\n", CONTROL_DATA.w_length));
		break;

	case GET_MAX:
		DbgMsg_UVCIO(("^B+UVC:[VS Probe control --GET_MAX]\r\n"));
		memset(pData, 0, sizeof(UVC_PROBE_COMMIT));
		UVC_GetCurProbeCommitData(pData, pProbeCommit);
		datasize = probe_len;

		DbgMsg_UVCIO(("^B-UVC:[VS Probe control --GET_MAX]:%d\r\n", REQUEST_LEN));
		ret_setup_data(pDataAddr, datasize);
		DbgMsg_UVCIO(("^B---UVC:[VS Probe control --GET_MAX]\r\n"));
		break;

	case GET_RES:
		_ret_usb_stall();
		DBG_ERR("^RUVC:[VS Probe control --GET_RES] Not imp\r\n");
		break;

	case GET_LEN:
		DbgMsg_UVCIO(("^BUVC:[VS Probe control --GET_LEN]\r\n"));
		*pData++ = probe_len;
		ret_setup_data(pDataAddr, 1);
		break;

	case GET_DEF:
		DbgMsg_UVCIO(("^BUVC:[VS Probe control --GET_DEF]\r\n"));
		memset(pData, 0, sizeof(UVC_PROBE_COMMIT));
		pData += 2;
		*pData++ = 1;
		*pData++ = 1;

		*pData++ = UVC_VSFRM_FRAMEINTERVAL_DEF & 0xFF;
		*pData++ = (UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xFF;
		*pData++ = (UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xFF;
		*pData++ = (UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xFF;

		pData += 10;
		*pData++ = gU2UvcMJPGMaxTBR & 0xFF;
		*pData++ = (gU2UvcMJPGMaxTBR >> 8) & 0xFF;
		*pData++ = (gU2UvcMJPGMaxTBR >> 16) & 0xFF;
		*pData++ = (gU2UvcMJPGMaxTBR >> 24) & 0xFF;

		*pData++ = (gU2UvcMJPGMaxTBR + 2) & 0xFF;
		*pData++ = (gU2UvcMJPGMaxTBR >> 8) & 0xFF;
		*pData++ = (gU2UvcMJPGMaxTBR >> 16) & 0xFF;
		*pData++ = (gU2UvcMJPGMaxTBR >> 24) & 0xFF;

		datasize = probe_len;

		ret_setup_data(pDataAddr, datasize);
		break;

	default:
		DBG_ERR("^RUVC:[VS Probe control -- %d] not support\r\n", CS);
		break;
	}
	DbgMsg_UVCIO(("^B-%s:0x%x\r\n", __func__, CS));
}


/**
    UVC VideoStreaming Control -- Commit

    @param UINT8 CS       Control attribute
    @param UINT8 *pData   Data pointer
    @return void
*/

static void UVC_VS_CommitCtrl(UINT8 CS, UINT8 *pData, UVC_PROBE_COMMIT *pProbeCommit, UINT32 vidDevIdx)
{
	UINT8 *pDataAddr;
	UINT8 datasize;
	UINT32 len;
	UVAC_VIDEO_FORMAT codecType = UVAC_VIDEO_FORMAT_H264;
	UINT32 probe_len = (gUvacUvcVer == UVAC_UVC_VER_100)? USB_UVC_PROBE_LENGTH : USB_UVC_110_PROBE_LENGTH;

	pDataAddr = pData;
	DbgMsg_UVC(("^B%s:0x%x, vidDevIdx=%d\r\n", __func__, CS, vidDevIdx));
	switch (CS) {
	case SET_CUR:
		DbgMsg_UVC(("^B-UVC:[VS Commit control --SET_CUR],Prv frmIdx=%d\r\n", pProbeCommit->bFrameIndex));
		len = probe_len;
		if (E_OK != _readEP0(pData, &len)) {
			DBG_ERR("Commit SetCur readEndpoint fail\r\n");
		}
		pProbeCommit->bFormatIndex = *(pData + 2);
		pProbeCommit->bFrameIndex = *(pData + 3);
		pProbeCommit->dwFrameInterval = *(pData + 4);
		pProbeCommit->dwFrameInterval |= *(pData + 5) << 8;
		pProbeCommit->dwFrameInterval |= *(pData + 6) << 16;
		pProbeCommit->dwFrameInterval |= *(pData + 7) << 24;

		pProbeCommit->wKeyFrameRate = *(pData + 8);
		pProbeCommit->wKeyFrameRate |= (*(pData + 9) << 8);
		pProbeCommit->wPFrameRate = *(pData + 10);
		pProbeCommit->wPFrameRate |= (*(pData + 11) << 8);
		pProbeCommit->wCompQuality = *(pData + 12);
		pProbeCommit->wCompQuality |= (*(pData + 13) << 8);
		pProbeCommit->wCompWindowSize = *(pData + 14);
		pProbeCommit->wCompWindowSize |= (*(pData + 15) << 8);
		pProbeCommit->wDelay = *(pData + 16);
		pProbeCommit->wDelay |= (*(pData + 17) << 8);
		pProbeCommit->dwMaxVideoFrameSize = *(pData + 18);
		pProbeCommit->dwMaxVideoFrameSize |= (*(pData + 19) << 8);
		pProbeCommit->dwMaxVideoFrameSize |= (*(pData + 20) << 16);
		pProbeCommit->dwMaxVideoFrameSize |= (*(pData + 21) << 24);
		pProbeCommit->dwMaxPayloadTransferSize = *(pData + 22);
		pProbeCommit->dwMaxPayloadTransferSize |= (*(pData + 23) << 8);
		pProbeCommit->dwMaxPayloadTransferSize |= (*(pData + 24) << 16);
		pProbeCommit->dwMaxPayloadTransferSize |= (*(pData + 25) << 24);
		
		if (gUvacUvcVer >= UVAC_UVC_VER_110) {
			pProbeCommit->dwClockFrequency = *(pData + 26);
			pProbeCommit->dwClockFrequency |= (*(pData + 27) << 8);
			pProbeCommit->dwClockFrequency |= (*(pData + 28) << 16);
			pProbeCommit->dwClockFrequency |= (*(pData + 29) << 24);
			pProbeCommit->bmFramingInfo = *(pData + 30);
			pProbeCommit->bPreferredVersion = *(pData + 31);
			pProbeCommit->bMinVersion = *(pData + 32);
			pProbeCommit->bMaxVersion = *(pData + 33);
		}
		DbgMsg_UVC(("^B--UVC:[VS Commit control --SET_CUR],frmIdx=%d,payload=%d\r\n", pProbeCommit->bFrameIndex, pProbeCommit->dwMaxPayloadTransferSize));


		UVC_DmpProbeData(3, pProbeCommit);
		if (0 == pProbeCommit->bFrameIndex) {
			DBG_ERR("Wrong FrameIdx=%d,pProbeCommit=0x%x => Set2Previous, DEF=%d\r\n", pProbeCommit->bFrameIndex, (unsigned int)pProbeCommit, UVC_VSFMT_DEF_FRM_IDX);
			U2UVAC_SetImageSize(UVC_VSFMT_PREV_FRM_IDX, vidDevIdx);
		} else if (UVAC_VID_RESO_MAX_CNT >= pProbeCommit->bFrameIndex) {
			//DbgMsg_UVC(("SET_CUR,frmIdx=%d,vidDevIdx=%d,wid=%d,height=%d\r\n", pProbeCommit->bFrameIndex, vidDevIdx, gU2UvcVidResoAry[pProbeCommit->bFrameIndex - 1].width, gU2UvcVidResoAry[pProbeCommit->bFrameIndex - 1].height));
			DbgMsg_UVC(("SET_CUR,frmIdx=%d,vidDevIdx=%d\r\n", pProbeCommit->bFrameIndex, vidDevIdx));
			U2UVAC_SetImageSize(pProbeCommit->bFrameIndex, vidDevIdx);
		} else {
			DBG_ERR("Wrong FrameIdx=%d,vidDevIdx=%d, Stall\r\n", pProbeCommit->bFrameIndex, vidDevIdx);
			usb_setEP0Stall();
			break;
		}
		usb_setEP0Done();

		if (pProbeCommit->dwFrameInterval) {
			U2UVAC_SetFrameRate(10000000 / pProbeCommit->dwFrameInterval, vidDevIdx);
		} else {
			DBG_ERR("Host Set FrameInterval=0 Fail. Set to %d fps, intrf=%d\r\n", UVC_FRMRATE_30, vidDevIdx);
			U2UVAC_SetFrameRate(UVC_FRMRATE_30, vidDevIdx);
		}
		if (UVAC_VID_DEV_CNT_MAX > vidDevIdx) {
			if (pProbeCommit->bFormatIndex == h264_fmt_index[vidDevIdx]) {
				codecType = UVAC_VIDEO_FORMAT_H264;
			} else if (pProbeCommit->bFormatIndex == mjpeg_fmt_index[vidDevIdx]) {
				codecType = UVAC_VIDEO_FORMAT_MJPG;
			} else if (pProbeCommit->bFormatIndex == yuv_fmt_index) {
				codecType = UVAC_VIDEO_FORMAT_YUV;
			} else {
				DBG_ERR("unknow format index=%d, h264=%d, mjpeg=%d, yuv=%d\r\n", pProbeCommit->bFormatIndex, h264_fmt_index[vidDevIdx], mjpeg_fmt_index[vidDevIdx], yuv_fmt_index);
			}
			U2UVAC_SetCodec(codecType, vidDevIdx);
		}
		break;

	case GET_CUR:
		DbgMsg_UVCIO(("^BUVC:[VS Commit control --GET_CUR]\r\n"));
		memset(pData, 0, sizeof(UVC_PROBE_COMMIT));
		UVC_GetCurProbeCommitData(pData, pProbeCommit);
		datasize = probe_len;
		ret_setup_data(pDataAddr, datasize);
		break;

	case GET_INFO:
		DbgMsg_UVCIO(("^BUVC:[VS Commit control --GET_INFO]\r\n"));
		*pData++ = 0x03;
		ret_setup_data(pDataAddr, 1);
		break;

	case GET_LEN:
		DbgMsg_UVCIO(("^BUVC:[VS Commit control --GET_LEN]\r\n"));
		*pData++ = probe_len;
		ret_setup_data(pDataAddr, 1);
		break;

	case GET_MIN:
	case GET_MAX:
	case GET_RES:
	case GET_DEF:
	default:
		_ret_usb_stall();
		DBG_ERR("^RUVC:[VS Commit control -- %d] Not imp\r\n", CS);
		break;
	}

}

static void UVC_AC_ProbeCommit_GetCur(UINT8 *pData)
{
	UINT8 *pDataAddr;
	UINT32 datasize;
	UINT32 len = sizeof(gU2UacSampleRate);
	UINT32 i;

	pDataAddr = pData;
	datasize = (UINT32)REQUEST_LEN;
	datasize = (datasize < len) ? datasize : len;
	DbgMsg_UVC(("^GUAC--GET_CUR,gU2UacSampleRate=0x%x,len=%d,usblen=%d\r\n", gU2UacSampleRate, len, datasize));
	memset(pData, 0, datasize);
	for (i = 0; i < datasize; i++) {
		*pData++ = (gU2UacSampleRate >> (8 * i)) & 0xFF;
	}
	ret_setup_data(pDataAddr, datasize);
}
static void UVC_AC_ProbeCommit_SetCur(UINT8 *pData)
{
	UINT32 len = sizeof(gU2UacSampleRate);
	UINT32 datasize = (UINT32)REQUEST_LEN;
	UINT32 i = 0;
	UINT32 tmpSampleRate = 0;

	len = (len < datasize) ? len : datasize;
	memset(pData, 0, datasize);
	if (E_OK != _readEP0(pData, &datasize)) {
		DBG_ERR("SetCur readEndpoint fail\r\n");
		tmpSampleRate = UAC_FREQUENCY_DEF;
	} else {
		tmpSampleRate = 0;
		for (i = 0; i < len; i++) {
			tmpSampleRate += (*(pData + i) << (8 * i));
		}
	}
	DbgMsg_UVCIO(("^GUAC--SET_CUR,gU2UacSampleRate=0x%x,len=%d,usblen=%d\r\n", tmpSampleRate, len, datasize));
	if ((tmpSampleRate > UAC_FREQUENCY_48K) || (tmpSampleRate < UAC_FREQUENCY_08K)) {
		DBG_ERR("Aud-Sample-Rate: 0x%x, prev=0x%x\r\n", tmpSampleRate, gU2UacSampleRate);
		_ret_usb_stall();
	} else {
		gU2UacSampleRate = tmpSampleRate;
		usb_setEP0Done();
	}
#if (_UVC_DBG_LVL_ > _UVC_DBG_CHK_)
	{
		i = 0;
		while (i < datasize) {
			DBG_DUMP("0x%x ", *pData++);
			i++;
			if ((i % 16) == 0) {
				DBG_DUMP("\r\n");
			}
		}
		DBG_DUMP("\r\n");
	}
#endif
}
static void UVC_AC_ProbeCtrl(UINT8 CS, UINT8 *pData)
{
	UINT8 *pDataAddr;
	UINT32 datasize;

	pDataAddr = pData;
	DbgMsg_UVCIO(("^G%s:0x%x\r\n", __func__, CS));

	switch (CS) {
	case GET_CUR://0x81
		DbgMsg_UVCIO(("UVC:[AC Probe control --GET_CUR]\r\n"));
		UVC_AC_ProbeCommit_GetCur(pData);
		break;
	case SET_CUR:
		DbgMsg_UVCIO(("UVC:[AC Probe control --SET_CUR]\r\n"));
		UVC_AC_ProbeCommit_SetCur(pData);
		//UVC_AudSetSampleRate(gU2UacSampleRate);

		DbgMsg_UVCIO(("UAC Probe SetCurr:sampleRateFormat=%d\r\n", gU2UacSampleRate));
		break;
	case GET_MIN:
		DbgMsg_UVC(("UVC:[AC Probe control --GET_MIN]\r\n"));
		memset(pData, 0, USB_UAC_PROBE_LENGTH);
		*pData++ = UAC_FREQUENCY_MIN & 0xFF;
		*pData++ = (UAC_FREQUENCY_MIN >> 8) & 0xFF;
		datasize = USB_UAC_PROBE_LENGTH;
		ret_setup_data(pDataAddr, datasize);
		break;
	case GET_MAX:
		DbgMsg_UVC(("UVC:[AC Probe control --GET_MAX]\r\n"));
		memset(pData, 0, USB_UAC_PROBE_LENGTH);
		*pData++ = UAC_FREQUENCY_MAX & 0xFF;
		*pData++ = (UAC_FREQUENCY_MAX >> 8) & 0xFF;
		datasize = USB_UAC_PROBE_LENGTH;
		ret_setup_data(pDataAddr, datasize);
		break;

	case GET_RES:
		DbgMsg_UVCIO(("UVC:[AC Commit control --GET_RES]\r\n"));
		memset(pData, 0, USB_UAC_PROBE_LENGTH);
		*pData++ = 0x02;
		*pData++ = 0x00;
		datasize = USB_UAC_PROBE_LENGTH;
		ret_setup_data(pDataAddr, datasize);
		break;
	default:
		DBG_ERR("^RUVC:[AC Probe control -- %x] not support\r\n", CS);
		_ret_usb_stall();
		break;
	}

}
static void UVC_AC_CommitCtrl(UINT8 CS, UINT8 *pData)
{
	UINT8 *pDataAddr;
	UINT32 datasize;

	pDataAddr = pData;
	DbgMsg_UVCIO(("^G%s:0x%x\r\n", __func__, CS));

	switch (CS) {
	case GET_CUR://0x81
		DbgMsg_UVCIO(("UVC:[AC Commit control --GET_CUR]\r\n"));
		UVC_AC_ProbeCommit_GetCur(pData);
		break;

	case SET_CUR:
		DbgMsg_UVCIO(("^G+UVC:[AC Commit control --SET_CUR]\r\n"));
		UVC_AC_ProbeCommit_SetCur(pData);
		//UVC_AudSetSampleRate(gU2UacSampleRate);
		break;

	case GET_MIN:
		DbgMsg_UVCIO(("UVC:[AC Commit control --GET_MIN]\r\n"));
		memset(pData, 0, USB_UAC_PROBE_LENGTH);
		*pData++ = UAC_FREQUENCY_MIN & 0xFF;
		*pData++ = (UAC_FREQUENCY_MIN >> 8) & 0xFF;
		datasize = USB_UAC_PROBE_LENGTH;
		ret_setup_data(pDataAddr, datasize);
		break;
	case GET_MAX:
		DbgMsg_UVCIO(("UVC:[AC Commit control --GET_MAX]\r\n"));
		memset(pData, 0, USB_UAC_PROBE_LENGTH);
		*pData++ = UAC_FREQUENCY_MAX & 0xFF;
		*pData++ = (UAC_FREQUENCY_MAX >> 8) & 0xFF;
		datasize = USB_UAC_PROBE_LENGTH;
		ret_setup_data(pDataAddr, datasize);
		break;
	case GET_RES:
		DbgMsg_UVCIO(("UVC:[AC Commit control --GET_RES]\r\n"));

		memset(pData, 0, USB_UAC_PROBE_LENGTH);
		*pData++ = (gUACRes & 0xFF);
		*pData++ = (gUACRes >> 8) & 0xFF;
		datasize = USB_UAC_PROBE_LENGTH;
		ret_setup_data(pDataAddr, datasize);
		break;
	case 4://SET_RES
		datasize = REQUEST_LEN;
		DbgMsg_UVCIO(("^G+UAC:req4,s=%d\r\n", datasize));
		if (E_OK != _readEP0(pData, &datasize)) {
			DBG_ERR("UAC req-4 readEndpoint fail\r\n");
		} else {
			usb_setEP0Done();
		}
		gUACRes = *pData ;
		gUACRes += *(pData + 1) << 8;
		DbgMsg_UVCIO(("^G-UAC:req4,s=%d,0x%x,0x%x\r\n", datasize, *pData, *(pData + 1)));
		break;
	default:
		DBG_ERR("^RUAC-Commit %d not support\r\n", CS);

		usb_setEP0Stall();
		//usb_setEP0Done();
		break;
	}

}

static void UVC_ACO_ProbeCtrl(UINT8 CS, UINT8 *pData)
{
	UINT8 *pDataAddr;
	UINT32 datasize;

	pDataAddr = pData;
	DbgMsg_UVCIO(("^G%s:0x%x\r\n", __func__, CS));

	switch (CS) {
	case GET_CUR://0x81
		DbgMsg_UVCIO(("UVC:[AC Probe control --GET_CUR]\r\n"));
		UVC_AC_ProbeCommit_GetCur(pData);
		break;
	case SET_CUR:
		DbgMsg_UVCIO(("UVC:[AC Probe control --SET_CUR]\r\n"));
		UVC_AC_ProbeCommit_SetCur(pData);
		//UVC_AudSetSampleRate(gU2UacSampleRate);

		DbgMsg_UVCIO(("UAC Probe SetCurr:sampleRateFormat=%d\r\n", gU2UacSampleRate));
		break;
	case GET_MIN:
		DbgMsg_UVC(("UVC:[AC Probe control --GET_MIN]\r\n"));
		memset(pData, 0, USB_UAC_PROBE_LENGTH);
		*pData++ = UAC_FREQUENCY_MIN & 0xFF;
		*pData++ = (UAC_FREQUENCY_MIN >> 8) & 0xFF;
		datasize = USB_UAC_PROBE_LENGTH;
		ret_setup_data(pDataAddr, datasize);
		break;
	case GET_MAX:
		DbgMsg_UVC(("UVC:[AC Probe control --GET_MAX]\r\n"));
		memset(pData, 0, USB_UAC_PROBE_LENGTH);
		*pData++ = UAC_FREQUENCY_MAX & 0xFF;
		*pData++ = (UAC_FREQUENCY_MAX >> 8) & 0xFF;
		datasize = USB_UAC_PROBE_LENGTH;
		ret_setup_data(pDataAddr, datasize);
		break;

	case GET_RES:
		DbgMsg_UVCIO(("UVC:[AC Commit control --GET_RES]\r\n"));
		memset(pData, 0, USB_UAC_PROBE_LENGTH);
		*pData++ = 0x02;
		*pData++ = 0x00;
		datasize = USB_UAC_PROBE_LENGTH;
		ret_setup_data(pDataAddr, datasize);
		break;
	default:
		DBG_ERR("^RUVC:[AC Probe control -- %x] not support\r\n", CS);
		_ret_usb_stall();
		break;
	}

}
static void UVC_ACO_CommitCtrl(UINT8 CS, UINT8 *pData)
{
	UINT8 *pDataAddr;
	UINT32 datasize;

	pDataAddr = pData;
	DbgMsg_UVCIO(("^G%s:0x%x\r\n", __func__, CS));

	switch (CS) {
	case GET_CUR://0x81
		DbgMsg_UVCIO(("UVC:[AC Commit control --GET_CUR]\r\n"));
		UVC_AC_ProbeCommit_GetCur(pData);
		break;

	case SET_CUR:
		DbgMsg_UVCIO(("^G+UVC:[AC Commit control --SET_CUR]\r\n"));
		UVC_AC_ProbeCommit_SetCur(pData);
		//UVC_AudSetSampleRate(gU2UacSampleRate);
		break;

	case GET_MIN:
		DbgMsg_UVCIO(("UVC:[AC Commit control --GET_MIN]\r\n"));
		memset(pData, 0, USB_UAC_PROBE_LENGTH);
		*pData++ = UAC_FREQUENCY_MIN & 0xFF;
		*pData++ = (UAC_FREQUENCY_MIN >> 8) & 0xFF;
		datasize = USB_UAC_PROBE_LENGTH;
		ret_setup_data(pDataAddr, datasize);
		break;
	case GET_MAX:
		DbgMsg_UVCIO(("UVC:[AC Commit control --GET_MAX]\r\n"));
		memset(pData, 0, USB_UAC_PROBE_LENGTH);
		*pData++ = UAC_FREQUENCY_MAX & 0xFF;
		*pData++ = (UAC_FREQUENCY_MAX >> 8) & 0xFF;
		datasize = USB_UAC_PROBE_LENGTH;
		ret_setup_data(pDataAddr, datasize);
		break;
	case GET_RES:
		DbgMsg_UVCIO(("UVC:[AC Commit control --GET_RES]\r\n"));

		memset(pData, 0, USB_UAC_PROBE_LENGTH);
		*pData++ = (gUACRes & 0xFF);
		*pData++ = (gUACRes >> 8) & 0xFF;
		datasize = USB_UAC_PROBE_LENGTH;
		ret_setup_data(pDataAddr, datasize);
		break;
	case 4://SET_RES
		datasize = REQUEST_LEN;
		DbgMsg_UVCIO(("^G+UAC:req4,s=%d\r\n", datasize));
		if (E_OK != _readEP0(pData, &datasize)) {
			DBG_ERR("UAC req-4 readEndpoint fail\r\n");
		} else {
			usb_setEP0Done();
		}
		gUACRes = *pData ;
		gUACRes += *(pData + 1) << 8;
		DbgMsg_UVCIO(("^G-UAC:req4,s=%d,0x%x,0x%x\r\n", datasize, *pData, *(pData + 1)));
		break;
	default:
		DBG_ERR("^RUAC-Commit %d not support\r\n", CS);

		usb_setEP0Stall();
		//usb_setEP0Done();
		break;
	}

}

#if 1//(ENABLE == UVC_METHOD3)
void U2UVC_DmpStillProbeCommitData(PUVC_STILL_PROBE_COMMIT pStilProbComm)
{
#if (_UVC_DBG_LVL_ >= _UVC_DBG_CHK_)
	DBG_DUMP("+%s\r\n", __func__);
	DBG_DUMP("bFormatIndex=0x%x\r\n",                      pStilProbComm->bFormatIndex);
	DBG_DUMP("bFrameIndex=0x%x\r\n",                       pStilProbComm->bFrameIndex);
	DBG_DUMP("bCompressionIndex=0x%x\r\n",                 pStilProbComm->bCompressionIndex);
	DBG_DUMP("dwMaxVideoFrameSize=0x%x\r\n",               pStilProbComm->dwMaxVideoFrameSize);
	DBG_DUMP("dwMaxPayloadTransferSize=0x%x\r\n",          pStilProbComm->dwMaxPayloadTransferSize);
	DBG_DUMP("-%s\r\n", __func__);
#endif
}
void U2UVC_InitStillProbeCommitData(PUVC_STILL_PROBE_COMMIT pStilProbComm)
{
	pStilProbComm->bFormatIndex = UVC_VSFMT_FIRSTINDEX; //video format index;ex:1,2,...
	pStilProbComm->bFrameIndex = UVC_VS_STILL_DEFAULT_FRM_IDX;
	pStilProbComm->bCompressionIndex = 0; //no compression support now
	pStilProbComm->dwMaxVideoFrameSize = UVC_VS_STILL_FRM_MAX_SIZE;
	pStilProbComm->dwMaxPayloadTransferSize = USB_MAX_TXF_SIZE_ONE_PACKET;
}
static void UVC_GetCurStillProbeCommitData(UINT8 *pData)
{
	*pData++ = gU2UvcStillProbeCommit.bFormatIndex;
	*pData++ = gU2UvcStillProbeCommit.bFrameIndex;
	*pData++ = gU2UvcStillProbeCommit.bCompressionIndex;

	*pData++ = gU2UvcStillProbeCommit.dwMaxVideoFrameSize & 0xFF;
	*pData++ = (gU2UvcStillProbeCommit.dwMaxVideoFrameSize >> 8) & 0xFF;
	*pData++ = (gU2UvcStillProbeCommit.dwMaxVideoFrameSize >> 16) & 0xFF;
	*pData++ = (gU2UvcStillProbeCommit.dwMaxVideoFrameSize >> 24) & 0xFF;

#if 1
	*pData++ = (USB_MAX_TXF_SIZE_ONE_PACKET) & 0xFF;
	*pData++ = (USB_MAX_TXF_SIZE_ONE_PACKET >> 8) & 0xFF;
	*pData++ = (USB_MAX_TXF_SIZE_ONE_PACKET >> 16) & 0xFF;
	*pData++ = (USB_MAX_TXF_SIZE_ONE_PACKET >> 24) & 0xFF;
#else
	*pData++ = (gU2UvcStillProbeCommit.dwMaxPayloadTransferSize) & 0xFF;
	*pData++ = (gU2UvcStillProbeCommit.dwMaxPayloadTransferSize >> 8) & 0xFF;
	*pData++ = (gU2UvcStillProbeCommit.dwMaxPayloadTransferSize >> 16) & 0xFF;
	*pData++ = (gU2UvcStillProbeCommit.dwMaxPayloadTransferSize >> 24) & 0xFF;
#endif
}
BOOL U2UVC_TrIgStIlImg(UINT32 dataAddr, UINT32 dataSize)
{
	UVAC_TXF_INFO   txfInfo;
	memset((void *) &txfInfo, 0, sizeof(UVAC_TXF_INFO));
	DbgMsg_UVC(("%s:addr=0x%x,Size=0x%x,sts=%d,dmaAbort=%d\r\n", __func__, dataAddr, dataSize, gU2UvacStillImgTrigSts, gU2UvcUsbDMAAbord));
	if (FALSE == gU2UvcUsbDMAAbord) {
		txfInfo.sAddr = txfInfo.oriAddr = dataAddr;
		txfInfo.size = dataSize;
		txfInfo.usbEP = (UINT8)U2UVAC_USB_EP[UVAC_TXF_QUE_IMG];
		txfInfo.txfCnt = 0;
		txfInfo.streamType = UVAC_TXF_STREAM_STILLIMG;
		U2UVAC_AddIsoInTxfInfo(&txfInfo, UVAC_TXF_QUE_IMG);
		set_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_TXF);
		return TRUE;
	} else {
		DBG_ERR("DMA-Abort,Can not trigger still image\r\n");
#if 0
		DbgMsg_UVC(("^MDMA-Abort but try to bulk-in image\r\n"));
		txfInfo.sAddr = txfInfo.oriAddr = dataAddr;
		txfInfo.size = dataSize;
		//txfInfo.usbEP = (UINT8)UVAC_USB_EP[UVAC_TXF_QUE_IMG];
		txfInfo.usbEP = (UINT8)UVAC_USB_EP[UVAC_TXF_QUE_V1];
		txfInfo.txfCnt = 0;
		txfInfo.streamType = UVAC_TXF_STREAM_STILLIMG;
		//U2UVAC_AddIsoInTxfInfo(&txfInfo, UVAC_TXF_QUE_IMG);
		U2UVAC_AddIsoInTxfInfo(&txfInfo, UVAC_TXF_QUE_V1);
		gUvcUsbDMAAbord = FALSE;
		set_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_TXF);
#endif
		return FALSE;
	}
}
static void UVC_CapStilImgUploadTest(UINT8 *pData)
{
	DbgMsg_UVC(("^G%s TrigSts=%d\r\n", __func__, gU2UvacStillImgTrigSts));
#if 0
	gU2UvacStillImgTrigSts = UVC_STILLIMG_TRIG_CTRL_TRANSMIT_BULK;//2
	//UVC_TrigStilImg(gU2UvcCapImgAddr, gU2UvcCapImgSize);
#endif
}
static void UVC_CapStilImgUpload(UINT8 *pData)
{
	FLGPTN uiFlag;
	DbgMsg_UVC(("^G%s TrigSts=%d\r\n", __func__, gU2UvacStillImgTrigSts));
#if 0
	uiFlag = kchk_flg(FLG_ID_U2UVAC, FLGUVAC_START);
#else
	uiFlag = FLGUVAC_START;
#endif
	if (uiFlag & FLGUVAC_START) {
		//1. Start IPL flow to capture a full-size image => another task is needed
		//2. divide the image into several
		//3. set a flag to txf
		gU2UvacStillImgTrigSts = *pData; //UVC_STILLIMG_TRIG_CTRL_TRANSMIT_BULK;//2
		//gU2UvacStillImgTrigSts = UVC_STILLIMG_TRIG_CTRL_TRANSMIT_BULK;//2
#if 1//(ENABLE == UVC_METHOD3_CMDTEST)
		U2UVC_TrIgStIlImg(gU2UvcCapImgAddr, gU2UvcCapImgSize);
#endif
	}
}
static void UVC_VS_Still_ProbeCtrl(UINT8 req, UINT8 *pData, UINT8 intrf)
{
	UINT8 *pDataAddr = pData;
	UINT32 len = 0;

	DbgMsg_UVCIO(("^B+%s:req=%d,pDataAddr=0x%x, size=0x%x, intrf=%d, ctrl-len=%d\r\n", __func__, req, pData, len, intrf, REQUEST_LEN));
	switch (req) {
	case SET_CUR:
		DbgMsg_UVCIO(("^B+[Still_ProbeCtrl--SET_CUR],len=%d,gU2UvacStillImgTrigSts%d\r\n", len, gU2UvacStillImgTrigSts));
		len = UVC_STILL_PROBE_COMMIT_LEN;
		if (E_OK != _readEP0(pData, &len)) {
			DBG_ERR("SetCur readEndpoint fail\r\n");
		}
		usb_setEP0Done();
		gU2UvcStillProbeCommit.bFormatIndex = *pData;
		gU2UvcStillProbeCommit.bFrameIndex = *(pData + 1);
		gU2UvcStillProbeCommit.bCompressionIndex = *(pData + 2);
		gU2UvcStillProbeCommit.dwMaxVideoFrameSize = *(pData + 3);
		gU2UvcStillProbeCommit.dwMaxVideoFrameSize |= (*(pData + 4) << 8);
		gU2UvcStillProbeCommit.dwMaxVideoFrameSize |= (*(pData + 5) << 16);
		gU2UvcStillProbeCommit.dwMaxVideoFrameSize |= (*(pData + 6) << 24);
		gU2UvcStillProbeCommit.dwMaxPayloadTransferSize = *(pData + 7);
		gU2UvcStillProbeCommit.dwMaxPayloadTransferSize |= (*(pData + 8) << 8);
		gU2UvcStillProbeCommit.dwMaxPayloadTransferSize |= (*(pData + 9) << 16);
		gU2UvcStillProbeCommit.dwMaxPayloadTransferSize |= (*(pData + 10) << 24);
		DbgMsg_UVCIO(("^B-[Still_ProbeCtrl--SET_CUR],fmtIdx=%d,frmIdx=%d,frmSize=0x%x,txfSize=0x%x\r\n", gU2UvcStillProbeCommit.bFormatIndex, \
					  gU2UvcStillProbeCommit.bFrameIndex, gU2UvcStillProbeCommit.dwMaxVideoFrameSize, gU2UvcStillProbeCommit.dwMaxPayloadTransferSize));
		break;
	case GET_CUR:
		DbgMsg_UVCIO(("^B[Still_ProbeCtrl--GET_CUR]\r\n"));
		U2UVC_DmpStillProbeCommitData(&gU2UvcStillProbeCommit);
		memset(pData, 0, sizeof(UVC_STILL_PROBE_COMMIT));
		UVC_GetCurStillProbeCommitData(pData);
		len = UVC_STILL_PROBE_COMMIT_LEN;
		ret_setup_data(pDataAddr, len);
		break;
	case GET_MIN://0x82, 130
	case GET_MAX://0x83, 131
	case GET_DEF://0x87, 135
		if (GET_MIN == req) {
			len = UVC_VS_STILL_FRM_MIN_SIZE;
		} else if (GET_DEF == req) {
			len = UVC_VS_STILL_FRM_DEF_SIZE;
		} else {
			len = UVC_VS_STILL_FRM_MAX_SIZE;
		}
		DbgMsg_UVCIO(("^B[Still_ProbeCtrl--GET_MIN/MAX]:req=%d,len=0x%x\r\n", req, len));
		U2UVC_InitStillProbeCommitData((PUVC_STILL_PROBE_COMMIT)pData);
		pData += 3;
		*pData++ = len & 0xFF;
		*pData++ = (len >> 8) & 0xFF;
		*pData++ = (len >> 16) & 0xFF;
		*pData++ = (len  >> 24) & 0xFF;


#if 1
		*pData++ = (USB_MAX_TXF_SIZE_ONE_PACKET) & 0xFF;
		*pData++ = (USB_MAX_TXF_SIZE_ONE_PACKET >> 8) & 0xFF;
		*pData++ = (USB_MAX_TXF_SIZE_ONE_PACKET >> 16) & 0xFF;
		*pData++ = (USB_MAX_TXF_SIZE_ONE_PACKET >> 24) & 0xFF;
#else
		len += 2;
		*pData++ = len & 0xFF;
		*pData++ = (len >> 8) & 0xFF;
		*pData++ = (len >> 16) & 0xFF;
		*pData++ = (len  >> 24) & 0xFF;
#endif
		ret_setup_data(pDataAddr, UVC_STILL_PROBE_COMMIT_LEN);
		break;
	case GET_INFO:
		DbgMsg_UVCIO(("^B[Still_ProbeCtrl--GET_INFO]\r\n"));
		*pData = 0x03;
		ret_setup_data(pDataAddr, 1);
		break;
	case GET_LEN:
		DbgMsg_UVCIO(("^B[Still_ProbeCtrl--GET_LEN]\r\n"));
		*pData = UVC_STILL_PROBE_COMMIT_LEN;
		ret_setup_data(pDataAddr, 1);
		break;
	case GET_RES:
	default:
		usb_setEP0Stall();
		//usb_setEP0Done();
		DBG_ERR("Still_ProbeCtrl-Not implement:%d\r\n", req);
		break;
	}
	DbgMsg_UVCIO(("^B-%s:req=%d,pDataAddr=0x%x, size=0x%x, intrf=%d\r\n", __func__, req, (UINT32)pDataAddr, len, intrf));
}
void UVC_VS_Still_CommitCtrl(UINT8 req, UINT8 *pData, UINT8 intrf)
{
	UINT8 *pDataAddr = pData;
	UINT32 len = 0;

	DbgMsg_UVCIO(("^B+%s:req=%d,pDataAddr=0x%x, size=0x%x, intrf=%d, ctrl-len=%d\r\n", __func__, req, pData, len, intrf, CONTROL_DATA.w_length));
	switch (req) {
	case SET_CUR:
		DbgMsg_UVCIO(("^B+[Still_CommitCtrl--SET_CUR],len=%d,gU2UvacStillImgTrigSts%d\r\n", len, gU2UvacStillImgTrigSts));
		len = UVC_STILL_PROBE_COMMIT_LEN;
		if (E_OK != _readEP0(pData, &len)) {
			DBG_ERR("SetCur readEndpoint fail\r\n");
		}
		if (*pData != (gU2UvcStillProbeCommit.bFormatIndex)) {
			DBG_ERR("Not Match Format,in=%d,ori=%d\r\n", *pData, gU2UvcStillProbeCommit.bFormatIndex);
			usb_setEP0Stall();
		} else {
			gU2UvcStillProbeCommit.bFormatIndex = *pData;
			gU2UvcStillProbeCommit.bFrameIndex = *(pData + 1);
			gU2UvcStillProbeCommit.bCompressionIndex = *(pData + 2);
			gU2UvcStillProbeCommit.dwMaxVideoFrameSize = *(pData + 3);
			gU2UvcStillProbeCommit.dwMaxVideoFrameSize |= (*(pData + 4) << 8);
			gU2UvcStillProbeCommit.dwMaxVideoFrameSize |= (*(pData + 5) << 16);
			gU2UvcStillProbeCommit.dwMaxVideoFrameSize |= (*(pData + 6) << 24);
			gU2UvcStillProbeCommit.dwMaxPayloadTransferSize = *(pData + 7);
			gU2UvcStillProbeCommit.dwMaxPayloadTransferSize |= (*(pData + 8) << 8);
			gU2UvcStillProbeCommit.dwMaxPayloadTransferSize |= (*(pData + 9) << 16);
			gU2UvcStillProbeCommit.dwMaxPayloadTransferSize |= (*(pData + 10) << 24);
			usb_setEP0Done();
		}
		DbgMsg_UVCIO(("^B-[Still_CommitCtrl--SET_CUR],fmtIdx=%d,frmIdx=%d,frmSize=0x%x,txfSize=0x%x\r\n", gU2UvcStillProbeCommit.bFormatIndex, \
					  gU2UvcStillProbeCommit.bFrameIndex, gU2UvcStillProbeCommit.dwMaxVideoFrameSize, gU2UvcStillProbeCommit.dwMaxPayloadTransferSize));

		//call back to trigger take-a-photo event
		UVC_CapStilImgUploadTest(pData);
		break;
	case GET_CUR:
		DbgMsg_UVCIO(("^B[Still_CommitCtrl--GET_CUR]\r\n"));
		U2UVC_DmpStillProbeCommitData(&gU2UvcStillProbeCommit);
		memset(pData, 0, sizeof(UVC_STILL_PROBE_COMMIT));
		UVC_GetCurStillProbeCommitData(pData);
		len = UVC_STILL_PROBE_COMMIT_LEN;
		ret_setup_data(pDataAddr, len);
		break;
	case GET_INFO:
		DbgMsg_UVCIO(("^B[Still_CommitCtrl--GET_INFO]\r\n"));
		*pData = 0x03;
		ret_setup_data(pDataAddr, 1);
		break;
	case GET_LEN:
		DbgMsg_UVCIO(("^B[Still_CommitCtrl--GET_LEN]\r\n"));
		*pData = UVC_STILL_PROBE_COMMIT_LEN;
		ret_setup_data(pDataAddr, 1);
		break;
	case GET_MIN://0x82, 130
	case GET_MAX://0x83, 131
	case GET_DEF://0x87, 135
	case GET_RES:
	default:
		usb_setEP0Stall();
		//usb_setEP0Done();
		DBG_ERR("Still_CommitCtrl-Not implement:%d\r\n", req);
		break;
	}
	DbgMsg_UVCIO(("^B-%s:req=%d,pDataAddr=0x%x, size=0x%x, intrf=%d\r\n", __func__, req, (UINT32)pDataAddr, len, intrf));
}
void UVC_VS_Still_ImgTrigCtrl(UINT8 req, UINT8 *pData, UINT8 intrf)
{
	UINT8 *pDataAddr = pData;
	UINT32 len = 0;

	DbgMsg_UVCIO(("^B+%s:req=%d,pDataAddr=0x%x, size=0x%x, intrf=%d\r\n", __func__, req, pData, len, intrf));
	switch (req) {
	case SET_CUR:
		len = UVC_STILLIMG_TRIG_REQ_LEN;
		DbgMsg_UVCIO(("^B+[Still_ImgTrigCtrl--SET_CUR],len=%d,gU2UvacStillImgTrigSts%d\r\n", len, gU2UvacStillImgTrigSts));
		if (E_OK != _readEP0(pData, &len)) {
			DBG_ERR("SetCur readEndpoint fail\r\n");
		}
		usb_setEP0Done();
		DbgMsg_UVCIO(("^B-[Still_ImgTrigCtrl--SET_CUR],len=%d,bTrigger=%d,preTrig=%d\r\n", len, *pData, gU2UvacStillImgTrigSts));

		UVC_CapStilImgUpload(pData);
		break;

	case GET_CUR:
		DbgMsg_UVCIO(("^B[Still_ImgTrigCtrl--GET_CUR]\r\n"));
		*pData = gU2UvacStillImgTrigSts;
		ret_setup_data(pDataAddr, UVC_STILLIMG_TRIG_REQ_LEN);
		break;
	case GET_INFO:
		DbgMsg_UVCIO(("^B[Still_ImgTrigCtrl--GET_INFO]\r\n"));
		*pData = 0x03;
		ret_setup_data(pDataAddr, 1);
		break;

	default:
		usb_setEP0Stall();
		//usb_setEP0Done();
		DBG_ERR("Not in UVC-Spec:%d\r\n", req);
		break;
	}
	DbgMsg_UVCIO(("^B-%s:req=%d,pDataAddr=0x%x, size=0x%x, intrf=%d\r\n", __func__, req, (UINT32)pDataAddr, len, intrf));
}
#endif

static void UVC_CDC_ClassReq(UINT8 nInterface, UINT8 Request)
{
	ER       retV;
	UINT32   len = 0;
	CDC_COM_ID ComID;
	if (gU2UvacIntfIdx_CDC_COMM[CDC_COM_1ST] == nInterface || gU2UvacIntfIdx_CDC_DATA[CDC_COM_1ST] == nInterface) {
		ComID = CDC_COM_1ST;
	} else {
		ComID = CDC_COM_2ND;
	}
	if (Request == REQ_GET_LINE_CODING) {
		if (gfpU2CdcPstnReqCB) {
			CDCLineCoding LineCoding = {0};
			len = sizeof(LineCoding);
			if (gfpU2CdcPstnReqCB(ComID, Request, (UINT8 *)&LineCoding, &len)) {
				if (len > sizeof(LineCoding)) {
					DBG_ERR("LinCoding data size should NOT exceed %d byte.\r\n", sizeof(LineCoding));
					len = sizeof(LineCoding);
				}
				ret_setup_data((UINT8 *)&LineCoding, len);
				return;
			} else {
				usb_setEP0Stall();
			}
		} else {
			usb_setEP0Stall();
		}
	} else if (Request == REQ_SET_LINE_CODING) {
		if (gfpU2CdcPstnReqCB) {
			CDCLineCoding LineCoding;
			len = CONTROL_DATA.device_request.w_length;
			retV = _readEP0((UINT8 *)&LineCoding, &len);
			if (retV != E_OK) {
				DBG_ERR("SetCancelReq,ReadEP=%d Fail=%d\r\n", USB_EP0, retV);
			} else {
				gfpU2CdcPstnReqCB(ComID, Request, (UINT8 *)&LineCoding, &len);
			}
			usb_setEP0Done();
		} else {
			usb_setEP0Stall();
		}
	} else if (Request == REQ_SET_CONTROL_LINE_STATE) {
		UINT16 ControlLineState = CONTROL_DATA.device_request.w_value;
		len = sizeof(ControlLineState);
		if (gfpU2CdcPstnReqCB) {
			gfpU2CdcPstnReqCB(ComID, Request, (UINT8 *)&ControlLineState, &len);
		}
		usb_setEP0Done();
	} else if (Request == REQ_SEND_BREAK) {
		UINT16 SendBreak = CONTROL_DATA.device_request.w_value;
		len = sizeof(SendBreak);
		if (gfpU2CdcPstnReqCB) {
			gfpU2CdcPstnReqCB(ComID, Request, (UINT8 *)&SendBreak, &len);
		}
		usb_setEP0Done();
	} else {
		DBG_ERR("Unsupported ClassReq=0x%x\r\n", CONTROL_DATA.device_request.b_request);
		//stall EP0, including unreconginze command and unsupport command.
		usb_setEP0Stall();
	}
	DBG_MSG("[CHK]--classRequest\r\n");
}
#if 0
static void UVC_SIDC_ClassReq(UINT8 bRequest)
{
	UINT8    bClassData[USIDC_CLS_DATA_LEN] = {0};
	UINT16   i = 0;
	UINT32   len = 0;
	ER       retV = E_OK;

	if (bRequest == USB_ClassReq_USIDC_GetDeviceStatusReq) {
		DBG_DUMP("GetDevStsReq:DevSts=%d\r\n", g_UvacDeviceState);

		i = USIDC_CLS_DATA_LEN;
		bClassData[0] = (UINT8) i;
		bClassData[3] = 0x20;               /*hard code response code*/

		if (g_UvacDeviceState == USIDC_DEVICE_OK) {
			bClassData[2] = 0x01;   //Device Ok
		} else {
			bClassData[2] = 0x19;   //Device Busy
		}
		g_UvacDeviceState = USIDC_DEVICE_OK;
		if (i > CONTROL_DATA.device_request.w_length) {
			i = CONTROL_DATA.device_request.w_length;
		}
		len = (i  < EP0_PACKET_SIZE) ? i : EP0_PACKET_SIZE;
		//driver will handle the cache issue
		//cpu_cleanInvalidateDCacheBlock((UINT32)bClassData, (((UINT32)bClassData) + len - 1));

		retV = usb_writeEndpoint(USB_EP0, (UINT8 *)bClassData, &len);
#if (THIS_DBGLVL >= 2)
		if (len != i) {
			DBG_ERR("ClsReq GetDevSts Len fail:%d,act=%d\r\n", i, len);
		}
#endif
		if (retV != E_OK) {
			DBG_ERR("ClsReq GetDevSts,WriteEP=%d Fail=%d\r\n", USB_EP0, retV);
		}
		usb_setEP0Done();
	} else if (bRequest == USB_ClassReq_USIDC_SetResetReq) {
		DBG_ERR("#SetResetReq\r\n");
		usb_setEP0Done();
		g_UvacSessionID = 0;
	} else if (bRequest == USB_ClassReq_USIDC_SetCancelReq) {
		DBG_ERR("#SetCancelReq\r\n");

		if ((CONTROL_DATA.device_request.w_index == 0)  && \
			(CONTROL_DATA.device_request.w_length == 0x06) && \
			(CONTROL_DATA.device_request.w_value == 0)) {
			gUvacStop = 1;
		}
		g_UvacDeviceState = USIDC_DEVICE_BUSY;
		len = CONTROL_DATA.device_request.w_length;
		if (len > USIDC_CLS_DATA_LEN) {
			len = USIDC_CLS_DATA_LEN;
		}
#if (THIS_DBGLVL >= 2)
		i = len;
#endif
		retV = _readEP0((UINT8 *)bClassData, &len);
#if (THIS_DBGLVL >= 2)
		if (len != i) {
			DBG_ERR("SetCancelReq Len fail:%d,act=%d\r\n", i, len);
		}
#endif
		if (retV != E_OK) {
			DBG_ERR("SetCancelReq,ReadEP=%d Fail=%d\r\n", USB_EP0, retV);
		}
		usb_setEP0Done();
	} else {
		DBG_ERR("Special ClassReq=0x%x\r\n", bRequest);
		//stall EP0, including unreconginze command and unsupport command.
		usb_setEP0Stall();
		usb_setEP0Done();
	}

	DBG_MSG("[CHK]--classRequest\r\n");
}
#endif
static void UVC_AC_MuteCtrl(UINT8 CS, UINT8 *pData)
{
	UINT8 *pDataAddr;
	UINT32 datasize;

	pDataAddr = pData;
	DbgMsg_UVCIO(("^G%s:0x%x\r\n", __func__, CS));

	switch (CS) {
	case GET_CUR: { //0x81
			DbgMsg_UVCIO(("^BUAC:[AS Mute control --GET_CUR]\r\n"));

			datasize = 1;
			memset(pData, 0, datasize);
			*pData = gUacMute;
			ret_setup_data(pDataAddr, datasize);
			break;
		}
	case SET_CUR: {
			DbgMsg_UVCIO(("^BUAC:[AS Mute control --SET_CUR]\r\n"));
			BOOL tmpMute;

			datasize = 1;
			memset(pData, 0, datasize);
			if (E_OK != _readEP0(pData, &datasize)) {
				DBG_ERR("SetCur readEndpoint fail\r\n");
				tmpMute = FALSE;
			} else {
				tmpMute = *pData;
			}

			if (tmpMute > 1) {
				DBG_ERR("Aud-Mute: 0x%x, prev=0x%x\r\n", tmpMute, gUacMute);
				usb_setEP0Stall();
				//usb_setEP0Done();
			} else {
				gUacMute = tmpMute;

				if (gUacMute) {
					if (gU2UacSetVolCB)
						gU2UacSetVolCB(1);
					else
						DbgMsg_UVCIO(("gU2UacSetVolCB is NULL\r\n"));
				} else {
					if (gU2UacSetVolCB)
						gU2UacSetVolCB(gUacVol);
					else
						DbgMsg_UVCIO(("gU2UacSetVolCB is NULL\r\n"));
				}

				DbgMsg_UVCIO(("UAC AS Mute control:Mute=%d\r\n", gUacMute));

				usb_setEP0Done();
			}
			break;
		}
	case GET_MIN:
	case GET_MAX:
	case GET_RES:
	default:
		DBG_ERR("^RUAC:[AC MuteCtrl -- %x] not support\r\n", CS);
		usb_setEP0Stall();
		//usb_setEP0Done();
		break;
	}
}

static void UVC_AC_VolCtrl(UINT8 CS, UINT8 *pData)
{
	UINT8 *pDataAddr;
	UINT32 datasize;

	pDataAddr = pData;
	DbgMsg_UVCIO(("^G%s:0x%x\r\n", __func__, CS));

	switch (CS) {
	case GET_CUR: { //0x81
			DbgMsg_UVCIO(("^BUAC:[AS Vol control --GET_CUR]\r\n"));

			datasize = 2;
			memset(pData, 0, datasize);

			*pData++ = gUacVol & 0xFF;
			*pData++ = (gUacVol >> 8) & 0xFF;
			ret_setup_data(pDataAddr, datasize);
			break;
		}
	case SET_CUR: {
			DbgMsg_UVCIO(("^BUAC:[AS Vol control --SET_CUR]\r\n"));
			BOOL tmpVol;

			datasize = 2;
			memset(pData, 0, datasize);
			if (E_OK != _readEP0(pData, &datasize)) {
				DBG_ERR("SetCur readEndpoint fail\r\n");
				tmpVol = 0;
			} else {
				tmpVol = *pData + (*(pData + 1) << 8);
			}

			gUacVol = tmpVol;

			if (gUacMute == 0) {
				if (gU2UacSetVolCB)
					gU2UacSetVolCB(gUacVol);
				else
					DbgMsg_UVCIO(("gU2UacSetVolCB is NULL\r\n"));
			}

			DbgMsg_UVCIO(("UAC AS Vol control:Volume=%d\r\n", gUacVol));

			usb_setEP0Done();
			break;
		}
	case GET_MIN:
	case GET_MAX:
	case GET_RES: {
			UINT32 tmpsetting;

			if (CS == GET_RES) {
				tmpsetting = UAC_VOL_RES;
			} else if (CS == GET_MAX) {
				tmpsetting = UAC_VOL_MAX;
			} else {
				tmpsetting = UAC_VOL_MIN;
			}

			datasize = 2;
			memset(pData, 0, datasize);

			*pData++ = tmpsetting & 0xFF;
			*pData++ = (tmpsetting >> 8) & 0xFF;

			ret_setup_data(pDataAddr, datasize);
			break;
		}
	default:
		DBG_ERR("^RUAC:[AC VolCtrl -- %x] not support\r\n", CS);
		usb_setEP0Stall();
		//usb_setEP0Done();
		break;
	}
}
static void UVC_ACO_MuteCtrl(UINT8 CS, UINT8 *pData)
{
	UINT8 *pDataAddr;
	UINT32 datasize;

	pDataAddr = pData;
	DBG_IND("^G%s:0x%x\r\n", __func__, CS);

	switch (CS) {
	case GET_CUR: { //0x81
			DBG_IND("^BUAC:[AS Mute control --GET_CUR]\r\n");

			datasize = 1;
			memset(pData, 0, datasize);
			*pData = gUacMute;
			ret_setup_data(pDataAddr, datasize);
			break;
		}
	case SET_CUR: {
			DBG_IND("^BUAC:[AS Mute control --SET_CUR]\r\n");
			BOOL tmpMute;

			datasize = 1;
			memset(pData, 0, datasize);
			if (E_OK != _readEP0(pData, &datasize)) {
				DBG_ERR("SetCur readEndpoint fail\r\n");
				tmpMute = FALSE;
			} else {
				tmpMute = *pData;
			}

			if (tmpMute > 1) {
				DBG_ERR("Aud-Mute: 0x%x, prev=0x%x\r\n", tmpMute, gUacMute);
				usb_setEP0Stall();
				//usb_setEP0Done();
			} else {
				DBG_IND("UAC AS Mute control:Mute=%d\r\n", gUacMute);

				usb_setEP0Done();
			}
			break;
		}
	case GET_MIN:
	case GET_MAX:
	case GET_RES:
	default:
		DBG_ERR("^RUAC:[AC MuteCtrl -- %x] not support\r\n", CS);
		usb_setEP0Stall();
		//usb_setEP0Done();
		break;
	}
}

static void UVC_ACO_VolCtrl(UINT8 CS, UINT8 *pData)
{
	UINT8 *pDataAddr;
	UINT32 datasize;

	pDataAddr = pData;
	DBG_IND("^G%s:0x%x\r\n", __func__, CS);

	switch (CS) {
	case GET_CUR: { //0x81
			DBG_IND("^BUAC:[AS Vol control --GET_CUR]\r\n");

			datasize = 2;
			memset(pData, 0, datasize);

			*pData++ = gUacVol & 0xFF;
			*pData++ = (gUacVol >> 8) & 0xFF;
			ret_setup_data(pDataAddr, datasize);
			break;
		}
	case SET_CUR: {
			DBG_IND("^BUAC:[AS Vol control --SET_CUR]\r\n");

			datasize = 2;
			memset(pData, 0, datasize);
			if (E_OK != _readEP0(pData, &datasize)) {
				DBG_ERR("SetCur readEndpoint fail\r\n");
			} else {
			}

			usb_setEP0Done();
			break;
		}
	case GET_MIN:
	case GET_MAX:
	case GET_RES: {
			UINT32 tmpsetting;

			if (CS == GET_RES) {
				tmpsetting = UAC_VOL_RES;
			} else if (CS == GET_MAX) {
				tmpsetting = UAC_VOL_MAX;
			} else {
				tmpsetting = UAC_VOL_MIN;
			}

			datasize = 2;
			memset(pData, 0, datasize);

			*pData++ = tmpsetting & 0xFF;
			*pData++ = (tmpsetting >> 8) & 0xFF;

			ret_setup_data(pDataAddr, datasize);
			break;
		}
	default:
		DBG_ERR("^RUAC:[AC VolCtrl -- %x] not support\r\n", CS);
		usb_setEP0Stall();
		//usb_setEP0Done();
		break;
	}
}

static void UVC_ACO_EP_Freq_Ctrl(UINT8 CS, UINT8 *pData)
{
	UINT32 datasize;

	DBG_IND("^G%s:0x%x\r\n", __func__, CS);

	switch (CS) {
	case SET_CUR: {
			//UINT32 freq = 0;
			datasize = (UINT32)REQUEST_LEN;

			memset(pData, 0, datasize);
			/*if (E_OK != _readEP0(pData, &datasize)) {
				DBG_ERR("SetCur readEndpoint fail\r\n");
			}*/

			//freq = *pData + (*(pData + 1) << 8) + (*(pData + 2) << 8);

			DBG_IND("freq = %d\r\n", freq);

			usb_setEP0Done();
			break;
		}
	case GET_CUR:
	case GET_MIN:
	case GET_MAX:
	case GET_RES:

	default:
		DBG_ERR("^RUAC:[AC VolCtrl -- %x] not support\r\n", CS);
		usb_setEP0Stall();
		//usb_setEP0Done();
		break;
	}
}

static void UVC_DFU_ClassReq(UINT8 request, UINT8 *p_data)
{
	INT32 length;
	UINT16 wBlockNum;

	DbgMsg_UVCIO(("request=0x%x, pData=0x%x\r\n", request, p_data));

	length = (INT32)CONTROL_DATA.device_request.w_length;
	wBlockNum = CONTROL_DATA.device_request.w_value;
	gUvacVendDataLen = 0;
	if (DFU_DNLOAD == request) {
		if (wBlockNum == 0) {
			dfu_remain_buf.addr = g_dfu_info.download_buf.addr;
			dfu_remain_buf.size = g_dfu_info.download_buf.size;
		}
		p_data = (UINT8 *)dfu_remain_buf.addr;
		while (length > 0) {
			if (length > USB_UVAC_CLS_DATABUF_LEN) {
				gUvacVendDataLen = USB_UVAC_CLS_DATABUF_LEN;
			} else {
				gUvacVendDataLen = length;
			}
			if (dfu_remain_buf.size < gUvacVendDataLen) {
				DBG_ERR("download_buf insufficient!\r\n");
				_ret_usb_stall();
				return;
			}
			//memset(p_data, 0, USB_UVAC_CLS_DATABUF_LEN);
			if (_readEP0((UINT8 *)dfu_remain_buf.addr, &gUvacVendDataLen) != E_OK) {
				DBG_ERR("Read failed!\r\n");
				_ret_usb_stall();
				return;
			}
			length -= gUvacVendDataLen;
			dfu_remain_buf.addr += gUvacVendDataLen;
			dfu_remain_buf.size -= gUvacVendDataLen;
		}
		usb_setEP0Done();
		gUvacVendDataLen = CONTROL_DATA.device_request.w_length;
		if (g_dfu_info.cb && g_dfu_info.cb(UVAC_DFU_REQUEST, request, wBlockNum, p_data, &gUvacVendDataLen)) {
		} else {
			DBG_ERR("not support!\r\n");
			_ret_usb_stall();
		}
	} else if (g_dfu_info.cb && g_dfu_info.cb(UVAC_DFU_REQUEST, request, wBlockNum, p_data, &gUvacVendDataLen)) {
		DbgMsg_UVCIO(("VendCmd Ok, retLen=%d,request=0x%x, p_data=0x%x\r\n", gUvacVendDataLen, request, p_data));
		DbgMsg_UVCIO(("  ==> *p_data=0x%x\r\n", *p_data));
		//DBG_DUMP("VendCmd Ok, retLen=%d, p_data=0x%x", gUvacVendDataLen,  p_data);
		//DBG_DUMP("==> *p_data=0x%x\r\n", *p_data);
		if (gUvacVendDataLen) {
			ret_setup_data(p_data, gUvacVendDataLen);
		}
	} else {
		_ret_usb_stall();
	}


}
static void UVC_HID_ClassReq(UINT8 request, UINT8 *p_data)
{
	INT32 length;
	UINT16 wValue;

	DbgMsg_UVCIO(("request=0x%x, pData=0x%x\r\n", request, p_data));

	length = (INT32)CONTROL_DATA.device_request.w_length;
	wValue = CONTROL_DATA.device_request.w_value;

	//gUvacVendDataLen = 0;

	if (g_hid_info.cb) {
		if (HID_SET_REPORT == request || HID_SET_IDLE == request || HID_SET_PROTOCOL == request) {
			if (length > USB_UVAC_CLS_DATABUF_LEN) {
				UINT32 remain_addr;

				if (g_hid_info.data_stage_buf.size < (UINT32)length) {
					DBG_ERR("data buf size(%d) < wLength(%d)\r\n", g_hid_info.data_stage_buf.size, length);
					return _ret_usb_stall();
				}
				remain_addr = g_hid_info.data_stage_buf.addr;
				p_data = (UINT8 *)g_hid_info.data_stage_buf.addr;
				while (length > 0) {
					if (length > USB_UVAC_CLS_DATABUF_LEN) {
						gUvacVendDataLen = USB_UVAC_CLS_DATABUF_LEN;
					} else {
						gUvacVendDataLen = length;
					}
					//memset(p_data, 0, USB_UVAC_CLS_DATABUF_LEN);
					if (_readEP0((UINT8 *)remain_addr, &gUvacVendDataLen) != E_OK) {
						DBG_ERR("Read failed!\r\n");
						_ret_usb_stall();
						return;
					}
					length -= gUvacVendDataLen;
					remain_addr += gUvacVendDataLen;
				}
				gUvacVendDataLen = CONTROL_DATA.device_request.w_length;
			} else {
				gUvacVendDataLen = CONTROL_DATA.device_request.w_length;
				memset(p_data, 0, USB_UVAC_CLS_DATABUF_LEN);
				if (_readEP0(p_data, &gUvacVendDataLen) != E_OK) {
					DBG_ERR("Read failed!\r\n");
					_ret_usb_stall();
					return;
				}
			}
			if (g_hid_info.cb(request, wValue, p_data, &gUvacVendDataLen)) {
			} else {
				DbgMsg_UVC(("request(%d) with wValue(0x%X) not supported!\r\n", request, wValue));
				//some host might not support stall of this cmd
				//_ret_usb_stall();
			}
			usb_setEP0Done();
		} else {
			gUvacVendDataLen = CONTROL_DATA.device_request.w_length;
			if (g_hid_info.cb(request, wValue, p_data, &gUvacVendDataLen)) {
				//DBG_DUMP("VendCmd Ok, retLen=%d,ctrl=%d, cs=0x%x, p_data=0x%x", gUvacVendDataLen, ControlCode, CS, p_data);
				//DBG_DUMP("==> *p_data=0x%x\r\n", *p_data);
				if (gUvacVendDataLen) {
					if (gUvacVendDataLen > USB_UVAC_CLS_DATABUF_LEN) {
						CONTROL_DATA.p_data = (UINT8 *)*(UINT32 *)p_data;
					} else {
						CONTROL_DATA.p_data = p_data;
					}
					CONTROL_DATA.w_length = gUvacVendDataLen;
					usb_RetSetupData();
				}
			} else {
				_ret_usb_stall();
			}
		}
	} else {
		_ret_usb_stall();
	}


}
/*---------------------------------------------------------------------------
 TPBulk_ClassRequestHandler

 Input      : none
 Output     : none
 Return Val : none
 Function   : processing of USB setup transaction TYPE = CLASS
----------------------------------------------------------------------------*/
void U2UvacClassRequestHandler(void)
{
	UINT8 ControlCode;
	UINT8 nInterface;
	UINT8 Unit_ID;
	UINT8 bRequest;
	UINT16 datalen;
	UINT8 *pData;
	UINT32 thisDevIdx = UVAC_VID_DEV_CNT_1;
	UVC_PROBE_COMMIT *pProbeCommit = 0;


	ControlCode = CONTROL_DATA.device_request.w_value >> 8;
	nInterface = CONTROL_DATA.device_request.w_index & 0xFF;
	Unit_ID = CONTROL_DATA.device_request.w_index >> 8;
	datalen = CONTROL_DATA.device_request.w_length;
	bRequest = CONTROL_DATA.device_request.b_request;

	pData = &gUvacClsReqDataBuf[0];
	DbgMsg_UVC(("%s:req=0x%x,ctrl=%d,intr=%d,uid=%d,len=%d++\r\n", __func__, bRequest, ControlCode, nInterface, Unit_ID, datalen));
	if (CONTROL_DATA.device_request.b_request == USB_CLASS_REQUEST_USBSTOR_RESET) {
		if ((CONTROL_DATA.device_request.w_index == 0)  &&
			(CONTROL_DATA.device_request.w_length == 0) &&
			(CONTROL_DATA.device_request.w_value == 0)  &&
			!(CONTROL_DATA.device_request.bm_request_type & USB_ENDPOINT_DIRECTION_MASK)) {
			usb_setEP0Done();
		} else {
			usb_setEP0Stall();
		}
	}

	if (CONTROL_DATA.device_request.bm_request_type & 0x2 && nInterface == U2UVAC_USB_RX_EP[0]) {
		//Recipient is endpoint
		DbgMsg_UVCIO(("^GEP:EP=0x%x,Req=0x%x,0x%x,unit=0x%x,ctrl=0x%x\r\n", nInterface, bRequest, pData, Unit_ID, ControlCode));
		if (nInterface == U2UVAC_USB_RX_EP[0]) {

			if (ControlCode == SAMPLING_FREQ_CONTROL) {
				UVC_ACO_EP_Freq_Ctrl(bRequest, pData);
			} else {
				DBG_ERR("EP ControlCode = %d\r\n", ControlCode);
				_ret_usb_stall();
			}
		} else {
			DBG_ERR("EP = %d\r\n", Unit_ID);
			_ret_usb_stall();
		}
	}
	else if (gU2UvacWinIntrfEnable && (gU2UvacIntfIdx_WinUsb == nInterface)) {
		DbgMsg_UVC(("^RWinUSB:req=%d,ctrl=%d,intr=%d,uid=%d,len=%d\r\n", bRequest, ControlCode, nInterface, Unit_ID, datalen));
		//callback
		U2UVC_WinUSB_ClassReq(ControlCode, bRequest, pData);
	}
#if USB_2ND_CDC
	else if (gU2UvacCdcEnabled && ((gU2UvacIntfIdx_CDC_COMM[CDC_COM_1ST] == nInterface) || (gU2UvacIntfIdx_CDC_DATA[CDC_COM_1ST] == nInterface) ||
								   (gU2UvacIntfIdx_CDC_COMM[CDC_COM_2ND] == nInterface) || (gU2UvacIntfIdx_CDC_COMM[CDC_COM_2ND] == nInterface))) {
#else
	else if (gU2UvacCdcEnabled && ((gU2UvacIntfIdx_CDC_COMM[CDC_COM_1ST] == nInterface) || (gU2UvacIntfIdx_CDC_DATA[CDC_COM_1ST] == nInterface))) {
#endif
		UVC_CDC_ClassReq(nInterface, bRequest);
	} else if (g_dfu_info.en && gUvacIntfIdx_DFU == nInterface) {
		UVC_DFU_ClassReq(bRequest, pData);
	} else if (g_hid_info.en && gUvacIntfIdx_HID == nInterface) {
		UVC_HID_ClassReq(bRequest, pData);
	} else if (((nInterface & 0x7F) == gU2UvacIntfIdx_VC[UVAC_VID_DEV_CNT_1]) || ((nInterface & 0x7F) == gU2UvacIntfIdx_VC[UVAC_VID_DEV_CNT_2])) {
		UVC_TERMINAL_ID UnitID = Unit_ID;
		if (UnitID == UVC_TERMINAL_ID_NULL) {
			if (ControlCode == VC_VIDEO_POWER_MODE_CONTROL) {
				DBG_ERR("UVC:[VC_VIDEO_POWER_MODE_CONTROL] Not imp.\r\n");

			} else if (ControlCode == VC_REQUEST_ERROR_CODE_CONTROL) {
				UVC_VC_ErrorCtrl(bRequest, pData);
			}
		}
		// Camera Terminal Requests
		else if (UnitID == UVC_TERMINAL_ID_ITT) {
			UVC_Unit_Cmd(g_fpUvcCT_CB, ControlCode, bRequest, pData);
		}
		// PU
		else if (UnitID == UVC_TERMINAL_ID_PROC_UNIT) {
			UVC_Unit_Cmd(g_fpUvcPU_CB, ControlCode, bRequest, pData);
		}
		// Selector Unit Requests
		else if (UnitID == UVC_TERMINAL_ID_SEL_UNIT) {
			if (ControlCode == SU_INPUT_SELECT_CONTROL) {
				DbgMsg_UVCIO(("UVC:[SU_INPUT_SELECT_CONTROL] \r\n"));
				UVC_SU_SelectCtrl(bRequest, pData);
			}
		} else if (UnitID == eu_unit_id[0]) {
			DbgMsg_UVCIO(("^BUVC-EU:ctl=%d, req=%d, pData=0x%x\r\n", ControlCode, bRequest, pData));
			if (g_fpUvcXU_CB) {
				UVC_Unit_Cmd(g_fpUvcXU_CB, ControlCode, bRequest, pData);
			} else if (eu_desc[0].eu_cb) {
				UVC_Unit_Cmd(eu_desc[0].eu_cb, ControlCode, bRequest, pData);
			}else {
				U2UVC_XU_VendorCmd(ControlCode, bRequest, pData);
			}
		} else if (UnitID == eu_unit_id[1]) {
			DbgMsg_UVCIO(("^BUVC-EU:ctl=%d, req=%d, pData=0x%x\r\n", ControlCode, bRequest, pData));
			if (eu_desc[1].eu_cb) {
				UVC_Unit_Cmd(eu_desc[1].eu_cb, ControlCode, bRequest, pData);
			}else {
				U2UVC_XU_VendorCmd(ControlCode, bRequest, pData);
			}
		} else {
			DBG_ERR("UVC: Unsupported CONTROLs, Uid=%d \r\n", UnitID);
			_ret_usb_stall();
		}
	}
	//Video Streaming I/F requests
	else if ((gU2UvacIntfIdx_VS[UVAC_VID_DEV_CNT_1] == (nInterface & 0x7F)) || (gU2UvacIntfIdx_VS[UVAC_VID_DEV_CNT_2] == (nInterface & 0x7F))) {
		UVC_TERMINAL_ID UnitID = Unit_ID;
		if (gU2UvacIntfIdx_VS[UVAC_VID_DEV_CNT_1] == (nInterface & 0x7F)) {
			thisDevIdx = UVAC_VID_DEV_CNT_1;
		} else {
			thisDevIdx = UVAC_VID_DEV_CNT_2;
		}
		pProbeCommit = &g_ProbeCommit[thisDevIdx];
		DbgMsg_UVC(("^BUVC:intr=0x%x,0x%x,0x%x,unit=0x%x,ctrl=0x%x,thisDevIdx=%d,pProbeCommit=0x%x\r\n", nInterface, bRequest, pData, UnitID, ControlCode, thisDevIdx, pProbeCommit));

		// Interface Requests
		if (UnitID == UVC_TERMINAL_ID_NULL) {
			if (ControlCode == VS_PROBE_CONTROL) {
				UVC_VS_ProbeCtrl(bRequest, pData, pProbeCommit, thisDevIdx);
			} else if (ControlCode == VS_COMMIT_CONTROL) {
				UVC_VS_CommitCtrl(bRequest, pData, pProbeCommit, thisDevIdx);
			} else if (ControlCode == VS_STILL_PROBE_CONTROL) {
				DbgMsg_UVCIO(("^BVS_STILL_PROBE_CONTROL:intr=0x%x,0x%x,0x%x,unit=0x%x,ctrl=0x%x\r\n", nInterface, bRequest, pData, UnitID, ControlCode));
				UVC_VS_Still_ProbeCtrl(bRequest, pData, nInterface);
			} else if (ControlCode == VS_STILL_COMMIT_CONTROL) {
				DbgMsg_UVCIO(("^BVS_STILL_COMMIT_CONTROL:intr=0x%x,0x%x,0x%x,unit=0x%x,ctrl=0x%x\r\n", nInterface, bRequest, pData, UnitID, ControlCode));
				UVC_VS_Still_CommitCtrl(bRequest, pData, nInterface);
			} else if (ControlCode == VS_STILL_IMAGE_TRIGGER_CONTROL) {
				DbgMsg_UVCIO(("^BVS_STILL_IMAGE_TRIGGER_CONTROL:intr=0x%x,0x%x,0x%x,unit=0x%x,ctrl=0x%x\r\n", nInterface, bRequest, pData, UnitID, ControlCode));
				UVC_VS_Still_ImgTrigCtrl(bRequest, pData, nInterface);
			} else {
				DBG_ERR("^RUVC: Unsupported CONTROLs \r\n");
				_ret_usb_stall();

			}
		}
	}
	//Audio Control I/F requests
	else if ((gU2UvacIntfIdx_AC[0] == (nInterface & 0x7F)) || (gU2UvacIntfIdx_AC[1] == (nInterface & 0x7F)) || (gU2UvacIntfIdx_AS[0] == (nInterface & 0x7F)) || (gU2UvacIntfIdx_AS[1] == (nInterface & 0x7F))) {
		UAC_TERMINAL_ID UnitID = CONTROL_DATA.device_request.w_index >> 8;;
		DbgMsg_UVCIO(("^GUAC:intr=0x%x,0x%x,0x%x,unit=0x%x,ctrl=0x%x\r\n", nInterface, bRequest, pData, UnitID, ControlCode));


		if (UnitID == UAC_TERMINAL_ID_NULL) { //endpoint control
			if (ControlCode == VS_PROBE_CONTROL) {
				UVC_AC_ProbeCtrl(bRequest, pData);
			} else if (ControlCode == VS_COMMIT_CONTROL) {
				UVC_AC_CommitCtrl(bRequest, pData);
			} else {
				DBG_ERR("^RUAC: Unsupported CONTROLs \r\n");
				_ret_usb_stall();
			}
		} else if (UnitID == UAC_TERMINAL_ID_FU) {
			if (ControlCode == UAC_FU_CONTROL_MUTE) {
				UVC_AC_MuteCtrl(bRequest, pData);
			} else if (ControlCode == UAC_FU_CONTROL_VOLUME) {
				UVC_AC_VolCtrl(bRequest, pData);
			} else {
				DBG_ERR("^RUAC: Unsupported CONTROLs \r\n");
				_ret_usb_stall();
			}
		}
	} //Audio Control I/F requests
	else if ((gU2UvacIntfIdx_AC[2] == (nInterface & 0x7F)) || (gU2UvacIntfIdx_AS[2] == (nInterface & 0x7F))) {
		UAC_TERMINAL_ID UnitID = CONTROL_DATA.device_request.w_index >> 8;;
		DBG_IND("^GUAC:intr=0x%x,0x%x,0x%x,unit=0x%x,ctrl=0x%x\r\n", nInterface, bRequest, pData, UnitID, ControlCode);


		if (UnitID == UAC_TERMINAL_ID_NULL) { //endpoint control
			if (ControlCode == VS_PROBE_CONTROL) {
				UVC_ACO_ProbeCtrl(bRequest, pData);
			} else if (ControlCode == VS_COMMIT_CONTROL) {
				UVC_ACO_CommitCtrl(bRequest, pData);
			} else {
				DBG_ERR("^RUAC: Unsupported CONTROLs \r\n");
				_ret_usb_stall();
			}
		} else if (UnitID == UAC_TERMINAL_ID_FU) {
			if (ControlCode == UAC_FU_CONTROL_MUTE) {
				UVC_ACO_MuteCtrl(bRequest, pData);
			} else if (ControlCode == UAC_FU_CONTROL_VOLUME) {
				UVC_ACO_VolCtrl(bRequest, pData);
			} else {
				DBG_ERR("^RUAC: Unsupported CONTROLs \r\n");
				_ret_usb_stall();
			}
		}
	} else {

		if (g_msdc_info.en && g_u2uvac_msdc_class_req_cb) {
			g_u2uvac_msdc_class_req_cb();
		} else {
			DBG_ERR("UVC:Cannot handle this interface] %x\r\n", nInterface);
			DBG_ERR("req=0x%x,ctrl=%d,intr=%d,uid=%d,len=%d\r\n", bRequest, ControlCode, nInterface, Unit_ID, datalen);

			_ret_usb_stall();
		}
	}
	DbgMsg_UVC(("%s:req=0x%x,ctrl=%d,intr=%d,uid=%d,len=%d--\r\n", __func__, bRequest, ControlCode, nInterface, Unit_ID, datalen));
}

void U2UVC_XU_VendorCmd(UINT32 ControlCode, UINT8 CS, UINT8 *pData)
{
	UINT8 vendCBRet;
	DbgMsg_UVCIO(("VendCmd: ctrl=%d, cs=0x%x, pData=0x%x\r\n", ControlCode, CS, pData));
	//DBG_DUMP("VendCmd: ctrl=%d, cs=0x%x, pData=0x%x\r\n", ControlCode, CS, pData);

	//ControlCode: 1, 2, ..., 8
	if ((UVAC_EU_VENDCMD_CNT > ControlCode) && (gU2UvacEUVendCmdCB[ControlCode])) {
		if (SET_CUR == CS) {
			DbgMsg_UVCIO(("^BUVC:[EU --SET_CUR]\r\n"));
			gUvacVendDataLen = USB_UVAC_CLS_DATABUF_LEN;
			memset(pData, 0, USB_UVAC_CLS_DATABUF_LEN);
			if (_readEP0(pData, &gUvacVendDataLen) != E_OK) {
				DBG_ERR("Read failed!\r\n");
				_ret_usb_stall();
				return;
			}
			usb_setEP0Done();
		}
		vendCBRet = gU2UvacEUVendCmdCB[ControlCode](ControlCode, CS, pData, &gUvacVendDataLen);
	} else {
		DBG_IND("gU2UvacEUVendCmdCB[%d] NULL, CS=%d, pData=0x%x\r\n", ControlCode, CS, pData);
		vendCBRet = FALSE;
	}
	if (TRUE == vendCBRet) {
		DbgMsg_UVCIO(("VendCmd Ok, retLen=%d,ctrl=%d, cs=0x%x, pData=0x%x\r\n", gUvacVendDataLen, ControlCode, CS, pData));
		DbgMsg_UVCIO(("  ==> *pData=0x%x\r\n", *pData));
		//DBG_DUMP("VendCmd Ok, retLen=%d,ctrl=%d, cs=0x%x, pData=0x%x", gUvacVendDataLen, ControlCode, CS, pData);
		//DBG_DUMP("==> *pData=0x%x\r\n", *pData);
		//gUvacVendDataLen = (gUvacVendDataLen > USB_UVAC_CLS_DATABUF_LEN) ? USB_UVAC_CLS_DATABUF_LEN : gUvacVendDataLen;
		if (SET_CUR != CS) {
			if (gUvacVendDataLen > USB_UVAC_CLS_DATABUF_LEN) {
				UINT8 *p_addr = (UINT8 *)*(UINT32 *)pData;
				ret_setup_data(p_addr, gUvacVendDataLen);
			} else {
				ret_setup_data(pData, gUvacVendDataLen);
			}
		}
	} else {
		DBG_IND("VendCmd Fail, retLen=%d,ctrl=%d, cs=0x%x, pData=0x%x\r\n", gUvacVendDataLen, ControlCode, CS, pData);
		_ret_usb_stall();
	}
}

void UvacStdRequestHandler(UINT32 uiRequest)
{
	UINT8 nInterface;
	UINT8 desc_type;
	UINT16 desc_len;

	if (FALSE == g_hid_info.en) {
		_ret_usb_stall();
		return;
	}

	desc_type = CONTROL_DATA.device_request.w_value >> 8;
	nInterface = CONTROL_DATA.device_request.w_index & 0xFF;


	DbgMsg_UVC(("++%s:req=0x%x, desc_type=0x%X, intr=%d, len=%d\r\n", __func__,
													CONTROL_DATA.device_request.b_request,
													desc_type,
													nInterface,
													CONTROL_DATA.device_request.w_length));
	//get reporter descriptor
	if (desc_type == g_hid_info.hid_desc.bDescriptorType && nInterface == gUvacIntfIdx_HID) {
		desc_len = g_hid_info.hid_desc.wDescriptorLength;
		if (g_hid_info.p_report_desc) {
			if (desc_len) {
				ret_setup_data(g_hid_info.p_report_desc, desc_len);
				DbgMsg_UVC(("--%s:req=0x%x, desc_type=0x%X, intr=%d, len=%d\r\n", __func__,
													CONTROL_DATA.device_request.b_request,
													desc_type,
													nInterface,
													desc_len));
				return;
			}
		}
	} else if (desc_type == g_hid_info.hid_desc.bHidDescType && nInterface == gUvacIntfIdx_HID) {
	//get hid descriptor
		desc_len = g_hid_info.hid_desc.bLength;
		if (desc_len) {
			ret_setup_data((UINT8 *)&g_hid_info.hid_desc, desc_len);
			DbgMsg_UVC(("--%s:req=0x%x, desc_type=0x%X, intr=%d, len=%d\r\n", __func__,
													CONTROL_DATA.device_request.b_request,
													desc_type,
													nInterface,
													desc_len));
			return;
		}
	}
	_ret_usb_stall();
}

