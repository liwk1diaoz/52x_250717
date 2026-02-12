/*
 * Copyright (C) 2004 Nathan Lutchansky <lutchann@litech.org>
 * Copyright (C) 2006 Jen-Yu Yu <babyfish@vstream.com.tw>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "sysdef.h" /* Carl, 20091029. */

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <rtp.h>
#include <rtp_media.h>
#include "live.h"   // Carl
#include "gm_memory.h"

extern int rtp_port_start, rtp_port_end, rtp_dscp, rtp_mtu;
int check_network_ready(struct rtp_endpoint *ep, short len);
unsigned int rtcp_timestamp_adjust(struct rtp_endpoint *ep, unsigned int curr_time);

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define TIMEVAL_DIFF_USEC(start, end) (((end.tv_sec)-(start.tv_sec))*1000000+((end.tv_usec)-(start.tv_usec)))
#define TIMEVAL_DIFF_SEC(start, end) ((end.tv_sec)-(start.tv_sec))
#define TIMEVAL_DIFF(start, end) (((end.tv_sec)-(start.tv_sec))*1000000+((end.tv_usec)-(start.tv_usec)))

//#define RTP_SPEED_LIMIT
#define RTP_SPEED_UPPER_BOUND   50  //Mbps

//#define RTCP_TIMESTAMP_ADJUST

//kevin 2006.12.25
static void rtcp_app_fire(struct event_info *ei, void *d)
{
	struct rtp_endpoint *ep = (struct rtp_endpoint *)d;
	//kevin
	//kevin 2007.1.17
	ep->enable_send_rtcp = 1;
	//rtcp_app_send( ep );
}


struct rtp_endpoint *new_rtp_endpoint(int payload)
{
	struct rtp_endpoint *ep;


	if (!(ep = (struct rtp_endpoint *)
			   malloc(sizeof(struct rtp_endpoint)))) {
		return NULL;
	}

	memset(ep, 0x0, sizeof(struct rtp_endpoint));

	ep->payload = payload;
	ep->max_data_size = rtp_mtu; /* default maximum */
	ep->ssrc = 0;

	//kevin 2006.12.24
	random_bytes((unsigned char *)&ep->ssrc, 4);
	//ep->ssrc = 0x00004823;

	// Carl
	// ep->sync_timestamp = ep->last_timestamp = ep->start_timestamp = ep->timebase = 0;

	//kevin 2006.11.24
	//ep->start_timestamp = 4294000000;
	ep->seqnum = 0;
	ep->packetskip = 0;
	ep->delay_time = 0;
	ep->stopdetect = 0;

//#ifndef WMP
	random_bytes((unsigned char *)&ep->seqnum, 2);
//#endif
	ep->packet_count = 0;
	ep->octet_count = 0;
	ep->event_enabled = 0; //kevin rtcp enabling flag
//#ifdef RTCP_ENABLE
	if (rtcp_enable) {
//kevin 2007.1.12 fixme
		if (1 || (payload == 96) || (payload == 26) || (payload == 99)) { /*RTCP is only enabled for Video session*/
			ep->force_rtcp = 1;
			//kevin 2007.1.17
			ep->enable_send_rtcp = 1;
			//kevin 2006.12.25
			//ep->rtcp_send_event = add_timer_event( 5000, 0, rtcp_fire, ep );
#if (RTCP_USER)
			ep->rtcp_send_event = add_timer_event(30000, 0, rtcp_app_fire, ep);
#else
			ep->rtcp_send_event = add_timer_event(5000, 0, rtcp_app_fire, ep);
#endif
			set_event_enabled(ep->rtcp_send_event, 0);
		} else {
			ep->force_rtcp = 0;
		}
//#else
	} else {
		ep->force_rtcp = 0;
	}
//#endif

	gettimeofday(&ep->last_rtcp_recv, NULL);
	ep->trans_type = 0;
	ep->keyframe_arrival = 0;
	time(&ep->ep_create_time);

	return ep;
}

