#include "PlaybackTsk.h"
#include "UIAppPlay.h"
#include "PrjInc.h"
#include "NVTUserCommand.h"
#include "FileDB.h"
#include "PrjCfg.h"
#include "PBXFileList/PBXFileList_DCF.h"
#include "PBXFileList/PBXFileList_FileDB.h"
#include "ImageApp/ImageApp_Play.h"
#include "SysMain.h"
#include "UIApp/AppDisp_PBView.h"
#include "sys_mempool.h"
#include "PlaybackTsk.h"
#include "UIApp/ExifVendor.h"
#include "UIApp/Network/UIAppWiFiCmd.h"
#include "GxStrg.h"
#include "GxVideoFile.h"

#define PB_FILE_FMT             PBFMT_JPG | PBFMT_WAV | PBFMT_AVI | PBFMT_MOVMJPG | PBFMT_MP4 | PBFMT_TS
#define RTSP_PLAY_FUNC          DISABLE

#if 0
PLAY_DISP_INFO  UIAppPlayDispConfig[4] = {
    //enable  disp_id       w     h      ratio     rotate dir
	{TRUE, PLAY_DISP_ID_1, 960,  240  ,  4 ,  3  ,  0},
	{TRUE, PLAY_DISP_ID_2, 960,  240  ,  4 ,  3  ,  0},
	{TRUE, PLAY_DISP_ID_3, 960,  240  ,  4 ,  3  ,  0},
	{TRUE, PLAY_DISP_ID_4, 960,  240  ,  4 ,  3  ,  0},
};
#else
IMAGEAPP_PLAY_CFG_DISP_INFO gPlay_Disp_Info;
#endif

static HD_COMMON_MEM_INIT_CONFIG g_play_mem_cfg = {0};
static UINT32 g_PlayExifBufPa = 0, g_PlayExifBufVa = 0;

static ER PlayExe_InitExif(void)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	UINT32 pa, blk_size;
	void *va;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	MEM_RANGE buf;

	EXIF_InstallID();

	blk_size = 64 * 1024 * 1;

	if(g_PlayExifBufVa == 0)  {
		if ((hd_ret = hd_common_mem_alloc("exifbuf", &pa, (void **)&va, blk_size, ddr_id)) != HD_OK) {
			DBG_ERR("hd_common_mem_alloc failed(%d)\r\n", hd_ret);
			return E_NOMEM;
		}
		g_PlayExifBufVa = (UINT32)va;
		g_PlayExifBufPa = (UINT32)pa;

		buf.addr = g_PlayExifBufVa;
		buf.size = 0x10000;
		EXIF_Init(EXIF_HDL_ID_1, &buf, ExifCB);
		ret = E_OK;
	}
	return ret;
}

static ER PlayExe_UninitExif(void)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;

	if ((hd_ret = hd_common_mem_free(g_PlayExifBufPa, (void *)g_PlayExifBufVa)) != HD_OK) {
		DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
		ret = E_SYS;
	}
	g_PlayExifBufPa = 0;
	g_PlayExifBufVa = 0;

	EXIF_UninstallID();

	return ret;
}


static INT32 PlayExe_InitCommonMem(void)
{
	HD_VIDEOOUT_SYSCAPS  video_out_syscaps;
	HD_VIDEOOUT_SYSCAPS *p_video_out_syscaps = &video_out_syscaps;
	HD_PATH_ID video_out_ctrl = (HD_PATH_ID)GxVideo_GetDeviceCtrl(DOUT1, DISPLAY_DEVCTRL_CTRLPATH);
    USIZE device_size  = {0};
    HD_RESULT ret = HD_OK;
    UINT32 i = 0;

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, p_video_out_syscaps);
	if (ret != HD_OK) {
		DBG_ERR("get video_out_syscaps failed\r\n");
        device_size.w = 960;
        device_size.h = 240;
	} else {
        device_size.w = p_video_out_syscaps->output_dim.w;
        device_size.h = p_video_out_syscaps->output_dim.h;
	}

	// config common pool (Decode)
	g_play_mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	g_play_mem_cfg.pool_info[i].blk_size = ALIGN_CEIL_64(PB_MAX_RAW_SIZE) + 1024;   //0x1000 for address page alignment
	g_play_mem_cfg.pool_info[i].blk_cnt = 1;
	g_play_mem_cfg.pool_info[i].ddr_id = DDR_ID0;
	i++;

	// config common pool (Display for playback lib)
	g_play_mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
