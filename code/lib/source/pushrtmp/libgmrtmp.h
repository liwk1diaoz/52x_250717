/*******************************************************************************
* Copyright  GrainMedia Technology Corporation 2010-2011.  All rights reserved.*
*------------------------------------------------------------------------------*
* Name: libgmrtmp.h                                                            *
* Description:                                                                 *
*     1: ......                                                                *
* Author: giggs                                                                *
*******************************************************************************/

#ifndef LIBGMRTMP_H_
#define LIBGMRTMP_H_

#include <pthread.h>

//==============================================================================
//
//                              DEFINES
//
//==============================================================================

#define ERR_RTMP_PARAMETER_ERROR        (-1)
#define ERR_RTMP_CLIENT_NOT_INIT        (-2)
#define ERR_RTMP_SETUP_URL_FAIL         (-3)
#define ERR_RTMP_CONNECT_FAIL           (-4)
#define ERR_RTMP_CONNECT_STREAM_FAIL    (-5)
#define ERR_RTMP_PUSH_NULL_DATA         (-6)
#define ERR_RTMP_DISCONNECTED           (-7)
#define ERR_RTMP_BODY_BUF_ERROR         (-8)
#define ERR_RTMP_SEND_PACKET_FAIL       (-9)
#define ERR_RTMP_TIMESTAMP_DISORDER     (-10)

//==============================================================================
//
//                              TYPE DEFINES
//
//==============================================================================

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

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

extern void *GM_RtmpClientInit(int buf_size);
extern void GM_RtmpClientDestroy(void *client);
extern int  GM_RtmpClientConnect(void *client, const char* url);
extern int  GM_RtmpClientDisconnect(void *client);
extern int  GM_RtmpClientPsuhH264(void *client, unsigned char *buf, int len, int keyframe, unsigned int timestamp);
extern int  GM_RtmpClientPsuhAAC(void *client, unsigned char *buf, int len, unsigned int timestamp);

extern void *GM_RtmpServerInit(void);
extern void GM_RtmpServerDestroy(void *server);
extern int  GM_RtmpServerCreateStream(void *server, char *stream_url, char *stream_key, int buf_size);
extern int  GM_RtmpServerSnedH264(void *server, int stream_id, unsigned char *buf, int len, int keyframe, unsigned int timestamp);
extern int  GM_RtmpServerSnedAAC(void *server, int stream_id, unsigned char *buf, int len, unsigned int timestamp);

#endif /* LIBGMRTMP_H_ */