void del_rtp_endpoint(struct rtp_endpoint *ep)
{
	//#ifdef RTCP_ENABLE
	if (rtcp_enable) {
		//kevin 2007.1.12
		if (ep->force_rtcp) {
			remove_event(ep->rtcp_send_event);
		}
	}
	//#endif
	switch (ep->trans_type) {
#if (RTP_OVER_UDP)
	case RTP_TRANS_UDP:
		//kevin 2007.1.17

		remove_event(ep->trans.udp.rtp_event);
		close(ep->trans.udp.rtp_fd);

		//#ifdef RTCP_ENABLE
		if (rtcp_enable) {
			//kevin 2007.1.12
			if (ep->force_rtcp) {
				remove_event(ep->trans.udp.rtcp_event);
				close(ep->trans.udp.rtcp_fd);
			}
		}
		//#endif
		interleave_disconnect(ep->trans.udp.conn, ep->trans.udp.rtp_chan);
		break;
#endif
	case RTP_TRANS_INTER:

		interleave_disconnect(ep->trans.inter.conn,
							  ep->trans.inter.rtp_chan);

		//#ifdef RTCP_ENABLE
		/* In rtsp_interleave_setup(), the port infor is kept to make receiving work.
		   To symm to the setting, we call interleave_disconnect() to clear the port info 20160526*/
		//if(rtcp_enable)
		{
			//kevin 2007.1.12
			//if(ep->force_rtcp)
			interleave_disconnect(ep->trans.inter.conn,
								  ep->trans.inter.rtcp_chan);
		}
		//#endif
		break;
	}

	free(ep);
}


#if (RTP_OVER_UDP)
static void udp_rtp_read(struct event_info *ei, void *d)
{
	struct rtp_endpoint *ep = (struct rtp_endpoint *)d;
	unsigned char buf[16384];
	int ret;

	ret = read(ep->trans.udp.rtp_fd, buf, sizeof(buf));
	if (ret > 0) {
		/* some SIP phones don't send RTCP */
		gettimeofday(&ep->last_rtcp_recv, NULL);
		return;
	} else if (ret < 0)
		spook_log(SL_VERBOSE, "error on UDP RTP socket: %s",
				  strerror(errno));
	else {
		spook_log(SL_VERBOSE, "UDP RTP socket closed");
	}
	//kevin very important, avoid server down

	ep->session->teardown(ep->session, ep, 1);   // Carl

}

static void udp_rtcp_read(struct event_info *ei, void *d)
{
	struct rtp_endpoint *ep = (struct rtp_endpoint *)d;
	unsigned char buf[16384];
	int ret;

	ret = read(ep->trans.udp.rtcp_fd, buf, sizeof(buf));
	if (ret > 0) {
		spook_log(SL_DEBUG, "received RTCP packet from client");
		gettimeofday(&ep->last_rtcp_recv, NULL);
		return;
	} else if (ret < 0)
		spook_log(SL_VERBOSE, "error on UDP RTCP socket: %s",
				  strerror(errno));
	else {
		spook_log(SL_VERBOSE, "UDP RTCP socket closed");
	}
	//kevin very important, avoid server down

	ep->session->teardown(ep->session, ep, 1);   // Carl

}
#endif


void interleave_recv_rtcp(struct rtp_endpoint *ep, unsigned char *d, int len)
{
	spook_log(SL_DEBUG, "received RTCP packet from client");
	gettimeofday(&ep->last_rtcp_recv, NULL);
}

