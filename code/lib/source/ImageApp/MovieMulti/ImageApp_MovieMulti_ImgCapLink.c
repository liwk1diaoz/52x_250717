#include "ImageApp_MovieMulti_int.h"

/// ========== Cross file variables ==========
UINT32 IsImgCapLinkOpened[MAX_IMGCAP_PATH] = {0};
MOVIE_IMGCAP_LINK ImgCapLink[MAX_IMGCAP_PATH];
MOVIE_IMGCAP_LINK_STATUS ImgCapLinkStatus[MAX_IMGCAP_PATH] = {0};
HD_VIDEOENC_PATH_CONFIG imgcap_venc_path_cfg[MAX_IMGCAP_PATH] = {0};
HD_VIDEOENC_IN  imgcap_venc_in_param[MAX_IMGCAP_PATH] = {0};
HD_VIDEOENC_OUT2 imgcap_venc_out_param[MAX_IMGCAP_PATH] = {0};
HD_VIDEOENC_BUFINFO imgcap_venc_bs_buf[MAX_IMGCAP_PATH] = {0};
UINT32 imgcap_venc_bs_buf_va[MAX_IMGCAP_PATH] = {0};
QUEUE_BUFFER_QUEUE ImgCapJobQueueClear = {0};
QUEUE_BUFFER_QUEUE ImgCapJobQueueReady = {0};
UINT32 ImgCapExifFuncEn = FALSE;
UINT32 imgcap_thum_with_exif = FALSE;
UINT32 imgcap_thum_no_auto_scaling[MAX_IMG_PATH][2] = {0};
UINT32 imgcap_rawenc_pa[_CFG_ETHCAM_ID_MAX] = {0}, imgcap_rawenc_va[_CFG_ETHCAM_ID_MAX] = {0}, imgcap_rawenc_size[_CFG_ETHCAM_ID_MAX] = {0};
UINT32 imgcap_use_venc_buf = FALSE;
UINT32 imgcap_venc_shared_sout_pa = 0;
UINT32 imgcap_venc_shared_sout_va = 0;
UINT32 imgcap_venc_shared_sout_size = 0;
UINT32 imgcap_venc_shared_sout_ddr_id = 0;
UINT32 imgcap_venc_shared_sout_count = 0;
MOVIEMULTI_JPG_QUALITY imgcap_jpg_quality = {50, 0};
/// ========== Noncross file variables ==========
static THREAD_HANDLE iamovie_imgcap_tsk_id;
static UINT32 imgcap_tsk_run[MAX_IMGCAP_PATH], is_imgcap_tsk_running[MAX_IMGCAP_PATH];
static IMGCAP_JOB_ELEMENT ImgCapJobElement[IMGCAP_JOBQUEUENUM];
static UINT32 exifthumb_pa = 0, exifthumb_va = 0, exifthumb_size = 0;
static UINT32 pending_status = 0;

#define PRI_IAMOVIE_IMGCAP           12
#define STKSIZE_IAMOVIE_IMGCAP       8192

static void _Cal_Jpg_Size(USIZE *psrc, USIZE *pdest , URECT *pdestwin, UINT32 convert)
{
	SIZECONVERT_INFO scale_info = {0};

	scale_info.uiSrcWidth  = psrc->w;
	scale_info.uiSrcHeight = psrc->h;
	scale_info.uiDstWidth  = pdest->w;
	scale_info.uiDstHeight = pdest->h;
	scale_info.uiDstWRatio = pdest->w;
	scale_info.uiDstHRatio = pdest->h;
	scale_info.alignType   = SIZECONVERT_ALIGN_FLOOR_32;
	if (convert) {
		DisplaySizeConvert(&scale_info);
		pdestwin->x = scale_info.uiOutX;
		pdestwin->y = scale_info.uiOutY;
		pdestwin->w = scale_info.uiOutWidth;
		pdestwin->h = scale_info.uiOutHeight;
	} else {
		pdestwin->x = 0;
		pdestwin->y = 0;
		pdestwin->w = pdest->w;
		pdestwin->h = pdest->h;
	}
	//DBG_DUMP("Dst: x %d, y %d, w %d, h %d\r\n", pdestwin->x, pdestwin->y, pdestwin->w, pdestwin->h);
}

static ER _alloc_exifthumbbuf(void)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;
	UINT32 pa;
	void *va;
	HD_COMMON_MEM_DDR_ID ddr_id;

	if (exifthumb_va) {
		DBG_WRN("exifthumbbuf is already allocated.\r\n");
		return ret;
	}

	ddr_id = dram_cfg.video_encode;
	exifthumb_size = 65536; //VDO_YUV_BUFSIZE(IMGCAP_THUMB_W, IMGCAP_THUMB_H, HD_VIDEO_PXLFMT_YUV420);
	if ((hd_ret = hd_common_mem_alloc("exifthumb", &pa, (void **)&va, exifthumb_size, ddr_id)) != HD_OK) {
		DBG_ERR("hd_common_mem_alloc failed(%d)\r\n", hd_ret);
		exifthumb_va = 0;
		exifthumb_pa = 0;
		exifthumb_size = 0;
		ret = E_SYS;
	} else {
		exifthumb_va = (UINT32)va;
		exifthumb_pa = (UINT32)pa;
		ret = E_OK;
	}
	return ret;
}

static ER _free_exifthumbbuf(void)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;

	if (exifthumb_va == 0) {
		DBG_WRN("exifthumbbuf is already freed.\r\n");
		return ret;
	}

	if ((hd_ret = hd_common_mem_free(exifthumb_pa, (void *)exifthumb_va)) != HD_OK) {
		DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
		ret = E_SYS;
	}
	exifthumb_pa = 0;
	exifthumb_va = 0;
	exifthumb_size = 0;

	return ret;
}

