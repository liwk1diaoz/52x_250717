#include <stdlib.h>
#include <string.h>
#include "ImageApp/ImageApp_Photo.h"
#include "ImageApp_Photo_int.h"
//#include "ImageUnit_VdoOut.h"
//#include "ImageUnit_ImgTrans.h"
//#include "ImageUnit_UserProc.h"
//#include "ImageUnit_Dummy.h"
#include "Utility/SwTimer.h"
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <kwrap/perf.h>
#include "SizeConvert.h"
#include "vf_gfx.h"
#include <kwrap/util.h>

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          IA_Photo_DispLink
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////// //////////////////////////////////////////////////////////
#include "../Common/ImageApp_Common_int.h"

#define FLGPHOTO_DISP_OPENED            FLGPTN_BIT(0)       //0x00000001
#define FLGPHOTO_DISP_SWTOPENED      FLGPTN_BIT(1)       //0x00000002
#define FLGPHOTO_DISP_RUN               FLGPTN_BIT(2)       //0x00000004
#define FLGPHOTO_DISP_TRIGGER           FLGPTN_BIT(3)       //0x00000008
#define FLGPHOTO_DISP_STOP              FLGPTN_BIT(4)       //0x00000010
#define FLGPHOTO_DISP_IDLE              FLGPTN_BIT(5)       //0x00000020

/*
static SWTIMER_ID PhotoDispSwTimerID=0;
static UINT16 g_uiDispLinkStream=0;
*/
static PHOTO_PROCESS_CB  g_PhotoDispCb = NULL;
static UINT32 g_photo_is_dispcb_executing= 0;

PHOTO_DISP_INFO g_PhotoDispLink[PHOTO_DISP_ID_MAX] = {0};
static UINT32 g_photo_disp_tsk_run[PHOTO_DISP_ID_MAX]= {0};
static UINT32 g_photo_is_disp_tsk_running[PHOTO_DISP_ID_MAX]= {0};
static THREAD_HANDLE g_photo_disp_tsk_id;
UINT32 g_photo_IsDispLinkOpened[PHOTO_DISP_ID_MAX] = {0};

#define PRI_IMAGEAPP_PHOTO_DISP             10
#define STKSIZE_IMAGEAPP_PHOTO_DISP         2048*3
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	ALIGN_CEIL_4(((w) * (h) * HD_VIDEO_PXLFMT_BPP(pxlfmt)) / 8)

UINT32 g_photo_is_disp_showpreview=1;
UINT32 g_photo_is_disp_qv_restart_preview=0;


void _ImageApp_Photo_OpenDispLink(PHOTO_DISP_INFO *p_disp_info);
void PhotoDisp_SwTimerOpen(void);
void PhotoDisp_SwTimerClose(void);

UINT32 ImageApp_Photo_Disp_IsPreviewShow()
{
	return g_photo_is_disp_showpreview;
}
void ImageApp_Photo_Disp_SetPreviewShow(UINT32 IsShow)
{
	UINT32 idx=0;
	UINT32 delay_cnt;
	HD_RESULT ret = HD_OK;
	//HD_VIDEOCAP_IN vcap_in_param[PHOTO_VID_IN_MAX] = {0};

	DBG_IND("SetPreviewShow=%d\r\n",IsShow);
	g_photo_is_disp_showpreview=IsShow;

	if(IsShow==0){
		delay_cnt = 10;
		while (g_photo_is_dispcb_executing==1 && delay_cnt) {
			vos_util_delay_ms(5);
			delay_cnt --;
		}
	}
	if(g_photo_is_disp_qv_restart_preview && IsShow){
		g_photo_is_disp_qv_restart_preview=0;
		for (idx= PHOTO_VID_IN_1; idx < PHOTO_VID_IN_MAX; idx++) {
			DBG_IND("SetPreviewShow idx=%d, enable=%d\r\n", idx, photo_vcap_sensor_info[idx - PHOTO_VID_IN_1].enable);
			if (photo_vcap_sensor_info[idx - PHOTO_VID_IN_1].enable) {
				if(g_photo_cap_sensor_mode_cfg[idx].in_size.w && g_photo_cap_sensor_mode_cfg[idx].in_size.h){
					if ((ret = hd_videocap_set(photo_vcap_sensor_info[idx].vcap_p[0], HD_VIDEOCAP_PARAM_IN, &g_prv_vcap_in_param[idx])) != HD_OK) {
						DBG_ERR("set HD_VIDEOCAP_PARAM_IN fail(%d)\r\n", ret);
					}

					//set vcap crop out
					if ((ret = hd_videocap_set(photo_vcap_sensor_info[idx].vcap_p[0], HD_VIDEOCAP_PARAM_OUT_CROP, &g_prv_vcap_cropout_param[idx])) != HD_OK) {
						DBG_ERR("set HD_VIDEOCAP_PARAM_IN_CROP fail(%d)\r\n", ret);
					}
				}
 				hd_videocap_bind(HD_VIDEOCAP_OUT(idx, 0), HD_VIDEOPROC_IN((idx*PHOTO_VID_IN_MAX+0), 0));
				hd_videocap_start(photo_vcap_sensor_info[idx].vcap_p[0]);
				hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_DISPLAY_PATH]);
			}
		}
	}
}

void ImageApp_Photo_Disp_SwTimerHdl(UINT32 uiEvent)
{
	/*
	set_flg(PHOTO_DISP_FLG_ID, FLGPHOTO_DISP_TRIGGER);
	*/
}

void ImageApp_Photo_Disp_SwTimerOpen(void)
{
	/*
	if (kchk_flg(PHOTO_DISP_FLG_ID, FLGPHOTO_DISP_SWTOPENED) != FLGPHOTO_DISP_SWTOPENED) {
		if (SwTimer_Open(&PhotoDispSwTimerID, PhotoDisp_SwTimerHdl) != 0) {
			DBG_ERR("Sw timer open failed!\r\n");
			return;
		}
		SwTimer_Cfg(PhotoDispSwTimerID, 33, SWTIMER_MODE_FREE_RUN);
		SwTimer_Start(PhotoDispSwTimerID);
		set_flg(PHOTO_DISP_FLG_ID, FLGPHOTO_DISP_SWTOPENED);
	}
	*/
}

