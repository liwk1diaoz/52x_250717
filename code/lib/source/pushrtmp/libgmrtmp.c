/*******************************************************************************
* Copyright  GrainMedia Technology Corporation 2010-2011.  All rights reserved.*
*------------------------------------------------------------------------------*
* Name: libgmrtmp.c                                                            *
* Description:                                                                 *
*     1: ......                                                                *
* Author: giggs                                                                *
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/prctl.h>
#include <poll.h>
#include <netinet/tcp.h>
#include "rtmp.h"
#include "log.h"
#include "libgmrtmp.h"

//==============================================================================
//
//                              DEFINES
//
//==============================================================================

#define LIBGMRTMP_DBG_FRAME_INFO            0
#define LIBGMRTMP_DBG_SEND_PACKET_PERF      0
#define LIBGMRTMP_DBG_TIMESTAMP_ADJUST      0

#define MAX_RTMP_SERVER_URL_KEY_LEN         64
#define MAX_RTMP_SERVER_STREAM              16
#define MAX_RTMP_SERVER_CONNECT             64

#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)

typedef struct _GM_RTMP_CLIENT
{
    void* rtmp_handle;
    unsigned int timestamp_base;
    unsigned int timestamp_latest;
    unsigned int timestamp_video_adj;
    unsigned int timestamp_audio_adj;
    int body_buf_size;
    char *body_buf;
    unsigned int video_frames;
    unsigned int audio_frames;
    pthread_mutex_t mutex;
} GM_RTMP_CLIENT;

typedef struct
{
    int                 connected;
    int                 client_fd;
    GM_RTMP_CLIENT      *rtmp_client;
    int                 stream_id;
    int                 playing;
    int                 I_frame_arrival;
    char                *connect;
    void                *private;
    pthread_mutex_t     lock;
} CONNECTION_INFO;

typedef struct
{
    int                 used;
    char                *buf;
    int                 buf_size;
    char                stream_url[MAX_RTMP_SERVER_URL_KEY_LEN];
    char                stream_key[MAX_RTMP_SERVER_URL_KEY_LEN];
    pthread_mutex_t     lock;
} STREAM_INFO;

typedef struct
{
    int                 server_fd;
    void                *sslCtx;
    CONNECTION_INFO     connection[MAX_RTMP_SERVER_CONNECT];
    STREAM_INFO         stream[MAX_RTMP_SERVER_STREAM];
    pthread_mutex_t     lock;
} STREAMING_SERVER;

#define STR2AVAL(av,str)    av.av_val = str; av.av_len = strlen(av.av_val)
#define SAVC(x) static const AVal av_##x = AVC(#x)

#define TIMEVAL_DIFF(start, end) (((long)(end.tv_sec)-(long)(start.tv_sec))*1000000+((long)(end.tv_usec)-(long)(start.tv_usec)))
#define NAMED_THREAD()       \
        {   \
            char thread_name[16];   \
            snprintf(thread_name, sizeof(thread_name), "%s", __FUNCTION__); \
            prctl(PR_SET_NAME, (unsigned long)thread_name); \
        }

//==============================================================================
//
//                              EXTERNAL VARIABLES REFERENCE
//
//==============================================================================

//==============================================================================
//
//                              EXTERNAL FUNCTION PROTOTYPES
//
//==============================================================================

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

static const AVal av_dquote = AVC("\"");
static const AVal av_escdquote = AVC("\\\"");

SAVC(app);
SAVC(connect);
SAVC(flashVer);
SAVC(swfUrl);
SAVC(pageUrl);
SAVC(tcUrl);
//SAVC(fpad);
SAVC(capabilities);
SAVC(audioCodecs);
SAVC(videoCodecs);
//SAVC(videoFunction);
SAVC(objectEncoding);
SAVC(_result);
SAVC(createStream);
SAVC(getStreamLength);
SAVC(play);
SAVC(fmsVer);
//SAVC(mode);
SAVC(level);
SAVC(code);
SAVC(description);
//SAVC(secureToken);
SAVC(pause);

SAVC(onStatus);
SAVC(status);
static const AVal av_NetStream_Play_Start = AVC("NetStream.Play.Start");
static const AVal av_Started_playing = AVC("Start Live");
static const AVal av_NetStream_Play_Stop = AVC("NetStream.Play.Stop");
static const AVal av_Stopped_playing = AVC("Stopped playing");
static const AVal av_NetStream_Pause_Notify = AVC("NetStream.Pause.Notify");
static const AVal av_Paused_live = AVC("Paused live");
static const AVal av_NetStream_Unpause_Notify = AVC("NetStream.Unpause.Notify");
static const AVal av_Unpaused_live = AVC("Unpaused live");
SAVC(details);
SAVC(clientid);
static const AVal av_NetStream_Authenticate_UsherToken = AVC("NetStream.Authenticate.UsherToken");
static const AVal av_RtmpSampleAccess = AVC("|RtmpSampleAccess");

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

void *GM_RtmpClientInit(int buf_size)
{
    GM_RTMP_CLIENT *rtmp_c = calloc(1, sizeof(GM_RTMP_CLIENT));

    if (!rtmp_c)
    {
        goto GM_RtmpClientInit_err;
    }

    pthread_mutex_init(&rtmp_c->mutex, NULL);
    pthread_mutex_lock(&rtmp_c->mutex);

    rtmp_c->rtmp_handle = (void *)RTMP_Alloc();
    if (!rtmp_c->rtmp_handle)
    {
        goto GM_RtmpClientInit_err;
    }
    RTMP_Init((RTMP *)rtmp_c->rtmp_handle);

    if (rtmp_c->body_buf_size)
    {
        rtmp_c->body_buf = calloc(rtmp_c->body_buf_size, 1);
        if (!rtmp_c->body_buf)
        {
            rtmp_c->body_buf_size = 0;
            RTMP_Free((RTMP *)rtmp_c->rtmp_handle);

            goto GM_RtmpClientInit_err;
        }
    }

    pthread_mutex_unlock(&rtmp_c->mutex);
    return rtmp_c;

GM_RtmpClientInit_err:
    if (rtmp_c)
    {
        pthread_mutex_unlock(&rtmp_c->mutex);
        free(rtmp_c);
    }
    return NULL;
}

void GM_RtmpClientDestroy(void *client)
{
    GM_RTMP_CLIENT *rtmp_c = (GM_RTMP_CLIENT *)client;
    RTMP * p_rtmp_handle = (RTMP *)rtmp_c->rtmp_handle;

    if (!rtmp_c)
        return;

    pthread_mutex_lock(&rtmp_c->mutex);
    if (p_rtmp_handle)
    {
        RTMP_Close(p_rtmp_handle);
        RTMP_Free(p_rtmp_handle);
        p_rtmp_handle = NULL;
    }
    if (rtmp_c->body_buf) free(rtmp_c->body_buf);
    memset(rtmp_c, 0, sizeof(GM_RTMP_CLIENT));
    pthread_mutex_unlock(&rtmp_c->mutex);

    free(rtmp_c);
    rtmp_c = NULL;
}

int GM_RtmpClientConnect(void *client, const char* url)
{
    GM_RTMP_CLIENT *rtmp_c = (GM_RTMP_CLIENT *)client;
    RTMP * p_rtmp_handle = (RTMP *)rtmp_c->rtmp_handle;
    int ret = 0;

    if (!rtmp_c || !p_rtmp_handle)
        return ERR_RTMP_CLIENT_NOT_INIT;

    pthread_mutex_lock(&rtmp_c->mutex);
    if (RTMP_SetupURL(p_rtmp_handle,(char*)url) == FALSE)
    {
        ret = ERR_RTMP_SETUP_URL_FAIL;
        goto GM_RtmpClientConnect_end;
    }

    RTMP_EnableWrite(p_rtmp_handle);

    if (RTMP_Connect(p_rtmp_handle, NULL) == FALSE)
    {
        ret = ERR_RTMP_CONNECT_FAIL;
        goto GM_RtmpClientConnect_end;
    }

    if (RTMP_ConnectStream(p_rtmp_handle, 0) == FALSE)
    {
        RTMP_Close(p_rtmp_handle);
        ret = ERR_RTMP_CONNECT_STREAM_FAIL;
        goto GM_RtmpClientConnect_end;
    }
    rtmp_c->timestamp_base = 0;
    rtmp_c->timestamp_latest = 0;
    rtmp_c->timestamp_video_adj = 0;
    rtmp_c->timestamp_audio_adj = 0;
    rtmp_c->video_frames = 0;
    rtmp_c->audio_frames = 0;
    ret = 0;

GM_RtmpClientConnect_end:
    pthread_mutex_unlock(&rtmp_c->mutex);
    return ret;
}

int GM_RtmpClientDisconnect(void *client)
{
    GM_RTMP_CLIENT *rtmp_c = (GM_RTMP_CLIENT *)client;
    RTMP * p_rtmp_handle = (RTMP *)rtmp_c->rtmp_handle;
    int ret = 0;

    if (!rtmp_c || !p_rtmp_handle)
        return ERR_RTMP_CLIENT_NOT_INIT;

    //pthread_mutex_lock(&rtmp_c->mutex);

    //RTMP_Close(p_rtmp_handle);
    RTMP_Close2(p_rtmp_handle);
    ret = 0;

    //pthread_mutex_unlock(&rtmp_c->mutex);
    return ret;
}

int SendH264SpsPps(GM_RTMP_CLIENT *rtmp_c, unsigned char *sps, int sps_len, unsigned char *pps, int pps_len, unsigned int timestamp)
{
    RTMPPacket * packet=NULL;
    unsigned char * body=NULL;
    RTMP * p_rtmp_handle = (RTMP *)rtmp_c->rtmp_handle;
    int i;
#if LIBGMRTMP_DBG_SEND_PACKET_PERF
    struct timeval tv_start, tv_end;
#endif

    if (!rtmp_c || !p_rtmp_handle)
        return ERR_RTMP_CLIENT_NOT_INIT;

    if (rtmp_c->body_buf_size < (int)sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE + sps_len + pps_len + 32 || !rtmp_c->body_buf)
    {
        if (rtmp_c->body_buf) free(rtmp_c->body_buf);

        rtmp_c->body_buf_size = sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE + sps_len + pps_len + 32;
        rtmp_c->body_buf = calloc(rtmp_c->body_buf_size, 1);
        if (!rtmp_c->body_buf)
        {
            rtmp_c->body_buf_size = 0;
            return ERR_RTMP_BODY_BUF_ERROR;
        }
    }

    packet = (RTMPPacket *)rtmp_c->body_buf;
    //memset(packet, 0, rtmp_c->body_buf_size);
    packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
    body = (unsigned char *)packet->m_body;
    i = 0;
    body[i++] = 0x17;
    body[i++] = 0x00;

    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;

    /*AVCDecoderConfigurationRecord*/
    body[i++] = 0x01;
    body[i++] = sps[1];
    body[i++] = sps[2];
    body[i++] = sps[3];
    body[i++] = 0xff;

    /*sps*/
    body[i++]   = 0xe1;
    body[i++] = (sps_len >> 8) & 0xff;
    body[i++] = sps_len & 0xff;
    memcpy(&body[i], sps, sps_len);
    i += sps_len;

    /*pps*/
    body[i++]   = 0x01;
    body[i++] = (pps_len >> 8) & 0xff;
    body[i++] = (pps_len) & 0xff;
    memcpy(&body[i], pps, pps_len);
    i += pps_len;

    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = i;
    packet->m_nChannel = 0x07;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = (rtmp_c->video_frames==0)? RTMP_PACKET_SIZE_LARGE:RTMP_PACKET_SIZE_MEDIUM;
    packet->m_nInfoField2 = 1;//p_rtmp_handle->m_stream_id;

    if (!rtmp_c->timestamp_base)
        rtmp_c->timestamp_base = timestamp;

    //packet->m_nTimeStamp = MAX(timestamp - rtmp_c->timestamp_base, rtmp_c->timestamp_latest);
    packet->m_nTimeStamp = timestamp - rtmp_c->timestamp_base;
    rtmp_c->timestamp_latest = timestamp;

