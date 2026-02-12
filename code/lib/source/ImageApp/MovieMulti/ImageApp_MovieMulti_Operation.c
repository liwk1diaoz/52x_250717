#include "ImageApp_MovieMulti_int.h"

/// ========== Cross file variables ==========
UINT32 IsImgLinkForDispEnabled[MAX_IMG_PATH][MAX_DISP_PATH] = {0};
UINT32 IsImgLinkForStreamingEnabled[MAX_IMG_PATH][MAX_WIFI_PATH] = {0};
UINT32 IsImgLinkForUVACEnabled[MAX_IMG_PATH][MAX_WIFI_PATH] = {0};
UINT32 IsImgLinkForAlgEnabled[MAX_IMG_PATH][_CFG_ALG_PATH_MAX] = {0};
UINT32 IsImgLinkTranscodeMode[MAX_IMG_PATH][2] = {0};
UINT32 IsImgLinkMode3RecStart[MAX_IMG_PATH][4] = {0};
/// ========== Noncross file variables ==========

ER ImageApp_MovieMulti_ImgLinkForDisp(MOVIE_CFG_REC_ID id, UINT32 action, BOOL allow_pull)
{
	UINT32 i = id, idx;
	ER ret;
	UINT32 iport;

	if ((i >= MaxLinkInfo.MaxImgLink) || (i >= MAX_IMG_PATH)) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}

	idx = i - _CFG_REC_ID_1;

	if ((IsImgLinkForDispEnabled[idx][0] == TRUE) && (action == ENABLE)) {
		DBG_ERR("id%d is already enabled.\r\n", i);
		return E_SYS;
	}
	if ((IsImgLinkForDispEnabled[idx][0] == FALSE) && (action == DISABLE)) {
		DBG_ERR("id%d is already disabled.\r\n", i);
		return E_SYS;
	}

	if (gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP] == UNUSED) {
		DBG_ERR("gSwitchLink[%d][%d]=%d\r\n", idx, IAMOVIE_VPRC_EX_DISP, gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP]);
		return E_SYS;
	}
	iport = gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_DISP]];
	if (iport > 2) {
		iport = 2;
	}
	if (action == ENABLE) {
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		if (img_vproc_vpe_en[idx] == MOVIE_VPE_NONE) {
#endif // #if (IAMOVIE_SUPPORT_VPE == ENABLE)
			if (img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[idx] == FALSE) {
					_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][img_vproc_3dnr_path[idx]]));
				} else {
					_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][4]));
				}
			}
			_LinkPortCntInc(&(ImgLinkStatus[idx].vcap_p[0]));
			if (!((img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == FALSE) && (iport == img_vproc_3dnr_path[idx]))) {
				_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][iport]));
			}
			if (img_vproc_no_ext[idx] == FALSE) {
				_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][IAMOVIE_VPRC_EX_DISP]));
			}
			IsImgLinkForDispEnabled[idx][0] = TRUE;
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		} else {
			if (img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[idx] == FALSE) {
					_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[0][img_vproc_3dnr_path[idx]]));
				} else {
					_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[0][4]));
				}
			}
			_LinkPortCntInc(&(ImgLinkStatus[idx].vcap_p[0]));

			if (!((img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == FALSE))) {
				_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[0][0]));
			}

			_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][iport]));
			if (img_vproc_no_ext[idx] == FALSE) {
				_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][IAMOVIE_VPRC_EX_DISP]));
			}
			IsImgLinkForDispEnabled[idx][0] = TRUE;
		}
#endif // #if (IAMOVIE_SUPPORT_VPE == ENABLE)
	} else if (action == DISABLE) {
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		if (img_vproc_vpe_en[idx] == MOVIE_VPE_NONE) {
#else
		if (1) {
#endif
			if (img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[idx] == FALSE) {
					_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][img_vproc_3dnr_path[idx]]));
				} else {
					_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][4]));
				}
			}
			_LinkPortCntDec(&(ImgLinkStatus[idx].vcap_p[0]));
			if (!((img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == FALSE) && (iport == img_vproc_3dnr_path[idx]))) {
				_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][iport]));
			}
			if (img_vproc_no_ext[idx] == FALSE) {
				_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][IAMOVIE_VPRC_EX_DISP]));
			}
			IsImgLinkForDispEnabled[idx][0] = FALSE;
		} else {
			if (img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[idx] == FALSE) {
					_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[0][img_vproc_3dnr_path[idx]]));
				} else {
					_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[0][4]));
				}
			}
			_LinkPortCntDec(&(ImgLinkStatus[idx].vcap_p[0]));

			if (!((img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == FALSE))) {
				_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[0][0]));
			}

			_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][iport]));
			if (img_vproc_no_ext[idx] == FALSE) {
				_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][IAMOVIE_VPRC_EX_DISP]));
			}
			IsImgLinkForDispEnabled[idx][0] = FALSE;
		}
	}
	ret = _ImageApp_MovieMulti_ImgLinkStatusUpdate(i, (action == ENABLE) ? UPDATE_REVERSE : UPDATE_FORWARD);

	return ret;
}

UINT32 ImageApp_MovieMulti_ImgLinkForDispStatus(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx;

	if ((i >= MaxLinkInfo.MaxImgLink) || (i >= MAX_IMG_PATH)) {
		return FALSE;
	}

	idx = i - _CFG_REC_ID_1;

	return IsImgLinkForDispEnabled[idx][0];
}

static ER _ImgLinkForStreaming(MOVIE_CFG_REC_ID id, UINT32 action)
{
	UINT32 i = id, idx;
	UINT32 iport;

	idx = i - _CFG_REC_ID_1;

	if (gSwitchLink[idx][IAMOVIE_VPRC_EX_WIFI] == UNUSED) {
		DBG_ERR("gSwitchLink[%d][%d]=%d\r\n", idx, IAMOVIE_VPRC_EX_WIFI, gSwitchLink[idx][IAMOVIE_VPRC_EX_WIFI]);
		return E_SYS;
	}
	iport = gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_WIFI]];
	if (iport > 2) {
		iport = 2;
	}
	if (action == ENABLE) {
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		if (img_vproc_vpe_en[idx] == MOVIE_VPE_NONE) {
#endif
			if (img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[idx] == FALSE) {
					_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][img_vproc_3dnr_path[idx]]));
				} else {
					_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][4]));
				}
			}
			_LinkPortCntInc(&(ImgLinkStatus[idx].vcap_p[0]));
			if (!((img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == FALSE) && (iport == img_vproc_3dnr_path[idx]))) {
				_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][iport]));
			}
			if (img_vproc_no_ext[idx] == FALSE) {
				_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][IAMOVIE_VPRC_EX_WIFI]));
			}
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		} else {
			if (img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[idx] == FALSE) {
					_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[0][img_vproc_3dnr_path[idx]]));
				} else {
					_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[0][4]));
				}
			}
			_LinkPortCntInc(&(ImgLinkStatus[idx].vcap_p[0]));

			if (!((img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == FALSE))) {
				_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[0][0]));
			}

			_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][iport]));
			if (img_vproc_no_ext[idx] == FALSE) {
				_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][IAMOVIE_VPRC_EX_WIFI]));
			}
		}
#endif
	} else if (action == DISABLE) {
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		if (img_vproc_vpe_en[idx] == MOVIE_VPE_NONE) {
#endif
			if (img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[idx] == FALSE) {
					_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][img_vproc_3dnr_path[idx]]));
				} else {
					_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][4]));
				}
			}
			_LinkPortCntDec(&(ImgLinkStatus[idx].vcap_p[0]));
			if (!((img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == FALSE) && (iport == img_vproc_3dnr_path[idx]))) {
				_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][iport]));
			}
			if (img_vproc_no_ext[idx] == FALSE) {
				_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][IAMOVIE_VPRC_EX_WIFI]));
			}
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		} else {
			if (img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[idx] == FALSE) {
					_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[0][img_vproc_3dnr_path[idx]]));
				} else {
					_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[0][4]));
				}
			}
			_LinkPortCntDec(&(ImgLinkStatus[idx].vcap_p[0]));

			if (!((img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == FALSE))) {
				_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[0][0]));
			}

			_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][iport]));
			if (img_vproc_no_ext[idx] == FALSE) {
				_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][IAMOVIE_VPRC_EX_WIFI]));
			}
		}
#endif
	}
	return _ImageApp_MovieMulti_ImgLinkStatusUpdate(i, (action == ENABLE) ? UPDATE_REVERSE : UPDATE_FORWARD);
}

ER ImageApp_MovieMulti_ImgLinkForStreaming(MOVIE_CFG_REC_ID id, UINT32 action, BOOL allow_pull)
{
	UINT32 i = id, idx;
	ER ret;

	if ((i >= MaxLinkInfo.MaxImgLink) || (i >= MAX_IMG_PATH)) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}

	idx = i - _CFG_REC_ID_1;

	if ((IsImgLinkForStreamingEnabled[idx][0] == TRUE) && (action == ENABLE)) {
		DBG_ERR("id%d is already enabled.\r\n", i);
		return E_SYS;
	}
	if ((IsImgLinkForStreamingEnabled[idx][0] == FALSE) && (action == DISABLE)) {
		DBG_ERR("id%d is already disabled.\r\n", i);
		return E_SYS;
	}
	ret = _ImgLinkForStreaming(id, action);
	IsImgLinkForStreamingEnabled[idx][0] = (action == ENABLE) ? TRUE : FALSE;

	return ret;
}

UINT32 ImageApp_MovieMulti_ImgLinkForStreamingStatus(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx;

	if ((i >= MaxLinkInfo.MaxImgLink) || (i >= MAX_IMG_PATH)) {
		return FALSE;
	}

	idx = i - _CFG_REC_ID_1;

	return IsImgLinkForStreamingEnabled[idx][0];
}

ER ImageApp_MovieMulti_ImgLinkForUVAC(MOVIE_CFG_REC_ID id, UINT32 action, BOOL allow_pull)
{
	UINT32 i = id, idx;
	ER ret;

	if ((i >= MaxLinkInfo.MaxImgLink) || (i >= MAX_IMG_PATH)) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}

	idx = i - _CFG_REC_ID_1;

	if ((IsImgLinkForUVACEnabled[idx][0] == TRUE) && (action == ENABLE)) {
		DBG_ERR("id%d is already enabled.\r\n", i);
		return E_SYS;
	}
	if ((IsImgLinkForUVACEnabled[idx][0] == FALSE) && (action == DISABLE)) {
		DBG_ERR("id%d is already disabled.\r\n", i);
		return E_SYS;
	}
	ret = _ImgLinkForStreaming(id, action);
	IsImgLinkForUVACEnabled[idx][0] = (action == ENABLE) ? TRUE : FALSE;

	return ret;
}

UINT32 ImageApp_MovieMulti_ImgLinkForUVACStatus(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id, idx;

	if ((i >= MaxLinkInfo.MaxImgLink) || (i >= MAX_IMG_PATH)) {
		return FALSE;
	}

	idx = i - _CFG_REC_ID_1;

	return IsImgLinkForUVACEnabled[idx][0];
}

ER ImageApp_MovieMulti_ImgLinkForAlg(MOVIE_CFG_REC_ID id, MOVIE_CFG_ALG_PATH path, UINT32 action, BOOL allow_pull)
{
	UINT32 i = id, idx;
	ER ret;
	UINT32 iport;

	if ((i >= MaxLinkInfo.MaxImgLink) || (i >= MAX_IMG_PATH)) {
		DBG_ERR("id%d is out of range.\r\n", i);
		return E_SYS;
	}

	idx = i - _CFG_REC_ID_1;

	if ((IsImgLinkForAlgEnabled[idx][path] == TRUE) && (action == ENABLE)) {
		DBG_ERR("id%d.%d is already enabled.\r\n", i, path);
		return E_SYS;
	}
	if ((IsImgLinkForAlgEnabled[idx][path] == FALSE) && (action == DISABLE)) {
		DBG_ERR("id%d.%d is already disabled.\r\n", i, path);
		return E_SYS;
	}

	if (path == _CFG_ALG_PATH3) {
		if (gSwitchLink[idx][IAMOVIE_VPRC_EX_ALG] == UNUSED) {
			DBG_ERR("gSwitchLink[%d][%d]=%d\r\n", idx, IAMOVIE_VPRC_EX_ALG, gSwitchLink[idx][IAMOVIE_VPRC_EX_ALG]);
			return E_SYS;
		}
		iport = gUserProcMap[idx][gSwitchLink[idx][IAMOVIE_VPRC_EX_ALG]];
		if (iport > 2) {
			iport = 2;
		}
		if (action == ENABLE) {
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
			if (img_vproc_vpe_en[idx] == MOVIE_VPE_NONE) {
#endif
				if (img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
					if (use_unique_3dnr_path[idx] == FALSE) {
						_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][img_vproc_3dnr_path[idx]]));
					} else {
						_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][4]));
					}
				}
				_LinkPortCntInc(&(ImgLinkStatus[idx].vcap_p[0]));
				if (!((img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == FALSE) && (iport == img_vproc_3dnr_path[idx]))) {
					_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][iport]));
				}
				if (img_vproc_no_ext[idx] == FALSE) {
					_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][IAMOVIE_VPRC_EX_ALG]));
				}
				IsImgLinkForAlgEnabled[idx][path] = TRUE;
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
			} else {
				if (img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
					if (use_unique_3dnr_path[idx] == FALSE) {
						_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[0][img_vproc_3dnr_path[idx]]));
					} else {
						_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[0][4]));
					}
				}
				_LinkPortCntInc(&(ImgLinkStatus[idx].vcap_p[0]));

				if (!((img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == FALSE))) {
					_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[0][0]));
				}

				_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][iport]));
				if (img_vproc_no_ext[idx] == FALSE) {
					_LinkPortCntInc(&(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][IAMOVIE_VPRC_EX_ALG]));
				}
				IsImgLinkForAlgEnabled[idx][path] = TRUE;
			}
