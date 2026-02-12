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
#include "vendor_videoenc.h"
#include "uvc_pu.h"
#include "hd_type.h"
#include "ae_ui.h"
#include "uvc_pu.h"
#include "uvc_ct.h"
#include "vendor_isp.h"

HD_IPOINT ct_zoom_table [41] = {
{1920,1080},
{1900,1070},
{1880,1058},
{1860,1044},
{1840,1034},
{1820,1024},
{1800,1012},
{1780,1000},
{1760,990},
{1740,980},
{1720,966},
{1700,956},
{1680,944},
{1660,932},
{1640,922},
{1620,910},
{1600,900},
{1580,888},
{1560,878},
{1540,866},
{1520,854},
{1500,842},
{1480,832},
{1460,820},
{1440,810},
{1420,798},
{1400,786},
{1380,776},
{1360,764},
{1340,752},
{1320,742},
{1300,730},
{1280,720},
{1260,708},
{1240,696},
{1220,686},
{1200,674},
{1180,662},
{1160,652},
{1140,640},
{1120,630}
};


// platform dependent
#if 1//defined(__LINUX)
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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_video_record, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define DEBUG_MENU 		1

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

#define UVC_ON 1
#define AUDIO_ON 1
#define WRITE_BS 0

#if UVC_ON

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <compiler.h>
#include <usb2dev.h>
#include "UVAC.h"
#include "comm/timer.h"
#include "kwrap/stdio.h"
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/examsys.h"
#include "kwrap/sxcmd.h"
#include <kwrap/cmdsys.h>
#include "kwrap/error_no.h"
#include <kwrap/util.h>
#include "hd_common.h"
#include "FileSysTsk.h"
#include <sys/stat.h>

#define BULK_PACKET_SIZE    512

#define USB_VID                         0x0603
#define USB_PID_PCCAM                   0x8612 // not support pc cam
#define USB_PID_WRITE                   0x8614
#define USB_PID_PRINT                   0x8613
#define USB_PID_MSDC                    0x8611

#define USB_PRODUCT_REVISION            '1', '.', '0', '0'
#define USB_VENDER_DESC_STRING          'N', 0x00,'O', 0x00,'V', 0x00,'A', 0x00,'T', 0x00,'E', 0x00,'K', 0x00, 0x20, 0x00,0x00, 0x00 // NULL
#define USB_VENDER_DESC_STRING_LEN      0x09
#define USB_PRODUCT_DESC_STRING         'D', 0x00,'E', 0x00,'M', 0x00,'O', 0x00,'1', 0x00, 0x20, 0x00,0x00, 0x00 // NULL
#define USB_PRODUCT_DESC_STRING_LEN     0x07
#define USB_PRODUCT_STRING              'N','v','t','-','D','S','C'
#define USB_SERIAL_NUM_STRING           '9', '8', '5', '2', '0','0', '0', '0', '0', '0','0', '0', '1', '0', '0'
#define USB_VENDER_STRING               'N','O','V','A','T','E','K'
#define USB_VENDER_SIDC_DESC_STRING     'N', 0x00,'O', 0x00,'V', 0x00,'A', 0x00,'T', 0x00,'E', 0x00,'K', 0x00, 0x20, 0x00,0x00, 0x00 // NULL
#define USB_VENDER_SIDC_DESC_STRING_LEN 0x09


static UINT8  gTestH264HdrValue[] = {
	0x00, 0x00, 0x00, 0x01, 0x67, 0x4D, 0x00, 0x32, 0x8D, 0xA4, 0x05, 0x01, 0xEC, 0x80,
	0x00, 0x00, 0x00, 0x01, 0x68, 0xEE, 0x3C, 0x80
};

typedef struct _CDCLineCoding {
	UINT32   uiBaudRateBPS; ///< Data terminal rate, in bits per second.
	UINT8    uiCharFormat;  ///< Stop bits.
	UINT8    uiParityType;  ///< Parity.
	UINT8    uiDataBits;    ///< Data bits (5, 6, 7, 8 or 16).
} CDCLineCoding;

typedef enum _CDC_PSTN_REQUEST {
	REQ_SET_LINE_CODING         =    0x20,
	REQ_GET_LINE_CODING         =    0x21,
	REQ_SET_CONTROL_LINE_STATE  =    0x22,
	REQ_SEND_BREAK              =    0x23,
	ENUM_DUMMY4WORD(CDC_PSTN_REQUEST)
} CDC_PSTN_REQUEST;

#define NVT_UI_UVAC_RESO_CNT  2
static UVAC_VID_RESO gUIUvacVidReso[NVT_UI_UVAC_RESO_CNT] = {
	{ 1920,   1080,   1,      30,      0,      0},
	{ 1280,    720,   1,      30,      0,      0},
};
#define NVT_UI_UVAC_AUD_SAMPLERATE_CNT                  1
//#define NVT_UI_FREQUENCY_16K                    0x003E80   //16k
#define NVT_UI_FREQUENCY_32K                    0x007D00   //32k
//#define NVT_UI_FREQUENCY_48K                    0x00BB80   //48k
static UINT32 gUIUvacAudSampleRate[NVT_UI_UVAC_AUD_SAMPLERATE_CNT] = {
	NVT_UI_FREQUENCY_32K
};

