/*
    USB Interrupt service routine

    This file define some ISR functions for USB device and is
    for internal used only.

    @file       usb_isr.c
    @ingroup    mIDrvUSB_Device
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include <stdio.h>
#include "usb2dev.h"
#include "usb2dev528_reg.h"
#include "usb2dev528_int.h"

#define __MODULE__    rtos_usb2dev
#include <kwrap/debug.h>
extern unsigned int rtos_usb2dev_debug_level;

/**
    @addtogroup mIDrvUSB_Device
*/
//@{


//----- Global funcs & variables define here  ------------------
static void usb528_ISR_IN0(void);
static void usb528_ISR_OUT0(void);
static void usb528_ISR_SETUP(void);
static void usb528_ISR_RST(void);
static void usb528_ISR_SUSPEND(void);
static void usb528_ISR_RESUME(void);

static void usb528_ISR_OBSELETE(void);

//_ALIGNED(4) CONTROL_XFER usb2dev_control_data;
extern CONTROL_XFER usb2dev_control_data;
_ALIGNED(4) void (*USBINTVector528[])(void) = {
	usb528_ISR_IN0,        //0*    EP0 TX INT
	usb528_ISR_OUT0,       //1     //1*    EP0 RX INT
	usb528_ISR_SETUP,      //2    //2*    EP0 Setup INT
	usb528_ISR_OBSELETE,   //3     //3     EP0 Setup Overwrite INT
	usb528_ISR_OBSELETE,   //4     //4     EP1 TX INT
	usb528_ISR_OBSELETE,   //5     EP2 RX INT
	usb528_ISR_RST,        //6*    Bus Reset INT
	usb528_ISR_SUSPEND,    //7*    Suspend INT
	usb528_ISR_RESUME,     //8*    Resume INT
	usb528_ISR_OBSELETE,   //9     Start of Frame INT
	usb528_ISR_OBSELETE,   //10    EP0 Tx NAK INT
	usb528_ISR_OBSELETE,   //11    EP0 Rx NAK INT
	usb528_ISR_OBSELETE,   //12    EP1 NAK INT
	usb528_ISR_OBSELETE,   //13    EP2 NAK INT
	usb528_ISR_OBSELETE,   //14    EP0 Tx STALL
	usb528_ISR_OBSELETE,   //15    EP0 Rx STALL
	usb528_ISR_OBSELETE,   //16    EP3 ACK
	usb528_ISR_OBSELETE,   //17    EP3 NAK
	usb528_ISR_OBSELETE,   //18    EP4 ACK
	usb528_ISR_OBSELETE,   //19    EP4 NAK
};



