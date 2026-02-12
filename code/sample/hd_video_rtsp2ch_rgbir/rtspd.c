#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <dirent.h>

//#include "platform_API_define.h"
//#include "audio_API_define.h"
#include "rtspd.h"
#include "librtsp.h"
//#include "debug_message.h"

#define DVR_ENC_EBST_ENABLE     0x55887799
#define DVR_ENC_EBST_DISABLE    0

#define MAX_ENC_SUB_NUM         2
#define MAX_ENC_CH_NUM          2
#define RTSP_BS_NUM          	MAX_ENC_SUB_NUM
#define RTSP_CH_NUM				MAX_ENC_CH_NUM

#define SDPSTR_MAX      512
#define SR_MAX          128
#define VQ_MAX          (SR_MAX)
#define VQ_LEN          5
#define AQ_MAX          64            /* 1 MP2 and 1 AMR for live streaming, another 2 for file streaming. */
#define AQ_LEN          2            /* 1 MP2 and 1 AMR for live streaming, another 2 for file streaming. */
#define AV_NAME_MAX     127

#define VIDEO_RTP_HZ    90000
#define AUDIO_RTP_HZ    8000

#define ERR_GOTO(x, y)      do { ret = x; goto y; } while(0)
#define MUTEX_FAILED(x)     (x == ERR_MUTEX)
#define VIDEO_FRAME_NUMBER VQ_LEN+1

#define NONE_BS_EVENT    0
#define START_BS_EVENT   1
#define STOP_BS_EVENT    2

typedef int (*open_container_fn)(int ch_num, int bs_num);
typedef int (*close_container_fn)(int ch_num, int bs_num);

typedef enum st_opt_type {
	OPT_NONE = 0,
	RTSP_LIVE_STREAMING,
} opt_type_t;

typedef struct st_vbs {
	int enabled; //DVR_ENC_EBST_ENABLE: enabled, DVR_ENC_EBST_DISABLE: disabled
	VIDEO_CODEC_TYPE enc_type;    // 0:ENC_TYPE_H264, 1:ENC_TYPE_MPEG4, 2:ENC_TYPE_MJPEG
} vbs_t;

typedef struct st_priv_vbs {
	char sdpstr[SDPSTR_MAX];
	int qno;
	int offs;
	int len;
	unsigned int tv_ms;
	int cap_ch;
	int cap_path;
	char *bs_buf;
	unsigned int bs_buf_len;
	pthread_mutex_t priv_vbs_mutex;
} priv_vbs_t;

typedef struct st_abs {
	//int reserved;
	int enabled;  	// DVR_ENC_EBST_ENABLE: enabled, DVR_ENC_EBST_DISABLE: disabled
	int enc_type;	// GM_PCM, GM_AAC, GM_G711_ALAW, GM_G711_ULAW
	int is_format_change;
} abs_t;

typedef struct st_priv_abs {
	//int reserved;
    char sdpstr[SDPSTR_MAX];
    int qno;
    int offs;
    int len;
	unsigned int tv_ms;
} priv_abs_t;

typedef struct st_bs {
	int event; // config change please set 1 for enqueue_thread to config this
	int enabled; //DVR_ENC_EBST_ENABLE: enabled, DVR_ENC_EBST_DISABLE: disabled
	opt_type_t opt_type;  /* 1:rtsp_live_streaming, 2: file_avi_recording 3:file_h264_recording */
	vbs_t video;  /* VIDEO, 0: main-bitstream, 1: sub1-bitstream, 2:sub2-bitstream */
	abs_t audio;  /* AUDIO, 0: main-bitstream, 1: sub1-bitstream, 2:sub2-bitstream */
} avbs_t;

typedef struct st_priv_bs {
	int play;
	int congest;
	int sr;
	char name[AV_NAME_MAX];
	open_container_fn open;
	close_container_fn close;
	priv_vbs_t video;  /* VIDEO, 0: main-bitstream, 1: sub1-bitstream, 2:sub2-bitstream */
	priv_abs_t audio;  /* AUDIO, 0: main-bitstream, 1: sub1-bitstream, 2:sub2-bitstream */
} priv_avbs_t;

typedef struct st_av {
	/* public data */
	avbs_t bs[RTSP_CH_NUM][RTSP_BS_NUM];  /* VIDEO, 0: main-bitstream, 1: sub1-bitstream, 2:sub2-bitstream */
	/* update date */
	pthread_mutex_t ubs_mutex;

	/* private data */
	int enabled;      //DVR_ENC_EBST_ENABLE: enabled, DVR_ENC_EBST_DISABLE: disabled
	priv_avbs_t priv_bs[RTSP_CH_NUM][RTSP_BS_NUM];
} av_t;

pthread_t enqueue_thread_id = 0;

