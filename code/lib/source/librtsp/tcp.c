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
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>      // Carl
#include <net/if.h>         // Carl
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sysdef.h"         // Carl

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <pmsg.h>

#include <rtp.h>
#include <base64_table.h>
#include "live.h"           // Carl
#include "tcp.h"            // Carl
#include "gm_memory.h"


// Carl
extern int  rtsp_tcp_port;
extern char *gmss_localaddr;
extern int  gmss_conn_max;
extern int  gmss_conn;


#if 0   // Carl
void write_access_log(char *path, struct sockaddr *addr, int code, char *req,
					  int length, char *referer, char *user_agent);
#endif

struct listener *listener   = NULL;
static struct conn *conn_list = NULL;

static void do_read(struct event_info *ei, void *d);

struct rtsp_conn {
	struct {
		struct rtp_endpoint *ep;
		int rtcp;
	} ichan[MAX_INTERLEAVE_CHANNELS];
};

void drop_conn(struct conn *c, int callback)     // Carl
{
	if (c->wait_teardown_reply > 0) {
		c->wait_teardown_reply--;
		c->drop_after = 1;
		return;
	}

	if (c->proto_state) {
		switch (c->proto) {
		case CONN_PROTO_HTTP:
			http_conn_disconnect(c);
			break;
		case CONN_PROTO_RTSP:
			rtsp_conn_disconnect(c, callback);  // Carl
			break;
		}
	} else {
		printf("==> %s, line = %d, c->proto_state is NULL\n", __FUNCTION__, __LINE__);
	}

#if 0
	remove_event(c->read_event);
	remove_event(c->write_event);
#else
	if (c->read_event != NULL) {
		remove_event(c->read_event);
	} else {
		printf("==> %s, line = %d, c->read_event is NULL\n", __FUNCTION__, __LINE__);
	}

	if (c->write_event != NULL) {
		remove_event(c->write_event);
	} else {
		printf("==> %s, line = %d, c->write_event is NULL\n", __FUNCTION__, __LINE__);
	}
#endif

	if (c->fd >= 0) {
		close(c->fd);
	}
	c->fd = -1;
	if (c->next) {
		c->next->prev = c->prev;
	}
	if (c->prev) {
		c->prev->next = c->next;
	} else {
		conn_list = c->next;
	}
	free(c);

	if (0 < gmss_conn) {
		--gmss_conn;    // Carl
	}
}


// Carl
void conn_cleanall(void)
{
	for (; (conn_list); drop_conn(conn_list, 0));
}


static void conn_write(struct event_info *ei, void *d)
{
	struct conn *c = (struct conn *)d;
	int ret, len;

	while (c->send_buf_r != c->send_buf_w) {
		if (c->send_buf_w < c->send_buf_r) {
			len = sizeof(c->send_buf) - c->send_buf_r;
		} else {
			len = c->send_buf_w - c->send_buf_r;
		}
		//ret = write( c->fd, c->send_buf + c->send_buf_r, len );
		ret = write(c->write_fd, c->send_buf + c->send_buf_r, len);

		if (c->wait_teardown_reply && strncmp((char *)(c->send_buf + c->send_buf_r), "RTSP/1.0 200 OK", 15) == 0) {
			c->wait_teardown_reply = 0;
		}

		if (ret <= 0) {
			if (ret < 0 && errno == EAGAIN) {
				return;
			}
			drop_conn(c, 1);     // Carl
			return;
		}
		c->send_buf_r += ret;
		if (c->send_buf_r == sizeof(c->send_buf)) {
			c->send_buf_r = 0;
		}
	}
	if (c->drop_after) {
		drop_conn(c, 1);    // Carl
	} else {
		set_event_enabled(c->write_event, 0);
	}
}

int avail_send_buf(struct conn *c)
{
	if (c->send_buf_r > c->send_buf_w) {
		return c->send_buf_r - c->send_buf_w - 1;
	} else {
		return sizeof(c->send_buf) - c->send_buf_w + c->send_buf_r - 1;
	}
}

int send_data(struct conn *c, unsigned char *d, int len)
{

	if (avail_send_buf(c) < len) {
		return 1;
	}
	while (--len >= 0) {
		c->send_buf[c->send_buf_w++] = *(d++);
		if (c->send_buf_w == sizeof(c->send_buf)) {
			c->send_buf_w = 0;
		}
	}
	set_event_enabled(c->write_event, 1);

	return 0;
}

