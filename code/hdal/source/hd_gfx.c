#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hdal.h"
#define HD_MODULE_NAME HD_GFX
#include "hd_int.h"
#include "gximage/hd_gximage.h"
#include "hd_logger_p.h"
#if defined(__LINUX)
#include <sys/mman.h>
#include <pthread.h>
#endif
#if defined(__FREERTOS)
#include <FreeRTOS.h>
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#endif

#define MAX_JOB_LIST 4

HD_GFX_HANDLE job_handle[MAX_JOB_LIST];
GFX_USER_DATA job_list1[MAX_JOB_IN_A_LIST + 1];
GFX_USER_DATA job_list2[MAX_JOB_IN_A_LIST + 1];
GFX_USER_DATA job_list3[MAX_JOB_IN_A_LIST + 1];
GFX_USER_DATA job_list4[MAX_JOB_IN_A_LIST + 1];

static int proc_fd = -1;
static int gfx_is_init = 0;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

#define CHECK_INIT() {if(!gfx_is_init){printf("hd_gfx is not init\n");return HD_ERR_UNINIT;}}

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#define GFX_OPEN(...) 0
#define GFX_IOCTL nvt_gfx_ioctl
#define GFX_CLOSE(...)
#endif
#if defined(__LINUX)
#define GFX_OPEN  open
#define GFX_IOCTL ioctl
#define GFX_CLOSE close
#endif
static int open_proc_fd(void)
{
	if(proc_fd >= 0)
		return HD_OK;

	proc_fd = GFX_OPEN("/proc/nvt_gfx" , O_RDWR, S_IRUSR);
	if(proc_fd < 0){
		printf("gfx : fail to open /proc/nvt_gfx\r\n");
		return HD_ERR_DRV;
	}
	
	return 0;
}

static int send_command(GFX_USER_DATA *command)
{
	int ret = 0;
	
	if(!command){
		printf("gfx : command is NULL\r\n");
		return HD_ERR_NG;
	}
	
	if(proc_fd < 0){
		ret = open_proc_fd();
		if(ret){
			printf("gfx : open_proc_fd() fail\r\n");
			return ret;
		}
	}

	return GFX_IOCTL(proc_fd , command->cmd, command);
}

static VDO_PXLFMT convert_format(HD_VIDEO_PXLFMT format)
{
	switch (format) 
	{
		case HD_VIDEO_PXLFMT_I1:
			return VDO_PXLFMT_I1;
		case HD_VIDEO_PXLFMT_I2:
			return VDO_PXLFMT_I2;
		case HD_VIDEO_PXLFMT_I4:
			return VDO_PXLFMT_I4;
		case HD_VIDEO_PXLFMT_I8:
			return VDO_PXLFMT_I8;
		case HD_VIDEO_PXLFMT_RGB888_PLANAR:
			return VDO_PXLFMT_RGB888_PLANAR;
		case HD_VIDEO_PXLFMT_RGB888:
			return VDO_PXLFMT_RGB888;
		case HD_VIDEO_PXLFMT_RGB565:
			return VDO_PXLFMT_RGB565;
		case HD_VIDEO_PXLFMT_ARGB1555:
			return VDO_PXLFMT_ARGB1555;
		case HD_VIDEO_PXLFMT_ARGB4444:
			return VDO_PXLFMT_ARGB4444;
		case HD_VIDEO_PXLFMT_ARGB8888:
			return VDO_PXLFMT_ARGB8888;
		case HD_VIDEO_PXLFMT_Y8:
			return VDO_PXLFMT_Y8;
		case HD_VIDEO_PXLFMT_YUV420_PLANAR:
			return VDO_PXLFMT_YUV420_PLANAR;
		case HD_VIDEO_PXLFMT_YUV420:
			return VDO_PXLFMT_YUV420;
		case HD_VIDEO_PXLFMT_YUV422_PLANAR:
			return VDO_PXLFMT_YUV422_PLANAR;
		case HD_VIDEO_PXLFMT_YUV422:
			return VDO_PXLFMT_YUV422;
		case HD_VIDEO_PXLFMT_YUV444_PLANAR:
			return VDO_PXLFMT_YUV444_PLANAR;
		case HD_VIDEO_PXLFMT_YUV444:
			return VDO_PXLFMT_YUV444;
		case HD_VIDEO_PXLFMT_YUV422_ONE:
			return VDO_PXLFMT_YUV422_ONE;
		case HD_VIDEO_PXLFMT_RAW16:
			return VDO_PXLFMT_RAW16;
		default :
			return 0;
	};
	
	return 0;
}

static int check_format_and_buffer(HD_VIDEO_PXLFMT format, UINT32 *p_addr)
{
	if(!p_addr){
		printf("gfx : check_format_and_buffer() has NULL p_addr\n");
		return -1;
	}

	switch (format)
	{
		case HD_VIDEO_PXLFMT_I1:
		case HD_VIDEO_PXLFMT_I2:
		case HD_VIDEO_PXLFMT_I4:
		case HD_VIDEO_PXLFMT_I8:
		case HD_VIDEO_PXLFMT_Y8:
		case HD_VIDEO_PXLFMT_RGB888:
		case HD_VIDEO_PXLFMT_RGB565:
		case HD_VIDEO_PXLFMT_ARGB1555:
		case HD_VIDEO_PXLFMT_ARGB4444:
		case HD_VIDEO_PXLFMT_ARGB8888:
		case HD_VIDEO_PXLFMT_YUV422_ONE:
		case HD_VIDEO_PXLFMT_RAW16:
			if(!p_addr[0]){
				printf("gfx : image format(%x) must have valid p_addr[0]\n", format);
				return -1;
			}else
				return 0;
		case HD_VIDEO_PXLFMT_YUV420:
		case HD_VIDEO_PXLFMT_YUV422:
		case HD_VIDEO_PXLFMT_YUV444:
			if(!p_addr[0]){
				printf("gfx : image format(%x) must have valid p_addr[0]\n", format);
				return -1;
			}else if(!p_addr[1]){
				printf("gfx : image format(%x) must have valid p_addr[1]\n", format);
				return -1;
			}else
				return 0;
		case HD_VIDEO_PXLFMT_RGB888_PLANAR:
		case HD_VIDEO_PXLFMT_YUV420_PLANAR:
		case HD_VIDEO_PXLFMT_YUV422_PLANAR:
		case HD_VIDEO_PXLFMT_YUV444_PLANAR:
			if(!p_addr[0]){
				printf("gfx : image format(%x) must have valid p_addr[0]\n", format);
				return -1;
			}else if(!p_addr[1]){
				printf("gfx : image format(%x) must have valid p_addr[1]\n", format);
				return -1;
			}else if(!p_addr[2]){
				printf("gfx : image format(%x) must have valid p_addr[2]\n", format);
				return -1;
			}else
				return 0;
		default :
			printf("gfx : check_format_and_buffer() got unknown format(%x)\n", format);
			return -1;
	};

	return -1;
}

