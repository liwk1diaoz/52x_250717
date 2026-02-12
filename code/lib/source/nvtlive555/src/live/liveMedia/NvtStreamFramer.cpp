#include "NvtStreamFramer.hh"
#include "GroupsockHelper.hh"
#include "NvtServerMediaSubsession.hh"
#include "nvtlive555.h"

#define CFG_EVALUATE_H264_1ST_FRAME_TIME 0 //Only Run on eCos
#define CFG_PENDING_DELAY_US 20000
#define CFG_DROP_THRESHOLD_US (-1000000)
#define CFG_DROP_THRESHOLD_MS (-2000)
#define CFG_POLLING_MODE 0
#define CFG_RECV_STACK_SIZE 4096

#define NVT_CLAMP(_NUM,_MIN,_MAX) ((_NUM>_MAX)?_MAX:(_NUM<_MIN)?_MIN:_NUM)

#if (CFG_EVALUATE_H264_1ST_FRAME_TIME)
static UINT32 m_uiFirstFrame_Begin = 0;
static UINT32 m_uiFirstFrame_GetFrame = 0;
static UINT32 m_uiFirstFrame_End = 0;

static UINT32 xGetTime32(void)
{
	struct timeval fTimeNow;
	gettimeofday(&fTimeNow, NULL);
	int64_t ret = ((int64_t)fTimeNow.tv_sec * 1000000 + (int64_t)fTimeNow.tv_usec) / 1000;
	return (UINT32)ret;
}
#endif

NvtStreamFramer::NvtStreamFramer(INIT_DATA *pInit)
	: FramedFilter(*pInit->env, NULL)
	, m_bRecvLoop(False)
	, m_bExecuted(False)
	, m_hRecv(0)
	, m_pStack(NULL)
	, m_hStrm(0)
	, m_hQueueMem(NULL)
	, m_pParentSms(pInit->pParentSms)
	, m_uMaxOutputPacketSize(0)
	, m_SessionID(pInit->SessionID)
	, m_uiMediaSrcID(pInit->uiMediaSrcID)
	, m_pCurrFrmData(NULL)
	, m_uCurrFrmSize(0)
	, m_uCurrFrmOffset(0)
	, m_bForceStop(False)
	, m_ThreadState(0)
	, m_bPauseState(False)
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
	memset(&m_QueueBuf, 0, sizeof(m_QueueBuf));
	memset(&m_RtpPacket, 0, sizeof(m_RtpPacket));
	memset(m_SessionName, 0, sizeof(m_SessionName));
	m_MediaType = pSms->GetMediaType();

#if defined(__ECOS) || defined(__LINUX680) || defined(__LINUX510)
	if (m_MediaType == NVT_MEDIA_TYPE_AUDIO) {
		//stream sender always uses channel 7 as audio
		m_uiMediaSrcID = 7;
	}
#endif
	switch (m_MediaType) {
	case NVT_MEDIA_TYPE_VIDEO:
		m_fpGetStrm = pNvtMgr->Get_StrmCb()->GetVideoStrm;
		m_fpReleaseStrm = pNvtMgr->Get_StrmCb()->ReleaseVideoStrm;
		m_StreamQueue.open(pSms->GetVideoInfo()->codec_type);
		break;
	case NVT_MEDIA_TYPE_AUDIO:
		m_fpGetStrm = pNvtMgr->Get_StrmCb()->GetAudioStrm;
		m_fpReleaseStrm = pNvtMgr->Get_StrmCb()->ReleaseAudioStrm;
		m_StreamQueue.open(pSms->GetAudioInfo()->codec_type);
		break;
	case NVT_MEDIA_TYPE_META:
		m_StreamQueue.open(NVT_CODEC_TYPE_META);
		break;
	default:
		break;
	}
}

Boolean NvtStreamFramer::Execute()
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	ISTRM_CB *pStrmCb = pNvtMgr->Get_StrmCb();

	if (m_bExecuted) {
		return True;
	} else {
		m_bExecuted = True;
	}

	if (m_MediaType == NVT_MEDIA_TYPE_AUDIO) {
		//stream sender always uses channel 7 as audio
	}
	//init live555 strm cb
	m_hStrm = pStrmCb->OpenStrm(m_uiMediaSrcID, m_MediaType, this);

	if (m_MediaType == NVT_MEDIA_TYPE_VIDEO) {
		pNvtMgr->RequireKeyFrame(m_uiMediaSrcID);
	}

#if (!CFG_POLLING_MODE)
	// create thread only if session id exists
	if (m_SessionID) {
		char media_type;
		switch (m_MediaType) {
		case NVT_MEDIA_TYPE_VIDEO:
			media_type = 'V';
			break;
		case NVT_MEDIA_TYPE_AUDIO:
			media_type = 'A';
			break;
		case NVT_MEDIA_TYPE_META:
			media_type = 'M';
			break;
		default:
			printf("live555: Execute() unhandled value: %d\r\n", (int)m_SessionID);
			return False;
		}
		sprintf(m_SessionName, "RTSP_%08X_%c", (int)m_SessionID, media_type);

		if (m_pStack == NULL) {
			m_pStack = new u_int8_t[CFG_RECV_STACK_SIZE];
		}

		m_bRecvLoop = True;
		THREAD_CREATE(m_SessionName, m_hRecv, ThreadRecv, this, m_pStack, CFG_RECV_STACK_SIZE, &m_hRecvObj);
		THREAD_RESUME(m_hRecv);
	}
#endif
	return True;
}

NvtStreamFramer::~NvtStreamFramer()
{
	if (m_hRecv) {
		m_bRecvLoop = False;
		m_StreamQueue.close();
		THREAD_JOIN(m_hRecv);
#if !defined(__FREERTOS) //rtos has no pthread_cancel()
		THREAD_DESTROY(m_hRecv);
#endif
	}
	if (m_hStrm) {
		NvtMgr_GetHandle()->Get_StrmCb()->CloseStrm(m_hStrm);
	}
	if (m_pParentSms != NULL) {
		((NvtServerMediaSubsession *)m_pParentSms)->NotifyFramerDeleted();
	}
	if (m_pStack) {
		delete[] m_pStack;
		m_pStack = NULL;
	}
}

void NvtStreamFramer::SetParentSms(void *pParentSms)
{
	m_pParentSms = pParentSms;
}
void NvtStreamFramer::SetForceStop(Boolean bForceStop)
{
	m_bForceStop = bForceStop;
}

Boolean NvtStreamFramer::IsContinueToNext()
{
#if (CFG_POLLING_MODE)
	return True;
#else
	if (m_RtpPacket.NextState == PACKET_STATE_NEW) {
		return False;
	} else {
		return True;
	}
#endif
}

Boolean NvtStreamFramer::IsPollingMode()
{
	return CFG_POLLING_MODE;
}

Boolean NvtStreamFramer::GetIsForceStop()
{
	return m_bForceStop;
}
NvtStreamQueue *NvtStreamFramer::GetStreamQueue()
{
	return &m_StreamQueue;
}

void NvtStreamFramer::SetMaxOutputPacketSize(u_int32_t uSize)
{
	m_uMaxOutputPacketSize = uSize;
}

NvtStreamFramer::RTP_PACKET *NvtStreamFramer::GetRtpPacket()
{
	return &m_RtpPacket;
}

THREAD_DECLARE(NvtStreamFramer::ThreadRecv, pStreamFramer)
{
	int er;
	Boolean bFirstFrm = True;
	NvtStreamQueue::MEM_HANDLE hQueueMem;
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	NvtStreamFramer *pFramer = (NvtStreamFramer *)pStreamFramer;
	NvtStreamQueue *pQueue = &pFramer->m_StreamQueue;
	NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)pFramer->m_pParentSms;
	NvtRTPSink *pSink = pSms->GetRTPSink();
	NvtMgr::TASK Task = { pSink->sendNew, pSink, pFramer->m_SessionID, pFramer };

	pFramer->m_ThreadState = 1;
	if (pFramer->m_fpGetStrm == NULL) {
		printf("live555: NvtStreamFramer::ThreadRecv() m_fpGetStrm is NULL.\r\n");
		pFramer->m_bRecvLoop = False;
		pFramer->m_bForceStop = True;
	}

	while (pFramer->m_bRecvLoop) {
		NVT_STRM_INFO StrmInfo = { 0 };
		StrmInfo.type = pFramer->m_MediaType;
		pFramer->m_ThreadState = 2;
		if ((hQueueMem = pQueue->lock()) == NULL) {
			if (pFramer->m_bRecvLoop) {
				// avoid quit case
				printf("live555: NvtStreamFramer::ThreadRecv() failed to lock.\r\n");
			}
			pFramer->m_bForceStop = True;
			break;
		}

		// because of live555's work queue mechanism that only allow one frame process in queue.
		// we design to lock twice for this mechanism
		pFramer->m_ThreadState = 3;
		if (bFirstFrm) {
			bFirstFrm = False;
		} else {
			pNvtMgr->PushJob(&Task);
			Task.seq_num++;
		}

		pFramer->m_ThreadState = 4;
		while ((er = pFramer->m_fpGetStrm(pFramer->m_hStrm, pFramer->m_uiMediaSrcID, 2000, &StrmInfo)) == NVT_COMMON_ER_EAGAIN &&
			pFramer->m_bRecvLoop) {
		}

		if (pFramer->m_bRecvLoop) {
			if (er == 0) {
				if (pQueue->push(hQueueMem, &StrmInfo) != 0) {
					printf("live555: NvtStreamFramer::ThreadRecv() failed to push.\r\n");
					pFramer->m_bForceStop = True;
					pFramer->m_fpReleaseStrm(pFramer->m_hStrm, pFramer->m_uiMediaSrcID);
					break;
				}
			} else {
				printf("live555: NvtStreamFramer::ThreadRecv() failed to fpGetStrm. er=%d\r\n", er);
				pFramer->m_bForceStop = True;
				// no need to release because GetStream failed.
				//pFramer->m_fpReleaseStrm(pFramer->m_hStrm, pFramer->m_uiMediaSrcID);
				break;
			}
		}
		if (er == 0) {
			pFramer->m_fpReleaseStrm(pFramer->m_hStrm, pFramer->m_uiMediaSrcID);
		}
	}

	pFramer->m_ThreadState = 5;
	pNvtMgr->CancelJob(&Task);
	pFramer->m_ThreadState = 6;
	THREAD_EXIT();
}