#if LIBGMRTMP_DBG_FRAME_INFO
    printf("%s  timestamp: %d    bodysize: %d\n", __FUNCTION__, packet->m_nTimeStamp, packet->m_nBodySize);
#endif

#if LIBGMRTMP_DBG_SEND_PACKET_PERF
    gettimeofday(&tv_start, NULL);
#endif

    if (RTMP_SendPacket(p_rtmp_handle, packet, TRUE) != TRUE)
        return ERR_RTMP_SEND_PACKET_FAIL;

#if LIBGMRTMP_DBG_SEND_PACKET_PERF
    gettimeofday(&tv_end, NULL);
    printf("%s  SendPacket spend %lu us\n", __FUNCTION__, TIMEVAL_DIFF(tv_start, tv_end));
#endif

    return 0;
}


int SendH264Frame(GM_RTMP_CLIENT *rtmp_c, unsigned char *buf, int len, int keyframe, unsigned int timestamp)
{
    RTMPPacket * packet=NULL;
    unsigned char * body=NULL;
    RTMP * p_rtmp_handle = (RTMP *)rtmp_c->rtmp_handle;
    int i;
#if LIBGMRTMP_DBG_SEND_PACKET_PERF
    struct timeval tv_start, tv_end;
#endif

    if (!rtmp_c || !p_rtmp_handle)
        return ERR_RTMP_CLIENT_NOT_INIT;

    if (rtmp_c->body_buf_size < (int)sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE + len + 32 || !rtmp_c->body_buf)
    {
        if (rtmp_c->body_buf) free(rtmp_c->body_buf);

        rtmp_c->body_buf_size = sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE + len + 32;
        rtmp_c->body_buf = calloc(rtmp_c->body_buf_size, 1);
        if (!rtmp_c->body_buf)
        {
            rtmp_c->body_buf_size = 0;
            return ERR_RTMP_BODY_BUF_ERROR;
        }
    }

    packet = (RTMPPacket *)rtmp_c->body_buf;
    //memset(packet, 0, rtmp_c->body_buf_size);
    packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
    body = (unsigned char *)packet->m_body;
    i = 0;
    body[i++] = keyframe? 0x17 : 0x27;  // 1:Iframe 2:Pframe 7:AVC
    body[i++] = 0x01;// AVC NALU
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;

    // NALU size
    body[i++] = len>>24 & 0xff;
    body[i++] = len>>16 & 0xff;
    body[i++] = len>>8 & 0xff;
    body[i++] = len & 0xff;
    memcpy(&body[i], buf, len);
    i += len;

    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = i;
    packet->m_nChannel = 0x07;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    packet->m_nInfoField2 = p_rtmp_handle->m_stream_id;

    if (!rtmp_c->timestamp_base)
        rtmp_c->timestamp_base = timestamp;

    //packet->m_nTimeStamp = MAX(timestamp - rtmp_c->timestamp_base, rtmp_c->timestamp_latest);
    packet->m_nTimeStamp = timestamp - rtmp_c->timestamp_base;
    rtmp_c->timestamp_latest = timestamp;

#if LIBGMRTMP_DBG_FRAME_INFO
    printf("%s   timestamp: %d    bodysize: %d  %s\n", __FUNCTION__, packet->m_nTimeStamp, packet->m_nBodySize, keyframe?"I":"P");
#endif

#if LIBGMRTMP_DBG_SEND_PACKET_PERF
    gettimeofday(&tv_start, NULL);
#endif

    if (RTMP_SendPacket(p_rtmp_handle, packet, TRUE) != TRUE)
        return ERR_RTMP_SEND_PACKET_FAIL;
    else
        rtmp_c->video_frames++;

#if LIBGMRTMP_DBG_SEND_PACKET_PERF
    gettimeofday(&tv_end, NULL);
    printf("%s   SendPacket spend %lu us\n", __FUNCTION__, TIMEVAL_DIFF(tv_start, tv_end));
#endif

    return 0;
}

