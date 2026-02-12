#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hdal.h"
#define HD_MODULE_NAME HD_OSG
#include "hd_int.h"
#include "videosprite/hd_videosprite.h"
#include "hd_videoprocess.h"
#include "hd_logger_p.h"
#if defined(__LINUX)
#include <sys/mman.h>
#endif


static int proc_fd = -1;

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#define VDS_OPEN(...) 0
#define VDS_IOCTL nvt_vds_ioctl
#define VDS_CLOSE(...)
#endif
#if defined(__LINUX)
#define VDS_OPEN  open
#define VDS_IOCTL ioctl
#define VDS_CLOSE close
#endif

static int open_proc_fd(void)
{
	if(proc_fd >= 0){
		printf("videosprite : /proc/nvt_videosprite is already opened. no need to open again\r\n");
		return 0;
	}

	proc_fd = VDS_OPEN("/proc/nvt_videosprite" , O_RDWR, S_IRUSR);
	if(proc_fd < 0){
		printf("videosprite : fail to open /proc/nvt_videosprite\r\n");
		return HD_ERR_SYS;
	}

	return 0;
}

static HD_RESULT translate_error_code(ISF_RV code)
{
	switch(code){
		case ISF_OK:
			return HD_OK;
		case ISF_ERR_INVALID_VALUE:
			return HD_ERR_INV;
		case ISF_ERR_INVALID_PORT_ID:
			return HD_ERR_PATH;
		case ISF_ERR_NEW_FAIL:
			return HD_ERR_NOBUF;
		case ISF_ERR_NOT_SUPPORT:
			return HD_ERR_NOT_SUPPORT;
		case ISF_ERR_COUNT_OVERFLOW:
			return HD_ERR_NOMEM;
		case ISF_ERR_COPY_FROM_USER:
		case ISF_ERR_COPY_TO_USER:
			return HD_ERR_SYS;
		case ISF_ERR_NOT_AVAIL:
			return HD_ERR_NOT_AVAIL;
		default :
			return HD_ERR_NG;
	}
}

