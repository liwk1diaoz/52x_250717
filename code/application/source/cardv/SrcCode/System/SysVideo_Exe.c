/*
    System Video Callback

    System Callback for Video Module.

    @file       SysVideo_Exe.c
    @ingroup    mIPRJSYS

    @note

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

////////////////////////////////////////////////////////////////////////////////
#include "PrjInc.h"
//local debug level: THIS_DBGLVL
#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          SysVideoExe
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

////////////////////////////////////////////////////////////////////////////////
#if (DISPLAY_FUNC == ENABLE)
#include "Dx.h"
#include "DxApi.h"
#include "DxDisplay.h"
#include "SysCommon.h"
#include "sys_mempool.h"
#include "UI/UIGraphics.h"
#include "vendor_common.h"
#include "vendor_videoout.h"
#if (OSD_USE_ROTATE_BUFFER == ENABLE)
#include "UI/UIView.h"
#endif
#include "vf_gfx.h"

static DX_HANDLE gDispDev = 0;

void Display_SetEnable(UINT8 LayerID, BOOL bEnable)
{
}
void Display_SetBuffer(UINT8 LayerID, UINT32 w, UINT32 h, UINT32 off, UINT32 fmt, UINT32 uiBufY, UINT32 uiBufCb, UINT32 uiBufCr)
{
}
void Display_SetWindow(UINT8 LayerID, int x, int y, int w, int h)
{
}

void Display_DetLCDDir(void);
int SX_TIMER_DET_LCDDIR_ID = -1;
#if (LCD_ROTATE_FUNCTION == ENABLE)
SX_TIMER_ITEM(Display_DetLCDDir, Display_DetLCDDir, 100, FALSE)
#endif

INT32 UI_ShowJPG(UINT8 LayerID, UINT32 uiJPGData, UINT32 uiJPGSize);

extern void Video_CB(UINT32 event, UINT32 param1, UINT32 param2);


USIZE Calc_VideoSize_LCD(void)
{
	DX_HANDLE cDispDev = 0;
	ISIZE sz = {0, 0};
	USIZE vdo1_lcd;
	cDispDev = Dx_GetObject(DX_CLASS_DISPLAY_EXT | DX_TYPE_LCD);
	Dx_Getcaps(cDispDev, DISPLAY_CAPS_MAXSIZE, (UINT32)&sz); //query mode info
	vdo1_lcd.w = sz.w;
	vdo1_lcd.h = sz.h;
	return vdo1_lcd;
}
USIZE Calc_VideoSize_MAX(void)
{
	DX_HANDLE cDispDev = 0;
	ISIZE sz = {0, 0};
	USIZE vdo1_max;
	cDispDev = Dx_GetObject(DX_CLASS_DISPLAY_EXT | DX_TYPE_LCD);
	Dx_Getcaps(cDispDev, DISPLAY_CAPS_MAXSIZE, (UINT32)&sz); //query mode info
	{
		vdo1_max.w = sz.w;
		vdo1_max.h = sz.h;
	}
	return vdo1_max;
}

void Check_VideoSize_BufferLimit(UINT32 DevID, DX_HANDLE cDispDev)
{
	ISIZE MaxBufSize = {0, 0};
	ISIZE DeviceSize = {0, 0};
	if (cDispDev != 0) {
		return;
	}
	DeviceSize = GxVideo_GetDeviceSize(DevID);
	Dx_Getcaps(cDispDev, DISPLAY_CAPS_MAXSIZE, (UINT32)&MaxBufSize);
	if ((DeviceSize.w * DeviceSize.h) > (MaxBufSize.w * MaxBufSize.h)) {
		int i;
		for (i = 0; i < 10; i++)
			DBG_ERR("*** Display [%d]: Device Size (%d,%d) > Max Buffer Size (%d,%d) !\r\n",
					DevID, DeviceSize.w, DeviceSize.h, MaxBufSize.w, MaxBufSize.h);
	}
}

void Video_CB_Fast(UINT32 event, UINT32 param1, UINT32 param2);

void System_OnVideoInit(void)
{
	DX_HANDLE cDispDev = 0;
	TM_BOOT_BEGIN("video", "init");
	TM_BOOT_BEGIN("video", "config");

	GxVideo_RegCB(Video_CB_Fast);         //Register CB function of GxVideo

	cDispDev = Dx_GetObject(DX_CLASS_DISPLAY_EXT | DX_TYPE_LCD);
	DBG_IND("LCD = %08x\r\n", cDispDev);

	// Init GxVideo
	GxVideo_InitDevice(DOUT1);
	if (cDispDev) {
		Dx_Init(cDispDev, 0, 0, DISPLAY_VER);
		Dx_Setconfig((DX_HANDLE)cDispDev, DISPLAY_CFG_MODE, LCDMODE);//set default LCD mode
		Dx_Setconfig((DX_HANDLE)cDispDev, DISPLAY_CFG_DUAL, FALSE);
	}
    gDispDev = cDispDev;
	// Init GxDisplay
	GxDisplay_Init(DOUT1, 0, 0);

#if (LCD_ROTATE_FUNCTION == ENABLE)
		SX_TIMER_DET_LCDDIR_ID = SxTimer_AddItem(&Timer_Display_DetLCDDir);
#endif

	TM_BOOT_END("video", "config");
}

void System_OnVideoExit(void)
{
	// Turn off LCD backlight
	GxVideo_SetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_BACKLIGHT, FALSE);
	// Exit GxVideo
	GxVideo_CloseDevice(DOUT1);
	Ux_PostEvent(NVTEVT_VIDEO_DETACH, 1, 0); // detach
}

///////////////////////////////////////////////////////////////////////


#if (LCD_ROTATE_FUNCTION == ENABLE)
void Display_DetLCDDir(void)
{
    if(!UI_IsForceLock()) {
	DX_HANDLE pDispDev1 = Dx_GetObject(DX_CLASS_DISPLAY_EXT | DX_TYPE_LCD);
	GxVideo_DetDir((UINT32)pDispDev1, 0);
	}
}
#endif

DX_HANDLE InsertDevObj = 0;

void System_HideDisplay(UINT32 DevID)
{
	if (DevID == DOUT2) {
		DBG_ERR("disp[%d] no support device\r\n", DevID);
		return;
	}
	if((DX_HANDLE)GxVideo_GetDevice(DevID))
		GxVideo_SetDeviceCtrl(DevID, DISPLAY_DEVCTRL_BACKLIGHT, FALSE);
}

void System_ShowDisplay(UINT32 DevID)
{
	if (DevID == DOUT2) {
		DBG_ERR("disp[%d] no support device\r\n", DevID);
		return;
	}

	if((DX_HANDLE)GxVideo_GetDevice(DevID))
		GxVideo_SetDeviceCtrl(DevID, DISPLAY_DEVCTRL_BACKLIGHT, TRUE);
}
void System_DetachDisplay(UINT32 DevID)
{
	if (DevID == DOUT2) {
		DBG_ERR("disp[%d] no support device\r\n", DevID);
		return;
	}
	GxVideo_SetDeviceCtrl(DevID, DISPLAY_DEVCTRL_BACKLIGHT, FALSE);
	GxVideo_CloseDevice(DevID); //slow API
}

#if defined(_UI_STYLE_LVGL_)

extern const uint32_t palette_define[];

static HD_RESULT set_out_fb(HD_PATH_ID video_out_ctrl, HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	HD_FB_FMT video_out_fmt={0};
	HD_FB_ATTR video_out_attr={0};
	HD_FB_DIM video_out_dim={0};

    #if (DISPLAY_OSD_FMT==PXLFMT_INDEX8)
	HD_FB_PALETTE_TBL video_out_palette={0};

	/* cast const uint32_t to UINT32 for saving mem to config color key */
	UINT32* gDemoKit_Palette = (UINT32 *) palette_define;

	/* make LV_COLOR_TRANSP as color key */
	if(LV_COLOR_TRANSP.full < 256){
		gDemoKit_Palette[LV_COLOR_TRANSP.full] &= 0x00FFFFFF;
	}

	video_out_palette.fb_id = HD_FB0;
	video_out_palette.table_size = 256;
	video_out_palette.p_table = gDemoKit_Palette;

	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_PALETTE_TABLE, &video_out_palette);
	if(ret!= HD_OK)
		return ret;
    #endif

	video_out_fmt.fb_id = HD_FB0;
	video_out_fmt.fmt = DISPLAY_HDAL_OSD_FMT;

	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_FMT, &video_out_fmt);
	if(ret!= HD_OK)
		return ret;

	video_out_attr.fb_id = HD_FB0;
	video_out_attr.alpha_blend = 0xFF;
	video_out_attr.alpha_1555 = 0xFF;
	video_out_attr.colorkey_en = 0;

	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_ATTR, &video_out_attr);
	if(ret!= HD_OK)
		return ret;

	video_out_dim.fb_id = HD_FB0;
