#include "ImageApp_MovieMulti_int.h"
#include "kflow_common/nvtmpp.h"
#include "NamingRule/NameRule_Custom.h"
#include "Utility/avl.h"
#include "efuse_protected.h"

/// ========== Cross file variables ==========
UINT32 img_flow_for_dumped_fast_boot = 0;
UINT32 IsMovieMultiOpened = 0;
MOVIEMULTI_MAX_LINK_INFO MaxLinkInfo = {1, MAX_DISP_PATH, MAX_WIFI_PATH, MAX_AUDCAP_PATH, 0};
UINT32 iamovie_need_reopen = 0;
MOVIEMULTI_DRAM_CFG dram_cfg = {0};
#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
UINT32 iamovie_use_filedb_and_naming = TRUE;
#else
UINT32 iamovie_use_filedb_and_naming = FALSE;
#endif

// Important notice!!! If enlarge MAX_IMG_PATH, following variable shoule also update initial value too !!!
#if (MAX_IMG_PATH == 4)
UINT32 img_vcap_capout_dir[MAX_IMG_PATH] = {HD_VIDEO_DIR_NONE, HD_VIDEO_DIR_NONE, HD_VIDEO_DIR_NONE, HD_VIDEO_DIR_NONE};
UINT32 img_sensor_mapping[MAX_IMG_PATH] = {0, 1, 2, 3};
UINT32 img_bsmux_bufsec[MAX_IMG_PATH][4] = {{7, 7, 7, 7}, {7, 7, 7, 7}, {7, 7, 7, 7}, {7, 7, 7, 7}};
UINT32 ImgLinkVdoEncBufMs[MAX_IMG_PATH][2] = {{3000, 3000}, {3000, 3000}, {3000, 3000}, {3000, 3000}};
MOVIE_AD_MAP ad_sensor_map[MAX_IMG_PATH] = {{UNUSED, UNUSED, UNUSED}, {UNUSED, UNUSED, UNUSED}, {UNUSED, UNUSED, UNUSED}, {UNUSED, UNUSED, UNUSED}};
#elif (MAX_IMG_PATH == 3)
UINT32 img_vcap_capout_dir[MAX_IMG_PATH] = {HD_VIDEO_DIR_NONE, HD_VIDEO_DIR_NONE, HD_VIDEO_DIR_NONE};
UINT32 img_sensor_mapping[MAX_IMG_PATH] = {0, 1, 2};
UINT32 img_bsmux_bufsec[MAX_IMG_PATH][4] = {{7, 7, 7, 7}, {7, 7, 7, 7}, {7, 7, 7, 7}};
UINT32 ImgLinkVdoEncBufMs[MAX_IMG_PATH][2] = {{3000, 3000}, {3000, 3000}, {3000, 3000}};
MOVIE_AD_MAP ad_sensor_map[MAX_IMG_PATH] = {{UNUSED, UNUSED, UNUSED}, {UNUSED, UNUSED, UNUSED}, {UNUSED, UNUSED, UNUSED}};
#else       // MAX_IMG_PATH == 2
UINT32 img_vcap_capout_dir[MAX_IMG_PATH] = {HD_VIDEO_DIR_NONE, HD_VIDEO_DIR_NONE};
UINT32 img_sensor_mapping[MAX_IMG_PATH] = {0, 1};
UINT32 img_bsmux_bufsec[MAX_IMG_PATH][4] = {{7, 7, 7, 7}, {7, 7, 7, 7}};
UINT32 ImgLinkVdoEncBufMs[MAX_IMG_PATH][2] = {{3000, 3000}, {3000, 3000}};
MOVIE_AD_MAP ad_sensor_map[MAX_IMG_PATH] = {{UNUSED, UNUSED, UNUSED}, {UNUSED, UNUSED, UNUSED}};
#endif

/// ========== Noncross file variables ==========
static UINT32 img_vprc_occupied = 0;
static HD_COMMON_MEM_INIT_CONFIG *mem_cfg = NULL;
static BOOL g_ia_moviemulti_is_link_init = FALSE;
static UINT32 g_ia_moviemulti_recid_to_bsport[_CFG_CLONE_ID_MAX] = {
	0,      // _CFG_REC_ID_1,
	4,      // _CFG_REC_ID_2,
	8,      // _CFG_REC_ID_3,
	12,     // _CFG_REC_ID_4,
	UNUSED,	// _CFG_REC_ID_MAX,
	UNUSED,	// _CFG_STRM_ID_1,
	UNUSED,	// _CFG_STRM_ID_2,
	UNUSED,	// _CFG_STRM_ID_3,
	UNUSED,	// _CFG_STRM_ID_4,
	UNUSED,	// _CFG_STRM_ID_MAX,
	UNUSED,	// _CFG_UVAC_ID_1,
	UNUSED,	// _CFG_UVAC_ID_2,
	UNUSED,	// _CFG_UVAC_ID_MAX,
	2,      // _CFG_CLONE_ID_1,
	6,      // _CFG_CLONE_ID_2,
	10,     // _CFG_CLONE_ID_3,
	14,     // _CFG_CLONE_ID_4,
};
static MOVIE_CFG_REC_ID g_ia_moviemulti_bsport_to_recid[16] = {
	_CFG_REC_ID_1,
	_CFG_REC_ID_1,
	_CFG_CLONE_ID_1,
	_CFG_CLONE_ID_1,
	_CFG_REC_ID_2,
	_CFG_REC_ID_2,
	_CFG_CLONE_ID_2,
	_CFG_CLONE_ID_2,
	_CFG_REC_ID_3,
	_CFG_REC_ID_3,
	_CFG_CLONE_ID_3,
	_CFG_CLONE_ID_3,
	_CFG_REC_ID_4,
	_CFG_REC_ID_4,
	_CFG_CLONE_ID_4,
	_CFG_CLONE_ID_4,
};
static BOOL g_isf_dvcam_is_file_naming[MAX_IMG_PATH * 2 + MAX_ETHCAM_PATH * 1] = {0};

static ER _update_table(void)
{
	UINT32 i, bsport_used = 0;

	// clear g_ia_moviemulti_recid_to_bsport[]
	for (i = 0; i < _CFG_CLONE_ID_MAX; i ++) {
		g_ia_moviemulti_recid_to_bsport[i] = UNUSED;
	}
	// clear g_ia_moviemulti_bsport_to_recid[]
	for (i = 0; i < 16; i ++) {
		g_ia_moviemulti_bsport_to_recid[i] = UNUSED;
	}
	// update g_ia_moviemulti_recid_to_bsport[] & g_ia_moviemulti_recid_to_bsport
	if (MaxLinkInfo.MaxImgLink > 0) {
		g_ia_moviemulti_recid_to_bsport[_CFG_REC_ID_1]   = 0;
		g_ia_moviemulti_recid_to_bsport[_CFG_CLONE_ID_1] = 2;
		g_ia_moviemulti_bsport_to_recid[bsport_used ++]  = _CFG_REC_ID_1;
		g_ia_moviemulti_bsport_to_recid[bsport_used ++]  = _CFG_REC_ID_1;
		g_ia_moviemulti_bsport_to_recid[bsport_used ++]  = _CFG_CLONE_ID_1;
		g_ia_moviemulti_bsport_to_recid[bsport_used ++]  = _CFG_CLONE_ID_1;
	}
	if (MaxLinkInfo.MaxImgLink > 1) {
		g_ia_moviemulti_recid_to_bsport[_CFG_REC_ID_2]   = 4;
		g_ia_moviemulti_recid_to_bsport[_CFG_CLONE_ID_2] = 6;
		g_ia_moviemulti_bsport_to_recid[bsport_used ++]  = _CFG_REC_ID_2;
		g_ia_moviemulti_bsport_to_recid[bsport_used ++]  = _CFG_REC_ID_2;
		g_ia_moviemulti_bsport_to_recid[bsport_used ++]  = _CFG_CLONE_ID_2;
		g_ia_moviemulti_bsport_to_recid[bsport_used ++]  = _CFG_CLONE_ID_2;
	}
	if (MaxLinkInfo.MaxImgLink > 2) {
		g_ia_moviemulti_recid_to_bsport[_CFG_REC_ID_3]   = 8;
		g_ia_moviemulti_recid_to_bsport[_CFG_CLONE_ID_3] = 10;
		g_ia_moviemulti_bsport_to_recid[bsport_used ++] = _CFG_REC_ID_3;
		g_ia_moviemulti_bsport_to_recid[bsport_used ++] = _CFG_REC_ID_3;
		g_ia_moviemulti_bsport_to_recid[bsport_used ++] = _CFG_CLONE_ID_3;
		g_ia_moviemulti_bsport_to_recid[bsport_used ++] = _CFG_CLONE_ID_3;
	}
	if (MaxLinkInfo.MaxImgLink > 3) {
		g_ia_moviemulti_recid_to_bsport[_CFG_REC_ID_4]   = 12;
		g_ia_moviemulti_recid_to_bsport[_CFG_CLONE_ID_4] = 14;
		g_ia_moviemulti_bsport_to_recid[bsport_used ++]  = _CFG_REC_ID_4;
		g_ia_moviemulti_bsport_to_recid[bsport_used ++]  = _CFG_REC_ID_4;
		g_ia_moviemulti_bsport_to_recid[bsport_used ++]  = _CFG_CLONE_ID_4;
		g_ia_moviemulti_bsport_to_recid[bsport_used ++]  = _CFG_CLONE_ID_4;
	}
	if (MaxLinkInfo.MaxEthCamLink > 0) {
		if (bsport_used < 14) {
			g_ia_moviemulti_bsport_to_recid[bsport_used ++] = _CFG_ETHCAM_ID_1;
			g_ia_moviemulti_bsport_to_recid[bsport_used ++] = _CFG_ETHCAM_ID_1;
		} else {
			DBG_ERR("Run of of BsMux port!\r\n");
		}
	}
	if (MaxLinkInfo.MaxEthCamLink > 1) {
		if (bsport_used < 14) {
			g_ia_moviemulti_bsport_to_recid[bsport_used ++] = _CFG_ETHCAM_ID_2;
			g_ia_moviemulti_bsport_to_recid[bsport_used ++] = _CFG_ETHCAM_ID_2;
		} else {
			DBG_ERR("Run out of BsMux port!\r\n");
		}
	}
#if 0
	DBG_DUMP("g_ia_moviemulti_recid_to_bsport[]:\r\n");
	for (i = 0; i < _CFG_CLONE_ID_MAX; i++) {
		DBG_DUMP("  %d,\r\n", g_ia_moviemulti_recid_to_bsport[i]);
	}
	DBG_DUMP("g_ia_moviemulti_bsport_to_recid[]:\r\n");
	for (i = 0; i < 16; i ++) {
		DBG_DUMP("  %d,\r\n", g_ia_moviemulti_bsport_to_recid[i]);
	}
#endif
	return E_OK;
}

