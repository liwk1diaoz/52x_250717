/*
    @file       usb_ClsDrvTsk.c
    @ingroup    mIUSB

    @brief      USB class drvier file for internal used.
                Including two major task : USBCtlStartTsk & USBSetupHalTsk
                They will handle the device source group and setup interrupts
                The driver support MSDC/PCC/SIDC...

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.

*/

#include <stdio.h>
#include <string.h>
#include "interrupt.h"
#include "usb2dev.h"
#include "usb2dev_reg.h"
#include "usb2dev_int.h"

void  USBCtlStartTsk(void *pvParameters);
void  USBSetupHalTsk(void *pvParameters);

USB_GENERIC_CB  gfpVendorReqHdl     = NULL;     // vendor request call back used by USB_CALLBACK_CX_VENDOR_REQUEST
FP              gfpClassRequest;
USB_GENERIC_CB  gfpStdUnknownReq	= NULL;
UINT8           gUSBIsrCause        = 0xFF;


static void usb_devINTGrp0(void);
static void usb_devINTGrp1(void);
static void usb_devINTGrp2(void);

static void usb_dispatchOutEpEvent(USB_FIFO_NUM FIFOn);
static void usb_dispatchInEpEvent(USB_FIFO_NUM FIFOn);

/**
    @addtogroup mIDrvUSB_Device
*/
//@{


/*
    USB control start task

    Processing of USB device interrupt and call related group handler

    @return void
*/
void USBCtlStartTsk(void *pvParameters)
{
	FLGPTN uiFlag;

	//coverity[no_escape]
	while (1) {
		set_flg(FLG_ID_USBTSK, FLGUSB_CTLTSK_IDLE);
         PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, FLG_ID_USBTSK, FLGUSB_SIG, TWF_CLR);
         PROFILE_TASK_BUSY();
		clr_flg(FLG_ID_USBTSK, FLGUSB_CTLTSK_IDLE);

		USB_PutString1(" gUSBCause = %x !! \r\n", (int)gUSBCause);

		if (gUSBCause != 0) {
			// Must Handle Grp2 first becuase the suspend sts in Grp2.
			if (gUSBCause & BIT2) {
				USB_PutString(" Interrupt Source Group 2 !!  \r\n");
				usb_devINTGrp2();
			}

			if (gUSBCause & BIT0) {
				USB_PutString(" Interrupt Source Group 0 !!  \r\n");
				usb_devINTGrp0();
			}

			if (gUSBCause & BIT1) {
				USB_PutString(" Interrupt Source Group 1 !!  \r\n");
				usb_devINTGrp1();
			}

			USB_PutString(" \r\n");
			gUSBCause = 0;

		} else {
			DBG_ERR(" => USB interrupt status = 0 !\r\n");
		}

		request_irq(INT_ID_USB, 0, IRQF_TRIGGER_HIGH, 0, 0);

	} // end of while(1)
}


/*
    USB setup HAL task

    Processing of USB setup transaction interrupt.
    Handle the standard request, vendor request and class request.

    @return void
*/
void USBSetupHalTsk(void *pvParameters)
{
	T_USB_DEVCXCFGFIFOSTATUS_REG    devCXCFG;
	FLGPTN                          uiFlag;
	UINT8                           type, req;

	bUSBflag_configuration = 0;
	clr_flg(FLG_ID_USBTSK, FLGUSB_SETUP);

	//coverity[no_escape]
	while (1) {
		set_flg(FLG_ID_USBTSK, FLGUSB_SETUPTSK_IDLE);
         PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, FLG_ID_USBTSK, FLGUSB_SETUP, TWF_CLR);
         PROFILE_TASK_BUSY();
		clr_flg(FLG_ID_USBTSK, FLGUSB_SETUPTSK_IDLE);

		type = usb2dev_control_data.device_request.bm_request_type & USB_REQUEST_TYPE_MASK;
		req  = usb2dev_control_data.device_request.b_request      & USB_REQUEST_MASK;

		USB_PutString1("usb setup type = 0x%x\r\n", (int)type);
		USB_PutString1("usb req = 0x%x\r\n", (int)req);

		if (type == USB_STANDARD_REQUEST) {
			// Check for Audio start/stop
			(*StandardDeviceRequest[req])();
		} else {
			if ((type == USB_VENDOR_REQUEST) && (gfpVendorReqHdl != NULL)) {
				gfpVendorReqHdl(usb2dev_control_data.device_request.b_request);
			} else {
				if ((type == USB_CLASS_REQUEST) && (gfpClassRequest)) {
					gfpClassRequest();
				} else {
					// set EP0 Stall
					gUSBCxFinishAction = USB_ACT_STALL;
				}
			}
		}

		if (gUSBCxFinishAction == USB_ACT_STALL) {
			devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
			devCXCFG.bit.CX_STL = 1; // bit[2]
			USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);
		} else if (gUSBCxFinishAction == USB_ACT_DONE) {
			devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
			devCXCFG.bit.CX_DONE = 1; // bit[0]
			USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);
		}

		// Clear Action
		gUSBCxFinishAction = USB_ACT_IDLE;
	}
}