void ImageApp_Photo_Disp_SwTimerClose(void)
{
	/*
	if (kchk_flg(PHOTO_DISP_FLG_ID, FLGPHOTO_DISP_SWTOPENED) == FLGPHOTO_DISP_SWTOPENED) {
		SwTimer_Stop(PhotoDispSwTimerID);
		SwTimer_Close(PhotoDispSwTimerID);
		clr_flg(PHOTO_DISP_FLG_ID, FLGPHOTO_DISP_SWTOPENED);
	}
	*/
}

ER ImageApp_Photo_DispTsk_Open(void)
{
	/*
	if (kchk_flg(PHOTO_DISP_FLG_ID, FLGPHOTO_DISP_OPENED) == FLGPHOTO_DISP_OPENED) {
		return E_SYS;
	}

	// clear flag first
	clr_flg(PHOTO_DISP_FLG_ID, 0xFFFFFFFF);

	// start task
	set_flg(PHOTO_DISP_FLG_ID, FLGPHOTO_DISP_OPENED);
	sta_tsk(PHOTO_DISP_TSK_ID, 0);

	// start timer
	PhotoDisp_SwTimerOpen();
	*/

	return 0;
}

ER ImageApp_Photo_DispTsk_Close(void)
{
	/*
	FLGPTN FlgPtn;

	if (kchk_flg(PHOTO_DISP_FLG_ID, FLGPHOTO_DISP_OPENED) != FLGPHOTO_DISP_OPENED) {
		return E_SYS;
	}

	// stop timer
	PhotoDisp_SwTimerClose();

	// stop task
	set_flg(PHOTO_DISP_FLG_ID, FLGPHOTO_DISP_STOP);
	wai_flg(&FlgPtn, PHOTO_DISP_FLG_ID, FLGPHOTO_DISP_IDLE, TWF_ORW | TWF_CLR);

	ter_tsk(PHOTO_DISP_TSK_ID);
	clr_flg(PHOTO_DISP_FLG_ID, FLGPHOTO_DISP_OPENED);
	*/

	return 0;
}

THREAD_RETTYPE ImageApp_Photo_DispTsk(void)
{
	/*
	FLGPTN FlgPtn;
	kent_tsk();

	// coverity[no_escape]
	while (1) {
		wai_flg(&FlgPtn, PHOTO_DISP_FLG_ID, FLGPHOTO_DISP_TRIGGER|FLGPHOTO_DISP_STOP, TWF_ORW | TWF_CLR);
		if (FlgPtn & FLGPHOTO_DISP_STOP)
			break;
		if (g_DispCb != NULL) {
			g_DispCb();
		}
	}
	set_flg(PHOTO_DISP_FLG_ID, FLGPHOTO_DISP_IDLE);
	ext_tsk();
	*/
	VOS_TICK t1, t2;
	THREAD_ENTRY();
	DBG_IND("ImageApp_Photo_DispTsk  Start, disp_tsk_run=%d\r\n",g_photo_disp_tsk_run[0]);
	g_photo_is_disp_tsk_running[0] = TRUE;
	while (g_photo_disp_tsk_run[0]) {
		if (g_PhotoDispCb == NULL) {
			DBG_ERR("disp proc cb is not registered.\r\n");
			g_photo_disp_tsk_run[0] = FALSE;
			break;
		}
		vos_perf_mark(&t1);
		if(g_photo_is_disp_showpreview){
			g_photo_is_dispcb_executing=1;
			g_PhotoDispCb();
			g_photo_is_dispcb_executing=0;
		}else{
			vos_util_delay_ms(50);
		}
		vos_perf_mark(&t2);
		if (t2 - t1 < 10000) {      // 10 ms
			//vos_task_delay_ms(100);
		}
	}
	g_photo_is_disp_tsk_running[0] = FALSE;

	THREAD_RETURN(0);

}

