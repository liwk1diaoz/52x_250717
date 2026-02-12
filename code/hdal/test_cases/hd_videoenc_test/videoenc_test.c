/**
	@brief Sample code of video encode test.\n

	@file videoen_test.c

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
#include <pthread.h>
#include "hdal.h"
#include "hd_util.h"
#include "hd_debug.h"
#include <kwrap/examsys.h>

#if defined(__LINUX)
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#include <kwrap/task.h>
#include <kwrap/util.h>
#define sleep(x)    vos_task_delay_ms(1000*x)
#define usleep(x)   vos_task_delay_us(x)
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

#define YUV_BLK_SIZE(w, h, pxlfmt)      (DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(w, h, pxlfmt))

#define VDO_MAX_SIZE_W     3840
#define VDO_MAX_SIZE_H     2160

#define MAX_YUV_BLK_CNT    3

typedef struct {
	UINT32 width;
	UINT32 height;
	UINT32 frame_cnt;
} PATTERN_FILE_ITEM;

static PATTERN_FILE_ITEM g_PatternFileTable[] = {
	{  352,  288,   30 },   // keep 352x288 in first choice as default
//	{ 3840, 2160,   10 },
	{ 1920, 1080,    5 },
//	{ 1280,  720,   90 },
//	{  720,  480,  300 },
//	{   16,   16,    1 },
};

#define PATTERN_FILE_NUM  (sizeof(g_PatternFileTable)/sizeof(PATTERN_FILE_ITEM))

#define MAX_LEN_TEST_NAME  128

typedef struct _VIDEO_RECORD {
	// (1)
	HD_DIM  enc_max_dim;
	HD_DIM  enc_dim;
	UINT32  pattern_idx;

	// (2)
	HD_VIDEOENC_SYSCAPS enc_syscaps;
	HD_PATH_ID enc_path;

	// (3) yuv pushin & user pull
	pthread_t  yuv_thread_id;
	pthread_t  enc_thread_id;
	UINT32     yuv_exit;
	UINT32     enc_exit;
	UINT32     flow_start;

	UINT32     file_sn;
	char       test_name[MAX_LEN_TEST_NAME];

} VIDEO_RECORD;

static VIDEO_RECORD stream[1] = {0}; //0: main stream

#define SET_TEST_NAME(name)  \
	{ \
		strncpy(stream[0].test_name, name, MAX_LEN_TEST_NAME-1); \
		stream[0].test_name[MAX_LEN_TEST_NAME-1] = '\0'; \
	}

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
	int      (*p_func)(void);  ///< command function
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
/*
static const char* hd_debug_get_menu_name(void)
{
	if (_cur_menu == NULL) return "x";
	else return _cur_menu->p_name;
}
*/
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

//////////////////////////////////////////////////////////////////////////////


static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	mem_cfg.pool_info[0].type     = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = YUV_BLK_SIZE(VDO_MAX_SIZE_W, VDO_MAX_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[0].blk_cnt  = MAX_YUV_BLK_CNT;
	mem_cfg.pool_info[0].ddr_id   = DDR_ID0;

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
		video_path_config.max_mem.rotate     = TRUE;
		video_path_config.max_mem.source_output   = TRUE;
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

		printf("enc_type=%d\r\n", enc_type);

		if (enc_type == 0) {

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

static HD_RESULT init_module(void)
{
    HD_RESULT ret;
    if((ret = hd_videoenc_init()) != HD_OK)
        return ret;
    return HD_OK;
}

static HD_RESULT open_module(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_0, HD_VIDEOENC_0_OUT_0, &p_stream->enc_path)) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT close_module(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videoenc_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}

static BOOL check_test_pattern(void)
{
	FILE *f_in;
	char filename[64];
	char filepath[128];
	UINT32 i;

	for (i=0; i< PATTERN_FILE_NUM; i++) {
		sprintf(filename, "video_frm_%lu_%lu_%lu_yuv420.dat", g_PatternFileTable[i].width, g_PatternFileTable[i].height, g_PatternFileTable[i].frame_cnt);
		sprintf(filepath, "/mnt/sd/%s", filename);

		if ((f_in = fopen(filepath, "rb")) == NULL) {
			printf("fail to open %s\n", filepath);
			printf("%s is in SDK/code/hdal/samples/pattern/%s\n", filename, filename);
			return FALSE;
		}

		fclose(f_in);
	}
	return TRUE;
}

static INT32 blk2idx(HD_COMMON_MEM_VB_BLK blk)   // convert blk(0xXXXXXXXX) to index (0, 1, 2)
{
	static HD_COMMON_MEM_VB_BLK blk_registered[MAX_YUV_BLK_CNT] = {0};
	INT32 i;
	for (i=0; i< MAX_YUV_BLK_CNT; i++) {
		if (blk_registered[i] == blk) return i;

		if (blk_registered[i] == 0) {
			blk_registered[i] = blk;
			return i;
		}
	}

	printf("convert blk(%0x%x) to index fail !!!!\r\n", blk);
	return (-1);
}

static void *feed_yuv_thread(void *arg)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)arg;
	UINT32 enc_w   = g_PatternFileTable[p_stream0->pattern_idx].width;
	UINT32 enc_h   = g_PatternFileTable[p_stream0->pattern_idx].height;
	UINT32 pat_cnt = g_PatternFileTable[p_stream0->pattern_idx].frame_cnt;
	
	HD_RESULT ret = HD_OK;
	int i;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32 blk_size = YUV_BLK_SIZE(enc_w, enc_h, HD_VIDEO_PXLFMT_YUV420);
	UINT32 yuv_size = VDO_YUV_BUFSIZE(enc_w, enc_h, HD_VIDEO_PXLFMT_YUV420);
	char filepath_yuv_main[128];
	FILE *f_in_main;
	UINT32 pa_yuv_main[MAX_YUV_BLK_CNT] = {0};
	UINT32 va_yuv_main[MAX_YUV_BLK_CNT] = {0};
	INT32  blkidx;
	HD_COMMON_MEM_VB_BLK blk;


	//------ [1] wait flow_start ------
	while (p_stream0->flow_start == 0) sleep(1);

	//------ [2] open input files ------
	sprintf(filepath_yuv_main, "/mnt/sd/video_frm_%lu_%lu_%lu_yuv420.dat", enc_w, enc_h, pat_cnt);
	if ((f_in_main = fopen(filepath_yuv_main, "rb")) == NULL) {
		printf("open file (%s) fail !!....\r\nPlease copy test pattern to SD Card !!\r\n\r\n", filepath_yuv_main);
		return 0;
	}

	//------ [3] feed yuv ------
	while (p_stream0->yuv_exit == 0) {

		//--- Get memory ---
		blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id); // Get block from mem pool
		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get block fail (0x%x).. try again later.....\r\n", blk);
			sleep(1);
			continue;
		}

		if ((blkidx = blk2idx(blk)) == -1) {
			printf("ERROR !! blk to idx fail !!\r\n");
			goto rel_blk;
		}

		pa_yuv_main[blkidx] = hd_common_mem_blk2pa(blk); // Get physical addr
		if (pa_yuv_main[blkidx] == 0) {
			printf("blk2pa fail, blk = 0x%x\r\n", blk);
			goto rel_blk;
		}

		if (va_yuv_main[blkidx] == 0) { // if NOT mmap yet, mmap it
			va_yuv_main[blkidx] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa_yuv_main[blkidx], blk_size); // Get virtual addr
			if (va_yuv_main[blkidx] == 0) {
				printf("Error: mmap fail !! pa_yuv_main[%d], blk = 0x%x\r\n", blkidx, blk);
				goto rel_blk;
			}
		}

		//--- Read YUV from file ---
		fread((void *)va_yuv_main[blkidx], 1, yuv_size, f_in_main);

		if (feof(f_in_main)) {
			fseek(f_in_main, 0, SEEK_SET);  //rewind
			fread((void *)va_yuv_main[blkidx], 1, yuv_size, f_in_main);
		}
		
		//--- data is written by CPU, flush CPU cache to PHY memory ---
		hd_common_mem_flush_cache((void *)va_yuv_main[blkidx], yuv_size);

		//--- push_in ---
		{
			HD_VIDEO_FRAME video_frame;

			video_frame.sign        = MAKEFOURCC('V','F','R','M');
			video_frame.p_next      = NULL;
			video_frame.ddr_id      = ddr_id;
			video_frame.pxlfmt      = HD_VIDEO_PXLFMT_YUV420;
			video_frame.dim.w       = enc_w;
			video_frame.dim.h       = enc_h;
			video_frame.count       = 0;
			video_frame.timestamp   = hd_gettime_us();
			video_frame.loff[0]     = enc_w; // Y
			video_frame.loff[1]     = enc_w; // UV
			video_frame.phy_addr[0] = pa_yuv_main[blkidx];                // Y
			video_frame.phy_addr[1] = pa_yuv_main[blkidx] + enc_w*enc_h;  // UV pack
			video_frame.blk         = blk;

			ret = hd_videoenc_push_in_buf(p_stream0->enc_path, &video_frame, NULL, 0);
			if (ret != HD_OK) {
				static int cnt_push_err=0;
				if (cnt_push_err++%30==0) printf("push_in error !!\r\n");
			}
		}

rel_blk:
		//--- Release memory ---
		hd_common_mem_release_block(blk);

		usleep(30000); // sleep 30 ms
	}

	//------ [4] uninit & exit ------
	// mummap for yuv buffer
	for (i=0; i< MAX_YUV_BLK_CNT; i++) {
		if (va_yuv_main[i] != 0)
			hd_common_mem_munmap((void *)va_yuv_main[i], blk_size);
	}

	// close file
	if (f_in_main)  fclose(f_in_main);


	return 0;
}

static void *encode_thread(void *arg)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_BS  data_pull;
	UINT32 j;

	UINT32 vir_addr_main;
	HD_VIDEOENC_BUFINFO phy_buf_main;
	char file_path_main[256];
	FILE *f_out_main;
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))

	//------ wait flow_start ------
	while (p_stream0->flow_start == 0) sleep(1);

	// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
	hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);

	// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);

	//----- open output files -----
	snprintf(file_path_main, 256, "/mnt/sd/dump_bs_test%03lu_%s.dat", p_stream0->file_sn, p_stream0->test_name);
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		HD_VIDEOENC_ERR("open file (%s) fail....\r\n", file_path_main);
	} else {
		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}

	//--------- pull data test ---------
	while (p_stream0->enc_exit == 0) {
		//pull data
		ret = hd_videoenc_pull_out_buf(p_stream0->enc_path, &data_pull, 300); // >1 = timeout mode

		if (ret == HD_OK) {
			for (j=0; j< data_pull.pack_num; j++) {
				UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.video_pack[j].phy_addr);
				UINT32 len = data_pull.video_pack[j].size;
				if (f_out_main) fwrite(ptr, 1, len, f_out_main);
				if (f_out_main) fflush(f_out_main);
			}

			// release data
			ret = hd_videoenc_release_out_buf(p_stream0->enc_path, &data_pull);
			if (ret != HD_OK) {
				printf("enc_release error=%d !!\r\n", ret);
			}
		}
	}

	// mummap for bs buffer
	if (vir_addr_main) hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);

	// close output file
	if (f_out_main) fclose(f_out_main);

	p_stream0->file_sn++;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////
static HD_RESULT start_encode_thread(VIDEO_RECORD *p_stream)
{
	int ret;

	p_stream->enc_exit = 0;

	ret = pthread_create(&p_stream->enc_thread_id, NULL, encode_thread, (void *)p_stream);
	if (ret < 0) {
		printf("create encode thread failed");
		return HD_ERR_NG;
	}
	return HD_OK;
}

static HD_RESULT stop_encode_thread(VIDEO_RECORD *p_stream)
{
	p_stream->enc_exit = 1;

	// destroy feed thread
	pthread_join(stream[0].enc_thread_id, NULL);
	
	return HD_OK;
}

static HD_RESULT start_feed_thread(VIDEO_RECORD *p_stream)
{
	int ret;

	p_stream->yuv_exit = 0;

	ret = pthread_create(&stream[0].yuv_thread_id, NULL, feed_yuv_thread, (void *)stream);
	if (ret < 0) {
		printf("create encode thread failed");
		return HD_ERR_NG;
	}
	return HD_OK;
}

static HD_RESULT stop_feed_thread(VIDEO_RECORD *p_stream)
{
	p_stream->yuv_exit = 1;

	// destroy feed thread
	pthread_join(stream[0].yuv_thread_id, NULL);
	
	return HD_OK;
}
/////////////////////////////////////////////////////////////////////////////////

static BOOL is_h26x_encoding(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	HD_VIDEOENC_OUT out_param = {0};
	hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
	return ((out_param.codec_type == HD_CODEC_TYPE_H264) || (out_param.codec_type == HD_CODEC_TYPE_H265))? TRUE:FALSE;
}

static int change_ve_in_crop(void)
{

	return 0;
}

static int change_ve_in_dim(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice, width, height;
	HD_RESULT ret = HD_OK;
	UINT32 i = 0;
	char test_name[MAX_LEN_TEST_NAME];

	// get setup choice
	printf("[in_dim] Please enter which dim\r\n------------------------------\r\n");
	for (i = 0; i < PATTERN_FILE_NUM ; i++) {
		printf("%2u : %4ux%4u\r\n", i, g_PatternFileTable[i].width, g_PatternFileTable[i].height);
	}
	printf("\r\n");
	
	choice = hd_read_decimal_key_input("");

	if (choice < PATTERN_FILE_NUM) {
		width  = g_PatternFileTable[choice].width;
		height = g_PatternFileTable[choice].height;

		p_stream0->pattern_idx = choice;
		snprintf(test_name, MAX_LEN_TEST_NAME, "dim%lux%lu", width, height);
		SET_TEST_NAME(test_name);
	} else {
		printf("wrong choice\r\n");
		return 0;
	}

	// stop videoenc
	hd_videoenc_stop(p_stream0->enc_path);

	// stop previous thread & close bitstream file
	stop_encode_thread(p_stream0);
	
	// stop feed yuv
	stop_feed_thread(p_stream0);
	

	// set new param
	{
		HD_VIDEOENC_IN in_param = {0};
		hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_IN, &in_param);
		in_param.dim.w   = width;
		in_param.dim.h   = height;
		ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_IN, &in_param);
		if (ret != HD_OK) {
			printf("set_enc_param_in0 = %d\r\n", ret);
			return ret;
		}
	}

	// start videoenc, videoproc
	hd_videoenc_start(p_stream0->enc_path);

	// start thread & write bitstream file
	start_encode_thread(p_stream0);   // this must call after hd_videoenc_start()

	// start feed thread
	start_feed_thread(p_stream0);

	return 0;
}