// VdoOut flow refine: do hd_common_init / hd_common_uninit in project
#if 0
static ER _imageapp_moviemulti_common_init(UINT32 type)
{
	HD_RESULT ret;

	if ((ret = hd_common_init(type)) != HD_OK) {
		DBG_ERR("hd_common_init fail(%d)\r\n", ret);
		return E_SYS;
	}
	return E_OK;
}

static ER _imageapp_moviemulti_common_uninit(void)
{
	HD_RESULT ret;

	if ((ret = hd_common_uninit()) != HD_OK) {
		DBG_ERR("hd_common_uninit fail(%d)\r\n", ret);
		return E_SYS;
	}
	return E_OK;
}
#endif

static ER _imageapp_moviemulti_mem_init(void)
{
	HD_RESULT ret;

	if (mem_cfg == NULL) {
		DBG_ERR("mem info not config yet\r\n");
		return E_SYS;
	}
	if ((ret = vendor_common_mem_relayout(mem_cfg)) != HD_OK) {
		DBG_ERR("vendor_common_mem_relayout fail(%d)\r\n", ret);
		return E_SYS;
	}
	return E_OK;
}

static ER _imageapp_moviemulti_mem_uninit(void)
{
	return E_OK;
}

static ER _imageapp_moviemulti_module_init(void)
{
	ER result = E_OK;
	HD_RESULT hd_ret = HD_ERR_NG;

	// ImgLink
	if ((hd_ret = hd_videocap_init()) != HD_OK) {
		DBG_ERR("hd_videocap_init fail(%d)\r\n", hd_ret);
		result = E_SYS;
	}
	if ((hd_ret = hd_videoproc_init()) != HD_OK) {
		DBG_ERR("hd_videoproc_init fail(%d)\r\n", hd_ret);
		result = E_SYS;
	}
	if ((hd_ret = hd_videoenc_init()) != HD_OK) {
		DBG_ERR("hd_videoenc_init fail(%d)\r\n", hd_ret);
		result = E_SYS;
	}
	if ((hd_ret = hd_bsmux_init()) != HD_OK) {
		DBG_ERR("hd_bsmux_init fail(%d)\r\n", hd_ret);
		//result = E_SYS;
	}
	if ((hd_ret = hd_fileout_init()) != HD_OK) {
		DBG_ERR("hd_fileout_init fail(%d)\r\n", hd_ret);
		//result = E_SYS;
	}
	// DispLink
	#if 0
	// VdoOut flow refine: hd_videoout_init in project
	if ((hd_ret = hd_videoout_init()) != HD_OK) {
		DBG_ERR("hd_videoout_init fail(%d)\r\n", hd_ret);
		result = E_SYS;
	}
	#endif
	// AudCapLink
	if (aud_acap_by_hdal) {
		if ((hd_ret = hd_audiocap_init()) != HD_OK) {
			DBG_ERR("hd_audiocap_init fail(%d)\r\n", hd_ret);
			result = E_SYS;
		}
	}
	if ((hd_ret = hd_audioenc_init()) != HD_OK) {
		DBG_ERR("hd_audioenc_init fail(%d)\r\n", hd_ret);
		result = E_SYS;
	}
	// EthCamLink
	if (EthCamRxFuncEn && (hd_ret = hd_videodec_init()) != HD_OK) {
		DBG_ERR("hd_videodec_init fail(%d)\r\n", hd_ret);
		result = E_SYS;
	}
	return result;
}

static ER _imageapp_moviemulti_module_uninit(void)
{
	ER result = E_OK;
	HD_RESULT hd_ret;

	// ImgLink
	if ((hd_ret = hd_videocap_uninit()) != HD_OK) {
		DBG_ERR("hd_videocap_uninit fail(%d)\r\n", hd_ret);
		result = E_SYS;
	}
	if ((hd_ret = hd_videoproc_uninit()) != HD_OK) {
		DBG_ERR("hd_videoproc_uninit fail(%d)\r\n", hd_ret);
		result = E_SYS;
	}
	if ((hd_ret = hd_videoenc_uninit()) != HD_OK) {
		DBG_ERR("hd_videoenc_uninit fail(%d)\r\n", hd_ret);
		result = E_SYS;
	}
	if ((hd_ret = hd_bsmux_uninit()) != HD_OK) {
		DBG_ERR("hd_bsmux_uninit fail(%d)\r\n", hd_ret);
		//result = E_SYS;
	}
	if ((hd_ret = hd_fileout_uninit()) != HD_OK) {
		DBG_ERR("hd_fileout_uninit fail(%d)\r\n", hd_ret);
		//result = E_SYS;
	}
	// DispLink
	#if 0
	if ((hd_ret = hd_videoout_uninit()) != HD_OK) {
		DBG_ERR("hd_videoout_uninit fail(%d)\r\n", hd_ret);
		result = E_SYS;
	}
	#endif
	// AudCapLink
	if (aud_acap_by_hdal) {
		if ((hd_ret = hd_audiocap_uninit()) != HD_OK) {
			DBG_ERR("hd_audiocap_uninit fail(%d)\r\n", hd_ret);
			result = E_SYS;
		}
	}
	if ((hd_ret = hd_audioenc_uninit()) != HD_OK) {
		DBG_ERR("hd_audioenc_uninit fail(%d)\r\n", hd_ret);
		result = E_SYS;
	}
	// EthCamLink
	if (EthCamRxFuncEn && (hd_ret = hd_videodec_uninit()) != HD_OK) {
		DBG_ERR("hd_videodec_uninit fail(%d)\r\n", hd_ret);
		result = E_SYS;
	}
	return result;
}

static ER _imageaapp_moviemulti_init(void)
{
	// init hdal
	// VdoOut flow refine: do hd_common_init() in project
	#if 0
	if (_imageapp_moviemulti_common_init(0) != HD_OK) {
		return E_SYS;
	}
	#endif
	// init memory
	if (_imageapp_moviemulti_mem_init() != HD_OK) {
		// VdoOut flow refine:
		//_imageapp_moviemulti_common_uninit();
		return E_SYS;
	}
	// init module
	if (_imageapp_moviemulti_module_init() != HD_OK) {
		_imageapp_moviemulti_mem_uninit();
		// VdoOut flow refine:
		//_imageapp_moviemulti_common_uninit();
		return E_SYS;
	}
	return E_OK;
}

static ER _imageaapp_moviemulti_uninit(void)
{
	ER result = E_OK;

	// uninit module
	if (_imageapp_moviemulti_module_uninit() != HD_OK) {
		result = E_SYS;
	}
	// uninit memory
	if (_imageapp_moviemulti_mem_uninit() != HD_OK) {
		result = E_SYS;
	}
	// uninit hdal
	// VdoOut flow refine: do hd_common_uninit() in project
	#if 0
	if (_imageapp_moviemulti_common_uninit() != HD_OK) {
		result = E_SYS;
	}
	#endif
	return result;
}