static  UVAC_VIDEO_FORMAT m_VideoFmt[UVAC_VID_DEV_CNT_MAX] = {0}; //UVAC_VID_DEV_CNT_MAX=2
static BOOL m_bIsStaticPattern = FALSE;

static UINT32 uigFrameIdx = 0, uigAudIdx = 0;
static UINT32 uvac_pa = 0;
static UINT32 uvac_va = 0;
static UINT32 uvac_size = 0;
static BOOL UVAC_enable(void);
static BOOL UVAC_disable(void);
#endif

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
#define CAP_OUT_FMT		HD_VIDEO_PXLFMT_RAW12
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

///////////////////////////////////////////////////////////////////////////////
HD_VIDEOENC_BUFINFO phy_buf_main;
HD_AUDIOCAP_BUFINFO phy_buf_main2;
UINT32 vir_addr_main;
UINT32 vir_addr_main2;
#define MAX_CAP_SIZE_W 1920
#define MAX_CAP_SIZE_H 1080
static int vdo_size_w =	1920;
static int vdo_size_h =	1080;
pthread_t cap_thread_id;
UINT32     flow_start=0;
#define JPG_YUV_TRANS 0

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
	UINT32     codec_type;

} VIDEO_RECORD;
VIDEO_RECORD stream[1] = {0}; //0: main stream

#if  AUDIO_ON
typedef struct _AUDIO_CAPONLY {
	HD_AUDIO_SR sample_rate_max;
	HD_AUDIO_SR sample_rate;

	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	UINT32 cap_exit;
} AUDIO_CAPONLY;

AUDIO_CAPONLY au_cap = {0};
#endif


static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(MAX_CAP_SIZE_W, MAX_CAP_SIZE_H, CAP_OUT_FMT)
														+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
														+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	mem_cfg.pool_info[0].blk_cnt = 2;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;

	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(MAX_CAP_SIZE_W, MAX_CAP_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[1].blk_cnt = 3;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	
#if AUDIO_ON
	// config common pool audio (main)
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[2].blk_size = 0x10000;
	mem_cfg.pool_info[2].blk_cnt = 2;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;
#endif

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
/*AUDIO */
#if AUDIO_ON
static HD_RESULT set_cap_cfg2(HD_PATH_ID *p_audio_cap_ctrl, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret;
	HD_PATH_ID audio_cap_ctrl = 0;
	HD_AUDIOCAP_DEV_CONFIG audio_dev_cfg = {0};
	HD_AUDIOCAP_DRV_CONFIG audio_drv_cfg = {0};

	ret = hd_audiocap_open(0, HD_AUDIOCAP_0_CTRL, &audio_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	// set audiocap dev parameter
	audio_dev_cfg.in_max.sample_rate = HD_AUDIO_SR_32000;
	audio_dev_cfg.in_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_dev_cfg.in_max.mode = HD_AUDIO_SOUND_MODE_MONO;
	audio_dev_cfg.in_max.frame_sample = 1024;
	audio_dev_cfg.frame_num_max = 10;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DEV_CONFIG, &audio_dev_cfg);
	if (ret != HD_OK) {
		return ret;
	}

	// set audiocap drv parameter
	audio_drv_cfg.mono = HD_AUDIO_MONO_RIGHT;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DRV_CONFIG, &audio_drv_cfg);

	*p_audio_cap_ctrl = audio_cap_ctrl;
	return ret;
}

static HD_RESULT set_cap_param2(HD_PATH_ID audio_cap_path, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOCAP_IN audio_cap_param = {0};

	// set audiocap input parameter
	audio_cap_param.sample_rate = HD_AUDIO_SR_32000;
	audio_cap_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cap_param.mode = HD_AUDIO_SOUND_MODE_MONO;
	audio_cap_param.frame_sample = 1024;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_IN, &audio_cap_param);

	return ret;
}

