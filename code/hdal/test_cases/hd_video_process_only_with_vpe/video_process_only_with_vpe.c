/**
	@brief Sample code of videoprocess with vpe.\n

	@file video_process_only_with_vpe.c

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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_video_process_only, argc, argv)
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

#define VDO_SIZE_W         1920
#define VDO_SIZE_H         1080

#define VPE_DEV_CNT    2
#define YUV_BLK_SIZE       (DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420))

#define VDO_DDR_ID		DDR_ID0

#define OUT_VPE_COUNT 2

#define MAX_SCL_RATIO_TEST 4


#define OUT0_BG_W ALIGN_FLOOR(VDO_SIZE_W, 16)
#define OUT0_BG_H ALIGN_FLOOR(VDO_SIZE_H, 2)

#define VPE0_OUT0_WIN_X ALIGN_FLOOR(80, 4)
#define VPE0_OUT0_WIN_Y ALIGN_FLOOR(60, 2)
#define VPE0_OUT0_WIN_W ALIGN_FLOOR((OUT0_BG_W/2), 8)
#define VPE0_OUT0_WIN_H ALIGN_FLOOR((OUT0_BG_H/2), 2)

#define VPE1_OUT0_WIN_X ALIGN_FLOOR((VPE0_OUT0_WIN_X+VPE0_OUT0_WIN_W+80), 4)
#define VPE1_OUT0_WIN_Y ALIGN_FLOOR((VPE0_OUT0_WIN_Y+VPE0_OUT0_WIN_H+60), 2)
#define VPE1_OUT0_WIN_W ALIGN_FLOOR((OUT0_BG_W/4), 8)
#define VPE1_OUT0_WIN_H ALIGN_FLOOR((OUT0_BG_H/4), 2)


#define OUT1_BG_W ALIGN_FLOOR(1280, 16)
#define OUT1_BG_H ALIGN_FLOOR(720, 2)

#define VPE0_OUT1_WIN_X ALIGN_FLOOR(40, 4)
#define VPE0_OUT1_WIN_Y ALIGN_FLOOR(30, 2)
#define VPE0_OUT1_WIN_W ALIGN_FLOOR((OUT1_BG_W/2), 8)
#define VPE0_OUT1_WIN_H ALIGN_FLOOR((OUT1_BG_H/2), 2)

#define VPE1_OUT1_WIN_X ALIGN_FLOOR((VPE0_OUT1_WIN_X+VPE0_OUT1_WIN_W+40), 4)
#define VPE1_OUT1_WIN_Y ALIGN_FLOOR((VPE0_OUT1_WIN_Y+VPE0_OUT1_WIN_H+30), 2)
#define VPE1_OUT1_WIN_W ALIGN_FLOOR((OUT1_BG_W/4), 8)
#define VPE1_OUT1_WIN_H ALIGN_FLOOR((OUT1_BG_H/4), 2)

#define TEST_PATTERN_PATH "/mnt/sd/video_frm_1920_1080_2_yuv420.dat"



#define VPE0_OUT0_INCROP_WIN_X ALIGN_FLOOR(10, 2)
#define VPE0_OUT0_INCROP_WIN_Y ALIGN_FLOOR(10, 2)
#define VPE0_OUT0_INCROP_WIN_W ALIGN_FLOOR((VDO_SIZE_W/3), 8)
#define VPE0_OUT0_INCROP_WIN_H ALIGN_FLOOR((VDO_SIZE_H/3), 2)

#define VPE0_OUT0_OUTCROP_WIN_X ALIGN_FLOOR(20, 4)
#define VPE0_OUT0_OUTCROP_WIN_Y ALIGN_FLOOR(40, 2)
#define VPE0_OUT0_OUTCROP_WIN_W ALIGN_FLOOR(VPE0_OUT0_WIN_W - 60, 4)
#define VPE0_OUT0_OUTCROP_WIN_H ALIGN_FLOOR(VPE0_OUT0_WIN_H - 100, 2)

#define VPE_HOLE_TEST 0
//the coordinate is based on the origin of output rect, not background
#define VPE_OUT0_HOLE_WIN_X ALIGN_FLOOR(VPE0_OUT1_WIN_X, 4)
#define VPE_OUT0_HOLE_WIN_Y ALIGN_FLOOR(VPE0_OUT1_WIN_Y, 2)
#define VPE_OUT0_HOLE_WIN_W ALIGN_FLOOR(240, 4)
#define VPE_OUT0_HOLE_WIN_H ALIGN_FLOOR(240, 2)
#define VPE_OUT1_HOLE_WIN_X ALIGN_FLOOR(0, 4)
#define VPE_OUT1_HOLE_WIN_Y ALIGN_FLOOR(0, 2)
#define VPE_OUT1_HOLE_WIN_W ALIGN_FLOOR(80, 4)
#define VPE_OUT1_HOLE_WIN_H ALIGN_FLOOR(80, 2)

///////////////////////////////////////////////////////////////////////////////

typedef struct _VIDEO_PROCESS {
	UINT32 id;
	HD_DIM  proc_max_dim;
	HD_VIDEOPROC_SYSCAPS proc_syscaps;
	HD_PATH_ID proc_ctrl;
	HD_PATH_ID proc_path[OUT_VPE_COUNT];
	HD_VIDEOPROC_OUT proc_out[OUT_VPE_COUNT];
	UINT32 	shot_count;
} VIDEO_PROCESS;

///////////////////////////////////////////////////////////////////////////////


static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (main)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[0].blk_cnt = VPE_DEV_CNT + VPE_DEV_CNT*OUT_VPE_COUNT + OUT_VPE_COUNT;//input buffer + pull depth buffer + output buffer
	mem_cfg.pool_info[0].ddr_id = VDO_DDR_ID;

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
static HD_RESULT set_proc_cfg(HD_PATH_ID video_proc_ctrl, HD_DIM* p_max_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};

	if (p_max_dim != NULL ) {
		video_cfg_param.pipe = 0xf2;//HD_VIDEOPROC_PIPE_VPE
		video_cfg_param.isp_id = HD_ISP_DONT_CARE; //0
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
		video_path_param.ddr_id = VDO_DDR_ID;
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
	}

	video_ctrl_param.func = 0;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);

	return ret;
}

static HD_RESULT set_proc_param(VIDEO_PROCESS *p_stream, UINT32 out_id)
{
	HD_RESULT ret = HD_OK;
	//dim MUST be zero for specified output region
	p_stream->proc_out[out_id].dim.w = 0;
	p_stream->proc_out[out_id].dim.h = 0;
	p_stream->proc_out[out_id].func = 0;
	p_stream->proc_out[out_id].pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	p_stream->proc_out[out_id].dir = HD_VIDEO_DIR_NONE;
	p_stream->proc_out[out_id].frc = HD_VIDEO_FRC_RATIO(1,1);
	p_stream->proc_out[out_id].depth = 1; //set 1 to allow pull

	ret = hd_videoproc_set(p_stream->proc_path[out_id], HD_VIDEOPROC_PARAM_OUT, &p_stream->proc_out[out_id]);

	return ret;
}
static HD_RESULT set_proc_crop_param(VIDEO_PROCESS *p_stream, UINT32 out_id, HD_IRECT* p_pre_crop, HD_IRECT* p_post_crop)
{
	HD_RESULT ret = HD_OK;

	if (p_pre_crop) {
		HD_VIDEOPROC_CROP  video_crop_param = {0};

		video_crop_param.mode  = HD_CROP_ON;
		video_crop_param.win.rect.x = p_pre_crop->x;
		video_crop_param.win.rect.y = p_pre_crop->y;
		video_crop_param.win.rect.w = p_pre_crop->w;
		video_crop_param.win.rect.h = p_pre_crop->h;
		ret |= hd_videoproc_set(p_stream->proc_path[out_id], HD_VIDEOPROC_PARAM_IN_CROP_PSR, &video_crop_param);
	}
	if (p_post_crop) {
		HD_VIDEOPROC_CROP  video_crop_param = {0};

		video_crop_param.mode  = HD_CROP_ON;
		video_crop_param.win.rect.x = p_post_crop->x;
		video_crop_param.win.rect.y = p_post_crop->y;
		video_crop_param.win.rect.w = p_post_crop->w;
		video_crop_param.win.rect.h = p_post_crop->h;
		ret |= hd_videoproc_set(p_stream->proc_path[out_id], HD_VIDEOPROC_PARAM_OUT_CROP, &video_crop_param);
	}

	return ret;
}

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module(VIDEO_PROCESS *p_stream, HD_DIM* p_proc_max_dim)
{
    HD_RESULT ret;
	UINT32 i;

	ret = hd_videoproc_open(0, HD_VIDEOPROC_0_CTRL, &p_stream->proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	// set videoproc config
	ret = set_proc_cfg(p_stream->proc_ctrl, p_proc_max_dim);
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}

	for (i=0; i < OUT_VPE_COUNT; i++) {
		if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_OUT(0, i), &p_stream->proc_path[i])) != HD_OK)
			return ret;
	}
	return HD_OK;
}
static HD_RESULT open_module2(VIDEO_PROCESS *p_stream, HD_DIM* p_proc_max_dim)
{
    HD_RESULT ret;
	UINT32 i;

	ret = hd_videoproc_open(0, HD_VIDEOPROC_1_CTRL, &p_stream->proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	// set videoproc config
	ret = set_proc_cfg(p_stream->proc_ctrl, p_proc_max_dim);
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}

	for (i=0; i < OUT_VPE_COUNT; i++) {
		if ((ret = hd_videoproc_open(HD_VIDEOPROC_1_IN_0, HD_VIDEOPROC_OUT(1, i), &p_stream->proc_path[i])) != HD_OK)
			return ret;
	}
	return HD_OK;
}


static HD_RESULT close_module(VIDEO_PROCESS *p_stream)
{
	HD_RESULT ret;
	UINT32 i;

	for (i = 0; i < OUT_VPE_COUNT; i++) {
		if ((ret = hd_videoproc_close(p_stream->proc_path[i])) != HD_OK)
			return ret;
	}
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}

static BOOL check_test_pattern(void)
{
	FILE *f_in;
	char filepath[128];

	sprintf(filepath, TEST_PATTERN_PATH);

	if ((f_in = fopen(filepath, "rb")) == NULL) {
		printf("fail to open %s\n", filepath);
		printf("Test pattern is in SDK/code/hdal/samples/pattern/\n");
		return FALSE;
	}

	fclose(f_in);
	return TRUE;
}

static void trigger(VIDEO_PROCESS *p_stream, VIDEO_PROCESS *p_stream2)
{
	HD_RESULT ret = HD_OK;
	UINT32 i, j;
	HD_COMMON_MEM_DDR_ID ddr_id = VDO_DDR_ID;
	UINT32 blk_size = YUV_BLK_SIZE;
	UINT32 yuv_size = VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	char filepath_yuv_main[128];
	FILE *f_in_main;
	UINT32 pa_yuv_main[VPE_DEV_CNT] = {0};
	UINT32 va_yuv_main[VPE_DEV_CNT] = {0};
	HD_COMMON_MEM_VB_BLK in_blk[2];
	HD_COMMON_MEM_VB_BLK out_blk[OUT_VPE_COUNT] = {0};
	UINT32 out_va[OUT_VPE_COUNT] = {0};
	UINT32 out_pa[OUT_VPE_COUNT] = {0};
	UINT32 out_blk_size[OUT_VPE_COUNT] = {0};
	UINT32 read_len;
	HD_VIDEO_FRAME out_video_frame[OUT_VPE_COUNT];
	VIDEO_PROCESS * stream[2];
	#define PHY2VIRT_MAIN(pa, i) (out_va[i] + ((pa) - out_pa[i]))

	stream[0] = p_stream;
	stream[1] = p_stream2;


	sprintf(filepath_yuv_main, TEST_PATTERN_PATH);
	if ((f_in_main = fopen(filepath_yuv_main, "rb")) == NULL) {
		printf("open file (%s) fail !!....\r\nPlease copy test pattern to SD Card !!\r\n\r\n", filepath_yuv_main);
		return;
	}
	//--- Get memory for input image buffer---
	for (i = 0; i < VPE_DEV_CNT; i++) {
get_in_blk:
		in_blk[i] = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id); // Get block from mem pool
		if (in_blk[i] == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get block[%d] fail (0x%x).. try again later.....\r\n", i, in_blk[i]);
			sleep(1);
			goto get_in_blk;
		}

		pa_yuv_main[i] = hd_common_mem_blk2pa(in_blk[i]); // Get physical addr
		if (pa_yuv_main[i] == 0) {
			printf("blk2pa fail, in_blk[%d] = 0x%x\r\n", i, in_blk[i]);
			fclose(f_in_main);
			goto rel_in_blk;
		}
		va_yuv_main[i] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa_yuv_main[i], blk_size); // Get virtual addr
		if (va_yuv_main[i] == 0) {
			printf("Error: mmap fail !! pa_yuv_main[%d], in_blk[%d] = 0x%x\r\n", i, i, in_blk[i]);
			fclose(f_in_main);
			goto rel_in_blk;
		}
		printf("\r\n@@IN[%d] BLK=0x%X, PA=0x%X, VA=0x%X@@\r\n", i, in_blk[i], pa_yuv_main[i], va_yuv_main[i]);
		//--- Read YUV from file ---
		read_len = fread((void *)va_yuv_main[i], 1, yuv_size, f_in_main);
		if (read_len != yuv_size) {
			printf("reading len error\n");
			goto quit;
		}
		//--- data is written by CPU, flush CPU cache to PHY memory ---
		hd_common_mem_flush_cache((void *)va_yuv_main[i], yuv_size);
	}
	// close input image file
	fclose(f_in_main);

	#if 0 //debug for pattern check
	for (i = 0; i < VPE_DEV_CNT; i++) {
		//save output buffer
		char file_path[128] = {0};
		FILE *f_pattern;
		snprintf(file_path, 128, "/mnt/sd/input_pattern[%lu].dat", i);

		printf("Save pattern to (%s) ....\r\n", file_path);
		if ((f_pattern = fopen(file_path, "wb")) == NULL) {
			printf("open file (%s) fail....\r\n\r\n", file_path);
    			goto rel_in_blk;
		}
		{
			if (f_pattern) fwrite((void *)va_yuv_main[i], 1, yuv_size, f_pattern);
			if (f_pattern) fflush(f_pattern);
		}

		fclose(f_pattern);
	}
	#endif

	//-- prepare output block
	for (i = 0; i < OUT_VPE_COUNT; i++) {
		UINT32 pa;
		if (!p_stream->proc_out[i].bg.w || !p_stream->proc_out[i].bg.h) {
			printf("out[%d] bg size zero!\r\n", i);
			continue;
		}
		out_blk_size[i] = (DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(p_stream->proc_out[i].bg.w, p_stream->proc_out[i].bg.h, HD_VIDEO_PXLFMT_YUV420));
get_out_blk:
		//--- Get memory ---
		out_blk[i] = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, out_blk_size[i], ddr_id); // Get block from mem pool
		if (out_blk[i] == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get out_blk[%d] fail (0x%x).. try again later.....\r\n", i, out_blk[i]);
			sleep(1);
			//continue;
			goto get_out_blk;
		}
		pa = hd_common_mem_blk2pa(out_blk[i]);
		printf("\r\n##OUT[%d] BLK=0x%X, PA=0x%X, SIZE=0x%X##\r\n", i, out_blk[i], pa, out_blk_size[i]);

		if (pa == 0) {
			printf("hd_common_mem_blk2pa error !!\r\n\r\n");
		} else {
			out_va[i] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, out_blk_size[i]); // Get virtual addr
			if (out_va[i]) {
				//clear buffer
				//Do NOT use this method to clear buffer in real procuct!
				memset((void *)out_va[i], 0, out_blk_size[i]);
				hd_common_mem_flush_cache((void *)out_va[i], out_blk_size[i]);
			} else {
				printf("Error: mmap fail !! pa=0x%X, blk=0x%x\r\n", pa, out_blk[i]);
			}
		}
	}

	//--- push_in ---
	for (i = 0; i < VPE_DEV_CNT; i++) {
		HD_VIDEO_FRAME video_frame = {0};

		//set input image buffer
		video_frame.sign        = MAKEFOURCC('V','F','R','M');
		//video_frame.p_next      = NULL;
		video_frame.ddr_id      = ddr_id;
		video_frame.pxlfmt      = HD_VIDEO_PXLFMT_YUV420;
		video_frame.dim.w       = VDO_SIZE_W;
		video_frame.dim.h       = VDO_SIZE_H;
		video_frame.count       = 0;
		video_frame.timestamp   = hd_gettime_us();
		video_frame.pw[0]       = VDO_SIZE_W; // Y
		video_frame.ph[0]       = VDO_SIZE_H; // UV
		video_frame.pw[1]       = VDO_SIZE_W >> 1; // Y
		video_frame.ph[1]       = VDO_SIZE_H >> 1; // UV
		video_frame.loff[0]     = ALIGN_CEIL_4(video_frame.pw[0]); // Y
		video_frame.loff[1]     = video_frame.loff[0]; // UV
		video_frame.phy_addr[0] = pa_yuv_main[i];                          // Y
		video_frame.phy_addr[1] = pa_yuv_main[i] + video_frame.loff[0]*video_frame.ph[0];  // UV
		video_frame.blk         = in_blk[i];

		//set user specified output buffer
		memset((void *)out_video_frame, 0, sizeof(out_video_frame));
		for (j = 0; j < OUT_VPE_COUNT; j++) {
			out_video_frame[j].sign = MAKEFOURCC('V','F','R','M');
			out_video_frame[j].blk = out_blk[j];
			out_video_frame[j].reserved[0] =  out_blk_size[j];
			if (j < (OUT_VPE_COUNT-1)) {
				out_video_frame[j].p_next = (HD_METADATA *)&out_video_frame[j+1];
			}
		}

		ret = hd_videoproc_push_in_buf(stream[i]->proc_path[0], &video_frame, &out_video_frame[0], 0); // only support non-blocking mode
		if (ret != HD_OK) {
			printf("push_in(%d) error = %d!!\r\n", 0, ret);
			break;
		}
		memset((void *)out_video_frame, 0, sizeof(out_video_frame));
		//wait output finish
		for (j = 0; j < OUT_VPE_COUNT; j++) {
			ret = hd_videoproc_pull_out_buf(stream[i]->proc_path[j], &out_video_frame[j], -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
			if (ret != HD_OK) {
				printf("pull_out[%d] error = %d!! blk=0x%X\r\n", i, ret, out_video_frame[j].blk);
				if (ret == HD_ERR_BAD_DATA) {
					printf("VPE[%d]-OUT[%d] blk=0x%X\r\n", i, j, out_video_frame[j].blk);
				}
				continue;
			}
			printf("VPE[%d]-OUT[%d] phy_addr_Y=0x%X, phy_addr_UV=0x%X\r\n", i, j, out_video_frame[j].phy_addr[0], out_video_frame[j].phy_addr[1]);
			ret = hd_videoproc_release_out_buf(stream[i]->proc_path[j], &out_video_frame[j]);
			if (ret != HD_OK) {
				printf("release_out[%d] error !!\r\n\r\n",j);
			}
		}
	}

	//save output buffer
	char file_path_main[OUT_VPE_COUNT][128] = {0};
	FILE *f_out_main[OUT_VPE_COUNT];
	for (i = 0; i < OUT_VPE_COUNT; i++) {
		out_pa[i] = hd_common_mem_blk2pa(out_blk[i]);
		if (out_pa[i] == 0) {
			printf("blk2pa fail, blk = 0x%x\r\n", out_blk[i]);
		}
		snprintf(file_path_main[i], 128, "/mnt/sd/vpe_out[%lu]_%lu_%lu_%08x_%lu.dat",
								i, stream[0]->proc_out[i].bg.w, stream[0]->proc_out[i].bg.h,
								(unsigned int)stream[0]->proc_out[i].pxlfmt, stream[0]->shot_count);
		//printf("phy_addr_Y=0x%X, phy_addr_UV=0x%X\r\n", out_video_frame[i].phy_addr[0], out_video_frame[i].phy_addr[1]);
		printf("Save to (%s) ....\r\n", file_path_main[i]);
		if ((f_out_main[i] = fopen(file_path_main[i], "wb")) == NULL) {
			printf("open file (%s) fail....\r\n\r\n", file_path_main[i]);
			fclose(f_out_main[i]);
   			goto rel_in_blk;
		}
		//save Y plane
		{
			UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(out_video_frame[i].phy_addr[0], i);
			UINT32 len = out_video_frame[i].loff[0]*out_video_frame[i].ph[0];
			if (f_out_main[i]) fwrite(ptr, 1, len, f_out_main[i]);
			if (f_out_main[i]) fflush(f_out_main[i]);
		}
		//save UV plane
		{
			UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(out_video_frame[i].phy_addr[1], i);
			UINT32 len = out_video_frame[i].loff[1]*out_video_frame[i].ph[1];
			if (f_out_main[i]) fwrite(ptr, 1, len, f_out_main[i]);
			if (f_out_main[i]) fflush(f_out_main[i]);
		}
		ret = hd_common_mem_munmap((void *)out_va[i], out_blk_size[i]);
		if (ret != HD_OK) {
			printf("mnumap error !!\r\n\r\n");
			fclose(f_out_main[i]);
   			goto rel_in_blk;
		}
		fclose(f_out_main[i]);
	}
	stream[0]->shot_count++;
	//release user output buffer
	for (i = 0; i < OUT_VPE_COUNT; i++) {
		ret = hd_common_mem_release_block(out_blk[i]);
		if (ret != HD_OK) {
			printf("_mem_release error !!\r\n\r\n");
		}
	}


rel_in_blk:
	//--- Release memory ---
	for (i = 0; i < VPE_DEV_CNT; i++) {
		ret = hd_common_mem_release_block(in_blk[i]);
		if (ret != HD_OK) {
			printf("_mem_release error !!\r\n\r\n");
		}
	}
quit:
	// mummap for input yuv buffer
	for (i=0; i< VPE_DEV_CNT; i++) {
		if (va_yuv_main[i] != 0) {
			ret = hd_common_mem_munmap((void *)va_yuv_main[i], blk_size);
			if (ret != HD_OK) {
				printf("mnumap error !!\r\n\r\n");
			}
		}
	}
}

MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	VIDEO_PROCESS stream[2] = {0}; //0: main stream
	UINT32 i;

	// check TEST pattern exist
	if (check_test_pattern() == FALSE) {
		printf("test_pattern isn't exist\r\n");
		exit(0);
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


	for (i = 0; i < OUT_VPE_COUNT; i++) {
		stream[i].id = i;
	}
	// open video_process modules (1st)
	stream[0].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[0].proc_max_dim.h = VDO_SIZE_H; //assign by user
	ret = open_module(&stream[0], &stream[0].proc_max_dim);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	// open video_process modules (2nd)
	stream[1].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[1].proc_max_dim.h = VDO_SIZE_H; //assign by user
	ret = open_module2(&stream[1], &stream[1].proc_max_dim);
	if (ret != HD_OK) {
		printf("open2 fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (1st)
	stream[0].proc_out[0].bg.w = OUT0_BG_W;
	stream[0].proc_out[0].bg.h = OUT0_BG_H;
	stream[0].proc_out[0].rect.x = VPE0_OUT0_WIN_X;
	stream[0].proc_out[0].rect.y = VPE0_OUT0_WIN_Y;
	stream[0].proc_out[0].rect.w = VPE0_OUT0_WIN_W;
	stream[0].proc_out[0].rect.h = VPE0_OUT0_WIN_H;
	ret = set_proc_param(&stream[0], 0);
	if (ret != HD_OK) {
		printf("set proc0-0 fail=%d\n", ret);
		goto exit;
	}
	stream[0].proc_out[1].bg.w = OUT1_BG_W;
	stream[0].proc_out[1].bg.h = OUT1_BG_H;
	stream[0].proc_out[1].rect.x = VPE0_OUT1_WIN_X;
	stream[0].proc_out[1].rect.y = VPE0_OUT1_WIN_Y;
	stream[0].proc_out[1].rect.w = VPE0_OUT1_WIN_W;
	stream[0].proc_out[1].rect.h = VPE0_OUT1_WIN_H;
	ret = set_proc_param(&stream[0], 1);
	if (ret != HD_OK) {
		printf("set proc0-1 fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (2nd)
	stream[1].proc_out[0].bg.w = OUT0_BG_W;
	stream[1].proc_out[0].bg.h = OUT0_BG_H;
	stream[1].proc_out[0].rect.x = VPE1_OUT0_WIN_X;
	stream[1].proc_out[0].rect.y = VPE1_OUT0_WIN_Y;
	stream[1].proc_out[0].rect.w = VPE1_OUT0_WIN_W;
	stream[1].proc_out[0].rect.h = VPE1_OUT0_WIN_H;
	ret = set_proc_param(&stream[1], 0);
	if (ret != HD_OK) {
		printf("set proc1-0 fail=%d\n", ret);
		goto exit;
	}
	stream[1].proc_out[1].bg.w = OUT1_BG_W;
	stream[1].proc_out[1].bg.h = OUT1_BG_H;
	stream[1].proc_out[1].rect.x = VPE1_OUT1_WIN_X;
	stream[1].proc_out[1].rect.y = VPE1_OUT1_WIN_Y;
	stream[1].proc_out[1].rect.w = VPE1_OUT1_WIN_W;
	stream[1].proc_out[1].rect.h = VPE1_OUT1_WIN_H;
	ret = set_proc_param(&stream[1], 1);
	if (ret != HD_OK) {
		printf("set proc1-1 fail=%d\n", ret);
		goto exit;
	}

	#if VPE_HOLE_TEST
	{
		HD_VIDEOPROC_CROP  video_crop_param = {0};

		//the coordinate is based on the origin of output rect, not background
		video_crop_param.mode  = HD_CROP_ON;
		video_crop_param.win.rect.x = VPE_OUT0_HOLE_WIN_X;
		video_crop_param.win.rect.y = VPE_OUT0_HOLE_WIN_Y;
		video_crop_param.win.rect.w = VPE_OUT0_HOLE_WIN_W;
		video_crop_param.win.rect.h = VPE_OUT0_HOLE_WIN_H;
		hd_videoproc_set(stream[0].proc_path[0], HD_VIDEOPROC_PARAM_OUT_CROP_PSR, &video_crop_param);

		video_crop_param.mode  = HD_CROP_ON;
		video_crop_param.win.rect.x = VPE_OUT1_HOLE_WIN_X;
		video_crop_param.win.rect.y = VPE_OUT1_HOLE_WIN_Y;
		video_crop_param.win.rect.w = VPE_OUT1_HOLE_WIN_W;
		video_crop_param.win.rect.h = VPE_OUT1_HOLE_WIN_H;
		hd_videoproc_set(stream[1].proc_path[0], HD_VIDEOPROC_PARAM_OUT_CROP_PSR, &video_crop_param);
	}
	#endif

	for (i = 0; i < OUT_VPE_COUNT; i++) {
		// start video_process modules
		hd_videoproc_start(stream[0].proc_path[i]);
		hd_videoproc_start(stream[1].proc_path[i]);
	}


	// query user key
	printf("Enter q to exit, s to trigger\n");
	while (1) {
		key = GETCHAR();
		if (key == 's') {
			trigger(&stream[0], &stream[1]);
		}
		if (key == 'q' || key == 0x3) {
			break;
		}
		if (key == '1') {
			HD_IRECT pre_crop;
			pre_crop.x = VPE0_OUT0_INCROP_WIN_X;
			pre_crop.y = VPE0_OUT0_INCROP_WIN_Y;
			pre_crop.w = VPE0_OUT0_INCROP_WIN_W;
			pre_crop.h = VPE0_OUT0_INCROP_WIN_H;
			ret = set_proc_crop_param(&stream[0], 0, &pre_crop, NULL);
			hd_videoproc_start(stream[0].proc_path[0]);
		}
		if (key == '2') {
			HD_IRECT post_crop;
			post_crop.x = VPE0_OUT0_OUTCROP_WIN_X;
			post_crop.y = VPE0_OUT0_OUTCROP_WIN_Y;
			post_crop.w = VPE0_OUT0_OUTCROP_WIN_W;
			post_crop.h = VPE0_OUT0_OUTCROP_WIN_H;
			ret = set_proc_crop_param(&stream[0], 0, NULL, &post_crop);
			hd_videoproc_start(stream[0].proc_path[0]);
		}
		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			// enter debug menu
			hd_debug_run_menu();
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
	}

	for (i = 0; i < OUT_VPE_COUNT; i++) {
		hd_videoproc_stop(stream[0].proc_path[i]); // it could force abort pull_out with blocking mode
		hd_videoproc_stop(stream[1].proc_path[i]); // it could force abort pull_out with blocking mode
	}


exit:

	// close video_process modules
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}
	ret = close_module(&stream[1]);
	if (ret != HD_OK) {
		printf("close 2 fail=%d\n", ret);
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
