#include <linux/module.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "vpe_ioctl.h"
#include "vpe_dbg.h"
#include "vpe_main.h"
#include "vpe_proc.h"
#include "vpe_version.h"
#include "vpet_api_int.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
UINT32 vpe_id_list = 0x3; // path 1 + path 2
module_param_named(vpe_id_list, vpe_id_list, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vpe_id_list, "List of vpe id");
#ifdef DEBUG
UINT32 vpe_debug_level = THIS_DBGLVL;
module_param_named(vpe_debug_level, vpe_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vpe_debug_level, "Debug message level");
#endif

//=============================================================================
// global variable
//=============================================================================
static VPE_DRV_INFO *p_vpe_pdrv_info;

#if (VPE_NEW_REG_METHOD)
static VPE_VOS_DRV vpe_vos_drv;
#endif

//=============================================================================
// function declaration
//=============================================================================

//=============================================================================
// extern functions
//=============================================================================
VPE_DEV_INFO *vpe_get_dev_info(void)
{
	if (p_vpe_pdrv_info == NULL) {
		DBG_ERR("vpe device info NULL! \n");
		return NULL;
	}

	return &p_vpe_pdrv_info->dev_info;
}

//=============================================================================
// misc device file operation
//=============================================================================
static INT32 vpe_drv_open(struct inode *inode, struct file *file)
{
	INT32 rt = 0;

	if (p_vpe_pdrv_info == NULL) {
		file->private_data = NULL;
		DBG_ERR("VPE driver is not installed! \n");
		rt = -ENXIO;
	} else {
		file->private_data = p_vpe_pdrv_info;
	}

	return 0;
}

