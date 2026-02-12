/*
    SIE KDRV configuration file.

    SIE KDRV configuration file. Define semaphore ID, flag ID, etc.

    @file       kdrv_sie_config.c
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "kdrv_sie_config.h"

#define SIE_SEM_MAX_CNT	1
KDRV_SIE_SEM_TABLE kdrv_sie_semtbl[KDRV_SIE_MAX_ENG];
/*
    Install sie kdrv data

    Install sie kdrv data. This API will be called when system start.

    @return void
*/

void kdrv_sie_install_id(void)
{
	UINT32  i;
	KDRV_SIE_LIMIT sie_limit;

	kdrv_sie_get_sie_limit(KDRV_SIE_ID_1, (void *)&sie_limit);
	for (i = 0; i <= sie_limit.max_spt_id; i++) {
		vos_sem_create(&kdrv_sie_semtbl[i].semphore_id, SIE_SEM_MAX_CNT, "SEM_KDRV_SIE");
	}
}

void kdrv_sie_uninstall_id(void)
{
	UINT32  i;
	KDRV_SIE_LIMIT sie_limit;

	kdrv_sie_get_sie_limit(KDRV_SIE_ID_1, (void *)&sie_limit);
	for (i = 0; i <= sie_limit.max_spt_id; i++) {
		vos_sem_destroy(kdrv_sie_semtbl[i].semphore_id);
	}
}

ID *kdrv_sie_get_sem_id(KDRV_SIE_PROC_ID id)
{
	KDRV_SIE_LIMIT sie_limit;

	kdrv_sie_get_sie_limit(id, (void *)&sie_limit);
	if (id <= sie_limit.max_spt_id) {
		return (&kdrv_sie_semtbl[id].semphore_id);
	} else {
		kdrv_sie_dbg_err("id = %d overflow\r\n", (int)id);
		return 0;
	}
}

