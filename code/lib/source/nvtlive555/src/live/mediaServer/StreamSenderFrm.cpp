#include "GroupsockHelper.hh"
#include "PatternFrm.h"
#include "NvtStreamFramer.hh"
#include "NvtMgr.hh"

#if defined(__LINUX) && !defined(_NO_NVT_SSENDER_)
#include <protected/nvtstreamsender_protected.h>
#elif defined(__ECOS) && !defined(_NO_NVT_SSENDER_)
#include "../../../../../../uitron/Include/App/p2pcam/nvtstreamsender/nvtstreamsender_protected.h"
#else //for compatible backward
#define    NVTSTREAMSENDER_RET_OK                       ( 0) ///< No error
#define    NVTSTREAMSENDER_RET_ERR                      (-1) ///< Found error
#define    NVTSTREAMSENDER_RET_EAGAIN                   (-2) ///< try again
#define    NVTSTREAMSENDER_RET_EIOCTL                   (-3) ///< ioctl failure
#define    NVTSTREAMSENDER_RET_EINTR                    (-4) ///< interrupt
#define    NVTSTREAMSENDER_RET_ETIME                    (-5) ///< timeout
#define    NVTSTREAMSENDER_VTYPE_NONE		0
#define    NVTSTREAMSENDER_VTYPE_MJPG		1 ///< motion jpeg
#define    NVTSTREAMSENDER_VTYPE_H264		2 ///< h.264
#define    NVTSTREAMSENDER_VTYPE_H265		3 ///< h.265
#define    NVTSTREAMSENDER_ATYPE_NONE		0
#define    NVTSTREAMSENDER_ATYPE_PCM		1
#define    NVTSTREAMSENDER_ATYPE_AAC		2
#define    NVTSTREAMSENDER_ATYPE_PPCM		3
#define    NVTSTREAMSENDER_ATYPE_ULAW		4
#define    NVTSTREAMSENDER_ATYPE_ALAW		5
typedef struct {
	unsigned long long ItemSN; //the serial no. of each stream data
	unsigned int Addr;
	unsigned int Size;
	unsigned int SVCLayerId;
	unsigned int FrameType; //0:P, 1:B, 2:I, 3:IDR, 4:KP
	struct timeval TimeStamp;
}VSTREAM_S;
typedef struct {
	unsigned long long ItemSN; //the serial no. of each stream data
	unsigned int Addr;
	unsigned int Size;
	struct timeval TimeStamp;
}ASTREAM_S;
typedef struct {
	unsigned int Addr;
	unsigned int Size;
	unsigned int CodecType;
}VIDEO_INFO_S;
typedef struct {
	unsigned int CodecType;			///< Audio Codec Type
	unsigned int ChannelCnt;
	unsigned int BitsPerSample;
	unsigned int SampePerSecond;	///< Unit: KHz, e.g. 48000 is 48000KHz
}AUDIO_INFO_S;
int NvtStreamSender_Open(void) { return 0; }
int NvtStreamSender_Close(int hDev) { return 0; }
int NvtStreamSender_GetnLockVStrm(int hDev, int ChanId, int Timeout_ms, VSTREAM_S *pVStream) { return 0; }
int NvtStreamSender_GetnLockAStrm(int hDev, int ChanId, int Timeout_ms, ASTREAM_S *pAStream) { return 0; }
int NvtStreamSender_ReleaseAStrm(int hDev, int ChanId) { return 0; }
int NvtStreamSender_ReleaseVStrm(int hDev, int ChanId) { return 0; }
int NvtStreamSender_GetVideoInfo(int hDev, int ChanId, int Timeout_ms, VIDEO_INFO_S *pVideoInfo) { return 0; }
int NvtStreamSender_GetAudioInfo(int hDev, int ChanId, int Timeout_ms, AUDIO_INFO_S *pAudioInfo) { return 0; }
#endif

#include <list>

typedef enum {
	H264_NALU_TYPE_SLICE = 1,       ///< Coded slice (P-Frame)
	H264_NALU_TYPE_DPA,             ///< Coded data partition A
	H264_NALU_TYPE_DPB,             ///< Coded data partition B
	H264_NALU_TYPE_DPC,             ///< Coded data partition C
	H264_NALU_TYPE_IDR = 5,         ///< Instantaneous decoder refresh (I-Frame)
	H264_NALU_TYPE_SEI,             ///< Supplement Enhancement Information
	H264_NALU_TYPE_SPS = 7,         ///< Sequence parameter sets
	H264_NALU_TYPE_PPS = 8,         ///< Picture parameter sets
	H264_NALU_TYPE_AUD,             ///< AUD
	H264_NALU_TYPE_EOSEQ,           ///< End sequence
	H264_NALU_TYPE_EOSTREAM,        ///< End stream
	H264_NALU_TYPE_FILL,            ///< Filler data
	H264_NALU_TYPE_SPSEXT,          ///< SPS extension
	H264_NALU_TYPE_AUXILIARY,       ///< Auxiliary slice
	H264_NALU_TYPE_MAX,
} H264_NALU_TYPE;

