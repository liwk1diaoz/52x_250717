/**
	@brief Sample code of video rtsp.\n

	@file pq_video_rtsp.c

	@author Photon Lin

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
#include "nvtrtspd.h"
#include "vendor_isp.h"
#include "vendor_vpe.h"
#include "msf_lib.h"
#include "vendor_videocapture.h"
#include "kwrap/list.h"
#include "kwrap/semaphore.h"
#include <kwrap/file.h>
#include <kwrap/perf.h>
// platform dependent
#if defined(__LINUX)
#include <pthread.h> //for pthread API
#define MAIN(argc, argv) int main(int argc, char** argv)
#define GETCHAR() getchar()
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h> //for sleep API
#define sleep(x) vos_util_delay_ms(1000*(x))
#define msleep(x) vos_util_delay_ms(x)
#define usleep(x) vos_util_delay_us(x)
#include <kwrap/examsys.h> //for MAIN(), GETCHAR() API
#define MAIN(argc, argv) EXAMFUNC_ENTRY(pq_video_rtsp, argc, argv)
#define GETCHAR() NVT_EXAMSYS_GETCHAR()
#endif

#define SEN_SEL_NONE            0
#define SEN_SEL_IMX290          1
#define SEN_SEL_OS02K10         2
#define SEL_SEL_AR0237IR        3
#define SEN_SEL_OV2718          4
#define SEN_SEL_OS05A10         5
#define SEN_SEL_IMX317          6
#define SEN_SEL_IMX335          7
#define SEN_SEL_F37             8
#define SEN_SEL_PS5268          9
#define SEN_SEL_SC4210         10
#define SEN_SEL_IMX307_SLAVE   11
#define SEN_SEL_F35            12
#define SEN_SEL_IMX415         13
#define SEN_SEL_SC500AI        14
#define SEN_SEL_SC401AI        15
#define SEN_SEL_OS04A10        16
#define SEN_SEL_GC4653         17
#define SEN_SEL_OS05A10_DUAL   18
#define SEN_SEL_PATGEN         99

///////////////////////////////////////////////////////////////////////////////
//strip level
#define HD_VIDEOPROC_CFG                0x000F0000 //vprc
#define HD_VIDEOPROC_CFG_STRIP_MASK     0x00000007  //vprc stripe rule mask: (default 0)
#define HD_VIDEOPROC_CFG_STRIP_LV1      0x00000000  //vprc "0: cut w>1280, GDC =  on, 2D_LUT off after cut (LL slow)
#define HD_VIDEOPROC_CFG_STRIP_LV2      0x00010000  //vprc "1: cut w>2048, GDC = off, 2D_LUT off after cut (LL fast)
#define HD_VIDEOPROC_CFG_STRIP_LV3      0x00020000  //vprc "2: cut w>2688, GDC = off, 2D_LUT off after cut (LL middle)(2D_LUT best)
#define HD_VIDEOPROC_CFG_STRIP_LV4      0x00030000  //vprc "3: cut w> 720, GDC =  on, 2D_LUT off after cut (LL not allow)(GDC best)
#define HD_VIDEOPROC_CFG_DISABLE_GDC    HD_VIDEOPROC_CFG_STRIP_LV2
#define HD_VIDEOPROC_CFG_LL_FAST        HD_VIDEOPROC_CFG_STRIP_LV2
#define HD_VIDEOPROC_CFG_2DLUT_BEST     HD_VIDEOPROC_CFG_STRIP_LV3
#define HD_VIDEOPROC_CFG_GDC_BEST       HD_VIDEOPROC_CFG_STRIP_LV4

///////////////////////////////////////////////////////////////////////////////
//pixel start for patgen
#define PATGEN_START_PIX_R  HD_VIDEO_PIX_RGGB_R
#define PATGEN_START_PIX_GR HD_VIDEO_PIX_RGGB_GR
#define PATGEN_START_PIX_GB HD_VIDEO_PIX_RGGB_GB
#define PATGEN_START_PIX_B  HD_VIDEO_PIX_RGGB_B

#define PATGEN_START_PIX HD_VIDEO_PIX_RGGB_R

///////////////////////////////////////////////////////////////////////////////
#define HD_VIDEOPROC_PIPE_VPE 0x000000F2

//header
#define DBGINFO_BUFSIZE() (0x200)

//RAW
#define VDO_RAW_BUFSIZE(w, h, pxlfmt) (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NRX: RAW compress: Only support 12bit mode
#define RAW_COMPRESS_RATIO 59
#define VDO_NRX_BUFSIZE(w, h) (ALIGN_CEIL_4(ALIGN_CEIL_64(w) * 12 / 8 * RAW_COMPRESS_RATIO / 100 * (h)))
//CA for AWB
#define VDO_CA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 3) << 1)
//LA for AE
#define VDO_LA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 1) << 1)
//VA for AF
#define VDO_VA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 4) << 1)

//YUV
#define VDO_YUV_BUFSIZE(w, h, pxlfmt) (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NVX: YUV compress
#define YUV_COMPRESS_RATIO 75
#define VDO_NVX_BUFSIZE(w, h, pxlfmt) (VDO_YUV_BUFSIZE(w, h, pxlfmt) * YUV_COMPRESS_RATIO / 100)

/*****************************************************************************************************************/
#define SEN_OUT_FPS		 20
#define SEN_OUT_FMT      HD_VIDEO_PXLFMT_NRX12
#define CAP_OUT_FMT      HD_VIDEO_PXLFMT_NRX12
#define SHDR_CAP_OUT_FMT HD_VIDEO_PXLFMT_RAW12_SHDR2  // Encode: HD_VIDEO_PXLFMT_NRX12_SHDR2
#define CA_WIN_NUM_W     32
#define CA_WIN_NUM_H     32
#define LA_WIN_NUM_W     32
#define LA_WIN_NUM_H     32
#define VA_WIN_NUM_W      8
#define VA_WIN_NUM_H      8
#define YOUT_WIN_NUM_W  128
#define YOUT_WIN_NUM_H  128
#define ETH_8BIT_SEL      0 //0: 2bit out, 1:8 bit out
#define ETH_OUT_SEL       1 //0: full, 1: subsample 1/2

#define VDO_SIZE_W_8M      3840
#define VDO_SIZE_H_8M      2160
#define VDO_SIZE_W_5M_BIG  2880
#define VDO_SIZE_H_5M_BIG  1620
#define VDO_SIZE_W_5M      2592
#define VDO_SIZE_H_5M      1944
#define VDO_SIZE_W_4M      2560
#define VDO_SIZE_H_4M      1440
#define VDO_SIZE_W_4M_BIG  2688
#define VDO_SIZE_H_4M_BIG  1520
#define VDO_SIZE_W_2M      1920
#define VDO_SIZE_H_2M      1080

#define VIDEOCAP_ALG_FUNC HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB
#define VIDEOPROC_ALG_FUNC HD_VIDEOPROC_FUNC_AF | HD_VIDEOPROC_FUNC_WDR | HD_VIDEOPROC_FUNC_DEFOG | HD_VIDEOPROC_FUNC_COLORNR | HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_3DNR_STA

static UINT32 sensor_sel_1 = 0;
static UINT32 sensor_sel_2 = 0;
static UINT32 sensor_shdr_1 = 0;
static UINT32 sensor_shdr_2 = 0;
static UINT32 data_mode1 = 0; //option for vcap output directly to vprc	(data_mode1 = 1 direct mode)
static UINT32 data_mode2 = 0; //option for vprc output lowlatency to venc
static UINT32 patgen_size_w = 1920, patgen_size_h = 1080;
static UINT32 vdo_size_w_1 = 1920, vdo_size_h_1 = 1080;
static UINT32 vdo_size_w_2 = 1920, vdo_size_h_2 = 1080;
static UINT32 msf_mode = 0;	//1: enable msf_mode
static UINT32 ir_dewarp_mode = 0;	//1: enable ir dewarp_mode
static UINT32 yuv_dewarp_mode = 0;	//1: enable yuv dewarp_mode
static BOOL is_nt98528 = 0;
static VPET_2DLUT_PARAM lut2d[2] = {0};
#define VPROC_IPP_DIR HD_VIDEO_DIR_MIRRORX
/*****************************************************************************************************************/
#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))
#define PHY2VIRT_SUB(pa) (vir_addr_sub + (pa - phy_buf_sub.buf_info.phy_addr))

static UINT32 vir_addr_main;
static HD_VIDEOENC_BUFINFO phy_buf_main;
static UINT32 vir_addr_sub;
static HD_VIDEOENC_BUFINFO phy_buf_sub;