int copy_hd_img_to_gfx(HD_GFX_IMG_BUF *src, GFX_IMG_BUF *dst)
{
	int i;
	
	if(HD_GFX_MAX_PLANE_NUM != GFX_MAX_PLANE_NUM){
		printf("copy_hd_img_to_gfx() : HD_GFX_MAX_PLANE_NUM(%d) != GFX_MAX_PLANE_NUM(%d)\n", 
			HD_GFX_MAX_PLANE_NUM, GFX_MAX_PLANE_NUM);
		return HD_ERR_NG;
	}
	
	if(!src || !dst){
		printf("copy_hd_img_to_gfx : src(%p) or dst(%p) is NULL\n", src, dst);
		return HD_ERR_NULL_PTR;
	}

	if(check_format_and_buffer(src->format, src->p_phy_addr)){
		printf("copy_hd_img_to_gfx : check_format_and_buffer() fail\n");
                return HD_ERR_NULL_PTR;
	}
	
	dst->w        = src->dim.w;
	dst->h        = src->dim.h;
	dst->format   = convert_format(src->format);
	
	if(!dst->format){
		printf("copy_hd_img_to_gfx() : unsupported format(%x)\n", src->format);
		return HD_ERR_NOT_SUPPORT;
	}

	for(i = 0 ; i < HD_GFX_MAX_PLANE_NUM ; ++i){
		dst->p_phy_addr[i] = src->p_phy_addr[i];
		dst->lineoffset[i] = src->lineoffset[i];
	}
	
	memcpy(dst->palette, src->palette, sizeof(src->palette));
	
	return 0;
}

GFX_SCALE_METHOD convert_scale_method(HD_GFX_SCALE_QUALITY quality)
{
	if(quality == HD_GFX_SCALE_QUALITY_NULL)
		return GFX_SCALE_METHOD_NULL;
	else if(quality == HD_GFX_SCALE_QUALITY_BICUBIC)
		return GFX_SCALE_METHOD_BICUBIC;
	else if(quality == HD_GFX_SCALE_QUALITY_BILINEAR)
		return GFX_SCALE_METHOD_BILINEAR;
	else if(quality == HD_GFX_SCALE_QUALITY_NEAREST)
		return GFX_SCALE_METHOD_NEAREST;
	else if(quality == HD_GFX_SCALE_QUALITY_INTEGRATION)
		return GFX_SCALE_METHOD_INTEGRATION;
	
	return HD_GFX_SCALE_QUALITY_MAX;
}

static GFX_ROTATE_ANGLE convert_rotation_angle(UINT32 angle)
{
	switch (angle) 
	{
		case HD_VIDEO_DIR_ROTATE_90 :
			return GFX_ROTATE_ANGLE_90;
		case HD_VIDEO_DIR_ROTATE_180 :
			return GFX_ROTATE_ANGLE_180;
		case HD_VIDEO_DIR_ROTATE_270 :
			return GFX_ROTATE_ANGLE_270;
		case HD_VIDEO_DIR_MIRRORX :
			return GFX_ROTATE_ANGLE_MIRROR_X;
		case HD_VIDEO_DIR_MIRRORY :
			return GFX_ROTATE_ANGLE_MIRROR_Y;
		default :
			return GFX_ROTATE_ANGLE_NULL;
	};
	
	return GFX_ROTATE_ANGLE_NULL;
}

int config_scale_param(GFX_USER_DATA *p_command, HD_GFX_SCALE *p_param, int dma_flush)
{
	int ret;

	if(!p_command || !p_param){
		printf("config_scale_param() invalid p_command(%p) or p_param(%p)\n", p_command, p_param);
		return HD_ERR_NULL_PTR;
	}

	hdal_flow_log_p("    src: w(%d) h(%d) fmt(%x) addr(%x,%x,%x) loff(%d,%d,%d)\n",
						p_param->src_img.dim.w, p_param->src_img.dim.h,
						p_param->src_img.format, p_param->src_img.p_phy_addr[0],
						p_param->src_img.p_phy_addr[1], p_param->src_img.p_phy_addr[2],
						p_param->src_img.lineoffset[0], p_param->src_img.lineoffset[1],
						p_param->src_img.lineoffset[2]);
	hdal_flow_log_p("    dst: w(%d) h(%d) fmt(%x) addr(%x,%x,%x) loff(%d,%d,%d)\n",
						p_param->dst_img.dim.w, p_param->dst_img.dim.h,
						p_param->dst_img.format, p_param->dst_img.p_phy_addr[0],
						p_param->dst_img.p_phy_addr[1], p_param->dst_img.p_phy_addr[2],
						p_param->dst_img.lineoffset[0], p_param->dst_img.lineoffset[1],
						p_param->dst_img.lineoffset[2]);
	hdal_flow_log_p("    source region(%d,%d,%d,%d) destination region(%d,%d,%d,%d) quality(%d)\n",
						p_param->src_region.x, p_param->src_region.y,
						p_param->src_region.w, p_param->src_region.h,
						p_param->dst_region.x, p_param->dst_region.y,
						p_param->dst_region.w, p_param->dst_region.h,
						p_param->quality);

	memset(p_command, 0, sizeof(GFX_USER_DATA));
	p_command->version = GFX_USER_VERSION;
	p_command->cmd     = GFX_USER_CMD_SCALE;

	ret = copy_hd_img_to_gfx(&p_param->src_img, &p_command->data.scale.src_img);
	if(ret){
		printf("config_scale_param() fail to convert src image\n");
		return ret;
	}

	ret = copy_hd_img_to_gfx(&p_param->dst_img, &p_command->data.scale.dst_img);
	if(ret){
		printf("config_scale_param() fail to convert dst image\n");
		return ret;
	}

	p_command->data.scale.src_x   = p_param->src_region.x;
	p_command->data.scale.src_y   = p_param->src_region.y;
	p_command->data.scale.src_w   = p_param->src_region.w;
	p_command->data.scale.src_h   = p_param->src_region.h;
	p_command->data.scale.dst_x   = p_param->dst_region.x;
	p_command->data.scale.dst_y   = p_param->dst_region.y;
	p_command->data.scale.dst_w   = p_param->dst_region.w;
	p_command->data.scale.dst_h   = p_param->dst_region.h;
	p_command->data.scale.flush   = dma_flush;
	p_command->data.scale.method  = convert_scale_method(p_param->quality);
	if(p_command->data.scale.method == GFX_SCALE_METHOD_MAX){
		printf("config_scale_param() quality(%d) is not supported\n", p_param->quality);
		return HD_ERR_NOT_SUPPORT;
	}

	return HD_OK;
}

