#include "ImageApp_MovieMulti_int.h"

///// Cross file variables
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == DISABLE)
UINT32 FirstBsMuxPath = UNUSED;
#endif
UINT32 BsMuxFrmCnt[MAX_BSMUX_PATH] = {0};
///// Noncross file variables
static UINT32 is_crypto_init = FALSE;
#if defined(__FREERTOS)
#else  // defined(__FREERTOS)
static int crypt_fd = -1;
static struct session_op crypt_sess = {0};
static struct crypt_op crypt_cryp = {0};
#endif // defined(__FREERTOS)

// switch sorting functions (for ImageApp_MovieMulti user only)
ER _Switch_Auto_Begin(SW_PORT_TABLE *tbl)
{
	UINT32 o,i;

	if (!tbl) {
		return E_SYS;
	}
	if (tbl->BeginCollect) {
		DBG_ERR("already begin!\r\n");
		return E_SYS;
	}
	tbl->InStart = 0;
	tbl->InEnd = (SWITCH_IN_MAX-1);
	//clear all output ISF_Switch_OUT
	for (o = 0; o < SWITCH_OUT_MAX; o++) {
		tbl->OUT[o].w = 0;
		tbl->OUT[o].h = 0;
		tbl->OUT[o].fmt = 0;
		tbl->OUT[o].frc = 0;
	}
	//clear all input ISF_Switch_IN
	for (i=0; i < SWITCH_IN_MAX; i++) {
		tbl->IN[i].w = 0;
		tbl->IN[i].h = 0;
		tbl->IN[i].fmt = 0;
		tbl->IN[i].frc = 0;
	}
	tbl->BeginCollect = 1;
	tbl->AutoInNum = 0;
	return E_OK;
}

ER _Switch_Auto_End(SW_PORT_TABLE *tbl)
{
	UINT32 o,i;
	if (!tbl) {
		return E_SYS;
	}
	if (!tbl->BeginCollect) {
		DBG_ERR("not begin yet!\r\n");
		return E_SYS;
	}
	if (tbl->AutoInNum >= SWITCH_IN_MAX) {
		DBG_ERR("invalid in num!\r\n");
		return E_SYS;
	}

	for (o = 0; o < SWITCH_OUT_MAX; o++) {
		if ((tbl->OUT[o].w != 0)
		||  (tbl->OUT[o].h != 0)
		||  (tbl->OUT[o].fmt != 0)
		||  (tbl->OUT[o].frc != 0) ) {
			DBG_MSG("out[%d] = (%d, %d, %d, %d)!\r\n", o, tbl->OUT[o].w, tbl->OUT[o].h, tbl->OUT[o].fmt, tbl->OUT[o].frc);
		}
	}
	//do insert sorting
	for (o = 0; o < SWITCH_OUT_MAX; o++) {
		if ((tbl->OUT[o].w != 0)
		||  (tbl->OUT[o].h != 0)
		||  (tbl->OUT[o].fmt != 0)
		||  (tbl->OUT[o].frc != 0) )
		{
			if (tbl->AutoInNum == 0) {
				//replace to OUT at first IN
				tbl->IN[0] = tbl->OUT[o];
				tbl->AutoInNum++;

			} else if(tbl->AutoInNum > 0) {

				UINT32 iFind = 0xffff; //0xff = drop
				UINT32 oFind = 0xffff; //0xff = drop
				//Scan all OUT
				//if IN equal to OUT => same to IN, drop OUT
				//if IN less to OUT => insert OUT before this IN
				for (i=tbl->AutoInNum-1; (iFind == 0xffff); i--) {  //from ISF_Switch_AutoInNum-1 to 0
					if (tbl->OUT[o].w > tbl->IN[i].w) {
						oFind = i; //insert OUT[o] before IN[i]
					} else if (tbl->OUT[o].w == tbl->IN[i].w) {
						if (tbl->OUT[o].h > tbl->IN[i].h) {
							oFind = i; //insert OUT[o] before IN[i]
						} else if (tbl->OUT[o].h == tbl->IN[i].h) {
							if (tbl->OUT[o].fmt > tbl->IN[i].fmt) {
								oFind = i; //insert OUT[o] before IN[i]
							} else if (tbl->OUT[o].fmt == tbl->IN[i].fmt) {
								if (tbl->OUT[o].frc > tbl->IN[i].frc) {
									oFind = i; //insert OUT[o] before IN[i]
								} else if (tbl->OUT[o].frc == tbl->IN[i].frc) {
									iFind = 0xff; //mark: same to IN[i], drop OUT[o]!
								}  else {
									if(oFind != 0xffff) {
										iFind = oFind;//mark: insert OUT[o] before IN[i]
									}
								}
							}  else {
								if(oFind != 0xffff) {
									iFind = oFind;//mark: insert OUT[o] before IN[i]
								}
							}
						} else {
							if(oFind != 0xffff) {
								iFind = oFind;//mark: insert OUT[o] before IN[i]
							}
						}
					} else {
						if(oFind != 0xffff) {
							iFind = oFind;//mark: insert OUT[o] before IN[i]
						}
					}
					if (i==0) {
						if (iFind == 0xffff) {
							if (oFind == 0) {
								iFind = 0;      // if larger than all existed, insert to first
							} else {
								iFind = 0xfffe; //cannot not found any large or equal, just append
							}
						}
						break; //quit this for loop
					}
				}
				if (iFind == 0xfffe) {
					//do: append OUT[o] at last IN[i]
					if (tbl->AutoInNum+1 > SWITCH_IN_MAX) {
						DBG_ERR("spec is out of input number %d!\r\n", SWITCH_IN_MAX);
						return E_SYS;
					}
					tbl->IN[tbl->AutoInNum] = tbl->OUT[o];
					tbl->AutoInNum++;
				} else if (iFind < SWITCH_IN_MAX) {
					BOOL bStop = FALSE;
					//do: insert OUT[o] before IN[j]
					if (tbl->AutoInNum+1 > SWITCH_IN_MAX) {
						DBG_ERR("spec is out of input number %d!\r\n", SWITCH_IN_MAX);
						return E_SYS;
					}
					//DBG_DUMP("iFind=%d\r\n", iFind);
					for (i=tbl->AutoInNum; !bStop; i--) {  //move iFind+1 to tbl->AutoInNum-1 to next
						//DBG_DUMP("copy in[%d] to in[%d]\r\n", i-1, i);
						tbl->IN[i] = tbl->IN[i-1];
						if ((i-1) == iFind) {
							bStop = TRUE;
						}
					}
					tbl->IN[iFind] = tbl->OUT[o]; //insert
					tbl->AutoInNum++;
				}
			}
		}
	}
	tbl->BeginCollect = 0;
	return E_OK;
}

