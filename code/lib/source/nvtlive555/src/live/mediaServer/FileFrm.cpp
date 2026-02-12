#include "GroupsockHelper.hh"
#include "PatternFrm.h"
#include "NvtStreamFramer.hh"
#include "NvtMgr.hh"

#define CFG_FILEFRM_FPS 30

#if (CFG_FAKE_INTERFACE) || defined(WIN32) || defined(__UBUNTU)

#include <list>
typedef struct _STRM_HANDLE {
	unsigned long long SeqNum;
	int Channel;
	struct timeval tm;
	struct timeval tm_begin;
	NvtStreamFramer *pFramer;
	FILE *fptrV;
	unsigned char *pV[2]; //double buffer for multi-thread
	FILE *fptrA;
	unsigned char *pA[2]; //double buffer for multi-thread
	unsigned int *pLst; //only for AAC
	int nLst; //only for AAC
	int frame_cnt; //for debug fps 
	FILEFRM_STATE state;
	//to simulate playback-file with seek
	SEM_HANDLE sem; // semaphore to lock file handle and frame index
}STRM_HANDLE;

static std::list <STRM_HANDLE *> g_strms; ///< list to manage clients

static int xIOpenStrm(int channel, NVT_MEDIA_TYPE media_type, void *p_framer)
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	STRM_HANDLE *p = new STRM_HANDLE;
	memset(p, 0, sizeof(*p));
	p->pFramer = (NvtStreamFramer *)p_framer;
	p->Channel = channel;
	p->state = FILEFRM_STATE_PLAY;
	SEM_CREATE(p->sem, 1);
	pNvtMgr->GlobalLock();
	g_strms.push_back(p);
	pNvtMgr->GlobalUnlock();
	return (int)p;
}

static int xICloseStrm(int h_strm)
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;
	if (pHdl->fptrV) {
		fclose(pHdl->fptrV);
	}
	if (pHdl->fptrA) {
		fclose(pHdl->fptrA);
	}

	for (int i = 0; i < 2; i++) {
		if (pHdl->pV[i]) {
			delete pHdl->pV[i];
			pHdl->pV[i] = NULL;
		}
		if (pHdl->pA[i]) {
			delete pHdl->pA[i];
			pHdl->pA[i] = NULL;
		}
	}

	if (pHdl->pLst) {
		delete pHdl->pLst;
		pHdl->pLst = NULL;
	}

	SEM_DESTROY(pHdl->sem);

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
	STATIC_CHANNEL *pChannel = FileFrmGetChannel(channel);

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
	STATIC_STRM_VIDEO *pVideo = &FileFrmGetChannel(channel)->Video;

	int cnt_ms = 0;
	while (pHdl->state == FILEFRM_STATE_PAUSE && cnt_ms++ < timeout_ms) {
#if defined(_WIN32)
		Sleep(1);
#else
		usleep(1 * 1000);
#endif
	}
	if (pHdl->state == FILEFRM_STATE_PAUSE) {
		return NVT_COMMON_ER_EAGAIN;
	}

	SEM_WAIT(pHdl->sem);

	gettimeofday(&p_strm->latency.begin_wait, NULL);

	if (pHdl->fptrV == NULL) {
		const char *filename_v = (const char *)pVideo->Frm[0].pBuf;
		pHdl->fptrV = fopen(filename_v, "rb");
		if (pHdl->fptrV == NULL) {
			printf("xIGetVideoStrm: open file failed.\r\n");
			SEM_SIGNAL(pHdl->sem);
			return -1;
		}
		pHdl->pV[0] = new unsigned char[0x100000];
		pHdl->pV[1] = new unsigned char[0x100000];
	}

	p_strm->addr = (UINT32)pHdl->pV[pHdl->SeqNum & 0x1];
	unsigned char *p = (unsigned char *)p_strm->addr;
	if (fread(p, 4, 1, pHdl->fptrV) == 0) { //end of file
		printf("xIGetVideoStrm: file end.\r\n");
		pHdl->state = FILEFRM_STATE_PAUSE;
		SEM_SIGNAL(pHdl->sem);
		return NVT_COMMON_ER_EAGAIN;;
	}

	p_strm->size = ((unsigned int)p[0] << 24) | (unsigned int)(p[1] << 16) | ((unsigned int)p[2] << 8) | ((unsigned int)p[3]);
	if (fread(&p[4], 1, p_strm->size, pHdl->fptrV) != p_strm->size) {
		printf("xIGetVideoStrm: fread failed.\r\n");
		SEM_SIGNAL(pHdl->sem);
		return -1;
	}

	// calc timestamp
	u_int32_t frame_time_us = 1000000 / CFG_FILEFRM_FPS;
	u_int64_t tm_64 = (pHdl->SeqNum / CFG_FILEFRM_FPS) * 1000000
		+ (pHdl->SeqNum % CFG_FILEFRM_FPS)*frame_time_us;
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
	p_strm->svc_idx = 0;
	gettimeofday(&p_strm->latency.wait_done, NULL);
	pHdl->SeqNum++;
	SEM_SIGNAL(pHdl->sem);
	return 0;
}

