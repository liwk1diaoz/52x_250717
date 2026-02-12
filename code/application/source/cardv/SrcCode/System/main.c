// libc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kwrap/type.h>
#include <kwrap/platform.h>
#include <kwrap/flag.h>
#include "UIWnd/UIFlow.h"
#include "System/SysMain.h"
#include "hdal.h"
#include "SxTimer/SxTimer.h"
#include "DrvExt.h"
#include "comm/hwclock.h"
#include "comm/timer.h"
#if (defined(_NVT_ETHREARCAM_RX_))
#include "UIApp/Network/EthCamAppNetwork.h"
#endif
#include <signal.h>

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          main
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

#define DEFAULT_YEAR 	2000
#define DEFAULT_MON 	1
#define DEFAULT_DAY 	1
#define DEFAULT_HOUR 	0
#define DEFAULT_MIN 	0
#define DEFAULT_SEC 	0

extern void *vdoout_get_buf(void);

#if defined(_UI_STYLE_LVGL_)

/******************************************************
*  include headers
*******************************************************/
#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"

/******************************************************
*  extern function declaration
*******************************************************/
extern bool keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
extern bool pointer_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
extern void lvglTimer(void);

/******************************************************
*  static variables
*******************************************************/
static lv_disp_buf_t disp_buf_2;


/******************************************************
*  global variables
*******************************************************/
lv_indev_t* indev_keypad = NULL;
lv_indev_t* indev_pointer = NULL;


int nvt_lvgl_init(void)
{
	/*-------------------------
	* Initialize your display
	* -----------------------*/

	/*LittlevGL init*/
	lv_init();

	/*Linux frame buffer device init*/
	fbdev_init();

	/*-----------------------------
	* Create a buffer for drawing
	*----------------------------*/
	/*Set a display buffer*/

#if LV_USE_GPU_NVT_DMA2D

	#if LV_USER_CFG_USE_TWO_BUFFER
		void *buf2_1 = (void *)((UINT32)fbp + (screensize * 2));
	#else
		void *buf2_1 = (void *)((UINT32)fbp + screensize);
	#endif

#else
	lv_color_t* buf2_1 = (lv_color_t*) malloc(LV_HOR_RES_MAX * LV_VER_RES_MAX * sizeof(lv_color_t));  /*A screen sized buffer*/
	LV_ASSERT_NULL(buf2_1);
	memset(buf2_1, 0, LV_HOR_RES_MAX * LV_VER_RES_MAX * sizeof(lv_color_t));

	#if LV_USER_CFG_USE_TWO_BUFFER
		lv_color_t* buf2_2 = (lv_color_t*) malloc(LV_HOR_RES_MAX * LV_VER_RES_MAX * sizeof(lv_color_t));  /*A screen sized buffer*/
		LV_ASSERT_NULL(buf2_2);
		memset(buf2_2, 0, LV_HOR_RES_MAX * LV_VER_RES_MAX * sizeof(lv_color_t));
	#endif
#endif

	lv_disp_buf_init(&disp_buf_2, buf2_1, 0, LV_HOR_RES_MAX * LV_VER_RES_MAX);   /*Initialize the display buffer*/

	/*-----------------------------------
	* Register the display in LVGL
	*----------------------------------*/
	/*Add a display the LittlevGL sing the frame buffer driver*/
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);

	//disp_drv.hor_res = LV_VER_RES_MAX;
	//disp_drv.ver_res = LV_HOR_RES_MAX;
	disp_drv.rotated = 0;

	/*Set a display buffer*/
	disp_drv.buffer = &disp_buf_2;

	/*Used to copy the buffer's content to the display*/
	disp_drv.flush_cb = disp_flush;

	disp_drv.rounder_cb = lv_user_rounder_callback;

#if LV_COLOR_DEPTH == 24
	disp_drv.set_px_cb = set_px_cb_8565;
#else
	disp_drv.set_px_cb = NULL;
