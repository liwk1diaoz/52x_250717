#include "GroupsockHelper.hh"
#include "PatternFrm.h"
#include "NvtStreamFramer.hh"
#include "NvtMgr.hh"
#include <list>

typedef struct _STRM_HANDLE {
	int hDev;
	int Channel;
	unsigned long long SeqNum;
	struct timeval tm;
	NvtStreamFramer *pFramer;
	NVT_MEDIA_TYPE MediaType;
	int frame_cnt; //for debug fps
}STRM_HANDLE;

static std::list <STRM_HANDLE *> g_strms; ///< list to manage clients

static int xIOpenStrm(int channel, NVT_MEDIA_TYPE media_type, void *p_framer)
{
	int hDev = 0;
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	NVTLIVE555_HDAL_CB *pCb = pNvtMgr->Get_HdalCb();

	if (media_type == NVT_MEDIA_TYPE_VIDEO) {
		hDev = pCb->open_video(channel);
	} else if (media_type == NVT_MEDIA_TYPE_AUDIO) {
		hDev = pCb->open_audio(channel);
	} else {
		printf("live555: xIOpenStrm(): unhandled mediatype: %d\r\n", (int)media_type);
		return 0;
	}
	if (hDev == 0) {
		return 0;
	}

	STRM_HANDLE *pHdl = new STRM_HANDLE;
	memset(pHdl, 0, sizeof(*pHdl));
	pHdl->hDev = hDev;
	pHdl->pFramer = (NvtStreamFramer *)p_framer;
	pHdl->Channel = channel;
	pHdl->MediaType = media_type;

	pNvtMgr->GlobalLock();
	g_strms.push_back(pHdl);
	pNvtMgr->GlobalUnlock();
	return (int)pHdl;
}

static int xICloseStrm(int h_strm)
{
	int er;
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	NVTLIVE555_HDAL_CB *pCb = pNvtMgr->Get_HdalCb();
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;

	if (pHdl->MediaType == NVT_MEDIA_TYPE_VIDEO) {
		er = pCb->close_video(pHdl->hDev);
	}
	else if (pHdl->MediaType == NVT_MEDIA_TYPE_AUDIO) {
		er = pCb->close_audio(pHdl->hDev);
	} else {
		printf("live555: xICloseStrm(): unhandled mediatype: %d\r\n", (int)pHdl->MediaType);
		return 0;
	}

	if (er != 0) {
		printf("live555: failed to xICloseStrm(), er = %d\r\n", er);
	}

	pNvtMgr->GlobalLock();
	for (std::list <STRM_HANDLE *>::iterator i = g_strms.begin();
		i != g_strms.end(); i++) {
		if ((*i) == pHdl) {
			delete *i;
			*i = NULL;
			break;
		}
	}
	g_strms.remove(NULL);
	pNvtMgr->GlobalUnlock();
	return 0;
}


static int xIGetVideoInfo(int h_strm, int channel, int timeout_ms, NVT_VIDEO_INFO *p_info)
{
	int er;
	int retry = 5;
	NVTLIVE555_VIDEO_INFO video_info;
	NVTLIVE555_HDAL_CB *pCb = NvtMgr_GetHandle()->Get_HdalCb();
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;

	memset(&video_info, 0, sizeof(video_info));
	while ((er = pCb->get_video_info(pHdl->hDev, timeout_ms, &video_info)) == NVTLIVE555_ER_RETRY && retry--) {
		SLEEP_MS(500);
	}
	if (er < 0) {
		if (retry < 0) {
			printf("live555: xIGetVideoInfo retry time out. channel = %d\r\n", channel);
		} else {
			printf("live555: failed to xIGetVideoInfo, channel = %d, er = %d\r\n", channel, er);
		}
		return er;
	}

	switch (video_info.codec_type)	{
	case NVTLIVE555_CODEC_MJPG:
		p_info->codec_type = NVT_CODEC_TYPE_MJPG;
		break;
	case NVTLIVE555_CODEC_H264:
		p_info->codec_type = NVT_CODEC_TYPE_H264;
		break;
	case NVTLIVE555_CODEC_H265:
		p_info->codec_type = NVT_CODEC_TYPE_H265;
		break;
	default:
		printf("live555: xIGetVideoInfo cannot parse CodecType = %d\r\n", video_info.codec_type);
		return -1;
	}

	if (video_info.vps_size) {
		p_info->vps_size = video_info.vps_size;
		memcpy(p_info->vps, video_info.vps, video_info.vps_size);
	}
	if (video_info.sps_size) {
		p_info->sps_size = video_info.sps_size;
		memcpy(p_info->sps, video_info.sps, video_info.sps_size);
	}
	if (video_info.pps_size) {
		p_info->pps_size = video_info.pps_size;
		memcpy(p_info->pps, video_info.pps, video_info.pps_size);
	}
	return 0;
}