#if 1
/*
    Processing of USB device interrupt source group 0

    @return void
*/
static void usb_devINTGrp0(void)
{
	T_USB_DEVINTGROUP0_REG          devIntGrp0;
	T_USB_DEVINTMASKGROUP0_REG      devIntMskGrp0;
	T_USB_DEVCXCFGFIFOSTATUS_REG    devCXCFG;

	devIntGrp0.reg = USB_GETREG(USB_DEVINTGROUP0_REG_OFS);
	devIntMskGrp0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
	gUSBCauseGroup[0] = devIntGrp0.reg & ~devIntMskGrp0.reg;

	if (gUSBCauseGroup[0] & BIT0) {
		USB_PutString(" CX_SETUP_INT ! \r\n");
		gUSBIsrCause = 2; //usb_ISR_SETUP();
		(*USBINTVector[gUSBIsrCause])();
	} else if (gUSBCauseGroup[0] & BIT3) {
		USB_PutString(" CX_COMEND_INT ! \r\n");
		// set CX_DONE
		devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
		devCXCFG.bit.CX_DONE = 1; // bit[0]
		USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);
	}

	if (gUSBCauseGroup[0] & BIT1) {
		USB_PutString(" CX_IN_INT ! \r\n");
		gUSBIsrCause = 0; //usb_ISR_IN0();
		(*USBINTVector[gUSBIsrCause])();
	}
	if (gUSBCauseGroup[0] & BIT2) {
		USB_PutString(" CX_OUT_INT ! \r\n");
		gUSBIsrCause = 1; //usb_ISR_OUT0();
		(*USBINTVector[gUSBIsrCause])();
	}
	if (gUSBCauseGroup[0] & BIT4) {
		USB_PutString(" CX_COMFAIL_INT ! \r\n");
		devIntGrp0.reg = USB_GETREG(USB_DEVINTGROUP0_REG_OFS);
		devIntGrp0.bit.CX_COMFAIL_INT = 0;
		USB_SETREG(USB_DEVINTGROUP0_REG_OFS, devIntGrp0.reg);
		//set EP0 stall
		devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
		devCXCFG.bit.CX_STL = 1; // bit[2]
		USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);
	}
	if (gUSBCauseGroup[0] & BIT5) {
		USB_PutString(" CX_COMABT_INT ! \r\n");
		devIntGrp0.reg = USB_GETREG(USB_DEVINTGROUP0_REG_OFS);
		devIntGrp0.bit.CX_COMABORT_INT = 0;
		USB_SETREG(USB_DEVINTGROUP0_REG_OFS, devIntGrp0.reg);
	}

	gUSBIsrCause = 0xff;
}