#if 0
/*
    Send data to host.

    This function is a helper function for return setup data to host.
    Because Interrupt was disabled in ISR, polling DMA_CMPLT here...

    @return void
*/
static void usb_ISR_RetSetupData(void)
{
	T_USB_DEVDMACTRL1_REG           devDMACtl1;
	T_USB_DEVDMACTRL2_REG           devDMACtl2;
	T_USB_DEVCXCFGFIFOSTATUS_REG    devCXCFG;
	T_USB_DEVINTGROUP2_REG          devIntGrp2;
	T_USB_DEVINTMASKGROUP0_REG      devIntMskGrp0;
	T_USB_DEVINTMASKGROUP2_REG      devIntMskGrp2;
	T_USB_DEVDMACTRL1_REG           bakDevDMACtl1;
	T_USB_DEVDMACTRL2_REG           bakDevDMACtl2;
	UINT32                          RetBytes;
	UINT32                          uiBakAccFifo;
	UINT32                          uiBakRemainLen;
	BOOL                            bDmaPaused = FALSE;
	BOOL                            bDmaPending = FALSE;


	if (usb2dev_control_data.w_length < EP0_PACKET_SIZE) {
		// CX_IN interrupt is default DISABLED
		// Disable it if this is last IN transfer
		devIntMskGrp0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
		devIntMskGrp0.bit.MCX_IN_INT = 1;
		USB_SETREG(USB_DEVINTMASKGROUP0_REG_OFS, devIntMskGrp0.reg);

		// this is the last packet to return host....
		RetBytes            = usb2dev_control_data.w_length;
		usb2dev_control_data.w_length = 0;
	} else {
		RetBytes             = EP0_PACKET_SIZE;
		usb2dev_control_data.w_length -= EP0_PACKET_SIZE;
	}


	if (1) {
		bakDevDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);

		// Check if there is DMA on-going
		if ((USB_GETREG(USB_DEVACCFIFO_REG_OFS) != 0) && (bakDevDMACtl1.bit.DMA_START == 1)) {
			T_USB_DEVDMACTRL0_REG devDMACtl0;

			devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
			devIntMskGrp2.bit.MDMA_CMPLT = 1;
			USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);

			bakDevDMACtl2.reg = USB_GETREG(USB_DEVDMACTRL2_REG_OFS);

			if (bakDevDMACtl2.reg & 0x03) {
				// If starting address is NOT word align
				// Chose to wait current DMA complete
			} else {
				// If starting address is word align
				// Pause current DMA
				// (DMA pause is only valid when address is word align)
				devDMACtl0.reg = USB_GETREG(USB_DEVDMACTRL0_REG_OFS);
				devDMACtl0.bit.DMA_ABORT = 1;
				USB_SETREG(USB_DEVDMACTRL0_REG_OFS, devDMACtl0.reg);
			}

			// wait DMA_CMPLT and clear it
			do {
				devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);
				USB_PutString(" $ ");
			} while (devIntGrp2.bit.DMA_CMPLT == 0);
			devIntGrp2.bit.DMA_CMPLT = 0;
			USB_SETREG(USB_DEVINTGROUP2_REG_OFS, devIntGrp2.reg);
			do {
				devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);
			} while (devIntGrp2.bit.DMA_CMPLT == 1);

			uiBakRemainLen = USB_GETREG(USB_DEVDMACTRLSTATUS_REG_OFS);
			if (uiBakRemainLen != 0) {
				uiBakAccFifo = USB_GETREG(USB_DEVACCFIFO_REG_OFS);
				bakDevDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);

				bDmaPaused = TRUE;
			}

			bDmaPending = TRUE;
		}
	}

	devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
	devIntMskGrp2.bit.MDMA_CMPLT = 1;
	USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);

	// flush cache
	dma_flushWriteCache((UINT32)usb2dev_control_data.p_data, RetBytes);
	// Config DMA direction & length
	devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
	devDMACtl1.bit.DMA_TYPE = 1; // memory to FIFO
	devDMACtl1.bit.DMA_LEN = RetBytes;
	USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);
	// set DMA target : FIFO Cx
	USB_SETREG(USB_DEVACCFIFO_REG_OFS, 0x10);
	// set DMA address
	devDMACtl2.reg = dma_getPhyAddr((UINT32)usb2dev_control_data.p_data);
	USB_SETREG(USB_DEVDMACTRL2_REG_OFS, devDMACtl2.reg);
	// start Dma
	devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
	devDMACtl1.bit.DMA_START = 1; // bit[0]
	USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);
	// wait until DMA complete
	do {
		devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);
	} while (devIntGrp2.bit.DMA_CMPLT == 0);

	// If no DMA is interrupted => finish CX IN as nothing happen
	if (bDmaPending == FALSE) {
		// clear DMA_CMPLT status
		devIntGrp2.bit.DMA_CMPLT = 0;
		USB_SETREG(USB_DEVINTGROUP2_REG_OFS, devIntGrp2.reg);
		do {
			devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);
		} while (devIntGrp2.bit.DMA_CMPLT == 1);
		// select FIFO : none
		USB_SETREG(USB_DEVACCFIFO_REG_OFS, 0);
	} else {
		// If there exist DMA interrupted && this DMA is not completed
		//      resume it
		// Else
		//      end with DMA flag is not cleared
		if (bDmaPaused == TRUE) {
			// clear DMA_CMPLT status
			devIntGrp2.bit.DMA_CMPLT = 0;
			USB_SETREG(USB_DEVINTGROUP2_REG_OFS, devIntGrp2.reg);
			do {
				devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);
			} while (devIntGrp2.bit.DMA_CMPLT == 1);

			// resume DMA transfer
			USB_SETREG(USB_DEVACCFIFO_REG_OFS, uiBakAccFifo);
			bakDevDMACtl2.bit.DMA_MADDR += bakDevDMACtl1.bit.DMA_LEN - uiBakRemainLen;
			USB_SETREG(USB_DEVDMACTRL2_REG_OFS, bakDevDMACtl2.reg);
			bakDevDMACtl1.bit.DMA_START = 0;
			bakDevDMACtl1.bit.DMA_LEN = uiBakRemainLen;
			USB_SETREG(USB_DEVDMACTRL1_REG_OFS, bakDevDMACtl1.reg);
			bakDevDMACtl1.bit.DMA_START = 1;
			USB_SETREG(USB_DEVDMACTRL1_REG_OFS, bakDevDMACtl1.reg);
		} else {
			// Not clear DMA_CMPLT
			// This will let USB ISR re-triggerd to set DMA finish flag
		}
	}


	usb2dev_control_data.p_data += RetBytes;

	// set CX_DONE at last time
	if (usb2dev_control_data.w_length == 0) {
		devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
		devCXCFG.bit.CX_DONE = 1; // bit[0]
		USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);
	}

	devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
	devIntMskGrp2.bit.MDMA_CMPLT = 0;
	USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);
}
#endif