#if PLAY_THUMB_AND_MOVIE // fixed common pool size to PB_MAX_VIDEO for playing thumbnail and movie together
	g_play_mem_cfg.pool_info[i].blk_size = ALIGN_CEIL_64(PB_MAX_VIDEO_W)*ALIGN_CEIL_64(PB_MAX_VIDEO_H)*3/2 + 1024;	//Three YUV420 buffers and ALIGN_CEIL_64 for H.264/H.265
#else
	g_play_mem_cfg.pool_info[i].blk_size = ALIGN_CEIL_64(1920)*ALIGN_CEIL_64(1080)*3/2 + 1024;  //Three YUV420 buffers and ALIGN_CEIL_64 for H.264/H.265
#endif
	g_play_mem_cfg.pool_info[i].blk_cnt = 4; //consider the playback library
	g_play_mem_cfg.pool_info[i].ddr_id = DDR_ID0;
	i++;

	// config common pool (Display for Project)
	g_play_mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	g_play_mem_cfg.pool_info[i].blk_size = ALIGN_CEIL_64(device_size.w) *ALIGN_CEIL_64(device_size.h)*3/2 + 1024;
	g_play_mem_cfg.pool_info[i].blk_cnt = 6; //consider the playback library
	g_play_mem_cfg.pool_info[i].ddr_id = DDR_ID0;
	i++;

	// config common pool for bs pushing in (file buffer)
	g_play_mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	g_play_mem_cfg.pool_info[i].blk_size = PB_MAX_FILE_SIZE + 1024;
	g_play_mem_cfg.pool_info[i].blk_cnt = 2;
	g_play_mem_cfg.pool_info[i].ddr_id = DDR_ID0;
	i++;

	// config common pool for exif parasing
	g_play_mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	g_play_mem_cfg.pool_info[i].blk_size = MAX_APP1_SIZE + 1024;
	g_play_mem_cfg.pool_info[i].blk_cnt = 1;
	g_play_mem_cfg.pool_info[i].ddr_id = DDR_ID0;
	i++;

#if PLAY_DEWARP == ENABLE
	g_play_mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	g_play_mem_cfg.pool_info[i].blk_size = ALIGN_CEIL_64(PB_MAX_DECODE_W)*ALIGN_CEIL_64(PB_MAX_DECODE_W)*3/2 + 1024;
	g_play_mem_cfg.pool_info[i].blk_cnt = 2; //consider the playback library
	g_play_mem_cfg.pool_info[i].ddr_id = DDR_ID0;
	i++;
#endif

    ImageApp_Play_SetParam(PLAY_PARAM_MEM_CFG, (UINT32)&g_play_mem_cfg);

	return 0;
}

static void PlayExe_SetDispParam(void)
{
#if 0
	PLAY_DISP_INFO*  p_disp = NULL;
	USIZE DeviceRatio;

	if (System_GetEnableDisp() & DISPLAY_1) {

		DeviceRatio = GxVideo_GetDeviceAspect(0);

		p_disp = &UIAppPlayDispConfig[0];
		p_disp->enable = TRUE;
		p_disp->width  =  0;
		p_disp->height =  0;
		p_disp->width_ratio  =  DeviceRatio.w;
		p_disp->height_ratio =  DeviceRatio.h;
		p_disp->rotate_dir   =  SysVideo_GetDirbyID(DOUT1);
		ImageApp_Play_Config(PLAY_CONFIG_DISP_INFO, (UINT32)p_disp);
	}
	if (System_GetEnableDisp() & DISPLAY_2) {

		DeviceRatio = GxVideo_GetDeviceAspect(1);

		p_disp = &UIAppPlayDispConfig[1];
		p_disp->enable = TRUE;
		p_disp->width  =  0;
		p_disp->height =  0;
		p_disp->width_ratio  =  DeviceRatio.w;
		p_disp->height_ratio =  DeviceRatio.h;
		p_disp->rotate_dir   =  SysVideo_GetDirbyID(DOUT2);
		ImageApp_Play_Config(PLAY_CONFIG_DISP_INFO, (UINT32)p_disp);
	}
#else
	gPlay_Disp_Info.vout_ctrl = GxVideo_GetDeviceCtrl(DOUT1,DISPLAY_DEVCTRL_CTRLPATH);
	gPlay_Disp_Info.vout_path = GxVideo_GetDeviceCtrl(DOUT1,DISPLAY_DEVCTRL_PATH);
	ImageApp_Play_SetParam(PLAY_PARAM_DISP_INFO, (UINT32)&gPlay_Disp_Info);
#endif
}

