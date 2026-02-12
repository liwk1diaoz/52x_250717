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

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "sysdef.h"     // Carl
#include "event.h"
#include "gm_memory.h"


/* Carl */
int gm_mutex_lock(void);
int gm_mutex_trylock(void);
int gm_mutex_unlock(void);
int exec_rtspd_cmd(void);
extern int  lif_sock;
extern int rtsp_log_conn_stat_mode;

static struct event *time_event_list = NULL;
static struct event *fd_event_list = NULL;
static struct event *always_event_list = NULL;
static int end_loop = 0;

void log_connection_status();

// Carl
static void release_event_list(struct event **ev)
{
	struct event    *te, *e = *ev;
	for (; (e); te = e, e = e->next, free(te));
	*ev = NULL;
}


// Carl
void event_cleanall(void)
{
	release_event_list(&time_event_list);
	release_event_list(&fd_event_list);
	release_event_list(&always_event_list);
}


int time_diff(time_ref *tr_start, time_ref *tr_end)
{
	return ((tr_end->tv_sec - tr_start->tv_sec) * 1000000
			+ tr_end->tv_usec - tr_start->tv_usec + 500) / 1000;
}

int time_ago(time_ref *tr)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	return time_diff(tr, &now);
}

void time_now(time_ref *tr)
{
	gettimeofday((struct timeval *)tr, NULL);
}

void time_add(time_ref *tr, int msec)
{
	tr->tv_sec += msec / 1000;
	tr->tv_usec += (msec % 1000) * 1000;
	if (tr->tv_usec >= 1000000) {
		tr->tv_usec -= 1000000;
		++tr->tv_sec;
	}
}

void time_future(time_ref *tr, int msec)
{
	gettimeofday(tr, NULL);
	time_add(tr, msec);
}


static struct event *new_event(callback f, void *d)
{
	struct event *e;

	e = (struct event *)malloc(sizeof(struct event));
	e->next = NULL;
	e->prev = NULL;
	e->type = 0;
	e->flags = 0;
	e->func = f;
	e->data = d;
	return e;
}

static void strip_events(struct event **list)
{
	struct event *e, *n;

	for (e = *list; e; e = n) {
		n = e->next;
		if (e->flags & EVENT_F_REMOVE) {
			if (e->next) {
				e->next->prev = e->prev;
			}
			if (e->prev) {
				e->prev->next = e->next;
			} else {
				*list = e->next;
			}
			free(e);    // Carl
		}
	}
}

struct event *add_timer_event(int msec, unsigned int flags, callback f, void *d)
{
	struct event *e;

	e = new_event(f, d);
	e->type = EVENT_TIME;
	e->flags = flags;
	e->ev.time.ival = msec;
	e->next = time_event_list;
	if (e->next) {
		e->next->prev = e;
	}
	time_event_list = e;
	time_now(&e->ev.time.fire);
	resched_event(e, NULL);
	return e;
}

struct event *add_alarm_event(time_ref *t, unsigned int flags, callback f, void *d)
{
	struct event *e;

	e = new_event(f, d);
	e->type = EVENT_TIME;
	e->flags = flags | EVENT_F_ONESHOT;
	e->next = time_event_list;
	if (e->next) {
		e->next->prev = e;
	}
	time_event_list = e;
	resched_event(e, t);
	return e;
}

void resched_event(struct event *e, time_ref *tr)
{
	if (tr) {
		e->ev.time.fire = *tr;
	} else if (e->flags & EVENT_F_ENABLED) {
		time_add(&e->ev.time.fire, e->ev.time.ival);
	} else {
		time_future(&e->ev.time.fire, e->ev.time.ival);
	}

	e->flags &= ~EVENT_F_REMOVE;
	e->flags |= EVENT_F_ENABLED;
}

struct event *add_fd_event(int fd, int write, unsigned int flags, callback f, void *d)
{
	struct event *e;

	e = new_event(f, d);
	e->type = EVENT_FD;
	//set it as enable default
	e->flags = flags | EVENT_F_ENABLED;
	e->ev.fd.fd = fd;
	e->ev.fd.write = write;
	e->next = fd_event_list;
	if (e->next) {
		e->next->prev = e;
	}
	fd_event_list = e;
	return e;
}

struct event *add_always_event(unsigned int flags, callback f, void *d)
{
	struct event *e;

	e = new_event(f, d);
	e->type = EVENT_ALWAYS;
	e->flags = flags | EVENT_F_ENABLED;
	e->next = always_event_list;
	if (e->next) {
		e->next->prev = e;
	}
	always_event_list = e;
	return e;
}


void remove_event(struct event *e)
{
	if (e) {
		e->flags |= EVENT_F_REMOVE;
		e->flags &= ~(EVENT_F_RUNNING | EVENT_F_ENABLED);
	}
}

void set_event_interval(struct event *e, int msec)
{
	e->ev.time.ival = msec;
	if (e->flags & EVENT_F_ENABLED) {
		resched_event(e, NULL);
	}
}

void set_event_enabled(struct event *e, int enabled)
{
	if (e == NULL) {
		//printf("==> %s e is NULL\n", __FUNCTION__);

		return;
	}

	e->flags &= ~EVENT_F_ENABLED;
	if (enabled) {
		e->flags |= EVENT_F_ENABLED;
	}
}

