#include <stdio.h>
#include "ImageApp/ImageApp_Play.h"
#include "PlaybackTsk.h"
#include <string.h>
#include "gximage/gximage.h"
#include "hd_type.h"
#include "kflow_common/nvtmpp.h"
#include "GxVideoFile.h"
#include <kwrap/util.h>
#include "vendor_common.h"
#include "vendor_videoout.h"

PLAY_DISP_INFO  g_play_disp_info[4] = {0};
static UINT32 g_u32max_disp_width      = 1920;
static UINT32 g_u32max_disp_height     = 1080;
static UINT32 g_u32max_file_size  = 0x500000; //Default
static UINT32 g_u32max_raw_size   = 2952*1922*2; //YUV422 UV packed
static UINT32 g_u32max_decode_w   = 2592;
static UINT32 g_u32max_decode_h   = 1944;
static UINT32 g_u32play_file_fmt       = PBFMT_JPG | PBFMT_WAV | PBFMT_AVI | PBFMT_MOVMJPG | PBFMT_MP4 | PBFMT_TS;
static UINT32 g_u32max_vid_width  = 1920;
static UINT32 g_u32max_vid_height = 1080;
static BOOL   g_bDecode_vid       = TRUE;
static BOOL   g_bSuspend_close    = FALSE;
static BOOL   g_bDecode_sar	     = TRUE;
static HD_COMMON_MEM_INIT_CONFIG *g_pPlay_MemCfg = NULL;
static HD_VIDEO_PXLFMT g_voout_pix_fmt = HD_VIDEO_PXLFMT_YUV420;
static IMAGEAPP_PLAY_STREAM g_PlayHDStream = {0}; 
static PLAY_FILE_DECRYPT g_decrypt_info = {0};
IMAGEAPP_PLAY_STREAM *g_pPlayHDStream = &g_PlayHDStream;

#if _TODO
static ER _ImageApp_Play_ConfigDisp(PLAY_DISP_INFO *p_disp_info)
{
	if (p_disp_info == NULL) {
		return E_SYS;
	}

	if (p_disp_info->disp_id >= PLAY_DISP_ID_MAX) {
		return E_SYS;
	}
	g_play_disp_info[p_disp_info->disp_id] = *p_disp_info;
	
	return E_OK;
}
#endif

static HD_RESULT ImageApp_Play_set_dec_cfg(HD_PATH_ID video_dec_path, HD_DIM *p_max_dim, UINT32 dec_type)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEODEC_PATH_CONFIG video_path_cfg = {0};
	//HD_VIDEODEC_IN video_in_param = {0};

	if (p_max_dim != NULL) {
		// set videodec path config
		video_path_cfg.max_mem.codec_type = dec_type;
		video_path_cfg.max_mem.dim.w = p_max_dim->w;
		video_path_cfg.max_mem.dim.h = p_max_dim->h;
		ret = hd_videodec_set(video_dec_path, HD_VIDEODEC_PARAM_PATH_CONFIG, &video_path_cfg);
	} else {
		ret = HD_ERR_NG;
	}

	//--- HD_VIDEODEC_PARAM_IN ---
#if 0	
	video_in_param.codec_type = dec_type;
	if (dec_type == HD_CODEC_TYPE_JPEG){
		video_in_param.width = ALIGN_CEIL_64(g_u32max_decode_w);
		video_in_param.height = ALIGN_CEIL_64(g_u32max_decode_h);
	}else if (dec_type == HD_CODEC_TYPE_H264){
		video_in_param.width = ALIGN_CEIL_64(g_u32max_vid_width);
		video_in_param.height = ALIGN_CEIL_16(g_u32max_vid_height);	
	}else if (dec_type == HD_CODEC_TYPE_H265){
		video_in_param.width = ALIGN_CEIL_64(g_u32max_vid_width);
		video_in_param.height = ALIGN_CEIL_64(g_u32max_vid_height);	
	}
	ret = hd_videodec_set(video_dec_path, HD_VIDEODEC_PARAM_IN, &video_in_param);
#endif

	return ret;
}

static void ImageApp_Play_TriggerCB(PVDO_FRAME pimg_buf, BOOL trigger_draw)
{
	if (trigger_draw) {
		PB_SetPBFlag(PB_SET_FLG_TRIGGER);
	}
}

static HD_RESULT ImageApp_Play_Init_Mem(void)
{
    HD_RESULT ret;

	if (g_pPlay_MemCfg == NULL){
		DBG_ERR("No memory cfg\r\n");
		return E_SYS;
	}

	if ((ret = vendor_common_mem_relayout(g_pPlay_MemCfg))!= HD_OK)
	{
		DBG_ERR("vendor_common_mem_relayout fail(%d)\r\n", ret);
		return E_SYS;
	}

	return E_OK;
}


