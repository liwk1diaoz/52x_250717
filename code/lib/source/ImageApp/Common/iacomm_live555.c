#include "ImageApp_Common_int.h"

/// ========== Cross file variables ==========
QUEUE_BUFFER_QUEUE VBufferQueueClear[MAX_RTSP_PATH] = {0};
QUEUE_BUFFER_QUEUE VBufferQueueReady[MAX_RTSP_PATH] = {0};
QUEUE_BUFFER_QUEUE ABufferQueueClear[MAX_RTSP_PATH] = {0};
QUEUE_BUFFER_QUEUE ABufferQueueReady[MAX_RTSP_PATH] = {0};
/// ========== Noncross file variables ==========
static STRM_VBUFFER_ELEMENT VBufferElement[MAX_RTSP_PATH][STRM_VQUEUENUM];
static STRM_ABUFFER_ELEMENT ABufferElement[MAX_RTSP_PATH][STRM_AQUEUENUM];
static VIDEO_INFO live555_video_info[MAX_RTSP_PATH] = {0};
static AUDIO_INFO live555_audio_info[MAX_RTSP_PATH] = {0};

#define PHY2VIRT_VSTRM(id, pa) (vir_addr_vstrm[id] + (pa - phy_buf_vstrm[id].buf_info.phy_addr))
#define PHY2VIRT_ASTRM(id, pa) (vir_addr_astrm[id] + (pa - phy_buf_astrm[id].buf_info.phy_addr))

static UINT32 vir_addr_vstrm[MAX_RTSP_PATH] = {0};
static HD_VIDEOENC_BUFINFO phy_buf_vstrm[MAX_RTSP_PATH] = {0};
static UINT32 vir_addr_astrm[MAX_RTSP_PATH] = {0};
static HD_AUDIOENC_BUFINFO phy_buf_astrm[MAX_RTSP_PATH] = {0};
static UINT32 wait_i_frame = FALSE;

static void _iacomm_cb(UINT32 id, IACOMM_USER_CB_EVENT event, IACOMM_RTSP_CB_INFO *pinfo, char *str)
{
	VOS_TICK t1, t2, t_diff;

	if (g_ia_common_usercb) {
		vos_perf_mark(&t1);
		g_ia_common_usercb(id, event, (UINT32)pinfo);
		vos_perf_mark(&t2);
		t_diff = (t2 - t1) / 1000;
		if (t_diff > IACOMMON_BS_ERR_TIMEOUT_CHECK) {
			DBG_WRN("%s callback run %dms > %dms!\r\n", str, t_diff, IACOMMON_BS_ERR_TIMEOUT_CHECK);
		}
	}
}

static int flush_video(UINT32 id)
{
	UINT32 i = id;
	HD_H26XENC_REQUEST_IFRAME req_i = {0};
	HD_RESULT hd_ret;

	if (i >= MAX_RTSP_PATH) {
		return 0;
	}
	while ((live555_video_info[i].pVBuf = (STRM_VBUFFER_ELEMENT *)_QUEUE_DeQueueFromHead(&(VBufferQueueReady[i]))) != NULL) {
		_QUEUE_EnQueueToTail(&VBufferQueueClear[i], (QUEUE_BUFFER_ELEMENT *)live555_video_info[i].pVBuf);
	}
	iacomm_flag_clr(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_BS_RDY << i));

	wait_i_frame = TRUE;
	req_i.enable = 1;
	if ((hd_ret = hd_videoenc_set(live555_video_info[i].venc_p, HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME, &req_i)) != HD_OK) {
		DBG_ERR("HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME fail(%d)\r\n", hd_ret);
	}
	if ((hd_ret = hd_videoenc_start(live555_video_info[i].venc_p)) != HD_OK) {
		DBG_ERR("hd_videoenc_start fail(%d)\r\n", hd_ret);
	}
	return hd_ret;
}

static int flush_audio(UINT32 id)
{
	UINT32 i = id;

	while ((live555_audio_info[i].pABuf = (STRM_ABUFFER_ELEMENT *)_QUEUE_DeQueueFromHead(&ABufferQueueReady[i])) != NULL) {
		_QUEUE_EnQueueToTail(&ABufferQueueClear[i], (QUEUE_BUFFER_ELEMENT *)live555_audio_info[i].pABuf);
	}
	iacomm_flag_clr(IACOMMON_FLG_ID, (FLGIACOMMON_AE_S0_BS_RDY << i));

	return 0;
}

