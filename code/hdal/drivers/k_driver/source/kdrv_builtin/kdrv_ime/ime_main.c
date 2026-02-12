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

#include "ime_drv.h"
#include "ime_main.h"
#include "ime_proc.h"
#include "ime_dbg.h"

#include "ime_platform.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int ime_debug_level = NVT_DBG_IND;
module_param_named(ime_debug_level, ime_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ime_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id ime_match_table[] = {
	{   .compatible = "nvt,kdrv_ime"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int nvt_ime_open(struct inode *inode, struct file *file);
static int nvt_ime_release(struct inode *inode, struct file *file);
//static long nvt_ime_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int nvt_ime_probe(struct platform_device *pdev);
static int nvt_ime_suspend(struct platform_device *pdev, pm_message_t state);
static int nvt_ime_resume(struct platform_device *pdev);
static int nvt_ime_remove(struct platform_device *pdev);
int __init nvt_ime_module_init(void);
void __exit nvt_ime_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int nvt_ime_open(struct inode *inode, struct file *file)
{
	IME_DRV_INFO *pdrv_info;

	pdrv_info = container_of(vos_icdev(inode), IME_DRV_INFO, cdev);
	file->private_data = pdrv_info;

	if (nvt_ime_drv_open(&pdrv_info->ime_info, MINOR(vos_irdev(inode)))) {
		nvt_dbg(ERR, "failed to open driver\n");
		return -1;
	}

	return 0;
}

static int nvt_ime_release(struct inode *inode, struct file *file)
{
	IME_DRV_INFO *pdrv_info;
	pdrv_info = container_of(vos_icdev(inode), IME_DRV_INFO, cdev);

	nvt_ime_drv_release(&pdrv_info->ime_info, MINOR(vos_irdev(inode)));
	return 0;
}

#if 0
static long nvt_ime_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PIME_DRV_INFO pdrv;

	inode = file_inode(filp);
	pdrv = filp->private_data;

	return nvt_ime_drv_ioctl(MINOR(vos_irdev(inode)), &pdrv->ime_info, cmd, arg);
}
#endif

struct file_operations nvt_ime_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_ime_open,
	.release = nvt_ime_release,
	//.unlocked_ioctl = nvt_ime_ioctl,
	.llseek  = no_llseek,
};

static int nvt_ime_probe(struct platform_device *pdev)
{
	IME_DRV_INFO *pdrv_info = NULL;//info;
	const struct of_device_id *match;
	int ret = 0;
	unsigned char ucloop;

	//nvt_dbg(IND, "%s\r\n", pdev->name);

	match = of_match_device(ime_match_table, &pdev->dev);
	if (!match) {
		nvt_dbg(ERR, "Platform device not found \n");
		return -EINVAL;
	}

	pdrv_info = kzalloc(sizeof(IME_DRV_INFO), GFP_KERNEL);
	if (!pdrv_info) {
		nvt_dbg(ERR, "failed to allocate memory\n");
		return -ENOMEM;
	}

	for (ucloop = 0 ; ucloop < IME_REG_NUM ; ucloop++) {
		pdrv_info->presource[ucloop] = platform_get_resource(pdev, IORESOURCE_MEM, ucloop);
		if (pdrv_info->presource[ucloop] == NULL) {
			nvt_dbg(ERR, "No IO memory resource defined:%d\n", (int)ucloop);
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < IME_REG_NUM ; ucloop++) {
		//nvt_dbg(IND, "%d. resource:0x%x size:0x%x\n", ucloop, pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (!request_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]), pdev->name)) {
			nvt_dbg(ERR, "failed to request memory resource%d\n", (int)ucloop);
			//for (; ucloop > 0 ;) {
			//  ucloop -= 1;
			//  release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
			//}

			release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));

			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < IME_REG_NUM ; ucloop++) {
		pdrv_info->ime_info.io_addr[ucloop] = ioremap_nocache(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (pdrv_info->ime_info.io_addr[ucloop] == NULL) {
			nvt_dbg(ERR, "ioremap() failed in module%d\n", (int)ucloop);

			//for (; ucloop > 0 ;) {
			//  ucloop -= 1;
			//  iounmap(pdrv_info->ime_info.io_addr[ucloop]);
			//}
			iounmap(pdrv_info->ime_info.io_addr[ucloop]);
			ret = -ENODEV;
			goto FAIL_FREE_RES;
		}
	}

	for (ucloop = 0 ; ucloop < IME_IRQ_NUM; ucloop++) {
		pdrv_info->ime_info.iinterrupt_id[ucloop] = platform_get_irq(pdev, ucloop);
		//nvt_dbg(IND, "IRQ %d. ID%d\n", ucloop, pdrv_info->ime_info.iinterrupt_id[ucloop]);
		if (pdrv_info->ime_info.iinterrupt_id[ucloop] < 0) {
			nvt_dbg(ERR, "No IRQ resource defined\n");
			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}

	//Get clock source
#if 1
	for (ucloop = 0 ; ucloop < IME_CLK_NUM; ucloop++) {
		pdrv_info->ime_info.pclk[ucloop] = clk_get(&pdev->dev, dev_name(&pdev->dev));
		if (IS_ERR(pdrv_info->ime_info.pclk[ucloop])) {
			nvt_dbg(ERR, "faile to get ime clock%d source\n", (int)ucloop);

			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}
	//nvt_dbg(IND, "ime get clock...\r\n");
#endif

	//Get pclock source
	for (ucloop = 0 ; ucloop < IME_CLK_NUM; ucloop++) {
		pdrv_info->ime_info.p_pclk[ucloop] = clk_get(&pdev->dev, "ime_pclk");
		if (IS_ERR(pdrv_info->ime_info.p_pclk[ucloop])) {
			DBG_ERR("failed to get ime pclock%d source\n", ucloop);

			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}

	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, IME_MINOR_COUNT, IME_NAME)) {
		nvt_dbg(ERR, "Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_REMAP;
	}

	//nvt_dbg(IND, "DevID Major:%d minor:%d\n", MAJOR(pdrv_info->dev_id), MINOR(pdrv_info->dev_id));

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_ime_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, IME_MINOR_COUNT)) {
		nvt_dbg(ERR, "Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	pdrv_info->pime_class = class_create(THIS_MODULE, "nvt_ime");
	if (IS_ERR(pdrv_info->pime_class)) {
		nvt_dbg(ERR, "failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (IME_MINOR_COUNT) ; ucloop++) {
		pdrv_info->pdevice[ucloop] = device_create(pdrv_info->pime_class, NULL
									 , MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
									 , IME_NAME"%d", ucloop);

		if (IS_ERR(pdrv_info->pdevice[ucloop])) {
			nvt_dbg(ERR, "failed in creating device%d.\n", (int)ucloop);

			//while (ucloop >= 0) {
			//  device_unregister(pdrv_info->pdevice[ucloop]);
			//  ucloop--;
			//}
			device_unregister(pdrv_info->pdevice[ucloop]);

			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}

	ret = nvt_ime_proc_init(pdrv_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DEV;
	}

	platform_set_drvdata(pdev, pdrv_info);

	ret = nvt_ime_drv_init(&pdrv_info->ime_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	/* register IRQ here*/
	if (request_irq(pdrv_info->ime_info.iinterrupt_id[0], nvt_ime_drv_isr, IRQF_TRIGGER_HIGH, "IME_INT", &pdrv_info->ime_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", (int)pdrv_info->ime_info.iinterrupt_id[0]);
		ret = -ENODEV;
		goto FAIL_FREE_IRQ;
	}

	return ret;

FAIL_FREE_IRQ:
	free_irq(pdrv_info->ime_info.iinterrupt_id[0], &pdrv_info->ime_info);
	return ret;


FAIL_DRV_INIT:
	nvt_ime_proc_remove(pdrv_info);

FAIL_DEV:
	for (ucloop = 0 ; ucloop < (IME_MINOR_COUNT) ; ucloop++) {
		device_unregister(pdrv_info->pdevice[ucloop]);
	}

FAIL_CLASS:
	class_destroy(pdrv_info->pime_class);

FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, IME_MINOR_COUNT);

FAIL_FREE_REMAP:
	for (ucloop = 0 ; ucloop < IME_REG_NUM ; ucloop++) {
		iounmap(pdrv_info->ime_info.io_addr[ucloop]);
	}

FAIL_FREE_RES:
	for (ucloop = 0 ; ucloop < IME_REG_NUM ; ucloop++) {
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
	}

FAIL_FREE_BUF:
	if (pdrv_info != NULL) {
		kfree(pdrv_info);

		// coverity[assigned_pointer]
		pdrv_info = NULL;
	}

	return ret;
}

static int nvt_ime_remove(struct platform_device *pdev)
{
	PIME_DRV_INFO pdrv_info = NULL;
	unsigned char ucloop;

	//nvt_dbg(IND, "\n");

	pdrv_info = platform_get_drvdata(pdev);

	nvt_ime_drv_remove(&pdrv_info->ime_info);

	nvt_ime_proc_remove(pdrv_info);

	for (ucloop = 0 ; ucloop < IME_CLK_NUM; ucloop++) {
		clk_put(pdrv_info->ime_info.pclk[ucloop]);
	}

	for (ucloop = 0 ; ucloop < (IME_MINOR_COUNT) ; ucloop++) {
		device_unregister(pdrv_info->pdevice[ucloop]);
	}

	class_destroy(pdrv_info->pime_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, IME_MINOR_COUNT);

	for (ucloop = 0 ; ucloop < IME_REG_NUM ; ucloop++) {
		iounmap(pdrv_info->ime_info.io_addr[ucloop]);
	}

	for (ucloop = 0 ; ucloop < IME_REG_NUM ; ucloop++) {
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
	}

	// coverity[check_after_deref]
	if (pdrv_info != NULL) {
		kfree(pdrv_info);

		// coverity[assigned_pointer]
		pdrv_info = NULL;
	}

	return 0;
}

static int nvt_ime_suspend(struct platform_device *pdev, pm_message_t state)
{
	PIME_DRV_INFO pdrv_info;;

	//nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_ime_drv_suspend(&pdrv_info->ime_info);

	//nvt_dbg(IND, "finished\n");
	return 0;
}


static int nvt_ime_resume(struct platform_device *pdev)
{
	PIME_DRV_INFO pdrv_info;;

	//nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_ime_drv_resume(&pdrv_info->ime_info);

	//nvt_dbg(IND, "finished\n");
	return 0;
}

static struct platform_driver nvt_ime_driver = {
	.driver = {
		.name   = "kdrv_ime",
		.owner = THIS_MODULE,
		.of_match_table = ime_match_table,
	},
	.probe      = nvt_ime_probe,
	.remove     = nvt_ime_remove,
	.suspend = nvt_ime_suspend,
	.resume = nvt_ime_resume
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
#endif

int __init nvt_ime_module_init(void)
{
	int ret;

	//nvt_dbg(WRN, "\n");
	ret = platform_driver_register(&nvt_ime_driver);

	return 0;
}

void __exit nvt_ime_module_exit(void)
{
	//nvt_dbg(WRN, "\n");
	platform_driver_unregister(&nvt_ime_driver);
}

module_init(nvt_ime_module_init);
module_exit(nvt_ime_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("ime k-driver");
MODULE_LICENSE("GPL");