void NvtStreamFramer::xGetNextFrm(void)
{
	m_pCurrFrmData = NULL;
	m_uCurrFrmSize = 0;
	m_uCurrFrmOffset = 0;

	if (m_bForceStop) {
		return;
	}

#if (!CFG_POLLING_MODE)
	if ((m_hQueueMem = m_StreamQueue.pull()) != NULL) {
		if (m_StreamQueue.get_buf(m_hQueueMem, &m_QueueBuf) == 0) {
			m_pCurrFrmData = m_QueueBuf.p_buf;
			m_uCurrFrmSize = m_QueueBuf.size;
			if (m_QueueBuf.strm_info.seq_num == 0) {
				NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
				pSms->RefreshCodecInfo(m_hStrm);
			}
		}
	}
#else
	NVT_STRM_INFO StrmInfo = { 0 };
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	IGET_STRM fpGetStrm;
	switch (m_MediaType) {
	case NVT_MEDIA_TYPE_VIDEO:
		fpGetStrm = pNvtMgr->Get_StrmCb()->GetVideoStrm;
		break;
	case NVT_MEDIA_TYPE_AUDIO:
		fpGetStrm = pNvtMgr->Get_StrmCb()->GetAudioStrm;
		break;
	default:
		printf("live555: NvtStreamFramer::xGetNextFrm() not support meta data.\r\n");
		return;
	}

	if ((m_hQueueMem = m_StreamQueue.lock()) == NULL) {
		printf("live555: NvtStreamFramer::xGetNextFrm() failed to lock.\r\n");
		return;
	}

	if (fpGetStrm(m_hStrm, m_uiMediaSrcID, 5000, &StrmInfo) == 0) {
		if (m_StreamQueue.push(m_hQueueMem, &StrmInfo) == 0) {
			if ((m_hQueueMem = m_StreamQueue.pull()) != NULL) {
				if (m_StreamQueue.get_buf(m_hQueueMem, &m_QueueBuf) == 0) {
					m_pCurrFrmData = m_QueueBuf.p_buf;
					m_uCurrFrmSize = m_QueueBuf.size;
					if (m_QueueBuf.strm_info.seq_num == 0) {
						NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
						pSms->RefreshCodecInfo(m_hStrm);
					}
				}
				m_StreamQueue.unlock(m_hQueueMem);
			}
		}
	}
#endif
}

double NvtStreamFramer::Seek(u_int32_t uiMediaSrcID, double fTarget)
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	if (m_MediaType == NVT_MEDIA_TYPE_META) {
		//metadata is no supported seek.
		return fTarget;
	}

	double align_time = 0;
	NVTLIVE555_SEEK SeekCb = pNvtMgr->Get_UserCb()->seek;
	if (SeekCb != NULL) {
		SeekCb(uiMediaSrcID, fTarget, &align_time);
	}

	// it had better resume after seek to avoid got old frame
	// do not resume at Execute
	if (m_bPauseState) {
		Resume();
	}

	return align_time;
}

void NvtStreamFramer::Pause()
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();

	if (m_hRecv) {
		m_bRecvLoop = False;
		m_StreamQueue.reset(); //for quit lock in thread
		THREAD_JOIN(m_hRecv);
#if !defined(__FREERTOS) //rtos has no pthread_cancel()
		THREAD_DESTROY(m_hRecv);
#endif
		m_hRecv = 0;
	}
	//for real reset. after thread quit, it has been pushed a buffer again in thread
	m_StreamQueue.reset();
	m_RtpPacket.NextState = PACKET_STATE_NEW;

	NVTLIVE555_PAUSE PauseCb = pNvtMgr->Get_UserCb()->pause;
	if (PauseCb) {
		PauseCb(m_uiMediaSrcID);
	}

	// flush nvtstream, only for playback file, (liveview will be infinite here)
	NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
	if (pSms->GetUrlInfo()->total_ms != 0) {
		NVT_STRM_INFO StrmInfo = { 0 };
		if (m_fpGetStrm) {
			int er;
			while ((er = m_fpGetStrm(m_hStrm, m_uiMediaSrcID, 33, &StrmInfo)) == 0) {
				m_fpReleaseStrm(m_hStrm, m_uiMediaSrcID);
			}
			if (er != NVT_COMMON_ER_EAGAIN) {
				printf("live555: NvtStreamFramer::Pause(), er = %d\r\n", er);
			}
		}
	}

	m_bPauseState = True;
}

void NvtStreamFramer::Resume()
{
	m_bPauseState = False;
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();

	if (m_hRecv == 0) {
		m_bRecvLoop = True;
		THREAD_CREATE(m_SessionName, m_hRecv, ThreadRecv, this, m_pStack, CFG_RECV_STACK_SIZE, &m_hRecvObj);
		THREAD_RESUME(m_hRecv);
	}

	NVTLIVE555_RESUME ResumeCb = pNvtMgr->Get_UserCb()->resume;
	if (ResumeCb) {
		ResumeCb(m_uiMediaSrcID);
	}
}

void NvtStreamFramer::AfterSendPacket()
{
	RTP_PACKET *pRtpPacket = &m_RtpPacket;

#if (!CFG_POLLING_MODE)
	if (pRtpPacket->NextState == PACKET_STATE_NEW) {
		m_StreamQueue.unlock(m_hQueueMem);
	}
#endif

#if 0
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	if (m_MediaType == NVT_MEDIA_TYPE_VIDEO && pRtpPacket->bLastPacket) {
		pNvtMgr->SendEvent(LIVE555_SERVER_EVENT_FRAME_END);
	}
#endif
}

void NvtStreamFramer::xStopNextFrm(void)
{
	// set pause state to notify thread of staying in idle loop (EAGAIN)
	m_bPauseState = True;
	handleClosure(this);
}


void NvtStreamFramer::xDropCurrFrm(void)
{
	NvtRTPSink *pSink = ((NvtServerMediaSubsession *)m_pParentSms)->GetRTPSink();
	pSink->ResetBuffer();
	m_StreamQueue.unlock(m_hQueueMem);
	dropPacket();
}


void NvtStreamFramer::DumpInfo()
{
	const char * MapCodec[] = {
		"UNKNOWN",
		"MJPG",
		"H264",
		"H265",
		"PCM",
		"AAC",
		"ALAW",
		"ULAW",
		"META",
		"COUNT",
	};

	NVT_CODEC_TYPE CodecType;
	NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
	switch (m_MediaType) {
	case NVT_MEDIA_TYPE_VIDEO:
		CodecType = pSms->GetVideoInfo()->codec_type;
		break;
	case NVT_MEDIA_TYPE_AUDIO:
		CodecType = pSms->GetAudioInfo()->codec_type;
		break;
	case NVT_MEDIA_TYPE_META:
		CodecType = NVT_CODEC_TYPE_META;
		break;
	default:
		CodecType = NVT_CODEC_TYPE_UNKNOWN;
		break;
	}
	printf("Session:%08X, Channel:%d, Codec:%4s, State:%d, Seq:%lld, PTS:%d.%d, CurrSize:%d/%d\r\n",
		(int)m_SessionID,
		(int)m_uiMediaSrcID,
		MapCodec[CodecType],
		m_ThreadState,
		m_QueueBuf.strm_info.seq_num,
		(int)fPresentationTime.tv_sec,
		(int)fPresentationTime.tv_usec,
		(int)m_uCurrFrmOffset,
		(int)m_uCurrFrmSize);
}

NvtStreamFramerMJPG *NvtStreamFramerMJPG
::createNew(INIT_DATA *pInit)
{
	return new NvtStreamFramerMJPG(pInit);
}

NvtStreamFramerMJPG
::NvtStreamFramerMJPG(INIT_DATA *pInit)
	: NvtStreamFramer(pInit)
	, m_SpecialHdrSize(NVT_RTP_HDR_SIZE_MJPG_SUB_MAIN + NVT_RTP_HDR_SIZE_MJPG_SUB_QUANT_HDR + NVT_RTP_HDR_SIZE_MJPG_SUB_QTBL)
	, m_width(0)
	, m_height(0)
{
	memset(m_qTable, 0, sizeof(m_qTable));
}

NvtStreamFramerMJPG::~NvtStreamFramerMJPG()
{
}