UINT32 _Switch_Auto_GetConnect(SW_PORT_TABLE *tbl, UINT32 o)
{
	UINT32 i, jFind = UNUSED;
	if (!tbl) {
		return UNUSED;
	}
	if (!tbl->AutoInNum) {
		tbl->BeginCollect = 0;
		return UNUSED;
	}
	if (o >= SWITCH_OUT_MAX) {
		return UNUSED;
	}

	tbl->InStart = 0;
	tbl->InEnd = SWITCH_IN_MAX - 1;
	DBG_MSG("Switch.ctrl scan in range %d~%d\r\n", tbl->InStart, tbl->InEnd);

	//do auto connect
	if ((tbl->OUT[o].w != 0)
	|| (tbl->OUT[o].h != 0)
	|| (tbl->OUT[o].fmt != 0)
	|| (tbl->OUT[o].frc != 0) ) {
		for (i = tbl->InStart; i <= tbl->InEnd; i++) {
			DBG_MSG("scan: out[%d](%d, %d, %d, %d) --- in[%d](%d, %d, %d, %d) : \r\n",
				o, tbl->OUT[o].w, tbl->OUT[o].h, tbl->OUT[o].fmt, tbl->OUT[o].frc,
				i, tbl->IN[i].w, tbl->IN[i].h, tbl->IN[i].fmt, tbl->IN[i].frc);
			if ((tbl->OUT[o].w == tbl->IN[i].w)
			&&  (tbl->OUT[o].h == tbl->IN[i].h)
			&&  (tbl->OUT[o].fmt == tbl->IN[i].fmt)
			&&  (tbl->OUT[o].frc == tbl->IN[i].frc)) {
				jFind = i;
				DBG_MSG("MATCH!\r\n");
				break;
			} else {
				DBG_MSG("x\r\n");
			}
		}
	}

	if(jFind != UNUSED) {
		DBG_MSG("Switch.ctrl Connet oPort %d iPort %d\r\n", o, jFind);
	}
	return jFind;
}

#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == DISABLE)
UINT32 _ImageApp_MovieMulti_GetFirstBsMuxPort(void)
{
	UINT32 i, j, firstport = UNUSED;

	if (firstport == UNUSED) {
		for (i = 0; i < MaxLinkInfo.MaxImgLink; i++) {
			for (j = 0; j < 4; j++) {
				if (ImgLinkStatus[i].bsmux_p[j]) {
					firstport = i * 4 + j;
					return firstport;
				}
			}
		}
	}
	if (firstport == UNUSED) {
		for (i = 0; i < MaxLinkInfo.MaxEthCamLink; i++) {
			for (j = 0; j < 2; j++) {
				if (EthCamLinkStatus[i].bsmux_p[j]) {
					firstport = MaxLinkInfo.MaxImgLink * 4 + i * 2 + j;
					return firstport;
				}
			}
		}
	}
	return firstport;
}
#endif

UINT32 _ImageApp_MovieMulti_IsRecording(void)
{
	UINT32 i, rec_running = FALSE;

	for (i = 0; i < MaxLinkInfo.MaxImgLink; i++) {
		if (ImageApp_MovieMulti_IsStreamRunning(_CFG_REC_ID_1 + i)) {
			rec_running = TRUE;
			break;
		}
		if (ImageApp_MovieMulti_IsStreamRunning(_CFG_CLONE_ID_1 + i)) {
			rec_running = TRUE;
			break;
		}
	}
	for (i = 0; i < MaxLinkInfo.MaxEthCamLink; i++) {
		if (ImageApp_MovieMulti_IsStreamRunning(_CFG_ETHCAM_ID_1 + i)) {
			rec_running = TRUE;
			break;
		}
	}
	return rec_running;
}

UINT32 _ImageApp_MovieMulti_frc(UINT32 dst_fr, UINT32 src_fr)
{
	UINT32 frc = 0;

	if (dst_fr == src_fr) {
		frc = HD_VIDEO_FRC_RATIO(1, 1);
	} else {
		frc = HD_VIDEO_FRC_RATIO(dst_fr, src_fr);
	}
	return frc;
}

