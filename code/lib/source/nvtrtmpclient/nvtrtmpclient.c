/*
    NvtRtmp Client.

    This file is the main function of NvtRtmp Client.

    @file       nvtrtmpclient.c
    @ingroup    NVTRTMPCLIENT
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include "protected/libgmrtmp.h"
#include "nvtrtmpclient.h"
#include "nvtrtmpclient_internal.h"
#include "ImageApp/ImageApp_Common.h"
#include "../source/ImageApp/Common/ImageApp_Common_int.h"

#define strcpy_m(dst, src)  { \
                                strncpy(dst, src, sizeof(dst)-1); \
                                dst[sizeof(dst)-1] = '\0'; \
                            }


static void *video_encode_thread(void *arg);
static void *audio_encode_thread(void *arg);

static VIDEO_INFO *g_p_video_info = NULL;
static AUDIO_INFO *g_p_audio_info = NULL;
static UINT32 g_u32NvtRtmpClientSts = NVTRTMPCLIENTSTS_STOP;
static UINT32 g_u32NvtRtmpClientWiFiSts = NVTRTMPCLIENT_WIFI_STS_IDE;

int sem_timedwait_millsecs(sem_t *sem, long msecs)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long secs = msecs/1000;
    msecs = msecs%1000;

    long add = 0;
    msecs = msecs*1000*1000 + ts.tv_nsec;
    add = msecs / (1000*1000*1000);
    ts.tv_sec += (add + secs);
    ts.tv_nsec = msecs%(1000*1000*1000);

    return sem_timedwait(sem, &ts);
}


ER NvtRtmpClient_GetVStream(NVTRTMP_CLIENT *rtmp_c, RtmpVencStream *pVStream, VIDEO_INFO *p_video_info)
{
    UINT32 v_info_addr=0, v_info_size=0;
    UINT32 data_offset = 0;
    char *ptr = NULL;
    NVTLIVE555_STRM_INFO strm = {0};
    NVTLIVE555_VIDEO_INFO vdo = {0};
    PayloadBuf *vPayloadBuf = &rtmp_c->vPayloadBuf;
    struct timeval nowtime;

    //wait the video bitstream
    live555_on_lock_video((int)p_video_info, 500, &strm);

    //get the info of sps and pps
    live555_on_get_video_info((int)p_video_info, 500, &vdo);

    ptr = (char *)strm.addr;
    if (ptr[0] != 0x00 || ptr[1] != 0x00 || ptr[2] != 0x00 || ptr[3] != 0x01) {
        g_u32NvtRtmpClientSts = NVTRTMPCLIENTSTS_VDO_ERROR;
        return E_SYS;
    }

    if (ptr[4] == 0x65) { //I-frame+
        pVStream->bIFrameFlag = TRUE;
    } else {
        pVStream->bIFrameFlag = FALSE;
    }

    // re-alloc output buffer if needed
    if (vPayloadBuf->bufPtr == NULL || vPayloadBuf->bufLength < strm.size) {
        if (vPayloadBuf->bufPtr != NULL) {
            free(vPayloadBuf->bufPtr);
        }

        vPayloadBuf->bufLength = strm.size + (strm.size >> 2);

        if ((vPayloadBuf->bufPtr = (UINT8 *)malloc(vPayloadBuf->bufLength)) == NULL) {
            DBG_ERR("Alloc video buffer (need %d Bytes) FAIL !!\r\n", vPayloadBuf->bufLength);
            vPayloadBuf->bufLength = 0;
            return E_NOMEM;
        }
    }

    // If it's I-frame, we have to put SPS/PPS at beginning of output buffer first
    if (pVStream->bIFrameFlag == TRUE) {
        v_info_addr = strm.addr - (vdo.sps_size + vdo.pps_size); //v_info_addr = sps + pps + bs
        v_info_size = vdo.sps_size + vdo.pps_size;

        // Copy SPS/PPS to output buffer
        memcpy(vPayloadBuf->bufPtr + data_offset, (UINT8 *)v_info_addr, v_info_size);
        data_offset += v_info_size;
    }

    gettimeofday(&nowtime, NULL);
    pVStream->TimeStamp   = nowtime;
    pVStream->SVCLayerId  = strm.svc_idx;

    // Copy bitstream to output buffer
    memcpy(vPayloadBuf->bufPtr + data_offset, (UINT8 *)strm.addr, strm.size);
    data_offset += strm.size;

    pVStream->Addr       = (UINT32)vPayloadBuf->bufPtr;
    pVStream->Size       = data_offset;

    return E_OK;
}

ER NvtRtmpClient_GetAStream(NVTRTMP_CLIENT *rtmp_c, RtmpAinStream *pAStream, AUDIO_INFO *paudio_info)
{
    NVTLIVE555_STRM_INFO strm = {0};
    NVTLIVE555_AUDIO_INFO auo = {0};
    struct timeval nowtime;
    PayloadBuf *aPayloadBuf = &rtmp_c->aPayloadBuf;
    ER ret = E_OK;

    //wait the audio bitstream
    ret = live555_on_lock_audio((int)paudio_info, 500, &strm);
    if (ret != E_OK) {
        g_u32NvtRtmpClientSts = NVTRTMPCLIENTSTS_ADO_ERROR;
        return E_SYS;
    }

    live555_on_get_audio_info((int)paudio_info, 500, &auo);

    // re-alloc output buffer if needed
    if (aPayloadBuf->bufPtr == NULL ||  aPayloadBuf->bufLength < strm.size) {
        if (aPayloadBuf->bufPtr != NULL) {
            free(aPayloadBuf->bufPtr);
        }

        aPayloadBuf->bufLength = strm.size+(strm.size >> 2);

        if ((aPayloadBuf->bufPtr = (UINT8 *)malloc(aPayloadBuf->bufLength)) == NULL) {
            DBG_ERR("Alloc audio buffer (need %d Bytes) FAIL !!\r\n", aPayloadBuf->bufLength);
            aPayloadBuf->bufLength = 0;
            return E_NOMEM;
        }
    }

    gettimeofday(&nowtime, NULL);
    pAStream->TimeStamp  = nowtime;

    // Copy bitstream to output buffer
    memcpy(aPayloadBuf->bufPtr, (UINT8 *)strm.addr, strm.size);

    pAStream->Addr       = (UINT32)aPayloadBuf->bufPtr;
    pAStream->Size       = strm.size;

    return E_OK;
}


ER NvtRtmpClient_ResetRtmpClient(NVTRTMP_CLIENT *rtmp_c)
{
    rtmp_c->status                  = NVTRTMP_CLIENT_STATUS_CLOSE;
    rtmp_c->enc_exit                = 0;
    rtmp_c->rtmp_client             = NULL;
    rtmp_c->vPayloadBuf.bufPtr      = NULL;
    rtmp_c->vPayloadBuf.bufLength   = 0;
    rtmp_c->aPayloadBuf.bufPtr      = NULL;
    rtmp_c->aPayloadBuf.bufLength   = 0;

    if ((sem_init(&rtmp_c->semVideo,     0, 0) != 0) ||
        (sem_init(&rtmp_c->semAudio,     0, 0) != 0) ||
        (sem_init(&rtmp_c->semReconnect, 0, 1) != 0) ) {
        DBG_ERR("sem_init() fail !!\r\n");
        return E_SYS;
    }

    return E_OK;
}

ER NvtRtmpClient_Open(char *rtmpURL, NVTRTMPCLIENT_HANDLER *phRtmpC)
{
    int ret = 0;
    *phRtmpC = NULL;

     // allocate NVTRTMP_CLIENT object
    NVTRTMP_CLIENT *rtmp_c = (NVTRTMP_CLIENT *)malloc(sizeof(NVTRTMP_CLIENT));
    if (rtmp_c == NULL) {
        DBG_ERR("alloc NvtRtmpClient handler fail....\r\n");
        return E_SYS;
    }
    if (E_OK != NvtRtmpClient_ResetRtmpClient(rtmp_c)) {
        goto NvtRtmpClientOpen_Fail;
    }

    // assign rtmpURL
    strcpy_m(rtmp_c->rtmp_url, rtmpURL);
    DBG_DUMP("Stream to Server: %s\r\n", rtmp_c->rtmp_url);

    // GM_RtmpClient Init & Connect
    DBG_DUMP("Initialize PushRtmp....\r\n");
    if ((rtmp_c->rtmp_client = GM_RtmpClientInit(0)) == NULL) {
        DBG_ERR("RtmpClientInit() fail ... !!\r\n");
        goto NvtRtmpClientOpen_Fail;
    }

    DBG_DUMP("%s:Trying connect to server...\r\n", __func__);

    if ((ret = GM_RtmpClientConnect(rtmp_c->rtmp_client, rtmp_c->rtmp_url)) != 0) {
        DBG_ERR("RtmpClientConnect Fail !! err = (%d)\r\n", ret);
        goto NvtRtmpClientOpen_Fail;
    }
    DBG_DUMP("%s:Connect to server successed\r\n", __func__);

    *phRtmpC = (void *)rtmp_c;

    rtmp_c->status = NVTRTMP_CLIENT_STATUS_OPEN;

    return E_OK;

NvtRtmpClientOpen_Fail:
    if (rtmp_c) {
        if (rtmp_c->rtmp_client != NULL) GM_RtmpClientDestroy(rtmp_c->rtmp_client);
        memset(rtmp_c, 0, sizeof(NVTRTMP_CLIENT));
        free(rtmp_c);
    }

    *phRtmpC = NULL;

    return E_SYS;
}


ER NvtRtmpClient_Close(NVTRTMPCLIENT_HANDLER hRtmpC)
{
    ER ret = E_OK;
    NVTRTMP_CLIENT *rtmp_c = (NVTRTMP_CLIENT *)hRtmpC;

    free(rtmp_c);
    rtmp_c = NULL;

    return ret;
}

ER NvtRtmpClient_Start(NVTRTMPCLIENT_HANDLER hRtmpC)
{
    NVTRTMP_CLIENT *rtmp_c = (NVTRTMP_CLIENT *)hRtmpC;
	ER ret = E_OK;

    if (!rtmp_c) {
        DBG_ERR("hRtmpC is NULL.\r\n");
        return E_PAR;
    }

    if (rtmp_c->status == NVTRTMP_CLIENT_STATUS_START) {
        DBG_WRN("NvtRtmpClient is already started.\r\n");
        return E_OK;
    }

    if (rtmp_c->status == NVTRTMP_CLIENT_STATUS_CLOSE) {
        DBG_ERR("NvtRtmpClient is closed.\r\n");
        return E_SYS;
    }

    // reset flag to enable video/audio loop
    rtmp_c->enc_exit = 0;

    // Create video/audio thread
    ret = pthread_create(&rtmp_c->video_enc_thread_id, NULL, video_encode_thread, (void *)rtmp_c);
    if (ret < 0) {
        DBG_ERR("create video thread failed\r\n");
        return E_SYS;
    }

    ret = pthread_create(&rtmp_c->audio_enc_thread_id, NULL, audio_encode_thread, (void *)rtmp_c);
    if (ret < 0) {
        DBG_ERR("create audio thread failed\r\n");
        return E_SYS;
    }

    rtmp_c->status = NVTRTMP_CLIENT_STATUS_START;
    g_u32NvtRtmpClientSts = NVTRTMPCLIENTSTS_LIVESTREAM;
    g_u32NvtRtmpClientWiFiSts = NVTRTMPCLIENT_WIFI_STS_START;

    return E_OK;
}

ER NvtRtmpClient_Stop(NVTRTMPCLIENT_HANDLER hRtmpC)
{
    NVTRTMP_CLIENT *rtmp_c = (NVTRTMP_CLIENT *)hRtmpC;
    ER ret = E_OK;

    g_u32NvtRtmpClientSts = NVTRTMPCLIENTSTS_STOP;
    g_u32NvtRtmpClientWiFiSts = NVTRTMPCLIENT_WIFI_STS_STOP;

    if (!rtmp_c) {
        DBG_ERR("hRtmpC is NULL.\r\n");
        return E_PAR;
    }

    if (rtmp_c->status == NVTRTMP_CLIENT_STATUS_STOP) {
        DBG_WRN("NvtRtmpClient is already stopped.\r\n");
        return E_OK;
    }

    // set flag to leave video/audio loop
    rtmp_c->enc_exit = 1;

    ret = GM_RtmpClientDisconnect(rtmp_c->rtmp_client);
    if (ret != E_OK) {
        DBG_ERR("RtmpClientDisconnect failed\r\n");
    }

    // wait for video/audio thread done, or wait for max 2 second.
    if (sem_timedwait_millsecs(&rtmp_c->semVideo, 2000) != 0) {
        DBG_WRN("wait video thread exit timeout..... cancel it by force !!\r\n");
        pthread_cancel(rtmp_c->video_enc_thread_id);
    }
    if (sem_timedwait_millsecs(&rtmp_c->semAudio, 2000) != 0) {
        DBG_WRN("wait audio thread exit timeout..... cancel it by force !!\r\n");
        pthread_cancel(rtmp_c->audio_enc_thread_id);
    }

    DBG_DUMP("RtmpClient_Stop() OK.\r\n");

    rtmp_c->status = NVTRTMP_CLIENT_STATUS_STOP;

    return E_OK;
}

UINT32 NvtRtmpClient_GetStatus(void)
{
    return g_u32NvtRtmpClientSts;
}

void NvtRtmpClient_AutoReconnect(NVTRTMP_CLIENT *rtmp_c)
{
    int ret=0;

    if (sem_trywait(&rtmp_c->semReconnect) == 0) {
        DBG_ERR("Disconnected !! Auto re-connect ...\r\n");

        // reconnect
        if ((ret = GM_RtmpClientConnect(rtmp_c->rtmp_client, rtmp_c->rtmp_url)) != 0) {
            DBG_ERR("RtmpClientConnect Fail !! err = (%d)\r\n", ret);
        }

        // prepare for next time re-connect
        sem_post(&rtmp_c->semReconnect);
    } else {
        // auto re-connect was already requested, just sleep 1 second to wait re-connect result.
        usleep(1000*1000);
    }
}

void NvtRtmpClient_SetWiFiStatus(UINT32 u32WiFiStatus)
{
   g_u32NvtRtmpClientWiFiSts = u32WiFiStatus;
}

static void *video_encode_thread(void *arg)
{
    ER ret = E_OK;
    NVTRTMP_CLIENT *rtmp_c = (NVTRTMP_CLIENT *)arg;
    RtmpVencStream vstream={0};
    unsigned int sTimeStamp = 0;
    int start_push = 0;
    int is_keyframe = 0;


    NAMED_THREAD();

    if (!rtmp_c) {
        DBG_ERR("rtmp_c is NULL.\r\n");
        return 0;
    }

    g_p_video_info = (VIDEO_INFO *)live555_on_open_video(0);

	while (rtmp_c->enc_exit == 0) {

        if (g_u32NvtRtmpClientWiFiSts == NVTRTMPCLIENT_WIFI_STS_DISCONNECT) {
            DBG_ERR("NVTRTMPCLIENT_WIFI_STS_DISCONNECT\r\n");
            live555_on_unlock_video((int)g_p_video_info);
            rtmp_c->enc_exit = 1;
            rtmp_c->status = NVTRTMP_CLIENT_STATUS_STOP;
            sem_post(&rtmp_c->semVideo);
            continue;
        }

        if (E_OK != (ret = NvtRtmpClient_GetVStream(rtmp_c, &vstream, g_p_video_info))) {
            DBG_ERR("GetVStream fail, ret=%d, sts=%d. Reset to NVTRTMPCLIENTSTS_STOP\r\n", (int)ret, g_u32NvtRtmpClientSts);
            live555_on_unlock_video((int)g_p_video_info);
            rtmp_c->enc_exit = 1;
            rtmp_c->status = NVTRTMP_CLIENT_STATUS_STOP;
            sem_post(&rtmp_c->semVideo);
            continue;
        }

        is_keyframe = (vstream.bIFrameFlag == TRUE);

		sTimeStamp = 1000 * ((unsigned long long)vstream.TimeStamp.tv_sec) + ((unsigned long long)vstream.TimeStamp.tv_usec / 1000);

        //printf("get v[%d] = %02x, %d, T = %d\n", (int)vstream.bIFrameFlag, vstream.Addr, (int)vstream.Size, (int)sTimeStamp);

		if (start_push == 0) {
			if (is_keyframe)
				start_push=1;
		}

		if (start_push) {
            if ((ret = GM_RtmpClientPsuhH264(rtmp_c->rtmp_client, (unsigned char *)vstream.Addr, vstream.Size, is_keyframe, sTimeStamp)) != 0) {
                DBG_ERR("RtmpClientPsuhH264 Fail (%d)\r\n", (int)ret);
                live555_on_unlock_video((int)g_p_video_info);
                rtmp_c->enc_exit = 1;
                rtmp_c->status = NVTRTMP_CLIENT_STATUS_STOP;
                g_u32NvtRtmpClientSts = NVTRTMPCLIENTSTS_PUSH_ERROR;
                sem_post(&rtmp_c->semVideo);
                continue;

                /*if (ret == ERR_RTMP_DISCONNECTED) {
                    NvtRtmpClient_AutoReconnect(rtmp_c);
                }*/
            }
		}

        live555_on_unlock_video((int)g_p_video_info);
        usleep(20*1000);
	}

    DBG_DUMP("video thread DONE.\r\n");

    sem_post(&rtmp_c->semVideo); // Info NvtRtmpClient_Stop() that video thread is DONE.

    return 0;
}

