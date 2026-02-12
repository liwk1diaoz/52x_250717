
#ifdef __ECOS
#include "ecosmain.h"
#endif


//global debug level: PRJ_DBG_LVL
#include "PrjInc.h"
#include "UIWnd/UIFlow.h"
#include "kwrap/cmdsys.h"
#include "kwrap/semaphore.h"

#if (UPDFW_MODE == ENABLE)
#include "Mode/UIModeUpdFw.h"
#endif
//local debug level: THIS_DBGLVL
#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          SysMain
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

extern void System_BootStart(void);
extern void System_BootEnd(void);
extern void System_ShutDownStart(void);
extern void System_ShutDownEnd(void);

BOOL g_bIsInitSystemFinish = TRUE;

int SX_TIMER_BURNIN_AUTORUN_ID = -1;

//////////////////////////////////////
UINT32 UI_EventTraceLvl = 0;

int UserMainProc_Init(void);
int UserMainProc_Exit(void);

void UserWaitEvent(NVTEVT wait_evt, UINT32 *wait_paramNum, UINT32 *wait_paramArray)
{
	NVTEVT evt = 0; //fix for CID 45083
	UINT32 paramNum = 0; //fix for CID 45084
	UINT32 paramArray[MAX_MESSAGE_PARAM_NUM] = {0}; //fix for CID 45085
	BOOL bQuit = FALSE;

	while (!bQuit) {
		PROFILE_TASK_IDLE();
		Ux_WaitEvent(&evt, &paramNum, paramArray);
		if (evt) {
			INT32 result = NVTEVT_PASS;
#if (UCTRL_FUNC)
			BOOL  isUctrlEvent = FALSE;
#endif
			PROFILE_TASK_BUSY();
			if (UI_EventTraceLvl >= 5) {
				DBG_DUMP("MSG: get event 0x%08x!\r\n", evt);
			}

#if (UCTRL_FUNC)
			if (evt & UCTRL_EVENT_MASK) {
				isUctrlEvent = TRUE;
				evt &= ~UCTRL_EVENT_MASK;
			}
#endif

			if (evt == wait_evt) {
				if (wait_paramNum) {
					(*wait_paramNum) = paramNum;
				}
				if (wait_paramArray) {
					memcpy(wait_paramArray, paramArray, sizeof(UINT32)*MAX_MESSAGE_PARAM_NUM);
				}
				result = NVTEVT_CONSUME;
				bQuit = TRUE;
			}
			if (IS_APP_EVENT(evt)) {
				result = Ux_AppDispatchMessage(evt, paramNum, paramArray);
			}
			/*
			{
			VControl* pObj;
			if(Ux_GetRootWindow(&pObj) != NVTRET_OK)
			    continue;
			}
			*/
#if(UI_FUNC==ENABLE)
			if (result != NVTEVT_CONSUME) {
				if (System_GetState(SYS_STATE_CURRMODE) != SYS_MODE_UNKNOWN) {
					result = Ux_WndDispatchMessage(evt, paramNum, paramArray);
				}
			}
#endif
			if ((UI_EventTraceLvl > 0) && (result != NVTEVT_CONSUME)) {
				DBG_DUMP("^YWRN: MSG: skip event 0x%08x!\r\n", evt);
			}

#if(UI_FUNC==ENABLE)
			if (System_GetState(SYS_STATE_CURRMODE) != SYS_MODE_UNKNOWN) {
				Ux_Redraw();
			}
#endif

#if (UCTRL_FUNC)
			// run uctrl callback
			if (isUctrlEvent) {
				UctrlMain_EventEnd(0, evt);
			}
#endif
		}
	}
}

extern void System_InstallAppObj(void);

BOOL bUI_Quit = FALSE;
void UI_Quit(void)
{
	bUI_Quit = TRUE;
}

/////////////////////////////////////////////////////////////////////////////

void System_InstallModeObj(void)
{
	//register project mode
#if (MAIN_MODE == ENABLE)
	PRIMARY_MODE_MAIN = System_AddMode(&gModeMain); //an empty mode for calibration
#endif
#if (PLAY_MODE == ENABLE)
	PRIMARY_MODE_PLAYBACK = System_AddMode(&gModePlay);
#endif
#if (PHOTO_MODE == ENABLE)
	PRIMARY_MODE_PHOTO = System_AddMode(&gModePhoto);
#endif
#if (MOVIE_MODE == ENABLE)
	PRIMARY_MODE_MOVIE = System_AddMode(&gModeMovie);
#endif
#if (USB_MODE == ENABLE)
	PRIMARY_MODE_USBMSDC = System_AddMode(&gModeUsbDisk);
	PRIMARY_MODE_USBMENU = System_AddMode(&gModeUsbMenu);
	PRIMARY_MODE_USBPCC = System_AddMode(&gModeUsbCam);
#endif
#if (SLEEP_MODE == ENABLE)
	PRIMARY_MODE_SLEEP = System_AddMode(&gModeSleep);
#endif
#if (IPCAM_MODE == ENABLE)
	PRIMARY_MODE_IPCAM = System_AddMode(&gModeIPCam);
#endif
#if (UPDFW_MODE == ENABLE)
	PRIMARY_MODE_UPDFW = System_AddMode(&gModeUpdFw);
	SYS_SUBMODE_UPDFW = System_AddSubMode(&gSubModeUpdFw);
#endif
#if(WIFI_FUNC==ENABLE)
	SYS_SUBMODE_WIFI = System_AddSubMode(&gSubModeWiFi);
#endif
#if (UVC_MULTIMEDIA_FUNC == ENABLE)
	SYS_SUBMODE_UVC = System_AddSubMode(&gSubModeUvc);
#endif
}

