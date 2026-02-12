#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
//#include "NvtVerInfo.h"


PB_ERR PB_Open(PPLAY_OBJ pPlayObj)
{
	if (guiOnPlaybackMode) {
		return PBERR_FAIL;
	}

	PB_InstallID();

	// Playback system initial
	if (PB_SysInit() != E_OK) {
		return PBERR_FAIL;
	}

	// update playback-obj info
	if (xPB_SetPBObject(pPlayObj) != E_OK) {
		return PBERR_FAIL;
	}

	gMenuPlayInfo.CurrentMode = PLAYMODE_UNKNOWN;

	//#NT#2012/05/17#Lincy Lin -begin
	//#NT#Porting Disp srv
	PB_DispSrv_Cfg();
	PB_DispSrv_SetDrawAct(PB_DISPSRV_DRAW | PB_DISPSRV_FLIP);
	//#NT#2012/05/17#Lincy Lin -end

	clr_flg(FLG_PB, FLGPB_MASKALL);
//#NT#2015/08/26#Scottie -begin
//#NT#Remove to avoid PB not idle yet
//    set_flg(FLG_PB, FLGPB_NOTBUSY);
//#NT#2015/08/26#Scottie -end

	guiOnPlaybackMode = TRUE;
	gucPlayTskWorking = TRUE;

	guiUserJPGOutWidth  = 0;
	guiUserJPGOutHeight = 0;
	guiUserJPGOutStartX = 0;
	guiUserJPGOutStartY = 0;

	THREAD_CREATE(PLAYBAKTSK_ID, PlaybackTsk, NULL, "PlaybackTsk");
	if (PLAYBAKTSK_ID == 0) {
		DBG_ERR("PLAYBAKTSK_ID = %d\r\n", (unsigned int)PLAYBAKTSK_ID);
		return E_SYS;
	}
	THREAD_RESUME(PLAYBAKTSK_ID);

	THREAD_CREATE(PLAYBAKDISPTSK_ID, PlaybackDispTsk, NULL, "PlaybackDispTsk");
	if (PLAYBAKDISPTSK_ID == 0) {
		DBG_ERR("PLAYBAKTSK_ID = %d\r\n", (unsigned int)PLAYBAKDISPTSK_ID);
		return E_SYS;
	}
	THREAD_RESUME(PLAYBAKDISPTSK_ID);

	return PBERR_OK;
}