int tcp_send_pmsg(struct conn *c, struct pmsg *msg, int len)
{
	unsigned char buf[2048];
	int i, f, totlen;

	if (msg->type != PMSG_RESP) {
		return -1;
	}
	i = sprintf((char *)buf, "%s %d %s\r\n", msg->proto_id, msg->sl.stat.code,
				msg->sl.stat.reason);
	for (f = 0; f < msg->header_count; ++f)
		i += sprintf((char *)(buf + i), "%s: %s\r\n", msg->fields[f].name,
					 msg->fields[f].value);
	if (len >= 0) {
		i += sprintf((char *)(buf + i), "Content-Length: %d\r\n\r\n", len);
		totlen = i + len;
	} else {
		i += sprintf((char *)(buf + i), "\r\n");
		totlen = i;
	}
	if (avail_send_buf(c) < totlen) {
		return -1;
	}
	send_data(c, buf, i);

	// dbg("%s\n", buf);    // Carl

	return 0;
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

static int handle_QT_tunnel(struct req *req, int post)
{
	unsigned char rep[8192];
	char *t;
	struct conn *cg;

	if (!(t = get_header(req->req, "x-sessioncookie")) ||
		strlen(t) + 1 > sizeof(req->conn->http_tunnel_cookie)) {
		send_data(req->conn, rep, sprintf((char *)rep, "HTTP/1.0 400 Missing session cookie\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n<html><body><p>Broken RTSP/HTTP tunnel request</p></body></html>\n"));
		req->conn->drop_after = 1;
		return 0;
	}
	if (! post) {
		strcpy(req->conn->http_tunnel_cookie, t);
		spook_log(SL_VERBOSE,
				  "processing RTSP-HTTP tunnel GET for %s", t);
		send_data(req->conn, rep, sprintf((char *)rep, "HTTP/1.0 200 OK\r\nConnection: close\r\nCache-Control: no-cache\r\nPragma: no-cache\r\nContent-Type: application/x-rtsp-tunnelled\r\n\r\n"));
		req->conn->proto = CONN_PROTO_RTSP;
		return 0;
	}
	for (cg = conn_list; cg; cg = cg->next)
		if (! strcmp(cg->http_tunnel_cookie, t)) {
			break;
		}
	if (! cg) {
		spook_log(SL_VERBOSE,
				  "unable to locate RTSP-HTTP GET request for %s", t);
		send_data(req->conn, rep, sprintf((char *)rep, "HTTP/1.0 400 Unknown session cookie\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n<html><body><p>Broken RTSP/HTTP tunnel request</p></body></html>\n"));
		req->conn->drop_after = 1;
		return 0;
	}
	spook_log(SL_VERBOSE, "RTSP-HTTP tunnel established for %s(fd=%d)", t,  cg->fd);
	//req->conn->fd = -1;

	req->conn->stream_conn = cg;
	req->conn->write_fd = cg->write_fd;
	req->conn->base64_count = 0;
	req->conn->proto = CONN_PROTO_RTSP;
	return 1;
}


static int handle_GET(struct req *req)
{
	char *t;

	if ((t = get_header(req->req, "Accept")) &&
		! strcasecmp(t, "application/x-rtsp-tunnelled")) {
		if (handle_QT_tunnel(req, 0)) {
			return -1;
		}
		return 0;
	}

	return http_handle_msg(req);
}


static int handle_POST(struct req *req)
{
	char *t;

	if ((t = get_header(req->req, "Content-Type")) &&
		! strcasecmp(t, "application/x-rtsp-tunnelled")) {
		//if( handle_QT_tunnel( req, 1 ) ) return -1;
		handle_QT_tunnel(req, 1);
		return 0;
	}

	return http_handle_msg(req);
}


static int handle_unknown(struct req *req)
{
	log_request(req, 501, 0);
	req->conn->drop_after = 1;
	req->resp = new_pmsg(256);
	req->resp->type = PMSG_RESP;
	req->resp->proto_id = add_pmsg_string(req->resp, req->req->proto_id);
	req->resp->sl.stat.code = 501;
	req->resp->sl.stat.reason = add_pmsg_string(req->resp, "Not Implemented");
	copy_headers(req->resp, req->req, "CSeq");
	tcp_send_pmsg(req->conn, req->resp, -1);
	return 0;
}

static int unbase64(unsigned char *d, int len, int *remain);

static int handle_request(struct conn *c)
{
	int ret;
	struct req *req;
	int msg_pos;

	if (c->req_buf[0] == 0) {
		return -1;
	}

	if ((req = malloc(sizeof(struct req))) == NULL) {
		return -1;
	}
	req->conn = c->stream_conn;
	req->resp = NULL;
	req->req = new_pmsg(c->req_len);
	req->req->msg_len = c->req_len;
	memcpy(req->req->msg, c->req_buf, req->req->msg_len);
	if ((msg_pos = parse_pmsg(req->req)) < 0) {
		free_pmsg(req->req);
		free(req);
		return -1;
	}


	spook_log(SL_VERBOSE, "client request: '%s' '%s' '%s'",
			  req->req->sl.req.method, req->req->sl.req.uri,
			  req->req->proto_id);

	switch (c->proto) {

	case CONN_PROTO_HTTP:
		if (! strcasecmp(req->req->sl.req.method, "GET")) {
			ret = handle_GET(req);
		} else if (! strcasecmp(req->req->sl.req.method, "POST")) {
			ret = handle_POST(req);

			if (msg_pos < c->req_len) {
				spook_log(SL_VERBOSE, "handle request(remaining): fd=%d len=%d pos=%d",
						  c->fd, c->req_len, msg_pos);
				memcpy(c->req_buf, c->req_buf + msg_pos, c->req_len - msg_pos);
				c->req_len =
					unbase64(c->req_buf,
							 c->req_len - msg_pos,
							 &c->base64_count);
				c->base64_count = 0;
				c->proto = CONN_PROTO_RTSP;
				ret = 2;
			}
		} else {
			ret = http_handle_msg(req);
		}
		break;
	case CONN_PROTO_RTSP:

		ret = rtsp_handle_msg(req, c);

		break;
	default:
		ret = handle_unknown(req);
		break;
	}
	if (ret <= 0) {
		free_pmsg(req->req);
		if (req->resp) {
			free_pmsg(req->resp);
		}
		free(req);
	}

	//return ret < 0 ? -1 : 0;
	return ret;
}

static int parse_client_data(struct conn *c)
{
	char *a, *b;
	int ret = 0;

	switch (c->proto) {
	case CONN_PROTO_START:
		if ((a = strchr((char *)(c->req_buf), '\n'))) {

			*a = 0;
			if (!(b = strrchr((char *)(c->req_buf), ' '))) {
				return -1;
			}
			if (! strncmp(b + 1, "HTTP/", 5)) {
				c->proto = CONN_PROTO_HTTP;
			} else if (! strncmp(b + 1, "RTSP/", 5)) {
				c->proto = CONN_PROTO_RTSP;
			} else {
				return -1;
			}
			*a = '\n';
			return 1;
		}
		break;
	case CONN_PROTO_RTSP:

		if (c->req_len < 4) {
			return 0;
		}
		if (c->req_buf[0] == '$') {
#if 1   /* Carl, 20090720. */ /* Fix VLC long-term disconnection issue. */
			int len = (int) GET_16(c->req_buf + 2);
			if (c->req_len < (len + 4)) {
				return 0;
			}

			ret = interleave_recv(c, c->req_buf[1], c->req_buf + 4, len);

			if (ret < 0) {
				return -1;
			}

			c->req_len -= len + 4;
			if (c->req_len) {
				memcpy(c->req_buf, c->req_buf + len + 4, c->req_len);
				return 1;   /* Carl, 20090721. */
			}
#else
			if (c->req_len < GET_16(c->req_buf + 2) + 4) {
				return 0;
			}
			interleave_recv(c, c->req_buf[1], c->req_buf + 4,
							GET_16(c->req_buf + 2));
			/* should really just delete the packet from the
			 * buffer, not the entire buffer */
			c->req_len = 0;
#endif
		} else {
			//find a substring
			if (! strstr((char *)(c->req_buf), "\r\n\r\n")) {
				return 0;
			}

			if (handle_request(c) < 0) {
				return -1;
			}
			/* should really just delete the request from the
			 * buffer, not the entire buffer */

			c->req_len = 0;
		}
		break;
	case CONN_PROTO_HTTP:
		if (! strstr((char *)(c->req_buf), "\r\n\r\n")) {
			return 0;
		}
		if ((ret = handle_request(c)) < 0) {
			return -1;
		}
		/* should really just delete the request from the
		 * buffer, not the entire buffer */
		if (ret == 0) {
			c->req_len = 0;
		}
		break;
	default:
		printf("==> %s, line = %d >> connection proto not support\n", __FUNCTION__, __LINE__);
		return -1;
		break;
	}
	return ret;
}

static int unbase64(unsigned char *d, int len, int *remain)
{
	int src = 0, dest = 0, i;
	unsigned char enc[4];

	for (;;) {
		for (i = 0; i < 4; ++src) {
			if (src >= len) {
				if (i > 0) {
					memcpy(d + dest, enc, i);
				}
				*remain = i;
				return dest + i;
			}
			if (base64[d[src]] < 0) {
				continue;
			}
			enc[i++] = d[src];
		}
		d[dest] = ((base64[enc[0]] << 2) & 0xFC) |
				  (base64[enc[1]] >> 4);
		d[dest + 1] = ((base64[enc[1]] << 4) & 0xF0) |
					  (base64[enc[2]] >> 2);
		d[dest + 2] = ((base64[enc[2]] << 6) & 0xC0) |
					  base64[enc[3]];
		if (enc[3] == '=') {
			if (enc[2] == '=') {
				dest += 1;
			} else {
				dest += 2;
			}
		} else {
			dest += 3;
		}
	}
}

static void do_read(struct event_info *ei, void *d)
{
	struct conn *c = (struct conn *)d;
	int ret;

	for (;;) {
		ret = read(c->fd,
				   c->req_buf + c->req_len,
				   sizeof(c->req_buf) - c->req_len - 1);
		if (ret <= 0) {
			if (ret < 0) {
				if (errno == EAGAIN) {
					return;
				}
				spook_log(SL_VERBOSE, "closing TCP connection to client due to read error: %s(%d)",
						  strerror(errno), c->fd);
			} else {
				spook_log(SL_VERBOSE, "client closed TCP connection");
			}
			{
				drop_conn(c, 1);     // Carl
			}
			return;
		}

#if 1
		if (c->req_len + ret <= sizeof(c->req_buf) - 1) {
			c->req_buf[c->req_len + ret] = 0;
		} else {
			c->req_buf[sizeof(c->req_buf) - 1] = 0;
		}
#endif
		spook_log(SL_VERBOSE, "rtsp do_read(%d): %d %d %d %s",
				  c->fd, ret, c->base64_count, c->req_len, c->req_buf + c->req_len);
		if (c->base64_count >= 0) {
			c->req_len -= c->base64_count;
			c->req_len +=
				unbase64(c->req_buf + c->req_len,
						 ret + c->base64_count,
						 &c->base64_count);
		} else {
			c->req_len += ret;
		}

		if (c->req_len == sizeof(c->req_buf) - 1) {
			spook_log(SL_VERBOSE, "malformed request from client; exceeded maximum size");
			drop_conn(c, 1);     // Carl
			return;
		}

		if (c->base64_count > 0) {
			continue;
		}

		c->req_buf[c->req_len] = 0;
		while ((ret = parse_client_data(c)) > 0);
		if (ret < 0) {
			drop_conn(c, 1);     // Carl
			return;
		}
	}
}


static void do_accept(struct event_info *ei, void *d)
{
	struct listener *listener = (struct listener *)d;
	int fd, addrlen;
	struct sockaddr_in addr;
	struct conn *c;
	int sendbuf = 102400;
	int keepAlive = 1;
	int keepIdle = 20;
	int keepInterval = 20;
	int keepCount = 3;

	addrlen = sizeof(addr);
	if ((fd = accept(listener->fd, (struct sockaddr *)&addr, (socklen_t *) &addrlen)) < 0) {
		return;
	}

	// Carl
	if (gmss_conn_max <= gmss_conn) {
		close(fd);
		return;
	} else {
		++gmss_conn;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&sendbuf, sizeof(sendbuf)) < 0)
		spook_log(SL_WARN, "ignoring error on setsockopt: %s",
				  strerror(errno));
	spook_log(SL_VERBOSE, "accepted connection from %s:%d",
			  inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	fcntl(fd, F_SETFL, O_NONBLOCK);
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive)) == -1) {
		spook_log(SL_VERBOSE, "setsockopt SO_KEEPALIVE error!");
	}
	if (setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void *)&keepIdle, sizeof(keepIdle)) == -1) {
		spook_log(SL_VERBOSE, "setsockopt TCP_KEEPIDLE error!");
	}
	if (setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval)) == -1) {
		spook_log(SL_VERBOSE, "setsockopt TCP_KEEPINTVL error!");
	}
	if (setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)) == -1) {
		spook_log(SL_VERBOSE, "setsockopt TCP_KEEPCNT error!");
	}

	c = (struct conn *)malloc(sizeof(struct conn));
	c->next = conn_list;
	if (c->next) {
		c->next->prev = c;
	}
	c->prev = NULL;
	c->file = 0;    // Carl
	c->fd = fd;
	c->write_fd = fd;
	c->client_addr = addr;
	c->proto = CONN_PROTO_START;
	c->http_tunnel_cookie[0] = 0;
	c->base64_count = -1;
	c->req_len = 0;
	c->req_list = NULL;
	c->read_event = add_fd_event(fd, 0, 0, do_read, c);
	c->write_event = add_fd_event(fd, 1, 0, conn_write, c);
	set_event_enabled(c->write_event, 0);
	c->send_buf_r = c->send_buf_w = 0;
	c->drop_after = 0;
	c->proto_state = NULL;
	c->stream_conn = c;
	c->wait_teardown_reply = 0;
	conn_list = c;
}


