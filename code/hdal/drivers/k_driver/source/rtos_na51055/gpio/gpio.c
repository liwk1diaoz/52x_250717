/*
    General Purpose I/O controller

    General Purpose I/O controller

    @file       gpio.c
    @ingroup    mIDrvIO_GPIO
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include <stdio.h>
#include "gpio.h"
#include "gpio_reg.h"
#include "interrupt.h"
#include "io_address.h"
#include "pad.h"
#include "kwrap/type.h"
#include <kwrap/nvt_type.h>
#include <kwrap/spinlock.h>
#include <kwrap/flag.h>
#include <kwrap/debug.h>
#include <comm/driver.h>
#include "top.h"

#define GPIO_INT_NUM                (32)
#define DGPIO_INT_NUM               (17)
#define DGPIO32_INT_NUM             (32)

static VK_DEFINE_SPINLOCK(gpio_spinlock);
#define loc_cpu(gpio_flags) vk_spin_lock_irqsave(&gpio_spinlock, gpio_flags)
#define unl_cpu(gpio_flags) vk_spin_unlock_irqrestore(&gpio_spinlock, gpio_flags)

#define loc_multiCores(gpio_flags)    loc_cpu(gpio_flags)
#define unl_multiCores(gpio_flags)    unl_cpu(gpio_flags)

static ID FLG_ID_GPIO;
static ID FLG_ID_GPIO2;
#define DRV_SUPPORT_IST ENABLE
//
//  gpio register access definition
//
#define GPIO_REG_ADDR(ofs)       (IOADDR_GPIO_REG_BASE+(ofs))
#define GPIO_GETREG(ofs)         INW(GPIO_REG_ADDR(ofs))
#define GPIO_SETREG(ofs,value)   OUTW(GPIO_REG_ADDR(ofs), (value))


/**
    @addtogroup mIDrvIO_GPIO
*/
//@{
static UINT32  uiGPIOOpened = FALSE;

static UINT32  uiGPIOIsrFlag = 0;   // record which GPIO ISR is installed
static UINT32  uiDGPIOIsrFlag = 0;  // record which DGPIO iSR is installed
#if (DRV_SUPPORT_IST == ENABLE && _EMULATION_ == DISABLE)
DRV_CB pfIstCB_GPIO[32] = {NULL};
DRV_CB pfIstCB_DGPIO[21] = {NULL};
DRV_CB pfIstCB_DGPIO32[32] = {NULL};
#else
static DRV_CB  fpGPIOIsr[GPIO_INT_NUM]      = { NULL, NULL, NULL, NULL, NULL,
						NULL, NULL, NULL, NULL, NULL,
						NULL, NULL, NULL, NULL, NULL,
						NULL, NULL, NULL, NULL, NULL,
						NULL, NULL, NULL, NULL, NULL,
						NULL, NULL, NULL, NULL, NULL,
						NULL, NULL
						};

static DRV_CB  fpDGPIOIsr[DGPIO_INT_NUM]    = { NULL, NULL, NULL, NULL, NULL,
						NULL, NULL, NULL, NULL, NULL,
						NULL, NULL, NULL, NULL, NULL,
						NULL, NULL
						};

static DRV_CB  fpDGPIO32Isr[DGPIO32_INT_NUM] = { NULL, NULL, NULL, NULL, NULL,
						NULL, NULL, NULL, NULL, NULL,
						NULL, NULL, NULL, NULL, NULL,
						NULL, NULL, NULL, NULL, NULL,
						NULL, NULL, NULL, NULL, NULL,
						NULL, NULL, NULL, NULL, NULL,
						NULL, NULL
						};
#endif

