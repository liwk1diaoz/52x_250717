#include "vf_gfx.h"
#include <kwrap/error_no.h>
#include "System/SysCommon.h"
#include "System/SysMain.h"
#include "UIApp/Movie/UIAppMovie.h"
#include "UIWnd/UIFlow.h"
#include "AppDisp_PipView.h"
#include "vendor_gfx.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          PipView
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

void PipView_SetStyle(UINT32 uiStyle)
{
	if (uiStyle >= DUALCAM_SETTING_MAX) {
		uiStyle = DUALCAM_FRONT;
	}
	UI_SetData(FL_DUAL_CAM, uiStyle);
	UI_SetData(FL_DUAL_CAM_MENU, uiStyle);
}
HD_RESULT PipView_copy_no_flush(VF_GFX_COPY *p_param)
{
	HD_GFX_COPY param;

	if(!p_param){
		printf("vf_gfx_copy:p_param is NULL\n");
		return HD_ERR_INV;
	}


	UINT32 plane_num, i;

	plane_num = (HD_VIDEO_MAX_PLANE > HD_GFX_MAX_PLANE_NUM ?
					HD_GFX_MAX_PLANE_NUM : HD_VIDEO_MAX_PLANE);

	memset(&param.src_img, 0, sizeof(HD_GFX_IMG_BUF));
	param.src_img.dim.w  = p_param->src_img.dim.w;
	param.src_img.dim.h  = p_param->src_img.dim.h;
	param.src_img.format = p_param->src_img.pxlfmt;
	param.src_img.ddr_id = p_param->src_img.ddr_id;
	for(i = 0 ; i < plane_num ; ++i){
		param.src_img.p_phy_addr[i] = p_param->src_img.phy_addr[i];
		param.src_img.lineoffset[i] = p_param->src_img.loff[i];
	}


	memset(&param.dst_img, 0, sizeof(HD_GFX_IMG_BUF));
	param.dst_img.dim.w  = p_param->dst_img.dim.w;
	param.dst_img.dim.h  = p_param->dst_img.dim.h;
	param.dst_img.format = p_param->dst_img.pxlfmt;
	param.dst_img.ddr_id = p_param->dst_img.ddr_id;
	for(i = 0 ; i < plane_num ; ++i){
		param.dst_img.p_phy_addr[i] = p_param->dst_img.phy_addr[i];
		param.dst_img.lineoffset[i] = p_param->dst_img.loff[i];
	}

	memcpy(&param.src_region, &p_param->src_region, sizeof(HD_IRECT));
	memcpy(&param.dst_pos, &p_param->dst_pos, sizeof(HD_IPOINT));
	param.colorkey = p_param->colorkey;
	param.alpha    = p_param->alpha;

	return vendor_gfx_copy_no_flush(&param);
}

INT32 PipView_CopyData(HD_VIDEO_FRAME *pSrcImg, IRECT *pSrcRegion, HD_VIDEO_FRAME *pDstImg, IRECT *pDstRegion, UINT32 bFlush)
{
	HD_RESULT hd_ret;
	VF_GFX_COPY vf_copy = {0};

	// set src
	if ((hd_ret = vf_init_ex(&vf_copy.src_img, pSrcImg->dim.w, pSrcImg->dim.h, pSrcImg->pxlfmt, pSrcImg->loff, pSrcImg->phy_addr)) != HD_OK) {
	//if ((hd_ret = vf_init(&vf_copy.src_img, pSrcImg->dim.w, pSrcImg->dim.h, pSrcImg->pxlfmt, pSrcImg->loff[0], pSrcImg->phy_addr[0], pSrcImg->loff[0]*pSrcImg->dim.h*3/2)) != HD_OK) {
		DBG_ERR("vf_init_ex src failed(%d)\r\n", hd_ret);
		return E_SYS;
	}

	// set dest
	if ((hd_ret = vf_init_ex(&vf_copy.dst_img, pDstImg->dim.w, pDstImg->dim.h, pDstImg->pxlfmt, pDstImg->loff, pDstImg->phy_addr)) != HD_OK) {
	//if ((hd_ret = vf_init(&vf_copy.dst_img, pDstImg->dim.w, pDstImg->dim.h, pDstImg->pxlfmt, pDstImg->loff[0], pDstImg->phy_addr[0], pDstImg->loff[0]*pDstImg->dim.h*3/2)) != HD_OK) {
		DBG_ERR("vf_init_ex dst failed(%d)\r\n", hd_ret);
		return E_SYS;
	}

	// set config
	vf_copy.engine = 0;
	vf_copy.alpha = 255;
	if (pSrcRegion == REGION_MATCH_IMG) {
		vf_copy.src_region.x = 0;
		vf_copy.src_region.y = 0;
		vf_copy.src_region.w = pSrcImg->dim.w;
		vf_copy.src_region.h = pSrcImg->dim.h;
	} else {
		vf_copy.src_region.x = pSrcRegion->x;
		vf_copy.src_region.y = pSrcRegion->y;
		vf_copy.src_region.w = pSrcRegion->w;
		vf_copy.src_region.h = pSrcRegion->h;
	}
	if (pDstRegion == REGION_MATCH_IMG) {
		vf_copy.dst_pos.x = 0;
		vf_copy.dst_pos.y = 0;
	} else {
		vf_copy.dst_pos.x = pDstRegion->x;
		vf_copy.dst_pos.y = pDstRegion->y;
	}
	if(bFlush){
		if ((hd_ret = vf_gfx_copy(&vf_copy)) != HD_OK) {
			DBG_ERR("vf_gfx_copy failed(%d)\r\n", hd_ret);
		}
	}else{
		if ((hd_ret = PipView_copy_no_flush(&vf_copy)) != HD_OK) {
			DBG_ERR("PipView_copy_no_flush failed(%d)\r\n", hd_ret);
		}
	}

	return E_OK;
}


