#ifndef _SDE_MAIN_H_
#define _SDE_MAIN_H_

#ifdef __KERNEL__
#include <linux/miscdevice.h>
#include <kwrap/dev.h>
#endif

#include "sde_dev_int.h"

#define SDE_NEW_REG_METHOD 1

//=============================================================================
// SDE device name
//=============================================================================
#define SDE_DEV_NAME "nvt_sde"
#define SDE_MODULE_MINOR_COUNT 1

//=============================================================================
// struct & definition
//=============================================================================
#ifdef __KERNEL__
typedef struct _SDE_DRV_INFO {
	SDE_DEV_INFO dev_info;
	#if (!SDE_NEW_REG_METHOD)
	struct miscdevice miscdev;
	#endif

	// proc entries
	struct proc_dir_entry *proc_root;
	struct proc_dir_entry *proc_info;
	struct proc_dir_entry *proc_dump_reg;
	struct proc_dir_entry *proc_dump_cfg;
	struct proc_dir_entry *proc_command;
	struct proc_dir_entry *proc_help;
	struct proc_dir_entry *proc_dbg_mode;
} SDE_DRV_INFO;

#if (SDE_NEW_REG_METHOD)
typedef struct sde_vos_drv {
	struct class *pmodule_class;
	struct device *pdevice[SDE_MODULE_MINOR_COUNT];
	struct cdev cdev;
	dev_t dev_id;
} SDE_VOS_DRV, *PSDE_VOS_DRV;
#endif
#endif

//=============================================================================
// extern functions
//=============================================================================
extern SDE_DEV_INFO *sde_get_dev_info(void);

#endif

