#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/*
  Playback mode frame buffer switch

  frame buffer switch for PLAYSYS_COPY2TMPBUF/PLAYSYS_COPY2IMETMPBUF/PLAYSYS_COPY2FBBUF

  @param void
  @return void
*/
void PB_FrameBufSwitch(PB_DISPLAY_BUFF FBAddrMode)
{
#if 0
	switch (FBAddrMode) {
	case PLAYSYS_COPY2TMPBUF:
		GxDisp_SetData(GXDISP_VIDEO1, g_pPlayTmpImgBuf, REGION_MATCH_IMG, NULL);
		GxDisp_Load();
		break;

#if 0
	case PLAYSYS_COPY2IMETMPBUF:
		GxDisp_SetData(GXDISP_VIDEO1, g_pPlayIMETmpImgBuf, NULL);
		GxDisp_Load();
		break;
#endif

	default:// PLAYSYS_COPY2FBBUF
		GxDisp_SetData(GXDISP_VIDEO1, &gPlayFBImgBuf, REGION_MATCH_IMG, NULL);
		GxDisp_Load();
		break;
	}
#endif
}

/*
    Show specific file in video2

    @param[in] pVdoObj
    @return void
*/
//#NT#2012/04/16#Lincy Lin -begin
//#NT#
#if 0
void PB_ShowSpecFileInV2(PUT_VDO_BUFADDR pBufAddr, PUT_CONFIG_VDO pCfgVDO)
{
	UtIDE_ConfigVideoAttr(UT_VIDEO2, pCfgVDO);

	UtIDE_SwitchVideoBuf(UT_VIDEO2, pBufAddr);

	UtIDE_EnableVideo(UT_VIDEO2, ENABLE);
}
#endif
//#NT#2012/04/16#Lincy Lin -end