static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE() +
                                    VDO_RAW_BUFSIZE(vdo_size_w_1, vdo_size_h_1, CAP_OUT_FMT) +
                                    VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H) +
                                    VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H) +
                                    VDO_VA_BUF_SIZE(VA_WIN_NUM_W, VA_WIN_NUM_H);

	if (is_nt98528) {
		mem_cfg.pool_info[0].blk_cnt = 3;
		if (sensor_sel_2 != SEN_SEL_NONE) {
			mem_cfg.pool_info[0].blk_cnt += 3;
		}
	} else {
		mem_cfg.pool_info[0].blk_cnt = 4;
	}
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(vdo_size_w_1, vdo_size_h_1, HD_VIDEO_PXLFMT_YUV420);
	if (is_nt98528) {
		mem_cfg.pool_info[1].blk_cnt = 2;
		if (sensor_sel_2 != SEN_SEL_NONE) {
			mem_cfg.pool_info[1].blk_cnt += 2;
		}
	} else {
		mem_cfg.pool_info[1].blk_cnt = 4;
	}
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	if (msf_mode) {
		//for queue sync frame
		mem_cfg.pool_info[1].blk_cnt += 2;
		//for enc frame
		mem_cfg.pool_info[1].blk_cnt += 1;
		//for dewarp plane buffer
		if (ir_dewarp_mode) {
			mem_cfg.pool_info[1].blk_cnt += 2;
		}

		// config common pool (msf working buffer)
		mem_cfg.pool_info[2].type = HD_COMMON_MEM_COMMON_POOL;
		mem_cfg.pool_info[2].blk_size = msf_get_work_buf_szie(vdo_size_w_1, vdo_size_h_1);
		mem_cfg.pool_info[2].blk_cnt = 1;
		mem_cfg.pool_info[2].ddr_id = DDR_ID0;
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

	if (sensor_sel_1 == SEN_SEL_IMX290) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
	} else if (sensor_sel_1 == SEN_SEL_OS05A10) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os05a10");
	} else if (sensor_sel_1 == SEN_SEL_OS02K10) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os02k10");
	} else if (sensor_sel_1 == SEL_SEL_AR0237IR) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_ar0237ir");
	} else if (sensor_sel_1 == SEN_SEL_OV2718) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_ov2718");
	} else if (sensor_sel_1 == SEN_SEL_IMX317) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx317");
	} else if (sensor_sel_1 == SEN_SEL_IMX335) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx335");
	} else if (sensor_sel_1 == SEN_SEL_F37) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_f37");
	}else if (sensor_sel_1 == SEN_SEL_PS5268) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_ps5268");
	} else if (sensor_sel_1 == SEN_SEL_SC4210) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_sc4210");
	} else if (sensor_sel_1 == SEN_SEL_IMX307_SLAVE) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx307_slave");
	} else if (sensor_sel_1 == SEN_SEL_F35) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_f35");
	} else if (sensor_sel_1 == SEN_SEL_IMX415) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx415");
	} else if (sensor_sel_1 == SEN_SEL_SC500AI) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_sc500ai");
	} else if (sensor_sel_1 == SEN_SEL_SC401AI) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_sc401ai");
	} else if (sensor_sel_1 == SEN_SEL_OS04A10) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os04a10");
	} else if (sensor_sel_1 == SEN_SEL_GC4653) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_gc4653");
	} else if (sensor_sel_1 == SEN_SEL_OS05A10_DUAL) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os05a10");
	} else if (sensor_sel_1 == SEN_SEL_PATGEN) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, HD_VIDEOCAP_SEN_PAT_GEN);
	} else {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "NULL");
	}

	printf("sensor 1 using %s \n", cap_cfg.sen_cfg.sen_dev.driver_name);

	if (sensor_sel_1 == SEL_SEL_AR0237IR) {
		// Parallel interface
		cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_P_RAW;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux = 0x204; //PIN_SENSOR_CFG_12BITS | PIN_SENSOR_CFG_MCLK
	} else if (sensor_sel_1 == SEN_SEL_IMX307_SLAVE) {
		cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux = 0x2A0; //PIN_SENSOR_CFG_LVDS_VDHD | PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	} else {
		// MIPI interface
		cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux = 0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	}

	if (sensor_sel_1 == SEN_SEL_IMX307_SLAVE) {
		cap_cfg.sen_cfg.sen_dev.if_cfg.tge.tge_en = TRUE;
		cap_cfg.sen_cfg.sen_dev.if_cfg.tge.swap = FALSE;
		cap_cfg.sen_cfg.sen_dev.if_cfg.tge.vcap_vd_src = HD_VIDEOCAP_SEN_TGE_CH1_VD_TO_VCAP0;
		if (sensor_sel_2 == SEN_SEL_IMX307_SLAVE) {
			cap_cfg.sen_cfg.sen_dev.if_cfg.tge.vcap_sync_set = (HD_VIDEOCAP_0|HD_VIDEOCAP_1);
		}
	}

	//Sensor interface choice
	if (sensor_sel_2 != SEN_SEL_NONE) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x301;
	} else if (sensor_sel_1 == SEN_SEL_IMX307_SLAVE) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x301;
	} else if (sensor_sel_1 == SEL_SEL_AR0237IR) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x0;
	} else {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xF01;//PIN_MIPI_LVDS_CFG_CLK0 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
	if (sensor_sel_2 != SEN_SEL_NONE) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = HD_VIDEOCAP_SEN_IGNORE;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = HD_VIDEOCAP_SEN_IGNORE;
	} else if (sensor_sel_1 == SEN_SEL_IMX307_SLAVE) {
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

	if (sensor_shdr_1 == 1) {
		if (is_nt98528) {
			cap_cfg.sen_cfg.shdr_map = HD_VIDEOCAP_SHDR_MAP(HD_VIDEOCAP_HDR_SENSOR1, (HD_VIDEOCAP_0|HD_VIDEOCAP_3));
		} else {
			cap_cfg.sen_cfg.shdr_map = HD_VIDEOCAP_SHDR_MAP(HD_VIDEOCAP_HDR_SENSOR1, (HD_VIDEOCAP_0|HD_VIDEOCAP_1));
		}
	}

	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = VIDEOCAP_ALG_FUNC;

	if (sensor_shdr_1 == 1) {
		iq_ctl.func |= HD_VIDEOCAP_FUNC_SHDR;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}

static HD_RESULT set_cap_cfg2(HD_PATH_ID *p_video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};

	if (sensor_sel_2 == SEN_SEL_IMX290) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
	} else if (sensor_sel_2 == SEN_SEL_OS05A10) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os05a10");
	} else if (sensor_sel_2 == SEN_SEL_OS02K10) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os02k10");
	} else if (sensor_sel_2 == SEL_SEL_AR0237IR) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_ar0237ir");
	} else if (sensor_sel_2 == SEN_SEL_OV2718) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_ov2718");
	} else if (sensor_sel_2 == SEN_SEL_IMX317) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx317");
	} else if (sensor_sel_2 == SEN_SEL_IMX335) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx335");
	} else if (sensor_sel_2 == SEN_SEL_F37) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_f37");
	} else if (sensor_sel_2 == SEN_SEL_PS5268) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_ps5268");
	} else if (sensor_sel_2 == SEN_SEL_SC4210) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_sc4210");
	} else if (sensor_sel_2 == SEN_SEL_IMX307_SLAVE) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx307_slave");
	} else if (sensor_sel_2 == SEN_SEL_F35) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_f35");
	} else if (sensor_sel_2 == SEN_SEL_IMX415) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx415");
	} else if (sensor_sel_2 == SEN_SEL_SC500AI) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_sc500ai");
	} else if (sensor_sel_2 == SEN_SEL_SC401AI) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_sc401ai");
	} else if (sensor_sel_2 == SEN_SEL_OS04A10) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os04a10");
	}else if (sensor_sel_2 == SEN_SEL_GC4653) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_gc4653");
	}else if (sensor_sel_2 == SEN_SEL_OS05A10_DUAL) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os05a10");
	} else {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "NULL");
	}

	printf("sensor 2 using %s\n", cap_cfg.sen_cfg.sen_dev.driver_name);

	if (sensor_sel_2 == SEL_SEL_AR0237IR) {
		// Parallel interface
		cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_P_RAW;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux = 0x204; //PIN_SENSOR_CFG_12BITS | PIN_SENSOR2_CFG_MCLK
	} else if (sensor_sel_2 == SEN_SEL_IMX307_SLAVE) {
		cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux = 0x2A0; //PIN_SENSOR_CFG_LVDS_VDHD | PIN_SENSOR_CFG_MIPI | PIN_SENSOR2_CFG_MCLK
	} else if (sensor_sel_2 == SEN_SEL_OS05A10_DUAL) {
		cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux = 0x220; //PIN_SENSOR2_CFG_MIPI | PIN_SENSOR2_CFG_MCLK
	} else {
		// MIPI interface
		cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux = 0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR2_CFG_MCLK
	}

	if (sensor_sel_2 == SEN_SEL_IMX307_SLAVE) {
		cap_cfg.sen_cfg.sen_dev.if_cfg.tge.tge_en = TRUE;
		cap_cfg.sen_cfg.sen_dev.if_cfg.tge.swap = FALSE;
		cap_cfg.sen_cfg.sen_dev.if_cfg.tge.vcap_vd_src = HD_VIDEOCAP_SEN_TGE_CH1_VD_TO_VCAP0;
		if (sensor_sel_1 == SEN_SEL_IMX307_SLAVE) {
			cap_cfg.sen_cfg.sen_dev.if_cfg.tge.vcap_sync_set = (HD_VIDEOCAP_0|HD_VIDEOCAP_1);
		}
	}

	//Sensor interface choice
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xC02;//PIN_MIPI_LVDS_CFG_CLK1 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
	if (sensor_sel_2 == SEN_SEL_OS05A10_DUAL) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	} else {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x01;//PIN_I2C_CFG_CH1
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI1_USE_C1;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 1;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_1_CTRL, &video_cap_ctrl); //open this for device control

	if (ret != HD_OK) {
		return ret;
	}

	if (is_nt98528 && (sensor_shdr_2 == 1)) {
		cap_cfg.sen_cfg.shdr_map = HD_VIDEOCAP_SHDR_MAP(HD_VIDEOCAP_HDR_SENSOR2, (HD_VIDEOCAP_1|HD_VIDEOCAP_4));
	}

	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = VIDEOCAP_ALG_FUNC;

	if (is_nt98528 && (sensor_shdr_2 == 1)) {
		iq_ctl.func |= HD_VIDEOCAP_FUNC_SHDR;
	}

	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}

static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim, UINT32 path)
{
	HD_RESULT ret = HD_OK;
	UINT32 color_bar_width = 200;
	{
		HD_VIDEOCAP_IN video_in_param = {0};

		if (sensor_sel_1 != SEN_SEL_PATGEN) {
			video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
			video_in_param.frc = HD_VIDEO_FRC_RATIO(SEN_OUT_FPS,1);
			video_in_param.dim.w = p_dim->w;
			video_in_param.dim.h = p_dim->h;
			video_in_param.pxlfmt = SEN_OUT_FMT;
		} else {
			color_bar_width = (patgen_size_w >> 3) & 0xFFFFFFFE;
			video_in_param.sen_mode = HD_VIDEOCAP_PATGEN_MODE(HD_VIDEOCAP_SEN_PAT_COLORBAR, color_bar_width);
			video_in_param.frc = HD_VIDEO_FRC_RATIO(SEN_OUT_FPS,1);
			video_in_param.dim.w = p_dim->w;
			video_in_param.dim.h = p_dim->h;
		}

		if (((sensor_shdr_1 == 1) && (path == 0)) || ((sensor_shdr_2 == 1) && (path == 1))) {
			video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_2;
		} else {
			video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
		}

		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN, &video_in_param);
		if (ret != HD_OK) {
			return ret;
		}
	}

	//no crop, full frame
	{
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_OFF;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &video_crop_param);
	}

	{
		HD_VIDEOCAP_OUT video_out_param = {0};

		//without setting dim for no scaling, using original sensor out size
		if ((sensor_shdr_1 == 1) || (sensor_shdr_2 == 1)) {
			video_out_param.pxlfmt = SHDR_CAP_OUT_FMT;
		} else {
			video_out_param.pxlfmt = CAP_OUT_FMT;

			if (sensor_sel_1 == SEN_SEL_PATGEN) {
				video_out_param.pxlfmt |= PATGEN_START_PIX;
				printf("patgen pxlfmt: 0x%X \r\n", video_out_param.pxlfmt);
			}
		}

		video_out_param.dir = HD_VIDEO_DIR_NONE;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
	}
	if (data_mode1) {//direct mode
		HD_VIDEOCAP_FUNC_CONFIG video_path_param = {0};

		video_path_param.out_func = HD_VIDEOCAP_OUTFUNC_DIRECT;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_FUNC_CONFIG, &video_path_param);
	}
	if ((sensor_sel_1 == SEN_SEL_IMX307_SLAVE) ||(sensor_sel_1 == SEN_SEL_OS02K10) || (sensor_sel_1 == SEN_SEL_OS04A10) || (sensor_sel_2 != SEN_SEL_NONE) ) {
		UINT32 data_lane = 2;
		vendor_videocap_set(video_cap_path, VENDOR_VIDEOCAP_PARAM_DATA_LANE, &data_lane);
	}

	return ret;
}

static HD_RESULT set_proc_cfg(HD_PATH_ID *p_video_proc_ctrl, HD_DIM* p_max_dim, HD_OUT_ID _out_id, UINT32 isp_id)
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
		video_cfg_param.isp_id = isp_id;

		video_cfg_param.ctrl_max.func = VIDEOPROC_ALG_FUNC;
		if ((sensor_shdr_1 == 1) || (sensor_shdr_2 == 1)) {
			video_cfg_param.ctrl_max.func |= HD_VIDEOPROC_FUNC_SHDR;
		} else {
			video_cfg_param.ctrl_max.func &= ~HD_VIDEOPROC_FUNC_SHDR;
		}
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;

		if ((sensor_shdr_1 == 1) || (sensor_shdr_2 == 1)) {
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
		if (data_mode1) {
			video_path_param.in_func |= HD_VIDEOPROC_INFUNC_DIRECT; //direct NOTE: enable direct
		}
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
	}

	video_ctrl_param.func = VIDEOPROC_ALG_FUNC;
	if ((HD_CTRL_ID)_out_id == HD_VIDEOPROC_0_CTRL) {
		video_ctrl_param.ref_path_3dnr = HD_VIDEOPROC_0_OUT_0;
	} else {
		video_ctrl_param.ref_path_3dnr = HD_VIDEOPROC_1_OUT_0;
	}
	if ((sensor_shdr_1 == 1) || (sensor_shdr_2 == 1)) {
		video_ctrl_param.func |= HD_VIDEOPROC_FUNC_SHDR;
	} else {
		video_ctrl_param.func &= ~HD_VIDEOPROC_FUNC_SHDR;
	}

	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);

	*p_video_proc_ctrl = video_proc_ctrl;

	return ret;
}

static HD_RESULT set_proc_cfg2(HD_PATH_ID *p_video_proc_ctrl, HD_DIM* p_max_dim, HD_OUT_ID _out_id, UINT32 isp_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
	HD_PATH_ID video_proc_ctrl = 0;

	ret = hd_videoproc_open(0, _out_id, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	if (p_max_dim != NULL ) {
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_VPE;
		video_cfg_param.isp_id = isp_id;
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

	{
		HD_VIDEOPROC_FUNC_CONFIG video_path_param = {0};

		video_path_param.in_func = 0;
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
	}

	video_ctrl_param.func = 0;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);
	*p_video_proc_ctrl = video_proc_ctrl;
	return ret;
}

static void set_vpe_cfg(UINT32 id, char *file_name)
{
	VOS_FILE fp;
	struct vos_stat stat;
	int rt;
	VPET_DCE_CTL_PARAM dce_ctl = {0};
	VPET_DRT_PARAM drt = {0};

	if (file_name == 0) {
		return;
	}

	//load 2d lut table
	rt = vos_file_stat(file_name, &stat);
	if (rt != 0) {
		printf("error file(%s) get stat fail\r\n", file_name);
		return;
	}

	lut2d[id].id = id;
	if (stat.st_size == (260 * 257 * 4)) {
		lut2d[id].lut2d.lut_sz = VPE_ISP_2DLUT_SZ_257X257;
	} else if (stat.st_size == (68 * 65 * 4)) {
		lut2d[id].lut2d.lut_sz = VPE_ISP_2DLUT_SZ_65X65;
	} else {
		printf("file(%s) size(%d) err\r\n", file_name, stat.st_size);
		return;
	}

	fp = vos_file_open(file_name, O_RDONLY, 0);
	if (fp == VOS_FILE_INVALID) {
		printf("file(%s) open fail\r\n", file_name);
		return;
	}
	vos_file_read(fp, &lut2d[id].lut2d.lut[0], stat.st_size);
	vos_file_close(fp);
	vendor_vpe_set_cmd(VPET_ITEM_2DLUT_PARAM, &lut2d[id]);

	//enable 2d-lut function
	dce_ctl.id = id;
	dce_ctl.dce_ctl.enable = 1;
	dce_ctl.dce_ctl.dce_mode = VPE_ISP_DCE_MODE_2DLUT_ONLY; // 0:GDC, 1:2DLUT
	dce_ctl.dce_ctl.lens_radius = 0;
	vendor_vpe_set_cmd(VPET_ITEM_DCE_CTL_PARAM, &dce_ctl);

	drt.id = id;
	drt.drt.cvt_sel[0] = 0; //path0
	drt.drt.cvt_sel[1] = 0; //path1
	drt.drt.cvt_sel[2] = 0; //path2
	drt.drt.cvt_sel[3] = 0; //path3
	vendor_vpe_set_cmd(VPET_ITEM_DRT_PARAM, &drt);
}

static HD_RESULT set_proc_flip_xy(HD_PATH_ID video_proc_path, UINT32 param)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_IN video_in_param = {0};

	hd_videoproc_get(video_proc_path, HD_VIDEOPROC_PARAM_IN, &video_in_param);
	video_in_param.dir = param;
	hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_IN, &video_in_param);
	return ret;
}