extern  char    gmss_ipif[];    // Carl


static int tcp_listen(int port)
{
	struct sockaddr_in addr;
	struct ifreq    ifreq;  // Carl
	char            *laddr; // Carl
	int opt, fd;

	if ((fd = socket(PF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) {
		spook_log(SL_ERR, "error creating listen socket: %s",
				  strerror(errno));
		return -1;
	}
	opt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		spook_log(SL_WARN, "ignoring error on setsockopt: %s",
				  strerror(errno));

	addr.sin_family = AF_INET;
	// Carl
	if (!inet_aton(gmss_ipif, &(addr.sin_addr))) {
		// inteface name or not
		memset(&ifreq, 0, sizeof(ifreq));
		strncpy(ifreq.ifr_name, gmss_ipif, IFNAMSIZ - 1);
		if (0 <= ioctl(fd, SIOCGIFADDR, &ifreq)) {
			addr.sin_addr.s_addr = ((struct sockaddr_in *) & (ifreq.ifr_addr))->sin_addr.s_addr;
		} else {
			addr.sin_addr.s_addr = 0;
		}
	}

	laddr = inet_ntoa(addr.sin_addr);
	dbg("Listen on address %s, TCP port %d\n", laddr, port);
	if (gmss_localaddr) {
		free(gmss_localaddr);
	}
	gmss_localaddr = malloc(strlen(laddr) + 1);
	if (gmss_localaddr) {
		strcpy(gmss_localaddr, laddr);
	}

	addr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		spook_log(SL_ERR, "unable to bind to tcp socket: %s",
				  strerror(errno));
		close(fd);
		return -1;
	}
	if (listen(fd, 5) < 0) {
		spook_log(SL_ERR,
				  "error when attempting to listen on tcp socket: %s",
				  strerror(errno));
		close(fd);
		return -1;
	}

	listener = (struct listener *)malloc(sizeof(struct listener));
	listener->fd = fd;

	add_fd_event(fd, 0, 0, do_accept, listener);

	spook_log(SL_INFO, "listening on tcp port %d", port);

	return 0;
}

/********************* GLOBAL CONFIGURATION DIRECTIVES ********************/

int config_port(int port)
{
	if ((port <= 0) || (port > 65535)) {
		spook_log(SL_ERR, "invalid listen port %d", port);
		return -1;
	}

	return tcp_listen(port);
}

