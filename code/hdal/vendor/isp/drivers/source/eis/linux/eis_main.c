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



#include "eis_main.h"
#include "eis_proc.h"
#include "../eis_int.h"



//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================


//=============================================================================
// global variable
//=============================================================================
/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static struct of_device_id eis_match_table[] = {
	{	.compatible = "nvt,nvt_eis"},
	{}
};
extern long eis_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
//=============================================================================
// function declaration
//=============================================================================
INT32 __init nvt_eis_module_init(void);
void __exit nvt_eis_module_exit(void);


//=============================================================================
// misc device file operation
//=============================================================================
static INT32 eis_drv_open(struct inode *inode, struct file *file)
{
	return 0;
}

static INT32 eis_drv_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations eis_fops = {
	.owner          = THIS_MODULE,
	.open           = eis_drv_open,
	.release        = eis_drv_release,
	.unlocked_ioctl = eis_ioctl,
	.llseek         = no_llseek,
};
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
static int eis_probe(struct platform_device *pdev)
{
	int ret = 0;
    unsigned char ucloop;
#if 1//DEV_UPDATE
	EIS_DRV_INFO* pdrv_info;
    const struct of_device_id *match;
#endif

	DBG_IND("%s\n", pdev->name);

	match = of_match_device(eis_match_table, &pdev->dev);
	if (!match) {
		DBG_ERR("[%s] OF not found \r\n", MODULE_NAME);
		return -EINVAL;
	}
	pdrv_info = kzalloc(sizeof(EIS_DRV_INFO), GFP_KERNEL);
	if (!pdrv_info) {
		DBG_ERR("[%s]failed to allocate memory\r\n", MODULE_NAME);
		return -ENOMEM;
	}

	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, MODULE_MINOR_COUNT, MODULE_NAME)) {
		pr_err("Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_REMAP;
	}

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &eis_fops);
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

    platform_set_drvdata(pdev, pdrv_info);

	//init module to isf_vdoprc
    eis_init_module();

    ret = nvt_eis_proc_init(pdrv_info);
	if (ret) {
    	nvt_dbg(ERR, "failed in creating proc.\n");
	}
	return ret;

FAIL_CLASS:
	class_destroy(pdrv_info->pmodule_class);
FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);
FAIL_FREE_REMAP:
    kfree(pdrv_info);
    pdrv_info = NULL;
    platform_set_drvdata(pdev, pdrv_info);
    DBG_ERR("probe fail\r\n");
    return ret;
}

static int eis_remove(struct platform_device *pdev)
{
	unsigned char ucloop;
	EIS_DRV_INFO* pdrv_info;

    DBG_IND("%s:%s\r\n", __func__, pdev->name);

    pdrv_info = platform_get_drvdata(pdev);

    //uninit module to isf_vdoprc
    eis_uninit_module();


	nvt_eis_proc_remove(pdrv_info);

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT ) ; ucloop++)
		device_unregister(pdrv_info->pdevice[ucloop]);

	class_destroy(pdrv_info->pmodule_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

    kfree(pdrv_info);
	pdrv_info = NULL;
	platform_set_drvdata(pdev, pdrv_info);
	return 0;
}

//=============================================================================
// platform driver
//=============================================================================
static struct platform_driver eis_driver = {
	.driver = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = eis_match_table,
	},
	.probe = eis_probe,
	.remove = eis_remove,
};

static INT32 __init eis_module_init(void)
{
	INT32 ret = 0;

	ret = platform_driver_register(&eis_driver);
	if (ret) {
		DBG_ERR("failed to register eis device \n");
		return ret;
	}
	return ret;
}

static void __exit eis_module_exit(void)
{
	platform_driver_unregister(&eis_driver);
}

module_init(eis_module_init);
module_exit(eis_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("nvt_eis driver");
MODULE_LICENSE("GPL");
