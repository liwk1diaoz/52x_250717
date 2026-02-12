/**
	@brief Sample code of video liveview.\n

	@file videocap_testkit.c

	@author Janice Huang

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "hdal.h"
#include "hd_debug.h"
#include "hd_util.h"
#include <kwrap/examsys.h>

#if defined(__LINUX)
#else
//for delay
#include <kwrap/task.h>
#define sleep(x)    vos_task_delay_ms(1000*x)
#define usleep(x)   vos_task_delay_us(x)
#endif

#define DEBUG_MENU 		1

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////

//header
#define DBGINFO_BUFSIZE()	(0x200)

//RAW
#define VDO_RAW_BUFSIZE(w, h, pxlfmt)   (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NRX: RAW compress: Only support 12bit mode
#define RAW_COMPRESS_RATIO 59
#define VDO_NRX_BUFSIZE(w, h)           (ALIGN_CEIL_4(ALIGN_CEIL_64(w) * 12 / 8 * RAW_COMPRESS_RATIO / 100 * (h)))
//CA for AWB
#define VDO_CA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 3) << 1)
//LA for AE
#define VDO_LA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 1) << 1)

//YUV
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	(ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NVX: YUV compress
#define YUV_COMPRESS_RATIO 75
#define VDO_NVX_BUFSIZE(w, h, pxlfmt)	(VDO_YUV_BUFSIZE(w, h, pxlfmt) * YUV_COMPRESS_RATIO / 100)

///////////////////////////////////////////////////////////////////////////////

#define SEN_OUT_FMT		HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT		HD_VIDEO_PXLFMT_NRX12
#define CA_WIN_NUM_W		32
#define CA_WIN_NUM_H		32
#define LA_WIN_NUM_W		32
#define LA_WIN_NUM_H		32
#define VA_WIN_NUM_W		16
#define VA_WIN_NUM_H		16
#define YOUT_WIN_NUM_W	128
#define YOUT_WIN_NUM_H	128
#define ETH_8BIT_SEL		0 //0: 2bit out, 1:8 bit out
#define ETH_OUT_SEL		1 //0: full, 1: subsample 1/2

#define VDO_SIZE_W		1920
#define VDO_SIZE_H		1080

#define VDOPRC_MAX_DIM_W VDO_SIZE_W
#define VDOPRC_MAX_DIM_H VDO_SIZE_H
#define VDO_OUTPUT_TYPE   1// 1 for LCD

#define TEST_WITH_LIVIEW	1

#define SEN1_VCAP_ID 0

///////////////////////////////////////////////////////////////////////////////
/********************************************************************
	DEBUG MENU IMPLEMENTATION
********************************************************************/
#define HD_DEBUG_MENU_ID_QUIT 0xFE
#define HD_DEBUG_MENU_ID_RETURN 0xFF
#define HD_DEBUG_MENU_ID_LAST (-1)

typedef struct _HD_DBG_MENU {
	int            menu_id;          ///< user command value
	const char     *p_name;          ///< command string
	int      (*p_func)(void);  ///< command function
	BOOL           b_enable;         ///< execution option
} HD_DBG_MENU;

static void hd_debug_menu_print_p(HD_DBG_MENU *p_menu, const char *p_title)
{
	printf("\n==============================");
	printf("\n %s", p_title);
	printf("\n------------------------------");

	while (p_menu->menu_id != HD_DEBUG_MENU_ID_LAST) {
		if (p_menu->b_enable) {
			printf("\n %02d : %s", p_menu->menu_id, p_menu->p_name);
		}
		p_menu++;
	}

	printf("\n------------------------------");
	printf("\n %02d : %s", HD_DEBUG_MENU_ID_QUIT, "Quit");
	printf("\n %02d : %s", HD_DEBUG_MENU_ID_RETURN, "Return");
	printf("\n------------------------------\n");
}

static int hd_debug_menu_exec_p(int menu_id, HD_DBG_MENU *p_menu)
{
	if (menu_id == HD_DEBUG_MENU_ID_RETURN) {
		return 0; // return 0 for return upper menu
	}

	if (menu_id == HD_DEBUG_MENU_ID_QUIT) {
		return -1; // return -1 to notify upper layer to quit
	}

	while (p_menu->menu_id != HD_DEBUG_MENU_ID_LAST) {
		if (p_menu->menu_id == menu_id && p_menu->b_enable) {
			printf("Run: %02d : %s\r\n", p_menu->menu_id, p_menu->p_name);
			if (p_menu->p_func) {
				return p_menu->p_func();
			} else {
				printf("ERR: null function for menu id = %d\n", menu_id);
				return 0; // just skip
			}
		}
		p_menu++;
	}

	printf("ERR: cannot find menu id = %d\n", menu_id);
	return 0;
}