#endif
		} else if (action == DISABLE) {
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
			if (img_vproc_vpe_en[idx] == MOVIE_VPE_NONE) {
#endif
				if (img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
					if (use_unique_3dnr_path[idx] == FALSE) {
						_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][img_vproc_3dnr_path[idx]]));
					} else {
						_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][4]));
					}
				}
				_LinkPortCntDec(&(ImgLinkStatus[idx].vcap_p[0]));
				if (!((img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == FALSE) && (iport == img_vproc_3dnr_path[idx]))) {
					_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][iport]));
				}
				if (img_vproc_no_ext[idx] == FALSE) {
					_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][IAMOVIE_VPRC_EX_ALG]));
				}
				IsImgLinkForAlgEnabled[idx][path] = FALSE;
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
			} else {
				if (img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
					if (use_unique_3dnr_path[idx] == FALSE) {
						_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[0][img_vproc_3dnr_path[idx]]));
					} else {
						_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[0][4]));
					}
				}
				_LinkPortCntDec(&(ImgLinkStatus[idx].vcap_p[0]));

				if (!((img_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[idx] == FALSE))) {
					_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[0][0]));
				}

				_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p_phy[img_vproc_splitter[idx]][iport]));
				if (img_vproc_no_ext[idx] == FALSE) {
					_LinkPortCntDec(&(ImgLinkStatus[idx].vproc_p[img_vproc_splitter[idx]][IAMOVIE_VPRC_EX_ALG]));
				}
				IsImgLinkForAlgEnabled[idx][path] = FALSE;
			}
#endif
		}
	}
	ret = _ImageApp_MovieMulti_ImgLinkStatusUpdate(i, (action == ENABLE) ? UPDATE_REVERSE : UPDATE_FORWARD);

	return ret;
}

UINT32 ImageApp_MovieMulti_ImgLinkForAlgStatus(MOVIE_CFG_REC_ID id, MOVIE_CFG_ALG_PATH path)
{
	UINT32 i = id, idx;

	if ((i >= MaxLinkInfo.MaxImgLink) || (i >= MAX_IMG_PATH)) {
		return FALSE;
	}

	idx = i - _CFG_REC_ID_1;

	return IsImgLinkForAlgEnabled[idx][path];
}

static ER _aenc_get_buf_info(UINT32 idx)
{
	HD_RESULT hd_ret;
	ER ret = E_OK;

	if (aud_aenc_bs_buf[idx].buf_info.phy_addr == 0) {
		if (AudCapLinkStatus[idx].aenc_p[0] == 0) {
			if ((hd_ret = hd_audioenc_start(AudCapLink[idx].aenc_p[0])) != HD_OK) {
				DBG_ERR("hd_audioenc_start fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
			if ((hd_ret = hd_audioenc_get(AudCapLink[idx].aenc_p[0], HD_AUDIOENC_PARAM_BUFINFO, &(aud_aenc_bs_buf[idx]))) != HD_OK) {
				DBG_ERR("hd_audioenc_get fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
			if (img_flow_for_dumped_fast_boot == FALSE) {
				if ((hd_ret = hd_audioenc_stop(AudCapLink[idx].aenc_p[0])) != HD_OK) {
					DBG_ERR("hd_audioenc_stop fail(%d)\n", hd_ret);
					ret = E_SYS;
				}
			}
		} else {
			if ((hd_ret = hd_audioenc_get(AudCapLink[idx].aenc_p[0], HD_AUDIOENC_PARAM_BUFINFO, &(aud_aenc_bs_buf[idx]))) != HD_OK) {
				DBG_ERR("hd_audioenc_get fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
		}
		aud_aenc_bs_buf_va[idx] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, aud_aenc_bs_buf[idx].buf_info.phy_addr, aud_aenc_bs_buf[idx].buf_info.buf_size);
	}
	return ret;
}

static ER _img_venc_get_buf_info(UINT32 idx, UINT32 tbl)
{
	HD_RESULT hd_ret;
	ER ret = E_OK;

	if (img_venc_bs_buf[idx][tbl].buf_info.phy_addr == 0) {
		if ((hd_ret = hd_videoenc_start(ImgLink[idx].venc_p[tbl])) != HD_OK) {
			DBG_ERR("hd_videoenc_start fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		if ((hd_ret = hd_videoenc_get(ImgLink[idx].venc_p[tbl], HD_VIDEOENC_PARAM_BUFINFO, &(img_venc_bs_buf[idx][tbl]))) != HD_OK) {
			DBG_ERR("hd_videoenc_get fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		if (img_flow_for_dumped_fast_boot == FALSE) {
			if ((hd_ret = hd_videoenc_stop(ImgLink[idx].venc_p[tbl])) != HD_OK) {
				DBG_ERR("hd_videoenc_stop fail(%d)\n", hd_ret);
				ret = E_SYS;
			}
		}
		img_venc_bs_buf_va[idx][tbl] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, img_venc_bs_buf[idx][tbl].buf_info.phy_addr, img_venc_bs_buf[idx][tbl].buf_info.buf_size);
	}
	return ret;
}

static ER _ethcam_venc_get_buf_info(UINT32 idx)
{
	HD_RESULT hd_ret;
	ER ret = E_OK;

	if (ethcam_venc_bs_buf[idx].buf_info.phy_addr == 0) {
		if ((hd_ret = hd_videoenc_start(EthCamLink[idx].venc_p[0])) != HD_OK) {
			DBG_ERR("hd_videoenc_start fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		if ((hd_ret = hd_videoenc_get(EthCamLink[idx].venc_p[0], HD_VIDEOENC_PARAM_BUFINFO, &(ethcam_venc_bs_buf[idx]))) != HD_OK) {
			DBG_ERR("hd_videoenc_get fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		if ((hd_ret = hd_videoenc_stop(EthCamLink[idx].venc_p[0])) != HD_OK) {
			DBG_ERR("hd_videoenc_stop fail(%d)\n", hd_ret);
			ret = E_SYS;
		}
		ethcam_venc_bs_buf_va[idx] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, ethcam_venc_bs_buf[idx].buf_info.phy_addr, ethcam_venc_bs_buf[idx].buf_info.buf_size);
	}
	return ret;
}

ER ImageApp_MovieMulti_RecStart(MOVIE_CFG_REC_ID id)
{
	HD_RESULT hd_ret;
	ER ret = E_SYS;
	UINT32 i = id;
	UINT32 iport;
	FLGPTN vflag;
	MOVIE_TBL_IDX tb;
	HD_BSMUX_BUFINFO bsmux_buf;

	vos_sem_wait(IAMOVIE_SEMID_OPERATION);

	if (ImageApp_MovieMulti_IsStreamRunning(i) == TRUE) {
		vos_sem_sig(IAMOVIE_SEMID_OPERATION);
		return ret;
	}

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		// check 2v1a mode
		if ((img_bsmux_2v1a_mode[tb.idx] != 0) && (tb.tbl == MOVIETYPE_MAIN)) {     // 2v1a mode
			if (ImageApp_MovieMulti_IsStreamRunning(_CFG_CLONE_ID_1 + i) == FALSE) {
				DBG_ERR("Clone path should start first if 2v1a mode enabled!\r\n");
				vos_sem_sig(IAMOVIE_SEMID_OPERATION);
				return ret;
			}
		}
		if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3)) {
			// get venc buffer addr
			_img_venc_get_buf_info(tb.idx, tb.tbl);
			// start aenc and get aenc buffer addr
			if (ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode == _CFG_REC_TYPE_AV) {
				if ((ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_AAC) || (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_ULAW) || (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_ALAW)) {
					_ImageApp_MovieMulti_AudRecStart(0);
					_aenc_get_buf_info(0);
				} else if (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_PCM) {
					ImageApp_MovieMulti_AudCapStart(0);
				}
			}
		}
		// get IME and UserProc port
		if (gSwitchLink[tb.idx][tb.tbl] == UNUSED) {
			DBG_ERR("gSwitchLink[%d][%d]=%d\r\n", tb.idx, tb.tbl, gSwitchLink[tb.idx][tb.tbl]);
			return E_SYS;
		}
		iport = gUserProcMap[tb.idx][gSwitchLink[tb.idx][tb.tbl]];     // 0:main, 1:clone
		if (iport > 2) {
			iport = 2;
		}
		// Set port
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		if (img_vproc_vpe_en[tb.idx] == MOVIE_VPE_NONE) {
#endif
			if (img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[tb.idx] == FALSE) {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][img_vproc_3dnr_path[tb.idx]]));
				} else {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][4]));
				}
			}
			_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vcap_p[0]));
			if (!((img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[tb.idx] == FALSE) && (iport == img_vproc_3dnr_path[tb.idx]))) {
				_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][iport]));
			}
			if (img_vproc_no_ext[tb.idx] == FALSE) {
				_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][tb.tbl]));
			}
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		} else {
			if (img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[tb.idx] == FALSE) {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][img_vproc_3dnr_path[tb.idx]]));
				} else {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][4]));
				}
			}
			_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vcap_p[0]));

			if (!((img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[tb.idx] == FALSE))) {
				_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][0]));
			}

			_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][iport]));
			if (img_vproc_no_ext[tb.idx] == FALSE) {
				_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][tb.tbl]));
			}
		}
#endif
		if (img_bsmux_2v1a_mode[tb.idx] == 0) {                                     // normal_mode or pseudo_rec_mode_2
			if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
				(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2) ||
				(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3) ||
				(img_vproc_yuv_compress[tb.idx] && (img_venc_sout_type[tb.idx][tb.tbl] != MOVIE_IMGCAP_SOUT_NONE))) {
				_LinkPortCntInc(&(ImgLinkStatus[tb.idx].venc_p[tb.tbl]));
#if (defined(_BSP_NA51055_) || defined(_BSP_NA51089_))
				VENDOR_VIDEOENC_H26X_SKIP_FRM_CFG skip = {0};
				skip.b_enable = img_venc_skip_frame_cfg[tb.idx][tb.tbl] ? ENABLE : DISABLE;
				skip.target_fr = GET_HI_UINT16(img_venc_skip_frame_cfg[tb.idx][tb.tbl]);
				skip.input_frm_cnt = GET_LO_UINT16(img_venc_skip_frame_cfg[tb.idx][tb.tbl]);
				vendor_videoenc_set(ImgLink[tb.idx].venc_p[tb.tbl], VENDOR_VIDEOENC_PARAM_OUT_H26X_SKIP_FRM, &skip);
#endif
			}
		} else {                                                                    // 2v1a mode
			if (tb.tbl == MOVIETYPE_MAIN) {
				if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
					(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2) ||
					(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3)) {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].venc_p[0]));
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].venc_p[1]));
#if (defined(_BSP_NA51055_) || defined(_BSP_NA51089_))
					VENDOR_VIDEOENC_H26X_SKIP_FRM_CFG skip = {0};
					UINT32 j;
					for (j = 0; j < 2; j++) {
						skip.b_enable = img_venc_skip_frame_cfg[tb.idx][j] ? ENABLE : DISABLE;
						skip.target_fr = GET_HI_UINT16(img_venc_skip_frame_cfg[tb.idx][j]);
						skip.input_frm_cnt = GET_LO_UINT16(img_venc_skip_frame_cfg[tb.idx][j]);
						vendor_videoenc_set(ImgLink[tb.idx].venc_p[j], VENDOR_VIDEOENC_PARAM_OUT_H26X_SKIP_FRM, &skip);
					}
