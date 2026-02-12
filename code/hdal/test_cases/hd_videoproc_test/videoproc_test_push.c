/**
	@brief Sample code of video snapshot from proc to frame.\n

	@file video_process_only.c

	@author Jeah Yen

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
#include "hd_util.h"
#include "videoproc_test_int.h"

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

#define VDO_FRM_NUM        1     // pattern file frame number
#define MAX_YUV_BLK_CNT    4


INT32 push_query_mem(VIDEO_PROCESS *p_stream, HD_COMMON_MEM_INIT_CONFIG* p_mem_cfg, INT32 i)
{
	return i;
}

///////////////////////////////////////////////////////////////////////////////

static void *feed_yuv_thread(void *arg);

HD_RESULT push_init(void)
{
	HD_RESULT ret = HD_OK;
	return ret;
}

HD_RESULT push_open(VIDEO_PROCESS *p_stream)
{
	HD_RESULT ret = HD_OK;
	p_stream->push_start = 0;
	p_stream->push_exit = 0;
	// create feed_thread (push_in frame)
	ret = pthread_create(&(p_stream->feed_thread_id), NULL, feed_yuv_thread, (void *)p_stream);
	return ret;
}

HD_RESULT push_set_out(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_IN* p_out)
{
	HD_RESULT ret = HD_OK;
	p_stream->push_dim.w = p_out->dim.w;
	p_stream->push_dim.h = p_out->dim.h;
	p_stream->push_pxlfmt = p_out->pxlfmt;
	printf("push_set_out() to %dx%d, %08x\r\n", p_stream->push_dim.w, p_stream->push_dim.h, p_stream->push_pxlfmt);
	return ret;
}

HD_RESULT push_start(VIDEO_PROCESS *p_stream)
{
	HD_RESULT ret = HD_OK;
	p_stream->push_start = 1;
	return ret;
}

HD_RESULT push_stop(VIDEO_PROCESS *p_stream)
{
	HD_RESULT ret = HD_OK;
	p_stream->push_exit = 1; 
	return ret;
}

HD_RESULT push_close(VIDEO_PROCESS *p_stream)
{
	HD_RESULT ret;
	// destroy feed_thread thread
	ret = pthread_join(p_stream->feed_thread_id,  (void* )NULL);
	return ret;
}

HD_RESULT push_uninit(void)
{
	HD_RESULT ret = HD_OK;
	return ret;
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
	VIDEO_PROCESS* p_stream0 = (VIDEO_PROCESS *)arg;
	HD_RESULT ret = HD_OK;
	int i;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32 blk_size = 0;
	UINT32 yuv_size = 0;
	char filepath_yuv_main[128];
	FILE *f_in_main;
	UINT32 pa_yuv_main[MAX_YUV_BLK_CNT] = {0};
	UINT32 va_yuv_main[MAX_YUV_BLK_CNT] = {0};
	INT32  blkidx;
	HD_COMMON_MEM_VB_BLK blk;
	UINT32 read_len;
	UINT32 filefmt= 0;
	UINT32 raw_seq_cnt = 1;

	//------ [1] wait flow_start ------
	while (p_stream0->push_start == 0) sleep(1);

	switch (HD_VIDEO_PXLFMT_CLASS(p_stream0->push_pxlfmt)) {
	case HD_VIDEO_PXLFMT_CLASS_YUV:
		yuv_size = VDO_YUV_BUFSIZE(p_stream0->push_dim.w, p_stream0->push_dim.h, p_stream0->push_pxlfmt);
		break;
	case HD_VIDEO_PXLFMT_CLASS_NVX:
		yuv_size = VDO_NVX_BUFSIZE(p_stream0->push_dim.w, p_stream0->push_dim.h, p_stream0->push_pxlfmt);
		break;
	case HD_VIDEO_PXLFMT_CLASS_RAW:
		yuv_size = VDO_RAW_BUFSIZE(p_stream0->push_dim.w, p_stream0->push_dim.h, p_stream0->push_pxlfmt);
		break;
	case HD_VIDEO_PXLFMT_CLASS_NRX:
		yuv_size = VDO_NRX_BUFSIZE(p_stream0->push_dim.w, p_stream0->push_dim.h);
		break;
	default:
		break;
	}

	if (yuv_size == 0) {
		printf("not support pxlfmt %08x!\r\n", p_stream0->push_pxlfmt);
		return 0;
	}

	blk_size = yuv_size + DBGINFO_BUFSIZE();
	//------ [2] open input files ------
	filefmt = p_stream0->push_pxlfmt;
	switch (HD_VIDEO_PXLFMT_CLASS(filefmt)) {
	case HD_VIDEO_PXLFMT_CLASS_RAW:
		raw_seq_cnt = HD_VIDEO_PXLFMT_PLANE(filefmt);
		if (raw_seq_cnt > 1)
			filefmt = (filefmt & ~HD_VIDEO_PXLFMT_PLANE_MASK) | (1 << 24); //modify plane to 1
		break;
	default:
		break;
	}
	sprintf(filepath_yuv_main, "/mnt/sd/video_frm_%d_%d_%08x_%d.dat", (int)p_stream0->push_dim.w, (int)p_stream0->push_dim.h, (unsigned int)filefmt, VDO_FRM_NUM);
	printf("load file (%s)....\r\n", filepath_yuv_main);

	if ((f_in_main = fopen(filepath_yuv_main, "rb")) == NULL) {
		printf("open file (%s) fail !!\r\nPlease copy test pattern to SD Card !!\r\n\r\n", filepath_yuv_main);
		return 0;
	}

		//--- Get memory ---
		blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id); // Get block from mem pool
		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get block fail (0x%x).. try again later.....\r\n", blk);
			sleep(1);
			goto quit;
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
		read_len = fread((void *)va_yuv_main[blkidx], 1, yuv_size, f_in_main);
		if (read_len != yuv_size) {
			if (feof(f_in_main)) {
				fseek(f_in_main, 0, SEEK_SET);  //rewind and try again
				read_len = fread((void *)va_yuv_main[blkidx], 1, yuv_size, f_in_main);
				if (read_len != yuv_size) {
					printf("reading len error\n");
					goto quit;
				}
			}
		}
		
		//--- data is written by CPU, flush CPU cache to PHY memory ---
		hd_common_mem_flush_cache((void *)va_yuv_main[blkidx], yuv_size);
		

	//------ [3] feed yuv ------
	while (p_stream0->push_exit == 0) {

		//--- push_in ---
		switch (HD_VIDEO_PXLFMT_CLASS(p_stream0->push_pxlfmt)) {
		case HD_VIDEO_PXLFMT_CLASS_YUV:
			switch (p_stream0->push_pxlfmt) {
			case HD_VIDEO_PXLFMT_YUV444_PLANAR:
			case HD_VIDEO_PXLFMT_YUV422_PLANAR:
			case HD_VIDEO_PXLFMT_YUV420_PLANAR:
				break;
			case HD_VIDEO_PXLFMT_YUV422:
			case HD_VIDEO_PXLFMT_YUV420:
			case HD_VIDEO_PXLFMT_YUV400:
				{
				HD_VIDEO_FRAME video_frame = {0};
				video_frame.sign        = MAKEFOURCC('V','F','R','M');
				video_frame.p_next      = NULL;
				video_frame.ddr_id      = ddr_id;
				video_frame.pxlfmt      = p_stream0->push_pxlfmt;
				video_frame.dim.w       = p_stream0->push_dim.w;
				video_frame.dim.h       = p_stream0->push_dim.h;
				video_frame.count       = 0;
				video_frame.timestamp   = hd_gettime_us();
				video_frame.pw[0]       = video_frame.dim.w; // Y
				video_frame.ph[0]       = video_frame.dim.h; // UV
				video_frame.pw[1]       = video_frame.dim.w >> 1; // Y
				video_frame.ph[1]       = video_frame.dim.h >> 1; // UV
				video_frame.loff[0]     = ALIGN_CEIL_4(video_frame.pw[0]); // Y
				video_frame.loff[1]     = video_frame.loff[0]; // UV
				video_frame.phy_addr[0] = pa_yuv_main[blkidx];                          // Y
				video_frame.phy_addr[1] = pa_yuv_main[blkidx] + video_frame.loff[0]*video_frame.ph[0];  // UV
				video_frame.blk         = blk;

				if(p_stream0->show_ret) printf("push_in(%d)!\r\n", 0);
				ret = hd_videoproc_push_in_buf(p_stream0->proc_path, &video_frame, NULL, 0); // only support non-blocking mode now
				if (ret != HD_OK) {
					printf("push_in error !!(%d)\r\n",ret);
				}
				}
				break;
			default:
				break;
			}
			break;
		case HD_VIDEO_PXLFMT_CLASS_NVX:
			break;
		case HD_VIDEO_PXLFMT_CLASS_RAW:
			{
				HD_VIDEO_FRAME video_frame = {0};
				UINT32 cnt = 0;
				video_frame.sign        = MAKEFOURCC('V','F','R','M');
				video_frame.p_next      = NULL;
				video_frame.ddr_id      = ddr_id;
				video_frame.pxlfmt      = p_stream0->push_pxlfmt | HD_VIDEO_PIX_RGGB_R;//HD_VIDEO_PXLFMT_YUV420;
				video_frame.dim.w       = p_stream0->push_dim.w;
				video_frame.dim.h       = p_stream0->push_dim.h;
				video_frame.count       = 0;
				video_frame.timestamp   = hd_gettime_us();
				video_frame.pw[0]       = video_frame.dim.w; // RAW
				video_frame.ph[0]       = video_frame.dim.h; // RAW
				video_frame.loff[0]     = ALIGN_CEIL_4((video_frame.dim.w) * HD_VIDEO_PXLFMT_BPP(video_frame.pxlfmt) / 8); // RAW
				video_frame.phy_addr[0] = pa_yuv_main[blkidx]; // RAW
				video_frame.blk         = blk;

				for (cnt = 0 ; cnt < raw_seq_cnt; cnt++) {   //push multiple times to simulate shdr behavior
					video_frame.pxlfmt |= (cnt << 12);   // fill sequence to pxlfmt
					if(p_stream0->show_ret) printf("push_in(%d)!\r\n", 0);
					ret = hd_videoproc_push_in_buf(p_stream0->proc_path, &video_frame, NULL, 0); // only support non-blocking mode now
					if (ret != HD_OK) {
						printf("push_in error !!\r\n");
					}
				}
			}
			break;
		case HD_VIDEO_PXLFMT_CLASS_NRX:
			break;
		default:
			break;
		}
#if defined(_NVT_FPGA_)
		usleep(66666); // sleep 33.333 ms = 1000 ms / 30 fps
#else
    	usleep(33333); // sleep 33.333 ms = 1000 ms / 30 fps
#endif
	}

rel_blk:
		//--- Release memory ---
		ret = hd_common_mem_release_block(blk);
		if (HD_OK != ret) {
			printf("release blk fail, ret(%d)\n", ret);
			goto quit;
		}

quit:
	//------ [4] uninit & exit ------
	// mummap for yuv buffer
	for (i=0; i< MAX_YUV_BLK_CNT; i++) {
		if (va_yuv_main[i] != 0) {
			ret = hd_common_mem_munmap((void *)va_yuv_main[i], blk_size);
			if (ret != HD_OK) {
				printf("mnumap error !!\r\n\r\n");
			}
		}
	}

	// close file
	if (f_in_main)  fclose(f_in_main);


	return 0;
}