static int hd_debug_menu_entry_p(HD_DBG_MENU *p_menu, const char *p_title)
{
	int menu_id = 0;

	do {
		hd_debug_menu_print_p(p_menu, p_title);
		menu_id = (int)hd_read_decimal_key_input("");
		if (hd_debug_menu_exec_p(menu_id, p_menu) == -1) {
			return -1; //quit
		}
	} while (menu_id != HD_DEBUG_MENU_ID_RETURN);

	return 0;
}
///////////////////////////////////////////////////////////////////////////////
typedef struct _VDOCAP_TEST {

	// (1)
	HD_VIDEOCAP_SYSCAPS cap_syscaps;
	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	HD_DIM  cap_dim;
	HD_DIM  proc_max_dim;

	// (2)
	HD_VIDEOPROC_SYSCAPS proc_syscaps;
	HD_PATH_ID proc_ctrl;
	HD_PATH_ID proc_path;

	HD_DIM  out_max_dim;
	HD_DIM  out_dim;

	// (3)
	HD_VIDEOOUT_SYSCAPS out_syscaps;
	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;

    HD_VIDEOOUT_HDMI_ID hdmi_id;

    // (4) sensor param
    UINT32 blk_cnt;
    UINT32 blk_size;
    HD_VIDEOCAP_DRV_CONFIG cap_cfg;
    HD_VIDEOCAP_CTRL iq_ctl;
    HD_VIDEOCAP_SEN_FRAME_NUM out_frame_num;
    HD_VIDEO_PXLFMT cap_pxlfmt;

	//pthread_t  cap_thread_id;
	//UINT32     cap_exit;
	//UINT32     cap_snap;
	//UINT32     flow_start;
	//INT32    wait_ms;
	//UINT32 	show_ret;
} VDOCAP_TEST;

VDOCAP_TEST stream[1] = {0}; //0: main stream

typedef enum _SENSOR_TYPE {
	SENSOR_IMX290_LINEAR,
	SENSOR_IMX290_HDR,
	SENSOR_TYPE_MAX,
	ENUM_DUMMY4WORD(SENSOR_TYPE)
} SENSOR_TYPE;

int vdocap_basic(VDOCAP_TEST *p_vcap_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);

static int vdocap_basic_flow(void);
static int vdocap_change_mode(void);
static int vdocap_crop(void);
static int vdocap_out(void);


