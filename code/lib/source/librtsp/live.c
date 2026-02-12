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

#include "sysdef.h"     /* Carl, 20091029. */

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>

#include <rtp.h>
#include <rtp_media.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "live.h"   // Carl
#include "gm_memory.h"

// #define spook_log(level, fmt, args...)   fprintf(stderr, fmt "\n", ## args)

// Carl
int gmss_get_sdp(gmss_sr *sr, char *dest, int len);
int gmss_get_sdp_multicast(gmss_sr *sr, char *dest, int len);
int remove_dest(struct stream_destination *dest);
int call_cmd_cb(char *name, int cmd, void *conn, void *p);
int rtcp_goodbye_send(struct rtp_endpoint *ep, unsigned int timestamp);


int live_get_sdp(gmss_sr *sr, char *dest, int *len)
{
	int i = 0, t;
	char *addr = "IP4 0.0.0.0";

	if (!sr->multicast.enabled) {
		i = snprintf(dest, *len, "v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=%s Live\r\ni=ICL Streaming Media\r\nc=IN %s\r\nt=0 0\r\n", CompanyName, addr);
		if (sr->range_npt_msec) {
			i += snprintf(dest + i, *len - i, "a=range:npt=0-%d.%03d\r\n", sr->range_npt_msec / 1000, sr->range_npt_msec % 1000);
		}
		t = gmss_get_sdp(sr, dest + i, *len - i);
	} else {
		i = snprintf(dest, *len, "v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=%s Live\r\ni=ICL Streaming Media\r\n", CompanyName);
		if (sr->range_npt_msec) {
			i += snprintf(dest + i, *len - i, "a=range:npt=0-%d.%03d\r\n", sr->range_npt_msec / 1000, sr->range_npt_msec % 1000);
		}
		t = gmss_get_sdp_multicast(sr, dest + i, *len - i);
	}

	i += t;

	*len = i;
	return t;
}


static int live_setup(struct session *s, int t, gmss_sr *sr)
{
	struct live_session *ls = (struct live_session *)s->private;
	int payload = 96 + t;

	if (! ls->source->track[t].rtp) {
		return -1;
	}

	if (ls->source->track[t].rtp->get_payload) {
		payload = ls->source->track[t].rtp->get_payload(payload,
				  ls->source->track[t].rtp->private);
		spook_log(SL_DEBUG, "live_setup payload %d", payload);
	}
	s->ep[t] = new_rtp_endpoint(payload);
	s->ep[t]->session = s;
	s->ep[t]->sr = sr;

	return 0;
}

static void live_play(struct session *s, double *start)
{
	struct live_session *ls = (struct live_session *)s->private;
	int t;

	spook_log(SL_DEBUG, "live : live_play");

#if 0   // Carl
	if (start) {
		*start = -1;
	}
#endif
	ls->playing = 1;
	ls->source->qos = 1;
	ls->source->safe_I_num = IFrameInterval;

	/* set track as active*/
	for (t = 0; t < MAX_TRACKS && ls->source->track[t].rtp; ++t)
		if (s->ep[t]) {
			set_waiting(ls->source->track[t].stream, 1);
		}
}


static void track_check_running(struct live_source *source, int t)
{
	struct live_session *ls;

	spook_log(SL_DEBUG, "live: track_check_running");
	/*check if any other client uses the track*/
	for (ls = source->sess_list; ls; ls = ls->next)
		if (ls->playing && ls->sess->ep[t]) {
			return;
		}
	/*if no, set the track as waiting state*/
	set_waiting(source->track[t].stream, 0);
}


static void live_teardown(struct session *s, struct rtp_endpoint *ep, int callback)      // Carl
{
	struct live_session *ls = (struct live_session *)s->private;
	int i, remaining = 0;

	spook_log(SL_DEBUG, "live_teardown");
	for (i = 0; i < MAX_TRACKS && ls->source->track[i].rtp; ++i) {
		if (! s->ep[i]) {
			continue;
		}
		if (! ep || s->ep[i] == ep) {
			del_rtp_endpoint(s->ep[i]);
			s->ep[i] = NULL;
			track_check_running(ls->source, i);
		} else {
			++remaining;
		}
	}

	if (remaining == 0) {
		struct live_source  *lsrc = ls->source;         // Carl
		gmss_sr *sr = (gmss_sr *)(lsrc->gmss_sr);       // Carl

		if (ls->next) {
			ls->next->prev = ls->prev;
		}
		if (ls->prev) {
			ls->prev->next = ls->next;
		} else {
			ls->source->sess_list = ls->next;
		}
		free(ls);
		del_session(s);

		// Carl
		if ((callback) && (lsrc) && (lsrc->sess_list == NULL) && (sr) && (sr->name)
#if (LIVE_TRIG) // Carl
			&& ((sr->live == 0) == (sr->conn != NULL))
#else
			&& (sr->live == 0) && (sr->conn)
#endif
		   ) {
			// dbg("stop the stream resource '%s'\n", sr->name);
			call_cmd_cb(sr->name, GM_STREAM_CMD_TEARDOWN, sr->conn, NULL);
		}
	}
}


