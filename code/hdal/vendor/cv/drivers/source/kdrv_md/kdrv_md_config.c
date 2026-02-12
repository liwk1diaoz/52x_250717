/*
    IPL KDRV configuration file.

    IPL KDRV configuration file. Define semaphore ID, flag ID, etc.

    @file       KDRV_AI_config.c
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#include "kdrv_md_int.h"

// ==================================================================
// Flag
// ==================================================================
#if defined(__FREERTOS)
ID 	   kdrv_md_flag_id[KDRV_MD_FLAG_COUNT] = {0};
#else
UINT32 kdrv_md_flag_id[KDRV_MD_FLAG_COUNT] = {0};
#endif

// ==================================================================
// Semaphore
// ==================================================================
#define SEM_MAX_CNT	1
#if defined __UITRON || defined __ECOS
KDRV_MD_SEM_TABLE kdrv_md_semtbl[KDRV_MD_SEMAPHORE_COUNT] = {
	// Cnt > 0, Max >= Cnt, ExtInfo & Attr is not referenced now
	// The sequence must be sync to enum member in DRV_SEMAPHORE
//  Semaphore ID,       Attr    Counter     Max counter
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_MD,
};
#else
KDRV_MD_SEM_TABLE kdrv_md_semtbl[KDRV_MD_SEMAPHORE_COUNT];
#endif

/*
    Install ipl kdrv data

    Install ipl kdrv data. This API will be called when system start.

    @return void
*/

void kdrv_md_install_id(void)
{
	UINT32  i;

	// Flag
	for (i = 0; i < KDRV_MD_FLAG_COUNT; i++) {
		OS_CONFIG_FLAG(kdrv_md_flag_id[i]);
	}

	// Semaphore
	for (i = 0; i < KDRV_MD_SEMAPHORE_COUNT; i++) {
#if defined __UITRON || defined __ECOS
		OS_CONFIG_SEMPHORE(kdrv_md_semtbl[i].semphore_id, kdrv_md_semtbl[i].attribute, kdrv_md_semtbl[i].counter, kdrv_md_semtbl[i].max_counter);
#else
		SEM_CREATE(kdrv_md_semtbl[i].semphore_id, SEM_MAX_CNT);
#endif
	}
}

void kdrv_md_uninstall_id(void)
{
#if defined __UITRON || defined __ECOS
#else
	UINT32  i;

	// Flag
	for (i = 0; i < KDRV_MD_FLAG_COUNT; i++) {
		rel_flg(kdrv_md_flag_id[i]);
	}
	// Semaphore
	for (i = 0; i < KDRV_MD_SEMAPHORE_COUNT; i++) {
		SEM_DESTROY(kdrv_md_semtbl[i].semphore_id);
	}
#endif
}



SEM_HANDLE* kdrv_md_get_sem_id(KDRV_MD_SEM idx)
{
	if (idx >= KDRV_MD_SEMAPHORE_COUNT)
	{
		return 0;
	}
	return &kdrv_md_semtbl[idx].semphore_id;
}

ID kdrv_md_get_flag_id(KDRV_MD_FLAG idx)
{
	if (idx >= KDRV_MD_FLAG_COUNT)
	{
		return 0;
	}
	return kdrv_md_flag_id[idx];
}


void kdrv_md_init(void)
{
	set_flg(kdrv_md_get_flag_id(FLG_ID_KDRV_MD), KDRV_MD_INIT_BITS);
}