int SendAACHeader(GM_RTMP_CLIENT *rtmp_c, unsigned char *buf, int len, unsigned int timestamp)
{
    RTMPPacket * packet=NULL;
    unsigned char * body=NULL;
    RTMP * p_rtmp_handle = (RTMP *)rtmp_c->rtmp_handle;
    int i;
    int config;
#if LIBGMRTMP_DBG_SEND_PACKET_PERF
    struct timeval tv_start, tv_end;
#endif

    if (!rtmp_c || !p_rtmp_handle)
        return ERR_RTMP_CLIENT_NOT_INIT;

    if (rtmp_c->body_buf_size < (int)sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE + len + 32 || !rtmp_c->body_buf)
    {
        if (rtmp_c->body_buf) free(rtmp_c->body_buf);

        rtmp_c->body_buf_size = sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE + len + 32;
        rtmp_c->body_buf = calloc(rtmp_c->body_buf_size, 1);
        if (!rtmp_c->body_buf)
        {
            rtmp_c->body_buf_size = 0;
            return ERR_RTMP_BODY_BUF_ERROR;
        }
    }

    packet = (RTMPPacket *)rtmp_c->body_buf;
    //memset(packet, 0, rtmp_c->body_buf_size);
    packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
    body = (unsigned char *)packet->m_body;
    i = 0;
    body[i++] = 0xAF;
    body[i++] = 0x00;

    config = ((((buf[2] & 0xC0) >> 6) + 1)<<11) +
             (((buf[2] & 0x3C) >> 2)<<7) +
             ((((buf[2] & 0x01) << 2) + ((buf[3] & 0xC0) >> 6))<<3);

    body[i++] = (config >> 8) & 0xFF;
    body[i++] = config & 0xFF;

    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = i;
    packet->m_nChannel = 0x06;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nInfoField2 = 1;//p_rtmp_handle->m_stream_id;

    if (!rtmp_c->timestamp_base)
        rtmp_c->timestamp_base = timestamp;

    //packet->m_nTimeStamp = MAX(timestamp - rtmp_c->timestamp_base, rtmp_c->timestamp_latest);
    packet->m_nTimeStamp = timestamp - rtmp_c->timestamp_base;
    rtmp_c->timestamp_latest = timestamp;

#if LIBGMRTMP_DBG_FRAME_INFO
    printf("%s   timestamp: %d    bodysize: %d\n", __FUNCTION__, packet->m_nTimeStamp, packet->m_nBodySize);
#endif

#if LIBGMRTMP_DBG_SEND_PACKET_PERF
    gettimeofday(&tv_start, NULL);
#endif

    if (RTMP_SendPacket(p_rtmp_handle, packet, TRUE) != TRUE)
        return ERR_RTMP_SEND_PACKET_FAIL;

#if LIBGMRTMP_DBG_SEND_PACKET_PERF
    gettimeofday(&tv_end, NULL);
    printf("%s   SendPacket spend %lu us\n", __FUNCTION__, TIMEVAL_DIFF(tv_start, tv_end));
#endif

    return 0;
}

int SendAACFrame(GM_RTMP_CLIENT *rtmp_c, unsigned char *buf, int len, unsigned int timestamp)
{
    RTMPPacket * packet=NULL;
    unsigned char * body=NULL;
    RTMP * p_rtmp_handle = (RTMP *)rtmp_c->rtmp_handle;
    int i;
#if LIBGMRTMP_DBG_SEND_PACKET_PERF
    struct timeval tv_start, tv_end;
#endif

    if (!rtmp_c || !p_rtmp_handle)
        return ERR_RTMP_CLIENT_NOT_INIT;

    if (rtmp_c->body_buf_size < (int)sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE + len + 32 || !rtmp_c->body_buf)
    {
        if (rtmp_c->body_buf) free(rtmp_c->body_buf);

        rtmp_c->body_buf_size = sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE + len + 32;
        rtmp_c->body_buf = calloc(rtmp_c->body_buf_size, 1);
        if (!rtmp_c->body_buf)
        {
            rtmp_c->body_buf_size = 0;
            return ERR_RTMP_BODY_BUF_ERROR;
        }
    }

    packet = (RTMPPacket *)rtmp_c->body_buf;
    //memset(packet, 0, rtmp_c->body_buf_size);
    packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
    body = (unsigned char *)packet->m_body;
    i = 0;
    body[i++] = 0xAF;
    body[i++] = 0x01;
    memcpy(&body[i], buf, len);
    i += len;

    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = i;
    packet->m_nChannel = 0x06;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    packet->m_nInfoField2 = p_rtmp_handle->m_stream_id;

    if (!rtmp_c->timestamp_base)
        rtmp_c->timestamp_base = timestamp;

    //packet->m_nTimeStamp = MAX(timestamp - rtmp_c->timestamp_base, rtmp_c->timestamp_latest);
    packet->m_nTimeStamp = timestamp - rtmp_c->timestamp_base;
    rtmp_c->timestamp_latest = timestamp;

#if LIBGMRTMP_DBG_FRAME_INFO
    printf("%s    timestamp: %d    bodysize: %d\n", __FUNCTION__, packet->m_nTimeStamp, packet->m_nBodySize);
#endif

#if LIBGMRTMP_DBG_SEND_PACKET_PERF
    gettimeofday(&tv_start, NULL);
#endif

    if (RTMP_SendPacket(p_rtmp_handle, packet, TRUE) != TRUE)
        return ERR_RTMP_SEND_PACKET_FAIL;
    else
        rtmp_c->audio_frames++;

#if LIBGMRTMP_DBG_SEND_PACKET_PERF
    gettimeofday(&tv_end, NULL);
    printf("%s    SendPacket spend %lu us\n", __FUNCTION__, TIMEVAL_DIFF(tv_start, tv_end));
#endif

    return 0;
}

static int rtmp_get_h264_nalu_size(unsigned char *buf, int buf_len)
{
    int i=0;
    while(i++ <= buf_len-4)
    {
        if (buf[i] == 0x00 && buf[i+1] == 0x00 && buf[i+2] == 0x00 && buf[i+3] == 0x01)
        {
            return i;
        }
    }
    return 0;
}