INT32 PipView_ScaleData(HD_VIDEO_FRAME *pSrcImg, IRECT *pSrcRegion, HD_VIDEO_FRAME *pDstImg, IRECT *pDstRegion)
{
	HD_RESULT hd_ret;
	VF_GFX_SCALE vf_scale = {0};

	// set src
	if ((hd_ret = vf_init_ex(&vf_scale.src_img, pSrcImg->dim.w, pSrcImg->dim.h, pSrcImg->pxlfmt, pSrcImg->loff, pSrcImg->phy_addr)) != HD_OK) {
		DBG_ERR("vf_init_ex src failed(%d)\r\n", hd_ret);
		return E_SYS;
	}

	// set dest
	if ((hd_ret = vf_init_ex(&vf_scale.dst_img, pDstImg->dim.w, pDstImg->dim.h, pDstImg->pxlfmt, pDstImg->loff, pDstImg->phy_addr)) != HD_OK) {
		DBG_ERR("vf_init_ex dst failed(%d)\r\n", hd_ret);
		return E_SYS;
	}

	// set config
	vf_scale.engine = 0;
	if (pSrcRegion == REGION_MATCH_IMG) {
		vf_scale.src_region.x = 0;
		vf_scale.src_region.y = 0;
		vf_scale.src_region.w = pSrcImg->dim.w;
		vf_scale.src_region.h = pSrcImg->dim.h;
	} else {
		vf_scale.src_region.x = pSrcRegion->x;
		vf_scale.src_region.y = pSrcRegion->y;
		vf_scale.src_region.w = pSrcRegion->w;
		vf_scale.src_region.h = pSrcRegion->h;
	}
	if (pDstRegion == REGION_MATCH_IMG) {
		vf_scale.dst_region.x = 0;
		vf_scale.dst_region.y = 0;
		vf_scale.dst_region.w = pDstImg->dim.w;
		vf_scale.dst_region.h = pDstImg->dim.h;
	} else {
		vf_scale.dst_region.x = pDstRegion->x;
		vf_scale.dst_region.y = pDstRegion->y;
		vf_scale.dst_region.w = pDstRegion->w;
		vf_scale.dst_region.h = pDstRegion->h;
	}
	vf_scale.quality = HD_GFX_SCALE_QUALITY_BILINEAR;
	if ((hd_ret = vf_gfx_scale(&vf_scale, 0)) != HD_OK) {
		DBG_ERR("vf_gfx_scale failed(%d)\r\n", hd_ret);
	}

	return E_OK;
}
INT32 PipView_FillDataBlack(HD_VIDEO_FRAME *pDstImg, IRECT *pDstRegion)
{
	//return E_OK;

	VF_GFX_DRAW_RECT fill_rect = {0};
	HD_RESULT hd_ret;
	memcpy((void *)&(fill_rect.dst_img), (void *)(pDstImg), sizeof(HD_VIDEO_FRAME));
	fill_rect.color = COLOR_RGB_BLACK;
	if (pDstRegion != REGION_MATCH_IMG) {
		fill_rect.rect.x = pDstRegion->x;
		fill_rect.rect.y = pDstRegion->y;
		fill_rect.rect.w = pDstRegion->w;
		fill_rect.rect.h = pDstRegion->h;
	}else{
		fill_rect.rect.x = 0;
		fill_rect.rect.y = 0;
		fill_rect.rect.w = pDstImg->dim.w;
		fill_rect.rect.h = pDstImg->dim.h;
	}
	fill_rect.type = HD_GFX_RECT_SOLID;
	fill_rect.thickness = 0;
	fill_rect.engine = 0;
	if ((hd_ret = vf_gfx_draw_rect(&fill_rect)) != HD_OK) {
		DBG_ERR("vf_gfx_draw_rect failed(%d)\r\n", hd_ret);
	}
	return E_OK;
}
INT32 PipView_RotateData(HD_VIDEO_FRAME *pSrcImg, IRECT *pSrcRegion, HD_VIDEO_FRAME *pDstImg, IPOINT *pDstLocation, UINT32 RotateDir)
{
	HD_RESULT hd_ret;
	VF_GFX_ROTATE vf_rotate = {0};

	// set src
	if ((hd_ret = vf_init_ex(&vf_rotate.src_img, pSrcImg->dim.w, pSrcImg->dim.h, pSrcImg->pxlfmt, pSrcImg->loff, pSrcImg->phy_addr)) != HD_OK) {
		DBG_ERR("vf_init_ex src failed(%d)\r\n", hd_ret);
		return E_SYS;
	}

	// set dest
	if ((hd_ret = vf_init_ex(&vf_rotate.dst_img, pDstImg->dim.w, pDstImg->dim.h, pDstImg->pxlfmt, pDstImg->loff, pDstImg->phy_addr)) != HD_OK) {
		DBG_ERR("vf_init_ex dst failed(%d)\r\n", hd_ret);
		return E_SYS;
	}

	// set config
	if (pSrcRegion == REGION_MATCH_IMG) {
		vf_rotate.src_region.x	= 0;
		vf_rotate.src_region.y	= 0;
		vf_rotate.src_region.w	= pSrcImg->dim.w;
		vf_rotate.src_region.h	= pSrcImg->dim.h;
	} else {
		vf_rotate.src_region.x	= pSrcRegion->x;
		vf_rotate.src_region.y	= pSrcRegion->y;
		vf_rotate.src_region.w	= pSrcRegion->w;
		vf_rotate.src_region.h	= pSrcRegion->h;
	}
	vf_rotate.dst_pos.x	= pDstLocation->x;
	vf_rotate.dst_pos.y	= pDstLocation->y;
	vf_rotate.angle		= RotateDir;
	if ((hd_ret = vf_gfx_rotate(&vf_rotate)) != HD_OK) {
		DBG_ERR("vf_gfx_rotate failed(%d)\r\n", hd_ret);
		return E_SYS;
	}

	return E_OK;
}