unsigned int sys_tick = 0;
struct timeval sys_sec = { -1, -1};
int sys_port = 554;
char *ipptr = NULL;
static int rtspd_sysinit = 0;
static int rtspd_set_event = 0;
static int rtspd_bs_num = RTSP_BS_NUM;
static int rtspd_ch_num = RTSP_CH_NUM;
static int first_play[RTSP_CH_NUM][RTSP_BS_NUM];

int pthread_setname_np(pthread_t thread, const char *name);
static int write_rtp_frame_ext(int ch_num, int bs_num, int is_video, void *data, int data_len, unsigned int tv_ms);
static int set_poll_event(void);
static char *get_local_ip(char *if_name);
static void ConvertIntelPCMtoMotorolaPCM(char * dest, char * src, int pcm_len);

pthread_mutex_t stream_queue_mutex;
av_t encode_av;

static char *rtsp_enc_type_str[] = {
	"INVALID",
	"H264",
	"MPEG4",
	"MJPEG",
	"H265",
};

static char *rtsp_audio_enc_type_str[] = {
	"PCM",
	"AAC",
	"G711_ALAW",
	"G711_ULAW"
};

RTSP_SERVER_STATE g_rtsp_server_state;	//Elvis - 20190412

static int do_queue_alloc(int type)
{
	int    rc;
	do {
		rc = stream_queue_alloc(type);
	} while MUTEX_FAILED(rc);

	return rc;
}

static unsigned int get_tick_gm(unsigned int tv_ms)
{
	sys_tick = tv_ms * (VIDEO_RTP_HZ / 1000);
	return sys_tick;
}

static unsigned int get_audio_tick_gm(unsigned int tv_ms)
{
	return (tv_ms * (AUDIO_RTP_HZ / 1000));
}

static int convert_to_rtsp_video_codec(VIDEO_CODEC_TYPE enc_type)
{
	int media_type;

	switch (enc_type) {
	case VIDEO_CODEC_TYPE_H264:
		media_type = GM_SS_TYPE_H264;
		break;
	case VIDEO_CODEC_TYPE_H265:
		media_type = GM_SS_TYPE_H265;
		break;
	case VIDEO_CODEC_TYPE_MPEG:
		media_type = GM_SS_TYPE_MP4;
		break;
	case VIDEO_CODEC_TYPE_JPEG:
	case VIDEO_CODEC_TYPE_MJPEG:
		media_type = GM_SS_TYPE_MJPEG;
		break;
	default:
		media_type  = -1;
		fprintf(stderr, "convert_to_rtsp_video_codec: type=%d, error!\n", enc_type);
		break;
	}
	return media_type;
}

static int convert_to_rtsp_audio_codec(int type)
{
    switch (type)
    {
        case AUDIO_CODEC_TYPE_PCM:
            return GM_SS_TYPE_PCM;    //12;
        case AUDIO_CODEC_TYPE_AAC:
            return GM_SS_TYPE_AAC;    //16;
        case AUDIO_CODEC_TYPE_ALAW:
            return GM_SS_TYPE_G711A ; //13;
        case AUDIO_CODEC_TYPE_ULAW:
            return GM_SS_TYPE_G711U;  //14;
        default:
			fprintf(stderr, "convert_to_rtsp_audio_codec: type=%d, error!\n", type);
            return GM_SS_TYPE_PCM;    //12;
     }
}

static int open_live_streaming(int ch_num, int bs_num)
{
	int video_type = 0, audio_type = 0;
	avbs_t *b;
	priv_avbs_t *pb;
	char livename[64];

	b = &encode_av.bs[ch_num][bs_num];
	pb = &encode_av.priv_bs[ch_num][bs_num];
	video_type = convert_to_rtsp_video_codec(b->video.enc_type);
	pb->video.qno = do_queue_alloc(video_type);

	audio_type = convert_to_rtsp_audio_codec(b->audio.enc_type);
	pb->audio.qno = do_queue_alloc(audio_type);

//	QAS_MSG("[Ch%d_%d] open_live_streaming, video type[%s=%d] audio type[%s=%d]" ,
//			ch_num,	bs_num, strVideoCodecType[b->video.enc_type], video_type,
//			strAudioCodecType[b->audio.enc_type], audio_type);

	sprintf(livename, "live/ch%02d_%d", ch_num, bs_num);

	pb->sr = stream_reg(livename, pb->video.qno, pb->video.sdpstr,
						pb->audio.qno , pb->audio.sdpstr,
						1, 0, 0, 0, 0, 0, 0);

	if (pb->sr < 0) {
		fprintf(stderr, "open_live_streaming: ch_num=%d bs_num=%d, setup error\n", ch_num, bs_num);
	}
	strcpy(pb->name, livename);
	return 0;
}

#define TIMEVAL_DIFF(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

