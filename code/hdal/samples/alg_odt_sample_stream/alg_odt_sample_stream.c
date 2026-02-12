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

#include "nvt_dis_api.h"
#include "nvt_odt.h"

#include "vendor_isp.h"
/*
#include "isp_api.h"
#include "ae_alg.h"
#include "aet_api.h"
*/

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

#define DET_MODE       ENABLE
#define DET_PROF       ENABLE
#define DET_DUMP       DISABLE
#define DET_DRAW       ENABLE
#define DET_FIX_FRM    ENABLE


#define USE_DRAM2              ENABLE
#define DIS_MODE               ENABLE
#define ODT_MODE               ENABLE
#define ODT_PROF               ENABLE
#define ODT_FIX_FRM            ENABLE

#define	ODT_USE_HDR			   0		// 1 ON; 0 OFF

#define SHDR_CAP_OUT_FMT HD_VIDEO_PXLFMT_NRX12_SHDR2

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

    // osg
#if DET_DRAW
	HD_PATH_ID mask_path0;
    HD_PATH_ID mask_path1;
    HD_PATH_ID mask_path2;
    HD_PATH_ID mask_path3;
#endif

    HD_VIDEOOUT_HDMI_ID hdmi_id;
} VIDEO_LIVEVIEW;

typedef struct _ODT_MEM_PARM {
	UINT32 pa;
	UINT32 va;
	UINT32 size;
    UINT32 blk;
} ODT_MEM_PARM;

typedef struct _DET_RESULT
{
	INT32 	x;
	INT32 	y;
	INT32 	w;
	INT32 	h;
}DET_RESULT;


static ODT_MEM_PARM g_mem = {0};
static HD_COMMON_MEM_VB_BLK g_blk_info[1];

typedef struct _DET_THREAD_PARM {
    ODT_MEM_PARM mem;
    VIDEO_LIVEVIEW stream;
} DET_THREAD_PARM;

typedef struct _DET_THREAD_DRAW_PARM {
    ODT_MEM_PARM det_mem;
    VIDEO_LIVEVIEW stream;
} DET_THREAD_DRAW_PARM;

static UINT32 g_shdr_mode = 0;

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
    UINT32 mem_size = 0;

#if DET_MODE
    //ADD DET MEM_SIZE
#endif
#if DIS_MODE
	mem_size += nvt_dis_cal_buffer_size();
#endif
#if ODT_MODE
	mem_size += odt_calcbuffsize(FALSE);
    //printf("^^^odt_calcbuffsize^^^0x%x\n\r", mem_size);

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

#if USE_DRAM2
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

#if USE_DRAM2
    HD_COMMON_MEM_DDR_ID      ddr_id = DDR_ID1;
#else
    HD_COMMON_MEM_DDR_ID      ddr_id = DDR_ID0;
#endif

#if DET_MODE
    //ADD DET MEM_SIZE.
#endif
#if DIS_MODE
	mem_size += nvt_dis_cal_buffer_size();
#endif
#if ODT_MODE
	mem_size += odt_calcbuffsize(FALSE);
#endif

	/* Allocate parameter buffer */
	if (g_mem.va != 0) {
		printf("err: mem has already been inited\r\n");
		return -1;
	}
    blk = hd_common_mem_get_block(HD_COMMON_MEM_CNN_POOL, mem_size, ddr_id);
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		printf("hd_common_mem_get_block fail\r\n");
		ret =  HD_ERR_NG;
		goto exit;
	}
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		printf("not get buffer, pa=%08x\r\n", (int)pa);
		return -1;
	}
	va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, mem_size);
	g_blk_info[0] = blk;

	/* Release buffer */
	if (va == 0) {
		ret = hd_common_mem_munmap((void *)va, mem_size);
		if (ret != HD_OK) {
			printf("mem unmap fail\r\n");
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

#if DET_MODE
    //ADD DET MEM_SIZE
#endif
#if DIS_MODE
	mem_size += nvt_dis_cal_buffer_size();
#endif
#if ODT_MODE
	mem_size += odt_calcbuffsize(FALSE);
#endif

	/* Release in buffer */
	if (g_mem.va) {
		ret = hd_common_mem_munmap((void *)g_mem.va, mem_size);
		if (ret != HD_OK) {
			printf("mem_uninit : (g_mem.va)hd_common_mem_munmap fail.\r\n");
			return ret;
		}
	}
	//ret = hd_common_mem_release_block((HD_COMMON_MEM_VB_BLK)g_mem.pa);
	ret = hd_common_mem_release_block(g_blk_info[0]);
	if (ret != HD_OK) {
		printf("mem_uninit : (g_mem.pa)hd_common_mem_release_block fail.\r\n");
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

#if DET_DRAW
	//open a mask in videoout
	if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_0, &p_stream->mask_path0)) != HD_OK)
		return ret;
    if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_1, &p_stream->mask_path1)) != HD_OK)
		return ret;
    if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_2, &p_stream->mask_path2)) != HD_OK)
		return ret;
    if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_3, &p_stream->mask_path3)) != HD_OK)
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

