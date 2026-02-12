#ifndef _UI_DATEIMPRINT_ID_H
#define _UI_DATEIMPRINT_ID_H

//#include "SysKer.h"
#include "kwrap/flag.h"


extern ID _SECTION(".kercfg_data") UI_DATEIMPRINT_FLG_ID;

extern void UiDateImprint_InstallID(void) _SECTION(".kercfg_text");
extern void UiDateImprint_UnInstallID(void) _SECTION(".kercfg_text");

#endif //_UI_DATEIMPRINT_ID_H