/////////////////////////////////////////////////////////////////////////////
INT32 System_UserKeyFilter(NVTEVT evt, UINT32 paramNum, UINT32 *paramArray);
INT32 System_UserTouchFilter(NVTEVT evt, UINT32 paramNum, UINT32 *paramArray);
#if (USB_MODE == ENABLE)
BOOL gIsUSBChargePreCheck = FALSE;
#endif
UINT32 bCalConsumeFps =FALSE;
UINT32 ConsumeEvtCnt =0;
UINT32 EvtSum=0;
UINT32 ConsumeFps =0;
UINT32 ConsumeUnit = 40;

#if defined(_UI_STYLE_LVGL_)

/**********************
*  macro
**********************/

#if defined(__FREERTOS)
	#define LV_USER_GET_TID		vos_task_get_tid 	
#else
	#include "pthread.h"
	#define LV_USER_GET_TID		pthread_self
#endif


/**********************
*  include headers
**********************/
#include "lvgl/lvgl.h"
#include "Resource/Plugin/lv_plugin_common.h"

/**********************
*  typedef and define
**********************/
typedef struct {

	NVT_KEY_EVENT nvt_key;
	LV_USER_KEY lv_user_key;

} LV_USER_KEYMAP;

#define LV_USER_KEYMAP_ENTRY_BEGIN(NAME) \
	static LV_USER_KEYMAP NAME[] = {

#define LV_USER_KEYMAP_ADD(NVT_KEY, LV_USER_KEY) \
		{NVT_KEY, LV_USER_KEY},

#define LV_USER_KEYMAP_ENTRY_END() \
		};


#define LV_USER_FLUSH_EVENT_THRESHOLD	(0.5)	/* percentage, between 0-1, flush NVTEVT_UPDATE_LVGL to prevent event queue full */
#define LV_USER_STOP_POST_THRESHOLD		(0.8)	/* percentage, between 0-1, stop post NVTEVT_UPDATE_LVGL if event queue close to full */


/**********************
*  keymap entry
**********************/
LV_USER_KEYMAP_ENTRY_BEGIN(lv_user_keymap_entry)

/* Enter key */
LV_USER_KEYMAP_ADD(NVTEVT_KEY_ENTER, 		LV_KEY_ENTER)
LV_USER_KEYMAP_ADD(NVTEVT_KEY_SELECT, 		LV_KEY_ENTER)

LV_USER_KEYMAP_ADD(NVTEVT_KEY_NEXT, 		LV_USER_KEY_NEXT)
LV_USER_KEYMAP_ADD(NVTEVT_KEY_PREV, 		LV_USER_KEY_PREV)
LV_USER_KEYMAP_ADD(NVTEVT_KEY_PREV, 		LV_USER_KEY_PREV)
LV_USER_KEYMAP_ADD(NVTEVT_KEY_LEFT, 		LV_KEY_LEFT)
LV_USER_KEYMAP_ADD(NVTEVT_KEY_RIGHT, 		LV_KEY_RIGHT)
LV_USER_KEYMAP_ADD(NVTEVT_KEY_UP, 			LV_KEY_UP)
LV_USER_KEYMAP_ADD(NVTEVT_KEY_DOWN, 		LV_KEY_DOWN)
LV_USER_KEYMAP_ADD(NVTEVT_KEY_SHUTTER1, 	LV_USER_KEY_SHUTTER1)
LV_USER_KEYMAP_ADD(NVTEVT_KEY_SHUTTER2, 	LV_USER_KEY_SHUTTER2)
LV_USER_KEYMAP_ADD(NVTEVT_KEY_ZOOMIN, 		LV_USER_KEY_ZOOMIN)
LV_USER_KEYMAP_ADD(NVTEVT_KEY_ZOOMOUT, 		LV_USER_KEY_ZOOMOUT)
LV_USER_KEYMAP_ADD(NVTEVT_KEY_MENU, 		LV_USER_KEY_MENU)
LV_USER_KEYMAP_ADD(NVTEVT_KEY_ZOOMOUT, 		LV_USER_KEY_ZOOMOUT)
LV_USER_KEYMAP_ADD(NVTEVT_KEY_MODE, 		LV_USER_KEY_MODE)
LV_USER_KEYMAP_ADD(NVTEVT_KEY_CALIBRATION, 	LV_USER_KEY_CALIBRATION)
LV_USER_KEYMAP_ENTRY_END()