struct session *live_open(char *path, void *d)
{
	struct live_source *source = (struct live_source *)d;
	struct live_session *ls;
	struct session      *s;

	spook_log(SL_DEBUG, "live_open %s", path);
	// Carl
	if ((s = new_session()) == NULL) {
		return NULL;
	}
	if ((ls = (struct live_session *)malloc(sizeof(struct live_session))) == NULL) {
		free(s);
		return NULL;
	}
	ls->next = source->sess_list;
	if (ls->next) {
		ls->next->prev = ls;
	}
	source->sess_list = ls;
	ls->prev = NULL;
	ls->sess = s;

	ls->source = source;
	ls->playing = 0;
	ls->delay = 0;  // Carl
	ls->sess->get_sdp = live_get_sdp;
	ls->sess->setup = live_setup;
	ls->sess->play = live_play;
	ls->sess->teardown = live_teardown;
	ls->sess->private = ls;

	return ls->sess;
}

void live_send_goodbye(void *d)
{
	struct live_track *track = (struct live_track *)d;
	struct live_session *ls, *next;

	for (ls = track->source->sess_list; ls; ls = next) {
		//int ret;
		next = ls->next;

		if (ls->playing && ls->sess->ep[track->index]) {
			rtcp_goodbye_send(ls->sess->ep[track->index], 0);
		}
	}
}

static void next_live_frame(struct frame *f, void *d)
{
	struct live_track *track = (struct live_track *)d;
	struct live_session *ls, *next;

	if (! track->rtp->frame(f, track->rtp->private)) {
		unref_frame(f);
		return;
	}

	if (! track->ready) {
		set_waiting(track->stream, 0);
		track->ready = 1;
	}
	f->format = track->stream->stream->format;
	if (track->source->qos == 1) {
		if (f->format == FORMAT_MPEG4) {
			if (((struct rtp_mpeg4 *)(track->rtp->private))->keyframe == 1) {
				track->source->curr_I_num++;
				if (track->source->safe_I_num == track->source->curr_I_num) {
					track->source->curr_I_num = 0;
				}
			}
		} else if (f->format == FORMAT_H264) {
			if (((struct rtp_h264 *)(track->rtp->private))->keyframe == 1) {
				track->source->curr_I_num++;
				if (track->source->safe_I_num == track->source->curr_I_num) {
					track->source->curr_I_num = 0;
				}
			}
		} else if (f->format == FORMAT_H265) { //sa4 : fixme
			if (((struct rtp_h265 *)(track->rtp->private))->keyframe == 1) {
				track->source->curr_I_num++;
				if (track->source->safe_I_num == track->source->curr_I_num) {
					track->source->curr_I_num = 0;
				}
			}
		}
	}

	if (track->source->I_frame_only == 1) {
		if (f->format == FORMAT_MPEG4) {
			if (((struct rtp_mpeg4 *)(track->rtp->private))->keyframe == 0) {
				unref_frame(f);
				return;
			} else {
				track->source->I_frame_only = 0;
#ifdef DEBUG_KEVIN
				printf("I-frame\n");
#endif
			}

		} else if (f->format == FORMAT_H264) {
			if (((struct rtp_h264 *)(track->rtp->private))->keyframe == 0) {
				unref_frame(f);
				return;
			} else {
				track->source->I_frame_only = 0;
#ifdef DEBUG_KEVIN
				printf("I-frame\n");
#endif
			}

		} else if (f->format == FORMAT_H265) { //sa4 : fixme
			if (((struct rtp_h265 *)(track->rtp->private))->keyframe == 0) {
				unref_frame(f);
				return;
			} else {
				track->source->I_frame_only = 0;
#ifdef DEBUG_KEVIN
				printf("I-frame\n");
#endif
			}

		}
	}

#if 0   // Carl
	gettimeofday(&tm, NULL);
	curr_time = tm.tv_sec * 1000 + tm.tv_usec / 1000;
#endif

	for (ls = track->source->sess_list; ls; ls = next) {
		int ret;
		next = ls->next;


		if (ls->playing && ls->sess->ep[track->index]) {
			ret = track->rtp->send(ls->sess->ep[track->index],
								   track->rtp->private, ((gm_ss_entity *)(f->ent))->timestamp, (unsigned int *) & (ls->delay));

			if (ret == -1) {
				ls->sess->teardown(ls->sess, NULL, 1);   // Carl
			} else if (ret > 0) {

				ls->delay = ret;

			}
		}
	}

	unref_frame(f);
}

/************************ CONFIGURATION DIRECTIVES ************************/