/*
    gpio ISR

    gpio ISR

    @return void
*/
irqreturn_t gpio_isr(int irq, void *devid)
{
	UINT32  tempGPIOIntStatus, tempDGPIOIntStatus;
	FLGPTN  GPIOFlag = 0, DGPIOFlag = 0;
	BOOL    bSetGPIOFlag = FALSE, bSetDGPIOFlag = FALSE;
	UINT32  uiEvent;
#if (DRV_SUPPORT_IST == DISABLE || _EMULATION_ == ENABLE)
	UINT32  i;
#endif

	// Get interrupt status and only handle enabled PINs
	tempGPIOIntStatus   = GPIO_GETREG(GPIO_INT_STS_REG_OFS) & GPIO_GETREG(GPIO_INT_EN_REG_OFS);
	tempDGPIOIntStatus  = GPIO_GETREG(GPIO_DINT_STS_REG_OFS) & GPIO_GETREG(GPIO_DINT_EN_REG_OFS);

	if ((tempGPIOIntStatus == 0) && (tempDGPIOIntStatus == 0)) {
		return IRQ_HANDLED;
	}

	// Clear interrupt status
	GPIO_SETREG(GPIO_INT_STS_REG_OFS, (REGVALUE)tempGPIOIntStatus);
	GPIO_SETREG(GPIO_DINT_STS_REG_OFS, (REGVALUE)tempDGPIOIntStatus);

#if (GPIO_INT_TO_DSP == ENABLE)
	tempGPIOIntStatus   = GPIO_GETREG(GPIO_DSP_INT_STS_REG_OFS) & GPIO_GETREG(GPIO_DSP_INT_EN_REG_OFS);
	tempDGPIOIntStatus  = GPIO_GETREG(GPIO_DSP_DINT_STS_REG_OFS) & GPIO_GETREG(GPIO_DSP_DINT_EN_REG_OFS);

	// Clear interrupt status
	GPIO_SETREG(GPIO_DSP_INT_STS_REG_OFS, (REGVALUE)tempGPIOIntStatus);
	GPIO_SETREG(GPIO_DSP_INT_STS_REG_OFS, (REGVALUE)tempDGPIOIntStatus);

	while (tempGPIOIntStatus) {
		i = __builtin_ctz(tempGPIOIntStatus);
		tempGPIOIntStatus &= ~(1 << i);
		DBG_DUMP("GPIO_INT[%2d]\r\n", i);
	}

	while (tempDGPIOIntStatus) {
		i = __builtin_ctz(tempDGPIOIntStatus);
		tempDGPIOIntStatus &= ~(1 << i);
		DBG_DUMP("DGPIO_INT[%2d]\r\n", i);
	}
#endif

#if (GPIO_INT_TO_DSP2 == ENABLE)
	tempGPIOIntStatus   = GPIO_GETREG(GPIO_DSP2_INT_STS_REG_OFS) & GPIO_GETREG(GPIO_DSP2_INT_EN_REG_OFS);
	tempDGPIOIntStatus  = GPIO_GETREG(GPIO_DSP2_DINT_STS_REG_OFS) & GPIO_GETREG(GPIO_DSP2_DINT_EN_REG_OFS);

	// Clear interrupt status
	GPIO_SETREG(GPIO_DSP2_INT_STS_REG_OFS, (REGVALUE)tempGPIOIntStatus);
	GPIO_SETREG(GPIO_DSP2_INT_STS_REG_OFS, (REGVALUE)tempDGPIOIntStatus);

	while (tempGPIOIntStatus) {
		i = __builtin_ctz(tempGPIOIntStatus);
		tempGPIOIntStatus &= ~(1 << i);
		DBG_DUMP("GPIO_INT[%2d]\r\n", i);
	}

	while (tempDGPIOIntStatus) {
		i = __builtin_ctz(tempDGPIOIntStatus);
		tempDGPIOIntStatus &= ~(1 << i);
		DBG_DUMP("DGPIO_INT[%2d]\r\n", i);
	}
#endif

	// Call handler
	uiEvent = tempGPIOIntStatus & uiGPIOIsrFlag;
	//Once testing under emulation(real or FPGA), not use IST both

#if (DRV_SUPPORT_IST == ENABLE && _EMULATION_ == DISABLE)
	if (uiEvent) {
		kick_bh(INT_ID_GPIO, uiEvent, NULL);
	}
#else
	while (uiEvent) {
		i = __builtin_ctz(uiEvent);
		if (fpGPIOIsr[i] != NULL) {
			fpGPIOIsr[i](0);
		}

		uiEvent &= ~(1 << i);
	}
#endif

	if (tempGPIOIntStatus & (~uiGPIOIsrFlag)) {
		GPIOFlag = tempGPIOIntStatus & (~uiGPIOIsrFlag);
		bSetGPIOFlag = TRUE;
	}

	uiEvent = tempDGPIOIntStatus & uiDGPIOIsrFlag;

#if (DRV_SUPPORT_IST == ENABLE && _EMULATION_ == DISABLE)

	if (uiEvent) {
		kick_bh(INT_ID_DUMMY_DGPIO, uiEvent, NULL);
	}
#else
	while (uiEvent) {
		i = __builtin_ctz(uiEvent);
		if (nvt_get_chip_id() == CHIP_NA51055) {
			if (fpDGPIOIsr[i] != NULL) {
				fpDGPIOIsr[i](0);
			}
		} else {
			if (fpDGPIO32Isr[i] != NULL) {
				fpDGPIO32Isr[i](0);
			}
		}

		uiEvent &= ~(1 << i);
	}
#endif
	if (tempDGPIOIntStatus & (~uiDGPIOIsrFlag)) {
		DGPIOFlag = tempDGPIOIntStatus & (~uiDGPIOIsrFlag);
		bSetDGPIOFlag = TRUE;
	}

	// Signal event flag for specific GPIO pin
	if (bSetGPIOFlag == TRUE) {
		iset_flg(FLG_ID_GPIO, GPIOFlag);
	}

	// Signal event flag for specific GPIO pin
	if (bSetDGPIOFlag == TRUE) {
		iset_flg(FLG_ID_GPIO2, DGPIOFlag);
	}

	return IRQ_HANDLED;
}