int write_rtp_frame_ext_audio(int ch_num, int au_enc_type , void *data, int data_len , unsigned int tv_ms)
{
	priv_avbs_t *pb;
	int bs_num;

	static unsigned int AUDIO_BITSTREAM_LEN = 12800;
	unsigned char audio_buf[AUDIO_BITSTREAM_LEN];

	if (rtspd_sysinit == 0) {
		fprintf(stderr , "rtspd_sysinit = 0 , %s fail!\n" , __FUNCTION__);
		return -1;
	}


	for(bs_num=0 ; bs_num<rtspd_bs_num ; bs_num++){
		/* audio sdp */
		if (encode_av.bs[ch_num][bs_num].audio.enc_type == AUDIO_CODEC_TYPE_PCM) {
			ConvertIntelPCMtoMotorolaPCM((char *)&(audio_buf[0]), data , data_len);
		} else if (encode_av.bs[ch_num][bs_num].audio.enc_type == AUDIO_CODEC_TYPE_AAC) {
			memcpy(&(audio_buf[0]), data , data_len);
			audio_buf[3] = 0x00;
			audio_buf[4] = 0x10;
			audio_buf[5] = (char) ((data_len - 7) >> 5);
			audio_buf[6] = (char) ((data_len - 7) << 3);
		}

		pb = &encode_av.priv_bs[ch_num][bs_num];

		switch (encode_av.bs[ch_num][bs_num].audio.enc_type) {
			case AUDIO_CODEC_TYPE_PCM:
				if (pb->audio.sdpstr[0] == 0) {
					snprintf(pb->audio.sdpstr, SDPSTR_MAX, "L16/8000");
				}
				pb->audio.offs = (int) &audio_buf[0];
				pb->audio.len = data_len;
				pb->audio.tv_ms = tv_ms;
				break;

			case AUDIO_CODEC_TYPE_AAC:
				if (pb->audio.sdpstr[0] == 0) {
					snprintf(pb->audio.sdpstr, SDPSTR_MAX, "%X", ((((audio_buf[2] & 0xC0) >> 6) + 1)<<11)+
							(((audio_buf[2] & 0x3C) >> 2)<<7)+
							((((audio_buf[2] & 0x01) << 2) +
							((audio_buf[3] & 0xC0) >> 6))<<3));
				}
				pb->audio.offs = (int) &audio_buf[3];
				pb->audio.len = data_len - 3;
				pb->audio.tv_ms = tv_ms;
				break;
			case AUDIO_CODEC_TYPE_ALAW:
			case AUDIO_CODEC_TYPE_ULAW:
				pb->audio.offs = (int) data;
				pb->audio.len = data_len;
				pb->audio.tv_ms = tv_ms;
				break;
			default:
				fprintf(stderr, "ch_num=%d bs_num=%d, enc_type(%d) not support!\n", ch_num, bs_num, encode_av.bs[ch_num][bs_num].audio.enc_type);
				break;
		}

		if(encode_av.bs[ch_num][bs_num].audio.enc_type != au_enc_type){
			printf("[rtspd]:%s Audio format change to %s(ch%d bs%d)\n" , __FUNCTION__ , rtsp_audio_enc_type_str[au_enc_type] , ch_num, bs_num);
			encode_av.bs[ch_num][bs_num].audio.enc_type = au_enc_type;
			encode_av.bs[ch_num][bs_num].audio.is_format_change = 1;
		}

		if (write_rtp_frame_ext(ch_num , bs_num, 0, (void *)pb->audio.offs, pb->audio.len, pb->audio.tv_ms) == 1) {
			pb->audio.offs = (int)NULL;
			pb->audio.len = 0;
		}

		//if write_rtp_frame_ext success , two parameters get cleaned
		//or wait until callback function clean it.
		while (pb->audio.offs || pb->audio.len) {
			usleep(1000*5);
			continue;
		}

	}

	return 0;
}