BOOL _ImageApp_MovieMulti_CheckProfileLevelValid(MOVIE_CFG_CODEC_INFO *pinfo)
{
	if (pinfo == NULL) {
		return FALSE;
	}
	if (pinfo->codec == _CFG_CODEC_MJPG) {
		return FALSE;
	} else if (pinfo->codec == _CFG_CODEC_H264) {
		if ((pinfo->profile == _CFG_H264_PROFILE_BASELINE) ||
			(pinfo->profile == _CFG_H264_PROFILE_MAIN) ||
			(pinfo->profile == _CFG_H264_PROFILE_HIGH)) {
			// do check level
		} else {
			return FALSE;
		}
		if ((pinfo->level == _CFG_H264_LEVEL_1)   ||
			(pinfo->level == _CFG_H264_LEVEL_1_1) ||
			(pinfo->level == _CFG_H264_LEVEL_1_2) ||
			(pinfo->level == _CFG_H264_LEVEL_1_3) ||
			(pinfo->level == _CFG_H264_LEVEL_2)   ||
			(pinfo->level == _CFG_H264_LEVEL_2_1) ||
			(pinfo->level == _CFG_H264_LEVEL_2_2) ||
			(pinfo->level == _CFG_H264_LEVEL_3)   ||
			(pinfo->level == _CFG_H264_LEVEL_3_1) ||
			(pinfo->level == _CFG_H264_LEVEL_3_2) ||
			(pinfo->level == _CFG_H264_LEVEL_4)   ||
			(pinfo->level == _CFG_H264_LEVEL_4_1) ||
			(pinfo->level == _CFG_H264_LEVEL_4_2) ||
			(pinfo->level == _CFG_H264_LEVEL_5)   ||
			(pinfo->level == _CFG_H264_LEVEL_5_1)) {
			return TRUE;
		} else {
			return FALSE;
		}
	} else if (pinfo->codec == _CFG_CODEC_H265) {
		if ((pinfo->profile == _CFG_H265_PROFILE_MAIN)) {
			// do check level
		} else {
			return FALSE;
		}
		if ((pinfo->level == _CFG_H265_LEVEL_1)   ||
			(pinfo->level == _CFG_H265_LEVEL_2)   ||
			(pinfo->level == _CFG_H265_LEVEL_2_1) ||
			(pinfo->level == _CFG_H265_LEVEL_3)   ||
			(pinfo->level == _CFG_H265_LEVEL_3_1) ||
			(pinfo->level == _CFG_H265_LEVEL_4)   ||
			(pinfo->level == _CFG_H265_LEVEL_4_1) ||
			(pinfo->level == _CFG_H265_LEVEL_5)   ||
			(pinfo->level == _CFG_H265_LEVEL_5_1) ||
			(pinfo->level == _CFG_H265_LEVEL_5_2) ||
			(pinfo->level == _CFG_H265_LEVEL_6)   ||
			(pinfo->level == _CFG_H265_LEVEL_6_1) ||
			(pinfo->level == _CFG_H265_LEVEL_6_2)) {
			return TRUE;
		} else {
			return FALSE;
		}
	}
	return FALSE;
}

static char *_enmsg(UINT32 en)
{
	static char *enmsg[2] = {
		"DISABLE",
		"ENABLE",
	};
	return en? enmsg[1] : enmsg[0];
}

ER _ImageApp_MovieMulti_CheckOpenSetting(void)
{
	ER ret = E_OK;
	UINT32 idx;

	for (idx = 0; idx < MaxLinkInfo.MaxImgLink; idx++) {
		// check dis function setting
		if (img_vproc_dis_func[idx] && ((img_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT) || img_vproc_yuv_compress[idx])) {
			DBG_ERR("ImgLink[%d]: DIS (%s) is exclusive of direct mode(%s) and YUV compress(%s)\r\n", idx, _enmsg(img_vproc_dis_func[idx]), _enmsg(img_vcap_func_cfg[idx].out_func == HD_VIDEOCAP_OUTFUNC_DIRECT), _enmsg(img_vproc_yuv_compress[idx]));
			return E_SYS;
		}
	}
	return ret;
}

UINT32 _ImageApp_MovieMulti_GetMaxImgPath(void)
{
	UINT32 max_path;

#if defined(_BSP_NA51055_) && !defined(__FREERTOS) && (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
	char  *chip_name = getenv("NVT_CHIP_ID");

	if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51084") == 0) {
		max_path = MAX_IMG_PATH;
	} else {
		max_path = MAX_IMG_PATH_NA51055;
	}
#else
	max_path = MAX_IMG_PATH;
#endif

	return max_path;
}

ER _ImageApp_MovieMulti_Crypto_Init(void)
{
	ER ret = E_SYS;

	if (is_crypto_init == TRUE) {
		DBG_ERR("Crypto already init\r\n");
		return ret;
	}

#if defined(__FREERTOS)
	if ((ret = crypto_open()) != E_OK) {
		DBG_ERR("crypto_open() fail\r\n");
	} else {
		crypto_setConfig(CRYPTO_CONFIG_ID_MODE, (iamovie_encrypt_mode == MOVIEMULTI_ENCRYPT_MODE_AES128) ? CRYPTO_MODE_AES128 : CRYPTO_MODE_AES256);
		crypto_setConfig(CRYPTO_CONFIG_ID_TYPE, CRYPTO_TYPE_ENCRYPT);
		crypto_setKey(iamovie_encrypt_key);
		is_crypto_init = TRUE;
	}
#else  // defined(__FREERTOS)
    struct stat cryptodev = {0};
	// check /dev/crypto
	if (stat("/dev/crypto", &cryptodev) == 0) {
		// open /dev/crypto
		if ((crypt_fd = open("/dev/crypto", O_RDWR, 0) ) < 0) {
			DBG_ERR("open(/dev/crypto) fail\r\n");
		} else {
			// create session
			crypt_sess.cipher = CRYPTO_AES_ECB;
			crypt_sess.keylen = (iamovie_encrypt_mode == MOVIEMULTI_ENCRYPT_MODE_AES128)? 16 : 32;
			crypt_sess.key = (void*)iamovie_encrypt_key;
			if (ioctl(crypt_fd, CIOCGSESSION, &crypt_sess)) {
				DBG_ERR("ioctl(CIOCGSESSION) fail\r\n");
			} else {
				is_crypto_init = TRUE;
				ret = E_OK;
			}
		}
	} else {
		DBG_ERR("/dev/crypto not exist\r\n");
	}
#endif // defined(__FREERTOS)
	return ret;
}