void NvtStreamFramerMJPG::doGetNextFrame()
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	RTP_PACKET *pRtpPacket = &m_RtpPacket;
	PACKET_STATE CurrState = pRtpPacket->NextState;

	//In this function, fFrameSize, fNumTruncatedBytes, fPresentationTime and fDurationInMicroseconds need to update.
	if (m_uMaxOutputPacketSize == 0) {
		// shouldn't happen
		unsigned uMaxOutputPacketSize = m_uMaxOutputPacketSize;
		envir() << "NvtMJPGStreamFramer::doGetNextFrame(): m_uMaxOutputPacketSize ("
				<< uMaxOutputPacketSize << ") is 0.\n";
	} else {
		fMaxSize = m_uMaxOutputPacketSize;
	}

	pRtpPacket->bLastPacket = 0;

	switch (CurrState) {
	case PACKET_STATE_NEW:
		xGetNextFrm();
		if (m_uCurrFrmSize == 0) {
			if (m_pParentSms != NULL) {
				NvtStreamFramer *pPartner = ((NvtServerMediaSubsession *)m_pParentSms)->GetPartnerFramer();
				if (pPartner != NULL) {
					pPartner->SetForceStop(True);
				}
				m_pParentSms = NULL;
			}
			fFrameSize = 0;
			pRtpPacket->NextState = PACKET_STATE_NEW;
			xStopNextFrm();//  handleClosure(this);
			return;
		} else if(m_bPauseState) {
			xDropCurrFrm();
			return;
		} else {
			if (1) { //MJPG is always I-Frame
				NVTLIVE555_STATISTIC statistic = {0};
				NVTLIVE555_STRATEGY strategy = {0};

				statistic.channel_id = m_uiMediaSrcID;
				statistic.is_svc_support = 0;
				statistic.client_ip_addr = ((NvtServerMediaSubsession *)m_pParentSms)->GetRTPSink()->GetDstAddress(); //wifiteam

				NVTLIVE555_BRC GetBrcCb = pNvtMgr->Get_UserCb()->get_brc;
				if (GetBrcCb != NULL) {
					GetBrcCb(&statistic, &strategy);
				}
			}

			u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];

			if (!(pSrc[0] == 0xFF && pSrc[1] == 0xD8)) {
				printf("live555: wrong jpeg header.\r\n");
				fFrameSize = 0;
				pRtpPacket->NextState = PACKET_STATE_NEW;
				xStopNextFrm();//  handleClosure(this);
				return;
			}

			bool headerOk = false;
			unsigned int i;
			for (i = 2; i < m_uCurrFrmSize ; ++i) {
				if (pSrc[i] != 0xFF) {
					printf("live555: wrong jpeg header-2.\r\n");
					fFrameSize = 0;
					pRtpPacket->NextState = PACKET_STATE_NEW;
					xStopNextFrm();//  handleClosure(this);
					return;
				}

				unsigned int tag_name = (unsigned int)pSrc[i + 1] | (((unsigned int)pSrc[i]) << 8);
				unsigned int tag_size = (unsigned int)pSrc[i + 3] | (((unsigned int)pSrc[i + 2]) << 8);
				// SOF
				if (tag_name == 0xFFC0) {
					m_height = (pSrc[i + 5] << 5) | (pSrc[i + 6] >> 3);
					m_width = (pSrc[i + 7] << 5) | (pSrc[i + 8] >> 3);
				}
				// DQT
				if (tag_name == 0xFFDB) {
					if (pSrc[i + 4] == 0) {
						memcpy(m_qTable, pSrc + i + 5, 64);
					} else if (pSrc[i + 4] == 1) {
						memcpy(m_qTable + 64, pSrc + i + 5, 64);
					}
				}
				// End of header
				if (tag_name == 0xFFDA) {
					i += tag_size + 2;
					headerOk = true;
					break;
				}

				i += tag_size + 1; //+1 but rather +2 because of ++i
			}

			//m_uCurrFrmOffset = i;
			//pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];

			if (!headerOk) {
				fFrameSize = 0;
				pRtpPacket->NextState = PACKET_STATE_NEW;
				xStopNextFrm();//  handleClosure(this);
				return;
			}

			if (m_uCurrFrmSize - i < fMaxSize) {
				fFrameSize = m_uCurrFrmSize - i;
				//memcpy(fTo,pSrc,fFrameSize);
				*pRtpPacket->ppReplace = pSrc;
				m_uCurrFrmOffset += fFrameSize;
				pRtpPacket->NextState = PACKET_STATE_NEW;
				pRtpPacket->bLastPacket = 1;
			} else {
				fFrameSize = fMaxSize;
				//memcpy(fTo,pSrc,fFrameSize);
				*pRtpPacket->ppReplace = pSrc;
				m_uCurrFrmOffset += fFrameSize;
				pRtpPacket->NextState = PACKET_STATE_CONT;
			}
		}
		break;
	case PACKET_STATE_CONT: {
			u_int32_t remain_byte = m_uCurrFrmSize - m_uCurrFrmOffset;
			u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];

			fFrameSize = (remain_byte < fMaxSize) ? remain_byte : fMaxSize;
			//memcpy(fTo,pSrc,fFrameSize);
			*pRtpPacket->ppReplace = pSrc;
			m_uCurrFrmOffset += fFrameSize;

			if (remain_byte <= fMaxSize) { //last fragment?
				pRtpPacket->NextState = PACKET_STATE_NEW;
				pRtpPacket->bLastPacket = 1;
			} else {
				pRtpPacket->NextState = PACKET_STATE_CONT;
			}
		}
		break;
	default:
		printf("live555: doGetNextFrame() unhandled value: %d\r\n", pRtpPacket->NextState);
		xStopNextFrm();
		return;
	}

	fPresentationTime = m_QueueBuf.pts;

#if (CFG_POLLING_MODE)
	if (pRtpPacket->bLastPacket) {
		u_int32_t frame_time_us = 1000000 / CFG_RTSPD_FAKE_FPS;
		int64_t uSecondsToGo = pNvtMgr->Get_NextPtsTimeDiff(&fPresentationTime, frame_time_us);
		fDurationInMicroseconds = (uSecondsToGo < 0) ? 0 : (unsigned)uSecondsToGo;
	} else {
		fDurationInMicroseconds = 0;
	}
#endif

	nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
				 (TaskFunc *)FramedSource::afterGetting, this);

}

void NvtStreamFramerMJPG::AfterSendPacket()
{
	if (m_uCurrFrmOffset == 0) {
		m_SpecialHdrSize = (NVT_RTP_HDR_SIZE_MJPG_SUB_MAIN + NVT_RTP_HDR_SIZE_MJPG_SUB_QUANT_HDR + NVT_RTP_HDR_SIZE_MJPG_SUB_QTBL);
	} else {
		m_SpecialHdrSize = NVT_RTP_HDR_SIZE_MJPG_SUB_MAIN;
	}

	NvtStreamFramer::AfterSendPacket();
}

u_int8_t NvtStreamFramerMJPG::type()
{
	return 1;
}

u_int8_t NvtStreamFramerMJPG::qFactor()
{
	return 128;
}

u_int8_t NvtStreamFramerMJPG::width()
{
	return m_width;
}

u_int8_t NvtStreamFramerMJPG::height()
{
	return m_height;
}

u_int8_t const *NvtStreamFramerMJPG::quantizationTables(u_int8_t &precision, u_int16_t &length)
{
	precision = 8;
	length = sizeof(m_qTable);
	return m_qTable;
}

u_int32_t NvtStreamFramerMJPG::GetSpecialHdrSize()
{
	return m_SpecialHdrSize;
}

NvtStreamFramerH264 *NvtStreamFramerH264
::createNew(INIT_DATA *pInit)
{
	return new NvtStreamFramerH264(pInit);
}

NvtStreamFramerH264
::NvtStreamFramerH264(INIT_DATA *pInit)
	: NvtStreamFramer(pInit)
	, m_uBufferOffset(4) //remove 00,00,00,01
	, m_uLastFU_Indicator(0)
	, m_uLastFU_Header(0)
	, m_bResetToKeyFrm(True)
	, m_bSvcSupport(False)
	, m_bSkipTillNextSvc0(False)
	, m_SvcSkipLevel(SVC_METHOD_SKIP_LEVEL_0)
	, m_SvcSkipLevelByUser(SVC_METHOD_SKIP_LEVEL_0)
{
	memset(m_skip_svc_frm, 0, sizeof(m_skip_svc_frm)); //NVT_MODIFIED, WIFI-TEAM
	memset(m_skip_svc_byte, 0, sizeof(m_skip_svc_byte)); //NVT_MODIFIED, WIFI-TEAM
	m_svc_socket_map[0] = 0;
	m_svc_socket_map[1] = 0;
	m_svc_socket_map[2] = 1;

	NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
	int SvcLevel = pSms->GetUrlInfo()->svc_level;
	if (SvcLevel >= SVC_METHOD_SKIP_LEVEL_0 && SvcLevel < SVC_METHOD_SKIP_LEVEL_COUNTS) {
		m_SvcSkipLevelByUser = (NvtStreamFramerH264::SVC_METHOD_SKIP_LEVEL)SvcLevel;
	}
}

NvtStreamFramerH264::~NvtStreamFramerH264()
{
}

u_int8_t *NvtStreamFramerH264::xAdjustDataOfs(u_int8_t *pData, u_int32_t *p_size)
{
	if (*p_size >= m_uBufferOffset) {
		pData += m_uBufferOffset;
		*p_size -= m_uBufferOffset;
	}
	return pData;
}