static void PlayExe_SetPBParam(void)
{
    USIZE video_size   = {0};
	ISIZE device_size  = {0};
#if 0
	USIZE device2_size = {0};
	ISIZE tmp_size 	   = {0};

	if (System_GetEnableDisp() & DISPLAY_1) {
		tmp_size = GxVideo_GetDeviceSize(0);
		device_size.h = (UINT32)tmp_size.h;
		device_size.w = (UINT32)tmp_size.w;
	}
	if (System_GetEnableDisp() & DISPLAY_2) {
		tmp_size = GxVideo_GetDeviceSize(1);
		device2_size.h = (UINT32)tmp_size.h;
		device2_size.w = (UINT32)tmp_size.w;
	}

	if (device2_size.h > device_size.h && device2_size.w > device_size.w) {
		device_size.h = device2_size.h;
		device_size.w = device2_size.w;
	}
#endif

	device_size = GxVideo_GetDeviceSize(0);

	ImageApp_Play_SetParam(PLAY_PARAM_PANELSZ, (UINT32)&device_size);
	ImageApp_Play_SetParam(PLAY_PARAM_PLAY_FMT, PB_FILE_FMT);
	ImageApp_Play_SetParam(PLAY_PARAM_MAX_FILE_SIZE, PB_MAX_FILE_SIZE);
	ImageApp_Play_SetParam(PLAY_PARAM_MAX_DECODE_WIDTH, PB_MAX_DECODE_W);
	ImageApp_Play_SetParam(PLAY_PARAM_MAX_DECODE_HEIGHT, PB_MAX_DECODE_H);
	ImageApp_Play_SetParam(PLAY_PARAM_PIXEL_FORMAT, (UINT32)PB_PIXEL_FORMAT);

	video_size.w = PB_MAX_VIDEO_W;
	video_size.h = PB_MAX_VIDEO_H;
    ImageApp_Play_SetParam(PLAY_PARAM_MAX_VIDEO_SIZE, (UINT32)&video_size);

}

BOOL PBDecVideoCB(UINT32 uiUserDataAddr, UINT32 uiUserDataSize)
{
	//return MovieUdta_ParseVendorUserData(uiUserDataAddr, uiUserDataSize);
	return 0;
}

#if PLAY_DEWARP == ENABLE

#include "vendor_videoprocess.h"
#include "vendor_vpe.h"

static HD_RESULT set_vprc_dev_config(
		const HD_PATH_ID ctrl_path_id,
		const HD_DIM dim,
		const HD_VIDEOPROC_PIPE pipe,
		const UINT32 isp_id
);

static HD_RESULT set_vprc_out(
		const HD_DIM dim,
		const HD_PATH_ID path_id);

static HD_RESULT set_vprc_in(
		const HD_DIM dim,
		const HD_PATH_ID path_id);

typedef struct {
	HD_PATH_ID vprc_path_id;
	HD_PATH_ID vprc_ctrl_path_id;
	HD_PATH_ID vpe_path_id;
	HD_PATH_ID vpe_ctrl_path_id;
	HD_DIM dim;
	UINT32 vpe_isp_id; /* vprc: don't care */
	VPE_DCTG_PARAM dctg;
	BOOL is_init;
	PBVIEW_DEWARP_CB cb;
} UIAppPlay_DewarpParam;

static UIAppPlay_DewarpParam dewarp_param = {0};

static HD_RESULT get_dctg_param(const UINT32 id, VPE_DCTG_PARAM* param)
{
	VPET_DCTG_PARAM dctg_param = {0};
	HD_RESULT ret;

	if(NULL == param){
		DBG_ERR("param can't be null!\n");
		return E_SYS;
	}

	dctg_param.id = id;
	ret = vendor_vpe_get_cmd(VPET_ITEM_DCTG_PARAM, &dctg_param);
	if(ret != HD_OK){
		DBG_ERR("vendor_vpe_set_cmd VPET_ITEM_DCTG_PARAM failed!(%d)\n", ret);
		return ret;
	}

	*param = dctg_param.dctg;

	return ret;
}