ER _ImageApp_MovieMulti_Crypto_Uninit(void)
{
	ER ret = E_OK;

	if (is_crypto_init == FALSE) {
		DBG_ERR("Crypto already uninit\r\n");
		return E_SYS;
	}

#if defined(__FREERTOS)
	if ((ret = crypto_close()) != E_OK) {
		DBG_ERR("crypto_close() fail\r\n");
	}
#else  // defined(__FREERTOS)
	// revoke session
	if (ioctl(crypt_fd, CIOCFSESSION, &(crypt_sess.ses))) {
		DBG_ERR("ioctl(CIOCFSESSION) fail\r\n");
		ret = E_SYS;
	}
	// close device
	if (close(crypt_fd)) {
		DBG_ERR("close(crypt_fd) fail\r\n");
		ret = E_SYS;
	}
#endif // defined(__FREERTOS)

	if(ret == E_OK)
		is_crypto_init = FALSE;

	return ret;
}

ER _ImageApp_MovieMulti_Crypto(UINT32 addr, UINT32 size)
{
	ER ret = E_SYS;

	if (is_crypto_init == FALSE) {
		DBG_ERR("Crypto not open\r\n");
		return ret;
	}
#if defined(__FREERTOS)
	UINT32 i, loop;
	UINT8 *ptr = (UINT8 *)addr;

	vos_sem_wait(IAMOVIE_SEMID_CRYPTO);
	loop = ALIGN_CEIL_16(size) / 16;
	for (i = 0; i < loop; i++) {
		crypto_setInput(ptr + 16 * i);
		crypto_pio_enable();
		crypto_getOutput(ptr + 16 * i);
	}
	vos_sem_sig(IAMOVIE_SEMID_CRYPTO);
	ret = E_OK;
#else  // defined(__FREERTOS)
	vos_sem_wait(IAMOVIE_SEMID_CRYPTO);
	crypt_cryp.ses = crypt_sess.ses;
	crypt_cryp.len = size;
	crypt_cryp.src = (void*)addr;
	crypt_cryp.dst = (void*)addr;
	crypt_cryp.iv = NULL;
	crypt_cryp.op = COP_ENCRYPT;
	if (ioctl(crypt_fd, CIOCCRYPT, &crypt_cryp)) {
		DBG_ERR("ioctl(CIOCCRYPT)\r\n");
	} else {
		ret = E_OK;
	}
	vos_sem_sig(IAMOVIE_SEMID_CRYPTO);
#endif // defined(__FREERTOS)
	return ret;
}

HD_RESULT _ImageApp_MovieMulti_UpdateVprcDepth(HD_PATH_ID path, UINT32 depth)
{
	HD_RESULT hd_ret = HD_OK;
	HD_VIDEOPROC_OUT video_out_param = {0};

	if ((hd_ret = hd_videoproc_get(path, HD_VIDEOPROC_PARAM_OUT, &video_out_param)) != HD_OK) {
		DBG_ERR("Get HD_VIDEOPROC_PARAM_OUT fail(%d)\r\n", hd_ret);
		return hd_ret;
	}
	video_out_param.depth = depth;
	if ((hd_ret = hd_videoproc_set(path, HD_VIDEOPROC_PARAM_OUT, &video_out_param)) != HD_OK) {
		DBG_ERR("Set HD_VIDEOPROC_PARAM_OUT fail(%d)\r\n", hd_ret);
	}
	return hd_ret;
}

// public functions
UINT32 ImageApp_MovieMulti_IsStreamRunning(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, ret = FALSE;
	MOVIE_TBL_IDX tb;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if (img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3) {
			ret = _LinkCheckStatus(ImgLinkStatus[tb.idx].venc_p[tb.tbl]);
		} else {
			if (img_bsmux_2v1a_mode[tb.idx] && tb.tbl == MOVIETYPE_CLONE) {
				ret = (_LinkCheckStatus(ImgLinkStatus[tb.idx].bsmux_p[2]) || _LinkCheckStatus(ImgLinkStatus[tb.idx].bsmux_p[3])) ? TRUE : FALSE;
			} else {
				ret = (_LinkCheckStatus(ImgLinkStatus[tb.idx].fileout_p[2*tb.tbl+0]) || _LinkCheckStatus(ImgLinkStatus[tb.idx].fileout_p[2*tb.tbl+1])) ? TRUE : FALSE;
			}
		}
	} else if (((tb.link_type == LINKTYPE_STRM) || (tb.link_type == LINKTYPE_UVAC)) && (tb.idx < MAX_WIFI_PATH)) {
		ret = _LinkCheckStatus(WifiLinkStatus[tb.idx].venc_p[0]);
	} else if ((tb.link_type == LINKTYPE_DISP) && (tb.idx < MAX_DISP_PATH)) {
		ret = _LinkCheckStatus(DispLinkStatus[tb.idx].vout_p[0]);
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		ret = (_LinkCheckStatus(EthCamLinkStatus[tb.idx].fileout_p[0]) || _LinkCheckStatus(EthCamLinkStatus[tb.idx].fileout_p[1])) ? TRUE : FALSE;
	} else if ((tb.link_type == LINKTYPE_ETHCAM_TX) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		ret = _LinkCheckStatus(ImgLinkStatus[tb.idx].venc_p[tb.tbl]);
	} else if ((tb.link_type == LINKTYPE_AUDCAP) && (tb.idx < MAX_AUDCAP_PATH)) {
		ret = _LinkCheckStatus(AudCapLinkStatus[tb.idx].aenc_p[0]);
	} else {
		DBG_WRN("id%d is out of range.\r\n", i);
		ret = FALSE;
	}
	return ret;
}

UINT32 ImageApp_MovieMulti_GetFreeRec(HD_BSMUX_CALC_SEC *pSetting)
{
	hd_bsmux_get(ImgLink[0].bsmux_p[0], HD_BSMUX_PARAM_CALC_SEC, pSetting);

	return pSetting->calc_sec;
}

UINT32 ImageApp_MovieMulti_GetRemainSize(CHAR drive, UINT64 *p_size)
{
#if 0
	if (iamovie_use_filedb_and_naming == TRUE) {
		iamoviemulti_fm_get_remain_size(drive, p_size);
	} else {
		*p_size = 0;
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
	}
#else
	iamoviemulti_fm_get_remain_size(drive, p_size);
#endif
	return 0;
}

