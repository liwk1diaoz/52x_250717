#ifndef __MODULE_MAIN_H__
#define __MODULE_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>


#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

//=============================================================================
// GYRO device name
//=============================================================================
#define MODULE_NAME "nvt_gyro_comm"
#define MODULE_MINOR_COUNT 1

//=============================================================================
// struct & definition
//=============================================================================
typedef struct _GYRO_COMM_INFO {
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
} GYRO_COMM_INFO, *PGYRO_COMM_INFO;


#endif
