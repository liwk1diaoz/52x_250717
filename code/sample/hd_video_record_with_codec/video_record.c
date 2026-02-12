/**
	@brief Sample code of video record.\n

	@file video_record.c

	@author Boyan Huang

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
#include "rtspd_api.h"
#include "vendor_isp.h"

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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_video_record_with_rtsp, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define DEBUG_MENU 0
#define LOAD_ISP_CFG 1

#define CHKPNT          printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)         printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)         printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

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
#define SHDR_CAP_OUT_FMT HD_VIDEO_PXLFMT_NRX12_SHDR2
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

#define ENC0_SIZE_W VDO_SIZE_W
#define ENC0_SIZE_H VDO_SIZE_H
#define ENC1_SIZE_W VDO_SIZE_W
#define ENC1_SIZE_H VDO_SIZE_H

#define VIDEOCAP_ALG_FUNC HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB
#define VIDEOPROC_ALG_FUNC HD_VIDEOPROC_FUNC_WDR | HD_VIDEOPROC_FUNC_DEFOG | HD_VIDEOPROC_FUNC_COLORNR

static UINT32 g_shdr_mode = 0;
static UINT32 g_3DNR = 1;
static UINT32 g_DATA_MODE = 0; //option for vcap output directly to vprc
static UINT32 g_DATA2_MODE = 0; //option for vprc output lowlatency to venc
///////////////////////////////////////////////////////////////////////////////


static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, CAP_OUT_FMT)
        													+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
        													+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	if (g_shdr_mode == 1) {
		mem_cfg.pool_info[0].blk_cnt = 4;
	} else {
		mem_cfg.pool_info[0].blk_cnt = 2;
	}

	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[1].blk_cnt = 3;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	if (g_3DNR) {
		mem_cfg.pool_info[1].blk_cnt += 1;
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

/*
static HD_RESULT get_cap_sysinfo(HD_PATH_ID video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_SYSINFO sys_info = {0};

	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
	printf("sys_info.devid =0x%X, cur_fps[0]=%d/%d, vd_count=%llu\r\n", sys_info.dev_id, GET_HI_UINT16(sys_info.cur_fps[0]), GET_LO_UINT16(sys_info.cur_fps[0]), sys_info.vd_count);
	return ret;
}
*/

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


    if(RESOLUTION_SET == 3){
        cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_P_RAW;
	    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x204; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	    printf("Parallel interface\n");
    }
    if(RESOLUTION_SET == 0 || RESOLUTION_SET == 1 || RESOLUTION_SET == 2){
	    cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI
	    printf("MIPI interface\n");
    }
	if (g_shdr_mode == 1) {
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
	if (g_shdr_mode == 1) {
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

	if (g_shdr_mode == 1) {
		cap_cfg.sen_cfg.shdr_map = HD_VIDEOCAP_SHDR_MAP(HD_VIDEOCAP_HDR_SENSOR1, (HD_VIDEOCAP_0|HD_VIDEOCAP_1));
	}

	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = VIDEOCAP_ALG_FUNC;

	if (g_shdr_mode == 1) {
		iq_ctl.func |= HD_VIDEOCAP_FUNC_SHDR;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}

/*
static HD_RESULT set_cap_cfg2(HD_PATH_ID *p_video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};

	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =	 0x220; //PIN_SENSOR2_CFG_MIPI
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xFF44;//PIN_MIPI_LVDS_CFG_CLK6 | PIN_MIPI_LVDS_CFG_DAT4 | PIN_MIPI_LVDS_CFG_DAT5 | PIN_MIPI_LVDS_CFG_DAT6 | PIN_MIPI_LVDS_CFG_DAT7
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x100;//PIN_I2C_CFG_CH3
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel =	HD_VIDEOCAP_SEN_CLANE_SEL_CSI1_USE_C6;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = 1;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = 2;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = 3;
	ret = hd_videocap_open(0, HD_VIDEOCAP_1_CTRL, &video_cap_ctrl); //open this for device control

	if (ret != HD_OK) {
		return ret;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = VIDEOCAP_ALG_FUNC;
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}
*/

static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim, UINT32 path)
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
		if ((path == 0) && (g_shdr_mode == 1)) {
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
		if ((path == 0) && (g_shdr_mode == 1)) {
			video_out_param.pxlfmt = SHDR_CAP_OUT_FMT;
		} else {
			video_out_param.pxlfmt = CAP_OUT_FMT;
		}

		video_out_param.dir = HD_VIDEO_DIR_NONE;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
	}
	if (g_DATA_MODE) {//direct mode
		HD_VIDEOCAP_FUNC_CONFIG video_path_param = {0};

		video_path_param.out_func = HD_VIDEOCAP_OUTFUNC_DIRECT;
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
	HD_PATH_ID video_proc_ctrl = 0;

	ret = hd_videoproc_open(0, _out_id, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	if (p_max_dim != NULL ) {
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWALL;
		if ((HD_CTRL_ID)_out_id == HD_VIDEOPROC_0_CTRL) {
			video_cfg_param.isp_id = 0;
		} else {
			video_cfg_param.isp_id = 1;
		}

		video_cfg_param.ctrl_max.func = VIDEOPROC_ALG_FUNC;
		if (g_3DNR == 1) {
			printf("[3DNR] Set 1\n");
			video_cfg_param.ctrl_max.func |= HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_3DNR_STA;
		} else {
			video_cfg_param.ctrl_max.func &= ~(HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_3DNR_STA);
		}
		if (g_shdr_mode == 1) {
			video_cfg_param.ctrl_max.func |= HD_VIDEOPROC_FUNC_SHDR;
		} else {
			video_cfg_param.ctrl_max.func &= ~HD_VIDEOPROC_FUNC_SHDR;
		}
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;

		// NOTE: only SHDR with path 1
		if (g_shdr_mode == 1) {
			video_cfg_param.in_max.pxlfmt = SHDR_CAP_OUT_FMT;
		} else {
			video_cfg_param.in_max.pxlfmt = CAP_OUT_FMT;
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
		if (g_DATA_MODE)
			video_path_param.in_func |= HD_VIDEOPROC_INFUNC_DIRECT; //direct NOTE: enable direct
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
		//printf("set_proc_param PATH_CONFIG=0x%X\r\n", ret);
	}

	video_ctrl_param.func = VIDEOPROC_ALG_FUNC;
	if (g_3DNR == 1) {
		printf("[3DNR] Set 2\n");
		video_ctrl_param.func |= HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_3DNR_STA;
		video_ctrl_param.ref_path_3dnr = HD_VIDEOPROC_0_OUT_0;
	} else {
		video_ctrl_param.func &= ~(HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_3DNR_STA);
	}
	if (g_shdr_mode == 1) {
		video_ctrl_param.func |= HD_VIDEOPROC_FUNC_SHDR;
	} else {
		video_ctrl_param.func &= ~HD_VIDEOPROC_FUNC_SHDR;
	}

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
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}
	{
		HD_VIDEOPROC_FUNC_CONFIG video_path_param = {0};

		if (g_DATA2_MODE)
			video_path_param.out_func |= HD_VIDEOPROC_OUTFUNC_LOWLATENCY; //enable low-latency
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_enc_cfg(HD_PATH_ID video_enc_path, HD_DIM *p_max_dim, UINT32 max_bitrate, UINT32 isp_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_PATH_CONFIG video_path_config = {0};
	HD_VIDEOENC_FUNC_CONFIG video_func_config = {0};

	if (p_max_dim != NULL) {

		//--- HD_VIDEOENC_PARAM_PATH_CONFIG ---
		video_path_config.max_mem.codec_type = HD_CODEC_TYPE_H264;
		video_path_config.max_mem.max_dim.w  = p_max_dim->w;
		video_path_config.max_mem.max_dim.h  = p_max_dim->h;
		video_path_config.max_mem.bitrate    = max_bitrate;
		video_path_config.max_mem.enc_buf_ms = 3000;
		video_path_config.max_mem.svc_layer  = HD_SVC_4X;
		video_path_config.max_mem.ltr        = TRUE;
		video_path_config.max_mem.rotate     = FALSE;
		video_path_config.max_mem.source_output   = FALSE;
		video_path_config.isp_id             = isp_id;

		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_PATH_CONFIG, &video_path_config);
		if (ret != HD_OK) {
			printf("set_enc_path_config = %d\r\n", ret);
			return HD_ERR_NG;
		}

		if (g_DATA2_MODE)
			video_func_config.in_func |= HD_VIDEOENC_INFUNC_LOWLATENCY; //enable low-latency

		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_FUNC_CONFIG, &video_func_config);
		if (ret != HD_OK) {
			printf("set_enc_func_config = %d\r\n", ret);
			return HD_ERR_NG;
		}
	}

	return ret;
}

static int g_EncType = 0;   // 0: H265, 1: H264
static int g_GOP = 30;
static int g_SVC = 0;       // 0:disbale, 1: svc 2x, 2: svc 4x
static int g_LTRPeriod = 0;
static int g_RCMode = HD_RC_MODE_CBR;   // HD_RC_MODE_CBR/HD_RC_MODE_VBR/HD_RC_MODE_FIX_QP/HD_RC_MODE_EVBR
static int g_Bitrate = 2048;
static int g_MinQP = 15;
static int g_MaxQP = 45;
static int g_InitIQP = 26;
static int g_InitPQP = 28;
static int g_InitKPQP = 27;
static int g_IPWeight = 0;
static int g_ChangePos = 90;
static int g_KPPeriod = 0;
static int g_KPWeight = 0;
static int g_MotionStr = -4;
static int g_RowRCIRange = 2;
static int g_RowRCPRange = 4;
static int g_AQIStr = 3;
static int g_AQPStr = 3;
static int g_AQMaxQP = 8;
static int g_AQMinQP = -8;
static INT16 aq_table[HD_H26XENC_AQ_MAP_TABLE_NUM] = {-120,-112,-104, -96, -88, -80, -72, -64, -56, -48, -40, -32, -24, -16, -8, 7, 15, 23, 31, 39,47, 55, 63, 71, 79, 87, 95, 103, 111, 119};

static HD_RESULT set_enc_param(HD_PATH_ID video_enc_path, HD_DIM *p_dim, UINT32 enc_type, UINT32 bitrate)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_IN  video_in_param = {0};
	HD_VIDEOENC_OUT video_out_param = {0};
	HD_H26XENC_RATE_CONTROL rc_param = {0};
    HD_H26XENC_AQ aq_param = {0};
    HD_H26XENC_ROW_RC rowrc_param = {0};

	if (p_dim != NULL) {

		//--- HD_VIDEOENC_PARAM_IN ---
		video_in_param.dir           = HD_VIDEO_DIR_NONE;
		video_in_param.pxl_fmt = HD_VIDEO_PXLFMT_YUV420_NVX2;
		video_in_param.dim.w   = p_dim->w;
		video_in_param.dim.h   = p_dim->h;
		video_in_param.frc     = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_IN, &video_in_param);
		if (ret != HD_OK) {
			printf("set_enc_param_in = %d\r\n", ret);
			return ret;
		}

		//printf("enc_type=%d\r\n", enc_type);

        if (0 == g_EncType || 1 == g_EncType) {
            if (g_EncType == 0) {
                //--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
                video_out_param.codec_type         = HD_CODEC_TYPE_H265;
                video_out_param.h26x.profile       = HD_H265E_MAIN_PROFILE;
                video_out_param.h26x.level_idc     = HD_H265E_LEVEL_5;
                video_out_param.h26x.gop_num       = g_GOP;
                video_out_param.h26x.ltr_interval  = g_LTRPeriod;
                video_out_param.h26x.ltr_pre_ref   = 0;
                video_out_param.h26x.gray_en       = 0;
                video_out_param.h26x.source_output = 0;
                video_out_param.h26x.svc_layer     = (HD_VIDEOENC_SVC_LAYER)g_SVC;
                video_out_param.h26x.entropy_mode  = HD_H265E_CABAC_CODING;
                ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
                if (ret != HD_OK) {
                    printf("set_enc_param_out = %d\r\n", ret);
                    return ret;
                }
            }
            else if (g_EncType == 1) {
                //--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
                video_out_param.codec_type         = HD_CODEC_TYPE_H264;
                video_out_param.h26x.profile       = HD_H264E_HIGH_PROFILE;
                video_out_param.h26x.level_idc     = HD_H264E_LEVEL_5_1;
                video_out_param.h26x.gop_num       = g_GOP;
                video_out_param.h26x.ltr_interval  = g_LTRPeriod;
                video_out_param.h26x.ltr_pre_ref   = 0;
                video_out_param.h26x.gray_en       = 0;
                video_out_param.h26x.source_output = 0;
                video_out_param.h26x.svc_layer     = (HD_VIDEOENC_SVC_LAYER)g_SVC;
                video_out_param.h26x.entropy_mode  = HD_H264E_CABAC_CODING;
                ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
                if (ret != HD_OK) {
                    printf("set_enc_param_out = %d\r\n", ret);
                    return ret;
                }
            }

			//--- HD_VIDEOENC_PARAM_OUT_RATE_CONTROL ---
            if (HD_RC_MODE_CBR == g_RCMode) {
                rc_param.rc_mode                = (HD_VIDEOENC_RC_MODE)g_RCMode;
                rc_param.cbr.bitrate            = g_Bitrate*1024;
                rc_param.cbr.frame_rate_base    = 25;
                rc_param.cbr.frame_rate_incr    = 1;
                rc_param.cbr.init_i_qp          = g_InitIQP;
                rc_param.cbr.min_i_qp           = g_MinQP;
                rc_param.cbr.max_i_qp           = g_MaxQP;
                rc_param.cbr.init_p_qp          = g_InitPQP;
                rc_param.cbr.min_p_qp           = g_MinQP;
                rc_param.cbr.max_p_qp           = g_MaxQP;
                rc_param.cbr.static_time        = 0;
                rc_param.cbr.ip_weight          = g_IPWeight;
                ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
                if (ret != HD_OK) {
                    printf("set_enc_rate_control = %d\r\n", ret);
                    return ret;
                }
            }
            else if (HD_RC_MODE_VBR == g_RCMode) {
                rc_param.rc_mode                = (HD_VIDEOENC_RC_MODE)g_RCMode;
                rc_param.vbr.bitrate            = g_Bitrate*1024;
                rc_param.vbr.frame_rate_base    = 25;
                rc_param.vbr.frame_rate_incr    = 1;
                rc_param.vbr.init_i_qp          = g_InitIQP;
                rc_param.vbr.max_i_qp           = g_MaxQP;
                rc_param.vbr.min_i_qp           = g_MinQP;
                rc_param.vbr.init_p_qp          = g_InitPQP;
                rc_param.vbr.max_p_qp           = g_MaxQP;
                rc_param.vbr.min_p_qp           = g_MinQP;
                rc_param.vbr.static_time        = 0;
                rc_param.vbr.ip_weight          = g_IPWeight;
                rc_param.vbr.change_pos         = g_ChangePos;
                ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
                if (ret != HD_OK) {
                    printf("set_enc_rate_control = %d\r\n", ret);
                    return ret;
                }
            }
            else if (HD_RC_MODE_FIX_QP == g_RCMode) {
                rc_param.rc_mode                = (HD_VIDEOENC_RC_MODE)g_RCMode;
                rc_param.fixqp.frame_rate_base  = 25;
                rc_param.fixqp.frame_rate_incr  = 1;
                rc_param.fixqp.fix_i_qp         = g_InitIQP;
                rc_param.fixqp.fix_p_qp         = g_InitPQP;
                ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
                if (ret != HD_OK) {
                    printf("set_enc_rate_control = %d\r\n", ret);
                    return ret;
                }
            }
            else if (HD_RC_MODE_EVBR == g_RCMode) {
                rc_param.rc_mode                = (HD_VIDEOENC_RC_MODE)g_RCMode;
                rc_param.evbr.bitrate           = g_Bitrate*1024;
                rc_param.evbr.frame_rate_base   = 25;
                rc_param.evbr.frame_rate_incr   = 1;
                rc_param.evbr.init_i_qp         = g_InitIQP;
                rc_param.evbr.max_i_qp          = g_MaxQP;
                rc_param.evbr.min_i_qp          = g_MinQP;
                rc_param.evbr.init_p_qp         = g_InitPQP;
                rc_param.evbr.max_p_qp          = g_MaxQP;
                rc_param.evbr.min_p_qp          = g_MinQP;
                rc_param.evbr.static_time       = 0;
                rc_param.evbr.ip_weight         = g_IPWeight;
                rc_param.evbr.key_p_period      = g_KPPeriod;
                rc_param.evbr.kp_weight         = g_KPWeight;
                rc_param.evbr.still_frame_cnd   = 100;
                rc_param.evbr.motion_ratio_thd  = 30;
                rc_param.evbr.motion_aq_str     = g_MotionStr;
                rc_param.evbr.still_i_qp        = g_InitIQP;
                rc_param.evbr.still_p_qp        = g_InitPQP;
                rc_param.evbr.still_kp_qp       = g_InitKPQP;
                ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
                if (ret != HD_OK) {
                    printf("set_enc_rate_control = %d\r\n", ret);
                    return ret;
                }
            }

            //--- HD_VIDEOENC_PARAM_OUT_AQ ---
            if (0 == g_AQIStr && 0 == g_AQPStr)
                aq_param.enable = 0;
            else
                aq_param.enable = 1;
            aq_param.i_str = g_AQIStr;
            aq_param.p_str = g_AQPStr;
            aq_param.max_delta_qp = g_AQMaxQP;
            aq_param.min_delta_qp = g_AQMinQP;
            aq_param.depth = 2;
            memcpy(aq_param.thd_table, aq_table, sizeof(aq_param.thd_table));
            ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_AQ, &aq_param);
			if (ret != HD_OK) {
				printf("set_enc_aq = %d\r\n", ret);
				return ret;
			}

            //--- HD_VIDEOENC_PARAM_OUT_ROW_RC ---
            if (0 == g_RowRCIRange && 0 == g_RowRCPRange)
                rowrc_param.enable = 0;
            else
                rowrc_param.enable = 1;
            rowrc_param.i_qp_range = g_RowRCIRange;
            rowrc_param.i_qp_step = 1;
            rowrc_param.p_qp_range = g_RowRCPRange;
            rowrc_param.p_qp_step = 1;
            rowrc_param.min_i_qp = g_MinQP;
            rowrc_param.max_i_qp = g_MaxQP;
            rowrc_param.min_p_qp = g_MinQP;
            rowrc_param.max_p_qp = g_MaxQP;
            ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
            if (ret != HD_OK) {
				printf("set_enc_row_rc = %d\r\n", ret);
				return ret;
			}
		} else if (enc_type == 2) {

			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_JPEG;
			video_out_param.jpeg.retstart_interval = 0;
			video_out_param.jpeg.image_quality = 50;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}

		} else {

			printf("not support enc_type\r\n");
			return HD_ERR_NG;
		}
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _VIDEO_RECORD {

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

	HD_DIM  enc_max_dim;
	HD_DIM  enc_dim;

	// (3)
	HD_VIDEOENC_SYSCAPS enc_syscaps;
	HD_PATH_ID enc_path;

	// (4) user pull
	pthread_t  enc_thread_id;
	UINT32     enc_exit;
	UINT32     flow_start;

} VIDEO_RECORD;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
    if ((ret = hd_videoenc_init()) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module(VIDEO_RECORD *p_stream, HD_DIM* p_proc_max_dim)
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

	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_0, HD_VIDEOENC_0_OUT_0, &p_stream->enc_path)) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT close_module(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret;

	if ((ret = hd_videocap_close(p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path)) != HD_OK)
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
	if ((ret = hd_videoenc_uninit()) != HD_OK)
		return ret;

	return HD_OK;
}

