
#ifndef _UIFLOWLVGL_SPORTCAM_H
#define _UIFLOWLVGL_SPORTCAM_H

/**************************************************************************************
 * External Variables
 **************************************************************************************/

extern const PALETTE_ITEM gDemoKit_Palette_Palette[256];

/**************************************************************************************
 * Include headers
 **************************************************************************************/


#if(IPCAM_FUNC!= ENABLE)
#include "UIWnd/LVGL_SPORTCAM/UIInfo/UIInfo.h"
#include "UIWnd/LVGL_SPORTCAM/UIInfo/DateTimeInfo.h"
#endif

/* Resource */
#include "UIWnd/LVGL_SPORTCAM/Resource/UIResource.h"
#include "UIWnd/LVGL_SPORTCAM/Resource/BG_Opening.h"
#include "UIWnd/LVGL_SPORTCAM/Resource/SoundData.h"
#include "UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowWaitMoment/UIFlowWaitMomentAPI.h"
#include "UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowUSB/UIFlowWndUSBAPI.h"
#include "UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMovie/UIFlowMovieFuncs.h"
#include "UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowPlay/UIFlowPlayFuncs.h"
#include "UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowPhoto/UIFlowPhotoFuncs.h"
#include "UIWnd/LVGL_SPORTCAM/UIInfo/UIInfo.h"
#include "UIWnd/LVGL_SPORTCAM/UIInfo/UIPhotoInfo.h"
#include "UIWnd/LVGL_SPORTCAM/UIInfo/UIPhotoMapping.h"
#include "UIWnd/LVGL_SPORTCAM/UIInfo/UIMovieInfo.h"
#include "UIWnd/LVGL_SPORTCAM/UIInfo/UIMovieMapping.h"
#include "UIWnd/LVGL_SPORTCAM/UIInfo/DateTimeInfo.h"
#include "UIFlowLVGL/UIFlowLVGL.h"
#include "UIFlowLVGL/UIFlowMenuCommonItem/TabMenu.h"
#include "UIFlowLVGL/UIFlowMenuCommonItem/MenuId.h"
#include "UIFlowLVGL/UIFlowMenuCommonItem/MenuMovie.h"
#include "UIFlowLVGL/UIFlowMenuCommonItem/MenuMode.h"
#include "UIFlowLVGL/UIFlowMenuCommonItem/MenuSetup.h"
#include "UIFlowLVGL/UIFlowMenuCommonItem/MenuPhoto.h"
#include "UIFlowLVGL/UIFlowMenuCommonItem/MenuPlayback.h"
#include "UIFlowLVGL/UIFlowMenuCommonItem/MenuCommon.h"

/**************************************************************************************
 * Type Define
 **************************************************************************************/

/* nvt message */
typedef struct {
	NVTEVT event;
	UINT32* paramArray;
	UINT32 paramNum;
} LV_USER_EVENT_NVTMSG_DATA;


/* user defined event, must start from _LV_PLUGIN_EVENT_LAST */
typedef enum {

	LV_USER_EVENT_START = _LV_PLUGIN_EVENT_LAST,
	LV_USER_EVENT_NVTMSG = LV_USER_EVENT_START, 		/* lv_event_get_data(): LV_USER_EVENT_NVTMSG_DATA */
	LV_USER_EVENT_KEY_RELEASE,							/* lv_event_get_data():  */

} LV_USER_EVENT;

/* user defined key for mapping NVT_KEY_XXX */
typedef enum {

	LV_USER_KEY_DOWN = LV_KEY_DOWN,
	LV_USER_KEY_UP = LV_KEY_UP,

	LV_USER_KEY_DEF_START = 128,
	LV_USER_KEY_SHUTTER1,
	LV_USER_KEY_SHUTTER2,
	LV_USER_KEY_ZOOMIN,
	LV_USER_KEY_ZOOMOUT,
	LV_USER_KEY_PREV,
	LV_USER_KEY_NEXT,
	LV_USER_KEY_MENU,
	LV_USER_KEY_SELECT,
	LV_USER_KEY_MODE,
	LV_USER_KEY_CALIBRATION,
	LV_USER_KEY_DEF_END = 255,
	LV_USER_KEY_UNKNOWN = LV_USER_KEY_DEF_END,

} LV_USER_KEY;


/**************************************************************************************
 * Macros
 **************************************************************************************/

#define MAX_MESSAGE_PARAM_NUM      3
LV_USER_EVENT_NVTMSG_DATA* gen_nvtmsg_data(NVTEVT event, UINT32 paramNum, ...);


/**************************************************************************************
 * Global Functions
 **************************************************************************************/

int lv_user_task_handler_lock(void);
void lv_user_task_handler_unlock(void);
int lv_user_task_handler_temp_release(int (*cb)(void));

void lv_user_update_pointer_state(
		uint32_t x,
		uint32_t y,
		bool pressed
);

#endif
