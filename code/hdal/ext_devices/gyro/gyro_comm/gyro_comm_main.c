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

#include "gyro_comm_platform.h"
#include "gyro_comm_api.h"
#include "gyro_comm_main.h"
#include "gyro_comm_proc.h"
#include "kwrap/debug.h"
//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int _gyro_debug_level = NVT_DBG_MSG;
module_param_named(_gyro_debug_level, _gyro_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(_gyro_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id gyro_match_table[] = {
	{	.compatible = "nvt,nvt_gyro_comm"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int nvt_gyro_comm_open(struct inode *inode, struct file *file);
static int nvt_gyro_comm_release(struct inode *inode, struct file *file);
//static long nvt_gyro_comm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int nvt_gyro_comm_probe(struct platform_device *pdev);
static int nvt_gyro_comm_remove(struct platform_device *pdev);
int __init nvt_gyro_comm_module_init(void);
void __exit nvt_gyro_comm_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int nvt_gyro_comm_open(struct inode *inode, struct file *file)
{
    nvt_dbg(IND, "\n");
	return 0;
}

static int nvt_gyro_comm_release(struct inode *inode, struct file *file)
{
    nvt_dbg(IND, "\n");
	return 0;
}
#if 0
static long nvt_gyro_comm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    nvt_dbg(IND, "\n");
	return nvt_gyro_comm_ioctl(filp, cmd, arg);
}
#endif
struct file_operations nvt_gyro_comm_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_gyro_comm_open,
	.release = nvt_gyro_comm_release,
	.unlocked_ioctl = nvt_gyro_comm_ioctl,
	.llseek  = no_llseek,
};

static int nvt_gyro_comm_probe(struct platform_device *pdev)
{
	int ret = 0;
    unsigned char ucloop;
	GYRO_COMM_INFO* pdrv_info;
    const struct of_device_id *match;

	DBG_IND("%s\n", pdev->name);

	match = of_match_device(gyro_match_table, &pdev->dev);
	if (!match) {
		DBG_ERR("[%s] OF not found \r\n", MODULE_NAME);
		return -EINVAL;
	}
	pdrv_info = kzalloc(sizeof(GYRO_COMM_INFO), GFP_KERNEL);
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
	cdev_init(&pdrv_info->cdev, &nvt_gyro_comm_fops);
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

	//init module
    nvt_gyro_comm_init();

    ret = nvt_gyro_comm_proc_init(pdrv_info);
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

static int nvt_gyro_comm_remove(struct platform_device *pdev)
{
	unsigned char ucloop;
	GYRO_COMM_INFO* pdrv_info;

    DBG_IND("%s:%s\r\n", __func__, pdev->name);

    pdrv_info = platform_get_drvdata(pdev);

    //uninit module
    nvt_gyro_comm_uninit();

	nvt_gyro_comm_proc_remove(pdrv_info);

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

static struct platform_driver nvt_gyro_comm_driver = {
	.driver = {
				.name   = MODULE_NAME,
				.owner = THIS_MODULE,
				.of_match_table = gyro_match_table,
				},
	.probe      = nvt_gyro_comm_probe,
	.remove     = nvt_gyro_comm_remove,
};

int __init nvt_gyro_comm_module_init(void)
{
	int ret;

	nvt_dbg(WRN, "\n");

	ret = platform_driver_register(&nvt_gyro_comm_driver);

	return 0;
}

void __exit nvt_gyro_comm_module_exit(void)
{
	nvt_dbg(WRN, "\n");
	platform_driver_unregister(&nvt_gyro_comm_driver);
}

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

module_init(nvt_gyro_comm_module_init);
module_exit(nvt_gyro_comm_module_exit);


MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("nvt ddr arbiter driver");
MODULE_LICENSE("GPL");
