/*
 * Copyright (C) 2004 Nathan Lutchansky <lutchann@litech.org>
 * Copyright (C) 2006 Hsin-Hua Lee <daisy.lee@gmail.com>
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


struct rtp_g726 {
	unsigned char *g726_data;
	int g726_len;
	int ts_incr;
	unsigned int timestamp;
	int format;
	int sampsize;

	int channels;
	int rate;

};


//static FILE *audiofile1= NULL;

void avsync(struct rtp_endpoint *ep, unsigned int curr_time);


static int g726_get_sdp(char *dest, int len, int payload, int port, void *d)
{
	struct rtp_g726 *out = (struct rtp_g726 *)d;

	//return snprintf( dest, len, "m=audio %d RTP/AVP %d\r\na=rtpmap:%d PCMA/16000/1\r\n", port, payload, payload);
	printf("m=audio %d RTP/AVP %d\r\na=rtpmap:%d PCMA/%d/%d\r\n", port, payload, payload, out->rate, out->channels);
	/* PayLoad type can reference to rfc3551 Table 4 */
	return snprintf(dest, len, "m=audio %d RTP/AVP %d\r\na=rtpmap:%d PCMA/%d/%d\r\n", port, payload, payload, out->rate, out->channels);

	//return snprintf( dest, len, "m=audio 0 RTP/AVP 97\r\na=rtpmap:97 ADPCM/16000/1\r\n");
}



static int g726_get_payload(int payload, void *d)
{
	return 97;
}

static int g726_process_frame(struct frame *f, void *d)
{
	struct rtp_g726 *out = (struct rtp_g726 *)d;
	//int timediff;
	if (f) {
		out->g726_data = f->d;
		out->g726_len = f->length;
		out->timestamp = ((gm_ss_entity *)f->ent)->timestamp;
		//out->timestamp += (f->length/out->channels);
	} else {
		//out->ts_incr = 0;
		out->g726_len = 0;
	}
	return 1; /* always ready! */
}
static int g726_send(struct rtp_endpoint *ep, void *d, unsigned int curr_time, unsigned int *delay)
{
	struct rtp_g726 *out = (struct rtp_g726 *)d;
	int i, plen, mark;
	struct iovec v[2];
	//unsigned int timediff;

	//g726_timesync( ep, curr_time, 0);

	//timediff = ep->sync_timestamp - ep->start_timestamp;

	avsync(ep, curr_time);

#if 0
	//printf("[Debug] curr_time=[%d], timediff=[%d], last_timestamp=[%d]\n", curr_time, timediff, ep->last_timestamp);
	if (timediff > 16 * 3000) {
		if (timediff > ep->last_timestamp + 16 * 200) {

			for (i = 0; i < out->g726_len; i += plen) {
				plen = out->g726_len - i;
				if (plen > ep->max_data_size) {
					plen = ep->max_data_size;
					plen -= plen % 1 ;
				}
				v[1].iov_base = out->g726_data + i;
				v[1].iov_len = plen;

				if (send_rtp_packet(ep, v, 2, ep->last_timestamp + i, 0) < 0) {
					return -1;
				}
			}

			ep->last_timestamp += out->ts_incr;
		} else if (timediff < ep->last_timestamp - 16 * 200) {

			return 0;
		}
	}

	//printf("[Debug2] curr_time=[%d], timediff=[%d], last_timestamp=[%d]\n", curr_time, timediff, ep->last_timestamp);
#endif
	for (i = 0; i < out->g726_len; i += plen) {
		plen = out->g726_len - i;
		if (plen > ep->max_data_size) {
			plen = ep->max_data_size;
			plen -= plen % 1 ;
			mark = 0;
		} else {
			mark = 1;
		}
		v[1].iov_base = out->g726_data + i;
		v[1].iov_len = plen;

		//if( send_rtp_packet( ep, v, 2, ep->last_timestamp + i,0 ) < 0 )
		if (send_rtp_packet(ep, v, 2, (out->timestamp + i), mark) < 0) {
			return -1;
		}
	}
	//ep->last_timestamp += out->ts_incr;

	return 0;
}

static int g726_release(void *p)
{
	if (p == NULL) {
		return -1;
	}
	free(p);
	return 0;
}

struct rtp_media *new_rtp_media_g726(struct stream *stream)
{
	struct rtp_g726 *out;
	int framesize;

	out = (struct rtp_g726 *)malloc(sizeof(struct rtp_g726));

	out->g726_len = 0;
	framesize = 1024;
	out->channels = 1;
	out->rate = 8000;
	//stream->get_framesize( stream, &framesize );
	//stream->get_framerate( stream, &out->channels, &out->rate );

	out->rate /= out->channels;

	out->ts_incr = framesize / out->channels;
	//out->ts_incr = 1024;
	out->timestamp = 0;

	//printf("out->ts_incr=[%d], channel=[%d], Hz=[%d], framesize=[%d]\n", out->ts_incr, out->channels, out->rate, framesize);

	return new_rtp_media(g726_get_sdp, g726_get_payload, g726_process_frame,
						 g726_send, g726_release, out, out->rate);

}

