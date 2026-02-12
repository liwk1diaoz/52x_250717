/**
	@brief Source file of vendor net application sample using user-space net flow.

	@file alg_fdcnn_sample_stream.c

	@ingroup alg_fdcnn_sample_stream

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
#include <kwrap/examsys.h>
#include <sys/time.h>

#include "vendor_isp.h"
#include "vendor_gfx.h"
#include "vendor_ai.h"
#include "vendor_ai_util.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "vendor_ai_cpu_postproc.h"

#include "pvdcnn_lib.h"


#if defined(__LINUX)
#else
//for delay
#include <kwrap/task.h>
#define sleep(x)    vos_task_delay_ms(1000*x)
#define usleep(x)   vos_task_delay_us(x)
#endif

#define DEBUG_MENU 			1

//#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
//#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
//#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////

#define	SENSOR_291			1
#define	SENSOR_S02K			2
#define	SENSOR_S05A			3
#define	SENSOR_CHOICE		SENSOR_291 // SENSOR_S02K

#define DBGINFO_BUFSIZE()	(0x200)

//RAW
#define VDO_RAW_BUFSIZE(w, h, pxlfmt)   (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//RAW compress only support 12bit mode
#define VDO_NRX_BUFSIZE(w, h)           (ALIGN_CEIL_64(w)/64*14*4*h)
//CA for AWB
#define VDO_CA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 3) << 1)
//LA for AE
#define VDO_LA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 1) << 1)

#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	ALIGN_CEIL_4(((w) * (h) * HD_VIDEO_PXLFMT_BPP(pxlfmt)) / 8)
#define VDO_NVX_BUFSIZE(w, h, pxlfmt)	(VDO_YUV_BUFSIZE(w, h, pxlfmt) * RAW_COMPRESS_RATIO / 100)

#define SEN_OUT_FMT			HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT			HD_VIDEO_PXLFMT_RAW12
#define CA_WIN_NUM_W		32
#define CA_WIN_NUM_H		32
#define LA_WIN_NUM_W		32
#define LA_WIN_NUM_H		32

#define VDO_SIZE_W			1920
#define VDO_SIZE_H			1080

#define SOURCE_PATH 		HD_VIDEOPROC_0_OUT_1
#define EXTEND_PATH1		HD_VIDEOPROC_0_OUT_5
#define EXTEND_PATH2		HD_VIDEOPROC_0_OUT_6

#define	VDO_FRAME_FORMAT	HD_VIDEO_PXLFMT_YUV420
#define OSG_LCD_WIDTH       960
#define OSG_LCD_HEIGHT      240

#define NN_PVDCNN_MODE			ENABLE
#define NN_PVDCNN_PROF			ENABLE
#define NN_PVDCNN_DUMP			DISABLE
#define NN_PVDCNN_DRAW			ENABLE
#define NN_PVDCNN_TOP_FPS		DISABLE

#define NN_USE_DRAM2           ENABLE

#define	NN_USE_HDR			   0		// 1 ON; 0 OFF

#define SHDR_CAP_OUT_FMT HD_VIDEO_PXLFMT_NRX12_SHDR2

#define VENDOR_AI_CFG  0x000f0000  //ai project config


typedef struct _VIDEO_LIVEVIEW {
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

	// (4) --
	HD_VIDEOPROC_SYSCAPS proc_alg_syscaps;
	HD_PATH_ID proc_alg_ctrl;
	HD_PATH_ID proc_alg_path;

	HD_DIM  proc_alg_max_dim;
	HD_DIM  proc_alg_dim;

	// (5) --
	HD_PATH_ID mask_alg_path;

#if NN_PVDCNN_DRAW
    HD_PATH_ID mask_path4;
    HD_PATH_ID mask_path5;
    HD_PATH_ID mask_path6;
    HD_PATH_ID mask_path7;
#endif

    HD_VIDEOOUT_HDMI_ID hdmi_id;
} VIDEO_LIVEVIEW;

static VENDOR_AIS_FLOW_MEM_PARM g_mem = {0};
static HD_COMMON_MEM_VB_BLK g_blk_info[1];

typedef struct _THREAD_PARM {
    VENDOR_AIS_FLOW_MEM_PARM mem;
    VIDEO_LIVEVIEW stream;
	UINT32 net_key;
} THREAD_PARM;

typedef struct _THREAD_DRAW_PARM {
    VENDOR_AIS_FLOW_MEM_PARM pvd_mem;
    VIDEO_LIVEVIEW stream;
} THREAD_DRAW_PARM;

static UINT32 g_shdr_mode = 0;
UINT32 fix_fps = 10;

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
    UINT32 mem_size = 0;

#if NN_PVDCNN_MODE
    mem_size += pvdcnn_calcbuffsize();
#endif

	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, CAP_OUT_FMT)
        													+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
        													+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	mem_cfg.pool_info[0].blk_cnt = 2;
    mem_cfg.pool_info[0].ddr_id = DDR_ID0;

	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, VDO_FRAME_FORMAT);
	mem_cfg.pool_info[1].blk_cnt = 3;
    mem_cfg.pool_info[1].ddr_id = DDR_ID0;

	// config common pool (sub)
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[2].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, VDO_FRAME_FORMAT);
	mem_cfg.pool_info[2].blk_cnt = 3;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;

    // config common pool (sub)
	mem_cfg.pool_info[3].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[3].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, VDO_FRAME_FORMAT);
	mem_cfg.pool_info[3].blk_cnt = 3;
	mem_cfg.pool_info[3].ddr_id = DDR_ID0;

	// for nn
	mem_cfg.pool_info[4].type 		= HD_COMMON_MEM_CNN_POOL;
	mem_cfg.pool_info[4].blk_size 	= mem_size;
	mem_cfg.pool_info[4].blk_cnt 	= 1;

#if NN_USE_DRAM2
    mem_cfg.pool_info[4].ddr_id = DDR_ID1;
#else
	mem_cfg.pool_info[4].ddr_id = DDR_ID0;
#endif

	ret = hd_common_mem_init(&mem_cfg);
	return ret;
}

static INT32 get_mem_block(VOID)
{
	HD_RESULT                 ret = HD_OK;
	UINT32                    pa, va;
	HD_COMMON_MEM_VB_BLK      blk;
    UINT32 mem_size = 0;

#if NN_USE_DRAM2
    HD_COMMON_MEM_DDR_ID      ddr_id = DDR_ID1;
#else
    HD_COMMON_MEM_DDR_ID      ddr_id = DDR_ID0;
#endif

#if NN_PVDCNN_MODE
    mem_size += pvdcnn_calcbuffsize();
#endif

	/* Allocate parameter buffer */
	if (g_mem.va != 0) {
		DBG_DUMP("err: mem has already been inited\r\n");
		return -1;
	}
    blk = hd_common_mem_get_block(HD_COMMON_MEM_CNN_POOL, mem_size, ddr_id);
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		DBG_DUMP("hd_common_mem_get_block fail\r\n");
		ret =  HD_ERR_NG;
		goto exit;
	}
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		DBG_DUMP("not get buffer, pa=%08x\r\n", (int)pa);
		return -1;
	}
	va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, mem_size);
	g_blk_info[0] = blk;

	/* Release buffer */
	if (va == 0) {
		ret = hd_common_mem_munmap((void *)va, mem_size);
		if (ret != HD_OK) {
			DBG_DUMP("mem unmap fail\r\n");
			return ret;
		}
		return -1;
	}
	g_mem.pa = pa;
	g_mem.va = va;
	g_mem.size = mem_size;