static HD_RESULT open_module2(AUDIO_CAPONLY *p_caponly)
{
	HD_RESULT ret;
	ret = set_cap_cfg2(&p_caponly->cap_ctrl, p_caponly->sample_rate_max);
	if (ret != HD_OK) {
		printf("set cap-cfg fail\n");
		return HD_ERR_NG;
	}
	if((ret = hd_audiocap_open(HD_AUDIOCAP_0_IN_0, HD_AUDIOCAP_0_OUT_0, &p_caponly->cap_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module2(AUDIO_CAPONLY *p_caponly)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_close(p_caponly->cap_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module2(void)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}

static void *capture_thread(void *arg)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIO_FRAME  data_pull;
	int get_phy_flag=0;
#if WRITE_BS
	char file_path_main[64], file_path_len[64];
	FILE *f_out_main, *f_out_len;
#endif
	AUDIO_CAPONLY *p_cap_only = (AUDIO_CAPONLY *)arg;
#if UVC_ON
	UVAC_STRM_FRM strmFrm = {0};
#endif
	#define PHY2VIRT_MAIN2(pa) (vir_addr_main2 + (pa - phy_buf_main2.buf_info.phy_addr))

#if WRITE_BS
	/* config pattern name */
	snprintf(file_path_main, sizeof(file_path_main), "/mnt/sd/audio_bs_%d_%d_%d_pcm.dat", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_cap_only->sample_rate);
	snprintf(file_path_len, sizeof(file_path_len), "/mnt/sd/audio_bs_%d_%d_%d_pcm.len", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_cap_only->sample_rate);
#endif

#if WRITE_BS
	/* open output files */
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		printf("open file (%s) fail....\r\n", file_path_main);
	} else {
		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}

	if ((f_out_len = fopen(file_path_len, "wb")) == NULL) {
		printf("open len file (%s) fail....\r\n", file_path_len);
	}
#endif

	/* pull data test */
	while (p_cap_only->cap_exit == 0) {

		if (!flow_start){
			if (get_phy_flag){
				/* mummap for bs buffer */
				//printf("release common buffer2\r\n");
				if (vir_addr_main2)hd_common_mem_munmap((void *)vir_addr_main2, phy_buf_main2.buf_info.buf_size);

				get_phy_flag = 0;
			}
			/* wait flow_start */
			usleep(10);
			continue;
		}else{
			if (!get_phy_flag){

				/* query physical address of bs buffer	(this can ONLY query after hd_audiocap_start() is called !!) */
				hd_audiocap_get(au_cap.cap_ctrl, HD_AUDIOCAP_PARAM_BUFINFO, &phy_buf_main2);

				/* mmap for bs buffer  (just mmap one time only, calculate offset to virtual address later) */
				vir_addr_main2 = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main2.buf_info.phy_addr, phy_buf_main2.buf_info.buf_size);
				if (vir_addr_main2 == 0) {
					printf("mmap error\r\n");
					return 0;
				}

				get_phy_flag = 1;
			}
		}

		// pull data
		ret = hd_audiocap_pull_out_buf(p_cap_only->cap_path, &data_pull, 200); // >1 = timeout mode

		if (ret == HD_OK) {
			UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN2(data_pull.phy_addr[0]);
			UINT32 size = data_pull.size;
			UINT32 timestamp = hd_gettime_ms();
			//printf("audio fram size =%u\r\n",size);
#if UVC_ON
			strmFrm.path = UVAC_STRM_AUD;
			strmFrm.addr = data_pull.phy_addr[0];
			strmFrm.size = data_pull.size;
			UVAC_SetEachStrmInfo(&strmFrm);
#endif			

#if WRITE_BS
			// write bs
			if (f_out_main) fwrite(ptr, 1, size, f_out_main);
			if (f_out_main) fflush(f_out_main);

			// write bs len
			if (f_out_len) fprintf(f_out_len, "%d %d\n", size, timestamp);
			if (f_out_len) fflush(f_out_len);
#endif
			// release data
			ret = hd_audiocap_release_out_buf(p_cap_only->cap_path, &data_pull);
			if (ret != HD_OK) {
				printf("release buffer failed. ret=%x\r\n", ret);
			}
		}

		usleep(1000000/32000*2);
	}

	if (get_phy_flag){
		/* mummap for bs buffer */
		//printf("release common buffer2\r\n");
		if (vir_addr_main2)hd_common_mem_munmap((void *)vir_addr_main2, phy_buf_main2.buf_info.buf_size);
	}

#if WRITE_BS
	/* close output file */
	if (f_out_main) fclose(f_out_main);
	if (f_out_len) fclose(f_out_len);
#endif
	return 0;
}
#endif // AUDIO_ON
/*AUDIO end*/

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
	printf("sys_info.devid =0x%X, cur_fps[0]=%d/%d, vd_count=%llu\r\n", sys_info.dev_id, GET_HI_UINT16(sys_info.cur_fps[0]), GET_LO_UINT16(sys_info.cur_fps[0]), sys_info.vd_count);
	return ret;
}

static HD_RESULT set_cap_cfg(HD_PATH_ID *p_video_cap_ctrl)
{
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
		video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
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
		video_crop_param.win.rect.w = MAX_CAP_SIZE_W/2;
		video_crop_param.win.rect.h= MAX_CAP_SIZE_H/2;
		video_crop_param.align.w = 4;
		video_crop_param.align.h = 4;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &video_crop_param);
		//printf("set_cap_param CROP ON=%d\r\n", ret);
	}
	#endif
	{
		HD_VIDEOCAP_OUT video_out_param = {0};

		//without setting dim for no scaling, using original sensor out size
		video_out_param.pxlfmt = CAP_OUT_FMT;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
	}

	return ret;
}

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
		video_cfg_param.ctrl_max.func = 0;
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;
		video_cfg_param.in_max.pxlfmt = CAP_OUT_FMT;
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
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}
	return ret;
}

