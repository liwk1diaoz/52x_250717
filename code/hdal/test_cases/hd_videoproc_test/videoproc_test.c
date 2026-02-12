/**
	@brief Sample code of video snapshot from proc to frame.\n

	@file video_process_only.c

	@author Jeah Yen

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_debug.h"
#include "hd_util.h"
#include "videoproc_test_int.h"

// platform dependent
#if defined(__LINUX)
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#define GETCHAR()				getchar()
#else
#include <kwrap/examsys.h> 	//for MAIN(), GETCHAR() API
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_videoproc_test, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#if defined(__LINUX)
#else
#include <kwrap/cmdsys.h> 		//for cat info
#endif


// how to provide data? => 	test: set dim of data to push (v)
//							test: set pxlfmt of data to push (v)
//							push: auto gen data with test dim and test pxlfmt (TODO)

// how to provide frc? => 	test: set fps of data to push (TODO)
//							push: auto push data with interval time of test fps (TODO)

// how to verify data? => 	test set dim of data to pull (v)
//							test set pxlfmt of data to pull (v)
//							test set content code of data to pull (TODO)
//                          pull: verify pxlfmt of data with test dim (TODO)
//                          pull: verify dim of data with test pxlfmt (TODO)
//							pull: encrypt data to code, verify with test code (TODO)

// how to verify frc? => 	test: set fps of data to pull (TODO)
//							pull: calculate fps of data, verify with test fps (TODO)

// how to verify invalid state?
// how to verify invalid param?
// how to verify invalid data?

//TODO: how to test extend path? => start main path

//TODO: how to test func SHDR? => delay to INTEGRATION TEST
//TODO: how to test crop AUTO? => delay to INTEGRATION TEST

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

#define VDO_MAX_SIZE_W     5120
#define VDO_MAX_SIZE_H     2880


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
	int      	  (*p_func)(void);  ///< command function
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

static HD_DBG_MENU *_cur_menu = NULL;

static const char* hd_debug_get_menu_name(void)
{
	if (_cur_menu == NULL) return "x";
	else return _cur_menu->p_name;
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
				int r;
				_cur_menu = p_menu;
				r = p_menu->p_func();
				_cur_menu = NULL;
				return r;
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
/*
[HDAL/VideoProc] state open/start/stop/close test
[HDAL/VideoProc] param func test
[HDAL/VideoProc] param input pxlfmt test (default)
[HDAL/VideoProc] param input dim test (default)
[HDAL/VideoProc] param input crop test
[HDAL/VideoProc] param input dir test
[HDAL/VideoProc] param input frc test
[HDAL/VideoProc] param output pxlfmt test
[HDAL/VideoProc] param output crop test
[HDAL/VideoProc] param output dir test
[HDAL/VideoProc] param output dim test
[HDAL/VideoProc] param output frc test
*/

//cfg
static HD_VIDEOPROC_DEV_CONFIG _test_cfg[5] = {
	{HD_VIDEOPROC_PIPE_RAWALL, HD_ISP_DONT_CARE, {0, 0}, {0, {VDO_MAX_SIZE_W, VDO_MAX_SIZE_H}, HD_VIDEO_PXLFMT_RAW12, 0, 0}},
};

//in
static HD_VIDEOPROC_IN _test_in[3] = {
	{0, {1920, 1080}, HD_VIDEO_PXLFMT_RAW12, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1)},
	{0, {640, 360}, HD_VIDEO_PXLFMT_RAW12, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1)},
	{0, {160, 88}, HD_VIDEO_PXLFMT_RAW12, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1)},
	//{0, {1920, 1080}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1)},
};


//out
static HD_VIDEOPROC_OUT _test_out[2] = {
	{0, {1920, 1080}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1), 1},
	{0, {1920, 1080}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1), 1},
};
/*
//out_ex
static HD_VIDEOPROC_OUT_EX out_ex[2] = {
	{0, {1920, 1080}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1), 1},
	{0, {1920, 1080}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1), 1},
};
*/
static VIDEO_PROCESS stream[1] = {0}; //0: main stream
static UINT32 test_pull_type = 1;