void *live_start_block(void *sr)
{
	struct live_source *source;
	int i;

#if 0   /* Carl */
	static int live_loop_thread_created = 0;
#endif

	spook_log(SL_DEBUG, "live: start_block");
	source = (struct live_source *)malloc(sizeof(struct live_source));
	if (source == NULL) {
		return NULL;
	}

	// Carl
	source->qos = 0;
	source->safe_I_num = 0;
	source->curr_I_num = 0;
	source->I_frame_only = 0;
	source->sess_list = NULL;
	source->gmss_sr = sr;   // Carl
	for (i = 0; i < MAX_TRACKS; ++i) {
		source->track[i].index = i;
		source->track[i].source = source;
		source->track[i].stream = NULL;
		source->track[i].ready = 1;     // Carl
		source->track[i].rtp = NULL;
	}


#if 0   /* Carl */
	/* GM Stan.Lin 2007/10/17, Implement Motion dection notification{*/
	if (live_loop_thread_created == 0) {
		live_loop_thread_created = 1;
		pthread_create(&(source->live_thread), NULL, live_loop, source);
		//printf("create live_loop_thread_created \n ");
	}
	/* }GM Stan.Lin 2007/10/17.*/
#endif

	return (void *)source;
}


int live_set_track(/*int num_tokens, struct token *tokens,*/gmss_sr *sr, gmss_qu *qu)
{
	struct live_source *source;
	int t;  /*, formats[] = { GM_SS_TYPE_H264,
                         GM_SS_TYPE_MP4,
                         GM_SS_TYPE_MJPEG,
                         GM_SS_TYPE_MP2,
                         GM_SS_TYPE_AMR};*/

	if ((sr == NULL) || (sr->live_source == NULL) || (qu == NULL)) {
		return -1;
	}
	source = (struct live_source *)(sr->live_source);

	spook_log(SL_DEBUG, "live: set_track %s(%d)", sr->name, qu->type);
	for (t = 0; t < MAX_TRACKS && source->track[t].rtp; ++t);

	if (t == MAX_TRACKS) {
		spook_log(SL_ERR, "live: exceeded maximum number of tracks");
		return -1;
	}

	if (!(source->track[t].stream = connect_to_stream(qu,
									next_live_frame,  &source->track[t]/*, formats, 5*/))) {        /* Carl */
		spook_log(SL_ERR, "live2: unable to connect to stream \"%s(%d)\"", sr->name, qu->type);
		return -1;
	}

	// switch (source->track[t].stream->stream->format )
	switch (qu->type) {
	case GM_SS_TYPE_H264:
		source->track[t].rtp = new_rtp_media_h264(source->track[t].stream->stream);
		break;
	case GM_SS_TYPE_H265:
		source->track[t].rtp = new_rtp_media_h265(source->track[t].stream->stream);
		break;
	case GM_SS_TYPE_MP4:
		source->track[t].rtp = new_rtp_media_mpeg4();
		break;
	case GM_SS_TYPE_MJPEG:
		source->track[t].rtp = new_rtp_media_jpeg_stream(source->track[t].stream->stream);
		break;
	case GM_SS_TYPE_MP2:
		source->track[t].rtp = new_rtp_media_mpa(source->track[t].stream->stream);
		break;
	case GM_SS_TYPE_AMR:
		source->track[t].rtp = new_rtp_media_amr(source->track[t].stream->stream);
		break;
	case GM_SS_TYPE_PCM:
		source->track[t].rtp = new_rtp_media_rawaudio_stream(source->track[t].stream->stream);
		break;
	case GM_SS_TYPE_G711A:
		source->track[t].rtp = new_rtp_media_g711a(source->track[t].stream->stream);
		break;
	case GM_SS_TYPE_G711U:
		source->track[t].rtp = new_rtp_media_g711u(source->track[t].stream->stream);
		break;
	case GM_SS_TYPE_G726:
		source->track[t].rtp = new_rtp_media_g726(source->track[t].stream->stream);
		break;
	case GM_SS_TYPE_AAC:
		source->track[t].rtp = new_rtp_media_aac(source->track[t].stream->stream);
		break;
#if 0   /* Carl */
	case FORMAT_MPV:
		source->track[t].rtp = new_rtp_media_mpv();
		break;
	case FORMAT_PCM:
	case FORMAT_ALAW:
		source->track[t].rtp = new_rtp_media_rawaudio_stream(
								   source->track[t].stream->stream);
		break;
	case FORMAT_ADPCM:
		source->track[t].rtp = new_rtp_media_adpcm(source->track[t].stream->stream);
		break;
	case FORMAT_G711:
		source->track[t].rtp = new_rtp_media_g711(source->track[t].stream->stream);
	case FORMAT_G723_1:
		source->track[t].rtp = new_rtp_media_g723_1(source->track[t].stream->stream);
		break;
#endif
	}

	if (! source->track[t].rtp) {
		spook_log(SL_DEBUG, "live: set_track rtp null %s(%d)", sr->name, qu->type);
		// Carl
		remove_dest(source->track[t].stream);
		source->track[t].stream = NULL;
		return -1;
	}

	set_waiting(source->track[t].stream, 0);     // Carl

	return 0;
}

