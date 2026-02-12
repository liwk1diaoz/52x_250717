/**
	@brief Source file of vf_gfx.

	@file vf_gfx.c
	
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
#include "vf_gfx.h"
#include "vendor_gfx.h"

static BOOL gximg_calc_plane_rect(HD_VIDEO_FRAME *p_img_buf,  UINT32 plane, HD_IRECT    *p_rect, HD_IRECT    *p_plane_rect)
{
	if (p_rect == 0) {
		printf("p_rect is NULL\r\n");
		return HD_ERR_INV;
	}

	if (plane >= HD_VIDEO_MAX_PLANE) {
		printf("plane(%d) is invalid\r\n", plane);
		return HD_ERR_INV;
	}

	p_plane_rect->x = p_rect->x;
	p_plane_rect->y = p_rect->y;
	p_plane_rect->w = p_rect->w;
	p_plane_rect->h = p_rect->h;

	switch (p_img_buf->pxlfmt) {
	case HD_VIDEO_PXLFMT_YUV444_PLANAR:
		break;
	case HD_VIDEO_PXLFMT_YUV422_PLANAR:
		if (plane > 0) {
			p_plane_rect->x >>= 1;
			p_plane_rect->w >>= 1;
		}
		break;
	case HD_VIDEO_PXLFMT_YUV420_PLANAR:
		if (plane > 0) {
			p_plane_rect->x >>= 1;
			p_plane_rect->y >>= 1;
			p_plane_rect->w >>= 1;
			p_plane_rect->h >>= 1;
		}
		break;
	case HD_VIDEO_PXLFMT_YUV422:
		break;
	case HD_VIDEO_PXLFMT_YUV420:
		if (plane > 0) {
			p_plane_rect->y >>= 1;
			p_plane_rect->h >>= 1;
		}
		break;
	case HD_VIDEO_PXLFMT_Y8:
	case HD_VIDEO_PXLFMT_I8:
	case HD_VIDEO_PXLFMT_RGB888_PLANAR:
		break;
	case HD_VIDEO_PXLFMT_ARGB8888:
		p_plane_rect->x <<= 2;
		p_plane_rect->w <<= 2;
		break;
	case HD_VIDEO_PXLFMT_ARGB8565:
		if (plane > 0) {
			p_plane_rect->x <<= 1;
			p_plane_rect->y <<= 1;
			p_plane_rect->w <<= 1;
			p_plane_rect->h <<= 1;
		}
		break;
	case HD_VIDEO_PXLFMT_ARGB1555:
	case HD_VIDEO_PXLFMT_ARGB4444:
	case HD_VIDEO_PXLFMT_RGB565:
		p_plane_rect->x <<= 1;
		p_plane_rect->w <<= 1;
		break;
	case HD_VIDEO_PXLFMT_I1:
			p_plane_rect->x >>= 3;
			p_plane_rect->w >>= 3;
		break;
	case HD_VIDEO_PXLFMT_I2:
			p_plane_rect->x >>= 2;
			p_plane_rect->w >>= 2;
		break;
	case HD_VIDEO_PXLFMT_I4:
			p_plane_rect->x >>= 1;
			p_plane_rect->w >>= 1;
		break;
	default:
		printf("pixel format %d\r\n", p_img_buf->pxlfmt);
		return HD_ERR_INV;
	}
	return HD_OK;
}

HD_RESULT vf_init(HD_VIDEO_FRAME *p_img_buf, UINT32 width, UINT32 height, HD_VIDEO_PXLFMT pxlfmt, UINT32 lineoff, UINT32 addr, UINT32 available_size)
{
	UINT32    need_size, plane_num, i;
	HD_IRECT  tmp_rect = {0}, tmp_plane_rect = {0};
	UINT32    buf_end;

	if (!p_img_buf) {
		printf("p_img_buf is Null\r\n");
		return HD_ERR_INV;
	}	
	if (!addr) {
		printf("addr is Null\r\n");
		return HD_ERR_INV;
	}
	memset(p_img_buf, 0x00, sizeof(HD_VIDEO_FRAME));
	if (lineoff & VF_GFX_LINEOFFSET_PATTERN) {
		p_img_buf->loff[0] = ALIGN_CEIL(width, lineoff - VF_GFX_LINEOFFSET_PATTERN);
	} else if (lineoff) {
		p_img_buf->loff[0] = lineoff;
	}
	// line offset is 0
	else {
		p_img_buf->loff[0] = ALIGN_CEIL(width, 4);
	}
	switch (pxlfmt) {
	case HD_VIDEO_PXLFMT_YUV422_PLANAR:
		p_img_buf->loff[1] = p_img_buf->loff[0] >> 1;
		p_img_buf->loff[2] = p_img_buf->loff[1];
		plane_num = 3;
		break;

	case HD_VIDEO_PXLFMT_YUV420_PLANAR:
		p_img_buf->loff[1] = p_img_buf->loff[0] >> 1;
		p_img_buf->loff[2] = p_img_buf->loff[1];
		plane_num = 3;
		break;

	case HD_VIDEO_PXLFMT_YUV422:
		p_img_buf->loff[1] = p_img_buf->loff[0];
		p_img_buf->loff[2] = p_img_buf->loff[1];
		plane_num = 2;
		break;

	case HD_VIDEO_PXLFMT_YUV420:
		p_img_buf->loff[1] = p_img_buf->loff[0];
		p_img_buf->loff[2] = p_img_buf->loff[1];
		plane_num = 2;
		break;

	case HD_VIDEO_PXLFMT_Y8:
	case HD_VIDEO_PXLFMT_I8:
		p_img_buf->loff[1] = 0;
		p_img_buf->loff[2] = 0;
		plane_num = 1;
		break;

	case HD_VIDEO_PXLFMT_ARGB8565:
		p_img_buf->loff[1] = p_img_buf->loff[0];
		plane_num = 2;
		break;

	case HD_VIDEO_PXLFMT_RGB888_PLANAR:
		p_img_buf->loff[1] = p_img_buf->loff[0];
		p_img_buf->loff[2] = p_img_buf->loff[1];
		plane_num = 3;
		break;

	case HD_VIDEO_PXLFMT_ARGB8888:
		plane_num = 1;
		break;

	case HD_VIDEO_PXLFMT_ARGB1555:
	case HD_VIDEO_PXLFMT_ARGB4444:
	case HD_VIDEO_PXLFMT_RGB565:
		p_img_buf->loff[0] = p_img_buf->loff[0];
		plane_num = 1;
		break;

	case HD_VIDEO_PXLFMT_I1:
		p_img_buf->loff[0] = (p_img_buf->loff[0] >> 3);
		plane_num = 1;
		break;
	case HD_VIDEO_PXLFMT_I2:
		p_img_buf->loff[0] = (p_img_buf->loff[0] >> 2);
		plane_num = 1;
		break;
	case HD_VIDEO_PXLFMT_I4:
		p_img_buf->loff[0] = (p_img_buf->loff[0] >> 1);
		plane_num = 1;
		break;

	default:
		printf("pixel format %d\r\n", p_img_buf->pxlfmt);
		return HD_ERR_INV;
	}
	p_img_buf->dim.w = width;
	p_img_buf->dim.h = height;
	p_img_buf->pxlfmt = pxlfmt;
	p_img_buf->phy_addr[0] = addr;
	buf_end = addr;
	tmp_rect.w = width;
	tmp_rect.h = height;

	for (i = 1; i < plane_num; i++) {
		gximg_calc_plane_rect(p_img_buf, i - 1, &tmp_rect, &tmp_plane_rect);
		p_img_buf->phy_addr[i] = buf_end + (tmp_plane_rect.h) * p_img_buf->loff[i - 1];
		buf_end += (tmp_plane_rect.h) * p_img_buf->loff[i - 1];
		p_img_buf->pw[i - 1] = tmp_plane_rect.w;
		p_img_buf->ph[i - 1] = tmp_plane_rect.h;
	}
	gximg_calc_plane_rect(p_img_buf, plane_num - 1, &tmp_rect, &tmp_plane_rect);
	p_img_buf->pw[plane_num - 1] = tmp_plane_rect.w;
	p_img_buf->ph[plane_num - 1] = tmp_plane_rect.h;
	buf_end += (tmp_plane_rect.h) * p_img_buf->loff[plane_num - 1];

	need_size = buf_end - addr;
	if (available_size < need_size) {
		printf("need_size 0x%x > Available size 0x%x\r\n", need_size, available_size);
		return HD_ERR_INV;
	}
	p_img_buf->sign = MAKEFOURCC('V','F','R','M');
	p_img_buf->count = 1;
	p_img_buf->timestamp = hd_gettime_us();

	return HD_OK;
}

HD_RESULT vf_init_ex(HD_VIDEO_FRAME *p_img_buf, UINT32 width, UINT32 height, HD_VIDEO_PXLFMT pxlfmt, UINT32 lineoff[HD_VIDEO_MAX_PLANE], UINT32 addr[HD_VIDEO_MAX_PLANE])
{
	UINT32    plane_num, i;
	HD_IRECT  tmp_rect = {0}, tmp_plane_rect = {0};

	if (!p_img_buf) {
		printf("p_img_buf is Null\r\n");
		return HD_ERR_INV;
	}
	if (!addr) {
		printf("addr is Null\r\n");
		return HD_ERR_INV;
	}
	if (!lineoff) {
		printf("lineoff is Null\r\n");
		return HD_ERR_INV;
	}
	switch (pxlfmt) {
	case HD_VIDEO_PXLFMT_YUV444_PLANAR:
		plane_num = 3;
		break;
	case HD_VIDEO_PXLFMT_YUV422_PLANAR:
		plane_num = 3;
		break;
	case HD_VIDEO_PXLFMT_YUV420_PLANAR:
		plane_num = 3;
		break;
	case HD_VIDEO_PXLFMT_YUV422:
		plane_num = 2;
		break;

	case HD_VIDEO_PXLFMT_YUV420:
		plane_num = 2;
		break;

	case HD_VIDEO_PXLFMT_Y8:
	case HD_VIDEO_PXLFMT_I8:
		plane_num = 1;
		break;

	case HD_VIDEO_PXLFMT_ARGB8565:
		plane_num = 2;
		break;

	case HD_VIDEO_PXLFMT_RGB888_PLANAR:
		plane_num = 3;
		break;

	case HD_VIDEO_PXLFMT_ARGB8888:
		plane_num = 1;
		break;

	case HD_VIDEO_PXLFMT_ARGB1555:
	case HD_VIDEO_PXLFMT_ARGB4444:
	case HD_VIDEO_PXLFMT_RGB565:
		plane_num = 1;
		break;
		
	case HD_VIDEO_PXLFMT_I1:
	case HD_VIDEO_PXLFMT_I2:
	case HD_VIDEO_PXLFMT_I4:
	case HD_VIDEO_PXLFMT_RAW8:
	case HD_VIDEO_PXLFMT_RAW10:
	case HD_VIDEO_PXLFMT_RAW12:
	case HD_VIDEO_PXLFMT_RAW16:
		plane_num = 1;
		break;

	default:
		printf("pixel format %d\r\n", pxlfmt);
		return HD_ERR_INV;
	}
	memset(p_img_buf, 0x00, sizeof(HD_VIDEO_FRAME));
	p_img_buf->dim.w = width;
	p_img_buf->dim.h = height;
	p_img_buf->pxlfmt = pxlfmt;
	tmp_rect.w = width;
	tmp_rect.h = height;
	for (i = 0; i < plane_num; i++) {
		p_img_buf->loff[i] = lineoff[i];
		p_img_buf->phy_addr[i] = addr[i];
		gximg_calc_plane_rect(p_img_buf, i, &tmp_rect, &tmp_plane_rect);
		p_img_buf->pw[i] = tmp_plane_rect.w;
		p_img_buf->ph[i] = tmp_plane_rect.h;
	}
	p_img_buf->sign = MAKEFOURCC('V','F','R','M');
	p_img_buf->count = 1;
	p_img_buf->timestamp = hd_gettime_us();
	
	return HD_OK;
}

static int vf_to_gfx(HD_VIDEO_FRAME *vf, HD_GFX_IMG_BUF *gfx)
{
	UINT32 plane_num, i;
	
	if(!vf || !gfx){
		printf("vf_to_gfx:vf(%p) or gfx(%p) is NULL\n", vf, gfx);
		return -1;
	}
	
	plane_num = (HD_VIDEO_MAX_PLANE > HD_GFX_MAX_PLANE_NUM ? 
					HD_GFX_MAX_PLANE_NUM : HD_VIDEO_MAX_PLANE);
	
	memset(gfx, 0, sizeof(HD_GFX_IMG_BUF));
	gfx->dim.w  = vf->dim.w;
	gfx->dim.h  = vf->dim.h;
	gfx->format = vf->pxlfmt;
    gfx->ddr_id = vf->ddr_id;
	for(i = 0 ; i < plane_num ; ++i){
		gfx->p_phy_addr[i] = vf->phy_addr[i];
		gfx->lineoffset[i] = vf->loff[i];
	}
	
	return 0;
}

HD_RESULT vf_gfx_copy(VF_GFX_COPY *p_param)
{
	HD_GFX_COPY param;
	int         ret;
	
	if(!p_param){
		printf("vf_gfx_copy:p_param is NULL\n");
		return HD_ERR_INV;
	}
	
	ret = vf_to_gfx(&p_param->src_img, &param.src_img);
	if(ret){
		printf("vf_gfx_copy:fail to convert src image\n");
		return HD_ERR_INV;
	}
	
	ret = vf_to_gfx(&p_param->dst_img, &param.dst_img);
	if(ret){
		printf("vf_gfx_copy:fail to convert dst image\n");
		return HD_ERR_INV;
	}
	
	memcpy(&param.src_region, &p_param->src_region, sizeof(HD_IRECT));
	memcpy(&param.dst_pos, &p_param->dst_pos, sizeof(HD_IPOINT));
	param.colorkey = p_param->colorkey;
	param.alpha    = p_param->alpha;
	
	return hd_gfx_copy(&param);
}

HD_RESULT vf_gfx_scale(VF_GFX_SCALE *p_param, int flush)
{
	HD_GFX_SCALE param;
	int          ret;
	
	if(!p_param){
		printf("vf_gfx_scale:p_param is NULL\n");
		return HD_ERR_INV;
	}
	
	ret = vf_to_gfx(&p_param->src_img, &param.src_img);
	if(ret){
		printf("vf_gfx_scale:fail to convert src image\n");
		return HD_ERR_INV;
	}
	
	ret = vf_to_gfx(&p_param->dst_img, &param.dst_img);
	if(ret){
		printf("vf_gfx_scale:fail to convert dst image\n");
		return HD_ERR_INV;
	}
	
	memcpy(&param.src_region, &p_param->src_region, sizeof(HD_IRECT));
	memcpy(&param.dst_region, &p_param->dst_region, sizeof(HD_IRECT));
	param.quality = p_param->quality;
	
	return vendor_gfx_scale_dma_flush(&param, flush);
}

HD_RESULT vf_gfx_rotate(VF_GFX_ROTATE *p_param)
{
	HD_GFX_ROTATE param;
	int           ret;
	
	if(!p_param){
		printf("vf_gfx_rotate:p_param is NULL\n");
		return HD_ERR_INV;
	}
	
	ret = vf_to_gfx(&p_param->src_img, &param.src_img);
	if(ret){
		printf("vf_gfx_rotate:fail to convert src image\n");
		return HD_ERR_INV;
	}
	
	ret = vf_to_gfx(&p_param->dst_img, &param.dst_img);
	if(ret){
		printf("vf_gfx_rotate:fail to convert dst image\n");
		return HD_ERR_INV;
	}
	
	memcpy(&param.src_region, &p_param->src_region, sizeof(HD_IRECT));
	memcpy(&param.dst_pos, &p_param->dst_pos, sizeof(HD_IPOINT));
	param.angle  = p_param->angle;
	
	return hd_gfx_rotate(&param);
}

HD_RESULT vf_gfx_draw_rect(VF_GFX_DRAW_RECT *p_param)
{
	HD_GFX_DRAW_RECT param;
	int              ret;
	
	if(!p_param){
		printf("vf_gfx_draw_rect:p_param is NULL\n");
		return HD_ERR_INV;
	}
	
	ret = vf_to_gfx(&p_param->dst_img, &param.dst_img);
	if(ret){
		printf("vf_gfx_draw_rect:fail to convert dst image\n");
		return HD_ERR_INV;
	}
	
	memcpy(&param.rect, &p_param->rect, sizeof(HD_IRECT));
	param.color     = p_param->color;
	param.type      = p_param->type;
	param.thickness = p_param->thickness;
	
	return hd_gfx_draw_rect(&param);
}

HD_RESULT vf_gfx_I8_colorkey(VF_GFX_COPY *p_param)
{
	HD_GFX_COPY param;
	int         ret;
	
	if(!p_param){
		printf("vf_gfx_I8_colorkey:p_param is NULL\n");
		return HD_ERR_INV;
	}
	
	ret = vf_to_gfx(&p_param->src_img, &param.src_img);
	if(ret){
		printf("vf_gfx_I8_colorkey:fail to convert src image\n");
		return HD_ERR_INV;
	}
	
	ret = vf_to_gfx(&p_param->dst_img, &param.dst_img);
	if(ret){
		printf("vf_gfx_I8_colorkey:fail to convert dst image\n");
		return HD_ERR_INV;
	}
	
	memcpy(&param.src_region, &p_param->src_region, sizeof(HD_IRECT));
	memcpy(&param.dst_pos, &p_param->dst_pos, sizeof(HD_IPOINT));
	param.colorkey = p_param->colorkey;
	param.alpha    = p_param->alpha;
	
	return vendor_gfx_I8_colorkey(&param);
}
