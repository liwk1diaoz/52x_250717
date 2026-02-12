#ifndef __DCE_MAIN_H__
#define __DCE_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "dce_drv.h"


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "nvt_dce_module"

typedef struct dce_drv_info {
	DCE_INFO module_info;

	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	struct resource *presource[MODULE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_module_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
} DCE_DRV_INFO, *PDCE_DRV_INFO;

//extern struct clk *p_dce_clk[MODULE_CLK_NUM];

#endif