/*
    Processing of USB device interrupt source group 1

    @return void
*/
static void usb_devINTGrp1(void)
{
	T_USB_DEVINTGROUP1_REG devIntGrp1;
	T_USB_DEVINTMASKGROUP1_REG devIntMskGrp1;

	devIntGrp1.reg = USB_GETREG(USB_DEVINTGROUP1_REG_OFS);
	devIntMskGrp1.reg = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
	gUSBCauseGroup[1] = devIntGrp1.reg & ~devIntMskGrp1.reg;


	//
	//  OUT FIFO Events
	//
	if (gUSBCauseGroup[1] & (BIT0 | BIT1)) {
		USB_PutString(" F0_OUT_INT or F0_SPK_INT ! \r\n");
		USB_PutString(" BULK OUT \n");
		usb_dispatchOutEpEvent(USB_FIFO0);
	}
	if (gUSBCauseGroup[1] & (BIT2 | BIT3)) {
		USB_PutString(" F1_OUT_INT or F1_SPK_INT ! \r\n");
		usb_dispatchOutEpEvent(USB_FIFO1);
	}
	if (gUSBCauseGroup[1] & (BIT4 | BIT5)) {
		USB_PutString(" F2_OUT_INT or F2_SPK_INT ! \r\n");
		usb_dispatchOutEpEvent(USB_FIFO2);
	}
	if (gUSBCauseGroup[1] & (BIT6 | BIT7)) {
		USB_PutString(" F3_OUT_INT or F3_SPK_INT ! \r\n");
		usb_dispatchOutEpEvent(USB_FIFO3);
	}
	if (gUSBCauseGroup[1] & (BIT8 | BIT9)) {
		USB_PutString(" F4_OUT_INT or F4_SPK_INT ! \r\n");
		usb_dispatchOutEpEvent(USB_FIFO4);
	}
	if (gUSBCauseGroup[1] & (BIT10 | BIT11)) {
		USB_PutString(" F5_OUT_INT or F5_SPK_INT ! \r\n");
		usb_dispatchOutEpEvent(USB_FIFO5);
	}
	if (gUSBCauseGroup[1] & (BIT12 | BIT13)) {
		USB_PutString(" F6_OUT_INT or F6_SPK_INT ! \r\n");
		usb_dispatchOutEpEvent(USB_FIFO6);
	}
	if (gUSBCauseGroup[1] & (BIT14 | BIT15)) {
		USB_PutString(" F7_OUT_INT or F7_SPK_INT ! \r\n");
		usb_dispatchOutEpEvent(USB_FIFO7);
	}


	//
	//  IN FIFO Events
	//
	if (gUSBCauseGroup[1] & BIT16) {
		USB_PutString(" F0_IN_INT ! \r\n");
		USB_PutString(" BULK IN \n");
		usb_dispatchInEpEvent(USB_FIFO0);
	}
	if (gUSBCauseGroup[1] & BIT17) {
		USB_PutString(" F1_IN_INT ! \r\n");
		usb_dispatchInEpEvent(USB_FIFO1);
	}
	if (gUSBCauseGroup[1] & BIT18) {
		USB_PutString(" F2_IN_INT ! \r\n");
		USB_PutString(" INTERRUPT IN \n");
		usb_dispatchInEpEvent(USB_FIFO2);
	}
	if (gUSBCauseGroup[1] & BIT19) {
		USB_PutString(" F3_IN_INT ! \r\n");
		usb_dispatchInEpEvent(USB_FIFO3);
	}
	if (gUSBCauseGroup[1] & BIT20) {
		USB_PutString(" F4_IN_INT ! \r\n");
		usb_dispatchInEpEvent(USB_FIFO4);
	}
	if (gUSBCauseGroup[1] & BIT21) {
		USB_PutString(" F5_IN_INT ! \r\n");
		usb_dispatchInEpEvent(USB_FIFO5);
	}
	if (gUSBCauseGroup[1] & BIT22) {
		USB_PutString(" F6_IN_INT ! \r\n");
		usb_dispatchInEpEvent(USB_FIFO6);
	}
	if (gUSBCauseGroup[1] & BIT23) {
		USB_PutString(" F7_IN_INT ! \r\n");
		usb_dispatchInEpEvent(USB_FIFO7);
	}


	gUSBIsrCause = 0xFF;
}