#if LV_USER_CFG_USE_ROTATE_BUFFER && (VDO_ROTATE_DIR != HD_VIDEO_DIR_ROTATE_180)
	video_out_dim.input_dim.w = DISPLAY_OSD_H;
	video_out_dim.input_dim.h = DISPLAY_OSD_W;
#else
	video_out_dim.input_dim.w = DISPLAY_OSD_W;
	video_out_dim.input_dim.h = DISPLAY_OSD_H;
#endif
	video_out_dim.output_rect.x = 0;
	video_out_dim.output_rect.y = 0;
	video_out_dim.output_rect.w = p_dim->w;
	video_out_dim.output_rect.h = p_dim->h;

    DBG_DUMP("lcd win %dx%d,size is %dx%d \r\n",video_out_dim.input_dim.w,video_out_dim.input_dim.h,video_out_dim.output_rect.w,video_out_dim.output_rect.h);
	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_DIM, &video_out_dim);
	if(ret!= HD_OK)
		return ret;

    return ret;
}
#endif
void System_AttachDisplay(UINT32 DevID, DX_HANDLE NewDevObj)
{
	if (DevID == DOUT2) {
		DBG_ERR("disp[%d] no support device\r\n", DevID);
		return;
	}
	GxVideo_OpenDevice(DevID, (UINT32)NewDevObj, 0);
	Check_VideoSize_BufferLimit(DevID, NewDevObj);
	GxVideo_SetDeviceCtrl(DevID, DISPLAY_DEVCTRL_BACKLIGHT, FALSE);

#if defined(_UI_STYLE_LVGL_)
{

#if LV_USER_CFG_USE_ROTATE_BUFFER && (VDO_ROTATE_DIR != HD_VIDEO_DIR_ROTATE_180)
	    //keep rotate,get device size would change xy
		GxVideo_SetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_SWAPXY, 1);
#endif

    VENDOR_FB_INIT fb_init;
	HD_FB_ENABLE video_out_enable={0};
    UINT32 ret=0;
	HD_VIDEOOUT_SYSCAPS  video_out_syscaps;
	HD_PATH_ID video_out_ctrl = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_CTRLPATH);
	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, &video_out_syscaps);
	if (ret != HD_OK) {
		DBG_ERR("get video_out_syscaps failed\r\n");
		return;
	}
    set_out_fb(video_out_ctrl,&video_out_syscaps.output_dim);
    //assign bf buffer to FB_INIT
    fb_init.fb_id = HD_FB0;
    fb_init.pa_addr = mempool_disp_osd1_pa;
    fb_init.buf_len = POOL_SIZE_OSD;
	ret = vendor_videoout_set(VENDOR_VIDEOOUT_ID0, VENDOR_VIDEOOUT_ITEM_FB_INIT, &fb_init);
	if(ret!= HD_OK)
		DBG_ERR("disp[%d] VENDOR_VIDEOOUT_ITEM_FB_INIT %d\r\n", DevID,ret);

    //enable fb after HD_VIDEOOUT_PARAM_FB_INIT which would clear buffer,avoid garbage on screen
	video_out_enable.fb_id = HD_FB0;
	video_out_enable.enable = 1;
	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_ENABLE, &video_out_enable);
	if(ret!= HD_OK)
	    DBG_ERR("disp[%d] HD_VIDEOOUT_PARAM_FB_ENABLE %d\r\n", DevID,ret);

}
#else

