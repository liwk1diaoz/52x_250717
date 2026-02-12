#ifndef __VPE_TASKLET_H__
#define __VPE_TASKLET_H__

#include "kdrv_vpe_int.h"



extern void vpe_tasklet_schedule(VPE_DRV_INT_INFO *p_set);
extern void vpe_tasklet_init(VPE_DRV_INT_INFO *p_set, void (*func) (unsigned long), unsigned long data);
extern void vpe_tasklet_exit(VPE_DRV_INT_INFO *p_set);

#endif

