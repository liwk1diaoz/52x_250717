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
#include <netinet/in.h>
#include <netinet/tcp.h>    // Carl
#include <arpa/inet.h>

#include "sysdef.h"         // Carl

#include <event.h>
#include <log.h>
#include <frame.h>
#include <stream.h>
#include <pmsg.h>

#include <rtp.h>
#include <live.h>           // Carl
#include "gm_memory.h"

// Carl
extern int      gmss_sr_max;
extern gmss_sr  *gmss_srs;
extern int rtsp_log_conn_stat_mode;

#if 0   // Carl
void write_access_log(char *path, struct sockaddr *addr, int code, char *req,
					  int length, char *referer, char *user_agent);
#endif
int live_get_sdp(gmss_sr *sr, char *dest, int *len);        // Carl
char *uri_file(char *path);                                 // Carl
int call_cmd_cb(char *name, int cmd, void *conn, void *p);  // Carl
int describe_parse_username(struct req *req, char *username, int username_len);


struct rtsp_session {
	struct rtsp_session *next;
	struct rtsp_session *prev;
	char id[32];
	struct session *sess;
};

struct rtsp_location {
	struct loc_node node;
	/*
	char realm[128];
	char username[128];
	char password[128];
	*/
	open_func open;
	void *private;
};

struct rtsp_conn {
	struct {
		struct rtp_endpoint *ep;
		int rtcp;
	} ichan[MAX_INTERLEAVE_CHANNELS];
};

static struct rtsp_location *rtsp_loc_list = NULL;
static struct rtsp_session *sess_list = NULL;

static char *get_local_path(char *path)
{
	char *c;
	static char *root = "/";

	if (strncasecmp(path, "rtsp://", 7)) {
		return NULL;
	}
	for (c = path + 7; *c != '/'; ++c) if (*c == 0) {
			return root;
		}
	return c;
}


/* Carl */
static gmss_sr *node_find_location(char *path, int len, void *conn)
{
	int     i;

	while (*path == '/') {
		if (1 < len) {
			++path;
			--len;
		} else {
			return NULL;
		}
	}

	for (i = 0; i < gmss_sr_max; ++i) {
		if ((gmss_srs[i].status == SR_STATUS_USED) && (gmss_srs[i].conn == conn) &&
			(strlen(gmss_srs[i].name) == len) &&
			(!strncmp(gmss_srs[i].name, path, len))) {
			return &(gmss_srs[i]);
		}
	}

	return NULL;
}


static gmss_sr *find_rtsp_location(char *uri, char *base, int *track, void *conn)
{
	char *path, *c, *end;
	int len;

	if (!(path = get_local_path(uri))) {
		return NULL;
	}
	len = strlen(path);

	if (track) {
		*track = -1;

		if ((c = strrchr(path, '/')) &&
			! strncmp(c, "/trackID=", 9)) {
			*track = strtol(c + 9, &end, 10);
			if (! *end) {
				len -= strlen(c);
			}
		}
	}

	if (len > 1 && path[len - 1] == '/') {
		--len;
	}

	if (base) {
		strncpy(base, path, len);
		base[len] = 0;
	}

	/* Carl */
	return node_find_location(path, len, conn);
}

static void init_location(struct loc_node *node, char *path,
						  struct loc_node **list)
{
	node->next = *list;
	node->prev = NULL;
	if (node->next) {
		node->next->prev = node;
	}
	*list = node;
	strcpy(node->path, path);
}

void new_rtsp_location(char *path, char *realm, char *username, char *password,
					   open_func open, void *private)
{
	struct rtsp_location *loc;

	loc = (struct rtsp_location *)malloc(sizeof(struct rtsp_location));
	init_location((struct loc_node *)loc, path, (struct loc_node **)&rtsp_loc_list);
	/*
	if( realm ) strcpy( loc->realm, realm );
	else loc->realm[0] = 0;
	if( username ) strcpy( loc->username, username );
	else loc->username[0] = 0;
	if( password ) strcpy( loc->password, password );
	else loc->password[0] = 0;
	*/
	loc->open = open;
	loc->private = private;
}

static void rtsp_session_close(struct session *sess)
{
	struct rtsp_session *rs = (struct rtsp_session *)sess->control_private;
	spook_log(SL_DEBUG, "freeing session %s", rs->id);
	if (rs->next) {
		rs->next->prev = rs->prev;
	}
	if (rs->prev) {
		rs->prev->next = rs->next;
	} else {
		sess_list = rs->next;
	}
	free(rs);
}