#if (VDO_USE_ROTATE_BUFFER == ENABLE)
    //keep rotate,get device size would change xy
	GxVideo_SetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_SWAPXY, 1);
#endif

#endif
}

void Video_CB_Fast(UINT32 event, UINT32 param1, UINT32 param2)
{
	switch (event) {
	case DISPLAY_CB_PLUG:
		//This CB will be trigger when TV or HDMI plug
		DBG_IND("DISPLAY_CB_PLUG\r\n");
		{
			InsertDevObj = (DX_HANDLE)param1;

			if (InsertDevObj == 0) {
				return;
			} else {
				char *pDevName = NULL;
				Dx_GetInfo(InsertDevObj, DX_INFO_NAME, &pDevName);
				DBG_IND("^Y===================================== video plug [%s]\r\n", pDevName);
			}

		}
		break;
	}
}
void System_OnVideoInit2(void)
{
	DX_HANDLE cDispDev = gDispDev;

	TM_BOOT_BEGIN("video", "open");

	if (cDispDev) {
		char *pDevName = NULL;
		Dx_GetInfo(cDispDev, DX_INFO_NAME, &pDevName);
		DBG_IND("^Y===================================== video fixed [%s]\r\n", pDevName);
		//config video Output
		System_AttachDisplay(DOUT1, cDispDev);
        #if (LCD_ROTATE_FUNCTION == ENABLE)
		Display_DetLCDDir();
        #endif
	}
	TM_BOOT_END("video", "open");
	TM_BOOT_END("video", "init");
	TM_BOOT_BEGIN("video", "show_logo");
#if (POWERONLOGO_FUNCTION == ENABLE)
	{
		Display_ShowSplash(SPLASH_POWERON);
		Delay_DelayMs(30); // delay some time to avoid LCD flicker as power on
		GxVideo_SetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_BACKLIGHT, TRUE);
        #if (defined(_MODEL_580_SDV_SJ10_) || defined(_MODEL_580_SDV_SJ10_FAST_BT_) || defined(_MODEL_580_SDV_C300_) || defined(_MODEL_580_SDV_C300_FAST_BT_))
        SysMain_system("modprobe lcd_backlight_pwm");  //modprobe lcd_backlight_pwm.ko
        #endif
	}
#endif
	TM_BOOT_END("video", "show_logo");

    #if 0
    //for hdal not close vout
    {
        HD_RESULT ret = 0;
        VENDOR_COMM_VOUT_KEEP_OPEN  vout_keep_open;

		vout_keep_open.is_keep_open = TRUE;
        ret = vendor_common_mem_set(VENDOR_COMMON_MEM_ITEM_VOUT_KEEP_OPEN, &vout_keep_open);
        if(ret!=HD_OK){
    		DBG_ERR("vout keep fail\r\n");
    	}
		ret = hd_common_mem_uninit();
        if(ret!=HD_OK){
    		DBG_ERR("mem_uninit fail\r\n");
    	}
	}
    #endif


    //init OSD