static int rtcp_app_send(struct rtp_endpoint *ep, unsigned int timestamp)
{
	struct timeval now;
	unsigned char buf[64];
	struct iovec v[1];
	//struct timespec ts;
	struct timeval tv;
	//uint64_t ntp64;
	unsigned int ntp_sec, ntp_usec;
#ifdef RTCP_TIMESTAMP_ADJUST
	uint32_t rtcp_timestamp;
#endif
	gmss_sr *sr = ep->sr;
	struct live_source *source;
	unsigned int clock_rate_hz;

	gettimeofday(&now, NULL);
	if (ep->rtcp_tv_base.tv_sec == 0 && ep->rtcp_tv_base.tv_usec == 0) {
		gettimeofday(&ep->rtcp_tv_base, NULL);
	}
	//kevin 2007.1.17
	ep->enable_send_rtcp = 0;
	//kevin
	buf[0] = '$';
	buf[1] = ep->trans.inter.rtcp_chan;
#if (RTCP_USER)
	PUT_16(buf + 2, 16);
	buf[4] = 2 << 6; // version
	buf[5] = 204; // packet type is Application-defined Report
	PUT_16(buf + 6, 3);   // length in words minus one
	PUT_32(buf + 8, ep->ssrc);
	memcpy(buf + 12, "Joe", 4);
	PUT_32(buf + 16, ep->last_timestamp);
#else
	gettimeofday(&tv, NULL);
	ntp_sec = now.tv_sec + 0x83AA7E80;
	ntp_usec = (double)((double)now.tv_usec * (double)0x4000000) / 15625.0;
#if 0
	ts.tv_sec = tv.tv_sec;
	ts.tv_nsec = tv.tv_usec * 1000;
	ntp64 = (uint64_t)(ts.tv_nsec) << 32;
	ntp64 /= 1000000000;
	ntp64 |= ((70LL * 365 + 17) * 24 * 60 * 60 + ts.tv_sec) << 32;
#endif

	// Sender Reoprt
	PUT_16(buf + 2, 28);
	buf[4] = 2 << 6; // version
	buf[5] = 200; // packet type is Application-defined Report
	PUT_16(buf + 6, 6);   // length in words minus one
	PUT_32(buf + 8, ep->ssrc);
	PUT_32(buf + 12, ntp_sec);   // NTP timestamp MSW
	PUT_32(buf + 16, ntp_usec);   // NTP timestamp LSW
#ifdef RTCP_TIMESTAMP_ADJUST
	if (ep->payload == 96) {
		rtcp_timestamp = tv.tv_sec * 90000 + tv.tv_usec * 9 / 100;
	} else {
		rtcp_timestamp = tv.tv_sec * 8000 + tv.tv_usec * 8 / 1000;
	}

	rtcp_timestamp = rtcp_timestamp_adjust(ep, rtcp_timestamp);

	PUT_32(buf + 20, rtcp_timestamp) ;
#else
	if (sr && sr->range_npt_msec) {
		// Has range_npt, may be a file playback
		// RTCP timestamp will not follow rtp packet timestamp
		source = (struct live_source *)(sr->live_source);
		if (ep->payload == 96) {
			if (source->track[0].rtp && source->track[0].rtp->clock_rate_hz) {
				clock_rate_hz = source->track[0].rtp->clock_rate_hz;
			} else {
				clock_rate_hz = 90000;
			}
		} else {
			if (source->track[1].rtp && source->track[1].rtp->clock_rate_hz) {
				clock_rate_hz = source->track[1].rtp->clock_rate_hz;
			} else {
				clock_rate_hz = 8000;
			}
		}

		PUT_32(buf + 20, (tv.tv_sec - ep->rtcp_tv_base.tv_sec)*clock_rate_hz + (tv.tv_usec - ep->rtcp_tv_base.tv_usec) / 1000 * (clock_rate_hz / 1000));
	} else {
		// Has no range_npt, may be a live stream
		// RTCP timestamp will follow rtp packet timestamp
		PUT_32(buf + 20, ep->last_timestamp) ;  //timestamp ); // RTP timestamp
	}
#endif
	PUT_32(buf + 24, ep->packet_count);   //0);// sender's packet count
	PUT_32(buf + 28, ep->octet_count);   //0);// sender's octet count
#endif

	switch (ep->trans_type) {
#if (RTP_OVER_UDP)
	case RTP_TRANS_UDP:
		//kevin 2007.1.17
#if (RTCP_USER)
		if (send(ep->trans.udp.rtcp_fd, &buf[4], 16, 0) < 0)
#else
		if (send(ep->trans.udp.rtcp_fd, &buf[4], 28, 0) < 0)
#endif
			spook_log(SL_VERBOSE, "error sending UDP RTCP frame: %s",
					  strerror(errno));
		else {
			return 0;
		}
		break;
#endif
	case RTP_TRANS_INTER:
		v[0].iov_base = buf;
#if (RTCP_USER)
		v[0].iov_len = 20;
#else
		v[0].iov_len = 32; //yhkang 20140305
#endif

		if (interleave_send(ep->trans.inter.conn,
							ep->trans.inter.rtcp_chan, v, 1) < 0) {
			spook_log(SL_VERBOSE, "error sending interleaved RTCP frame");
		}

		else {
			return 0;
		}
		break;
	}

	printf("   rtcp_app_send, teardown\n");

	ep->session->teardown(ep->session, ep, 1);  // Carl
	return -1;
}

