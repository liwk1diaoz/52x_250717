#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <asm/signal.h>
#include <kwrap/dev.h>
#include <plat/nvt-gpio.h>
#include <plat/nvt-sramctl.h>
#include <plat/pad.h>
#include <plat/top.h>

#include "gyro_spi_main.h"
#include "gyro_spi_dbg.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int gyro_spi_debug_level = NVT_DBG_IND;
module_param_named(gyro_spi_debug_level, gyro_spi_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(gyro_spi_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id gyro_spi_match_table[] = {
	{	.compatible = "nvt,nvt_gyro_spi"},
	{}
};

//=============================================================================
// function define
//=============================================================================

static int nvt_gyro_spi_open(struct inode *inode, struct file *file)
{
	GYRO_DRV_INFO *pdrv_info;

	pdrv_info = container_of(inode->i_cdev, GYRO_DRV_INFO, cdev);
	file->private_data = pdrv_info;

    return 0;
}

static int nvt_gyro_spi_release(struct inode *inode, struct file *file)
{
	GYRO_DRV_INFO *pdrv_info;
	pdrv_info = container_of(inode->i_cdev, GYRO_DRV_INFO, cdev);

    return 0;
}

static long nvt_gyro_spi_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	GYRO_DRV_INFO *pdrv;

	inode = file_inode(filp);
	pdrv = filp->private_data;

	return 0 ;
}

struct file_operations nvt_gyro_spi_fops = {
    .owner   = THIS_MODULE,
    .open    = nvt_gyro_spi_open,
    .release = nvt_gyro_spi_release,
    .unlocked_ioctl = nvt_gyro_spi_ioctl,
    .llseek  = no_llseek,
};

static int nvt_gyro_spi_probe(struct platform_device * pdev)
{
    GYRO_DRV_INFO *pdrv_info;
	const struct of_device_id *match;
    int ret = 0;
	unsigned char ucloop;
	//PIN_GROUP_CONFIG pinmux_config[1];
    DBG_IND("%s\n", pdev->name);

	//pinmux_config[0].pin_function = PIN_FUNC_SPI;
	//nvt_pinmux_capture(pinmux_config, 1);
	//printk("%s: get pinmux 0x%x\r\n", __func__, pinmux_config[0].config);

#if (NVT_GYRO_528_PINMUX == ENABLE)
    nvt_gyro_spi_drv_pinmux_setting();
#elif (NVT_GYRO_530_PINMUX == ENABLE)
	nvt_gyro_spi_drv_pinmux_setting();
#endif

	match = of_match_device(gyro_spi_match_table, &pdev->dev);
	if (!match){
        DBG_ERR("Platform device not found\n");
		return -EINVAL;
	}
	DBG_ERR("Get device tree success\r\n");

    pdrv_info = kzalloc(sizeof(GYRO_DRV_INFO), GFP_KERNEL);
    if (!pdrv_info) {
        DBG_ERR("failed to allocate memory\n");
        return -ENOMEM;
    }
	DBG_ERR("Allocate memory success\r\n");

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		pdrv_info->presource[ucloop] = platform_get_resource(pdev, IORESOURCE_MEM, ucloop);
		if (pdrv_info->presource[ucloop] == NULL) {
			DBG_ERR("No IO memory resource defined:%d\n", ucloop);
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}
	DBG_ERR("Get resource success\r\n");

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		if (!request_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]), pdev->name)) {
			DBG_ERR("failed to request memory resource%d\n", ucloop);
			release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}
	DBG_ERR("Allocate resource success\r\n");

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
        pdrv_info->module_info.io_addr[ucloop] = ioremap_nocache(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (pdrv_info->module_info.io_addr[ucloop] == NULL) {
			DBG_ERR("ioremap() failed in module%d\n", ucloop);
			if (ucloop == 0) {
				iounmap(pdrv_info->module_info.io_addr[ucloop]);
			}
			ret = -ENODEV;
			goto FAIL_FREE_RES;
		}
	}
	DBG_ERR("ioremap io_addr success\r\n");

	for (ucloop = 0 ; ucloop < MODULE_IRQ_NUM; ucloop++) {
		pdrv_info->module_info.intr_id[ucloop] = platform_get_irq(pdev, ucloop);
		if (pdrv_info->module_info.intr_id[ucloop] < 0) {
			DBG_ERR("No IRQ resource defined\n");
			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}
	DBG_ERR("Get irq success\r\n");

	//Get clock source
	for (ucloop = 0 ; ucloop < 1; ucloop++) {
		pdrv_info->module_info.pclk[ucloop] = clk_get(&pdev->dev, dev_name(&pdev->dev));
		if (IS_ERR(pdrv_info->module_info.pclk[ucloop])) {
			DBG_ERR("faile to get clock%d source\n", ucloop);
			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}
	DBG_ERR("Get clock source success\r\n");

	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, MODULE_MINOR_COUNT, MODULE_NAME)) {
		DBG_ERR("Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_REMAP;
	}
	DBG_ERR("Register chrdev region success\r\n");
	DBG_IND("DevID Major:%d minor:%d\n" \
		    , MAJOR(pdrv_info->dev_id), MINOR(pdrv_info->dev_id));

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_gyro_spi_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, MODULE_MINOR_COUNT)) {
		DBG_ERR("Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}
	DBG_ERR("Cdev init success\r\n");

    pdrv_info->pmodule_class = class_create(THIS_MODULE, "nvt_gyro");
	if(IS_ERR(pdrv_info->pmodule_class)) {
		DBG_ERR("failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}
	DBG_ERR("Class create success\r\n");

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
    ucloop = 0;
    pdrv_info->pdevice[ucloop] = device_create(pdrv_info->pmodule_class, NULL
	                             , MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
                                 , MODULE_NAME);

    if(IS_ERR(pdrv_info->pdevice[ucloop])) {
        DBG_ERR("failed in creating device%d.\n", ucloop);
        ret = -ENODEV;
        goto FAIL_CLASS;
	}
	DBG_ERR("Create device success\r\n");

	ret = nvt_gyro_spi_drv_init(&pdrv_info->module_info);
	platform_set_drvdata(pdev, pdrv_info);
	if(ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}
	DBG_ERR("Probe success done\r\n");

	#if 0
	//for internal testing
	emu_gyroAuto(clk_get_rate(pdrv_info->module_info.pclk[0]));
	#endif
	return ret;

FAIL_DRV_INIT:
FAIL_CLASS:
	class_destroy(pdrv_info->pmodule_class);

FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

FAIL_FREE_REMAP:
	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++)
	    iounmap(pdrv_info->module_info.io_addr[ucloop]);

FAIL_FREE_RES:
	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++)
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));