static int config_line_param(GFX_USER_DATA *p_command, HD_GFX_DRAW_LINE *p_param, UINT32 flush)
{
	int ret;
	
	if(!p_command || !p_param){
		printf("config_line_param() invalid p_command(%p) or p_param(%p)\n", p_command, p_param);
		return HD_ERR_NULL_PTR;
	}
	
	hdal_flow_log_p("    dst: w(%d) h(%d) fmt(%x) addr(%x,%x,%x) loff(%d,%d,%d)\n",
						p_param->dst_img.dim.w, p_param->dst_img.dim.h,
						p_param->dst_img.format, p_param->dst_img.p_phy_addr[0], 
						p_param->dst_img.p_phy_addr[1], p_param->dst_img.p_phy_addr[2],
						p_param->dst_img.lineoffset[0], p_param->dst_img.lineoffset[1],
						p_param->dst_img.lineoffset[2]);
	hdal_flow_log_p("    color(%x) start(%d,%d) end(%d,%d) thickness(%d)\n", 
						p_param->color, p_param->start.x, p_param->start.y,
						p_param->end.x, p_param->end.y, p_param->thickness);
	
	memset(p_command, 0, sizeof(GFX_USER_DATA));
	p_command->version = GFX_USER_VERSION;
	p_command->cmd     = GFX_USER_CMD_DRAW_LINE;
	
	ret = copy_hd_img_to_gfx(&p_param->dst_img, &p_command->data.draw_line.dst_img);
	if(ret){
		printf("copy_hd_img_to_gfx() fail to convert dst image\n");
		return ret;
	}
	
	p_command->data.draw_line.color     = p_param->color;
	p_command->data.draw_line.start_x   = p_param->start.x;
	p_command->data.draw_line.start_y   = p_param->start.y;
	p_command->data.draw_line.end_x     = p_param->end.x;
	p_command->data.draw_line.end_y     = p_param->end.y;
	p_command->data.draw_line.thickness = p_param->thickness;
	p_command->data.draw_line.flush     = flush;

	return HD_OK;
}

static int config_rect_param(GFX_USER_DATA *p_command, HD_GFX_DRAW_RECT *p_param, UINT32 flush)
{
	int ret;
	
	if(!p_command || !p_param){
		printf("config_rect_param() invalid p_command(%p) or p_param(%p)\n", p_command, p_param);
		return HD_ERR_NULL_PTR;
	}
	
	hdal_flow_log_p("    dst: w(%d) h(%d) fmt(%x) addr(%x,%x,%x) loff(%d,%d,%d)\n",
						p_param->dst_img.dim.w, p_param->dst_img.dim.h,
						p_param->dst_img.format, p_param->dst_img.p_phy_addr[0], 
						p_param->dst_img.p_phy_addr[1], p_param->dst_img.p_phy_addr[2],
						p_param->dst_img.lineoffset[0], p_param->dst_img.lineoffset[1],
						p_param->dst_img.lineoffset[2]);
	hdal_flow_log_p("    color(%x) rect(%d,%d,%d,%d) type(%d) thickness(%d)\n", 
						p_param->color, p_param->rect.x, p_param->rect.y, p_param->rect.w,
						p_param->rect.h, p_param->type, p_param->thickness);
	
	memset(p_command, 0, sizeof(GFX_USER_DATA));
	p_command->version = GFX_USER_VERSION;
	p_command->cmd     = GFX_USER_CMD_DRAW_RECT;
	
	ret = copy_hd_img_to_gfx(&p_param->dst_img, &p_command->data.draw_rect.dst_img);
	if(ret){
		printf("config_rect_param() fail to convert dst image\n");
		return ret;
	}
	
	p_command->data.draw_rect.color     = p_param->color;
	p_command->data.draw_rect.x         = p_param->rect.x;
	p_command->data.draw_rect.y         = p_param->rect.y;
	p_command->data.draw_rect.w         = p_param->rect.w;
	p_command->data.draw_rect.h         = p_param->rect.h;
	p_command->data.draw_rect.thickness = p_param->thickness;
	p_command->data.draw_rect.flush     = flush;
	
	if(p_param->type == HD_GFX_RECT_SOLID)
		p_command->data.draw_rect.type = GFX_RECT_TYPE_SOLID;
	else if(p_param->type == HD_GFX_RECT_HOLLOW)
		p_command->data.draw_rect.type = GFX_RECT_TYPE_HOLLOW;
	else{
		printf("config_rect_param() : rect type(%d) is not supported\n", p_param->type);
		return HD_ERR_NOT_SUPPORT;
	}
	
	return 0;
}