static HD_RESULT set_dctg_param(const UINT32 id, const VPE_DCTG_PARAM* param)
{
	VPET_DCTG_PARAM dctg = {0};
	VPET_DCE_CTL_PARAM dce_ctl = {0};
	HD_RESULT ret;

	if(NULL == param){
		DBG_ERR("param can't be null!\n");
		return E_SYS;
	}

	dce_ctl.id = id;
	dce_ctl.dce_ctl.enable = 1;
	dce_ctl.dce_ctl.dce_mode = 2; // 0:GDC, 1:2DLUT
	dce_ctl.dce_ctl.lens_radius = 0;
	ret = vendor_vpe_set_cmd(VPET_ITEM_DCE_CTL_PARAM, &dce_ctl);
	if(ret != HD_OK){
		DBG_ERR("vendor_vpe_set_cmd VPET_ITEM_DCE_CTL_PARAM failed!(%d)\n", ret);
		return ret;
	}

	dctg.id = id;
	dctg.dctg = *param;
	ret = vendor_vpe_set_cmd(VPET_ITEM_DCTG_PARAM, &dctg);
	if(ret != HD_OK){
		DBG_ERR("vendor_vpe_set_cmd VPET_ITEM_DCTG_PARAM failed!(%d)\n", ret);
		return ret;
	}

	return ret;
}

static HD_RESULT open_vprc(
		const HD_DIM dim,
		const UINT32 dev_id,
		const HD_VIDEOPROC_PIPE pipe,
		const UINT32 isp_id,
		HD_PATH_ID* path_id,
		HD_PATH_ID* ctrl_path_id)
{
	HD_RESULT ret = HD_OK;

	ret = hd_videoproc_open(0, HD_VIDEOPROC_CTRL(dev_id), ctrl_path_id); //open this for device control
	if (ret != HD_OK){
		DBG_ERR("hd_videoproc_open failed(%d)\n", ret);
		return ret;
	}

	ret = set_vprc_dev_config(*ctrl_path_id, dim, pipe, isp_id);
	if (ret != HD_OK){
		return ret;
	}

	ret = hd_videoproc_open(HD_VIDEOPROC_IN(dev_id, 0), HD_VIDEOPROC_OUT(dev_id, 0), path_id);
	if (ret != HD_OK) {
		DBG_ERR("hd_videodec_open failed!(%d)\n", ret);
		return ret;
	}

	return ret;
}

static HD_RESULT close_vprc(
		const HD_PATH_ID path_id,
		const HD_PATH_ID ctrl_path_id)
{
	HD_RESULT ret = HD_OK;

	ret = hd_videoproc_stop(path_id);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoproc_stop failed!(ret = %d, path_id = %lx)\n", ret, path_id);
		return ret;
	}

	ret = hd_videoproc_close(path_id);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoproc_close failed!(%d)\n", ret);
		return ret;
	}

	ret = hd_videoproc_close(ctrl_path_id); //open this for device control
	if (ret != HD_OK){
		DBG_ERR("hd_videoproc_close failed(%d)\n", ret);
		return ret;
	}

	return ret;
}

static HD_RESULT dewarp_release_out_buf_cb(HD_VIDEO_FRAME *frame)
{
	HD_RESULT ret = HD_OK;

	ret = hd_videoproc_release_out_buf(dewarp_param.vpe_path_id, frame);
	if (ret != HD_OK) {
		DBG_ERR("hd_videodec_release_out_buf failed(%d)!!\r\n", ret);
	}

	return ret;
}

static HD_RESULT dewarp_pull_out_buf_cb(HD_VIDEO_FRAME *out_frame)
{
	HD_RESULT ret = HD_OK;

	ret = hd_videoproc_pull_out_buf(dewarp_param.vpe_path_id, out_frame, -1);
	if(ret != HD_OK){
		DBG_ERR("hd_videoproc_pull_out_buf failed(%d)\n", ret);
	}

	return ret;
}