/**********************
*  static variables
**********************/
static bool volatile g_keyboard_pressed = false;
static uint32_t volatile g_keyboard_value = 0;
static ID g_lv_task_handler_semid;
extern lv_indev_t* indev_keypad;
extern lv_indev_t* indev_pointer;
static int g_ui_task_id = -1;
extern int nvt_lvgl_init(void);
static BOOL is_fb_init = FALSE;
static bool volatile g_pointer_pressed = false;
static uint32_t g_pointer_x = 0, g_pointer_y = 0;

/**********************
*  debug functions
**********************/

#define DBG_TASK_HANDLER 0 /* dump task handler lock / unlock. CAUTION: it may dump a lot of messages */
#define DBG_EVENT_STATE 0  /* dump event state */

#if DBG_TASK_HANDLER
	#define DBG_DUMP_TASK_HANDLER(fmtstr, args...) DBG_DUMP(fmtstr, ##args)
#else
	#define DBG_DUMP_TASK_HANDLER(fmtstr, args...)
#endif

#if DBG_EVENT_STATE
	#define DBG_DUMP_EVENT_STATE(fmtstr, args...) DBG_DUMP(fmtstr, ##args)
#else
	#define DBG_DUMP_EVENT_STATE(fmtstr, args...)
#endif

/**********************
*  static functions
**********************/
static int  _lv_user_task_handler_lock(void);					/* lock/unlock function for UserMainProc */
static void  _lv_user_task_handler_unlock(void);				/* unlock function for UserMainProc */
static LV_USER_KEY lv_user_keymap_find(NVT_KEY_EVENT nvt_key);	/* convert NVT_KEY_EVENT to LV_USER_KEY */

void lv_user_update_pointer_state(
		uint32_t x,
		uint32_t y,
		bool pressed
)
{
	g_pointer_x = x;
	g_pointer_y = y;
	g_pointer_pressed = pressed;
}

static int  _lv_user_task_handler_lock(void)
{
	int r = 0;

	DBG_DUMP_TASK_HANDLER("[tid:%d] lock lv_task_handler\r\n", g_ui_task_id);

	r = vos_sem_wait(g_lv_task_handler_semid);

	if(r != E_OK)
	{
		DBG_DUMP("[tid:%d] lock lv_task_handler failed!!\r\n", g_ui_task_id);
	}

	return r;
}

static void  _lv_user_task_handler_unlock(void)
{
	DBG_DUMP_TASK_HANDLER("[tid:%d] unlock lv_task_handler\r\n",  g_ui_task_id);

	vos_sem_sig(g_lv_task_handler_semid);
}


/* lock function API, should not be called in UserMainProc */
int lv_user_task_handler_lock(void)
{
	int r = 0;
	int task_id = LV_USER_GET_TID();

	if(task_id == g_ui_task_id)
		return 0;

	DBG_DUMP_TASK_HANDLER("[tid:%d] lock lv_task_handler\r\n", task_id);

	r = vos_sem_wait(g_lv_task_handler_semid);

	if(r != E_OK)
	{
		DBG_DUMP("[tid:%d] lock lv_task_handler failed!!\r\n", task_id);
	}

	return r;
}

/* unlock function API, should not be called in UserMainProc */
void lv_user_task_handler_unlock(void)
{
	int task_id = LV_USER_GET_TID();

	if(task_id == g_ui_task_id)
		return;

	DBG_DUMP_TASK_HANDLER("[tid:%d] unlock lv_task_handler\r\n",  task_id);

	vos_sem_sig(g_lv_task_handler_semid);
}

int lv_user_task_handler_temp_release(int (*cb)(void))
{
	int ret;
	int task_id = LV_USER_GET_TID();

	if(task_id == g_ui_task_id){

		/* temp release lock to prevent deadlock */
		DBG_DUMP_TASK_HANDLER("release lv_task_handler lock\r\n");

		_lv_user_task_handler_unlock();
		ret = cb();

		/* resume lock */
		DBG_DUMP_TASK_HANDLER("resume lv_task_handler lock\r\n");
		_lv_user_task_handler_lock();
	}
	else{
		ret = cb();
	}

	return ret;
}


/* for LV_USER_EVENT_NVTMSG */
LV_USER_EVENT_NVTMSG_DATA* gen_nvtmsg_data(NVTEVT event, UINT32 paramNum, ...)
{
	va_list ap;
	static LV_USER_EVENT_NVTMSG_DATA data = { 0 };
	static UINT32 pUIParamArray[MAX_MESSAGE_PARAM_NUM];
	UINT32 i = 0;

	data.event = event;
	data.paramNum = paramNum;

	if (paramNum) {

		data.paramArray = pUIParamArray;

		if (paramNum > MAX_MESSAGE_PARAM_NUM) {
			DBG_ERR("parameter number exceeds %d. Only take %d\n\r", MAX_MESSAGE_PARAM_NUM, MAX_MESSAGE_PARAM_NUM);
			paramNum = MAX_MESSAGE_PARAM_NUM;
		}
		va_start(ap, paramNum);
		for (i = 0; i < paramNum; i++) {
			pUIParamArray[i] = va_arg(ap, UINT32);
		}
		va_end(ap);
	}
	else{
		data.paramArray = NULL;
	}

	return &data;
}


