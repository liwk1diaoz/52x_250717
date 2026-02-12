#include <linux/module.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "sde_api.h"
#include "sde_ioctl.h"
#include "sde_dbg.h"
#include "sde_main.h"
#include "sde_proc.h"
#include "sde_version.h"
#include "sdet_api_int.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
UINT32 sde_id_list = 0x3; // path 1 + path 2
module_param_named(sde_id_list, sde_id_list, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sde_id_list, "List of sde id");
#ifdef DEBUG
UINT32 sde_debug_level = THIS_DBGLVL;
module_param_named(sde_debug_level, sde_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sde_debug_level, "Debug message level");
#endif

//=============================================================================
// global variable
//=============================================================================
static SDE_DRV_INFO *p_sde_pdrv_info;

#if (SDE_NEW_REG_METHOD)
static SDE_VOS_DRV sde_vos_drv;
#endif

//=============================================================================
// function declaration
//=============================================================================

//=============================================================================
// extern functions
//=============================================================================
SDE_DEV_INFO *sde_get_dev_info(void)
{
	if (p_sde_pdrv_info == NULL) {
		DBG_ERR("sde device info NULL! \n");
		return NULL;
	}

	return &p_sde_pdrv_info->dev_info;
}

//=============================================================================
// misc device file operation
//=============================================================================
static INT32 sde_drv_open(struct inode *inode, struct file *file)
{
	INT32 rt = 0;

	if (p_sde_pdrv_info == NULL) {
		file->private_data = NULL;
		DBG_ERR("SDE driver is not installed! \n");
		rt = -ENXIO;
	} else {
		file->private_data = p_sde_pdrv_info;
	}

	return 0;
}

