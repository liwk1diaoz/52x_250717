/**
	@brief Header file of IO control definition of dis alg sample.

	@file dis_alg_drv.h

	@ingroup dis_alg

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _DIS_ALG_IOCTL_H_
#define _DIS_ALG_IOCTL_H_

#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <kwrap/dev.h>
#define DEV_UPDATE 1

#if DEV_UPDATE
#define MODULE_NAME "nvt_disflow"
#define MODULE_MINOR_COUNT 1
#endif
/********************************************************************
	TYPE DEFINITION
********************************************************************/
/**
	Net flow device
*/
#if DEV_UPDATE
typedef struct disflow_drv_info {
	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	struct resource* presource[MODULE_MINOR_COUNT];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *p_proc_module_root;
	struct proc_dir_entry *p_proc_help_entry;
	struct proc_dir_entry *p_proc_cmd_entry;


} DISFLOW_DRV_INFO, *PDISFLOW_DRV_INFO;
#endif


#endif  /* _DIS_ALG_IOCTL_H_ */