int get_event_enabled(struct event *e)
{
	return e->flags & EVENT_F_ENABLED ? 1 : 0;
}

void exit_rtsp_event_loop(void)
{
	end_loop = 1;
}

void regular_update_log_connection()
{
	static struct timespec tt1;
	struct timespec tt2;

	if (rtsp_log_conn_stat_mode > 0) {
		clock_gettime(CLOCK_MONOTONIC, &tt2);
		if (tt2.tv_sec - tt1.tv_sec >= rtsp_log_conn_stat_mode) {
			log_connection_status();
			clock_gettime(CLOCK_MONOTONIC, &tt1);
		}
	}
}

void rtsp_event_loop(int single)
{
	struct timeval t, *st;
	struct event *e;
	struct event_info ei;
	int diff, nexttime = 0, highfd, ret;
	fd_set rfds, wfds;

	end_loop = 0;

	do {
		st = NULL;

		gm_mutex_lock();    // Carl

		/* check how long the timeout should be */
		for (e = time_event_list; e; e = e->next)
			if (e->flags & EVENT_F_ENABLED) {
				diff = -time_ago(&e->ev.time.fire);
				if (diff < 5) {
					diff = 0;
				}
				if (! st || diff < nexttime) {
					nexttime = diff;
				}
				st = &t;
				e->flags |= EVENT_F_RUNNING;
			} else {
				e->flags &= ~EVENT_F_RUNNING;
			}
		for (e = always_event_list; e; e = e->next)
			if (e->flags & EVENT_F_ENABLED) {
				st = &t;
				nexttime = 0;
				e->flags |= EVENT_F_RUNNING;
			} else {
				e->flags &= ~EVENT_F_RUNNING;
			}

		if (st) {
			if (nexttime < ENQ_CHECK_TIME) {
				nexttime = ENQ_CHECK_TIME;    // Carl, 10ms.
			}
			st->tv_sec = nexttime / 1000;
			st->tv_usec = (nexttime % 1000) * 1000;
		} else { // Carl
			st = &t;
			t.tv_sec = ENQ_CHECK_TIME / 1000;
			t.tv_usec = (ENQ_CHECK_TIME % 1000) * 1000;
		}

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		highfd = -1;
		/* This is all so ugly...  It should use poll() eventually. */
		for (e = fd_event_list; e; e = e->next) {
			if (e->flags & EVENT_F_ENABLED) {
				FD_SET(e->ev.fd.fd,
					   e->ev.fd.write ? &wfds : &rfds);
				if (e->ev.fd.fd > highfd) {
					highfd = e->ev.fd.fd;
				}
				e->flags |= EVENT_F_RUNNING;
			} else {
				e->flags &= ~EVENT_F_RUNNING;
			}
		}

		gm_mutex_unlock();  // Carl

		ret = select(highfd + 1, &rfds, &wfds, NULL, st);

		gm_mutex_lock();    // Carl
		// pthread_testcancel();    // Carl
		// if (gm_mutex_trylock()) goto loop_next;      /* Carl */

		exec_rtspd_cmd();   // Carl

		for (e = time_event_list; e; e = e->next) {
			if (!(e->flags & EVENT_F_RUNNING)) {
				continue;
			}
			if (end_loop) {
				break;
			}
			diff = -time_ago(&e->ev.time.fire);
			if (diff < 5) {
				if (!(e->flags & EVENT_F_ONESHOT)) {
					resched_event(e, NULL);
				} else {
					e->flags |= EVENT_F_REMOVE;
				}
				ei.e = e;
				ei.type = EVENT_TIME;
				ei.data = NULL;
				(*e->func)(&ei, e->data);
			}
		}

		for (e = always_event_list; e; e = e->next) {
			if (!(e->flags & EVENT_F_RUNNING)) {
				continue;
			}
			if (end_loop) {
				break;
			}
			if (e->flags & EVENT_F_ONESHOT) {
				e->flags |= EVENT_F_REMOVE;
			}
			ei.e = e;
			ei.type = EVENT_ALWAYS;
			ei.data = NULL;
			(*e->func)(&ei, e->data);
		}

		if (ret > 0) for (e = fd_event_list; e; e = e->next) {
				if (!(e->flags & EVENT_F_RUNNING)) {
					continue;
				}
				if (end_loop) {
					break;
				}
				// Carl
				if (e->ev.fd.fd == lif_sock) {
					(*e->func)(&ei, e->data);
					continue;
				}

				if (FD_ISSET(e->ev.fd.fd,
							 e->ev.fd.write ? &wfds : &rfds)) {
					if (e->flags & EVENT_F_ONESHOT) {
						e->flags |= EVENT_F_REMOVE;
					}
					ei.e = e;
					ei.type = EVENT_FD;
					ei.data = NULL;
					(*e->func)(&ei, e->data);
				}
			}

		regular_update_log_connection();

		gm_mutex_unlock();      /* Carl */

// loop_next:
		strip_events(&time_event_list);
		strip_events(&fd_event_list);
		strip_events(&always_event_list);
	} while (! end_loop && ! single);
}

