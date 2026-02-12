#include <sdio.h>
#include <strg_def.h>
#include <hdal.h>
#include "sys_mempool.h"
#include "sys_card.h"

BOOL card_det_insert(void)
{
	return TRUE;
}

BOOL card_det_wp(void)
{
	return FALSE;
}


void card_init(void)
{
	PSTORAGE_OBJ   pStrg;

	pStrg = sdio_getStorageObject(STRG_OBJ_FAT1);
#if defined(__FREERTOS)
	sdio_setCallBack(SDIO_CALLBACK_CARD_DETECT, (SDIO_CALLBACK_HDL)card_det_insert);
	sdio_setCallBack(SDIO_CALLBACK_WRITE_PROTECT, (SDIO_CALLBACK_HDL)card_det_wp);
#endif
	pStrg->SetParam(STRG_SET_MEMORY_REGION, mempool_storage_sdio, POOL_SIZE_STORAGE_SDIO);
}
