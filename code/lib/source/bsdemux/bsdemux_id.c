/**
    Install kernel resource ID for NMedia Play BsDemux

    Install kernel resource ID for NMedia Play BsDemux

    @file       NMediaPlayBsDemux_ID.c
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "nmediaplay_api.h"
#include "bsdemux_id.h"
#include "bsdemux.h"

/**
	Task priority
*/
#define NMP_BSDMX_DEMUX_PRI          8
#define NMP_TSDEMUX_PRI              16

/**
	Stack size
*/
#define NMP_BSDMX_DEMUX_STKSIZE      2048
#define NMP_TSDEMUX_STKSIZE          1024

/**
	Task ID
*/
THREAD_HANDLE  NMP_BSDEMUX_TSK_ID    = 0;
THREAD_HANDLE  NMP_TSDEMUX_TSK_ID    = 0;

/**
	Flag ID
*/
ID             NMP_BSDEMUX_FLG_ID    = 0;
ID             NMP_TSDEMUX_FLG_ID    = 0;

/**
	Semaphore ID
*/
SEM_HANDLE     NMP_BSDEMUX_VDO_SEM_ID[NMP_BSDEMUX_MAX_PATH] = {0};
SEM_HANDLE     NMP_BSDEMUX_AUD_SEM_ID[NMP_BSDEMUX_MAX_PATH] = {0};

void NMP_BsDemux_InstallID(void)
{
	UINT32 i = 0;

#if defined __UITRON || defined __ECOS
	OS_CONFIG_TASK(NMP_BSDEMUX_TSK_ID, NMP_BSDMX_DEMUX_PRI, NMP_BSDMX_DEMUX_STKSIZE, _NMP_BsDemux_Demux_Tsk);
	OS_CONFIG_TASK(NMP_TSDEMUX_TSK_ID, NMP_TSDEMUX_PRI, NMP_TSDEMUX_STKSIZE, _NMP_TsDemux_Tsk);
#endif

	OS_CONFIG_FLAG(NMP_BSDEMUX_FLG_ID);
	OS_CONFIG_FLAG(NMP_TSDEMUX_FLG_ID);

	for (i = 0; i < NMP_BSDEMUX_MAX_PATH; i++) {
#if defined __UITRON || defined __ECOS
		OS_CONFIG_SEMPHORE(NMP_BSDEMUX_VDO_SEM_ID, 0, 1, 1);
		OS_CONFIG_SEMPHORE(NMP_BSDEMUX_AUD_SEM_ID, 0, 1, 1);
#else
		SEM_CREATE(NMP_BSDEMUX_VDO_SEM_ID[i], 1);
		SEM_CREATE(NMP_BSDEMUX_AUD_SEM_ID[i], 1);
#endif
	}
}

void NMP_BsDemux_UninstallID(void) 
{
	UINT32 i = 0;

	rel_flg(NMP_BSDEMUX_FLG_ID);
	rel_flg(NMP_TSDEMUX_FLG_ID);

	for (i = 0; i < NMP_BSDEMUX_MAX_PATH; i++) {
		SEM_DESTROY(NMP_BSDEMUX_VDO_SEM_ID[i]);
		SEM_DESTROY(NMP_BSDEMUX_AUD_SEM_ID[i]);
	}
}
//@}
