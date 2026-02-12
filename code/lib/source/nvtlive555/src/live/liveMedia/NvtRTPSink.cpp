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
// RTP sink for H.264 video (RFC 3984)
// Implementation

#include "NvtRTPSink.hh"
#include "NvtStreamFramer.hh"
#include "NvtServerMediaSubsession.hh"
#include "Base64.hh"
#include "GroupsockHelper.hh"

NvtRTPSink::NvtRTPSink(INIT_DATA *pInit)
	: MultiFramedRTPSink(*pInit->env, pInit->RTPgs, pInit->rtpPayloadType, pInit->rtpTimestampFrequency, pInit->rtpPayloadFormatName, pInit->numChannels)
	, m_pRtpPacket(NULL)
	, m_pFramer(NULL)
	, m_pParentSms(pInit->pParentSms)
	, m_uiMediaSrcID(pInit->uiMediaSrcID)
{
	NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
	setOnSendErrorFunc(&NvtRTPSink::OnSendErrorFunc, this);

	m_MediaType = pSms->GetMediaType();
	m_RtpHdrSize = NVT_RTP_HDR_SIZE_STD;

	if (m_MediaType == NVT_MEDIA_TYPE_VIDEO) {
		if (pSms->GetVideoInfo()->codec_type == NVT_CODEC_TYPE_MJPG) {
			m_RtpHdrSize = NVT_RTP_HDR_SIZE_MJPG;
		}
		//increase video buffer to send_buf_size user expect
		increaseSendBufferTo(*pInit->env, pInit->RTPgs->socketNum(), pNvtMgr->Get_SendBufSize());
	} else if (m_MediaType == NVT_MEDIA_TYPE_AUDIO) {
		if (pSms->GetAudioInfo()->codec_type == NVT_CODEC_TYPE_AAC) {
			m_RtpHdrSize = NVT_RTP_HDR_SIZE_AAC;
		}
	} else if (m_MediaType == NVT_MEDIA_TYPE_META) {
		m_RtpHdrSize = NVT_RTP_HDR_SIZE_STD;
	} else {
		printf("live555: cannot handle mediatype-3 %d\r\n", m_MediaType);
	}
}

NvtRTPSink::~NvtRTPSink()
{
	NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
	if (pSms) {
		pSms->NotifySinkDeleted();
	} else {
		printf("live555: ~NvtRTPSink's pSms cannot be NULL.\r\n");
	}
}

void NvtRTPSink::SetForceStop()
{
	if (m_pFramer != NULL) {
		m_pFramer->SetForceStop(True);
	}
}

void NvtRTPSink::ResetBuffer()
{
	fOutBuf->resetOffset();
}

unsigned int NvtRTPSink::GetDstAddress()
{
	if (fRTPInterface.gs()) {
		return fRTPInterface.gs()->destAddress().s_addr;
	} else {
		return 0;
	}
}

NvtStreamFramer* NvtRTPSink::GetFramer()
{
	return m_pFramer;
}

void NvtRTPSink::OnSendErrorFunc(void *clientData)
{
	NvtRTPSink *pSink = (NvtRTPSink *)clientData;
	printf("live555: OnSendErrorFunc()\r\n");
	pSink->envir().reportBackgroundError();
	pSink->SetForceStop();
}

Boolean NvtRTPSink::sourceIsCompatibleWithUs(MediaSource &source)
{
	// Our source must be an appropriate framer:
	m_pFramer = (NvtStreamFramer *) &source;
	m_pFramer->SetMaxOutputPacketSize(ourMaxPacketSize() - m_RtpHdrSize);

	m_pRtpPacket = m_pFramer->GetRtpPacket();
	m_pRtpPacket->pBuf = fOutBuf->curPtr() + m_RtpHdrSize;
	m_pRtpPacket->ppReplace = &fpReplace;
	m_pRtpPacket->pSkipThisSend = &bSkipThisSend;
	m_pRtpPacket->piSocketIdx = &fSocketIdx; //NVT_MODIFIED, Multi-UDP Source
	m_pRtpPacket->pByteSent = &fByteSent; //NVT_MODIFIED, WIFI-TEAM
	return True;
}

void NvtRTPSink::doSpecialFrameHandling(unsigned /*fragmentationOffset*/,
										unsigned char * /*frameStart*/,
										unsigned /*numBytesInFrame*/,
										struct timeval framePresentationTime,
										unsigned /*numRemainingBytes*/)
{
	if (m_pRtpPacket->bLastPacket) {
		setMarkerBit();
	}
	setTimestamp(framePresentationTime);
}

