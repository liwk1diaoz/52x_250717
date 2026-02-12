#include <stdlib.h>
#include <string.h>
#include "ImageApp/ImageApp_Photo.h"
#include "ImageApp_Photo_int.h"
#include "ImageApp_Photo_FileNaming.h"
#include "Utility/SwTimer.h"
#include <kwrap/task.h>
#include <kwrap/util.h>
#include <kwrap/flag.h>
#include <kwrap/perf.h>
#include "FileSysTsk.h"
#include "vendor_videoenc.h"
#include "SizeConvert.h"
#include "vf_gfx.h"
#include "Utility/Color.h"
#include "exif/Exif.h"
#include "GxImageFile.h"
#include "vendor_isp.h"
#include "kwrap/perf.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          IA_Photo_CapLink
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////// //////////////////////////////////////////////////////////
#define LOAD_ISP_DTSI   0

#define FLGPHOTO_CAP_START            FLGPTN_BIT(0)       //0x00000001
#define FLGPHOTO_CAP_IDLE      		FLGPTN_BIT(1)       //0x00000002


#define FLGPHOTO_WRITEFILE_START        	FLGPTN_BIT(3)
#define FLGPHOTO_WRITEFILE_IDLE      	FLGPTN_BIT(4)
#define FLGPHOTO_WRITEFILE_COMPLETE 	FLGPTN_BIT(5)

#define PRI_IMAGEAPP_PHOTO_CAP             10
#define STKSIZE_IMAGEAPP_PHOTO_CAP         (4*2048)
#define PRI_IMAGEAPP_PHOTO_WRITEFILE             10
#define STKSIZE_IMAGEAPP_PHOTO_WRITEFILE         (2*2048)
#define PHOTOCAP_THUMB_W 160
#define PHOTOCAP_THUMB_H  120
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	ALIGN_CEIL_4(((w) * (h) * HD_VIDEO_PXLFMT_BPP(pxlfmt)) / 8)
#define SAVE_FILE_DBG  0
#define JPG_BUFF_MAX  5
#define FILE_BUF_MAX_NUM    10


static PHOTO_PROCESS_CB  g_PhotoCapImgCb = NULL;
static PHOTO_PROCESS_CB  g_PhotoWriteFileTskCb = NULL;
PHOTO_CAP_FSYS_FP  g_PhotoCapWriteFileCb = NULL;
PHOTO_PROCESS_PARAM_CB  g_PhotoCapQvCb = NULL;
PHOTO_PROCESS_PARAM_CB  g_PhotoCapScrCb = NULL;
PHOTO_CAP_CBMSG_FP  g_PhotoCapMsgCb = NULL;
PHOTO_CAP_S2DET_FP  g_PhotoCapDetS2Cb = NULL;
static UINT32 g_photo_is_capcb_executing= 0;

static UINT32 g_photo_cap_tsk_run[PHOTO_CAP_ID_MAX]= {0} , g_photo_is_cap_tsk_running[PHOTO_CAP_ID_MAX]= {0};
static UINT32 g_photo_writefile_tsk_run[PHOTO_CAP_ID_MAX]= {0} , g_photo_is_writefile_tsk_running[PHOTO_CAP_ID_MAX]= {0};
static THREAD_HANDLE g_photo_cap_tsk_id;
static THREAD_HANDLE g_photo_writefile_tsk_id;
static ID g_photo_cap_flg_id = 0;
static ID g_photo_writefile_flg_id = 0;

UINT32 g_photo_IsCapLinkOpened[PHOTO_CAP_ID_MAX] = {0};
UINT32 g_photo_IsCapStartLinkOpened[PHOTO_CAP_ID_MAX] = {0};
UINT32 g_exifthumb_scr_pa = 0, g_exifthumb_scr_va = 0, g_exifthumb_scr_size = 0;
static UINT32 g_scrjpg_pa = 0, g_scrjpg_va = 0, g_scrjpg_size = 0;
static UINT32 g_thumbjpg_pa = 0, g_thumbjpg_va = 0, g_thumbjpg_size = 0;
static UINT32 g_thumbyuv_pa = 0, g_thumbyuv_va = 0, g_thumbyuv_size = 0;

static UINT32 g_jpg_buff_pa[PHOTO_CAP_ID_MAX][JPG_BUFF_MAX] = {0}, g_jpg_buff_va[PHOTO_CAP_ID_MAX][JPG_BUFF_MAX] = {0}, g_jpg_buff_size = 0, g_pre_jpg_buff_size;
static IMGCAP_FILE_INFOR g_PhotoCapFileInfor[FILE_BUF_MAX_NUM];
UINT32 NormalJpgQueueF, NormalJpgQueueR;
HD_VIDEOCAP_IN g_prv_vcap_in_param[PHOTO_VID_IN_MAX] = {0};
HD_VIDEOCAP_CROP g_prv_vcap_cropout_param[PHOTO_VID_IN_MAX] = {0};

void _ImageApp_Photo_OpenCapLink(PHOTO_CAP_INFO *p_cap_info);
static void _ImageApp_Photo_Cal_Jpg_Size(USIZE *psrc, USIZE *pdest , URECT *pdestwin)
{
	SIZECONVERT_INFO scale_info = {0};

	scale_info.uiSrcWidth  = psrc->w;
	scale_info.uiSrcHeight = psrc->h;
	scale_info.uiDstWidth  = pdest->w;
	scale_info.uiDstHeight = pdest->h;
	scale_info.uiDstWRatio = pdest->w;
	scale_info.uiDstHRatio = pdest->h;
	scale_info.alignType   = SIZECONVERT_ALIGN_FLOOR_32;
	DisplaySizeConvert(&scale_info);
	pdestwin->x = scale_info.uiOutX;
	pdestwin->y = scale_info.uiOutY;
	pdestwin->w = scale_info.uiOutWidth;
	pdestwin->h = scale_info.uiOutHeight;
	//DBG_DUMP("Dst: x %d, y %d, w %d, h %d\r\n", pdestwin->x, pdestwin->y, pdestwin->w, pdestwin->h);
}