#endif
				}
			}
		}
		if (tb.tbl == MOVIETYPE_MAIN) {
			if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
				(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3)) {
				// set bsmux buffer address
				bsmux_buf.videnc.phy_addr = img_venc_bs_buf[tb.idx][tb.tbl].buf_info.phy_addr;
				bsmux_buf.videnc.buf_size = img_venc_bs_buf[tb.idx][tb.tbl].buf_info.buf_size;
				bsmux_buf.audenc.phy_addr = (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_PCM) ? aud_acap_bs_buf[0].buf_info.phy_addr : aud_aenc_bs_buf[0].buf_info.phy_addr;
				bsmux_buf.audenc.buf_size = (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_PCM) ? aud_acap_bs_buf[0].buf_info.buf_size : aud_aenc_bs_buf[0].buf_info.buf_size;
				if ((ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_ONLY) != _CFG_MAIN_EMR_ONLY && (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) != _CFG_MAIN_EMR_LOOP) {      // main
					if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[0], HD_BSMUX_PARAM_BUFINFO, (void*)&bsmux_buf)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_BUFINFO) fail(%d)\n", hd_ret);
					}
					BsMuxFrmCnt[ImgLink[tb.idx].bsmux_o[0]] = 0;
					if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE)) {
						_LinkPortCntInc(&(ImgLinkStatus[tb.idx].bsmux_p[0]));
						_LinkPortCntInc(&(ImgLinkStatus[tb.idx].fileout_p[0]));
					}
				}
				if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {                                   // main_emr
					if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[1], HD_BSMUX_PARAM_BUFINFO, (void*)&bsmux_buf)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_BUFINFO) fail(%d)\n", hd_ret);
					}
					BsMuxFrmCnt[ImgLink[tb.idx].bsmux_o[1]] = 0;
					if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE)) {
						_LinkPortCntInc(&(ImgLinkStatus[tb.idx].bsmux_p[1]));
						_LinkPortCntInc(&(ImgLinkStatus[tb.idx].fileout_p[1]));
					}
				}
			} else {
				if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_1) ||
					(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2)) {
						_LinkPortCntInc(&(ImgLinkStatus[tb.idx].fileout_p[0]));
				}
			}
		} else {
			if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
				(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3)) {
				// set bsmux buffer address
				bsmux_buf.videnc.phy_addr = img_venc_bs_buf[tb.idx][tb.tbl].buf_info.phy_addr;
				bsmux_buf.videnc.buf_size = img_venc_bs_buf[tb.idx][tb.tbl].buf_info.buf_size;
				bsmux_buf.audenc.phy_addr = (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_PCM) ? aud_acap_bs_buf[0].buf_info.phy_addr : aud_aenc_bs_buf[0].buf_info.phy_addr;
				bsmux_buf.audenc.buf_size = (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_PCM) ? aud_acap_bs_buf[0].buf_info.buf_size : aud_aenc_bs_buf[0].buf_info.buf_size;
				if ((ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_ONLY) != _CFG_CLONE_EMR_ONLY && (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_LOOP) != _CFG_CLONE_EMR_LOOP) {    // clone
					if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[2], HD_BSMUX_PARAM_BUFINFO, (void*)&bsmux_buf)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_BUFINFO) fail(%d)\n", hd_ret);
					}
					BsMuxFrmCnt[ImgLink[tb.idx].bsmux_o[2]] = 0;
					if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE)) {
						_LinkPortCntInc(&(ImgLinkStatus[tb.idx].bsmux_p[2]));
						if (img_bsmux_2v1a_mode[tb.idx] == 0) {                                     // normal mode
							_LinkPortCntInc(&(ImgLinkStatus[tb.idx].fileout_p[2]));
						}
					}
				}
				if (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) {                                  // clone_emr
					if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[3], HD_BSMUX_PARAM_BUFINFO, (void*)&bsmux_buf)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_BUFINFO) fail(%d)\n", hd_ret);
					}
					BsMuxFrmCnt[ImgLink[tb.idx].bsmux_o[3]] = 0;
					if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE)) {
						_LinkPortCntInc(&(ImgLinkStatus[tb.idx].bsmux_p[3]));
						if (img_bsmux_2v1a_mode[tb.idx] == 0) {                                     // normal mode
							_LinkPortCntInc(&(ImgLinkStatus[tb.idx].fileout_p[3]));
						}
					}
				}
			} else {
				if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_1) ||
					(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2)) {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].fileout_p[2]));
				}
			}
		}

		img_emr_loop_start[tb.idx][tb.tbl] = 0;

		ret = _ImageApp_MovieMulti_ImgLinkStatusUpdate(i, UPDATE_REVERSE);
		if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3) ||
			(img_vproc_yuv_compress[tb.idx] && (img_venc_sout_type[tb.idx][tb.tbl] != MOVIE_IMGCAP_SOUT_NONE))) {
			vflag = FLGIAMOVIE_VE_M0 << (2 * tb.idx + tb.tbl);
			vos_flag_set(IAMOVIE_FLG_ID, vflag);
		}
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		// start aenc and get aenc buffer addr
		if (EthCamLinkRecInfo[tb.idx].rec_mode == _CFG_REC_TYPE_AV) {
			if ((EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_AAC) || (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_ULAW) || (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_ALAW)) {
				_ImageApp_MovieMulti_AudRecStart(0);
				_aenc_get_buf_info(0);
			} else if (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_PCM) {
				ImageApp_MovieMulti_AudCapStart(0);
			}
		}
		// Set port
		// set bsmux buffer address
		bsmux_buf.videnc.phy_addr = EthCamLinkRecInfo[tb.idx].buf_pa;
		bsmux_buf.videnc.buf_size = EthCamLinkRecInfo[tb.idx].buf_size;
		bsmux_buf.audenc.phy_addr = (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_PCM) ? aud_acap_bs_buf[0].buf_info.phy_addr : aud_aenc_bs_buf[0].buf_info.phy_addr;
		bsmux_buf.audenc.buf_size = (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_PCM) ? aud_acap_bs_buf[0].buf_info.buf_size : aud_aenc_bs_buf[0].buf_info.buf_size;
		if ((ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_ONLY) != _CFG_MAIN_EMR_ONLY && (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) != _CFG_MAIN_EMR_LOOP) {      // main
			if ((hd_ret = hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[0], HD_BSMUX_PARAM_BUFINFO, (void*)&bsmux_buf)) != HD_OK) {
				DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_BUFINFO) fail(%d)\n", hd_ret);
			}
			BsMuxFrmCnt[EthCamLink[tb.idx].bsmux_o[0]] = 0;
			_LinkPortCntInc(&(EthCamLinkStatus[tb.idx].bsmux_p[0]));
			_LinkPortCntInc(&(EthCamLinkStatus[tb.idx].fileout_p[0]));
		}
		if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {                                   // main_emr
			if ((hd_ret =hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[1], HD_BSMUX_PARAM_BUFINFO, (void*)&bsmux_buf)) != HD_OK) {
				DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_BUFINFO) fail(%d)\n", hd_ret);
			}
			BsMuxFrmCnt[EthCamLink[tb.idx].bsmux_o[1]] = 0;
			_LinkPortCntInc(&(EthCamLinkStatus[tb.idx].bsmux_p[1]));
			_LinkPortCntInc(&(EthCamLinkStatus[tb.idx].fileout_p[1]));
		}

		ethcam_emr_loop_start[tb.idx][0] = 0;

		ret = _ImageApp_MovieMulti_EthCamLinkStatusUpdate(i, UPDATE_REVERSE);
	}
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == DISABLE)
	FirstBsMuxPath = _ImageApp_MovieMulti_GetFirstBsMuxPort();
#endif

	vos_sem_sig(IAMOVIE_SEMID_OPERATION);
	return ret;
}

ER ImageApp_MovieMulti_RecStop(MOVIE_CFG_REC_ID id)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;
	UINT32 i = id, j, k, started_path = 0;
	UINT32 iport;
	FLGPTN vflag, uiFlag;
	MOVIE_TBL_IDX tb;

	vos_sem_wait(IAMOVIE_SEMID_OPERATION);

	if (ImageApp_MovieMulti_IsStreamRunning(i) == FALSE) {
		vos_sem_sig(IAMOVIE_SEMID_OPERATION);
		return E_SYS;
	}

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3)) {
			if (IsImgLinkMode3RecStart[tb.idx][tb.tbl*2] || IsImgLinkMode3RecStart[tb.idx][tb.tbl*2+1]) {
				DBG_ERR("Please call ImageApp_MovieMulti_Mode3_RecStop() before ImageApp_MovieMulti_RecStop()\r\n");
				vos_sem_sig(IAMOVIE_SEMID_OPERATION);
				return E_SYS;
			}
		}
		// check 2v1a mode
		if ((img_bsmux_2v1a_mode[tb.idx] != 0) && (tb.tbl == MOVIETYPE_MAIN)) {     // 2v1a mode
			if (ImageApp_MovieMulti_IsStreamRunning(_CFG_CLONE_ID_1 + i) == TRUE) {
				DBG_ERR("Clone path should stop first if 2v1a mode enabled!\r\n");
				vos_sem_sig(IAMOVIE_SEMID_OPERATION);
				return ret;
			}
		}
		if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3)) {
			// stop aenc
			if (ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode == _CFG_REC_TYPE_AV) {
				if ((ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_AAC) || (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_ULAW) || (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_ALAW)) {
					_ImageApp_MovieMulti_AudRecStop(0);
				} else if (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_PCM) {
					ImageApp_MovieMulti_AudCapStop(0);
				}
			}
		}
		if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3) ||
			(img_vproc_yuv_compress[tb.idx] && (img_venc_sout_type[tb.idx][tb.tbl] != MOVIE_IMGCAP_SOUT_NONE))) {
			vflag = FLGIAMOVIE_VE_M0 << (2 * tb.idx + tb.tbl);
			vos_flag_clr(IAMOVIE_FLG_ID, vflag);
			vos_flag_wait_timeout(&vflag, IAMOVIE_FLG_ID, FLGIAMOVIE_TSK_VE_IDLE, TWF_ORW, vos_util_msec_to_tick(100));
		}
		// get IME and UserProc port
		if (gSwitchLink[tb.idx][tb.tbl] == UNUSED) {
			DBG_ERR("gSwitchLink[%d][%d]=%d\r\n", tb.idx, tb.tbl, gSwitchLink[tb.idx][tb.tbl]);
			return E_SYS;
		}
		iport = gUserProcMap[tb.idx][gSwitchLink[tb.idx][tb.tbl]];     // 0:main, 1:clone
		if (iport > 2) {
			iport = 2;
		}
		// Set port
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		if (img_vproc_vpe_en[tb.idx] == MOVIE_VPE_NONE) {
#endif
			if (img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[tb.idx] == FALSE) {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][img_vproc_3dnr_path[tb.idx]]));
				} else {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][4]));
				}
			}
			_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vcap_p[0]));
			if (!((img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[tb.idx] == FALSE) && (iport == img_vproc_3dnr_path[tb.idx]))) {
				_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][iport]));
			}
			if (img_vproc_no_ext[tb.idx] == FALSE) {
				_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][tb.tbl]));
			}
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		} else {
			if (img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[tb.idx] == FALSE) {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][img_vproc_3dnr_path[tb.idx]]));
				} else {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][4]));
				}
			}
			_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vcap_p[0]));

			if (!((img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[tb.idx] == FALSE))) {
				_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][0]));
			}

			_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][iport]));
			if (img_vproc_no_ext[tb.idx] == FALSE) {
				_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][tb.tbl]));
			}
		}
#endif
		if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3) ||
			(img_vproc_yuv_compress[tb.idx] && (img_venc_sout_type[tb.idx][tb.tbl] != MOVIE_IMGCAP_SOUT_NONE))) {
			_LinkPortCntDec(&(ImgLinkStatus[tb.idx].venc_p[tb.tbl]));
		}
		if (tb.tbl == MOVIETYPE_MAIN) {
			if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
				(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3)) {
				if ((ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_ONLY) != _CFG_MAIN_EMR_ONLY && (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) != _CFG_MAIN_EMR_LOOP) {      // main
					if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE)) {
						_LinkPortCntDec(&(ImgLinkStatus[tb.idx].bsmux_p[0]));
						_LinkPortCntDec(&(ImgLinkStatus[tb.idx].fileout_p[0]));
					}
				}
				if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {                                   // main_emr
					if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE)) {
						_LinkPortCntDec(&(ImgLinkStatus[tb.idx].bsmux_p[1]));
						_LinkPortCntDec(&(ImgLinkStatus[tb.idx].fileout_p[1]));
					}
				}
			} else {
				if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_1) ||
					(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2)) {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].fileout_p[0]));
				}
			}
		} else {
			if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
				(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3)) {
				if ((ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_ONLY) != _CFG_CLONE_EMR_ONLY && (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_LOOP) != _CFG_CLONE_EMR_LOOP) {    // clone
					if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE)) {
						_LinkPortCntDec(&(ImgLinkStatus[tb.idx].bsmux_p[2]));
						if (img_bsmux_2v1a_mode[tb.idx] == 0) {                                     // normal mode
							_LinkPortCntDec(&(ImgLinkStatus[tb.idx].fileout_p[2]));
						}
					}
				}
				if (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) {                                  // clone_emr
					if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE)) {
						_LinkPortCntDec(&(ImgLinkStatus[tb.idx].bsmux_p[3]));
						if (img_bsmux_2v1a_mode[tb.idx] == 0) {                                     // normal mode
							_LinkPortCntDec(&(ImgLinkStatus[tb.idx].fileout_p[3]));
						}
					}
				}
			} else {
				if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_1) ||
					(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2)) {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].fileout_p[2]));
				}
			}
		}
		vflag = (FLGIAMOVIE2_JPG_M0_RDY | FLGIAMOVIE2_JPG_C0_RDY | FLGIAMOVIE2_THM_M0_RDY | FLGIAMOVIE2_THM_C0_RDY) << (2 * tb.idx + tb.tbl);
		if ((ret = vos_flag_wait_timeout(&uiFlag, IAMOVIE_FLG_ID2, vflag, TWF_ANDW, vos_util_msec_to_tick(1000))) != E_OK) {
			DBG_ERR("Wait rawenc done fail!(%d)\r\n", ret);
			ret = E_OK;
		}
		if (_ImageApp_MovieMulti_ImgLinkStatusUpdate(i, UPDATE_FORWARD) != E_OK) {
			ret = E_SYS;
		}

		// clear variables
		trig_once_limited[tb.idx][tb.tbl] = 0;
		trig_once_cnt[tb.idx][tb.tbl] = 0;
		trig_once_first_i[tb.idx][tb.tbl] = 0;
		trig_once_2v1a_stop_path[tb.idx][tb.tbl] = 0;
		img_venc_timestamp[tb.idx][tb.tbl] = 0;
		img_bsmux_cutnow_with_period_cnt[tb.idx][tb.tbl] = UNUSED;

		// reopen aenc if needed
		if (_ImageApp_MovieMulti_AudReopenAEnc(0) != E_OK) {
			ret = E_SYS;
		}

		// reopen venc if needed
		if (_ImageApp_MovieMulti_ImgReopenVEnc(tb.idx, tb.tbl) != E_OK) {
			ret = E_SYS;
		}

		// check whether need to reopen ImgCapLink
		for (j = 0; (j < MaxLinkInfo.MaxImgLink) && (j < MAX_IMG_PATH); j++) {
			for (k = 0; k < 2; k++) {
				if (ImgLinkStatus[j].venc_p[k]) {
					started_path ++;
				}
			}
		}
		if (iamovie_need_reopen && started_path == 0) {
			_ImageApp_MovieMulti_ImgCapLinkClose(0);
			_ImageApp_MovieMulti_ImgCapLinkOpen(0);
		}
		if (iamovie_use_filedb_and_naming == TRUE) {
			ImageApp_MovieMulti_InitCrash(id);
		}
		if (gpsdata_rec_va[tb.idx][tb.tbl]) {
			if ((hd_ret = hd_common_mem_free(gpsdata_rec_pa[tb.idx][tb.tbl], (void *)gpsdata_rec_va[tb.idx][tb.tbl])) != HD_OK) {
				DBG_ERR("hd_common_mem_free(gpsdata_rec_va[%d][%d]) failed(%d)\r\n", tb.idx, tb.tbl, hd_ret);
			}
			gpsdata_rec_pa[tb.idx][tb.tbl] = 0;
			gpsdata_rec_va[tb.idx][tb.tbl] = 0;
			gpsdata_rec_size[tb.idx][tb.tbl] = 0;
			gpsdata_rec_offset[tb.idx][tb.tbl] = 0;
		}
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		// stop aenc
		if (EthCamLinkRecInfo[tb.idx].rec_mode == _CFG_REC_TYPE_AV) {
			if ((EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_AAC) || (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_ULAW) || (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_ALAW)) {
				_ImageApp_MovieMulti_AudRecStop(0);
			} else if (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_PCM) {
				ImageApp_MovieMulti_AudCapStop(0);
			}
		}
		// Set port
		if ((ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_ONLY) != _CFG_MAIN_EMR_ONLY && (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) != _CFG_MAIN_EMR_LOOP) {      // main
			_LinkPortCntDec(&(EthCamLinkStatus[tb.idx].bsmux_p[0]));
			_LinkPortCntDec(&(EthCamLinkStatus[tb.idx].fileout_p[0]));
		}
		if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {                                   // main_emr
			_LinkPortCntDec(&(EthCamLinkStatus[tb.idx].bsmux_p[1]));
			_LinkPortCntDec(&(EthCamLinkStatus[tb.idx].fileout_p[1]));
		}
		if (_ImageApp_MovieMulti_EthCamLinkStatusUpdate(i, UPDATE_FORWARD) != E_OK) {
			ret = E_SYS;
		}

		// clear variables
		ethcam_venc_timestamp[tb.idx] = 0;
		ethcam_bsmux_cutnow_with_period_cnt[tb.idx][0] = UNUSED;

		// reopen aenc if needed
		if (_ImageApp_MovieMulti_AudReopenAEnc(0) != E_OK) {
			ret = E_SYS;
		}

		if (iamovie_use_filedb_and_naming == TRUE) {
			ImageApp_MovieMulti_InitCrash(id);
		}
		if (gpsdata_eth_va[tb.idx][0]) {
			if ((hd_ret = hd_common_mem_free(gpsdata_eth_pa[tb.idx][0], (void *)gpsdata_eth_va[tb.idx][0])) != HD_OK) {
				DBG_ERR("hd_common_mem_free(gpsdata_eth_va[%d][0]) failed(%d)\r\n", tb.idx, hd_ret);
			}
			gpsdata_eth_pa[tb.idx][0] = 0;
			gpsdata_eth_va[tb.idx][0] = 0;
			gpsdata_eth_size[tb.idx][0] = 0;
			gpsdata_eth_offset[tb.idx][0] = 0;
		}
	}
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == DISABLE)
	FirstBsMuxPath = _ImageApp_MovieMulti_GetFirstBsMuxPort();
