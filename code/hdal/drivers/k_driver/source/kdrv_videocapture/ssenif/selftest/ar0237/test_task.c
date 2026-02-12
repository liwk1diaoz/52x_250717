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
//#include "comm/hwclock.h"

#include <linux/i2c.h>
#include "kdrv_videocapture/kdrv_ssenif.h"


static unsigned int mode;
module_param_named(mode, mode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mode, "0 is HDR<default>. 1 is Non-HDR");



#define I2C_NAME "test_task"
#define I2C_ADDR 0x10

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
	msgs[1].len   = 2;
	msgs[1].buf   = buf2;

	if (i2c_transfer(test_i2c_info->iic_adapter, msgs, 2) != 2) {
		printk("%s fail: i2c_transfer not OK \n", __FUNCTION__);
	}

	return (UINT32)(((UINT32)buf2[0]<<8)+((UINT32)buf2[1]<<0));
}

void test_sensor_write(UINT32 addre, UINT32 value)
{
	struct i2c_msg  msgs;
	unsigned char   buf[4];

	buf[0]     = (addre>>8)  & 0xFF;
	buf[1]     = (addre>>0)  & 0xFF;
	buf[2]     = (value>>8) & 0xFF;
	buf[3]     = (value>>0) & 0xFF;
	msgs.addr  = I2C_ADDR;
	msgs.flags = 0;//w
	msgs.len   = 4;
	msgs.buf   = buf;

	if (i2c_transfer(test_i2c_info->iic_adapter, &msgs, 1) != 1) {
		printk("%s fail: i2c_transfer not OK \n", __FUNCTION__);
	}

	//printk("test_sensor_read 0x%04X = 0x%04X\r\n",addre,test_sensor_read(addre));

}



#define emu_lvds_ar0237_write(x, y, z)  test_sensor_write((x), (y))