static HD_RESULT set_enc_cfg(HD_PATH_ID video_enc_path, HD_DIM *p_max_dim, UINT32 max_bitrate)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_PATH_CONFIG video_path_config = {0};

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
		video_path_config.isp_id             = 0;
		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_PATH_CONFIG, &video_path_config);
		if (ret != HD_OK) {
			printf("set_enc_path_config = %d\r\n", ret);
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

	if (p_dim != NULL) {

		//--- HD_VIDEOENC_PARAM_IN ---
		video_in_param.dir           = HD_VIDEO_DIR_NONE;
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
			//printf("enc_type = HD_CODEC_TYPE_H265\r\n");
			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_H265;
			video_out_param.h26x.profile       = HD_H265E_MAIN_PROFILE;
			video_out_param.h26x.level_idc     = HD_H265E_LEVEL_5;
			video_out_param.h26x.gop_num       = 15;
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
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control = %d\r\n", ret);
				return ret;
			}
		} else if (enc_type == 1) {
			//printf("enc_type = HD_CODEC_TYPE_H264\r\n");
			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_H264;
			video_out_param.h26x.profile       = HD_H264E_HIGH_PROFILE;
			video_out_param.h26x.level_idc     = HD_H264E_LEVEL_5_1;
			video_out_param.h26x.gop_num       = 15;
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
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control = %d\r\n", ret);
				return ret;
			}

		} else if (enc_type == 2) {
			//printf("enc_type = HD_CODEC_TYPE_JPEG\r\n");
			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_JPEG;
			video_out_param.jpeg.retstart_interval = 0;
			video_out_param.jpeg.image_quality = 50;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}
			
			//MJPG YUV trans(YUV420 to YUV422)
			VENDOR_VIDEOENC_JPG_YUV_TRANS_CFG yuv_trans_cfg = {0};
			yuv_trans_cfg.jpg_yuv_trans_en = JPG_YUV_TRANS;
			//printf("MJPG YUV TRANS(%d)\r\n", JPG_YUV_TRANS);
			vendor_videoenc_set(video_enc_path, VENDOR_VIDEOENC_PARAM_OUT_JPG_YUV_TRANS, &yuv_trans_cfg);

		} else {

			printf("not support enc_type\r\n");
			return HD_ERR_NG;
		}
	}

	return ret;
}

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
    if ((ret = hd_videoenc_init()) != HD_OK)
		return ret;
#if AUDIO_ON		
	if((ret = hd_audiocap_init()) != HD_OK)
        return ret;
#endif
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
	ret = set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim);
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

static void *encode_thread(void *arg)
{

	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_BS  data_pull;
	UINT32 j;
	int get_phy_flag=0;

#if WRITE_BS
	char file_path_main[32] = "/mnt/sd/dump_bs_main.dat";
	FILE *f_out_main;
#endif
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))
	
#if UVC_ON
	UVAC_STRM_FRM      strmFrm = {0};
	UINT32             loop =0;
	UVAC_VID_BUF_INFO  vid_buf_info = {0};
	UINT32             uvacNeedMem=0;
	UINT32             ChannelNum = 1;
#endif
	//UINT32 temp=1;

#if WRITE_BS
	//----- open output files -----
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		HD_VIDEOENC_ERR("open file (%s) fail....\r\n", file_path_main);
	} else {
		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}
#endif

	//--------- pull data test ---------
	while (p_stream0->enc_exit == 0) {
		if (!flow_start){
			if (get_phy_flag){
				/* mummap for bs buffer */
				//printf("release common buffer2\r\n");
				if (vir_addr_main)hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);

				get_phy_flag = 0;	
			}
			//------ wait flow_start ------
			usleep(10);
			continue;
		}else{
			if (!get_phy_flag){

				// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
				ret = hd_videoenc_get(stream[0].enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);

				// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
				vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);

				get_phy_flag = 1;
			}
		}

		//pull data
		ret = hd_videoenc_pull_out_buf(p_stream0->enc_path, &data_pull, -1); // -1 = blocking mode

		if (ret == HD_OK) {
#if UVC_ON
			//strmFrm.path = (i % 2 == 0) ? UVAC_STRM_VID : UVAC_STRM_VID2;
			strmFrm.path = UVAC_STRM_VID ;
			strmFrm.addr = data_pull.video_pack[data_pull.pack_num-1].phy_addr;
			strmFrm.size = data_pull.video_pack[data_pull.pack_num-1].size;
			strmFrm.pStrmHdr = 0;
			strmFrm.strmHdrSize = 0;
			if (p_stream0->codec_type == HD_CODEC_TYPE_H264){
				if (data_pull.pack_num > 1) {
					//printf("I-frame\r\n");
					strmFrm.pStrmHdr = (UINT8 *)data_pull.video_pack[0].phy_addr;
					for (loop = 0; loop < data_pull.pack_num - 1; loop ++) {
						strmFrm.strmHdrSize += data_pull.video_pack[loop].size;
					}
				} else {
					strmFrm.pStrmHdr = 0;
					strmFrm.strmHdrSize = 0;
					//printf("P-frame\r\n");
				}
			}
			UVAC_SetEachStrmInfo(&strmFrm);
			/*timstamp for test
			if (temp){
				system("echo \"uvc\" > /proc/nvt_info/bootts");
				temp=0;
			}
			*/
#endif
			
#if WRITE_BS		
			for (j=0; j< data_pull.pack_num; j++) {
				UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.video_pack[j].phy_addr);
				UINT32 len = data_pull.video_pack[j].size;
				if (f_out_main) fwrite(ptr, 1, len, f_out_main);
				if (f_out_main) fflush(f_out_main);
			}

