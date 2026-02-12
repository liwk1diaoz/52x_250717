#include "ImageApp/ImageApp_Photo.h"

///////////////////////////////////////////////////////////////////////////////
//#define __MODULE__          IA_Photo_Id
//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
//#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#include "DebugModule.h"
///////////////////////////////////////////////////////////////////////////////

#define PRI_PHOTO_DISP          10
#define PRI_PHOTO_WIFI          10
#define STKSIZE_PHOTO_DISP    3072
#define STKSIZE_PHOTO_WIFI    3072

UINT32 PHOTO_DISP_TSK_ID = 0;
UINT32 PHOTO_DISP_FLG_ID = 0;
UINT32 PHOTO_WIFI_TSK_ID = 0;
UINT32 PHOTO_WIFI_FLG_ID = 0;


void ImageApp_Photo_InstallID(void)
{
	/*
	OS_CONFIG_TASK(PHOTO_DISP_TSK_ID, PRI_PHOTO_DISP, STKSIZE_PHOTO_DISP, PhotoDispTsk);
	OS_CONFIG_TASK(PHOTO_WIFI_TSK_ID, PRI_PHOTO_WIFI, STKSIZE_PHOTO_WIFI, PhotoWiFiTsk);
	OS_CONFIG_FLAG(PHOTO_DISP_FLG_ID);
	OS_CONFIG_FLAG(PHOTO_WIFI_FLG_ID);
	*/
}
