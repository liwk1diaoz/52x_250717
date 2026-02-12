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

#include "dce_drv.h"
#include "dce_reg.h"
#include "dce_main.h"
#include "dce_proc.h"
#include "dce_dbg.h"

//struct clk *p_dce_clk[MODULE_CLK_NUM] = {NULL};

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int dce_debug_level = NVT_DBG_WRN;
module_param_named(dce_debug_level, dce_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dce_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id dce_match_table[] = {
	{   .compatible = "nvt,kdrv_dce"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int nvt_dce_open(struct inode *inode, struct file *file);
static int nvt_dce_release(struct inode *inode, struct file *file);
//static long nvt_dce_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int nvt_dce_probe(struct platform_device *pdev);
static int nvt_dce_suspend(struct platform_device *pdev, pm_message_t state);
static int nvt_dce_resume(struct platform_device *pdev);
static int nvt_dce_remove(struct platform_device *pdev);
int __init nvt_dce_module_init(void);
void __exit nvt_dce_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int nvt_dce_open(struct inode *inode, struct file *file)
{
	DCE_DRV_INFO *pdrv_info;

	pdrv_info = container_of(vos_icdev(inode), DCE_DRV_INFO, cdev);
	file->private_data = pdrv_info;

	if (nvt_dce_drv_open(&pdrv_info->module_info, MINOR(vos_irdev(inode)))) {
		DBG_ERR("failed to open driver\n");
		return -1;
	}

	return 0;
}

static int nvt_dce_release(struct inode *inode, struct file *file)
{
	DCE_DRV_INFO *pdrv_info;
	pdrv_info = container_of(vos_icdev(inode), DCE_DRV_INFO, cdev);

	nvt_dce_drv_release(&pdrv_info->module_info, MINOR(vos_irdev(inode)));
	return 0;
}

#if 0
static long nvt_dce_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PDCE_DRV_INFO pdrv;

	inode = file_inode(filp);
	pdrv = filp->private_data;

	return nvt_dce_drv_ioctl(MINOR(vos_irdev(inode)), &pdrv->module_info, cmd, arg);
}
#endif

struct file_operations nvt_dce_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_dce_open,
	.release = nvt_dce_release,
	// NOTE: XXXXX
	//.unlocked_ioctl = nvt_dce_ioctl,
	.llseek  = no_llseek,
};

static int nvt_dce_probe(struct platform_device *pdev)
{
	DCE_DRV_INFO *pdrv_info;//info;
	const struct of_device_id *match;
	int ret = 0;
	int ucloop;

	DBG_IND("kdrv %s\n", pdev->name);

	match = of_match_device(dce_match_table, &pdev->dev);
	if (!match) {
		DBG_ERR("Platform device not found \n");
		return -EINVAL;
	}

	DBG_IND("1\r\n");

	pdrv_info = kzalloc(sizeof(DCE_DRV_INFO), GFP_KERNEL);
	if (!pdrv_info) {
		DBG_ERR("failed to allocate memory\n");
		return -ENOMEM;
	}

	DBG_IND("2\r\n");

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		pdrv_info->presource[ucloop] = platform_get_resource(pdev, IORESOURCE_MEM, ucloop);
		if (pdrv_info->presource[ucloop] == NULL) {
			DBG_ERR("No IO memory resource defined:%d\n", ucloop);
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		DBG_IND("%d. resource:0x%x size:0x%x\n", ucloop, pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (!request_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]), pdev->name)) {
			DBG_ERR("failed to request memory resource%d\n", ucloop);
			while (ucloop >= 0) {
				release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
				ucloop--;
			}
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	DBG_IND("3\r\n");

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		pdrv_info->module_info.io_addr[ucloop] = ioremap_nocache(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (pdrv_info->module_info.io_addr[ucloop] == NULL) {
			DBG_ERR("ioremap() failed in module%d\n", ucloop);
			while (ucloop >= 0) {
				iounmap(pdrv_info->module_info.io_addr[ucloop]);
				ucloop--;
			}
			ret = -ENODEV;
			goto FAIL_FREE_RES;
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_IRQ_NUM; ucloop++) {
		pdrv_info->module_info.iinterrupt_id[ucloop] = platform_get_irq(pdev, ucloop);
		DBG_IND("IRQ %d. ID%d\n", ucloop, pdrv_info->module_info.iinterrupt_id[ucloop]);
		if (pdrv_info->module_info.iinterrupt_id[ucloop] < 0) {
			DBG_ERR("No IRQ resource defined\n");
			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}

	DBG_IND("4\r\n");

	//Get clock source
	for (ucloop = 0 ; ucloop < MODULE_CLK_NUM; ucloop++) {
		pdrv_info->module_info.pclk[ucloop] = clk_get(&pdev->dev, dev_name(&pdev->dev));
		DBG_DUMP("%s\r\n", dev_name(&pdev->dev));
		if (IS_ERR(pdrv_info->module_info.pclk[ucloop])) {
			DBG_ERR("failed to get dce clock%d source\n", ucloop);

			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}

	//Get pclock source
	for (ucloop = 0 ; ucloop < MODULE_CLK_NUM; ucloop++) {
		pdrv_info->module_info.p_pclk[ucloop] = clk_get(&pdev->dev, "dce_pclk");
		if (IS_ERR(pdrv_info->module_info.p_pclk[ucloop])) {
			DBG_ERR("failed to get dce pclock%d source\n", ucloop);

			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}

	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, MODULE_MINOR_COUNT, MODULE_NAME)) {
		DBG_ERR("Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_REMAP;
	}

	DBG_IND("DevID Major:%d minor:%d\n" \
			, MAJOR(pdrv_info->dev_id), MINOR(pdrv_info->dev_id));

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_dce_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, MODULE_MINOR_COUNT)) {
		DBG_ERR("Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	DBG_IND("5\r\n");

	pdrv_info->pmodule_class = class_create(THIS_MODULE, "kdrv_dce");
	if (IS_ERR(pdrv_info->pmodule_class)) {
		DBG_ERR("failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		pdrv_info->pdevice[ucloop] = device_create(pdrv_info->pmodule_class, NULL
									 , MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
									 , MODULE_NAME"%d", ucloop);

		if (IS_ERR(pdrv_info->pdevice[ucloop])) {
			DBG_ERR("failed in creating device%d.\n", ucloop);

			while (ucloop >= 0) {
				device_unregister(pdrv_info->pdevice[ucloop]);
				ucloop--;
			}
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}

	DBG_IND("6\r\n");

	ret = nvt_dce_proc_init(pdrv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DEV;
	}

	DBG_IND("7\r\n");

	platform_set_drvdata(pdev, pdrv_info);

	ret = nvt_dce_drv_init(&pdrv_info->module_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	if (request_irq(pdrv_info->module_info.iinterrupt_id[0], nvt_dce_drv_isr, IRQF_TRIGGER_HIGH, "DCE_INT", &pdrv_info->module_info)) {
		DBG_ERR("failed to register an IRQ Int:%d\n", pdrv_info->module_info.iinterrupt_id[0]);
		ret = -ENODEV;
		goto FAIL_FREE_IRQ;
	}


	return ret;

FAIL_FREE_IRQ:
	free_irq(pdrv_info->module_info.iinterrupt_id[0], &pdrv_info->module_info);
	return ret;

FAIL_DRV_INIT:
	nvt_dce_proc_remove(pdrv_info);

FAIL_DEV:
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		device_unregister(pdrv_info->pdevice[ucloop]);
	}

FAIL_CLASS:
	class_destroy(pdrv_info->pmodule_class);

FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

FAIL_FREE_REMAP:
	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		iounmap(pdrv_info->module_info.io_addr[ucloop]);
	}

FAIL_FREE_RES:
	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
	}

FAIL_FREE_BUF:
	kfree(pdrv_info);
	pdrv_info = NULL;
	return ret;
}

static int nvt_dce_remove(struct platform_device *pdev)
{
	PDCE_DRV_INFO pdrv_info;
	unsigned char ucloop;

	DBG_IND("\n");

	pdrv_info = platform_get_drvdata(pdev);

	nvt_dce_drv_remove(&pdrv_info->module_info);

	nvt_dce_proc_remove(pdrv_info);

	for (ucloop = 0 ; ucloop < MODULE_CLK_NUM; ucloop++) {
		clk_put(pdrv_info->module_info.pclk[ucloop]);
	}

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		device_unregister(pdrv_info->pdevice[ucloop]);
	}

	class_destroy(pdrv_info->pmodule_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		iounmap(pdrv_info->module_info.io_addr[ucloop]);
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
	}

	kfree(pdrv_info);
	pdrv_info = NULL;
	return 0;
}

static int nvt_dce_suspend(struct platform_device *pdev, pm_message_t state)
{
	PDCE_DRV_INFO pdrv_info;;

	DBG_IND("start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_dce_drv_suspend(&pdrv_info->module_info);

	DBG_IND("finished\n");
	return 0;
}


static int nvt_dce_resume(struct platform_device *pdev)
{
	PDCE_DRV_INFO pdrv_info;;

	DBG_IND("start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_dce_drv_resume(&pdrv_info->module_info);

	DBG_IND("finished\n");
	return 0;
}

static struct platform_driver nvt_dce_driver = {
	.driver = {
		.name   = "kdrv_dce",
		.owner = THIS_MODULE,
		.of_match_table = dce_match_table,
	},
	.probe      = nvt_dce_probe,
	.remove     = nvt_dce_remove,
	.suspend = nvt_dce_suspend,
	.resume = nvt_dce_resume
};

#if defined(_GROUP_KO_)
#undef __init
#undef __exit
#undef module_init
#undef module_exit
#define __init
#define __exit
#define module_init(x)
#define module_exit(x)
#ifndef MODULE // in built-in case
#undef MODULE_VERSION
#undef VOS_MODULE_VERSION
#define MODULE_VERSION(x)
#define VOS_MODULE_VERSION(a1, a2, a3, a4, a5)
#endif
#endif

int __init nvt_dce_module_init(void)
{
	int ret;

	ret = platform_driver_register(&nvt_dce_driver);

	return 0;
}

void __exit nvt_dce_module_exit(void)
{
	platform_driver_unregister(&nvt_dce_driver);
}

module_init(nvt_dce_module_init);
module_exit(nvt_dce_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("dce driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.00.000");
