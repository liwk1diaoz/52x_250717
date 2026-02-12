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

typedef int (*rtp_media_get_sdp_func)(char *dest, int len, int payload,
									  int port, void *d);
typedef int (*rtp_media_get_payload_func)(int payload, void *d);
typedef int (*rtp_media_frame_func)(struct frame *f, void *d);
typedef int (*rtp_media_send_func)(struct rtp_endpoint *ep, void *d,  unsigned int curr_time, unsigned int *delay);
typedef int (*rtp_media_release_type_func)(void *); // Carl

struct rtp_media {
	rtp_media_get_sdp_func get_sdp;
	rtp_media_get_payload_func get_payload;
	rtp_media_frame_func frame;
	rtp_media_send_func send;
	rtp_media_release_type_func release;
	void *private;
	unsigned int clock_rate_hz;
};

struct rtp_jpeg {
	int type;
	int q;
	int width;
	int height;
	int luma_table;
	int chroma_table;
	unsigned char *quant[16];
	unsigned char *scan_data;
	int scan_data_len;
	int init_done;
	int ts_incr;
	int framerate;
	int keyframe;   // 2007.07.17 TC.Kuo add for motion alarm I-Frame video
	// Keyframe is defined in the first frame per second
	unsigned int timestamp;
};

//#define IOVSIZE 64
//frame size 8M = 3840*2160 * 1.5
//iovsize = 3840*2160 * 1.5 / MTU(1444)
#define IOVSIZE 8620 //1024    //RTP link list

struct rtp_mpeg4 {

	struct iovec iov[IOVSIZE];
	int iov_count;
	unsigned char config[512];
	int config_len;
	int pali;
	char fmtp[600];
	int init_done;
	int ts_incr;
	int keyframe;
	int key_frame_offset;
	int vop_time_increment_resolution;
	int vop_time_increment;
	int vtir_bitlen;
	unsigned int timestamp;
};



struct mem_poll {
	unsigned char *base;
	unsigned int size;
	unsigned int pidx;
	unsigned int idx;
};

/*
  * h264 memory pool management
  * brief/
  *     pack NAL raw data with RTP syntax
  * input
  *
  * output
  */

struct rtp_h264 {
	struct iovec iov[IOVSIZE];
	int iov_count;
	unsigned char config[512];
	int config_len;
	unsigned char sps[256];
	int sps_len;
	unsigned char pps[128];
	int pps_len;
	int pali;
	char fmtp[600];
	int init_done;
	int ts_incr;
	int keyframe;
	int key_frame_offset;
	int vop_time_increment_resolution;
	int vop_time_increment;
	int vtir_bitlen;
	unsigned int timestamp;
	struct mem_poll mpoll;
};

struct rtp_h265 {
	struct iovec iov[IOVSIZE];
	int iov_count;
	unsigned char config[512];
	int config_len;
	unsigned char vps[256];
	int vps_len;
	unsigned char sps[256];
	int sps_len;
	unsigned char pps[128];
	int pps_len;
	int pali;
	char fmtp[600];
	int init_done;
	int ts_incr;
	int keyframe;
	int key_frame_offset;
	int vop_time_increment_resolution;
	int vop_time_increment;
	int vtir_bitlen;
	unsigned int timestamp;
	struct mem_poll mpoll;
};

struct rtp_rawaudio {
	unsigned char *rawaudio_data;
	int rawaudio_len;
	int format;
	int sampsize;
	int channels;
	int rate;
	int ts_incr;
	unsigned int timestamp;
};

struct rtp_media *new_rtp_media(rtp_media_get_sdp_func get_sdp,
								rtp_media_get_payload_func get_payload, rtp_media_frame_func frame,
								rtp_media_send_func send, rtp_media_release_type_func release, void *private,
								unsigned int clock_rate_hz);    // Carl
struct rtp_media *new_rtp_media_mpeg4(void);
struct rtp_media *new_rtp_media_h264(struct stream *stream);
struct rtp_media *new_rtp_media_h265(struct stream *stream);
struct rtp_media *new_rtp_media_mpv(void);
struct rtp_media *new_rtp_media_h263_stream(struct stream *stream);
struct rtp_media *new_rtp_media_jpeg_stream(struct stream *stream);
struct rtp_media *new_rtp_media_mpa(struct stream *stream);
struct rtp_media *new_rtp_media_rawaudio_stream(struct stream *stream);
struct rtp_media *new_rtp_media_g711a(struct stream *stream);
struct rtp_media *new_rtp_media_g711u(struct stream *stream);
struct rtp_media *new_rtp_media_adpcm(struct stream *stream);
struct rtp_media *new_rtp_media_amr(struct stream *stream);
struct rtp_media *new_rtp_media_aac(struct stream *stream);
struct rtp_media *new_rtp_media_g726(struct stream *stream);
