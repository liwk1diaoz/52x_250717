#include "ImageApp/ImageApp_Photo.h"
#include "ImageApp_Photo_int.h"
//#include "ImageUnit_VdoEnc.h"
//#include "ImageUnit_ImgTrans.h"
//#include "ImageUnit_UserProc.h"
//#include "ImageUnit_Dummy.h"
//#include "ImageUnit_NetHTTP.h"
//#include "ImageUnit_StreamSender.h"
//#include "NMediaRecVdoEnc.h"
//#include "ImageUnit_VdoIn.h"
//#include "NMediaRecImgCap.h"
#include "Utility/SwTimer.h"
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <kwrap/perf.h>
#include <kwrap/util.h>

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          IA_Photo_WFiLink
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////// //////////////////////////////////////////////////////////
#include "../Common/ImageApp_Common_int.h"

#define FLGPHOTO_WIFI_AEPULL_START              	FLGPTN_BIT(0)       //0x00000001
#define FLGPHOTO_WIFI_AEPULL_IDLE              	FLGPTN_BIT(1)       //0x00000002

PHOTO_STRM_CBR_INFO  g_photo_strm_cbr_info[(PHOTO_STRM_ID_MAX-PHOTO_STRM_ID_MIN)] = {0};
PHOTO_DRAW_CB *g_fn_photo_drawcb= NULL;
/*
static SWTIMER_ID PhotoWiFiSwTimerID=0;
static PHOTO_WIFI_PIP_PROCESS_CB  g_WiFiCb = NULL;
static UINT16 g_uiWiFiLinkStream=0;
static BOOL g_bWifiLinkOpened=0;
static BOOL g_bWiFiLinkWifiStart=0;
*/

static UINT32 g_photo_IsWifiLinkOpened[(PHOTO_STRM_ID_MAX-PHOTO_STRM_ID_MIN)] = {0};
static THREAD_HANDLE g_photo_wifi_tsk_id;
static UINT32 g_photo_wifi_tsk_run[(PHOTO_STRM_ID_MAX-PHOTO_STRM_ID_MIN)], g_photo_is_wifi_tsk_running[(PHOTO_STRM_ID_MAX-PHOTO_STRM_ID_MIN)];
static PHOTO_PROCESS_CB  g_PhotoWifiCb = NULL; //vproc pull data and push in venc

static THREAD_HANDLE g_photo_wifi_aepull_tsk_id;
static UINT32 g_photo_wifi_aepull_tsk_run[(PHOTO_STRM_ID_MAX-PHOTO_STRM_ID_MIN)], g_photo_is_wifi_aepull_tsk_running[(PHOTO_STRM_ID_MAX-PHOTO_STRM_ID_MIN)];
static PHOTO_PROCESS_CB  g_PhotoWifiAePullCb = NULL; //venc pull data and push in rtsp
static ID g_photo_wifi_aepull_flg_id = 0;

static UINT32 g_photo_is_wifi_showpreview=1;
static UINT32 g_photo_is_wificb_executing= 0;
static UINT32 g_photo_is_vepullcb_executing= 0;

HD_VIDEOENC_IN  photo_wifi_venc_in_param = {0};
HD_VIDEOENC_OUT2 photo_wifi_venc_out_param = {0};
HD_H26XENC_RATE_CONTROL2 photo_wifi_venc_rc_param = {0};


#define PRI_IMAGEAPP_PHOTO_WIFI             8
#define STKSIZE_IMAGEAPP_PHOTO_WIFI         2048*3

#define PRI_IMAGEAPP_PHOTO_WIFI_AEPULL             10
#define STKSIZE_IMAGEAPP_PHOTO_WIFI_AEPULL         2048

void _ImageApp_Photo_NMI_CB(CHAR *Name, UINT32 uiEventID, UINT32 param);
BOOL _ImageApp_Photo_open_http_cb(MEM_RANGE *open_parm);
void _ImageApp_Photo_SetVideoMaxBuf(UINT32 OutPortIndex, PHOTO_STRM_INFO *p_strm_info);
void _ImageApp_Photo_AllocImgCapMaxBuf(PHOTO_STRM_INFO *p_strm_info);
void _ImageApp_Photo_InitWiFiLink(PHOTO_STRM_INFO *p_strm_info);
INT32 _ImageApp_Photo_ConfigStrmCbr(PHOTO_STRM_CBR_INFO *p_strm_cbr_info);
void PhotoWiFi_SwTimerHdl(UINT32 uiEvent);
void PhotoWiFi_SwTimerOpen(void);
void PhotoWiFi_SwTimerClose(void);


UINT32 ImageApp_Photo_WiFi_IsPreviewShow()
{
	return g_photo_is_wifi_showpreview;
}
void ImageApp_Photo_WiFi_SetPreviewShow(UINT32 IsShow)
{
	UINT32 delay_cnt;
	DBG_IND("SetPreviewShow=%d\r\n",IsShow);
	#if 1
	g_photo_is_wifi_showpreview=IsShow;
	if(IsShow==0){
		delay_cnt = 10;
		while (g_photo_is_wificb_executing==1 && delay_cnt) {
			vos_util_delay_ms(50);
			delay_cnt --;
		}
		//DBG_ERR("g_photo_is_wificb_executing=%d\r\n",g_photo_is_wificb_executing);

		delay_cnt = 10;
		while (g_photo_is_vepullcb_executing==1 && delay_cnt) {
			vos_util_delay_ms(50);
			delay_cnt --;
		}
		//DBG_ERR("g_photo_is_vepullcb_executing=%d\r\n",g_photo_is_vepullcb_executing);
	}
	#endif
}


