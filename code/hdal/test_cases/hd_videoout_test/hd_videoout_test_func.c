
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "hdal.h"
#include "hd_videoout_test_func.h"
#include "vendor_videoout.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          vout_func
#define __DBGLVL__          6 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
 #include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////

UINT32 vdoout_test_enable(HD_PATH_ID video_out_ctrl,UINT32 id,UINT32 bEn)
{
	HD_RESULT ret = HD_OK;
	HD_FB_ENABLE video_out_enable={0};

	video_out_enable.fb_id = id;
	video_out_enable.enable = bEn;

	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_ENABLE, &video_out_enable);
	if(ret!= HD_OK)
		return ret;
	memset((void *)&video_out_enable,0,sizeof(HD_FB_ENABLE));

	video_out_enable.fb_id = id;
	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_ENABLE, &video_out_enable);
	if(ret!= HD_OK)
		return ret;
	//printf("##video_out_enable id:%d,en:%d \r\n", (int)(video_out_enable.fb_id), (int)(video_out_enable.enable));

	return ret;
}

UINT32 vdoout_test_format(HD_PATH_ID video_out_ctrl,UINT32 id,UINT32 fmt)
{
	HD_RESULT ret = HD_OK;
	HD_FB_FMT video_out_fmt={0};

	video_out_fmt.fb_id = id;
	video_out_fmt.fmt = fmt;

	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_FMT, &video_out_fmt);
	if(ret!= HD_OK)
		return ret;
	memset((void *)&video_out_fmt,0,sizeof(HD_FB_FMT));

	video_out_fmt.fb_id = id;
	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_FMT, &video_out_fmt);
	if(ret!= HD_OK)
		return ret;
	DBG_IND("FB%d,fmt:%x \r\n", (int)(video_out_fmt.fb_id), (unsigned int)(video_out_fmt.fmt));

	return ret;
}

UINT32 vdoout_test_palette(HD_PATH_ID video_out_ctrl,UINT32 id)
{
	HD_RESULT ret = HD_OK;
	HD_FB_PALETTE_TBL video_out_palette={0};
	UINT32 table[256]={ARBG_8888_RED,ARBG_8888_GREEN,ARBG_8888_BLUE,ARBG_8888_PINK,ARBG_8888_PINK_50,0};

	video_out_palette.fb_id = id;
	video_out_palette.table_size = 256;
	video_out_palette.p_table = table;

	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_PALETTE_TABLE, &video_out_palette);
	if(ret!= HD_OK)
		return ret;
	memset((void *)&video_out_palette,0,sizeof(HD_FB_PALETTE_TBL));
	video_out_palette.fb_id = id;
	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_PALETTE_TABLE, &video_out_palette);
	if(ret!= HD_OK)
		return ret;
	DBG_IND("FB%d,table_size:%d %x %x %x %x\r\n",video_out_palette.fb_id,video_out_palette.table_size,video_out_palette.p_table[0]
	,video_out_palette.p_table[1],video_out_palette.p_table[2],video_out_palette.p_table[3]);

	return ret;
}

UINT32 vdoout_test_attr(HD_PATH_ID video_out_ctrl,UINT32 id,UINT32 blend,UINT16 r,UINT16 g,UINT16 b)
{
	HD_RESULT ret = HD_OK;
	HD_FB_ATTR video_out_attr={0};

	video_out_attr.fb_id = id;
	video_out_attr.alpha_blend = blend;
	video_out_attr.alpha_1555 = blend;
	if(!r&&!g&&!b) {
		video_out_attr.colorkey_en = 0;
	} else {
		video_out_attr.colorkey_en = 1;
		video_out_attr.r_ckey = r;
		video_out_attr.g_ckey = g;
		video_out_attr.b_ckey = b;
	}
	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_ATTR, &video_out_attr);
	if(ret!= HD_OK)
		return ret;
	memset((void *)&video_out_attr,0,sizeof(HD_FB_ATTR));
	video_out_attr.fb_id = id;
	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_ATTR, &video_out_attr);
	if(ret!= HD_OK)
		return ret;
	DBG_IND("FB%d,alpha_blend:%d ckey_en %d color %x %x %x\r\n", (int)(video_out_attr.fb_id), (int)(video_out_attr.alpha_blend), (int)(video_out_attr.colorkey_en), (unsigned int)(r), (unsigned int)(g), (unsigned int)(b));

	return ret;
}