typedef enum {
	H265_NALU_TYPE_SLICE = 1,
	H265_NALU_TYPE_IDR = 19,
	H265_NALU_TYPE_VPS = 32,
	H265_NALU_TYPE_SPS = 33,
	H265_NALU_TYPE_PPS = 34,
	H265_NALU_TYPE_AUD = 35,
	H265_NALU_TYPE_EOS_NUT = 36,
	H265_NALU_TYPE_EOB_NUT = 37,
	H265_NALU_TYPE_SEI_PREFIX = 39,
	H265_NALU_TYPE_SEI_SUFFIX = 40,
} H265_NALU_TYPE;

typedef struct _STRM_HANDLE {
	int hDev;
	unsigned long long SeqNum;
	int Channel;
	struct timeval tm;
	NvtStreamFramer *pFramer;
	int frame_cnt; //for debug fps 
}STRM_HANDLE;

static std::list <STRM_HANDLE *> g_strms; ///< list to manage clients

static int xIOpenStrm(int channel, NVT_MEDIA_TYPE media_type, void *p_framer)
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	STRM_HANDLE *pHdl = new STRM_HANDLE;
	memset(pHdl, 0, sizeof(*pHdl));
	pHdl->pFramer = (NvtStreamFramer *)p_framer;
	pHdl->Channel = channel;
	pHdl->hDev = NvtStreamSender_Open();
	if (pHdl->hDev == 0 || pHdl->hDev == NVTSTREAMSENDER_RET_ERR) {
		printf("live555: failed to NvtStreamSender_Open()\r\n");
		delete pHdl;
		return 0;
	}
	pNvtMgr->GlobalLock();
	g_strms.push_back(pHdl);
	pNvtMgr->GlobalUnlock();
	return (int)pHdl;
}

