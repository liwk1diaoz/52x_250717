#include <stdio.h>
#include <kwrap/type.h>
#include <kwrap/debug.h>
#include <strg_def.h>
#include "nand_int.h"

//nand
extern STORAGE_OBJ gNandFWObj1;
extern STORAGE_OBJ gNandFWObj2;
extern STORAGE_OBJ gNandFWObj3;
extern STORAGE_OBJ gNandFWObj4;
extern STORAGE_OBJ gNandFWObj5;
extern STORAGE_OBJ gNandFWObj6;
extern STORAGE_OBJ gNandFWObj7;
extern STORAGE_OBJ gNandFWObj8;
PSTORAGE_OBJ gNandStrgObjectFSWTab[] = {
	&gNandFWObj1,
	&gNandFWObj2,
	&gNandFWObj3,
	&gNandFWObj4,
	&gNandFWObj5,
	&gNandFWObj6,
	&gNandFWObj7,
	&gNandFWObj8,
};

PSTORAGE_OBJ nand_getStorageObject(STRG_OBJ_ID strgObjID)
{
	// Reserved area
	if (strgObjID < STRG_OBJ_FAT1) {
		STRG_OBJ_ID support_cnt = sizeof(gNandStrgObjectFSWTab)/sizeof(PSTORAGE_OBJ);
		if (strgObjID >= support_cnt) {
			DBG_ERR("strgObjID too large: %d (limit:%d)\n", strgObjID, support_cnt);
			return NULL;
		}
		return (PSTORAGE_OBJ)gNandStrgObjectFSWTab[strgObjID];
	}

	DBG_ERR("nand obect not support FAT or PStore.\n");
	return NULL;
}

//sdio-1
extern STORAGE_OBJ gSDIOObj_fat;
PSTORAGE_OBJ sdio_getStorageObject(STRG_OBJ_ID strgObjID)
{
	if (strgObjID == STRG_OBJ_FAT1) {
		return &gSDIOObj_fat;
	}

	DBG_ERR("sdio object only support STRG_OBJ_FAT1.\n");
	return NULL;
}

//emmc
extern STORAGE_OBJ gEMMCObj_fat;
PSTORAGE_OBJ emmc_getStorageObject(STRG_OBJ_ID strgObjID)
{
	if (strgObjID == STRG_OBJ_FAT1) {
		return &gEMMCObj_fat;
	}

	DBG_ERR("emmc object only support STRG_OBJ_FAT1.\n");
	return NULL;
}

