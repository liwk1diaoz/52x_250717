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


INT32 pull_query_mem(VIDEO_PROCESS *p_stream, HD_COMMON_MEM_INIT_CONFIG* p_mem_cfg, INT32 i)
{
	return i;
}


///////////////////////////////////////////////////////////////////////////////

static void *aquire_yuv_thread(void *arg);

HD_RESULT pull_init(void)
{
	HD_RESULT ret = HD_OK;
	return ret;
}

HD_RESULT pull_open(VIDEO_PROCESS *p_stream)
{
	HD_RESULT ret;
	p_stream->pull_start = 0;
	p_stream->pull_exit = 0;
	// create aquire_thread (pull_out frame)
	ret = pthread_create(&(p_stream->aquire_thread_id), NULL, aquire_yuv_thread, (void *)p_stream);
	return ret;
}

HD_RESULT pull_set_in(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_OUT* p_in)
{
	HD_RESULT ret = HD_OK;
	p_stream->pull_dim.w = p_in->dim.w;
	p_stream->pull_dim.h = p_in->dim.h;
	p_stream->pull_pxlfmt = p_in->pxlfmt;
	printf("pull_set_in() to %dx%d, %08x\r\n", p_stream->pull_dim.w, p_stream->pull_dim.h, p_stream->pull_pxlfmt);
	return ret;
}

HD_RESULT pull_start(VIDEO_PROCESS *p_stream)
{
	HD_RESULT ret = HD_OK;
	p_stream->pull_start = 1;
	return ret;
}

HD_RESULT pull_stop(VIDEO_PROCESS *p_stream)
{
	HD_RESULT ret = HD_OK;
	// let feed_thread, aquire_thread stop loop and exit
	p_stream->pull_exit = 1;
	return ret;
}

HD_RESULT pull_close(VIDEO_PROCESS *p_stream)
{
	HD_RESULT ret;
	// destroy aquire_thread
	ret = pthread_join(p_stream->aquire_thread_id, (void* )NULL);
	return ret;
}

HD_RESULT pull_uninit(void)
{
	HD_RESULT ret = HD_OK;
	return ret;
}

HD_RESULT pull_snapshot(VIDEO_PROCESS *p_stream, const char* name)
{
	printf("\r\ntake snapshot for %s!\r\n", name);
	strncpy(p_stream->snap_file, name, 128-1); p_stream->snap_file[127] = 0; p_stream->do_snap = 1;
	return HD_OK;
}