static void *audio_encode_thread(void *arg)
{
    ER ret = E_OK;
    NVTRTMP_CLIENT *rtmp_c = (NVTRTMP_CLIENT *)arg;
    RtmpAinStream astream = {0};
    unsigned int sTimeStamp = 0;

    NAMED_THREAD();

    if (!rtmp_c) {
        DBG_ERR("rtmp_c is NULL.\r\n");
        return 0;
    }

    g_p_audio_info = (AUDIO_INFO *)live555_on_open_audio(0);

	while (rtmp_c->enc_exit == 0) {

        if (g_u32NvtRtmpClientWiFiSts == NVTRTMPCLIENT_WIFI_STS_DISCONNECT) {
            DBG_ERR("NVTRTMPCLIENT_WIFI_STS_DISCONNECT\r\n");
            live555_on_unlock_audio((int)g_p_audio_info);
            rtmp_c->enc_exit = 1;
            rtmp_c->status = NVTRTMP_CLIENT_STATUS_STOP;
            sem_post(&rtmp_c->semAudio);
            continue;
        }

        if (E_OK != (ret = NvtRtmpClient_GetAStream(rtmp_c, &astream, g_p_audio_info))) {
            DBG_ERR("GetAStream fail, ret=%d, sts=%d. Reset to NVTRTMPCLIENTSTS_STOP\r\n", (int)ret, g_u32NvtRtmpClientSts);
            live555_on_unlock_audio((int)g_p_audio_info);
            rtmp_c->enc_exit = 1;
            rtmp_c->status = NVTRTMP_CLIENT_STATUS_STOP;
            sem_post(&rtmp_c->semAudio);
            continue;
        }
        sTimeStamp = 1000 * ((unsigned long long)astream.TimeStamp.tv_sec) + ((unsigned long long)astream.TimeStamp.tv_usec / 1000);

        //printf("get a = %02x, %d, T = %d\n", astream.Addr, (int)astream.Size, (int)sTimeStamp);

        if ((ret = GM_RtmpClientPsuhAAC(rtmp_c->rtmp_client, (UINT8 *)astream.Addr, astream.Size, sTimeStamp)) != 0) {
            DBG_ERR("RtmpClientPsuhAAC Fail (%d)\r\n", (int)ret);
            live555_on_unlock_audio((int)g_p_audio_info);
            rtmp_c->enc_exit = 1;
            rtmp_c->status = NVTRTMP_CLIENT_STATUS_STOP;
            g_u32NvtRtmpClientSts = NVTRTMPCLIENTSTS_PUSH_ERROR;
            sem_post(&rtmp_c->semAudio);
            continue;

            /*if (ret == ERR_RTMP_DISCONNECTED) {
                NvtRtmpClient_AutoReconnect(rtmp_c);
            }*/
        }

        live555_on_unlock_audio((int)g_p_audio_info);
        usleep(20*1000);
	}

    DBG_DUMP("audio thread DONE.\r\n");

    sem_post(&rtmp_c->semAudio); // Info NvtRtmpClient_Stop() that audio thread is DONE.

    return 0;
}

