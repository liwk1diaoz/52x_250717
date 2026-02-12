#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

void PB_CaptureScreen(PPB_LOGO_INFO pPBLogoInfo)
{
	if (NULL != pPBLogoInfo) {
		g_stPBCSNInfo.puiFileAddr = &pPBLogoInfo->uiFileAddr;
		g_stPBCSNInfo.puiFileSize = &pPBLogoInfo->uiFileSize;
		if ((0 == pPBLogoInfo->uiDstWidth) || (0 == pPBLogoInfo->uiDstHeight)) {
			// Special case: using panel size
			g_stPBCSNInfo.uiDstWidth = g_PBDispInfo.uiDisplayW;
			g_stPBCSNInfo.uiDstHeight = g_PBDispInfo.uiDisplayH;
		} else {
			g_stPBCSNInfo.uiDstWidth = pPBLogoInfo->uiDstWidth;
			g_stPBCSNInfo.uiDstHeight = pPBLogoInfo->uiDstHeight;
		}
		PB_PlaySetCmmd(PB_CMMD_CAPTURE_SCREEN);
	} else {
		DBG_ERR("parameter NG! \r\n");
		PB_SetFinishFlag(DECODE_JPG_READERROR);
	}
}

