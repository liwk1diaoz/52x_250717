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

#define VDO_SIZE_W         1920
#define VDO_SIZE_H     	   1080
#define VDO_MAX_SIZE_W     5120
#define VDO_MAX_SIZE_H     2880

INT32 proc_query_mem(VIDEO_PROCESS *p_stream, HD_COMMON_MEM_INIT_CONFIG* p_mem_cfg, INT32 i)
{
	// config common pool (in)
	p_mem_cfg->pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	p_mem_cfg->pool_info[i].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(VDO_MAX_SIZE_W, VDO_MAX_SIZE_H, HD_VIDEO_PXLFMT_RAW12);
	p_mem_cfg->pool_info[i].blk_cnt = 2;
	p_mem_cfg->pool_info[i].ddr_id = DDR_ID0;
	i++;
	// config common pool (out)
	p_mem_cfg->pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	p_mem_cfg->pool_info[i].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_MAX_SIZE_W, VDO_MAX_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	p_mem_cfg->pool_info[i].blk_cnt = 2;
	p_mem_cfg->pool_info[i].ddr_id = DDR_ID0;
	i++;
	// config common pool (ext)
	p_mem_cfg->pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	p_mem_cfg->pool_info[i].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_MAX_SIZE_W, VDO_MAX_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	p_mem_cfg->pool_info[i].blk_cnt = 2;
	p_mem_cfg->pool_info[i].ddr_id = DDR_ID0;
	i++;
	return i;
}

///////////////////////////////////////////////////////////////////////////////

HD_RESULT proc_init(void)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
	return HD_OK;
}

HD_RESULT proc_set_config(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_DEV_CONFIG* p_cfg)
{
    HD_RESULT ret = HD_OK;
	//p_stream->dev_id = 0;
	//p_stream->out_id = 0;
	//p_stream->in_id = 0;

	// set videoproc config
	{
		HD_VIDEOPROC_CTRL video_ctrl_param = {0};
		HD_PATH_ID video_proc_ctrl = 0;

		ret = hd_videoproc_open(
			0, 
			HD_VIDEOPROC_CTRL(p_stream->dev_id), 
			&video_proc_ctrl); //open this for device control
		if (ret != HD_OK)
			return ret;

		p_cfg->pipe = p_stream->pipe;
		p_cfg->ctrl_max.func = p_stream->func;
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, p_cfg);
		if (ret != HD_OK) {
			return HD_ERR_NG;
		}

		video_ctrl_param.func = p_stream->func;
		video_ctrl_param.ref_path_3dnr= p_stream->ref_path_3dnr;
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);
		if (ret != HD_OK)
			return ret;

		p_stream->proc_ctrl = video_proc_ctrl;
	}
	
	return ret;
}

HD_RESULT proc_open(VIDEO_PROCESS *p_stream)
{
    HD_RESULT ret = HD_OK;

	if (p_stream->out_id >= 5) {
		// extend path, require open path 0 (source path)
		if ((ret = hd_videoproc_open(
			HD_VIDEOPROC_IN(p_stream->dev_id, p_stream->in_id), 
			HD_VIDEOPROC_OUT(p_stream->dev_id, 0), 
			&p_stream->proc_src_path)) != HD_OK)
			return ret;
	}
	if ((ret = hd_videoproc_open(
		HD_VIDEOPROC_IN(p_stream->dev_id, p_stream->in_id), 
		HD_VIDEOPROC_OUT(p_stream->dev_id, p_stream->out_id), 
		&p_stream->proc_path)) != HD_OK)
		return ret;

	return HD_OK;
}

HD_RESULT proc_set_in(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_IN* p_in)
{
	HD_RESULT ret = HD_OK;

	{ //if videoproc is already binding to dest module, not require to setting this!
		ret = hd_videoproc_set(p_stream->proc_path, HD_VIDEOPROC_PARAM_IN, p_in);
	}

	return ret;
}