static HD_RESULT set_proc_param(HD_PATH_ID video_proc_path, HD_DIM* p_dim, UINT32 depth)
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
		if (msf_mode) {
			video_out_param.depth = depth;
		}
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}
	{
		HD_VIDEOPROC_FUNC_CONFIG video_path_param = {0};

		if (data_mode2) {
			video_path_param.out_func |= HD_VIDEOPROC_OUTFUNC_LOWLATENCY; //enable low-latency
		}
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
	}

	return ret;
}

static HD_RESULT set_enc_cfg(HD_PATH_ID video_enc_path, HD_DIM *p_max_dim, UINT32 max_bitrate, UINT32 isp_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_PATH_CONFIG video_path_config = {0};
	HD_VIDEOENC_FUNC_CONFIG video_func_config = {0};

	if (p_max_dim != NULL) {

		//--- HD_VIDEOENC_PARAM_PATH_CONFIG ---
		video_path_config.max_mem.codec_type      = HD_CODEC_TYPE_H264;
		video_path_config.max_mem.max_dim.w       = p_max_dim->w;
		video_path_config.max_mem.max_dim.h       = p_max_dim->h;
		video_path_config.max_mem.bitrate         = max_bitrate;
		video_path_config.max_mem.enc_buf_ms      = 3000;
		video_path_config.max_mem.svc_layer       = HD_SVC_DISABLE;
		video_path_config.max_mem.ltr             = FALSE;
		video_path_config.max_mem.rotate          = FALSE;
		video_path_config.max_mem.source_output   = FALSE;
		video_path_config.isp_id                  = isp_id;

		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_PATH_CONFIG, &video_path_config);
		if (ret != HD_OK) {
			printf("set_enc_path_config = %d\r\n", ret);
			return HD_ERR_NG;
		}

		if (data_mode2) {
			video_func_config.in_func |= HD_VIDEOENC_INFUNC_LOWLATENCY; //enable low-latency
		}

		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_FUNC_CONFIG, &video_func_config);
		if (ret != HD_OK) {
			printf("set_enc_func_config = %d\r\n", ret);
			return HD_ERR_NG;
		}
	}

	return ret;
}

static HD_RESULT set_enc_param(HD_PATH_ID video_enc_path, HD_DIM *p_dim, UINT32 enc_type, UINT32 bitrate)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_IN  video_in_param = {0};
	HD_VIDEOENC_OUT video_out_param = {0};
	HD_H26XENC_RATE_CONTROL rc_param = {0};
	HD_H26XENC_VUI vui = {0};

	if (p_dim != NULL) {

		//--- HD_VIDEOENC_PARAM_IN ---
		video_in_param.dir     = HD_VIDEO_DIR_NONE;
		video_in_param.pxl_fmt = HD_VIDEO_PXLFMT_YUV420;
		video_in_param.dim.w   = p_dim->w;
		video_in_param.dim.h   = p_dim->h;
		video_in_param.frc     = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_IN, &video_in_param);
		if (ret != HD_OK) {
			printf("set_enc_param_in = %d\r\n", ret);
			return ret;
		}

		if (enc_type == 0) {
			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_H265;
			video_out_param.h26x.profile       = HD_H265E_MAIN_PROFILE;
			video_out_param.h26x.level_idc     = HD_H265E_LEVEL_5;
			video_out_param.h26x.gop_num       = 60;
			video_out_param.h26x.ltr_interval  = 0;
			video_out_param.h26x.ltr_pre_ref   = 0;
			video_out_param.h26x.gray_en       = 0;
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
			if (is_nt98528) {
				if (vdo_size_w_1 <= VDO_SIZE_W_2M) {
					rc_param.cbr.ip_weight       = 0;
				} else if (vdo_size_w_1 <= VDO_SIZE_W_4M) {
					rc_param.cbr.ip_weight       = -30;
				} else {
					rc_param.cbr.ip_weight       = -40;
				}
			}
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control = %d\r\n", ret);
				return ret;
			}

			vui.vui_en = TRUE;
			vui.color_range = 1; // 0: Not full range, 1: Full range
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_VUI , &vui);
			if (ret != HD_OK) {
				printf("set_enc_out_ui = %d\r\n", ret);
				return ret;
			}
		} else if (enc_type == 1) {
			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_H264;
			video_out_param.h26x.profile       = HD_H264E_HIGH_PROFILE;
			video_out_param.h26x.level_idc     = HD_H264E_LEVEL_5_1;
			video_out_param.h26x.gop_num       = 60;
			video_out_param.h26x.ltr_interval  = 0;
			video_out_param.h26x.ltr_pre_ref   = 0;
			video_out_param.h26x.gray_en       = 0;
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
			if (is_nt98528) {
				if (vdo_size_w_1 <= VDO_SIZE_W_2M) {
					rc_param.cbr.ip_weight       = 0;
				} else if (vdo_size_w_1 <= VDO_SIZE_W_4M) {
					rc_param.cbr.ip_weight       = -30;
				} else {
					rc_param.cbr.ip_weight       = -40;
				}
			}
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control = %d\r\n", ret);
				return ret;
			}

			vui.vui_en = TRUE;
			vui.color_range = 1; // 0: Not full range, 1: Full range
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_VUI , &vui);
			if (ret != HD_OK) {
				printf("set_enc_out_ui = %d\r\n", ret);
				return ret;
			}
		} else if (enc_type == 2) {
			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_JPEG;
			video_out_param.jpeg.retstart_interval = 0;
			video_out_param.jpeg.image_quality = bitrate / (1024 * 1024);
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

/*****************************************************************************************************************/
typedef struct _VIDEO_RECORD {

	// (1) vcap
	HD_VIDEOCAP_SYSCAPS cap_syscaps;
	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	HD_DIM  cap_dim;
	HD_DIM  proc_max_dim;

	// (2) vproc(0: ipp, 1: vpe)
	HD_VIDEOPROC_SYSCAPS proc_syscaps[2];
	HD_PATH_ID proc_ctrl[2];
	HD_PATH_ID proc_path[2];

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

	// set videoproc config(ipp)
	ret = set_proc_cfg(&p_stream->proc_ctrl[0], p_proc_max_dim, HD_VIDEOPROC_0_CTRL, 0);
	if (ret != HD_OK) {
		printf("set proc-cfg(ipp) fail=%d\n", ret);
		return HD_ERR_NG;
	}

	// set videoproc config(vpe)
	ret = set_proc_cfg2(&p_stream->proc_ctrl[1], p_proc_max_dim, HD_VIDEOPROC_2_CTRL, 0);
	if (ret != HD_OK) {
		printf("set proc-cfg(vpe) fail=%d\n", ret);
		return HD_ERR_NG;
	}

	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path[0])) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_2_IN_0, HD_VIDEOPROC_2_OUT_0, &p_stream->proc_path[1])) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_0, HD_VIDEOENC_0_OUT_0, &p_stream->enc_path)) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT open_module2(VIDEO_RECORD *p_stream, HD_DIM* p_proc_max_dim)
{
	HD_RESULT ret;

	// set videocap config
	ret = set_cap_cfg2(&p_stream->cap_ctrl);
	if (ret != HD_OK) {
		printf("set cap-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}

	// set videoproc config(ipp)
	ret = set_proc_cfg(&p_stream->proc_ctrl[0], p_proc_max_dim, HD_VIDEOPROC_1_CTRL, 1);
	if (ret != HD_OK) {
		printf("set proc-cfg(ipp) fail=%d\n", ret);
		return HD_ERR_NG;
	}

	// set videoproc config(vpe)
	ret = set_proc_cfg2(&p_stream->proc_ctrl[1], p_proc_max_dim, HD_VIDEOPROC_3_CTRL, 1);
	if (ret != HD_OK) {
		printf("set proc-cfg(vpe) fail=%d\n", ret);
		return HD_ERR_NG;
	}

	if ((ret = hd_videocap_open(HD_VIDEOCAP_1_IN_0, HD_VIDEOCAP_1_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_1_IN_0, HD_VIDEOPROC_1_OUT_0, &p_stream->proc_path[0])) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_3_IN_0, HD_VIDEOPROC_3_OUT_0, &p_stream->proc_path[1])) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_1, HD_VIDEOENC_0_OUT_1, &p_stream->enc_path)) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT close_module(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret;

	if ((ret = hd_videocap_close(p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path[0])) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path[1])) != HD_OK)
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

typedef struct _VIDEO_INFO {
	VIDEO_RECORD *p_stream;
	HD_VIDEOENC_BS  data_pull;
	NVTLIVE555_CODEC codec_type;
	unsigned char vps[64];
	int vps_size;
	unsigned char sps[64];
	int sps_size;
	unsigned char pps[64];
	int pps_size;
	int ref_cnt;
} VIDEO_INFO;

typedef struct _ADUIO_INFO {
	int ref_cnt;
	NVTLIVE555_AUDIO_INFO info;
} AUDIO_INFO;

//0:stream0, 1:stream1
static VIDEO_INFO video_info[2] = { 0 };
//static AUDIO_INFO audio_info[1] = { 0 };

static int flush_video(VIDEO_INFO *p_video_info)
{
	while (hd_videoenc_pull_out_buf(p_video_info->p_stream->enc_path, &p_video_info->data_pull, 0) == 0) {
		hd_videoenc_release_out_buf(p_video_info->p_stream->enc_path, &p_video_info->data_pull);
	}

	HD_H26XENC_REQUEST_IFRAME req_i = {0};
	req_i.enable   = 1;
	int ret = hd_videoenc_set(p_video_info->p_stream->enc_path, HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME, &req_i);
	if (ret != HD_OK) {
		printf("set_enc_param_out_request_ifrm0 = %d\r\n", ret);
		return ret;
	}
	hd_videoenc_start(p_video_info->p_stream->enc_path);
	return ret;
}

static int on_open_video(int channel)
{
	if ((size_t)channel >= sizeof(video_info) / sizeof(video_info[0])) {
		printf("nvtrtspd video channel exceed\n");
		return 0;
	}
	if (video_info[channel].ref_cnt != 0) {
		printf("nvtrtspd video in use.\n");
		return 0;
	}

	//no video
	if (video_info[channel].codec_type == NVTLIVE555_CODEC_UNKNOWN) {
		return 0;
	}

	video_info[channel].ref_cnt++;


	flush_video(&video_info[channel]);
	return (int)&video_info[channel];
}

static int on_close_video(int handle)
{
	if (handle) {
		VIDEO_INFO *p_info = (VIDEO_INFO *)handle;
		p_info->ref_cnt--;
	}
	return 0;
}

static int refresh_video_info(int id, VIDEO_INFO *p_video_info)
{
	//while to get vsp, sps, pps
	int ret;
	p_video_info->codec_type = NVTLIVE555_CODEC_UNKNOWN;
	p_video_info->vps_size = p_video_info->sps_size = p_video_info->pps_size = 0;
	memset(p_video_info->vps, 0, sizeof(p_video_info->vps));
	memset(p_video_info->sps, 0, sizeof(p_video_info->sps));
	memset(p_video_info->pps, 0, sizeof(p_video_info->pps));
	while ((ret = hd_videoenc_pull_out_buf(p_video_info->p_stream->enc_path, &p_video_info->data_pull, 500)) == 0) {
		if (p_video_info->data_pull.vcodec_format == HD_CODEC_TYPE_JPEG) {
			p_video_info->codec_type = NVTLIVE555_CODEC_MJPG;
			hd_videoenc_release_out_buf(p_video_info->p_stream->enc_path, &p_video_info->data_pull);
			break;
		} else if (p_video_info->data_pull.vcodec_format == HD_CODEC_TYPE_H264) {
			p_video_info->codec_type = NVTLIVE555_CODEC_H264;
			if (p_video_info->data_pull.pack_num != 3) {
				hd_videoenc_release_out_buf(p_video_info->p_stream->enc_path, &p_video_info->data_pull);
				continue;
			}

			if (id == 0) {
				memcpy(p_video_info->sps, (void *)PHY2VIRT_MAIN(p_video_info->data_pull.video_pack[0].phy_addr), p_video_info->data_pull.video_pack[0].size);
				memcpy(p_video_info->pps, (void *)PHY2VIRT_MAIN(p_video_info->data_pull.video_pack[1].phy_addr), p_video_info->data_pull.video_pack[1].size);
			} else {
				memcpy(p_video_info->sps, (void *)PHY2VIRT_SUB(p_video_info->data_pull.video_pack[0].phy_addr), p_video_info->data_pull.video_pack[0].size);
				memcpy(p_video_info->pps, (void *)PHY2VIRT_SUB(p_video_info->data_pull.video_pack[1].phy_addr), p_video_info->data_pull.video_pack[1].size);
			}

			p_video_info->sps_size = p_video_info->data_pull.video_pack[0].size;
			p_video_info->pps_size = p_video_info->data_pull.video_pack[1].size;
			hd_videoenc_release_out_buf(p_video_info->p_stream->enc_path, &p_video_info->data_pull);
			break;
		} else if (p_video_info->data_pull.vcodec_format == HD_CODEC_TYPE_H265) {
			p_video_info->codec_type = NVTLIVE555_CODEC_H265;
			if (p_video_info->data_pull.pack_num != 4) {
				hd_videoenc_release_out_buf(p_video_info->p_stream->enc_path, &p_video_info->data_pull);
				continue;
			}

			if (id == 0) {
				memcpy(p_video_info->vps, (void *)PHY2VIRT_MAIN(p_video_info->data_pull.video_pack[0].phy_addr), p_video_info->data_pull.video_pack[0].size);
				memcpy(p_video_info->sps, (void *)PHY2VIRT_MAIN(p_video_info->data_pull.video_pack[1].phy_addr), p_video_info->data_pull.video_pack[1].size);
				memcpy(p_video_info->pps, (void *)PHY2VIRT_MAIN(p_video_info->data_pull.video_pack[2].phy_addr), p_video_info->data_pull.video_pack[2].size);
			} else {
				memcpy(p_video_info->vps, (void *)PHY2VIRT_SUB(p_video_info->data_pull.video_pack[0].phy_addr), p_video_info->data_pull.video_pack[0].size);
				memcpy(p_video_info->sps, (void *)PHY2VIRT_SUB(p_video_info->data_pull.video_pack[1].phy_addr), p_video_info->data_pull.video_pack[1].size);
				memcpy(p_video_info->pps, (void *)PHY2VIRT_SUB(p_video_info->data_pull.video_pack[2].phy_addr), p_video_info->data_pull.video_pack[2].size);
			}

			p_video_info->vps_size = p_video_info->data_pull.video_pack[0].size;
			p_video_info->sps_size = p_video_info->data_pull.video_pack[1].size;
			p_video_info->pps_size = p_video_info->data_pull.video_pack[2].size;
			hd_videoenc_release_out_buf(p_video_info->p_stream->enc_path, &p_video_info->data_pull);
			break;
		} else {
			hd_videoenc_release_out_buf(p_video_info->p_stream->enc_path, &p_video_info->data_pull);
		}
	}
	return 0;
}