ER live555_init(UINT32 id)
{
	UINT32 i = id;

	live555_video_info[i].id = i;
	live555_video_info[i].codec_type = NVTLIVE555_CODEC_UNKNOWN;
	live555_audio_info[i].id = i;
	live555_audio_info[i].info.codec_type = NVTLIVE555_CODEC_UNKNOWN;

	return E_OK;
}

ER live555_reset_Aqueue(UINT32 id)
{
	UINT32 i = id, j;
	FLGPTN uiFlag;

	if (i >= MAX_RTSP_PATH) {
		return E_SYS;
	}

	iacomm_flag_wait(&uiFlag, IACOMMON_FLG_ID, ((FLGIACOMMON_AE_S0_CQUE_FREE | FLGIACOMMON_AE_S0_RQUE_FREE) << i), TWF_ANDW);
	iacomm_flag_clr(IACOMMON_FLG_ID, (FLGIACOMMON_AE_S0_QUE_RDY << i));

	memset(&ABufferQueueClear[i], 0, sizeof(ABufferQueueClear[i]));
	memset(&ABufferQueueReady[i], 0, sizeof(ABufferQueueReady[i]));

	for (j = 0; j < STRM_AQUEUENUM; j++) {
		_QUEUE_EnQueueToTail(&ABufferQueueClear[i], (QUEUE_BUFFER_ELEMENT *)&(ABufferElement[i][j]));
	}
	iacomm_flag_set(IACOMMON_FLG_ID, (FLGIACOMMON_AE_S0_QUE_RDY << i));

	return E_OK;
}

ER live555_reset_Vqueue(UINT32 id)
{
	UINT32 i = id, j;
	FLGPTN uiFlag;

	if (i >= MAX_RTSP_PATH) {
		return E_SYS;
	}

	iacomm_flag_wait(&uiFlag, IACOMMON_FLG_ID, ((FLGIACOMMON_VE_S0_CQUE_FREE | FLGIACOMMON_VE_S0_RQUE_FREE) << i), TWF_ANDW);
	iacomm_flag_clr(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_QUE_RDY << i));

	memset(&VBufferQueueClear[i], 0, sizeof(VBufferQueueClear[i]));
	memset(&VBufferQueueReady[i], 0, sizeof(VBufferQueueReady[i]));

	for (j = 0; j < STRM_VQUEUENUM; j++) {
		_QUEUE_EnQueueToTail(&VBufferQueueClear[i], (QUEUE_BUFFER_ELEMENT *)&(VBufferElement[i][j]));
	}
	iacomm_flag_set(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_QUE_RDY << i));

   return E_OK;
}

UINT32 live555_mmap_venc_bs(UINT32 id, HD_PATH_ID path_id)
{
	UINT32 i = id, pid = path_id;
	HD_RESULT hd_ret;

	if (i >= MAX_RTSP_PATH) {
		return 0;
	}

	live555_video_info[i].venc_p = pid;

	if ((hd_ret = hd_videoenc_get(pid, HD_VIDEOENC_PARAM_BUFINFO, &(phy_buf_vstrm[i]))) != HD_OK) {
		vir_addr_vstrm[i] = 0;
		DBG_ERR("Get HD_VIDEOENC_PARAM_BUFINFO fail(%d)\r\n", hd_ret);
	} else {
		vir_addr_vstrm[i] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_vstrm[i].buf_info.phy_addr, phy_buf_vstrm[i].buf_info.buf_size);
	}
	ImageApp_Common_DUMP("V_BSBuf=%x,%x\r\n", phy_buf_vstrm[i].buf_info.phy_addr, phy_buf_vstrm[i].buf_info.buf_size);

	return vir_addr_vstrm[i];
}

UINT32 live555_munmap_venc_bs(UINT32 id)
{
	UINT32 i = id;
	HD_RESULT hd_ret ;
	if (i >= MAX_RTSP_PATH) {
		return 0;
	}

	if ((hd_ret = hd_common_mem_munmap((void *)vir_addr_vstrm[i], phy_buf_vstrm[i].buf_info.buf_size)) != HD_OK) {
		DBG_ERR("hd_common_mem_munmap fail(%d), addr=%0x%x, 0x%x\r\n", hd_ret, vir_addr_vstrm[i],phy_buf_vstrm[i].buf_info.phy_addr);
	}
	vir_addr_vstrm[i] = 0;
	memset(&(phy_buf_vstrm[i]), 0, sizeof(phy_buf_vstrm[i]));

	return 0;
}

