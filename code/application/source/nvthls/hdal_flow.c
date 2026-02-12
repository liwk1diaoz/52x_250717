#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal_flow.h"
#include "avfile/media_def.h"  //for MEDIA_FILEFORMAT_MP4
#include "avfile/movieinterface_def.h" // for MOVREC_ENDTYPE_NORMAL
#include "vendor_videoenc.h"
#include "vendor_audioenc.h"
#include "vendor_videocapture.h"
#include "tse.h"

static UINT32 g_frame_rate = 0;
static UINT32 g_width=0;
static UINT32 g_height=0;

static HD_RESULT set_enc_param(HD_PATH_ID video_enc_path, HD_DIM *p_dim, UINT32 enc_type, UINT32 bitrate);

static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+
#if HLS_HDMI_MODEL == 1
					VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, CAP_OUT_FMT)+
#else
					VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, CAP_OUT_FMT)+
#endif
					VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)+
					VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	mem_cfg.pool_info[0].blk_cnt = 5;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[1].blk_cnt = 5;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	//file sys
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[2].blk_size = HLS_POOL_SIZE_FILESYS;
	mem_cfg.pool_info[2].blk_cnt = 1;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;

	#if (HLS_AUDIO_ENABLE == 1)
		mem_cfg.pool_info[3].type = HD_COMMON_MEM_COMMON_POOL;
		mem_cfg.pool_info[3].blk_size = 0x5000;
		mem_cfg.pool_info[3].blk_cnt = 3;
		mem_cfg.pool_info[3].ddr_id = DDR_ID0;
	#endif
	ret = hd_common_mem_init(&mem_cfg);
	return ret;
}

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_init()) != HD_OK){
		printf("hd_videocap_init fail\n");
		return ret;
	}
	if ((ret = hd_videoproc_init()) != HD_OK){
		printf("hd_videoproc_init fail\n");
		return ret;
	}
	if ((ret = hd_videoenc_init()) != HD_OK){
		printf("hd_videoenc_init fail\n");
		return ret;
	}
	if ((ret = hd_bsmux_init()) != HD_OK){
		printf("hd_bsmux_init fail\n");
		return ret;
	}
	if ((ret = hd_fileout_init()) != HD_OK){
		printf("hd_fileout_init fail\n");
		return ret;
	}


	// gfx for bsmux and fileout
	if ((ret = hd_gfx_init()) != HD_OK){
		printf("gfx init fail\n");
		return ret;
	}
	
	#if (HLS_AUDIO_ENABLE == 1)
		if((ret = hd_audiocap_init()) != HD_OK){
			printf("hd_audiocap_init fail\n");
			return ret;
		}
		if((ret = hd_audioenc_init()) != HD_OK){
			printf("hd_audioenc_init fail\n");
			return ret;
		}
	#endif

	return HD_OK;
}

static HD_RESULT set_cap_cfg(HD_PATH_ID *p_video_cap_ctrl)
{

	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};
	#if HLS_HDMI_MODEL == 1
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_ad_tc358840");
		cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x1 | 0x100 | 0x200 | 0x400 | 0x800;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0|PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
		cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.ccir_vd_hd_pin = TRUE;
	#else
		//snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os02k10");
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
		cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x301;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0|PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
		cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = HD_VIDEOCAP_SEN_IGNORE;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = HD_VIDEOCAP_SEN_IGNORE;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	#endif
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	#if HLS_HDMI_MODEL == 1
		iq_ctl.func = 0;
	#else
		iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB;
	#endif
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);
	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}


static HD_RESULT set_proc_cfg(HD_PATH_ID *p_video_proc_ctrl, HD_DIM* p_max_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
	HD_PATH_ID video_proc_ctrl = 0;
	ret = hd_videoproc_open(0, HD_VIDEOPROC_0_CTRL, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;
	if (p_max_dim != NULL ) {
		#if HLS_HDMI_MODEL == 1
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_YUVALL;
		#else
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWALL;
		#endif
		video_cfg_param.isp_id = 0;
		video_cfg_param.ctrl_max.func = 0;
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;
		video_cfg_param.in_max.pxlfmt = CAP_OUT_FMT;
		video_cfg_param.in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &video_cfg_param);
		if (ret != HD_OK) {
			return HD_ERR_NG;
		}
	}
	video_ctrl_param.func = 0;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);
	*p_video_proc_ctrl = video_proc_ctrl;
	return ret;
}