static HD_COMMON_MEM_VB_BLK _Scale_YUV(VF_GFX_SCALE *pscale, HD_VIDEO_FRAME *psrc, USIZE *pdest, URECT *pdestwin, BOOL use_private_buf)
{
	HD_COMMON_MEM_VB_BLK blk;
	UINT32 blk_size, pa;
	UINT32 addr[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 loff[HD_VIDEO_MAX_PLANE] = {0};
	VF_GFX_DRAW_RECT fill_rect = {0};
	HD_RESULT hd_ret;
	UINT32 two_pass_scaling = FALSE;
	UINT32 temp_w, temp_h, temp_blk_size;
	void *va;
	UINT32 tempbuf_pa = 0, tempbuf_va = 0;
	HD_COMMON_MEM_DDR_ID ddr_id;

	if ((psrc->dim.w >= (16 * pdest->w)) || (psrc->dim.h >= (16 * pdest->h))) {
		DBG_DUMP("Use two pass scale(%d,%d)->(%d,%d)\r\n", psrc->dim.w, psrc->dim.h, pdest->w, pdest->h);
		temp_w = 640;
		temp_h = ALIGN_CEIL_4(psrc->dim.h * temp_w / psrc->dim.w);
		temp_blk_size = VDO_YUV_BUFSIZE(temp_w, temp_h, HD_VIDEO_PXLFMT_YUV420);
		ddr_id = dram_cfg.video_encode;
		if ((hd_ret = hd_common_mem_alloc("imgcaptmp", &pa, (void **)&va, temp_blk_size, ddr_id)) != HD_OK) {
			DBG_ERR("hd_common_mem_alloc failed(%d)\r\n", hd_ret);
			tempbuf_va = 0;
			tempbuf_pa = 0;
			temp_blk_size = 0;
		} else {
			tempbuf_va = (UINT32)va;
			tempbuf_pa = (UINT32)pa;

			// set src
			addr[0] = psrc->phy_addr[HD_VIDEO_PINDEX_Y];
			loff[0] = psrc->loff[HD_VIDEO_PINDEX_Y];
			addr[1] = psrc->phy_addr[HD_VIDEO_PINDEX_UV];
			loff[1] = psrc->loff[HD_VIDEO_PINDEX_UV];
			if ((hd_ret = vf_init_ex(&(pscale->src_img), psrc->dim.w, psrc->dim.h, psrc->pxlfmt, loff, addr)) != HD_OK) {
				DBG_ERR("vf_init_ex src failed(%d)\r\n", hd_ret);
			}
			// set dest
			addr[0] = tempbuf_pa;
			loff[0] = ALIGN_CEIL_4(temp_w);
			addr[1] = tempbuf_pa + loff[0] * temp_h;
			loff[1] = ALIGN_CEIL_4(temp_w);
			if ((hd_ret = vf_init_ex(&(pscale->dst_img), temp_w, temp_h, HD_VIDEO_PXLFMT_YUV420, loff, addr)) != HD_OK) {
				DBG_ERR("vf_init_ex dst failed(%d)\r\n", hd_ret);
			}
			// set config
			pscale->engine = 0;
			pscale->src_region.x = 0;
			pscale->src_region.y = 0;
			pscale->src_region.w = psrc->dim.w;
			pscale->src_region.h = psrc->dim.h;
			pscale->dst_region.x = 0;
			pscale->dst_region.y = 0;
			pscale->dst_region.w = temp_w;
			pscale->dst_region.h = temp_h;
			pscale->quality = HD_GFX_SCALE_QUALITY_NULL;
			vf_gfx_scale(pscale, 0);

			two_pass_scaling = TRUE;
		}
	}

	blk_size = VDO_YUV_BUFSIZE(pdest->w, pdest->h, HD_VIDEO_PXLFMT_YUV420);

	if (use_private_buf) {
		if (blk_size > exifthumb_size) {
			DBG_ERR("Request blk_size(%d) > exifthumb_size(%d)\r\n", blk_size, exifthumb_size);
			return HD_COMMON_MEM_VB_INVALID_BLK;
		}
		pa = exifthumb_pa;
		blk = -2;
	} else {
		ddr_id = dram_cfg.video_proc[0];      // common pool is defined by video_proc
		if ((blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id)) == HD_COMMON_MEM_VB_INVALID_BLK) {
			DBG_ERR("hd_common_mem_get_block fail(%d)\r\n", blk);
			return HD_COMMON_MEM_VB_INVALID_BLK;
		}
		if ((pa = hd_common_mem_blk2pa(blk)) == 0) {
			DBG_ERR("hd_common_mem_blk2pa fail\r\n");
			if ((hd_ret = hd_common_mem_release_block(blk)) != HD_OK) {
				DBG_ERR("hd_common_mem_release_block fail(%d)\r\n", hd_ret);
			}
			return HD_COMMON_MEM_VB_INVALID_BLK;
		}
	}

	// set src
	if (two_pass_scaling == FALSE) {
		addr[0] = psrc->phy_addr[HD_VIDEO_PINDEX_Y];
		loff[0] = psrc->loff[HD_VIDEO_PINDEX_Y];
		addr[1] = psrc->phy_addr[HD_VIDEO_PINDEX_UV];
		loff[1] = psrc->loff[HD_VIDEO_PINDEX_UV];
		if ((hd_ret = vf_init_ex(&(pscale->src_img), psrc->dim.w, psrc->dim.h, psrc->pxlfmt, loff, addr)) != HD_OK) {
			DBG_ERR("vf_init_ex src failed(%d)\r\n", hd_ret);
		}
	} else {
		addr[0] = tempbuf_pa;
		loff[0] = ALIGN_CEIL_4(temp_w);
		addr[1] = tempbuf_pa + loff[0] * temp_h;
		loff[1] = ALIGN_CEIL_4(temp_w);
		if ((hd_ret = vf_init_ex(&(pscale->src_img), temp_w, temp_h, psrc->pxlfmt, loff, addr)) != HD_OK) {
			DBG_ERR("vf_init_ex src failed(%d)\r\n", hd_ret);
		}
	}
	// set dest
	addr[0] = pa;
	loff[0] = ALIGN_CEIL_4(pdest->w);
	addr[1] = pa + loff[0] * pdest->h;
	loff[1] = ALIGN_CEIL_4(pdest->w);
	if ((hd_ret = vf_init_ex(&(pscale->dst_img), pdest->w, pdest->h, HD_VIDEO_PXLFMT_YUV420, loff, addr)) != HD_OK) {
		DBG_ERR("vf_init_ex dst failed(%d)\r\n", hd_ret);
	}


	if ((pdest->w != pdestwin->w ) ||(pdest->h != pdestwin->h)) {
		// clear buffer by black
		memcpy((void *)&(fill_rect.dst_img), (void *)&(pscale->dst_img), sizeof(HD_VIDEO_FRAME));
		fill_rect.color = COLOR_RGB_BLACK;
		fill_rect.rect.x = 0;
		fill_rect.rect.y = 0;
		fill_rect.rect.w = pdest->w;
		fill_rect.rect.h = pdest->h;
		fill_rect.type = HD_GFX_RECT_SOLID;
		fill_rect.thickness = 0;
		fill_rect.engine = 0;
		if ((hd_ret = vf_gfx_draw_rect(&fill_rect)) != HD_OK) {
			DBG_ERR("vf_gfx_draw_rect failed(%d)\r\n", hd_ret);
		}
	}

	// set config
	pscale->engine = 0;
	pscale->src_region.x = 0;
	pscale->src_region.y = 0;
	if (two_pass_scaling == FALSE) {
		pscale->src_region.w = psrc->dim.w;
		pscale->src_region.h = psrc->dim.h;
	} else {
		pscale->src_region.w = temp_w;
		pscale->src_region.h = temp_h;
	}
	pscale->dst_region.x = pdestwin->x;
	pscale->dst_region.y = pdestwin->y;
	pscale->dst_region.w = pdestwin->w;
	pscale->dst_region.h = pdestwin->h;
	pscale->quality = HD_GFX_SCALE_QUALITY_NULL;
	vf_gfx_scale(pscale, 0);

	pscale->dst_img.count 	= 0;
	pscale->dst_img.timestamp = hd_gettime_us();
	pscale->dst_img.blk		= blk;

	if (tempbuf_va) {
		if ((hd_ret = hd_common_mem_free(tempbuf_pa, (void *)tempbuf_va)) != HD_OK) {
			DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
		}
		tempbuf_pa = 0;
		tempbuf_va = 0;
	}
	return blk;
}

static ER _get_imgcap_buf_info(UINT32 idx)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;

	if (imgcap_venc_bs_buf_va[idx] == 0) {
		if ((hd_ret = hd_videoenc_get(ImgCapLink[idx].venc_p[0], HD_VIDEOENC_PARAM_BUFINFO, &(imgcap_venc_bs_buf[idx]))) != HD_OK) {
			DBG_ERR("hd_videoenc_get fail(%d)\n", hd_ret);
			ret = E_SYS;
		} else {
			imgcap_venc_bs_buf_va[idx] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, imgcap_venc_bs_buf[idx].buf_info.phy_addr, imgcap_venc_bs_buf[idx].buf_info.buf_size);
		}
	}
	return ret;
}

ER _ImageApp_MovieMulti_AllocSharedSrcOutBuf(UINT32 buf_ddr_id, UINT32 buf_size, UINT32 *buf_pa, UINT32 *buf_va)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	UINT32 pa;
	void *va;
	UINT32 blk_size = imgcap_venc_shared_sout_size;

	if (buf_size > imgcap_venc_shared_sout_size) {
		DBG_ERR("required size(0x%08x)>claim size(0x%08x)\r\n", buf_size, imgcap_venc_shared_sout_size);
		return ret;
	}

	if (imgcap_venc_shared_sout_va == 0) {                                      // shared buffer not allocated yet
		if ((hd_ret = hd_common_mem_alloc("ssoutbuf", &pa, (void **)&va, blk_size, buf_ddr_id)) != HD_OK) {
			*buf_pa     = 0;
			*buf_va     = 0;
			DBG_ERR("Allocated shared source out buffer fail(%d)! wanted size=%x\r\n", hd_ret, blk_size);
			ret = E_SYS;
		} else {
			imgcap_venc_shared_sout_ddr_id = buf_ddr_id;
			imgcap_venc_shared_sout_pa     = (UINT32)pa;
			imgcap_venc_shared_sout_va     = (UINT32)va;

			*buf_pa     = imgcap_venc_shared_sout_pa;
			*buf_va     = imgcap_venc_shared_sout_va;
			imgcap_venc_shared_sout_count ++;
			DBG_DUMP("Allocated shared source out buffer OK! wanted=0x%08x, allocated=0x%08x(cnt=%d)\r\n", buf_size, imgcap_venc_shared_sout_size, imgcap_venc_shared_sout_count);
			ret = E_OK;
		}
	} else {
		*buf_pa     = imgcap_venc_shared_sout_pa;
		*buf_va     = imgcap_venc_shared_sout_va;
		imgcap_venc_shared_sout_count ++;
		DBG_DUMP("Shared source out buffer already allocated! wanted=0x%08x, allocated=0x%08x(cnt=%d)\r\n", buf_size, imgcap_venc_shared_sout_size, imgcap_venc_shared_sout_count);
		ret = E_OK;
	}
	return ret;
}

