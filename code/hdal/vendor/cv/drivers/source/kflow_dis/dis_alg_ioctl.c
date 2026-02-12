/**
	@brief Source file of IO control of vendor net flow sample.

	@file net_flow_sample_ioctl.c

	@ingroup net_flow_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/of_device.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "kwrap/type.h"
//#include "frammap/frammap_if.h"
#include "mach/fmem.h"
#include "dis_alg_ioctl.h"
#include "dis_alg_drv.h"
#include "dis_alg_proc.h"
#include "nvt_dis.h"
#include "dis_alg_lib.h"
#include "kflow_dis_version.h"


//#define  DEBUG_TASK

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static struct of_device_id disflow_match_table[] = {
	{	.compatible = "nvt,nvt_disflow"},
	{}
};

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/
//=============================================================================
// function define
//=============================================================================
static int dis_flow_open(struct inode *inode, struct file *file)
{
	return 0;
}
static long dis_flow_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
    CHAR                  version_info[32] = KFLOW_DIS_VERSION;

	UINT32                dis_only_proc_mv    = 0;
	DIS_ALG_PARAM         dis_alg_info        = {0};
	DIS_IPC_INIT          mem                 = {0};
	DIS_BLKSZ             dis_alg_blksz       = {0};
    DIS_MDS_DIM           dis_alg_mds_dim     = {0};
    DIS_MV_INFO_SIZE      dis_alg_mv_info     = {0};



	if (_IOC_TYPE(cmd) != DIS_FLOW_IOC_COMMON_TYPE) {
		ret = -ENOIOCTLCMD;
		goto exit;
	}

	switch (cmd) {
	case DIS_FLOW_IOC_INIT:
		 nvt_dis_init( );
		 break;

	case DIS_FLOW_IOC_UNINIT:
		 nvt_dis_uninit( );
		 break;

	case DIS_FLOW_IOC_SET_PARAM_IN:
		if (copy_from_user(&dis_alg_info, (void __user *)arg, sizeof(DIS_ALG_PARAM))) {
			ret = -EFAULT;
			goto exit;
		}
		nvt_dis_set((void *)&dis_alg_info);
		break;

	case DIS_FLOW_IOC_GET_PARAM_IN:
		nvt_dis_get((void *)&dis_alg_info);
		ret = (copy_to_user((void __user *)arg, &dis_alg_info, sizeof(DIS_ALG_PARAM))) ? (-EFAULT) : 0;
		break;

	case DIS_FLOW_IOC_SET_IMG_DMA_IN:
		if (copy_from_user(&mem, (void __user *)arg, sizeof(DIS_IPC_INIT))) {
			ret = -EFAULT;
			goto exit;
		}
		nvt_dis_buffer_set((void*)&mem);
		break;

	case DIS_FLOW_IOC_SET_ONLY_MV_EN:
		if (copy_from_user(&dis_only_proc_mv, (void __user *)arg, sizeof(UINT32))) {
			ret = -EFAULT;
			goto exit;
		}
		dis_set_only_mv(dis_only_proc_mv);
		break;

	case DIS_FLOW_IOC_GET_ONLY_MV_EN:
		dis_only_proc_mv = dis_get_only_mv();
		ret = (copy_to_user((void __user *)arg, &dis_only_proc_mv, sizeof(UINT32))) ? (-EFAULT) : 0;
		break;

	case DIS_FLOW_IOC_GET_MV_MAP_OUT:
		if (copy_from_user(&dis_alg_mv_info, (void __user *)arg, sizeof(DIS_MV_INFO_SIZE))) {
			ret = -EFAULT;
			goto exit;
		}
        dis_get_ready_motionvec(&dis_alg_mv_info);
		break;

	case DIS_FLOW_IOC_GET_MV_MDS_DIM:
		dis_alg_mds_dim = dis_get_mds_dim( );
		ret = (copy_to_user((void __user *)arg, &dis_alg_mds_dim, sizeof(DIS_MDS_DIM))) ? (-EFAULT) : 0;
		break;


	case DIS_FLOW_IOC_SET_MV_BLOCKS_DIM:
		if (copy_from_user(&dis_alg_blksz, (void __user *)arg, sizeof(DIS_BLKSZ))) {
			ret = -EFAULT;
			goto exit;
		}
        dis_set_blksize(dis_alg_blksz);
		break;
	case DIS_FLOW_IOC_GET_MV_BLOCKS_DIM:
		dis_alg_blksz = dis_get_blksize_info();
		ret = (copy_to_user((void __user *)arg, &dis_alg_blksz, sizeof(DIS_BLKSZ))) ? (-EFAULT) : 0;
		break;
	case DIS_FLOW_IOC_TRIGGER:
		nvt_dis_process();
		break;

    //GET KLOW VERSION
    case DIS_FLOW_IOC_GET_VER:
        ret = (copy_to_user((void __user *)arg, version_info, sizeof(KFLOW_DIS_VERSION))) ? (-EFAULT) : 0;
        break;
	default :
		break;
	}

exit:
	return ret;
}

static int dis_flow_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations dis_flow_fops = {
	.owner          = THIS_MODULE,
	.open           = dis_flow_open,
	.release        = dis_flow_release,
	.unlocked_ioctl = dis_flow_ioctl,
	.llseek         = no_llseek,
};

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
static int dis_flow_probe(struct platform_device *pdev)
{
	int ret = 0;
    unsigned char ucloop;
#if DEV_UPDATE
	DISFLOW_DRV_INFO* pdrv_info;
    const struct of_device_id *match;
#endif

	match = of_match_device(disflow_match_table, &pdev->dev);
	if (!match) {
		DBG_ERR("[%s] OF not found \r\n", MODULE_NAME);
		return -EINVAL;
	}
	pdrv_info = kzalloc(sizeof(DISFLOW_DRV_INFO), GFP_KERNEL);
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
	cdev_init(&pdrv_info->cdev, &dis_flow_fops);
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
    #ifdef DEBUG_TASK
    dis_alg_task_open();
    #else
    dis_init_module(1);
    #endif
    ret = nvt_dis_alg_proc_init(pdrv_info);
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

static int  dis_flow_remove(struct platform_device *pdev)
{
	unsigned char ucloop;
	DISFLOW_DRV_INFO* pdrv_info;

    DBG_IND("%s:%s\r\n", __func__, pdev->name);

    pdrv_info = platform_get_drvdata(pdev);
    #ifdef DEBUG_TASK
    dis_alg_task_close();
    #else
    dis_uninit_module();
    #endif

	nvt_dis_alg_proc_remove(pdrv_info);

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
// platform device
//=============================================================================

static struct platform_driver dis_flow_driver = {
	.driver = {
				.name   = MODULE_NAME,
				.owner = THIS_MODULE,
				.of_match_table = disflow_match_table,
		      },
	.probe      = dis_flow_probe,
	.remove     = dis_flow_remove,
};

int __init dis_flow_module_init(void)
{
	int ret;

	ret = platform_driver_register(&dis_flow_driver);
	if (ret) {
		DBG_ERR(" platform_driver_register failed!\n");
		return -1;
	}
	return 0;
}

void __exit dis_flow_module_exit(void)
{
	platform_driver_unregister(&dis_flow_driver);
	printk(KERN_ERR "exit:\n");
}

module_init(dis_flow_module_init);
module_exit(dis_flow_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("DIS FLOW");
MODULE_LICENSE("GPL");
VOS_MODULE_VERSION(kflow_dis, major, minor, bugfix, ext);