#if defined(_UI_STYLE_LVGL_)
// for movie stamp init lite gfx
//extern void UI_GfxInitLite(void);
//    UI_GfxInitLite();
#else
    #if(UI_FUNC)
    {
		ISIZE DeviceSize;
		DeviceSize = GxVideo_GetDeviceSize(DOUT1);
        #if (OSD_USE_ROTATE_BUFFER == ENABLE)
		View_Window_ConfigAttr(DOUT1, OSD_ATTR_ROTATE, SysVideo_GetDirbyID(DOUT1));
        #endif
    	UI_GfxInit();
    	UI_DispInit();
    	UI_ControlShowInit();

	    UIDisplay_Init(LAYER_OSD1, TRUE, &DeviceSize); //attacth to UI source

		#if defined(_UI_STYLE_LVGL_)
	    	extern const uint32_t palette_define[];
	    	GxDisplay_SetColorKey(LAYER_OSD1, TRUE, palette_define[0]);
		#else
	    	GxDisplay_SetColorKey(LAYER_OSD1,TRUE,COLOR_DEMOKIT_PALETTE_00);
		#endif


        #if (DISPLAY_OSD_FMT == DISP_PXLFMT_INDEX8)
			#if defined(_UI_STYLE_LVGL_)
	    	GxDisplay_SetPalette(LAYER_OSD1, 0, 256, palette_define);
			#else
	    	GxDisplay_SetPalette(LAYER_OSD1, 0, 256, gDemoKit_Palette_Palette);
			#endif
        #endif

		GxDisplay_Set(LAYER_OSD1, LAYER_STATE_ENABLE, FALSE);
        GxDisplay_FlushEx(LAYER_OSD1, FALSE);
	    GxDisplay_Flip(FALSE); //buffer flip and set pallette,blend
        //for next time wnd redraw would enable OSD1
		GxDisplay_Set(LAYER_OSD1, LAYER_STATE_ENABLE, TRUE);
	}
    #endif
#endif
}

#if (defined(_MODEL_580_SDV_SJ10_) || defined(_MODEL_580_SDV_SJ10_FAST_BT_) || defined(_MODEL_580_SDV_C300_) || defined(_MODEL_580_SDV_C300_FAST_BT_))
void System_OnVideoInit3(void)
{
    ISIZE DeviceSize;

    DeviceSize = GxVideo_GetDeviceSize(DOUT1);

#if (OSD_USE_ROTATE_BUFFER == ENABLE)
    View_Window_ConfigAttr(DOUT1, OSD_ATTR_ROTATE, SysVideo_GetDirbyID(DOUT1));
#endif

    UIDisplay_Init(LAYER_OSD1, TRUE, &DeviceSize); //attacth to UI source

#if defined(_UI_STYLE_LVGL_)
    extern const uint32_t palette_define[];

    GxDisplay_SetColorKey(LAYER_OSD1, TRUE, palette_define[0]);
#else
    GxDisplay_SetColorKey(LAYER_OSD1,TRUE,COLOR_DEMOKIT_PALETTE_00);
#endif

#if (DISPLAY_OSD_FMT == DISP_PXLFMT_INDEX8)
	#if defined(_UI_STYLE_LVGL_)
	GxDisplay_SetPalette(LAYER_OSD1, 0, 256, palette_define);
	#else
	GxDisplay_SetPalette(LAYER_OSD1, 0, 256, gDemoKit_Palette_Palette);
	#endif
#endif

    GxDisplay_Set(LAYER_OSD1, LAYER_STATE_ENABLE, FALSE);
    GxDisplay_FlushEx(LAYER_OSD1, FALSE);
    GxDisplay_Flip(FALSE); //buffer flip and set pallette,blend
    //for next time wnd redraw would enable OSD1
    GxDisplay_Set(LAYER_OSD1, LAYER_STATE_ENABLE, TRUE);
}
#endif

INT32 System_OnVideoAttach(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    DBG_FUNC_BEGIN("\r\n");
	return NVTEVT_CONSUME;
}

