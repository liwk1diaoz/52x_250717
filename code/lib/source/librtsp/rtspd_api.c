/**
	@brief Sample code of video record.\n

	@file video_record.c

	@author Boyan Huang

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "hdal.h"
#include "hd_debug.h"
#include <netinet/in.h>
#include <net/if.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>

// Vanness
#include <linux/sockios.h>

#define DEBUG_MENU 		1

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)


///////////////////////////////////////////////////////////////////////////////

typedef struct _VIDEO_RECORD {

	// (1)
	HD_VIDEOCAP_SYSCAPS cap_syscaps;
	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	HD_DIM  cap_dim;
	HD_DIM  proc_max_dim;

	// (2)
	HD_VIDEOPROC_SYSCAPS proc_syscaps;
	HD_PATH_ID proc_ctrl;
	HD_PATH_ID proc_path;

	HD_DIM  enc_max_dim;
	HD_DIM  enc_dim;

	// (3)
	HD_VIDEOENC_SYSCAPS enc_syscaps;
	HD_PATH_ID enc_path;

	// (4) user pull
	pthread_t  enc_thread_id;
	UINT32     enc_exit;
	UINT32     flow_start;

} VIDEO_RECORD;
///////////////////////////////////////////////////////////////////////////////

#include "sysdef.h"
#include "event.h"
#include "rtp.h"
#include "live.h"
#include "frame.h"
#include "stream.h"
#include "rtp_media.h"
#include "tcp.h"
#include "gm_memory.h"
#include "librtsp.h"
#include "hd_type.h"
//#include "isf_stream_def.h"
#include <semaphore.h>

#define CAP_CH_NUM          1
#define RTSP_NUM_PER_CAP    1 //4
#define CAP_PATH_NUM        1 //4
#define ENC_TRACK_NUM       1 //4

#define SDPSTR_MAX      128
#define SR_MAX          64
#define VQ_MAX          (SR_MAX)
#define VQ_LEN          5
#define AQ_MAX          64            /* 1 MP2 and 1 AMR for live streaming, another 2 for file streaming. */
#define AQ_LEN          2            /* 1 MP2 and 1 AMR for live streaming, another 2 for file streaming. */
#define AV_NAME_MAX     127

#define RTP_HZ          90000

#define ERR_GOTO(x, y)      do { ret = x; goto y; } while(0)
#define MUTEX_FAILED(x)     (x == ERR_MUTEX)
#define VIDEO_FRAME_NUMBER VQ_LEN+1

#define NONE_BS_EVENT    0
#define START_BS_EVENT   1
#define STOP_BS_EVENT    2

#define CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num)    \
    do {    \
        if((ch_num >= CAP_CH_NUM || ch_num < 0) || \
            (sub_num >= RTSP_NUM_PER_CAP || sub_num < 0)) {    \
            fprintf(stderr, "%s: ch_num=%d, sub_num=%d error!\n",__FUNCTION__, ch_num, sub_num);    \
            return -1; \
        }    \
    } while(0)    \


typedef int (*open_container_fn)(int ch_num, int sub_num);
typedef int (*close_container_fn)(int ch_num, int sub_num);

typedef enum st_opt_type {
    OPT_NONE=0,
    RTSP_LIVE_STREAMING,
} opt_type_t;

typedef struct st_vbs {
    int enabled; //DVR_ENC_EBST_ENABLE: enabled, DVR_ENC_EBST_DISABLE: disabled
    int enc_type;    // 0:ENC_TYPE_H264, 1:ENC_TYPE_MPEG4, 2:ENC_TYPE_MJPEG
} vbs_t;

typedef struct st_priv_vbs {
    char sdpstr[SDPSTR_MAX];
    int qno;
    int offs;
    int len;
    unsigned int tv_ms;
    int cap_ch;
    int cap_path;
    int rec_track;
    char *bs_buf;
    unsigned int bs_buf_len;
    pthread_mutex_t priv_vbs_mutex;
} priv_vbs_t;

typedef struct st_bs {
    int event; // config change please set 1 for enqueue_thread to config this
    int enabled; //DVR_ENC_EBST_ENABLE: enabled, DVR_ENC_EBST_DISABLE: disabled
    opt_type_t opt_type;  /* 1:rtsp_live_streaming, 2: file_avi_recording 3:file_h264_recording */
    vbs_t video;  /* VIDEO, 0: main-bitstream, 1: sub1-bitstream, 2:sub2-bitstream */
} avbs_t;

typedef struct st_priv_bs {
    int play;
    int congest;
    int sr;
     char name[AV_NAME_MAX];
    open_container_fn open;
    close_container_fn close;
    priv_vbs_t video;  /* VIDEO, 0: main-bitstream, 1: sub1-bitstream, 2:sub2-bitstream */
} priv_avbs_t;

typedef struct st_av {
    /* public data */
    avbs_t bs[RTSP_NUM_PER_CAP];  /* VIDEO, 0: main-bitstream, 1: sub1-bitstream, 2:sub2-bitstream */
    /* update date */
    pthread_mutex_t ubs_mutex;

    /* private data */
    int enabled;      //DVR_ENC_EBST_ENABLE: enabled, DVR_ENC_EBST_DISABLE: disabled
    priv_avbs_t priv_bs[RTSP_NUM_PER_CAP];
} av_t;