#endif

	/*Finally register the driver*/
	lv_disp_t * disp;
	disp = lv_disp_drv_register(&disp_drv); /*It flushes the internal graphical buffer to the frame buffer*/

	lv_disp_drv_update(disp, &disp_drv);


    /*Register a keypad input device*/
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keypad_read;
    indev_keypad = lv_indev_drv_register(&indev_drv);



    /*Register a keypad input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = pointer_read;
    indev_pointer = lv_indev_drv_register(&indev_drv);

    return 0;
}
#endif

#if (USB_POWER_DOWN==ENABLE)
static void rtos_usb_power_disable(void)
{
	system("mem w 0xF06010D0 0x000000FF");
	system("mem w 0xF06010D4 0x00000034");
	system("mem w 0xF0601008 0x00000014");
	system("mem w 0xF0601000 0x00000088");
	system("mem w 0xF0601030 0x00000035");
	system("mem w 0xF060102C 0x000000FF");
	system("mem w 0xF0601028 0x00000020");
	system("mem w 0xF0601024 0x00000093");
}
#endif

//movie to other position
#if 0//(ETH_POWER_DOWN ==ENABLE)
static void rtos_eth_power_disable(void)
{
	system("mem w 0xF02B3AE8 0x01");
	system("mem w 0xF02B3AE0 0x11");
	system("mem w 0xF02B38DC 0x01");
	system("mem w 0xF02B389C 0x33");
	system("mem w 0xF02B38CC 0xA5");
	system("mem w 0xF02B388C 0x11");
	system("mem w 0xF02B38C8 0x01");
	system("mem w 0xF02B3888 0x08");
	system("mem w 0xF02B38C8 0x03");
	system("mem w 0xF02B3888 0x08");
	system("mem w 0xF02B38F8 0x61");
}
#endif

int nvt_user_init(void)
{
    UXUSER_OBJ uxuser_param={0};
    ER ret=0;

    //project init
    UIControl_InstallID();

    uxuser_param.pfMainProc = UserMainProc;
	uxuser_param.pfNvtUserCB = UserErrCb;
    ret = Ux_Open(&uxuser_param);
    if(ret!=0){
		DBG_ERR("Ux_Open fail%d\r\n",ret);
		return -1;
    }

	BKG_OBJ uiBackObj = {0};
	//Init UI background
	uiBackObj.pDefaultFuncTbl = 0; //gBackgroundExtFuncTable
	uiBackObj.done_event = NVTEVT_BACKGROUND_DONE;
	ret = BKG_Open(&uiBackObj);
    if(ret!=0){
		DBG_ERR("BKG_Open fail%d\r\n",ret);
		return -1;
    }


    /* move lvgl timer here due to post event must after timer initialized */
#if defined(_UI_STYLE_LVGL_)

    TIMER_ID id;
	if (timer_open((TIMER_ID *)&id, (DRV_CB)lvglTimer) != E_OK) {
		DBG_ERR("timer trig open failed!\r\n");
		return -1;
	}

	timer_cfg(id, LV_DISP_DEF_REFR_PERIOD * 1000, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);

#endif

    //This will affect the boot speed and move to other position.
 	#if 0//(ETH_POWER_DOWN==ENABLE)
	rtos_eth_power_disable();
	#endif

	#if (USB_POWER_DOWN==ENABLE)
	rtos_usb_power_disable();
	#endif

    return 0;
}

int nvt_user_uninit(void)
{
	Ux_Close();
	BKG_Close();
    GxGfx_Exit();
    UIControl_UninstallID();
    return 0;
}

UINT32 nvt_hdal_init(void)
{
    HD_RESULT ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	ret = hd_common_init(0);
    if(ret != HD_OK) {
        DBG_ERR("common init fail=%d\n", ret);
        return -1;
    }

    #if (GDC_POWER_DOWN==ENABLE)
    hd_common_sysconfig(HD_VIDEOPROC_CFG, 0, HD_VIDEOPROC_CFG_LL_FAST, 0);
    #else
    hd_common_sysconfig(HD_VIDEOPROC_CFG, 0, HD_VIDEOPROC_CFG_GDC_BEST, 0);
    #endif

#if defined(_UI_STYLE_LVGL_)
	#if LV_USER_CFG_USE_ROTATE_BUFFER
	mem_cfg.pool_info[0].type     = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = LOGO_BS_BLK_SIZE;
	mem_cfg.pool_info[0].blk_cnt  = 1;
	mem_cfg.pool_info[0].ddr_id   = DDR_ID0;
	#endif
#else
	#if (VDO_USE_ROTATE_BUFFER==ENABLE)
	if(System_IsBootFromRtos() ==FALSE){
		mem_cfg.pool_info[0].type     = HD_COMMON_MEM_COMMON_POOL;
		mem_cfg.pool_info[0].blk_size = LOGO_BS_BLK_SIZE;
		mem_cfg.pool_info[0].blk_cnt  = 1;
		mem_cfg.pool_info[0].ddr_id   = DDR_ID0;
	}
	#endif
#endif

	ret = hd_common_mem_init(&mem_cfg);
    if(ret != HD_OK) {
        DBG_ERR("mem init fail=%d\n", ret);
        return -1;
    }
    ret = hd_gfx_init();
    if(ret != HD_OK) {
        DBG_ERR("gfx init fail=%d\n", ret);
        return -1;
    }

    return 0;
}