#endif

	if (ret == E_OK && g_ia_moviemulti_usercb) {
		if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
			if (!((img_bsmux_2v1a_mode[tb.idx] != 0) && (tb.tbl == MOVIETYPE_CLONE))) {
				g_ia_moviemulti_usercb(i, MOVIE_USER_CB_EVENT_REC_STOP, img_pseudo_rec_mode[tb.idx][tb.tbl]);
			}
		} else {
			g_ia_moviemulti_usercb(i, MOVIE_USER_CB_EVENT_REC_STOP, 0);
		}
	}

	vos_sem_sig(IAMOVIE_SEMID_OPERATION);
	return ret;
}

ER _ImageApp_MovieMulti_TriggerThumb(MOVIE_CFG_REC_ID id, UINT32 is_emr)
{
	UINT32 i = id;
	ER ret = E_SYS;
	MOVIE_TBL_IDX tb;
	FLGPTN vflag;
	IMGCAP_JOB_ELEMENT *pJob = NULL;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if (IsImgLinkTranscodeMode[tb.idx][tb.tbl]) {
			DBG_WRN("Not support thumb in transcode mode.\r\n");
			return E_NOSPT;
		}
		if ((ImageApp_MovieMulti_IsStreamRunning(i) == TRUE) || (ImageApp_MovieMulti_IsStreamRunning((i | ETHCAM_TX_MAGIC_KEY)) == TRUE)) {
			if ((pJob = (IMGCAP_JOB_ELEMENT *)_QUEUE_DeQueueFromHead(&ImgCapJobQueueClear)) != NULL) {
				pJob->type   = IMGCAP_THUM;
				pJob->path   = i;
				if (!ImgCapExifFuncEn && imgcap_thum_with_exif) {
					DBG_WRN("Enable thumbnail exif but exif funtion is not enabled.\r\n");
				}
				pJob->exif   = (ImgCapExifFuncEn && imgcap_thum_with_exif) ? TRUE : FALSE;
				pJob->is_emr = is_emr;

				_QUEUE_EnQueueToTail(&ImgCapJobQueueReady, (QUEUE_BUFFER_ELEMENT *)pJob);
				vflag = FLGIAMOVIE2_THM_M0_RDY << (2 * tb.idx + tb.tbl);
				vos_flag_clr(IAMOVIE_FLG_ID2, vflag);
				vos_flag_set(IAMOVIE_FLG_ID2, FLGIAMOVIE2_VE_IMGCAP);
				ret = E_OK;
			}
		}
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		if (ethcam_venc_imgcap_on_rx[tb.idx] == TRUE) {
			if (ImageApp_MovieMulti_IsStreamRunning(i) == TRUE) {
				if ((pJob = (IMGCAP_JOB_ELEMENT *)_QUEUE_DeQueueFromHead(&ImgCapJobQueueClear)) != NULL) {
					pJob->type   = IMGCAP_THUM;
					pJob->path   = i;
					if (!ImgCapExifFuncEn && imgcap_thum_with_exif) {
						DBG_WRN("Enable thumbnail exif but exif funtion is not enabled.\r\n");
					}
					pJob->exif   = (ImgCapExifFuncEn && imgcap_thum_with_exif) ? TRUE : FALSE;
					pJob->is_emr = is_emr;

					_QUEUE_EnQueueToTail(&ImgCapJobQueueReady, (QUEUE_BUFFER_ELEMENT *)pJob);
					vflag = FLGIAMOVIE2_THM_E0_RDY << tb.idx;
					vos_flag_clr(IAMOVIE_FLG_ID2, vflag);
					vos_flag_set(IAMOVIE_FLG_ID2, FLGIAMOVIE2_VE_IMGCAP);
					ret = E_OK;
				}
			}
		} else {
			if (g_ia_moviemulti_usercb) {
				g_ia_moviemulti_usercb(i, MOVIE_USRR_CB_EVENT_TRIGGER_ETHCAM_THUMB, is_emr);
			}
		}
	}
	return ret;
}

ER ImageApp_MovieMulti_TriggerSnapshot(MOVIE_CFG_REC_ID id)
{
	UINT32 i = id;
	ER ret = E_SYS;
	MOVIE_TBL_IDX tb;
	FLGPTN vflag;
	IMGCAP_JOB_ELEMENT *pJob = NULL;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if (IsImgLinkTranscodeMode[tb.idx][tb.tbl]) {
			DBG_WRN("Not support snapshot in transcode mode.\r\n");
			return E_NOSPT;
		}
		if ((ImageApp_MovieMulti_IsStreamRunning(i) == TRUE) || (ImageApp_MovieMulti_IsStreamRunning((i | ETHCAM_TX_MAGIC_KEY)) == TRUE)) {
			if ((pJob = (IMGCAP_JOB_ELEMENT *)_QUEUE_DeQueueFromHead(&ImgCapJobQueueClear)) != NULL) {
				pJob->type = IMGCAP_JSTM;
				pJob->path = i;
				pJob->exif = ImgCapExifFuncEn ? TRUE : FALSE;
				_QUEUE_EnQueueToTail(&ImgCapJobQueueReady, (QUEUE_BUFFER_ELEMENT *)pJob);
				vflag = FLGIAMOVIE2_JPG_M0_RDY << (2 * tb.idx + tb.tbl);
				vos_flag_clr(IAMOVIE_FLG_ID2, vflag);
				vos_flag_set(IAMOVIE_FLG_ID2, FLGIAMOVIE2_VE_IMGCAP);
				ret = E_OK;
			}
		}
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		if (ImageApp_MovieMulti_IsStreamRunning(i) == TRUE) {
			if ((pJob = (IMGCAP_JOB_ELEMENT *)_QUEUE_DeQueueFromHead(&ImgCapJobQueueClear)) != NULL) {
				pJob->type = IMGCAP_JSTM;
				pJob->path = i;
				pJob->exif = ImgCapExifFuncEn ? TRUE : FALSE;
				_QUEUE_EnQueueToTail(&ImgCapJobQueueReady, (QUEUE_BUFFER_ELEMENT *)pJob);
				vflag = FLGIAMOVIE2_JPG_E0_RDY << tb.idx;
				vos_flag_clr(IAMOVIE_FLG_ID2, vflag);
				vos_flag_set(IAMOVIE_FLG_ID2, FLGIAMOVIE2_VE_IMGCAP);
				ret = E_OK;
			}
		}
	}
	return ret;
}

ER ImageApp_MovieMulti_TrigEMR(MOVIE_CFG_REC_ID id)
{
#if (BSMUX_USE_NEW_EMR_API == ENABLE)
	UINT32 i = id, j;
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	MOVIE_TBL_IDX tb;
	HD_PATH_ID bsmux_ctrl_path = 0;
	HD_BSMUX_EN_UTIL emr_util = {0};
	HD_BSMUX_TRIG_EVENT emr_event = {0};

	if (i == _CFG_CTRL_ID) {
		for (j = 0; j < MaxLinkInfo.MaxImgLink; j++) {
			if (ImageApp_MovieMulti_IsStreamRunning((_CFG_REC_ID_1 + j)) == TRUE) {
				if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {
					if (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_PAUSE) {
						emr_util.enable  = TRUE;
						emr_util.resv[0] = TRUE;                            // EMR pause
						emr_util.resv[1] = ImgLink[j].bsmux_p[0];           // paused path
					} else {
						emr_util.enable  = TRUE;
					}
					emr_util.type = HD_BSMUX_EN_UTIL_EMERGENCY;
					hd_bsmux_set(ImgLink[j].bsmux_p[1], HD_BSMUX_PARAM_EN_UTIL, &emr_util);
					if (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) {
						img_emr_loop_start[j][0] = TRUE;
						BsMuxFrmCnt[ImgLink[j].bsmux_o[1]] %= ImgLinkRecInfo[j][0].frame_rate;
					}
				}
			}
			if (ImageApp_MovieMulti_IsStreamRunning((_CFG_CLONE_ID_1 + j)) == TRUE) {
				if (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) {
					if (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_PAUSE) {
						emr_util.enable  = TRUE;
						emr_util.resv[0] = TRUE;                            // EMR pause
						emr_util.resv[1] = ImgLink[j].bsmux_p[2];           // paused path
					} else {
						emr_util.enable  = TRUE;
					}
					emr_util.type = HD_BSMUX_EN_UTIL_EMERGENCY;
					hd_bsmux_set(ImgLink[j].bsmux_p[3], HD_BSMUX_PARAM_EN_UTIL, &emr_util);
					if (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_LOOP) {
						img_emr_loop_start[j][1] = TRUE;
						BsMuxFrmCnt[ImgLink[j].bsmux_o[3]] %= ImgLinkRecInfo[j][1].frame_rate;
					}
				}
			}
		}
		for (j = 0; j < MaxLinkInfo.MaxEthCamLink; j++) {
			if (ImageApp_MovieMulti_IsStreamRunning((_CFG_ETHCAM_ID_1 + j)) == TRUE) {
				if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {
					if (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_PAUSE) {
						emr_util.enable  = TRUE;
						emr_util.resv[0] = TRUE;                            // EMR pause
						emr_util.resv[1] = EthCamLink[j].bsmux_p[0];        // paused path
					} else {
						emr_util.enable  = TRUE;
					}
					emr_util.type = HD_BSMUX_EN_UTIL_EMERGENCY;
					hd_bsmux_set(EthCamLink[j].bsmux_p[1], HD_BSMUX_PARAM_EN_UTIL, &emr_util);
					if (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) {
						ethcam_emr_loop_start[j][0] = TRUE;
						BsMuxFrmCnt[EthCamLink[j].bsmux_o[1]] %= EthCamLinkRecInfo[j].vfr;
					}
				}
			}
		}
		if ((hd_ret = bsmux_get_ctrl_path(0, &bsmux_ctrl_path)) != HD_OK) {
			DBG_ERR("bsmux_get_ctrl_path() fail(%d)\n", hd_ret);
		} else {
			emr_event.type = HD_BSMUX_TRIG_EVENT_EMERGENCY;
			if ((hd_ret = hd_bsmux_set(bsmux_ctrl_path , HD_BSMUX_PARAM_TRIG_EVENT, &emr_event)) != HD_OK) {
				DBG_ERR("hd_bsmux_set(0x%x, HD_BSMUX_TRIG_EVENT_EMERGENCY) fail(%d)\n", bsmux_ctrl_path, hd_ret);
			} else {
				ret = E_OK;
			}
		}
	} else {
		if (ImageApp_MovieMulti_IsStreamRunning(i) == FALSE) {
			DBG_ERR("id%d is not running.\r\n", i);
			return ret;
		}
		_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

		if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
			if (ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode == _CFG_REC_TYPE_TIMELAPSE || ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode == _CFG_REC_TYPE_GOLFSHOT) {
				DBG_ERR("Not support EMR in timelapse or golfshot mode(%d).\r\n", ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode);
				ret = E_NOSPT;
				return ret;
			}
			if (tb.tbl == MOVIETYPE_MAIN) {
				if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {
					if (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_PAUSE) {
						emr_util.enable  = TRUE;
						emr_util.resv[0] = TRUE;                            // EMR pause
						emr_util.resv[1] = ImgLink[tb.idx].bsmux_p[0];      // paused path
					} else {
						emr_util.enable  = TRUE;
					}
					emr_util.type = HD_BSMUX_EN_UTIL_EMERGENCY;
					hd_bsmux_set(ImgLink[tb.idx].bsmux_p[1], HD_BSMUX_PARAM_EN_UTIL, &emr_util);
					emr_event.type = HD_BSMUX_TRIG_EVENT_EMERGENCY;
					if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[1], HD_BSMUX_PARAM_TRIG_EVENT, &emr_event)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(0x%x, HD_BSMUX_TRIG_EVENT_EMERGENCY) fail(%d)\n", ImgLink[tb.idx].bsmux_p[1], hd_ret);
					} else {
						if (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) {
							img_emr_loop_start[tb.idx][tb.tbl] = TRUE;
							BsMuxFrmCnt[ImgLink[tb.idx].bsmux_o[1]] %= ImgLinkRecInfo[tb.idx][0].frame_rate;
						}
						ret = E_OK;
					}
				}
			} else if (tb.tbl == MOVIETYPE_CLONE) {
				if (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) {
					if (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_PAUSE) {
						emr_util.enable  = TRUE;
						emr_util.resv[0] = TRUE;                            // EMR pause
						emr_util.resv[1] = ImgLink[tb.idx].bsmux_p[2];      // paused path
					} else {
						emr_util.enable  = TRUE;
					}
					emr_util.type = HD_BSMUX_EN_UTIL_EMERGENCY;
					hd_bsmux_set(ImgLink[tb.idx].bsmux_p[3], HD_BSMUX_PARAM_EN_UTIL, &emr_util);
					emr_event.type = HD_BSMUX_TRIG_EVENT_EMERGENCY;
					if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[3], HD_BSMUX_PARAM_TRIG_EVENT, &emr_event)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(0x%x, HD_BSMUX_TRIG_EVENT_EMERGENCY) fail(%d)\n", ImgLink[tb.idx].bsmux_p[3], hd_ret);
					} else {
						if (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_LOOP) {
							img_emr_loop_start[tb.idx][tb.tbl] = TRUE;
							BsMuxFrmCnt[ImgLink[tb.idx].bsmux_o[3]] %= ImgLinkRecInfo[tb.idx][1].frame_rate;
						}
						ret = E_OK;
					}
				}
			}
		} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
			if (EthCamLinkRecInfo[tb.idx].rec_mode == _CFG_REC_TYPE_TIMELAPSE || EthCamLinkRecInfo[tb.idx].rec_mode == _CFG_REC_TYPE_GOLFSHOT) {
				DBG_ERR("Not support EMR in timelapse or golfshot mode(%d).\r\n", EthCamLinkRecInfo[tb.idx].rec_mode);
				ret = E_NOSPT;
				return ret;
			}
			if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {
				if (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_PAUSE) {
					emr_util.enable  = TRUE;
					emr_util.resv[0] = TRUE;                                // EMR pause
					emr_util.resv[1] = EthCamLink[tb.idx].bsmux_p[0];       // paused path
				} else {
					emr_util.enable  = TRUE;
				}
				emr_util.type = HD_BSMUX_EN_UTIL_EMERGENCY;
				hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[1], HD_BSMUX_PARAM_EN_UTIL, &emr_util);
				emr_event.type = HD_BSMUX_TRIG_EVENT_EMERGENCY;
				if ((hd_ret = hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[1], HD_BSMUX_PARAM_TRIG_EVENT, &emr_event)) != HD_OK) {
					DBG_ERR("hd_bsmux_set(0x%x, HD_BSMUX_TRIG_EVENT_EMERGENCY) fail(%d)\n", EthCamLink[tb.idx].bsmux_p[1], hd_ret);
				} else {
					if (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) {
						ethcam_emr_loop_start[tb.idx][0] = TRUE;
						BsMuxFrmCnt[EthCamLink[tb.idx].bsmux_o[1]] %= EthCamLinkRecInfo[tb.idx].vfr;
					}
					ret = E_OK;
				}
			}
		}
	}
	return ret;