void _ImageApp_Photo_NMI_CB(CHAR *Name, UINT32 uiEventID, UINT32 param)
{
	/*
	if (strcmp(Name, "VdoEnc") == 0) {
		switch (uiEventID) {
		case NMI_VDOENC_EVENT_STAMP_CB: {
			MP_VDOENC_YUV_SRC* p_yuv_src   = NULL;
			p_yuv_src = (MP_VDOENC_YUV_SRC*)param;
			if (g_fn_photo_drawcb) {
				g_fn_photo_drawcb(p_yuv_src->uiYAddr, p_yuv_src->uiCbAddr,p_yuv_src->uiCrAddr,p_yuv_src->uiYLineOffset );
			}

			}
			break;
		default:
			break;
		}
	}
	*/
}
BOOL _ImageApp_Photo_open_http_cb(MEM_RANGE *open_parm)
{
	/*
	if (!open_parm) {
		DBG_ERR("open_parm is NULL");
		return FALSE;
	}
	open_parm->bitstream_mem_range.Addr = ImageUnit_GetParam(&ISF_VdoEnc, ISF_OUT1, VDOENC_PARAM_ENCBUF_ADDR);
	open_parm->bitstream_mem_range.Size = ImageUnit_GetParam(&ISF_VdoEnc, ISF_OUT1, VDOENC_PARAM_ENCBUF_SIZE);
	DBG_IND("http bitstream range addr = 0x%x ,size = 0x%x\r\n",open_parm->bitstream_mem_range.Addr,open_parm->bitstream_mem_range.Size);
	*/
	return TRUE;
}
void _ImageApp_Photo_SetVideoMaxBuf(UINT32 OutPortIndex, PHOTO_STRM_INFO *p_strm_info)
{
	/*
	NMI_VDOENC_MAX_MEM_INFO MaxMemInfo = {0};
	MaxMemInfo.uiCodec          = MEDIAVIDENC_H265;
	MaxMemInfo.uiWidth          = p_strm_info->max_width;
	MaxMemInfo.uiHeight         = p_strm_info->max_height;
	MaxMemInfo.uiTargetByterate = p_strm_info->max_bitrate;
	MaxMemInfo.uiEncBufMs       = 1500;
	MaxMemInfo.uiRecFormat      = MEDIAREC_LIVEVIEW;
	MaxMemInfo.uiSVCLayer       = MP_VDOENC_SVC_LAYER2;
	MaxMemInfo.uiLTRInterval	= 1;
	MaxMemInfo.uiRotate         = 0;
	MaxMemInfo.bAllocSnapshotBuf= 0;
	ImageUnit_SetParam(OutPortIndex, VDOENC_PARAM_MAX_MEM_INFO, (UINT32) &MaxMemInfo);
	*/
}
void _ImageApp_Photo_AllocImgCapMaxBuf(PHOTO_STRM_INFO *p_strm_info)
{
	/*
	NMI_IMGCAP_MAX_MEM_INFO ImgCapMaxInfo = {0};

	ImgCapMaxInfo.bRelease = 0;
	ImgCapMaxInfo.uiWidth = 16;
	ImgCapMaxInfo.uiHeight = 16;
	ImageUnit_Begin(&ISF_VdoEnc, 0);
	ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_IMGCAP_MAX_MEM_INFO, (UINT32)&ImgCapMaxInfo);
	ImageUnit_End();
	*/
}
void _ImageApp_Photo_InitWiFiLink(PHOTO_STRM_INFO *p_strm_info)
{
	/*
	ISF_UNIT  *pStrmUnit;

    	if (p_strm_info == NULL) {
		return;
	}

	if (p_strm_info->strm_id >= PHOTO_STRM_ID_MAX) {
		DBG_ERR("strm_id  =%d\r\n",p_strm_info->strm_id);
		return;
	}
	g_uiWiFiLinkStream = PHOTO_VID_OUT_MAX +  PHOTO_VID_IN_MAX;
    	//open
	ImageStream_Open(&ISF_Stream[ g_uiWiFiLinkStream]);

	ImageStream_Begin(&ISF_Stream[g_uiWiFiLinkStream], 0);

	if (p_strm_info->strm_type == PHOTO_STRM_TYPE_HTTP)
		pStrmUnit = &ISF_NetHTTP;
	else
		pStrmUnit = &ISF_StreamSender;//&ISF_NetRTSP;

	_ImageApp_Photo_AllocImgCapMaxBuf(p_strm_info);

	ImageUnit_Begin(&ISF_Dummy, 0);
	ImageUnit_SetParam(ISF_IN1 +  PHOTO_IME3DNR_ID_MAX + PHOTO_VID_OUT_MAX , DUMMY_PARAM_UNIT_TYPE_IMM, DUMMY_UNIT_TYPE_ROOT);
	ImageUnit_SetOutput(ISF_OUT1  +  PHOTO_IME3DNR_ID_MAX + PHOTO_VID_OUT_MAX , ImageUnit_In(&ISF_VdoEnc, ISF_IN1), ISF_PORT_STATE_RUN);
	ImageUnit_End();
	ImageStream_SetRoot(0, ImageUnit_In(&ISF_Dummy, ISF_IN1 + PHOTO_IME3DNR_ID_MAX + PHOTO_VID_OUT_MAX));



	// Set vdoenc
	ImageUnit_Begin(&ISF_VdoEnc, 0);
	ImageUnit_SetOutput(ISF_OUT1, ImageUnit_In(pStrmUnit, ISF_IN1), ISF_PORT_STATE_RUN);
	ImageUnit_SetVdoImgSize(ISF_IN1, p_strm_info->width, p_strm_info->height);
	ImageUnit_SetVdoBufferCount(ISF_IN1, 1);
	_ImageApp_Photo_SetVideoMaxBuf(ISF_OUT1,p_strm_info);
	// set minimum snapshot size
	ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_IMGCAP_W, 16);
	ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_IMGCAP_H, 16);

	//ImageUnit_SetVdoFramerate(ISF_IN1, ISF_VDO_FRC(p_strm_info->frame_rate, 1));
	//set FPS for direct trigger mode
	ImageUnit_SetVdoFramerate(ISF_IN1, ISF_VDO_FRC(1, 1));
	if (p_strm_info->multi_view_type == PHOTO_MULTI_VIEW_SBS_LR)
		ImageUnit_SetVdoAspectRatio(ISF_IN1, p_strm_info->width_ratio << 1, p_strm_info->height_ratio);
	else
		ImageUnit_SetVdoAspectRatio(ISF_IN1, p_strm_info->width_ratio, p_strm_info->height_ratio);
	ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_CODEC, p_strm_info->codec);
	ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_TARGETRATE, p_strm_info->target_bitrate);

	if(photo_strm_cbr_info[p_strm_info->strm_id-PHOTO_STRM_ID_MIN].enable){
    		ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_CBRINFO, (UINT32) &(photo_strm_cbr_info[p_strm_info->strm_id-PHOTO_STRM_ID_MIN].cbr_info));
	}
	ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_RECFORMAT, MEDIAREC_LIVEVIEW);
	ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_ENCBUF_MS, 1500);
	ImageUnit_SetParam(ISF_OUT1, VDOENC_PARAM_ENCBUF_RESERVED_MS, 1000);
	ImageUnit_SetParam(ISF_CTRL, VDOENC_PARAM_EVENT_CB, (UINT32) _ImageApp_Photo_NMI_CB);
	//assign SYNCSRC to framerate source = VdoIn output
	ImageUnit_SetParam(ISF_IN1, VDOENC_PARAM_SYNCSRC, (UINT32)ISF_VIN(p_strm_info->vid_in));
	ImageUnit_End();


	// Set streamsender
	ImageUnit_Begin(pStrmUnit, 0);
	ImageUnit_SetOutput(ISF_OUT1, ISF_PORT_EOS, ISF_PORT_STATE_RUN);
	if (p_strm_info->strm_type == PHOTO_STRM_TYPE_HTTP){
		ImageUnit_SetParam(ISF_OUT1, NETHTTP_PARAM_OPEN_CALLBACK_IMM, (UINT32)_ImageApp_Photo_open_http_cb);
	}else {
#if 0//ISF_NetRTSP
		ImageUnit_SetParam(ISF_CTRL, NETRTSP_PARAM_MAX_CLIENT, PHOTO_RTSP_MAX_CLIENT);
		ImageUnit_SetParam(ISF_CTRL, NETRTSP_PARAM_AUDIO, 0);
		ImageUnit_SetParam(ISF_CTRL, NETRTSP_PARAM_MEDIA_SOURCE, RTSPNVT_MEDIA_SOURCE_BY_URL);
		ImageUnit_SetParam(ISF_CTRL, NETRTSP_PARAM_TOS, (0x20) << 2);
#else//ISF_StreamSender
		ImageUnit_SetParam(ISF_CTRL, STMSND_PARAM_SLOTNUM_VIDEO_IMM, 30);
#endif
	}
	ImageUnit_End();

	// Stream end
	ImageStream_End();
	ImageStream_UpdateAll(&ISF_Stream[g_uiWiFiLinkStream]);
	*/
	UINT32 idx;
	HD_RESULT ret;
	HD_VIDEOENC_PATH_CONFIG venc_path_cfg = {0};
	HD_VIDEOENC_FUNC_CONFIG venc_func_cfg = {0};
	//HD_VIDEOENC_IN  venc_in_param = {0};
	//HD_VIDEOENC_OUT2 venc_out_param = {0};
	//HD_H26XENC_RATE_CONTROL2 venc_rc_param = {0};

	if (p_strm_info->strm_id >= PHOTO_STRM_ID_MAX) {
		DBG_ERR("strm_id  =%d\r\n",p_strm_info->strm_id);
		return;
	}
	idx = p_strm_info->strm_id - PHOTO_STRM_ID_MIN;
	DBG_IND("_ImageApp_Photo_InitWiFiLink ,idx=(%d)\n", idx);

	//if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_0, HD_VIDEOENC_0_OUT_0, &photo_strm_info[idx].venc_p[0])) != HD_OK){
	//}

	//if ((ret = venc_open_ch(HD_VIDEOENC_IN(0 , 1), HD_VIDEOENC_OUT(0 , 1), &(photo_strm_info[idx].venc_p[0]))) != HD_OK) {
	//if ((ret = hd_videoenc_open(HD_VIDEOENC_IN(0 , 2), HD_VIDEOENC_OUT(0 , 2), &(photo_strm_info[idx].venc_p[0]))) != HD_OK) {
	if ((ret = hd_videoenc_open(HD_VIDEOENC_IN(0 , 1), HD_VIDEOENC_OUT(0 , 1), &(photo_strm_info[idx].venc_p[0]))) != HD_OK) {
		DBG_ERR("hd_videoenc_open fail(%d)\n", ret);
	}

	if (p_strm_info->codec == PHOTO_CODEC_MJPG) {
		venc_path_cfg.max_mem.codec_type       = HD_CODEC_TYPE_JPEG;
	} else if (p_strm_info->codec == PHOTO_CODEC_H264) {
		venc_path_cfg.max_mem.codec_type       = HD_CODEC_TYPE_H264;
	} else {
		venc_path_cfg.max_mem.codec_type       = HD_CODEC_TYPE_H265;
	}
	venc_path_cfg.max_mem.max_dim.w        = p_strm_info->max_width;
	venc_path_cfg.max_mem.max_dim.h        = p_strm_info->max_height;
	venc_path_cfg.max_mem.bitrate          = p_strm_info->max_bitrate* 8;               // project setting is byte rate
	venc_path_cfg.max_mem.enc_buf_ms       = 3000;
	venc_path_cfg.max_mem.svc_layer        = HD_SVC_DISABLE;
	venc_path_cfg.max_mem.ltr              = FALSE;
	venc_path_cfg.max_mem.rotate           = FALSE;
	venc_path_cfg.max_mem.source_output    = FALSE;
	venc_path_cfg.isp_id                   = 0;

	DBG_IND("frame_rate=%d, bitrate=%d, w=%d, %d, codec_type=%d\n", p_strm_info->frame_rate, venc_cfg.max_mem.bitrate,venc_cfg.max_mem.max_dim.w,venc_cfg.max_mem.max_dim.h,venc_cfg.max_mem.codec_type );

	IACOMM_VENC_CFG photo_venc_cfg = {
		.video_enc_path = photo_strm_info[idx].venc_p[0],
		.ppath_cfg      = &(venc_path_cfg),
		.pfunc_cfg      = &(venc_func_cfg),
	};
	if ((ret = venc_set_cfg(&photo_venc_cfg)) != HD_OK) {
		DBG_ERR("venc_set_cfg fail(%d)\n", ret);
	}

	photo_wifi_venc_in_param.dim.w               = p_strm_info->width;
	photo_wifi_venc_in_param.dim.h               = p_strm_info->height;
	DBG_IND("dim.w=%d, %d\n", photo_wifi_venc_in_param.dim.w,photo_wifi_venc_in_param.dim.h);


	photo_wifi_venc_in_param.pxl_fmt             = HD_VIDEO_PXLFMT_YUV420;
	photo_wifi_venc_in_param.dir                 = HD_VIDEO_DIR_NONE;
	photo_wifi_venc_in_param.frc                 = HD_VIDEO_FRC_RATIO(1, 1);

	if (p_strm_info->codec == PHOTO_CODEC_MJPG) {
		// TODO
		photo_wifi_venc_out_param.codec_type             = HD_CODEC_TYPE_JPEG;
		photo_wifi_venc_out_param.jpeg.retstart_interval = 0;
		photo_wifi_venc_out_param.jpeg.image_quality     = 50;
		photo_wifi_venc_out_param.jpeg.bitrate           = 0;
		photo_wifi_venc_out_param.jpeg.frame_rate_base   = 1;
		photo_wifi_venc_out_param.jpeg.frame_rate_incr   = 1;
	} else if (p_strm_info->codec == PHOTO_CODEC_H264) {
		photo_wifi_venc_out_param.codec_type             = HD_CODEC_TYPE_H264;
		photo_wifi_venc_out_param.h26x.gop_num           = g_photo_strm_cbr_info[idx].cbr_info.uiGOP ? g_photo_strm_cbr_info[idx].cbr_info.uiGOP : 30;
		photo_wifi_venc_out_param.h26x.source_output     = DISABLE;
		photo_wifi_venc_out_param.h26x.profile           = HD_H264E_HIGH_PROFILE;
		photo_wifi_venc_out_param.h26x.level_idc         = HD_H264E_LEVEL_5_1;
		photo_wifi_venc_out_param.h26x.svc_layer         = HD_SVC_DISABLE;
		photo_wifi_venc_out_param.h26x.entropy_mode      = HD_H264E_CABAC_CODING;
	} else {
		photo_wifi_venc_out_param.codec_type             = HD_CODEC_TYPE_H265;
		photo_wifi_venc_out_param.h26x.gop_num           = g_photo_strm_cbr_info[idx].cbr_info.uiGOP ? g_photo_strm_cbr_info[idx].cbr_info.uiGOP : 30;
		photo_wifi_venc_out_param.h26x.source_output     = DISABLE;
		photo_wifi_venc_out_param.h26x.profile           = HD_H265E_MAIN_PROFILE;
		photo_wifi_venc_out_param.h26x.level_idc         = HD_H265E_LEVEL_5;
		photo_wifi_venc_out_param.h26x.svc_layer         = HD_SVC_DISABLE;
		photo_wifi_venc_out_param.h26x.entropy_mode      = HD_H265E_CABAC_CODING;
	}

	DBG_IND("gop_num=%d\n", photo_wifi_venc_out_param.h26x.gop_num);

	photo_wifi_venc_rc_param.rc_mode                     = HD_RC_MODE_CBR;
	photo_wifi_venc_rc_param.cbr.bitrate                 = p_strm_info->target_bitrate * 8;       // project setting is byte rate
	photo_wifi_venc_rc_param.cbr.frame_rate_base         = p_strm_info->frame_rate;
	photo_wifi_venc_rc_param.cbr.frame_rate_incr         = 1;
	photo_wifi_venc_rc_param.cbr.init_i_qp               = g_photo_strm_cbr_info[idx].cbr_info.uiInitIQp;
	photo_wifi_venc_rc_param.cbr.max_i_qp                = g_photo_strm_cbr_info[idx].cbr_info.uiMaxIQp;
	photo_wifi_venc_rc_param.cbr.min_i_qp                = g_photo_strm_cbr_info[idx].cbr_info.uiMinIQp;
	photo_wifi_venc_rc_param.cbr.init_p_qp               = g_photo_strm_cbr_info[idx].cbr_info.uiInitPQp;
	photo_wifi_venc_rc_param.cbr.max_p_qp                = g_photo_strm_cbr_info[idx].cbr_info.uiMaxPQp;
	photo_wifi_venc_rc_param.cbr.min_p_qp                = g_photo_strm_cbr_info[idx].cbr_info.uiMinPQp;
	photo_wifi_venc_rc_param.cbr.static_time             = g_photo_strm_cbr_info[idx].cbr_info.uiStaticTime;
	photo_wifi_venc_rc_param.cbr.ip_weight               = g_photo_strm_cbr_info[idx].cbr_info.iIPWeight;
	photo_wifi_venc_rc_param.cbr.key_p_period            = 0;
	photo_wifi_venc_rc_param.cbr.kp_weight               = 0;
	photo_wifi_venc_rc_param.cbr.p2_weight               = 0;
	photo_wifi_venc_rc_param.cbr.p3_weight               = 0;
	photo_wifi_venc_rc_param.cbr.lt_weight               = 0;
	photo_wifi_venc_rc_param.cbr.motion_aq_str           = 0;
	photo_wifi_venc_rc_param.cbr.max_frame_size          = 0;
	DBG_IND("bitrate=%d, frame_rate_base=%d, init_i_qp=%d, max_i_qp=%d, min_i_qp=%d\n", photo_wifi_venc_rc_param.cbr.bitrate,photo_wifi_venc_rc_param.cbr.frame_rate_base,photo_wifi_venc_rc_param.cbr.init_i_qp,photo_wifi_venc_rc_param.cbr.max_i_qp,photo_wifi_venc_rc_param.cbr.min_i_qp);
	DBG_IND("init_p_qp=%d, max_p_qp=%d, min_p_qp=%d, static_time=%d, ip_weight=%d\n", photo_wifi_venc_rc_param.cbr.init_p_qp,photo_wifi_venc_rc_param.cbr.max_p_qp,photo_wifi_venc_rc_param.cbr.min_p_qp,photo_wifi_venc_rc_param.cbr.static_time,photo_wifi_venc_rc_param.cbr.ip_weight);

	IACOMM_VENC_PARAM photo_venc_param = {
		.video_enc_path = photo_strm_info[idx].venc_p[0],
		.pin            = &(photo_wifi_venc_in_param),
		.pout           = &(photo_wifi_venc_out_param),
		.prc            = &(photo_wifi_venc_rc_param),
					};
	if ((ret = venc_set_param(&photo_venc_param)) != HD_OK) {
		DBG_ERR("venc_set_param fail(%d)\n", ret);
}

}
void ImageApp_Photo_VEncPull(void)
{
	STRM_VBUFFER_ELEMENT *pVBuf = NULL;
	HD_RESULT hd_ret;
	HD_VIDEOENC_BS  venc_data;
	//UINT32 idx=0, loop, vidx=0;
	UINT32 loop, vidx=0;
	UINT32 use_external_stream = FALSE;

	HD_VIDEOENC_POLL_LIST venc_poll_list = {0};
	venc_poll_list.path_id = photo_strm_info[0].venc_p[0];

	if ((hd_ret = hd_videoenc_poll_list(&venc_poll_list, 1, -1)) == HD_OK) {
		if (venc_poll_list.revent.event == TRUE) {
			if ((hd_ret = hd_videoenc_pull_out_buf(photo_strm_info[0].venc_p[0], &venc_data, 0)) == HD_OK) {
				ImageApp_Common_GetParam(0, IACOMMON_PARAM_MISC_USE_EXTERNAL_STREAMING, &use_external_stream);
				if (use_external_stream == FALSE) {
					if (iacomm_flag_chk(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_QUE_RDY << vidx))) {
						iacomm_flag_clr(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_CQUE_FREE << vidx));
						if ((pVBuf = (STRM_VBUFFER_ELEMENT *)_QUEUE_DeQueueFromHead(&VBufferQueueClear[vidx])) != NULL) {
							pVBuf->timestamp = venc_data.timestamp;
							pVBuf->vcodec_format = venc_data.vcodec_format;
							pVBuf->pack_num = venc_data.pack_num;
							for (loop = 0; loop < pVBuf->pack_num; loop++) {
								pVBuf->phy_addr[loop] = venc_data.video_pack[loop].phy_addr;
								pVBuf->size[loop] = venc_data.video_pack[loop].size;
							}
							_QUEUE_EnQueueToTail(&VBufferQueueReady[vidx], (QUEUE_BUFFER_ELEMENT *)pVBuf);
							iacomm_flag_set(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_BS_RDY << vidx));
						}
						iacomm_flag_set(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_CQUE_FREE << vidx));
					}
				}
				if (photo_vdo_bs_cb[vidx]) {
					photo_vdo_bs_cb[vidx](vidx, &venc_data);
				}
				hd_ret = hd_videoenc_release_out_buf(photo_strm_info[0].venc_p[0], &venc_data);
			}
		}
	}
}