UINT32 ImageApp_MovieMulti_SetReservedSize(CHAR drive, UINT64 size, UINT32 is_loop)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		iamoviemulti_fm_set_resv_size(drive, size, is_loop);
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
	}
	return 0;
}

UINT32 ImageApp_MovieMulti_SetExtendCrashInfo(MOVIE_CFG_REC_ID id, UINT32 unit, UINT32 max_num, UINT32 enable)
{
	HD_RESULT hd_ret;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;
	HD_BSMUX_EXTINFO setting = {0};
	setting.unit    = unit;
	setting.max_num = max_num;
	setting.enable  = enable;
	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);
	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if (IsImgLinkOpened[tb.idx] == TRUE) {
			if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[2*tb.tbl], HD_BSMUX_PARAM_EXTINFO, (void*)&setting)) != HD_OK) {
				DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EXTINFO) fail(%d)\n", hd_ret);
			}
		}
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		if (is_ethcamlink_opened[tb.idx] == TRUE) {
			if ((hd_ret = hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[0], HD_BSMUX_PARAM_EXTINFO, (void*)&setting)) != HD_OK) {
				DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EXTINFO) fail(%d)\n", hd_ret);
			}
		}
	}
	return 0;
}

UINT32 ImageApp_MovieMulti_IsExtendCrash(MOVIE_CFG_REC_ID id)
{
	HD_RESULT hd_ret;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;
	HD_BSMUX_EXTINFO setting = {0};
	HD_PATH_ID bsmux_ctrl_path = 0;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);
	if (i == _CFG_CTRL_ID) {
		if ((hd_ret = bsmux_get_ctrl_path(0, &bsmux_ctrl_path)) != HD_OK) {
			DBG_ERR("bsmux_get_ctrl_path() fail(%d)\n", hd_ret);
		}
		if ((hd_ret = hd_bsmux_get(bsmux_ctrl_path, HD_BSMUX_PARAM_EXTINFO, (void*)&setting)) != HD_OK) {
			DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EXTINFO) fail(%d)\n", hd_ret);
		}
	} else if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if (IsImgLinkOpened[tb.idx] == TRUE) {
			if ((hd_ret = hd_bsmux_get(ImgLink[tb.idx].bsmux_p[2*tb.tbl], HD_BSMUX_PARAM_EXTINFO, (void*)&setting)) != HD_OK) {
				DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EXTINFO) fail(%d)\n", hd_ret);
			}
		}
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		if (is_ethcamlink_opened[tb.idx] == TRUE) {
			if ((hd_ret = hd_bsmux_get(EthCamLink[tb.idx].bsmux_p[0], HD_BSMUX_PARAM_EXTINFO, (void*)&setting)) != HD_OK) {
				DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EXTINFO) fail(%d)\n", hd_ret);
			}
		}
	}
	return setting.enable;
}

ER ImageApp_MovieMulti_SetExtendCrash(MOVIE_CFG_REC_ID id)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;
	HD_BSMUX_TRIG_EVENT event = {0};
	HD_PATH_ID bsmux_ctrl_path = 0;

	event.type = HD_BSMUX_TRIG_EVENT_EXTEND;
	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);
	if (i == _CFG_CTRL_ID) {
		if ((hd_ret = bsmux_get_ctrl_path(0, &bsmux_ctrl_path)) != HD_OK) {
			DBG_ERR("bsmux_get_ctrl_path() fail(%d)\n", hd_ret);
			ret = E_SYS;
		} else {
			if ((hd_ret = hd_bsmux_set(bsmux_ctrl_path, HD_BSMUX_PARAM_TRIG_EVENT, (void*)&event)) != HD_OK) {
				DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EXTINFO) fail(%d)\n", hd_ret);
				ret = E_SYS;
			} else {
				ret = event.ret_val;
			}
		}
	} else if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if (IsImgLinkOpened[tb.idx] == TRUE) {
			if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[2*tb.tbl], HD_BSMUX_PARAM_TRIG_EVENT, (void*)&event)) != HD_OK) {
				DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EXTINFO) fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
		}
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		if (is_ethcamlink_opened[tb.idx] == TRUE) {
			if ((hd_ret = hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[0], HD_BSMUX_PARAM_TRIG_EVENT, (void*)&event)) != HD_OK) {
				DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EXTINFO) fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
		}
	} else {
		ret = E_PAR;
	}
	return ret;
}

ER ImageApp_MovieMulti_SetCutNow(MOVIE_CFG_REC_ID id)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;
	HD_BSMUX_TRIG_EVENT event = {0};

	event.type = HD_BSMUX_TRIG_EVENT_CUTNOW;
	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);
	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if (IsImgLinkOpened[tb.idx] == TRUE) {
			if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[2*tb.tbl], HD_BSMUX_PARAM_TRIG_EVENT, (void*)&event)) != HD_OK) {
				DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EXTINFO) fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
		}
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		if (is_ethcamlink_opened[tb.idx] == TRUE) {
			if ((hd_ret = hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[0], HD_BSMUX_PARAM_TRIG_EVENT, (void*)&event)) != HD_OK) {
				DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EXTINFO) fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
		}
	} else {
		ret = E_PAR;
	}
	return ret;
}