static HD_COMMON_MEM_VB_BLK _ImageApp_Photo_Scale_YUV(VF_GFX_SCALE *pscale, HD_VIDEO_FRAME *psrc, USIZE *pdest_sz, URECT *pdestwin, HD_VIDEO_PXLFMT pxl_fmt)
{
	HD_COMMON_MEM_VB_BLK blk;
	UINT32 blk_size, pa;
	UINT32 addr[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 loff[HD_VIDEO_MAX_PLANE] = {0};
	VF_GFX_DRAW_RECT fill_rect = {0};
	HD_RESULT hd_ret;

	blk_size = VDO_YUV_BUFSIZE(pdest_sz->w, pdest_sz->h, pxl_fmt);

	//if (blk_size > g_exifthumb_scr_size) {
	if (blk_size > g_thumbyuv_size) {
		DBG_ERR("Request blk_size(%d) > g_thumbyuv_size(%d)\r\n", blk_size, g_thumbyuv_size);
		return HD_COMMON_MEM_VB_INVALID_BLK;
	}
	pa = g_thumbyuv_pa;//g_exifthumb_scr_pa;
	blk = -2; //must, for use private buffer, push in

	// set src
	addr[0] = psrc->phy_addr[HD_VIDEO_PINDEX_Y];
	loff[0] = psrc->loff[HD_VIDEO_PINDEX_Y];
	if(pxl_fmt==HD_VIDEO_PXLFMT_YUV420 ){
		addr[1] = psrc->phy_addr[HD_VIDEO_PINDEX_UV];
		loff[1] = psrc->loff[HD_VIDEO_PINDEX_UV];
	}else{
		addr[1] = psrc->phy_addr[HD_VIDEO_PINDEX_U];
		loff[1] = psrc->loff[HD_VIDEO_PINDEX_U];
		addr[2] = psrc->phy_addr[HD_VIDEO_PINDEX_V];
		loff[2] = psrc->loff[HD_VIDEO_PINDEX_V];
	}
	if ((hd_ret = vf_init_ex(&(pscale->src_img), psrc->dim.w, psrc->dim.h, psrc->pxlfmt, loff, addr)) != HD_OK) {
		DBG_ERR("vf_init_ex src failed(%d)\r\n", hd_ret);
	}
	// set dest
	addr[0] = pa;
	loff[0] = ALIGN_CEIL_4(pdest_sz->w);
	if(pxl_fmt==HD_VIDEO_PXLFMT_YUV420 ){
		addr[1] = pa + loff[0] * pdest_sz->h;
		loff[1] = ALIGN_CEIL_4(pdest_sz->w);
	}else{
		addr[1] = pa + loff[0] * pdest_sz->h;
		loff[1] = ALIGN_CEIL_4(pdest_sz->w);
		addr[2] = pa + loff[0] * pdest_sz->h + (loff[1] * pdest_sz->h)/2;
		loff[2] = ALIGN_CEIL_4(pdest_sz->w);
	}
	if ((hd_ret = vf_init_ex(&(pscale->dst_img), pdest_sz->w, pdest_sz->h, pxl_fmt, loff, addr)) != HD_OK) {
		DBG_ERR("vf_init_ex dst failed(%d)\r\n", hd_ret);
	}


	if ((pdest_sz->w != pdestwin->w ) ||(pdest_sz->h != pdestwin->h)) {
		// clear buffer by black
		//gximg_fill_data((VDO_FRAME *)&(pscale->dst_img), GXIMG_REGION_MATCH_IMG, COLOR_YUV_BLACK);
		memcpy((void *)&(fill_rect.dst_img), (void *)&(pscale->dst_img), sizeof(HD_VIDEO_FRAME));
		fill_rect.color = COLOR_RGB_BLACK;
		fill_rect.rect.x = 0;
		fill_rect.rect.y = 0;
		fill_rect.rect.w = pdest_sz->w;
		fill_rect.rect.h = pdest_sz->h;
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
	pscale->src_region.w = psrc->dim.w;
	pscale->src_region.h = psrc->dim.h;
	pscale->dst_region.x = pdestwin->x;
	pscale->dst_region.y = pdestwin->y;
	pscale->dst_region.w = pdestwin->w;
	pscale->dst_region.h = pdestwin->h;
	pscale->quality = HD_GFX_SCALE_QUALITY_INTEGRATION;//HD_GFX_SCALE_QUALITY_BILINEAR;
	vf_gfx_scale(pscale, 1);

	pscale->dst_img.count 	= 0;
	pscale->dst_img.timestamp = hd_gettime_us();
	pscale->dst_img.blk		= blk;

	return blk;
}
#define IMG_RATIO_UINT  512
#define IPL_ALIGN_ROUNDUP(x, y) ((((x) + (y) - 1) / (y)) * (y))

UINT32 _ImageApp_Photo_SENCROPRATIO_ADJ_VSIZE(UINT32 ImgRatio, UINT32 DefRatio, UINT32 InSize, UINT32 Align)
{
	UINT32 Img_H, Img_V;
	UINT32 Def_H, Def_V;
	UINT32 rtValue;
	UINT32 VRatio;

	Def_H = (DefRatio & 0xffff0000) >> 16;
	Def_V = DefRatio & 0x0000ffff;

	Img_H = (ImgRatio & 0xffff0000) >> 16;
	Img_V = ImgRatio & 0x0000ffff;

	VRatio = (IMG_RATIO_UINT * Def_H * Img_V) / (Def_V * Img_H);
	rtValue = InSize * VRatio / IMG_RATIO_UINT;

	if (rtValue > InSize) {
		rtValue = InSize;
	}
	rtValue = IPL_ALIGN_ROUNDUP(rtValue, Align);
	return rtValue;
}

UINT32 _ImageApp_Photo_SENCROPRATIO_ADJ_HSIZE(UINT32 ImgRatio, UINT32 DefRatio, UINT32 InSize, UINT32 Align)
{
	UINT32 Img_H, Img_V;
	UINT32 Def_H, Def_V;
	UINT32 rtValue;
	UINT32 HRatio;

	Def_H = (DefRatio & 0xffff0000) >> 16;
	Def_V = DefRatio & 0x0000ffff;

	Img_H = (ImgRatio & 0xffff0000) >> 16;
	Img_V = ImgRatio & 0x0000ffff;

	HRatio = (IMG_RATIO_UINT * Def_V * Img_H) / (Def_H * Img_V);

	rtValue = InSize * HRatio / IMG_RATIO_UINT;

	if (rtValue > InSize) {
		rtValue = InSize;
	}
	rtValue = IPL_ALIGN_ROUNDUP(rtValue, Align);
	return rtValue;
}
void NORMAL_SET_FILE_INFOR(UINT32 Idx, IMGCAP_FILE_INFOR Infor)
{
	g_PhotoCapFileInfor[Idx] = Infor;
}

IMGCAP_FILE_INFOR NORMAL_GET_FILE_INFOR(UINT32 Idx)
{
	return g_PhotoCapFileInfor[Idx];
}


static void  _ImageApp_Photo_CapScrCB(UINT32 idx)
{
	HD_VIDEO_FRAME video_frame = {0};
	//UINT32 idx=0;
	HD_RESULT ret=0;
	UINT32 vir_addr_main= {0};
	HD_VIDEOENC_BUFINFO phy_buf_main= {0};
	HD_VIDEOENC_BS  data_pull= {0};
	#if (SAVE_FILE_DBG ==1)
	char file_path_main[64] = {0};
	//FILE *f_out_main;
	FST_FILE hFile = NULL;
	#endif
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))
	IMG_CAP_DATASTAMP_INFO Stamp_CBInfo = {0};

	//Get screennail
	if(photo_cap_info[idx].screen_img==SEL_SCREEN_IMG_ON){
		memset(&video_frame, 0, sizeof(HD_VIDEO_FRAME));
		while(1){
			//DBG_DUMP("screen proc_pull ....\r\n");
			ret = hd_videoproc_pull_out_buf(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_SCR_PATH], &video_frame, -1); // -1 = blocking mode
			if (ret != HD_OK) {
				if (ret != HD_ERR_UNDERRUN){
					DBG_ERR("screen proc_pull error=%d !!\r\n\r\n", ret);
					return;
				}else{
					//DBG_ERR("screen proc_pull continue\r\n");
					continue;
				}
			}else{
				break;
			}
		}
		//DBG_DUMP("screen proc frame.count = %llu\r\n", video_frame.count);

		if (photo_cap_info[idx].datastamp == SEL_DATASTAMP_ON && g_PhotoCapMsgCb) {
			IMG_CAP_YCC_IMG_INFO ScrImgInfo = {0};
			memset(&Stamp_CBInfo,0, sizeof(IMG_CAP_DATASTAMP_INFO));

			ScrImgInfo.ch[IMG_CAP_YUV_Y].width=video_frame.pw[IMG_CAP_YUV_Y];//photo_cap_info[idx].screen_img_size.w;
			ScrImgInfo.ch[IMG_CAP_YUV_Y].height=video_frame.ph[IMG_CAP_YUV_Y];//photo_cap_info[idx].screen_img_size.h;
			ScrImgInfo.ch[IMG_CAP_YUV_Y].line_ofs=video_frame.loff[IMG_CAP_YUV_Y];//photo_cap_info[idx].screen_img_size.w;
			ScrImgInfo.ch[IMG_CAP_YUV_U].width=video_frame.pw[IMG_CAP_YUV_U];//photo_cap_info[idx].screen_img_size.w>>1;
			ScrImgInfo.ch[IMG_CAP_YUV_U].height=video_frame.ph[IMG_CAP_YUV_U];//photo_cap_info[idx].screen_img_size.h>>1;
			ScrImgInfo.ch[IMG_CAP_YUV_U].line_ofs=video_frame.loff[IMG_CAP_YUV_U];//photo_cap_info[idx].screen_img_size.w>>1;
			ScrImgInfo.ch[IMG_CAP_YUV_V].width=video_frame.pw[IMG_CAP_YUV_V];//photo_cap_info[idx].screen_img_size.w>>1;
			ScrImgInfo.ch[IMG_CAP_YUV_V].height=video_frame.ph[IMG_CAP_YUV_V];//photo_cap_info[idx].screen_img_size.h>>1;
			ScrImgInfo.ch[IMG_CAP_YUV_V].line_ofs=video_frame.loff[IMG_CAP_YUV_V];//photo_cap_info[idx].screen_img_size.w>>1;
			ScrImgInfo.pixel_addr[0]=video_frame.phy_addr[0];
			ScrImgInfo.pixel_addr[1]=video_frame.phy_addr[1];
			ScrImgInfo.pixel_addr[2]=video_frame.phy_addr[2];

			//DBG_DUMP("screen video_frame.pw[0]=%d, %d, %d\r\n", video_frame.pw[0],video_frame.ph[0],video_frame.loff[0]);
			//DBG_DUMP("screen video_frame.pw[1]=%d, %d, %d\r\n", video_frame.pw[1],video_frame.ph[1],video_frame.loff[1]);
			//DBG_DUMP("screen video_frame.pw[2]=%d, %d, %d\r\n", video_frame.pw[2],video_frame.ph[2],video_frame.loff[2]);
			//DBG_DUMP("video_frame.phy_addr[0]=0x%x, 0x%x, 0x%x\r\n", video_frame.phy_addr[0],video_frame.phy_addr[1],video_frame.phy_addr[2]);

			//g_PhotoCapMsgCb(CAP_DS_EVENT_PRI,&PriImgInfo);
			Stamp_CBInfo.event=CAP_DS_EVENT_SCR;
			Stamp_CBInfo.ImgInfo= ScrImgInfo;
			g_PhotoCapMsgCb(IMG_CAP_CBMSG_GEN_DATASTAMP_PIC,&Stamp_CBInfo);
		}

		photo_cap_venc_in_param.dim.w = photo_cap_info[idx].screen_img_size.w;
		photo_cap_venc_in_param.dim.h = photo_cap_info[idx].screen_img_size.h;
		photo_cap_venc_in_param.pxl_fmt= photo_cap_info[idx].screen_fmt;
		//photo_cap_venc_out_param.jpeg.image_quality = 60;
#if 0//PHOTO_BRC_MODE
		photo_cap_venc_out_param.jpeg.image_quality = 0;
		photo_cap_venc_out_param.jpeg.bitrate       = bitrate;
#else
		photo_cap_venc_out_param.jpeg.image_quality = 60;
		photo_cap_venc_out_param.jpeg.bitrate       = 0;
#endif

		if ((ret =hd_videoenc_stop(photo_cap_info[0].venc_p)) != HD_OK) {
			DBG_ERR("SCR hd_videoenc_stop fail(%d), venc_p=0x%x\r\n", ret, photo_cap_info[0].venc_p);
			return;
		}
		_ImageApp_Photo_venc_set_param(photo_cap_info[0].venc_p, &photo_cap_venc_in_param, &photo_cap_venc_out_param);

		if ((ret = hd_videoenc_start(photo_cap_info[0].venc_p)) != HD_OK) {
			DBG_ERR("SCR hd_videoenc_start fail(%d), venc_p=0x%x\r\n", ret, photo_cap_info[0].venc_p);
			return;
		}

		//DBG_DUMP("enc_push ....\r\n");
		//DBG_DUMP("encode %d/%d\r\n", enc_count+1, flow_max_shot);
		ret = hd_videoenc_push_in_buf(photo_cap_info[0].venc_p, &video_frame, NULL, -1); // blocking mode
		if (ret != HD_OK) {
			DBG_ERR("enc_push error=%d !!\r\n\r\n", ret);
			return;
		}

		//DBG_DUMP("proc_release ....\r\n");
		ret = hd_videoproc_release_out_buf(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_SCR_PATH], &video_frame);
		if (ret != HD_OK) {
			DBG_ERR("proc_release error=%d !!\r\n\r\n", ret);
			return;
		}

		// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
		hd_videoenc_get(photo_cap_info[0].venc_p, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);

		// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
		vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);
		if (vir_addr_main == 0) {
			DBG_ERR("mmap error !!\r\n\r\n");
			return;
		}

		//DBG_DUMP("enc_pull ....\r\n");
		while(1){
			ret = hd_videoenc_pull_out_buf(photo_cap_info[0].venc_p, &data_pull, -1); // -1 = blocking mode
			if (ret != HD_OK) {
				if (ret != HD_ERR_UNDERRUN){
					DBG_ERR("enc_pull error=%d !!\r\n\r\n", ret);
					return;
				}else{
					//DBG_ERR("enc_pull continue\r\n");
					continue;
				}
			}else{
				break;
			}
		}
		//g_scrjpg_va=g_exifthumb_scr_va+0x10000;

		g_scrjpg_size=data_pull.video_pack[0].size;
		memcpy((UINT8*)g_scrjpg_va, (UINT8*)PHY2VIRT_MAIN(data_pull.video_pack[0].phy_addr),g_scrjpg_size );
		//DBG_DUMP("g_scrjpg_va=0x%x, g_scrjpg_szie=%d\r\n",g_scrjpg_va,g_scrjpg_size);
		if(g_scrjpg_size >=photo_cap_info[idx].screen_bufsize){
			DBG_ERR("scr size over flow wanted=%d, but now=%d\r\n",photo_cap_info[idx].screen_bufsize,g_scrjpg_size);
		}
		#if (SAVE_FILE_DBG ==1)//save file
		snprintf(file_path_main, 64, "/mnt/sd/scr_%lux%lu.jpg",
			photo_cap_info[idx].screen_img_size.w, photo_cap_info[idx].screen_img_size.h);
		DBG_DUMP("save (%s) .... \r\n", file_path_main);

		//----- open output files -----
		if ((hFile = FileSys_OpenFile(file_path_main, FST_CREATE_ALWAYS | FST_OPEN_WRITE)) == NULL) {
			HD_VIDEOENC_ERR("open file (%s) fail....\r\n\r\n", file_path_main);
			return;
		}
		if (hFile){
			FileSys_WriteFile(hFile, (UINT8 *)g_scrjpg_va, &g_scrjpg_size, 0, NULL);
			FileSys_CloseFile(hFile);
		}
		#endif

		// release data
		ret = hd_videoenc_release_out_buf(photo_cap_info[0].venc_p, &data_pull);
		if (ret != HD_OK) {
			printf("scr hd_videoenc_release_out_buf error=%d !!\r\n", ret);
		}

		// mummap for bs buffer
		ret = hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
		if (ret != HD_OK) {
			DBG_ERR("mnumap error !!\r\n\r\n");
		}
	}
}
static void  _ImageApp_Photo_CapQvCB(UINT32 idx)
{
	HD_VIDEO_FRAME video_frame = {0};
	//INT32 wait_ms=-1;//500;
	//UINT32 idx=0;
	HD_RESULT ret;
	IMG_CAP_QV_DATA QVInfor = {0};
	IMG_CAP_YCC_IMG_INFO QvImgInfo = {0};
	IMG_CAP_DATASTAMP_INFO Stamp_CBInfo = {0};

	//hd_videoproc_pull_out_buf(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_QV_PATH], &video_frame, wait_ms);
	//hd_videoout_push_in_buf(g_PhotoDispLink[idx].vout_p[0], &video_frame, NULL, wait_ms);
	//hd_videoproc_release_out_buf(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_QV_PATH], &video_frame);
	while(1){
		if ((ret = hd_videoproc_pull_out_buf(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_QV_THUMB_PATH], &video_frame, -1)) != HD_OK) {
		//if ((ret = hd_videoproc_pull_out_buf(photo_vcap_sensor_info[idx].vproc_p_ext[VPRC_CAP_QV_THUMB_PATH], &video_frame, -1)) != HD_OK) {
			if (ret != HD_ERR_UNDERRUN){
				DBG_ERR("QV  pull_out error, idx=%d, ret=%d\r\n",idx,ret);
				return;
			}else{
				//DBG_ERR("screen proc_pull continue\r\n");
				continue;
			}
		}else{
			break;
		}
	}
	QvImgInfo.ch[IMG_CAP_YUV_Y].width=video_frame.pw[IMG_CAP_YUV_Y];//photo_cap_info[idx].screen_img_size.w;
	QvImgInfo.ch[IMG_CAP_YUV_Y].height=video_frame.ph[IMG_CAP_YUV_Y];//photo_cap_info[idx].screen_img_size.h;
	QvImgInfo.ch[IMG_CAP_YUV_Y].line_ofs=video_frame.loff[IMG_CAP_YUV_Y];//photo_cap_info[idx].screen_img_size.w;
	QvImgInfo.ch[IMG_CAP_YUV_U].width=video_frame.pw[IMG_CAP_YUV_U];//photo_cap_info[idx].screen_img_size.w>>1;
	QvImgInfo.ch[IMG_CAP_YUV_U].height=video_frame.ph[IMG_CAP_YUV_U];//photo_cap_info[idx].screen_img_size.h>>1;
	QvImgInfo.ch[IMG_CAP_YUV_U].line_ofs=video_frame.loff[IMG_CAP_YUV_U];//photo_cap_info[idx].screen_img_size.w>>1;
	QvImgInfo.ch[IMG_CAP_YUV_V].width=video_frame.pw[IMG_CAP_YUV_V];//photo_cap_info[idx].screen_img_size.w>>1;
	QvImgInfo.ch[IMG_CAP_YUV_V].height=video_frame.ph[IMG_CAP_YUV_V];//photo_cap_info[idx].screen_img_size.h>>1;
	QvImgInfo.ch[IMG_CAP_YUV_V].line_ofs=video_frame.loff[IMG_CAP_YUV_V];//photo_cap_info[idx].screen_img_size.w>>1;
	QvImgInfo.pixel_addr[0]=video_frame.phy_addr[0];
	QvImgInfo.pixel_addr[1]=video_frame.phy_addr[1];
	QvImgInfo.pixel_addr[2]=video_frame.phy_addr[2];
	QVInfor.ImgInfo = QvImgInfo;

	Stamp_CBInfo.event=CAP_DS_EVENT_QV;
	Stamp_CBInfo.ImgInfo= QvImgInfo;
	if(photo_cap_info[idx].datastamp == SEL_DATASTAMP_ON && g_PhotoCapMsgCb){
		g_PhotoCapMsgCb(IMG_CAP_CBMSG_GEN_DATASTAMP_PIC,&Stamp_CBInfo);
	}
	if(photo_cap_info[idx].qv_img==SEL_QV_IMG_ON){
		if(g_PhotoCapMsgCb){
			g_PhotoCapMsgCb(IMG_CAP_CBMSG_QUICKVIEW,&QVInfor);
		}
		if ((ret = hd_videoout_push_in_buf(g_PhotoDispLink[0].vout_p[0], &video_frame, NULL, 0)) != HD_OK) {
			DBG_ERR("QV push_in error(%d)\r\n", ret);
		}
	}

	if ((ret = hd_videoproc_release_out_buf(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_QV_THUMB_PATH], &video_frame)) != HD_OK) {
	//if ((ret = hd_videoproc_release_out_buf(photo_vcap_sensor_info[idx].vproc_p_ext[VPRC_CAP_QV_THUMB_PATH], &video_frame)) != HD_OK) {
		DBG_ERR("QV  release_out error(%d)\r\n", ret);
	}


#if 1
		//Get thumbnail
		VF_GFX_SCALE gfx_scale = {0};
		HD_VIDEO_FRAME yuv_frm = {0};
		//HD_VIDEOENC_BUFINFO bsbuf;
		URECT dest_win;
		USIZE src_size, dest_size;
		HD_COMMON_MEM_VB_BLK blk;

		UINT32 vir_addr_main;
		HD_VIDEOENC_BUFINFO phy_buf_main;
		HD_VIDEOENC_BS  data_pull;
		#if (SAVE_FILE_DBG ==1)
		char file_path_main[64] = {0};
		//FILE *f_out_main;
		FST_FILE hFile = NULL;
		static UINT32 file_cnt=0;
		#endif
		#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))

		src_size.w = video_frame.pw[0];
		src_size.h = video_frame.ph[0];
		dest_size.w = PHOTOCAP_THUMB_W;
		dest_size.h = PHOTOCAP_THUMB_H;
		_ImageApp_Photo_Cal_Jpg_Size(&src_size, &dest_size , &dest_win);

		memcpy(&yuv_frm,&video_frame,sizeof(HD_VIDEO_FRAME));

		if ((blk = _ImageApp_Photo_Scale_YUV(&gfx_scale, &yuv_frm, &dest_size, &dest_win, photo_cap_info[idx].qv_img_fmt)) == HD_COMMON_MEM_VB_INVALID_BLK) {
			DBG_ERR("Scale_YUV fail\r\n");
			return;
		}

		#if (SAVE_FILE_DBG ==1)//save yuv data file
		snprintf(file_path_main, 64, "/mnt/sd/thumb_%lux%lu_%d.yuv",
			PHOTOCAP_THUMB_W, PHOTOCAP_THUMB_H,file_cnt);
		DBG_DUMP("thumb save(%s) .... \r\n", file_path_main);
		//file_cnt++;

		//----- open output files -----
		if ((hFile = FileSys_OpenFile(file_path_main, FST_CREATE_ALWAYS | FST_OPEN_WRITE)) == NULL) {
			HD_VIDEOENC_ERR("open file (%s) fail....\r\n\r\n", file_path_main);
			return;
		}
		if (hFile){
			UINT32 file_size=dest_size.w*dest_size.h;
			FileSys_WriteFile(hFile, (UINT8 *)gfx_scale.dst_img.phy_addr[0], &file_size, 0, NULL);
			FileSys_CloseFile(hFile);
		}
		#endif
		photo_cap_venc_in_param.dim.w = PHOTOCAP_THUMB_W;
		photo_cap_venc_in_param.dim.h = PHOTOCAP_THUMB_H;
		photo_cap_venc_in_param.pxl_fmt= photo_cap_info[idx].qv_img_fmt;
		//photo_cap_venc_out_param.jpeg.image_quality = 90;
#if 0//PHOTO_BRC_MODE
		photo_cap_venc_out_param.jpeg.image_quality = 0;
		photo_cap_venc_out_param.jpeg.bitrate       = bitrate;
#else
		photo_cap_venc_out_param.jpeg.image_quality = 90;
		photo_cap_venc_out_param.jpeg.bitrate       = 0;
#endif

		if ((ret = hd_videoenc_stop(photo_cap_info[0].venc_p)) != HD_OK) {
			DBG_ERR("thumb hd_videoenc_stop fail(%d), venc_p=0x%x\r\n", ret, photo_cap_info[0].venc_p);
			return;
		}
		_ImageApp_Photo_venc_set_param(photo_cap_info[0].venc_p, &photo_cap_venc_in_param, &photo_cap_venc_out_param);

		if ((ret = hd_videoenc_start(photo_cap_info[0].venc_p)) != HD_OK) {
			DBG_ERR("thumb hd_videoenc_start fail(%d), venc_p=0x%x\r\n", ret, photo_cap_info[0].venc_p);
			return;
		}
		//DBG_DUMP("enc_push ....\r\n");
		ret = hd_videoenc_push_in_buf(photo_cap_info[0].venc_p, &gfx_scale.dst_img, NULL, -1); // blocking mode
		if (ret != HD_OK) {
			DBG_ERR("enc_push error=%d !!\r\n\r\n", ret);
			return;
		}

		// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
		hd_videoenc_get(photo_cap_info[0].venc_p, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);

		// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
		vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);
		if (vir_addr_main == 0) {
			DBG_ERR("mmap error !!\r\n\r\n");
			return;
		}

		//DBG_DUMP("enc_pull ....\r\n");
		while(1){
			ret = hd_videoenc_pull_out_buf(photo_cap_info[0].venc_p, &data_pull, -1); // -1 = blocking mode
			if (ret != HD_OK) {
				if (ret != HD_ERR_UNDERRUN){
					DBG_ERR("enc_pull error=%d !!\r\n\r\n", ret);
					goto err_exit;
					//return;
				}else{
					//DBG_ERR("enc_pull continue\r\n");
					continue;
				}
			}else{
				break;
			}
		}
		//g_thumbjpg_va=g_exifthumb_scr_va+0x10000+photo_cap_info[idx].screen_bufsize;

		g_thumbjpg_size=data_pull.video_pack[0].size;
		if((g_thumbyuv_va - g_thumbjpg_va) < g_thumbjpg_size){
			DBG_ERR("thumb jpg size over flow ,wanted=%d, but now=%d\r\n", g_thumbyuv_va - g_thumbjpg_va, g_thumbjpg_size);
		}
		memcpy((UINT8*)g_thumbjpg_va, (UINT8*)PHY2VIRT_MAIN(data_pull.video_pack[0].phy_addr),g_thumbjpg_size );
		//DBG_DUMP("g_thumbjpg_va=0x%x, g_thumbjpg_size=%d\r\n",g_thumbjpg_va,g_thumbjpg_size);
		//UINT8* pThumbJpg=(UINT8*)g_thumbjpg_va;
		//DBG_ERR("pThumbJpg[0]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x \r\n",pThumbJpg[0],pThumbJpg[1],pThumbJpg[2],pThumbJpg[3],pThumbJpg[g_thumbjpg_size-2],pThumbJpg[g_thumbjpg_size-1]);

		// release data
		ret = hd_videoenc_release_out_buf(photo_cap_info[0].venc_p, &data_pull);
		if (ret != HD_OK) {
			printf("thumb hd_videoenc_release_out_buf error=%d !!\r\n", ret);
		}

		#if (SAVE_FILE_DBG ==1)//save file
		snprintf(file_path_main, 64, "/mnt/sd/thumb_%lux%lu_%d.jpg",
			PHOTOCAP_THUMB_W, PHOTOCAP_THUMB_H,file_cnt);
		DBG_DUMP("thumb save(%s) .... \r\n", file_path_main);
		file_cnt++;

		//----- open output files -----
		if ((hFile = FileSys_OpenFile(file_path_main, FST_CREATE_ALWAYS | FST_OPEN_WRITE)) == NULL) {
			HD_VIDEOENC_ERR("open file (%s) fail....\r\n\r\n", file_path_main);
			return;
		}
		if (hFile){
			FileSys_WriteFile(hFile, (UINT8 *)g_thumbjpg_va, &g_thumbjpg_size, 0, NULL);
			FileSys_CloseFile(hFile);
		}
		#endif