pthread_t enqueue_thread_id = 0;
pthread_t encode_thread_id = 0;
unsigned int sys_tick = 0;
struct timeval sys_sec = {-1, -1};
int sys_port = 554;
char *ipptr = NULL;

pthread_mutex_t stream_queue_mutex;
av_t enc[CAP_CH_NUM];
static UINT32 virt_start_addr = 0;  // start address of bs_buffer in isf_vdoenc (virtual address)
static HD_VIDEOENC_BUFINFO phy_start_addr = {0};   // start address of bs_buffer in isf_vdoenc (physical address)
#define PHY2VIRT_SUB(bs_phy_addr) (virt_start_addr + (bs_phy_addr - phy_start_addr.buf_info.phy_addr))

#define TIMESTAMP_TO_MS(ts)  ((ts >> 32) * 1000 + (ts & 0xFFFFFFFF) / 1000)

static int rtspd_sysinit = 0;
static int rtspd_set_event = 0;

static int rtspd_codec = HD_CODEC_TYPE_H265;

#define DVR_ENC_EBST_ENABLE     0x55887799
#define DVR_ENC_EBST_DISABLE    0

#define ENC_TYPE_H264           0
#define ENC_TYPE_MPEG4          1
#define ENC_TYPE_MJPEG          2
#define ENC_TYPE_H265           3

#define OUT_BUFF_SIZE 2*1024*1024
#define TIMEVAL_DIFF(start, end) ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

#ifdef __CONFIG_WITH_FDT__
#include "../../../sample/fdt/sensor_cfg_fdt.c"
#endif

//VIDEO_RECORD stream[1] = {0}; //0: main stream
sem_t sem_rtspd;
HD_VIDEOENC_BS	data_pull;


priv_avbs_t *find_file_sr(char *name, int srno)
{
    int ch_num, sub_num, hit=0;
    priv_avbs_t *pb;

    for (ch_num = 0; ch_num < CAP_CH_NUM; ch_num++) {
        for (sub_num = 0; sub_num < RTSP_NUM_PER_CAP; sub_num++) {
            pb = &enc[ch_num].priv_bs[sub_num];
            if ((pb->sr == srno) && (pb->name) && (strcmp(pb->name, name) == 0)) {
                hit = 1;
                break;
            }
        }
        if (hit)
            break;
    }
    return (hit ? pb : NULL);
}

static int frm_cb(int type, int qno, gm_ss_entity *entity)
{
    priv_avbs_t *pb;
    int ch_num, sub_num;
    for (ch_num = 0; ch_num < CAP_CH_NUM; ch_num++) {
        for (sub_num = 0; sub_num < RTSP_NUM_PER_CAP; sub_num++) {
            pb = &enc[ch_num].priv_bs[sub_num];
            if (pb->video.offs == (int)(entity->data)
                    && pb->video.len == entity->size && pb->video.qno==qno) {
                pthread_mutex_lock(&pb->video.priv_vbs_mutex);
                pb->video.offs = 0;
                pb->video.len = 0;
                pthread_mutex_unlock(&pb->video.priv_vbs_mutex);
            }
        }
    }
    return 0;
}

static int cmd_cb(char *name, int sno, int cmd, void *p)
{
    int ret = -1;
    priv_avbs_t *pb;

    switch(cmd)
    {
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
            if( strncmp(name, "live/", 5) == 0 ) {
                if ((pb = find_file_sr(name, sno)) == NULL)
                    ERR_GOTO(-1, cmd_cb_err);
                if (pb->video.qno >= 0)
                    pb->play = 1;
            }
            ret = 0;
            break;
        case GM_STREAM_CMD_PAUSE:
            printf("%s:%d <GM_STREAM_CMD_PAUSE>\n", __FUNCTION__, __LINE__);
            ret = 0;
            break;
        case GM_STREAM_CMD_TEARDOWN:
            if( strncmp(name, "live/", 5) == 0 ) {
                if ((pb = find_file_sr(name, sno)) == NULL)
                    ERR_GOTO(-1, cmd_cb_err);
                pb->play = 0;
            }
            ret = 0;
            break;
        default:
            fprintf(stderr, "%s: not support cmd %d\n", __func__, cmd);
            break;
    }

cmd_cb_err:
    if( ret < 0 ) {
        fprintf(stderr, "%s: cmd %d error %d\n", __func__, cmd, ret);
    }
    return ret;
}

