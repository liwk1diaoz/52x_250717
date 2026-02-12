#include "GroupsockHelper.hh"
#include "PatternFrm.h"
#include "NvtStreamFramer.hh"
#include "NvtMgr.hh"

#if (CFG_FAKE_INTERFACE)
#include <list>
typedef struct _STRM_HANDLE {
	unsigned long long SeqNum;
	int Channel;
	struct timeval tm;
	struct timeval tm_begin;
	NvtStreamFramer *pFramer;
	int frame_cnt; //for debug fps 
}STRM_HANDLE;

static std::list <STRM_HANDLE *> g_strms; ///< list to manage clients

static int xIOpenStrm(int channel, NVT_MEDIA_TYPE media_type, void *p_framer)
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	STRM_HANDLE *p = new STRM_HANDLE;
	memset(p, 0, sizeof(*p));
	p->pFramer = (NvtStreamFramer *)p_framer;
	p->Channel = channel;
	pNvtMgr->GlobalLock();
	g_strms.push_back(p);
	pNvtMgr->GlobalUnlock();
	return (int)p;
}

static int xICloseStrm(int h_strm)
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;
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
	//STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;
	STATIC_CHANNEL *pChannel = StaticFrmGetChannel(channel);

	if (pChannel == NULL) {
		return -1;
	}

	STATIC_STRM_VIDEO *pVideo = &pChannel->Video;
	p_info->codec_type = pVideo->Codec;
	if (pVideo->VPS.pBuf) {
		memcpy(p_info->vps, pVideo->VPS.pBuf, pVideo->VPS.Size);
		p_info->vps_size = pVideo->VPS.Size;
	}
	if (pVideo->SPS.pBuf) {
		memcpy(p_info->sps, pVideo->SPS.pBuf, pVideo->SPS.Size);
		p_info->sps_size = pVideo->SPS.Size;
	}
	if (pVideo->PPS.pBuf) {
		memcpy(p_info->pps, pVideo->PPS.pBuf, pVideo->PPS.Size);
		p_info->pps_size = pVideo->PPS.Size;
	}
	return 0;
}

static int xIGetVideoStrm(int h_strm, int channel, int timeout_ms, NVT_STRM_INFO *p_strm)
{
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;
	STATIC_STRM_VIDEO *pVideo = &StaticFrmGetChannel(channel)->Video;
	INT32 Idx = (pHdl->SeqNum+1) & 0x1; //p-frame first to test wait i-frame

	gettimeofday(&p_strm->latency.begin_wait, NULL);

	// calc timestamp
	u_int32_t frame_time_us = 1000000 / CFG_RTSPD_FAKE_FPS;
	u_int64_t tm_64 = (pHdl->SeqNum / CFG_RTSPD_FAKE_FPS) * 1000000
		+ (pHdl->SeqNum % CFG_RTSPD_FAKE_FPS)*frame_time_us;
	p_strm->tm.tv_sec = (long)(tm_64 / 1000000);
	p_strm->tm.tv_usec = (long)(tm_64 % 1000000);

	if (tm_64 == 0) {
		gettimeofday(&pHdl->tm_begin, NULL);
	} else {
		struct timeval tm;
		gettimeofday(&tm, NULL);
		int64_t tm_64_expect = (int64_t)pHdl->tm_begin.tv_sec * 1000000 + pHdl->tm_begin.tv_usec + tm_64;
		int64_t tm_64_curr = (int64_t)tm.tv_sec * 1000000 + tm.tv_usec;
		int delay_ms = (int)(tm_64_expect - tm_64_curr) / 1000;
		if (delay_ms > 0) {
#if defined(_WIN32)
			Sleep(delay_ms);
#else
			usleep(delay_ms * 1000);
#endif
		}
	}

	//keep last information for debug
	pHdl->tm = p_strm->tm;
	pHdl->frame_cnt++;
	
	p_strm->seq_num = pHdl->SeqNum;
	p_strm->addr = (UINT32)pVideo->Frm[Idx].pBuf;
	p_strm->size = pVideo->Frm[Idx].Size;
	p_strm->svc_idx = 0;
	gettimeofday(&p_strm->latency.wait_done, NULL);
	pHdl->SeqNum++;

#if 0
	if (pHdl->SeqNum == 30) {
		NvtMgr *pNvtMgr = NvtMgr_GetHandle();
		static NvtMgr::MSG_CMD g_MsgCmd;
		g_MsgCmd.argc = 3;
		strcpy(g_MsgCmd.argv[1], "audio");
		strcpy(g_MsgCmd.argv[2], "off");
		NvtMgr::TASK task = { pNvtMgr->DispatchCmd, &g_MsgCmd };
		pNvtMgr->PushJob(&task);
	}
#endif
	return 0;
}

