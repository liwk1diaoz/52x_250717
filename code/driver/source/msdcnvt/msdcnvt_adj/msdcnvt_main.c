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
#include <linux/mm.h>
#include <asm/signal.h>
#include "msdcnvt_int.h"
#include "msdcnvt_drv.h"
#include "msdcnvt_main.h"
#include "msdcnvt_adj.h"
//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int msdcnvt_adj_debug_level = NVT_DBG_WRN;//(NVT_DBG_INFO | NVT_DBG_WARN | NVT_DBG_ERR);
module_param_named(msdcnvt_adj_debug_level, msdcnvt_adj_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(msdcnvt_adj_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id msdcnvt_match_table[] = {
	{   .compatible = "nvt,msdcnvt_adj"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int msdcnvt_open(struct inode *inode, struct file *file);
static int msdcnvt_release(struct inode *inode, struct file *file);
static int msdcnvt_probe(struct platform_device *p_dev);
static int msdcnvt_suspend(struct platform_device *p_dev, pm_message_t state);
static int msdcnvt_resume(struct platform_device *p_dev);
static int msdcnvt_remove(struct platform_device *p_dev);
int __init msdcnvt_module_init(void);
void __exit msdcnvt_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int msdcnvt_open(struct inode *inode, struct file *file)
{
	MSDCNVT_DRV_INFO *p_drv_info;

	p_drv_info = container_of(inode->i_cdev, MSDCNVT_DRV_INFO, cdev);
	file->private_data = p_drv_info;

	if (msdcnvt_drv_open(&p_drv_info->module_info, MINOR(inode->i_rdev))) {
		DBG_ERR("failed to open driver\n");
		return -1;
	}

	return 0;
}

static int msdcnvt_release(struct inode *inode, struct file *file)
{
	MSDCNVT_DRV_INFO *p_drv_info;

	p_drv_info = container_of(inode->i_cdev, MSDCNVT_DRV_INFO, cdev);
	msdcnvt_drv_release(&p_drv_info->module_info, MINOR(inode->i_rdev));
	file->private_data = NULL;
	return 0;
}

struct file_operations msdcnvt_fops = {
	.owner   = THIS_MODULE,
	.open    = msdcnvt_open,
	.release = msdcnvt_release,
	.llseek  = no_llseek,
};

static int msdcnvt_probe(struct platform_device *p_dev)
{
	MSDCNVT_DRV_INFO *p_drv_info;
	int ret = 0;
	unsigned char ucloop;

	DBG_IND("%s\n", p_dev->name);

	p_drv_info = kzalloc(sizeof(MSDCNVT_DRV_INFO), GFP_KERNEL);
	if (!p_drv_info) {
		DBG_ERR("failed to allocate memory\n");
		return -ENOMEM;
	}

	//Dynamic to allocate Device ID
	if (alloc_chrdev_region(&p_drv_info->dev_id, MODULE_MINOR_ID, MODULE_MINOR_COUNT, MODULE_NAME)) {
		DBG_ERR("Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_REMAP;
	}

	DBG_IND("DevID Major:%d minor:%d\n" \
			, MAJOR(p_drv_info->dev_id), MINOR(p_drv_info->dev_id));

	/* Register character device for the volume */
	cdev_init(&p_drv_info->cdev, &msdcnvt_fops);
	p_drv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&p_drv_info->cdev, p_drv_info->dev_id, MODULE_MINOR_COUNT)) {
		DBG_ERR("Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	p_drv_info->pmodule_class = class_create(THIS_MODULE, MODULE_NAME);
	if (IS_ERR(p_drv_info->pmodule_class)) {
		DBG_ERR("failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		p_drv_info->p_device[ucloop] = device_create(p_drv_info->pmodule_class, NULL
									   , MKDEV(MAJOR(p_drv_info->dev_id), (ucloop + MODULE_MINOR_ID)), NULL
									   , MODULE_NAME"%d", ucloop);

		if (IS_ERR(p_drv_info->p_device[ucloop])) {
			DBG_ERR("failed in creating device%d.\n", ucloop);
#if (MODULE_MINOR_COUNT > 1)
			for (; ucloop > 0 ; ucloop--) {
				device_unregister(p_drv_info->p_device[ucloop - 1]);
			}
#endif			
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}
	
	ret = msdcnvt_drv_init(&p_drv_info->module_info);

	platform_set_drvdata(p_dev, p_drv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;

FAIL_DRV_INIT:
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		device_unregister(p_drv_info->p_device[ucloop]);
	}

FAIL_CLASS:
	class_destroy(p_drv_info->pmodule_class);

FAIL_CDEV:
	cdev_del(&p_drv_info->cdev);
	unregister_chrdev_region(p_drv_info->dev_id, MODULE_MINOR_COUNT);

FAIL_FREE_REMAP:

	kfree(p_drv_info);
	return ret;
}

static int msdcnvt_remove(struct platform_device *p_dev)
{
	PMSDCNVT_DRV_INFO p_drv_info;
	unsigned char ucloop;

	DBG_IND("\n");

	p_drv_info = platform_get_drvdata(p_dev);

	msdcnvt_drv_remove(&p_drv_info->module_info);

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		device_unregister(p_drv_info->p_device[ucloop]);
	}

	class_destroy(p_drv_info->pmodule_class);
	cdev_del(&p_drv_info->cdev);
	unregister_chrdev_region(p_drv_info->dev_id, MODULE_MINOR_COUNT);

	kfree(p_drv_info);
	return 0;
}

static int msdcnvt_suspend(struct platform_device *p_dev, pm_message_t state)
{
	PMSDCNVT_DRV_INFO p_drv_info;;

	DBG_IND("start\n");

	p_drv_info = platform_get_drvdata(p_dev);
	msdcnvt_drv_suspend(&p_drv_info->module_info);

	DBG_IND("finished\n");
	return 0;
}


static int msdcnvt_resume(struct platform_device *p_dev)
{
	PMSDCNVT_DRV_INFO p_drv_info;;

	DBG_IND("start\n");

	p_drv_info = platform_get_drvdata(p_dev);
	msdcnvt_drv_resume(&p_drv_info->module_info);

	DBG_IND("finished\n");
	return 0;
}

static struct platform_driver msdcnvt_driver = {
	.driver = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = msdcnvt_match_table,
	},
	.probe = msdcnvt_probe,
	.remove = msdcnvt_remove,
	.suspend = msdcnvt_suspend,
	.resume = msdcnvt_resume
};

int __init msdcnvt_module_init(void)
{
	int ret;

	ret = platform_driver_register(&msdcnvt_driver);

	return ret;
}

void __exit msdcnvt_module_exit(void)
{
	platform_driver_unregister(&msdcnvt_driver);
}

module_init(msdcnvt_module_init);
module_exit(msdcnvt_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("msdcnvt_adj driver");
MODULE_LICENSE("GPL");
