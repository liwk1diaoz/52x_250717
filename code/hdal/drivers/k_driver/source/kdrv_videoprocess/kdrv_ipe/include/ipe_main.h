#ifndef __IPE_MAIN_H__
#define __IPE_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "ipe_drv.h"


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "nvt_ipe"

typedef struct ipe_drv_info {
	IPE_INFO module_info;

	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	struct resource *presource[MODULE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_module_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
} IPE_DRV_INFO, *PIPE_DRV_INFO;


#endif