//static void *encode_thread(void *arg)
static void *encode_thread(void *arg)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_BS  data_pull;
	//UINT32 j;

	UINT32 vir_addr_main;
	HD_VIDEOENC_BUFINFO phy_buf_main;
	//char file_path_main[32] = "/mnt/sd/dump_bs_main.dat";
	//FILE *f_out_main;
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))

	//------ wait flow_start ------
	while (p_stream0->flow_start == 0) sleep(1);

	// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
	hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);

	// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);

	//----- open output files -----
	// NOTE: mark
	#if 0
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		HD_VIDEOENC_ERR("open file (%s) fail....\r\n", file_path_main);
	} else {
		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}
	#endif

	printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	//--------- pull data test ---------
	while (p_stream0->enc_exit == 0) {
		//pull data
		ret = hd_videoenc_pull_out_buf(p_stream0->enc_path, &data_pull, -1); // -1 = blocking mode

		if (ret == HD_OK) {
			// NOTE: mark
			#if 0
			for (j=0; j< data_pull.pack_num; j++) {
				UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.video_pack[j].phy_addr);
				UINT32 len = data_pull.video_pack[j].size;
				if (f_out_main) fwrite(ptr, 1, len, f_out_main);
				if (f_out_main) fflush(f_out_main);
			}
			#endif

			rtspd_send_frame(p_stream0->enc_path,&data_pull,phy_buf_main,vir_addr_main);
			// release data
			ret = hd_videoenc_release_out_buf(p_stream0->enc_path, &data_pull);
			if (ret != HD_OK) {
				printf("enc_release error=%d !!\r\n", ret);
			}
		}
	}

	// mummap for bs buffer
	if (vir_addr_main) hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);

	// NOTE: mark
	#if 0
	// close output file
	if (f_out_main) fclose(f_out_main);
	#endif

	return 0;
}

