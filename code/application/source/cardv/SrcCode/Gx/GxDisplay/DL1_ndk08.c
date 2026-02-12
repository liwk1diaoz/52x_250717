
#include "DL.h"
#include "GxVideo.h"
#include "GxDisplay.h" //for DISP_PXLFMT_XXXX
#include "GxDisplay_int.h"
#include "vendor_videoout.h"
#include "hd_common.h"
#include "FileSysTsk.h"

UINT32 fb_map[LAYER_NUM]={HD_FB0,0,0,0};
static UINT32 _DL_fmt_coef(UINT32 fmt)
{
	switch(fmt) {
		case DISP_PXLFMT_ARGB1555_PK:
			return 2;
		case DISP_PXLFMT_ARGB8888_PK:
			return 4;
		case DISP_PXLFMT_ARGB4444_PK:
			return 2;
		case DISP_PXLFMT_ARGB8565_PK:
			return 3; //RGB plan lineoffset
        case DISP_PXLFMT_INDEX8:
			return 1;
		default:
			DBG_ERR("not supp %d\r\n",fmt);
			return 1;
	}
}

static UINT32 _DL_fmt_hdal(UINT32 fmt)
{
	switch(fmt) {
		case DISP_PXLFMT_ARGB1555_PK:
			return HD_VIDEO_PXLFMT_ARGB1555;
		case DISP_PXLFMT_ARGB8888_PK:
			return HD_VIDEO_PXLFMT_ARGB8888;
		case DISP_PXLFMT_ARGB4444_PK:
			return HD_VIDEO_PXLFMT_ARGB4444;
		case DISP_PXLFMT_ARGB8565_PK:
			return HD_VIDEO_PXLFMT_ARGB8565;
        case DISP_PXLFMT_INDEX8:
			return HD_VIDEO_PXLFMT_I8;
		default:
			DBG_ERR("not supp 0x%x\r\n",fmt);
			return 1;
	}
}

INT32 _DL_BufferSwitch(UINT32 LayerID,
					  UINT32 w, UINT32 h, UINT16 PxlFmt, UINT32 uiBufAddr)

{
    HD_RESULT ret;
    VENDOR_FB_INIT fb_init;
	UINT32 cDevID = _DD(LayerID);
	UINT32 cLayerID = _DL(LayerID);

#if (DEVICE_COUNT < 2)
	if (cDevID >= 1) {
        DBG_ERR("not sup 0x%x\r\n",LayerID);
		return HD_ERR_NOT_SUPPORT;
	}
#endif

    if(cLayerID>LAYER_OSD1){
        DBG_ERR("not sup 0x%x\r\n",LayerID);
        return HD_ERR_NOT_SUPPORT;
    }

    //assign bf buffer to FB_INIT
    fb_init.fb_id = fb_map[cLayerID];
    fb_init.pa_addr = uiBufAddr;
    fb_init.buf_len = w*h*_DL_fmt_coef(PxlFmt);
	DBG_IND("uiBufAddr 0x%x buf_len 0x%x %d\r\n",uiBufAddr,fb_init.buf_len,_DL_fmt_coef(PxlFmt));
    ret = hd_common_mem_flush_cache((void *)fb_init.pa_addr, fb_init.buf_len);
    if(ret!=HD_OK){
        DBG_ERR("flush %d\r\n",ret);
        return ret;
    }
#if defined(__FREERTOS)
	ret = vendor_videoout_set(VENDOR_VIDEOOUT_ID0, VENDOR_VIDEOOUT_ITEM_FB_INIT, &fb_init);
#else
    return ret;
#endif

	return ret;
}

INT32 _DL_BufferInit(UINT32 LayerID,
					UINT32 w, UINT32 h, UINT32 PxlFmt, UINT32 uiBufAddr, IRECT *pWin, UINT32 uiWinAttr)
{
	HD_RESULT ret = HD_OK;
	UINT32 cDevID = _DD(LayerID);
	UINT32 cLayerID = _DL(LayerID);
	HD_FB_FMT video_out_fmt={0};
	HD_FB_DIM video_out_dim={0};
    HD_FB_ID fb_id = 0;
    HD_PATH_ID video_out_ctrl= 0;

	DBG_IND("w=%d,h=%d,PxlFmt=0x%x,uiBufAddr=0x%x\r\n",w,h,PxlFmt,uiBufAddr);
#if (DEVICE_COUNT < 2)
	if (cDevID >= 1) {
        DBG_ERR("not sup 0x%x\r\n",LayerID);
		return HD_ERR_NOT_SUPPORT;
	}
#endif

    if(cLayerID>LAYER_OSD1){
        DBG_ERR("not sup 0x%x\r\n",LayerID);
        return HD_ERR_NOT_SUPPORT;
    }

    fb_id = fb_map[cLayerID];
    video_out_ctrl = GxVideo_GetDeviceCtrl(cDevID,DISPLAY_DEVCTRL_CTRLPATH);

	video_out_fmt.fb_id = fb_id;
	video_out_fmt.fmt = _DL_fmt_hdal(PxlFmt);

	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_FMT, &video_out_fmt);
	if(ret!= HD_OK)
		return ret;
	video_out_dim.fb_id = fb_id;
	video_out_dim.input_dim.w = w;
	video_out_dim.input_dim.h = h;
	video_out_dim.output_rect.x = pWin->x;
	video_out_dim.output_rect.y = pWin->y;
	video_out_dim.output_rect.w = pWin->w;
	video_out_dim.output_rect.h = pWin->h;

	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_DIM, &video_out_dim);
	if(ret!= HD_OK)
		return ret;

    return _DL_BufferSwitch(LayerID,w,h,PxlFmt,uiBufAddr);
}

