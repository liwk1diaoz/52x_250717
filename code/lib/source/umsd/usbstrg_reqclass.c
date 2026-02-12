/*
    USB MSDC class-specific Request handler

    USB Mass-Storage-Device-Class Task class-specific Request handler.

    @file       UsbStrgReqClass.c
    @ingroup    mILibUsbMSDC
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include    "usb2dev.h"
#include    "usbstrg_reqclass.h"
#include    "usbstrg_task.h"
//#include    "cache.h"

extern BOOL g_bU2MsdcCompositeDevice;
extern UINT32 guiU2MsdcInterfaceIdx;

void U2Msdc_ClassRequestHandler(void)
{
	msdc_debug(("U2Msdc_ClassRequestHandler Req=%d \r\n", usb2dev_control_data.device_request.b_request));

	if (usb2dev_control_data.device_request.b_request == USTRG_CSR_RESET_REQUEST) {

		UINT32 idx = (g_bU2MsdcCompositeDevice)? guiU2MsdcInterfaceIdx : USTRG_CSR_RESET_INEDEX;

		if ((usb2dev_control_data.device_request.w_index == idx)  &&
			(usb2dev_control_data.device_request.w_length == USTRG_CSR_RESET_LENGTH) &&
			(usb2dev_control_data.device_request.w_value == USTRG_CSR_RESET_VALUE)  &&
			(usb2dev_control_data.device_request.bm_request_type == USTRG_CSR_RESET_REQTYPE)) {
			msdc_debug(("MSDC Set FLGMSDC_MSDCRESET\r\n"));
			U2Msdc_SetFlag(FLGMSDC_MSDCRESET);

			usb2dev_set_ep0_done();
		} else {
			DBG_WRN("Stall EP0\r\n");
			//stall EP0
			usb2dev_set_ep_stall(USB_EP0);
			//usb2dev_set_ep0_done();
		}
	} else if (usb2dev_control_data.device_request.b_request == USTRG_CSR_MAXLUN_REQUEST) {

		UINT32 idx = (g_bU2MsdcCompositeDevice)? guiU2MsdcInterfaceIdx : USTRG_CSR_MAXLUN_INEDEX;

		if ((usb2dev_control_data.device_request.w_value == USTRG_CSR_MAXLUN_VALUE) &&
			(usb2dev_control_data.device_request.w_index == idx) &&
			(usb2dev_control_data.device_request.w_length == USTRG_CSR_MAXLUN_LENGTH) &&
			(usb2dev_control_data.device_request.bm_request_type == USTRG_CSR_MAXLUN_REQTYPE)) {
			UINT32 len, MsdcTemp;

			MsdcTemp = g_uiU2TotalLUNs - 1;
			len = 1;

			msdc_debug(("MSDC Lun Number = %d\r\n", (int)MsdcTemp));

			U2Msdc_SetIdle();
			usb2dev_write_endpoint(USB_EP0, (UINT8 *)&MsdcTemp, &len);
			U2Msdc_ClearIdle();

			usb2dev_set_ep0_done();
		} else {
			DBG_WRN("Stall EP0\r\n");

			//stall EP0
			usb2dev_set_ep_stall(USB_EP0);
			//usb2dev_set_ep0_done();
		}
	} else {
		DBG_WRN("Stall EP0\r\n");

		//stall EP0
		usb2dev_set_ep_stall(USB_EP0);
		//usb2dev_set_ep0_done();
	}
}