exit:
	return ret;
}

static HD_RESULT release_mem_block(VOID)
{
	HD_RESULT ret = HD_OK;
    UINT32 mem_size = 0;

#if NN_PVDCNN_MODE
    mem_size += pvdcnn_calcbuffsize();
#endif

	/* Release in buffer */
	if (g_mem.va) {
		ret = hd_common_mem_munmap((void *)g_mem.va, mem_size);
		if (ret != HD_OK) {
			DBG_DUMP("mem_uninit : (g_mem.va)hd_common_mem_munmap fail.\r\n");
			return ret;
		}
	}
	//ret = hd_common_mem_release_block((HD_COMMON_MEM_VB_BLK)g_mem.pa);
	ret = hd_common_mem_release_block(g_blk_info[0]);
	if (ret != HD_OK) {
		DBG_DUMP("mem_uninit : (g_mem.pa)hd_common_mem_release_block fail.\r\n");
		return ret;
	}

	return ret;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	hd_common_mem_uninit();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT get_cap_caps(HD_PATH_ID video_cap_ctrl, HD_VIDEOCAP_SYSCAPS *p_video_cap_syscaps)
{
	HD_RESULT ret = HD_OK;
	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSCAPS, p_video_cap_syscaps);
	return ret;
}
#if 0
static HD_RESULT get_cap_sysinfo(HD_PATH_ID video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_SYSINFO sys_info = {0};

	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
	printf("sys_info.devid =0x%X, cur_fps[0]=%d/%d, vd_count=%llu\r\n", sys_info.dev_id, GET_HI_UINT16(sys_info.cur_fps[0]), GET_LO_UINT16(sys_info.cur_fps[0]), sys_info.vd_count);
	return ret;
}
#endif
static HD_RESULT set_cap_cfg(HD_PATH_ID *p_video_cap_ctrl)
{
#if SENSOR_CHOICE==SENSOR_291
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};

	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x301;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0|PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB;
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;