static ER ImageApp_Play_Uninit_Mem(void)
{
	return HD_OK;
}

static ER ImageApp_Play_Init_Module(void)
{
    HD_RESULT ret;
	
	if ((ret = hd_videodec_init()) != HD_OK)
		return E_SYS;

	return E_OK;

}

static HD_RESULT ImageApp_Play_Open_Module(void)
{
	HD_RESULT ret;

	if ((ret = hd_videodec_open(HD_VIDEODEC_0_IN_0, HD_VIDEODEC_0_OUT_0, &g_PlayHDStream.vdec_path[0])) != HD_OK)
		return ret;

	if ((ret = hd_videodec_open(HD_VIDEODEC_0_IN_1, HD_VIDEODEC_0_OUT_1, &g_PlayHDStream.vdec_path[1])) != HD_OK)
		return ret;

	if ((ret = hd_videodec_open(HD_VIDEODEC_0_IN_2, HD_VIDEODEC_0_OUT_2, &g_PlayHDStream.vdec_path[2])) != HD_OK)
		return ret;


	return HD_OK;
}

static HD_RESULT ImageApp_Play_Close_Module(void)
{
	HD_RESULT ret;
	UINT32 i;
	VENDOR_VIDEOOUT_FUNC_CONFIG videoout_cfg ={0};
	
	for (i = 0; i < 3; i++) {
    	// stop video_playback modules (main)
    	if ((ret = hd_videodec_stop(g_pPlayHDStream->vdec_path[i])) != HD_OK) {
    		DBG_ERR("hd_videodec_stop fail=%d\n", ret);
    		return ret;
    	}
    
    	if ((ret = hd_videodec_close(g_pPlayHDStream->vdec_path[i])) != HD_OK) {
    		DBG_ERR("hd_videodec_close fail=%d\n", ret);
    		return ret;
    	}
    }

    //set vout stop would keep last frame for change mode
    videoout_cfg.in_func = VENDOR_VIDEOOUT_INFUNC_KEEP_LAST;
	ret = vendor_videoout_set(g_pPlayHDStream->vout_path, VENDOR_VIDEOOUT_ITEM_FUNC_CONFIG, &videoout_cfg);
	if(ret != HD_OK){
		DBG_ERR("vendor_videoout_set fail=%d\n", ret);
		return ret;
	}

    ret = hd_videoout_stop(g_pPlayHDStream->vout_path);
	if ((ret != HD_OK) && (ret != HD_ERR_NOT_OPEN) && (ret != HD_ERR_NOT_START)) {
    		DBG_ERR("hd_videoout_stop fail=%d\n", ret);
    		return ret;
    	}

	return HD_OK;
}

static HD_RESULT ImageApp_Play_Uninit_Module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videodec_uninit()) != HD_OK) {
		DBG_ERR("hd_videodec_uninit fail=%d\n", ret);
		return ret;
	}

	return HD_OK;
}