void NvtStreamFramerH264::doGetNextFrame()
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	//NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
	RTP_PACKET *pRtpPacket = &m_RtpPacket;
	PACKET_STATE CurrState = pRtpPacket->NextState;

	//In this function, fFrameSize, fNumTruncatedBytes, fPresentationTime and fDurationInMicroseconds need to update.
	if (m_uMaxOutputPacketSize == 0) {
		// shouldn't happen
		unsigned uMaxOutputPacketSize = m_uMaxOutputPacketSize;
		envir() << "NvtH264StreamFramer::doGetNextFrame(): m_uMaxOutputPacketSize ("
				<< uMaxOutputPacketSize << ") is 0.\n";
	} else {
		fMaxSize = m_uMaxOutputPacketSize;
	}

	pRtpPacket->bLastPacket = 0;

	if (*pRtpPacket->pByteSent == -1
		&& !m_bResetToKeyFrm) {
		printf("live555: socket queue full, skip until next I.\r\n");
		m_bResetToKeyFrm = True;
		pNvtMgr->RequireKeyFrame(m_uiMediaSrcID);
	}

	switch (CurrState) {
	case PACKET_STATE_NEW_OR_SPS:
#if (CFG_EVALUATE_H264_1ST_FRAME_TIME)
		m_uiFirstFrame_Begin = xGetTime32();
#endif
		xGetNextFrm();
		m_pCurrFrmData = xAdjustDataOfs(m_pCurrFrmData, &m_uCurrFrmSize);

		if (m_QueueBuf.strm_info.seq_num == 0) {
			// wait I-Frame
			m_bResetToKeyFrm = True;
		}

#if (CFG_EVALUATE_H264_1ST_FRAME_TIME)
		m_uiFirstFrame_GetFrame = xGetTime32();
#endif
		if (m_uCurrFrmSize == 0 || m_pCurrFrmData == NULL) {
			if (m_pParentSms != NULL) {
				NvtStreamFramer *pPartner = ((NvtServerMediaSubsession *)m_pParentSms)->GetPartnerFramer();
				if (pPartner != NULL) {
					pPartner->SetForceStop(True);
				}
				m_pParentSms = NULL;
			}
			fFrameSize = 0;
			pRtpPacket->NextState = PACKET_STATE_NEW_OR_SPS;
			xStopNextFrm();//  handleClosure(this);
			return;
		} else if (m_bPauseState) {
			xDropCurrFrm();
			return;
		} else {
			if (m_pCurrFrmData[0] == NALU_TYPE_IDR) {
				NVTLIVE555_STATISTIC statistic = {0};
				NVTLIVE555_STRATEGY strategy = {0};

				statistic.channel_id = m_uiMediaSrcID;
				statistic.is_svc_support = m_bSvcSupport;
				statistic.client_ip_addr = ((NvtServerMediaSubsession *)m_pParentSms)->GetRTPSink()->GetDstAddress(); //wifiteam
				memcpy(statistic.skip_svc_frm,  m_skip_svc_frm, sizeof(m_skip_svc_frm)); //NVT_MODIFIED, WIFI-TEAM
				memcpy(statistic.skip_svc_byte, m_skip_svc_byte, sizeof(m_skip_svc_byte)); //NVT_MODIFIED, WIFI-TEAM

				NVTLIVE555_BRC GetBrcCb = pNvtMgr->Get_UserCb()->get_brc;
				if (GetBrcCb != NULL) {
					GetBrcCb(&statistic, &strategy);
				}

				memset(m_skip_svc_frm, 0, sizeof(m_skip_svc_frm)); //NVT_MODIFIED, WIFI-TEAM
				memset(m_skip_svc_byte, 0, sizeof(m_skip_svc_byte)); //NVT_MODIFIED, WIFI-TEAM

				if (strategy.en_mask & 0x1) {
					m_SvcSkipLevel = (SVC_METHOD_SKIP_LEVEL)strategy.svc_level;
					m_SvcSkipLevel = (m_SvcSkipLevelByUser > m_SvcSkipLevel) ? m_SvcSkipLevelByUser : m_SvcSkipLevel;
				}
				if (strategy.en_mask & 0x2) {
					int i;
					for (i = 0; i < 3 ; i++) {
						if (strategy.svc_sock_priority[i] < 0 || strategy.svc_sock_priority[i] > 1) {
							printf("live555: wrong svc_sock_priority. get value:%d\r\n", strategy.svc_sock_priority[i]);
							break;
						}
					}
					if (i == 3) {
						for (i = 0; i < 3 ; i++) {
							m_svc_socket_map[i] = strategy.svc_sock_priority[i];
						}
						//printf("%d %d %d\r\n",g_svc_socket_map[0],g_svc_socket_map[1],g_svc_socket_map[2]);
					}
				}
				//printf("(%d,%d)=>%d\r\n",strategy.svc_level,m_SvcSkipLevelByUser,m_SvcSkipLevel);
			}

			if (m_bResetToKeyFrm) {
				if (m_pCurrFrmData[0] == NALU_TYPE_IDR) {
					//Send SPS
					NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
					NVT_VIDEO_INFO *pInfo = pSms->GetVideoInfo();
					u_int8_t *sps = pInfo->sps; u_int32_t sps_size = (u_int32_t)pInfo->sps_size;
					sps = xAdjustDataOfs(sps, &sps_size);
					fFrameSize = sps_size;
					*pRtpPacket->ppReplace = NULL;
					memcpy(fTo, sps, fFrameSize);

					//reset time
					fPresentationTime.tv_sec = 0;
					fPresentationTime.tv_usec = 0;

					pRtpPacket->NextState = PACKET_STATE_PPS;
					m_bResetToKeyFrm = False;
				} else {
					m_skip_svc_frm[m_QueueBuf.strm_info.svc_idx]++;
					m_skip_svc_byte[m_QueueBuf.strm_info.svc_idx] += m_uCurrFrmSize;
					xDropCurrFrm();
					return;
				}
			} else if (m_bSvcSupport &&
					   (m_QueueBuf.strm_info.svc_idx > (UINT32)((SVC_METHOD_SKIP_LEVEL_COUNTS - 1) - m_SvcSkipLevel)
						|| (m_SvcSkipLevel == 3 && m_pCurrFrmData[0] != NALU_TYPE_IDR)) //level=3 means drop non-I-Frame
					  ) {
				xDropCurrFrm();
				return;
			} else {
				//Detect SVC Support
				if (m_QueueBuf.strm_info.svc_idx != 0) {
					m_bSvcSupport = True;
				}

				if (m_bSvcSupport && m_bSkipTillNextSvc0) { //NVT_MODIFIED, WIFI-TEAM
					if (m_QueueBuf.strm_info.svc_idx != 0) {
						//skip this frame
						printf("live555: socket near to full, skip this non-svc-0 frame.\r\n");
						xDropCurrFrm();
						return;
					} else {
						m_bSkipTillNextSvc0 = False;
					}
				}

				//NVT_MODIFIED, Multi-UDP Source, SVC Multi-Queue Slot
				if (m_SvcSkipLevel > 2) {
					*pRtpPacket->piSocketIdx = 0;
				} else {
					*pRtpPacket->piSocketIdx = m_svc_socket_map[m_QueueBuf.strm_info.svc_idx];
				}

				if (m_pCurrFrmData[0] == NALU_TYPE_IDR) {
					//Send SPS
					NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
					NVT_VIDEO_INFO *pInfo = pSms->GetVideoInfo();
					u_int8_t *sps = pInfo->sps;
					u_int32_t sps_size = (u_int32_t)pInfo->sps_size;
					sps = xAdjustDataOfs(sps, &sps_size);
					fFrameSize = sps_size;
					*pRtpPacket->ppReplace = NULL;
					memcpy(fTo, sps, fFrameSize);
					pRtpPacket->NextState = PACKET_STATE_PPS;
				} else if (m_uCurrFrmSize < fMaxSize) {
					//Send P-Frame
					u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
					fFrameSize = m_uCurrFrmSize;
					//memcpy(fTo,pSrc,fFrameSize);
					*pRtpPacket->ppReplace = pSrc;
					m_uCurrFrmOffset += fFrameSize;
					pRtpPacket->NextState = PACKET_STATE_NEW_OR_SPS;
					pRtpPacket->bLastPacket = 1;
				} else {
					u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
					fFrameSize = fMaxSize;
					//memcpy(fTo+1,pSrc,fFrameSize-1);
					*pRtpPacket->ppReplace = pSrc - 1;
					m_uCurrFrmOffset += fFrameSize - 1;

					m_uLastFU_Indicator = (m_pCurrFrmData[0] & 0xE0) | 28; // FU indicator, remove nal_unit_type, use FU-A(0x28) instead, keep forbidden_zero_bit,nal_ref_idc (ref:H264 spec)
					m_uLastFU_Header = 0x80 | (m_pCurrFrmData[0] & 0x1F);  // FU header (with S bit) [S E R nal_unit_type]

					//fTo[0] = m_uLastFU_Indicator; // FU indicator
					//fTo[1] = m_uLastFU_Header; // FU header (with S bit)

					pSrc[-1] = m_uLastFU_Indicator;
					pSrc[0] = m_uLastFU_Header;

					pRtpPacket->NextState = PACKET_STATE_CONT;
				}
			}
		}
		break;
	case PACKET_STATE_PPS: {
			//Send PPS
			NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
			NVT_VIDEO_INFO *pInfo = pSms->GetVideoInfo();
			u_int8_t *pps = pInfo->pps;
			u_int32_t pps_size = (u_int32_t)pInfo->pps_size;
			pps = xAdjustDataOfs(pps, &pps_size);
			fFrameSize = pps_size;
			*pRtpPacket->ppReplace = NULL;
			memcpy(fTo, pps, fFrameSize);
			pRtpPacket->NextState = PACKET_STATE_CONT;
		}
		break;
	case PACKET_STATE_CONT: {
			if (m_uCurrFrmOffset == 0 && m_uCurrFrmSize < fMaxSize) { // I-Frame,that followed by SPS,PPS, and Size is smaller than one RTP size.
				u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
				fFrameSize = m_uCurrFrmSize;
				//memcpy(fTo,pSrc,fFrameSize);
				*pRtpPacket->ppReplace = pSrc;
				m_uCurrFrmOffset += fFrameSize;
				pRtpPacket->NextState = PACKET_STATE_NEW_OR_SPS;
				pRtpPacket->bLastPacket = 1;
			} else {
				// We are sending this NAL unit data as FU-A packets.  We've already sent the
				// first packet (fragment).  Now, send the next fragment.  Note that we add
				// FU indicator and FU header bytes to the front.  (We reuse these bytes that
				// we already sent for the first fragment, but clear the S bit, and add the E
				// bit if this is the last fragment.)
				u_int32_t remain_byte = m_uCurrFrmSize - m_uCurrFrmOffset;
				u_int32_t uOverHead = (m_uCurrFrmOffset == 0) ? 1 : 2;
				u_int32_t numBytesToSend = uOverHead + m_uCurrFrmSize - m_uCurrFrmOffset;
				u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];

				if (m_uCurrFrmOffset == 0) { // I-Frame,that followed by SPS,PPS
					m_uLastFU_Indicator = (m_pCurrFrmData[0] & 0xE0) | 28; // FU indicator, remove nal_unit_type, use FU-A(0x28) instead, keep forbidden_zero_bit,nal_ref_idc (ref:H264 spec)
					m_uLastFU_Header = 0x80 | (m_pCurrFrmData[0] & 0x1F);  // FU header (with S bit) [S E R nal_unit_type]
					pSrc[-1] = m_uLastFU_Indicator;
					pSrc[0] = m_uLastFU_Header;
				} else {
					//fTo[0] = m_uLastFU_Indicator;
					//fTo[1] = m_uLastFU_Header&~0x80; // FU header (no S bit)

					pSrc[-2] = m_uLastFU_Indicator;
					pSrc[-1] = m_uLastFU_Header & ~0x80; // FU header (no S bit)
				}

				fFrameSize = (numBytesToSend < fMaxSize) ? numBytesToSend : fMaxSize;
				//memcpy(fTo+uOverHead,pSrc,fFrameSize-uOverHead);
				*pRtpPacket->ppReplace = pSrc - uOverHead;
				m_uCurrFrmOffset += fFrameSize - uOverHead;


				if ((remain_byte + uOverHead) <= fMaxSize) { //last fragment?
					//fTo[1] |= 0x40; // set the E bit in the FU header
					pSrc[-1] |= 0x40; // set the E bit in the FU header
					pRtpPacket->NextState = PACKET_STATE_NEW_OR_SPS;
					pRtpPacket->bLastPacket = 1;
				} else {
					pRtpPacket->NextState = PACKET_STATE_CONT;
				}
			}
		}
		break;
	default:
		printf("live555: doGetNextFrame() unhandled value: %d\r\n", pRtpPacket->NextState);
		xStopNextFrm();
		return;
	}

	fPresentationTime = m_QueueBuf.pts;

