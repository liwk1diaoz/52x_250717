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
#include <fcntl.h>
#include <errno.h>

#include "sysdef.h" /* Carl, 20091029. */

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <rtp.h>
#include <rtp_media.h>
#include "gm_memory.h"


unsigned int ticknhz(struct rtp_endpoint *ep, unsigned int nhz);    // Carl


struct rtp_amr {
	unsigned char *amr_data;
	int amr_len;
	int ts_incr;
	unsigned int timestamp;
};


void avsync(struct rtp_endpoint *ep, unsigned int curr_time);   // Carl


static int amr_get_sdp(char *dest, int len, int payload, int port, void *d)
{
	return snprintf(dest, len, "m=audio 0 RTP/AVP 97\r\na=rtpmap:97 AMR/8000\r\na=fmtp:97 octet-align=1\r\n");
}

static int amr_get_payload(int payload, void *d)
{
	return 97;
}

static int amr_process_frame(struct frame *f, void *d)
{
	struct rtp_amr *out = (struct rtp_amr *)d;

	if (f) {
		f->d[0] = 0xF0;
		f->d[1] &= 0x7F;

		out->amr_data = f->d;
		out->amr_len = f->length;
	} else {

		out->amr_len = 0;
	}

	return 1;
}


static int amr_send(struct rtp_endpoint *ep, void *d, unsigned int curr_time, unsigned int *delay)
{
	struct rtp_amr *out = (struct rtp_amr *)d;
	struct iovec v[2];
//	unsigned int timediff;   // Carl


	v[1].iov_base = out->amr_data;
	v[1].iov_len = out->amr_len;

#if 0   // Carl
	if (ep->sync_timestamp != 0) {
		ep->sync_timestamp = 8 * curr_time ;
	} else {
		ep->sync_timestamp = ep->start_timestamp = 8 * curr_time ;
	}

	timediff = ep->sync_timestamp - ep->start_timestamp;
	if (timediff > 8 * 3000) {
		if (timediff > ep->last_timestamp + 8 * 200) {
			if (send_rtp_packet(ep, v, 2, ep->last_timestamp, 0) < 0) {
				return -1;
			}

			ep->last_timestamp += out->ts_incr;
		} else if (timediff < ep->last_timestamp - 8 * 200) {

			return 0;
		}
	}
#else
	avsync(ep, curr_time);
	curr_time = ticknhz(ep, 8000);
#endif

	if (send_rtp_packet(ep, v, 2, /*ep->last_timestamp*/ curr_time, 1) < 0) { // Carl
		return -1;
	}

	// ep->last_timestamp += out->ts_incr;  // Carl

	return 0;
}


// Carl
int amr_release(void *p)
{
	if (p == NULL) {
		return -1;
	}
	free(p);
	return 0;
}


struct rtp_media *new_rtp_media_amr(struct stream *stream)
{
	struct rtp_media *m;    // Carl
	struct rtp_amr *out;


	out = (struct rtp_amr *)malloc(sizeof(struct rtp_amr));

	out->amr_len = 0;
	out->ts_incr = 160;
	out->timestamp = 0;

	// Carl
	m = new_rtp_media(amr_get_sdp, amr_get_payload, amr_process_frame,
					  amr_send, amr_release, out, 8000);
	if (m == NULL) {
		free(out);
	}

	return m;
}