#if (HLS_AUDIO_ENABLE == 1)
static HD_RESULT set_a_cap_cfg(HD_PATH_ID *p_audio_cap_ctrl)
{
	HD_RESULT ret;
	HD_PATH_ID audio_cap_ctrl = 0;
	HD_AUDIOCAP_DEV_CONFIG audio_dev_cfg = {0};
	HD_AUDIOCAP_DRV_CONFIG audio_drv_cfg = {0};

	ret = hd_audiocap_open(0, HD_AUDIOCAP_0_CTRL, &audio_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	// set audiocap dev parameter
	audio_dev_cfg.in_max.sample_rate = HLS_AUDIO_SAMPLE_RATE;
	audio_dev_cfg.in_max.sample_bit = HLS_AUDIO_SAMPLEBIT_WIDTH;
	audio_dev_cfg.in_max.mode = HLS_AUDIO_MODE;
	audio_dev_cfg.in_max.frame_sample = HLS_AUDIO_FRAME_SAMPLE;
	audio_dev_cfg.frame_num_max = 10;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DEV_CONFIG, &audio_dev_cfg);
	if (ret != HD_OK) {
		return ret;
	}

	// set audiocap drv parameter
	audio_drv_cfg.mono = HD_AUDIO_MONO_RIGHT;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DRV_CONFIG, &audio_drv_cfg);

	*p_audio_cap_ctrl = audio_cap_ctrl;
	return ret;


}



#endif

static HD_RESULT open_module(VIDEO_RECORD *p_stream, HD_DIM* p_proc_max_dim)
{
	HD_RESULT ret;
	// set videocap config
	ret = set_cap_cfg(&p_stream->cap_ctrl);
	if (ret != HD_OK) {
		printf("set cap-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoproc config
	ret = set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim);
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}

	#if (HLS_AUDIO_ENABLE == 1)
		ret = set_a_cap_cfg(&p_stream->a_cap_ctrl);
		if (ret != HD_OK) {
			printf("set a_cap-cfg fail\r\n");
			return HD_ERR_NG;
		}
		if((ret = hd_audiocap_open(HD_AUDIOCAP_0_IN_0, HD_AUDIOCAP_0_OUT_0, &p_stream->a_cap_path)) != HD_OK)
			return ret;
		if((ret = hd_audioenc_open(HD_AUDIOENC_0_IN_0, HD_AUDIOENC_0_OUT_0, &p_stream->a_enc_path)) != HD_OK)
			return ret;
	#endif

	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_0, HD_VIDEOENC_0_OUT_0, &p_stream->enc_path)) != HD_OK)
		return ret;
	if ((ret = hd_bsmux_open(0, 0, &p_stream->bsmux_path)) != HD_OK)
		return ret;
	if ((ret = hd_fileout_open(0, 0, &p_stream->fileout_path)) != HD_OK)
		return ret;



	return HD_OK;
}


HD_RESULT set_fileout_param(VIDEO_RECORD *stream)
{
	HD_RESULT ret = HD_OK;
	printf("set_fileout_param\n");
	ret = hd_fileout_set(stream->fileout_path, HD_FILEOUT_PARAM_REG_CALLBACK, stream->fileout_cb_fun);
	if(ret != HD_OK)
	{
		printf("hd_fileout_set HD_FILEOUT_PARAM_REG_CALLBACK   fail ret:%d\n",ret);
	}


	return ret;
}