int env_init(void)
{
    int ret = 0;
    int ch_num, sub_num;
    av_t *e;
    avbs_t *b;
    priv_avbs_t *pb;

    memset(enc,0,sizeof(enc));

    /* private data initial */
    for (ch_num = 0; ch_num < CAP_CH_NUM; ch_num++) {
        e = &enc[ch_num];
        if (pthread_mutex_init(&e->ubs_mutex, NULL)) {
            perror("env_init: mutex init failed:");
            exit(-1);
        }
        for (sub_num = 0; sub_num < RTSP_NUM_PER_CAP; sub_num++) {
            b = &e->bs[sub_num];
            b->opt_type = RTSP_LIVE_STREAMING;

			switch( rtspd_codec)
			{
				case HD_CODEC_TYPE_JPEG :     b->video.enc_type = ENC_TYPE_MJPEG;     break;
				case HD_CODEC_TYPE_H264 :     b->video.enc_type = ENC_TYPE_H264;      break;
				case HD_CODEC_TYPE_H265 :     b->video.enc_type = ENC_TYPE_H265;      break;
				default:
					perror("unknown codec type !!\r\n");
					break;
			}

            b->event = NONE_BS_EVENT;
            b->enabled = DVR_ENC_EBST_DISABLE;
            b->video.enabled = DVR_ENC_EBST_DISABLE;

            pb = &e->priv_bs[sub_num];
            pb->video.qno = -1;
            pb->video.offs=0;
            pb->video.len=0;
            pb->sr = -1;
            if (pthread_mutex_init(&pb->video.priv_vbs_mutex, NULL)) {
                perror("env_enc_init: priv_vbs mutex init failed:");
                exit(-1);
            }
        }
    }

    srand((unsigned int)time(NULL));
    if ((ret = stream_server_init(ipptr, (int) sys_port, 0, 1444, 256, SR_MAX, VQ_MAX, VQ_LEN,
                                  AQ_MAX, AQ_LEN, frm_cb, cmd_cb)) < 0)
        fprintf(stderr, "stream_server_init, ret %d\n", ret);
    if ((ret = stream_server_start()) < 0)
        fprintf(stderr, "stream_server_start, ret %d\n", ret);
    return ret;
}

int env_set_bs_new_event(int ch_num, int sub_num, int event)
{
    int ret = 0;
    avbs_t *b;

    CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
    b = &enc[ch_num].bs[sub_num];

    switch (event) {
        case START_BS_EVENT:
            if (b->opt_type == OPT_NONE)
                goto err_exit;
            if (b->enabled == DVR_ENC_EBST_ENABLE) {
                fprintf(stderr, "Already enabled: ch_num=%d, sub_num=%d\n", ch_num, sub_num);
                ret = -1;
                goto err_exit;
            }
            break;
        case STOP_BS_EVENT:
            if (b->enabled != DVR_ENC_EBST_ENABLE) {
                fprintf(stderr, "Already disabled: ch_num=%d, sub_num=%d\n", ch_num, sub_num);
                ret = -1;
                goto err_exit;
            }
            break;
        default:
            fprintf(stderr, "env_set_bs_new_event: ch_num=%d, sub_num=%d, event=%d, error\n",
                            ch_num, sub_num, event);
            ret = -1;
            goto err_exit;
    }
    b->event = event;
    rtspd_set_event = 1;
err_exit:
    return ret;
}

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
    sys_tick=tv_ms*(RTP_HZ/1000);
    return sys_tick;
}

static int convert_gmss_media_type(int type)
{
    int media_type;

    switch(type) {
        case ENC_TYPE_H264:
            media_type = GM_SS_TYPE_H264;
            break;
		case ENC_TYPE_H265:
			media_type = GM_SS_TYPE_H265;
			break;
        case ENC_TYPE_MPEG4:
            media_type = GM_SS_TYPE_MP4;
            break;
        case ENC_TYPE_MJPEG:
            media_type = GM_SS_TYPE_MJPEG;
            break;
        default:
            media_type  = -1;
            fprintf(stderr, "convert_gmss_media_type: type=%d, error!\n", type);
            break;
    }
    return media_type;
}

static int open_live_streaming(int ch_num, int sub_num)
{
    int media_type;
    avbs_t *b;
    priv_avbs_t *pb;
    char livename[64];

    CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
    b = &enc[ch_num].bs[sub_num];
    pb = &enc[ch_num].priv_bs[sub_num];
    media_type = convert_gmss_media_type(b->video.enc_type);
    pb->video.qno = do_queue_alloc(media_type);

    sprintf(livename, "live/ch%02d_%d", ch_num, sub_num);
    pb->sr = stream_reg(livename, pb->video.qno, pb->video.sdpstr,
            0, 0, 1, 0, 0, 0, 0, 0, 0);

    if (pb->sr < 0)
        fprintf(stderr, "open_live_streaming: ch_num=%d, sub_num=%d setup error\n", ch_num, sub_num);
    strcpy(pb->name, livename);
    return 0;
}