static HD_RESULT open_module_extend2(VIDEO_LIVEVIEW *p_stream, HD_DIM* p_proc_max_dim, UINT32 out_type)
{
	HD_RESULT ret;
	// set videoout config
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, EXTEND_PATH2, &p_stream->proc_alg_path)) != HD_OK)
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

#if DET_DRAW
    if ((ret = hd_videoout_close(p_stream->mask_path0)) != HD_OK)
		return ret;
    if ((ret = hd_videoout_close(p_stream->mask_path1)) != HD_OK)
		return ret;
    if ((ret = hd_videoout_close(p_stream->mask_path2)) != HD_OK)
		return ret;
    if ((ret = hd_videoout_close(p_stream->mask_path3)) != HD_OK)
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

#if (DET_DRAW)
static int init_mask_param(HD_PATH_ID mask_path)
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
#endif

#if ((DIS_MODE && ODT_MODE) && (DET_DRAW))
static int odt_mask_draw(HD_PATH_ID mask_path, odt_obj *p_face, BOOL bdraw, UINT32 color)
{
    HD_OSG_MASK_ATTR attr;
    HD_RESULT ret = HD_OK;
    UINT32 color_pool[4] = {0x00FF0000, 0x0000FF00, 0x000000FF, 0x00FFFFFF};
    memset(&attr, 0, sizeof(HD_OSG_MASK_ATTR));
    if(!bdraw)
        return init_mask_param(mask_path);
    attr.position[0].x = p_face->obj_rect.x;
    attr.position[0].y = p_face->obj_rect.y;
    attr.position[1].x = p_face->obj_rect.x + p_face->obj_rect.w;
    attr.position[1].y = p_face->obj_rect.y;
    attr.position[2].x = p_face->obj_rect.x + p_face->obj_rect.w;
    attr.position[2].y = p_face->obj_rect.y + p_face->obj_rect.h;
    attr.position[3].x = p_face->obj_rect.x;
    attr.position[3].y = p_face->obj_rect.y + p_face->obj_rect.h;
    attr.type          = HD_OSG_MASK_TYPE_HOLLOW;
    attr.alpha         = 255;
    attr.color         = color_pool[p_face->id % 4];
    attr.thickness     = 2;
    ret = hd_videoout_set(mask_path, HD_VIDEOOUT_PARAM_OUT_MASK_ATTR, &attr);
    return ret;
}
#endif
#if ((DIS_MODE && ODT_MODE) && DET_DRAW)
static int odt_draw_info(odt_obj *p_fd_odt, UINT32 fd_odt_num, VIDEO_LIVEVIEW *p_stream)
{
    HD_RESULT ret = HD_OK;
    odt_mask_draw(p_stream->mask_path0, p_fd_odt + 0, (BOOL)(fd_odt_num >= 1), 0x00FF0000);
    odt_mask_draw(p_stream->mask_path1, p_fd_odt + 1, (BOOL)(fd_odt_num >= 2), 0x00FF0000);
    odt_mask_draw(p_stream->mask_path2, p_fd_odt + 2, (BOOL)(fd_odt_num >= 3), 0x00FF0000);
    odt_mask_draw(p_stream->mask_path3, p_fd_odt + 3, (BOOL)(fd_odt_num >= 4), 0x00FF0000);
    return ret;
}
#endif

