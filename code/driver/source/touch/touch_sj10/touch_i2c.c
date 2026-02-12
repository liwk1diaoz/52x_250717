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
#include "touch_sj10.h"

#define MALLOC(x) kzalloc((x), GFP_KERNEL)
#define FREE(x) kfree((x))
#else
#include "touch_sj10.h"
#include <stdlib.h>
#include <string.h>

#define MALLOC(x) malloc((x))
#define FREE(x) free((x))
#endif

#define __MODULE__          touch_i2c
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#include "kwrap/debug.h"

static struct i2c_board_info touch_i2c_device = {
	.type = TOUCH_I2C_NAME,
	.addr = TOUCH_I2C_ADDR,
};


typedef struct {
	struct i2c_client  *iic_client;
	struct i2c_adapter *iic_adapter;
} TOUCH_I2C_INFO;

static TOUCH_I2C_INFO *touch_i2c_info;

static const struct i2c_device_id touch_i2c_id[] = {
	{ TOUCH_I2C_NAME, 0 },
	{ }
};
static int touch_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	touch_i2c_info = NULL;
	touch_i2c_info = MALLOC(sizeof(TOUCH_I2C_INFO));
	if (touch_i2c_info == NULL) {
		DBG_ERR("%s fail: MALLOC not OK.\n", __FUNCTION__);
		return E_SYS;
	}

	touch_i2c_info->iic_client  = client;
	touch_i2c_info->iic_adapter = client->adapter;

	i2c_set_clientdata(client, touch_i2c_info);
	//printk(KERN_ERR "touch_i2c_probe!\r\n");
	DBG_IND("touch_i2c_probe!\r\n");

	return 0;
}

static int touch_i2c_remove(struct i2c_client *client)
{
	FREE(touch_i2c_info);
	touch_i2c_info = NULL;
	//printk(KERN_ERR "touch_i2c_remove!\r\n");
	DBG_IND("touch_i2c_remove!\r\n");
	return 0;
}

static struct i2c_driver touch_i2c_driver = {
	.driver = {
		.name  = TOUCH_I2C_NAME,
		.owner = THIS_MODULE,
	},
	.probe    = touch_i2c_probe,
	.remove   = touch_i2c_remove,
	.id_table = touch_i2c_id
};

ER touch_i2c_init_driver(UINT32 i2c_id)
{
	ER ret = E_OK;

	if (i2c_new_device(i2c_get_adapter(i2c_id), &touch_i2c_device) == NULL) {
		DBG_ERR("%s fail: i2c_new_device not OK.\n", __FUNCTION__);
		//printk(KERN_ERR "%s fail: i2c_new_device not OK.\n", __FUNCTION__);
	}
	if (i2c_add_driver(&touch_i2c_driver) != 0) {
		DBG_ERR("%s fail: i2c_add_driver not OK.\n", __FUNCTION__);
		//printk(KERN_ERR "%s fail: i2c_add_driver not OK.\n", __FUNCTION__);
	}

	return ret;
}

void touch_i2c_remove_driver(UINT32 id)
{
	i2c_unregister_device(touch_i2c_info->iic_client);
	i2c_del_driver(&touch_i2c_driver);
}

INT32 touch_i2c_transfer(struct i2c_msg *msgs, INT32 num)
{
	int ret;

	if (unlikely(touch_i2c_info->iic_adapter == NULL)) {
		DBG_ERR("%s fail: touch_i2c_info->ii2c_adapter not OK\n", __FUNCTION__);
		//printk(KERN_ERR "%s fail: pmu_i2c_info->ii2c_adapter not OK\n", __FUNCTION__);
		return -1;
	}

#if 0
	if (unlikely(i2c_transfer(touch_i2c_info->iic_adapter, msgs, num) != num)) {
		DBG_ERR("%s fail: i2c_transfer not OK \n", __FUNCTION__);
		return -1;
	}
#else
	ret = i2c_transfer(touch_i2c_info->iic_adapter, msgs, num);
	if (ret != num) {
		DBG_ERR("%s fail: i2c_transfer not OK, ret %d\n", __FUNCTION__, ret);
		//printk(KERN_ERR "%s fail: i2c_transfer not OK, ret %d\n", __FUNCTION__, ret);
		return -1;
	}
#endif

	return 0;
}