INT32 System_OnVideoDetach(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
#if (defined(_MODEL_580_SDV_SJ10_) || defined(_MODEL_580_SDV_SJ10_FAST_BT_))
    DX_HANDLE cDispDev = gDispDev;

    switch (SysGetFlag(FL_CHG_DISP)) {
    case LCD_CHG_DISP_LCD1:
    GxVideo_CloseDevice(DOUT1);

        SysSetFlag(FL_CURR_DISP, LCD_CHG_DISP_LCD1);
        SysMain_system("rmmod disp_if8b_lcd1_st7789");
        vos_util_delay_ms(300);
        SysMain_system("modprobe disp_ifdsi_lcd1_st7701");
        vos_util_delay_ms(300);

        System_OnVideoInit();
        GxVideo_OpenDevice(DOUT1, (UINT32)cDispDev, 0);
        System_OnVideoInit3();
        SysSetFlag(FL_CHG_DISP, LCD_CHG_DISP_NONE);
        break;

    case LCD_CHG_DISP_LCD2:
        GxVideo_CloseDevice(DOUT1);

        SysSetFlag(FL_CURR_DISP, LCD_CHG_DISP_LCD2);
        SysMain_system("rmmod disp_ifdsi_lcd1_st7701");
        vos_util_delay_ms(300);
        SysMain_system("modprobe disp_if8b_lcd1_st7789");
        vos_util_delay_ms(300);

        System_OnVideoInit();
        GxVideo_OpenDevice(DOUT1, (UINT32)cDispDev, 0);
        System_OnVideoInit3();
        SysSetFlag(FL_CHG_DISP, LCD_CHG_DISP_NONE);
        break;

    default:
        ;
    }
#elif (defined(_MODEL_580_SDV_C300_) || defined(_MODEL_580_SDV_C300_FAST_BT_))
    DX_HANDLE cDispDev = gDispDev;

    switch (SysGetFlag(FL_CHG_DISP)) {
    case LCD_CHG_DISP_LCD1:
    GxVideo_CloseDevice(DOUT1);

        SysSetFlag(FL_CURR_DISP, LCD_CHG_DISP_LCD1);
        SysMain_system("rmmod disp_ifdsi_lcd1_st7701_c300");
        vos_util_delay_ms(300);
        SysMain_system("modprobe disp_if8b_lcd1_st7789_c300");
        vos_util_delay_ms(300);

        System_OnVideoInit();
        GxVideo_OpenDevice(DOUT1, (UINT32)cDispDev, 0);
        System_OnVideoInit3();
        SysSetFlag(FL_CHG_DISP, LCD_CHG_DISP_NONE);
        break;

    case LCD_CHG_DISP_LCD2:
        GxVideo_CloseDevice(DOUT1);

        SysSetFlag(FL_CURR_DISP, LCD_CHG_DISP_LCD2);
        SysMain_system("rmmod disp_if8b_lcd1_st7789_c300");
        vos_util_delay_ms(300);
        SysMain_system("modprobe disp_ifdsi_lcd1_st7701_c300");
        vos_util_delay_ms(300);

        System_OnVideoInit();
        GxVideo_OpenDevice(DOUT1, (UINT32)cDispDev, 0);
        System_OnVideoInit3();
        SysSetFlag(FL_CHG_DISP, LCD_CHG_DISP_NONE);
        break;

    default:
        ;
    }
#endif

    DBG_FUNC_BEGIN("\r\n");
	return NVTEVT_CONSUME;
}

INT32 System_OnVideoDir(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    DBG_FUNC_BEGIN("\r\n");
	return NVTEVT_CONSUME;
}

INT32 System_OnVideoInsert(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    DBG_FUNC_BEGIN("\r\n");
	return NVTEVT_CONSUME;
}

INT32 System_OnVideoRemove(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    DBG_FUNC_BEGIN("\r\n");
	return NVTEVT_CONSUME;
}

INT32 System_OnVideoMode(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    DBG_FUNC_BEGIN("\r\n");
	return NVTEVT_CONSUME;
}

/////////////////////////////////////////////////////////////////////////////


extern void VdoOut_CB(UINT32 event, UINT32 param1, UINT32 param2);


/////////////////////////////////////////////////////////////////////////////

UINT32   p_gLogoJPG = 0;
UINT32   p_gLogoSize = 0;
static UINT32	logo_frame_buf_pa = 0;
static UINT32 logo_frame_buf_va = 0;


void Load_DispLogo(void)
{

}