int live555_refresh_video_info(UINT32 id)
{
	UINT32 i = id;
	FLGPTN uiFlag;

	if (i >= MAX_RTSP_PATH) {
		return 0;
	}

	live555_video_info[i].codec_type = NVTLIVE555_CODEC_UNKNOWN;
	live555_video_info[i].vps_size = live555_video_info[i].sps_size = live555_video_info[i].pps_size = 0;
	memset(live555_video_info[i].vps, 0, sizeof(live555_video_info[i].vps));
	memset(live555_video_info[i].sps, 0, sizeof(live555_video_info[i].sps));
	memset(live555_video_info[i].pps, 0, sizeof(live555_video_info[i].pps));

	if (vir_addr_vstrm[i] == 0) {
		return 0;
	}

	while (1) {
		iacomm_flag_wait(&uiFlag, IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_BS_RDY << i), TWF_ORW | TWF_CLR);
		while ((live555_video_info[i].pVBuf = (STRM_VBUFFER_ELEMENT *)_QUEUE_DeQueueFromHead(&VBufferQueueReady[i])) != NULL) {
			if (live555_video_info[i].pVBuf->vcodec_format == HD_CODEC_TYPE_JPEG) {
				live555_video_info[i].codec_type = NVTLIVE555_CODEC_MJPG;
				_QUEUE_EnQueueToTail(&VBufferQueueClear[i], (QUEUE_BUFFER_ELEMENT *)live555_video_info[i].pVBuf);
				return 0;
			} else if (live555_video_info[i].pVBuf->vcodec_format == HD_CODEC_TYPE_H264) {
				live555_video_info[i].codec_type = NVTLIVE555_CODEC_H264;
				if (live555_video_info[i].pVBuf->pack_num != 3) {
					_QUEUE_EnQueueToTail(&VBufferQueueClear[i], (QUEUE_BUFFER_ELEMENT *)live555_video_info[i].pVBuf);
					continue;
				}
				memcpy(live555_video_info[i].sps, (void *)PHY2VIRT_VSTRM(id, live555_video_info[i].pVBuf->phy_addr[0]), live555_video_info[i].pVBuf->size[0]);
				memcpy(live555_video_info[i].pps, (void *)PHY2VIRT_VSTRM(id, live555_video_info[i].pVBuf->phy_addr[1]), live555_video_info[i].pVBuf->size[1]);
				live555_video_info[i].sps_size = live555_video_info[i].pVBuf->size[0];
				live555_video_info[i].pps_size = live555_video_info[i].pVBuf->size[1];
				_QUEUE_EnQueueToTail(&VBufferQueueClear[i], (QUEUE_BUFFER_ELEMENT *)live555_video_info[i].pVBuf);
				return 0;
			} else if (live555_video_info[i].pVBuf->vcodec_format == HD_CODEC_TYPE_H265) {
				live555_video_info[i].codec_type = NVTLIVE555_CODEC_H265;
				if (live555_video_info[i].pVBuf->pack_num != 4) {
					_QUEUE_EnQueueToTail(&VBufferQueueClear[i], (QUEUE_BUFFER_ELEMENT *)live555_video_info[i].pVBuf);
					continue;
				}
				memcpy(live555_video_info[i].vps, (void *)PHY2VIRT_VSTRM(id, live555_video_info[i].pVBuf->phy_addr[0]), live555_video_info[i].pVBuf->size[0]);
				memcpy(live555_video_info[i].sps, (void *)PHY2VIRT_VSTRM(id, live555_video_info[i].pVBuf->phy_addr[1]), live555_video_info[i].pVBuf->size[1]);
				memcpy(live555_video_info[i].pps, (void *)PHY2VIRT_VSTRM(id, live555_video_info[i].pVBuf->phy_addr[2]), live555_video_info[i].pVBuf->size[2]);
				live555_video_info[i].vps_size = live555_video_info[i].pVBuf->size[0];
				live555_video_info[i].sps_size = live555_video_info[i].pVBuf->size[1];
				live555_video_info[i].pps_size = live555_video_info[i].pVBuf->size[2];
				_QUEUE_EnQueueToTail(&VBufferQueueClear[i], (QUEUE_BUFFER_ELEMENT *)live555_video_info[i].pVBuf);
				return 0;
			} else {
				_QUEUE_EnQueueToTail(&VBufferQueueClear[i], (QUEUE_BUFFER_ELEMENT *)live555_video_info[i].pVBuf);
			}
		}
	}
	return 0;
}