static int xICloseStrm(int h_strm)
{
	int er;
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;
	if ((er = NvtStreamSender_Close(pHdl->hDev)) != NVTSTREAMSENDER_RET_OK) {
		printf("live555: failed to NvtStreamSender_Close(), er = %d\r\n", er);
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
	unsigned char *p_vps = NULL;
	unsigned char *p_sps = NULL;
	unsigned char *p_pps = NULL;
	VIDEO_INFO_S video_info = { 0 };
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;
	while ((er = NvtStreamSender_GetVideoInfo(pHdl->hDev, channel, timeout_ms, &video_info)) == NVTSTREAMSENDER_RET_EAGAIN &&
		retry--) {
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

	switch (video_info.CodecType)	{
	case NVTSTREAMSENDER_VTYPE_MJPG:
		p_info->codec_type = NVT_CODEC_TYPE_MJPG;
		break;
	case NVTSTREAMSENDER_VTYPE_H264:
		p_info->codec_type = NVT_CODEC_TYPE_H264;
		break;
	case NVTSTREAMSENDER_VTYPE_H265:
		p_info->codec_type = NVT_CODEC_TYPE_H265;
		break;
	default:
		printf("live555: xIGetVideoInfo cannot parse streamsender CodecType = %d\r\n", video_info.CodecType);
		return -1;
	}

	unsigned char *p_buf = (UINT8 *)video_info.Addr;

	while ((unsigned int)p_buf < (video_info.Addr + video_info.Size))
	{
		if (*p_buf == 0x00 && *(p_buf + 1) == 0x00 && *(p_buf + 2) == 0x00 && *(p_buf + 3) == 0x01)
		{
			switch (*(p_buf + 4) & 0x1F)
			{
			case H264_NALU_TYPE_SPS:
				p_sps = p_buf;
				break;
			case H264_NALU_TYPE_PPS:
				p_pps = p_buf;
				break;
			default:
				switch (*(p_buf + 4) >> 0x1)
				{
				case H265_NALU_TYPE_SPS:
					p_sps = p_buf;
					break;
				case H265_NALU_TYPE_PPS:
					p_pps = p_buf;
					break;
				case H265_NALU_TYPE_VPS:
					p_vps = p_buf;
					break;
				default:
					break;
				}
				break;
			}
			p_buf += 5;
		}

		p_buf++;
	}

	if (p_sps != NULL && p_pps != NULL)
	{
		if (p_vps)
		{
			p_info->vps_size = p_sps - p_vps;
			if ((int)sizeof(p_info->vps) < p_info->vps_size) {
				printf("vps_size too large: %d \r\n", p_info->vps_size);
				return -1;
			}
			memcpy(p_info->vps, p_vps, p_info->vps_size);
		}

		p_info->sps_size = p_pps - p_sps;
		p_info->pps_size = video_info.Size - p_info->sps_size - p_info->vps_size;
		if ((int)sizeof(p_info->sps) < p_info->sps_size) {
			printf("sps_size too large: %d \r\n", p_info->sps_size);
			return -1;
		}
		memcpy(p_info->sps, p_sps, p_info->sps_size);
		if ((int)sizeof(p_info->pps) < p_info->pps_size) {
			printf("pps_size too large: %d \r\n", p_info->pps_size);
			return -1;
		}
		memcpy(p_info->pps, p_pps, p_info->pps_size);
	}
	return 0;
}

static int xIGetVideoStrm(int h_strm, int channel, int timeout_ms, NVT_STRM_INFO *p_strm)
{
	int er;
	struct timeval now;
	VSTREAM_S vstream = { 0 };
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;

	gettimeofday(&p_strm->latency.begin_wait, NULL);

	er = NvtStreamSender_GetnLockVStrm(pHdl->hDev, channel, timeout_ms, &vstream);
	if (er < 0) {
		if (er != NVTSTREAMSENDER_RET_EAGAIN) {
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
	pHdl->SeqNum = vstream.ItemSN; 
	pHdl->tm = vstream.TimeStamp;
	pHdl->frame_cnt++;

	p_strm->seq_num = vstream.ItemSN;
	p_strm->addr = vstream.Addr;
	p_strm->size = vstream.Size;
	p_strm->tm = vstream.TimeStamp;
	p_strm->svc_idx = vstream.SVCLayerId;
	gettimeofday(&p_strm->latency.wait_done, NULL);
	return 0;
}

static int xIReleaseVideoStrm(int h_strm, int channel)
{
	int er;
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;
	er = NvtStreamSender_ReleaseVStrm(pHdl->hDev, channel);
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
	AUDIO_INFO_S audio_info = { 0 };
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;
	while ((er = NvtStreamSender_GetAudioInfo(pHdl->hDev, channel, timeout_ms, &audio_info)) == NVTSTREAMSENDER_RET_EAGAIN &&
		retry--) {
		SLEEP_MS(500);
	}
	if (er < 0) {
		if (retry < 0) {
			printf("live555: xIGetAudioInfo retry time out. channel = %d\r\n", channel);
		} else if (er == NVTSTREAMSENDER_RET_ETIME) {
			//printf("live555: no audio.\r\n", channel); //show on DynamicRTSPServer.cpp
		} else {
			printf("live555: failed to xIGetAudioInfo, channel = %d, er = %d\r\n", channel, er);
		}
		return er;
	}

	switch (audio_info.CodecType) {
	case NVTSTREAMSENDER_ATYPE_PCM:
	case NVTSTREAMSENDER_ATYPE_PPCM:
		p_info->codec_type = NVT_CODEC_TYPE_PCM;
		break;
	case NVTSTREAMSENDER_ATYPE_AAC:
		p_info->codec_type = NVT_CODEC_TYPE_AAC;
		break;
	case NVTSTREAMSENDER_ATYPE_ULAW:
		p_info->codec_type = NVT_CODEC_TYPE_G711_ULAW;
		break;
	case NVTSTREAMSENDER_ATYPE_ALAW:
		p_info->codec_type = NVT_CODEC_TYPE_G711_ALAW;
		break;
	default:
		printf("live555: xIGetAudioInfo cannot parse streamsender CodecType = %d\r\n", audio_info.CodecType);
		return -1;
	}
	p_info->bit_per_sample = audio_info.BitsPerSample;
	p_info->channel_cnt = audio_info.ChannelCnt;
	p_info->sample_per_second = audio_info.SampePerSecond;
	return 0;
}

static int xIGetAudioStrm(int h_strm, int channel, int timeout_ms, NVT_STRM_INFO *p_strm)
{
	int er;
	struct timeval now;
	ASTREAM_S astream = { 0 };
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;

	gettimeofday(&p_strm->latency.begin_wait, NULL);

	er = NvtStreamSender_GetnLockAStrm(pHdl->hDev, channel, timeout_ms, &astream);
	if (er < 0) {
		if (er != NVTSTREAMSENDER_RET_EAGAIN) {
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
	pHdl->SeqNum = astream.ItemSN;
	pHdl->tm = astream.TimeStamp;
	pHdl->frame_cnt++;

	p_strm->seq_num = astream.ItemSN;
	p_strm->addr = astream.Addr;
	p_strm->size = astream.Size;
	p_strm->tm = astream.TimeStamp;
	gettimeofday(&p_strm->latency.wait_done, NULL);
	return 0;
}

static int xIReleaseAudioStrm(int h_strm, int channel)
{
	int er;
	STRM_HANDLE *pHdl = (STRM_HANDLE *)h_strm;
	er = NvtStreamSender_ReleaseAStrm(pHdl->hDev, channel);
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

ISTRM_CB *StreamSenderFrmGetStrmCb()
{
	return &m_InterfaceCb;
}