static int write_rtp_frame_ext(int ch_num, int sub_num, void *data, int data_len, unsigned int tv_ms)
{
    int ret, media_type;
    avbs_t *b;
    priv_avbs_t *pb;
    gm_ss_entity entity;
    struct timeval curr_tval;
    static struct timeval err_print_tval;

    pb = &enc[ch_num].priv_bs[sub_num];
    b = &enc[ch_num].bs[sub_num];

    if((pb->play == 0) || (b->event != NONE_BS_EVENT)) {
        ret = 1;
        goto exit_free_as_buf;
    }

    entity.data = (char *) data;
    entity.size = data_len;
    entity.timestamp = get_tick_gm(tv_ms);
    media_type = convert_gmss_media_type(b->video.enc_type);

    pthread_mutex_lock(&stream_queue_mutex);
    ret = stream_media_enqueue(media_type, pb->video.qno, &entity);
    pthread_mutex_unlock(&stream_queue_mutex);

    if (ret < 0) {
        gettimeofday(&curr_tval, NULL );
        if (ret == ERR_FULL) {
            pb->congest = 1;
            if (TIMEVAL_DIFF(err_print_tval, curr_tval) > 5000000) {
                fprintf(stderr, "ext enqueue queue ch_num=%d, sub_num=%d full\n", ch_num, sub_num);
            }
        } else if ((ret != ERR_NOTINIT) && (ret != ERR_MUTEX) && (ret != ERR_NOTRUN)) {
            if (TIMEVAL_DIFF(err_print_tval, curr_tval) > 5000000) {
                fprintf(stderr, "ext enqueue queue ch_num=%d, sub_num=%d error %d\n", ch_num, sub_num, ret);
            }
        }
        if (TIMEVAL_DIFF(err_print_tval, curr_tval) > 5000000) {
            fprintf(stderr, "ext enqueue queue ch_num=%d, sub_num=%d error %d\n", ch_num, sub_num, ret);
            gettimeofday(&err_print_tval, NULL );
        }
        goto exit_free_audio_buf;
    }
    return 0;

exit_free_audio_buf:
exit_free_as_buf:
    return 1;
}

static int close_live_streaming(int ch_num, int sub_num)
{
    priv_avbs_t *pb;
    int ret = 0;

    CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
    pb = &enc[ch_num].priv_bs[sub_num];
    if (pb->sr >= 0) {
        ret = stream_dereg(pb->sr, 1);
        if (ret < 0)
            goto err_exit;
        pb->sr = -1;
        pb->video.qno = -1;
        pb->play = 0;
    }

err_exit:
    if(ret < 0)
        fprintf(stderr, "%s: stream_dereg(%d) err %d\n", __func__, pb->sr, ret);
    return ret;
}

int open_bs(int ch_num, int sub_num)
{
    avbs_t *b;
    priv_avbs_t *pb;

    CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
    pb = &enc[ch_num].priv_bs[sub_num];
    b = &enc[ch_num].bs[sub_num];

    enc[ch_num].enabled = DVR_ENC_EBST_ENABLE;
    enc[ch_num].bs[sub_num].enabled = DVR_ENC_EBST_ENABLE;
    enc[ch_num].bs[sub_num].video.enabled = DVR_ENC_EBST_ENABLE;

    switch (b->opt_type) {
        case RTSP_LIVE_STREAMING:
            //set_sdpstr(pb->video.sdpstr, b->video.enc_type);
            pb->open = open_live_streaming;
            pb->close = close_live_streaming;
            break;
        case OPT_NONE:
        default:
            break;
    }
    return 0;
}

int close_bs(int ch_num, int sub_num)
{
    av_t *e;
    int sub, is_close_channel = 1;

    CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
    e = &enc[ch_num];

    e->bs[sub_num].video.enabled = DVR_ENC_EBST_DISABLE;
    e->bs[sub_num].enabled = DVR_ENC_EBST_DISABLE;
    for (sub = 0; sub < RTSP_NUM_PER_CAP; sub++) {
        if (e->bs[sub].video.enabled == DVR_ENC_EBST_ENABLE) {
            is_close_channel = 0;
            break;
        }
    }
    if (is_close_channel == 1)
        enc[ch_num].enabled = DVR_ENC_EBST_DISABLE;
    return 0;
}

static int bs_check_event(void)
{
    int ch_num, sub_num, ret = 0;
    avbs_t *b;

    for (ch_num = 0; ch_num < CAP_CH_NUM; ch_num++) {
        for (sub_num = 0; sub_num<RTSP_NUM_PER_CAP; sub_num++) {
            b = &enc[ch_num].bs[sub_num];
            if (b->event != NONE_BS_EVENT) {
                ret = 1;
                break;
            }
        }
    }
    return ret;
}

void bs_new_event(void)
{
    int ch_num, sub_num;
    avbs_t *b;
    priv_avbs_t *pb;

    if (bs_check_event() == 0) {
        rtspd_set_event = 0;
        return;
    }

    for (ch_num = 0; ch_num < CAP_CH_NUM; ch_num++) {
        pthread_mutex_lock(&enc[ch_num].ubs_mutex);
        for (sub_num = 0; sub_num < RTSP_NUM_PER_CAP; sub_num++) {
            b = &enc[ch_num].bs[sub_num];
            pb = &enc[ch_num].priv_bs[sub_num];
            switch (b->event) {
                case START_BS_EVENT:
                    open_bs(ch_num, sub_num);
                    if(pb->open) pb->open(ch_num, sub_num);
                    b->event = NONE_BS_EVENT;
                    break;
                case STOP_BS_EVENT:
                    pb->open = NULL;
                    if(pb->close) {  /* for recording */
                        pb->close(ch_num, sub_num);
                        pb->close = NULL;
                        close_bs(ch_num, sub_num);
                    }
                    b->event = NONE_BS_EVENT;
                    break;
                default:
                    break;
            }
        }
        pthread_mutex_unlock(&enc[ch_num].ubs_mutex);
    }
}