int live555_set_audio_info(UINT32 id, NVTLIVE555_AUDIO_INFO *info)
{
	UINT32 i = id;

	if (i >= MAX_RTSP_PATH) {
		return 0;
	}
	live555_audio_info[i].info.codec_type = info->codec_type;
	live555_audio_info[i].info.sample_per_second = info->sample_per_second;
	live555_audio_info[i].info.bit_per_sample = info->bit_per_sample;
	live555_audio_info[i].info.channel_cnt = info->channel_cnt;

	return 0;
}

int live555_on_open_video(int channel)
{
	IACOMM_USER_CB_EVENT cb_event = IACOMM_USER_CB_EVENT_RTSP_OPEN;
	char cb_name[] = "IACOMM_USER_CB_EVENT_RTSP_OPEN";
	IACOMM_RTSP_CB_INFO cb_info = {0};

	cb_info.type = IACOMM_RTSP_BS_VIDEO;

	if ((size_t)channel >= sizeof(live555_video_info) / sizeof(live555_video_info[0])) {
		DBG_ERR("nvtrtspd video channel exceed\n");
		cb_info.err_code = IACOMM_RTSP_CHANNEL_EXCEED;
		_iacomm_cb(channel, cb_event, &cb_info, cb_name);
		return 0;
	}
	if (live555_video_info[channel].ref_cnt != 0) {
		DBG_ERR("nvtrtspd video in use., channel=%d\n",channel);
		cb_info.err_code = IACOMM_RTSP_CHANNEL_IN_USE;
		_iacomm_cb(channel, cb_event, &cb_info, cb_name);
		//return 0;
		return (int)&live555_video_info[channel];
	}

	//no video
	if (live555_video_info[channel].codec_type == NVTLIVE555_CODEC_UNKNOWN) {
		DBG_ERR("%s():NVTLIVE555_CODEC_UNKNOWN\n", __func__);
		cb_info.err_code = IACOMM_RTSP_CODEC_UNKNOWN;
		_iacomm_cb(channel, cb_event, &cb_info, cb_name);
		return 0;
	}
	live555_video_info[channel].ref_cnt++;
	flush_video(channel);
	ImageApp_Common_DUMP("%s():OK\r\n", __func__);
	cb_info.err_code = IACOMM_RTSP_OK;
	_iacomm_cb(channel, cb_event, &cb_info, cb_name);
	return (int)&live555_video_info[channel];
}

int live555_on_close_video(int handle)
{
	UINT32 id = 0;
	IACOMM_USER_CB_EVENT cb_event = IACOMM_USER_CB_EVENT_RTSP_CLOSE;
	char cb_name[] = "IACOMM_USER_CB_EVENT_RTSP_CLOSE";
	IACOMM_RTSP_CB_INFO cb_info = {0};

	cb_info.type = IACOMM_RTSP_BS_VIDEO;

	if (handle) {
		VIDEO_INFO *p_info = (VIDEO_INFO *)handle;
		p_info->ref_cnt--;
		id = p_info->id;
	}
	ImageApp_Common_DUMP("%s():OK\r\n", __func__);
	cb_info.err_code = IACOMM_RTSP_OK;
	_iacomm_cb(id, cb_event, &cb_info, cb_name);
	wait_i_frame = FALSE;
	return 0;
}

int live555_on_get_video_info(int handle, int timeout_ms, NVTLIVE555_VIDEO_INFO *p_info)
{
	VIDEO_INFO *p_video_info = (VIDEO_INFO *)handle;

	p_info->codec_type = p_video_info->codec_type;
	p_info->vps_size = p_video_info->vps_size;
	p_info->sps_size = p_video_info->sps_size;
	p_info->pps_size = p_video_info->pps_size;

	if (p_video_info->vps_size) {
		memcpy(p_info->vps, p_video_info->vps, p_info->vps_size);
	}
	if (p_video_info->sps_size) {
		memcpy(p_info->sps, p_video_info->sps, p_info->sps_size);
	}
	if (p_video_info->pps_size) {
		memcpy(p_info->pps, p_video_info->pps, p_info->pps_size);
	}
	ImageApp_Common_DUMP("%s():OK\r\n", __func__);
	return  0;
}

