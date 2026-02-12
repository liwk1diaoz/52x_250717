/*
    IME KDRV configuration file.

    IME KDRV configuration file. Define semaphore ID, flag ID, etc.

    @file       kdrv_ime_config.c
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include "kdrv_videoprocess/kdrv_ipp_config.h"
// ==================================================================
// Flag
// ==================================================================

#if 1
ID kdrv_ime_flag_id[KDRV_IME_FLAG_COUNT] = {0};
#else
UINT32 kdrv_ime_flag_id[KDRV_IME_FLAG_COUNT] = {0};
#endif

// ==================================================================
// Semaphore
// ==================================================================
#define SEM_MAX_CNT 1
#if defined __UITRON || defined __ECOS
KDRV_IPP_SEM_TABLE kdrv_ime_semtbl[KDRV_IME_SEMAPHORE_COUNT] = {
	// Cnt > 0, Max >= Cnt, ExtInfo & Attr is not referenced now
	// The sequence must be sync to enum member in DRV_SEMAPHORE
//  Semaphore ID,       Attr    Counter     Max counter
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_IME,
};
#else
static KDRV_IPP_SEM_TABLE kdrv_ime_semtbl[KDRV_IME_SEMAPHORE_COUNT];
#endif
/*
    Install ipl kdrv data

    Install ipl kdrv data. This API will be called when system start.

    @return void
*/

void kdrv_ime_install_id(void)
{
	UINT32  i;

	// Flag
	for (i = 0; i < KDRV_IME_FLAG_COUNT; i++) {
		OS_CONFIG_FLAG(kdrv_ime_flag_id[i]);
	}

	// Semaphore
	for (i = 0; i < KDRV_IME_SEMAPHORE_COUNT; i++) {
#if defined __UITRON || defined __ECOS
		OS_CONFIG_SEMPHORE(kdrv_ime_semtbl[i].semphore_id, kdrv_ime_semtbl[i].attribute, kdrv_ime_semtbl[i].counter, kdrv_ime_semtbl[i].max_counter);
#else
		SEM_CREATE(kdrv_ime_semtbl[i].semphore_id, SEM_MAX_CNT);
#endif
	}
}

void kdrv_ime_uninstall_id(void)
{
#if defined __UITRON || defined __ECOS
#else
	UINT32  i;

	// Flag
	for (i = 0; i < KDRV_IME_FLAG_COUNT; i++) {
		rel_flg(kdrv_ime_flag_id[i]);
	}
	// Semaphore
	for (i = 0; i < KDRV_IME_SEMAPHORE_COUNT; i++) {
		SEM_DESTROY(kdrv_ime_semtbl[i].semphore_id);
	}
#endif
}



SEM_HANDLE *kdrv_ime_get_sem_id(KDRV_IME_SEM idx)
{
	if (idx >= KDRV_IME_SEMAPHORE_COUNT) {
		return 0;
	}
	return &kdrv_ime_semtbl[idx].semphore_id;
}

ID kdrv_ime_get_flag_id(KDRV_IME_FLAG idx)
{
	if (idx >= KDRV_IME_FLAG_COUNT) {
		return 0;
	}
	return kdrv_ime_flag_id[idx];
}


void kdrv_ime_flow_init(void)
{
	set_flg(kdrv_ime_get_flag_id(FLG_ID_KDRV_IME), KDRV_IPP_IME_INIT_BITS);
}