/**
@name OS level GPIO API
*/
//@{

/**
    Wait gpio interrupt flag

    @param[in] gpioIntID    GPIO interrupt ID
                            - @b GPIO_INT_00 ~ GPIO_INT_23: normal GPIO interrupt ID
                            - @b GPIO_INT_32 ~ GPIO_INT_36: DGPIO interrupt ID
                            - @b GPIO_INT_48 ~ GPIO_INT_59: DRAM_KEY GPIO interrupt ID

    @return
        - @b E_OK: success
        - @b Else: fail
*/
ER gpio_waitIntFlag(UINT32 gpioIntID)
{
	FLGPTN uiFlag;

	if (gpioIntID < GPIO_INT_32) {
		return  wai_flg(&uiFlag, FLG_ID_GPIO, FLGPTN_BIT(gpioIntID), TWF_ORW | TWF_CLR);
	} else {
		return  wai_flg(&uiFlag, FLG_ID_GPIO2, FLGPTN_BIT(gpioIntID - GPIO_INT_32), TWF_ORW | TWF_CLR);
	}
}

/**
    Open GPIO driver

    Open GPIO block.

    @return Always return E_OK
*/
ER gpio_open(void)
{
	if (uiGPIOOpened) {
		uiGPIOOpened++;
		return E_OK;
	}

	// Clear flag
	clr_flg(FLG_ID_GPIO, FLGPTN_BIT_ALL);
	clr_flg(FLG_ID_GPIO2, FLGPTN_BIT_ALL);

	uiGPIOOpened = 1;

	return E_OK;
}

/**
    Close GPIO driver

    @return Always return E_OK
*/
ER gpio_close(void)
{
	if (uiGPIOOpened == FALSE) {
		return E_OK;
	} else if (uiGPIOOpened > 1) {
		uiGPIOOpened--;
		return E_OK;
	}

	uiGPIOOpened = FALSE;

	return E_OK;
}

/**
    Get gpio open status.

    check if gpio driver is opened.

    @return
        - @b TRUE: GPIO driver is opened
        - @b FALSE: GPIO driver is not opened
*/
BOOL gpio_isOpened(void)
{
	return uiGPIOOpened > 0;
}

//@} //OS level GPIO API


/**
@name Common GPIO pin access API
*/
//@{

/**
    Set GPIO pin direction.

    Set specified GPIO pin to input or output mode.

    @param[in] pin GPIO pin number.
    @param[in] dir GPIO direction
                - @b GPIO_DIR_INPUT: set pin to input mode
                - @b GPIO_DIR_OUTPUT: set pin to output mode

    @return void
*/
void gpio_setDir(UINT32 pin, GPIO_DIR dir)
{
	REGVALUE    RegData;
	UINT32      ofs = (pin >> 5) << 2;
	unsigned long flags;

#if 0
	// Patch S_GPIO_29 double bound issue
	// When input mode, pad driving should <= 20mA
	if ((pin == S_GPIO_29) && (dir == GPIO_DIR_INPUT)) {
		PAD_DRIVINGSINK driving;

		pad_get_driving_sink(PAD_DS_SGPIO29, &driving);
		if (driving >= PAD_DRIVINGSINK_25MA) {
			DBG_WRN("SGPIO29 can't be set to input mode when driving is 0x%lx\r\n", driving);
			DBG_WRN("Force driving to 12.5mA...\r\n");
			pad_set_driving_sink(PAD_DS_SGPIO29, PAD_DRIVINGSINK_12P5MA);
		}
	}
#endif

	pin &= (32 - 1);

	//race condition protect. enter critical section
	//loc_cpu(flags);
	loc_multiCores(flags);

	RegData = GPIO_GETREG(GPIO_STRG_DIR_REG_OFS + ofs);

	if (dir) {
		RegData |= (1 << pin);    //output
	} else {
		RegData &= ~(1 << pin);    //input
	}

	GPIO_SETREG(GPIO_STRG_DIR_REG_OFS + ofs, RegData);

	//race condition protect. leave critical section
	//unl_cpu(flags);
	unl_multiCores(flags);
}

/**
    Get GPIO pin direction.

    Get input/output mode of specified GPIO pin.

    @param[in] pin GPIO pin number.

    @return
        - @b GPIO_DIR_INPUT: pin is under input mode
        - @b GPIO_DIR_OUTPUT: pin is under output mode
*/
GPIO_DIR gpio_getDir(UINT32 pin)
{
	UINT32 tmp;
	UINT32 ofs = (pin >> 5) << 2;

	pin &= (32 - 1);
	tmp = (1 << pin);
	return (GPIO_GETREG(GPIO_STRG_DIR_REG_OFS + ofs) & tmp) != 0;
}