UINT32 nvt_hdal_uninit(void)
{
    HD_RESULT ret;

    ret = hd_gfx_uninit();
    if(ret != HD_OK) {
        DBG_ERR("gfx uninit fail=%d\n", ret);
        return -1;
    }
	ret = hd_common_mem_uninit();
    if(ret != HD_OK) {
        DBG_ERR("mem uninit fail=%d\n", ret);
        return -1;
    }
	ret = hd_common_uninit();
    if(ret != HD_OK) {
        DBG_ERR("common uninit fail=%d\n", ret);
        return -1;
    }
    return 0;

}

ID FLG_ID_MAIN = 0;

#define FLG_MAIN_ON                  0x00000001      //
#define FLG_MAIN_OFF                 0x00000002      //
#define FLG_MAIN_COMMAND             (FLG_MAIN_ON|FLG_MAIN_OFF)      //

extern ID _SECTION(".kercfg_data") FLG_ID_MAIN;

void System_InstallID(void)
{
	OS_CONFIG_FLAG(FLG_ID_MAIN);
}

void System_PowerOnFinish(void)
{
	//clr_flg(FLG_ID_MAIN, FLG_MAIN_COMMAND);
	//sta_tsk(IDLETSK_ID,0);
	vos_flag_set(FLG_ID_MAIN, FLG_MAIN_ON);
}

void System_PowerOffStart(void)
{
	vos_flag_set(FLG_ID_MAIN, FLG_MAIN_OFF);
}

void System_WaitForPowerOnFinish(void)
{
	FLGPTN uiFlag;
	vos_flag_wait(&uiFlag, FLG_ID_MAIN, FLG_MAIN_ON, TWF_ORW);
}

void System_WaitForPowerOffStart(void)
{
	FLGPTN uiFlag;
	vos_flag_wait(&uiFlag, FLG_ID_MAIN, FLG_MAIN_OFF, TWF_ORW);
}
#define MAX_CMD_LEN     512
static UINT32 is_boot_from_rtos = FALSE;

static UINT32 _IsBootFromRtos(void)
{
	UINT32 ret = FALSE;
	int h_cmd;
	char *boot_cmd = NULL;

	if ((boot_cmd = (char *)malloc(MAX_CMD_LEN)) == NULL) {
		DBG_ERR("allocate buffer fail\r\n");
		return ret;
	}
	memset((void *)boot_cmd, 0, MAX_CMD_LEN);

	if ((h_cmd = open("/proc/cmdline", O_RDWR)) < 0) {
		DBG_ERR("open /proc/cmdline failed\r\n");
	} else {
		read(h_cmd, boot_cmd, MAX_CMD_LEN - 1);
		close(h_cmd);
		if (strstr(boot_cmd, "rtos_boot=on") != NULL) {
			ret = TRUE;
		}
	}
	free(boot_cmd);

	return ret;
}

UINT32 System_IsBootFromRtos(void)
{
	return is_boot_from_rtos;
}
void System_ChkBootFromRtos(void)
{
	is_boot_from_rtos = _IsBootFromRtos();
	//is_boot_from_rtos=0;
}

// signal handler
static void signal_ctrl(int no)
{
#if 0
	switch (no) {
	case SIGPIPE: {
			break;
		}
	default: {
			break;
		}
	}
#endif
}

int NvtMain(void)
{
	struct tm ctv = {0};

	// register SIGPIPE handler
	signal(SIGPIPE, signal_ctrl);

	System_InstallID();
#if (USB_MODE==ENABLE)
	System_OnUsbPreInit();
#endif
	System_OnPowerPreInit();

	//System_OnPowerPreInit() should be prior to rtc checking
	ctv = hwclock_get_time(TIME_ID_CURRENT);
	if(ctv.tm_year < DEFAULT_YEAR) {
		ctv.tm_year = DEFAULT_YEAR;
		ctv.tm_mon = DEFAULT_MON;
		ctv.tm_mday = DEFAULT_DAY;
		ctv.tm_hour = DEFAULT_HOUR;
		ctv.tm_min = DEFAULT_MIN;
		ctv.tm_sec = DEFAULT_SEC;
		hwclock_set_time(TIME_ID_CURRENT, ctv, 0);
	}

#if (POWERON_TRACE == ENABLE)
	System_WaitForPowerOnFinish();  // Wait for boot cmd
#endif
	Dx_InitIO();
#if (defined(_NVT_ETHREARCAM_RX_))
	EthCamNet_EthHubPortIsolate(0, 0);
	EthCamNet_EthHubPortIsolate(1, 1);
#endif

	SxTimer_Open(); //start sxtimer task
	System_OnTimerInit();
#if (USB_MODE==ENABLE)
	System_OnUsbInit();
#endif
	nvt_hdal_init();
	nvt_user_init();

	System_WaitForPowerOffStart();  // Wait for shutdown cmd
	nvt_user_uninit();
	nvt_hdal_uninit();

	System_OnPowerPostExit(); //never return
	return 0;
}

