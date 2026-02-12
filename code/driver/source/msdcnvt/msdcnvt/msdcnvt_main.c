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
#include "msdcnvt_drv.h"
#include "msdcnvt_reg.h"
#include "msdcnvt_main.h"
#include "msdcnvt_proc.h"
#include "msdcnvt_int.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int msdcnvt_debug_level = NVT_DBG_WRN;//(NVT_DBG_INFO | NVT_DBG_WARN | NVT_DBG_ERR);
module_param_named(msdcnvt_debug_level, msdcnvt_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(msdcnvt_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id msdcnvt_match_table[] = {
	{   .compatible = "nvt,msdcnvt"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int msdcnvt_open(struct inode *inode, struct file *file);
static int msdcnvt_release(struct inode *inode, struct file *file);
static long msdcnvt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
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

static long msdcnvt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PMSDCNVT_DRV_INFO p_drv;

	inode = file_inode(filp);
	p_drv = filp->private_data;

	return msdcnvt_drv_ioctl(MINOR(inode->i_rdev), &p_drv->module_info, cmd, arg);
}

static int msdcnvt_mmap(struct file *file, struct vm_area_struct *vma)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();

	int er = 0;
	PMSDCNVT_DRV_INFO p_drv;
	unsigned long pfn_start;
	unsigned long virt_start;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long size = vma->vm_end - vma->vm_start;

	p_drv = file->private_data;
	pfn_start = (virt_to_phys(p_ctrl->ipc.p_cfg) >> PAGE_SHIFT) + vma->vm_pgoff;
	virt_start = (unsigned long)p_ctrl->ipc.p_cfg + offset;

	DBG_IND("phy: 0x%lx, offset: 0x%lx, size: 0x%lx, vma_start=%lx\n", pfn_start << PAGE_SHIFT, offset, size, vma->vm_start);

	er = remap_pfn_range(vma, vma->vm_start, pfn_start, size, vma->vm_page_prot);
	if (er)
		DBG_ERR("%s: remap_pfn_range failed at [0x%lx  0x%lx]\n",
				__func__, vma->vm_start, vma->vm_end);
	else
		DBG_IND("%s: map 0x%lx to 0x%lx, size: 0x%lx\n", __func__, virt_start,
				vma->vm_start, size);
	return er;
}

struct file_operations msdcnvt_fops = {
	.owner   = THIS_MODULE,
	.open    = msdcnvt_open,
	.release = msdcnvt_release,
	.mmap    = msdcnvt_mmap,
	.unlocked_ioctl = msdcnvt_ioctl,
	.llseek  = no_llseek,
};

static int msdcnvt_probe(struct platform_device *p_dev)
{
	MSDCNVT_DRV_INFO *p_drv_info;
#if 0
	const struct of_device_id *match;
#endif
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

	ret = msdcnvt_proc_init(p_drv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DEV;
	}

	ret = msdcnvt_drv_init(&p_drv_info->module_info);

	platform_set_drvdata(p_dev, p_drv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;

FAIL_DRV_INIT:
	msdcnvt_proc_remove(p_drv_info);

FAIL_DEV:
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

	msdcnvt_proc_remove(p_drv_info);

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
	DBG_WRN("\n");
	platform_driver_unregister(&msdcnvt_driver);
}

module_init(msdcnvt_module_init);
module_exit(msdcnvt_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("msdcnvt driver");
MODULE_LICENSE("GPL");

EXPORT_SYMBOL(msdcnvt_get_data_addr);
EXPORT_SYMBOL(msdcnvt_get_data_size);
EXPORT_SYMBOL(msdcnvt_add_callback_bi);
EXPORT_SYMBOL(msdcnvt_add_callback_si);
EXPORT_SYMBOL(msdcnvt_get_trans_size);
EXPORT_SYMBOL(msdcnvt_check_params);