static ER _ImageApp_MovieMulti_Stream_Open(void)
{
	UINT32 i, idx;

	if (_ImageApp_MovieMulti_CheckOpenSetting() != E_OK) {
		DBG_ERR("\r\n\r\nSetting conflict!!! Check setting first.\r\n\r\n");
		return E_SYS;
	}

	for (i = 0; i < MaxLinkInfo.MaxImgLink; i++) {
		img_vprc_occupied |= (0x01 << img_sensor_mapping[i]);
	}
	//DBG_DUMP("init img_vprc_occupied to 0x%x\r\n", img_vprc_occupied);

	// create disp path
	for (i = _CFG_DISP_ID_1; i < (_CFG_DISP_ID_1 + MaxLinkInfo.MaxDispLink); i ++) {
		idx = i - _CFG_DISP_ID_1;
		_ImageApp_MovieMulti_DispLinkOpen(_CFG_DISP_ID_1 + idx);
	}

	// create wifi path
	_ImageApp_MovieMulti_WifiLinkOpen(_CFG_STRM_ID_1);

	// config audio param
	_ImageApp_MovieMulti_SetupAudioParam();

	// create image path
	for (i = _CFG_REC_ID_1; i < (_CFG_REC_ID_1 + MaxLinkInfo.MaxImgLink); i ++) {
		idx = i - _CFG_REC_ID_1;
		if (IsSetImgLinkRecInfo[idx][0] == TRUE) {
			_ImageApp_MovieMulti_ImgLinkOpen(i);
		}
	}

	// create imgcap path
	_ImageApp_MovieMulti_ImgCapLinkOpen(0);

	// create audio capture path
	_ImageApp_MovieMulti_AudCapLinkOpen(0);

	// create ethcam path
	if (EthCamRxFuncEn && MaxLinkInfo.MaxEthCamLink) {
		for (i = 0; i < MaxLinkInfo.MaxEthCamLink; i ++) {
			_ImageApp_MovieMulti_EthCamLinkOpen(_CFG_ETHCAM_ID_1 + i);
		}
	}
	return E_OK;
}

static ER _ImageApp_MovieMulti_Stream_Close(void)
{
	UINT32 i, idx, j;
	HD_RESULT hd_ret;

	if (wifi_movie_uvac_func_en) {
		UVAC_Close();

		if (wifi_movie_uvac_va) {
			if ((hd_ret = hd_common_mem_free(wifi_movie_uvac_pa, (void *)wifi_movie_uvac_va)) != HD_OK) {
				DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
			}
		}
		wifi_movie_uvac_pa = 0;
		wifi_movie_uvac_va = 0;
		wifi_movie_uvac_size = 0;
	}

	// close disp path
	for (i = _CFG_DISP_ID_1; i < (_CFG_DISP_ID_1 + MaxLinkInfo.MaxDispLink); i ++) {
		idx = i - _CFG_DISP_ID_1;
		_ImageApp_MovieMulti_DispLinkClose(_CFG_DISP_ID_1 + idx);
	}

	// close wifi path
	_ImageApp_MovieMulti_WifiLinkClose(_CFG_STRM_ID_1);

	// close imgcap path
	_ImageApp_MovieMulti_ImgCapLinkClose(0);

	// close image path
	for (i = _CFG_REC_ID_1; i < (_CFG_REC_ID_1 + MaxLinkInfo.MaxImgLink); i ++) {
		idx = i - _CFG_REC_ID_1;
		if (IsSetImgLinkRecInfo[idx][0] == TRUE) {
			_ImageApp_MovieMulti_ImgLinkClose(i);
		}
	}

	// close audio capture path (since audio path is an individual path, close last)
	_ImageApp_MovieMulti_AudCapLinkClose(0);

	// close ethcam path
	if (EthCamRxFuncEn && MaxLinkInfo.MaxEthCamLink) {
		for (i = 0; i < MaxLinkInfo.MaxEthCamLink; i ++) {
			_ImageApp_MovieMulti_EthCamLinkClose(_CFG_ETHCAM_ID_1 + i);
		}
	}

	// release buffer
	memset(IsSetImgLinkRecInfo, 0, sizeof(IsSetImgLinkRecInfo));
	memset(IsSetImgLinkAlgInfo, 0, sizeof(IsSetImgLinkAlgInfo));
	memset(IsImgLinkForEthCamTxEnabled, 0, sizeof(IsImgLinkForEthCamTxEnabled));
	if (gpsdata_va) {
		if ((hd_ret = hd_common_mem_free(gpsdata_pa, (void *)gpsdata_va)) != HD_OK) {
			DBG_ERR("hd_common_mem_free(gpsdata_va) failed(%d)\r\n", hd_ret);
		}
		gpsdata_pa = 0;
		gpsdata_va = 0;
		gpsdata_size = 0;
		gpsdata_offset = 0;
	}
	iamovie_gps_buffer_size = MAX_GPS_DATA_SIZE;
	for (i = 0; i < MAX_IMG_PATH; i ++) {
		for (j = 0; j < 2; j ++) {
			if (gpsdata_rec_va[i][j]) {
				if ((hd_ret = hd_common_mem_free(gpsdata_rec_pa[i][j], (void *)gpsdata_rec_va[i][j])) != HD_OK) {
					DBG_ERR("hd_common_mem_free(gpsdata_rec_va[%d][%d]) failed(%d)\r\n", i, j, hd_ret);
				}
				gpsdata_rec_pa[i][j] = 0;
				gpsdata_rec_va[i][j] = 0;
				gpsdata_rec_size[i][j] = 0;
				gpsdata_rec_offset[i][j] = 0;
			}
		}
	}
	for (i = 0; i < MAX_ETHCAM_PATH; i ++) {
		for (j = 0; j < 1; j ++) {
			if (gpsdata_eth_va[i][j]) {
				if ((hd_ret = hd_common_mem_free(gpsdata_eth_pa[i][j], (void *)gpsdata_eth_va[i][j])) != HD_OK) {
					DBG_ERR("hd_common_mem_free(gpsdata_eth_va[%d][%d]) failed(%d)\r\n", i, j, hd_ret);
				}
				gpsdata_eth_pa[i][j] = 0;
				gpsdata_eth_va[i][j] = 0;
				gpsdata_eth_size[i][j] = 0;
				gpsdata_eth_offset[i][j] = 0;
			}
		}
	}
	// reset movie uvac setting
	wifi_movie_uvac_func_en = FALSE;

	// reset crypto variable
	iamovie_encrypt_type = MOVIEMULTI_ENCRYPT_TYPE_NONE;
	iamovie_encrypt_mode = MOVIEMULTI_ENCRYPT_MODE_AES128;
	memset(iamovie_encrypt_key, 0, sizeof(iamovie_encrypt_key));

	img_vprc_occupied = 0;
	img_flow_for_dumped_fast_boot = 0;

	return E_SYS;
}

ER _ImageApp_MovieMulti_LinkCfg(void)
{
	UINT32 i;

	if (g_ia_moviemulti_is_link_init == FALSE) {
		for (i = _CFG_DISP_ID_1; i < (_CFG_DISP_ID_1 + MaxLinkInfo.MaxDispLink); i++) {
			_ImageApp_MovieMulti_DispLinkCfg(i);
		}
		for (i = _CFG_STRM_ID_1; i < (_CFG_STRM_ID_1 + MaxLinkInfo.MaxWifiLink); i++) {
			_ImageApp_MovieMulti_WifiLinkCfg(i);
		}
		for (i = 0; i < MaxLinkInfo.MaxImgLink; i++) {
			_ImageApp_MovieMulti_ImgLinkCfg(i, FALSE);
		}
		for (i = 0; i < MaxLinkInfo.MaxAudCapLink; i++) {
			_ImageApp_MovieMulti_AudCapLinkCfg(0);
		}
		for (i = _CFG_ETHCAM_ID_1; i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink); i++) {
			_ImageApp_MovieMulti_EthCamLinkCfg(i, FALSE);
		}
		g_ia_moviemulti_is_link_init = TRUE;
	}
	return E_OK;
}