int set_poll_event(void)
{
    int ch_num, sub_num, ret = -1;
    av_t *e;
    avbs_t *b;

    for (ch_num = 0; ch_num < CAP_CH_NUM; ch_num++) {
        e = &enc[ch_num];
        if (e->enabled != DVR_ENC_EBST_ENABLE)
            continue;
        for (sub_num = 0; sub_num < RTSP_NUM_PER_CAP; sub_num++) {
            b = &e->bs[sub_num];
            if (b->video.enabled == DVR_ENC_EBST_ENABLE) {
                ret = 0;
            }
        }
    }
    return ret;
}

static void env_release_resources(void)
{
    int ret, ch_num;
    av_t *e;

    if ((ret = stream_server_stop()))
        fprintf(stderr, "stream_server_stop() error %d\n", ret);
    for (ch_num = 0; ch_num < CAP_CH_NUM; ch_num++) {
        e = &enc[ch_num];
        pthread_mutex_destroy(&e->ubs_mutex);
    }
}

static void _rtspd_stop(void)
{
    pthread_mutex_destroy(&stream_queue_mutex);
    rtspd_sysinit = 0;
}

char *get_local_ip(void)
{
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);
    return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
}

void update_video_sdp(int cap_ch, int cap_path, int rec_track,void *arg)
{
	priv_avbs_t *pb;
	char  *data_addr = 0, *sps_addr  = 0, *pps_addr=0, *vps_addr=0;
	UINT32 data_size = 0,sps_size=0, pps_size=0, vps_size=0;
	unsigned char * bs_bs_bs_buf = 0, *p=0;
	UINT32 bs_len = 0;
	UINT32 bIsKey = 0;

	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_BS  data_pull;
	UINT32 j;

	//--- Pull bs, until get I-frame ---
	do 
    {
		//pull data
		ret = hd_videoenc_pull_out_buf(p_stream0->enc_path, &data_pull, -1);
		if (ret == HD_OK) 
        {
			bIsKey      = data_pull.frame_type;

			if((bIsKey != HD_FRAME_TYPE_IDR)&&(bIsKey != HD_FRAME_TYPE_I))
			{
				// release data
				ret = hd_videoenc_release_out_buf(p_stream0->enc_path, &data_pull);
			}
            
        } else 
        {
        	printf("pull bs fail, try again ...\r\n\r\n");
        }
        
	}while ((bIsKey != HD_FRAME_TYPE_IDR)&&(bIsKey != HD_FRAME_TYPE_I));

	//--- Parse I-frame to get Desc ---
	if (data_pull.vcodec_format ==  HD_CODEC_TYPE_H264) {
		sps_addr  = (char *)PHY2VIRT_SUB(data_pull.video_pack[0].phy_addr);
		sps_size  = data_pull.video_pack[0].size;
		pps_addr  = (char *)PHY2VIRT_SUB(data_pull.video_pack[1].phy_addr);
		pps_size  = data_pull.video_pack[1].size;
		data_addr = (char *)PHY2VIRT_SUB(data_pull.video_pack[2].phy_addr);
		data_size = data_pull.video_pack[2].size;
	} else if (data_pull.vcodec_format == HD_CODEC_TYPE_H265) {
		vps_addr  = (char *)PHY2VIRT_SUB(data_pull.video_pack[0].phy_addr);
		vps_size  = data_pull.video_pack[0].size;
		sps_addr  = (char *)PHY2VIRT_SUB(data_pull.video_pack[1].phy_addr);
		sps_size  = data_pull.video_pack[1].size;
		pps_addr  = (char *)PHY2VIRT_SUB(data_pull.video_pack[2].phy_addr);
		pps_size  = data_pull.video_pack[2].size;
		data_addr = (char *)PHY2VIRT_SUB(data_pull.video_pack[3].phy_addr);
		data_size = data_pull.video_pack[3].size;
	} else if (data_pull.vcodec_format == HD_CODEC_TYPE_JPEG) {
		data_addr = (char *)PHY2VIRT_SUB(data_pull.video_pack[0].phy_addr);
		data_size = data_pull.video_pack[0].size;
	}
	
	if (data_pull.vcodec_format ==  HD_CODEC_TYPE_H264) {
		bs_len = data_size + sps_size + pps_size;
	} else if (data_pull.vcodec_format == HD_CODEC_TYPE_H265) {
		bs_len = data_size + sps_size + pps_size + vps_size;
	} else if (data_pull.vcodec_format == HD_CODEC_TYPE_JPEG) {
		bs_len = data_size;
	}

	bs_bs_bs_buf = malloc(bs_len);
	p = bs_bs_bs_buf;

	if (data_pull.vcodec_format ==  HD_CODEC_TYPE_H264) {
		memcpy(p, sps_addr, sps_size);  p += sps_size;
		memcpy(p, pps_addr, pps_size);  p += pps_size;
		memcpy(p, data_addr, data_size);
	} else if (data_pull.vcodec_format == HD_CODEC_TYPE_H265) {
		memcpy(p, vps_addr, vps_size);  p += vps_size;
		memcpy(p, sps_addr, sps_size);  p += sps_size;
		memcpy(p, pps_addr, pps_size);  p += pps_size;
		memcpy(p, data_addr, data_size);
	} else if (data_pull.vcodec_format == HD_CODEC_TYPE_JPEG) {
		memcpy(p, data_addr, data_size);
	}

	pb = &enc[cap_ch].priv_bs[cap_path];
	if (data_pull.vcodec_format == HD_CODEC_TYPE_H264) {
		stream_sdp_parameter_encoder("H264", (unsigned char *)bs_bs_bs_buf, bs_len, pb->video.sdpstr, SDPSTR_MAX);
	} else if (data_pull.vcodec_format == HD_CODEC_TYPE_H265) {
		stream_sdp_parameter_encoder("H265", (unsigned char *)bs_bs_bs_buf, bs_len, pb->video.sdpstr, SDPSTR_MAX);
	} else if (data_pull.vcodec_format == HD_CODEC_TYPE_JPEG) {
		stream_sdp_parameter_encoder("MJPEG", (unsigned char *)bs_bs_bs_buf, bs_len, pb->video.sdpstr, SDPSTR_MAX);
	}

	free(bs_bs_bs_buf);

	// release data
	ret = hd_videoenc_release_out_buf(p_stream0->enc_path, &data_pull);
}

