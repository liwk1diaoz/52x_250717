#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

void PB_ReScaleForRotate2Display(void)
{
#if _TODO
	PVDO_FRAME pSrcImg;

	if (gMenuPlayInfo.ZoomIndex != 0) {
		// zoom mode before rotate
		if (gucZoomUpdateNewFB == FALSE) {
			return;
		}

		gucZoomUpdateNewFB = FALSE;
		pSrcImg = &gPlayFBImgBuf;
	} else {
		pSrcImg = g_pDecodedImgBuf;
	}

	gximg_scale_data_fine(pSrcImg, GXIMG_REGION_MATCH_IMG, g_pPlayTmpImgBuf, GXIMG_REGION_MATCH_IMG);
#endif	
}

