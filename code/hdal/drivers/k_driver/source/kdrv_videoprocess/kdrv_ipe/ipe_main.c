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

//#include "uitron_wrapper/type.h"
#include "ipe_drv.h"
#include "ipe_reg.h"
#include "ipe_main.h"
#include "ipe_proc.h"
#include "ipe_dbg.h"


//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int ipe_debug_level = (NVT_DBG_IND | NVT_DBG_WRN | NVT_DBG_ERR);
module_param_named(ipe_debug_level, ipe_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ipe_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id ipe_match_table[] = {
	{   .compatible = "nvt,kdrv_ipe"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int nvt_ipe_open(struct inode *inode, struct file *file);
static int nvt_ipe_release(struct inode *inode, struct file *file);
// NOTE: XXXXX
//static long nvt_ipe_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int nvt_ipe_probe(struct platform_device *pdev);
static int nvt_ipe_suspend(struct platform_device *pdev, pm_message_t state);
static int nvt_ipe_resume(struct platform_device *pdev);
static int nvt_ipe_remove(struct platform_device *pdev);
int __init nvt_ipe_module_init(void);
void __exit nvt_ipe_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int nvt_ipe_open(struct inode *inode, struct file *file)
{
	IPE_DRV_INFO *pdrv_info;

	pdrv_info = container_of(vos_icdev(inode), IPE_DRV_INFO, cdev);
	file->private_data = pdrv_info;

	if (nvt_ipe_drv_open(&pdrv_info->module_info, MINOR(vos_irdev(inode)))) {
		nvt_dbg(ERR, "failed to open driver\n");
		return -1;
	}

	return 0;
}

static int nvt_ipe_release(struct inode *inode, struct file *file)
{
	IPE_DRV_INFO *pdrv_info;

	pdrv_info = container_of(vos_icdev(inode), IPE_DRV_INFO, cdev);
	nvt_ipe_drv_release(&pdrv_info->module_info, MINOR(vos_irdev(inode)));
	return 0;
}

// NOTE: XXXXX
#if 0
static long nvt_ipe_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PIPE_DRV_INFO pdrv;

	inode = file_inode(filp);
	pdrv = filp->private_data;

	return nvt_ipe_drv_ioctl(MINOR(vos_irdev(inode)), &pdrv->module_info, cmd, arg);
}
#endif

struct file_operations nvt_ipe_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_ipe_open,
	.release = nvt_ipe_release,
	// NOTE: XXXXX
	//.unlocked_ioctl = nvt_ipe_ioctl,
	.llseek  = no_llseek,
};

static int nvt_ipe_probe(struct platform_device *pdev)
{
	IPE_DRV_INFO *pdrv_info;
	const struct of_device_id *match;
	int ret = 0;
	int ucloop;

	//nvt_dbg(IND, "kdrv %s\n", pdev->name);

	match = of_match_device(ipe_match_table, &pdev->dev);
	if (!match) {
		nvt_dbg(ERR, "Platform device not found \n");
		return -EINVAL;
	}

	pdrv_info = kzalloc(sizeof(IPE_DRV_INFO), GFP_KERNEL);
	if (!pdrv_info) {
		nvt_dbg(ERR, "failed to allocate memory\n");
		return -ENOMEM;
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		pdrv_info->presource[ucloop] = platform_get_resource(pdev, IORESOURCE_MEM, ucloop);
		if (pdrv_info->presource[ucloop] == NULL) {
			nvt_dbg(ERR, "No IO memory resource defined:%d\n", ucloop);
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		//nvt_dbg(IND, "%d. resource:0x%x size:0x%x\n", ucloop, pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (!request_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]), pdev->name)) {
			nvt_dbg(ERR, "failed to request memory resource%d\n", ucloop);
			while (ucloop >= 0) {
				release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
				ucloop--;
			}
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		pdrv_info->module_info.io_addr[ucloop] = ioremap_nocache(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (pdrv_info->module_info.io_addr[ucloop] == NULL) {
			nvt_dbg(ERR, "ioremap() failed in module%d\n", ucloop);
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
		//nvt_dbg(IND, "IRQ %d. ID%d\n", ucloop, pdrv_info->module_info.iinterrupt_id[ucloop]);
		if (pdrv_info->module_info.iinterrupt_id[ucloop] < 0) {
			nvt_dbg(ERR, "No IRQ resource defined\n");
			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}

	//Get clock source
	for (ucloop = 0 ; ucloop < MODULE_CLK_NUM; ucloop++) {
		pdrv_info->module_info.pclk[ucloop] = clk_get(&pdev->dev, dev_name(&pdev->dev));
		if (IS_ERR(pdrv_info->module_info.pclk[ucloop])) {
			nvt_dbg(ERR, "faile to get ipe clock%d source\n", ucloop);

			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}
	//nvt_dbg(IND, " get clock source\r\n");

	//Get pclock source
	for (ucloop = 0 ; ucloop < MODULE_CLK_NUM; ucloop++) {
		pdrv_info->module_info.p_pclk[ucloop] = clk_get(&pdev->dev, "ipe_pclk");
		if (IS_ERR(pdrv_info->module_info.p_pclk[ucloop])) {
			DBG_ERR("failed to get ipe pclock%d source\n", ucloop);

			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}


	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, MODULE_MINOR_COUNT, MODULE_NAME)) {
		nvt_dbg(ERR, "Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_REMAP;
	}

	//nvt_dbg(IND, "DevID Major:%d minor:%d\n", MAJOR(pdrv_info->dev_id), MINOR(pdrv_info->dev_id));

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_ipe_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, MODULE_MINOR_COUNT)) {
		nvt_dbg(ERR, "Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	pdrv_info->pmodule_class = class_create(THIS_MODULE, "kdrv_ipe");
	if (IS_ERR(pdrv_info->pmodule_class)) {
		nvt_dbg(ERR, "failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		pdrv_info->pdevice[ucloop] = device_create(pdrv_info->pmodule_class, NULL
									 , MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
									 , MODULE_NAME"%d", ucloop);

		if (IS_ERR(pdrv_info->pdevice[ucloop])) {
			nvt_dbg(ERR, "failed in creating device%d.\n", ucloop);

			while (ucloop >= 0) {
				device_unregister(pdrv_info->pdevice[ucloop]);
				ucloop--;
			}
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}

	ret = nvt_ipe_proc_init(pdrv_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DEV;
	}

	platform_set_drvdata(pdev, pdrv_info);

	ret = nvt_ipe_drv_init(&pdrv_info->module_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	/* register IRQ here*/
	if (request_irq(pdrv_info->module_info.iinterrupt_id[0], nvt_ipe_drv_isr, IRQF_TRIGGER_HIGH, "IPE_INT", &pdrv_info->module_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pdrv_info->module_info.iinterrupt_id[0]);
		ret = -ENODEV;
		goto FAIL_FREE_IRQ;
	}

	return ret;

FAIL_FREE_IRQ:
	free_irq(pdrv_info->module_info.iinterrupt_id[0], &pdrv_info->module_info);
	return ret;


FAIL_DRV_INIT:
	nvt_ipe_proc_remove(pdrv_info);

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

static int nvt_ipe_remove(struct platform_device *pdev)
{
	PIPE_DRV_INFO pdrv_info;
	unsigned char ucloop;

	//nvt_dbg(IND, "\n");

	pdrv_info = platform_get_drvdata(pdev);

	nvt_ipe_drv_remove(&pdrv_info->module_info);

	nvt_ipe_proc_remove(pdrv_info);

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

static int nvt_ipe_suspend(struct platform_device *pdev, pm_message_t state)
{
	PIPE_DRV_INFO pdrv_info;

	//nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_ipe_drv_suspend(&pdrv_info->module_info);

	//nvt_dbg(IND, "finished\n");
	return 0;
}


static int nvt_ipe_resume(struct platform_device *pdev)
{
	PIPE_DRV_INFO pdrv_info;;

	//nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_ipe_drv_resume(&pdrv_info->module_info);

	//nvt_dbg(IND, "finished\n");
	return 0;
}

static struct platform_driver nvt_ipe_driver = {
	.driver = {
		.name   = "kdrv_ipe",
		.owner = THIS_MODULE,
		.of_match_table = ipe_match_table,
	},
	.probe      = nvt_ipe_probe,
	.remove     = nvt_ipe_remove,
	.suspend = nvt_ipe_suspend,
	.resume = nvt_ipe_resume
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

int __init nvt_ipe_module_init(void)
{
	int ret;

	//nvt_dbg(WRN, "\n");
	ret = platform_driver_register(&nvt_ipe_driver);

	return 0;
}

void __exit nvt_ipe_module_exit(void)
{
	//nvt_dbg(WRN, "\n");
	platform_driver_unregister(&nvt_ipe_driver);
}

module_init(nvt_ipe_module_init);
module_exit(nvt_ipe_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("ipe driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.00.000");