Boolean NvtRTPSink::frameCanAppearAfterPacketStart(unsigned char const * /*frameStart*/,
		unsigned /*numBytesInFrame*/) const
{
	return False;
}

void NvtRTPSink::NotifyFinishSendPacket()
{
	m_pFramer->AfterSendPacket();
}


NvtRTPSinkVideo::NvtRTPSinkVideo(INIT_DATA *pInit)
	: NvtRTPSink(pInit)
{
}

NvtRTPSinkVideo::~NvtRTPSinkVideo()
{
}

NvtRTPSinkAudio::NvtRTPSinkAudio(INIT_DATA *pInit)
	: NvtRTPSink(pInit)
{
}

NvtRTPSinkAudio::~NvtRTPSinkAudio()
{
}


NvtRTPSinkMJPG *NvtRTPSinkMJPG::createNew(INIT_DATA *pInit)
{
	return new NvtRTPSinkMJPG(pInit);
}

NvtRTPSinkMJPG::NvtRTPSinkMJPG(INIT_DATA *pInit)
	: NvtRTPSinkVideo(pInit)
	, m_FrameOffset(0)
	, m_LastnumBytesInFrame(0)
{
	memset(m_QTblTmp, 0, sizeof(m_QTblTmp));
}

NvtRTPSinkMJPG::~NvtRTPSinkMJPG()
{
}

void NvtRTPSinkMJPG::doSpecialFrameHandling(unsigned fragmentationOffset,
		unsigned char *frameStart,
		unsigned numBytesInFrame,
		struct timeval framePresentationTime,
		unsigned numRemainingBytes)
{
	NvtStreamFramerMJPG *pSource = (NvtStreamFramerMJPG *)m_pFramer;

	u_int8_t mainJPEGHeader[8]; // the special header
	u_int8_t const type = pSource->type();

	mainJPEGHeader[0] = 0; // Type-specific
	mainJPEGHeader[1] = m_FrameOffset >> 16;
	mainJPEGHeader[2] = m_FrameOffset >> 8;
	mainJPEGHeader[3] = m_FrameOffset;
	mainJPEGHeader[4] = type;
	mainJPEGHeader[5] = pSource->qFactor();
	mainJPEGHeader[6] = pSource->width();
	mainJPEGHeader[7] = pSource->height();
	setSpecialHeaderBytes(mainJPEGHeader, sizeof mainJPEGHeader);

	if (m_FrameOffset == 0) {
		// There is also a Quantization Header:
		u_int8_t precision;
		u_int16_t length;
		u_int8_t const *quantizationTables
			= pSource->quantizationTables(precision, length);

		if (length != 128) {
			printf("NvtMJPGRTPSink only support 2 Q-Tbls\n");
		}

		u_int8_t *quantizationHeader;
		unsigned const quantizationHeaderSize = 4 + length;
		if (quantizationHeaderSize > sizeof(m_QTblTmp)) {
			printf("GetQTblTmp too large size.\n");
			abort();
		} else {
			quantizationHeader = m_QTblTmp;
		}
		quantizationHeader[0] = 0; // MBZ
		quantizationHeader[1] = precision;
		quantizationHeader[2] = length >> 8;
		quantizationHeader[3] = length & 0xFF;
		if (quantizationTables != NULL) { // sanity check
			for (u_int16_t i = 0; i < length; ++i) {
				quantizationHeader[4 + i] = quantizationTables[i];
			}
		}

		setSpecialHeaderBytes(quantizationHeader, quantizationHeaderSize, sizeof mainJPEGHeader);
	}

	m_FrameOffset += numBytesInFrame;

	if (m_pRtpPacket->bLastPacket) {
		m_FrameOffset = 0;
	}

	NvtRTPSink::doSpecialFrameHandling(fragmentationOffset, frameStart, numBytesInFrame, framePresentationTime, numRemainingBytes);
}

unsigned NvtRTPSinkMJPG::specialHeaderSize() const
{
	/*
	    because specialHeaderSize is invoked before doGetNextFrame and after doSpecialFrameHandling,
	    so we cannot use m_FrameOffset to evaluate the dynamic header size.
	*/
	NvtStreamFramerMJPG *pFramer = (NvtStreamFramerMJPG *)m_pFramer;
	return pFramer->GetSpecialHdrSize();
}

NvtRTPSinkH264 *NvtRTPSinkH264::createNew(INIT_DATA *pInit)
{
	return new NvtRTPSinkH264(pInit);
}

NvtRTPSinkH264::NvtRTPSinkH264(INIT_DATA *pInit)
	: NvtRTPSinkVideo(pInit)
	, m_FmtpSDPLine(NULL)
{
}