static struct rtsp_session *new_rtsp_session(struct session *sess)
{
	struct rtsp_session *rs;
	rs = (struct rtsp_session *)malloc(sizeof(struct rtsp_session));
	rs->next = sess_list;
	rs->prev = NULL;
	if (rs->next) {
		rs->next->prev = rs;
	}
	sess_list = rs;
	random_id((unsigned char *)(rs->id), 20);
	rs->sess = sess;
	sess->control_private = rs;
	sess->control_close = rtsp_session_close;
	return rs;
}

static struct rtsp_session *get_session(char *id)
{
	struct rtsp_session *rs;

	if (! id) {
		return NULL;
	}
	for (rs = sess_list; rs; rs = rs->next)
		if (! strcmp(rs->id, id)) {
			break;
		}
	return rs;
}

void rtsp_conn_disconnect(struct conn *c, int callback)  // Carl
{
	struct rtsp_conn *rc = (struct rtsp_conn *)c->proto_state;
	int i;

	if (rc) {
		for (i = 0; i < MAX_INTERLEAVE_CHANNELS; ++i)
			if (rc->ichan[i].ep && ! rc->ichan[i].rtcp) {
				// dbg("%s:ichan[%d].ep %p, rtcp %d\n", __func__, i, rc->ichan[i].ep, rc->ichan[i].rtcp);
				rc->ichan[i].ep->session->teardown(
					rc->ichan[i].ep->session,
					rc->ichan[i].ep, callback); // Carl
			}
		free(rc);
		return;
	}
}

void interleave_disconnect(struct conn *c, int chan)
{
	struct rtsp_conn *rc;

	if (c == NULL) {
		return;
	}

	rc = (struct rtsp_conn *)c->proto_state;

	//kevin 2006.12.28 add check
	if (rc == NULL) {
		return;
	}

	rc->ichan[chan].ep = NULL;
}

int interleave_recv(struct conn *c, int chan, unsigned char *d, int len)
{
	struct rtsp_conn *rc = (struct rtsp_conn *)c->proto_state;

	if (rc == NULL) {
		//printf("==> %s, line = %d, Warnning!! c->proto_state is NULL\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (chan >= MAX_INTERLEAVE_CHANNELS || ! rc->ichan[chan].ep) {
		return -1;
	}

	//#ifdef RTCP_ENABLE
	if (rtcp_enable) {
		if (rc->ichan[chan].rtcp) {
			interleave_recv_rtcp(rc->ichan[chan].ep, d, len);
		}
	}
	//#endif
	return 0;
}