int rtcp_goodbye_send(struct rtp_endpoint *ep, unsigned int timestamp)
{
	struct timeval now;
	unsigned char buf[64];
	struct iovec v[1];
	//struct timespec ts;
	//struct timeval tv;
	uint64_t ntp64 = 0;
	//uint32_t rtcp_timestamp;

	gettimeofday(&now, NULL);
	//kevin 2007.1.17
	ep->enable_send_rtcp = 0;
	//kevin
	buf[0] = '$';
	buf[1] = ep->trans.inter.rtcp_chan;
	PUT_16(buf + 2, 36);

	buf[4] = 2 << 6; // version
	buf[5] = 200; // packet type is Application-defined Report
	PUT_16(buf + 6, 6);   // length in words minus one
	PUT_32(buf + 8, ep->ssrc);
	PUT_32(buf + 12, (uint32_t)(ntp64 >> 32)); // NTP timestamp MSW
	PUT_32(buf + 16, (uint32_t)ntp64);   // NTP timestamp LSW
	PUT_32(buf + 20, timestamp);   // RTP timestamp
	PUT_32(buf + 24, ep->packet_count);   // sender's packet count
	PUT_32(buf + 28, ep->octet_count);   // sender's octet count*/

	buf[32] = (2 << 6) + 1; // version
	buf[33] = 203; // packet type is Application-defined Report
	PUT_16(buf + 34, 1);   // length in words minus one
	PUT_32(buf + 36, ep->ssrc);

	switch (ep->trans_type) {
#if (RTP_OVER_UDP)
	case RTP_TRANS_UDP:
		//kevin 2007.1.17
		if (send(ep->trans.udp.rtcp_fd, &buf[4], 36, 0) < 0)
			spook_log(SL_VERBOSE, "error sending UDP RTCP frame: %s",
					  strerror(errno));
		else {
			return 0;
		}
		break;
#endif
	case RTP_TRANS_INTER:
		v[0].iov_base = buf;
		v[0].iov_len = 40;

		if (interleave_send(ep->trans.inter.conn,
							ep->trans.inter.rtcp_chan, v, 1) < 0) {
			spook_log(SL_VERBOSE, "error sending interleaved RTCP frame");
		} else {
			return 0;
		}
		break;
	}
	ep->session->teardown(ep->session, ep, 1);   // Carl
	return -1;
}


#if (RTP_OVER_UDP)
int connect_udp_endpoint(struct rtp_endpoint *ep,
						 struct in_addr dest_ip, int dest_port, int *our_port)
{
	struct sockaddr_in rtpaddr, rtcpaddr;
	int port, success = 0, i, max_tries, rtpfd = -1, rtcpfd = -1;
	int ds_field;

	rtpaddr.sin_family = rtcpaddr.sin_family = AF_INET;
	rtpaddr.sin_addr.s_addr = rtcpaddr.sin_addr.s_addr = 0;
	port = rtp_port_start + random() % (rtp_port_end - rtp_port_start);

	if (port & 0x1) {
		++port;
	}
	max_tries = (rtp_port_end - rtp_port_start + 1) / 2;

