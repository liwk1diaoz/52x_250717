/**
	@brief Sample code of video record.\n

	@file video_record.c

	@author 

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
#include "rtspd.h"
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
#define LOAD_ISP_CFG 0

#define CHKPNT          printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)         printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)         printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////
#define DBGINFO_BUFSIZE()	(0x200)

//RAW
#define VDO_RAW_BUFSIZE(w, h, pxlfmt)   (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//RAW compress only support 12bit mode
#define VDO_NRX_BUFSIZE(w, h)           ALIGN_CEIL_4(ALIGN_CEIL_4(ALIGN_CEIL_4((w) * 12 / 8) * RAW_COMPRESS_RATIO / 100) * (h) + ALIGN_CEIL_4((ALIGN_CEIL_32(w) / 32) * 16 / 8*(h)))
//CA for AWB
#define VDO_CA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 3) << 1)
//LA for AE
#define VDO_LA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 1) << 1)
//YOUT for SHDR/WDR
#define VDO_YOUT_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4(ALIGN_CEIL_4(win_num_w * 12 / 8) * win_num_h)
//ETH
#define VDO_ETH_BUF_SIZE(roi_w, roi_h, b_out_sel, b_8bit_sel) ALIGN_CEIL_4((((roi_w >> (b_out_sel ? 1 : 0)) >> (b_8bit_sel ? 0 : 2)) * (roi_h - 4)) >> (b_out_sel ? 1 : 0))
//VA for AF
#define VDO_VA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 1) << 2)

#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	ALIGN_CEIL_4(((w) * (h) * HD_VIDEO_PXLFMT_BPP(pxlfmt)) / 8)
#define VDO_NVX_BUFSIZE(w, h, pxlfmt)	(VDO_YUV_BUFSIZE(w, h, pxlfmt) * RAW_COMPRESS_RATIO / 100)

#define SEN_OUT_FMT     HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT     HD_VIDEO_PXLFMT_RAW12
#define SHDR_CAP_OUT_FMT HD_VIDEO_PXLFMT_NRX12_SHDR2
#define PRC_OUT_FMT     HD_VIDEO_PXLFMT_YUV420_NVX2
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

#define SRC_PATH		HD_VIDEOPROC_0_OUT_0 //out 0~4 is physical path ..... enable it before enable ir path
#define SRC_PXLFMT		HD_VIDEO_PXLFMT_YUV420
#define RESOLUTION_SET  3 //0: 2M(IMX290), 1:5M(OS05A) 2: 2M (OS02K10) 3: 2M (AR0237IR)
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

#define IR_PATH			HD_VIDEOPROC_0_OUT_5 //out 5 is special path of ir
#define IR_PXLFMT 		0xe5200020 //ir 8-bits pxlfmt
#define IR_ENCFMT 		HD_VIDEO_PXLFMT_Y8
#define SUB_VDO_SIZE_W	(1920/2) //ir.w = RAW.w / 2
#define SUB_VDO_SIZE_H	(1080/2) //ir.h = RAW.h / 2


#define VIDEOCAP_ALG_FUNC HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB
#define VIDEOPROC_ALG_FUNC_0 HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_COLORNR | HD_VIDEOPROC_FUNC_WDR

#define VIDEOPROC_ALG_FUNC_1 HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_COLORNR

#define IR_ISP_FUNC	1 //for ISP tuning, send IR frame to 2nd vprc

#define ENC0_SIZE_W VDO_SIZE_W
#define ENC0_SIZE_H VDO_SIZE_H
#define ENC1_SIZE_W VDO_SIZE_W
#define ENC1_SIZE_H VDO_SIZE_H


static UINT32 g_shdr_mode = 0;
static UINT32 g_3DNR = 0;
static UINT32 g_DATA_MODE = 0; //option for vcap output directly to vprc
static UINT32 g_DATA2_MODE = 0; //option for vprc output lowlatency to venc
static UINT32 g_cap_out_fmt = 0;
static UINT32 g_prc_out_fmt = 0;
static UINT32 g_fps = 30;
///////////////////////////////////////////////////////////////////////////////
static UINT32 rtsp_path = 0;
pthread_t  ir_thread_id;

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
		mem_cfg.pool_info[0].blk_cnt = 5;
	} else {
		mem_cfg.pool_info[0].blk_cnt = 3;
	}

	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, SRC_PXLFMT);
	mem_cfg.pool_info[1].blk_cnt = 3;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	if (g_3DNR) {
		mem_cfg.pool_info[1].blk_cnt += 1;
	}

	// config common pool (sub)(sub2)
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[2].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(SUB_VDO_SIZE_W, SUB_VDO_SIZE_H, IR_ENCFMT);
	mem_cfg.pool_info[2].blk_cnt = 3;
#if 	(IR_ISP_FUNC == 1)
	mem_cfg.pool_info[2].blk_cnt += 2;
#endif
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;

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

static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim, UINT32 path)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};

		video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
		video_in_param.frc = HD_VIDEO_FRC_RATIO(g_fps,1);
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
		if (path == 0) {
			if ((g_cap_out_fmt == 0) && (g_shdr_mode == 0)) video_out_param.pxlfmt = HD_VIDEO_PXLFMT_RAW12;
			if ((g_cap_out_fmt == 0) && (g_shdr_mode == 1)) video_out_param.pxlfmt = HD_VIDEO_PXLFMT_RAW12_SHDR2;
			if ((g_cap_out_fmt == 1) && (g_shdr_mode == 0)) video_out_param.pxlfmt = HD_VIDEO_PXLFMT_NRX12;
			if ((g_cap_out_fmt == 1) && (g_shdr_mode == 1)) video_out_param.pxlfmt = HD_VIDEO_PXLFMT_NRX12_SHDR2;
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
		if ((HD_CTRL_ID)_out_id == HD_VIDEOPROC_0_CTRL) {
			video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWALL;
            video_cfg_param.ctrl_max.func = VIDEOPROC_ALG_FUNC_0;
		} else {
			video_cfg_param.pipe = HD_VIDEOPROC_PIPE_YUVALL;
            video_cfg_param.ctrl_max.func = VIDEOPROC_ALG_FUNC_1;
		}
		if ((HD_CTRL_ID)_out_id == HD_VIDEOPROC_0_CTRL) {
			video_cfg_param.isp_id = 0x00000000;
		} else {
			video_cfg_param.isp_id = 0x00020000;
		}

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
		if ((HD_CTRL_ID)_out_id == HD_VIDEOPROC_0_CTRL) {
			// NOTE: only SHDR with path 1
			{
				if ((g_cap_out_fmt == 0) && (g_shdr_mode == 0)) video_cfg_param.in_max.pxlfmt = HD_VIDEO_PXLFMT_RAW12;
				if ((g_cap_out_fmt == 0) && (g_shdr_mode == 1)) video_cfg_param.in_max.pxlfmt = HD_VIDEO_PXLFMT_RAW12_SHDR2;
				if ((g_cap_out_fmt == 1) && (g_shdr_mode == 0)) video_cfg_param.in_max.pxlfmt = HD_VIDEO_PXLFMT_NRX12;
				if ((g_cap_out_fmt == 1) && (g_shdr_mode == 1)) video_cfg_param.in_max.pxlfmt = HD_VIDEO_PXLFMT_NRX12_SHDR2;
			}
		}else {
			video_cfg_param.in_max.pxlfmt = IR_PXLFMT;
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
    if ((HD_CTRL_ID)_out_id == HD_VIDEOPROC_0_CTRL) {
	   video_ctrl_param.func = VIDEOPROC_ALG_FUNC_0;
    }
    else
    {
       video_ctrl_param.func = VIDEOPROC_ALG_FUNC_1;
    }

	if (g_3DNR == 1) {
		printf("[3DNR] Set 2\n");
		video_ctrl_param.func |= HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_3DNR_STA;
		//use current path as reference path (if venc.dim == raw.dim)
		if ((HD_CTRL_ID)_out_id == HD_VIDEOPROC_0_CTRL) {
			video_ctrl_param.ref_path_3dnr = HD_VIDEOPROC_0_OUT_0;
		} else {
			video_ctrl_param.ref_path_3dnr = HD_VIDEOPROC_1_OUT_0;
		}
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

static HD_RESULT set_proc_param(HD_PATH_ID video_proc_path, HD_DIM* p_dim, HD_VIDEO_PXLFMT pxlfmt, BOOL is_pull)
{
	HD_RESULT ret = HD_OK;

	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = pxlfmt;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);   
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}
	else {
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = 0;
		video_out_param.dim.h = 0;
		video_out_param.pxlfmt = pxlfmt; 
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = is_pull; //set 1 to allow pull
		printf("video_proc_path=0x%08x,video_out_param.depth = %d\n",video_proc_path,video_out_param.depth);
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

static HD_RESULT set_proc_param_extend(HD_PATH_ID video_proc_path, HD_DIM* p_dim, HD_VIDEO_PXLFMT pxlfmt, BOOL is_pull, HD_PATH_ID src_path)
{
	HD_RESULT ret = HD_OK;

	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT_EX video_out_param = {0};
		video_out_param.src_path = src_path;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = pxlfmt;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = is_pull; //set 1 to allow pull
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_EX, &video_out_param);
	} else {
		HD_VIDEOPROC_OUT_EX video_out_param = {0};
		video_out_param.src_path = src_path;
		video_out_param.dim.w = 0;
		video_out_param.dim.h = 0;
		video_out_param.pxlfmt = pxlfmt;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = is_pull; //set 1 to allow pull
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_EX, &video_out_param);
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

static HD_RESULT set_enc_param(HD_PATH_ID video_enc_path, HD_DIM *p_dim, HD_VIDEO_PXLFMT pxlfmt, UINT32 enc_type, UINT32 bitrate)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_IN  video_in_param = {0};
	HD_VIDEOENC_OUT video_out_param = {0};
	HD_H26XENC_RATE_CONTROL rc_param = {0};

	if (p_dim != NULL) {

		//--- HD_VIDEOENC_PARAM_IN ---
		video_in_param.dir           = HD_VIDEO_DIR_NONE;
		video_in_param.pxl_fmt = pxlfmt;
		video_in_param.dim.w   = p_dim->w;
		video_in_param.dim.h   = p_dim->h;
		video_in_param.frc     = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_IN, &video_in_param);
		if (ret != HD_OK) {
			printf("set_enc_param_in = %d\r\n", ret);
			return ret;
		}

		printf("enc_type=%d\r\n", enc_type);

		if (enc_type == 0) {

			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_H265;
			video_out_param.h26x.profile       = HD_H265E_MAIN_PROFILE;
			video_out_param.h26x.level_idc     = HD_H265E_LEVEL_5;
			video_out_param.h26x.gop_num       = 15;
			video_out_param.h26x.ltr_interval  = 0;
			video_out_param.h26x.ltr_pre_ref   = 0;
			video_out_param.h26x.gray_en       = (pxlfmt == HD_VIDEO_PXLFMT_Y8) ? 1: 0;
			video_out_param.h26x.source_output = 0;
			video_out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			video_out_param.h26x.entropy_mode  = HD_H265E_CABAC_CODING;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}

			//--- HD_VIDEOENC_PARAM_OUT_RATE_CONTROL ---
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = bitrate;
			rc_param.cbr.frame_rate_base = 30;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control = %d\r\n", ret);
				return ret;
			}
		} else if (enc_type == 1) {

			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_H264;
			video_out_param.h26x.profile       = HD_H264E_HIGH_PROFILE;
			video_out_param.h26x.level_idc     = HD_H264E_LEVEL_5_1;
			video_out_param.h26x.gop_num       = 15;
			video_out_param.h26x.ltr_interval  = 0;
			video_out_param.h26x.ltr_pre_ref   = 0;
			video_out_param.h26x.gray_en       = (pxlfmt == HD_VIDEO_PXLFMT_Y8) ? 1: 0;
			video_out_param.h26x.source_output = 0;
			video_out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			video_out_param.h26x.entropy_mode  = HD_H264E_CABAC_CODING;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}

			//--- HD_VIDEOENC_PARAM_OUT_RATE_CONTROL ---
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = bitrate;
			rc_param.cbr.frame_rate_base = 30;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control = %d\r\n", ret);
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
	HD_PATH_ID proc_path2;
    
	HD_DIM  enc_max_dim;
	HD_DIM  enc_dim;

	// (3)
	HD_VIDEOENC_SYSCAPS enc_syscaps;
	HD_PATH_ID enc_path;

	// (4) user pull
	pthread_t  enc_thread_id;
    pthread_t  aquire_thread_id;
	UINT32     enc_exit;
	UINT32     flow_start;
    UINT32 	do_snap;
    UINT32 	shot_count;

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

static HD_RESULT open_module_2(VIDEO_RECORD *p_stream, HD_DIM* p_proc_max_dim)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, IR_PATH,  &p_stream->proc_path)) != HD_OK)
		return ret;
#if 	(IR_ISP_FUNC == 1)
	// set videoproc config
	ret = set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim, HD_VIDEOPROC_1_CTRL);
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}

	if ((ret = hd_videoproc_open(HD_VIDEOPROC_1_IN_0, HD_VIDEOPROC_1_OUT_0,  &p_stream->proc_path2)) != HD_OK)
		return ret;
#endif
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_1, HD_VIDEOENC_0_OUT_1,  &p_stream->enc_path)) != HD_OK)
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

static HD_RESULT close_module_2(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
#if 	(IR_ISP_FUNC == 1)
	if ((ret = hd_videoproc_close(p_stream->proc_path2)) != HD_OK)
		return ret;
#endif
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

static void *encode_thread(void *arg)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)arg;
	VIDEO_RECORD* p_stream1 = p_stream0 + 1;
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_BS  data_pull;
	//UINT32 j;
	HD_VIDEOENC_POLL_LIST poll_list[2];

	UINT32 vir_addr_main;
	HD_VIDEOENC_BUFINFO phy_buf_main;
	//char file_path_main[32] = "/mnt/sd/dump_bs_sen_1.dat";
	//FILE *f_out_main;
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))
	UINT32 vir_addr_sub;
	HD_VIDEOENC_BUFINFO phy_buf_sub;
	//char file_path_sub[32]  = "/mnt/sd/dump_bs_sen_2.dat";
	//FILE *f_out_sub;
	#define PHY2VIRT_SUB(pa) (vir_addr_sub + (pa - phy_buf_sub.buf_info.phy_addr))
	UINT8 *data_buf0 = 0;
	UINT8 *data_buf1 = 0;
	
	unsigned long frame_timestamp_ms = 0;
	unsigned long frame_data_len = 0;
	
	//Allocate data buffer for video frame
   data_buf0 =(unsigned char*)malloc(1920 * 1088 * 2);
   if(data_buf0 == NULL) {
	    printf("Ch0_0, malloc memory for video buffer fail !!! (size[1920 x 1080 x 2 ])");
       goto thread_end;
   }
	
	data_buf1 =(unsigned char*)malloc(1920 * 1088 * 2);
   if(data_buf1 == NULL) {
	    printf("Ch0_1, malloc memory for video buffer fail !!! (size[1920 x 1080 x 2 ])");
       goto thread_end;
   }
	
	//------ wait flow_start ------
	while (p_stream0->flow_start == 0) sleep(1);

	// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
	hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);
	hd_videoenc_get(p_stream1->enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_sub);

	// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);
	vir_addr_sub  = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_sub.buf_info.phy_addr, phy_buf_sub.buf_info.buf_size);

	//----- open output files -----
	// NOTE: mark
	#if 0
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		HD_VIDEOENC_ERR("open file (%s) fail....\r\n", file_path_main);
	} else {
		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}
	if ((f_out_sub = fopen(file_path_sub, "wb")) == NULL) {
		HD_VIDEOENC_ERR("open file (%s) fail....\r\n", file_path_sub);
	} else {
		printf("\r\ndump sub  bitstream to file (%s) ....\r\n", file_path_sub);
	}
	#endif

	printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	//--------- pull data test ---------
	poll_list[0].path_id = p_stream0->enc_path;
	poll_list[1].path_id = p_stream1->enc_path;

	while (p_stream0->enc_exit == 0) {
		if (HD_OK == hd_videoenc_poll_list(poll_list, 2, -1)) {    // multi path poll_list , -1 = blocking mode
			if (TRUE == poll_list[0].revent.event) {
				//pull data
				ret = hd_videoenc_pull_out_buf(p_stream0->enc_path, &data_pull, 0); // 0 = non-blocking mode

				if (ret == HD_OK) {
					// NOTE: mark
					UINT32 j;
					int is_keyframe = data_pull.frame_type == HD_FRAME_TYPE_IDR?1:0;

					frame_timestamp_ms = data_pull.timestamp/1000;
					frame_data_len = 0;

					for (j=0; j< data_pull.pack_num; j++) {
						UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.video_pack[j].phy_addr);
						UINT32 pkt_len = data_pull.video_pack[j].size;
						memcpy(&data_buf0[0]+frame_data_len, ptr, pkt_len);
						frame_data_len += pkt_len;						
					}
					if (write_rtp_frame_ext_video(0, 0, &data_buf0[0], frame_data_len, is_keyframe, frame_timestamp_ms) != 0) ERR_MSG("write_rtp_frame_ext_video fail!");
					// release data
					ret = hd_videoenc_release_out_buf(p_stream0->enc_path, &data_pull);
				}
			}

			if (TRUE == poll_list[1].revent.event) {
				//pull data
				ret = hd_videoenc_pull_out_buf(p_stream1->enc_path, &data_pull, 0); // 0 = non-blocking mode

				if (ret == HD_OK) {
					// NOTE: mark
					UINT32 j;
					int is_keyframe = data_pull.frame_type == HD_FRAME_TYPE_IDR?1:0;

					frame_timestamp_ms = data_pull.timestamp/1000;
					frame_data_len = 0;
					for (j=0; j< data_pull.pack_num; j++) {
						UINT8 *ptr = (UINT8 *)PHY2VIRT_SUB(data_pull.video_pack[j].phy_addr);
						UINT32 pkt_len = data_pull.video_pack[j].size;
						memcpy(&data_buf1[0]+frame_data_len, ptr, pkt_len);
						frame_data_len += pkt_len;
					}
					if (write_rtp_frame_ext_video(1, 0, &data_buf1[0], frame_data_len, is_keyframe, frame_timestamp_ms) != 0) ERR_MSG("write_rtp_frame_ext_video fail!");
					// release data
					ret = hd_videoenc_release_out_buf(p_stream1->enc_path, &data_pull);
					if (ret != HD_OK) {
						printf("enc_release error=%d !!\r\n", ret);
					}
				}
			}
		}
	}

	// mummap for bs buffer
	if (vir_addr_main) 	hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
	if (vir_addr_sub)  	hd_common_mem_munmap((void *)vir_addr_sub, phy_buf_sub.buf_info.buf_size);

	// close output file
	// NOTE: mark
	#if 0
	if (f_out_main) fclose(f_out_main);
	if (f_out_sub) fclose(f_out_sub);
	#endif

thread_end:
	if (data_buf0 != 0) free(data_buf0);
	if (data_buf1 != 0) free(data_buf1);
	printf("EXIT, encode_thread\n");
	pthread_exit(NULL);

	return NULL;
}

static void *ir_thread(void *arg)
{
	INT rt;
	INT32 option;
	UINT32 trig = 1;
	UINT32 id, dbg_en;
	//IQT_NIGHT_MODE night_mode = {0};
	AWBT_TH awb_parameter = {0};
	//AWBT_SCENE_MODE awb_scene = {0};
	ISPT_IR_INFO ir_info = {0};
	IQT_RGBIR_PARAM rgbir = {0};
	AET_CFG_INFO cfg_info = {0};
	if (vendor_isp_init() == HD_ERR_NG) {
		printf("vendor_isp_init fail\n");
		return 0;
	}
	cfg_info.id = 0;
#if 1 //new SDK
	strncpy(cfg_info.path, "/mnt/app/isp/isp_ar0237ir_0.cfg", CFG_NAME_LENGTH);
#else
	strncpy(cfg_info.path, "/etc/isp/isp_ar0237ir_0.cfg", CFG_NAME_LENGTH);
#endif
	vendor_isp_set_ae(AET_ITEM_RLD_CONFIG, &cfg_info);
	vendor_isp_set_awb(AWBT_ITEM_RLD_CONFIG, &cfg_info);
	vendor_isp_set_iq(IQT_ITEM_RLD_CONFIG, &cfg_info);
	vendor_isp_uninit();

	// open MCU device
	if (vendor_isp_init() == HD_ERR_NG) {
		return 0;
	}
usleep(1000*1000);
	while (trig) {
		option = 1;
		switch (option) {
		case 1:
			do {
				//printf("Set isp id (0, 1)>> \n");
				id = 0;//(UINT32)get_choose_int();
				ir_info.id = id;
				//night_mode.id = id;
				//awb_scene.id = id;
				rgbir.id = id;
			} while (0);
			do {
				//printf("Set dbg en (0, 1)>> \n");
				dbg_en = 1;//(UINT32)get_choose_int();
			} while (0);

			while (1){
				rt = vendor_isp_get_common(ISPT_ITEM_IR_INFO, &ir_info);
				if (rt < 0) {
					printf("set ISPT_ITEM_IR_INFO fail!\n");
					goto _end;
				}
				if (dbg_en == 1) {
					printf("IR LEVEL=%d\n",ir_info.info.ir_level);
				}

				rt = vendor_isp_get_iq(IQT_ITEM_RGBIR_PARAM, &rgbir);
				if (rt < 0) {
					printf("get IQT_ITEM_RGBIR_PARAM fail!\n");
					goto _end;
				}
				#if 0
				if (ir_info.info.ir_level > rgbir.rgbir.auto_param.night_mode_th) {
					night_mode.mode = 1;
					awb_scene.mode = 10;
					rt = vendor_isp_set_iq(IQT_ITEM_NIGHT_MODE, &night_mode);
					if (rt < 0) {
						printf("set IQT_ITEM_NIGHT_MODE fail!\n");
						goto _end;
					}
					rt = vendor_isp_set_awb(AWBT_ITEM_SCENE, &awb_scene);
					if (rt < 0) {
						printf("set AWBT_ITEM_SCENE fail!\n");
						goto _end;
					}
				} else if (ir_info.info.ir_level < 235) {
					night_mode.mode = 0;
					awb_scene.mode = 0;
					rt = vendor_isp_set_iq(IQT_ITEM_NIGHT_MODE, &night_mode);
					if (rt < 0) {
						printf("set IQT_ITEM_NIGHT_MODE fail!\n");
						goto _end;
					}
					rt = vendor_isp_set_awb(AWBT_ITEM_SCENE, &awb_scene);
					if (rt < 0) {
						printf("set AWBT_ITEM_SCENE fail!\n");
						goto _end;
					}
				}
				#endif

				if (ir_info.info.ir_level > 152) {
					rt = vendor_isp_get_awb(AWBT_ITEM_TH, &awb_parameter);
					if (rt < 0) {
						printf("get AWBT_ITEM_TH fail!\n");
						goto _end;
					}
					//awb_parameter.th.rmb_u = 124;
					awb_parameter.th.y_l = 1; 
			          awb_parameter.th.rpb_u = 400; 
			          awb_parameter.th.rsb_l = -80; 
			          awb_parameter.th.r2g_u = 250; 
			          awb_parameter.th.b2g_u = 200; 
			          awb_parameter.th.rmb_u = 250; 
					rt = vendor_isp_set_awb(AWBT_ITEM_TH, &awb_parameter);
					if (rt < 0) {
						printf("set AWBT_ITEM_TH fail!\n");
						goto _end;
					}
				} else {
					rt = vendor_isp_get_awb(AWBT_ITEM_TH, &awb_parameter);
					if (rt < 0) {
						printf("get AWBT_ITEM_TH fail!\n");
						goto _end;
					}
					//awb_parameter.th.rmb_u = 62;
					awb_parameter.th.y_l = 5; 
		          awb_parameter.th.rpb_u = 307; 
		          awb_parameter.th.rsb_l = -50; 
		          awb_parameter.th.r2g_u = 198; 
		          awb_parameter.th.b2g_u = 108; 
					awb_parameter.th.rmb_u = 62;
					rt = vendor_isp_set_awb(AWBT_ITEM_TH, &awb_parameter);
					if (rt < 0) {
						printf("set AWBT_ITEM_TH fail!\n");
						goto _end;
					}
				}

				usleep(1000*1000);
			}
			break;

		case 0:
		default:
			trig = 0;
			break;
		}
	}
_end:

	vendor_isp_uninit();

	return 0;
}

static void *aquire_yuv_thread(void *arg)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)arg;
	VIDEO_RECORD* p_stream1 = p_stream0+1;
	HD_RESULT ret = HD_OK;
	HD_VIDEO_FRAME video_frame = {0};
	UINT32 phy_addr_main, vir_addr_main;
	UINT32 yuv_size;
	char file_path_main[32] = {0};
	FILE *f_out_main;
	#define YUV_PHY2VIRT_MAIN(pa) (vir_addr_main + ((pa) - phy_addr_main))

	printf("\r\nif you want to snapshot, enter \"s\" to trigger !!\r\n");
	printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	//--------- pull data test ---------
	while (p_stream0->enc_exit == 0) {

		if(p_stream0->do_snap) {
			//
			ret = hd_videoproc_pull_out_buf(p_stream0->proc_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
            if (ret != HD_OK) {
				printf("pull_out(-1) error = %d!!\r\n", ret);
        		goto skip;
			}

			p_stream0->do_snap = 0;

			phy_addr_main = hd_common_mem_blk2pa(video_frame.blk); // Get physical addr
			if (phy_addr_main == 0) {
				printf("blk2pa fail, blk = 0x%x\r\n", video_frame.blk);
        			goto skip;
			}

			yuv_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);

			// mmap for frame buffer (just mmap one time only, calculate offset to virtual address later)
			vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_addr_main, yuv_size);
			if (vir_addr_main == 0) {
				printf("mmap error !!\r\n\r\n");
        			goto skip;
			}

			snprintf(file_path_main, 32, "/mnt/sd/dump_frm_yuv420_%lu.dat", p_stream0->shot_count);
			printf("dump snapshot frame to file (%s) ....\r\n", file_path_main);

			//----- open output files -----
			if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
				printf("open file (%s) fail....\r\n\r\n", file_path_main);
        			goto skip;
			}

			//save Y plane
			{
				UINT8 *ptr = (UINT8 *)YUV_PHY2VIRT_MAIN(video_frame.phy_addr[0]);
				UINT32 len = video_frame.loff[0]*video_frame.ph[0];
				if (f_out_main) fwrite(ptr, 1, len, f_out_main);
				if (f_out_main) fflush(f_out_main);
			}

			//save UV plane
			if(1)
         {
				UINT8 *ptr = (UINT8 *)YUV_PHY2VIRT_MAIN(video_frame.phy_addr[1]);
				UINT32 len = video_frame.loff[1]*video_frame.ph[1];
				if (f_out_main) fwrite(ptr, 1, len, f_out_main);
				if (f_out_main) fflush(f_out_main);
			}

			// mummap for frame buffer
			ret = hd_common_mem_munmap((void *)vir_addr_main, yuv_size);
			if (ret != HD_OK) {
				printf("mnumap error !!\r\n\r\n");
        			goto skip;
			}

			ret = hd_videoproc_release_out_buf(p_stream0->proc_path, &video_frame);
			if (ret != HD_OK) {
				printf("release_out() error !!\r\n\r\n");
        			goto skip;
			}

			// close output file
			fclose(f_out_main);

			printf("dump snapshot ok\r\n\r\n");

			p_stream0->shot_count ++;			
			
			
			//IR Path
			ret = hd_videoproc_pull_out_buf(p_stream1->proc_path2, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
            if (ret != HD_OK) {
				printf("pull_out(-1) error = %d!!\r\n", ret);
        		goto skip;
			}

			phy_addr_main = hd_common_mem_blk2pa(video_frame.blk); // Get physical addr
			if (phy_addr_main == 0) {
				printf("blk2pa fail, blk = 0x%x\r\n", video_frame.blk);
        			goto skip;
			}

			yuv_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);

			// mmap for frame buffer (just mmap one time only, calculate offset to virtual address later)
			vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_addr_main, yuv_size);
			if (vir_addr_main == 0) {
				printf("mmap error !!\r\n\r\n");
        			goto skip;
			}

			snprintf(file_path_main, 32, "/mnt/sd/dump_irfrm_yuv420_%lu.dat", p_stream1->shot_count);
			printf("dump snapshot frame to file (%s) ....\r\n", file_path_main);

			//----- open output files -----
			if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
				printf("open file (%s) fail....\r\n\r\n", file_path_main);
        			goto skip;
			}

			//save Y plane
			{
				UINT8 *ptr = (UINT8 *)YUV_PHY2VIRT_MAIN(video_frame.phy_addr[0]);
				UINT32 len = video_frame.loff[0]*video_frame.ph[0];
				if (f_out_main) fwrite(ptr, 1, len, f_out_main);
				if (f_out_main) fflush(f_out_main);
			}

			//save UV plane
			if(0)
         {
				UINT8 *ptr = (UINT8 *)YUV_PHY2VIRT_MAIN(video_frame.phy_addr[1]);
				UINT32 len = video_frame.loff[1]*video_frame.ph[1];
				if (f_out_main) fwrite(ptr, 1, len, f_out_main);
				if (f_out_main) fflush(f_out_main);
			}

			// mummap for frame buffer
			ret = hd_common_mem_munmap((void *)vir_addr_main, yuv_size);
			if (ret != HD_OK) {
				printf("mnumap error !!\r\n\r\n");
        			goto skip;
			}

			ret = hd_videoproc_release_out_buf(p_stream1->proc_path2, &video_frame);
			if (ret != HD_OK) {
				printf("release_out() error !!\r\n\r\n");
        			goto skip;
			}

			// close output file
			fclose(f_out_main);

			printf("dump IR snapshot ok\r\n\r\n");

			p_stream1->shot_count ++;
		}
skip:
		usleep(10000); //delay 10 ms
	}

	return 0;
}

MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	VIDEO_RECORD stream[2] = {0}; //0: main stream, 1: sub stream
	UINT32 enc_type = 0;
	UINT32 enc_bitrate = 4;
	HD_DIM main_dim;
	HD_DIM sub_dim;
	#if (LOAD_ISP_CFG == 1)
	AET_CFG_INFO cfg_info = {0};

	INT rt;
	INT32 option;
	//UINT32 trig = 1;
	UINT32 id, dbg_en;
	IQT_NIGHT_MODE night_mode = {0};
	AWBT_TH awb_parameter = {0};
	AWBT_SCENE_MODE awb_scene = {0};
	ISPT_IR_INFO ir_info = {0};
	IQT_RGBIR_PARAM rgbir = {0};

	#endif

	if (argc == 1 || argc > 10) {
		printf("Usage: <3DNR> <shdr_mode> <enc_type> <enc_bitrate> <data_mode> <data2_mode>.\r\n");
		printf("Help:\r\n");
		printf("  <3DNR>        : 0(disable), 1(enable)\r\n");
		printf("  <shdr_mode>   : 0(disable), 1(enable)\r\n");
		printf("  <enc_type>    : 0(H265), 1(H264)\r\n");
		printf("  <enc_bitrate> : Mbps\r\n");
		printf("  <data_mode>   : 0(D2D), 1(Direct)\r\n");
		printf("  <data2_mode>   : 0(D2D), 1(LowLatency)\r\n");
		printf("  <cap_fmt>   : 0(RAW12), 1(NRX12)\r\n");
		printf("  <prc_fmt>   : 0(YUV420), 1(YUV420_NVX2)\r\n");
		printf("  <fps>   : fps\r\n");
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
	if (argc > 7) {
		g_cap_out_fmt = atoi(argv[7]);
		if(g_cap_out_fmt > 1) {
			printf("error: not support g_cap_out_fmt! (%d) \r\n", g_cap_out_fmt);
			return 0;
		}
	}
	if (argc > 8) {
		g_prc_out_fmt = atoi(argv[8]);
		if(g_prc_out_fmt > 1) {
			printf("error: not support g_prc_out_fmt! (%d) \r\n", g_prc_out_fmt);
			return 0;
		}
	}
	if (argc > 9) {
		g_fps = atoi(argv[9]);
		if(g_fps < 1) {
			printf("error: not support g_fps! (%d) \r\n", g_fps);
			return 0;
		}
	}

// vendor ir
// create ir_thread (do ir event)
	ret = pthread_create(&ir_thread_id, NULL, ir_thread, NULL);
	if (ret < 0) {
		printf("create ir thread failed");
		goto exit;
	}
//
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

	// open video_record modules (sub)(sub2)
	stream[1].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[1].proc_max_dim.h = VDO_SIZE_H; //assign by user
	ret = open_module_2(&stream[1], &stream[1].proc_max_dim);
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
	sub_dim.w = SUB_VDO_SIZE_W;
	sub_dim.h = SUB_VDO_SIZE_H;

	// set videoproc parameter (main)
	if (g_prc_out_fmt == 1)
	{
		ret = set_proc_param(stream[0].proc_path, NULL, HD_VIDEO_PXLFMT_YUV420_NVX2, FALSE);
		if (ret != HD_OK) {
			printf("set proc fail=%d\n", ret);
			goto exit;
		}
	}
	else{
		ret = set_proc_param(stream[0].proc_path, NULL, HD_VIDEO_PXLFMT_YUV420, 1);
		if (ret != HD_OK) {
			printf("set proc fail=%d\n", ret);
			goto exit;
		}
	}

	// set videoproc parameter (sub)
	ret = set_proc_param_extend(stream[1].proc_path, NULL, IR_PXLFMT, FALSE, SRC_PATH);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

#if 	(IR_ISP_FUNC == 1)
	// set videoproc parameter (sub2)
	ret = set_proc_param(stream[1].proc_path2, NULL, HD_VIDEO_PXLFMT_Y8, 1);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}
    printf("stream[1].proc_path2 = 0x%08X\n",stream[1].proc_path2);
#endif

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
	ret = set_enc_param(stream[0].enc_path, &stream[0].enc_dim, SRC_PXLFMT, enc_type, enc_bitrate * 1024 * 1024);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}

	// set videoenc config (sub)
	stream[1].enc_max_dim.w = sub_dim.w;
	stream[1].enc_max_dim.h = sub_dim.h;
	ret = set_enc_cfg(stream[1].enc_path, &stream[1].enc_max_dim, 1 * 1024 * 1024, 0);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}

	// set videoenc parameter (sub)
	stream[1].enc_dim.w = sub_dim.w;
	stream[1].enc_dim.h = sub_dim.h;
	ret = set_enc_param(stream[1].enc_path, &stream[1].enc_dim, IR_ENCFMT, enc_type, 1 * 1024 * 1024);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}

	// bind video_record modules (main)
	hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOENC_0_IN_0);

#if 	(IR_ISP_FUNC == 1)
	// bind video_record modules (sub)
	hd_videoproc_bind(IR_PATH, HD_VIDEOPROC_1_IN_0);
	// bind video_record modules (sub2)
	hd_videoproc_bind(HD_VIDEOPROC_1_OUT_0, HD_VIDEOENC_0_IN_1);
#else
	// bind video_record modules (sub)
	hd_videoproc_bind(IR_PATH, HD_VIDEOENC_0_IN_1);
#endif

	// create encode_thread (pull_out bitstream)
	ret = pthread_create(&stream[0].enc_thread_id, NULL, encode_thread, (void *)stream);
	if (ret < 0) {
		printf("create encode thread failed");
		goto exit;
	}

    // create aquire_thread (pull_out frame)
	ret = pthread_create(&stream[0].aquire_thread_id, NULL, aquire_yuv_thread, (void *)stream);
	if (ret < 0) {
		printf("create aquire thread failed");
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
	// start video_record modules (sub)
	hd_videoproc_start(stream[1].proc_path);
#if 	(IR_ISP_FUNC == 1)
	// start video_record modules (sub2)
	hd_videoproc_start(stream[1].proc_path2);
#endif

	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);
	hd_videoenc_start(stream[0].enc_path);
	hd_videoenc_start(stream[1].enc_path);

	// let encode_thread start to work
	stream[0].flow_start = 1;
	
    //start rtspd
	rtspd_start(2,1,enc_type);

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
            stream[1].enc_exit = 1;
			// quit program
			break;
		}
        if (key == 's') {
			stream[0].do_snap = 1;
		}
		if(key == 'c' || key == 'C') 
		{
			if(rtsp_path)
				rtsp_path=0;
			else
				rtsp_path=1;
		}
		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			// enter debug menu
			hd_debug_run_menu();
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
	}

	// destroy encode thread
	pthread_join(stream[0].enc_thread_id, NULL);
    
    //stop rtspd
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

	// stop video_record modules (sub)
	hd_videoproc_stop(stream[1].proc_path);
#if 	(IR_ISP_FUNC == 1)
	// stop video_record modules (sub2)
	hd_videoproc_stop(stream[1].proc_path2);
#endif
	hd_videoenc_stop(stream[1].enc_path);

	// unbind video_record modules (main)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);

#if 	(IR_ISP_FUNC == 1)
	// unbind video_record modules (sub)
	hd_videoproc_unbind(IR_PATH);
	// unbind video_record modules (sub2)
	hd_videoproc_unbind(HD_VIDEOPROC_1_OUT_0);
#else
	// unbind video_record modules (sub)
	hd_videoproc_unbind(IR_PATH);
#endif

exit:
	// close video_record modules (main)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

	// close video_record modules (sub)(sub2)
	ret = close_module_2(&stream[1]);
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