ER _ImageApp_Photo_WiFiLinkOpen(PHOTO_STRM_INFO *p_strm_info)
{
	/*
	_ImageApp_Photo_InitWiFiLink(p_strm_info);
	g_bWifiLinkOpened=TRUE;
	*/
	UINT32 idx;
	T_CFLG cflg ;
	HD_RESULT hd_ret=HD_OK;

	if (p_strm_info->strm_id >= PHOTO_STRM_ID_MAX) {
		DBG_ERR("strm_id  =%d\r\n",p_strm_info->strm_id);
		return E_SYS;
	}
	idx = p_strm_info->strm_id - PHOTO_STRM_ID_MIN;

	if(g_photo_IsWifiLinkOpened[idx] == TRUE)	{
		return 0;
	}
	DBG_IND("WiFiLinkOpen ,idx=%d, IsWifiLinkOpened=%d\r\n",idx, g_photo_IsWifiLinkOpened[idx]);

	_ImageApp_Photo_InitWiFiLink(p_strm_info);

	g_photo_wifi_tsk_run[idx] = FALSE;
	g_photo_is_wifi_tsk_running[idx] = FALSE;


	g_photo_wifi_aepull_tsk_run[idx] = FALSE;
	g_photo_is_wifi_aepull_tsk_running[idx] = FALSE;


	g_PhotoWifiAePullCb= (PHOTO_PROCESS_CB) &ImageApp_Photo_VEncPull;
	if ((hd_ret |= vos_flag_create(&g_photo_wifi_aepull_flg_id, &cflg, "photo_aepull_flg")) != E_OK) {
		DBG_ERR("g_photo_wifi_aepull_flg_id fail\r\n");
	}

	g_photo_IsWifiLinkOpened[idx] = TRUE;

	return 0;
}
ER _ImageApp_Photo_WiFiLinkClose(PHOTO_STRM_INFO *p_strm_info)
{
	/*
	UINT32 i;

	if(g_bWifiLinkOpened==FALSE)
		return E_SYS;

	ImageStream_Close(&ISF_Stream[g_uiWiFiLinkStream]);

    	for (i = PHOTO_STRM_ID_MIN; i < PHOTO_STRM_ID_MAX; i++) {
		if (photo_strm_cbr_info[i - PHOTO_STRM_ID_MIN].enable) {
			photo_strm_cbr_info[i - PHOTO_STRM_ID_MIN].enable = FALSE;
		}
	}
	g_bWifiLinkOpened=FALSE;
	g_bWiFiLinkWifiStart=FALSE;
	*/
	UINT32 idx=0, delay_cnt=0;
	FLGPTN uiFlag=0;
	HD_RESULT hd_ret=0;
	UINT32 use_external_stream = FALSE;

	if (p_strm_info->strm_id >= PHOTO_STRM_ID_MAX) {
		DBG_ERR("strm_id  =%d\r\n",p_strm_info->strm_id);
		return E_SYS;
	}
	idx = p_strm_info->strm_id - PHOTO_STRM_ID_MIN;

	if (g_photo_IsWifiLinkOpened[idx] == FALSE) {
		DBG_ERR("WifiLink not opend.\r\n");
		return E_SYS;
	}


	// step: stop raw buffer pull task
	g_photo_wifi_aepull_tsk_run[idx] = FALSE;       // set this flag and task will return by itself, no need to destroy
	vos_flag_set(g_photo_wifi_aepull_flg_id, FLGPHOTO_WIFI_AEPULL_IDLE);

	delay_cnt = 50;
	while (g_photo_is_wifi_aepull_tsk_running[0] && delay_cnt) {
		vos_util_delay_ms(10);
		delay_cnt --;
	}
	if (g_photo_is_wifi_aepull_tsk_running[0] == TRUE) {
		DBG_WRN("Destroy WifiAePullTsk\r\n");

		vos_task_destroy(g_photo_wifi_aepull_tsk_id);
	}


	if (vos_flag_destroy(g_photo_wifi_aepull_flg_id) != E_OK) {
		DBG_ERR("photo_cap_flg_id destroy failed.\r\n");
	}

	// step: stop raw buffer pull task
	g_photo_wifi_tsk_run[idx] = FALSE;       // set this flag and task will return by itself, no need to destroy
	delay_cnt = 50;
	while (g_photo_is_wifi_tsk_running[0] && delay_cnt) {
		vos_util_delay_ms(10);
		delay_cnt --;
	}
	if (g_photo_is_wifi_tsk_running[0] == TRUE) {
		DBG_WRN("Destroy WifiTsk\r\n");
		vos_task_destroy(g_photo_wifi_tsk_id);
	}

	// step: stop WifiLink
	 hd_videoenc_stop(photo_strm_info[idx].venc_p[0]);

	// step: stop live555
	ImageApp_Common_GetParam(0, IACOMMON_PARAM_MISC_USE_EXTERNAL_STREAMING, &use_external_stream);
	if (use_external_stream == FALSE) {
		iacomm_flag_clr(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_BS_RDY << idx));// | (FLGIACOMMON_AE_S0_READY << idx));
		iacomm_flag_wait(&uiFlag, IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_RQUE_FREE << idx), TWF_ANDW);

		live555_munmap_venc_bs(idx);

		while (_QUEUE_DeQueueFromHead(&VBufferQueueReady[0]) != NULL) {
		}
		while (_QUEUE_DeQueueFromHead(&ABufferQueueReady[0]) != NULL) {
		}
		while (_QUEUE_DeQueueFromHead(&VBufferQueueClear[0]) != NULL) {
		}
		while (_QUEUE_DeQueueFromHead(&ABufferQueueClear[0]) != NULL) {
		}
	}

	if ((hd_ret = venc_close_ch(photo_strm_info[idx].venc_p[0])) != HD_OK) {
		DBG_ERR("venc_close_ch fail.(%d)\r\n", hd_ret);
	}

	g_photo_IsWifiLinkOpened[idx] = FALSE;

	return 0;
}
THREAD_RETTYPE ImageApp_Photo_WifiAePullTsk(void)
{
	//VOS_TICK t1, t2;
	THREAD_ENTRY();
	FLGPTN uiFlag=0;

	g_photo_is_wifi_aepull_tsk_running[0] = TRUE;
	while (g_photo_wifi_aepull_tsk_run[0]) {
		PROFILE_TASK_IDLE();
		vos_flag_wait(&uiFlag, g_photo_wifi_aepull_flg_id, FLGPHOTO_WIFI_AEPULL_START|FLGPHOTO_WIFI_AEPULL_IDLE, TWF_ORW);
		PROFILE_TASK_BUSY();
		if (g_PhotoWifiAePullCb == NULL) {
			DBG_ERR("wifi rawproc cb is not registered.\r\n");
			g_photo_wifi_aepull_tsk_run[0] = FALSE;
			break;
		}
		if (uiFlag & FLGPHOTO_WIFI_AEPULL_IDLE) {
			g_photo_wifi_aepull_tsk_run[0] = FALSE;
			break;
		}

		if (uiFlag & FLGPHOTO_WIFI_AEPULL_START) {
			if(g_photo_is_wifi_showpreview){

				g_photo_is_vepullcb_executing=1;
				//vos_perf_mark(&t1);
				g_PhotoWifiAePullCb();
				//ImageApp_Photo_VEncPull();
				//vos_perf_mark(&t2);
				g_photo_is_vepullcb_executing=0;
			}else{
				vos_util_delay_ms(50);
			}
		}
		//UINT32 t3 = t2 - t1;
		//DBGD(t3);
		//if (t2 - t1 < 10000) {      // 10 ms
			//vos_task_delay_ms(100);
		//}

	}
	g_photo_is_wifi_aepull_tsk_running[0] = FALSE;

	THREAD_RETURN(0);

}
THREAD_RETTYPE ImageApp_Photo_WifiTsk(void)
{
	/*
	FLGPTN FlgPtn;
	kent_tsk();

	// coverity[no_escape]
	while (1) {
		wai_flg(&FlgPtn, PHOTO_WIFI_FLG_ID, FLGPHOTO_WIFI_TRIGGER|FLGPHOTO_WIFI_STOP, TWF_ORW | TWF_CLR);
		if (FlgPtn & FLGPHOTO_WIFI_STOP)
			break;
		if (g_WiFiCb != NULL) {
			g_WiFiCb();
		}
	}
	set_flg(PHOTO_WIFI_FLG_ID, FLGPHOTO_WIFI_IDLE);
	ext_tsk();
	*/
	VOS_TICK t1, t2;
	THREAD_ENTRY();
	g_photo_is_wifi_tsk_running[0] = TRUE;
	while (g_photo_wifi_tsk_run[0]) {
		if (g_PhotoWifiCb == NULL) {
			DBG_ERR("wifi rawproc cb is not registered.\r\n");
			g_photo_wifi_tsk_run[0] = FALSE;
			break;
		}
		if(g_photo_is_wifi_showpreview){
			g_photo_is_wificb_executing=1;
			vos_perf_mark(&t1);
			g_PhotoWifiCb();
			vos_perf_mark(&t2);
			//UINT32 t3 = t2 - t1;
			//DBGD(t3);
			if (t2 - t1 < 10000) {      // 10 ms
				//vos_task_delay_ms(100);
			}
			//ImageApp_Photo_VEncPull();
			g_photo_is_wificb_executing=0;
		}else{
			vos_util_delay_ms(50);
		}
	}
	g_photo_is_wifi_tsk_running[0] = FALSE;

	THREAD_RETURN(0);

}