static HD_RESULT dewarp_push_in_buf_cb(HD_VIDEO_FRAME *in_frame, HD_VIDEO_FRAME *out_frame)
{
	HD_RESULT ret;

	if((in_frame->dim.w != dewarp_param.dim.w) || (in_frame->dim.h != dewarp_param.dim.h)){

		ret = hd_videoproc_stop(dewarp_param.vpe_path_id);
		if (ret != HD_OK) {
			DBG_ERR("hd_videoproc_stop failed!(ret = %d, path_id = %lx)\n", ret, dewarp_param.vpe_path_id);
			goto EXIT;
		}

		ret = hd_videoproc_stop(dewarp_param.vprc_path_id);
		if (ret != HD_OK) {
			DBG_ERR("hd_videoproc_stop failed!(ret = %d, path_id = %lx)\n", ret, dewarp_param.vprc_path_id);
			goto EXIT;
		}

		ret = set_vprc_out(in_frame->dim, dewarp_param.vpe_path_id);
		if(ret != HD_OK){
			goto EXIT;
		}

		ret = set_vprc_in(in_frame->dim, dewarp_param.vpe_path_id);
		if(ret != HD_OK){
			goto EXIT;
		}

		ret = set_vprc_out(in_frame->dim, dewarp_param.vprc_path_id);
		if(ret != HD_OK){
			goto EXIT;
		}

		ret = set_vprc_in(in_frame->dim, dewarp_param.vprc_path_id);
		if(ret != HD_OK){
			goto EXIT;
		}

		ret = hd_videoproc_start(dewarp_param.vprc_path_id);
		if (ret != HD_OK) {
			DBG_ERR("hd_videoproc_start failed!(ret = %d, path_id = %lx)\n", ret, dewarp_param.vprc_path_id);
			goto EXIT;
		}

		ret = hd_videoproc_start(dewarp_param.vpe_path_id);
		if (ret != HD_OK) {
			DBG_ERR("hd_videoproc_start failed!(ret = %d, path_id = %lx)\n", ret, dewarp_param.vpe_path_id);
			goto EXIT;
		}

		dewarp_param.dim = in_frame->dim;
	}

	ret = hd_videoproc_push_in_buf(dewarp_param.vprc_path_id, in_frame, NULL, -1);
	if(ret != HD_OK){
		DBG_ERR("hd_videoproc_push_in_buf failed(%d)\n", ret);
		goto EXIT;
	}

EXIT:
	return ret;
}

static HD_RESULT set_vprc_dev_config(
		const HD_PATH_ID ctrl_path_id,
		const HD_DIM dim,
		const HD_VIDEOPROC_PIPE pipe,
		const UINT32 isp_id
)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG param = {0};

	param.pipe = pipe;
	param.isp_id = isp_id;
	param.ctrl_max.func = 0;
	param.in_max.func = 0;
	param.in_max.dim = dim;
	param.in_max.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	param.in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
	ret = hd_videoproc_set(ctrl_path_id, HD_VIDEOPROC_PARAM_DEV_CONFIG, &param);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoproc_set HD_VIDEOPROC_PARAM_DEV_CONFIG!(%d)\n", ret);
		return ret;
	}

	return ret;
}

static HD_RESULT set_vprc_out(
		const HD_DIM dim,
		const HD_PATH_ID path_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_OUT param = {0};

	DBG_IND("set_vprc_out dim = {%lu, %lu}\n", dim.w, dim.h);

	param.func = 0;
	param.dim = dim;
	param.bg = dim;
	param.rect = (HD_URECT){0, 0, dim.w, dim.h};
	param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	param.dir = HD_VIDEO_DIR_NONE;
	param.frc = HD_VIDEO_FRC_RATIO(1,1);
	param.depth = 1;

	ret = hd_videoproc_set(path_id, HD_VIDEOPROC_PARAM_OUT, &param);
	if(ret != HD_OK)
		DBG_ERR("hd_videoproc_set HD_VIDEOPROC_PARAM_OUT failed(%d)\n", ret);

	return ret;
}

static HD_RESULT set_vprc_in(
		const HD_DIM dim,
		const HD_PATH_ID path_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_IN param = {0};

	DBG_IND("set_vprc_in dim = {%lu, %lu}\n", dim.w, dim.h);

	param.dim = dim;
	param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	param.func = 0;
	param.dir = HD_VIDEO_DIR_NONE;
	param.frc = HD_VIDEO_FRC_RATIO(1,1);

	ret = hd_videoproc_set(path_id, HD_VIDEOPROC_PARAM_IN, &param);
	if(ret != HD_OK)
		DBG_ERR("hd_videoproc_set HD_VIDEOPROC_PARAM_OUT failed(%d)\n", ret);

	return ret;
}

#define VPRC_IPP_DEV_ID		0
#define VPRC_VPE_DEV_ID		1
#define VPRC_VPE_ISP_ID		0