#if (CFG_POLLING_MODE)
	u_int32_t uiFps = CFG_RTSPD_FAKE_FPS;
	u_int32_t frame_time_us = 1000000 / uiFps;
	if (pRtpPacket->bLastPacket) {
		int64_t uSecondsToGo = pNvtMgr->Get_NextPtsTimeDiff(&fPresentationTime, frame_time_us);
		fDurationInMicroseconds = (uSecondsToGo < 0) ? 0 : (unsigned)uSecondsToGo;
	} else {
		fDurationInMicroseconds = 0;
	}
#endif
	nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
				 (TaskFunc *)FramedSource::afterGetting, this);
}

void NvtStreamFramerH264::AfterSendPacket()
{
#if (CFG_EVALUATE_H264_1ST_FRAME_TIME)
	RTP_PACKET *pRtpPacket = &m_RtpPacket;
	if (pRtpPacket->bLastPacket) {
		m_uiFirstFrame_End = xGetTime32();;
		printf("%d, %d, %d\r\n", m_uiFirstFrame_GetFrame - m_uiFirstFrame_Begin, m_uiFirstFrame_End - m_uiFirstFrame_Begin, m_uCurrFrmSize);
	}
#endif
	NvtStreamFramer::AfterSendPacket();
}

NvtStreamFramerPCM *NvtStreamFramerPCM
::createNew(INIT_DATA *pInit)
{
	return new NvtStreamFramerPCM(pInit);
}

NvtStreamFramerPCM
::NvtStreamFramerPCM(INIT_DATA *pInit)
	: NvtStreamFramer(pInit)
{
}

NvtStreamFramerPCM::~NvtStreamFramerPCM()
{
}

void NvtStreamFramerPCM::xSwapCopy(u_int8_t *pDst, u_int8_t *pSrc, u_int32_t uiSize)
{
	while (uiSize) {
		*pDst = *(pSrc + 1);
		*(pDst + 1) = *(pSrc);
		uiSize -= 2;
		pDst += 2;
		pSrc += 2;
	}
}

void NvtStreamFramerPCM::doGetNextFrame()
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	RTP_PACKET *pRtpPacket = &m_RtpPacket;
	PACKET_STATE CurrState = pRtpPacket->NextState;

	//In this function, fFrameSize, fNumTruncatedBytes, fPresentationTime and fDurationInMicroseconds need to update.
	if (m_uMaxOutputPacketSize == 0) {
		// shouldn't happen
		unsigned uMaxOutputPacketSize = m_uMaxOutputPacketSize;
		envir() << "NvtMJPGStreamFramer::doGetNextFrame(): m_uMaxOutputPacketSize ("
				<< uMaxOutputPacketSize << ") is 0.\n";
	} else {
		fMaxSize = m_uMaxOutputPacketSize;
	}

	switch (pRtpPacket->NextState) {
	case PACKET_STATE_NEW:
		xGetNextFrm();
		if (m_uCurrFrmSize == 0 || (m_uCurrFrmSize & 0x1)) {
			if (m_uCurrFrmSize & 0x1) {
				printf("live555-pcm, getting odd size=%d is invalid.\r\n", (int)m_uCurrFrmSize);
			}
			if (m_pParentSms != NULL) {
				NvtStreamFramer *pPartner = ((NvtServerMediaSubsession *)m_pParentSms)->GetPartnerFramer();
				if (pPartner != NULL) {
					pPartner->SetForceStop(True);
				}
				m_pParentSms = NULL;
			}
			fFrameSize = 0;
			pRtpPacket->NextState = PACKET_STATE_NEW;
			xStopNextFrm();//  handleClosure(this);
			return;
		} else if (m_bPauseState) {
			xDropCurrFrm();
			return;
		} else {
			if (m_uCurrFrmSize < fMaxSize) {
				u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
				fFrameSize = m_uCurrFrmSize;
				xSwapCopy(fTo, pSrc, fFrameSize);
				m_uCurrFrmOffset += fFrameSize;
				pRtpPacket->NextState = PACKET_STATE_NEW;
			} else {
				u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
				fFrameSize = fMaxSize;
				xSwapCopy(fTo, pSrc, fFrameSize);
				m_uCurrFrmOffset += fFrameSize;
				pRtpPacket->NextState = PACKET_STATE_CONT;
			}
		}
		break;
	case PACKET_STATE_CONT: {
			u_int32_t remain_byte = m_uCurrFrmSize - m_uCurrFrmOffset;

			u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
			fFrameSize = (remain_byte < fMaxSize) ? remain_byte : fMaxSize;
			xSwapCopy(fTo, pSrc, fFrameSize);
			m_uCurrFrmOffset += fFrameSize;

			if (remain_byte <= fMaxSize) { //last fragment?
				pRtpPacket->NextState = PACKET_STATE_NEW;
			} else {
				pRtpPacket->NextState = PACKET_STATE_CONT;
			}
		}
		break;
	default:
		printf("live555: doGetNextFrame() unhandled value: %d\r\n", pRtpPacket->NextState);
		xStopNextFrm();
		return;
	}

	//PCM's frame_time_us cannot move to in the front, because it use fFrameSize
	NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
	NVT_AUDIO_INFO *pInfo = pSms->GetAudioInfo();
	u_int32_t frame_time_us = (u_int32_t)((u_int64_t)fFrameSize * 1000000 / (pInfo->sample_per_second
										  * pInfo->channel_cnt * pInfo->bit_per_sample / 8));

	if (CurrState == NvtStreamFramer::PACKET_STATE_NEW) {
		fPresentationTime = m_QueueBuf.pts;
	} else {
		unsigned uSeconds = fPresentationTime.tv_usec + frame_time_us;
		fPresentationTime.tv_sec += uSeconds / 1000000;
		fPresentationTime.tv_usec = uSeconds % 1000000;
	}

	int64_t uSecondsToGo = pNvtMgr->Get_NextPtsTimeDiff(&fPresentationTime, frame_time_us);
	//fDurationInMicroseconds = frame_time_us;
	fDurationInMicroseconds = (uSecondsToGo < 0) ? 0 : (unsigned)uSecondsToGo;

	nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
				 (TaskFunc *)FramedSource::afterGetting, this);

}