#endif		
			// release data
			ret = hd_videoenc_release_out_buf(p_stream0->enc_path, &data_pull);
			if (ret != HD_OK) {
				printf("enc_release error=%d !!\r\n", ret);
			}
		}
		usleep(33000);
	}
	if (get_phy_flag){
		// mummap for bs buffer
		printf("release common buffer\r\n");
		if (vir_addr_main) hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
	}
	
#if WRITE_BS
	// close output file
	if (f_out_main) fclose(f_out_main);
#endif

	return 0;
}

#if UVC_ON
_ALIGNED(4) static UINT16 m_UVACSerialStrDesc3[] = {
	0x0320,                             // 20: size of String Descriptor = 32 bytes
	// 03: String Descriptor type
	'5', '1', '0', '5', '5',            // 96611-00000-001 (default)
	'0', '0', '0', '0', '0',
	'0', '0', '1', '0', '0'
};
_ALIGNED(4) const static UINT8 m_UVACManuStrDesc[] = {
	USB_VENDER_DESC_STRING_LEN * 2 + 2, // size of String Descriptor = 6 bytes
	0x03,                       // 03: String Descriptor type
	USB_VENDER_DESC_STRING
};

_ALIGNED(4) const static UINT8 m_UVACProdStrDesc[] = {
	USB_PRODUCT_DESC_STRING_LEN * 2 + 2, // size of String Descriptor = 6 bytes
	0x03,                       // 03: String Descriptor type
	USB_PRODUCT_DESC_STRING
};


static void USBMakerInit_UVAC(UVAC_VEND_DEV_DESC *pUVACDevDesc)
{

	pUVACDevDesc->pManuStringDesc = (UVAC_STRING_DESC *)m_UVACManuStrDesc;
	pUVACDevDesc->pProdStringDesc = (UVAC_STRING_DESC *)m_UVACProdStrDesc;

	pUVACDevDesc->pSerialStringDesc = (UVAC_STRING_DESC *)m_UVACSerialStrDesc3;

	pUVACDevDesc->VID = USB_VID;
	pUVACDevDesc->PID = USB_PID_PCCAM;

}

static UINT32 xUvacStartVideoCB(UVAC_VID_DEV_CNT vidDevIdx, UVAC_STRM_INFO *pStrmInfo)
{
	HD_DIM main_dim;
	int ret=0;

	if (pStrmInfo) {
		//printf("xxxxUVAC Start[%d] resoIdx=%d,W=%d,H=%d,codec=%d,fps=%d,path=%d,tbr=0x%x\r\n", vidDevIdx, pStrmInfo->strmResoIdx, pStrmInfo->strmWidth, pStrmInfo->strmHeight, pStrmInfo->strmCodec, pStrmInfo->strmFps, pStrmInfo->strmPath, pStrmInfo->strmTBR);
		m_VideoFmt[vidDevIdx] = pStrmInfo->strmCodec;

		//stop poll data
		flow_start = 0;
		// mummap for video bs buffer
		if (vir_addr_main) hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
		/* mummap for audio bs buffer */
		if (vir_addr_main2)hd_common_mem_munmap((void *)vir_addr_main2, phy_buf_main2.buf_info.buf_size);

		//stop engine(modules)
#if AUDIO_ON
		//stop audio capture module
		hd_audiocap_stop(au_cap.cap_path);
#endif
		// stop video_record modules (main)
		hd_videocap_stop(stream[0].cap_path);
		hd_videoproc_stop(stream[0].proc_path);
		hd_videoenc_stop(stream[0].enc_path);

		//set codec,resolution
		switch (pStrmInfo->strmCodec){
			case 0:
				stream[0].codec_type = 1;
				//printf("set codec=H264\r\n");
				break;
			case 1:
				stream[0].codec_type = 2;
				//printf("set codec=MJPG\r\n");
				break;
			default:
				printf("unrecognized codec(%d)\r\n",pStrmInfo->strmCodec);
				break;
		}

		//set encode resolution to device
		stream[0].enc_dim.w = pStrmInfo->strmWidth;
		stream[0].enc_dim.h = pStrmInfo->strmHeight;

		// set videoproc parameter (main)
		ret = set_proc_param(stream[0].proc_path, &stream[0].enc_dim);
		if (ret != HD_OK) {
			printf("set proc fail=%d\n", ret);
		}
		// set videoenc parameter (main)
		ret = set_enc_param(stream[0].enc_path, &stream[0].enc_dim, stream[0].codec_type, 2 * 1024 * 1024);
		if (ret != HD_OK) {
			printf("set enc fail=%d\n", ret);
		}

		//start engine(modules)
		hd_videocap_start(stream[0].cap_path);
		hd_videoproc_start(stream[0].proc_path);
		hd_videoenc_start(stream[0].enc_path);

#if AUDIO_ON
		//start audio capture module
		hd_audiocap_start(au_cap.cap_path);
#endif
		flow_start = 1;
	}
	return E_OK;
}

