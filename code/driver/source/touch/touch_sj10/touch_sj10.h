#ifndef _TOUCH_GT911_H
#define _TOUCH_GT911_H

#if 0
#include "Type.h"
#include "IOInit.h"
#include "pad.h"
#include "top.h"
#include "gpio.h"
#include "adc.h"
#include "pwm.h"
#endif

#ifdef __KERNEL__
#include <linux/delay.h>
#include "kwrap/type.h"
#include "kwrap/flag.h"
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/io.h>
#else
#include "kwrap/type.h"
#include "kwrap/flag.h"
#include "rtos_na51089/kdrv_i2c.h"
#endif

#define TOUCH_I2C_NAME			"touch_gt911"
#define TOUCH_I2C_ADDR			(0x70>>1)

#define TOUCH_ID_MAX		1


typedef enum _TOUCH_I2C_ID {
	TOUCH_I2C_ID_1 = 0,
	TOUCH_I2C_ID_2 = 1,
	TOUCH_I2C_ID_3 = 2,
	TOUCH_I2C_ID_4 = 3,
	TOUCH_I2C_ID_5 = 4,
	ENUM_DUMMY4WORD(TOUCH_I2C_ID)
} TOUCH_I2C_ID;

typedef struct _TOUCH_I2C {
	TOUCH_I2C_ID id;
	UINT32 addr;
} TOUCH_I2C;

ER touch_i2c_init_driver(UINT32 i2c_id);
void touch_i2c_remove_driver(UINT32 id);
INT32 touch_i2c_transfer(struct i2c_msg *msgs, INT32 num);

#endif

