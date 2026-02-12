/*
    NVT Media interface.

    This file is Media interface for media 5.0

    @file       NvtMediaInterface.c
    @ingroup    mIAPPNVTMEDIAINTERFACE

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/


#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#ifdef __KERNEL__
#include <linux/string.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#else
#include <string.h>
#endif

#include "nvt_media_interface.h"

#define MODULE_DBGLVL       2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER

#define __MODULE__          nvt_media_interface
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int nvt_media_interface_debug_level = NVT_DBG_WRN;
#ifdef __KERNEL__
module_param_named(debug_level_nvt_media_interface, nvt_media_interface_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level_nvt_media_interface, "nvt_media_interface debug level");
#else
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#endif

#if defined (__UITRON)
#include "NvtVerInfo.h"
NVTVER_VERSION_ENTRY(MovInterface, 1, 00, 000, 00)
#endif


#define NMI_UNIT_MAX 15

typedef struct _NMI_UNIT_ENTRY {
	NMI_UNIT *pUnit;
}
NMI_UNIT_ENTRY;

static NMI_UNIT_ENTRY gNMI_UnitTable[NMI_UNIT_MAX];

int gNMI_UnitCount = 0;


BOOL NMI_AddUnit(NMI_UNIT *pUnit)
{
	if (!pUnit) {
		DBG_ERR("[NMI] empty unit\r\n");
		return FALSE;
	}

	if (gNMI_UnitCount == NMI_UNIT_MAX) {
		DBG_ERR("[NMI] table list is full\r\n");
		return FALSE;
	}

	//DBG_DUMP("[NMI] Add %s unit\r\n", pUnit->Name);
	gNMI_UnitTable[gNMI_UnitCount].pUnit = pUnit;
	gNMI_UnitCount++;
	return TRUE;
}

NMI_UNIT *NMI_GetUnit(const CHAR *pUnitName)
{
	INT32 i = 0;

	for (i = 0; i < gNMI_UnitCount; i++) {
		if (strcmp(gNMI_UnitTable[i].pUnit->Name, pUnitName) == 0) { //match "module" string by table->pName
			//match!
			//DBG_DUMP("[NMI] Get %s unit\r\n", gNMI_UnitTable[i].pUnit->Name);
			return gNMI_UnitTable[i].pUnit;
		}
	}

	DBG_ERR("[NMI] unit not found\r\n");
	return NULL;
}
