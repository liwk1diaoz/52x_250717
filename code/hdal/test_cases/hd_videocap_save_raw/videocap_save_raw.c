/**
	@brief Sample code of videocapture only.\n

	@file videocap_save_raw.c

	@author Ben Wang

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
#include "vendor_common.h"

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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_videocap_save_raw, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define DEBUG_MENU 1

#define CHKPNT      					printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)						printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x) 						printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

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

#define CROP_X 480
#define CROP_Y 270
#define CROP_W 960
#define CROP_H 540
BOOL raw_enc  = FALSE; // option for Bayer compress
BOOL crop = FALSE;
BOOL whole_block = TRUE;

UINT32 crop_x=0, crop_y=0, crop_w=1920, crop_h=1080;
UINT32 frame_rate = 30;
BOOL print_frame_cnt = FALSE;

///////////////////////////////////////////////////////////////////////////////

#define SEN_OUT_FMT		HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT		HD_VIDEO_PXLFMT_RAW12
#define CA_WIN_NUM_W		32
#define CA_WIN_NUM_H		32
#define LA_WIN_NUM_W		32
#define LA_WIN_NUM_H		32
#define VA_WIN_NUM_W		16
#define VA_WIN_NUM_H		16


#define VDO_SIZE_W     1920
#define VDO_SIZE_H     1080

#define CAP_OUT_Q_DEPTH  1

#define USE_REAL_SENSOR  1  // set 0 for patgen

#define RAW_BUFFER_DDR_ID DDR_ID0
///////////////////////////////////////////////////////////////////////////////

#if defined(_BSP_NA51000_)
#define MAX_FRAME_NUM  3
#elif defined(_BSP_NA51055_)
#define MAX_FRAME_NUM  2
#endif
#define MAX_DDR_NUM   2
#define MAX_RAW_QUEUE   600
typedef struct _RAW_INFO {
	UINT64 vd_cnt[MAX_RAW_QUEUE];
	void *va[MAX_RAW_QUEUE];
	UINT32 pa[MAX_RAW_QUEUE];
} RAW_INFO;

RAW_INFO raw_info[MAX_FRAME_NUM] = {0};
UINT32 idx[MAX_FRAME_NUM];



typedef struct _DDR_INFO {
	void *va;
	UINT32 pa;
	UINT32 size;
} DDR_INFO;

DDR_INFO ddr_max_free[MAX_DDR_NUM] = {0};
DDR_INFO ddr_remain[MAX_DDR_NUM] = {0};


static HD_RESULT mem_alloc_remain(UINT32 * phy_addr, void * * virt_addr, UINT32 size, HD_COMMON_MEM_DDR_ID ddr)
{
	if (ALIGN_CEIL_4(size) > ddr_remain[ddr].size) {
		return HD_ERR_NOMEM;
	} else {
		*phy_addr = ddr_remain[ddr].pa;
		*virt_addr = ddr_remain[ddr].va;
		ddr_remain[ddr].size -= ALIGN_CEIL_4(size);
		ddr_remain[ddr].pa += ALIGN_CEIL_4(size);
		ddr_remain[ddr].va = (void *)((UINT32)ddr_remain[ddr].va + ALIGN_CEIL_4(size));
		//printf("DDR[%d] pa=0x%X, va=0x%X, size=0x%X\r\n", ddr, (unsigned int)ddr_remain[ddr].pa, (unsigned int)ddr_remain[ddr].va, size);
		return HD_OK;
	}
}
static HD_RESULT mem_init(UINT32 frame_num, UINT32 *p_blk_size)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
	UINT32 w, h;

	if (crop) {
		w = crop_w;
		h = crop_h;
	} else {
		w = VDO_SIZE_W;
		h = VDO_SIZE_H;
	}
	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	if (raw_enc) {
		mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_NRX_BUFSIZE(w, h)
														+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
														+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	} else {
		mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(w, h,CAP_OUT_FMT)
														+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
														+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	}
	mem_cfg.pool_info[0].blk_cnt = 6 + CAP_OUT_Q_DEPTH;
	mem_cfg.pool_info[0].blk_cnt *= frame_num;

	mem_cfg.pool_info[0].ddr_id = RAW_BUFFER_DDR_ID;
	#if 0
	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[1].blk_cnt = 3;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	#endif
	ret = hd_common_mem_init(&mem_cfg);
	*p_blk_size = mem_cfg.pool_info[0].blk_size;

	return ret;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	hd_common_mem_uninit();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT get_cap_sysinfo(HD_PATH_ID video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_SYSINFO sys_info = {0};

	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
	printf("sys_info.devid =0x%X, cur_fps[0]=%d/%d, vd_count=%llu\r\n", sys_info.dev_id, GET_HI_UINT16(sys_info.cur_fps[0]), GET_LO_UINT16(sys_info.cur_fps[0]), sys_info.vd_count);
	return ret;
}

static HD_RESULT set_cap_cfg(HD_PATH_ID *p_video_cap_ctrl, HD_OUT_ID _out_id, UINT32 *p_cap_pin_map, UINT32 cap_shdr_map, char *p_driver_name)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	#if USE_REAL_SENSOR
	HD_VIDEOCAP_CTRL iq_ctl = {0};
	UINT32 i;


	strncpy(cap_cfg.sen_cfg.sen_dev.driver_name, p_driver_name, (HD_VIDEOCAP_SEN_NAME_LEN - 1));
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x301;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0|PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	for (i=0; i < HD_VIDEOCAP_SEN_SER_MAX_DATALANE; i++) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[i] = *(p_cap_pin_map+i);
	}
	cap_cfg.sen_cfg.shdr_map = cap_shdr_map;
	#else
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, HD_VIDEOCAP_SEN_PAT_GEN);
	#endif
	ret = hd_videocap_open(0, _out_id, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	#if USE_REAL_SENSOR
	iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB;
	if (cap_shdr_map) {
		iq_ctl.func |= HD_VIDEOCAP_FUNC_SHDR;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);
	#endif
	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}

static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim, HD_VIDEOCAP_SEN_FRAME_NUM frame_num)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};
		#if USE_REAL_SENSOR
		video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
		video_in_param.frc = HD_VIDEO_FRC_RATIO(frame_rate,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		video_in_param.pxlfmt = SEN_OUT_FMT;
		video_in_param.out_frame_num = frame_num;
		#else
		video_in_param.sen_mode = HD_VIDEOCAP_PATGEN_MODE(HD_VIDEOCAP_SEN_PAT_COLORBAR, 200);
		video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		#endif
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN, &video_in_param);
		//printf("set_cap_param MODE=%d\r\n", ret);
		if (ret != HD_OK) {
			return ret;
		}
	}
	if (crop == 0) {//no crop, full frame
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_OFF;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &video_crop_param);
		//printf("set_cap_param CROP NONE=%d\r\n", ret);
		if (ret != HD_OK) {
			return ret;
		}
	} else {//HD_CROP_ON
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_ON;
		video_crop_param.win.rect.x = crop_x;
		video_crop_param.win.rect.y = crop_y;
		video_crop_param.win.rect.w = crop_w;
		video_crop_param.win.rect.h= crop_h;
		video_crop_param.align.w = 4;
		video_crop_param.align.h = 4;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &video_crop_param);
		//printf("set_cap_param CROP ON=%d\r\n", ret);
	}

	{
		HD_VIDEOCAP_OUT video_out_param = {0};

		//without setting dim for no scaling, using original sensor out size
		video_out_param.pxlfmt = CAP_OUT_FMT;
		if (frame_num > 1) {
			//just convert RAW to SHDR RAW format
			//e.g. HD_VIDEO_PXLFMT_RAW8 -> HD_VIDEO_PXLFMT_RAW8_SHDR2
			video_out_param.pxlfmt &=~ 0x0F000000;
			video_out_param.pxlfmt |= (frame_num << 24);
		}
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.depth = CAP_OUT_Q_DEPTH*frame_num;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
	}

	{
		HD_VIDEOCAP_FUNC_CONFIG video_path_param = {0};

		video_path_param.ddr_id = RAW_BUFFER_DDR_ID;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_FUNC_CONFIG, &video_path_param);
		//printf("set_cap_param PATH_CONFIG=0x%X\r\n", ret);
	}
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _VIDEO_LIVEVIEW {

	// (1)
	HD_VIDEOCAP_SYSCAPS cap_syscaps;
	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	HD_DIM  cap_dim;
	CHAR cap_drv_name[HD_VIDEOCAP_SEN_NAME_LEN];
	UINT32 cap_pin_map[HD_VIDEOCAP_SEN_SER_MAX_DATALANE];
	UINT32 cap_shdr_map;
	HD_VIDEOCAP_SEN_FRAME_NUM cap_frame_num;
	HD_DIM  proc_max_dim;
	// (4) user pull
	pthread_t  cap_thread_id;
	UINT32     cap_exit;
	UINT32     cap_snap;
	UINT32     flow_start;
	UINT32     blk_size;
	INT32    wait_ms;
	UINT32 	show_ret;
} VIDEO_LIVEVIEW;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	VENDOR_COMM_MAX_FREE_BLOCK max_free_block = {0};
	UINT32 i;

	if ((ret = hd_videocap_init()) != HD_OK)
		return ret;

	for (i = 0; i < MAX_DDR_NUM; i++) {
		max_free_block.ddr = i;
		if (HD_OK == vendor_common_mem_get(VENDOR_COMMON_MEM_ITEM_MAX_FREE_BLOCK_SIZE, &max_free_block)) {
			if (max_free_block.size) {
				if (HD_OK == hd_common_mem_alloc("RAW", &ddr_max_free[i].pa, (void **)&ddr_max_free[i].va, max_free_block.size, max_free_block.ddr)) {
					ddr_remain[i].pa = ddr_max_free[i].pa;
					ddr_remain[i].va = ddr_max_free[i].va;
					ddr_remain[i].size = ddr_max_free[i].size = max_free_block.size;
				}
				printf("DDR[%d] max free size = 0x%08X, pa=0x%X, va=0x%08X\r\n", i, max_free_block.size, ddr_remain[i].pa, (unsigned int)ddr_remain[i].va);
			}
		}
	}

	return HD_OK;
}

static HD_RESULT open_module(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	// set videocap config
	ret = set_cap_cfg(&p_stream->cap_ctrl, HD_VIDEOCAP_0_CTRL, p_stream->cap_pin_map, p_stream->cap_shdr_map, p_stream->cap_drv_name);
	if (ret != HD_OK) {
		printf("set cap-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT close_module(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;

	if ((ret = hd_videocap_close(p_stream->cap_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;

	if ((ret = hd_videocap_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}

///////////////////////////////////////////////////////////////////////////////
static void save_all_raw(UINT32 frame_num, UINT32 frame_size)
{
	char file_path_main[MAX_FRAME_NUM][64] = {0};
	FILE *f_out_main;
	UINT32 i,j;

	if (frame_size == 0)
		return;
	for (i = 0; i < frame_num; i++) {
		for (j = 0; j < idx[i]; j++) {
			if (print_frame_cnt) {
				snprintf(file_path_main[i], 63, "/mnt/sd/RAW%lu_w%d_h%d_12b_pack_%lu_%llu.raw", i, (int)crop_w, (int)crop_h, j, raw_info[i].vd_cnt[j]);
			} else {
				snprintf(file_path_main[i], 63, "/mnt/sd/RAW%lu_w%d_h%d_12b_pack_%lu.raw", i, (int)crop_w, (int)crop_h, j);
			}
			printf("%s\r\n", file_path_main[i]);
			//----- open output files -----
			if ((f_out_main = fopen(file_path_main[i], "wb")) == NULL) {
				printf("open file (%s) fail....\r\n\r\n", file_path_main[i]);
				continue;
			}
			if (f_out_main) {
				UINT8 *ptr = (UINT8 *)raw_info[i].va[j];

				fwrite(ptr, 1, frame_size, f_out_main);
				fflush(f_out_main);
			}
			// close output file
			fclose(f_out_main);
		}
	}
}
static void *cap_raw_thread(void *arg)
{
	VIDEO_LIVEVIEW *p_stream0 = (VIDEO_LIVEVIEW *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEO_FRAME video_frame[MAX_FRAME_NUM] = {0};

	UINT32 phy_addr_main[MAX_FRAME_NUM];
	UINT32 index, size = 0;
	UINT32 i;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;

	//------ wait flow_start ------
	while (p_stream0->flow_start == 0) sleep(1);

	// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
	//hd_videoenc_get(video_enc_path0, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);

	// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
	//vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);

	printf("\r\nif you want to capture raw, enter \"s\" to trigger !!\r\n");
	printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	//--------- pull data test ---------
	while (p_stream0->cap_exit == 0) {

		if(p_stream0->cap_snap) {
			//p_stream0->cap_snap = 0;
			for (i = 0; i < p_stream0->cap_frame_num; i++) {
				ret = hd_videocap_pull_out_buf(p_stream0->cap_path, &video_frame[i], p_stream0->wait_ms);// -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
				if (ret != HD_OK) {
					if(p_stream0->show_ret) printf("pull_out(%d) error = %d!!\r\n", p_stream0->wait_ms, ret);
					continue;
				}

				index = (video_frame[i].pxlfmt >> 12) & 0xF;
				if (index >= p_stream0->cap_frame_num) {
					printf("Something wrong!!! (0x%X)\r\n", video_frame[i].pxlfmt);
					continue;
				}
				if (whole_block) {
					phy_addr_main[index] = hd_common_mem_blk2pa(video_frame[i].blk); // Get physical addr
				} else {
					phy_addr_main[index] = video_frame[i].phy_addr[0];
				}
				if (phy_addr_main[index] == 0) {
					printf("phy_addr_main[%d]error !!\r\n\r\n", index);
					goto release_out;
				}
				raw_info[index].vd_cnt[idx[index]] = video_frame[i].count;

				if(p_stream0->show_ret) {
					if (i == 0) {
						printf("id   vd_cnt  fmt           resolution     lineoffset     PA             blk_size\r\n");
					}
					printf("[%d] %6d  0x08%X    %4dx%4d      %4d         0x%08X       %d\r\n", index,
																					(unsigned int)video_frame[i].count,
																					video_frame[i].pxlfmt,
																					video_frame[i].dim.w,
																					video_frame[i].dim.h,
																					video_frame[i].loff[0],
																					video_frame[i].phy_addr[0],
																					p_stream0->blk_size);
				}
			}
			if (whole_block) {
				size = p_stream0->blk_size;
			} else {
				size = video_frame[0].loff[0]*video_frame[0].ph[0];
			}
			//copy raw
			for (i = 0; i < p_stream0->cap_frame_num; i++) {
				ret = mem_alloc_remain(&raw_info[i].pa[idx[i]], (void **)&raw_info[i].va[idx[i]], size, ddr_id);
				if (ret != HD_OK) {
					printf("ddr[%d] full, idx=%d\r\n", ddr_id, idx[i]);
					ddr_id++;
					if (ddr_id < MAX_DDR_NUM) {
						ret = mem_alloc_remain(&raw_info[i].pa[idx[i]], (void **)&raw_info[i].va[idx[i]], size, ddr_id);
						if (ret != HD_OK) {
							//no DDR2
							p_stream0->cap_exit = 1;
							break;
						}
					} else {
						//printf("ddr[%d] full\r\n", ddr_id);
						p_stream0->cap_exit = 1;
						break;
					}
				}
				if(p_stream0->show_ret) {
					printf("raw_info[%d] idx[%d] pa=0x%X va=0x%X\r\n", i,idx[i], raw_info[i].pa[idx[i]], (UINT32)raw_info[i].va[idx[i]]);
				}
				if(hd_gfx_memcpy(raw_info[i].pa[idx[i]], phy_addr_main[i], size) == NULL){
					printf("hd_gfx_memcpy fail\n");
				}
				idx[i]++;
				if (idx[i] >= MAX_RAW_QUEUE) {
					p_stream0->cap_exit = 1;
					break;
				}
			}
release_out:
			for (i = 0; i < p_stream0->cap_frame_num; i++) {
				ret = hd_videocap_release_out_buf(p_stream0->cap_path, &video_frame[i]);
				if (ret != HD_OK) {
					printf("cap_release error !!\r\n\r\n");
				}
			}
		}
		usleep(500);
	}
	save_all_raw(p_stream0->cap_frame_num, size);
	return 0;
}

MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	VIDEO_LIVEVIEW stream[2] = {0}; //0: shdr main stream, 1: shdr sub stream
	UINT32 frame_num = 1;
	UINT32 i;
	UINT32 sensor_sel = 0;

	// query program options
	if (argc == 1 || argc > 10) {
		printf("Usage: <sensor_sel> <frm_num> <whole_block> <raw_enc> <show_ret> <crop_w> <crop_h> <fps>.\r\n");
		printf("Help:\r\n");
		printf("  <sensor_sel>    : 0(imx290), 1(os02k10)\r\n");
		printf("  <frm_num>       : frame number (> 0)\r\n");
		printf("  <whole_block>   : 0(disable), 1(enable)\r\n");
		printf("  <raw_enc>       : 0(disable), 1(enable)\r\n");
		printf("  <show_ret>      : 0(disable), 1(enable)\r\n");
		printf("  <crop_w>        : ex. 1920\r\n");
		printf("  <crop_h>        : ex. 1080\r\n");
		printf("  <fps>           : ex. 30\r\n");
		printf("  <print frm_cnt> : 0(disable), 1(enable)\r\n");
		return 0;
	}
    
	if (argc >= 2) {
		sensor_sel = atoi(argv[1]);
		printf("sensor_sel %d\r\n", sensor_sel);
	}
	if (argc >= 3) {
		frame_num = atoi(argv[2]);
		printf("frame_num %d\r\n", frame_num);
		if (frame_num > MAX_FRAME_NUM) {
			printf("only support max frame_num=%d\n", MAX_FRAME_NUM);
			goto exit;
		}
	}
	if (argc >= 4) {
		whole_block = atoi(argv[3]);
		printf("whole_block %d\r\n", whole_block);
	}

	if (argc >= 5) {
		raw_enc = atoi(argv[4]);
		printf("raw_enc %d\r\n", raw_enc);
	}

	if (argc >= 6) {
		stream[0].show_ret = atoi(argv[5]);
		printf("show_log %d\r\n", stream[0].show_ret);
	}

	if (argc >= 7) {
		crop_w = atoi(argv[6]);
		printf("CROP_W = %d\r\n", crop_w);
	}

	if (argc >= 8) {
		crop_h = atoi(argv[7]);
		printf("CROP_H = %d\r\n", crop_h);
	}

	if (argc >= 9) {
		frame_rate = atoi(argv[8]);
		printf("FrameRate = %d\r\n", frame_rate);
	}

	if (argc >= 10) {
		print_frame_cnt = atoi(argv[9]);
		printf("print_frame_cnt = %d\r\n", print_frame_cnt);
	}

	if((crop_w < VDO_SIZE_W)&&(crop_h < VDO_SIZE_H)) {
		crop = TRUE;
	} else {
		crop = FALSE;
		crop_w = VDO_SIZE_W;
		crop_h = VDO_SIZE_H;
	}

	crop_x = ((VDO_SIZE_W - crop_w)>>1);
	crop_y = ((VDO_SIZE_H - crop_h)>>1);

	printf("crop = %d, crop_xy = {%d, %d}, crop_size = {%d, %d}\r\n", crop, crop_x, crop_y, crop_w, crop_h);

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
	}

	// init memory
	ret = mem_init(frame_num, &stream[0].blk_size);
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		goto exit;
	}
	ret = hd_gfx_init();
	if(ret != HD_OK) {
        printf("init gfx fail=%d\n", ret);
        goto exit;
    }
	// init all modules
	ret = init_module();
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}


	//pull with blocking
	stream[0].wait_ms = -1;



	// open video liview modules (main)
	for (i = 0; i < HD_VIDEOCAP_SEN_SER_MAX_DATALANE; i++) {
		stream[0].cap_pin_map[i] = HD_VIDEOCAP_SEN_IGNORE;
	}
	stream[0].cap_pin_map[0] = 0;
	stream[0].cap_pin_map[1] = 1;

	if(sensor_sel == 0) {
		snprintf(stream[0].cap_drv_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
	} else if(sensor_sel == 1) {
		snprintf(stream[0].cap_drv_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os02k10");
	} else {
		printf("sensor is not support!\r\n");
	}
	
	if (frame_num == HD_VIDEOCAP_SEN_FRAME_NUM_1) {
		stream[0].cap_shdr_map = 0;
	} else if (frame_num == HD_VIDEOCAP_SEN_FRAME_NUM_2) {
		stream[0].cap_pin_map[2] = 2;
		stream[0].cap_pin_map[3] = 3;
		stream[0].cap_shdr_map = HD_VIDEOCAP_SHDR_MAP(HD_VIDEOCAP_HDR_SENSOR1, (HD_VIDEOCAP_0|HD_VIDEOCAP_1));
	}
	ret = open_module(&stream[0]);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	// set videocap parameter (main)
	stream[0].cap_dim.w = VDO_SIZE_W; //assign by user
	stream[0].cap_dim.h = VDO_SIZE_H; //assign by user
	stream[0].cap_frame_num = frame_num;
	ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim, stream[0].cap_frame_num);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}


	ret = pthread_create(&stream[0].cap_thread_id, NULL, cap_raw_thread, (void *)stream);
	if (ret < 0) {
		printf("create encode thread failed");
		return -1;
	}

	// start capture modules (main)
	ret = hd_videocap_start(stream[0].cap_path);
	if (ret != HD_OK) {
		printf("start fail=%d\n", ret);
		goto exit;
	}

	// let cap_raw_thread start to work
	stream[0].flow_start = 1;

	printf("Enter q to exit\n");
	while (1) {
		key = GETCHAR();
		if (key == 's') {
			stream[0].cap_snap = 1;
		}
		if (key == 'q' || key == 0x3) {
			// quit program
			stream[0].cap_exit = 1;
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

	// stop capture modules (main)
	hd_videocap_stop(stream[0].cap_path);

	// destroy capture thread
	pthread_join(stream[0].cap_thread_id, NULL);


	//free hdal malloc
	for (i = 0; i < MAX_DDR_NUM; i++) {
		ret = hd_common_mem_free(ddr_max_free[i].pa, ddr_max_free[i].va);
		if (ret != HD_OK) {
			printf("err:free pa = 0x%x, va = 0x%x\r\n", (unsigned int)(ddr_max_free[i].pa), (unsigned int)(ddr_max_free[i].va));
		}
	}
exit:
	// close capture modules (main)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

	// uninit all modules
	ret = exit_module();
	if (ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}

	ret = hd_gfx_uninit();
	if(ret != HD_OK)
		printf("uninit fail=%d\n", ret);

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
