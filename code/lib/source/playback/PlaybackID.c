#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

#define PRI_PLAYBACK        12
#define STKSIZE_PLAYBACK    4096

THREAD_HANDLE PLAYBAKTSK_ID = 0; // File Service: low priority
ID FLG_ID_PLAYBACK = 0;
//#NT#2012/05/15#Lincy Lin -begin
//#NT#Porting DispSrv
ID FLG_ID_PB_DISP = 0;
//#NT#2012/05/15#Lincy Lin -end

THREAD_HANDLE PLAYBAKDISPTSK_ID = 0;
ID FLG_ID_PB_DRAW = 0;

void PB_InstallID(void)
{
	OS_CONFIG_FLAG(FLG_ID_PLAYBACK);
	OS_CONFIG_FLAG(FLG_ID_PB_DISP);
	OS_CONFIG_FLAG(FLG_ID_PB_DRAW);
}

void PB_UninstallID(void)
{
	vos_flag_destroy(FLG_ID_PLAYBACK);
	vos_flag_destroy(FLG_ID_PB_DISP);
	vos_flag_destroy(FLG_ID_PB_DRAW);
}