static int on_get_video_info(int handle, int timeout_ms, NVTLIVE555_VIDEO_INFO *p_info)
{
	VIDEO_INFO *p_video_info = (VIDEO_INFO *)handle;

	p_info->codec_type = p_video_info->codec_type;
	if (p_video_info->vps_size) {
		p_info->vps_size = p_video_info->vps_size;
		memcpy(p_info->vps, p_video_info->vps, p_info->vps_size);
	}

	if (p_video_info->sps_size) {
		p_info->sps_size = p_video_info->sps_size;
		memcpy(p_info->sps, p_video_info->sps, p_info->sps_size);
	}

	if (p_video_info->pps_size) {
		p_info->pps_size = p_video_info->pps_size;
		memcpy(p_info->pps, p_video_info->pps, p_info->pps_size);
	}

	return  0;
}

static int on_lock_video(int handle, int timeout_ms, NVTLIVE555_STRM_INFO *p_strm)
{
	VIDEO_INFO *p_video_info = (VIDEO_INFO *)handle;
	int id = (p_video_info == &video_info[0])? 0: 1;

	//while to get vsp, sps, pps
	int ret = hd_videoenc_pull_out_buf(p_video_info->p_stream->enc_path, &p_video_info->data_pull, -1);
	if (ret != 0) {
		return ret;
	}

	if (id == 0) {
		p_strm->addr = PHY2VIRT_MAIN(p_video_info->data_pull.video_pack[p_video_info->data_pull.pack_num-1].phy_addr);
	} else {
		p_strm->addr = PHY2VIRT_SUB(p_video_info->data_pull.video_pack[p_video_info->data_pull.pack_num-1].phy_addr);
	}
	p_strm->size = p_video_info->data_pull.video_pack[p_video_info->data_pull.pack_num-1].size;
	p_strm->timestamp = p_video_info->data_pull.timestamp;

	return 0;
}

static int on_unlock_video(int handle)
{
	VIDEO_INFO *p_video_info = (VIDEO_INFO *)handle;
	int ret = hd_videoenc_release_out_buf(p_video_info->p_stream->enc_path, &p_video_info->data_pull);
	return ret;
}


static int on_open_audio(int channel)
{
	return 0;
}

static int on_close_audio(int handle)
{
	if (handle) {
		AUDIO_INFO *p_info = (AUDIO_INFO *)handle;
		p_info->ref_cnt--;
	}
	return 0;
}

static int on_get_audio_info(int handle, int timeout_ms, NVTLIVE555_AUDIO_INFO *p_info)
{
	return 0;
}

static int on_lock_audio(int handle, int timeout_ms, NVTLIVE555_STRM_INFO *p_strm)
{
	return 0;
}

static int on_unlock_audio(int handle)
{
	return 0;
}

static void *encode_thread(void *arg)
{
	VIDEO_INFO *p_video_info = (VIDEO_INFO *)arg;
	VIDEO_RECORD* p_stream0 = p_video_info[0].p_stream;
	VIDEO_RECORD* p_stream1 = p_video_info[1].p_stream;

	//------ wait flow_start ------
	if (sensor_sel_1 != SEN_SEL_NONE) {
		while (p_stream0->flow_start == 0) sleep(1);
	} else {
		while (p_stream1->flow_start == 0) sleep(1);
	}

	// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
	if (sensor_sel_1 != SEN_SEL_NONE) {
		hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);
	}
	if (msf_mode == 0) {
		if (sensor_sel_2 != SEN_SEL_NONE) {
			hd_videoenc_get(p_stream1->enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_sub);
		}
	}

	// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
	if (sensor_sel_1 != SEN_SEL_NONE) {
		vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);
	}
	if (msf_mode == 0) {
		if (sensor_sel_2 != SEN_SEL_NONE) {
			vir_addr_sub  = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_sub.buf_info.phy_addr, phy_buf_sub.buf_info.buf_size);
		}
	}

	if (sensor_sel_1 != SEN_SEL_NONE) {
		refresh_video_info(0, &p_video_info[0]);
	}
	if (msf_mode == 0) {
		if (sensor_sel_2 != SEN_SEL_NONE) {
			refresh_video_info(1, &p_video_info[1]);
		}
	}

	//live555 setup
	NVTLIVE555_HDAL_CB hdal_cb;
	hdal_cb.open_video = on_open_video;
	hdal_cb.close_video = on_close_video;
	hdal_cb.get_video_info = on_get_video_info;
	hdal_cb.lock_video = on_lock_video;
	hdal_cb.unlock_video = on_unlock_video;
	hdal_cb.open_audio = on_open_audio;
	hdal_cb.close_audio = on_close_audio;
	hdal_cb.get_audio_info = on_get_audio_info;
	hdal_cb.lock_audio = on_lock_audio;
	hdal_cb.unlock_audio = on_unlock_audio;
	nvtrtspd_init(&hdal_cb);
	nvtrtspd_open();

	return 0;
}
#if 0
#endif
/*****************************************************************************************************************/
typedef void *MSF_THREAD_FP(void *arg);

typedef struct {
	HD_VIDEO_FRAME frame;
	struct vos_list_head list;
} MSF_FRAME_LIST;

typedef struct {
	//common information
	char name[10];
	UINT32 thread_start;
	UINT32 thread_exit;
	pthread_t thread_id;
	ID ctl_sem;

	VIDEO_RECORD *vid_rec;

	//only for msf pull image thread
	UINT32 dewarp_flag;
	struct vos_list_head frame_list_root;
	UINT32 frame_count;

	//only for msf proc thread
	UINT32 msf_hdl;
	MSF_IQ_PARAM iq_param;
	HD_COMMON_MEM_VB_BLK msf_buf_blk;
	UINT32 msf_buf_size;
	UINT32 msf_buf_pa;
	void* msf_buf_va;
} MSF_STREAM_INFO;

typedef struct {
	MSF_STREAM_INFO yuv;
	MSF_STREAM_INFO ir;
	MSF_STREAM_INFO msf;
} MSF_INFO;

static MSF_INFO gmsf_info = {0};

static void msf_put_frame_list(MSF_STREAM_INFO *pstream, MSF_FRAME_LIST *pframe_list)
{
	vos_sem_wait(pstream->ctl_sem);
	pstream->frame_count += 1;
	vos_list_add_tail(&pframe_list->list, &pstream->frame_list_root);
	vos_sem_sig(pstream->ctl_sem);
}

static MSF_FRAME_LIST* msf_get_frame_list(MSF_STREAM_INFO *pstream)
{
	MSF_FRAME_LIST *pframe_list;

	vos_sem_wait(pstream->ctl_sem);
	pframe_list = vos_list_first_entry_or_null(&pstream->frame_list_root, MSF_FRAME_LIST, list);
	if (pframe_list) {
		pstream->frame_count -= 1;
		vos_list_del(&pframe_list->list);
	}
	vos_sem_sig(pstream->ctl_sem);
	return pframe_list;
}

static HD_RESULT msf_flush_frame(MSF_STREAM_INFO *pstream, MSF_FRAME_LIST *pframe_list)
{
	UINT32 idx;
	HD_RESULT ret;

	idx = pstream->dewarp_flag;
	ret = hd_videoproc_release_out_buf(pstream->vid_rec->proc_path[idx], &pframe_list->frame);
	if (ret != HD_OK) {
		printf("%s: frame flush error=%d !!\r\n\r\n", pstream->name, ret);
	}
	free(pframe_list);
	return ret;
}

static UINT64 msf_calc_frame_interval(MSF_FRAME_LIST *pfrm0, MSF_FRAME_LIST *pfrm1)
{
	UINT64 diff[2], min_diff;

	diff[0] = (UINT64)((INT64)pfrm0->frame.timestamp - (INT64)pfrm1->frame.timestamp);
	diff[1] = (UINT64)((INT64)pfrm1->frame.timestamp - (INT64)pfrm0->frame.timestamp);

	if (diff[0] < diff[1]) {
		min_diff = diff[0];
	} else {
		min_diff = diff[1];
	}
	return min_diff;
}

static int msf_do_frame_sync(MSF_STREAM_INFO *pstream_yuv, MSF_STREAM_INFO *pstream_ir)
{
	int rt;
	UINT64 min_diff[2];
	MSF_FRAME_LIST *pyuv_list[2];
	MSF_FRAME_LIST *pir_list[2];

	vos_sem_wait(pstream_yuv->ctl_sem);
	vos_sem_wait(pstream_ir->ctl_sem);

	rt = -1;
	if ((pstream_yuv->frame_count >= 2) && (pstream_ir->frame_count >= 2)) {
		pyuv_list[0] = vos_list_first_entry(&pstream_yuv->frame_list_root, MSF_FRAME_LIST, list);
		pyuv_list[1] = vos_list_next_entry(pyuv_list[0], list);

		pir_list[0] = vos_list_first_entry(&pstream_ir->frame_list_root, MSF_FRAME_LIST, list);
		pir_list[1] = vos_list_next_entry(pir_list[0], list);

		if (pyuv_list[1]->frame.timestamp > pir_list[1]->frame.timestamp) {
			//2 ir + 1 yuv
			//calculate ir[0] & yuv[0] diff
			min_diff[0] = msf_calc_frame_interval(pir_list[0], pyuv_list[0]);

			//calculate ir[1] & yuv[0] diff
			min_diff[1] = msf_calc_frame_interval(pir_list[1], pyuv_list[0]);

			if (min_diff[0] < min_diff[1]) {
				rt = 1;
				//printf("yuv(%lld, %lld) ir(%lld, %lld) (ir0-y0, %lld) < (ir1-y0, %lld) pass\r\n",
				//			pyuv_list[0]->frame.timestamp, pyuv_list[0]->frame.count,
				//			pir_list[0]->frame.timestamp, pir_list[0]->frame.count,
				//			min_diff[0], min_diff[1]);
			} else {
				printf("yuv0(%lld, %lld) ir0(%lld, %lld) yuv1(%lld, %lld) ir1(%lld, %lld) (ir0-y0, %lld) > (ir1-y0, %lld) drop\r\n",
							pyuv_list[0]->frame.timestamp, pyuv_list[0]->frame.count,
							pir_list[0]->frame.timestamp, pir_list[0]->frame.count,
							pyuv_list[1]->frame.timestamp, pyuv_list[1]->frame.count,
							pir_list[1]->frame.timestamp, pir_list[1]->frame.count,
							min_diff[0], min_diff[1]);

				pstream_ir->frame_count -= 1;
				vos_list_del(&pir_list[0]->list);
				msf_flush_frame(pstream_ir, pir_list[0]);
			}
		} else {
			//1 ir + 2 yuv
			//calculate ir[0] & yuv[0] diff
			min_diff[0] = msf_calc_frame_interval(pir_list[0], pyuv_list[0]);

			//calculate ir[0] & yuv[1] diff
			min_diff[1] = msf_calc_frame_interval(pir_list[0], pyuv_list[1]);

			if (min_diff[0] < min_diff[1]) {
				rt = 1;
				//printf("yuv(%lld, %lld) ir(%lld, %lld) (ir0-y0, %lld) < (ir0-y1, %lld) pass\r\n",
				//			pyuv_list[0]->frame.timestamp, pyuv_list[0]->frame.count,
				//			pir_list[0]->frame.timestamp, pir_list[0]->frame.count,
				//			min_diff[0], min_diff[1]);
			} else {
				printf("yuv0(%lld, %lld) ir0(%lld, %lld) yuv1(%lld, %lld) ir1(%lld, %lld) (ir0-y0, %lld) > (ir0-y1, %lld) drop\r\n",
							pyuv_list[0]->frame.timestamp, pyuv_list[0]->frame.count,
							pir_list[0]->frame.timestamp, pir_list[0]->frame.count,
							pyuv_list[1]->frame.timestamp, pyuv_list[1]->frame.count,
							pir_list[1]->frame.timestamp, pir_list[1]->frame.count,
							min_diff[0], min_diff[1]);

				pstream_yuv->frame_count -= 1;
				vos_list_del(&pyuv_list[0]->list);
				msf_flush_frame(pstream_yuv, pyuv_list[0]);
			}
		}
	}

	vos_sem_sig(pstream_ir->ctl_sem);
	vos_sem_sig(pstream_yuv->ctl_sem);
	return rt;
}


