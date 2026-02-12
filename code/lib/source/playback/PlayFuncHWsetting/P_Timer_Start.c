#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "Utility/SwTimer.h"

/*
    Start playback timer.
    This is internal API.

    @param[in] TimerID
    @param[in] TimerValue
    @return void
*/
void PB_TimerStart(UINT32 TimerID, UINT32 TimerValue)
{
	#if 1//_TODO
	SwTimer_Cfg(TimerID, TimerValue, SWTIMER_MODE_FREE_RUN);
	SwTimer_Start(TimerID);
	#endif
}

/*
    Start playback timer call back API.
    This is internal API.

    @return void
*/
void PB_JpgDecodeTimerStartCB(void)
{
	PB_TimerStart(gMenuPlayInfo.PlayTimeOutID, 10);
}

