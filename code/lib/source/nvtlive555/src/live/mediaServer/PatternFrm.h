#ifndef STATICFRMDATA_H
#define STATICFRMDATA_H
#include "NvtMedia.hh"
typedef struct _STATIC_BUF
{
	unsigned char *pBuf;
	unsigned int Size;
} STATIC_BUF, *PSTATIC_BUF;

typedef struct _STATIC_STRM_VIDEO {
	STATIC_BUF VPS;
	STATIC_BUF SPS;
	STATIC_BUF PPS;
	STATIC_BUF Frm[2];
	NVT_CODEC_TYPE  Codec;
	unsigned long long total_ms;
}STATIC_STRM_VIDEO, *PSTATIC_STRM_VIDEO;

typedef struct _STATIC_STRM_AUDIO {
	unsigned int SamplePerSecond;
	unsigned int BitPerSample;
	unsigned int ChannelCnt;
	STATIC_BUF Frm[2];
	NVT_CODEC_TYPE  Codec;
	unsigned long long total_ms;
}STATIC_STRM_AUDIO, *PSTATIC_STRM_AUDIO;

typedef struct _STATIC_CHANNEL {
	STATIC_STRM_VIDEO Video;
	STATIC_STRM_AUDIO Audio;
}STATIC_CHANNEL, *PSTATIC_CHANNEL;

typedef enum _FILEFRM_STATE {
	FILEFRM_STATE_PLAY,
	FILEFRM_STATE_PAUSE,
} FILEFRM_STATE;

STATIC_CHANNEL *StaticFrmGetChannel(int Channel);
ISTRM_CB *StaticFrmGetStrmCb();
STATIC_CHANNEL *FileFrmGetChannel(int Channel);
ISTRM_CB *FileFrmGetStrmCb();
extern "C" int FileFrmSeek(int channel, double request, double *response);
extern "C" int FileFrmSetState(int channel, FILEFRM_STATE state);
extern "C" int FileFrmGetTotalTime(int channel, unsigned int *total_ms);
ISTRM_CB *StreamSenderFrmGetStrmCb();
ISTRM_CB *HdalFrmGetStrmCb();

#endif // STATICFRMDATA_H