VOID *odt_draw_thread(VOID *arg)
{
    DET_THREAD_DRAW_PARM *p_draw_parm = (DET_THREAD_DRAW_PARM*)arg;
    VIDEO_LIVEVIEW stream = p_draw_parm->stream;

	HD_VIDEO_FRAME video_frame = {0};
    HD_RESULT ret = HD_OK;

#if (DET_DRAW)
    #if (DIS_MODE && ODT_MODE)
    	UINT32				p_loc;
    	UINT32				DETnumber = 0;
    	static odt_obj		ODTInfo[ODT_NUM_MAX] = {0};
    	HD_URECT DispCord = {0, 0, OSG_LCD_WIDTH, OSG_LCD_HEIGHT};
    #endif
#endif

    // wait det init ready
    sleep(2);

    while(1)
    {
        ret = hd_videoproc_pull_out_buf(stream.proc_alg_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
        if(ret != HD_OK)
        {
            printf("ERR : hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
            break;
        }

#if DET_DRAW
	#if (DIS_MODE && ODT_MODE)
		if(odt_regist_state(NVT_ODT_FDCNN) == ODT_STA_OK)
		{
			DETnumber = odt_getrsltobj_for_func(ODTInfo, NVT_ODT_FDCNN, &DispCord, &p_loc);
            //printf("draw objnumber = %d, x =%d, y=%d, w=%d, h=%d\r\n",DETnumber, ODTInfo[0].obj_rect.x,ODTInfo[0].obj_rect.y,ODTInfo[0].obj_rect.w,ODTInfo[0].obj_rect.h);
			odt_draw_info(ODTInfo, DETnumber, &stream);
		}
	#endif
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


static ODT_MEM_PARM det_getmem(ODT_MEM_PARM *valid_mem, UINT32 required_size)
{
	ODT_MEM_PARM mem = {0};
	required_size = ALIGN_CEIL_4(required_size);
	if(required_size <= valid_mem->size) {
		mem.va = valid_mem->va;
        mem.pa = valid_mem->pa;
		mem.size = required_size;

		valid_mem->va += required_size;
        valid_mem->pa += required_size;
		valid_mem->size -= required_size;
	} else {
		printf("required size %d > total memory size %d\r\n", required_size, valid_mem->size);
	}
	return mem;
}

#define DET_MAX_OUTNUM 10
VOID *det_thread(VOID *arg)
{
    DET_THREAD_PARM *p_det_parm = (DET_THREAD_PARM*)arg;
    VIDEO_LIVEVIEW stream = p_det_parm->stream;

    HD_VIDEO_FRAME video_frame = {0};
    HD_RESULT ret = HD_OK;

#if ((DIS_MODE && ODT_MODE))
    static DET_RESULT det_info[DET_MAX_OUTNUM] = {0};
    UINT32 det_num = 0;
    static HD_URECT det_size = {0, 0, VDO_SIZE_W, VDO_SIZE_H};
#endif

#if DET_PROF
    static struct timeval tstart0, tend0;
    static UINT64 cur_time0 = 0, mean_time0 = 0, sum_time0 = 0;
    static UINT32 icount = 0;
#endif


#if (DIS_MODE && ODT_MODE)
    UINT32 j = 0;
    odt_rslt_loc                    TimeStamp = {0};
    static odt_od_result            det_odt_rslts[DET_MAX_OUTNUM] = {0};
    static INT32                    regist_flag = ODT_STA_ERROR;
    static UINT32                   det_rslt_loc;
    static odt_process_buf_param    det_odt_buf_param;
    static odt_rslt_inf             det_buf1[ODT_MAX_DISRSLTBUF_NUM];
    static odt_rslt_inf             det_buf2;
    static odt_od_result            det_buf3[DET_MAX_OUTNUM];
    det_odt_buf_param.detect_rslt_addr        = (UINT32)det_buf3;
    det_odt_buf_param.odt_od_rslt_buffer_addr = (UINT32)(&det_buf2);
    det_odt_buf_param.odt_rslt_buffer_addr    = (UINT32)det_buf1;
#endif
    while(1)
    {
#if (DIS_MODE && ODT_MODE)
        if (regist_flag == ODT_STA_ERROR) {
			regist_flag = odt_regist(&det_odt_buf_param, NVT_ODT_FDCNN);
			if (regist_flag == ODT_STA_ERROR) {
				continue;
			}else{
				odt_config(odt_cfg_max_obj_life1, 15, NVT_ODT_FDCNN);
				odt_config(odt_cfg_max_obj_life2, 1, NVT_ODT_FDCNN);
			}
		}
#endif
#if DET_PROF
       gettimeofday(&tstart0, NULL);
#endif

        ret = hd_videoproc_pull_out_buf(stream.proc_alg_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
        if(ret != HD_OK)
        {
            printf("hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
            goto exit;

        }
#if (DIS_MODE && ODT_MODE)
		TimeStamp.timestamp_sec  = (video_frame.count >> 32) & 0xFFFFFFFF;
		TimeStamp.timestamp_usec = video_frame.count & 0xFFFFFFFF;
		det_rslt_loc = (UINT32)&TimeStamp;
#endif

        ret = hd_videoproc_release_out_buf(stream.proc_alg_path, &video_frame);
        if(ret != HD_OK)
        {
            printf("hd_videoproc_release_out_buf fail (%d)\n\r", ret);
            goto exit;
        }

#if DET_PROF
        gettimeofday(&tend0, NULL);
        cur_time0 = (UINT64)(tend0.tv_sec - tstart0.tv_sec) * 1000000 + (tend0.tv_usec - tstart0.tv_usec);
        sum_time0 += cur_time0;
        mean_time0 = sum_time0/(++icount);
        #if (!DET_FIX_FRM)
        printf("[DET] process cur time(us): %lld, mean time(us): %lld\r\n", cur_time0, mean_time0);
        #endif
#endif

#if DET_FIX_FRM
        if (cur_time0 < 1000000)
		    usleep(1000000 - cur_time0 + (mean_time0*0));
#endif

#if (DIS_MODE && ODT_MODE)
		det_size.w = 960;
        det_size.h = 540;
        det_num = 1;

        det_info[0].x = ( 900 * det_size.w + (VDO_SIZE_W>>1) ) / VDO_SIZE_W + det_size.x;
        det_info[0].y = ( 420 * det_size.h + (VDO_SIZE_H>>1) ) / VDO_SIZE_H + det_size.y;
        det_info[0].w = ( 120 * det_size.w + (VDO_SIZE_W>>1) ) / VDO_SIZE_W;
        det_info[0].h = ( 240 * det_size.h + (VDO_SIZE_H>>1) ) / VDO_SIZE_H;

#endif
#if (DIS_MODE && ODT_MODE)
		for(j = 0; j < det_num; j++)
		{
			det_odt_rslts[j].x = det_info[j].x;
			det_odt_rslts[j].y = det_info[j].y;
			det_odt_rslts[j].w = det_info[j].w;
			det_odt_rslts[j].h = det_info[j].h;
			det_odt_rslts[j].score = 1;
			det_odt_rslts[j].class = 0;
			det_odt_rslts[j].valid = TRUE;
		}
        //printf("fdcnn_odt_rslts: x = %d, y = %d, w = %d, h = %d\n\r", fdcnn_odt_rslts[0].x,fdcnn_odt_rslts[0].y,fdcnn_odt_rslts[0].w,fdcnn_odt_rslts[0].h);
		odt_setodrslt_timestamp(det_num, det_odt_rslts, det_rslt_loc, NVT_ODT_FDCNN);
#endif
    }
exit:
    return 0;
}

#if DIS_MODE
static DIS_IPC_INIT dis_getmem(ODT_MEM_PARM *valid_mem, UINT32 required_size)
{
	DIS_IPC_INIT mem = {0};
	required_size = ALIGN_CEIL_4(required_size);
	if(required_size <= valid_mem->size) {
		mem.addr = valid_mem->va;
		mem.size = required_size;

		valid_mem->va += required_size;
        valid_mem->pa += required_size;
		valid_mem->size -= required_size;
	} else {
		printf("ERR : required size %d > total memory size %d\r\n", required_size, valid_mem->size);
	}
	return mem;
}
VOID *dis_thread_api(VOID *arg)
{
	HD_RESULT ret = HD_OK;
	DIS_IPC_INIT   *share_mem = (DIS_IPC_INIT*)arg;

	//allocate memory (parm, model, io buffer)
	ret = nvt_dis_init(share_mem);
	if (ret != HD_OK) {
		printf("err:nvt_dis_init error %d\r\n",ret);
		goto dis_exit;
	}
	while(1) {
			ret = nvt_dis_only_calc_vector();
			if (ret != HD_OK) {
				printf("err:nvt_dis_only_calc_vector error %d\r\n",ret);
				goto dis_exit;
			} else {
				odt_savedisrslt();
			}
	}
dis_exit:
	ret = nvt_dis_uninit();
	if (ret != HD_OK) {
		printf("err:nvt_dis_uninit error %d\r\n",ret);
	}
	return 0;
}
#endif
#if ODT_MODE
static ODT_IPC_INIT odt_getmem(ODT_MEM_PARM *valid_mem, UINT32 required_size)
{
	ODT_IPC_INIT mem = {0};
	required_size = ALIGN_CEIL_4(required_size);
	if(required_size <= valid_mem->size) {
		mem.addr = valid_mem->va;
		mem.size = required_size;
		valid_mem->va += required_size;
        valid_mem->pa += required_size;
		valid_mem->size -= required_size;
	} else {
		printf("ERR : required size %d > total memory size %d\r\n", required_size, valid_mem->size);
	}
	return mem;
}
VOID *odt_thread_api(VOID *arg)
{
	HD_RESULT ret = HD_OK;
	#if ODT_PROF
    static struct timeval tstart0, tend0;
    static UINT64 cur_time0 = 0, mean_time0 = 0, sum_time0 = 0;
    static UINT32 icount = 0;
	#endif
	while(1) {
		if (!odt_getinitstate()) {
			return 0;
		}
		#if ODT_PROF
        	gettimeofday(&tstart0, NULL);
		#endif
		odt_ot_process();
		odt_od_process();
		#if ODT_PROF
	        gettimeofday(&tend0, NULL);
	        cur_time0 = (UINT64)(tend0.tv_sec - tstart0.tv_sec) * 1000000 + (tend0.tv_usec - tstart0.tv_usec);
	        sum_time0 += cur_time0;
	        mean_time0 = sum_time0/(++icount);
			#if(!ODT_FIX_FRM)
	        	printf("[ODT] process cur time(us): %lld, mean time(us): %lld\r\n", cur_time0, mean_time0);
			#endif
		#endif
		#if ODT_FIX_FRM
        if (cur_time0 < 30000)
		    usleep(30000 - cur_time0 + (mean_time0*0));
		#endif
	}
	ret = odt_uninit();
	if (ret != HD_OK) {
		printf("err:odt_uninit error %d\r\n",ret);
	}
	return 0;
}
#endif
int main(int argc, char** argv)
{
	HD_RESULT ret;
	UINT32 out_type = 1;
    VIDEO_LIVEVIEW stream[3] = {0};

	g_shdr_mode = ODT_USE_HDR;

	AET_CFG_INFO cfg_info = {0};
	
	CHAR odt_alg_version[32];

	// query program options
	if (argc >= 2) {
		out_type = atoi(argv[1]);
		printf("out_type %d\r\n", out_type);
		if(out_type > 2) {
			printf("error: not support out_type!\r\n");
			return 0;
		}
	}
    stream[0].hdmi_id=HD_VIDEOOUT_HDMI_1920X1080I60;//default

	// query program options
	if (argc >= 3 && (atoi(argv[2]) !=0)) {
		stream[0].hdmi_id = atoi(argv[2]);
		printf("hdmi_mode %d\r\n", stream[0].hdmi_id);
	}
	
	//get odt version
     odt_get_version(odt_alg_version);
     printf("odt_alg_version: %s\n\r", odt_alg_version);

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
	}

	// init memory
	ret = mem_init();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		goto exit;
	}

	ret = get_mem_block();
	if (ret != HD_OK) {
		printf("mem_init fail=%d\n", ret);
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
		printf("hd_gfx_init fail=%d\n", ret);
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

    stream[2].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[2].proc_max_dim.h = VDO_SIZE_H; //assign by user
	ret = open_module_extend2(&stream[2], &stream[2].proc_max_dim, out_type);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

#if DET_DRAW
	// must set once before hd_videoout_start
	if(init_mask_param(stream[0].mask_path0)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
	if(init_mask_param(stream[0].mask_path1)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
    if(init_mask_param(stream[0].mask_path2)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
    if(init_mask_param(stream[0].mask_path3)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
#endif
	//render mask
#if DET_DRAW
	ret = hd_videoout_start(stream[0].mask_path0);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoout_start(stream[0].mask_path1);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoout_start(stream[0].mask_path2);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoout_start(stream[0].mask_path3);
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

	stream[2].proc_alg_max_dim.w = VDO_SIZE_W;
	stream[2].proc_alg_max_dim.h = VDO_SIZE_H;
    ret = set_proc_param_extend(stream[2].proc_alg_path, SOURCE_PATH, &stream[2].proc_alg_max_dim, HD_VIDEO_DIR_NONE);
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
    hd_videoproc_start(stream[2].proc_alg_path);
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);
	hd_videoout_start(stream[0].out_path);

    ODT_MEM_PARM local_mem = g_mem;

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
#if DIS_MODE
        DIS_IPC_INIT dis_mem = {0};
        pthread_t dis_thread_id;
        UINT32 dis_mem_size = nvt_dis_cal_buffer_size();
        dis_mem = dis_getmem(&local_mem, dis_mem_size);
        ret = pthread_create(&dis_thread_id, NULL, dis_thread_api, (VOID*)(&dis_mem));
        if (ret < 0) {
            printf("create dis thread failed");
            goto exit;
        }
#endif
#if ODT_MODE
        ODT_IPC_INIT odt_mem = {0};
        pthread_t odt_thread_id;
        UINT32 odt_mem_size = odt_calcbuffsize(FALSE);
        odt_mem = odt_getmem(&local_mem, odt_mem_size);
        ret = odt_init(&odt_mem);
        if (ret != HD_OK) {
            printf("err:odt_init error %d\r\n",ret);
            goto odt_exit;
        }
        ret = pthread_create(&odt_thread_id, NULL, odt_thread_api, NULL);
        if (ret < 0) {
            printf("create odt_od thread failed");
            goto exit;
        }
#endif

#if DET_MODE
    DET_THREAD_PARM det_thread_parm;
    pthread_t det_thread_id;
    UINT32 det_mem_size = 0; //add det mem_size

    det_thread_parm.mem = det_getmem(&local_mem, det_mem_size); // alloc det mem_size
    det_thread_parm.stream = stream[1];

	ret = pthread_create(&det_thread_id, NULL, det_thread, (VOID*)(&det_thread_parm));
    if (ret < 0) {
        printf("create det thread failed");
        goto exit;
    }
#endif

#if DET_DRAW
    DET_THREAD_DRAW_PARM det_draw_parm;
    pthread_t det_draw_id;
    det_draw_parm.stream = stream[0];
    det_draw_parm.det_mem = det_thread_parm.mem;
    ret = pthread_create(&det_draw_id, NULL, odt_draw_thread, (VOID*)(&det_draw_parm));
    if (ret < 0) {
        printf("create fdcnn draw thread failed");
        goto exit;
    }
#endif

#if DIS_MODE
	pthread_join(dis_thread_id, NULL);
#endif
#if ODT_MODE
	pthread_join(odt_thread_id, NULL);
#endif

#if DET_MODE
    pthread_join(det_thread_id, NULL);
#endif

#if (DET_DRAW)
    pthread_join(det_draw_id, NULL);
#endif

	// stop video_liveview modules (main)
	hd_videocap_stop(stream[0].cap_path);
	hd_videoproc_stop(stream[0].proc_path);
	hd_videoproc_stop(stream[0].proc_alg_path);
    hd_videoproc_stop(stream[1].proc_alg_path);
    hd_videoproc_stop(stream[2].proc_alg_path);
	hd_videoout_stop(stream[0].out_path);

	// unbind video_liveview modules (main)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);
    //hd_videoproc_unbind(SOURCE_PATH);
    //hd_videoproc_unbind(EXTEND_PATH1);
    //hd_videoproc_unbind(EXTEND_PATH2);
#if ODT_MODE
odt_exit:
		ret = odt_uninit();
		if (ret != HD_OK) {
			printf("err:odt_uninit error %d\r\n",ret);
		}
#endif
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

    ret = close_module_extend(&stream[2]);
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
		printf("hd_gfx_uninit fail=%d\n", ret);
	}

	ret = release_mem_block();
	if (ret != HD_OK) {
		printf("mem_uninit fail=%d\n", ret);
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