NvtStreamFramerAAC *NvtStreamFramerAAC
::createNew(INIT_DATA *pInit)
{
	return new NvtStreamFramerAAC(pInit);
}

NvtStreamFramerAAC
::NvtStreamFramerAAC(INIT_DATA *pInit)
	: NvtStreamFramer(pInit)
{
}

NvtStreamFramerAAC::~NvtStreamFramerAAC()
{
}

void NvtStreamFramerAAC::doGetNextFrame()
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	NvtServerMediaSubsession *pSms = ((NvtServerMediaSubsession *)m_pParentSms);
	NVT_AUDIO_INFO *pInfo = pSms->GetAudioInfo();
	RTP_PACKET *pRtpPacket = &m_RtpPacket;
	// don't multiply by pNvAudio->GetChannels(m_uiMediaSrcID), because aac's GetSamplePerSecond has included it. [NA51000-236]
	u_int32_t frame_time_us = (u_int32_t)(1024 * 1000000 / pInfo->sample_per_second);

	//In this function, fFrameSize, fNumTruncatedBytes, fPresentationTime and fDurationInMicroseconds need to update.
	if (m_uMaxOutputPacketSize == 0) {
		// shouldn't happen
		unsigned uMaxOutputPacketSize = m_uMaxOutputPacketSize;
		envir() << "NvtMJPGStreamFramer::doGetNextFrame(): m_uMaxOutputPacketSize ("
				<< uMaxOutputPacketSize << ") is 0.\n";
	} else {
		fMaxSize = m_uMaxOutputPacketSize;
	}

	pRtpPacket->bLastPacket = 0;

	switch (pRtpPacket->NextState) {
	case PACKET_STATE_NEW:
		xGetNextFrm();
		if (m_uCurrFrmSize == 0) {
			if (m_pParentSms != NULL) {
				NvtStreamFramer *pPartner = ((NvtServerMediaSubsession *)m_pParentSms)->GetPartnerFramer();
				if (pPartner != NULL) {
					pPartner->SetForceStop(True);
				}
				m_pParentSms = NULL;
			}
			fFrameSize = 0;
			pRtpPacket->NextState = PACKET_STATE_NEW;
			xStopNextFrm();//  handleClosure(this);
			return;
		} else if (m_bPauseState) {
			xDropCurrFrm();
			return;
		} else {
			if (m_uCurrFrmSize < fMaxSize) {
				u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
				fFrameSize = m_uCurrFrmSize;
				memcpy(fTo, pSrc, fFrameSize);
				m_uCurrFrmOffset += fFrameSize;
				pRtpPacket->NextState = PACKET_STATE_NEW;
				pRtpPacket->bLastPacket = 1;
			} else {
				u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
				fFrameSize = fMaxSize;
				memcpy(fTo, pSrc, fFrameSize);
				m_uCurrFrmOffset += fFrameSize;
				pRtpPacket->NextState = PACKET_STATE_CONT;
			}
		}
		break;
	case PACKET_STATE_CONT: {
			u_int32_t remain_byte = m_uCurrFrmSize - m_uCurrFrmOffset;

			u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
			fFrameSize = (remain_byte < fMaxSize) ? remain_byte : fMaxSize;
			memcpy(fTo, pSrc, fFrameSize);
			m_uCurrFrmOffset += fFrameSize;

			if (remain_byte <= fMaxSize) { //last fragment?
				pRtpPacket->NextState = PACKET_STATE_NEW;
				pRtpPacket->bLastPacket = 1;
			} else {
				pRtpPacket->NextState = PACKET_STATE_CONT;
			}
		}
		break;
	default:
		printf("live555: doGetNextFrame() unhandled value: %d\r\n", pRtpPacket->NextState);
		xStopNextFrm();
		return;
	}

	fPresentationTime = m_QueueBuf.pts;

	if (pRtpPacket->NextState == PACKET_STATE_NEW) {
		int64_t uSecondsToGo = pNvtMgr->Get_NextPtsTimeDiff(&fPresentationTime, frame_time_us);
		//fDurationInMicroseconds = frame_time_us;
		fDurationInMicroseconds = (uSecondsToGo < 0) ? 0 : (unsigned)uSecondsToGo;
	} else {
		fDurationInMicroseconds = 0;
	}

	nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
				 (TaskFunc *)FramedSource::afterGetting, this);
}


NvtStreamFramerG711 *NvtStreamFramerG711
::createNew(INIT_DATA *pInit)
{
	return new NvtStreamFramerG711(pInit);
}

NvtStreamFramerG711
::NvtStreamFramerG711(INIT_DATA *pInit)
	: NvtStreamFramer(pInit)
{
}

NvtStreamFramerG711::~NvtStreamFramerG711()
{
}

void NvtStreamFramerG711::doGetNextFrame()
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	RTP_PACKET *pRtpPacket = &m_RtpPacket;
	PACKET_STATE CurrState = pRtpPacket->NextState;

	//In this function, fFrameSize, fNumTruncatedBytes, fPresentationTime and fDurationInMicroseconds need to update.
	if (m_uMaxOutputPacketSize == 0) {
		// shouldn't happen
		unsigned uMaxOutputPacketSize = m_uMaxOutputPacketSize;
		envir() << "NvtMJPGStreamFramer::doGetNextFrame(): m_uMaxOutputPacketSize ("
				<< uMaxOutputPacketSize << ") is 0.\n";
	} else {
		fMaxSize = m_uMaxOutputPacketSize;
	}

	switch (pRtpPacket->NextState) {
	case PACKET_STATE_NEW:
		xGetNextFrm();
		if (m_uCurrFrmSize == 0) {
			if (m_pParentSms != NULL) {
				NvtStreamFramer *pPartner = ((NvtServerMediaSubsession *)m_pParentSms)->GetPartnerFramer();
				if (pPartner != NULL) {
					pPartner->SetForceStop(True);
				}
				m_pParentSms = NULL;
			}
			fFrameSize = 0;
			pRtpPacket->NextState = PACKET_STATE_NEW;
			xStopNextFrm();//  handleClosure(this);
			return;
		} else if (m_bPauseState) {
			xDropCurrFrm();
			return;
		}

		if (m_uCurrFrmSize < fMaxSize) {
			u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
			fFrameSize = m_uCurrFrmSize;
			memcpy(fTo, pSrc, fFrameSize);
			m_uCurrFrmOffset += fFrameSize;
			pRtpPacket->NextState = PACKET_STATE_NEW;
		} else {
			u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
			fFrameSize = fMaxSize;
			memcpy(fTo, pSrc, fFrameSize);
			m_uCurrFrmOffset += fFrameSize;
			pRtpPacket->NextState = PACKET_STATE_CONT;
		}
		break;
	case PACKET_STATE_CONT: {
			u_int32_t remain_byte = m_uCurrFrmSize - m_uCurrFrmOffset;

			u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
			fFrameSize = (remain_byte < fMaxSize) ? remain_byte : fMaxSize;
			memcpy(fTo, pSrc, fFrameSize);
			m_uCurrFrmOffset += fFrameSize;

			if (remain_byte <= fMaxSize) { //last fragment?
				pRtpPacket->NextState = PACKET_STATE_NEW;
			} else {
				pRtpPacket->NextState = PACKET_STATE_CONT;
			}
		}
		break;
	default:
		printf("live555: doGetNextFrame() unhandled value: %d\r\n", pRtpPacket->NextState);
		xStopNextFrm();
		return;
	}

	//G711's frame_time_us cannot move to in the front, because it use fFrameSize
	NvtServerMediaSubsession *pSms = ((NvtServerMediaSubsession *)m_pParentSms);
	NVT_AUDIO_INFO *pInfo = pSms->GetAudioInfo();
	u_int32_t frame_time_us = (u_int32_t)((u_int64_t)fFrameSize * 1000000 / (pInfo->sample_per_second * pInfo->channel_cnt));

	if (CurrState == NvtStreamFramer::PACKET_STATE_NEW) {
		fPresentationTime = m_QueueBuf.pts;
	} else {
		// Increment by the play time of the previous data:
		unsigned uSeconds   = fPresentationTime.tv_usec + frame_time_us;
		fPresentationTime.tv_sec += uSeconds / 1000000;
		fPresentationTime.tv_usec = uSeconds % 1000000;
	}

	int64_t uSecondsToGo = pNvtMgr->Get_NextPtsTimeDiff(&fPresentationTime, frame_time_us);
	//fDurationInMicroseconds = frame_time_us;
	fDurationInMicroseconds = (uSecondsToGo < 0) ? 0 : (unsigned)uSecondsToGo;

	nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
				 (TaskFunc *)FramedSource::afterGetting, this);
}

//-----------------------------------

NvtStreamFramerMeta *NvtStreamFramerMeta
::createNew(INIT_DATA *pInit)
{
	return new NvtStreamFramerMeta(pInit);
}

NvtStreamFramerMeta
::NvtStreamFramerMeta(INIT_DATA *pInit)
	: NvtStreamFramer(pInit)
	, m_pMetaText(NULL)
	, m_uMetaTextSize(0)
	, m_bFirstFrm(1)
{
	m_uMetaTextSize = (3 * 1024); //max size
	m_pMetaText = new char[m_uMetaTextSize];
}