/*
    Processing of USB device interrupt source group 2

    @return void
*/
static void usb_devINTGrp2(void)
{
	T_USB_DEVINTGROUP2_REG devIntGrp2;
	T_USB_DEVINTMASKGROUP2_REG devIntMskGrp2;
	REGVALUE devTX0Byte;
	//REGVALUE devRX0Byte;

	devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);
	devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
	gUSBCauseGroup[2] = devIntGrp2.reg & ~devIntMskGrp2.reg;

	USB_SETREG(USB_DEVINTGROUP2_REG_OFS, gUSBCauseGroup[2]);

	if (gUSBCauseGroup[2] & BIT0) {
		USB_PutString(" USBRST_INT ! \r\n");

		set_flg(FLG_ID_USBTSK, FLGUSB_DMADONEALL);

		gUSBIsrCause = 6; //usb_ISR_RST();
		(*USBINTVector[gUSBIsrCause])();
	}
	if (gUSBCauseGroup[2] & BIT1) {
		USB_PutString(" SUSP_INT ! \r\n");

		set_flg(FLG_ID_USBTSK, FLGUSB_DMADONEALL);

		gUSBIsrCause = 7; //usb_ISR_SUSPEND();
		(*USBINTVector[gUSBIsrCause])();
	}
	if (gUSBCauseGroup[2] & BIT2) {
		USB_PutString(" RESM_INT ! \r\n");

		set_flg(FLG_ID_USBTSK, FLGUSB_DMADONEALL);

		gUSBIsrCause = 8; //usb_ISR_RESUME();
		(*USBINTVector[gUSBIsrCause])();
	}
	if (gUSBCauseGroup[2] & BIT3) {
		USB_PutString(" ISO_SEQ_ERR_INT ! \r\n");
		// do error handle here...
	}
	if (gUSBCauseGroup[2] & BIT4) {
		USB_PutString(" ISO_SEQ_ABORT_INT ! \r\n");
		// do error handle here...
	}
	if (gUSBCauseGroup[2] & BIT5) {
		USB_PutString(" TX0BYTE_INT ! \r\n");

		// clear TX0BYTE
		devTX0Byte = USB_GETREG(USB_DEVTX0BYTE_REG_OFS);
		USB_SETREG(USB_DEVTX0BYTE_REG_OFS, 0);
		if (devTX0Byte & BIT0) {
			if (gUSBManage.fp_event_callback) {
				gUSBManage.fp_event_callback(USB_EVENT_EP1TX0BYTE);
			}
			USB_PutString(" EP1 TX0BYTE_INT ! \r\n");
		} else if (devTX0Byte & BIT2) {
			if (gUSBManage.fp_event_callback) {
				gUSBManage.fp_event_callback(USB_EVENT_EP3TX0BYTE);
			}
			USB_PutString(" EP3 TX0BYTE_INT ! \r\n");
		}
	}
	if (gUSBCauseGroup[2] & BIT6) {
		USB_PutString(" RX0BYTE_INT ! \r\n");

		// clear RX0BYTE
		//devRX0Byte = USB_GETREG(USB_DEVRX0BYTE_REG_OFS);
		USB_SETREG(USB_DEVRX0BYTE_REG_OFS, 0);
	}
	if (gUSBCauseGroup[2] & BIT7) {
		USB_PutString(" DMA_CMPLT ! \r\n");

		set_flg(FLG_ID_USBTSK, FLGUSB_DMADONE);
	}
	if (gUSBCauseGroup[2] & BIT8) {
		USB_PutString(" DMA_ERROR ! \r\n");

		set_flg(FLG_ID_USBTSK, FLGUSB_DMADONEALL);
	}
	if (gUSBCauseGroup[2] & BIT9) {
		USB_PutString(" DMA2_CMPLT ! \r\n");

		set_flg(FLG_ID_USBTSK, FLGUSB_DMA2DONE);
	}
	if (gUSBCauseGroup[2] & BIT10) {
		USB_PutString(" DMA3_CMPLT ! \r\n");

		set_flg(FLG_ID_USBTSK, FLGUSB_DMA3DONE);
	}
	if (gUSBCauseGroup[2] & BIT11) {
		USB_PutString(" DMA4_CMPLT ! \r\n");

		set_flg(FLG_ID_USBTSK, FLGUSB_DMA4DONE);
	}
	if (gUSBCauseGroup[2] & BIT12) {
		USB_PutString(" DMA5_CMPLT ! \r\n");

		set_flg(FLG_ID_USBTSK, FLGUSB_DMA5DONE);
	}
	if (gUSBCauseGroup[2] & BIT13) {
		USB_PutString(" DEV_IDLE ! \r\n");
	}
	if (gUSBCauseGroup[2] & BIT14) {
		USB_PutString(" DEV_WAKEUP_BYVBUS ! \r\n");
	}

	gUSBIsrCause = 0xff;

}
#endif


