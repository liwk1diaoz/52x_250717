#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/*
    Playback timeout handler.
    This is internal API.

    @param[in] data
    @return void
*/
void PB_PlayTimerOutHdl(UINT32 uiEvent)
{
    CHKPNT;
	set_flg(FLG_PB, FLGPB_TIMEOUT);
}