HD_RESULT set_bsmux_param(VIDEO_RECORD *stream)
{
	HD_RESULT ret = HD_OK;
	HD_BSMUX_VIDEOINFO bsmux_video = {0};
	HD_BSMUX_FILEINFO bsmux_fileinfo = {0};
	HD_BSMUX_BUFINFO bsmux_bufinfo={0};
	HD_BSMUX_WRINFO bsmux_wrinfo={0};


	HD_BSMUX_AUDIOINFO bsmux_audio={0};
	#if (HLS_AUDIO_ENABLE == 0)
		bsmux_audio.aud_en = 0;
		bsmux_audio.codectype = MOVAUDENC_AAC;
		bsmux_audio.chs =1;
		bsmux_audio.asr = 8000;
		bsmux_audio.adts_bytes = 7;
	#else
		bsmux_audio.aud_en = 1;
		if(HLS_AUDIO_TYPE == HLS_AUDIO_AAC)
		{
			printf("audio type :aac\n");
			bsmux_audio.codectype = MOVAUDENC_AAC;
		}
		else if(HLS_AUDIO_TYPE == HLS_AUDIO_ULAW)
		{
			bsmux_audio.codectype = MOVAUDENC_ULAW;
		}
		else if(HLS_AUDIO_TYPE == HLS_AUDIO_ALAW)
		{
			bsmux_audio.codectype = MOVAUDENC_ALAW;
		}
		else
		{
			printf("HLS_AUDIO_TYPE fail :%d\n",HLS_AUDIO_TYPE);
			ret = HD_ERR_NG;
			goto exit;
		}
		bsmux_audio.chs = HLS_AUDIO_MODE;
		bsmux_audio.asr = HLS_AUDIO_SAMPLE_RATE;
		if(bsmux_audio.codectype == MOVAUDENC_AAC)
		{
			bsmux_audio.adts_bytes = 7;
		}
		else
		{
			bsmux_audio.adts_bytes = 0;
		}
	#endif
	ret = hd_bsmux_set(stream->bsmux_path, HD_BSMUX_PARAM_AUDIOINFO, &bsmux_audio);
	if(ret != HD_OK)
	{
		printf("hd_bsmux_set HD_BSMUX_PARAM_AUDIOINFO fail ret:%d\n",ret);
		goto exit;
	}
	printf("set_bsmux_param in\n");
	if(HLS_ENC_TYPE == HLS_H264)
	{
		bsmux_video.vidcodec = MEDIAVIDENC_H264;
	}
	else if (HLS_ENC_TYPE == HLS_H265)
	{
		bsmux_video.vidcodec = MEDIAVIDENC_H265;
	}
	else if (HLS_ENC_TYPE == HLS_JPEG)
	{
		bsmux_video.vidcodec = MEDIAVIDENC_MJPG;
	}
	else
	{
		printf("HLS_ENC_TYPE=%d error\n",HLS_ENC_TYPE);
		ret = HD_ERR_PARAM;
		goto exit;
	}
	bsmux_video.vfr = g_frame_rate;
	bsmux_video.width = g_width; 
	bsmux_video.height =  g_height;
	bsmux_video.tbr= HLS_BITRATE;
	bsmux_video.gop = HLS_GOP;	
	ret = hd_bsmux_set(stream->bsmux_path, HD_BSMUX_PARAM_VIDEOINFO, &bsmux_video);
	if(ret != HD_OK)
	{
		printf("hd_bsmux_set HD_BSMUX_PARAM_VIDEOINFO fail ret:%d\n",ret);
		goto exit;
	}
	bsmux_fileinfo.emron = FALSE;
	bsmux_fileinfo.emrloop = FALSE;
	bsmux_fileinfo.strgid = 0;
	bsmux_fileinfo.seamlessSec = stream->record_time;
	bsmux_fileinfo.rollbacksec = 4; // no using, set default value
	bsmux_fileinfo.keepsec = 6; // no using, set default value
	bsmux_fileinfo.endtype= MOVREC_ENDTYPE_CUTOVERLAP;
	bsmux_fileinfo.filetype = MEDIA_FILEFORMAT_TS;
	#if (HLS_AUDIO_ENABLE == 1)
	bsmux_fileinfo.recformat = MEDIAREC_AUD_VID_BOTH;
	#else
	bsmux_fileinfo.recformat = MEDIAREC_VID_ONLY;
	#endif
	bsmux_fileinfo.playvfr = 30; // this is for timelapse
	bsmux_fileinfo.revsec = 7; //follow cardv
	bsmux_fileinfo.overlop_on = FALSE;
	ret = hd_bsmux_set(stream->bsmux_path, HD_BSMUX_PARAM_FILEINFO, &bsmux_fileinfo);
	if(ret != HD_OK)
	{
		printf("hd_bsmux_set HD_BSMUX_PARAM_FILEINFO fail ret:%d\n",ret);
		goto exit;
	}
	//for get video enc buf, need start videoenc first
	ret = hd_videoenc_start(stream->enc_path);
	if(ret != HD_OK)
	{
		printf("hd_videoenc_start fail ret:%d\n",ret);
		goto exit;
	}
	ret = hd_videoenc_get(stream->enc_path, HD_VIDEOENC_PARAM_BUFINFO, &bsmux_bufinfo.videnc);
	if(ret != HD_OK)
	{
		printf("hd_videoenc_get HD_VIDEOENC_PARAM_BUFINFO fail ret:%d\n",ret);
		goto exit;
	}
	ret = hd_videoenc_stop(stream->enc_path);
	if(ret != HD_OK)
	{
		printf("hd_videoenc_stop fail ret:%d\n",ret);
		goto exit;
	}

	//for get audio enc buf, need start audio enc first
	#if HLS_AUDIO_ENABLE == 1
	ret = hd_audioenc_start(stream->a_enc_path);
        if(ret != HD_OK)
        {
                printf("hd_audioenc_start fail ret:%d\n",ret);
                goto exit;
        }
        ret = hd_audioenc_get(stream->a_enc_path, HD_AUDIOENC_PARAM_BUFINFO, &bsmux_bufinfo.audenc);
        if(ret != HD_OK)
        {
                printf("hd_audioenc_get HD_AUDIOENC_PARAM_BUFINFO  fail ret:%d\n",ret);
                goto exit;
        }
        ret = hd_audioenc_stop(stream->a_enc_path);
        if(ret != HD_OK)
        {
                printf("hd_audioenc_stop fail ret:%d\n",ret);
                goto exit;
        }
	printf("audio buf:%x size:%x\n",bsmux_bufinfo.audenc.phy_addr,bsmux_bufinfo.audenc.buf_size);
	#endif

	printf("video buf:%x size:%x\n",bsmux_bufinfo.videnc.phy_addr,bsmux_bufinfo.videnc.buf_size);
	ret = hd_bsmux_set(stream->bsmux_path, HD_BSMUX_PARAM_BUFINFO, &bsmux_bufinfo);
	if(ret != HD_OK)
	{
		printf("hd_bsmux_set HD_BSMUX_PARAM_BUFINFO fail ret:%d\n",ret);
		goto exit;
	}
	//follow by cardv  UIAppMovie_Exe.c, maybe no need to set
	bsmux_wrinfo.wrblk_size = 0x200000;
	bsmux_wrinfo.flush_freq = 10;
	ret = hd_bsmux_set(stream->bsmux_path, HD_BSMUX_PARAM_WRINFO, &bsmux_wrinfo);
	if(ret != HD_OK)
	{
		printf("hd_bsmux_set HD_BSMUX_WRINFO fail ret:%d\n",ret);
		goto exit;
	}
	ret = hd_bsmux_set(stream->bsmux_path, HD_BSMUX_PARAM_REG_CALLBACK, stream->bsmux_cb_fun);
	if(ret != HD_OK)
	{
		printf("hd_bsmux_set HD_BSMUX_PARAM_REG_CALLBACK fail ret:%d\n",ret);
		goto exit;
	}
exit:
	return ret;
}