ER _ImageApp_MovieMulti_FreeSharedSrcOutBuf(void)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;

	if (imgcap_venc_shared_sout_count == 0) {
		DBG_ERR("Shared source out buffer already freed!\r\n");
		return E_SYS;
	}

	imgcap_venc_shared_sout_count --;

	if (imgcap_venc_shared_sout_count == 0) {
		if (imgcap_venc_shared_sout_va) {
			if ((hd_ret = hd_common_mem_free(imgcap_venc_shared_sout_pa, (void *)imgcap_venc_shared_sout_va)) != HD_OK) {
				DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
				ret = E_SYS;
			}
			imgcap_venc_shared_sout_pa     = 0;
			imgcap_venc_shared_sout_va     = 0;
			imgcap_venc_shared_sout_ddr_id = 0;
		}
	}
	return ret;
}

#define YUV_NORMAL                      0
#define YUV_COMPRESS_USE_SOUT           1
#define YUV_COMPRESS_NO_SOUT            2

static HD_PATH_ID _GetVideoFrameForImgCap(UINT32 idx, UINT32 tbl, HD_VIDEO_FRAME *p_frame)
{
	HD_RESULT hd_ret;
	FLGPTN uiFlag;

	if (img_venc_imgcap_yuvsrc[idx][tbl] == MOVIE_IMGCAP_YUVSRC_MAIN) {
		DBG_WRN("Thumb:[%d][%d] not support when yuv compressed without sourceout.\r\n", idx, tbl);
		return 0;
	}
	if (_LinkCheckStatus(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][img_venc_imgcap_yuvsrc[idx][tbl]])) {
		if ((img_manual_push_vdo_frame[idx][tbl] == TRUE) && (img_venc_imgcap_yuvsrc[idx][tbl] == tbl)) {
			_ImageApp_MovieMulti_GetVideoFrameForImgCap(idx, tbl, p_frame);
			if (vos_flag_wait_timeout(&uiFlag, IAMOVIE_FLG_ID2, FLGIAMOVIE2_RAWENC_FRM_WAIT, TWF_ORW, vos_util_msec_to_tick(300)) != E_OK) {
				DBG_ERR("_ImageApp_MovieMulti_GetVideoFrameForImgCap(%d,%d) fail\r\n", idx, tbl);
				return 0;
			}
		} else {
			if ((hd_ret = hd_videoproc_pull_out_buf(ImgLink[idx].vproc_p[img_vproc_splitter[idx]][img_venc_imgcap_yuvsrc[idx][tbl]], p_frame, 500)) != HD_OK) {
				if ((hd_ret == HD_ERR_TIMEDOUT) && ((img_venc_imgcap_yuvsrc[idx][tbl] == IAMOVIE_VPRC_EX_DISP) || (img_venc_imgcap_yuvsrc[idx][tbl] == IAMOVIE_VPRC_EX_WIFI))) {
					_ImageApp_MovieMulti_GetVideoFrameForImgCap(idx, img_venc_imgcap_yuvsrc[idx][tbl], p_frame);
					if (vos_flag_wait_timeout(&uiFlag, IAMOVIE_FLG_ID2, FLGIAMOVIE2_RAWENC_FRM_WAIT, TWF_ORW, vos_util_msec_to_tick(300)) != E_OK) {
						DBG_ERR("_ImageApp_MovieMulti_GetVideoFrameForImgCap(%d,%d) fail\r\n", idx, tbl);
						return 0;
					}
				} else {
					DBG_ERR("hd_videoproc_pull_out_buf fail(%d)\r\n", hd_ret);
					return 0;
				}
			}
		}
	} else {
		DBG_WRN("Thumb:[%d][%d] YUV path is not started (%d)\r\n", idx, tbl, img_venc_imgcap_yuvsrc[idx][tbl]);
		return 0;
	}
	return ImgLink[idx].vproc_p[img_vproc_splitter[idx]][img_venc_imgcap_yuvsrc[idx][tbl]];
}