static LV_USER_KEY lv_user_keymap_find(NVT_KEY_EVENT nvt_key)
{
	static uint16_t size = sizeof(lv_user_keymap_entry) / sizeof(LV_USER_KEYMAP);

	for(int i=0 ; i<size ;i++ )
	{
		if(lv_user_keymap_entry[i].nvt_key == nvt_key){
			return lv_user_keymap_entry[i].lv_user_key;
		}
	}

	DBG_WRN("can't find value maps to NVT_KEY_EVENT(%lx)\r\n", nvt_key);

	return LV_USER_KEY_UNKNOWN;
}

/**********************
*  extern functions
**********************/

void lvglTimer(void)
{
    if(!bUI_Quit){

    	static UINT32 max = 0;
    	static bool has_printed_flush_event_msg = false;
    	static bool has_printed_stop_post_msg = false;
    	float rate;
    	UINT32 used;


    	if(!max){
    		Ux_Get(UX_EVENT_QUEUE_MAX,&max);
    		DBG_DUMP("UX_EVENT_QUEUE_MAX = %lu\r\n", max);
    	}

    	Ux_Get(UX_EVENT_QUEUE_USED,&used);

    	rate = ((float)used / (float)max);

    	if(rate >= LV_USER_FLUSH_EVENT_THRESHOLD){

    		if(!has_printed_flush_event_msg){

				DBG_WRN("event queue(used:%lu max:%lu rate:%.2f) >= flush threshold(rate:%.2f), flush lvgl update event\r\n",
						used, max,
						rate, LV_USER_FLUSH_EVENT_THRESHOLD);

				has_printed_flush_event_msg = true;
    		}

    		Ux_FlushEventByRange(NVTEVT_UPDATE_LVGL,NVTEVT_UPDATE_LVGL);
    	}
    	else{
    		has_printed_flush_event_msg = false;
    	}


		if(rate >= LV_USER_STOP_POST_THRESHOLD){

			if(!has_printed_stop_post_msg){

				DBG_WRN("event queue(used:%lu max:%lu rate:%.2f) >= stop post threshold(rate:%.2f), stop post lvgl update event\r\n",
						used, max,
						rate, LV_USER_STOP_POST_THRESHOLD);

				has_printed_stop_post_msg = true;
			}
		}
		else{
			has_printed_stop_post_msg = false;
			Ux_PostEvent(NVTEVT_UPDATE_LVGL, 0, 0);
		}
    }
}

bool keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{


    data->state = (lv_indev_state_t)(
        g_keyboard_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL);

    uint32_t act_key = g_keyboard_value;

    if(act_key == 0)
    	return false;

	/*Translate the keys to LVGL control characters according to your key definitions*/
    act_key = lv_user_keymap_find(act_key);

    if(act_key == LV_USER_KEY_UNKNOWN)
    	act_key =0;

    data->key = act_key;
    /*Return `false` because we are not buffering and no more data to read*/
    return false;

}

bool pointer_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{

    data->state = (lv_indev_state_t)(
    		g_pointer_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL);


    data->point.x = g_pointer_x;
    data->point.y = g_pointer_y;

    /*Return `false` because we are not buffering and no more data to read*/
    return false;
}