#else  // (BSMUX_USE_NEW_EMR_API == ENABLE)
	UINT32 i = id;
	ER ret = E_SYS;
	MOVIE_TBL_IDX tb;
	HD_BSMUX_TRIG_EMR emr_param = {0};

	if (ImageApp_MovieMulti_IsStreamRunning(i) == FALSE) {
		DBG_ERR("id%d is not running.\r\n", i);
		return ret;
	}
	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if (ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode == _CFG_REC_TYPE_TIMELAPSE || ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode == _CFG_REC_TYPE_GOLFSHOT) {
			DBG_ERR("Not support EMR in timelapse or golfshot mode(%d).\r\n", ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode);
			ret = E_NOSPT;
			return ret;
		}
		if (tb.tbl == MOVIETYPE_MAIN) {
			if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {
				if (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_PAUSE) {
					emr_param.emr_on = TRUE;
					emr_param.pause_on = TRUE;
					emr_param.pause_id = ImgLink[tb.idx].bsmux_p[0];
				} else {
					emr_param.emr_on = TRUE;
				}
				if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[1], HD_BSMUX_PARAM_TRIG_EMR, &emr_param)) != HD_OK) {
					DBG_ERR("hd_bsmux_set(0x%x, HD_BSMUX_PARAM_TRIG_EMR) fail(%d)\n", ImgLink[tb.idx].bsmux_p[1], hd_ret);
				} else {
					ret = E_OK;
				}
			}
		} else if (tb.tbl == MOVIETYPE_CLONE) {
			if (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) {
				if (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_PAUSE) {
					emr_param.emr_on = TRUE;
					emr_param.pause_on = TRUE;
					emr_param.pause_id = ImgLink[tb.idx].bsmux_p[2];
				} else {
					emr_param.emr_on = TRUE;
				}
				if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[3], HD_BSMUX_PARAM_TRIG_EMR, &emr_param)) != HD_OK) {
					DBG_ERR("hd_bsmux_set(0x%x, HD_BSMUX_PARAM_TRIG_EMR) fail(%d)\n", ImgLink[tb.idx].bsmux_p[3], hd_ret);
				} else {
					ret = E_OK;
				}
			}
		}
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		if (EthCamLinkRecInfo[tb.idx].rec_mode == _CFG_REC_TYPE_TIMELAPSE || EthCamLinkRecInfo[tb.idx].rec_mode == _CFG_REC_TYPE_GOLFSHOT) {
			DBG_ERR("Not support EMR in timelapse or golfshot mode(%d).\r\n", EthCamLinkRecInfo[tb.idx].rec_mode);
			ret = E_NOSPT;
			return ret;
		}
		if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {
			if (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_PAUSE) {
				emr_param.emr_on = TRUE;
				emr_param.pause_on = TRUE;
				emr_param.pause_id = ImgLink[tb.idx].bsmux_p[0];
			} else {
				emr_param.emr_on = TRUE;
			}
			if ((hd_ret = hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[1], HD_BSMUX_PARAM_TRIG_EMR, &emr_param)) != HD_OK) {
				DBG_ERR("hd_bsmux_set(0x%x, HD_BSMUX_PARAM_TRIG_EMR) fail(%d)\n", EthCamLink[tb.idx].bsmux_p[1], hd_ret);
			} else {
				ret = E_OK;
			}
		}
	}
	return ret;
#endif // (BSMUX_USE_NEW_EMR_API == ENABLE)
}

ER ImageApp_MovieMulti_TrigOnce(MOVIE_CFG_REC_ID id, UINT32 sec)
{
	UINT32 i = id;
	ER ret = E_SYS;
	MOVIE_TBL_IDX tb;

	if (ImageApp_MovieMulti_IsStreamRunning(i) == TRUE) {
		DBG_ERR("id%d is running.\r\n", i);
		return ret;
	}

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if (ImgLinkFileInfo.seamless_sec >= sec) {
			if (_ImageApp_MovieMulti_IsBGTskRunning() == FALSE) {
				_ImageApp_MovieMulti_CreateBGTsk();
			}
			trig_once_limited[tb.idx][tb.tbl] = ImgLinkRecInfo[tb.idx][tb.tbl].frame_rate * sec;
			trig_once_cnt[tb.idx][tb.tbl] = 0;
			trig_once_first_i[tb.idx][tb.tbl] = FALSE;
			ImageApp_MovieMulti_RecStart(i);
			ret = E_OK;
		} else {
			DBG_ERR("TrigOnce second(%d) > ImgLinkFileInfo.seamless_sec(%d)\r\n", sec, ImgLinkFileInfo.seamless_sec);
		}
	}
	return ret;
}

#define COMBINATION_LCOK         0x30348899
static UINT32 combination_lock = 0;

void _assign_key_(UINT32 num)
{
	combination_lock = num;
}

ER ImageApp_MovieMulti_EthCamTxStart(MOVIE_CFG_REC_ID id)
{
	ER ret = E_SYS;
	HD_RESULT hd_ret;
	UINT32 i = id;
	UINT32 iport;
	FLGPTN vflag;
	MOVIE_TBL_IDX tb;

	if (combination_lock != COMBINATION_LCOK) {
		DBG_ERR("EthCam Tx function is not support on NT96580\r\n");
		return E_SYS;
	}

	vos_sem_wait(IAMOVIE_SEMID_OPERATION);

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if (IsImgLinkForEthCamTxEnabled[tb.idx][tb.tbl] == TRUE) {
			DBG_WRN("id %d is already started!\r\n", i);
			vos_sem_sig(IAMOVIE_SEMID_OPERATION);
			return ret;
		}

		#if _TODO
		if (MaxLinkInfo.MaxAudCapLink) {
			_ImageApp_MovieMulti_EthCamTxRecId1_GetAudCapAddr(20*1024);
			_ImageApp_MovieMulti_EthCamTxRecId1_InitAudCapParam();
		}
		#endif

		if ((tb.tbl == MOVIETYPE_CLONE) && (img_venc_trigger_mode[tb.idx][tb.tbl] == MOVIE_CODEC_TRIGGER_DIRECT)) {
			UINT32 frc = img_venc_in_param[tb.idx][tb.tbl].frc;
			img_venc_in_param[tb.idx][tb.tbl].frc = _ImageApp_MovieMulti_frc(1, 1);
			IACOMM_VENC_PARAM venc_param = {
				.video_enc_path = ImgLink[tb.idx].venc_p[tb.tbl],
				.pin            = &(img_venc_in_param[tb.idx][tb.tbl]),
				.pout           = NULL,
				.prc            = NULL,
			};
			if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
				DBG_ERR("venc_set_param fail(%d)\n", ret);
			}
			img_venc_in_param[tb.idx][tb.tbl].frc = frc;
		}

		// get venc buffer addr
		_img_venc_get_buf_info(tb.idx, tb.tbl);
		// start aenc and get aenc buffer addr
		if (ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode == _CFG_REC_TYPE_AV) {
			if ((ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_AAC) || (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_ULAW) || (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_ALAW)) {
				_ImageApp_MovieMulti_AudRecStart(0);
				_aenc_get_buf_info(0);
			} else if (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_PCM) {
				ImageApp_MovieMulti_AudCapStart(0);
			}
		}

		// get IME and UserProc port
		if (gSwitchLink[tb.idx][tb.tbl] == UNUSED) {
			DBG_ERR("gSwitchLink[%d][%d]=%d\r\n", tb.idx, tb.tbl, gSwitchLink[tb.idx][tb.tbl]);
			return E_SYS;
		}
		iport = gUserProcMap[tb.idx][gSwitchLink[tb.idx][tb.tbl]];     // 0:main, 1:clone
		if (iport > 2) {
			iport = 2;
		}
		// Set port
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		if (img_vproc_vpe_en[tb.idx] == MOVIE_VPE_NONE) {
#endif
			if (img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[tb.idx] == FALSE) {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][img_vproc_3dnr_path[tb.idx]]));
				} else {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][4]));
				}
			}
			_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vcap_p[0]));
			if (!((img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[tb.idx] == FALSE) && (iport == img_vproc_3dnr_path[tb.idx]))) {
				_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][iport]));
			}
			if (img_vproc_no_ext[tb.idx] == FALSE) {
				_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][tb.tbl]));
			}
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		} else {
			if (img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[tb.idx] == FALSE) {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][img_vproc_3dnr_path[tb.idx]]));
				} else {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][4]));
				}
			}
			_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vcap_p[0]));

			if (!((img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[tb.idx] == FALSE))) {
				_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][0]));
			}

			_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][iport]));
			if (img_vproc_no_ext[tb.idx] == FALSE) {
				_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][tb.tbl]));
			}
		}
#endif
		_LinkPortCntInc(&(ImgLinkStatus[tb.idx].venc_p[tb.tbl]));

#if (defined(_BSP_NA51055_) || defined(_BSP_NA51089_))
		VENDOR_VIDEOENC_H26X_SKIP_FRM_CFG skip = {0};
		skip.b_enable = img_venc_skip_frame_cfg[tb.idx][tb.tbl] ? ENABLE : DISABLE;
		skip.target_fr = GET_HI_UINT16(img_venc_skip_frame_cfg[tb.idx][tb.tbl]);
		skip.input_frm_cnt = GET_LO_UINT16(img_venc_skip_frame_cfg[tb.idx][tb.tbl]);
		vendor_videoenc_set(ImgLink[tb.idx].venc_p[tb.tbl], VENDOR_VIDEOENC_PARAM_OUT_H26X_SKIP_FRM, &skip);
#endif

		ret = _ImageApp_MovieMulti_ImgLinkStatusUpdate(i, UPDATE_REVERSE);
		vflag = FLGIAMOVIE_VE_M0 << (2 * tb.idx + tb.tbl);
		vos_flag_set(IAMOVIE_FLG_ID, vflag);

		IsImgLinkForEthCamTxEnabled[tb.idx][tb.tbl] = TRUE;
		ret = E_OK;
	}
	vos_sem_sig(IAMOVIE_SEMID_OPERATION);
	return ret;
}