static HD_RESULT get_cap_caps(HD_PATH_ID video_cap_ctrl, HD_VIDEOCAP_SYSCAPS *p_video_cap_syscaps)
{
	HD_RESULT ret = HD_OK;
	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSCAPS, p_video_cap_syscaps);
	return ret;
}

static HD_RESULT set_proc_param(HD_PATH_ID video_proc_path, HD_DIM* p_dim)
{
	HD_RESULT ret = HD_OK;
	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}
	return ret;
}

static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};
		video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
		video_in_param.frc = HD_VIDEO_FRC_RATIO(g_frame_rate,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		video_in_param.pxlfmt = SEN_OUT_FMT;
		video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN, &video_in_param);
		//printf("set_cap_param MODE=%d\r\n", ret);
		if (ret != HD_OK) {
			return ret;
		}
	}
	#if 1 //no crop, full frame
	{
		HD_VIDEOCAP_CROP video_crop_param = {0};
		video_crop_param.mode = HD_CROP_OFF;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &video_crop_param);
		//printf("set_cap_param CROP NONE=%d\r\n", ret);
	}
	#else //HD_CROP_ON
	{
		HD_VIDEOCAP_CROP video_crop_param = {0};
		video_crop_param.mode = HD_CROP_ON;
		video_crop_param.win.rect.x = 0;
		video_crop_param.win.rect.y = 0;
		video_crop_param.win.rect.w = 1920/2;
		video_crop_param.win.rect.h= 1080/2;
		video_crop_param.align.w = 4;
		video_crop_param.align.h = 4;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &video_crop_param);
		//printf("set_cap_param CROP ON=%d\r\n", ret);
	}
	#endif
	{
		HD_VIDEOCAP_OUT video_out_param = {0};
		//without setting dim for no scaling, using original sensor out size
		video_out_param.pxlfmt = CAP_OUT_FMT;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
	}
	return ret;
}