/*
    Dispatch EP event of OUT FIFO

    @param[in] FIFOn    FIFO number

    @return void
*/
static void usb_dispatchOutEpEvent(USB_FIFO_NUM FIFOn)
{
	USB_EP EP;

	EP = usb_getOutEpFromFifo(FIFOn);
	switch (EP) {
	case USB_EP1: {
			if (gUSBManage.fp_event_callback) {
				gUSBManage.fp_event_callback(USB_EVENT_EP1_RX);
			}
		}
		break;
	case USB_EP2: {
			if (gUSBManage.fp_event_callback) {
				gUSBManage.fp_event_callback(USB_EVENT_EP2_RX);
			}
		}
		break;
	case USB_EP3: {
			if (gUSBManage.fp_event_callback) {
				gUSBManage.fp_event_callback(USB_EVENT_EP3_RX);
			}
		}
		break;
	case USB_EP4: {
			if (gUSBManage.fp_event_callback) {
				gUSBManage.fp_event_callback(USB_EVENT_EP4_RX);
			}
		}
		break;

	case USB_EP5:
	case USB_EP6:
	case USB_EP7:
	case USB_EP8:
	case USB_EP9:
	case USB_EP10:
	case USB_EP11:
	case USB_EP12:
	case USB_EP13:
	case USB_EP14:
	case USB_EP15: {
			if (gUSBManage.fp_event_callback) {
				gUSBManage.fp_event_callback(USB_EVENT_EP5_RX + (EP - USB_EP5));
			}
		}
		break;
	default:
		break;
	}
}

/*
    Dispatch EP event of IN FIFO

    @param[in] FIFOn    FIFO number

    @return void
*/
static void usb_dispatchInEpEvent(USB_FIFO_NUM FIFOn)
{
	USB_EP EP;

	EP = usb_getInEpFromFifo(FIFOn);
	switch (EP) {
	case USB_EP1: {
			if (gUSBManage.fp_event_callback) {
				gUSBManage.fp_event_callback(USB_EVENT_EP1_TX);
			}
		}
		break;
	case USB_EP2: {
			if (gUSBManage.fp_event_callback) {
				gUSBManage.fp_event_callback(USB_EVENT_EP2_TX);
			}
		}
		break;
	case USB_EP3: {
			if (gUSBManage.fp_event_callback) {
				gUSBManage.fp_event_callback(USB_EVENT_EP3_TX);
			}
		}
		break;
	case USB_EP4: {
			if (gUSBManage.fp_event_callback) {
				gUSBManage.fp_event_callback(USB_EVENT_EP4_TX);
			}
		}
		break;

	case USB_EP5:
	case USB_EP6:
	case USB_EP7:
	case USB_EP8:
	case USB_EP9:
	case USB_EP10:
	case USB_EP11:
	case USB_EP12:
	case USB_EP13:
	case USB_EP14:
	case USB_EP15: {
			if (gUSBManage.fp_event_callback) {
				gUSBManage.fp_event_callback(USB_EVENT_EP5_TX + (EP - USB_EP5));
			}
		}
		break;

	default:
		break;
	}
}

//@}
