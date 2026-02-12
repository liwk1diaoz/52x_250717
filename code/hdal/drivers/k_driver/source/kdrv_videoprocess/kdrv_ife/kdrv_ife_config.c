/*
    IPL DAL configuration file.

    IPL DAL configuration file. Define semaphore ID, flag ID, etc.

    @file       KDRV_IPP_config.c
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
//#include "ipp/dal/kdrv_ife_config.h"
#include "kdrv_videoprocess/kdrv_ipp_config.h"
// ==================================================================
// Flag
// ==================================================================
#if 1
ID kdrv_ife_flag_id[KDRV_IFE_FLAG_COUNT] = {0};
#else
UINT32 kdrv_ife_flag_id[KDRV_IFE_FLAG_COUNT] = {0};
#endif

// ==================================================================
// Semaphore
// ==================================================================
#define SEM_MAX_CNT 1
#if defined __UITRON || defined __ECOS
KDRV_IPP_SEM_TABLE kdrv_ife_semtbl[KDRV_IFE_SEMAPHORE_COUNT] = {
	// Cnt > 0, Max >= Cnt, ExtInfo & Attr is not referenced now
	// The sequence must be sync to enum member in DRV_SEMAPHORE
//  Semaphore ID,       Attr    Counter     Max counter
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_IFE,
};
#else
static KDRV_IPP_SEM_TABLE kdrv_ife_semtbl[KDRV_IFE_SEMAPHORE_COUNT];
#endif
/*
    Install ipl kdrv data

    Install ipl kdrv data. This API will be called when system start.

    @return void
*/

void kdrv_ife_install_id(void)
{
	UINT32  i;

	// Flag
	for (i = 0; i < KDRV_IFE_FLAG_COUNT; i++) {
		OS_CONFIG_FLAG(kdrv_ife_flag_id[i]);
	}

	// Semaphore
	for (i = 0; i < KDRV_IFE_SEMAPHORE_COUNT; i++) {
#if defined __UITRON || defined __ECOS
		OS_CONFIG_SEMPHORE(kdrv_ife_semtbl[i].semphore_id, kdrv_ife_semtbl[i].attribute, kdrv_ife_semtbl[i].counter, kdrv_ife_semtbl[i].max_counter);
#else
		SEM_CREATE(kdrv_ife_semtbl[i].semphore_id, SEM_MAX_CNT);
#endif
	}
}

void kdrv_ife_uninstall_id(void)
{
#if defined __UITRON || defined __ECOS
#else
	UINT32  i;

	// Flag
	for (i = 0; i < KDRV_IFE_FLAG_COUNT; i++) {
		rel_flg(kdrv_ife_flag_id[i]);
	}
	// Semaphore
	for (i = 0; i < KDRV_IFE_SEMAPHORE_COUNT; i++) {
		SEM_DESTROY(kdrv_ife_semtbl[i].semphore_id);
	}
#endif
}



SEM_HANDLE *kdrv_ife_get_sem_id(KDRV_IFE_SEM idx)
{
	if (idx >= KDRV_IFE_SEMAPHORE_COUNT) {
		return 0;
	}
	return &kdrv_ife_semtbl[idx].semphore_id;
}

ID kdrv_ife_get_flag_id(KDRV_IFE_FLAG idx)
{
	if (idx >= KDRV_IFE_FLAG_COUNT) {
		return 0;
	}
	return kdrv_ife_flag_id[idx];
}


void kdrv_ife_flow_init(void)
{
	set_flg(kdrv_ife_get_flag_id(FLG_ID_KDRV_IFE), KDRV_IPP_IFE_INIT_BITS);
}

