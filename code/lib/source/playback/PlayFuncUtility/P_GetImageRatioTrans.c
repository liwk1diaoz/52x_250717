#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

//#NT#2008/03/30#Scottie -begin
//#Modify the following threshold for APOLLO project..
#define PB_NORMALIZE_IMGRATIO_THRESHOLD     10//5
//#NT#2008/03/30#Scottie -end
/*
    Get ratio transform between image and display.
    This is internal API.

    @param[in] ImageWidth
    @param[in] ImageHeight
    @param[in] DisplayWidth
    @param[in] DisplayHeight
    @return result (PB_KEEPAR_NONE/PB_KEEPAR_PILLARBOXING/PB_KEEPAR_LETTERBOXING)
*///////////////////////////////////////////////////////////////////////////////////////
INT32 PB_GetImageRatioTrans(UINT32 ImageWidth, UINT32 ImageHeight, UINT32 DisplayWidth, UINT32 DisplayHeight)
{
	if ((g_PBSetting.EnableFlags & PB_ENABLE_KEEP_ASPECT_RATIO) != PB_ENABLE_KEEP_ASPECT_RATIO) {
		return PB_KEEPAR_NONE;
	}

	//#NT#2016/09/02#HM Tseng -begin
	//#NT#Check aspect ratio if ImageHeight > ImageWidth
	/*if (ImageWidth < ImageHeight)
	{
	    return PB_KEEPAR_NONE;
	}*/
	//#NT#2016/09/02#HM Tseng -end

	return PB_ChkImgAspectRatio(ImageWidth, ImageHeight, DisplayWidth, DisplayHeight);
}

INT32 PB_ChkImgAspectRatio(UINT32 ImageWidth, UINT32 ImageHeight, UINT32 PanelWidth, UINT32 PanelHeight)
{
	float ImageRatio, AspectRatio;

	if (ImageHeight == 0) {
		DBG_ERR("ImageHeight is 0\r\n");
		return PB_KEEPAR_NONE;
	}

	if (PanelHeight == 0) {
		DBG_ERR("PanelHeight is 0\r\n");
		return PB_KEEPAR_NONE;
	}

	ImageRatio = (float)ImageWidth / (float)ImageHeight;
	AspectRatio = (float)PanelWidth / (float)PanelHeight;
	ImageRatio = (INT32)(ImageRatio * 100);
	AspectRatio = (INT32)(AspectRatio * 100);

	if (ImageRatio > AspectRatio) {
		if ((ImageRatio - AspectRatio) < PB_NORMALIZE_IMGRATIO_THRESHOLD) {
			return PB_KEEPAR_NONE;
		} else {
			return PB_KEEPAR_LETTERBOXING;
		}
	} else if (ImageRatio < AspectRatio) {
		if ((AspectRatio - ImageRatio) < PB_NORMALIZE_IMGRATIO_THRESHOLD) {
			return PB_KEEPAR_NONE;
		} else {
			return PB_KEEPAR_PILLARBOXING;
		}
	} else {
		return PB_KEEPAR_NONE;
	}
}