THREAD_RETTYPE _ImageApp_MovieMulti_ImgCapTsk(void)
{
	FLGPTN vflag, uiFlag;
	IMGCAP_JOB_ELEMENT *pJob = NULL;
	HD_FILEOUT_BUF jpg_buf = {0};
	MOVIE_TBL_IDX tb;
	HD_VIDEO_FRAME yuv_frm = {0}, yuv_frm2 = {0};
	HD_VIDEOENC_BS  jpg_bs;
	HD_RESULT hd_ret;
	USIZE src_size = {0}, dest_size = {0};
	URECT dest_win = {0};
	VF_GFX_SCALE gfx_scale = {0};
	HD_COMMON_MEM_VB_BLK blk;
	UINT32 jpgenc_result;
	MEM_RANGE exif_data, exif_jpg, primary_jpg, dst_jpg_file;
	MOVIE_IMGCAP_INFO info;
	MOVIE_JPEG_INFO jpg_info;
	VOS_TICK t1, t2, t_diff;
	UINT32 yuv_type;
	MOVIE_CFG_CODEC venc_sout_codec;
	HD_PATH_ID venc_sout_path, bsmux_path, fileout_path[2];
	HD_DIM venc_sout_dim, venc_target_size;
	UINT32 venc_sout_pa, bsmux_path_status, fileout_path_status[2], j;
	HD_PATH_ID src_yuv_path, exif_yuv_path;

	THREAD_ENTRY();

	is_imgcap_tsk_running[0] = TRUE;
	while (imgcap_tsk_run[0]) {
		vos_flag_wait(&uiFlag, IAMOVIE_FLG_ID2, (FLGIAMOVIE2_VE_IMGCAP | FLGIAMOVIE2_VE_IMGCAP_ESC), TWF_ORW | TWF_CLR);
		if (uiFlag & FLGIAMOVIE2_VE_IMGCAP_ESC) {
			imgcap_tsk_run[0] = 0;
			break;
		}
		while ((pJob = (IMGCAP_JOB_ELEMENT *)_QUEUE_DeQueueFromHead(&ImgCapJobQueueReady)) != NULL) {
			info.path_id = pJob->path;
			// step: init variables
			jpgenc_result = FALSE;
			blk = HD_COMMON_MEM_VB_INVALID_BLK;
			src_yuv_path = 0;
			exif_yuv_path = 0;

			_ImageApp_MovieMulti_RecidGetTableIndex(pJob->path, &tb);

			if (pending_status) {
				ImageApp_MovieMulti_DUMP("Skip ImgCap job since ImgCap is pending\r\n");
				goto SKIP_THIS_JOB;
			}

			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				venc_sout_codec   = ImgLinkRecInfo[tb.idx][tb.tbl].codec;
				venc_sout_path    = ImgLink[tb.idx].venc_p[tb.tbl];
				venc_sout_dim.w   = ImgLinkRecInfo[tb.idx][tb.tbl].size.w;
				venc_sout_dim.h   = ImgLinkRecInfo[tb.idx][tb.tbl].size.h;
				if (img_venc_sout_type[tb.idx][tb.tbl] == MOVIE_IMGCAP_SOUT_SHARED_PRIVATE) {
					venc_sout_pa      = imgcap_venc_shared_sout_pa;
				} else {
					venc_sout_pa      = img_venc_sout_pa[tb.idx][tb.tbl];
				}
				bsmux_path        = ImgLink[tb.idx].bsmux_p[2*tb.tbl + pJob->is_emr];
				bsmux_path_status = ImgLinkStatus[tb.idx].bsmux_p[2*tb.tbl + pJob->is_emr];
				for (j = 0; j < 2; j++) {
					fileout_path[j]        = ImgLink[tb.idx].fileout_p[2*tb.tbl+j];
					fileout_path_status[j] = ImgLinkStatus[tb.idx].fileout_p[2*tb.tbl+j];
				}
				if (pJob->type == IMGCAP_THUM) {
					venc_target_size.w = img_venc_imgcap_thum_size[tb.idx][tb.tbl].w;
					venc_target_size.h = img_venc_imgcap_thum_size[tb.idx][tb.tbl].h;
				} else {
					venc_target_size.w = ImgLinkRecInfo[tb.idx][tb.tbl].raw_enc_size.w;
					venc_target_size.h = ImgLinkRecInfo[tb.idx][tb.tbl].raw_enc_size.h;
				}
				if (img_venc_imgcap_yuvsrc[tb.idx][tb.tbl] == MOVIE_IMGCAP_YUVSRC_SOUT) {
					yuv_type = YUV_COMPRESS_USE_SOUT;
				} else if (img_vproc_yuv_compress[tb.idx] && (tb.tbl == 0)) {
					if (img_venc_sout_type[tb.idx][tb.tbl] != MOVIE_IMGCAP_SOUT_NONE) {
						yuv_type = YUV_COMPRESS_USE_SOUT;
						if (pJob->type != IMGCAP_JSTM) {
							if (img_venc_imgcap_yuvsrc[tb.idx][tb.tbl] != MOVIE_IMGCAP_YUVSRC_MAIN) {
								yuv_type = YUV_COMPRESS_NO_SOUT;
							}
						}
					} else {
						yuv_type = YUV_COMPRESS_NO_SOUT;
					}
				} else {
					yuv_type = YUV_NORMAL;
				}
			} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
				venc_sout_codec   = EthCamLinkRecInfo[tb.idx].codec;
				venc_sout_path    = EthCamLink[tb.idx].venc_p[0];
				venc_sout_dim.w   = EthCamLinkRecInfo[tb.idx].width;
				venc_sout_dim.h   = EthCamLinkRecInfo[tb.idx].height;
				if (ethcam_venc_sout_type[tb.idx] == MOVIE_IMGCAP_SOUT_SHARED_PRIVATE) {
					venc_sout_pa      = imgcap_venc_shared_sout_pa;
				} else {
					venc_sout_pa      = ethcam_venc_sout_pa[tb.idx];
				}
				bsmux_path        = EthCamLink[tb.idx].bsmux_p[pJob->is_emr];
				bsmux_path_status = EthCamLinkStatus[tb.idx].bsmux_p[pJob->is_emr];;
				for (j = 0; j < 2; j++) {
					fileout_path[j]        = EthCamLink[tb.idx].fileout_p[j];
					fileout_path_status[j] = EthCamLinkStatus[tb.idx].fileout_p[j];
				}
				if (pJob->type == IMGCAP_THUM) {
					venc_target_size.w = ethcam_venc_imgcap_thum_size[tb.idx].w;
					venc_target_size.h = ethcam_venc_imgcap_thum_size[tb.idx].h;
				} else {
					venc_target_size.w = EthCamLinkRecInfo[tb.idx].width;
					venc_target_size.h = EthCamLinkRecInfo[tb.idx].height;
				}
				yuv_type = YUV_COMPRESS_USE_SOUT;
			} else {
				goto SKIP_THIS_JOB;
			}

//			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
#if 0
				if (img_vproc_yuv_compress[tb.idx] && (tb.tbl == 0)) {
					if (img_venc_sout_type[tb.idx][tb.tbl] != MOVIE_IMGCAP_SOUT_NONE) {
						yuv_type = YUV_COMPRESS_USE_SOUT;
						if (pJob->type != IMGCAP_JSTM) {
							if (img_venc_imgcap_yuvsrc[tb.idx][tb.tbl] != MOVIE_IMGCAP_YUVSRC_MAIN) {
								yuv_type = YUV_COMPRESS_NO_SOUT;
							}
						}
					} else {
						yuv_type = YUV_COMPRESS_NO_SOUT;
					}
				} else {
					yuv_type = YUV_NORMAL;
				}
#endif
				// step: get yuv buffer
				if (yuv_type == YUV_COMPRESS_USE_SOUT) {
					HD_H26XENC_TRIG_SNAPSHOT snap_param = {0};
					if ((hd_ret = hd_videoenc_set(venc_sout_path, HD_VIDEOENC_PARAM_OUT_TRIG_SNAPSHOT, &snap_param)) != HD_OK) {
						DBG_ERR("hd_videoenc_set(HD_VIDEOENC_PARAM_OUT_TRIG_SNAPSHOT) fail(%d)\r\n", hd_ret);
						goto SKIP_THIS_JOB;
					}
					UINT32 w_aligned, h_aligned;
					w_aligned = (venc_sout_codec == _CFG_CODEC_H264) ? ALIGN_CEIL_16(venc_sout_dim.w) : ALIGN_CEIL_64(venc_sout_dim.w);
					h_aligned = (venc_sout_codec == _CFG_CODEC_H264) ? ALIGN_CEIL_16(venc_sout_dim.h) : ALIGN_CEIL_64(venc_sout_dim.h);
					yuv_frm.sign        = MAKEFOURCC('V', 'F', 'R', 'M');
					yuv_frm.ddr_id      = dram_cfg.video_encode;
					yuv_frm.pxlfmt      = HD_VIDEO_PXLFMT_YUV420;
					yuv_frm.dim.w       = venc_sout_dim.w;
					yuv_frm.dim.h       = venc_sout_dim.h;
					yuv_frm.pw[0]       = venc_sout_dim.w;
					yuv_frm.ph[0]       = venc_sout_dim.h;
					yuv_frm.pw[1]       = yuv_frm.pw[0] / 2;
					yuv_frm.ph[1]       = yuv_frm.ph[0] / 2;
					yuv_frm.pw[2]       = yuv_frm.pw[1];
					yuv_frm.ph[2]       = yuv_frm.ph[1];
					yuv_frm.loff[0]     = w_aligned;
					yuv_frm.loff[1]     = w_aligned;
					yuv_frm.loff[2]     = w_aligned;
					yuv_frm.phy_addr[0] = venc_sout_pa;
					yuv_frm.phy_addr[1] = venc_sout_pa + w_aligned * h_aligned;
					yuv_frm.phy_addr[2] = yuv_frm.phy_addr[1];
					yuv_frm.blk         = -2;
				} else if (yuv_type == YUV_NORMAL) {
					if (img_manual_push_vdo_frame[tb.idx][tb.tbl] == FALSE) {
						if (img_vproc_no_ext[tb.idx] == FALSE) {
							if ((hd_ret = hd_videoproc_pull_out_buf(ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][tb.tbl], &yuv_frm, 500)) != HD_OK) {
								DBG_ERR("hd_videoproc_pull_out_buf fail(%d)\r\n", hd_ret);
								goto SKIP_THIS_JOB;
							}
							src_yuv_path = ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][tb.tbl];
						} else {
							UINT32 iport;
							if (gSwitchLink[tb.idx][tb.tbl] == UNUSED) {
								DBG_ERR("gSwitchLink[%d][%d]=%d\r\n", tb.idx, tb.tbl, gSwitchLink[tb.idx][tb.tbl]);
								goto SKIP_THIS_JOB;
							}
							iport = gUserProcMap[tb.idx][gSwitchLink[tb.idx][tb.tbl]];     // 0:main, 1:clone
							if (iport > 2) {
								iport = 2;
							}
							if ((hd_ret = hd_videoproc_pull_out_buf(ImgLink[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][iport], &yuv_frm, 500)) != HD_OK) {
								DBG_ERR("hd_videoproc_pull_out_buf fail(%d)\r\n", hd_ret);
								goto SKIP_THIS_JOB;
							}
							src_yuv_path = ImgLink[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][iport];
						}
					} else {
						_ImageApp_MovieMulti_GetVideoFrameForImgCap(tb.idx, tb.tbl, &yuv_frm);
						if (vos_flag_wait_timeout(&uiFlag, IAMOVIE_FLG_ID2, FLGIAMOVIE2_RAWENC_FRM_WAIT, TWF_ORW, vos_util_msec_to_tick(300)) != E_OK) {
							DBG_ERR("_ImageApp_MovieMulti_GetVideoFrameForImgCap(%d,%d) fail\r\n", tb.idx, tb.tbl);
							goto SKIP_THIS_JOB;
						}
						src_yuv_path = ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][tb.tbl];
					}
				} else {
					if (pJob->type == IMGCAP_JSTM) {
						if (img_manual_push_vdo_frame[tb.idx][tb.tbl] == FALSE) {
							if ((hd_ret = hd_videoproc_pull_out_buf(ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][tb.tbl], &yuv_frm, 500)) != HD_OK) {
								DBG_ERR("hd_videoproc_pull_out_buf fail(%d)\r\n", hd_ret);
								goto SKIP_THIS_JOB;
							}
							src_yuv_path = ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][tb.tbl];
						} else {
							_ImageApp_MovieMulti_GetVideoFrameForImgCap(tb.idx, tb.tbl, &yuv_frm);
							if (vos_flag_wait_timeout(&uiFlag, IAMOVIE_FLG_ID2, FLGIAMOVIE2_RAWENC_FRM_WAIT, TWF_ORW, vos_util_msec_to_tick(300)) != E_OK) {
								DBG_ERR("_ImageApp_MovieMulti_GetVideoFrameForImgCap(%d,%d) fail\r\n", tb.idx, tb.tbl);
								goto SKIP_THIS_JOB;
							}
							src_yuv_path = ImgLink[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][tb.tbl];
						}
					} else {
						if ((src_yuv_path = _GetVideoFrameForImgCap(tb.idx, tb.tbl, &yuv_frm)) == 0) {
							goto SKIP_THIS_JOB;
						}
					}
				}

				// step: callback if raw encode
				if (g_ia_moviemulti_usercb && ((pJob->type == IMGCAP_JSTM) || imgcap_thum_with_exif)) {
					vos_perf_mark(&t1);
					#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
					g_ia_moviemulti_usercb(pJob->path, MOVIE_USER_CB_EVENT_RAWENC_PREPARED, (UINT32)&yuv_frm);
					#else
					g_ia_moviemulti_usercb(pJob->path, MOVIE_USER_CB_EVENT_RAWENC_PREPARED, pJob->path);
					#endif
					vos_perf_mark(&t2);
					t_diff = (t2 - t1) / 1000;
					if (t_diff > IAMOVIE_USER_CB_TIMEOUT_CHECK) {
						DBG_WRN("MOVIE_USER_CB_EVENT_RAWENC_PREPARED run %dms > %dms!\r\n", t_diff, IAMOVIE_USER_CB_TIMEOUT_CHECK);
					}
				}

				// step create exif
				if (pJob->exif && (yuv_type == YUV_COMPRESS_NO_SOUT)) {
					//DBG_WRN("Cannot create exif since use compressed YUV without source out.\r\n");
					if ((exif_yuv_path = _GetVideoFrameForImgCap(tb.idx, tb.tbl, &yuv_frm2)) == FALSE) {
						DBG_WRN("Cannot create exif since use compressed YUV and get other yuv fail\r\n");
					}
				}
				if (pJob->exif && ((yuv_type != YUV_COMPRESS_NO_SOUT) || exif_yuv_path)) {
					// step: alloc exifthumbbuf
					_alloc_exifthumbbuf();

					// step: calculate exif size
					if (exif_yuv_path) {
						src_size.w = yuv_frm2.dim.w;
						src_size.h = yuv_frm2.dim.h;
					} else {
						src_size.w = yuv_frm.dim.w;
						src_size.h = yuv_frm.dim.h;
					}
					dest_size.w = IMGCAP_EXIF_W;
					dest_size.h = IMGCAP_EXIF_H;
					if (src_size.w && src_size.h && dest_size.w && dest_size.h) {
						UINT32 convert = (imgcap_thum_no_auto_scaling[tb.idx][tb.tbl] & 0x02) ? FALSE : TRUE;
						_Cal_Jpg_Size(&src_size, &dest_size , &dest_win, convert);
					}

					// step: scale image to exif thumb size
					if ((blk = _Scale_YUV(&gfx_scale, (exif_yuv_path ? &yuv_frm2 : &yuv_frm), &dest_size, &dest_win, TRUE)) == HD_COMMON_MEM_VB_INVALID_BLK) {
						goto RELEASE_YUV_BUF;
					}

					if (g_ia_moviemulti_usercb) {
						info.type = IMGCAP_EXIF_THUM,
						info.img_size.w = IMGCAP_EXIF_W;
						info.img_size.h = IMGCAP_EXIF_H;
						info.yuv_frame = &gfx_scale.dst_img;
						vos_perf_mark(&t1);
						g_ia_moviemulti_usercb(pJob->path, MOVIE_USER_CB_EVENT_JENC_PREPARED, (UINT32)&info);
						vos_perf_mark(&t2);
						t_diff = (t2 - t1) / 1000;
						if (t_diff > IAMOVIE_USER_CB_TIMEOUT_CHECK) {
							DBG_WRN("MOVIE_USER_CB_EVENT_JENC_PREPARED run %dms > %dms!\r\n", t_diff, IAMOVIE_USER_CB_TIMEOUT_CHECK);
						}
					}

					// step: config venc for exif
					imgcap_venc_in_param[0].dim.w = IMGCAP_EXIF_W;
					imgcap_venc_in_param[0].dim.h = IMGCAP_EXIF_H;
					imgcap_venc_out_param[0].jpeg.image_quality = 50;
					IACOMM_VENC_PARAM venc_param = {
						.video_enc_path = ImgCapLink[0].venc_p[0],
						.pin            = &(imgcap_venc_in_param[0]),
						.pout           = &(imgcap_venc_out_param[0]),
						.prc            = NULL,
					};
					if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
						DBG_ERR("venc_set_param fail(%d)\n", hd_ret);
						goto SKIP_THIS_JOB;
					}

					// step: start venc
					_LinkPortCntInc(&(ImgCapLinkStatus[0].venc_p[0]));
					_ImageApp_MovieMulti_ImgCapLinkStatusUpdate(0, UPDATE_REVERSE);
					_get_imgcap_buf_info(0);

					// step: push yuv to venc
					if ((hd_ret = hd_videoenc_push_in_buf(ImgCapLink[0].venc_p[0], &(gfx_scale.dst_img), NULL, 0)) != HD_OK) {
						DBG_ERR("hd_videoenc_push_in_buf fail(%d)\r\n", hd_ret);
						goto STOP_VDOENC;
					}

					// step: pull jpg from venc
					if ((hd_ret = hd_videoenc_pull_out_buf(ImgCapLink[0].venc_p[0], &jpg_bs, -1)) != HD_OK) {
						DBG_ERR("hd_videoenc_pull_out_buf fail(%d)\r\n", hd_ret);
						goto STOP_VDOENC;
					}

					// step: create exif
					exif_data.addr = exifthumb_va;
					exif_data.size = 65536;
					exif_jpg.addr = imgcap_venc_bs_buf_va[0] + (jpg_bs.video_pack[0].phy_addr - imgcap_venc_bs_buf[0].buf_info.phy_addr);    // va
					exif_jpg.size = jpg_bs.video_pack[0].size;
					if (EXIF_CreateExif(tb.idx, &exif_data, &exif_jpg) != EXIF_ER_OK) {
						exif_data.size = 0;
					} else {
						if (g_ia_moviemulti_usercb) {
							vos_perf_mark(&t1);
							g_ia_moviemulti_usercb(pJob->path, MOVIE_USER_CB_EVENT_EXIF_DONE, (UINT32)&exif_data);
							vos_perf_mark(&t2);
							t_diff = (t2 - t1) / 1000;
							if (t_diff > IAMOVIE_USER_CB_TIMEOUT_CHECK) {
								DBG_WRN("MOVIE_USER_CB_EVENT_EXIF_DONE run %dms > %dms!\r\n", t_diff, IAMOVIE_USER_CB_TIMEOUT_CHECK);
							}
						}
					}

					// step: release exif jpg
					if ((hd_ret = hd_videoenc_release_out_buf(ImgCapLink[0].venc_p[0], &jpg_bs)) != HD_OK) {
						DBG_ERR("hd_videoenc_release_out_buf fail(%d)\r\n", hd_ret);
						goto STOP_VDOENC;
					}

					// step: stop venc
					_LinkPortCntDec(&(ImgCapLinkStatus[0].venc_p[0]));
					_ImageApp_MovieMulti_ImgCapLinkStatusUpdate(0, UPDATE_FORWARD);

					blk = HD_COMMON_MEM_VB_INVALID_BLK;
				}

				// step: calculate jpg size
				src_size.w = yuv_frm.dim.w;
				src_size.h = yuv_frm.dim.h;
				if (pJob->type == IMGCAP_THUM) {
					info.type = IMGCAP_THUM;
					if (venc_target_size.w && venc_target_size.h) {
						dest_size.w = venc_target_size.w;
						dest_size.h = venc_target_size.h;
					} else {
						dest_size.w = IMGCAP_THUMB_W;
						dest_size.h = IMGCAP_THUMB_H;
					}
				} else {
					info.type = IMGCAP_JSTM;
					if (yuv_type == YUV_COMPRESS_NO_SOUT) {
						dest_size.w = src_size.w;
						dest_size.h = src_size.h;
					} else {
						dest_size.w = venc_target_size.w;
						dest_size.h = venc_target_size.h;
					}
				}
				if (src_size.w && src_size.h && dest_size.w && dest_size.h) {
					UINT32 convert = ((pJob->type == IMGCAP_THUM) && (imgcap_thum_no_auto_scaling[tb.idx][tb.tbl] & 0x01)) ? FALSE : TRUE;
					_Cal_Jpg_Size(&src_size, &dest_size , &dest_win, convert);
				}

				// step: scale image to jpg size
				if ((src_size.w != dest_size.w) || (src_size.h != dest_size.h)) {
					if ((blk = _Scale_YUV(&gfx_scale, &yuv_frm, &dest_size, &dest_win, FALSE)) == HD_COMMON_MEM_VB_INVALID_BLK) {
						goto RELEASE_YUV_BUF;
					}
				}

				if (g_ia_moviemulti_usercb) {
					info.img_size.w = dest_size.w;
					info.img_size.h = dest_size.h;
					info.yuv_frame  = &gfx_scale.dst_img;
					vos_perf_mark(&t1);
					g_ia_moviemulti_usercb(pJob->path, MOVIE_USER_CB_EVENT_JENC_PREPARED, (UINT32)&info);
					vos_perf_mark(&t2);
					t_diff = (t2 - t1) / 1000;
					if (t_diff > IAMOVIE_USER_CB_TIMEOUT_CHECK) {
						DBG_WRN("MOVIE_USER_CB_EVENT_JENC_PREPARED run %dms > %dms!\r\n", t_diff, IAMOVIE_USER_CB_TIMEOUT_CHECK);
					}
				}

				// step: config venc
				imgcap_venc_in_param[0].dim.w = dest_size.w;
				imgcap_venc_in_param[0].dim.h = dest_size.h;
				imgcap_venc_out_param[0].jpeg.image_quality = (pJob->type == IMGCAP_THUM) ? 50 : imgcap_jpg_quality.jpg_quality;
				IACOMM_VENC_PARAM venc_param = {
					.video_enc_path = ImgCapLink[0].venc_p[0],
					.pin            = &(imgcap_venc_in_param[0]),
					.pout           = &(imgcap_venc_out_param[0]),
					.prc            = NULL,
				};
				if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
					DBG_ERR("venc_set_param fail(%d)\n", hd_ret);
					goto RELEASE_YUV_BUF;
				}

				// step: start venc
				_LinkPortCntInc(&(ImgCapLinkStatus[0].venc_p[0]));
				_ImageApp_MovieMulti_ImgCapLinkStatusUpdate(0, UPDATE_REVERSE);
				_get_imgcap_buf_info(0);

				// step: push yuv to venc
				if ((hd_ret = hd_videoenc_push_in_buf(ImgCapLink[0].venc_p[0], ((blk == HD_COMMON_MEM_VB_INVALID_BLK) ? &yuv_frm : &(gfx_scale.dst_img)), NULL, 0)) != HD_OK) {
					DBG_ERR("hd_videoenc_push_in_buf fail(%d)\r\n", hd_ret);
					goto STOP_VDOENC;
				}

				// step: pull jpg from venc
				if ((hd_ret = hd_videoenc_pull_out_buf(ImgCapLink[0].venc_p[0], &jpg_bs, -1)) != HD_OK) {
					DBG_ERR("hd_videoenc_pull_out_buf fail(%d)\r\n", hd_ret);
					goto RELEASE_BLOCK;
				}

				// step: combine exif
				if (pJob->exif && ((yuv_type != YUV_COMPRESS_NO_SOUT) || exif_yuv_path)) {
					primary_jpg.addr = imgcap_venc_bs_buf_va[0] + (jpg_bs.video_pack[0].phy_addr - imgcap_venc_bs_buf[0].buf_info.phy_addr);    // va
					primary_jpg.size = jpg_bs.video_pack[0].size;
					GxImgFile_CombineJPG(&exif_data, &primary_jpg, NULL, &dst_jpg_file); // user should reserve the exif data size prior to primary data
				} else {
					dst_jpg_file.addr = imgcap_venc_bs_buf_va[0] + jpg_bs.video_pack[0].phy_addr - imgcap_venc_bs_buf[0].buf_info.phy_addr;     // va
					dst_jpg_file.size = jpg_bs.video_pack[0].size;
				}

				// step: free exifthumbbuf here to aviod memory corruption due to use private buffer for rawenc (imgcap_use_venc_buf == 0)
				if (exifthumb_va) {
					_free_exifthumbbuf();
				}

				// bush jpg to bsmux or fileout
				if (pJob->type == IMGCAP_JSTM) {
					if (imgcap_use_venc_buf) {
						if (g_ia_moviemulti_usercb) {
							jpg_info.sign    = MAKEFOURCC('J', 'S', 'T', 'M');
							jpg_info.addr_pa = imgcap_venc_bs_buf[0].buf_info.phy_addr + dst_jpg_file.addr - imgcap_venc_bs_buf_va[0];
							jpg_info.addr_va = dst_jpg_file.addr;
							jpg_info.size    = dst_jpg_file.size;
							vos_perf_mark(&t1);
							g_ia_moviemulti_usercb(pJob->path, MOVIE_USER_CB_EVENT_JENC_DONE, (UINT32)&jpg_info);
							vos_perf_mark(&t2);
							t_diff = (t2 - t1) / 1000;
							if (t_diff > IAMOVIE_USER_CB_TIMEOUT_CHECK) {
								DBG_WRN("MOVIE_USER_CB_EVENT_JENC_DONE run %dms > %dms!\r\n", t_diff, IAMOVIE_USER_CB_TIMEOUT_CHECK);
							}
						}
						for (j = 0; j < 2; j++) {
							if (_LinkCheckStatus(fileout_path_status[j])) {
								jpg_buf.sign = MAKEFOURCC('F', 'O', 'U', 'T');
								jpg_buf.type = MEDIA_FILEFORMAT_JPG;
								jpg_buf.fileop = HD_FILEOUT_FOP_SNAPSHOT;
								jpg_buf.event = HD_BSMUX_FEVENT_NORMAL;
								jpg_buf.addr = dst_jpg_file.addr;                                                             // note: fileout use va instead
								jpg_buf.size = dst_jpg_file.size;
								if ((hd_ret = hd_fileout_push_in_buf(fileout_path[j], &jpg_buf, -1)) != HD_OK) {
									DBG_ERR("hd_fileout_push_in_buf fail(%d)\r\n", hd_ret);
								} else {
									jpgenc_result = TRUE;
								}
								break;
							} else {
								jpgenc_result = TRUE;
							}
						}
					} else {
						HD_COMMON_MEM_DDR_ID ddr_id = dram_cfg.video_encode;
						UINT32 pa;
						void *va;

						if (imgcap_rawenc_va[pJob->path]) {
							if ((hd_ret = hd_common_mem_free(imgcap_rawenc_pa[pJob->path], (void *)imgcap_rawenc_va[pJob->path])) != HD_OK) {
								DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
							}
							imgcap_rawenc_va[pJob->path] = 0;
							imgcap_rawenc_pa[pJob->path] = 0;
							imgcap_rawenc_size[pJob->path] = 0;
						}

						imgcap_rawenc_size[pJob->path] = dst_jpg_file.size;
						if ((hd_ret = hd_common_mem_alloc("rawenc", &pa, (void **)&va, imgcap_rawenc_size[pJob->path], ddr_id)) != HD_OK) {
							DBG_ERR("hd_common_mem_alloc rawenc_buf failed, path=%d, size=%d, ret=%d\r\n", pJob->path, imgcap_rawenc_size[pJob->path], hd_ret);
							imgcap_rawenc_va[pJob->path] = 0;
							imgcap_rawenc_pa[pJob->path] = 0;
							imgcap_rawenc_size[pJob->path] = 0;
						} else {
							imgcap_rawenc_va[pJob->path] = (UINT32)va;
							imgcap_rawenc_pa[pJob->path] = (UINT32)pa;
							memcpy((void*)imgcap_rawenc_va[pJob->path], (void*)dst_jpg_file.addr, dst_jpg_file.size);

							if (g_ia_moviemulti_usercb) {
								jpg_info.sign    = MAKEFOURCC('J', 'S', 'T', 'M');
								jpg_info.addr_pa = imgcap_rawenc_pa[pJob->path];
								jpg_info.addr_va = imgcap_rawenc_va[pJob->path];
								jpg_info.size    = dst_jpg_file.size;
								vos_perf_mark(&t1);
								g_ia_moviemulti_usercb(pJob->path, MOVIE_USER_CB_EVENT_JENC_DONE, (UINT32)&jpg_info);
								vos_perf_mark(&t2);
								t_diff = (t2 - t1) / 1000;
								if (t_diff > IAMOVIE_USER_CB_TIMEOUT_CHECK) {
									DBG_WRN("MOVIE_USER_CB_EVENT_JENC_DONE run %dms > %dms!\r\n", t_diff, IAMOVIE_USER_CB_TIMEOUT_CHECK);
								}
							}
							for (j = 0; j < 2; j++) {
								if (_LinkCheckStatus(fileout_path_status[j])) {
									jpg_buf.sign = MAKEFOURCC('F', 'O', 'U', 'T');
									jpg_buf.type = MEDIA_FILEFORMAT_JPG;
									jpg_buf.fileop = HD_FILEOUT_FOP_SNAPSHOT;
									jpg_buf.event = HD_BSMUX_FEVENT_NORMAL;
									jpg_buf.addr = imgcap_rawenc_va[pJob->path];        // note: fileout use va instead
									jpg_buf.size = dst_jpg_file.size;
									if ((hd_ret = hd_fileout_push_in_buf(fileout_path[j], &jpg_buf, -1)) != HD_OK) {
										DBG_ERR("hd_fileout_push_in_buf fail(%d)\r\n", hd_ret);
									} else {
										jpgenc_result = TRUE;
									}
									break;
								} else {
									jpgenc_result = TRUE;
								}
							}
						}
					}
				} else if (pJob->type == IMGCAP_THUM) {
					if (g_ia_moviemulti_usercb) {
						jpg_info.sign    = MAKEFOURCC('T', 'H', 'U', 'M');
						jpg_info.addr_pa = imgcap_venc_bs_buf[0].buf_info.phy_addr + dst_jpg_file.addr - imgcap_venc_bs_buf_va[0];
						jpg_info.addr_va = dst_jpg_file.addr;
						jpg_info.size    = dst_jpg_file.size;
						vos_perf_mark(&t1);
						g_ia_moviemulti_usercb(pJob->path, MOVIE_USER_CB_EVENT_JENC_DONE, (UINT32)&jpg_info);
						vos_perf_mark(&t2);
						t_diff = (t2 - t1) / 1000;
						if (t_diff > IAMOVIE_USER_CB_TIMEOUT_CHECK) {
							DBG_WRN("MOVIE_USER_CB_EVENT_JENC_DONE run %dms > %dms!\r\n", t_diff, IAMOVIE_USER_CB_TIMEOUT_CHECK);
						}
					}
//					if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
						if (_LinkCheckStatus(bsmux_path_status)) {
							HD_BSMUX_PUT_DATA thumb_info = {
								.type     = HD_BSMUX_PUT_DATA_TYPE_THUMB,
								.phy_addr = imgcap_venc_bs_buf[0].buf_info.phy_addr + dst_jpg_file.addr - imgcap_venc_bs_buf_va[0],
								.vir_addr = dst_jpg_file.addr,
								.size     = dst_jpg_file.size,
							};
							if ((hd_ret = hd_bsmux_set(bsmux_path, HD_BSMUX_PARAM_PUT_DATA, &thumb_info)) != HD_OK) {
								DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_THUMB) fail(%d)\n", hd_ret);
							}
						}