static HD_DBG_MENU vdocap_test_main_menu[] = {
	{0x01, "open/close/start/stop",  vdocap_basic_flow,              TRUE},
	{0x02, "change mode",            vdocap_change_mode,             TRUE},
	{0x03, "crop",                   vdocap_crop,             TRUE},
	{0x04, "out",                   vdocap_out,             TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT mem_init(UINT32 blk_cnt, UINT32 blk_size)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = blk_size;
	mem_cfg.pool_info[0].blk_cnt = blk_cnt;

	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[1].blk_cnt = 3;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;

	ret = hd_common_mem_init(&mem_cfg);
	return ret;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	hd_common_mem_uninit();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
#if 0
static void save_raw(int id)
{
	char cmd[256];

	snprintf(cmd, 256, "echo saveraw %d > /proc/nvt_ctl_sie/cmd", id);
	system(cmd);
}
#endif
static HD_RESULT get_cap_caps(HD_PATH_ID video_cap_ctrl, HD_VIDEOCAP_SYSCAPS *p_video_cap_syscaps)
{
	HD_RESULT ret = HD_OK;
	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSCAPS, p_video_cap_syscaps);
	return ret;
}

static HD_RESULT get_cap_sysinfo(HD_PATH_ID video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_SYSINFO sys_info = {0};

	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
	printf("sys_info.devid =0x%X, cur_fps[0]=%d/%d, vd_count=%llu, output_started=%d, cur_dim(%dx%d)\r\n",
		sys_info.dev_id, GET_HI_UINT16(sys_info.cur_fps[0]), GET_LO_UINT16(sys_info.cur_fps[0]), sys_info.vd_count, sys_info.output_started, sys_info.cur_dim.w, sys_info.cur_dim.h);
	return ret;
}

static HD_RESULT set_cap_cfg(VDOCAP_TEST *p_stream)
{
	HD_RESULT ret = HD_OK;

	ret = hd_videocap_open(0, HD_VIDEOCAP_CTRL(SEN1_VCAP_ID), &p_stream->cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}
	ret |= hd_videocap_set(p_stream->cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &p_stream->cap_cfg);
	ret |= hd_videocap_set(p_stream->cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &p_stream->iq_ctl);

	return ret;
}

static HD_RESULT set_cap_param(VDOCAP_TEST *p_stream)// HD_PATH_ID video_cap_path, HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};

		video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
		video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
		video_in_param.dim.w = p_stream->cap_dim.w;
		video_in_param.dim.h = p_stream->cap_dim.h;
		video_in_param.pxlfmt = SEN_OUT_FMT;
		video_in_param.out_frame_num = p_stream->out_frame_num;
		ret = hd_videocap_set(p_stream->cap_path, HD_VIDEOCAP_PARAM_IN, &video_in_param);
		//printf("set_cap_param MODE=%d\r\n", ret);
		if (ret != HD_OK) {
			return ret;
		}
	}
	#if 1 //no crop, full frame
	{
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_OFF;
		ret = hd_videocap_set(p_stream->cap_path, HD_VIDEOCAP_PARAM_OUT_CROP, &video_crop_param);
		//printf("set_cap_param CROP NONE=%d\r\n", ret);
	}
	#else //HD_CROP_ON
	{
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_ON;
		video_crop_param.win.rect.x = 0;
		video_crop_param.win.rect.y = 0;
		video_crop_param.win.rect.w = 1920/2;
		video_crop_param.win.rect.h= 1080/2;
		video_crop_param.align.w = 4;
		video_crop_param.align.h = 4;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT_CROP, &video_crop_param);
		//printf("set_cap_param CROP ON=%d\r\n", ret);
	}
	#endif
	{
		HD_VIDEOCAP_OUT video_out_param = {0};

		//without setting dim for no scaling, using original sensor out size
		video_out_param.pxlfmt = p_stream->cap_pxlfmt;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		ret = hd_videocap_set(p_stream->cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_proc_cfg(VDOCAP_TEST *p_stream)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};

	if (p_stream == NULL) {
		return HD_ERR_NG;
	}
	ret = hd_videoproc_open(0, HD_VIDEOPROC_0_CTRL, &p_stream->proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;


	video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWALL;
	video_cfg_param.isp_id = SEN1_VCAP_ID;
	video_cfg_param.ctrl_max.func = 0;
	if (p_stream->iq_ctl.func & HD_VIDEOCAP_FUNC_SHDR) {
		video_cfg_param.ctrl_max.func |= HD_VIDEOPROC_FUNC_SHDR;
	}
	video_cfg_param.in_max.func = 0;
	video_cfg_param.in_max.dim.w = p_stream->proc_max_dim.w;
	video_cfg_param.in_max.dim.h = p_stream->proc_max_dim.h;
	video_cfg_param.in_max.pxlfmt = p_stream->cap_pxlfmt;
	video_cfg_param.in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
	ret = hd_videoproc_set(p_stream->proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &video_cfg_param);
	if (ret != HD_OK) {
		return HD_ERR_NG;
	}

	video_ctrl_param.func = 0;
	if (p_stream->iq_ctl.func & HD_VIDEOCAP_FUNC_SHDR) {
		video_ctrl_param.func |= HD_VIDEOPROC_FUNC_SHDR;
	}
	ret = hd_videoproc_set(p_stream->proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);

	return ret;
}

static HD_RESULT set_proc_param(HD_PATH_ID video_proc_path, HD_DIM* p_dim)
{
	HD_RESULT ret = HD_OK;

	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_out_cfg(HD_PATH_ID *p_video_out_ctrl, UINT32 out_type,HD_VIDEOOUT_HDMI_ID hdmi_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_MODE videoout_mode = {0};
	HD_PATH_ID video_out_ctrl = 0;

	ret = hd_videoout_open(0, HD_VIDEOOUT_0_CTRL, &video_out_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	printf("out_type=%d\r\n", out_type);
	switch(out_type){
	case 0:
		videoout_mode.output_type = HD_COMMON_VIDEO_OUT_CVBS;
		videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.cvbs= HD_VIDEOOUT_CVBS_NTSC;
	break;
	case 1:
		videoout_mode.output_type = HD_COMMON_VIDEO_OUT_LCD;
		videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.lcd = HD_VIDEOOUT_LCD_0;
	break;
	case 2:
		videoout_mode.output_type = HD_COMMON_VIDEO_OUT_HDMI;
		videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.hdmi= hdmi_id;
	break;
	default:
		printf("not support out_type\r\n");
	break;
	}
	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_MODE, &videoout_mode);

	*p_video_out_ctrl=video_out_ctrl ;
	return ret;
}

static HD_RESULT get_out_caps(HD_PATH_ID video_out_ctrl,HD_VIDEOOUT_SYSCAPS *p_video_out_syscaps)
{
	HD_RESULT ret = HD_OK;
    HD_DEVCOUNT video_out_dev = {0};

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_DEVCOUNT, &video_out_dev);
	if (ret != HD_OK) {
		return ret;
	}
	printf("##devcount %d\r\n", video_out_dev.max_dev_count);

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, p_video_out_syscaps);
	if (ret != HD_OK) {
		return ret;
	}
	return ret;
}

static HD_RESULT set_out_param(HD_PATH_ID video_out_path, HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_IN video_out_param={0};

	video_out_param.dim.w = p_dim->w;
	video_out_param.dim.h = p_dim->h;
	video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	video_out_param.dir = HD_VIDEO_DIR_NONE;
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		return ret;
	}
	memset((void *)&video_out_param,0,sizeof(HD_VIDEOOUT_IN));
	ret = hd_videoout_get(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		return ret;
	}
	printf("##video_out_param w:%d,h:%d %x %x\r\n", video_out_param.dim.w, video_out_param.dim.h, video_out_param.pxlfmt, video_out_param.dir);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////


static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_init()) != HD_OK)
		return ret;
#if	TEST_WITH_LIVIEW
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoout_init()) != HD_OK)
		return ret;
#endif
	return HD_OK;
}