int GM_RtmpClientPsuhH264(void *client, unsigned char *buf, int len, int keyframe, unsigned int timestamp)
{
    int idx=0;
    int sps_len=0, pps_len=0, frame_len=0, skip_nalu_len=0;
    unsigned char *sps_data = NULL, *pps_data = NULL, *frame_data = NULL;
    GM_RTMP_CLIENT *rtmp_c = (GM_RTMP_CLIENT *)client;
    RTMP * p_rtmp_handle = (RTMP *)rtmp_c->rtmp_handle;
    int ret=0;

    if (!rtmp_c || !p_rtmp_handle)
        return ERR_RTMP_CLIENT_NOT_INIT;

    pthread_mutex_lock(&rtmp_c->mutex);

    if (!buf || len < 11)
    {
        ret = ERR_RTMP_PUSH_NULL_DATA;
        goto GM_RtmpClientPsuhH264_end;
    }

    // Adjust timestamp for A/V timestamp disorder issue
    if (timestamp < rtmp_c->timestamp_latest)
    {
        rtmp_c->timestamp_video_adj = MAX(rtmp_c->timestamp_latest - timestamp, rtmp_c->timestamp_video_adj);
#if LIBGMRTMP_DBG_TIMESTAMP_ADJUST
        printf("%s video update timestamp_adj   v: %d    a: %d\n", __FUNCTION__, rtmp_c->timestamp_video_adj, rtmp_c->timestamp_audio_adj);
#endif
    }
    timestamp += rtmp_c->timestamp_video_adj;

    if (rtmp_c->timestamp_latest < 0xFFFFFC17 &&
        timestamp < rtmp_c->timestamp_latest)
    {
        ret = ERR_RTMP_TIMESTAMP_DISORDER;
        goto GM_RtmpClientPsuhH264_end;
    }

    if (RTMP_IsConnected(p_rtmp_handle)!=TRUE)
    {
        ret = ERR_RTMP_DISCONNECTED;
        goto GM_RtmpClientPsuhH264_end;
    }

    if (keyframe)
    {
        if (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x00 && buf[3] == 0x01 && buf[4] == 0x67)     // SPS
        {
            sps_len = rtmp_get_h264_nalu_size(&buf[4], len-4);
            sps_data = buf+idx+4;
            idx += sps_len+4;
        }
        if (buf[idx] == 0x00 && buf[1+idx] == 0x00 && buf[2+idx] == 0x00 && buf[3+idx] == 0x01 && buf[4+idx] == 0x68) // PPS
        {
            pps_len = rtmp_get_h264_nalu_size(&buf[idx+4],len-4-idx);
            pps_data = buf+idx+4;
            idx += pps_len+4;
        }

        if (sps_len || pps_len)
        {
            ret = SendH264SpsPps(rtmp_c, sps_data, sps_len, pps_data, pps_len, timestamp);
        }

        while(1)
        {
            if (buf[idx] == 0x00 && buf[1+idx] == 0x00 && buf[2+idx] == 0x00 && buf[3+idx] == 0x01 && buf[4+idx] != 0x65)
            {
                skip_nalu_len = rtmp_get_h264_nalu_size(&buf[idx+4],len-4-idx);
                idx += skip_nalu_len+4;
            }
            else
            {
                break;
            }
        }

        if (buf[idx] == 0x00 && buf[1+idx] == 0x00 && buf[2+idx] == 0x00 && buf[3+idx] == 0x01 && buf[4+idx] == 0x65) // I-Frame
        {
            frame_len = len-idx-4;
            frame_data = buf+idx+4;

            if (frame_len > 0)
            {
                ret = SendH264Frame(rtmp_c, frame_data, frame_len, keyframe, timestamp);
            }
        }
    }
    else
    {
        if (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x00 && buf[3] == 0x01)
        {
            frame_len = len-idx-4;
            frame_data = buf+idx+4;

            if (frame_len > 0)
            {
                ret = SendH264Frame(rtmp_c, frame_data, frame_len, keyframe, timestamp);
            }
        }
    }

    // Rebalance video_timestamp_adj and audio_timestamp_adj
    if (rtmp_c->timestamp_video_adj && rtmp_c->timestamp_audio_adj)
    {
        if (rtmp_c->timestamp_video_adj > rtmp_c->timestamp_audio_adj)
        {
            rtmp_c->timestamp_video_adj -= rtmp_c->timestamp_audio_adj;
            rtmp_c->timestamp_audio_adj = 0;
        }
        else
        {
            rtmp_c->timestamp_audio_adj -= rtmp_c->timestamp_video_adj;
            rtmp_c->timestamp_video_adj = 0;
        }
#if LIBGMRTMP_DBG_TIMESTAMP_ADJUST
        printf("%s rebalance timestamp_adj   v: %d    a: %d\n", __FUNCTION__, rtmp_c->timestamp_video_adj, rtmp_c->timestamp_audio_adj);
#endif
    }

GM_RtmpClientPsuhH264_end:
    pthread_mutex_unlock(&rtmp_c->mutex);
    return ret;
}

int GM_RtmpClientPsuhAAC(void *client, unsigned char *buf, int len, unsigned int timestamp)
{
    GM_RTMP_CLIENT *rtmp_c = (GM_RTMP_CLIENT *)client;
    RTMP * p_rtmp_handle = (RTMP *)rtmp_c->rtmp_handle;
    int ret=0;

    if (!rtmp_c || !p_rtmp_handle)
        return ERR_RTMP_CLIENT_NOT_INIT;

    pthread_mutex_lock(&rtmp_c->mutex);

    if (!buf || len < 7)
    {
        ret = ERR_RTMP_PUSH_NULL_DATA;
        goto GM_RtmpClientPsuhAAC_end;
    }

    // Adjust timestamp for A/V timestamp disorder issue
    if (timestamp < rtmp_c->timestamp_latest)
    {
        rtmp_c->timestamp_audio_adj = MAX(rtmp_c->timestamp_latest - timestamp, rtmp_c->timestamp_audio_adj);
#if LIBGMRTMP_DBG_TIMESTAMP_ADJUST
        printf("%s audio update timestamp_adj   v: %d    a: %d\n", __FUNCTION__, rtmp_c->timestamp_video_adj, rtmp_c->timestamp_audio_adj);
#endif
    }
    timestamp += rtmp_c->timestamp_audio_adj;

    if (rtmp_c->timestamp_latest < 0xFFFFFC17 &&
        timestamp < rtmp_c->timestamp_latest)
    {
        ret = ERR_RTMP_TIMESTAMP_DISORDER;
        goto GM_RtmpClientPsuhAAC_end;
    }

    if (RTMP_IsConnected(p_rtmp_handle)!=TRUE)
    {
        ret = ERR_RTMP_DISCONNECTED;
        goto GM_RtmpClientPsuhAAC_end;
    }

    if (buf[0] == 0xFF && (buf[1]&0xF0) == 0xF0)
    {
        if (rtmp_c->audio_frames%100 == 0)
        {
            SendAACHeader(rtmp_c, buf, len, timestamp);
        }
        ret = SendAACFrame(rtmp_c, buf+7, len-7, timestamp);
    }
    else
    {
        ret = SendAACFrame(rtmp_c, buf, len, timestamp);
    }

GM_RtmpClientPsuhAAC_end:
    pthread_mutex_unlock(&rtmp_c->mutex);
    return ret;
}

static int SendConnectResult(RTMP *r, double txn)
{
    RTMPPacket packet;
    char pbuf[384], *pend = pbuf + sizeof(pbuf);
    AVal av;

    packet.m_nChannel = 0x03;
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av__result);
    enc = AMF_EncodeNumber(enc, pend, txn);
    *enc++ = AMF_OBJECT;

    STR2AVAL(av, "FMS/3,5,1,525");
    enc = AMF_EncodeNamedString(enc, pend, &av_fmsVer, &av);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_capabilities, 31.0);
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    *enc++ = AMF_OBJECT;

    STR2AVAL(av, "status");
    enc = AMF_EncodeNamedString(enc, pend, &av_level, &av);
    STR2AVAL(av, "NetConnection.Connect.Success");
    enc = AMF_EncodeNamedString(enc, pend, &av_code, &av);
    STR2AVAL(av, "Connection succeeded.");
    enc = AMF_EncodeNamedString(enc, pend, &av_description, &av);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_objectEncoding, r->m_fEncoding);
#if 0
    STR2AVAL(av, "58656322c972d6cdf2d776167575045f8484ea888e31c086f7b5ffbd0baec55ce442c2fb");
    enc = AMF_EncodeNamedString(enc, pend, &av_secureToken, &av);
#endif
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    packet.m_nBodySize = enc - packet.m_body;

    return RTMP_SendPacket(r, &packet, FALSE);
}

static int SendResultNumber(RTMP *r, double txn, double ID)
{
    RTMPPacket packet;
    char pbuf[256], *pend = pbuf + sizeof(pbuf);

    packet.m_nChannel = 0x03;
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av__result);
    enc = AMF_EncodeNumber(enc, pend, txn);
    *enc++ = AMF_NULL;
    enc = AMF_EncodeNumber(enc, pend, ID);

    packet.m_nBodySize = enc - packet.m_body;

    return RTMP_SendPacket(r, &packet, FALSE);
}

int SendPlayStart(RTMP *r)
{
    RTMPPacket packet;
    char pbuf[512], *pend = pbuf + sizeof(pbuf);

    packet.m_nChannel = 0x05;
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 1;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av_onStatus);
    enc = AMF_EncodeNumber(enc, pend, 0);
    *enc++ = AMF_NULL;
    *enc++ = AMF_OBJECT;

    enc = AMF_EncodeNamedString(enc, pend, &av_level, &av_status);
    enc = AMF_EncodeNamedString(enc, pend, &av_code, &av_NetStream_Play_Start);
    enc = AMF_EncodeNamedString(enc, pend, &av_description, &av_Started_playing);
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    packet.m_nBodySize = enc - packet.m_body;
    return RTMP_SendPacket(r, &packet, TRUE);
}

int SendSampleAccess(RTMP *r)
{
    RTMPPacket packet;
    char pbuf[512], *pend = pbuf + sizeof(pbuf);

    packet.m_nChannel = 0x05;
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet.m_packetType = RTMP_PACKET_TYPE_INFO;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 1;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av_RtmpSampleAccess);
    enc = AMF_EncodeBoolean(enc, pend, 1);
    enc = AMF_EncodeBoolean(enc, pend, 1);

    packet.m_nBodySize = enc - packet.m_body;
    return RTMP_SendPacket(r, &packet, FALSE);
}