/**
    Set GPIO pin.

    Set specified GPIO pin to output HIGH.

    @param[in] pin GPIO pin number.

    @return void
*/
void gpio_setPin(UINT32 pin)
{
	UINT32 tmp;
	UINT32 ofs = (pin >> 5) << 2;

	pin &= (32 - 1);
	tmp = (1 << pin);

	GPIO_SETREG(GPIO_STRG_SET_REG_OFS + ofs, tmp);
}

/**
    Clear GPIO pin.

    Clear specified GPIO pin to output LOW.

    @param[in] pin GPIO pin number.

    @return void
*/
void gpio_clearPin(UINT32 pin)
{
	UINT32 tmp;
	UINT32 ofs = (pin >> 5) << 2;

	pin &= (32 - 1);
	tmp = (1 << pin);

	GPIO_SETREG(GPIO_STRG_CLR_REG_OFS + ofs, tmp);
}

/**
    Get GPIO pin.

    Get signal status of specified GPIO pin

    @param[in] pin GPIO pin number.

    @return
        - @b 0: LOW
        - @b 1: HIGH
*/
UINT32 gpio_getPin(UINT32 pin)
{
	UINT32 tmp;
	UINT32 ofs = (pin >> 5) << 2;

	pin &= (32 - 1);
	tmp = (1 << pin);

	return (GPIO_GETREG(GPIO_STRG_DAT_REG_OFS + ofs) & tmp) != 0;
}

/**
    Set GPIO pin through data registers.

    Set specified GPIO pin to output HIGH through data registers.

    @param[in] pin GPIO pin number.

    @return void
*/
void  gpio_pullSet(UINT32 pin)
{
	REGVALUE    RegData;
	UINT32      ofs = (pin >> 5) << 2;
	unsigned long flags;

	pin &= (32 - 1);

	//race condition protect. enter critical section
	//loc_cpu(flags);
	loc_multiCores(flags);

	RegData = GPIO_GETREG(GPIO_STRG_DAT_REG_OFS + ofs);
	RegData |= (1 << pin);

	GPIO_SETREG(GPIO_STRG_DAT_REG_OFS + ofs, RegData);

	//race condition protect. leave critical section
	unl_multiCores(flags);
}

/**
    Clear GPIO pin through data registers.

    Clear specified GPIO pin to output LOW through data registers.

    @param[in] pin GPIO pin number.

    @return void
*/
void  gpio_pullClear(UINT32 pin)
{
	REGVALUE    RegData;
	UINT32      ofs = (pin >> 5) << 2;
	unsigned long flags;

	pin &= (32 - 1);

	//race condition protect. enter critical section
	//loc_cpu(flags);
	loc_multiCores(flags);

	RegData  = GPIO_GETREG(GPIO_STRG_DAT_REG_OFS + ofs);
	RegData &= ~(1 << pin);

	GPIO_SETREG(GPIO_STRG_DAT_REG_OFS + ofs, RegData);

	//race condition protect. leave critical section
	//unl_cpu(flags);
	unl_multiCores(flags);
}

//@} //Common GPIO pin access API

/**
    @name GPIO specific configuration API
*/
//@{

/**
    Read GPIO data register.

    @param[in] dataidx data register index (0,1,2,3,4,5,6,7)

    @return register value of requested
*/
UINT32 gpio_readData(UINT32 dataidx)
{
	UINT32 ofs = (dataidx << 2);

	return GPIO_GETREG(GPIO_STRG_DAT_REG_OFS + ofs);
}

/**
    Write GPIO data register.

    @param[in] dataidx data register index (0,1,2,3,4,5,6,7)
    @param[in] value register word value to write to data register

    @return void
*/
void gpio_writeData(UINT32 dataidx, UINT32 value)
{
	UINT32 ofs = (dataidx << 2);

	GPIO_SETREG(GPIO_STRG_DAT_REG_OFS + ofs, value);
}

#if 0
/**
    Read GPIO direction register.

    @param[in] dataidx direction register index (0,1,2,3,4,5,6,7)

    @return register value of requested
*/
UINT32 gpio_readDir(UINT32 dataidx)
{
	UINT32 ofs = (dataidx << 2);

	return GPIO_GETREG(GPIO_STRG_DIR_REG_OFS + ofs);
}

/**
    Write GPIO direction register.

    @param[in] dataidx direction register index (0,1,2,3,4,5,6,7)
    @param[in] value register word value to write to data register

    @return void
*/
void gpio_writeDir(UINT32 dataidx, UINT32 value)
{
	UINT32 ofs = (dataidx << 2);

	GPIO_SETREG(GPIO_STRG_DIR_REG_OFS + ofs, value);
}
#endif
//@} //GPIO specific configuration API