void UserMainProc(void)
{
	TM_BOOT_BEGIN("flow", "preboot");
	//////////////////////////////////////////////////////////////

	UserMainProc_Init();

	if(is_fb_init == FALSE){
		nvt_lvgl_init();
		UIFlowLVGL();
		is_fb_init = TRUE;
	}

	//////////////////////////////////////////////////////////////
#if(UCTRL_FUNC == ENABLE)
	System_OnUctrl();
#endif

	System_InstallAppObj(); //install VControl type list of App Object and UIControl Object
	System_InstallModeObj(); //install SYS_MODE objects
	TM_BOOT_END("flow", "preboot");

	Ux_SendEvent(0, NVTEVT_SYSTEM_BOOT, 1, 1);
	NVTEVT evt = 0;
	UINT32 paramNum = 0; //fix for CID 45082
	UINT32 paramArray[MAX_MESSAGE_PARAM_NUM] = {0};
	VOS_TICK ttick=0;
    INT32 result = NVTEVT_PASS;

    g_ui_task_id = LV_USER_GET_TID();

	if ((vos_sem_create(&g_lv_task_handler_semid, 1, "LV_TSK_SEM")) != E_OK) {
		DBG_ERR("Create LV_TSK_SEM fail\r\n");
		goto exit;
	}

	/* keypad is event driven */
	lv_task_set_prio(indev_keypad->driver.read_task, LV_TASK_PRIO_OFF);

	while(!bUI_Quit) {

		Ux_WaitEvent(&evt, &paramNum, paramArray);

		if(!evt) {
			continue;
		} else {

			if (IS_KEY_EVENT(evt)) {
                //for sleep mode,filter key and any key wake up
				result = System_UserKeyFilter(evt, paramNum, paramArray);
    	        if(result != NVTEVT_CONSUME) {

    				vos_perf_mark(&ttick);

    				if(paramNum >= 1){

						DBG_DUMP_EVENT_STATE("key event[0x%08x] time[%d] paramNum[%u] key state[0x%08x]\r\n",
								evt,
								ttick/1000,
								paramNum,
								paramArray[0]);
    				}
					else{
						DBG_WRN("received key event(%lx) without key state\r\n", evt);
					}

    				g_keyboard_value = evt;

    				if((paramArray[0] > NVTEVT_KEY_PRESS_START && paramArray[0] < NVTEVT_KEY_PRESS_END) ||
    				   (paramArray[0] > NVTEVT_KEY_CONTINUE_START && paramArray[0] < NVTEVT_KEY_CONTINUE_END)
    				){
    					g_keyboard_pressed = true;
    				}
    				else{
    					g_keyboard_pressed = false;
    				}
                    //call input task immediately for read key state
                    if(indev_keypad){
                    	_lv_user_task_handler_lock();
                        indev_keypad->driver.read_task->task_cb(indev_keypad->driver.read_task);
                        _lv_user_task_handler_unlock();
                    }

                    /* workaround to extend key release event for lvgl */
                    if((paramArray[0] > NVTEVT_KEY_RELEASE_START) && (paramArray[0] < NVTEVT_KEY_RELEASE_END)){

                    	uint32_t nvt_user_key = lv_user_keymap_find(evt);

                    	if(nvt_user_key != LV_USER_KEY_UNKNOWN){
                    		_lv_user_task_handler_lock();
                    		lv_event_send(lv_plugin_scr_act(), LV_USER_EVENT_KEY_RELEASE, &nvt_user_key);
                    		_lv_user_task_handler_unlock();
                    	}
                    	else{
                    		DBG_WRN("evt = %lx\r\n", evt);
                    	}
                    }

    	        }
            }
			else if(evt==NVTEVT_UPDATE_LVGL){
                //for lvlg lv_task_handler,needless send to user

	        }
			else {

	        	if (IS_APP_EVENT(evt)){
	        		DBG_DUMP_EVENT_STATE("app event[0x%08x] time[%d] paramNum[%u]\r\n", evt, ttick/1000, paramNum);
	        		Ux_AppDispatchMessage(evt, paramNum, paramArray);
	        	}
	        	else{
	        		DBG_DUMP_EVENT_STATE("nvtmsg event[0x%08x] time[%d] paramNum[%u]\r\n", evt, ttick/1000, paramNum);
	        	}


	        	lv_obj_t *scr = lv_plugin_scr_act();

	        	if(NULL == scr){
	        		DBG_WRN("no active screen, drop nvtmsg event[0x%08x]\r\n", evt);
	        		continue;
	        	}

	        	LV_USER_EVENT_NVTMSG_DATA data;

	        	data.event = evt;
	        	data.paramNum = paramNum;
	        	data.paramArray = paramArray;

	        	_lv_user_task_handler_lock();
	        	lv_event_send(scr, LV_USER_EVENT_NVTMSG, &data);
	        	_lv_user_task_handler_unlock();
	        }
		}


		/* critical section lock */
		_lv_user_task_handler_lock();

		/* lvgl main function */
        lv_task_handler();

        /* critical section unlock */
        _lv_user_task_handler_unlock();
	}

	vos_sem_destroy(g_lv_task_handler_semid);


exit:
	//////////////////////////////////////////////////////////////
	UserMainProc_Exit();
	//////////////////////////////////////////////////////////////
	System_PowerOffStart();

}

#if (LV_TICK_CUSTOM == 1)
unsigned int custom_tick_get(void)
{
    VOS_TICK ttick1 = 0;
    static VOS_TICK ttick2 =0;
    static const unsigned int us_overflow_value = UINT32_MAX/1000;
    static unsigned int us_overflow_cnt = 0;

    vos_perf_mark(&ttick1);

    /* us overflow */
    if(ttick1 < ttick2){
    	us_overflow_cnt++;
    }

    ttick2 = ttick1;


    return ((ttick1/1000) + us_overflow_cnt*us_overflow_value);
}
#endif

#else