ER ImageApp_Play_Open(void)
{
	//UINT32 size = 0;
	//UINT32 panel_size = g_u32max_disp_width * g_u32max_disp_height;
	UINT32 dec_1st_buffer_needed;	
	PLAY_OBJ PlayObj = {0};
	HD_RESULT ret;


	if (ImageApp_Play_Init_Mem() != E_OK){
		return E_SYS;
	}

	if (ImageApp_Play_Init_Module() != E_OK){
		return E_SYS;
	}	

	if (ImageApp_Play_Open_Module() != E_OK){
		return E_SYS;
	}	

   	// set videodec config (main)
	g_pPlayHDStream->vdec_type = HD_CODEC_TYPE_JPEG;
	g_pPlayHDStream->vdec_max_dim.w = g_u32max_decode_w;
	g_pPlayHDStream->vdec_max_dim.h = g_u32max_decode_h;
	ret = ImageApp_Play_set_dec_cfg(g_pPlayHDStream->vdec_path[0], &g_pPlayHDStream->vdec_max_dim, g_pPlayHDStream->vdec_type);
	if (ret != HD_OK) {
		DBG_ERR("set JPG dec-cfg fail=%d\n", ret);
		return E_SYS;
	}

	g_pPlayHDStream->vdec_type = HD_CODEC_TYPE_H264;
	g_pPlayHDStream->vdec_max_dim.w = ALIGN_CEIL_64(g_u32max_vid_width); // This must be set according to the real size for H264.
	g_pPlayHDStream->vdec_max_dim.h = ALIGN_CEIL_16(g_u32max_vid_height); // This must be set according to the real size for H264.
	ret = ImageApp_Play_set_dec_cfg(g_pPlayHDStream->vdec_path[1], &g_pPlayHDStream->vdec_max_dim, g_pPlayHDStream->vdec_type);
	if (ret != HD_OK) {
		DBG_ERR("set H264 dec-cfg fail=%d\n", ret);
		return E_SYS;
	}	

	g_pPlayHDStream->vdec_type = HD_CODEC_TYPE_H265;
	g_pPlayHDStream->vdec_max_dim.w = ALIGN_CEIL_64(g_u32max_vid_width); // This must be set according to the real size for H265.
	g_pPlayHDStream->vdec_max_dim.h = ALIGN_CEIL_64(g_u32max_vid_height); // This must be set according to the real size for H265.
	ret = ImageApp_Play_set_dec_cfg(g_pPlayHDStream->vdec_path[2], &g_pPlayHDStream->vdec_max_dim, g_pPlayHDStream->vdec_type);
	if (ret != HD_OK) {
		DBG_ERR("set H265 dec-cfg fail=%d\n", ret);
		return E_SYS;
	}

	// start video_playback modules (main)
	DBG_DUMP("%s: g_pPlayHDStream->vdec_path[0] = 0x%x\r\n", __func__, g_pPlayHDStream->vdec_path[0]);
	DBG_DUMP("%s: g_pPlayHDStream->vdec_path[1] = 0x%x\r\n", __func__, g_pPlayHDStream->vdec_path[1]);
	DBG_DUMP("%s: g_pPlayHDStream->vdec_path[2] = 0x%x\r\n", __func__, g_pPlayHDStream->vdec_path[2]);
	hd_videodec_start(g_pPlayHDStream->vdec_path[0]);
	hd_videodec_start(g_pPlayHDStream->vdec_path[1]);
	hd_videodec_start(g_pPlayHDStream->vdec_path[2]);

	if (g_bDecode_vid) {
		GxVidFile_SetParam(GXVIDFILE_PARAM_MAX_W, g_u32max_vid_width);
		GxVidFile_SetParam(GXVIDFILE_PARAM_MAX_H, g_u32max_vid_height);

		GxVidFile_Query1stFrameWkBufSize(&dec_1st_buffer_needed, 0);
		#if 0
		if (g_u32max_disp_width > 1920 || g_u32max_disp_width > 1440) {
			size = dec_1st_buffer_needed + panel_size * 4 + panel_size * 2;
		} else {
			size = dec_1st_buffer_needed + panel_size * 4 + panel_size * 4 * 2;
		}
		#endif
	} 
	
	// init the Playback object
	memset(&PlayObj, 0, sizeof(PLAY_OBJ));

	//PlayObj.uiMemoryAddr  = 0; //reserve, useing the HDAL common buffer
	//PlayObj.uiMemorySize  = 0; //reserve, useing the HDAL common buffer
	PlayObj.uiPlayFileFmt = g_u32play_file_fmt;
	PB_SetParam(PBPRMID_DISP_TRIG_CALLBACK, (UINT32)ImageApp_Play_TriggerCB);
	PB_SetParam(PBPRMID_MAX_PANELSZ, g_u32max_disp_width * g_u32max_disp_height);
	PB_SetParam(PBPRMID_MAX_FILE_SIZE, g_u32max_file_size);
	PB_SetParam(PBPRMID_MAX_RAW_SIZE, g_u32max_raw_size);
	PB_SetParam(PBPRMID_MAX_DECODE_WIDTH, g_u32max_decode_w);
	PB_SetParam(PBPRMID_MAX_DECODE_HEIGHT, g_u32max_decode_h);
	PB_SetParam(PBPRMID_PIXEL_FORMAT, g_voout_pix_fmt);
	PB_SetParam(PBPRMID_HD_VIDEODEC_PATH, (UINT32)g_pPlayHDStream->vdec_path);

	if (!g_bSuspend_close) {
		// Open playback task
		if (PB_Open(&PlayObj) != PBERR_OK) {
			DBG_ERR("Error open playback task\r\n");
		}
		DBG_DUMP("[ImageAPP_Play]Open End\r\n");
	} else {
		PB_SetParam(PBPRMID_PLAYBACK_OBJ, (UINT32)&PlayObj);
	}

	return E_OK;

}