void _update_video_sdp(int cap_ch, int cap_path, int rec_track,void *arg)
{
	priv_avbs_t *pb;
	char  *data_addr = 0, *sps_addr  = 0, *pps_addr=0, *vps_addr=0;
	UINT32 data_size = 0,sps_size=0, pps_size=0, vps_size=0;
	unsigned char * bs_bs_bs_buf = 0, *p=0;
	UINT32 bs_len = 0;
	UINT32 bIsKey = 0;
	UINT32 j;

	//--- Pull bs, until get I-frame ---
	do 
    {
		sem_wait(&sem_rtspd);
		//pull data
		//ret = hd_videoenc_pull_out_buf(p_stream0->enc_path, &data_pull, -1);
		//if (ret == HD_OK) 
        {
			bIsKey      = data_pull.frame_type;

			//if((bIsKey != HD_FRAME_TYPE_IDR)&&(bIsKey != HD_FRAME_TYPE_I))
			{
				// release data
				//ret = hd_videoenc_release_out_buf(p_stream0->enc_path, &data_pull);
			}
            
        } //else 
        {
        	//printf("pull bs fail, try again ...\r\n\r\n");
        }
        
	}while ((bIsKey != HD_FRAME_TYPE_IDR)&&(bIsKey != HD_FRAME_TYPE_I));

	//--- Parse I-frame to get Desc ---
	if (data_pull.vcodec_format ==  HD_CODEC_TYPE_H264) {
		sps_addr  = (char *)PHY2VIRT_SUB(data_pull.video_pack[0].phy_addr);
		sps_size  = data_pull.video_pack[0].size;
		pps_addr  = (char *)PHY2VIRT_SUB(data_pull.video_pack[1].phy_addr);
		pps_size  = data_pull.video_pack[1].size;
		data_addr = (char *)PHY2VIRT_SUB(data_pull.video_pack[2].phy_addr);
		data_size = data_pull.video_pack[2].size;
	} else if (data_pull.vcodec_format == HD_CODEC_TYPE_H265) {
		vps_addr  = (char *)PHY2VIRT_SUB(data_pull.video_pack[0].phy_addr);
		vps_size  = data_pull.video_pack[0].size;
		sps_addr  = (char *)PHY2VIRT_SUB(data_pull.video_pack[1].phy_addr);
		sps_size  = data_pull.video_pack[1].size;
		pps_addr  = (char *)PHY2VIRT_SUB(data_pull.video_pack[2].phy_addr);
		pps_size  = data_pull.video_pack[2].size;
		data_addr = (char *)PHY2VIRT_SUB(data_pull.video_pack[3].phy_addr);
		data_size = data_pull.video_pack[3].size;
	} else if (data_pull.vcodec_format == HD_CODEC_TYPE_JPEG) {
		data_addr = (char *)PHY2VIRT_SUB(data_pull.video_pack[0].phy_addr);
		data_size = data_pull.video_pack[0].size;
	}


	if (data_pull.vcodec_format ==  HD_CODEC_TYPE_H264) {
		bs_len = data_size + sps_size + pps_size;
	} else if (data_pull.vcodec_format == HD_CODEC_TYPE_H265) {
		bs_len = data_size + sps_size + pps_size + vps_size;
	} else if (data_pull.vcodec_format == HD_CODEC_TYPE_JPEG) {
		bs_len = data_size;
	}

	bs_bs_bs_buf = malloc(bs_len);
	p = bs_bs_bs_buf;

	if (data_pull.vcodec_format ==  HD_CODEC_TYPE_H264) {
		memcpy(p, sps_addr, sps_size);  p += sps_size;
		memcpy(p, pps_addr, pps_size);  p += pps_size;
		memcpy(p, data_addr, data_size);
	} else if (data_pull.vcodec_format == HD_CODEC_TYPE_H265) {
		memcpy(p, vps_addr, vps_size);  p += vps_size;
		memcpy(p, sps_addr, sps_size);  p += sps_size;
		memcpy(p, pps_addr, pps_size);  p += pps_size;
		memcpy(p, data_addr, data_size);
	} else if (data_pull.vcodec_format == HD_CODEC_TYPE_JPEG) {
		memcpy(p, data_addr, data_size);
	}

	pb = &enc[cap_ch].priv_bs[cap_path];
	if (data_pull.vcodec_format == HD_CODEC_TYPE_H264) {
		stream_sdp_parameter_encoder("H264", (unsigned char *)bs_bs_bs_buf, bs_len, pb->video.sdpstr, SDPSTR_MAX);
	} else if (data_pull.vcodec_format == HD_CODEC_TYPE_H265) {
		stream_sdp_parameter_encoder("H265", (unsigned char *)bs_bs_bs_buf, bs_len, pb->video.sdpstr, SDPSTR_MAX);
	} else if (data_pull.vcodec_format == HD_CODEC_TYPE_JPEG) {
		stream_sdp_parameter_encoder("MJPEG", (unsigned char *)bs_bs_bs_buf, bs_len, pb->video.sdpstr, SDPSTR_MAX);
	}

	free(bs_bs_bs_buf);

	// release data
	//ret = hd_videoenc_release_out_buf(p_stream0->enc_path, &data_pull);
}