ER _ImageApp_MovieMulti_RecidGetTableIndex(MOVIE_CFG_REC_ID id, MOVIE_TBL_IDX *p)
{
	ER ret = E_OK;
	UINT32 i = id;

	if ((i < (_CFG_REC_ID_1 + MaxLinkInfo.MaxImgLink)) && (i < (_CFG_REC_ID_1 + MAX_IMG_PATH))) {
		p->idx = i - _CFG_REC_ID_1;
		p->tbl = MOVIETYPE_MAIN;
		p->link_type = LINKTYPE_REC;
	} else if ((i >= _CFG_CLONE_ID_1) && (i < (_CFG_CLONE_ID_1 + MaxLinkInfo.MaxImgLink)) && (i < (_CFG_CLONE_ID_1 + MAX_IMG_PATH))) {
		p->idx = i - _CFG_CLONE_ID_1;
		p->tbl = MOVIETYPE_CLONE;
		p->link_type = LINKTYPE_REC;
	} else if ((i >= _CFG_STRM_ID_1) && (i < (_CFG_STRM_ID_1 + MaxLinkInfo.MaxWifiLink)) && (i < (_CFG_STRM_ID_1 + MAX_WIFI_PATH))) {
		p->idx = i - _CFG_STRM_ID_1;
		p->tbl = 0;
		p->link_type = LINKTYPE_STRM;
	} else if ((i >= _CFG_UVAC_ID_1) && (i < (_CFG_UVAC_ID_1 + MaxLinkInfo.MaxWifiLink)) && (i < (_CFG_UVAC_ID_1 + MAX_WIFI_PATH))) {
		p->idx = i - _CFG_UVAC_ID_1;
		p->tbl = 0;
		p->link_type = LINKTYPE_UVAC;
	}  else if ((i >= _CFG_DISP_ID_1) && (i < (_CFG_DISP_ID_1 + MaxLinkInfo.MaxDispLink)) && (i < (_CFG_DISP_ID_1 + MAX_DISP_PATH))) {
		p->idx = i - _CFG_DISP_ID_1;
		p->tbl = UNUSED;
		p->link_type = LINKTYPE_DISP;
	}  else if ((i >= _CFG_ETHCAM_ID_1) && (i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink)) && (i < (_CFG_ETHCAM_ID_1 + MAX_ETHCAM_PATH))) {
		p->idx = i - _CFG_ETHCAM_ID_1;
		p->tbl = UNUSED;
		p->link_type = LINKTYPE_ETHCAM;
	}  else if ((i >= _CFG_AUDIO_ID_1) && (i < (_CFG_AUDIO_ID_1 + MaxLinkInfo.MaxAudCapLink)) && (i < (_CFG_AUDIO_ID_1 + MAX_AUDCAP_PATH))) {
		p->idx = i - _CFG_AUDIO_ID_1;
		p->tbl = UNUSED;
		p->link_type = LINKTYPE_AUDCAP;
	} else if ((i & ETHCAM_TX_MAGIC_KEY) == ETHCAM_TX_MAGIC_KEY) { // for ethcam tx use only
		i &= ~ETHCAM_TX_MAGIC_KEY;
		if ((i < (_CFG_REC_ID_1 + MaxLinkInfo.MaxImgLink)) && (i < (_CFG_REC_ID_1 + MAX_IMG_PATH))) {
			p->idx = i - _CFG_REC_ID_1;
			p->tbl = MOVIETYPE_MAIN;
			p->link_type = LINKTYPE_ETHCAM_TX;
		} else if ((i >= _CFG_CLONE_ID_1) && (i < (_CFG_CLONE_ID_1 + MaxLinkInfo.MaxImgLink)) && (i < (_CFG_CLONE_ID_1 + MAX_IMG_PATH))) {
			p->idx = i - _CFG_CLONE_ID_1;
			p->tbl = MOVIETYPE_CLONE;
			p->link_type = LINKTYPE_ETHCAM_TX;
		}
	} else {
		p->idx = UNUSED;
		p->tbl = UNUSED;
		p->link_type = UNUSED;
		ret = (i == _CFG_CTRL_ID) ? E_OK : E_SYS;
	}
	return ret;
}

ER _ImageApp_MovieMulti_BsPortGetTableIndex(UINT32 id, MOVIE_TBL_IDX *p)
{
	ER ret = E_OK;
	UINT32 i = id;

	if (i < MaxLinkInfo.MaxImgLink * 4) {
		p->idx = i / 4;
		p->tbl = (i % 4) / 2;
		p->link_type = LINKTYPE_REC;
	} else if (i < MaxLinkInfo.MaxImgLink * 4 + MaxLinkInfo.MaxEthCamLink * 2) {
		p->idx = (i - MaxLinkInfo.MaxImgLink * 4) / 2;
		p->tbl = UNUSED;
		p->link_type = LINKTYPE_ETHCAM;
	} else {
		p->idx = UNUSED;
		p->tbl = UNUSED;
		p->link_type = UNUSED;
		ret = E_SYS;
	}
	return ret;
}

UINT32 _ImageApp_MovieMulti_GetFreeVprc(void)
{
	UINT32 i = 0, occupied = img_vprc_occupied;

	while (occupied & 0x01) {
		occupied >>= 1;
		i++;
	}
	img_vprc_occupied |= (0x01 << i);
	//DBG_DUMP("%s:i=%d, img_vprc_occupied=%x\r\n", __func__, i, img_vprc_occupied);

	return i;
}

#define MAX_REC_WIDTH        1920
#define MAX_REC_HEIGHT       1080
#define MAX_WIFI_WIDTH       1280
#define MAX_WIFI_HEIGHT       720

static BOOL _limited_func_check(void)
{
	UINT32 result = TRUE;
#if defined(_BSP_NA51089_)
	UINT32 is_support_full_func = FALSE;
#if defined(__FREERTOS)
	is_support_full_func = efuse_check_available_extend(EFUSE_ABILITY_CUSTOM, 0);
#else
	int h_avl;
	char avl_info[] = "avl EFUSE_ABILITY_CUSTOM";

	if ((h_avl = open("/proc/nvt_info/drvdump/available", O_RDWR)) < 0) {
		DBG_ERR("Please modprobe drvdump first\r\n");
	} else {
		write(h_avl, avl_info, sizeof(avl_info));
		read(h_avl, avl_info, sizeof(avl_info));
		close(h_avl);
		is_support_full_func = (avl_info[0] == '1') ? TRUE : FALSE;
	}
#endif
	if (is_support_full_func == FALSE) {
		USIZE sz = {0};
		if (MaxLinkInfo.MaxImgLink > 1) {
			DBG_FATAL("NT96561 only support 1 ImgLink, but set to %d\r\n", MaxLinkInfo.MaxImgLink);
			result = FALSE;
		}
		if (MaxLinkInfo.MaxEthCamLink > 0) {
			DBG_FATAL("NT96561 only support 0 EthCamLink, but set to %d\r\n", MaxLinkInfo.MaxEthCamLink);
			result = FALSE;
		}
		if (MainIMECropInfo[0].IMEWin.w && MainIMECropInfo[0].IMEWin.h) {
			sz.w = MainIMECropInfo[0].IMEWin.w;
			sz.h = MainIMECropInfo[0].IMEWin.h;
		} else {
			sz.w = ImgLinkRecInfo[0][0].size.w;
			sz.h = ImgLinkRecInfo[0][0].size.h;
		}
		if (sz.w > 1920 || sz.h > 1080) {
			DBG_FATAL("NT96561 only supports up to FHD encode in main, but set main to (%d,%d)\r\n", sz.w, sz.h);
			result = FALSE;
		}
		if (CloneIMECropInfo[0].IMEWin.w && CloneIMECropInfo[0].IMEWin.h) {
			sz.w = CloneIMECropInfo[0].IMEWin.w;
			sz.h = CloneIMECropInfo[0].IMEWin.h;
		} else {
			sz.w = ImgLinkRecInfo[0][1].size.w;
			sz.h = ImgLinkRecInfo[0][1].size.h;
		}
		if (sz.w > MAX_REC_WIDTH || sz.h > MAX_REC_HEIGHT) {
			DBG_FATAL("NT96561 only supports up to FHD encode in clone, but set clone to (%d,%d)\r\n", sz.w, sz.h);
			result = FALSE;
		}
		sz.w = WifiLinkStrmInfo[0].size.w;
		sz.h = WifiLinkStrmInfo[0].size.h;
		if (sz.w > MAX_WIFI_WIDTH || sz.h > MAX_WIFI_HEIGHT) {
			DBG_FATAL("NT96561 only supports up to HD encode in wifi, but set wifi to (%d,%d)\r\n", sz.w, sz.h);
			result = FALSE;
		}
	}
#endif
	return result;
}

ER ImageApp_MovieMulti_Open(void)
{
	if (avl_check_available(__xstring(__section_name__)) != TRUE) {
		DBG_FATAL("This chip does not support ImageApp_MovieMulti function.\r\n");
		vos_util_delay_ms(100);         // add delay to show dbg_fatal message in linux
		vos_debug_halt();
	}

	if (_limited_func_check() == FALSE) {
		DBG_FATAL("This chip only support partial function.\r\n");
		vos_util_delay_ms(100);         // add delay to show dbg_fatal message in linux
		vos_debug_halt();
	}

#if 0//defined(_BSP_NA51089_)
	if (EthCamRxFuncEn && (avl_check_available("ethcam_rx")) != TRUE) {
		DBG_FATAL("This chip does not support EthCam Rx function.\r\n");
		vos_util_delay_ms(100);         // add delay to show dbg_fatal message in linux
		vos_debug_halt();
	}
#endif

	if (IsMovieMultiOpened == TRUE) {
		DBG_ERR("ImageApp_MovieMulti is already opened.\r\n");
		return E_SYS;
	}

	if (!EthCamRxFuncEn && MaxLinkInfo.MaxEthCamLink) {
		DBG_WRN("EthCamRx function is disable but MaxEthCamLink is set(%d), forced set to 0\r\n", MaxLinkInfo.MaxEthCamLink);
		MaxLinkInfo.MaxEthCamLink = 0;
	}

	_imageaapp_moviemulti_init();
	_ImageApp_MovieMulti_InstallID();
	ImageApp_Common_Init();
	_ImageApp_MovieMulti_Stream_Open();
	if (iamovie_encrypt_type != MOVIEMULTI_ENCRYPT_TYPE_NONE) {
		_ImageApp_MovieMulti_Crypto_Init();
	}
	vos_flag_set(IAMOVIE_FLG_ID, FLGIAMOVIE_FLOW_LINK_CFG | FLGIAMOVIE_TSK_IDLE_MASK);
	vos_flag_set(IAMOVIE_FLG_ID2, FLGIAMOVIE2_RAWENC_RDY);

	iamovie_need_reopen = TRUE;
	IsMovieMultiOpened = TRUE;

	return E_OK;
}