INT32 UIAppPlay_Dewarp_Init(
		const HD_DIM max_dim,
		const VPE_DCTG_PARAM* dctg_default
)
{
	HD_RESULT ret = E_OK;

	if(dewarp_param.is_init == TRUE)
		return E_OK;

	if(dctg_default == NULL){
		dewarp_param.dctg = (VPE_DCTG_PARAM){
				.mount_type = 0,
				.lut2d_sz = 3,
				.lens_r = 540,
				.lens_x_st = 420,
				.lens_y_st = 0,
				.theta_st = 51471,
				.theta_ed = 102943,
				.phi_st = -51472,
				.phi_ed = 51471,
				.rot_y = 0,
				.rot_z = -154414
		//		.rot_z = -154414 + 102943 * id
		};
	}
	else{
		dewarp_param.dctg = *dctg_default;
	}

	ret = vendor_vpe_init();
	if (ret != HD_OK) {
		DBG_ERR("vendor_vpe_init failed!(%d)\n", ret);
		goto EXIT;
	}

	ret = hd_videoproc_init();
    if (ret != HD_OK) {
    	DBG_ERR("hd_videoproc_init fail(%d)\r\n", ret);
    	goto EXIT;
    }

    dewarp_param.dim = max_dim;
    DBG_IND("dewarp_init dim = {%lu, %lu}\n", dewarp_param.dim.w, dewarp_param.dim.h);

	/* HD_VIDEOPROC_PIPE_SCALE */
    ret = open_vprc(
    		max_dim,
			VPRC_IPP_DEV_ID,
			HD_VIDEOPROC_PIPE_SCALE,
			HD_ISP_DONT_CARE,
			&dewarp_param.vprc_path_id,
			&dewarp_param.vprc_ctrl_path_id
			);

	if(ret != HD_OK){
		goto EXIT;
	}

	ret = set_vprc_out(max_dim, dewarp_param.vprc_path_id);
	if(ret != HD_OK){
		goto EXIT;
	}

	ret = set_vprc_in(max_dim, dewarp_param.vprc_path_id);
	if(ret != HD_OK){
		goto EXIT;
	}

	/* HD_VIDEOPROC_PIPE_VPE */
    ret = open_vprc(
    		max_dim,
			VPRC_VPE_DEV_ID,
			HD_VIDEOPROC_PIPE_VPE,
			VPRC_VPE_ISP_ID,
			&dewarp_param.vpe_path_id,
			&dewarp_param.vpe_ctrl_path_id
			);

	if(ret != HD_OK){
		goto EXIT;
	}

	ret = set_vprc_out(max_dim, dewarp_param.vpe_path_id);
	if(ret != HD_OK){
		goto EXIT;
	}

	ret = set_vprc_in(max_dim, dewarp_param.vpe_path_id);
	if(ret != HD_OK){
		goto EXIT;
	}

	ret = set_dctg_param(VPRC_VPE_ISP_ID, &dewarp_param.dctg);
	if(ret != HD_OK){
		goto EXIT;
	}

	ret = hd_videoproc_bind(HD_VIDEOPROC_OUT(VPRC_IPP_DEV_ID, 0), HD_VIDEOPROC_IN(VPRC_VPE_DEV_ID, 0));
	if(ret != HD_OK){
		goto EXIT;
	}

	dewarp_param.cb.push_in_buf = dewarp_push_in_buf_cb;
	dewarp_param.cb.pull_out_buf = dewarp_pull_out_buf_cb;
	dewarp_param.cb.release_out_buf = dewarp_release_out_buf_cb;

	PBView_SetDewarpCB(dewarp_param.cb);

	ret = hd_videoproc_start(dewarp_param.vprc_path_id);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoproc_start failed!(ret = %d, path_id = %lx)\n", ret, dewarp_param.vprc_path_id);
		goto EXIT;
	}

	ret = hd_videoproc_start(dewarp_param.vpe_path_id);
	if (ret != HD_OK) {
		DBG_ERR("hd_videoproc_start failed!(ret = %d, path_id = %lx)\n", ret, dewarp_param.vpe_path_id);
		goto EXIT;
	}

EXIT:
	if(ret != HD_OK){
		return E_SYS;
	}
	else{
		dewarp_param.is_init = TRUE;
		return E_OK;
	}
}