void *_enqueue_thread(void *ptr)
{
    while (rtspd_sysinit) {
        if (rtspd_set_event)
            bs_new_event();
        if (set_poll_event() < 0) {
            sleep(1);
            continue;
        }
        usleep(1000);		
    }
    env_release_resources();
    pthread_exit(NULL);
    return NULL;
}

void *_encode_thread(void *arg)
{
	int i, cap_path;
	priv_avbs_t *pb;
	unsigned char* out_buf = malloc(OUT_BUFF_SIZE);
	unsigned char *send_buf=0, *p=0;
	int send_len = 0, out_len = 0;
	char  *data_addr = 0, *sps_addr  = 0, *pps_addr=0, *vps_addr=0;
	UINT32 data_size = 0, sps_size=0, pps_size=0, vps_size=0;

	cap_path = 0;

	while (1) {
		sem_wait(&sem_rtspd);
		
		if (rtspd_sysinit == 0){
		    break;
		}

		if (rtspd_set_event) {
		    usleep(2000);
		    continue;
		}

		if (set_poll_event() < 0) {
		    usleep(2000);
		    continue;
		}

		for (i = 0; i < CAP_CH_NUM; i++) {

			if (rtspd_sysinit == 0) {
				continue;
			}

			pb = &enc[i].priv_bs[cap_path];

			//ret = hd_videoenc_pull_out_buf(stream[0].enc_path/*p_stream0->enc_path*/, &data_pull, -1);

			//if (ret != HD_OK) {
				// Release video encoded data
			//	ret = hd_videoenc_release_out_buf(stream[0].enc_path/*p_stream0->enc_path*/, &data_pull);		//???
			//	continue;
			//}
			
			if(data_pull.pack_num==3){//data_pull.vcodec_format == HD_CODEC_TYPE_H264){
				sps_addr  = (char *)PHY2VIRT_SUB(data_pull.video_pack[0].phy_addr);
				sps_size  = data_pull.video_pack[0].size;
				pps_addr  = (char *)PHY2VIRT_SUB(data_pull.video_pack[1].phy_addr);
				pps_size  = data_pull.video_pack[1].size;
				data_addr = (char *)PHY2VIRT_SUB(data_pull.video_pack[2].phy_addr);
				data_size = data_pull.video_pack[2].size;
			}else if(data_pull.pack_num==4){//data_pull.vcodec_format == HD_CODEC_TYPE_H265){//// vps + sps + pps + bs
				vps_addr  = (char *)PHY2VIRT_SUB(data_pull.video_pack[0].phy_addr);
				vps_size  = data_pull.video_pack[0].size;
				sps_addr  = (char *)PHY2VIRT_SUB(data_pull.video_pack[1].phy_addr);
				sps_size  = data_pull.video_pack[1].size;
				pps_addr  = (char *)PHY2VIRT_SUB(data_pull.video_pack[2].phy_addr);
				pps_size  = data_pull.video_pack[2].size;
				data_addr = (char *)PHY2VIRT_SUB(data_pull.video_pack[3].phy_addr);
				data_size = data_pull.video_pack[3].size;
			}else{
				data_addr = (char *)PHY2VIRT_SUB(data_pull.video_pack[0].phy_addr);
				data_size = data_pull.video_pack[0].size;
			}
			
			out_len =0;
			send_len=0;
			if (data_pull.pack_num==3){//data_pull.vcodec_format == HD_CODEC_TYPE_H264) {
				send_len = data_size + sps_size + pps_size;
			} else if (data_pull.pack_num==4){//data_pull.vcodec_format == HD_CODEC_TYPE_H265) {
				send_len = data_size + sps_size + pps_size + vps_size;
			} else if (data_pull.pack_num==1){//data_pull.vcodec_format == HD_CODEC_TYPE_JPEG) {
				send_len = data_size;
			}

			send_buf = &out_buf[out_len];
			p = &out_buf[out_len];

			if (data_pull.pack_num==3){//data_pull.vcodec_format == HD_CODEC_TYPE_H264) {
				memcpy(p, sps_addr, sps_size);  p += sps_size;  out_len += sps_size;
				memcpy(p, pps_addr, pps_size);  p += pps_size;  out_len += pps_size;
				memcpy(p, data_addr, data_size);        out_len += data_size;
			} else if (data_pull.pack_num==4){//data_pull.vcodec_format == HD_CODEC_TYPE_H265) {
				memcpy(p, vps_addr, vps_size);  p += vps_size;  out_len += vps_size;
				memcpy(p, sps_addr, sps_size);  p += sps_size;  out_len += sps_size;
				memcpy(p, pps_addr, pps_size);  p += pps_size;  out_len += pps_size;
				memcpy(p, data_addr, data_size);        out_len += data_size;
			} else if (data_pull.pack_num==1){//data_pull.vcodec_format == HD_CODEC_TYPE_JPEG) {
				memcpy(p, data_addr, data_size);        out_len += data_size;
			}

			pthread_mutex_lock(&pb->video.priv_vbs_mutex);
			pb->video.offs = (int)send_buf;
			pb->video.len = send_len;
			pb->video.tv_ms = TIMESTAMP_TO_MS(data_pull.timestamp);
			pthread_mutex_unlock(&pb->video.priv_vbs_mutex);
			
			if (write_rtp_frame_ext(0, 0, (void *)pb->video.offs, pb->video.len, pb->video.tv_ms) == 1) {
				pthread_mutex_lock(&pb->video.priv_vbs_mutex);
				pb->video.offs = (int)NULL;
				pb->video.len = 0;
				pthread_mutex_unlock(&pb->video.priv_vbs_mutex);
			}
			
			// Release video encoded data
			//ret = hd_videoenc_release_out_buf(stream[0].enc_path/*p_stream0->enc_path*/, &data_pull);
		}
		//usleep(10000);
	}

	if (out_buf)
		free(out_buf);

	encode_thread_id = (pthread_t)NULL;
	pthread_exit(NULL);
	return NULL;
}
static int _rtspd_start(int port)
{
    int ret;
	int ch_num, rtsp_num;
    pthread_attr_t attr;

    if (rtspd_sysinit == 1){
        return -1;}
		
    if ((0 < port) && (port < 0x10000)){
        sys_port = port;}
	
    if ((ret = env_init()) < 0){
        return ret;}
	
    if (pthread_mutex_init(&stream_queue_mutex, NULL)) {
        perror("rtspd_start: mutex init failed:");
        exit(-1);
    }
	
    rtspd_sysinit = 1;

    /* Record Thread */
    if (encode_thread_id == (pthread_t)NULL) {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        ret = pthread_create(&encode_thread_id, &attr, &_encode_thread, NULL);
        pthread_attr_destroy(&attr);
    }

    if (enqueue_thread_id == (pthread_t)NULL) {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        ret = pthread_create(&enqueue_thread_id, &attr, &_enqueue_thread, NULL);
        pthread_attr_destroy(&attr);
    }

    for (ch_num = 0; ch_num < CAP_CH_NUM; ch_num++) {
        pthread_mutex_lock(&enc[ch_num].ubs_mutex);
        for (rtsp_num = 0; rtsp_num < RTSP_NUM_PER_CAP; rtsp_num++)
            env_set_bs_new_event(ch_num, rtsp_num, START_BS_EVENT);
        pthread_mutex_unlock(&enc[ch_num].ubs_mutex);
    }

    return 0;
}

