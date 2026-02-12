#ifdef __KERNEL__
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <kwrap/dev.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>

#include "touch_sj10.h"
#else
//#include "kwrap/type.h"
//#include "kwrap/semaphore.h"
//#include "kwrap/flag.h"
#include "touch_sj10.h"
#include <stdio.h>
#include <string.h>
#include "plat/gpio.h"
#include "touch_dtsi.h"
#endif

#include "kwrap/util.h"
#include <touch_inc.h>

//#include "SwTimer.h"
//#include <stdlib.h>
//#include "ErrorNo.h"
//#include "Delay.h"
#if 1
//#include <compiler.h>
//#include <rtosfdt.h>
//#include <libfdt.h>
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "touch_common.h"
#include "touch_dbg.h"
#include "kwrap/util.h"

#endif

#if 0
#define __MODULE__			touch_gt911
#define __DBGLVL__			2      // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__			"*"    //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
#endif
static TOUCH_I2C touch_i2c = {
	TOUCH_I2C_ID_3, TOUCH_I2C_ADDR
};

static TOUCH_DRV_INFO g_tp_drv_info[TOUCH_ID_MAX] = {
	{
	.tp_hw.i2c_cfg.i2c_id = TOUCH_I2C_ID_3,
	.tp_hw.i2c_cfg.i2c_addr = TOUCH_I2C_ADDR,
	.tp_hw.gpio_cfg.gpio_pwr = (12+64),/* no use io*/
	.tp_hw.gpio_cfg.gpio_rst = (4 +32),  /*p_gpio_4*/
	.tp_hw.gpio_cfg.gpio_int = 9,/*c_gpio_9*/
	.tp_hw.reset_time.rst_time = 50,
	},
};

volatile  UINT32 P1_CoordRecvdX =0;
volatile UINT32 P1_CoordRecvdY =0;
UINT8 TP_RELEASE_STATUS=0;



TOUCH_DRV_INFO* mdrv_get_touch_info(UINT32 id)
{
	id = TOUCH_MIN(id, TOUCH_ID_MAX-1);

	return &g_tp_drv_info[id];
}


/*
static INT32 touch_i2c_write(UINT32 addr, UINT32 data)
{
	struct i2c_msg msgs;
	UINT8 buf[3];
	INT32 ret;

	buf[0]     = (addr >> 8) & 0xFF;//(u8Reg>>8) & 0xff;
	buf[1]     = addr & 0xFF;
	buf[2]     = data & 0xFF;
	msgs.addr  = touch_i2c.addr;
	msgs.flags = 0;//w
	msgs.len   = 3;
	msgs.buf   = buf;

	ret = touch_i2c_transfer(&msgs, 1);
	return ret;
}*/

static UINT32 touch_i2c_read(UINT32 addr, UINT8 *data)
{
	struct i2c_msg msgs[2];
	UINT8 buf[1], buf2[1];
	INT32 ret;

//	buf[0]        = (addr >> 8) & 0xFF;//(u8Reg>>8) & 0xff;
	buf[0]        = addr & 0xFF;
	msgs[0].addr  = touch_i2c.addr;
	msgs[0].flags = 0;//w
	msgs[0].len   = 1;
	msgs[0].buf   = buf;

	buf2[0]       = 0;
	msgs[1].addr  = touch_i2c.addr;
	msgs[1].flags = 1;//r
	msgs[1].len   = 1;
	msgs[1].buf   = buf2;

	ret = touch_i2c_transfer(msgs, 2);
	if (ret == 0) { //OK
		*data = buf2[0];
	}
	else
	{
		buf2[0] = 0;
		*data = buf2[0];
	}

	return ret;
}
int touch_init_gt911(void)
{
	int ret;

	ret = touch_i2c_init_driver(touch_i2c.id);



	return ret;
}
BOOL DetTP_IsPenDown(void)
{
	UINT8 recv_data=0;
	BOOL pendown=FALSE;

	touch_i2c_read(0x03, &recv_data);
	TP_RELEASE_STATUS=(recv_data & 0xc0)>>6;
	/*tp press:TP_RELEASE_STATUS=2
	    tp release:TP_RELEASE_STATUS=1;
	*/

	pendown=(TP_RELEASE_STATUS&0x2)>>1;

	return pendown;
}