int nvt_test_thread(void *__test)
{

	/* Hook i2c driver */
	if (i2c_new_device(i2c_get_adapter(1), &test_i2c_device) == NULL) {
		DBG_ERR("%s fail: i2c_new_device not OK.\n", __FUNCTION__);
	}
	if (i2c_add_driver(&test_i2c_driver) != 0) {
		DBG_ERR("%s fail: i2c_add_driver not OK.\n", __FUNCTION__);
	}

	iowrite32(0x00000100, (void __iomem *)0xFD01000C);
	iowrite32((ioread32((void __iomem *)0xFD0100B0)&(~0x1)), (void __iomem *)0xFD0100B0);

	msleep(10);

	/* Config SIE-MCLK freq*/
	iowrite32(0x00000707, (void __iomem *)0xFD020030);

	/* Enable SIE-MCLK */
	iowrite32((ioread32((void __iomem *)0xFD020070)|(0x1<<2)), (void __iomem *)0xFD020070);
	msleep(10);

	/* release reset pin: P_GPIO_3 */
	iowrite32((ioread32((void __iomem *)0xFD070024)|(0x1<<3)), (void __iomem *)0xFD070024);
	iowrite32((0x1<<3), (void __iomem *)0xFD070064);
	msleep(10);
	iowrite32((0x1<<3), (void __iomem *)0xFD070044);
	msleep(100);

	emu_lvds_ar0237_write(0x301A, 0x0001, 2);

	msleep(200);

	if (mode != 0) {
		// Linear Seq
		emu_lvds_ar0237_write(0x3088, 0x8000, 2);
		emu_lvds_ar0237_write(0x3086, 0x4558, 2);
		emu_lvds_ar0237_write(0x3086, 0x72A6, 2);
		emu_lvds_ar0237_write(0x3086, 0x4A31, 2);
		emu_lvds_ar0237_write(0x3086, 0x4342, 2);
		emu_lvds_ar0237_write(0x3086, 0x8E03, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A14, 2);
		emu_lvds_ar0237_write(0x3086, 0x4578, 2);
		emu_lvds_ar0237_write(0x3086, 0x7B3D, 2);
		emu_lvds_ar0237_write(0x3086, 0xFF3D, 2);
		emu_lvds_ar0237_write(0x3086, 0xFF3D, 2);
		emu_lvds_ar0237_write(0x3086, 0xEA2A, 2);
		emu_lvds_ar0237_write(0x3086, 0x43D, 2);
		emu_lvds_ar0237_write(0x3086, 0x102A, 2);
		emu_lvds_ar0237_write(0x3086, 0x52A, 2);
		emu_lvds_ar0237_write(0x3086, 0x1535, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A05, 2);
		emu_lvds_ar0237_write(0x3086, 0x3D10, 2);
		emu_lvds_ar0237_write(0x3086, 0x4558, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A04, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A14, 2);
		emu_lvds_ar0237_write(0x3086, 0x3DFF, 2);
		emu_lvds_ar0237_write(0x3086, 0x3DFF, 2);
		emu_lvds_ar0237_write(0x3086, 0x3DEA, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A04, 2);
		emu_lvds_ar0237_write(0x3086, 0x622A, 2);
		emu_lvds_ar0237_write(0x3086, 0x288E, 2);
		emu_lvds_ar0237_write(0x3086, 0x36, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A08, 2);
		emu_lvds_ar0237_write(0x3086, 0x3D64, 2);
		emu_lvds_ar0237_write(0x3086, 0x7A3D, 2);
		emu_lvds_ar0237_write(0x3086, 0x444, 2);
		emu_lvds_ar0237_write(0x3086, 0x2C4B, 2);
		emu_lvds_ar0237_write(0x3086, 0xA403, 2);
		emu_lvds_ar0237_write(0x3086, 0x430D, 2);
		emu_lvds_ar0237_write(0x3086, 0x2D46, 2);
		emu_lvds_ar0237_write(0x3086, 0x4316, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A90, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E06, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A98, 2);
		emu_lvds_ar0237_write(0x3086, 0x5F16, 2);
		emu_lvds_ar0237_write(0x3086, 0x530D, 2);
		emu_lvds_ar0237_write(0x3086, 0x1660, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E4C, 2);
		emu_lvds_ar0237_write(0x3086, 0x2904, 2);
		emu_lvds_ar0237_write(0x3086, 0x2984, 2);
		emu_lvds_ar0237_write(0x3086, 0x8E03, 2);
		emu_lvds_ar0237_write(0x3086, 0x2AFC, 2);
		emu_lvds_ar0237_write(0x3086, 0x5C1D, 2);
		emu_lvds_ar0237_write(0x3086, 0x5754, 2);
		emu_lvds_ar0237_write(0x3086, 0x495F, 2);
		emu_lvds_ar0237_write(0x3086, 0x5305, 2);
		emu_lvds_ar0237_write(0x3086, 0x5307, 2);
		emu_lvds_ar0237_write(0x3086, 0x4D2B, 2);
		emu_lvds_ar0237_write(0x3086, 0xF810, 2);
		emu_lvds_ar0237_write(0x3086, 0x164C, 2);
		emu_lvds_ar0237_write(0x3086, 0x955, 2);
		emu_lvds_ar0237_write(0x3086, 0x562B, 2);
		emu_lvds_ar0237_write(0x3086, 0xB82B, 2);
		emu_lvds_ar0237_write(0x3086, 0x984E, 2);
		emu_lvds_ar0237_write(0x3086, 0x1129, 2);
		emu_lvds_ar0237_write(0x3086, 0x9460, 2);
		emu_lvds_ar0237_write(0x3086, 0x5C19, 2);
		emu_lvds_ar0237_write(0x3086, 0x5C1B, 2);
		emu_lvds_ar0237_write(0x3086, 0x4548, 2);
		emu_lvds_ar0237_write(0x3086, 0x4508, 2);
		emu_lvds_ar0237_write(0x3086, 0x4588, 2);
		emu_lvds_ar0237_write(0x3086, 0x29B6, 2);
		emu_lvds_ar0237_write(0x3086, 0x8E01, 2);
		emu_lvds_ar0237_write(0x3086, 0x2AF8, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E02, 2);
		emu_lvds_ar0237_write(0x3086, 0x2AFA, 2);
		emu_lvds_ar0237_write(0x3086, 0x3F09, 2);
		emu_lvds_ar0237_write(0x3086, 0x5C1B, 2);
		emu_lvds_ar0237_write(0x3086, 0x29B2, 2);
		emu_lvds_ar0237_write(0x3086, 0x3F0C, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E03, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E15, 2);
		emu_lvds_ar0237_write(0x3086, 0x5C13, 2);
		emu_lvds_ar0237_write(0x3086, 0x3F11, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E0F, 2);
		emu_lvds_ar0237_write(0x3086, 0x5F2B, 2);
		emu_lvds_ar0237_write(0x3086, 0x902B, 2);
		emu_lvds_ar0237_write(0x3086, 0x803E, 2);
		emu_lvds_ar0237_write(0x3086, 0x62A, 2);
		emu_lvds_ar0237_write(0x3086, 0xF23F, 2);
		emu_lvds_ar0237_write(0x3086, 0x103E, 2);
		emu_lvds_ar0237_write(0x3086, 0x160, 2);
		emu_lvds_ar0237_write(0x3086, 0x29A2, 2);
		emu_lvds_ar0237_write(0x3086, 0x29A3, 2);
		emu_lvds_ar0237_write(0x3086, 0x5F4D, 2);
		emu_lvds_ar0237_write(0x3086, 0x1C2A, 2);
		emu_lvds_ar0237_write(0x3086, 0xFA29, 2);
		emu_lvds_ar0237_write(0x3086, 0x8345, 2);
		emu_lvds_ar0237_write(0x3086, 0xA83E, 2);
		emu_lvds_ar0237_write(0x3086, 0x72A, 2);
		emu_lvds_ar0237_write(0x3086, 0xFB3E, 2);
		emu_lvds_ar0237_write(0x3086, 0x2945, 2);
		emu_lvds_ar0237_write(0x3086, 0x8824, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E08, 2);
		emu_lvds_ar0237_write(0x3086, 0x2AFA, 2);
		emu_lvds_ar0237_write(0x3086, 0x5D29, 2);
		emu_lvds_ar0237_write(0x3086, 0x9288, 2);
		emu_lvds_ar0237_write(0x3086, 0x102B, 2);
		emu_lvds_ar0237_write(0x3086, 0x48B, 2);
		emu_lvds_ar0237_write(0x3086, 0x1686, 2);
		emu_lvds_ar0237_write(0x3086, 0x8D48, 2);
		emu_lvds_ar0237_write(0x3086, 0x4D4E, 2);
		emu_lvds_ar0237_write(0x3086, 0x2B80, 2);
		emu_lvds_ar0237_write(0x3086, 0x4C0B, 2);
		emu_lvds_ar0237_write(0x3086, 0x3F36, 2);
		emu_lvds_ar0237_write(0x3086, 0x2AF2, 2);
		emu_lvds_ar0237_write(0x3086, 0x3F10, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E01, 2);
		emu_lvds_ar0237_write(0x3086, 0x6029, 2);
		emu_lvds_ar0237_write(0x3086, 0x8229, 2);
		emu_lvds_ar0237_write(0x3086, 0x8329, 2);
		emu_lvds_ar0237_write(0x3086, 0x435C, 2);
		emu_lvds_ar0237_write(0x3086, 0x155F, 2);
		emu_lvds_ar0237_write(0x3086, 0x4D1C, 2);
		emu_lvds_ar0237_write(0x3086, 0x2AFA, 2);
		emu_lvds_ar0237_write(0x3086, 0x4558, 2);
		emu_lvds_ar0237_write(0x3086, 0x8E00, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A98, 2);
		emu_lvds_ar0237_write(0x3086, 0x3F0A, 2);
		emu_lvds_ar0237_write(0x3086, 0x4A0A, 2);
		emu_lvds_ar0237_write(0x3086, 0x4316, 2);
		emu_lvds_ar0237_write(0x3086, 0xB43, 2);
		emu_lvds_ar0237_write(0x3086, 0x168E, 2);
		emu_lvds_ar0237_write(0x3086, 0x32A, 2);
		emu_lvds_ar0237_write(0x3086, 0x9C45, 2);
		emu_lvds_ar0237_write(0x3086, 0x783F, 2);
		emu_lvds_ar0237_write(0x3086, 0x72A, 2);
		emu_lvds_ar0237_write(0x3086, 0x9D3E, 2);
		emu_lvds_ar0237_write(0x3086, 0x305D, 2);
		emu_lvds_ar0237_write(0x3086, 0x2944, 2);
		emu_lvds_ar0237_write(0x3086, 0x8810, 2);
		emu_lvds_ar0237_write(0x3086, 0x2B04, 2);
		emu_lvds_ar0237_write(0x3086, 0x530D, 2);
		emu_lvds_ar0237_write(0x3086, 0x4558, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E08, 2);
		emu_lvds_ar0237_write(0x3086, 0x8E01, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A98, 2);
		emu_lvds_ar0237_write(0x3086, 0x8E00, 2);
		emu_lvds_ar0237_write(0x3086, 0x76A7, 2);
		emu_lvds_ar0237_write(0x3086, 0x77A7, 2);
		emu_lvds_ar0237_write(0x3086, 0x4644, 2);
		emu_lvds_ar0237_write(0x3086, 0x1616, 2);
		emu_lvds_ar0237_write(0x3086, 0xA57A, 2);
		emu_lvds_ar0237_write(0x3086, 0x1244, 2);
		emu_lvds_ar0237_write(0x3086, 0x4B18, 2);
		emu_lvds_ar0237_write(0x3086, 0x4A04, 2);
		emu_lvds_ar0237_write(0x3086, 0x4316, 2);
		emu_lvds_ar0237_write(0x3086, 0x643, 2);
		emu_lvds_ar0237_write(0x3086, 0x1605, 2);
		emu_lvds_ar0237_write(0x3086, 0x4316, 2);
		emu_lvds_ar0237_write(0x3086, 0x743, 2);
		emu_lvds_ar0237_write(0x3086, 0x1658, 2);
		emu_lvds_ar0237_write(0x3086, 0x4316, 2);
		emu_lvds_ar0237_write(0x3086, 0x5A43, 2);
		emu_lvds_ar0237_write(0x3086, 0x1645, 2);
		emu_lvds_ar0237_write(0x3086, 0x588E, 2);
		emu_lvds_ar0237_write(0x3086, 0x32A, 2);
		emu_lvds_ar0237_write(0x3086, 0x9C45, 2);
		emu_lvds_ar0237_write(0x3086, 0x787B, 2);
		emu_lvds_ar0237_write(0x3086, 0x3F07, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A9D, 2);
		emu_lvds_ar0237_write(0x3086, 0x530D, 2);
		emu_lvds_ar0237_write(0x3086, 0x8B16, 2);
		emu_lvds_ar0237_write(0x3086, 0x863E, 2);
		emu_lvds_ar0237_write(0x3086, 0x2345, 2);
		emu_lvds_ar0237_write(0x3086, 0x5825, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E10, 2);
		emu_lvds_ar0237_write(0x3086, 0x8E01, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A98, 2);
		emu_lvds_ar0237_write(0x3086, 0x8E00, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E10, 2);
		emu_lvds_ar0237_write(0x3086, 0x8D60, 2);
		emu_lvds_ar0237_write(0x3086, 0x1244, 2);
		emu_lvds_ar0237_write(0x3086, 0x4BB9, 2);
		emu_lvds_ar0237_write(0x3086, 0x2C2C, 2);
		emu_lvds_ar0237_write(0x3086, 0x2C2C, 2);
	} else {
		emu_lvds_ar0237_write(0x3088, 0x816A, 2);
		emu_lvds_ar0237_write(0x3086, 0x4558, 2);
		emu_lvds_ar0237_write(0x3086, 0x729B, 2);
		emu_lvds_ar0237_write(0x3086, 0x4A31, 2);
		emu_lvds_ar0237_write(0x3086, 0x4342, 2);
		emu_lvds_ar0237_write(0x3086, 0x8E03, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A14, 2);
		emu_lvds_ar0237_write(0x3086, 0x4578, 2);
		emu_lvds_ar0237_write(0x3086, 0x7B3D, 2);
		emu_lvds_ar0237_write(0x3086, 0xFF3D, 2);
		emu_lvds_ar0237_write(0x3086, 0xFF3D, 2);
		emu_lvds_ar0237_write(0x3086, 0xEA2A, 2);
		emu_lvds_ar0237_write(0x3086, 0x43D, 2);
		emu_lvds_ar0237_write(0x3086, 0x102A, 2);
		emu_lvds_ar0237_write(0x3086, 0x52A, 2);
		emu_lvds_ar0237_write(0x3086, 0x1535, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A05, 2);
		emu_lvds_ar0237_write(0x3086, 0x3D10, 2);
		emu_lvds_ar0237_write(0x3086, 0x4558, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A04, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A14, 2);
		emu_lvds_ar0237_write(0x3086, 0x3DFF, 2);
		emu_lvds_ar0237_write(0x3086, 0x3DFF, 2);
		emu_lvds_ar0237_write(0x3086, 0x3DEA, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A04, 2);
		emu_lvds_ar0237_write(0x3086, 0x622A, 2);
		emu_lvds_ar0237_write(0x3086, 0x288E, 2);
		emu_lvds_ar0237_write(0x3086, 0x36, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A08, 2);
		emu_lvds_ar0237_write(0x3086, 0x3D64, 2);
		emu_lvds_ar0237_write(0x3086, 0x7A3D, 2);
		emu_lvds_ar0237_write(0x3086, 0x444, 2);
		emu_lvds_ar0237_write(0x3086, 0x2C4B, 2);
		emu_lvds_ar0237_write(0x3086, 0x8F00, 2);
		emu_lvds_ar0237_write(0x3086, 0x430C, 2);
		emu_lvds_ar0237_write(0x3086, 0x2D63, 2);
		emu_lvds_ar0237_write(0x3086, 0x4316, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A90, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E06, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A98, 2);
		emu_lvds_ar0237_write(0x3086, 0x168E, 2);
		emu_lvds_ar0237_write(0x3086, 0x32A, 2);
		emu_lvds_ar0237_write(0x3086, 0xFC5C, 2);
		emu_lvds_ar0237_write(0x3086, 0x1D57, 2);
		emu_lvds_ar0237_write(0x3086, 0x5449, 2);
		emu_lvds_ar0237_write(0x3086, 0x5F53, 2);
		emu_lvds_ar0237_write(0x3086, 0x553, 2);
		emu_lvds_ar0237_write(0x3086, 0x74D, 2);
		emu_lvds_ar0237_write(0x3086, 0x2BF8, 2);
		emu_lvds_ar0237_write(0x3086, 0x1016, 2);
		emu_lvds_ar0237_write(0x3086, 0x4C08, 2);
		emu_lvds_ar0237_write(0x3086, 0x5556, 2);
		emu_lvds_ar0237_write(0x3086, 0x2BB8, 2);
		emu_lvds_ar0237_write(0x3086, 0x2B98, 2);
		emu_lvds_ar0237_write(0x3086, 0x4E11, 2);
		emu_lvds_ar0237_write(0x3086, 0x2904, 2);
		emu_lvds_ar0237_write(0x3086, 0x2984, 2);
		emu_lvds_ar0237_write(0x3086, 0x2994, 2);
		emu_lvds_ar0237_write(0x3086, 0x605C, 2);
		emu_lvds_ar0237_write(0x3086, 0x195C, 2);
		emu_lvds_ar0237_write(0x3086, 0x1B45, 2);
		emu_lvds_ar0237_write(0x3086, 0x4845, 2);
		emu_lvds_ar0237_write(0x3086, 0x845, 2);
		emu_lvds_ar0237_write(0x3086, 0x8829, 2);
		emu_lvds_ar0237_write(0x3086, 0xB68E, 2);
		emu_lvds_ar0237_write(0x3086, 0x12A, 2);
		emu_lvds_ar0237_write(0x3086, 0xF83E, 2);
		emu_lvds_ar0237_write(0x3086, 0x22A, 2);
		emu_lvds_ar0237_write(0x3086, 0xFA3F, 2);
		emu_lvds_ar0237_write(0x3086, 0x95C, 2);
		emu_lvds_ar0237_write(0x3086, 0x1B29, 2);
		emu_lvds_ar0237_write(0x3086, 0xB23F, 2);
		emu_lvds_ar0237_write(0x3086, 0xC3E, 2);
		emu_lvds_ar0237_write(0x3086, 0x23E, 2);
		emu_lvds_ar0237_write(0x3086, 0x135C, 2);
		emu_lvds_ar0237_write(0x3086, 0x133F, 2);
		emu_lvds_ar0237_write(0x3086, 0x113E, 2);
		emu_lvds_ar0237_write(0x3086, 0xB5F, 2);
		emu_lvds_ar0237_write(0x3086, 0x2B90, 2);
		emu_lvds_ar0237_write(0x3086, 0x2B80, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E06, 2);
		emu_lvds_ar0237_write(0x3086, 0x162A, 2);
		emu_lvds_ar0237_write(0x3086, 0xF23F, 2);
		emu_lvds_ar0237_write(0x3086, 0x103E, 2);
		emu_lvds_ar0237_write(0x3086, 0x160, 2);
		emu_lvds_ar0237_write(0x3086, 0x29A2, 2);
		emu_lvds_ar0237_write(0x3086, 0x29A3, 2);
		emu_lvds_ar0237_write(0x3086, 0x5F4D, 2);
		emu_lvds_ar0237_write(0x3086, 0x192A, 2);
		emu_lvds_ar0237_write(0x3086, 0xFA29, 2);
		emu_lvds_ar0237_write(0x3086, 0x8345, 2);
		emu_lvds_ar0237_write(0x3086, 0xA83E, 2);
		emu_lvds_ar0237_write(0x3086, 0x72A, 2);
		emu_lvds_ar0237_write(0x3086, 0xFB3E, 2);
		emu_lvds_ar0237_write(0x3086, 0x2945, 2);
		emu_lvds_ar0237_write(0x3086, 0x8821, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E08, 2);
		emu_lvds_ar0237_write(0x3086, 0x2AFA, 2);
		emu_lvds_ar0237_write(0x3086, 0x5D29, 2);
		emu_lvds_ar0237_write(0x3086, 0x9288, 2);
		emu_lvds_ar0237_write(0x3086, 0x102B, 2);
		emu_lvds_ar0237_write(0x3086, 0x48B, 2);
		emu_lvds_ar0237_write(0x3086, 0x1685, 2);
		emu_lvds_ar0237_write(0x3086, 0x8D48, 2);
		emu_lvds_ar0237_write(0x3086, 0x4D4E, 2);
		emu_lvds_ar0237_write(0x3086, 0x2B80, 2);
		emu_lvds_ar0237_write(0x3086, 0x4C0B, 2);
		emu_lvds_ar0237_write(0x3086, 0x3F2B, 2);
		emu_lvds_ar0237_write(0x3086, 0x2AF2, 2);
		emu_lvds_ar0237_write(0x3086, 0x3F10, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E01, 2);
		emu_lvds_ar0237_write(0x3086, 0x6029, 2);
		emu_lvds_ar0237_write(0x3086, 0x8229, 2);
		emu_lvds_ar0237_write(0x3086, 0x8329, 2);
		emu_lvds_ar0237_write(0x3086, 0x435C, 2);
		emu_lvds_ar0237_write(0x3086, 0x155F, 2);
		emu_lvds_ar0237_write(0x3086, 0x4D19, 2);
		emu_lvds_ar0237_write(0x3086, 0x2AFA, 2);
		emu_lvds_ar0237_write(0x3086, 0x4558, 2);
		emu_lvds_ar0237_write(0x3086, 0x8E00, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A98, 2);
		emu_lvds_ar0237_write(0x3086, 0x3F06, 2);
		emu_lvds_ar0237_write(0x3086, 0x1244, 2);
		emu_lvds_ar0237_write(0x3086, 0x4A04, 2);
		emu_lvds_ar0237_write(0x3086, 0x4316, 2);
		emu_lvds_ar0237_write(0x3086, 0x543, 2);
		emu_lvds_ar0237_write(0x3086, 0x1658, 2);
		emu_lvds_ar0237_write(0x3086, 0x4316, 2);
		emu_lvds_ar0237_write(0x3086, 0x5A43, 2);
		emu_lvds_ar0237_write(0x3086, 0x1606, 2);
		emu_lvds_ar0237_write(0x3086, 0x4316, 2);
		emu_lvds_ar0237_write(0x3086, 0x743, 2);
		emu_lvds_ar0237_write(0x3086, 0x168E, 2);
		emu_lvds_ar0237_write(0x3086, 0x32A, 2);
		emu_lvds_ar0237_write(0x3086, 0x9C45, 2);
		emu_lvds_ar0237_write(0x3086, 0x787B, 2);
		emu_lvds_ar0237_write(0x3086, 0x3F07, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A9D, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E2E, 2);
		emu_lvds_ar0237_write(0x3086, 0x4558, 2);
		emu_lvds_ar0237_write(0x3086, 0x253E, 2);
		emu_lvds_ar0237_write(0x3086, 0x68E, 2);
		emu_lvds_ar0237_write(0x3086, 0x12A, 2);
		emu_lvds_ar0237_write(0x3086, 0x988E, 2);
		emu_lvds_ar0237_write(0x3086, 0x12, 2);
		emu_lvds_ar0237_write(0x3086, 0x444B, 2);
		emu_lvds_ar0237_write(0x3086, 0x343, 2);
		emu_lvds_ar0237_write(0x3086, 0x2D46, 2);
		emu_lvds_ar0237_write(0x3086, 0x4316, 2);
		emu_lvds_ar0237_write(0x3086, 0xA343, 2);
		emu_lvds_ar0237_write(0x3086, 0x165D, 2);
		emu_lvds_ar0237_write(0x3086, 0xD29, 2);
		emu_lvds_ar0237_write(0x3086, 0x4488, 2);
		emu_lvds_ar0237_write(0x3086, 0x102B, 2);
		emu_lvds_ar0237_write(0x3086, 0x453, 2);
		emu_lvds_ar0237_write(0x3086, 0xD8B, 2);
		emu_lvds_ar0237_write(0x3086, 0x1685, 2);
		emu_lvds_ar0237_write(0x3086, 0x448E, 2);
		emu_lvds_ar0237_write(0x3086, 0x32A, 2);
		emu_lvds_ar0237_write(0x3086, 0xFC5C, 2);
		emu_lvds_ar0237_write(0x3086, 0x1D8D, 2);
		emu_lvds_ar0237_write(0x3086, 0x6057, 2);
		emu_lvds_ar0237_write(0x3086, 0x5417, 2);
		emu_lvds_ar0237_write(0x3086, 0xFF17, 2);
		emu_lvds_ar0237_write(0x3086, 0x4B2A, 2);
		emu_lvds_ar0237_write(0x3086, 0xF43E, 2);
		emu_lvds_ar0237_write(0x3086, 0x62A, 2);
		emu_lvds_ar0237_write(0x3086, 0xFC49, 2);
		emu_lvds_ar0237_write(0x3086, 0x5F53, 2);
		emu_lvds_ar0237_write(0x3086, 0x553, 2);
		emu_lvds_ar0237_write(0x3086, 0x74D, 2);
		emu_lvds_ar0237_write(0x3086, 0x2BF8, 2);
		emu_lvds_ar0237_write(0x3086, 0x1016, 2);
		emu_lvds_ar0237_write(0x3086, 0x4C08, 2);
		emu_lvds_ar0237_write(0x3086, 0x5556, 2);
		emu_lvds_ar0237_write(0x3086, 0x2BB8, 2);
		emu_lvds_ar0237_write(0x3086, 0x2B98, 2);
		emu_lvds_ar0237_write(0x3086, 0x4E11, 2);
		emu_lvds_ar0237_write(0x3086, 0x2904, 2);
		emu_lvds_ar0237_write(0x3086, 0x2984, 2);
		emu_lvds_ar0237_write(0x3086, 0x2994, 2);
		emu_lvds_ar0237_write(0x3086, 0x605C, 2);
		emu_lvds_ar0237_write(0x3086, 0x195C, 2);
		emu_lvds_ar0237_write(0x3086, 0x1B45, 2);
		emu_lvds_ar0237_write(0x3086, 0x4845, 2);
		emu_lvds_ar0237_write(0x3086, 0x845, 2);
		emu_lvds_ar0237_write(0x3086, 0x8829, 2);
		emu_lvds_ar0237_write(0x3086, 0xB68E, 2);
		emu_lvds_ar0237_write(0x3086, 0x12A, 2);
		emu_lvds_ar0237_write(0x3086, 0xF83E, 2);
		emu_lvds_ar0237_write(0x3086, 0x22A, 2);
		emu_lvds_ar0237_write(0x3086, 0xFA3F, 2);
		emu_lvds_ar0237_write(0x3086, 0x95C, 2);
		emu_lvds_ar0237_write(0x3086, 0x1B29, 2);
		emu_lvds_ar0237_write(0x3086, 0xB23F, 2);
		emu_lvds_ar0237_write(0x3086, 0xC3E, 2);
		emu_lvds_ar0237_write(0x3086, 0x23E, 2);
		emu_lvds_ar0237_write(0x3086, 0x135C, 2);
		emu_lvds_ar0237_write(0x3086, 0x133F, 2);
		emu_lvds_ar0237_write(0x3086, 0x113E, 2);
		emu_lvds_ar0237_write(0x3086, 0xB5F, 2);
		emu_lvds_ar0237_write(0x3086, 0x2B90, 2);
		emu_lvds_ar0237_write(0x3086, 0x2B80, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E10, 2);
		emu_lvds_ar0237_write(0x3086, 0x2AF2, 2);
		emu_lvds_ar0237_write(0x3086, 0x3F10, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E01, 2);
		emu_lvds_ar0237_write(0x3086, 0x6029, 2);
		emu_lvds_ar0237_write(0x3086, 0xA229, 2);
		emu_lvds_ar0237_write(0x3086, 0xA35F, 2);
		emu_lvds_ar0237_write(0x3086, 0x4D1C, 2);
		emu_lvds_ar0237_write(0x3086, 0x2AFA, 2);
		emu_lvds_ar0237_write(0x3086, 0x2983, 2);
		emu_lvds_ar0237_write(0x3086, 0x45A8, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E07, 2);
		emu_lvds_ar0237_write(0x3086, 0x2AFB, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E29, 2);
		emu_lvds_ar0237_write(0x3086, 0x4588, 2);
		emu_lvds_ar0237_write(0x3086, 0x243E, 2);
		emu_lvds_ar0237_write(0x3086, 0x82A, 2);
		emu_lvds_ar0237_write(0x3086, 0xFA5D, 2);
		emu_lvds_ar0237_write(0x3086, 0x2992, 2);
		emu_lvds_ar0237_write(0x3086, 0x8810, 2);
		emu_lvds_ar0237_write(0x3086, 0x2B04, 2);
		emu_lvds_ar0237_write(0x3086, 0x8B16, 2);
		emu_lvds_ar0237_write(0x3086, 0x868D, 2);
		emu_lvds_ar0237_write(0x3086, 0x484D, 2);
		emu_lvds_ar0237_write(0x3086, 0x4E2B, 2);
		emu_lvds_ar0237_write(0x3086, 0x804C, 2);
		emu_lvds_ar0237_write(0x3086, 0xB3F, 2);
		emu_lvds_ar0237_write(0x3086, 0x332A, 2);
		emu_lvds_ar0237_write(0x3086, 0xF23F, 2);
		emu_lvds_ar0237_write(0x3086, 0x103E, 2);
		emu_lvds_ar0237_write(0x3086, 0x160, 2);
		emu_lvds_ar0237_write(0x3086, 0x2982, 2);
		emu_lvds_ar0237_write(0x3086, 0x2983, 2);
		emu_lvds_ar0237_write(0x3086, 0x2943, 2);
		emu_lvds_ar0237_write(0x3086, 0x5C15, 2);
		emu_lvds_ar0237_write(0x3086, 0x5F4D, 2);
		emu_lvds_ar0237_write(0x3086, 0x1C2A, 2);
		emu_lvds_ar0237_write(0x3086, 0xFA45, 2);
		emu_lvds_ar0237_write(0x3086, 0x588E, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A, 2);
		emu_lvds_ar0237_write(0x3086, 0x983F, 2);
		emu_lvds_ar0237_write(0x3086, 0x64A, 2);
		emu_lvds_ar0237_write(0x3086, 0x739D, 2);
		emu_lvds_ar0237_write(0x3086, 0xA43, 2);
		emu_lvds_ar0237_write(0x3086, 0x160B, 2);
		emu_lvds_ar0237_write(0x3086, 0x4316, 2);
		emu_lvds_ar0237_write(0x3086, 0x8E03, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A9C, 2);
		emu_lvds_ar0237_write(0x3086, 0x4578, 2);
		emu_lvds_ar0237_write(0x3086, 0x3F07, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A9D, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E12, 2);
		emu_lvds_ar0237_write(0x3086, 0x4558, 2);
		emu_lvds_ar0237_write(0x3086, 0x3F04, 2);
		emu_lvds_ar0237_write(0x3086, 0x8E01, 2);
		emu_lvds_ar0237_write(0x3086, 0x2A98, 2);
		emu_lvds_ar0237_write(0x3086, 0x8E00, 2);
		emu_lvds_ar0237_write(0x3086, 0x9176, 2);
		emu_lvds_ar0237_write(0x3086, 0x9C77, 2);
		emu_lvds_ar0237_write(0x3086, 0x9C46, 2);
		emu_lvds_ar0237_write(0x3086, 0x4416, 2);
		emu_lvds_ar0237_write(0x3086, 0x1690, 2);
		emu_lvds_ar0237_write(0x3086, 0x7A12, 2);
		emu_lvds_ar0237_write(0x3086, 0x444B, 2);
		emu_lvds_ar0237_write(0x3086, 0x4A00, 2);
		emu_lvds_ar0237_write(0x3086, 0x4316, 2);
		emu_lvds_ar0237_write(0x3086, 0x6343, 2);
		emu_lvds_ar0237_write(0x3086, 0x1608, 2);
		emu_lvds_ar0237_write(0x3086, 0x4316, 2);
		emu_lvds_ar0237_write(0x3086, 0x5043, 2);
		emu_lvds_ar0237_write(0x3086, 0x1665, 2);
		emu_lvds_ar0237_write(0x3086, 0x4316, 2);
		emu_lvds_ar0237_write(0x3086, 0x6643, 2);
		emu_lvds_ar0237_write(0x3086, 0x168E, 2);
		emu_lvds_ar0237_write(0x3086, 0x32A, 2);
		emu_lvds_ar0237_write(0x3086, 0x9C45, 2);
		emu_lvds_ar0237_write(0x3086, 0x783F, 2);
		emu_lvds_ar0237_write(0x3086, 0x72A, 2);
		emu_lvds_ar0237_write(0x3086, 0x9D5D, 2);
		emu_lvds_ar0237_write(0x3086, 0xC29, 2);
		emu_lvds_ar0237_write(0x3086, 0x4488, 2);
		emu_lvds_ar0237_write(0x3086, 0x102B, 2);
		emu_lvds_ar0237_write(0x3086, 0x453, 2);
		emu_lvds_ar0237_write(0x3086, 0xD8B, 2);
		emu_lvds_ar0237_write(0x3086, 0x1686, 2);
		emu_lvds_ar0237_write(0x3086, 0x3E1F, 2);
		emu_lvds_ar0237_write(0x3086, 0x4558, 2);
		emu_lvds_ar0237_write(0x3086, 0x283E, 2);
		emu_lvds_ar0237_write(0x3086, 0x68E, 2);
		emu_lvds_ar0237_write(0x3086, 0x12A, 2);
		emu_lvds_ar0237_write(0x3086, 0x988E, 2);
		emu_lvds_ar0237_write(0x3086, 0x8D, 2);
		emu_lvds_ar0237_write(0x3086, 0x6012, 2);
		emu_lvds_ar0237_write(0x3086, 0x444B, 2);
		emu_lvds_ar0237_write(0x3086, 0xB92C, 2);
		emu_lvds_ar0237_write(0x3086, 0x2C2C, 2);
	}


	// Default 12_Aug
	emu_lvds_ar0237_write(0x30F0, 0x1283, 2);
	emu_lvds_ar0237_write(0x3064, 0x1802, 2);
	emu_lvds_ar0237_write(0x3EEE, 0xA0AA, 2);
	emu_lvds_ar0237_write(0x30BA, 0x762C, 2);
	emu_lvds_ar0237_write(0x3FA4, 0xF70, 2);
	emu_lvds_ar0237_write(0x309E, 0x16A, 2);
	emu_lvds_ar0237_write(0x3096, 0xF880, 2);
	emu_lvds_ar0237_write(0x3F32, 0xF880, 2);
	emu_lvds_ar0237_write(0x3092, 0x6F, 2);


	if (mode != 0) {
		//HiSPi Setup
		emu_lvds_ar0237_write(0x31AE, 0x0304, 2);
		emu_lvds_ar0237_write(0x31C6, 0x0400, 2);
		emu_lvds_ar0237_write(0x306E, 0x9010, 2);
		emu_lvds_ar0237_write(0x301A, 0x0058, 2);


		emu_lvds_ar0237_write(0x30B0, 0x0918, 2);//0x0918 / 0x118
		emu_lvds_ar0237_write(0x31AC, 0x0C0C, 2);
		emu_lvds_ar0237_write(0x318E, 0x0000, 2);
		emu_lvds_ar0237_write(0x3082, 0x0009, 2);
		emu_lvds_ar0237_write(0x30BA, 0x762C, 2);
		emu_lvds_ar0237_write(0x31D0, 0x0000, 2);


		// PLL
		emu_lvds_ar0237_write(0x302A, 6,   2);
		emu_lvds_ar0237_write(0x302C, 1,   2);
		emu_lvds_ar0237_write(0x302E, 4,   2);
		emu_lvds_ar0237_write(0x3030, 66,  2);
		emu_lvds_ar0237_write(0x3036, 12,  2);
		emu_lvds_ar0237_write(0x3038, 1,   2);



		// working mode
		emu_lvds_ar0237_write(0x302A, 12,  2);
		emu_lvds_ar0237_write(0x302C, 1,   2);
		emu_lvds_ar0237_write(0x302E, 4,   2);
		emu_lvds_ar0237_write(0x3030, 66,  2);
		emu_lvds_ar0237_write(0x3036, 12,  2);
		emu_lvds_ar0237_write(0x3038, 2,   2);

		emu_lvds_ar0237_write(0x3004, 12,  2);
		emu_lvds_ar0237_write(0x3008, 1931, 2);
		emu_lvds_ar0237_write(0x3002, 4,   2);
		emu_lvds_ar0237_write(0x3006, 1083, 2);
		emu_lvds_ar0237_write(0x30A2, 1,   2);
		emu_lvds_ar0237_write(0x30A6, 1,   2);
		emu_lvds_ar0237_write(0x3040, 0, 2);
		emu_lvds_ar0237_write(0x300A, 1097, 2);
		emu_lvds_ar0237_write(0x300C, 1129, 2);
		emu_lvds_ar0237_write(0x3012, 1095, 2);
	} else {
		//HiSPi Setup
		emu_lvds_ar0237_write(0x31AE, 0x0304, 2);
		emu_lvds_ar0237_write(0x31C6, 0x0400, 2);
		emu_lvds_ar0237_write(0x306E, 0x9018, 2);
		emu_lvds_ar0237_write(0x301A, 0x0058, 2);

		emu_lvds_ar0237_write(0x30B0, 0x0938, 2);
		emu_lvds_ar0237_write(0x31AC, 0x0C0C, 2);
		emu_lvds_ar0237_write(0x318E, 0x1200, 2);
		emu_lvds_ar0237_write(0x3082, 0x0800, 2);
		emu_lvds_ar0237_write(0x30BA, 0x772C, 2);
		emu_lvds_ar0237_write(0x31D0, 0x0000, 2);


		// working mode
		emu_lvds_ar0237_write(0x302A, 6,  2);
		emu_lvds_ar0237_write(0x302C, 1,   2);
		emu_lvds_ar0237_write(0x302E, 4,   2);
		emu_lvds_ar0237_write(0x3030, 66,  2);
		emu_lvds_ar0237_write(0x3036, 12,  2);
		emu_lvds_ar0237_write(0x3038, 1,   2);

		emu_lvds_ar0237_write(0x3004, 12,  2);
		emu_lvds_ar0237_write(0x3008, 1931, 2);
		emu_lvds_ar0237_write(0x3002, 4,   2);
		emu_lvds_ar0237_write(0x3006, 1083, 2);
		emu_lvds_ar0237_write(0x30A2, 1,   2);
		emu_lvds_ar0237_write(0x30A6, 1,   2);
		emu_lvds_ar0237_write(0x3040, 0, 2);
		emu_lvds_ar0237_write(0x300A, 1216, 2);
		emu_lvds_ar0237_write(0x300C, 2036, 2);
		emu_lvds_ar0237_write(0x3012, 1095, 2);
		emu_lvds_ar0237_write(0x3212, 115, 2);

	}

	emu_lvds_ar0237_write(0x301A, 0x005C, 2);

	{
		UINT32 handler;
		UINT32 wait;
		//UINT32 curtime1, curtime2;

		if (kdrv_ssenif_open(0, KDRV_SSENIF_ENGINE_LVDS0) == 0) {
			handler = KDRV_DEV_ID(0, KDRV_SSENIF_ENGINE_LVDS0, 0);
		} else {
			DBG_ERR("kdrv_ssenif_open failed\r\n");
			return 0;
		}

		kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_SENSORTYPE,		KDRV_SSENIF_SENSORTYPE_ONSEMI);
		kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_DLANE_NUMBER,		4);
		kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_VALID_WIDTH,		1920);
		kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_VALID_HEIGHT,		1080);
		kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_PIXEL_DEPTH,		12);
		kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_CLANE_SWITCH,		KDRV_SSENIF_CLANE_LVDS0_USE_C0C4);
		kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_TIMEOUT_PERIOD,	1000);
		kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_STOP_METHOD,		KDRV_SSENIF_STOPMETHOD_FRAME_END);
		kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_IMGID_TO_SIE,		0);
		kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_IMGID_TO_SIE2,	1);
		kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_OUTORDER_0,		KDRV_SSENIF_LANESEL_HSI_D0);
		kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_OUTORDER_1,		KDRV_SSENIF_LANESEL_HSI_D1);
		kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_OUTORDER_2,		KDRV_SSENIF_LANESEL_HSI_D2);
		kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_OUTORDER_3,		KDRV_SSENIF_LANESEL_HSI_D3);

		kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_SYNCCODE_DEFAULT, KDRV_SSENIFLVDS_CONTROL_WORD(KDRV_SSENIF_LANESEL_HSI_LOW4, 0));

		/* Enable */
		kdrv_ssenif_trigger(handler, ENABLE);

		/* Measure FrameRate */
		if (mode != 1) {
			kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND2);
			//curtime1 = hwclock_get_counter();
			kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND2);
			kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND2);
			kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND2);
			kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND2);
			kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND2);
			wait = KDRV_SSENIF_INTERRUPT_FRAMEEND2;
			kdrv_ssenif_get(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, &wait);
			wait = KDRV_SSENIF_INTERRUPT_FRAMEEND2;
			kdrv_ssenif_get(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, &wait);
			wait = KDRV_SSENIF_INTERRUPT_FRAMEEND2;
			kdrv_ssenif_get(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, &wait);
			wait = KDRV_SSENIF_INTERRUPT_FRAMEEND2;
			kdrv_ssenif_get(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, &wait);
			wait = KDRV_SSENIF_INTERRUPT_FRAMEEND2;
			kdrv_ssenif_get(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, &wait);
			//curtime2 = hwclock_get_counter();
		} else {
			kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND);
			//curtime1 = hwclock_get_counter();
			kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND);
			kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND);
			kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND);
			kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND);
			kdrv_ssenif_set(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, KDRV_SSENIF_INTERRUPT_FRAMEEND);
			wait = KDRV_SSENIF_INTERRUPT_FRAMEEND;
			kdrv_ssenif_get(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, &wait);
			wait = KDRV_SSENIF_INTERRUPT_FRAMEEND;
			kdrv_ssenif_get(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, &wait);
			wait = KDRV_SSENIF_INTERRUPT_FRAMEEND;
			kdrv_ssenif_get(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, &wait);
			wait = KDRV_SSENIF_INTERRUPT_FRAMEEND;
			kdrv_ssenif_get(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, &wait);
			wait = KDRV_SSENIF_INTERRUPT_FRAMEEND;
			kdrv_ssenif_get(handler, KDRV_SSENIF_LVDS_WAIT_INTERRUPT, &wait);
			//curtime2 = hwclock_get_counter();
		}


		//printk("mode=%d  Frame Time %u us\n", mode, (curtime2 - curtime1) / 10);

		//kdrv_ssenif_trigger(handler, DISABLE);
		//kdrv_ssenif_close(0, KDRV_SSENIF_ENGINE_LVDS0);
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