int SendPlayStop(RTMP *r)
{
    RTMPPacket packet;
    char pbuf[512], *pend = pbuf + sizeof(pbuf);

    packet.m_nChannel = 0x05;
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av_onStatus);
    enc = AMF_EncodeNumber(enc, pend, 0);
    *enc++ = AMF_NULL;
    *enc++ = AMF_OBJECT;

    enc = AMF_EncodeNamedString(enc, pend, &av_level, &av_status);
    enc = AMF_EncodeNamedString(enc, pend, &av_code, &av_NetStream_Play_Stop);
    enc = AMF_EncodeNamedString(enc, pend, &av_description, &av_Stopped_playing);
    enc = AMF_EncodeNamedString(enc, pend, &av_details, &r->Link.playpath);
    enc = AMF_EncodeNamedString(enc, pend, &av_clientid, &av_clientid);
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    packet.m_nBodySize = enc - packet.m_body;
    return RTMP_SendPacket(r, &packet, FALSE);
}

int SendPlayPause(RTMP *r)
{
    RTMPPacket packet;
    char pbuf[512], *pend = pbuf + sizeof(pbuf);

    packet.m_nChannel = 0x05;
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av_onStatus);
    enc = AMF_EncodeNumber(enc, pend, 0);
    *enc++ = AMF_NULL;
    *enc++ = AMF_OBJECT;

    enc = AMF_EncodeNamedString(enc, pend, &av_level, &av_status);
    enc = AMF_EncodeNamedString(enc, pend, &av_code, &av_NetStream_Pause_Notify);
    enc = AMF_EncodeNamedString(enc, pend, &av_description, &av_Paused_live);
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    packet.m_nBodySize = enc - packet.m_body;
    return RTMP_SendPacket(r, &packet, TRUE);
}

int SendPlayUnpause(RTMP *r)
{
    RTMPPacket packet;
    char pbuf[512], *pend = pbuf + sizeof(pbuf);

    packet.m_nChannel = 0x05;
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av_onStatus);
    enc = AMF_EncodeNumber(enc, pend, 0);
    *enc++ = AMF_NULL;
    *enc++ = AMF_OBJECT;

    enc = AMF_EncodeNamedString(enc, pend, &av_level, &av_status);
    enc = AMF_EncodeNamedString(enc, pend, &av_code, &av_NetStream_Unpause_Notify);
    enc = AMF_EncodeNamedString(enc, pend, &av_description, &av_Unpaused_live);
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    packet.m_nBodySize = enc - packet.m_body;
    return RTMP_SendPacket(r, &packet, TRUE);
}

int RTMP_SendChunkSize(RTMP *r)
{
    RTMPPacket packet;
    char pbuf[256], *pend = pbuf + sizeof(pbuf);

    packet.m_nChannel = 0x02;
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet.m_packetType = RTMP_PACKET_TYPE_CHUNK_SIZE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    packet.m_nBodySize = 4;

    AMF_EncodeInt32(packet.m_body, pend, r->m_outChunkSize);
    return RTMP_SendPacket(r, &packet, FALSE);
}

int countAMF(AMFObject *obj, int *argc)
{
    int i, len;

    for (i = 0, len = 0; i < obj->o_num; i++)
    {
        AMFObjectProperty *p = &obj->o_props[i];
        len += 4;
        (*argc) += 2;
        if (p->p_name.av_val)
            len += 1;
        len += 2;
        if (p->p_name.av_val)
            len += p->p_name.av_len + 1;
        switch (p->p_type)
        {
        case AMF_BOOLEAN:
            len += 1;
            break;
        case AMF_STRING:
            len += p->p_vu.p_aval.av_len;
            break;
        case AMF_NUMBER:
            len += 40;
            break;
        case AMF_OBJECT:
            len += 9;
            len += countAMF(&p->p_vu.p_object, argc);
            (*argc) += 2;
            break;
        case AMF_NULL:
        default:
            break;
        }
    }
    return len;
}

char *dumpAMF(AMFObject *obj, char *ptr, AVal *argv, int *argc)
{
    int i, ac = *argc;
    const char opt[] = "NBSO Z";

    for (i = 0; i < obj->o_num; i++)
    {
        AMFObjectProperty *p = &obj->o_props[i];
        argv[ac].av_val = ptr + 1;
        argv[ac++].av_len = 2;
        ptr += sprintf(ptr, " -C ");
        argv[ac].av_val = ptr;
        if (p->p_name.av_val)
            *ptr++ = 'N';
        *ptr++ = opt[p->p_type];
        *ptr++ = ':';
        if (p->p_name.av_val)
            ptr += sprintf(ptr, "%.*s:", p->p_name.av_len, p->p_name.av_val);
        switch (p->p_type)
        {
        case AMF_BOOLEAN:
            *ptr++ = p->p_vu.p_number != 0 ? '1' : '0';
            argv[ac].av_len = ptr - argv[ac].av_val;
            break;
        case AMF_STRING:
            memcpy(ptr, p->p_vu.p_aval.av_val, p->p_vu.p_aval.av_len);
            ptr += p->p_vu.p_aval.av_len;
            argv[ac].av_len = ptr - argv[ac].av_val;
            break;
        case AMF_NUMBER:
            ptr += sprintf(ptr, "%f", p->p_vu.p_number);
            argv[ac].av_len = ptr - argv[ac].av_val;
            break;
        case AMF_OBJECT:
            *ptr++ = '1';
            argv[ac].av_len = ptr - argv[ac].av_val;
            ac++;
            *argc = ac;
            ptr = dumpAMF(&p->p_vu.p_object, ptr, argv, argc);
            ac = *argc;
            argv[ac].av_val = ptr + 1;
            argv[ac++].av_len = 2;
            argv[ac].av_val = ptr + 4;
            argv[ac].av_len = 3;
            ptr += sprintf(ptr, " -C O:0");
            break;
        case AMF_NULL:
        default:
            argv[ac].av_len = ptr - argv[ac].av_val;
            break;
        }
        ac++;
    }
    *argc = ac;
    return ptr;
}

void AVreplace(AVal *src, const AVal *orig, const AVal *repl)
{
    char *srcbeg = src->av_val;
    char *srcend = src->av_val + src->av_len;
    char *dest, *sptr, *dptr;
    int n = 0;

    /* count occurrences of orig in src */
    sptr = src->av_val;
    while (sptr < srcend && (sptr = strstr(sptr, orig->av_val)))
    {
        n++;
        sptr += orig->av_len;
    }
    if (!n)
        return;

    if ((dest = malloc(src->av_len + 1 + (repl->av_len - orig->av_len) * n)) == NULL)
        return;

    sptr = src->av_val;
    dptr = dest;
    while (sptr < srcend && (sptr = strstr(sptr, orig->av_val)))
    {
        n = sptr - srcbeg;
        memcpy(dptr, srcbeg, n);
        dptr += n;
        memcpy(dptr, repl->av_val, repl->av_len);
        dptr += repl->av_len;
        sptr += orig->av_len;
        srcbeg = sptr;
    }
    n = srcend - srcbeg;
    memcpy(dptr, srcbeg, n);
    dptr += n;
    *dptr = '\0';
    src->av_val = dest;
    src->av_len = dptr - dest;
}

int find_stream_id(STREAMING_SERVER *server, char *stream_url, char *stream_key)
{
    int i;
    char * pch;
    int url_len, key_len;

    for (i=0; i<MAX_RTMP_SERVER_STREAM; i++)
    {
        pch = strchr(stream_url, '/');
        pch = strchr(pch+1,'/');
        pch = strchr(pch+1,'/');
        if (!pch)
            continue;

        url_len = (*(pch+strlen(pch+1))=='/')?strlen(pch+1)-1:strlen(pch+1);
        key_len = strlen(stream_key);

        if (server->stream[i].used &&
            (int)strlen(server->stream[i].stream_url) == url_len &&
            strncmp(server->stream[i].stream_url, pch+1, url_len) == 0 &&
            (int)strlen(server->stream[i].stream_key) == key_len &&
            strncmp(server->stream[i].stream_key, stream_key, key_len) == 0)
        {
            break;
        }
    }

    if (i < MAX_RTMP_SERVER_STREAM)
        return i;
    else
        return -1;
}