ER ImageApp_Photo_WiFiLinkStart(PHOTO_STRM_INFO *p_strm_info)
{
	UINT32 idx;
	NVTLIVE555_AUDIO_INFO aud_info={0};
	UINT32 use_external_stream = FALSE;

	if (p_strm_info->strm_id >= PHOTO_STRM_ID_MAX) {
		DBG_ERR("strm_id  =%d\r\n",p_strm_info->strm_id);
		return E_SYS;
	}
	idx = p_strm_info->strm_id - PHOTO_STRM_ID_MIN;
	if(g_photo_wifi_tsk_run[idx] == TRUE){
		return 0;
	}
	DBG_IND("WiFiLinkStart , idx =%d\r\n",idx);

	g_photo_wifi_aepull_tsk_run[idx] = TRUE;
	if ((g_photo_wifi_aepull_tsk_id = vos_task_create(ImageApp_Photo_WifiAePullTsk, 0, "IAPhotoWifiAeTsk", PRI_IMAGEAPP_PHOTO_WIFI_AEPULL, STKSIZE_IMAGEAPP_PHOTO_WIFI_AEPULL)) == 0) {
		DBG_ERR("photo WifiAePullTsk create failed.\r\n");
	} else {
		vos_task_resume(g_photo_wifi_aepull_tsk_id);
	}

	ImageApp_Common_GetParam(0, IACOMMON_PARAM_MISC_USE_EXTERNAL_STREAMING, &use_external_stream);
	if (use_external_stream == FALSE) {
		iacomm_flag_clr(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_BS_RDY << idx));// | (FLGIACOMMON_AE_S0_READY << idx));
		live555_reset_Aqueue(idx);
		live555_reset_Vqueue(idx);
	}

	hd_videoenc_start(photo_strm_info[0].venc_p[0]);
	// step: start live555
	if (use_external_stream == FALSE) {
		live555_mmap_venc_bs(idx, photo_strm_info[0].venc_p[0]);
	}
	vos_flag_set(g_photo_wifi_aepull_flg_id, FLGPHOTO_WIFI_AEPULL_START);


	g_photo_wifi_tsk_run[idx] = TRUE;
	if ((g_photo_wifi_tsk_id = vos_task_create(ImageApp_Photo_WifiTsk, 0, "IAPhotoWifiTsk", PRI_IMAGEAPP_PHOTO_WIFI, STKSIZE_IMAGEAPP_PHOTO_WIFI)) == 0) {
		DBG_ERR("photo WifiTsk create failed.\r\n");
	} else {
		vos_task_resume(g_photo_wifi_tsk_id);
	}

	if (use_external_stream == FALSE) {
		live555_refresh_video_info(idx);

		aud_info.codec_type = NVTLIVE555_CODEC_UNKNOWN;
		live555_set_audio_info(idx, &aud_info);
	}

	return 0;
}
ER ImageApp_Photo_WiFiLinkStop(PHOTO_STRM_ID strm_id)
{
	/*
	ISF_UNIT  *pStrmUnit;
    	PHOTO_STRM_INFO *p_strm_info;
    	UINT32    id ;
	if(g_bWiFiLinkWifiStart==FALSE)
		return E_SYS;

	if ((strm_id < PHOTO_STRM_ID_MAX) && (strm_id >= PHOTO_STRM_ID_MIN)) {
		id = strm_id - PHOTO_STRM_ID_MIN;
	} else {
		DBG_ERR("ID %d is out of range!\r\n", strm_id);
		return E_SYS;
	}

    	p_strm_info = &photo_strm_info[id];

	if (p_strm_info->strm_type == PHOTO_STRM_TYPE_HTTP)
		pStrmUnit = &ISF_NetHTTP;
	else
		pStrmUnit = &ISF_StreamSender;//&ISF_NetRTSP;

	ImageStream_Begin(&ISF_Stream[g_uiWiFiLinkStream], 0);

	ImageUnit_Begin(&ISF_Dummy, 0);
 	ImageUnit_SetOutput(ISF_OUT1  +  PHOTO_IME3DNR_ID_MAX + PHOTO_VID_OUT_MAX , ISF_PORT_KEEP, ISF_PORT_STATE_OFF);
	ImageUnit_End();

	ImageUnit_Begin(&ISF_VdoEnc, 0);
	ImageUnit_SetOutput(ISF_OUT1, ISF_PORT_KEEP, ISF_PORT_STATE_OFF);
 	ImageUnit_End();

	// Set streamsender
	ImageUnit_Begin(pStrmUnit, 0);
	ImageUnit_SetOutput(ISF_OUT1, ISF_PORT_EOS, ISF_PORT_STATE_OFF);
 	ImageUnit_End();

	ImageStream_End();
	ImageStream_UpdateAll(&ISF_Stream[g_uiWiFiLinkStream]);
	g_bWiFiLinkWifiStart=FALSE;
	*/
	return 0;
}