static UINT32 xUvacStopVideoCB(UINT32 isClosed)
{
	//printf(":isClosed=%d\r\n", isClosed);
	printf("stop encode flow\r\n");
	flow_start = 0;
	return E_OK;
}

static BOOL UVAC_enable(void)
{

	UVAC_INFO       UvacInfo = {0};
	UINT32          retV = 0;
	
	UVAC_VEND_DEV_DESC UIUvacDevDesc = {0};
	UVAC_AUD_SAMPLERATE_ARY uvacAudSampleRateAry = {0};
	UINT32 bVidFmtType = UVAC_VIDEO_FORMAT_H264_MJPEG;
	m_bIsStaticPattern = FALSE;
	UVAC_VID_BUF_INFO vid_buf_info = {0};
	UINT32            uvacNeedMem  = 0;
	UINT32            ChannelNum   = 1;
	
	printf("###Real Open UVAC-lib\r\n");
	
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32 pa;
	void *va;
	HD_RESULT hd_ret;
	
	timer_init();
	
	hd_ret = hd_gfx_init();
	if(hd_ret != HD_OK) {
        printf("init gfx fail=%d\n", hd_ret);
	}
	
	uvac_size = UVAC_GetNeedMemSize();
	//printf("uvac_size = %u\r\n", uvac_size);
	if ((hd_ret = hd_common_mem_alloc("usbmovie", &pa, (void **)&va, uvac_size, ddr_id)) != HD_OK) {
		printf("hd_common_mem_alloc failed(%d)\r\n", hd_ret);
		uvac_va = 0;
		uvac_pa = 0;
		uvac_size = 0;
	} else {
		uvac_va = (UINT32)va;
		uvac_pa = (UINT32)pa;
	}
	sleep(1);
	uvacNeedMem *= ChannelNum;
	UvacInfo.UvacMemAdr    = uvac_pa;
	UvacInfo.UvacMemSize   = uvac_size;

	vid_buf_info.vid_buf_pa =  phy_buf_main.buf_info.phy_addr;
	vid_buf_info.vid_buf_size =  phy_buf_main.buf_info.buf_size;
	UVAC_SetConfig(UVAC_CONFIG_VID_BUF_INFO, (UINT32)&vid_buf_info);

	UvacInfo.fpStartVideoCB  = (UVAC_STARTVIDEOCB)xUvacStartVideoCB;
	UvacInfo.fpStopVideoCB  = (UVAC_STOPVIDEOCB)xUvacStopVideoCB;
	
	USBMakerInit_UVAC(&UIUvacDevDesc);
	
	UVAC_SetConfig(UVAC_CONFIG_VEND_DEV_DESC, (UINT32)&UIUvacDevDesc);
	UVAC_SetConfig(UVAC_CONFIG_HW_COPY_CB, (UINT32)hd_gfx_memcpy);
	//printf("%s:uvacAddr=0x%x,s=0x%x;IPLAddr=0x%x,s=0x%x\r\n", __func__, UvacInfo.UvacMemAdr, UvacInfo.UvacMemSize, m_VideoBuf.addr, m_VideoBuf.size);

	UvacInfo.hwPayload[0] = FALSE;//hit
	UvacInfo.hwPayload[1] = FALSE;//hit

	//must set to 1I14P ratio
	UvacInfo.strmInfo.strmWidth = MAX_CAP_SIZE_W;
	UvacInfo.strmInfo.strmHeight = MAX_CAP_SIZE_H;
	UvacInfo.strmInfo.strmFps = 30;
	//UvacInfo.strmInfo.strmCodec = MEDIAREC_ENC_H264;
	//w=1280,720,30,2
	UVAC_ConfigVidReso(gUIUvacVidReso, NVT_UI_UVAC_RESO_CNT);
	
	uvacAudSampleRateAry.aryCnt = NVT_UI_UVAC_AUD_SAMPLERATE_CNT;
	uvacAudSampleRateAry.pAudSampleRateAry = &gUIUvacAudSampleRate[0]; //NVT_UI_FREQUENCY_32K
	UVAC_SetConfig(UVAC_CONFIG_AUD_SAMPLERATE, (UINT32)&uvacAudSampleRateAry);
	UvacInfo.channel = ChannelNum;
	//printf("w=%d,h=%d,fps=%d,codec=%d\r\n", UvacInfo.strmInfo.strmWidth, UvacInfo.strmInfo.strmHeight, UvacInfo.strmInfo.strmFps, UvacInfo.strmInfo.strmCodec);

	UVAC_SetConfig(UVAC_CONFIG_VIDEO_FORMAT_TYPE, bVidFmtType);
	
	UVAC_SetConfig(UVAC_CONFIG_CT_CB, (UINT32)&xUvacCT_CB);

	UVAC_SetConfig(UVAC_CONFIG_PU_CB, (UINT32)&xUvacPU_CB);

	retV = UVAC_Open(&UvacInfo);
	if (retV != E_OK) {
		printf("Error open UVAC task\r\n", retV);
	}

	return TRUE;
}