#elif SENSOR_CHOICE==SENSOR_S02K
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};

	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os02k10");
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =	0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xF01;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;

	if(g_shdr_mode==1) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = HD_VIDEOCAP_SEN_IGNORE;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = HD_VIDEOCAP_SEN_IGNORE;
	} else {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
	}

	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	if (g_shdr_mode == 1) {
		cap_cfg.sen_cfg.shdr_map = HD_VIDEOCAP_SHDR_MAP(HD_VIDEOCAP_HDR_SENSOR1, (HD_VIDEOCAP_0|HD_VIDEOCAP_1));
	}


	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB;

	if (g_shdr_mode == 1) {
		iq_ctl.func |= HD_VIDEOCAP_FUNC_SHDR;
	}

	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;

#elif SENSOR_CHOICE==SENSOR_S05A
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};

	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os05a10");
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =	0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xF01;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB;
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;

#endif
}

static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};

		video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
		video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		video_in_param.pxlfmt = SEN_OUT_FMT;

		// NOTE: only SHDR with path 1
		if (g_shdr_mode == 1) {
			video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_2;
		} else {
			video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
		}
		///video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;

		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN, &video_in_param);
		//printf("set_cap_param MODE=%d\r\n", ret);
		if (ret != HD_OK) {
			return ret;
		}
	}
	#if 1 //no crop, full frame
	{
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_OFF;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &video_crop_param);
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
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &video_crop_param);
		//printf("set_cap_param CROP ON=%d\r\n", ret);
	}
	#endif
	{
		HD_VIDEOCAP_OUT video_out_param = {0};

		//without setting dim for no scaling, using original sensor out size
		if (g_shdr_mode == 1) {
			video_out_param.pxlfmt = SHDR_CAP_OUT_FMT;
		} else {
			video_out_param.pxlfmt = CAP_OUT_FMT;
		}
		///video_out_param.pxlfmt = CAP_OUT_FMT;

		video_out_param.dir = HD_VIDEO_DIR_NONE;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_proc_cfg(HD_PATH_ID *p_video_proc_ctrl, HD_DIM* p_max_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
	HD_PATH_ID video_proc_ctrl = 0;

	ret = hd_videoproc_open(0, HD_VIDEOPROC_0_CTRL, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	if (p_max_dim != NULL ) {
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWALL;
		video_cfg_param.isp_id = 0;

		if (g_shdr_mode == 1) {
			video_cfg_param.ctrl_max.func |= HD_VIDEOPROC_FUNC_SHDR;
		} else {
			video_cfg_param.ctrl_max.func &= ~HD_VIDEOPROC_FUNC_SHDR;
		}
		///video_cfg_param.ctrl_max.func = 0;

		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;

		if (g_shdr_mode == 1) {
			video_cfg_param.in_max.pxlfmt = SHDR_CAP_OUT_FMT;
		} else {
			video_cfg_param.in_max.pxlfmt = CAP_OUT_FMT;
		}
		///video_cfg_param.in_max.pxlfmt = CAP_OUT_FMT;

		video_cfg_param.in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &video_cfg_param);
		if (ret != HD_OK) {
			return HD_ERR_NG;
		}
	}

	video_ctrl_param.func = 0;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);

	*p_video_proc_ctrl = video_proc_ctrl;

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
		video_out_param.pxlfmt = VDO_FRAME_FORMAT;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = 1;	// set > 0 to allow pull out (nn)

		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}

	return ret;
}