	for (i = 0; i < max_tries; ++i) {
		if (port + 1 > rtp_port_end) {
			port = rtp_port_start;
		}
		rtpaddr.sin_port = htons(port);

		//#ifdef RTCP_ENABLE
		if (rtcp_enable) {
			rtcpaddr.sin_port = htons(port + 1);
		}
		//#endif
		if (rtpfd < 0 &&
			(rtpfd = socket(PF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0)) < 0) {
			spook_log(SL_WARN, "unable to create UDP RTP socket: %s",
					  strerror(errno));
			return -1;
		}

		//#ifdef RTCP_ENABLE
		if (rtcp_enable) {
			//kevin 2007.1.12
			if (ep->force_rtcp)
				if (rtcpfd < 0 &&
					(rtcpfd = socket(PF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0)) < 0) {
					spook_log(SL_WARN, "unable to create UDP RTCP socket: %s",
							  strerror(errno));
					close(rtpfd);
					return -1;
				}
		}
		//#endif
		if (bind(rtpfd, (struct sockaddr *)&rtpaddr,
				 sizeof(rtpaddr)) < 0) {
			if (errno == EADDRINUSE) {
				port += 2;
				continue;
			}
			spook_log(SL_WARN, "strange error when binding RTP socket: %s",
					  strerror(errno));
			close(rtpfd);
			close(rtcpfd);
			return -1;
		}

		//#ifdef RTCP_ENABLE
		if (rtcp_enable) {
			//kevin 2007.1.12
			if (ep->force_rtcp)
				if (bind(rtcpfd, (struct sockaddr *)&rtcpaddr,
						 sizeof(rtcpaddr)) < 0) {
					if (errno == EADDRINUSE) {
						close(rtpfd);
						rtpfd = -1;
						port += 2;
						continue;
					}
					spook_log(SL_WARN, "strange error when binding RTCP socket: %s",
							  strerror(errno));
					close(rtpfd);
					close(rtcpfd);
					return -1;
				}
		}
		//#endif
		success = 1;
		break;
	}
	if (! success) {
		spook_log(SL_WARN, "ran out of UDP RTP ports!");
		return -1;
	}
	rtpaddr.sin_family = rtcpaddr.sin_family = AF_INET;

	rtpaddr.sin_addr = rtcpaddr.sin_addr = dest_ip;
	rtpaddr.sin_port = htons(dest_port);

	//#ifdef RTCP_ENABLE
	if (rtcp_enable) {
		rtcpaddr.sin_port = htons(dest_port + 1);
	}
	//#endif
	if (connect(rtpfd, (struct sockaddr *)&rtpaddr,
				sizeof(rtpaddr)) < 0) {
		spook_log(SL_WARN, "strange error when connecting RTP socket: %s",
				  strerror(errno));
		close(rtpfd);
		close(rtcpfd);
		return -1;
	}

	//#ifdef RTCP_ENABLE
	if (rtcp_enable) {
		//kevin 2007.1.12
		if (ep->force_rtcp)
			if (connect(rtcpfd, (struct sockaddr *)&rtcpaddr,
						sizeof(rtcpaddr)) < 0) {
				spook_log(SL_WARN, "strange error when connecting RTCP socket: %s",
						  strerror(errno));
				close(rtpfd);
				close(rtcpfd);
				return -1;
			}
	}
	//#endif
	i = sizeof(rtpaddr);
	if (getsockname(rtpfd, (struct sockaddr *)&rtpaddr, (socklen_t *) &i) < 0) {
		spook_log(SL_WARN, "strange error from getsockname: %s",
				  strerror(errno));
		close(rtpfd);
		close(rtcpfd);
		return -1;
	}

	ep->max_data_size = rtp_mtu; /* good guess for preventing fragmentation */
	ep->trans_type = RTP_TRANS_UDP;
	sprintf(ep->trans.udp.sdp_addr, "IP4 %s",
			inet_ntoa(rtpaddr.sin_addr));
	ep->trans.udp.sdp_port = ntohs(rtpaddr.sin_port);
	ep->trans.udp.rtp_fd = rtpfd;

	// Set DSCP
	ds_field = (rtp_dscp & 0x3F) << 2;
	setsockopt(ep->trans.udp.rtp_fd, IPPROTO_IP, IP_TOS, (void *) &ds_field, sizeof(ds_field));