int live555_on_lock_video(int handle, int timeout_ms, NVTLIVE555_STRM_INFO *p_strm)
{
	FLGPTN uiFlag;
	VIDEO_INFO *p_video_info = (VIDEO_INFO *)handle;
#if (IACOMMON_DEBUG_VTIME == ENABLE)
	static UINT32 tick[31];
	static UINT32 tick_idx = 0;
#endif
	IACOMM_USER_CB_EVENT cb_event = IACOMM_USER_CB_ERROR_RTSP_BS_ERR;
	char cb_name[] = "IACOMM_USER_CB_ERROR_RTSP_BS_ERR";
	IACOMM_RTSP_CB_INFO cb_info = {0};
	UINT32 keep_sem_flag = 0;

	vos_sem_wait(IACOMMON_SEMID_LOCK_VIDEO);

	if (handle == 0) {
		DBG_ERR("handle NULL\r\n");
		cb_info.err_code = IACOMM_RTSP_FAIL;
		_iacomm_cb(p_video_info->id, cb_event, &cb_info, cb_name);
		vos_sem_sig(IACOMMON_SEMID_LOCK_VIDEO);
		return -1;
	}

	if (p_video_info == NULL) {
		DBG_ERR("p_video_info NULL\r\n");
		cb_info.err_code = IACOMM_RTSP_FAIL;
		_iacomm_cb(p_video_info->id, cb_event, &cb_info, cb_name);
		vos_sem_sig(IACOMMON_SEMID_LOCK_VIDEO);
		return -1;
	}

	if (p_strm == NULL) {
		DBG_ERR("p_strm NULL\r\n");
		cb_info.err_code = IACOMM_RTSP_FAIL;
		_iacomm_cb(p_video_info->id, cb_event, &cb_info, cb_name);
		vos_sem_sig(IACOMMON_SEMID_LOCK_VIDEO);
		return -1;
	}

	cb_info.type = IACOMM_RTSP_BS_VIDEO;

	if (vir_addr_vstrm[p_video_info->id] == 0) {
		DBG_ERR("vir_addr_vstrm[%d] == 0\r\n", p_video_info->id);
		cb_info.err_code = IACOMM_RTSP_FAIL;
		_iacomm_cb(p_video_info->id, cb_event, &cb_info, cb_name);
		vos_sem_sig(IACOMMON_SEMID_LOCK_VIDEO);
		return -1;
	}

ON_LOCK_VIDEO_RETRY:
	if (iacomm_flag_wait_timeout(&uiFlag, IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_BS_RDY << p_video_info->id), (TWF_ORW | TWF_CLR), vos_util_msec_to_tick(timeout_ms)) == E_TMOUT) {
		FLGPTN chkptn;
		chkptn = vos_flag_chk(IACOMMON_FLG_ID, FLGPTN_BIT_ALL);
		DBG_ERR("wait_timeout(uiFlag=%lx IACOMMON_FLG_ID=%lu, p_video_info->id = %lu timeout_ms=%lu)\r\n", chkptn, IACOMMON_FLG_ID, p_video_info->id, timeout_ms);
		cb_info.err_code = IACOMM_RTSP_FAIL;
		_iacomm_cb(p_video_info->id, cb_event, &cb_info, cb_name);
		vos_sem_sig(IACOMMON_SEMID_LOCK_VIDEO);
		return -1;
	}

	ImageApp_Common_GetParam(0, IACOMMON_PARAM_MISC_KEEP_SEM_FLAG, &keep_sem_flag);
	if (keep_sem_flag && iacomm_reinited) {
		//wait_i_frame = TRUE;
		iacomm_reinited = FALSE;
		flush_video(p_video_info->id);
		DBG_DUMP("live555_on_lock_video flush video since ImageApp_Common reinited\r\n");
		goto ON_LOCK_VIDEO_RETRY;
	}

	iacomm_flag_clr(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_RQUE_FREE << p_video_info->id));

	if ((p_video_info->pVBuf = (STRM_VBUFFER_ELEMENT *)_QUEUE_DeQueueFromHead(&VBufferQueueReady[p_video_info->id])) == NULL) {
		iacomm_flag_set(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_RQUE_FREE << p_video_info->id));
		DBG_ERR("Queue is NULL\r\n");
		cb_info.err_code = IACOMM_RTSP_BS_UNDERRUN;
		_iacomm_cb(p_video_info->id, cb_event, &cb_info, cb_name);
		vos_sem_sig(IACOMMON_SEMID_LOCK_VIDEO);
		return -1;
	}

	if (wait_i_frame == TRUE) {
		if (((p_video_info->codec_type == NVTLIVE555_CODEC_H264) || (p_video_info->codec_type == NVTLIVE555_CODEC_H265)) && (p_video_info->pVBuf->pack_num == 1)) {
			DBG_DUMP("%s: The first frame is not I, skip.\r\n", __func__);
			_QUEUE_EnQueueToTail(&VBufferQueueClear[p_video_info->id], (QUEUE_BUFFER_ELEMENT *)p_video_info->pVBuf);
			iacomm_flag_set(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_RQUE_FREE << p_video_info->id));
			goto ON_LOCK_VIDEO_RETRY;
		}
		wait_i_frame = FALSE;
	}