static HD_RESULT set_proc_param_extend(HD_PATH_ID video_proc_path, HD_PATH_ID src_path, HD_DIM* p_dim, UINT32 dir)
{
	HD_RESULT ret = HD_OK;

	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT_EX video_out_param = {0};
		video_out_param.src_path = src_path;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = VDO_FRAME_FORMAT;
		video_out_param.dir = dir;
		video_out_param.depth = 1; //set 1 to allow pull

		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_EX, &video_out_param);
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

	#if 1
	videoout_mode.output_type = HD_COMMON_VIDEO_OUT_LCD;
	videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
	videoout_mode.output_mode.lcd = HD_VIDEOOUT_LCD_0;
	if (out_type != 1) {
		printf("520 only support LCD\r\n");
	}
	#else
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
	#endif
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
	video_out_param.pxlfmt = VDO_FRAME_FORMAT;
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
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoout_init()) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module(VIDEO_LIVEVIEW *p_stream, HD_DIM* p_proc_max_dim, UINT32 out_type)
{
	HD_RESULT ret;
	// set videocap config
	ret = set_cap_cfg(&p_stream->cap_ctrl);
	if (ret != HD_OK) {
		printf("set cap-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoproc config
	ret = set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim);
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoproc config for nn
	ret = set_proc_cfg(&p_stream->proc_alg_ctrl, p_proc_max_dim);
	if (ret != HD_OK) {
		printf("set proc-cfg alg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoout config
	ret = set_out_cfg(&p_stream->out_ctrl, out_type,p_stream->hdmi_id);
	if (ret != HD_OK) {
		printf("set out-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, SOURCE_PATH, &p_stream->proc_alg_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0, &p_stream->out_path)) != HD_OK)
		return ret;

#if NN_PVDCNN_DRAW
    if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_4, &p_stream->mask_path4)) != HD_OK)
		return ret;
    if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_5, &p_stream->mask_path5)) != HD_OK)
		return ret;
    if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_6, &p_stream->mask_path6)) != HD_OK)
		return ret;
    if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_7, &p_stream->mask_path7)) != HD_OK)
		return ret;
#endif
	return HD_OK;
}

static HD_RESULT open_module_extend1(VIDEO_LIVEVIEW *p_stream, HD_DIM* p_proc_max_dim, UINT32 out_type)
{
	HD_RESULT ret;
	// set videoout config
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, EXTEND_PATH1, &p_stream->proc_alg_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_close(p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_close(p_stream->out_path)) != HD_OK)
		return ret;

#if NN_PVDCNN_DRAW
    if ((ret = hd_videoout_close(p_stream->mask_path4)) != HD_OK)
		return ret;
    if ((ret = hd_videoout_close(p_stream->mask_path5)) != HD_OK)
		return ret;
    if ((ret = hd_videoout_close(p_stream->mask_path6)) != HD_OK)
		return ret;
    if ((ret = hd_videoout_close(p_stream->mask_path7)) != HD_OK)
		return ret;
#endif

	return HD_OK;
}

static HD_RESULT close_module_extend(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_close(p_stream->proc_alg_path)) != HD_OK)
		return ret;
	return HD_OK;
}


static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoout_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}

#if NN_PVDCNN_DRAW
int init_mask_param(HD_PATH_ID mask_path)
{
	HD_OSG_MASK_ATTR attr;

	memset(&attr, 0, sizeof(HD_OSG_MASK_ATTR));

    // ghost target
    attr.position[0].x = 1;
    attr.position[0].y = 1;
    attr.position[1].x = 9;
    attr.position[1].y = 1;
    attr.position[2].x = 9;
    attr.position[2].y = 9;
    attr.position[3].x = 1;
    attr.position[3].y = 9;
    attr.type          = HD_OSG_MASK_TYPE_HOLLOW;
    attr.alpha         = 0;
    attr.color         = 0x00FF0000;
    attr.thickness     = 0;

	return hd_videoout_set(mask_path, HD_VIDEOOUT_PARAM_OUT_MASK_ATTR, &attr);
}

int pvdcnn_mask_draw(HD_PATH_ID mask_path, PVDCNN_RSLT *p_obj, BOOL bdraw, UINT32 color)
{
    HD_OSG_MASK_ATTR attr;
    HD_RESULT ret = HD_OK;

    memset(&attr, 0, sizeof(HD_OSG_MASK_ATTR));

    if (!bdraw || p_obj == NULL) {
        return init_mask_param(mask_path);
    }

	HD_URECT *p_box = &p_obj->box;

    attr.position[0].x = p_box->x;
    attr.position[0].y = p_box->y;
    attr.position[1].x = p_box->x + p_box->w;
    attr.position[1].y = p_box->y;
    attr.position[2].x = p_box->x + p_box->w;
    attr.position[2].y = p_box->y + p_box->h;
    attr.position[3].x = p_box->x;
    attr.position[3].y = p_box->y + p_box->h;
    attr.type          = HD_OSG_MASK_TYPE_HOLLOW;
    attr.alpha         = 255;
    attr.color         = color;
    attr.thickness     = 2;

    ret = hd_videoout_set(mask_path, HD_VIDEOOUT_PARAM_OUT_MASK_ATTR, &attr);

    return ret;
}