static HD_RESULT set_enc_param(HD_PATH_ID video_enc_path, HD_DIM *p_dim, UINT32 enc_type, UINT32 bitrate)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_IN  video_in_param = {0};
	HD_VIDEOENC_OUT video_out_param = {0};
	HD_H26XENC_RATE_CONTROL rc_param = {0};
	if (p_dim != NULL) {
		//--- HD_VIDEOENC_PARAM_IN ---
		video_in_param.dir           = HD_VIDEO_DIR_NONE;
		video_in_param.pxl_fmt = HD_VIDEO_PXLFMT_YUV420;
		video_in_param.dim.w   = p_dim->w;
		video_in_param.dim.h   = p_dim->h;
		video_in_param.frc     = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_IN, &video_in_param);
		if (ret != HD_OK) {
			printf("set_enc_param_in = %d\r\n", ret);
			return ret;
		}
		printf("enc_type=%d\r\n", enc_type);
		if (enc_type == 0) {
			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_H265;
			video_out_param.h26x.profile       = HD_H265E_MAIN_PROFILE;
			video_out_param.h26x.level_idc     = HD_H265E_LEVEL_5;
			video_out_param.h26x.gop_num       = HLS_GOP;
			video_out_param.h26x.ltr_interval  = 0;
			video_out_param.h26x.ltr_pre_ref   = 0;
			video_out_param.h26x.gray_en       = 0;
			video_out_param.h26x.source_output = 0;
			video_out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			video_out_param.h26x.entropy_mode  = HD_H265E_CABAC_CODING;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}
			//--- HD_VIDEOENC_PARAM_OUT_RATE_CONTROL ---
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = bitrate;
			rc_param.cbr.frame_rate_base = g_frame_rate;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control = %d\r\n", ret);
				return ret;
			}
		} else if (enc_type == 1) {
			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_H264;
			video_out_param.h26x.profile       = HD_H264E_HIGH_PROFILE;
			video_out_param.h26x.level_idc     = HD_H264E_LEVEL_5_1;
			video_out_param.h26x.gop_num       = HLS_GOP;
			video_out_param.h26x.ltr_interval  = 0;
			video_out_param.h26x.ltr_pre_ref   = 0;
			video_out_param.h26x.gray_en       = 0;
			video_out_param.h26x.source_output = 0;
			video_out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			video_out_param.h26x.entropy_mode  = HD_H264E_CABAC_CODING;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}
			//--- HD_VIDEOENC_PARAM_OUT_RATE_CONTROL ---
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = bitrate;
			rc_param.cbr.frame_rate_base = g_frame_rate;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control = %d\r\n", ret);
				return ret;
			}
		} else if (enc_type == 2) {
			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_JPEG;
			video_out_param.jpeg.retstart_interval = 0;
			video_out_param.jpeg.image_quality = 50;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}
		} else {
			printf("not support enc_type\r\n");
			return HD_ERR_NG;
		}
	}
	return ret;
}

static HD_RESULT set_enc_cfg(HD_PATH_ID video_enc_path, HD_DIM *p_max_dim, UINT32 max_bitrate)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_PATH_CONFIG video_path_config = {0};
	if (p_max_dim != NULL) {
		//--- HD_VIDEOENC_PARAM_PATH_CONFIG ---
		video_path_config.max_mem.codec_type = HD_CODEC_TYPE_H264;
		video_path_config.max_mem.max_dim.w  = p_max_dim->w;
		video_path_config.max_mem.max_dim.h  = p_max_dim->h;
		video_path_config.max_mem.bitrate    = max_bitrate;
		video_path_config.max_mem.enc_buf_ms = 3000;
		video_path_config.max_mem.svc_layer  = HD_SVC_4X;
		video_path_config.max_mem.ltr        = TRUE;
		video_path_config.max_mem.rotate     = FALSE;
		video_path_config.max_mem.source_output   = FALSE;
		video_path_config.isp_id             = 0;
		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_PATH_CONFIG, &video_path_config);
		if (ret != HD_OK) {
			printf("set_enc_path_config = %d\r\n", ret);
			return HD_ERR_NG;
		}
	}


	//for ts buf
	VENDOR_VIDEOENC_BS_RESERVED_SIZE_CFG bs_reserved_cfg = {0};
	bs_reserved_cfg.reserved_size = HLS_TS_VID_SIZE;
	vendor_videoenc_set(video_enc_path, VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE, &bs_reserved_cfg);
	return ret;
}

