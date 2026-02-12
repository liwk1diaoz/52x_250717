#include    "U2UvacReqVendor.h"
#include    "U2UvacVideoTsk.h"
#include    "U2UvacDbg.h"
#include    "U2UvacID.h"
#define UVC_VENDREQ_BUF_SIZE                64
static UINT8 gVendorBuf[UVC_VENDREQ_BUF_SIZE];

UVAC_VENDOR_REQ_CB g_fpU2UvacVendorReqCB = NULL;
UVAC_VENDOR_REQ_CB g_fpU2UvacVendorReqIQCB = NULL;

/*---------------------------------------------------------------------------
 Function   : Read data from host by control pipe.
----------------------------------------------------------------------------*/
static void UvcVendor_ReadDataFromHost(void)
{
	int iLoop, i;
	UINT32 *pBuf;
	UINT32 uiLen, nReadBytes;
	FLGPTN uiFlag;

	uiLen = CONTROL_DATA.device_request.w_length;
	DBG_DUMP("Vendor  len:%d\r\n", uiLen);
	pBuf = (UINT32 *)&gVendorBuf;
	while (1) {
#if UVAC_PATCH_CXOUT
		usb_unmaskEPINT(USB_EP0);
#endif

		wai_flg(&uiFlag, FLG_ID_U2UVAC, FLGUVAC_OUT0, TWF_CLR);
		DBG_IND("-- uiFlag=0x%x\r\n", uiFlag);
		if (uiLen < EP0_PACKET_SIZE) {
			nReadBytes = uiLen;
		} else {
			nReadBytes = EP0_PACKET_SIZE;
		}

		uiLen -= nReadBytes;
		iLoop = nReadBytes >> 2;

		if (E_OK == usb_readEndpoint(USB_EP0, (UINT8 *)&pBuf, &nReadBytes)) {
			for (i = 0; i < iLoop ; i++) {
				pBuf++;
			}
			if (nReadBytes % 4) {
				pBuf++;
			}
		} else {
			DBG_ERR("read error\r\n");
			break;
		}
		if (uiLen == 0) {
			break;
		}
	}
#if UVAC_PATCH_CXOUT
	usb_maskEPINT(USB_EP0);
#endif
}

void U2UvacVendorRequestHandler(UINT32 uiRequest)
{
	UINT32 tmpSize = 0;
	UINT8 reqIdx = 0;
	UVAC_VENDOR_PARAM Param = {0};
	DBG_IND(" req=%d, cb=0x%x, iqcb=0x%x\r\n", uiRequest, g_fpU2UvacVendorReqCB, g_fpU2UvacVendorReqIQCB);

	reqIdx = Param.uiReguest = CONTROL_DATA.device_request.b_request;
	Param.uiValue = CONTROL_DATA.device_request.w_value;
	Param.uiIndex = CONTROL_DATA.device_request.w_index;//interface or endpoint
	Param.uiDataSize = CONTROL_DATA.device_request.w_length;
	if (CONTROL_DATA.device_request.bm_request_type & USB_ENDPOINT_DIRECTION_MASK) {
		Param.bHostToDevice = FALSE;
		if (g_fpU2UvacVendorReqIQCB && ((reqIdx & UVAC_VENDREQ_IQ_MASK) >= UVAC_VENDREQ_IQ_MASK)) {
			g_fpU2UvacVendorReqIQCB(&Param);
		} else if (g_fpU2UvacVendorReqCB && ((reqIdx & UVAC_VENDREQ_CUST_MASK) >= UVAC_VENDREQ_CUST_MASK)) {
			g_fpU2UvacVendorReqCB(&Param);
		} else {
			DBG_ERR("Unsupport Vendor Request:0x%x, 0x%x\r\n", reqIdx, uiRequest);
			usb_setEP0Stall();
			usb_setEP0Done();
		}
		if (Param.uiDataSize > EP0_PACKET_SIZE) {
			Param.uiDataSize = EP0_PACKET_SIZE;
			DBG_ERR("Vendor data size should NOT exceed %d bytes!\r\n", EP0_PACKET_SIZE);
		}
		if (0 == Param.uiDataSize) {
			DBG_ERR("Data Size Zero !!\r\n");
			usb_setEP0Stall();
		} else {
			if (Param.uiDataSize > UVC_VENDREQ_BUF_SIZE) {
				tmpSize = UVC_VENDREQ_BUF_SIZE;
				DBG_ERR("Param.uiDataSize=%d > %d\r\n", Param.uiDataSize, UVC_VENDREQ_BUF_SIZE);
			} else {
				tmpSize = Param.uiDataSize;
			}
			if (Param.uiDataAddr) {
				memcpy((void *)&gVendorBuf[0], (void *)Param.uiDataAddr, tmpSize);
			}
			if (Param.uiDataSize > CONTROL_DATA.device_request.w_length) {
				DBG_ERR("Param.uiDataSize=%d > Host=%d\r\n", Param.uiDataSize, CONTROL_DATA.device_request.w_length);
				CONTROL_DATA.w_length  = CONTROL_DATA.device_request.w_length;
			} else {
				DBG_ERR("Param.uiDataSize=%d <= Host=%d\r\n", Param.uiDataSize, CONTROL_DATA.device_request.w_length);
				CONTROL_DATA.w_length  = Param.uiDataSize;
			}
			CONTROL_DATA.p_data    = (UINT8 *)&gVendorBuf[0];
			DBG_DUMP("Data Size=%d,%d,%d\r\n", CONTROL_DATA.w_length, Param.uiDataSize, CONTROL_DATA.device_request.w_length);
			usb_RetSetupData();
		}
		usb_setEP0Done();
	} else {
		Param.bHostToDevice = TRUE;
		if (0 == Param.uiDataSize) {
			DBG_ERR("Vendor data size zero\r\n");
			//usb_setEP0Stall();
		} else {
			UvcVendor_ReadDataFromHost();
			Param.uiDataAddr = (UINT32)&gVendorBuf[0];
		}
		if (g_fpU2UvacVendorReqIQCB && ((reqIdx & UVAC_VENDREQ_IQ_MASK) >= UVAC_VENDREQ_IQ_MASK)) {
			g_fpU2UvacVendorReqIQCB(&Param);
		} else if (g_fpU2UvacVendorReqCB && ((reqIdx & UVAC_VENDREQ_CUST_MASK) >= UVAC_VENDREQ_CUST_MASK)) {
			g_fpU2UvacVendorReqCB(&Param);
		} else {
			DBG_ERR("Unsupport Vendor Request:%d\r\n", reqIdx);
			usb_setEP0Stall();
		}
		usb_setEP0Done();
	}
	DBG_DUMP("HostToDevice=%d, bRequest=%d, wValue=%d, wIndex=%d, wLength=%d\r\n", Param.bHostToDevice, Param.uiReguest, Param.uiValue, Param.uiIndex, Param.uiDataSize);
#if (_UVC_DBG_LVL_ >= _UVC_DBG_CHK_)
	UINT32 tmpV = 0;
	DBG_DUMP("VenReq=0x%x/0x%x,dataSize=%d,cb=0x%x,iqcb=0x%x\r\n", reqIdx, uiRequest, Param.uiDataSize, g_fpU2UvacVendorReqCB, g_fpU2UvacVendorReqIQCB);
	while (tmpV < Param.uiDataSize) {
		DBG_DUMP("0x%x ", gVendorBuf[tmpV]);
		tmpV ++;
	}
#endif
	return;
}