#if 0
#endif
static void *msf_pull_frame_thread(void *arg)
{
	UINT32 idx;
	HD_RESULT ret = HD_OK;
	MSF_STREAM_INFO *pstream = (MSF_STREAM_INFO *)arg;
	VIDEO_RECORD *pvidrec = (VIDEO_RECORD *)pstream->vid_rec;

	MSF_FRAME_LIST *pframe_list;

	//init param
	idx = pstream->dewarp_flag;
	VOS_INIT_LIST_HEAD(&pstream->frame_list_root);

	//printf("%s: thread enter\r\n", p_stream->proc_name);
	//wait flow trigger
	while (pstream->thread_start == 0) sleep(1);

	printf("%s: start\r\n", pstream->name);
	while (pstream->thread_exit == 0) {

		//alloc & init frame list
		pframe_list = malloc(sizeof(MSF_FRAME_LIST));
		if (pframe_list == 0) {
			goto proc_rel_none;
		}

		VOS_INIT_LIST_HEAD(&pframe_list->list);

		//get frame information
		//proc_wait_ms: -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
		ret = hd_videoproc_pull_out_buf(pvidrec->proc_path[idx], &pframe_list->frame, -1);
		if (ret != HD_OK) {
			free(pframe_list);
		    printf("%s: pull_out(%d) error = %d!!\r\n", pstream->name, (-1), ret);
		} else {
			//printf("%s: frame%lld %lld (%d %d 0x%.8x)\r\n", pstream->name,
			//			pframe_list->frame.count, pframe_list->frame.timestamp,
			//			pframe_list->frame.pw[0], pframe_list->frame.ph[0], pframe_list->frame.pxlfmt);
			msf_put_frame_list(pstream, pframe_list);

		}
proc_rel_none:
		usleep(1000); //delay 1 ms
	}

	//free all memory
	while(1) {
		pframe_list = msf_get_frame_list(pstream);
		if (pframe_list) {
			msf_flush_frame(pstream, pframe_list);
		} else {
			break;
		}
	}
    printf("%s: thread exit\r\n",  pstream->name);

	return 0;
}

#if 0
#endif
static void msf_proc_unprepare(MSF_STREAM_INFO *pstream)
{
	msf_close(pstream->msf_hdl);
	msf_uninit();

	//release msf_buf_blk
	if (pstream->msf_buf_blk != HD_COMMON_MEM_VB_INVALID_BLK) {
		if (pstream->msf_buf_va) {
			hd_common_mem_munmap((void *)pstream->msf_buf_va, pstream->msf_buf_size);
		}
		hd_common_mem_release_block(pstream->msf_buf_blk);
	}
}

static int msf_proc_prepare(MSF_STREAM_INFO *pstream)
{
	HD_RESULT rt;
	MSF_BUF msf_buf;

	rt = msf_init();
	if (rt != HD_OK) {
		printf("msf_init fail\r\n");
		goto msf_prepare_end;
	}

	//get msf buffer
	pstream->msf_buf_size = msf_get_work_buf_szie(vdo_size_w_1, vdo_size_h_1);
	pstream->msf_buf_blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, pstream->msf_buf_size, DDR_ID0); // Get block from mem pool
	if (pstream->msf_buf_blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get msf_buf_blk fail\r\n");
		goto msf_prepare_end;
	}

    pstream->msf_buf_pa = hd_common_mem_blk2pa(pstream->msf_buf_blk); // Get physical addr
	if (pstream->msf_buf_pa == 0) {
		printf("get msf_buf_pa fail\r\n");
		goto msf_prepare_end;
	}

	pstream->msf_buf_va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pstream->msf_buf_pa, pstream->msf_buf_size);
	if (pstream->msf_buf_va == 0) {
		printf("get msf_buf_va fail\r\n");
		goto msf_prepare_end;
	}

	msf_buf.ddr_id = DDR_ID0;
	msf_buf.pa_addr = pstream->msf_buf_pa;
	msf_buf.va_addr = pstream->msf_buf_va;
	msf_buf.size = pstream->msf_buf_size;
	printf("msf_buf pa = 0x%lx, va = 0x%lx size = %d\r\n", msf_buf.pa_addr, msf_buf.va_addr, msf_buf.size);

	pstream->msf_hdl = msf_open(msf_buf);
	printf("msf_hdl = %lx\r\n", pstream->msf_hdl);

	pstream->iq_param.curve_th0 = 40;
	pstream->iq_param.curve_th1 = 80;
	pstream->iq_param.curve_val0 = 255;
	pstream->iq_param.curve_val1 = 64;
	return 0;

msf_prepare_end:
	msf_proc_unprepare(pstream);
	return -1;
}

static void *msf_proc_thread(void *arg)
{
	MSF_INFO *pmsf_info = (MSF_INFO *)arg;

	MSF_STREAM_INFO *pstream_msf = (MSF_STREAM_INFO *)&pmsf_info->msf;

	MSF_STREAM_INFO *pstream_yuv = (MSF_STREAM_INFO *)&pmsf_info->yuv;
	VIDEO_RECORD *pvidrec_yuv = (VIDEO_RECORD *)pstream_yuv->vid_rec;
	MSF_FRAME_LIST *pframe_list_yuv;

	MSF_STREAM_INFO *pstream_ir = (MSF_STREAM_INFO *)&pmsf_info->ir;
	MSF_FRAME_LIST *pframe_list_ir;

	int rt;
	HD_RESULT ret;
	MSF_IQ_PARAM iq_param;
	//VOS_TICK t[2];

	//printf("%s: thread enter\r\n", pmsf->proc_name);

	rt = msf_proc_prepare(pstream_msf);
	if (rt < 0) {
		printf("%s: msf_prepare fail\r\n", pstream_msf->name);
		return 0;
	}

	//wait flow trigger
	while (pstream_msf->thread_start == 0) sleep(1);

	printf("%s: start\r\n", pstream_msf->name);
	while (pstream_msf->thread_exit == 0) {

		rt = msf_do_frame_sync(pstream_yuv, pstream_ir);
		if (rt == 1) {

			pframe_list_yuv = msf_get_frame_list(pstream_yuv);//normal image
			pframe_list_ir = msf_get_frame_list(pstream_ir);//ir image

			vos_sem_wait(pstream_msf->ctl_sem);
			iq_param = pstream_msf->iq_param;
			vos_sem_sig(pstream_msf->ctl_sem);
			//vos_perf_mark(&t[0]);
			msf_proc(pstream_msf->msf_hdl, &pframe_list_yuv->frame, &pframe_list_ir->frame, &iq_param);
			//vos_perf_mark(&t[1]);
			//printf("msf_proc time = %d %d\r\n", (unsigned int)(t[1] - t[0]));

			ret = hd_videoenc_push_in_buf(pvidrec_yuv->enc_path, &pframe_list_yuv->frame, NULL, 0); // only support non-blocking mode now
			if (ret != HD_OK) {
				if (ret != HD_ERR_OVERRUN) {
					printf("%s_%s: enc_push error=%d !!\r\n\r\n", pstream_msf->name, pstream_yuv->name, ret);
				}
			}
			msf_flush_frame(pstream_yuv, pframe_list_yuv);
			msf_flush_frame(pstream_ir, pframe_list_ir);
		}
		usleep(1000); //delay 1 ms
	}
	msf_proc_unprepare(pstream_msf);
    printf("%s: thread exit\r\n",  pstream_msf->name);
	return 0;
}
#if 0
#endif
static int msf_proc_init(MSF_STREAM_INFO *pstream, char *pname, UINT32 dewarp_flag, VIDEO_RECORD *pvidrec, MSF_THREAD_FP fp, void* pthread_arg)
{
	int ret;
	struct sched_param param;

	//reset all parameter
	memset(pstream, 0, sizeof(MSF_STREAM_INFO));

	//init common information
	pstream->vid_rec = pvidrec;
	snprintf(&pstream->name[0], 10, pname);
	pstream->thread_start = 1;
	pstream->thread_exit = 0;
	vos_sem_create(&pstream->ctl_sem, 1, pname);

	pstream->dewarp_flag = dewarp_flag;

	//create thread
	ret = pthread_create(&pstream->thread_id, NULL, fp, pthread_arg);
	//pthread_setname_np(pstream->thread_id, &pstream->name[0]);
	param.sched_priority = 80;
	pthread_setschedparam(pstream->thread_id, SCHED_RR, &param);
	return ret;
}

static void msf_proc_exit(MSF_STREAM_INFO *pstream)
{
	pstream->thread_exit = 1;
	pthread_join(pstream->thread_id, (void* )NULL);
}

static void msf_proc_uninit(MSF_STREAM_INFO *pstream)
{
	vos_sem_destroy(pstream->ctl_sem);
}

#define MSF_MENU_ID_RETURN 0xFF
#define MSF_MENU_ID_LAST (-1)

typedef struct _MSF_MENU {
	int            menu_id;          ///< user command value
	const char     *p_name;          ///< command string
	int      	  (*p_func)(int argc, int *args);  ///< command function
	BOOL           b_enable;         ///< execution option
} MSF_MENU;

static int msf_menu_get_ver(int argc, int *args)
{
	CHAR *ver;
	ver = msf_get_version();
	printf("msf ver: %s\r\n", ver);
	return 0;
}

static int msf_menu_set_dbg_lvl(int argc, int *args)
{
	if (argc >= 1) {
		msf_set_dbg_lvl((MSF_DBG_LVL)args[0]);
		return 0;
	}
	return -1;
}

static int msf_menu_adj_curve(int argc, int *args)
{
	if (argc >= 4) {
		vos_sem_wait(gmsf_info.msf.ctl_sem);
		gmsf_info.msf.iq_param.curve_th0 = args[0];
		gmsf_info.msf.iq_param.curve_th1 = args[1];
		gmsf_info.msf.iq_param.curve_val0 = args[2];
		gmsf_info.msf.iq_param.curve_val1 = args[3];
		vos_sem_sig(gmsf_info.msf.ctl_sem);
		return 0;
	}
	return -1;
}

static int msf_menu_dump_info(int argc, int *args)
{
	msf_dump_info();
	return 0;
}

static int msf_menu_set_dbg_mode(int argc, int *args)
{
	if (argc >= 1) {
		msf_set_dbg_mode(gmsf_info.msf.msf_hdl, args[0]);
		return 0;
	}
	return -1;
}


static MSF_MENU msf_menu[] = {
	{0x01, "msf get ver", msf_menu_get_ver, TRUE},
	{0x02, "msf set dbg lvl(lvl)", msf_menu_set_dbg_lvl, TRUE},
	{0x03, "msf adj curve(th0 th1 w0 w1)", msf_menu_adj_curve, TRUE},
	{0x04, "msf dump info", msf_menu_dump_info, TRUE},
	{0x05, "msf set dbg mode(mode)", msf_menu_set_dbg_mode, TRUE},
	// escape muse be last
	{-1, "", NULL, FALSE},
};

static void msf_menu_print(MSF_MENU *p_menu, const char *p_title)
{
	printf("\n==============================");
	printf("\n %s", p_title);
	printf("\n------------------------------");

	while (p_menu->menu_id != MSF_MENU_ID_LAST) {
		if (p_menu->b_enable) {
			printf("\n %02d : %s", p_menu->menu_id, p_menu->p_name);
		}
		p_menu++;
	}

	printf("\n------------------------------");
	printf("\n %02d : %s", MSF_MENU_ID_RETURN, "Return");
	printf("\n------------------------------\n");
}

static void msf_get_str_input(char *buf, int size)
{
	char *ret;

	fflush(stdin);
	do {
		ret = fgets(buf, size, stdin);
	} while (NULL == ret || buf[0] == ' ' || buf[0] == '\n');
}

