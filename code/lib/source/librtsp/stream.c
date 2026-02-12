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
#include <fcntl.h>
#include <pthread.h>
#include <netinet/in.h>

#include "sysdef.h"     /* Carl, 20091029. */

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <pthread.h>    /* Carl, 20091109. */
#include "gm_memory.h"


void ref_frame(struct frame *f);    // Carl
void live_send_goodbye(void *d);

struct converter {
	struct stream_destination *input;
	struct stream *output;
};

static struct stream *stream_list = NULL;

void disconnect_session_client(void *d)
{
	struct stream *s = (struct stream *)d;
	struct stream_destination *dest;

	for (dest = s->dest_list; dest; dest = dest->next) {

		//pthread_mutex_lock( &deliver_mutex);
		//dest->process_frame( f, dest->d );
		live_send_goodbye(dest->d);
		//pthread_mutex_unlock( &deliver_mutex );

	}
}

void deliver_frame_to_stream(struct frame *f, void *d)
{
	struct stream *s = (struct stream *)d;
	struct stream_destination *dest;
	static pthread_mutex_t deliver_mutex;
	static int deliver_frame_init = 0;

#if 1   // Carl
	if (deliver_frame_init == 0) {
		pthread_mutex_init(&deliver_mutex, NULL);
		deliver_frame_init = 1;
	}
#endif

	for (dest = s->dest_list; dest; dest = dest->next) {

		if (! dest->waiting) {
			continue;
		}
		// dbg("dest@%p is waiting\n", dest);
		ref_frame(f);    // Carl
#if 1   // Carl
		pthread_mutex_lock(&deliver_mutex);
		dest->process_frame(f, dest->d);
		pthread_mutex_unlock(&deliver_mutex);
#else
		dest->process_frame(f, dest->d);
#endif
	}

	unref_frame(f);
}

int format_match(int format, int *formats, int format_count)
{
	int i;

	if (format_count == 0) {
		return 1;
	}
	for (i = 0; i < format_count; ++i)
		if (format == formats[i]) {
			return 1;
		}
	return 0;
}

static struct stream_destination *new_dest(struct stream *s,
		frame_deliver_func process_frame, void *d)
{
	struct stream_destination *dest;

	if ((s == NULL) || (d == NULL) || (process_frame == NULL)) {
		return NULL;    // Carl
	}

	dest = (struct stream_destination *)
		   malloc(sizeof(struct stream_destination));
	if (dest == NULL) {
		return NULL;    // Carl
	}
	dest->next = s->dest_list;
	dest->prev = NULL;
	if (dest->next) {
		dest->next->prev = dest;
	}
	s->dest_list = dest;
	dest->stream = s;
	dest->waiting = 0;
	dest->process_frame = process_frame;
	dest->d = d;

	return dest;
}


// Carl
int remove_dest(struct stream_destination *dest)
{
	struct stream   *s;

	if (dest == NULL) {
		return 0;
	}
	if (((s = dest->stream) == NULL) || (s->dest_list == NULL)) {
		return -1;
	}

	// lock here ???
	if (s->dest_list == dest) {
		s->dest_list = dest->next;
		if (s->dest_list) {
			s->dest_list->prev = NULL;
		}
	} else {
		dest->prev->next = dest->next;
		if (dest->next) {
			dest->next->prev = dest->prev;
		}
	}

	free(dest);
	return 0;
}


struct stream_destination *connect_to_stream(gmss_qu *qu,
		frame_deliver_func process_frame, void *d)
{
	struct stream *s;

	if ((qu == NULL) || (d == NULL) || (process_frame == NULL)) {
		return NULL;
	}

	for (s = stream_list; s; s = s->next) {
		if ((s->src_private == qu) && (s->format == qu->type)) {
			return new_dest(s, process_frame, d);
		}
	}

	return NULL;
}

void del_stream(struct stream *s)
{
	if (s->next) {
		s->next->prev = s->prev;
	}
	if (s->prev) {
		s->prev->next = s->next;
	} else {
		stream_list = s->next;
	}
	free(s);
}


// Carl
int remove_stream(struct stream *s)
{
	int ret = 0;


	if (s == NULL) {
		return 0;
	}
	if (s->dest_list) {
		return -1;
	}

	// lock here ???
	if (s == stream_list) {
		stream_list = s->next;
		if (stream_list) {
			stream_list->prev = NULL;
		}
	} else {
		s->prev->next = s->next;
		if (s->next) {
			s->next->prev = s->prev;
		}
	}
	if (s->src_private) {
		((gmss_qu *)(s->src_private))->stream = NULL;
	}

	free(s);

	return ret;
}


struct stream *new_stream(gmss_qu *qu, int format)  // Carl
{
	struct stream *s;

	// Carl
	if (qu == NULL) {
		return NULL;
	}
	if ((s = (struct stream *) malloc(sizeof(struct stream))) == NULL) {
		return NULL;
	}
	s->next = stream_list;
	s->prev = NULL;
	if (s->next) {
		s->next->prev = s;
	}
	stream_list = s;
	s->format = format;
	s->dest_list = NULL;
	s->get_framerate = NULL;
	s->set_running = NULL;
	s->src_private = (void *) qu;
	qu->stream = (void *) s;
	// dbg("stream@%p, type=%d, qu@%p\n", s, format, qu);

	return s;
}

void set_waiting(struct stream_destination *dest, int waiting)
{
	struct stream *s = dest->stream;

	/* We call set_running every time a destination starts listening,
	 * or when the last destination stops listening.  It is good to know
	 * when new listeners come so maybe the source can send a keyframe. */

	if (dest->waiting) {
		if (waiting) {
			return;    /* no change in status */
		}
		dest->waiting = 0;

		/* see if anybody else is listening */
		for (dest = s->dest_list; dest; dest = dest->next) {
			waiting |= dest->waiting;
		}
		if (waiting) {
			return;    /* others are still listening */
		}
	} else {
		if (! waiting) {
			return;    /* no change in status */
		}
		dest->waiting = 1;
	}

	if (s->set_running) {
		s->set_running(s, waiting);
	} else {
		// dbg("%s: try to set_running@NULL(%p, %d)\n", __func__, dest, waiting);
	}
}