int write_rtp_frame_ext_video(int ch_num, int bs_num, void *data, int data_len , int is_keyframe , unsigned int tv_ms)
{
	priv_avbs_t *pb;
	avbs_t *avbs;

	avbs = &encode_av.bs[ch_num][bs_num];
	pb = &encode_av.priv_bs[ch_num][bs_num];
	pb->video.bs_buf_len = data_len;
	pb->video.cap_ch = ch_num;
	pb->video.cap_path = 0;
	pb->video.bs_buf = (char *)data;

	if (rtspd_sysinit == 0) {
		fprintf(stderr , "rtspd_sysinit = 0 , write_rtp_frame_ext_video fail!\n");
		return -1;
	}

	while(rtspd_set_event) {
//		printf("usleep(2000)\n");
		usleep(2000);
	}
	if (rtspd_sysinit == 0) {
		fprintf(stderr , "rtspd_sysinit = 0 , write_rtp_frame_ext_video fail!\n");
		return -1;
	}

	while(set_poll_event() < 0) {
//		printf("usleep(2000)\n");
		usleep(2000);
	}

	pb = &encode_av.priv_bs[ch_num][bs_num];
	while (pb->video.offs || pb->video.len) {
		usleep(1000*5);
		continue;
	}

	if (pb->play == 0) {
		first_play[ch_num][bs_num] = -1;
	}


	if (rtspd_sysinit == 0) {
		fprintf(stderr , "rtspd_sysinit = 0 , write_rtp_frame_ext_video fail!\n");
		return -1;
	}

	pb = &encode_av.priv_bs[ch_num][bs_num];
	avbs = &encode_av.bs[ch_num][bs_num];

	if (rtspd_sysinit == 0) {
		fprintf(stderr , "rtspd_sysinit = 0 , write_rtp_frame_ext_video fail!\n");
		return -1;
	}

	pb = &encode_av.priv_bs[ch_num][bs_num];
	avbs = &encode_av.bs[ch_num][bs_num];


	if ((avbs->video.enc_type != VIDEO_CODEC_TYPE_JPEG)
			&& (avbs->video.enc_type != VIDEO_CODEC_TYPE_MJPEG)){
		if (pb->play == 1 && is_keyframe == 1)
			first_play[ch_num][bs_num] = 1;
	} else {
		first_play[ch_num][bs_num] = 1;
	}

	if (first_play[ch_num][bs_num] == 1) {
		pthread_mutex_lock(&pb->video.priv_vbs_mutex);
		pb->video.offs = (int)(pb->video.bs_buf);
		pb->video.len = data_len;
		pb->video.tv_ms = tv_ms;
		pthread_mutex_unlock(&pb->video.priv_vbs_mutex);
		if (write_rtp_frame_ext(ch_num, bs_num, 1, (void *)pb->video.offs , pb->video.len, pb->video.tv_ms) == 1) {
			pb->video.offs = (int)NULL;
			pb->video.len = 0;
		}
	}

	//if write_rtp_frame_ext success , two parameters get cleaned
	//or wait until callback function clean it.
	while (pb->video.offs || pb->video.len) {
		usleep(1000*5);
		continue;
	}

	return 0;
}

static int write_rtp_frame_ext(int ch_num, int bs_num, int is_video, void *data, int data_len, unsigned int tv_ms)
{
	int ret, media_type;
	avbs_t *b;
	priv_avbs_t *pb;
	gm_ss_entity entity;
	struct timeval curr_tval;
	static struct timeval err_print_tval;

	pb = &encode_av.priv_bs[ch_num][bs_num];
	b = &encode_av.bs[ch_num][bs_num];

	if ((pb->play == 0) || (b->event != NONE_BS_EVENT)) {
		ret = 1;
		goto exit_free_as_buf;
	}

	entity.data = (char *) data;
	entity.size = data_len;
	if (is_video) {
		entity.timestamp = get_tick_gm(tv_ms);
		media_type = convert_to_rtsp_video_codec(b->video.enc_type);
	} else {
		entity.timestamp = get_audio_tick_gm(tv_ms);
		media_type = convert_to_rtsp_audio_codec(b->audio.enc_type);
	}

	pthread_mutex_lock(&stream_queue_mutex);
	if (is_video) {
		ret = stream_media_enqueue(media_type, pb->video.qno, &entity);
	} else {
		ret = stream_media_enqueue(media_type, pb->audio.qno, &entity);
	}
	pthread_mutex_unlock(&stream_queue_mutex);


	if (ret < 0) {
		gettimeofday(&curr_tval, NULL);
		if (ret == ERR_FULL) {
			pb->congest = 1;
			if (TIMEVAL_DIFF(err_print_tval, curr_tval) > 5000000) {
				fprintf(stderr, "ext enqueue queue ch_num=%d bs_num=%d, full\n", ch_num, bs_num);
			}
		} else if ((ret != ERR_NOTINIT) && (ret != ERR_MUTEX) && (ret != ERR_NOTRUN)) {
			if (TIMEVAL_DIFF(err_print_tval, curr_tval) > 5000000) {
				fprintf(stderr, "ext enqueue queue ch_num=%d bs_num=%d, error %d\n", ch_num, bs_num, ret);
			}
		}
		if (TIMEVAL_DIFF(err_print_tval, curr_tval) > 5000000) {
			fprintf(stderr, "ext enqueue queue ch_num=%d bs_num=%d, error %d\n", ch_num, bs_num, ret);
			gettimeofday(&err_print_tval, NULL);
		}
		goto exit_free_audio_buf;
	}
	return 0;

exit_free_audio_buf:
exit_free_as_buf:
	return 1;
}

