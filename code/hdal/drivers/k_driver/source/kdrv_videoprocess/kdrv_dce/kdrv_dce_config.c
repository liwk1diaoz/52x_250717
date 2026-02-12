/*
    IPL KDRV configuration file.

    IPL KDRV configuration file. Define semaphore ID, flag ID, etc.

    @file       KDRV_DCE_config.c
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
//#include "kdrv_dce_config.h"
#include "kdrv_videoprocess/kdrv_ipp_config.h"
// ==================================================================
// Flag
// ==================================================================
ID kdrv_dce_flag_id[KDRV_DCE_FLAG_COUNT] = {0};

// ==================================================================
// Semaphore
// ==================================================================
#define SEM_MAX_CNT 1
#if defined __UITRON || defined __ECOS
KDRV_IPP_SEM_TABLE kdrv_dce_semtbl[KDRV_DCE_SEMAPHORE_COUNT] = {
	// Cnt > 0, Max >= Cnt, ExtInfo & Attr is not referenced now
	// The sequence must be sync to enum member in DRV_SEMAPHORE
//  Semaphore ID,       Attr    Counter     Max counter
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_RHE,
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_IFE,
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_DCE,
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_IPE,
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_IME,
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_IFE2,
};
#else
KDRV_IPP_SEM_TABLE kdrv_dce_semtbl[KDRV_DCE_SEMAPHORE_COUNT];
#endif
/*
    Install ipl kdrv data

    Install ipl kdrv data. This API will be called when system start.

    @return void
*/

void kdrv_dce_install_id(void)
{
	UINT32  i;

	// Flag
	for (i = 0; i < KDRV_DCE_FLAG_COUNT; i++) {
		OS_CONFIG_FLAG(kdrv_dce_flag_id[i]);
	}

	// Semaphore
	for (i = 0; i < KDRV_DCE_SEMAPHORE_COUNT; i++) {
#if defined __UITRON || defined __ECOS
		OS_CONFIG_SEMPHORE(kdrv_dce_semtbl[i].semphore_id, kdrv_dce_semtbl[i].attribute, kdrv_dce_semtbl[i].counter, kdrv_dce_semtbl[i].max_counter);
#else
		SEM_CREATE(kdrv_dce_semtbl[i].semphore_id, SEM_MAX_CNT);
#endif
	}
}

void kdrv_dce_uninstall_id(void)
{
#if defined __UITRON || defined __ECOS
#else
	UINT32  i;

	// Flag
	for (i = 0; i < KDRV_DCE_FLAG_COUNT; i++) {
		rel_flg(kdrv_dce_flag_id[i]);
	}
	// Semaphore
	for (i = 0; i < KDRV_DCE_SEMAPHORE_COUNT; i++) {
		SEM_DESTROY(kdrv_dce_semtbl[i].semphore_id);
	}
#endif
}



SEM_HANDLE *kdrv_dce_get_sem_id(KDRV_DCE_SEM idx)
{
	if (idx >= KDRV_DCE_SEMAPHORE_COUNT) {
		return 0;
	}
	return &kdrv_dce_semtbl[idx].semphore_id;
}

ID kdrv_dce_get_flag_id(KDRV_DCE_FLAG idx)
{
	if (idx >= KDRV_DCE_FLAG_COUNT) {
		return 0;
	}
	return kdrv_dce_flag_id[idx];
}


void kdrv_dce_flow_init(void)
{
	set_flg(kdrv_dce_get_flag_id(FLG_ID_KDRV_DCE), KDRV_IPP_DCE_INIT_BITS);
}