static BOOL UVAC_disable(void)
{
	HD_RESULT    ret = HD_OK;
	ret = hd_common_mem_free(uvac_pa, &uvac_va);
	if (HD_OK == ret) {
		printf("Error handle test for free an invalid address: fail %d\r\n", (int)(ret));
	} 
	UVAC_Close();

	return TRUE;
}
#endif

static HD_RESULT set_proc_ct_crop_param(HD_PATH_ID video_proc_path, HD_IRECT *af_crop)
{
	HD_RESULT ret = HD_OK;
	double	ratio_w = 0;
	double	ratio_h = 0;
	HD_VIDEOPROC_CROP video_out_crop_param = {0};
	HD_VIDEOPROC_OUT video_out_param = {0};
printf ("set_proc_ct_crop_param %d %d %d %d\r\n", af_crop->x, af_crop->y, af_crop->w, af_crop->h);
	
	ratio_w = (double)1920 / (double)af_crop->w;
	ratio_h = (double)1080 / (double)af_crop->h;
	if (af_crop != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		video_out_param.func = 0;
		video_out_param.dim.w = 1920 * ratio_w;
		video_out_param.dim.h = 1080 * ratio_h;
		video_out_param.dim.w = ALIGN_CEIL_4(video_out_param.dim.w);
		video_out_param.dim.h = ALIGN_CEIL_4(video_out_param.dim.h);
		//printf ("video_out_param.dim.w: %u video_out_param.dim.h: %u\r\n", video_out_param.dim.w, video_out_param.dim.h);
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = 0; //set 1 to allow pull
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
		video_out_crop_param.mode = HD_CROP_ON;
		video_out_crop_param.win.rect.x = af_crop->x * ratio_w;
		video_out_crop_param.win.rect.y = af_crop->y * ratio_h;
		
		video_out_crop_param.win.rect.x = ALIGN_CEIL_4(video_out_crop_param.win.rect.x);
		video_out_crop_param.win.rect.y = ALIGN_CEIL_4(video_out_crop_param.win.rect.y);
		video_out_crop_param.win.rect.w = 1920;
		video_out_crop_param.win.rect.h = 1080;
		//boundary checking (TODO)
#if 0
		if (video_out_crop_param.win.rect.x + video_out_crop_param.win.rect.w > 1920){
			video_out_crop_param.win.rect.x = video_out_crop_param.win.rect.x - (video_out_crop_param.win.rect.x + video_out_crop_param.win.rect.w - 1920);
		}
		if (video_out_crop_param.win.rect.y + video_out_crop_param.win.rect.h > 1080){
			video_out_crop_param.win.rect.y = video_out_crop_param.win.rect.y - (video_out_crop_param.win.rect.y + video_out_crop_param.win.rect.h - 1080);
		}
#endif
		if (video_out_crop_param.win.rect.x < 0)
			video_out_crop_param.win.rect.x = 0;
		if (video_out_crop_param.win.rect.y < 0)
			video_out_crop_param.win.rect.y = 0;

		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_CROP, &video_out_crop_param);
	} else {
		video_out_crop_param.mode = HD_CROP_OFF;
		video_out_crop_param.win.rect.x = 0;
		video_out_crop_param.win.rect.y = 0;
		video_out_crop_param.win.rect.w = 0;
		video_out_crop_param.win.rect.h = 0;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_CROP, &video_out_crop_param);
	}
	
	return ret;
}

