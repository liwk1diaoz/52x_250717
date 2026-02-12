/**
	@brief Sample code of video liveview with mode and pxlfmt.\n

	@file video_liveview_test.c

	@author Janice Huang

	@ingroup mhdal

	@note This file is modified from video_liveview.c.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_debug.h"
#include "vendor_videocapture.h"
#include "vendor_videoprocess.h"


// platform dependent
#if defined(__LINUX)
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#define GETCHAR()				getchar()
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#include <kwrap/examsys.h> 	//for MAIN(), GETCHAR() API
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_video_liveview_test, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
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

#define SEN_OUT_FMT     HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT     HD_VIDEO_PXLFMT_RAW12
#define SHDR_CAP_OUT_FMT HD_VIDEO_PXLFMT_RAW12_SHDR2
#define CA_WIN_NUM_W    32
#define CA_WIN_NUM_H    32
#define LA_WIN_NUM_W    32
#define LA_WIN_NUM_H    32
#define VA_WIN_NUM_W    16
#define VA_WIN_NUM_H    16
#define YOUT_WIN_NUM_W  128
#define YOUT_WIN_NUM_H  128
#define ETH_8BIT_SEL    0 //0: 2bit out, 1:8 bit out
#define ETH_OUT_SEL     1 //0: full, 1: subsample 1/2

#define RESOLUTION_SET  0 //0: 2M(IMX290), 1:5M(OS05A) 2: 2M (OS02K10) 3: 2M (AR0237IR)
#if ( RESOLUTION_SET == 0)
#define VDO_SIZE_W      1920
#define VDO_SIZE_H      1080
#elif (RESOLUTION_SET == 1)
#define VDO_SIZE_W      2592
#define VDO_SIZE_H      1944
#elif ( RESOLUTION_SET == 2)
#define VDO_SIZE_W      1920
#define VDO_SIZE_H      1080
#elif ( RESOLUTION_SET == 3)
#define VDO_SIZE_W      1920
#define VDO_SIZE_H      1080
#endif

#define VIDEOCAP_ALG_FUNC HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB
#define VIDEOPROC_ALG_FUNC HD_VIDEOPROC_FUNC_SHDR | HD_VIDEOPROC_FUNC_WDR | HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_COLORNR | HD_VIDEOPROC_FUNC_DEFOG

#define OUT_VPE_COUNT 2

#define PIP_STX    ALIGN_FLOOR(400, 4)
#define PIP_STY    ALIGN_FLOOR(120, 2)
#define PIP_WIDTH  ALIGN_FLOOR((960/3), 8)
#define PIP_HEIGHT ALIGN_FLOOR((240/3), 2)



static UINT32 g_shdr = 0; //0:disable, 1:enable SHDR

static UINT32 g_capbind = 0;  //0:D2D, 1:direct, 2: one-buf, 0xff: no-bind
static UINT32 g_capfmt = 0; //0:RAW, 1:RAW-compress
static UINT32 g_prcbind = 0xff;  //0:D2D, 1:lowlatency, 2: one-buf, 0xff: no-bind
static UINT32 g_prcfmt = 0; //0:YUV, 1:YUV-compress
static UINT32 g_pattern_gen = 0; //0:real sensor, 1:pattern_gen


#define SEN1_VCAP_ID 0
#define VCAP_SIE_MAP 1  // 0 for SIE1, 1 for SIE2
#define ISP_ID_REMAP_SIE(vcap_id, sie_map_id)  ((1 << 31) | (vcap_id << 24) | sie_map_id)


///////////////////////////////////////////////////////////////////////////////

static HD_RESULT get_cap_sysinfo(HD_PATH_ID video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_SYSINFO sys_info = {0};

	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
	printf("sys_info.devid =0x%X, cur_fps[0]=%d/%d, vd_count=%llu, output_started=%d, cur_dim(%dx%d)\r\n",
		sys_info.dev_id, GET_HI_UINT16(sys_info.cur_fps[0]), GET_LO_UINT16(sys_info.cur_fps[0]), sys_info.vd_count, sys_info.output_started, sys_info.cur_dim.w, sys_info.cur_dim.h);
	return ret;
}

static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	if ((g_capbind == 0) || (g_capbind == 0xff)) {
		//normal
		mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()
        													+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
        													+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
		if (g_capfmt == HD_VIDEO_PXLFMT_NRX12)
			mem_cfg.pool_info[0].blk_size += VDO_NRX_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H);
		else
			mem_cfg.pool_info[0].blk_size += VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, g_capfmt);
		mem_cfg.pool_info[0].blk_cnt = 2;
		mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	}
	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	if ((g_prcbind == 0) || (g_prcbind == 0xff)) {
		//normal
		mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
		mem_cfg.pool_info[1].blk_cnt = 4;
		mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	}


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

static HD_RESULT get_cap_caps(HD_PATH_ID video_cap_ctrl, HD_VIDEOCAP_SYSCAPS *p_video_cap_syscaps)
{
	HD_RESULT ret = HD_OK;
	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSCAPS, p_video_cap_syscaps);
	return ret;
}

static HD_RESULT set_cap_cfg(HD_PATH_ID *p_video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};

    #if (RESOLUTION_SET == 0)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
	printf("Using nvt_sen_imx290\n");
	#elif (RESOLUTION_SET == 1)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os05a10");
	printf("Using nvt_sen_os05a10\n");
	#elif (RESOLUTION_SET == 2)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os02k10");
	printf("Using nvt_sen_os02k10\n");
	#elif (RESOLUTION_SET == 3)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_ar0237ir");
	printf("Using nvt_sen_ar0237ir\n");
	#endif

	if (g_pattern_gen) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, HD_VIDEOCAP_SEN_PAT_GEN);
	}
    if(RESOLUTION_SET == 3) {
        cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_P_RAW;
	    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x204; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	    printf("Parallel interface\n");
    }
    if(RESOLUTION_SET == 0 || RESOLUTION_SET == 1 || RESOLUTION_SET == 2) {
	    cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI
	    printf("MIPI interface\n");
    }
	if (g_shdr == 1) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		printf("Using g_shdr_mode\n");
	} else {
		#if (RESOLUTION_SET == 0)
		printf("Using imx290\n");
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		#elif (RESOLUTION_SET == 1)
		printf("Using OS052A\n");
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		#elif (RESOLUTION_SET == 2)
		printf("Using OS02K10\n");
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		#endif
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
	if (g_shdr == 1) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
	} else {
		#if (RESOLUTION_SET == 0)
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
		#elif (RESOLUTION_SET == 1)
			printf("Using OS052A or shdr\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
		#elif (RESOLUTION_SET == 2)
			printf("Using OS02K10 or shdr\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
		#endif
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control

	if (ret != HD_OK) {
		return ret;
	}

	if (g_shdr == 1) {
		cap_cfg.sen_cfg.shdr_map = HD_VIDEOCAP_SHDR_MAP(HD_VIDEOCAP_HDR_SENSOR1, (HD_VIDEOCAP_0|HD_VIDEOCAP_1));
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = VIDEOCAP_ALG_FUNC;

	if (g_shdr == 1) {
		iq_ctl.func |= HD_VIDEOCAP_FUNC_SHDR;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}

static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim, UINT32 path)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};

		if (g_pattern_gen) {
			video_in_param.sen_mode = HD_VIDEOCAP_PATGEN_MODE(HD_VIDEOCAP_SEN_PAT_COLORBAR, 200);
		} else {
			video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
		}
		video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		video_in_param.pxlfmt = SEN_OUT_FMT;

		// NOTE: only SHDR with path 1
		if ((path == 0) && (g_shdr == 1)) {
			video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_2;
		} else {
			video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
		}

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
		// NOTE: only SHDR with path 1
		if ((path == 0) && (g_shdr == 1)) {
			if (g_capfmt == HD_VIDEO_PXLFMT_NRX12) {
				video_out_param.pxlfmt = HD_VIDEO_PXLFMT_NRX12_SHDR2;
			} else {
				video_out_param.pxlfmt = SHDR_CAP_OUT_FMT;
			}
		} else {
			video_out_param.pxlfmt = g_capfmt;
		}

		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.depth = 0;
		if (g_capbind == 0xff) //no-bind mode
			video_out_param.depth = 1;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
	}
	{
		HD_VIDEOCAP_FUNC_CONFIG video_path_param = {0};

		video_path_param.out_func = 0;
		if (g_capbind == 1) //direct mode
			video_path_param.out_func = HD_VIDEOCAP_OUTFUNC_DIRECT;
		if (g_capbind == 2) //one-buf mode
			video_path_param.out_func = HD_VIDEOCAP_OUTFUNC_ONEBUF;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_FUNC_CONFIG, &video_path_param);
		//printf("set_cap_param PATH_CONFIG=0x%X\r\n", ret);
	}
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_proc_cfg(HD_PATH_ID *p_video_proc_ctrl, HD_DIM* p_max_dim, HD_OUT_ID _out_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
	HD_VIDEOPROC_LL_CONFIG video_ll_param = {0};
	HD_PATH_ID video_proc_ctrl = 0;

	ret = hd_videoproc_open(0, _out_id, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	if (p_max_dim != NULL ) {
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWALL;
		video_cfg_param.isp_id = 0;
		video_cfg_param.ctrl_max.func = 0;
		if (g_shdr == 1) {
			video_cfg_param.ctrl_max.func |= HD_VIDEOPROC_FUNC_SHDR;
		}
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;
		if (g_shdr == 1) {
			if (g_capfmt == HD_VIDEO_PXLFMT_NRX12) {
				video_cfg_param.in_max.pxlfmt = HD_VIDEO_PXLFMT_NRX12_SHDR2;
			} else {
				video_cfg_param.in_max.pxlfmt = SHDR_CAP_OUT_FMT;
			}
		} else {
		video_cfg_param.in_max.pxlfmt = g_capfmt;
		}
		video_cfg_param.in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &video_cfg_param);
		if (ret != HD_OK) {
			return HD_ERR_NG;
		}
	}
	{
		HD_VIDEOPROC_FUNC_CONFIG video_path_param = {0};

		video_path_param.in_func = 0;
		if (g_capbind == 1)
			video_path_param.in_func |= HD_VIDEOPROC_INFUNC_DIRECT; //direct NOTE: enable direct
		if (g_capbind == 2)
			video_path_param.in_func |= HD_VIDEOPROC_INFUNC_ONEBUF;
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
		//printf("set_proc_param PATH_CONFIG=0x%X\r\n", ret);
	}

	video_ctrl_param.func = 0;
	if (g_shdr == 1) {
		video_ctrl_param.func |= HD_VIDEOPROC_FUNC_SHDR;
	}
	video_ctrl_param.ref_path_3dnr = 0;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);

	video_ll_param.delay_trig_lowlatency = 0;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_LL_CONFIG, &video_ll_param);

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
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = 0;
		if (g_prcbind == 0xff) //no-bind mode
			video_out_param.depth = 1;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}
	{
		HD_VIDEOPROC_FUNC_CONFIG video_path_param = {0};

		video_path_param.out_func = 0;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
	}

	return ret;
}
static HD_RESULT set_proc2_cfg(HD_PATH_ID *p_video_proc_ctrl, HD_DIM* p_max_dim, HD_OUT_ID ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
	HD_PATH_ID video_proc_ctrl = 0;

	ret = hd_videoproc_open(0, ctrl, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	if (p_max_dim != NULL ) {
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_VPE;
		video_cfg_param.isp_id = HD_ISP_DONT_CARE;

		video_cfg_param.ctrl_max.func = 0;
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;
		video_cfg_param.in_max.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
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
static HD_RESULT set_proc2_param(HD_PATH_ID video_proc_path, HD_DIM* p_dim, HD_URECT* p_rect, HD_IRECT* p_pre_crop, HD_IRECT* p_hole)
{
	HD_RESULT ret = HD_OK;

	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT video_out_param = {0};
		if (p_rect == NULL) {
			video_out_param.dim.w = p_dim->w;
			video_out_param.dim.h = p_dim->h;
		} else {
			video_out_param.dim.w = 0; //dim must be zero for specified out window
			video_out_param.dim.h = 0;
			video_out_param.bg.w = p_dim->w;
			video_out_param.bg.h = p_dim->h;
			video_out_param.rect.x = p_rect->x;
			video_out_param.rect.y = p_rect->y;
			video_out_param.rect.w = p_rect->w;
			video_out_param.rect.h = p_rect->h;
		}
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = 1;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}
	if (p_pre_crop) {
		HD_VIDEOPROC_CROP  video_crop_param = {0};

		video_crop_param.mode  = HD_CROP_ON;
		video_crop_param.win.rect.x = p_pre_crop->x;
		video_crop_param.win.rect.y = p_pre_crop->y;
		video_crop_param.win.rect.w = p_pre_crop->w;
		video_crop_param.win.rect.h = p_pre_crop->h;
		hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_IN_CROP_PSR, &video_crop_param);
	}
	if (p_hole) {
		HD_VIDEOPROC_CROP  video_crop_param = {0};

		video_crop_param.mode  = HD_CROP_ON;
		video_crop_param.win.rect.x = p_hole->x;
		video_crop_param.win.rect.y = p_hole->y;
		video_crop_param.win.rect.w = p_hole->w;
		video_crop_param.win.rect.h = p_hole->h;
		hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_CROP_PSR, &video_crop_param);
	}
	return ret;
}
///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_out_cfg(HD_PATH_ID *p_video_out_ctrl, UINT32 out_type, HD_VIDEOOUT_HDMI_ID hdmi_id)
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
	switch(out_type) {
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

	if (ret != HD_OK) {
		return ret;
	}

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
	HD_VIDEOOUT_IN video_out_param = {0};
	HD_VIDEOOUT_FUNC_CONFIG videoout_cfg = {0};

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
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_FUNC_CONFIG, &videoout_cfg);


	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _VIDEO_LIVEVIEW {

	// (1)
	HD_VIDEOCAP_SYSCAPS cap_syscaps;
	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	HD_DIM  cap_dim;

	// only used when (g_capbind = 0xff)  //no-bind mode
	pthread_t  cap_thread_id;
	UINT32	cap_enter;
	UINT32	cap_exit;
	UINT32	cap_count;
	UINT32 	cap_loop;

	// (2)
	HD_DIM  proc_max_dim;
	HD_VIDEOPROC_SYSCAPS proc_syscaps;
	HD_PATH_ID proc_ctrl;
	HD_PATH_ID proc_path;
	HD_PATH_ID proc_path2; //for g_3dnr
	HD_PATH_ID proc2_ctrl;
	HD_PATH_ID proc2_path[OUT_VPE_COUNT];

	// only used when (g_prcbind = 0xff)  //no-bind mode
	pthread_t  prc_thread_id;
	UINT32	prc_enter;
	UINT32	prc_exit;
	UINT32 	prc_count;
	UINT32 	prc_loop;
	UINT32  cap_sec_start;
	UINT32  proc_sec_start;

	// (3)
	HD_DIM  out_max_dim;
	HD_DIM  out_dim;
	HD_VIDEOOUT_SYSCAPS out_syscaps;
	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;
	HD_VIDEOOUT_HDMI_ID hdmi_id;


} VIDEO_LIVEVIEW;

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
	ret = set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim, HD_VIDEOPROC_0_CTRL);
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}

	ret = set_proc2_cfg(&p_stream->proc2_ctrl, p_proc_max_dim, HD_VIDEOPROC_1_CTRL);
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
	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_1_IN_0, HD_VIDEOPROC_1_OUT_0, &p_stream->proc2_path[0])) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_1_IN_0, HD_VIDEOPROC_1_OUT_1, &p_stream->proc2_path[1])) != HD_OK)
		return ret;
	if ((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0, &p_stream->out_path)) != HD_OK)
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
	if ((ret = hd_videoproc_close(p_stream->proc2_path[0])) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc2_path[1])) != HD_OK)
		return ret;
	if ((ret = hd_videoout_close(p_stream->out_path)) != HD_OK)
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

// only used when (g_capbind = 0xff)  //no-bind mode
static void *capture_thread(void *arg)
{
	VIDEO_LIVEVIEW* p_stream0 = (VIDEO_LIVEVIEW *)arg;
	HD_RESULT ret = HD_OK;

	HD_VIDEO_FRAME video_frame = {0};

	p_stream0->cap_exit = 0;
	p_stream0->cap_loop = 0;
	p_stream0->cap_count = 0;
	p_stream0->cap_sec_start = 1;
	//------ wait flow_start ------
	while (p_stream0->cap_enter == 0) usleep(100);

	//--------- pull data test ---------
	while (p_stream0->cap_exit == 0) {

		//printf("cap_pull ....\r\n");
		ret = hd_videocap_pull_out_buf(p_stream0->cap_path, &video_frame, -1); // -1 = blocking mode
		if (ret != HD_OK) {
			if (ret != HD_ERR_UNDERRUN)
			printf("cap_pull error=%d !!\r\n\r\n", ret);
    			goto skip;
		}
		//printf("cap frame.count = %llu\r\n", video_frame.count);

		//printf("proc_push ....\r\n");
		ret = hd_videoproc_push_in_buf(p_stream0->proc_path, &video_frame, NULL, 0); // only support non-blocking mode now
		if (ret != HD_OK) {
			printf("proc_push error=%d !!\r\n\r\n", ret);
    			goto skip;
		}

		//printf("cap_release ....\r\n");
		ret = hd_videocap_release_out_buf(p_stream0->cap_path, &video_frame);
		if (ret != HD_OK) {
			printf("cap_release error=%d !!\r\n\r\n", ret);
    			goto skip;
		}

		p_stream0->cap_count ++;
		//printf("capture count = %d\r\n", p_stream0->cap_count);
skip:
		p_stream0->cap_loop++;
		usleep(100); //sleep for getchar()
		while (p_stream0->cap_sec_start == 0) {
			//printf("vcap thread pause\r\n");
			usleep(100);
		}
	}

	return 0;
}
static BOOL process_running = FALSE;
static void wait_process_stopped(void)
{
	while (process_running) {
		usleep(100);
	}
}
// only used when (g_prcbind = 0xff)  //no-bind mode
static void *process_thread(void *arg)
{
	VIDEO_LIVEVIEW* p_stream0 = (VIDEO_LIVEVIEW *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEO_FRAME video_frame = {0};
	UINT32 out_blk_size = 0;
	HD_COMMON_MEM_VB_BLK out_blk = 0;
	HD_VIDEO_FRAME out_video_frame[2];
	UINT32 j;
	UINT32 pull_retry[OUT_VPE_COUNT];

	p_stream0->prc_exit = 0;
	p_stream0->prc_loop = 0;
	p_stream0->prc_count = 0;
	p_stream0->proc_sec_start = 1;
	//------ wait flow_start ------
	while (p_stream0->prc_enter == 0) usleep(100);

	//--------- pull data test ---------
	while (p_stream0->prc_exit == 0) {
		process_running= TRUE;
		//printf("proc_pull ....\r\n");
		ret = hd_videoproc_pull_out_buf(p_stream0->proc_path, &video_frame, -1); // -1 = blocking mode
		if (ret != HD_OK) {
			if (ret != HD_ERR_UNDERRUN)
			printf("proc_pull error=%d !!\r\n\r\n", ret);
    		goto skip2;
		}
		//printf("proc frame.count = %llu\r\n", video_frame.count);

		//-- prepare output block for VPE
		out_blk_size = (DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(p_stream0->out_max_dim.w, p_stream0->out_max_dim.h, HD_VIDEO_PXLFMT_YUV420));
		out_blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, out_blk_size, DDR_ID0); // Get block from mem pool
		if (out_blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get out_blk fail\r\n");
			goto skip2;
		}
		#if 0//debug only
		UINT32 pa;
		UINT32 out_va = 0;
		pa = hd_common_mem_blk2pa(out_blk);
		//printf("\r\n##OUT BLK=0x%X, PA=0x%X, SIZE=0x%X##\r\n", out_blk, pa, out_blk_size);

		if (pa == 0) {
			printf("hd_common_mem_blk2pa error !!\r\n\r\n");
		} else {
			out_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, out_blk_size); // Get virtual addr
			if (out_va) {
				//clear buffer
				//Do NOT use this method to clear buffer in real procuct!
				memset((void *)out_va, 0, out_blk_size);
				hd_common_mem_flush_cache((void *)out_va, out_blk_size);
			} else {
				printf("Error: mmap fail !! pa=0x%X, blk=0x%x\r\n", pa, out_blk);
			}
		}
		ret = hd_common_mem_munmap((void *)out_va, out_blk_size);
		if (ret != HD_OK) {
			printf("mnumap error !!\r\n\r\n");
		}
		#endif

		memset((void *)out_video_frame, 0, sizeof(out_video_frame));
		for (j = 0; j < OUT_VPE_COUNT; j++) {
			out_video_frame[j].sign = MAKEFOURCC('V','F','R','M');
			out_video_frame[j].blk = out_blk;
			out_video_frame[j].reserved[0] = out_blk_size;
			if (j < (OUT_VPE_COUNT-1)) {
				out_video_frame[j].p_next = (HD_METADATA *)&out_video_frame[j+1];
			}
		}
		ret = hd_videoproc_push_in_buf(p_stream0->proc2_path[0], &video_frame, &out_video_frame[0], 0); // only support non-blocking mode
		if (ret != HD_OK) {
			printf("push_in(%d) error = %d!!\r\n", 0, ret);
			goto RELEASE_VPE_OUT;
		}
		//wait output finish
		memset((void *)out_video_frame, 0, sizeof(out_video_frame));
		memset((void *)pull_retry, 0, sizeof(pull_retry));
		#if 1
		for (j = 0; j < OUT_VPE_COUNT; j++) {
PULL_RETRY:
			ret = hd_videoproc_pull_out_buf(p_stream0->proc2_path[j], &out_video_frame[j], -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
			if (ret != HD_OK) {
				if (ret == HD_ERR_UNDERRUN) {//only for wait_ms=-1
					//try again to avoid block lost
					printf("try pull[%d] again(%d)\r\n", j, pull_retry[j]);
					pull_retry[j]++;
					if (pull_retry[j] < 10) {
						goto PULL_RETRY;
					} else {
						printf("Something wrong!!!");
					}
				}
				printf("pull_out error = %d!! blk=0x%X\r\n", ret, out_video_frame[j].blk);
				if (ret == HD_ERR_BAD_DATA) {
					printf("VPE-OUT[%d] blk=0x%X\r\n", j, out_video_frame[j].blk);
				}
				continue;
			}
			if (out_video_frame[j].blk != out_blk) {
				printf("[%d]block lost!(%d) out_video_frame[j].blk = 0x%X out_blk=0x%X\r\n", j, pull_retry[j], out_video_frame[j].blk, out_blk);
				//try again to avoid block lost
				pull_retry[j]++;
				if (pull_retry[j] < 10) {
					goto PULL_RETRY;
				} else {
					printf("Something wrong!!!");
				}
			}
			ret = hd_videoproc_release_out_buf(p_stream0->proc2_path[j], &out_video_frame[j]);
			if (ret != HD_OK) {
				printf("release_out[%d] error !!\r\n\r\n",j);
			}
		}
		#else //debug only, for single output
		j = 0;
		ret = hd_videoproc_pull_out_buf(p_stream0->proc2_path[j], &out_video_frame[0], -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
		if (ret != HD_OK) {
			printf("pull_out error = %d!! blk=0x%X\r\n", ret, out_video_frame[0].blk);
			if (ret == HD_ERR_BAD_DATA) {
				printf("VPE-OUT[%d] blk=0x%X\r\n", j, out_video_frame[0].blk);
			}
			continue;
		}
		ret = hd_videoproc_release_out_buf(p_stream0->proc2_path[j], &out_video_frame[0]);
		if (ret != HD_OK) {
			printf("release_out[%d] error !!\r\n\r\n",j);
		}
		j=1;
		ret = hd_videoproc_pull_out_buf(p_stream0->proc2_path[1], &out_video_frame[1], -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
		if (ret != HD_OK) {
			printf("pull_out error = %d!! blk=0x%X\r\n", ret, out_video_frame[1].blk);
			if (ret == HD_ERR_BAD_DATA) {
				printf("VPE-OUT[%d] blk=0x%X\r\n", j, out_video_frame[1].blk);
			}
			continue;
		}
		ret = hd_videoproc_release_out_buf(p_stream0->proc2_path[1], &out_video_frame[1]);
		if (ret != HD_OK) {
			printf("release_out[%d] error !!\r\n\r\n",j);
		}
		#endif

		//printf("proc_release ....\r\n");
		ret = hd_videoproc_release_out_buf(p_stream0->proc_path, &video_frame);
		if (ret != HD_OK) {
			printf("proc_release error=%d !!\r\n\r\n", ret);
		}

		//printf("out_push ....\r\n");
		//check if pull-out frame of VPE is ok
		if (out_video_frame[0].blk == out_blk) {
			ret = hd_videoout_push_in_buf(p_stream0->out_path, &out_video_frame[0], NULL, -1); // blocking mode
			if (ret != HD_OK) {
				printf("out_push error=%d !!\r\n\r\n", ret);
	    		goto skip2;
			}
		}

		//printf("proc_release ....\r\n");
		//release user output buffer
RELEASE_VPE_OUT:
		ret = hd_common_mem_release_block(out_blk);
		if (ret != HD_OK) {
			printf("_mem_release error !!\r\n\r\n");
			goto skip2;
		}


		p_stream0->prc_count ++;
		//printf("process count = %d\r\n", p_stream0->prc_count);
skip2:
		p_stream0->prc_loop++;
		usleep(100); //sleep for getchar()

		while (p_stream0->proc_sec_start == 0) {
			//printf("vprc thread pause\r\n");
			process_running= FALSE;
			usleep(100);
		}
	}

	return 0;
}


MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	VIDEO_LIVEVIEW stream[1] = {0}; //0: main stream
	UINT32 out_type = 1;
	HD_DIM main_dim;
	HD_IRECT pre_crop;
	HD_IRECT hole;
	HD_URECT rect;
	HD_DIM   bg;


    stream[0].hdmi_id=HD_VIDEOOUT_HDMI_1920X1080I60;//default

	if (argc > 1) {
		g_pattern_gen = atoi(argv[1]);
		printf("g_pattern_gen %d\r\n", g_pattern_gen);
	}


	// assign g_capfmt
	if (g_capfmt == 0) {
		g_capfmt = HD_VIDEO_PXLFMT_RAW12;
	} else {
		g_capfmt =  HD_VIDEO_PXLFMT_NRX12;
	}

	// assign g_prcfmt
	if (g_prcfmt == 0) {
		g_prcfmt = HD_VIDEO_PXLFMT_YUV420;
	} else {
		#if 0 //680
			if (enc_type == 0) {
				g_prcfmt = HD_VIDEO_PXLFMT_YUV420_NVX1_H265;
			} else if (enc_type == 1) {
				g_prcfmt = HD_VIDEO_PXLFMT_YUV420_NVX1_H264;
			} else {
				g_prcfmt = HD_VIDEO_PXLFMT_YUV420;
			}
		#else //520
			g_prcfmt = HD_VIDEO_PXLFMT_YUV420_NVX2;
		#endif
	}

	if (argc > 6) {
		g_shdr = atoi(argv[6]);
		printf("SHDR %d\r\n", g_shdr);
		if(g_shdr > 3) {
			printf("error: not support SHDR! (%d) \r\n", g_shdr);
			return 0;
		}
	}

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

	// init all modules
	ret = init_module();
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
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
	ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim, 0);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	// assign parameter by program options
	main_dim.w = VDO_SIZE_W;
	main_dim.h = VDO_SIZE_H;

	// set videoproc parameter
	ret = set_proc_param(stream[0].proc_path, &main_dim);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set VPE out0 parameter
	bg.w = ALIGN_FLOOR(stream[0].out_max_dim.w, 16);
	bg.h = ALIGN_FLOOR(stream[0].out_max_dim.h, 2);

	//full size
	rect.x = ALIGN_FLOOR(0, 4);
	rect.y = ALIGN_FLOOR(0, 2);
	rect.w = ALIGN_FLOOR(bg.w, 8);
	rect.h = ALIGN_FLOOR(bg.h, 2);

	//the coordinate is based on the origin of output rect, not background
	hole.x = ALIGN_FLOOR(PIP_STX - rect.x, 4);
	hole.y = ALIGN_FLOOR(PIP_STY - rect.y, 2);
	hole.w = ALIGN_FLOOR(PIP_WIDTH, 4);
	hole.h = ALIGN_FLOOR(PIP_HEIGHT, 2);
	ret = set_proc2_param(stream[0].proc2_path[0], &bg, &rect, NULL, &hole);
	if (ret != HD_OK) {
		printf("set proc2_path[0] fail=%d\n", ret);
		goto exit;
	}

	// set VPE out1 parameter
	rect.x = PIP_STX;
	rect.y = PIP_STY;
	rect.w = PIP_WIDTH;
	rect.h = PIP_HEIGHT;

	pre_crop.x = ALIGN_FLOOR(100, 2);
	pre_crop.y = ALIGN_FLOOR(200, 2);
	pre_crop.w = ALIGN_FLOOR((VDO_SIZE_W/4), 8);
	pre_crop.h = ALIGN_FLOOR((VDO_SIZE_H/4), 2);

	ret = set_proc2_param(stream[0].proc2_path[1], &bg, &rect, &pre_crop, NULL);
	if (ret != HD_OK) {
		printf("set proc2_path[1] fail=%d\n", ret);
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

	if (g_capbind == 0xff) { //no-bind mode
		// create capture_thread (pull_out frame, push_in frame then pull_out frame)
		ret = pthread_create(&stream[0].cap_thread_id, NULL, capture_thread, (void *)stream);
		if (ret < 0) {
			printf("create capture_thread failed");
			goto exit;
		}
	} else {
		// bind video_liveview modules (main)
		hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	}

	if (g_prcbind == 0xff) { //no-bind mode
		// create process_thread (pull_out frame, push_in frame then pull_out bitstream)
		ret = pthread_create(&stream[0].prc_thread_id, NULL, process_thread, (void *)stream);
		if (ret < 0) {
			printf("create process_thread failed");
			goto exit;
		}
	} else {
		// bind video_liveview modules (main)
		hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOOUT_0_IN_0);
	}

	// start video_liveview modules (main)
	if (g_capbind == 1) {
		//direct NOTE: ensure videocap start after 1st videoproc phy path start
		hd_videoproc_start(stream[0].proc_path);
		hd_videocap_start(stream[0].cap_path);
	} else {
		hd_videocap_start(stream[0].cap_path);
		hd_videoproc_start(stream[0].proc_path);
	}

	hd_videoproc_start(stream[0].proc2_path[0]);
	hd_videoproc_start(stream[0].proc2_path[1]);
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);
	hd_videoout_start(stream[0].out_path);

	if (g_capbind == 0xff) { //no-bind mode
		// start capture_thread
		stream[0].cap_enter = 1;
	}
	if (g_prcbind == 0xff) { //no-bind mode
		// start process_thread
		stream[0].prc_enter = 1;
	}

	// query user key
	printf("Enter q to exit\n");
	while (1) {
		key = GETCHAR();
		if (key == 'q' || key == 0x3) {
			// quit program
			break;
		}

		if (key == 'b') {
			stream[0].cap_sec_start= 0;
			stream[0].proc_sec_start= 0;

			wait_process_stopped();
			hd_videoproc_stop(stream[0].proc2_path[0]);
			hd_videoproc_stop(stream[0].proc2_path[1]);

			hd_videoproc_close(stream[0].proc2_path[0]);
			hd_videoproc_close(stream[0].proc2_path[1]);

			hd_videoproc_open(HD_VIDEOPROC_1_IN_0, HD_VIDEOPROC_1_OUT_0, &stream[0].proc2_path[0]);
			hd_videoproc_open(HD_VIDEOPROC_1_IN_0, HD_VIDEOPROC_1_OUT_1, &stream[0].proc2_path[1]);

			set_proc2_cfg(&stream[0].proc2_ctrl, &stream[0].proc_max_dim, HD_VIDEOPROC_1_CTRL);

			//full size
			rect.x = ALIGN_FLOOR(0, 4);
			rect.y = ALIGN_FLOOR(0, 2);
			rect.w = ALIGN_FLOOR(bg.w, 8);
			rect.h = ALIGN_FLOOR(bg.h, 2);

			//the coordinate is based on the origin of output rect, not background
			hole.x = ALIGN_FLOOR(PIP_STX - rect.x, 4);
			hole.y = ALIGN_FLOOR(PIP_STY - rect.y, 2);
			hole.w = ALIGN_FLOOR(PIP_WIDTH, 4);
			hole.h = ALIGN_FLOOR(PIP_HEIGHT, 2);
			ret = set_proc2_param(stream[0].proc2_path[0], &bg, &rect, NULL, &hole);
			if (ret != HD_OK) {
				printf("set proc2_path[0] fail=%d\n", ret);
				goto exit;
			}

			// set VPE out1 parameter
			rect.x = PIP_STX;
			rect.y = PIP_STY;
			rect.w = PIP_WIDTH;
			rect.h = PIP_HEIGHT;

			pre_crop.x = ALIGN_FLOOR(100, 2);
			pre_crop.y = ALIGN_FLOOR(200, 2);
			pre_crop.w = ALIGN_FLOOR((VDO_SIZE_W/4), 8);
			pre_crop.h = ALIGN_FLOOR((VDO_SIZE_H/4), 2);

			ret = set_proc2_param(stream[0].proc2_path[1], &bg, &rect, &pre_crop, NULL);
			if (ret != HD_OK) {
				printf("set proc2_path[1] fail=%d\n", ret);
				goto exit;
			}

			hd_videoproc_start(stream[0].proc2_path[0]);
			hd_videoproc_start(stream[0].proc2_path[1]);
			stream[0].cap_sec_start= 1;
			stream[0].proc_sec_start= 1;
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
	}

	if (g_capbind == 0xff) { //no-bind mode
		// stop capture_thread
		stream[0].cap_exit = 1;
	}
	if (g_prcbind == 0xff) { //no-bind mode
		// stop process_thread
		stream[0].prc_exit = 1;
	}
	// stop video_liveview modules (main)
	if (g_capbind == 1) {
		//direct NOTE: ensure videocap stop after all videoproc path stop
		hd_videoproc_stop(stream[0].proc_path);
		hd_videocap_stop(stream[0].cap_path);
	} else {
		hd_videocap_stop(stream[0].cap_path);
		hd_videoproc_stop(stream[0].proc_path);
	}
	hd_videoproc_stop(stream[0].proc2_path[0]);
	hd_videoproc_stop(stream[0].proc2_path[1]);
	hd_videoout_stop(stream[0].out_path);

	if (g_capbind == 0xff) { //no-bind mode
		// destroy capture_thread
		pthread_join(stream[0].cap_thread_id, NULL);  //NOTE: before destory, call stop to breaking pull(-1)
	} else {
		// unbind video_liveview modules (main)
		hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	}

	if (g_prcbind == 0xff) { //no-bind mode
		// destroy process_thread
		pthread_join(stream[0].prc_thread_id, NULL);  //NOTE: before destory, call stop to breaking pull(-1)
	} else {
		// unbind video_liveview modules (main)
		hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);
	}

exit:
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

	return 0;
}