NvtRTPSinkH264::~NvtRTPSinkH264()
{
	if (m_FmtpSDPLine) {
		delete[] m_FmtpSDPLine;
	}
}

char const *NvtRTPSinkH264::auxSDPLine()
{
	// Generate a new "a=fmtp:" line each time, using our SPS and PPS (if we have them),
	// otherwise parameters from our framer source (in case they've changed since the last time that
	// we were called):
	NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
	NVT_VIDEO_INFO *pInfo = pSms->GetVideoInfo();

	u_int8_t *sps = pInfo->sps;
	u_int32_t spsSize = pInfo->sps_size;
	u_int8_t *pps = pInfo->pps;
	u_int32_t ppsSize = pInfo->pps_size;

	if (sps == NULL || pps == NULL) {
		return NULL;
	}

	//here must remove 00 00 00 01
	if (sps && sps[0] == 0x00 && sps[1] == 0x00 && sps[2] == 0x00 && sps[3] == 0x01) {
		sps += 4;
		spsSize -= 4;
	}
	if (pps && pps[0] == 0x00 && pps[1] == 0x00 && pps[2] == 0x00 && pps[3] == 0x01) {
		pps += 4;
		ppsSize -= 4;
	}

	u_int32_t profile_level_id;
	if (spsSize < 4) { // sanity check
		profile_level_id = 0;
	} else {
		//sps with 00 00 00 01
		unsigned int profile_idc = sps[1];
		unsigned int constraint_setN_flag = sps[2];
		unsigned int level_idc = sps[3];
		profile_level_id = (profile_idc << 16) | (constraint_setN_flag << 8) | level_idc; // profile_idc|constraint_setN_flag|level_idc
	}

	// Set up the "a=fmtp:" SDP line for this stream:
	char *sps_base64 = base64Encode((char *)sps, spsSize);
	char *pps_base64 = base64Encode((char *)pps, ppsSize);
	char const *fmtpFmt =
		"a=fmtp:%d packetization-mode=1"
		";profile-level-id=%06X"
		";sprop-parameter-sets=%s,%s\r\n";
	unsigned fmtpFmtSize = strlen(fmtpFmt)
						   + 3 /* max char len */
						   + 6 /* 3 bytes in hex */
						   + strlen(sps_base64) + strlen(pps_base64);
	char *fmtp = new char[fmtpFmtSize];
	sprintf(fmtp, fmtpFmt,
			rtpPayloadType(),
			profile_level_id,
			sps_base64, pps_base64);
	delete[] sps_base64;
	delete[] pps_base64;

	if (m_FmtpSDPLine) {
		delete[] m_FmtpSDPLine;
		m_FmtpSDPLine = NULL;
	}
	m_FmtpSDPLine = fmtp;

	return m_FmtpSDPLine;
}


NvtRTPSinkH265 *NvtRTPSinkH265::createNew(INIT_DATA *pInit)
{
	return new NvtRTPSinkH265(pInit);
}

NvtRTPSinkH265::NvtRTPSinkH265(INIT_DATA *pInit)
	: NvtRTPSinkVideo(pInit)
	, m_FmtpSDPLine(NULL)
{
}

NvtRTPSinkH265::~NvtRTPSinkH265()
{
	if (m_FmtpSDPLine) {
		delete[] m_FmtpSDPLine;
	}
}

unsigned NvtRTPSinkH265::removeH264or5EmulationBytes(u_int8_t *to, unsigned toMaxSize,
		u_int8_t const *from, unsigned fromSize)
{
	unsigned toSize = 0;
	unsigned i = 0;
	while (i < fromSize && toSize + 1 < toMaxSize) {
		if (i + 2 < fromSize && from[i] == 0 && from[i + 1] == 0 && from[i + 2] == 3) {
			to[toSize] = to[toSize + 1] = 0;
			toSize += 2;
			i += 3;
		} else {
			to[toSize] = from[i];
			toSize += 1;
			i += 1;
		}
	}

	return toSize;
}


