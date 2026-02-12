#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

INT32 PB_WaitCommandFinish(PB_WAIT_MODE TimeOut)
{
	FLGPTN nFlag;

	// check if playback not start
	if (guiOnPlaybackMode != TRUE) {
		return PB_STA_ERR_TASK;
	}

	if (TimeOut == PB_WAIT_INFINITE) {
		wai_flg(&nFlag, FLG_PB, FLGPB_DONE | FLGPB_NOIMAGE | FLGPB_WRITEERROR |
				FLGPB_READERROR | FLGPB_DECODEERROR | FLGPB_INITFAIL, TWF_ORW);
	}

	// return flag that been set before
	if (kchk_flg(FLG_PB, FLGPB_DONE)) {
		return PB_STA_DONE;
	} else if (kchk_flg(FLG_PB, FLGPB_NOIMAGE)) {
		return PB_STA_NOIMAGE;
	} else if (kchk_flg(FLG_PB, FLGPB_READERROR)) {
		return PB_STA_ERR_FILE;
	} else if (kchk_flg(FLG_PB, FLGPB_DECODEERROR)) {
		return PB_STA_ERR_DECODE;
	} else if (kchk_flg(FLG_PB, FLGPB_WRITEERROR)) {
		return PB_STA_ERR_WRITE;
	} else if (kchk_flg(FLG_PB, FLGPB_INITFAIL)) {
		return PB_STA_INITFAIL;
	} else if (kchk_flg(FLG_PB, FLGPB_NOTBUSY)) {
		return PB_STA_STANDBY;
	} else {
		return PB_STA_BUSY;
	}
}

