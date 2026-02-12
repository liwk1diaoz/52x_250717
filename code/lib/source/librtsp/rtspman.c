/*
 * Copyright (C) 2009 Faraday Technology Corporation.
 * Author:  Carl Lin.  <nhlin@faraday-tech.com>
 *
 * NOTE:
 *  1. stream_server_init() MUST be called before any other interface functions.
 *  2. Call stream_server_init() only when all streams and queues are deregistered
 *     and the streaming server had been stopped.
 *
 *
 * Version  Date        Description
 * ----------------------------------------------
 * 0.1.0                First release.
 *
 */


#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>


#if defined(KERN_SUB_VERSION_14)
#include <asm/ucontext.h>
#else
#include <sys/ucontext.h>
#endif


#if !defined(PLATFORM_X86)
//#include <asm/sigcontext.h>   /* Commented out for Linux 2.6.26-2 & x86. */
#endif

#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include "sysdef.h"
#include "event.h"
#include "rtp.h"
#include "live.h"
#include "frame.h"
#include "stream.h"
#include "rtp_media.h"
#include "tcp.h"
#include "gm_memory.h"


#define CHIPID_OFFSET_MAGIC_NUM     0x11D57C
#define CHIPID_VALUE_MAGIC_NUM      0x7788

typedef struct _chip_id_pattern_ {
	unsigned int shitf_bit;
	unsigned int pattern;
} chip_id_pattern;

unsigned int chip_id_addr[] = {0x90C00000 + CHIPID_OFFSET_MAGIC_NUM,
							   0x99000000 + CHIPID_OFFSET_MAGIC_NUM
							  };
chip_id_pattern all_support_chip_id[] = {{16, 0xF998},   /* GM8210 */
	{16, 0xFA0F},   /* GM8287 */
	{16, 0xF909},   /* GM8187 */
	{16, 0xF8AE},   /* GM8126 */
	{16, 0xF8B1},   /* GM8139, GM8138 */
	{12, 0xF8BE},   /* GM8136, GM8135 */
};


#define FILE_PREFIX_MAX         516
#define IPIF_MAX                32
#define GMSS_CONN_MAX           64
#define GMSS_SR_MAX             64
#define GMSS_VQNO               64
#define GMSS_VQLEN              3
#define GMSS_AQNO               64
#define GMSS_AQLEN              2
#define LIF_BUF_MAX             2048
#define RTSPMAN_PORT            9852

#define RTSP_PORT_DEF           554

#define GMSS_CMD_SHIFT          24
#define GMSS_QNO_MAX            (1 << GMSS_CMD_SHIFT)
#define GMSS_QNO_MASK           (GMSS_QNO_MAX-1)
#define GMSS_CMD_MASK           (~GMSS_QNO_MASK)

#define GMSS_CMD_STOP           (1 << GMSS_CMD_SHIFT)
#define GMSS_CMD_RESET          (2 << GMSS_CMD_SHIFT)
#define GMSS_CMD_DEREG          (3 << GMSS_CMD_SHIFT)
#define GMSS_CMD_DEREG_FQ       (4 << GMSS_CMD_SHIFT)
#define GMSS_CMD_ENQ            (5 << GMSS_CMD_SHIFT)
#define GMSS_CMD_FLUSH          (6 << GMSS_CMD_SHIFT)
#define GMSS_CMD_SRESET         (7 << GMSS_CMD_SHIFT)
#define GMSS_CMD_SRESET_FQ      (8 << GMSS_CMD_SHIFT)
#define GMSS_CMD_GOODBYE        (9 << GMSS_CMD_SHIFT)

#define RTSPD_STATUS_STOP       0
#define RTSPD_STATUS_START      1
#define RTSPD_STATUS_RUN        2

#define STREAM_THREAD_NULL      0
#define STREAM_THREAD_START     1
#define STREAM_THREAD_STOP      2
#define STREAM_THREAD_RESET     3

#define MT_VIDEO                0
#define MT_AUDIO                1
#define MT_ERROR                -1

#define QUEUE_FREE              0
#define QUEUE_ALLOCATED         1

#define CLEAN_TO_NULL           0
#define CLEAN_TO_STOP           1
#define CLEAN_TO_RESET          2
#define CLEAN_TO_FORCE          3

#define LOCK_WITH_TRY           1
#define IPC_UDP                 0

#define RTSPMAN_USOCK_NAME      "/tmp/rtspman.usock"
#define ERR_GOTO(x, y)          do { ret = x; goto y; } while(0)
#define qu_size(n)              (sizeof(gmss_qu)+sizeof(gmss_qu_entity)*(n))
#define test_free_null(x)       do {                \
		if (x) {        \
			free(x);    \
			x = NULL;   \
		}               \
	} while (0)


#if (LOCK_WITH_TRY)
#define pth_getlock     pthread_mutex_trylock
#else
#define pth_getlock     pthread_mutex_lock
#endif

static char version[]   = "0.1.0";

int spook_init(void);
int config_port(int port);
void rtsp_event_loop(int single);
struct session *live_open(char *path, void *d);
void *live_start_block(void *);
int live_set_track(gmss_sr *sr, gmss_qu *qu);
struct stream *new_stream(gmss_qu *qu, int format);
struct event *add_fd_event(int fd, int write, unsigned int flags, callback f, void *d);
int free_stream(void *);
int remove_rtp_media(void *m);
void deliver_frame_to_stream(struct frame *f, void *d);
void drop_conn(struct conn *c, int callback);
void disconnect_session_client(void *d);
void sdp_parameter_encoder_h264(unsigned char *data, int len, char *sdp, int sdp_len_max);
void sdp_parameter_encoder_mpeg4(unsigned char *data, int len, char *sdp, int sdp_len_max);
void sdp_parameter_encoder_mjpeg(unsigned char *data, int len, char *sdp, int sdp_len_max);
void sdp_parameter_encoder_aac(unsigned char *data, int len, char *sdp, int sdp_len_max);

pthread_t       gmss_thread_id;
pthread_mutex_t interface_mutex;    /* Only one app thread could access the library interface. */
pthread_mutex_t server_mutex;
// pthread_mutex_t  filepath_mutex;
#if (ENQ_TRIG_OPTIMIZED)    /* Carl, 20100203. */
unsigned int    lif_enq_trig    = 0;
#else
unsigned int    enq_trig_count  = 0;
#endif
volatile unsigned int   lif_enq = 0;
unsigned int    gmss_clean_type = CLEAN_TO_RESET;
int     sigpipe             = 0;
int     gmss_av_resync      = 0;
int     mutex_init          = 0;
int     rtspd_init          = 0;
int     rtspd_running       = RTSPD_STATUS_STOP;
int     rtspd_cmd           = STREAM_THREAD_NULL;
int     lif_sock            = -1;                           /* Library interface sock */
int     rtsp_tcp_port       = RTSP_PORT_DEF;
int     rtp_port_start      = 1000;
int     rtp_port_end        = 60000;
int     rtp_dscp            = 0;
int     rtp_mtu             = MTU;
int     gmss_conn           = 0;
int     gmss_conn_max       = GMSS_CONN_MAX;
int     gmss_sr_max         = GMSS_SR_MAX;
int     gmss_qmax           = GMSS_VQNO + GMSS_AQNO;
int     gmss_qno[QNO_MAX]   = {GMSS_VQNO, GMSS_AQNO};
int     gmss_qlen[QNO_MAX]  = {GMSS_VQLEN, GMSS_AQLEN};
gmss_sr *gmss_srs           = NULL;
gmss_qu **gmss_qus          = NULL;
struct event    *lif_event  = NULL;
char    *gmss_localaddr     = NULL;
int (*rtspd_cb)(int type, int qno, gm_ss_entity *entity)    = NULL;
int (*rtspd_cmd_cb)(char *name, int sno, int cmd, void *p)  = NULL;
#if (!IPC_UDP)
struct sockaddr_un  lif_sock_name = {AF_LOCAL, RTSPMAN_USOCK_NAME};
#endif
char    gmss_file_prefix[FILE_PREFIX_MAX] = "";     /* by filepath_mutex */
char    gmss_ipif[IPIF_MAX] = {0};
unsigned int    lif_buf[LIF_BUF_MAX];
int     rtsp_log_conn_stat_mode = 0;

int gm_mutex_lock(void)
{
	return pthread_mutex_lock(&server_mutex);
}


int gm_mutex_trylock(void)
{
	return pthread_mutex_trylock(&server_mutex);
}


int gm_mutex_unlock(void)
{
	return pthread_mutex_unlock(&server_mutex);
}


char *uri_file(char *path)
{
	char    *fn = NULL;
	int     len;

	if (path == NULL) {
		return NULL;
	}
	while (*path == '/') {
		++path;
	}
	if (*path == 0) {
		return NULL;
	}
	for (fn = path; (*fn) && (*fn != '/'); ++fn);
	fn = ((*fn == 0) || (*(fn + 1) == 0)) ? NULL : fn + 1;
	if (fn == NULL) {
		return NULL;
	}

	len = ((unsigned int)fn) - ((unsigned int)path) - 1;

	// if (pthread_mutex_lock(&filepath_mutex)) return NULL;
	fn = ((FILE_PREFIX_MAX <= len) || (gmss_file_prefix[len]) ||
		  (strncmp(path, gmss_file_prefix, len))) ? NULL : path;
	// pthread_mutex_unlock(&filepath_mutex);

	return fn;
}


int call_cmd_cb(char *name, int cmd, void *conn, void *p)
{
	int i, tail, srno, ret = 0;
	gmss_sr *sr;

	if ((name == NULL) || (rtspd_cmd_cb == NULL)) {
		return -1;
	}
	while (*name == '/') {
		++name;
	}
	if (*name == 0) {
		return -1;
	}
	if ((tail = strlen(name) - 1) < 0) {
		return -1;
	}
	if (name[tail] == '/') {
		name[tail] = 0;
	}

	for (i = 0; i < gmss_sr_max; ++i) {
		sr = &(gmss_srs[i]);
		if ((sr->status == SR_STATUS_USED) && (sr->name) &&
#if (LIVE_TRIG) // Carl
			(sr->conn == conn) && ((conn != NULL) == (sr->live == 0)) &&
#else
			(sr->live == 0) && (sr->conn) && (sr->conn == conn) &&
#endif
			(!strcmp(name, sr->name))) {
			break;
		}
	}

	if (i < gmss_sr_max) {
		srno = i;
	} else {
		srno = -1;
	}

	if (cmd == GM_STREAM_CMD_OPTION) {
		//srno = -1;
		gm_mutex_unlock();
		ret = rtspd_cmd_cb(name, srno, cmd, p);
		gm_mutex_lock();
	} else if (cmd == GM_STREAM_CMD_DESCRIBE) {
		//srno = -1;
		gm_mutex_unlock();
		ret = rtspd_cmd_cb(name, srno, cmd, p);
		gm_mutex_lock();
	} else if (cmd == GM_STREAM_CMD_SETUP) {
		//srno = -1;
		gm_mutex_unlock();
		ret = rtspd_cmd_cb(name, srno, cmd, p);
		gm_mutex_lock();
	} else if (cmd == GM_STREAM_CMD_OPEN) {
		//srno = -1;
		gm_mutex_unlock();
		ret = rtspd_cmd_cb(name, srno, cmd, p);
		gm_mutex_lock();
	} else if (cmd == GM_STREAM_CMD_PAUSE) {
		//srno = -1;
		gm_mutex_unlock();
		ret = rtspd_cmd_cb(name, srno, cmd, p);
		gm_mutex_lock();
	} else {
		for (i = 0; i < gmss_sr_max; ++i) {
			sr = &(gmss_srs[i]);
			if ((sr->status == SR_STATUS_USED) && (sr->name) &&
#if (LIVE_TRIG) // Carl
				(sr->conn == conn) && ((conn != NULL) == (sr->live == 0)) &&
#else
				(sr->live == 0) && (sr->conn) && (sr->conn == conn) &&
#endif
				(!strcmp(name, sr->name))) {
				break;
			}
		}
		if (i < gmss_sr_max) {
			srno = i;
			ret = rtspd_cmd_cb(name, srno, cmd, p);
		} else {
			ERR_GOTO(-1, call_cmd_cb_err);
		}
	}

call_cmd_cb_err:
	if (name[tail] == 0) {
		name[tail] = '/';
	}
	return ret;
}