ER ImageApp_MovieMulti_EthCamTxStop(MOVIE_CFG_REC_ID id)
{
	ER ret = E_OK;
	UINT32 i = id, j, k, started_path = 0;
	UINT32 iport;
	FLGPTN vflag;
	MOVIE_TBL_IDX tb;

	vos_sem_wait(IAMOVIE_SEMID_OPERATION);

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if (IsImgLinkForEthCamTxEnabled[tb.idx][tb.tbl] == FALSE) {
			DBG_WRN("id %d is already stopped!\r\n", i);
			ret = E_SYS;
			vos_sem_sig(IAMOVIE_SEMID_OPERATION);
			return ret;
		}

		// stop aenc
		if (ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode == _CFG_REC_TYPE_AV) {
			if ((ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_AAC) || (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_ULAW) || (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_ALAW)) {
				_ImageApp_MovieMulti_AudRecStop(0);
			} else if (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_PCM) {
				ImageApp_MovieMulti_AudCapStop(0);
			}
		}
		vflag = FLGIAMOVIE_VE_M0 << (2 * tb.idx + tb.tbl);
		vos_flag_clr(IAMOVIE_FLG_ID, vflag);
		vos_flag_wait_timeout(&vflag, IAMOVIE_FLG_ID, FLGIAMOVIE_TSK_VE_IDLE, TWF_ORW, vos_util_msec_to_tick(100));

		// get IME and UserProc port
		if (gSwitchLink[tb.idx][tb.tbl] == UNUSED) {
			DBG_ERR("gSwitchLink[%d][%d]=%d\r\n", tb.idx, tb.tbl, gSwitchLink[tb.idx][tb.tbl]);
			return E_SYS;
		}
		iport = gUserProcMap[tb.idx][gSwitchLink[tb.idx][tb.tbl]];     // 0:main, 1:clone
		if (iport > 2) {
			iport = 2;
		}
		// Set port
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		if (img_vproc_vpe_en[tb.idx] == MOVIE_VPE_NONE) {
#endif
			if (img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[tb.idx] == FALSE) {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][img_vproc_3dnr_path[tb.idx]]));
				} else {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][4]));
				}
			}
			_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vcap_p[0]));
			if (!((img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[tb.idx] == FALSE) && (iport == img_vproc_3dnr_path[tb.idx]))) {
				_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][iport]));
			}
			if (img_vproc_no_ext[tb.idx] == FALSE) {
				_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][tb.tbl]));
			}
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		} else {
			if (img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[tb.idx] == FALSE) {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][img_vproc_3dnr_path[tb.idx]]));
				} else {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][4]));
				}
			}
			_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vcap_p[0]));

			if (!((img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[tb.idx] == FALSE))) {
				_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][0]));
			}

			_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][iport]));
			if (img_vproc_no_ext[tb.idx] == FALSE) {
				_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][tb.tbl]));
			}
		}
#endif
		_LinkPortCntDec(&(ImgLinkStatus[tb.idx].venc_p[tb.tbl]));

		if (_ImageApp_MovieMulti_ImgLinkStatusUpdate(i, UPDATE_FORWARD) != E_OK) {
			ret = E_SYS;
		}

		// reopen aenc if needed
		if (_ImageApp_MovieMulti_AudReopenAEnc(0) != E_OK) {
			ret = E_SYS;
		}

		// reopen venc if needed
		if (_ImageApp_MovieMulti_ImgReopenVEnc(tb.idx, tb.tbl) != E_OK) {
			ret = E_SYS;
		}

		// check whether need to reopen ImgCapLink
		for (j = 0; (j < MaxLinkInfo.MaxImgLink) && (j < MAX_IMG_PATH); j++) {
			for (k = 0; k < 2; k++) {
				if (ImgLinkStatus[j].venc_p[k]) {
					started_path ++;
				}
			}
		}
		if (iamovie_need_reopen && started_path == 0) {
			_ImageApp_MovieMulti_ImgCapLinkClose(0);
			_ImageApp_MovieMulti_ImgCapLinkOpen(0);
		}

		#if _TODO
		if (MaxLinkInfo.MaxAudCapLink) {
			_ImageApp_MovieMulti_EthCamTxRecId1_DestroyAudCapBuff();
		}
		#endif

		IsImgLinkForEthCamTxEnabled[tb.idx][tb.tbl] = FALSE;
	}
	vos_sem_sig(IAMOVIE_SEMID_OPERATION);
	return ret;
}

ER ImageApp_MovieMulti_EthCam_TxTriggerThumb(MOVIE_CFG_REC_ID id)
{
	ER ret = E_SYS;
	UINT32 i = id;

	if ((i & ETHCAM_TX_MAGIC_KEY) != ETHCAM_TX_MAGIC_KEY) {
		return ret;
	}
	i &= ~ETHCAM_TX_MAGIC_KEY;

	ret = _ImageApp_MovieMulti_TriggerThumb(i, FALSE);

	return ret;
}

ER ImageApp_MovieMulti_TranscodeStart(MOVIE_CFG_REC_ID id)
{
	HD_RESULT hd_ret;
	ER ret = E_SYS;
	UINT32 i = id;
	FLGPTN vflag;
	MOVIE_TBL_IDX tb;
	HD_BSMUX_BUFINFO bsmux_buf;

	vos_sem_wait(IAMOVIE_SEMID_OPERATION);

	if (ImageApp_MovieMulti_IsStreamRunning(i) == TRUE) {
		vos_sem_sig(IAMOVIE_SEMID_OPERATION);
		return ret;
	}

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		// check 2v1a mode
		if ((img_bsmux_2v1a_mode[tb.idx] != 0) && (tb.tbl == MOVIETYPE_MAIN)) {     // 2v1a mode
			if (ImageApp_MovieMulti_IsStreamRunning(_CFG_CLONE_ID_1 + i) == FALSE) {
				DBG_ERR("Clone path should start first if 2v1a mode enabled!\r\n");
				vos_sem_sig(IAMOVIE_SEMID_OPERATION);
				return ret;
			}
		}
		if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3)) {
			// get venc buffer addr
			_img_venc_get_buf_info(tb.idx, tb.tbl);
			// start aenc and get aenc buffer addr
			if (ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode == _CFG_REC_TYPE_AV) {
				ImageApp_MovieMulti_AudTranscodeStart(0);
				_aenc_get_buf_info(0);
			}
		}

		// force set venc to trigger mode
		if (1) {
			UINT32 frc = img_venc_in_param[tb.idx][tb.tbl].frc;
			img_venc_in_param[tb.idx][tb.tbl].frc = _ImageApp_MovieMulti_frc(1, 1);
			IACOMM_VENC_PARAM venc_param = {
				.video_enc_path = ImgLink[tb.idx].venc_p[tb.tbl],
				.pin            = &(img_venc_in_param[tb.idx][tb.tbl]),
				.pout           = NULL,
				.prc            = NULL,
			};
			if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
				DBG_ERR("venc_set_param fail(%d)\n", ret);
			}
			img_venc_in_param[tb.idx][tb.tbl].frc = frc;
		}

		if (img_bsmux_2v1a_mode[tb.idx] == 0) {                                     // normal_mode & pseudo_rec_mode_2
			if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
				(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2) ||
				(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3) ||
				(img_vproc_yuv_compress[tb.idx] && (img_venc_sout_type[tb.idx][tb.tbl] != MOVIE_IMGCAP_SOUT_NONE))) {
				_LinkPortCntInc(&(ImgLinkStatus[tb.idx].venc_p[tb.tbl]));
#if (defined(_BSP_NA51055_) || defined(_BSP_NA51089_))
				VENDOR_VIDEOENC_H26X_SKIP_FRM_CFG skip = {0};
				skip.b_enable = img_venc_skip_frame_cfg[tb.idx][tb.tbl] ? ENABLE : DISABLE;
				skip.target_fr = GET_HI_UINT16(img_venc_skip_frame_cfg[tb.idx][tb.tbl]);
				skip.input_frm_cnt = GET_LO_UINT16(img_venc_skip_frame_cfg[tb.idx][tb.tbl]);
				vendor_videoenc_set(ImgLink[tb.idx].venc_p[tb.tbl], VENDOR_VIDEOENC_PARAM_OUT_H26X_SKIP_FRM, &skip);
#endif
			}
		} else {                                                                    // 2v1a mode
			if (tb.tbl == MOVIETYPE_MAIN) {
				if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
					(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2) ||
					(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3)) {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].venc_p[0]));
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].venc_p[1]));
#if (defined(_BSP_NA51055_) || defined(_BSP_NA51089_))
					VENDOR_VIDEOENC_H26X_SKIP_FRM_CFG skip = {0};
					UINT32 j;
					for (j = 0; i < 2; j++) {
						skip.b_enable = img_venc_skip_frame_cfg[tb.idx][j] ? ENABLE : DISABLE;
						skip.target_fr = GET_HI_UINT16(img_venc_skip_frame_cfg[tb.idx][j]);
						skip.input_frm_cnt = GET_LO_UINT16(img_venc_skip_frame_cfg[tb.idx][j]);
						vendor_videoenc_set(ImgLink[tb.idx].venc_p[j], VENDOR_VIDEOENC_PARAM_OUT_H26X_SKIP_FRM, &skip);
					}
#endif
				}
			}
		}
		if (tb.tbl == MOVIETYPE_MAIN) {
			if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
				(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3)) {
				// set bsmux buffer address
				bsmux_buf.videnc.phy_addr = img_venc_bs_buf[tb.idx][tb.tbl].buf_info.phy_addr;
				bsmux_buf.videnc.buf_size = img_venc_bs_buf[tb.idx][tb.tbl].buf_info.buf_size;
				bsmux_buf.audenc.phy_addr = (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_PCM) ? aud_acap_bs_buf[0].buf_info.phy_addr : aud_aenc_bs_buf[0].buf_info.phy_addr;
				bsmux_buf.audenc.buf_size = (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_PCM) ? aud_acap_bs_buf[0].buf_info.buf_size : aud_aenc_bs_buf[0].buf_info.buf_size;
				if ((ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_ONLY) != _CFG_MAIN_EMR_ONLY && (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) != _CFG_MAIN_EMR_LOOP) {      // main
					if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[0], HD_BSMUX_PARAM_BUFINFO, (void*)&bsmux_buf)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_BUFINFO) fail(%d)\n", hd_ret);
					}
					BsMuxFrmCnt[ImgLink[tb.idx].bsmux_o[0]] = 0;
					if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE)) {
						_LinkPortCntInc(&(ImgLinkStatus[tb.idx].bsmux_p[0]));
						_LinkPortCntInc(&(ImgLinkStatus[tb.idx].fileout_p[0]));
					}
				}
				if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {                                   // main_emr
					if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[1], HD_BSMUX_PARAM_BUFINFO, (void*)&bsmux_buf)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_BUFINFO) fail(%d)\n", hd_ret);
					}
					BsMuxFrmCnt[ImgLink[tb.idx].bsmux_o[1]] = 0;
					if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE)) {
						_LinkPortCntInc(&(ImgLinkStatus[tb.idx].bsmux_p[1]));
						_LinkPortCntInc(&(ImgLinkStatus[tb.idx].fileout_p[1]));
					}
				}
			} else {
				if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_1) ||
					(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2)) {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].fileout_p[0]));
				}
			}
		} else {
			if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
				(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3)) {
				// set bsmux buffer address
				bsmux_buf.videnc.phy_addr = img_venc_bs_buf[tb.idx][tb.tbl].buf_info.phy_addr;
				bsmux_buf.videnc.buf_size = img_venc_bs_buf[tb.idx][tb.tbl].buf_info.buf_size;
				bsmux_buf.audenc.phy_addr = (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_PCM) ? aud_acap_bs_buf[0].buf_info.phy_addr : aud_aenc_bs_buf[0].buf_info.phy_addr;
				bsmux_buf.audenc.buf_size = (ImgLinkRecInfo[tb.idx][tb.tbl].aud_codec == _CFG_AUD_CODEC_PCM) ? aud_acap_bs_buf[0].buf_info.buf_size : aud_aenc_bs_buf[0].buf_info.buf_size;
				if ((ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_ONLY) != _CFG_CLONE_EMR_ONLY && (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_LOOP) != _CFG_CLONE_EMR_LOOP) {    // clone
					if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[2], HD_BSMUX_PARAM_BUFINFO, (void*)&bsmux_buf)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_BUFINFO) fail(%d)\n", hd_ret);
					}
					BsMuxFrmCnt[ImgLink[tb.idx].bsmux_o[2]] = 0;
					if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE)) {
						_LinkPortCntInc(&(ImgLinkStatus[tb.idx].bsmux_p[2]));
						if (img_bsmux_2v1a_mode[tb.idx] == 0) {                                     // normal mode
							_LinkPortCntInc(&(ImgLinkStatus[tb.idx].fileout_p[2]));
						}
					}
				}
				if (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) {                                  // clone_emr
					if ((hd_ret = hd_bsmux_set(ImgLink[tb.idx].bsmux_p[3], HD_BSMUX_PARAM_BUFINFO, (void*)&bsmux_buf)) != HD_OK) {
						DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_BUFINFO) fail(%d)\n", hd_ret);
					}
					BsMuxFrmCnt[ImgLink[tb.idx].bsmux_o[3]] = 0;
					if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE)) {
						_LinkPortCntInc(&(ImgLinkStatus[tb.idx].bsmux_p[3]));
						if (img_bsmux_2v1a_mode[tb.idx] == 0) {                                     // normal mode
							_LinkPortCntInc(&(ImgLinkStatus[tb.idx].fileout_p[3]));
						}
					}
				}
			} else {
				if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_1) ||
					(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2)) {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].fileout_p[2]));
				}
			}
		}
		ret = _ImageApp_MovieMulti_ImgLinkStatusUpdate(i, UPDATE_REVERSE);
		if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3) ||
			(img_vproc_yuv_compress[tb.idx] && (img_venc_sout_type[tb.idx][tb.tbl] != MOVIE_IMGCAP_SOUT_NONE))) {
			vflag = FLGIAMOVIE_VE_M0 << (2 * tb.idx + tb.tbl);
			vos_flag_set(IAMOVIE_FLG_ID, vflag);
		}
		IsImgLinkTranscodeMode[tb.idx][tb.tbl] = TRUE;
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		// get venc buffer addr
		_ethcam_venc_get_buf_info(tb.idx);
		// start aenc and get aenc buffer addr
		if (EthCamLinkRecInfo[tb.idx].rec_mode == _CFG_REC_TYPE_AV) {
			if ((EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_AAC) || (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_ULAW) || (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_ALAW)) {
				_ImageApp_MovieMulti_AudRecStart(0);
				_aenc_get_buf_info(0);
			} else if (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_PCM) {
				ImageApp_MovieMulti_AudCapStart(0);
			}
		}