//					}

					#if 0 // just for test, remove later
					FILE *fp;
					fp = fopen("/mnt/sd/thumb.jpg", "wb");
					DBG_DUMP("addr=%x,size=%x\r\n", dst_jpg_file.addr, dst_jpg_file.size);
					fwrite((UINT8*)dst_jpg_file.addr, 1, dst_jpg_file.size, fp);
					fflush(fp);
					fclose(fp);
					#endif
				}

				// step: release jpg
				if ((hd_ret = hd_videoenc_release_out_buf(ImgCapLink[0].venc_p[0], &jpg_bs)) != HD_OK) {
					DBG_ERR("hd_videoenc_release_out_buf fail(%d)\r\n", hd_ret);
				}
RELEASE_BLOCK:
				// step: relase buffer
				if (blk != HD_COMMON_MEM_VB_INVALID_BLK) {
					if ((hd_ret = hd_common_mem_release_block(blk)) != HD_OK) {
						DBG_ERR("hd_common_mem_release_block(%d) failed(%d)\r\n", blk, hd_ret);
					}
				}
STOP_VDOENC:
				// step: stop venc
				_LinkPortCntDec(&(ImgCapLinkStatus[0].venc_p[0]));
				_ImageApp_MovieMulti_ImgCapLinkStatusUpdate(0, UPDATE_FORWARD);
