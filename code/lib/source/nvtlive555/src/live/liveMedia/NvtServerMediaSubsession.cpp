/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2013 Live Networks, Inc.  All rights reserved.
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from a MJPG video file.
// Implementation

#include "NvtServerMediaSubsession.hh"
#include "NvtRTPSink.hh"
#include "NvtStreamFramer.hh"


NvtServerMediaSubsession *
NvtServerMediaSubsession::createNew(INIT_DATA *pInit)
{
	return new NvtServerMediaSubsession(pInit);
}

NvtServerMediaSubsession::NvtServerMediaSubsession(INIT_DATA *pInit)
	: FileServerMediaSubsession(*pInit->env, pInit->FileName, False)
	, m_pFarmer(NULL)
	, m_pRTPSink(NULL)
	, m_pPartnerSms(NULL)
	, m_MediaType(pInit->MediaType)
	, m_uiMediaSrcID(pInit->uiMediaSrcID)
	, m_VideoInfo(pInit->VideoInfo)
	, m_AudioInfo(pInit->AudioInfo)
{
	strncpy(m_FileName, pInit->FileName, sizeof(m_FileName) - 1);
	memcpy(&m_UrlInfo, &pInit->UrlInfo, sizeof(m_UrlInfo));
}

NvtServerMediaSubsession::~NvtServerMediaSubsession()
{
	if (m_pPartnerSms != NULL) {
		m_pPartnerSms->SetPartnerSms(NULL);
	}
}

FramedSource *NvtServerMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate)
{
	NvtStreamFramer::INIT_DATA InitData = {0};

	m_pFarmer = NULL;
	estBitrate = 2000; // kbps, estimate

	InitData.env = &envir();
	InitData.FileName = m_FileName;
	InitData.pParentSms = this;
	InitData.SessionID = clientSessionId;
	InitData.uiMediaSrcID = m_uiMediaSrcID;
	
	if (m_MediaType == NVT_MEDIA_TYPE_VIDEO) {
		switch (m_VideoInfo.codec_type) {
		case NVT_CODEC_TYPE_MJPG:
			m_pFarmer = NvtStreamFramerMJPG::createNew(&InitData);
			break;
		case NVT_CODEC_TYPE_H264:
			m_pFarmer = NvtStreamFramerH264::createNew(&InitData);
			break;
		case NVT_CODEC_TYPE_H265:
			m_pFarmer = NvtStreamFramerH265::createNew(&InitData);
			break;
		default:
			printf("live555:createNewStreamSource unknown type %d\r\n", m_VideoInfo.codec_type);
			return NULL;
		}
	} else if (m_MediaType == NVT_MEDIA_TYPE_AUDIO) {
		switch (m_AudioInfo.codec_type) {
		case NVT_CODEC_TYPE_PCM:
			m_pFarmer = NvtStreamFramerPCM::createNew(&InitData);
			break;
		case NVT_CODEC_TYPE_AAC:
			m_pFarmer = NvtStreamFramerAAC::createNew(&InitData);
			break;
		case NVT_CODEC_TYPE_G711_ALAW:
		case NVT_CODEC_TYPE_G711_ULAW:
			m_pFarmer = NvtStreamFramerG711::createNew(&InitData);
			break;
		default:
			printf("live555:createNewStreamSource unknown type %d\r\n", m_VideoInfo.codec_type);
			return NULL;
		}
	} else if (m_MediaType == NVT_MEDIA_TYPE_META) {
		m_pFarmer = NvtStreamFramerMeta::createNew(&InitData);
	} else {
		printf("live555: cannot handle mediatype-2 %d\r\n", m_MediaType);
	}

	return m_pFarmer;
}