static HD_RESULT get_logo_frame_buf(HD_VIDEO_FRAME *p_video_frame)
{
    HD_RESULT ret = 0;
    UINT32 va = 0;
	HD_COMMON_MEM_DDR_ID ddr_id = 0;
	UINT32 blk_size = 0, pa = 0;
	UINT32 width = 0, height = 0;
	HD_VIDEO_PXLFMT pxlfmt = 0;

	if (p_video_frame == NULL) {
		return HD_ERR_SYS;
	}

	// config yuv info
	ddr_id = DDR_ID0;
	width = LOGO_SIZE_W;
	height = LOGO_SIZE_H;
	pxlfmt = LOGO_FMT;
	blk_size = LOGO_YUV_BLK_SIZE;
	if(System_IsBootFromRtos() ==TRUE){
		logo_frame_buf_pa=mempool_nvtmpp_temp_pa;
		logo_frame_buf_va=mempool_nvtmpp_temp_va;
		if(mempool_nvtmpp_temp_va==0){
			DBG_ERR("temp_va==0\r\n");
		}
		p_video_frame->sign = MAKEFOURCC('V', 'F', 'R', 'M');
		p_video_frame->ddr_id = ddr_id;
		p_video_frame->pxlfmt = pxlfmt;
		p_video_frame->dim.w = width;
		p_video_frame->dim.h = height;
		p_video_frame->pw[0] = ALIGN_CEIL_64(width);
		p_video_frame->ph[0] = ALIGN_CEIL_64(height);
		p_video_frame->phy_addr[0] = logo_frame_buf_pa;
		p_video_frame->blk = -2UL;


		//no use vdodec
		UINT32 addr[HD_VIDEO_MAX_PLANE] = {0};
		UINT32 loff[HD_VIDEO_MAX_PLANE] = {0};
		addr[0] = logo_frame_buf_pa;
		loff[0] = ALIGN_CEIL_4(width);
		addr[1] = logo_frame_buf_pa + loff[0] * height;
		loff[1] = ALIGN_CEIL_4(width);
		if ((ret = vf_init_ex(p_video_frame, width,height, HD_VIDEO_PXLFMT_YUV420, loff, addr)) != HD_OK) {
			DBG_ERR("vf_init_ex dst failed(%d)\r\n", ret);
		}
		p_video_frame->blk = -2UL;
	}else{
		ret = hd_common_mem_alloc("NVTMPP_TEMP", &pa, (void **)&va, blk_size, ddr_id);
		if(ret !=0) {
		    return ret;
		}

		p_video_frame->sign = MAKEFOURCC('V', 'F', 'R', 'M');
		p_video_frame->ddr_id = ddr_id;
		p_video_frame->pxlfmt = pxlfmt;
		p_video_frame->dim.w = width;
		p_video_frame->dim.h = height;
		p_video_frame->phy_addr[0] = pa;
		p_video_frame->blk = -2;

		DBG_DUMP("frame blk %x blk_size %x,width %d,height %d,pa %x\r\n",p_video_frame->blk,blk_size,width,height,pa);

		//[coverity]:156332 tmp buffer would not release,it would keep in vout until next push
		ret = hd_common_mem_free(pa, (void *)va);
		if (ret != HD_OK) {
			DBG_ERR("err:free pa = 0x%x, va = 0x%x\r\n", pa, va);
		}
	}
	return HD_OK;
}
INT32 UI_ShowLogoYUV(void)
{
	HD_RESULT   ret;
	HD_VIDEO_FRAME video_frame = {0};
	HD_PATH_ID video_out_path = GxVideo_GetDeviceCtrl(DOUT1,DISPLAY_DEVCTRL_PATH);
	HD_VIDEOOUT_IN video_out_param={0};
	HD_VIDEOOUT_WIN_ATTR video_out_param2 = {0};
	VENDOR_VIDEOOUT_FUNC_CONFIG videoout_cfg = {0};


	video_out_param.dim.w = LOGO_SIZE_W;
	video_out_param.dim.h = LOGO_SIZE_H;
	video_out_param.pxlfmt = LOGO_FMT;

#if defined(_UI_STYLE_LVGL_)

	#if LV_USER_CFG_USE_ROTATE_BUFFER
	video_out_param.dir = VDO_ROTATE_DIR;
	#else
	video_out_param.dir = HD_VIDEO_DIR_NONE;
	#endif

#else

	#if(VDO_USE_ROTATE_BUFFER == ENABLE)
	video_out_param.dir = VDO_ROTATE_DIR;
	#else
	video_out_param.dir = HD_VIDEO_DIR_NONE;
	#endif

#endif



	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if(ret!=HD_OK)
	    goto exit;

	video_out_param2.visible = TRUE;
	video_out_param2.rect.x = 0;
	video_out_param2.rect.y = 0;

#if defined(_UI_STYLE_LVGL_)

	#if LV_USER_CFG_USE_ROTATE_BUFFER &&  (VDO_ROTATE_DIR != HD_VIDEO_DIR_ROTATE_180)
		video_out_param2.rect.w = GxVideo_GetDeviceSize(DOUT1).h;
		video_out_param2.rect.h = GxVideo_GetDeviceSize(DOUT1).w;
	#else
		video_out_param2.rect.w = GxVideo_GetDeviceSize(DOUT1).w;
		video_out_param2.rect.h = GxVideo_GetDeviceSize(DOUT1).h;
	#endif

#else

	#if (VDO_USE_ROTATE_BUFFER == ENABLE)
		video_out_param2.rect.w = GxVideo_GetDeviceSize(DOUT1).h;
		video_out_param2.rect.h = GxVideo_GetDeviceSize(DOUT1).w;
	#else
		video_out_param2.rect.w = GxVideo_GetDeviceSize(DOUT1).w;
		video_out_param2.rect.h = GxVideo_GetDeviceSize(DOUT1).h;
	#endif

#endif


	video_out_param2.layer = HD_LAYER1;
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &video_out_param2);
	if(ret!=HD_OK)
	    goto exit;





	ret = hd_videoout_start(video_out_path);
	if (ret != HD_OK) {
	    DBG_ERR("hd_videoout_start , ret=%d\r\n",ret);
		goto exit;
	}

	// get video frame
	ret = get_logo_frame_buf(&video_frame);
	if (ret != HD_OK) {
		DBG_ERR("get video frame error(%d) !!\r\n", ret);
	    goto exit;
	}


	memcpy((void *)logo_frame_buf_va,(void *)g_ucBGOpening_yuv,LOGO_YUV_BLK_SIZE);

	DBG_DUMP("logo_frame_buf_va=0x%lx\r\n",logo_frame_buf_va);
	DBG_DUMP("logo_frame_buf_va=0x%lx\r\n",logo_frame_buf_va);
	DBG_DUMP("logo_frame_buf_va=0x%lx\r\n",logo_frame_buf_va);

	ret = hd_common_mem_flush_cache((void *)logo_frame_buf_va, LOGO_YUV_BLK_SIZE);
	if(ret!=HD_OK){
	    DBG_ERR("flush %d\r\n",ret);
	    goto exit;
	}


	ret = hd_videoout_push_in_buf(video_out_path, &video_frame, NULL, 0); // only support non-blocking mode now
	if (ret != HD_OK) {
		DBG_ERR("videoout push_in error %d!!\r\n",ret);
	    goto exit;
	}

	videoout_cfg.in_func = VENDOR_VIDEOOUT_INFUNC_KEEP_LAST;
	if ((ret = vendor_videoout_set(video_out_path, VENDOR_VIDEOOUT_ITEM_FUNC_CONFIG, &videoout_cfg)) != HD_OK) {
	    DBG_ERR("KEEP_LAST fail ret=%d\r\n",ret);
	    return ret;
	}