int gmss_get_sdp(gmss_sr *sr, char *dest, int len)
{
	int i = 0, t, port = 0;
	gmss_qu *q;
	int aac_cfg_int, aac_channel, aac_sample_rate;
	int aac_sample_rate_tbl[] = {96000, 88200, 64000, 48000,
								 44100, 32000, 24000, 22050,
								 16000, 12000, 11025, 8000,
								 7350, 0, 0, 0
								};

	for (t = 0; t < QNO_MAX; ++t) {
		q = sr->qus[t];
		if ((q == NULL) || (q->used <= QUEUE_ALLOCATED)) {
			continue;
		}
		switch (q->type) {
		case GM_SS_TYPE_H264:
			i += snprintf(dest + i, len - i, "m=video %d RTP/AVP 96\r\n"
						  "b=AS:%u\r\n"
						  "a=rtpmap:96 H264/90000\r\n"
						  "a=fmtp:96 profile-level-id=42A01E;packetization-mode=1;sprop-parameter-sets=%s\r\n"
						  "a=control:trackID=0\r\n",
						  port, sr->bandwidth_info, (q->sdpstr) ? q->sdpstr : "");
			break;
		case GM_SS_TYPE_H265:
			i += snprintf(dest + i, len - i, "m=video %d RTP/AVP 96\r\n"
						  "b=AS:%u\r\n"
						  "a=rtpmap:96 H265/90000\r\n"
						  "a=fmtp:96 profile-space=0;profile-id=1;tier-flag=0;interop-constraints=000000000000;%s\r\n"
						  "a=control:trackID=0\r\n",
						  port, sr->bandwidth_info, (q->sdpstr) ? q->sdpstr : "");
			break;
		case GM_SS_TYPE_MP4:
			i += snprintf(dest + i, len - i, "m=video %d RTP/AVP 96\r\n"
						  "b=AS:%u\r\n"
						  "a=rtpmap:96 MP4V-ES/90000\r\n"
						  "a=fmtp:96 profile-level-id=1;config=%s\r\n"
						  "a=control:trackID=0\r\n",
						  port, sr->bandwidth_info, (q->sdpstr) ? q->sdpstr : "");
			break;
		case GM_SS_TYPE_MJPEG:
			i += snprintf(dest + i, len - i, "m=video %d RTP/AVP 26\r\n"
						  "b=AS:%u\r\n"
						  "a=fmtp:26 config=%s\r\n"
						  "a=control:trackID=0\r\n",
						  port, sr->bandwidth_info, (q->sdpstr) ? q->sdpstr : "");
			break;
		case GM_SS_TYPE_MP2:
			i += snprintf(dest + i, len - i, "m=audio %d RTP/AVP 14\r\n"
						  "a=fmtp:14 config=%s\r\n"
						  "a=control:trackID=1\r\n",
						  port, (q->sdpstr) ? q->sdpstr : "");
			break;
		case GM_SS_TYPE_AMR:
			i += snprintf(dest + i, len - i, "m=audio %d RTP/AVP 97\r\n"
						  "a=rtpmap:97 AMR/8000\r\n"
						  "a=fmtp:97 octet-align=1\r\n"
						  "a=control:trackID=1\r\n",
						  port);
			break;
		case GM_SS_TYPE_PCM:
			if (strlen(q->sdpstr)) {
				i += snprintf(dest + i, len - i, "m=audio %d RTP/AVP 97\r\n"
							  "a=rtpmap:97 %s\r\n"
							  "a=fmtp:97\r\n"
							  "a=control:trackID=1\r\n",
							  port,
							  q->sdpstr);
			} else {
				i += snprintf(dest + i, len - i, "m=audio %d RTP/AVP 97\r\n"
							  "a=rtpmap:97 L16/8000/1\r\n"
							  "a=fmtp:97\r\n"
							  "a=control:trackID=1\r\n",
							  port);
			}
			break;
		case GM_SS_TYPE_G711A:
			i += snprintf(dest + i, len - i, "m=audio %d RTP/AVP 8\r\n"
						  //"a=rtpmap:97 PCMU/8000/2\r\n"
						  "a=control:trackID=1\r\n",
						  port);
			break;
		case GM_SS_TYPE_G711U:
			i += snprintf(dest + i, len - i, "m=audio %d RTP/AVP 0\r\n"
						  //"a=rtpmap:97 PCMU/8000/2\r\n"
						  "a=control:trackID=1\r\n",
						  port);
			break;
		case GM_SS_TYPE_G726:
			i += snprintf(dest + i, len - i, "m=audio %d RTP/AVP 97\r\n"
						  "a=rtpmap:97 G726-32/8000\r\n"
						  "a=control:trackID=1\r\n",
						  port);
			break;
		case GM_SS_TYPE_AAC:
			if (strlen(q->sdpstr) > 0) { //20150320 yhk
				sscanf(q->sdpstr, "%x", &aac_cfg_int);
				aac_channel = (aac_cfg_int & 0x00000078) >> 3;
				aac_sample_rate = aac_sample_rate_tbl[(aac_cfg_int & 0x00000780) >> 7];
			} else { //20150320 yhk
				strcpy(q->sdpstr, "1588");
				aac_channel = 1;
				aac_sample_rate = 8000;
			}
			i += snprintf(dest + i, len - i, "m=audio %d RTP/AVP 97\r\n"
						  "a=rtpmap:97 MPEG4-GENERIC/%d/%d\r\n"
						  "a=fmtp:97 streamtype=5;profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=%s\r\n"
						  "a=control:trackID=1\r\n",
						  port,
						  (q->sdpstr) ? aac_sample_rate : 8000,
						  (q->sdpstr) ? aac_channel : 1,
						  (q->sdpstr) ? q->sdpstr : "1588");
			break;
		}
	}

	return i;
}

int gmss_get_sdp_multicast(gmss_sr *sr, char *dest, int len)
{
	int i = 0, t;
	gmss_qu *q;
	int aac_cfg_int, aac_channel, aac_sample_rate;
	int aac_sample_rate_tbl[] = {96000, 88200, 64000, 48000,
								 44100, 32000, 24000, 22050,
								 16000, 12000, 11025, 8000,
								 7350, 0, 0, 0
								};

	for (t = 0; t < QNO_MAX; ++t) {
		q = sr->qus[t];
		if ((q == NULL) || (q->used <= QUEUE_ALLOCATED)) {
			continue;
		}
		switch (q->type) {
		case GM_SS_TYPE_H264:
			i += snprintf(dest + i, len - i, "c=IN IP4 %s/1\r\nt=0 0\r\na=type:broadcast\r\nm=video %d RTP/AVP 96\r\n"
						  "b=AS:%u\r\n"
						  "a=rtpmap:96 H264/90000\r\n"
						  "a=fmtp:96 profile-level-id=42A01E;packetization-mode=1;sprop-parameter-sets=%s\r\n"
						  "a=control:trackID=0\r\n",
						  sr->multicast.v_ipaddr, sr->multicast.v_port, sr->bandwidth_info, (q->sdpstr) ? q->sdpstr : "");
			break;
		case GM_SS_TYPE_H265:
			i += snprintf(dest + i, len - i,
						  "c=IN IP4 %s/1\r\nt=0 0\r\na=type:broadcast\r\nm=video %d RTP/AVP 96\r\n"
						  "b=AS:%u\r\n"
						  "a=rtpmap:96 H265/90000\r\n"
						  "a=fmtp:96 profile-space=0;profile-id=1;tier-flag=0;interop-constraints=000000000000;%s\r\n"
						  "a=control:trackID=0\r\n",
						  sr->multicast.v_ipaddr, sr->multicast.v_port,
						  sr->bandwidth_info,
						  (q->sdpstr) ? q->sdpstr : "");
			break;
		case GM_SS_TYPE_MP4:
			i += snprintf(dest + i, len - i, "c=IN IP4 %s/1\r\nt=0 0\r\na=type:broadcast\r\nm=video %d RTP/AVP 96\r\n"
						  "b=AS:%u\r\n"
						  "a=rtpmap:96 MP4V-ES/90000\r\n"
						  "a=fmtp:96 profile-level-id=1;config=%s\r\n"
						  "a=control:trackID=0\r\n",
						  sr->multicast.v_ipaddr, sr->multicast.v_port, sr->bandwidth_info, (q->sdpstr) ? q->sdpstr : "");
			break;
		case GM_SS_TYPE_MJPEG:
			i += snprintf(dest + i, len - i, "c=IN IP4 %s/1\r\nt=0 0\r\na=type:broadcast\r\nm=video %d RTP/AVP 26\r\n"
						  "b=AS:%u\r\n"
						  "a=fmtp:26 config=%s\r\n"
						  "a=control:trackID=0\r\n",
						  sr->multicast.v_ipaddr, sr->multicast.v_port, sr->bandwidth_info, (q->sdpstr) ? q->sdpstr : "");
			break;
		case GM_SS_TYPE_MP2:
			i += snprintf(dest + i, len - i, "c=IN IP4 %s/1\r\nt=0 0\r\na=type:broadcast\r\nm=audio %d RTP/AVP 14\r\n"
						  "a=fmtp:14 config=%s\r\n"
						  "a=control:trackID=1\r\n",
						  sr->multicast.a_ipaddr, sr->multicast.a_port, (q->sdpstr) ? q->sdpstr : "");
			break;
		case GM_SS_TYPE_AMR:
			i += snprintf(dest + i, len - i, "c=IN IP4 %s/1\r\nt=0 0\r\na=type:broadcast\r\nm=audio %d RTP/AVP 97\r\n"
						  "a=rtpmap:97 AMR/8000\r\n"
						  "a=fmtp:97 octet-align=1\r\n"
						  "a=control:trackID=1\r\n",
						  sr->multicast.a_ipaddr, sr->multicast.a_port);
			break;
		case GM_SS_TYPE_PCM:
			if (strlen(q->sdpstr)) {
				i += snprintf(dest + i, len - i, "c=IN IP4 %s/1\r\nt=0 0\r\na=type:broadcast\r\nm=audio %d RTP/AVP 97\r\n"
							  "a=rtpmap:97 %s\r\n"
							  "a=fmtp:97\r\n"
							  "a=control:trackID=1\r\n",
							  sr->multicast.a_ipaddr, sr->multicast.a_port,
							  q->sdpstr);
			} else {
				i += snprintf(dest + i, len - i, "c=IN IP4 %s/1\r\nt=0 0\r\na=type:broadcast\r\nm=audio %d RTP/AVP 97\r\n"
							  "a=rtpmap:97 L16/8000/1\r\n"
							  "a=fmtp:97\r\n"
							  "a=control:trackID=1\r\n",
							  sr->multicast.a_ipaddr, sr->multicast.a_port);
			}
			break;
		case GM_SS_TYPE_G711A:
			i += snprintf(dest + i, len - i, "c=IN IP4 %s/1\r\nt=0 0\r\na=type:broadcast\r\nm=audio %d RTP/AVP 8\r\n"
						  //"a=rtpmap:97 PCMU/8000/2\r\n"
						  "a=control:trackID=1\r\n",
						  sr->multicast.a_ipaddr, sr->multicast.a_port);
			break;
		case GM_SS_TYPE_G711U:
			i += snprintf(dest + i, len - i, "c=IN IP4 %s/1\r\nt=0 0\r\na=type:broadcast\r\nm=audio %d RTP/AVP 0\r\n"
						  //"a=rtpmap:97 PCMU/8000/2\r\n"
						  "a=control:trackID=1\r\n",
						  sr->multicast.a_ipaddr, sr->multicast.a_port);
			break;
		case GM_SS_TYPE_G726:
			i += snprintf(dest + i, len - i, "c=IN IP4 %s/1\r\nt=0 0\r\na=type:broadcast\r\nm=audio %d RTP/AVP 97\r\n"
						  "a=rtpmap:97 G726-32/8000\r\n"
						  "a=control:trackID=1\r\n",
						  sr->multicast.a_ipaddr, sr->multicast.a_port);
			break;
		case GM_SS_TYPE_AAC:
			if (strlen(q->sdpstr) > 0) { //20150320 yhk
				sscanf(q->sdpstr, "%x", &aac_cfg_int);
				aac_channel = (aac_cfg_int & 0x00000078) >> 3;
				aac_sample_rate = aac_sample_rate_tbl[(aac_cfg_int & 0x00000780) >> 7];
			} else { //20150320 yhk
				strcpy(q->sdpstr, "1588");
				aac_channel = 1;
				aac_sample_rate = 8000;
			}
			i += snprintf(dest + i, len - i, "c=IN IP4 %s/1\r\nt=0 0\r\na=type:broadcast\r\nm=audio %d RTP/AVP 97\r\n"
						  "a=rtpmap:97 MPEG4-GENERIC/%d/%d\r\n"
						  "a=fmtp:97 streamtype=5;profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=%s\r\n"
						  "a=control:trackID=1\r\n",
						  sr->multicast.a_ipaddr, sr->multicast.a_port,
						  (q->sdpstr) ? aac_sample_rate : 8000,
						  (q->sdpstr) ? aac_channel : 1,
						  (q->sdpstr) ? q->sdpstr : "1588");
			break;
		}
	}

	return i;
}