HD_RESULT proc_set_out(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_OUT* p_out)
{
	HD_RESULT ret = HD_OK;

	if (p_stream->out_id >= 5) {
		HD_VIDEOPROC_OUT c = p_out[0];
		c.dim.w = VDO_SIZE_W;
		c.dim.h = VDO_SIZE_H;
		// extend path, require open path 0 (source path)
		ret = hd_videoproc_set(p_stream->proc_src_path, HD_VIDEOPROC_PARAM_OUT, &c);
		if (ret != HD_OK)
			return ret;
	}
	if (p_stream->out_id >= 5) {
		HD_VIDEOPROC_OUT_EX c;
		c.src_path = p_stream->proc_src_path;
		c.dim.w = p_out->dim.w;
		c.dim.h = p_out->dim.h;
		c.pxlfmt = p_out->pxlfmt;
		c.dir = p_out->dir;
		c.frc = p_out->frc;
		c.depth = p_out->depth;
		ret = hd_videoproc_set(p_stream->proc_path, HD_VIDEOPROC_PARAM_OUT_EX, &c);
	} else 	{
		ret = hd_videoproc_set(p_stream->proc_path, HD_VIDEOPROC_PARAM_OUT, p_out);
	}
	/*
	{
		HD_VIDEOPROC_OUT_EX video_out_param = {0};
		video_out_param.src_path = src_path;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.dir = dir;
		video_out_param.depth = 1; //set 1 to allow pull
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_EX, &video_out_param);
	}
	*/
	return ret;
}

HD_RESULT proc_set_in_crop(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_CROP* p_crop)
{
	HD_RESULT ret = HD_OK;
	ret = hd_videoproc_set(p_stream->proc_path, HD_VIDEOPROC_PARAM_IN_CROP, p_crop);
	return ret;
}

HD_RESULT proc_set_out_crop(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_CROP* p_crop)
{
	HD_RESULT ret = HD_OK;
	if (p_stream->out_id >= 5) {
		ret = hd_videoproc_set(p_stream->proc_path, HD_VIDEOPROC_PARAM_OUT_EX_CROP, p_crop);
	} else {
		ret = hd_videoproc_set(p_stream->proc_path, HD_VIDEOPROC_PARAM_OUT_CROP, p_crop);
	}
	return ret;
}

HD_RESULT proc_set_in_frc(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_FRC* p_frc)
{
	HD_RESULT ret = HD_OK;
	hd_videoproc_set(p_stream->proc_path, HD_VIDEOPROC_PARAM_IN_FRC, p_frc);
	return ret;
}

HD_RESULT proc_set_out_frc(VIDEO_PROCESS *p_stream, HD_VIDEOPROC_FRC* p_frc)
{
	HD_RESULT ret = HD_OK;
	hd_videoproc_set(p_stream->proc_path, HD_VIDEOPROC_PARAM_OUT_FRC, p_frc);
	return ret;
}

HD_RESULT proc_start(VIDEO_PROCESS *p_stream)
{
	HD_RESULT ret;
	if (p_stream->out_id >= 5) {
		// extend path, require start path 0 (source path)
		ret = hd_videoproc_start(p_stream->proc_src_path);
	}
	if ((ret = hd_videoproc_start(p_stream->proc_path)) != HD_OK)
		return ret;
	return HD_OK;
}

HD_RESULT proc_stop(VIDEO_PROCESS *p_stream)
{
	HD_RESULT ret;
	if (p_stream->out_id >= 5) {
		// extend path, require stop path 0 (source path)
		ret = hd_videoproc_stop(p_stream->proc_src_path);
	}
	if ((ret = hd_videoproc_stop(p_stream->proc_path)) != HD_OK)
		return ret;
	return HD_OK;
}

HD_RESULT proc_close(VIDEO_PROCESS *p_stream)
{
	HD_RESULT ret;
	if (p_stream->out_id >= 5) {
		// extend path, require close path 0 (source path)
		ret = hd_videoproc_close(p_stream->proc_src_path);
	}
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	return HD_OK;
}

HD_RESULT proc_uninit(void)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}