ER ImageApp_MovieMulti_Close(void)
{
	if (IsMovieMultiOpened == FALSE) {
		DBG_ERR("ImageApp_MovieMulti is already closed.\r\n");
		return E_SYS;
	}
	iamovie_need_reopen = FALSE;
	vos_flag_clr(IAMOVIE_FLG_ID, FLGIAMOVIE_FLOW_LINK_CFG);     // clear before stream close
	if (iamovie_encrypt_type != MOVIEMULTI_ENCRYPT_TYPE_NONE) {
		_ImageApp_MovieMulti_Crypto_Uninit();
	}
	_ImageApp_MovieMulti_Stream_Close();
	vos_flag_clr(IAMOVIE_FLG_ID2, FLGIAMOVIE2_MASK);            // clear after stream close
	_ImageApp_MovieMulti_UninstallID();
	ImageApp_Common_Uninit();
	_imageaapp_moviemulti_uninit();

	IsMovieMultiOpened = FALSE;

	return E_OK;
}

ER ImageApp_MovieMulti_Config(UINT32 config_id, UINT32 value)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	UINT32 i, id, idx, tbl;

	switch (config_id) {
	case MOVIE_CONFIG_RECORD_INFO: {
			if (value != 0) {
				MOVIE_RECODE_INFO *recinfo = (MOVIE_RECODE_INFO *)value;
				i = recinfo->rec_id;
				if ((i < (_CFG_REC_ID_1 + MaxLinkInfo.MaxImgLink)) && (i < (_CFG_REC_ID_1 + MAX_IMG_PATH))) {
					idx = i - _CFG_REC_ID_1;
					tbl = 0;
				} else if ((i >= _CFG_CLONE_ID_1) && (i < (_CFG_CLONE_ID_1 + MaxLinkInfo.MaxImgLink)) && (i < (_CFG_CLONE_ID_1 + MAX_IMG_PATH))) {
					idx = i - _CFG_CLONE_ID_1;
					tbl = 1;
				} else {
					break;
				}
				memcpy((void *)&(ImgLinkRecInfo[idx][tbl]), (void *)value, sizeof(MOVIE_RECODE_INFO));
				IsSetImgLinkRecInfo[idx][tbl] = TRUE;
				ret = E_OK;
			}
			break;
		}

	case MOVIE_CONFIG_STREAM_INFO: {
			if (value != 0) {
				MOVIE_STRM_INFO *strminfo = (MOVIE_STRM_INFO *)value;
				if (strminfo->strm_id >= _CFG_STRM_ID_1 && strminfo->strm_id < (_CFG_STRM_ID_1 + MaxLinkInfo.MaxWifiLink)) {
					id = strminfo->strm_id - _CFG_STRM_ID_1;
					if (id < (sizeof(WifiLinkStrmInfo) / sizeof(MOVIE_STRM_INFO))) {
						memcpy((void *)&(WifiLinkStrmInfo[id]), (void *)value, sizeof(MOVIE_STRM_INFO));
						ret = E_OK;
					}
				}
			}
			break;
		}

	case MOVIE_CONFIG_AUDIO_INFO: {
			if (value != 0) {
				memcpy((void *)&g_AudioInfo, (void *)value, sizeof(MOVIEMULTI_AUDIO_INFO));
				if (g_AudioInfo.aud_ch == _CFG_AUDIO_CH_STEREO && g_AudioInfo.aud_ch_num != 2) {
					DBG_WRN("Set MOVIE_CONFIG_AUDIO_INFO to stereo but ch_num != 2, forced set ch_num = 2\r\n");
					g_AudioInfo.aud_ch_num = 2;
					ret = E_PAR;
				}
#if defined(_BSP_NA51055_)
				else if (((g_AudioInfo.aud_ch == _CFG_AUDIO_CH_LEFT) || (g_AudioInfo.aud_ch == _CFG_AUDIO_CH_RIGHT)) && g_AudioInfo.aud_ch_num != 1) {
					DBG_WRN("Set MOVIE_CONFIG_AUDIO_INFO to mono but set ch_num to %d, forced set ch_num = 1\r\n", g_AudioInfo.aud_ch_num);
					g_AudioInfo.aud_ch_num = 1;
					ret = E_PAR;
				}
#else
				else if (((g_AudioInfo.aud_ch == _CFG_AUDIO_CH_LEFT) || (g_AudioInfo.aud_ch == _CFG_AUDIO_CH_RIGHT)) && ((g_AudioInfo.aud_ch_num != 1) && (g_AudioInfo.aud_ch_num != 2))) {
					DBG_WRN("Set MOVIE_CONFIG_AUDIO_INFO to mono but set ch_num to %d, forced set ch_num = 1\r\n", g_AudioInfo.aud_ch_num);
					g_AudioInfo.aud_ch_num = 1;
					ret = E_PAR;
				}
#endif
				else {
					ret = E_OK;
				}
			}
			break;
		}

	case MOVIE_CONFIG_DISP_INFO: {
			// VdoOut flow refine: pass vdoout path id to ImageApp
			if (value != 0) {
				MOVIEMULTI_CFG_DISP_INFO *ptr = (MOVIEMULTI_CFG_DISP_INFO*) value;
				DispLink[0].vout_p_ctrl  = ptr->vout_ctrl;
				DispLink[0].vout_p[0]    = ptr->vout_path;
				DispLink[0].vout_ratio.w = ptr->ratio.w;
				DispLink[0].vout_ratio.h = ptr->ratio.h;
				DispLink[0].vout_dir     = ptr->dir;
				ret = E_OK;
			}
			break;
		}

	case MOVIE_CONFIG_ALG_INFO: {
			if (value != 0) {
				MOVIE_ALG_INFO *alginfo = (MOVIE_ALG_INFO *)value;
				id = ImageApp_MovieMulti_Recid2ImgLink(alginfo->rec_id);
				if ((id != UNUSED) && (id < (sizeof(ImgLinkAlgInfo) / sizeof(MOVIE_ALG_INFO)))) {
					memcpy((void *)&(ImgLinkAlgInfo[id]), (void *)value, sizeof(MOVIE_ALG_INFO));
					IsSetImgLinkAlgInfo[id] = TRUE;
					ret = E_OK;
				}
			}
			break;
		}

	case MOVIE_CONFIG_MAX_LINK_INFO: {
			if (value != 0) {
				MOVIEMULTI_MAX_LINK_INFO *linkinfo = (MOVIEMULTI_MAX_LINK_INFO *)value;
				if (linkinfo->MaxImgLink <= _ImageApp_MovieMulti_GetMaxImgPath()) {
					MaxLinkInfo.MaxImgLink = linkinfo->MaxImgLink;
				} else {
					DBG_ERR("Set MaxImgLink to %d is exceed MAX_IMG_PATH(%d)\r\n", linkinfo->MaxImgLink, _ImageApp_MovieMulti_GetMaxImgPath());
				}
				if (linkinfo->MaxDispLink <= MAX_DISP_PATH) {
					MaxLinkInfo.MaxDispLink = linkinfo->MaxDispLink;
				} else {
					DBG_ERR("Set MaxDispLink to %d is exceed MAX_DISP_PATH(%d)\r\n", linkinfo->MaxDispLink, MAX_DISP_PATH);
				}
				if (linkinfo->MaxWifiLink <= MAX_WIFI_PATH) {
					MaxLinkInfo.MaxWifiLink = linkinfo->MaxWifiLink;
				} else {
					DBG_ERR("Set MaxWifiLink to %d is exceed MAX_WIFI_PATH(%d)\r\n", linkinfo->MaxWifiLink, MAX_WIFI_PATH);
				}
				if (linkinfo->MaxAudCapLink <= MAX_AUDCAP_PATH) {
					MaxLinkInfo.MaxAudCapLink = linkinfo->MaxAudCapLink;
				} else {
					DBG_ERR("Set MaxAudCapLink to %d is exceed MAX_AUDCAP_PATH(%d)\r\n", linkinfo->MaxAudCapLink, MAX_AUDCAP_PATH);
				}
				if (linkinfo->MaxEthCamLink <= MAX_ETHCAM_PATH) {
					MaxLinkInfo.MaxEthCamLink = linkinfo->MaxEthCamLink;
				} else {
					DBG_ERR("Set MaxEthCamLink to %d is exceed MAX_ETHCAM_PATH(%d)\r\n", linkinfo->MaxEthCamLink, MAX_ETHCAM_PATH);
				}
				g_ia_moviemulti_is_link_init = FALSE;
				DBG_DUMP("MaxLinkInfo = {%d, %d, %d, %d, %d}\r\n", MaxLinkInfo.MaxImgLink, MaxLinkInfo.MaxDispLink, MaxLinkInfo.MaxWifiLink, MaxLinkInfo.MaxAudCapLink, MaxLinkInfo.MaxEthCamLink);
				_update_table();
				ret = E_OK;
			}
			break;
		}

	case MOVIE_CONFIG_MEM_POOL_INFO: {
			if (value != 0) {
				mem_cfg = (HD_COMMON_MEM_INIT_CONFIG *)value;
				ret = E_OK;
			}
			break;
		}

	case MOVIE_CONFIG_SENSOR_INFO: {
			if (value != 0) {
				MOVIE_SENSOR_INFO *seninfo = (MOVIE_SENSOR_INFO *)value;
				UINT32 isp_ver = 0;
				if (vendor_isp_get_common(ISPT_ITEM_VERSION, &isp_ver) == HD_ERR_UNINIT) {
					if ((hd_ret = vendor_isp_init()) != HD_OK) {
						DBG_ERR("vendor_isp_init fail(%d)\n", hd_ret);
					}
				}
				i = seninfo->rec_id;
				if ((i < (_CFG_REC_ID_1 + MaxLinkInfo.MaxImgLink)) && (i < (_CFG_REC_ID_1 + MAX_IMG_PATH))) {
					memcpy((void *)&(img_vcap_cfg[i]), (void *)&(seninfo->vcap_cfg), sizeof(HD_VIDEOCAP_DRV_CONFIG));
					img_vcap_senout_pxlfmt[i] = seninfo->senout_pxlfmt;
					img_vcap_capout_pxlfmt[i] = seninfo->capout_pxlfmt;
					img_vcap_data_lane[i] = seninfo->data_lane;
					ret = E_OK;
					char file_path[64];
					if (strlen(seninfo->file_path) != 0) {
						strncpy(file_path, seninfo->file_path, 63);
					} else {
						strncpy(file_path, "/mnt/app/application.dtb", 63);
					}
					if (strlen(seninfo->ae_path.path) != 0) {
						AET_DTSI_INFO ae_dtsi_info;
						ae_dtsi_info.id = img_sensor_mapping[seninfo->ae_path.id];
						strncpy(ae_dtsi_info.node_path, seninfo->ae_path.path, 31);
						strncpy(ae_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
						ae_dtsi_info.buf_addr = (UINT8 *)seninfo->ae_path.addr;
						if ((hd_ret = vendor_isp_set_ae(AET_ITEM_RLD_DTSI, &ae_dtsi_info)) != HD_OK) {
							DBG_ERR("vendor_isp_set_ae fail(%d)\r\n", hd_ret);
							ret = E_SYS;
						} else {
							ImageApp_MovieMulti_DUMP("vendor_isp_set_ae() OK, id=%d, node=%s, path =%s\r\n", ae_dtsi_info.id, ae_dtsi_info.node_path, ae_dtsi_info.file_path);
						}
					}
					if (strlen(seninfo->awb_path.path) != 0) {
						AWBT_DTSI_INFO awb_dtsi_info;
						awb_dtsi_info.id = img_sensor_mapping[seninfo->awb_path.id];
						strncpy(awb_dtsi_info.node_path, seninfo->awb_path.path, 31);
						strncpy(awb_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
						awb_dtsi_info.buf_addr = (UINT8 *)seninfo->awb_path.addr;
						if ((hd_ret = vendor_isp_set_awb(AWBT_ITEM_RLD_DTSI, &awb_dtsi_info)) != HD_OK) {
							DBG_ERR("vendor_isp_set_awb fail(%d)\r\n", hd_ret);
							ret = E_SYS;
						} else {
							ImageApp_MovieMulti_DUMP("vendor_isp_set_awb() OK, id=%d, node=%s, path =%s\r\n", awb_dtsi_info.id, awb_dtsi_info.node_path, awb_dtsi_info.file_path);
						}
					}
					#if 0
					if (strlen(seninfo->af_path.path) != 0) {
						AFT_DTSI_INFO af_dtsi_info;
						af_dtsi_info.id = img_sensor_mapping[seninfo->af_path.id];
						strncpy(af_dtsi_info.node_path, seninfo->af_path.path, 31);
						strncpy(af_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
						af_dtsi_info.buf_addr = (UINT8 *)seninfo->af_path.addr;
						if ((hd_ret = vendor_isp_set_af(AFT_ITEM_RLD_DTSI, &af_dtsi_info)) != HD_OK) {
							DBG_ERR("vendor_isp_set_af fail(%d)\r\n", hd_ret);
							ret = E_SYS;
						} else {
							ImageApp_MovieMulti_DUMP("vendor_isp_set_af() OK, id=%d, node=%s, path =%s\r\n", af_dtsi_info.id, af_dtsi_info.node_path, af_dtsi_info.file_path);
						}
					}
					#endif
					if (strlen(seninfo->iq_path.path) != 0) {
						IQT_DTSI_INFO iq_dtsi_info;
						iq_dtsi_info.id = img_sensor_mapping[seninfo->iq_path.id];
						strncpy(iq_dtsi_info.node_path, seninfo->iq_path.path, 31);
						strncpy(iq_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
						iq_dtsi_info.buf_addr = (UINT8 *)seninfo->iq_path.addr;
						if ((hd_ret = vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &iq_dtsi_info)) != HD_OK) {
							DBG_ERR("vendor_isp_set_iq fail(%d)\r\n", hd_ret);
							ret = E_SYS;
						} else {
							ImageApp_MovieMulti_DUMP("vendor_isp_set_iq(IQ) OK, id=%d, node=%s, path =%s\r\n", iq_dtsi_info.id, iq_dtsi_info.node_path, iq_dtsi_info.file_path);
						}
					}
					if (strlen(seninfo->iq_shading_path.path) != 0) {
						IQT_DTSI_INFO iq_dtsi_info;
						iq_dtsi_info.id = img_sensor_mapping[seninfo->iq_shading_path.id];
						strncpy(iq_dtsi_info.node_path, seninfo->iq_shading_path.path, 31);
						strncpy(iq_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
						iq_dtsi_info.buf_addr = (UINT8 *)seninfo->iq_shading_path.addr;
						if ((hd_ret = vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &iq_dtsi_info)) != HD_OK) {
							DBG_ERR("vendor_isp_set_iq fail(%d)\r\n", hd_ret);
							ret = E_SYS;
						} else {
							ImageApp_MovieMulti_DUMP("vendor_isp_set_iq(SHADING) OK, id=%d, node=%s, path =%s\r\n", iq_dtsi_info.id, iq_dtsi_info.node_path, iq_dtsi_info.file_path);
						}
					}
					if (strlen(seninfo->iq_dpc_path.path) != 0) {
						IQT_DTSI_INFO iq_dtsi_info;
						iq_dtsi_info.id = img_sensor_mapping[seninfo->iq_dpc_path.id];
						strncpy(iq_dtsi_info.node_path, seninfo->iq_dpc_path.path, 31);
						strncpy(iq_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
						iq_dtsi_info.buf_addr = (UINT8 *)seninfo->iq_dpc_path.addr;
						if ((hd_ret = vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &iq_dtsi_info)) != HD_OK) {
							DBG_ERR("vendor_isp_set_iq fail(%d)\r\n", hd_ret);
							ret = E_SYS;
						} else {
							ImageApp_MovieMulti_DUMP("vendor_isp_set_iq(DPC) OK, id=%d, node=%s, path =%s\r\n", iq_dtsi_info.id, iq_dtsi_info.node_path, iq_dtsi_info.file_path);
						}
					}
					if (strlen(seninfo->iq_ldc_path.path) != 0) {
						IQT_DTSI_INFO iq_dtsi_info;
						iq_dtsi_info.id = img_sensor_mapping[seninfo->iq_ldc_path.id];
						strncpy(iq_dtsi_info.node_path, seninfo->iq_ldc_path.path, 31);
						strncpy(iq_dtsi_info.file_path, file_path, DTSI_NAME_LENGTH);
						iq_dtsi_info.buf_addr = (UINT8 *)seninfo->iq_ldc_path.addr;
						if ((hd_ret = vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &iq_dtsi_info)) != HD_OK) {
							DBG_ERR("vendor_isp_set_iq fail(%d)\r\n", hd_ret);
							ret = E_SYS;
						} else {
							ImageApp_MovieMulti_DUMP("vendor_isp_set_iq(LDC) OK, id=%d, node=%s, path =%s\r\n", iq_dtsi_info.id, iq_dtsi_info.node_path, iq_dtsi_info.file_path);
						}
					}
				}
				if (isp_ver == 0) {
					if ((hd_ret = vendor_isp_uninit()) != HD_OK) {
						DBG_ERR("vendor_isp_uninit fail(%d)\n", hd_ret);
					}
				}
			}
			break;
		}

	case MOVIE_CONFIG_SENSOR_MAPPING: {
			if (value != 0) {
				memcpy((void *)&(img_sensor_mapping[0]), (void *)value, sizeof(UINT32) * _ImageApp_MovieMulti_GetMaxImgPath());
				UINT16 k;
				DBG_DUMP("SensorMapping = {");
				for (k = 0; k < _ImageApp_MovieMulti_GetMaxImgPath(); k++) {
					DBG_DUMP("%d, ", img_sensor_mapping[k]);
				}
				DBG_DUMP("}\r\n");
				//DBG_DUMP("SensorMapping = {%d, %d}\r\n", img_sensor_mapping[0], img_sensor_mapping[1]);
				ret = E_OK;
			}
			break;
		}

	case MOVIE_CONFIG_AD_MAP: {
			if (value != 0) {
				MOVIE_AD_MAP *admap = (MOVIE_AD_MAP *)value;
				i = admap->rec_id;
				if ((i < (_CFG_REC_ID_1 + MaxLinkInfo.MaxImgLink)) && (i < (_CFG_REC_ID_1 + MAX_IMG_PATH))) {
					idx = i - _CFG_REC_ID_1;
					memcpy((void *)&(ad_sensor_map[idx]), (void *)value, sizeof(MOVIE_AD_MAP));
					ret = E_OK;
					DBG_DUMP("ad_sensor_map[%d]=%d, %d, %x\r\n", idx, ad_sensor_map[idx].rec_id, ad_sensor_map[idx].vin_id, ad_sensor_map[idx].vcap_id_bit);
				}
			}

			break;
		}

	case MOVIE_CONFIG_DRAM: {
			if (value != 0) {
				MOVIEMULTI_DRAM_CFG *pdramcfg = (MOVIEMULTI_DRAM_CFG *)value;
				memcpy((void *)&(dram_cfg), (void *)pdramcfg, sizeof(MOVIEMULTI_DRAM_CFG));
				DBG_DUMP("dram_cfg:\r\n");
				DBG_DUMP("  video_cap:");
				for (i = 0; i < _ImageApp_MovieMulti_GetMaxImgPath(); i++) {
					DBG_DUMP("[%d]", dram_cfg.video_cap[i]);
				}
				DBG_DUMP("\r\n");
				DBG_DUMP("  video_proc:");
				for (i = 0; i < _ImageApp_MovieMulti_GetMaxImgPath(); i++) {
					DBG_DUMP("[%d]", dram_cfg.video_proc[i]);
				}
				DBG_DUMP("\r\n");
				DBG_DUMP("  video_encode:[%d]\r\n", dram_cfg.video_encode);
				DBG_DUMP("  video_decode:[%d](N/A)\r\n", dram_cfg.video_decode);
				DBG_DUMP("  video_out   :[%d]\r\n", dram_cfg.video_out);
				ret = E_OK;
			}
			break;
		}
	}
	return ret;
}

static UINT32 _CountBitNo(UINT32 val)
{
	UINT32 temp = val;
	UINT32 cnt = 0;

	while (temp) {
		if (temp & 0x01) {
			cnt ++;
		}
		temp >>= 1;
	}
	return cnt;
}

ER _ImageApp_MovieMulti_CheckEMRMode(void)
{
	UINT32 emr = ImgLinkFileInfo.emr_on;
	UINT32 temp;
	ER ret = E_OK;

	// sync check rule from 67x
	if (0) { //((emr & _CFG_EMR_SET_CRASH) && (emr & (MASK_EMR_MAIN | MASK_EMR_CLONE))) {
		ret = E_SYS;
	} else {
		if ((temp = emr & MASK_EMR_MAIN)) {
			if (_CountBitNo(temp) > 1) {
				ret = E_SYS;
			}
		}
		if ((temp = emr & MASK_EMR_CLONE)) {
			if (_CountBitNo(temp) > 1) {
				ret = E_SYS;
			}
		}
	}
	return ret;
}

ER ImageApp_MovieMulti_FileOption(MOVIE_RECODE_FILE_OPTION *prec_option)
{
	ER ret = E_SYS;

	if (prec_option) {
		memcpy(&ImgLinkFileInfo, prec_option, sizeof(MOVIE_RECODE_FILE_OPTION));

		if ((ret =_ImageApp_MovieMulti_CheckEMRMode()) != E_OK) {
			DBG_ERR("EMR mode conflicted(%x)! Forced set to EMR_OFF\r\n", ImgLinkFileInfo.emr_on);
			ImgLinkFileInfo.emr_on = _CFG_EMR_OFF;
		}
	}
	return ret;
}

UINT32 ImageApp_MovieMulti_Recid2ImgLink(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id;

	if (i < (_CFG_REC_ID_1 + MaxLinkInfo.MaxImgLink)) {
		return (i - _CFG_REC_ID_1);
	} else if ((i >= _CFG_CLONE_ID_1) && (i < (_CFG_CLONE_ID_1 + MaxLinkInfo.MaxImgLink))) {
		return (i - _CFG_CLONE_ID_1);
	}
	return UNUSED;
}

UINT32 ImageApp_MovieMulti_Recid2BsPort(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id;
	UINT32 ret = UNUSED;

	if (i < _CFG_CLONE_ID_MAX) {
		ret = g_ia_moviemulti_recid_to_bsport[i];
	} else if (i >= _CFG_ETHCAM_ID_1 && i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink)) {
		ret = 4 * MaxLinkInfo.MaxImgLink + 2 * (i - _CFG_ETHCAM_ID_1);
	}
	return ret;
}

