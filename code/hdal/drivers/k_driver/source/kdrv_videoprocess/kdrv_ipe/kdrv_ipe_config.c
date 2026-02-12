/*
    IPL KDRV configuration file.

    IPL KDRV configuration file. Define semaphore ID, flag ID, etc.

    @file       KDRV_IPE_config.c
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#include "kdrv_videoprocess/kdrv_ipp_config.h"
// NOTE: XXXXX
//#if defined(__LINUX)
//#include <linux/slab.h>
//#endif
// ==================================================================
// Flag
// ==================================================================
// NOTE: XXXXX
#if 1
ID kdrv_ipe_flag_id[KDRV_IPE_FLAG_COUNT] = {0};
#else
UINT32 kdrv_ipe_flag_id[KDRV_IPE_FLAG_COUNT] = {0};
#endif
// ==================================================================
// Semaphore
// ==================================================================
#define SEM_MAX_CNT 1
#if defined __UITRON || defined __ECOS
KDRV_IPP_SEM_TABLE kdrv_ipe_semtbl[KDRV_IPE_SEMAPHORE_COUNT] = {
	// Cnt > 0, Max >= Cnt, ExtInfo & Attr is not referenced now
	// The sequence must be sync to enum member in DRV_SEMAPHORE
//  Semaphore ID,       Attr    Counter     Max counter
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_IPE,
};
#else
static KDRV_IPP_SEM_TABLE kdrv_ipe_semtbl[KDRV_IPE_SEMAPHORE_COUNT];
#endif

// ==================================================================
// Internal dram buffer
// ==================================================================
//#define IPE_DRAM_GAMMA_WORD (208)
//#define IPE_DRAM_3DLUT_WORD (912)
//#define IPE_DRAM_YCURVE_WORD (80)

//UINT32 *p_buffer = NULL;
//UINT32 *p_gamma_buffer = NULL;
//UINT32 *p_ycurve_buffer = NULL;
//UINT32 *p_3dcc_buffer = NULL;

/*
    Install ipl kdrv data

    Install ipl kdrv data. This API will be called when system start.

    @return void
*/

void kdrv_ipe_install_id(void)
{
	UINT32  i;

	// Flag
	for (i = 0; i < KDRV_IPE_FLAG_COUNT; i++) {
		OS_CONFIG_FLAG(kdrv_ipe_flag_id[i]);
	}

	// Semaphore
	for (i = 0; i < KDRV_IPE_SEMAPHORE_COUNT; i++) {
#if defined __UITRON || defined __ECOS
		OS_CONFIG_SEMPHORE(kdrv_ipe_semtbl[i].semphore_id, kdrv_ipe_semtbl[i].attribute, kdrv_ipe_semtbl[i].counter, kdrv_ipe_semtbl[i].max_counter);
#else
		SEM_CREATE(kdrv_ipe_semtbl[i].semphore_id, SEM_MAX_CNT);
#endif
	}
}

void kdrv_ipe_uninstall_id(void)
{
#if defined __UITRON || defined __ECOS
#else
	UINT32  i;

	// Flag
	for (i = 0; i < KDRV_IPE_FLAG_COUNT; i++) {
		rel_flg(kdrv_ipe_flag_id[i]);
	}
	// Semaphore
	for (i = 0; i < KDRV_IPE_SEMAPHORE_COUNT; i++) {
		SEM_DESTROY(kdrv_ipe_semtbl[i].semphore_id);
	}
#endif
}

SEM_HANDLE *kdrv_ipe_get_sem_id(KDRV_IPE_SEM idx)
{
	if (idx >= KDRV_IPE_SEMAPHORE_COUNT) {
		return 0;
	}
	return &kdrv_ipe_semtbl[idx].semphore_id;
}

ID kdrv_ipe_get_flag_id(KDRV_IPE_FLAG idx)
{
	if (idx >= KDRV_IPE_FLAG_COUNT) {
		return 0;
	}
	return kdrv_ipe_flag_id[idx];
}

void kdrv_ipe_flow_init(void)
{
	set_flg(kdrv_ipe_get_flag_id(FLG_ID_KDRV_IPE), KDRV_IPP_IPE_INIT_BITS);
}

// NOTE: XXXXX
#if (defined __UITRON || defined __ECOS)

#elif defined(__FREERTOS)
//#define ipe_buffer_size (IPE_DRAM_GAMMA_WORD + IPE_DRAM_YCURVE_WORD)
//UINT32 ipe_buffer[ipe_buffer_size] = {0};
INT32 kdrv_ipe_dma_buffer_allocate(void)
{
	//p_buffer = ipe_buffer;
	//p_gamma_buffer = p_buffer;
	//p_ycurve_buffer = &p_buffer[IPE_DRAM_GAMMA_WORD];

	return E_OK;
}

INT32 kdrv_ipe_dma_buffer_free(void)
{
	//p_buffer = NULL;
	//p_gamma_buffer = NULL;
	//p_ycurve_buffer = NULL;

	return E_OK;
}
#else
INT32 kdrv_ipe_dma_buffer_allocate(void)
{
	//UINT32 size;
	ER er_return = E_OK;

/*
	size = (IPE_DRAM_GAMMA_WORD + IPE_DRAM_YCURVE_WORD) * 4;

	if (p_buffer == NULL) {
		p_buffer = kmalloc(size, GFP_KERNEL);
		if (p_buffer == NULL) {
			//DBG_ERR("kalloc %d fail\r\n", size);
			return E_NOMEM;//-ENOMEM;
		}
		p_gamma_buffer = p_buffer;
		p_ycurve_buffer = &p_buffer[IPE_DRAM_GAMMA_WORD];
	}
*/

#if 0
	size = (IPE_DRAM_3DLUT_WORD * 4);
	if (p_3dcc_buffer == NULL) {
		p_3dcc_buffer = kmalloc(size, GFP_KERNEL);
		if (p_3dcc_buffer == NULL) {
			DBG_ERR("kalloc %d fail\r\n", size);
			return E_NOMEM;
		}
	}
#endif
	return er_return;

}

INT32 kdrv_ipe_dma_buffer_free(void)
{
/*
	if (p_buffer != NULL) {
		kfree(p_buffer);
		p_buffer = NULL;
		p_gamma_buffer = NULL;
		p_ycurve_buffer = NULL;
	}
*/
#if 0
	if (p_3dcc_buffer != NULL) {
		kfree(p_3dcc_buffer);
		p_3dcc_buffer = NULL;
	}
#endif

	return E_OK;
}
#endif