static INT32 vpe_drv_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static INT32 vpe_do_ioc_cmd(VPE_DEV_INFO *pdev_info, UINT32 cmd, UINT32 arg)
{
	INT32 ret = 0;
	UINT32 *buf_addr = pdev_info->ioctl_buf;
	UINT32 size;

	size = vpet_api_get_item_size(_IOC_NR(cmd));

	if ((_IOC_DIR(cmd) == _IOC_READ) || (_IOC_DIR(cmd) == (_IOC_READ|_IOC_WRITE))) {
		ret = copy_from_user(buf_addr, (void __user *)arg, size) ? -EFAULT : 0;
		if (ret < 0) {
			DBG_ERR("copy_from_user ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}

		ret = vpet_api_get_cmd(_IOC_NR(cmd), (UINT32) buf_addr);
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

		ret = vpet_api_set_cmd(_IOC_NR(cmd), (UINT32) buf_addr);
		if (ret < 0) {
			DBG_ERR("set command ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}
	} else {
		DBG_ERR("only support read/write \n");
	}

	return ret;
}

static long vpe_drv_ioctl(struct file *filp, UINT32 cmd, unsigned long arg)
{
	VPE_DRV_INFO *pdrv_info = NULL;
	VPE_DEV_INFO *pdev_info = NULL;
	INT32 ret = 0;

	pdrv_info = (VPE_DRV_INFO *)filp->private_data;
	if (pdrv_info == NULL) {
		return -ENXIO;
	}

	pdev_info = &pdrv_info->dev_info;
	if (pdev_info == NULL) {
		return -ENXIO;
	}

	down(&pdev_info->ioc_mutex);

	ret = vpe_do_ioc_cmd(pdev_info, cmd, arg);

	up(&pdev_info->ioc_mutex);

	return ret;
}

//=============================================================================
// platform driver
//=============================================================================
struct file_operations vpe_drv_fops = {
	.owner   = THIS_MODULE,
	.open    = vpe_drv_open,
	.release = vpe_drv_release,
	.unlocked_ioctl = vpe_drv_ioctl,
	.llseek  = no_llseek,
};

static void vpe_remove_drv(VPE_DRV_INFO *pdrv_info)
{
	#if (VPE_NEW_REG_METHOD)
	unsigned char ucloop;
	VPE_VOS_DRV* pvpe_vos_drv = &vpe_vos_drv;

	for (ucloop = 0 ; ucloop < (VPE_MODULE_MINOR_COUNT ) ; ucloop++)
		device_unregister(pvpe_vos_drv->pdevice[ucloop]);

	class_destroy(pvpe_vos_drv->pmodule_class);
	cdev_del(&pvpe_vos_drv->cdev);
	vos_unregister_chrdev_region(pvpe_vos_drv->dev_id, VPE_MODULE_MINOR_COUNT);
	#endif

	if (pdrv_info) {
		#if (!VPE_NEW_REG_METHOD)
		if (pdrv_info->miscdev.fops)
			misc_deregister(&pdrv_info->miscdev);
		#endif
		vpe_dev_deconstruct(&pdrv_info->dev_info);
		vpe_proc_remove(pdrv_info);
		kfree(pdrv_info);
		pdrv_info = NULL;
		p_vpe_pdrv_info = NULL;
	}
}

static INT32 vpe_remove(struct platform_device *pdev)
{
	VPE_DRV_INFO *pdrv_info = platform_get_drvdata(pdev);

	vpe_remove_drv(pdrv_info);

	return 0;
}

static INT32 vpe_probe(struct platform_device *pdev)
{
	INT32 ret = 0;
	#if (VPE_NEW_REG_METHOD)
	unsigned char ucloop;
	VPE_VOS_DRV* pvpe_vos_drv = &vpe_vos_drv;
	#endif

	p_vpe_pdrv_info = kzalloc(sizeof(VPE_DRV_INFO), GFP_KERNEL);
	if (p_vpe_pdrv_info == NULL) {
		DBG_ERR("failed to alloc vpe_drv_info_t! \n");
		ret = -ENOMEM;
		vpe_remove_drv((VPE_DRV_INFO *)pdev);
		return ret;
	}

	platform_set_drvdata(pdev, p_vpe_pdrv_info);
	ret = vpe_dev_construct(&p_vpe_pdrv_info->dev_info);
	if (ret < 0) {
		DBG_ERR("failed to construct vpe_dev_info_t \n");
		vpe_remove_drv((VPE_DRV_INFO *)pdev);
		return ret;
	}

	ret = vpe_proc_init(p_vpe_pdrv_info);
	if (ret < 0) {
		DBG_ERR("failed to initialize proc! \n");
		vpe_remove_drv((VPE_DRV_INFO *)pdev);
		return ret;
	}

	#if (VPE_NEW_REG_METHOD)
	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pvpe_vos_drv->dev_id, VPE_MODULE_MINOR_COUNT, VPE_DEV_NAME)) {
		pr_err("Can't get device ID\n");
		return -ENODEV;
	}

	/* Register character device for the volume */
	cdev_init(&pvpe_vos_drv->cdev, &vpe_drv_fops);
	pvpe_vos_drv->cdev.owner = THIS_MODULE;

	if (cdev_add(&pvpe_vos_drv->cdev, pvpe_vos_drv->dev_id, VPE_MODULE_MINOR_COUNT)) {
		pr_err("Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	pvpe_vos_drv->pmodule_class = class_create(THIS_MODULE, VPE_DEV_NAME);
	if(IS_ERR(pvpe_vos_drv->pmodule_class)) {
		pr_err("failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (VPE_MODULE_MINOR_COUNT ) ; ucloop++) {
		pvpe_vos_drv->pdevice[ucloop] = device_create(pvpe_vos_drv->pmodule_class, NULL
			, MKDEV(MAJOR(pvpe_vos_drv->dev_id), (ucloop + MINOR(pvpe_vos_drv->dev_id))), NULL
			, VPE_DEV_NAME);

		if(IS_ERR(pvpe_vos_drv->pdevice[ucloop])) {

			pr_err("failed in creating device%d.\n", ucloop);

			if (ucloop == 0) {
				device_unregister(pvpe_vos_drv->pdevice[ucloop]);
			}

			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}
	return ret;

	FAIL_CLASS:
	class_destroy(pvpe_vos_drv->pmodule_class);

	FAIL_CDEV:
	cdev_del(&pvpe_vos_drv->cdev);
	vos_unregister_chrdev_region(pvpe_vos_drv->dev_id, VPE_MODULE_MINOR_COUNT);

	return ret;
	#else
	// register misc device
	p_vpe_pdrv_info->miscdev.minor = MISC_DYNAMIC_MINOR;
	p_vpe_pdrv_info->miscdev.name = VPE_DEV_NAME;
	p_vpe_pdrv_info->miscdev.fops = &vpe_drv_fops;
	ret = misc_register(&p_vpe_pdrv_info->miscdev);
	if (ret < 0) {
		DBG_ERR("failed to register misc device! \n");
		return ret;
	}

	return ret;
	#endif
}

static struct platform_driver vpe_driver = {
	.probe = vpe_probe,
	.remove = vpe_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = VPE_DEV_NAME,
	},
};

//=============================================================================
// platform device
//=============================================================================
static void vpe_release(struct device *dev)
{
}

static struct platform_device vpe_device = {
	.id = 0,
	.num_resources = 0,
	.resource = NULL,
	.name = VPE_DEV_NAME,
	.dev = {
		.release = vpe_release,
	},
};

static INT32 __init vpe_module_init(void)
{
	INT32 ret = 0;

	ret = platform_device_register(&vpe_device);
	if (ret) {
		DBG_ERR("failed to register vpe device \n");
		return ret;
	}

	ret = platform_driver_register(&vpe_driver);
	if (ret) {
		DBG_ERR("failed to register vpe driver \n");
		platform_device_unregister(&vpe_device);
		return ret;
	}

	return ret;
}

static void __exit vpe_module_exit(void)
{
	platform_driver_unregister(&vpe_driver);
	platform_device_unregister(&vpe_device);
}

module_init(vpe_module_init);
module_exit(vpe_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("nvt_vpe driver");
MODULE_LICENSE("GPL");