UINT32 ImageApp_MovieMulti_BsPort2Recid(UINT32 port)
{
	UINT32 i = port;
	UINT32 ret = UNUSED;

	if (i < 16) {
		ret = g_ia_moviemulti_bsport_to_recid[i];
	}
	return ret;
}

UINT32 ImageApp_MovieMulti_BsPort2Imglink(UINT32 port)
{
	UINT32 i = port;
	UINT32 ret = UNUSED;

	if (i < (MaxLinkInfo.MaxImgLink * 4)) {
		ret = i / 4;
	}
	return ret;
}

UINT32 ImageApp_MovieMulti_BsPort2EthCamlink(UINT32 port)
{
	UINT32 i = port;
	UINT32 port_used_by_imglink;
	UINT32 ret = UNUSED;

	port_used_by_imglink = 4 * MaxLinkInfo.MaxImgLink;
	if (i >= port_used_by_imglink && i < (port_used_by_imglink + 2 * MaxLinkInfo.MaxEthCamLink)) {
		ret = (i - port_used_by_imglink) / 2 + _CFG_ETHCAM_ID_1;
	}
	return ret;
}

UINT32 ImageApp_MovieMulti_VePort2Imglink(UINT32 port)
{
	UINT32 i = port;
	UINT32 ret = UNUSED;

	if (i < (MaxLinkInfo.MaxImgLink * 2)) {
		ret = i / 2;
	}
	return ret;
}