static int change_ve_in_dir(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice;
	HD_VIDEO_DIR dir;
	HD_RESULT ret = HD_OK;

	// get setup choice
	printf("[in_dir] Please enter which dir\r\n------------------------------\r\n00 : None\r\n01 : dir90\r\n02 : dir180\r\n03 : dir270\r\n");
	choice = hd_read_decimal_key_input("");

	switch (choice) {
		case 0:  dir = HD_VIDEO_DIR_NONE;         SET_TEST_NAME("dir0");	break;
		case 1:  dir = HD_VIDEO_DIR_ROTATE_90;    SET_TEST_NAME("dir90");	break;
		case 2:  dir = HD_VIDEO_DIR_ROTATE_180;   SET_TEST_NAME("dir180");	break;
		case 3:  dir = HD_VIDEO_DIR_ROTATE_270;   SET_TEST_NAME("dir270");	break;
		default:
			printf("wrong choice\r\n");
			return 0;
	}

	// stop videoenc
	hd_videoenc_stop(p_stream0->enc_path);

	// stop previous thread & close bitstream file
	stop_encode_thread(p_stream0);

	// set new param
	{
		HD_VIDEOENC_IN in_param = {0};
		hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_IN, &in_param);
		in_param.dir   = dir;
		ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_IN, &in_param);
		if (ret != HD_OK) {
			printf("set_enc_param_in0 = %d\r\n", ret);
			return ret;
		}
	}

	// start videoenc
	hd_videoenc_start(p_stream0->enc_path);

	// start thread & write bitstream file
	start_encode_thread(p_stream0);   // this must call after hd_videoenc_start()

	return 0;
}

static int change_ve_in_pxlfmt(void)
{
	printf("NOT support yet... !!\n");
#if 0	
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice;
	HD_VIDEO_PXLFMT pxlfmt;
	HD_RESULT ret = HD_OK;

	// get setup choice
	printf("[in_pxlfmt] Please enter which pxlfmt\r\n------------------------------\r\n00 : YUV420\r\n01 : NVX1_H264\r\n02 : NVX1_H265\r\n");
	choice = hd_read_decimal_key_input("");

	switch (choice) {
		case 0:  pxlfmt = HD_VIDEO_PXLFMT_YUV420;              break;
		case 1:  pxlfmt = HD_VIDEO_PXLFMT_YUV420_NVX1_H264;    break;
		case 2:  pxlfmt = HD_VIDEO_PXLFMT_YUV420_NVX1_H265;    break;
		default:
			printf("wrong choice\r\n");
			return 0;
	}

	// stop videoenc
	hd_videoenc_stop(p_stream0->enc_path);

	// stop previous thread & close bitstream file
	stop_encode_thread(p_stream0);

	// set new param
	{
		HD_VIDEOENC_IN in_param = {0};
		hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_IN, &in_param);
		in_param.pxl_fmt   = pxlfmt;
		ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_IN, &in_param);
		if (ret != HD_OK) {
			printf("set_enc_param_in0 = %d\r\n", ret);
			return ret;
		}
	}

	// start videoenc, videoproc
	hd_videoenc_start(p_stream0->enc_path);
	hd_videoproc_start(p_stream0->video_proc_path);

	// start thread & write bitstream file
	start_encode_thread(p_stream0);   // this must call after hd_videoenc_start()
#endif
	return 0;
}

