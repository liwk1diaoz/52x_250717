#ifndef _EMMC_H
#define _EMMC_H

#include <strg_def.h>

extern PSTORAGE_OBJ emmc_getStorageObject(STRG_OBJ_ID strgObjID);
extern void emmc_set_dev_node(char *pStr);

#endif