void Set_CT_Zoom_Absolute(UINT32 indx)
{
	int ret;
	static HD_IRECT ct_crop_size = {0};
	ct_crop_size.x = (1920 - ct_zoom_table[indx].x) / 2; 
	ct_crop_size.y = (1080 - ct_zoom_table[indx].y) / 2; 
	ct_crop_size.w = ct_zoom_table[indx].x;
	ct_crop_size.h = ct_zoom_table[indx].y;

	ret = set_proc_ct_crop_param(stream[0].proc_path, &ct_crop_size);
	if (ret != HD_OK) {
		printf("fail to set vp afmask attr\r\n");
	}
	ret = hd_videoproc_start(stream[0].proc_path);
	if (ret != HD_OK) {
		printf("fail to start proc_path %d\n", ret);
	}
}

MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;

	UINT32 enc_type = 1;//0:h265, 1:h264, 2:MJPG
	HD_DIM main_dim;

	// query program options
	if (argc == 2) {
		enc_type = atoi(argv[1]);
		printf("enc_type %d\r\n", enc_type);
		if(enc_type > 2) {
			printf("error: not support enc_type!\r\n");
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

	// open video_record modules (main)
	stream[0].proc_max_dim.w =MAX_CAP_SIZE_W; //assign by user
	stream[0].proc_max_dim.h =MAX_CAP_SIZE_H; //assign by user
	ret = open_module(&stream[0], &stream[0].proc_max_dim);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

#if AUDIO_ON	
	//open capture module
	au_cap.sample_rate_max = HD_AUDIO_SR_32000; //assign by user
	ret = open_module2(&au_cap);
	if(ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	//set audiocap parameter
	au_cap.sample_rate = HD_AUDIO_SR_32000; //assign by user
	ret = set_cap_param2(au_cap.cap_path, au_cap.sample_rate);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}
#endif

	// get videocap capability
	ret = get_cap_caps(stream[0].cap_ctrl, &stream[0].cap_syscaps);
	if (ret != HD_OK) {
		printf("get cap-caps fail=%d\n", ret);
		goto exit;
	}

	// set videocap parameter
	stream[0].cap_dim.w = MAX_CAP_SIZE_W; //assign by user
	stream[0].cap_dim.h = MAX_CAP_SIZE_H; //assign by user
	ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	// assign parameter by program options
	main_dim.w = MAX_CAP_SIZE_W;
	main_dim.h = MAX_CAP_SIZE_H;

	// set videoproc parameter (main)
	ret = set_proc_param(stream[0].proc_path, &main_dim);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoenc config (main)
	stream[0].enc_max_dim.w = MAX_CAP_SIZE_W;
	stream[0].enc_max_dim.h = MAX_CAP_SIZE_H;
	ret = set_enc_cfg(stream[0].enc_path, &stream[0].enc_max_dim, 2 * 1024 * 1024);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}

	// set videoenc parameter (main)
	stream[0].enc_dim.w = main_dim.w;
	stream[0].enc_dim.h = main_dim.h;
	ret = set_enc_param(stream[0].enc_path, &stream[0].enc_dim, enc_type, 2 * 1024 * 1024);
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
#if AUDIO_ON
	//audio create capture thread
	ret = pthread_create(&cap_thread_id, NULL, capture_thread, (void *)&au_cap);
	if (ret < 0) {
		printf("create encode thread failed");
		goto exit;
	}

	//start audio capture module
	hd_audiocap_start(au_cap.cap_path);
#endif	
	
	// start video_record modules (main)
	hd_videocap_start(stream[0].cap_path);
	hd_videoproc_start(stream[0].proc_path);
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	//sleep(1);
	hd_videoenc_start(stream[0].enc_path);

	flow_start =1;

	//init isp 
	vendor_isp_init();

    sleep(2);

#if UVC_ON
	UVAC_enable(); //init usb device, start to prepare video/audio buffer to send
#endif	
	// query user key
	printf("Enter q to exit\n");
	while (1) {
		key = GETCHAR();
		if (key == 'q' || key == 0x3) {
			// let encode_thread stop loop and exit
			stream[0].enc_exit = 1; //stop video
#if AUDIO_ON			
			au_cap.cap_exit = 1; //stop audio
#endif
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
	}

	// destroy encode thread
	pthread_join(stream[0].enc_thread_id, NULL);
#if AUDIO_ON	
	pthread_join(cap_thread_id, NULL); // stop audio
#endif

#if UVC_ON
	UVAC_disable();
#endif

#if AUDIO_ON
	//stop audio capture module
	hd_audiocap_stop(au_cap.cap_path);
#endif
	// stop video_record modules (main)
	hd_videocap_stop(stream[0].cap_path);
	hd_videoproc_stop(stream[0].proc_path);
	hd_videoenc_stop(stream[0].enc_path);

	// unbind video_record modules (main)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);

exit:
	// close video_record modules (main)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}
	
#if AUDIO_ON
	//close audio module
	ret = close_module2(&au_cap);
	if(ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}
#endif	
	// uninit all modules
	ret = exit_module();
	if (ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}
#if AUDIO_ON	
	// uninit all modules
	ret = exit_module2();
	if (ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}
#endif	
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