void _ImageApp_Photo_WiFiSetVdoImgSize(UINT32 Path, UINT32 w, UINT32 h)
{
	/*
	ImageUnit_Begin(&ISF_VdoEnc, 0);
	ImageUnit_SetVdoImgSize(ISF_IN1, w, h);
	ImageUnit_End();

	ImageStream_UpdateAll(&ISF_Stream[g_uiWiFiLinkStream]);
	*/
}
void _ImageApp_Photo_WiFiSetAspectRatio(UINT32 Path, UINT32 w, UINT32 h)
{
	/*
	ImageUnit_Begin(&ISF_VdoEnc, 0);
	ImageUnit_SetVdoAspectRatio(ISF_IN1, w, h);
	ImageUnit_End();

	ImageUnit_Begin(&ISF_NetHTTP, 0);
	ImageUnit_SetVdoAspectRatio(ISF_IN1, w, h);
	ImageUnit_End();

	ImageStream_UpdateAll(&ISF_Stream[g_uiWiFiLinkStream]);
	*/
	UINT32 idx;
	PHOTO_STRM_INFO *p_strm_info = &photo_strm_info[Path - PHOTO_STRM_ID_MIN];
	HD_VIDEOPROC_OUT vprc_out_param = {0};
	HD_RESULT ret = HD_OK;
	UINT32 use_external_stream = FALSE;

	if (p_strm_info->strm_id >= PHOTO_STRM_ID_MAX) {
		DBG_ERR("strm_id  =%d\r\n",p_strm_info->strm_id);
		return;
	}
	idx = p_strm_info->strm_id - PHOTO_STRM_ID_MIN;

	if ( photo_strm_info[0].venc_p[0]==0) {
		DBG_ERR("venc_p =0, Path=%d\r\n",Path);
		return;
	}

	// step: stop live555
	ImageApp_Common_GetParam(0, IACOMMON_PARAM_MISC_USE_EXTERNAL_STREAMING, &use_external_stream);
	if (use_external_stream == FALSE) {
		iacomm_flag_clr(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_BS_RDY << idx));
	}
	if ((ret = hd_videoenc_stop(photo_strm_info[0].venc_p[0])) != HD_OK) {
		DBG_ERR("hd_videoenc_stop fail(%d), venc_p=0x%x\r\n", ret, photo_strm_info[0].venc_p[0]);
	}

	if (use_external_stream == FALSE) {
		live555_munmap_venc_bs(idx);
		live555_reset_Vqueue(idx);
	}

	ImageApp_Photo_WiFi_SetPreviewShow(0);
	vprc_out_param.dim.w = ALIGN_ROUND(p_strm_info->height * w/h, 16);//photo_strm_info[PHOTO_STRM_ID_1 - PHOTO_STRM_ID_MIN].width;
	vprc_out_param.dim.h = p_strm_info->height;
	DBG_IND("vprc dim.w=%d, %d\r\n", vprc_out_param.dim.w,vprc_out_param.dim.h);

	vprc_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	vprc_out_param.dir = HD_VIDEO_DIR_NONE;
	vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
	vprc_out_param.depth = 1;//allow pull
	ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_STREAMING_PATH], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param);


	//if ((ret =hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_STREAMING_PATH])) != HD_OK) {
	//	DBG_ERR("hd_videoproc_stop fail(%d)\r\n", ret);
	//}

	if ((ret =hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_STREAMING_PATH])) != HD_OK) {
		DBG_ERR("hd_videoproc_start fail(%d)\r\n", ret);
	}

	photo_wifi_venc_in_param.dim.w               = ALIGN_ROUND(p_strm_info->height * w/h, 16);//p_strm_info->width;
	photo_wifi_venc_in_param.dim.h               = p_strm_info->height;

	DBG_IND("venc dim.w=%d, %d\r\n", photo_wifi_venc_in_param.dim.w,photo_wifi_venc_in_param.dim.h);
	_ImageApp_Photo_venc_set_param(photo_strm_info[0].venc_p[0], &photo_wifi_venc_in_param, &photo_wifi_venc_out_param);

	if ((ret = hd_videoenc_start(photo_strm_info[0].venc_p[0])) != HD_OK) {
		DBG_ERR("hd_videoenc_start fail(%d), venc_p=0x%x\r\n", ret, photo_strm_info[0].venc_p[0]);
	}

	if (use_external_stream == FALSE) {
		iacomm_flag_clr(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_BS_RDY << idx));
	}

	ImageApp_Photo_WiFi_SetPreviewShow(1);

	// step: start live555
	if (use_external_stream == FALSE) {
		live555_mmap_venc_bs(idx, photo_strm_info[0].venc_p[0]);
		live555_refresh_video_info(idx);

		NVTLIVE555_AUDIO_INFO aud_info={0};
		aud_info.codec_type = NVTLIVE555_CODEC_UNKNOWN;
		live555_set_audio_info(idx, &aud_info);
	}
}