static int send_command(VDS_USER_DATA *command)
{
	int ret = 0;
	
	if(!command){
		printf("videosprite : command is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}

	if(proc_fd < 0){
		ret = open_proc_fd();
		if(ret){
			printf("videosprite : open_proc_fd() fail\r\n");
			return ret;
		}
	}

	ret = translate_error_code(VDS_IOCTL(proc_fd , command->cmd, command));
	if(ret != HD_OK)
		hdal_flow_log_p("        fail(%x)\n", ret);

	return ret;
}

static int vds_open(VDS_PHASE phase, UINT32 rgn, UINT32 vid)
{
	VDS_USER_DATA command;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", phase, rgn, vid);

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_OPEN;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	return send_command(&command);
}

static int vds_start(VDS_PHASE phase, UINT32 rgn, UINT32 vid)
{
	VDS_USER_DATA command;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", phase, rgn, vid);

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_START;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	return send_command(&command);
}

static int vds_stop(VDS_PHASE phase, UINT32 rgn, UINT32 vid)
{
	VDS_USER_DATA command;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", phase, rgn, vid);

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_STOP;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	return send_command(&command);
}

static int vds_set_stamp_buf(VDS_PHASE phase, UINT32 rgn, UINT32 vid, HD_OSG_STAMP_BUF *p_buf)
{
	VDS_USER_DATA command;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", phase, rgn, vid);

	if(!p_buf){
		printf("vds_set_stamp_buf() : NULL p_buf\n");
		hdal_flow_log_p("        NULL p_buf\n");
		return HD_ERR_NULL_PTR;
	}

	hdal_flow_log_p("        type(%d) size(%d) addr(%x)\n",
						p_buf->type, p_buf->size, p_buf->p_addr);

	if(!p_buf->size){
		printf("vds_set_stamp_buf() : buf size is 0\n");
		hdal_flow_log_p("        buf size is 0\n");
		return HD_ERR_INV;
	}

	if(!p_buf->p_addr){
		printf("vds_set_stamp_buf() : buf addr is 0\n");
		hdal_flow_log_p("        buf addr is 0\n");
		return HD_ERR_INV;
	}

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_SET_STAMP_BUF;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	if(p_buf->type == HD_OSG_BUF_TYPE_SINGLE)
		command.data.buf.type = VDS_BUF_TYPE_SINGE;
	else if(p_buf->type == HD_OSG_BUF_TYPE_PING_PONG)
		command.data.buf.type = VDS_BUF_TYPE_PING_PONG;
	else{
		printf("vds_set_stamp_buf() : buf type(%d) is not supported\n", p_buf->type);
		hdal_flow_log_p("        buf type(%d) is not supported\n", p_buf->type);
		return HD_ERR_NOT_SUPPORT;
	}

	command.data.buf.size = p_buf->size;
	command.data.buf.p_addr = p_buf->p_addr;

	return send_command(&command);
}

static int vds_get_stamp_buf(VDS_PHASE phase, UINT32 rgn, UINT32 vid, HD_OSG_STAMP_BUF *p_buf)
{
	VDS_USER_DATA    command;
	int              ret;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", phase, rgn, vid);

	if(!p_buf){
		printf("vds_get_stamp_buf() : NULL p_buf\n");
		hdal_flow_log_p("        NULL p_buf\n");
		return HD_ERR_NULL_PTR;
	}

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_GET_STAMP_BUF;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	ret = send_command(&command);
	if(ret){
		printf("VDS_USER_CMD_GET_STAMP_BUF(%d,%d,%d) fail\r\n", phase, rgn, vid);
		return ret;
	}

	if(command.data.buf.type == VDS_BUF_TYPE_SINGE)
		p_buf->type = HD_OSG_BUF_TYPE_SINGLE;
	else if(command.data.buf.type == VDS_BUF_TYPE_PING_PONG)
		p_buf->type = HD_OSG_BUF_TYPE_PING_PONG;
	else{
		printf("vds_get_stamp_buf() : buf type(%d) is not supported\n", command.data.buf.type);
		hdal_flow_log_p("        buf type(%d) is not supported\n", command.data.buf.type);
		return HD_ERR_NOT_SUPPORT;
	}

	p_buf->size = command.data.buf.size;
	p_buf->p_addr = command.data.buf.p_addr;

	hdal_flow_log_p("        type(%d) size(%d) addr(%x)\n",
						p_buf->type, p_buf->size, p_buf->p_addr);

	return HD_OK;
}

static int vds_set_stamp_img(VDS_PHASE phase, UINT32 rgn, UINT32 vid, HD_OSG_STAMP_IMG *p_img)
{
	VDS_USER_DATA command;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", phase, rgn, vid);

	if(!p_img){
		printf("vds_set_stamp_img() : NULL p_img\n");
		hdal_flow_log_p("        NULL p_img\n");
		return HD_ERR_NULL_PTR;
	}

	hdal_flow_log_p("        w(%d) h(%d) fmt(%x) addr(%x)\n",
						p_img->dim.w, p_img->dim.h, p_img->fmt, p_img->p_addr);

	if(!p_img->dim.w || !p_img->dim.h){
		printf("vds_set_stamp_img() : img w(%d)/h(%d) is 0\n", p_img->dim.w, p_img->dim.h);
		hdal_flow_log_p("        w(%d)/h(%d) is 0\n", p_img->dim.w, p_img->dim.h);
		return HD_ERR_INV;
	}

	if(!p_img->p_addr){
		printf("vds_set_stamp_img() : image addr is 0\n");
		hdal_flow_log_p("        image addr is 0\n");
		return HD_ERR_INV;
	}

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_SET_STAMP_IMG;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	if(p_img->fmt == HD_VIDEO_PXLFMT_RGB565)
		command.data.img.fmt = VDS_IMG_FMT_PICTURE_RGB565;
	else if(p_img->fmt == HD_VIDEO_PXLFMT_ARGB1555)
		command.data.img.fmt = VDS_IMG_FMT_PICTURE_ARGB1555;
	else if(p_img->fmt == HD_VIDEO_PXLFMT_ARGB4444)
		command.data.img.fmt = VDS_IMG_FMT_PICTURE_ARGB4444;
	else if(p_img->fmt == HD_VIDEO_PXLFMT_ARGB8888)
		command.data.img.fmt = VDS_IMG_FMT_PICTURE_ARGB8888;
	else if(p_img->fmt == HD_VIDEO_PXLFMT_I1)
		command.data.img.fmt = VDS_IMG_FMT_PICTURE_PALETTE1;
	else if(p_img->fmt == HD_VIDEO_PXLFMT_I2)
		command.data.img.fmt = VDS_IMG_FMT_PICTURE_PALETTE2;
	else if(p_img->fmt == HD_VIDEO_PXLFMT_I4)
		command.data.img.fmt = VDS_IMG_FMT_PICTURE_PALETTE4;
	else{
		printf("vds_set_stamp_img() : img fmt(%d) is not supported\n", p_img->fmt);
		hdal_flow_log_p("        img fmt(%d) is not supported\n", p_img->fmt);
		return HD_ERR_NOT_SUPPORT;
	}

	command.data.img.w      = p_img->dim.w;
	command.data.img.h      = p_img->dim.h;
	command.data.img.p_addr = p_img->p_addr;

	return send_command(&command);
}

static int vds_get_stamp_img(VDS_PHASE phase, UINT32 rgn, UINT32 vid, HD_OSG_STAMP_IMG *p_img)
{
	VDS_USER_DATA    command;
	int              ret;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", phase, rgn, vid);

	if(!p_img){
		printf("vds_get_stamp_img() : NULL p_img\n");
		hdal_flow_log_p("        NULL p_attr\n");
		return HD_ERR_NULL_PTR;
	}

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_GET_STAMP_IMG;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	if(p_img->copy == HD_OSG_COPY_HW_CPU)
		command.data.img.copy = VDS_COPY_HW_CPU;
	else if(p_img->copy == HD_OSG_COPY_HW_DMA)
		command.data.img.copy = VDS_COPY_HW_DMA;
	else
		command.data.img.copy = VDS_COPY_HW_NULL;

	ret = send_command(&command);
	if(ret){
		printf("VDS_USER_CMD_GET_STAMP_IMG(%d,%d,%d) fail\r\n", phase, rgn, vid);
		return ret;
	}

	if(command.data.img.fmt == VDS_IMG_FMT_PICTURE_RGB565)
		p_img->fmt = HD_VIDEO_PXLFMT_RGB565;
	else if(command.data.img.fmt == VDS_IMG_FMT_PICTURE_ARGB1555)
		p_img->fmt = HD_VIDEO_PXLFMT_ARGB1555;
	else if(command.data.img.fmt == VDS_IMG_FMT_PICTURE_ARGB4444)
		p_img->fmt = HD_VIDEO_PXLFMT_ARGB4444;
	else if(command.data.img.fmt == VDS_IMG_FMT_PICTURE_ARGB8888)
		p_img->fmt = HD_VIDEO_PXLFMT_ARGB8888;
	else if(command.data.img.fmt == VDS_IMG_FMT_PICTURE_PALETTE1)
		p_img->fmt = HD_VIDEO_PXLFMT_I1;
	else if(command.data.img.fmt == VDS_IMG_FMT_PICTURE_PALETTE2)
		p_img->fmt = HD_VIDEO_PXLFMT_I2;
	else if(command.data.img.fmt == VDS_IMG_FMT_PICTURE_PALETTE4)
		p_img->fmt = HD_VIDEO_PXLFMT_I4;
	else
		p_img->fmt = HD_VIDEO_PXLFMT_NONE;

	if(command.data.img.copy == VDS_COPY_HW_CPU)
		p_img->copy = HD_OSG_COPY_HW_CPU;
	else if(command.data.img.copy == VDS_COPY_HW_DMA)
		p_img->copy = HD_OSG_COPY_HW_DMA;
	else
		p_img->copy = HD_OSG_COPY_HW_NULL;

	p_img->dim.w  = command.data.img.w;
	p_img->dim.h  = command.data.img.h;
	p_img->p_addr = command.data.img.p_addr;

	hdal_flow_log_p("        w(%d) h(%d) fmt(%x) addr(%x) copy(%d)\n",
						p_img->dim.w, p_img->dim.h, p_img->fmt,
						p_img->p_addr, command.data.img.copy);

	return HD_OK;
}

static int set_stamp_qp(VDS_PHASE phase, UINT32 rgn, UINT32 vid, _VDS_QP *p_attr)
{
	VDS_USER_DATA command;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", (int)phase, (int)rgn, (int)vid);

	if(!p_attr){
		printf("set_stamp_qp() : NULL p_attr\n");
		hdal_flow_log_p("        NULL p_attr\n");
		return HD_ERR_NULL_PTR;
	}

	hdal_flow_log_p("        lpm_mode(%d) tnr_mode(%d) fro_mode(%d)\n",
						p_attr->data.lpm_mode, p_attr->data.tnr_mode, p_attr->data.fro_mode);

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_SET_QP;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	command.data.qp.lpm_mode = p_attr->data.lpm_mode;
	command.data.qp.tnr_mode = p_attr->data.tnr_mode;
	command.data.qp.fro_mode = p_attr->data.fro_mode;

	return send_command(&command);
}

static int set_stamp_gcac(VDS_PHASE phase, UINT32 rgn, UINT32 vid, _VDS_COLOR_INVERT *p_attr)
{
	VDS_USER_DATA command;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", (int)phase, (int)rgn, (int)vid);

	if(!p_attr){
		printf("set_stamp_gcac() : NULL p_attr\n");
		hdal_flow_log_p("        NULL p_attr\n");
		return HD_ERR_NULL_PTR;
	}

	hdal_flow_log_p("        blk_num(%d) org_color_level(%d) inv_color_level(%d) nor_diff_th(%d)\n",
						p_attr->data.blk_num, p_attr->data.org_color_level, p_attr->data.inv_color_level, p_attr->data.nor_diff_th);
	hdal_flow_log_p("        inv_diff_th(%d) sta_only_mode(%d) full_eval_mode(%d) eval_lum_targ(%d)\n",
						p_attr->data.inv_diff_th, p_attr->data.sta_only_mode, p_attr->data.full_eval_mode, p_attr->data.eval_lum_targ);

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_SET_COLOR_INVERT;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	command.data.color_invert.blk_num         = p_attr->data.blk_num;
	command.data.color_invert.org_color_level = p_attr->data.org_color_level;
	command.data.color_invert.inv_color_level = p_attr->data.inv_color_level;
	command.data.color_invert.nor_diff_th     = p_attr->data.nor_diff_th;
	command.data.color_invert.inv_diff_th     = p_attr->data.inv_diff_th;
	command.data.color_invert.sta_only_mode   = p_attr->data.sta_only_mode;
	command.data.color_invert.full_eval_mode  = p_attr->data.full_eval_mode;
	command.data.color_invert.eval_lum_targ   = p_attr->data.eval_lum_targ;

	return send_command(&command);
}

typedef struct _VENDOR_VIDEOENC_OSG_STAMP_ATTR {
	UINT32            magic1;
	UINT32            magic2;
	HD_OSG_STAMP_ATTR attr;
	UINT32            bg_alpha;
} VENDOR_VIDEOENC_OSG_STAMP_ATTR;

static int vds_set_stamp_attr(VDS_PHASE phase, UINT32 rgn, UINT32 vid, HD_OSG_STAMP_ATTR *p_attr)
{
	VDS_USER_DATA command;
	UINT32        bg_alpha = 0; 

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", phase, rgn, vid);

	if(!p_attr){
		printf("_vds_set_stamp_attr() : NULL p_attr\n");
		hdal_flow_log_p("        NULL p_attr\n");
		return HD_ERR_NULL_PTR;
	}

	//handle vendor API
	{
		unsigned int *magic = (unsigned int*)p_attr;
		if(magic[0] == 0xABCDEFAB){
			if(magic[1] == 1)
				return set_stamp_qp(phase, rgn, vid, (_VDS_QP *)p_attr);
			else if(magic[1] == 2)
				return set_stamp_gcac(phase, rgn, vid, (_VDS_COLOR_INVERT *)p_attr);
			else if(magic[1] == 3){
				bg_alpha = ((VENDOR_VIDEOENC_OSG_STAMP_ATTR*)p_attr)->bg_alpha;
				p_attr = &(((VENDOR_VIDEOENC_OSG_STAMP_ATTR*)p_attr)->attr);
			}
		}
	}

	hdal_flow_log_p("        x(%d) y(%d) alpha(%d) colorkey_en(%d) colorkey_val(%d) qp_en(%d) qp_fix(%d) qp_val(%d) layer(%d) region(%d) gcac_en(%d) gcac_w(%d) gcac_h(%d)\n",
						p_attr->position.x, p_attr->position.y, p_attr->alpha,
						p_attr->colorkey_en, p_attr->colorkey_val,
						p_attr->qp_en, p_attr->qp_fix, p_attr->qp_val,
						p_attr->layer,p_attr->region, p_attr->gcac_enable,
						p_attr->gcac_blk_width, p_attr->gcac_blk_height);

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_SET_STAMP_ATTR;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	command.data.stamp.x               = p_attr->position.x;
	command.data.stamp.y               = p_attr->position.y;
	command.data.stamp.fg_alpha        = p_attr->alpha;
	command.data.stamp.bg_alpha        = bg_alpha;
	command.data.stamp.colorkey_en     = p_attr->colorkey_en;
	command.data.stamp.colorkey_val    = p_attr->colorkey_val;
	command.data.stamp.qp_en           = p_attr->qp_en;
	command.data.stamp.qp_fix          = p_attr->qp_fix;
	command.data.stamp.qp_val          = p_attr->qp_val;
	command.data.stamp.layer           = p_attr->layer;
	command.data.stamp.region          = p_attr->region;
	command.data.stamp.gcac_enable     = p_attr->gcac_enable;
	command.data.stamp.gcac_blk_width  = p_attr->gcac_blk_width;
	command.data.stamp.gcac_blk_height = p_attr->gcac_blk_height;

	return send_command(&command);
}

static int vds_get_stamp_attr(VDS_PHASE phase, UINT32 rgn, UINT32 vid, HD_OSG_STAMP_ATTR *p_attr)
{
	VDS_USER_DATA    command;
	int              ret;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", phase, rgn, vid);

	if(!p_attr){
		printf("vds_get_stamp_attr() : NULL p_attr\n");
		hdal_flow_log_p("        NULL p_attr\n");
		return HD_ERR_NULL_PTR;
	}

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_GET_STAMP_ATTR;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	ret = send_command(&command);
	if(ret){
		printf("VDS_USER_CMD_GET_STAMP_ATTR(%d,%d,%d) fail\r\n", phase, rgn, vid);
		return ret;
	}

	p_attr->position.x      = command.data.stamp.x;
	p_attr->position.y      = command.data.stamp.y;
	p_attr->alpha           = command.data.stamp.fg_alpha;
	p_attr->colorkey_en     = command.data.stamp.colorkey_en;
	p_attr->colorkey_val    = command.data.stamp.colorkey_val;
	p_attr->qp_en           = command.data.stamp.qp_en;
	p_attr->qp_fix          = command.data.stamp.qp_fix;
	p_attr->qp_val          = command.data.stamp.qp_val;
	p_attr->layer           = command.data.stamp.layer;
	p_attr->region          = command.data.stamp.region;
	p_attr->gcac_enable     = command.data.stamp.gcac_enable;
	p_attr->gcac_blk_width  = command.data.stamp.gcac_blk_width;
	p_attr->gcac_blk_height = command.data.stamp.gcac_blk_height;

	hdal_flow_log_p("        x(%d) y(%d) alpha(%d) colorkey_en(%d) colorkey_val(%d) qp_en(%d) qp_fix(%d) qp_val(%d) layer(%d) region(%d) gcac_en(%d) gcac_w(%d) gcac_h(%d)\n",
						p_attr->position.x, p_attr->position.y, p_attr->alpha,
						p_attr->colorkey_en, p_attr->colorkey_val,
						p_attr->qp_en, p_attr->qp_fix, p_attr->qp_val,
						p_attr->layer,p_attr->region, p_attr->gcac_enable,
						p_attr->gcac_blk_width, p_attr->gcac_blk_height);

	return HD_OK;
}

static int set_encoder_builtin_mask(VDS_PHASE phase, UINT32 rgn, UINT32 vid, _VDS_ENC_MASK *p_attr)
{
	VDS_USER_DATA command;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", (int)phase, (int)rgn, (int)vid);

	if(!p_attr){
		printf("set_encoder_builtin_mask() : NULL p_attr\n");
		hdal_flow_log_p("        NULL p_attr\n");
		return HD_ERR_NULL_PTR;
	}

	hdal_flow_log_p("        type(%d) color(%x) alpha(%d) x(%d) y(%d) w(%d) h(%d) thick(%d)\n",
					p_attr->data.type, p_attr->data.color, p_attr->data.alpha,
					p_attr->data.x, p_attr->data.y,
					p_attr->data.w, p_attr->data.h,
					p_attr->data.thickness);

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_SET_ENCODER_BUILTIN_MASK;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	command.data.enc_mask.type       = p_attr->data.type;
	command.data.enc_mask.color      = p_attr->data.color;
	command.data.enc_mask.alpha      = p_attr->data.alpha;
	command.data.enc_mask.x          = p_attr->data.x;
	command.data.enc_mask.y          = p_attr->data.y;
	command.data.enc_mask.w          = p_attr->data.w;
	command.data.enc_mask.h          = p_attr->data.h;
	command.data.enc_mask.thickness  = p_attr->data.thickness;
	command.data.enc_mask.layer      = p_attr->data.layer;
	command.data.enc_mask.region     = p_attr->data.region;

	return send_command(&command);
}

static int vds_set_mask_attr(VDS_PHASE phase, UINT32 rgn, UINT32 vid, HD_OSG_MASK_ATTR *p_attr)
{
	VDS_USER_DATA command;
	OSG_INCONTINUOUS_MASK *im = NULL;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", phase, rgn, vid);

	if(!p_attr){
		printf("vds_set_mask_attr() : NULL p_attr\n");
		hdal_flow_log_p("        NULL p_attr\n");
		return HD_ERR_NULL_PTR;
	}

	//handle vendor API
	{
		unsigned int *magic = (unsigned int*)p_attr;
		if(magic[0] == 0xABCDEFAB){
			if(magic[1] == 1)
				return set_encoder_builtin_mask(phase, rgn, vid, (_VDS_ENC_MASK *)p_attr);
		}
	}

	//encoder mask should use vendor API
	if(phase == VDS_PHASE_COE_STAMP){
		printf("vds_set_mask_attr() : encoder mask should use vendor_videoenc_set, not hd_videoenc_set\n");
		hdal_flow_log_p("        vds_set_mask_attr() : encoder mask should use vendor_videoenc_set, not hd_videoenc_set\n");
		return HD_ERR_NOT_SUPPORT;
	}

	if(p_attr->type == OSG_MASK_TYPE_INCONTINUOUS){
		im = (OSG_INCONTINUOUS_MASK*)p_attr;
		hdal_flow_log_p("        type(%d) color(%x) x(%d) y(%d) hline(%d) hhole(%d) vline(%d) vhole(%d) h_thick(%d) v_thick(%d)\n",
						im->type, im->color, im->x, im->y,
						im->h_line_len, im->h_hole_len,
						im->v_line_len, im->v_hole_len,
						im->h_thickness, im->v_thickness);
	}
	else
		hdal_flow_log_p("        type(%d) color(%x) alpha(%d) x(%d) y(%d) w(%d) h(%d) thick(%d)\n",
						p_attr->type, p_attr->color, p_attr->alpha,
						p_attr->position[0].x, p_attr->position[0].y,
						p_attr->position[1].x - p_attr->position[0].x,
						p_attr->position[2].y - p_attr->position[1].y,
						p_attr->thickness);

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_SET_MASK_ATTR;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	if(p_attr->type == HD_OSG_MASK_TYPE_SOLID || p_attr->type == HD_OSG_MASK_TYPE_NULL)
		command.data.mask.type = VDS_MASK_TYPE_SOLID;
	else if(p_attr->type == HD_OSG_MASK_TYPE_HOLLOW)
		command.data.mask.type = VDS_MASK_TYPE_HOLLOW;
	else if(p_attr->type == OSG_MASK_TYPE_INCONTINUOUS)
		command.data.mask.type = VDS_MASK_TYPE_INCONTINUOUS;
	else{
		printf("vds_set_mask_attr() : type(%d) is not supported\n", p_attr->type);
		hdal_flow_log_p("        type(%d) is invalid\n", p_attr->type);
		return HD_ERR_NOT_SUPPORT;
	}

	if(p_attr->type == OSG_MASK_TYPE_INCONTINUOUS){
		command.data.mask.color                            = im->color;
		command.data.mask.data.incontinuous.x              = im->x;
		command.data.mask.data.incontinuous.y              = im->y;
		command.data.mask.data.incontinuous.h_line_len     = im->h_line_len;
		command.data.mask.data.incontinuous.h_hole_len     = im->h_hole_len;
		command.data.mask.data.incontinuous.h_thickness    = im->h_thickness;
		command.data.mask.data.incontinuous.v_line_len     = im->v_line_len;
		command.data.mask.data.incontinuous.v_hole_len     = im->v_hole_len;
		command.data.mask.data.incontinuous.v_thickness    = im->v_thickness;
	}else{
		command.data.mask.color                = p_attr->color;
		command.data.mask.data.normal.alpha    = p_attr->alpha;
		command.data.mask.data.normal.x[0]     = p_attr->position[0].x;
		command.data.mask.data.normal.x[1]     = p_attr->position[1].x;
		command.data.mask.data.normal.x[2]     = p_attr->position[2].x;
		command.data.mask.data.normal.x[3]     = p_attr->position[3].x;
		command.data.mask.data.normal.y[0]     = p_attr->position[0].y;
		command.data.mask.data.normal.y[1]     = p_attr->position[1].y;
		command.data.mask.data.normal.y[2]     = p_attr->position[2].y;
		command.data.mask.data.normal.y[3]     = p_attr->position[3].y;
		command.data.mask.thickness            = p_attr->thickness;
	}

	return send_command(&command);
}

static int vds_get_mask_attr(VDS_PHASE phase, UINT32 rgn, UINT32 vid, HD_OSG_MASK_ATTR *p_attr)
{
	VDS_USER_DATA         command;
	OSG_INCONTINUOUS_MASK *im = NULL;
	int                   ret;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", phase, rgn, vid);

	if(!p_attr){
		printf("vds_get_mask_attr() : NULL p_attr\n");
		hdal_flow_log_p("        NULL p_attr\n");
		return HD_ERR_NULL_PTR;
	}

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_GET_MASK_ATTR;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	ret = send_command(&command);
	if(ret){
		printf("VDS_USER_CMD_GET_MASK_ATTR(%d,%d,%d) fail\r\n", phase, rgn, vid);
		return ret;
	}

	if(command.data.mask.type == VDS_MASK_TYPE_SOLID)
		p_attr->type = HD_OSG_MASK_TYPE_SOLID;
	else if(command.data.mask.type == VDS_MASK_TYPE_HOLLOW)
		p_attr->type = HD_OSG_MASK_TYPE_HOLLOW;
	else if(command.data.mask.type == VDS_MASK_TYPE_INCONTINUOUS){
		im       = (OSG_INCONTINUOUS_MASK*)p_attr;
		im->type = OSG_MASK_TYPE_INCONTINUOUS;
	}
	else{
		printf("vds_get_mask_attr() : type(%d) is invalid\n", command.data.mask.type);
		hdal_flow_log_p("        type(%d) is invalid\n", command.data.mask.type);
		return HD_ERR_NOT_SUPPORT;
	}

	if(command.data.mask.type == VDS_MASK_TYPE_INCONTINUOUS){
		im->color        = command.data.mask.color;
		im->x            = command.data.mask.data.incontinuous.x;
		im->y            = command.data.mask.data.incontinuous.y;
		im->h_line_len   = command.data.mask.data.incontinuous.h_line_len;
		im->h_hole_len   = command.data.mask.data.incontinuous.h_hole_len;
		im->h_thickness  = command.data.mask.data.incontinuous.h_thickness;
		im->v_line_len   = command.data.mask.data.incontinuous.v_line_len;
		im->v_hole_len   = command.data.mask.data.incontinuous.v_hole_len;
		im->v_thickness  = command.data.mask.data.incontinuous.v_thickness;
	}else{
		p_attr->color         = command.data.mask.color;
		p_attr->alpha         = command.data.mask.data.normal.alpha;
		p_attr->position[0].x = command.data.mask.data.normal.x[0];
		p_attr->position[1].x = command.data.mask.data.normal.x[1];
		p_attr->position[2].x = command.data.mask.data.normal.x[2];
		p_attr->position[3].x = command.data.mask.data.normal.x[3];
		p_attr->position[0].y = command.data.mask.data.normal.y[0];
		p_attr->position[1].y = command.data.mask.data.normal.y[1];
		p_attr->position[2].y = command.data.mask.data.normal.y[2];
		p_attr->position[3].y = command.data.mask.data.normal.y[3];
		p_attr->thickness     = command.data.mask.thickness;
	}

	if(command.data.mask.type == VDS_MASK_TYPE_INCONTINUOUS){
		hdal_flow_log_p("        type(%d) color(%x) x(%d) y(%d) hline(%d) hhole(%d) vline(%d) vhole(%d) h_thick(%d) v_thick(%d)\n",
						im->type, im->color, im->x, im->y,
						im->h_line_len, im->h_hole_len,
						im->v_line_len, im->v_hole_len,
						(int)im->h_thickness, (int)im->v_thickness);
	}else{
		hdal_flow_log_p("        type(%d) color(%x) alpha(%d) x(%d) y(%d) w(%d) h(%d) thick(%d)\n",
						p_attr->type, p_attr->color, p_attr->alpha,
						p_attr->position[0].x, p_attr->position[0].y,
						p_attr->position[1].x - p_attr->position[0].x,
						p_attr->position[2].y - p_attr->position[1].y,
						p_attr->thickness);
	}

	return HD_OK;
}

static int vds_set_mosaic_attr(VDS_PHASE phase, UINT32 rgn, UINT32 vid, HD_OSG_MOSAIC_ATTR *p_attr)
{
	VDS_USER_DATA command;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", phase, rgn, vid);

	if(!p_attr){
		printf("vds_set_mosaic_attr() : NULL p_attr\n");
		hdal_flow_log_p("        NULL p_attr\n");
		return HD_ERR_NULL_PTR;
	}

	hdal_flow_log_p("        bw(%d) bh(%d) x(%d) y(%d) w(%d) h(%d)\n",
						p_attr->mosaic_blk_w, p_attr->mosaic_blk_h,
						p_attr->position[0].x, p_attr->position[0].y,
						p_attr->position[1].x - p_attr->position[0].x,
						p_attr->position[2].y - p_attr->position[1].y);

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_SET_MOSAIC_ATTR;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	command.data.mosaic.mosaic_blk_w = p_attr->mosaic_blk_w;
	command.data.mosaic.mosaic_blk_h = p_attr->mosaic_blk_h;
	command.data.mosaic.x[0]         = p_attr->position[0].x;
	command.data.mosaic.x[1]         = p_attr->position[1].x;
	command.data.mosaic.x[2]         = p_attr->position[2].x;
	command.data.mosaic.x[3]         = p_attr->position[3].x;
	command.data.mosaic.y[0]         = p_attr->position[0].y;
	command.data.mosaic.y[1]         = p_attr->position[1].y;
	command.data.mosaic.y[2]         = p_attr->position[2].y;
	command.data.mosaic.y[3]         = p_attr->position[3].y;

	return send_command(&command);
}

static int vds_get_mosaic_attr(VDS_PHASE phase, UINT32 rgn, UINT32 vid, HD_OSG_MOSAIC_ATTR *p_attr)
{
	VDS_USER_DATA    command;
	int              ret;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", phase, rgn, vid);

	if(!p_attr){
		printf("vds_get_mosaic_attr() : NULL p_attr\n");
		hdal_flow_log_p("        NULL p_attr\n");
		return HD_ERR_NULL_PTR;
	}

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_GET_MOSAIC_ATTR;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	ret = send_command(&command);
	if(ret){
		printf("VDS_USER_CMD_GET_MOSAIC_ATTR(%d,%d,%d) fail\r\n", phase, rgn, vid);
		return ret;
	}

	p_attr->mosaic_blk_w  = command.data.mosaic.mosaic_blk_w;
	p_attr->mosaic_blk_h  = command.data.mosaic.mosaic_blk_h;
	p_attr->position[0].x = command.data.mosaic.x[0];
	p_attr->position[1].x = command.data.mosaic.x[1];
	p_attr->position[2].x = command.data.mosaic.x[2];
	p_attr->position[3].x = command.data.mosaic.x[3];
	p_attr->position[0].y = command.data.mosaic.y[0];
	p_attr->position[1].y = command.data.mosaic.y[1];
	p_attr->position[2].y = command.data.mosaic.y[2];
	p_attr->position[3].y = command.data.mosaic.y[3];

	hdal_flow_log_p("        bw(%d) bh(%d) x(%d) y(%d) w(%d) h(%d)\n",
						p_attr->mosaic_blk_w, p_attr->mosaic_blk_h,
						p_attr->position[0].x, p_attr->position[0].y,
						p_attr->position[1].x - p_attr->position[0].x,
						p_attr->position[2].y - p_attr->position[1].y);

	return HD_OK;
}

static int vds_set_palette_attr(VDS_PHASE phase, UINT32 rgn, UINT32 vid, HD_PALETTE_TBL *p_attr)
{
	VDS_USER_DATA command;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", phase, rgn, vid);

	if(!p_attr){
		printf("vds_set_palette_attr() : NULL p_attr\n");
		hdal_flow_log_p("        NULL p_attr\n");
		return HD_ERR_NULL_PTR;
	}
	
	if(p_attr->table_size != 16 * sizeof(UINT32)){
		printf("vds_set_palette_attr() : invalid table size(%d)\n", p_attr->table_size);
		hdal_flow_log_p("        invalid table size\n");
		return HD_ERR_INV;
	}

	if(!p_attr->p_table){
		printf("vds_set_palette_attr() : NULL p_table\n");
		hdal_flow_log_p("        NULL p_table\n");
		return HD_ERR_NULL_PTR;
	}

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_SET_PALETTE;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	command.data.palette.size   = p_attr->table_size;
	command.data.palette.p_addr = (UINT32)p_attr->p_table;

	return send_command(&command);
}

static int vds_get_palette_attr(VDS_PHASE phase, UINT32 rgn, UINT32 vid, HD_PALETTE_TBL *p_attr)
{
	VDS_USER_DATA         command;
	int                   ret;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", phase, rgn, vid);

	if(!p_attr){
		printf("vds_get_palette_attr() : NULL p_attr\n");
		hdal_flow_log_p("        NULL p_attr\n");
		return HD_ERR_NULL_PTR;
	}
	
	if(p_attr->table_size != 16 * sizeof(UINT32)){
		printf("vds_get_palette_attr() : invalid table size(%d)\n", (int)p_attr->table_size);
		hdal_flow_log_p("        invalid table size\n");
		return HD_ERR_INV;
	}

	if(!p_attr->p_table){
		printf("vds_get_palette_attr() : NULL p_table\n");
		hdal_flow_log_p("        NULL p_table\n");
		return HD_ERR_NULL_PTR;
	}

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_GET_PALETTE;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	command.data.palette.size   = p_attr->table_size;
	command.data.palette.p_addr = (UINT32)p_attr->p_table;

	ret = send_command(&command);
	if(ret){
		printf("VDS_USER_CMD_GET_PALETTE(%d,%d,%d) fail\r\n", phase, rgn, vid);
		return ret;
	}
	
	memset(p_attr, 0, sizeof(HD_PALETTE_TBL));
	p_attr->table_size = command.data.palette.size;
	p_attr->p_table    = (UINT32*)command.data.palette.p_addr;

	return HD_OK;
}

static int vds_close(VDS_PHASE phase, UINT32 rgn, UINT32 vid)
{
	VDS_USER_DATA command;

	hdal_flow_log_p("        phase(%d) rgn(%d) vid(%d)\n", phase, rgn, vid);

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_CLOSE;
	command.phase   = phase;
	command.rgn     = rgn;
	command.vid     = vid;

	return send_command(&command);
}


#if 0
int vds_allocate_buffer(int size, unsigned int *addr)
{
	VDS_USER_DATA command;
	int ret;

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_ALLOCATE;

	command.data.buf.size = size;
	ret = send_command(&command);
	if(ret){
		printf("VDS_USER_CMD_ALLOCATE(%d) fail\r\n", size);
		return ret;
	}

	*addr = command.data.buf.p_addr;

	return 0;
}
#endif

HD_RESULT _hd_osg_reset(void)
{
	VDS_USER_DATA command;

	hdal_flow_log_p("        _hd_osg_reset()\n");

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_RESET;

	return send_command(&command);
}


/*
    unit_id = (ISF_UNIT_VDOPRC + dev_id) or (ISF_UNIT_VDOENC + dev_id) or (ISF_UNIT_VDOOUT + dev_id)

    port_id = (ISF_IN_BASE + in_id) or (ISF_OUT_BASE + out_id)

    osg_id = (0x00010000 | stamp_id) or (0x00020000 | stamp_ex_id) or (0x00040000 | mask_id) or (0x00080000 | mask_ex_id)
*/

HD_RESULT _hd_osg_attach(UINT32 unit_id, UINT32 port_id, UINT32 osg_id)
{
	if((unit_id >= ISF_UNIT_VDOPRC) && (unit_id < ISF_UNIT_VDOPRC + ISF_MAX_VDOPRC)) {
		//vpc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = (((unit_id - ISF_UNIT_VDOPRC) << 16) | (port_id - ISF_OUT_BASE));
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) {phase = VDS_PHASE_IME_STAMP; vid = unit_id - ISF_UNIT_VDOPRC;}
		if(osg_id & 0x00020000)  phase = VDS_PHASE_IME_EXT_STAMP;
		if(osg_id & 0x00040000) {phase = VDS_PHASE_IME_MASK; vid = unit_id - ISF_UNIT_VDOPRC;}
		if(osg_id & 0x00080000)  phase = VDS_PHASE_IME_EXT_MASK;
		if(phase == VDS_PHASE_NULL){
			DBG_ERR("invalid phase=%d\n", (int)phase);
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		return vds_open(phase, rgn, vid);
	} else if((unit_id >= ISF_UNIT_VDOENC) && (unit_id < ISF_UNIT_VDOENC + ISF_MAX_VDOENC) && (port_id >= ISF_IN_BASE)) {
		//enc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = port_id - ISF_IN_BASE;
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) phase = VDS_PHASE_COE_STAMP;
		if(osg_id & 0x00020000) phase = VDS_PHASE_COE_EXT_STAMP;
		if(osg_id & 0x00040000) phase = VDS_PHASE_COE_STAMP;
		if(osg_id & 0x00080000) phase = VDS_PHASE_COE_EXT_MASK;
		if(phase == VDS_PHASE_NULL){
			DBG_ERR("invalid phase=%d\n", (int)phase);
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		return vds_open(phase, rgn, vid);
	} else if((unit_id >= ISF_UNIT_VDOOUT) && (unit_id < ISF_UNIT_VDOOUT + ISF_MAX_VDOOUT)) {
		//vo case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = unit_id - ISF_UNIT_VDOOUT;
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) phase = VDS_PHASE_VO_STAMP;
		if(osg_id & 0x00020000) phase = VDS_PHASE_VO_STAMP;
		if(osg_id & 0x00040000) phase = VDS_PHASE_VO_MASK;
		if(osg_id & 0x00080000) phase = VDS_PHASE_VO_MASK;
		if(phase == VDS_PHASE_NULL){
			DBG_ERR("invalid phase=%d\n", (int)phase);
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		return vds_open(phase, rgn, vid);
	}

	DBG_ERR("invalid unit_id=%d\n", (int)unit_id);
	hdal_flow_log_p("        invalid unit(%x) port(%x)\n", (int)unit_id, (int)port_id);
	return HD_ERR_PATH;
}

HD_RESULT _hd_osg_detach(UINT32 unit_id, UINT32 port_id, UINT32 osg_id)
{
	if((unit_id >= ISF_UNIT_VDOPRC) && (unit_id < ISF_UNIT_VDOPRC + ISF_MAX_VDOPRC)) {
		//vpc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = (((unit_id - ISF_UNIT_VDOPRC) << 16) | (port_id - ISF_OUT_BASE));
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) {phase = VDS_PHASE_IME_STAMP; vid = unit_id - ISF_UNIT_VDOPRC;}
		if(osg_id & 0x00020000)  phase = VDS_PHASE_IME_EXT_STAMP;
		if(osg_id & 0x00040000) {phase = VDS_PHASE_IME_MASK; vid = unit_id - ISF_UNIT_VDOPRC;}
		if(osg_id & 0x00080000)  phase = VDS_PHASE_IME_EXT_MASK;
		if(phase == VDS_PHASE_NULL){
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		return vds_close(phase, rgn, vid);
	} else if((unit_id >= ISF_UNIT_VDOENC) && (unit_id < ISF_UNIT_VDOENC + ISF_MAX_VDOENC) && (port_id >= ISF_IN_BASE)) {
		//enc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = port_id - ISF_IN_BASE;
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) phase = VDS_PHASE_COE_STAMP;
		if(osg_id & 0x00020000) phase = VDS_PHASE_COE_EXT_STAMP;
		if(osg_id & 0x00040000) phase = VDS_PHASE_COE_STAMP;
		if(osg_id & 0x00080000) phase = VDS_PHASE_COE_EXT_MASK;
		if(phase == VDS_PHASE_NULL){
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		return vds_close(phase, rgn, vid);
	} else if((unit_id >= ISF_UNIT_VDOOUT) && (unit_id < ISF_UNIT_VDOOUT + ISF_MAX_VDOOUT)) {
		//vpc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = unit_id - ISF_UNIT_VDOOUT;
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) phase = VDS_PHASE_VO_STAMP;
		if(osg_id & 0x00020000) phase = VDS_PHASE_VO_STAMP;
		if(osg_id & 0x00040000) phase = VDS_PHASE_VO_MASK;
		if(osg_id & 0x00080000) phase = VDS_PHASE_VO_MASK;
		if(phase == VDS_PHASE_NULL){
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		return vds_close(phase, rgn, vid);
	}

	hdal_flow_log_p("        invalid unit(%x) port(%x)\n", unit_id, port_id);
	return HD_ERR_PATH;
}

HD_RESULT _hd_osg_get(UINT32 unit_id, UINT32 port_id, UINT32 osg_id, UINT32 param_id, VOID* p_param)
{
	if((unit_id >= ISF_UNIT_VDOPRC) && (unit_id < ISF_UNIT_VDOPRC + ISF_MAX_VDOPRC)) {
		//vpc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = (((unit_id - ISF_UNIT_VDOPRC) << 16) | (port_id - ISF_OUT_BASE));
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) {phase = VDS_PHASE_IME_STAMP; vid = unit_id - ISF_UNIT_VDOPRC;}
		if(osg_id & 0x00020000)  phase = VDS_PHASE_IME_EXT_STAMP;
		if(osg_id & 0x00040000) {phase = VDS_PHASE_IME_MASK; vid = unit_id - ISF_UNIT_VDOPRC;}
		if(osg_id & 0x00080000)  phase = VDS_PHASE_IME_EXT_MASK;
		if(phase == VDS_PHASE_NULL){
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		switch(param_id) {
		case (UINT32)HD_VIDEOPROC_PARAM_IN_STAMP_BUF:
			return vds_get_stamp_buf(phase, rgn, vid, (HD_OSG_STAMP_BUF*)p_param);
		case (UINT32)HD_VIDEOPROC_PARAM_IN_STAMP_IMG:
			return vds_get_stamp_img(phase, rgn, vid, (HD_OSG_STAMP_IMG*)p_param);
		case (UINT32)HD_VIDEOPROC_PARAM_IN_STAMP_ATTR:
			return vds_get_stamp_attr(phase, rgn, vid, (HD_OSG_STAMP_ATTR*)p_param);
		case (UINT32)HD_VIDEOPROC_PARAM_IN_MASK_ATTR:
			return vds_get_mask_attr(phase, rgn, vid, (HD_OSG_MASK_ATTR*)p_param);
		case (UINT32)HD_VIDEOPROC_PARAM_IN_MOSAIC_ATTR:
			return vds_get_mosaic_attr(phase, rgn, vid, (HD_OSG_MOSAIC_ATTR*)p_param);
		case (UINT32)HD_VIDEOPROC_PARAM_IN_PALETTE_TABLE:
			return vds_get_palette_attr(phase, rgn, vid, (HD_PALETTE_TBL*)p_param);
		}
	} else if((unit_id >= ISF_UNIT_VDOENC) && (unit_id < ISF_UNIT_VDOENC + ISF_MAX_VDOENC) && (port_id >= ISF_IN_BASE)) {
		//enc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = port_id - ISF_IN_BASE;
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) phase = VDS_PHASE_COE_STAMP;
		if(osg_id & 0x00020000) phase = VDS_PHASE_COE_EXT_STAMP;
		if(osg_id & 0x00040000) phase = VDS_PHASE_COE_STAMP;
		if(osg_id & 0x00080000) phase = VDS_PHASE_COE_EXT_MASK;
		if(phase == VDS_PHASE_NULL){
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		switch(param_id) {
		case (UINT32)HD_VIDEOENC_PARAM_IN_STAMP_BUF:
			return vds_get_stamp_buf(phase, rgn, vid, (HD_OSG_STAMP_BUF*)p_param);
		case (UINT32)HD_VIDEOENC_PARAM_IN_STAMP_IMG:
			return vds_get_stamp_img(phase, rgn, vid, (HD_OSG_STAMP_IMG*)p_param);
		case (UINT32)HD_VIDEOENC_PARAM_IN_STAMP_ATTR:
			return vds_get_stamp_attr(phase, rgn, vid, (HD_OSG_STAMP_ATTR*)p_param);
		case (UINT32)HD_VIDEOENC_PARAM_IN_MASK_ATTR:
			return vds_get_mask_attr(phase, rgn, vid, (HD_OSG_MASK_ATTR*)p_param);
		case (UINT32)HD_VIDEOENC_PARAM_IN_MOSAIC_ATTR:
			return vds_get_mosaic_attr(phase, rgn, vid, (HD_OSG_MOSAIC_ATTR*)p_param);
		case (UINT32)HD_VIDEOENC_PARAM_IN_PALETTE_TABLE:
			return vds_get_palette_attr(phase, rgn, vid, (HD_PALETTE_TBL*)p_param);
		}
	} else if((unit_id >= ISF_UNIT_VDOOUT) && (unit_id < ISF_UNIT_VDOOUT + ISF_MAX_VDOOUT)) {
		//vpc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = unit_id - ISF_UNIT_VDOOUT;
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) phase = VDS_PHASE_VO_STAMP;
		if(osg_id & 0x00020000) phase = VDS_PHASE_VO_STAMP;
		if(osg_id & 0x00040000) phase = VDS_PHASE_VO_MASK;
		if(osg_id & 0x00080000) phase = VDS_PHASE_VO_MASK;
		if(phase == VDS_PHASE_NULL){
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		switch(param_id) {
		case (UINT32)HD_VIDEOOUT_PARAM_OUT_STAMP_BUF:
			return vds_get_stamp_buf(phase, rgn, vid, (HD_OSG_STAMP_BUF*)p_param);
		case (UINT32)HD_VIDEOOUT_PARAM_OUT_STAMP_IMG:
			return vds_get_stamp_img(phase, rgn, vid, (HD_OSG_STAMP_IMG*)p_param);
		case (UINT32)HD_VIDEOOUT_PARAM_OUT_STAMP_ATTR:
			return vds_get_stamp_attr(phase, rgn, vid, (HD_OSG_STAMP_ATTR*)p_param);
		case (UINT32)HD_VIDEOOUT_PARAM_OUT_MASK_ATTR:
			return vds_get_mask_attr(phase, rgn, vid, (HD_OSG_MASK_ATTR*)p_param);
		case (UINT32)HD_VIDEOOUT_PARAM_OUT_MOSAIC_ATTR:
			return vds_get_mosaic_attr(phase, rgn, vid, (HD_OSG_MOSAIC_ATTR*)p_param);
		case (UINT32)HD_VIDEOOUT_PARAM_OUT_PALETTE_TABLE:
			return vds_get_palette_attr(phase, rgn, vid, (HD_PALETTE_TBL*)p_param);
		}
	}

	hdal_flow_log_p("        invalid unit(%x) port(%x)\n", unit_id, port_id);
	return HD_ERR_PATH;
}

HD_RESULT _hd_osg_set(UINT32 unit_id, UINT32 port_id, UINT32 osg_id, UINT32 param_id, VOID* p_param)
{
	if((unit_id >= ISF_UNIT_VDOPRC) && (unit_id < ISF_UNIT_VDOPRC + ISF_MAX_VDOPRC)) {
		//vpc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = (((unit_id - ISF_UNIT_VDOPRC) << 16) | (port_id - ISF_OUT_BASE));
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) {phase = VDS_PHASE_IME_STAMP; vid = unit_id - ISF_UNIT_VDOPRC;}
		if(osg_id & 0x00020000)  phase = VDS_PHASE_IME_EXT_STAMP;
		if(osg_id & 0x00040000) {phase = VDS_PHASE_IME_MASK; vid = unit_id - ISF_UNIT_VDOPRC;}
		if(osg_id & 0x00080000)  phase = VDS_PHASE_IME_EXT_MASK;
		if(phase == VDS_PHASE_NULL){
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		switch(param_id) {
		case (UINT32)HD_VIDEOPROC_PARAM_IN_STAMP_BUF:
			return vds_set_stamp_buf(phase, rgn, vid, (HD_OSG_STAMP_BUF*)p_param);
		case (UINT32)HD_VIDEOPROC_PARAM_IN_STAMP_IMG:
			return vds_set_stamp_img(phase, rgn, vid, (HD_OSG_STAMP_IMG*)p_param);
		case (UINT32)HD_VIDEOPROC_PARAM_IN_STAMP_ATTR:
			return vds_set_stamp_attr(phase, rgn, vid, (HD_OSG_STAMP_ATTR*)p_param);
		case (UINT32)HD_VIDEOPROC_PARAM_IN_MASK_ATTR:
			return vds_set_mask_attr(phase, rgn, vid, (HD_OSG_MASK_ATTR*)p_param);
		case (UINT32)HD_VIDEOPROC_PARAM_IN_MOSAIC_ATTR:
			return vds_set_mosaic_attr(phase, rgn, vid, (HD_OSG_MOSAIC_ATTR*)p_param);
		case (UINT32)HD_VIDEOPROC_PARAM_IN_PALETTE_TABLE:
			return vds_set_palette_attr(phase, rgn, vid, (HD_PALETTE_TBL*)p_param);
		}
	} else if((unit_id >= ISF_UNIT_VDOENC) && (unit_id < ISF_UNIT_VDOENC + ISF_MAX_VDOENC) && (port_id >= ISF_IN_BASE)) {
		//enc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = port_id - ISF_IN_BASE;
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) phase = VDS_PHASE_COE_STAMP;
		if(osg_id & 0x00020000) phase = VDS_PHASE_COE_EXT_STAMP;
		if(osg_id & 0x00040000) phase = VDS_PHASE_COE_STAMP;
		if(osg_id & 0x00080000) phase = VDS_PHASE_COE_EXT_MASK;
		if(phase == VDS_PHASE_NULL){
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		switch(param_id) {
		case (UINT32)HD_VIDEOENC_PARAM_IN_STAMP_BUF:
			return vds_set_stamp_buf(phase, rgn, vid, (HD_OSG_STAMP_BUF*)p_param);
		case (UINT32)HD_VIDEOENC_PARAM_IN_STAMP_IMG:
			return vds_set_stamp_img(phase, rgn, vid, (HD_OSG_STAMP_IMG*)p_param);
		case (UINT32)HD_VIDEOENC_PARAM_IN_STAMP_ATTR:
			return vds_set_stamp_attr(phase, rgn, vid, (HD_OSG_STAMP_ATTR*)p_param);
		case (UINT32)HD_VIDEOENC_PARAM_IN_MASK_ATTR:
			return vds_set_mask_attr(phase, rgn, vid, (HD_OSG_MASK_ATTR*)p_param);
		case (UINT32)HD_VIDEOENC_PARAM_IN_MOSAIC_ATTR:
			return vds_set_mosaic_attr(phase, rgn, vid, (HD_OSG_MOSAIC_ATTR*)p_param);
		case (UINT32)HD_VIDEOENC_PARAM_IN_PALETTE_TABLE:
			return vds_set_palette_attr(phase, rgn, vid, (HD_PALETTE_TBL*)p_param);
		}
	} else if ((unit_id >= ISF_UNIT_VDOOUT) && (unit_id < ISF_UNIT_VDOOUT + ISF_MAX_VDOOUT)) {
		//vpc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = unit_id - ISF_UNIT_VDOOUT;
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) phase = VDS_PHASE_VO_STAMP;
		if(osg_id & 0x00020000) phase = VDS_PHASE_VO_STAMP;
		if(osg_id & 0x00040000) phase = VDS_PHASE_VO_MASK;
		if(osg_id & 0x00080000) phase = VDS_PHASE_VO_MASK;
		if(phase == VDS_PHASE_NULL){
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		switch(param_id) {
		case (UINT32)HD_VIDEOOUT_PARAM_OUT_STAMP_BUF:
			return vds_set_stamp_buf(phase, rgn, vid, (HD_OSG_STAMP_BUF*)p_param);
		case (UINT32)HD_VIDEOOUT_PARAM_OUT_STAMP_IMG:
			return vds_set_stamp_img(phase, rgn, vid, (HD_OSG_STAMP_IMG*)p_param);
		case (UINT32)HD_VIDEOOUT_PARAM_OUT_STAMP_ATTR:
			return vds_set_stamp_attr(phase, rgn, vid, (HD_OSG_STAMP_ATTR*)p_param);
		case (UINT32)HD_VIDEOOUT_PARAM_OUT_MASK_ATTR:
			return vds_set_mask_attr(phase, rgn, vid, (HD_OSG_MASK_ATTR*)p_param);
		case (UINT32)HD_VIDEOOUT_PARAM_OUT_MOSAIC_ATTR:
			return vds_set_mosaic_attr(phase, rgn, vid, (HD_OSG_MOSAIC_ATTR*)p_param);
		case (UINT32)HD_VIDEOOUT_PARAM_OUT_PALETTE_TABLE:
			return vds_set_palette_attr(phase, rgn, vid, (HD_PALETTE_TBL*)p_param);
		}
	}

	hdal_flow_log_p("        invalid unit(%x) port(%x)\n", unit_id, port_id);
	return HD_ERR_PATH;
}

HD_RESULT _hd_osg_enable(UINT32 unit_id, UINT32 port_id, UINT32 osg_id)
{
	if((unit_id >= ISF_UNIT_VDOPRC) && (unit_id < ISF_UNIT_VDOPRC + ISF_MAX_VDOPRC)) {
		//vpc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = (((unit_id - ISF_UNIT_VDOPRC) << 16) | (port_id - ISF_OUT_BASE));
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) {phase = VDS_PHASE_IME_STAMP; vid = unit_id - ISF_UNIT_VDOPRC;}
		if(osg_id & 0x00020000)  phase = VDS_PHASE_IME_EXT_STAMP;
		if(osg_id & 0x00040000) {phase = VDS_PHASE_IME_MASK; vid = unit_id - ISF_UNIT_VDOPRC;}
		if(osg_id & 0x00080000)  phase = VDS_PHASE_IME_EXT_MASK;
		if(phase == VDS_PHASE_NULL){
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		return vds_start(phase, rgn, vid);
	} else if((unit_id >= ISF_UNIT_VDOENC) && (unit_id < ISF_UNIT_VDOENC + ISF_MAX_VDOENC) && (port_id >= ISF_IN_BASE)) {
		//enc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = port_id - ISF_IN_BASE;
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) phase = VDS_PHASE_COE_STAMP;
		if(osg_id & 0x00020000) phase = VDS_PHASE_COE_EXT_STAMP;
		if(osg_id & 0x00040000) phase = VDS_PHASE_COE_STAMP;
		if(osg_id & 0x00080000) phase = VDS_PHASE_COE_EXT_MASK;
		if(phase == VDS_PHASE_NULL){
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		return vds_start(phase, rgn, vid);
	} else if ((unit_id >= ISF_UNIT_VDOOUT) && (unit_id < ISF_UNIT_VDOOUT + ISF_MAX_VDOOUT)) {
		//vpc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = unit_id - ISF_UNIT_VDOOUT;
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) phase = VDS_PHASE_VO_STAMP;
		if(osg_id & 0x00020000) phase = VDS_PHASE_VO_STAMP;
		if(osg_id & 0x00040000) phase = VDS_PHASE_VO_MASK;
		if(osg_id & 0x00080000) phase = VDS_PHASE_VO_MASK;
		if(phase == VDS_PHASE_NULL){
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		return vds_start(phase, rgn, vid);
	}

	hdal_flow_log_p("        invalid unit(%x) port(%x)\n", unit_id, port_id);
	return HD_ERR_PATH;
}

HD_RESULT _hd_osg_disable(UINT32 unit_id, UINT32 port_id, UINT32 osg_id)
{
	if((unit_id >= ISF_UNIT_VDOPRC) && (unit_id < ISF_UNIT_VDOPRC + ISF_MAX_VDOPRC)) {
		//vpc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = (((unit_id - ISF_UNIT_VDOPRC) << 16) | (port_id - ISF_OUT_BASE));
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) {phase = VDS_PHASE_IME_STAMP; vid = unit_id - ISF_UNIT_VDOPRC;}
		if(osg_id & 0x00020000)  phase = VDS_PHASE_IME_EXT_STAMP;
		if(osg_id & 0x00040000) {phase = VDS_PHASE_IME_MASK; vid = unit_id - ISF_UNIT_VDOPRC;}
		if(osg_id & 0x00080000)  phase = VDS_PHASE_IME_EXT_MASK;
		if(phase == VDS_PHASE_NULL){
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		return vds_stop(phase, rgn, vid);
	} else if((unit_id >= ISF_UNIT_VDOENC) && (unit_id < ISF_UNIT_VDOENC + ISF_MAX_VDOENC) && (port_id >= ISF_IN_BASE)) {
		//enc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = port_id - ISF_IN_BASE;
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) phase = VDS_PHASE_COE_STAMP;
		if(osg_id & 0x00020000) phase = VDS_PHASE_COE_EXT_STAMP;
		if(osg_id & 0x00040000) phase = VDS_PHASE_COE_STAMP;
		if(osg_id & 0x00080000) phase = VDS_PHASE_COE_EXT_MASK;
		if(phase == VDS_PHASE_NULL){
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		return vds_stop(phase, rgn, vid);
	} else if ((unit_id >= ISF_UNIT_VDOOUT) && (unit_id < ISF_UNIT_VDOOUT + ISF_MAX_VDOOUT)) {
		//vpc case
		VDS_PHASE phase = VDS_PHASE_NULL;
		UINT32 vid = unit_id - ISF_UNIT_VDOOUT;
		UINT32 rgn = 0;
		if(osg_id & 0x00010000) phase = VDS_PHASE_VO_STAMP;
		if(osg_id & 0x00020000) phase = VDS_PHASE_VO_STAMP;
		if(osg_id & 0x00040000) phase = VDS_PHASE_VO_MASK;
		if(osg_id & 0x00080000) phase = VDS_PHASE_VO_MASK;
		if(phase == VDS_PHASE_NULL){
			hdal_flow_log_p("        invalid phase(%x)\n", phase);
			return HD_ERR_PATH;
		}
		rgn = osg_id & 0xffff;

		return vds_stop(phase, rgn, vid);
	}

	hdal_flow_log_p("        invalid unit(%x) port(%x)\n", unit_id, port_id);
	return HD_ERR_PATH;
}

void _hd_osg_cfg_max(unsigned int max_prc_path,
	                 unsigned int max_enc_path,
					 unsigned int max_out_path,
					 unsigned int max_stamp_img,
					 unsigned int *max_prc_stamp,
					 unsigned int *max_prc_mask,
					 unsigned int *max_enc_stamp,
					 unsigned int *max_enc_mask,
					 unsigned int *max_out_stamp,
					 unsigned int *max_out_mask)
{
	VDS_USER_DATA command;

	if(!max_prc_stamp || !max_prc_mask || !max_enc_stamp ||
	   !max_enc_mask || !max_out_stamp || !max_out_mask){
		printf("proc stamp(%p), proc mask(%p), enc stamp(%p) enc mask(%p), out stamp(%p), out mask(%p) is NULL\n",
			max_prc_stamp, max_prc_mask, max_enc_stamp, max_enc_mask, max_out_stamp, max_out_mask);
		hdal_flow_log_p("        NULL cfg array\n");
		return ;
	}

	hdal_flow_log_p("proc path(%d), enc path(%d), out path(%d), stamp image(%d)\n",
			max_prc_path, max_enc_path, max_out_path, max_stamp_img);
	hdal_flow_log_p("proc stamp(%d,%d), proc mask(%d,%d), enc stamp(%d,%d), enc mask(%d,%d), out stamp(%d,%d), out mask(%d,%d)\n",
			max_prc_stamp[0], max_prc_stamp[1], max_prc_mask[0], max_prc_mask[1],
			max_enc_stamp[0], max_enc_stamp[1], max_enc_mask[0], max_enc_mask[1],
			max_out_stamp[0], max_out_stamp[1], max_out_mask[0], max_out_mask[1]);

	memset(&command, 0, sizeof(VDS_USER_DATA));
	command.version = VDS_USER_VERSION;
	command.cmd     = VDS_USER_CMD_SET_CFG_MAX;

	command.data.cfg.max_prc_path     = max_prc_path;
	command.data.cfg.max_enc_path     = max_enc_path;
	command.data.cfg.max_out_path     = max_out_path;
	command.data.cfg.max_stamp_img    = max_stamp_img;
	command.data.cfg.max_prc_stamp[0] = max_prc_stamp[0];
	command.data.cfg.max_prc_stamp[1] = max_prc_stamp[1];
	command.data.cfg.max_prc_mask[0]  = max_prc_mask[0];
	command.data.cfg.max_prc_mask[1]  = max_prc_mask[1];
	command.data.cfg.max_enc_stamp[0] = max_enc_stamp[0];
	command.data.cfg.max_enc_stamp[1] = max_enc_stamp[1];
	command.data.cfg.max_enc_mask[0]  = max_enc_mask[0];
	command.data.cfg.max_enc_mask[1]  = max_enc_mask[1];
	command.data.cfg.max_out_stamp[0] = max_out_stamp[0];
	command.data.cfg.max_out_stamp[1] = max_out_stamp[1];
	command.data.cfg.max_out_mask[0]  = max_out_mask[0];
	command.data.cfg.max_out_mask[1]  = max_out_mask[1];

	send_command(&command);
}

int _hd_osg_is_init(VOID)
{
	return (proc_fd >= 0);
}

void _hd_osg_uninit(VOID)
{
	VDS_CLOSE(proc_fd);
	proc_fd = -1;
}