/**
@name DGPIO specific configuration
@{
*/
/**
    Read DGPIO data register.

    @return register value of requested
*/
UINT32 dgpio_readData(void)
{
	return GPIO_GETREG(GPIO_DGPIO_DAT_REG_OFS);
}

/**
    Write DGPIO data register.

    @param[in] value register word value to write to data register

    @return void
*/
void dgpio_writeData(UINT32 value)
{
	GPIO_SETREG(GPIO_DGPIO_DAT_REG_OFS, value);
}

#if 0
/**
    Read DGPIO direction register.

    @return register value of requested
*/
UINT32 dgpio_readDir(void)
{
	return GPIO_GETREG(GPIO_DGPIO_DIR_REG_OFS);
}

/**
    Write DGPIO direction register.

    @param[in] value register word value to write to direction register

    @return void
*/
void dgpio_writeDir(UINT32 value)
{
	GPIO_SETREG(GPIO_DGPIO_DIR_REG_OFS, value);
}
#endif

//@} //DGPIO specific configuration

/**
    @name GPIO interrupt configuration
@{
*/

/**
    Enable GPIO interrupt pin.

    For example, gpio_enableInt(GPIO_INT_00); or gpio_enableInt(GPIO_INT_USBWAKEUP);

    @note Corresponding GPIO PIN should be set to output mode (by gpio_setDir()).
        Once GPIO PIN is set to input mode, any transition on this PAD will pass to GPIO interrupt circuit (even corresponding pinmux is set to module)

    @param[in] ipin enabled GPIO Interrupt pin:
                    - GPIO_INT_00 ~ GPIO_INT_40

    @return void
*/
void  gpio_enableInt(UINT32 ipin)
{
	REGVALUE    RegData;
	unsigned long flags;

	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (ipin >= (DGPIO_INT_NUM + 32)) {
			return;
		}
	} else {
		if (ipin >= (DGPIO32_INT_NUM + 32)) {
			return;
		}
	}

	//race condition protect. enter critical section
	//loc_cpu(flags);
	loc_multiCores(flags);

	if (ipin < 32) {
		RegData  = GPIO_GETREG(GPIO_INT_EN_REG_OFS);
		RegData |= (1 << ipin);
		GPIO_SETREG(GPIO_INT_EN_REG_OFS, RegData);

		if (nvt_get_chip_id() != CHIP_NA51055) {
			RegData  = GPIO_GETREG(GPIO_DESTINATION_REG_OFS);
			RegData |= (1 << ipin);
			GPIO_SETREG(GPIO_DESTINATION_REG_OFS, RegData);
		}
	} else {
		ipin -= 32;
		RegData  = GPIO_GETREG(GPIO_DINT_EN_REG_OFS);
		RegData |= (1 << ipin);
		GPIO_SETREG(GPIO_DINT_EN_REG_OFS, RegData);

		if (nvt_get_chip_id() != CHIP_NA51055) {
			RegData  = GPIO_GETREG(GPIO_D_DESTINATION_REG_OFS);
			RegData |= (1 << ipin);
			GPIO_SETREG(GPIO_D_DESTINATION_REG_OFS, RegData);
		}
	}

	//race condition protect. leave critical section
	//unl_cpu(flags);
	unl_multiCores(flags);
}

/**
    Disable GPIO interrupt pin.

    @param[in] ipin disabled GPIO Interrupt pin:
                    - GPIO_INT_00 ~ GPIO_INT_40

    @return void
*/
void  gpio_disableInt(UINT32 ipin)
{
	REGVALUE    RegData;
	unsigned long flags;


	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (ipin >= (DGPIO_INT_NUM + 32)) {
			return;
		}
	} else {
		if (ipin >= (DGPIO32_INT_NUM + 32)) {
			return;
		}
	}

	//race condition protect. enter critical section
	//loc_cpu(flags);
	loc_multiCores(flags);

	if (ipin < 32) {
		RegData = GPIO_GETREG(GPIO_INT_EN_REG_OFS);
		RegData &= ~(1 << ipin);
		GPIO_SETREG(GPIO_INT_EN_REG_OFS, RegData);

		if (nvt_get_chip_id() != CHIP_NA51055) {
			RegData  = GPIO_GETREG(GPIO_DESTINATION_REG_OFS);
			RegData &= ~(1 << ipin);
			GPIO_SETREG(GPIO_DESTINATION_REG_OFS, RegData);
		}
	} else {
		ipin -= 32;
		RegData = GPIO_GETREG(GPIO_DINT_EN_REG_OFS);
		RegData &= ~(1 << ipin);
		GPIO_SETREG(GPIO_DINT_EN_REG_OFS, RegData);

		if (nvt_get_chip_id() != CHIP_NA51055) {
			RegData  = GPIO_GETREG(GPIO_D_DESTINATION_REG_OFS);
			RegData &= ~(1 << ipin);
			GPIO_SETREG(GPIO_D_DESTINATION_REG_OFS, RegData);
		}
	}

	//race condition protect. leave critical section
	//unl_cpu(flags);
	unl_multiCores(flags);
}

