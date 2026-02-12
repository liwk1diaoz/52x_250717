#ifndef __SYSDEF_H
#define __SYSDEF_H

/*Platform Setting*/
#define CompanyName "RTSP Server"
#define ServerName "Streaming Server v0.1"

/*Network Setting*/
#define MTU 1444
/*QoS Parameter*/
#define RTCP_ENABLE
#define RTCP_USER               0
#define RETRY_TIME              0
#define DEBUG_KEVIN
//auto upgrade bitrate duration
#define IFrameInterval          1
#define GM

#define RTP_OVER_UDP            1
#define SUPPORT_PAUSE           1
#define SUPPORT_SET_PARAMETER   1
#define LIVE_TRIG               1
#define VER_RELEASE             1
#define ENQ_TRIG_OPTIMIZED      1   /* Carl, 20100203. */
#define LIBRTSP_MEM_DBG         0

#define SR_STATUS_NULL          0
#define SR_STATUS_QEMPTY        1
#define SR_STATUS_QIMMEDIATE    2
#define SR_STATUS_USED          3

#define QNO_MAX                 2
#define QNO_VIDEO               0
#define QNO_AUDIO               1

#define SESS_ID_LEN             32
#define ENQ_TRIG_COUNT          8
#define ENQ_CHECK_TIME          1000 //8    // in ms.

#define STREAM_NAME_LEN         256
#define REALM_LEN               128
#define USERNAME_LEN            128
#define PASSWORD_LEN            128
#define STREAM_SDP_LEN          256+64

#include <netinet/in.h>
#include "librtsp.h"
#include "frame.h"
#include <pthread.h>

#if (VER_RELEASE)
#define dbg(args...)
#else
#define dbg(args...)    fprintf(stderr, ##args)
#endif


extern pthread_mutex_t  interface_mutex;
extern pthread_mutex_t  server_mutex;
extern pthread_mutex_t  filepath_mutex;
typedef struct session *(*open_func)(char *path, void *d);

typedef struct st_gmss_multicast {
	int  enabled;
	char v_ipaddr[32];
	int  v_port;
	char a_ipaddr[32];
	int  a_port;
	struct session *session;
} gmss_multicast;

typedef struct st_gmss_qu_entity {
	gm_ss_entity    ent;
	struct frame    f;
} gmss_qu_entity;

typedef struct st_gmss_qu {
	int             used;
	int             flush;
	int             type;
	volatile int    in;
	volatile int    out;
	int             size;
	char            sdpstr[STREAM_SDP_LEN];
	void            *stream;    // struct stream *
	gmss_qu_entity  queue[1];
} gmss_qu;

typedef struct st_gmss_sr {
	int             status;
	int             live;
	int             qno[QNO_MAX];
	gmss_qu         *qus[QNO_MAX];
	char            name[STREAM_NAME_LEN];
	open_func       open;
	void            *live_source;       // struct live_source *
	void            *conn;
	gmss_multicast  multicast;
	char            realm[REALM_LEN];
	char            username[USERNAME_LEN];
	char            password[PASSWORD_LEN];
	int             max_connection;
	unsigned int    range_npt_msec;
	unsigned int    bandwidth_info;
} gmss_sr;

int rtcp_enable;

int access_log_init(void);

#endif

