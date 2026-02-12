#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "Utility/SwTimer.h"
/*
    System un-initial to stop Playback.
    This is internal API.

    @return void
*/
void PB_SysUnInit(void)
{
	#if 1//_TODO
	SwTimer_Close(gMenuPlayInfo.PlayTimeOutID);
	#endif
}