INT32 UIAppPlay_Dewarp_Uninit(VOID)
{
	HD_RESULT ret = E_OK;

	if(dewarp_param.is_init == FALSE)
		return E_OK;

    ret = close_vprc(dewarp_param.vpe_path_id, dewarp_param.vpe_ctrl_path_id);
	if(ret != HD_OK){
		goto EXIT;
	}

    ret = close_vprc(dewarp_param.vprc_path_id, dewarp_param.vprc_ctrl_path_id);
	if(ret != HD_OK){
		goto EXIT;
	}

	ret = hd_videoproc_unbind(HD_VIDEOPROC_OUT(VPRC_IPP_DEV_ID, 0));
	if (ret != HD_OK) {
		DBG_ERR("hd_videoproc_unbind failed!(%d)\n", ret);
		goto EXIT;
	}

	ret = vendor_vpe_uninit();
	if (ret != HD_OK) {
		DBG_ERR("vendor_vpe_init failed!(%d)\n", ret);
		goto EXIT;
	}

	ret = hd_videoproc_uninit();
    if (ret != HD_OK) {
    	DBG_ERR("hd_videoproc_init fail!(%d)\n", ret);
    	goto EXIT;
    }

	memset(&dewarp_param.cb, 0, sizeof(dewarp_param.cb));
	PBView_SetDewarpCB(dewarp_param.cb);

EXIT:
	if(ret != HD_OK){
		return E_SYS;
	}
	else{
		dewarp_param.is_init = FALSE;
		return E_OK;
	}
}

INT32 UIAppPlay_Dewarp_Reset()
{
	HD_RESULT ret;

	ret = set_dctg_param(VPRC_VPE_ISP_ID, &dewarp_param.dctg);
	if(ret != HD_OK)
		return E_SYS;

	return E_OK;
}

INT32 UIAppPlay_Dewarp_TouchMove(INT32 delta_x, INT delta_y)
{
	HD_RESULT ret;
	VPE_DCTG_PARAM param;

	ret = get_dctg_param(VPRC_VPE_ISP_ID, &param);
	if(ret != HD_OK)
		return E_SYS;

	/* ********************************************************************************
	 * TODO:
	 * how to set dctg param by xy delta of touch panel should be confirmed with ISP team
	 * below is test code to make sure dewarp effect is active
	 ********************************************************************************* */
	param.rot_y += delta_y;
	param.rot_z += delta_x;

	ret = set_dctg_param(VPRC_VPE_ISP_ID, &param);
	if(ret != HD_OK)
		return E_SYS;

	return E_OK;
}

#endif