void ImageApp_MovieMulti_Root_Path(CHAR *prootpath, UINT32 rec_id)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		if (prootpath == NULL) {
			DBG_ERR("prootpath is NULL");
			return;
		}
		NH_Custom_SetRootPathByPath(ImageApp_MovieMulti_Recid2BsPort(rec_id),prootpath);
		NH_Custom_SetRootPathByPath(ImageApp_MovieMulti_Recid2BsPort(rec_id) + 1,prootpath);
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
	}
}

UINT32 ImageApp_MovieMulti_Folder_Naming(MOVIEMULTI_RECODE_FOLDER_NAMING *pfolder_naming)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		UINT32                   ret             = E_SYS;
		NMC_FOLDER_INFO          nmc_folder_info = {0};
		UINT32                   id;
		MOVIE_TBL_IDX            tb;

		nmc_folder_info.pathid  = ImageApp_MovieMulti_Recid2BsPort(pfolder_naming->rec_id);//pImageStream->rec.fileout_out;
		nmc_folder_info.pMovie  = pfolder_naming->movie;
		nmc_folder_info.pEMR    = pfolder_naming->emr;
		nmc_folder_info.pRO     = pfolder_naming->emr;
		nmc_folder_info.pPhoto  = pfolder_naming->snapshot;
		nmc_folder_info.pEvent1 = NULL;
		nmc_folder_info.pEvent2 = NULL;
		nmc_folder_info.pEvent3 = NULL;

		DBG_IND("nmc_folder_info.pathid %d !\r\n", nmc_folder_info.pathid);
		DBG_IND("nmc_folder_info.pMovie %s !\r\n", nmc_folder_info.pMovie);
		DBG_IND("nmc_folder_info.pEMR %s !\r\n", nmc_folder_info.pEMR);
		DBG_IND("nmc_folder_info.pRO %s !\r\n", nmc_folder_info.pRO);
		DBG_IND("nmc_folder_info.pPhoto %s !\r\n", nmc_folder_info.pPhoto);

		NH_Custom_SetFolderPath(&nmc_folder_info);

		nmc_folder_info.pathid  = ImageApp_MovieMulti_Recid2BsPort(pfolder_naming->rec_id) + 1;//pImageStream->rec.fileout_out;
		nmc_folder_info.pMovie  = pfolder_naming->movie;
		nmc_folder_info.pEMR    = pfolder_naming->emr;
		nmc_folder_info.pRO     = pfolder_naming->emr;
		nmc_folder_info.pPhoto  = pfolder_naming->snapshot;
		nmc_folder_info.pEvent1 = NULL;
		nmc_folder_info.pEvent2 = NULL;
		nmc_folder_info.pEvent3 = NULL;

		DBG_IND("nmc_folder_info.pathid %d !\r\n", nmc_folder_info.pathid);
		DBG_IND("nmc_folder_info.pMovie %s !\r\n", nmc_folder_info.pMovie);
		DBG_IND("nmc_folder_info.pEMR %s !\r\n", nmc_folder_info.pEMR);
		DBG_IND("nmc_folder_info.pRO %s !\r\n", nmc_folder_info.pRO);
		DBG_IND("nmc_folder_info.pPhoto %s !\r\n", nmc_folder_info.pPhoto);

		NH_Custom_SetFolderPath(&nmc_folder_info);

		id  = pfolder_naming->rec_id;

		_ImageApp_MovieMulti_RecidGetTableIndex(id, &tb);

		if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
			g_isf_dvcam_is_file_naming[tb.idx * 2 + tb.tbl] = pfolder_naming->file_naming;
			ret = E_OK;
		} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
			g_isf_dvcam_is_file_naming[2 * MaxLinkInfo.MaxImgLink + tb.idx] = pfolder_naming->file_naming;
			ret = E_OK;
		}
		return ret;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return E_NOSPT;
	}
}

