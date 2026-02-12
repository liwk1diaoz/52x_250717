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
#include <priv.h>   /* Carl. */
#include "gm_memory.h"

#define abs(X)    (((X)>0)?(X):-(X))

struct rtp_mpa {
	unsigned char *mpa_data;
	int mpa_len;
	int ts_incr;
	int samplerate;
	int channel;
	int bitrate;
	unsigned int timestamp;
};


void avsync(struct rtp_endpoint *ep, unsigned int curr_time);   // Carl


static int mpa_get_sdp(char *dest, int len, int payload, int port, void *d)
{
	struct rtp_mpa *out = (struct rtp_mpa *)d;
	int blockalign = 144 * out->bitrate / out->samplerate;

	return snprintf(dest, len, "m=audio %d RTP/AVP 14\r\na=fmtp:14 config=0050%04x0000%04x0000%04x%04x0000\r\n", port, out->channel, out->samplerate, out->bitrate / 8, blockalign);


}

static int mpa_get_payload(int payload, void *d)
{
	return 14;
}

static int mpa_process_frame(struct frame *f, void *d)
{
	struct rtp_mpa *out = (struct rtp_mpa *)d;

	if (f) {
		out->mpa_data = f->d;
		out->mpa_len = f->length;
	}

	return 1;
}

static int mpa_send(struct rtp_endpoint *ep, void *d, unsigned int curr_time, unsigned int *delay)
{
	struct rtp_mpa *out = (struct rtp_mpa *)d;
	int i, plen;
	unsigned char mpahdr[4];
	struct iovec v[4];

	PUT_16(mpahdr, 0);
	v[1].iov_base = mpahdr;
	v[1].iov_len = 4;

	avsync(ep, curr_time);  // Carl

	for (i = 0; i < out->mpa_len; i += plen) {
		plen = out->mpa_len - i;
		if (plen > ep->max_data_size) {
			plen = ep->max_data_size;
		}
		PUT_16(mpahdr + 2, i);
		v[2].iov_base = out->mpa_data + i;
		v[2].iov_len = plen;


		if (send_rtp_packet(ep, v, 3, ep->last_timestamp,
							plen + i == out->mpa_len) < 0) {
			return -1;
		}
	}
	// ep->last_timestamp += out->ts_incr;  // Carl
	// gm_check(ep);    // Carl
	ep->stopdetect = 0;

	return 0;
}


int mpa_release(void *p)
{
	if (p == NULL) {
		return -1;
	}
	free(p);
	return 0;
}


struct rtp_media *new_rtp_media_mpa(struct stream *stream)
{
	struct rtp_media *m;    // Carl
	struct rtp_mpa *out;
	// int fincr, fbase, bitrate;

	out = (struct rtp_mpa *)malloc(sizeof(struct rtp_mpa));
	out->mpa_len = 0;
	out->timestamp = 0;

	// Carl
	m = new_rtp_media(mpa_get_sdp, mpa_get_payload, mpa_process_frame,
					  mpa_send, mpa_release, out, 90000);
	if (m == NULL) {
		free(out);
	}

	return m;
}