static int close_live_streaming(int ch_num, int bs_num)
{
	priv_avbs_t *pb;
	int ret = 0;

	pb = &encode_av.priv_bs[ch_num][bs_num];
	if (pb->sr >= 0) {

		if(pb->audio.qno != -1)
			stream_queue_release(pb->audio.qno);

		if(pb->video.qno != -1)
			stream_queue_release(pb->video.qno);

		ret = stream_dereg(pb->sr, 1);
		if (ret < 0) {
			goto err_exit;
		}
		pb->sr = -1;
		pb->video.qno = -1;
		pb->audio.qno = -1;
		memset(pb->audio.sdpstr , 0 , SDPSTR_MAX);
		pb->play = 0;
	}

err_exit:
	if (ret < 0) {
		fprintf(stderr, "%s: stream_dereg(%d) err %d\n", __func__, pb->sr, ret);
	}
	return ret;
}

int open_bs(int ch_num, int bs_num)
{
	avbs_t *b;
	priv_avbs_t *pb;

	pb = &encode_av.priv_bs[ch_num][bs_num];
	b = &encode_av.bs[ch_num][bs_num];

	encode_av.enabled = DVR_ENC_EBST_ENABLE;
	encode_av.bs[ch_num][bs_num].enabled = DVR_ENC_EBST_ENABLE;
	encode_av.bs[ch_num][bs_num].video.enabled = DVR_ENC_EBST_ENABLE;

	switch (b->opt_type) {
	case RTSP_LIVE_STREAMING:
		pb->open = open_live_streaming;
		pb->close = close_live_streaming;
		break;
	case OPT_NONE:
	default:
		break;
	}
	return 0;
}

int close_bs(int ch_num, int bs_num)
{
	av_t *e;
	int ch, sub, is_close_channel = 1;
	int break_loop = 0;

	e = &encode_av;

	e->bs[ch_num][bs_num].video.enabled = DVR_ENC_EBST_DISABLE;
	e->bs[ch_num][bs_num].enabled = DVR_ENC_EBST_DISABLE;
	for (ch = 0; ch < rtspd_ch_num; ch++){
		for (sub = 0; sub < rtspd_bs_num; sub++) {
			if (e->bs[ch][sub].video.enabled == DVR_ENC_EBST_ENABLE) {
				is_close_channel = 0;
				break_loop = 1;
				break;
			}
		}

		if(break_loop)
			break;
	}
	if (is_close_channel == 1) {
		encode_av.enabled = DVR_ENC_EBST_DISABLE;
	}
	return 0;
}

static int bs_check_event(void)
{
	int ch_num, bs_num, ret = 0;
	avbs_t *b;

	for (ch_num = 0; ch_num < rtspd_ch_num; ch_num++) {
		for (bs_num = 0; bs_num < rtspd_bs_num; bs_num++) {
			b = &encode_av.bs[ch_num][bs_num];
			if (b->event != NONE_BS_EVENT) {
				ret = 1;
				break;
			}
		}

		if(ret == 1)
			break;
	}
	return ret;
}

void bs_new_event(void)
{
	int ch_num, bs_num;
	avbs_t *b;
	priv_avbs_t *pb;

	if (bs_check_event() == 0) {
		rtspd_set_event = 0;
		return;
	}

	for (ch_num = 0; ch_num < rtspd_ch_num; ch_num++) {
		for (bs_num = 0; bs_num < rtspd_bs_num; bs_num++) {
			pthread_mutex_lock(&encode_av.ubs_mutex);
			b = &encode_av.bs[ch_num][bs_num];
			pb = &encode_av.priv_bs[ch_num][bs_num];
			switch (b->event) {
			case START_BS_EVENT:
				open_bs(ch_num, bs_num);
				if (pb->open) {
					pb->open(ch_num, bs_num);
				}
				b->event = NONE_BS_EVENT;
				break;
			case STOP_BS_EVENT:
				pb->open = NULL;
				if (pb->close) { /* for recording */
					pb->close(ch_num, bs_num);
					pb->close = NULL;
					close_bs(ch_num, bs_num);
				}
				b->event = NONE_BS_EVENT;
				break;
			default:
				break;
			}
			pthread_mutex_unlock(&encode_av.ubs_mutex);
		}
	}
}

int env_set_bs_new_event(int ch_num, int bs_num, int event)
{
	avbs_t *b;
	int ret = 0;

	b = &encode_av.bs[ch_num][bs_num];

	switch (event) {
	case START_BS_EVENT:
		if (b->opt_type == OPT_NONE) {
			goto err_exit;
		}
		if (b->enabled == DVR_ENC_EBST_ENABLE) {
			fprintf(stderr, "Already enabled: ch_num=%d bs_num=%d, \n", ch_num, bs_num);
			ret = -1;
			goto err_exit;
		}
		break;
	case STOP_BS_EVENT:
		if (b->enabled != DVR_ENC_EBST_ENABLE) {
			fprintf(stderr, "Already disabled: ch_num=%d bs_num=%d \n", ch_num, bs_num);
			ret = -1;
			goto err_exit;
		}
		break;
	default:
		fprintf(stderr, "env_set_bs_new_event: ch_num=%d, bs_num=%d, event=%d, error\n",
				ch_num, bs_num, event);
		ret = -1;
		goto err_exit;
	}
	b->event = event;
	rtspd_set_event = 1;

err_exit:
	return ret;
}