static int msf_get_arg(char *buf, int *args, int args_max)
{
	unsigned long val;
	int radix, argc;
	char *substr;
	char *tmp;

	argc = 0;

	while (1) {
		if (argc == 0) {
			substr = strtok_r(buf, " ", &tmp);
		} else {
			substr = strtok_r(NULL, " ", &tmp);
		}

		if (substr == NULL) {
			break;
		}

		//convert char to int
		if (!strncmp(substr, "0x", 2)) {
			radix = 16;
		}
		else {
			radix = 10;
		}
		val = strtoul(substr, (char **)NULL, radix);
		args[argc] = (int)val;
		argc += 1;

		if (argc >= args_max) {
			break;
		}
	}
	return argc;
}

static int msf_menu_exec(int menu_id, MSF_MENU *p_menu, int argc, int *args)
{
	if (menu_id == MSF_MENU_ID_RETURN) {
		return 0; // return 0 for return upper menu
	}

	while (p_menu->menu_id != MSF_MENU_ID_LAST) {
		if (p_menu->menu_id == menu_id && p_menu->b_enable) {
			printf("Run: %02d : %s\r\n", p_menu->menu_id, p_menu->p_name);
			if (p_menu->p_func) {
				return p_menu->p_func(argc, args);
			} else {
				printf("null function for menu id = %d\n", menu_id);
				return 0; // just skip
			}
		}
		p_menu++;
	}

	printf("cannot find menu id = %d\n", menu_id);
	return 0;
}

static int msf_menu_entry(MSF_MENU *p_menu, const char *p_title)
{
	char cmd[80];
	char msg[128];
	int argc, args[10], idx, i;

	do {

		msf_menu_print(p_menu, p_title);
		msf_get_str_input(cmd, sizeof(cmd));
		memset(args, -1, sizeof(args));
		argc = msf_get_arg(cmd, args, sizeof(args));

		idx = 0;
		idx += snprintf(&msg[idx], (128 - idx), "argc:%d cmd:", argc);
		for (i = 0; i < argc; i ++) {
			idx += snprintf(&msg[idx], (128 - idx), "%d ", args[i]);
		}
		printf("%s\n", msg);
		msf_menu_exec(args[0], p_menu, (argc - 1), &args[1]);
	} while(args[0] != MSF_MENU_ID_RETURN);

	return 0;
}