exit:
	if((ret = hd_videoout_stop(video_out_path)) != HD_OK){
	    DBG_ERR("hd_videoout_stop, ret=%d\n", ret);
	    return ret;
	}

	return HD_OK;
}
INT32 UI_ShowJPG(UINT8 LayerID, UINT32 uiJPGData, UINT32 uiJPGSize)
{
	HD_RESULT   ret;
	HD_PATH_ID  dec_path = 0;
	HD_VIDEODEC_PATH_CONFIG video_path_cfg = {0};
	HD_VIDEODEC_IN video_in_param = {0};
	UINT32 pa = 0, va = 0;
	UINT32 blk_size = LOGO_BS_BLK_SIZE;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_VIDEO_CODEC  dec_type = HD_CODEC_TYPE_JPEG;
	HD_VIDEODEC_BS video_bitstream = {0};
	HD_VIDEO_FRAME video_frame = {0};
	HD_PATH_ID video_out_path = GxVideo_GetDeviceCtrl(DOUT1,DISPLAY_DEVCTRL_PATH);
	HD_VIDEOOUT_IN video_out_param={0};
	HD_VIDEOOUT_WIN_ATTR video_out_param2 = {0};
	VENDOR_VIDEOOUT_FUNC_CONFIG videoout_cfg = {0};

	if(System_IsBootFromRtos() ==TRUE){
		return UI_ShowLogoYUV();
	}

	if(uiJPGSize>blk_size){
	    DBG_ERR("log bs buffer samll %d < %d\r\n",blk_size,uiJPGSize);
	    return -1;
	}

	video_out_param.dim.w = LOGO_SIZE_W;
	video_out_param.dim.h = LOGO_SIZE_H;
	video_out_param.pxlfmt = LOGO_FMT;

#if defined(_UI_STYLE_LVGL_)

	#if LV_USER_CFG_USE_ROTATE_BUFFER
	video_out_param.dir = VDO_ROTATE_DIR;
	#else
	video_out_param.dir = HD_VIDEO_DIR_NONE;
	#endif

#else

	#if(VDO_USE_ROTATE_BUFFER == ENABLE)
	video_out_param.dir = VDO_ROTATE_DIR;
	#else
	video_out_param.dir = HD_VIDEO_DIR_NONE;
	#endif

#endif

	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if(ret!=HD_OK)
        goto exit;

	video_out_param2.visible = TRUE;
	video_out_param2.rect.x = 0;
	video_out_param2.rect.y = 0;

#if defined(_UI_STYLE_LVGL_)

	#if LV_USER_CFG_USE_ROTATE_BUFFER &&  (VDO_ROTATE_DIR != HD_VIDEO_DIR_ROTATE_180)
		video_out_param2.rect.w = GxVideo_GetDeviceSize(DOUT1).h;
		video_out_param2.rect.h = GxVideo_GetDeviceSize(DOUT1).w;
	#else
		video_out_param2.rect.w = GxVideo_GetDeviceSize(DOUT1).w;
		video_out_param2.rect.h = GxVideo_GetDeviceSize(DOUT1).h;
	#endif

#else

	#if (VDO_USE_ROTATE_BUFFER == ENABLE)
		video_out_param2.rect.w = GxVideo_GetDeviceSize(DOUT1).h;
		video_out_param2.rect.h = GxVideo_GetDeviceSize(DOUT1).w;
	#else
		video_out_param2.rect.w = GxVideo_GetDeviceSize(DOUT1).w;
		video_out_param2.rect.h = GxVideo_GetDeviceSize(DOUT1).h;
	#endif

#endif

	video_out_param2.layer = HD_LAYER1;
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &video_out_param2);
	if(ret!=HD_OK)
        goto exit;

	if((ret = hd_videodec_init()) != HD_OK){
		goto exit;
	}
    if((ret = hd_videodec_open(HD_VIDEODEC_0_IN_0, HD_VIDEODEC_0_OUT_0, &dec_path)) != HD_OK){
        goto exit;
    }

	// set videodec path config
	video_path_cfg.max_mem.codec_type = dec_type;
	video_path_cfg.max_mem.dim.w = LOGO_SIZE_W;
	video_path_cfg.max_mem.dim.h = LOGO_SIZE_H;
	ret = hd_videodec_set(dec_path, HD_VIDEODEC_PARAM_PATH_CONFIG, &video_path_cfg);
	if (ret != HD_OK) {
		goto exit;
	}

	//--- HD_VIDEODEC_PARAM_IN ---
	video_in_param.codec_type = dec_type;
	ret = hd_videodec_set(dec_path, HD_VIDEODEC_PARAM_IN, &video_in_param);
	if (ret != HD_OK) {
		goto exit;
	}

    ret = hd_videodec_start(dec_path);
	if (ret != HD_OK) {
		goto exit;
	}
    ret = hd_videoout_start(video_out_path);
	if (ret != HD_OK) {
		goto exit;
	}

    ret = hd_common_mem_alloc("logo_bs", &pa, (void **)&va, blk_size, ddr_id);
    if(ret !=0) {
        return ret;
    }
	// get video frame
	ret = get_logo_frame_buf(&video_frame);
	if (ret != HD_OK) {
		DBG_ERR("get video frame error(%d) !!\r\n", ret);
        goto exit;
	}

    memcpy((void *)va,(void *)uiJPGData,uiJPGSize);
	ret = hd_common_mem_flush_cache((void *)va, uiJPGSize);
    if(ret!=HD_OK){
        DBG_ERR("flush %d\r\n",ret);
        goto exit;
    }

	// config video bs
	video_bitstream.sign          = MAKEFOURCC('V','S','T','M');
	video_bitstream.p_next        = NULL;
	video_bitstream.ddr_id        = ddr_id;
	video_bitstream.vcodec_format = dec_type;
	video_bitstream.timestamp     = hd_gettime_us();
	video_bitstream.blk           = -2; //no common pool,should use -2
	video_bitstream.count         = 0;
	video_bitstream.phy_addr      = pa;
	video_bitstream.size          = uiJPGSize;
	ret = hd_videodec_push_in_buf(dec_path, &video_bitstream, &video_frame, 0); // only support non-blocking mode now
	if (ret != HD_OK) {
		DBG_ERR("videodec push_in error(%d) !!\r\n", ret);
		goto exit;
	}

    ret = hd_videodec_pull_out_buf(dec_path, &video_frame, 500); // >1 = timeout mode
	if (ret != HD_OK) {
        goto exit;
	}
	ret = hd_videoout_push_in_buf(video_out_path, &video_frame, NULL, 0); // only support non-blocking mode now
	if (ret != HD_OK) {
		DBG_ERR("videoout push_in error %d!!\r\n",ret);
        goto exit;
	}

    videoout_cfg.in_func = VENDOR_VIDEOOUT_INFUNC_KEEP_LAST;
    if ((ret = vendor_videoout_set(video_out_path, VENDOR_VIDEOOUT_ITEM_FUNC_CONFIG, &videoout_cfg)) != HD_OK) {
        DBG_ERR("KEEP_LAST fail ret=%d\r\n",ret);
        return ret;
    }

