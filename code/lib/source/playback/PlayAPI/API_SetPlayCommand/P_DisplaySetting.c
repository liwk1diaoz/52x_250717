#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/**
    Set IME and IDE out width & height.

    @param[in] pResize
    @return void
*/
void PB_SetIMEIDESize(UINT32 uiDispW, UINT32 uiDispH)
{
	UINT32 SrcWidth = g_PBDispInfo.uiDisplayW, SrcHeight = g_PBDispInfo.uiDisplayH;
	UINT32 uiFBWidth, uiFBHeight;

	////////////////////////////////////////////////////////////////////////////
	// Alignment for IME Scaling function
	////////////////////////////////////////////////////////////////////////////
	uiFBWidth  = ALIGN_CEIL_16(uiDispW);
	uiFBHeight = ALIGN_CEIL_16(uiDispH);

	if (gucPlayTskWorking) {
		FLGPTN PlayCMD;

		// Wait playback not busy, do not clear
		wai_flg(&PlayCMD, FLG_PB, FLGPB_NOTBUSY, TWF_ORW/*|TWF_CLR*/);
	}

	// Copy ori-frame to new-define-frame
	PB_CopyImage2Display(PLAYSYS_COPY2FBBUF, PLAYSYS_COPY2TMPBUF);

	// Update g_PBObj info
	if ((uiFBWidth != SrcWidth) || (uiFBHeight != SrcHeight)) {
		// Re-alloc Playback memory
		PB_AllocPBMemory(uiFBWidth, uiFBHeight);
	}

}

void PB_ChangeDisplaySize(UINT32 uiDisp_w, UINT32 uiDisp_h)
{
	xPB_DispSrv_Init4PB();

	PB_SetIMEIDESize(uiDisp_w, uiDisp_h);

#if 0
	if (g_FlowPBThumbPhase == FLOWPB_THUMB_Phase1) {
		CoordinateTrans_TransferRects_Ex(g_FlowPBThumbRect1, g_FlowPBThumbRectBase1, sizeof(g_FlowPBThumbRectBase1), COORD_METHOD_ABSOULTE);
		g_pFlowPBThumbRect = g_FlowPBThumbRectBase1;
		gMenuPlayInfo.pRectDrawThumb = g_FlowPBThumbRect1;
	} else { // if(g_FlowPBThumbPhase == FLOWPB_THUMB_Phase2)
		CoordinateTrans_TransferRects_Ex(g_FlowPBThumbRect2, g_FlowPBThumbRectBase2, sizeof(g_FlowPBThumbRectBase2), COORD_METHOD_ABSOULTE);
		g_pFlowPBThumbRect = g_FlowPBThumbRectBase2;
		gMenuPlayInfo.pRectDrawThumb = g_FlowPBThumbRect2;
	}

	// Set the position and size of first video frame
	CoordinateTrans_TransferRects_Ex(&rcVidFrame, &g_rcVidFrameBase, sizeof(g_rcVidFrameBase), COORD_METHOD_INCREMENTAL);
	PB_Set1stVideoFrame(rcVidFrame.uiLeft, rcVidFrame.uiTop,
						rcVidFrame.uiWidth, rcVidFrame.uiHeight);

	//setting the magnified thumbnail image (Video2)
	if (gMenuPlayInfo.CurrentMode == PLAYMODE_THUMB) {
		UIPlay_UpdateThumbRect();
		UIThumb_DoMoveFlow(MOV_NO);
	} else if (PB_GetCurrZoomIndex() > 0) {
		UIZoom_ReDrawScaledImage();
	}
#endif
}