err_exit:
		// mummap for bs buffer
		ret = hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
		if (ret != HD_OK) {
			DBG_ERR("mnumap error !!\r\n\r\n");
		}

#endif
}
static BOOL _ImageApp_Photo_ChkCapNum(UINT32 PicNum, UINT32 CapCnt, PHOTO_CAP_S2DET_FP S2DetFP)
{

	UINT32 MaxCnt;

	MaxCnt = PicNum;

	DBG_IND("ChkCapNum CapCnt=%d, MaxCnt=%d\r\n",CapCnt,MaxCnt);

	if (MaxCnt == SEL_PICNUM_INF) {
		if (S2DetFP) {
			return (S2DetFP());
		} else {
			return 0;//(uiFlag & FLG_CAPTURE_START_1);
		}
	} else if (CapCnt < (MaxCnt)) {
		return 1;
	}

	return 0;
}

static void  _ImageApp_Photo_CapImgCB(void)
{

	HD_VIDEO_FRAME video_frame = {0};
	//HD_VIDEO_FRAME vcap_video_frame = {0};
	HD_RESULT ret=0;
	UINT32 idx=0;
	//UINT32	prc_count=0;  
  MEM_RANGE JPEG_BS_INFO[3]; 
	//UINT32 	enc_count=0;
	HD_VIDEOCAP_SYSINFO sys_info = {0};
	UINT64 vd_count; 
	UINT32 vir_addr_main= 0;
	HD_VIDEOENC_BUFINFO phy_buf_main= {0};
	HD_VIDEOENC_BS  data_pull= {0};
	UINT32 	flow_max_shot=1;
	UINT32 	save_count=0;

#if (SAVE_FILE_DBG ==1)
	char file_path_main[64] = {0};
	//FILE *f_out_main;
	FST_FILE hFile = NULL;

#endif
	//UINT32 j=0;
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))
	UINT32  cap_main_path=VPRC_CAP_MAIN_PATH;
	//scale
	//VF_GFX_SCALE gfx_scale = {0};
	//HD_VIDEO_FRAME yuv_frm = {0};
	//HD_VIDEOENC_BUFINFO bsbuf;
	//URECT dest_win;
	//USIZE src_size, dest_size;
	//HD_COMMON_MEM_VB_BLK blk;

	MEM_RANGE exif_data={0}, exif_jpg={0}, main_jpg={0}, scr_jpg={0}, dst_jpg_file={0};
	//MEM_RANGE main_jpg, scr_jpg, dst_jpg_file={0};
	IMG_CAP_DATASTAMP_INFO Stamp_CBInfo = {0};
	UINT32 IsVcapRelease=1;
	//INT32    writecb_rt=FST_STA_OK;
	VOS_TICK t1=0, t2=0;
	FLGPTN uiFlag=0;
	UINT32 BufIdx=0;

	AET_OPERATION ae_ui_operation = {0};
	AWBT_OPERATION awb_ui_operation = {0};
	IQT_OPERATION iq_ui_operation = {0};
#if (LOAD_ISP_DTSI == 1)
	IQT_DTSI_INFO dtsi_info = {0};
#endif
	HD_VIDEOCAP_OUT vcap_out_param[PHOTO_VID_IN_MAX] = {0};
	UINT32 bSizeChkRetry=0;
	UINT32 SizeChkRetryCount=0;
	UINT32 rho_retrycnt;
	//UINT32 start_time = 0, time1 = 0;
	UINT32 shdr_pull_count=0;
	AET_LONGEXP_MODE longexp = {0};
	AET_STATUS_INFO status = {0};
	HD_VIDEOCAP_IN vcap_in_param[PHOTO_VID_IN_MAX] = {0};
	HD_VIDEOCAP_CROP vcap_crop_param[PHOTO_VID_IN_MAX] = {0};

	//sync from hdal/samples/video_capture_flow.c
	//#############
	//load cap 3A Start
	//#############

	if ((ret = vendor_isp_init()) != HD_OK) {
		DBG_ERR("vendor_isp_init fail(%d)\r\n", ret);
	}

	longexp.id = 0;
	vendor_isp_get_ae(AET_ITEM_LONGEXP, &longexp);

				
	status.id = 0;
	vendor_isp_get_ae(AET_ITEM_STATUS, &status);
	
	ae_ui_operation.id = 0;
	ae_ui_operation.operation = AE_OPERATION_CAPTURE;
	ret |= vendor_isp_set_ae(AET_ITEM_OPERATION, &ae_ui_operation);
	awb_ui_operation.id = 0;
	awb_ui_operation.operation = AWB_OPERATION_CAPTURE;
	ret  |= vendor_isp_set_awb(AWBT_ITEM_OPERATION, &awb_ui_operation);
	iq_ui_operation.id = 0;
	iq_ui_operation.operation = IQ_UI_OPERATION_CAPTURE;
	ret  |= vendor_isp_set_iq(IQT_ITEM_OPERATION, &iq_ui_operation);
	if (ret  != HD_OK) {
		printf("set 3A/IQ OPERATION_CAPTURE fail!! \r\n");
	}

	//usleep(68000); // default set to wait 1 frame time (> 33ms)
	hd_videocap_get(photo_vcap_sensor_info[PHOTO_VID_IN_1].video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
	vd_count = sys_info.vd_count;

	{
		int i;
		for(i=0; i<30; i++){
			usleep(11000);
			//msleep(9);
			hd_videocap_get(photo_vcap_sensor_info[PHOTO_VID_IN_1].video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
			if((sys_info.vd_count - vd_count)>2)
				break;
		}
		//DBG_ERR("wait %d \r\n",i);
	}
	//vos_perf_mark(&start_time);
	//DBG_ERR("start %d\r\n",start_time);
	if ((ret = vendor_isp_uninit()) != HD_OK) {
		DBG_ERR("vendor_isp_uninit fail(%d)\r\n", ret);
	}
	//#############
	//load cap 3A End
	//#############
	//for (idx= PHOTO_VID_IN_1; idx < PHOTO_VID_IN_MAX; idx++) {

	//DBG_DUMP("stop liveview - begin\n");
	ImageApp_Photo_Disp_SetPreviewShow(0);

	//hd_videocap_stop(p_stream0->cap_path);
	//hd_videoproc_stop(p_stream0->proc_path);
	//hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	for (idx= PHOTO_VID_IN_1; idx < PHOTO_VID_IN_MAX; idx++) {
		DBG_IND("CapImgCB idx=%d, enable=%d\r\n", idx, photo_vcap_sensor_info[idx - PHOTO_VID_IN_1].enable);
		if (photo_vcap_sensor_info[idx - PHOTO_VID_IN_1].enable) {
			if ((ret = hd_videocap_stop(photo_vcap_sensor_info[idx].vcap_p[0])) != HD_OK) {
				DBG_ERR("set hd_videocap_stop fail(%d)\r\n", ret);
			}
			//if ((ret = hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[0][g_photo_3ndr_path])) != HD_OK) {
			//	DBG_ERR("set hd_videoproc_stop fail(%d)\r\n", ret);
			//}

			if ((ret = hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_DISPLAY_PATH])) != HD_OK) {
				DBG_ERR("set hd_videoproc_stop fail(%d)\r\n", ret);
			}
			if ((ret = hd_videocap_unbind(HD_VIDEOCAP_OUT(idx, 0))) != HD_OK) {
				DBG_ERR("set hd_videocap_unbind fail(%d)\r\n", ret);
			}
		}
	}
	//DBG_DUMP("stop liveview - end\n");


	//process_thread

	//#############
	//load cap IQ Start
	//#############

	if ((ret = vendor_isp_init()) != HD_OK) {
		DBG_ERR("vendor_isp_init fail(%d)\r\n", ret);
	}

#if (LOAD_ISP_DTSI == 1)

#if 0
	dtsi_info.id = 0;
	strncpy(dtsi_info.node_path, "/isp/iq/imx290_iq_0_cap", DTSI_NAME_LENGTH);
	strncpy(dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
	dtsi_info.buf_addr = NULL;
	ret |= vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &dtsi_info);
	if (ret != HD_OK) {
		DBG_ERR("set capture dtsi fail!! \r\n");
	}
#endif
	if (strlen(photo_vcap_sensor_info[idx].iqcap_path.path) != 0) {
		//IQT_DTSI_INFO iq_dtsi_info;
		dtsi_info.id = photo_vcap_sensor_info[idx].iqcap_path.id;
		strncpy(dtsi_info.node_path, photo_vcap_sensor_info[idx].iqcap_path.path, 31);
		strncpy(dtsi_info.file_path, "null", DTSI_NAME_LENGTH);
		dtsi_info.buf_addr = (UINT8 *)photo_vcap_sensor_info[idx].iqcap_path.addr;
		if ((ret = vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &dtsi_info)) != HD_OK) {
			DBG_ERR("set capture dtsi fail!! \r\n");
		}
	}else{
		DBG_ERR("iqcap_path.path fail!! \r\n");
	}