static int media_type(int type)
{
	if ((GM_SS_VIDEO_MIN <= type) && (type <= GM_SS_VIDEO_MAX)) {
		return MT_VIDEO;
	} else if ((GM_SS_AUDIO_MIN <= type) && (type <= GM_SS_AUDIO_MAX)) {
		return MT_AUDIO;
	}

	return MT_ERROR;
}


static void gmss_qu_reset(gmss_qu *qu)
{
	int size;

	//test_free_null(qu->sdpstr);
	memset((void *)qu->sdpstr, 0, STREAM_SDP_LEN);
	if (qu->stream) {
		remove_stream(qu->stream);
	}
	size = qu->size;
	memset(qu, 0, sizeof(gmss_qu));
	qu->size = size;
}


static void gmss_sr_reset(gmss_sr *sr, int free_queue)
{
	struct live_source  *ls;
	struct stream       *s;
	gmss_qu             *qu;
	int                 t;
	gmss_qu             *qus[QNO_MAX];

	if (free_queue) {
		for (t = 0; t < QNO_MAX; ++t) {
			qus[t] = sr->qus[t];
		}
	}

	//test_free_null(sr->name);
	memset((void *)sr->name, 0, STREAM_NAME_LEN);
	if ((ls = (struct live_source *)(sr->live_source))) {
		for (t = 0; t < QNO_MAX; ++t) {
			if (ls->track[t].rtp) {
				remove_rtp_media(ls->track[t].rtp);
			}
			if ((ls->track[t].stream)) {
				s = ls->track[t].stream->stream;
				remove_dest(ls->track[t].stream);
				if (s) {
					qu = (gmss_qu *)(s->src_private);
					if (qu) {
						--(qu->used);
					}
					if (s->dest_list == NULL) {
						remove_stream(s);
						if (qu) {
							// qu->stream = NULL;
							//test_free_null(qu->sdpstr);
							memset((void *)qu->sdpstr, 0, STREAM_SDP_LEN);
							qu->used = QUEUE_ALLOCATED;
						}
					}
				}
			}
		}
		free(ls);
	}

	if (free_queue) {
		for (t = 0; t < QNO_MAX; ++t) {
			if ((qus[t]) && (qus[t]->used <= QUEUE_ALLOCATED)) {
				gmss_qu_reset(qus[t]);
			}
		}
	}

	memset(sr, 0, sizeof(gmss_sr));
}


static gmss_qu **gmss_qu_alloc(int vqno, int vqlen, int aqno, int aqlen)
{
	int     i, j, ns, size;
	gmss_qu **q;
	void    *p;

	if ((vqno < 0) || (aqno < 0) || ((vqno == 0) && (aqno == 0))) {
		return NULL;
	}

	gmss_qmax = vqno + aqno;
	ns = gmss_qmax * sizeof(gmss_qu *);
	size = ns + vqno * qu_size(vqlen) + aqno * qu_size(aqlen);
	p = calloc(1, size);
	if (p == NULL) {
		return NULL;
	}

	q = (gmss_qu **) p;
	p += ns;
	if (0 < vqno) {
		for (i = 0, size = qu_size(vqlen); i < vqno; ++i) {
			q[i] = (gmss_qu *)p;
			q[i]->size = vqlen + 1;
			p += size;
			for (j = 0; j < q[i]->size; ++j) {
				pthread_mutex_init(&(q[i]->queue[j].f.mutex), NULL);
			}
		}
	}
	if (0 < aqno) {
		for (i = vqno, size = qu_size(aqlen); i < gmss_qmax; ++i) {
			q[i] = (gmss_qu *)p;
			q[i]->size = aqlen + 1;
			p += size;
			for (j = 0; j < q[i]->size; ++j) {
				pthread_mutex_init(&(q[i]->queue[j].f.mutex), NULL);
			}
		}
	}

	return q;
}


static int reset_resources(int all)
{
	int     i;

	for (i = 0; i < gmss_sr_max; ++i) {
		if ((gmss_srs[i].status == SR_STATUS_NULL) || ((!all) && (gmss_srs[i].live))) {
			continue;
		}
		gmss_sr_reset(&(gmss_srs[i]), 1);
	}

	// for fault-tolerance
	if ((1) && (all)) {
		for (i = 0; i < gmss_qmax; ++i)
			if (gmss_qus[i]->used) {
				gmss_qu_reset(gmss_qus[i]);
			}
	}

	return 0;
}


void unref_frame(struct frame *f)
{
	int rc;
	gmss_qu *q;

	pthread_mutex_lock(&(f->mutex));
	rc = --(f->ref_count);
	pthread_mutex_unlock(&(f->mutex));
	if (0 < rc) {
		return;
	}

	if ((gmss_qmax <= f->qno) || (gmss_qus == NULL) || (rtspd_cb == NULL)) {
		dbg("f->qno/gmss_qus/rtspd_cb error\n");
		return;
	}

	rc = (*rtspd_cb)(f->format, f->qno, f->ent);
	q = gmss_qus[f->qno];
	q->out = (q->out + 1) % (q->size);
}


void gmss_sr_qu_reset(gmss_sr *sr, int free_queue)
{
	if (sr == NULL) {
		return;
	}
	gmss_sr_reset(sr, free_queue);
}


int stream_cleanall(int sno, int free_queue)
{
	struct live_source  *ls;
	// struct live_session      *lsess;
	// struct stream        *s;
	// struct stream_destination    *dest;
	// struct rtp_media *rtp;
	// int      i;
	struct rtp_endpoint *ep;
	// gmss_qu  *qus[QNO_MAX];
	gmss_sr *sr = &(gmss_srs[sno]);

	// Close all live sessions here.
	ls = (struct live_source *)(sr->live_source);
	while (ls->sess_list) {
		if (ls->sess_list->sess == NULL) {
			dbg("live_session->sess NULL\n");
			break;
		}
		ep = (ls->sess_list->sess->ep[0]) ? ls->sess_list->sess->ep[0] : ls->sess_list->sess->ep[1];
		if (ep == NULL) {
			dbg("live_session->sess->ep[*] NULL\n");
			break;
		}

		if (ep->trans_type == RTP_TRANS_INTER && ep->trans.inter.conn) {
			drop_conn(ep->trans.inter.conn, 0);
		} else if (ep->trans_type == RTP_TRANS_UDP && ep->trans.udp.conn) {
			drop_conn(ep->trans.udp.conn, 0);
		} else {
			dbg("live_session->sess->ep[]->trans.inter.conn NULL\n");
			break;
		}
	}

	gmss_sr_qu_reset(sr, free_queue);

	return 0;
}


void rtspd_clean(void *p)
{
	dbg("%s: rtspd thread cancelled\n", __func__);
	rtspd_running = RTSPD_STATUS_STOP;
	rtspd_cmd = STREAM_THREAD_NULL;
}


void reset_all_streams(int free_queue)
{
	int i;
	gmss_sr *sr;

	for (i = 0; i < gmss_sr_max; ++i) {
		sr = &(gmss_srs[i]);
		gmss_sr_qu_reset(sr, free_queue);
	}
}

void clean_resources(void *p)
{
	unsigned int    clean_type;

	clean_type = (p) ? ((unsigned int)p) : gmss_clean_type;
	if (clean_type <= CLEAN_TO_NULL) {
		return;
	}

	dbg("%s: clean_type %d\n", __func__, clean_type);
	conn_cleanall();
	reset_resources(0);
	event_cleanall();
	if (listener) {
		if (0 <= listener->fd) {
			close(listener->fd);
		}
		free(listener);
		listener = NULL;
	}
	close(lif_sock);
	lif_sock = -1;
#if (ENQ_TRIG_OPTIMIZED)    /* Carl, 20100203. */
	lif_enq_trig = 0;
#endif
	lif_enq = 0;
	test_free_null(gmss_localaddr);

	if (CLEAN_TO_RESET <= clean_type) {
		reset_resources(1);
	}

	if (CLEAN_TO_FORCE <= clean_type) {
		test_free_null(gmss_srs);
		test_free_null(gmss_qus);
	}

	rtspd_clean(NULL);
}