#if 1
#define TP_PRESSURE_TH      (320)     // 255 - 10

#define TP_HW_MAX_X         (720)
#define TP_HW_MAX_Y         (480)
#define TP_HW_MIN_X         (0)
#define TP_HW_MIN_Y         (0)

#define TP_PANEL_X1         (0)
#define TP_PANEL_Y1         (0)
#define TP_PANEL_X2         (719)
#define TP_PANEL_Y2         (479)

#define TP_PANEL_W          (TP_PANEL_X2-TP_PANEL_X1 + 1)
#define TP_PANEL_H          (TP_PANEL_Y2-TP_PANEL_Y1 + 1)

//Touch Panel recieved data structure // 2 points touch
typedef struct
{
    UINT32 P1_TouchStatus;
    UINT32 P1_TouchEventID;
    UINT32 P1_CoordRecvdX;
    UINT32 P1_CoordRecvdY;
    UINT32 P2_TouchStatus;
    UINT32 P2_TouchEventID;
    UINT32 P2_CoordRecvdX;
    UINT32 P2_CoordRecvdY;
} TP_RECV_DATA, *pPT_RECV_DATA;

//////////////////////////////////////////////////////////////////////

static TP_RECV_DATA     g_TPRecvdData;
static pPT_RECV_DATA    pTPRecvdData = &g_TPRecvdData;
//static TP_INFO          g_TpInfo;

/*
    0:  Use AUTO Mode with Polling
    1:  Use AUTO Mode with Interrupt (N/A)
    2:  Use AUTO Mode with FIFO Threshold (N/A)
*/
#define GSTP_USED_MODE      0


static void DetTP_Convert2RealXY(UINT32 *pX, UINT32 *pY)
{

    INT32 tempX=0,tempY=0;
    tempX = *pX;
    tempY = *pY;
    if (tempX > TP_HW_MAX_X)
        tempX = TP_HW_MAX_X;
    else if (tempX < TP_HW_MIN_X)
        tempX = TP_HW_MIN_X;

    if (tempY > TP_HW_MAX_Y)
        tempY = TP_HW_MAX_Y;
    else if (tempY < TP_HW_MIN_Y)
        tempY = TP_HW_MIN_Y;

    *pX = tempX;
    *pY = TP_PANEL_H - tempY - 1;
    //DBG_WRN("Convert:(%d,%d) to (%d,%d)\r\n", tempX, tempY, *pX, *pY);

}

static void DetTP_GetData(pPT_RECV_DATA pData)
{
	UINT8 recv_data[8] = {0};

	touch_i2c_read(0x03, &recv_data[0]);
	touch_i2c_read(0x04, &recv_data[1]);
	touch_i2c_read(0x05, &recv_data[2]);
	touch_i2c_read(0x06, &recv_data[3]);

	pData->P1_TouchStatus = 1;
	{
		pData->P1_TouchEventID  	= 1 ; //recv_data[3] >> 6 ; //event flag
		pData->P1_CoordRecvdX		= (recv_data[2] & 0x0f) << 8 | recv_data[3];
		pData->P1_CoordRecvdY		= (recv_data[0] & 0x0f) << 8 | recv_data[1];

	}
	DetTP_Convert2RealXY(&(pData->P1_CoordRecvdX), &(pData->P1_CoordRecvdY));

	pData->P2_TouchStatus   	= 0;
	pData->P2_TouchEventID  	= 0;
	pData->P2_CoordRecvdX   	= 0;
	pData->P2_CoordRecvdY   	= 0;

}

void DetTP_GetXY(INT32 *pX, INT32 *pY)
{

    memset(pTPRecvdData, 0, sizeof(TP_RECV_DATA));
    DetTP_GetData(pTPRecvdData);
    if (pTPRecvdData->P1_TouchEventID == 1)
    {
        *pX = pTPRecvdData->P1_CoordRecvdX;
        *pY = pTPRecvdData->P1_CoordRecvdY;
    }
    else if (pTPRecvdData->P2_TouchEventID == 1)
    {
        *pX = pTPRecvdData->P2_CoordRecvdX;
        *pY = pTPRecvdData->P2_CoordRecvdY;
    }
    else
    {
        *pX = -1;
        *pY = -1;
    }
}

