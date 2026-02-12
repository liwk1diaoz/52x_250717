
#include "vpe_platform.h"
#include "vpe_tasklet.h"



void vpe_tasklet_schedule(VPE_DRV_INT_INFO *p_set)
{
    tasklet_schedule(&(p_set->job_tasklet));
}

void vpe_tasklet_init(VPE_DRV_INT_INFO *p_set, void (*func) (unsigned long), unsigned long data)
{
    tasklet_init(&(p_set->job_tasklet), func, data);
}


void vpe_tasklet_exit(VPE_DRV_INT_INFO *p_set)
{
    tasklet_kill(&(p_set->job_tasklet));
}


#if defined (__KERNEL__)

EXPORT_SYMBOL(vpe_tasklet_schedule);
EXPORT_SYMBOL(vpe_tasklet_init);
EXPORT_SYMBOL(vpe_tasklet_exit);

#endif