void UserMainProc(void)
{
	NVTEVT evt = 0; //fix for CID 45081
	UINT32 paramNum = 0; //fix for CID 45082
	UINT32 paramArray[MAX_MESSAGE_PARAM_NUM] = {0};
	//debug_msg("event loop - begin!\r\n");

	TM_BOOT_BEGIN("flow", "preboot");
	//////////////////////////////////////////////////////////////

	UserMainProc_Init();
	//////////////////////////////////////////////////////////////
#if(UCTRL_FUNC == ENABLE)
	System_OnUctrl();
#endif

	System_InstallAppObj(); //install VControl type list of App Object and UIControl Object
	System_InstallModeObj(); //install SYS_MODE objects
	TM_BOOT_END("flow", "preboot");

	Ux_SendEvent(0, NVTEVT_SYSTEM_BOOT, 1, 1);


#if (POWERON_FAST_BOOT == ENABLE)
	System_Debug_Msg(TRUE);
#endif

#if 0
#if (_BOARD_DRAM_SIZE_ > 0x04000000)
#ifdef __ECOS
	ecos_main();
#endif
#endif
#endif
	while (!bUI_Quit) {
		PROFILE_TASK_IDLE();
		Ux_WaitEvent(&evt, &paramNum, paramArray);
		if (evt) {
			INT32 result = NVTEVT_PASS;
			UINT32 EvtBegin=0,EvtEnd=0;
#if (UCTRL_FUNC)
			BOOL  isUctrlEvent = FALSE;
#endif
			PROFILE_TASK_BUSY();
			if (UI_EventTraceLvl >= 5) {
				DBG_DUMP("MSG: get event 0x%08x!\r\n", evt);
			}

			if(bCalConsumeFps) {
				//EvtBegin = Perf_GetCurrent();
				vos_perf_mark(&EvtBegin);
				ConsumeEvtCnt++;
			}
#if (UCTRL_FUNC)
			if (evt & UCTRL_EVENT_MASK) {
				isUctrlEvent = TRUE;
				evt &= ~UCTRL_EVENT_MASK;
			}
#endif
#if INPUT_FUNC
			if (IS_KEY_EVENT(evt)) {
				result = System_UserKeyFilter(evt, paramNum, paramArray);
			}
#endif
#if defined(_TOUCH_ON_)
			if (result != NVTEVT_CONSUME) {
				if (IS_TOUCH_EVENT(evt)) {
					result = System_UserTouchFilter(evt, paramNum, paramArray);
				}
			}
#endif

			if (result != NVTEVT_CONSUME) {
				if (IS_APP_EVENT(evt)) {
					result = Ux_AppDispatchMessage(evt, paramNum, paramArray);
				}
			}
			if (result != NVTEVT_CONSUME) {
				if (System_GetState(SYS_STATE_CURRMODE) != SYS_MODE_UNKNOWN) {
					result = Ux_WndDispatchMessage(evt, paramNum, paramArray);
				}
			}
			if ((UI_EventTraceLvl > 0) && (result != NVTEVT_CONSUME)) {
				DBG_DUMP("^YWRN: MSG: skip event 0x%08x!\r\n", evt);
			}

			if (System_GetState(SYS_STATE_CURRMODE) != SYS_MODE_UNKNOWN) {
				Ux_Redraw();
			}
#if 0 //background task wait done flag,need to remove
			// A special case.
			// UI background is busy until finishing the event handler of BACKGROUND_DONE
			if (evt == NVTEVT_BACKGROUND_DONE) {
				BKG_Done();
			}
#endif
#if (UCTRL_FUNC)
			// run uctrl callback
			if (isUctrlEvent) {
				UctrlMain_EventEnd(0, evt);
			}
#endif


			if(bCalConsumeFps) {
				//EvtEnd = Perf_GetCurrent();
				vos_perf_mark(&EvtEnd);

				EvtSum += (EvtEnd-EvtBegin);
				if(ConsumeEvtCnt%ConsumeUnit==0){
					ConsumeFps = ConsumeEvtCnt*1000000/EvtSum;
					DBG_DUMP("^Y1 evt cost %d us (get evt fps %d)\r\n",EvtSum/ConsumeEvtCnt,ConsumeFps);
					//reset count,calculate next 40 event
					EvtSum=0;
					ConsumeEvtCnt=0;
				}
			}
			//return result;
		}
	}

	//////////////////////////////////////////////////////////////
	UserMainProc_Exit();
	//////////////////////////////////////////////////////////////
	System_PowerOffStart();
	//debug_msg("event loop - end!\r\n");
}
#endif
#if 0  //LOGFILE_FUNC TODO
extern void LogFile_DumpMemAndSwReset(void);
#endif //LOGFILE_FUNC TODO
///< error status callback
void UserErrCb(UXUSER_CB_TYPE evt, UINT32 p1, UINT32 p2, UINT32 p3)
{
	static BOOL g_has_dump = FALSE;
	if (g_has_dump == FALSE) {
		g_has_dump = TRUE;
		Ux_DumpEvents();
        #if 1//_TODO
        nvt_cmdsys_runcmd("ker dump");
    	nvt_cmdsys_runcmd("usage cpucfg 500");
    	nvt_cmdsys_runcmd("usage cpustart");
        Delay_DelayMs(5000);
    	nvt_cmdsys_runcmd("usage cpustop");
        #endif
	}
	#if (LOGFILE_FUNC==ENABLE)
	#if 0  //LOGFILE_FUNC TODO
	LogFile_DumpMemAndSwReset();
	#endif //LOGFILE_FUNC TODO
	#endif
}