#endif

	if ((ret = vendor_isp_uninit()) != HD_OK) {
		DBG_ERR("vendor_isp_uninit fail(%d)\r\n", ret);
	}

	//#############
	//load cap IQ End
	//#############
	idx=0;
	flow_max_shot=photo_cap_info[idx].picnum;
	DBG_IND("picnum=%d\r\n",photo_cap_info[idx].picnum);
	//while(save_count < flow_max_shot){
	while(_ImageApp_Photo_ChkCapNum(flow_max_shot, save_count, g_PhotoCapDetS2Cb)){
		for (idx= PHOTO_VID_IN_1; idx < PHOTO_VID_IN_MAX; idx++) {
			DBG_IND("CapImgCB idx=%d, enable=%d\r\n", idx, photo_vcap_sensor_info[idx - PHOTO_VID_IN_1].enable);
			//if (photo_vcap_sensor_info[idx - PHOTO_VID_IN_1].enable) {
			if (photo_cap_info[idx - PHOTO_VID_IN_1].actflag) {

				//for jira IVOT_N12033_CO-63, IVOT_N12061_CO-185
				vcap_out_param[idx].pxlfmt = photo_vcap_sensor_info[idx].capout_pxlfmt;
				vcap_out_param[idx].dir = HD_VIDEO_DIR_NONE;
				if (photo_vcap_sensor_info[idx].vcap_ctrl.func & HD_VIDEOCAP_FUNC_SHDR) {
					vcap_out_param[idx].depth = 2; //set 2 to allow pull
				}else{
					vcap_out_param[idx].depth = 1; //set 1 to allow pull
				}
				if ((ret = hd_videocap_set(photo_vcap_sensor_info[idx].vcap_p[0], HD_VIDEOCAP_PARAM_OUT, &vcap_out_param[idx])) != HD_OK) {
					DBG_ERR("set HD_VIDEOCAP_PARAM_IN_CROP fail(%d)\r\n", ret);
				}
				if(g_photo_cap_sensor_mode_cfg[idx].in_size.w && g_photo_cap_sensor_mode_cfg[idx].in_size.h){
					if ((ret = hd_videocap_get(photo_vcap_sensor_info[idx].vcap_p[0], HD_VIDEOCAP_PARAM_IN, &vcap_in_param[idx])) != HD_OK) {
						DBG_ERR("get HD_VIDEOCAP_PARAM_IN fail(%d)\r\n", ret);
					}
					//vcap_in_param[idx].dim.w= prev_vcap_in_param[idx].dim.w;
					//vcap_in_param[idx].dim.h= prev_vcap_in_param[idx].dim.h;

					memcpy(&g_prv_vcap_in_param[idx], &vcap_in_param[idx],sizeof(HD_VIDEOCAP_IN));

					vcap_in_param[idx].dim.w = g_photo_cap_sensor_mode_cfg[idx].in_size.w;//photo_vcap_sensor_info[idx].sSize[0].w;//sz.w;
					vcap_in_param[idx].dim.h = g_photo_cap_sensor_mode_cfg[idx].in_size.h;//photo_vcap_sensor_info[idx].sSize[0].h;//sz.h;
					vcap_in_param[idx].frc =g_photo_cap_sensor_mode_cfg[idx].frc;

					if ((ret = hd_videocap_set(photo_vcap_sensor_info[idx].vcap_p[0], HD_VIDEOCAP_PARAM_IN, &vcap_in_param[idx])) != HD_OK) {
						DBG_ERR("set HD_VIDEOCAP_PARAM_IN fail(%d)\r\n", ret);
					}
					//set vcap crop out
					if ((ret = hd_videocap_get(photo_vcap_sensor_info[idx].vcap_p[0], HD_VIDEOCAP_PARAM_OUT_CROP, &vcap_crop_param[idx])) != HD_OK) {
						DBG_ERR("get HD_VIDEOCAP_PARAM_OUT_CROP fail(%d)\r\n", ret);
					}
					memcpy(&g_prv_vcap_cropout_param[idx], &vcap_crop_param[idx],sizeof(HD_VIDEOCAP_CROP));

					memset(&vcap_crop_param[idx],0, sizeof(HD_VIDEOCAP_CROP));
					ISIZE sensor_info_size={ g_photo_cap_sensor_mode_cfg[idx].in_size.w,  g_photo_cap_sensor_mode_cfg[idx].in_size.h};
					UINT32 img_ratio=photo_cap_info[idx].img_ratio;
					HD_URECT vcap_out_crop_rect=_ImageApp_Photo_vcap_set_crop_out(img_ratio ,sensor_info_size);

					vcap_crop_param[idx].mode = HD_CROP_ON;
					vcap_crop_param[idx].win.rect.x   = vcap_out_crop_rect.x;
					vcap_crop_param[idx].win.rect.y   = vcap_out_crop_rect.y;
					vcap_crop_param[idx].win.rect.w   = vcap_out_crop_rect.w;
					vcap_crop_param[idx].win.rect.h   = vcap_out_crop_rect.h;
					vcap_crop_param[idx].align.w = 4;
					vcap_crop_param[idx].align.h = 4;

					if ((ret = hd_videocap_set(photo_vcap_sensor_info[idx].vcap_p[0], HD_VIDEOCAP_PARAM_OUT_CROP, &vcap_crop_param[idx])) != HD_OK) {
						DBG_ERR("set HD_VIDEOCAP_PARAM_IN_CROP fail(%d)\r\n", ret);
					}
				}
				if ((ret = hd_videocap_start(photo_vcap_sensor_info[idx].vcap_p[0])) != HD_OK) {
					DBG_ERR("set hd_videocap_start fail(%d)\r\n", ret);
				}
				
				hd_videocap_get(photo_vcap_sensor_info[idx].video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
				if(g_photo_cap_sensor_mode_cfg[idx].in_size.w && g_photo_cap_sensor_mode_cfg[idx].in_size.h){
					vd_count = sys_info.vd_count;

					{
						int i;
						for(i=0; i<30; i++){
							usleep(11000);
							//msleep(9);
							hd_videocap_get(photo_vcap_sensor_info[idx].video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
							if((sys_info.vd_count - vd_count)>2){
								break;
							}
						}
						DBG_DUMP("wait=%d ,vd_count=%d,%d\r\n",i, sys_info.vd_count , vd_count);
					}
				}

				if ((ret = vendor_isp_init()) != HD_OK) {
					DBG_ERR("vendor_isp_init fail(%d)\r\n", ret);
				}
				{


					if (longexp.mode == AE_CAP_LONGEXP_ON) {
						AET_MANUAL manual = {0};
						ISPT_SENSOR_EXPT sensor_expt = {0};
						ISPT_SENSOR_GAIN sensor_gain = {0}; 				
						//set ae manual
						manual.id = 0;
						manual.manual.expotime = status.status_info.expotime[0];
						manual.manual.iso_gain = status.status_info.iso_gain[0];
						//manual.manual.lock_en = 1;
						manual.manual.mode= LOCK_MODE;
						printf("id = %d, mode = %d, manual_expt = %d, manual_gain = %d\n", manual.id, manual.manual.mode, manual.manual.expotime, manual.manual.iso_gain);

					//	vendor_isp_set_ae(AET_ITEM_MANUAL, &manual);
						//set sensor gain/exposure
						sensor_expt.id = 0;
						sensor_expt.time[0] = manual.manual.expotime;
						vendor_isp_set_common(ISPT_ITEM_SENSOR_EXPT, &sensor_expt);
						sensor_gain.ratio[0] = manual.manual.iso_gain*1000/100;
						vendor_isp_set_common(ISPT_ITEM_SENSOR_GAIN, &sensor_gain);

					//	ae_ui_operation.id = 0;
					//	ae_ui_operation.operation = AE_OPERATION_MOVIE;
					//	ret |= vendor_isp_set_ae(AET_ITEM_OPERATION, &ae_ui_operation);									
					}
				}
				
				if ((ret = vendor_isp_uninit()) != HD_OK) {
					DBG_ERR("vendor_isp_uninit fail(%d)\r\n", ret);
				}
				
				{
					cap_main_path=VPRC_CAP_MAIN_PATH;
					//DBG_DUMP("Cap main and 3DNR path diff\r\n");
					if ((ret = hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_MAIN_PATH])) != HD_OK) {
						DBG_ERR("VPRC_CAP_MAIN_PATH set hd_videoproc_start fail ,idx=%d ,ret=%d\r\n",idx, ret);
					}
				}
				if(photo_cap_info[idx].screen_img==SEL_SCREEN_IMG_ON){
					if ((ret = hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_SCR_PATH])) != HD_OK) {
						DBG_ERR("VPRC_CAP_SCR_PATH set hd_videoproc_start fail ,idx=%d ,ret=%d\r\n",idx, ret);
					}
				}
				if(1){//photo_cap_info[idx].qv_img==SEL_QV_IMG_ON){ //if 1 for Exif thumb
					if ((ret = hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_QV_THUMB_PATH])) != HD_OK) {
						DBG_ERR("VPRC_CAP_QV_THUMB_PATH set hd_videoproc_start fail ,idx=%d ,ret=%d\r\n",idx, ret);
					}
				}
				//IPL rotate
				{
					HD_VIDEOPROC_IN vprc_in_param = {0};
					UINT32 j;
					HD_PATH_ID video_proc_path= HD_VIDEOPROC_IN((idx*PHOTO_VID_IN_MAX+1), 0);
					hd_videoproc_get(video_proc_path, HD_VIDEOPROC_PARAM_IN, &vprc_in_param);

					vprc_in_param.dir = photo_ipl_mirror[idx].mirror_type;
					// vprc
					if ((ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_IN, &vprc_in_param)) != HD_OK) {
						DBG_ERR("hd_videoproc_set fail(%d)\n", ret);
					}
					// resetrt one running path to active setting
					for(j = 0; j < 4; j++) {
						if (photo_vcap_sensor_info[idx].vproc_p_phy[1][j]) {
							if ((ret = hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[1][j])) != HD_OK) {
								DBG_ERR("hd_videoproc_start fail(%d)\n", ret);
							}
							break;
						}
					}
				}


				//if ((ret = hd_videoenc_start(photo_cap_info[idx].venc_p)) != HD_OK) {
				if ((ret = hd_videoenc_start(photo_cap_info[0].venc_p)) != HD_OK) {
					DBG_ERR("init hd_videoenc_start fail(%d), venc_p=0x%x\r\n", ret, photo_cap_info[0].venc_p);
					return;
				}

				//DBG_ERR("cap_pull ....\r\n");
				if(1){//photo_cap_info[idx].qv_img==SEL_QV_IMG_ON){
					shdr_pull_count=0;
					CAP_SHDR_QV:
					if(IsVcapRelease){
						while(1){
							ret = hd_videocap_pull_out_buf(photo_vcap_sensor_info[idx].vcap_p[0], &video_frame, -1); // -1 = blocking mode
							if (ret != HD_OK) {
								if (ret != HD_ERR_UNDERRUN){
									DBG_ERR("cap_pull error=%d !!\r\n\r\n", ret);
									return;
								}else{
									//DBG_ERR("QV cap_pull continue\r\n");
									continue;
								}
							}else{
								break;
							}
						}
					}
				if (longexp.mode == AE_CAP_LONGEXP_ON) {
					hd_videocap_stop(photo_vcap_sensor_info[idx].vcap_p[0]);
					IsVcapRelease = 0;
				}
					
					ret = hd_videoproc_push_in_buf(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_QV_THUMB_PATH], &video_frame, NULL, 0); // only support non-blocking mode now
					//ret = hd_videoproc_push_in_buf(photo_vcap_sensor_info[idx].vproc_p_ext[VPRC_CAP_QV_THUMB_PATH], &video_frame, NULL, 0); // only support non-blocking mode now
					if (ret != HD_OK) {
						DBG_ERR("[%d]QV_PATH proc_push error=%d !!\r\n\r\n", idx,ret);
						return;
					}
				if (longexp.mode != AE_CAP_LONGEXP_ON) {	
					ret = hd_videocap_release_out_buf(photo_vcap_sensor_info[idx].vcap_p[0], &video_frame);
					if (ret != HD_OK) {
						DBG_ERR("cap_release error=%d !!\r\n\r\n", ret);
						return;
					}
					IsVcapRelease=1;
				}
					

					shdr_pull_count++;
					if (photo_vcap_sensor_info[idx].vcap_ctrl.func & HD_VIDEOCAP_FUNC_SHDR && shdr_pull_count< 2) {
						goto CAP_SHDR_QV;
					}
					if(g_PhotoCapQvCb){
						g_PhotoCapQvCb(idx);
					}
				}

				//DBG_DUMP("proc_push ....\r\n");
				//DBG_DUMP("proc %d/%d\r\n", prc_count+1, flow_max_shot);
				if(photo_cap_info[idx].screen_img==SEL_SCREEN_IMG_ON){
					shdr_pull_count=0;
					CAP_SHDR_SCR:
					if(IsVcapRelease){
						while(1){
							ret = hd_videocap_pull_out_buf(photo_vcap_sensor_info[idx].vcap_p[0], &video_frame, -1); // -1 = blocking mode
							if (ret != HD_OK) {
								if (ret != HD_ERR_UNDERRUN){
									DBG_ERR("cap_pull error=%d !!\r\n\r\n", ret);
									return;
								}else{
									//DBG_ERR("SCR cap_pull continue\r\n");
									continue;
								}
							}else{
								break;
							}
						}
					}
					ret = hd_videoproc_push_in_buf(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_SCR_PATH], &video_frame, NULL, 0); // only support non-blocking mode now
					if (ret != HD_OK) {
						DBG_ERR("SCR_PATH proc_push error=%d !!\r\n\r\n", ret);
						return;
					}
					if (longexp.mode != AE_CAP_LONGEXP_ON) {
					ret = hd_videocap_release_out_buf(photo_vcap_sensor_info[idx].vcap_p[0], &video_frame);
					if (ret != HD_OK) {
						DBG_ERR("cap_release error=%d !!\r\n\r\n", ret);
						return;
					}
					IsVcapRelease=1;
					}
					shdr_pull_count++;
					if (photo_vcap_sensor_info[idx].vcap_ctrl.func & HD_VIDEOCAP_FUNC_SHDR && shdr_pull_count< 2) {
						goto CAP_SHDR_SCR;
					}

					if(g_PhotoCapScrCb){
						g_PhotoCapScrCb(idx);
					}
				}


				shdr_pull_count=0;
				CAP_SHDR_MAIN:

				if(IsVcapRelease){
					while(1){
						ret = hd_videocap_pull_out_buf(photo_vcap_sensor_info[idx].vcap_p[0], &video_frame, -1); // -1 = blocking mode
						if (ret != HD_OK) {
							if (ret != HD_ERR_UNDERRUN){
								DBG_ERR("cap_pull error=%d !!\r\n\r\n", ret);
								return;
							}else{
								//DBG_ERR("SCR cap_pull continue\r\n");
								continue;
							}
						}else{
							break;
						}
					}
				}
				ret = hd_videoproc_push_in_buf(photo_vcap_sensor_info[idx].vproc_p_phy[1][cap_main_path], &video_frame, NULL, 0); // only support non-blocking mode now
				if (ret != HD_OK) {
					DBG_ERR("MAIN_PATH proc_push error=%d !!\r\n\r\n", ret);
					return;
				}
				ret = hd_videocap_release_out_buf(photo_vcap_sensor_info[idx].vcap_p[0], &video_frame);
				if (ret != HD_OK) {
					DBG_ERR("cap_release error=%d !!\r\n\r\n", ret);
					return;
				}
				IsVcapRelease=1;

				shdr_pull_count++;
				if (photo_vcap_sensor_info[idx].vcap_ctrl.func & HD_VIDEOCAP_FUNC_SHDR && shdr_pull_count< 2) {
					goto CAP_SHDR_MAIN;
				}

				if(g_PhotoCapMsgCb){
					g_PhotoCapMsgCb(IMG_CAP_CBMSG_SET_EXIF_DATA, &idx);
				}


				memset(&video_frame, 0, sizeof(HD_VIDEO_FRAME));

				//DBG_DUMP("main proc_pull ....\r\n");
				while(1){
					ret = hd_videoproc_pull_out_buf(photo_vcap_sensor_info[idx].vproc_p_phy[1][cap_main_path], &video_frame, -1); // -1 = blocking mode
					if (ret != HD_OK) {
						if (ret != HD_ERR_UNDERRUN){
							DBG_ERR("proc_pull error=%d !!\r\n\r\n", ret);
							return;
						}else{
							//DBG_ERR("main proc_pull continue\r\n");
							continue;
						}
					}else{
						break;
					}
				}
				//DBG_DUMP("proc frame.count = %llu\r\n", video_frame.count);


				if (photo_cap_info[idx].datastamp == SEL_DATASTAMP_ON && g_PhotoCapMsgCb) {
					IMG_CAP_YCC_IMG_INFO PriImgInfo = {0};
					memset(&Stamp_CBInfo,0, sizeof(IMG_CAP_DATASTAMP_INFO));

					PriImgInfo.ch[IMG_CAP_YUV_Y].width=video_frame.pw[IMG_CAP_YUV_Y];//photo_cap_info[idx].screen_img_size.w;
					PriImgInfo.ch[IMG_CAP_YUV_Y].height=video_frame.ph[IMG_CAP_YUV_Y];//photo_cap_info[idx].screen_img_size.h;
					PriImgInfo.ch[IMG_CAP_YUV_Y].line_ofs=video_frame.loff[IMG_CAP_YUV_Y];//photo_cap_info[idx].screen_img_size.w;
					PriImgInfo.ch[IMG_CAP_YUV_U].width=video_frame.pw[IMG_CAP_YUV_U];//photo_cap_info[idx].screen_img_size.w>>1;
					PriImgInfo.ch[IMG_CAP_YUV_U].height=video_frame.ph[IMG_CAP_YUV_U];//photo_cap_info[idx].screen_img_size.h>>1;
					PriImgInfo.ch[IMG_CAP_YUV_U].line_ofs=video_frame.loff[IMG_CAP_YUV_U];//photo_cap_info[idx].screen_img_size.w>>1;
					PriImgInfo.ch[IMG_CAP_YUV_V].width=video_frame.pw[IMG_CAP_YUV_V];//photo_cap_info[idx].screen_img_size.w>>1;
					PriImgInfo.ch[IMG_CAP_YUV_V].height=video_frame.ph[IMG_CAP_YUV_V];//photo_cap_info[idx].screen_img_size.h>>1;
					PriImgInfo.ch[IMG_CAP_YUV_V].line_ofs=video_frame.loff[IMG_CAP_YUV_V];//photo_cap_info[idx].screen_img_size.w>>1;
					PriImgInfo.pixel_addr[0]=video_frame.phy_addr[0];
					PriImgInfo.pixel_addr[1]=video_frame.phy_addr[1];
					PriImgInfo.pixel_addr[2]=video_frame.phy_addr[2];

					//DBG_DUMP("screen video_frame.pw[0]=%d, %d, %d\r\n", video_frame.pw[0],video_frame.ph[0],video_frame.loff[0]);
					//DBG_DUMP("screen video_frame.pw[1]=%d, %d, %d\r\n", video_frame.pw[1],video_frame.ph[1],video_frame.loff[1]);
					//DBG_DUMP("screen video_frame.pw[2]=%d, %d, %d\r\n", video_frame.pw[2],video_frame.ph[2],video_frame.loff[2]);
					//DBG_DUMP("video_frame.phy_addr[0]=0x%x, 0x%x, 0x%x\r\n", video_frame.phy_addr[0],video_frame.phy_addr[1],video_frame.phy_addr[2]);

					Stamp_CBInfo.event=CAP_DS_EVENT_PRI;
					Stamp_CBInfo.ImgInfo= PriImgInfo;
					g_PhotoCapMsgCb(IMG_CAP_CBMSG_GEN_DATASTAMP_PIC,&Stamp_CBInfo);
				}


				photo_cap_venc_in_param.dim.w = photo_cap_info[idx].sCapSize.w;
				photo_cap_venc_in_param.dim.h = photo_cap_info[idx].sCapSize.h;
				photo_cap_venc_in_param.pxl_fmt = photo_cap_info[idx].jpgfmt;
				//photo_cap_venc_out_param.jpeg.image_quality = photo_cap_info[idx].quality;
				bSizeChkRetry=0;
				SizeChkRetryCount=1;
				rho_retrycnt = (photo_cap_info[idx].rho_retrycnt != 0)? photo_cap_info[idx].rho_retrycnt: 4;
				chk_size_retry:
#if 0//PHOTO_BRC_MODE
				#if 0
				photo_cap_venc_out_param.jpeg.image_quality = 0;
				photo_cap_venc_out_param.jpeg.bitrate       = photo_cap_info[idx].rho_targetsize*8;
				photo_cap_venc_out_param.jpeg.frame_rate_base       = 1;
				photo_cap_venc_out_param.jpeg.frame_rate_incr       = 1;
				#else
				photo_cap_venc_out_param.jpeg.image_quality = 0;
				photo_cap_venc_out_param.jpeg.bitrate       = photo_cap_info[idx].rho_targetsize*8*30;
				photo_cap_venc_out_param.jpeg.frame_rate_base       = 0;
				photo_cap_venc_out_param.jpeg.frame_rate_incr       = 1;


				VENDOR_VIDEOENC_JPG_BRC_CFG jpg_brc_cfg = {0};

				jpg_brc_cfg.upper_bound =  photo_cap_info[idx].rho_hboundsize;
				jpg_brc_cfg.lower_bound = photo_cap_info[idx].rho_lboundsize;
				jpg_brc_cfg.brc_retry_cnt = photo_cap_info[idx].rho_retrycnt;
				jpg_brc_cfg.quality_step = photo_cap_info[idx].rho_retrycnt;
				jpg_brc_cfg.init_quality= photo_cap_info[idx].rho_initqf;

				if ((ret = vendor_videoenc_set(photo_cap_info[0].venc_p, VENDOR_VIDEOENC_PARAM_OUT_JPG_BRC, &jpg_brc_cfg)) != HD_OK) {
					DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_JPG_BRC) failed(%d)\r\n", ret);
				}

				#endif
