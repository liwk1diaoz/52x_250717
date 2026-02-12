#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "Utility/SwTimer.h"

/*
    System initial to start playback.
    This is internal API.

    @return E_SYS: init NG.
            E_OK: init OK.
*/
INT32 PB_SysInit(void)
{
	// timer setting
	if (SwTimer_Open((PSWTIMER_ID)&gMenuPlayInfo.PlayTimeOutID, PB_PlayTimerOutHdl) != E_OK) {
		DBG_ERR("Can't Open PlayTimerOut !\r\n");
		return E_SYS;
	}
	return E_OK;
}
