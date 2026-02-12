/*
    VPE KDRV configuration file.

    VPE KDRV configuration file. Define semaphore ID, flag ID, etc.

    @file       kdrv_vpe_config.c
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include "kdrv_videoprocess/kdrv_ipp_config.h"
// ==================================================================
// Flag
// ==================================================================

#if 1
ID kdrv_vpe_flag_id[KDRV_VPE_FLAG_COUNT] = {0};
#else
UINT32 kdrv_vpe_flag_id[KDRV_VPE_FLAG_COUNT] = {0};
#endif

// ==================================================================
// Semaphore
// ==================================================================
#define SEM_MAX_CNT 1
#if defined __UITRON || defined __ECOS
KDRV_IPP_SEM_TABLE kdrv_vpe_semtbl[KDRV_VPE_SEMAPHORE_COUNT] = {
	// Cnt > 0, Max >= Cnt, ExtInfo & Attr is not referenced now
	// The sequence must be sync to enum member in DRV_SEMAPHORE
//  Semaphore ID,       Attr    Counter     Max counter
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_VPE,
};
#else
static KDRV_IPP_SEM_TABLE kdrv_vpe_semtbl[KDRV_VPE_SEMAPHORE_COUNT];
#endif
/*
    Install ipl kdrv data

    Install ipl kdrv data. This API will be called when system start.

    @return void
*/

void kdrv_vpe_install_id(void)
{
	UINT32  i;

	// Flag
	for (i = 0; i < KDRV_VPE_FLAG_COUNT; i++) {
		OS_CONFIG_FLAG(kdrv_vpe_flag_id[i]);
	}

	// Semaphore
	for (i = 0; i < KDRV_VPE_SEMAPHORE_COUNT; i++) {
#if defined __UITRON || defined __ECOS
		OS_CONFIG_SEMPHORE(kdrv_vpe_semtbl[i].semphore_id, kdrv_vpe_semtbl[i].attribute, kdrv_vpe_semtbl[i].counter, kdrv_vpe_semtbl[i].max_counter);
#else
		SEM_CREATE(kdrv_vpe_semtbl[i].semphore_id, SEM_MAX_CNT);
#endif
	}
}

void kdrv_vpe_uninstall_id(void)
{
#if defined __UITRON || defined __ECOS
#else
	UINT32  i;

	// Flag
	for (i = 0; i < KDRV_VPE_FLAG_COUNT; i++) {
		rel_flg(kdrv_vpe_flag_id[i]);
	}
	// Semaphore
	for (i = 0; i < KDRV_VPE_SEMAPHORE_COUNT; i++) {
		SEM_DESTROY(kdrv_vpe_semtbl[i].semphore_id);
	}
#endif
}

SEM_HANDLE *kdrv_vpe_get_sem_id(KDRV_VPE_SEM idx)
{
	if (idx >= KDRV_VPE_SEMAPHORE_COUNT) {
		return 0;
	}
	return &kdrv_vpe_semtbl[idx].semphore_id;
}

ID kdrv_vpe_get_flag_id(KDRV_VPE_FLAG idx)
{
	if (idx >= KDRV_VPE_FLAG_COUNT) {
		return 0;
	}
	return kdrv_vpe_flag_id[idx];
}


void kdrv_vpe_flow_init(void)
{
	set_flg(kdrv_vpe_get_flag_id(FLG_ID_KDRV_VPE), KDRV_IPP_VPE_INIT_BITS);
}