#else
				if(SizeChkRetryCount && SizeChkRetryCount<= rho_retrycnt){
					if(photo_cap_info[idx].quality>SizeChkRetryCount*10){
						photo_cap_venc_out_param.jpeg.image_quality = photo_cap_info[idx].quality-SizeChkRetryCount*10;
					}else{
						DBG_WRN("quality get minimum, image_quality=%d, SizeChkRetryCount=%d\r\n", photo_cap_venc_out_param.jpeg.image_quality, SizeChkRetryCount);
					}
				}else{
					photo_cap_venc_out_param.jpeg.image_quality = photo_cap_info[idx].quality;
				}
				photo_cap_venc_out_param.jpeg.bitrate       = 0;
				photo_cap_venc_out_param.jpeg.frame_rate_base       = 1;
				photo_cap_venc_out_param.jpeg.frame_rate_incr       = 1;
				photo_cap_venc_out_param.jpeg.retstart_interval     = 0;
				bSizeChkRetry =1;
#endif
				DBG_IND("main quality=%d, targetsize=%d\r\n", photo_cap_venc_out_param.jpeg.image_quality, photo_cap_venc_out_param.jpeg.bitrate/(8*30));

				hd_videoenc_stop(photo_cap_info[0].venc_p);
				_ImageApp_Photo_venc_set_param(photo_cap_info[0].venc_p, &photo_cap_venc_in_param, &photo_cap_venc_out_param);

				if ((ret = hd_videoenc_start(photo_cap_info[0].venc_p)) != HD_OK) {
					DBG_ERR("main hd_videoenc_start fail(%d), venc_p=0x%x\r\n", ret, photo_cap_info[0].venc_p);
					return;
				}

				//DBG_DUMP("enc_push ....\r\n");
				//DBG_DUMP("encode %d/%d\r\n", enc_count+1, flow_max_shot);
				ret = hd_videoenc_push_in_buf(photo_cap_info[0].venc_p, &video_frame, NULL, -1); // blocking mode
				if (ret != HD_OK) {
					DBG_ERR("enc_push error=%d !!\r\n\r\n", ret);
					return;
				}
				//move down for size check retry flow
				//DBG_DUMP("proc_release ....\r\n");
				//ret = hd_videoproc_release_out_buf(photo_vcap_sensor_info[idx].vproc_p_phy[1][cap_main_path], &video_frame);
				//if (ret != HD_OK) {
				//	DBG_ERR("proc_release error=%d !!\r\n\r\n", ret);
				//	return;
				//}

				// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
				hd_videoenc_get(photo_cap_info[0].venc_p, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);

				// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
				vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);
				if (vir_addr_main == 0) {
					DBG_ERR("mmap error !!\r\n\r\n");
					return;
				}

				//DBG_DUMP("enc_pull ....\r\n");
				while(1){
					ret = hd_videoenc_pull_out_buf(photo_cap_info[0].venc_p, &data_pull, -1); // -1 = blocking mode
					if (ret != HD_OK) {
						if (ret != HD_ERR_UNDERRUN){
							DBG_ERR("enc_pull error=%d !!\r\n\r\n", ret);
							return;
						}else{
							//DBG_ERR("main enc_pull continue\r\n");
							continue;
						}
					}else{
						break;
					}
				}



				main_jpg.addr = PHY2VIRT_MAIN(data_pull.video_pack[0].phy_addr);    // get va
				main_jpg.size = data_pull.video_pack[0].size;
				DBG_IND("main_jpg.size=%d, rho_retrycnt=%d, filebufsize=%d\r\n",main_jpg.size, rho_retrycnt,photo_cap_info[idx].filebufsize);
				if(bSizeChkRetry==1  && (main_jpg.size >photo_cap_info[idx].filebufsize) && (SizeChkRetryCount< rho_retrycnt)){
					SizeChkRetryCount++;

					//printf("enc_release ....\r\n");
					ret = hd_videoenc_release_out_buf(photo_cap_info[0].venc_p, &data_pull);
					if (ret != HD_OK) {
						DBG_ERR("enc_release error=%d !!\r\n", ret);
					}

					// mummap for bs buffer
					ret = hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
					if (ret != HD_OK) {
						DBG_ERR("mnumap error !!\r\n\r\n");
					}
					goto chk_size_retry;
				}


				//DBG_DUMP("proc_release ....\r\n");
				ret = hd_videoproc_release_out_buf(photo_vcap_sensor_info[idx].vproc_p_phy[1][cap_main_path], &video_frame);
				if (ret != HD_OK) {
					DBG_ERR("proc_release error=%d !!\r\n\r\n", ret);
					return;
				}

				DBG_IND("JPG_QUEUE_NUM=%d, QUEUE_F=%d, %d, jpg_buff=%d\r\n",NORMAL_JPG_QUEUE_NUM(), NORMAL_JPG_QUEUE_F(), NORMAL_JPG_QUEUE_R(),photo_cap_info[idx].jpg_buff);

				while ((NORMAL_JPG_QUEUE_F() >= NORMAL_JPG_QUEUE_R()) && (NORMAL_JPG_QUEUE_NUM() >= photo_cap_info[idx].jpg_buff)) {
					DBG_IND("[flow]wait free buffer %d %d\r\n", NORMAL_JPG_QUEUE_F(), NORMAL_JPG_QUEUE_R());
					vos_flag_wait(&uiFlag, g_photo_writefile_flg_id, FLGPHOTO_WRITEFILE_COMPLETE, TWF_ORW | TWF_CLR);
				}

				BufIdx = (NORMAL_JPG_QUEUE_F() % photo_cap_info[idx].jpg_buff);
				DBG_IND("BufIdx=%d, main_jpg.size=%d\r\n",BufIdx, main_jpg.size);




				scr_jpg.addr=g_scrjpg_va;
				scr_jpg.size=g_scrjpg_size;

				// step: create exif
				#if 0
				exif_data.addr = g_exifthumb_scr_va;
				exif_data.size = 0x10000;
				#else
				exif_data.addr = g_jpg_buff_va[0][BufIdx] ;
				exif_data.size = 0x10000;
				#endif
				exif_jpg.addr = g_thumbjpg_va;    // va
				exif_jpg.size = g_thumbjpg_size;
				if (EXIF_CreateExif(idx, &exif_data, &exif_jpg) != EXIF_ER_OK) {
					DBG_ERR("CreateExif fail\r\n\r\n");
					exif_data.size = 0;
				}
				if(g_jpg_buff_va[0][BufIdx]==0){
					DBG_ERR("g_jpg_buff_va 0 ! %d\r\n", BufIdx);
				}
				if(main_jpg.size >photo_cap_info[idx].filebufsize){
					DBG_ERR("jpg size over flow ,wanted=%d, but now=%d\r\n", photo_cap_info[idx].filebufsize,main_jpg.size );
				}

				memcpy((void*)(g_jpg_buff_va[0][BufIdx]+ exif_data.size), (void*)main_jpg.addr, main_jpg.size);

				main_jpg.addr=g_jpg_buff_va[0][BufIdx]+ exif_data.size;
				GxImgFile_CombineJPG(&exif_data, &main_jpg, &scr_jpg, &dst_jpg_file); // user should reserve the exif data size prior to primary data
        
        //#NT#2023/11/09#Philex Lin - begin
        // export primary bit stream/thumbnail buffer addr/size to uppper level
        JPEG_BS_INFO[0].addr =main_jpg.addr;
        JPEG_BS_INFO[0].size =main_jpg.size;
        JPEG_BS_INFO[1].addr =scr_jpg.addr;
        JPEG_BS_INFO[1].size =scr_jpg.size;
        
        JPEG_BS_INFO[2].addr =g_thumbjpg_va;
        JPEG_BS_INFO[2].size =g_thumbjpg_size;
        
        
        
				if(g_PhotoCapMsgCb){
					g_PhotoCapMsgCb(IMG_CAP_CBMSG_JPEG_OK, JPEG_BS_INFO);
        //#NT#2023/11/09#Philex Lin - end   
          //g_PhotoCapMsgCb(IMG_CAP_CBMSG_JPEG_OK, NULL);
				}
				//DBG_DUMP("dst_jpg_file.addr=0x%x, size=%d\r\n", dst_jpg_file.addr, dst_jpg_file.size);
				//write file
				if(g_PhotoCapWriteFileCb){
					vos_perf_mark(&t1);
					#if 0
					if(dst_jpg_file.size){
						writecb_rt=g_PhotoCapWriteFileCb((UINT32)dst_jpg_file.addr, dst_jpg_file.size, HD_CODEC_TYPE_JPEG, idx);

					}else{
						UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.video_pack[0].phy_addr);
						UINT32 len = data_pull.video_pack[0].size;
						writecb_rt=g_PhotoCapWriteFileCb((UINT32)ptr, len, HD_CODEC_TYPE_JPEG, idx);
					}
					#else
					IMGCAP_FILE_INFOR JPGFileInfor;
					JPGFileInfor.Addr=dst_jpg_file.addr;
					JPGFileInfor.Size=dst_jpg_file.size;
					JPGFileInfor.SensorID = idx;
					NORMAL_SET_FILE_INFOR((NORMAL_JPG_QUEUE_F() % photo_cap_info[idx].jpg_buff), JPGFileInfor);
					NORMAL_JPG_QUEUE_IN();
					vos_flag_set(g_photo_writefile_flg_id, FLGPHOTO_WRITEFILE_START);
					#endif
					vos_perf_mark(&t2);
					//DBG_DUMP("save time=%d us\r\n", (t2-t1));
				}else{
					#if (SAVE_FILE_DBG ==1)
					snprintf(file_path_main, 64, "/mnt/sd/test_cap_%lux%lu_%lu_%lu.jpg",
						photo_cap_info[idx].sCapSize.w, photo_cap_info[idx].sCapSize.h,
						flow_max_shot, save_count+1);
					DBG_DUMP("save %d/%d (%s) .... ", save_count+1, flow_max_shot, file_path_main);


					//----- open output files -----
					if ((hFile = FileSys_OpenFile(file_path_main, FST_CREATE_ALWAYS | FST_OPEN_WRITE)) == NULL) {
					//if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
						DBG_ERR("open file (%s) fail....\r\n\r\n", file_path_main);
						return;
					}
					DBG_DUMP("pack_num=%d\r\n", data_pull.pack_num);
					for (j=0; j< data_pull.pack_num; j++) {
						UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.video_pack[j].phy_addr);
						UINT32 len = data_pull.video_pack[j].size;
						//if (f_out_main) fwrite(ptr, 1, len, f_out_main);
						//if (f_out_main) fflush(f_out_main);
						if (hFile) FileSys_WriteFile(hFile, (UINT8 *)ptr, &len, 0, NULL);
					}

					// close output file
					//fclose(f_out_main);
					FileSys_CloseFile(hFile);
					#endif
				}
				//DBG_DUMP("save ok\r\n");

				//printf("enc_release ....\r\n");
				ret = hd_videoenc_release_out_buf(photo_cap_info[0].venc_p, &data_pull);
				if (ret != HD_OK) {
					DBG_ERR("enc_release error=%d !!\r\n", ret);
				}

				//save_count ++;
				//DBG_DUMP("save count = %d\r\n", save_count);
				//if ((save_count >= flow_max_shot)) {
					// let save_thread stop loop and exit
					//save_exit = 1;
				//}

				// mummap for bs buffer
				ret = hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
				if (ret != HD_OK) {
					DBG_ERR("mnumap error !!\r\n\r\n");
				}

				if (longexp.mode != AE_CAP_LONGEXP_ON) {
				hd_videocap_stop(photo_vcap_sensor_info[idx].vcap_p[0]);
				}
				hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_MAIN_PATH]);
				if(photo_cap_info[idx].screen_img==SEL_SCREEN_IMG_ON){
					hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_SCR_PATH]);
				}
				hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_QV_THUMB_PATH]);
				//hd_videoproc_stop(photo_vcap_sensor_info[idx].vproc_p_ext[VPRC_CAP_QV_THUMB_PATH]);
				hd_videoenc_stop(photo_cap_info[0].venc_p);


				//DBG_DUMP("start liveview - begin\n");
				// set videocap parameter (liveview)
				//p_stream0->cap_dim.w = VDO_SIZE_W; //assign by user
				//p_stream0->cap_dim.h = VDO_SIZE_H; //assign by user
				//ret = set_cap_param(p_stream0->cap_path, &p_stream0->cap_dim, FALSE);
				//if (ret != HD_OK) {
				//	DBG_ERR("set cap fail=%d\n", ret);
				//	return;
				//}
				// set videoproc parameter (liveview)
				//ret = set_proc_param(p_stream0->proc_path, NULL, FALSE);
				//if (ret != HD_OK) {
				//	DBG_ERR("set proc fail=%d\n", ret);
				//	return;
				//}
#if 0
				hd_videocap_bind(HD_VIDEOCAP_OUT(idx, 0), HD_VIDEOPROC_IN(0, 0));
				hd_videocap_start(photo_vcap_sensor_info[idx].vcap_p[0]);
				hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_DISPLAY_PATH]);
				_ImageApp_Photo_Disp_SetAllowPull(1);
				//DBG_DUMP("start liveview - end\n");
#endif
			}
		}
		save_count ++;
		//DBG_DUMP("save count = %d\r\n", save_count);
	}
	//if(g_PhotoCapMsgCb){
	//	g_PhotoCapMsgCb(IMG_CAP_CBMSG_FSTOK, &writecb_rt);
	//}

    if (NORMAL_JPG_QUEUE_NUM()) {
        vos_flag_wait(&uiFlag, g_photo_writefile_flg_id, FLGPHOTO_WRITEFILE_COMPLETE, TWF_ORW | TWF_CLR);
        while(NORMAL_JPG_QUEUE_NUM()) {
            vos_flag_set(g_photo_writefile_flg_id, FLGPHOTO_WRITEFILE_START);
            vos_flag_wait(&uiFlag, g_photo_writefile_flg_id, FLGPHOTO_WRITEFILE_COMPLETE, TWF_ORW | TWF_CLR);
        }
    }

	//################
	//load liveview 3A/IQ Start
	//################
	if ((ret = vendor_isp_init()) != HD_OK) {
		DBG_ERR("vendor_isp_init fail(%d)\r\n", ret);
	}

	ae_ui_operation.id = 0;
	ae_ui_operation.operation = AE_OPERATION_PHOTO;
	ret |= vendor_isp_set_ae(AET_ITEM_OPERATION, &ae_ui_operation);
	awb_ui_operation.id = 0;
	awb_ui_operation.operation = AWB_OPERATION_MOVIE;
	ret |= vendor_isp_set_awb(AWBT_ITEM_OPERATION, &awb_ui_operation);
	iq_ui_operation.id = 0;
	iq_ui_operation.operation = IQ_UI_OPERATION_MOVIE;
	ret |= vendor_isp_set_iq(IQT_ITEM_OPERATION, &iq_ui_operation);
	{
	//	AET_MANUAL manual = {0};
	//	manual.manual.lock_en = 0;
		//vendor_isp_set_ae(AET_ITEM_MANUAL, &manual);	
	}
	
	if (ret != HD_OK) {
		DBG_ERR("set 3A/IQ OPERATION_MOVIE fail!! \r\n");
	}

#if (LOAD_ISP_DTSI == 1)
#if 0 //sample for linux
	dtsi_info.id = 0;
	strncpy(dtsi_info.node_path, "/isp/iq/imx290_iq_0", DTSI_NAME_LENGTH);
	strncpy(dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
	dtsi_info.buf_addr = NULL;
	ret |= vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &dtsi_info);
	if (ret != HD_OK) {
		DBG_ERR("set capture dtsi fail!! \r\n");
	}
#endif
	if (strlen(photo_vcap_sensor_info[idx].iq_path.path) != 0) {
		//IQT_DTSI_INFO iq_dtsi_info;
		dtsi_info.id = photo_vcap_sensor_info[idx].iq_path.id;
		strncpy(dtsi_info.node_path, photo_vcap_sensor_info[idx].iq_path.path, 31);
		strncpy(dtsi_info.file_path, "null", DTSI_NAME_LENGTH);
		dtsi_info.buf_addr = (UINT8 *)photo_vcap_sensor_info[idx].iq_path.addr;
		if ((ret = vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &dtsi_info)) != HD_OK) {
			DBG_ERR("set capture dtsi fail!! \r\n");
		}
	}
