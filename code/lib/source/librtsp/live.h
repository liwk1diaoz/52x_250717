#ifndef __LIVE_H__
#define __LIVE_H__

struct live_source;

struct live_session {
	struct live_session *next;
	struct live_session *prev;
	struct session *sess;
	struct live_source *source;
	int playing;
	int delay;
};

struct live_track {
	int index;
	struct live_source *source;
	struct stream_destination *stream;
	int ready;
	struct rtp_media *rtp;
};

struct live_source {
	struct live_session *sess_list;
	struct live_track track[MAX_TRACKS];
	int qos;
	int safe_I_num;
	int curr_I_num;
	int I_frame_only;

	pthread_t live_thread;
	void    *gmss_sr;   // Carl
};

typedef struct msg_st {
	long int msg_type;
	char buf[64];
} MESSAGE;

#endif