	//#ifdef RTCP_ENABLE
	if (rtcp_enable) {
		//kevin 2007.1.17
		ep->trans.udp.rtcp_fd = rtcpfd;
		ep->trans.udp.rtp_event = add_fd_event(rtpfd, 0, 0, udp_rtp_read, ep);
		//kevin 2007.1.12
		if (ep->force_rtcp) {
			ep->trans.udp.rtcp_event = add_fd_event(rtcpfd, 0, 0, udp_rtcp_read, ep);
		}

		// Set DSCP
		ds_field = (rtp_dscp & 0x3F) << 2;
		setsockopt(ep->trans.udp.rtcp_fd, IPPROTO_IP, IP_TOS, (void *) &ds_field, sizeof(ds_field));
	}
	//#endif
	*our_port = port;

	return 0;
}
#endif


void connect_interleaved_endpoint(struct rtp_endpoint *ep,
								  struct conn *conn, int rtp_chan, int rtcp_chan)
{
	int ds_field;

	ep->trans_type = RTP_TRANS_INTER;
	ep->trans.inter.conn = conn;
	ep->trans.inter.rtp_chan = rtp_chan;

	// Set DSCP
	ds_field = (rtp_dscp & 0x3F) << 2;
	setsockopt(ep->trans.inter.conn->fd, IPPROTO_IP, IP_TOS, (void *) &ds_field, sizeof(ds_field));

	//#ifdef RTCP_ENABLE
	if (rtcp_enable) {
		//kevin 2007.1.12
		if (ep->force_rtcp) {
			ep->trans.inter.rtcp_chan = rtcp_chan;
		}
	}
	//#endif
}

#ifdef RTP_SPEED_LIMIT
static struct timeval tv_pre, tv_post;
static int rtp_speed_adjust_us = 0;

void rtp_speed_limit_pre_proc()
{
	struct timeval tv0;
	int delat_us;
	struct timeval tv1, tv2, tv_delay;
	int us_diff_dbg;

	gettimeofday(&tv0, NULL);

	if (TIMEVAL_DIFF_SEC(tv_post, tv0) < 2) {
		delat_us = TIMEVAL_DIFF_USEC(tv_post, tv0);
		if (rtp_speed_adjust_us > 0) {
			if (delat_us > rtp_speed_adjust_us) {
				rtp_speed_adjust_us = 0;
			} else {
				rtp_speed_adjust_us = rtp_speed_adjust_us - delat_us;
			}

			if (rtp_speed_adjust_us > 500) {
				rtp_speed_adjust_us = MIN(rtp_speed_adjust_us, 10000);
				gettimeofday(&tv1, NULL);
				tv_delay.tv_sec = 0;
				tv_delay.tv_usec = rtp_speed_adjust_us;
				select(0, NULL, NULL, NULL, &tv_delay);
				gettimeofday(&tv2, NULL);

				us_diff_dbg = TIMEVAL_DIFF_USEC(tv1, tv2);
				rtp_speed_adjust_us = rtp_speed_adjust_us - us_diff_dbg;
			}

		}
	} else {
		rtp_speed_adjust_us = 0;
	}

	gettimeofday(&tv_pre, NULL);
}

void rtp_speed_limit_post_proc(int rtp_len)
{
	int target_us;
	int real_us;

	gettimeofday(&tv_post, NULL);

	if (TIMEVAL_DIFF_SEC(tv_pre, tv_post) < 2) {
		target_us = (rtp_len * 8 / RTP_SPEED_UPPER_BOUND);
		real_us = TIMEVAL_DIFF_USEC(tv_pre, tv_post);
		rtp_speed_adjust_us += (target_us - real_us);
		rtp_speed_adjust_us = MAX(rtp_speed_adjust_us, -10000);
	} else {
		rtp_speed_adjust_us = 0;
	}
}
#endif