RELEASE_YUV_BUF:
				// step: release yuv
				if (yuv_type == YUV_COMPRESS_USE_SOUT) {
					// No need to release buffer if use source out
				} else {
					if ((hd_ret = hd_videoproc_release_out_buf(src_yuv_path, &yuv_frm)) != HD_OK) {
						DBG_ERR("hd_videoproc_release_out_buf fail(%d)\r\n", hd_ret);
					}
				}
				if (exif_yuv_path) {
					if ((hd_ret = hd_videoproc_release_out_buf(exif_yuv_path, &yuv_frm2)) != HD_OK) {
						DBG_ERR("hd_videoproc_release_out_buf fail(%d)\r\n", hd_ret);
					}
				}
//			}
SKIP_THIS_JOB:
			// step: free exifthumbbuf if this job is skiped
			if (exifthumb_va) {
				_free_exifthumbbuf();
			}

			// step: callback
			if (g_ia_moviemulti_usercb && pJob->type == IMGCAP_JSTM) {
				vos_perf_mark(&t1);
				if (jpgenc_result == TRUE) {
					g_ia_moviemulti_usercb(pJob->path, MOVIE_USER_CB_EVENT_SNAPSHOT_OK, pJob->path);
				} else {
					g_ia_moviemulti_usercb(pJob->path, MOVIE_USER_CB_ERROR_SNAPSHOT_ERR, pJob->path);
				}
				vos_perf_mark(&t2);
				t_diff = (t2 - t1) / 1000;
				if (t_diff > IAMOVIE_USER_CB_TIMEOUT_CHECK) {
					DBG_WRN("MOVIE_USER_CB_EVENT_SNAPSHOT_OK/ERR run %dms > %dms!\r\n", t_diff, IAMOVIE_USER_CB_TIMEOUT_CHECK);
				}
				if (tb.link_type == LINKTYPE_REC) {
					vflag = FLGIAMOVIE2_JPG_M0_RDY << (2 * tb.idx + tb.tbl);
				} else {
					vflag = FLGIAMOVIE2_JPG_E0_RDY << tb.idx;
				}
			} else {
				if (tb.link_type == LINKTYPE_REC) {
					vflag = FLGIAMOVIE2_THM_M0_RDY << (2 * tb.idx + tb.tbl);
				} else {
					vflag = FLGIAMOVIE2_THM_E0_RDY << tb.idx;
				}
			}
			vos_flag_set(IAMOVIE_FLG_ID2, vflag);
			_QUEUE_EnQueueToTail(&ImgCapJobQueueClear, (QUEUE_BUFFER_ELEMENT *)pJob);
		}
	}
	is_imgcap_tsk_running[0] = FALSE;

	THREAD_RETURN(0);
}

