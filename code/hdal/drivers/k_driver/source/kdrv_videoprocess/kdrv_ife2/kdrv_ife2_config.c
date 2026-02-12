/*
    IPL DAL configuration file.

    IPL DAL configuration file. Define semaphore ID, flag ID, etc.

    @file       kdrv_ife2_config.c
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include "kdrv_videoprocess/kdrv_ipp_config.h"
// ==================================================================
// Flag
// ==================================================================
#if 1
ID kdrv_ife2_flag_id[KDRV_IFE2_FLAG_COUNT] = {0};
#else
UINT32 kdrv_ife2_flag_id[KDRV_IFE2_FLAG_COUNT] = {0};
#endif

// ==================================================================
// Semaphore
// ==================================================================
#define SEM_MAX_CNT 1
#if defined __UITRON || defined __ECOS
KDRV_IPP_SEM_TABLE kdrv_ife2_semtbl[KDRV_IFE2_SEMAPHORE_COUNT] = {
	// Cnt > 0, Max >= Cnt, ExtInfo & Attr is not referenced now
	// The sequence must be sync to enum member in DRV_SEMAPHORE
//  Semaphore ID,       Attr    Counter     Max counter
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_IFE2,
};
#else
static KDRV_IPP_SEM_TABLE kdrv_ife2_semtbl[KDRV_IFE2_SEMAPHORE_COUNT];
#endif
/*
    Install ipl kdrv data

    Install ipl kdrv data. This API will be called when system start.

    @return void
*/

void kdrv_ife2_install_id(void)
{
	UINT32  i;

	// Flag
	for (i = 0; i < KDRV_IFE2_FLAG_COUNT; i++) {
		OS_CONFIG_FLAG(kdrv_ife2_flag_id[i]);
	}

	// Semaphore
	for (i = 0; i < KDRV_IFE2_SEMAPHORE_COUNT; i++) {
#if defined __UITRON || defined __ECOS
		OS_CONFIG_SEMPHORE(kdrv_ife2_semtbl[i].semphore_id, kdrv_ife2_semtbl[i].attribute, kdrv_ife2_semtbl[i].counter, kdrv_ife2_semtbl[i].max_counter);
#else
		SEM_CREATE(kdrv_ife2_semtbl[i].semphore_id, SEM_MAX_CNT);
#endif
	}
}

void kdrv_ife2_uninstall_id(void)
{
#if defined __UITRON || defined __ECOS
#else
	UINT32  i;

	// Flag
	for (i = 0; i < KDRV_IFE2_FLAG_COUNT; i++) {
		rel_flg(kdrv_ife2_flag_id[i]);
	}
	// Semaphore
	for (i = 0; i < KDRV_IFE2_SEMAPHORE_COUNT; i++) {
		SEM_DESTROY(kdrv_ife2_semtbl[i].semphore_id);
	}
#endif
}



SEM_HANDLE *kdrv_ife2_get_sem_id(KDRV_IFE2_SEM idx)
{
	if (idx >= KDRV_IFE2_SEMAPHORE_COUNT) {
		return 0;
	}
	return &kdrv_ife2_semtbl[idx].semphore_id;
}

ID kdrv_ife2_get_flag_id(KDRV_IFE2_FLAG idx)
{
	if (idx >= KDRV_IFE2_FLAG_COUNT) {
		return 0;
	}
	return kdrv_ife2_flag_id[idx];
}


void kdrv_ife2_flow_init(void)
{
	set_flg(kdrv_ife2_get_flag_id(FLG_ID_KDRV_IFE2), KDRV_IPP_IFE2_INIT_BITS);
}

