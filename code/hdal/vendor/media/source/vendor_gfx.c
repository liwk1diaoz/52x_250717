/**
	@brief Source file of vendor media videoout.

	@file vendor_gfx.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/


#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#if defined(__LINUX)
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#endif
#include "kflow_common/type_vdo.h"
#include "vendor_gfx.h"
#include "gximage/hd_gximage.h"

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#define GFX_OPEN(...)  0
#define GFX_IOCTL      nvt_gfx_ioctl
#define GFX_CLOSE(...)
#endif
#if defined(__LINUX)
#define GFX_OPEN     open
#define GFX_IOCTL    ioctl
#define GFX_CLOSE    close
#endif

extern int copy_hd_img_to_gfx(HD_GFX_IMG_BUF *src, GFX_IMG_BUF *dst);
extern GFX_SCALE_METHOD convert_scale_method(HD_GFX_SCALE_QUALITY quality);
extern int config_scale_param(GFX_USER_DATA *p_command, HD_GFX_SCALE *p_param, int dma_flush);
extern HD_RESULT _hd_gfx_copy(HD_GFX_COPY *p_param, UINT32 flush, GFX_GRPH_ENGINE engine);
extern HD_RESULT _hd_gfx_draw_rect(HD_GFX_DRAW_RECT *p_param, UINT32 flush);
extern HD_RESULT _hd_gfx_draw_line(HD_GFX_DRAW_LINE *p_param, UINT32 flush);

HD_RESULT vendor_gfx_ise_scale_y8(VENDOR_GFX_ISE_SCALE_Y8 *p_param)
{
	int              fd, ret;
	GFX_USER_DATA    command;

	if(!p_param){
		printf("vendor_gfx_ise_scale_y8() : p_param is NULL\n");
		return HD_ERR_NULL_PTR;
	}

	if(!p_param->p_src_phy_addr || !p_param->p_dst_phy_addr){
		printf("vendor_gfx_ise_scale_y8() : src buf(%x) or dst buf(%x) is NULL\n",
			p_param->p_src_phy_addr, p_param->p_dst_phy_addr);
		return HD_ERR_NULL_PTR;
	}

	if(!p_param->src_dim.w || !p_param->src_dim.h){
		printf("vendor_gfx_ise_scale_y8() : invalid src w(%d) or h(%d)\n",
			p_param->src_dim.w, p_param->src_dim.h);
		return HD_ERR_INV;
	}

	if(!p_param->dst_dim.w || !p_param->dst_dim.h){
		printf("vendor_gfx_ise_scale_y8() : invalid dst w(%d) or h(%d)\n",
			p_param->dst_dim.w, p_param->dst_dim.h);
		return HD_ERR_INV;
	}

	if(!p_param->src_loff || !p_param->dst_loff){
		printf("vendor_gfx_ise_scale_y8() : invalid src loff(%d) or dst loff(%d)\n",
			p_param->src_loff, p_param->dst_loff);
		return HD_ERR_NULL_PTR;
	}

	if(p_param->src_loff < p_param->src_dim.w){
		printf("vendor_gfx_ise_scale_y8() : src loff(%d) can't be smaller than w(%d)\n",
			p_param->src_loff, p_param->src_dim.w);
		return HD_ERR_INV;
	}

	if(p_param->dst_loff < p_param->dst_dim.w){
		printf("vendor_gfx_ise_scale_y8() : dst loff(%d) can't be smaller than w(%d)\n",
			p_param->dst_loff, p_param->dst_dim.w);
		return HD_ERR_INV;
	}

	if(p_param->quality >= HD_GFX_SCALE_QUALITY_MAX){
		printf("vendor_gfx_ise_scale_y8() : invalid quality(%d)\n", p_param->quality);
		return HD_ERR_INV;
	}

	if(p_param->engine >= VENDOR_GFX_ISE_ENGINE_MAX){
		printf("vendor_gfx_ise_scale_y8() : invalid engine(%d)\n", p_param->engine);
		return HD_ERR_INV;
	}

	fd = GFX_OPEN("/proc/nvt_gfx" , O_RDWR, S_IRUSR);
	//coverity[dead_error_condition]
	if(fd < 0){
		//coverity[dead_error_begin]
		printf("vendor_gfx_ise_scale_y8() : fail to open /proc/nvt_gfx\r\n");
		return HD_ERR_DRV;
	}

	memset(&command, 0, sizeof(GFX_USER_DATA));
	command.version                       = GFX_USER_VERSION;
	command.cmd                           = GFX_USER_CMD_ISE_SCALE_Y8;
	command.data.ise_scale_y8.src_w       = p_param->src_dim.w;
	command.data.ise_scale_y8.src_h       = p_param->src_dim.h;
	command.data.ise_scale_y8.dst_w       = p_param->dst_dim.w;
	command.data.ise_scale_y8.dst_h       = p_param->dst_dim.h;
	command.data.ise_scale_y8.p_src       = p_param->p_src_phy_addr;
	command.data.ise_scale_y8.src_loff    = p_param->src_loff;
	command.data.ise_scale_y8.p_dst       = p_param->p_dst_phy_addr;
	command.data.ise_scale_y8.dst_loff    = p_param->dst_loff;

	if(p_param->quality == HD_GFX_SCALE_QUALITY_NULL)
		command.data.ise_scale_y8.method = GFX_SCALE_METHOD_NULL;
	else if(p_param->quality == HD_GFX_SCALE_QUALITY_BICUBIC)
		command.data.ise_scale_y8.method = GFX_SCALE_METHOD_BICUBIC;
	else if(p_param->quality == HD_GFX_SCALE_QUALITY_BILINEAR)
		command.data.ise_scale_y8.method = GFX_SCALE_METHOD_BILINEAR;
	else if(p_param->quality == HD_GFX_SCALE_QUALITY_NEAREST)
		command.data.ise_scale_y8.method = GFX_SCALE_METHOD_NEAREST;
	else if(p_param->quality == HD_GFX_SCALE_QUALITY_INTEGRATION)
		command.data.ise_scale_y8.method = GFX_SCALE_METHOD_INTEGRATION;
	else{
		printf("vendor_gfx_ise_scale_y8() : invalid quality(%d)\n", p_param->quality);
		ret = HD_ERR_INV;
		goto exit;
	}

	if(p_param->engine == VENDOR_GFX_ISE_ENGINE_1)
		command.data.ise_scale_y8.engine = GFX_ISE_ENGINE_1;
	else if(p_param->engine == VENDOR_GFX_ISE_ENGINE_2)
		command.data.ise_scale_y8.engine = GFX_ISE_ENGINE_2;
	else{
		printf("vendor_gfx_ise_scale_y8() : invalid engine(%d)\n", p_param->engine);
		ret = HD_ERR_INV;
		goto exit;
	}

	ret = GFX_IOCTL(fd , command.cmd, &command);

exit:
	GFX_CLOSE(fd);

	return ret;
}

HD_RESULT vendor_gfx_affine(VENDOR_GFX_AFFINE *p_param){
	int              i, fd, ret;
	GFX_USER_DATA    command;

	memset(&command, 0, sizeof(GFX_USER_DATA));

	if(!p_param){
		printf("vendor_gfx_affine() : p_param is NULL\n");
		return HD_ERR_NULL_PTR;
	}

	if(p_param->src_img.format != p_param->dst_img.format){
		printf("vendor_gfx_affine() : src format(%x) != dst format(%x).\n",
			p_param->src_img.format, p_param->dst_img.format);
		return HD_ERR_NOT_SUPPORT;
	}

	if(p_param->src_img.format != HD_VIDEO_PXLFMT_YUV420        &&
	   p_param->src_img.format != HD_VIDEO_PXLFMT_RGB888_PLANAR &&
	   p_param->src_img.format != HD_VIDEO_PXLFMT_Y8){
		printf("vendor_gfx_affine() : invalid src format(%x) or dst format(%x). only yuv420Packed/RGB888Planar/Y8 are supported\n",
			p_param->src_img.format, p_param->dst_img.format);
		return HD_ERR_NOT_SUPPORT;
	}

	if(!p_param->src_img.dim.w || p_param->src_img.dim.w != p_param->dst_img.dim.w){
		printf("vendor_gfx_affine() : invalid src w(%d) or dst w(%d)\n",
			p_param->src_img.dim.w, p_param->dst_img.dim.w);
		return HD_ERR_INV;
	}

	if(!p_param->src_img.dim.h || p_param->src_img.dim.h != p_param->dst_img.dim.h){
		printf("vendor_gfx_affine() : invalid src h(%d) or dst h(%d)\n",
			p_param->src_img.dim.h, p_param->dst_img.dim.h);
		return HD_ERR_INV;
	}

	for(i = 0 ; i < GFX_MAX_PLANE_NUM ; ++i){
		command.data.affine.p_src[i]       = p_param->src_img.p_phy_addr[i];
		command.data.affine.src_loff[i]    = p_param->src_img.lineoffset[i];
		command.data.affine.p_dst[i]       = p_param->dst_img.p_phy_addr[i];
		command.data.affine.dst_loff[i]    = p_param->dst_img.lineoffset[i];
		command.data.affine.fCoeffA[i]     = p_param->fCoeffA;
		command.data.affine.fCoeffB[i]     = p_param->fCoeffB;
		command.data.affine.fCoeffC[i]     = p_param->fCoeffC;
		command.data.affine.fCoeffD[i]     = p_param->fCoeffD;
		command.data.affine.fCoeffE[i]     = p_param->fCoeffE;
		command.data.affine.fCoeffF[i]     = p_param->fCoeffF;
	}

	if(p_param->src_img.format == HD_VIDEO_PXLFMT_YUV420){
		command.data.affine.plane_num  = 2;
		command.data.affine.uvpack[1]  = 1;
		command.data.affine.fCoeffC[1] = p_param->fCoeffC / 2;
		command.data.affine.fCoeffF[1] = p_param->fCoeffF / 2;
	}else if(p_param->src_img.format == HD_VIDEO_PXLFMT_RGB888_PLANAR){
		command.data.affine.plane_num = 3;
	}else{//Y8
		command.data.affine.plane_num = 1;
	}

	for(i = 0 ; i < command.data.affine.plane_num ; ++i){
		if(!p_param->src_img.p_phy_addr[i]){
			printf("vendor_gfx_affine() : src buf[%d] is NULL\n", i);
			return HD_ERR_NULL_PTR;
		}

		if(!p_param->dst_img.p_phy_addr[i]){
			printf("vendor_gfx_affine() : dst buf[%d] is NULL\n", i);
			return HD_ERR_NULL_PTR;
		}

		if(!p_param->src_img.lineoffset[i] ||
		    p_param->src_img.lineoffset[i] != p_param->dst_img.lineoffset[i]){
			printf("vendor_gfx_affine() : invalid src lineoffset[%d](%d) or dst lineoffset[%d](%d)\n",
				i, p_param->src_img.lineoffset[i], i, p_param->dst_img.lineoffset[i]);
			return HD_ERR_NULL_PTR;
		}
	}

	fd = GFX_OPEN("/proc/nvt_gfx" , O_RDWR, S_IRUSR);
	//coverity[dead_error_condition]
	if(fd < 0){
		//coverity[dead_error_begin]
		printf("vendor_gfx_affine() : fail to open /proc/nvt_gfx\r\n");
		return HD_ERR_DRV;
	}

	command.version       = GFX_USER_VERSION;
	command.cmd           = GFX_USER_CMD_AFFINE;
	command.data.affine.w = p_param->src_img.dim.w;
	command.data.affine.h = p_param->src_img.dim.h;
	ret                   = GFX_IOCTL(fd , command.cmd, &command);
	GFX_CLOSE(fd);

	return ret;
}

HD_RESULT vendor_gfx_I8_colorkey(HD_GFX_COPY *p_param)
{
	GFX_USER_DATA    command;
	int              ret, fd;

	if(!p_param){
		printf("vendor_gfx_I8_colorkey() : NULL p_param\n");
		return HD_ERR_INV;
	}

	memset(&command, 0, sizeof(GFX_USER_DATA));
	command.version = GFX_USER_VERSION;
	command.cmd     = GFX_USER_CMD_I8_COLORKEY;

	ret = copy_hd_img_to_gfx(&p_param->src_img, &command.data.copy.src_img);
	if(ret){
		printf("vendor_gfx_I8_colorkey() fail to convert src image\n");
		return ret;
	}

	ret = copy_hd_img_to_gfx(&p_param->dst_img, &command.data.copy.dst_img);
	if(ret){
		printf("vendor_gfx_I8_colorkey() fail to convert dst image\n");
		return ret;
	}

	fd = GFX_OPEN("/proc/nvt_gfx" , O_RDWR, S_IRUSR);
	//coverity[dead_error_condition]
	if(fd < 0){
		//coverity[dead_error_begin]
		printf("vendor_gfx_I8_colorkey() : fail to open /proc/nvt_gfx\r\n");
		return HD_ERR_DRV;
	}

	command.data.copy.src_x    = p_param->src_region.x;
	command.data.copy.src_y    = p_param->src_region.y;
	command.data.copy.src_w    = p_param->src_region.w;
	command.data.copy.src_h    = p_param->src_region.h;
	command.data.copy.dst_x    = p_param->dst_pos.x;
	command.data.copy.dst_y    = p_param->dst_pos.y;
	command.data.copy.colorkey = p_param->colorkey;
	command.data.copy.alpha    = p_param->alpha;

	ret = GFX_IOCTL(fd , command.cmd, &command);
	GFX_CLOSE(fd);

	return ret;
}

HD_RESULT vendor_gfx_scale_link_list_buf(VENDOR_GFX_SCALE_LINK_LIST_BUF *p_param)
{
	GFX_USER_DATA    command;
	int              ret, fd;

	if(!p_param){
		printf("vendor_gfx_scale_link_list_buf() : NULL p_param\n");
		return HD_ERR_INV;
	}

	memset(&command, 0, sizeof(GFX_USER_DATA));
	command.version = GFX_USER_VERSION;
	command.cmd     = GFX_USER_CMD_SCALE_LINK_LIST_BUF;

	fd = GFX_OPEN("/proc/nvt_gfx" , O_RDWR, S_IRUSR);
	//coverity[dead_error_condition]
	if(fd < 0){
		//coverity[dead_error_begin]
		printf("vendor_gfx_scale_link_list_buf() : fail to open /proc/nvt_gfx\r\n");
		return HD_ERR_DRV;
	}

	command.data.scale_link_list_buf.p_addr = p_param->p_addr;
	command.data.scale_link_list_buf.length = p_param->length;

	ret = GFX_IOCTL(fd , command.cmd, &command);
	GFX_CLOSE(fd);

	return ret;
}

HD_RESULT vendor_gfx_scale_link_list(VENDOR_GFX_SCALE_DMA_FLUSH *p_param, int num)
{
	static int       fd = -1;
	int              ret, i;
	GFX_USER_DATA    ise_ll_job_list[MAX_JOB_IN_A_LIST];

	if(!p_param){
		printf("vendor_gfx_scale_link_list() : p_param is NULL\n");
		return HD_ERR_NULL_PTR;
	}

	if(num > MAX_JOB_IN_A_LIST){
		printf("vendor_gfx_scale_link_list() : num(%d) > max(%d)\n", num, MAX_JOB_IN_A_LIST);
		return HD_ERR_NULL_PTR;
	}

	if(fd < 0)
		fd = GFX_OPEN("/proc/nvt_gfx" , O_RDWR, S_IRUSR);
	//coverity[dead_error_condition]
	if(fd < 0){
		//coverity[dead_error_begin]
		printf("vendor_gfx_scale_link_list() : fail to open /proc/nvt_gfx\r\n");
		return HD_ERR_DRV;
	}

	memset(&ise_ll_job_list,  0, sizeof(ise_ll_job_list));
	for(i = 0 ; i < num ; ++i){
		ise_ll_job_list[i].version = GFX_USER_VERSION;
		ise_ll_job_list[i].cmd     = GFX_USER_CMD_SCALE_LINK_LIST;

		ret = copy_hd_img_to_gfx(&(p_param[i].src_img), &(ise_ll_job_list[i].data.ise_scale_dma_flush.src_img));
		if(ret){
			printf("vendor_gfx_scale_link_list() fail to convert src image\n");
			return ret;
		}

		ret = copy_hd_img_to_gfx(&(p_param[i].dst_img), &(ise_ll_job_list[i].data.ise_scale_dma_flush.dst_img));
		if(ret){
			printf("vendor_gfx_scale_link_list() fail to convert dst image\n");
			return ret;
		}
		ise_ll_job_list[i].data.ise_scale_dma_flush.src_x   = p_param[i].src_region.x;
		ise_ll_job_list[i].data.ise_scale_dma_flush.src_y   = p_param[i].src_region.y;
		ise_ll_job_list[i].data.ise_scale_dma_flush.src_w   = p_param[i].src_region.w;
		ise_ll_job_list[i].data.ise_scale_dma_flush.src_h   = p_param[i].src_region.h;
		ise_ll_job_list[i].data.ise_scale_dma_flush.dst_x   = p_param[i].dst_region.x;
		ise_ll_job_list[i].data.ise_scale_dma_flush.dst_y   = p_param[i].dst_region.y;
		ise_ll_job_list[i].data.ise_scale_dma_flush.dst_w   = p_param[i].dst_region.w;
		ise_ll_job_list[i].data.ise_scale_dma_flush.dst_h   = p_param[i].dst_region.h;
		ise_ll_job_list[i].data.ise_scale_dma_flush.method  = convert_scale_method(p_param[i].quality);
		if(ise_ll_job_list[i].data.ise_scale_dma_flush.method == GFX_SCALE_METHOD_MAX){
			printf("vendor_gfx_scale_link_list() quality(%d) is not supported\n", p_param[i].quality);
			return HD_ERR_NOT_SUPPORT;
		}
		ise_ll_job_list[i].data.ise_scale_dma_flush.in_buf_flush     = p_param[i].in_buf_flush;
		ise_ll_job_list[i].data.ise_scale_dma_flush.out_buf_flush    = p_param[i].out_buf_flush;
	}

	ret = GFX_IOCTL(fd , GFX_USER_CMD_SCALE_LINK_LIST, ise_ll_job_list);

	//GFX_CLOSE(fd);

	return ret;
}

HD_RESULT vendor_gfx_scale_dma_flush(HD_GFX_SCALE *p_param, int flush)
{
	int              fd = -1;
	int              ret;
	GFX_USER_DATA    command;

	if(!p_param){
		printf("vendor_gfx_scale_dma_flush() : p_param is NULL\n");
		return HD_ERR_NULL_PTR;
	}

	if(fd < 0)
		fd = GFX_OPEN("/proc/nvt_gfx" , O_RDWR, S_IRUSR);
	//coverity[dead_error_condition]
	if(fd < 0){
		//coverity[dead_error_begin]
		printf("vendor_gfx_scale_dma_flush() : fail to open /proc/nvt_gfx\r\n");
		return HD_ERR_DRV;
	}

	ret = config_scale_param(&command, p_param, flush);
	if(ret){
		printf("vendor_gfx_scale_dma_flush() fail to configure scale parameters\n");
		return ret;
	}

	ret = GFX_IOCTL(fd , command.cmd, &command);

	GFX_CLOSE(fd);

	return ret;
}

static HD_RESULT _vendor_gfx_alpha_blend(VENDOR_GFX_ALPHA_BLEND *p_param, UINT32 flush)
{
	int              i, fd, ret, plane = 1;
	GFX_USER_DATA    command;

	memset(&command, 0, sizeof(GFX_USER_DATA));

	if(!p_param){
		printf("vendor_gfx_alpha_blend() : p_param is NULL\n");
		return HD_ERR_NULL_PTR;
	}

	if (!(p_param->src_img.format == HD_VIDEO_PXLFMT_YUV420 && p_param->dst_img.format == HD_VIDEO_PXLFMT_YUV420) &&
	   !(p_param->src_img.format == HD_VIDEO_PXLFMT_Y8 && p_param->dst_img.format == HD_VIDEO_PXLFMT_Y8) &&
	   !(p_param->src_img.format == HD_VIDEO_PXLFMT_ARGB8888 && p_param->dst_img.format == HD_VIDEO_PXLFMT_ARGB8888)) {
		printf("vendor_gfx_alpha_blend() : invalid src format(%x) or dst format(%x). only yuv420Packed/Y8/argb8888 are supported\n",
			p_param->src_img.format, p_param->dst_img.format);
		return HD_ERR_NOT_SUPPORT;
	}

	if(!p_param->src_img.dim.w || !p_param->src_img.dim.h){
		printf("vendor_gfx_alpha_blend() : invalid src w(%d) or src h(%d)\n",
			p_param->src_img.dim.w, p_param->src_img.dim.h);
		return HD_ERR_INV;
	}

	if(!p_param->dst_img.dim.w || !p_param->dst_img.dim.h){
		printf("vendor_gfx_alpha_blend() : invalid dst w(%d) or dst h(%d)\n",
			(int)p_param->dst_img.dim.w, (int)p_param->dst_img.dim.h);
		return HD_ERR_INV;
	}

	if (p_param->src_img.format == HD_VIDEO_PXLFMT_YUV420) {
		plane = 2;
	} else if (p_param->src_img.format == HD_VIDEO_PXLFMT_Y8) {
		plane = 1;
	} else if (p_param->src_img.format == HD_VIDEO_PXLFMT_ARGB8888) {
		plane = 1;
	}

	for(i = 0 ; i < plane ; ++i){
		if(!p_param->src_img.p_phy_addr[i]){
			printf("vendor_gfx_alpha_blend() : src buf[%d] is NULL\n", i);
			return HD_ERR_NULL_PTR;
		}

		if(!p_param->dst_img.p_phy_addr[i]){
			printf("vendor_gfx_alpha_blend() : dst buf[%d] is NULL\n", i);
			return HD_ERR_NULL_PTR;
		}

		if(!p_param->src_img.lineoffset[i] || !p_param->dst_img.lineoffset[i]){
			printf("vendor_gfx_alpha_blend() : invalid src lineoffset[%d](%d) or dst lineoffset[%d](%d)\n",
				i, p_param->src_img.lineoffset[i], i, p_param->dst_img.lineoffset[i]);
			return HD_ERR_NULL_PTR;
		}
	}

	ret = copy_hd_img_to_gfx(&p_param->src_img, &command.data.alpha_blend.src_img);
	if(ret){
		printf("vendor_gfx_alpha_blend() : fail to convert src image\n");
		return ret;
	}

	ret = copy_hd_img_to_gfx(&p_param->dst_img, &command.data.alpha_blend.dst_img);
	if(ret){
		printf("vendor_gfx_alpha_blend() : fail to convert dst image\n");
		return ret;
	}

	fd = GFX_OPEN("/proc/nvt_gfx" , O_RDWR, S_IRUSR);
	//coverity[dead_error_condition]
	if(fd < 0){
		//coverity[dead_error_begin]
		printf("vendor_gfx_grph_trigger() : fail to open /proc/nvt_gfx\r\n");
		return HD_ERR_DRV;
	}

	command.version                  = GFX_USER_VERSION;
	command.cmd                      = GFX_USER_CMD_ALPHA_BLEND;
	command.data.alpha_blend.dst_x   = p_param->dst_pos.x;
	command.data.alpha_blend.dst_y   = p_param->dst_pos.y;
	command.data.alpha_blend.p_alpha = p_param->p_alpha;
	command.data.alpha_blend.flush   = flush;

	ret = GFX_IOCTL(fd , command.cmd, &command);
	GFX_CLOSE(fd);

	return ret;
}

HD_RESULT vendor_gfx_alpha_blend(VENDOR_GFX_ALPHA_BLEND *p_param)
{
	return _vendor_gfx_alpha_blend(p_param, 1);
}

HD_RESULT vendor_gfx_alpha_blend_no_flush(VENDOR_GFX_ALPHA_BLEND *p_param)
{
	return _vendor_gfx_alpha_blend(p_param, 0);
}

static HD_RESULT _vendor_gfx_grph_trigger_with_flush(VENDOR_GFX_ENG_ID id, VENDOR_GFX_GRPH_TRIGGER_PARAM *p_param, int flush)
{
	GFX_USER_DATA    command;
	int              ret, fd;

	if(!p_param){
		printf("vendor_gfx_grph_trigger() : NULL p_param\n");
		return HD_ERR_INV;
	}

	memset(&command, 0, sizeof(GFX_USER_DATA));
	command.version = GFX_USER_VERSION;
	command.cmd     = GFX_USER_CMD_RAW_GRAPHIC;

	fd = GFX_OPEN("/proc/nvt_gfx" , O_RDWR, S_IRUSR);
	//coverity[dead_error_condition]
	if(fd < 0){
		//coverity[dead_error_begin]
		printf("vendor_gfx_grph_trigger() : fail to open /proc/nvt_gfx\r\n");
		return HD_ERR_DRV;
	}

	if(id == VENDOR_GFX_ENG_ID1)
		command.data.raw_graphic.engine = 0;
	else if(id == VENDOR_GFX_ENG_ID2)
		command.data.raw_graphic.engine = 1;
	else{
		printf("vendor_gfx_grph_trigger() : invalid id(%d)\r\n", id);
		GFX_CLOSE(fd);
		return HD_ERR_INV;
	}

	command.data.raw_graphic.p_addr = (UINT32)p_param;
	command.data.raw_graphic.length = sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM);
	command.data.raw_graphic.flush  = flush;

	ret = GFX_IOCTL(fd , command.cmd, &command);
	GFX_CLOSE(fd);

	return ret;
}

HD_RESULT vendor_gfx_grph_trigger(VENDOR_GFX_ENG_ID id, VENDOR_GFX_GRPH_TRIGGER_PARAM *p_param)
{
	return _vendor_gfx_grph_trigger_with_flush(id, p_param, 1);
}

HD_RESULT vendor_gfx_grph_trigger_no_flush(VENDOR_GFX_ENG_ID id, VENDOR_GFX_GRPH_TRIGGER_PARAM *p_param)
{
	return _vendor_gfx_grph_trigger_with_flush(id, p_param, 0);
}

HD_RESULT vendor_gfx_copy_no_flush(HD_GFX_COPY *p_param)
{
	return _hd_gfx_copy(p_param, 0, GFX_GRPH_ENGINE_1);
}

HD_RESULT vendor_gfx_rotate_no_flush(HD_GFX_ROTATE *p_param)
{
	extern HD_RESULT _hd_gfx_rotate(HD_GFX_ROTATE *p_param, UINT32 flush);

	return _hd_gfx_rotate(p_param, 0);
}

HD_RESULT vendor_gfx_422_packed_to_yuyv(HD_GFX_COLOR_TRANSFORM *p_param)
{
	int              i, fd, ret;
	GFX_USER_DATA    command;

	memset(&command, 0, sizeof(GFX_USER_DATA));

	if(!p_param){
		printf("vendor_gfx_422_packed_to_yuyv() : p_param is NULL\n");
		return HD_ERR_NULL_PTR;
	}

	p_param->src_img.format = HD_VIDEO_PXLFMT_YUV422;
	p_param->dst_img.format = HD_VIDEO_PXLFMT_YUV422_ONE;

	if(!p_param->src_img.dim.w || !p_param->src_img.dim.h){
		printf("vendor_gfx_422_packed_to_yuyv() : invalid src w(%d) or src h(%d)\n",
			p_param->src_img.dim.w, p_param->src_img.dim.h);
		return HD_ERR_INV;
	}

	if(!p_param->dst_img.dim.w || !p_param->dst_img.dim.h){
		printf("vendor_gfx_422_packed_to_yuyv() : invalid dst w(%d) or dst h(%d)\n",
			p_param->dst_img.dim.w, p_param->dst_img.dim.h);
		return HD_ERR_INV;
	}

	for(i = 0 ; i < 2 ; ++i){
		if(!p_param->src_img.p_phy_addr[i]){
			printf("vendor_gfx_422_packed_to_yuyv() : src buf[%d] is NULL\n", i);
			return HD_ERR_NULL_PTR;
		}

		if(!p_param->src_img.lineoffset[i]){
			printf("vendor_gfx_422_packed_to_yuyv() : invalid src lineoffset[%d](%d)\n",
				i, p_param->src_img.lineoffset[i]);
			return HD_ERR_NULL_PTR;
		}
	}

	if(!p_param->dst_img.p_phy_addr[0]){
		printf("vendor_gfx_422_packed_to_yuyv() : dst buf[0] is NULL\n", i);
		return HD_ERR_NULL_PTR;
	}

	if(2*p_param->src_img.lineoffset[0] != p_param->dst_img.lineoffset[0]){
		printf("vendor_gfx_422_packed_to_yuyv() : 2*src lineoffset[0](%d) should be dst lineoffset[0](%d)\n",
			p_param->src_img.lineoffset[0], p_param->dst_img.lineoffset[0]);
		return HD_ERR_NULL_PTR;
	}

	ret = copy_hd_img_to_gfx(&p_param->src_img, &command.data.color_transform.src_img);
	if(ret){
		printf("vendor_gfx_422_packed_to_yuyv() : fail to convert src image\n");
		return ret;
	}

	ret = copy_hd_img_to_gfx(&p_param->dst_img, &command.data.color_transform.dst_img);
	if(ret){
		printf("vendor_gfx_422_packed_to_yuyv() : fail to convert dst image\n");
		return ret;
	}

	fd = GFX_OPEN("/proc/nvt_gfx" , O_RDWR, S_IRUSR);
	//coverity[dead_error_condition]
	if(fd < 0){
		//coverity[dead_error_condition]
		printf("vendor_gfx_422_packed_to_yuyv() : fail to open /proc/nvt_gfx\r\n");
		return HD_ERR_DRV;
	}

	command.version = GFX_USER_VERSION;
	command.cmd     = GFX_USER_CMD_COLOR_TRANSFORM;

	ret = GFX_IOCTL(fd , command.cmd, &command);
	GFX_CLOSE(fd);

	return ret;
}

HD_RESULT vendor_gfx_copy_no_flush_eng2(HD_GFX_COPY *p_param)
{

	return _hd_gfx_copy(p_param, 0, GFX_GRPH_ENGINE_2);
}

HD_RESULT vendor_gfx_force_use_physical_address(int force_pa)
{
	int              fd, ret;
	GFX_USER_DATA    command;

	memset(&command, 0, sizeof(GFX_USER_DATA));

	fd = GFX_OPEN("/proc/nvt_gfx" , O_RDWR, S_IRUSR);
	//coverity[dead_error_condition]
	if(fd < 0){
		//coverity[dead_error_condition]
		printf("vendor_gfx_force_use_physical_address() : fail to open /proc/nvt_gfx\r\n");
		return HD_ERR_DRV;
	}

	command.version = GFX_USER_VERSION;
	command.cmd     = GFX_USER_CMD_FORCE_USE_PHYSICAL_ADDRESS;
	command.data.force_use_physical_address.force_physical_address = force_pa;

	ret = GFX_IOCTL(fd , command.cmd, &command);
	GFX_CLOSE(fd);

	return ret;
}

HD_RESULT vendor_gfx_draw_rect_no_flush(HD_GFX_DRAW_RECT *p_param)
{
	return _hd_gfx_draw_rect(p_param, 0);
}

HD_RESULT vendor_gfx_draw_line_no_flush(HD_GFX_DRAW_LINE *p_param)
{
	return _hd_gfx_draw_line(p_param, 0);
}

HD_RESULT vendor_gfx_draw_mask(HD_GFX_DRAW_MASK *p_param, int flush)
{
	int              fd, ret;
	GFX_USER_DATA    command;

	memset(&command, 0, sizeof(GFX_USER_DATA));

	if(!p_param){
		printf("vendor_gfx_draw_mask() : p_param is NULL\n");
		return HD_ERR_NULL_PTR;
	}

	if(!p_param->dst_img.dim.w || !p_param->dst_img.dim.h){
		printf("vendor_gfx_draw_mask() : invalid dst w(%d) or dst h(%d)\n",
			p_param->dst_img.dim.w, p_param->dst_img.dim.h);
		return HD_ERR_INV;
	}

	if(!p_param->dst_img.p_phy_addr[0]){
		printf("vendor_gfx_draw_mask() : dst buf[0] is NULL\n");
		return HD_ERR_NULL_PTR;
	}

	ret = copy_hd_img_to_gfx(&p_param->dst_img, &command.data.mask.dst_img);
	if(ret){
		printf("vendor_gfx_draw_mask() : fail to convert dst image\n");
		return ret;
	}

	fd = GFX_OPEN("/proc/nvt_gfx" , O_RDWR, S_IRUSR);
	//coverity[dead_error_condition]
	if(fd < 0){
		//coverity[dead_error_condition]
		printf("vendor_gfx_draw_mask() : fail to open /proc/nvt_gfx\r\n");
		return HD_ERR_DRV;
	}

	command.version = GFX_USER_VERSION;
	command.cmd     = GFX_USER_CMD_DRAW_MASK;
	
	command.data.mask.color     = p_param->color;
	command.data.mask.x[0]      = p_param->pos[0].x;
	command.data.mask.y[0]      = p_param->pos[0].y;
	command.data.mask.x[1]      = p_param->pos[1].x;
	command.data.mask.y[1]      = p_param->pos[1].y;
	command.data.mask.x[2]      = p_param->pos[2].x;
	command.data.mask.y[2]      = p_param->pos[2].y;
	command.data.mask.x[3]      = p_param->pos[3].x;
	command.data.mask.y[3]      = p_param->pos[3].y;
	command.data.mask.thickness = p_param->thickness;
	command.data.mask.flush     = flush;
	
	if(p_param->type == HD_GFX_RECT_SOLID)
		command.data.mask.type = GFX_RECT_TYPE_SOLID;
	else if(p_param->type == HD_GFX_RECT_HOLLOW)
		command.data.mask.type = GFX_RECT_TYPE_HOLLOW;
	else{
		printf("vendor_gfx_draw_mask() : rect type(%d) is not supported\n", p_param->type);
		return HD_ERR_NOT_SUPPORT;
	}

	ret = GFX_IOCTL(fd , command.cmd, &command);
	GFX_CLOSE(fd);

	return ret;
}