RTPSink *NvtServerMediaSubsession::createNewRTPSink(Groupsock *rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource * /*inputSource*/)
{
	NvtRTPSink::INIT_DATA InitData = {0};

	m_pRTPSink = NULL;

	InitData.env = &envir();
	InitData.RTPgs = rtpGroupsock;
	InitData.rtpPayloadType = rtpPayloadTypeIfDynamic;
	InitData.uiMediaSrcID = m_uiMediaSrcID;
	InitData.pParentSms = (void *)this;

	if (m_MediaType == NVT_MEDIA_TYPE_VIDEO) {
		InitData.rtpTimestampFrequency = 90000; //video is always 90KHz
		InitData.numChannels = 1; //video always is default 1
		switch (m_VideoInfo.codec_type) {
		case NVT_CODEC_TYPE_MJPG:
			InitData.rtpPayloadFormatName = "JPEG";
			InitData.rtpPayloadType = 26;
			m_pRTPSink = NvtRTPSinkMJPG::createNew(&InitData);
			break;
		case NVT_CODEC_TYPE_H264:
			InitData.rtpPayloadFormatName = "H264";
			m_pRTPSink = NvtRTPSinkH264::createNew(&InitData);
			break;
		case NVT_CODEC_TYPE_H265:
			InitData.rtpPayloadFormatName = "H265";
			m_pRTPSink = NvtRTPSinkH265::createNew(&InitData);
			break;
		default:
			printf("live555:createNewRTPSink unknown type %d\r\n", m_VideoInfo.codec_type);
			return NULL;
		}
	} else if (m_MediaType == NVT_MEDIA_TYPE_AUDIO) {
		InitData.rtpTimestampFrequency = m_AudioInfo.sample_per_second;
		InitData.numChannels = m_AudioInfo.channel_cnt;
		switch (m_AudioInfo.codec_type) {
		case NVT_CODEC_TYPE_PCM:
			InitData.rtpPayloadFormatName = "L16";
			if (m_AudioInfo.bit_per_sample == 16) {
				if (m_AudioInfo.sample_per_second == 44100 && m_AudioInfo.channel_cnt == 2) {
					InitData.rtpPayloadType = 10; // a static RTP payload type
				} else if (m_AudioInfo.sample_per_second == 44100 && m_AudioInfo.channel_cnt == 1) {
					InitData.rtpPayloadType = 11; // a static RTP payload type
				}
			}
			m_pRTPSink = NvtRTPSinkPCM::createNew(&InitData);
			break;

		case NVT_CODEC_TYPE_AAC:
			InitData.rtpPayloadFormatName = "MPEG4-GENERIC";
			m_pRTPSink = NvtRTPSinkAAC::createNew(&InitData);
			break;

		case NVT_CODEC_TYPE_G711_ULAW:
			InitData.rtpPayloadFormatName = "PCMU";
			if (m_AudioInfo.channel_cnt == 1 && m_AudioInfo.sample_per_second == 8000) {
				InitData.rtpPayloadType = 0;
			}
			m_pRTPSink = NvtRTPSinkG711::createNew(&InitData);
			break;

		case NVT_CODEC_TYPE_G711_ALAW:
			InitData.rtpPayloadFormatName = "PCMA";
			if (m_AudioInfo.channel_cnt == 1 && m_AudioInfo.sample_per_second == 8000) {
				InitData.rtpPayloadType = 8;
			}
			m_pRTPSink = NvtRTPSinkG711::createNew(&InitData);
			break;

		default:
			printf("live555:createNewStreamSource unknown type %d\r\n", m_AudioInfo.codec_type);
			return NULL;
		}
	} else if (m_MediaType == NVT_MEDIA_TYPE_META) {
		InitData.rtpTimestampFrequency = 90000;
		InitData.numChannels = 1; //meta always is default 1
		InitData.rtpPayloadFormatName = "vnd.onvif.metadata";
		m_pRTPSink = NvtRTPSinkMeta::createNew(&InitData);
	} else {
		printf("live555: cannot handle mediatype-1 %d\r\n", m_MediaType);
	}

	return m_pRTPSink;
}

void NvtServerMediaSubsession::seekStreamSource(FramedSource *inputSource, double &seekNPT, double streamDuration, u_int64_t &numBytes)
{
	// it's possible frame has deleted before seek in condition for 2 clients quick connect-disconnect.
	if (m_pFarmer) {
		seekNPT = m_pFarmer->Seek(m_uiMediaSrcID, seekNPT);
	} else {
		seekNPT = 0.0;
	}
}

float NvtServerMediaSubsession::duration() const
{
	return (float)m_UrlInfo.total_ms / 1000;
}

NvtStreamFramer *NvtServerMediaSubsession::GetPartnerFramer()
{
	if (m_pPartnerSms == NULL) {
		return NULL;
	}
	return m_pPartnerSms->GetFramer();
}

void NvtServerMediaSubsession::SetPartnerSms(NvtServerMediaSubsession *pPartnerSms)
{
	m_pPartnerSms = pPartnerSms;
}

int NvtServerMediaSubsession::RefreshCodecInfo(u_int32_t hStrm)
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	ISTRM_CB *pStrmCb = pNvtMgr->Get_StrmCb();
	if (m_MediaType == NVT_MEDIA_TYPE_VIDEO) {
		return pStrmCb->GetVideoInfo(hStrm, m_uiMediaSrcID, 100, &m_VideoInfo);
	} else if (m_MediaType == NVT_MEDIA_TYPE_AUDIO) {
		return pStrmCb->GetAudioInfo(hStrm, m_uiMediaSrcID, 100, &m_AudioInfo);
	}
	return 0;
}

NvtStreamFramer *NvtServerMediaSubsession::GetFramer()
{
	return m_pFarmer;
}

NvtRTPSink *NvtServerMediaSubsession::GetRTPSink()
{
	return m_pRTPSink;    //wifiteam
}

NVT_MEDIA_TYPE NvtServerMediaSubsession::GetMediaType()
{
	return m_MediaType;
}

NVT_VIDEO_INFO *NvtServerMediaSubsession::GetVideoInfo()
{
	return &m_VideoInfo;
}

NVT_AUDIO_INFO *NvtServerMediaSubsession::GetAudioInfo()
{
	return &m_AudioInfo;
}

NVTLIVE555_URL_INFO *NvtServerMediaSubsession::GetUrlInfo()
{
	return &m_UrlInfo;
}

void NvtServerMediaSubsession::NotifyFramerDeleted()
{
	m_pFarmer = NULL;
}

void NvtServerMediaSubsession::NotifySinkDeleted()
{
	m_pRTPSink = NULL;
}

u_int32_t NvtServerMediaSubsession::GetMediaSrcID()
{
	return m_uiMediaSrcID;
}