int exec_ipc_cmd(unsigned int cmdno)
{
	unsigned int    cmd = cmdno & GMSS_CMD_MASK;
	unsigned int    no = cmdno & GMSS_QNO_MASK;
	int ret = 0;
	gmss_qu *q;

	switch (cmd) {
	case GMSS_CMD_DEREG:
	case GMSS_CMD_DEREG_FQ:
		// dbg("%s: GMSS_CMD_DEREG(%d)\n", __func__, no);
		if (gmss_sr_max <= no) {
			return -1;
		}
		ret = stream_cleanall(no, cmd == GMSS_CMD_DEREG_FQ);
		break;
	case GMSS_CMD_ENQ:
		// if ((gmss_qmax <= no) || ((q = gmss_qus[no]) == NULL) || (q->in == q->out)) break;
		// dbg("ENQ(%04x) ", no);
		if (rtspd_cb) {
			int i;
			for (i = 0; i < gmss_qmax; ++i) {
				if (((q = gmss_qus[i]) == NULL) || (q->stream == NULL) || (q->in == q->out)) {
					continue;
				}
				while (q->out != q->in) {
					deliver_frame_to_stream(&(q->queue[q->out].f), q->stream);
				}
			}
		} else {
			ret = -1;
		}
		break;
	case GMSS_CMD_FLUSH:
		if ((gmss_qmax <= no) || ((q = gmss_qus[no]) == NULL) || (q->in == q->out)) {
			break;
		}
		if (rtspd_cb) {
			while (q->out != q->in) {
				unref_frame(&(q->queue[q->out].f));
			}
		} else {
			ret = -1;
		}
		break;
	case GMSS_CMD_STOP:
		clean_resources((void *)CLEAN_TO_STOP);
		gm_mutex_unlock();
		pthread_exit(NULL);
		break;
	case GMSS_CMD_RESET:
		clean_resources((void *)CLEAN_TO_RESET);
		gm_mutex_unlock();
		pthread_exit(NULL);
		break;
	case GMSS_CMD_SRESET:
		reset_all_streams(0);
		break;
	case GMSS_CMD_SRESET_FQ:
		reset_all_streams(1);
		break;
		// default:
		// ret = exec_rtspd_cmd();
	}

	return ret;
}


int exec_rtspd_cmd(void)
{
	int ret = 0;

	switch (rtspd_cmd) {
	case STREAM_THREAD_STOP:
		exec_ipc_cmd(GMSS_CMD_STOP);
		break;
	case STREAM_THREAD_RESET:
		exec_ipc_cmd(GMSS_CMD_RESET);
		break;
	}

	return ret;
}


static void lif_sock_callback(struct event_info *ei, void *d)
{
	int i, size, ret;

#if (!ENQ_TRIG_OPTIMIZED)   /* Carl, 20100203. */
	exec_ipc_cmd(GMSS_CMD_ENQ);
#endif

	do {
		size = recv(lif_sock, lif_buf, sizeof(lif_buf), MSG_DONTWAIT);
		if (size < 0) {
			if (errno != EAGAIN) {
				dbg("lif_sock(%d) recv err %d,%d\n", lif_sock, size, errno);
				// what to do here ???
			}
			break;
		}
		if (size & (sizeof(unsigned int) - 1)) {
			dbg("lif_sock(%d) recv incomplete (%d bytes).\n", lif_sock, size);
		}

		size /= sizeof(unsigned int);
		for (i = 0; i < size; ++i) {
#if (ENQ_TRIG_OPTIMIZED)    /* Carl, 20100203. */
			if ((lif_buf[i] & GMSS_CMD_MASK) == GMSS_CMD_ENQ) {
				++lif_enq;
			}
#endif
			ret = exec_ipc_cmd(lif_buf[i]);
			if ((lif_buf[i] & GMSS_CMD_MASK) == GMSS_CMD_GOODBYE) {
				return;    /* to make rtcp_goodbye processed before drop_conn */
			}
		}
	} while (0 < size);

#if (!ENQ_TRIG_OPTIMIZED)   /* Carl, 20100203. */
	lif_enq = 0;
#endif
}


void rtspd_clean_test(void *p)
{
	dbg("%s: abnormal cancelled\n", __func__);
}


void *rtspd_thread(void *ptr)
{
	int ret;
	char thread_name[16];

	snprintf(thread_name, sizeof(thread_name), "%s", __FUNCTION__);
	prctl(PR_SET_NAME, (unsigned long)thread_name);

	if (spook_init() < 0) {
		goto rtspd_thread_error;
	}

	ret = config_port(rtsp_tcp_port);
	// dbg("config_port %d\n", ret);
	if (ret < 0) {
		goto rtspd_thread_error;
	}

	if ((lif_event = add_fd_event(lif_sock, 0, 0, lif_sock_callback, NULL)) == NULL) {
		dbg("%s: add_fd_event(lif_sock) error\n", __func__);
		goto rtspd_thread_error;
	}

	rtspd_cmd = STREAM_THREAD_NULL;
	rtspd_running = RTSPD_STATUS_RUN;

	pthread_cleanup_push(rtspd_clean_test, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	dbg("Goto rtsp_event_loop(0).\n");
	rtsp_event_loop(0);

rtspd_thread_error:
	rtspd_running = RTSPD_STATUS_STOP;
	pthread_cleanup_pop(0);
	return NULL;
}


static void signal_proc(int sig)
{
	switch (sig) {
	case SIGPIPE:
		// dbg(" get SIGPIPE ");
		break;
	default:
		fprintf(stderr, "librtsp gets an unknown signal %d !\n", sig);
		break;
	}
}


void segv_handler(int signal, siginfo_t *si, void *context)
{
#if (0)
	struct ucontext *u = context;
	fprintf(stderr, "Signal %d received by pid %d\n", signal, getpid());
	if (signal == SIGSEGV)
		fprintf(stderr, "faulting address is 0x%08x"
#if defined(PLATFORM_X86)
				"\n",
#else
				", eip=%08lx\n",
#endif
				(unsigned int)si->si_addr
#if defined(PLATFORM_X86)   /* Carl, 20091102. */
				//u->uc_mcontext.eip
#else
				//,u->uc_mcontext.arm_pc
				, u->uc_mcontext.pc
#endif
			   );
#endif
	exit(1);
}

static unsigned int read_physical_addr(unsigned int addr)
{
	int _fdmem;
	int *map = NULL;
	unsigned int val;

	/* open /dev/mem and error checking */
	_fdmem = open("/dev/mem", O_RDWR | O_SYNC);

	if (_fdmem < 0) {
		return 0;
	}

	/* mmap() the opened /dev/mem */
	map = (int *)(mmap(0, 100, PROT_READ | PROT_WRITE, MAP_SHARED, _fdmem, addr));

	/* use 'map' pointer to access the mapped area! */
	val = (unsigned int) * map;

	/* unmap the area & error checking */
	if (munmap(map, 100) == -1) {
		perror("Error un-mmapping the file");
	}

	/* close the character device */
	close(_fdmem);

	return val;
}

static int check_platform()
{
#if 0   //!defined(PLATFORM_X86)
	int i, j;
	FILE *fp;
	char cmd_buf[32];
	char cmd_ret[12];
	int char_shift;
	unsigned int chip_id_val;
	int check_ret = 0;

	// Check by /proc/pmu/chipver
	if (access("/proc/pmu/chipver", F_OK) != -1) {
		snprintf(cmd_buf, sizeof(cmd_buf) - 1, "cat /proc/pmu/chipver");
		fp = popen(cmd_buf, "r");
		fgets(cmd_ret, sizeof(cmd_ret) - 1, fp);
		pclose(fp);

		if (strncmp(&cmd_ret[0], "8287", 4) == 0 ||
			strncmp(&cmd_ret[0], "8210", 4) == 0 ||
			strncmp(&cmd_ret[0], "8139", 4) == 0 ||
			strncmp(&cmd_ret[0], "8138", 4) == 0 ||
			strncmp(&cmd_ret[0], "8136", 4) == 0 ||
			strncmp(&cmd_ret[0], "8135", 4) == 0) {
			check_ret = 1;
			goto check_platform_exit;
		}
	}

	// Check by devmem
	for (i = 0; i < sizeof(chip_id_addr) / sizeof(unsigned int); i++) {
		snprintf(cmd_buf, sizeof(cmd_buf) - 1, "devmem 0x%08X", chip_id_addr[i] - CHIPID_OFFSET_MAGIC_NUM);
		fp = popen(cmd_buf, "r");
		fgets(cmd_ret, sizeof(cmd_ret) - 1, fp);
		pclose(fp);

		if (cmd_ret[0] == '0' && cmd_ret[1] == 'x') {
			for (j = 0; j < sizeof(all_support_chip_id) / sizeof(chip_id_pattern); j++) {
				char_shift = 4 - all_support_chip_id[j].shitf_bit / 4;

				if ((((cmd_ret[2 + char_shift] - '0') << 12) +
					 ((cmd_ret[3 + char_shift] - '0') << 8) +
					 ((cmd_ret[4 + char_shift] - '0') << 4) +
					 (cmd_ret[5 + char_shift] - '0'))
					== all_support_chip_id[j].pattern - CHIPID_VALUE_MAGIC_NUM) {
					chip_id_val = read_physical_addr(chip_id_addr[i] - CHIPID_OFFSET_MAGIC_NUM);
					if (((chip_id_val >> all_support_chip_id[j].shitf_bit) + CHIPID_VALUE_MAGIC_NUM) == all_support_chip_id[j].pattern) {
						check_ret = 1;
						goto check_platform_exit;
					}
				}
			}
		}

		if (check_ret) {
			goto check_platform_exit;
		}
	}

check_platform_exit:
	return check_ret;
#else
	return 1;
#endif
}

/* Startup & Shutdown */
int stream_server_init(char *ipif, int port, int dscp, int mtu, int cons, int streams,
					   int vqno, int vqlen, int aqno, int aqlen,
					   int (*frm_cb)(int type, int qno, gm_ss_entity *entity),
					   int (*cmd_cb)(char *name, int sno, int cmd, void *p))
{
	pthread_mutexattr_t attr;
	int ret = 0;

	printf("GM RTSP Library [build - %s %s]\n", __DATE__, __TIME__);

	if (!check_platform()) {
		printf("GM RTSP Library Not Support This Platform!\n");
		return -1;
	}

	if (!sigpipe) {
		struct sigaction sigact;
		sigact.sa_sigaction = segv_handler;
		sigemptyset(&sigact.sa_mask);
		sigact.sa_flags = SA_SIGINFO;
		sigaction(SIGSEGV, &sigact, NULL);

		signal(SIGPIPE, signal_proc);
		// signal(SIGPIPE, SIG_IGN);
		sigpipe = 1;
	}

	if (rtspd_running != RTSPD_STATUS_STOP) {
		return ERR_RUNNING;
	}

	if (mutex_init) {
		// pthread_mutex_destroy(&filepath_mutex);
		pthread_mutex_destroy(&server_mutex);
		pthread_mutex_destroy(&interface_mutex);
		mutex_init = 0;
	}

	pthread_mutexattr_init(&attr);
	/* Used noly if __USE_UNIX98 is defined ??? */
#if GLIB //KERN_VERSION_26
	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#else
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
#endif
	if ((pthread_mutex_init(&interface_mutex, NULL)) ||
		(pthread_mutex_init(&server_mutex, NULL)) /*||
        (pthread_mutex_init(&filepath_mutex, NULL))*/) {
		ret = ERR_MUTEX;
	}
	pthread_mutexattr_destroy(&attr);
	if (ret) {
		dbg("thread mutex init error\n");
		return ret;
	}
	mutex_init = 1;

	if (pthread_mutex_trylock(&interface_mutex)) {
		return ERR_MUTEX;
	}

	if (((ipif) && ((strlen(ipif) < 2) || (IPIF_MAX <= strlen(ipif)))) ||
		(port < 0) || (dscp < 0) || (mtu < 0) || (cons <= 0) || (streams <= 0) || (GMSS_QNO_MAX < streams) ||
		(vqno < 0) || ((0 < vqno) && (vqlen <= 0)) ||
		(aqno < 0) || ((0 < aqno) && (aqlen <= 0)) ||
		((vqno == 0) && (aqno == 0)) || (GMSS_QNO_MAX < (vqno + aqno)) ||
		(frm_cb == NULL)) {
		ERR_GOTO(ERR_PARAM, init_err001);
	}

	rtspd_init = 0;

	/* Free allocated resources here. */
	test_free_null(gmss_srs);
	test_free_null(gmss_qus);

	/* Check ipif here ??? */
	if (ipif) {
		strcpy(gmss_ipif, ipif);
	} else {
		gmss_ipif[0] = 0;
	}
	rtsp_tcp_port = (port) ? port : RTSP_PORT_DEF;
	rtp_dscp = (dscp < 63) ? dscp : 63;
	rtp_mtu = (mtu > 0 && mtu <= MTU) ? mtu : MTU;
	gmss_conn_max = cons;

	if ((gmss_srs = (gmss_sr *) calloc(gmss_sr_max = streams, sizeof(gmss_sr))) == NULL) {
		ERR_GOTO(ERR_MEM, init_err001);
	}
	if ((gmss_qus = gmss_qu_alloc(vqno, vqlen, aqno, aqlen)) == NULL) {
		ERR_GOTO(ERR_MEM, init_err002);
	}

	rtspd_cb = frm_cb;
	rtspd_cmd_cb = cmd_cb;
	rtspd_init = 1;
	rtcp_enable = 1;
	goto init_err001;

init_err002:
	test_free_null(gmss_qus);
	test_free_null(gmss_srs);
init_err001:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}


int stream_server_file_init(char *path)
{
	int ret = 0;

	if (path == NULL) {
		return ERR_PARAM;
	}
	if (FILE_PREFIX_MAX <= strlen(path)) {
		return ERR_PARAM;
	}

	if ((!mutex_init) || (pthread_mutex_trylock(&interface_mutex))) {
		return ERR_MUTEX;
	}
	if (rtspd_running != RTSPD_STATUS_STOP) {
		ERR_GOTO(ERR_RUNNING, file_init_err);
	}
	// if (pthread_mutex_lock(&filepath_mutex)) ERR_GOTO(ERR_MUTEX, file_init_err);

	strcpy(gmss_file_prefix, path);

	// pthread_mutex_unlock(&filepath_mutex);
file_init_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}




int stream_server_start(void)
{
	int ret = 0;
	pthread_attr_t  attr;

	if ((!mutex_init) || (pthread_mutex_trylock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, ss_start_err);
	}
	if (rtspd_running != RTSPD_STATUS_STOP) {
		ERR_GOTO(ERR_RUNNING, ss_start_err);
	}
	// if (rtspd_cmd != STREAM_THREAD_NULL) ERR_GOTO(ERR_BUSY, ss_start_err);

	if (0 <= lif_sock) {
		close(lif_sock);
		lif_sock = -1;
		// unlink(RTSPMAN_USOCK_NAME);
	}

	lif_enq = 0;
#if (ENQ_TRIG_OPTIMIZED)    /* Carl, 20100203. */
	lif_enq_trig = 0;
#endif

#if (IPC_UDP)
	{
		struct sockaddr_in  addr;
		int                 val;
		struct linger       ling = {0, 0};

		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(RTSPMAN_PORT);
		inet_aton("127.0.0.1", &(addr.sin_addr));

		if ((lif_sock = socket(PF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0)) < 0) {
			ERR_GOTO(ERR_RESOURCE, ss_start_err);
		}

		val = 1;
		setsockopt(lif_sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
		// setsockopt(lif_sock, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
		val = 0;
		setsockopt(lif_sock, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));

		if ((bind(lif_sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) ||
			(connect(lif_sock, (struct sockaddr *) &addr, sizeof(addr)) < 0)) {
			ERR_GOTO(ERR_IPC, ss_start_err);
		}
	}
#else
	unlink(RTSPMAN_USOCK_NAME);

	if ((lif_sock = socket(PF_LOCAL, SOCK_RAW | SOCK_CLOEXEC, 0)) < 0) {
		ERR_GOTO(ERR_RESOURCE, ss_start_err);
	}
	if ((bind(lif_sock, (struct sockaddr *) &lif_sock_name, SUN_LEN(&lif_sock_name)) < 0) ||
		(connect(lif_sock, (struct sockaddr *) &lif_sock_name, SUN_LEN(&lif_sock_name)) < 0)) {
		ERR_GOTO(ERR_IPC, ss_start_err);
	}
#endif

	rtspd_cmd = STREAM_THREAD_START;
	rtspd_running = RTSPD_STATUS_START;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&gmss_thread_id, &attr, &rtspd_thread, NULL)) {
		ret = ERR_THREAD;
	}
	dbg("thread id %lu\n", gmss_thread_id);
	pthread_attr_destroy(&attr);

ss_start_err:
	if (ret) {
		rtspd_cmd = STREAM_THREAD_NULL;
		rtspd_running = RTSPD_STATUS_STOP;
		if (lif_sock) {
			close(lif_sock);
			lif_sock = -1;
#if (!IPC_UDP)
			unlink(RTSPMAN_USOCK_NAME);
#endif
		}
	}

	pthread_mutex_unlock(&interface_mutex);
	return ret;
}


