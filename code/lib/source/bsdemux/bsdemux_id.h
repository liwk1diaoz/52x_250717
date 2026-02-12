/**
    Install kernel resource ID for NMedia Play BsDemux

    Install kernel resource ID for NMedia Play BsDemux

    @file       nmediaplay_bsdemux_id.h
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "bsdemux.h"

/**
	Task ID
*/
extern THREAD_HANDLE _SECTION(".kercfg_data") NMP_BSDEMUX_TSK_ID;
extern THREAD_HANDLE _SECTION(".kercfg_data") NMP_TSDEMUX_TSK_ID;

/**
	Flag ID
*/
extern ID _SECTION(".kercfg_data") NMP_BSDEMUX_FLG_ID;
extern ID _SECTION(".kercfg_data") NMP_TSDEMUX_FLG_ID;

/**
	Semaphore ID
*/
extern SEM_HANDLE _SECTION(".kercfg_data") NMP_BSDEMUX_VDO_SEM_ID[NMP_BSDEMUX_MAX_PATH];
extern SEM_HANDLE _SECTION(".kercfg_data") NMP_BSDEMUX_AUD_SEM_ID[NMP_BSDEMUX_MAX_PATH];

extern void NMP_BsDemux_InstallID(void) _SECTION(".kercfg_text");
extern void NMP_BsDemux_UninstallID(void);

//@}
