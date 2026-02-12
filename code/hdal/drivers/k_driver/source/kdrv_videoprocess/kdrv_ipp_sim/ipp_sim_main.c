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

#include "ipp_sim_drv.h"
#include "ipp_sim_main.h"
#include "ipp_sim_proc.h"
#include "ipp_sim_dbg.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int ipp_sim_debug_level = NVT_DBG_WRN;
module_param_named(ipp_sim_debug_level, ipp_sim_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ipp_sim_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
#if 0
static struct of_device_id ipp_sim_match_table[] = {
	{   .compatible = "nvt,kdrv_ipp_sim"},
	{}
};
#endif

//=============================================================================
// function declaration
//=============================================================================
//static int nvt_ipp_sim_open(struct inode *inode, struct file *file);
//static int nvt_ipp_sim_release(struct inode *inode, struct file *file);
//static long nvt_ipp_sim_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
//static int nvt_ipp_sim_probe(struct platform_device *pdev);
//static int nvt_ipp_sim_suspend(struct platform_device *pdev, pm_message_t state);
//static int nvt_ipp_sim_resume(struct platform_device *pdev);
//static int nvt_ipp_sim_remove(struct platform_device *pdev);
int __init nvt_ipp_sim_module_init(void);
void __exit nvt_ipp_sim_module_exit(void);

//=============================================================================
// function define
//=============================================================================
#if 0
static int nvt_ipp_sim_open(struct inode *inode, struct file *file)
{
	IPP_SIM_DRV_INFO *pdrv_info;

	pdrv_info = container_of(inode->i_cdev, IPP_SIM_DRV_INFO, cdev);

	pdrv_info = container_of(inode->i_cdev, IPP_SIM_DRV_INFO, cdev);
	file->private_data = pdrv_info;

	if (nvt_ipp_sim_drv_open(&pdrv_info->ipp_sim_info, MINOR(inode->i_rdev))) {
		nvt_dbg(ERR, "failed to open driver\n");
		return -1;
	}

	return 0;
}

static int nvt_ipp_sim_release(struct inode *inode, struct file *file)
{
	IPP_SIM_DRV_INFO *pdrv_info;
	pdrv_info = container_of(inode->i_cdev, IPP_SIM_DRV_INFO, cdev);

	nvt_ipp_sim_drv_release(&pdrv_info->ipp_sim_info, MINOR(inode->i_rdev));
	return 0;
}

static long nvt_ipp_sim_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PIPP_SIM_DRV_INFO pdrv;

	inode = file_inode(filp);
	pdrv = filp->private_data;

	return nvt_ipp_sim_drv_ioctl(MINOR(inode->i_rdev), &pdrv->ipp_sim_info, cmd, arg);
}

struct file_operations nvt_ipp_sim_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_ipp_sim_open,
	.release = nvt_ipp_sim_release,
	.unlocked_ioctl = nvt_ipp_sim_ioctl,
	.llseek  = no_llseek,
};
#endif

static IPP_SIM_DRV_INFO *pdrv_ipp_sim_info = NULL;//info;


#if 0
static int nvt_ipp_sim_probe(struct platform_device *pdev)
{
	IPP_SIM_DRV_INFO *pdrv_info;//info;
	//const struct of_device_id *match;
	int ret = 0;
	unsigned char ucloop;

	nvt_dbg(IND, "nvt_ipp_sim_probe - 1\r\n");

#if 0
	match = of_match_device(ipp_sim_match_table, &pdev->dev);
	if (!match) {
		nvt_dbg(ERR, "Platform device not found \n");
		return -EINVAL;
	}
#endif

	pdrv_info = kzalloc(sizeof(IPP_SIM_DRV_INFO), GFP_KERNEL);
	if (!pdrv_info) {
		nvt_dbg(ERR, "failed to allocate memory\n");
		return -ENOMEM;
	}

#if 0
	for (ucloop = 0 ; ucloop < IPP_SIM_REG_NUM ; ucloop++) {
		pdrv_info->presource[ucloop] = platform_get_resource(pdev, IORESOURCE_MEM, ucloop);
		if (pdrv_info->presource[ucloop] == NULL) {
			nvt_dbg(ERR, "No IO memory resource defined:%d\n", ucloop);
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}
#endif

#if 0
	for (ucloop = 0 ; ucloop < IPP_SIM_REG_NUM ; ucloop++) {
		//nvt_dbg(IND, "%d. resource:0x%x size:0x%x\n", ucloop, pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (!request_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]), pdev->name)) {
			nvt_dbg(ERR, "failed to request memory resource%d\n", ucloop);
			//for (; ucloop > 0 ;) {
			//  ucloop -= 1;
			//  release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
			//}

			release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));

			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}
