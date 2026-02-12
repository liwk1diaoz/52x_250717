#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/*
    Wait playback timer callback API.
    This is internal API.

    @return void
*/
void PB_JpgDecodeTimerWaitCB(void)
{
	FLGPTN uiFlag;

	wai_flg(&uiFlag, FLG_PB, FLGPB_TIMEOUT, TWF_ORW | TWF_CLR);
}