static INT32 sde_drv_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static INT32 sde_do_ioc_cmd(SDE_DEV_INFO *pdev_info, UINT32 cmd, UINT32 arg)
{
	INT32 ret = 0;
	UINT32 *buf_addr = pdev_info->ioctl_buf;
	UINT32 size;

	size = sdet_api_get_item_size(_IOC_NR(cmd));

	if ((_IOC_DIR(cmd) == _IOC_READ) || (_IOC_DIR(cmd) == (_IOC_READ|_IOC_WRITE))) {
		ret = copy_from_user(buf_addr, (void __user *)arg, size) ? -EFAULT : 0;
		if (ret < 0) {
			DBG_ERR("copy_from_user ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}

		ret = sdet_api_get_cmd(_IOC_NR(cmd), (UINT32) buf_addr);
		if (ret < 0) {
			DBG_ERR("get command ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}

		ret = copy_to_user((void __user *)arg, buf_addr, size) ? -EFAULT : 0;
		if (ret < 0) {
			DBG_ERR("copy_to_user ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}
	} else if (_IOC_DIR(cmd) == _IOC_WRITE) {
		ret = copy_from_user(buf_addr, (void __user *)arg, size) ? -EFAULT : 0;
		if (ret < 0) {
			DBG_ERR("copy_from_user ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}

		ret = sdet_api_set_cmd(_IOC_NR(cmd), (UINT32) buf_addr);
		if (ret < 0) {
			DBG_ERR("set command ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}
	} else {
		DBG_ERR("only support read/write \n");
	}

	return ret;
}

static long sde_drv_ioctl(struct file *filp, UINT32 cmd, unsigned long arg)
{
	SDE_DRV_INFO *pdrv_info = NULL;
	SDE_DEV_INFO *pdev_info = NULL;
	INT32 ret = 0;

	pdrv_info = (SDE_DRV_INFO *)filp->private_data;
	if (pdrv_info == NULL) {
		return -ENXIO;
	}

	pdev_info = &pdrv_info->dev_info;
	if (pdev_info == NULL) {
		return -ENXIO;
	}

	down(&pdev_info->ioc_mutex);

	ret = sde_do_ioc_cmd(pdev_info, cmd, arg);

	up(&pdev_info->ioc_mutex);

	return ret;
}

//=============================================================================
// platform driver
//=============================================================================
struct file_operations sde_drv_fops = {
	.owner   = THIS_MODULE,
	.open    = sde_drv_open,
	.release = sde_drv_release,
	.unlocked_ioctl = sde_drv_ioctl,
	.llseek  = no_llseek,
};

static void sde_remove_drv(SDE_DRV_INFO *pdrv_info)
{
	#if (SDE_NEW_REG_METHOD)
	unsigned char ucloop;
	SDE_VOS_DRV* psde_vos_drv = &sde_vos_drv;

	for (ucloop = 0 ; ucloop < (SDE_MODULE_MINOR_COUNT ) ; ucloop++)
		device_unregister(psde_vos_drv->pdevice[ucloop]);

	class_destroy(psde_vos_drv->pmodule_class);
	cdev_del(&psde_vos_drv->cdev);
	vos_unregister_chrdev_region(psde_vos_drv->dev_id, SDE_MODULE_MINOR_COUNT);
	#endif

	if (pdrv_info) {
		#if (!SDE_NEW_REG_METHOD)
		if (pdrv_info->miscdev.fops)
			misc_deregister(&pdrv_info->miscdev);
		#endif
		sde_dev_deconstruct(&pdrv_info->dev_info);
		sde_proc_remove(pdrv_info);
		kfree(pdrv_info);
		pdrv_info = NULL;
		p_sde_pdrv_info = NULL;
	}
}

static INT32 sde_remove(struct platform_device *pdev)
{
	SDE_DRV_INFO *pdrv_info = platform_get_drvdata(pdev);

	sde_remove_drv(pdrv_info);

	return 0;
}

static INT32 sde_probe(struct platform_device *pdev)
{
	INT32 ret = 0;
	#if (SDE_NEW_REG_METHOD)
	unsigned char ucloop;
	SDE_VOS_DRV* psde_vos_drv = &sde_vos_drv;
	#endif

	p_sde_pdrv_info = kzalloc(sizeof(SDE_DRV_INFO), GFP_KERNEL);
	if (p_sde_pdrv_info == NULL) {
		DBG_ERR("failed to alloc sde_drv_info_t! \n");
		ret = -ENOMEM;
		sde_remove_drv((SDE_DRV_INFO *)pdev);
		return ret;
	}

	platform_set_drvdata(pdev, p_sde_pdrv_info);
	ret = sde_dev_construct(&p_sde_pdrv_info->dev_info);
	if (ret < 0) {
		DBG_ERR("failed to construct sde_dev_info_t \n");
		sde_remove_drv((SDE_DRV_INFO *)pdev);
		return ret;
	}

	ret = sde_proc_init(p_sde_pdrv_info);
	if (ret < 0) {
		DBG_ERR("failed to initialize proc! \n");
		sde_remove_drv((SDE_DRV_INFO *)pdev);
		return ret;
	}

	#if (SDE_NEW_REG_METHOD)
	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&psde_vos_drv->dev_id, SDE_MODULE_MINOR_COUNT, SDE_DEV_NAME)) {
		pr_err("Can't get device ID\n");
		return -ENODEV;
	}

	/* Register character device for the volume */
	cdev_init(&psde_vos_drv->cdev, &sde_drv_fops);
	psde_vos_drv->cdev.owner = THIS_MODULE;

	if (cdev_add(&psde_vos_drv->cdev, psde_vos_drv->dev_id, SDE_MODULE_MINOR_COUNT)) {
		pr_err("Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	psde_vos_drv->pmodule_class = class_create(THIS_MODULE, SDE_DEV_NAME);
	if(IS_ERR(psde_vos_drv->pmodule_class)) {
		pr_err("failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (SDE_MODULE_MINOR_COUNT ) ; ucloop++) {
		psde_vos_drv->pdevice[ucloop] = device_create(psde_vos_drv->pmodule_class, NULL
			, MKDEV(MAJOR(psde_vos_drv->dev_id), (ucloop + MINOR(psde_vos_drv->dev_id))), NULL
			, SDE_DEV_NAME);

		if(IS_ERR(psde_vos_drv->pdevice[ucloop])) {

			pr_err("failed in creating device%d.\n", ucloop);

			if (ucloop == 0) {
				device_unregister(psde_vos_drv->pdevice[ucloop]);
			}

			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}
	return ret;

	FAIL_CLASS:
	class_destroy(psde_vos_drv->pmodule_class);

	FAIL_CDEV:
	cdev_del(&psde_vos_drv->cdev);
	vos_unregister_chrdev_region(psde_vos_drv->dev_id, SDE_MODULE_MINOR_COUNT);

	return ret;
	#else
	// register misc device
	p_sde_pdrv_info->miscdev.minor = MISC_DYNAMIC_MINOR;
	p_sde_pdrv_info->miscdev.name = SDE_DEV_NAME;
	p_sde_pdrv_info->miscdev.fops = &sde_drv_fops;
	ret = misc_register(&p_sde_pdrv_info->miscdev);
	if (ret < 0) {
		DBG_ERR("failed to register misc device! \n");
		return ret;
	}

	return ret;
	#endif
}

static struct platform_driver sde_driver = {
	.probe = sde_probe,
	.remove = sde_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = SDE_DEV_NAME,
	},
};

//=============================================================================
// platform device
//=============================================================================
static void sde_release(struct device *dev)
{
}

static struct platform_device sde_device = {
	.id = 0,
	.num_resources = 0,
	.resource = NULL,
	.name = SDE_DEV_NAME,
	.dev = {
		.release = sde_release,
	},
};

static INT32 __init sde_module_init(void)
{
	INT32 ret = 0;
	UINT32 i;

	ret = platform_device_register(&sde_device);
	if (ret) {
		DBG_ERR("failed to register sde device \n");
		return ret;
	}

	ret = platform_driver_register(&sde_driver);
	if (ret) {
		DBG_ERR("failed to register sde driver \n");
		platform_device_unregister(&sde_device);
		return ret;
	}

	for (i = 0; i < SDE_ID_MAX_NUM; i++) {
		sde_flow_init(i);
	}
	return ret;
}

static void __exit sde_module_exit(void)
{
	platform_driver_unregister(&sde_driver);
	platform_device_unregister(&sde_device);
}

module_init(sde_module_init);
module_exit(sde_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("nvt_sde driver");
MODULE_LICENSE("GPL");