HD_RESULT hd_gfx_init(VOID)
{
	GFX_USER_DATA command;
	int           ret;
	
	pthread_mutex_lock(&lock);

	//to support multi-thread, if gfx is already init, simply returns HD_OK
	if(gfx_is_init != 0){
		gfx_is_init++;
		ret = HD_OK;
		goto out;
	}
	
	hdal_flow_log_p("hd_gfx_init\n");
	
	if(HD_GFX_MAX_PLANE_NUM != GFX_MAX_PLANE_NUM){
		printf("hd_gfx_init() : HD_GFX_MAX_PLANE_NUM(%d) != GFX_MAX_PLANE_NUM(%d)\n", 
			HD_GFX_MAX_PLANE_NUM, GFX_MAX_PLANE_NUM);
		ret = HD_ERR_NG;
		goto out;
	}
	
	if(HD_GFX_MAX_MULTI_OUT_NUM != GFX_MAX_MULTI_OUT_NUM){
		printf("hd_gfx_init() : HD_GFX_MAX_MULTI_OUT_NUM(%d) != GFX_MAX_MULTI_OUT_NUM(%d)\n", 
			HD_GFX_MAX_MULTI_OUT_NUM, GFX_MAX_MULTI_OUT_NUM);
		ret = HD_ERR_NG;
		goto out;
	}

	memset(&job_handle, 0, sizeof(job_handle));
	memset(&job_list1,  0, sizeof(job_list1));
	memset(&job_list2,  0, sizeof(job_list2));
	memset(&job_list3,  0, sizeof(job_list3));
	memset(&job_list4,  0, sizeof(job_list4));
	
	if(proc_fd < 0){
		ret = open_proc_fd();
		if(ret != HD_OK){
			printf("hd_gfx_init open fd fail\n");
			goto out;
		}
	}
	
	memset(&command, 0, sizeof(GFX_USER_DATA));
	command.version = GFX_USER_VERSION;
	command.cmd     = GFX_USER_CMD_INIT;
	
	ret = send_command(&command);
	if(ret != HD_OK){
		printf("fail to init hd_gfx\n");
		goto out;
	}
	
	gfx_is_init = 1;
	ret = HD_OK;
out:
	pthread_mutex_unlock(&lock);
	
	return ret;
}

HD_RESULT hd_gfx_uninit(VOID)
{
	GFX_USER_DATA command;
	int           ret;
	
	pthread_mutex_lock(&lock);

	//to support multi-thread, if gfx is already init, simply returns HD_OK
	if(gfx_is_init != 1){
		gfx_is_init--;
		ret = HD_OK;
		goto out;
	}
	
	hdal_flow_log_p("hd_gfx_uninit\n");
	
	CHECK_INIT();

	memset(&job_handle, 0, sizeof(job_handle));
	memset(&job_list1,  0, sizeof(job_list1));
	memset(&job_list2,  0, sizeof(job_list2));
	memset(&job_list3,  0, sizeof(job_list3));
	memset(&job_list4,  0, sizeof(job_list4));
	
	memset(&command, 0, sizeof(GFX_USER_DATA));
	command.version = GFX_USER_VERSION;
	command.cmd     = GFX_USER_CMD_UNINIT;
	
	ret = send_command(&command);
	
	if(proc_fd >= 0)
		GFX_CLOSE(proc_fd);

	proc_fd = -1;
	gfx_is_init = 0;
	ret = HD_OK;
out:
	pthread_mutex_unlock(&lock);
	return ret;
}