static int set_poll_event(void)
{
	int ch_num, bs_num, ret = -1;
	av_t *e;
	avbs_t *b;

	for (ch_num = 0; ch_num < rtspd_ch_num; ch_num++) {
		for (bs_num = 0; bs_num < rtspd_bs_num; bs_num++) {
			e = &encode_av;
			if (e->enabled != DVR_ENC_EBST_ENABLE) {
				continue;
			}
			b = &e->bs[ch_num][bs_num];
			if (b->video.enabled == DVR_ENC_EBST_ENABLE) {
				ret = 0;
			}
		}
	}
	return ret;
}


static void env_release_resources(void)
{
	int ret;//, ch_num;
	av_t *e;

	if ((ret = stream_server_stop())) {
		fprintf(stderr, "stream_server_stop() error %d\n", ret);
	}
//	for (ch_num = 0; ch_num < rtspd_ch_num; ch_num++) {
//		for (bs_num = 0; bs_num < rtspd_bs_num; bs_num++) {
			e = &encode_av;
			pthread_mutex_destroy(&e->ubs_mutex);
//		}
//	}
}

static int frm_cb(int type, int qno, gm_ss_entity *entity)
{
	priv_avbs_t *pb;
	int ch_num, bs_num;
	for (ch_num = 0; ch_num < rtspd_ch_num; ch_num++) {
		for (bs_num = 0; bs_num < rtspd_bs_num; bs_num++) {
			pb = &encode_av.priv_bs[ch_num][bs_num];
			if (pb->video.offs == (int)(entity->data)
				&& pb->video.len == entity->size && pb->video.qno == qno) {
				pthread_mutex_lock(&pb->video.priv_vbs_mutex);
				pb->video.offs = 0;
				pb->video.len = 0;
				pthread_mutex_unlock(&pb->video.priv_vbs_mutex);
			}

			if (pb->audio.offs == (int)(entity->data)
				&& pb->audio.len == entity->size && pb->audio.qno == qno) {
				pthread_mutex_lock(&pb->video.priv_vbs_mutex);
				pb->audio.offs = 0;
				pb->audio.len = 0;
				pthread_mutex_unlock(&pb->video.priv_vbs_mutex);
			}
		}
	}
	return 0;
}

priv_avbs_t *find_file_sr(char *name, int srno)
{
	int ch_num, bs_num, hit = 0;
	priv_avbs_t *pb;

	for (ch_num = 0; ch_num < rtspd_ch_num; ch_num++) {
		for (bs_num = 0; bs_num < rtspd_bs_num; bs_num++) {
			pb = &encode_av.priv_bs[ch_num][bs_num];
			if ((pb->sr == srno) && (pb->name) && (strcmp(pb->name, name) == 0)) {
				hit = 1;
				break;
			}
		}

		if(hit == 1)
			break;
	}
	return (hit ? pb : NULL);
}

static int cmd_cb(char *name, int sno, int cmd, void *p)
{
	int ret = -1;
	priv_avbs_t *pb;

	switch (cmd) {
	case GM_STREAM_CMD_OPTION:
		ret = 0;
		break;
	case GM_STREAM_CMD_DESCRIBE:
		ret = 0;
		break;
	case GM_STREAM_CMD_OPEN:
		printf("%s:%d <GM_STREAM_CMD_OPEN>\n", __FUNCTION__, __LINE__);
		ERR_GOTO(-10, cmd_cb_err);
		break;
	case GM_STREAM_CMD_SETUP:
		ret = 0;
		break;
	case GM_STREAM_CMD_PLAY:
		if (strncmp(name, "live/", 5) == 0) {
			if ((pb = find_file_sr(name, sno)) == NULL) {
				ERR_GOTO(-1, cmd_cb_err);
			}

			if (pb->video.qno >= 0) {
				printf("play %s\n", name);
				pb->play = 1;
			}
		}
		ret = 0;
		break;
	case GM_STREAM_CMD_PAUSE:
		printf("%s:%d <GM_STREAM_CMD_PAUSE>\n", __FUNCTION__, __LINE__);
		ret = 0;
		break;
	case GM_STREAM_CMD_TEARDOWN:
		if (strncmp(name, "live/", 5) == 0) {
			if ((pb = find_file_sr(name, sno)) == NULL) {
				ERR_GOTO(-1, cmd_cb_err);
			}
			printf("teardown %s\n", name);
			pb->play = 0;
		}
		ret = 0;
		break;
	default:
		fprintf(stderr, "%s: not support cmd %d\n", __func__, cmd);
		break;
	}

cmd_cb_err:
	if (ret < 0) {
		fprintf(stderr, "%s: cmd %d error %d\n", __func__, cmd, ret);
	}
	return ret;
}