#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		// vprc
		if (ethcam_vproc_vpe_en[tb.idx]) {
			_LinkPortCntInc(&(EthCamLinkStatus[tb.idx].vproc_p_phy[0]));
			if (ethcam_vproc_no_ext[tb.idx] == FALSE) {
				_LinkPortCntInc(&(EthCamLinkStatus[tb.idx].vproc_p[0]));
			}
		}
#endif

		// force set venc to trigger mode
		if (1) {
			UINT32 frc = ethcam_venc_in_param[tb.idx].frc;
			ethcam_venc_in_param[tb.idx].frc = _ImageApp_MovieMulti_frc(1, 1);
			IACOMM_VENC_PARAM venc_param = {0};
			venc_param	.video_enc_path = EthCamLink[tb.idx].venc_p[0];
			venc_param	.pin            = &(ethcam_venc_in_param[tb.idx]);
			venc_param	.pout           = NULL;
			venc_param	.prc            = NULL;
			if ((hd_ret = venc_set_param(&venc_param)) != HD_OK) {
				DBG_ERR("venc_set_param fail(%d)\n", ret);
			}
			ethcam_venc_in_param[tb.idx].frc = frc;
		}
		_LinkPortCntInc(&(EthCamLinkStatus[tb.idx].venc_p[0]));
#if (defined(_BSP_NA51055_) || defined(_BSP_NA51089_))
		VENDOR_VIDEOENC_H26X_SKIP_FRM_CFG skip = {0};
		skip.b_enable = ethcam_venc_skip_frame_cfg[tb.idx] ? ENABLE : DISABLE;
		skip.target_fr = GET_HI_UINT16(ethcam_venc_skip_frame_cfg[tb.idx]);
		skip.input_frm_cnt = GET_LO_UINT16(ethcam_venc_skip_frame_cfg[tb.idx]);
		vendor_videoenc_set(EthCamLink[tb.idx].venc_p[0], VENDOR_VIDEOENC_PARAM_OUT_H26X_SKIP_FRM, &skip);
#endif

		// Set port
		// set bsmux buffer address
		bsmux_buf.videnc.phy_addr = ethcam_venc_bs_buf[tb.idx].buf_info.phy_addr;
		bsmux_buf.videnc.buf_size = ethcam_venc_bs_buf[tb.idx].buf_info.buf_size;
		bsmux_buf.audenc.phy_addr = (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_PCM) ? aud_acap_bs_buf[0].buf_info.phy_addr : aud_aenc_bs_buf[0].buf_info.phy_addr;
		bsmux_buf.audenc.buf_size = (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_PCM) ? aud_acap_bs_buf[0].buf_info.buf_size : aud_aenc_bs_buf[0].buf_info.buf_size;
		if ((ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_ONLY) != _CFG_MAIN_EMR_ONLY && (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) != _CFG_MAIN_EMR_LOOP) {      // main
			if ((hd_ret = hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[0], HD_BSMUX_PARAM_BUFINFO, (void*)&bsmux_buf)) != HD_OK) {
				DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_BUFINFO) fail(%d)\n", hd_ret);
			}
			BsMuxFrmCnt[EthCamLink[tb.idx].bsmux_o[0]] = 0;
			_LinkPortCntInc(&(EthCamLinkStatus[tb.idx].bsmux_p[0]));
			_LinkPortCntInc(&(EthCamLinkStatus[tb.idx].fileout_p[0]));
		}
		if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {                                   // main_emr
			if ((hd_ret =hd_bsmux_set(EthCamLink[tb.idx].bsmux_p[1], HD_BSMUX_PARAM_BUFINFO, (void*)&bsmux_buf)) != HD_OK) {
				DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_BUFINFO) fail(%d)\n", hd_ret);
			}
			BsMuxFrmCnt[EthCamLink[tb.idx].bsmux_o[1]] = 0;
			_LinkPortCntInc(&(EthCamLinkStatus[tb.idx].bsmux_p[1]));
			_LinkPortCntInc(&(EthCamLinkStatus[tb.idx].fileout_p[1]));
		}

		ethcam_emr_loop_start[tb.idx][0] = 0;

		ret = _ImageApp_MovieMulti_EthCamLinkStatusUpdate(i, UPDATE_REVERSE);
		vflag = FLGIAMOVIE_VE_E0 << tb.idx;
		vos_flag_set(IAMOVIE_FLG_ID, vflag);
	}
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == DISABLE)
	FirstBsMuxPath = _ImageApp_MovieMulti_GetFirstBsMuxPort();
#endif

	vos_sem_sig(IAMOVIE_SEMID_OPERATION);
	return ret;
}

ER ImageApp_MovieMulti_TranscodeStop(MOVIE_CFG_REC_ID id)
{
	ER ret = E_OK;
	HD_RESULT hd_ret;
	UINT32 i = id, j, k, started_path = 0;
	FLGPTN vflag, uiFlag;
	MOVIE_TBL_IDX tb;

	vos_sem_wait(IAMOVIE_SEMID_OPERATION);

	if (ImageApp_MovieMulti_IsStreamRunning(i) == FALSE) {
		vos_sem_sig(IAMOVIE_SEMID_OPERATION);
		return E_SYS;
	}

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		// check 2v1a mode
		if ((img_bsmux_2v1a_mode[tb.idx] != 0) && (tb.tbl == MOVIETYPE_MAIN)) {     // 2v1a mode
			if (ImageApp_MovieMulti_IsStreamRunning(_CFG_CLONE_ID_1 + i) == TRUE) {
				DBG_ERR("Clone path should stop first if 2v1a mode enabled!\r\n");
				vos_sem_sig(IAMOVIE_SEMID_OPERATION);
				return ret;
			}
		}
		if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3)) {
			// stop aenc
			if (ImgLinkRecInfo[tb.idx][tb.tbl].rec_mode == _CFG_REC_TYPE_AV) {
				ImageApp_MovieMulti_AudTranscodeStop(0);
			}
		}
		if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3) ||
			(img_vproc_yuv_compress[tb.idx] && (img_venc_sout_type[tb.idx][tb.tbl] != MOVIE_IMGCAP_SOUT_NONE))) {
			vflag = FLGIAMOVIE_VE_M0 << (2 * tb.idx + tb.tbl);
			vos_flag_clr(IAMOVIE_FLG_ID, vflag);
			vos_flag_wait_timeout(&vflag, IAMOVIE_FLG_ID, FLGIAMOVIE_TSK_VE_IDLE, TWF_ORW, vos_util_msec_to_tick(100));
		}
		if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2) ||
			(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3) ||
			(img_vproc_yuv_compress[tb.idx] && (img_venc_sout_type[tb.idx][tb.tbl] != MOVIE_IMGCAP_SOUT_NONE))) {
			_LinkPortCntDec(&(ImgLinkStatus[tb.idx].venc_p[tb.tbl]));
		}
		if (tb.tbl == MOVIETYPE_MAIN) {
			if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
				(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3)) {
				if ((ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_ONLY) != _CFG_MAIN_EMR_ONLY && (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) != _CFG_MAIN_EMR_LOOP) {      // main
					if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE)) {
						_LinkPortCntDec(&(ImgLinkStatus[tb.idx].bsmux_p[0]));
						_LinkPortCntDec(&(ImgLinkStatus[tb.idx].fileout_p[0]));
					}
				}
				if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {                                   // main_emr
					if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE)) {
						_LinkPortCntDec(&(ImgLinkStatus[tb.idx].bsmux_p[1]));
						_LinkPortCntDec(&(ImgLinkStatus[tb.idx].fileout_p[1]));
					}
				}
			} else {
				if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_1) ||
					(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2)) {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].fileout_p[0]));
				}
			}
		} else {
			if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE) ||
				(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_3)) {
				if ((ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_ONLY) != _CFG_CLONE_EMR_ONLY && (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_LOOP) != _CFG_CLONE_EMR_LOOP) {    // clone
					if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE)) {
						_LinkPortCntDec(&(ImgLinkStatus[tb.idx].bsmux_p[2]));
						if (img_bsmux_2v1a_mode[tb.idx] == 0) {                                     // normal mode
							_LinkPortCntDec(&(ImgLinkStatus[tb.idx].fileout_p[2]));
						}
					}
				}
				if (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) {                                  // clone_emr
					if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_NONE)) {
						_LinkPortCntDec(&(ImgLinkStatus[tb.idx].bsmux_p[3]));
						if (img_bsmux_2v1a_mode[tb.idx] == 0) {                                     // normal mode
							_LinkPortCntDec(&(ImgLinkStatus[tb.idx].fileout_p[3]));
						}
					}
				}
			} else {
				if ((img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_1) ||
					(img_pseudo_rec_mode[tb.idx][tb.tbl] == MOVIE_PSEUDO_REC_MODE_2)) {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].fileout_p[2]));
				}
			}
		}
		vflag = (FLGIAMOVIE2_JPG_M0_RDY | FLGIAMOVIE2_JPG_C0_RDY | FLGIAMOVIE2_THM_M0_RDY | FLGIAMOVIE2_THM_C0_RDY) << (2 * tb.idx + tb.tbl);
		if ((ret = vos_flag_wait_timeout(&uiFlag, IAMOVIE_FLG_ID2, vflag, TWF_ANDW, vos_util_msec_to_tick(1000))) != E_OK) {
			DBG_ERR("Wait rawenc done fail!(%d)\r\n", ret);
			ret = E_OK;
		}
		if (_ImageApp_MovieMulti_ImgLinkStatusUpdate(i, UPDATE_FORWARD) != E_OK) {
			ret = E_SYS;
		}

		// clear variables
		trig_once_limited[tb.idx][tb.tbl] = 0;
		trig_once_cnt[tb.idx][tb.tbl] = 0;
		trig_once_first_i[tb.idx][tb.tbl] = 0;
		trig_once_2v1a_stop_path[tb.idx][tb.tbl] = 0;
		img_venc_timestamp[tb.idx][tb.tbl] = 0;
		img_bsmux_cutnow_with_period_cnt[tb.idx][tb.tbl] = UNUSED;

		// reopen aenc if needed
		if (_ImageApp_MovieMulti_AudReopenAEnc(0) != E_OK) {
			ret = E_SYS;
		}

		// reopen venc if needed
		if (_ImageApp_MovieMulti_ImgReopenVEnc(tb.idx, tb.tbl) != E_OK) {
			ret = E_SYS;
		}

		// check whether need to reopen ImgCapLink
		for (j = 0; (j < MaxLinkInfo.MaxImgLink) && (j < MAX_IMG_PATH); j++) {
			for (k = 0; k < 2; k++) {
				if (ImgLinkStatus[j].venc_p[k]) {
					started_path ++;
				}
			}
		}
		if (iamovie_need_reopen && started_path == 0) {
			_ImageApp_MovieMulti_ImgCapLinkClose(0);
			_ImageApp_MovieMulti_ImgCapLinkOpen(0);
		}
		if (iamovie_use_filedb_and_naming == TRUE) {
			ImageApp_MovieMulti_InitCrash(id);
		}
		if (gpsdata_rec_va[tb.idx][tb.tbl]) {
			if ((hd_ret = hd_common_mem_free(gpsdata_rec_pa[tb.idx][tb.tbl], (void *)gpsdata_rec_va[tb.idx][tb.tbl])) != HD_OK) {
				DBG_ERR("hd_common_mem_free(gpsdata_rec_va[%d][%d]) failed(%d)\r\n", tb.idx, tb.tbl, hd_ret);
			}
			gpsdata_rec_pa[tb.idx][tb.tbl] = 0;
			gpsdata_rec_va[tb.idx][tb.tbl] = 0;
			gpsdata_rec_size[tb.idx][tb.tbl] = 0;
			gpsdata_rec_offset[tb.idx][tb.tbl] = 0;
		}
		IsImgLinkTranscodeMode[tb.idx][tb.tbl] = FALSE;
	} else if ((tb.link_type == LINKTYPE_ETHCAM) && (tb.idx < MAX_ETHCAM_PATH)) {
		// stop aenc
		if (EthCamLinkRecInfo[tb.idx].rec_mode == _CFG_REC_TYPE_AV) {
			if ((EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_AAC) || (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_ULAW) || (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_ALAW)) {
				_ImageApp_MovieMulti_AudRecStop(0);
			} else if (EthCamLinkRecInfo[tb.idx].aud_codec == _CFG_AUD_CODEC_PCM) {
				ImageApp_MovieMulti_AudCapStop(0);
			}
		}

#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		// vprc
		if (ethcam_vproc_vpe_en[tb.idx]) {
			_LinkPortCntDec(&(EthCamLinkStatus[tb.idx].vproc_p_phy[0]));
			if (ethcam_vproc_no_ext[tb.idx] == FALSE) {
				_LinkPortCntDec(&(EthCamLinkStatus[tb.idx].vproc_p[0]));
			}
		}
#endif

		vflag = FLGIAMOVIE_VE_E0 << tb.idx;
		vos_flag_clr(IAMOVIE_FLG_ID, vflag);
		vos_flag_wait_timeout(&vflag, IAMOVIE_FLG_ID, FLGIAMOVIE_TSK_VE_IDLE, TWF_ORW, vos_util_msec_to_tick(100));
		_LinkPortCntDec(&(EthCamLinkStatus[tb.idx].venc_p[0]));

		// Set port
		if ((ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_ONLY) != _CFG_MAIN_EMR_ONLY && (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) != _CFG_MAIN_EMR_LOOP) {      // main
			_LinkPortCntDec(&(EthCamLinkStatus[tb.idx].bsmux_p[0]));
			_LinkPortCntDec(&(EthCamLinkStatus[tb.idx].fileout_p[0]));
		}
		if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {                                   // main_emr
			_LinkPortCntDec(&(EthCamLinkStatus[tb.idx].bsmux_p[1]));
			_LinkPortCntDec(&(EthCamLinkStatus[tb.idx].fileout_p[1]));
		}
		if (_ImageApp_MovieMulti_EthCamLinkStatusUpdate(i, UPDATE_FORWARD) != E_OK) {
			ret = E_SYS;
		}

		// clear variables
		ethcam_venc_timestamp[tb.idx] = 0;
		ethcam_bsmux_cutnow_with_period_cnt[tb.idx][0] = UNUSED;

		// reopen aenc if needed
		if (_ImageApp_MovieMulti_AudReopenAEnc(0) != E_OK) {
			ret = E_SYS;
		}

#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		// repoen vprc if needed
		if (_ImageApp_MovieMulti_EthCamReopenVPrc(tb.idx) != E_OK) {
			ret = E_SYS;
		}
#endif

		// reopen venc if needed
		if (_ImageApp_MovieMulti_EthCamReopenVEnc(tb.idx) != E_OK) {
			ret = E_SYS;
		}

		if (iamovie_use_filedb_and_naming == TRUE) {
			ImageApp_MovieMulti_InitCrash(id);
		}
		if (gpsdata_eth_va[tb.idx][0]) {
			if ((hd_ret = hd_common_mem_free(gpsdata_eth_pa[tb.idx][0], (void *)gpsdata_eth_va[tb.idx][0])) != HD_OK) {
				DBG_ERR("hd_common_mem_free(gpsdata_eth_va[%d][0]) failed(%d)\r\n", tb.idx, hd_ret);
			}
			gpsdata_eth_pa[tb.idx][0] = 0;
			gpsdata_eth_va[tb.idx][0] = 0;
			gpsdata_eth_size[tb.idx][0] = 0;
			gpsdata_eth_offset[tb.idx][0] = 0;
		}
	}
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == DISABLE)
	FirstBsMuxPath = _ImageApp_MovieMulti_GetFirstBsMuxPort();
