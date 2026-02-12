/*
    KDRV SDE configuration file.

    KDRV SDE configuration file. Define semaphore ID, flag ID, etc.

    @file       KDRV_SDE_config.c
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/


#include "sde_platform.h"
#include "kdrv_sde_int.h"
#include "kdrv_videoprocess/kdrv_sde.h"

ID FLG_ID_KDRV_SDE = 0; //for kdrv lock.
SEM_HANDLE SEMID_KDRV_SDE;

void kdrv_sde_install_id(void)
{
	OS_CONFIG_FLAG(FLG_ID_KDRV_SDE);
	SEM_CREATE(SEMID_KDRV_SDE, 1);

}

#ifdef __KERNEL__
EXPORT_SYMBOL(kdrv_sde_install_id);
#endif

void kdrv_sde_uninstall_id(void)
{
	rel_flg(FLG_ID_KDRV_SDE);
	SEM_DESTROY(SEMID_KDRV_SDE);
}

void kdrv_sde_flow_init(void)
{
	set_flg(FLG_ID_KDRV_SDE, KDRV_SDE_INIT_BITS);
}

