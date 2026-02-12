#include "ImageApp_UsbMovie_int.h"

///// Cross file variables
///// Noncross file variables


UINT32 _ImageApp_UsbMovie_StartVideoCB(UVAC_VID_DEV_CNT vidDevIdx, UVAC_STRM_INFO *pStrmInfo)
{
	UINT32 idx = vidDevIdx, j = 0;
	HD_RESULT hd_ret;
	NVT_USBMOVIE_VID_RESO vidReso = {0};

	if (idx >= MAX_USBIMG_PATH) {
		DBG_ERR("vidDevIdx%d is out of range.\r\n", idx);
		return E_SYS;
	}

	if (pStrmInfo) {
		DBG_DUMP("UVAC Start[%d] resoIdx=%d,W=%d,H=%d,codec=%d,fps=%d,path=%d,tbr=0x%x\r\n", vidDevIdx, pStrmInfo->strmResoIdx, pStrmInfo->strmWidth, pStrmInfo->strmHeight, pStrmInfo->strmCodec, pStrmInfo->strmFps, pStrmInfo->strmPath, pStrmInfo->strmTBR);
	}

	if (gUvacSetVidResoCB && pStrmInfo) {
		vidReso.dev_id = idx;
		vidReso.sen_id = idx;
		vidReso.ResoIdx = pStrmInfo->strmResoIdx;
		vidReso.width = pStrmInfo->strmWidth;
		vidReso.height = pStrmInfo->strmHeight;
		vidReso.fps = pStrmInfo->strmFps;
		vidReso.codec = pStrmInfo->strmCodec;
		vidReso.tbr = pStrmInfo->strmTBR;
		gUvacSetVidResoCB(&vidReso);
		idx = vidReso.sen_id;
		pStrmInfo->strmTBR = vidReso.tbr;
	}

	if (pStrmInfo && (idx < UVAC_VID_DEV_CNT_MAX)) {
		IACOMM_VPROC_PARAM vproc_param = {0};
		HD_VIDEOPROC_FUNC_CONFIG vproc_func_param = {0};
		vproc_func_param.ddr_id = usbimg_dram_cfg.video_proc[idx];
		if (usbimg_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR && pStrmInfo->strmWidth == usbimg_vproc_param[idx][1].dim.w && pStrmInfo->strmHeight == usbimg_vproc_param[idx][1].dim.h) {
			vproc_func_param.out_func &= ~HD_VIDEOPROC_OUTFUNC_ONEBUF;
			// update vprc extend path (
			usbimg_vproc_param_ex[idx][j].src_path = UsbImgLink[idx].vproc_p_phy[1];   // if img size match vproc_p_phy[2] (3DNR path), mapping to this path directly
			usbimg_vproc_param_ex[idx][j].dim.w = usbimg_max_imgsize[idx].w;
			usbimg_vproc_param_ex[idx][j].dim.h = usbimg_max_imgsize[idx].h;
			usbimg_vproc_param_ex[idx][j].frc = _ImageApp_UsbMovie_frc(vidReso.fps, GET_HI_UINT16(usbimg_vcap_in_param[idx].frc));
			if ((hd_ret = hd_videoproc_set(UsbImgLink[idx].vproc_p[j], HD_VIDEOPROC_PARAM_OUT_EX, &(usbimg_vproc_param_ex[idx][j]))) != HD_OK) {
				DBG_ERR("hd_videoproc_set fail(%d)\r\n", hd_ret);
			}
		} else {
			vproc_func_param.out_func |= HD_VIDEOPROC_OUTFUNC_ONEBUF;
			// update vprc real and extend path
			usbimg_vproc_param[idx][j].dim.w = pStrmInfo->strmWidth;
			usbimg_vproc_param[idx][j].dim.h = pStrmInfo->strmHeight;
			if ((hd_ret = hd_videoproc_set(UsbImgLink[idx].vproc_p_phy[j], HD_VIDEOPROC_PARAM_OUT, &(usbimg_vproc_param[idx][j]))) != HD_OK) {
				DBG_ERR("hd_videoproc_set fail(%d)\r\n", hd_ret);
			}
			usbimg_vproc_param_ex[idx][j].src_path = UsbImgLink[idx].vproc_p_phy[j];   // if img size does not match vproc_p_phy[2] (3DNR path), mapping to corresponding path
			usbimg_vproc_param_ex[idx][j].dim.w = pStrmInfo->strmWidth;
			usbimg_vproc_param_ex[idx][j].dim.h = pStrmInfo->strmHeight;
			usbimg_vproc_param_ex[idx][j].frc = _ImageApp_UsbMovie_frc(vidReso.fps, GET_HI_UINT16(usbimg_vcap_in_param[idx].frc));
			if ((hd_ret = hd_videoproc_set(UsbImgLink[idx].vproc_p[j], HD_VIDEOPROC_PARAM_OUT_EX, &(usbimg_vproc_param_ex[idx][j]))) != HD_OK) {
				DBG_ERR("hd_videoproc_set fail(%d)\r\n", hd_ret);
			}
		}
		// update 3DNR reference ONEBUF mode setting
		vproc_param.video_proc_path = UsbImgLink[idx].vproc_p_phy[1];
		vproc_param.p_dim           = &(usbimg_vproc_param[idx][1].dim);
		vproc_param.pxlfmt          = HD_VIDEO_PXLFMT_YUV420;
		vproc_param.frc             = _ImageApp_UsbMovie_frc(1, 1);
		vproc_param.pfunc           = &vproc_func_param;
		memset((void *)&(vproc_param.video_proc_crop_param), 0, sizeof(HD_VIDEOPROC_CROP));
		if ((hd_ret = vproc_set_param(&vproc_param)) != HD_OK) {
			DBG_ERR("vproc_set_param fail(%d)\n", hd_ret);
		}

		// update venc path
		usbimg_venc_in_param[idx][j].dim.w = pStrmInfo->strmWidth;
		usbimg_venc_in_param[idx][j].dim.h = pStrmInfo->strmHeight;
		usbimg_venc_in_param[idx][j].pxl_fmt = HD_VIDEO_PXLFMT_YUV420;

		if (pStrmInfo->strmCodec == UVAC_VIDEO_FORMAT_H264) {
			usbimg_venc_out_param[idx][j].codec_type                 = HD_CODEC_TYPE_H264;
			usbimg_venc_out_param[idx][j].h26x.gop_num               = vidReso.fps;
			usbimg_venc_out_param[idx][j].h26x.ltr_interval          = 0;
			usbimg_venc_out_param[idx][j].h26x.ltr_pre_ref           = 0;
			usbimg_venc_out_param[idx][j].h26x.gray_en               = DISABLE;
			usbimg_venc_out_param[idx][j].h26x.source_output         = DISABLE;
			usbimg_venc_out_param[idx][j].h26x.profile               = HD_H264E_HIGH_PROFILE;
			usbimg_venc_out_param[idx][j].h26x.level_idc             = HD_H264E_LEVEL_5_1;
			usbimg_venc_out_param[idx][j].h26x.svc_layer             = HD_SVC_DISABLE;
			usbimg_venc_out_param[idx][j].h26x.entropy_mode          = HD_H264E_CABAC_CODING;
			usbimg_venc_out_param[idx][j].h26x.chrm_qp_idx          = 0;

			usbimg_venc_rc_param[idx][j].cbr.bitrate                 = pStrmInfo->strmTBR * 8;
			usbimg_venc_rc_param[idx][j].cbr.frame_rate_base         = vidReso.fps;
			usbimg_venc_rc_param[idx][j].cbr.frame_rate_incr         = 1;
		} else {
			usbimg_venc_out_param[idx][j].codec_type                 = HD_CODEC_TYPE_JPEG;
			usbimg_venc_out_param[idx][j].jpeg.retstart_interval     = 0;
			usbimg_venc_out_param[idx][j].jpeg.image_quality         = usbimg_jpg_quality.jpg_quality;
			if (usbimg_jpg_quality.buf_size == 0xffffffff) {
				usbimg_venc_out_param[idx][j].jpeg.bitrate               = pStrmInfo->strmTBR * 8;
				usbimg_venc_out_param[idx][j].jpeg.frame_rate_base       = vidReso.fps;
			} else {
				usbimg_venc_out_param[idx][j].jpeg.bitrate               = 0;
				usbimg_venc_out_param[idx][j].jpeg.frame_rate_base       = 1;
			}
			usbimg_venc_out_param[idx][j].jpeg.frame_rate_incr       = 1;

			usbimg_venc_rc_param[idx][j].cbr.bitrate                 = pStrmInfo->strmTBR * 8;

			if (usbimg_jpg_quality.buf_size == 0xffffffff) {
				VENDOR_VIDEOENC_JPG_RC_CFG jpg_rc_param = {0};
				jpg_rc_param.vbr_mode_en = 0;  // 0: CBR, 1: VBR
				jpg_rc_param.min_quality = 10; // min quality
				jpg_rc_param.max_quality = 70; // max quality
				if ((hd_ret = vendor_videoenc_set(UsbImgLink[idx].venc_p[j], VENDOR_VIDEOENC_PARAM_OUT_JPG_RC, &jpg_rc_param)) != HD_OK) {
					DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_JPG_RC) fail(%d)\n", hd_ret);
				}
			}
		}
		IACOMM_VENC_PARAM venc_param = {
			.video_enc_path = UsbImgLink[idx].venc_p[j],
			.pin            = &(usbimg_venc_in_param[idx][j]),
			.pout           = &(usbimg_venc_out_param[idx][j]),
			.prc            = &(usbimg_venc_rc_param[idx][j]),
		};
		if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
			DBG_ERR("venc_set_param fail(%d)\n", hd_ret);
		}
		_ImageApp_UsbMovie_AudCapStart(0);
		_ImageApp_UsbMovie_UvacStart(idx, j);
	}
	return E_OK;
}

void _ImageApp_UsbMovie_StopVideoCB(UVAC_VID_DEV_CNT vidDevIdx)
{
	UINT32 idx = vidDevIdx, j = 0;

	if (idx >= MAX_USBIMG_PATH) {
		DBG_ERR("vidDevIdx%d is out of range.\r\n", idx);
		return;
	}

	_ImageApp_UsbMovie_AudCapStop(0);
	_ImageApp_UsbMovie_UvacStop(idx, j);
}

UINT32 _ImageApp_UsbMovie_SetVolumeCB(UINT32 volume)
{
#if 0
	DBG_IND("UVAC Set Volume = %d\r\n", volume);

	ImageUnit_Begin(&ISF_AudIn, 0);
	ImageUnit_SetParam(ISF_CTRL, AUDIN_PARAM_VOL_IMM, volume);
	ImageUnit_End();
#endif
	return 0;
}

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
void _ImageApp_UsbMovie_memcpy(UINT32 uiDst, UINT32 uiSrc, UINT32 uiSize)
{
	hd_gfx_memcpy(uiDst, uiSrc, uiSize);
}
#endif

