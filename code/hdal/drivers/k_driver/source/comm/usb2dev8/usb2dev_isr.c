/*
    USB Interrupt service routine

    This file define some ISR functions for USB device and is
    for internal used only.

    @file       usb_isr.c
    @ingroup    mIDrvUSB_Device
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef __KERNEL__
#include <stdio.h>
#endif
#include "usb2dev.h"
#include "usb2dev_reg.h"
#include "usb2dev_int.h"

/**
    @addtogroup mIDrvUSB_Device
*/
//@{


//----- Global funcs & variables define here  ------------------
static void usb_ISR_IN0(void);
static void usb_ISR_OUT0(void);
static void usb_ISR_SETUP(void);
static void usb_ISR_RST(void);
static void usb_ISR_SUSPEND(void);
static void usb_ISR_RESUME(void);

static void usb_ISR_OBSELETE(void);

_ALIGNED(4) CONTROL_XFER usb2dev_control_data;
_ALIGNED(4) void (*USBINTVector[])(void) = {
	usb_ISR_IN0,        //0*    EP0 TX INT
	usb_ISR_OUT0,       //1     //1*    EP0 RX INT
	usb_ISR_SETUP,      //2    //2*    EP0 Setup INT
	usb_ISR_OBSELETE,   //3     //3     EP0 Setup Overwrite INT
	usb_ISR_OBSELETE,   //4     //4     EP1 TX INT
	usb_ISR_OBSELETE,   //5     EP2 RX INT
	usb_ISR_RST,        //6*    Bus Reset INT
	usb_ISR_SUSPEND,    //7*    Suspend INT
	usb_ISR_RESUME,     //8*    Resume INT
	usb_ISR_OBSELETE,   //9     Start of Frame INT
	usb_ISR_OBSELETE,   //10    EP0 Tx NAK INT
	usb_ISR_OBSELETE,   //11    EP0 Rx NAK INT
	usb_ISR_OBSELETE,   //12    EP1 NAK INT
	usb_ISR_OBSELETE,   //13    EP2 NAK INT
	usb_ISR_OBSELETE,   //14    EP0 Tx STALL
	usb_ISR_OBSELETE,   //15    EP0 Rx STALL
	usb_ISR_OBSELETE,   //16    EP3 ACK
	usb_ISR_OBSELETE,   //17    EP3 NAK
	usb_ISR_OBSELETE,   //18    EP4 ACK
	usb_ISR_OBSELETE,   //19    EP4 NAK
};


/*
    Processing of EP0 TX.

    @return void
*/
static void usb_ISR_IN0(void)
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

	usb2dev_write_endpoint(USB_EP0, usb2dev_control_data.p_data, &RetBytes);

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
static void usb_ISR_OUT0(void)
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
static void usb_ISR_SETUP(void)
{
	UINT32                      *pControlData;
	FLGPTN                          uiFlag;

	pControlData = (UINT32 *)(&usb2dev_control_data.device_request);

	// Read 8-byte setup packet from FIFO
	*pControlData++ = USB_GETREG(USB_DEVDMACTRL3_REG_OFS);
	*pControlData   = USB_GETREG(USB_DEVDMACTRL3_REG_OFS);

	usb2dev_control_data.w_length = usb2dev_control_data.device_request.w_length;

	clr_flg(FLG_ID_USBTSK, FLGUSB_SETUPTSK_IDLE);

	// notify setup task to parsing the setup command
	set_flg(FLG_ID_USBTSK, FLGUSB_SETUP);

	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_SETUPTSK_IDLE, TWF_ORW);

	USB_PutString("setup ok!\r\n");
}

/*
    Processing of Reset.

    @return void
*/
static void usb_ISR_RST(void)
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

	usb_setControllerState(USB_CONTROLLER_STATE_RESET);

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
static void usb_ISR_SUSPEND(void)
{
	T_USB_DEVFIFOBYTECNT_REG    devFIFOByteCnt;
	T_USB_DEVMAINCTRL_REG       devMainCtl;

	// Add error handler for USB cable is unplugged while a USB DMA tranfer is not finished.

	//Moved the following code from the end to the beginning of this function code.
	//because the fmda_errorhandler() will set flag and may cause task switch.
	if (usb2dev_get_controller_state() == USB_CONTROLLER_STATE_RESET) {
		usb_setControllerState(USB_CONTROLLER_STATE_SUSPEND);
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

	#ifndef __KERNEL__
	// Clear GPIO Wakeup interrupt status
	gpio_clearIntStatus(GPIO_INT_USBWAKEUP);
	#endif

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
static void usb_ISR_RESUME(void)
{
	//The state of the usb controller should be updated
	//when resuming from the suspend state.
	if (usb2dev_get_controller_state() == USB_CONTROLLER_STATE_SUSPEND) {
		usb_setControllerState(USB_CONTROLLER_STATE_RESET);
	}

	if (gUSBManage.fp_event_callback) {
		gUSBManage.fp_event_callback(USB_EVENT_RESUME);
	}

	if ((gUsbChargingCB != NULL) && (gChargerSts == USB_CHARGER_STS_NONE)) {
		USB_CHARGE_EVENT chargeEvent = USB_CHARGE_EVENT_CONNECT;

		if (usb2dev_check_configured()) {
			if (!(gUSBManage.p_config_desc->bm_attributes & USB_DEVDESC_ATB_SELF_POWER)) {
				// Only bus-power can draw up to 500mA after configured
				chargeEvent = USB_CHARGE_EVENT_CONFIGURED;
			}
		}

		gUsbChargingCB(chargeEvent);
	}

}

static void usb_ISR_OBSELETE(void)
{
	// If seeing this msg. Reference to 96650's usb driver for analyzing.
	DBG_ERR("Should Not be seen.\r\n");
}

//@}
