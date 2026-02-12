#ifndef __MSDCNVT_MAIN_H
#define __MSDCNVT_MAIN_H
#include <linux/cdev.h>
#include <linux/types.h>
#include "msdcnvt_drv.h"

#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "msdcnvt_adj"

typedef struct _MSDCNVT_DRV_INFO {
	MSDCNVT_INFO module_info;

	struct class *pmodule_class;
	struct device *p_device[MODULE_MINOR_COUNT];
	struct cdev cdev;
	dev_t dev_id;

} MSDCNVT_DRV_INFO, *PMSDCNVT_DRV_INFO;


#endif