static int xIReleaseVideoStrm(int h_strm, int channel)
{
	return 0;
}

static int xIGetAudioInfo(int h_strm, int channel, int timeout_ms, NVT_AUDIO_INFO *p_info)
{
	//STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;
	STATIC_CHANNEL *pChannel = StaticFrmGetChannel(channel);

	if (pChannel == NULL) {
		return -1;
	}

	STATIC_STRM_AUDIO *pAudio = &pChannel->Audio;
	p_info->codec_type = pAudio->Codec;
	p_info->sample_per_second = pAudio->SamplePerSecond;
	p_info->bit_per_sample = pAudio->BitPerSample;
	p_info->channel_cnt = pAudio->ChannelCnt;
	return 0;
}

static int xIGetAudioStrm(int h_strm, int channel, int timeout_ms, NVT_STRM_INFO *p_strm)
{
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;
	STATIC_CHANNEL *pChannel = StaticFrmGetChannel(channel);
	if (pChannel == NULL) {
		return -1;
	}
	STATIC_STRM_AUDIO *pAudio = &pChannel->Audio;
	INT32 Idx = pHdl->SeqNum & 0x1;

	gettimeofday(&p_strm->latency.begin_wait, NULL);

	// calc timestamp
	u_int32_t frame_time_us = (u_int32_t)((u_int64_t)pAudio->Frm->Size * 1000000 / (pAudio->SamplePerSecond
		* pAudio->ChannelCnt * pAudio->BitPerSample / 8));
	u_int64_t tm_64 = pHdl->SeqNum * frame_time_us;
	p_strm->tm.tv_sec = (long)(tm_64 / 1000000);
	p_strm->tm.tv_usec = (long)(tm_64 % 1000000);

	if (tm_64 == 0) {
		gettimeofday(&pHdl->tm_begin, NULL);
	} else {
		struct timeval tm;
		gettimeofday(&tm, NULL);
		int64_t tm_64_expect = (int64_t)pHdl->tm_begin.tv_sec * 1000000 + pHdl->tm_begin.tv_usec + tm_64;
		int64_t tm_64_curr = (int64_t)tm.tv_sec * 1000000 + tm.tv_usec;
		int delay_ms = (int)(tm_64_expect - tm_64_curr) / 1000;
		if (delay_ms > 0) {
#if defined(_WIN32)
			Sleep(delay_ms);
#else
			usleep(delay_ms * 1000);
#endif
		}
	}

	//keep last information for debug
	pHdl->tm = p_strm->tm;
	pHdl->frame_cnt++;

	p_strm->seq_num = pHdl->SeqNum;
	p_strm->addr = (UINT32)pAudio->Frm[Idx].pBuf;
	p_strm->size = pAudio->Frm[Idx].Size;
	p_strm->svc_idx = 0;
	gettimeofday(&p_strm->latency.wait_done, NULL);
	pHdl->SeqNum++;
	return 0;
}

static int xIReleaseAudioStrm(int h_strm, int channel)
{
	return 0;
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
static u_int32_t g_stack[1024 / sizeof(u_int32_t)];
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
	SLEEP_MS(eval_sec * 1000);
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

ISTRM_CB *StaticFrmGetStrmCb()
{
	return &m_InterfaceCb;
}
#endif