#if (IACOMMON_DEBUG_VTIME == ENABLE)
	vos_perf_mark(&(tick[tick_idx]));
	tick[tick_idx] /= 1000;
	tick_idx ++;
	if (tick_idx == 31) {
		DBG_DUMP("[%d.%03d]", tick[30]/1000, tick[30]%1000);
		for (tick_idx = 0; tick_idx < 30; tick_idx++) {
			DBG_DUMP("%d, ", tick[tick_idx + 1] - tick[tick_idx]);
		}
		DBG_DUMP("\r\n");
		tick[0] = tick[30];
		tick_idx = 1;
	}
#endif

	if (p_video_info->pVBuf == NULL) {
		DBG_ERR("pVBuf null!!!\r\n");
		vos_sem_sig(IACOMMON_SEMID_LOCK_VIDEO);
		return -1;
	}

	if (p_video_info->pVBuf->phy_addr[p_video_info->pVBuf->pack_num-1] == 0) {
		DBG_ERR("phy_addr 0!!!\r\n");
		vos_sem_sig(IACOMMON_SEMID_LOCK_VIDEO);
		return -1;
	}
	if (vir_addr_vstrm[p_video_info->id] == 0) {
		DBG_ERR("vir_addr_vstrm[%d] == 0  \r\n", p_video_info->id);
		cb_info.err_code = IACOMM_RTSP_FAIL;
		_iacomm_cb(p_video_info->id, cb_event, &cb_info, cb_name);
		vos_sem_sig(IACOMMON_SEMID_LOCK_VIDEO);
		return -1;
	}

	p_strm->addr = PHY2VIRT_VSTRM(p_video_info->id, p_video_info->pVBuf->phy_addr[p_video_info->pVBuf->pack_num-1]);
	p_strm->size = p_video_info->pVBuf->size[p_video_info->pVBuf->pack_num-1];
	p_strm->timestamp = p_video_info->pVBuf->timestamp;

	char *ptr = (char*)p_strm->addr;

	if (ptr == NULL) {
		DBG_ERR("ptr null!!!\r\n");
		vos_sem_sig(IACOMMON_SEMID_LOCK_VIDEO);
		return -1;
	}

	if (ptr[0] != 0x00 || ptr[1] != 0x00 || ptr[2] != 0x00 || ptr[3] != 0x01) {
		DBG_ERR("vbuffer is overwritten!!!\r\n");
		cb_info.err_code = IACOMM_RTSP_BS_OVERRUN;
		_iacomm_cb(p_video_info->id, cb_event, &cb_info, cb_name);
	}
	if (vir_addr_vstrm[p_video_info->id] == 0) {
		DBG_ERR("vir_addr_vstrm[%d] == 0\r\n", p_video_info->id);
		cb_info.err_code = IACOMM_RTSP_FAIL;
		_iacomm_cb(p_video_info->id, cb_event, &cb_info, cb_name);
		vos_sem_sig(IACOMMON_SEMID_LOCK_VIDEO);
		return -1;
	}
	//DBG_DUMP("%s():OK\r\n", __func__);
	vos_sem_sig(IACOMMON_SEMID_LOCK_VIDEO);
	return 0;
}

int live555_on_unlock_video(int handle)
{
	VIDEO_INFO *p_video_info = (VIDEO_INFO *)handle;

	_QUEUE_EnQueueToTail(&VBufferQueueClear[p_video_info->id], (QUEUE_BUFFER_ELEMENT *)p_video_info->pVBuf);
	iacomm_flag_set(IACOMMON_FLG_ID, (FLGIACOMMON_VE_S0_RQUE_FREE << p_video_info->id));

	return 0;
}