char const *NvtRTPSinkH265::auxSDPLine()
{
	// Generate a new "a=fmtp:" line each time, using our SPS and PPS (if we have them),
	// otherwise parameters from our framer source (in case they've changed since the last time that
	// we were called):
	NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
	NVT_VIDEO_INFO *pInfo = pSms->GetVideoInfo();

	u_int8_t *vps = pInfo->vps;
	u_int32_t vpsSize = pInfo->vps_size;
	u_int8_t *sps = pInfo->sps;
	u_int32_t spsSize = pInfo->sps_size;
	u_int8_t *pps = pInfo->pps;
	u_int32_t ppsSize = pInfo->pps_size;

	if (sps == NULL || pps == NULL || vps == NULL) {
		return NULL;
	}

	//here must remove 00 00 00 01
	if (vps && vps[0]==0x00 && vps[1]==0x00 && vps[2]== 0x00 && vps[3]==0x01) {
		vps += 4;
		vpsSize -= 4;
	}
	if (sps && sps[0] == 0x00 && sps[1] == 0x00 && sps[2] == 0x00 && sps[3] == 0x01) {
		sps += 4;
		spsSize -= 4;
	}
	if (pps && pps[0] == 0x00 && pps[1] == 0x00 && pps[2] == 0x00 && pps[3] == 0x01) {
		pps += 4;
		ppsSize -= 4;
	}

	u_int8_t *vpsWEB = new u_int8_t[vpsSize]; // "WEB" means "Without Emulation Bytes"
	unsigned vpsWEBSize = removeH264or5EmulationBytes(vpsWEB, vpsSize, vps, vpsSize);
	if (vpsWEBSize < 6/*'profile_tier_level' offset*/ + 12/*num 'profile_tier_level' bytes*/) {
		// Bad VPS size => assume our source isn't ready
		delete[] vpsWEB;
		return NULL;
	}

	u_int8_t const *profileTierLevelHeaderBytes = &vpsWEB[6];
	unsigned profileSpace = profileTierLevelHeaderBytes[0] >> 6; // general_profile_space
	unsigned profileId = profileTierLevelHeaderBytes[0] & 0x1F; // general_profile_idc
	unsigned tierFlag = (profileTierLevelHeaderBytes[0] >> 5) & 0x1; // general_tier_flag
	unsigned levelId = profileTierLevelHeaderBytes[11]; // general_level_idc
	u_int8_t const *interop_constraints = &profileTierLevelHeaderBytes[5];
	char interopConstraintsStr[16] = { 0 };
	sprintf(interopConstraintsStr, "%02X%02X%02X%02X%02X%02X",
			interop_constraints[0], interop_constraints[1], interop_constraints[2],
			interop_constraints[3], interop_constraints[4], interop_constraints[5]);
	delete[] vpsWEB;


	char *sprop_vps = base64Encode((char *)vps, vpsSize);
	char *sprop_sps = base64Encode((char *)sps, spsSize);
	char *sprop_pps = base64Encode((char *)pps, ppsSize);

	char const *fmtpFmt =
		"a=fmtp:%d profile-space=%u"
		";profile-id=%u"
		";tier-flag=%u"
		";level-id=%u"
		";interop-constraints=%s"
		";sprop-vps=%s"
		";sprop-sps=%s"
		";sprop-pps=%s\r\n";
	unsigned fmtpFmtSize = strlen(fmtpFmt)
						   + 3 /* max num chars: rtpPayloadType */ + 20 /* max num chars: profile_space */
						   + 20 /* max num chars: profile_id */
						   + 20 /* max num chars: tier_flag */
						   + 20 /* max num chars: level_id */
						   + strlen(interopConstraintsStr)
						   + strlen(sprop_vps)
						   + strlen(sprop_sps)
						   + strlen(sprop_pps);
	char *fmtp = new char[fmtpFmtSize];
	sprintf(fmtp, fmtpFmt,
			rtpPayloadType(), profileSpace,
			profileId,
			tierFlag,
			levelId,
			interopConstraintsStr,
			sprop_vps,
			sprop_sps,
			sprop_pps);

	delete[] sprop_vps;
	delete[] sprop_sps;
	delete[] sprop_pps;

	if (m_FmtpSDPLine) {
		delete[] m_FmtpSDPLine;
		m_FmtpSDPLine = NULL;
	}
	m_FmtpSDPLine = fmtp;
	return m_FmtpSDPLine;
}

NvtRTPSinkPCM *NvtRTPSinkPCM::createNew(INIT_DATA *pInit)
{
	return new NvtRTPSinkPCM(pInit);
}

NvtRTPSinkPCM::NvtRTPSinkPCM(INIT_DATA *pInit)
	: NvtRTPSinkAudio(pInit)
{
}

NvtRTPSinkPCM::~NvtRTPSinkPCM()
{
}

NvtRTPSinkAAC *NvtRTPSinkAAC::createNew(INIT_DATA *pInit)
{
	return new NvtRTPSinkAAC(pInit);
}