//static void *encode_thread2(void *arg)
//{
//	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)arg;
//	VIDEO_RECORD* p_stream1 = p_stream0 + 1;
//	HD_RESULT ret = HD_OK;
//	HD_VIDEOENC_BS  data_pull;
//	//UINT32 j;
//	HD_VIDEOENC_POLL_LIST poll_list[2];
//
//	UINT32 vir_addr_main;
//	HD_VIDEOENC_BUFINFO phy_buf_main;
//	//char file_path_main[32] = "/mnt/sd/dump_bs_sen_1.dat";
//	//FILE *f_out_main;
//	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))
//	UINT32 vir_addr_sub;
//	HD_VIDEOENC_BUFINFO phy_buf_sub;
//	//char file_path_sub[32]  = "/mnt/sd/dump_bs_sen_2.dat";
//	//FILE *f_out_sub;
//	#define PHY2VIRT_SUB(pa) (vir_addr_sub + (pa - phy_buf_sub.buf_info.phy_addr))
//
//	//------ wait flow_start ------
//	while (p_stream0->flow_start == 0) sleep(1);
//
//	// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
//	hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);
//	hd_videoenc_get(p_stream1->enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_sub);
//
//	// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
//	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);
//	vir_addr_sub  = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_sub.buf_info.phy_addr, phy_buf_sub.buf_info.buf_size);
//
//	//----- open output files -----
//	// NOTE: mark
//	#if 0
//	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
//		HD_VIDEOENC_ERR("open file (%s) fail....\r\n", file_path_main);
//	} else {
//		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
//	}
//	if ((f_out_sub = fopen(file_path_sub, "wb")) == NULL) {
//		HD_VIDEOENC_ERR("open file (%s) fail....\r\n", file_path_sub);
//	} else {
//		printf("\r\ndump sub  bitstream to file (%s) ....\r\n", file_path_sub);
//	}
//	#endif
//
//	printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");
//	//--------- pull data test ---------
//	poll_list[0].path_id = p_stream0->enc_path;
//	poll_list[1].path_id = p_stream1->enc_path;
//
//	while (p_stream0->enc_exit == 0) {
//		if (HD_OK == hd_videoenc_poll_list(poll_list, 2, -1)) {    // multi path poll_list , -1 = blocking mode
//			if (TRUE == poll_list[0].revent.event) {
//				//pull data
//				ret = hd_videoenc_pull_out_buf(p_stream0->enc_path, &data_pull, 0); // 0 = non-blocking mode
//
//				if (ret == HD_OK) {
//					// NOTE: mark
//					#if 0
//					for (j=0; j< data_pull.pack_num; j++) {
//						UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.video_pack[j].phy_addr);
//						UINT32 len = data_pull.video_pack[j].size;
//						if (f_out_main) fwrite(ptr, 1, len, f_out_main);
//						if (f_out_main) fflush(f_out_main);
//					}
//					#endif
//
//					if (rtsp_path == 0) {
//						rtspd_send_frame(p_stream0->enc_path,&data_pull,phy_buf_main,vir_addr_main);
//					}
//
//					// release data
//					ret = hd_videoenc_release_out_buf(p_stream0->enc_path, &data_pull);
//				}
//			}
//
//			if (TRUE == poll_list[1].revent.event) {
//				//pull data
//				ret = hd_videoenc_pull_out_buf(p_stream1->enc_path, &data_pull, 0); // 0 = non-blocking mode
//
//				if (ret == HD_OK) {
//					// NOTE: mark
//					#if 0
//					for (j=0; j< data_pull.pack_num; j++) {
//						UINT8 *ptr = (UINT8 *)PHY2VIRT_SUB(data_pull.video_pack[j].phy_addr);
//						UINT32 len = data_pull.video_pack[j].size;
//						if (f_out_sub) fwrite(ptr, 1, len, f_out_sub);
//						if (f_out_sub) fflush(f_out_sub);
//					}
//					#endif
//
//					if (rtsp_path == 1) {
//						rtspd_send_frame(p_stream1->enc_path,&data_pull,phy_buf_sub,vir_addr_sub);
//					}
//
//					// release data
//					ret = hd_videoenc_release_out_buf(p_stream1->enc_path, &data_pull);
//				}
//			}
//		}
//	}
//
//	// mummap for bs buffer
//	hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
//	hd_common_mem_munmap((void *)vir_addr_sub, phy_buf_sub.buf_info.buf_size);
//
//	// close output file
//	// NOTE: mark
//	#if 0
//	if (f_out_main) fclose(f_out_main);
//	if (f_out_sub) fclose(f_out_sub);
//	#endif
//
//	return 0;
//}
static const char ENC_NAME[3][10] = {"H265", "H264", "JPEG"};
static const char RC_NAME[5][10] = {"NULL", "CBR", "VBR", "FixQP", "EVBR"};