ER ImageApp_MovieMulti_SetCutNowWithPeriod(MOVIE_CFG_REC_ID id, UINT32 sec, UINT32 protected)
{
	ER ret = E_OK;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;
	UINT32 crash_idx;
	MOVIE_CB_CRASH_INFO *p_cinfo;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);
	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if (IsImgLinkOpened[tb.idx] == TRUE) {
			if ((ret = ImageApp_MovieMulti_SetCutNow(i)) != E_OK) {
				DBG_ERR("ImageApp_MovieMulti_SetCutNow failed(%d)\r\n", ret);
			} else {
				img_bsmux_cutnow_with_period_cnt[tb.idx][tb.tbl * 2] = ImgLinkRecInfo[tb.idx][tb.tbl].frame_rate * sec;
				DBG_DUMP("Set %d target cnt = %d\r\n", i, img_bsmux_cutnow_with_period_cnt[tb.idx][tb.tbl * 2]);
				if (iamovie_use_filedb_and_naming == TRUE) {
					crash_idx = tb.idx * 2 + tb.tbl;
					p_cinfo = _MovieMulti_Get_CrashInfo(crash_idx);
					p_cinfo->is_crash_curr = 0;
					p_cinfo->is_crash_prev = 0;
					p_cinfo->is_crash_next = protected ? 1 : 0;
				}
			}
		}
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		if (is_ethcamlink_opened[tb.idx] == TRUE) {
			if ((ret = ImageApp_MovieMulti_SetCutNow(i)) != E_OK) {
				DBG_ERR("ImageApp_MovieMulti_SetCutNow failed(%d)\r\n", ret);
			} else {
				ethcam_bsmux_cutnow_with_period_cnt[tb.idx][0] = EthCamLinkRecInfo[tb.idx].vfr * sec;
				DBG_DUMP("Set %d target cnt = %d\r\n", i, ethcam_bsmux_cutnow_with_period_cnt[tb.idx][0]);
				if (iamovie_use_filedb_and_naming == TRUE) {
					crash_idx = MaxLinkInfo.MaxImgLink * 2 + tb.idx;
					p_cinfo = _MovieMulti_Get_CrashInfo(crash_idx);
					p_cinfo->is_crash_curr = 0;
					p_cinfo->is_crash_prev = 0;
					p_cinfo->is_crash_next = protected ? 1 : 0;
				}
			}
		}
	} else {
		ret = E_NOSPT;
	}
	return ret;
}

ER ImageApp_MovieMulti_GetDropCnt(MOVIE_CFG_REC_ID id, MOVIEMULTI_BS_DROP_CNT *pcnt)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;
    HD_BSMUX_EN_UTIL util = {0};

	if (pcnt) {
		memset(pcnt, 0, sizeof(MOVIEMULTI_BS_DROP_CNT));
		if (IsMovieMultiOpened == FALSE) {
			DBG_WRN("ImageApp_MovieMulti is not opened\r\n");
			return ret;
		}
		_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);
		if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
			if (_LinkCheckStatus(ImgLinkStatus[tb.idx].bsmux_p[2*tb.tbl])) {
				util.type = HD_BSMUX_EN_UTIL_EN_DROP;
			    if ((hd_ret = hd_bsmux_get(ImgLink[tb.idx].bsmux_p[2*tb.tbl], HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
					DBG_ERR("hd_bsmux_get(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
				} else {
					pcnt->en = util.enable;
					pcnt->cnt1 = util.resv[0];
					pcnt->cnt2 = util.resv[1];
					ret = E_OK;
				}
			} else {
				DBG_WRN("bsmux port(%x) is not in running state\r\n", ImgLinkStatus[tb.idx].bsmux_p[2*tb.tbl]);
			}
		}
	} else {
		DBG_ERR("pcnt = NULL!\r\n");
	}
	return ret;
}

ER ImageApp_MovieMulti_GetAudEncBufInfo(MOVIE_CFG_REC_ID id, HD_AUDIOENC_BUFINFO *pinfo)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	UINT32 idx = 0;

	if (pinfo) {
		memset(pinfo, 0, sizeof(HD_AUDIOENC_BUFINFO));
		if ((hd_ret = hd_audioenc_get(AudCapLink[idx].aenc_p[0], HD_AUDIOENC_PARAM_BUFINFO, pinfo)) != HD_OK) {
			DBG_ERR("Get HD_AUDIOENC_PARAM_BUFINFO fail(%d)\r\n", hd_ret);
		} else {
			ret = E_OK;
		}
	} else {
		DBG_ERR("pinfo = NULL!\r\n");
	}
	return ret;
}

ER ImageApp_MovieMulti_GetVdoEncBufInfo(MOVIE_CFG_REC_ID id, HD_VIDEOENC_BUFINFO *pinfo)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	if (pinfo) {
		memset(pinfo, 0, sizeof(HD_VIDEOENC_BUFINFO));
		_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);
		if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
			if ((hd_ret = hd_videoenc_get(ImgLink[tb.idx].venc_p[tb.tbl], HD_VIDEOENC_PARAM_BUFINFO, pinfo)) != HD_OK) {
				DBG_ERR("Get HD_VIDEOENC_PARAM_BUFINFO fail(%d)\r\n", hd_ret);
			} else {
				ret = E_OK;
			}
		} else if ((tb.link_type == LINKTYPE_STRM) && (tb.idx < MAX_WIFI_PATH)) {
			if ((hd_ret = hd_videoenc_get(WifiLink[tb.idx].venc_p[0], HD_VIDEOENC_PARAM_BUFINFO, pinfo)) != HD_OK) {
				DBG_ERR("Get HD_VIDEOENC_PARAM_BUFINFO fail(%d)\r\n", hd_ret);
			} else {
				ret = E_OK;
			}
		} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
			if ((hd_ret = hd_videoenc_get(EthCamLink[tb.idx].venc_p[0], HD_VIDEOENC_PARAM_BUFINFO, pinfo)) != HD_OK) {
				DBG_ERR("Get HD_VIDEOENC_PARAM_BUFINFO fail(%d)\r\n", hd_ret);
			} else {
				ret = E_OK;
			}
		}
	} else {
		DBG_ERR("pinfo = NULL!\r\n");
	}
	return ret;
}