int interleave_send(struct conn *c, int chan, struct iovec *v, int count)
{
	int i;

	for (i = 0; i < count; ++i) {
		send_data(c, v[i].iov_base, v[i].iov_len);
	}
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

static int rtsp_create_reply(struct req *req, int code, char *reason)
{
	if (!(req->resp = new_pmsg(1024))) {
		return -1;    // Carl
	}
	req->resp->type = PMSG_RESP;
	req->resp->proto_id = add_pmsg_string(req->resp, "RTSP/1.0");
	req->resp->sl.stat.code = code;
	req->resp->sl.stat.reason = add_pmsg_string(req->resp, reason);
	copy_headers(req->resp, req->req, "CSeq");
	return 0;
}

static void rtsp_send_error(struct req *req, int code, char *reason)
{
	log_request(req, code, 0);
	rtsp_create_reply(req, code, reason);
	tcp_send_pmsg(req->conn, req->resp, -1);
}


#if 1   /* Carl */
static int rtsp_verify_auth(struct req *req, char *realm,
							char *username, char *password)
{
	int ret = check_digest_response(req->req, realm, username, password);

	if (ret > 0) {
		return 0;
	}

	log_request(req, 401, 0);
	rtsp_create_reply(req, 401, "Unauthorized");
	add_digest_challenge(req->resp, realm, ret == 0 ? 1 : 0);
	tcp_send_pmsg(req->conn, req->resp, -1);
	return -1;
}
#endif


int stream_max_client_check(struct req *req, gmss_sr *sr)
{
	struct live_source  *ls;
	struct live_session *lsess;
	struct rtp_endpoint *ep;
	int client_cnt = 0;

	if (sr->status == SR_STATUS_NULL) {
		goto stream_max_client_check_reject;
	}
	if ((ls = (struct live_source *)(sr->live_source)) == NULL) {
		goto stream_max_client_check_reject;
	}

	lsess = ls->sess_list;

	while (lsess) {
		if (lsess->sess == NULL) {
			break;
		}
		ep = (lsess->sess->ep[0]) ? lsess->sess->ep[0] : lsess->sess->ep[1];
		if ((ep == NULL) || (ep->trans.inter.conn == NULL)) {
			break;
		}
		lsess = lsess->next;
		client_cnt++;
	}

	if (!sr->max_connection || client_cnt < sr->max_connection) {
		return 0;
	}

stream_max_client_check_reject:
	log_request(req, 406, 0);
	rtsp_create_reply(req, 406, "Not Acceptable");
	tcp_send_pmsg(req->conn, req->resp, -1);
	return -1;
}

void log_connection_status()
{
	int i;
	struct live_source  *ls;
	struct live_session *lsess;
	struct rtp_endpoint *ep;
	gmss_sr         *sr;
	char ip_str[INET_ADDRSTRLEN];
	struct tm *timeinfo;
	char datetime_str[32];
	FILE *pFile;

	if ((pFile = fopen("/var/rtsp_status", "w")) == NULL) {
		return;
	}

	if (lockf(fileno(pFile), F_TLOCK, 0) == -1) {
		if (pFile != NULL) {
			fclose(pFile);
		}
		return;
	}

	for (i = 0; i < gmss_sr_max; i++) {
		sr = &(gmss_srs[i]);
		if (!sr->live) {
			continue;
		}

		if (sr->status == SR_STATUS_NULL) {
			continue;
		}
		if ((ls = (struct live_source *)(sr->live_source)) == NULL) {
			continue;
		}
		if ((lsess = ls->sess_list) == NULL) {
			continue;
		}

		//printf("%s\n", sr->name);
		if (pFile != NULL) {
			fprintf(pFile, "%s\n", sr->name);
		}

		while (lsess) {
			if (lsess->sess == NULL) {
				break;
			}
			ep = (lsess->sess->ep[0]) ? lsess->sess->ep[0] : lsess->sess->ep[1];
			if ((ep == NULL) || (ep->trans_type == 0)) {
				break;
			}

			switch (ep->trans_type) {
#if (RTP_OVER_UDP)
			case RTP_TRANS_UDP:
				inet_ntop(AF_INET, &(ep->trans.udp.conn->client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
				timeinfo = localtime(&ep->ep_create_time);
				strftime(datetime_str, sizeof(datetime_str), "%F %T", timeinfo);
				//printf("  [UDP] %s:%d  %s\n", ip_str, ep->trans.udp.conn->client_addr.sin_port, datetime_str);
				if (pFile != NULL) {
					fprintf(pFile, "  [UDP] %s:%d  %s\n", ip_str, ntohs(ep->trans.udp.conn->client_addr.sin_port), datetime_str);
				}
				break;
#endif
			case RTP_TRANS_INTER:
				inet_ntop(AF_INET, &(ep->trans.inter.conn->client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
				timeinfo = localtime(&ep->ep_create_time);
				strftime(datetime_str, sizeof(datetime_str), "%F %T", timeinfo);
				//printf("  [TCP] %s:%d  %s\n", ip_str, ep->trans.inter.conn->client_addr.sin_port, datetime_str);
				if (pFile != NULL) {
					fprintf(pFile, "  [TCP] %s:%d  %s\n", ip_str, ntohs(ep->trans.inter.conn->client_addr.sin_port), datetime_str);
				}
				break;
			}

			lsess = lsess->next;
		}
	}

	lockf(fileno(pFile), F_ULOCK, 0);
	if (pFile != NULL) {
		fclose(pFile);
	}
}

static int handle_OPTIONS(struct req *req)
{
	char    *pn = NULL;

	// dbg("%s\n%s\n", __func__, req->conn->req_buf);

	pn = get_local_path(req->req->sl.req.uri);
	if ((pn)) {
		call_cmd_cb(pn, GM_STREAM_CMD_OPTION, NULL, NULL);
	}

	rtsp_create_reply(req, 200, "OK");
	add_header(req->resp, "Server", ServerName);
	add_header(req->resp, "Public",
			   "DESCRIBE, SETUP, TEARDOWN, PLAY"
#if (SUPPORT_PAUSE)     // Carl
			   ", PAUSE"
#endif
#if (SUPPORT_SET_PARAMETER)
			   ", SET_PARAMETER, GET_PARAMETER"
#endif
			  );
	log_request(req, 200, 0);
	tcp_send_pmsg(req->conn, req->resp, -1);
	return 0;
}

static int handle_DESCRIBE(struct req *req)
{
	char sdp[640], path[256], hdr[512];
	int sdp_len;
	gmss_sr *sr;
	char    *pn = NULL;
	int     ret;
	char req_username[128] = {0};

	// dbg("%s\n%s\n", __func__, req->conn->req_buf);

	pn = get_local_path(req->req->sl.req.uri);
	if ((pn)) {
		if (describe_parse_username(req, req_username, sizeof(req_username) - 1) != 0) {
			call_cmd_cb(pn, GM_STREAM_CMD_DESCRIBE, NULL, req_username);
		} else {
			call_cmd_cb(pn, GM_STREAM_CMD_DESCRIBE, NULL, NULL);
		}
		pn = NULL;
	}

	if (!(sr = find_rtsp_location(req->req->sl.req.uri, path, NULL,
								  (req->conn->file) ? req->conn : NULL))) {
		// Carl
		pn = uri_file(get_local_path(req->req->sl.req.uri));
		if ((pn) && (0 <= (ret = call_cmd_cb(pn, GM_STREAM_CMD_OPEN, NULL, NULL))) &&
			(ret < gmss_sr_max)) {
			sr = &(gmss_srs[ret]);
			sr->conn = req->conn;
			req->conn->file = 1;
		} else {
			rtsp_send_error(req, 404, "Not Found");
			return 0;
		}
	}

#if 1   /* Carl */
	if (sr->realm[0] &&
		sr->username[0] &&
		sr->password[0] &&
		rtsp_verify_auth(req, sr->realm, sr->username, sr->password) < 0) {
		return 0;
	}
#endif

	if (stream_max_client_check(req, sr) < 0) {
		return 0;
	}

	sdp_len = sizeof(sdp) - 2;

	if (0 < live_get_sdp(sr, sdp, &sdp_len)) {
		rtsp_create_reply(req, 200, "OK");
		add_header(req->resp, "Server", ServerName);
		sprintf(hdr, "%s/", req->req->sl.req.uri);
		add_header(req->resp, "Content-Base", hdr);
		add_header(req->resp, "Content-Type", "application/sdp");
		log_request(req, 200, sdp_len);
		if (tcp_send_pmsg(req->conn, req->resp, sdp_len) >= 0) {
			send_data(req->conn, (unsigned char *)sdp, sdp_len);
		}
	} else {
		rtsp_send_error(req, 404, "Not Found");
	}

	// sess->teardown( sess, NULL );

	return 0;
}


#if (RTP_OVER_UDP)  // Carl
static int rtsp_udp_setup(struct session *s, int track,
						  struct req *req, char *t)
{
	char *p, *end, trans[128], path[256];
	int cport, server_port, i;
	gmss_sr *sr;
	struct rtsp_conn *rc = (struct rtsp_conn *)req->conn->proto_state;

	if (!(p = strstr(t, "client_port")) || *(p + 11) != '=') {
		if (!(p = strstr(t, "port")) || *(p + 4) != '=') {
			return -1;
		} else {
			cport = strtol(p + 5, &end, 10);
			if (end == p + 5) {
				return -1;
			}
		}
	} else {
		cport = strtol(p + 12, &end, 10);
		if (end == p + 12) {
			return -1;
		}
	}

	spook_log(SL_DEBUG, "client requested UDP port %d", cport);


	if (connect_udp_endpoint(s->ep[track], req->conn->client_addr.sin_addr,
							 cport, &server_port) < 0) {
		return -1;
	}

	add_header(req->resp, "Cache-Control", "no-cache");

	spook_log(SL_VERBOSE, "our port is %d, client port is %d",
			  server_port, cport);


	if ((!(sr = find_rtsp_location(req->req->sl.req.uri, path, &track,
								   (req->conn->file) ? req->conn : NULL))) || (track < 0) || (MAX_TRACKS <= track)) {
		rtsp_send_error(req, 404, "Not Found");
		return -1;
	}

	if (!sr->multicast.enabled) {
		sprintf(trans,
				"RTP/AVP;unicast;mode=play;client_port=%d-%d;server_port=%d-%d",
				cport, cport + 1, server_port, server_port + 1);
	} else {
		sprintf(trans,
				"RTP/AVP/UDP;destination=%s;port=%d-%d;ttl=1;mode=play",
				(track == 0) ? sr->multicast.v_ipaddr : sr->multicast.a_ipaddr,
				(track == 0) ? sr->multicast.v_port : sr->multicast.a_port,
				(track == 0) ? sr->multicast.v_port + 1 : sr->multicast.a_port + 1);
	}


	add_header(req->resp, "Transport", trans);
	if (! rc) {
		rc = (struct rtsp_conn *)malloc(sizeof(struct rtsp_conn));
		if (! rc) {
			spook_log(SL_ERR,
					  "out of memory on malloc rtsp_conn");
			return -1;
		}
		for (i = 0; i < MAX_INTERLEAVE_CHANNELS; ++i) {
			rc->ichan[i].ep = NULL;
		}
		req->conn->proto_state = rc;
	}
	rc->ichan[track].ep = s->ep[track];
	rc->ichan[track].rtcp = 0;
	rc->ichan[track].ep->trans.udp.conn = req->conn;
	rc->ichan[track].ep->trans.udp.rtp_chan = track;

	return 0;
}
#endif

static int rtsp_interleave_setup(struct session *s, int track,
								 struct req *req, char *t)
{
	struct rtsp_conn *rc = (struct rtsp_conn *)req->conn->proto_state;
	char *p, *end, trans[128];
	int rtp_chan = -1, rtcp_chan = -1, i;

	if ((p = strstr(t, "interleaved"))) {
		if (*(p + 11) != '=') {
			return -1;
		}
		rtp_chan = strtol(p + 12, &end, 10);
		rtcp_chan = rtp_chan + 1; // XXX make better parser
		if (end == p + 12) {
			return -1;
		}
		if (rtp_chan < 0 || rtcp_chan < 0 ||
			rtp_chan >= MAX_INTERLEAVE_CHANNELS ||
			rtcp_chan >= MAX_INTERLEAVE_CHANNELS) {
			return -1;
		}
		spook_log(SL_VERBOSE, "requested interleave channels %d-%d",
				  rtp_chan, rtcp_chan);
		if (rc && (rc->ichan[rtp_chan].ep ||
				   rc->ichan[rtcp_chan].ep)) {
			return -1;
		}
	} else {
		spook_log(SL_VERBOSE, "requested any interleave channel");
		if (rc) {
			for (i = 0; i < MAX_INTERLEAVE_CHANNELS; i += 2)
				if (! rc->ichan[i].ep && ! rc->ichan[i + 1].ep) {
					break;
				}
			if (i >= MAX_INTERLEAVE_CHANNELS) {
				return -1;
			}
			rtp_chan = i;
			rtcp_chan = i + 1;
		} else {
			rtp_chan = 0;
			rtcp_chan = 1;
		}
	}

	if (! rc) {
		rc = (struct rtsp_conn *)malloc(sizeof(struct rtsp_conn));
		if (! rc) {
			spook_log(SL_ERR,
					  "out of memory on malloc rtsp_conn");
			return -1;
		}
		for (i = 0; i < MAX_INTERLEAVE_CHANNELS; ++i) {
			rc->ichan[i].ep = NULL;
		}
		req->conn->proto_state = rc;
	}

	rc->ichan[rtp_chan].ep = s->ep[track];
	rc->ichan[rtp_chan].rtcp = 0;

	//#ifdef RTCP_ENABLE
	//if(rtcp_enable)  /* not send rtcp packet, but keep the port infomation to recv rtcp packet 20160527*/
	{
		rc->ichan[rtcp_chan].ep = s->ep[track];
		rc->ichan[rtcp_chan].rtcp = 1;
	}
	//#endif
	connect_interleaved_endpoint(s->ep[track], req->conn,
								 rtp_chan, rtcp_chan);

	sprintf(trans, "RTP/AVP/TCP;unicast;interleaved=%d-%d",
			rtp_chan, rtcp_chan);


	add_header(req->resp, "Transport", trans);
	return 0;
}

static int handle_SETUP(struct req *req)
{
	char *t, *sh, path[256];
	int track, ret;
	struct session *s;
	struct rtsp_session *rs = NULL;
	// struct rtsp_location *loc;
	gmss_sr *sr;
	char    *pn = NULL;
	char req_username[128] = {0};
	struct live_source *source ;
	struct sockaddr_in conn;
	int server_port[2];

	// dbg("%s\n%s\n", __func__, req->conn->req_buf);

	pn = get_local_path(req->req->sl.req.uri);
	if ((pn)) {
		if (describe_parse_username(req, req_username, sizeof(req_username) - 1) != 0) {
			call_cmd_cb(pn, GM_STREAM_CMD_SETUP, NULL, req_username);
		} else {
			call_cmd_cb(pn, GM_STREAM_CMD_SETUP, NULL, NULL);
		}
		pn = NULL;
	}

	if ((!(sr = find_rtsp_location(req->req->sl.req.uri, path, &track,
								   (req->conn->file) ? req->conn : NULL))) || (track < 0) || (MAX_TRACKS <= track)) {
		rtsp_send_error(req, 404, "Not Found");
		return 0;
	}

#if 1   /* Carl */
	if (sr->realm[0] &&
		sr->username[0] &&
		sr->password[0] &&
		rtsp_verify_auth(req, sr->realm, sr->username, sr->password) < 0) {
		return 0;
	}
#endif

	if (sr->multicast.enabled) {
		source = sr->live_source;

		if (!source->sess_list) {
			s = sr->open(path, sr->live_source);
			sr->multicast.session = s;

			strcpy(s->addr, sr->multicast.v_ipaddr);
			s->setup(s, 0, sr);
			conn.sin_addr.s_addr = inet_addr(sr->multicast.v_ipaddr);
			connect_udp_endpoint(s->ep[0], conn.sin_addr,
								 sr->multicast.v_port, &server_port[0]);
			strcpy(s->addr, sr->multicast.a_ipaddr);
			s->setup(s, 1, sr);
			conn.sin_addr.s_addr = inet_addr(sr->multicast.a_ipaddr);
			connect_udp_endpoint(s->ep[1], conn.sin_addr,
								 sr->multicast.a_port, &server_port[1]);

			s->play(s, NULL);
		}
	}

	if (!(t = get_header(req->req, "Transport"))) {
		// XXX better error reply
		rtsp_send_error(req, 461, "Unspecified Transport");
		return 0;
	}

	if (!(sh = get_header(req->req, "Session"))) {
		if (!(s = sr->open(path, sr->live_source))) {       // live_open()
			rtsp_send_error(req, 404, "Not Found");
			return 0;
		}
		sprintf(s->addr, "IP4 %s", inet_ntoa(req->conn->client_addr.sin_addr));
	} else if (!(rs = get_session(sh))) {
		rtsp_send_error(req, 454, "Session Not Found");
		return 0;
	} else {
		s = rs->sess;
	}

	if (s->ep[track]) {
		// XXX better error reply
		rtsp_send_error(req, 461, "Unsupported Transport");
		return 0;
	}

	if (s->setup(s, track, sr) < 0) {       // live_setup()
		rtsp_send_error(req, 404, "Not Found");
		if (!rs) {
			s->teardown(s, NULL, 1);    // Carl
		}
		return 0;
	}

	spook_log(SL_DEBUG, "setting up RTP (%s)", t);

	rtsp_create_reply(req, 200, "OK");

	if (!strncasecmp(t, "RTP/AVP/TCP", 11)) {
		ret = rtsp_interleave_setup(s, track, req, t);

	}
#if (RTP_OVER_UDP)  // Carl
	else if ((!strncasecmp(t, "RTP/AVP", 7) && (t[7] != '/')) ||
			 (!strncasecmp(t, "RTP/AVP/UDP", 11))) {
		ret = rtsp_udp_setup(s, track, req, t);
	}
#endif
	else {
		ret = -1;
	}

	if (ret < 0) {
		free_pmsg(req->resp);
		rtsp_send_error(req, 461, "Unsupported Transport");
		s->teardown(s, s->ep[track], 1);    // Carl
	} else {
		if (!rs) {
			rs = new_rtsp_session(s);
		}
		add_header(req->resp, "Session", rs->id);
		log_request(req, 200, 0);
		tcp_send_pmsg(req->conn, req->resp, -1);
	}

	return 0;
}

static int handle_SET_PARAMETER(struct req *req)
{
	// dbg("%s\n%s\n", __func__, req->conn->req_buf);

	rtsp_create_reply(req, 200, "OK");
	log_request(req, 200, 0);
	tcp_send_pmsg(req->conn, req->resp, -1);

	return 0;
}

static int handle_GET_PARAMETER(struct req *req)
{
	time_t timep;
	// dbg("%s\n%s\n", __func__, req->conn->req_buf);

	time(&timep);
	rtsp_create_reply(req, 200, "OK");
	//add_header( req->resp, "Content-Type", "application/x-rtsp-udp-packetpair;charset=UTF-8");
	add_header(req->resp, "Date", strtok(ctime(&timep), "\n"));
	log_request(req, 200, 0);
	tcp_send_pmsg(req->conn, req->resp, -1);
	return 0;

}
static int handle_PLAY(struct req *req)
{
	struct rtsp_session *rs;
	double start;
	int have_start = 0, i, p, ret;
	char *range, hdr[512];
	char path[256];
	gmss_sr *sr;

	// dbg("%s\n%s\n", __func__, req->conn->req_buf);

	if (rtsp_log_conn_stat_mode == -1) {
		log_connection_status();
	}

	if (!(rs = get_session(get_header(req->req, "Session")))) {
		rtsp_send_error(req, 454, "Session Not Found");
		return 0;
	}

	range = get_header(req->req, "Range");
	if (range && sscanf(range, "npt=%lf-", &start) == 1) {
#if (SUPPORT_PAUSE)     // Carl
		if (0.0f < start)
#endif
			have_start = 1; // Carl
		// dbg("%s: start at %f seconds\n", __func__, start);
		spook_log(SL_VERBOSE,
				  "starting streaming for session %s at position %.4f",
				  rs->id, start);
	} else spook_log(SL_VERBOSE,
						 "starting streaming for session %s", rs->id);

	if (!(sr = find_rtsp_location(req->req->sl.req.uri, path, NULL,
								  (req->conn->file) ? req->conn : NULL))) {
		rtsp_send_error(req, 404, "Not Found");
		return 0;
	}
	if (!sr->multicast.enabled) {
		rs->sess->play(rs->sess, have_start ? &start : NULL);        // live_play()
	}

	rtsp_create_reply(req, 200, "OK");
	add_header(req->resp, "Session", rs->id);


	// if( have_start && start >= 0 )
	{

		struct timeval tm;
		gettimeofday(&tm, NULL);

		spook_log(SL_DEBUG, "backend seeked to %.4f", start);
#if (SUPPORT_PAUSE)     // Carl
		if (have_start) {
			sprintf(hdr, "npt=%.3f-", start);
		} else
#endif
			sprintf(hdr, "npt=now-");
		add_header(req->resp, "Range", hdr);
		p = 0;
		for (i = 0; i < MAX_TRACKS; ++i) {
			if (! rs->sess->ep[i]) {
				continue;
			}

#if (SUPPORT_PAUSE)     // Carl
			if (have_start) {
				ret = snprintf(hdr + p, sizeof(hdr) - p,
							   "url=%s/trackID=%d;seq=%d,",
							   req->req->sl.req.uri, i,
							   rs->sess->ep[i]->seqnum);
			} else
#endif
			{
				ret = snprintf(hdr + p, sizeof(hdr) - p,
							   "url=%s/trackID=%d;seq=%d;rtptime=%u,",
							   req->req->sl.req.uri, i,
							   rs->sess->ep[i]->seqnum,
							   0);
			}

			if (ret >= sizeof(hdr) - p) {
				p = -1;
				break;
			}
			p += ret;

			rs->sess->ep[i]->enable_send_rtcp = 1;
		}
		if (p > 0) {
			hdr[p - 1] = 0; /* Kill last comma */
			if (!sr->multicast.enabled) {
				add_header(req->resp, "RTP-Info", hdr);
			}
		}
	}

	log_request(req, 200, 0);
	tcp_send_pmsg(req->conn, req->resp, -1);

	{
		// Carl
		char    *pn;
		int     live;

		i = 0;
		setsockopt(req->conn->fd, SOL_TCP, TCP_CORK, (void *) &i, sizeof(i));
		i = 1;
		setsockopt(req->conn->fd, SOL_TCP, TCP_NODELAY, (void *) &i, sizeof(i));

#if (LIVE_TRIG) // Carl
		pn = get_local_path(req->req->sl.req.uri);
		live = (uri_file(pn) == NULL);
#else
		pn = uri_file(get_local_path(req->req->sl.req.uri));
#endif
		if (pn) {
			// dbg("play the file '%s'\n", req->req->sl.req.uri );
			call_cmd_cb(pn, GM_STREAM_CMD_PLAY,
#if (LIVE_TRIG)
						(live) ? NULL : req->conn,
						((!have_start) || (live)) ? NULL : &start
#else
						req->conn, (have_start) ? &start : NULL
#endif
					   );
		}
	}

	return 0;
}

static int handle_PAUSE(struct req *req)
{
	struct rtsp_session *rs;

	// dbg("%s\n%s\n", __func__, req->conn->req_buf);

	if (!(rs = get_session(get_header(req->req, "Session")))) {
		rtsp_send_error(req, 454, "Session Not Found");
		return 0;
	}

#if (SUPPORT_PAUSE)
	{
		char                *pn;
		struct live_source  *ls = NULL;

		if ((rs->sess) && (rs->sess->private) &&
			(((struct live_session *)(rs->sess->private))->source)) {
			ls = ((struct live_session *)(rs->sess->private))->source;
		}
		if ((ls) && (ls->sess_list) && (ls->sess_list->next == NULL)) {
			//pn = uri_file(get_local_path(req->req->sl.req.uri));
			pn = get_local_path(req->req->sl.req.uri);
			//if (pn) {
			// dbg("pause the file '%s'\n", req->req->sl.req.uri );
			if (call_cmd_cb(pn, GM_STREAM_CMD_PAUSE, req->conn, NULL) < 0) {
				goto pause_not_file;
			}
			/*}
			else {
			    printf("%s dbg3\n", __FUNCTION__);
			    goto pause_not_file;
			}*/
		} else {
pause_not_file:
			rtsp_send_error(req, 501, "Not Implemented");
			rs->sess->teardown(rs->sess, NULL, 1);   // Carl
			return 0;
		}
	}
#else
	if (! rs->sess->pause) {
		rtsp_send_error(req, 501, "Not Implemented");
		rs->sess->teardown(rs->sess, NULL, 1);   // Carl
		return 0;
	}
#endif

	// rs->sess->pause( rs->sess );

	rtsp_create_reply(req, 200, "OK");
	add_header(req->resp, "Session", rs->id);
	log_request(req, 200, 0);
	tcp_send_pmsg(req->conn, req->resp, -1);

	return 0;
}

static int handle_TEARDOWN(struct req *req)
{
	gmss_sr *sr;
	struct rtsp_session *rs;
	int track;

	// dbg("%s\n%s\n", __func__, req->conn->req_buf);

	if (!(sr = find_rtsp_location(req->req->sl.req.uri, NULL, &track,
								  (req->conn->file) ? req->conn : NULL)) || track >= MAX_TRACKS) {
		rtsp_send_error(req, 404, "Not Found");
		return 0;
	}

	if (!(rs = get_session(get_header(req->req, "Session")))) {
		rtsp_send_error(req, 454, "Session Not Found");
		return 0;
	}

	spook_log(SL_VERBOSE, "tearing down %s in session %s",
			  req->req->sl.req.uri, rs->id);

	rtsp_create_reply(req, 200, "OK");
	add_header(req->resp, "Session", rs->id);


	/* This might destroy the session, so do it after creating the reply */

	log_request(req, 200, 0);
	tcp_send_pmsg(req->conn, req->resp, -1);

	// dbg("before teardown()\n");
	rs->sess->teardown(rs->sess, track < 0 ? NULL : rs->sess->ep[track], 1);     // Carl

	if (rtsp_log_conn_stat_mode == -1) {
		log_connection_status();
	}

	return 0;
}

static int handle_unknown(struct req *req)
{
	// dbg("%s\n%s\n", __func__, req->conn->req_buf);

	rtsp_send_error(req, 501, "Not Implemented");
	return 0;
}

int rtsp_handle_msg(struct req *req, struct conn *c)
{
	int ret;

	//printf("==> %s, line = %d >> method = %s\n", __FUNCTION__, __LINE__, req->req->sl.req.method);

	/*
	if(req->conn->req_buf)
	    printf("==> %s, line = %d >> req_buf = %s\n", __FUNCTION__, __LINE__, req->conn->req_buf);
	else
	    printf("==> %s, line = %d >> req_buf is NULL\n", __FUNCTION__, __LINE__);
	    */

	if (! strcasecmp(req->req->sl.req.method, "OPTIONS")) {
		ret = handle_OPTIONS(req);
	} else if (! strcasecmp(req->req->sl.req.method, "DESCRIBE")) {
		ret = handle_DESCRIBE(req);
	} else if (! strcasecmp(req->req->sl.req.method, "SETUP")) {
		ret = handle_SETUP(req);
	} else if (! strcasecmp(req->req->sl.req.method, "PLAY")) {
		ret = handle_PLAY(req);
	} else if (! strcasecmp(req->req->sl.req.method, "PAUSE")) {
		ret = handle_PAUSE(req);
	} else if (! strcasecmp(req->req->sl.req.method, "SET_PARAMETER")) {
		ret = handle_SET_PARAMETER(req);
	} else if (! strcasecmp(req->req->sl.req.method, "GET_PARAMETER")) {
		ret = handle_GET_PARAMETER(req);
	} else if (! strcasecmp(req->req->sl.req.method, "TEARDOWN")) {
		c->wait_teardown_reply = 5;
		ret = handle_TEARDOWN(req);
	} else {

		printf("==> %s, line = %d >> handle_unknown\n", __FUNCTION__, __LINE__);

		ret = handle_unknown(req);
	}
	return ret;
}

