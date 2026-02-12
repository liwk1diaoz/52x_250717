/*
    General Purpose I/O controller

    General Purpose I/O controller

    @file       gpio.c
    @ingroup    mIDrvIO_GPIO
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "io/gpio.h"
#include "gpio_reg.h"
#include "kwrap/type.h"
#include <kwrap/nvt_type.h>
#include <kwrap/flag.h>
#include <kwrap/debug.h>

#define GPIO_INT_NUM                (32)
#define DGPIO_INT_NUM               (17)

#define GPIO_VAL_H        "1"
#define GPIO_VAL_L        "0"

#define GPIO_DIR_IN       "in"
#define GPIO_DIR_OUT      "out"

//
//  gpio register access definition
//
//#define GPIO_REG_ADDR(ofs)       (IOADDR_GPIO_REG_BASE+(ofs))
//#define GPIO_GETREG(ofs)         INW(GPIO_REG_ADDR(ofs))
//#define GPIO_SETREG(ofs,value)   OUTW(GPIO_REG_ADDR(ofs), (value))


/**
    @addtogroup mIDrvIO_GPIO
*/
//@{
static UINT32  uiGPIOOpened = FALSE;


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
	/*FLGPTN uiFlag;

	if (gpioIntID < GPIO_INT_32) {
		return  wai_flg(&uiFlag, FLG_ID_GPIO, FLGPTN_BIT(gpioIntID), TWF_ORW | TWF_CLR);
	} else {
		return  wai_flg(&uiFlag, FLG_ID_GPIO2, FLGPTN_BIT(gpioIntID - GPIO_INT_32), TWF_ORW | TWF_CLR);
	}*/
	DBG_ERR("Not support\tr\n");

	return E_OK;
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
	//clr_flg(FLG_ID_GPIO, FLGPTN_BIT_ALL);
	//clr_flg(FLG_ID_GPIO2, FLGPTN_BIT_ALL);

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
	int fd;
	char cmd[40] = {0};

	sprintf(cmd, "%d", (int)pin);

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if(fd > 0) {
		write(fd, cmd, strlen(cmd)+1);
		close(fd);
	} else {
		DBG_ERR("open export failed\r\n");
		return;
	}

	sprintf(cmd, "/sys/class/gpio/gpio%d/direction", (int)pin);

	fd = open(cmd, O_WRONLY);
	if(fd > 0)  {
		if (dir == GPIO_DIR_INPUT) {
			write(fd, GPIO_DIR_IN, sizeof(GPIO_DIR_IN));
		} else {
			write(fd, GPIO_DIR_OUT, sizeof(GPIO_DIR_OUT));
		}
		close(fd);
	} else {
		DBG_ERR("open direction failed\r\n");
	}
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
	int fd;
	char cmd[40] = {0};
	char dir[16] = {0};
	int len;
	char in[4] = {'i', 'n', '\n', 0};

	sprintf(cmd, "%d", (int)pin);

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if(fd > 0) {
		write(fd, cmd, strlen(cmd));
		close(fd);
	} else {
		DBG_ERR("open export failed\r\n");
		return 0;
	}

	sprintf(cmd, "/sys/class/gpio/gpio%d/direction", (int)pin);

	fd = open(cmd, O_RDONLY);
	if(fd > 0) {
		if(lseek(fd,0,SEEK_SET)==-1)
			DBG_ERR("lseek failed!\n");

		if((len = read(fd, dir, sizeof(dir))) == -1) {
			DBG_ERR("read failed!\n");
			close(fd);
			return -1;
		}

		dir[len] = 0;

		close(fd);
	} else {
		DBG_ERR("open direction failed\r\n");
		return 0;
	}

	if (strcmp(dir, in) == 0) {
		return GPIO_DIR_INPUT;
	} else {
		return GPIO_DIR_OUTPUT;
	}
}

/**
    Set GPIO pin.

    Set specified GPIO pin to output HIGH.

    @param[in] pin GPIO pin number.

    @return void
*/
void gpio_setPin(UINT32 pin)
{
	int fd;
	char cmd[40] = {0};

	sprintf(cmd, "%d", (int)pin);

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if(fd > 0) {
		write(fd, cmd, strlen(cmd)+1);
		close(fd);
	} else {
		DBG_ERR("open export failed\r\n");
		return;
	}

	sprintf(cmd, "/sys/class/gpio/gpio%d/value", (int)pin);

	fd = open(cmd, O_WRONLY);
	if(fd > 0)  {
		write(fd, GPIO_VAL_H, sizeof(GPIO_VAL_H));
		close(fd);
	} else {
		DBG_ERR("open value failed\r\n");
	}
}