void UserCalConsumeFps(UINT32 bEn,UINT32 unit)
{
	if(bEn){
		//reset count,calculate next 40 event
		EvtSum=0;
		ConsumeEvtCnt=0;
		if(unit)
			ConsumeUnit = unit;
	}
	bCalConsumeFps = bEn;

	DBG_DUMP("bCalConsumeFps %d ConsumeUnit %d\r\n",bCalConsumeFps,ConsumeUnit);
}


//////////////////////////////////////////////////////////////

#if (POWERON_FAST_BOOT == ENABLE)

// Services
extern void UserMainProc2(void);
extern void UserMainProc3(void);
extern void UserMainProc4(void);
extern void UserMainProc5(void);
extern void UserMainProc6(void);
extern void UserMainProc7(void);
extern void UserMainProc8(void);
UINT32 USERINIT_FLG_ID = 0;
UINT32 USEREXIT_FLG_ID = 0;
UINT32 USERPROC2_ID = 0;
UINT32 USERPROC3_ID = 0;
UINT32 USERPROC4_ID = 0;
UINT32 USERPROC5_ID = 0;
UINT32 USERPROC6_ID = 0;
UINT32 USERPROC7_ID = 0;
UINT32 USERPROC8_ID = 0;
//#define PRI_NVTUSER                 10
//#define PRI_FILESYS                 10
//#define PRI_FSBACKGROUND            9
#define PRI_USERPROC2               12 //ensure its priority is lower then PRI_NVTUSER
#define PRI_USERPROC3               12 //ensure its priority is lower then PRI_NVTUSER
#define PRI_USERPROC4               12 //ensure its priority is lower then PRI_NVTUSER
#define PRI_USERPROC5               12 //ensure its priority is lower then PRI_NVTUSER
#define PRI_USERPROC6               12 //ensure its priority is lower then PRI_NVTUSER
#define PRI_USERPROC7               12 //ensure its priority is lower then PRI_NVTUSER
#define PRI_USERPROC8               12 //ensure its priority is higher then PRI_NVTUSER (for network IPC)
#define STKSIZE_USERPROC2           2048
#define STKSIZE_USERPROC3           2048
#define STKSIZE_USERPROC4           2048
#define STKSIZE_USERPROC5           2048
#define STKSIZE_USERPROC6           2048
#define STKSIZE_USERPROC7           2048
#define STKSIZE_USERPROC8           2048
#endif

void UserMainProc_InstallID(void)
{
#if (POWERON_FAST_BOOT == ENABLE)
	OS_CONFIG_FLAG(USERINIT_FLG_ID);
	OS_CONFIG_FLAG(USEREXIT_FLG_ID);
	OS_CONFIG_TASK(USERPROC2_ID,     PRI_USERPROC2,       STKSIZE_USERPROC2,        UserMainProc2);
	OS_CONFIG_TASK(USERPROC3_ID,     PRI_USERPROC3,       STKSIZE_USERPROC3,        UserMainProc3);
	OS_CONFIG_TASK(USERPROC4_ID,     PRI_USERPROC4,       STKSIZE_USERPROC4,        UserMainProc4);
	OS_CONFIG_TASK(USERPROC5_ID,     PRI_USERPROC5,       STKSIZE_USERPROC5,        UserMainProc5);
	OS_CONFIG_TASK(USERPROC6_ID,     PRI_USERPROC6,       STKSIZE_USERPROC6,        UserMainProc6);
	OS_CONFIG_TASK(USERPROC7_ID,     PRI_USERPROC7,       STKSIZE_USERPROC7,        UserMainProc7);
	OS_CONFIG_TASK(USERPROC8_ID,     PRI_USERPROC8,       STKSIZE_USERPROC8,        UserMainProc8);
#endif
}

BOOL bSystemFlow = 1;

int UserMainProc_Init(void)
{
	System_BootStart();
	bSystemFlow = 1;
#if (POWERON_FAST_BOOT == ENABLE)
	INIT_CLRFLAG(FLGINIT_MASKALL);

	INIT_SETFLAG(FLGINIT_BEGIN); //set by itself
	sta_tsk(USERPROC2_ID, 0);
	sta_tsk(USERPROC3_ID, 0);
	sta_tsk(USERPROC4_ID, 0);
	sta_tsk(USERPROC5_ID, 0);
	sta_tsk(USERPROC6_ID, 0);
	sta_tsk(USERPROC7_ID, 0);
	sta_tsk(USERPROC8_ID, 0);
#endif
	SystemInit();

#if (POWERON_FAST_BOOT == ENABLE)
	INIT_SETFLAG(FLGINIT_END);
#endif
	System_BootEnd();
	return 0;
}