static int xIGetVideoStrm(int h_strm, int channel, int timeout_ms, NVT_STRM_INFO *p_strm)
{
	int er;
	struct timeval now;
	NVTLIVE555_STRM_INFO vstream = { 0 };
	NVTLIVE555_HDAL_CB *pCb = NvtMgr_GetHandle()->Get_HdalCb();
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;

	gettimeofday(&p_strm->latency.begin_wait, NULL);

	er = pCb->lock_video(pHdl->hDev, timeout_ms, &vstream);
	if (er < 0) {
		if (er != NVTLIVE555_ER_RETRY) {
			printf("live555: xIGetVideoStrm got er = %d, channel = %d\r\n", er, channel);
			return er;
		} else {
			gettimeofday(&now, NULL);
			if (((now.tv_sec - p_strm->latency.begin_wait.tv_sec) * 1000 +
				(now.tv_usec - p_strm->latency.begin_wait.tv_usec) / 1000) < 5) {
				return -1;
			}
			return NVT_COMMON_ER_EAGAIN;
		}
	}

	//keep last information for debug
	pHdl->tm.tv_sec = (long)(vstream.timestamp / 1000000);
	pHdl->tm.tv_usec = (long)(vstream.timestamp % 1000000);
	pHdl->frame_cnt++;

	p_strm->seq_num = pHdl->SeqNum;
	p_strm->addr = vstream.addr;
	p_strm->size = vstream.size;
	p_strm->tm = pHdl->tm;
	p_strm->svc_idx = vstream.svc_idx;
	gettimeofday(&p_strm->latency.wait_done, NULL);
	pHdl->SeqNum++;
	return 0;
}

static int xIReleaseVideoStrm(int h_strm, int channel)
{
	int er;
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;
	NVTLIVE555_HDAL_CB *pCb = NvtMgr_GetHandle()->Get_HdalCb();
	er = pCb->unlock_video(pHdl->hDev);
	return er;
}

/**
ADTS parser
const int sampling_frequencies[] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000,	7350 };
p_adts[0] = 0xFF
p_adts[1] != 0xF9
int profile_index = (p_adts[2] >> 2) & 0xF;
int channel_cnt = ((p_adts[2] & 0x1) << 2) | (p_adts[3] >> 6);
**/
static int xIGetAudioInfo(int h_strm, int channel, int timeout_ms, NVT_AUDIO_INFO *p_info)
{
	int er;
	int retry = 5;
	NVTLIVE555_AUDIO_INFO audio_info;
	NVTLIVE555_HDAL_CB *pCb = NvtMgr_GetHandle()->Get_HdalCb();
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;

	memset(&audio_info, 0, sizeof(audio_info));
	while ((er = pCb->get_audio_info(pHdl->hDev, timeout_ms, &audio_info)) == NVTLIVE555_ER_RETRY && retry--) {
		SLEEP_MS(500);
	}

	if (er < 0) {
		if (retry < 0) {
			printf("live555: xIGetAudioInfo retry time out. channel = %d\r\n", channel);
		} else {
			printf("live555: failed to xIGetAudioInfo, channel = %d, er = %d\r\n", channel, er);
		}
		return er;
	}

	switch (audio_info.codec_type) {
	case NVTLIVE555_CODEC_PCM:
		p_info->codec_type = NVT_CODEC_TYPE_PCM;
		break;
	case NVTLIVE555_CODEC_AAC:
		p_info->codec_type = NVT_CODEC_TYPE_AAC;
		break;
	case NVTLIVE555_CODEC_G711_ALAW:
		p_info->codec_type = NVT_CODEC_TYPE_G711_ALAW;
		break;
	case NVTLIVE555_CODEC_G711_ULAW:
		p_info->codec_type = NVT_CODEC_TYPE_G711_ULAW;
		break;
	default:
		printf("live555: xIGetAudioInfo cannot parse CodecType = %d\r\n", audio_info.codec_type);
		return -1;
	}
	p_info->bit_per_sample = audio_info.bit_per_sample;
	p_info->channel_cnt = audio_info.channel_cnt;
	p_info->sample_per_second = audio_info.sample_per_second;
	return 0;
}