#endif

	if ((ret = vendor_isp_uninit()) != HD_OK) {
		DBG_ERR("vendor_isp_uninit fail(%d)\r\n", ret);
	}

	//################
	//load liveview IQ End
	//################

	if(photo_cap_info[idx].qv_img==SEL_QV_IMG_OFF){
		for (idx= PHOTO_VID_IN_1; idx < PHOTO_VID_IN_MAX; idx++) {
			DBG_IND("CapImgCB end idx=%d, enable=%d\r\n", idx, photo_vcap_sensor_info[idx - PHOTO_VID_IN_1].enable);
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
				hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[0][g_photo_3ndr_path]);
				hd_videoproc_start(photo_vcap_sensor_info[idx].vproc_p_phy[0][IME_DISPLAY_PATH]);
			}
		}
		ImageApp_Photo_Disp_SetPreviewShow(1);
	}else{
		g_photo_is_disp_qv_restart_preview=1;
	}
	//DBG_DUMP("start liveview - end\n");
	if(g_PhotoCapMsgCb){
		g_PhotoCapMsgCb(IMG_CAP_CBMSG_RET_PRV, NULL);
		g_PhotoCapMsgCb(IMG_CAP_CBMSG_CAPEND, NULL);
	}

}
void _ImageApp_Photo_TriggerCap(void)
{
	UINT32 j=0;
	HD_VIDEOPROC_OUT vprc_out_param = {0};
	UINT32 idx=0;
	HD_RESULT ret = HD_OK;
	UINT32 pa;
	void *va;
#if 0

	HD_URECT vprc_crop_win = {0};
	UINT32 width = 0, height = 0;
	if ((ipc_sie_info->psie->SensorIn_Win.Act.SizeH < ipc_sie_info->psie->SensorIn_Win.Crop.SizeH) || (ipc_sie_info->psie->SensorIn_Win.Act.SizeV < ipc_sie_info->psie->SensorIn_Win.Crop.SizeV)) {
		DBG_ERR("id: %d, act size (%d %d) < crop Size(%d %d)\r\n", id, ipc_sie_info->psie->SensorIn_Win.Act.SizeH, ipc_sie_info->psie->SensorIn_Win.Act.SizeV, ipc_sie_info->psie->SensorIn_Win.Crop.SizeH, ipc_sie_info->psie->SensorIn_Win.Crop.SizeV);
		ipc_sie_info->psie->SensorIn_Win.Crop.StartPix.x = 0;
		ipc_sie_info->psie->SensorIn_Win.Crop.StartPix.y = 0;
		ipc_sie_info->psie->SensorIn_Win.Crop.SizeH = ipc_sie_info->psie->SensorIn_Win.Act.SizeH;
		ipc_sie_info->psie->SensorIn_Win.Crop.SizeV = ipc_sie_info->psie->SensorIn_Win.Act.SizeV;
	} else {
		ipc_sie_info->psie->SensorIn_Win.Crop.StartPix.x = (ipc_sie_info->psie->SensorIn_Win.Act.SizeH - ipc_sie_info->psie->SensorIn_Win.Crop.SizeH) >> 1;
		ipc_sie_info->psie->SensorIn_Win.Crop.StartPix.y = (ipc_sie_info->psie->SensorIn_Win.Act.SizeV - ipc_sie_info->psie->SensorIn_Win.Crop.SizeV) >> 1;
	}

	width = _ImageApp_Photo_SENCROPRATIO_ADJ_HSIZE(photo_cap_info[idx].img_ratio, ctrl_info->info->mode->ratio.ratio_h_v, width, 8);
	height = _ImageApp_Photo_SENCROPRATIO_ADJ_VSIZE(photo_cap_info[idx].img_ratio, ctrl_info->info->mode->ratio.ratio_h_v, height, 4);

	if ((photo_vcap_syscaps[idx].max_dim.h < photo_cap_info[idx].sCapSize.h) || (photo_vcap_syscaps[idx].max_dim.w < photo_cap_info[idx].sCapSize.w)) {
		DBG_ERR("id: %d, act size (%d %d) < crop Size(%d %d)\r\n", id, ipc_sie_info->psie->SensorIn_Win.Act.SizeH, ipc_sie_info->psie->SensorIn_Win.Act.SizeV, ipc_sie_info->psie->SensorIn_Win.Crop.SizeH, ipc_sie_info->psie->SensorIn_Win.Crop.SizeV);
		vprc_crop_win.x = 0;
		vprc_crop_win.y = 0;
		vprc_crop_win.w =0;
		vprc_crop_win.h = 0;
	} else {
		vprc_crop_win.x = (ipc_osie_info->psie->SensorIn_Win.Act.SizeH - ipc_sie_info->psie->SensorIn_Win.Crop.SizeH) >> 1;
		vprc_crop_win.y = (ipc_sie_info->psie->SensorIn_Win.Act.SizeV - ipc_sie_info->psie->SensorIn_Win.Crop.SizeV) >> 1;
	}
#endif


	if(g_photo_IsCapStartLinkOpened[idx]==FALSE){
		for (idx= PHOTO_VID_IN_1; idx < PHOTO_VID_IN_MAX; idx++) {
			DBG_IND("TriggerCap idx=%d, enable=%d\r\n", idx, photo_vcap_sensor_info[idx - PHOTO_VID_IN_1].enable);
			if (photo_vcap_sensor_info[idx - PHOTO_VID_IN_1].enable) {
				// main jpg
				j=VPRC_CAP_MAIN_PATH;
				//if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(1, 0), HD_VIDEOPROC_OUT(1, j), &photo_vcap_sensor_info[idx].vproc_p_phy[1][j])) != HD_OK){
				if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN((idx*PHOTO_VID_IN_MAX+1), 0), HD_VIDEOPROC_OUT((idx*PHOTO_VID_IN_MAX+1), j), &photo_vcap_sensor_info[idx].vproc_p_phy[1][j])) != HD_OK){
					DBG_ERR("[%d]VPRC_CAP_MAIN_PATH hd_videoproc_open fail(%d), out=%d\r\n",idx, ret, j);
					return ;
				}
				//HD_VIDEOPROC_OUT vprc_cfg_out = {0};
				memset(&vprc_out_param, 0, sizeof(HD_VIDEOPROC_OUT));

				vprc_out_param.func = 0;

				vprc_out_param.dim.w = photo_cap_info[idx].sCapSize.w;//photo_vcap_sensor_info[idx].sSize[j].w;//cap size, temp
				vprc_out_param.dim.h = photo_cap_info[idx].sCapSize.h;//photo_vcap_sensor_info[idx].sSize[j].h;//cap size, temp
				vprc_out_param.pxlfmt = photo_cap_info[idx].jpgfmt;//HD_VIDEO_PXLFMT_YUV420;
				vprc_out_param.dir = HD_VIDEO_DIR_NONE;
				vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
				vprc_out_param.depth = 1; //allow pull
				if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[1][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK){
					DBG_ERR("VPRC_CAP_MAIN_PATH hd_videoproc_set fail(%d)\r\n", ret);
					return ;
				}

				// set videoproc
				// screennail
				//DBG_ERR("screen_img=%d\r\n", photo_cap_info[idx].screen_img);
				if(1){//photo_cap_info[idx].screen_img==SEL_SCREEN_IMG_ON){
					j=VPRC_CAP_SCR_PATH;
					if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN((idx*PHOTO_VID_IN_MAX+1), 0), HD_VIDEOPROC_OUT((idx*PHOTO_VID_IN_MAX+1), j), &photo_vcap_sensor_info[idx].vproc_p_phy[1][j])) != HD_OK){
						DBG_ERR("VPRC_CAP_SCR_THUMB_PATH hd_videoproc_open fail(%d), out=%d\r\n", ret, j);
						return ;
					}
					if(photo_cap_info[idx].screen_img==SEL_SCREEN_IMG_ON){
						//HD_VIDEOPROC_OUT vprc_cfg_out = {0};
						memset(&vprc_out_param, 0, sizeof(vprc_out_param));
						vprc_out_param.func = 0;

						vprc_out_param.dim.w = photo_cap_info[idx].screen_img_size.w;//photo_vcap_sensor_info[idx].sSize[j].w;//cap size, temp
						vprc_out_param.dim.h = photo_cap_info[idx].screen_img_size.h;//photo_vcap_sensor_info[idx].sSize[j].h;//cap size, temp
						vprc_out_param.pxlfmt = photo_cap_info[idx].screen_fmt;//HD_VIDEO_PXLFMT_YUV420;
						vprc_out_param.dir = HD_VIDEO_DIR_NONE;
						vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
						vprc_out_param.depth = 1; //allow pull
						if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[1][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK){
							DBG_ERR("VPRC_CAP_SCR_THUMB_PATH hd_videoproc_set fail(%d)\r\n", ret);
							return ;
						}
					}
				}
				// set videoproc
				// quickview
				//DBG_ERR("qv_img=%d\r\n", photo_cap_info[idx].qv_img);
				if(1){//photo_cap_info[idx].qv_img==SEL_QV_IMG_ON){
					j=VPRC_CAP_QV_THUMB_PATH;
					if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN((idx*PHOTO_VID_IN_MAX+1), 0), HD_VIDEOPROC_OUT((idx*PHOTO_VID_IN_MAX+1), j), &photo_vcap_sensor_info[idx].vproc_p_phy[1][j])) != HD_OK){
						DBG_ERR("VPRC_CAP_QV_PATH hd_videoproc_open fail(%d), out=%d\r\n", ret, j);
						return ;
					}

					//HD_VIDEOPROC_OUT vprc_cfg_out = {0};
					memset(&vprc_out_param, 0, sizeof(vprc_out_param));
					vprc_out_param.func = 0;
					//DBG_DUMP("QV wxh=%d, %d\r\n", photo_cap_info[idx].qv_img_size.w, photo_cap_info[idx].qv_img_size.h);

					vprc_out_param.dim.w = photo_cap_info[idx].qv_img_size.w;//photo_vcap_sensor_info[idx].sSize[j].w;//cap size, temp
					vprc_out_param.dim.h = photo_cap_info[idx].qv_img_size.h;//photo_vcap_sensor_info[idx].sSize[j].h;//cap size, temp
					vprc_out_param.pxlfmt = photo_cap_info[idx].qv_img_fmt;//HD_VIDEO_PXLFMT_YUV420;
					vprc_out_param.dir = HD_VIDEO_DIR_NONE;
					vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
					vprc_out_param.depth = 1; //allow pull
					if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[1][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK){
						DBG_ERR("VPRC_CAP_QV_PATH hd_videoproc_set fail(%d)\r\n", ret);
						return ;
					}
				}
			}
		}

		idx=0;
		// venc set param
		//--- HD_VIDEOENC_PARAM_IN ---
		memset(&photo_cap_venc_in_param,0,sizeof(HD_VIDEOENC_IN));
		photo_cap_venc_in_param.dir           = HD_VIDEO_DIR_NONE;
		photo_cap_venc_in_param.pxl_fmt =  photo_cap_info[idx].jpgfmt;//HD_VIDEO_PXLFMT_YUV420;
		if(photo_cap_info[idx].screen_img==SEL_SCREEN_IMG_ON){
			photo_cap_venc_in_param.dim.w   = photo_cap_info[idx].screen_img_size.w;//p_dim->w;
			photo_cap_venc_in_param.dim.h   = photo_cap_info[idx].screen_img_size.h;//p_dim->h;
		}else{
			photo_cap_venc_in_param.dim.w   = photo_cap_info[idx].sCapSize.w;//p_dim->w;
			photo_cap_venc_in_param.dim.h   = photo_cap_info[idx].sCapSize.h;//p_dim->h;
		}
		photo_cap_venc_in_param.frc     = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoenc_set(photo_cap_info[0].venc_p, HD_VIDEOENC_PARAM_IN, &photo_cap_venc_in_param);
		if (ret != HD_OK) {
			DBG_ERR("set_enc_param_in = %d\r\n", ret);
			return;
		}

		//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
		memset(&photo_cap_venc_out_param,0,sizeof(HD_VIDEOENC_OUT2));
		photo_cap_venc_out_param.codec_type         = HD_CODEC_TYPE_JPEG;
		photo_cap_venc_out_param.jpeg.retstart_interval = 0;
#if 0//PHOTO_BRC_MODE
		photo_cap_venc_out_param.jpeg.image_quality = 0;
		photo_cap_venc_out_param.jpeg.bitrate       = bitrate;
#else
		photo_cap_venc_out_param.jpeg.image_quality = 70;
		photo_cap_venc_out_param.jpeg.bitrate       = 0;