/*
    Processing of EP0 TX.

    @return void
*/
static void usb528_ISR_IN0(void)
{
#if 1
	T_USB_DEVCXCFGFIFOSTATUS_REG    devCXCFG;
	T_USB_DEVINTMASKGROUP0_REG      devIntMskGrp0;
	UINT32                          RetBytes;

	if (usb2dev_control_data.w_length == 0) {
		return;
	}

	if (usb2dev_control_data.w_length <= EP0_PACKET_SIZE) {
		// CX_IN interrupt is default DISABLED
		// Disable it if this is last IN transfer
		devIntMskGrp0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
		devIntMskGrp0.bit.MCX_IN_INT = 1;
		USB_SETREG(USB_DEVINTMASKGROUP0_REG_OFS, devIntMskGrp0.reg);

		// this is the last packet to return host....
		RetBytes            = usb2dev_control_data.w_length;
	} else {
		RetBytes             = EP0_PACKET_SIZE;
	}

	usb2dev528_write_endpoint(USB_EP0, usb2dev_control_data.p_data, &RetBytes);

	usb2dev_control_data.p_data   += RetBytes;
	usb2dev_control_data.w_length -= RetBytes;

	// set CX_DONE at last time
	if (usb2dev_control_data.w_length == 0) {
		devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
		devCXCFG.bit.CX_DONE = 1;
		USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);
	} else {
		// To prevent abnormal situation to cause transfer not complete all.
		devIntMskGrp0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
		devIntMskGrp0.bit.MCX_IN_INT = 0;
		USB_SETREG(USB_DEVINTMASKGROUP0_REG_OFS, devIntMskGrp0.reg);
	}

#else
	if (usb2dev_control_data.w_length) {
		usb_ISR_RetSetupData();
	}
#endif
}

/*
    Processing of EP0 RX.

    @return void
*/
static void usb528_ISR_OUT0(void)
{
	if (gUSBManage.fp_event_callback) {
		gUSBManage.fp_event_callback(USB_EVENT_OUT0);
	}
}

/*
    Processing of EP0 Setup.

    First abort DMA and clean fifo if FIFO is being accessed. Then
    read out 8-byte setup packet from FIFO and wakeup the setup task
    to handle other things.

    @return void
*/
static void usb528_ISR_SETUP(void)
{
	UINT32                      *pControlData;

	pControlData = (UINT32 *)(&usb2dev_control_data.device_request);

	// Read 8-byte setup packet from FIFO
	*pControlData++ = USB_GETREG(USB_DEVDMACTRL3_REG_OFS);
	*pControlData   = USB_GETREG(USB_DEVDMACTRL3_REG_OFS);

	usb2dev_control_data.w_length = usb2dev_control_data.device_request.w_length;

	// notify setup task to parsing the setup command
	set_flg(FLG_ID_USBTSK, FLGUSB_SETUP);

	USB_PutString("setup ok!\r\n");
}