/* Returns immediately, does NOT keep the caller waiting. */
int stream_server_stop(void)
{
	int             ret = 0;
	unsigned int    val = GMSS_CMD_STOP;

	if ((!mutex_init) || (pthread_mutex_trylock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, server_stop_err);
	}
	if (rtspd_running != RTSPD_STATUS_RUN) {
		ERR_GOTO(ERR_NOTRUN, server_stop_err);
	}
	if (rtspd_cmd != STREAM_THREAD_NULL) {
		ERR_GOTO(ERR_BUSY, server_stop_err);
	}

	rtspd_cmd = STREAM_THREAD_STOP;
	if (send(lif_sock, &val, sizeof(val), MSG_DONTWAIT | MSG_NOSIGNAL) <= 0) {
		ret = ERR_IPC;
	}

server_stop_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}


/* Returns immediately, does NOT keep the caller waiting. */
int stream_server_reset(void)
{
	int             ret = 0;
	unsigned int    val = GMSS_CMD_RESET;

	if ((!mutex_init) || (pthread_mutex_trylock(&interface_mutex))) {
		return ERR_MUTEX;
	}
	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, server_reset_err);
	}

	if (rtspd_running) {
		if (rtspd_cmd != STREAM_THREAD_NULL) {
			ERR_GOTO(ERR_BUSY, server_reset_err);
		}
		rtspd_cmd = STREAM_THREAD_RESET;
		if (send(lif_sock, &val, sizeof(val), MSG_DONTWAIT | MSG_NOSIGNAL) <= 0) {
			ERR_GOTO(ERR_IPC, server_reset_err);
		}
	} else {
		/* Release all queues and stream resources here. */
		reset_resources(1);
		test_free_null(gmss_localaddr);
		rtspd_init = 0;
	}

server_reset_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}


/* Reset the streaming server by cancelling the streaming thread.
   Returns immediately, does NOT keep the caller waiting. */
int stream_server_reset_force(void)
{
	int ret = 0;

	if ((!mutex_init) || (pthread_mutex_trylock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, stop_err);
	}
	if (rtspd_running == RTSPD_STATUS_STOP) {
		clean_resources((void *)CLEAN_TO_FORCE);
		rtspd_init = 0;
		goto stop_err;
	}

	if (gm_mutex_lock() == 0) {
		ret = pthread_cancel(gmss_thread_id);
		if (ret) {
			dbg("Cancel rtsp server thread 0x%u error %d.\n", (unsigned int)gmss_thread_id, ret);
		}
		clean_resources((void *)CLEAN_TO_FORCE);
		rtspd_init = 0;
		gm_mutex_unlock();
	} else {
		ret = ERR_MUTEX;
	}

stop_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}