INT32 PipView_OnDraw_2sensor(APPDISP_VIEW_DRAW *pDraw)
{
	//IRECT	src_region;
	IRECT	dst_region;
	UINT32	dual_cam;
	#if (DUALCAM_PIP_BEHIND_FLIP)
	IPOINT	dstLocation = {0, 0};
	#endif

	if (1)//(pDraw->viewcnt == 2)
	{
		//dual_cam = UI_GetData(FL_DUAL_CAM);
		dual_cam = MovieExe_GetPipStyle();
		if (dual_cam == DUALCAM_FRONT)
		{
			// img1
			if (pDraw->p_src_img[0] && (pDraw->p_dst_img != pDraw->p_src_img[0])) {
				//GxImg_FillData(pDraw->p_dst_img, NULL, COLOR_YUV_BLACK);

				dst_region.x = 0;
				dst_region.y = 0;
				dst_region.w = pDraw->p_dst_img->dim.w;
				dst_region.h = pDraw->p_dst_img->dim.h;

				if (pDraw->p_src_img[0]->dim.w== pDraw->p_dst_img->dim.w && pDraw->p_src_img[0]->dim.h== pDraw->p_dst_img->dim.h){
					PipView_CopyData(pDraw->p_src_img[0], REGION_MATCH_IMG, pDraw->p_dst_img, REGION_MATCH_IMG, 1);
				}else{
					PipView_ScaleData(pDraw->p_src_img[0], REGION_MATCH_IMG, pDraw->p_dst_img, REGION_MATCH_IMG);
				}
			}
		}
		else if (dual_cam == DUALCAM_BEHIND)
		{
			// img2
			if (pDraw->p_src_img[1] && (pDraw->p_dst_img != pDraw->p_src_img[1])) {
				//GxImg_FillData(pDraw->p_dst_img, NULL, COLOR_YUV_BLACK); //mark for latency

				#if (DUALCAM_PIP_BEHIND_FLIP == DISABLE)
					PipView_ScaleData(pDraw->p_src_img[1], REGION_MATCH_IMG, pDraw->p_dst_img, REGION_MATCH_IMG);
				#else
					// flip rear image to destination buffer
					PipView_RotateData(pDraw->p_src_img[1], REGION_MATCH_IMG, pDraw->p_dst_img, &dstLocation, HD_VIDEO_DIR_MIRRORX);
				#endif
			}
		}
		else if (dual_cam == DUALCAM_BOTH2)
		{
			// rear image is bigger
			if (pDraw->p_src_img[1] && (pDraw->p_dst_img != pDraw->p_src_img[1])) {
				//GxImg_FillData(pDraw->p_dst_img, NULL, COLOR_YUV_BLACK);

				#if (DUALCAM_PIP_BEHIND_FLIP == DISABLE)
					PipView_ScaleData(pDraw->p_src_img[1], REGION_MATCH_IMG, pDraw->p_dst_img, REGION_MATCH_IMG);
				#else
					// flip rear image to destination buffer
					PipView_RotateData(pDraw->p_src_img[1], REGION_MATCH_IMG, pDraw->p_dst_img, &dstLocation, HD_VIDEO_DIR_MIRRORX);
				#endif
			}

			// front image is smaller
			if (pDraw->p_src_img[0]) {
				dst_region.x = 0;
				dst_region.y = 0;
				dst_region.w = pDraw->p_dst_img->dim.w / 2;
				dst_region.h = pDraw->p_dst_img->dim.h / 2;
				PipView_ScaleData(pDraw->p_src_img[0], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
			}
		}
		else if (dual_cam == DUALCAM_BOTH)
		{
			// front image is bigger
			if (pDraw->p_src_img[0] && (pDraw->p_dst_img != pDraw->p_src_img[0])) {
				//GxImg_FillData(pDraw->p_dst_img, NULL, COLOR_YUV_BLACK);

				PipView_ScaleData(pDraw->p_src_img[0], REGION_MATCH_IMG, pDraw->p_dst_img, REGION_MATCH_IMG);
			}

			// rear image is smaller
			if (pDraw->p_src_img[1]) {
				dst_region.x = 0;
				dst_region.y = 0;
				dst_region.w = pDraw->p_dst_img->dim.w / 2;
				dst_region.h = pDraw->p_dst_img->dim.h / 2;

				#if (DUALCAM_PIP_BEHIND_FLIP == DISABLE)
					PipView_ScaleData(pDraw->p_src_img[1], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
				#else
					// flip rear image to destination buffer
					IRECT		dstRegion;

					// scale rear image to top-right corner of destination buffer (size 1/4)
					dstRegion.x = pDraw->p_dst_img->dim.w / 2;
					dstRegion.y = 0;
					dstRegion.w = ALIGN_CEIL_4(pDraw->p_dst_img->dim.w / 2);
					dstRegion.h = ALIGN_CEIL_4(pDraw->p_dst_img->dim.h / 2);
					PipView_ScaleData(pDraw->p_src_img[1], REGION_MATCH_IMG, pDraw->p_dst_img, &dstRegion);

					// flip and paste scaled image from top-right to top-left of destination buffer
					dstLocation.x = 0;
					dstLocation.y = 0;
					PipView_RotateData(pDraw->p_dst_img, &dstRegion, pDraw->p_dst_img, &dstLocation, HD_VIDEO_DIR_MIRRORX);

					// paste top-right of front image to destination buffer
					if (pDraw->p_src_img[0]) {
						IRECT        srcRegion;
                        srcRegion.x = pDraw->p_src_img[0]->dim.w / 2;
                        srcRegion.y = 0;
                        srcRegion.w = ALIGN_CEIL_4(pDraw->p_src_img[0]->dim.w / 2);
                        srcRegion.h = ALIGN_CEIL_4(pDraw->p_src_img[0]->dim.h / 2);
                        PipView_ScaleData(pDraw->p_src_img[0], &srcRegion, pDraw->p_dst_img, &dstRegion);
					}
				#endif
			}
		}
		#if 0
		else if (dual_cam == DUALCAM_CUSTOM_4)
		{ // Rx, Tx1, 1:1
			if(pDraw->p_dst_img->Width && pDraw->p_dst_img->Height){
				GxImg_FillData(pDraw->p_dst_img, NULL, COLOR_YUV_BLACK) ;
			}
			//DBG_DUMP("src[1] w=%d, %d, src[2] w=%d, %d, dst_img=%d, %d\r\n",pDraw->p_src_img[1]->Width,pDraw->p_src_img[1]->Height,pDraw->p_src_img[2]->Width,pDraw->p_src_img[2]->Height,pDraw->p_dst_img->Width,pDraw->p_dst_img->Height);
			// img1
			dst_region.x = 0;
			dst_region.y = 0;
			dst_region.w = pDraw->p_dst_img->Width / 2;
			dst_region.h = pDraw->p_dst_img->Height;

			if(pDraw->p_src_img[0] && pDraw->p_dst_img->Width && pDraw->p_dst_img->Height){
				GxImg_ScaleData(pDraw->p_src_img[0], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
			}
			// img2
			dst_region.x = pDraw->p_dst_img->Width / 2;
			dst_region.y = 0;
			if(pDraw->p_src_img[1] && pDraw->p_dst_img->Width && pDraw->p_dst_img->Height){
				#if (DUALCAM_PIP_BEHIND_FLIP == DISABLE)
				GxImg_ScaleData(pDraw->p_src_img[1], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
				#else
				IPOINT		dstLocation;
				IRECT  		SrcRegion;
				SrcRegion.x = 0;
				SrcRegion.y = 0;
				SrcRegion.w = pDraw->p_dst_img->Width/2;
				SrcRegion.h = pDraw->p_dst_img->Height;

				dstLocation.x = pDraw->p_dst_img->Width/2;
				dstLocation.y = 0;
				GxImg_RotatePasteData(pDraw->p_src_img[1], &SrcRegion, pDraw->p_dst_img, &dstLocation, GX_IMAGE_ROTATE_HRZ, GX_IMAGE_ROTATE_ENG2);
				#endif
			}
		}
		#endif
		else// if (dual_cam == DUALCAM_LR_FULL)
		{
			ISIZE	img_ratio;
			USIZE	disp_aspect_ratio = GxVideo_GetDeviceAspect(DOUT1);
			IRECT	src_region;

			if ((pDraw->p_dst_img != pDraw->p_src_img[0]) && (pDraw->p_dst_img != pDraw->p_src_img[1])) {
				//GxImg_FillData(pDraw->p_dst_img, NULL, COLOR_YUV_BLACK);
			}

			Movie_GetRecSize(0, &img_ratio);

			if ((img_ratio.w * disp_aspect_ratio.h) > (disp_aspect_ratio.w * img_ratio.h) ||
				(img_ratio.w * disp_aspect_ratio.h * 2) > (disp_aspect_ratio.w * img_ratio.h)) {

				if (pDraw->p_src_img[1]) {
					#if (DUALCAM_PIP_BEHIND_FLIP == DISABLE)
						// crop rear image and copy to right of destination buffer
						src_region.w = pDraw->p_dst_img->dim.w / 2;
						src_region.h = pDraw->p_dst_img->dim.h;
						src_region.x = ALIGN_CEIL_4((pDraw->p_src_img[1]->dim.w - src_region.w) / 2);
						src_region.y = 0;
						dst_region.w = pDraw->p_dst_img->dim.w / 2;
						dst_region.h = pDraw->p_dst_img->dim.h;
						dst_region.x = pDraw->p_dst_img->dim.w / 2;
						dst_region.y = 0;
						PipView_ScaleData(pDraw->p_src_img[1], &src_region, pDraw->p_dst_img, &dst_region);
					#else
						// crop rear image and copy to left of destination buffer
						src_region.w = pDraw->p_dst_img->dim.w / 2;
						src_region.h = pDraw->p_dst_img->dim.h;
						src_region.x = ALIGN_CEIL_4((pDraw->p_src_img[1]->dim.w - src_region.w) / 2);
						src_region.y = 0;
						dst_region.w = pDraw->p_dst_img->dim.w / 2;
						dst_region.h = pDraw->p_dst_img->dim.h;
						dst_region.x = 0;
						dst_region.y = 0;
						PipView_ScaleData(pDraw->p_src_img[1], &src_region, pDraw->p_dst_img, &dst_region);

						// flip and paste cropped rear image from left to right of destination buffer
						dstLocation.x = pDraw->p_dst_img->dim.w / 2;
						dstLocation.y = 0;
						PipView_RotateData(pDraw->p_dst_img, &dst_region, pDraw->p_dst_img, &dstLocation, HD_VIDEO_DIR_MIRRORX);
					#endif
				}

				if (pDraw->p_src_img[0]) {
					// crop front image and copy to left of destination buffer
					src_region.w = pDraw->p_dst_img->dim.w / 2;
					src_region.h = pDraw->p_dst_img->dim.h;
					src_region.x = ALIGN_CEIL_4((pDraw->p_src_img[0]->dim.w - src_region.w) / 2);
					src_region.y = 0;
					dst_region.w = pDraw->p_dst_img->dim.w / 2;
					dst_region.h = pDraw->p_dst_img->dim.h;
					dst_region.x = 0;
					dst_region.y = 0;
					PipView_ScaleData(pDraw->p_src_img[0], &src_region, pDraw->p_dst_img, &dst_region);
				}
			} else {

				if (pDraw->p_src_img[1]) {
					#if (DUALCAM_PIP_BEHIND_FLIP == DISABLE)
						// crop rear image and copy to right of destination buffer
						src_region.w = pDraw->p_dst_img->dim.w / 2;
						src_region.h = pDraw->p_dst_img->dim.h;
						src_region.x = 0;
						src_region.y = ALIGN_CEIL_4((pDraw->p_src_img[1]->dim.h - src_region.h) / 2);
						dst_region.w = pDraw->p_dst_img->dim.w / 2;
						dst_region.h = pDraw->p_dst_img->dim.h;
						dst_region.x = pDraw->p_dst_img->dim.w / 2;
						dst_region.y = 0;
						PipView_ScaleData(pDraw->p_src_img[1], &src_region, pDraw->p_dst_img, &dst_region);
					#else
						// crop rear image and copy to left of destination buffer
						src_region.w = pDraw->p_dst_img->dim.w / 2;
						src_region.h = pDraw->p_dst_img->dim.h;
						src_region.x = 0;
						src_region.y = ALIGN_CEIL_4((pDraw->p_src_img[1]->dim.h - src_region.h) / 2);
						dst_region.w = pDraw->p_dst_img->dim.w / 2;
						dst_region.h = pDraw->p_dst_img->dim.h;
						dst_region.x = 0;
						dst_region.y = 0;
						PipView_ScaleData(pDraw->p_src_img[1], &src_region, pDraw->p_dst_img, &dst_region);

						// flip and paste cropped rear image from left to right of destination buffer
						dstLocation.x = pDraw->p_dst_img->dim.w / 2;
						dstLocation.y = 0;
						PipView_RotateData(pDraw->p_dst_img, &dst_region, pDraw->p_dst_img, &dstLocation, HD_VIDEO_DIR_MIRRORX);
					#endif
				}

				if (pDraw->p_src_img[0]) {
					// crop front image and copy to left of destination buffer
					src_region.w = pDraw->p_dst_img->dim.w / 2;
					src_region.h = pDraw->p_dst_img->dim.h;
					src_region.x = 0;
					src_region.y = ALIGN_CEIL_4((pDraw->p_src_img[0]->dim.h - src_region.h) / 2);
					dst_region.w = pDraw->p_dst_img->dim.w / 2;
					dst_region.h = pDraw->p_dst_img->dim.h;
					dst_region.x = 0;
					dst_region.y = 0;
					PipView_ScaleData(pDraw->p_src_img[0], &src_region, pDraw->p_dst_img, &dst_region);
				}
			}
		}
	}

	return E_OK;
}

#if 1 // TODO!!!
INT32 PipView_OnDraw_3sensor(APPDISP_VIEW_DRAW *pDraw) //PIP = Picture In Picture
{
	IRECT            dst_region;
	#if (DUALCAM_PIP_BEHIND_FLIP)
	IPOINT	dstLocation = {0, 0};
	#endif

	if (1){//pDraw->viewcnt == 3){

		if(UI_GetData(FL_DUAL_CAM) == DUALCAM_BOTH){
			if(pDraw->p_dst_img->dim.w  && pDraw->p_dst_img->dim.h){
				PipView_FillDataBlack(pDraw->p_dst_img, NULL) ;
			}

			// img1
			dst_region.x = 0;
			dst_region.y = 0;
			dst_region.w = pDraw->p_dst_img->dim.w / 2;
			dst_region.h = pDraw->p_dst_img->dim.h / 2;
			if(pDraw->p_src_img[0] && pDraw->p_dst_img->dim.w && pDraw->p_dst_img->dim.h){
				PipView_ScaleData(pDraw->p_src_img[0], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
			}
			// img2
			dst_region.x = pDraw->p_dst_img->dim.w / 2;
			dst_region.y = 0;
			if(pDraw->p_src_img[1] && pDraw->p_dst_img->dim.w && pDraw->p_dst_img->dim.h){
				#if (DUALCAM_PIP_BEHIND_FLIP == DISABLE)
				PipView_ScaleData(pDraw->p_src_img[1], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
				#else
				// flip rear image to destination buffer
				IRECT		dstRegion;

				// scale rear image to top-right corner of destination buffer (size 1/4)
				dstRegion.x = 0;//pDraw->p_dst_img->dim.w / 2;
				dstRegion.y = 0;
				dstRegion.w = ALIGN_CEIL_4(pDraw->p_dst_img->dim.w / 2);
				dstRegion.h = ALIGN_CEIL_4(pDraw->p_dst_img->dim.h / 2);
				PipView_ScaleData(pDraw->p_src_img[1], REGION_MATCH_IMG, pDraw->p_dst_img, &dstRegion);

				// flip and paste scaled image from top-right to top-left of destination buffer
				dstLocation.x = pDraw->p_dst_img->dim.w / 2;//0;
				dstLocation.y = 0;
				PipView_RotateData(pDraw->p_dst_img, &dstRegion, pDraw->p_dst_img, &dstLocation, HD_VIDEO_DIR_MIRRORX);

				// paste top-right of front image to destination buffer
				if (pDraw->p_src_img[0]) {
					PipView_ScaleData(pDraw->p_src_img[0], &dstRegion, pDraw->p_dst_img, &dstRegion);
				}
				#endif
			}
			// img3
			dst_region.x = 0;
			dst_region.y = pDraw->p_dst_img->dim.h / 2;
			if(pDraw->p_src_img[2] && pDraw->p_dst_img->dim.w && pDraw->p_dst_img->dim.h){
				#if (DUALCAM_PIP_BEHIND_FLIP == DISABLE)
				PipView_ScaleData(pDraw->p_src_img[2], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
				#else
				IRECT		dstRegion;

				// scale rear image to top-right corner of destination buffer (size 1/4)
				dstRegion.x = pDraw->p_dst_img->dim.w / 2;
				dstRegion.y = dst_region.y;
				dstRegion.w = ALIGN_CEIL_4(pDraw->p_dst_img->dim.w / 2);
				dstRegion.h = ALIGN_CEIL_4(pDraw->p_dst_img->dim.h / 2);
				PipView_ScaleData(pDraw->p_src_img[2], REGION_MATCH_IMG, pDraw->p_dst_img, &dstRegion);

				// flip and paste scaled image from top-right to top-left of destination buffer
				dstLocation.x = 0;
				dstLocation.y = dst_region.y;
				PipView_RotateData(pDraw->p_dst_img, &dstRegion, pDraw->p_dst_img, &dstLocation, HD_VIDEO_DIR_MIRRORX);

				// paste top-right of front image to destination buffer
				//if (pDraw->p_src_img[0]) {
				//	PipView_ScaleData(pDraw->p_src_img[0], &dstRegion, pDraw->p_dst_img, &dstRegion);
				//}
				PipView_FillDataBlack(pDraw->p_dst_img, &dstRegion) ;
				#endif
			}
		}else if(UI_GetData(FL_DUAL_CAM) == DUALCAM_FRONT){//Tx1
			if(pDraw->p_src_img[1] && pDraw->p_dst_img != pDraw->p_src_img[1]){
			        PipView_FillDataBlack(pDraw->p_dst_img, NULL) ;
			}
			// img1
			dst_region.x = 0;
			dst_region.y = 0;
			dst_region.w = pDraw->p_dst_img->dim.w;
			dst_region.h = pDraw->p_dst_img->dim.h;
			if(pDraw->p_src_img[1] && pDraw->p_dst_img != pDraw->p_src_img[1]){
			        PipView_ScaleData(pDraw->p_src_img[1], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
			}

		}else if(UI_GetData(FL_DUAL_CAM) == DUALCAM_BEHIND){//Tx2
			if(pDraw->p_src_img[2] && pDraw->p_dst_img != pDraw->p_src_img[2]){
			        PipView_FillDataBlack(pDraw->p_dst_img, NULL) ;
			}

			// img2
			dst_region.x = 0;
			dst_region.y = 0;
			dst_region.w = pDraw->p_dst_img->dim.w;
			dst_region.h = pDraw->p_dst_img->dim.h;
			if(pDraw->p_src_img[2] && pDraw->p_dst_img != pDraw->p_src_img[2]){
			        PipView_ScaleData(pDraw->p_src_img[2], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
			}
		}else if(UI_GetData(FL_DUAL_CAM) == DUALCAM_BOTH2){//Rx Sensor
			//if(pDraw->p_src_img[0] && pDraw->p_dst_img != pDraw->p_src_img[0])
			//        PipView_FillDataBlack(pDraw->p_dst_img, NULL, COLOR_YUV_BLACK) ;

			// img0
			dst_region.x = 0;
			dst_region.y = 0;
			dst_region.w = pDraw->p_dst_img->dim.w;
			dst_region.h = pDraw->p_dst_img->dim.h;
			if(pDraw->p_src_img[0] && pDraw->p_dst_img != pDraw->p_src_img[0]){
			        PipView_ScaleData(pDraw->p_src_img[0], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
			}
		}
		#if 0//_TODO
		else  if(UI_GetData(FL_DUAL_CAM) == DUALCAM_CUSTOM_1){ //Tx1, Tx2
			//if(pDraw->p_dst_img->Width && pDraw->p_dst_img->Height){
			//	GxImg_FillData(pDraw->p_dst_img, NULL, COLOR_YUV_BLACK) ;
			//}
			//DBG_DUMP("src[1] w=%d, %d, src[2] w=%d, %d, dst_img=%d, %d\r\n",pDraw->p_src_img[1]->Width,pDraw->p_src_img[1]->Height,pDraw->p_src_img[2]->Width,pDraw->p_src_img[2]->Height,pDraw->p_dst_img->Width,pDraw->p_dst_img->Height);
			// img1
			dst_region.x = 0;
			dst_region.y = 0;
			dst_region.w = pDraw->p_dst_img->Width / 2;
			dst_region.h = pDraw->p_dst_img->Height;

			if(pDraw->p_src_img[1] && pDraw->p_dst_img->Width && pDraw->p_dst_img->Height){
				PipView_ScaleData(pDraw->p_src_img[1], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
			}
			// img2
			dst_region.x = pDraw->p_dst_img->Width / 2;
			dst_region.y = 0;
			if(pDraw->p_src_img[2] && pDraw->p_dst_img->Width && pDraw->p_dst_img->Height){
				PipView_ScaleData(pDraw->p_src_img[2], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
			}
		}else  if(UI_GetData(FL_DUAL_CAM) == DUALCAM_CUSTOM_2){ //Tx1, Rx, Tx2, 1:2:1
			//if(pDraw->p_dst_img->Width && pDraw->p_dst_img->Height){
			//	GxImg_FillData(pDraw->p_dst_img, NULL, COLOR_YUV_BLACK) ;
			//}
			//DBG_DUMP("src[0] w=%d, %d, src[1] w=%d, %d, src[2] w=%d, %d, dst_img=%d, %d\r\n",pDraw->p_src_img[0]->Width,pDraw->p_src_img[0]->Height,pDraw->p_src_img[1]->Width,pDraw->p_src_img[1]->Height,pDraw->p_src_img[2]->Width,pDraw->p_src_img[2]->Height,pDraw->p_dst_img->Width,pDraw->p_dst_img->Height);
			// img1
			UINT32 WidthUnit=ALIGN_CEIL_4(pDraw->p_dst_img->Width / 4);
			dst_region.x = WidthUnit;
			dst_region.y = 0;
			dst_region.w = 2*WidthUnit;
			dst_region.h = pDraw->p_dst_img->Height;
			if(pDraw->p_src_img[0] && pDraw->p_dst_img->Width && pDraw->p_dst_img->Height){
				PipView_ScaleData(pDraw->p_src_img[0], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
			}
			// img2
			dst_region.x = 0;
			dst_region.y = 0;
			dst_region.w = WidthUnit;
			if(pDraw->p_src_img[1] && pDraw->p_dst_img->Width && pDraw->p_dst_img->Height){
				PipView_ScaleData(pDraw->p_src_img[1], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
			}
			// img3
			dst_region.x = 3*WidthUnit;
			dst_region.y = 0;
			dst_region.w = WidthUnit;
			if(pDraw->p_src_img[2] && pDraw->p_dst_img->Width && pDraw->p_dst_img->Height){
				PipView_ScaleData(pDraw->p_src_img[2], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
			}
		}else  if(UI_GetData(FL_DUAL_CAM) == DUALCAM_CUSTOM_3){ //Tx1, Rx, Tx2, 1:1:1
			if(pDraw->p_dst_img->Width && pDraw->p_dst_img->Height){
				GxImg_FillData(pDraw->p_dst_img, NULL, COLOR_YUV_BLACK) ;
			}
			//DBG_DUMP("src[0] w=%d, %d, src[1] w=%d, %d, src[2] w=%d, %d, dst_img=%d, %d\r\n",pDraw->p_src_img[0]->Width,pDraw->p_src_img[0]->Height,pDraw->p_src_img[1]->Width,pDraw->p_src_img[1]->Height,pDraw->p_src_img[2]->Width,pDraw->p_src_img[2]->Height,pDraw->p_dst_img->Width,pDraw->p_dst_img->Height);
			// img1
			UINT32 WidthUnit=ALIGN_CEIL_4(pDraw->p_dst_img->Width / 3);
			dst_region.x = WidthUnit;
			dst_region.y = 0;
			dst_region.w = WidthUnit;
			dst_region.h = pDraw->p_dst_img->Height;
			if(pDraw->p_src_img[0] && pDraw->p_dst_img->Width && pDraw->p_dst_img->Height){
				PipView_ScaleData(pDraw->p_src_img[0], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
			}
			// img2
			dst_region.x = 0;
			dst_region.y = 0;
			dst_region.w = WidthUnit;
			if(pDraw->p_src_img[1] && pDraw->p_dst_img->Width && pDraw->p_dst_img->Height){
				PipView_ScaleData(pDraw->p_src_img[1], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
			}
			// img3
			dst_region.x = 2*WidthUnit;
			dst_region.y = 0;
			dst_region.w = WidthUnit;
			if(pDraw->p_src_img[2] && pDraw->p_dst_img->Width && pDraw->p_dst_img->Height){
				PipView_ScaleData(pDraw->p_src_img[2], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
			}
		}
		#endif
	}
	return E_OK;
}
INT32 PipView_OnDraw_4sensor_2view(APPDISP_VIEW_DRAW *pDraw) //PIP = Picture In Picture
{
	IRECT            dst_region;

    // img2
	dst_region.x = 0;
	dst_region.y = 0;
	dst_region.w = pDraw->p_dst_img->dim.w / 2;
	dst_region.h = pDraw->p_dst_img->dim.h / 2;
	if(pDraw->p_src_img[1]){
		PipView_ScaleData(pDraw->p_src_img[1], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
	}
	return E_OK;
}

INT32 PipView_OnDraw_4sensor(APPDISP_VIEW_DRAW *pDraw) //PIP = Picture In Picture
{
	IRECT            dst_region;
	//GxImg_FillData(pDraw->p_dst_img, NULL, COLOR_YUV_BLACK) ;

	// img1
	dst_region.x = 0;
	dst_region.y = 0;
	dst_region.w = pDraw->p_dst_img->dim.w / 2;
	dst_region.h = pDraw->p_dst_img->dim.h / 2;
	if(pDraw->p_src_img[0]){
		PipView_ScaleData(pDraw->p_src_img[0], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
	}
	// img2
	dst_region.x = pDraw->p_dst_img->dim.w / 2;
	dst_region.y = 0;
	if(pDraw->p_src_img[1]){
		PipView_ScaleData(pDraw->p_src_img[1], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
	}
	// img3
	dst_region.x = 0;
	dst_region.y = pDraw->p_dst_img->dim.h / 2;
	if(pDraw->p_src_img[2]){
		PipView_ScaleData(pDraw->p_src_img[2], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
	}
	// img4
	dst_region.x = pDraw->p_dst_img->dim.w / 2;
	dst_region.y = pDraw->p_dst_img->dim.h / 2;
	if(pDraw->p_src_img[3]){
		PipView_ScaleData(pDraw->p_src_img[3], REGION_MATCH_IMG, pDraw->p_dst_img, &dst_region);
	}
	return E_OK;
}
#endif

#if 1
INT32 PipView_OnDraw(APPDISP_VIEW_DRAW *pDraw)
{
	if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_MOVIE || System_GetState(SYS_STATE_NEXTMODE) == PRIMARY_MODE_MOVIE) {
#if (SENSOR_CAPS_COUNT == 4)
		if (pDraw->viewcnt == 2){
		    PipView_OnDraw_4sensor_2view(pDraw);
		} else{
		    PipView_OnDraw_4sensor(pDraw);
		}
#elif (((SENSOR_CAPS_COUNT& SENSOR_ON_MASK) + ETH_REARCAM_CAPS_COUNT) ==3)
		PipView_OnDraw_3sensor(pDraw);
//#elif (((SENSOR_CAPS_COUNT& SENSOR_ON_MASK) + ETH_REARCAM_CAPS_COUNT) ==2)
#else
		PipView_OnDraw_2sensor(pDraw);
//#else
//		DBG_ERR("Not Support %d sensor\r\n",SENSOR_CAPS_COUNT);
#endif
	}else{
#if (SENSOR_CAPS_COUNT == 4)
	    if (pDraw->viewcnt == 2){
	        PipView_OnDraw_4sensor_2view(pDraw);
	    } else{
	        PipView_OnDraw_4sensor(pDraw);
	    }
#elif (SENSOR_CAPS_COUNT == 3)
	    PipView_OnDraw_3sensor(pDraw);
#elif (SENSOR_CAPS_COUNT == 2)
	    PipView_OnDraw_2sensor(pDraw);
#else
	    DBG_ERR("Not Support %d sensor\r\n",SENSOR_CAPS_COUNT);
#endif
	}
    return E_OK;
}
#else
INT32 PipView_OnDraw(APPDISP_VIEW_DRAW *pDraw)
{
	if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_MOVIE || System_GetState(SYS_STATE_NEXTMODE) == PRIMARY_MODE_MOVIE) {
		PipView_OnDraw_2sensor(pDraw);
	}

	return E_OK;
}
#endif