int UserMainProc_Exit(void)
{
	System_ShutDownStart();
	bSystemFlow = 0;
#if (POWERON_FAST_BOOT == ENABLE)
	EXIT_CLRFLAG(FLGEXIT_MASKALL);

	EXIT_SETFLAG(FLGEXIT_BEGIN); //set by itself
	sta_tsk(USERPROC2_ID, 0);
	sta_tsk(USERPROC3_ID, 0);
	sta_tsk(USERPROC4_ID, 0);
	sta_tsk(USERPROC5_ID, 0);
	sta_tsk(USERPROC6_ID, 0);
	sta_tsk(USERPROC7_ID, 0);
	sta_tsk(USERPROC8_ID, 0);
#endif
	SystemExit();

#if (POWERON_FAST_BOOT == ENABLE)
	EXIT_SETFLAG(FLGEXIT_END);
#endif
	System_ShutDownEnd();
	return 0;
}

#if (POWERON_FAST_BOOT == ENABLE)
void UserMainProc2(void)
{
	kent_tsk();
	if (bSystemFlow == 1) {
		INIT_WAITFLAG(FLGINIT_BEGIN2); //wait until trigger begin
		SystemInit2();
		INIT_SETFLAG(FLGINIT_END2);
	} else {
		EXIT_WAITFLAG(FLGEXIT_BEGIN2); //wait until trigger begin
		SystemExit2();
		EXIT_SETFLAG(FLGEXIT_END2);
	}
	ext_tsk();
}

void UserMainProc3(void)
{
	kent_tsk();
	if (bSystemFlow == 1) {
		INIT_WAITFLAG(FLGINIT_BEGIN3); //wait until trigger begin
		SystemInit3();
		INIT_SETFLAG(FLGINIT_END3);
	} else {
		EXIT_WAITFLAG(FLGEXIT_BEGIN3); //wait until trigger begin
		SystemExit3();
		EXIT_SETFLAG(FLGEXIT_END3);
	}
	ext_tsk();
}

void UserMainProc4(void)
{
	kent_tsk();
	if (bSystemFlow == 1) {
		INIT_WAITFLAG(FLGINIT_BEGIN4); //wait until trigger begin
		SystemInit4();
		INIT_SETFLAG(FLGINIT_END4);
	} else {
		EXIT_WAITFLAG(FLGEXIT_BEGIN4); //wait until trigger begin
		SystemExit4();
		EXIT_SETFLAG(FLGEXIT_END4);
	}
	ext_tsk();
}

void UserMainProc5(void)
{
	kent_tsk();
	if (bSystemFlow == 1) {
		INIT_WAITFLAG(FLGINIT_BEGIN5); //wait until trigger begin
		SystemInit5();
		INIT_SETFLAG(FLGINIT_END5);
	} else {
		EXIT_WAITFLAG(FLGEXIT_BEGIN5); //wait until trigger begin
		SystemExit5();
		EXIT_SETFLAG(FLGEXIT_END5);
	}
	ext_tsk();
}

void UserMainProc6(void)
{
	kent_tsk();
	if (bSystemFlow == 1) {
		INIT_WAITFLAG(FLGINIT_BEGIN6); //wait until trigger begin
		SystemInit6();
		INIT_SETFLAG(FLGINIT_END6);
	} else {
		EXIT_WAITFLAG(FLGEXIT_BEGIN6); //wait until trigger begin
		SystemExit6();
		EXIT_SETFLAG(FLGEXIT_END6);
	}
	ext_tsk();
}

void UserMainProc7(void)
{
	kent_tsk();
	if (bSystemFlow == 1) {
		INIT_WAITFLAG(FLGINIT_BEGIN7); //wait until trigger begin
		SystemInit7();
		INIT_SETFLAG(FLGINIT_END7);
	} else {
		EXIT_WAITFLAG(FLGEXIT_BEGIN7); //wait until trigger begin
		SystemExit7();
		EXIT_SETFLAG(FLGEXIT_END7);
	}
	ext_tsk();
}

void UserMainProc8(void)
{
	kent_tsk();
	if (bSystemFlow == 1) {
		INIT_WAITFLAG(FLGINIT_BEGIN8); //wait until trigger begin
		SystemInit8();
		INIT_SETFLAG(FLGINIT_END8);
	} else {
		EXIT_WAITFLAG(FLGEXIT_BEGIN8); //wait until trigger begin
		SystemExit8();
		EXIT_SETFLAG(FLGEXIT_END8);
	}
	ext_tsk();
}

#endif

void System_WaitFS(void)
{
	static BOOL is_fs_ready = FALSE;
    if (!is_fs_ready) {
		TM_BOOT_BEGIN("Fs", "WaitAttach");
		#if (POWERON_FAST_BOOT == ENABLE)
		if (INIT_CHKFLAG(FLGINIT_STRGATH) == 0){
		#else
		// POWERON_WAIT_FS_READY set ENABLE, already wait on System_OnBoot()
		if (POWERON_WAIT_FS_READY == DISABLE){
		#endif
			DBG_IND("Wait FileSys b\r\n");
			UserWaitEvent(NVTEVT_STRG_ATTACH, NULL, NULL);
			DBG_DUMP("Wait FileSys e\r\n");
		}
		TM_BOOT_END("Fs", "WaitAttach");
		is_fs_ready = TRUE;
    }

}


