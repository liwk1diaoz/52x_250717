#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/random.h>
#include <asm/signal.h>

#include "test_drv.h"
#include "test_main.h"
#include "test_dbg.h"
#if 1

#include <linux/delay.h>
#include <mach/rcw_macro.h>
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "comm/hwclock.h"

#include <linux/i2c.h>
#include "kdrv_videocapture/kdrv_ssenif.h"


static unsigned int mode;
module_param_named(mode, mode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mode, "0 is HDR<default>. 1 is Non-HDR");



#define I2C_NAME "test_task"
#define I2C_ADDR 0x1A

static struct i2c_board_info test_i2c_device = {
	.type = I2C_NAME,
	.addr = I2C_ADDR,
};


typedef struct {
	struct i2c_client  *iic_client;
	struct i2c_adapter *iic_adapter;
} TEST_I2C_INFO;

static TEST_I2C_INFO *test_i2c_info;

static int test_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	test_i2c_info = NULL;
	test_i2c_info = kzalloc(sizeof(TEST_I2C_INFO), GFP_KERNEL);
	if (test_i2c_info == NULL) {
		printk("%s fail: kzalloc not OK.\n", __FUNCTION__);
		return -ENOMEM;
	}

	test_i2c_info->iic_client  = client;
	test_i2c_info->iic_adapter = client->adapter;

	i2c_set_clientdata(client, test_i2c_info);

	return 0;
}

static int test_i2c_remove(struct i2c_client *client)
{
	kfree(test_i2c_info);
	return 0;
}


static const struct i2c_device_id test_i2c_id[] = {
	{ I2C_NAME, 0 },
	{ }
};

static struct i2c_driver test_i2c_driver = {
	.driver = {
		.name  = I2C_NAME,
		.owner = THIS_MODULE,
	},
	.probe    = test_i2c_probe,
	.remove   = test_i2c_remove,
	.id_table = test_i2c_id
};

UINT32 test_sensor_read(UINT32 addre)
{
	struct i2c_msg  msgs[2];
	unsigned char   buf[2], buf2[2];

	buf[0]     = (addre>>8)  & 0xFF;
	buf[1]     = (addre>>0)  & 0xFF;

	msgs[0].addr  = I2C_ADDR;
	msgs[0].flags = 0;//w
	msgs[0].len   = 2;
	msgs[0].buf   = buf;

	buf2[0]       = 0;
	buf2[1]       = 0;

	msgs[1].addr  = I2C_ADDR;
	msgs[1].flags = 1;//r
	msgs[1].len   = 1;
	msgs[1].buf   = buf2;

	if (i2c_transfer(test_i2c_info->iic_adapter, msgs, 2) != 2) {
		printk("%s fail: i2c_transfer not OK \n", __FUNCTION__);
	}

	return (UINT32)(((UINT32)buf2[0]<<0));
}

void test_sensor_write(UINT32 addre, UINT32 value)
{
	struct i2c_msg  msgs;
	unsigned char   buf[4];

	buf[0]     = (addre>>8)  & 0xFF;
	buf[1]     = (addre>>0)  & 0xFF;
	buf[2]     = (value>>0) & 0xFF;
	//buf[3]     = (value>>0) & 0xFF;
	msgs.addr  = I2C_ADDR;
	msgs.flags = 0;//w
	msgs.len   = 3;
	msgs.buf   = buf;

	if (i2c_transfer(test_i2c_info->iic_adapter, &msgs, 1) != 1) {
		printk("%s fail: i2c_transfer not OK \n", __FUNCTION__);
	}

	//printk("test_sensor_read 0x%04X = 0x%04X\r\n",addre,test_sensor_read(addre));

}



#define emu_csi_imx291_write(x, y)  test_sensor_write((x), (y))




