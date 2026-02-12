#include "ImageApp_UsbMovie_int.h"

/// ========== Cross file variables ==========
USBMOVIE_AUDIO_INFO g_UsbAudioInfo = {0};
UINT32 IsUsbAudCapLinkOpened[MAX_USBAUD_PATH] = {0};
USBMOVIE_AUDCAP_LINK UsbAudCapLink[MAX_USBAUD_PATH] = {0};
USBMOVIE_AUDCAP_LINK_STATUS UsbAudCapLinkStatus[MAX_USBAUD_PATH] = {0};
HD_AUDIOCAP_DEV_CONFIG usbaud_acap_dev_cfg[MAX_USBAUD_PATH] = {0};
HD_AUDIOCAP_DRV_CONFIG usbaud_acap_drv_cfg[MAX_USBAUD_PATH] = {0};
HD_AUDIOCAP_IN usbaud_acap_param[MAX_USBAUD_PATH] = {0};
HD_AUDIOCAP_BUFINFO usbaud_acap_bs_buf[MAX_USBAUD_PATH] = {0};
UINT32 usbaud_acap_bs_buf_va[MAX_USBAUD_PATH] = {0};
UINT32 usbaud_cap_volume = 100;
/// ========== Noncross file variables ==========

ER _ImageApp_UsbMovie_AudCapLinkCfg(UINT32 id)
{
	UINT32 idx = id;

	if (idx >= MAX_USBAUD_PATH) {
		DBG_ERR("id%d is out of range.\r\n", idx);
		return E_SYS;
	}

	// video capture
	UsbAudCapLink[idx].acap_ctrl   = HD_AUDIOCAP_CTRL(idx);
	UsbAudCapLink[idx].acap_i[0]   = HD_AUDIOCAP_IN(idx, 0);
	UsbAudCapLink[idx].acap_o[0]   = HD_AUDIOCAP_OUT(idx, 0);

	return E_OK;
}