INT32 _DL_SetEnable(UINT32 LayerID, BOOL bEnable)
{
    HD_RESULT ret;
	UINT32 cDevID = _DD(LayerID);
	UINT32 cLayerID = _DL(LayerID);
	HD_FB_ENABLE video_out_enable = {0};
    HD_FB_ID fb_id = 0;
    HD_PATH_ID video_out_ctrl = 0;

	DBG_IND("LayerID 0x%x,bEnable 0x%x\r\n",LayerID,bEnable);
#if (DEVICE_COUNT < 2)
	if (cDevID >= 1) {
        DBG_ERR("not sup 0x%x\r\n",LayerID);
		return HD_ERR_NOT_SUPPORT;
	}
#endif

    if(cLayerID>LAYER_OSD1){
        DBG_ERR("not sup 0x%x\r\n",LayerID);
        return HD_ERR_NOT_SUPPORT;
    }

    fb_id = fb_map[cLayerID];
    video_out_ctrl = GxVideo_GetDeviceCtrl(cDevID,DISPLAY_DEVCTRL_CTRLPATH);

	video_out_enable.fb_id = fb_id;
	video_out_enable.enable = bEnable;

	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_ENABLE, &video_out_enable);

    return ret;
}
BOOL _DL_GetEnable(UINT32 LayerID)
{
    HD_RESULT ret;
	UINT32 cDevID = _DD(LayerID);
	UINT32 cLayerID = _DL(LayerID);
	HD_FB_ENABLE video_out_enable = {0};
    HD_FB_ID fb_id = 0;
    HD_PATH_ID video_out_ctrl = 0;

	DBG_IND("LayerID 0x%x \r\n",LayerID);
#if (DEVICE_COUNT < 2)
	if (cDevID >= 1) {
        DBG_ERR("not sup 0x%x\r\n",LayerID);
		return HD_ERR_NOT_SUPPORT;
	}
#endif

    if(cLayerID>LAYER_OSD1){
        DBG_ERR("not sup 0x%x\r\n",LayerID);
        return HD_ERR_NOT_SUPPORT;
    }
    fb_id = fb_map[cLayerID];
    video_out_ctrl = GxVideo_GetDeviceCtrl(cDevID,DISPLAY_DEVCTRL_CTRLPATH);

	video_out_enable.fb_id = fb_id;

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_ENABLE, &video_out_enable);
    if(ret!= HD_OK){
        DBG_ERR("ret 0x%x\r\n",ret);
        return 0;
    }
    return video_out_enable.enable;
}

INT32 _DL_SetPalette(UINT32 LayerID, UINT32 nStart, UINT32 nCount, UINT32 *pData)
{
	HD_RESULT ret = HD_OK;
	UINT32 cDevID = _DD(LayerID);
	UINT32 cLayerID = _DL(LayerID);
	HD_FB_PALETTE_TBL video_out_palette={0};
    HD_FB_ID fb_id = 0;
    HD_PATH_ID video_out_ctrl = 0;
	DBG_IND("\r\n");
#if (DEVICE_COUNT < 2)
	if (cDevID >= 1) {
        DBG_ERR("not sup 0x%x\r\n",LayerID);
		return HD_ERR_NOT_SUPPORT;
	}
#endif

    if(cLayerID>LAYER_OSD1){
        DBG_ERR("not sup 0x%x\r\n",LayerID);
        return HD_ERR_NOT_SUPPORT;
    }
    fb_id = fb_map[cLayerID];
    video_out_ctrl = GxVideo_GetDeviceCtrl(cDevID,DISPLAY_DEVCTRL_CTRLPATH);

	video_out_palette.fb_id = fb_id;
	video_out_palette.table_size = nCount;
	video_out_palette.p_table = pData;

	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_PALETTE_TABLE, &video_out_palette);

    return ret;
}
INT32 _DL_GetPalette(UINT32 LayerID, UINT32 nStart, UINT32 nCount, UINT32 *pData)
{
	HD_RESULT ret = HD_OK;
	UINT32 cDevID = _DD(LayerID);
	UINT32 cLayerID = _DL(LayerID);
	HD_FB_PALETTE_TBL video_out_palette={0};
    HD_FB_ID fb_id = 0;
    HD_PATH_ID video_out_ctrl = 0;
	DBG_IND("\r\n");
#if (DEVICE_COUNT < 2)
	if (cDevID >= 1) {
        DBG_ERR("not sup 0x%x\r\n",LayerID);
		return HD_ERR_NOT_SUPPORT;
	}
#endif

    if(cLayerID>LAYER_OSD1){
        DBG_ERR("not sup 0x%x\r\n",LayerID);
        return HD_ERR_NOT_SUPPORT;
    }
    fb_id = fb_map[cLayerID];
    video_out_ctrl = GxVideo_GetDeviceCtrl(cDevID,DISPLAY_DEVCTRL_CTRLPATH);

	video_out_palette.fb_id = fb_id;
	video_out_palette.table_size = nCount;
	video_out_palette.p_table = pData;  //should have buf,now is pthisDM->Pal

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_PALETTE_TABLE, &video_out_palette);

    return ret;
}