void _ImageApp_Photo_OpenDispLink(PHOTO_DISP_INFO *p_disp_info)
{
	/*
    	if (p_disp_info == NULL) {
		return;
	}

	if (p_disp_info->disp_id >= PHOTO_DISP_ID_MAX) {
		DBG_ERR("disp_id  =%d\r\n",p_disp_info->disp_id);
		return;
	}
	g_uiDispLinkStream =PHOTO_VID_IN_MAX+ p_disp_info->vid_out;
    	//open
	ImageStream_Open(&ISF_Stream[g_uiDispLinkStream]);

	ImageStream_Begin(&ISF_Stream[g_uiDispLinkStream], 0);

	ImageUnit_Begin(&ISF_Dummy, 0);
	ImageUnit_SetParam(ISF_IN1 + PHOTO_IME3DNR_ID_MAX + p_disp_info->vid_out , DUMMY_PARAM_UNIT_TYPE_IMM, DUMMY_UNIT_TYPE_ROOT);
	ImageUnit_SetOutput(ISF_OUT1  + PHOTO_IME3DNR_ID_MAX + p_disp_info->vid_out , ImageUnit_In(&ISF_ImgTrans, ISF_IN1 + p_disp_info->vid_out), ISF_PORT_STATE_RUN);
	ImageUnit_End();
	ImageStream_SetRoot(0, ImageUnit_In(&ISF_Dummy, ISF_IN1 + PHOTO_IME3DNR_ID_MAX + p_disp_info->vid_out));



	// Set imgtrans
	ImageUnit_Begin(&ISF_ImgTrans, 0);
	ImageUnit_SetOutput(ISF_OUT1 + p_disp_info->vid_out, ImageUnit_In(g_isf_vout_info[p_disp_info->vid_out].p_vdoOut, ISF_IN1 + p_disp_info->vid_out), ISF_PORT_STATE_RUN);
	ImageUnit_SetParam(ISF_OUT1 + p_disp_info->vid_out, IMGTRANS_PARAM_CONNECT_IMM, ISF_IN1 + p_disp_info->vid_out);
	#if 0
	if (g_photo_dual_disp.enable) {
    		ImageUnit_SetOutput(ISF_OUT2, ImageUnit_In(&ISF_VdoOut2, ISF_IN1), ISF_PORT_STATE_RUN);
    		ImageUnit_SetParam(ISF_OUT2, IMGTRANS_PARAM_CONNECT_IMM, ISF_IN1);
    		ImageUnit_SetVdoImgSize(ISF_IN1 + p_disp_info->vid_out, g_photo_dual_disp.disp_size.w, g_photo_dual_disp.disp_size.h);
	}else
	#else
	{
    		ImageUnit_SetVdoImgSize(ISF_IN1 + p_disp_info->vid_out, 0, 0);
	}
	#endif
	// set image size 0 to auto sync from vdoout
	ImageUnit_SetParam(ISF_IN1 + p_disp_info->vid_out, IMGTRANS_PARAM_MAX_IMG_WIDTH_IMM, p_disp_info->width);
	ImageUnit_SetParam(ISF_IN1 + p_disp_info->vid_out, IMGTRANS_PARAM_MAX_IMG_HEIGHT_IMM, p_disp_info->height);
	ImageUnit_SetVdoAspectRatio(ISF_IN1, p_disp_info->width_ratio, p_disp_info->height_ratio);
	ImageUnit_End();


	// Set vdoout
	ImageUnit_Begin(g_isf_vout_info[p_disp_info->vid_out].p_vdoOut, 0);
	// set image size 0 to auto calculate by image ratio
	ImageUnit_SetVdoImgSize(ISF_IN1 , 0, 0);
	if (((p_disp_info->rotate_dir & ISF_VDO_DIR_ROTATE_270) == ISF_VDO_DIR_ROTATE_270) || \
		((p_disp_info->rotate_dir & ISF_VDO_DIR_ROTATE_90) == ISF_VDO_DIR_ROTATE_90) ) {
		if (p_disp_info->multi_view_type == PHOTO_MULTI_VIEW_SBS_LR)
			ImageUnit_SetVdoAspectRatio(ISF_IN1, p_disp_info->height_ratio << 1, p_disp_info->width_ratio);
		else
			ImageUnit_SetVdoAspectRatio(ISF_IN1, p_disp_info->height_ratio, p_disp_info->width_ratio);
	}
	else {
		if (p_disp_info->multi_view_type == PHOTO_MULTI_VIEW_SBS_LR)
			ImageUnit_SetVdoAspectRatio(ISF_IN1, p_disp_info->width_ratio << 1, p_disp_info->height_ratio);
		else
			ImageUnit_SetVdoAspectRatio(ISF_IN1, p_disp_info->width_ratio, p_disp_info->height_ratio);
	}
	ImageUnit_SetVdoDirection(ISF_IN1, p_disp_info->rotate_dir);
	ImageUnit_SetVdoPreWindow(ISF_IN1, 0, 0, 0, 0);  //window range = full device range
	ImageUnit_SetOutput(ISF_OUT1, ISF_PORT_EOS, ISF_PORT_STATE_RUN);
	ImageUnit_End();
#if 0
	if (g_photo_dual_disp.enable) {
    		ImageUnit_Begin(&ISF_VdoOut2, 0);
    		ImageUnit_SetVdoImgSize(ISF_IN1, 0, 0);
    		ImageUnit_SetVdoAspectRatio(ISF_IN1, g_photo_dual_disp.disp_ar.w, g_photo_dual_disp.disp_ar.h);
    		ImageUnit_SetVdoPreWindow(ISF_IN1, 0, 0, 0, 0);  //window range = full device range
    		ImageUnit_SetOutput(ISF_OUT1, ISF_PORT_EOS, ISF_PORT_STATE_RUN);
    		ImageUnit_End();
	}
#endif
	// Stream end
	ImageStream_End();
	ImageStream_UpdateAll(&ISF_Stream[g_uiDispLinkStream]);
	*/
	//HD_DIM sz;
	UINT32 idx;
	HD_URECT rect;
    HD_DIM disp_dim = {0};

	if (p_disp_info->disp_id >= PHOTO_DISP_ID_MAX) {
		DBG_ERR("disp_id  =%d\r\n",p_disp_info->disp_id);
		return;
	}
	idx = p_disp_info->disp_id - PHOTO_DISP_ID_MIN;

	if (g_PhotoDispLink[idx].vout_p_ctrl == 0 || g_PhotoDispLink[idx].vout_p[0] == 0) {
		DBG_ERR("Disp path is not config\r\n");
		return;
	}
	HD_RESULT ret = HD_OK;
	HD_DEVCOUNT video_out_dev = {0};

	ret = hd_videoout_get(g_PhotoDispLink[idx].vout_p_ctrl , HD_VIDEOOUT_PARAM_DEVCOUNT, &video_out_dev);
	if (ret != HD_OK) {
		return;
	}

	ret = hd_videoout_get(g_PhotoDispLink[idx].vout_p_ctrl , HD_VIDEOOUT_PARAM_SYSCAPS,  &g_PhotoDispLink[idx].vout_syscaps);
	if (ret != HD_OK) {
		return;
	}

    disp_dim.w = g_PhotoDispLink[idx].vout_syscaps.output_dim.w;
    disp_dim.h = g_PhotoDispLink[idx].vout_syscaps.output_dim.h;
    if ((p_disp_info->rotate_dir != HD_VIDEO_DIR_NONE) && (disp_dim.w < disp_dim.h)) {
        disp_dim.w = g_PhotoDispLink[idx].vout_syscaps.output_dim.h;
        disp_dim.h = g_PhotoDispLink[idx].vout_syscaps.output_dim.w;
    }

	SIZECONVERT_INFO     CovtInfo = {0};
	if(photo_disp_imecrop_info[idx].enable==0){
		CovtInfo.uiSrcWidth  = photo_cap_info[idx].sCapSize.w;
		CovtInfo.uiSrcHeight = photo_cap_info[idx].sCapSize.h;
	}else{
		CovtInfo.uiSrcWidth  = photo_disp_imecrop_info[idx].IMEWin.w;//src ratio
		CovtInfo.uiSrcHeight = photo_disp_imecrop_info[idx].IMEWin.h;//src ratio

	}
	CovtInfo.uiDstWidth  = disp_dim.w;
	CovtInfo.uiDstHeight = disp_dim.h;
	CovtInfo.uiDstWRatio = g_PhotoDispLink[idx].vout_ratio.w;
	CovtInfo.uiDstHRatio = g_PhotoDispLink[idx].vout_ratio.h;
	CovtInfo.alignType   = SIZECONVERT_ALIGN_FLOOR_32;
	DisplaySizeConvert(&CovtInfo);
    if (((p_disp_info->rotate_dir & HD_VIDEO_DIR_ROTATE_90) == HD_VIDEO_DIR_ROTATE_90) || ((p_disp_info->rotate_dir & HD_VIDEO_DIR_ROTATE_270) == HD_VIDEO_DIR_ROTATE_270)) {
        CovtInfo.uiOutWidth = ALIGN_CEIL_8(CovtInfo.uiOutWidth);
        CovtInfo.uiOutHeight = ALIGN_CEIL_8(CovtInfo.uiOutHeight);
        CovtInfo.uiOutX = (disp_dim.w - CovtInfo.uiOutWidth)>>1;
        CovtInfo.uiOutY = (disp_dim.h - CovtInfo.uiOutHeight)>>1;
    }
	DBG_DUMP("\r\nOpenDispLink imecrop_enable=%d, SrcWidth %d, %d, DstWidth=%d, %d ,DstWRatio=%d, %d\r\n", photo_disp_imecrop_info[idx].enable,CovtInfo.uiSrcWidth,CovtInfo.uiSrcHeight,CovtInfo.uiDstWidth,CovtInfo.uiDstHeight,CovtInfo.uiDstWRatio,CovtInfo.uiDstHRatio);

	DBG_DUMP("CovtInfo %d, %d, %d, %d\r\n", CovtInfo.uiOutX,CovtInfo.uiOutY,CovtInfo.uiOutWidth,CovtInfo.uiOutHeight);

	if (CovtInfo.uiOutWidth && CovtInfo.uiOutHeight) {
        if (0){//(p_disp_info->rotate_dir == HD_VIDEO_DIR_ROTATE_90) || (p_disp_info->rotate_dir == HD_VIDEO_DIR_ROTATE_270)) {
    		rect.x = CovtInfo.uiOutY;
    		rect.y = CovtInfo.uiOutX;
    		rect.w = CovtInfo.uiOutHeight;
    		rect.h = CovtInfo.uiOutWidth;
        } else {
    		rect.x = CovtInfo.uiOutX;
    		rect.y = CovtInfo.uiOutY;
    		rect.w = CovtInfo.uiOutWidth;
    		rect.h = CovtInfo.uiOutHeight;
        }
	} else {
		rect.x = 0;
		rect.y = 0;
		rect.w = g_PhotoDispLink[idx].vout_syscaps.output_dim.w;
		rect.h = g_PhotoDispLink[idx].vout_syscaps.output_dim.h;
	}
	//DBG_DUMP("rect.x= %d, %d, %d, %d\r\n", rect.x,rect.y,rect.w,rect.h);

	HD_VIDEOOUT_IN video_out_param={0};
	HD_VIDEOOUT_WIN_ATTR video_out_param2 = {0};

	video_out_param.dim.w = rect.w;
	video_out_param.dim.h = rect.h;
	video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	video_out_param.dir = p_disp_info->rotate_dir;
	ret = hd_videoout_set(g_PhotoDispLink[idx].vout_p[0], HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoout_set\r\n");
		return;
	}

	video_out_param2.visible = TRUE;
	if (((p_disp_info->rotate_dir & HD_VIDEO_DIR_ROTATE_90) == HD_VIDEO_DIR_ROTATE_90) || ((p_disp_info->rotate_dir & HD_VIDEO_DIR_ROTATE_270) == HD_VIDEO_DIR_ROTATE_270)) {
		video_out_param2.rect.x = rect.y;
		video_out_param2.rect.y = rect.x;
		video_out_param2.rect.w = rect.h;
		video_out_param2.rect.h = rect.w;
	}else{
		video_out_param2.rect.x = rect.x;
		video_out_param2.rect.y = rect.y;
		video_out_param2.rect.w = rect.w;
		video_out_param2.rect.h = rect.h;
	}
	video_out_param2.layer = HD_LAYER1;
	if ((ret = hd_videoout_set(g_PhotoDispLink[idx].vout_p[0], HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &video_out_param2)) != HD_OK) {
		DBG_ERR("hd_videoout_set fail(%d)\r\n", ret);
	}


	memset((void *)&video_out_param,0,sizeof(HD_VIDEOOUT_IN));
	ret = hd_videoout_get(g_PhotoDispLink[idx].vout_p[0], HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoout_get\r\n");
		return;
	}

	//g_photo_IsDispLinkOpened[idx] = TRUE;
	g_photo_disp_tsk_run[idx] = FALSE;
	g_photo_is_disp_tsk_running[idx] = FALSE;


}
ER _ImageApp_Photo_DispLinkOpen(PHOTO_DISP_INFO *p_disp_info)
{
	UINT32 idx;

	if (p_disp_info->disp_id >= PHOTO_DISP_ID_MAX) {
		DBG_ERR("disp_id  =%d\r\n",p_disp_info->disp_id);
		return E_SYS;
	}
	idx = p_disp_info->disp_id - PHOTO_DISP_ID_MIN;

	_ImageApp_Photo_OpenDispLink(p_disp_info);
/*
	g_photo_disp_tsk_run[idx] = TRUE;
	if ((g_photo_disp_tsk_id = vos_task_create(ImageApp_Photo_DispTsk, 0, "ImageApp_Photo_DispTsk", PRI_IMAGEAPP_PHOTO_DISP, STKSIZE_IMAGEAPP_PHOTO_DISP)) == 0) {
		DBG_ERR("photo DispTsk create failed.\r\n");
	} else {
		vos_task_resume(g_photo_disp_tsk_id);
	}
*/
	g_photo_IsDispLinkOpened[idx] = TRUE;

	return 0;
}
ER _ImageApp_Photo_DispLinkClose(PHOTO_DISP_INFO *p_disp_info)
{
	/*
	ImageStream_Close(&ISF_Stream[g_uiDispLinkStream]);
	*/
	UINT32 idx=0, delay_cnt=0;
	HD_RESULT hd_ret=0;

	if (p_disp_info->disp_id >= PHOTO_DISP_ID_MAX) {
		DBG_ERR("disp_id  =%d\r\n",p_disp_info->disp_id);
		return E_SYS;
	}
	idx = p_disp_info->disp_id - PHOTO_DISP_ID_MIN;

	if (g_photo_IsDispLinkOpened[idx] == FALSE) {
		DBG_ERR("Disp path %d is already closed.\r\n", idx);
		return E_SYS;
	}
	if (g_photo_is_disp_tsk_running[idx]) {
		//DBG_WRN("Disp task still running, force stop.\r\n");
		//ImageApp_MovieMulti_DispStop(id);

		if (g_photo_IsDispLinkOpened[idx] == FALSE) {
			DBG_ERR("DispLink not opend.\r\n");
			return E_SYS;
		}

		g_photo_disp_tsk_run[idx] = FALSE;       // set this flag and task will return by itself, no need to destroy

		delay_cnt = 50;
		while (g_photo_is_disp_tsk_running[0] && delay_cnt) {
			vos_util_delay_ms(10);
			delay_cnt --;
		}
		if (g_photo_is_disp_tsk_running[0] == TRUE) {
			DBG_WRN("Destroy DispTsk\r\n");
			vos_task_destroy(g_photo_disp_tsk_id);
		}

	}

	if ((hd_ret = vout_get_caps(g_PhotoDispLink[idx].vout_p_ctrl, &(g_PhotoDispLink[idx].vout_syscaps))) != HD_OK) {
		DBG_ERR("vout_get_caps fail(%d)\n", hd_ret);
	}

    {
	VENDOR_VIDEOOUT_FUNC_CONFIG videoout_cfg ={0};

    //set vout stop would keep last frame for change mode
    videoout_cfg.in_func = VENDOR_VIDEOOUT_INFUNC_KEEP_LAST;
	HD_RESULT ret = vendor_videoout_set(g_PhotoDispLink[idx].vout_p[0], VENDOR_VIDEOOUT_ITEM_FUNC_CONFIG, &videoout_cfg);
	if(ret!=HD_OK){
		return ret;
	}
    }
	hd_videoout_stop(g_PhotoDispLink[idx].vout_p[0]);  // do not stop vout, only update counter only !!! to avoid change mode black screen

	g_photo_IsDispLinkOpened[idx] = FALSE;

	return 0;
}
ER _ImageApp_Photo_DispLinkStart(PHOTO_DISP_INFO *p_disp_info)
{
	UINT32 idx;
	idx = p_disp_info->vid_in;
	DBG_IND("_ImageApp_Photo_DispLinkStart ,idx=%d\r\n",idx);

	if(g_photo_disp_tsk_run[idx] == TRUE){
		return 0;
	}

	g_photo_disp_tsk_run[idx] = TRUE;
	if ((g_photo_disp_tsk_id = vos_task_create(ImageApp_Photo_DispTsk, 0, "IAPhotoDispTsk", PRI_IMAGEAPP_PHOTO_DISP, STKSIZE_IMAGEAPP_PHOTO_DISP)) == 0) {
		DBG_ERR("photo DispTsk create failed.\r\n");
	} else {
		vos_task_resume(g_photo_disp_tsk_id);
	}

	hd_videoout_start(g_PhotoDispLink[idx].vout_p[0]);

	return 0;
}