static ER _ImageApp_UsbMovie_AudCapLink(UINT32 id)
{
	UINT32 idx = id;
	ER ret = E_OK;
	HD_RESULT hd_ret;
	HD_AUDIOCAP_VOLUME audio_cap_volume = {0};

	if (idx >= MAX_USBAUD_PATH) {
		DBG_ERR("id%d is out of range.\r\n", idx);
		return E_SYS;
	}

	// set audiocap
	IACOMM_ACAP_CFG acap_cfg = {
		.p_audio_cap_ctrl = &(UsbAudCapLink[idx].acap_p_ctrl),
		.ctrl_id          = UsbAudCapLink[idx].acap_ctrl,
		.pdev_cfg         = &(usbaud_acap_dev_cfg[idx]),
		.pdrv_cfg         = &(usbaud_acap_drv_cfg[idx]),
	};
	if ((hd_ret = acap_set_cfg(&acap_cfg)) != HD_OK) {
		DBG_ERR("acap_set_cfg fail(%d)\r\n", hd_ret);
		ret = E_SYS;
	}
	if ((hd_ret = acap_open_ch(UsbAudCapLink[idx].acap_i[0], UsbAudCapLink[idx].acap_o[0], &(UsbAudCapLink[idx].acap_p[0]))) != HD_OK) {
		DBG_ERR("acap_open_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	IACOMM_ACAP_PARAM acap_param = {
		.audio_cap_path = UsbAudCapLink[idx].acap_p[0],
		.pcap_param     = &(usbaud_acap_param[idx]),
	};
	if ((hd_ret = acap_set_param(&acap_param)) != HD_OK) {
		DBG_ERR("acap_set_param fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	if ((hd_ret = hd_audiocap_start(UsbAudCapLink[idx].acap_p[0])) != HD_OK) {
		DBG_ERR("hd_audiocap_start fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	if ((hd_ret = hd_audiocap_get(UsbAudCapLink[idx].acap_ctrl, HD_AUDIOCAP_PARAM_BUFINFO, &(usbaud_acap_bs_buf[idx]))) != HD_OK) {
		DBG_ERR("hd_audiocap_get fail(%d)\n", hd_ret);
		ret = E_SYS;
	} else {
		usbaud_acap_bs_buf_va[idx] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, usbaud_acap_bs_buf[idx].buf_info.phy_addr, usbaud_acap_bs_buf[idx].buf_info.buf_size);
	}
	if ((hd_ret = hd_audiocap_stop(UsbAudCapLink[idx].acap_p[0])) != HD_OK) {
		DBG_ERR("hd_audiocap_stop fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	audio_cap_volume.volume = usbaud_cap_volume;
	if ((hd_ret = hd_audiocap_set(UsbAudCapLink[0].acap_p_ctrl, HD_AUDIOCAP_PARAM_VOLUME, &audio_cap_volume)) != HD_OK) {
		DBG_ERR("Set HD_AUDIOCAP_PARAM_VOLUME fail(%d)\r\n", hd_ret);
	}
	return ret;
}

static ER _ImageApp_UsbMovie_AudCapUnlink(UINT32 id)
{
	UINT32 idx = id;
	ER ret = E_OK;
	HD_RESULT hd_ret;

	if (idx >= MAX_USBAUD_PATH) {
		DBG_ERR("id%d is out of range.\r\n", idx);
		return E_SYS;
	}

	// set audiocap
	if ((hd_ret = acap_close_ch(UsbAudCapLink[idx].acap_p[0])) != HD_OK) {
		DBG_ERR("acap_close_ch fail(%d)\n", hd_ret);
		ret = E_SYS;
	}
	return ret;
}

static void acap_status_cb(HD_PATH_ID path_id, UINT32 state)
{
	FLGPTN vflag;

	if (state == CB_STATE_AFTER_RUN) {
		vos_flag_set(IAUSBMOVIE_FLG_ID, FLGIAUSBMOVIE_AC_PCM);
	} else if (state == CB_STATE_BEFORE_STOP) {
		vos_flag_clr(IAUSBMOVIE_FLG_ID, FLGIAUSBMOVIE_AC_PCM);
		vos_flag_wait_timeout(&vflag, IAUSBMOVIE_FLG_ID, FLGIAUSBMOVIE_TSK_AC_IDLE, TWF_ORW, vos_util_msec_to_tick(100));
	}
}

ER _ImageApp_UsbMovie_AudCapLinkStatusUpdate(UINT32 id, UINT32 direction)
{
	UINT32 idx = id;
	ER ret = E_OK;

	if (idx >= MAX_USBAUD_PATH) {
		DBG_ERR("id%d is out of range.\r\n", idx);
		return E_SYS;
	}

	// set audiocap
	ret |= _LinkUpdateStatus(acap_set_state, UsbAudCapLink[idx].acap_p[0], &(UsbAudCapLinkStatus[idx].acap_p[0]), acap_status_cb);

	return ret;
}

ER _ImageApp_UsbMovie_AudCapLinkOpen(UINT32 id)
{
	UINT32 i = id, idx = id;

	if (idx >= MAX_USBAUD_PATH) {
		DBG_ERR("id%d is out of range.\r\n", idx);
		return E_SYS;
	}

	if (IsUsbAudCapLinkOpened[idx] == TRUE) {
		DBG_ERR("UsbAudCap[%d] is already opened.\r\n", idx);
		return E_SYS;
	}
	_ImageApp_UsbMovie_AudCapLinkCfg(i);
	_ImageApp_UsbMovie_SetupAudioParam();
	_ImageApp_UsbMovie_AudCapLink(i);
	IsUsbAudCapLinkOpened[idx] = TRUE;

	return E_OK;
}

ER _ImageApp_UsbMovie_AudCapLinkClose(UINT32 id)
{
	UINT32 i = id, idx = id;

	if (idx >= MAX_USBAUD_PATH) {
		DBG_ERR("id%d is out of range.\r\n", idx);
		return E_SYS;
	}

	if (IsUsbAudCapLinkOpened[idx] == FALSE) {
		DBG_ERR("UsbAudCap[%d] is already closed.\r\n", idx);
		return E_SYS;
	}

	_ImageApp_UsbMovie_AudCapUnlink(i);

	// reset variables
	if (usbaud_acap_bs_buf_va[idx]) {
		hd_common_mem_munmap((void *)usbaud_acap_bs_buf_va[idx], usbaud_acap_bs_buf[idx].buf_info.buf_size);
		usbaud_acap_bs_buf_va[idx] = 0;
		memset((void *)&(usbaud_acap_bs_buf[idx]), 0, sizeof(HD_AUDIOENC_BUFINFO));
	}

	IsUsbAudCapLinkOpened[idx] = FALSE;

	return E_OK;
}

ER _ImageApp_UsbMovie_AudCapStart(UINT32 id)
{
	ER ret;
	UINT32 i = id, idx = id;

	if (idx >= MAX_USBAUD_PATH) {
		DBG_ERR("id%d is out of range.\r\n", idx);
		return E_SYS;
	}

	if (IsUsbAudCapLinkOpened[idx] == FALSE) {
		DBG_ERR("UsbAudCap[%d] is not opened.\r\n", idx);
		return E_SYS;
	}
	_LinkPortCntInc(&(UsbAudCapLinkStatus[idx].acap_p[0]));
	ret = _ImageApp_UsbMovie_AudCapLinkStatusUpdate(i, UPDATE_REVERSE);

	return ret;
}

ER _ImageApp_UsbMovie_AudCapStop(UINT32 id)
{
	ER ret;
	UINT32 i = id, idx = id;

	if (idx >= MAX_USBAUD_PATH) {
		DBG_ERR("id%d is out of range.\r\n", idx);
		return E_SYS;
	}

	if (IsUsbAudCapLinkOpened[idx] == FALSE) {
		DBG_ERR("UsbAudCap[%d] is not opened.\r\n", idx);
		return E_SYS;
	}
	_LinkPortCntDec(&(UsbAudCapLinkStatus[idx].acap_p[0]));
	ret = _ImageApp_UsbMovie_AudCapLinkStatusUpdate(i, UPDATE_FORWARD);

	return ret;
}

