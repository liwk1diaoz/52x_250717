#include "ImageApp_UsbMovie_int.h"
#include "kflow_common/nvtmpp.h"
#include "vendor_isp.h"
#include "vendor_common.h"

/// ========== Cross file variables ==========
BOOL g_ia_usbmovie_trace_on = TRUE;
UVAC_INFO gUvacInfo = {0};
UVAC_VEND_DEV_DESC *gpUvacVendDevDesc = NULL;
IMAGAPP_UVAC_SET_VIDRESO_CB gUvacSetVidResoCB = NULL;
UINT32 MaxUsbImgLink = 1;
UVAC_DRAM_CFG usbimg_dram_cfg = {0};
UINT32 gUvacMaxFrameSize = 0;
/// ========== Noncross file variables ==========
static BOOL g_ia_usbmovie_is_link_init = FALSE;
static HD_COMMON_MEM_INIT_CONFIG *mem_cfg = NULL;
static UINT32 uvac_pa = 0, uvac_va = 0;
static UINT32 uvac_size = 0;
static UVAC_CHANNEL g_UsbMovie_UvacChannel = UVAC_CHANNEL_1V1A;
static UINT32 g_UsbMovieMode = NVT_USBMOVIE_MODE_CAM;
static UINT32 g_UsbMediaFmt = NVT_USBMOVIE_MEDIA_H264;

static ER _imageapp_usbmovie_mem_init(void)
{
	HD_RESULT hd_ret;

	if (mem_cfg == NULL) {
		DBG_ERR("mem info not config yet\r\n");
		return E_SYS;
	}
	if ((hd_ret = vendor_common_mem_relayout(mem_cfg)) != HD_OK) {
		DBG_ERR("vendor_common_mem_relayout fail(%d)\r\n", hd_ret);
		return E_SYS;
	}
	return E_OK;
}

static ER _imageapp_usbmovie_mem_uninit(void)
{
	return E_OK;
}

static ER _imageapp_usbmovie_module_init(void)
{
	ER ret = E_OK;
	HD_RESULT hd_ret = HD_ERR_NG;

	// ImgLink
	if ((hd_ret = hd_videocap_init()) != HD_OK) {
		DBG_ERR("hd_videocap_init fail(%d)\r\n", hd_ret);
		ret = E_SYS;
	}
	if ((hd_ret = hd_videoproc_init()) != HD_OK) {
		DBG_ERR("hd_videoproc_init fail(%d)\r\n", hd_ret);
		ret = E_SYS;
	}
	if ((hd_ret = hd_videoenc_init()) != HD_OK) {
		DBG_ERR("hd_videoenc_init fail(%d)\r\n", hd_ret);
		ret = E_SYS;
	}
	// AudCapLink
	if ((hd_ret = hd_audiocap_init()) != HD_OK) {
		DBG_ERR("hd_audiocap_init fail(%d)\r\n", hd_ret);
		ret = E_SYS;
	}
	return ret;
}

static ER _imageapp_usbmovie_module_uninit(void)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;

	// ImgLink
	if ((hd_ret = hd_videocap_uninit()) != HD_OK) {
		DBG_ERR("hd_videocap_uninit fail(%d)\r\n", hd_ret);
		ret = E_SYS;
	}
	if ((hd_ret = hd_videoproc_uninit()) != HD_OK) {
		DBG_ERR("hd_videoproc_uninit fail(%d)\r\n", hd_ret);
		ret = E_SYS;
	}
	if ((hd_ret = hd_videoenc_uninit()) != HD_OK) {
		DBG_ERR("hd_videoenc_uninit fail(%d)\r\n", hd_ret);
		ret = E_SYS;
	}
	// AudCapLink
	if ((hd_ret = hd_audiocap_uninit()) != HD_OK) {
		DBG_ERR("hd_audiocap_uninit fail(%d)\r\n", hd_ret);
		ret = E_SYS;
	}
	return ret;
}

static ER _imageaapp_usbmovie_init(void)
{
	// init hdal
	// init memory
	if (_imageapp_usbmovie_mem_init() != HD_OK) {
		return E_SYS;
	}
	// init module
	if (_imageapp_usbmovie_module_init() != HD_OK) {
		_imageapp_usbmovie_mem_uninit();
		return E_SYS;
	}
	return E_OK;
}

static ER _imageaapp_usbmovie_uninit(void)
{
	ER result = E_OK;

	// uninit module
	if (_imageapp_usbmovie_module_uninit() != HD_OK) {
		result = E_SYS;
	}
	// uninit memory
	if (_imageapp_usbmovie_mem_uninit() != HD_OK) {
		result = E_SYS;
	}
	return result;
}