ER ImageApp_MovieMulti_GetVcapSysInfo(MOVIE_CFG_REC_ID id, HD_VIDEOCAP_SYSINFO *pinfo)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret = HD_OK;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	if (pinfo) {
		memset(pinfo, 0, sizeof(HD_VIDEOCAP_SYSINFO));
		_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);
		if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
			if ((hd_ret = hd_videocap_get(ImgLink[tb.idx].vcap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, pinfo)) != HD_OK) {
				DBG_ERR("id%d(%x) get HD_VIDEOCAP_PARAM_SYSINFO fail(%d)\r\n", id, ImgLink[tb.idx].vcap_ctrl, hd_ret);
			} else {
				ret = E_OK;
			}
		}
	} else {
		DBG_ERR("pinfo is NULL\r\n");
	}
	return ret;
}

UINT32 ImageApp_MovieMulti_DetBufUsage(MOVIE_CFG_REC_ID id)
{
	UINT32 usage = 0;
	HD_RESULT hd_ret;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;
	HD_BSMUX_EN_UTIL util = {0};
	HD_PATH_ID bsmux_path = 0;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if ((IsImgLinkOpened[tb.idx] == TRUE) && (_LinkCheckStatus(ImgLinkStatus[tb.idx].bsmux_p[2*tb.tbl]))) {
			bsmux_path = ImgLink[tb.idx].bsmux_p[2*tb.tbl];
			util.type = HD_BSMUX_EN_UTIL_BUF_USAGE;
			if ((hd_ret = hd_bsmux_get(bsmux_path, HD_BSMUX_PARAM_EN_UTIL, (void*)&util)) != HD_OK) {
				DBG_ERR("hd_bsmux_get(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
			} else {
				//DBG_DUMP("BUF[%d]: used:%d free:%d total:%d\r\n", bsmux_path, util.resv[0], util.resv[1], util.enable);
				usage = util.resv[0];
			}
		}
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		if ((is_ethcamlink_opened[tb.idx] == TRUE) && (_LinkCheckStatus(EthCamLinkStatus[tb.idx].bsmux_p[0]))) {
			bsmux_path = EthCamLink[tb.idx].bsmux_p[0];
			util.type = HD_BSMUX_EN_UTIL_BUF_USAGE;
			if ((hd_ret = hd_bsmux_get(bsmux_path, HD_BSMUX_PARAM_EN_UTIL, (void*)&util)) != HD_OK) {
				DBG_ERR("hd_bsmux_get(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
			} else {
				//DBG_DUMP("BUF[%d]: used:%d free:%d total:%d\r\n", bsmux_path, util.resv[0], util.resv[1], util.enable);
				usage = util.resv[0];
			}
		}
	}
	return usage;
}

ER ImageApp_MovieMulti_SetUTCTimeInfo(MOVIE_CFG_REC_ID id, struct tm *p_timeinfo)
{
	UINT32 i = id;
	MOVIE_TBL_IDX tb;
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	HD_PATH_ID bsmux_path = UNUSED;
	HD_BSMUX_EN_UTIL util = {0};

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if (p_timeinfo) {
		if (i == _CFG_CTRL_ID) {
			if ((hd_ret = bsmux_get_ctrl_path(0, &bsmux_path)) != HD_OK) {
				DBG_ERR("bsmux_get_ctrl_path() fail(%d)\n", hd_ret);
			}
		} else {
			if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
				bsmux_path = ImgLink[tb.idx].bsmux_p[2*tb.tbl];
			} else {
				bsmux_path = EthCamLink[tb.idx].bsmux_p[0];
			}
		}
		if (bsmux_path != UNUSED) {
			util.type = HD_BSMUX_EN_UTIL_UTC_TIME;
			util.enable = TRUE;
			util.resv[0] = HD_BSMUX_UTC_DATE(p_timeinfo->tm_year, p_timeinfo->tm_mon, p_timeinfo->tm_mday);
			util.resv[1] = HD_BSMUX_UTC_TIME(p_timeinfo->tm_hour, p_timeinfo->tm_min, p_timeinfo->tm_sec);
			if ((hd_ret = hd_bsmux_set(bsmux_path, HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
				DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
			} else {
				ret = E_OK;
			}
		}
	} else {
		DBG_ERR("p_timeinfo is NULL\r\n");
	}
	return ret;
}

INT32 ImageApp_MovieMulti_SetFormatFree(CHAR drive, BOOL is_on)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		if (0 != iamoviemulti_fm_set_format_free(drive, is_on)) {
			return -1;
		}
		DBG_IND("^G SetFormatFree %d\r\n", is_on);
		return 0;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return -1;
	}
}

INT32 ImageApp_MovieMulti_GetFolderInfo(CHAR *p_path, UINT32 *ratio, UINT64 *f_size)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		if (0 != iamoviemulti_fm_get_dir_info(p_path, ratio, f_size)) {
			return -1;
		}
		DBG_IND("^G GetFolderInfo %s: %d (%lld)\r\n", p_path, (*ratio), (*f_size));
		return 0;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return -1;
	}
}

INT32 ImageApp_MovieMulti_SetFolderInfo(CHAR *p_path, UINT32 ratio, UINT64 f_size)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		if (0 != iamoviemulti_fm_set_dir_info(p_path, ratio, f_size)) {
			return -1;
		}
		DBG_IND("^G SetFolderInfo %s: %d (%lld)\r\n", p_path, ratio, f_size);
		return 0;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return -1;
	}
}

INT32 ImageApp_MovieMulti_SetCyclicRec(CHAR *p_path, BOOL is_on)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		if (0 != iamoviemulti_fm_set_cyclic_rec(p_path, is_on)) {
			return -1;
		}
		DBG_IND("^G SetCyclicRec %s: %d\r\n", p_path, is_on);
		return 0;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return -1;
	}
}

INT32 ImageApp_MovieMulti_SetExtStrType(CHAR drive, UINT32 type)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		if (0 != iamoviemulti_fm_set_extstr_type(drive, type)) {
			return -1;
		}
		DBG_IND("^G SetExtStrType %s: %d\r\n", drive, type);
		return 0;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return -1;
	}
}