int find_stream_url_exist(STREAMING_SERVER *server, char *stream_url)
{
    int i;
    char * pch;
    int url_len;

    for (i=0; i<MAX_RTMP_SERVER_STREAM; i++)
    {
        pch = strchr(stream_url, '/');
        pch = strchr(pch+1,'/');
        pch = strchr(pch+1,'/');
        if (!pch)
            continue;

        url_len = (*(pch+strlen(pch+1))=='/')?strlen(pch+1)-1:strlen(pch+1);

        if (server->stream[i].used &&
            (int)strlen(server->stream[i].stream_url) == url_len &&
            strncmp(server->stream[i].stream_url, pch+1, url_len) == 0)
        {
            break;
        }
    }

    if (i < MAX_RTMP_SERVER_STREAM)
        return i;
    else
        return -1;
}

int occupy_stream(STREAMING_SERVER *server)
{
    int i;

    for (i=0; i<MAX_RTMP_SERVER_STREAM; i++)
    {
        if (!server->stream[i].used)
        {
            server->stream[i].used = 1;
            break;
        }
    }

    if (i < MAX_RTMP_SERVER_STREAM)
        return i;
    else
        return -1;
}

void release_stream(STREAMING_SERVER *server, STREAM_INFO *stream)
{
    if (stream && stream->used)
    {
        stream->used = 0;
        if (stream->buf)
            free(stream->buf);
        stream->buf_size = 0;
    }
}

CONNECTION_INFO *occupy_connection(STREAMING_SERVER *server, int fd)
{
    int i;
    CONNECTION_INFO *conn = NULL;

    for (i=0; i<MAX_RTMP_SERVER_CONNECT; i++)
    {
        if (!server->connection[i].connected)
        {
            server->connection[i].connected = 1;
            server->connection[i].client_fd = fd;
            server->connection[i].rtmp_client = GM_RtmpClientInit(128*1024);
            server->connection[i].stream_id = -1;
            server->connection[i].playing = 0;
            server->connection[i].I_frame_arrival = 0;
            server->connection[i].private = (void *)server;
            conn = &server->connection[i];
            break;
        }
    }

    return conn;
}

void release_connection(CONNECTION_INFO *connection)
{
    if (connection && connection->connected)
    {
        connection->connected = 0;
        connection->client_fd = -1;
        GM_RtmpClientDestroy(connection->rtmp_client);
        connection->stream_id = -1;
        connection->playing = 0;
        connection->I_frame_arrival = 0;
        connection->private = NULL;
    }

    return;
}

// Returns 0 for OK/Failed/error, 1 for 'Stop or Complete'
int ServeInvoke(CONNECTION_INFO *conn, RTMP * r, RTMPPacket *packet, unsigned int offset)
{
    const char *body;
    unsigned int nBodySize;
    int ret = 0, nRes;
    STREAMING_SERVER *server = NULL;

    //printf("%s, received packet type %02X, size %u bytes\n", __FUNCTION__, packet->m_packetType, packet->m_nBodySize);

    if (conn && conn->private)
        server = (STREAMING_SERVER *)conn->private;

    body = packet->m_body + offset;
    nBodySize = packet->m_nBodySize - offset;

    if (body[0] != 0x02) // make sure it is a string method name we start with
    {
        RTMP_Log(RTMP_LOGWARNING, "%s, Sanity failed. no string method in invoke packet", __FUNCTION__);
        return 0;
    }

    AMFObject obj;
    nRes = AMF_Decode(&obj, body, nBodySize, FALSE);
    if (nRes < 0)
    {
        RTMP_Log(RTMP_LOGERROR, "%s, error decoding invoke packet", __FUNCTION__);
        return 0;
    }

    AMF_Dump(&obj);
    AVal method;
    AMFProp_GetString(AMF_GetProp(&obj, NULL, 0), &method);
    double txn = AMFProp_GetNumber(AMF_GetProp(&obj, NULL, 1));
    RTMP_Log(RTMP_LOGDEBUG, "%s, client invoking <%s>", __FUNCTION__, method.av_val);

    if (AVMATCH(&method, &av_connect))
    {
        AMFObject cobj;
        AVal pname, pval;
        int i;

        conn->connect = packet->m_body;
        packet->m_body = NULL;

        AMFProp_GetObject(AMF_GetProp(&obj, NULL, 2), &cobj);
        for (i = 0; i < cobj.o_num; i++)
        {
            pname = cobj.o_props[i].p_name;
            pval.av_val = NULL;
            pval.av_len = 0;
            if (cobj.o_props[i].p_type == AMF_STRING)
                pval = cobj.o_props[i].p_vu.p_aval;
            if (AVMATCH(&pname, &av_app))
            {
                r->Link.app = pval;
                pval.av_val = NULL;
                if (!r->Link.app.av_val)
                    r->Link.app.av_val = "";
            }
            else if (AVMATCH(&pname, &av_flashVer))
            {
                r->Link.flashVer = pval;
                pval.av_val = NULL;
            }
            else if (AVMATCH(&pname, &av_swfUrl))
            {
                r->Link.swfUrl = pval;
                pval.av_val = NULL;
            }
            else if (AVMATCH(&pname, &av_tcUrl))
            {
                r->Link.tcUrl = pval;
                pval.av_val = NULL;
            }
            else if (AVMATCH(&pname, &av_pageUrl))
            {
                r->Link.pageUrl = pval;
                pval.av_val = NULL;
            }
            else if (AVMATCH(&pname, &av_audioCodecs))
            {
                r->m_fAudioCodecs = cobj.o_props[i].p_vu.p_number;
            }
            else if (AVMATCH(&pname, &av_videoCodecs))
            {
                r->m_fVideoCodecs = cobj.o_props[i].p_vu.p_number;
            }
            else if (AVMATCH(&pname, &av_objectEncoding))
            {
                r->m_fEncoding = cobj.o_props[i].p_vu.p_number;
            }
        }
        /* Still have more parameters? Copy them */
        if (obj.o_num > 3)
        {
            int i = obj.o_num - 3;
            r->Link.extras.o_num = i;
            if ((r->Link.extras.o_props = malloc(i * sizeof(AMFObjectProperty))) == NULL)
                return 1;
            memcpy(r->Link.extras.o_props, obj.o_props + 3, i * sizeof(AMFObjectProperty));
            obj.o_num = 3;
        }

        AMFProp_GetString(AMF_GetProp(&obj, NULL, 3), &r->Link.playpath);
        if (find_stream_url_exist(server, r->Link.tcUrl.av_val) < 0)
            return -1;

        r->m_nServerBW = 5000000;
        RTMP_SendServerBW(r);
        r->m_nClientBW = 5000000;
        RTMP_SendClientBW(r);
        r->m_outChunkSize = 4096;
        RTMP_SendChunkSize(r);

        SendConnectResult(r, txn);
    }
    else if (AVMATCH(&method, &av_createStream))
    {
        SendResultNumber(r, txn, 1/*++server->streamID*/);
    }
    else if (AVMATCH(&method, &av_getStreamLength))
    {
        //SendResultNumber(r, txn, 10.0);
    }
    else if (AVMATCH(&method, &av_NetStream_Authenticate_UsherToken))
    {
        AVal usherToken;
        AMFProp_GetString(AMF_GetProp(&obj, NULL, 3), &usherToken);
        AVreplace(&usherToken, &av_dquote, &av_escdquote);
        r->Link.usherToken = usherToken;
    }
    else if (AVMATCH(&method, &av_play))
    {
        RTMPPacket pc = {0};

        AMFProp_GetString(AMF_GetProp(&obj, NULL, 3), &r->Link.playpath);

        /*printf("av_play: hostname  %s\n", r->Link.hostname.av_val);
        printf("av_play: sockshost %s\n", r->Link.sockshost.av_val);
        printf("av_play: playpath0 %s\n", r->Link.playpath0.av_val);
        printf("av_play: playpath  %s\n", r->Link.playpath.av_val);
        printf("av_play: tcUrl     %s\n", r->Link.tcUrl.av_val);
        printf("av_play: swfUrl    %s\n", r->Link.swfUrl.av_val);
        printf("av_play: pageUrl   %s\n", r->Link.pageUrl.av_val);*/

        printf("RTMP Server: Client connect to %s/%s\n", r->Link.tcUrl.av_val, r->Link.playpath.av_val);

        if ((conn->stream_id = find_stream_id(server, r->Link.tcUrl.av_val, r->Link.playpath.av_val)) < 0)
            return -1;

        pc.m_body = conn->connect;
        conn->connect = NULL;
        RTMPPacket_Free(&pc);
        ret = 0;
        RTMP_SendCtrl(r, 0, 1, 0);
        SendPlayStart(r);
        SendSampleAccess(r);

        conn->playing = 1;
        conn->I_frame_arrival = 0;
    }
    else if (AVMATCH(&method, &av_pause))
    {
        if (conn->playing)
        {
            SendPlayPause(r);
            RTMP_SendCtrl(r, 1, 1, 0);
            conn->playing = 0;
            conn->I_frame_arrival = 0;
        }
        else
        {
            SendPlayUnpause(r);
            RTMP_SendCtrl(r, 0, 1, 0);
            conn->playing = 1;
            conn->I_frame_arrival = 0;
        }


    }
    AMF_Reset(&obj);
    return ret;
}