int send_rtp_packet(struct rtp_endpoint *ep, struct iovec *v, int count,
					unsigned int timestamp, int marker)
{

	unsigned char rtphdr[16];
	struct msghdr mh;
	int i;
	short len = 0;
	int ret;

//#ifdef RTCP_ENABLE
	if (rtcp_enable) {
		/*RTCP is only enabled for Video session*/
		//kevin 2007.1.17
		if (ep->enable_send_rtcp == 1 && ep->delay_time == 0 && ep->stopdetect == 0 && ep->rtcp_send_event != NULL) {
			if (rtcp_app_send(ep, timestamp) < 0) {
				return -1;
			}
			if (!(ep->event_enabled)) {
				if (ep->rtcp_send_event == NULL) {
					//printf("==> %s, line = %d, ep->rtcp_send_event is NULL\n", __FUNCTION__, __LINE__);
					return -1;
				}

				set_event_enabled(ep->rtcp_send_event, 1);

				ep->event_enabled = 1;
			}
		}
	}
//#endif


	//kevin no need ep->last_timestamp = timestamp;

	// for Streaming application Mode, we need send the packet to user.
	for (i = 1; i < count; ++i) {
		len += v[i].iov_len;
	}

	rtphdr[0] = '$';
	//kevin
	rtphdr[1] = ep->trans.inter.rtp_chan;

	//12 is RTP header length
	rtphdr[2] = (len + 12) >> 8;
	rtphdr[3] = (len + 12);
	rtphdr[4] = 2 << 6; /* version */

	rtphdr[5] = ep->payload & 0x7f;
	if (marker) {
		rtphdr[5] |= 0x80;
	}
	PUT_16(rtphdr + 6, ep->seqnum);
	PUT_32(rtphdr + 8, timestamp);       // Carl
	PUT_32(rtphdr + 12, ep->ssrc);

#ifdef RTP_SPEED_LIMIT
	rtp_speed_limit_pre_proc();
#endif

	switch (ep->trans_type) {
#if (RTP_OVER_UDP)
	case RTP_TRANS_UDP:
		v[0].iov_base = &rtphdr[4];
		v[0].iov_len = 12;
		memset(&mh, 0, sizeof(mh));
		mh.msg_iov = v;
		mh.msg_iovlen = count ;

		if (sendmsg(ep->trans.udp.rtp_fd, &mh, 0) < 0) {
			spook_log(SL_VERBOSE, "error sending UDP RTP frame: %s",
					  strerror(errno));
			//kevin 2006.8.26
			//ep->session->teardown( ep->session, ep );
			return -1;
		}
		break;
#endif
	case RTP_TRANS_INTER:

		v[0].iov_base = rtphdr;
		v[0].iov_len = 16;

		memset(&mh, 0, sizeof(mh));
		mh.msg_iov = v;
		mh.msg_iovlen = count;

#ifdef GM
		ret = check_network_ready(ep, len);
#endif

		if (ret > 0) {
			if ((ret = sendmsg(ep->trans.inter.conn->fd, &mh, 0)) < 0) {
				// printf("%s:%d<Close Socket !!>\n",__FUNCTION__,__LINE__);
				return -1;
			}
			ep->packetskip = 0;
		} else if (ret < 0) {
			// printf("%s:%d<Close Socket !!>\n",__FUNCTION__,__LINE__);
			return -1;
		}
	} /* end select case*/

	ep->seqnum = (ep->seqnum + 1) & 0xFFFF;
	ep->packet_count++;

#ifdef RTP_SPEED_LIMIT
	rtp_speed_limit_post_proc(len + 54);
#endif

	return 0;

}



/********************* GLOBAL CONFIGURATION DIRECTIVES ********************/

int config_rtprange(int port_start, int port_end)
{
	if (port_start & 0x1) {
		++port_start;
	}
	if (!(rtp_port_end & 0x1)) {
		--port_end;
	}

	spook_log(SL_DEBUG, "RTP port range is %d-%d",  port_start, port_end);

	if ((port_end - port_start + 1) < 8) {
		spook_log(SL_ERR, "at least 8 ports are needed for RTP");
		return -1;
	}

	rtp_port_start = port_start;
	rtp_port_end = port_end;

	return 0;
}