#endif
		ret = hd_videoenc_set(photo_cap_info[0].venc_p, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &photo_cap_venc_out_param);
		if (ret != HD_OK) {
			DBG_ERR("set_enc_param_out = %d\r\n", ret);
			return;
		}
		g_photo_IsCapStartLinkOpened[idx]=TRUE;
	}else{
		for (idx= PHOTO_VID_IN_1; idx < PHOTO_VID_IN_MAX; idx++) {
			DBG_IND("TriggerCap idx=%d, enable=%d\r\n", idx, photo_vcap_sensor_info[idx - PHOTO_VID_IN_1].enable);
			if (photo_vcap_sensor_info[idx - PHOTO_VID_IN_1].enable) {
				// main jpg
				j=VPRC_CAP_MAIN_PATH;
				memset(&vprc_out_param, 0, sizeof(HD_VIDEOPROC_OUT));

				vprc_out_param.func = 0;

				vprc_out_param.dim.w = photo_cap_info[idx].sCapSize.w;//photo_vcap_sensor_info[idx].sSize[j].w;//cap size, temp
				vprc_out_param.dim.h = photo_cap_info[idx].sCapSize.h;//photo_vcap_sensor_info[idx].sSize[j].h;//cap size, temp
				vprc_out_param.pxlfmt =  photo_cap_info[idx].jpgfmt;//HD_VIDEO_PXLFMT_YUV420;
				vprc_out_param.dir = HD_VIDEO_DIR_NONE;
				vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
				vprc_out_param.depth = 1; //allow pull
				if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[1][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK){
					DBG_ERR("VPRC_CAP_MAIN_PATH hd_videoproc_set fail(%d)\r\n", ret);
					return ;
				}
				DBG_IND("Cap Size w=%dx%d\r\n",  photo_cap_info[idx].sCapSize.w,  photo_cap_info[idx].sCapSize.h);


				//add for jira NA51055-1066
				//boot and cap VGA first, then cap 12M->crash
				// set videoproc
				// screennail
				//DBG_ERR("screen_img=%d\r\n", photo_cap_info[idx].screen_img);
				if(photo_cap_info[idx].screen_img==SEL_SCREEN_IMG_ON){
					j=VPRC_CAP_SCR_PATH;
					memset(&vprc_out_param, 0, sizeof(vprc_out_param));
					vprc_out_param.func = 0;

					vprc_out_param.dim.w = photo_cap_info[idx].screen_img_size.w;//photo_vcap_sensor_info[idx].sSize[j].w;//cap size, temp
					vprc_out_param.dim.h = photo_cap_info[idx].screen_img_size.h;//photo_vcap_sensor_info[idx].sSize[j].h;//cap size, temp
					vprc_out_param.pxlfmt = photo_cap_info[idx].screen_fmt;//HD_VIDEO_PXLFMT_YUV420;
					vprc_out_param.dir = HD_VIDEO_DIR_NONE;
					vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
					vprc_out_param.depth = 1; //allow pull
					if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[1][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK){
						DBG_ERR("VPRC_CAP_SCR_PATH hd_videoproc_set fail(%d)\r\n", ret);
						return ;
					}
				}

				if(1){//photo_cap_info[idx].qv_img==SEL_QV_IMG_ON){
					j=VPRC_CAP_QV_THUMB_PATH;
					#if 1
					//HD_VIDEOPROC_OUT vprc_cfg_out = {0};
					memset(&vprc_out_param, 0, sizeof(vprc_out_param));
					vprc_out_param.func = 0;
					//DBG_DUMP("QV wxh=%d, %d\r\n", photo_cap_info[idx].qv_img_size.w, photo_cap_info[idx].qv_img_size.h);

					vprc_out_param.dim.w = photo_cap_info[idx].qv_img_size.w;//photo_vcap_sensor_info[idx].sSize[j].w;//cap size, temp
					vprc_out_param.dim.h = photo_cap_info[idx].qv_img_size.h;//photo_vcap_sensor_info[idx].sSize[j].h;//cap size, temp
					vprc_out_param.pxlfmt = photo_cap_info[idx].qv_img_fmt;//HD_VIDEO_PXLFMT_YUV420;
					vprc_out_param.dir = HD_VIDEO_DIR_NONE;
					vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
					vprc_out_param.depth = 1; //allow pull
					if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[1][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK){
						DBG_ERR("VPRC_CAP_QV_PATH hd_videoproc_set fail(%d)\r\n", ret);
						return ;
					}
					#else
					HD_VIDEOPROC_OUT_EX video_out_param_ex = {0};
					//DBG_DUMP("QV wxh=%d, %d\r\n", photo_cap_info[idx].qv_img_size.w, photo_cap_info[idx].qv_img_size.h);
					video_out_param_ex.src_path=photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_MAIN_PATH];
					video_out_param_ex.dim.w = photo_cap_info[idx].qv_img_size.w;//photo_vcap_sensor_info[idx].sSize[j].w;//cap size, temp
					video_out_param_ex.dim.h = photo_cap_info[idx].qv_img_size.h;//photo_vcap_sensor_info[idx].sSize[j].h;//cap size, temp
					video_out_param_ex.pxlfmt = photo_cap_info[idx].qv_img_fmt;//HD_VIDEO_PXLFMT_YUV420;
					video_out_param_ex.dir = HD_VIDEO_DIR_NONE;
					video_out_param_ex.frc = HD_VIDEO_FRC_RATIO(1, 1);
					video_out_param_ex.depth = 1; //allow pull
					if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_ext[j], HD_VIDEOPROC_PARAM_OUT_EX, &video_out_param_ex)) != HD_OK){
						DBG_ERR("VPRC_CAP_QV_PATH hd_videoproc_set fail(%d)\r\n", ret);
						return ;
					}
					#endif
				}
			}
		}
	}

	idx=0;
	g_jpg_buff_size=photo_cap_info[idx].filebufsize * photo_cap_info[idx].jpg_buff + photo_cap_info[idx].screen_bufsize+ (PHOTOCAP_THUMB_W*PHOTOCAP_THUMB_H)*5/2;
	DBG_IND("g_jpg_buff_size=%d, filebufsize=%d, jpg_buff=%d\r\n", g_jpg_buff_size ,photo_cap_info[idx].filebufsize,photo_cap_info[idx].jpg_buff);
	if(g_pre_jpg_buff_size !=g_jpg_buff_size ){
		//add for mem leak Start
		if ((ret = hd_videoenc_start(photo_cap_info[0].venc_p)) != HD_OK) {
			DBG_ERR("set hd_videoenc_start fail(%d), venc_p=0x%x\r\n", ret, photo_cap_info[0].venc_p);
			return;
		}
		//add for mem leak End
		if(g_pre_jpg_buff_size){
			if(g_jpg_buff_va[idx][0]){
				if ((ret = hd_common_mem_free((UINT32)g_jpg_buff_pa[idx][0], (void *)g_jpg_buff_va[idx][0])) != HD_OK) {
					DBG_ERR("hd_common_mem_free failed(%d)\r\n", ret);
				}
			}
			for (j = 0; j <JPG_BUFF_MAX; j ++) {
				g_jpg_buff_va[idx][j] = 0;
				g_jpg_buff_pa[idx][j] = 0;
			}
			g_scrjpg_pa = 0;
			g_scrjpg_va = 0;
			g_scrjpg_size = 0;
			g_thumbjpg_pa = 0;
			g_thumbjpg_va = 0;
			g_thumbjpg_size = 0;
			g_thumbyuv_pa =0;
			g_thumbyuv_va =0;
			g_thumbyuv_size = 0;
		}

		if ((ret = hd_common_mem_alloc("jpg_buff", &pa, (void **)&va, g_jpg_buff_size, DDR_ID0)) != HD_OK) {
			DBG_ERR("jpg_buff hd_common_mem_alloc failed(%d)\r\n", ret);
			for (j = 0; j <JPG_BUFF_MAX; j ++) {
				g_jpg_buff_va[idx][j] = 0;
				g_jpg_buff_pa[idx][j] = 0;
			}
			g_jpg_buff_size = 0;
		} else {
			g_jpg_buff_va[idx][0] = (UINT32)va;
			g_jpg_buff_pa[idx][0] = (UINT32)pa;
		}
		for (j = 1; j < photo_cap_info[idx].jpg_buff; j ++) {
			g_jpg_buff_va[idx][j]=g_jpg_buff_va[idx][0]+photo_cap_info[idx].filebufsize*j;
			g_jpg_buff_pa[idx][j]=g_jpg_buff_pa[idx][0]+photo_cap_info[idx].filebufsize*j;
		}

		g_scrjpg_va=g_jpg_buff_va[idx][0]+photo_cap_info[idx].filebufsize * photo_cap_info[idx].jpg_buff ;
		g_scrjpg_pa=g_jpg_buff_pa[idx][0]+photo_cap_info[idx].filebufsize * photo_cap_info[idx].jpg_buff ;

		g_thumbjpg_va=g_scrjpg_va+photo_cap_info[idx].screen_bufsize;
		g_thumbjpg_pa=g_scrjpg_pa+photo_cap_info[idx].screen_bufsize;

		g_thumbyuv_size=(PHOTOCAP_THUMB_W*PHOTOCAP_THUMB_H)*3/2;//yuv420
		g_thumbyuv_va=g_thumbjpg_va+(PHOTOCAP_THUMB_W*PHOTOCAP_THUMB_H);
		g_thumbyuv_pa=g_thumbjpg_pa+(PHOTOCAP_THUMB_W*PHOTOCAP_THUMB_H);
		g_pre_jpg_buff_size=g_jpg_buff_size;
	}

	DBG_IND("g_jpg_buff_va=0x%x, g_scrjpg_va=0x%x, g_thumbjpg_va=0x%x, g_thumbyuv_va=0x%x, 0x%x\r\n",g_jpg_buff_va[0][0],g_scrjpg_va,g_thumbjpg_va,g_thumbyuv_va,g_thumbyuv_pa);

	NORMAL_JPG_QUEUE_RESET();

	vos_flag_set(g_photo_cap_flg_id, FLGPHOTO_CAP_START);
}
THREAD_RETTYPE ImageApp_Photo_CapTsk(void)
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
	FLGPTN uiFlag=0;

	VOS_TICK t1, t2;
	THREAD_ENTRY();

	g_photo_is_cap_tsk_running[0] = TRUE;
	while (g_photo_cap_tsk_run[0]) {
		PROFILE_TASK_IDLE();
		vos_flag_wait(&uiFlag, g_photo_cap_flg_id, FLGPHOTO_CAP_START|FLGPHOTO_CAP_IDLE, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();

		if (g_PhotoCapImgCb == NULL) {
			DBG_ERR("cap proc cb is not registered.\r\n");
			g_photo_cap_tsk_run[0] = FALSE;
			break;
		}
		if (uiFlag & FLGPHOTO_CAP_IDLE) {
			g_photo_cap_tsk_run[0] = FALSE;
			break;
		}
		vos_perf_mark(&t1);
		if (uiFlag & FLGPHOTO_CAP_START) {
			g_photo_is_capcb_executing= 1;
 			g_PhotoCapImgCb();
			g_photo_is_capcb_executing= 0;
		}
		vos_perf_mark(&t2);
		if (t2 - t1 < 10000) {      // 10 ms
			//vos_task_delay_ms(100);
		}
	}
	g_photo_is_cap_tsk_running[0] = FALSE;

	THREAD_RETURN(0);

}
IMG_CAP_FST_INFO g_PhotoCapFS_Status = {FST_STA_OK};

void ImgCap_SetFS_Status(INT32 FS_Status)
{
	g_PhotoCapFS_Status.Status = FS_Status;
}

IMG_CAP_FST_INFO ImgCap_GetFS_Status(void)
{
	return g_PhotoCapFS_Status;
}

static void  _ImageApp_Photo_WriteFileTskCB(void)
{
	UINT64 FreeSpace;
	IMGCAP_FILE_INFOR Infor;
	UINT32 idx=0;
	IMG_CAP_FST_INFO FstStatus = {FST_STA_OK};
	FstStatus = ImgCap_GetFS_Status();

	if (NORMAL_JPG_QUEUE_F() <= NORMAL_JPG_QUEUE_R()) {
		DBG_ERR("[flow]jpg buf is empty(%d %d)\r\n", NORMAL_JPG_QUEUE_F(), NORMAL_JPG_QUEUE_R());
		goto NORMAL_WRITE_FILE_END;
	}

#if 0
	for (Id = IPL_ID_1; Id < IPL_ID_MAX_NUM; Id++) {
		if (ImgCap_GetUIInfo(Id, CAP_SEL_ACTFLAG)) {
			break;
		} else if (Id >= (IPL_ID_MAX_NUM - 1) && ImgCap_GetUIInfo(Id, CAP_SEL_ACTFLAG) != ENABLE) {
			DBG_ERR("[flow]WriteFile_Normal Id get false (%d)\r\n", Id);
		} else {
		}
	}
#endif


	Infor = NORMAL_GET_FILE_INFOR(NORMAL_JPG_QUEUE_R() % photo_cap_info[idx].jpg_buff);
	FreeSpace = FileSys_GetDiskInfo(FST_INFO_FREE_SPACE);
	DBG_IND("FreeSpace=%lld, Infor.Size=%d, FstStatus.Status=%d\r\n", FreeSpace, Infor.Size,FstStatus.Status);

	if ((FreeSpace > Infor.Size) && (FstStatus.Status == FST_STA_OK)) {
		DBG_IND("WriteFileTskCB Infor.Addr=0x%x, Sz=%d, SensorID=%d\r\n", Infor.Addr, Infor.Size, Infor.SensorID);

		//FstStatus.Status = ImgCap_WriteFile(Infor.Addr, Infor.Size, IMGCAP_FSYS_JPG, Infor.SensorID);
		FstStatus.Status =g_PhotoCapWriteFileCb((UINT32)Infor.Addr, Infor.Size, HD_CODEC_TYPE_JPEG, Infor.SensorID);

		DBG_IND("[flow]save file done %d\r\n", FstStatus.Status);
	} else {
		DBG_ERR("[flow]FreeSpace=%lld ,Infor.Size=%d, FstStatus=%d\r\n", FreeSpace, Infor.Size, FstStatus.Status);
	}
	ImgCap_SetFS_Status(FstStatus.Status);

	NORMAL_JPG_QUEUE_OUT();

	if(g_PhotoCapMsgCb){
		g_PhotoCapMsgCb(IMG_CAP_CBMSG_FSTOK, &FstStatus);
	}

	vos_flag_set(g_photo_writefile_flg_id, FLGPHOTO_WRITEFILE_COMPLETE);

NORMAL_WRITE_FILE_END:

	DBG_IND("NORMAL_WRITE_FILE_END\r\n");


}
THREAD_RETTYPE ImageApp_Photo_WriteFileTsk(void)
{

	FLGPTN uiFlag=0;

	VOS_TICK t1, t2;
	THREAD_ENTRY();

	g_photo_is_writefile_tsk_running[0] = TRUE;
	while (g_photo_writefile_tsk_run[0]) {
		PROFILE_TASK_IDLE();
		vos_flag_wait(&uiFlag, g_photo_writefile_flg_id, FLGPHOTO_WRITEFILE_START|FLGPHOTO_WRITEFILE_IDLE, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
		if (g_PhotoWriteFileTskCb == NULL) {
			DBG_ERR("cap proc cb is not registered.\r\n");
			g_photo_writefile_tsk_run[0] = FALSE;
			break;
		}
		if (uiFlag & FLGPHOTO_WRITEFILE_IDLE) {
			g_photo_writefile_tsk_run[0] = FALSE;
			break;
		}
		vos_perf_mark(&t1);
		if (uiFlag & FLGPHOTO_WRITEFILE_START) {
 			g_PhotoWriteFileTskCb();
		}
		vos_perf_mark(&t2);
		if (t2 - t1 < 10000) {      // 10 ms
			//vos_task_delay_ms(100);
		}
	}
	g_photo_is_writefile_tsk_running[0] = FALSE;

	THREAD_RETURN(0);

}

void _ImageApp_Photo_OpenCapLink(PHOTO_CAP_INFO *p_cap_info)
{

	//HD_DIM sz;
	UINT32 idx;

	if (p_cap_info->cap_id >= PHOTO_CAP_ID_MAX) {
		DBG_ERR("cap_id  =%d\r\n",p_cap_info->cap_id);
		return;
	}
	idx = p_cap_info->cap_id - PHOTO_CAP_ID_1;

#if 0// move to ImageApp_Photo_TriggerCap for PHOTO_CFG_CAP_INFO setting
	UINT32 j=0;
	HD_VIDEOPROC_OUT vprc_out_param = {0};
	//UINT32 idx=0;
	HD_RESULT ret = HD_OK;

	if (photo_vcap_sensor_info[idx].vproc_func & HD_VIDEOPROC_FUNC_3DNR) {
		// 3DNR
		j=VPRC_CAP_3DNR_PATH;
		if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(1, 0), HD_VIDEOPROC_OUT(1, j), &photo_vcap_sensor_info[idx].vproc_p_phy[1][j])) != HD_OK){
			DBG_ERR("VPRC_CAP_3DNR_PATH hd_videoproc_open fail(%d), out=%d\r\n", ret, j);
			return ;
		}
		//HD_VIDEOPROC_OUT vprc_cfg_out = {0};
		memset(&vprc_out_param, 0, sizeof(HD_VIDEOPROC_OUT));
		vprc_out_param.func = 0;
		vprc_out_param.dim.w =  photo_vcap_sensor_info[idx].sSize[j].w;//photo_vcap_sensor_info[idx].sSize[j].w;//cap size, temp
		vprc_out_param.dim.h =  photo_vcap_sensor_info[idx].sSize[j].h;//photo_vcap_sensor_info[idx].sSize[j].h;//cap size, temp
		vprc_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		vprc_out_param.dir = HD_VIDEO_DIR_NONE;
		vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
		vprc_out_param.depth = 1; //allow pull
		if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[1][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK){
			DBG_ERR("VPRC_CAP_3DNR_PATH hd_videoproc_set fail(%d)\r\n", ret);
			return ;
		}
	}
	// main jpg
	j=VPRC_CAP_MAIN_PATH;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(1, 0), HD_VIDEOPROC_OUT(1, j), &photo_vcap_sensor_info[idx].vproc_p_phy[1][j])) != HD_OK){
		DBG_ERR("VPRC_CAP_MAIN_PATH hd_videoproc_open fail(%d), out=%d\r\n", ret, j);
		return ;
	}
	//HD_VIDEOPROC_OUT vprc_cfg_out = {0};
	memset(&vprc_out_param, 0, sizeof(HD_VIDEOPROC_OUT));

	vprc_out_param.func = 0;

	vprc_out_param.dim.w = photo_cap_info[idx].sCapSize.w;//photo_vcap_sensor_info[idx].sSize[j].w;//cap size, temp
	vprc_out_param.dim.h = photo_cap_info[idx].sCapSize.h;//photo_vcap_sensor_info[idx].sSize[j].h;//cap size, temp
	vprc_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	vprc_out_param.dir = HD_VIDEO_DIR_NONE;
	vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
	vprc_out_param.depth = 1; //allow pull
	if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[1][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK){
		DBG_ERR("VPRC_CAP_MAIN_PATH hd_videoproc_set fail(%d)\r\n", ret);
		return ;
	}

	// set videoproc
	// screennail
	DBG_ERR("screen_img=%d\r\n", photo_cap_info[idx].screen_img);
	if(photo_cap_info[idx].screen_img==SEL_SCREEN_IMG_ON){
		j=VPRC_CAP_SCR_THUMB_PATH;
		if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(1, 0), HD_VIDEOPROC_OUT(1, j), &photo_vcap_sensor_info[idx].vproc_p_phy[1][j])) != HD_OK){
			DBG_ERR("VPRC_CAP_SCR_THUMB_PATH hd_videoproc_open fail(%d), out=%d\r\n", ret, j);
			return ;
		}

		//HD_VIDEOPROC_OUT vprc_cfg_out = {0};
		memset(&vprc_out_param, 0, sizeof(vprc_out_param));
		vprc_out_param.func = 0;

		vprc_out_param.dim.w = photo_cap_info[idx].screen_img_size.w;//photo_vcap_sensor_info[idx].sSize[j].w;//cap size, temp
		vprc_out_param.dim.h = photo_cap_info[idx].screen_img_size.h;//photo_vcap_sensor_info[idx].sSize[j].h;//cap size, temp
		vprc_out_param.pxlfmt = photo_cap_info[idx].screen_fmt;//HD_VIDEO_PXLFMT_YUV420;
		vprc_out_param.dir = HD_VIDEO_DIR_NONE;
		vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
		vprc_out_param.depth = 1; //allow pull
		if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[1][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK){
			DBG_ERR("VPRC_CAP_SCR_THUMB_PATH hd_videoproc_set fail(%d)\r\n", ret);
			return ;
		}
	}
	// set videoproc
	// quickview
	DBG_ERR("qv_img=%d\r\n", photo_cap_info[idx].qv_img);
	if(photo_cap_info[idx].qv_img==SEL_QV_IMG_ON){
		j=VPRC_CAP_QV_PATH;
		if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(1, 0), HD_VIDEOPROC_OUT(1, j), &photo_vcap_sensor_info[idx].vproc_p_phy[1][j])) != HD_OK){
			DBG_ERR("VPRC_CAP_QV_PATH hd_videoproc_open fail(%d), out=%d\r\n", ret, j);
			return ;
		}

		//HD_VIDEOPROC_OUT vprc_cfg_out = {0};
		memset(&vprc_out_param, 0, sizeof(vprc_out_param));
		vprc_out_param.func = 0;
		DBG_DUMP("QV wxh=%d, %d\r\n", photo_cap_info[idx].qv_img_size.w, photo_cap_info[idx].qv_img_size.h);

		vprc_out_param.dim.w = photo_cap_info[idx].qv_img_size.w;//photo_vcap_sensor_info[idx].sSize[j].w;//cap size, temp
		vprc_out_param.dim.h = photo_cap_info[idx].qv_img_size.h;//photo_vcap_sensor_info[idx].sSize[j].h;//cap size, temp
		vprc_out_param.pxlfmt = photo_cap_info[idx].qv_img_fmt;//HD_VIDEO_PXLFMT_YUV420;
		vprc_out_param.dir = HD_VIDEO_DIR_NONE;
		vprc_out_param.frc = HD_VIDEO_FRC_RATIO(1, 1);
		vprc_out_param.depth = 1; //allow pull
		if ((ret = hd_videoproc_set(photo_vcap_sensor_info[idx].vproc_p_phy[1][j], HD_VIDEOPROC_PARAM_OUT, &vprc_out_param)) != HD_OK){
			DBG_ERR("VPRC_CAP_QV_PATH hd_videoproc_set fail(%d)\r\n", ret);
			return ;
		}
	}



	// venc set param
	//--- HD_VIDEOENC_PARAM_IN ---
	memset(&photo_cap_venc_in_param,0,sizeof(HD_VIDEOENC_IN));
	photo_cap_venc_in_param.dir           = HD_VIDEO_DIR_NONE;
	photo_cap_venc_in_param.pxl_fmt = HD_VIDEO_PXLFMT_YUV420;
	if(photo_cap_info[idx].screen_img==SEL_SCREEN_IMG_ON){
		photo_cap_venc_in_param.dim.w   = photo_cap_info[idx].screen_img_size.w;//p_dim->w;
		photo_cap_venc_in_param.dim.h   = photo_cap_info[idx].screen_img_size.h;//p_dim->h;
	}else{
		photo_cap_venc_in_param.dim.w   = photo_cap_info[idx].sCapSize.w;//p_dim->w;
		photo_cap_venc_in_param.dim.h   = photo_cap_info[idx].sCapSize.h;//p_dim->h;
	}
	photo_cap_venc_in_param.frc     = HD_VIDEO_FRC_RATIO(1,1);
	ret = hd_videoenc_set(photo_cap_info[idx].venc_p, HD_VIDEOENC_PARAM_IN, &photo_cap_venc_in_param);
	if (ret != HD_OK) {
		DBG_ERR("set_enc_param_in = %d\r\n", ret);
		return;
	}

	//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
	memset(&photo_cap_venc_out_param,0,sizeof(HD_VIDEOENC_OUT2));
	photo_cap_venc_out_param.codec_type         = HD_CODEC_TYPE_JPEG;
	photo_cap_venc_out_param.jpeg.retstart_interval = 0;
