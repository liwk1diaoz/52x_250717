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

#ifndef __FRAME_H__
#define __FRAME_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 1   // Carl
#define FORMAT_EMPTY        0
#define FORMAT_H264         (GM_SS_TYPE_H264)
#define FORMAT_H265         (GM_SS_TYPE_H265)
#define FORMAT_MPEG4        (GM_SS_TYPE_MP4)
#define FORMAT_JPEG         (GM_SS_TYPE_MJPEG)
#define FORMAT_MPA          (GM_SS_TYPE_MP2)
#define FORMAT_AMR          (GM_SS_TYPE_AMR)
#define FORMAT_PCM          (GM_SS_TYPE_PCM)
#define FORMAT_G711         (GM_SS_TYPE_G711)
#define FORMAT_ALAW         201
#else
#define FORMAT_EMPTY        0
#define FORMAT_RAW_RGB24    1
#define FORMAT_RAW_UYVY     2
#define FORMAT_RAW_BGR24    3
#define FORMAT_PCM          64
#define FORMAT_MPEG4        100
#define FORMAT_JPEG         101
#define FORMAT_MPV          102
#define FORMAT_H263         103
#define FORMAT_H264         104
#define FORMAT_MPA          200
#define FORMAT_ALAW         201
#define FORMAT_AMR          301
#define FORMAT_ADPCM        302
#define FORMAT_MP2          303
#define FORMAT_MP3          304
#define FORMAT_G711         305
#define FORMAT_G723_1       306
#endif

struct frame;

typedef int (*frame_destructor)(struct frame *f, void *d);

struct frame {
	int ref_count;
//	int size;
	pthread_mutex_t mutex; /* only used to lock the ref_count */
//	frame_destructor destructor;
//	void *destructor_data;
	int format;
//	int width;
//	int height;
	int length;
//	int key;
//	int step;
//	int keyframe;
	unsigned char   *d;
	unsigned int    qno;    // Carl
	void            *ent;   // Carl, gm_ss_entity
};

void init_frame_heap(int size, int count);
int get_max_frame_size(void);
struct frame *new_frame(int size);
struct frame *clone_frame(struct frame *orig);
void ref_frame(struct frame *f);
void unref_frame(struct frame *f);


struct frame_slot {
	struct frame_slot *next;
	struct frame_slot *prev;
	int pending;
	struct frame *f;
};

typedef void (*frame_deliver_func)(struct frame *f, void *d);

struct frame_exchanger {
	int master_fd;
	int slave_fd;
	pthread_mutex_t mutex;
	pthread_cond_t slave_wait;
	struct event *master_event;
	frame_deliver_func f;
	void *d;
	struct frame_slot *slave_cur;
	struct frame_slot *master_read;
	struct frame_slot *master_write;
};

struct frame_exchanger *new_exchanger(int slots,
									  frame_deliver_func func, void *d);

/* master functions */
int exchange_frame(struct frame_exchanger *ex, struct frame *frame);

/* slave functions */
struct frame *get_next_frame(struct frame_exchanger *ex, int wait);
void deliver_frame(struct frame_exchanger *ex, struct frame *f);

#ifdef __cplusplus
}
#endif

#endif  /* __FRAME_H__ */
