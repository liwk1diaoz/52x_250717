/*
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       UIFlow.h
    @ingroup    mIPRJAPUIFlow

    @brief      UI Flow Functions
                This file is the user interface ( for interchange flow control).

    @note       Nothing.

    @date       2005/04/01
*/

#ifndef _UIFLOW_H
#define _UIFLOW_H
#include "PrjCfg.h"
#include "kwrap/error_no.h"


#include "NvtUser/NvtUser.h"
#include "NvtUser/NvtBack.h"
#include "UIControl/UIControlWnd.h"
#include "UIControl/UIControl.h"
#include "AppControl/AppControl.h"
#include "UIFrameworkExt.h"
#include "NVTUserCommand.h"
#include "UIApp/AppLib.h"
#include "GxTimer.h"

#include "Mode/UIMode.h"

#include "UIApp/UIAppCommon.h"
#include "UIApp/Setup/UISetup.h"
#include "UIApp/Photo/UIStorageCheck.h"

#if(WIFI_FUNC==ENABLE)
#include "WiFiIpc/nvtwifi.h"
#endif
//Mode
#if (MAIN_MODE == ENABLE)
#include "Mode/UIModeMain.h"
#endif
#if 1//(PHOTO_MODE == ENABLE)
#include "Mode/UIModePhoto.h"
//#include "UIWnd/SPORTCAM/UIInfo/UIPhotoInfo.h"
//#include "UIWnd/SPORTCAM/UIInfo/UIPhotoMapping.h"
#endif
#if (MOVIE_MODE == ENABLE)
#include "Mode/UIModeMovie.h"
//#include "UIWnd/SPORTCAM/UIInfo/UIMovieInfo.h"
//#include "UIWnd/SPORTCAM/UIInfo/UIMovieMapping.h"
#endif

#if 1//(PLAY_MODE == ENABLE)
#include "Mode/UIModePlayback.h"
#endif
#if (USB_MODE == ENABLE)
#include "Mode/UIModeUsbDisk.h"
#include "Mode/UIModeUsbPTP.h"
#include "Mode/UIModeUsbCam.h"
#include "Mode/UIModeUsbMenu.h"
#endif
#if (SLEEP_MODE == ENABLE)
#include "Mode/UIModeSleep.h"
#endif
#if (UPDFW_MODE == ENABLE)
#include "Mode/UIModeUpdFw.h"
#endif
#if(WIFI_FUNC==ENABLE)
#include "Mode/UIModeWifi.h"
#endif
#if (UCTRL_FUNC)
#include "Mode/UCtrlMain.h"
#endif
#if (IPCAM_MODE == ENABLE)
#include "Mode/UIModeIPCam.h"
#endif

#include "kwrap/task.h"

#define TIMER_HALF_SEC			500
#define TIMER_ONE_SEC			1000
#define TIMER_TWO_SEC			2000

// Hideo test: general macros
#define MAKE_WORD(l, h)     ((UINT16)(((UINT8)(l)) | ((UINT16)((UINT8)(h))) << 8))
#define MAKE_LONG(l, h)     ((UINT32)(((UINT16)(l)) | ((UINT32)((UINT16)(h))) << 16))
#define LO_LONG(x)          ((UINT32)(x))
#define HI_LONG(x)          ((UINT32)((x>>32) & ((1ULL<<32) - 1)))
#define LO_WORD(l)          ((UINT16)(l))
#define HI_WORD(l)          ((UINT16)(((UINT32)(l) >> 16) & 0xFFFF))
#define LO_BYTE(w)          ((UINT8)(w))
#define HI_BYTE(w)          ((UINT8)(((UINT16)(w) >> 8) & 0xFF))
#define BIT(b, x)           ( ((1 << (b)) & (x)) >> (b) )
#define BITS(s, e, x)       ( (((((1 << ((e)-(s) + 1)) - 1) << (s)) & (x)) >> (s)) )
#define MAKE_EVEN(x)        ((x) & ~1)
#define MAKE_QUAD(x)        ((x) & ~3)

#define LIMIT(var,min,max)  (var) = ((var) < (min)) ? (min) : \
									(((var) > (max)) ? (max) : (var))

#define LIMITR(var,min,max) (var) = ((var) > (max)) ? (min) : \
									(((var) < (min)) ? (max) : (var))

#define SWAP_BYTES(x)       (MAKE_WORD( HI_BYTE(x), LO_BYTE(x)))

// Swaps words and bytes.
// For example, SWAP_WORDS(0x33221100) = 0x00112233
#define SWAP_WORDS(x)       (MAKE_LONG( MAKE_WORD( HI_BYTE(HI_WORD(x)),   \
										LO_BYTE(HI_WORD(x))),  \
										MAKE_WORD( HI_BYTE(LO_WORD(x)),   \
												LO_BYTE(LO_WORD(x)))))

#define ELEMS_OF_ARRAY(x)   (sizeof(x) / sizeof(x[0]))

#define SxCmd_GetTempMem(size) SysMain_GetTempBuffer(size)
#define SxCmd_RelTempMem(addr) SysMain_RelTempBuffer(addr)


#if defined(_UI_STYLE_SPORTCAM_)
#include "UIWnd/SPORTCAM/UIFlow_SPORTSCAM.h"
#include "UIWnd/SPORTCAM/UIInfo/UICfgDefault.h"
#elif defined(_UI_STYLE_ALEXA_)
#include "UIWnd/ALEXA/UIFlow_ALEXA.h"
#elif defined(_UI_STYLE_SPORTCAM_TOUCH_)
#include "UIWnd/SPORTCAM_TOUCH/UIFlow_SPORTSCAM_TOUCH.h"
#elif defined(_UI_STYLE_LVGL_)
	#include "LVGL_SPORTCAM/UIFlowLVGL_SPORTCAM.h"
	#include "LVGL_SPORTCAM/UIInfo/UICfgDefault.h"
#else
	#error "Unknown UI Style, please check UI_Style in nvt-info.dtsi"
#endif


#endif
