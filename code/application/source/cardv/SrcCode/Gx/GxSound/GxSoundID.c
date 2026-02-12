#include "GxSoundIntr.h"

#define PRI_PLAYSOUND            3
#define STKSIZE_PLAYSOUND        1024

extern void PlaySoundTsk(void);
extern void PlayDataTsk(void);

THREAD_HANDLE PLAYSOUNDTSK_ID = 0;
THREAD_HANDLE PLAYDATATSK_ID = 0;

ID FLG_ID_SOUND = 0;
ID FLG_ID_DATA = 0;


void GxSound_InstallID(void)
{
	//OS_CONFIG_TASK(PLAYSOUNDTSK_ID,   PRI_PLAYSOUND,     STKSIZE_PLAYSOUND,      PlaySoundTsk);
	OS_CONFIG_FLAG(FLG_ID_SOUND);
	OS_CONFIG_FLAG(FLG_ID_DATA);
}

void GxSound_UninstallID(void)
{
	rel_flg(FLG_ID_SOUND);
	rel_flg(FLG_ID_DATA);
}
BOOL GxSound_ChkModInstalled(void)
{
	if (PLAYSOUNDTSK_ID) {
		return TRUE;
	} else {
		return FALSE;
	}
}
