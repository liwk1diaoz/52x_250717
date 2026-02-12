#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/*
    Set current playback command.
    This is internal API.

    @param PBCmmd
    @return void
*/
void PB_PlaySetCmmd(UINT32 PBCmmd)
{
	FLGPTN PlayCMD;

	if (!gucPlayTskWorking) {
		return;
	}

	// Wait playback not busy
	wai_flg(&PlayCMD, FLG_PB, FLGPB_NOTBUSY, TWF_ORW | TWF_CLR);

	guiCurrPlayCmmd = PBCmmd;
	//#NT#2009/02/11#Niven Cho -begin
	//#NT#Added., Fix A1050-F0115 Copy NAND to Card and Plug-In USB at the same time, then un-Plug USB then system hang. Add FLGPB_NOIMAGE for PB_Format case
//#NT#2007/07/18#Corey Ke -begin
//#Fix bug when write to file error
	clr_flg(FLG_PB, FLGPB_DONE | FLGPB_READERROR | FLGPB_DECODEERROR | FLGPB_WRITEERROR | FLGPB_NOIMAGE);
//#NT#2007/07/18#Corey Ke -end
	//#NT#2009/02/11#Niven Cho -end

	set_flg(FLG_PB, FLGPB_COMMAND);
}