exit:
	// release video frame buf
	ret = hd_videodec_release_out_buf(dec_path, &video_frame);
	if (ret != HD_OK) {
		DBG_ERR("release video frame error(%d) !!\r\n\r\n", ret);
	}

    ret = hd_common_mem_free(pa,(void *)va);
	if (ret != HD_OK) {
		DBG_ERR("free error(%d) !!\n", ret);
	}
    if((ret = hd_videodec_stop(dec_path)) != HD_OK)
        return ret;


    if((ret = hd_videoout_stop(video_out_path)) != HD_OK)
        return ret;
    if((ret = hd_videodec_close(dec_path)) != HD_OK)
        return ret;
    if((ret = hd_videodec_uninit()) != HD_OK)
        return ret;

    return HD_OK;
}


void Display_ShowSplash(SPLASH_ID splash_id)
{
    DBG_FUNC_BEGIN("\r\n");
	//load default logo
	if (splash_id == SPLASH_POWERON) {
#if (POWERONLOGO_FUNCTION == ENABLE)
		p_gLogoJPG = (UINT32)g_ucBGOpening;
		p_gLogoSize = Logo_getBGOpening_size();
#endif
	} else {
#if (POWEROFFLOGO_FUNCTION == ENABLE)
		p_gLogoJPG = (UINT32)g_ucBGGoodbye;
		p_gLogoSize = Logo_getBGGoodbye_size();
#endif
	}
	//show logo
	DBG_IND("Show Logo\r\n");
	UI_ShowJPG(LOGO_DISP_LAYER, (UINT32)p_gLogoJPG, p_gLogoSize);
}
void Display_HideSplash(void)
{
    DBG_FUNC_BEGIN("\r\n");
}
void Display_ShowPreview(void)
{
	static BOOL bFirst = TRUE;
	if (bFirst) {
		TM_BOOT_BEGIN("video", "hide_logo");
	}
	DBG_IND("Show Video\r\n");

	Display_SetEnable(LAYER_VDO1, TRUE);
#if (POWERONLOGO_FUNCTION == ENABLE)
	Display_HideSplash();
#else
	GxVideo_SetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_BACKLIGHT, TRUE);
#endif
	if (bFirst) {
		TM_BOOT_END("video", "hide_logo");
	}
	bFirst = FALSE;
}

//DevID is DOUT1,DOUT2
UINT32 SysVideo_GetDirbyID(UINT32 DevID)
{
    return VDO_ROTATE_DIR;
}
#endif