/**
  Initialize application for Playback mode

  Initialize application for Playback mode.

  @param void
  @return void
*/
INT32 PlayExe_OnOpen(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{

	UINT32 useFileDB = 0;
	UINT32 uiPoolAddr = (UINT32)mempool_filedb;

	//call default
	Ux_DefaultEvent(pCtrl, NVTEVT_EXE_OPEN, paramNum, paramArray);

	if (PlayExe_InitCommonMem() < 0) {
		return NVTEVT_CONSUME;
	}
 
	PlayExe_SetDispParam();
	PlayExe_SetPBParam();

#if USE_FILEDB
	UI_SetData(FL_IsUseFileDB, 1);
#else
	UI_SetData(FL_IsUseFileDB, 0);
#endif
	useFileDB = UI_GetData(FL_IsUseFileDB);
	if (useFileDB) {
		CHAR      *rootPath = "A:\\";
		//CHAR*      defaultfolder="A:\\CarDV\\";

		PPBX_FLIST_OBJ  pFlist = PBXFList_FDB_getObject();
		pFlist->Config(PBX_FLIST_CONFIG_MEM, uiPoolAddr, POOL_SIZE_FILEDB);
		pFlist->Config(PBX_FLIST_CONFIG_MAX_FILENUM, 5000, 0);
		pFlist->Config(PBX_FLIST_CONFIG_MAX_FILEPATH_LEN, 60, 0);
		pFlist->Config(PBX_FLIST_CONFIG_VALID_FILETYPE, PBX_FLIST_FILE_TYPE_JPG | PBX_FLIST_FILE_TYPE_AVI | PBX_FLIST_FILE_TYPE_MOV | PBX_FLIST_FILE_TYPE_MP4 | PBX_FLIST_FILE_TYPE_TS, 0);
		pFlist->Config(PBX_FLIST_CONFIG_DCF_ONLY, FALSE, 0);
		pFlist->Config(PBX_FLIST_CONFIG_SORT_BYSN_DELIMSTR, (UINT32)"_", 0);
		pFlist->Config(PBX_FLIST_CONFIG_SORT_BYSN_DELIMNUM, 1, 0);
		pFlist->Config(PBX_FLIST_CONFIG_SORT_BYSN_NUMOFSN, 6, 0);
		pFlist->Config(PBX_FLIST_CONFIG_SORT_TYPE,PBX_FLIST_SORT_BY_SN,0);
		pFlist->Config(PBX_FLIST_CONFIG_ROOT_PATH, (UINT32)rootPath, 0);
		pFlist->Config(PBX_FLIST_CONFIG_SUPPORT_LONGNAME, 1, 0);
		PB_SetParam(PBPRMID_FILELIST_OBJ, (UINT32)pFlist);
	} else {
		PB_SetParam(PBPRMID_FILELIST_OBJ, (UINT32)PBXFList_DCF_getObject());
	}
	PB_SetParam(PBPRMID_DEC_VIDEO_CALLBACK, (UINT32)PBDecVideoCB);
	PB_SetParam(PBPRMID_ONDRAW_CALLBACK, (UINT32)PBView_OnDrawCB);

#if 0 /* decrypt */

	UINT8 key[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	PLAY_FILE_DECRYPT crypt_info = {
		.type    = PLAY_DECRYPT_TYPE_CONTAINER | PLAY_DECRYPT_TYPE_ALL_FRAME,
		.mode    = PLAY_DECRYPT_MODE_AES256,
		.key     = key,
		.key_len = 32,
	};
	ImageApp_Play_SetParam(PLAY_PARAM_DECRYPT_INFO, (UINT32)&crypt_info);

#endif

	//open
	ImageApp_Play_Open();

    //init EXIF
    PlayExe_InitExif();

#if(WIFI_AP_FUNC==ENABLE)
	if (System_GetState(SYS_STATE_CURRSUBMODE) == SYS_SUBMODE_WIFI) {
#if (RTSP_PLAY_FUNC==ENABLE)
		/*RTSPNVT_OPEN Open = {0};
		Open.uiApiVer = RTSPNVT_API_VERSION;
		Open.Type = RTSPNVT_TYPE_PLAYBACK;
		Open.uiPortNum = 554;
		//#NT#2016/07/13#Charlie Chang -begin
		//#NT# re-allocate rtsp addr and size
		Open.uiWorkAddr = Pool.Addr;
		Open.uiWorkSize = Pool.Size;
		//#NT#2016/07/13#Charlie Chang -end
		RtspNvt_Open(&Open);
		//#NT#2016/07/20#Charlie Chang -begin
		//#NT# for PlayExe_GetTmpBuf function to get buf
		RtspNvt_GetWorkBuf(&g_stream_app_mem);
		//#NT#2016/07/20#Charlie Chang -end
		*/

#endif
		//WifiCmd_Done(WIFIFLAG_MODE_DONE,E_OK);
		Ux_PostEvent(NVTEVT_WIFI_EXE_MODE_DONE, 1, E_OK);
	}
#endif

#if PLAY_DEWARP == ENABLE
	{
		HD_DIM max_dim = (HD_DIM) {PB_MAX_DECODE_W, PB_MAX_DECODE_H};

		UIAppPlay_Dewarp_Init(max_dim, NULL);
	}
#endif

	return NVTEVT_CONSUME;
}

INT32 PlayExe_OnVideoChange(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	// change to current UI mode again
	Ux_SendEvent(0, NVTEVT_SYSTEM_MODE, 1, System_GetState(SYS_STATE_CURRMODE));
	return NVTEVT_CONSUME;

	//return NVTEVT_PASS; //PASS this event to UIWnd!
}

INT32 PlayExe_OnClose(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
    #if _TODO //refer to NA51055-840 JIRA and using new method
    PBView_KeepLastView();
	#endif

	PlayExe_UninitExif();

	ImageApp_Play_Close();

#if PLAY_DEWARP == ENABLE
	UIAppPlay_Dewarp_Uninit();
#endif

	//call default
	Ux_DefaultEvent(pCtrl, NVTEVT_EXE_CLOSE, paramNum, paramArray);

	return NVTEVT_CONSUME;
}

////////////////////////////////////////////////////////////

EVENT_ENTRY CustomPlayObjCmdMap[] = {
	{NVTEVT_EXE_OPEN,               PlayExe_OnOpen            },
	{NVTEVT_EXE_CLOSE,              PlayExe_OnClose            },
	//#NT#2012/07/31#Hideo Lin -end
	{NVTEVT_VIDEO_CHANGE,           PlayExe_OnVideoChange     },
	{NVTEVT_NULL,                   0},  //End of Command Map
};

CREATE_APP(CustomPlayObj, APP_SETUP)

