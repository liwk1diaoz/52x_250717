/**
    Header file of NvtRtmp Client

    Exported header file of NvtRtmp Client.

    @file       nvtrtmpclient.h
    @ingroup    NVTRTMPCLIENT
    @version    V1.00.000
    @date       2018/03/14

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _NVTRTMPCLIENT_H_
#define _NVTRTMPCLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <kwrap/stdio.h>
#include <kwrap/debug.h>
#include <kwrap/error_no.h>
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <kwrap/util.h>
#include <kwrap/perf.h>
#include <kwrap/cpu.h>
#include <kwrap/verinfo.h>

#define NAMED_THREAD()       \
        {   \
            char thread_name[16];   \
            snprintf(thread_name, sizeof(thread_name), "%s", __FUNCTION__); \
            prctl(PR_SET_NAME, (unsigned long)thread_name); \
        }

#define NVTRTMPCLIENTSTS_IDLE         0
#define NVTRTMPCLIENTSTS_LIVESTREAM   1
#define NVTRTMPCLIENTSTS_STOP         2
#define NVTRTMPCLIENTSTS_VDO_ERROR    3
#define NVTRTMPCLIENTSTS_ADO_ERROR    4
#define NVTRTMPCLIENTSTS_PUSH_ERROR   5

#define NVTRTMPCLIENT_WIFI_STS_IDE          0
#define NVTRTMPCLIENT_WIFI_STS_START        1
#define NVTRTMPCLIENT_WIFI_STS_STOP         2
#define NVTRTMPCLIENT_WIFI_STS_DISCONNECT   3

/// ========== Flag id definition area ==========
#define FLG_LS_VIDEO_TRIG    0x00000001  //LS = LIVESTREAM
#define FLG_LS_VIDEO_STOP    0x00000002
#define FLG_LS_VIDEO_IDLE    0x00000004
#define FLG_LS_AUDIO_TRIG    0x00010000
#define FLG_LS_AUDIO_STOP    0x00020000
#define FLG_LS_AUDIO_IDLE    0x00040000

/**
    NvtRtmpClient handler

    Handler for calling NvtRtmpClient functions.

    @note get from NvtRtmpClient_Open(), used in NvtRtmpClient_Start() / NvtRtmpClient_Stop() / NvtRtmpClient_Close()
*/
typedef  void*     NVTRTMPCLIENT_HANDLER;       ///< NvtRtmpClient handler


/**
    Open NvtRtmpClient.

    This API would initialize NvtRtmpClient.
    Other APIs should be invoked after calling this API.

    @param[in]  pNvtRtmpClientUsrObj    the NvtRtmpClient user object.
    @param[out] phRtmpC                 NvtRtmpClient handler, to be used in other NvtRtmpClient functions.

    @return
        - @b ER_SUCCESS if success.
        - @b ER_MEM_ALLOC_FAIL if alloc NvtRtmpClient handler fail.
        - @b ER_FUNC_START_FAIL if streamsender/pushrtmp init fail. Or connecting to given rtmp URL fail.
*/
extern ER NvtRtmpClient_Open(char *rtmpURL, NVTRTMPCLIENT_HANDLER *phRtmpC);


/**
    Start NvtRtmpClient to push stream.

    This API will start to get video/audio bitstream and push to given rtmp server URL.

    @param[in] hRtmpC      NvtRtmpClient handler.

    @return
        - @b ER_SUCCESS if success.
        - @b ER_FUNC_START_FAIL if fail to create video/audio threads.
        - @b ER_INVALID_INPUT_DATA if given hRtmpC is NULL.
*/
extern ER NvtRtmpClient_Start(NVTRTMPCLIENT_HANDLER hRtmpC);


/**
    Stop NvtRtmpClient to push stream.

    This API will stop pushing stream to rtmp server.

    @param[in] hRtmpC      NvtRtmpClient handler.

    @return
        - @b ER_SUCCESS if success.
        - @b ER_INVALID_INPUT_DATA if given hRtmpC is NULL.
*/
extern ER NvtRtmpClient_Stop(NVTRTMPCLIENT_HANDLER hRtmpC);


/**
    Close NvtRtmpClient.

    This API will close NvtRtmpClient and release resource.

    @param[in] hRtmpC      NvtRtmpClient handler.

    @return
        - @b ER_SUCCESS if success.
        - @b ER_FUNC_STOP_FAIL if streamsender close fail.
        - @b ER_INVALID_INPUT_DATA if given hRtmpC is NULL.
*/
extern ER NvtRtmpClient_Close(NVTRTMPCLIENT_HANDLER hRtmpC);

extern void NvtRtmpClient_SetWiFiStatus(UINT32 u32WiFiStatus);
#ifdef __cplusplus
}
#endif
/* ----------------------------------------------------------------- */
//@}
#endif //_NVTRTMPCLIENT_H_
