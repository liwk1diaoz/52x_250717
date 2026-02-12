#ifndef __IME_MAIN_H__
#define __IME_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "ime_drv.h"


#define IME_MINOR_ID      0
#define IME_MINOR_COUNT   1
#define IME_NAME          "kdrv_ime"

typedef struct ime_drv_info {
	IME_INFO ime_info;

	struct class *pime_class;
	struct device *pdevice[IME_MINOR_COUNT];
	struct resource *presource[IME_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_ime_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
} IME_DRV_INFO, *PIME_DRV_INFO;


#endif