/**
    Get GPIO interrupt pin enable status.

    If you enable some GPIO interrupt in your code, you have to disable the
    interrupt when you don't want to use the GPIO anymore.

    @param[in] ipin GPIO Interrupt pin:
                    - GPIO_INT_00 ~ GPIO_INT_40
    @return
        - @b TRUE: interrupt is enabled
        - @b FALSE: interrupt is disabled
*/
UINT32 gpio_getIntEnable(UINT32 ipin)
{
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (ipin >= (DGPIO_INT_NUM + 32)) {
			return 0;
		}
	} else {
		if (ipin >= (DGPIO32_INT_NUM + 32)) {
			return 0;
		}
	}

	if (ipin < 32) {
		return (GPIO_GETREG(GPIO_INT_EN_REG_OFS) & (1 << ipin)) > 0;
	} else {
		ipin -= 32;
		return (GPIO_GETREG(GPIO_DINT_EN_REG_OFS) & (1 << ipin)) > 0;
	}
}

/**
    Clear GPIO interrupt status.

    @param[in] ipin GPIO Interrupt pin:
                    - GPIO_INT_00 ~ GPIO_INT_40

    @return void
*/
void gpio_clearIntStatus(UINT32 ipin)
{
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (ipin >= (DGPIO_INT_NUM + 32)) {
			return;
		}
	} else {
		if (ipin >= (DGPIO32_INT_NUM + 32)) {
			return;
		}
	}

	if (ipin < 32) {
		GPIO_SETREG(GPIO_INT_STS_REG_OFS, (1 << ipin));
	} else {
		ipin -= 32;
		GPIO_SETREG(GPIO_DINT_STS_REG_OFS, (1 << ipin));
	}
}

/**
    Get GPIO interrupt pin status.

    @param[in] ipin GPIO Interrupt pin:
                    - GPIO_INT_00 ~ GPIO_INT_40

    @return
        - @b TRUE: interrupt pending for ipin
        - @b FALSE: interrupt is NOT pending for ipin
*/
UINT32 gpio_getIntStatus(UINT32 ipin)
{
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (ipin >= (DGPIO_INT_NUM + 32)) {
			return 0;
		}
	} else {
		if (ipin >= (DGPIO32_INT_NUM + 32)) {
			return 0;
		}
	}

	if (ipin < 32) {
		return (GPIO_GETREG(GPIO_INT_STS_REG_OFS) & (1 << ipin)) > 0;
	} else {
		ipin -= 32;
		return (GPIO_GETREG(GPIO_DINT_STS_REG_OFS) & (1 << ipin)) > 0;
	}
}

/**
    Set GPIO interrupt type and polarity.

    @param[in] ipin GPIO Interrupt pin:
                    - GPIO_INT_00 ~ GPIO_INT_40
    @param[in] type GPIO interrupt trigger type
                    - @b GPIO_INTTYPE_EDGE: edge trigger
                    - @b GPIO_INTTYPE_LEVEL: level trigger
    @param[in] pol GPIO interrupt trigger polarity:
                    - @b GPIO_INTPOL_POSHIGH
                    - @b GPIO_INTPOL_NEGLOW
                    - @b GPIO_INTPOL_BOTHEDGE (valid when type is GPIO_INTTYPE_EDGE)

    @return void
*/
void gpio_setIntTypePol(UINT32 ipin, GPIO_INTTYPE type, GPIO_INTPOL pol)
{
	REGVALUE    RegData;
	unsigned long flags;

	//race condition protect. enter critical section
	//loc_cpu(flags);
	loc_multiCores(flags);

	if (ipin < 32) {
		RegData = GPIO_GETREG(GPIO_INT_TYPE_REG_OFS);
		if (type) {
			RegData |= (1 << ipin);
		} else {
			RegData &= ~(1 << ipin);
		}
		GPIO_SETREG(GPIO_INT_TYPE_REG_OFS, RegData);

		RegData = GPIO_GETREG(GPIO_INT_POL_REG_OFS);
		if (pol) {
			RegData |= (1 << ipin);
		} else {
			RegData &= ~(1 << ipin);
		}
		GPIO_SETREG(GPIO_INT_POL_REG_OFS, RegData);

		RegData = GPIO_GETREG(GPIO_INT_EDGE_TYPE_REG_OFS);
		if ((type == GPIO_INTTYPE_EDGE) && (pol == GPIO_INTPOL_BOTHEDGE)) {
			RegData |= (1 << ipin);
		} else {
			RegData &= ~(1 << ipin);
		}
		GPIO_SETREG(GPIO_INT_EDGE_TYPE_REG_OFS, RegData);
	} else {
		ipin -= 32;

		RegData = GPIO_GETREG(GPIO_DINT_TYPE_REG_OFS);
		if (type) {
			RegData |= (1 << ipin);
		} else {
			RegData &= ~(1 << ipin);
		}
		GPIO_SETREG(GPIO_DINT_TYPE_REG_OFS, RegData);

		RegData = GPIO_GETREG(GPIO_DINT_POL_REG_OFS);

		if (pol) {
			RegData |= (1 << ipin);
		} else {
			RegData &= ~(1 << ipin);
		}
		GPIO_SETREG(GPIO_DINT_POL_REG_OFS, RegData);

		RegData = GPIO_GETREG(GPIO_DINT_EDGE_TYPE_REG_OFS);
		if ((type == GPIO_INTTYPE_EDGE) && (pol == GPIO_INTPOL_BOTHEDGE)) {
			RegData |= (1 << ipin);
		} else {
			RegData &= ~(1 << ipin);
		}
		GPIO_SETREG(GPIO_DINT_EDGE_TYPE_REG_OFS, RegData);
	}

	//race condition protect. leave critical section
	//unl_cpu(flags);
	unl_multiCores(flags);
}