void *enqueue_thread(void *ptr)
{
//	g_rtsp_server_state = RTSP_SERVER_STATE_RUNNING;	//Elvis - 20190702
	while (rtspd_sysinit) {
		if (rtspd_set_event) {
			bs_new_event();
			g_rtsp_server_state = RTSP_SERVER_STATE_RUNNING;	//Elvis - 20190725
		}
		if (set_poll_event() < 0) {
			sleep(1);
			continue;
		}
		usleep(1000);
	}
	env_release_resources();
	g_rtsp_server_state = RTSP_SERVER_STATE_IDLE;	//Elvis - 20190412
//	pthread_exit(NULL);
	return NULL;
}

int env_init(VIDEO_CODEC_TYPE enc_type)
{
	int ret = 0;
	int ch_num, bs_num;
	av_t *e;
	avbs_t *b;
	priv_avbs_t *pb;

	memset(&encode_av, 0, sizeof(av_t));
	memset(first_play , 0 , sizeof(first_play[0][0])*RTSP_CH_NUM*RTSP_BS_NUM);

	/* private data initial */
	for (ch_num = 0; ch_num < rtspd_ch_num; ch_num++) {
		for (bs_num = 0; bs_num < rtspd_bs_num; bs_num++) {
			e = &encode_av;
			if (pthread_mutex_init(&e->ubs_mutex, NULL)) {
				perror("env_init: mutex init failed:");
				exit(-1);
			}
			b = &e->bs[ch_num][bs_num];
			b->opt_type = RTSP_LIVE_STREAMING;
			b->video.enc_type = enc_type;
			b->event = NONE_BS_EVENT;
			b->enabled = DVR_ENC_EBST_DISABLE;
			b->video.enabled = DVR_ENC_EBST_DISABLE;
			b->audio.enc_type = AUDIO_CODEC_TYPE_PCM;

			pb = &e->priv_bs[ch_num][bs_num];
			pb->video.qno = -1;
			pb->video.offs = 0;
			pb->video.len = 0;
			pb->audio.qno = -1;
			pb->audio.offs = 0;
			pb->audio.len = 0;
			pb->sr = -1;
			pb->video.bs_buf = NULL;
			if (pthread_mutex_init(&pb->video.priv_vbs_mutex, NULL)) {
				perror("env_enc_init: priv_vbs mutex init failed:");
				exit(-1);
			}
		}
	}

	srand((unsigned int)time(NULL));
	if ((ret = stream_server_init(ipptr, (int) sys_port, 0, 1444, 256, SR_MAX, VQ_MAX, VQ_LEN,
								  AQ_MAX, AQ_LEN, frm_cb, cmd_cb)) < 0) {
		fprintf(stderr, "stream_server_init, ret %d\n", ret);
	}
	if ((ret = stream_server_start()) < 0) {
		fprintf(stderr, "stream_server_start, ret %d\n", ret);
	}
	return ret;
}


#define MAX_AUDIO_BITSTREAM_NUM   1


static void ConvertIntelPCMtoMotorolaPCM(char * dest, char * src, int pcm_len)
{
    int i;

    //printf ("ConvertIntelPCMtoMotorolaPCM pcm_len=%d\n", pcm_len);

    for (i=0; i<pcm_len; i+=2)
    {
        /*int Low8 = (Src&0xFF)<<8;
        int High8 = (Src&0xFF00)>>8;
        int New16 = Low8 | High8 ;
        *Dst = New16;
        return 1;*/
        dest[i] = src[i+1];
        dest[i+1] = src[i];
    }
}

int rtspd_stop()
{
	pthread_mutex_destroy(&stream_queue_mutex);
	rtspd_sysinit = 0;
	enqueue_thread_id = 0;
	g_rtsp_server_state = RTSP_SERVER_STATE_UNINIT;	//Elvis - 20190412
	return 0;
}