ER ImageApp_Play_Close(void)
{
    HD_RESULT ret;
	
	if (!g_bSuspend_close) {
		if (PBERR_OK != PB_Close(PB_WAIT_INFINITE)) {
			DBG_ERR("Playback already Closed\r\n");
			return E_SYS;
		}
		DBG_DUMP("[ImageAPP_Play]Close\r\n");
	}

    ret = ImageApp_Play_Close_Module();
	if (ret!= HD_OK){
		DBG_ERR("Close Module failed\r\n");
		return ret;
	}

    ret = ImageApp_Play_Uninit_Module();
	if (ret!= HD_OK){
		DBG_ERR("Uninit Module failed\r\n");
		return ret;
	}

	ImageApp_Play_Uninit_Mem();

    memset(&g_PlayHDStream, 0, sizeof(IMAGEAPP_PLAY_STREAM));   

	return E_OK;
}


ER ImageApp_Play_SetParam(UINT32 param, UINT32 value)
{
	if (param < PLAY_PARAM_START) {
		return E_SYS;
	}
	
	switch (param) {
	case PLAY_PARAM_PANELSZ:
		{
			PUSIZE size = (PUSIZE)value;

			g_u32max_disp_width = size->w;
			g_u32max_disp_height = size->h;
		}
		break;

	case PLAY_PARAM_MAX_RAW_SIZE:
		g_u32max_raw_size = value;
		break;

	case PLAY_PARAM_MAX_DECODE_WIDTH:
		g_u32max_decode_w = value;
		break;

	case PLAY_PARAM_MAX_DECODE_HEIGHT:
		g_u32max_decode_h = value;
		break;		

	case PLAY_PARAM_MAX_FILE_SIZE:
		g_u32max_file_size = value;
		break;

	case PLAY_PARAM_DEC_VID:
		g_bDecode_vid = value;
		break;

	case PLAY_PARAM_DUAL_DISP:
		break;

	case PLAY_PARAM_PLAY_FMT:
		g_u32play_file_fmt = value;
		break;

	case PLAY_PARAM_SUSPEND_CLOSE_IMM:
		g_bSuspend_close = value;
		break;
		
	case PLAY_PARAM_DEC_SAR:
		g_bDecode_sar = value;		
		break;
		
	case PLAY_PARAM_MAX_VIDEO_SIZE:
		{
			PUSIZE size = (PUSIZE)value;

			g_u32max_vid_width  = size->w;
			g_u32max_vid_height = size->h;			
		}
	    break;

    case PLAY_PARAM_MEM_CFG:
		g_pPlay_MemCfg = (HD_COMMON_MEM_INIT_CONFIG *)value;
		break;

		
	case PLAY_PARAM_DISP_INFO:
		//_ImageApp_Play_ConfigDisp((PLAY_DISP_INFO *) value);
					// VdoOut flow refine: pass vdoout path id to ImageApp
		if (value != 0) {
			IMAGEAPP_PLAY_CFG_DISP_INFO *ptr = (IMAGEAPP_PLAY_CFG_DISP_INFO*) value;
			g_PlayHDStream.vout_ctrl = ptr->vout_ctrl;
			g_PlayHDStream.vout_path = ptr->vout_path;
		}
		break;

    case PLAY_PARAM_PIXEL_FORMAT:
		g_voout_pix_fmt = (HD_VIDEO_PXLFMT)value;
		break;
		
    case PLAY_PARAM_DECRYPT_INFO:
		if (value) {
			PLAY_FILE_DECRYPT *ptr = (PLAY_FILE_DECRYPT *)value;
			g_decrypt_info = *ptr;

			if(g_decrypt_info.type == PLAY_DECRYPT_TYPE_NONE){
				PB_SetParam(PBPRMID_VID_DECRYPT_KEY, 0);
			}
			else{
				PB_SetParam(PBPRMID_VID_DECRYPT_KEY, (UINT32)g_decrypt_info.key);
				PB_SetParam(PBPRMID_VID_DECRYPT_MODE, g_decrypt_info.mode);
				PB_SetParam(PBPRMID_VID_DECRYPT_POS, g_decrypt_info.type);
			}
		}
		break;

	default:
		DBG_ERR("param[%08x] = %d\r\n", param, value);
		break;
	}
	
	return E_OK;
}

UINT32 ImageApp_Play_GSetParam(UINT32 param)
{
	UINT32 ret = 0;
	if (param < PLAY_PARAM_START) {
		return ret;
	}
	switch (param) {
	case PLAY_PARAM_POOL_ADDRESS:
		break;
		
	default:
		DBG_ERR("param[%08x]\r\n", param);
		break;
	}

	return ret;
}