NvtStreamFramerMeta::~NvtStreamFramerMeta()
{
	if (m_pMetaText) {
		delete m_pMetaText;
	}
}

void NvtStreamFramerMeta::doGetNextFrame()
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	RTP_PACKET *pRtpPacket = &m_RtpPacket;

	//In this function, fFrameSize, fNumTruncatedBytes, fPresentationTime and fDurationInMicroseconds need to update.
	if (m_uMaxOutputPacketSize == 0) {
		// shouldn't happen
		unsigned uMaxOutputPacketSize = m_uMaxOutputPacketSize;
		envir() << "NvtMJPGStreamFramer::doGetNextFrame(): m_uMaxOutputPacketSize ("
				<< uMaxOutputPacketSize << ") is 0.\n";
	} else {
		fMaxSize = m_uMaxOutputPacketSize;
	}

	pRtpPacket->bLastPacket = 0;

#if 1
	switch (pRtpPacket->NextState) {
	case PACKET_STATE_NEW:
		//DO NOT call xGetNextFrm(), because we get data via linux lib rather than uITRON
		m_pMetaText[0] = 0; //clean string
		m_uCurrFrmOffset = 0;
		if (pNvtMgr->GetNextMetaDataFrm(m_pMetaText, m_uMetaTextSize, m_bFirstFrm) != 0) {
			printf("live555: metadata skipped.\r\n");
			xStopNextFrm();//  handleClosure(this);
			return;
		} else if (m_bPauseState) {
			xDropCurrFrm();
			return;
		}

		m_bFirstFrm = 0;
		m_pCurrFrmData = (u_int8_t *)m_pMetaText;
		m_uCurrFrmSize = strlen((char *)m_pCurrFrmData) + 1;

		if (m_uCurrFrmSize < fMaxSize) {
			u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
			fFrameSize = m_uCurrFrmSize;
			memcpy(fTo, pSrc, fFrameSize);
			m_uCurrFrmOffset += fFrameSize;
			pRtpPacket->bLastPacket = 1;
			pRtpPacket->NextState = PACKET_STATE_NEW;
		} else {
			u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
			fFrameSize = fMaxSize;
			memcpy(fTo, pSrc, fFrameSize);
			m_uCurrFrmOffset += fFrameSize;
			pRtpPacket->NextState = PACKET_STATE_CONT;
		}
		break;
	case PACKET_STATE_CONT: {
			u_int32_t remain_byte = m_uCurrFrmSize - m_uCurrFrmOffset;

			u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
			fFrameSize = (remain_byte < fMaxSize) ? remain_byte : fMaxSize;
			memcpy(fTo, pSrc, fFrameSize);
			m_uCurrFrmOffset += fFrameSize;

			if (remain_byte <= fMaxSize) { //last fragment?
				pRtpPacket->bLastPacket = 1;
				pRtpPacket->NextState = PACKET_STATE_NEW;
			} else {
				pRtpPacket->NextState = PACKET_STATE_CONT;
			}
		}
		break;
	default:
		printf("live555: doGetNextFrame() unhandled value: %d\r\n", pRtpPacket->NextState);
		xStopNextFrm();
		return;
	}
#else
	const char event_msg[] = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?> \
<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" xmlns:xmime=\"http://tempuri.org/xmime.xsd\" xmlns:xop=\"http://www.w3.org/2004/08/xop/include\" xmlns:wsrfbf=\"http://docs.oasis-open.org/wsrf/bf-2\" xmlns:wstop=\"http://docs.oasis-open.org/wsn/t-1\" xmlns:tt=\"http://www.onvif.org/ver10/schema\" xmlns:wsa=\"http://www.w3.org/2005/08/addressing\" xmlns:wsrfr=\"http://docs.oasis-open.org/wsrf/r-2\" xmlns:tetpps=\"http://www.onvif.org/ver10/events/wsdl/PullPointSubscriptionBinding\" xmlns:tete=\"http://www.onvif.org/ver10/events/wsdl/EventBinding\" xmlns:tev=\"http://www.onvif.org/ver10/events/wsdl\" xmlns:tetsm=\"http://www.onvif.org/ver10/events/wsdl/SubscriptionManagerBinding\" xmlns:tetnp=\"http://www.onvif.org/ver10/events/wsdl/NotificationProducerBinding\" xmlns:tetnc=\"http://www.onvif.org/ver10/events/wsdl/NotificationConsumerBinding\" xmlns:tetpp=\"http://www.onvif.org/ver10/events/wsdl/PullPointBinding\" xmlns:tetcp=\"http://www.onvif.org/ver10/events/wsdl/CreatePullPointBinding\" xmlns:tetps=\"http://www.onvif.org/ver10/events/wsdl/PausableSubscriptionManagerBinding\" xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\" xmlns:tns1=\"http://www.onvif.org/ver10/topics\"> \
<SOAP-ENV:Header> \
<wsa:Action>http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/PullMessagesResponse</wsa:Action> \
</SOAP-ENV:Header> \
<SOAP-ENV:Body> \
<tev:PullMessagesResponse> \
<tev:CurrentTime>2000-02-15T02:33:55Z</tev:CurrentTime> \
<tev:TerminationTime>2000-02-15T02:34:55Z</tev:TerminationTime> \
<wsnt:NotificationMessage> \
<wsnt:Topic Dialect=\"http://www.onvif.org/ver10/tev/topicExpression/ConcreteSet\">tns1:VideoSource/MotionAlarm</wsnt:Topic> \
<wsnt:Message> \
<tt:Message PropertyOperation=\"Changed\" UtcTime=\"2000-02-15T02:34:55Z\"> \
<tt:Source> \
<tt:SimpleItem Name=\"Source\" Value=\"1\" /> \
</tt:Source> \
<tt:Data> \
<tt:SimpleItem Name=\"State\" Value=\"false\" /> \
</tt:Data> \
</tt:Message> \
</wsnt:Message> \
</wsnt:NotificationMessage> \
<wsnt:NotificationMessage> \
<wsnt:Topic Dialect=\"http://www.onvif.org/ver10/tev/topicExpression/ConcreteSet\">tns1:Device/Trigger/DigitalInput</wsnt:Topic> \
<wsnt:Message> \
<tt:Message PropertyOperation=\"Changed\" UtcTime=\"2000-02-15T02:34:55Z\"> \
<tt:Source> \
<tt:SimpleItem Name=\"InputToken\" Value=\"1\" /> \
</tt:Source> \
<tt:Data> \
<tt:SimpleItem Name=\"LogicalState\" Value=\"false\" /> \
</tt:Data> \
</tt:Message> \
</wsnt:Message> \
</wsnt:NotificationMessage> \
</tev:PullMessagesResponse> \
</SOAP-ENV:Body> \
</SOAP-ENV:Envelope>";
	fFrameSize = sizeof(event_msg);
	memcpy(fTo, event_msg, fFrameSize);
	//always frame end
	pRtpPacket->bLastPacket = 1;
#endif


	//always reset
	fPresentationTime = m_QueueBuf.pts;

#if (CFG_POLLING_MODE)
	u_int32_t frame_time_us = 1000000;
	int64_t uSecondsToGo = pNvtMgr->Get_NextPtsTimeDiff(&fPresentationTime, frame_time_us);
	//fDurationInMicroseconds = frame_time_us;
	fDurationInMicroseconds = (uSecondsToGo < 0) ? 0 : (unsigned)uSecondsToGo;
#endif

	nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
				 (TaskFunc *)FramedSource::afterGetting, this);
}



NvtStreamFramerH265 *NvtStreamFramerH265
::createNew(INIT_DATA *pInit)
{
	return new NvtStreamFramerH265(pInit);
}

NvtStreamFramerH265
::NvtStreamFramerH265(INIT_DATA *pInit)
	: NvtStreamFramer(pInit)
	, m_uBufferOffset(4) //remove 00,00,00,01
	, m_uLastFU_Header(0)
	, m_bResetToKeyFrm(True)
{
	m_uLast_Payload_Header[0] = 0;
	m_uLast_Payload_Header[1] = 0;
}

NvtStreamFramerH265::~NvtStreamFramerH265()
{
}

u_int8_t *NvtStreamFramerH265::xAdjustDataOfs(u_int8_t *pData, u_int32_t *p_size)
{
	if (*p_size >= m_uBufferOffset) {
		pData += m_uBufferOffset;
		*p_size -= m_uBufferOffset;
	}
	return pData;
}

bool NvtStreamFramerH265::xIsKeyFrm()
{
	if (m_pCurrFrmData) {
		u_int32_t nalu_type = m_pCurrFrmData[0] >> 1;
		return (nalu_type == NAL_UNIT_CODED_SLICE_IDR_W_RADL || nalu_type == NAL_UNIT_CODED_SLICE_IDR_N_LP);
	}
	return false;
}

