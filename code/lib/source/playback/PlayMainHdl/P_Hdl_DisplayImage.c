#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

void PB_Hdl_DisplayImage(UINT32 PlayCMD)
{
	switch (PlayCMD) {
	case PB_CMMD_SINGLE:
		//#NT#2012/05/17#Lincy Lin -begin
		//#NT#Porting disply Srv
#if PB_USE_DISP_SRV
		//#NT#2012/09/04#Ben Wang -begin
		//#NT#DispSrv might be used by MediaFramework to play movie, so we need to config again every time single view is performed.
		PB_DispSrv_Cfg();
		//#NT#2012/09/04#Ben Wang -end
		PB_DispSrv_SetDrawCb(PB_VIEW_STATE_SINGLE);
#endif
		//#NT#2012/05/17#Lincy Lin -end
		PB_SingleHandle();
		break;

	case PB_CMMD_BROWSER:
		//#NT#2012/05/17#Lincy Lin -begin
		//#NT#Porting disply Srv
#if PB_USE_DISP_SRV
		PB_DispSrv_SetDrawCb(PB_VIEW_STATE_THUMB);
#endif
		//#NT#2012/05/17#Lincy Lin -end
		PB_BrowserHandle();
		break;

	case PB_CMMD_ZOOM:
		//#NT#2012/05/17#Lincy Lin -begin
		//#NT#Porting disply Srv
#if PB_USE_DISP_SRV
		PB_DispSrv_SetDrawCb(PB_VIEW_STATE_SINGLE);
#endif
		//#NT#2012/05/17#Lincy Lin -end
		PB_ZoomHandle();
		break;

	case PB_CMMD_ROTATE_DISPLAY:
		//#NT#2012/05/17#Lincy Lin -begin
		//#NT#Porting disply Srv
#if PB_USE_DISP_SRV
		PB_DispSrv_SetDrawCb(PB_VIEW_STATE_ROTATE);
#endif
		//#NT#2012/05/17#Lincy Lin -end
		PB_RotateDisplayHandle(TRUE);
		PB_SetFinishFlag(DECODE_JPG_DONE);
		break;

	case PB_CMMD_DE_SPECFILE:
		PB_Hdl_PlaySpecFileHandle();
		break;

	case PB_CMMD_CUSTOMIZE_DISPLAY:
		PB_UserDispHandle();
		break;
	}
}