typedef struct {
    char id;
    int *value;
    int default_value;
    int min_value;
    int max_value;
    char *note;
} EncSyntaxMap;

static const EncSyntaxMap enc_param[] =
{
    {'e',   &g_EncType,     0,      0,      1,      "encode type (0: H265, 1: H264)"},
    {'g',   &g_GOP,         30,     1,      4096,   "gop"},
    {'s',   &g_SVC,         0,      0,      2,      "svc (0:disbale, 1: svc 2x, 2: svc 4x)"},
    {'l',   &g_LTRPeriod,   0,      0,      4096,   "ltr period"},
    {'t',   &g_RCMode,      1,      1,      4,      "rc mode (1: CBR, 2: VBR, 3: FixQP, 4: EVBR)"},
    {'b',   &g_Bitrate,     2048,   1,      65536,  "bitrate(kbps)"},
    {'q',   &g_MinQP,       15,     0,      51,     "min qp"},
    {'Q',   &g_MaxQP,       45,     0,      51,     "max qp"},
    {'i',   &g_InitIQP,     26,     0,      51,     "init i qp"},
    {'p',   &g_InitPQP,     28,     0,      51,     "init p qp"},
    {'k',   &g_InitKPQP,    27,     0,      51,     "init kp qp"},
    {'W',   &g_IPWeight,    0,      -100,   100,    "ip weight"},
    {'c',   &g_ChangePos,   90,     0,      100,    "change pos"},
    {'K',   &g_KPPeriod,    0,      0,      4096,   "kp period"},
    {'w',   &g_KPWeight,    0,      -100,   100,    "kp weight"},
    {'m',   &g_MotionStr,   -4,     -15,    15,     "motion aq strength"},
    {'R',   &g_RowRCIRange, 2,      0,      15,     "I frame row rc range"},
    {'r',   &g_RowRCPRange, 4,      0,      15,     "P frame row rc range"},
    {'A',   &g_AQIStr,      3,      0,      7,      "I frame AQ strength"},
    {'a',   &g_AQPStr,      3,      0,      7,      "P frame AQ strength"},
    {'D',   &g_AQMaxQP,     8,      0,      15,     "AQ max delta QP"},
    {'d',   &g_AQMinQP,     -8,     -15,    0,      "AQ min delta QP"},
};