/*****************************************************************************************************************/
#if 0
#endif
MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	VIDEO_RECORD stream[2] = {0}; //0: stream0, 1: stream1
	UINT32 enc_type = 0;
	UINT32 enc_bitrate = 4;
	UINT32 depth;
	HD_DIM main_dim;
	AET_CFG_INFO cfg_info = {0};
	AWBT_CT_TO_CGAIN ct_to_cgain = {0};
	ISPT_C_GAIN c_gain = {0};
	ISPT_CT ct = {0};
	char *ir_file_name = 0;
	char *yuv_file_name = 0;


	if (argc == 1 || argc > 12) {
		printf("\033[1;37;43mUsage:\033[0m \033[1;32;40m<sensor_sel_1 > \033[1;31;40m<sensor_shdr_1> \033[1;32;40m<sensor_sel_2 > \033[1;31;40m<sensor_shdr_2> \033[1;37;40m<enc_type> <enc_bitrate> <msf_mode> <ir_dewarp_mode> <yuv_dewarp_mode>\033[0m.\r\n");
		printf("\r\n");
		printf("\033[1;32;40m  <sensor_sel_1 > :  0(NONE),   1(imx290), 2(os02k10), 3(ar0237ir), 4(ov2718),  5(os05a10)              \033[0m\r\n");
		printf("\033[1;32;40m                     6(imx317), 7(imx335), 8(f37),     9(ps5268),  10(sc4210), 11(imx307_slave)         \033[0m\r\n");
		printf("\033[1;32;40m                    12(f35),   13(imx415),14(sc500ai),15(sc401ai),16(os04a10), 17(gc4653), 18(os05a10_dual)               \033[0m\r\n");
		printf("\033[1;32;40m                    99(patgen)                                                                          \033[0m\r\n");
		printf("\033[1;31;40m  <sensor_shdr_1> : 0(disable), 1(enable)                                                  \033[0m\r\n");
		printf("\033[1;32;40m  <sensor_sel_2 > : as <sensor_sel_1 >                                                     \033[0m\r\n");
		printf("\033[1;31;40m  <sensor_shdr_2> : 0(disable), 1(enable)                                                  \033[0m\r\n");
		printf("\033[1;37;40m  <enc_type     > : 0(H265), 1(H264), 2(MJPG)                                              \033[0m\r\n");
		printf("\033[1;37;40m  <enc_bitrate  > : Mbps for H265/H264, or Quality for MJPG                                \033[0m\r\n");
		printf("\033[1;31;40m  <msf_mode  > : 0(disable), 1(enable)                                                  \033[0m\r\n");
		printf("\033[1;31;40m  <ir_dewarp_mode> : 0(disable), 1(enable)	                                        		\033[0m\r\n");
		printf("\033[1;31;40m  <yuv_dewarp_mode> : 0(disable), 1(enable)	                                            \033[0m\r\n");
		printf("\033[1;31;40m  <ir 2dlut table> : bin file					                                        	\033[0m\r\n");
		printf("\033[1;31;40m  <yuv 2dlut table> : bin file					                                        	\033[0m\r\n");
		return 0;
	}

	// query program options
	if (argc > 1) {
		sensor_sel_1 = atoi(argv[1]);
		switch (sensor_sel_1) {
			default:
			case SEN_SEL_NONE:
				printf("sensor 1: none \r\n");
			break;
			case SEN_SEL_IMX290:
				printf("sensor 1: imx290 \r\n");
				break;
			case SEN_SEL_OS02K10:
				printf("sensor 1: os02k10 \r\n");
				break;
			case SEL_SEL_AR0237IR:
				printf("sensor 1: ar0237ir \r\n");
				break;
			case SEN_SEL_OV2718:
				printf("sensor 1: ov2718 \r\n");
				break;
			case SEN_SEL_OS05A10:
				printf("sensor 1: os05a10 \r\n");
				break;
			case SEN_SEL_IMX317:
				printf("sensor 1: imx317 \r\n");
				break;
			case SEN_SEL_IMX335:
				printf("sensor 1: imx335 \r\n");
				break;
			case SEN_SEL_F37:
				printf("sensor 1: f37 \r\n");
				break;
			case SEN_SEL_PS5268:
				printf("sensor 1: ps5268 \r\n");
				break;
			case SEN_SEL_SC4210:
				printf("sensor 1: sc4210 \r\n");
				break;
			case SEN_SEL_IMX307_SLAVE:
				printf("sensor 1: imx307 slave \r\n");
				break;
			case SEN_SEL_F35:
				printf("sensor 1: f35 \r\n");
				break;
			case SEN_SEL_IMX415:
				printf("sensor 1: imx415 \r\n");
				break;
			case SEN_SEL_SC500AI:
				printf("sensor 1: sc500ai \r\n");
				break;
			case SEN_SEL_SC401AI:
				printf("sensor 1: sc401ai \r\n");
				break;
			case SEN_SEL_OS04A10:
				printf("sensor 1: os04a10 \r\n");
				break;
			case SEN_SEL_GC4653:
				printf("sensor 1: gc4653 \r\n");
				break;
			case SEN_SEL_OS05A10_DUAL:
				printf("sensor 1: os05a10 dual \r\n");
				break;
			case SEN_SEL_PATGEN:
				printf("sensor 1: patgen \r\n");
				break;
		}
	}

	if (argc > 2) {
		sensor_shdr_1 = atoi(argv[2]);
		if (sensor_shdr_1 == 0) {
			printf("sensor 1: shdr off \r\n");
		} else if (sensor_shdr_1 == 1) {
			if ((sensor_sel_1 == SEL_SEL_AR0237IR) || (sensor_sel_1 == SEN_SEL_OV2718)) {
				sensor_shdr_1 = 0;
				printf("sensor 1: shdr off, sensor not support. \r\n");
			} else {
				printf("sensor 1: shdr on \r\n");
			}
		} else {
			printf("error: not support shdr_mode! (%d) \r\n", sensor_shdr_1);
			return 0;
		}
	}

	if (argc > 3) {
		if (sensor_sel_1 == SEN_SEL_PATGEN) {
			if (atoi(argv[3]) > 0) {
				patgen_size_w = atoi(argv[3]);
			}
			sensor_sel_2 = SEN_SEL_NONE;
			printf("sensor 2: none, patgen w = %d \r\n", patgen_size_w);
		} else {
			sensor_sel_2 = atoi(argv[3]);
			switch (sensor_sel_2) {
				default:
				case SEN_SEL_NONE:
					printf("sensor 2: none \r\n");
				break;
				case SEN_SEL_IMX290:
					printf("sensor 2: imx290 \r\n");
					break;
				case SEN_SEL_OS02K10:
					printf("sensor 2: os02k10 \r\n");
					break;
				case SEL_SEL_AR0237IR:
					printf("sensor 2: ar0237ir \r\n");
					break;
				case SEN_SEL_OV2718:
					printf("sensor 2: ov2718 \r\n");
					break;
				case SEN_SEL_OS05A10:
					printf("sensor 2: os05a10 \r\n");
					break;
				case SEN_SEL_IMX317:
					printf("sensor 2: imx317 \r\n");
					break;
				case SEN_SEL_IMX335:
					printf("sensor 2: imx335 \r\n");
					break;
				case SEN_SEL_F37:
					printf("sensor 2: f37 \r\n");
					break;
				case SEN_SEL_PS5268:
					printf("sensor 2: ps5268 \r\n");
					break;
				case SEN_SEL_SC4210:
					printf("sensor 2: sc4210 \r\n");
					break;
				case SEN_SEL_IMX307_SLAVE:
					printf("sensor 2: imx307 slave \r\n");
					break;
				case SEN_SEL_F35:
					printf("sensor 2: f35 \r\n");
					break;
				case SEN_SEL_IMX415:
					printf("sensor 2: imx415 \r\n");
					break;
				case SEN_SEL_SC500AI:
					printf("sensor 2: sc500ai \r\n");
					break;
				case SEN_SEL_SC401AI:
					printf("sensor 2: sc401ai \r\n");
					break;
				case SEN_SEL_OS04A10:
					printf("sensor 2: os04a10 \r\n");
					break;
				case SEN_SEL_GC4653:
					printf("sensor 2: gc4653 \r\n");
					break;
				case SEN_SEL_OS05A10_DUAL:
					printf("sensor 2: os05a10 dual \r\n");
					break;
			}
		}
	}

	if (argc > 4) {
		if (sensor_sel_1 == SEN_SEL_PATGEN) {
			if (atoi(argv[4]) > 0) {
				patgen_size_h = atoi(argv[4]);
			}
			sensor_shdr_2 = 0;
			printf("sensor 2: shdr off, patgen h = %d \r\n", patgen_size_h);
		} else {
			sensor_shdr_2 = atoi(argv[4]);
			if (sensor_shdr_2 == 0) {
				printf("sensor 2: shdr off \r\n");
			} else if (sensor_shdr_2 == 1) {
				if ((sensor_sel_2 == SEL_SEL_AR0237IR) || (sensor_sel_2 == SEN_SEL_OV2718)) {
					sensor_shdr_2 = 0;
					printf("sensor 2: shdr off, sensor not support. \r\n");
				} else {
					printf("sensor 2: shdr on \r\n");
				}
			} else {
				printf("error: not support shdr_mode! (%d) \r\n", sensor_shdr_2);
				return 0;
			}
		}
	}

	if (argc > 5) {
		enc_type = atoi(argv[5]);
		if (enc_type == 0) {
			printf("Enc type: H.265\r\n", enc_type);
		} else if (enc_type == 1) {
			printf("Enc type: H.264\r\n", enc_type);
		} else if (enc_type == 2) {
			printf("Enc type: MJPG\r\n", enc_type);
		} else {
			printf("error: not support enc_type! (%d)\r\n", enc_type);
			return 0;
		}
	}

	if (argc > 6) {
		enc_bitrate = atoi(argv[6]);
		printf("Enc bitrate: %d Mbps\r\n", enc_bitrate);
	}

	if (argc > 7) {
		msf_mode = atoi(argv[7]);
		printf("msf_mode: %d\r\n", msf_mode);
	}

	if (argc > 8) {
		ir_dewarp_mode = atoi(argv[8]);
		printf("ir_dewarp_mode: %d\r\n", ir_dewarp_mode);
	}

	if (argc > 9) {
		yuv_dewarp_mode = atoi(argv[9]);
		printf("yuv_dewarp_mode: %d\r\n", yuv_dewarp_mode);
	}

	if (argc > 10) {
		ir_file_name = argv[10];
		printf("ir_file_name: %s\r\n", ir_file_name);
	}

	if (argc > 11) {
		yuv_file_name = argv[11];
		printf("yuv_file_name: %s\r\n", yuv_file_name);
	}

	if ((sensor_sel_1 == SEN_SEL_NONE) && (sensor_sel_2 == SEN_SEL_NONE)) {
		printf("sensor_sel_1 & sensor_sel_1 = SEN_SEL_NONE \n");
		return 0;
	}

	{
		char *chip_name = getenv("NVT_CHIP_ID");
		if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51084") == 0) {
			printf("IS NT98528 \n");
			is_nt98528 = 1;
		} else {
			printf("NOT NT98528 \n");
			is_nt98528 = 0;
		}
	}

	if ((!is_nt98528) && (sensor_sel_2 != 0) && ((sensor_shdr_1 == 1) || (sensor_shdr_2 == 1))) {
		printf("sensor_sel_2 != 0, sensor_shdr force to 0 \n");
		sensor_shdr_1 = 0;
		sensor_shdr_2 = 0;
	}

	if ((data_mode1 == 1) & (sensor_sel_2 != SEN_SEL_NONE)) {
		printf("sensor_sel_2 != 0, disable direct mode \n");
		data_mode1 = 0;
	}

	if (vendor_isp_init() != HD_ERR_NG) {
		if (sensor_sel_1 != SEN_SEL_NONE) {
			cfg_info.id = 0;
			if ((sensor_sel_1 == SEN_SEL_IMX290) && (sensor_shdr_1 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_IMX290) && (sensor_shdr_1 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0_hdr.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_OS05A10) && (sensor_shdr_1 == 0) && (is_nt98528 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0_52x.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_OS05A10) && (sensor_shdr_1 == 0) && (is_nt98528 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0_528.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_OS05A10) && (sensor_shdr_1 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0_hdr.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_OS05A10_DUAL) && (sensor_shdr_1 == 0) && (is_nt98528 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0_52x.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_OS05A10_DUAL) && (sensor_shdr_1 == 0) && (is_nt98528 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0_528.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_OS05A10_DUAL) && (sensor_shdr_1 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0_hdr.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_OS02K10) && (sensor_shdr_1 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os02k10_0.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_OS02K10) && (sensor_shdr_1 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os02k10_0_hdr.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_1 == SEL_SEL_AR0237IR) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_ar0237ir_0.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_1 == SEN_SEL_OV2718) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_ov2718_0.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_1 == SEN_SEL_IMX317) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_IMX335) && (sensor_shdr_1 == 0) && (is_nt98528 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx335_0_52x.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_IMX335) && (sensor_shdr_1 == 1) && (is_nt98528 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx335_0_hdr_52x.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_IMX335) && (sensor_shdr_1 == 0) && (is_nt98528 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx335_0_528.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_IMX335) && (sensor_shdr_1 == 1) && (is_nt98528 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx335_0_hdr_528.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_1 == SEN_SEL_F37) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_f37.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_F35) && (sensor_shdr_1 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_f35.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_F35) && (sensor_shdr_1 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_f35_0_hdr.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_1 == SEN_SEL_PS5268) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_ps5268.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_SC4210) && (sensor_shdr_1 == 0) && (is_nt98528 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_sc4210_0_52x.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_SC4210) && (sensor_shdr_1 == 1) && (is_nt98528 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_sc4210_0_hdr_52x.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_SC4210) && (sensor_shdr_1 == 0) && (is_nt98528 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_sc4210_0_528.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_SC4210) && (sensor_shdr_1 == 1) && (is_nt98528 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_sc4210_0_hdr_528.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_IMX415) && (sensor_shdr_1 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx415_0.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_1 == SEN_SEL_IMX415) && (sensor_shdr_1 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx415_0_hdr.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_1 == SEN_SEL_SC500AI) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_sc401ai_0.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_1 == SEN_SEL_SC401AI) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_sc401ai_0.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_1 == SEN_SEL_OS04A10) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0_52x.cfg", CFG_NAME_LENGTH);
			} else {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0.cfg", CFG_NAME_LENGTH);
			}

			printf("sensor 1 load %s \n", cfg_info.path);

			vendor_isp_set_ae(AET_ITEM_RLD_CONFIG, &cfg_info);
			vendor_isp_set_awb(AWBT_ITEM_RLD_CONFIG, &cfg_info);
			vendor_isp_set_iq(IQT_ITEM_RLD_CONFIG, &cfg_info);

			ct_to_cgain.id = 0;
			ct_to_cgain.ct_to_cgain.ct = 4700;
			vendor_isp_get_awb(AWBT_ITEM_CT_TO_CGAIN, &ct_to_cgain);

			c_gain.id = 0;
			c_gain.gain[0] = ct_to_cgain.ct_to_cgain.r_gain;
			c_gain.gain[1] = ct_to_cgain.ct_to_cgain.g_gain;
			c_gain.gain[2] = ct_to_cgain.ct_to_cgain.b_gain;
			vendor_isp_set_common(ISPT_ITEM_C_GAIN, &c_gain);

			ct.id = 0;
			ct.ct = 4700;
			vendor_isp_set_common(ISPT_ITEM_CT, &ct);
		}

		if (sensor_sel_2 != SEN_SEL_NONE) {
			cfg_info.id = 1;
			if ((sensor_sel_2 == SEN_SEL_IMX290) && (sensor_shdr_1 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_2 == SEN_SEL_IMX290) && (sensor_shdr_2 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0_hdr.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_2 == SEN_SEL_OS05A10) && (sensor_shdr_1 == 0) && (is_nt98528 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0_52x.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_2 == SEN_SEL_OS05A10) && (sensor_shdr_1 == 0) && (is_nt98528 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0_528.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_2 == SEN_SEL_OS05A10_DUAL) && (sensor_shdr_1 == 0) && (is_nt98528 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0_52x.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_2 == SEN_SEL_OS05A10_DUAL) && (sensor_shdr_1 == 0) && (is_nt98528 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0_528.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_2 == SEN_SEL_OS02K10) && (sensor_shdr_2 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os02k10_0.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_2 == SEN_SEL_OS02K10) && (sensor_shdr_2 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os02k10_0_hdr.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_2 == SEL_SEL_AR0237IR) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_ar0237ir_0.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_2 == SEN_SEL_OV2718) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_ov2718_0.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_2 == SEN_SEL_IMX317) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_2 == SEN_SEL_IMX335 && (sensor_shdr_2 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx335_0.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_2 == SEN_SEL_IMX335) && (sensor_shdr_2 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0_hdr.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_2 == SEN_SEL_F37) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_f37.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_2 == SEN_SEL_F35) && (sensor_shdr_2 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_f35.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_2 == SEN_SEL_F35) && (sensor_shdr_2 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_f35_0_hdr.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_2 == SEN_SEL_PS5268) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_ps5268.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_2 == SEN_SEL_SC4210) && (sensor_shdr_1 == 0) && (is_nt98528 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_sc4210_0_52x.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_2 == SEN_SEL_SC4210) && (sensor_shdr_1 == 1) && (is_nt98528 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_sc4210_0_hdr_52x.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_2 == SEN_SEL_SC4210) && (sensor_shdr_1 == 0) && (is_nt98528 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_sc4210_0_528.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_2 == SEN_SEL_SC4210) && (sensor_shdr_1 == 1) && (is_nt98528 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_sc4210_0_hdr_528.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_2 == SEN_SEL_IMX415) && (sensor_shdr_2 == 0)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx415_0.cfg", CFG_NAME_LENGTH);
			} else if ((sensor_sel_2 == SEN_SEL_IMX415) && (sensor_shdr_2 == 1)) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx415_0_hdr.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_2 == SEN_SEL_SC500AI) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_sc401ai_0.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_2 == SEN_SEL_SC401AI) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_sc401ai_0.cfg", CFG_NAME_LENGTH);
			} else if (sensor_sel_2 == SEN_SEL_OS04A10) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0_52x.cfg", CFG_NAME_LENGTH);
			} else {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0.cfg", CFG_NAME_LENGTH);
			}

			printf("sensor 2 load %s \n", cfg_info.path);

			vendor_isp_set_ae(AET_ITEM_RLD_CONFIG, &cfg_info);
			vendor_isp_set_awb(AWBT_ITEM_RLD_CONFIG, &cfg_info);
			vendor_isp_set_iq(IQT_ITEM_RLD_CONFIG, &cfg_info);

			ct_to_cgain.id = 1;
			ct_to_cgain.ct_to_cgain.ct = 4700;
			vendor_isp_get_awb(AWBT_ITEM_CT_TO_CGAIN, &ct_to_cgain);

			c_gain.id = 1;
			c_gain.gain[0] = ct_to_cgain.ct_to_cgain.r_gain;
			c_gain.gain[1] = ct_to_cgain.ct_to_cgain.g_gain;
			c_gain.gain[2] = ct_to_cgain.ct_to_cgain.b_gain;
			vendor_isp_set_common(ISPT_ITEM_C_GAIN, &c_gain);

			ct.id = 1;
			ct.ct = 4700;
			vendor_isp_set_common(ISPT_ITEM_CT, &ct);
		}

		vendor_isp_uninit();
	} else {
		printf("vendor_isp_init failed!\r\n");
	}

	if ((sensor_sel_1 == SEN_SEL_OS05A10) || (sensor_sel_1 == SEN_SEL_IMX335) || (sensor_sel_1 == SEN_SEL_OS05A10_DUAL)){
		vdo_size_w_1 = VDO_SIZE_W_5M;
		vdo_size_h_1 = VDO_SIZE_H_5M;
	} else if (sensor_sel_1 == SEN_SEL_SC500AI) {
		vdo_size_w_1 = VDO_SIZE_W_5M_BIG;
		vdo_size_h_1 = VDO_SIZE_H_5M_BIG;
	} else if ((sensor_sel_1 == SEN_SEL_IMX317) || (sensor_sel_1 == SEN_SEL_IMX415)) {
		vdo_size_w_1 = VDO_SIZE_W_8M;
		vdo_size_h_1 = VDO_SIZE_H_8M;
	} else if ((sensor_sel_1 == SEN_SEL_SC4210) || (sensor_sel_1 == SEN_SEL_SC401AI) || (sensor_sel_1 == SEN_SEL_GC4653)) {
		vdo_size_w_1 = VDO_SIZE_W_4M;
		vdo_size_h_1 = VDO_SIZE_H_4M;
	} else if (sensor_sel_1 == SEN_SEL_OS04A10) {
		vdo_size_w_1 = VDO_SIZE_W_4M_BIG;
		vdo_size_h_1 = VDO_SIZE_H_4M_BIG;
	} else if (sensor_sel_1 == SEN_SEL_PATGEN) {
		vdo_size_w_1 = patgen_size_w;
		vdo_size_h_1 = patgen_size_h;
	} else {
		vdo_size_w_1 = VDO_SIZE_W_2M;
		vdo_size_h_1 = VDO_SIZE_H_2M;
	}

	if ((sensor_sel_2 == SEN_SEL_OS05A10) || (sensor_sel_2 == SEN_SEL_IMX335) || (sensor_sel_2 == SEN_SEL_OS05A10_DUAL)){
		vdo_size_w_2 = VDO_SIZE_W_5M;
		vdo_size_h_2 = VDO_SIZE_H_5M;
	} else if (sensor_sel_2 == SEN_SEL_SC500AI) {
		vdo_size_w_2 = VDO_SIZE_W_5M_BIG;
		vdo_size_h_2 = VDO_SIZE_H_5M_BIG;
	} else if ((sensor_sel_2 == SEN_SEL_IMX317) || (sensor_sel_2 == SEN_SEL_IMX415)) {
		vdo_size_w_2 = VDO_SIZE_W_8M;
		vdo_size_h_2 = VDO_SIZE_H_8M;
	} else if ((sensor_sel_2 == SEN_SEL_SC4210) || (sensor_sel_2 == SEN_SEL_SC401AI) || (sensor_sel_2 == SEN_SEL_GC4653)) {
		vdo_size_w_2 = VDO_SIZE_W_4M;
		vdo_size_h_2 = VDO_SIZE_H_4M;
	} else if (sensor_sel_2 == SEN_SEL_OS04A10) {
		vdo_size_w_2 = VDO_SIZE_W_4M_BIG;
		vdo_size_h_2 = VDO_SIZE_H_4M_BIG;
	} else {
		vdo_size_w_2 = VDO_SIZE_W_2M;
		vdo_size_h_2 = VDO_SIZE_H_2M;
	}

	if (msf_mode) {
		data_mode1 = 0;
		if ((sensor_sel_1 == SEN_SEL_NONE) || (sensor_sel_2 == SEN_SEL_NONE)) {
			msf_mode = 0;
			printf("error: sen1(%d) & sen2(%d) not support low light mode, force off\r\n", sensor_sel_1, sensor_sel_2);
			return 0;
		}

		if (ir_dewarp_mode) {
			printf("ir_2d_file_name=%s\r\n", ir_file_name);
		}

		if (yuv_dewarp_mode) {
			printf("yuv_2d_file_name=%s\r\n", yuv_file_name);
		}
	} else {
		ir_dewarp_mode = 0;
		yuv_dewarp_mode = 0;
	}

	// open vpe
	if (vendor_vpe_init() != HD_ERR_NG) {
		if (yuv_dewarp_mode) {
			set_vpe_cfg(0, yuv_file_name);
		}

		if (ir_dewarp_mode) {
			set_vpe_cfg(1, ir_file_name);
		}
		vendor_vpe_uninit();
	} else {
		printf("vendor_vpe_init failed!\r\n");
	}

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
	}

	hd_common_sysconfig(HD_VIDEOPROC_CFG, 0, HD_VIDEOPROC_CFG_STRIP_LV1, 0);

	// init memory
	ret = mem_init();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		goto exit;
	}

	// init all modules
	ret = init_module();
	if (ret != HD_OK) {
		printf("init module fail=%d\n", ret);
		goto exit;
	}

	if (sensor_sel_1 != SEN_SEL_NONE) {
		// open video_record modules (main)
		stream[0].proc_max_dim.w = vdo_size_w_1;
		stream[0].proc_max_dim.h = vdo_size_h_1;
		ret = open_module(&stream[0], &stream[0].proc_max_dim);
		if (ret != HD_OK) {
			printf("open module fail=%d\n", ret);
			goto exit;
		}

		// get videocap capability
		ret = get_cap_caps(stream[0].cap_ctrl, &stream[0].cap_syscaps);
		if (ret != HD_OK) {
			printf("get cap-caps fail=%d\n", ret);
			goto exit;
		}

		// set videocap parameter
		stream[0].cap_dim.w = vdo_size_w_1;
		stream[0].cap_dim.h = vdo_size_h_1;
		ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim, 0);
		if (ret != HD_OK) {
			printf("set cap fail=%d\n", ret);
			goto exit;
		}

		// assign parameter by program options
		main_dim.w = vdo_size_w_1;
		main_dim.h = vdo_size_h_1;

		// set videoproc parameter (ipp)
		depth = ((msf_mode > 0) && (yuv_dewarp_mode == 0)) ? 1 : 0;
		ret = set_proc_param(stream[0].proc_path[0], &main_dim, depth);
		if (ret != HD_OK) {
			printf("set proc(ipp) fail=%d\n", ret);
			goto exit;
		}

		// set videoproc parameter (vpe)
		depth = ((msf_mode > 0) && (yuv_dewarp_mode > 0)) ? 1 : 0;
		ret = set_proc_param(stream[0].proc_path[1], &main_dim, depth);
		if (ret != HD_OK) {
			printf("set proc4 fail=%d\n", ret);
			goto exit;
		}

		// set videoenc config (main)
		stream[0].enc_max_dim.w = vdo_size_w_1;
		stream[0].enc_max_dim.h = vdo_size_h_1;
		ret = set_enc_cfg(stream[0].enc_path, &stream[0].enc_max_dim, enc_bitrate * 1024 * 1024, 0);
		if (ret != HD_OK) {
			printf("set enc-cfg fail=%d\n", ret);
			goto exit;
		}

		// set videoenc parameter (main)
		stream[0].enc_dim.w = vdo_size_w_1;
		stream[0].enc_dim.h = vdo_size_h_1;
		ret = set_enc_param(stream[0].enc_path, &stream[0].enc_dim, enc_type, enc_bitrate * 1024 * 1024);
		if (ret != HD_OK) {
			printf("set enc fail=%d\n", ret);
			goto exit;
		}

		// bind video_record modules (main)
		if (msf_mode == 0) {
			//normal case
			hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
			hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOENC_0_IN_0);
		} else {

			//msf case
			hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
			if (yuv_dewarp_mode) {
				//msf + dewarp case
				hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOPROC_2_IN_0);
			}
		}

		set_proc_flip_xy(stream[0].proc_path[0], VPROC_IPP_DIR);
		video_info[0].p_stream = &stream[0];
	}

	if (sensor_sel_2 != SEN_SEL_NONE) {
		// open video liview modules (sensor 2nd)
		stream[1].proc_max_dim.w = vdo_size_w_2; //assign by user
		stream[1].proc_max_dim.h = vdo_size_h_2; //assign by user
		ret = open_module2(&stream[1], &stream[1].proc_max_dim);
		if (ret != HD_OK) {
			printf("open module2 fail=%d\n", ret);
			goto exit;
		}

		// get videocap capability
		ret = get_cap_caps(stream[1].cap_ctrl, &stream[1].cap_syscaps);
		if (ret != HD_OK) {
			printf("get cap-caps fail=%d\n", ret);
			goto exit;
		}

		// set videocap parameter
		stream[1].cap_dim.w = vdo_size_w_2;
		stream[1].cap_dim.h = vdo_size_h_2;
		ret = set_cap_param(stream[1].cap_path, &stream[1].cap_dim, 1);
		if (ret != HD_OK) {
			printf("set cap2 fail=%d\n", ret);
			goto exit;
		}

		main_dim.w = vdo_size_w_2;
		main_dim.h = vdo_size_h_2;

		// set videoproc parameter (ipp)
		depth = ((msf_mode > 0) && (ir_dewarp_mode == 0)) ? 1 : 0;
		ret = set_proc_param(stream[1].proc_path[0], &main_dim, depth);
		if (ret != HD_OK) {
			printf("set proc2 fail=%d\n", ret);
			goto exit;
		}

		// set videoproc parameter (vpe)
		depth = ((msf_mode > 0) && (ir_dewarp_mode > 0)) ? 1 : 0;
		ret = set_proc_param(stream[1].proc_path[1], &main_dim, depth);
		if (ret != HD_OK) {
			printf("set proc3 fail=%d\n", ret);
			goto exit;
		}

		stream[1].enc_max_dim.w = vdo_size_w_2;
		stream[1].enc_max_dim.h = vdo_size_h_2;
		ret = set_enc_cfg(stream[1].enc_path, &stream[1].enc_max_dim, enc_bitrate * 1024 * 1024, 1);
		if (ret != HD_OK) {
			printf("set enc-cfg fail=%d\n", ret);
			goto exit;
		}

		stream[1].enc_dim.w = vdo_size_w_2;
		stream[1].enc_dim.h = vdo_size_h_2;
		ret = set_enc_param(stream[1].enc_path, &stream[1].enc_dim, enc_type, enc_bitrate * 1024 * 1024);
		if (ret != HD_OK) {
			printf("set enc2 fail=%d\n", ret);
			goto exit;
		}

		if (msf_mode == 0) {
			//normal case
			hd_videocap_bind(HD_VIDEOCAP_1_OUT_0, HD_VIDEOPROC_1_IN_0);
			hd_videoproc_bind(HD_VIDEOPROC_1_OUT_0, HD_VIDEOENC_0_IN_1);
		} else {

			//msf case
			hd_videocap_bind(HD_VIDEOCAP_1_OUT_0, HD_VIDEOPROC_1_IN_0);
			if (ir_dewarp_mode) {
				//msf + dewarp case
				hd_videoproc_bind(HD_VIDEOPROC_1_OUT_0, HD_VIDEOPROC_3_IN_0);
			}
		}

		set_proc_flip_xy(stream[1].proc_path[0], VPROC_IPP_DIR);
		video_info[1].p_stream = &stream[1];
	}

	if (msf_mode) {

		UINT32 sw_vd_sync = (HD_VIDEOCAP_0|HD_VIDEOCAP_1);

		ret = vendor_videocap_set(stream[0].cap_ctrl, VENDOR_VIDEOCAP_PARAM_SW_VD_SYNC, &sw_vd_sync);
		if (ret != HD_OK) {
			printf("VENDOR_VIDEOCAP_PARAM_SW_VD_SYNC failed!(%d)\r\n", ret);
		}

		ret = vendor_videocap_set(stream[1].cap_ctrl, VENDOR_VIDEOCAP_PARAM_SW_VD_SYNC, &sw_vd_sync);
		if (ret != HD_OK) {
			printf("VENDOR_VIDEOCAP_PARAM_SW_VD_SYNC failed!(%d)\r\n", ret);
		}

		//setting msf information
		msf_proc_init(&gmsf_info.yuv, "yuv_proc", yuv_dewarp_mode, &stream[0], msf_pull_frame_thread, &gmsf_info.yuv);
		msf_proc_init(&gmsf_info.ir, "ir_proc", ir_dewarp_mode, &stream[1], msf_pull_frame_thread, &gmsf_info.ir);
		msf_proc_init(&gmsf_info.msf, "msf_proc", 0, 0, msf_proc_thread, &gmsf_info);
	}

	if (sensor_sel_1 != SEN_SEL_NONE) {
		ret = pthread_create(&stream[0].enc_thread_id, NULL, encode_thread, (void *)&video_info[0]);
	} else {
		ret = pthread_create(&stream[1].enc_thread_id, NULL, encode_thread, (void *)&video_info[0]);
	}

	if (ret < 0) {
		printf("create encode thread failed");
		goto exit;
	}

	if (sensor_sel_1 != SEN_SEL_NONE) {
		if (data_mode1) {
			//direct NOTE: ensure videocap start after 1st videoproc phy path start
			hd_videoproc_start(stream[0].proc_path[0]);
			hd_videocap_start(stream[0].cap_path);
		} else {
			hd_videocap_start(stream[0].cap_path);
			hd_videoproc_start(stream[0].proc_path[0]);

			if (yuv_dewarp_mode) {
				hd_videoproc_start(stream[0].proc_path[1]);
			}
		}

		// just wait ae/awb stable for auto-test, if don't care, user can remove it
		//sleep(1);

		hd_videoenc_start(stream[0].enc_path);
	}

	if (sensor_sel_2 != SEN_SEL_NONE) {
		hd_videocap_start(stream[1].cap_path);
		hd_videoproc_start(stream[1].proc_path[0]);

		if (ir_dewarp_mode) {
			hd_videoproc_start(stream[1].proc_path[1]);
		}

		// just wait ae/awb stable for auto-test, if don't care, user can remove it
		//sleep(1);
		if (msf_mode == 0) {
			hd_videoenc_start(stream[1].enc_path);
		}
	}

	// let encode_thread start to work
	if (sensor_sel_1 != SEN_SEL_NONE) {
		stream[0].flow_start = 1;
	} else {
		stream[1].flow_start = 1;
	}

	// query user key
	printf("Enter q to exit \n");

	while (1) {
		if (msf_mode == 1) {
			printf("\n");
			printf("======================================\n");
			printf("Enter f to msf adj mode\n");
			printf("Enter q to exit\n");
			printf("--------------------------------------\n");
		}

		key = GETCHAR();
		if (key == 'q' || key == 0x3) {
			// let encode_thread stop loop and exit
			stream[0].enc_exit = 1;

			if (msf_mode) {
				msf_proc_exit(&gmsf_info.msf); 		//msf
				msf_proc_exit(&gmsf_info.yuv); 		//yuv
				msf_proc_exit(&gmsf_info.ir);		//ir
			}

			// quit program
			break;
		}

		if (msf_mode == 1) {
			if (key == 'f') {
				msf_menu_entry(msf_menu, "MSF");
			}
		}
	}
	nvtrtspd_close();

	// stop video_record modules (main)
	if (sensor_sel_1 != SEN_SEL_NONE) {
		if (data_mode1) {
			//direct NOTE: ensure videocap stop after all videoproc path stop
			hd_videoproc_stop(stream[0].proc_path[0]);
			hd_videocap_stop(stream[0].cap_path);
		} else {
			hd_videocap_stop(stream[0].cap_path);
			hd_videoproc_stop(stream[0].proc_path[0]);

			if (yuv_dewarp_mode) {
				hd_videoproc_stop(stream[0].proc_path[1]);
			}
		}
		hd_videoenc_stop(stream[0].enc_path);

		// refresh to indicate no stream
		refresh_video_info(0, &video_info[0]);
	}

	if (sensor_sel_2 != SEN_SEL_NONE) {
		hd_videocap_stop(stream[1].cap_path);
		hd_videoproc_stop(stream[1].proc_path[0]);

		if (ir_dewarp_mode) {
			hd_videoproc_stop(stream[1].proc_path[1]);
		}

		if (msf_mode == 0) {
			hd_videoenc_stop(stream[1].enc_path);
		}

		// refresh to indicate no stream
		refresh_video_info(1, &video_info[1]);
	}

	// destroy encode thread
	if (sensor_sel_1 != SEN_SEL_NONE) {
		pthread_join(stream[0].enc_thread_id, NULL);
	} else {
		pthread_join(stream[1].enc_thread_id, NULL);
	}

	if (msf_mode) {
		msf_proc_uninit(&gmsf_info.yuv);
		msf_proc_uninit(&gmsf_info.ir);
		msf_proc_uninit(&gmsf_info.msf);
	}

	if (sensor_sel_1 != SEN_SEL_NONE) {
		// unbind video_record modules (main)
		hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
		if (msf_mode == 0) {
			hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);
		} else {
			if (yuv_dewarp_mode) {
				hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);
			}
		}

		if (vir_addr_main) hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
	}

	if (sensor_sel_2 != SEN_SEL_NONE) {
		// unbind video_record modules (sub)
		hd_videocap_unbind(HD_VIDEOCAP_1_OUT_0);
		if (msf_mode == 0) {
			hd_videoproc_unbind(HD_VIDEOPROC_1_OUT_0);
		} else {
			if (ir_dewarp_mode) {
				hd_videoproc_unbind(HD_VIDEOPROC_1_OUT_0);
			}
		}

		if (vir_addr_sub) hd_common_mem_munmap((void *)vir_addr_sub, phy_buf_sub.buf_info.buf_size);
	}

exit:
	if (sensor_sel_1 != SEN_SEL_NONE) {
		// close video_record modules (main)
		ret = close_module(&stream[0]);
		if (ret != HD_OK) {
			printf("close fail=%d\n", ret);
		}
	}

	if (sensor_sel_2 != SEN_SEL_NONE) {
		ret = close_module(&stream[1]);
		if (ret != HD_OK) {
			printf("close fail=%d\n", ret);
		}
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