INT32 _ImageApp_Photo_ConfigStrmCbr(PHOTO_STRM_CBR_INFO *p_strm_cbr_info)
{
	if (p_strm_cbr_info == NULL) {
		return E_SYS;
	}

	if (p_strm_cbr_info->strm_id >= PHOTO_STRM_ID_MAX || p_strm_cbr_info->strm_id < PHOTO_STRM_ID_MIN) {
		return E_SYS;
	}
	g_photo_strm_cbr_info[p_strm_cbr_info->strm_id - PHOTO_STRM_ID_MIN] = *p_strm_cbr_info;
	return 0;
}


void ImageApp_Photo_WiFiConfig(UINT32 config_id, UINT32 value)
{
    	switch (config_id) {
	case PHOTO_CFG_WIFI_REG_CB: {
			if (value != 0) {
				g_PhotoWifiCb = (PHOTO_PROCESS_CB) value;
			}
		}
		break;
	case PHOTO_CFG_CBR_INFO:
		_ImageApp_Photo_ConfigStrmCbr((PHOTO_STRM_CBR_INFO *) value);
		break;
	/*
	case PHOTO_CFG_DRAW_CB: {
			if (value) {
				g_fn_photo_drawcb = (PHOTO_DRAW_CB *) value;
			}
		}
		break;
	*/
	default:
		DBG_ERR("Unknown config_id=%d\r\n", config_id);
		break;
    	}
}