static HD_RESULT open_module(VDOCAP_TEST *p_stream, UINT32 out_type)
{
	HD_RESULT ret;
	// set videocap config
	ret = set_cap_cfg(p_stream);
	if (ret != HD_OK) {
		printf("set cap-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	if ((ret = hd_videocap_open(HD_VIDEOCAP_IN(SEN1_VCAP_ID, 0), HD_VIDEOCAP_OUT(SEN1_VCAP_ID, 0), &p_stream->cap_path)) != HD_OK)
		return ret;

#if	TEST_WITH_LIVIEW
	// set videoproc config
	ret = set_proc_cfg(p_stream);
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoout config
	ret = set_out_cfg(&p_stream->out_ctrl, out_type,p_stream->hdmi_id);
	if (ret != HD_OK) {
		printf("set out-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0, &p_stream->out_path)) != HD_OK)
		return ret;
#endif
	return HD_OK;
}

static HD_RESULT close_module(VDOCAP_TEST *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_close(p_stream->cap_path)) != HD_OK)
		return ret;
#if	TEST_WITH_LIVIEW
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_close(p_stream->out_path)) != HD_OK)
		return ret;
#endif
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_uninit()) != HD_OK)
		return ret;
#if	TEST_WITH_LIVIEW
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoout_uninit()) != HD_OK)
		return ret;
#endif
	return HD_OK;
}

static void vdocap_update_sensor_info(SENSOR_TYPE sen_type, VDOCAP_TEST *p_vcap_test)
{
	UINT32 i;

	for (i = 0; i < HD_VIDEOCAP_SEN_SER_MAX_DATALANE; i++) {
		p_vcap_test->cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[i] = HD_VIDEOCAP_SEN_IGNORE;
	}
	p_vcap_test->cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	p_vcap_test->cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	p_vcap_test->cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	p_vcap_test->cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	p_vcap_test->blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H,HD_VIDEO_PXLFMT_RAW16)
        													+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
        													+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
    p_vcap_test->iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB;
	switch(sen_type){
	case SENSOR_IMX290_HDR:
		p_vcap_test->iq_ctl.func |= HD_VIDEOCAP_FUNC_SHDR;
		p_vcap_test->blk_cnt = 4;
		p_vcap_test->out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_2;
		p_vcap_test->cap_pxlfmt = HD_VIDEO_PXLFMT_RAW12_SHDR2;
		p_vcap_test->cap_cfg.sen_cfg.shdr_map = HD_VIDEOCAP_SHDR_MAP(HD_VIDEOCAP_HDR_SENSOR1, (HD_VIDEOCAP_0|HD_VIDEOCAP_1));
		p_vcap_test->cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;
		snprintf(p_vcap_test->cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
		p_vcap_test->cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
		p_vcap_test->cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
		p_vcap_test->cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
		p_vcap_test->cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
	break;
	case SENSOR_IMX290_LINEAR:
	default:
		p_vcap_test->blk_cnt = 2;
		p_vcap_test->out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
		p_vcap_test->cap_pxlfmt = HD_VIDEO_PXLFMT_RAW12;
		p_vcap_test->cap_cfg.sen_cfg.shdr_map = 0;
		p_vcap_test->cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x301;
		snprintf(p_vcap_test->cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
		p_vcap_test->cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
		p_vcap_test->cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
	break;
	}
}
static int vdocap_open(void)
{
	int ret;

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		return ret;
	}

	//vdocap_update_sensor_info(SENSOR_TYPE sen_type, &stream[0]);
	// init memory
	ret = mem_init(stream[0].blk_cnt, stream[0].blk_size);
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		return ret;
	}

	// init all modules
	ret = init_module();
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		return ret;
	}

	stream[0].hdmi_id=HD_VIDEOOUT_HDMI_1920X1080I60;//default
	// open VDOCAP_TEST modules (main)
	stream[0].proc_max_dim.w = VDOPRC_MAX_DIM_W; //assign by user
	stream[0].proc_max_dim.h = VDOPRC_MAX_DIM_H; //assign by user
	ret = open_module(&stream[0], VDO_OUTPUT_TYPE);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		return ret;
	}

	// get videocap capability
	ret = get_cap_caps(stream[0].cap_ctrl, &stream[0].cap_syscaps);
	if (ret != HD_OK) {
		printf("get cap-caps fail=%d\n", ret);
		return ret;
	}
#if	TEST_WITH_LIVIEW
	// get videoout capability
	ret = get_out_caps(stream[0].out_ctrl, &stream[0].out_syscaps);
	if (ret != HD_OK) {
		printf("get out-caps fail=%d\n", ret);
		return ret;
	}
	stream[0].out_max_dim = stream[0].out_syscaps.output_dim;
#endif
	// set videocap parameter
	stream[0].cap_dim.w = VDO_SIZE_W; //assign by user
	stream[0].cap_dim.h = VDO_SIZE_H; //assign by user
	ret = set_cap_param(&stream[0]);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		return ret;
	}

#if	TEST_WITH_LIVIEW
	// set videoproc parameter (main)
	ret = set_proc_param(stream[0].proc_path, NULL);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		return ret;
	}

	// set videoout parameter (main)
	stream[0].out_dim.w = stream[0].out_max_dim.w; //using device max dim.w
	stream[0].out_dim.h = stream[0].out_max_dim.h; //using device max dim.h
	ret = set_out_param(stream[0].out_path, &stream[0].out_dim);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		return ret;
	}
	// bind video_liveview modules (main)
	hd_videocap_bind(HD_VIDEOCAP_OUT(SEN1_VCAP_ID, 0), HD_VIDEOPROC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOOUT_0_IN_0);