/*
    Processing of Reset.

    @return void
*/
static void usb528_ISR_RST(void)
{
	T_USB_DEVINTMASKGROUP1_REG      devIntMskGrp1;
	T_USB_DEVTEST_REG               devTest;
	T_USB_DEVCXCFGFIFOSTATUS_REG    devCXCFG;
	T_USB_DEVMAINCTRL_REG           devMainCtl;

	//disable all DMA FIFO access
	USB_SETREG(USB_DEVACCFIFO_REG_OFS, 0);

	if (gUSBManage.fp_event_callback) {
		gUSBManage.fp_event_callback(USB_EVENT_RESET);
	}

	if ((gUsbChargingCB != NULL) && (gChargerSts == USB_CHARGER_STS_NONE)) {
		gUsbChargingCB(USB_CHARGE_EVENT_CONNECT);
	}

	//Set USB device Addr=0 & not in configured state
	USB_SETREG(USB_DEVADDR_REG_OFS, 0);

	// disable FIFO0 IN int // TODO Review
	devIntMskGrp1.reg = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
	devIntMskGrp1.bit.MF0_IN_INT = 1;  // bit[16]
	USB_SETREG(USB_DEVINTMASKGROUP1_REG_OFS, devIntMskGrp1.reg);

	// clear Bus Reset int
	//devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);
	//devIntGrp2.bit.USBRST_INT = 1;  // bit[0]
	//USB_SETREG(USB_DEVINTGROUP2_REG_OFS, devIntGrp2.reg);

	// Clear All FIFO
	devTest.reg = USB_GETREG(USB_DEVTEST_REG_OFS);
	devTest.bit.TST_CLRFF = 1; // bit[0]
	USB_SETREG(USB_DEVTEST_REG_OFS, devTest.reg);

	// clear EP0 Stall
	devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
	devCXCFG.bit.CX_STL = 0; // bit[2]
	USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);

	usb528_setControllerState(USB_CONTROLLER_STATE_RESET);

	// In 2.0 spec page255, the remote wakeup should be disabled after bus reset.
	bUSBflag_Remote_wakeup &= ~0x01;
	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	devMainCtl.bit.CAP_RMWAKUP = 0;
	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

}