#endif

#if 0
	for (ucloop = 0 ; ucloop < IPP_SIM_REG_NUM ; ucloop++) {
		pdrv_info->ipp_sim_info.io_addr[ucloop] = ioremap_nocache(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (pdrv_info->ipp_sim_info.io_addr[ucloop] == NULL) {
			nvt_dbg(ERR, "ioremap() failed in module%d\n", ucloop);

			//for (; ucloop > 0 ;) {
			//  ucloop -= 1;
			//  iounmap(pdrv_info->ipp_sim_info.io_addr[ucloop]);
			//}
			iounmap(pdrv_info->ipp_sim_info.io_addr[ucloop]);
			ret = -ENODEV;
			goto FAIL_FREE_RES;
		}
	}
#endif

#if 0
	for (ucloop = 0 ; ucloop < IPP_SIM_IRQ_NUM; ucloop++) {
		pdrv_info->ipp_sim_info.iinterrupt_id[ucloop] = platform_get_irq(pdev, ucloop);
		//nvt_dbg(IND, "IRQ %d. ID%d\n", ucloop, pdrv_info->ipp_sim_info.iinterrupt_id[ucloop]);
		if (pdrv_info->ipp_sim_info.iinterrupt_id[ucloop] < 0) {
			nvt_dbg(ERR, "No IRQ resource defined\n");
			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}
#endif

	//Get clock source
#if 0
	for (ucloop = 0 ; ucloop < 1; ucloop++) {
		pdrv_info->ipp_sim_info.pclk[ucloop] = clk_get(&pdev->dev, dev_name(&pdev->dev));
		if (IS_ERR(pdrv_info->ipp_sim_info.pclk[ucloop])) {
			nvt_dbg(ERR, "faile to get clock%d source\n", ucloop);

			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}
	//nvt_dbg(IND, "ipp_sim: get clock...\r\n");
#endif


	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, IPP_SIM_MINOR_COUNT, IPP_SIM_NAME)) {
		nvt_dbg(ERR, "Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_REMAP;
	}

	nvt_dbg(IND, "nvt_ipp_sim_probe - 2\r\n");

	//nvt_dbg(IND, "DevID Major:%d minor:%d\n", MAJOR(pdrv_info->dev_id), MINOR(pdrv_info->dev_id));

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_ipp_sim_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	nvt_dbg(IND, "nvt_ipp_sim_probe - 3\r\n");

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, IPP_SIM_MINOR_COUNT)) {
		nvt_dbg(ERR, "Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	nvt_dbg(IND, "nvt_ipp_sim_probe - 4\r\n");

	pdrv_info->pipp_sim_class = class_create(THIS_MODULE, "nvt_ipp_sim");
	if (IS_ERR(pdrv_info->pipp_sim_class)) {
		nvt_dbg(ERR, "failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	nvt_dbg(IND, "nvt_ipp_sim_probe - 5\r\n");

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (IPP_SIM_MINOR_COUNT) ; ucloop++) {
		pdrv_info->pdevice[ucloop] = device_create(pdrv_info->pipp_sim_class, NULL
									 , MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
									 , IPP_SIM_NAME"%d", ucloop);

		if (IS_ERR(pdrv_info->pdevice[ucloop])) {
			nvt_dbg(ERR, "failed in creating device%d.\n", ucloop);

			//while (ucloop >= 0) {
			//  device_unregister(pdrv_info->pdevice[ucloop]);
			//  ucloop--;
			//}
			device_unregister(pdrv_info->pdevice[ucloop]);

			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}

	nvt_dbg(IND, "nvt_ipp_sim_probe - 6\r\n");

	ret = nvt_ipp_sim_proc_init(pdrv_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DEV;
	}

	nvt_dbg(IND, "nvt_ipp_sim_probe - 7\r\n");

	ret = nvt_ipp_sim_drv_init(&pdrv_info->ipp_sim_info);

	nvt_dbg(IND, "nvt_ipp_sim_probe - 8\r\n");

	platform_set_drvdata(pdev, pdrv_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;

FAIL_DRV_INIT:
	nvt_ipp_sim_proc_remove(pdrv_info);

FAIL_DEV:
	for (ucloop = 0 ; ucloop < (IPP_SIM_MINOR_COUNT) ; ucloop++) {
		device_unregister(pdrv_info->pdevice[ucloop]);
	}

FAIL_CLASS:
	class_destroy(pdrv_info->pipp_sim_class);

FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, IPP_SIM_MINOR_COUNT);

FAIL_FREE_REMAP:
	//for (ucloop = 0 ; ucloop < IPP_SIM_REG_NUM ; ucloop++) {
	//  iounmap(pdrv_info->ipp_sim_info.io_addr[ucloop]);
	//}

//FAIL_FREE_RES:
	//for (ucloop = 0 ; ucloop < IPP_SIM_REG_NUM ; ucloop++) {
	//  release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
	//}

//FAIL_FREE_BUF:
//	kfree(pdrv_info);
//	pdrv_info = NULL;


	return ret;
}
#endif

#if 0
static int nvt_ipp_sim_remove(struct platform_device *pdev)
{
	PIPP_SIM_DRV_INFO pdrv_info;
	unsigned char ucloop;

	//nvt_dbg(IND, "\n");

	pdrv_info = platform_get_drvdata(pdev);

	nvt_ipp_sim_drv_remove(&pdrv_info->ipp_sim_info);

	nvt_ipp_sim_proc_remove(pdrv_info);

	//for (ucloop = 0 ; ucloop < IPP_SIM_CLK_NUM; ucloop++) {
	//  clk_put(pdrv_info->ipp_sim_info.pclk[ucloop]);
	//}

	for (ucloop = 0 ; ucloop < (IPP_SIM_MINOR_COUNT) ; ucloop++) {
		device_unregister(pdrv_info->pdevice[ucloop]);
	}

	class_destroy(pdrv_info->pipp_sim_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, IPP_SIM_MINOR_COUNT);

	//for (ucloop = 0 ; ucloop < IPP_SIM_REG_NUM ; ucloop++) {
	//  iounmap(pdrv_info->ipp_sim_info.io_addr[ucloop]);
	//}

	//for (ucloop = 0 ; ucloop < IPP_SIM_REG_NUM ; ucloop++) {
	//  release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
	//}

	kfree(pdrv_info);
	pdrv_info = NULL;
	return 0;
}
#endif

#if 0
static int nvt_ipp_sim_suspend(struct platform_device *pdev, pm_message_t state)
{
	PIPP_SIM_DRV_INFO pdrv_info;;

	//nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_ipp_sim_drv_suspend(&pdrv_info->ipp_sim_info);

	//nvt_dbg(IND, "finished\n");
	return 0;
}
#endif

#if 0
static int nvt_ipp_sim_resume(struct platform_device *pdev)
{
	PIPP_SIM_DRV_INFO pdrv_info;;

	//nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_ipp_sim_drv_resume(&pdrv_info->ipp_sim_info);

	//nvt_dbg(IND, "finished\n");
	return 0;
}
#endif

#if 0
static struct platform_driver nvt_ipp_sim_driver = {
	.driver = {
		.name   = "kdrv_ipp_sim",
		.owner = THIS_MODULE,
		.of_match_table = ipp_sim_match_table,
	},
	.probe      = nvt_ipp_sim_probe,
	.remove     = nvt_ipp_sim_remove,
	.suspend = nvt_ipp_sim_suspend,
	.resume = nvt_ipp_sim_resume
};
#endif

int __init nvt_ipp_sim_module_init(void)
{
	int ret;

	nvt_dbg(IND, "\n");
	//ret = platform_driver_register(&nvt_ipp_sim_driver);

	pdrv_ipp_sim_info = kzalloc(sizeof(IPP_SIM_DRV_INFO), GFP_KERNEL);
	if (!pdrv_ipp_sim_info) {
		nvt_dbg(ERR, "failed to allocate memory\n");
		return -ENOMEM;
	}

	ret = nvt_ipp_sim_proc_init(pdrv_ipp_sim_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");

		kfree(pdrv_ipp_sim_info);

		pdrv_ipp_sim_info = NULL;
		return -1;
	}

	ret = nvt_ipp_sim_drv_init(&pdrv_ipp_sim_info->ipp_sim_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating drv.\n");

		kfree(pdrv_ipp_sim_info);

		pdrv_ipp_sim_info = NULL;
		return -1;
	}

	nvt_dbg(IND, "nvt_ipp_sim_module_init, done\r\n");

	return 0;
}

void __exit nvt_ipp_sim_module_exit(void)
{
	int ret;
	//nvt_dbg(WRN, "\n");
	//platform_driver_unregister(&nvt_ipp_sim_driver);

	ret = nvt_ipp_sim_proc_remove(pdrv_ipp_sim_info);

	kfree(pdrv_ipp_sim_info);

	pdrv_ipp_sim_info = NULL;

	nvt_dbg(IND, "nvt_ipp_sim_module_exit...\n");
}

module_init(nvt_ipp_sim_module_init);
module_exit(nvt_ipp_sim_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("ipp_sim k-driver");
MODULE_LICENSE("GPL");