#endif
	return ret;

}
static int vdocap_close(void)
{
	int ret;
#if	TEST_WITH_LIVIEW
	// unbind video_liveview modules (main)
	hd_videocap_unbind(HD_VIDEOCAP_OUT(SEN1_VCAP_ID, 0));
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);
#endif

	// close video_liveview modules (main)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

	// uninit all modules
	ret = exit_module();
	if (ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}

	// uninit memory
	ret = mem_exit();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
	}

	// uninit hdal
	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
	}
	return ret;
}
static int vdocap_start(void)
{
	int ret;
	// start video_liveview modules (main)
	ret = hd_videocap_start(stream[0].cap_path);
	if (ret != HD_OK) {
		printf("start cap fail=%d\n", ret);
		return ret;
	}
#if	TEST_WITH_LIVIEW
	ret = hd_videoproc_start(stream[0].proc_path);
	if (ret != HD_OK) {
		printf("start proc fail=%d\n", ret);
		return ret;
	}
	ret = hd_videoout_start(stream[0].out_path);
	if (ret != HD_OK) {
		printf("start out fail=%d\n", ret);
		return ret;
	}
#endif
	return 0;
}
static int vdocap_stop(void)
{
	int ret;
	// stop video_liveview modules (main)
	ret = hd_videocap_stop(stream[0].cap_path);
	if (ret != HD_OK) {
		printf("stop cap fail=%d\n", ret);
		return ret;
	}
#if	TEST_WITH_LIVIEW
	ret = hd_videoproc_stop(stream[0].proc_path);
	if (ret != HD_OK) {
		printf("stop proc fail=%d\n", ret);
		return ret;
	}
	ret = hd_videoout_stop(stream[0].out_path);
	if (ret != HD_OK) {
		printf("stop out fail=%d\n", ret);
		return ret;
	}
#endif
	return 0;
}

static int vdocap_flow_open(void)
{
	if (vdocap_open()) {
		printf("open failed\r\n");
	}
	return 0;
}
static int vdocap_flow_start(void)
{
	if (vdocap_start()) {
		printf("start failed\r\n");
	}
	return 0;
}
static int vdocap_flow_stop(void)
{
	if (vdocap_stop()) {
		printf("stop failed\r\n");
	}
	return 0;
}
static int vdocap_flow_close(void)
{
	if (vdocap_close()) {
		printf("close failed\r\n");
	}
	return 0;
}

static int vdocap_set_sensor_type(SENSOR_TYPE param)
{
	if (param >= SENSOR_TYPE_MAX) {
		printf("error param =%d\r\n", param);
		return 0;
	}
	vdocap_update_sensor_info(param, &stream[0]);
	return 0;
}
static int vdocap_sensor_imx290_linear(void)
{
	vdocap_set_sensor_type(SENSOR_IMX290_LINEAR);
	return 0;
}
static int vdocap_sensor_imx290_hdr(void)
{
	vdocap_set_sensor_type(SENSOR_IMX290_HDR);
	return 0;
}
static HD_DBG_MENU vdocap_sensor_select_menu[] = {
	{0x01, "IMX290_LINEAR",     vdocap_sensor_imx290_linear,         TRUE},
	{0x02, "IMX290_HDR",    vdocap_sensor_imx290_hdr,        TRUE},
	// escape muse be last
	{-1,   "",         NULL,               FALSE},
};
static int vdocap_sensor_select(void)
{
	return hd_debug_menu_entry_p(vdocap_sensor_select_menu, "VDOCAP SELECT SENSOR");
}
static HD_DBG_MENU vdocap_basic_flow_menu[] = {
	{0x00, "sensor selection",    vdocap_sensor_select,        TRUE},
	{0x01, "open",     vdocap_flow_open,         TRUE},
	{0x02, "start",    vdocap_flow_start,        TRUE},
	{0x03, "stop",     vdocap_flow_stop,         TRUE},
	{0x04, "close",    vdocap_flow_close,        TRUE},
	// escape muse be last
	{-1,   "",         NULL,               FALSE},
};

static int vdocap_basic_flow(void)
{
	return hd_debug_menu_entry_p(vdocap_basic_flow_menu, "VDOCAP BASIC FLOW");
}

typedef enum _MODE_CHANGE {
	MODE_1080P30_LINEAR = 0,
	MODE_720P30_LINEAR,
	MODE_VGAP30_LINEAR,
	MODE_0_LINEAR,
	MODE_1_LINEAR,
	MODE_2_LINEAR,
	MODE_CHANGE_MAX,
	ENUM_DUMMY4WORD(MODE_CHANGE)
} MODE_CHANGE;