int nvt_test_thread(void *__test)
{

	/* Hook i2c driver */
	if (i2c_new_device(i2c_get_adapter(1), &test_i2c_device) == NULL) {
		DBG_ERR("%s fail: i2c_new_device not OK.\n", __FUNCTION__);
	}
	if (i2c_add_driver(&test_i2c_driver) != 0) {
		DBG_ERR("%s fail: i2c_add_driver not OK.\n", __FUNCTION__);
	}

	/* Config SIE-MCLK freq*/
	iowrite32(0x01011313, (void __iomem *)0xC0020030);

	/* Enable SIE-MCLK */
	iowrite32((ioread32((void __iomem *)0xC0020070)|(0x1<<2)), (void __iomem *)0xC0020070);
	msleep(10);

	/* release reset pin: S_GPIO_4 */
	iowrite32((ioread32((void __iomem *)0xC0070028)|(0x1<<4)), (void __iomem *)0xC0070028);
	iowrite32((0x1<<4), (void __iomem *)0xC0070068);
	msleep(10);
	iowrite32((0x1<<4), (void __iomem *)0xC0070048);
	msleep(100);


	{
		UINT32 handler;
		UINT32 wait;
		UINT32 curtime1, curtime2;

		if (kdrv_ssenif_open(0, KDRV_SSENIF_ENGINE_CSI0) == 0) {
			handler = KDRV_DEV_ID(0, KDRV_SSENIF_ENGINE_CSI0, 0);
		} else {
			DBG_ERR("kdrv_ssenif_open failed\r\n");
			return 0;
		}

		//{
			//PCSIOBJ p_csiobj;
			//p_csiobj = csi_get_drv_object(CSI_ID_CSI);
			//p_csiobj->set_config(CSI_CONFIG_ID_A_FORCE_CLK_HS, ENABLE);
		//}

		kdrv_ssenif_set(handler, KDRV_SSENIF_CSI_SENSORTYPE,		KDRV_SSENIF_SENSORTYPE_SONY_NONEHDR);
		kdrv_ssenif_set(handler, KDRV_SSENIF_CSI_SENSOR_TARGET_MCLK, 37125000);
		kdrv_ssenif_set(handler, KDRV_SSENIF_CSI_SENSOR_REAL_MCLK,	24000000);
		kdrv_ssenif_set(handler, KDRV_SSENIF_CSI_DLANE_NUMBER,		2);
		kdrv_ssenif_set(handler, KDRV_SSENIF_CSI_VALID_HEIGHT,		1080);

		kdrv_ssenif_set(handler, KDRV_SSENIF_CSI_CLANE_SWITCH,		KDRV_SSENIF_CLANE_CSI0_USE_C0);
		kdrv_ssenif_set(handler, KDRV_SSENIF_CSI_TIMEOUT_PERIOD,	1000);
		kdrv_ssenif_set(handler, KDRV_SSENIF_CSI_STOP_METHOD,		KDRV_SSENIF_STOPMETHOD_FRAME_END);

		/* Enable */
		kdrv_ssenif_trigger(handler, ENABLE);


{
		emu_csi_imx291_write(0x3002, 0x01);//Master mode stop
		msleep(20);
		emu_csi_imx291_write(0x3000, 0x01);// standby
		msleep(20);
		emu_csi_imx291_write(0x3005, 0x01);
		emu_csi_imx291_write(0x3007, 0x00);
		emu_csi_imx291_write(0x3009, 0x02);
		emu_csi_imx291_write(0x300A, 0xF0);
		emu_csi_imx291_write(0x300F, 0x00);
		emu_csi_imx291_write(0x3010, 0x21);
		emu_csi_imx291_write(0x3012, 0x64);
		emu_csi_imx291_write(0x3013, 0x00);
		emu_csi_imx291_write(0x3016, 0x09);
		emu_csi_imx291_write(0x3018, 0x65);//VMAX
		emu_csi_imx291_write(0x3019, 0x04);
		emu_csi_imx291_write(0x301c, 0x30);//HMAX
		emu_csi_imx291_write(0x301d, 0x11);

		emu_csi_imx291_write(0x3046, 0x01);
		emu_csi_imx291_write(0x304B, 0x0a);
		emu_csi_imx291_write(0x305C, 0x18);//INCK
		emu_csi_imx291_write(0x305D, 0x03);
		emu_csi_imx291_write(0x305E, 0x20);
		emu_csi_imx291_write(0x305F, 0x01);

		emu_csi_imx291_write(0x3070, 0x02);
		emu_csi_imx291_write(0x3071, 0x11);
		emu_csi_imx291_write(0x309B, 0x10);
		emu_csi_imx291_write(0x309C, 0x22);
		emu_csi_imx291_write(0x30A2, 0x02);
		emu_csi_imx291_write(0x30A6, 0x20);
		emu_csi_imx291_write(0x30A8, 0x20);
		emu_csi_imx291_write(0x30AA, 0x20);
		emu_csi_imx291_write(0x30AC, 0x20);
		emu_csi_imx291_write(0x30B0, 0x43);

		emu_csi_imx291_write(0x3119, 0x9E);
		emu_csi_imx291_write(0x311C, 0x1E);
		emu_csi_imx291_write(0x311E, 0x08);
		emu_csi_imx291_write(0x3128, 0x05);
		emu_csi_imx291_write(0x3129, 0x00);
		emu_csi_imx291_write(0x313D, 0x83);
		emu_csi_imx291_write(0x3150, 0x03);
		emu_csi_imx291_write(0x315E, 0x1A);//INCKSEL5
		emu_csi_imx291_write(0x3164, 0x1A);//INCKSEL6
		emu_csi_imx291_write(0x317C, 0x00);//ADBIT2
		emu_csi_imx291_write(0x317E, 0x00);
		emu_csi_imx291_write(0x31EC, 0x0E);


		emu_csi_imx291_write(0x32b8, 0x50);
		emu_csi_imx291_write(0x32b9, 0x10);
		emu_csi_imx291_write(0x32ba, 0x00);
		emu_csi_imx291_write(0x32bb, 0x04);
		emu_csi_imx291_write(0x32C8, 0x50);
		emu_csi_imx291_write(0x32C9, 0x10);
		emu_csi_imx291_write(0x32CA, 0x00);
		emu_csi_imx291_write(0x32CB, 0x04);


		emu_csi_imx291_write(0x332c, 0xD3);
		emu_csi_imx291_write(0x332d, 0x10);
		emu_csi_imx291_write(0x332e, 0x0D);
		emu_csi_imx291_write(0x3358, 0x06);
		emu_csi_imx291_write(0x3359, 0xE1);
		emu_csi_imx291_write(0x335A, 0x11);
		emu_csi_imx291_write(0x3360, 0x1E);
		emu_csi_imx291_write(0x3361, 0x61);
		emu_csi_imx291_write(0x3362, 0x10);
		emu_csi_imx291_write(0x33B0, 0x50);
		emu_csi_imx291_write(0x33B2, 0x1A);
		emu_csi_imx291_write(0x33B3, 0x04);


		emu_csi_imx291_write(0x3405, 0x10);
		emu_csi_imx291_write(0x3407, 0x03);
		emu_csi_imx291_write(0x3414, 0x0A);
		emu_csi_imx291_write(0x3418, 0x49);
		emu_csi_imx291_write(0x3419, 0x04);
		emu_csi_imx291_write(0x342C, 0x47);
		emu_csi_imx291_write(0x342D, 0x00);
		emu_csi_imx291_write(0x3430, 0x0F);
		emu_csi_imx291_write(0x3431, 0x00);
		emu_csi_imx291_write(0x3441, 0x0C);
		emu_csi_imx291_write(0x3442, 0x0C);
		emu_csi_imx291_write(0x3443, 0x01);
		emu_csi_imx291_write(0x3444, 0x20);
		emu_csi_imx291_write(0x3445, 0x25);
		emu_csi_imx291_write(0x3446, 0x57);
		emu_csi_imx291_write(0x3447, 0x00);
		emu_csi_imx291_write(0x3448, 0x37);
		emu_csi_imx291_write(0x3449, 0x00);
		emu_csi_imx291_write(0x344A, 0x1F);
		emu_csi_imx291_write(0x344B, 0x00);
		emu_csi_imx291_write(0x344C, 0x1F);
		emu_csi_imx291_write(0x344D, 0x00);
		emu_csi_imx291_write(0x344E, 0x1F);
		emu_csi_imx291_write(0x344F, 0x00);
		emu_csi_imx291_write(0x3450, 0x77);
		emu_csi_imx291_write(0x3451, 0x00);
		emu_csi_imx291_write(0x3452, 0x1F);
		emu_csi_imx291_write(0x3453, 0x00);
		emu_csi_imx291_write(0x3454, 0x17);
		emu_csi_imx291_write(0x3472, 0x9C);
		emu_csi_imx291_write(0x3473, 0x07);

		emu_csi_imx291_write(0x3000, 0x00);// operating
		msleep(20);
		emu_csi_imx291_write(0x3002, 0x00);//Master mode start
	}






		/* Measure FrameRate */
		kdrv_ssenif_set(handler, KDRV_SSENIF_CSI_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND);
		curtime1 = hwclock_get_counter();
		kdrv_ssenif_set(handler, KDRV_SSENIF_CSI_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND);
		kdrv_ssenif_set(handler, KDRV_SSENIF_CSI_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND);
		kdrv_ssenif_set(handler, KDRV_SSENIF_CSI_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND);
		kdrv_ssenif_set(handler, KDRV_SSENIF_CSI_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND);
		kdrv_ssenif_set(handler, KDRV_SSENIF_CSI_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND);
		wait = KDRV_SSENIF_INTERRUPT_FRAMEEND;
		kdrv_ssenif_get(handler, KDRV_SSENIF_CSI_WAIT_INTERRUPT, &wait);
		wait = KDRV_SSENIF_INTERRUPT_FRAMEEND;
		kdrv_ssenif_get(handler, KDRV_SSENIF_CSI_WAIT_INTERRUPT, &wait);
		wait = KDRV_SSENIF_INTERRUPT_FRAMEEND;
		kdrv_ssenif_get(handler, KDRV_SSENIF_CSI_WAIT_INTERRUPT, &wait);
		wait = KDRV_SSENIF_INTERRUPT_FRAMEEND;
		kdrv_ssenif_get(handler, KDRV_SSENIF_CSI_WAIT_INTERRUPT, &wait);
		wait = KDRV_SSENIF_INTERRUPT_FRAMEEND;
		kdrv_ssenif_get(handler, KDRV_SSENIF_CSI_WAIT_INTERRUPT, &wait);
		curtime2 = hwclock_get_counter();

		printk("CSI0  Frame Time %u us\n", (curtime2 - curtime1) / 10);

		//kdrv_ssenif_trigger(handler, DISABLE);
		//kdrv_ssenif_close(0, KDRV_SSENIF_ENGINE_CSI0);
	}

	i2c_unregister_device(test_i2c_info->iic_client);
	i2c_del_driver(&test_i2c_driver);

	return 0;
}
#else

static unsigned int mode;
module_param_named(mode, mode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mode, "0 is xxx. 1 is xxx");


int nvt_test_thread(void *__test)
{

	while (1) {
		DBG_DUMP("nvt_test_thread running\r\n");
		msleep(2000);
	}


	return 0;
}



#endif
