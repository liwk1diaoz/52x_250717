#ifndef _VPE_MAIN_H_
#define _VPE_MAIN_H_

#ifdef __KERNEL__
#include <linux/miscdevice.h>
#include <kwrap/dev.h>
#endif

#include "vpe_dev_int.h"

#define VPE_NEW_REG_METHOD 1

//=============================================================================
// VPE device name
//=============================================================================
#define VPE_DEV_NAME "nvt_vpe"
#define VPE_MODULE_MINOR_COUNT 1

//=============================================================================
// struct & definition
//=============================================================================
#ifdef __KERNEL__


typedef struct _VPE_DRV_INFO {
	VPE_DEV_INFO dev_info;
	#if (!VPE_NEW_REG_METHOD)
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
} VPE_DRV_INFO;

#if (VPE_NEW_REG_METHOD)
typedef struct vpe_vos_drv {
	struct class *pmodule_class;
	struct device *pdevice[VPE_MODULE_MINOR_COUNT];
	struct cdev cdev;
	dev_t dev_id;
} VPE_VOS_DRV, *PVPE_VOS_DRV;
#endif
#endif

//=============================================================================
// extern functions
//=============================================================================
extern VPE_DEV_INFO *vpe_get_dev_info(void);

#endif

