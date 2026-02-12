#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <netinet/in.h>

#include "sysdef.h" /* Carl, 20091029. */

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <rtp.h>
#include <rtp_media.h>
#include <net/if.h>
#include <netinet/tcp.h>

#define abs(X)    (((X)>0)?(X):-(X))


int get_tcpNuOfbuf(struct rtp_endpoint *ep)
{
	int buflen, error;

	error = ioctl(ep->trans.inter.conn->fd, TIOCOUTQ, &buflen);
	if (error) {
		dbg("%s:%d <Get the number of TCP buffer error!!>\n", __FUNCTION__, __LINE__);
	}
	return buflen;
}


int check_network_ready(struct rtp_endpoint *ep, short len)
{
	int fd_max;
	fd_set wfds;
	struct timeval tv;
	int ret, qnum;

	/* 96 for Mpeg4 payload type */
	if (ep->trans_type != RTP_TRANS_INTER) {
		return 1;
	}

	qnum = get_tcpNuOfbuf(ep);

	if (qnum > 0x14000)
//	if(qnum > 0x06000)
	{
		fd_max = ep->trans.inter.conn->fd;
		FD_ZERO(&wfds);
		FD_SET(ep->trans.inter.conn->fd, &wfds);
		if (ep->packetskip) {
			tv.tv_sec = 0;
			tv.tv_usec = 10;
			ret = select(fd_max + 1, NULL, &wfds, NULL, &tv);
		} else {
			tv.tv_sec = 1;
			tv.tv_usec = 100 * 1000;
			ret = select(fd_max + 1, NULL, &wfds, NULL, &tv);
		}

		if (ret == 0/* && ep->payload == 96*/) {
			ep->seqnum = (ep->seqnum + 1) & 0xFFFF;
			ep->packetskip++;
			if (ep->packetskip > 6000) {
				//printf("Close Socket\n");
				return -1;
			}
			return 0;
		} else if (ret == -1) {
			printf("%s:%d<Close Socket !!>\n", __FUNCTION__, __LINE__);
			return -1;
		}
	}

	return 1;
}


#if 0   // Carl
void gm_check(struct rtp_endpoint *ep)
{
	struct session *session;
	struct rtp_endpoint *tmp_ep;
	int i, payload, diff;
	unsigned int timestamp;
	static int cnt = 0;

	/* timestamp: sound's time, tmp_ep->last_timestamp:Video's time */
	timestamp = ep->last_timestamp;
	payload = ep->payload;
	session = ep->session;
	for (i = 0; i < MAX_TRACKS; i++) {
		tmp_ep = session->ep[i];
		if ((tmp_ep == NULL) || (payload == tmp_ep->payload)) {
			continue;
		}
		diff = timestamp - tmp_ep->last_timestamp;
		if (abs(diff) > 10000000) {
			dbg("%s:%d <diff = %d, tp=%d, last_tp=%d>\n", __FUNCTION__, __LINE__, abs(diff), timestamp, tmp_ep->last_timestamp);
			tmp_ep->last_timestamp += diff;
			break;
		}
		if (abs(diff) < 9000) {
			cnt = 0;
			break;
		}
		if (++cnt > 10) {
			if (diff > 0) {
				tmp_ep->last_timestamp += (diff / 10);
			} else {
				tmp_ep->last_timestamp -=  50;
			}
		}
	}
}
#endif


// Carl
#define RTP_HZ      90000
extern int  gmss_av_resync;


// Carl
void avsync(struct rtp_endpoint *ep, unsigned int curr_time)
{
	struct session      *sess;
	struct rtp_endpoint *aep;
	int                 t;

	if (ep == NULL) {
		return;
	}

	if (ep->timed == 0) {
		if (gmss_av_resync == 0) {
			goto avsync_alone;
		}
		if ((sess = ep->session) == NULL) {
			goto avsync_alone;
		}
		t = (ep == sess->ep[0]) ? 1 : 0;
		aep = sess->ep[t];
		if ((aep == NULL) || (!(aep->timed))) {
avsync_alone:
			ep->timebase = curr_time;
		} else {
			ep->timebase = (0 <= ((int)(curr_time - (aep->timebase + aep->last_timestamp)))) ?
						   aep->timebase : curr_time - aep->last_timestamp;
		}
		ep->timed = 1;
	}

	ep->last_timestamp = curr_time - ep->timebase;
}

unsigned int rtcp_timestamp_adjust(struct rtp_endpoint *ep, unsigned int curr_time)
{
	if (ep == NULL) {
		return curr_time;
	}

	if (ep->timed) {
		return curr_time - ep->timebase;
	}

	return curr_time;
}

// Carl
unsigned int ticknhz(struct rtp_endpoint *ep, unsigned int nhz)
{
	if (ep == NULL) {
		return 0;
	}
	ep->remainder += ep->last_timestamp - ep->nhz_last_timestamp;
	ep->nhz_last_timestamp = ep->last_timestamp;
	ep->nhz += (ep->remainder / RTP_HZ) * nhz;
	ep->remainder %= RTP_HZ;
	return ep->nhz + (((ep->remainder / 10) * nhz) / (RTP_HZ / 10));
}


#if 1   // Carl
void ref_frame(struct frame *f)
{
	pthread_mutex_lock(&f->mutex);   // Carl
	++f->ref_count;
	pthread_mutex_unlock(&f->mutex);
}
#endif

