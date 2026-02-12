/*
 * Copyright (C) 2004 Nathan Lutchansky <lutchann@litech.org>
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

#include "sysdef.h"

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
//#include <outputs.h>
#include <rtp.h>
#include <rtp_media.h>

//#include <conf_parse.h>
#include "gm_memory.h"

unsigned int ticknhz(struct rtp_endpoint *ep, unsigned int nhz);
void avsync(struct rtp_endpoint *ep, unsigned int curr_time);


static int rawaudio_get_sdp(char *dest, int len, int payload,
							int port, void *d)
{
	struct rtp_rawaudio *out = (struct rtp_rawaudio *)d;

	switch (out->format) {
	case FORMAT_PCM:
		if (out->rate == 44100 && out->channels == 2)
			return snprintf(dest, len,
							"m=audio %d RTP/AVP 10\r\n", port);
		else if (out->rate == 44100 && out->channels == 1)
			return snprintf(dest, len,
							"m=audio %d RTP/AVP 11\r\n", port);
		else {
			return snprintf(dest, len, "m=audio %d RTP/AVP %d\r\na=rtpmap:%d L16/%d/%d\r\n", port, payload, payload, out->rate, out->channels);
		}


		break;
	case FORMAT_ALAW:
		if (out->rate == 8000 && out->channels == 1)
			return snprintf(dest, len,
							"m=audio %d RTP/AVP 8\r\n", port);
		else {
			return snprintf(dest, len, "m=audio %d RTP/AVP %d\r\na=rtpmap:%d PCMA/%d/%d\r\n", port, payload, payload, out->rate, out->channels);
		}
		break;
	}
	return -1;
}

static int rawaudio_get_payload(int payload, void *d)
{


	struct rtp_rawaudio *out = (struct rtp_rawaudio *)d;

	switch (out->format) {
	case FORMAT_PCM:
		if (out->rate == 16000 && out->channels == 2) {
			return 97;    //10;
		} else if (out->rate == 16000 && out->channels == 1) {
			return 97 ;    //11;
		} else {
			return payload;
		}
		break;
	}
	return -1;


}

static int rawaudio_process_frame(struct frame *f, void *d)
{
	struct rtp_rawaudio *out = (struct rtp_rawaudio *)d;


	out->timestamp += out->rawaudio_len / out->channels / out->sampsize;


	out->rawaudio_data = f->d;
	out->rawaudio_len = f->length;


	return 1; /* always ready! */
}

#if 0
static unsigned int ticknhz_raw(struct rtp_endpoint *ep, unsigned int nhz)
{
	if (ep == NULL) {
		return 0;
	}
	ep->remainder += ep->last_timestamp - ep->nhz_last_timestamp;
	ep->nhz_last_timestamp = ep->last_timestamp;
	ep->nhz += (ep->remainder / 16000) * nhz;
	ep->remainder %= 16000;
	return ep->nhz + (((ep->remainder / 10) * nhz) / (16000 / 10));
}
#endif

static int rawaudio_send(struct rtp_endpoint *ep, void *d, unsigned int curr_time, unsigned int *delay)
{
	struct rtp_rawaudio *out = (struct rtp_rawaudio *)d;
	int i, plen, mark;
	struct iovec v[2];

	avsync(ep, curr_time);

	for (i = 0; i < out->rawaudio_len; i += plen) {
		plen = out->rawaudio_len - i;
		if (plen > ep->max_data_size) {
			plen = ep->max_data_size;
			plen -= plen % (out->channels * out->sampsize);
			mark = 0;
		} else {
			mark = 1;
		}
		v[1].iov_base = out->rawaudio_data + i;
		v[1].iov_len = plen;

		//avsync(ep, curr_time);
		//curr_time = ticknhz_raw(ep, 16000);


		if (send_rtp_packet(ep, v, 2,
							//curr_time,
							ep->last_timestamp, //out->timestamp + i / out->channels / out->sampsize,
							mark) < 0) {
			return -1;
		}
	}
	return 0;
}

static int rawaudio_release(void *p)
{
	if (p == NULL) {
		return -1;
	}
	free(p);
	return 0;
}

struct rtp_media *new_rtp_media_rawaudio_stream(struct stream *stream)
{
	struct rtp_rawaudio *out;

	out = (struct rtp_rawaudio *)malloc(sizeof(struct rtp_rawaudio));
	out->rawaudio_len = 0;
	//stream->get_framerate( stream, &out->channels, &out->rate );
	out->channels = 1 ;
	out->rate = 8000;
	out->rate /= out->channels;
	out->format = stream->format;
	out->sampsize = out->format == FORMAT_PCM ? 2 : 1;
	out->ts_incr = 0;
	out->timestamp = 0;

	return new_rtp_media(rawaudio_get_sdp, rawaudio_get_payload,
						 rawaudio_process_frame, rawaudio_send, rawaudio_release, out, out->rate);
}