UINT32 vdoout_test_fb_dim(HD_PATH_ID video_out_ctrl,HD_DIM *p_input_dim,HD_URECT *p_out_cur_win)
{
    HD_FB_DIM fb_dim_param;
    UINT32 ret = 0;

    fb_dim_param.fb_id = HD_FB0;
    fb_dim_param.input_dim.w = p_input_dim->w;
    fb_dim_param.input_dim.h = p_input_dim->h;

    memcpy((void *)&fb_dim_param.output_rect,(void *)p_out_cur_win,sizeof(HD_URECT));

    ret = hd_videoout_set(video_out_ctrl,HD_VIDEOOUT_PARAM_FB_DIM, &fb_dim_param);
    if(ret!=0){
        return ret;
    }
    memset((void *)&fb_dim_param,0,sizeof(HD_FB_DIM));
    ret = hd_videoout_get(video_out_ctrl,HD_VIDEOOUT_PARAM_FB_DIM, &fb_dim_param);
    if(ret!=0){
        return ret;
    }
    DBG_IND("x:%d,y:%d,w:%d,h:%d \r\n", (int)(fb_dim_param.output_rect.x), (int)(fb_dim_param.output_rect.y), (int)(fb_dim_param.output_rect.w), (int)(fb_dim_param.output_rect.h));
    return ret;
}

VOUT_FBM_OBJ ui_buf_obj ={0};

#if defined(__LINUX_USER__)
UINT32 vdoout_fbm_init(VOUT_FBM_INIT *obj)
{
    ui_buf_obj.buffer[0] = (char *)obj->addr;
    return 0;
}
char *vdoout_fbm_getflip(void)
{
    return ui_buf_obj.buffer[0];
}
UINT32 vdoout_fbm_setflip(void)
{
    return 0;
}


#else
INT32 vdoout_get_fmt_coef(UINT32 fmt)
{
	switch(fmt) {
		case HD_VIDEO_PXLFMT_ARGB1555:
			return 2;
		case HD_VIDEO_PXLFMT_ARGB8888:
			return 4;
		case HD_VIDEO_PXLFMT_ARGB4444:
			return 2;
		case HD_VIDEO_PXLFMT_ARGB8565:
			return 3;  //A+RGB total buf
        case HD_VIDEO_PXLFMT_I8:
			return 1;
		default:
			DBG_ERR("not supp %d\r\n", (int)(fmt));
			return 1;
	}
}

UINT32 vdoout_fbm_init(VOUT_FBM_INIT *obj)
{
    if((!obj->width)||(!obj->height)||(!obj->addr)||(!obj->size)||(!obj->format)) {
        DBG_ERR("err parm\r\n");
        return -1;
    }
    memset((void *)&ui_buf_obj,0,sizeof(VOUT_FBM_OBJ));
    ui_buf_obj.width = obj->width;
    ui_buf_obj.height= obj->height;
    ui_buf_obj.format= obj->format;
    ui_buf_obj.format_coef = vdoout_get_fmt_coef(obj->format);

    ui_buf_obj.one_buf_size = ui_buf_obj.width*ui_buf_obj.height*ui_buf_obj.format_coef;
    if(obj->size<ui_buf_obj.one_buf_size) {
        DBG_ERR("not not enough");
        return -1;
    }
    ui_buf_obj.buffer[0] = (char *)obj->addr;
    if(obj->size>=2*ui_buf_obj.one_buf_size){
        ui_buf_obj.buffer[1] = (char *)obj->addr+ui_buf_obj.width*ui_buf_obj.height*ui_buf_obj.format_coef;
    }
    ui_buf_obj.flag = 1;
    //DBG_IND("ui_screen_w %d ui_screen_h %d ui_format_coef %d \r\n", (int)(ui_buf_obj.width), (int)(ui_buf_obj.height), (int)(ui_buf_obj.format_coef));
    return 0;

}
char *vdoout_fbm_getflip(void)
{
    char *addr = 0;
    if(ui_buf_obj.flag==1) {
        addr = ui_buf_obj.buffer[ui_buf_obj.buf_index];
        //DBG_IND("%x ,%d\r\n", (unsigned int)((UINT32)addr), (int)ui_buf_obj.buf_index);
    } else {
        DBG_ERR("no init\r\n");
    }

    return addr;
}
UINT32 vdoout_fbm_setflip(void)
{
    VENDOR_FB_INIT fb_init;
    UINT32 ret=0;
    char *addr = ui_buf_obj.buffer[ui_buf_obj.buf_index];

    //assign bf buffer to FB_INIT
    fb_init.fb_id = HD_FB0;
    fb_init.pa_addr = (UINT32)addr;
    fb_init.buf_len = ui_buf_obj.one_buf_size;
	ret = vendor_videoout_set(VENDOR_VIDEOOUT_ID0, VENDOR_VIDEOOUT_ITEM_FB_INIT, &fb_init);
	if(ret!= HD_OK) {
		DBG_ERR("init FB %d\n", (int)(errno));
		return ret;
	}

    if(ui_buf_obj.buffer[1]){
        ui_buf_obj.buf_index++;
        if(ui_buf_obj.buf_index==2)
            ui_buf_obj.buf_index =0;
    }
    return ret;
}
#endif