static int change_ve_out_codec(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice =0;
	HD_RESULT ret = HD_OK;

	// get setup choice
	printf("[out_codec] Please enter which codec\r\n------------------------------\r\n00 : MJPEG\r\n01 : H264\r\n02 : H265\r\n");
	printf("81 : wrong codec type\r\n");
	printf("82 : wrong profile\r\n");
	printf("83 : wrong level_idc\r\n");
	printf("84 : wrong entropy\r\n");
	printf("85 : wrong source_output\r\n");
	printf("86 : wrong jpeg quality\r\n");
	printf("87 : wrong jpeg frame_rate_incr\r\n");
	choice = hd_read_decimal_key_input("");

	// stop videoenc
	hd_videoenc_stop(p_stream0->enc_path);

	// stop previous thread & close bitstream file
	stop_encode_thread(p_stream0);

	// set new param
	switch (choice) {
		case 0:
		{
			HD_VIDEOENC_OUT out_param = {0};

		    out_param.codec_type             = HD_CODEC_TYPE_JPEG;
			out_param.jpeg.image_quality     = 50;
			out_param.jpeg.retstart_interval = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("MJPEG_q50_res0");
		}
		break;
		case 1:
		{
			HD_VIDEOENC_OUT out_param = {0};

			out_param.codec_type         = HD_CODEC_TYPE_H264;
			out_param.h26x.profile       = HD_H264E_HIGH_PROFILE;
			out_param.h26x.level_idc     = HD_H264E_LEVEL_5_1;
			out_param.h26x.gop_num       = 15;
			out_param.h26x.ltr_interval  = 0;
			out_param.h26x.ltr_pre_ref   = 0;
			out_param.h26x.gray_en       = 0;
			out_param.h26x.source_output = 0;
			out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			out_param.h26x.entropy_mode  = HD_H264E_CABAC_CODING;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("H264_gop15_high51_cabac_ltr_svcOFF");
		}
		break;
		case 2:
		{
			HD_VIDEOENC_OUT out_param = {0};

			out_param.codec_type         = HD_CODEC_TYPE_H265;
			out_param.h26x.profile       = HD_H265E_MAIN_PROFILE;
			out_param.h26x.level_idc     = HD_H265E_LEVEL_5;
			out_param.h26x.gop_num       = 15;
			out_param.h26x.ltr_interval  = 0;
			out_param.h26x.ltr_pre_ref   = 0;
			out_param.h26x.gray_en       = 0;
			out_param.h26x.source_output = 0;
			out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			out_param.h26x.entropy_mode  = HD_H265E_CABAC_CODING;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("H265_gop15_main150_cabac_ltr_svcOFF");
		}
		break;
		case 81:
		{
			HD_VIDEOENC_OUT out_param = {0};
			printf("[TEST_ERR] test wrong codec type =>\r\n");
			out_param.codec_type         = HD_CODEC_TYPE_MAX;  // invalid codec type
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 82:
		{
			HD_VIDEOENC_OUT out_param = {0};
			printf("[TEST_ERR] test wrong profile =>\r\n");
			out_param.codec_type         = HD_CODEC_TYPE_H265;
			out_param.h26x.profile       = HD_H265E_MAIN_PROFILE+1; // invalid profile
			out_param.h26x.level_idc     = HD_H265E_LEVEL_5;
			out_param.h26x.gop_num       = 15;
			out_param.h26x.ltr_interval  = 0;
			out_param.h26x.ltr_pre_ref   = 0;
			out_param.h26x.gray_en       = 0;
			out_param.h26x.source_output = 0;
			out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			out_param.h26x.entropy_mode  = HD_H265E_CABAC_CODING;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 83:
		{
			HD_VIDEOENC_OUT out_param = {0};
			printf("[TEST_ERR] test wrong level_idc =>\r\n");
			out_param.codec_type         = HD_CODEC_TYPE_H265;
			out_param.h26x.profile       = HD_H265E_MAIN_PROFILE;
			out_param.h26x.level_idc     = HD_H265E_LEVEL_5+1;  // invalid level_idc
			out_param.h26x.gop_num       = 15;
			out_param.h26x.ltr_interval  = 0;
			out_param.h26x.ltr_pre_ref   = 0;
			out_param.h26x.gray_en       = 0;
			out_param.h26x.source_output = 0;
			out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			out_param.h26x.entropy_mode  = HD_H265E_CABAC_CODING;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 84:
		{
			HD_VIDEOENC_OUT out_param = {0};
			printf("[TEST_ERR] test wrong entropy =>\r\n");
			out_param.codec_type         = HD_CODEC_TYPE_H265;
			out_param.h26x.profile       = HD_H265E_MAIN_PROFILE;
			out_param.h26x.level_idc     = HD_H265E_LEVEL_5;
			out_param.h26x.gop_num       = 15;
			out_param.h26x.ltr_interval  = 0;
			out_param.h26x.ltr_pre_ref   = 0;
			out_param.h26x.gray_en       = 0;
			out_param.h26x.source_output = 0;
			out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			out_param.h26x.entropy_mode  = HD_H265E_CABAC_CODING+1;  // invalid entropy
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 85:
		{
			HD_VIDEOENC_OUT out_param = {0};
			printf("[TEST_ERR] test wrong source_output =>\r\n");
			out_param.codec_type         = HD_CODEC_TYPE_H265;
			out_param.h26x.profile       = HD_H265E_MAIN_PROFILE;
			out_param.h26x.level_idc     = HD_H265E_LEVEL_5;
			out_param.h26x.gop_num       = 15;
			out_param.h26x.ltr_interval  = 0;
			out_param.h26x.ltr_pre_ref   = 0;
			out_param.h26x.gray_en       = 0;
			out_param.h26x.source_output = 3;  // invalid source_output
			out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			out_param.h26x.entropy_mode  = HD_H265E_CABAC_CODING;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 86:
		{
			HD_VIDEOENC_OUT2 out_param = {0};
			printf("[TEST_ERR] test wrong jpeg_quality =>\r\n");
		    out_param.codec_type             = HD_CODEC_TYPE_JPEG;
			out_param.jpeg.image_quality     = 101; // invalid jpeg quality
			out_param.jpeg.retstart_interval = 0;
			out_param.jpeg.bitrate           = 0;
			out_param.jpeg.frame_rate_base   = 30;
			out_param.jpeg.frame_rate_incr   = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 87:
		{
			HD_VIDEOENC_OUT2 out_param = {0};
			printf("[TEST_ERR] test wrong jpeg frame_rate_incr =>\r\n");
		    out_param.codec_type             = HD_CODEC_TYPE_JPEG;
			out_param.jpeg.image_quality     = 50;
			out_param.jpeg.retstart_interval = 0;
			out_param.jpeg.bitrate           = 0;
			out_param.jpeg.frame_rate_base   = 30;
			out_param.jpeg.frame_rate_incr   = 0;  // invalid jpeg frame_rate_incr
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		default:
			break;
	}

	// start videoenc
	hd_videoenc_start(p_stream0->enc_path);

	// start thread & write bitstream file
	start_encode_thread(p_stream0);   // this must call after hd_videoenc_start()

	return 0;
}

static int change_ve_out_gray(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice =0;
	HD_RESULT ret = HD_OK;

	// check H26x
	if (FALSE == is_h26x_encoding()) {
		printf("ERROR : this function only available for H26x\n");
		return 0;
	}

	// get setup choice
	printf("[out_codec] Please enter which codec\r\n------------------------------\r\n00 : gray_OFF\r\n01 : gray_ON\r\n");
	printf("81 : wrong gray_en\r\n");
	choice = hd_read_decimal_key_input("");

	// stop videoenc
	hd_videoenc_stop(p_stream0->enc_path);

	// stop previous thread & close bitstream file
	stop_encode_thread(p_stream0);

	// set new param
	switch (choice) {
		case 0:
		{
			HD_VIDEOENC_OUT out_param = {0};
			
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.h26x.gray_en       = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("gray_OFF");
		}
		break;
		case 1:
		{
			HD_VIDEOENC_OUT out_param = {0};
			
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.h26x.gray_en       = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("gray_ON");
		}
		break;
		case 81:
		{
			HD_VIDEOENC_OUT out_param = {0};
			printf("[TEST_ERR] test wrong gray_en =>\r\n");
			out_param.codec_type         = HD_CODEC_TYPE_H265;
			out_param.h26x.profile       = HD_H265E_MAIN_PROFILE;
			out_param.h26x.level_idc     = HD_H265E_LEVEL_5;
			out_param.h26x.gop_num       = 15;
			out_param.h26x.ltr_interval  = 0;
			out_param.h26x.ltr_pre_ref   = 0;
			out_param.h26x.gray_en       = 2;  // invalid gray_en
			out_param.h26x.source_output = 0;
			out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			out_param.h26x.entropy_mode  = HD_H265E_CABAC_CODING;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		default:
			break;
	}

	// start videoenc
	hd_videoenc_start(p_stream0->enc_path);

	// start thread & write bitstream file
	start_encode_thread(p_stream0);   // this must call after hd_videoenc_start()

	return 0;
}

static int change_ve_out_gop(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice =0;
	HD_RESULT ret = HD_OK;

	// check H26x
	if (FALSE == is_h26x_encoding()) {
		printf("ERROR : this function only available for H26x\n");
		return 0;
	}

	// get setup choice
	printf("[out_codec] Please enter which codec\r\n------------------------------\r\n00 : GOP 15\r\n01 : GOP 37\r\n02 : GOP 180\r\n");
	printf("81 : wrong gop\r\n");
	choice = hd_read_decimal_key_input("");

	// stop videoenc
	hd_videoenc_stop(p_stream0->enc_path);

	// stop previous thread & close bitstream file
	stop_encode_thread(p_stream0);

	// set new param
	switch (choice) {
		case 0:
		{
			HD_VIDEOENC_OUT out_param = {0};
			
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.h26x.gop_num       = 15;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("GOP_15");
		}
		break;
		case 1:
		{
			HD_VIDEOENC_OUT out_param = {0};
			
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.h26x.gop_num       = 37;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("GOP_37");
		}
		break;
		case 2:
		{
			HD_VIDEOENC_OUT out_param = {0};
			
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.h26x.gop_num       = 180;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("GOP_180");
		}
		break;
		case 81:
		{
			HD_VIDEOENC_OUT out_param = {0};
			printf("[TEST_ERR] test wrong gop =>\r\n");
			out_param.codec_type         = HD_CODEC_TYPE_H265;
			out_param.h26x.profile       = HD_H265E_MAIN_PROFILE;
			out_param.h26x.level_idc     = HD_H265E_LEVEL_5;
			out_param.h26x.gop_num       = 4097;  // invalid gop value
			out_param.h26x.ltr_interval  = 0;
			out_param.h26x.ltr_pre_ref   = 0;
			out_param.h26x.gray_en       = 0;
			out_param.h26x.source_output = 0;
			out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			out_param.h26x.entropy_mode  = HD_H265E_CABAC_CODING;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		default:
			break;
	}

	// start videoenc
	hd_videoenc_start(p_stream0->enc_path);

	// start thread & write bitstream file
	start_encode_thread(p_stream0);   // this must call after hd_videoenc_start()

	return 0;
}

static int change_ve_out_svc(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice =0;
	HD_RESULT ret = HD_OK;

	// check H26x
	if (FALSE == is_h26x_encoding()) {
		printf("ERROR : this function only available for H26x\n");
		return 0;
	}

	// get setup choice
	printf("[out_codec] Please enter which codec\r\n------------------------------\r\n00 : SVC_OFF\r\n01 : SVC_2X\r\n02 : SVC_4X\r\n");
	printf("81 : wrong svc_layer\r\n");
	choice = hd_read_decimal_key_input("");

	// stop videoenc
	hd_videoenc_stop(p_stream0->enc_path);

	// stop previous thread & close bitstream file
	stop_encode_thread(p_stream0);

	// set new param
	switch (choice) {
		case 0:
		{
			HD_VIDEOENC_OUT out_param = {0};
			
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.h26x.svc_layer       = HD_SVC_DISABLE;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("SVC_OFF");
		}
		break;
		case 1:
		{
			HD_VIDEOENC_OUT out_param = {0};
			
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.h26x.svc_layer       = HD_SVC_2X;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("SVC_2X");
		}
		break;
		case 2:
		{
			HD_VIDEOENC_OUT out_param = {0};
			
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.h26x.svc_layer       = HD_SVC_4X;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("SVC_4X");
		}
		break;
		case 81:
		{
			HD_VIDEOENC_OUT out_param = {0};
			printf("[TEST_ERR] test wrong svc_layer =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.h26x.svc_layer       = HD_SVC_MAX;  // invalid svc_layer
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		default:
			break;
	}

	// start videoenc
	hd_videoenc_start(p_stream0->enc_path);

	// start thread & write bitstream file
	start_encode_thread(p_stream0);   // this must call after hd_videoenc_start()

	return 0;
}

static int change_ve_out_ltr(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice =0;
	HD_RESULT ret = HD_OK;

	// check H26x
	if (FALSE == is_h26x_encoding()) {
		printf("ERROR : this function only available for H26x\n");
		return 0;
	}

	// get setup choice
	printf("[out_codec] Please enter which codec\r\n------------------------------\r\n00 : LTR_OFF\r\n01 : LTR_7_0\r\n02 : LTR_7_1\r\n");
	printf("81 : wrong ltr_interval\r\n");
	printf("82 : wrong ltr_pre_ref\r\n");
	choice = hd_read_decimal_key_input("");

	// stop videoenc
	hd_videoenc_stop(p_stream0->enc_path);

	// stop previous thread & close bitstream file
	stop_encode_thread(p_stream0);

	// set new param
	switch (choice) {
		case 0:
		{
			HD_VIDEOENC_OUT out_param = {0};
			
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.h26x.ltr_interval   = 0;
			out_param.h26x.ltr_pre_ref    = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("LTR_OFF");
		}
		break;
		case 1:
		{
			HD_VIDEOENC_OUT out_param = {0};
			
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.h26x.ltr_interval   = 7;
			out_param.h26x.ltr_pre_ref    = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("LTR_7_0");
		}
		break;
		case 2:
		{
			HD_VIDEOENC_OUT out_param = {0};
			
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.h26x.ltr_interval   = 7;
			out_param.h26x.ltr_pre_ref    = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("LTR_7_1");
		}
		break;
		case 81:
		{
			HD_VIDEOENC_OUT out_param = {0};
			printf("[TEST_ERR] test wrong ltr_interval =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.h26x.ltr_interval   = 4096;  // invalid ltr_interval
			out_param.h26x.ltr_pre_ref    = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 82:
		{
			HD_VIDEOENC_OUT out_param = {0};
			printf("[TEST_ERR] test wrong ltr_pre_ref =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.h26x.ltr_interval   = 30;
			out_param.h26x.ltr_pre_ref    = 2;  // invalid ltr_pre_ref
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_param_out0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		default:
			break;
	}

	// start videoenc
	hd_videoenc_start(p_stream0->enc_path);

	// start thread & write bitstream file
	start_encode_thread(p_stream0);   // this must call after hd_videoenc_start()

	return 0;
}

static int change_ve_out_vui(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice =0;
	HD_RESULT ret = HD_OK;

	// check H26x
	if (FALSE == is_h26x_encoding()) {
		printf("ERROR : this function only available for H26x\n");
		return 0;
	}

	// get setup choice
	printf("[out_codec] Please enter which codec\r\n------------------------------\r\n00 : vui_0\r\n01 : vui_1_4_3_22_33_44_2_1_1\r\n02 : vui_1_5_4_66_55_44_3_0_0\r\n");
	printf("81 : wrong vui_en\r\n");
	printf("82 : wrong sar_width\r\n");
	printf("83 : wrong sar_height\r\n");
	//printf("84 : wrong matrix_coef\r\n");
	//printf("85 : wrong transfer_characteristics\r\n");
	//printf("86 : wrong colour_primaries\r\n");
	printf("87 : wrong video_format\r\n");
	printf("88 : wrong color_range\r\n");
	printf("89 : wrong timing_present_flag\r\n");
	choice = hd_read_decimal_key_input("");

	// stop videoenc
	hd_videoenc_stop(p_stream0->enc_path);

	// stop previous thread & close bitstream file
	stop_encode_thread(p_stream0);

	// set new param
	switch (choice) {
		case 0:
		{
			HD_H26XENC_VUI vui_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			vui_param.vui_en                   = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			if (ret != HD_OK) {
				printf("set enc_vui = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("vui_0");
		}
		break;
		case 1:
		{
			HD_H26XENC_VUI vui_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			vui_param.vui_en                   = 1;
			vui_param.sar_width                = 4;
			vui_param.sar_height               = 3;
			vui_param.matrix_coef              = 22;
			vui_param.transfer_characteristics = 33;
			vui_param.colour_primaries         = 44;
			vui_param.video_format             = 2;
			vui_param.color_range              = 1;
			vui_param.timing_present_flag      = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			if (ret != HD_OK) {
				printf("set enc_vui = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("vui_1_4_3_22_33_44_2_1_1");
		}
		break;
		case 2:
		{
			HD_H26XENC_VUI vui_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			vui_param.vui_en                   = 1;
			vui_param.sar_width                = 5;
			vui_param.sar_height               = 4;
			vui_param.matrix_coef              = 66;
			vui_param.transfer_characteristics = 55;
			vui_param.colour_primaries         = 44;
			vui_param.video_format             = 3;
			vui_param.color_range              = 0;
			vui_param.timing_present_flag      = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			if (ret != HD_OK) {
				printf("set enc_vui = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("vui_1_5_4_66_55_44_3_0_0");
		}
		break;
		case 81:
		{
			HD_H26XENC_VUI vui_param;
			printf("[TEST_ERR] test wrong vui_en =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			vui_param.vui_en                   = 2;  // invalid vui_en
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			if (ret != HD_OK) {
				printf("set enc_vui = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 82:
		{
			HD_H26XENC_VUI vui_param;
			printf("[TEST_ERR] test wrong sar_width =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			vui_param.vui_en                   = 1;
			vui_param.sar_width                = 65536;  // invalid sar_width
			vui_param.sar_height               = 3;
			vui_param.matrix_coef              = 22;
			vui_param.transfer_characteristics = 33;
			vui_param.colour_primaries         = 44;
			vui_param.video_format             = 2;
			vui_param.color_range              = 1;
			vui_param.timing_present_flag      = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			if (ret != HD_OK) {
				printf("set enc_vui = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 83:
		{
			HD_H26XENC_VUI vui_param;
			printf("[TEST_ERR] test wrong sar_height =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			vui_param.vui_en                   = 1;
			vui_param.sar_width                = 4;
			vui_param.sar_height               = 65536;  // invalid sar_height
			vui_param.matrix_coef              = 22;
			vui_param.transfer_characteristics = 33;
			vui_param.colour_primaries         = 44;
			vui_param.video_format             = 2;
			vui_param.color_range              = 1;
			vui_param.timing_present_flag      = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			if (ret != HD_OK) {
				printf("set enc_vui = %d\r\n", ret);
				return ret;
			}
		}
		break;
#if 0
		case 84:
		{
			HD_H26XENC_VUI vui_param;
			printf("[TEST_ERR] test wrong matrix_coef =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			vui_param.vui_en                   = 1;
			vui_param.sar_width                = 4;
			vui_param.sar_height               = 3;
			vui_param.matrix_coef              = 22;  // UINT8 , must 0~255 ... can't test invalid
			vui_param.transfer_characteristics = 33;
			vui_param.colour_primaries         = 44;
			vui_param.video_format             = 2;
			vui_param.color_range              = 1;
			vui_param.timing_present_flag      = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			if (ret != HD_OK) {
				printf("set enc_vui = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 85:
		{
			HD_H26XENC_VUI vui_param;
			printf("[TEST_ERR] test wrong transfer_characteristics =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			vui_param.vui_en                   = 1;
			vui_param.sar_width                = 4;
			vui_param.sar_height               = 3;
			vui_param.matrix_coef              = 22;
			vui_param.transfer_characteristics = 33;  // UINT8 , must 0~255 ... can't test invalid
			vui_param.colour_primaries         = 44;
			vui_param.video_format             = 2;
			vui_param.color_range              = 1;
			vui_param.timing_present_flag      = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			if (ret != HD_OK) {
				printf("set enc_vui = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 86:
		{
			HD_H26XENC_VUI vui_param;
			printf("[TEST_ERR] test wrong colour_primaries =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			vui_param.vui_en                   = 1;
			vui_param.sar_width                = 4;
			vui_param.sar_height               = 3;
			vui_param.matrix_coef              = 22;
			vui_param.transfer_characteristics = 33;
			vui_param.colour_primaries         = 44;  // UINT8 , must 0~255 ... can't test invalid
			vui_param.video_format             = 2;
			vui_param.color_range              = 1;
			vui_param.timing_present_flag      = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			if (ret != HD_OK) {
				printf("set enc_vui = %d\r\n", ret);
				return ret;
			}
		}
		break;
#endif
		case 87:
		{
			HD_H26XENC_VUI vui_param;
			printf("[TEST_ERR] test wrong video_format =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			vui_param.vui_en                   = 1;
			vui_param.sar_width                = 4;
			vui_param.sar_height               = 3;
			vui_param.matrix_coef              = 22;
			vui_param.transfer_characteristics = 33;
			vui_param.colour_primaries         = 44;
			vui_param.video_format             = 8;  // invalid video_format
			vui_param.color_range              = 1;
			vui_param.timing_present_flag      = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			if (ret != HD_OK) {
				printf("set enc_vui = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 88:
		{
			HD_H26XENC_VUI vui_param;
			printf("[TEST_ERR] test wrong color_range =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			vui_param.vui_en                   = 1;
			vui_param.sar_width                = 4;
			vui_param.sar_height               = 3;
			vui_param.matrix_coef              = 22;
			vui_param.transfer_characteristics = 33;
			vui_param.colour_primaries         = 44;
			vui_param.video_format             = 2;
			vui_param.color_range              = 2;  // invalid color_range
			vui_param.timing_present_flag      = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			if (ret != HD_OK) {
				printf("set enc_vui = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 89:
		{
			HD_H26XENC_VUI vui_param;
			printf("[TEST_ERR] test wrong timing_present_flag =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			vui_param.vui_en                   = 1;
			vui_param.sar_width                = 4;
			vui_param.sar_height               = 3;
			vui_param.matrix_coef              = 22;
			vui_param.transfer_characteristics = 33;
			vui_param.colour_primaries         = 44;
			vui_param.video_format             = 2;
			vui_param.color_range              = 1;
			vui_param.timing_present_flag      = 2;  // invalid timing_present_flag
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
			if (ret != HD_OK) {
				printf("set enc_vui = %d\r\n", ret);
				return ret;
			}
		}
		break;
		default:
			break;
	}

	// start videoenc
	hd_videoenc_start(p_stream0->enc_path);

	// start thread & write bitstream file
	start_encode_thread(p_stream0);   // this must call after hd_videoenc_start()

	return 0;
}

static int change_ve_out_db(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice =0;
	HD_RESULT ret = HD_OK;

	// check H26x
	if (FALSE == is_h26x_encoding()) {
		printf("ERROR : this function only available for H26x\n");
		return 0;
	}

	// get setup choice
	printf("[out_codec] Please enter which codec\r\n------------------------------\r\n00 : db_D0S0T0_5_3\r\n01 : db_D0S1T0_-5_-3\r\n02 : db_D1S0T0\r\n03 : db_D0S0T1_4_2\r\n04 : db_D0S1T1_-4_-2\r\n");
	printf("81 : wrong dis_ilf_idc\r\n");
	printf("82 : wrong db_alpha_+\r\n");
	printf("83 : wrong db_alpha_-\r\n");
	printf("84 : wrong db_beta_+\r\n");
	printf("85 : wrong db_beta_-\r\n");
	choice = hd_read_decimal_key_input("");

	// stop videoenc
	hd_videoenc_stop(p_stream0->enc_path);

	// stop previous thread & close bitstream file
	stop_encode_thread(p_stream0);

	// set new param
	switch (choice) {
		case 0:
		{
			HD_H26XENC_DEBLOCK db_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			db_param.dis_ilf_idc = MAKE_DIS_ILF_IDC(0, 0, 0);
			db_param.db_alpha    = 5;
			db_param.db_beta     = 3;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			if (ret != HD_OK) {
				printf("set enc_out_deblock = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("db_D0S0T0_5_3");
		}
		break;
		case 1:
		{
			HD_H26XENC_DEBLOCK db_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			db_param.dis_ilf_idc = MAKE_DIS_ILF_IDC(0, 1, 0);
			db_param.db_alpha    = -5;
			db_param.db_beta     = -3;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			if (ret != HD_OK) {
				printf("set enc_out_deblock = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("db_D0S1T0_-5_-3");
		}
		break;
		case 2:
		{
			HD_H26XENC_DEBLOCK db_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			db_param.dis_ilf_idc = MAKE_DIS_ILF_IDC(1, 0, 0);
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			if (ret != HD_OK) {
				printf("set enc_out_deblock = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("db_D1S0T0");
		}
		break;
		case 3:
		{
			HD_H26XENC_DEBLOCK db_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			db_param.dis_ilf_idc = MAKE_DIS_ILF_IDC(0, 0, 1);
			db_param.db_alpha    = 4;
			db_param.db_beta     = 2;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			if (ret != HD_OK) {
				printf("set enc_out_deblock = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("db_D0S0T1_4_2");
		}
		break;
		case 4:
		{
			HD_H26XENC_DEBLOCK db_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			db_param.dis_ilf_idc = MAKE_DIS_ILF_IDC(0, 1, 1);
			db_param.db_alpha    = -4;
			db_param.db_beta     = -2;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			if (ret != HD_OK) {
				printf("set enc_out_deblock = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("db_D0S1T1_-4_-2");
		}
		break;
		case 81:
		{
			HD_H26XENC_DEBLOCK db_param;
			printf("[TEST_ERR] test wrong dis_ilf_idc =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			db_param.dis_ilf_idc = 7; // invalid dis_ilf_idc
			db_param.db_alpha    = 5;
			db_param.db_beta     = 3;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			if (ret != HD_OK) {
				printf("set enc_out_deblock = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 82:
		{
			HD_H26XENC_DEBLOCK db_param;
			printf("[TEST_ERR] test wrong db_alpha_+ =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			db_param.dis_ilf_idc = MAKE_DIS_ILF_IDC(0, 0, 0);
			db_param.db_alpha    = 13;  // invalid db_alpha
			db_param.db_beta     = 3;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			if (ret != HD_OK) {
				printf("set enc_out_deblock = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 83:
		{
			HD_H26XENC_DEBLOCK db_param;
			printf("[TEST_ERR] test wrong db_alpha_- =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			db_param.dis_ilf_idc = MAKE_DIS_ILF_IDC(0, 0, 0);
			db_param.db_alpha    = -13;  // invalid db_alpha
			db_param.db_beta     = 3;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			if (ret != HD_OK) {
				printf("set enc_out_deblock = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 84:
		{
			HD_H26XENC_DEBLOCK db_param;
			printf("[TEST_ERR] test wrong db_beta_+ =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			db_param.dis_ilf_idc = MAKE_DIS_ILF_IDC(0, 0, 0);
			db_param.db_alpha    = 5;
			db_param.db_beta     = 13;  // invalid db_beta
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			if (ret != HD_OK) {
				printf("set enc_out_deblock = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 85:
		{
			HD_H26XENC_DEBLOCK db_param;
			printf("[TEST_ERR] test wrong db_beta_- =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			db_param.dis_ilf_idc = MAKE_DIS_ILF_IDC(0, 0, 0);
			db_param.db_alpha    = 5;
			db_param.db_beta     = -13;  // invalid db_beta
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &db_param);
			if (ret != HD_OK) {
				printf("set enc_out_deblock = %d\r\n", ret);
				return ret;
			}
		}
		break;
		default:
			break;
	}

	// start videoenc
	hd_videoenc_start(p_stream0->enc_path);

	// start thread & write bitstream file
	start_encode_thread(p_stream0);   // this must call after hd_videoenc_start()

	return 0;
}

static int change_ve_out_slice(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice =0;
	HD_RESULT ret = HD_OK;

	// check H26x
	if (FALSE == is_h26x_encoding()) {
		printf("ERROR : this function only available for H26x\n");
		return 0;
	}

	// get setup choice
	printf("[out_codec] Please enter which codec\r\n------------------------------\r\n00 : slice_0\r\n01 : slice_1_2\r\n02 : slice_1_5\r\n03 : slice_1_17\r\n");
	printf("81 : wrong enable\r\n");
	choice = hd_read_decimal_key_input("");

	// stop videoenc
	hd_videoenc_stop(p_stream0->enc_path);

	// stop previous thread & close bitstream file
	stop_encode_thread(p_stream0);

	// set new param
	switch (choice) {
		case 0:
		{
			HD_H26XENC_SLICE_SPLIT slice_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT, &slice_param);
			slice_param.enable = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT, &slice_param);
			if (ret != HD_OK) {
				printf("set enc_out_slicesplit = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("slice_0");
		}
		break;
		case 1:
		{
			HD_H26XENC_SLICE_SPLIT slice_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT, &slice_param);
			slice_param.enable        = 1;
			slice_param.slice_row_num = 2;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT, &slice_param);
			if (ret != HD_OK) {
				printf("set enc_out_slicesplit = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("slice_1_2");
		}
		break;
		case 2:
		{
			HD_H26XENC_SLICE_SPLIT slice_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT, &slice_param);
			slice_param.enable        = 1;
			slice_param.slice_row_num = 5;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT, &slice_param);
			if (ret != HD_OK) {
				printf("set enc_out_slicesplit = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("slice_1_5");
		}
		break;
		case 3:
		{
			HD_H26XENC_SLICE_SPLIT slice_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT, &slice_param);
			slice_param.enable        = 1;
			slice_param.slice_row_num = 17;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT, &slice_param);
			if (ret != HD_OK) {
				printf("set enc_out_slicesplit = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("slice_1_17");
		}
		break;
		case 81:
		{
			HD_H26XENC_SLICE_SPLIT slice_param;
			printf("[TEST_ERR] test wrong enable =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT, &slice_param);
			slice_param.enable = 2;  // invalid enable
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT, &slice_param);
			if (ret != HD_OK) {
				printf("set enc_out_slicesplit = %d\r\n", ret);
				return ret;
			}
		}
		break;
		default:
			break;
	}

	// start videoenc
	hd_videoenc_start(p_stream0->enc_path);

	// start thread & write bitstream file
	start_encode_thread(p_stream0);   // this must call after hd_videoenc_start()

	return 0;
}

static int change_ve_out_gdr(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice =0;
	HD_RESULT ret = HD_OK;

	// check H26x
	if (FALSE == is_h26x_encoding()) {
		printf("ERROR : this function only available for H26x\n");
		return 0;
	}

	// get setup choice
	printf("[out_codec] Please enter which codec\r\n------------------------------\r\n00 : gdr_0\r\n01 : gdr_1_N2P7\r\n02 : gdr_1_N5P7\r\n03 : gdr_1_N17P9\r\n");
	printf("81 : wrong enable\r\n");
	choice = hd_read_decimal_key_input("");

	// stop videoenc
	hd_videoenc_stop(p_stream0->enc_path);

	// stop previous thread & close bitstream file
	stop_encode_thread(p_stream0);

	// set new param
	switch (choice) {
		case 0:
		{
			HD_H26XENC_GDR gdr_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_GDR, &gdr_param);
			gdr_param.enable = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_GDR, &gdr_param);
			if (ret != HD_OK) {
				printf("set enc_out_gdr = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("gdr_0");
		}
		break;
		case 1:
		{
			HD_H26XENC_GDR gdr_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_GDR, &gdr_param);
			gdr_param.enable = 1;
			gdr_param.number = 2;
			gdr_param.period = 7;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_GDR, &gdr_param);
			if (ret != HD_OK) {
				printf("set enc_out_gdr = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("gdr_1_N2P7");
		}
		break;
		case 2:
		{
			HD_H26XENC_GDR gdr_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_GDR, &gdr_param);
			gdr_param.enable = 1;
			gdr_param.number = 5;
			gdr_param.period = 7;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_GDR, &gdr_param);
			if (ret != HD_OK) {
				printf("set enc_out_gdr = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("gdr_1_N5P7");
		}
		break;
		case 3:
		{
			HD_H26XENC_GDR gdr_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_GDR, &gdr_param);
			gdr_param.enable = 1;
			gdr_param.number = 17;
			gdr_param.period = 9;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_GDR, &gdr_param);
			if (ret != HD_OK) {
				printf("set enc_out_gdr = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("gdr_1_N17P9");
		}
		break;
		case 81:
		{
			HD_H26XENC_GDR gdr_param;
			printf("[TEST_ERR] test wrong enable =>\r\n");
			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_GDR, &gdr_param);
			gdr_param.enable = 2;  // invalid enable
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_GDR, &gdr_param);
			if (ret != HD_OK) {
				printf("set enc_out_gdr = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("gdr_0");
		}
		break;
		default:
			break;
	}

	// start videoenc
	hd_videoenc_start(p_stream0->enc_path);

	// start thread & write bitstream file
	start_encode_thread(p_stream0);   // this must call after hd_videoenc_start()

	return 0;
}

static int change_ve_out_aq_rowrc(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice =0;
	HD_RESULT ret = HD_OK;

	// check H26x
	if (FALSE == is_h26x_encoding()) {
		printf("ERROR : this function only available for H26x\n");
		return 0;
	}

	// get setup choice
	printf("[out_codec] Please enter which codec\r\n------------------------------\r\n00 : aq_0_rowrc_0\r\n01 : aq_0_rowrc_1\r\n02 : aq_1_rowrc_0\r\n03 : aq_1_rowrc_1\r\n");
	printf("81 : wrong rowrc.enable\r\n");
	printf("82 : wrong rowrc.i_qp_range\r\n");
	printf("83 : wrong rowrc.i_qp_step\r\n");
	printf("84 : wrong rowrc.p_qp_range\r\n");
	printf("85 : wrong rowrc.p_qp_step\r\n");
	printf("86 : wrong rowrc.min_i_qp\r\n");
	printf("87 : wrong rowrc.max_i_qp\r\n");
	printf("88 : wrong rowrc.min_p_qp\r\n");
	printf("89 : wrong rowrc.max_p_qp\r\n");
	printf("90 : wrong aq.enable\r\n");
	printf("91 : wrong aq.i_str\r\n");
	printf("92 : wrong aq.p_str\r\n");
	printf("93 : wrong aq.max_delta_qp\r\n");
	printf("94 : wrong aq.min_delta_qp\r\n");
	choice = hd_read_decimal_key_input("");

	// stop videoenc
	hd_videoenc_stop(p_stream0->enc_path);

	// stop previous thread & close bitstream file
	stop_encode_thread(p_stream0);

	// set new param
	switch (choice) {
		case 0:
		{
			HD_H26XENC_AQ aq_param;
			HD_H26XENC_ROW_RC rowrc_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_AQ, &aq_param);
			aq_param.enable = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_AQ, &aq_param);
			if (ret != HD_OK) {
				printf("set enc_out_aq = %d\r\n", ret);
				return ret;
			}

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			rowrc_param.enable = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			if (ret != HD_OK) {
				printf("set enc_out_rowrc = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("aq_0_rowrc_0");
		}
		break;
		case 1:
		{
			HD_H26XENC_AQ aq_param;
			HD_H26XENC_ROW_RC rowrc_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_AQ, &aq_param);
			aq_param.enable = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_AQ, &aq_param);
			if (ret != HD_OK) {
				printf("set enc_out_aq = %d\r\n", ret);
				return ret;
			}

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			rowrc_param.enable = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			if (ret != HD_OK) {
				printf("set enc_out_rowrc = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("aq_0_rowrc_1");
		}
		break;
		case 2:
		{
			HD_H26XENC_AQ aq_param;
			HD_H26XENC_ROW_RC rowrc_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_AQ, &aq_param);
			aq_param.enable = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_AQ, &aq_param);
			if (ret != HD_OK) {
				printf("set enc_out_aq = %d\r\n", ret);
				return ret;
			}

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			rowrc_param.enable = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			if (ret != HD_OK) {
				printf("set enc_out_rowrc = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("aq_1_rowrc_0");
		}
		break;
		case 3:
		{
			HD_H26XENC_AQ aq_param;
			HD_H26XENC_ROW_RC rowrc_param;

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_AQ, &aq_param);
			aq_param.enable = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_AQ, &aq_param);
			if (ret != HD_OK) {
				printf("set enc_out_aq = %d\r\n", ret);
				return ret;
			}

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			rowrc_param.enable = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			if (ret != HD_OK) {
				printf("set enc_out_rowrc = %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("aq_1_rowrc_1");
		}
		break;
		//////////////////////
		case 81:
		{
			HD_H26XENC_ROW_RC rowrc_param = {0};
			printf("[TEST_ERR] test wrong rowrc.enable =>\r\n");
			rowrc_param.enable     = 2;  // invalid enable
			rowrc_param.i_qp_range = 2;
			rowrc_param.i_qp_step  = 1;
			rowrc_param.p_qp_range = 4;
			rowrc_param.p_qp_step  = 1;
			rowrc_param.min_i_qp   = 1;
			rowrc_param.max_i_qp   = 51;
			rowrc_param.min_p_qp   = 1;
			rowrc_param.max_p_qp   = 51;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			if (ret != HD_OK) {
				printf("set enc_out_rowrc = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 82:
		{
			HD_H26XENC_ROW_RC rowrc_param = {0};
			printf("[TEST_ERR] test wrong rowrc.i_qp_range =>\r\n");
			rowrc_param.enable     = 1;
			rowrc_param.i_qp_range = 16;  // invalid i_qp_range
			rowrc_param.i_qp_step  = 1;
			rowrc_param.p_qp_range = 4;
			rowrc_param.p_qp_step  = 1;
			rowrc_param.min_i_qp   = 1;
			rowrc_param.max_i_qp   = 51;
			rowrc_param.min_p_qp   = 1;
			rowrc_param.max_p_qp   = 51;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			if (ret != HD_OK) {
				printf("set enc_out_rowrc = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 83:
		{
			HD_H26XENC_ROW_RC rowrc_param = {0};
			printf("[TEST_ERR] test wrong rowrc.i_qp_step =>\r\n");
			rowrc_param.enable     = 1;
			rowrc_param.i_qp_range = 2;
			rowrc_param.i_qp_step  = 16;  // invalid i_qp_step
			rowrc_param.p_qp_range = 4;
			rowrc_param.p_qp_step  = 1;
			rowrc_param.min_i_qp   = 1;
			rowrc_param.max_i_qp   = 51;
			rowrc_param.min_p_qp   = 1;
			rowrc_param.max_p_qp   = 51;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			if (ret != HD_OK) {
				printf("set enc_out_rowrc = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 84:
		{
			HD_H26XENC_ROW_RC rowrc_param = {0};
			printf("[TEST_ERR] test wrong rowrc.p_qp_range =>\r\n");
			rowrc_param.enable     = 1;
			rowrc_param.i_qp_range = 2;
			rowrc_param.i_qp_step  = 1;
			rowrc_param.p_qp_range = 16;  // invalid p_qp_range
			rowrc_param.p_qp_step  = 1;
			rowrc_param.min_i_qp   = 1;
			rowrc_param.max_i_qp   = 51;
			rowrc_param.min_p_qp   = 1;
			rowrc_param.max_p_qp   = 51;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			if (ret != HD_OK) {
				printf("set enc_out_rowrc = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 85:
		{
			HD_H26XENC_ROW_RC rowrc_param = {0};
			printf("[TEST_ERR] test wrong rowrc.p_qp_step =>\r\n");
			rowrc_param.enable     = 1;
			rowrc_param.i_qp_range = 2;
			rowrc_param.i_qp_step  = 1;
			rowrc_param.p_qp_range = 4;
			rowrc_param.p_qp_step  = 16;  // invalid p_qp_step
			rowrc_param.min_i_qp   = 1;
			rowrc_param.max_i_qp   = 51;
			rowrc_param.min_p_qp   = 1;
			rowrc_param.max_p_qp   = 51;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			if (ret != HD_OK) {
				printf("set enc_out_rowrc = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 86:
		{
			HD_H26XENC_ROW_RC rowrc_param = {0};
			printf("[TEST_ERR] test wrong rowrc.min_i_qp =>\r\n");
			rowrc_param.enable     = 1;
			rowrc_param.i_qp_range = 2;
			rowrc_param.i_qp_step  = 1;
			rowrc_param.p_qp_range = 4;
			rowrc_param.p_qp_step  = 1;
			rowrc_param.min_i_qp   = 52;  // invalid qp
			rowrc_param.max_i_qp   = 51;
			rowrc_param.min_p_qp   = 1;
			rowrc_param.max_p_qp   = 51;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			if (ret != HD_OK) {
				printf("set enc_out_rowrc = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 87:
		{
			HD_H26XENC_ROW_RC rowrc_param = {0};
			printf("[TEST_ERR] test wrong rowrc.max_i_qp =>\r\n");
			rowrc_param.enable     = 1;
			rowrc_param.i_qp_range = 2;
			rowrc_param.i_qp_step  = 1;
			rowrc_param.p_qp_range = 4;
			rowrc_param.p_qp_step  = 1;
			rowrc_param.min_i_qp   = 1;
			rowrc_param.max_i_qp   = 53;  // invalid qp
			rowrc_param.min_p_qp   = 1;
			rowrc_param.max_p_qp   = 51;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			if (ret != HD_OK) {
				printf("set enc_out_rowrc = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 88:
		{
			HD_H26XENC_ROW_RC rowrc_param = {0};
			printf("[TEST_ERR] test wrong rowrc.min_p_qp =>\r\n");
			rowrc_param.enable     = 1;
			rowrc_param.i_qp_range = 2;
			rowrc_param.i_qp_step  = 1;
			rowrc_param.p_qp_range = 4;
			rowrc_param.p_qp_step  = 1;
			rowrc_param.min_i_qp   = 1;
			rowrc_param.max_i_qp   = 51;
			rowrc_param.min_p_qp   = 54;  // invalid qp
			rowrc_param.max_p_qp   = 51;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			if (ret != HD_OK) {
				printf("set enc_out_rowrc = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 89:
		{
			HD_H26XENC_ROW_RC rowrc_param = {0};
			printf("[TEST_ERR] test wrong rowrc.max_p_qp =>\r\n");
			rowrc_param.enable     = 1;
			rowrc_param.i_qp_range = 2;
			rowrc_param.i_qp_step  = 1;
			rowrc_param.p_qp_range = 4;
			rowrc_param.p_qp_step  = 1;
			rowrc_param.min_i_qp   = 1;
			rowrc_param.max_i_qp   = 51;
			rowrc_param.min_p_qp   = 1;
			rowrc_param.max_p_qp   = 55;  // invalid qp
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROW_RC, &rowrc_param);
			if (ret != HD_OK) {
				printf("set enc_out_rowrc = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 90:
		{
			HD_H26XENC_AQ aq_param = {0};
			printf("[TEST_ERR] test wrong aq.enable =>\r\n");
			aq_param.enable       = 2;  // invalid enable
			aq_param.i_str        = 3;
			aq_param.p_str        = 1;
			aq_param.max_delta_qp = 6;
			aq_param.min_delta_qp = -6;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_AQ, &aq_param);
			if (ret != HD_OK) {
				printf("set enc_out_aq = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 91:
		{
			HD_H26XENC_AQ aq_param = {0};
			printf("[TEST_ERR] test wrong aq.i_str =>\r\n");
			aq_param.enable       = 1;
			aq_param.i_str        = 9;  // invalid i_str
			aq_param.p_str        = 1;
			aq_param.max_delta_qp = 6;
			aq_param.min_delta_qp = -6;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_AQ, &aq_param);
			if (ret != HD_OK) {
				printf("set enc_out_aq = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 92:
		{
			HD_H26XENC_AQ aq_param = {0};
			printf("[TEST_ERR] test wrong aq.p_str =>\r\n");
			aq_param.enable       = 1;
			aq_param.i_str        = 3;
			aq_param.p_str        = 9;  // invalid p_str
			aq_param.max_delta_qp = 6;
			aq_param.min_delta_qp = -6;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_AQ, &aq_param);
			if (ret != HD_OK) {
				printf("set enc_out_aq = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 93:
		{
			HD_H26XENC_AQ aq_param = {0};
			printf("[TEST_ERR] test wrong aq.max_delta_qp =>\r\n");
			aq_param.enable       = 1;
			aq_param.i_str        = 3;
			aq_param.p_str        = 1;
			aq_param.max_delta_qp = 9;  // invalid max_delta_qp
			aq_param.min_delta_qp = -6;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_AQ, &aq_param);
			if (ret != HD_OK) {
				printf("set enc_out_aq = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 94:
		{
			HD_H26XENC_AQ aq_param = {0};
			printf("[TEST_ERR] test wrong aq.min_delta_qp =>\r\n");
			aq_param.enable       = 1;
			aq_param.i_str        = 3;
			aq_param.p_str        = 1;
			aq_param.max_delta_qp = 6;
			aq_param.min_delta_qp = -9;  // invalid min_delta_qp
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_AQ, &aq_param);
			if (ret != HD_OK) {
				printf("set enc_out_aq = %d\r\n", ret);
				return ret;
			}
		}
		break;
		default:
			break;
	}

	// start videoenc
	hd_videoenc_start(p_stream0->enc_path);

	// start thread & write bitstream file
	start_encode_thread(p_stream0);   // this must call after hd_videoenc_start()

	return 0;
}

static int change_ve_out_roi(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice =0;
	HD_RESULT ret = HD_OK;

	// check H26x
	if (FALSE == is_h26x_encoding()) {
		printf("ERROR : this function only available for H26x\n");
		return 0;
	}

	// get setup choice
	printf("[out_codec] Please enter which codec\r\n------------------------------\r\n00 : roi_0\r\n01 : roi__2_1_33_3_32_80_48_64__7_1_-4_0_160_176_16_80\r\n02 : roi__2_1_33_3_64_128_128_64__7_1_-4_0_192_64_64_128\r\n");
	//printf("81 : wrong roi_qp_mode\r\n");
	printf("82 : wrong st_roi[xx].enable\r\n");
	printf("83 : wrong st_roi[xx].mode\r\n");
	printf("84 : wrong st_roi[xx].qp(fix)\r\n");
	printf("85 : wrong st_roi[xx].qp(delta_+)\r\n");
	printf("86 : wrong st_roi[xx].qp(delta_-)\r\n");
	choice = hd_read_decimal_key_input("");

	// stop videoenc
	hd_videoenc_stop(p_stream0->enc_path);

	// stop previous thread & close bitstream file
	stop_encode_thread(p_stream0);

	// set new param
	switch (choice) {
		case 0:
		{
			HD_H26XENC_ROI roi_param = {0};
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROI, &roi_param);
			if (ret != HD_OK) {
				printf("set enc_out_roi= %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("roi_0");
		}
		break;
		case 1:
		{
			HD_H26XENC_ROI roi_param = {0};
			roi_param.st_roi[2].enable = 1;
			roi_param.st_roi[2].qp     = 33;
			roi_param.st_roi[2].mode   = HD_VIDEOENC_QPMODE_FIXED_QP;
			roi_param.st_roi[2].rect.x = 32;
			roi_param.st_roi[2].rect.y = 80;
			roi_param.st_roi[2].rect.w = 48;
			roi_param.st_roi[2].rect.h = 64;
			roi_param.st_roi[7].enable = 1;
			roi_param.st_roi[7].qp     = -4;
			roi_param.st_roi[7].mode   = HD_VIDEOENC_QPMODE_DELTA;
			roi_param.st_roi[7].rect.x = 160;
			roi_param.st_roi[7].rect.y = 176;
			roi_param.st_roi[7].rect.w = 16;
			roi_param.st_roi[7].rect.h = 80;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROI, &roi_param);
			if (ret != HD_OK) {
				printf("set enc_out_roi= %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("roi__2_1_33_3_32_80_48_64__7_1_-4_0_160_176_16_80");
		}
		break;
		case 2:
		{
			HD_H26XENC_ROI roi_param = {0};
			roi_param.st_roi[2].enable = 1;
			roi_param.st_roi[2].qp     = 33;
			roi_param.st_roi[2].mode   = HD_VIDEOENC_QPMODE_FIXED_QP;
			roi_param.st_roi[2].rect.x = 64;
			roi_param.st_roi[2].rect.y = 128;
			roi_param.st_roi[2].rect.w = 128;
			roi_param.st_roi[2].rect.h = 64;
			roi_param.st_roi[7].enable = 1;
			roi_param.st_roi[7].qp     = -4;
			roi_param.st_roi[7].mode   = HD_VIDEOENC_QPMODE_DELTA;
			roi_param.st_roi[7].rect.x = 192;
			roi_param.st_roi[7].rect.y = 64;
			roi_param.st_roi[7].rect.w = 64;
			roi_param.st_roi[7].rect.h = 128;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROI, &roi_param);
			if (ret != HD_OK) {
				printf("set enc_out_roi= %d\r\n", ret);
				return ret;
			}

			SET_TEST_NAME("roi__2_1_33_3_64_128_128_64__7_1_-4_0_192_64_64_128");
		}
		break;
#if 0
		case 81:  // only for 680
		{
			HD_H26XENC_ROI roi_param = {0};
			printf("[TEST_ERR] test wrong roi_qp_mode =>\r\n");
			roi_param.roi_qp_mode      = 4;  // invalid roi_qp_mode
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROI, &roi_param);
			if (ret != HD_OK) {
				printf("set enc_out_roi= %d\r\n", ret);
				return ret;
			}
		}
		break;
#endif
		case 82:
		{
			HD_H26XENC_ROI roi_param = {0};
			printf("[TEST_ERR] test wrong st_roi[xx].enable =>\r\n");
			roi_param.st_roi[2].enable = 2;  // invalid enable
			roi_param.st_roi[2].qp     = 33;
			roi_param.st_roi[2].mode   = HD_VIDEOENC_QPMODE_FIXED_QP;
			roi_param.st_roi[2].rect.x = 64;
			roi_param.st_roi[2].rect.y = 128;
			roi_param.st_roi[2].rect.w = 128;
			roi_param.st_roi[2].rect.h = 64;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROI, &roi_param);
			if (ret != HD_OK) {
				printf("set enc_out_roi= %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 83:
		{
			HD_H26XENC_ROI roi_param = {0};
			printf("[TEST_ERR] test wrong st_roi[xx].mode =>\r\n");
			roi_param.st_roi[2].enable = 1;
			roi_param.st_roi[2].qp     = 33;
			roi_param.st_roi[2].mode   = 4;  // invalid mode
			roi_param.st_roi[2].rect.x = 64;
			roi_param.st_roi[2].rect.y = 128;
			roi_param.st_roi[2].rect.w = 128;
			roi_param.st_roi[2].rect.h = 64;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROI, &roi_param);
			if (ret != HD_OK) {
				printf("set enc_out_roi= %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 84:
		{
			HD_H26XENC_ROI roi_param = {0};
			printf("[TEST_ERR] test wrong st_roi[xx].qp(fix) =>\r\n");
			roi_param.st_roi[2].enable = 1;
			roi_param.st_roi[2].qp     = 52;  // invalid qp(fix)
			roi_param.st_roi[2].mode   = HD_VIDEOENC_QPMODE_FIXED_QP;
			roi_param.st_roi[2].rect.x = 64;
			roi_param.st_roi[2].rect.y = 128;
			roi_param.st_roi[2].rect.w = 128;
			roi_param.st_roi[2].rect.h = 64;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROI, &roi_param);
			if (ret != HD_OK) {
				printf("set enc_out_roi= %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 85:
		{
			HD_H26XENC_ROI roi_param = {0};
			printf("[TEST_ERR] test wrong st_roi[xx].qp(delta_+) =>\r\n");
			roi_param.st_roi[2].enable = 1;
			roi_param.st_roi[2].qp     = 32;  // invalid qp(delta)
			roi_param.st_roi[2].mode   = HD_VIDEOENC_QPMODE_DELTA;
			roi_param.st_roi[2].rect.x = 64;
			roi_param.st_roi[2].rect.y = 128;
			roi_param.st_roi[2].rect.w = 128;
			roi_param.st_roi[2].rect.h = 64;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROI, &roi_param);
			if (ret != HD_OK) {
				printf("set enc_out_roi= %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 86:
		{
			HD_H26XENC_ROI roi_param = {0};
			printf("[TEST_ERR] test wrong st_roi[xx].qp(delta_-) =>\r\n");
			roi_param.st_roi[2].enable = 1;
			roi_param.st_roi[2].qp     = -33;  // invalid qp(delta)
			roi_param.st_roi[2].mode   = HD_VIDEOENC_QPMODE_DELTA;
			roi_param.st_roi[2].rect.x = 64;
			roi_param.st_roi[2].rect.y = 128;
			roi_param.st_roi[2].rect.w = 128;
			roi_param.st_roi[2].rect.h = 64;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ROI, &roi_param);
			if (ret != HD_OK) {
				printf("set enc_out_roi= %d\r\n", ret);
				return ret;
			}
		}
		break;
		default:
			break;
	}

	// start videoenc
	hd_videoenc_start(p_stream0->enc_path);

	// start thread & write bitstream file
	start_encode_thread(p_stream0);   // this must call after hd_videoenc_start()

	return 0;
}

static int change_ve_out_prolv(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice =0;
	HD_RESULT ret = HD_OK;

	// check H26x
	if (FALSE == is_h26x_encoding()) {
		printf("ERROR : this function only available for H26x\n");
		return 0;
	}

	// get setup choice
	printf("[out_codec] Please enter which codec\r\n------------------------------\r\n00 : H264_HIGH_42_CAVLC\r\n01 : H264_MAIN_32_CABAC\r\n02 : H265_MAIN_63_CABAC\r\n");
	choice = hd_read_decimal_key_input("");

	// stop videoenc
	hd_videoenc_stop(p_stream0->enc_path);

	// stop previous thread & close bitstream file
	stop_encode_thread(p_stream0);

	// set new param
	switch (choice) {
		case 0:
		{
			HD_VIDEOENC_OUT out_param = {0};

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.codec_type        = HD_CODEC_TYPE_H264;
			out_param.h26x.profile      = HD_H264E_HIGH_PROFILE;
			out_param.h26x.level_idc    = HD_H264E_LEVEL_4_2;
			out_param.h26x.entropy_mode = HD_H264E_CAVLC_CODING;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_out_param0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("H264_HIGH_42_CAVLC");
		}
		break;
		case 1:
		{
			HD_VIDEOENC_OUT out_param = {0};

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.codec_type        = HD_CODEC_TYPE_H264;
			out_param.h26x.profile      = HD_H264E_MAIN_PROFILE;
			out_param.h26x.level_idc    = HD_H264E_LEVEL_3_2;
			out_param.h26x.entropy_mode = HD_H264E_CABAC_CODING;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_out_param0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("H264_MAIN_32_CABAC");
		}
		break;
		case 2:
		{
			HD_VIDEOENC_OUT out_param = {0};

			hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			out_param.codec_type        = HD_CODEC_TYPE_H265;
			out_param.h26x.profile      = HD_H265E_MAIN_PROFILE;
			out_param.h26x.level_idc    = HD_H265E_LEVEL_2_1;
			out_param.h26x.entropy_mode = HD_H265E_CABAC_CODING;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &out_param);
			if (ret != HD_OK) {
				printf("set enc_out_param0 = %d\r\n", ret);
				return ret;
			}
			SET_TEST_NAME("H265_MAIN_63_CABAC");
		}
		break;
		default:
			break;
	}

	// start videoenc
	hd_videoenc_start(p_stream0->enc_path);

	// start thread & write bitstream file
	start_encode_thread(p_stream0);   // this must call after hd_videoenc_start()

	return 0;
}

static int change_ve_out_rc(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice = 0;
	HD_RESULT ret = HD_OK;

	// check H26x
	if (FALSE == is_h26x_encoding()) {
		printf("ERROR : this function only available for H26x\n");
		return 0;
	}

	// get setup choice
	printf("[out_rc] Please enter which rc\r\n------------------------------\r\n00 : CBR(2Mb/s)\r\n01 : CBR(1Mb/s)\r\n02 : CBR(512kb/s)\r\n03 : VBR(2Mb/s)\r\n04 : VBR(512kb/s)\r\n05 : FIXQP(20,20)\r\n06 : FIXQP(45,45)\r\n");
	printf("*************************\n");
	printf("31 : wrong rc_mode\r\n");
	printf("*************************\n");
	printf("41 : wrong vbr.bitrate\r\n");
	printf("42 : wrong vbr.frame_rate_incr\r\n");
	printf("43 : wrong vbr.i_qp\r\n");
	printf("44 : wrong vbr.p_qp\r\n");
	printf("45 : wrong vbr.static_time\r\n");
	printf("46 : wrong vbr.ip_weight\r\n");
	printf("47 : wrong vbr.key_p_period\r\n");
	printf("48 : wrong vbr.kp_weight\r\n");
	printf("49 : wrong vbr.p2_weight\r\n");
	printf("50 : wrong vbr.p3_weight\r\n");
	printf("51 : wrong vbr.lt_weight\r\n");
	printf("52 : wrong vbr.motion_aq_str\r\n");
	printf("53 : wrong vbr.change_pos\r\n");
	printf("*************************\n");
	printf("61 : wrong evbr.bitrate\r\n");
	printf("62 : wrong evbr.frame_rate_incr\r\n");
	printf("63 : wrong evbr.i_qp\r\n");
	printf("64 : wrong evbr.p_qp\r\n");
	printf("65 : wrong evbr.static_time\r\n");
	printf("66 : wrong evbr.ip_weight\r\n");
	printf("67 : wrong evbr.key_p_period\r\n");
	printf("68 : wrong evbr.kp_weight\r\n");
	printf("69 : wrong evbr.p2_weight\r\n");
	printf("70 : wrong evbr.p3_weight\r\n");
	printf("71 : wrong evbr.lt_weight\r\n");
	printf("72 : wrong evbr.motion_aq_str\r\n");
	printf("73 : wrong evbr.still_frame_cnt\r\n");
	printf("74 : wrong evbr.motion_ratio_thd\r\n");
	printf("75 : wrong evbr.still_qp\r\n");
	printf("*************************\n");
	printf("81 : wrong cbr.bitrate\r\n");
	printf("82 : wrong cbr.frame_rate_incr\r\n");
	printf("83 : wrong cbr.i_qp\r\n");
	printf("84 : wrong cbr.p_qp\r\n");
	printf("85 : wrong cbr.static_time\r\n");
	printf("86 : wrong cbr.ip_weight\r\n");
	printf("87 : wrong cbr.key_p_period\r\n");
	printf("88 : wrong cbr.kp_weight\r\n");
	printf("89 : wrong cbr.p2_weight\r\n");
	printf("90 : wrong cbr.p3_weight\r\n");
	printf("91 : wrong cbr.lt_weight\r\n");
	printf("92 : wrong cbr.motion_aq_str\r\n");
	printf("*************************\n");
	printf("101: wrong fixqp.frame_rate_incr\r\n");
	printf("102: wrong fixqp.qp\r\n");

	choice = hd_read_decimal_key_input("");

	// set new param
	switch (choice) {
		case 0:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};

			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = 2 * 1024 * 1024;
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
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 1:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};

			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = 1 * 1024 * 1024;
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
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 2:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};

			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = 512 * 1024;
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
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 3:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};

			rc_param.rc_mode             = HD_RC_MODE_VBR;
			rc_param.vbr.bitrate         = 2 * 1024 * 1024;
			rc_param.vbr.frame_rate_base = 30;
			rc_param.vbr.frame_rate_incr = 1;
			rc_param.vbr.init_i_qp       = 26;
			rc_param.vbr.min_i_qp        = 10;
			rc_param.vbr.max_i_qp        = 45;
			rc_param.vbr.init_p_qp       = 26;
			rc_param.vbr.min_p_qp        = 10;
			rc_param.vbr.max_p_qp        = 45;
			rc_param.vbr.static_time     = 4;
			rc_param.vbr.ip_weight       = 0;
			rc_param.vbr.change_pos      = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 4:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};

			rc_param.rc_mode             = HD_RC_MODE_VBR;
			rc_param.vbr.bitrate         = 512 * 1024;
			rc_param.vbr.frame_rate_base = 30;
			rc_param.vbr.frame_rate_incr = 1;
			rc_param.vbr.init_i_qp       = 26;
			rc_param.vbr.min_i_qp        = 10;
			rc_param.vbr.max_i_qp        = 45;
			rc_param.vbr.init_p_qp       = 26;
			rc_param.vbr.min_p_qp        = 10;
			rc_param.vbr.max_p_qp        = 45;
			rc_param.vbr.static_time     = 4;
			rc_param.vbr.ip_weight       = 0;
			rc_param.vbr.change_pos      = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 5:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};

			rc_param.rc_mode               = HD_RC_MODE_FIX_QP;
			rc_param.fixqp.fix_i_qp        = 20;
			rc_param.fixqp.fix_p_qp        = 20;
			rc_param.fixqp.frame_rate_base = 30;
			rc_param.fixqp.frame_rate_incr = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 6:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};

			rc_param.rc_mode             = HD_RC_MODE_FIX_QP;
			rc_param.fixqp.fix_i_qp      = 45;
			rc_param.fixqp.fix_p_qp      = 45;
			rc_param.fixqp.frame_rate_base = 30;
			rc_param.fixqp.frame_rate_incr = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		/////////////////////
		case 31:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong rc_mode =>\r\n");
			rc_param.rc_mode             = HD_RC_MAX;  // invalid rc_mode
			rc_param.cbr.bitrate         = 2 * 1024 * 1024;
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
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		/////////////////
		case 41:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong vbr.bitrate =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_VBR;
			rc_param.vbr.bitrate         = 128 * 1024 * 1024 + 1; // invalid bitrate
			rc_param.vbr.frame_rate_base = 30;
			rc_param.vbr.frame_rate_incr = 1;
			rc_param.vbr.init_i_qp       = 26;
			rc_param.vbr.min_i_qp        = 10;
			rc_param.vbr.max_i_qp        = 45;
			rc_param.vbr.init_p_qp       = 26;
			rc_param.vbr.min_p_qp        = 10;
			rc_param.vbr.max_p_qp        = 45;
			rc_param.vbr.static_time     = 4;
			rc_param.vbr.ip_weight       = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 42:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong vbr.frame_rate_incr =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_VBR;
			rc_param.vbr.bitrate         = 2 * 1024 * 1024;
			rc_param.vbr.frame_rate_base = 30;
			rc_param.vbr.frame_rate_incr = 0;  // invalid frame_rate_incr
			rc_param.vbr.init_i_qp       = 26;
			rc_param.vbr.min_i_qp        = 10;
			rc_param.vbr.max_i_qp        = 45;
			rc_param.vbr.init_p_qp       = 26;
			rc_param.vbr.min_p_qp        = 10;
			rc_param.vbr.max_p_qp        = 45;
			rc_param.vbr.static_time     = 4;
			rc_param.vbr.ip_weight       = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 43:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong vbr.i_qp =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_VBR;
			rc_param.vbr.bitrate         = 2 * 1024 * 1024;
			rc_param.vbr.frame_rate_base = 30;
			rc_param.vbr.frame_rate_incr = 1;
			rc_param.vbr.init_i_qp       = 53;  // invalid qp
			rc_param.vbr.min_i_qp        = 52;  // invalid qp
			rc_param.vbr.max_i_qp        = 54;  // invalid qp
			rc_param.vbr.init_p_qp       = 26;
			rc_param.vbr.min_p_qp        = 10;
			rc_param.vbr.max_p_qp        = 45;
			rc_param.vbr.static_time     = 4;
			rc_param.vbr.ip_weight       = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 44:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong vbr.p_qp =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_VBR;
			rc_param.vbr.bitrate         = 2 * 1024 * 1024;
			rc_param.vbr.frame_rate_base = 30;
			rc_param.vbr.frame_rate_incr = 1;
			rc_param.vbr.init_i_qp       = 26;
			rc_param.vbr.min_i_qp        = 10;
			rc_param.vbr.max_i_qp        = 45;
			rc_param.vbr.init_p_qp       = 77;  // invalid qp
			rc_param.vbr.min_p_qp        = 66;  // invalid qp
			rc_param.vbr.max_p_qp        = 88;  // invalid qp
			rc_param.vbr.static_time     = 4;
			rc_param.vbr.ip_weight       = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 45:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong vbr.static_time =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_VBR;
			rc_param.vbr.bitrate         = 2 * 1024 * 1024;
			rc_param.vbr.frame_rate_base = 30;
			rc_param.vbr.frame_rate_incr = 1;
			rc_param.vbr.init_i_qp       = 26;
			rc_param.vbr.min_i_qp        = 10;
			rc_param.vbr.max_i_qp        = 45;
			rc_param.vbr.init_p_qp       = 26;
			rc_param.vbr.min_p_qp        = 10;
			rc_param.vbr.max_p_qp        = 45;
			rc_param.vbr.static_time     = 21;  // invalid static_time
			rc_param.vbr.ip_weight       = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 46:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong vbr.ip_weight =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_VBR;
			rc_param.vbr.bitrate         = 2 * 1024 * 1024;
			rc_param.vbr.frame_rate_base = 30;
			rc_param.vbr.frame_rate_incr = 1;
			rc_param.vbr.init_i_qp       = 26;
			rc_param.vbr.min_i_qp        = 10;
			rc_param.vbr.max_i_qp        = 45;
			rc_param.vbr.init_p_qp       = 26;
			rc_param.vbr.min_p_qp        = 10;
			rc_param.vbr.max_p_qp        = 45;
			rc_param.vbr.static_time     = 4;
			rc_param.vbr.ip_weight       = 101; // invalid ip_weight
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 47:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong vbr.key_p_period =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_VBR;
			rc_param.vbr.bitrate         = 2 * 1024 * 1024;
			rc_param.vbr.frame_rate_base = 30;
			rc_param.vbr.frame_rate_incr = 1;
			rc_param.vbr.init_i_qp       = 26;
			rc_param.vbr.min_i_qp        = 10;
			rc_param.vbr.max_i_qp        = 45;
			rc_param.vbr.init_p_qp       = 26;
			rc_param.vbr.min_p_qp        = 10;
			rc_param.vbr.max_p_qp        = 45;
			rc_param.vbr.static_time     = 4;
			rc_param.vbr.ip_weight       = 0;
			rc_param.vbr.key_p_period    = 4097;  // invalid key_p_period
			rc_param.vbr.kp_weight       = 0;
			rc_param.vbr.p2_weight       = 0;
			rc_param.vbr.p3_weight       = 0;
			rc_param.vbr.lt_weight       = 0;
			rc_param.vbr.motion_aq_str   = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 48:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong vbr.kp_weight =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_VBR;
			rc_param.vbr.bitrate         = 2 * 1024 * 1024;
			rc_param.vbr.frame_rate_base = 30;
			rc_param.vbr.frame_rate_incr = 1;
			rc_param.vbr.init_i_qp       = 26;
			rc_param.vbr.min_i_qp        = 10;
			rc_param.vbr.max_i_qp        = 45;
			rc_param.vbr.init_p_qp       = 26;
			rc_param.vbr.min_p_qp        = 10;
			rc_param.vbr.max_p_qp        = 45;
			rc_param.vbr.static_time     = 4;
			rc_param.vbr.ip_weight       = 0;
			rc_param.vbr.key_p_period    = 0;
			rc_param.vbr.kp_weight       = 101; // invalid kp_weight
			rc_param.vbr.p2_weight       = 0;
			rc_param.vbr.p3_weight       = 0;
			rc_param.vbr.lt_weight       = 0;
			rc_param.vbr.motion_aq_str   = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 49:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong vbr.p2_weight =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_VBR;
			rc_param.vbr.bitrate         = 2 * 1024 * 1024;
			rc_param.vbr.frame_rate_base = 30;
			rc_param.vbr.frame_rate_incr = 1;
			rc_param.vbr.init_i_qp       = 26;
			rc_param.vbr.min_i_qp        = 10;
			rc_param.vbr.max_i_qp        = 45;
			rc_param.vbr.init_p_qp       = 26;
			rc_param.vbr.min_p_qp        = 10;
			rc_param.vbr.max_p_qp        = 45;
			rc_param.vbr.static_time     = 4;
			rc_param.vbr.ip_weight       = 0;
			rc_param.vbr.key_p_period    = 0;
			rc_param.vbr.kp_weight       = 0;
			rc_param.vbr.p2_weight       = 101;  // invalid p2_weight
			rc_param.vbr.p3_weight       = 0;
			rc_param.vbr.lt_weight       = 0;
			rc_param.vbr.motion_aq_str   = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 50:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong vbr.p3_weight =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_VBR;
			rc_param.vbr.bitrate         = 2 * 1024 * 1024;
			rc_param.vbr.frame_rate_base = 30;
			rc_param.vbr.frame_rate_incr = 1;
			rc_param.vbr.init_i_qp       = 26;
			rc_param.vbr.min_i_qp        = 10;
			rc_param.vbr.max_i_qp        = 45;
			rc_param.vbr.init_p_qp       = 26;
			rc_param.vbr.min_p_qp        = 10;
			rc_param.vbr.max_p_qp        = 45;
			rc_param.vbr.static_time     = 4;
			rc_param.vbr.ip_weight       = 0;
			rc_param.vbr.key_p_period    = 0;
			rc_param.vbr.kp_weight       = 0;
			rc_param.vbr.p2_weight       = 0;
			rc_param.vbr.p3_weight       = 101;  // invalid p3_weight
			rc_param.vbr.lt_weight       = 0;
			rc_param.vbr.motion_aq_str   = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 51:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong vbr.lt_weight =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_VBR;
			rc_param.vbr.bitrate         = 2 * 1024 * 1024;
			rc_param.vbr.frame_rate_base = 30;
			rc_param.vbr.frame_rate_incr = 1;
			rc_param.vbr.init_i_qp       = 26;
			rc_param.vbr.min_i_qp        = 10;
			rc_param.vbr.max_i_qp        = 45;
			rc_param.vbr.init_p_qp       = 26;
			rc_param.vbr.min_p_qp        = 10;
			rc_param.vbr.max_p_qp        = 45;
			rc_param.vbr.static_time     = 4;
			rc_param.vbr.ip_weight       = 0;
			rc_param.vbr.key_p_period    = 0;
			rc_param.vbr.kp_weight       = 0;
			rc_param.vbr.p2_weight       = 0;
			rc_param.vbr.p3_weight       = 0;
			rc_param.vbr.lt_weight       = 101;  // invalid lt_weight
			rc_param.vbr.motion_aq_str   = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 52:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong vbr.motion_aq_str =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_VBR;
			rc_param.vbr.bitrate         = 2 * 1024 * 1024;
			rc_param.vbr.frame_rate_base = 30;
			rc_param.vbr.frame_rate_incr = 1;
			rc_param.vbr.init_i_qp       = 26;
			rc_param.vbr.min_i_qp        = 10;
			rc_param.vbr.max_i_qp        = 45;
			rc_param.vbr.init_p_qp       = 26;
			rc_param.vbr.min_p_qp        = 10;
			rc_param.vbr.max_p_qp        = 45;
			rc_param.vbr.static_time     = 4;
			rc_param.vbr.ip_weight       = 0;
			rc_param.vbr.key_p_period    = 0;
			rc_param.vbr.kp_weight       = 0;
			rc_param.vbr.p2_weight       = 0;
			rc_param.vbr.p3_weight       = 0;
			rc_param.vbr.lt_weight       = 0;
			rc_param.vbr.motion_aq_str   = 16; // invalid motion_aq_str
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 53:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong vbr.change_pos =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_VBR;
			rc_param.vbr.bitrate         = 2 * 1024 * 1024;
			rc_param.vbr.frame_rate_base = 30;
			rc_param.vbr.frame_rate_incr = 1;
			rc_param.vbr.init_i_qp       = 26;
			rc_param.vbr.min_i_qp        = 10;
			rc_param.vbr.max_i_qp        = 45;
			rc_param.vbr.init_p_qp       = 26;
			rc_param.vbr.min_p_qp        = 10;
			rc_param.vbr.max_p_qp        = 45;
			rc_param.vbr.static_time     = 4;
			rc_param.vbr.ip_weight       = 0;
			rc_param.vbr.key_p_period    = 0;
			rc_param.vbr.kp_weight       = 0;
			rc_param.vbr.p2_weight       = 0;
			rc_param.vbr.p3_weight       = 0;
			rc_param.vbr.lt_weight       = 0;
			rc_param.vbr.motion_aq_str   = 0;
			rc_param.vbr.change_pos      = 101; // invalid change_pos
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		////////////////////////
		case 61:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong evbr.bitrate =>\r\n");
			rc_param.rc_mode              = HD_RC_MODE_EVBR;
			rc_param.evbr.bitrate         = 128 * 1024 * 1024 + 1; // invalid bitrate
			rc_param.evbr.frame_rate_base = 30;
			rc_param.evbr.frame_rate_incr = 1;
			rc_param.evbr.init_i_qp       = 26;
			rc_param.evbr.min_i_qp        = 10;
			rc_param.evbr.max_i_qp        = 45;
			rc_param.evbr.init_p_qp       = 26;
			rc_param.evbr.min_p_qp        = 10;
			rc_param.evbr.max_p_qp        = 45;
			rc_param.evbr.static_time     = 4;
			rc_param.evbr.ip_weight       = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 62:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong evbr.frame_rate_incr =>\r\n");
			rc_param.rc_mode              = HD_RC_MODE_EVBR;
			rc_param.evbr.bitrate         = 2 * 1024 * 1024;
			rc_param.evbr.frame_rate_base = 30;
			rc_param.evbr.frame_rate_incr = 0;  // invalid frame_rate_incr
			rc_param.evbr.init_i_qp       = 26;
			rc_param.evbr.min_i_qp        = 10;
			rc_param.evbr.max_i_qp        = 45;
			rc_param.evbr.init_p_qp       = 26;
			rc_param.evbr.min_p_qp        = 10;
			rc_param.evbr.max_p_qp        = 45;
			rc_param.evbr.static_time     = 4;
			rc_param.evbr.ip_weight       = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 63:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong evbr.i_qp =>\r\n");
			rc_param.rc_mode              = HD_RC_MODE_EVBR;
			rc_param.evbr.bitrate         = 2 * 1024 * 1024;
			rc_param.evbr.frame_rate_base = 30;
			rc_param.evbr.frame_rate_incr = 1;
			rc_param.evbr.init_i_qp       = 53;  // invalid qp
			rc_param.evbr.min_i_qp        = 52;  // invalid qp
			rc_param.evbr.max_i_qp        = 54;  // invalid qp
			rc_param.evbr.init_p_qp       = 26;
			rc_param.evbr.min_p_qp        = 10;
			rc_param.evbr.max_p_qp        = 45;
			rc_param.evbr.static_time     = 4;
			rc_param.evbr.ip_weight       = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 64:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong evbr.p_qp =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_EVBR;
			rc_param.evbr.bitrate         = 2 * 1024 * 1024;
			rc_param.evbr.frame_rate_base = 30;
			rc_param.evbr.frame_rate_incr = 1;
			rc_param.evbr.init_i_qp       = 26;
			rc_param.evbr.min_i_qp        = 10;
			rc_param.evbr.max_i_qp        = 45;
			rc_param.evbr.init_p_qp       = 77;  // invalid qp
			rc_param.evbr.min_p_qp        = 66;  // invalid qp
			rc_param.evbr.max_p_qp        = 88;  // invalid qp
			rc_param.evbr.static_time     = 4;
			rc_param.evbr.ip_weight       = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 65:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong evbr.static_time =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_EVBR;
			rc_param.evbr.bitrate         = 2 * 1024 * 1024;
			rc_param.evbr.frame_rate_base = 30;
			rc_param.evbr.frame_rate_incr = 1;
			rc_param.evbr.init_i_qp       = 26;
			rc_param.evbr.min_i_qp        = 10;
			rc_param.evbr.max_i_qp        = 45;
			rc_param.evbr.init_p_qp       = 26;
			rc_param.evbr.min_p_qp        = 10;
			rc_param.evbr.max_p_qp        = 45;
			rc_param.evbr.static_time     = 21;  // invalid static_time
			rc_param.evbr.ip_weight       = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 66:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong evbr.ip_weight =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_EVBR;
			rc_param.evbr.bitrate         = 2 * 1024 * 1024;
			rc_param.evbr.frame_rate_base = 30;
			rc_param.evbr.frame_rate_incr = 1;
			rc_param.evbr.init_i_qp       = 26;
			rc_param.evbr.min_i_qp        = 10;
			rc_param.evbr.max_i_qp        = 45;
			rc_param.evbr.init_p_qp       = 26;
			rc_param.evbr.min_p_qp        = 10;
			rc_param.evbr.max_p_qp        = 45;
			rc_param.evbr.static_time     = 4;
			rc_param.evbr.ip_weight       = 101; // invalid ip_weight
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 67:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong evbr.key_p_period =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_EVBR;
			rc_param.evbr.bitrate         = 2 * 1024 * 1024;
			rc_param.evbr.frame_rate_base = 30;
			rc_param.evbr.frame_rate_incr = 1;
			rc_param.evbr.init_i_qp       = 26;
			rc_param.evbr.min_i_qp        = 10;
			rc_param.evbr.max_i_qp        = 45;
			rc_param.evbr.init_p_qp       = 26;
			rc_param.evbr.min_p_qp        = 10;
			rc_param.evbr.max_p_qp        = 45;
			rc_param.evbr.static_time     = 4;
			rc_param.evbr.ip_weight       = 0;
			rc_param.evbr.key_p_period    = 4097;  // invalid key_p_period
			rc_param.evbr.kp_weight       = 0;
			rc_param.evbr.p2_weight       = 0;
			rc_param.evbr.p3_weight       = 0;
			rc_param.evbr.lt_weight       = 0;
			rc_param.evbr.motion_aq_str   = -6;
			rc_param.evbr.motion_ratio_thd = 30;
			rc_param.evbr.still_frame_cnd = 100;
			rc_param.evbr.still_i_qp      = 28;
			rc_param.evbr.still_p_qp      = 36;
			rc_param.evbr.still_kp_qp     = 30;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 68:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong evbr.kp_weight =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_EVBR;
			rc_param.evbr.bitrate         = 2 * 1024 * 1024;
			rc_param.evbr.frame_rate_base = 30;
			rc_param.evbr.frame_rate_incr = 1;
			rc_param.evbr.init_i_qp       = 26;
			rc_param.evbr.min_i_qp        = 10;
			rc_param.evbr.max_i_qp        = 45;
			rc_param.evbr.init_p_qp       = 26;
			rc_param.evbr.min_p_qp        = 10;
			rc_param.evbr.max_p_qp        = 45;
			rc_param.evbr.static_time     = 4;
			rc_param.evbr.ip_weight       = 0;
			rc_param.evbr.key_p_period    = 0;
			rc_param.evbr.kp_weight       = 101; // invalid kp_weight
			rc_param.evbr.p2_weight       = 0;
			rc_param.evbr.p3_weight       = 0;
			rc_param.evbr.lt_weight       = 0;
			rc_param.evbr.motion_aq_str   = 0;
			rc_param.evbr.motion_ratio_thd = 30;
			rc_param.evbr.still_frame_cnd = 100;
			rc_param.evbr.still_i_qp      = 28;
			rc_param.evbr.still_p_qp      = 36;
			rc_param.evbr.still_kp_qp     = 30;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 69:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong evbr.p2_weight =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_EVBR;
			rc_param.evbr.bitrate         = 2 * 1024 * 1024;
			rc_param.evbr.frame_rate_base = 30;
			rc_param.evbr.frame_rate_incr = 1;
			rc_param.evbr.init_i_qp       = 26;
			rc_param.evbr.min_i_qp        = 10;
			rc_param.evbr.max_i_qp        = 45;
			rc_param.evbr.init_p_qp       = 26;
			rc_param.evbr.min_p_qp        = 10;
			rc_param.evbr.max_p_qp        = 45;
			rc_param.evbr.static_time     = 4;
			rc_param.evbr.ip_weight       = 0;
			rc_param.evbr.key_p_period    = 0;
			rc_param.evbr.kp_weight       = 0;
			rc_param.evbr.p2_weight       = 101;  // invalid p2_weight
			rc_param.evbr.p3_weight       = 0;
			rc_param.evbr.lt_weight       = 0;
			rc_param.evbr.motion_aq_str   = 0;
			rc_param.evbr.motion_ratio_thd = 30;
			rc_param.evbr.still_frame_cnd = 100;
			rc_param.evbr.still_i_qp      = 28;
			rc_param.evbr.still_p_qp      = 36;
			rc_param.evbr.still_kp_qp     = 30;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 70:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong evbr.p3_weight =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_EVBR;
			rc_param.evbr.bitrate         = 2 * 1024 * 1024;
			rc_param.evbr.frame_rate_base = 30;
			rc_param.evbr.frame_rate_incr = 1;
			rc_param.evbr.init_i_qp       = 26;
			rc_param.evbr.min_i_qp        = 10;
			rc_param.evbr.max_i_qp        = 45;
			rc_param.evbr.init_p_qp       = 26;
			rc_param.evbr.min_p_qp        = 10;
			rc_param.evbr.max_p_qp        = 45;
			rc_param.evbr.static_time     = 4;
			rc_param.evbr.ip_weight       = 0;
			rc_param.evbr.key_p_period    = 0;
			rc_param.evbr.kp_weight       = 0;
			rc_param.evbr.p2_weight       = 0;
			rc_param.evbr.p3_weight       = 101;  // invalid p3_weight
			rc_param.evbr.lt_weight       = 0;
			rc_param.evbr.motion_aq_str   = 0;
			rc_param.evbr.motion_ratio_thd = 30;
			rc_param.evbr.still_frame_cnd = 100;
			rc_param.evbr.still_i_qp      = 28;
			rc_param.evbr.still_p_qp      = 36;
			rc_param.evbr.still_kp_qp     = 30;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 71:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong evbr.lt_weight =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_EVBR;
			rc_param.evbr.bitrate         = 2 * 1024 * 1024;
			rc_param.evbr.frame_rate_base = 30;
			rc_param.evbr.frame_rate_incr = 1;
			rc_param.evbr.init_i_qp       = 26;
			rc_param.evbr.min_i_qp        = 10;
			rc_param.evbr.max_i_qp        = 45;
			rc_param.evbr.init_p_qp       = 26;
			rc_param.evbr.min_p_qp        = 10;
			rc_param.evbr.max_p_qp        = 45;
			rc_param.evbr.static_time     = 4;
			rc_param.evbr.ip_weight       = 0;
			rc_param.evbr.key_p_period    = 0;
			rc_param.evbr.kp_weight       = 0;
			rc_param.evbr.p2_weight       = 0;
			rc_param.evbr.p3_weight       = 0;
			rc_param.evbr.lt_weight       = 101;  // invalid lt_weight
			rc_param.evbr.motion_aq_str   = 0;
			rc_param.evbr.motion_ratio_thd = 30;
			rc_param.evbr.still_frame_cnd = 100;
			rc_param.evbr.still_i_qp      = 28;
			rc_param.evbr.still_p_qp      = 36;
			rc_param.evbr.still_kp_qp     = 30;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 72:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong evbr.motion_aq_str =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_EVBR;
			rc_param.evbr.bitrate         = 2 * 1024 * 1024;
			rc_param.evbr.frame_rate_base = 30;
			rc_param.evbr.frame_rate_incr = 1;
			rc_param.evbr.init_i_qp       = 26;
			rc_param.evbr.min_i_qp        = 10;
			rc_param.evbr.max_i_qp        = 45;
			rc_param.evbr.init_p_qp       = 26;
			rc_param.evbr.min_p_qp        = 10;
			rc_param.evbr.max_p_qp        = 45;
			rc_param.evbr.static_time     = 4;
			rc_param.evbr.ip_weight       = 0;
			rc_param.evbr.key_p_period    = 0;
			rc_param.evbr.kp_weight       = 0;
			rc_param.evbr.p2_weight       = 0;
			rc_param.evbr.p3_weight       = 0;
			rc_param.evbr.lt_weight       = 0;
			rc_param.evbr.motion_aq_str   = 16; // invalid motion_aq_str
			rc_param.evbr.motion_ratio_thd = 30;
			rc_param.evbr.still_frame_cnd = 100;
			rc_param.evbr.still_i_qp      = 28;
			rc_param.evbr.still_p_qp      = 36;
			rc_param.evbr.still_kp_qp     = 30;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 73:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong evbr.still_frame_cnd =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_EVBR;
			rc_param.evbr.bitrate         = 2 * 1024 * 1024;
			rc_param.evbr.frame_rate_base = 30;
			rc_param.evbr.frame_rate_incr = 1;
			rc_param.evbr.init_i_qp       = 26;
			rc_param.evbr.min_i_qp        = 10;
			rc_param.evbr.max_i_qp        = 45;
			rc_param.evbr.init_p_qp       = 26;
			rc_param.evbr.min_p_qp        = 10;
			rc_param.evbr.max_p_qp        = 45;
			rc_param.evbr.static_time     = 4;
			rc_param.evbr.ip_weight       = 0;
			rc_param.evbr.key_p_period    = 0;
			rc_param.evbr.kp_weight       = 0;
			rc_param.evbr.p2_weight       = 0;
			rc_param.evbr.p3_weight       = 0;
			rc_param.evbr.lt_weight       = 0;
			rc_param.evbr.motion_aq_str   = 0;
			rc_param.evbr.motion_ratio_thd = 30;
			rc_param.evbr.still_frame_cnd = 4097;  // invalid still_frame_cnd
			rc_param.evbr.still_i_qp      = 28;
			rc_param.evbr.still_p_qp      = 36;
			rc_param.evbr.still_kp_qp     = 30;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 74:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong evbr.motion_ratio_thd =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_EVBR;
			rc_param.evbr.bitrate         = 2 * 1024 * 1024;
			rc_param.evbr.frame_rate_base = 30;
			rc_param.evbr.frame_rate_incr = 1;
			rc_param.evbr.init_i_qp       = 26;
			rc_param.evbr.min_i_qp        = 10;
			rc_param.evbr.max_i_qp        = 45;
			rc_param.evbr.init_p_qp       = 26;
			rc_param.evbr.min_p_qp        = 10;
			rc_param.evbr.max_p_qp        = 45;
			rc_param.evbr.static_time     = 4;
			rc_param.evbr.ip_weight       = 0;
			rc_param.evbr.key_p_period    = 0;
			rc_param.evbr.kp_weight       = 0;
			rc_param.evbr.p2_weight       = 0;
			rc_param.evbr.p3_weight       = 0;
			rc_param.evbr.lt_weight       = 0;
			rc_param.evbr.motion_aq_str   = 0;
			rc_param.evbr.motion_ratio_thd = 101;  // invalid motion_ratio_thd
			rc_param.evbr.still_frame_cnd = 100;
			rc_param.evbr.still_i_qp      = 28;
			rc_param.evbr.still_p_qp      = 36;
			rc_param.evbr.still_kp_qp     = 30;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 75:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong evbr.still_qp =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_EVBR;
			rc_param.evbr.bitrate         = 2 * 1024 * 1024;
			rc_param.evbr.frame_rate_base = 30;
			rc_param.evbr.frame_rate_incr = 1;
			rc_param.evbr.init_i_qp       = 26;
			rc_param.evbr.min_i_qp        = 10;
			rc_param.evbr.max_i_qp        = 45;
			rc_param.evbr.init_p_qp       = 26;
			rc_param.evbr.min_p_qp        = 10;
			rc_param.evbr.max_p_qp        = 45;
			rc_param.evbr.static_time     = 4;
			rc_param.evbr.ip_weight       = 0;
			rc_param.evbr.key_p_period    = 0;
			rc_param.evbr.kp_weight       = 0;
			rc_param.evbr.p2_weight       = 0;
			rc_param.evbr.p3_weight       = 0;
			rc_param.evbr.lt_weight       = 0;
			rc_param.evbr.motion_aq_str   = 0;
			rc_param.evbr.motion_ratio_thd = 30;
			rc_param.evbr.still_frame_cnd = 100;
			rc_param.evbr.still_i_qp      = 67;  // invalid still_qp
			rc_param.evbr.still_p_qp      = 56;  // invalid still_qp
			rc_param.evbr.still_kp_qp     = 78;  // invalid still_qp
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		//////////////////////////
		case 81:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong cbr.bitrate =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = 128 * 1024 * 1024 + 1; // invalid bitrate
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
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 82:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong cbr.frame_rate_incr =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = 2 * 1024 * 1024;
			rc_param.cbr.frame_rate_base = 30;
			rc_param.cbr.frame_rate_incr = 0;  // invalid frame_rate_incr
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 83:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong cbr.i_qp =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = 2 * 1024 * 1024;
			rc_param.cbr.frame_rate_base = 30;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 53;  // invalid qp
			rc_param.cbr.min_i_qp        = 52;  // invalid qp
			rc_param.cbr.max_i_qp        = 54;  // invalid qp
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 84:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong cbr.p_qp =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = 2 * 1024 * 1024;
			rc_param.cbr.frame_rate_base = 30;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 77;  // invalid qp
			rc_param.cbr.min_p_qp        = 66;  // invalid qp
			rc_param.cbr.max_p_qp        = 88;  // invalid qp
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 85:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong cbr.static_time =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = 2 * 1024 * 1024;
			rc_param.cbr.frame_rate_base = 30;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 21;  // invalid static_time
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 86:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong cbr.ip_weight =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = 2 * 1024 * 1024;
			rc_param.cbr.frame_rate_base = 30;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 101; // invalid ip_weight
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 87:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong cbr.key_p_period =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = 2 * 1024 * 1024;
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
			rc_param.cbr.key_p_period    = 4097;  // invalid key_p_period
			rc_param.cbr.kp_weight       = 0;
			rc_param.cbr.p2_weight       = 0;
			rc_param.cbr.p3_weight       = 0;
			rc_param.cbr.lt_weight       = 0;
			rc_param.cbr.motion_aq_str   = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 88:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong cbr.kp_weight =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = 2 * 1024 * 1024;
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
			rc_param.cbr.key_p_period    = 0;
			rc_param.cbr.kp_weight       = 101; // invalid kp_weight
			rc_param.cbr.p2_weight       = 0;
			rc_param.cbr.p3_weight       = 0;
			rc_param.cbr.lt_weight       = 0;
			rc_param.cbr.motion_aq_str   = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 89:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong cbr.p2_weight =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = 2 * 1024 * 1024;
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
			rc_param.cbr.key_p_period    = 0;
			rc_param.cbr.kp_weight       = 0;
			rc_param.cbr.p2_weight       = 101;  // invalid p2_weight
			rc_param.cbr.p3_weight       = 0;
			rc_param.cbr.lt_weight       = 0;
			rc_param.cbr.motion_aq_str   = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 90:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong cbr.p3_weight =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = 2 * 1024 * 1024;
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
			rc_param.cbr.key_p_period    = 0;
			rc_param.cbr.kp_weight       = 0;
			rc_param.cbr.p2_weight       = 0;
			rc_param.cbr.p3_weight       = 101;  // invalid p3_weight
			rc_param.cbr.lt_weight       = 0;
			rc_param.cbr.motion_aq_str   = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 91:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong cbr.lt_weight =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = 2 * 1024 * 1024;
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
			rc_param.cbr.key_p_period    = 0;
			rc_param.cbr.kp_weight       = 0;
			rc_param.cbr.p2_weight       = 0;
			rc_param.cbr.p3_weight       = 0;
			rc_param.cbr.lt_weight       = 101;  // invalid lt_weight
			rc_param.cbr.motion_aq_str   = 0;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 92:
		{
			HD_H26XENC_RATE_CONTROL2 rc_param = {0};
			printf("[TEST_ERR] test wrong cbr.motion_aq_str =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = 2 * 1024 * 1024;
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
			rc_param.cbr.key_p_period    = 0;
			rc_param.cbr.kp_weight       = 0;
			rc_param.cbr.p2_weight       = 0;
			rc_param.cbr.p3_weight       = 0;
			rc_param.cbr.lt_weight       = 0;
			rc_param.cbr.motion_aq_str   = 16; // invalid motion_aq_str
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		////////////////
		case 101:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong fixqp.frame_rate_incr =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_FIX_QP;
			rc_param.fixqp.frame_rate_base = 30;
			rc_param.fixqp.frame_rate_incr = 0;  // invalid frame_rate_incr
			rc_param.fixqp.fix_i_qp        = 26;
			rc_param.fixqp.fix_p_qp        = 26;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 102:
		{
			HD_H26XENC_RATE_CONTROL rc_param = {0};
			printf("[TEST_ERR] test wrong fixqp.qp =>\r\n");
			rc_param.rc_mode             = HD_RC_MODE_FIX_QP;
			rc_param.fixqp.frame_rate_base = 30;
			rc_param.fixqp.frame_rate_incr = 1;  
			rc_param.fixqp.fix_i_qp        = 78;  // invalid qp
			rc_param.fixqp.fix_p_qp        = 87;  // invalid qp
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		default:
			printf("wrong choice\r\n");
			return 0;
	}

	// start videoenc
	hd_videoenc_start(p_stream0->enc_path);


	return 0;
}

static int change_ve_out_req_i(void)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)&stream[0];
	UINT32 choice =0;
	HD_RESULT ret = HD_OK;

	// check H26x
	if (FALSE == is_h26x_encoding()) {
		printf("ERROR : this function only available for H26x\n");
		return 0;
	}

	// get setup choice
	printf("[out_codec] Please enter which codec\r\n------------------------------\r\n00 : request_i\r\n");
	printf("81 : wrong enable\r\n");
	choice = hd_read_decimal_key_input("");

	switch (choice)
	{
		case 0:
		// set new param
		{
			HD_H26XENC_REQUEST_IFRAME req_i = {0};
			req_i.enable   = 1;
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME, &req_i);
			if (ret != HD_OK) {
				printf("set_enc_param_out_request_ifrm0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
		case 81:
		// set new param
		{
			HD_H26XENC_REQUEST_IFRAME req_i = {0};
			req_i.enable   = 2;  // invalid enable
			ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME, &req_i);
			if (ret != HD_OK) {
				printf("set_enc_param_out_request_ifrm0 = %d\r\n", ret);
				return ret;
			}
		}
		break;
			
		default:
			break;
	}

	// start videoenc, videoproc
	hd_videoenc_start(p_stream0->enc_path);

	return 0;
}

static HD_DBG_MENU menu_videoenc[] = {
	{  1, "change_ve.in_crop",     change_ve_in_crop,     TRUE},
	{  2, "change_ve.in_dim",      change_ve_in_dim,      TRUE},
	{  3, "change_ve.in_dir",      change_ve_in_dir,      TRUE},
	{  4, "change_ve.in_pxlfmt",   change_ve_in_pxlfmt,   TRUE},
	{  5, "change_ve.out_codec",   change_ve_out_codec,   TRUE},
	{  6, "change_ve.out_prolv",   change_ve_out_prolv,   TRUE},
	{  7, "change_ve.out_gop",     change_ve_out_gop,     TRUE},
	{  8, "change_ve.out_rc",      change_ve_out_rc,      TRUE},
	{  9, "change_ve.out_gray",    change_ve_out_gray,    TRUE},
	{ 10, "change_ve.out_svc",     change_ve_out_svc,     TRUE},
	{ 11, "change_ve.out_ltr",     change_ve_out_ltr,     TRUE},
	{ 12, "change_ve.out_vui",     change_ve_out_vui,     TRUE},
	{ 13, "change_ve.out_db",      change_ve_out_db,      TRUE},
	{ 14, "change_ve.out_slice",   change_ve_out_slice,   TRUE},
	{ 15, "change_ve.out_gdr",     change_ve_out_gdr,     TRUE},
	{ 16, "change_ve.out_aq_rowrc",change_ve_out_aq_rowrc,TRUE},
	{ 17, "change_ve.out_roi",     change_ve_out_roi,     TRUE},
	{100, "change_ve.out_req_i",   change_ve_out_req_i,   TRUE},
	// escape must be last
	{HD_DEBUG_MENU_ID_LAST,  "",    NULL,                  FALSE},
};

static HD_RESULT run_menu(void)
{
	hd_debug_menu_entry_p(menu_videoenc, "VIDEOENC");
	return HD_OK;
}

/////////////////////////////////////////////////////////////////////////////////

EXAMFUNC_ENTRY(hd_videoenc_test, argc, argv)
{
	HD_RESULT ret;
	
	UINT32 enc_type = 0;

	// check TEST pattern exist
	if (check_test_pattern() == FALSE)
	{
		printf("test_pattern isn't exist\r\n");
		return -1;
	}

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
	ret = open_module(&stream[0]);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	// assign parameter by program options
	
	// set videoenc config (main)
	stream[0].enc_max_dim.w = VDO_MAX_SIZE_W;
	stream[0].enc_max_dim.h = VDO_MAX_SIZE_H;
	ret = set_enc_cfg(stream[0].enc_path, &stream[0].enc_max_dim, 2 * 1024 * 1024);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}
	
	// set videoenc parameter (main)
	stream[0].pattern_idx = 0;  // use first pattern as default running
	stream[0].enc_dim.w = g_PatternFileTable[stream[0].pattern_idx].width;
	stream[0].enc_dim.h = g_PatternFileTable[stream[0].pattern_idx].height;
	ret = set_enc_param(stream[0].enc_path, &stream[0].enc_dim, enc_type, 2 * 1024 * 1024);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}

	// create encode_thread (pull_out bitstream) & feed_yuv_thread (push_in frame)
	SET_TEST_NAME("default");
	start_encode_thread((VIDEO_RECORD *)stream);
	start_feed_thread((VIDEO_RECORD *)stream);


	// start video_record modules (main)
	hd_videoenc_start(stream[0].enc_path);

	// let encode_thread start to work
	stream[0].flow_start = 1;

	// RUN TEST MENU - until user hit Quit
	run_menu();

	// stop video_record modules (main)
	hd_videoenc_stop(stream[0].enc_path);

	// uninit
	stop_encode_thread((VIDEO_RECORD *)stream);
	stop_feed_thread((VIDEO_RECORD *)stream);
	
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
