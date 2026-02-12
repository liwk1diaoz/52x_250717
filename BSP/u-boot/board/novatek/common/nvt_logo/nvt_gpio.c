
#include <common.h>
#include <command.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <part.h>
#include <asm/hardware.h>
#include <asm/nvt-common/nvt_types.h>
#include <asm/nvt-common/nvt_common.h>
#include <asm/nvt-common/shm_info.h>
#include <stdlib.h>
#include <linux/arm-smccc.h>
#include "nvt_display_common.h"
#include "nvt_gpio.h"
#include <asm/arch/display.h>
#include <asm/arch/top.h>
#include <linux/libfdt.h>
#include <asm/arch/gpio.h>

/**
    Set GPIO pin direction.

    Set specified GPIO pin to input or output mode.

    @param[in] pin GPIO pin number.
    @param[in] dir GPIO direction
                - @b GPIO_DIR_INPUT: set pin to input mode
                - @b GPIO_DIR_OUTPUT: set pin to output mode

    @return void
*/
void nvt_gpio_setDir(UINT32 pin, GPIO_DIR dir)
{
	REGVALUE    RegData;
	UINT32      ofs = (pin >> 5) << 2;
	unsigned long flags;

	pin &= (32 - 1);

	RegData = GPIO_GETREG(GPIO_STRG_DIR_REG_OFS + ofs);

	if (dir) {
		RegData |= (1 << pin);    //output
	} else {
		RegData &= ~(1 << pin);    //input
	}

	GPIO_SETREG(GPIO_STRG_DIR_REG_OFS + ofs, RegData);
}

/**
    Get GPIO pin direction.

    Get input/output mode of specified GPIO pin.

    @param[in] pin GPIO pin number.

    @return
        - @b GPIO_DIR_INPUT: pin is under input mode
        - @b GPIO_DIR_OUTPUT: pin is under output mode
*/
GPIO_DIR nvt_gpio_getDir(UINT32 pin)
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
void nvt_gpio_setPin(UINT32 pin)
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
void nvt_gpio_clearPin(UINT32 pin)
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
UINT32 nvt_gpio_getPin(UINT32 pin)
{
	UINT32 tmp;
	UINT32 ofs = (pin >> 5) << 2;

	pin &= (32 - 1);
	tmp = (1 << pin);

	return (GPIO_GETREG(GPIO_STRG_DAT_REG_OFS + ofs) & tmp) != 0;
}