BOOL ImageApp_MovieMulti_isFileNaming(MOVIE_CFG_REC_ID rec_id)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		UINT32 id = rec_id;
		MOVIE_TBL_IDX tb;
		BOOL ret = FALSE;

		_ImageApp_MovieMulti_RecidGetTableIndex(id, &tb);

		if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
	    	ret = g_isf_dvcam_is_file_naming[tb.idx*2+tb.tbl];
		} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
			ret = g_isf_dvcam_is_file_naming[2 * MaxLinkInfo.MaxImgLink + tb.idx];
		}
		return ret;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return FALSE;
	}
}

HD_PATH_ID ImageApp_MovieMulti_GetVcapCtrlPort(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id;
	MOVIE_TBL_IDX tb;
	HD_PATH_ID path = 0;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		path = ImgLink[tb.idx].vcap_p_ctrl;
	}
	return path;
}

HD_PATH_ID ImageApp_MovieMulti_GetVcapPort(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id;
	MOVIE_TBL_IDX tb;
	HD_PATH_ID path = 0;

	if (IsMovieMultiOpened) {
		_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

		if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
			path = ImgLink[tb.idx].vcap_p[0];
		}
	}
	return path;
}

HD_PATH_ID ImageApp_MovieMulti_GetVprcInPort(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, j;
	MOVIE_TBL_IDX tb;
	HD_PATH_ID path = 0;

	if (IsMovieMultiOpened) {
		_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

		if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
			for (j = 0; j < 5; j++) {
				if (_LinkCheckStatus(ImgLinkStatus[tb.idx].vproc_p_phy[0][j])) {
					path = ImgLink[tb.idx].vproc_p_phy[0][j];
					break;
				}
			}
			if (path == 0) {
				DBG_WRN("No active vprc path!\r\n");
			}
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_IMG_PATH)) {
			if (ethcam_vproc_vpe_en[tb.idx]) {
				path = EthCamLink[tb.idx].vproc_p_phy[0];
			} else {
				DBG_WRN("vprc(%d) not support!\r\n", i);
			}
#endif
		} else {
			DBG_WRN("Link(%d) not support!\r\n", i);
		}
	} else {
		DBG_WRN("ImageApp_MovieMulti is not opened!\r\n");
	}
	return path;
}

HD_PATH_ID ImageApp_MovieMulti_GetVprcSplitterInPort(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, j;
	MOVIE_TBL_IDX tb;
	HD_PATH_ID path = 0;

	if (IsMovieMultiOpened) {
		_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

		if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
			for (j = 0; j < 5; j++) {
				if (_LinkCheckStatus(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j])) {
					path = ImgLink[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][j];
					break;
				}
			}
			if (path == 0) {
				DBG_WRN("No active vprc path!\r\n");
			}
		} else {
			DBG_WRN("Link(%d) not support!\r\n", i);
		}
	} else {
		DBG_WRN("ImageApp_MovieMulti is not opened!\r\n");
	}
	return path;
}

HD_PATH_ID ImageApp_MovieMulti_GetVprc3DNRPort(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id;
	MOVIE_TBL_IDX tb;
	HD_PATH_ID path = 0;

	if (IsMovieMultiOpened) {
		_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

		if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
			path = ImgLink[tb.idx].vproc_p_phy[0][HD_GET_OUT(img_vproc_ctrl[tb.idx].ref_path_3dnr) - 1];
		}
	}
	return path;
}

HD_PATH_ID ImageApp_MovieMulti_GetVdoEncPort(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id;
	MOVIE_TBL_IDX tb;
	HD_PATH_ID path = 0;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		path = ImgLink[tb.idx].venc_p[tb.tbl];
	} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
		path = WifiLink[tb.idx].venc_p[0];
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		path = EthCamLink[tb.idx].venc_p[0];
	}
	return path;
}

HD_PATH_ID ImageApp_MovieMulti_GetRawEncPort(MOVIE_CFG_REC_ID id)
{
	UINT32 idx = 0;

	return ImgCapLink[idx].venc_p[0];
}

HD_PATH_ID ImageApp_MovieMulti_GetVprcPort(MOVIE_CFG_REC_ID id, IAMOVIE_VPRC_EX_PATH path)
{
	UINT32 i = id, iport;
	HD_PATH_ID vprc_path = 0;

	if (i >= MaxLinkInfo.MaxImgLink) {
		DBG_ERR("id%d is out of range\r\n", i);
		return vprc_path;;
	}
	if (path >= IAMOVIE_VPRC_EX_MAX) {
		DBG_ERR("path%d is out of range\r\n", path);
		return vprc_path;
	}
	if (IsImgLinkOpened[i] == FALSE) {
		return vprc_path;
	}
	if (img_vproc_no_ext[i] == FALSE) {
		vprc_path = ImgLink[i].vproc_p[img_vproc_splitter[i]][path];
	} else {
		if (gSwitchLink[i][path] == UNUSED) {
			return vprc_path;
		} else {
			iport = gUserProcMap[i][gSwitchLink[i][path]];
		}
		if (iport > 2) {
			iport = 2;
		}
		vprc_path = ImgLink[i].vproc_p_phy[img_vproc_splitter[i]][iport];
	}
	return vprc_path;
}

HD_PATH_ID ImageApp_MovieMulti_GetVprcPhyPort(MOVIE_CFG_REC_ID id, IAMOVIE_VPRC_EX_PATH path)
{
	UINT32 i = id, iport;
	HD_PATH_ID vprc_path = 0;

	if (i >= MaxLinkInfo.MaxImgLink) {
		DBG_ERR("id%d is out of range\r\n", i);
		return vprc_path;;
	}
	if (path >= IAMOVIE_VPRC_EX_MAX) {
		DBG_ERR("path%d is out of range\r\n", path);
		return vprc_path;
	}
	if (IsImgLinkOpened[i] == FALSE) {
		return vprc_path;
	}
	if (gSwitchLink[i][path] == UNUSED) {
		return vprc_path;
	} else {
		iport = gUserProcMap[i][gSwitchLink[i][path]];
	}
	if (iport > 2) {
		iport = 2;
	}
	vprc_path = ImgLink[i].vproc_p_phy[img_vproc_splitter[i]][iport];

	return vprc_path;
}

HD_PATH_ID ImageApp_MovieMulti_GetAlgDataPort(MOVIE_CFG_REC_ID id, MOVIE_CFG_ALG_PATH path)
{
	UINT32 i = id, iport;

	if (i >= MaxLinkInfo.MaxImgLink) {
		DBG_ERR("id%d is out of range\r\n", i);
		return 0;
	}

	if (IsImgLinkOpened[i] == FALSE) {
		return 0;
	}

	if (path == _CFG_ALG_PATH3) {
		if (img_vproc_no_ext[i] == FALSE) {
			return ImgLink[i].vproc_p[img_vproc_splitter[i]][IAMOVIE_VPRC_EX_ALG];
		} else {
			if (gSwitchLink[i][IAMOVIE_VPRC_EX_ALG] == UNUSED) {
				return 0;
			} else {
				iport = gUserProcMap[i][gSwitchLink[i][IAMOVIE_VPRC_EX_ALG]];
			}
			if (iport > 2) {
				iport = 2;
			}
			return ImgLink[i].vproc_p_phy[img_vproc_splitter[i]][iport];
		}
	} else {
		return 0;
	}
	return 0;
}

HD_PATH_ID ImageApp_MovieMulti_GetAudEncPort(MOVIE_CFG_REC_ID id)
{
	UINT32 idx = 0;

	return AudCapLink[idx].aenc_p[0];
}

