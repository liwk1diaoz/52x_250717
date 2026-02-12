#ifndef __VPE_MAIN_H__
#define __VPE_MAIN_H__

#include <linux/cdev.h>
#include <linux/types.h>
#include "vpe_drv.h"


#define VPE_MINOR_ID      0
#define VPE_MINOR_COUNT   1
#define VPE_NAME          "kdrv_vpe"

typedef struct vpe_drv_info {
	VPE_INFO vpe_info;

	struct class *pvpe_class;
	struct device *pdevice[VPE_MINOR_COUNT];
	struct resource *presource[VPE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_vpe_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
} VPE_DRV_INFO, *PVPE_DRV_INFO;


#endif