static int xIReleaseVideoStrm(int h_strm, int channel)
{
	return 0;
}

static int xIGetAudioInfo(int h_strm, int channel, int timeout_ms, NVT_AUDIO_INFO *p_info)
{
	STATIC_CHANNEL *pChannel = FileFrmGetChannel(channel);

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
	STATIC_STRM_AUDIO *pAudio = &FileFrmGetChannel(channel)->Audio;
	u_int32_t frame_time_us = 0;

	int cnt_ms = 0;
	while (pHdl->state == FILEFRM_STATE_PAUSE && cnt_ms++ < timeout_ms) {
#if defined(_WIN32)
		Sleep(1);
#else
		usleep(1 * 1000);
#endif
	}
	if (pHdl->state == FILEFRM_STATE_PAUSE) {
		return NVT_COMMON_ER_EAGAIN;
	}

	SEM_WAIT(pHdl->sem);

	gettimeofday(&p_strm->latency.begin_wait, NULL);

	if (pHdl->fptrA == NULL) {
		const char *filename_a = (const char *)pAudio->Frm[0].pBuf;
		pHdl->fptrA = fopen(filename_a, "rb");
		if (pHdl->fptrA == NULL) {
			printf("xIGetAudioStrm: open file failed.\r\n");
			SEM_SIGNAL(pHdl->sem);
			return -1;
		}
		if (pAudio->Codec == NVT_CODEC_TYPE_AAC) {
			const char *filename_aac_list = (const char *)pAudio->Frm[1].pBuf;
			FILE *fptr = fopen(filename_aac_list, "rb");
			if (fptr == NULL) {
				printf("xIGetAudioStrm: open file failed.\r\n");
				SEM_SIGNAL(pHdl->sem);
				return -1;
			}
			fseek(fptr, 0, SEEK_END);
			pHdl->nLst = ftell(fptr) / sizeof(unsigned int);
			pHdl->pLst = new unsigned int[pHdl->nLst];
			fseek(fptr, 0, SEEK_SET);
			fread(pHdl->pLst, pHdl->nLst * sizeof(UINT32), 1, fptr);
			fclose(fptr);
		}
		pHdl->pA[0] = new unsigned char[0x100000];
		pHdl->pA[1] = new unsigned char[0x100000];
	}

	int read_size = 0;
	p_strm->addr = (UINT32)pHdl->pA[pHdl->SeqNum & 0x1];
	unsigned char *p = (unsigned char *)p_strm->addr;

	switch (pAudio->Codec) {
	case NVT_CODEC_TYPE_PCM:
		p_strm->size = ((pAudio->ChannelCnt * pAudio->SamplePerSecond * pAudio->BitPerSample) >> 3) / 64; //packet PCM
		frame_time_us = (u_int32_t)((u_int64_t)p_strm->size * 1000000 / (pAudio->SamplePerSecond
			* pAudio->ChannelCnt * pAudio->BitPerSample / 8));
		break;
	case NVT_CODEC_TYPE_AAC:
		if (pHdl->SeqNum >= (unsigned long long)pHdl->nLst) {
			p_strm->addr = 0;
			p_strm->size = 0;
			SEM_SIGNAL(pHdl->sem);
			return 0;
		}
		p_strm->size = pHdl->pLst[pHdl->SeqNum];
		frame_time_us = (u_int32_t)(1024 * 1000000 / pAudio->SamplePerSecond);
		break;
	case NVT_CODEC_TYPE_G711_ALAW:
	case NVT_CODEC_TYPE_G711_ULAW:
		p_strm->size = 1024;
		frame_time_us = (u_int32_t)((u_int64_t)p_strm->size * 1000000 / (pAudio->SamplePerSecond*pAudio->ChannelCnt));
		break;
	default:
		printf("xIGetAudioStrm: cannot handle codec: %d \r\n", pAudio->Codec);
		SEM_SIGNAL(pHdl->sem);
		return -1;
	}

	if ((read_size = (int)fread(p, 1, p_strm->size, pHdl->fptrA)) != (int)p_strm->size) {
		printf("xIGetAudioStrm: file end.\r\n");
		pHdl->state = FILEFRM_STATE_PAUSE;
		SEM_SIGNAL(pHdl->sem);
		return NVT_COMMON_ER_EAGAIN;
	}

	// calc timestamp
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
	p_strm->svc_idx = 0;
	gettimeofday(&p_strm->latency.wait_done, NULL);
	pHdl->SeqNum++;
	SEM_SIGNAL(pHdl->sem);
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

ISTRM_CB *FileFrmGetStrmCb()
{
	return &m_InterfaceCb;
}

int FileFrmSeek(int channel, double request, double *response)
{
	*response = 0;

	STATIC_CHANNEL *pChannel = FileFrmGetChannel(channel);

	if (pChannel == NULL) {
		return -1;
	}

	for (std::list <STRM_HANDLE *>::iterator i = g_strms.begin();
		i != g_strms.end(); i++) {
		STRM_HANDLE *pHdl = (STRM_HANDLE *)(*i);
		if (pHdl->Channel == channel) {
			if (pChannel->Video.Codec != NVT_CODEC_TYPE_H264 || pChannel->Audio.Codec != NVT_CODEC_TYPE_PCM) {
				printf("Seek supports only H264+PCM\r\n");
				return -1;
			}
			SEM_WAIT(pHdl->sem);
			*response = 29.0;
			if (pHdl->fptrV) {
				fseek(pHdl->fptrV, 14818276, SEEK_SET);
			}
			if (pHdl->fptrA) {
				fseek(pHdl->fptrA, 29 * pChannel->Audio.SamplePerSecond * pChannel->Audio.BitPerSample * pChannel->Audio.ChannelCnt / 8, SEEK_SET);
			}
			pHdl->SeqNum = 0; //simulate timestamp not sync after seek
			SEM_SIGNAL(pHdl->sem);
		}
	}
	return 0;
}

int FileFrmSetState(int channel, FILEFRM_STATE state)
{
	STATIC_CHANNEL *pChannel = FileFrmGetChannel(channel);

	if (pChannel == NULL) {
		return -1;
	}

	for (std::list <STRM_HANDLE *>::iterator i = g_strms.begin();
		i != g_strms.end(); i++) {
		STRM_HANDLE *pHdl = (STRM_HANDLE *)(*i);
		if (pHdl->Channel == channel) {
			pHdl->state = state;
		}
	}

	return 0;
}

int FileFrmGetTotalTime(int channel, unsigned int *total_ms)
{
	STATIC_CHANNEL *pChannel = FileFrmGetChannel(channel);

	if (pChannel == NULL) {
		return -1;
	}
	*total_ms = (unsigned int)pChannel->Video.total_ms;
	return 0;
}

#endif