/*
    Processing of Suspend.

    @return void
*/
static void usb528_ISR_SUSPEND(void)
{
	T_USB_DEVFIFOBYTECNT_REG    devFIFOByteCnt;
	T_USB_DEVMAINCTRL_REG       devMainCtl;

	// Add error handler for USB cable is unplugged while a USB DMA tranfer is not finished.

	//Moved the following code from the end to the beginning of this function code.
	//because the fmda_errorhandler() will set flag and may cause task switch.
	if (usb2dev528_get_controller_state() == USB_CONTROLLER_STATE_RESET) {
		usb528_setControllerState(USB_CONTROLLER_STATE_SUSPEND);
	}

	//Clear all USB FIFOs
	devFIFOByteCnt.reg = USB_GETREG(USB_DEVFIFO0BYTECNT_REG_OFS);
	devFIFOByteCnt.bit.FFRST = 1;
	USB_SETREG(USB_DEVFIFO0BYTECNT_REG_OFS, devFIFOByteCnt.reg);

	devFIFOByteCnt.reg = USB_GETREG(USB_DEVFIFO1BYTECNT_REG_OFS);
	devFIFOByteCnt.bit.FFRST = 1;
	USB_SETREG(USB_DEVFIFO1BYTECNT_REG_OFS, devFIFOByteCnt.reg);

	devFIFOByteCnt.reg = USB_GETREG(USB_DEVFIFO2BYTECNT_REG_OFS);
	devFIFOByteCnt.bit.FFRST = 1;
	USB_SETREG(USB_DEVFIFO2BYTECNT_REG_OFS, devFIFOByteCnt.reg);

	devFIFOByteCnt.reg = USB_GETREG(USB_DEVFIFO3BYTECNT_REG_OFS);
	devFIFOByteCnt.bit.FFRST = 1;
	USB_SETREG(USB_DEVFIFO3BYTECNT_REG_OFS, devFIFOByteCnt.reg);

	devFIFOByteCnt.reg = USB_GETREG(USB_DEVFIFO4BYTECNT_REG_OFS);
	devFIFOByteCnt.bit.FFRST = 1;
	USB_SETREG(USB_DEVFIFO4BYTECNT_REG_OFS, devFIFOByteCnt.reg);

	devFIFOByteCnt.reg = USB_GETREG(USB_DEVFIFO5BYTECNT_REG_OFS);
	devFIFOByteCnt.bit.FFRST = 1;
	USB_SETREG(USB_DEVFIFO5BYTECNT_REG_OFS, devFIFOByteCnt.reg);

	devFIFOByteCnt.reg = USB_GETREG(USB_DEVFIFO6BYTECNT_REG_OFS);
	devFIFOByteCnt.bit.FFRST = 1;
	USB_SETREG(USB_DEVFIFO6BYTECNT_REG_OFS, devFIFOByteCnt.reg);

	devFIFOByteCnt.reg = USB_GETREG(USB_DEVFIFO7BYTECNT_REG_OFS);
	devFIFOByteCnt.bit.FFRST = 1;
	USB_SETREG(USB_DEVFIFO7BYTECNT_REG_OFS, devFIFOByteCnt.reg);


	//Diable DMA for each FIFO
	USB_SETREG(USB_DEVACCFIFO_REG_OFS, 0);

	// Clear GPIO Wakeup interrupt status
	gpio_clearIntStatus(GPIO_INT_USBWAKEUP);


	//For the USB device mode require to know device enter suspend mode.
	//If the USB device don't care suspend mode, set this callback function as NULL.
	if (gUSBManage.fp_usb_suspend != NULL) {
		//Notify AP that device enter suspend mode.
		gUSBManage.fp_usb_suspend();
	} else {
		//Defualt mode to enter suspend. USB_GoSuspend()
		devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
		devMainCtl.bit.GOSUSP = 1;
		USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);
	}

	//For Some host in Standby mode, VBus will turn on case.
	//Since Device enter suspend. Ignore all extra Interrupt. In fact, there is no others "true" interrupt except suspend!!
	gUSBCause = 0;    //Set this global value will ignore all other fake interrupts.

	if (gUSBManage.fp_event_callback) {
		gUSBManage.fp_event_callback(USB_EVENT_SUSPEND);
	}

	if ((gUsbChargingCB != NULL) && (gChargerSts == USB_CHARGER_STS_NONE)) {
		gUsbChargingCB(USB_CHARGE_EVENT_SUSPEND);
	}
}

/*
    Processing of Resume.

    @return void
*/
static void usb528_ISR_RESUME(void)
{
	//The state of the usb controller should be updated
	//when resuming from the suspend state.
	if (usb2dev528_get_controller_state() == USB_CONTROLLER_STATE_SUSPEND) {
		usb528_setControllerState(USB_CONTROLLER_STATE_RESET);
	}

	if (gUSBManage.fp_event_callback) {
		gUSBManage.fp_event_callback(USB_EVENT_RESUME);
	}

	if ((gUsbChargingCB != NULL) && (gChargerSts == USB_CHARGER_STS_NONE)) {
		USB_CHARGE_EVENT chargeEvent = USB_CHARGE_EVENT_CONNECT;

		if (usb2dev528_check_configured()) {
			if (!(gUSBManage.p_config_desc->bm_attributes & USB_DEVDESC_ATB_SELF_POWER)) {
				// Only bus-power can draw up to 500mA after configured
				chargeEvent = USB_CHARGE_EVENT_CONFIGURED;
			}
		}

		gUsbChargingCB(chargeEvent);
	}

}

static void usb528_ISR_OBSELETE(void)
{
	// If seeing this msg. Reference to 96650's usb driver for analyzing.
	DBG_ERR("Should Not be seen.\r\n");
}

//@}
