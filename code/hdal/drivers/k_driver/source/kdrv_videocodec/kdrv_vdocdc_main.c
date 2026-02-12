
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/miscdevice.h>

#include "kwrap/type.h"
#include "kwrap/file.h"
#include "kwrap/dev.h"

#include "kdrv_vdocdc_main.h"
#include "kdrv_vdocdc_dbg.h"
#include "kdrv_vdocdc_ioctl.h"
#include "kdrv_vdocdc_proc.h"
#include "kdrv_vdocdc_thread.h"

extern long kdrv_vdocdc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

//=============================================================================
// Global variable
//=============================================================================
struct file_operations kdrv_vdocdc_fops = {
    .owner   = THIS_MODULE,
    .llseek  = no_llseek,
    .unlocked_ioctl = kdrv_vdocdc_ioctl,
};

#if 0
static struct miscdevice kdrv_vdocdc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "kdrv_h26x",
    .fops = &kdrv_vdocdc_fops,
};
#else
static KDRV_VDOCDC_DRV_INFO m_pdrv_info;
#endif

int __init kdrv_vdocdc_module_init(void)
{
	int ret = 0;
	unsigned char ucloop;
	KDRV_VDOCDC_DRV_INFO* pdrv_info = &m_pdrv_info;

	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, MODULE_MINOR_COUNT, MODULE_NAME)) {
		pr_err("Can't get device ID\n");
		return -ENODEV;
	}
	
	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &kdrv_vdocdc_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, MODULE_MINOR_COUNT)) {
		pr_err("Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	pdrv_info->pmodule_class = class_create(THIS_MODULE, MODULE_NAME);
	if(IS_ERR(pdrv_info->pmodule_class)) {
		pr_err("failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT ) ; ucloop++) {
		pdrv_info->pdevice[ucloop] = device_create(pdrv_info->pmodule_class, NULL
			, MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
			, MODULE_NAME);

		if(IS_ERR(pdrv_info->pdevice[ucloop])) {

			pr_err("failed in creating device%d.\n", ucloop);

			if (ucloop == 0) {
				device_unregister(pdrv_info->pdevice[ucloop]);
			}

			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}
	
	kdrv_vdocdc_proc_init();
#ifdef VDOCDC_LL
	kdrv_vdocdc_create_thread();
#endif

	return ret;

FAIL_CLASS:
	class_destroy(pdrv_info->pmodule_class);

FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

	return ret;
}

void __exit kdrv_vdocdc_module_exit(void)
{
	unsigned char ucloop;
	KDRV_VDOCDC_DRV_INFO* pdrv_info = &m_pdrv_info;

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT ) ; ucloop++)
		device_unregister(pdrv_info->pdevice[ucloop]);

	class_destroy(pdrv_info->pmodule_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

#ifdef VDOCDC_LL
	kdrv_vdocdc_remove_thread();
#endif
	kdrv_vdocdc_proc_remove();
}

module_init(kdrv_vdocdc_module_init);
module_exit(kdrv_vdocdc_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("vdocdc driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(H26XE_VER_STR);