UINT32 live555_mmap_aenc_bs(UINT32 id, HD_PATH_ID path_id)
{
	UINT32 i = id, pid = path_id;
	HD_RESULT hd_ret;

	if (i >= MAX_RTSP_PATH) {
		return 0;
	}

	live555_audio_info[i].aenc_p = pid;

	if ((hd_ret = hd_audioenc_get(pid, HD_AUDIOENC_PARAM_BUFINFO, &(phy_buf_astrm[i]))) != HD_OK) {
		vir_addr_astrm[i] = 0;
		DBG_ERR("Get HD_AUDIOENC_PARAM_BUFINFO fail(%d)\r\n", hd_ret);
	} else {
		vir_addr_astrm[i] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_astrm[i].buf_info.phy_addr, phy_buf_astrm[i].buf_info.buf_size);
	}
	ImageApp_Common_DUMP("A_BSBuf=%x,%x\r\n", phy_buf_astrm[i].buf_info.phy_addr, phy_buf_astrm[i].buf_info.buf_size);

	return vir_addr_astrm[i];
}

UINT32 live555_munmap_aenc_bs(UINT32 id)
{
	UINT32 i = id;
	HD_RESULT hd_ret;

	if (i >= MAX_RTSP_PATH) {
		return 0;
	}

	if ((hd_ret = hd_common_mem_munmap((void *)vir_addr_astrm[i], phy_buf_astrm[i].buf_info.buf_size)) != HD_OK) {
		DBG_ERR("hd_common_mem_munmap fail(%d), addr=%0x%x, 0x%x\r\n", hd_ret, vir_addr_astrm[i],phy_buf_astrm[i].buf_info.phy_addr);
	}
	vir_addr_astrm[i] = 0;
	memset(&(phy_buf_astrm[i]), 0, sizeof(phy_buf_astrm[i]));

	return 0;
}

int live555_on_open_audio(int channel)
{
	IACOMM_USER_CB_EVENT cb_event = IACOMM_USER_CB_EVENT_RTSP_OPEN;
	char cb_name[] = "IACOMM_USER_CB_EVENT_RTSP_OPEN";
	IACOMM_RTSP_CB_INFO cb_info = {0};

	cb_info.type = IACOMM_RTSP_BS_AUDIO;

	if ((size_t)channel >= sizeof(live555_audio_info) / sizeof(live555_audio_info[0])) {
		DBG_ERR("nvtrtspd audio channel exceed\n");
		cb_info.err_code = IACOMM_RTSP_CHANNEL_EXCEED;
		_iacomm_cb(channel, cb_event, &cb_info, cb_name);
		return 0;
	}
	if (live555_audio_info[channel].ref_cnt != 0) {
		DBG_ERR("nvtrtspd audio in use.\n");
		cb_info.err_code = IACOMM_RTSP_CHANNEL_IN_USE;
		_iacomm_cb(channel, cb_event, &cb_info, cb_name);
		//return 0;
		return (int)&live555_audio_info[channel];
	}
	//no audio
	if (live555_audio_info[channel].info.codec_type == NVTLIVE555_CODEC_UNKNOWN) {
		DBG_ERR("%s():NVTLIVE555_CODEC_UNKNOWN\n", __func__);
		cb_info.err_code = IACOMM_RTSP_CODEC_UNKNOWN;
		_iacomm_cb(channel, cb_event, &cb_info, cb_name);
		return 0;
	}
	live555_audio_info[channel].ref_cnt++;
	flush_audio(channel);
	ImageApp_Common_DUMP("%s():OK\r\n", __func__);
	cb_info.err_code = IACOMM_RTSP_OK;
	_iacomm_cb(channel, cb_event, &cb_info, cb_name);
	return (int)&live555_audio_info[channel];
}

int live555_on_close_audio(int handle)
{
	UINT32 id = 0;
	IACOMM_USER_CB_EVENT cb_event = IACOMM_USER_CB_EVENT_RTSP_CLOSE;
	char cb_name[] = "IACOMM_USER_CB_EVENT_RTSP_CLOSE";
	IACOMM_RTSP_CB_INFO cb_info = {0};

	cb_info.type = IACOMM_RTSP_BS_AUDIO;

	if (handle) {
		AUDIO_INFO *p_info = (AUDIO_INFO *)handle;
		p_info->ref_cnt--;
		id = p_info->id;
	}
	ImageApp_Common_DUMP("%s():OK\r\n", __func__);
	cb_info.err_code = IACOMM_RTSP_OK;
	_iacomm_cb(id, cb_event, &cb_info, cb_name);
	return 0;
}

