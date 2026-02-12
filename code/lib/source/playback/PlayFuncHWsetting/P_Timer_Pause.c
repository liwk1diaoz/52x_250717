#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "Utility/SwTimer.h"

/*
    Pause playback timer.
    This is internal API.

    @return void
*/
void PB_TimerPause(UINT32 TimerID)
{
	#if 1//_TODO
	SwTimer_Stop(TimerID);
	#endif
}

/*
    Pause playback timer callback API.
    This is internal API.

    @return void
*/
void PB_JpgDecodeTimerPauseCB(void)
{
	PB_TimerPause(gMenuPlayInfo.PlayTimeOutID);
}

