#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/*
    Set current playback status & flag.
    This is internal API.

    @param Error_check
    @return void
*/
void PB_SetFinishFlag(INT32 Error_check)
{
	switch (Error_check) {
	case DECODE_JPG_DECODEERROR:
		gusCurrPlayStatus = PB_STA_ERR_DECODE;
		set_flg(FLG_PB, FLGPB_DECODEERROR);
		DBG_ERR("Set Flag Decode error\r\n");
		break;

	case DECODE_JPG_READERROR:
		gusCurrPlayStatus = PB_STA_ERR_FILE;
		set_flg(FLG_PB, FLGPB_READERROR);
		DBG_ERR("Set Flag Read File error\r\n");
		break;

	case DECODE_JPG_WRITEERROR:
		gusCurrPlayStatus = PB_STA_ERR_WRITE;
		set_flg(FLG_PB, FLGPB_WRITEERROR);
		DBG_ERR("Set Flag Write File error\r\n");
		break;

	case DECODE_JPG_FILESYSFAIL:
		gusCurrPlayStatus = PB_STA_INITFAIL;
		set_flg(FLG_PB, FLGPB_INITFAIL);
		DBG_ERR("Set Flag Init File error\r\n");
		break;

	case DECODE_JPG_NOIMAGE:
		// Clear Frame buffer
		xPB_DirectClearFrameBuffer(g_uiDefaultBGColor);
		gusCurrPlayStatus = PB_STA_NOIMAGE;
		set_flg(FLG_PB, FLGPB_NOIMAGE);
		DBG_WRN("Set Flag No image File\r\n");
		break;

	default:
		gusCurrPlayStatus = PB_STA_DONE;
		set_flg(FLG_PB, FLGPB_DONE);
		DBG_IND("Set Flag Done\r\n");
		break;
	}
}