/**
    Clear GPIO pin.

    Clear specified GPIO pin to output LOW.

    @param[in] pin GPIO pin number.

    @return void
*/
void gpio_clearPin(UINT32 pin)
{
	int fd;
	char cmd[40] = {0};

	sprintf(cmd, "%d", (int)pin);

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if(fd > 0) {
		write(fd, cmd, strlen(cmd)+1);
		close(fd);
	} else {
		DBG_ERR("open export failed\r\n");
		return;
	}

	sprintf(cmd, "/sys/class/gpio/gpio%d/value", (int)pin);

	fd = open(cmd, O_WRONLY);
	if(fd > 0)  {
		write(fd, GPIO_VAL_L, sizeof(GPIO_VAL_L));
		close(fd);
	} else {
		DBG_ERR("open value failed\r\n");
	}
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
	int fd;
	char cmd[40] = {0};
	char value[16] = {0};
	int len;
	char high[3] = {'1', '\n', 0};

	sprintf(cmd, "%d", (int)pin);

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if(fd > 0) {
		write(fd, cmd, strlen(cmd));
		close(fd);
	} else {
		DBG_ERR("open export failed\r\n");
		return 0;
	}

	sprintf(cmd, "/sys/class/gpio/gpio%d/value", (int)pin);

	fd = open(cmd, O_RDONLY);
	if(fd > 0) {
		if(lseek(fd,0,SEEK_SET)==-1)
			DBG_ERR("lseek failed!\n");

		if((len = read(fd, value, sizeof(value))) == -1) {
			DBG_ERR("read failed!\n");
			close(fd);
			return -1;
		}

		value[len] = 0;

		close(fd);
	} else {
		DBG_ERR("open value failed\r\n");
		return 0;
	}

	if (strcmp(value, high) == 0) {
		return 1;
	} else {
		return 0;
	}
}

/**
    Set GPIO pin through data registers.

    Set specified GPIO pin to output HIGH through data registers.

    @param[in] pin GPIO pin number.

    @return void
*/
void  gpio_pullSet(UINT32 pin)
{
	DBG_ERR("Not support\tr\n");
}

/**
    Clear GPIO pin through data registers.

    Clear specified GPIO pin to output LOW through data registers.

    @param[in] pin GPIO pin number.

    @return void
*/
void  gpio_pullClear(UINT32 pin)
{
	DBG_ERR("Not support\tr\n");
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
	return 0;
}

/**
    Write GPIO data register.

    @param[in] dataidx data register index (0,1,2,3,4,5,6,7)
    @param[in] value register word value to write to data register

    @return void
*/
void gpio_writeData(UINT32 dataidx, UINT32 value)
{
	DBG_ERR("Not support\tr\n");
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
	DBG_ERR("Not support\tr\n");
	return 0;
}

/**
    Write DGPIO data register.

    @param[in] value register word value to write to data register

    @return void
*/
void dgpio_writeData(UINT32 value)
{
	DBG_ERR("Not support\tr\n");
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
	DBG_ERR("Not support\tr\n");
}

/**
    Disable GPIO interrupt pin.

    @param[in] ipin disabled GPIO Interrupt pin:
                    - GPIO_INT_00 ~ GPIO_INT_40

    @return void
*/
void  gpio_disableInt(UINT32 ipin)
{
	DBG_ERR("Not support\tr\n");
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
	DBG_ERR("Not support\tr\n");

	return 0;
}

/**
    Clear GPIO interrupt status.

    @param[in] ipin GPIO Interrupt pin:
                    - GPIO_INT_00 ~ GPIO_INT_40

    @return void
*/
void gpio_clearIntStatus(UINT32 ipin)
{
	DBG_ERR("Not support\tr\n");
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
	DBG_ERR("Not support\tr\n");
	return 0;
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
	DBG_ERR("Not support\tr\n");
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

void gpio_platform_init(void)
{
}

void gpio_platform_uninit(void)
{
}

//@}