static HD_VIDEOCAP_IN vdocap_in_param[] = {
	{HD_VIDEOCAP_SEN_MODE_AUTO, HD_VIDEO_FRC_RATIO(30,1), {1920,1080}, HD_VIDEO_PXLFMT_RAW10, HD_VIDEOCAP_SEN_FRAME_NUM_1},
	{HD_VIDEOCAP_SEN_MODE_AUTO, HD_VIDEO_FRC_RATIO(30,1), {1280,720}, HD_VIDEO_PXLFMT_RAW10, HD_VIDEOCAP_SEN_FRAME_NUM_1},
	{HD_VIDEOCAP_SEN_MODE_AUTO, HD_VIDEO_FRC_RATIO(30,1), {640,480}, HD_VIDEO_PXLFMT_RAW10, HD_VIDEOCAP_SEN_FRAME_NUM_1},
	{0, HD_VIDEO_FRC_RATIO(30,1), {0}, HD_VIDEO_PXLFMT_RAW10, HD_VIDEOCAP_SEN_FRAME_NUM_1},
	{1, HD_VIDEO_FRC_RATIO(30,1), {0}, HD_VIDEO_PXLFMT_RAW10, HD_VIDEOCAP_SEN_FRAME_NUM_1},
	{2, HD_VIDEO_FRC_RATIO(30,1), {0}, HD_VIDEO_PXLFMT_RAW10, HD_VIDEOCAP_SEN_FRAME_NUM_1}
};
static int vdocap_set_mode(MODE_CHANGE param)
{
	int ret;

	if (param >= (sizeof(vdocap_in_param)/sizeof(HD_VIDEOCAP_IN))) {
		printf("error param =%d\r\n", param);
		return 0;
	}
	//printf("vdocap_in_param[%d] mode=0x%X, dim=%dx%d\r\n", param, vdocap_in_param[param].sen_mode, vdocap_in_param[param].dim.w, vdocap_in_param[param].dim.h);
	vdocap_stop();
	ret = hd_videocap_set(stream[0].cap_path, HD_VIDEOCAP_PARAM_IN, &vdocap_in_param[param]);
	if (ret != HD_OK) {
		printf("set HD_VIDEOCAP_PARAM_IN fail=%d\n", ret);
	} else {
		char cmd[256];

		if (HD_OK == vdocap_start()) {
			sleep(1);
			snprintf(cmd, 256, "echo dbg 0 0x00000001 > /proc/ctl_sen/cmd");
			system(cmd);
		}
	}
	return 0;
}
static int vdocap_mode_1080p30(void)
{
	vdocap_set_mode(MODE_1080P30_LINEAR);
	return 0;
}
static int vdocap_mode_720p30(void)
{
	vdocap_set_mode(MODE_720P30_LINEAR);
	return 0;
}
static int vdocap_mode_vgap30(void)
{
	vdocap_set_mode(MODE_VGAP30_LINEAR);
	return 0;
}
static int vdocap_mode_0(void)
{
	vdocap_set_mode(MODE_0_LINEAR);
	return 0;
}
static int vdocap_mode_1(void)
{
	vdocap_set_mode(MODE_1_LINEAR);
	return 0;
}
static int vdocap_mode_2(void)
{
	vdocap_set_mode(MODE_2_LINEAR);
	return 0;
}
static HD_DBG_MENU vdocap_change_mode_menu[] = {
	{0x01, "1080P30",  vdocap_mode_1080p30,       TRUE},
	{0x02, "720P30",   vdocap_mode_720p30,        TRUE},
	{0x03, "VGA",      vdocap_mode_vgap30,        TRUE},
	{0x04, "MODE 0",   vdocap_mode_0,        TRUE},
	{0x05, "MODE 1",   vdocap_mode_1,        TRUE},
	{0x06, "MODE 2",   vdocap_mode_2,        TRUE},

	// escape muse be last
	{-1,   "",         NULL,               FALSE},
};
static int vdocap_change_mode(void)
{
	return hd_debug_menu_entry_p(vdocap_change_mode_menu, "VDOCAP CHANGE MODE");
}

typedef enum _CROP_ITEM {
	CROP_OFF = 0,
	CROP_0_0_960_540,
	CROP_480_270_960_540,
	CROP_960_540_960_540,
	CROP_AUTO_16_9_600_CENTER,
	CROP_AUTO_16_9_300_CENTER,
	CROP_AUTO_4_3_500_CENTER,
	CROP_0_0_2000_2000,
	CROP_N1_N1_300_300,
	CROP_11_73_133_143,
	AUTO_16_9_1100_CENTER,
	ENUM_DUMMY4WORD(CROP_ITEM)
} CROP_ITEM;

static HD_VIDEOCAP_CROP vdocap_crop_param[] = {
	{HD_CROP_OFF, {0}, {0}, {0}},
	{HD_CROP_ON, {{0},{0,0,960,540}},{0},{0}},
	{HD_CROP_ON, {{0},{480,270,960,540}},{0},{0}},
	{HD_CROP_ON, {{0},{960,540,960,540}},{0},{0}},
	{HD_CROP_AUTO, {0},{HD_VIDEOCAP_RATIO(16,9),600,{960,540}},{0}},
	{HD_CROP_AUTO, {0},{HD_VIDEOCAP_RATIO(16,9),300,{960,540}},{0}},
	{HD_CROP_AUTO, {0},{HD_VIDEOCAP_RATIO(4,3),500,{960,540}},{0}},
	{HD_CROP_ON, {{0},{0,0,2000,2000}},{0},{0}},
	{HD_CROP_ON, {{0},{-1,-1,300,300}},{0},{0}},
	{HD_CROP_ON, {{0},{11,73,133,143}},{0},{0}},
	{HD_CROP_AUTO, {0},{HD_VIDEOCAP_RATIO(16,9),1100,{960,540}},{0}},
};
static int vdocap_set_crop(CROP_ITEM param)
{
	int ret;

	if (param >= (sizeof(vdocap_crop_param)/sizeof(HD_VIDEOCAP_CROP))) {
		printf("error param =%d\r\n", param);
		return 0;
	}

	ret = hd_videocap_set(stream[0].cap_path, HD_VIDEOCAP_PARAM_OUT_CROP, &vdocap_crop_param[param]);
	if (ret != HD_OK) {
		printf("set HD_VIDEOCAP_PARAM_OUT_CROP fail=%d\n", ret);
	} else {
		char cmd[256];

		if (HD_OK == vdocap_start()) {
			snprintf(cmd, 256, "echo dbgtype 0 1 > /proc/nvt_ctl_sie/cmd");
			system(cmd);
		}
	}
	return 0;
}