static int proc_test_flow_init(void)
{
	HD_RESULT ret;

	UINT32 pull_type = test_pull_type;
	stream[0].show_ret = 0; //show pull result
	if (pull_type == 0) {
		stream[0].wait_ms = 0;
		printf("pull(%d) with non-blocking\r\n", stream[0].wait_ms);
	} else if (pull_type == 1) {
		stream[0].wait_ms = 500;
		printf("pull(%d) with blocking-timeout\r\n", stream[0].wait_ms);
	} else {
		stream[0].wait_ms = -1;
		printf("pull(%d) with blocking\r\n", stream[0].wait_ms);
	}

	// init hdal
	ret = hd_common_init(0); if (ret < 0) { printf("common init failed=%d\n", ret);	}
	// init memory
	{
		INT32 i = 0;
		HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

		// config common pool (main)
		i = push_query_mem(&stream[0], &mem_cfg, i);
		i = proc_query_mem(&stream[0], &mem_cfg, i);
		i = pull_query_mem(&stream[0], &mem_cfg, i);

		ret = hd_common_mem_init(&mem_cfg); if (ret < 0) { printf("mem init failed=%d\n", ret);	}
	}
	// init
	ret = push_init(); if (ret < 0) { printf("push init failed=%d\n", ret);	}
	ret = pull_init(); if (ret < 0) { printf("pull init failed=%d\n", ret);	}
	ret = proc_init(); if (ret < 0) { printf("this init failed=%d\n", ret); }
	return 1;
}
static int proc_test_flow_open(void)
{
	HD_RESULT ret;
	// set config
	if (stream[0].pipe == HD_VIDEOPROC_PIPE_RAWALL) {
		if (stream[0].func == HD_VIDEOPROC_FUNC_SHDR) {
			_test_cfg[0].in_max.pxlfmt = HD_VIDEO_PXLFMT_RAW12_SHDR2;
		} else {
			_test_cfg[0].in_max.pxlfmt = HD_VIDEO_PXLFMT_RAW12;
		}
	}
	if (stream[0].pipe == HD_VIDEOPROC_PIPE_YUVALL) {
			_test_cfg[0].in_max.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	}
	if (stream[0].pipe == HD_VIDEOPROC_PIPE_COLOR) {
			_test_cfg[0].in_max.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	}
	if (stream[0].pipe == HD_VIDEOPROC_PIPE_SCALE) {
			_test_cfg[0].in_max.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	}
	if (stream[0].pipe == HD_VIDEOPROC_PIPE_DEWARP) {
			_test_cfg[0].in_max.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	}
	if (stream[0].pipe == 0xF2 /*HD_VIDEOPROC_PIPE_VPE*/) {
			_test_cfg[0].in_max.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	}
	ret = proc_set_config(&stream[0], &_test_cfg[0]); if (ret < 0) { printf("this config failed=%d\n", ret); }
	// open
	ret = proc_open(&stream[0]); if (ret < 0) { printf("this open failed=%d\n", ret); }
	ret = push_open(&stream[0]); if (ret < 0) {	printf("push open failed=%d\n", ret); }
	ret = pull_open(&stream[0]); if (ret < 0) {	printf("pull open failed=%d\n", ret); }
	return 1;
}
static int proc_test_flow_start(void)
{
	HD_RESULT ret;
	// set parameter
	_test_in[stream[0].input_dim].pxlfmt = _test_cfg[0].in_max.pxlfmt;
	ret = push_set_out(&stream[0], &_test_in[stream[0].input_dim]); if (ret < 0) { printf("push set failed=%d\n", ret); }
	ret = proc_set_in(&stream[0], &_test_in[stream[0].input_dim]); if (ret < 0) { printf("this set failed=%d\n", ret); }
	// set parameter
	ret = proc_set_out(&stream[0], &_test_out[stream[0].output_dim]); if (ret < 0) { printf("this set failed=%d\n", ret); }
	ret = pull_set_in(&stream[0], &_test_out[stream[0].output_dim]); if (ret < 0) { printf("pull set failed=%d\n", ret); }
	// start
	ret = proc_start(&stream[0]);
	ret = push_start(&stream[0]);
	ret = pull_start(&stream[0]);

	sleep(2);
	system("cat /proc/hdal/vprc/info");
	return 1;
}
static int proc_test_flow_stop(void)
{
	HD_RESULT ret;
	// stop
	ret = push_stop(&stream[0]); if (ret < 0) { printf("push stop failed=%d\n", ret); }
	ret = pull_stop(&stream[0]); if (ret < 0) { printf("pull stop failed=%d\n", ret); }
	ret = proc_stop(&stream[0]); if (ret < 0) { printf("this stop failed=%d\n", ret); }

	sleep(2);
	system("cat /proc/hdal/vprc/info");
	return 1;
}
static int proc_test_flow_close(void)
{
	HD_RESULT ret;
	// close video_process modules (main)
	ret = proc_close(&stream[0]); if (ret < 0) { printf("this close failed=%d\n", ret); }
	ret = push_close(&stream[0]); if (ret < 0) { printf("push close failed=%d\n", ret); }
	ret = pull_close(&stream[0]); if (ret < 0) { printf("pull close failed=%d\n", ret); }
	return 1;
}
static int proc_test_flow_uninit(void)
{
	HD_RESULT ret;
	// uninit all modules
	ret = proc_uninit(); if (ret < 0) { printf("this uninit failed=%d\n", ret); }
	ret = push_uninit(); if (ret < 0) { printf("push uninit failed=%d\n", ret); }
	ret = pull_uninit(); if (ret < 0) { printf("pull uninit failed=%d\n", ret); }
	// uninit memory
	ret = hd_common_mem_uninit(); if (ret < 0) { printf("mem uninit failed=%d\n", ret); }
	// uninit hdal
	ret = hd_common_uninit(); if (ret < 0) { printf("common uninit failed=%d\n", ret); }
	return 1;
}
static int proc_test_cat_info(void)
{
#if defined(__LINUX)
	system("cat /proc/hdal/vprc/info");
#else
	nvt_cmdsys_runcmd("vprc info");
	//nvt_examsys_runcmd("xxxxxxxxx");
#endif
	return 1;
}
static int proc_test_cat_mem(void)
{
#if defined(__LINUX)
	system("cat /proc/hdal/comm/info");
#else
	nvt_cmdsys_runcmd("comm info");
	//nvt_examsys_runcmd("xxxxxxxxx"); 
#endif
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////
static int proc_test_devcfg_pipe(UINT32 i)
{
	printf("*** set videoproc.devcfg_pipe = (%08x) ***\r\n", 
		i);

	stream[0].pipe = i;
	// set parameter
	return 1;
}

static int proc_test_devcfg_pipe_rawall(void)
{
	return proc_test_devcfg_pipe(HD_VIDEOPROC_PIPE_RAWALL); //RHE+IFE+DCE+IPE+IME //need HD_VIDEO_PXLFMT_RAW12
}
static int proc_test_devcfg_pipe_yuvall(void)
{
	return proc_test_devcfg_pipe(HD_VIDEOPROC_PIPE_YUVALL); //IPE+IME //need HD_VIDEO_PXLFMT_YUV420
}
static int proc_test_devcfg_pipe_color(void)
{
	return proc_test_devcfg_pipe(HD_VIDEOPROC_PIPE_COLOR); //IPE only //need HD_VIDEO_PXLFMT_YUV420
}
static int proc_test_devcfg_pipe_scale(void)
{
	return proc_test_devcfg_pipe(HD_VIDEOPROC_PIPE_SCALE); //IME only //need HD_VIDEO_PXLFMT_YUV420
}
static int proc_test_devcfg_pipe_dewarp(void)
{
	return proc_test_devcfg_pipe(HD_VIDEOPROC_PIPE_DEWARP); //DCE only //need HD_VIDEO_PXLFMT_YUV420
}
static int proc_test_devcfg_pipe_vpe(void)
{
	//HD_VIDEOPROC_PIPE_VPE
	return proc_test_devcfg_pipe(0xF2); //VPE only //need HD_VIDEO_PXLFMT_YUV420
}

static HD_DBG_MENU proc_menu_devcfg_pipe[] = {
	{0x01, "devcfg_pipe=rawall",  		proc_test_devcfg_pipe_rawall,	TRUE},
	{0x02, "devcfg_pipe=yuvall",		proc_test_devcfg_pipe_yuvall,	TRUE},
	{0x03, "devcfg_pipe=dewarp", 		proc_test_devcfg_pipe_dewarp,	TRUE},
	{0x04, "devcfg_pipe=color",  		proc_test_devcfg_pipe_color,	TRUE},
	{0x05, "devcfg_pipe=scale",  		proc_test_devcfg_pipe_scale,	TRUE},
	{0x06, "devcfg_pipe=vpe",  			proc_test_devcfg_pipe_vpe,	TRUE},
	// escape muse be last
	{-1,   "",               	NULL,               FALSE},
};

static int proc_test_list_devcfg_pipe(void)
{
	//select func, change when open
	return hd_debug_menu_entry_p(proc_menu_devcfg_pipe, "VDOPRC TEST DEVCFG_PIPE");
}

///////////////////////////////////////////////////////////////////////////////////

static int proc_test_ctrl_func(UINT32 i)
{
	printf("*** set videoproc.ctrl_func = (%08x) ***\r\n", 
		i);

	stream[0].func = i;
	stream[0].ref_path_3dnr = HD_VIDEOPROC_0_OUT_0;
	// set parameter
	return 1;
}

static int proc_test_ctrl_func_0(void)
{
	return proc_test_ctrl_func(0);
}
static int proc_test_ctrl_func_3dnr(void)
{
	return proc_test_ctrl_func(HD_VIDEOPROC_FUNC_3DNR);
}
static int proc_test_ctrl_func_colornr(void)
{
	return proc_test_ctrl_func(HD_VIDEOPROC_FUNC_COLORNR);
}
static int proc_test_ctrl_func_wdr(void)
{
	return proc_test_ctrl_func(HD_VIDEOPROC_FUNC_WDR);
}
static int proc_test_ctrl_func_shdr(void)
{
	return proc_test_ctrl_func(HD_VIDEOPROC_FUNC_SHDR); //need HD_VIDEO_PXLFMT_RAW12_SHDR2
}
static int proc_test_ctrl_func_defog(void)
{
	return proc_test_ctrl_func(HD_VIDEOPROC_FUNC_DEFOG);
}

static HD_DBG_MENU proc_menu_ctrl_func[] = {
	{0x01, "ctrl_func=0",  		proc_test_ctrl_func_0,	TRUE},
	//NR - noise reduction
	{0x02, "ctrl_func=3dnr",  		proc_test_ctrl_func_3dnr,	TRUE},
	{0x03, "ctrl_func=colornr",	proc_test_ctrl_func_colornr,	TRUE},
	//DR - dynamic range enhance
	{0x04, "ctrl_func=wdr",  		proc_test_ctrl_func_wdr,	TRUE},
	{0x05, "ctrl_func=shdr",  		proc_test_ctrl_func_shdr,	TRUE},  //need push 2 frames
	{0x06, "ctrl_func=defog",  	proc_test_ctrl_func_defog,	TRUE},
	// escape muse be last
	{-1,   "",               	NULL,               FALSE},
};

static int proc_test_list_ctrl_func(void)
{
	//select func, change when start
	return hd_debug_menu_entry_p(proc_menu_ctrl_func, "VDOPRC TEST CTRL_FUNC");
}

///////////////////////////////////////////////////////////////////////////////////

static int proc_test_list_input_fmt(void)
{
	//select fmt, input when push
	/*	
	HD_VIDEO_PXLFMT_RAW8			= 0x41080000, ///< 1 plane, pixel=RAW(w,h) x 8bits
	HD_VIDEO_PXLFMT_RAW10			= 0x410a0000, ///< 1 plane, pixel=RAW(w,h) x 10bits
	HD_VIDEO_PXLFMT_RAW12			= 0x410c0000, ///< 1 plane, pixel=RAW(w,h) x 12bits
	HD_VIDEO_PXLFMT_RAW14			= 0x410e0000, ///< 1 plane, pixel=RAW(w,h) x 14bits
	HD_VIDEO_PXLFMT_RAW16			= 0x41100000, ///< 1 plane, pixel=RAW(w,h) x 16bits
	HD_VIDEO_PXLFMT_NRX8			= 0xf1080000, ///< novatek-raw-compress-1 of RAW8
	HD_VIDEO_PXLFMT_NRX10			= 0xf10a0000, ///< novatek-raw-compress-1 of RAW10
	HD_VIDEO_PXLFMT_NRX12			= 0xf10c0000, ///< novatek-raw-compress-1 of RAW12
	HD_VIDEO_PXLFMT_NRX14			= 0xf10e0000, ///< novatek-raw-compress-1 of RAW14
	HD_VIDEO_PXLFMT_NRX16			= 0xf1100000, ///< novatek-raw-compress-1 of RAW16 (IPC NA51000 and NA51023 only)

	HD_VIDEO_PXLFMT_Y8			= 0x51080400, ///< 1 plane, pixel=Y(w,h)
	HD_VIDEO_PXLFMT_YUV400		= HD_VIDEO_PXLFMT_Y8,
	HD_VIDEO_PXLFMT_YUV420_PLANAR	= 0x530c0420, ///< 3 plane, pixel=Y(w,h), U(w/2,h/2), and V(w/2,h/2)
	HD_VIDEO_PXLFMT_YUV420		= 0x520c0420, ///< 2 plane, pixel=Y(w,h), UV(w/2,h/2), semi-planer format with U1V1
	HD_VIDEO_PXLFMT_YUV422_PLANAR	= 0x53100422, ///< 3 plane, pixel=Y(w,h), U(w/2,h), and V(w/2,h)
	HD_VIDEO_PXLFMT_YUV422		= 0x52100422, ///< 2 plane, pixel=Y(w,h), UV(w/2,h), semi-planer format with U1V1
	HD_VIDEO_PXLFMT_YUV422_ONE		= 0x51100422, ///< 1 plane, pixel=UYVY(w,h), packed format with Y2U1V1
	HD_VIDEO_PXLFMT_YUV444_PLANAR	= 0x53180444, ///< 3 plane, pixel=Y(w,h), U(w,h), and V(w,h)
	HD_VIDEO_PXLFMT_YUV444		= 0x52180444, ///< 2 plane, pixel=Y(w,h), UV(w,h), semi-planer format with U1V1
	HD_VIDEO_PXLFMT_YUV444_ONE		= 0x51180444, ///< 1 plane, pixel=YUV(w,h), packed format with Y1U1V1
	HD_VIDEO_PXLFMT_YUV420_NVX1_H264 = 0x610c0420, ///< novatek-yuv-compress-1 of YUV420 for h264 (IPC NA51000 only)
	HD_VIDEO_PXLFMT_YUV420_NVX1_H265 = 0x610c1420, ///< novatek-yuv-compress-1 of YUV420 for h265 (IPC NA51000 only)
	HD_VIDEO_PXLFMT_YUV420_NVX2	= 0x610c2420, ///< novatek-yuv-compress-2 of YUV420 (IPC NA51023 only)

	HD_VIDEO_PXLFMT_RGB888_PLANAR	= 0x23180888, ///< 3 plane, pixel=R(w,h), G(w,h), B(w,h)

	*/
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////

static int proc_test_output_id(UINT32 i)
{
	printf("*** set videoproc.out id = (%d) ***\r\n", 
		i);

	stream[0].out_id = i;
	// set parameter
	return 1;
}

static int proc_test_output_id_0(void)
{
	return proc_test_output_id(0);
}
static int proc_test_output_id_1(void)
{
	return proc_test_output_id(1);
}
static int proc_test_output_id_2(void)
{
	return proc_test_output_id(2);
}
static int proc_test_output_id_3(void)
{
	return proc_test_output_id(3);
}
static int proc_test_output_id_4(void)
{
	return proc_test_output_id(4);
}
static int proc_test_output_id_5(void)
{
	return proc_test_output_id(5);
}
static int proc_test_output_id_6(void)
{
	return proc_test_output_id(6);
}
static int proc_test_output_id_7(void)
{
	return proc_test_output_id(7);
}
static int proc_test_output_id_8(void)
{
	return proc_test_output_id(8);
}
static int proc_test_output_id_9(void)
{
	return proc_test_output_id(9);
}
static int proc_test_output_id_10(void)
{
	return proc_test_output_id(10);
}
static int proc_test_output_id_11(void)
{
	return proc_test_output_id(11);
}
static int proc_test_output_id_12(void)
{
	return proc_test_output_id(12);
}
static int proc_test_output_id_13(void)
{
	return proc_test_output_id(13);
}
static int proc_test_output_id_14(void)
{
	return proc_test_output_id(14);
}
static int proc_test_output_id_15(void)
{
	return proc_test_output_id(15);
}

static HD_DBG_MENU proc_menu_output_id[] = {
	{0x01, "out_id=0",  	proc_test_output_id_0,	TRUE},
	{0x02, "out_id=1",  	proc_test_output_id_1,	TRUE},
	{0x03, "out_id=2",  	proc_test_output_id_2,	TRUE},
	{0x04, "out_id=3",  	proc_test_output_id_3,	TRUE},
	{0x05, "out_id=4",  	proc_test_output_id_4,	TRUE},
	{0x06, "out_id=5",  	proc_test_output_id_5,	TRUE},
	{0x07, "out_id=6",  	proc_test_output_id_6,	TRUE},
	{0x08, "out_id=7",  	proc_test_output_id_7,	TRUE},
	{0x09, "out_id=8",  	proc_test_output_id_8,	TRUE},
	{0x0a, "out_id=9",  	proc_test_output_id_9,	TRUE},
	{0x0b, "out_id=10",  	proc_test_output_id_10,	TRUE},
	{0x0c, "out_id=11",  	proc_test_output_id_11,	TRUE},
	{0x0d, "out_id=12",  	proc_test_output_id_12,	TRUE},
	{0x0e, "out_id=13",  	proc_test_output_id_13,	TRUE},
	{0x0f, "out_id=14",  	proc_test_output_id_14,	TRUE},
	{0x10, "out_id=15",  	proc_test_output_id_15,	TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

static int proc_test_list_output_id(void)
{
	return hd_debug_menu_entry_p(proc_menu_output_id, "VDOPRC TEST OUT ID");
}

///////////////////////////////////////////////////////////////////////////////////

static int proc_test_input_dim(UINT32 i)
{
 	HD_VIDEOPROC_IN* p_in = &_test_in[i];
	printf("*** set videoproc.in dim = (%d,%d) ***\r\n", 
		p_in->dim.w, p_in->dim.h);

	stream[0].input_dim = i;
	// set parameter
	return 1;
}

static int proc_test_input_dim_1080(void)
{
	return proc_test_input_dim(0);
}

static int proc_test_input_dim_360(void)
{
	return proc_test_input_dim(1);
}

static int proc_test_input_dim_88(void)
{
	return proc_test_input_dim(2);
}

static HD_DBG_MENU proc_menu_input_dim[] = {
	{0x01, "in_dim=1920x1080",  	proc_test_input_dim_1080,	TRUE},
	{0x02, "in_dim=640x360",  		proc_test_input_dim_360,	TRUE},
	{0x03, "in_dim=160x88",  		proc_test_input_dim_88,	TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

static int proc_test_list_input_dim(void)
{
	return hd_debug_menu_entry_p(proc_menu_input_dim, "VDOPRC TEST IN DIM");
}

///////////////////////////////////////////////////////////////////////////////////

static HD_VIDEOPROC_CROP _test_in_crop[7] = {
	{HD_CROP_OFF, {{0, 0},{0,0,0,0}}},
	{HD_CROP_ON,  {{0, 0},{0,0,1920,1080}}},
	{HD_CROP_ON,  {{0, 0},{0,0,960,540}}},
	{HD_CROP_ON,  {{0, 0},{960,0,960,540}}},
	{HD_CROP_ON,  {{0, 0},{0,540,960,540}}},
	{HD_CROP_ON,  {{0, 0},{960,540,960,540}}},
	{HD_CROP_ON,  {{0, 0},{960/2,540/2,960,540}}},
//	{HD_CROP_AUTO, {{0, 0},{0,0,1920,1080}}},
};

static int proc_test_input_crop(UINT32 i)
{
	HD_RESULT ret;
	HD_VIDEOPROC_CROP* p_crop = &_test_in_crop[i];

	printf("*** set videoproc.out crop = (%d,%d,%d,%d) ***\r\n", 
		p_crop->win.rect.x, p_crop->win.rect.y, p_crop->win.rect.w, p_crop->win.rect.h);

	ret = proc_set_in_crop(&stream[0], p_crop);
	if (ret != HD_OK) {
		printf("run in crop %d fail\r\n", i);
	}
	ret = proc_start(&stream[0]);
	if (ret != HD_OK) {
		printf("start fail = %d\r\n", ret);
	}

	sleep(1);
	pull_snapshot(&stream[0], hd_debug_get_menu_name());
	return 1;
}

static int proc_test_input_crop_off(void)
{
	return proc_test_input_crop(0);
}
static int proc_test_input_crop_all(void)
{
	return proc_test_input_crop(1);
}
static int proc_test_input_crop_left_top(void)
{
	return proc_test_input_crop(2);
}
static int proc_test_input_crop_right_top(void)
{
	return proc_test_input_crop(3);
}
static int proc_test_input_crop_left_bottom(void)
{
	return proc_test_input_crop(4);
}
static int proc_test_input_crop_right_bottom(void)
{
	return proc_test_input_crop(5);
}
static int proc_test_input_crop_center(void)
{
	return proc_test_input_crop(6);
}

static HD_DBG_MENU proc_menu_input_crop[] = {
	{0x01, "in_crop=off",  			proc_test_input_crop_off,	TRUE},
	{0x02, "in_crop=all",  			proc_test_input_crop_all,	TRUE},
	{0x03, "in_crop=left_top",  	proc_test_input_crop_left_top,	TRUE},
	{0x04, "in_crop=right_top",  	proc_test_input_crop_right_top,	TRUE},
	{0x05, "in_crop=left_bottom",  	proc_test_input_crop_left_bottom,	TRUE},
	{0x06, "in_crop=right_bottom",  proc_test_input_crop_right_bottom,	TRUE},
	{0x07, "in_crop=center",  		proc_test_input_crop_center,	TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

static int proc_test_list_input_crop(void)
{
	return hd_debug_menu_entry_p(proc_menu_input_crop, "VDOPRC TEST IN CROP");
}

///////////////////////////////////////////////////////////////////////////////////

static HD_VIDEOPROC_IN _test_in_dir[4] = {
	{0, {1920, 1080}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1)},
	{0, {1920, 1080}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_MIRRORX, HD_VIDEO_FRC_RATIO(1,1)},
	{0, {1920, 1080}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_MIRRORY, HD_VIDEO_FRC_RATIO(1,1)},
	{0, {1920, 1080}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_MIRRORX|HD_VIDEO_DIR_MIRRORY, HD_VIDEO_FRC_RATIO(1,1)},
};

static int proc_test_input_dir(UINT32 i)
{
	HD_RESULT ret;
	HD_VIDEOPROC_IN* p_in = &_test_in_dir[i];
	printf("*** set videoproc.in dir = (%08x) ***\r\n", 
		p_in->dir);

	ret = proc_set_in(&stream[0], p_in);
	if (ret != HD_OK) {
		printf("run in dir %d fail\r\n", i);
	}
	ret = proc_start(&stream[0]);
	if (ret != HD_OK) {
		printf("start fail = %d\r\n", ret);
	}
	
	sleep(1);
	pull_snapshot(&stream[0], hd_debug_get_menu_name());
	return 1;
}

static int proc_test_input_dir_0(void)
{
	return proc_test_input_dir(0);
}
static int proc_test_input_dir_mirrorx(void)
{
	return proc_test_input_dir(1);
}
static int proc_test_input_dir_mirrory(void)
{
	return proc_test_input_dir(2);
}
static int proc_test_input_dir_mirrorxy(void)
{
	return proc_test_input_dir(3);
}

static HD_DBG_MENU proc_menu_input_dir[] = {
	{0x01, "in_dir=0",  			proc_test_input_dir_0,	TRUE},
	{0x02, "in_dir=mirr_x",  		proc_test_input_dir_mirrorx,	TRUE},
	{0x03, "in_dir=mirr_y",  		proc_test_input_dir_mirrory,	TRUE},
	{0x04, "in_dir=mirr_xy",  		proc_test_input_dir_mirrorxy,	TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

static int proc_test_list_input_dir(void)
{
	return hd_debug_menu_entry_p(proc_menu_input_dir, "VDOPRC TEST IN DIR");
}

///////////////////////////////////////////////////////////////////////////////////
/*
static int proc_test_input_pxlfmt(void)
{
	return 1;
}
*/

///////////////////////////////////////////////////////////////////////////////////

static int proc_test_input_frc(UINT32 dest)
{
	HD_RESULT ret;
	HD_VIDEOPROC_FRC video_in_param = {0};
	HD_VIDEOPROC_FRC* p_frc = &video_in_param;
	UINT32 vp_in_dest_fr, vp_in_src_fr;
	vp_in_src_fr = 30;
	vp_in_dest_fr = dest;
	printf("*** set videoproc.in frc = (%d/%d) ***\r\n", vp_in_dest_fr, vp_in_src_fr);
	p_frc->frc = HD_VIDEO_FRC_RATIO(vp_in_dest_fr, vp_in_src_fr);
	ret = proc_set_in_frc(&stream[0], p_frc);
	if (ret != HD_OK) {
		printf("run in frc %d fail\r\n", dest);
	}
	ret = proc_start(&stream[0]);
	if (ret != HD_OK) {
		printf("start fail = %d\r\n", ret);
	}
	
	sleep(2);
	system("cat /proc/hdal/vprc/info");
	return 1;
}

static int proc_test_input_frc_30(void)
{
	return proc_test_input_frc(30);
}
static int proc_test_input_frc_25(void)
{
	return proc_test_input_frc(25);
}
static int proc_test_input_frc_20(void)
{
	return proc_test_input_frc(20);
}
static int proc_test_input_frc_15(void)
{
	return proc_test_input_frc(15);
}
static int proc_test_input_frc_10(void)
{
	return proc_test_input_frc(10);
}
static int proc_test_input_frc_5(void)
{
	return proc_test_input_frc(5);
}
static int proc_test_input_frc_0(void)
{
	return proc_test_input_frc(0);
}

static HD_DBG_MENU proc_menu_input_frc[] = {
	{0x01, "in_frc=30_30",  		proc_test_input_frc_30,	TRUE},
	{0x02, "in_frc=25_30",          proc_test_input_frc_25,	TRUE},
	{0x03, "in_frc=20_30",          proc_test_input_frc_20,	TRUE},
	{0x04, "in_frc=15_30",          proc_test_input_frc_15,	TRUE},
	{0x05, "in_frc=10_30",          proc_test_input_frc_10,	TRUE},
	{0x06, "in_frc=5_30",          	proc_test_input_frc_5,	TRUE},
	{0x07, "in_frc=0_30",    		proc_test_input_frc_0,	TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

static int proc_test_list_input_frc(void)
{
	return hd_debug_menu_entry_p(proc_menu_input_frc, "VDOPRC TEST IN FRC");
}


///////////////////////////////////////////////////////////////////////////////////

static HD_VIDEOPROC_OUT _test_out_dir[6] = {
	{0, {1920, 1080}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1), 1},
	{0, {1080, 1920}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_ROTATE_90, HD_VIDEO_FRC_RATIO(1,1), 1},
	{0, {1080, 1920}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_ROTATE_270, HD_VIDEO_FRC_RATIO(1,1), 1},
	{0, {640, 360}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1), 1},
	{0, {360, 640}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_ROTATE_90, HD_VIDEO_FRC_RATIO(1,1), 1},
	{0, {360, 640}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_ROTATE_270, HD_VIDEO_FRC_RATIO(1,1), 1},
};

static int proc_test_output_dir(UINT32 i)
{
	HD_RESULT ret;
	HD_VIDEOPROC_OUT* p_out = &_test_out_dir[i];
	printf("*** set videoproc.out dir = (%08x) ***\r\n", 
		p_out->dir);

	ret = proc_set_out(&stream[0], p_out);
	if (ret != HD_OK) {
		printf("run out dir %d fail\r\n", i);
	}
	ret = proc_start(&stream[0]);
	if (ret != HD_OK) {
		printf("start fail = %d\r\n", ret);
	}
	
	sleep(1);
	pull_snapshot(&stream[0], hd_debug_get_menu_name());
	return 1;
}

static int proc_test_output_dir_0(void)
{
	return proc_test_output_dir(0);
}
static int proc_test_output_dir_rot90(void)
{
	return proc_test_output_dir(1);
}
static int proc_test_output_dir_rot270(void)
{
	return proc_test_output_dir(2);
}
static int proc_test_output_dir_scale(void)
{
	return proc_test_output_dir(3);
}
static int proc_test_output_dir_rot90_scale(void)
{
	return proc_test_output_dir(4);
}
static int proc_test_output_dir_rot270_scale(void)
{
	return proc_test_output_dir(5);
}

static HD_DBG_MENU proc_menu_output_dir[] = {
	{0x01, "out_dir=0",  			proc_test_output_dir_0,	TRUE},
	{0x02, "out_dir=rot90",  		proc_test_output_dir_rot90,	TRUE},
	{0x03, "out_dir=rot270",  		proc_test_output_dir_rot270,	TRUE},
	{0x04, "out_dir=scale",  		proc_test_output_dir_scale,	TRUE},
	{0x05, "out_dir=rot90_scale",  	proc_test_output_dir_rot90_scale,	TRUE},
	{0x06, "out_dir=rot270_scale",  proc_test_output_dir_rot270_scale,	TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

static int proc_test_list_output_dir(void)
{
	return hd_debug_menu_entry_p(proc_menu_output_dir, "VDOPRC TEST OUT DIR");
}

///////////////////////////////////////////////////////////////////////////////////

static HD_VIDEOPROC_OUT _test_out_dim[4] = {
	{0, {1920, 1080}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1), 1},
	{0, {960, 540}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1), 1},  // 1/2x of 1920
	{0, {160, 88}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1), 1},  //  1/16x of 1920
	{0, {5120, 2880}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1), 1},  //  32x of 160
};

static int proc_test_output_dim(UINT32 i)
{
	HD_RESULT ret;
 	HD_VIDEOPROC_OUT* p_out = &_test_out_dim[i];
	printf("*** set videoproc.out dim = (%d,%d) ***\r\n", 
		p_out->dim.w, p_out->dim.h);

	ret = proc_set_out(&stream[0], p_out);
	if (ret != HD_OK) {
		printf("run out dim %d fail\r\n", i);
	}
	ret = proc_start(&stream[0]);
	if (ret != HD_OK) {
		printf("start fail = %d\r\n", ret);
	}
	
	sleep(1);
	pull_snapshot(&stream[0], hd_debug_get_menu_name());
	return 1;
}

static int proc_test_output_dim_1080(void)
{
	return proc_test_output_dim(0);
}
static int proc_test_output_dim_540(void)
{
	return proc_test_output_dim(1);
}
static int proc_test_output_dim_88(void)
{
	return proc_test_output_dim(2);
}
static int proc_test_output_dim_2880(void)
{
	return proc_test_output_dim(3);
}

static HD_DBG_MENU proc_menu_output_dim[] = {
	{0x01, "out_dim=1920x1080",  	proc_test_output_dim_1080,	TRUE},
	{0x02, "out_dim=960x540",  		proc_test_output_dim_540,	TRUE},
	{0x03, "out_dim=160x88",  		proc_test_output_dim_88,	TRUE},
	{0x04, "out_dim=5120x2880",  	proc_test_output_dim_2880,	TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

static int proc_test_list_output_dim(void)
{
	return hd_debug_menu_entry_p(proc_menu_output_dim, "VDOPRC TEST OUT DIM");
}


///////////////////////////////////////////////////////////////////////////////////

static HD_VIDEOPROC_OUT _test_out_pxlfmt[4] = {
	{0, {1920, 1080}, HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1), 1},
	{0, {1920, 1080}, HD_VIDEO_PXLFMT_YUV420_PLANAR, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1), 1},
	{0, {1920, 1080}, HD_VIDEO_PXLFMT_Y8, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1), 1},
#if defined(_BSP_NA51055_)||defined(_BSP_NA51089_)
	{0, {1920, 1080}, HD_VIDEO_PXLFMT_YUV420_NVX2, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1), 1},
#else
	{0, {1920, 1080}, HD_VIDEO_PXLFMT_YUV420_NVX1_H264, HD_VIDEO_DIR_NONE, HD_VIDEO_FRC_RATIO(1,1), 1},
#endif
};

static int proc_test_output_pxlfmt(UINT32 i)
{
	HD_RESULT ret;
 	HD_VIDEOPROC_OUT* p_out = &_test_out_pxlfmt[i];
	printf("*** set videoproc.out pxlfmt = (%08x) ***\r\n", 
		p_out->pxlfmt);

	ret = proc_set_out(&stream[0], p_out);
	if (ret != HD_OK) {
		printf("run out fmt %d fail\r\n", i);
	}
	ret = proc_start(&stream[0]);
	if (ret != HD_OK) {
		printf("start fail = %d\r\n", ret);
	}
	
	sleep(1);
	pull_snapshot(&stream[0], hd_debug_get_menu_name());
	return 1;
}

static int proc_test_output_fmt_yuv420(void)
{
	return proc_test_output_pxlfmt(0);
}
static int proc_test_output_fmt_yuv420p(void)
{
	return proc_test_output_pxlfmt(1);
}
static int proc_test_output_fmt_y8(void)
{
	return proc_test_output_pxlfmt(2);
}
static int proc_test_output_fmt_nvx(void)
{
	return proc_test_output_pxlfmt(3);
}

static HD_DBG_MENU proc_menu_output_fmt[] = {
	{0x01, "out_fmt=yuv420",  		proc_test_output_fmt_yuv420,	TRUE},
	{0x02, "out_fmt=yuv420p",  		proc_test_output_fmt_yuv420p,	TRUE},
	{0x03, "out_fmt=y8",  			proc_test_output_fmt_y8,	TRUE},
	{0x04, "out_fmt=nvx",  			proc_test_output_fmt_nvx,	TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

static int proc_test_list_output_fmt(void)
{
	return hd_debug_menu_entry_p(proc_menu_output_fmt, "VDOPRC TEST OUT FMT");
}


///////////////////////////////////////////////////////////////////////////////////

static HD_VIDEOPROC_CROP _test_out_crop[7] = {
	{HD_CROP_OFF, {{0, 0},{0,0,0,0}}},
	{HD_CROP_ON,  {{0, 0},{0,0,1920,1080}}},
	{HD_CROP_ON,  {{0, 0},{0,0,960,540}}},
	{HD_CROP_ON,  {{0, 0},{960,0,960,540}}},
	{HD_CROP_ON,  {{0, 0},{0,540,960,540}}},
	{HD_CROP_ON,  {{0, 0},{960,540,960,540}}},
	{HD_CROP_ON,  {{0, 0},{960/2,540/2,960,540}}},
//	{HD_CROP_AUTO, {{0, 0},{0,0,1920,1080}}},
};

static int proc_test_output_crop(UINT32 i)
{
	HD_RESULT ret;
	HD_VIDEOPROC_CROP* p_crop = &_test_out_crop[i];

	printf("*** set videoproc.out crop = (%d,%d,%d,%d) ***\r\n", 
		p_crop->win.rect.x, p_crop->win.rect.y, p_crop->win.rect.w, p_crop->win.rect.h);

	ret = proc_set_out_crop(&stream[0], p_crop);
	if (ret != HD_OK) {
		printf("run out crop %d fail\r\n", i);
	}
	ret = proc_start(&stream[0]);
	if (ret != HD_OK) {
		printf("start fail = %d\r\n", ret);
	}
	
	sleep(1);
	pull_snapshot(&stream[0], hd_debug_get_menu_name());
	return 1;
}

static int proc_test_output_crop_off(void)
{
	return proc_test_output_crop(0);
}
static int proc_test_output_crop_all(void)
{
	return proc_test_output_crop(1);
}
static int proc_test_output_crop_left_top(void)
{
	return proc_test_output_crop(2);
}
static int proc_test_output_crop_right_top(void)
{
	return proc_test_output_crop(3);
}
static int proc_test_output_crop_left_bottom(void)
{
	return proc_test_output_crop(4);
}
static int proc_test_output_crop_right_bottom(void)
{
	return proc_test_output_crop(5);
}
static int proc_test_output_crop_center(void)
{
	return proc_test_output_crop(6);
}

static HD_DBG_MENU proc_menu_output_crop[] = {
	{0x01, "out_crop=off",  			proc_test_output_crop_off,	TRUE},
	{0x02, "out_crop=all",  			proc_test_output_crop_all,	TRUE},
	{0x03, "out_crop=left_top",  		proc_test_output_crop_left_top,	TRUE},
	{0x04, "out_crop=right_top",  		proc_test_output_crop_right_top,	TRUE},
	{0x05, "out_crop=left_bottom",  	proc_test_output_crop_left_bottom,	TRUE},
	{0x06, "out_crop=right_bottom",  	proc_test_output_crop_right_bottom,	TRUE},
	{0x07, "out_crop=center",  			proc_test_output_crop_center,	TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

static int proc_test_list_output_crop(void)
{
	return hd_debug_menu_entry_p(proc_menu_output_crop, "VDOPRC TEST OUT CROP");
}

///////////////////////////////////////////////////////////////////////////////////

static int proc_test_output_frc(UINT32 dest)
{
	HD_RESULT ret;
	HD_VIDEOPROC_FRC video_out_param = {0};
	HD_VIDEOPROC_FRC* p_frc = &video_out_param;
	UINT32 vp_out_dest_fr, vp_out_src_fr;
	vp_out_src_fr = 30;
	vp_out_dest_fr = dest;
	printf("*** set videoproc.out frc = (%d/%d) ***\r\n", vp_out_dest_fr, vp_out_src_fr);
	p_frc->frc = HD_VIDEO_FRC_RATIO(vp_out_dest_fr, vp_out_src_fr);
	ret = proc_set_out_frc(&stream[0], p_frc);
	if (ret != HD_OK) {
		printf("run out frc %d fail\r\n", dest);
	}
	ret = proc_start(&stream[0]);
	if (ret != HD_OK) {
		printf("start fail = %d\r\n", ret);
	}
	
	sleep(2);
	system("cat /proc/hdal/vprc/info");
	return 1;
}
static int proc_test_output_frc_30(void)
{
	return proc_test_output_frc(30);
}
static int proc_test_output_frc_25(void)
{
	return proc_test_output_frc(25);
}
static int proc_test_output_frc_20(void)
{
	return proc_test_output_frc(20);
}
static int proc_test_output_frc_15(void)
{
	return proc_test_output_frc(15);
}
static int proc_test_output_frc_10(void)
{
	return proc_test_output_frc(10);
}
static int proc_test_output_frc_5(void)
{
	return proc_test_output_frc(5);
}
static int proc_test_output_frc_0(void)
{
	return proc_test_output_frc(0);
}

static HD_DBG_MENU proc_menu_output_frc[] = {
	{0x01, "out_frc=30_30",  		proc_test_output_frc_30,	TRUE},
	{0x02, "out_frc=25_30",  		proc_test_output_frc_25,	TRUE},
	{0x03, "out_frc=20_30",         proc_test_output_frc_20,	TRUE},
	{0x04, "out_frc=15_30",         proc_test_output_frc_15,	TRUE},
	{0x05, "out_frc=10_30",         proc_test_output_frc_10,	TRUE},
	{0x06, "out_frc=5_30",         	proc_test_output_frc_5,	TRUE},
	{0x07, "out_frc=0_30",        	proc_test_output_frc_0,	TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

static int proc_test_list_output_frc(void)
{
	return hd_debug_menu_entry_p(proc_menu_output_frc, "VDOPRC TEST OUT FRC");
}

///////////////////////////////////////////////////////////////////////////////////

static int proc_test_output_pre_crop(UINT32 i)
{
	HD_RESULT ret;
	HD_VIDEOPROC_CROP* p_crop = &_test_out_crop[i];

	printf("*** set videoproc.out pre scale crop = (%d,%d,%d,%d) ***\r\n",
		p_crop->win.rect.x, p_crop->win.rect.y, p_crop->win.rect.w, p_crop->win.rect.h);

	ret = hd_videoproc_set(stream[0].proc_path, HD_VIDEOPROC_PARAM_IN_CROP_PSR, p_crop);
	if (ret != HD_OK) {
		printf("run out crop %d fail\r\n", i);
	}
	ret = proc_start(&stream[0]);
	if (ret != HD_OK) {
		printf("start fail = %d\r\n", ret);
	}

	sleep(1);
	pull_snapshot(&stream[0], hd_debug_get_menu_name());
	return 1;
}

static int proc_test_output_pre_crop_off(void)
{
	return proc_test_output_pre_crop(0);
}
static int proc_test_output_pre_crop_all(void)
{
	return proc_test_output_pre_crop(1);
}
static int proc_test_output_pre_crop_left_top(void)
{
	return proc_test_output_pre_crop(2);
}
static int proc_test_output_pre_crop_right_top(void)
{
	return proc_test_output_pre_crop(3);
}
static int proc_test_output_pre_crop_left_bottom(void)
{
	return proc_test_output_pre_crop(4);
}
static int proc_test_output_pre_crop_right_bottom(void)
{
	return proc_test_output_pre_crop(5);
}
static int proc_test_output_pre_crop_center(void)
{
	return proc_test_output_pre_crop(6);
}

static HD_DBG_MENU proc_menu_output_pre_crop[] = {
	{0x01, "out_pre_crop=off",  			proc_test_output_pre_crop_off,	TRUE},
	{0x02, "out_pre_crop=all",  			proc_test_output_pre_crop_all,	TRUE},
	{0x03, "out_pre_crop=left_top",  		proc_test_output_pre_crop_left_top,	TRUE},
	{0x04, "out_pre_crop=right_top",  		proc_test_output_pre_crop_right_top,	TRUE},
	{0x05, "out_pre_crop=left_bottom",  	proc_test_output_pre_crop_left_bottom,	TRUE},
	{0x06, "out_pre_crop=right_bottom",  	proc_test_output_pre_crop_right_bottom,	TRUE},
	{0x07, "out_pre_crop=center",  			proc_test_output_pre_crop_center,	TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

static int proc_test_list_output_pre_scl_crop(void)
{
	return hd_debug_menu_entry_p(proc_menu_output_pre_crop, "VDOPRC TEST OUT PRE-SCALE CROP");
}

///////////////////////////////////////////////////////////////////////////////////

static HD_DBG_MENU proc_test_main_menu[] = {
	//0~
	{0x01, "init",  				proc_test_flow_init,	TRUE},
	{0x02, "open",  				proc_test_flow_open,	TRUE},
	{0x03, "start",  				proc_test_flow_start,	TRUE},
	{0x04, "stop",  				proc_test_flow_stop,	TRUE},
	{0x05, "close",  				proc_test_flow_close,	TRUE},
	{0x06, "uninit",  			proc_test_flow_uninit,	TRUE},
	{0x07, "(cat)info",  			proc_test_cat_info,		TRUE},
	{0x08, "(cat)mem",			proc_test_cat_mem,	TRUE},
	//10~
	{0x0a, "*cfg_dev_id",			NULL,	TRUE},
	{0x0b, "*cfg_in_id",			NULL,	TRUE},
	{0x0c, "*cfg_out_id",			proc_test_list_output_id,	TRUE},
	{0x0d, "*cfg_pipe",            proc_test_list_devcfg_pipe,	TRUE},
	//20~
	{0x14, "*ctrl_func",           proc_test_list_ctrl_func,	TRUE},
	//30~
	{0x1e, "*in_fmt",				proc_test_list_input_fmt,	TRUE},
	{0x1f, "*in_dim",				proc_test_list_input_dim,	TRUE},
	{0x21, "*in_frc",             	proc_test_list_input_frc,	TRUE},
	{0x22, "*in_crop",           	proc_test_list_input_crop,	TRUE},
	{0x23, "*in_dir",             	proc_test_list_input_dir,	TRUE},
	//40~
	{0x28, "*out_fmt",			proc_test_list_output_fmt,	TRUE},
	{0x29, "*out_dim",			proc_test_list_output_dim,	TRUE},
	{0x2a, "*out_dir",			proc_test_list_output_dir,	TRUE},
	{0x2b, "*out_crop",			proc_test_list_output_crop,	TRUE},
	{0x2c, "*out_frc",			proc_test_list_output_frc,	TRUE},
	{0x2d, "*out_pre_scl_crop",	proc_test_list_output_pre_scl_crop,	TRUE},
	// escape muse be last
	{-1,   "",               		NULL,    FALSE},
};

MAIN(argc, argv)
{
	proc_test_devcfg_pipe(HD_VIDEOPROC_PIPE_RAWALL);
	proc_test_ctrl_func(0);
	hd_debug_menu_entry_p(proc_test_main_menu, "VDOPRC TESTKIT MAIN");

	return 0;
}

