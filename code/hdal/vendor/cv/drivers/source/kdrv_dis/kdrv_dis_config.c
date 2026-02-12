/*
    IPL KDRV configuration file.

    IPL KDRV configuration file. Define semaphore ID, flag ID, etc.

    @file       KDRV_DIS_config.c
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "kdrv_dis_int.h"

#if defined(__FREERTOS)
#include <rtosfdt.h>
#include <stdio.h>
#include <libfdt.h>
#include <compiler.h>
#endif
// ==================================================================
// Flag
// ==================================================================
#if defined(__FREERTOS)
ID 		  kdrv_dis_flag_id[KDRV_DIS_FLAG_COUNT] = {0};;
unsigned int dis_clock_freq[1];
#else
UINT32    kdrv_dis_flag_id[KDRV_DIS_FLAG_COUNT] = {0};;
#endif

// ==================================================================
// Semaphore
// ==================================================================
KDRV_DIS_SEM_TABLE kdrv_dis_semtbl[KDRV_DIS_SEMAPHORE_COUNT];
/*
    Install ipl kdrv data

    Install ipl kdrv data. This API will be called when system start.

    @return void
*/

void kdrv_dis_install_id(void)
{
	UINT32  eng = 0;

	for (eng = 0; eng < KDRV_DIS_FLAG_COUNT; eng++) {
		OS_CONFIG_FLAG(kdrv_dis_flag_id[eng]);
		SEM_CREATE(kdrv_dis_semtbl[eng].semphore_id, 1);
	}
}
EXPORT_SYMBOL(kdrv_dis_install_id);


void kdrv_dis_uninstall_id(void)
{
	UINT32  eng = 0;

	for (eng = 0; eng < (UINT32)KDRV_DIS_FLAG_COUNT; eng++) {
		rel_flg(kdrv_dis_flag_id[eng]);
		SEM_DESTROY(kdrv_dis_semtbl[eng].semphore_id);
	}

}
EXPORT_SYMBOL(kdrv_dis_uninstall_id);

void kdrv_dis_init(void)
{
	UINT32 eng = 0;	

	for (eng = 0; eng < (UINT32)KDRV_DIS_FLAG_COUNT; eng++) {		
		set_flg(kdrv_dis_flag_id[eng], KDRV_DIS_INIT_BITS);	
	}
}
EXPORT_SYMBOL(kdrv_dis_init);

SEM_HANDLE *kdrv_dis_get_sem_id(KDRV_DIS_SEM idx)
{
	if (idx >= KDRV_DIS_SEMAPHORE_COUNT) {
		return 0;
	}
	return &kdrv_dis_semtbl[idx].semphore_id;
}

ID kdrv_dis_get_flag_id(KDRV_DIS_FLAG idx)
{
	if (idx >= KDRV_DIS_FLAG_COUNT) {
		return 0;
	}
	return kdrv_dis_flag_id[idx];
}

#if defined(__FREERTOS)
UINT32 kdrv_dis_drv_get_clock_freq(UINT8 clk_idx)
{
	unsigned char *fdt_addr = (unsigned char *)fdt_get_base();
	char path[64] = {0};
	INT nodeoffset;
	UINT32 *cell = NULL;
	UINT32 idx;

	for(idx = 0; idx < 1; idx++) {
		dis_clock_freq[idx] = 0;
	}
	sprintf(path,"/dis@f0c50000");
	nodeoffset = fdt_path_offset((const void*)fdt_addr, path);

	cell = (UINT32 *)fdt_getprop((const void*)fdt_addr, nodeoffset, "clock-frequency", NULL);
	if (cell) {
		dis_clock_freq[0] = be32_to_cpu(cell[0]);
	}

	DBG_IND("DIS_RTOS: dis_clock(%d)\r\n", dis_clock_freq[0]);	

	return dis_clock_freq[clk_idx];
}
#endif