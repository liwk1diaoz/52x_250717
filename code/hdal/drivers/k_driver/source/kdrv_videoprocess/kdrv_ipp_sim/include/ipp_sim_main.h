#ifndef __IPP_SIM_MAIN_H__
#define __IPP_SIM_MAIN_H__

#include <linux/cdev.h>
#include <linux/types.h>
#include "ipp_sim_drv.h"


#define IPP_SIM_MINOR_ID      0
#define IPP_SIM_MINOR_COUNT   1
#define IPP_SIM_NAME          "kdrv_ipp_sim"

typedef struct ipp_sim_drv_info {
	IPP_SIM_INFO ipp_sim_info;

	struct class *pipp_sim_class;
	struct device *pdevice[IPP_SIM_MINOR_COUNT];
	struct resource *presource[IPP_SIM_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_ipp_sim_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
} IPP_SIM_DRV_INFO, *PIPP_SIM_DRV_INFO;


#endif