#endif

	if (ret == E_OK && g_ia_moviemulti_usercb) {
		if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
			if (!((img_bsmux_2v1a_mode[tb.idx] != 0) && (tb.tbl == MOVIETYPE_CLONE))) {
				g_ia_moviemulti_usercb(i, MOVIE_USER_CB_EVENT_REC_STOP, img_pseudo_rec_mode[tb.idx][tb.tbl]);
			}
		} else {
			g_ia_moviemulti_usercb(i, MOVIE_USER_CB_EVENT_REC_STOP, 0);
		}
	}

	vos_sem_sig(IAMOVIE_SEMID_OPERATION);
	return ret;
}

ER ImageApp_MovieMulti_ImgLinkPreStart(MOVIE_CFG_REC_ID id, IAMOVIE_VPRC_EX_PATH path)
{
	UINT32 i = id, iport;
	MOVIE_TBL_IDX tb;
	ER ret = E_OK;

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if (gSwitchLink[tb.idx][path] == UNUSED) {
			DBG_ERR("gSwitchLink[%d][%d]=%d\r\n", tb.idx, path, gSwitchLink[tb.idx][path]);
			return E_SYS;
		}
		iport = gUserProcMap[tb.idx][gSwitchLink[tb.idx][path]];
		if (iport > 2) {
			iport = 2;
		}
		// Set port
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		if (img_vproc_vpe_en[tb.idx] == MOVIE_VPE_NONE) {
#endif
			if (img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[tb.idx] == FALSE) {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][img_vproc_3dnr_path[tb.idx]]));
				} else {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][4]));
				}
			}
			_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vcap_p[0]));
			if (!((img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[tb.idx] == FALSE) && (iport == img_vproc_3dnr_path[tb.idx]))) {
				_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][iport]));
			}
			if (img_vproc_no_ext[tb.idx] == FALSE) {
				_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][path]));
			}
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		} else {
			if (img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[tb.idx] == FALSE) {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][img_vproc_3dnr_path[tb.idx]]));
				} else {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][4]));
				}
			}
			_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vcap_p[0]));
			if (!((img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[tb.idx] == FALSE))) {
				_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][0]));
			}
			_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][iport]));
			if (img_vproc_no_ext[tb.idx] == FALSE) {
				_LinkPortCntInc(&(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][path]));
			}
		}
#endif
		if (_ImageApp_MovieMulti_ImgLinkStatusUpdate(i, UPDATE_REVERSE) != E_OK) {
			ret = E_SYS;
		}
	}
	return ret;
}

ER ImageApp_MovieMulti_ImgLinkPostStop(MOVIE_CFG_REC_ID id, IAMOVIE_VPRC_EX_PATH path)
{
	UINT32 i = id, iport;
	MOVIE_TBL_IDX tb;
	ER ret = E_OK;


	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if (gSwitchLink[tb.idx][path] == UNUSED) {
			DBG_ERR("gSwitchLink[%d][%d]=%d\r\n", tb.idx, path, gSwitchLink[tb.idx][path]);
			return E_SYS;
		}
		iport = gUserProcMap[tb.idx][gSwitchLink[tb.idx][path]];
		if (iport > 2) {
			iport = 2;
		}
		// Set port
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		if (img_vproc_vpe_en[tb.idx] == MOVIE_VPE_NONE) {
#endif
			if (img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[tb.idx] == FALSE) {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][img_vproc_3dnr_path[tb.idx]]));
				} else {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][4]));
				}
			}
			_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vcap_p[0]));
			if (!((img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[tb.idx] == FALSE) && (iport == img_vproc_3dnr_path[tb.idx]))) {
				_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][iport]));
			}
			if (img_vproc_no_ext[tb.idx] == FALSE) {
				_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][path]));
			}
#if (IAMOVIE_SUPPORT_VPE == ENABLE)
		} else {
			if (img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) {
				if (use_unique_3dnr_path[tb.idx] == FALSE) {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][img_vproc_3dnr_path[tb.idx]]));
				} else {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][4]));
				}
			}
			_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vcap_p[0]));
			if (!((img_vproc_ctrl[tb.idx].func & HD_VIDEOPROC_FUNC_3DNR) && (use_unique_3dnr_path[tb.idx] == FALSE))) {
				_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[0][0]));
			}
			_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p_phy[img_vproc_splitter[tb.idx]][iport]));
			if (img_vproc_no_ext[tb.idx] == FALSE) {
				_LinkPortCntDec(&(ImgLinkStatus[tb.idx].vproc_p[img_vproc_splitter[tb.idx]][path]));
			}
		}
#endif
		if (_ImageApp_MovieMulti_ImgLinkStatusUpdate(i, UPDATE_FORWARD) != E_OK) {
			ret = E_SYS;
		}
	}
	return ret;
}

ER ImageApp_MovieMulti_Mode3_RecStart(MOVIE_CFG_REC_ID id)
{
	ER ret = E_OK;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	vos_sem_wait(IAMOVIE_SEMID_OPERATION);

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if ((img_pseudo_rec_mode[tb.idx][tb.tbl] != MOVIE_PSEUDO_REC_MODE_3)) {
			DBG_ERR("Can only be called in pseudo record mode 3\r\n");
			vos_sem_sig(IAMOVIE_SEMID_OPERATION);
			return E_SYS;
		}
		if (ImageApp_MovieMulti_IsStreamRunning(i) == FALSE) {
			DBG_ERR("Can only be called if pseudo record mode 3 is running\r\n");
			vos_sem_sig(IAMOVIE_SEMID_OPERATION);
			return E_SYS;
		}
		if (tb.tbl == MOVIETYPE_MAIN) {
			if ((ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_ONLY) != _CFG_MAIN_EMR_ONLY && (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) != _CFG_MAIN_EMR_LOOP) {       // main
				if (IsImgLinkMode3RecStart[tb.idx][0] == FALSE) {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].bsmux_p[0]));
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].fileout_p[0]));
					IsImgLinkMode3RecStart[tb.idx][0] = TRUE;
				} else {
					DBG_ERR("Mode3_RecStart is already called\r\n");
					vos_sem_sig(IAMOVIE_SEMID_OPERATION);
					return E_SYS;
				}
			}
			if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {                                   // main_emr
				if (IsImgLinkMode3RecStart[tb.idx][1] == FALSE) {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].bsmux_p[1]));
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].fileout_p[1]));
					IsImgLinkMode3RecStart[tb.idx][1] = TRUE;
				} else {
					DBG_ERR("Mode3_RecStart is already called\r\n");
					vos_sem_sig(IAMOVIE_SEMID_OPERATION);
					return E_SYS;
				}
			}
		} else {
			if ((ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_ONLY) != _CFG_CLONE_EMR_ONLY && (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_LOOP) != _CFG_CLONE_EMR_LOOP) {   // clone
				if (IsImgLinkMode3RecStart[tb.idx][2] == FALSE) {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].bsmux_p[2]));
					if (img_bsmux_2v1a_mode[tb.idx] == 0) {                                     // normal mode
						_LinkPortCntInc(&(ImgLinkStatus[tb.idx].fileout_p[2]));
					}
					IsImgLinkMode3RecStart[tb.idx][2] = TRUE;
				} else {
					DBG_ERR("Mode3_RecStart is already called\r\n");
					vos_sem_sig(IAMOVIE_SEMID_OPERATION);
					return E_SYS;
				}
			}
			if (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) {                                  // clone_emr
				if (IsImgLinkMode3RecStart[tb.idx][3] == FALSE) {
					_LinkPortCntInc(&(ImgLinkStatus[tb.idx].bsmux_p[3]));
					if (img_bsmux_2v1a_mode[tb.idx] == 0) {                                     // normal mode
						_LinkPortCntInc(&(ImgLinkStatus[tb.idx].fileout_p[3]));
					}
					IsImgLinkMode3RecStart[tb.idx][3] = TRUE;
				} else {
					DBG_ERR("Mode3_RecStart is already called\r\n");
					vos_sem_sig(IAMOVIE_SEMID_OPERATION);
					return E_SYS;
				}
			}
		}
		ret = _ImageApp_MovieMulti_ImgLinkStatusUpdate(i, UPDATE_REVERSE);
	} else {
		ret = E_NOSPT;
	}
	vos_sem_sig(IAMOVIE_SEMID_OPERATION);
	return ret;
}

ER ImageApp_MovieMulti_Mode3_RecStop(MOVIE_CFG_REC_ID id)
{
	ER ret = E_OK;
	UINT32 i = id;
	MOVIE_TBL_IDX tb;

	vos_sem_wait(IAMOVIE_SEMID_OPERATION);

	_ImageApp_MovieMulti_RecidGetTableIndex(i, &tb);

	if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
		if ((img_pseudo_rec_mode[tb.idx][tb.tbl] != MOVIE_PSEUDO_REC_MODE_3)) {
			DBG_ERR("Can only be called in pseudo record mode 3\r\n");
			vos_sem_sig(IAMOVIE_SEMID_OPERATION);
			return E_SYS;
		}
		if (ImageApp_MovieMulti_IsStreamRunning(i) == FALSE) {
			DBG_ERR("Can only be called if pseudo record mode 3 is running\r\n");
			vos_sem_sig(IAMOVIE_SEMID_OPERATION);
			return E_SYS;
		}
		if (tb.tbl == MOVIETYPE_MAIN) {
			if ((ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_ONLY) != _CFG_MAIN_EMR_ONLY && (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) != _CFG_MAIN_EMR_LOOP) {       // main
				if (IsImgLinkMode3RecStart[tb.idx][0] == TRUE) {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].bsmux_p[0]));
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].fileout_p[0]));
					IsImgLinkMode3RecStart[tb.idx][0] = FALSE;
				} else {
					DBG_ERR("Mode3_RecStop is already called\r\n");
					vos_sem_sig(IAMOVIE_SEMID_OPERATION);
					return E_SYS;
				}
			}
			if (ImgLinkFileInfo.emr_on & MASK_EMR_MAIN) {                                   // main_emr
				if (IsImgLinkMode3RecStart[tb.idx][1] == TRUE) {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].bsmux_p[1]));
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].fileout_p[1]));
					IsImgLinkMode3RecStart[tb.idx][1] = FALSE;
				} else {
					DBG_ERR("Mode3_RecStop is already called\r\n");
					vos_sem_sig(IAMOVIE_SEMID_OPERATION);
					return E_SYS;
				}
			}
		} else {
			if ((ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_ONLY) != _CFG_CLONE_EMR_ONLY && (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_LOOP) != _CFG_CLONE_EMR_LOOP) {   // clone
				if (IsImgLinkMode3RecStart[tb.idx][2] == TRUE) {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].bsmux_p[2]));
					if (img_bsmux_2v1a_mode[tb.idx] == 0) {                                     // normal mode
						_LinkPortCntDec(&(ImgLinkStatus[tb.idx].fileout_p[2]));
					}
					IsImgLinkMode3RecStart[tb.idx][2] = FALSE;
				} else {
					DBG_ERR("Mode3_RecStop is already called\r\n");
					vos_sem_sig(IAMOVIE_SEMID_OPERATION);
					return E_SYS;
				}
			}
			if (ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) {                                  // clone_emr
				if (IsImgLinkMode3RecStart[tb.idx][3] == TRUE) {
					_LinkPortCntDec(&(ImgLinkStatus[tb.idx].bsmux_p[3]));
					if (img_bsmux_2v1a_mode[tb.idx] == 0) {                                     // normal mode
						_LinkPortCntDec(&(ImgLinkStatus[tb.idx].fileout_p[3]));
						IsImgLinkMode3RecStart[tb.idx][3] = FALSE;
					}
				} else {
					DBG_ERR("Mode3_RecStop is already called\r\n");
					vos_sem_sig(IAMOVIE_SEMID_OPERATION);
					return E_SYS;
				}
			}
		}
		ret = _ImageApp_MovieMulti_ImgLinkStatusUpdate(i, UPDATE_FORWARD);
	} else {
		ret = E_NOSPT;
	}

	if (ret == E_OK && g_ia_moviemulti_usercb) {
		if ((tb.link_type == LINKTYPE_REC) && (tb.idx < MAX_IMG_PATH) && (tb.tbl < MOVIETYPE_MAX)) {
			if (!((img_bsmux_2v1a_mode[tb.idx] != 0) && (tb.tbl == MOVIETYPE_CLONE))) {
				g_ia_moviemulti_usercb(i, MOVIE_USER_CB_EVENT_REC_STOP, img_pseudo_rec_mode[tb.idx][tb.tbl]);
			}
		} else {
			g_ia_moviemulti_usercb(i, MOVIE_USER_CB_EVENT_REC_STOP, 0);
		}
	}

	vos_sem_sig(IAMOVIE_SEMID_OPERATION);
	return ret;
}