int live555_on_get_audio_info(int handle, int timeout_ms, NVTLIVE555_AUDIO_INFO *p_info)
{
	AUDIO_INFO *p_audio_info = (AUDIO_INFO *)handle;

	p_info->codec_type = p_audio_info->info.codec_type;
	p_info->sample_per_second = p_audio_info->info.sample_per_second;
	p_info->bit_per_sample = p_audio_info->info.bit_per_sample;
	p_info->channel_cnt = p_audio_info->info.channel_cnt;
	ImageApp_Common_DUMP("%s():OK\r\n", __func__);

	return 0;
}

int live555_on_lock_audio(int handle, int timeout_ms, NVTLIVE555_STRM_INFO *p_strm)
{
	FLGPTN uiFlag;
	AUDIO_INFO *p_audio_info = (AUDIO_INFO *)handle;
	IACOMM_USER_CB_EVENT cb_event = IACOMM_USER_CB_ERROR_RTSP_BS_ERR;
	char cb_name[] = "IACOMM_USER_CB_ERROR_RTSP_BS_ERR";
	IACOMM_RTSP_CB_INFO cb_info = {0};

	if (handle == 0) {
		DBG_ERR("handle NULL\r\n");
		return -1;
	}

	if (p_audio_info == NULL) {
		DBG_ERR("p_audio_info NULL\r\n");
		return -1;
	}

	if (p_strm == NULL) {
		DBG_ERR("p_strm NULL\r\n");
		return -1;
	}

	cb_info.type = IACOMM_RTSP_BS_AUDIO;

	if (vir_addr_astrm[p_audio_info->id] == 0) {
		DBG_ERR("vir_addr_astrm[%d] == 0\r\n", p_audio_info->id);
		cb_info.err_code = IACOMM_RTSP_FAIL;
		_iacomm_cb(p_audio_info->id, cb_event, &cb_info, cb_name);
		return -1;
	}
	if (iacomm_flag_wait_timeout(&uiFlag, IACOMMON_FLG_ID, (FLGIACOMMON_AE_S0_BS_RDY << p_audio_info->id), (TWF_ORW | TWF_CLR), vos_util_msec_to_tick(timeout_ms)) == E_TMOUT) {
		DBG_ERR("wait_timeout\r\n");
		cb_info.err_code = IACOMM_RTSP_FAIL;
		_iacomm_cb(p_audio_info->id, cb_event, &cb_info, cb_name);
		return -1;
	}
	iacomm_flag_clr(IACOMMON_FLG_ID, (FLGIACOMMON_AE_S0_RQUE_FREE << p_audio_info->id));

	if ((p_audio_info->pABuf = (STRM_ABUFFER_ELEMENT *)_QUEUE_DeQueueFromHead(&ABufferQueueReady[p_audio_info->id])) == NULL) {
		iacomm_flag_set(IACOMMON_FLG_ID, (FLGIACOMMON_AE_S0_RQUE_FREE << p_audio_info->id));
		DBG_ERR("Queue is NULL\r\n");
		cb_info.err_code = IACOMM_RTSP_BS_UNDERRUN;
		_iacomm_cb(p_audio_info->id, cb_event, &cb_info, cb_name);
		return -1;
	}

	p_strm->addr = PHY2VIRT_ASTRM(p_audio_info->id, p_audio_info->pABuf->phy_addr);
	p_strm->size = p_audio_info->pABuf->size;
	p_strm->timestamp = p_audio_info->pABuf->timestamp;

	return 0;
}

int live555_on_unlock_audio(int handle)
{
	AUDIO_INFO *p_audio_info = (AUDIO_INFO *)handle;

	_QUEUE_EnQueueToTail(&ABufferQueueClear[p_audio_info->id], (QUEUE_BUFFER_ELEMENT *)p_audio_info->pABuf);
	iacomm_flag_set(IACOMMON_FLG_ID, (FLGIACOMMON_AE_S0_RQUE_FREE << p_audio_info->id));

	return 0;
}

