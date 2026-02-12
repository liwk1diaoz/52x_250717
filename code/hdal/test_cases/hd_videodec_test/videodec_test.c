/**
	@brief Sample code of video decode.\n

	@file videodec_test.c

	@author YanYu Chen

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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_videodec_test, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define DEBUG_MENU 1
#define SVC_BITSTREAM_TESTING 0

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////

//header
#define DBGINFO_BUFSIZE()	(0x200)

//RAW
#define VDO_RAW_BUFSIZE(w, h, pxlfmt)   (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NRX: RAW compress: Only support 12bit mode
#define RAW_COMPRESS_RATIO 70
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

//YUV
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	ALIGN_CEIL_4(((w) * (h) * HD_VIDEO_PXLFMT_BPP(pxlfmt)) / 8)
//NVX: YUV compress
#define YUV_COMPRESS_RATIO 70
#define VDO_NVX_BUFSIZE(w, h, pxlfmt)	(VDO_YUV_BUFSIZE(w, h, pxlfmt) * YUV_COMPRESS_RATIO / 100)
 
///////////////////////////////////////////////////////////////////////////////

#define VDO_SIZE_W		640
#define VDO_SIZE_H		480
#define BS_BLK_SIZE     0x200000
#define BS_BLK_DESC_SIZE    0x200     // bitstream buffer size
#define H26X_NAL_MAXSIZE    512
#define VDO_DDR_ID          DDR_ID0

BOOL bRandom_fourcc = 0;
BOOL bRandom_pa = 0;
BOOL bZero_blk = 0;
BOOL bRandom_bssize = 0;
BOOL bErr_dectype = 0;

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
	HD_RESULT      (*p_func)(void);  ///< command function
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
				return p_menu->p_func();
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
///////////////////////////////////////////////////////////////////////////////
extern int vdodec_basic_flow(void);
extern int vdodec_random_fourcc(void);
extern int vdodec_random_pa(void);
extern int vdodec_zero_blk(void);
extern int vdodec_random_bssize(void);
extern int vdodec_err_dectype(void);

//extern int vdocap_crop(void);
//extern int vdocap_out(void);


static HD_DBG_MENU vdodec_test_main_menu[] = {
	{0x01, "open/close/start/stop",  vdodec_basic_flow,              TRUE},
	//{0x07, "clear all error setting",                   vdodec_clear_err_setting,             TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

///////////////////////////////////////////////////////////////////////////////
HD_RESULT mem_init(UINT32 *p_blk_size)
{
	HD_RESULT ret = HD_OK;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (main)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_64(VDO_SIZE_W), ALIGN_CEIL_64(VDO_SIZE_H), HD_VIDEO_PXLFMT_YUV420);  // align to 64 for h265 raw buffer
	mem_cfg.pool_info[0].blk_cnt = 5;
	mem_cfg.pool_info[0].ddr_id = VDO_DDR_ID;
	// config common pool for bs pushing in
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_USER_POOL_BEGIN;
	mem_cfg.pool_info[1].blk_size = BS_BLK_SIZE;
	mem_cfg.pool_info[1].blk_cnt = 1;
	mem_cfg.pool_info[1].ddr_id = VDO_DDR_ID;

	ret = hd_common_mem_init(&mem_cfg);
	*p_blk_size = mem_cfg.pool_info[0].blk_size;
	return ret;
}

HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	ret = hd_common_mem_uninit();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

HD_RESULT set_dec_cfg(HD_PATH_ID video_dec_path, HD_DIM *p_max_dim, UINT32 dec_type)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEODEC_PATH_CONFIG video_path_cfg = {0};

	if (p_max_dim != NULL) {
		// set videodec path config
		video_path_cfg.max_mem.codec_type = dec_type;
		video_path_cfg.max_mem.dim.w = p_max_dim->w;
		video_path_cfg.max_mem.dim.h = p_max_dim->h;
		ret = hd_videodec_set(video_dec_path, HD_VIDEODEC_PARAM_PATH_CONFIG, &video_path_cfg);
	} else {
		ret = HD_ERR_NG;
	}

	return ret;
}

HD_RESULT parse_h26x_desc(UINT32 codec, UINT32 src_addr, UINT32 size, BOOL *is_desc)
{
	HD_RESULT r = HD_OK;
	UINT32 start_code = 0, count = 0;
	UINT8 *ptr8 = NULL;

	if (src_addr == 0) {
		printf("buf_addr is 0\r\n");
		return HD_ERR_NG;
	}
	if (size == 0) {
		printf("size is 0\r\n");
		return HD_ERR_NG;
	}
	if (!is_desc) {
		printf("is_desc is null\r\n");
		return HD_ERR_NG;
	}

	ptr8 = (UINT8 *)src_addr;
	count = size;

	if (codec == HD_CODEC_TYPE_H264) {
		while (count--) {
			// search start code to skip (sps, pps)
			if ((*ptr8 == 0x00) && (*(ptr8 + 1) == 0x00) && (*(ptr8 + 2) == 0x00) && (*(ptr8 + 3) == 0x01)) {
	            start_code++;
			}
			if (start_code == 2) {
				*is_desc = TRUE;
				return HD_OK;
			}
			ptr8++;
		}
	} else if (codec == HD_CODEC_TYPE_H265) {
		while (count--) {
			// search start code to skip (vps, sps, pps)
			if ((*ptr8 == 0x00) && (*(ptr8 + 1) == 0x00) && (*(ptr8 + 2) == 0x00) && (*(ptr8 + 3) == 0x01)) {
	            start_code++;
			}
			if (start_code == 3) {
				*is_desc = TRUE;
				return HD_OK;
			}
			ptr8++;
		}
	} else {
		printf("unknown codec (%d)\r\n", codec);
		return HD_ERR_NG;
	}

	*is_desc = FALSE;
	return r;
}

HD_RESULT set_dec_param(HD_PATH_ID video_dec_path, UINT32 dec_type)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEODEC_IN video_in_param = {0};

	//--- HD_VIDEODEC_PARAM_IN ---
	video_in_param.codec_type = dec_type;
	ret = hd_videodec_set(video_dec_path, HD_VIDEODEC_PARAM_IN, &video_in_param);

	//--- HD_VIDEODEC_PARAM_IN_DESC ---
	if (dec_type == HD_CODEC_TYPE_H264 || dec_type == HD_CODEC_TYPE_H265) {
		//UINT32 desc_len = 0;
		BOOL is_desc = FALSE;
		INT bs_size = 0, read_len = 0;
		char codec_name[8], file_name[128];
		UINT8 desc_buf[H26X_NAL_MAXSIZE+1] = {0};
		FILE *bs_fd, *bslen_fd;

		// assign video codec
		switch (dec_type) {
			case HD_CODEC_TYPE_H264:
				snprintf(codec_name, sizeof(codec_name), "h264");
				break;
			case HD_CODEC_TYPE_H265:
				snprintf(codec_name, sizeof(codec_name), "h265");
				break;
			default:
				printf("invalid video codec(%d)\n", dec_type);
				break;
		}

		// open input files
		sprintf(file_name, "/mnt/sd/video_bs_%d_%d_%s.dat", VDO_SIZE_W, VDO_SIZE_H, codec_name);
		if ((bs_fd = fopen(file_name, "rb")) == NULL) {
			printf("open file (%s) fail !!\r\n", file_name);
			return HD_ERR_SYS;
		}
		snprintf(file_name, sizeof(file_name), "/mnt/sd/video_bs_%d_%d_%s.len", VDO_SIZE_W, VDO_SIZE_H, codec_name);
		if ((bslen_fd = fopen(file_name, "rb")) == NULL) {
			printf("open file (%s) fail !!\r\n", file_name);
			ret = HD_ERR_SYS;
			goto set_desc_q;
		}

		// get bs size
		fscanf(bslen_fd, "%d\n", &bs_size);

		// read bs from file
		read_len = fread((void *)&desc_buf, 1, bs_size, bs_fd);
		desc_buf[read_len] = '\0';
		if (read_len != bs_size) {
			printf("set_desc reading error (read_len=%d, bs_size=%d)\n", read_len, bs_size);
			ret = HD_ERR_SYS;
			goto set_desc_q;
		}

		// parse and get h.26x desc
		parse_h26x_desc(dec_type, (UINT32)&desc_buf, bs_size, &is_desc);
		if (is_desc) {
			HD_H26XDEC_DESC desc_info = {0};
			desc_info.addr = (UINT32)&desc_buf;
			desc_info.len = read_len;
			ret = hd_videodec_set(video_dec_path, HD_VIDEODEC_PARAM_IN_DESC, &desc_info);
		} else {
			printf("invalid desc_addr = 0x%x, len = 0x%x\n", (UINT32)&desc_buf, read_len);
		}

set_desc_q:
		if (bs_fd) fclose(bs_fd);
		if (bslen_fd) fclose(bslen_fd);
	}

	return ret;
}
///////////////////////////////////////////////////////////////////////////////

typedef struct _VIDEO_DECODE {

	// (1) video dec
	HD_VIDEODEC_SYSCAPS  dec_syscaps;
	HD_PATH_ID           dec_path;
	HD_DIM               dec_max_dim;
	UINT32               dec_type;
	UINT32               flow_start;
	UINT32               dec_exit;

	// (2) user push
	pthread_t            feed_thread_id;

	// (3) user pull
	pthread_t            dec_thread_id;
	UINT32               blk_size;

} VIDEO_DECODE;

VIDEO_DECODE stream[1] = {0}; //0: main stream

HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if((ret = hd_videodec_init()) != HD_OK)
		return ret;
	return HD_OK;
}

HD_RESULT open_module(VIDEO_DECODE *p_stream)
{
    HD_RESULT ret;
    if((ret = hd_videodec_open(HD_VIDEODEC_0_IN_0, HD_VIDEODEC_0_OUT_0, &p_stream->dec_path)) != HD_OK)
        return ret;
    return HD_OK;
}

HD_RESULT close_module(VIDEO_DECODE *p_stream)
{
    HD_RESULT ret;
    if((ret = hd_videodec_close(p_stream->dec_path)) != HD_OK)
        return ret;
    return HD_OK;
}

HD_RESULT exit_module(void)
{
    HD_RESULT ret;
    if((ret = hd_videodec_uninit()) != HD_OK)
        return ret;
    return HD_OK;
}

BOOL check_test_pattern(UINT32 dec_type)
{
	FILE *f_in;
	char codec_name[8], filename[64], filepath[128];

	// assign video codec
	switch (dec_type) {
		case HD_CODEC_TYPE_JPEG:	snprintf(codec_name, sizeof(codec_name), "jpeg");		break;
		case HD_CODEC_TYPE_H264:	snprintf(codec_name, sizeof(codec_name), "h264");		break;
		case HD_CODEC_TYPE_H265:	snprintf(codec_name, sizeof(codec_name), "h265");		break;
		default:
			printf("invalid video codec(%d)\n", dec_type);
			return FALSE;
	}

	// check .dat file
	sprintf(filename, "video_bs_%d_%d_%s.dat", VDO_SIZE_W, VDO_SIZE_H, codec_name);
	sprintf(filepath, "/mnt/sd/%s", filename);

	if ((f_in = fopen(filepath, "rb")) == NULL) {
		printf("fail to open %s\n", filepath);
		printf("%s is in SDK/code/hdal/samples/pattern/%s\n", filename, filename);
		return FALSE;
	}
	fclose(f_in);

	// check .len file
	sprintf(filename, "video_bs_%d_%d_%s.len", VDO_SIZE_W, VDO_SIZE_H, codec_name);
	sprintf(filepath, "/mnt/sd/%s", filename);

	if ((f_in = fopen(filepath, "rb")) == NULL) {
		printf("fail to open %s\n", filepath);
		printf("%s is in SDK/code/hdal/samples/pattern/%s\n", filename, filename);
		return FALSE;
	}
	fclose(f_in);

	return TRUE;
}

HD_RESULT get_video_frame_buf(HD_VIDEO_FRAME *p_video_frame)
{
	HD_COMMON_MEM_VB_BLK blk;
	HD_COMMON_MEM_DDR_ID ddr_id = 0;
	UINT32 blk_size = 0, pa = 0;
	UINT32 width = 0, height = 0;
	HD_VIDEO_PXLFMT pxlfmt = 0;

	if (p_video_frame == NULL) {
		printf("config_vdo_frm: p_video_frame is null\n");
		return HD_ERR_SYS;
	}

	// config yuv info
	ddr_id = VDO_DDR_ID;
	width = VDO_SIZE_W;
	height = VDO_SIZE_H;
	pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_64(width), ALIGN_CEIL_64(height), pxlfmt);  // align to 64 for h265 raw buffer

	// get memory
	blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id); // Get block from mem pool
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("config_vdo_frm: get blk fail, blk(0x%x)\n", blk);
		return HD_ERR_NOMEM;
	}
	pa = hd_common_mem_blk2pa(blk); // get physical addr
	if (pa == 0) {
		printf("config_vdo_frm: blk2pa fail, blk(0x%x)\n", blk);
		return HD_ERR_SYS;
	}

	p_video_frame->sign = MAKEFOURCC('V', 'F', 'R', 'M');
	p_video_frame->ddr_id = ddr_id;
	p_video_frame->pxlfmt = pxlfmt;
	p_video_frame->dim.w = width;
	p_video_frame->dim.h = height;
	p_video_frame->phy_addr[0] = pa;
	p_video_frame->blk = blk;

	return HD_OK;
}

static void *feed_bs_thread(void *arg)
{
	VIDEO_DECODE *p_stream0 = (VIDEO_DECODE *)arg;
	char codec_name[8], file_name[128];
	FILE *bs_fd = 0, *bslen_fd = 0;
	HD_RESULT ret = HD_OK;
	HD_COMMON_MEM_VB_BLK blk;
	HD_COMMON_MEM_DDR_ID ddr_id = VDO_DDR_ID;
	UINT32 blk_size = BS_BLK_SIZE;
	UINT32 pa = 0, va = 0, bs_buf_start = 0, bs_buf_curr = 0, bs_buf_end = 0;
	INT bs_size = 0, read_len = 0;
	BOOL is_desc = FALSE;
	HD_VIDEO_FRAME video_frame = {0};

	// wait flow_start
	while (p_stream0->flow_start == 0) sleep(1);

	// assign video codec
	switch (p_stream0->dec_type) {
		case HD_CODEC_TYPE_JPEG:
			snprintf(codec_name, sizeof(codec_name), "jpeg");
			break;
		case HD_CODEC_TYPE_H264:
			snprintf(codec_name, sizeof(codec_name), "h264");
			break;
		case HD_CODEC_TYPE_H265:
			snprintf(codec_name, sizeof(codec_name), "h265");
			break;
		default:
			printf("invalid video codec(%d)\n", p_stream0->dec_type);
			break;
	}

	// open input files
#if SVC_BITSTREAM_TESTING
	sprintf(file_name, "/mnt/sd/video_bs_%d_%d_%s_svc.dat", VDO_SIZE_W, VDO_SIZE_H, codec_name);
#else
	sprintf(file_name, "/mnt/sd/video_bs_%d_%d_%s.dat", VDO_SIZE_W, VDO_SIZE_H, codec_name);
#endif
	if ((bs_fd = fopen(file_name, "rb")) == NULL) {
		printf("open file (%s) fail !!....\r\nPlease copy test pattern to SD Card !!\r\n\r\n", file_name);
		return 0;
	}
	printf("bs file: [%s]\n", file_name);

#if SVC_BITSTREAM_TESTING
	snprintf(file_name, sizeof(file_name), "/mnt/sd/video_bs_%d_%d_%s_svc.len", VDO_SIZE_W, VDO_SIZE_H, codec_name);
#else
	snprintf(file_name, sizeof(file_name), "/mnt/sd/video_bs_%d_%d_%s.len", VDO_SIZE_W, VDO_SIZE_H, codec_name);
#endif
	if ((bslen_fd = fopen(file_name, "rb")) == NULL) {
		printf("open file (%s) fail !!....\r\nPlease copy test pattern to SD Card !!\r\n\r\n", file_name);
		goto quit_rel_fd;
	}
	printf("bslen file: [%s]\n", file_name);

	// get memory
	blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_POOL_BEGIN, blk_size, ddr_id); // Get block from mem pool
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get block fail, blk = 0x%x\n", blk);
		goto quit;
	}
	pa = hd_common_mem_blk2pa(blk); // get physical addr
	if (pa == 0) {
		printf("blk2pa fail, blk(0x%x)\n", blk);
		goto rel_blk;
	}
	if (pa > 0) {
		va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, blk_size); // Get virtual addr
		if (va == 0) {
			printf("get va fail, va(0x%x)\n", blk);
			goto rel_blk;
		}
		// allocate bs buf
		bs_buf_start = va;
		bs_buf_curr = bs_buf_start;
		bs_buf_end = bs_buf_start + blk_size;
	}

	// get video frame
	ret = get_video_frame_buf(&video_frame);
	if (ret != HD_OK) {
		printf("get video frame error(%d) !!\r\n", ret);
		goto quit;
	}

	// feed bs
	while (p_stream0->dec_exit == 0) {

		// get bs size
		if (fscanf(bslen_fd, "%d\n", &bs_size) == EOF) {
			// reach EOF, read from the beginning
			fseek(bs_fd, 0, SEEK_SET);
			fseek(bslen_fd, 0, SEEK_SET);
			if (fscanf(bslen_fd, "%d\n", &bs_size) == EOF) {
				printf("[ERROR] fscanf error\n");
				continue;
			}
		}
		if (bs_size == 0) {
			printf("Invalid bs_size(%d)\n", bs_size);
			continue;
		}

		// check bs buf rollback
		if ((bs_buf_curr + bs_size) > bs_buf_end) {
			bs_buf_curr = bs_buf_start;
		}

		// read bs from file
		read_len = fread((void *)bs_buf_curr, 1, bs_size, bs_fd);
		if (read_len != bs_size) {
			printf("reading error\n");
			continue;
		}

		if (p_stream0->dec_type == HD_CODEC_TYPE_H264 || p_stream0->dec_type == HD_CODEC_TYPE_H265) {
			// get h.26x desc
			is_desc = FALSE;
			parse_h26x_desc(p_stream0->dec_type, bs_buf_curr, bs_size, &is_desc);
			if (is_desc) {
				HD_H26XDEC_DESC desc_info = {0};
				desc_info.addr = bs_buf_curr;
				desc_info.len = read_len;
				if (0) { // update desc [ToDo]
					ret = hd_videodec_set(p_stream0->dec_path, HD_VIDEODEC_PARAM_IN_DESC, &desc_info);
					if (ret != HD_OK) {
						printf("set DESC fail\r\n");
						goto quit;
					}
					hd_videodec_start(p_stream0->dec_path); // will apply in next I-frame, set if need (not support)
				}
			}
		}

		// push in
		if (!is_desc) { // only push I or P
			HD_VIDEODEC_BS video_bitstream = {0};

			// config video bs
			video_bitstream.sign          = MAKEFOURCC('V','S','T','M');
			video_bitstream.p_next        = NULL;
			video_bitstream.ddr_id        = ddr_id;
			video_bitstream.vcodec_format = p_stream0->dec_type;
			video_bitstream.timestamp     = hd_gettime_us();
			video_bitstream.blk           = blk;
			video_bitstream.count         = 0;
			video_bitstream.phy_addr      = pa + (bs_buf_curr - bs_buf_start);
			video_bitstream.size          = bs_size;

			ret = hd_videodec_push_in_buf(p_stream0->dec_path, &video_bitstream, &video_frame, 0); // always blocking mode
			if (ret != HD_OK) {
				printf("push_in error(%d) !!\r\n", ret);
				continue;
			}
		}

		bs_buf_curr += ALIGN_CEIL_64(bs_size); // shift to next

		usleep(500000); // sleep 500 ms, to avoid pull_out write card too slow
	}

quit:
	// release video frame buf
	ret = hd_videodec_release_out_buf(p_stream0->dec_path, &video_frame);
	if (ret != HD_OK) {
		printf("release video frame error(%d) !!\r\n\r\n", ret);
	}

	// mummap
	ret = hd_common_mem_munmap((void*)va, blk_size);
	if (ret != HD_OK) {
		printf("mnumap error(%d) !!\r\n\r\n", ret);
	}

rel_blk:
	// release blk
	ret = hd_common_mem_release_block(blk);
	if (ret != HD_OK) {
		printf("release error(%d) !!\n", ret);
	}

quit_rel_fd:
	if (bs_fd) fclose(bs_fd);
	if (bslen_fd) fclose(bslen_fd);

	return 0;
}

static void *decode_thread(void *arg)
{
	VIDEO_DECODE* p_stream0 = (VIDEO_DECODE *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEO_FRAME video_frame;
	UINT32 vir_addr_main = 0, phy_addr_main = 0;
	char file_path_main[64];
	FILE *f_out_main;
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_addr_main))

	// wait flow_start
	while (p_stream0->flow_start == 0) {
		sleep(1);
	}

	// config pattern name
	snprintf(file_path_main, sizeof(file_path_main), "/mnt/sd/dump_frm_%d_%d_yuv.dat", VDO_SIZE_W, VDO_SIZE_H);

	// open output files
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		printf("open file (%s) fail....\r\n", file_path_main);
		return 0;
	} else {
		printf("\r\ndump yuv to file (%s) ....\r\n", file_path_main);
	}

	printf("\r\nif you want to stop, enter \"03\" exit !!\r\n\r\n");

	// pull frame data
	while (p_stream0->dec_exit == 0) {
		// pull data
		ret = hd_videodec_pull_out_buf(p_stream0->dec_path, &video_frame, -1); // >1 = timeout mode
		if (ret != HD_OK) {
			if (p_stream0->dec_exit == 1) {
				break;
			}
			printf("vdodec_pull error(%d)\r\n\r\n", ret);
    		continue; // skip this time
		}

		phy_addr_main = hd_common_mem_blk2pa(video_frame.blk); // Get physical addr
		if (phy_addr_main == 0) {
			printf("hd_common_mem_blk2pa error !!\r\n\r\n");
			goto release_out;
		}
		vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_addr_main, p_stream0->blk_size);
		if (vir_addr_main == 0) {
			printf("memory map error !!\r\n\r\n");
			goto release_out;
		}

		// write y
		UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(video_frame.phy_addr[0]);
		UINT32 len = 0;
		if (p_stream0->dec_type == HD_CODEC_TYPE_H265 || p_stream0->dec_type == HD_CODEC_TYPE_JPEG) {
			len = ALIGN_CEIL_64(VDO_SIZE_W)*ALIGN_CEIL_64(VDO_SIZE_H);
		} else if (p_stream0->dec_type == HD_CODEC_TYPE_H264) {
			len = ALIGN_CEIL_64(VDO_SIZE_W)*ALIGN_CEIL_16(VDO_SIZE_H);
		}		
		if (f_out_main) fwrite(ptr, 1, len, f_out_main);

		// write uv
		ptr = (UINT8 *)PHY2VIRT_MAIN(video_frame.phy_addr[1]);
		len = len >> 1;
		if (f_out_main) fwrite(ptr, 1, len, f_out_main);

		if (f_out_main) fflush(f_out_main);

		// mummap for bs buffer
		hd_common_mem_munmap((void *)vir_addr_main, p_stream0->blk_size);

release_out:
		ret = hd_videodec_release_out_buf(p_stream0->dec_path, &video_frame);
		if (ret != HD_OK) {
			printf("release_ouf_buf fail, ret(%d)\r\n", ret);
		}
	}

	// close output file
	if (f_out_main) fclose(f_out_main);

	return 0;
}
///////////////////////////////////////////////////////////////////////////////

int vdodec_open(void)
{
	int ret;
	//UINT32 dec_type = 0;
	HD_DIM main_dim = {0};
#if 0
	// query program options
	if (argc == 2) {
		dec_type = atoi(argv[1]);
		printf("dec_type %d\r\n", dec_type);
		if(stream[0].dec_type > 2) {
			printf("error: not support dec_type %d\r\n", dec_type);
			return 0;
		}
	}
#endif
	// assign parameter by program options
	main_dim.w = VDO_SIZE_W;
	main_dim.h = VDO_SIZE_H;
//	stream[0].dec_type = HD_CODEC_TYPE_H264;
#if 0
	if (dec_type == 0) {
		stream[0].dec_type = HD_CODEC_TYPE_H265;
	} else if (dec_type == 1) {
		stream[0].dec_type = HD_CODEC_TYPE_H264;
	} else {
		stream[0].dec_type = HD_CODEC_TYPE_JPEG;
	}
#endif
	// check TEST pattern exist
	if (check_test_pattern(stream[0].dec_type) == FALSE) {
		printf("test_pattern isn't exist\r\n");
		exit(0);
	}

	// init hdal
	ret = hd_common_init(0);
    if(ret != HD_OK) {
        printf("common fail=%d\n", ret);
        return ret;
    }

	// init memory
	ret = mem_init(&stream[0].blk_size);
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		return ret;
	}

	// init all modules
	ret = init_module();
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
        return ret;
    }

	// open all modules
	ret = open_module(&stream[0]);
    if(ret != HD_OK) {
        printf("open fail=%d\n", ret);
       return ret;
    }

	// set videodec config
	stream[0].dec_max_dim.w = main_dim.w;
	stream[0].dec_max_dim.h = main_dim.h;
	ret = set_dec_cfg(stream[0].dec_path, &stream[0].dec_max_dim, stream[0].dec_type);
	if (ret != HD_OK) {
		printf("set dec-cfg fail=%d\n", ret);
		return ret;
	}

	// set videodec paramter
	ret = set_dec_param(stream[0].dec_path, stream[0].dec_type);
	if (ret != HD_OK) {
		printf("set dec fail=%d\n", ret);
		return ret;
	}

	return ret;
}
int vdodec_close(void)
{
	int ret;

	// close video_record modules
	ret = close_module(&stream[0]);
	if(ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

	// uninit all modules
	ret = exit_module();
	if(ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}

	// uninit memory
    ret = mem_exit();
    if(ret != HD_OK) {
        printf("mem fail=%d\n", ret);
    }

	// uninit hdal
	ret = hd_common_uninit();
    if(ret != HD_OK) {
        printf("common-uninit fail=%d\n", ret);
    }

	return ret;
}
int vdodec_start(void)
{
	int ret;

	// create decode_thread (pull_out frame)
	ret = pthread_create(&stream[0].dec_thread_id, NULL, decode_thread, (void *)stream);
	if (ret < 0) {
		printf("create decode_thread failed\n");
		return ret;
	}
	
	// create feed_bs_thread (push_in bitstream)
	ret = pthread_create(&stream[0].feed_thread_id, NULL, feed_bs_thread, (void *)stream);
	if (ret < 0) {
		printf("create feed_bs_thread failed\n");
		return ret;
	}

	//int ret;
	// start video_decode_only modules
	hd_videodec_start(stream[0].dec_path);

	// let thread start to work
	stream[0].flow_start = 1;
	stream[0].dec_exit = 0;

	return 0;
}
int vdodec_stop(void)
{
	stream[0].flow_start = 0;
	stream[0].dec_exit = 1;

	// stop all modules
	hd_videodec_stop(stream[0].dec_path);

	// destroy decode thread
	pthread_join(stream[0].dec_thread_id, NULL);

	// destroy feed bs thread
	pthread_join(stream[0].feed_thread_id, NULL);

	return 0;
}

int vdodec_flow_open(void)
{
	vdodec_open();
	return 0;
}
int vdodec_flow_start(void)
{
	vdodec_start();
	return 0;
}
int vdodec_flow_stop(void)
{
	vdodec_stop();
	bRandom_fourcc = 0;
	bRandom_pa = 0;
	bZero_blk = 0;
	bRandom_bssize = 0;
	bErr_dectype = 0;

	return 0;
}
int vdodec_flow_close(void)
{
	vdodec_close();
	return 0;
}

static HD_DBG_MENU vdodec_basic_flow_menu[] = {
	{0x01, "open",     vdodec_flow_open,         TRUE},
	{0x02, "start",    vdodec_flow_start,        TRUE},
	{0x03, "stop",     vdodec_flow_stop,         TRUE},
	{0x04, "close",    vdodec_flow_close,        TRUE},
	{0x05, "set random fourcc", 		   vdodec_random_fourcc,			 TRUE},
	{0x06, "set random phy_addr",			 vdodec_random_pa,			   TRUE},
	{0x07, "set zero blk",			  vdodec_zero_blk,			   TRUE},
	{0x08, "set random bssize",			 vdodec_random_bssize,			 TRUE},
	{0x09, "set error decode type", 				  vdodec_err_dectype,			  TRUE},
	// escape muse be last
	{-1,   "",         NULL,               FALSE},
};

int vdodec_basic_flow(void)
{
	return hd_debug_menu_entry_p(vdodec_basic_flow_menu, "VDODEC BASIC FLOW");
}

int vdodec_random_fourcc(void)
{
	bRandom_fourcc = 1;
	return 0;
}

int vdodec_random_pa(void)
{
	bRandom_pa = 1;
	return 0;
}

int vdodec_zero_blk(void)
{
	bZero_blk = 1;
	return 0;
}

int vdodec_random_bssize(void)
{
	bRandom_bssize = 1;
	return 0;
}

int vdodec_err_dectype(void)
{
	bErr_dectype = 1;
	return 0;
}

//int main(int argc, char** argv)
MAIN(argc, argv)
{
	INT key;
	UINT32 dec_type = 0;

	// query program options
	if (argc == 2) {
		dec_type = atoi(argv[1]);
		printf("dec_type %d\r\n", dec_type);
		if(stream[0].dec_type > 2) {
			printf("error: not support dec_type %d\r\n", dec_type);
			return 0;
		}
	}

	if (dec_type == 0) {
		stream[0].dec_type = HD_CODEC_TYPE_H265;
	} else if (dec_type == 1) {
		stream[0].dec_type = HD_CODEC_TYPE_H264;
	} else if (dec_type == 2) {
		stream[0].dec_type = HD_CODEC_TYPE_JPEG;
	} else {
		printf("error codec type\r\n");
		return 0;
	}


	hd_debug_menu_entry_p(vdodec_test_main_menu, "VDODEC TEST MAIN");

	// query user key
	printf("Enter q to exit\n");
	while (1) {
		key = GETCHAR();
		if (key == 'q' || key == 0x3) {
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

	return 0;
}