NvtRTPSinkAAC::NvtRTPSinkAAC(INIT_DATA *pInit)
	: NvtRTPSinkAudio(pInit)
	, m_FmtpSDPLine(NULL)
{
}

NvtRTPSinkAAC::~NvtRTPSinkAAC()
{
	if (m_FmtpSDPLine) {
		delete[] m_FmtpSDPLine;
	}
}

unsigned NvtRTPSinkAAC::specialHeaderSize() const
{
	return NVT_RTP_HDR_SIZE_AAC_SUB_MAIN;
}

static unsigned const samplingFrequencyFromIndex[16] = {
	96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
	16000, 12000, 11025, 8000, 7350, 0, 0, 0
};

char const *NvtRTPSinkAAC::auxSDPLine()
{
	NvtServerMediaSubsession *pSms = (NvtServerMediaSubsession *)m_pParentSms;
	NVT_AUDIO_INFO *pInfo = pSms->GetAudioInfo();

	int i;
	char fConfigString[16] = {0};
	unsigned char audioSpecificConfig[2];

	u_int8_t profile = 1; //NVT's profile is always main.
	u_int8_t const audioObjectType = profile + 1;
	u_int8_t samplingFrequencyIndex = 0;
	u_int8_t channelConfiguration = pInfo->channel_cnt;

	for (i = 0; i < 16; i++) {
		if (pInfo->sample_per_second == (int)samplingFrequencyFromIndex[i]) {
			samplingFrequencyIndex = i;
			break;
		}
	}

	if (channelConfiguration != 1 && channelConfiguration != 2) {
		envir() << "channelConfiguration wrong value \n";
	}

	audioSpecificConfig[0] = (audioObjectType << 3) | (samplingFrequencyIndex >> 1); //samplingFrequencyIndex 有 table 對應 44.1 那些
	audioSpecificConfig[1] = (samplingFrequencyIndex << 7) | (channelConfiguration << 3); //也是 aac header 裡面的
	sprintf(fConfigString, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);

	// Set up the "a=fmtp:" SDP line for this stream:
	char const *fmtpFmt =
		"a=fmtp:%d "
		"streamtype=%d;profile-level-id=1;"
		"mode=%s;sizelength=13;indexlength=3;indexdeltalength=3;"
		"config=%s\r\n";
	unsigned fmtpFmtSize = strlen(fmtpFmt)
						   + 3 /* max char len */
						   + 3 /* max char len */
						   + strlen("AAC-hbr")
						   + strlen(fConfigString);
	char *fmtp = new char[fmtpFmtSize];
	sprintf(fmtp, fmtpFmt,
			rtpPayloadType(),
			5, //5 for audio type
			"AAC-hbr",
			fConfigString);
	m_FmtpSDPLine = strDup(fmtp);
	delete[] fmtp;

	return m_FmtpSDPLine;
}

void NvtRTPSinkAAC::doSpecialFrameHandling(unsigned fragmentationOffset,
		unsigned char *frameStart,
		unsigned numBytesInFrame,
		struct timeval framePresentationTime,
		unsigned numRemainingBytes)
{
	// Set the "AU Header Section".  This is 4 bytes: 2 bytes for the
	// initial "AU-headers-length" field, and 2 bytes for the first
	// (and only) "AU Header":
	unsigned fullFrameSize
		= fragmentationOffset + numBytesInFrame + numRemainingBytes;
	unsigned char headers[4];
	headers[0] = 0;
	headers[1] = 16 /* bits */; // AU-headers-length
	headers[2] = fullFrameSize >> 5;
	headers[3] = (fullFrameSize & 0x1F) << 3;

	setSpecialHeaderBytes(headers, sizeof headers);

	NvtRTPSink::doSpecialFrameHandling(fragmentationOffset, frameStart, numBytesInFrame, framePresentationTime, numRemainingBytes);
}


NvtRTPSinkG711 *NvtRTPSinkG711::createNew(INIT_DATA *pInit)
{
	return new NvtRTPSinkG711(pInit);
}

NvtRTPSinkG711::NvtRTPSinkG711(INIT_DATA *pInit)
	: NvtRTPSinkAudio(pInit)
{
}

NvtRTPSinkG711::~NvtRTPSinkG711()
{
}

NvtRTPSinkMeta *NvtRTPSinkMeta::createNew(INIT_DATA *pInit)
{
	return new NvtRTPSinkMeta(pInit);
}

NvtRTPSinkMeta::NvtRTPSinkMeta(INIT_DATA *pInit)
	: NvtRTPSink(pInit)
{
}

NvtRTPSinkMeta::~NvtRTPSinkMeta()
{
}