void NvtStreamFramerH265::doGetNextFrame()
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	RTP_PACKET *pRtpPacket = &m_RtpPacket;
	PACKET_STATE CurrState = pRtpPacket->NextState;

	//In this function, fFrameSize, fNumTruncatedBytes, fPresentationTime and fDurationInMicroseconds need to update.
	if (m_uMaxOutputPacketSize == 0) {
		// shouldn't happen
		unsigned uMaxOutputPacketSize = m_uMaxOutputPacketSize;
		envir() << "NvtH265StreamFramer::doGetNextFrame(): m_uMaxOutputPacketSize ("
				<< uMaxOutputPacketSize << ") is 0.\n";
	} else {
		fMaxSize = m_uMaxOutputPacketSize;
	}

	pRtpPacket->bLastPacket = 0;

	if (*pRtpPacket->pByteSent == -1
		&& !m_bResetToKeyFrm) {
		printf("live555: socket sent failed, skip until next I.\r\n");
		m_bResetToKeyFrm = True;
		pNvtMgr->RequireKeyFrame(m_uiMediaSrcID);
	}

	switch (CurrState) {
	case PACKET_STATE_NEW_OR_VPS:
		xGetNextFrm();
		m_pCurrFrmData = xAdjustDataOfs(m_pCurrFrmData, &m_uCurrFrmSize);

		if (m_QueueBuf.strm_info.seq_num == 0) {
			// wait I-Frame
			m_bResetToKeyFrm = True;
		}

		if (m_uCurrFrmSize == 0 || m_pCurrFrmData == NULL)    {
			if (m_pParentSms != NULL) {
				NvtStreamFramer *pPartner = ((NvtServerMediaSubsession *)m_pParentSms)->GetPartnerFramer();
				if (pPartner != NULL) {
					pPartner->SetForceStop(True);
				}
				m_pParentSms = NULL;
			}
			fFrameSize = 0;
			pRtpPacket->NextState = PACKET_STATE_NEW_OR_VPS;
			xStopNextFrm();//  handleClosure(this);
			return;
		} else if (m_bPauseState) {
			xDropCurrFrm();
			return;
		} else  {
			if (m_bResetToKeyFrm) {
				if (xIsKeyFrm()) {
					//Send VPS
					NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
					NVT_VIDEO_INFO *pInfo = pSms->GetVideoInfo();
					u_int8_t *vps = pInfo->vps;
					u_int32_t vps_size = (u_int32_t)pInfo->vps_size;
					vps = xAdjustDataOfs(vps, &vps_size);
					fFrameSize = vps_size;
					*pRtpPacket->ppReplace = NULL;
					memcpy(fTo, vps, fFrameSize);
					pRtpPacket->NextState = PACKET_STATE_SPS;
					m_bResetToKeyFrm = False;
					//pRtpPacket->bLastPacket = 1;
				} else {
					xDropCurrFrm();
					return;
				}
			} else {
				*pRtpPacket->piSocketIdx = 0; //SVC 0,1 use Socket[0]

				if (xIsKeyFrm()) {
					//Send VPS
					NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
					NVT_VIDEO_INFO *pInfo = pSms->GetVideoInfo();
					u_int8_t *vps = pInfo->vps;
					fFrameSize = pInfo->vps_size;
					*pRtpPacket->ppReplace = NULL;
					memcpy(fTo, vps, fFrameSize);
					pRtpPacket->NextState = PACKET_STATE_SPS;
				} else if (m_uCurrFrmSize < fMaxSize) {
					//Send P-Frame
					u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
					fFrameSize = m_uCurrFrmSize;
					//memcpy(fTo,pSrc,fFrameSize);
					*pRtpPacket->ppReplace = pSrc;
					m_uCurrFrmOffset += fFrameSize;
					pRtpPacket->NextState = PACKET_STATE_NEW_OR_VPS;
					pRtpPacket->bLastPacket = 1;
				} else {
					u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
					fFrameSize = fMaxSize;
					*pRtpPacket->ppReplace = pSrc - 1;
					m_uCurrFrmOffset += fFrameSize - 1;

					u_int8_t nal_unit_type = (m_pCurrFrmData[0] & 0x7E) >> 1;
					m_uLast_Payload_Header[0] = (m_pCurrFrmData[0] & 0x81) | (49 << 1); // Payload header (1st byte)
					m_uLast_Payload_Header[1] = m_pCurrFrmData[1]; // Payload header (2nd byte)
					m_uLastFU_Header = 0x80 | nal_unit_type; // FU header (with S bit)

					pSrc[-1] = m_uLast_Payload_Header[0];
					pSrc[0] = m_uLast_Payload_Header[1];
					pSrc[1] = m_uLastFU_Header;

					pRtpPacket->NextState = PACKET_STATE_CONT;
				}
			}
		}
		break;
	case PACKET_STATE_SPS:  {
			//Send SPS
			NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
			NVT_VIDEO_INFO *pInfo = pSms->GetVideoInfo();
			u_int8_t *sps = pInfo->sps;
			u_int32_t sps_size = (u_int32_t)pInfo->sps_size;
			sps = xAdjustDataOfs(sps, &sps_size);
			fFrameSize = sps_size;
			*pRtpPacket->ppReplace = NULL;
			memcpy(fTo, sps, fFrameSize);
			pRtpPacket->NextState = PACKET_STATE_PPS;
		}
		break;
	case PACKET_STATE_PPS: {
			//Send PPS
			NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
			NVT_VIDEO_INFO *pInfo = pSms->GetVideoInfo();
			u_int8_t *pps = pInfo->pps;
			u_int32_t pps_size = (u_int32_t)pInfo->pps_size;
			pps = xAdjustDataOfs(pps, &pps_size);
			fFrameSize = pps_size;
			*pRtpPacket->ppReplace = NULL;
			memcpy(fTo, pps, fFrameSize);
			pRtpPacket->NextState = PACKET_STATE_CONT;
		}
		break;
	case PACKET_STATE_CONT: {
			if (m_uCurrFrmOffset == 0 && m_uCurrFrmSize < fMaxSize) { // I-Frame,that followed by SPS,PPS, and Size is smaller than one RTP size.
				u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];
				fFrameSize = m_uCurrFrmSize;
				//memcpy(fTo,pSrc,fFrameSize);
				*pRtpPacket->ppReplace = pSrc;
				m_uCurrFrmOffset += fFrameSize;
				pRtpPacket->NextState = PACKET_STATE_NEW_OR_VPS;
				pRtpPacket->bLastPacket = 1;
			} else {
				// We are sending this NAL unit data as FU-A packets.  We've already sent the
				// first packet (fragment).  Now, send the next fragment.  Note that we add
				// FU indicator and FU header bytes to the front.  (We reuse these bytes that
				// we already sent for the first fragment, but clear the S bit, and add the E
				// bit if this is the last fragment.)
				u_int32_t remain_byte = m_uCurrFrmSize - m_uCurrFrmOffset;
				u_int32_t uOverHead = (m_uCurrFrmOffset == 0) ? 1 : 3;
				u_int32_t numBytesToSend = uOverHead + m_uCurrFrmSize - m_uCurrFrmOffset;
				u_int8_t *pSrc = &m_pCurrFrmData[m_uCurrFrmOffset];

				if (m_uCurrFrmOffset == 0) { // I-Frame,that followed by SPS,PPS
					u_int8_t nal_unit_type = (m_pCurrFrmData[0] & 0x7E) >> 1;
					m_uLast_Payload_Header[0] = (m_pCurrFrmData[0] & 0x81) | (49 << 1); // Payload header (1st byte)
					m_uLast_Payload_Header[1] = m_pCurrFrmData[1]; // Payload header (2nd byte)
					m_uLastFU_Header = 0x80 | nal_unit_type; // FU header (with S bit)
					pSrc[-1] = m_uLast_Payload_Header[0];
					pSrc[0] = m_uLast_Payload_Header[1];
					pSrc[1] = m_uLastFU_Header;
				} else {
					pSrc[-3] = m_uLast_Payload_Header[0]; // Payload header (1st byte)
					pSrc[-2] = m_uLast_Payload_Header[1]; // Payload header (2nd byte)
					pSrc[-1] = m_uLastFU_Header & ~0x80; // FU header (no S bit)
				}

				fFrameSize = (numBytesToSend < fMaxSize) ? numBytesToSend : fMaxSize;
				//memcpy(fTo+uOverHead,pSrc,fFrameSize-uOverHead);
				*pRtpPacket->ppReplace = pSrc - uOverHead;
				m_uCurrFrmOffset += fFrameSize - uOverHead;


				if ((remain_byte + uOverHead) <= fMaxSize) { //last fragment?
					//fTo[1] |= 0x40; // set the E bit in the FU header
					pSrc[-1] |= 0x40; // set the E bit in the FU header
					pRtpPacket->NextState = PACKET_STATE_NEW_OR_VPS;
					pRtpPacket->bLastPacket = 1;
				} else {
					pRtpPacket->NextState = PACKET_STATE_CONT;
				}
			}
		}
		break;
	default:
		printf("live555: doGetNextFrame() unhandled value: %d\r\n", pRtpPacket->NextState);
		xStopNextFrm();
		return;
	}

	fPresentationTime = m_QueueBuf.pts;

#if (CFG_POLLING_MODE)
	u_int32_t frame_time_us = 1000000 / CFG_RTSPD_FAKE_FPS;
	if (pRtpPacket->bLastPacket) {
		int64_t uSecondsToGo = pNvtMgr->Get_NextPtsTimeDiff(&fPresentationTime, frame_time_us);
		fDurationInMicroseconds = (uSecondsToGo < 0) ? 0 : (unsigned)uSecondsToGo;
	} else {
		fDurationInMicroseconds = 0;
	}
#endif

	nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
				 (TaskFunc *)FramedSource::afterGetting, this);

}

void NvtStreamFramerH265::AfterSendPacket()
{
	NvtStreamFramer::AfterSendPacket();
}