ER _ImageApp_MovieMulti_ImgCapLinkCfg(MOVIE_CFG_REC_ID id)
{
	UINT32 idx = 0;

	// video encode
	ImgCapLink[idx].venc_i[0] = HD_VIDEOENC_IN (0, MaxLinkInfo.MaxImgLink * 2 + MaxLinkInfo.MaxWifiLink * 1 + idx);
	ImgCapLink[idx].venc_o[0] = HD_VIDEOENC_OUT(0, MaxLinkInfo.MaxImgLink * 2 + MaxLinkInfo.MaxWifiLink * 1 + idx);

	return E_OK;
}

static ER _ImageApp_MovieMulti_ImgCapLink(MOVIE_CFG_REC_ID id)
{
	UINT32 idx = 0;
	ER ret = E_OK;
	HD_RESULT hd_ret;

	if ((hd_ret = venc_open_ch(ImgCapLink[idx].venc_i[0], ImgCapLink[idx].venc_o[0], &(ImgCapLink[idx].venc_p[0]))) != HD_OK) {
		DBG_ERR("venc_open_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	HD_VIDEOENC_FUNC_CONFIG imgcap_venc_func_cfg = {
		.ddr_id  = dram_cfg.video_encode,
		.in_func = 0,
	};
	IACOMM_VENC_CFG venc_cfg = {
		.video_enc_path = ImgCapLink[idx].venc_p[0],
		.ppath_cfg      = &(imgcap_venc_path_cfg[idx]),
		.pfunc_cfg      = &(imgcap_venc_func_cfg),
	};
	if ((hd_ret = venc_set_cfg(&venc_cfg)) != HD_OK) {
		DBG_ERR("venc_set_cfg fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	IACOMM_VENC_PARAM venc_param = {
		.video_enc_path = ImgCapLink[idx].venc_p[0],
		.pin            = &(imgcap_venc_in_param[idx]),
		.pout           = &(imgcap_venc_out_param[idx]),
		.prc            = NULL,
	};
	if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
		DBG_ERR("venc_set_param fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	VENDOR_VIDEOENC_BS_RESERVED_SIZE_CFG bs_reserved_size = {0};
	bs_reserved_size.reserved_size = ImgCapExifFuncEn ? VENC_BS_RESERVED_SIZE_JPG : VENC_BS_RESERVED_SIZE_TS;
	if ((hd_ret = vendor_videoenc_set(ImgCapLink[idx].venc_p[0], VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE, &bs_reserved_size)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE) fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	return ret;
}

static ER _ImageApp_MovieMulti_ImgCapUnlink(MOVIE_CFG_REC_ID id)
{
	UINT32 idx = 0;
	ER ret = E_OK;
	HD_RESULT hd_ret;

	if (imgcap_venc_bs_buf_va[idx]) {
		hd_common_mem_munmap((void *)imgcap_venc_bs_buf_va[idx], imgcap_venc_bs_buf[idx].buf_info.buf_size);
		imgcap_venc_bs_buf_va[idx] = 0;
	}

	if ((hd_ret = venc_close_ch(ImgCapLink[idx].venc_p[0])) != HD_OK) {
		DBG_ERR("venc_close_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	return ret;
}

ER _ImageApp_MovieMulti_ImgCapLinkStatusUpdate(MOVIE_CFG_REC_ID id, UINT32 direction)
{
	UINT32 idx = 0;
	ER ret = E_OK;

	// set videoenc
	ret |= _LinkUpdateStatus(venc_set_state, ImgCapLink[idx].venc_p[0], &(ImgCapLinkStatus[idx].venc_p[0]), NULL);

	return ret;
}

ER _ImageApp_MovieMulti_ImgCapLinkOpen(MOVIE_CFG_REC_ID id)
{
	UINT32 i = 0, idx = 0, j;

	if (IsImgCapLinkOpened[idx] == TRUE) {
		DBG_ERR("id%d is already opened.\r\n", i);
		return E_SYS;
	}

	_ImageApp_MovieMulti_ImgCapLinkCfg(i);
	_ImageApp_MovieMulti_SetupImgCapParam(i);
	_ImageApp_MovieMulti_ImgCapLink(i);

	memset(&ImgCapJobQueueClear, 0, sizeof(ImgCapJobQueueClear));
	memset(&ImgCapJobQueueReady, 0, sizeof(ImgCapJobQueueReady));
	for (j = 0; j < IMGCAP_JOBQUEUENUM; j++) {
		_QUEUE_EnQueueToTail(&ImgCapJobQueueClear, (QUEUE_BUFFER_ELEMENT *)&ImgCapJobElement[j]);
	}

	imgcap_tsk_run[idx] = TRUE;
	vos_flag_clr(IAMOVIE_FLG_ID2, FLGIAMOVIE2_VE_IMGCAP);
	if ((iamovie_imgcap_tsk_id = vos_task_create(_ImageApp_MovieMulti_ImgCapTsk, 0, "IAMovieImgCapTsk", PRI_IAMOVIE_IMGCAP, STKSIZE_IAMOVIE_IMGCAP)) == 0) {
		DBG_ERR("IAMovieImgCapTsk create failed.\r\n");
	} else {
		vos_task_resume(iamovie_imgcap_tsk_id);
	}

	IsImgCapLinkOpened[idx] = TRUE;

	return E_OK;
}

ER _ImageApp_MovieMulti_ImgCapLinkClose(MOVIE_CFG_REC_ID id)
{
	UINT32 i = 0, idx = 0, j, delay_cnt; //, imgcaptsk_status = FALSE;
	HD_RESULT hd_ret;

	if (IsImgCapLinkOpened[idx] == FALSE) {
		DBG_ERR("id%d is already closed.\r\n", i);
		return E_SYS;
	}

	vos_flag_set(IAMOVIE_FLG_ID2, FLGIAMOVIE2_VE_IMGCAP_ESC);
	imgcap_tsk_run[idx] = FALSE;

	while (_QUEUE_DeQueueFromHead(&ImgCapJobQueueReady) != NULL) {
		// clear job queue
	}

	#if 0
	if (ImgCapLinkStatus[0].venc_p[0]) {
		imgcaptsk_status = TRUE;
		delay_cnt = 50;
	} else {
		vos_util_delay_ms(10);
	}
	if (imgcaptsk_status) {
		while (is_imgcap_tsk_running[0] && delay_cnt) {
			vos_util_delay_ms(10);
			delay_cnt --;
		}
	}
	#else
	delay_cnt = 50;
	while (is_imgcap_tsk_running[0] && delay_cnt) {
		vos_util_delay_ms(10);
		delay_cnt --;
	}
	#endif

	if (is_imgcap_tsk_running[idx]) {
		ImageApp_MovieMulti_DUMP("Destroy IAMovieImgCapTsk\r\n");
		vos_task_destroy(iamovie_imgcap_tsk_id);
	}

	for (j = 0; j < MAX_IMG_PATH; j++) {
		if (imgcap_rawenc_va[j]) {
			if ((hd_ret = hd_common_mem_free(imgcap_rawenc_pa[j], (void *)imgcap_rawenc_va[j])) != HD_OK) {
				DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
			}
			imgcap_rawenc_va[j] = 0;
			imgcap_rawenc_pa[j] = 0;
			imgcap_rawenc_size[j] = 0;
		}
	}

	imgcap_thum_with_exif = FALSE;

	_ImageApp_MovieMulti_ImgCapUnlink(i);
	IsImgCapLinkOpened[idx] = FALSE;

	return E_OK;
}

ER ImageApp_MovieMulti_ImgCapLink_Pending(BOOL status)
{
	ER ret;
	if (status) {
		pending_status = 1;
		ImageApp_MovieMulti_DUMP("ImageApp_MovieMulti_ImgCapLink_Pending(TRUE)\r\n");
		ret = _ImageApp_MovieMulti_ImgCapUnlink(0);
	} else {
		pending_status = 0;
		ImageApp_MovieMulti_DUMP("ImageApp_MovieMulti_ImgCapLink_Pending(FALSE)\r\n");
		ret = _ImageApp_MovieMulti_ImgCapLink(0);
	}
	return ret;
}