static int vdocap_crop_off(void)
{
	vdocap_set_crop(CROP_OFF);
	return 0;
}
static int vdocap_crop_0_0_960_540(void)
{
	vdocap_set_crop(CROP_0_0_960_540);
	return 0;
}
static int vdocap_crop_480_270_960_540(void)
{
	vdocap_set_crop(CROP_480_270_960_540);
	return 0;
}
static int vdocap_crop_960_540_960_540(void)
{
	vdocap_set_crop(CROP_960_540_960_540);
	return 0;
}
static int vdocap_crop_auto_16_9_600_CENTER(void)
{
	vdocap_set_crop(CROP_AUTO_16_9_600_CENTER);
	return 0;
}
static int vdocap_crop_auto_16_9_300_CENTER(void)
{
	vdocap_set_crop(CROP_AUTO_16_9_300_CENTER);
	return 0;
}
static int vdocap_crop_auto_4_3_500_CENTER(void)
{
	vdocap_set_crop(CROP_AUTO_4_3_500_CENTER);
	return 0;
}
static int vdocap_crop_0_0_2000_200(void)
{
	vdocap_set_crop(CROP_0_0_2000_2000);
	return 0;
}
static int vdocap_crop_N1_N1_300_300(void)
{
	vdocap_set_crop(CROP_N1_N1_300_300);
	return 0;
}
static int vdocap_crop_11_73_133_143(void)
{
	vdocap_set_crop(CROP_11_73_133_143);
	return 0;
}
static int vdocap_crop_auto_16_9_1100_CENTER(void)
{
	vdocap_set_crop(AUTO_16_9_1100_CENTER);
	return 0;
}
static HD_DBG_MENU vdocap_crop_menu[] = {
	{0x01, "OFF",              vdocap_crop_off,       TRUE},
	{0x02, "0_0_960_540", vdocap_crop_0_0_960_540,        TRUE},
	{0x03, "480_270_960_540", vdocap_crop_480_270_960_540,        TRUE},
	{0x04, "960_540_960_540", vdocap_crop_960_540_960_540,        TRUE},
	{0x05, "AUTO_16_9_600_CENTER", vdocap_crop_auto_16_9_600_CENTER,        TRUE},
	{0x06, "AUTO_16_9_300_CENTER", vdocap_crop_auto_16_9_300_CENTER,        TRUE},
	{0x07, "AUTO_4_3_500_CENTER", vdocap_crop_auto_4_3_500_CENTER,        TRUE},
	{0x08, "0_0_2000_2000", vdocap_crop_0_0_2000_200,        TRUE},
	{0x09, "-1_-1_300_300", vdocap_crop_N1_N1_300_300,        TRUE},
	{0x0A, "11_73_133_143", vdocap_crop_11_73_133_143,        TRUE},
	{0x0B, "AUTO_16_9_1100_CENTER", vdocap_crop_auto_16_9_1100_CENTER,        TRUE},

	// escape muse be last
	{-1,   "",         NULL,               FALSE},
};
static int vdocap_crop(void)
{
	return hd_debug_menu_entry_p(vdocap_crop_menu, "VDOCAP CROP");
}

typedef enum _OUT_ITEM {
	OUT_NO_SCALING_RAW8 = 0,
	OUT_NO_SCALING_RAW10,
	OUT_NO_SCALING_RAW12,
	OUT_NO_SCALING_RAW16,
	OUT_NO_SCALING_NRX12,
	OUT_640_360,
	OUT_NO_SCALING_RAW14,
	OUT_NO_SCALING_NRX8,
	ENUM_DUMMY4WORD(OUT_ITEM)
} OUT_ITEM;

