/*
    IPL KDRV configuration file.

    IPL KDRV configuration file. Define semaphore ID, flag ID, etc.

    @file       KDRV_IVE_config.c
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "ive_platform.h"
#include "ive_dbg.h"

/*
    Install ipl kdrv data

    Install ipl kdrv data. This API will be called when system start.

    @return void
*/

void kdrv_ive_install_id(void)
{
	UINT32  i;

	// Flag
	for (i = 0; i < KDRV_IVE_FLAG_COUNT; i++) {
		OS_CONFIG_FLAG(kdrv_ive_flag_id[i]);
	}

	// Semaphore
	for (i = 0; i < KDRV_IVE_SEMAPHORE_COUNT; i++) {
		ive_platform_create_resource_config(i);
	}
}

void kdrv_ive_uninstall_id(void)
{
	UINT32  i;

	// Flag
	for (i = 0; i < KDRV_IVE_FLAG_COUNT; i++) {
		ive_platform_rel_id_config(i);
	}
	// Semaphore
	for (i = 0; i < KDRV_IVE_SEMAPHORE_COUNT; i++) {
		ive_platform_rel_sem_config(i);
	}
}



SEM_HANDLE* kdrv_ive_get_sem_id(KDRV_IVE_SEM idx)
{
	if (idx >= KDRV_IVE_SEMAPHORE_COUNT)
	{
		return 0;
	}
	return &kdrv_ive_semtbl[idx].semphore_id;
}

ID kdrv_ive_get_flag_id(KDRV_IVE_FLAG idx)
{
	if (idx >= KDRV_IVE_FLAG_COUNT)
	{
		return 0;
	}
	return kdrv_ive_flag_id[idx];
}


void kdrv_ive_init(void)
{
	set_flg(kdrv_ive_get_flag_id(FLG_ID_KDRV_IVE), KDRV_IVE_INIT_BITS);
}

