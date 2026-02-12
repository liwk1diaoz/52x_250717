#ifndef __IFE2_MAIN_H__
#define __IFE2_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "ife2_drv.h"


#define IFE2_MINOR_ID      0
#define IFE2_MINOR_COUNT   1
#define IFE2_NAME          "kdrv_ife2"

typedef struct ife2_drv_info {
	IFE2_INFO ife2_info;

	struct class *pife2_class;
	struct device *pdevice[IFE2_MINOR_COUNT];
	struct resource *presource[IFE2_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_ife2_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
} IFE2_DRV_INFO, *PIFE2_DRV_INFO;


#endif