INT32 _DL_SetColorKey(UINT32 LayerID, UINT32 PxlFmt,UINT32 KeyOp,UINT32 ColorKey)
{
	HD_RESULT ret = HD_OK;
	UINT32 cDevID = _DD(LayerID);
	UINT32 cLayerID = _DL(LayerID);
	HD_FB_ATTR video_out_attr={0};
    HD_FB_ID fb_id = 0;
    HD_PATH_ID video_out_ctrl = 0;

    DBG_IND("KeyOp 0x%x ColorKey 0x%x\r\n",KeyOp,ColorKey);

#if (DEVICE_COUNT < 2)
	if (cDevID >= 1) {
        DBG_ERR("not sup 0x%x\r\n",LayerID);
		return HD_ERR_NOT_SUPPORT;
	}
#endif

    if(cLayerID>LAYER_OSD1){
        DBG_ERR("not sup 0x%x\r\n",LayerID);
        return HD_ERR_NOT_SUPPORT;
    }

    fb_id = fb_map[cLayerID];
    video_out_ctrl = GxVideo_GetDeviceCtrl(cDevID,DISPLAY_DEVCTRL_CTRLPATH);

	video_out_attr.fb_id = fb_id;
    //in hdal OSD is source alpha,not global,those are global
	video_out_attr.alpha_blend = 0;
    video_out_attr.alpha_1555 = 0;
	video_out_attr.colorkey_en = KeyOp;
	video_out_attr.r_ckey = ColorKey&0xFF0000;
	video_out_attr.g_ckey = ColorKey&0x00FF00;
	video_out_attr.b_ckey = ColorKey&0x0000FF;

	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_ATTR, &video_out_attr);
	return ret;

}
extern UINT32 SysMain_GetTempBuffer(UINT32 uiSize);
extern UINT32 SysMain_RelTempBuffer(UINT32 addr);

INT32 _DL_DumpBuf(UINT32 LayerID)
{
	HD_RESULT ret = HD_OK;
	UINT32 cDevID = _DD(LayerID);
	UINT32 cLayerID = _DL(LayerID);
    VENDOR_FB_INIT fb_init;
    UINT32 addr=0,size=0;
	char filename[64] = "A:\\ideXo1b0.raw";
#if (DEVICE_COUNT < 2)
	if (cDevID >= 1) {
        DBG_ERR("not sup 0x%x\r\n",LayerID);
	}
#endif

    if(cLayerID>LAYER_OSD1){
        DBG_ERR("not sup 0x%x\r\n",LayerID);
    }

    fb_init.fb_id = fb_map[cLayerID];
	ret = vendor_videoout_get(VENDOR_VIDEOOUT_ID0, VENDOR_VIDEOOUT_ITEM_FB_INIT, &fb_init);
    if(ret != HD_OK){
        DBG_ERR("ret %d\r\n",ret);
        return ret;
    }

    DBG_IND("%x %x\n",fb_init.pa_addr,fb_init.buf_len);

    addr = SysMain_GetTempBuffer(fb_init.buf_len);
    size = fb_init.buf_len;
    if((addr!=0)&&(size!=0)){
	    FST_FILE filehdl = NULL;
		filename[7] = 'o';
		filename[8] = '1'+(cLayerID-LAYER_OSD1);
        DBG_DUMP("filename %s\r\n",filename);
        memset((void *)addr,0,fb_init.buf_len);
        memcpy((void *)addr,(void *)fb_init.pa_addr,fb_init.buf_len);
        ret = hd_common_mem_flush_cache((void *)addr, fb_init.buf_len);
        if(ret!=HD_OK){
            DBG_ERR("flush %d\r\n",ret);
            goto exit;
        }

        filehdl = FileSys_OpenFile(filename, FST_CREATE_ALWAYS | FST_OPEN_WRITE);

        ret=FileSys_WriteFile(filehdl,(UINT8 *)addr, &size, 0, NULL);
        if(ret!=FST_STA_OK){
            DBG_ERR("write ret %d\r\n",ret);
        }
		FileSys_CloseFile(filehdl);
    }
exit:
    ret = SysMain_RelTempBuffer(addr);
    if(ret!=HD_OK){
        DBG_ERR("rel %d\r\n",ret);
    }
    return ret;
}