static void *aquire_yuv_thread(void *arg)
{
	VIDEO_PROCESS* p_stream0 = (VIDEO_PROCESS *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEO_FRAME video_frame = {0};
	UINT32 phy_addr_main, vir_addr_main;
	UINT32 blk_size = 0;
	UINT32 yuv_size = 0;
	char file_path_main[128] = {0};
	FILE *f_out_main;
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + ((pa) - phy_addr_main))

	//------ [1] wait flow_start ------
	while (p_stream0->pull_start == 0) sleep(1);

	//printf("\r\nif you want to snapshot, enter \"s\" to trigger !!\r\n");
	//printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	//--------- pull data test ---------
	while (p_stream0->pull_exit == 0) {

		if(1) {
			ret = hd_videoproc_pull_out_buf(p_stream0->proc_path, &video_frame, p_stream0->wait_ms); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
			if (ret != HD_OK) {
				if(p_stream0->show_ret) printf("pull_out(%d) error = %d!!\r\n", p_stream0->wait_ms, ret);
        			goto skip;
			}

			if(p_stream0->show_ret) printf("pull_out(%d) ok!!\r\n", p_stream0->wait_ms);
			if(!p_stream0->do_snap) {
				ret = hd_videoproc_release_out_buf(p_stream0->proc_path, &video_frame);
				if (ret != HD_OK) {
					printf("release_out() error = %d!!\r\n", ret);
				}
				goto skip;
			}
			p_stream0->do_snap = 0;
			phy_addr_main = hd_common_mem_blk2pa(video_frame.blk); // Get physical addr
			if (phy_addr_main == 0) {
				printf("blk2pa fail, blk = 0x%x\r\n", video_frame.blk);
        			goto skip;
			}

	switch (HD_VIDEO_PXLFMT_CLASS(video_frame.pxlfmt)) {
	case HD_VIDEO_PXLFMT_CLASS_YUV:
		// ALIGN_CEIL_16 for rotate 90/270
		yuv_size = VDO_YUV_BUFSIZE(ALIGN_CEIL_16(video_frame.dim.w), ALIGN_CEIL_16(video_frame.dim.h), video_frame.pxlfmt);
		break;
	case HD_VIDEO_PXLFMT_CLASS_NVX:
		yuv_size = VDO_NVX_BUFSIZE(video_frame.dim.w, video_frame.dim.h, video_frame.pxlfmt);
		break;
	case HD_VIDEO_PXLFMT_CLASS_RAW:
		yuv_size = VDO_RAW_BUFSIZE(video_frame.dim.w, video_frame.dim.h, video_frame.pxlfmt);
		break;
	case HD_VIDEO_PXLFMT_CLASS_NRX:
		yuv_size = VDO_NRX_BUFSIZE(video_frame.dim.w, video_frame.dim.h);
		break;
	default:
		break;
	}

	if (yuv_size == 0) {
		printf("not support pxlfmt %08x!\r\n", video_frame.pxlfmt);
		return 0;
	}

	blk_size = yuv_size + DBGINFO_BUFSIZE();

			// mmap for frame buffer (just mmap one time only, calculate offset to virtual address later)
			vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_addr_main, blk_size);
			if (vir_addr_main == 0) {
				printf("mmap error !!\r\n\r\n");
        			goto skip;
			}

			//p_stream0->shot_count
			snprintf(file_path_main, 128, "/mnt/sd/dump_frm_%lu_%lu_%08x_%s.dat",
				video_frame.dim.w, video_frame.dim.h, (unsigned int)video_frame.pxlfmt, p_stream0->snap_file);
			printf("dump snapshot frame to file (%s) ....\r\n", file_path_main);

			//--- data will be readed by CPU, flush CPU cache to PHY memory ---
			hd_common_mem_flush_cache((void *)vir_addr_main, yuv_size);

			//----- open output files -----
			if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
				printf("open file (%s) fail....\r\n\r\n", file_path_main);
        			goto skip;
			}

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
			if (video_frame.pxlfmt == HD_VIDEO_PXLFMT_YUV420_NVX2) {
#else  // _BSP_NA51000_
			if ((video_frame.pxlfmt == HD_VIDEO_PXLFMT_YUV420_NVX1_H264)
			|| (video_frame.pxlfmt == HD_VIDEO_PXLFMT_YUV420_NVX1_H265)) {
#endif
				printf("save nvx format...\r\n");
				//save Y plane - NVX1 compressed
				{
					UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(video_frame.phy_addr[0]);
					UINT32 len = (video_frame.loff[0]*4)*(video_frame.ph[0]/4);
					if (f_out_main) fwrite(ptr, 1, len, f_out_main);
					if (f_out_main) fflush(f_out_main);
				}
				//save UV plane - NVX1 compressed
				{
					UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(video_frame.phy_addr[1]);
					UINT32 len = (video_frame.loff[1]*4)*(video_frame.ph[1]/4);
					if (f_out_main) fwrite(ptr, 1, len, f_out_main);
					if (f_out_main) fflush(f_out_main);
				}
			} else {
				printf("save yuv format...\r\n");
				//save Y plane
				{
					UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(video_frame.phy_addr[0]);
					UINT32 len = video_frame.loff[0]*video_frame.ph[0];
					if (f_out_main) fwrite(ptr, 1, len, f_out_main);
					if (f_out_main) fflush(f_out_main);
				}
				//save UV plane
				{
					UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(video_frame.phy_addr[1]);
					UINT32 len = video_frame.loff[1]*video_frame.ph[1];
					if (f_out_main) fwrite(ptr, 1, len, f_out_main);
					if (f_out_main) fflush(f_out_main);
				}
			}

			// mummap for frame buffer
			ret = hd_common_mem_munmap((void *)vir_addr_main, yuv_size);
			if (ret != HD_OK) {
				printf("mnumap error !!\r\n\r\n");
        			goto skip;
			}

			if(p_stream0->show_ret) printf("release_out() ....\r\n");
			ret = hd_videoproc_release_out_buf(p_stream0->proc_path, &video_frame);
			if (ret != HD_OK) {
				printf("release_out() error !!\r\n\r\n");
        			goto skip;
			}

			// close output file
			fclose(f_out_main);

			printf("dump snapshot ok\r\n\r\n");

			p_stream0->shot_count ++;
		}
skip:
		usleep(1000); //delay 1 ms
	}

	return 0;
}