/**
    Set the interrupt service routine for specific interrupt source

    If you set some GPIO interrupt service routine in your code,
    you have to set the ISR to NULL when you don't want to use the
    GPIO anymore.

    @param[in] ipin     GPIO interrupt PIN
                    - GPIO_INT_00 ~ GPIO_INT_40
    @param[in] pHdl     User defined ISR for specific interrupt source

    @return void
*/
void gpio_setIntIsr(UINT32 ipin, DRV_CB pHdl)
{
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (ipin >= (DGPIO_INT_NUM + 32)) {
			return;
		}
	} else {
		if (ipin >= (DGPIO32_INT_NUM + 32)) {
			return;
		}
	}

	if (ipin < 32) {
		if (pHdl != NULL) {
			uiGPIOIsrFlag |= 1 << ipin;
		} else {
			uiGPIOIsrFlag &= ~(1 << ipin);
		}
#if (DRV_SUPPORT_IST == ENABLE && _EMULATION_ == DISABLE)
		pfIstCB_GPIO[ipin]  = pHdl;
#else
		fpGPIOIsr[ipin]     = pHdl;
#endif
	} else {
		ipin -= 32;
		if (pHdl != NULL) {
			uiDGPIOIsrFlag |= 1 << ipin;
		} else {
			uiDGPIOIsrFlag &= ~(1 << ipin);
		}
#if (DRV_SUPPORT_IST == ENABLE && _EMULATION_ == DISABLE)
		if (nvt_get_chip_id() == CHIP_NA51055)
			pfIstCB_DGPIO[ipin] = pHdl;
		else
			pfIstCB_DGPIO32[ipin] = pHdl;
			
#else
		if (nvt_get_chip_id() == CHIP_NA51055)
			fpDGPIOIsr[ipin]    = pHdl;
		else
			fpDGPIO32Isr[ipin]    = pHdl;
#endif
	}
}

/**
	Enable GPIO destination pin.

	For example, gpio_enableInt(GPIO_INT_00); or gpio_enableInt(GPIO_INT_USBWAKEUP);

	@note Corresponding GPIO PIN should be set to output mode (by gpio_setDir()).
	Once GPIO PIN is set to input mode, any transition on this PAD will pass to GPIO interrupt circuit (even corresponding pinmux is set to module)

	@param[in] ipin enabled GPIO destination pin:
			- GPIO_INT_00 ~ GPIO_INT_40

	@return void
*/
void  gpio_enableDestination(UINT32 ipin)
{
	REGVALUE    RegData;
	unsigned long flags;

	if (nvt_get_chip_id() == CHIP_NA51055) 
		return;

	if (ipin >= (DGPIO32_INT_NUM + 32)) {
		return;
	}

	loc_multiCores(flags);

	if (ipin < 32) {
		RegData  = GPIO_GETREG(GPIO_DESTINATION_REG_OFS);
		RegData |= (1 << ipin);
		GPIO_SETREG(GPIO_DESTINATION_REG_OFS, RegData);
	} else {
		ipin -= 32;

		RegData  = GPIO_GETREG(GPIO_D_DESTINATION_REG_OFS);
		RegData |= (1 << ipin);
		GPIO_SETREG(GPIO_D_DESTINATION_REG_OFS, RegData);
	}

	//race condition protect. leave critical section
	//unl_cpu(flags);
	unl_multiCores(flags);
}

