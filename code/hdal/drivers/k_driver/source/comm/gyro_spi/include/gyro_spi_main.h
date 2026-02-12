#ifndef _GYRO_MAIN_H_
#define _GYRO_MAIN_H_
#include <linux/cdev.h>
#include <linux/types.h>

#include "gyro_spi_drv.h"


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "nvt_gyro_spi"

typedef struct gyro_drv_info {
	GYRO_MODULE_INFO module_info;

	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	struct resource *presource[MODULE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

    // proc entries
    struct proc_dir_entry *pproc_module_root;
    struct proc_dir_entry *pproc_help_entry;
    struct proc_dir_entry *pproc_cmd_entry;
} GYRO_DRV_INFO, *PGYRO_DRV_INFO;


#endif