static ER _ImageApp_UsbMovie_Stream_Open(void)
{
	UINT32 i;

	for (i = 0; i < MaxUsbImgLink; i++) {
		_ImageApp_UsbMovie_ImgLinkOpen(i);
	}
	_ImageApp_UsbMovie_AudCapLinkOpen(0);

	return E_OK;
}

static ER _ImageApp_UsbMovie_Stream_Close(void)
{
	UINT32 i;

	for (i = 0; i < MaxUsbImgLink; i++) {
		_ImageApp_UsbMovie_ImgLinkClose(i);
	}
	_ImageApp_UsbMovie_AudCapLinkClose(0);

	return E_SYS;
}

ER _ImageApp_UsbMovie_LinkCfg(void)
{
	UINT32 i;

	if (g_ia_usbmovie_is_link_init == FALSE) {
		for (i = 0; i < MaxUsbImgLink; i++) {
			_ImageApp_UsbMovie_ImgLinkCfg(i);
		}
		_ImageApp_UsbMovie_AudCapLinkCfg(0);
		g_ia_usbmovie_is_link_init = TRUE;
	}
	return E_OK;
}

UINT32 _ImageApp_UsbMovie_frc(UINT32 dst_fr, UINT32 src_fr)
{
	UINT32 frc = 0;

	if (dst_fr == src_fr) {
		frc = HD_VIDEO_FRC_RATIO(1, 1);
	} else {
		frc = HD_VIDEO_FRC_RATIO(dst_fr, src_fr);
	}
	return frc;
}

ER ImageApp_UsbMovie_Open(void)
{
	HD_RESULT hd_ret;
	UINT32 pa;
	void *va;
	HD_COMMON_MEM_DDR_ID ddr_id;

	_imageaapp_usbmovie_init();
	_ImageApp_UsbMovie_InstallID();
	_ImageApp_UsbMovie_Stream_Open();

	ddr_id = usbimg_dram_cfg.working_buf;
	uvac_size = UVAC_GetNeedMemSize() * MaxUsbImgLink;

	if ((hd_ret = hd_common_mem_alloc("usbmovie", &pa, (void **)&va, uvac_size, ddr_id)) != HD_OK) {
		DBG_ERR("hd_common_mem_alloc failed(%d)\r\n", hd_ret);
		uvac_va = 0;
		uvac_pa = 0;
		uvac_size = 0;
	} else {
		uvac_va = (UINT32)va;
		uvac_pa = (UINT32)pa;
	}

	gUvacInfo.UvacMemAdr = uvac_pa;
	gUvacInfo.UvacMemSize = uvac_size;
	gUvacInfo.channel = g_UsbMovie_UvacChannel;
	gUvacInfo.fpStartVideoCB = _ImageApp_UsbMovie_StartVideoCB;
	gUvacInfo.fpStopVideoCB = _ImageApp_UsbMovie_StopVideoCB;
	gUvacInfo.fpSetVolCB = _ImageApp_UsbMovie_SetVolumeCB;

	//DBG_DUMP("UVAC buffer:%x/%x\r\n", uvac_pa, uvac_size);

	if (gpUvacVendDevDesc) {
		UVAC_SetConfig(UVAC_CONFIG_VEND_DEV_DESC, (UINT32)gpUvacVendDevDesc);
	}

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	UVAC_SetConfig(UVAC_CONFIG_HW_COPY_CB, (UINT32)_ImageApp_UsbMovie_memcpy);
#endif

	if (UVAC_Open(&gUvacInfo) != E_OK) {
		DBG_ERR("UVAC_Open fail\r\n");
	}
	vos_flag_set(IAUSBMOVIE_FLG_ID, FLGIAUSBMOVIE_FLOW_LINK_CFG);

	return E_OK;
}

ER ImageApp_UsbMovie_Close(void)
{
	HD_RESULT hd_ret;

	vos_flag_clr(IAUSBMOVIE_FLG_ID, FLGIAUSBMOVIE_FLOW_LINK_CFG);

	UVAC_Close();

	if ((hd_ret = hd_common_mem_free(uvac_pa, (void *)uvac_va)) != HD_OK) {
		DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
	}
	uvac_pa = 0;
	uvac_va = 0;
	uvac_size = 0;

	_ImageApp_UsbMovie_Stream_Close();
	_ImageApp_UsbMovie_UninstallID();
	_imageaapp_usbmovie_uninit();

	// reset variables
	MaxUsbImgLink = 1;
	gUvacMaxFrameSize = 0;

	return E_OK;
}