void PhotoWiFi_SwTimerHdl(UINT32 uiEvent)
{
	/*
	set_flg(PHOTO_WIFI_FLG_ID, FLGPHOTO_WIFI_TRIGGER);
	*/
}

void PhotoWiFi_SwTimerOpen(void)
{
	/*
	if (kchk_flg(PHOTO_WIFI_FLG_ID, FLGPHOTO_WIFI_SWTOPENED) != FLGPHOTO_WIFI_SWTOPENED) {
		if (SwTimer_Open(&PhotoWiFiSwTimerID, PhotoWiFi_SwTimerHdl) != 0) {
			DBG_ERR("Sw timer open failed!\r\n");
			return;
		}
		SwTimer_Cfg(PhotoWiFiSwTimerID, 33, SWTIMER_MODE_FREE_RUN);
		SwTimer_Start(PhotoWiFiSwTimerID);
		set_flg(PHOTO_WIFI_FLG_ID, FLGPHOTO_WIFI_SWTOPENED);
	}
	*/
}

void PhotoWiFi_SwTimerClose(void)
{
	/*
	if (kchk_flg(PHOTO_WIFI_FLG_ID, FLGPHOTO_WIFI_SWTOPENED) == FLGPHOTO_WIFI_SWTOPENED) {
		SwTimer_Stop(PhotoWiFiSwTimerID);
		SwTimer_Close(PhotoWiFiSwTimerID);
		clr_flg(PHOTO_WIFI_FLG_ID, FLGPHOTO_WIFI_SWTOPENED);
	}
	*/
}