void _ImageApp_Photo_DispSetAspectRatio(UINT32 Path, UINT32 w, UINT32 h)
{
	/*
	UINT32    rotate_dir=0;//coverity 120717
	PHOTO_DISP_INFO *p_disp_info = &photo_disp_info[Path - PHOTO_DISP_ID_MIN];
	ImageUnit_GetVdoDirection(g_isf_vout_info[p_disp_info->vid_out].p_vdoOut, ISF_IN1+ p_disp_info->vid_out, &rotate_dir);

	ImageUnit_Begin(&ISF_ImgTrans, 0);
	ImageUnit_SetVdoAspectRatio(ISF_IN1 + p_disp_info->vid_out, w, h);
	ImageUnit_End();
	DBG_IND("vid_out=%d\r\n",p_disp_info->vid_out);

	ImageUnit_Begin(g_isf_vout_info[p_disp_info->vid_out].p_vdoOut, 0);
	if (((rotate_dir & ISF_VDO_DIR_ROTATE_270) == ISF_VDO_DIR_ROTATE_270) ||
		((rotate_dir & ISF_VDO_DIR_ROTATE_90) == ISF_VDO_DIR_ROTATE_90) ) {
		if (p_disp_info->multi_view_type == PHOTO_MULTI_VIEW_SBS_LR)
			ImageUnit_SetVdoAspectRatio(ISF_IN1, h << 1, w);
		else
			ImageUnit_SetVdoAspectRatio(ISF_IN1, h, w);
	}
	else {
		if (p_disp_info->multi_view_type == PHOTO_MULTI_VIEW_SBS_LR)
			ImageUnit_SetVdoAspectRatio(ISF_IN1, w << 1, h);
		else{
			ImageUnit_SetVdoAspectRatio(ISF_IN1, w , h);
		}
	}
	ImageUnit_End();
	#if 0
	if (g_photo_dual_disp.enable) {
		ImageUnit_Begin(&ISF_VdoOut2, 0);
		g_photo_dual_disp.disp_ar.w=w;
		g_photo_dual_disp.disp_ar.h=h;
		ImageUnit_SetVdoAspectRatio(ISF_IN1, g_photo_dual_disp.disp_ar.w, g_photo_dual_disp.disp_ar.h);
		ImageUnit_End();
	}
	#endif
	ImageStream_UpdateAll(&ISF_Stream[g_uiDispLinkStream]);
	*/
	UINT32 idx;
	HD_URECT rect;
	PHOTO_DISP_INFO *p_disp_info = &photo_disp_info[Path - PHOTO_DISP_ID_MIN];
	HD_VIDEOPROC_OUT vprc_out_param = {0};
    HD_DIM disp_dim = {0};

	if (p_disp_info->disp_id >= PHOTO_DISP_ID_MAX) {
		DBG_ERR("disp_id  =%d\r\n",p_disp_info->disp_id);
		return;
	}
	idx = p_disp_info->disp_id - PHOTO_DISP_ID_MIN;

	if (g_PhotoDispLink[0].vout_p_ctrl == 0) {
		DBG_ERR("Disp path is not config, vout_p_ctrl=0, idx=%d\r\n", idx);
		return;
	}
	if ( g_PhotoDispLink[0].vout_p[0] == 0) {
		DBG_ERR("Disp path is not config, vout_p[0]=0, idx=%d\r\n", idx);
		return;
	}
	if(g_PhotoDispLink[0].vout_syscaps.output_dim.w==0 || g_PhotoDispLink[0].vout_syscaps.output_dim.h==0){
		DBG_ERR("output_dim is not config, idx=%d\r\n", idx);
		return;
	}
	HD_RESULT ret = HD_OK;

    disp_dim.w = g_PhotoDispLink[0].vout_syscaps.output_dim.w;
    disp_dim.h = g_PhotoDispLink[0].vout_syscaps.output_dim.h;
    if ((p_disp_info->rotate_dir != HD_VIDEO_DIR_NONE) && (disp_dim.w < disp_dim.h)) {
        disp_dim.w = g_PhotoDispLink[0].vout_syscaps.output_dim.h;
        disp_dim.h = g_PhotoDispLink[0].vout_syscaps.output_dim.w;
    }
	SIZECONVERT_INFO     CovtInfo = {0};
	if(photo_disp_imecrop_info[idx].enable==0){
		CovtInfo.uiSrcWidth  = w;//photo_cap_info[idx].sCapSize.w;
		CovtInfo.uiSrcHeight = h;//photo_cap_info[idx].sCapSize.h;
	}else{
		CovtInfo.uiSrcWidth  = photo_disp_imecrop_info[idx].IMEWin.w;//src ratio
		CovtInfo.uiSrcHeight = photo_disp_imecrop_info[idx].IMEWin.h;//src ratio
	}
	//CovtInfo.uiSrcWidth  = w;//photo_cap_info[idx].sCapSize.w;
	//CovtInfo.uiSrcHeight = h;//photo_cap_info[idx].sCapSize.h;
	CovtInfo.uiDstWidth  = disp_dim.w;
	CovtInfo.uiDstHeight = disp_dim.h;
	CovtInfo.uiDstWRatio = g_PhotoDispLink[0].vout_ratio.w;
	CovtInfo.uiDstHRatio = g_PhotoDispLink[0].vout_ratio.h;
	CovtInfo.alignType   = SIZECONVERT_ALIGN_FLOOR_32;
	DisplaySizeConvert(&CovtInfo);
    if ((p_disp_info->rotate_dir == HD_VIDEO_DIR_ROTATE_90) || (p_disp_info->rotate_dir == HD_VIDEO_DIR_ROTATE_270)) {
        CovtInfo.uiOutWidth = ALIGN_CEIL_8(CovtInfo.uiOutWidth);
        CovtInfo.uiOutHeight = ALIGN_CEIL_8(CovtInfo.uiOutHeight);
        CovtInfo.uiOutX = (disp_dim.w - CovtInfo.uiOutWidth)>>1;
        CovtInfo.uiOutY = (disp_dim.h - CovtInfo.uiOutHeight)>>1;
    }

	DBG_DUMP("DispSetAspectRatio imecrop_enable=%d, SrcWidth=%d, %d, DstWidth=%d, %d ,DstWRatio=%d, %d\r\n", photo_disp_imecrop_info[idx].enable,CovtInfo.uiSrcWidth,CovtInfo.uiSrcHeight,CovtInfo.uiDstWidth,CovtInfo.uiDstHeight,CovtInfo.uiDstWRatio,CovtInfo.uiDstHRatio);
	//DBG_DUMP("w=%d, %d, IMEWin.w=%d, %d\r\n", w,h,photo_disp_imecrop_info[idx].IMEWin.w,photo_disp_imecrop_info[idx].IMEWin.h);

	DBG_DUMP("CovtInfo %d, %d, %d, %d\r\n", CovtInfo.uiOutX,CovtInfo.uiOutY,CovtInfo.uiOutWidth,CovtInfo.uiOutHeight);

	if (CovtInfo.uiOutWidth && CovtInfo.uiOutHeight) {
        if (0){//(p_disp_info->rotate_dir == HD_VIDEO_DIR_ROTATE_90) || (p_disp_info->rotate_dir == HD_VIDEO_DIR_ROTATE_270)) {
    		rect.x = CovtInfo.uiOutY;
    		rect.y = CovtInfo.uiOutX;
    		rect.w = CovtInfo.uiOutHeight;
    		rect.h = CovtInfo.uiOutWidth;
        } else {
		rect.x = CovtInfo.uiOutX;
		rect.y = CovtInfo.uiOutY;
		rect.w = CovtInfo.uiOutWidth;
		rect.h = CovtInfo.uiOutHeight;
        }
	} else {
		rect.x = 0;
		rect.y = 0;
		rect.w = g_PhotoDispLink[0].vout_syscaps.output_dim.w;
		rect.h = g_PhotoDispLink[0].vout_syscaps.output_dim.h;
	}


	HD_VIDEOOUT_IN video_out_param={0};
	HD_VIDEOOUT_WIN_ATTR video_out_param2 = {0};

	video_out_param.dim.w = rect.w;
	video_out_param.dim.h = rect.h;
	video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	video_out_param.dir = p_disp_info->rotate_dir;//HD_VIDEO_DIR_NONE;
	ret = hd_videoout_set(g_PhotoDispLink[0].vout_p[0], HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoout_set\r\n");
		return;
	}

	video_out_param2.visible = TRUE;
	if ((p_disp_info->rotate_dir == HD_VIDEO_DIR_ROTATE_90) || (p_disp_info->rotate_dir == HD_VIDEO_DIR_ROTATE_270)) {
		video_out_param2.rect.x = rect.y;
		video_out_param2.rect.y = rect.x;
		video_out_param2.rect.w = rect.h;
		video_out_param2.rect.h = rect.w;
	}else{
		video_out_param2.rect.x = rect.x;
		video_out_param2.rect.y = rect.y;
		video_out_param2.rect.w = rect.w;
		video_out_param2.rect.h = rect.h;
	}
	video_out_param2.layer = HD_LAYER1;
	if ((ret = hd_videoout_set(g_PhotoDispLink[0].vout_p[0], HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &video_out_param2)) != HD_OK) {
		DBG_ERR("hd_videoout_set fail(%d)\r\n", ret);
	}


	if(photo_disp_imecrop_info[idx].enable==0){
		vprc_out_param.dim.w = CovtInfo.uiOutWidth;//photo_vcap_sensor_info[idx].sSize[j].w;//p_dim->w;
		vprc_out_param.dim.h = CovtInfo.uiOutHeight;//photo_vcap_sensor_info[idx].sSize[j].h;//p_dim->h;
		vprc_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		vprc_out_param.dir = HD_VIDEO_DIR_NONE;
		vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
		vprc_out_param.depth = 1;//allow pull
		ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_DISPLAY_PATH], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param);
	}else{
		//DBG_DUMP("DispSetAspectRatio IMESize.w=%d, %d\r\n", photo_disp_imecrop_info[idx].IMESize.w, photo_disp_imecrop_info[idx].IMESize.h);
		//DBG_DUMP("enable=%d, IMEWin.x=%d, %d, %d, %d\r\n", photo_disp_imecrop_info[idx].enable,photo_disp_imecrop_info[idx].IMEWin.x, photo_disp_imecrop_info[idx].IMEWin.y, photo_disp_imecrop_info[idx].IMEWin.w, photo_disp_imecrop_info[idx].IMEWin.h);

		vprc_out_param.dim.w = photo_disp_imecrop_info[idx].IMESize.w;//photo_vcap_sensor_info[idx].sSize[j].w;//p_dim->w;
		vprc_out_param.dim.h = photo_disp_imecrop_info[idx].IMESize.h;//photo_vcap_sensor_info[idx].sSize[j].h;//p_dim->h;
		vprc_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		vprc_out_param.dir = HD_VIDEO_DIR_NONE;
		vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
		vprc_out_param.depth = 1;//allow pull
		if ((ret =hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_DISPLAY_PATH], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK) {
			DBG_ERR("hd_videoproc_set(HD_VIDEOPROC_PARAM_OUT) fail(%d)\r\n", ret);
		}

		HD_VIDEOPROC_CROP video_out_crop_param = {0};
		video_out_crop_param.mode =HD_CROP_ON;
		video_out_crop_param.win.rect.x = photo_disp_imecrop_info[idx].IMEWin.x;
		video_out_crop_param.win.rect.y = photo_disp_imecrop_info[idx].IMEWin.y;
		video_out_crop_param.win.rect.w = photo_disp_imecrop_info[idx].IMEWin.w;
		video_out_crop_param.win.rect.h = photo_disp_imecrop_info[idx].IMEWin.h;
		if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_DISPLAY_PATH], HD_VIDEOPROC_PARAM_OUT_CROP, &video_out_crop_param)) != HD_OK) {
			DBG_ERR("hd_videoproc_set(HD_VIDEOPROC_PARAM_OUT_CROP) fail(%d)\r\n", ret);
		}
	}


	ImageApp_Photo_Disp_SetPreviewShow(0);


	if ((ret =hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_DISPLAY_PATH])) != HD_OK) {
		DBG_ERR("hd_videoproc_stop fail(%d)\r\n", ret);
	}

	if ((ret =hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_DISPLAY_PATH])) != HD_OK) {
		DBG_ERR("hd_videoproc_start fail(%d)\r\n", ret);
	}

	if ((ret =hd_videoout_stop(g_PhotoDispLink[0].vout_p[0])) != HD_OK) {
		DBG_ERR("hd_videoout_stop fail(%d)\r\n", ret);
	}

	if ((ret =hd_videoout_start(g_PhotoDispLink[0].vout_p[0])) != HD_OK) {
		DBG_ERR("hd_videoout_start fail(%d)\r\n", ret);
	}

	ImageApp_Photo_Disp_SetPreviewShow(1);

}
void _ImageApp_Photo_DispSetVdoImgSize(UINT32 Path, UINT32 w, UINT32 h)
{
	/*
	UINT32    rotate_dir=0;//coverity 120717
	PHOTO_DISP_INFO *p_disp_info = &photo_disp_info[Path - PHOTO_DISP_ID_MIN];
	ImageUnit_GetVdoDirection(g_isf_vout_info[p_disp_info->vid_out].p_vdoOut, ISF_IN1+ p_disp_info->vid_out, &rotate_dir);

	ImageUnit_Begin(&ISF_ImgTrans, 0);
	ImageUnit_SetVdoImgSize(ISF_IN1 + p_disp_info->vid_out, w, h);
	ImageUnit_End();

	ImageUnit_Begin(g_isf_vout_info[p_disp_info->vid_out].p_vdoOut, 0);
	if (((rotate_dir & ISF_VDO_DIR_ROTATE_270) == ISF_VDO_DIR_ROTATE_270) ||
		((rotate_dir & ISF_VDO_DIR_ROTATE_90) == ISF_VDO_DIR_ROTATE_90) ) {
		ImageUnit_SetVdoImgSize(ISF_IN1, h, w);
	}else{
		ImageUnit_SetVdoImgSize(ISF_IN1, w, h);
	}
	ImageUnit_SetVdoPreWindow(ISF_IN1, 0, 0, 0, 0);  //window range = full device range
	ImageUnit_End();
	#if 0
    	if (g_photo_dual_disp.enable) {
    		ImageUnit_Begin(&ISF_VdoOut2, 0);
    		ImageUnit_SetVdoImgSize(ISF_IN1, w, h);
    		ImageUnit_SetVdoPreWindow(ISF_IN1, 0, 0, 0, 0);  //window range = full device range
    		ImageUnit_End();
    	}
	#endif
	ImageStream_UpdateAll(&ISF_Stream[g_uiDispLinkStream]);
	*/
}
void ImageApp_Photo_DispGetVdoImgSize(UINT32 Path, UINT32* w, UINT32* h)
{
	/*
	UINT32    rotate_dir=0;//coverity 120717

	PHOTO_DISP_INFO *p_disp_info = &photo_disp_info[Path - PHOTO_DISP_ID_MIN];
	ImageUnit_GetVdoDirection(g_isf_vout_info[p_disp_info->vid_out].p_vdoOut, ISF_IN1+ p_disp_info->vid_out, &rotate_dir);

	ImageUnit_Begin(g_isf_vout_info[p_disp_info->vid_out].p_vdoOut, 0);
	ImageUnit_SetVdoImgSize(ISF_IN1, 0, 0);
	ImageUnit_SetVdoPreWindow(ISF_IN1, 0, 0, 0, 0);  //window range = full device range
	if (((rotate_dir & ISF_VDO_DIR_ROTATE_270) == ISF_VDO_DIR_ROTATE_270) ||
		((rotate_dir & ISF_VDO_DIR_ROTATE_90) == ISF_VDO_DIR_ROTATE_90) ) {
		if (p_disp_info->multi_view_type == PHOTO_MULTI_VIEW_SBS_LR)
			ImageUnit_SetVdoAspectRatio(ISF_IN1, *h << 1, *w);
		else
			ImageUnit_SetVdoAspectRatio(ISF_IN1, *h, *w);
	}
	else {
		if (p_disp_info->multi_view_type == PHOTO_MULTI_VIEW_SBS_LR)
			ImageUnit_SetVdoAspectRatio(ISF_IN1, *w << 1, *h);
		else{
			ImageUnit_SetVdoAspectRatio(ISF_IN1, *w , *h);
		}
	}
	ImageUnit_End();
	ImageStream_UpdateAll(&ISF_Stream[g_uiDispLinkStream]);

	ImageUnit_GetVdoImgSize(&ISF_VdoOut1, ISF_IN1 , (w), (h));
	*/
	PHOTO_DISP_INFO *p_disp_info = &photo_disp_info[Path - PHOTO_DISP_ID_MIN];
	UINT32 idx;

	if (p_disp_info->disp_id >= PHOTO_DISP_ID_MAX) {
		DBG_ERR("disp_id  =%d\r\n",p_disp_info->disp_id);
		return;
	}
	idx = p_disp_info->disp_id - PHOTO_DISP_ID_MIN;

	if (g_photo_IsDispLinkOpened[idx] == FALSE) {
		*w = 0;
		*h = 0;
	} else {
		*w = g_PhotoDispLink[idx].vout_syscaps.output_dim.w;
		*h = g_PhotoDispLink[idx].vout_syscaps.output_dim.h;
	}

}
void _ImageApp_Photo_DispResetPath(void)
{
	/*
	ImageStream_UpdateAll(&ISF_Stream[g_uiDispLinkStream]);
	*/
}
void ImageApp_Photo_DispConfig(UINT32 config_id, UINT32 value)
{

    	switch (config_id) {
	case PHOTO_CFG_DISP_REG_CB: {
			if (value != 0) {
				g_PhotoDispCb = (PHOTO_PROCESS_CB) value;
			}
		}
		break;
	case PHOTO_CFG_VOUT_INFO:{
			if (value != 0) {
				PHOTO_VOUT_INFO *ptr = (PHOTO_VOUT_INFO*) value;
				g_PhotoDispLink[0].vout_p_ctrl = ptr->vout_ctrl;
				g_PhotoDispLink[0].vout_p[0] = ptr->vout_path;
				g_PhotoDispLink[0].vout_ratio= ptr->vout_ratio;
			}
		}
		break;
	default:
		DBG_ERR("Unknown config_id=%d\r\n", config_id);
		break;
    	}

}
HD_RESULT ImageApp_Photo_DispPullOut(PHOTO_DISP_INFO *p_disp_info, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)//(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)
{
/*
	UINT32 i = id, idx;

	if ((i >= MaxLinkInfo.MaxImgLink) || (i >= MAX_IMG_PATH)) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return HD_ERR_NG;
	}
	idx = i - _CFG_REC_ID_1;
*/

	UINT32 idx;
	idx = p_disp_info->vid_in;
	//DBG_ERR("idx =%d, %d\r\n", idx, photo_vcap_sensor_info[idx].vproc_p_phy[1]);

	return hd_videoproc_pull_out_buf(photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_DISPLAY_PATH], p_video_frame, wait_ms);
}