ER ImageApp_UsbMovie_Config(UINT32 config_id, UINT32 value)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	UINT32 i;

	switch (config_id) {
	case USBMOVIE_CFG_MEM_POOL_INFO: {
			if (value != 0) {
				mem_cfg = (HD_COMMON_MEM_INIT_CONFIG *)value;
				ret = E_OK;
			}
			break;
		}

	case USBMOVIE_CFG_SENSOR_INFO: {
			if (value != 0) {
				ret = E_OK;
				USBMOVIE_SENSOR_INFO *seninfo = (USBMOVIE_SENSOR_INFO *)value;
				if ((hd_ret = vendor_isp_init()) != HD_OK) {
					DBG_ERR("vendor_isp_init fail(%d)\n", hd_ret);
				}
				i = seninfo->rec_id;
				if (i < MAX_USBIMG_PATH) {
					memcpy((void *)&(usbimg_vcap_cfg[i]), (void *)&(seninfo->vcap_cfg), sizeof(HD_VIDEOCAP_DRV_CONFIG));
					usbimg_vcap_senout_pxlfmt[i] = seninfo->senout_pxlfmt;
					usbimg_vcap_capout_pxlfmt[i] = seninfo->capout_pxlfmt;
					usbimg_vcap_data_lane[i] = seninfo->data_lane;
				} else {
					DBG_ERR("sensor id%d is out of range\r\n", i);
					ret = E_SYS;
				}
				char file_path[64];
				if (strlen(seninfo->file_path) != 0) {
					strncpy(file_path, seninfo->file_path, 63);
				} else {
					strncpy(file_path, "/mnt/app/application.dtb", 63);
				}
				if (strlen(seninfo->ae_path.path) != 0) {
					AET_DTSI_INFO ae_dtsi_info;
					ae_dtsi_info.id = seninfo->ae_path.id;
					strncpy(ae_dtsi_info.node_path, seninfo->ae_path.path, 31);
					strncpy(ae_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
					ae_dtsi_info.buf_addr = (UINT8 *)seninfo->ae_path.addr;
					if ((hd_ret = vendor_isp_set_ae(AET_ITEM_RLD_DTSI, &ae_dtsi_info)) != HD_OK) {
						DBG_ERR("vendor_isp_set_ae fail(%d)\r\n", hd_ret);
						ret = E_SYS;
					}
				}
				if (strlen(seninfo->awb_path.path) != 0) {
					AWBT_DTSI_INFO awb_dtsi_info;
					awb_dtsi_info.id = seninfo->awb_path.id;
					strncpy(awb_dtsi_info.node_path, seninfo->awb_path.path, 31);
					strncpy(awb_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
					awb_dtsi_info.buf_addr = (UINT8 *)seninfo->awb_path.addr;
					if ((hd_ret = vendor_isp_set_awb(AWBT_ITEM_RLD_DTSI, &awb_dtsi_info)) != HD_OK) {
						DBG_ERR("vendor_isp_set_awb fail(%d)\r\n", hd_ret);
						ret = E_SYS;
					}
				}
				if (strlen(seninfo->af_path.path) != 0) {
					AFT_DTSI_INFO af_dtsi_info;
					af_dtsi_info.id = seninfo->af_path.id;
					strncpy(af_dtsi_info.node_path, seninfo->af_path.path, 31);
					strncpy(af_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
					af_dtsi_info.buf_addr = (UINT8 *)seninfo->af_path.addr;
					if ((hd_ret = vendor_isp_set_af(AFT_ITEM_RLD_DTSI, &af_dtsi_info)) != HD_OK) {
						DBG_ERR("vendor_isp_set_af fail(%d)\r\n", hd_ret);
						ret = E_SYS;
					}
				}
				if (strlen(seninfo->iq_path.path) != 0) {
					IQT_DTSI_INFO iq_dtsi_info;
					iq_dtsi_info.id = seninfo->iq_path.id;
					strncpy(iq_dtsi_info.node_path, seninfo->iq_path.path, 31);
					strncpy(iq_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
					iq_dtsi_info.buf_addr = (UINT8 *)seninfo->iq_path.addr;
					if ((hd_ret = vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &iq_dtsi_info)) != HD_OK) {
						DBG_ERR("vendor_isp_set_iq fail(%d)\r\n", hd_ret);
						ret = E_SYS;
					}
				}
				if (strlen(seninfo->iq_shading_path.path) != 0) {
					IQT_DTSI_INFO iq_dtsi_info;
					iq_dtsi_info.id = seninfo->iq_shading_path.id;
					strncpy(iq_dtsi_info.node_path, seninfo->iq_shading_path.path, 31);
					strncpy(iq_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
					iq_dtsi_info.buf_addr = (UINT8 *)seninfo->iq_shading_path.addr;
					if ((hd_ret = vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &iq_dtsi_info)) != HD_OK) {
						DBG_ERR("vendor_isp_set_iq fail(%d)\r\n", hd_ret);
						ret = E_SYS;
					}
				}
				if (strlen(seninfo->iq_dpc_path.path) != 0) {
					IQT_DTSI_INFO iq_dtsi_info;
					iq_dtsi_info.id = seninfo->iq_dpc_path.id;
					strncpy(iq_dtsi_info.node_path, seninfo->iq_dpc_path.path, 31);
					strncpy(iq_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
					iq_dtsi_info.buf_addr = (UINT8 *)seninfo->iq_dpc_path.addr;
					if ((hd_ret = vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &iq_dtsi_info)) != HD_OK) {
						DBG_ERR("vendor_isp_set_iq fail(%d)\r\n", hd_ret);
						ret = E_SYS;
					}
				}
				if (strlen(seninfo->iq_ldc_path.path) != 0) {
					IQT_DTSI_INFO iq_dtsi_info;
					iq_dtsi_info.id = seninfo->iq_ldc_path.id;
					strncpy(iq_dtsi_info.node_path, seninfo->iq_ldc_path.path, 31);
					strncpy(iq_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
					iq_dtsi_info.buf_addr = (UINT8 *)seninfo->iq_ldc_path.addr;
					if ((hd_ret = vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &iq_dtsi_info)) != HD_OK) {
						DBG_ERR("vendor_isp_set_iq fail(%d)\r\n", hd_ret);
						ret = E_SYS;
					}
				}
				if ((hd_ret = vendor_isp_uninit()) != HD_OK) {
					DBG_ERR("vendor_isp_uninit fail(%d)\n", hd_ret);
				}
			}
			break;
		}

	case USBMOVIE_CFG_CHANNEL: {
			if (value == UVAC_CHANNEL_1V1A) {
				g_UsbMovie_UvacChannel = value;
				MaxUsbImgLink = 1;
			} else {
				g_UsbMovie_UvacChannel = UVAC_CHANNEL_2V1A;
				MaxUsbImgLink = 2;
			}
			break;
		}

	case USBMOVIE_CFG_TBR_MJPG: {
			ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_UVAC_TBR_MJPG, value);
			break;
		}

	case USBMOVIE_CFG_TBR_H264: {
			ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_UVAC_TBR_H264, value);
			break;
		}

	case USBMOVIE_CFG_SET_MOVIE_SIZE_CB: {
			gUvacSetVidResoCB = (IMAGAPP_UVAC_SET_VIDRESO_CB)value;
			break;
		}

	case USBMOVIE_CFG_MODE: {
			g_UsbMovieMode = (UINT32)value;
			break;
		}

	case USBMOVIE_CFG_MEDIA_FMT: {
			g_UsbMediaFmt = (UINT32)value;
			break;
		}

	case USBMOVIE_CFG_VDOIN_INFO: {
			break;
		}

	case USBMOVIE_CFG_SENSOR_MAPPING: {
			if (value != 0) {
				memcpy((void *)&(usbimg_sensor_mapping[0]), (void *)value, sizeof(UINT32) * MAX_USBIMG_PATH);
				ret = E_OK;
			}
			break;
		}

	case USBMOVIE_CFG_DRAM: {
			if (value != 0) {
				UVAC_DRAM_CFG *pdramcfg = (UVAC_DRAM_CFG *)value;
				memcpy((void *)&(usbimg_dram_cfg), (void *)pdramcfg, sizeof(UVAC_DRAM_CFG));
				ImageApp_UsbMovie_DUMP("dram_cfg:\r\n");
				ImageApp_UsbMovie_DUMP("  working_buf :[%d]\r\n", usbimg_dram_cfg.working_buf);
				ImageApp_UsbMovie_DUMP("  video_cap:");
				for (i = 0; i < 2; i++) {
					ImageApp_UsbMovie_DUMP("[%d]", usbimg_dram_cfg.video_cap[i]);
				}
				ImageApp_UsbMovie_DUMP("\r\n");
				ImageApp_UsbMovie_DUMP("  video_proc:");
				for (i = 0; i < 2; i++) {
					ImageApp_UsbMovie_DUMP("[%d]", usbimg_dram_cfg.video_proc[i]);
				}
				ImageApp_UsbMovie_DUMP("\r\n");
				ImageApp_UsbMovie_DUMP("  video_encode:[%d]\r\n", usbimg_dram_cfg.video_encode);
				ret = E_OK;
			}
			break;
		}
	}
	return ret;
}