#endif

#if NN_PVDCNN_DRAW
int pvdcnn_draw_info(VENDOR_AIS_FLOW_MEM_PARM buf, VIDEO_LIVEVIEW *p_stream)
{
    HD_URECT size = {0, 0, OSG_LCD_WIDTH, OSG_LCD_HEIGHT};
	static PVDCNN_RSLT info[4] = {0};

    UINT32 num = pvdcnn_get_result(buf, info, &size, 4);

    pvdcnn_mask_draw(p_stream->mask_path4, info + 0, (BOOL)(num >= 1), 0x0000FF00);
    pvdcnn_mask_draw(p_stream->mask_path5, info + 1, (BOOL)(num >= 2), 0x0000FF00);
    pvdcnn_mask_draw(p_stream->mask_path6, info + 2, (BOOL)(num >= 3), 0x0000FF00);
    pvdcnn_mask_draw(p_stream->mask_path7, info + 3, (BOOL)(num >= 4), 0x0000FF00);

    return HD_OK;
}
#endif


VOID *draw_thread(VOID *arg)
{
    THREAD_DRAW_PARM *p_draw_parm = (THREAD_DRAW_PARM*)arg;
    VIDEO_LIVEVIEW stream = p_draw_parm->stream;
#if NN_PVDCNN_DRAW
    VENDOR_AIS_FLOW_MEM_PARM pvd_mem = p_draw_parm->pvd_mem;
#endif
    HD_VIDEO_FRAME video_frame = {0};
    HD_RESULT ret = HD_OK;

    // wait fd ro pvd init ready
    sleep(2);

    while(1)
    {
        ret = hd_videoproc_pull_out_buf(stream.proc_alg_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
        if(ret != HD_OK)
        {
            printf("ERR : hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
            break;
        }

#if NN_PVDCNN_DRAW
        pvdcnn_draw_info(pvd_mem, &stream);
#endif

        ret = hd_videoproc_release_out_buf(stream.proc_alg_path, &video_frame);
        if(ret != HD_OK)
        {
            printf("ERR : hd_videoproc_release_out_buf fail (%d)\n\r", ret);
            break;
        }
    }

    return 0;
}

static UINT32 load_file(CHAR *p_filename, UINT32 va)
{
	FILE  *fd;
	UINT32 file_size = 0, read_size = 0;
	const UINT32 model_addr = va;

	fd = fopen(p_filename, "rb");
	if (!fd) {
		printf("ERR: cannot read %s\r\n", p_filename);
		return 0;
	}

	fseek ( fd, 0, SEEK_END );
	file_size = ALIGN_CEIL_4( ftell(fd) );
	fseek ( fd, 0, SEEK_SET );

	read_size = fread ((void *)model_addr, 1, file_size, fd);
	if (read_size != file_size) {
		printf("ERR: size mismatch, real = %d, idea = %d\r\n", (int)read_size, (int)file_size);
	}
	fclose(fd);
	return read_size;
}

static VENDOR_AIS_FLOW_MEM_PARM getmem(VENDOR_AIS_FLOW_MEM_PARM *valid_mem, UINT32 required_size)
{
	VENDOR_AIS_FLOW_MEM_PARM mem = {0};
	required_size = ALIGN_CEIL_4(required_size);
	if(required_size <= valid_mem->size) {
		mem.va = valid_mem->va;
        mem.pa = valid_mem->pa;
		mem.size = required_size;

		valid_mem->va += required_size;
        valid_mem->pa += required_size;
		valid_mem->size -= required_size;
	} else {
		printf("ERR : required size %d > total memory size %d\r\n", required_size, valid_mem->size);
	}
	return mem;
}

VOID *pvdcnn_thread(VOID *arg)
{
    THREAD_PARM *p_parm = (THREAD_PARM*)arg;
    VIDEO_LIVEVIEW stream = p_parm->stream;
    VENDOR_AIS_FLOW_MEM_PARM mem = p_parm->mem;

    HD_VIDEO_FRAME video_frame = {0};
    HD_RESULT ret = HD_OK;
	UINT32 fps_delay = 0;

#if NN_PVDCNN_PROF
    static struct timeval tstart0, tend0;
    static UINT64 cur_time0 = 0, mean_time0 = 0, sum_time0 = 0;
    static UINT32 icount = 0;
#endif

    UINT32 buf_size = pvdcnn_calcbuffsize();

    VENDOR_AIS_FLOW_MEM_PARM buf = getmem(&mem, buf_size);
	printf("buf: va = %0x, pa = %0x, size = %ld\r\n", buf.va, buf.pa, buf.size);

    UINT32 read_size = load_file("/mnt/sd/CNNLib/para/pvdcnn/nvt_model.bin", buf.va);
	printf("read model size = %ld\r\n", read_size);

	PVDCNN_INIT_PRMS init_prms = {0};
	init_prms.net_id = 0;
	init_prms.conf_thr = 0.395;
	init_prms.conf_thr2 = 0.4;
	init_prms.nms_thr = 0.3;	
	init_prms.net_key = p_parm->net_key;
	
    ret = pvdcnn_init(buf, &init_prms);
	if (ret != HD_OK) {
		printf("pvdcnn_init fail\r\n");
		return 0;
	}

    while (1) {
        ret = hd_videoproc_pull_out_buf(stream.proc_alg_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
        if (ret != HD_OK) {
            printf("ERR : hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
            break;
        }

        // init image
        HD_GFX_IMG_BUF input_image;
        input_image.dim.w  = video_frame.dim.w;
        input_image.dim.h = video_frame.dim.h;
        input_image.format = VDO_FRAME_FORMAT;
        input_image.p_phy_addr[0] = video_frame.phy_addr[0];
        input_image.p_phy_addr[1] = video_frame.phy_addr[1];
        input_image.p_phy_addr[2] = video_frame.phy_addr[1]; // for avoid hd_gfx_scale message
        input_image.lineoffset[0] = video_frame.loff[0];
        input_image.lineoffset[1] = video_frame.loff[0]; // for avoid hd_gfx_scale message
        input_image.lineoffset[2] = video_frame.loff[0]; // for avoid hd_gfx_scale message

        ret = pvdcnn_set_img(buf, &input_image);
		if (ret != HD_OK) {
			printf("pvdcnn_set_img fail\r\n");
			break;
		}

        ret = hd_videoproc_release_out_buf(stream.proc_alg_path, &video_frame);
        if (ret != HD_OK) {
            printf("ERR : hd_videoproc_release_out_buf fail (%d)\n\r", ret);
            break;
        }

#if NN_PVDCNN_PROF
        gettimeofday(&tstart0, NULL);
#endif

        ret = pvdcnn_process(buf);
		if (ret != HD_OK) {
			printf("pvdcnn_process fail\r\n");
			break;
		}


#if NN_PVDCNN_PROF
        gettimeofday(&tend0, NULL);
        cur_time0 = (UINT64)(tend0.tv_sec - tstart0.tv_sec) * 1000000 + (tend0.tv_usec - tstart0.tv_usec);
        sum_time0 += cur_time0;
        mean_time0 = sum_time0 / (++icount);
        printf("[PVD] process cur time(us): %lld, mean time(us): %lld\r\n", cur_time0, mean_time0);
#endif

#if ((!NN_PVDCNN_TOP_FPS) && NN_PVDCNN_PROF)
		fps_delay = (UINT32)(1000000/fix_fps);
        if (cur_time0 < fps_delay)
            usleep(fps_delay - cur_time0 + (mean_time0*0));
#else
		usleep(fps_delay);
#endif

#if NN_PVDCNN_DUMP
		HD_URECT ref_size = {0, 0, VDO_SIZE_W, VDO_SIZE_H};
        ref_size.w = video_frame.dim.w;
        ref_size.h = video_frame.dim.h;

		static PVDCNN_RSLT rslt[16] = {0};
        UINT32 rslt_num = pvdcnn_get_result(buf, rslt, &ref_size, 16);

        pvdcnn_print_rslt(rslt, rslt_num, "PVD");
#endif
    }
    ret = pvdcnn_uninit(buf);
	if (ret != HD_OK) {
		printf("pvdcnn_uninit fail\r\n");
	}
    return 0;
}

int main(int argc, char** argv)
{
	HD_RESULT ret;
	UINT32 out_type = 1;
	UINT32 pvdcnn_key = 0;
	INT32 idx = 1;
    VIDEO_LIVEVIEW stream[2] = {0};

	g_shdr_mode = NN_USE_HDR;

	AET_CFG_INFO cfg_info = {0};

	// query program options
	if (argc > idx) {
		out_type = atoi(argv[idx++]);
		printf("out_type %d\r\n", out_type);
		if(out_type > 2) {
			printf("error: not support out_type!\r\n");
			return 0;
		}
	}
	if (argc > idx) {
		sscanf(argv[idx++], "%lx", &pvdcnn_key);
	}
	if (argc > idx) {
		fix_fps = atoi(argv[idx++]);
		printf("ai pvd fps %d\r\n", fix_fps);
		if(fix_fps < 1 || fix_fps > 30) {
			printf("error: not support ai pvd fps. Should be greater than 0 is less than 30!\r\n");
			return 0;
		}
	}
    stream[0].hdmi_id=HD_VIDEOOUT_HDMI_1920X1080I60;//default

	// query program options
	if (argc > idx && (atoi(argv[idx]) !=0)) {
		stream[0].hdmi_id = atoi(argv[idx]);
		printf("hdmi_mode %d\r\n", stream[0].hdmi_id);
	}

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
	}

	//set project config for AI
	hd_common_sysconfig(0, (1<<16), 0, VENDOR_AI_CFG); //enable AI engine

	// init memory
	ret = mem_init();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		goto exit;
	}

	ret = get_mem_block();
	if (ret != HD_OK) {
		DBG_ERR("mem_init fail=%d\n", ret);
		goto exit;
	}

	// init all modules
	ret = init_module();	// vdocap, vdoproc, vdoout
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}

    ret = hd_gfx_init();
	if (ret != HD_OK) {
		DBG_ERR("hd_gfx_init fail=%d\n", ret);
		goto exit;
	}

	// open video_liveview modules (main)
	stream[0].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[0].proc_max_dim.h = VDO_SIZE_H; //assign by user
	ret = open_module(&stream[0], &stream[0].proc_max_dim, out_type);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

    stream[1].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[1].proc_max_dim.h = VDO_SIZE_H; //assign by user
	ret = open_module_extend1(&stream[1], &stream[1].proc_max_dim, out_type);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

#if NN_PVDCNN_DRAW
    if(init_mask_param(stream[0].mask_path4)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
    if(init_mask_param(stream[0].mask_path5)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
    if(init_mask_param(stream[0].mask_path6)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
    if(init_mask_param(stream[0].mask_path7)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
#endif

	//render mask
#if NN_PVDCNN_DRAW
	ret = hd_videoout_start(stream[0].mask_path4);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoout_start(stream[0].mask_path5);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoout_start(stream[0].mask_path6);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoout_start(stream[0].mask_path7);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
#endif

	// get videocap capability
	ret = get_cap_caps(stream[0].cap_ctrl, &stream[0].cap_syscaps);
	if (ret != HD_OK) {
		printf("get cap-caps fail=%d\n", ret);
		goto exit;
	}

	// get videoout capability
	ret = get_out_caps(stream[0].out_ctrl, &stream[0].out_syscaps);
	if (ret != HD_OK) {
		printf("get out-caps fail=%d\n", ret);
		goto exit;
	}
	stream[0].out_max_dim = stream[0].out_syscaps.output_dim;

	// set videocap parameter
	stream[0].cap_dim.w = VDO_SIZE_W; //assign by user
	stream[0].cap_dim.h = VDO_SIZE_H; //assign by user
	ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (main)
	ret = set_proc_param(stream[0].proc_path, NULL);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (alg)
	stream[0].proc_alg_max_dim.w = VDO_SIZE_W;
	stream[0].proc_alg_max_dim.h = VDO_SIZE_H;
	ret = set_proc_param(stream[0].proc_alg_path, &stream[0].proc_alg_max_dim);
	if (ret != HD_OK) {
		printf("set proc alg fail=%d\n", ret);
		goto exit;
	}

	stream[1].proc_alg_max_dim.w = VDO_SIZE_W;
	stream[1].proc_alg_max_dim.h = VDO_SIZE_H;
    ret = set_proc_param_extend(stream[1].proc_alg_path, SOURCE_PATH, &stream[1].proc_alg_max_dim, HD_VIDEO_DIR_NONE);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoout parameter (main)
	stream[0].out_dim.w = stream[0].out_max_dim.w; //using device max dim.w
	stream[0].out_dim.h = stream[0].out_max_dim.h; //using device max dim.h
	ret = set_out_param(stream[0].out_path, &stream[0].out_dim);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}

	// bind video_liveview modules (main)
	hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOOUT_0_IN_0);
    //hd_videoproc_bind(SOURCE_PATH, HD_VIDEOOUT_0_IN_0);
    //hd_videoproc_bind(EXTEND_PATH1, HD_VIDEOOUT_0_IN_0);
    //hd_videoproc_bind(EXTEND_PATH2, HD_VIDEOOUT_0_IN_0);

	// start video_liveview modules (main)
	hd_videocap_start(stream[0].cap_path);
	hd_videoproc_start(stream[0].proc_path);
	hd_videoproc_start(stream[0].proc_alg_path);
    hd_videoproc_start(stream[1].proc_alg_path);
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);
	hd_videoout_start(stream[0].out_path);

    VENDOR_AIS_FLOW_MEM_PARM local_mem = g_mem;
	printf("total mem size = %ld\r\n", local_mem.size);

	// - hdr -----
	cfg_info.id = 0;

	if (vendor_isp_init() == HD_ERR_NG) {
		printf("vendor_isp_init fail!\n\r");
		return -1;
	}

	if(g_shdr_mode==1) {
		strncpy(cfg_info.path, "/mnt/app/isp/isp_os02k10_0_hdr.cfg", CFG_NAME_LENGTH);
		printf("Load /mnt/app/isp/isp_os02k10_0_hdr.cfg \n");
	}
	else {
		strncpy(cfg_info.path, "/mnt/app/isp/isp_os02k10_0.cfg", CFG_NAME_LENGTH);
		printf("Load /mnt/app/isp/isp_os02k10_0.cfg \n");
	}
	vendor_isp_set_ae(AET_ITEM_RLD_CONFIG, &cfg_info);
	vendor_isp_set_awb(AWBT_ITEM_RLD_CONFIG, &cfg_info);
	vendor_isp_set_iq(IQT_ITEM_RLD_CONFIG, &cfg_info);
	vendor_isp_uninit();
	// - end hdr -

    // main process
#if NN_PVDCNN_MODE
    THREAD_PARM pvd_thread_parm;
    pthread_t pvd_thread_id;
    UINT32 pvd_mem_size = pvdcnn_calcbuffsize();

    pvd_thread_parm.mem = getmem(&local_mem, pvd_mem_size);
    pvd_thread_parm.stream = stream[1];
	pvd_thread_parm.net_key = (UINT32)pvdcnn_key;

	ret = pthread_create(&pvd_thread_id, NULL, pvdcnn_thread, (VOID*)(&pvd_thread_parm));
    if (ret < 0) {
        DBG_ERR("create pvdcnn thread failed");
        goto exit;
    }
#endif

#if NN_PVDCNN_DRAW
    THREAD_DRAW_PARM pvdcnn_draw_parm;
    pthread_t pvdcnn_draw_id;
    pvdcnn_draw_parm.stream = stream[0];
    pvdcnn_draw_parm.pvd_mem = pvd_thread_parm.mem;

    ret = pthread_create(&pvdcnn_draw_id, NULL, draw_thread, (VOID*)(&pvdcnn_draw_parm));
    if (ret < 0) {
        DBG_ERR("create pvdcnn draw thread failed");
        goto exit;
    }
#endif



#if NN_PVDCNN_MODE
    pthread_join(pvd_thread_id, NULL);
#endif

#if NN_PVDCNN_DRAW
    pthread_join(pvdcnn_draw_id, NULL);
#endif

	// stop video_liveview modules (main)
	hd_videocap_stop(stream[0].cap_path);
	hd_videoproc_stop(stream[0].proc_path);
	hd_videoproc_stop(stream[0].proc_alg_path);
    hd_videoproc_stop(stream[1].proc_alg_path);
	hd_videoout_stop(stream[0].out_path);

	// unbind video_liveview modules (main)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);
    //hd_videoproc_unbind(SOURCE_PATH);
    //hd_videoproc_unbind(EXTEND_PATH1);
    //hd_videoproc_unbind(EXTEND_PATH2);
exit:

	// close video_liveview modules (main)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

    ret = close_module_extend(&stream[1]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

	// uninit all modules
	ret = exit_module();
	if (ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}

    ret = hd_gfx_uninit();
	if (ret != HD_OK) {
		DBG_ERR("hd_gfx_uninit fail=%d\n", ret);
	}

	ret = release_mem_block();
	if (ret != HD_OK) {
		DBG_ERR("mem_uninit fail=%d\n", ret);
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

	return 0;
}