static BOOL rtspd_start_flag = 0;
void rtspd_start(int port)
{
	int cap_ch;
	sem_init(&sem_rtspd, 0, 0);
	
	while (rtspd_start_flag == 0) sleep(1);
	_update_video_sdp(0,0,0,NULL);
	
	_rtspd_start(port);
	
	printf("Connect command:\n");
	for (cap_ch = 0; cap_ch < CAP_CH_NUM; cap_ch++) {
		printf("    rtsp://%s/live/ch%02d_0\n", get_local_ip(), cap_ch);
	}
}

void rtspd_send_frame(HD_PATH_ID enc_path,HD_VIDEOENC_BS* p_video_bitstream,HD_VIDEOENC_BUFINFO phy_buf_main,UINT32 vir_addr_main)
{
	memcpy(&data_pull,p_video_bitstream,sizeof(HD_VIDEOENC_BS));
	phy_start_addr.buf_info.phy_addr = phy_buf_main.buf_info.phy_addr;
	phy_start_addr.buf_info.buf_size= phy_buf_main.buf_info.buf_size;
	virt_start_addr = vir_addr_main;
	rtspd_start_flag = 1;

    rtspd_codec = data_pull.vcodec_format;
	sem_post(&sem_rtspd);
}

void rtspd_stop(void)
{
	sem_destroy(&sem_rtspd);

	_rtspd_stop();
}

