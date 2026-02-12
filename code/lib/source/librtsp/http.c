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
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sysdef.h" /* Carl, 20091029. */

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <pmsg.h>

#include <rtp.h>
#include "gm_memory.h"

void write_access_log(char *path, struct sockaddr *addr, int code, char *req,
					  int length, char *referer, char *user_agent);

struct http_stream_session;

struct http_location {
	struct loc_node node;
	struct stream_destination *input;
	struct frame *frame;
	struct timeval frame_time;
	int length_with_jfif;
	struct http_stream_session *sess_list;
	int streaming;
	int max_cache_time;
};

struct http_stream_session {
	struct http_stream_session *next;
	struct http_stream_session *prev;
	struct http_location *loc;
	struct pmsg *resp;
	struct conn *conn;
};

static struct http_location *http_loc_list = NULL;


static struct loc_node *node_find_location(struct loc_node *list,
		char *path, int len)
{
	struct loc_node *loc;

	for (loc = list; loc; loc = loc->next)
		if (! strncmp(path, loc->path, strlen(loc->path))
			&& (loc->path[len] == '/'
				|| loc->path[len] == 0)) {
			return loc;
		}
	return NULL;
}


static struct http_location *find_http_location(char *path)
{
	int len;

	len = strlen(path);
	if (path[len - 1] == '/') {
		--len;
	}

	return (struct http_location *)node_find_location(
			   (struct loc_node *)http_loc_list, path, len);
}


static void log_request(struct req *req, int code, int length)
{
	char *ref, *ua;

	ref = get_header(req->req, "referer");
	ua = get_header(req->req, "user-agent");
#if 0   // Carl
	write_access_log(NULL, (struct sockaddr *)&req->conn->client_addr,
					 code, (char *)(req->conn->req_buf), length,
					 ref ? ref : "-", ua ? ua : "-");
#endif
}


static void http_create_reply(struct req *req, int code, char *reply)
{
	req->resp = new_pmsg(1024);     // Carl
	req->resp->type = PMSG_RESP;
	req->resp->proto_id = add_pmsg_string(req->resp, "HTTP/1.0");
	req->resp->sl.stat.code = code;
	req->resp->sl.stat.reason = add_pmsg_string(req->resp, reply);
}


static void http_send_error(struct req *req, int code, char *reply, char *t)
{
	log_request(req, code, 0);
	http_create_reply(req, code, reply);
	add_header(req->resp, "Connection", "close");
	add_header(req->resp, "Content-Type", "text/html");
	req->conn->drop_after = 1;
	if (tcp_send_pmsg(req->conn, req->resp, (int) strlen(t)) >= 0) {
		send_data(req->conn, (unsigned char *)t, (int) strlen(t));
	}
}


void http_conn_disconnect(struct conn *c)
{
	struct http_stream_session *s =
		(struct http_stream_session *)c->proto_state;
	struct http_location *loc = s->loc;

	if (s->next) {
		s->next->prev = s->prev;
	}
	if (s->prev) {
		s->prev->next = s->next;
	} else {
		loc->sess_list = s->next;
	}
	if (s->resp) {
		free_pmsg(s->resp);
	}
	free(s);
	c->proto_state = NULL;
	if (! loc->sess_list) {
		set_waiting(loc->input, 0);
	}
}


static void add_http_stream_session(struct http_location *loc, struct conn *c,
									struct pmsg *resp)
{
	struct http_stream_session *s;

	s = (struct http_stream_session *)
		malloc(sizeof(struct http_stream_session));
	s->next = loc->sess_list;
	s->prev = NULL;
	if (s->next) {
		s->next->prev = s;
	}
	loc->sess_list = s;
	s->loc = loc;
	s->resp = resp;
	s->conn = c;
	c->proto_state = s;
}


static void send_frame_with_jfif(struct http_location *loc, struct conn *c)
{
	static unsigned char add_jfif[] = {
		0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46,
		0x00, 0x01, 0x02, 0x01, 0x00, 0x48, 0x00, 0x48, 0x00, 0x00
	};

	if (loc->length_with_jfif > loc->frame->length) {
		send_data(c, add_jfif, 20);
		send_data(c, loc->frame->d + 2, loc->frame->length - 2);
	} else {
		send_data(c, loc->frame->d, loc->frame->length);
	}
}


static int send_frame(struct http_location *loc, struct conn *c,
					  struct pmsg *resp)
{
	if (resp) {
		if (avail_send_buf(c) < loc->length_with_jfif + 1024) {
			return -1;
		}
		tcp_send_pmsg(c, resp, loc->length_with_jfif);
		free_pmsg(resp);
		c->drop_after = 1;
	} else {
		char hdrs[128];
		int hlen;

		hlen = sprintf(hdrs, "--boundary\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n", loc->length_with_jfif);
		if (avail_send_buf(c) < loc->length_with_jfif + hlen) {
			return -1;
		}
		send_data(c, (unsigned char *)hdrs, hlen);
	}
	send_frame_with_jfif(loc, c);
	return 0;
}


static int check_frame_freshness(struct http_location *loc)
{
	struct timeval now;

	/* If there is no frame cached, we have no fresh frame */
	if (! loc->frame) {
		return 0;
	}

	/* If we haven't hit the cache expiration time, the frame is fresh */
	gettimeofday(&now, NULL);
	if (loc->frame_time.tv_sec + loc->max_cache_time > now.tv_sec) {
		return 1;
	}

	/* If the cached frame is stale, unref it in case the source is
	 * starved for free frames */
	unref_frame(loc->frame);
	loc->frame = NULL;

	return 0;
}


static int handle_GET(struct req *req)
{
	struct http_location *loc;

	loc = find_http_location(req->req->sl.req.uri);
	if (! loc || strlen(req->req->sl.req.uri) > strlen(loc->node.path)) {
		http_send_error(req, 404, "Not Found", "<html><body><p>File not found</p></body></html>");
		return 0;
	}
	http_create_reply(req, 200, "OK");
	add_header(req->resp, "Expires", "0");
	add_header(req->resp, "Pragma", "no-cache");
	add_header(req->resp, "Cache-Control", "no-cache");
	if (loc->streaming) {
		log_request(req, 200, 0);
		add_header(req->resp, "Content-Type",
				   "multipart/x-mixed-replace;boundary=\"boundary\"");
		tcp_send_pmsg(req->conn, req->resp, -1);
		add_http_stream_session(loc, req->conn, NULL);
	} else {
		/* Steal the request pmsg from the req since we free it
		 * later in send_frame */
		struct pmsg *resp = req->resp;

		log_request(req, 200, 0);
		req->resp = NULL;
		add_header(resp, "Connection", "close");
		add_header(resp, "Content-Type", "image/jpeg");
		if (! check_frame_freshness(loc) ||
			send_frame(loc, req->conn, resp) < 0) {
			add_http_stream_session(loc, req->conn, resp);
		}
	}
	if (loc->sess_list) {
		set_waiting(loc->input, 1);
	}
	return 0;
}


static int handle_unknown(struct req *req)
{
	http_send_error(req, 501, "Not Implemented", "<html><body><p>POST requests are unsupported here</p></body></html>");
	return 0;
}


int http_handle_msg(struct req *req)
{
	int ret;

	if (! strcasecmp(req->req->sl.req.method, "GET")) {
		ret = handle_GET(req);
	} else {
		ret = handle_unknown(req);
	}
	return ret;
}