/* < 0: error, 0: ready, 1: busy, 2: not running. */
int stream_server_status(void)
{
	int ret;

	if ((!mutex_init) || (pthread_mutex_trylock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (!rtspd_init) {
		ret = ERR_NOTINIT;
	} else if (!rtspd_running) {
		ret = SERVER_NOT_RUN;
	} else {
		ret = (rtspd_cmd == STREAM_THREAD_NULL) ? SERVER_READY : SERVER_BUSY;
	}

	pthread_mutex_unlock(&interface_mutex);
	return ret;
}

int stream_server_log_connection(int mode)
{
	int ret = 0;

	if ((!mutex_init) || (pthread_mutex_lock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (mode >= -1) {
		rtsp_log_conn_stat_mode = mode;
	} else {
		rtsp_log_conn_stat_mode = 0;
		ret = -1;
	}

	pthread_mutex_unlock(&interface_mutex);
	return ret;
}

int stream_server_signal_handle(int enable)
{
	struct sigaction sigact;

	if ((!mutex_init) || (pthread_mutex_lock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (enable) {
		if (!sigpipe) {

			sigact.sa_sigaction = segv_handler;
			sigemptyset(&sigact.sa_mask);
			sigact.sa_flags = SA_SIGINFO;
			sigaction(SIGSEGV, &sigact, NULL);

			signal(SIGPIPE, signal_proc);
			// signal(SIGPIPE, SIG_IGN);
			sigpipe = 1;
		}
	} else {
		if (sigpipe) {
			sigact.sa_handler = SIG_DFL;
			sigaction(SIGSEGV, &sigact, NULL);

			signal(SIGPIPE, SIG_DFL);
			sigpipe = 0;
		}
	}

	pthread_mutex_unlock(&interface_mutex);
	return 0;
}

/* Queue Manipulation */
int stream_queue_alloc(int type)
{
	int     i, n, ret = ERR_NOAVAIL, qn0 = 0, qn1 = gmss_qno[0];
	gmss_qu *q = NULL;

	if ((!mutex_init) || (pthread_mutex_trylock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, qalloc_err);
	}

	if ((n = media_type(type)) < 0) {
		ERR_GOTO(ERR_PARAM, qalloc_err);
	}

	if (n) {    // audio
		qn0 = gmss_qno[0];
		qn1 = gmss_qmax;    // gmss_qno[0]+gmss_qno[1]
	}

	for (i = qn0; i < qn1; ++i) {
		q = gmss_qus[i];
		if (q->used) {
			continue;
		}
		q->used = QUEUE_ALLOCATED;
		q->flush = 0;
		q->in = 0;
		q->out = 0;
		q->type = type;
		ret = i;
		break;
	}

qalloc_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}


int stream_queue_release(int qi)
{
	int ret = 0;

	if ((qi < 0) || (gmss_qmax <= qi)) {
		return ERR_PARAM;
	}
	if ((!mutex_init) || (pthread_mutex_trylock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, qrelease_err);
	}

	if (QUEUE_ALLOCATED < gmss_qus[qi]->used) {
		ERR_GOTO(ERR_BUSY, qrelease_err);
	}

	gmss_qus[qi]->used = 0;

qrelease_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}

/* Stream Registration & Deregistration */
int stream_reg(char *name, int vqno, char *vsdpstr, int aqno, char *asdpstr, int live,
			   unsigned int v_mc_ip, int v_mc_port, unsigned int a_mc_ip, int a_mc_port,
			   char *username, char *password)
{
	int     t, qnobase, i = 0, ret = ERR_NOAVAIL, server_lock = 0;
	gmss_sr *sr = NULL;
	gmss_qu *q;
	char    *sdpstr;
	struct in_addr v_dest_ip, a_dest_ip;
	int trylock_cnt = 0;
	//if ((!mutex_init) || (pthread_mutex_trylock(&interface_mutex))) return ERR_MUTEX;

	if (mutex_init) {
		while (pthread_mutex_trylock(&interface_mutex)) {
			trylock_cnt++;
			if (trylock_cnt > 1000) {
				return ERR_MUTEX;
			}
			usleep(1000);
		}
	} else {
		return ERR_MUTEX;
	}

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, stream_reg_err);
	}
	if (name == NULL) {
		ERR_GOTO(ERR_PARAM, stream_reg_err);
	}
	if (((vqno < 0) || (gmss_qno[MT_VIDEO] <= vqno)) &&
		((aqno < gmss_qno[MT_VIDEO]) || (gmss_qmax <= aqno))) {
		ERR_GOTO(ERR_PARAM, stream_reg_err);
	}

	//if (gm_mutex_trylock()) ERR_GOTO(ERR_MUTEX, stream_reg_err);
	trylock_cnt = 0;
	while (gm_mutex_trylock()) {
		trylock_cnt++;
		if (trylock_cnt > 1000) {
			ERR_GOTO(ERR_MUTEX, stream_reg_err);
		}
		usleep(1000);
	}
	server_lock = 1;

	for (i = 0; i < gmss_sr_max; ++i) {
		if (gmss_srs[i].status == SR_STATUS_NULL) {
			break;
		}
	}
	if (gmss_sr_max <= i) {
		ERR_GOTO(ERR_NOAVAIL, stream_reg_err);
	}

	sr = &(gmss_srs[i]);
	sr->live = live;
	sr->open = (live) ? live_open : live_open /* file_open ??? */;

	//test_free_null(sr->name);
	//if ((sr->name = malloc(strlen(name)+1)) == NULL) ERR_GOTO(ERR_MEM, stream_reg_err);
	//strcpy(sr->name, name);
	if (name && strlen(name) > 0 && strlen(name) < STREAM_NAME_LEN) {
		strcpy(sr->name, name);
	} else {
		memset((void *)sr->name, 0, STREAM_NAME_LEN);
		if (name && strlen(name) >= STREAM_NAME_LEN) {
			printf("%s error: stream name len > %d\n", __FUNCTION__, STREAM_NAME_LEN);
		}
	}

	strcpy(sr->realm, "Streaming Server");
	if (username && strlen(username) > 0 && strlen(username) < USERNAME_LEN &&
		password && strlen(password) > 0 && strlen(password) < PASSWORD_LEN) {
		strcpy(sr->username, username);
		strcpy(sr->password, password);
	} else {
		memset((void *)sr->username, 0, USERNAME_LEN);
		memset((void *)sr->password, 0, PASSWORD_LEN);
		if (username && password && (strlen(username) >= USERNAME_LEN || strlen(password) >= PASSWORD_LEN)) {
			printf("%s error: username len > %d or password len > %d\n", __FUNCTION__, USERNAME_LEN, PASSWORD_LEN);
		}
	}

	if ((sr->live_source = live_start_block((void *)sr)) == NULL) {
		ERR_GOTO(ERR_MEM, stream_reg_err);
	}

	for (t = 0; t < QNO_MAX; ++t) {
		sr->qno[t] = (t) ? (qnobase = gmss_qno[MT_VIDEO], aqno) : (qnobase = 0, vqno);
		sr->qus[t] = (qnobase <= sr->qno[t]) ? gmss_qus[sr->qno[t]] : NULL;
		if ((q = sr->qus[t]) != NULL) {
#if (LIVE_TRIG)
			if (QUEUE_ALLOCATED < q->used) {
				ERR_GOTO(ERR_PARAM, stream_reg_err);
			}
#endif
			++(q->used);
			/*if (q->sdpstr == NULL) {
			    if ((sdpstr = (t) ? asdpstr : vsdpstr)) {
			        if ((q->sdpstr = malloc(strlen(sdpstr)+1)) == NULL) ERR_GOTO(ERR_MEM, stream_reg_err);
			        strcpy(q->sdpstr, sdpstr);
			    }
			}*/
			if ((sdpstr = (t) ? asdpstr : vsdpstr)) {
				if (sdpstr && strlen(sdpstr) > 0 && strlen(sdpstr) < STREAM_SDP_LEN) {
					strcpy(q->sdpstr, sdpstr);
				} else {
					memset((void *)q->sdpstr, 0, STREAM_SDP_LEN);
					if (sdpstr && strlen(sdpstr) >= STREAM_SDP_LEN) {
						printf("%s error: sdpstr len > %d\n", __FUNCTION__, STREAM_SDP_LEN);
					}
				}
			}
			if (q->stream == NULL) {
				if ((q->stream = (void *)new_stream(q, q->type)) == NULL) {
					ERR_GOTO(ERR_MEM, stream_reg_err);
				}
			}
			if (live_set_track(sr, q) < 0) {
				ERR_GOTO(ERR_MEM, stream_reg_err);
			}
		}
	}

	sr->status = SR_STATUS_USED;
	sr->max_connection = 0;
	sr->range_npt_msec = 0;
	sr->bandwidth_info = 4000;

#if (RTP_OVER_UDP)
	if (v_mc_ip && (rtp_port_start < v_mc_port) && (v_mc_port < rtp_port_end) &&
		a_mc_ip && (rtp_port_start < a_mc_port) && (a_mc_port < rtp_port_end)) {
		sr->multicast.enabled = 1;
		v_dest_ip.s_addr = v_mc_ip;
		strcpy(sr->multicast.v_ipaddr, inet_ntoa(v_dest_ip));
		sr->multicast.v_port = v_mc_port;
		a_dest_ip.s_addr = a_mc_ip;
		strcpy(sr->multicast.a_ipaddr, inet_ntoa(a_dest_ip));
		sr->multicast.a_port = a_mc_port;
	} else
#endif
	{
		sr->multicast.enabled = 0;
	}

	ret = i;

stream_reg_err:
	if ((ret < 0) && (sr)) {
		struct live_source  *ls;
		//test_free_null(sr->name);
		memset((void *)sr->name, 0, STREAM_NAME_LEN);
		if ((ls = (struct live_source *)(sr->live_source))) {
			for (t = 0; t < QNO_MAX; ++t) {
				if ((q = sr->qus[t]) == NULL) {
					continue;
				}
				if (ls->track[t].rtp) {
					remove_rtp_media(ls->track[t].rtp);
				}
				if ((ls->track[t].stream)) {
					remove_dest(ls->track[t].stream);
				}
				if ((--(q->used)) <= QUEUE_ALLOCATED) {
					remove_stream((struct stream *)(q->stream));
					// q->stream = NULL;
					q->used = QUEUE_ALLOCATED;
				}
			}
			free(ls);
		}
		memset(sr, 0, sizeof(gmss_sr));
	}

	if (server_lock) {
		gm_mutex_unlock();
	}
	pthread_mutex_unlock(&interface_mutex);

	return ret;
}

int stream_rename(int sno, char *name)
{
	int ret = 0;

	if ((!mutex_init) || (pthread_mutex_lock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, stream_rename_err);
	}
	if (gmss_srs == NULL) {
		ERR_GOTO(ERR_RESOURCE, stream_rename_err);
	}
	if ((sno < 0) || (gmss_sr_max <= sno)) {
		ERR_GOTO(ERR_PARAM, stream_rename_err);
	}

	if (rtspd_running) {
		/*if (gmss_srs[sno].name)
		{
		    free(gmss_srs[sno].name);
		    if ((gmss_srs[sno].name = malloc(strlen(name)+1)) == NULL) ERR_GOTO(ERR_MEM, stream_rename_err);
		        strcpy(gmss_srs[sno].name, name);
		}*/
		if (name && strlen(name) > 0 && strlen(name) < STREAM_NAME_LEN) {
			strcpy(gmss_srs[sno].name, name);
		} else {
			memset((void *)gmss_srs[sno].name, 0, STREAM_NAME_LEN);
			if (name && strlen(name) >= STREAM_NAME_LEN) {
				printf("%s error: stream name len > %d\n", __FUNCTION__, STREAM_NAME_LEN);
			}
		}
	}

stream_rename_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}

int stream_updatesdp(int sno, char *vsdpstr, char *asdpstr)
{
	int ret = 0;
	int t;
	gmss_qu *q;
	char    *sdpstr;

	if ((!mutex_init) || (pthread_mutex_lock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, stream_updatesdp_err);
	}
	if (gmss_srs == NULL) {
		ERR_GOTO(ERR_RESOURCE, stream_updatesdp_err);
	}
	if ((sno < 0) || (gmss_sr_max <= sno)) {
		ERR_GOTO(ERR_PARAM, stream_updatesdp_err);
	}

	if (rtspd_running) {
		for (t = 0; t < QNO_MAX; ++t) {
			if ((q = gmss_srs[sno].qus[t]) != NULL) {
				if ((sdpstr = (t) ? asdpstr : vsdpstr)) {
					if (sdpstr && strlen(sdpstr) > 0 && strlen(sdpstr) < STREAM_SDP_LEN) {
						strcpy(q->sdpstr, sdpstr);
					} else {
						memset((void *)q->sdpstr, 0, STREAM_SDP_LEN);
						if (sdpstr && strlen(sdpstr) >= STREAM_SDP_LEN) {
							printf("%s error: sdpstr len > %d\n", __FUNCTION__, STREAM_SDP_LEN);
						}
					}
				}
			}
		}
	}

stream_updatesdp_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}

int stream_authorization(int sno, char *username, char *password)
{
	int ret = 0;

	if ((!mutex_init) || (pthread_mutex_lock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, stream_authorization_err);
	}
	if (gmss_srs == NULL) {
		ERR_GOTO(ERR_RESOURCE, stream_authorization_err);
	}
	if ((sno < 0) || (gmss_sr_max <= sno)) {
		ERR_GOTO(ERR_PARAM, stream_authorization_err);
	}

	if (rtspd_running) {
		if (username && strlen(username) > 0 && strlen(username) < USERNAME_LEN &&
			password && strlen(password) > 0 && strlen(password) < PASSWORD_LEN) {
			strcpy(gmss_srs[sno].username, username);
			strcpy(gmss_srs[sno].password, password);
		} else {
			memset((void *)gmss_srs[sno].username, 0, USERNAME_LEN);
			memset((void *)gmss_srs[sno].password, 0, PASSWORD_LEN);
			if (username && password && (strlen(username) >= USERNAME_LEN || strlen(password) >= PASSWORD_LEN)) {
				printf("%s error: username len > %d or password len > %d\n", __FUNCTION__, USERNAME_LEN, PASSWORD_LEN);
			}
		}
	}

stream_authorization_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}

int stream_max_connection(int sno, int max_connection)
{
	int ret = 0;

	if ((!mutex_init) || (pthread_mutex_lock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, stream_max_connection_err);
	}
	if (gmss_srs == NULL) {
		ERR_GOTO(ERR_RESOURCE, stream_max_connection_err);
	}
	if ((sno < 0) || (gmss_sr_max <= sno)) {
		ERR_GOTO(ERR_PARAM, stream_max_connection_err);
	}

	if (rtspd_running) {
		if (max_connection >= 0) {
			gmss_srs[sno].max_connection = max_connection;
		} else {
			ret = -1;
		}
	}

stream_max_connection_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}

int stream_range_npt(int sno, unsigned int msec)
{
	int ret = 0;

	if ((!mutex_init) || (pthread_mutex_lock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, stream_range_npt_err);
	}
	if (gmss_srs == NULL) {
		ERR_GOTO(ERR_RESOURCE, stream_range_npt_err);
	}
	if ((sno < 0) || (gmss_sr_max <= sno)) {
		ERR_GOTO(ERR_PARAM, stream_range_npt_err);
	}

	if (rtspd_running) {
		if (msec) {
			gmss_srs[sno].range_npt_msec = msec;
		} else {
			ret = -1;
		}
	}

stream_range_npt_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}

int stream_clock_rate(int sno, unsigned int video_hz, unsigned int audio_hz)
{
	struct live_source  *ls;
	int ret = 0;

	if ((!mutex_init) || (pthread_mutex_lock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, stream_clock_rate_err);
	}
	if (gmss_srs == NULL) {
		ERR_GOTO(ERR_RESOURCE, stream_clock_rate_err);
	}
	if ((sno < 0) || (gmss_sr_max <= sno)) {
		ERR_GOTO(ERR_PARAM, stream_clock_rate_err);
	}

	if (rtspd_running) {
		if (video_hz || audio_hz) {
			if ((ls = (struct live_source *)(gmss_srs[sno].live_source))) {
				if (video_hz && ls->track[0].rtp) {
					ls->track[0].rtp->clock_rate_hz = video_hz;
				}
				if (audio_hz && ls->track[1].rtp) {
					ls->track[1].rtp->clock_rate_hz = audio_hz;
				}
			} else {
				ret = -1;
			}
		} else {
			ret = -1;
		}
	}

stream_clock_rate_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}

int stream_bandwidth_info(int sno, unsigned int kbps)
{
	int ret = 0;

	if ((!mutex_init) || (pthread_mutex_lock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, stream_range_npt_err);
	}
	if (gmss_srs == NULL) {
		ERR_GOTO(ERR_RESOURCE, stream_range_npt_err);
	}
	if ((sno < 0) || (gmss_sr_max <= sno)) {
		ERR_GOTO(ERR_PARAM, stream_range_npt_err);
	}

	if (rtspd_running) {
		gmss_srs[sno].bandwidth_info = kbps;
	}

stream_range_npt_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}

int stream_disconnect(int sno)
{
	int ret = 0;
	int t;

	if ((!mutex_init) || (pthread_mutex_lock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, stream_disconnect_err);
	}
	if (gmss_srs == NULL) {
		ERR_GOTO(ERR_RESOURCE, stream_disconnect_err);
	}
	if ((sno < 0) || (gmss_sr_max <= sno)) {
		ERR_GOTO(ERR_PARAM, stream_disconnect_err);
	}

	if (gmss_srs[sno].status == SR_STATUS_NULL) {
		ERR_GOTO(ERR_PARAM, stream_disconnect_err);
	}
	if (gmss_srs[sno].status != SR_STATUS_USED) {
		ERR_GOTO(ERR_BUSY, stream_disconnect_err);
	}

	for (t = 0; t < QNO_MAX; ++t) {
		if (gmss_srs[sno].qus[t] != NULL) {
			disconnect_session_client(gmss_srs[sno].qus[t]->stream);
		}
	}

stream_disconnect_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}

int stream_disconnect_force(int sno, struct sockaddr_in addr)
{
	int ret = 0;
	struct live_source  *ls;
	struct rtp_endpoint *ep;
	struct live_session *lsession;
	gmss_sr *sr = &(gmss_srs[sno]);

	if ((!mutex_init) || (pthread_mutex_lock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, stream_disconnect_force_err);
	}
	if (gmss_srs == NULL) {
		ERR_GOTO(ERR_RESOURCE, stream_disconnect_force_err);
	}
	if ((sno < 0) || (gmss_sr_max <= sno)) {
		ERR_GOTO(ERR_PARAM, stream_disconnect_force_err);
	}

	if (gmss_srs[sno].status == SR_STATUS_NULL) {
		ERR_GOTO(ERR_PARAM, stream_disconnect_force_err);
	}
	if (gmss_srs[sno].status != SR_STATUS_USED) {
		ERR_GOTO(ERR_BUSY, stream_disconnect_force_err);
	}

	// Close all live sessions here.
	ls = (struct live_source *)(sr->live_source);
	lsession = ls->sess_list;
	while (lsession) {
		if (lsession->sess == NULL) {
			dbg("live_session->sess NULL\n");
			break;
		}
		ep = (lsession->sess->ep[0]) ? lsession->sess->ep[0] : lsession->sess->ep[1];
		if (ep == NULL) {
			dbg("live_session->sess->ep[*] NULL\n");
			break;
		}

		if (ep->trans_type == RTP_TRANS_INTER && ep->trans.inter.conn) {
			if (ep->trans.inter.conn->client_addr.sin_addr.s_addr == addr.sin_addr.s_addr &&
				ep->trans.inter.conn->client_addr.sin_port == addr.sin_port) {
				/*if (ep->session->teardown)
				    ep->session->teardown(ep->session, NULL, 1);*/
				drop_conn(ep->trans.inter.conn, 0);
				goto stream_disconnect_force_err;
			} else {
				lsession = lsession->next;
			}
		}
#if (RTP_OVER_UDP)
		else if (ep->trans_type == RTP_TRANS_UDP && ep->trans.udp.conn) {
			if (ep->trans.udp.conn->client_addr.sin_addr.s_addr == addr.sin_addr.s_addr &&
				ep->trans.udp.conn->client_addr.sin_port == addr.sin_port) {
				/*if (ep->session->teardown)
				    ep->session->teardown(ep->session, NULL, 1);*/
				drop_conn(ep->trans.udp.conn, 0);
				goto stream_disconnect_force_err;
			} else {
				lsession = lsession->next;
			}
		}
#endif
		else {
			dbg("live_session->sess->ep[]->trans.inter.conn NULL\n");
			printf("%s(100)\n", __FUNCTION__);
			break;
		}
	}

stream_disconnect_force_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}

int stream_dereg(int sno, int free_queue)
{
	int ret = 0, server_lock = 0;
	unsigned int    val = (free_queue) ? GMSS_CMD_DEREG_FQ : GMSS_CMD_DEREG;
	unsigned int    val_1;
	int t;
	gmss_sr *sr = NULL;
	int trylock_cnt, tmp;

	//if ((!mutex_init) || (pthread_mutex_lock(&interface_mutex))) return ERR_MUTEX;
	if (!mutex_init) {
		return ERR_MUTEX;
	}
	trylock_cnt = 0;
	while ((tmp = pthread_mutex_lock(&interface_mutex))) {
		trylock_cnt++;
		if (trylock_cnt > 1000) {
			ERR_GOTO(ERR_MUTEX, stream_dereg_err);
		}
		usleep(1000);
	}

	//if (gm_mutex_trylock()) ERR_GOTO(ERR_MUTEX, stream_reg_err);
	trylock_cnt = 0;
	while ((tmp = gm_mutex_trylock())) {
		trylock_cnt++;
		if (trylock_cnt > 1000) {
			ERR_GOTO(ERR_MUTEX, stream_dereg_err);
		}
		usleep(1000);
	}
	server_lock = 1;

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, stream_dereg_err);
	}
	if (gmss_srs == NULL) {
		ERR_GOTO(ERR_RESOURCE, stream_dereg_err);
	}
	if ((sno < 0) || (gmss_sr_max <= sno)) {
		ERR_GOTO(ERR_PARAM, stream_dereg_err);
	}

	if (rtspd_running) {
		if (gmss_srs[sno].status == SR_STATUS_NULL) {
			ERR_GOTO(ERR_PARAM, stream_dereg_err);
		}
		if (gmss_srs[sno].status != SR_STATUS_USED) {
			ERR_GOTO(ERR_BUSY, stream_dereg_err);
		}
		gmss_srs[sno].status = /*(immediate) ?*/ SR_STATUS_QIMMEDIATE /*: SR_STATUS_QEMPTY*/;
		val |= (unsigned int)sno;

		sr = &(gmss_srs[sno]);

		for (t = 0; t < QNO_MAX; ++t) {
			if (gmss_srs[sno].qus[t] != NULL) {
				disconnect_session_client(gmss_srs[sno].qus[t]->stream);
			}
		}

		val_1 = GMSS_CMD_GOODBYE; /* to trigger rtcp_goodbye to be processed */
		if (send(lif_sock, &val_1, sizeof(val_1), MSG_DONTWAIT | MSG_NOSIGNAL) <= 0) {
			ERR_GOTO(ERR_IPC, stream_dereg_err);
		}

		if (sr->multicast.enabled) {
			if (sr->multicast.session) {
				if (sr->multicast.session->teardown) {
					sr->multicast.session->teardown(sr->multicast.session, NULL, 0);
				}
			}
		}

		if (send(lif_sock, &val, sizeof(val), MSG_DONTWAIT | MSG_NOSIGNAL) <= 0) {
			ERR_GOTO(ERR_IPC, stream_dereg_err);    // -2
		}

		//test_free_null(gmss_srs[sno].name);
		memset((void *)gmss_srs[sno].name, 0, STREAM_NAME_LEN);
	} else {
		gmss_sr_reset(&(gmss_srs[sno]), free_queue);
	}

stream_dereg_err:
	if (server_lock) {
		gm_mutex_unlock();
	}
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}


/* -1: error, 0: not streaming, 1 : streaming. */
int stream_status(int sno)
{
	int ret = 0;
	struct live_source  *ls;

	if ((!mutex_init) || (pthread_mutex_trylock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if (gmss_srs == NULL) {
		ERR_GOTO(ERR_NOTINIT, stream_status_err);
	}
	if ((sno < 0) || (gmss_sr_max <= sno) || (gmss_srs[sno].status == SR_STATUS_NULL)) {
		ERR_GOTO(ERR_PARAM, stream_status_err);
	}

	if (gm_mutex_trylock()) {
		ERR_GOTO(ERR_MUTEX, stream_status_err);
	}

	ls = (struct live_source *)(gmss_srs[sno].live_source);
	if (ls) {
		ret = (ls->sess_list) ? STREAM_STREAMING : STREAM_NOT_STREAMING;
	} else {
		ret = ERR_OTHER;
	}

	gm_mutex_unlock();

stream_status_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}


int stream_reset(int free_queue)
{
	int             ret = 0;
	unsigned int    val = (free_queue) ? GMSS_CMD_SRESET_FQ : GMSS_CMD_SRESET;

	if ((!mutex_init) || (pthread_mutex_trylock(&interface_mutex))) {
		return ERR_MUTEX;
	}

	if ((!rtspd_init) || (gmss_srs == NULL)) {
		ERR_GOTO(ERR_NOTINIT, stream_reset_err);
	}

	if (rtspd_running) {
		if (send(lif_sock, &val, sizeof(val), MSG_DONTWAIT | MSG_NOSIGNAL) <= 0) {
			ERR_GOTO(ERR_IPC, stream_reset_err);
		}
	} else {
		reset_all_streams(free_queue);
	}

stream_reset_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}


/* Media Manipulation */
int stream_media_enqueue(int type, int qi, gm_ss_entity *entity)
{
	int     mt, qn0, qn1, in, ret = 0, trig = 0;
	gmss_qu         *q;
	gm_ss_entity    *ent;
	struct frame    *f;
	unsigned int    val = GMSS_CMD_ENQ;

	if ((entity == NULL) || (entity->size <= 0) || (entity->data == NULL)) {
		return ERR_PARAM;
	}
#if 0
	if ((!mutex_init) || (pthread_mutex_trylock(&interface_mutex))) {
		return ERR_MUTEX;
	}
#else
	int trylock_cnt = 0;

	if (mutex_init) {
		while (pthread_mutex_trylock(&interface_mutex)) {
			trylock_cnt++;
			if (trylock_cnt > 1000) {
				return ERR_MUTEX;
			}
			usleep(1000);
		}
	} else {
		return ERR_MUTEX;
	}
#endif

	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, menq_err);
	}
	if (rtspd_running != RTSPD_STATUS_RUN) {
		ERR_GOTO(ERR_NOTRUN, menq_err);
	}
	if ((mt = media_type(type)) < 0) {
		ERR_GOTO(ERR_PARAM, menq_err);
	}
	qn0 = (mt == MT_VIDEO) ? (qn1 = gmss_qno[MT_VIDEO], 0) : (qn1 = gmss_qmax, gmss_qno[MT_VIDEO]);
	if ((qi < qn0) | (qn1 <= qi)) {
		ERR_GOTO(ERR_PARAM, menq_err);
	}

	if ((q = gmss_qus[qi]) == NULL) {
		ERR_GOTO(ERR_PARAM, menq_err);
	}
	if ((q->used <= QUEUE_ALLOCATED) || (q->type != type)) {
		ERR_GOTO(ERR_PARAM, menq_err);
	}

	in = (q->in + 1) % (q->size);
	if (in == q->out) {
		trig = 1;
		ERR_GOTO(ERR_FULL, menq_full);
	}
	// q->queue[q->in] = (void *) entity;
	ent = &(q->queue[q->in].ent);
	memcpy(ent, entity, sizeof(gm_ss_entity));
	/* init frame */
	f = &(q->queue[q->in].f);
	// memset(f, 0, sizeof(struct frame));
	f->ref_count = 1;
	f->length = ent->size;
	f->d = (unsigned char *)(ent->data);
	f->format = q->type;
	f->qno = qi;
	f->ent = ent;
	q->in = in;

#if (!ENQ_TRIG_OPTIMIZED)   /* Carl, 20100203. */
	if (ENQ_TRIG_COUNT <= (++enq_trig_count)) {
		enq_trig_count = 0;
		trig = 1;
	}
#endif

menq_full:

#if (ENQ_TRIG_OPTIMIZED)    /* Carl, 20100203. */
	trig = (int)(lif_enq_trig - lif_enq);
	if (trig <= 0)
#else
	if ((trig) && (lif_enq == 0))
#endif
	{
#if (!ENQ_TRIG_OPTIMIZED)   /* Carl, 20100203. */
		lif_enq = 1;    // not good ???
#endif
		val |= (unsigned int)qi;
		if (send(lif_sock, &val, sizeof(val), MSG_DONTWAIT | MSG_NOSIGNAL) <= 0) {
			ERR_GOTO(ERR_IPC, menq_err);
		}
#if (ENQ_TRIG_OPTIMIZED)    /* Carl, 20100203. */
		else {
			++lif_enq_trig; /* Carl, 20100205. */
		}
#endif
	}

menq_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}


int stream_media_flush(int type, int qi)
{
	int     mt, qn0, qn1, ret = 0;
	gmss_qu *q;
	unsigned int    val = GMSS_CMD_FLUSH;

	if ((!mutex_init) || (pthread_mutex_trylock(&interface_mutex))) {
		return ERR_MUTEX;
	}
	if (!rtspd_init) {
		ERR_GOTO(ERR_NOTINIT, mflush_err);
	}

	if ((mt = media_type(type)) < 0) {
		ERR_GOTO(ERR_PARAM, mflush_err);
	}
	qn0 = (mt == MT_VIDEO) ? (qn1 = gmss_qno[MT_VIDEO], 0) : (qn1 = gmss_qmax, gmss_qno[MT_VIDEO]);
	if ((qi < qn0) | (qn1 <= qi)) {
		ERR_GOTO(ERR_PARAM, mflush_err);
	}

	if ((q = gmss_qus[qi]) == NULL) {
		ERR_GOTO(ERR_PARAM, mflush_err);
	}
	if ((q->used <= QUEUE_ALLOCATED) || (q->type != type)) {
		ERR_GOTO(ERR_PARAM, mflush_err);
	}

	if (rtspd_running) {
		q->flush = 1;
		val |= (unsigned int)qi;
		if (send(lif_sock, &val, sizeof(val), MSG_DONTWAIT | MSG_NOSIGNAL) <= 0) {
			ERR_GOTO(ERR_IPC, mflush_err);
		}
	} else {
		q->flush = q->out = q->in = 0;
	}

mflush_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}


/* Be undocumented. */
int media_resync(int *resync)
{
	if (resync) {
		gmss_av_resync = *resync;
	}
	return gmss_av_resync;
}


gm_ss_sr_t *sr_clients(int sno)
{
	struct live_source  *ls;
	struct live_session *lsess;
	struct rtp_endpoint *ep;
	gm_ss_clnt_t    *client;
	gm_ss_sr_t      *srtable = NULL;
	gmss_sr         *sr;

	if (gmss_srs == NULL) {
		return NULL;
	}
	if ((sr = &(gmss_srs[sno])) == NULL) {
		return NULL;
	}
	if (sr->status == SR_STATUS_NULL) {
		return NULL;
	}
	if ((ls = (struct live_source *)(sr->live_source)) == NULL) {
		return NULL;
	}
	if ((lsess = ls->sess_list) == NULL) {
		return NULL;
	}
	if ((srtable = calloc(1, sizeof(gm_ss_sr_t))) == NULL) {
		return NULL;
	}
	if ((srtable->name = malloc(strlen(sr->name) + 1)) == NULL) {
		free(srtable);
		return NULL;
	}
	srtable->index = sno;
	strcpy(srtable->name, sr->name);

	while (lsess) {
		if (lsess->sess == NULL) {
			break;
		}
		ep = (lsess->sess->ep[0]) ? lsess->sess->ep[0] : lsess->sess->ep[1];
		if (ep == NULL) {
			break;
		}
#if (RTP_OVER_UDP)
		if (ep->trans_type == RTP_TRANS_UDP && ep->trans.udp.conn == NULL) {
			break;
		}
#endif
		if (ep->trans_type == RTP_TRANS_INTER && ep->trans.inter.conn == NULL) {
			break;
		}
		if ((client = calloc(1, sizeof(gm_ss_clnt_t))) == NULL) {
			break;
		}
		switch (ep->trans_type) {
#if (RTP_OVER_UDP)
		case RTP_TRANS_UDP:
			memcpy(&(client->addr), &(ep->trans.udp.conn->client_addr), sizeof(struct sockaddr_in));
			break;
#endif
		case RTP_TRANS_INTER:
			memcpy(&(client->addr), &(ep->trans.inter.conn->client_addr), sizeof(struct sockaddr_in));
			break;
		}
		client->next = srtable->client;
		srtable->client = client;
		lsess = lsess->next;
	}

	return srtable;
}


int stream_query_clients(int type, int sno, gm_ss_sr_t **client_tree)
{
	int         i, ret  = 0;
	gm_ss_sr_t  *st, *srtable = NULL;
	gmss_sr     *sr;

	if ((!mutex_init) || (pthread_mutex_trylock(&interface_mutex))) {
		return ERR_MUTEX;
	}
	if ((!rtspd_init) || (gmss_srs == NULL)) {
		ERR_GOTO(ERR_NOTINIT, qclient_err);
	}
	if (rtspd_running != RTSPD_STATUS_RUN) {
		ERR_GOTO(ERR_NOTRUN, qclient_err);
	}
	if (((type == GM_SS_QUERY_SR) && (gmss_sr_max <= sno)) ||
		(type < GM_SS_QUERY_SR) || (GM_SS_QUERY_ALL < type)) {
		ERR_GOTO(ERR_PARAM, qclient_err);
	}
	if (gm_mutex_trylock()) {
		ERR_GOTO(ERR_MUTEX, qclient_err);
	}

	switch (type) {
	case GM_SS_QUERY_SR:
		if (0 <= sno) {
			srtable = sr_clients(sno);
		}
		break;
	default:
		for (i = gmss_sr_max - 1; 0 <= i; --i) {
			sr = &(gmss_srs[i]);
			if (((type == GM_SS_QUERY_LIVE) && (!(sr->live))) ||
				((type == GM_SS_QUERY_FILE) && (sr->live))) {
				continue;
			}
			st = sr_clients(i);
			if (st == NULL) {
				continue;
			}
			st->next = srtable;
			srtable = st;
		}
		break;
	}

	*client_tree = srtable;

	gm_mutex_unlock();
qclient_err:
	pthread_mutex_unlock(&interface_mutex);
	return ret;
}


int stream_query_clients_free_tree(gm_ss_sr_t *client_tree)
{
	gm_ss_clnt_t *client_info, *next_client_info;

	if (client_tree) {
		client_info = client_tree->client;
		while (client_info) {
			next_client_info = client_info->next;
			free(client_info);
			client_info = next_client_info;
		}
		if (client_tree->name) {
			free(client_tree->name);
		}
		free(client_tree);
	}

	return 0;
}


int stream_query_clients_num(int sno)
{
	struct live_source  *ls;
	struct live_session *lsess;
	struct rtp_endpoint *ep;
	gmss_sr         *sr;
	int client_cnt = 0;

	if (gmss_srs == NULL) {
		return 0;
	}
	if ((sr = &(gmss_srs[sno])) == NULL) {
		return 0;
	}
	if (sr->status == SR_STATUS_NULL) {
		return 0;
	}
	if ((ls = (struct live_source *)(sr->live_source)) == NULL) {
		return 0;
	}
	if ((lsess = ls->sess_list) == NULL) {
		return 0;
	}

	while (lsess) {
		if (lsess->sess == NULL) {
			break;
		}
		ep = (lsess->sess->ep[0]) ? lsess->sess->ep[0] : lsess->sess->ep[1];
		if ((ep == NULL) || (ep->trans.inter.conn == NULL)) {
			break;
		}
		lsess = lsess->next;
		client_cnt++;
	}

	return client_cnt;
}


char *stream_query_localaddr(void)
{
	char    *laddr  = NULL;

	if ((!mutex_init) || (pthread_mutex_trylock(&interface_mutex))) {
		return NULL;
	}
	if ((!rtspd_init) || (rtspd_running != RTSPD_STATUS_RUN)) {
		goto query_laddr_err;
	}

	if (gmss_localaddr) {
		laddr = malloc(strlen(gmss_localaddr) + 1);
		if (laddr) {
			strcpy(laddr, gmss_localaddr);
		}
	}

query_laddr_err:
	pthread_mutex_unlock(&interface_mutex);
	return laddr;
}


char *stream_query_version(void)
{
	return version;
}

void stream_sdp_parameter_encoder(char *type, unsigned char *data, int len, char *sdp, int sdp_len_max)
{
	if (!strcmp(type, "H264")) {
		sdp_parameter_encoder_h264(data, len, sdp, sdp_len_max);
	} else if (!strcmp(type, "H265")) {
		sdp_parameter_encoder_h265(data, len, sdp, sdp_len_max);
	} else if (!strcmp(type, "MPEG4")) {
		sdp_parameter_encoder_mpeg4(data, len, sdp, sdp_len_max);
	} else if (!strcmp(type, "MJPEG")) {
		sdp_parameter_encoder_mjpeg(data, len, sdp, sdp_len_max);
	} else if (!strcmp(type, "AAC")) {
		sdp_parameter_encoder_aac(data, len, sdp, sdp_len_max);
	}
}

void librtsp_setup_rtcp(int enable)
{
	rtcp_enable = enable;
}

int librtsp_get_rtcp_status(void)
{
	return rtcp_enable;
}