int DetTP_Init(void)
{
	#if 1//touch_dtsi_FROM_FILE
	TOUCH_DRV_INFO *ptouch_info = mdrv_get_touch_info(0);
	//UINT32 tp_reg_adr = 0;
	//UINT32 i = 0;
	//UINT8 check_sum = 0;
	#endif
	int ret;
	//touch_dtsi_FROM_FILE

	DBG_IND("touch load dtsi file success\r\n");
	touch_i2c.id = ptouch_info->tp_hw.i2c_cfg.i2c_id;
	touch_i2c.addr = ptouch_info->tp_hw.i2c_cfg.i2c_addr;
	//gpio_direction_output(ptouch_info->tp_hw.gpio_cfg.gpio_pwr, 1);
	gpio_direction_output(ptouch_info->tp_hw.gpio_cfg.gpio_int,0);
	//vos_util_delay_ms(500);//电源上电时间太短TP报I2C错误
	gpio_direction_output(ptouch_info->tp_hw.gpio_cfg.gpio_rst, 0);
	vos_util_delay_ms(100);
	//gpio_set_value(ptouch_info->tp_hw.gpio_cfg.gpio_rst, 0);
	//vos_util_delay_ms(100);
	gpio_set_value(ptouch_info->tp_hw.gpio_cfg.gpio_rst, 1);
	vos_util_delay_ms(100);
	gpio_direction_input(ptouch_info->tp_hw.gpio_cfg.gpio_int);

	ret=touch_init_gt911();
	vos_util_delay_ms(20);
	return ret;
}
#endif


#if defined __FREERTOS

int touch_ioctl (int fd, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case IOC_TOUCH_PARKING_MODE:
		DBG_DUMP("touch_ioctl, IOC_TOUCH_PARKING_MODE %d, arg %d\r\n", IOC_TOUCH_PARKING_MODE, arg);
		break;

	case IOC_TOUCH_PARKING_SENSITIVITY:
		DBG_DUMP("touch_ioctl, IOC_TOUCH_PARKING_SENSITIVITY %d, arg %d\r\n", IOC_TOUCH_PARKING_SENSITIVITY, arg);
		break;

	default:
		DBG_ERR("unknown cmd 0x%x\r\n", cmd);
		return -1;
	}

	return 0;
}

int touch_init(void)
{
	int ret;

	ret = touch_init_gt911();

	return ret;
}

void touch_exit(void)
{
	touch_i2c_remove_driver(touch_i2c.id);
}


#elif defined __KERNEL__

static long touch_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch(cmd) {
		case IOC_TOUCH_ISPENDOWN:{
			BOOL pen_down;
			pen_down=DetTP_IsPenDown();
			copy_to_user((void *)arg, (void *)&pen_down, sizeof(BOOL));
		}
		break;
		case IOC_TOUCH_GET_XY_VALUE:{
			INT32 xyData[2] = {0};
			DetTP_GetXY(&xyData[0], &xyData[1]);
			copy_to_user((void *)arg, (void *)xyData, sizeof(xyData));
		}
		break;
		default:
		return -EINVAL;
	}
	return 0;
}

static const struct file_operations touch_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl	= touch_ioctl,
};

static struct miscdevice touch_miscdev = {
	.minor          = MISC_DYNAMIC_MINOR,
	.name           = "touch",
	.fops           = &touch_fops,
};

static int __init touch_init(void)
{
	int ret=E_OK;

	//ret = touch_init_gt911();
	ret=DetTP_Init();
	if (ret) {
		DBG_ERR("touch_init_gt911 fail, ret = %d\n", ret);
		return ret;
	}

	ret = misc_register(&touch_miscdev);
	if (ret) {
		DBG_ERR("misc_register fail, ret = %d\n", ret);
		return ret;
	}

	return 0;
}
static void __exit touch_exit(void)
{
	touch_i2c_remove_driver(touch_i2c.id);
}

module_init(touch_init);
module_exit(touch_exit);

MODULE_DESCRIPTION("GT911 touch");
MODULE_AUTHOR("Novatek Corp.");
MODULE_LICENSE("GPL");
#endif