int ServePacket(CONNECTION_INFO *conn, RTMP *r, RTMPPacket *packet)
{
    int ret = 0;

    RTMP_Log(RTMP_LOGDEBUG, "%s, received packet type %02X, size %u bytes", __FUNCTION__, packet->m_packetType, packet->m_nBodySize);
    //printf("%s, received packet type %02X, size %u bytes\n", __FUNCTION__, packet->m_packetType, packet->m_nBodySize);

    switch (packet->m_packetType)
    {
    case RTMP_PACKET_TYPE_CHUNK_SIZE:
        //HandleChangeChunkSize(r, packet);
        break;

    case RTMP_PACKET_TYPE_BYTES_READ_REPORT:
        break;

    case RTMP_PACKET_TYPE_CONTROL:
        //HandleCtrl(r, packet);
        break;

    case RTMP_PACKET_TYPE_SERVER_BW:
        //HandleServerBW(r, packet);
        break;

    case RTMP_PACKET_TYPE_CLIENT_BW:
        //HandleClientBW(r, packet);
        break;

    case RTMP_PACKET_TYPE_AUDIO:
        //RTMP_Log(RTMP_LOGDEBUG, "%s, received: audio %lu bytes", __FUNCTION__, packet.m_nBodySize);
        break;

    case RTMP_PACKET_TYPE_VIDEO:
        //RTMP_Log(RTMP_LOGDEBUG, "%s, received: video %lu bytes", __FUNCTION__, packet.m_nBodySize);
        break;

    case RTMP_PACKET_TYPE_FLEX_STREAM_SEND:
        break;

    case RTMP_PACKET_TYPE_FLEX_SHARED_OBJECT:
        break;

    case RTMP_PACKET_TYPE_FLEX_MESSAGE:
    {
        RTMP_Log(RTMP_LOGDEBUG, "%s, flex message, size %u bytes, not fully supported", __FUNCTION__, packet->m_nBodySize);
        //RTMP_LogHex(packet.m_body, packet.m_nBodySize);

        // some DEBUG code
        /*RTMP_LIB_AMFObject obj;
         int nRes = obj.Decode(packet.m_body+1, packet.m_nBodySize-1);
         if(nRes < 0) {
         RTMP_Log(RTMP_LOGERROR, "%s, error decoding AMF3 packet", __FUNCTION__);
         //return;
         }

         obj.Dump(); */

        if (ServeInvoke(conn, r, packet, 1))
        {
            //RTMP_Close(r);
            ret = -1;
        }
        break;
    }
    case RTMP_PACKET_TYPE_INFO:
        break;

    case RTMP_PACKET_TYPE_SHARED_OBJECT:
        break;

    case RTMP_PACKET_TYPE_INVOKE:
        RTMP_Log(RTMP_LOGDEBUG, "%s, received: invoke %u bytes", __FUNCTION__, packet->m_nBodySize);
        //RTMP_LogHex(packet.m_body, packet.m_nBodySize);

        if (ServeInvoke(conn, r, packet, 0))
        {
            //RTMP_Close(r);
            ret = -1;
        }
        break;

    case RTMP_PACKET_TYPE_FLASH_VIDEO:
        break;
    default:
        RTMP_Log(RTMP_LOGDEBUG, "%s, unknown packet type received: 0x%02x", __FUNCTION__, packet->m_packetType);
#ifdef _DEBUG
        RTMP_LogHex(RTMP_LOGDEBUG, packet->m_body, packet->m_nBodySize);
#endif
    }
    return ret;
}

static void set_tcp_nodelay(int fd)
{
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable));
}

static void *rtmp_do_service(void *arg)
{
    RTMP *rtmp;
    CONNECTION_INFO *conn = (CONNECTION_INFO *)arg;
    STREAMING_SERVER *server = NULL;

    pthread_detach(pthread_self());
    NAMED_THREAD();

    if (conn && conn->private)
        server = (STREAMING_SERVER *)conn->private;

    rtmp = (RTMP *)conn->rtmp_client->rtmp_handle;
    RTMPPacket packet = {0};

    // timeout for http requests
    fd_set fds;
    struct timeval tv;

    memset(&tv, 0, sizeof(struct timeval));
    tv.tv_sec = 5;

    FD_ZERO(&fds);
    FD_SET(conn->client_fd, &fds);

    conn->rtmp_client->video_frames = 0;

    if (select(conn->client_fd + 1, &fds, NULL, NULL, &tv) <= 0)
    {
        RTMP_Log(RTMP_LOGERROR, "Request timeout/select failed, ignoring request");
        printf("rtmp_do_service end\n");
        return NULL;
    }
    else
    {
        rtmp->m_sb.sb_socket = conn->client_fd;
        if (server->sslCtx && !RTMP_TLS_Accept(rtmp, server->sslCtx))
        {
            RTMP_Log(RTMP_LOGERROR, "TLS handshake failed");
            goto cleanup;
        }
        if (!RTMP_Serve(rtmp))
        {
            RTMP_Log(RTMP_LOGERROR, "Handshake failed");
            goto cleanup;
        }
    }

    //server->arglen = 0;
    while (RTMP_IsConnected(rtmp) && RTMP_ReadPacket_without_close(rtmp, &packet))
    {
        //printf("RTMP_ReadPacket read result: %d %d\n", packet.m_nBytesRead, packet.m_nBodySize);
        if (!RTMPPacket_IsReady(&packet))
            continue;
        if (ServePacket(conn, rtmp, &packet) != 0)
        {
            RTMPPacket_Free(&packet);
            break;
        }
        RTMPPacket_Free(&packet);
    }

cleanup:
    //RTMP_LogPrintf("Closing connection... ");
    pthread_mutex_lock(&conn->lock);
    rtmp->Link.playpath.av_val = NULL;
    rtmp->Link.tcUrl.av_val = NULL;
    rtmp->Link.swfUrl.av_val = NULL;
    rtmp->Link.pageUrl.av_val = NULL;
    rtmp->Link.app.av_val = NULL;
    rtmp->Link.flashVer.av_val = NULL;
    if (rtmp->Link.usherToken.av_val)
    {
        free(rtmp->Link.usherToken.av_val);
        rtmp->Link.usherToken.av_val = NULL;
    }
    //RTMP_LogPrintf("done!\n\n");

    release_connection(conn);
    pthread_mutex_unlock(&conn->lock);

    printf("RTMP Server: Client disconnect\n");

    return NULL;
}

void *rtmp_server(void *arg)
{
    STREAMING_SERVER *server = arg;
    CONNECTION_INFO *conn;
    pthread_t rtmp_do_service_id;
    struct sockaddr_in addr;
    int sockfd;

    NAMED_THREAD();

    while (1)
    {
        socklen_t addrlen = sizeof(struct sockaddr_in);
        sockfd = accept(server->server_fd, (struct sockaddr *) &addr, &addrlen);

        if (sockfd > 0)
        {
            set_tcp_nodelay(sockfd);

            pthread_mutex_lock(&server->lock);
            conn = occupy_connection(server, sockfd);
            pthread_mutex_unlock(&server->lock);
            if (conn != NULL)
            {
                /* Create a new thread and transfer the control to that */
                if (pthread_create(&rtmp_do_service_id, NULL, rtmp_do_service, conn) != 0)
                {
                    printf("pthread_create rtmp_do_service fail\n");
                    close(sockfd);
                }
            }
            else
            {
                printf("RTMP Server: too many connetcion (max: %d)\n", MAX_RTMP_SERVER_CONNECT);
                close(sockfd);
            }
        }
        else
        {
            printf("RTMP Server: %s: accept failed", __FUNCTION__);
        }
    }

    return 0;
}