static HD_VIDEOCAP_OUT vdocap_out_param[] = {
	{{0}, HD_VIDEO_PXLFMT_RAW8, HD_VIDEO_DIR_NONE, 0, 0},
	{{0}, HD_VIDEO_PXLFMT_RAW10, HD_VIDEO_DIR_NONE, 0, 0},
	{{0}, HD_VIDEO_PXLFMT_RAW12, HD_VIDEO_DIR_NONE, 0, 0},
	{{0}, HD_VIDEO_PXLFMT_RAW16, HD_VIDEO_DIR_NONE, 0, 0},
	{{0}, HD_VIDEO_PXLFMT_NRX12, HD_VIDEO_DIR_NONE, 0, 0},
	{{640,360}, CAP_OUT_FMT, HD_VIDEO_DIR_NONE, 0, 0},
	{{0}, HD_VIDEO_PXLFMT_RAW14, HD_VIDEO_DIR_NONE, 0, 0},
	{{0}, HD_VIDEO_PXLFMT_NRX8, HD_VIDEO_DIR_NONE, 0, 0},
};
static int vdocap_set_out(OUT_ITEM param)
{
	int ret;
	static HD_VIDEO_PXLFMT pxlfmt = CAP_OUT_FMT;

	if (param >= (sizeof(vdocap_out_param)/sizeof(HD_VIDEOCAP_OUT))) {
		printf("error param =%d\r\n", param);
		return 0;
	}
	if (param <= OUT_NO_SCALING_NRX12) {
		HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};

		//pixel format change need to stop/start vdoprc
		vdocap_stop();
		pxlfmt = vdocap_out_param[param].pxlfmt;
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWALL;
		video_cfg_param.in_max.dim.w = VDOPRC_MAX_DIM_W;
		video_cfg_param.in_max.dim.h = VDOPRC_MAX_DIM_H;
		video_cfg_param.in_max.pxlfmt = pxlfmt;
		video_cfg_param.in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(stream[0].proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &video_cfg_param);
		if (ret != HD_OK) {
			printf("set HD_VIDEOPROC_PARAM_DEV_CONFIG fail=%d\n", ret);
		}

	} else if (param == OUT_640_360) {
		vdocap_out_param[param].pxlfmt = pxlfmt;
	}
	ret = hd_videocap_set(stream[0].cap_path, HD_VIDEOCAP_PARAM_OUT, &vdocap_out_param[param]);
	if (ret != HD_OK) {
		printf("set HD_VIDEOCAP_PARAM_OUT fail=%d\n", ret);
	} else {
		char cmd[256];

		if (HD_OK == vdocap_start()) {
			snprintf(cmd, 256, "echo dbgtype 0 1 > /proc/nvt_ctl_sie/cmd");
			system(cmd);
		}
	}
	return 0;
}

static int vdocap_out_no_scale_raw8(void)
{
	vdocap_set_out(OUT_NO_SCALING_RAW8);
	return 0;
}
static int vdocap_out_no_scale_raw10(void)
{
	vdocap_set_out(OUT_NO_SCALING_RAW10);
	return 0;
}
static int vdocap_out_no_scale_raw12(void)
{
	vdocap_set_out(OUT_NO_SCALING_RAW12);
	return 0;
}
static int vdocap_out_no_scale_raw16(void)
{
	vdocap_set_out(OUT_NO_SCALING_RAW16);
	return 0;
}
static int vdocap_out_no_scale_nrx12(void)
{
	vdocap_set_out(OUT_NO_SCALING_NRX12);
	return 0;
}
static int vdocap_out_640_360(void)
{
	vdocap_set_out(OUT_640_360);
	return 0;
}
static int vdocap_out_no_scale_raw14(void)
{
	vdocap_set_out(OUT_NO_SCALING_RAW14);
	return 0;
}
static int vdocap_out_no_scale_nrx8(void)
{
	vdocap_set_out(OUT_NO_SCALING_NRX8);
	return 0;
}
static HD_DBG_MENU vdocap_out_menu[] = {
	{0x01, "NO Scaling_RAW8", vdocap_out_no_scale_raw8,       TRUE},
	{0x02, "NO Scaling_RAW10", vdocap_out_no_scale_raw10,       TRUE},
	{0x03, "NO Scaling_RAW12", vdocap_out_no_scale_raw12,       TRUE},
	{0x04, "NO Scaling_RAW16", vdocap_out_no_scale_raw16,       TRUE},
	{0x05, "NO Scaling_NRX12", vdocap_out_no_scale_nrx12,       TRUE},
	{0x06, "OUT_640_360", vdocap_out_640_360,        TRUE},
	{0x07, "NO Scaling_RAW14", vdocap_out_no_scale_raw14,       TRUE},
	{0x08, "NO Scaling_NRX8", vdocap_out_no_scale_nrx8,       TRUE},
	// escape muse be last
	{-1,   "",         NULL,               FALSE},
};
static int vdocap_out(void)
{
	return hd_debug_menu_entry_p(vdocap_out_menu, "VDOCAP OUT");
}
EXAMFUNC_ENTRY(videocap_test, argc, argv)
{
	INT key;

	//default IMX290 linear
	printf("Default using IMX290 Linear\r\n");
	vdocap_update_sensor_info(SENSOR_IMX290_LINEAR, &stream[0]);

	hd_debug_menu_entry_p(vdocap_test_main_menu, "VDOCAP TESTKIT MAIN");

	// query user key
	printf("Enter q to exit, t to VCAP TEST menu\n");
	while (1) {
		key = NVT_EXAMSYS_GETCHAR();
		if (key == 'q' || key == 0x3) {
			// quit program
			break;
		}

		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			// enter debug menu
			hd_debug_run_menu();
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
		if (key == '0') {
			get_cap_sysinfo(stream[0].cap_ctrl);
		}
		if (key == 't') {
			hd_debug_menu_entry_p(vdocap_test_main_menu, "VDOCAP TESTKIT MAIN");
		}
	}

	return 0;
}