static HD_RESULT close_module(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret;
	#if (HLS_AUDIO_ENABLE == 1)
		if((ret = hd_audiocap_close(p_stream->a_cap_path)) != HD_OK)
			return ret;
		if((ret = hd_audioenc_close(p_stream->a_enc_path)) != HD_OK)
			return ret;
	#endif
	if ((ret = hd_videocap_close(p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path)) != HD_OK)
		return ret;
	if ((ret = hd_bsmux_close(p_stream->bsmux_path)) != HD_OK)
		return ret;
	if ((ret = hd_fileout_close(p_stream->fileout_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	#if (HLS_AUDIO_ENABLE == 1)
		if((ret = hd_audiocap_uninit()) != HD_OK)
			return ret;
		if((ret = hd_audioenc_uninit()) != HD_OK)
			return ret;
	#endif
	if ((ret = hd_videocap_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_bsmux_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_fileout_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}

HD_RESULT hdal_mem_uinit(void)
{
	HD_RESULT ret;
        // uninit memory
	hd_common_mem_uninit();
        // uninit hdal
	ret = hd_common_uninit();
        if (ret != HD_OK) {
                printf("common fail=%d\n", ret);
		return ret;
        }
	return ret;

}

static HD_RESULT hdal_uninit(VIDEO_RECORD *stream)
{
	HD_RESULT ret;
        // close video_record modules (main)
        ret = close_module(stream);
        if (ret != HD_OK) {
                printf("close fail=%d\n", ret);
		goto exit;
        }
        // uninit all modules
        ret = exit_module();
        if (ret != HD_OK) {
                printf("exit fail=%d\n", ret);
		goto exit;
        }
 exit:
	return ret;
}

HD_RESULT hdal_stop(VIDEO_RECORD *stream)
{

	// stop video_record modules (main)
	#if (HLS_AUDIO_ENABLE == 1)
		// stop audio_record modules
		hd_audiocap_stop(stream->a_cap_path);
		hd_audioenc_stop(stream->a_enc_path);
		// unbind audio_record modules
		hd_audiocap_unbind(HD_AUDIOCAP_0_OUT_0);
	#endif
	hd_videocap_stop(stream->cap_path);
	hd_videoproc_stop(stream->proc_path);
	hd_videoenc_stop(stream->enc_path);
	hd_bsmux_stop(stream->bsmux_path);
	hd_fileout_stop(stream->fileout_path);

	// unbind video_record modules (main)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);
	hdal_uninit(stream);
	return HD_OK;
}

#if (HLS_AUDIO_ENABLE == 1)
static HD_RESULT set_a_cap_param(HD_PATH_ID audio_cap_path)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOCAP_IN audio_cap_param = {0};

	// set audiocap input parameter
	audio_cap_param.sample_rate = HLS_AUDIO_SAMPLE_RATE;
	audio_cap_param.sample_bit = HLS_AUDIO_SAMPLEBIT_WIDTH;
	audio_cap_param.mode = HLS_AUDIO_MODE;
	audio_cap_param.frame_sample = HLS_AUDIO_FRAME_SAMPLE;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_IN, &audio_cap_param);

	return ret;
}

static HD_RESULT set_a_enc_cfg(HD_PATH_ID audio_enc_path, UINT32 enc_type)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOENC_PATH_CONFIG audio_path_cfg = {0};
	// set audioenc path config
	audio_path_cfg.max_mem.codec_type = enc_type;
	audio_path_cfg.max_mem.sample_rate = HLS_AUDIO_SAMPLE_RATE;
	audio_path_cfg.max_mem.sample_bit = HLS_AUDIO_SAMPLEBIT_WIDTH;
	audio_path_cfg.max_mem.mode = HLS_AUDIO_MODE;
	ret = hd_audioenc_set(audio_enc_path, HD_AUDIOENC_PARAM_PATH_CONFIG, &audio_path_cfg);

	///for ts buf
	VENDOR_AUDIOENC_BS_RESERVED_SIZE_CFG reserved_size = {0};
	reserved_size.reserved_size = HLS_TS_AUD_SIZE;
	if ((ret = vendor_audioenc_set(audio_enc_path, VENDOR_AUDIOENC_ITEM_BS_RESERVED_SIZE, &reserved_size)) != HD_OK) {
		printf("vendor_audioenc_set(VENDOR_AUDIOENC_ITEM_BS_RESERVED_SIZE) fail(%d)\r\n", ret);
	}


	return ret;
}

static HD_RESULT set_a_enc_param(HD_PATH_ID audio_enc_path, UINT32 enc_type)
{

	HD_RESULT ret = HD_OK;
	HD_AUDIOENC_IN audio_in_param = {0};
	HD_AUDIOENC_OUT audio_out_param = {0};

	// set audioenc input parameter
	audio_in_param.sample_rate = HLS_AUDIO_SAMPLE_RATE;
	audio_in_param.sample_bit = HLS_AUDIO_SAMPLEBIT_WIDTH;
	audio_in_param.mode = HLS_AUDIO_MODE;
	ret = hd_audioenc_set(audio_enc_path, HD_AUDIOENC_PARAM_IN, &audio_in_param);
	if (ret != HD_OK) {
		printf("set_a_enc_param_in = %d\r\n", ret);
		return ret;
	}

	// set audioenc output parameter
	audio_out_param.codec_type = enc_type;
	audio_out_param.aac_adts = (enc_type == HD_AUDIO_CODEC_AAC) ? TRUE : FALSE;
	ret = hd_audioenc_set(audio_enc_path, HD_AUDIOENC_PARAM_OUT, &audio_out_param);
	if (ret != HD_OK) {
		printf("set_a_enc_param_out = %d\r\n", ret);
		return ret;
	}

        return ret;
}

#endif

#if HLS_HDMI_MODEL == 1
static HD_RESULT get_ccir_config(VIDEO_RECORD *stream, UINT32 *frame_rate, UINT32 *width, UINT32 *height)
{

	VENDOR_VIDEOCAP_GET_PLUG_INFO plug_info = {0};

	HD_RESULT ret=0;

	if(!stream || !frame_rate || !width || !height)
	{
		printf("get_ccir_config param address stream:%x frame_rate:%x width:%x height:%x error\n",
		stream, frame_rate, width, height);
		return HD_ERR_NULL_PTR;
	}


	BOOL plugged = FALSE;
	UINT32 retry_count=10;
	while(plugged == FALSE){
		ret = vendor_videocap_get(stream->cap_ctrl, VENDOR_VIDEOCAP_PARAM_GET_PLUG, &plugged);
		if(ret != HD_OK)
		{
			printf("vendor_videocap_get VENDOR_VIDEOCAP_PARAM_GET_PLUG fail ret:%d\n",ret);
			return ret;
		}
		printf("sensor plugged:%d retry_count:%d\n",plugged,retry_count);
		if ((ret = vendor_videocap_get(stream->cap_ctrl, VENDOR_VIDEOCAP_PARAM_GET_PLUG_INFO, &plug_info)) == HD_OK)
		{
			*frame_rate = plug_info.fps%100;
			*width = plug_info.size.w;
			*height = plug_info.size.h;
			if(*frame_rate!=0 && *width!=0 && *height!=0)
			{
				break;
			}
		}
		else
		{
			printf("vendor_videocap_get VENDOR_VIDEOCAP_PARAM_GET_PLUG_INFO fail ret:%d\n",ret);
		}
		retry_count--;
		if(retry_count == 0)
		{
			printf("get sensor info fail\r\n");
			return HD_ERR_NG;
		}
		sleep(1);
	}
	return ret;
}

#endif

HD_RESULT hdal_init( VIDEO_RECORD *stream)
{

	if(stream == NULL)
	{
		printf("stream structure NULL\n");
		return HD_ERR_NULL_PTR;
	}

	UINT32 enc_type = HLS_ENC_TYPE;
	#if (HLS_AUDIO_ENABLE == 1)
		UINT32 a_enc_type = HLS_AUDIO_TYPE;
	#endif
	HD_RESULT ret;



		// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
	}

	// init memory
	ret = mem_init();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		goto exit;
	}


	// init all modules
	ret = init_module();
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}


	// open video_record modules (main)
	stream->proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream->proc_max_dim.h = VDO_SIZE_H; //assign by user
	ret = open_module(stream, &stream->proc_max_dim);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	#if HLS_HDMI_MODEL == 1
		//need call after open module
		ret = get_ccir_config(stream, &g_frame_rate, &g_width, &g_height);
		if(ret != HD_OK)
		{
			printf("get_ccir_config fail ret:%d\n",ret);
			goto exit;
		}
		if(g_frame_rate >= 60)
		{
			g_frame_rate = 60;
		}
		else if(g_frame_rate <60 && g_frame_rate >=30)
		{
			g_frame_rate = 30;
		}
		else
		{
			g_frame_rate = 15;
		}
	#else
		g_frame_rate = HLS_FRAME_RATE;
		g_width = VDO_SIZE_W;
		g_height = VDO_SIZE_H;
		if(g_width > VDO_SIZE_W)
		{
			printf("HDMI get wigth:%d please set VDO_SIZE_W > %d\n",g_width);
			goto exit;
		} 
		if(g_height > VDO_SIZE_H)
		{
			printf("HDMI get height:%d please set VDO_SIZE_H > %d\n",g_height);
			goto exit;
		} 
	#endif

	printf("fr:%u w:%u h:%u\n",g_frame_rate,g_width,g_height);

	#if (HLS_AUDIO_ENABLE == 1)
		// set audiocap parameter
		ret = set_a_cap_param(stream->a_cap_path);
		if (ret != HD_OK) {
			printf("set audio cap fail=%d\n", ret);
			goto exit;
		}

		if (HLS_AUDIO_TYPE == HLS_AUDIO_AAC) {
			a_enc_type = HD_AUDIO_CODEC_AAC;
		}
		else if (HLS_AUDIO_TYPE == HLS_AUDIO_ULAW)
		{
			a_enc_type = HD_AUDIO_CODEC_ULAW;
		}
		else if(HLS_AUDIO_TYPE == HLS_AUDIO_ALAW)
		{
			a_enc_type = HD_AUDIO_CODEC_ALAW;
		}
		else{
			printf("HLS_AUDIO_TYPE fail :%d\n",HLS_AUDIO_TYPE);
			goto exit;
		}

		// set audioenc config
		ret = set_a_enc_cfg(stream->a_enc_path, a_enc_type);
		if (ret != HD_OK) {
			printf("set audio enc-cfg fail=%d\n", ret);
			goto exit;
		}

		// set audioenc paramter
		ret = set_a_enc_param(stream->a_enc_path, a_enc_type);
		if (ret != HD_OK) {
			printf("set audio enc fail=%d\n", ret);
			goto exit;
		}


	#endif


	// get videocap capability
	ret = get_cap_caps(stream->cap_ctrl, &stream->cap_syscaps);
	if (ret != HD_OK) {
		printf("get cap-caps fail=%d\n", ret);
		goto exit;
	}	

	// set videocap parameter
	stream->cap_dim.w = g_width; //assign by user
	stream->cap_dim.h = g_height; //assign by user
	ret = set_cap_param(stream->cap_path, &stream->cap_dim);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}


	// set videoproc parameter (main)
	ret = set_proc_param(stream->proc_path, NULL);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}


	// set videoenc config (main)
	stream->enc_max_dim.w = g_width;
	stream->enc_max_dim.h = g_height;
	stream->enc_bitrate = HLS_BITRATE;
	ret = set_enc_cfg(stream->enc_path, &stream->enc_max_dim, HLS_BITRATE);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}
	
        // set videoenc parameter (main)
	#if 1
	stream->enc_dim.w = g_width;
	stream->enc_dim.h = g_height;
	ret = set_enc_param(stream->enc_path, &stream->enc_dim, enc_type, HLS_BITRATE);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}
	#endif
	//set bsmux param
	ret = set_bsmux_param(stream);
	if (ret != HD_OK) {
		printf("set set_bsmux_param fail=%d\n", ret);
		goto exit;
	}

	//set_fileout_param
	ret = set_fileout_param(stream);
	if (ret != HD_OK) {
		printf("set set_fileout_param fail=%d\n", ret);
		goto exit;
	}

	// bind video_record modules (main)
	hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOENC_0_IN_0);

	#if (HLS_AUDIO_ENABLE == 1)
		// bind audio_record modules
		hd_audiocap_bind(HD_AUDIOCAP_0_OUT_0, HD_AUDIOENC_0_IN_0);
		// start audio_record modules
		hd_audioenc_start(stream->a_enc_path);
		hd_audiocap_start(stream->a_cap_path);

	#endif

	hd_videocap_start(stream->cap_path);
	hd_videoproc_start(stream->proc_path);
	hd_videoenc_start(stream->enc_path);
	hd_bsmux_start(stream->bsmux_path);
	hd_fileout_start(stream->fileout_path);
	return HD_OK;



exit:
	hdal_uninit(stream);

	return HD_ERR_NG;
}