int rtspd_start(int max_ch_cnt, int max_bs_cnt, VIDEO_CODEC_TYPE enc_type)
{
	int ret, ch_num, bs_num;
	pthread_attr_t attr;
	char tmp[32] = {'\0'};

	if (rtspd_sysinit == 1) {
		printf("rtspd server is already started , skip\n");
		return 0;
	}

	rtspd_bs_num = max_bs_cnt;
	rtspd_ch_num = max_ch_cnt;

//	if ((0 < port) && (port < 0x10000)) {
//		sys_port = port;
//	}

	if ((ret = env_init(enc_type)) < 0) {
		fprintf(stderr , "rtspd_start fail!\n");
		return ret;
	}

	if (pthread_mutex_init(&stream_queue_mutex, NULL)) {
		perror("rtspd_start: mutex init failed:");
		return -1;
	}

	rtspd_sysinit = 1;
	g_rtsp_server_state = RTSP_SERVER_STATE_INIT;	//Elvis - 20190412

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_destroy(&attr);
	ret = pthread_create(&enqueue_thread_id, &attr, &enqueue_thread, NULL);

	sprintf(tmp, "rtsp_enqueue");
	DBG_MSG("pthread_setname_np(%s)", tmp);
	ret = pthread_setname_np(enqueue_thread_id, tmp);
	if (ret != 0)
		ERR_MSG("pthread_setname_np(%s) fail!",tmp);

	printf("Connect command:\n");
	for (ch_num = 0; ch_num < rtspd_ch_num; ch_num++) {
		for (bs_num = 0; bs_num < rtspd_bs_num; bs_num++) {
			pthread_mutex_lock(&encode_av.ubs_mutex);
			env_set_bs_new_event(ch_num, bs_num, START_BS_EVENT);
			pthread_mutex_unlock(&encode_av.ubs_mutex);
			printf("LAN:    rtsp://%s/live/ch%02d_%d\n", get_local_ip("eth0"), ch_num, bs_num);
		}
	}
	return 0;
}

int is_bs_all_disable(void)
{
	av_t *e;
	int ch_num, bs_num;

	for (ch_num = 0; ch_num < rtspd_ch_num; ch_num++) {
		for (bs_num = 0; bs_num < rtspd_bs_num; bs_num++) {

			e = &encode_av;
			if (e->bs[ch_num][bs_num].enabled == DVR_ENC_EBST_ENABLE) {
				return 0;    /* already enabled */
			}
		}
	}
	return 1;
}

static char *get_local_ip(char *if_name)
{
	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, if_name, IFNAMSIZ - 1);
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);
	return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
}

int update_video_sdp(int ch_num, int bs_num, VIDEO_CODEC_TYPE enc_type , unsigned char *data , int data_len)
{
	static int isFirstUpdate = 1;
	priv_avbs_t *pb;
	char rtsp_enc_type_tmp_str[10];
	avbs_t *avbs;
	int reopen_live_stream = 0;

	avbs = &encode_av.bs[ch_num][bs_num];
	pb = &encode_av.priv_bs[ch_num][bs_num];

	void __update_video_sdp(int ch_num, int bs_num, VIDEO_CODEC_TYPE enc_type , unsigned char *data , int data_len)
	{
		//Update sdp parameter encoder
		avbs->video.enc_type = enc_type;
		sprintf(rtsp_enc_type_tmp_str , rtsp_enc_type_str[convert_to_rtsp_video_codec(enc_type)]);
		QAS_MSG("[Ch%d_%d] RTSP codec change to %s", ch_num, bs_num, rtsp_enc_type_tmp_str);
		stream_sdp_parameter_encoder(rtsp_enc_type_tmp_str, (unsigned char*) data , data_len , pb->video.sdpstr, SDPSTR_MAX);
		return;
	}

	//Codec not change
	if(avbs->video.enc_type == enc_type){
		if(isFirstUpdate == 1){
			__update_video_sdp(ch_num, bs_num , enc_type , data , data_len);
			isFirstUpdate = 0;
		}
	}
	else{
		__update_video_sdp(ch_num, bs_num , enc_type , data , data_len);
		reopen_live_stream = 1;
	}

	if(avbs->audio.is_format_change == 1){
		reopen_live_stream = 1;
		avbs->audio.is_format_change = 0;
	}

	if(reopen_live_stream){
		int timeout_ms = 10000;	//10 sec
		int sleep_ms = 500;	//500 ms
		while (timeout_ms >=0){
			if (g_rtsp_server_state == RTSP_SERVER_STATE_RUNNING) {
				if ((pb->close != 0) && (pb->open != 0)) {
					pthread_mutex_lock(&encode_av.ubs_mutex);
					pb->close(ch_num, bs_num);
					pb->open(ch_num, bs_num);
					pthread_mutex_unlock(&encode_av.ubs_mutex);
					break;
				}
			}
			usleep(500*1000);
			timeout_ms -= sleep_ms;
		};

		if (timeout_ms > 0) QAS_MSG("[Ch%d_%d] reopen RTSP stream done !!", ch_num, bs_num);
		else {
			ERR_MSG("[Ch%d_%d] reopen RTSP stream done fail !!", ch_num, bs_num);
			return -1;
		}
	}
	return 0;
}

//Elvis - 20190412 +
RTSP_SERVER_STATE get_rtsp_server_state(){
	return g_rtsp_server_state;
}
//Elvis - 20190412 -