STREAMING_SERVER *start_rtmp_server(int port)
{
    struct sockaddr_in addr;
    int sockfd = -1, tmp;
    STREAMING_SERVER *server;
    char *cert = NULL, *key = NULL;
    pthread_t rtmp_server_id;
    int i;

    if ((server = (STREAMING_SERVER *)calloc(1, sizeof(STREAMING_SERVER))) == NULL)
        goto start_rtmp_server_err;

    pthread_mutex_init(&server->lock, NULL);
    for (i=0; i<MAX_RTMP_SERVER_STREAM; i++)
        pthread_mutex_init(&server->stream[i].lock, NULL);
    for (i=0; i<MAX_RTMP_SERVER_CONNECT; i++)
        pthread_mutex_init(&server->connection[i].lock, NULL);

    if (cert && key)
        server->sslCtx = RTMP_TLS_AllocServerContext(cert, key);

    sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd == -1)
    {
        printf("%s couldn't create socket", __FUNCTION__);
        goto start_rtmp_server_err;
    }

    tmp = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&tmp, sizeof(tmp));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
    {
        printf("%s TCP bind failed for port number: %d", __FUNCTION__, port);
        goto start_rtmp_server_err;
    }

    if (listen(sockfd, 10) == -1)
    {
        printf("%s listen failed\n", __FUNCTION__);
        goto start_rtmp_server_err;
    }

    server->server_fd = sockfd;

    if (pthread_create(&rtmp_server_id, NULL, rtmp_server, server) != 0)
    {
        printf("RTMP server rtmp_server ctreat fail\n");
        goto start_rtmp_server_err;
    }

    return server;

start_rtmp_server_err:

    if (server)
        free(server);
    if (sockfd >= 0)
        close(sockfd);

    return NULL;
}

void *GM_RtmpServerInit()
{
    //RTMP_debuglevel = RTMP_LOGALL;

    return (void *)start_rtmp_server(1935);
}

void GM_RtmpServerDestroy(void *server)
{
    STREAMING_SERVER *s = (STREAMING_SERVER *)server;

    if (s)
    {
        if (s->sslCtx)
            RTMP_TLS_FreeServerContext(s->sslCtx);
        if (s->server_fd >= 0)
            close(s->server_fd);
        free(s);
    }
}

int GM_RtmpServerCreateStream(void *server, char *stream_url, char *stream_key, int buf_size)
{
    STREAMING_SERVER *s = (STREAMING_SERVER *)server;
    int stream_id = -1;

    pthread_mutex_lock(&s->lock);

    if ((stream_id = occupy_stream(server)) < 0)
    {
        printf("RTMP Server: too many stream (max: %d)\n", MAX_RTMP_SERVER_STREAM);
        goto GM_RTMP_Server_add_stream_err;
    }

    s->stream[stream_id].buf_size = buf_size;
    if (buf_size)
    {
        if ((s->stream[stream_id].buf = malloc(buf_size)) == NULL)
        {
            printf("RTMP Server: add_stream alloc fail\n");
            goto GM_RTMP_Server_add_stream_err;
        }
    }

    strncpy(s->stream[stream_id].stream_url, stream_url, sizeof(s->stream[stream_id].stream_url));
    strncpy(s->stream[stream_id].stream_key, stream_key, sizeof(s->stream[stream_id].stream_key));

    pthread_mutex_unlock(&s->lock);
    return stream_id;

GM_RTMP_Server_add_stream_err:
    if (stream_id >=0 && s->stream[stream_id].buf)
        free(s->stream[stream_id].buf);
    s->stream[stream_id].buf_size = 0;

    pthread_mutex_unlock(&s->lock);

    return -1;
}

int GM_RtmpServerSnedH264(void *server, int stream_id, unsigned char *buf, int len, int keyframe, unsigned int timestamp)
{
    STREAMING_SERVER *s = (STREAMING_SERVER *)server;
    int i, ret = 0;
    unsigned char *send_buf;

    if (!server ||
        stream_id < 0 ||
        stream_id >= MAX_RTMP_SERVER_STREAM ||
        !buf ||
        !len ||
        !s->stream[stream_id].used)
    {
        return ERR_RTMP_PARAMETER_ERROR;
    }

    pthread_mutex_lock(&s->stream[stream_id].lock);

    if (s->stream[stream_id].buf_size && s->stream[stream_id].buf)
    {
        if (s->stream[stream_id].buf_size > len)
        {
            memcpy(s->stream[stream_id].buf, buf, len);
            send_buf = (unsigned char *)s->stream[stream_id].buf;
        }
        else
        {
            printf("RTMP Server: stream %d buf not enough (frame: %d  total: %d)\n", stream_id, len, s->stream[stream_id].buf_size);
            send_buf = buf;
        }
    }
    else
    {
        send_buf = buf;
    }

    for (i=0; i<MAX_RTMP_SERVER_CONNECT; i++)
    {
        if (s->connection[i].connected && s->connection[i].stream_id == stream_id)
        {
            if (s->connection[i].playing && !s->connection[i].I_frame_arrival && keyframe)
                s->connection[i].I_frame_arrival = 1;

            if (s->connection[i].playing && s->connection[i].I_frame_arrival)
            {
                //printf("call GM_RtmpClientPsuhH264 start\n");
                pthread_mutex_lock(&s->connection[i].lock);
                if ((ret = GM_RtmpClientPsuhH264(s->connection[i].rtmp_client, send_buf, len, keyframe, timestamp)) != 0)
                {
                    printf("RTMP Server: stream %d GM_RtmpClientPsuhH264 Fail (%d)\n", stream_id, ret);
                }
                pthread_mutex_unlock(&s->connection[i].lock);
                //printf("call GM_RtmpClientPsuhH264 end\n");
            }
        }
    }

    pthread_mutex_unlock(&s->stream[stream_id].lock);

    return ret;
}

int GM_RtmpServerSnedAAC(void *server, int stream_id, unsigned char *buf, int len, unsigned int timestamp)
{
    STREAMING_SERVER *s = (STREAMING_SERVER *)server;
    int i, ret = 0;
    unsigned char *send_buf;

    if (!server ||
        stream_id < 0 ||
        stream_id >= MAX_RTMP_SERVER_STREAM ||
        !buf ||
        !len ||
        !s->stream[stream_id].used)
    {
        return ERR_RTMP_PARAMETER_ERROR;
    }

    pthread_mutex_lock(&s->stream[stream_id].lock);

    if (s->stream[stream_id].buf_size && s->stream[stream_id].buf)
    {
        if (s->stream[stream_id].buf_size > len)
        {
            memcpy(s->stream[stream_id].buf, buf, len);
            send_buf = (unsigned char *)s->stream[stream_id].buf;
        }
        else
        {
            printf("RTMP Server: stream %d buf not enough (frame: %d  total: %d)\n", stream_id, len, s->stream[stream_id].buf_size);
            send_buf = buf;
        }
    }
    else
    {
        send_buf = buf;
    }

    for (i=0; i<MAX_RTMP_SERVER_CONNECT; i++)
    {
        if (s->connection[i].connected && s->connection[i].stream_id == stream_id)
        {
            if (s->connection[i].playing && s->connection[i].I_frame_arrival)
            {
                //printf("call GM_RtmpClientPsuhH264 start\n");
                pthread_mutex_lock(&s->connection[i].lock);
                if ((ret = GM_RtmpClientPsuhAAC(s->connection[i].rtmp_client, send_buf, len, timestamp)) != 0)
                {
                    printf("RTMP Server: stream %d GM_RtmpClientPsuhAAC Fail (%d)\n", stream_id, ret);
                }
                pthread_mutex_unlock(&s->connection[i].lock);
                //printf("call GM_RtmpClientPsuhH264 end\n");
            }
        }
    }

    pthread_mutex_unlock(&s->stream[stream_id].lock);

    return ret;
}