ER PhotoWiFiTsk_Open(void)
{
	/*
	if (kchk_flg(PHOTO_WIFI_FLG_ID, FLGPHOTO_WIFI_OPENED) == FLGPHOTO_WIFI_OPENED) {
		DBG_ERR("Error\r\n");
		return E_SYS;
	}

	// clear flag first
	clr_flg(PHOTO_WIFI_FLG_ID, 0xFFFFFFFF);

	// start task
	set_flg(PHOTO_WIFI_FLG_ID, FLGPHOTO_WIFI_OPENED);
	sta_tsk(PHOTO_WIFI_TSK_ID, 0);

	// start timer
	PhotoWiFi_SwTimerOpen();
	*/
	return 0;
}

ER PhotoWiFiTsk_Close(void)
{
	/*
	FLGPTN FlgPtn;

	if (kchk_flg(PHOTO_WIFI_FLG_ID, FLGPHOTO_WIFI_OPENED) != FLGPHOTO_WIFI_OPENED) {
		return E_SYS;
	}

	// stop timer
	PhotoWiFi_SwTimerClose();
	// stop task
	set_flg(PHOTO_WIFI_FLG_ID, FLGPHOTO_WIFI_STOP);
	wai_flg(&FlgPtn, PHOTO_WIFI_FLG_ID, FLGPHOTO_WIFI_IDLE, TWF_ORW | TWF_CLR);

	// stop task
	ter_tsk(PHOTO_WIFI_TSK_ID);
	clr_flg(PHOTO_WIFI_FLG_ID, FLGPHOTO_WIFI_OPENED);
	*/
	return 0;
}

HD_RESULT ImageApp_Photo_WifiPullOut(PHOTO_STRM_INFO *p_strm_info, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)
{
	UINT32 idx;
	if (p_strm_info->strm_id >= PHOTO_STRM_ID_MAX) {
		DBG_ERR("strm_id  =%d\r\n",p_strm_info->strm_id);
		return HD_ERR_NG;
	}
	idx = p_strm_info->strm_id - PHOTO_STRM_ID_MIN;

	return hd_videoproc_pull_out_buf(photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_STREAMING_PATH], p_video_frame, wait_ms);
}

HD_RESULT ImageApp_Photo_WifiPushIn(PHOTO_STRM_INFO *p_strm_info, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)
{
	UINT32 idx;
	if (p_strm_info->strm_id >= PHOTO_STRM_ID_MAX) {
		DBG_ERR("strm_id  =%d\r\n",p_strm_info->strm_id);
		return HD_ERR_NG;
	}
	idx = p_strm_info->strm_id - PHOTO_STRM_ID_MIN;

	return hd_videoenc_push_in_buf(photo_strm_info[idx].venc_p[0], p_video_frame, NULL, wait_ms);
}

HD_RESULT ImageApp_Photo_WifiReleaseOut(PHOTO_STRM_INFO *p_strm_info, HD_VIDEO_FRAME* p_video_frame)
{
	UINT32 idx;
	if (p_strm_info->strm_id >= PHOTO_STRM_ID_MAX) {
		DBG_ERR("strm_id  =%d\r\n",p_strm_info->strm_id);
		return HD_ERR_NG;
	}
	idx = p_strm_info->strm_id - PHOTO_STRM_ID_MIN;

	return hd_videoproc_release_out_buf(photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_STREAMING_PATH], p_video_frame);
}

HD_PATH_ID ImageApp_Photo_GetVdoEncPort(UINT32 id)
{
	UINT32 idx = id;
	HD_PATH_ID path = 0;

	path = photo_strm_info[idx].venc_p[0];
	return path;
}