void print_usage(void)
{
    int i;
    for (i = 0; i < (int)(sizeof(enc_param)/sizeof(EncSyntaxMap)); i++) {
        printf("%c: %s (range %d ~ %d)\n", enc_param[i].id, enc_param[i].note, enc_param[i].min_value, enc_param[i].max_value);
    }
}

int parse_cfg(int argc, char **argv)
{
    int ret = 0;
    int i, idx;
    int key, value;
    if (1 == argc) {
        print_usage();
        ret = -1;
        goto exit_parse;
    }

    for (i = 0; i < (int)(sizeof(enc_param)/sizeof(EncSyntaxMap)); i++) {
        *enc_param[i].value = enc_param[i].default_value;
    }

    while ((key = getopt(argc, argv, "e:g:s:l:t:b:q:Q:i:p:k:W:c:K:w:m:R:r:A:a:D:d:h")) != EOF) {
        // find key
        idx = -1;
        for (i = 0; i < (int)(sizeof(enc_param)/sizeof(EncSyntaxMap)); i++) {
            if (key == enc_param[i].id) {
                idx = i;
                break;
            }
        }
        if (idx >= 0) {
            value = atoi(optarg);
            if (value < enc_param[idx].min_value || value > enc_param[idx].max_value) {
                printf("syntax %c (value %d) is out of range (%d ~ %d)\n", enc_param[idx].id, value, enc_param[idx].min_value, enc_param[idx].max_value);
                ret = -1;
                goto exit_parse;
            }
            *enc_param[idx].value = value;
        }
        else {
            if ('h' == key)
                print_usage();
            //else
            //    printf("unknown \'%c\'\n", (char)key);
            ret = -1;
            goto exit_parse;
        }
    }

    printf("%s: gop %d, svc %d, ltr %d, row rc range %d/%d, aq str %d/%d delta qp %d/%d\n",
        ENC_NAME[g_EncType], g_GOP, g_SVC, g_LTRPeriod, g_RowRCIRange, g_RowRCPRange, g_AQIStr, g_AQPStr, g_AQMaxQP, g_AQMinQP);
    printf("%s: bitrate %d kbps, qp %d ~ %d, init qp %d/%d/%d, weight %d/%d, kp period %d, motion str %d\n",
        RC_NAME[g_RCMode], g_Bitrate, g_MinQP, g_MaxQP, g_InitIQP, g_InitKPQP, g_InitPQP, g_IPWeight, g_KPWeight, g_KPPeriod, g_MotionStr);

exit_parse:
    return ret;
}

MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	VIDEO_RECORD stream[2] = {0}; //0: main stream
	UINT32 enc_type = 0;
	UINT32 enc_bitrate = 4;
	HD_DIM main_dim;
	#if (LOAD_ISP_CFG == 1)
	AET_CFG_INFO cfg_info = {0};
	#endif

#if 1
    if (parse_cfg(argc, argv) < 0)
        return 0;
#else
	if (argc == 1 || argc > 7) {
		printf("Usage: <3DNR> <shdr_mode> <enc_type> <enc_bitrate> <data_mode> <data2_mode>.\r\n");
		printf("Help:\r\n");
		printf("  <3DNR>        : 0(disable), 1(enable)\r\n");
		printf("  <shdr_mode>   : 0(disable), 1(enable)\r\n");
		printf("  <enc_type>    : 0(H265), 1(H264)\r\n");
		printf("  <enc_bitrate> : Mbps\r\n");
		printf("  <data_mode>   : 0(D2D), 1(Direct)\r\n");
		printf("  <data2_mode>   : 0(D2D), 1(LowLatency)\r\n");
		return 0;
	}

	// query program options
	if (argc > 1) {
		g_3DNR = atoi(argv[1]);
		printf("g_3DNR %d\r\n", g_3DNR);
		if(g_3DNR > 2) {
			printf("error: not support g_3DNR! (%d) \r\n", g_3DNR);
			return 0;
		}
	}
	if (argc > 2) {
		g_shdr_mode = atoi(argv[2]);
		printf("g_shdr_mode %d\r\n", g_shdr_mode);
		if(g_shdr_mode > 2) {
			printf("error: not support g_shdr_mode! (%d) \r\n", g_shdr_mode);
			return 0;
		}
	}
	if (argc > 3) {
		enc_type = atoi(argv[3]);
		if (enc_type == 0) {
			printf("enc_type: H.265\r\n", enc_type);
		} else if (enc_type == 1) {
			printf("enc_type: H.264\r\n", enc_type);
		} else {
			printf("error: not support enc_type!\r\n");
			return 0;
		}
	}
	if (argc > 4) {
		enc_bitrate = atoi(argv[4]);
		printf("enc_bitrate: %d Mbps\r\n", enc_bitrate);
	}
	if (argc > 5) {
		g_DATA_MODE = atoi(argv[5]);
		if(g_DATA_MODE > 1) {
			printf("error: not support g_DATA_MODE! (%d) \r\n", g_DATA_MODE);
			return 0;
		}
	}
	if (argc > 6) {
		g_DATA2_MODE = atoi(argv[6]);
		if(g_DATA2_MODE > 1) {
			printf("error: not support g_DATA2_MODE! (%d) \r\n", g_DATA2_MODE);
			return 0;
		}
	}