FAIL_FREE_BUF:
    kfree(pdrv_info);
    return ret;
}

static int nvt_gyro_spi_remove(struct platform_device *pdev)
{
	GYRO_DRV_INFO *pdrv_info;
	unsigned char ucloop;

    DBG_IND("%s\n", pdev->name);

	pdrv_info = platform_get_drvdata(pdev);

	nvt_gyro_spi_drv_remove(&pdrv_info->module_info);

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT ) ; ucloop++)
		device_unregister(pdrv_info->pdevice[ucloop]);

	class_destroy(pdrv_info->pmodule_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++)
	    iounmap(pdrv_info->module_info.io_addr[ucloop]);

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++)
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));

    kfree(pdrv_info);
	pdrv_info = NULL;
	platform_set_drvdata(pdev, pdrv_info);
	return 0;
}

static int nvt_gyro_spi_suspend( struct platform_device *pdev, pm_message_t state)
{
	GYRO_DRV_INFO *pdrv_info;;

	pdrv_info = platform_get_drvdata(pdev);

	nvt_gyro_spi_drv_suspend(&pdrv_info->module_info);

	return 0;
}


static int nvt_gyro_spi_resume( struct platform_device *pdev)
{
	GYRO_DRV_INFO *pdrv_info;;

	pdrv_info = platform_get_drvdata(pdev);

	nvt_gyro_spi_drv_resume(&pdrv_info->module_info);

	return 0;
}

static struct platform_driver nvt_gyro_spi_driver = {
    .driver = {
		        .name   = "nvt_gyro",
				.owner = THIS_MODULE,
				.of_match_table = gyro_spi_match_table,
		      },
    .probe      = nvt_gyro_spi_probe,
    .remove     = nvt_gyro_spi_remove,
	.suspend = nvt_gyro_spi_suspend,
	.resume = nvt_gyro_spi_resume
};

int __init nvt_gyro_spi_init(void)
{
	int ret;

	ret = platform_driver_register(&nvt_gyro_spi_driver);

	return 0;
}

void __exit nvt_gyro_spi_exit(void)
{
	platform_driver_unregister(&nvt_gyro_spi_driver);
}

module_init(nvt_gyro_spi_init);
module_exit(nvt_gyro_spi_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("gyro spi driver");
MODULE_VERSION("1.00.001");
MODULE_LICENSE("GPL");
