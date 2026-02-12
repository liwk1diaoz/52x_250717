#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/**
    Set current playback object setting structure.

    @param[in] pPlayObj
    @return void

    Example:
@code
{
    PLAY_OBJ PlayObj;

    PlayObj.uiMemoryAddr = 0x80000000;
    PlayObj.uiMemorySize = 0x00200000;
    ...
    xPB_SetPBObject(&PlayObj);
}
@endcode
*/
INT32 xPB_SetPBObject(PPLAY_OBJ pPlayObj)
{
	xPB_DispSrv_Init4PB();

	memcpy(&g_PBObj, pPlayObj, sizeof(PLAY_OBJ));

#if _TODO
	g_uiPBBufStart = pPlayObj->uiMemoryAddr; //reserve, useing the HDAL common buffer
	g_uiPBBufEnd   = pPlayObj->uiMemoryAddr + pPlayObj->uiMemorySize; //reserve, useing the HDAL common buffer
	guiPlayAllBufSize = g_uiPBBufEnd - g_uiPBBufStart;
#endif

	if (pPlayObj->uiPlayFileFmt & PB_SUPPORT_FMT) {
		gusPlayFileFormat = pPlayObj->uiPlayFileFmt;
	} else {
		gusPlayFileFormat = PB_SUPPORT_FMT;
		g_PBObj.uiPlayFileFmt = PB_SUPPORT_FMT;
	}


#if 0
//TBD: will modify the drawing backgroud image mechanism
	gMenuPlayInfo.uiMovieBGAddrY = pPlayObj->uiMovieBGAddrY;
	gMenuPlayInfo.uiMovieBGAddrCb = pPlayObj->uiMovieBGAddrCb;
	gMenuPlayInfo.uiMovieBGAddrCr = pPlayObj->uiMovieBGAddrCr;

	gMenuPlayInfo.uiBadFileBGAddrY = pPlayObj->uiBadFileBGAddrY;
	gMenuPlayInfo.uiBadFileBGAddrCb = pPlayObj->uiBadFileBGAddrCb;
	gMenuPlayInfo.uiBadFileBGAddrCr = pPlayObj->uiBadFileBGAddrCr;
#endif

	// Memory alloc..
	return PB_AllocPBMemory(g_PBDispInfo.uiDisplayW, g_PBDispInfo.uiDisplayH);
}

void PB_SetExpectJPEGSize(UINT32 uiCompressRatio, UINT32 uiUpBoundRatio, UINT32 uiLowBoundRatio, UINT32 uiLimitCnt)
{
	g_stPBBRCInfo.uiCompressRatio = uiCompressRatio;
	g_stPBBRCInfo.uiUpBoundRatio  = uiUpBoundRatio;
	g_stPBBRCInfo.uiLowBoundRatio = uiLowBoundRatio;
	g_stPBBRCInfo.uiReEncCnt      = uiLimitCnt;
}

void PB_ConfigVdoWIN(PB_DISP_IDX DispIdx, PURECT pRect)
{
	if (PBDISP_IDX_SEC == DispIdx) {
		memcpy(&g_PBVdoWIN[PBDISP_IDX_SEC], pRect, sizeof(URECT));
	} else {
		memcpy(&g_PBVdoWIN[PBDISP_IDX_PRI], pRect, sizeof(URECT));
	}
}

