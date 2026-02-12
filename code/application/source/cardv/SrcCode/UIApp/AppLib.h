#ifndef _APPLIB_H_
#define _APPLIB_H_


/////////////////////////////////////////
// AppLib
typedef enum {
	APP_BASE = APP_TYPE_MIN,
	APP_SETUP,
	APP_PLAY,
	APP_PHOTO,
	APP_MOVIEPLAY,
	APP_MOVIEREC,
	APP_PRINT,
	APP_PCC,
	APP_MSDC,
	APP_USBCHARGE,
#if(WIFI_AP_FUNC==ENABLE)
	APP_WIFICMD,
#endif
	APP_TYPE_MAX
}
APP_TYPE;


#endif //_APPLIB_H_