static int xIGetAudioStrm(int h_strm, int channel, int timeout_ms, NVT_STRM_INFO *p_strm)
{
	int er;
	struct timeval now;
	NVTLIVE555_STRM_INFO astream = { 0 };
	NVTLIVE555_HDAL_CB *pCb = NvtMgr_GetHandle()->Get_HdalCb();
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;

	gettimeofday(&p_strm->latency.begin_wait, NULL);

	er = pCb->lock_audio(pHdl->hDev, timeout_ms, &astream);
	if (er < 0) {
		if (er != NVTLIVE555_ER_RETRY) {
			printf("live555: xIGetAudioStrm got er = %d, channel = %d\r\n", er, channel);
			return er;
		} else {
			gettimeofday(&now, NULL);
			if (((now.tv_sec - p_strm->latency.begin_wait.tv_sec) * 1000 +
				(now.tv_usec - p_strm->latency.begin_wait.tv_usec) / 1000) < 5) {
				return -1;
			}
			return NVT_COMMON_ER_EAGAIN;
		}
	}

	//keep last information for debug
	pHdl->tm.tv_sec = (long)(astream.timestamp / 1000000);
	pHdl->tm.tv_usec = (long)(astream.timestamp % 1000000);
	pHdl->frame_cnt++;

	p_strm->seq_num = pHdl->SeqNum;
	p_strm->addr = astream.addr;
	p_strm->size = astream.size;
	p_strm->tm = pHdl->tm;
	p_strm->svc_idx = astream.svc_idx;
	gettimeofday(&p_strm->latency.wait_done, NULL);
	pHdl->SeqNum++;
	return 0;
}

static int xIReleaseAudioStrm(int h_strm, int channel)
{
	int er;
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;
	NVTLIVE555_HDAL_CB *pCb = NvtMgr_GetHandle()->Get_HdalCb();
	er = pCb->unlock_audio(pHdl->hDev);
	return er;
}

static int dbg_cmd_dump()
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	if (g_strms.size() == 0) {
		printf("live555: no client.\r\n");
	}
	pNvtMgr->GlobalLock();
	for (std::list <STRM_HANDLE *>::iterator i = g_strms.begin();
		i != g_strms.end(); i++) {
		STRM_HANDLE *pHdl = *i;
		if (pHdl->pFramer) {
			pHdl->pFramer->DumpInfo();
		}
	}
	pNvtMgr->GlobalUnlock();
	return 0;
}

#if defined(__ECOS) || defined(WIN32)
static u_int32_t g_stack[2048 / sizeof(u_int32_t)];
static THREAD_OBJ  m_hFps_Obj;
#endif
static THREAD_HANDLE m_hFps;
static THREAD_DECLARE(ThreadFps, lpParam)
{
	const int eval_sec = 15; //evaluate in 15 sec
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	if (g_strms.size() == 0) {
		printf("live555: no client.\r\n");
	}
	printf("live555: please wait %d sec.\r\n", eval_sec);
	pNvtMgr->GlobalLock();
	for (std::list <STRM_HANDLE *>::iterator i = g_strms.begin();
		i != g_strms.end(); i++) {
		STRM_HANDLE *pHdl = *i;
		pHdl->frame_cnt = 0;
	}
	pNvtMgr->GlobalUnlock();
	SLEEP_SEC(eval_sec);
	pNvtMgr->GlobalLock();
	for (std::list <STRM_HANDLE *>::iterator i = g_strms.begin();
		i != g_strms.end(); i++) {
		STRM_HANDLE *pHdl = *i;
		char MediaType;
		switch (pHdl->pFramer->GetMediaType())
		{
		case NVT_MEDIA_TYPE_VIDEO:
			MediaType = 'V';
			break;
		case NVT_MEDIA_TYPE_AUDIO:
			MediaType = 'A';
			break;
		case NVT_MEDIA_TYPE_META:
			MediaType = 'M';
			break;
		default:
			MediaType = 'N';
			break;
		}
		printf("%08X(%c): %f\r\n", (int)pHdl->pFramer->GetSessionID(), MediaType, (float)pHdl->frame_cnt / eval_sec);
	}
	pNvtMgr->GlobalUnlock();

	THREAD_EXIT();
}

static int dbg_cmd_fps()
{
#if defined(__ECOS)
	u_int32_t uiStackSize = sizeof(g_stack);
#endif
	THREAD_CREATE("RTSP_FPS", m_hFps, ThreadFps, NULL, g_stack, uiStackSize, &m_hFps_Obj);
	THREAD_RESUME(m_hFps);
	return 0;
}

static int xIDbgCmd(NVT_DBG_CMD cmd)
{
	int er = -1;
	switch (cmd)
	{
	case NVT_DBG_CMD_DUMP:
		er = dbg_cmd_dump();
		break;
	case NVT_DBG_CMD_FPS:
		er = dbg_cmd_fps();
		break;
	default:
		break;
	}
	return er;
}

static ISTRM_CB m_InterfaceCb = {
	xIOpenStrm,
	xICloseStrm,
	xIGetVideoStrm,
	xIReleaseVideoStrm,
	xIGetVideoInfo,
	xIGetAudioStrm,
	xIReleaseAudioStrm,
	xIGetAudioInfo,
	xIDbgCmd,
};

ISTRM_CB *HdalFrmGetStrmCb()
{
	return &m_InterfaceCb;
}