HD_RESULT hd_gfx_crop_img(HD_GFX_IMG_BUF src_img, HD_IRECT src_region, HD_GFX_IMG_BUF dst_img)
{
	printf("gfx : crop_img is not supported\n");
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT _hd_gfx_copy(HD_GFX_COPY *p_param, UINT32 flush, GFX_GRPH_ENGINE engine)
{
	GFX_USER_DATA    command;
	int              ret;
	
	if(!p_param){
		printf("_hd_gfx_copy() : NULL p_param\n");
		return HD_ERR_INV;
	}
	
	CHECK_INIT();
	
	memset(&command, 0, sizeof(GFX_USER_DATA));
	command.version = GFX_USER_VERSION;
	command.cmd     = GFX_USER_CMD_COPY;

	ret = copy_hd_img_to_gfx(&p_param->src_img, &command.data.copy.src_img);
	if(ret){
		printf("_hd_gfx_copy() fail to convert src image\n");
		return ret;
	}

	ret = copy_hd_img_to_gfx(&p_param->dst_img, &command.data.copy.dst_img);
	if(ret){
		printf("_hd_gfx_copy() fail to convert dst image\n");
		return ret;
	}
	
	command.data.copy.src_x    = p_param->src_region.x;
	command.data.copy.src_y    = p_param->src_region.y;
	command.data.copy.src_w    = p_param->src_region.w;
	command.data.copy.src_h    = p_param->src_region.h;
	command.data.copy.dst_x    = p_param->dst_pos.x;
	command.data.copy.dst_y    = p_param->dst_pos.y;
	command.data.copy.colorkey = p_param->colorkey;
	command.data.copy.alpha    = p_param->alpha;
	command.data.copy.flush    = flush;
	command.data.copy.engine   = engine;
	
	return send_command(&command);
}

HD_RESULT hd_gfx_copy(HD_GFX_COPY *p_param)
{
	hdal_flow_log_p("hd_gfx_copy:\n");

	if(!p_param){
		printf("hd_gfx_copy() : NULL p_param\n");
		return HD_ERR_INV;
	}

	CHECK_INIT();

	hdal_flow_log_p("    src: w(%d) h(%d) fmt(%x) addr(%x,%x,%x) loff(%d,%d,%d)\n",
						p_param->src_img.dim.w, p_param->src_img.dim.h,
						p_param->src_img.format, p_param->src_img.p_phy_addr[0], 
						p_param->src_img.p_phy_addr[1], p_param->src_img.p_phy_addr[2],
						p_param->src_img.lineoffset[0], p_param->src_img.lineoffset[1],
						p_param->src_img.lineoffset[2]);
	hdal_flow_log_p("    dst: w(%d) h(%d) fmt(%x) addr(%x,%x,%x) loff(%d,%d,%d)\n",
						p_param->dst_img.dim.w, p_param->dst_img.dim.h,
						p_param->dst_img.format, p_param->dst_img.p_phy_addr[0], 
						p_param->dst_img.p_phy_addr[1], p_param->dst_img.p_phy_addr[2],
						p_param->dst_img.lineoffset[0], p_param->dst_img.lineoffset[1],
						p_param->dst_img.lineoffset[2]);
	hdal_flow_log_p("    region(%d,%d,%d,%d) pos(%d,%d) colorkey(%x) alpha(%d)\n", 
						p_param->src_region.x, p_param->src_region.y,
						p_param->src_region.w, p_param->src_region.h,
						p_param->dst_pos.x, p_param->dst_pos.y,
						p_param->colorkey, p_param->alpha);

	return _hd_gfx_copy(p_param, 1, GFX_GRPH_ENGINE_1);
}

HD_RESULT hd_gfx_scale(HD_GFX_SCALE *p_param)
{
	GFX_USER_DATA    command;
	int              ret;

	hdal_flow_log_p("hd_gfx_scale:\n");

	if(!p_param){
		printf("hd_gfx_scale() : NULL p_param\n");
		return HD_ERR_INV;
	}

	CHECK_INIT();

	ret = config_scale_param(&command, p_param, 1);
	if(ret){
		printf("hd_gfx_scale() fail to configure scale parameters\n");
		return ret;
	}

	return send_command(&command);
}

HD_RESULT _hd_gfx_rotate(HD_GFX_ROTATE *p_param, UINT32 flush)
{
	GFX_USER_DATA    command;
	int              ret;

	hdal_flow_log_p("_hd_gfx_rotate:\n");

	if(!p_param){
		printf("_hd_gfx_rotate() : NULL p_param\n");
		return HD_ERR_INV;
	}

	CHECK_INIT();

	hdal_flow_log_p("    src: w(%d) h(%d) fmt(%x) addr(%x,%x,%x) loff(%d,%d,%d)\n",
						p_param->src_img.dim.w, p_param->src_img.dim.h,
						p_param->src_img.format, p_param->src_img.p_phy_addr[0],
						p_param->src_img.p_phy_addr[1], p_param->src_img.p_phy_addr[2],
						p_param->src_img.lineoffset[0], p_param->src_img.lineoffset[1],
						p_param->src_img.lineoffset[2]);
	hdal_flow_log_p("    dst: w(%d) h(%d) fmt(%x) addr(%x,%x,%x) loff(%d,%d,%d)\n",
						p_param->dst_img.dim.w, p_param->dst_img.dim.h,
						p_param->dst_img.format, p_param->dst_img.p_phy_addr[0],
						p_param->dst_img.p_phy_addr[1], p_param->dst_img.p_phy_addr[2],
						p_param->dst_img.lineoffset[0], p_param->dst_img.lineoffset[1],
						p_param->dst_img.lineoffset[2]);
	hdal_flow_log_p("    region(%d,%d,%d,%d) pos(%d,%d) angle(%x) flush(%ld)\n",
						p_param->src_region.x, p_param->src_region.y,
						p_param->src_region.w, p_param->src_region.h,
						p_param->dst_pos.x, p_param->dst_pos.y,
						p_param->angle,flush);

	memset(&command, 0, sizeof(GFX_USER_DATA));
	command.version = GFX_USER_VERSION;
	command.cmd     = GFX_USER_CMD_ROTATE;

	ret = copy_hd_img_to_gfx(&p_param->src_img, &command.data.rotate.src_img);
	if(ret){
		printf("hd_gfx_rotate() fail to convert src image\n");
		return ret;
	}

	ret = copy_hd_img_to_gfx(&p_param->dst_img, &command.data.rotate.dst_img);
	if(ret){
		printf("hd_gfx_rotate() fail to convert dst image\n");
		return ret;
	}

	command.data.rotate.src_x = p_param->src_region.x;
	command.data.rotate.src_y = p_param->src_region.y;
	command.data.rotate.src_w = p_param->src_region.w;
	command.data.rotate.src_h = p_param->src_region.h;
	command.data.rotate.dst_x = p_param->dst_pos.x;
	command.data.rotate.dst_y = p_param->dst_pos.y;
	command.data.rotate.angle = convert_rotation_angle(p_param->angle);
	if(command.data.rotate.angle == GFX_ROTATE_ANGLE_NULL){
		printf("hd_gfx_rotate() rotation angle(%x) is not supported\n", p_param->angle);
		return HD_ERR_NOT_SUPPORT;
	}
	command.data.rotate.flush = flush;

	return send_command(&command);
}

HD_RESULT hd_gfx_rotate(HD_GFX_ROTATE *p_param)
{
	hdal_flow_log_p("hd_gfx_rotate:\n");
	
	if(!p_param){
		printf("hd_gfx_rotate() : NULL p_param\n");
		return HD_ERR_INV;
	}
	
	CHECK_INIT();
	
	hdal_flow_log_p("    src: w(%d) h(%d) fmt(%x) addr(%x,%x,%x) loff(%d,%d,%d)\n",
						p_param->src_img.dim.w, p_param->src_img.dim.h,
						p_param->src_img.format, p_param->src_img.p_phy_addr[0], 
						p_param->src_img.p_phy_addr[1], p_param->src_img.p_phy_addr[2],
						p_param->src_img.lineoffset[0], p_param->src_img.lineoffset[1],
						p_param->src_img.lineoffset[2]);
	hdal_flow_log_p("    dst: w(%d) h(%d) fmt(%x) addr(%x,%x,%x) loff(%d,%d,%d)\n",
						p_param->dst_img.dim.w, p_param->dst_img.dim.h,
						p_param->dst_img.format, p_param->dst_img.p_phy_addr[0], 
						p_param->dst_img.p_phy_addr[1], p_param->dst_img.p_phy_addr[2],
						p_param->dst_img.lineoffset[0], p_param->dst_img.lineoffset[1],
						p_param->dst_img.lineoffset[2]);
	hdal_flow_log_p("    region(%d,%d,%d,%d) pos(%d,%d) angle(%x)\n", 
						p_param->src_region.x, p_param->src_region.y,
						p_param->src_region.w, p_param->src_region.h,
						p_param->dst_pos.x, p_param->dst_pos.y,
						p_param->angle);

	return _hd_gfx_rotate(p_param, 1);
}

HD_RESULT hd_gfx_color_transform(HD_GFX_COLOR_TRANSFORM *p_param)
{
	GFX_USER_DATA    command;
	int              ret, i;
	
	hdal_flow_log_p("hd_gfx_color_transform:\n");
	
	if(HD_GFX_MAX_PLANE_NUM != GFX_MAX_PLANE_NUM){
		printf("hd_gfx_color_transform() : HD_GFX_MAX_PLANE_NUM(%d) != GFX_MAX_PLANE_NUM(%d)\n", 
			HD_GFX_MAX_PLANE_NUM, GFX_MAX_PLANE_NUM);
		return HD_ERR_NG;
	}
	
	if(!p_param){
		printf("hd_gfx_color_transform() : NULL p_param\n");
		return HD_ERR_INV;
	}
	
	CHECK_INIT();
	
	hdal_flow_log_p("    src: w(%d) h(%d) fmt(%x) addr(%x,%x,%x) loff(%d,%d,%d)\n",
						p_param->src_img.dim.w, p_param->src_img.dim.h,
						p_param->src_img.format, p_param->src_img.p_phy_addr[0], 
						p_param->src_img.p_phy_addr[1], p_param->src_img.p_phy_addr[2],
						p_param->src_img.lineoffset[0], p_param->src_img.lineoffset[1],
						p_param->src_img.lineoffset[2]);
	hdal_flow_log_p("    dst: w(%d) h(%d) fmt(%x) addr(%x,%x,%x) loff(%d,%d,%d)\n",
						p_param->dst_img.dim.w, p_param->dst_img.dim.h,
						p_param->dst_img.format, p_param->dst_img.p_phy_addr[0], 
						p_param->dst_img.p_phy_addr[1], p_param->dst_img.p_phy_addr[2],
						p_param->dst_img.lineoffset[0], p_param->dst_img.lineoffset[1],
						p_param->dst_img.lineoffset[2]);
	hdal_flow_log_p("    tmp buf(%x,%d) table(%x,%x,%x)\n", 
						p_param->p_tmp_buf, p_param->tmp_buf_size,
						p_param->p_lookup_table[0], p_param->p_lookup_table[1],
						p_param->p_lookup_table[2]);
	
	memset(&command, 0, sizeof(GFX_USER_DATA));
	command.version = GFX_USER_VERSION;
	command.cmd     = GFX_USER_CMD_COLOR_TRANSFORM;

	ret = copy_hd_img_to_gfx(&p_param->src_img, &command.data.color_transform.src_img);
	if(ret){
		printf("hd_gfx_color_transform() fail to convert src image\n");
		return ret;
	}

	ret = copy_hd_img_to_gfx(&p_param->dst_img, &command.data.color_transform.dst_img);
	if(ret){
		printf("hd_gfx_color_transform() fail to convert dst image\n");
		return ret;
	}
	
	command.data.color_transform.p_tmp_buf = p_param->p_tmp_buf;
	command.data.color_transform.tmp_buf_size = p_param->tmp_buf_size;
	for(i = 0 ; i < HD_GFX_MAX_PLANE_NUM ; ++i)
		command.data.color_transform.p_lookup_table[i] = p_param->p_lookup_table[i];
	
	return send_command(&command);
}

HD_RESULT _hd_gfx_draw_line(HD_GFX_DRAW_LINE *p_param, UINT32 flush)
{
	GFX_USER_DATA command;
	int           ret;

	hdal_flow_log_p("hd_gfx_draw_line:\n");

	CHECK_INIT();

	ret = config_line_param(&command, p_param, flush);
	if(ret){
		printf("hd_gfx_draw_line() fail to configure line parameters\n");
		return ret;
	}

	return send_command(&command);
}

HD_RESULT hd_gfx_draw_line(HD_GFX_DRAW_LINE *p_param)
{
	return _hd_gfx_draw_line(p_param, 1);
}

HD_RESULT _hd_gfx_draw_rect(HD_GFX_DRAW_RECT *p_param, UINT32 flush)
{
	GFX_USER_DATA command;
	int           ret;
	
	hdal_flow_log_p("hd_gfx_draw_rect:\n");
	
	CHECK_INIT();
	
	ret = config_rect_param(&command, p_param, flush);
	if(ret){
		printf("hd_gfx_draw_rect() fail to configure rect parameters\n");
		return ret;
	}
	
	return send_command(&command);
}

HD_RESULT hd_gfx_draw_rect(HD_GFX_DRAW_RECT *p_param)
{	
	return _hd_gfx_draw_rect(p_param, 1);
}

HD_RESULT hd_gfx_begin_job(HD_GFX_HANDLE *p_handle)
{
	int i, ret;
	
	hdal_flow_log_p("hd_gfx_begin_job:\n");
	
	CHECK_INIT();
	
	if(!p_handle){
		printf("hd_gfx_begin_job() p_handle is NULL\n");
		return HD_ERR_INV;
	}
	
	pthread_mutex_lock(&lock);
	
	for(i = 0 ; i < MAX_JOB_LIST ; ++i)
		if(job_handle[i] == 0){
			job_handle[i] = (i + 1);
			*p_handle = job_handle[i];
			ret = HD_OK;
			hdal_flow_log_p("    handle(%d)\n", *p_handle);
			goto out;
		}
		
	printf("hd_gfx_begin_job() all handles are in use\n");
	ret = HD_ERR_RESOURCE;
	
out:
	pthread_mutex_unlock(&lock);
	
	return ret;
}

HD_RESULT hd_gfx_end_job(HD_GFX_HANDLE handle)
{
	int              ret;
	GFX_USER_DATA    *job_list;
	
	CHECK_INIT();
	
	hdal_flow_log_p("hd_gfx_end_job(%x)\n", handle);
	
	if(handle == 0 || handle > MAX_JOB_LIST){
		printf("hd_gfx_end_job() invalid handle(%d). max is %d\n", handle, MAX_JOB_LIST);
		return HD_ERR_INV;
	}
	
	pthread_mutex_lock(&lock);
	
	if(handle == 1)
		job_list = job_list1;
	else if(handle == 2)
		job_list = job_list2;
	else if(handle == 3)
		job_list = job_list3;
	else if(handle == 4)
		job_list = job_list4;
		
	memset(job_list, 0, sizeof(GFX_USER_DATA));
	job_list[0].version = GFX_USER_VERSION;
	job_list[0].cmd     = GFX_USER_CMD_JOB_LIST;
	ret = send_command(job_list);
	if(ret != HD_OK)
		printf("hd_gfx_end_job() handle(%d) job list fail\n", handle);
	
	job_handle[handle - 1] = 0;
	memset(job_list,  0, sizeof(job_list1));
	
	pthread_mutex_unlock(&lock);
	
	return ret;
}

HD_RESULT hd_gfx_cancel_job(HD_GFX_HANDLE handle)
{
	hdal_flow_log_p("hd_gfx_cancel_job(%x)\n", handle);
	
	CHECK_INIT();
	
	if(handle == 0 || handle > MAX_JOB_LIST){
		printf("hd_gfx_cancel_job() invalid handle(%d). max is %d\n", handle, MAX_JOB_LIST);
		return HD_ERR_INV;
	}
	
	pthread_mutex_lock(&lock);
	
	job_handle[handle - 1] = 0;
	
	if(handle == 1)
		memset(&job_list1,  0, sizeof(job_list1));
	else if(handle == 2)
		memset(&job_list2,  0, sizeof(job_list2));
	else if(handle == 3)
		memset(&job_list3,  0, sizeof(job_list3));
	else if(handle == 4)
		memset(&job_list4,  0, sizeof(job_list4));
		
	pthread_mutex_unlock(&lock);
	
	return HD_OK;
}

HD_RESULT hd_gfx_add_draw_line_list(HD_GFX_HANDLE handle, HD_GFX_DRAW_LINE param[], UINT32 num)
{
	int              i, idx;
	int              ret;
	GFX_USER_DATA    *job_list;
	
	hdal_flow_log_p("hd_gfx_add_draw_line_list(%x) : %d lines\n", handle, num);
	
	CHECK_INIT();
	
	if(handle == 0 || handle > MAX_JOB_LIST){
		printf("hd_gfx_add_draw_line_list() invalid handle(%d). max is %d\n", handle, MAX_JOB_LIST);
		return HD_ERR_INV;
	}
	
	if(!param || !num){
		printf("hd_gfx_add_draw_line_list() param(%p) or num(%d) is NULL\n", param, num);
		return HD_ERR_INV;
	}
	
	if(num > MAX_JOB_IN_A_LIST){
		printf("hd_gfx_add_draw_line_list() num can't be %d. max is %d\n", num, MAX_JOB_IN_A_LIST);
		return HD_ERR_RESOURCE;
	}
	
	pthread_mutex_lock(&lock);
	
	if(handle == 1)
		job_list = job_list1;
	else if(handle == 2)
		job_list = job_list2;
	else if(handle == 3)
		job_list = job_list3;
	else if(handle == 4)
		job_list = job_list4;
		
	for(i = 1 ; i <= MAX_JOB_IN_A_LIST ; ++i)
		if(job_list[i].cmd != GFX_USER_CMD_DRAW_LINE && job_list[i].cmd != GFX_USER_CMD_DRAW_RECT)
			break;

	if((i + num - 1) > MAX_JOB_IN_A_LIST){
		printf("hd_gfx_add_draw_line_list() can't add %d jobs\n", num);
		printf("hd_gfx_add_draw_line_list() %d jobs had been added. max is %d\n", i - 1, MAX_JOB_IN_A_LIST);
		ret = HD_ERR_RESOURCE;
		goto out;
	}
	
	idx = i;
	for(i = 0 ; i < (int)num ; ++i){
		ret = config_line_param(&job_list[idx + i], &param[i], 1);
		if(ret != HD_OK){
			printf("hd_gfx_add_draw_line_list() fail to add %dth job\n", i);
			break;
		}
	}
	
	if(ret != HD_OK)
		for(i = 0 ; i < (int)num ; ++i)
			//coverity[overrun-local]
			memset(&job_list[idx + i], 0, sizeof(GFX_USER_DATA));
out:
	pthread_mutex_unlock(&lock);
	
	return ret;
}

HD_RESULT hd_gfx_add_draw_rect_list(HD_GFX_HANDLE handle, HD_GFX_DRAW_RECT param[], UINT32 num)
{
	int              i, idx;
	int              ret;
	GFX_USER_DATA    *job_list;
	
	hdal_flow_log_p("hd_gfx_add_draw_rect_list(%x) : %d rectangles\n", handle, num);
	
	CHECK_INIT();
	
	if(handle == 0 || handle > MAX_JOB_LIST){
		printf("hd_gfx_add_draw_rect_list() invalid handle(%d). max is %d\n", handle, MAX_JOB_LIST);
		return HD_ERR_INV;
	}
	
	if(!param || !num){
		printf("hd_gfx_add_draw_rect_list() param(%p) or num(%d) is NULL\n", param, num);
		return HD_ERR_INV;
	}
	
	if(num > MAX_JOB_IN_A_LIST){
		printf("hd_gfx_add_draw_rect_list() num can't be %d. max is %d\n", num, MAX_JOB_IN_A_LIST);
		return HD_ERR_RESOURCE;
	}
	
	pthread_mutex_lock(&lock);
	
	if(handle == 1)
		job_list = job_list1;
	else if(handle == 2)
		job_list = job_list2;
	else if(handle == 3)
		job_list = job_list3;
	else if(handle == 4)
		job_list = job_list4;
		
	for(i = 1 ; i <= MAX_JOB_IN_A_LIST ; ++i)
		if(job_list[i].cmd != GFX_USER_CMD_DRAW_LINE && job_list[i].cmd != GFX_USER_CMD_DRAW_RECT)
			break;

	if((i + num - 1) > MAX_JOB_IN_A_LIST){
		printf("hd_gfx_add_draw_rect_list() can't add %d jobs\n", num);
		printf("hd_gfx_add_draw_rect_list() %d jobs had been added. max is %d\n", i - 1, MAX_JOB_IN_A_LIST);
		ret = HD_ERR_RESOURCE;
		goto out;
	}
	
	idx = i;
	for(i = 0 ; i < (int)num ; ++i){
		ret = config_rect_param(&job_list[idx + i], &param[i], 1);
		if(ret != HD_OK){
			printf("hd_gfx_add_draw_rect_list() fail to add %dth job\n", i);
			break;
		}
	}
	
	if(ret != HD_OK)
		for(i = 0 ; i < (int)num ; ++i)
			//coverity[overrun-local]
			memset(&job_list[idx + i], 0, sizeof(GFX_USER_DATA));
	
out:
	pthread_mutex_unlock(&lock);
	
	return ret;
}

void *hd_gfx_memcpy(UINT32 dest, const UINT32 src, size_t n)
{
	GFX_USER_DATA    command;
	int              ret;
	
	hdal_flow_log_p("hd_gfx_memcpy: dst(%x) src(%x) len(%d)\n", dest, src, n);
	
	if(!dest || !src || !n){
		printf("hd_gfx_memcpy() : invalid dest(%x) src(%x) n(%d)\n", dest, src, (int)n);
		return NULL;
	}
	
	if(!gfx_is_init){
		printf("hd_gfx is not init\n");
		return NULL;
	}
	
	memset(&command, 0, sizeof(GFX_USER_DATA));
	command.version = GFX_USER_VERSION;
	command.cmd     = GFX_USER_CMD_DMA_COPY;
	
	command.data.dma_copy.p_src  = src;
	command.data.dma_copy.p_dst  = dest;
	command.data.dma_copy.length = n;
	
	ret = send_command(&command);
	if(ret == HD_OK)
		return (void*)dest;
	else
		return NULL;
}

HD_RESULT hd_gfx_arithmetic(HD_GFX_ARITHMETIC *p_param)
{
	GFX_USER_DATA    command;
	
	if(!p_param){
		printf("hd_gfx_arithmetic() : NULL p_param\n");
		return HD_ERR_INV;
	}
	
	CHECK_INIT();
	
	hdal_flow_log_p("hd_gfx_arithmetic: op1(%x) op2(%x) out(%x) size(%d) opt(%d) bits(%d)\n", 
						p_param->p_op1, p_param->p_op2, p_param->p_out, 
						p_param->size, p_param->operation, p_param->bits);
	
	if(!p_param->p_op1 || !p_param->p_op2 || !p_param->p_out || !p_param->size){
		printf("hd_gfx_arithmetic() : invalid p_op1(%x) p_op2(%x) p_out(%x) size(%d)\n", 
			p_param->p_op1, p_param->p_op2, p_param->p_out, p_param->size);
		return HD_ERR_INV;
	}
	
	memset(&command, 0, sizeof(GFX_USER_DATA));
	command.version = GFX_USER_VERSION;
	command.cmd     = GFX_USER_CMD_ARITHMETIC;
	
	command.data.arithmetic.p_op1     = p_param->p_op1;
	command.data.arithmetic.p_op2     = p_param->p_op2;
	command.data.arithmetic.p_out     = p_param->p_out;
	command.data.arithmetic.size      = p_param->size;
	
	if(p_param->operation == HD_GFX_ARITH_OP_PLUS)
		command.data.arithmetic.operation = GFX_ARITH_OP_PLUS;
	else if(p_param->operation == HD_GFX_ARITH_OP_MINUS)
		command.data.arithmetic.operation = GFX_ARITH_OP_MINUS;
	else if(p_param->operation == HD_GFX_ARITH_OP_MULTIPLY)
		command.data.arithmetic.operation = GFX_ARITH_OP_MULTIPLY;
	else{
		printf("hd_gfx_arithmetic() operation(%d) is not supported\n", p_param->operation);
		return HD_ERR_NOT_SUPPORT;
	}
	
	if(p_param->bits == 8)
		command.data.arithmetic.bits = GFX_ARITH_BIT_8;
	else if(p_param->bits == 16)
		command.data.arithmetic.bits = GFX_ARITH_BIT_16;
	else{
		printf("hd_gfx_arithmetic() bits(%d) is not supported\n", p_param->bits);
		return HD_ERR_NOT_SUPPORT;
	}
	
	return send_command(&command);
}

HD_RESULT hd_gfx_affine(HD_GFX_AFFINE *p_param){
	int              i;
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
		command.data.affine.fCoeffA[i]     = p_param->coeff_a;
		command.data.affine.fCoeffB[i]     = p_param->coeff_b;
		command.data.affine.fCoeffC[i]     = p_param->coeff_c;
		command.data.affine.fCoeffD[i]     = p_param->coeff_d;
		command.data.affine.fCoeffE[i]     = p_param->coeff_e;
		command.data.affine.fCoeffF[i]     = p_param->coeff_f;
	}
	
	if(p_param->src_img.format == HD_VIDEO_PXLFMT_YUV420){
		command.data.affine.plane_num  = 2;
		command.data.affine.uvpack[1]  = 1;
		command.data.affine.fCoeffC[1] = p_param->coeff_c / 2;
		command.data.affine.fCoeffF[1] = p_param->coeff_f / 2;
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
	
	command.version       = GFX_USER_VERSION;
	command.cmd           = GFX_USER_CMD_AFFINE;
	command.data.affine.w = p_param->src_img.dim.w;
	command.data.affine.h = p_param->src_img.dim.h;	
	
	return send_command(&command);
}