#endif

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

	// open video_record modules (main)
	stream[0].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[0].proc_max_dim.h = VDO_SIZE_H; //assign by user
	ret = open_module(&stream[0], &stream[0].proc_max_dim);
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

	// set videoproc parameter (main)
	ret = set_proc_param(stream[0].proc_path, NULL);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoenc config (main)
	stream[0].enc_max_dim.w = main_dim.w;
	stream[0].enc_max_dim.h = main_dim.h;
	ret = set_enc_cfg(stream[0].enc_path, &stream[0].enc_max_dim, enc_bitrate * 1024 * 1024, 0);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}

	// set videoenc parameter (main)
	stream[0].enc_dim.w = main_dim.w;
	stream[0].enc_dim.h = main_dim.h;
	ret = set_enc_param(stream[0].enc_path, &stream[0].enc_dim, enc_type, enc_bitrate * 1024 * 1024);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}

	// bind video_record modules (main)
	hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOENC_0_IN_0);

	// create encode_thread (pull_out bitstream)
	ret = pthread_create(&stream[0].enc_thread_id, NULL, encode_thread, (void *)stream);
	if (ret < 0) {
		printf("create encode thread failed");
		goto exit;
	}

	// start video_record modules (main)
	if (g_DATA_MODE) {
		//direct NOTE: ensure videocap start after 1st videoproc phy path start
		hd_videoproc_start(stream[0].proc_path);
		hd_videocap_start(stream[0].cap_path);
	} else {
		hd_videocap_start(stream[0].cap_path);
		hd_videoproc_start(stream[0].proc_path);
	}

	#if (LOAD_ISP_CFG == 1)
	if (vendor_isp_init() == HD_ERR_NG) {
		return -1;
	}
	cfg_info.id = 0;
	strncpy(cfg_info.path, "/mnt/app/isp/isp_os02k10_0.cfg", CFG_NAME_LENGTH);
	vendor_isp_set_ae(AET_ITEM_RLD_CONFIG, &cfg_info);
	vendor_isp_set_awb(AWBT_ITEM_RLD_CONFIG, &cfg_info);
	vendor_isp_set_iq(IQT_ITEM_RLD_CONFIG, &cfg_info);
	vendor_isp_uninit();
	#endif

	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);
	hd_videoenc_start(stream[0].enc_path);

	// let encode_thread start to work
	stream[0].flow_start = 1;
	rtspd_start(554);

	// query user key
	printf("Enter q to exit \n");
//	if (g_shdr_mode == 1) {
//		printf("Enter s to switch dr mode \n");
//	}
	while (1) {
		key = GETCHAR();
		if (key == 'q' || key == 0x3) {
			// let encode_thread stop loop and exit
			stream[0].enc_exit = 1;
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
	}
	rtspd_stop();

	// stop video_record modules (main)
	if (g_DATA_MODE) {
		//direct NOTE: ensure videocap stop after all videoproc path stop
		hd_videoproc_stop(stream[0].proc_path);
		hd_videocap_stop(stream[0].cap_path);
	} else {
		hd_videocap_stop(stream[0].cap_path);
		hd_videoproc_stop(stream[0].proc_path);
	}
	hd_videoenc_stop(stream[0].enc_path);

	// destroy encode thread
	//pthread_join(stream[0].enc_thread_id, NULL);
	pthread_cancel(stream[0].enc_thread_id);

	// unbind video_record modules (main)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);

exit:
	// close video_record modules (main)
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
