#ifndef _EIS_MAIN_H_
#define _EIS_MAIN_H_

#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

//=============================================================================
// EIS device name
//=============================================================================
#define MODULE_NAME "nvt_eis"
#define MODULE_MINOR_COUNT 1

//=============================================================================
// struct & definition
//=============================================================================
typedef struct _EIS_DRV_INFO {
	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	struct resource* presource[MODULE_MINOR_COUNT];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *p_proc_module_root;
	struct proc_dir_entry *p_proc_info_entry;
	struct proc_dir_entry *p_proc_cmd_entry;
	struct proc_dir_entry *p_proc_help_entry;
} EIS_DRV_INFO, *PEIS_DRV_INFO;

#endif