INT32 ImageApp_MovieMulti_SetUseHiddenFirst(CHAR drive, BOOL value)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		if (0 != iamoviemulti_fm_set_reuse_hidden_first(drive, value)) {
			return -1;
		}
		DBG_IND("^G SetUseHiddenFirst %s: %d\r\n", drive, value);
		return 0;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return -1;
	}
}

INT32 ImageApp_MovieMulti_SetSyncTime(CHAR drive, BOOL value)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		if (0 != iamoviemulti_fm_set_sync_time(drive, (INT32)value)) {
			return -1;
		}
		DBG_IND("^G SetSyncTime %s: %d\r\n", drive, value);
		return 0;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return -1;
	}
}

INT32 ImageApp_MovieMulti_SetFileUnit(CHAR *p_path, UINT32 unit)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		if (0 != iamoviemulti_fm_set_file_unit(p_path, unit)) {
			return -1;
		}
		DBG_IND("^G SetFileUnit %s: %d\r\n", p_path, unit);
		return 0;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return -1;
	}
}

INT32 ImageApp_MovieMulti_SetSkipReadOnly(CHAR drive, BOOL value)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		if (0 != iamoviemulti_fm_set_skip_read_only(drive, value)) {
			return -1;
		}
		DBG_IND("^G SetUseHiddenFirst %s: %d\r\n", drive, value);
		return 0;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return -1;
	}
}

ER ImageApp_MovieMulti_GetDesc(MOVIE_CFG_REC_ID id, UINT32 codec, PMEM_RANGE desc, PMEM_RANGE sps, PMEM_RANGE pps, PMEM_RANGE vps)
{
#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
	ER ret = E_OK;
	HD_RESULT hd_ret = HD_OK;
	UINT32 i = id, need_stop = 0;
	MOVIE_TBL_IDX tb = {0};
	UINT32 bs_buf_va = 0;
	HD_VIDEOENC_BUFINFO bs_buf = {0};
	VENDOR_VIDEOENC_H26X_DESC_CFG h26x_desc = {0};

	if (!desc || !sps || !pps || !vps) {
		DBG_ERR("Some parameters are null (desc=0x%x, sps=0x%x, pps=0x%x, vps=0x%x)\r\n", desc, sps, pps, vps);
		return E_PAR;
	}

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if (_LinkCheckStatus(ImgLinkStatus[tb.idx].venc_p[tb.tbl]) == FALSE) {
		if (ret == E_OK) {
			if ((hd_ret = hd_videoenc_start(ImgLink[tb.idx].venc_p[tb.tbl])) != HD_OK) {
				DBG_ERR("hd_videoenc_start fail(%d)\n", hd_ret);
				ret = E_SYS;
			} else {
				need_stop = TRUE;
			}
		}
	}
	if (ret == E_OK) {
		if ((hd_ret = hd_videoenc_get(ImgLink[tb.idx].venc_p[tb.tbl], HD_VIDEOENC_PARAM_BUFINFO, &bs_buf)) != HD_OK) {
			DBG_ERR("hd_videoenc_get fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
	}
	if (ret == E_OK) {
		if ((bs_buf_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, bs_buf.buf_info.phy_addr, bs_buf.buf_info.buf_size)) == 0) {
			DBG_ERR("hd_common_mem_mmap fail\n");
			ret = E_SYS;
		}
	}
	if (ret == E_OK) {
		if ((hd_ret = vendor_videoenc_get(ImgLink[tb.idx].venc_p[tb.tbl], VENDOR_VIDEOENC_PARAM_OUT_H26X_DESC, &h26x_desc)) != HD_OK) {
			DBG_ERR("vendor_videoenc_get(VENDOR_VIDEOENC_PARAM_OUT_H26X_DESC) fail\n");
			ret = E_SYS;
		}
	}
	if (ret == E_OK) {
		UINT32 total_size = h26x_desc.vps_size + h26x_desc.sps_size + h26x_desc.pps_size;
		if (desc->size < total_size) {
			DBG_ERR("desc buffer size too small (%d<%d)\r\n", desc->size, total_size);
			desc->size = 0;
		} else {
			desc->size = total_size;
			vps->addr = desc->addr;
			vps->size = h26x_desc.vps_size;
			if (vps->size) {
				memcpy((void *)vps->addr, (void *)(h26x_desc.vps_paddr - bs_buf.buf_info.phy_addr + bs_buf_va) , vps->size);
			}
			sps->addr = vps->addr + vps->size;
			sps->size = h26x_desc.sps_size;
			if (sps->size) {
				memcpy((void *)sps->addr, (void *)(h26x_desc.sps_paddr - bs_buf.buf_info.phy_addr + bs_buf_va) , sps->size);
			}
			pps->addr = sps->addr + sps->size;
			pps->size = h26x_desc.pps_size;
			if (pps->size) {
				memcpy((void *)pps->addr, (void *)(h26x_desc.pps_paddr - bs_buf.buf_info.phy_addr + bs_buf_va) , pps->size);
			}

		}
	} else {
		desc->size = 0;
		vps->addr = desc->addr;
		vps->size = 0;
		sps->addr = desc->addr;
		sps->size = 0;
		pps->addr = desc->addr;
		pps->size = 0;
	}
	if (bs_buf_va) {
		hd_common_mem_munmap((void *)bs_buf_va, bs_buf.buf_info.buf_size);
	}
	if (_LinkCheckStatus(ImgLinkStatus[tb.idx].venc_p[tb.tbl]) == FALSE) {
		if (need_stop) {
			if ((hd_ret = hd_videoenc_stop(ImgLink[tb.idx].venc_p[tb.tbl])) != HD_OK) {
				DBG_ERR("hd_videoenc_stop fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
		}
	}
	return ret;
#else
	return E_NOSPT;
#endif
}