/**
	Disable GPIO disnation pin.

	@param[in] ipin disabled GPIO destination pin:
		- GPIO_INT_00 ~ GPIO_INT_40
	
	@return void
*/
void  gpio_disableDestination(UINT32 ipin)
{
	REGVALUE    RegData;
	unsigned long flags;

	if (nvt_get_chip_id() == CHIP_NA51055) 
		return;

	if (ipin >= (DGPIO32_INT_NUM + 32)) {
		return;
	}

	//race condition protect. enter critical section
	//loc_cpu(flags);
	loc_multiCores(flags);

	if (ipin < 32) {
		RegData  = GPIO_GETREG(GPIO_DESTINATION_REG_OFS);
		RegData &= ~(1 << ipin);
		GPIO_SETREG(GPIO_DESTINATION_REG_OFS, RegData);
	} else {
		ipin -= 32;

		RegData  = GPIO_GETREG(GPIO_D_DESTINATION_REG_OFS);
		RegData &= ~(1 << ipin);
		GPIO_SETREG(GPIO_D_DESTINATION_REG_OFS, RegData);
	}

	//race condition protect. leave critical section
	//unl_cpu(flags);
	unl_multiCores(flags);
}

//@} //GPIO interrupt configuration

//@}


void gpio_drv_do_tasklet(unsigned long event)
{
#if (DRV_SUPPORT_IST == ENABLE && _EMULATION_ == DISABLE)
	UINT32 i, events;
	unsigned long flags;

	loc_cpu(flags);
	events = event;
	unl_cpu(flags);

	do {
		i = __builtin_ctz(events);
		events &= ~(1 << i);

		if (pfIstCB_GPIO[i] != NULL) {
			pfIstCB_GPIO[i](0);
		} else {
			printf("%s: %ld is NULL, event 0x%lx\r\n",
				__func__,
				i, events);
		}
	} while (events);
#endif
}

void dgpio_drv_do_tasklet(unsigned long event)
{
#if (DRV_SUPPORT_IST == ENABLE && _EMULATION_ == DISABLE)
	UINT32 i, events;
	unsigned long flags;

	loc_cpu(flags);
	events = event;
	unl_cpu(flags);

	do {
		i = __builtin_ctz(events);
		events &= ~(1 << i);

		if (nvt_get_chip_id() == CHIP_NA51055) {
			if (pfIstCB_DGPIO[i] != NULL) {
				pfIstCB_DGPIO[i](0);
			} else {
				printf("%s: %ld is NULL, event 0x%lx\r\n",
					__func__,
					i, events);
			}
		} else {
			if (pfIstCB_DGPIO32[i] != NULL) {
				pfIstCB_DGPIO32[i](0);
			} else {
				printf("%s: %ld is NULL, event 0x%lx\r\n",
					__func__,
					i, events);
			}
		}
	} while (events);
#endif
}

int gpio_get_value(UINT32 pin)
{
	return gpio_getPin(pin);
}

void gpio_set_value(UINT32 pin, int value)
{
	if (value)
		gpio_setPin(pin);
	else
		gpio_clearPin(pin);
}

void gpio_direction_input(UINT32 pin)
{
	gpio_setDir(pin, GPIO_DIR_INPUT);
}

void gpio_direction_output(UINT32 pin, int value)
{
	gpio_setDir(pin, GPIO_DIR_OUTPUT);

	gpio_set_value(pin, value);
}

irq_bh_handler_t gpio_bh_ist(int irq, unsigned long event, void *data)
{
	gpio_drv_do_tasklet(event);

	return (irq_bh_handler_t) IRQ_HANDLED;
}

irq_bh_handler_t dgpio_bh_ist(int irq, unsigned long event, void *data)
{
	dgpio_drv_do_tasklet(event);

	return (irq_bh_handler_t) IRQ_HANDLED;
}


void gpio_platform_init(void)
{
	cre_flg(&FLG_ID_GPIO, NULL, "gpio_flag");
	cre_flg(&FLG_ID_GPIO2, NULL, "dgpio_flag");

	request_irq(INT_ID_GPIO, gpio_isr, IRQF_TRIGGER_HIGH, "gpio", 0);
	request_irq_bh(INT_ID_GPIO, (irq_bh_handler_t) gpio_bh_ist, IRQF_BH_PRI_HIGH);
	request_irq_bh(INT_ID_DUMMY_DGPIO, (irq_bh_handler_t) dgpio_bh_ist, IRQF_BH_PRI_HIGH);
}

void gpio_platform_uninit(void)
{
	free_irq(INT_ID_GPIO, NULL);
	rel_flg(FLG_ID_GPIO);
}

//@}