HD_RESULT ImageApp_Photo_DispPushIn(PHOTO_DISP_INFO *p_disp_info, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)//(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame, INT32 wait_ms)
{
/*
	UINT32 i = id, idx;

	if (i < _CFG_DISP_ID_1 || i >= (_CFG_DISP_ID_1 + MaxLinkInfo.MaxDispLink) || (i >= (_CFG_DISP_ID_1 + MAX_DISP_PATH))) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return HD_ERR_NG;
	}
	idx = i - _CFG_DISP_ID_1;
*/

	UINT32 idx;

	if (p_disp_info->disp_id >= PHOTO_DISP_ID_MAX) {
		DBG_ERR("disp_id  =%d\r\n",p_disp_info->disp_id);
		return HD_ERR_NG;
	}
	idx = p_disp_info->disp_id - PHOTO_DISP_ID_MIN;

	return hd_videoout_push_in_buf(g_PhotoDispLink[idx].vout_p[0], p_video_frame, NULL, wait_ms);
}

HD_RESULT ImageApp_Photo_DispReleaseOut(PHOTO_DISP_INFO *p_disp_info, HD_VIDEO_FRAME* p_video_frame)//(MOVIE_CFG_REC_ID id, HD_VIDEO_FRAME* p_video_frame)
{
/*
	UINT32 i = id, idx;

	if ((i >= MaxLinkInfo.MaxImgLink) || (i >= MAX_IMG_PATH)) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return HD_ERR_NG;
	}
	idx = i - _CFG_REC_ID_1;
*/

	UINT32 idx;

	idx = p_disp_info->vid_in;

	return hd_videoproc_release_out_buf(photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_DISPLAY_PATH], p_video_frame);
}

