#ifndef __SDE_PROC_H_
#define __SDE_PROC_H__
#include "sde_main.h"

int nvt_sde_proc_init(PSDE_DRV_INFO pdrv_info);
int nvt_sde_proc_remove(PSDE_DRV_INFO pdrv_info);

extern PSDE_DRV_INFO pdrv_info_data;

#endif