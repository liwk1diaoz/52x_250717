/**
	@brief Sample code of video record.\n

	@file video_record.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_debug.h"
#include "nvtrtspd.h"
#include <kwrap/examsys.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/time.h>
#include <detection.h>
#include <nalu_header.h>
#include <pd_shm.h>
#include <pthread.h>			//for pthread API



#define CFG_KEPT_FRAME_CNT 18000 ////record 10 mins (10*60*30) frames

#define NN_USE_RTSP_DRAW   1

#define HD_VIDEOENC_PATH(dev_id, in_id, out_id)      (((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

#define CHKPNT          printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)         printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)         printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////
#define PHY2VIRT(pa) (p_video_info->vir_addr + (pa - p_video_info->phy_buf.buf_info.phy_addr))

typedef struct _VIDEO_INFO_F {
	int info_size; //for check VIDEO_INFO_F version
	int fd_info; //info
	int fd_strm; //stream
	NVTLIVE555_CODEC codec_type;
	unsigned char vps[64];
	int vps_size;
	unsigned char sps[64];
	int sps_size;
	unsigned char pps[64];
	int pps_size;
	int frame_idx;
	int frame_cnt;
	unsigned char *p_tmp; // for RELOAD only, a tmp buffer to store 1 frame content
	unsigned long long tm_begin; // for RELOAD only, time to begin
	unsigned long long chunk[CFG_KEPT_FRAME_CNT][2]; //size, timestamp
} VIDEO_INFO_F;

typedef struct _VIDEO_INFO {
	HD_PATH_ID enc_path;
	HD_VIDEOENC_BS  data_pull;
	HD_VIDEOENC_BUFINFO phy_buf;
	UINT32 vir_addr;
	NVTLIVE555_CODEC codec_type;
	unsigned char vps[64];
	int vps_size;
	unsigned char sps[64];
	int sps_size;
	unsigned char pps[64];
	int pps_size;
	int ref_cnt;
	VIDEO_INFO_F *p_dump; //for dump
	int          channel;
	unsigned char *result_data;
	unsigned char *nalu_data;
	int            nalu_len;
	int            lock_count;
	int            unlock_count;
	int            sei_count;
	int            is_pull_data;
} VIDEO_INFO;

typedef enum _APP_MODE {
	APP_MODE_NORMAL = 0,
	APP_MODE_DUMP,
	APP_MODE_RELOAD
} APP_MODE;

//static int b_hd_init = 0;
static VIDEO_INFO video_info[8] = { 0 };
static APP_MODE m_mode = APP_MODE_NORMAL;

static int flush_video(VIDEO_INFO *p_video_info)
{
	while (hd_videoenc_pull_out_buf(p_video_info->enc_path, &p_video_info->data_pull, 0) == 0) {
		if(hd_videoenc_release_out_buf(p_video_info->enc_path, &p_video_info->data_pull) != HD_OK) {
			printf("failed to hd_videoenc_release_out_buf.\n");
		}
	}
#if 0 //reset i frame not support multi-process
	HD_H26XENC_REQUEST_IFRAME req_i = {0};
	req_i.enable   = 1;
	int ret = hd_videoenc_set(p_video_info->enc_path, HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME, &req_i);
	if (ret != HD_OK) {
		printf("set_enc_param_out_request_ifrm0 = %d\r\n", ret);
		return ret;
	}
	hd_videoenc_start(p_video_info->enc_path);
	return ret;
#else
	return 0;
#endif
}

static int refresh_video_info(int id, VIDEO_INFO *p_video_info)
{
	//while to get vsp, sps, pps
	int ret;
	p_video_info->codec_type = NVTLIVE555_CODEC_UNKNOWN;
	p_video_info->vps_size = p_video_info->sps_size = p_video_info->pps_size = 0;
	memset(p_video_info->vps, 0, sizeof(p_video_info->vps));
	memset(p_video_info->sps, 0, sizeof(p_video_info->sps));
	memset(p_video_info->pps, 0, sizeof(p_video_info->pps));
	while ((ret = hd_videoenc_pull_out_buf(p_video_info->enc_path, &p_video_info->data_pull, 500)) == 0) {
		if (p_video_info->data_pull.vcodec_format == HD_CODEC_TYPE_JPEG) {
			p_video_info->codec_type = NVTLIVE555_CODEC_MJPG;
			if (hd_videoenc_release_out_buf(p_video_info->enc_path, &p_video_info->data_pull) != HD_OK) {
				printf("failed to hd_videoenc_release_out_buf.\n");
			}
			break;
		} else if (p_video_info->data_pull.vcodec_format == HD_CODEC_TYPE_H264) {
			p_video_info->codec_type = NVTLIVE555_CODEC_H264;
			if (p_video_info->data_pull.pack_num != 3) {
				if(hd_videoenc_release_out_buf(p_video_info->enc_path, &p_video_info->data_pull) != HD_OK) {
					printf("failed to hd_videoenc_release_out_buf.\n");
				}
				continue;
			}
			memcpy(p_video_info->sps, (void *)PHY2VIRT(p_video_info->data_pull.video_pack[0].phy_addr), p_video_info->data_pull.video_pack[0].size);
			memcpy(p_video_info->pps, (void *)PHY2VIRT(p_video_info->data_pull.video_pack[1].phy_addr), p_video_info->data_pull.video_pack[1].size);
			p_video_info->sps_size = p_video_info->data_pull.video_pack[0].size;
			p_video_info->pps_size = p_video_info->data_pull.video_pack[1].size;
			if(hd_videoenc_release_out_buf(p_video_info->enc_path, &p_video_info->data_pull) != HD_OK) {
				printf("failed to hd_videoenc_release_out_buf.\n");
			}
			break;
		} else if (p_video_info->data_pull.vcodec_format == HD_CODEC_TYPE_H265) {
			p_video_info->codec_type = NVTLIVE555_CODEC_H265;
			if (p_video_info->data_pull.pack_num != 4) {
				if(hd_videoenc_release_out_buf(p_video_info->enc_path, &p_video_info->data_pull) != HD_OK) {
					printf("failed to hd_videoenc_release_out_buf.\n");
				}
				continue;
			}
			memcpy(p_video_info->vps, (void *)PHY2VIRT(p_video_info->data_pull.video_pack[0].phy_addr), p_video_info->data_pull.video_pack[0].size);
			memcpy(p_video_info->sps, (void *)PHY2VIRT(p_video_info->data_pull.video_pack[1].phy_addr), p_video_info->data_pull.video_pack[1].size);
			memcpy(p_video_info->pps, (void *)PHY2VIRT(p_video_info->data_pull.video_pack[2].phy_addr), p_video_info->data_pull.video_pack[2].size);
			p_video_info->vps_size = p_video_info->data_pull.video_pack[0].size;
			p_video_info->sps_size = p_video_info->data_pull.video_pack[1].size;
			p_video_info->pps_size = p_video_info->data_pull.video_pack[2].size;
			if(hd_videoenc_release_out_buf(p_video_info->enc_path, &p_video_info->data_pull) != HD_OK) {
				printf("failed to hd_videoenc_release_out_buf.\n");
			}
			break;
		} else {
			if(hd_videoenc_release_out_buf(p_video_info->enc_path, &p_video_info->data_pull) != HD_OK) {
				printf("failed to hd_videoenc_release_out_buf.\n");
			}
		}
	}
	return 0;
}

#if NN_USE_RTSP_DRAW
static char   *g_shm = NULL;

void init_share_memory(void)
{
	int shmid = 0;
	key_t key;

	// Segment key.
	key = PD_SHM_KEY;
	// Create the segment.
	if( ( shmid = shmget( key, PD_SHMSZ, 0666 ) ) < 0 ) {
		perror( "shmget" );
		exit(1);
	}
	// Attach the segment to the data space.
	if( ( g_shm = shmat( shmid, NULL, 0 ) ) == (char *)-1 ) {
		perror( "shmat" );
		exit(1);
	}
}


void exit_share_memory(void)
{
	shmdt(g_shm);
}
#endif

static int on_open_video(int channel)
{
	if ((size_t)channel >= sizeof(video_info) / sizeof(video_info[0])) {
		printf("nvtrtspd video channel exceed\n");
		return 0;
	}
	if (video_info[channel].ref_cnt != 0) {
		printf("nvtrtspd video ch:%d in used on other client.\n", channel);
		return 0;
	}

	#if 0
	if (b_hd_init == 0) {
		// init hdal
		HD_RESULT ret = hd_common_init(1);
		if (ret != HD_OK) {
			printf("common fail=%d\n", ret);
			return 0;
		}

		// init memory
		ret = hd_common_mem_init(NULL);
		if (ret != HD_OK) {
			printf("mem fail=%d\n", ret);
			return 0;
		}

		// init videoenc modules
		ret = hd_videoenc_init();
		if (ret != HD_OK) {
			printf("init fail=%d\n", ret);
			return 0;
		}
		b_hd_init = 1;
		#if NN_USE_RTSP_DRAW
		init_detect_data();
		#endif
	}
	#endif

	hd_videoenc_get(video_info[channel].enc_path, HD_VIDEOENC_PARAM_BUFINFO, &video_info[channel].phy_buf);
	video_info[channel].vir_addr = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, video_info[channel].phy_buf.buf_info.phy_addr, video_info[channel].phy_buf.buf_info.buf_size);
	refresh_video_info(channel, &video_info[channel]);

	//no video
	if (video_info[channel].codec_type == NVTLIVE555_CODEC_UNKNOWN) {
		return 0;
	}
	#if NN_USE_RTSP_DRAW
	if (video_info[channel].ref_cnt == 0) {
		video_info[channel].channel = channel;
		video_info[channel].result_data = malloc(1024);
		video_info[channel].nalu_data = malloc(1024);
		video_info[channel].lock_count = 0;
		video_info[channel].unlock_count = 0;
		video_info[channel].sei_count = 0;
	}
	#endif
	video_info[channel].ref_cnt++;
	video_info[channel].is_pull_data = 0;

	flush_video(&video_info[channel]);

	if (m_mode == APP_MODE_DUMP) {
		char path[256] = { 0 };
		video_info[channel].p_dump = (VIDEO_INFO_F *)malloc(sizeof(VIDEO_INFO_F));
		memset(video_info[channel].p_dump, 0, sizeof(VIDEO_INFO_F));

		video_info[channel].p_dump->info_size = sizeof(VIDEO_INFO_F);
		video_info[channel].p_dump->frame_cnt = CFG_KEPT_FRAME_CNT;

		snprintf(path, 256, "/mnt/sd/%d.info", channel);
		video_info[channel].p_dump->fd_info = open(path, O_RDWR|O_CREAT|O_TRUNC, 666);
		if (video_info[channel].p_dump->fd_info == -1) {
			printf("err to create %s\n", path);
		}

		snprintf(path, 256, "/mnt/sd/%d.strm", channel);
		video_info[channel].p_dump->fd_strm = open(path, O_RDWR|O_CREAT|O_TRUNC, 666);
		if (video_info[channel].p_dump->fd_info == -1) {
			printf("err to create %s\n", path);
		}

		printf("start dumping %s\n", path);
	}

	return (int)&video_info[channel];
}

static int on_close_video(int handle)
{
	if (handle) {
		VIDEO_INFO *p_video_info = (VIDEO_INFO *)handle;
		// mummap for bs buffer
		if (p_video_info->vir_addr) {
			hd_common_mem_munmap((void *)p_video_info->vir_addr, p_video_info->phy_buf.buf_info.buf_size);
			p_video_info->vir_addr = 0;
		}
		if (p_video_info->p_dump) {
			p_video_info->p_dump->frame_cnt = p_video_info->p_dump->frame_idx;
			p_video_info->p_dump->frame_idx = 0;
			if (write(p_video_info->p_dump->fd_info, p_video_info->p_dump, sizeof(VIDEO_INFO_F)) < 0) {
				printf("failed to write .info\n");
			}
			close(p_video_info->p_dump->fd_strm);
			close(p_video_info->p_dump->fd_info);
			free(p_video_info->p_dump);
			p_video_info->p_dump = NULL;
			printf("stop dumping.\n");
		}
		p_video_info->ref_cnt--;
		#if NN_USE_RTSP_DRAW
		if (p_video_info->ref_cnt == 0) {
			free(p_video_info->result_data);
			p_video_info->result_data = NULL;
			free(p_video_info->nalu_data);
			p_video_info->nalu_data = NULL;
		}
		#endif
	}
	return 0;
}

static int on_get_video_info(int handle, int timeout_ms, NVTLIVE555_VIDEO_INFO *p_info)
{
	VIDEO_INFO *p_video_info = (VIDEO_INFO *)handle;

	p_info->codec_type = p_video_info->codec_type;
	if (p_video_info->vps_size) {
		p_info->vps_size = p_video_info->vps_size;
		memcpy(p_info->vps, p_video_info->vps, p_info->vps_size);
	}

	if (p_video_info->sps_size) {
		p_info->sps_size = p_video_info->sps_size;
		memcpy(p_info->sps, p_video_info->sps, p_info->sps_size);
	}

	if (p_video_info->pps_size) {
		p_info->pps_size = p_video_info->pps_size;
		memcpy(p_info->pps, p_video_info->pps, p_info->pps_size);
	}

	if (p_video_info->p_dump) {
		p_video_info->p_dump->codec_type = p_video_info->codec_type;
		if (p_video_info->vps_size) {
			p_video_info->p_dump->vps_size = p_video_info->vps_size;
			memcpy(p_video_info->p_dump->vps, p_video_info->vps, p_info->vps_size);
		}

		if (p_video_info->sps_size) {
			p_video_info->p_dump->sps_size = p_video_info->sps_size;
			memcpy(p_video_info->p_dump->sps, p_video_info->sps, p_info->sps_size);
		}

		if (p_video_info->pps_size) {
			p_video_info->p_dump->pps_size = p_video_info->pps_size;
			memcpy(p_video_info->p_dump->pps, p_video_info->pps, p_info->pps_size);
		}
	}

	return  0;
}

#if 0
static int fdcnn_rtsp_pd_draw(int detect_type,int index, PVDCNN_RESULT *p_obj_out, int fdcnn_num)
{
	{   //if((p_face->x != 1) && (p_face->y != 1) && (p_face->w != 1) && (p_face->h != 1)){
            //printf("type=%d, x = %d,y = %d,w = %d,h = %d,c = %d,index =%d\r\n ",detect_type,p_face->x,p_face->y,p_face->w,p_face->h,p_face->category,index);

        //}
        switch(detect_type)
		{
			case PD:

                if (fdcnn_num > index )
                {
                    if (p_obj_out->box.x <= 0)
                        p_obj_out->box.x = 1;
                    if (p_obj_out->box.y <= 0)
                        p_obj_out->box.y = 1;
                    if (p_obj_out->box.h <= 0)
                        p_obj_out->box.h = 1;
                    if (p_obj_out->box.w <= 0)
                        p_obj_out->box.w = 1;
                    p_obj_out->box.x *= RATIO_43_W;
                    p_obj_out->box.y *= RATIO_43_H;
                    p_obj_out->box.w *= RATIO_43_W;
                    p_obj_out->box.h *= RATIO_43_H;
				    generate_pdt_idx(index,index,p_obj_out->box.x, p_obj_out->box.y,p_obj_out->box.w,p_obj_out->box.h);
                }
                else
                {
                    p_obj_out->box.x = 1;
                    p_obj_out->box.y = 1;
                    p_obj_out->box.w = 1;
                    p_obj_out->box.h = 1;
                    generate_pdt_idx(index,index,p_obj_out->box.x, p_obj_out->box.y,p_obj_out->box.w,p_obj_out->box.h);
                }
                break;
			default:
				break;
		}
	}
	return 0;
}

static int fdcnn_pd_draw_info(VENDOR_AIS_FLOW_MEM_PARM pd_buf, VIDEO_LIVEVIEW *p_stream)
{
    HD_RESULT ret = HD_OK;
    static PVDCNN_RESULT pdcnn_info[FDCNN_MAX_OUTNUM];
    UINT32 fdcnn_num;
    UINT32 num;
    static HD_URECT fdcnn_size = {0, 0, OSG_RTSP_WIDTH, OSG_RTSP_HEIGHT};
    fdcnn_num = av_pdcnn_get_result(pd_buf, pdcnn_info, &fdcnn_size, FDCNN_MAX_OUTNUM);
    for(num = 0;num < 16 ;num++)
    {
	    fdcnn_rtsp_pd_draw(PD,num,pdcnn_info+num, fdcnn_num);
    }
    return ret;
}
#endif

#if NN_USE_RTSP_DRAW

int pdcnn_update_draw_info(int channel)
{
	UINT32       final_pdobj_num, i;
	PD_SHM_INFO  *p_pd_shm = (PD_SHM_INFO  *)g_shm;
	PD_SHM_RESULT *p_obj;
	FLOAT        ratio[2];

	ratio[0] = (FLOAT)p_pd_shm->enc_path[channel].w / (FLOAT)p_pd_shm->pd_path.w;
	ratio[1] = (FLOAT)p_pd_shm->enc_path[channel].h / (FLOAT)p_pd_shm->pd_path.h;
	final_pdobj_num = p_pd_shm->pd_num;
	//printf("channel = %d, w = %d, h = %d\r\n", channel, p_pd_shm->enc_path[channel].w, p_pd_shm->enc_path[channel].h);
	//printf("final_pdobj_num = %d\r\n", final_pdobj_num);
	for (i = 0; i< final_pdobj_num; i++) {
		p_obj = &p_pd_shm->pd_results[i];
		generate_pdt_idx(i, i, p_obj->x1*ratio[0], p_obj->y1 * ratio[1], (p_obj->x2 - p_obj->x1)*ratio[0], (p_obj->y2 - p_obj->y1)* ratio[1]);
		//printf("p[%d] = %d, %d, %d, %d\r\n", i, p_obj->x1, p_obj->y1, p_obj->x2 - p_obj->x1, p_obj->y2 - p_obj->y1);
	}
	for (i = final_pdobj_num; i< MAX_DETECT_COUNT; i++) {
		p_obj = &p_pd_shm->pd_results[i];
		generate_pdt_idx(i, i, 1, 1, 1, 1);
	}
    return HD_OK;
}

#if 0
static void dump_hex(unsigned char *buf, int buflen)
{
	int i;

	printf(" buf = \r\n");
	for (i = 0;i < buflen; i+=8) {
		printf(" 0x%02x, 0x%02x, 0x%02x, 0x%02x", buf[i], buf[i+1], buf[i+2], buf[i+3]);
		printf(" 0x%02x, 0x%02x, 0x%02x, 0x%02x\r\n", buf[i+4], buf[i+5], buf[i+6], buf[i+7]);
	}
	printf("end\r\n");
}
#endif

static int get_sei_stream(char **buf, int *buflen, VIDEO_INFO *p_video_info)
{
	unsigned int detect_type_1 = PD;
	int g_enc_type = 1; // H264
    int copy_len_1=0;
    int struct_len_1=0;
	unsigned char end_code[4] = {0x45, 0x4E, 0x44, 0x5F}; // 'E' 'N' 'D' '_'

	if(p_video_info->sei_count == 0) {
		copy_len_1 = 0;
	    generate_pdt(MAX_DETECT_COUNT,3,3,0,0);
		pdcnn_update_draw_info(p_video_info->channel);
		copy_result_data(detect_type_1,p_video_info->result_data,&copy_len_1);
	    memcpy(p_video_info->result_data+copy_len_1,end_code,4);
		get_struct_size(detect_type_1,&struct_len_1);
		p_video_info->nalu_len = gen_nalu_sei(g_enc_type, p_video_info->nalu_data, p_video_info->result_data, struct_len_1);
		//dump_hex(p_video_info->nalu_data, p_video_info->nalu_len);
		p_video_info->sei_count ++;
	} else {
		p_video_info->sei_count ++;
		if(p_video_info->sei_count >= 2)
			p_video_info->sei_count = 0;
	}
	*buf = (char *)p_video_info->nalu_data;
	*buflen = p_video_info->nalu_len;
	return 0;
}
#endif

static int on_lock_video(int handle, int timeout_ms, NVTLIVE555_STRM_INFO *p_strm)
{
	VIDEO_INFO *p_video_info = (VIDEO_INFO *)handle;
#if NN_USE_RTSP_DRAW
	static UINT64 timestamp;
#endif

	//printf("on_lock_video %d b\r\n", p_video_info->channel);
#if NN_USE_RTSP_DRAW
	if(p_video_info->lock_count == 0) {
#endif
		//while to get vsp, sps, pps
		if (p_video_info->is_pull_data) {
			printf("error on_lock_video \r\n");
			return -1;
		}
		int ret = hd_videoenc_pull_out_buf(p_video_info->enc_path, &p_video_info->data_pull, -1);
		if (ret != 0) {
			printf("on_lock_video e, timeout\r\n");
			return ret;
		}
		p_video_info->is_pull_data = 1;
		#if NN_USE_RTSP_DRAW
		timestamp = p_video_info->data_pull.timestamp;
		#endif
		p_strm->addr = PHY2VIRT(p_video_info->data_pull.video_pack[p_video_info->data_pull.pack_num-1].phy_addr);
		p_strm->size = p_video_info->data_pull.video_pack[p_video_info->data_pull.pack_num-1].size;
		p_strm->timestamp = p_video_info->data_pull.timestamp;

		if (p_video_info->p_dump) {
			int idx = p_video_info->p_dump->frame_idx;
			if (idx < p_video_info->p_dump->frame_cnt) {
				p_video_info->p_dump->chunk[idx][0] = p_strm->size;
				p_video_info->p_dump->chunk[idx][1] = p_strm->timestamp;
				if (write(p_video_info->p_dump->fd_strm, (unsigned char *)p_strm->addr, p_strm->size) < 0) {
					printf("failed to write .strm\n");
				}
				p_video_info->p_dump->frame_idx++;
			}
		}
#if NN_USE_RTSP_DRAW
	} else {
		char *stream;
		int   stream_len;

		get_sei_stream(&stream, &stream_len, p_video_info);
		p_strm->addr = (UINT32)stream;
		p_strm->size = (UINT32)stream_len;
		p_strm->timestamp = timestamp;

		//printf("get sei, stream = 0x%x, len=%d\r\n", stream, stream_len);
	}
	p_video_info->lock_count++;
	if(p_video_info->lock_count > 1) {
		p_video_info->lock_count = 0;
	}
#endif
	//printf("on_lock_video %d e\r\n", p_video_info->channel);
	return 0;
}

static int on_unlock_video(int handle)
{
	VIDEO_INFO *p_video_info = (VIDEO_INFO *)handle;
	int  ret = 0;

	//printf("on_unlock_video , count = %d \r\n", count);
	if(p_video_info->is_pull_data) {
		//printf("release frame \r\n");
		ret = hd_videoenc_release_out_buf(p_video_info->enc_path, &p_video_info->data_pull);
		p_video_info->is_pull_data = 0;
	}
	#if NN_USE_RTSP_DRAW
	p_video_info->unlock_count++;
	if(p_video_info->unlock_count > 1) {
		p_video_info->unlock_count = 0;
	}
	#endif
	return ret;
}


static int on_open_audio(int channel)
{
	return 0;
}

static int on_close_audio(int handle)
{
	return 0;
}

static int on_get_audio_info(int handle, int timeout_ms, NVTLIVE555_AUDIO_INFO *p_info)
{
	return 0;
}

static int on_lock_audio(int handle, int timeout_ms, NVTLIVE555_STRM_INFO *p_strm)
{
	return 0;
}

static int on_unlock_audio(int handle)
{
	return 0;
}

static int on_open_video_f(int channel)
{
	char path[256] = { 0 };
	snprintf(path, 256, "/mnt/sd/%d.info", channel);

	struct stat st;
	if (stat(path, &st) != 0) {
		printf("unable to sate %s\n", path);
		return 0;
	}

	int fd_info = open(path, O_RDONLY, 666);
	if (fd_info <= 0) {
		printf("failed to open %s\n", path);
		return 0;
	}

	VIDEO_INFO_F *p_dump = (VIDEO_INFO_F *)malloc(st.st_size);
	if (read(fd_info, p_dump, st.st_size) != st.st_size) {
		printf("failed to read %s\n", path);
		free(p_dump);
		close(fd_info);
		return 0;
	}

	p_dump->fd_info = fd_info;
	p_dump->frame_idx = 0;
	if (p_dump->info_size != sizeof(VIDEO_INFO_F)) {
		printf("%s version info not matched, must re-dump bitstream\n", path);
		free(p_dump);
		close(fd_info);
		return 0;
	}

	snprintf(path, 256, "/mnt/sd/%d.strm", channel);
	p_dump->fd_strm = open(path, O_RDONLY, 666);
	if (p_dump->fd_strm <= 0) {
		printf("failed to open %s\n", path);
		free(p_dump);
		close(fd_info);
		return 0;
	}

	// find max frame size
	int i;
	int max_frame_size = 0;
	for (i = 0; i < p_dump->frame_cnt; i++) {
		if (max_frame_size < (int)p_dump->chunk[i][0]) {
			max_frame_size = (int)p_dump->chunk[i][0];
		}
	}

	p_dump->p_tmp = (unsigned char *)malloc(max_frame_size);
	if (p_dump->p_tmp == NULL) {
		printf("failed to alloc %d bytes for %s\n", max_frame_size, path);
		free(p_dump);
		close(fd_info);
		return 0;
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);
	p_dump->tm_begin = tv.tv_sec * 1000000 + tv.tv_usec;
	return (int)p_dump;
}

static int on_close_video_f(int handle)
{
	VIDEO_INFO_F *p_dump = (VIDEO_INFO_F *)handle;

	free(p_dump->p_tmp);
	close(p_dump->fd_info);
	close(p_dump->fd_strm);
	free(p_dump);
	return 0;
}

static int on_get_video_info_f(int handle, int timeout_ms, NVTLIVE555_VIDEO_INFO *p_info)
{
	VIDEO_INFO_F *p_dump = (VIDEO_INFO_F *)handle;

	p_info->codec_type = p_dump->codec_type;
	if (p_dump->vps_size) {
		p_info->vps_size = p_dump->vps_size;
		memcpy(p_info->vps, p_dump->vps, p_info->vps_size);
	}

	if (p_dump->sps_size) {
		p_info->sps_size = p_dump->sps_size;
		memcpy(p_info->sps, p_dump->sps, p_info->sps_size);
	}

	if (p_dump->pps_size) {
		p_info->pps_size = p_dump->pps_size;
		memcpy(p_info->pps, p_dump->pps, p_info->pps_size);
	}
	return  0;
}

static int on_lock_video_f(int handle, int timeout_ms, NVTLIVE555_STRM_INFO *p_strm)
{
	VIDEO_INFO_F *p_dump = (VIDEO_INFO_F *)handle;

	if (p_dump->frame_idx >= p_dump->frame_cnt) {
		//frame end
		return -1;
	}

	int size = (int)p_dump->chunk[p_dump->frame_idx][0];
	if (read(p_dump->fd_strm, p_dump->p_tmp, size) != size) {
		printf("failed to read strm in %d bytes.\n", size);
		return -1;
	}

	// calc delay time
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long long diff_src = p_dump->chunk[p_dump->frame_idx][1] - p_dump->chunk[0][1];
	unsigned long long diff_dst = tv.tv_sec * 1000000 + tv.tv_usec - p_dump->tm_begin;
	if (diff_dst < diff_src) {
		usleep((int)(diff_src-diff_dst));
	}

	p_strm->addr = (int)p_dump->p_tmp;
	p_strm->size = size;
	p_strm->timestamp = p_dump->chunk[p_dump->frame_idx][1];

	p_dump->frame_idx++;
	return 0;
}

static int on_unlock_video_f(int handle)
{
	return 0;
}

VOID *exit_thread(VOID *arg)
{
	PD_SHM_INFO  *p_pd_shm = (PD_SHM_INFO  *)g_shm;

    while (!p_pd_shm->exit) {
		usleep(33000);
    }
	nvtrtspd_close();
	return 0;
}

EXAMFUNC_ENTRY(nvtrtsp_ipc, argc, argv)
{
	HD_RESULT ret;
	pthread_t exit_thread_id;

	if (argc > 1) {
		switch (argv[1][0]) {
		case '1':
			m_mode = APP_MODE_DUMP;
			printf("=> dump mode.\n");
			break;
		case '2':
			m_mode = APP_MODE_RELOAD;
			printf("=> reload mode.\n");
			break;
		default:
			break;
		}
	}
	#if NN_USE_RTSP_DRAW
	// init share memory
	init_share_memory();
	#endif

	// init hdal
	ret = hd_common_init(1);
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		return 0;
	}

	// init memory
	ret = hd_common_mem_init(NULL);
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		return 0;
	}

	// init videoenc modules
	ret = hd_videoenc_init();
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		return 0;
	}
	#if NN_USE_RTSP_DRAW
	init_detect_data();
	#endif


	ret = pthread_create(&exit_thread_id, NULL, exit_thread, NULL);
	if (ret < 0) {
		printf("create exit_thread failedr\r\n");
		return -1;
	}

	video_info[0].enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(0), HD_OUT(0));
	video_info[1].enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(1), HD_OUT(1));
	video_info[2].enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(2), HD_OUT(2));
	video_info[3].enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(3), HD_OUT(3));
	video_info[4].enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(4), HD_OUT(4));
	video_info[5].enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(5), HD_OUT(5));
	video_info[6].enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(6), HD_OUT(6));
	video_info[7].enc_path = HD_VIDEOENC_PATH(HD_DAL_VIDEOENC(0), HD_IN(7), HD_OUT(7));

	//live555 setup
	NVTLIVE555_HDAL_CB hdal_cb;
	if (m_mode != APP_MODE_RELOAD) {
		hdal_cb.open_video = on_open_video;
		hdal_cb.close_video = on_close_video;
		hdal_cb.get_video_info = on_get_video_info;
		hdal_cb.lock_video = on_lock_video;
		hdal_cb.unlock_video = on_unlock_video;

	} else {
		hdal_cb.open_video = on_open_video_f;
		hdal_cb.close_video = on_close_video_f;
		hdal_cb.get_video_info = on_get_video_info_f;
		hdal_cb.lock_video = on_lock_video_f;
		hdal_cb.unlock_video = on_unlock_video_f;
	}
	hdal_cb.open_audio = on_open_audio;
	hdal_cb.close_audio = on_close_audio;
	hdal_cb.get_audio_info = on_get_audio_info;
	hdal_cb.lock_audio = on_lock_audio;
	hdal_cb.unlock_audio = on_unlock_audio;
	nvtrtspd_init(&hdal_cb);
	printf("\r\nif you want to stop, just kill this pid: %d\n", getpid());
	nvtrtspd_open();



	if (m_mode != APP_MODE_RELOAD) {
		// uninit videoenc modules
		ret = hd_videoenc_uninit();
		if (ret != HD_OK) {
			printf("exit fail=%d\n", ret);
		}

		// uninit memory
		ret = hd_common_mem_uninit();
		if (ret != HD_OK) {
			printf("uninit mem fail=%d\n", ret);
		}

		// uninit hdal
		ret = hd_common_uninit();
		if (ret != HD_OK) {
			printf("uninit common fail=%d\n", ret);
		}
		#if NN_USE_RTSP_DRAW
		// exit share memory
		exit_share_memory();
		#endif
	}

	return 0;
}
