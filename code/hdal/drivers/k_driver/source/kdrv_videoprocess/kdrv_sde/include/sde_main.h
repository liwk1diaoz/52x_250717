#ifndef __SDE_MAIN_H__
#define __SDE_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "sde_drv.h"


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "nvt_sde"

typedef struct sde_drv_info {
	SDE_INFO module_info;

	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	struct resource *presource[SDE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_module_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
	struct proc_dir_entry *pproc_fw_power_saving_entry;
	struct proc_dir_entry *pproc_hw_power_saving_entry;
} SDE_DRV_INFO, *PSDE_DRV_INFO;


#endif