#if 0//PHOTO_BRC_MODE
	photo_cap_venc_out_param.jpeg.image_quality = 0;
	photo_cap_venc_out_param.jpeg.bitrate       = bitrate;
#else
	photo_cap_venc_out_param.jpeg.image_quality = 70;
	photo_cap_venc_out_param.jpeg.bitrate       = 0;
#endif
	ret = hd_videoenc_set(photo_cap_info[idx].venc_p, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &photo_cap_venc_out_param);
	if (ret != HD_OK) {
		DBG_ERR("set_enc_param_out = %d\r\n", ret);
		return;
	}
#endif




	//g_photo_IsDispLinkOpened[idx] = TRUE;
	g_photo_cap_tsk_run[idx] = FALSE;
	g_photo_is_cap_tsk_running[idx] = FALSE;

	g_photo_writefile_tsk_run[idx] = FALSE;
	g_photo_is_writefile_tsk_running[idx] = FALSE;
}
ER _ImageApp_Photo_CapLinkOpen(PHOTO_CAP_INFO *p_cap_info)
{
	UINT32 idx;

	if (p_cap_info->cap_id >= PHOTO_CAP_ID_MAX) {
		DBG_ERR("disp_id  =%d\r\n",p_cap_info->cap_id);
		return E_SYS;
	}
	idx = p_cap_info->cap_id - PHOTO_CAP_ID_1;

	_ImageApp_Photo_OpenCapLink(p_cap_info);
/*
	g_photo_disp_tsk_run[idx] = TRUE;
	if ((g_photo_disp_tsk_id = vos_task_create(ImageApp_Photo_DispTsk, 0, "ImageApp_Photo_DispTsk", PRI_IMAGEAPP_PHOTO_DISP, STKSIZE_IMAGEAPP_PHOTO_DISP)) == 0) {
		DBG_ERR("photo DispTsk create failed.\r\n");
	} else {
		vos_task_resume(g_photo_disp_tsk_id);
	}
*/
	g_photo_IsCapLinkOpened[idx] = TRUE;
	NORMAL_JPG_QUEUE_RESET();
	return 0;
}
ER _ImageApp_Photo_CapLinkClose(PHOTO_CAP_INFO *p_cap_info)
{
	/*
	ImageStream_Close(&ISF_Stream[g_uiDispLinkStream]);
	*/
	UINT32 idx, delay_cnt, j;
	HD_RESULT hd_ret;

	if (p_cap_info->cap_id >= PHOTO_CAP_ID_MAX) {
		DBG_ERR("disp_id  =%d\r\n",p_cap_info->cap_id);
		return E_SYS;
	}
	idx = p_cap_info->cap_id - PHOTO_CAP_ID_1;

	if (g_photo_IsCapLinkOpened[idx] == FALSE) {
		DBG_ERR("Cap path %d is already closed.\r\n", idx);
		return E_SYS;
	}
	if (g_photo_is_cap_tsk_running[idx]) {
		//DBG_WRN("Disp task still running, force stop.\r\n");
		//ImageApp_MovieMulti_DispStop(id);

		if (g_photo_IsCapLinkOpened[idx] == FALSE) {
			DBG_ERR("CapLink not opend.\r\n");
			return E_SYS;
		}
		g_photo_cap_tsk_run[idx] = FALSE;       // set this flag and task will return by itself, no need to destroy
		vos_flag_set(g_photo_cap_flg_id, FLGPHOTO_CAP_IDLE);

		delay_cnt = 50;
		while (g_photo_is_cap_tsk_running[idx] && delay_cnt) {
			vos_util_delay_ms(10);
			delay_cnt --;
		}
		if (g_photo_is_cap_tsk_running[idx] == TRUE) {
			DBG_WRN("Destroy CapTsk\r\n");
			vos_task_destroy(g_photo_cap_tsk_id);
		}

	}

	if (vos_flag_destroy(g_photo_cap_flg_id) != E_OK) {
		DBG_ERR("photo_cap_flg_id destroy failed.\r\n");
	}


	if (g_photo_is_writefile_tsk_running[idx]) {
		//DBG_WRN("Disp task still running, force stop.\r\n");
		//ImageApp_MovieMulti_DispStop(id);

		if (g_photo_IsCapLinkOpened[idx] == FALSE) {
			DBG_ERR("CapLink not opend.\r\n");
			return E_SYS;
		}
		g_photo_writefile_tsk_run[idx] = FALSE;       // set this flag and task will return by itself, no need to destroy
		vos_flag_set(g_photo_writefile_flg_id, FLGPHOTO_WRITEFILE_IDLE);

		delay_cnt = 50;
		while (g_photo_is_writefile_tsk_running[idx] && delay_cnt) {
			vos_util_delay_ms(10);
			delay_cnt --;
		}
		if (g_photo_is_writefile_tsk_running[idx] == TRUE) {
			DBG_WRN("Destroy Writefile Tsk\r\n");
			vos_task_destroy(g_photo_writefile_tsk_id);
		}

	}

	if (vos_flag_destroy(g_photo_writefile_flg_id) != E_OK) {
		DBG_ERR("photo_cap_flg_id destroy failed.\r\n");
	}

	#if 0
	if(g_exifthumb_scr_va){
		if ((hd_ret = hd_common_mem_free(g_exifthumb_scr_pa, (void *)g_exifthumb_scr_va)) != HD_OK) {
			DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
		}
	}

	g_exifthumb_scr_pa=0;
	g_exifthumb_scr_va=0;
	g_exifthumb_scr_size=0;
	g_scrjpg_pa = 0;
	g_scrjpg_va = 0;
	g_scrjpg_size = 0;
	g_thumbjpg_pa = 0;
	g_thumbjpg_va = 0;
	g_thumbjpg_size = 0;
	#endif

	if(g_jpg_buff_va[idx][0]){
		if ((hd_ret = hd_common_mem_free(g_jpg_buff_pa[idx][0], (void *)g_jpg_buff_va[idx][0])) != HD_OK) {
			DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
		}
	}
	for (j = 0; j <JPG_BUFF_MAX; j ++) {
		g_jpg_buff_va[idx][j] = 0;
		g_jpg_buff_pa[idx][j] = 0;
	}
	g_jpg_buff_size = 0;
	g_pre_jpg_buff_size = 0;
	g_scrjpg_pa = 0;
	g_scrjpg_va = 0;
	g_scrjpg_size = 0;
	g_thumbjpg_pa = 0;
	g_thumbjpg_va = 0;
	g_thumbjpg_size = 0;
	g_thumbyuv_pa =0;
	g_thumbyuv_va =0;
	g_thumbyuv_size = 0;

	//cap
	if(g_photo_IsCapStartLinkOpened[idx]){
		if ((hd_ret = hd_videoproc_close(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_MAIN_PATH])) != HD_OK) {
			DBG_ERR("MAIN_PATH hd_videoproc_close fail(%d)\r\n", hd_ret);
		}

		if ((hd_ret = hd_videoproc_close(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_SCR_PATH])) != HD_OK) {
		//if ((hd_ret = hd_videoproc_close(photo_vcap_sensor_info[idx].vproc_p_ext[VPRC_CAP_SCR_PATH])) != HD_OK) {
			DBG_ERR("SCR_THUMB_PATH hd_videoproc_close fail(%d)\r\n", hd_ret);
		}
		if ((hd_ret = hd_videoproc_close(photo_vcap_sensor_info[idx].vproc_p_phy[1][VPRC_CAP_QV_THUMB_PATH])) != HD_OK) {
			DBG_ERR("QV_PATH hd_videoproc_close fail(%d)\r\n", hd_ret);
		}

	}
	if ((hd_ret = hd_videoenc_close(photo_cap_info[0].venc_p)) != HD_OK) {
		DBG_ERR("set hd_videoenc_close fail(%d)\r\n", hd_ret);
	}

	g_photo_IsCapStartLinkOpened[idx]=FALSE;

	return 0;
}
ER _ImageApp_Photo_CapLinkStart(PHOTO_CAP_INFO *p_cap_info)
{
	UINT32 idx;
	T_CFLG cflg ;
	HD_RESULT hd_ret=HD_OK;
	VENDOR_VIDEOENC_BS_RESERVED_SIZE_CFG bs_reserved_size = {0};

	memset(&cflg, 0, sizeof(T_CFLG));

	idx = p_cap_info->cap_id;
	if(g_photo_cap_tsk_run[idx]  == TRUE){
		return 0;
	}

	g_photo_cap_tsk_run[idx] = TRUE;
	g_photo_writefile_tsk_run[idx] = TRUE;

	if ((hd_ret |= vos_flag_create(&g_photo_cap_flg_id, &cflg, "photo_cap_flg")) != E_OK) {
		DBG_ERR("photo_cap_flg_id fail\r\n");
	}

	if ((hd_ret |= vos_flag_create(&g_photo_writefile_flg_id, &cflg, "photo_writefile_flg")) != E_OK) {
		DBG_ERR("photo_writefile_flg_id fail\r\n");
	}

	g_PhotoCapImgCb = (PHOTO_PROCESS_CB) &_ImageApp_Photo_CapImgCB;
	g_PhotoWriteFileTskCb=(PHOTO_PROCESS_CB) &_ImageApp_Photo_WriteFileTskCB;
	g_PhotoCapQvCb = (PHOTO_PROCESS_PARAM_CB) &_ImageApp_Photo_CapQvCB;
	g_PhotoCapScrCb = (PHOTO_PROCESS_PARAM_CB) &_ImageApp_Photo_CapScrCB;
	g_PhotoCapWriteFileCb=(PHOTO_CAP_FSYS_FP) ImageApp_Photo_WriteCB;

	//set verdor cmd for venc reserve 64KB in the front of output jpg
	bs_reserved_size.reserved_size = 65536;
	if ((hd_ret = vendor_videoenc_set(photo_cap_info[0].venc_p, VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE, &bs_reserved_size)) != HD_OK) {
		DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE) fail(%d), venc_p=0x%x\n", hd_ret, photo_cap_info[0].venc_p);
	}

	if ((g_photo_cap_tsk_id = vos_task_create(ImageApp_Photo_CapTsk, 0, "IAPhotoCapTsk", PRI_IMAGEAPP_PHOTO_CAP, STKSIZE_IMAGEAPP_PHOTO_CAP)) == 0) {
		DBG_ERR("photo CapTsk create failed.\r\n");
	} else {
		vos_task_resume(g_photo_cap_tsk_id);
	}

	if ((g_photo_writefile_tsk_id = vos_task_create(ImageApp_Photo_WriteFileTsk, 0, "IAPhotoWFileTsk", PRI_IMAGEAPP_PHOTO_WRITEFILE, STKSIZE_IMAGEAPP_PHOTO_WRITEFILE)) == 0) {
		DBG_ERR("photo WriteFileTsk create failed.\r\n");
	} else {
		vos_task_resume(g_photo_writefile_tsk_id);
	}

	return 0;
}

void _ImageApp_Photo_CapSetAspectRatio(UINT32 Path, UINT32 w, UINT32 h)
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
}
void _ImageApp_Photo_CapSetVdoImgSize(UINT32 Path, UINT32 w, UINT32 h)
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
void ImageApp_Photo_CapGetVdoImgSize(UINT32 Path, UINT32* w, UINT32* h)
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
}
PHOTO_CAP_INFO *ImageApp_Photo_GetCapConfig(UINT32 idx)
{
	return &photo_cap_info[idx];
}
void ImageApp_Photo_CapConfig(UINT32 config_id, UINT32 value)
{

    	switch (config_id) {
	case PHOTO_CFG_CAP_MSG_CB: {
			if (value != 0) {
				g_PhotoCapMsgCb = (PHOTO_CAP_CBMSG_FP) value;
			}
		}
		break;
	case PHOTO_CFG_CAP_DETS2_CB: {
			if (value != 0) {
				g_PhotoCapDetS2Cb = (PHOTO_CAP_S2DET_FP) value;
			}
		}
		break;
	default:
		DBG_ERR("Unknown config_id=%d\r\n", config_id);
		break;
    	}

}

