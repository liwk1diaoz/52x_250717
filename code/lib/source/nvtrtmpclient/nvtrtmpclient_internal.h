/**
    Header file of NvtRtmpClient (internal)

    @file       nvtrtmpclient_internal.h
    @ingroup    NVTRTMPCLIENT
    @version    V1.00.000
    @date       2018/03/14

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _NVTRTMPCLIENT_INTERNAL_H_
#define _NVTRTMPCLIENT_INTERNAL_H_

#include <semaphore.h>

/**
    @addtogroup NVTRTMPCLIENT
*/
//@{

/**
    @name max rtmp url length

    max rtmp url length.
*/
//@{
#define MAX_RTMPURL_LEN         1024        ///< max string length of rtmp URL
//@}


/**
    @name NvtRtmpClient Status

    NvtRtmpClient status.
*/
//@{
#define NVTRTMP_CLIENT_STATUS_CLOSE               0        ///< NvtRtmp Client Status is closed
#define NVTRTMP_CLIENT_STATUS_OPEN                1        ///< NvtRtmp Client Status is opened
#define NVTRTMP_CLIENT_STATUS_START               2        ///< NvtRtmp Client Status is started
#define NVTRTMP_CLIENT_STATUS_STOP                3        ///< NvtRtmp Client Status is stopped
//@}

typedef struct {
    UINT32  Addr;               ///< Video frame address.
    UINT32  Size;               ///< Video frame size.
	BOOL    bIFrameFlag;        ///< I-Frame flag. NVT_TRUE means this frame is I-frame
	struct  timeval TimeStamp;  ///< Encoded timestamp
	UINT32  SVCLayerId;         ///< SVC temporal layer ID
} RtmpVencStream;

typedef struct {
    UINT32 Addr;                ///< Audio frame address.
    UINT32 Size;                ///< Audio frame size.
	struct timeval TimeStamp;   ///< Encoded timestamp
} RtmpAinStream;

typedef struct
{
    UINT32 bufLength;           ///< Buffer size.
    UINT8 *bufPtr;              ///< Buffer address.
} PayloadBuf;


typedef struct _NVTRTMP_CLIENT
{
    UINT32      status;                     ///< NvtRtmpClient status

    pthread_t   video_enc_thread_id;        ///< pthread ID for video thread
    pthread_t   audio_enc_thread_id;        ///< pthread ID for audio thread
    int         enc_exit;                   ///< flag to set to if you want video/audio thread to continue looping
    sem_t       semVideo;                   ///< for NvtRtmpClient_Stop() to wait video thread
    sem_t       semAudio;                   ///< for NvtRtmpClient_Stop() to wait audio thread
    sem_t       semReconnect;               ///< for video/audio thread to auto re-connect

    char        rtmp_url[MAX_RTMPURL_LEN];  ///< destination rtmp URL to push stream
    int         chanId;                     ///< source channel to get video/audio bitstream
    void        *rtmp_client;               ///< control object for libpushrmp

    INT32       hDevVideo;                  ///< video control handler for libnvtstreamsender
    INT32       hDevAudio;                  ///< audio control handler for libnvtstreamsender
    PayloadBuf  vPayloadBuf;                ///< temp buffer to put video bitstream
    PayloadBuf  aPayloadBuf;                ///< temp buffer to put audio bitstream
} NVTRTMP_CLIENT, *PNVTRTMP_CLIENT;


//@}
#endif /* _NVTRTMPCLIENT_INTERNAL_H_  */


