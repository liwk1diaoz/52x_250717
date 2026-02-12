/**
	@brief Sample code of video record.\n

	@file VIDEO_STREAM.c

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
#include "hdal.h"
#include "hd_debug.h"
#include "vendor_videoenc.h"
#include "vendor_audiocapture.h"
#include "vendor_common.h"
#include "vendor_videoprocess.h"
#include "sys_mempool.h"
#include "uvc_debug_menu.h"
#include "uvc_cam.h"
#include "kwrap/task.h"

// platform dependent
#if 1//defined(__LINUX)
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#define GETCHAR()				getchar()
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#include <kwrap/examsys.h> 	//for MAIN(), GETCHAR() API
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_video_record, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define DEBUG_MENU 		1

#define UVC_ON 1
#define AUDIO_ON 1
#define WRITE_BS 0
#define ALG_FD_PD 0
#define UAC_RX_ON 0
#define UVC_SUPPORT_YUV_FMT 1
#define HID_FUNC 1
#define CDC_FUNC 0
#define MULTI_STREAM 0
#define UVAC_WAIT_RELEASE 1
#define MSDC_FUNC 0 // 0: DISABLE, 1: MSDC_DISK, 2: MSDC_IQ

#define POOL_SIZE_USER_DEFINIED  0x1000000

#if UVC_ON
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <compiler.h>
#include <usb2dev.h>
#include "UVAC.h"
#include "comm/timer.h"
#include "kwrap/stdio.h"
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/examsys.h"
#include "kwrap/sxcmd.h"
#include <kwrap/cmdsys.h>
#include "kwrap/error_no.h"
#include <kwrap/util.h>
#include "hd_common.h"
#include "FileSysTsk.h"
#include <sys/stat.h>
#include "ImageApp/ImageApp_UsbMovie.h"
#include "umsd.h"
#include "sdio.h"

#define BULK_PACKET_SIZE    512

#define USB_VID                         0x0603
#define USB_PID_PCCAM                   0x8612 // not support pc cam
#define USB_PID_WRITE                   0x8614
#define USB_PID_PRINT                   0x8613
#define USB_PID_MSDC                    0x8611

#define USB_PRODUCT_REVISION            '1', '.', '0', '0'
#define USB_VENDER_DESC_STRING          'N', 0x00,'O', 0x00,'V', 0x00,'A', 0x00,'T', 0x00,'E', 0x00,'K', 0x00, 0x20, 0x00,0x00, 0x00 // NULL
#define USB_VENDER_DESC_STRING_LEN      0x09
#define USB_PRODUCT_DESC_STRING         'D', 0x00,'E', 0x00,'M', 0x00,'O', 0x00,'1', 0x00, 0x20, 0x00,0x00, 0x00 // NULL
#define USB_PRODUCT_DESC_STRING_LEN     0x07
#define USB_PRODUCT_STRING              'N','v','t','-','D','S','C'
#define USB_SERIAL_NUM_STRING           '9', '8', '5', '2', '0','0', '0', '0', '0', '0','0', '0', '1', '0', '0'
#define USB_VENDER_STRING               'N','O','V','A','T','E','K'
#define USB_VENDER_SIDC_DESC_STRING     'N', 0x00,'O', 0x00,'V', 0x00,'A', 0x00,'T', 0x00,'E', 0x00,'K', 0x00, 0x20, 0x00,0x00, 0x00 // NULL
#define USB_VENDER_SIDC_DESC_STRING_LEN 0x09


static UINT8  gTestH264HdrValue[] = {
	0x00, 0x00, 0x00, 0x01, 0x67, 0x4D, 0x00, 0x32, 0x8D, 0xA4, 0x05, 0x01, 0xEC, 0x80,
	0x00, 0x00, 0x00, 0x01, 0x68, 0xEE, 0x3C, 0x80
};

//Max sensor size
#define MAX_CAP_SIZE_W 1920 //3840
#define MAX_CAP_SIZE_H 1080 //2160
//Max bitstream size
#define MAX_BS_W 1920 //3840
#define MAX_BS_H 1080 //2160
#define MAX_BS_FPS 30
#define NVT_UI_UVAC_RESO_CNT  2
#if UVC_SUPPORT_YUV_FMT
#define MAX_YUV_W 640
#define MAX_YUV_H 360
#define MAX_YUV_FPS 30
#endif

static UVAC_VID_RESO gUIUvacVidReso[NVT_UI_UVAC_RESO_CNT] = {
	{ MAX_BS_W,   MAX_BS_H,   1,      MAX_BS_FPS,      0,      0},
	{ 1280,    720,   1,      30,      0,      0},
};

#define NVT_UI_UVAC_AUD_SAMPLERATE_CNT                  1
#define NVT_UI_FREQUENCY_16K                    0x003E80   //16k
#define NVT_UI_FREQUENCY_32K                    0x007D00   //32k
//#define NVT_UI_FREQUENCY_48K                    0x00BB80   //48k
static UINT32 gUIUvacAudSampleRate[NVT_UI_UVAC_AUD_SAMPLERATE_CNT] = {
	NVT_UI_FREQUENCY_16K//NVT_UI_FREQUENCY_32K
};
#if UVC_SUPPORT_YUV_FMT
static UVAC_VID_RESO gUIUvacVidYuvReso[] = {
	{MAX_YUV_W,	MAX_YUV_H,	1,		MAX_YUV_FPS,		 0,		 0},
	{320,	240,	1,		30,		 0,		 0},
};
#endif
static  UVAC_VIDEO_FORMAT m_VideoFmt[UVAC_VID_DEV_CNT_MAX] = {0}; //UVAC_VID_DEV_CNT_MAX=2
static BOOL m_bIsStaticPattern = FALSE;

static UINT32 uigFrameIdx = 0, uigAudIdx = 0;
static UINT32 uvac_pa = 0;
static UINT32 uvac_va = 0;
static UINT32 uvac_size = 0;
#if (MAX_CAP_SIZE_W<MAX_BS_W) || (MAX_CAP_SIZE_H<MAX_BS_H)
//disable direct mode for scaling up
static UINT32 g_capbind = 0;  //0:D2D, 1:direct, 2: one-buf, 0xff: no-bind
#else
static UINT32 g_capbind = 1;  //0:D2D, 1:direct, 2: one-buf, 0xff: no-bind
#endif
static UINT32 g_one_buf = 1;    //0x0: vprc noraml, 0x1: one buffer
static UINT32 g_low_latency = 0;
static BOOL UVAC_enable(void);
static BOOL UVAC_disable(void);
#endif

///////////////////////////////////////////////////////////////////////////////

//header
#define DBGINFO_BUFSIZE()	(0x200)

//RAW
#define VDO_RAW_BUFSIZE(w, h, pxlfmt)   (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NRX: RAW compress: Only support 12bit mode
#define RAW_COMPRESS_RATIO 59
#define VDO_NRX_BUFSIZE(w, h)           (ALIGN_CEIL_4(ALIGN_CEIL_64(w) * 12 / 8 * RAW_COMPRESS_RATIO / 100 * (h)))
//CA for AWB
#define VDO_CA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 3) << 1)
//LA for AE
#define VDO_LA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 1) << 1)

//YUV
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	(ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NVX: YUV compress
#define YUV_COMPRESS_RATIO 75
#define VDO_NVX_BUFSIZE(w, h, pxlfmt)	(VDO_YUV_BUFSIZE(w, h, pxlfmt) * YUV_COMPRESS_RATIO / 100)

///////////////////////////////////////////////////////////////////////////////
#if ((MAX_CAP_SIZE_W == 3840 && MAX_CAP_SIZE_H == 2160)|| (MAX_CAP_SIZE_W == 3264 && MAX_CAP_SIZE_H == 1836))
#define SEN_OUT_FMT		HD_VIDEO_PXLFMT_NRX12
#define CAP_OUT_FMT		HD_VIDEO_PXLFMT_NRX12
#else
#define SEN_OUT_FMT		HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT		HD_VIDEO_PXLFMT_RAW12
#endif
#define CA_WIN_NUM_W		32
#define CA_WIN_NUM_H		32
#define LA_WIN_NUM_W		32
#define LA_WIN_NUM_H		32
#define VA_WIN_NUM_W		16
#define VA_WIN_NUM_H		16
#define YOUT_WIN_NUM_W	128
#define YOUT_WIN_NUM_H	128
#define ETH_8BIT_SEL		0 //0: 2bit out, 1:8 bit out
#define ETH_OUT_SEL		1 //0: full, 1: subsample 1/2

#define VIDEOCAP_ALG_FUNC HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB
#define VIDEOPROC_ALG_FUNC HD_VIDEOPROC_FUNC_WDR | HD_VIDEOPROC_FUNC_DEFOG | HD_VIDEOPROC_FUNC_COLORNR | HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_3DNR_STA

///////////////////////////////////////////////////////////////////////////////
HD_VIDEOENC_BUFINFO phy_buf_main;
#if MULTI_STREAM
HD_VIDEOENC_BUFINFO phy_buf_sub1;
HD_VIDEOENC_BUFINFO phy_buf_sub2;
#endif
HD_AUDIOCAP_BUFINFO phy_buf_main2;
UINT32 vir_addr_main;
#if MULTI_STREAM
UINT32 vir_addr_sub1;
UINT32 vir_addr_sub2;
#endif
UINT32 vir_addr_main2;
VK_TASK_HANDLE cap_thread_id;
UINT32 flow_audio_start = 0, encode_start = 0, acquire_start = 0, ai_start = 0;
#define JPG_YUV_TRANS 0
pthread_mutex_t flow_start_lock;

#if  UAC_RX_ON
UINT32 vir_addr_main3;
pthread_t uac_thread_id;
#endif


#if ALG_FD_PD
#define NN_FDCNN_FD_MODE       ENABLE
#define NN_FDCNN_FD_PROF       DISABLE
#define NN_FDCNN_FD_DUMP       DISABLE
#define NN_FDCNN_FD_DRAW       DISABLE
#define NN_FDCNN_FD_FIX_FRM    DISABLE
#define NN_FDCNN_HIGH_PRIORITY DISABLE
#define NN_FDCNN_FD_TYPE       FDCNN_NETWORK_V20

#define NN_PVDCNN_MODE			ENABLE
#define NN_PVDCNN_PROF			DISABLE
#define NN_PVDCNN_DUMP			DISABLE
#define NN_PVDCNN_DRAW			DISABLE
#define NN_PVDCNN_FIX_FRM		DISABLE
#define NN_PVDCNN_HIGH_PRIORITY	DISABLE

#include "fdcnn_lib.h"
#include "pvdcnn_lib.h"
#include "vendor_ai/vendor_ai.h"
#include "net_util_sample.h"
#include "net_gen_sample/net_gen_sample.h"
#include "net_flow_sample/net_flow_sample.h"
#include "net_pre_sample/net_pre_sample.h"
#include "net_post_sample/net_post_sample.h"
#include "net_flow_user_sample/net_flow_user_sample.h"
#include "math.h"

#define EXTEND_PATH1	      	  HD_VIDEOPROC_0_OUT_5
#define EXTEND_PATH2	          HD_VIDEOPROC_0_OUT_6
#define OSG_LCD_WIDTH             960
#define OSG_LCD_HEIGHT            240
#define NN_USE_DRAM2              ENABLE
#define VENDOR_AI_CFG             0x000f0000  //ai project config
#define VDO_ENC_OUT_BUFFER_SIZE  (1920*1080 * 1.5 * 0.8)
#define MAX_ENC_OUT_BLK_CNT       2

static VENDOR_AIS_FLOW_MEM_PARM g_mem = {0};
static HD_COMMON_MEM_VB_BLK g_blk_info[1];


#endif //ALG_FD_PD

#include "vendor_isp.h"

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
	HD_PATH_ID proc_main_path;
#if MULTI_STREAM
	HD_PATH_ID proc_sub1_path;
	HD_PATH_ID proc_sub2_path;
#endif
	HD_PATH_ID ref_path;

	HD_DIM  enc_main_max_dim;
#if MULTI_STREAM
	HD_DIM  enc_sub1_max_dim;
	HD_DIM  enc_sub2_max_dim;
#endif
	HD_DIM  enc_main_dim;
#if MULTI_STREAM
	HD_DIM  enc_sub1_dim;
	HD_DIM  enc_sub2_dim;
#endif

	// (3)
	HD_VIDEOENC_SYSCAPS enc_syscaps;
	HD_PATH_ID enc_main_path;
	#if MULTI_STREAM
	HD_PATH_ID enc_sub1_path;
	HD_PATH_ID enc_sub2_path;
	#endif

	// (4) user pull
	VK_TASK_HANDLE enc_thread_id;
	UINT32         enc_exit;
	HD_VIDEO_CODEC codec_type;

	// (5) YUV pull
#if UVC_SUPPORT_YUV_FMT
	VK_TASK_HANDLE  acquire_thread_id;
	UINT32     acquire_exit;
#endif

#if ALG_FD_PD
    // osg
	UINT32 stamp_size;

	//ALG
	HD_DIM  proc_alg_max_dim;
	HD_PATH_ID mask_alg_path;

#if NN_FDCNN_FD_DRAW
	HD_PATH_ID mask_path0;
	HD_PATH_ID mask_path1;
	HD_PATH_ID mask_path2;
	HD_PATH_ID mask_path3;
#endif

#if NN_PVDCNN_DRAW
	HD_PATH_ID mask_path4;
	HD_PATH_ID mask_path5;
	HD_PATH_ID mask_path6;
	HD_PATH_ID mask_path7;
#endif
#endif//ALG_FD_PD
} VIDEO_STREAM;

#if ALG_FD_PD
VIDEO_STREAM stream[4] = {0}; //0: main stream
#if NN_FDCNN_FD_DRAW
int fdcnn_fd_draw_info(VENDOR_AIS_FLOW_MEM_PARM fd_buf, VIDEO_STREAM *p_stream);
#endif

#if NN_PVDCNN_DRAW
int pvdcnn_draw_info(VENDOR_AIS_FLOW_MEM_PARM buf, VIDEO_STREAM *p_stream);
#endif

typedef struct _THREAD_PARM {
    VENDOR_AIS_FLOW_MEM_PARM mem;
    VIDEO_STREAM stream;
} THREAD_PARM;

typedef struct _THREAD_DRAW_PARM {
    VENDOR_AIS_FLOW_MEM_PARM fd_mem;
    VENDOR_AIS_FLOW_MEM_PARM pvd_mem;
    VIDEO_STREAM stream;
} THREAD_DRAW_PARM;

int	draw_exit = 0;
int	pvdcnn_exit = 0;
int	fdcnn_exit = 0;

#else//ALG_FD_PD
VIDEO_STREAM stream[1] = {0}; //0: main stream
#endif//ALG_FD_PD

#if UVC_SUPPORT_YUV_FMT
#define GFX_QUEUE_MAX_NUM 3
typedef struct _GFX {	// (1)
    HD_COMMON_MEM_VB_BLK buf_blk;
    UINT32               buf_size;
    UINT32               buf_pa;
    UINT32               buf_va;
} GFX;
GFX  gfx[GFX_QUEUE_MAX_NUM];
#endif

#if  AUDIO_ON
typedef struct _AUDIO_CAPONLY {
	HD_AUDIO_SR sample_rate_max;
	HD_AUDIO_SR sample_rate;

	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	UINT32 cap_exit;
} AUDIO_CAPONLY;

AUDIO_CAPONLY au_cap = {0};
#endif

#if MULTI_STREAM
typedef struct _STREAM_USER_DATA {
	UINT32 magic;
	UINT8 streamId;
	UINT8 seq;
	UINT16 reserved;
	UINT32 timestamp;
	UINT32 dataLen;
	//UINT8 data[dataLen];
} STREAM_USER_DATA;
BOOL multi_stream_func = FALSE;
#endif

#include "detection.h"

#define MD              1       // face detection
#define PD              2       // people detection
#define FD              3       // face detection
#define FTG             4       // face tracking granding

#define OSG_WIDTH 		1920
#define OSG_HEIGHT     1080

face_result *gfd_result;
people_result *gpd_result;

#define CS_INTERFACE                        0x24
#define VC_EXTENSION_UNIT                   0x06

UINT8 get_eu_control_num(UINT8 *p_control, UINT8 size)
{
	UINT32 i, j;
	UINT8 num = 0, temp;

	for (j = 0; j < size; j++) {
		for (i = 0; i < 8; i++) {
			temp  = p_control[j] >> i;
			if (temp & 0x1) {
				num++;
			}
		}
	}
	return num;
}

int get_struct_size(unsigned int type, int *struct_len)
{
        int ret = 0;

        switch (type) {
#if 0
        case MD:
                *struct_len += sizeof(motion_result) + 4;
                break;
#endif
        case PD:
                *struct_len += sizeof(people_result) + 4;
                break;
        case FD:
                *struct_len += sizeof(face_result) + 4;
                break;
#if 0
        case FTG:
                *struct_len += sizeof(face_tracking_result) + 4;
                break;
#endif
        default:
                printf("get struct size: detect type(%d) error!\n", type);
                ret = -1;
                break;
        }
        return ret;
}

int copy_result_data(unsigned int type, unsigned char *data, int *len_offset)
{
        int ret = 0, i=0;

        switch (type) {
#if 0
        case MD:
                memcpy(data + *len_offset, (unsigned char *)gmd_result, sizeof(motion_result));
                *len_offset += sizeof(motion_result);
                break;
#endif
        case PD:
#if 0
                for (i = 0; i < (int)gpd_result->people_num; i++) {
                        printf("People id(%d) x(%u)  y(%u) width(%u) height(%u)\n", gpd_result->people[i].id, gpd_result->people[i].x, gpd_result->people[i].y, gpd_result->people[i].w, gpd_result->people[i].h);
                }
#endif
                memcpy(data + *len_offset, (unsigned char *)gpd_result, sizeof(people_result));
                *len_offset += sizeof(people_result);
                break;
        case FD:
#if 0
                for (i = 0; i < (int)gfd_result->face_num; i++) {
                        printf("Face id(%d) x(%u)  y(%u) width(%u) height(%u)\n", gfd_result->face[i].id, gfd_result->face[i].x, gfd_result->face[i].y, gfd_result->face[i].w, gfd_result->face[i].h);

                }
#endif
                memcpy(data + *len_offset, (unsigned char *)gfd_result, sizeof(face_result));
                *len_offset += sizeof(face_result);
                break;
#if 0
        case FTG:
                for (i = 0; i < (int)gftg_result->face_num; i++) {
                        printf("FTG id(%d) x(%u)  y(%u) width(%u) height(%u)\n", gftg_result->ftg[i].id, gftg_result->ftg[i].x, gftg_result->ftg[i].y, gftg_result->ftg[i].w, gftg_result->ftg[i].h);

                }
                memcpy(data + *len_offset, (unsigned char *)gftg_result, sizeof(face_tracking_result));
                *len_offset += sizeof(face_tracking_result);
                break;
#endif
        default:
                printf("copy_result_data: detect type(%d) error!\n", type);
                ret = -1;
                break;
        }
        return ret;
}

void init_people_detection(void)
{
        gpd_result = (people_result *)malloc(sizeof(people_result));
        if(gpd_result == NULL)
        {
                printf("[%s] gpd_result is NULL\n",__func__);
                return;
        }
        memset(gpd_result, 0xff, sizeof(people_result));
        gpd_result->identity[0] = 0x50; //'P'
        gpd_result->identity[1] = 0x50; //'P'
        gpd_result->identity[2] = 0x4C; //'L'
        gpd_result->identity[3] = 0x45; //'E'
	/*
    generate_pdt_idx(0,0,0,0,0,0);
    generate_pdt_idx(1,0,0,0,0,0);
    generate_pdt_idx(2,0,0,0,0,0);
    generate_pdt_idx(3,0,0,0,0,0);
    generate_pdt_idx(4,0,0,0,0,0);
    generate_pdt_idx(5,0,0,0,0,0);
    generate_pdt_idx(6,0,0,0,0,0);
    generate_pdt_idx(7,0,0,0,0,0);
    generate_pdt_idx(8,0,0,0,0,0);
    generate_pdt_idx(9,0,0,0,0,0);
    */
}

void generate_pdt_idx(int index,unsigned int id, unsigned int x, unsigned int y, unsigned int w,unsigned int h)
{
        //printf("index = %d,id=%d,x=%x,y=%d,w=%d,h=%d\n",index,id,x,y,w,h);
        gpd_result->people[index].id = id;
    gpd_result->people[index].x = x;
    gpd_result->people[index].y = y;
    gpd_result->people[index].w = w;
    gpd_result->people[index].h = h;
}

//void generate_pdt(int num, unsigned int timestamp,unsigned int x_off,unsigned int y_off)
void generate_pdt(int num, short delay_fps,short trace_fps,unsigned int x_off,unsigned int y_off)
{
        if(gpd_result == NULL)
        {
        printf("[%s] gpd_result is NULL\n",__func__);
        return;
        }
        //gpd_result->timestamp = timestamp;
        gpd_result->trace_fps = trace_fps;
        gpd_result->delay_fps = delay_fps;
        if(num > MAX_DETECT_COUNT)
                num = MAX_DETECT_COUNT;
        gpd_result->people_num = num;
	/*
	generate_pdt_idx(0,332,100 + x_off,100 + y_off,100,400);
    generate_pdt_idx(1,2,400 + x_off,0 + y_off,200,400);
    generate_pdt_idx(2,323,500 + x_off,200 + y_off,300,400);
    generate_pdt_idx(3,24,600+ x_off,20 + y_off,200,100);
    generate_pdt_idx(4,51,700 + x_off,30 + y_off,200,200);
    generate_pdt_idx(5,46,600 + x_off,500 + y_off,200,100);
    generate_pdt_idx(6,37,50+ x_off,600 + y_off,200,300);
    generate_pdt_idx(7,82,140 + x_off,800 + y_off,200,200);
    generate_pdt_idx(8,19,320 + x_off,700 + y_off,10,10);
    generate_pdt_idx(9,222,600+ x_off,140 + y_off,30,10);
	*/
        return ;
}

void init_face_detection(void)
{
        gfd_result = (face_result *)malloc(sizeof(face_result));
        memset(gfd_result, 0xff, sizeof(face_result));
        gfd_result->identity[0] = 0x46; //'F'
        gfd_result->identity[1] = 0x41; //'A'
        gfd_result->identity[2] = 0x43; //'C'
        gfd_result->identity[3] = 0x45; //'E'
}


void generate_fdt_idx(int index,unsigned int id, unsigned int x, unsigned int y, unsigned int w,unsigned int h)
{
	gfd_result->face[index].id = id;
    gfd_result->face[index].x = x;
    gfd_result->face[index].y = y;
    gfd_result->face[index].w = w;
    gfd_result->face[index].h = h;
}

void generate_fdt(int num,short delay_fps,short trace_fps,unsigned int x_off,unsigned int y_off)
{
        if(gfd_result == NULL)
        {
                printf("[%s] fdt_result is NULL\n",__func__);
                return;
        }

    //gfd_result->timestamp = timestamp;
        gfd_result->delay_fps = delay_fps;
        gfd_result->trace_fps = trace_fps;
    if(num > MAX_DETECT_COUNT)
        num = MAX_DETECT_COUNT;
    gfd_result->face_num = num;
	/*
    generate_fdt_idx(0,223,0 + x_off,0 + y_off,100,40);
    generate_fdt_idx(1,2,0 + x_off,0 + y_off,20,400);
    generate_fdt_idx(2,323,0 + x_off,0 + y_off,30,400);
    generate_fdt_idx(3,24,0 + x_off,0 + y_off,400,10);
    generate_fdt_idx(4,51,0 + x_off,0 + y_off,400,20);
    generate_fdt_idx(5,46,0 + x_off,0 + y_off,40,300);
    generate_fdt_idx(6,37,0 + x_off,0 + y_off,40,40);
    generate_fdt_idx(7,82,0 + x_off,0 + y_off,20,200);
    generate_fdt_idx(8,19,0 + x_off,0 + y_off,10,100);
    generate_fdt_idx(9,222,0 + x_off,0 + y_off,30,100);
	*/

}

#if ALG_FD_PD
static int cnn_fill_alg_info(VENDOR_AIS_FLOW_MEM_PARM cnn_buf, int detect_type)
{
	HD_RESULT ret = HD_OK;
    static PVDCNN_RSLT pdcnn_info[FDCNN_MAX_OUTNUM];
	static FDCNN_RESULT fdcnn_info[FDCNN_MAX_OUTNUM] = {0};
    UINT32 cnn_num;
    UINT32 index;
    static HD_URECT cnn_size = {0, 0, OSG_WIDTH, OSG_HEIGHT};
	switch(detect_type)
	{
		case FD:
			cnn_num = fdcnn_getresults(cnn_buf, fdcnn_info, &cnn_size, FDCNN_MAX_OUTNUM);
			for(index = 0;index < 16 ;index++)
			{
				//printf("FD: x=%d y=%d w=%d h=%d index=%d, cnn_num=%d\r\n",fdcnn_info->x, fdcnn_info->y,fdcnn_info->w,fdcnn_info->h, index,cnn_num );
				if (cnn_num > index)
                {
                    if (fdcnn_info->x <= 0)
                        fdcnn_info->x = 1;
                    if (fdcnn_info->y <= 0)
                        fdcnn_info->y = 1;
                    if (fdcnn_info->h <= 0)
                        fdcnn_info->h = 1;
                    if (fdcnn_info->w <= 0)
                        fdcnn_info->w = 1;
				    generate_fdt_idx(index,index, fdcnn_info->x, fdcnn_info->y,fdcnn_info->w,fdcnn_info->h);

                }
                else
                {
                    fdcnn_info->x = 1;
                    fdcnn_info->y = 1;
                    fdcnn_info->w = 1;
                    fdcnn_info->h = 1;
                    generate_fdt_idx(index,index, fdcnn_info->x, fdcnn_info->y,fdcnn_info->w,fdcnn_info->h);
                }
			}
			break;
		case PD:
			cnn_num = pvdcnn_get_result(cnn_buf, pdcnn_info, &cnn_size, FDCNN_MAX_OUTNUM);
			for(index = 0;index < 16 ;index++)
			{
				//printf("PD: x=%d y=%d w=%d h=%d index=%d, cnn_num=%d\r\n",pdcnn_info->box.x, pdcnn_info->box.y,pdcnn_info->box.w,pdcnn_info->box.h, index,cnn_num);
				if (cnn_num > index )
                {
                    if (pdcnn_info->box.x <= 0)
                        pdcnn_info->box.x = 1;
                    if (pdcnn_info->box.y <= 0)
                        pdcnn_info->box.y = 1;
                    if (pdcnn_info->box.h <= 0)
                        pdcnn_info->box.h = 1;
                    if (pdcnn_info->box.w <= 0)
                        pdcnn_info->box.w = 1;
				    generate_pdt_idx(index,index,pdcnn_info->box.x, pdcnn_info->box.y,pdcnn_info->box.w,pdcnn_info->box.h);
                }
                else
                {
					pdcnn_info->box.x = 1;
                    pdcnn_info->box.y = 1;
                    pdcnn_info->box.w = 1;
                    pdcnn_info->box.h = 1;
                    generate_pdt_idx(index,index,pdcnn_info->box.x, pdcnn_info->box.y,pdcnn_info->box.w,pdcnn_info->box.h);
                }
			}
			break;
		default:
			break;
	}
    return ret;

}
#endif

#if ALG_FD_PD
static HD_RESULT mem_init(UINT32 stamp_size)
#else
static HD_RESULT mem_init(void)
#endif
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
	HD_DIM buf_size;
#if ALG_FD_PD
	UINT32 mem_size = 0;

#if NN_FDCNN_FD_MODE
    mem_size += fdcnn_calcbuffsize(NN_FDCNN_FD_TYPE);
#endif

#if NN_PVDCNN_MODE
    mem_size += pvdcnn_calcbuffsize();
#endif

#endif//ALG_FD_PD

#if UVC_SUPPORT_YUV_FMT
	UINT32 i = 0;
	for (i=0; i<GFX_QUEUE_MAX_NUM; i++) {
		gfx[i].buf_blk = 0;
		gfx[i].buf_size = ALIGN_CEIL_4(6* MAX_YUV_W * MAX_YUV_H);
		gfx[i].buf_pa = 0;
		gfx[i].buf_va = 0;
	}
#endif

	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	if (g_capbind == 1) {
		//direct ... NOT require raw
		mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
														+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	} else {
		buf_size.w = (MAX_CAP_SIZE_W > MAX_BS_W) ? MAX_CAP_SIZE_W : MAX_BS_W;
		buf_size.h = (MAX_CAP_SIZE_H > MAX_BS_H) ? MAX_CAP_SIZE_H : MAX_BS_H;
		mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(buf_size.w, buf_size.h, CAP_OUT_FMT)
														+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
														+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	}
	mem_cfg.pool_info[0].blk_cnt = 2;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;

	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(MAX_CAP_SIZE_W, ALIGN_CEIL_16(MAX_CAP_SIZE_H), HD_VIDEO_PXLFMT_YUV420);	//padding to 16x
#if ((MAX_CAP_SIZE_W == 3840 && MAX_CAP_SIZE_H == 2160) || (MAX_CAP_SIZE_W == 3264 && MAX_CAP_SIZE_H == 1836))
	mem_cfg.pool_info[1].blk_cnt = 5;
#else
	#if MULTI_STREAM
	mem_cfg.pool_info[1].blk_cnt = 6;
	#else
	mem_cfg.pool_info[1].blk_cnt = 3;
	#endif
#endif
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;

#if AUDIO_ON
	// config common pool audio (main)
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[2].blk_size = 0x10000;
	mem_cfg.pool_info[2].blk_cnt = 2;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;
#endif

#if ALG_FD_PD
	// config common pool (sub)
	mem_cfg.pool_info[3].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[3].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(MAX_CAP_SIZE_W, MAX_CAP_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[3].blk_cnt = 3;
	mem_cfg.pool_info[3].ddr_id = DDR_ID0;

    // config common pool (sub)
	mem_cfg.pool_info[4].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[4].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(MAX_CAP_SIZE_W, MAX_CAP_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[4].blk_cnt = 3;
	mem_cfg.pool_info[4].ddr_id = DDR_ID0;

	// for nn
	mem_cfg.pool_info[5].type 		= HD_COMMON_MEM_CNN_POOL;
	mem_cfg.pool_info[5].blk_size 	= mem_size;
	mem_cfg.pool_info[5].blk_cnt 	= 1;
#if NN_USE_DRAM2
    mem_cfg.pool_info[5].ddr_id = DDR_ID1;
#else
	mem_cfg.pool_info[5].ddr_id = DDR_ID0;
#endif
#endif //ALG_FD_PD

#if UVC_SUPPORT_YUV_FMT
	// config common pool audio (main)
	mem_cfg.pool_info[6].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[6].blk_size = DBGINFO_BUFSIZE() + gfx[0].buf_size;
	mem_cfg.pool_info[6].blk_cnt = GFX_QUEUE_MAX_NUM;
	mem_cfg.pool_info[6].ddr_id = DDR_ID0;
#endif

#if (MSDC_FUNC == 1)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_USER_DEFINIED_POOL;
	mem_cfg.pool_info[0].blk_size = POOL_SIZE_USER_DEFINIED;
	mem_cfg.pool_info[0].blk_cnt = 1;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
#endif

	ret = hd_common_mem_init(&mem_cfg);

#if UVC_SUPPORT_YUV_FMT
	for (i=0; i<GFX_QUEUE_MAX_NUM; i++) {
		//get gfx's buffer block
		gfx[i].buf_blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, gfx[i].buf_size, DDR_ID0);
		if (gfx[i].buf_blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get block fail\r\n", gfx[i].buf_blk);
			return HD_ERR_NOMEM;
		}
		//translate gfx's buffer block to physical address
		gfx[i].buf_pa = hd_common_mem_blk2pa(gfx[i].buf_blk);
		if (gfx[i].buf_pa == 0) {
			printf("blk2pa fail, buf_blk = 0x%x\r\n", gfx[i].buf_blk);
			return HD_ERR_NOMEM;
		}
		gfx[i].buf_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, gfx[i].buf_pa, gfx[i].buf_size);
		if(!gfx[i].buf_va){
				printf("gfx(%d) hd_common_mem_mmap() fail\n", i);
				return -1;
		}
	}
#endif

	return ret;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	hd_common_mem_uninit();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
/*AUDIO */
#if AUDIO_ON
static HD_RESULT set_cap_cfg2(HD_PATH_ID *p_audio_cap_ctrl, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret;
	HD_PATH_ID audio_cap_ctrl = 0;
	HD_AUDIOCAP_DEV_CONFIG audio_dev_cfg = {0};
	HD_AUDIOCAP_DRV_CONFIG audio_drv_cfg = {0};
	INT32 prepwr_enable = TRUE;

	ret = hd_audiocap_open(0, HD_AUDIOCAP_0_CTRL, &audio_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	// set audiocap dev parameter
	audio_dev_cfg.in_max.sample_rate = (gUIUvacAudSampleRate[0] == NVT_UI_FREQUENCY_32K) ? HD_AUDIO_SR_32000 : HD_AUDIO_SR_16000;
	audio_dev_cfg.in_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_dev_cfg.in_max.mode = HD_AUDIO_SOUND_MODE_MONO;
	audio_dev_cfg.in_max.frame_sample = 1024;
	audio_dev_cfg.frame_num_max = 10;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DEV_CONFIG, &audio_dev_cfg);
	if (ret != HD_OK) {
		return ret;
	}

	// set audiocap drv parameter
	audio_drv_cfg.mono = HD_AUDIO_MONO_RIGHT;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DRV_CONFIG, &audio_drv_cfg);

	// set PREPWR
	vendor_audiocap_set(audio_cap_ctrl, VENDOR_AUDIOCAP_ITEM_PREPWR_ENABLE, &prepwr_enable);

	*p_audio_cap_ctrl = audio_cap_ctrl;

	return ret;
}

static HD_RESULT set_cap_param2(HD_PATH_ID audio_cap_path, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOCAP_IN audio_cap_param = {0};

	// set audiocap input parameter
	audio_cap_param.sample_rate = (gUIUvacAudSampleRate[0] == NVT_UI_FREQUENCY_32K) ? HD_AUDIO_SR_32000 : HD_AUDIO_SR_16000;
	audio_cap_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cap_param.mode = HD_AUDIO_SOUND_MODE_MONO;
	audio_cap_param.frame_sample = 1024;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_IN, &audio_cap_param);

	return ret;
}

static HD_RESULT open_module2(AUDIO_CAPONLY *p_caponly)
{
	HD_RESULT ret;
	ret = set_cap_cfg2(&p_caponly->cap_ctrl, p_caponly->sample_rate_max);
	if (ret != HD_OK) {
		printf("set cap-cfg fail\n");
		return HD_ERR_NG;
	}
	if((ret = hd_audiocap_open(HD_AUDIOCAP_0_IN_0, HD_AUDIOCAP_0_OUT_0, &p_caponly->cap_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module2(AUDIO_CAPONLY *p_caponly)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_close(p_caponly->cap_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module2(void)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}


THREAD_DECLARE(capture_thread, arg)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIO_FRAME  data_pull;
	int get_phy_flag=0;
	unsigned int uiaud_sample_rate = (gUIUvacAudSampleRate[0] == NVT_UI_FREQUENCY_32K) ? 32000 : 16000;
#if WRITE_BS
	char file_path_main[64], file_path_len[64];
	FILE *f_out_main, *f_out_len;
#endif
	AUDIO_CAPONLY *p_cap_only = (AUDIO_CAPONLY *)arg;
#if UVC_ON
	UVAC_STRM_FRM strmFrm = {0};
#endif
	#define PHY2VIRT_MAIN2(pa) (vir_addr_main2 + (pa - phy_buf_main2.buf_info.phy_addr))

#if WRITE_BS
	/* config pattern name */
	snprintf(file_path_main, sizeof(file_path_main), "/mnt/sd/audio_bs_%d_%d_%d_pcm.dat", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_cap_only->sample_rate);
	snprintf(file_path_len, sizeof(file_path_len), "/mnt/sd/audio_bs_%d_%d_%d_pcm.len", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_cap_only->sample_rate);
#endif

#if WRITE_BS
	/* open output files */
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		printf("open file (%s) fail....\r\n", file_path_main);
	} else {
		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}

	if ((f_out_len = fopen(file_path_len, "wb")) == NULL) {
		printf("open len file (%s) fail....\r\n", file_path_len);
	}
#endif

	/* pull data test */
	while (p_cap_only->cap_exit == 0) {

		if (!flow_audio_start){
			if (get_phy_flag){
				/* mummap for bs buffer */
				//printf("release common buffer2\r\n");
				if (vir_addr_main2)hd_common_mem_munmap((void *)vir_addr_main2, phy_buf_main2.buf_info.buf_size);

				get_phy_flag = 0;
			}
			/* wait flow_audio_start */
			usleep(10);
			continue;
		}else{
			if (!get_phy_flag){

				/* query physical address of bs buffer	(this can ONLY query after hd_audiocap_start() is called !!) */
				hd_audiocap_get(au_cap.cap_ctrl, HD_AUDIOCAP_PARAM_BUFINFO, &phy_buf_main2);

				/* mmap for bs buffer  (just mmap one time only, calculate offset to virtual address later) */
				vir_addr_main2 = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main2.buf_info.phy_addr, phy_buf_main2.buf_info.buf_size);
				if (vir_addr_main2 == 0) {
					printf("mmap error\r\n");
					return 0;
				}

				get_phy_flag = 1;
			}
		}

		// pull data
		ret = hd_audiocap_pull_out_buf(p_cap_only->cap_path, &data_pull, 200); // >1 = timeout mode

		if (ret == HD_OK) {
			UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN2(data_pull.phy_addr[0]);
			UINT32 size = data_pull.size;
			UINT32 timestamp = hd_gettime_ms();
			//printf("audio fram size =%u\r\n",size);
#if UVC_ON
			hd_common_mem_flush_cache((void *)ptr, size);

			strmFrm.path = UVAC_STRM_AUD;
			strmFrm.addr = data_pull.phy_addr[0];
			strmFrm.size = data_pull.size;
			strmFrm.timestamp = data_pull.timestamp;
			UVAC_SetEachStrmInfo(&strmFrm);

			#if UVAC_WAIT_RELEASE
			UVAC_WaitStrmDone(UVAC_STRM_AUD);
			#endif
#endif

#if WRITE_BS
			// write bs
			if (f_out_main) fwrite(ptr, 1, size, f_out_main);
			if (f_out_main) fflush(f_out_main);

			// write bs len
			if (f_out_len) fprintf(f_out_len, "%d %d\n", size, timestamp);
			if (f_out_len) fflush(f_out_len);
#endif
			// release data
			ret = hd_audiocap_release_out_buf(p_cap_only->cap_path, &data_pull);
			if (ret != HD_OK) {
				printf("release buffer failed. ret=%x\r\n", ret);
			}
		}

		//usleep(1000000/uiaud_sample_rate*2);
	}

	if (get_phy_flag){
		/* mummap for bs buffer */
		//printf("release common buffer2\r\n");
		if (vir_addr_main2)hd_common_mem_munmap((void *)vir_addr_main2, phy_buf_main2.buf_info.buf_size);
	}

#if WRITE_BS
	/* close output file */
	if (f_out_main) fclose(f_out_main);
	if (f_out_len) fclose(f_out_len);
#endif
	return 0;
}
#endif // AUDIO_ON
/*AUDIO end*/

#if  UAC_RX_ON
static void *uac_thread(void *arg)
{

#if UVC_ON
	HD_RESULT ret = HD_OK;
	int get_phy_flag=0;
	char file_path_main[64];
	FILE *f_out_main;
	AUDIO_CAPONLY *p_cap_only = (AUDIO_CAPONLY *)arg;
	UVAC_STRM_FRM strmFrm = {0};

	#define PHY2VIRT_MAIN3(pa) (vir_addr_main3 + (pa - uvac_pa))

	/* config pattern name */
	snprintf(file_path_main, sizeof(file_path_main), "/mnt/sd/uac_bs_%d_%d_%d_pcm.dat", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_cap_only->sample_rate);

	/* open output files */
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		printf("open file (%s) fail....\r\n", file_path_main);
	} else {
		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}

	/* pull data test */
	while (p_cap_only->cap_exit == 0) {

		if (!uvac_pa){
			/* wait flow_start */
			usleep(10);
			continue;
		}else{
			if (!get_phy_flag){

				/* mmap for bs buffer  (just mmap one time only, calculate offset to virtual address later) */
				vir_addr_main3 = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, uvac_pa, uvac_size);
				if (vir_addr_main3 == 0) {
					printf("mmap error\r\n");
					return 0;
				}

				get_phy_flag = 1;
			}
		}

		// pull data
		ret = UVAC_PullOutStrm(&strmFrm, 300); // >1 = timeout mode

		if (ret == E_OK) {
			UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN3(strmFrm.addr);
			UINT32 size = strmFrm.size;

			// write bs
			if (f_out_main) fwrite(ptr, 1, size, f_out_main);
			if (f_out_main) fflush(f_out_main);

			// release data
			ret = UVAC_ReleaseOutStrm(&strmFrm);
			if (ret != E_OK) {
				printf("release buffer failed. ret=%x\r\n", ret);
			}
		}
	}

	if (get_phy_flag){
		/* mummap for bs buffer */
		//printf("release common buffer2\r\n");
		if (vir_addr_main3)hd_common_mem_munmap((void *)vir_addr_main3, uvac_size);
	}

	/* close output file */
	if (f_out_main) fclose(f_out_main);
#endif

	return 0;
}
#endif

static HD_RESULT get_cap_caps(HD_PATH_ID video_cap_ctrl, HD_VIDEOCAP_SYSCAPS *p_video_cap_syscaps)
{
	HD_RESULT ret = HD_OK;
	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSCAPS, p_video_cap_syscaps);
	return ret;
}

static HD_RESULT get_cap_sysinfo(HD_PATH_ID video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_SYSINFO sys_info = {0};

	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
	printf("sys_info.devid =0x%X, cur_fps[0]=%d/%d, vd_count=%llu\r\n", sys_info.dev_id, GET_HI_UINT16(sys_info.cur_fps[0]), GET_LO_UINT16(sys_info.cur_fps[0]), sys_info.vd_count);
	return ret;
}

static HD_RESULT set_cap_cfg(HD_PATH_ID *p_video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};
#if (MAX_CAP_SIZE_W == 3840 && MAX_CAP_SIZE_H == 2160)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_sc8238");
#elif (MAX_CAP_SIZE_W == 3264 && MAX_CAP_SIZE_H == 1836)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_gc8034");
#else
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
#endif
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x301;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0|PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;//HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;//HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = VIDEOCAP_ALG_FUNC;
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}

static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim, int fps)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};

		video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
		video_in_param.frc = HD_VIDEO_FRC_RATIO(fps,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		video_in_param.pxlfmt = SEN_OUT_FMT;
		video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN, &video_in_param);
		printf("set_cap_param fps=%d\r\n", fps);
		if (ret != HD_OK) {
			return ret;
		}
	}
	#if 1 //no crop, full frame
	{
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_OFF;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &video_crop_param);
		//printf("set_cap_param CROP NONE=%d\r\n", ret);
	}
	#else //HD_CROP_ON
	{
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_ON;
		video_crop_param.win.rect.x = 0;
		video_crop_param.win.rect.y = 0;
		video_crop_param.win.rect.w = MAX_CAP_SIZE_W/2;
		video_crop_param.win.rect.h= MAX_CAP_SIZE_H/2;
		video_crop_param.align.w = 4;
		video_crop_param.align.h = 4;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &video_crop_param);
		//printf("set_cap_param CROP ON=%d\r\n", ret);
	}
	#endif
	{
		HD_VIDEOCAP_OUT video_out_param = {0};

		//without setting dim for no scaling, using original sensor out size
		video_out_param.pxlfmt = CAP_OUT_FMT;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
	}

	{
		HD_VIDEOCAP_FUNC_CONFIG video_path_param = {0};

		video_path_param.out_func = 0;
		if (g_capbind == 1) //direct mode
			video_path_param.out_func = HD_VIDEOCAP_OUTFUNC_DIRECT;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_FUNC_CONFIG, &video_path_param);
		//printf("set_cap_param PATH_CONFIG=0x%X\r\n", ret);
	}

	return ret;
}

static HD_RESULT set_proc_cfg(HD_PATH_ID *p_video_proc_ctrl, HD_DIM* p_max_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
	HD_PATH_ID video_proc_ctrl = 0;

	ret = hd_videoproc_open(0, HD_VIDEOPROC_0_CTRL, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	if (p_max_dim != NULL ) {
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWALL;
		video_cfg_param.isp_id = 0;
		video_cfg_param.ctrl_max.func = VIDEOPROC_ALG_FUNC;
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;
		video_cfg_param.in_max.pxlfmt = CAP_OUT_FMT;
		video_cfg_param.in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &video_cfg_param);
		if (ret != HD_OK) {
			return HD_ERR_NG;
		}
	}
	{
		HD_VIDEOPROC_FUNC_CONFIG video_path_param = {0};

		video_path_param.in_func = 0;
		if (g_capbind == 1)
			video_path_param.in_func |= HD_VIDEOPROC_INFUNC_DIRECT; //direct NOTE: enable direct

		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
		//printf("set_proc_param PATH_CONFIG=0x%X\r\n", ret);
	}

	*p_video_proc_ctrl = video_proc_ctrl;

	return ret;
}

static HD_RESULT set_proc_param(HD_PATH_ID video_proc_path, HD_DIM* p_dim, UINT32 pull_allow)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};

	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = pull_allow; //set 1 to allow pull

		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}

	HD_VIDEOPROC_FUNC_CONFIG video_path_param = {0};
	video_path_param.out_func = 0;
	if (g_low_latency == 1 && video_proc_path == stream[0].proc_main_path){
		//only the proc_path should be set
		printf("g_low_latency = %d\r\n", g_low_latency);
		video_path_param.out_func |= HD_VIDEOPROC_OUTFUNC_LOWLATENCY;
	}
	if (g_one_buf == 1){
		//ref_path and proc_path need one buffer mode
		printf("g_one_buf = %d\r\n", g_one_buf);
		video_path_param.out_func |= HD_VIDEOPROC_OUTFUNC_ONEBUF;
	}
	ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
	if (ret != HD_OK) {
		return HD_ERR_NG;
	}

	if (stream[0].codec_type == HD_CODEC_TYPE_JPEG){
		UINT32 h_align = 16;
		//to avoid MJPEG might encode read YUV out of range
		ret = vendor_videoproc_set(video_proc_path, VENDOR_VIDEOPROC_PARAM_HEIGHT_ALIGN, &h_align);
		if (ret != HD_OK) {
			printf("VENDOR_VIDEOPROC_PARAM_HEIGHT_ALIGN failed!(%d)\r\n", ret);
		}
	} else {
		UINT32 h_align = 2;
		//to avoid MJPEG might encode read YUV out of range
		ret = vendor_videoproc_set(video_proc_path, VENDOR_VIDEOPROC_PARAM_HEIGHT_ALIGN, &h_align);
		if (ret != HD_OK) {
			printf("VENDOR_VIDEOPROC_PARAM_HEIGHT_ALIGN failed!(%d)\r\n", ret);
		}
	}

	video_ctrl_param.func = VIDEOPROC_ALG_FUNC;
	if (video_proc_path == stream[0].proc_main_path){
		if (p_dim->w == MAX_CAP_SIZE_W && p_dim->h == MAX_CAP_SIZE_H){
			video_ctrl_param.ref_path_3dnr = HD_VIDEOPROC_0_OUT_0;
		} else {
			video_ctrl_param.ref_path_3dnr = HD_VIDEOPROC_0_OUT_4;
		}
		ret = hd_videoproc_set(stream[0].proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);
	}

	return ret;
}

static HD_RESULT set_enc_cfg(HD_PATH_ID video_enc_path, HD_DIM *p_max_dim, UINT32 max_bitrate)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_PATH_CONFIG video_path_config = {0};

	if (p_max_dim != NULL) {

		//--- HD_VIDEOENC_PARAM_PATH_CONFIG ---
		video_path_config.max_mem.codec_type = HD_CODEC_TYPE_H264;
		video_path_config.max_mem.max_dim.w  = p_max_dim->w;
		video_path_config.max_mem.max_dim.h  = p_max_dim->h;
		video_path_config.max_mem.bitrate    = max_bitrate;
		video_path_config.max_mem.enc_buf_ms = 3000;
		video_path_config.max_mem.svc_layer  = HD_SVC_4X;
		video_path_config.max_mem.ltr        = TRUE;
		video_path_config.max_mem.rotate     = FALSE;
		video_path_config.max_mem.source_output   = FALSE;
		video_path_config.isp_id             = 0;
		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_PATH_CONFIG, &video_path_config);
		if (ret != HD_OK) {
			printf("set_enc_path_config = %d\r\n", ret);
			return HD_ERR_NG;
		}
	}
	{
		HD_VIDEOENC_FUNC_CONFIG video_func_config = {0};
			video_func_config.in_func = 0;
		if (g_low_latency == 1)
			video_func_config.in_func |= HD_VIDEOENC_INFUNC_LOWLATENCY;
		else if (g_one_buf == 1)
			video_func_config.in_func |= HD_VIDEOENC_INFUNC_ONEBUF;

		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_FUNC_CONFIG, &video_func_config);
		if (ret != HD_OK) {
			printf("set_enc_path_config = %d\r\n", ret);
			return HD_ERR_NG;
		}
	}
	return ret;
}

static HD_RESULT set_enc_param(HD_PATH_ID video_enc_path, HD_DIM *p_dim, UINT32 enc_type, UINT32 bitrate)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_IN  video_in_param = {0};
	HD_VIDEOENC_OUT video_out_param = {0};
	HD_VIDEOENC_OUT2 video_out2_param = {0};
	HD_H26XENC_RATE_CONTROL rc_param = {0};

	#if MULTI_STREAM
	{
		VENDOR_VIDEOENC_BS_RESERVED_SIZE_CFG bs_reserved_cfg = {0};

		bs_reserved_cfg.reserved_size = sizeof(STREAM_USER_DATA);
		vendor_videoenc_set(video_enc_path, VENDOR_VIDEOENC_PARAM_OUT_BS_RESERVED_SIZE, &bs_reserved_cfg);
	}
	#endif

	if (p_dim != NULL) {

		//--- HD_VIDEOENC_PARAM_IN ---
		video_in_param.dir           = HD_VIDEO_DIR_NONE;
		video_in_param.pxl_fmt = HD_VIDEO_PXLFMT_YUV420;
		video_in_param.dim.w   = p_dim->w;
		video_in_param.dim.h   = p_dim->h;
		video_in_param.frc     = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_IN, &video_in_param);
		if (ret != HD_OK) {
			printf("set_enc_param_in = %d\r\n", ret);
			return ret;
		}

		if (enc_type == HD_CODEC_TYPE_H265) {
			//printf("enc_type = HD_CODEC_TYPE_H265\r\n");
			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_H265;
			video_out_param.h26x.profile       = HD_H265E_MAIN_PROFILE;
			video_out_param.h26x.level_idc     = HD_H265E_LEVEL_5;
			video_out_param.h26x.gop_num       = 15;
			video_out_param.h26x.ltr_interval  = 0;
			video_out_param.h26x.ltr_pre_ref   = 0;
			video_out_param.h26x.gray_en       = 0;
			video_out_param.h26x.source_output = 0;
			video_out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			video_out_param.h26x.entropy_mode  = HD_H265E_CABAC_CODING;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}

			//--- HD_VIDEOENC_PARAM_OUT_RATE_CONTROL ---
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = bitrate;
			rc_param.cbr.frame_rate_base = 30;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control = %d\r\n", ret);
				return ret;
			}
		} else if (enc_type == HD_CODEC_TYPE_H264) {
			//printf("enc_type = HD_CODEC_TYPE_H264\r\n");
			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_H264;
			video_out_param.h26x.profile       = HD_H264E_HIGH_PROFILE;
			video_out_param.h26x.level_idc     = HD_H264E_LEVEL_5_1;
			video_out_param.h26x.gop_num       = 15;
			video_out_param.h26x.ltr_interval  = 0;
			video_out_param.h26x.ltr_pre_ref   = 0;
			video_out_param.h26x.gray_en       = 0;
			video_out_param.h26x.source_output = 0;
			video_out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			video_out_param.h26x.entropy_mode  = HD_H264E_CABAC_CODING;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}

			//--- HD_VIDEOENC_PARAM_OUT_RATE_CONTROL ---
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = bitrate;
			rc_param.cbr.frame_rate_base = 30;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control = %d\r\n", ret);
				return ret;
			}

		} else if (enc_type == HD_CODEC_TYPE_JPEG) {
			//printf("enc_type = HD_CODEC_TYPE_JPEG\r\n");
			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			#if 0
			video_out_param.codec_type         = HD_CODEC_TYPE_JPEG;
			video_out_param.jpeg.retstart_interval = 0;
			if (p_dim->w>2560 && p_dim->h>1440){
				video_out_param.jpeg.image_quality = 50;
			} else {
				video_out_param.jpeg.image_quality = 90;
			}
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			//--- HD_VIDEOENC_PARAM_OUT_RATE_CONTROL ---
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = bitrate;
			rc_param.cbr.frame_rate_base = 30;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			#else
			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out2_param.codec_type         = HD_CODEC_TYPE_JPEG;
			video_out2_param.jpeg.retstart_interval = 0;
			video_out2_param.jpeg.image_quality = 50;
			video_out2_param.jpeg.bitrate = bitrate;
			if (p_dim->w == 3840 || p_dim->w == 3264) {
				video_out2_param.jpeg.frame_rate_base = 25;
			} else {
				video_out2_param.jpeg.frame_rate_base = 30;
			}
			video_out2_param.jpeg.frame_rate_incr = 1;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &video_out2_param);

			VENDOR_VIDEOENC_JPG_RC_CFG jpg_rc_param = {0};
			jpg_rc_param.vbr_mode_en = 0;  // 0: CBR, 1: VBR
			jpg_rc_param.min_quality = 30; // min quality
			jpg_rc_param.max_quality = 70; // max quality
			vendor_videoenc_set(video_enc_path, VENDOR_VIDEOENC_PARAM_OUT_JPG_RC, &jpg_rc_param);
			#endif
			//--- HD_VIDEOENC_PARAM_OUT_RATE_CONTROL ---
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = bitrate;
			rc_param.cbr.frame_rate_base = 30;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}

			//MJPG YUV trans(YUV420 to YUV422)
      #if 0
			VENDOR_VIDEOENC_JPG_YUV_TRANS_CFG yuv_trans_cfg = {0};
			yuv_trans_cfg.jpg_yuv_trans_en = JPG_YUV_TRANS;
			//printf("MJPG YUV TRANS(%d)\r\n", JPG_YUV_TRANS);
			vendor_videoenc_set(video_enc_path, VENDOR_VIDEOENC_PARAM_OUT_JPG_YUV_TRANS, &yuv_trans_cfg);
      #endif

		} else {

			printf("not support enc_type\r\n");
			return HD_ERR_NG;
		}
	}

	return ret;
}

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
    if ((ret = hd_videoenc_init()) != HD_OK)
		return ret;
#if AUDIO_ON
	if((ret = hd_audiocap_init()) != HD_OK)
        return ret;
#endif
/*
#if ALG_FD_PD
	if ((ret = hd_videoout_init()) != HD_OK)
		return ret;
#endif
*/
	return HD_OK;
}

static HD_RESULT open_module(VIDEO_STREAM *p_stream, HD_DIM* p_proc_max_dim)
{
	HD_RESULT ret;
	// set videocap config
	ret = set_cap_cfg(&p_stream->cap_ctrl);
	if (ret != HD_OK) {
		printf("set cap-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoproc config
	ret = set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim);
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
#if ALG_FD_PD
	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_main_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_1, &p_stream->proc_alg_path)) != HD_OK)
		return ret;

#if NN_FDCNN_FD_DRAW
	//open a mask in videoout
	if((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_MASK_0, &p_stream->mask_path0)) != HD_OK)
		return ret;
    if((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_MASK_EX_1, &p_stream->mask_path1)) != HD_OK)
		return ret;
    if((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_MASK_2, &p_stream->mask_path2)) != HD_OK)
		return ret;
    if((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_MASK_EX_3, &p_stream->mask_path3)) != HD_OK)
		return ret;
#endif

#if NN_PVDCNN_DRAW
    if((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_MASK_4, &p_stream->mask_path4)) != HD_OK)
		return ret;
    if((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_MASK_EX_5, &p_stream->mask_path5)) != HD_OK)
		return ret;
    if((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_MASK_6, &p_stream->mask_path6)) != HD_OK)
		return ret;
    if((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_MASK_EX_7, &p_stream->mask_path7)) != HD_OK)
		return ret;
#endif

#else //ALG_FD_PD
	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_main_path)) != HD_OK)
		return ret;
	#if MULTI_STREAM
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_1, &p_stream->proc_sub1_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_2, &p_stream->proc_sub2_path)) != HD_OK)
		return ret;
	#endif
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_4, &p_stream->ref_path)) != HD_OK)
		return ret;
#endif //ALG_FD_PD
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_0, HD_VIDEOENC_0_OUT_0, &p_stream->enc_main_path)) != HD_OK)
		return ret;
	#if MULTI_STREAM
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_1, HD_VIDEOENC_0_OUT_1, &p_stream->enc_sub1_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_2, HD_VIDEOENC_0_OUT_2, &p_stream->enc_sub2_path)) != HD_OK)
		return ret;
	#endif
	return HD_OK;
}

#if ALG_FD_PD
static HD_RESULT open_module_extend1(VIDEO_STREAM *p_stream, HD_DIM* p_proc_max_dim)
{
	HD_RESULT ret;
	// set videoout config
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, EXTEND_PATH1, &p_stream->proc_alg_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module_extend2(VIDEO_STREAM *p_stream, HD_DIM* p_proc_max_dim)
{
	HD_RESULT ret;
	// set videoout config
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, EXTEND_PATH2, &p_stream->proc_alg_path)) != HD_OK)
		return ret;
	return HD_OK;
}
#endif //ALG_FD_PD

static HD_RESULT close_module(VIDEO_STREAM *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_close(p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc_main_path)) != HD_OK)
		return ret;
#if MULTI_STREAM
	if ((ret = hd_videoproc_close(p_stream->proc_sub1_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc_sub2_path)) != HD_OK)
		return ret;
#endif
	if ((ret = hd_videoproc_close(p_stream->ref_path)) != HD_OK)
		return ret;

#if ALG_FD_PD

#if NN_FDCNN_FD_DRAW
    if ((ret = hd_videoproc_close(p_stream->mask_path0)) != HD_OK)
		return ret;
    if ((ret = hd_videoproc_close(p_stream->mask_path1)) != HD_OK)
		return ret;
    if ((ret = hd_videoproc_close(p_stream->mask_path2)) != HD_OK)
		return ret;
    if ((ret = hd_videoproc_close(p_stream->mask_path3)) != HD_OK)
		return ret;
#endif
#if NN_PVDCNN_DRAW
    if ((ret = hd_videoproc_close(p_stream->mask_path4)) != HD_OK)
		return ret;
    if ((ret = hd_videoproc_close(p_stream->mask_path5)) != HD_OK)
		return ret;
    if ((ret = hd_videoproc_close(p_stream->mask_path6)) != HD_OK)
		return ret;
    if ((ret = hd_videoproc_close(p_stream->mask_path7)) != HD_OK)
		return ret;
#endif

#endif//ALG_FD_PD
	if ((ret = hd_videoenc_close(p_stream->enc_main_path)) != HD_OK)
		return ret;
#if MULTI_STREAM
	if ((ret = hd_videoenc_close(p_stream->enc_sub1_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_sub2_path)) != HD_OK)
		return ret;
#endif
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_uninit()) != HD_OK)
		return ret;

	return HD_OK;
}

#if UVC_SUPPORT_YUV_FMT

static HD_RESULT yuv_transform(UINT32 buf_va, UINT32 buf_size, HD_DIM *main_dim)
{
	int                       fd;
	HD_GFX_COLOR_TRANSFORM    param;
	void                      *va;
	HD_RESULT                 ret;
	HD_VIDEO_FRAME            frame;
	UINT32                    src_size, dst_size, temp_size;
	int                       len;
	int                       w = 0, h = 0;
	UINT32                    src_pa, src_va, dst_pa, dst_va, temp_pa;
	static UINT32                    gfx_qcnt = 0;

#if UVC_ON
	UVAC_STRM_FRM      strmFrm = {0};
#endif

	w = main_dim->w;
	h = main_dim->h;
	//calculate yuv420 image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = h;
	frame.loff[0] = w;
	frame.loff[1] = w;
	src_size = hd_common_mem_calc_buf_size(&frame);
	if(!src_size){
		printf("hd_common_mem_calc_buf_size() fail to calculate src size\n");
		return -1;
	}

	//calculate yuyv image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV422_ONE;
	frame.dim.h   = h;
	frame.loff[0] = w;
	dst_size = hd_common_mem_calc_buf_size(&frame);
	if(!dst_size){
		printf("hd_common_mem_calc_buf_size() fail to calculate yuv size\n");
		return -1;
	}
	temp_size = dst_size;

	if((src_size + dst_size + temp_size) > gfx[gfx_qcnt].buf_size){
		printf("required size(%d) > allocated size(%d)\n", (src_size + dst_size + temp_size), gfx[gfx_qcnt].buf_size);
		return -1;
	}

	src_pa  = gfx[gfx_qcnt].buf_pa;
	dst_pa  = src_pa + src_size;
	temp_pa = dst_pa + dst_size;

	// cpoy video stream to gfx buffer
	if (gfx[gfx_qcnt].buf_va){
		memcpy((void*)gfx[gfx_qcnt].buf_va,(const void *)buf_va, src_size);//gfx[gfx_qcnt].buf_size);
	} else {
		printf("gfx[%d].buf_va NULL!\n, gfx_qcnt");
		return -1;
	}

	src_va  = gfx[gfx_qcnt].buf_va;
	dst_va  = src_va + src_size;

	//use gfx engine to transform yuv420 image to yuyv
	memset(&param, 0, sizeof(HD_GFX_COLOR_TRANSFORM));
	param.src_img.dim.w            = w;
	param.src_img.dim.h            = h;
	param.src_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.src_img.p_phy_addr[0]    = src_pa;
	param.src_img.p_phy_addr[1]    = src_pa + w*h;
	param.src_img.lineoffset[0]    = w;
	param.src_img.lineoffset[1]    = w;
	param.dst_img.dim.w            = w;
	param.dst_img.dim.h            = h;
	param.dst_img.format           = HD_VIDEO_PXLFMT_YUV422_ONE;
	param.dst_img.p_phy_addr[0]    = dst_pa;
	param.dst_img.lineoffset[0]    = w * 2;
	param.p_tmp_buf                = temp_pa;
	param.tmp_buf_size             = temp_size;

	ret = hd_gfx_color_transform(&param);
	if(ret != HD_OK){
		printf("hd_gfx_color_transform fail=%d\n", ret);
		goto exit;
	}

#if UVC_ON
	strmFrm.path = UVAC_STRM_VID ;
	strmFrm.addr = dst_pa;
	strmFrm.size = dst_size;
	strmFrm.pStrmHdr = 0;
	strmFrm.strmHdrSize = 0;
	strmFrm.va = dst_va;
	if (stream[0].codec_type == HD_CODEC_TYPE_RAW){
		UVAC_SetEachStrmInfo(&strmFrm);

		#if UVAC_WAIT_RELEASE
		UVAC_WaitStrmDone(UVAC_STRM_VID);
		#endif
	}
	gfx_qcnt++;
	if (gfx_qcnt >= GFX_QUEUE_MAX_NUM)
		gfx_qcnt = 0; // This state machine does not consider queue overwrite issue.
#endif
	ret = HD_OK;

exit:
	return ret;
}

THREAD_DECLARE(acquire_yuv_thread, arg)
{
	VIDEO_STREAM* p_stream0 = (VIDEO_STREAM *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEO_FRAME video_frame = {0};
	UINT32 phy_addr_main, vir_addr_main;
	UINT32 yuv_size;

	HD_DIM main_dim;
	main_dim.w = MAX_YUV_W;
	main_dim.h = MAX_YUV_H;

	#define PHY2VIRT_MAIN_YUV(pa) (vir_addr_main + ((pa) - phy_addr_main))

	//--------- pull data test ---------
	while (!p_stream0->acquire_exit) {
		if(acquire_start) {
			main_dim.w = stream[0].enc_main_dim.w;
			main_dim.h = stream[0].enc_main_dim.h;
			ret = hd_videoproc_pull_out_buf(p_stream0->proc_main_path, &video_frame, 100); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
			if (ret != HD_OK) {
				printf("pull_out(100) error = %d!!\r\n", ret);
				goto skip;
			}

			phy_addr_main = hd_common_mem_blk2pa(video_frame.blk); // Get physical addr
			if (phy_addr_main == 0) {
				printf("blk2pa fail, blk = 0x%x\r\n", video_frame.blk);
				goto skip;
			}

			yuv_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(main_dim.w, main_dim.h, HD_VIDEO_PXLFMT_YUV422);

			// mmap for frame buffer (just mmap one time only, calculate offset to virtual address later)
			//printf("phy_addr_main: 0x%x, 0x%x\n", phy_addr_main, yuv_size);
			vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_addr_main, yuv_size);
			if (vir_addr_main == 0) {
				printf("mmap error !!\r\n\r\n");
				goto skip;
			}

			UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN_YUV(video_frame.phy_addr[0]);
			UINT32 len = video_frame.loff[0]*video_frame.ph[0] + video_frame.loff[1]*video_frame.ph[1];
			yuv_transform((UINT32)ptr, len, &main_dim);

			// mummap for frame buffer
			ret = hd_common_mem_munmap((void *)vir_addr_main, yuv_size);
			if (ret != HD_OK) {
				printf("mnumap error !!\r\n\r\n");
				goto skip;
			}

			ret = hd_videoproc_release_out_buf(p_stream0->proc_main_path, &video_frame);
			if (ret != HD_OK) {
				printf("release_out() error !!\r\n\r\n");
				goto skip;
			}
		}
skip:
		usleep(10000); //delay 1 ms
	}
	return 0;
}
#endif // UVC_SUPPORT_YUV_FMT

THREAD_DECLARE(encode_thread, arg)
{

	VIDEO_STREAM* p_stream0 = (VIDEO_STREAM *)arg;
	HD_RESULT ret_main = HD_OK;
	HD_VIDEOENC_BS  data_pull_main;
#if MULTI_STREAM
	HD_RESULT ret_sub1 = HD_OK, ret_sub2 = HD_OK;
	HD_VIDEOENC_BS  data_pull_sub1, data_pull_sub2;
#endif
	UINT32 j;
	int get_phy_flag=0;

#if WRITE_BS
	char file_path_main[32] = "/mnt/sd/dump_bs_main.dat";
	FILE *f_out_main;
#endif
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))
#if MULTI_STREAM
	#define PHY2VIRT_SUB1(pa) (vir_addr_sub1 + (pa - phy_buf_sub1.buf_info.phy_addr))
	#define PHY2VIRT_SUB2(pa) (vir_addr_sub2 + (pa - phy_buf_sub2.buf_info.phy_addr))
#endif

#if UVC_ON
	UVAC_STRM_FRM      strmFrm = {0};
	UINT32             loop =0;
	UINT32             uvacNeedMem=0;
	UINT32             ChannelNum = 1;
#endif
	//UINT32 temp=1;
	UINT8 main_seq = 0, sub1_seq = 0, sub2_seq = 0;
#if WRITE_BS
	//----- open output files -----
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		HD_VIDEOENC_ERR("open file (%s) fail....\r\n", file_path_main);
	} else {
		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}
#endif

	//--------- pull data test ---------
	while (p_stream0->enc_exit == 0) {
		if (!get_phy_flag){
			get_phy_flag = 1;
			// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
			ret_main = hd_videoenc_get(stream[0].enc_main_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);
			#if MULTI_STREAM
			if (p_stream0->codec_type == HD_CODEC_TYPE_H264){
				ret_sub1 = hd_videoenc_get(stream[0].enc_sub1_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_sub1);
				ret_sub2 = hd_videoenc_get(stream[0].enc_sub2_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_sub2);
			}
			printf("++++phy_buf_main=0x%08X, phy_buf_sub1=0x%08X, phy_buf_sub2=0x%08X\n", phy_buf_main.buf_info.phy_addr, phy_buf_sub1.buf_info.phy_addr, phy_buf_sub2.buf_info.phy_addr);
			#endif
			// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
			vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);
			#if MULTI_STREAM
			if (p_stream0->codec_type == HD_CODEC_TYPE_H264){
				vir_addr_sub1 = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_sub1.buf_info.phy_addr, phy_buf_sub1.buf_info.buf_size);
				vir_addr_sub2 = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_sub2.buf_info.phy_addr, phy_buf_sub2.buf_info.buf_size);
			}
			printf("++++vir_addr_main=0x%08X, vir_addr_sub1=0x%08X, vir_addr_sub2=0x%08X\n", vir_addr_main, vir_addr_sub1, vir_addr_sub2);
			#endif
		}

		if (!encode_start){
			//------ wait encode_start ------
			usleep(10);
			continue;
		}

		//pull data
		ret_main = hd_videoenc_pull_out_buf(p_stream0->enc_main_path, &data_pull_main, -1); // -1 = blocking mode

		{
			//UINT64 cur_time = hd_gettime_us();

			//printf("vd = %lld, encout = %lld, diff = %lld\r\n", data_pull.timestamp, cur_time, cur_time - data_pull.timestamp);
		}

		if (ret_main == HD_OK) {
#if UVC_ON
			strmFrm.path = UVAC_STRM_VID ;
			strmFrm.addr = data_pull_main.video_pack[data_pull_main.pack_num-1].phy_addr;
			strmFrm.size = data_pull_main.video_pack[data_pull_main.pack_num-1].size;
			strmFrm.pStrmHdr = 0;
			strmFrm.strmHdrSize = 0;
			strmFrm.timestamp = data_pull_main.timestamp;
			if (p_stream0->codec_type == HD_CODEC_TYPE_H264){
				if (data_pull_main.pack_num > 1) {
					UINT32 header_size = 0;
					//if strmFrm.addr contains SPS/PPS, there is no need of pStrmHdr
					for (loop = 0; loop < data_pull_main.pack_num - 1; loop ++) {
						header_size += data_pull_main.video_pack[loop].size;
					}
					strmFrm.size += header_size;
					strmFrm.addr -= header_size;
				}
				#if MULTI_STREAM
				if (multi_stream_func) {
					STREAM_USER_DATA *p_user;
					UINT32 user_size = sizeof(STREAM_USER_DATA);

					p_user = (STREAM_USER_DATA *)PHY2VIRT_MAIN(strmFrm.addr - user_size);
					p_user->magic = MAKEFOURCC('S','T','R','M');
					p_user->streamId = 1;
					p_user->seq = main_seq;
					main_seq++;
					p_user->timestamp = (UINT32)(data_pull_main.timestamp/1000);
					p_user->dataLen = strmFrm.size;

					strmFrm.addr -= user_size;
					strmFrm.size += user_size;
				}
				#endif
			}
			strmFrm.va = PHY2VIRT_MAIN(strmFrm.addr);
			//printf("++++va_main=0x%08X\n", strmFrm.va);
			if (stream[0].codec_type != HD_CODEC_TYPE_RAW){
				UVAC_SetEachStrmInfo(&strmFrm);

				#if UVAC_WAIT_RELEASE
				UVAC_WaitStrmDone(UVAC_STRM_VID);
				#endif
			}
			/*timstamp for test
			if (temp){
				system("echo \"uvc\" > /proc/nvt_info/bootts");
				temp=0;
			}
			*/
#endif

#if WRITE_BS
			for (j=0; j< data_pull_main.pack_num; j++) {
				UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull_main.video_pack[j].phy_addr);
				UINT32 len = data_pull_main.video_pack[j].size;
				if (f_out_main) fwrite(ptr, 1, len, f_out_main);
				if (f_out_main) fflush(f_out_main);
			}

#endif

			// release data
			ret_main = hd_videoenc_release_out_buf(p_stream0->enc_main_path, &data_pull_main);
			if (ret_main != HD_OK) {
				printf("enc_release main error=%d !!\r\n", ret_main);
			}
		}

		#if MULTI_STREAM
		if (multi_stream_func == TRUE && p_stream0->codec_type == HD_CODEC_TYPE_H264){
			//pull data
			ret_sub1 = hd_videoenc_pull_out_buf(p_stream0->enc_sub1_path, &data_pull_sub1, -1); // -1 = blocking mode
			if (ret_sub1 == HD_OK) {
				strmFrm.path = UVAC_STRM_VID ;
				strmFrm.addr = data_pull_sub1.video_pack[data_pull_sub1.pack_num-1].phy_addr;
				strmFrm.size = data_pull_sub1.video_pack[data_pull_sub1.pack_num-1].size;
				strmFrm.pStrmHdr = 0;
				strmFrm.strmHdrSize = 0;
				strmFrm.timestamp = data_pull_sub1.timestamp;
				if (p_stream0->codec_type == HD_CODEC_TYPE_H264){
					if (data_pull_sub1.pack_num > 1) {
						//printf("I-frame, data_pull_sub1.pack_num =%u\r\n", data_pull_sub1.pack_num);
						UINT32 header_size = 0;
						//if strmFrm.addr contains SPS/PPS, there is no need of pStrmHdr
						for (loop = 0; loop < data_pull_sub1.pack_num - 1; loop ++) {
							header_size += data_pull_sub1.video_pack[loop].size;
						}
						strmFrm.size += header_size;
						strmFrm.addr -= header_size;
					}
					STREAM_USER_DATA *p_user;
					UINT32 user_size = sizeof(STREAM_USER_DATA);

					p_user = (STREAM_USER_DATA *)PHY2VIRT_SUB1(strmFrm.addr - user_size);
					p_user->magic = MAKEFOURCC('S','T','R','M');
					p_user->streamId = 2;
					p_user->seq = sub1_seq;
					sub1_seq++;
					p_user->timestamp = (UINT32)(data_pull_sub1.timestamp/1000);
					p_user->dataLen = strmFrm.size;

					strmFrm.addr -= user_size;
					strmFrm.size += user_size;
				}
				strmFrm.va = PHY2VIRT_SUB1(strmFrm.addr);
				//printf("++++va_sub1=0x%08X\n", strmFrm.va);
				if (stream[0].codec_type != HD_CODEC_TYPE_RAW){
					UVAC_SetEachStrmInfo(&strmFrm);

					#if UVAC_WAIT_RELEASE
					UVAC_WaitStrmDone(UVAC_STRM_VID);
					#endif
				}
				/*timstamp for test
				if (temp){
					system("echo \"uvc\" > /proc/nvt_info/bootts");
					temp=0;
				}
				*/

				// release data
				ret_sub1 = hd_videoenc_release_out_buf(p_stream0->enc_sub1_path, &data_pull_sub1);
				if (ret_sub1 != HD_OK) {
					printf("enc_release sub1 error=%d !!\r\n", ret_sub1);
				}
			}
		}

		if (multi_stream_func == TRUE && p_stream0->codec_type == HD_CODEC_TYPE_H264) {
			//pull data
			ret_sub2 = hd_videoenc_pull_out_buf(p_stream0->enc_sub2_path, &data_pull_sub2, -1); // -1 = blocking mode
			if (ret_sub2 == HD_OK){
				strmFrm.path = UVAC_STRM_VID ;
				strmFrm.addr = data_pull_sub2.video_pack[data_pull_sub2.pack_num-1].phy_addr;
				strmFrm.size = data_pull_sub2.video_pack[data_pull_sub2.pack_num-1].size;
				strmFrm.pStrmHdr = 0;
				strmFrm.strmHdrSize = 0;
				strmFrm.timestamp = data_pull_sub2.timestamp;
				if (p_stream0->codec_type == HD_CODEC_TYPE_H264){
					if (data_pull_sub2.pack_num > 1) {
						UINT32 header_size = 0;
						//printf("I-frame, data_pull_sub2.pack_num =%u\r\n", data_pull_sub2.pack_num);
						//if user data is used, the strmFrm.addr of I-frame should contain SPS/PPS hence there is no need of pStrmHdr
						for (loop = 0; loop < data_pull_sub2.pack_num - 1; loop ++) {
							header_size += data_pull_sub2.video_pack[loop].size;
						}
						strmFrm.size += header_size;
						strmFrm.addr -= header_size;
					}
					STREAM_USER_DATA *p_user;
					UINT32 user_size = sizeof(STREAM_USER_DATA);

					p_user = (STREAM_USER_DATA *)PHY2VIRT_SUB2(strmFrm.addr - user_size);
					p_user->magic = MAKEFOURCC('S','T','R','M');
					p_user->streamId = 4;
					p_user->seq = sub2_seq;
					sub2_seq++;
					p_user->timestamp = (UINT32)(data_pull_sub2.timestamp/1000);
					p_user->dataLen = strmFrm.size;

					strmFrm.addr -= user_size;
					strmFrm.size += user_size;
				}
				strmFrm.va = PHY2VIRT_SUB2(strmFrm.addr);
				//printf("++++va_sub2=0x%08X\n", strmFrm.va);
				if (stream[0].codec_type != HD_CODEC_TYPE_RAW){
					UVAC_SetEachStrmInfo(&strmFrm);

					#if UVAC_WAIT_RELEASE
					UVAC_WaitStrmDone(UVAC_STRM_VID);
					#endif
				}
				/*timstamp for test
				if (temp){
					system("echo \"uvc\" > /proc/nvt_info/bootts");
					temp=0;
				}
				*/

				// release data
				ret_sub2 = hd_videoenc_release_out_buf(p_stream0->enc_sub2_path, &data_pull_sub2);
				if (ret_sub2 != HD_OK) {
					printf("enc_release sub2 error=%d !!\r\n", ret_sub2);
				}
			}
		}
		#endif
	}
	if (get_phy_flag){
		// mummap for bs buffer
		//printf("release common buffer\r\n");
		if (vir_addr_main) hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
		#if MULTI_STREAM
		if (vir_addr_sub1) hd_common_mem_munmap((void *)vir_addr_sub1, phy_buf_sub1.buf_info.buf_size);
		if (vir_addr_sub2) hd_common_mem_munmap((void *)vir_addr_sub2, phy_buf_sub2.buf_info.buf_size);
		#endif
	}

#if WRITE_BS
	// close output file
	if (f_out_main) fclose(f_out_main);
#endif

	return 0;
}

#if UVC_ON
_ALIGNED(4) static UINT16 m_UVACSerialStrDesc3[] = {
	0x0320,                             // 20: size of String Descriptor = 32 bytes
	// 03: String Descriptor type
	'5', '1', '0', '5', '5',            // 96611-00000-001 (default)
	'0', '0', '0', '0', '0',
	'0', '0', '1', '0', '0'
};
_ALIGNED(4) const static UINT8 m_UVACManuStrDesc[] = {
	USB_VENDER_DESC_STRING_LEN * 2 + 2, // size of String Descriptor = 6 bytes
	0x03,                       // 03: String Descriptor type
	USB_VENDER_DESC_STRING
};

_ALIGNED(4) const static UINT8 m_UVACProdStrDesc[] = {
	USB_PRODUCT_DESC_STRING_LEN * 2 + 2, // size of String Descriptor = 6 bytes
	0x03,                       // 03: String Descriptor type
	USB_PRODUCT_DESC_STRING
};

#if HID_FUNC
#include "hid_def.h"

#define HID_READ_BUFFER_POOL     HD_COMMON_MEM_USER_DEFINIED_POOL
#define HID_WRITE_BUFFER_POOL	(HID_READ_BUFFER_POOL+1)

#define HID_READ_BUFFER_SIZE    (512*1024)
#define HID_WRITE_BUFFER_SIZE	(512*1024)

typedef struct _HID_MEM_PARM {
	UINT32 pa;
	UINT32 va;
	UINT32 size;
} HID_MEM_PARM;

static HID_MEM_PARM hid_read_buf = {0};
static HID_MEM_PARM hid_write_buf = {0};

//just a sample for report descriptor
static UINT8 report_desc[] = {
	HID_USAGE_PAGE_16(VENDOR_USAGE_PAGE_CFU_TLC&0xFF, VENDOR_USAGE_PAGE_CFU_TLC>>8),
	HID_USAGE_16(VENDOR_USAGE_CFU&0xFF, VENDOR_USAGE_CFU>>8),
	HID_COLLECTION(APPLICATION),
		HID_USAGE_8(VERSION_FEATURE_USAGE),
		HID_REPORT_ID(CFU_GET_REATURE_REPORT_ID),
		HID_LOGICAL_MIN_8(0x00),//a dummy item for CV test
		HID_LOGICAL_MAX_8(0x7F),//a dummy item for CV test
		HID_REPORT_SIZE(8),
		HID_REPORT_COUNT(sizeof(CFU_FW_VERSION)),
		HID_FEATURE_8(Data_Var_Abs),

		HID_USAGE_8(CONTENT_OUTPUT_USAGE),
		HID_REPORT_ID(CFU_UPDATE_CONTENT_REPORT_ID),
		HID_OUTPUT_16(Data_Var_Abs, BuffBytes),
		HID_USAGE_8(CONTENT_RESPONSE_INPUT_USAGE),
		HID_REPORT_SIZE(8),
		HID_REPORT_COUNT(sizeof(CFU_UPDATE_CONTENT_RESP)),
		HID_INPUT_8(Data_Var_Abs),

		HID_USAGE_8(OFFER_OUTPUT_USAGE),
		HID_REPORT_ID(CFU_UPDATE_OFFER_REPORT_ID),
		HID_REPORT_SIZE(8),
		HID_REPORT_COUNT(sizeof(CFU_UPDATE_OFFER)),
		HID_OUTPUT_8(Data_Var_Abs),

		HID_USAGE_8(OFFER_RESPONSE_INPUT_USAGE),
		HID_REPORT_SIZE(8),
		HID_REPORT_COUNT(sizeof(CFU_UPDATE_OFFER_RESP)),
		HID_INPUT_8(Data_Var_Abs),
	HID_END_COLLECTION(),
	HID_USAGE_PAGE_16(VENDOR_USAGE_PAGE_VC_TLC&0xFF, VENDOR_USAGE_PAGE_VC_TLC>>8),
	HID_USAGE_16(VENDOR_USAGE_VC&0xFF, VENDOR_USAGE_VC>>8),
	HID_COLLECTION(APPLICATION),
		HID_USAGE_8(VC_REQUEST_FEATURE_USAGE),
		HID_REPORT_ID(VC_REQUEST_REPORT_ID),
		HID_FEATURE_16(Data_Var_Abs, BuffBytes),

		HID_USAGE_8(VC_RESPONSE_INPUT_USAGE),
		HID_REPORT_ID(VC_RESPONSE_REPORT_ID),
		HID_INPUT_8(Data_Var_Abs),
	HID_END_COLLECTION()
};
static BOOL UvacHidCB(UINT8 request, UINT16 value, UINT8 *p_data, UINT32 *p_dataLen)
{
	BOOL ret = TRUE;
	UINT8 report_type, report_id;

	report_type = value >> 8;
	report_id = value & 0xFF;

	printf("%s:request = 0x%X, ReportType=0x%X, ReportID=0x%X\r\n", __func__, request, report_type, report_id);

	//just a sample
	switch(request)
	{
		case SET_REPORT:
	    case SET_IDLE:
	    case SET_PROTOCOL:
	        ret = TRUE;
            break;
		case GET_REPORT:
		default:
			ret = FALSE;
			break;
	}
	return ret;
}
#endif


static void USBMakerInit_UVAC(UVAC_VEND_DEV_DESC *pUVACDevDesc)
{

	pUVACDevDesc->pManuStringDesc = (UVAC_STRING_DESC *)m_UVACManuStrDesc;
	pUVACDevDesc->pProdStringDesc = (UVAC_STRING_DESC *)m_UVACProdStrDesc;

	pUVACDevDesc->pSerialStringDesc = (UVAC_STRING_DESC *)m_UVACSerialStrDesc3;

	pUVACDevDesc->VID = USB_VID;
	pUVACDevDesc->PID = USB_PID_PCCAM;

}

static UINT32 xUvacStartVideoCB(UVAC_VID_DEV_CNT vidDevIdx, UVAC_STRM_INFO *pStrmInfo)
{
	HD_DIM main_dim;
	int ret=0;
	int fps;
	int bitrate;

	if (pStrmInfo) {
		printf("UVAC Start[%d] resoIdx=%d,W=%d,H=%d,codec=%d,fps=%d,path=%d,tbr=0x%x\r\n", vidDevIdx, pStrmInfo->strmResoIdx, pStrmInfo->strmWidth, pStrmInfo->strmHeight, pStrmInfo->strmCodec, pStrmInfo->strmFps, pStrmInfo->strmPath, pStrmInfo->strmTBR);

		//stop poll data
		pthread_mutex_lock(&flow_start_lock);
		encode_start = 0;
		acquire_start = 0;
		ai_start = 0;
		pthread_mutex_unlock(&flow_start_lock);

		// stop VIDEO_STREAM modules (main)
		if (g_capbind == 1) {
			hd_videoproc_stop(stream[0].proc_main_path);
#if MULTI_STREAM
			if (stream[0].codec_type == HD_CODEC_TYPE_H264){
				hd_videoproc_stop(stream[0].proc_sub1_path);
				hd_videoproc_stop(stream[0].proc_sub2_path);
			}
#endif
			if (stream[0].enc_main_dim.w != MAX_CAP_SIZE_W || stream[0].enc_main_dim.h != MAX_CAP_SIZE_H){
				hd_videoproc_stop(stream[0].ref_path);
			}
			hd_videocap_stop(stream[0].cap_path);
		} else {
			hd_videocap_stop(stream[0].cap_path);
			hd_videoproc_stop(stream[0].proc_main_path);
#if MULTI_STREAM
			if (stream[0].codec_type == HD_CODEC_TYPE_H264){
				hd_videoproc_stop(stream[0].proc_sub1_path);
				hd_videoproc_stop(stream[0].proc_sub2_path);
			}
#endif
			if (stream[0].enc_main_dim.w != MAX_CAP_SIZE_W || stream[0].enc_main_dim.h != MAX_CAP_SIZE_H){
				hd_videoproc_stop(stream[0].ref_path);
			}
		}
		if (stream[0].codec_type != HD_CODEC_TYPE_RAW){
			hd_videoenc_stop(stream[0].enc_main_path);
		}
#if MULTI_STREAM
		if (stream[0].codec_type == HD_CODEC_TYPE_H264){
			hd_videoenc_stop(stream[0].enc_sub1_path);
			hd_videoenc_stop(stream[0].enc_sub2_path);
		}
#endif
		m_VideoFmt[vidDevIdx] = pStrmInfo->strmCodec;

		//set codec,resolution
		switch (pStrmInfo->strmCodec){
			case UVAC_VIDEO_FORMAT_H264:
				stream[0].codec_type = HD_CODEC_TYPE_H264;
				//g_low_latency = 1;
				break;
			case UVAC_VIDEO_FORMAT_MJPG:
				stream[0].codec_type = HD_CODEC_TYPE_JPEG;
#if MULTI_STREAM
				multi_stream_func = FALSE;	//MJPEG does not support this feature
#endif
				//g_low_latency = 0;
				break;
			case UVAC_VIDEO_FORMAT_YUV:
				stream[0].codec_type = HD_CODEC_TYPE_RAW;
				//g_low_latency = 0;
				break;
			default:
				printf("unrecognized codec(%d)\r\n",pStrmInfo->strmCodec);
				break;
		}

		//set encode resolution to device
		stream[0].enc_main_dim.w = pStrmInfo->strmWidth;
		stream[0].enc_main_dim.h = pStrmInfo->strmHeight;
#if MULTI_STREAM
		stream[0].enc_sub1_dim.w = 1280;
		stream[0].enc_sub1_dim.h = 720;
		stream[0].enc_sub2_dim.w = 640;
		stream[0].enc_sub2_dim.h = 360;
#endif

		if (pStrmInfo->strmCodec == UVAC_VIDEO_FORMAT_YUV){
			//YUV format: stop vdoenc and allow pull from vprc
			ret = set_proc_param(stream[0].proc_main_path, &stream[0].enc_main_dim, 1);
		} else {
			ret = set_proc_param(stream[0].proc_main_path, &stream[0].enc_main_dim, 0);
		}
#if MULTI_STREAM
		if (pStrmInfo->strmCodec == UVAC_VIDEO_FORMAT_H264){
			ret = set_proc_param(stream[0].proc_sub1_path, &stream[0].enc_sub1_dim, 0);
			ret = set_proc_param(stream[0].proc_sub2_path, &stream[0].enc_sub2_dim, 0);
		}
#endif
		if (ret != HD_OK) {
			printf("set proc fail=%d\n", ret);
		}
		if (stream[0].codec_type == HD_CODEC_TYPE_JPEG) {
			//MJPG bitrate
			if (stream[0].enc_main_dim.w == 3840 || stream[0].enc_main_dim.h == 3264) {
				bitrate = 8*12*1024*1024;
			} else if(stream[0].enc_main_dim.w == 1920) {
				bitrate = 8*12*1024*1024;
			} else {
				bitrate = 8*7*1024*1024;
			}
		} else if (stream[0].codec_type == HD_CODEC_TYPE_RAW) {
			//YUV bitrate
			bitrate = 8*12*1024*1024;
		} else {
			//H264 bitrate
			if (stream[0].enc_main_dim.w == 3840 || stream[0].enc_main_dim.h == 3264) {
				bitrate = 8*8*1024*1024;
			} else if(stream[0].enc_main_dim.w == 1920) {
				bitrate = 8*2*1024*1024;
			} else {
				bitrate = 8*1*1024*1024;
			}
		}
		// set videoenc parameter (main)
		if (stream[0].codec_type != HD_CODEC_TYPE_RAW){
			ret = set_enc_param(stream[0].enc_main_path, &stream[0].enc_main_dim, stream[0].codec_type, bitrate);
		}
#if MULTI_STREAM
		if (stream[0].codec_type == HD_CODEC_TYPE_H264){
			ret = set_enc_param(stream[0].enc_sub1_path, &stream[0].enc_sub1_dim, stream[0].codec_type, 4*8*1024*1024);
			ret = set_enc_param(stream[0].enc_sub2_path, &stream[0].enc_sub2_dim, stream[0].codec_type, 2*8*1024*1024);
		}
#endif
		if (ret != HD_OK) {
			printf("set enc fail=%d\n", ret);
		}
		//4K MJPG max fps is 25
		if (stream[0].codec_type == HD_CODEC_TYPE_JPEG && (stream[0].enc_main_dim.w == 3840 || stream[0].enc_main_dim.h == 3264)) {
			fps = 25;
		} else {
			fps = 30;
		}
		ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim, fps);
		if (ret != HD_OK) {
			printf("set cap fail=%d\n", ret);
		}
		if (g_capbind == 0) {
			//start engine(modules)
			hd_videocap_start(stream[0].cap_path);
		}
#if ALG_FD_PD
#if NN_FDCNN_FD_DRAW
	ret = hd_videoproc_start(stream[0].mask_path0);
	if (ret != HD_OK) {
		printf("fail to start vp mask\n");
		//goto exit;
	}
	ret = hd_videoproc_start(stream[0].mask_path1);
	if (ret != HD_OK) {
		printf("fail to start vp mask\n");
		//goto exit;
	}
	ret = hd_videoproc_start(stream[0].mask_path2);
	if (ret != HD_OK) {
		printf("fail to start vp mask\n");
		//goto exit;
	}
	ret = hd_videoproc_start(stream[0].mask_path3);
	if (ret != HD_OK) {
		printf("fail to start vp mask\n");
		//goto exit;
	}
#endif

#if NN_PVDCNN_DRAW
	ret = hd_videoproc_start(stream[0].mask_path4);
	if (ret != HD_OK) {
		printf("fail to start vp mask\n");
		//goto exit;
	}
	ret = hd_videoproc_start(stream[0].mask_path5);
	if (ret != HD_OK) {
		printf("fail to start vp mask\n");
		//goto exit;
	}
	ret = hd_videoproc_start(stream[0].mask_path6);
	if (ret != HD_OK) {
		printf("fail to start vp mask\n");
		//goto exit;
	}
	ret = hd_videoproc_start(stream[0].mask_path7);
	if (ret != HD_OK) {
		printf("fail to start vp mask\n");
		//goto exit;
	}
#endif
#endif	//ALG_FD_PD
		hd_videoproc_start(stream[0].proc_main_path);
#if MULTI_STREAM
		if (stream[0].codec_type == HD_CODEC_TYPE_H264){
			hd_videoproc_start(stream[0].proc_sub1_path);
			hd_videoproc_start(stream[0].proc_sub2_path);
		}
#endif
		if (pStrmInfo->strmWidth != MAX_CAP_SIZE_W || pStrmInfo->strmHeight != MAX_CAP_SIZE_H){
			hd_videoproc_start(stream[0].ref_path);
		}
		if (g_capbind == 1) {
			//start engine(modules)
			hd_videocap_start(stream[0].cap_path);
		}
		if (pStrmInfo->strmCodec != UVAC_VIDEO_FORMAT_YUV){
			hd_videoenc_start(stream[0].enc_main_path);
		}

#if MULTI_STREAM
		if (pStrmInfo->strmCodec == UVAC_VIDEO_FORMAT_H264){
			hd_videoenc_start(stream[0].enc_sub1_path);
			hd_videoenc_start(stream[0].enc_sub2_path);
		}
#endif

		if (pStrmInfo->strmCodec != UVAC_VIDEO_FORMAT_YUV){
			pthread_mutex_lock(&flow_start_lock);
			encode_start = 1;
			acquire_start = 0;
			ai_start = 1;
			pthread_mutex_unlock(&flow_start_lock);
		} else {
			pthread_mutex_lock(&flow_start_lock);
			encode_start = 0;
			acquire_start = 1;
			ai_start = 0;
			pthread_mutex_unlock(&flow_start_lock);
		}
	}
	return E_OK;
}

static UINT32 xUvacStopVideoCB(UINT32 isClosed)
{
	//printf(":isClosed=%d\r\n", isClosed);
	printf("stop encode flow\r\n");
	pthread_mutex_lock(&flow_start_lock);
	encode_start = 0;
	acquire_start = 0;
	ai_start = 0;
	pthread_mutex_unlock(&flow_start_lock);
	return E_OK;
}

static UINT32 xUvacStartAudioCB(UVAC_AUD_DEV_CNT audDevIdx, UVAC_STRM_INFO *pStrmInfo)
{
	UINT32 stop_acap = FALSE;

	if (pStrmInfo->isAudStrmOn) {
		//stop poll data
		pthread_mutex_lock(&flow_start_lock);
		flow_audio_start = 0;
		pthread_mutex_unlock(&flow_start_lock);

#if AUDIO_ON
		//start audio capture module
		hd_audiocap_start(au_cap.cap_path);
#endif
		pthread_mutex_lock(&flow_start_lock);
		flow_audio_start = 1;
		pthread_mutex_unlock(&flow_start_lock);
	} else {
		pthread_mutex_lock(&flow_start_lock);
		if (flow_audio_start) {
			stop_acap = TRUE;
		}
		flow_audio_start = 0;
		pthread_mutex_unlock(&flow_start_lock);

#if AUDIO_ON
		//stop audio capture module
		if (stop_acap) {
			hd_audiocap_stop(au_cap.cap_path);
		}
#endif

		return E_OK;
	}

	return E_OK;
}


BOOL xUvacEU_CB(UINT32 CS, UINT8 request, UINT8 *pData, UINT32 *pDataLen){
	BOOL ret = FALSE;

	if (request == SET_CUR){
		printf("%s: SET_CUR, CS=0x%02X, request = 0x%X,  pData=0x%X, len=%d\r\n", __func__, CS, request, pData, *pDataLen);
	}
	switch (CS){
		case EU_CONTROL_ID_01:
		case EU_CONTROL_ID_02:
		case EU_CONTROL_ID_03:
		case EU_CONTROL_ID_04:
		case EU_CONTROL_ID_05:
		case EU_CONTROL_ID_06:
		case EU_CONTROL_ID_07:
		case EU_CONTROL_ID_08:
		case EU_CONTROL_ID_09:
		case EU_CONTROL_ID_10:
		case EU_CONTROL_ID_11:
		case EU_CONTROL_ID_12:
		case EU_CONTROL_ID_13:
		case EU_CONTROL_ID_14:
		case EU_CONTROL_ID_15:
		case EU_CONTROL_ID_16:
			//not supported now
			break;
		default:
			break;
	}

	if (request != SET_CUR) {
		printf("%s: CS=0x%02X, request = 0x%X,  pData=0x%X, len=%d\r\n", __func__, CS, request, pData, *pDataLen);
	}

	if (ret && request == SET_CUR){
		*pDataLen = 0;
	}

	return ret;
}

static BOOL xUvac_CT_AE_Mode(UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = TRUE;
	AET_MANUAL AEInfo = {0};

	switch(request)
	{
		//No need to support GET_LEN, GET_MAX, GET_MIN
		case GET_INFO:
			*pDataLen = 1;	//must be 1 according to UVC spec
			pData[0] = SUPPORT_GET_REQUEST | SUPPORT_SET_REQUEST;
			break;
		case GET_RES:
			*pDataLen = 1;
			pData[0] = 0x06;
			break;
		case GET_DEF:
			*pDataLen = 1;
			pData[0] = 0x02;
			break;
		case GET_CUR:
			if (vendor_isp_init() == HD_ERR_NG) {
				return FALSE;
			}
			*pDataLen = 1;
			AEInfo.id = 0;
			vendor_isp_get_ae(AET_ITEM_MANUAL, &AEInfo);
			if (AEInfo.manual.lock_en){
				pData[0] = 0x04;
			} else {
				pData[0] = 0x02;
			}
			vendor_isp_uninit();
			break;
		case SET_CUR:
			if (vendor_isp_init() == HD_ERR_NG) {
				return FALSE;
			}
			AEInfo.id = 0;
			if (pData[0] == 0x04){
				vendor_isp_get_ae(AET_ITEM_MANUAL, &AEInfo);
				AEInfo.manual.lock_en = 1;
				vendor_isp_set_ae(AET_ITEM_MANUAL, &AEInfo);
			} else if (pData[0] == 0x02){
				vendor_isp_get_ae(AET_ITEM_MANUAL, &AEInfo);
				AEInfo.manual.lock_en = 0;
				vendor_isp_set_ae(AET_ITEM_MANUAL, &AEInfo);
			} else {
				printf("CT AE mode not support (%u)\r\n", pData[0]);
			}
			vendor_isp_uninit();
			break;
		default:
			ret = FALSE;
			break;
	}
	return ret;
}

static BOOL xUvac_CT_Exposure_Time_Absolute(UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = TRUE;
	AET_MANUAL AEInfo = {0};

	switch(request)
	{
		case GET_LEN:
			*pDataLen = 2;	//must be 2 according to UVC spec
			pData[0] = 4;
			pData[1] = 0;
			break;
		case GET_INFO:
			*pDataLen = 1;	//must be 1 according to UVC spec
			pData[0] =  SUPPORT_GET_REQUEST | SUPPORT_SET_REQUEST;
			break;
		case GET_MIN:
			*pDataLen = 4;
			pData[0] = 625 & 0xFF;
			pData[1] = (625 >> 8) & 0xFF;
			pData[2] = (625 >> 16) & 0xFF;
			pData[3] = (625 >> 24) & 0xFF;
			break;
		case GET_MAX:
			*pDataLen = 4;
			pData[0] = 160000 & 0xFF;
			pData[1] = (160000 >> 8) & 0xFF;
			pData[2] = (160000 >> 16) & 0xFF;
			pData[3] = (160000 >> 24) & 0xFF;
			break;
		case GET_RES:
			*pDataLen = 4;
			pData[0] = 1;
			pData[1] = 0;
			pData[2] = 0;
			pData[3] = 0;
			break;
		case GET_DEF:
			*pDataLen = 4;
			pData[0] = 40000 & 0xFF;;
			pData[1] = (40000 >> 8) & 0xFF;
			pData[2] = (40000 >> 16) & 0xFF;
			pData[3] = (40000 >> 24) & 0xFF;
			break;
		case GET_CUR:
			if (vendor_isp_init() == HD_ERR_NG) {
				return FALSE;
			}
			*pDataLen = 4;
			AEInfo.id = 0;
			vendor_isp_get_ae(AET_ITEM_MANUAL, &AEInfo);
			if (AEInfo.manual.expotime==0 || AEInfo.manual.iso_gain==0){
				pData[0] = 625 & 0xFF;
				pData[1] = (625 >> 8) & 0xFF;
				pData[2] = (625 >> 16) & 0xFF;
				pData[3] = (625 >> 24) & 0xFF;
				AEInfo.manual.expotime = 625;       //set default value
				AEInfo.manual.iso_gain = 100;		//fixed to 100
				vendor_isp_set_ae(AET_ITEM_MANUAL, &AEInfo);
			} else {
				pData[0] = AEInfo.manual.expotime & 0xFF;
				pData[1] = (AEInfo.manual.expotime >> 8) & 0xFF;
				pData[2] = (AEInfo.manual.expotime >> 16) & 0xFF;
				pData[3] = (AEInfo.manual.expotime >> 24) & 0xFF;
			}
			vendor_isp_uninit();
			break;
		case SET_CUR:
			if (vendor_isp_init() == HD_ERR_NG) {
				return FALSE;
			}
			AEInfo.id = 0;
			vendor_isp_get_ae(AET_ITEM_MANUAL, &AEInfo);
			AEInfo.manual.expotime = pData[0] + (pData[1]<<8) + (pData[2]<<16) + (pData[3] << 24);
			AEInfo.manual.iso_gain = 100;
			vendor_isp_set_ae(AET_ITEM_MANUAL, &AEInfo);
			vendor_isp_uninit();
			break;
		default:
			ret = FALSE;
			break;
	}
	return ret;
}

BOOL xUvacCT_CB(UINT32 CS, UINT8 request, UINT8 *pData, UINT32 *pDataLen){
	BOOL ret = FALSE;

	if (request == SET_CUR){
		printf("%s: SET_CUR, CS=0x%02X, request = 0x%X,  pData=0x%X, len=%d\r\n", __func__, CS, request, pData, *pDataLen);
	}
	switch (CS){
		case CT_SCANNING_MODE_CONTROL:
			//not supported now
			break;
		case CT_AE_MODE_CONTROL:
			ret = xUvac_CT_AE_Mode(request, pData, pDataLen);
			break;
		case CT_AE_PRIORITY_CONTROL:
			//not supported now
			break;
		case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
			ret = xUvac_CT_Exposure_Time_Absolute(request, pData, pDataLen);
			break;
		case CT_EXPOSURE_TIME_RELATIVE_CONTROL:
		case CT_FOCUS_ABSOLUTE_CONTROL:
		case CT_FOCUS_RELATIVE_CONTROL:
		case CT_FOCUS_AUTO_CONTROL:
		case CT_IRIS_ABSOLUTE_CONTROL:
		case CT_IRIS_RELATIVE_CONTROL:
		case CT_ZOOM_ABSOLUTE_CONTROL:
		case CT_ZOOM_RELATIVE_CONTROL:
		case CT_PANTILT_ABSOLUTE_CONTROL:
		case CT_PANTILT_RELATIVE_CONTROL:
		case CT_ROLL_ABSOLUTE_CONTROL:
		case CT_ROLL_RELATIVE_CONTROL:
		case CT_PRIVACY_CONTROL:
			//not supported now
			break;
		default:
			break;
	}

	if (request != SET_CUR) {
		printf("%s: CS=0x%02X, request = 0x%X,  pData=0x%X, len=%d\r\n", __func__, CS, request, pData, *pDataLen);
	}

	if (ret && request == SET_CUR){
		*pDataLen = 0;
	}

	return ret;
}

BOOL xUvacPU_CB(UINT32 CS, UINT8 request, UINT8 *pData, UINT32 *pDataLen){
	BOOL ret = FALSE;

	if (request == SET_CUR){
		printf("%s: SET_CUR, CS=0x%02X, request = 0x%X,  pData=0x%X, len=%d\r\n", __func__, CS, request, pData, *pDataLen);
	}
	switch (CS){
	case PU_BACKLIGHT_COMPENSATION_CONTROL:
	case PU_BRIGHTNESS_CONTROL:
	case PU_CONTRAST_CONTROL:
	case PU_GAIN_CONTROL:
	case PU_POWER_LINE_FREQUENCY_CONTROL:
	case PU_HUE_CONTROL:
	case PU_SATURATION_CONTROL:
	case PU_SHARPNESS_CONTROL:
	case PU_GAMMA_CONTROL:
	case PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
	case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
	case PU_WHITE_BALANCE_COMPONENT_CONTROL:
	case PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL:
	case PU_DIGITAL_MULTIPLIER_CONTROL:
	case PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL:
	case PU_HUE_AUTO_CONTROL:
	case PU_ANALOG_VIDEO_STANDARD_CONTROL:
	case PU_ANALOG_LOCK_STATUS_CONTROL:
		//not supported now
		break;
	default:
		break;
	}

	if (request != SET_CUR) {
		printf("%s: CS=0x%02X, request = 0x%X,  pData=0x%X, len=%d\r\n", __func__, CS, request, pData, *pDataLen);
	}

	if (ret && request == SET_CUR){
		*pDataLen = 0;
	}

	return ret;
}

#if (MSDC_FUNC == 1)
static UINT32 user_va = 0;
static UINT32 user_pa = 0;
static HD_COMMON_MEM_VB_BLK msdc_blk;
static PMSDC_OBJ p_msdc_object;
static _ALIGNED(64) UINT8 InquiryData[36] = {
	0x00, 0x80, 0x05, 0x02, 0x20, 0x00, 0x00, 0x00,
//    //Vendor identification, PREMIER
	'N', 'O', 'V', 'A', 'T', 'E', 'K', '-',
//    //product identification, DC8365
	'N', 'T', '9', '8', '5', '2', '8', '-',
	'D', 'S', 'P', ' ', 'I', 'N', 'S', 'I',
//    //product revision level, 1.00
	'D', 'E', ' ', ' '
};

static BOOL emuusb_msdc_strg_detect(void)
{
	return 1;
}
#endif

#if CDC_FUNC
typedef _PACKED_BEGIN struct {
	UINT32   uiBaudRateBPS; ///< Data terminal rate, in bits per second.
	UINT8    uiCharFormat;  ///< Stop bits.
	UINT8    uiParityType;  ///< Parity.
	UINT8    uiDataBits;    ///< Data bits (5, 6, 7, 8 or 16).
} _PACKED_END CDCLineCoding;

typedef enum _CDC_PSTN_REQUEST {
	REQ_SET_LINE_CODING         =    0x20,
	REQ_GET_LINE_CODING         =    0x21,
	REQ_SET_CONTROL_LINE_STATE  =    0x22,
	REQ_SEND_BREAK              =    0x23,
	ENUM_DUMMY4WORD(CDC_PSTN_REQUEST)
} CDC_PSTN_REQUEST;

#define CDC_READ_BUFFER_POOL     HD_COMMON_MEM_USER_DEFINIED_POOL
#define CDC_WRITE_BUFFER_POOL	(CDC_READ_BUFFER_POOL+1)

#define CDC_READ_BUFFER_SIZE    (512*1024)
#define CDC_WRITE_BUFFER_SIZE	(512*1024)

typedef struct _CDC_MEM_PARM {
	UINT32 pa;
	UINT32 va;
	UINT32 size;
} CDC_MEM_PARM;

static CDC_MEM_PARM cdc_read_buf = {0};
static CDC_MEM_PARM cdc_write_buf = {0};

static BOOL CdcPstnReqCB(CDC_COM_ID ComID, UINT8 Code, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL bSupported = TRUE;
	CDCLineCoding LineCoding;

	switch (Code) {
	case REQ_GET_LINE_CODING:
		LineCoding.uiBaudRateBPS = 115200;
		LineCoding.uiCharFormat = 0;//CDC_LINEENCODING_OneStopBit;
		LineCoding.uiParityType = 0;//CDC_PARITY_None;
		LineCoding.uiDataBits = 8;
		*pDataLen = sizeof(LineCoding);

		memcpy(pData, &LineCoding, *pDataLen);
		break;
	case REQ_SET_LINE_CODING:
		if (*pDataLen == sizeof(LineCoding)) {
			memcpy(&LineCoding, pData, *pDataLen);
		} else {
			bSupported = FALSE;
		}
		break;
	case REQ_SET_CONTROL_LINE_STATE:
		//debug console test
		if (*(UINT16 *)pData == 0x3) { //console ready
		}
		break;
	default:
		bSupported = FALSE;
		break;
	}
	return bSupported;
}
#endif

static BOOL UVAC_enable(void)
{

	UVAC_INFO       UvacInfo = {0};
	UINT32          retV = 0;

	UVAC_VEND_DEV_DESC UIUvacDevDesc = {0};
	UVAC_AUD_SAMPLERATE_ARY uvacAudSampleRateAry = {0};
	UINT32 bVidFmtType = UVAC_VIDEO_FORMAT_H264_MJPEG;
	m_bIsStaticPattern = FALSE;
	UINT32            uvacNeedMem  = 0;
	UINT32            ChannelNum   = 1;

	printf("###Real Open UVAC-lib\r\n");

	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32 pa;
	void *va;
	HD_RESULT hd_ret;

	timer_init();

	hd_ret = hd_gfx_init();
	if(hd_ret != HD_OK) {
        printf("init gfx fail=%d\n", hd_ret);
	}

	#if (MSDC_FUNC == 1)
	{
		STORAGE_OBJ *pStrg = sdio_getStorageObject(STRG_OBJ_FAT1);
		USB_MSDC_INFO       MSDCInfo;
		UINT32 i;
		HD_RESULT                 ret;
		UVAC_MSDC_INFO       uvac_msdc_info;

		if (pStrg == NULL) {
			printf("failed to get STRG_OBJ_FAT1.n");
			return -1;
		}

		msdc_blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_DEFINIED_POOL, POOL_SIZE_USER_DEFINIED, DDR_ID0);
	    if (HD_COMMON_MEM_VB_INVALID_BLK == msdc_blk) {
	            printf("hd_common_mem_get_block fail\r\n");
	            return HD_ERR_NG;
	    }
	    user_pa = hd_common_mem_blk2pa(msdc_blk);
	    if (user_pa == 0) {
	            printf("not get buffer, pa=%08x\r\n", (int)user_pa);
	            return HD_ERR_NG;
	    }
	    user_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, user_pa, POOL_SIZE_USER_DEFINIED);
	    /* Release buffer */
	    if (user_va == 0) {
	            printf("mem map fail\r\n");
	            ret = hd_common_mem_munmap((void *)user_va, POOL_SIZE_USER_DEFINIED);
	            if (ret != HD_OK) {
	                    printf("mem unmap fail\r\n");
	                    return ret;
	            }
	            return HD_ERR_NG;
	    }

		p_msdc_object = Msdc_getObject(MSDC_ID_USB20);
		p_msdc_object->SetConfig(USBMSDC_CONFIG_ID_SELECT_POWER,  USBMSDC_POW_SELFPOWER);
		p_msdc_object->SetConfig(USBMSDC_CONFIG_ID_COMPOSITE,  TRUE);

		MSDCInfo.uiMsdcBufAddr_va= (UINT32)user_va;
		MSDCInfo.uiMsdcBufAddr_pa= (UINT32)user_pa;
		if (MSDCInfo.uiMsdcBufAddr_pa == 0) {
			printf("malloc buffer failed\n");
		}
		MSDCInfo.uiMsdcBufSize = MSDC_MIN_BUFFER_SIZE;
		MSDCInfo.pInquiryData = (UINT8 *)&InquiryData[0];

		MSDCInfo.msdc_check_cb = NULL;
		MSDCInfo.msdc_vendor_cb = NULL;

		MSDCInfo.msdc_RW_Led_CB = NULL;
		MSDCInfo.msdc_StopUnit_Led_CB = NULL;
		MSDCInfo.msdc_UsbSuspend_Led_CB = NULL;

		for (i = 0; i < MAX_LUN; i++) {
			MSDCInfo.msdc_type[i] = MSDC_STRG;
		}

		MSDCInfo.pStrgHandle[0] = pStrg;//pSDIOObj
		MSDCInfo.msdc_strgLock_detCB[0] = NULL;
		MSDCInfo.msdc_storage_detCB[0] = emuusb_msdc_strg_detect;
		MSDCInfo.LUNs = 1;

		if (p_msdc_object->Open(&MSDCInfo) != E_OK) {
			printf("msdc open failed\r\n ");
		}

		uvac_msdc_info.en = TRUE;
		UVAC_SetConfig(UVAC_CONFIG_MSDC_INFO, (UINT32) &uvac_msdc_info);
	}
	#elif (MSDC_FUNC == 2)
	{
		UVAC_MSDC_INFO uvac_msdc_info;
		uvac_msdc_info.en = TRUE;
		nvt_cmdsys_runcmd("msdcnvt open");
		UVAC_SetConfig(UVAC_CONFIG_MSDC_INFO, (UINT32) &uvac_msdc_info);
	}
	#endif

	UVAC_SetConfig(UVAC_CONFIG_MAX_FRAME_SIZE, 4*1024*1024); //default 800KB, MAX:3MB

#if  UAC_RX_ON
	UVAC_SetConfig(UVAC_CONFIG_UAC_RX_ENABLE, TRUE);
#endif

	uvac_size = UVAC_GetNeedMemSize();
	//printf("uvac_size = %u\r\n", uvac_size);
	if ((hd_ret = hd_common_mem_alloc("usbmovie", &pa, (void **)&va, uvac_size, ddr_id)) != HD_OK) {
		printf("hd_common_mem_alloc failed(%d)\r\n", hd_ret);
		uvac_va = 0;
		uvac_pa = 0;
		uvac_size = 0;
	} else {
		uvac_va = (UINT32)va;
		uvac_pa = (UINT32)pa;
	}

	uvacNeedMem *= ChannelNum;
	UvacInfo.UvacMemAdr    = uvac_pa;
	UvacInfo.UvacMemSize   = uvac_size;

	UvacInfo.fpStartVideoCB  = (UVAC_STARTVIDEOCB)xUvacStartVideoCB;
	UvacInfo.fpStopVideoCB  = (UVAC_STOPVIDEOCB)xUvacStopVideoCB;

	USBMakerInit_UVAC(&UIUvacDevDesc);

	UVAC_SetConfig(UVAC_CONFIG_VEND_DEV_DESC, (UINT32)&UIUvacDevDesc);
	UVAC_SetConfig(UVAC_CONFIG_HW_COPY_CB, (UINT32)hd_gfx_memcpy);

	UVAC_SetConfig(UVAC_CONFIG_AUD_START_CB, (UINT32)xUvacStartAudioCB);
	//printf("%s:uvacAddr=0x%x,s=0x%x;IPLAddr=0x%x,s=0x%x\r\n", __func__, UvacInfo.UvacMemAdr, UvacInfo.UvacMemSize, m_VideoBuf.addr, m_VideoBuf.size);

	#if 0 //sample code for customized EU descriptor
	{
		static UINT8 baSourceID[] = {0}; //this should be a static array, set 0 for default
		static UINT8 bmControls[] = {0xFF, 0x00}; //this should be a static array
		UVAC_EU_DESC eu_desc = {0};
		//UINT8 guid[16] = {0x29, 0xa7, 0x87, 0xc9, 0xd3, 0x59, 0x69, 0x45, 0x84, 0x67, 0xff, 0x08, 0x49, 0xfc, 0x19, 0xe8};
		UINT8 guid[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

		eu_desc.bUnitID = 5;
		eu_desc.bDescriptorType = CS_INTERFACE;
		eu_desc.bDescriptorSubtype = VC_EXTENSION_UNIT;
		memcpy(eu_desc.guidExtensionCode, guid, sizeof(eu_desc.guidExtensionCode));
		eu_desc.bNrInPins = sizeof(baSourceID);
		eu_desc.baSourceID = baSourceID;
		eu_desc.bControlSize = sizeof(bmControls);
		eu_desc.bmControls = bmControls;
		eu_desc.bNumControls = get_eu_control_num(eu_desc.bmControls, eu_desc.bControlSize);
		eu_desc.bLength = 24 + eu_desc.bNrInPins + eu_desc.bControlSize;
		UVAC_SetConfig(UVAC_CONFIG_EU_DESC, (UINT32)&eu_desc);
	}
	#endif
	#if 0 //sample code for setting UVC and UAC string
	{
		//this should be a static array
		static UINT16 uvc_string_descriptor[] = {
			0x0316,                             // 16: size of String Descriptor = 22 bytes
												// 03: String Descriptor type
			'U', 'V', 'C', '_', 'D',            // UVC_DEVICE
			'E', 'V', 'I', 'C', 'E'
		};
		//this should be a static array
		static UINT16 uac_string_descriptor[] = {
			0x0316,                             // 16: size of String Descriptor = 22 bytes
												// 03: String Descriptor type
			'U', 'A', 'C', '_', 'D',            // UAC_DEVICE
			'E', 'V', 'I', 'C', 'E'
		};
		UVAC_SetConfig(UVAC_CONFIG_UVC_STRING, (UINT32)uvc_string_descriptor);
		UVAC_SetConfig(UVAC_CONFIG_UAC_STRING, (UINT32)uac_string_descriptor);
	}
	#endif
	#if HID_FUNC//sample code for setting HID
	{
		//this should be a static array
		static UINT16 hid_string_descriptor[] = {
			0x0316,                             // 16: size of String Descriptor = 22 bytes
												// 03: String Descriptor type
			'H', 'I', 'D', '_', 'D',            // UAC_DEVICE
			'E', 'V', 'I', 'C', 'E'
		};
		UVAC_HID_INFO hid_info = {0};

		hid_info.en = TRUE;
		hid_info.cb = UvacHidCB;
		hid_info.hid_desc.bLength = 9;
		hid_info.hid_desc.bHidDescType = 0x21;
		hid_info.hid_desc.bcdHID = 0x0111;
		hid_info.hid_desc.bCountryCode = 0;
		hid_info.hid_desc.bNumDescriptors = 1;
		hid_info.hid_desc.bDescriptorType = 0x22;//Report Descriptor
		hid_info.hid_desc.wDescriptorLength = sizeof(report_desc);
		hid_info.p_report_desc = report_desc;
		hid_info.intr_out = TRUE;
		hid_info.intr_interval = 3;
		hid_info.p_vendor_string = (UVAC_STRING_DESC *)hid_string_descriptor;
		UVAC_SetConfig(UVAC_CONFIG_HID_INFO, (UINT32)&hid_info);
	}
	#endif

	//must set to 1I14P ratio
	UvacInfo.strmInfo.strmWidth = MAX_BS_W;
	UvacInfo.strmInfo.strmHeight = MAX_BS_H;
	UvacInfo.strmInfo.strmFps = MAX_BS_FPS;
	//UvacInfo.strmInfo.strmCodec = MEDIAREC_ENC_H264;
	//w=1280,720,30,2
	UVAC_ConfigVidReso(gUIUvacVidReso, NVT_UI_UVAC_RESO_CNT);
	//set default frame index, starting from 1
	//UVAC_SetConfig(UVAC_CONFIG_MJPG_DEF_FRM_IDX, 1);
	//UVAC_SetConfig(UVAC_CONFIG_H264_DEF_FRM_IDX, 1);

	#if UVC_SUPPORT_YUV_FMT
	{
		UVAC_VID_RESO_ARY uvacVidYuvResoAry = {0};

		uvacVidYuvResoAry.aryCnt = sizeof(gUIUvacVidYuvReso)/sizeof(UVAC_VID_RESO);
		uvacVidYuvResoAry.pVidResAry = &gUIUvacVidYuvReso[0];
		//set default frame index, starting from 1
		//uvacVidYuvResoAry.bDefaultFrameIndex = 1;
		UVAC_SetConfig(UVAC_CONFIG_YUV_FRM_INFO, (UINT32)&uvacVidYuvResoAry);
	}
	#endif
	uvacAudSampleRateAry.aryCnt = NVT_UI_UVAC_AUD_SAMPLERATE_CNT;
	uvacAudSampleRateAry.pAudSampleRateAry = &gUIUvacAudSampleRate[0]; //NVT_UI_FREQUENCY_32K
	UVAC_SetConfig(UVAC_CONFIG_AUD_SAMPLERATE, (UINT32)&uvacAudSampleRateAry);
	UvacInfo.channel = ChannelNum;
	//printf("w=%d,h=%d,fps=%d,codec=%d\r\n", UvacInfo.strmInfo.strmWidth, UvacInfo.strmInfo.strmHeight, UvacInfo.strmInfo.strmFps, UvacInfo.strmInfo.strmCodec);
	UVAC_SetConfig(UVAC_CONFIG_AUD_CHANNEL_NUM, ChannelNum);
	UVAC_SetConfig(UVAC_CONFIG_VIDEO_FORMAT_TYPE, bVidFmtType);

	//Extension Unit
	#if 1
	UVAC_SetConfig(UVAC_CONFIG_XU_CTRL, (UINT32)xUvacEU_CB);
	#else
	//deprecated, backward compatible
	UVAC_SetConfig(UVAC_CONFIG_EU_VENDCMDCB_ID01, (UINT32)xUvacEUVendCmdCB_Idx1);
	UVAC_SetConfig(UVAC_CONFIG_EU_VENDCMDCB_ID02, (UINT32)xUvacEUVendCmdCB_Idx2);
	UVAC_SetConfig(UVAC_CONFIG_EU_VENDCMDCB_ID03, (UINT32)xUvacEUVendCmdCB_Idx3);
	UVAC_SetConfig(UVAC_CONFIG_EU_VENDCMDCB_ID04, (UINT32)xUvacEUVendCmdCB_Idx4);
	#endif

	//CT & PU
	UVAC_SetConfig(UVAC_CONFIG_CT_CB, (UINT32)xUvacCT_CB);
	UVAC_SetConfig(UVAC_CONFIG_PU_CB, (UINT32)xUvacPU_CB);

	#if MULTI_STREAM
	//Multi-Stream
	if (multi_stream_func) {
		UVAC_SetConfig(UVAC_CONFIG_VID_USER_DATA_SIZE, sizeof(STREAM_USER_DATA));
	} else {
		UVAC_SetConfig(UVAC_CONFIG_VID_USER_DATA_SIZE, 0);
	}
	#endif

	#if CDC_FUNC
	UVAC_SetConfig(UVAC_CONFIG_CDC_PSTN_REQUEST_CB, (UINT32)CdcPstnReqCB);
	UVAC_SetConfig(UVAC_CONFIG_CDC_ENABLE, TRUE);
	#endif

	retV = UVAC_Open(&UvacInfo);
	if (retV != E_OK) {
		printf("Error open UVAC task\r\n", retV);
	}

	return TRUE;
}

static BOOL UVAC_disable(void)
{
	HD_RESULT    ret = HD_OK;

	#if (MSDC_FUNC == 1)
	{
		p_msdc_object->Close();

		if(msdc_blk) {
			hd_common_mem_release_block(msdc_blk);
		}
		if(user_va) {
			if(hd_common_mem_munmap((void *)user_va, POOL_SIZE_USER_DEFINIED)) {
				printf("fail to unmap va(%x)\n", (int)user_va);
			}
		}
	}
	#elif (MSDC_FUNC == 2)
	{
		nvt_cmdsys_runcmd("msdcnvt close");
	}
	#endif

	ret = hd_common_mem_free(uvac_pa, &uvac_va);
	if (HD_OK == ret) {
		printf("Error handle test for free an invalid address: fail %d\r\n", (int)(ret));
	}
	UVAC_Close();

	return TRUE;
}
#endif

#if ALG_FD_PD
static HD_RESULT set_proc_param_extend(HD_PATH_ID video_proc_path, HD_PATH_ID src_path, HD_DIM* p_dim, UINT32 dir)
{
	HD_RESULT ret = HD_OK;

	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT_EX video_out_param = {0};
		video_out_param.src_path = src_path;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.dir = dir;
		video_out_param.depth = 1; //set 1 to allow pull

		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_EX, &video_out_param);
	}

	return ret;
}

static UINT32 load_file(CHAR *p_filename, UINT32 va)
{
	FILE  *fd;
	UINT32 file_size = 0, read_size = 0;
	const UINT32 model_addr = va;

	fd = fopen(p_filename, "rb");
	if (!fd) {
		printf("ERR: cannot read %s\r\n", p_filename);
		return 0;
	}

	fseek ( fd, 0, SEEK_END );
	file_size = ALIGN_CEIL_4( ftell(fd) );
	fseek ( fd, 0, SEEK_SET );

	read_size = fread ((void *)model_addr, 1, file_size, fd);
	if (read_size != file_size) {
		printf("ERR: size mismatch, real = %d, idea = %d\r\n", (int)read_size, (int)file_size);
	}
	fclose(fd);
	return read_size;
}

#if (NN_FDCNN_FD_DRAW || NN_PVDCNN_DRAW)
int init_mask_param(HD_PATH_ID mask_path)
{
	HD_OSG_MASK_ATTR attr;

	memset(&attr, 0, sizeof(HD_OSG_MASK_ATTR));

    // ghost target
    attr.position[0].x = 1;
    attr.position[0].y = 1;
    attr.position[1].x = 9;
    attr.position[1].y = 1;
    attr.position[2].x = 9;
    attr.position[2].y = 9;
    attr.position[3].x = 1;
    attr.position[3].y = 9;
    attr.type          = HD_OSG_MASK_TYPE_HOLLOW;
    attr.alpha         = 0;
    attr.color         = 0x00FF0000;
    attr.thickness     = 2;

	return hd_videoproc_set(mask_path, HD_VIDEOPROC_PARAM_IN_MASK_ATTR, &attr);
}

int fdcnn_mask_draw(HD_PATH_ID mask_path, FDCNN_RESULT *p_face, BOOL bdraw, UINT32 color)
{
    HD_OSG_MASK_ATTR attr;
    HD_RESULT ret = HD_OK;
    memset(&attr, 0, sizeof(HD_OSG_MASK_ATTR));

    if(!bdraw){
		printf("xxxxxxxxxx\r\n");
        return init_mask_param(mask_path);
	}

    attr.position[0].x = p_face->x;
    attr.position[0].y = p_face->y;
    attr.position[1].x = p_face->x + p_face->w;
    attr.position[1].y = p_face->y;
    attr.position[2].x = p_face->x + p_face->w;
    attr.position[2].y = p_face->y + p_face->h;
    attr.position[3].x = p_face->x;
    attr.position[3].y = p_face->y + p_face->h;
    attr.type          = HD_OSG_MASK_TYPE_HOLLOW;
    attr.alpha         = 255;
    attr.color         = color;
    attr.thickness     = 2;

    ret = hd_videoproc_set(mask_path, HD_VIDEOPROC_PARAM_IN_MASK_ATTR, &attr);

    return ret;
}

int pvdcnn_mask_draw(HD_PATH_ID mask_path, PVDCNN_RSLT *p_obj, BOOL bdraw, UINT32 color)
{
    HD_OSG_MASK_ATTR attr;
    HD_RESULT ret = HD_OK;

    memset(&attr, 0, sizeof(HD_OSG_MASK_ATTR));

    if (!bdraw || p_obj == NULL) {
        return init_mask_param(mask_path);
    }

	HD_URECT *p_box = &p_obj->box;

    attr.position[0].x = p_box->x;
    attr.position[0].y = p_box->y;
    attr.position[1].x = p_box->x + p_box->w;
    attr.position[1].y = p_box->y;
    attr.position[2].x = p_box->x + p_box->w;
    attr.position[2].y = p_box->y + p_box->h;
    attr.position[3].x = p_box->x;
    attr.position[3].y = p_box->y + p_box->h;
    attr.type          = HD_OSG_MASK_TYPE_HOLLOW;
    attr.alpha         = 255;
    attr.color         = color;
    attr.thickness     = 2;

    ret = hd_videoproc_set(mask_path, HD_VIDEOPROC_PARAM_IN_MASK_ATTR, &attr);

    return ret;
}

#endif //(NN_FDCNN_FD_DRAW || NN_PVDCNN_DRAW)

#if NN_FDCNN_FD_DRAW
int fdcnn_fd_draw_info(VENDOR_AIS_FLOW_MEM_PARM fd_buf, VIDEO_STREAM *p_stream)
{
    HD_RESULT ret = HD_OK;
    static FDCNN_RESULT fdcnn_info[FDCNN_MAX_OUTNUM] = {0};
    UINT32 fdcnn_num;
    static HD_URECT fdcnn_size = {0, 0, OSG_LCD_WIDTH, OSG_LCD_HEIGHT};

    fdcnn_num = fdcnn_getresults(fd_buf, fdcnn_info, &fdcnn_size, FDCNN_MAX_OUTNUM);

    fdcnn_mask_draw(p_stream->mask_path0, fdcnn_info + 0, (BOOL)(fdcnn_num >= 1), 0x00FF0000);
    fdcnn_mask_draw(p_stream->mask_path1, fdcnn_info + 1, (BOOL)(fdcnn_num >= 2), 0x00FF0000);
    fdcnn_mask_draw(p_stream->mask_path2, fdcnn_info + 2, (BOOL)(fdcnn_num >= 3), 0x00FF0000);
    fdcnn_mask_draw(p_stream->mask_path3, fdcnn_info + 3, (BOOL)(fdcnn_num >= 4), 0x00FF0000);

    return ret;
}
#endif

#if NN_PVDCNN_DRAW
int pvdcnn_draw_info(VENDOR_AIS_FLOW_MEM_PARM buf, VIDEO_STREAM *p_stream)
{
    HD_URECT size = {0, 0, OSG_LCD_WIDTH, OSG_LCD_HEIGHT};
	static PVDCNN_RSLT info[4] = {0};

    UINT32 num = pvdcnn_get_result(buf, info, &size, 4);

    pvdcnn_mask_draw(p_stream->mask_path4, info + 0, (BOOL)(num >= 1), 0x0000FF00);
    pvdcnn_mask_draw(p_stream->mask_path5, info + 1, (BOOL)(num >= 2), 0x0000FF00);
    pvdcnn_mask_draw(p_stream->mask_path6, info + 2, (BOOL)(num >= 3), 0x0000FF00);
    pvdcnn_mask_draw(p_stream->mask_path7, info + 3, (BOOL)(num >= 4), 0x0000FF00);

    return HD_OK;
}
#endif

static VENDOR_AIS_FLOW_MEM_PARM getmem(VENDOR_AIS_FLOW_MEM_PARM *valid_mem, UINT32 required_size)
{
	VENDOR_AIS_FLOW_MEM_PARM mem = {0};
	required_size = ALIGN_CEIL_4(required_size);
	if(required_size <= valid_mem->size) {
		mem.va = valid_mem->va;
        mem.pa = valid_mem->pa;
		mem.size = required_size;

		valid_mem->va += required_size;
        valid_mem->pa += required_size;
		valid_mem->size -= required_size;
	} else {
		printf("ERR : required size %d > total memory size %d\r\n", required_size, valid_mem->size);
	}
	return mem;
}

static INT32 get_mem_block(VOID)
{
	HD_RESULT                 ret = HD_OK;
	UINT32                    pa, va;
	HD_COMMON_MEM_VB_BLK      blk;
    UINT32 mem_size = 0;

#if NN_USE_DRAM2
    HD_COMMON_MEM_DDR_ID      ddr_id = DDR_ID1;
#else
    HD_COMMON_MEM_DDR_ID      ddr_id = DDR_ID0;
#endif

#if NN_FDCNN_FD_MODE
    mem_size += fdcnn_calcbuffsize(NN_FDCNN_FD_TYPE);
#endif
#if NN_PVDCNN_MODE
    mem_size += pvdcnn_calcbuffsize();
#endif

	/* Allocate parameter buffer */
	if (g_mem.va != 0) {
		DBG_DUMP("err: mem has already been inited\r\n");
		return -1;
	}
    blk = hd_common_mem_get_block(HD_COMMON_MEM_CNN_POOL, mem_size, ddr_id);
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		DBG_DUMP("hd_common_mem_get_block fail\r\n");
		ret =  HD_ERR_NG;
		goto exit;
	}
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		DBG_DUMP("not get buffer, pa=%08x\r\n", (int)pa);
		return -1;
	}
	va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, mem_size);
	g_blk_info[0] = blk;

	/* Release buffer */
	if (va == 0) {
		ret = hd_common_mem_munmap((void *)va, mem_size);
		if (ret != HD_OK) {
			DBG_DUMP("mem unmap fail\r\n");
			return ret;
		}
		return -1;
	}
	g_mem.pa = pa;
	g_mem.va = va;
	g_mem.size = mem_size;

exit:
	return ret;
}

static HD_RESULT release_mem_block(VOID)
{
	HD_RESULT ret = HD_OK;
    UINT32 mem_size = 0;

#if NN_FDCNN_FD_MODE
    mem_size += fdcnn_calcbuffsize(NN_FDCNN_FD_TYPE);
#endif
#if NN_PVDCNN_MODE
    mem_size += pvdcnn_calcbuffsize();
#endif

	/* Release in buffer */
	if (g_mem.va) {
		ret = hd_common_mem_munmap((void *)g_mem.va, mem_size);
		if (ret != HD_OK) {
			DBG_DUMP("mem_uninit : (g_mem.va)hd_common_mem_munmap fail.\r\n");
			return ret;
		}
	}
	//ret = hd_common_mem_release_block((HD_COMMON_MEM_VB_BLK)g_mem.pa);
	ret = hd_common_mem_release_block(g_blk_info[0]);
	if (ret != HD_OK) {
		DBG_DUMP("mem_uninit : (g_mem.pa)hd_common_mem_release_block fail.\r\n");
		return ret;
	}

	return ret;
}

static HD_RESULT close_module_extend(VIDEO_STREAM *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_close(p_stream->proc_alg_path)) != HD_OK)
		return ret;
	return HD_OK;
}


VOID *fdcnn_fd_thread(VOID *arg)
{
    THREAD_PARM *p_fd_parm = (THREAD_PARM*)arg;
    VIDEO_STREAM stream = p_fd_parm->stream;
    VENDOR_AIS_FLOW_MEM_PARM fd_mem = p_fd_parm->mem;

    HD_VIDEO_FRAME video_frame = {0};
    HD_RESULT ret = HD_OK;

#if NN_FDCNN_FD_DUMP
    static FDCNN_RESULT fdcnn_info[FDCNN_MAX_OUTNUM] = {0};
    UINT32 fdcnn_num = 0, i;
    static HD_URECT fdcnn_size = {0, 0, MAX_CAP_SIZE_W, MAX_CAP_SIZE_H};
#endif

#if NN_FDCNN_FD_PROF
    static struct timeval tstart0, tend0;
    static UINT64 cur_time0 = 0, mean_time0 = 0, sum_time0 = 0;
    static UINT32 icount = 0;
#endif

    UINT32 fdcnn_buf_size = fdcnn_calcbuffsize(NN_FDCNN_FD_TYPE);
    UINT32 fdcnn_cachebuf_size = fdcnn_calccachebuffsize(NN_FDCNN_FD_TYPE);

    VENDOR_AIS_FLOW_MEM_PARM fdcnn_buf  = getmem(&fd_mem, fdcnn_buf_size);
    VENDOR_AIS_FLOW_MEM_PARM fdcnn_cachebuf = getmem(&fd_mem, fdcnn_cachebuf_size);

    if (NN_FDCNN_FD_TYPE == FDCNN_NETWORK_V10) // FDCNN_NETWORK_V10 need 4 file
    {
	    CHAR file_path[4][256] =  {      "/mnt/sd/CNNLib/para/fdcnn_method1/file1.bin", \
	                                     "/mnt/sd/CNNLib/para/fdcnn_method1/file2.bin", \
	                                     "/mnt/sd/CNNLib/para/fdcnn_method1/file3.bin", \
	                                     "/mnt/sd/CNNLib/para/fdcnn_method1/file4.bin"  };

	    UINT32 model_addr_1 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_1, NN_FDCNN_FD_TYPE);
	    UINT32 model_addr_2 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_2, NN_FDCNN_FD_TYPE);
	    UINT32 model_addr_3 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_3, NN_FDCNN_FD_TYPE);
	    UINT32 model_addr_4 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_4, NN_FDCNN_FD_TYPE);
	    load_file(file_path[0], model_addr_1);
	    load_file(file_path[1], model_addr_2);
	    load_file(file_path[2], model_addr_3);
	    load_file(file_path[3], model_addr_4);
    }
    else if (NN_FDCNN_FD_TYPE == FDCNN_NETWORK_V20) // FDCNN_NETWORK_V20 need 1 file
    {
        CHAR file_path[256] =  "/mnt/sd/CNNLib/para/fdcnn_method2/file1.bin";
        UINT32 model_addr_1 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_1, NN_FDCNN_FD_TYPE);
        load_file(file_path, model_addr_1);
    }
    else
    {
        DBG_ERR("Not support net type %d !\r\n", NN_FDCNN_FD_TYPE);
        return 0;
    }

    ret = fdcnn_init(fdcnn_buf, fdcnn_cachebuf, NN_FDCNN_FD_TYPE);
    if (ret != HD_OK)
    {
        DBG_ERR("fdcnn_init fail=%d\n", ret);
        return 0;
    }

    while(!fdcnn_exit)
    {
		if (!ai_start){
			usleep(10);
			continue;
		}

        ret = hd_videoproc_pull_out_buf(stream.proc_alg_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
        if(ret != HD_OK)
        {
            DBG_ERR("fdnn hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
            goto exit;
        }

        // init image
        HD_GFX_IMG_BUF input_image;
        input_image.dim.w  = video_frame.dim.w;
        input_image.dim.h = video_frame.dim.h;
        input_image.format = HD_VIDEO_PXLFMT_YUV420;
        input_image.p_phy_addr[0] = video_frame.phy_addr[0];
        input_image.p_phy_addr[1] = video_frame.phy_addr[1];
        input_image.p_phy_addr[2] = video_frame.phy_addr[1]; // for avoid hd_gfx_scale message
        input_image.lineoffset[0] = video_frame.loff[0];
        input_image.lineoffset[1] = video_frame.loff[0]; // for avoid hd_gfx_scale message
        input_image.lineoffset[2] = video_frame.loff[0]; // for avoid hd_gfx_scale message

        ret = fdcnn_set_image(fdcnn_buf, &input_image);
        if (ret != HD_OK)
        {
            DBG_ERR("fdcnn_set_image fail=%d\n", ret);
            goto exit;
        }

        ret = hd_videoproc_release_out_buf(stream.proc_alg_path, &video_frame);
        if(ret != HD_OK)
        {
            DBG_ERR("hd_videoproc_release_out_buf fail (%d)\n\r", ret);
            goto exit;
        }

#if NN_FDCNN_FD_PROF
        gettimeofday(&tstart0, NULL);
#endif

        ret = fdcnn_process(fdcnn_buf);
        if (ret != HD_OK)
        {
            DBG_ERR("fdcnn_process fail=%d\n", ret);
            goto exit;
        }

#if NN_FDCNN_FD_PROF
        gettimeofday(&tend0, NULL);
        cur_time0 = (UINT64)(tend0.tv_sec - tstart0.tv_sec) * 1000000 + (tend0.tv_usec - tstart0.tv_usec);
        sum_time0 += cur_time0;
        mean_time0 = sum_time0/(++icount);
        #if (!NN_FDCNN_FD_FIX_FRM)
        printf("[FD] process cur time(us): %lld, mean time(us): %lld\r\n", cur_time0, mean_time0);
        #endif
#endif

#if NN_FDCNN_FD_FIX_FRM
        if (cur_time0 < 100000)
		    usleep(100000 - cur_time0 + (mean_time0*0));
#endif

#if NN_FDCNN_FD_DUMP
        fdcnn_size.w = video_frame.dim.w;
        fdcnn_size.h = video_frame.dim.h;
        fdcnn_num = fdcnn_getresults(fdcnn_buf, fdcnn_info, &fdcnn_size, FDCNN_MAX_OUTNUM);
#if 1
        printf("[FD2] ----------- num : %ld ----------- \n", fdcnn_num);
        for(i = 0; i < fdcnn_num; i++ )
        {
            printf("[FD] %ld\t%ld\t%ld\t%ld\t%ld\r\n", fdcnn_info[i].x, fdcnn_info[i].y, fdcnn_info[i].w, fdcnn_info[i].h, fdcnn_info[i].score);
        }
#endif

#endif

	cnn_fill_alg_info(fdcnn_buf, FD);

    }
exit:
    ret = fdcnn_uninit(fdcnn_buf);
    if (ret != HD_OK)
    {
        DBG_ERR("fdcnn_uninit fail=%d\n", ret);
    }
    return 0;
}

VOID *pvdcnn_thread(VOID *arg)
{
    THREAD_PARM *p_parm = (THREAD_PARM*)arg;
    VIDEO_STREAM stream = p_parm->stream;
    VENDOR_AIS_FLOW_MEM_PARM mem = p_parm->mem;

    HD_VIDEO_FRAME video_frame = {0};
    HD_RESULT ret = HD_OK;


#if NN_PVDCNN_PROF
    static struct timeval tstart0, tend0;
    static UINT64 cur_time0 = 0, mean_time0 = 0, sum_time0 = 0;
    static UINT32 icount = 0;
#endif

    UINT32 buf_size = pvdcnn_calcbuffsize();

    VENDOR_AIS_FLOW_MEM_PARM buf = getmem(&mem, buf_size);
	printf("buf: va = %0x, pa = %0x, size = %ld\r\n", buf.va, buf.pa, buf.size);

    UINT32 read_size = load_file("/mnt/sd/CNNLib/para/pvdcnn/nvt_model.bin", buf.va);
	printf("read model size = %ld\r\n", read_size);

	PVDCNN_INIT_PRMS init_prms;
	init_prms.net_id = 4;
	init_prms.conf_thr = 0.3;
	init_prms.conf_thr2 = 0.3;
	init_prms.nms_thr = 0.2;

    ret = pvdcnn_init(buf, &init_prms);
	if (ret != HD_OK) {
		printf("pvdcnn_init fail\r\n");
		return 0;
	}

    while (!pvdcnn_exit)
	{
		if (!ai_start){
			usleep(10);
			continue;
		}

        ret = hd_videoproc_pull_out_buf(stream.proc_alg_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
        if (ret != HD_OK) {
            printf("pvd ERR : hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
            break;
        }

        // init image
        HD_GFX_IMG_BUF input_image;
        input_image.dim.w  = video_frame.dim.w;
        input_image.dim.h = video_frame.dim.h;
        input_image.format = HD_VIDEO_PXLFMT_YUV420;
        input_image.p_phy_addr[0] = video_frame.phy_addr[0];
        input_image.p_phy_addr[1] = video_frame.phy_addr[1];
        input_image.p_phy_addr[2] = video_frame.phy_addr[1]; // for avoid hd_gfx_scale message
        input_image.lineoffset[0] = video_frame.loff[0];
        input_image.lineoffset[1] = video_frame.loff[0]; // for avoid hd_gfx_scale message
        input_image.lineoffset[2] = video_frame.loff[0]; // for avoid hd_gfx_scale message

        ret = pvdcnn_set_img(buf, &input_image);
		if (ret != HD_OK) {
			printf("pvdcnn_set_img fail\r\n");
			break;
		}

        ret = hd_videoproc_release_out_buf(stream.proc_alg_path, &video_frame);
        if (ret != HD_OK) {
            printf("ERR : hd_videoproc_release_out_buf fail (%d)\n\r", ret);
            break;
        }

#if NN_PVDCNN_PROF
        gettimeofday(&tstart0, NULL);
#endif

        ret = pvdcnn_process(buf);
		if (ret != HD_OK) {
			printf("pvdcnn_process fail\r\n");
			break;
		}


#if NN_PVDCNN_PROF
        gettimeofday(&tend0, NULL);
        cur_time0 = (UINT64)(tend0.tv_sec - tstart0.tv_sec) * 1000000 + (tend0.tv_usec - tstart0.tv_usec);
        sum_time0 += cur_time0;
        mean_time0 = sum_time0 / (++icount);
        #if (!NN_PVDCNN_FIX_FRM)
        printf("[PVD] process cur time(us): %lld, mean time(us): %lld\r\n", cur_time0, mean_time0);
        #endif
#endif

#if NN_PVDCNN_FIX_FRM
        if (cur_time0 < 100000)
            usleep(100000 - cur_time0 + (mean_time0*0));
#endif

#if NN_PVDCNN_DUMP
		HD_URECT ref_size = {0, 0, MAX_CAP_SIZE_W, MAX_CAP_SIZE_H};
        ref_size.w = video_frame.dim.w;
        ref_size.h = video_frame.dim.h;

		static PVDCNN_RSLT rslt[16] = {0};
        UINT32 rslt_num = pvdcnn_get_result(buf, rslt, &ref_size, 16);

        pvdcnn_print_rslt(rslt, rslt_num, "PVD");
#endif

	cnn_fill_alg_info(buf, PD);

    }
    ret = pvdcnn_uninit(buf);
	if (ret != HD_OK) {
		printf("pvdcnn_uninit fail\r\n");
	}
    return 0;
}


VOID *draw_thread(VOID *arg)
{
    THREAD_DRAW_PARM *p_draw_parm = (THREAD_DRAW_PARM*)arg;
    VIDEO_STREAM stream = p_draw_parm->stream;
#if NN_FDCNN_FD_DRAW
    VENDOR_AIS_FLOW_MEM_PARM fd_mem = p_draw_parm->fd_mem;
#endif
#if NN_PVDCNN_DRAW
    VENDOR_AIS_FLOW_MEM_PARM pvd_mem = p_draw_parm->pvd_mem;
#endif
    HD_VIDEO_FRAME video_frame = {0};
    HD_RESULT ret = HD_OK;

    // wait fd ro pvd init ready
    sleep(2);

    while(!draw_exit)
    {

		if (!ai_start){
			usleep(10);
			continue;
		}
        ret = hd_videoproc_pull_out_buf(stream.proc_alg_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
        if(ret != HD_OK)
        {
            printf("fd ERR : hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
            break;
        }

#if NN_FDCNN_FD_DRAW
        fdcnn_fd_draw_info(fd_mem, &stream);
#endif

#if NN_PVDCNN_DRAW
        pvdcnn_draw_info(pvd_mem, &stream);
#endif

        ret = hd_videoproc_release_out_buf(stream.proc_alg_path, &video_frame);
        if(ret != HD_OK)
        {
            printf("ERR : hd_videoproc_release_out_buf fail (%d)\n\r", ret);
            break;
        }
    }

    return 0;
}
#endif//ALG_FD_PD
#if HID_FUNC
static void write_hid_test(UINT8 *p_buf, UINT32 buf_size, INT32 timeout)
{
	INT32 ret_value;

	memcpy((void *)hid_write_buf.va, p_buf, buf_size);
	//--- data is written by CPU, flush CPU cache to PHY memory ---
	hd_common_mem_flush_cache((void *)hid_write_buf.va, buf_size);
	printf("Write HID %d byte(%d)\r\n", buf_size, timeout);
	ret_value = UVAC_WriteHidData((void *)hid_write_buf.pa, buf_size, timeout);
	printf("Write HID ret %d\r\n", ret_value);
}
static void read_hid_test(UINT32 buf_size, INT32 timeout)
{
	INT32 i;
	UINT8 *pBuf;
	INT32 ret_value;

	printf("Read HID %d byte(%d)\r\n", buf_size, timeout);
	ret_value = UVAC_ReadHidData((UINT8 *)hid_read_buf.pa, buf_size, timeout);
	printf("Read HID ret %d\r\n", ret_value);

	if (ret_value > 0) {
		vendor_common_mem_cache_sync((void *)hid_read_buf.va, ret_value, VENDOR_COMMON_MEM_DMA_FROM_DEVICE);
		//dump buffer
		pBuf = (UINT8 *)hid_read_buf.va;
		for (i = 0; i < ret_value; i++) {
			if (i % 16 == 0) {
				printf("\r\n");
			}
			printf("0x%02X ", *(pBuf + i));
		}
		printf("\r\n");
	}
}
static INT32 get_hid_mem_block(VOID)
{
	HD_RESULT hd_ret;
	HD_COMMON_MEM_DDR_ID      ddr_id = DDR_ID0;
	UINT32 pa;
	void *va;

	hid_read_buf.size = HID_READ_BUFFER_SIZE;
	if ((hd_ret = hd_common_mem_alloc("hid_read", &pa, (void **)&va, hid_read_buf.size, ddr_id)) != HD_OK) {
		printf("hd_common_mem_alloc hid_read_buf failed(%d)\r\n", hd_ret);
		hid_read_buf.va = 0;
		hid_read_buf.pa = 0;
		hid_read_buf.size = 0;
	} else {
		hid_read_buf.va = (UINT32)va;
		hid_read_buf.pa = (UINT32)pa;
	}

	hid_write_buf.size = HID_WRITE_BUFFER_SIZE;
	if ((hd_ret |= hd_common_mem_alloc("hid_write", &pa, (void **)&va, hid_write_buf.size, ddr_id)) != HD_OK) {
		printf("hd_common_mem_alloc hid_write_buf failed(%d)\r\n", hd_ret);
		hid_write_buf.va = 0;
		hid_write_buf.pa = 0;
		hid_write_buf.size = 0;
	} else {
		hid_write_buf.va = (UINT32)va;
		hid_write_buf.pa = (UINT32)pa;
	}

	return hd_ret;
}

static HD_RESULT release_hid_mem_block(VOID)
{
	HD_RESULT ret = HD_OK;

	/* Release in buffer */
	if (hid_read_buf.va) {
		ret = hd_common_mem_free(hid_read_buf.pa, (void *) hid_read_buf.va);
		if (ret != HD_OK) {
			printf("mem_uninit : (hid_read_buf.va)hd_common_mem_free fail.\r\n");
			return ret;
		}
	}
	if (hid_write_buf.va) {
		ret = hd_common_mem_free(hid_write_buf.pa, (void *) hid_write_buf.va);
		if (ret != HD_OK) {
			printf("mem_uninit : (hid_write_buf.va)hd_common_mem_free fail.\r\n");
			return ret;
		}
	}


	return ret;
}
#endif// HID_FUNC

#if CDC_FUNC
static void write_cdc_test(UINT8 *p_buf, UINT32 buf_size, INT32 timeout)
{
	INT32 ret_value;

	memcpy((void *)cdc_write_buf.va, p_buf, buf_size);
	//--- data is written by CPU, flush CPU cache to PHY memory ---
	hd_common_mem_flush_cache((void *)cdc_write_buf.va, buf_size);
	printf("Write CDC %d byte(%d)\r\n", buf_size, timeout);
	ret_value = UVAC_WriteCdcData(CDC_COM_1ST, (void *)cdc_write_buf.pa, buf_size, timeout);
	printf("Write CDC ret %d\r\n", ret_value);
}
static void read_cdc_test(UINT32 buf_size, INT32 timeout)
{
	INT32 i;
	UINT8 *pBuf;
	INT32 ret_value;

	printf("Read CDC %d byte(%d)\r\n", buf_size, timeout);
	ret_value = UVAC_ReadCdcData(CDC_COM_1ST, (UINT8 *)cdc_read_buf.pa, buf_size, timeout);
	printf("Read CDC ret %d\r\n", ret_value);

	if (ret_value > 0) {
		vendor_common_mem_cache_sync((void *)cdc_read_buf.va, ret_value, VENDOR_COMMON_MEM_DMA_FROM_DEVICE);
		//dump buffer
		pBuf = (UINT8 *)cdc_read_buf.va;
		*(pBuf + ret_value) = '\0';
		printf("%s ", pBuf);
		printf("\r\n");
	}
}
static INT32 get_cdc_mem_block(VOID)
{
	HD_RESULT hd_ret;
	HD_COMMON_MEM_DDR_ID      ddr_id = DDR_ID0;
	UINT32 pa;
	void *va;

	cdc_read_buf.size = CDC_READ_BUFFER_SIZE;
	if ((hd_ret = hd_common_mem_alloc("cdc_read", &pa, (void **)&va, cdc_read_buf.size, ddr_id)) != HD_OK) {
		printf("hd_common_mem_alloc cdc_read_buf failed(%d)\r\n", hd_ret);
		cdc_read_buf.va = 0;
		cdc_read_buf.pa = 0;
		cdc_read_buf.size = 0;
	} else {
		cdc_read_buf.va = (UINT32)va;
		cdc_read_buf.pa = (UINT32)pa;
	}

	cdc_write_buf.size = CDC_WRITE_BUFFER_SIZE;
	if ((hd_ret |= hd_common_mem_alloc("cdc_write", &pa, (void **)&va, cdc_write_buf.size, ddr_id)) != HD_OK) {
		printf("hd_common_mem_alloc cdc_write_buf failed(%d)\r\n", hd_ret);
		cdc_write_buf.va = 0;
		cdc_write_buf.pa = 0;
		cdc_write_buf.size = 0;
	} else {
		cdc_write_buf.va = (UINT32)va;
		cdc_write_buf.pa = (UINT32)pa;
	}

	return hd_ret;
}

static HD_RESULT release_cdc_mem_block(VOID)
{
	HD_RESULT ret = HD_OK;

	/* Release in buffer */
	if (cdc_read_buf.va) {
		ret = hd_common_mem_free(cdc_read_buf.pa, (void *) cdc_read_buf.va);
		if (ret != HD_OK) {
			printf("mem_uninit : (cdc_read_buf.va)hd_common_mem_free fail.\r\n");
			return ret;
		}
	}
	if (cdc_write_buf.va) {
		ret = hd_common_mem_free(cdc_write_buf.pa, (void *) cdc_write_buf.va);
		if (ret != HD_OK) {
			printf("mem_uninit : (cdc_write_buf.va)hd_common_mem_free fail.\r\n");
			return ret;
		}
	}


	return ret;
}
#endif// CDC_FUNC

static void app_main(int argc, char* argv[])
{
	HD_RESULT ret;
	INT key;
#if UVC_SUPPORT_YUV_FMT
	UINT32 i = 0;
#endif

	UINT32 enc_type = 1;//0:h265, 1:h264, 2:MJPG
	HD_DIM main_dim, ref_dim;
#if MULTI_STREAM
	HD_DIM sub1_dim;
	HD_DIM sub2_dim;
#endif
#if ALG_FD_PD
	AET_CFG_INFO cfg_info = {0};
#endif
	// query program options
	if (argc == 2) {
		enc_type = atoi(argv[1]);
		printf("enc_type %d\r\n", enc_type);
		if(enc_type > 2) {
			printf("error: not support enc_type!\r\n");
		}
	}
	stream[0].codec_type = enc_type;
#if ALG_FD_PD
	init_people_detection();
	init_face_detection();
#endif

	AET_CFG_INFO cfg_info = {0};
	if (vendor_isp_init() != HD_ERR_NG) {
		cfg_info.id = 0;
		//strncpy(cfg_info.path, "/mnt/app/isp/sc8238_20200916-9.cfg", CFG_NAME_LENGTH);
		//printf("sensor 1 load /mnt/app/isp/sc8238_20200916-9.cfg\n");
		vendor_isp_set_ae(AET_ITEM_RLD_CONFIG, &cfg_info);
		vendor_isp_set_awb(AWBT_ITEM_RLD_CONFIG, &cfg_info);
		vendor_isp_set_iq(IQT_ITEM_RLD_CONFIG, &cfg_info);
		vendor_isp_uninit();
	}

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
	}

#if ALG_FD_PD
	//set project config for AI
	hd_common_sysconfig(0, (1<<16), 0, VENDOR_AI_CFG); //enable AI engine
#endif

	// init memory
#if (MSDC_FUNC == 2)
	mempool_init();
#endif
#if ALG_FD_PD
	ret = mem_init(stream[0].stamp_size);
#else
	ret = mem_init();
#endif
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		goto exit;
	}

#if ALG_FD_PD
	ret = get_mem_block();
	if (ret != HD_OK) {
		DBG_ERR("mem_init fail=%d\n", ret);
		goto exit;
	}
#endif
#if HID_FUNC
	ret = get_hid_mem_block();
	if (ret != HD_OK) {
		printf("hid_mem_init fail=%d\n", ret);
		goto exit;
	}
#endif

#if CDC_FUNC
	ret = get_cdc_mem_block();
	if (ret != HD_OK) {
		printf("cdc_mem_init fail=%d\n", ret);
		goto exit;
	}
#endif

	// init all modules
	ret = init_module();
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}

#if UVC_SUPPORT_YUV_FMT
	ret = hd_gfx_init();
	if(ret != HD_OK) {
        printf("init gfx fail=%d\n", ret);
	}
#endif

	// open VIDEO_STREAM modules (main)
	stream[0].proc_max_dim.w = (MAX_CAP_SIZE_W > MAX_BS_W) ? MAX_CAP_SIZE_W : MAX_BS_W; //assign by user
	stream[0].proc_max_dim.h = (MAX_CAP_SIZE_H > MAX_BS_H) ? MAX_CAP_SIZE_H : MAX_BS_H; //assign by user

	ret = open_module(&stream[0], &stream[0].proc_max_dim);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

#if ALG_FD_PD

    stream[1].proc_max_dim.w = MAX_CAP_SIZE_H; //assign by user
	stream[1].proc_max_dim.h = MAX_CAP_SIZE_W; //assign by user
	ret = open_module_extend1(&stream[1], &stream[1].proc_max_dim);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

    stream[2].proc_max_dim.w = MAX_CAP_SIZE_W; //assign by user
	stream[2].proc_max_dim.h = MAX_CAP_SIZE_H; //assign by user
	ret = open_module_extend2(&stream[2], &stream[2].proc_max_dim);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

#if NN_FDCNN_FD_DRAW
	// must set once before hd_videoout_start
	if(init_mask_param(stream[0].mask_path0)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
	if(init_mask_param(stream[0].mask_path1)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
    if(init_mask_param(stream[0].mask_path2)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
    if(init_mask_param(stream[0].mask_path3)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
#endif

#if NN_PVDCNN_DRAW
    if(init_mask_param(stream[0].mask_path4)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
    if(init_mask_param(stream[0].mask_path5)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
    if(init_mask_param(stream[0].mask_path6)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
    if(init_mask_param(stream[0].mask_path7)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
#endif

#endif	//ALG_FD_PD

#if AUDIO_ON
	//open capture module
	au_cap.sample_rate_max = (gUIUvacAudSampleRate[0] == NVT_UI_FREQUENCY_32K) ? HD_AUDIO_SR_32000 : HD_AUDIO_SR_16000; //assign by user
	ret = open_module2(&au_cap);
	if(ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	//set audiocap parameter
	au_cap.sample_rate = (gUIUvacAudSampleRate[0] == NVT_UI_FREQUENCY_32K) ? HD_AUDIO_SR_32000 : HD_AUDIO_SR_16000; //assign by user
	ret = set_cap_param2(au_cap.cap_path, au_cap.sample_rate);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}
#endif

#if ALG_FD_PD
	//render mask
#if NN_FDCNN_FD_DRAW
	ret = hd_videoproc_start(stream[0].mask_path0);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoproc_start(stream[0].mask_path1);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoproc_start(stream[0].mask_path2);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoproc_start(stream[0].mask_path3);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
#endif

#if NN_PVDCNN_DRAW
	ret = hd_videoproc_start(stream[0].mask_path4);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoproc_start(stream[0].mask_path5);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoproc_start(stream[0].mask_path6);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoproc_start(stream[0].mask_path7);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
#endif

#endif //ALG_FD_PD

	// get videocap capability
	ret = get_cap_caps(stream[0].cap_ctrl, &stream[0].cap_syscaps);
	if (ret != HD_OK) {
		printf("get cap-caps fail=%d\n", ret);
		goto exit;
	}

	// set videocap parameter
	stream[0].cap_dim.w = MAX_CAP_SIZE_W; //assign by user
	stream[0].cap_dim.h = MAX_CAP_SIZE_H; //assign by user
	ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim, MAX_BS_FPS);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	// assign parameter by program options
	main_dim.w = MAX_BS_W;
	main_dim.h = MAX_BS_H;
	ref_dim.w = MAX_CAP_SIZE_W;
	ref_dim.h = MAX_CAP_SIZE_H;
#if MULTI_STREAM
	sub1_dim.w = 1280;
	sub1_dim.h = 720;
	sub2_dim.w = 640;
	sub2_dim.h = 360;
#endif

	// set videoproc parameter (main)
	ret = set_proc_param(stream[0].proc_main_path, &main_dim, 0);
#if MULTI_STREAM
	ret = set_proc_param(stream[0].proc_sub1_path, &sub1_dim, 0);
	ret = set_proc_param(stream[0].proc_sub2_path, &sub2_dim, 0);
#endif
	if (ret != HD_OK) {
		printf("1111set proc fail=%d\n", ret);
		goto exit;
	}
	ret = set_proc_param(stream[0].ref_path, &ref_dim, 0);
	if (ret != HD_OK) {
		printf("ref proc fail=%d\n", ret);
		goto exit;
	}

#if ALG_FD_PD
	// set videoproc parameter (alg)
	stream[0].proc_alg_max_dim.w = MAX_CAP_SIZE_W;
	stream[0].proc_alg_max_dim.h = MAX_CAP_SIZE_H;
	ret = set_proc_param(stream[0].proc_alg_path, &stream[0].proc_alg_max_dim, 0);
	if (ret != HD_OK) {
		printf("set proc alg fail=%d\n", ret);
		goto exit;
	}

	stream[1].proc_alg_max_dim.w = MAX_CAP_SIZE_W;
	stream[1].proc_alg_max_dim.h = MAX_CAP_SIZE_H;
    ret = set_proc_param_extend(stream[1].proc_alg_path, HD_VIDEOPROC_0_OUT_1, &stream[1].proc_alg_max_dim, HD_VIDEO_DIR_NONE);
	if (ret != HD_OK) {
		printf("yyyset proc fail=%d\n", ret);
		goto exit;
	}

	stream[2].proc_alg_max_dim.w = MAX_CAP_SIZE_W;
	stream[2].proc_alg_max_dim.h = MAX_CAP_SIZE_H;
    ret = set_proc_param_extend(stream[2].proc_alg_path, HD_VIDEOPROC_0_OUT_1, &stream[2].proc_alg_max_dim, HD_VIDEO_DIR_NONE);
	if (ret != HD_OK) {
		printf("xxxset proc fail=%d\n", ret);
		goto exit;
	}
#endif

	// set videoenc config (main)
	stream[0].enc_main_max_dim.w = MAX_BS_W;
	stream[0].enc_main_max_dim.h = MAX_BS_H;
#if MULTI_STREAM
	stream[0].enc_sub1_max_dim.w = 1280;
	stream[0].enc_sub1_max_dim.h = 720;
	stream[0].enc_sub2_max_dim.w = 640;
	stream[0].enc_sub2_max_dim.h = 360;
#endif
	ret = set_enc_cfg(stream[0].enc_main_path, &stream[0].enc_main_max_dim, 96 * 1024 * 1024);
#if MULTI_STREAM
	ret = set_enc_cfg(stream[0].enc_sub1_path, &stream[0].enc_sub1_max_dim, 32 * 1024 * 1024);
	ret = set_enc_cfg(stream[0].enc_sub2_path, &stream[0].enc_sub2_max_dim, 16 * 1024 * 1024);
#endif
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}

	// set videoenc parameter (main)
	stream[0].enc_main_dim.w = main_dim.w;
	stream[0].enc_main_dim.h = main_dim.h;
#if MULTI_STREAM
	stream[0].enc_sub1_dim.w = 1280;
	stream[0].enc_sub1_dim.h = 720;
	stream[0].enc_sub2_dim.w = 640;
	stream[0].enc_sub2_dim.h = 360;
#endif
	ret = set_enc_param(stream[0].enc_main_path, &stream[0].enc_main_dim, enc_type, 16 * 1024 * 1024);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}
#if MULTI_STREAM
	ret = set_enc_param(stream[0].enc_sub1_path, &stream[0].enc_sub1_dim, enc_type, 32 * 1024 * 1024);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}
	ret = set_enc_param(stream[0].enc_sub2_path, &stream[0].enc_sub2_dim, enc_type, 16 * 1024 * 1024);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}
#endif

	// bind VIDEO_STREAM modules (main)
	hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOENC_0_IN_0);
#if MULTI_STREAM
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_1, HD_VIDEOENC_0_IN_1);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_2, HD_VIDEOENC_0_IN_2);
#endif

#if AUDIO_ON
	//audio create capture thread
	THREAD_CREATE(cap_thread_id, capture_thread, (void *)&au_cap, "capture_thread");
	if (0 == cap_thread_id) {
		printf("create encode thread failed");
		goto exit;
	}
	THREAD_RESUME(cap_thread_id);
	//start audio capture module
	hd_audiocap_start(au_cap.cap_path);
#endif

#if  UAC_RX_ON
	ret = pthread_create(&uac_thread_id, NULL, uac_thread, (void *)&au_cap);
	if (ret < 0) {
		printf("create encode thread failed");
		goto exit;
	}
#endif

	// start VIDEO_STREAM modules (main)
	if (g_capbind == 1) {
		//direct NOTE: ensure videocap start after 1st videoproc phy path start
		hd_videoproc_start(stream[0].proc_main_path);
#if MULTI_STREAM
		hd_videoproc_start(stream[0].proc_sub1_path);
		hd_videoproc_start(stream[0].proc_sub2_path);
#endif
		if ((main_dim.w!=ref_dim.w) || (main_dim.h!=ref_dim.h)){
			hd_videoproc_start(stream[0].ref_path);
		}
		hd_videocap_start(stream[0].cap_path);
	} else {
		hd_videocap_start(stream[0].cap_path);
		hd_videoproc_start(stream[0].proc_main_path);
#if MULTI_STREAM
		hd_videoproc_start(stream[0].proc_sub1_path);
		hd_videoproc_start(stream[0].proc_sub2_path);
#endif
		if ((main_dim.w!=ref_dim.w) || (main_dim.h!=ref_dim.h)){
			hd_videoproc_start(stream[0].ref_path);
		}
	}

#if ALG_FD_PD
	hd_videoproc_start(stream[0].proc_alg_path);
    hd_videoproc_start(stream[1].proc_alg_path);
    hd_videoproc_start(stream[2].proc_alg_path);
#endif
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	//sleep(1);
	hd_videoenc_start(stream[0].enc_main_path);
#if MULTI_STREAM
	hd_videoenc_start(stream[0].enc_sub1_path);
	hd_videoenc_start(stream[0].enc_sub2_path);
#endif

	// create encode_thread (pull_out bitstream)
	THREAD_CREATE(stream[0].enc_thread_id, encode_thread, (void *)stream, "encode_thread");
	if (0 == stream[0].enc_thread_id) {
		printf("create encode thread failed");
		goto exit;
	}
	THREAD_RESUME(stream[0].enc_thread_id);

#if ALG_FD_PD
	VENDOR_AIS_FLOW_MEM_PARM local_mem = g_mem;
	printf("total mem size = %ld\r\n", local_mem.size);

	    // main process
#if NN_FDCNN_FD_MODE
    THREAD_PARM fd_thread_parm;
    pthread_t fd_thread_id;
    UINT32 fd_mem_size = fdcnn_calcbuffsize(NN_FDCNN_FD_TYPE);

    fd_thread_parm.mem = getmem(&local_mem, fd_mem_size);
    fd_thread_parm.stream = stream[1];

#if NN_FDCNN_HIGH_PRIORITY
	pthread_attr_t fd_attr;
	struct sched_param fd_param;

	pthread_attr_init(&fd_attr);
	fd_param.sched_priority = 99-3;//99-3;
	pthread_attr_setschedpolicy(&fd_attr,SCHED_RR);
	pthread_attr_setschedparam(&fd_attr,&fd_param);
	pthread_attr_setinheritsched(&fd_attr,PTHREAD_EXPLICIT_SCHED);

	ret = pthread_create(&fd_thread_id, &fd_attr, fdcnn_fd_thread, (VOID*)(&fd_thread_parm));
#else //NN_FDCNN_HIGH_PRIORITY
	ret = pthread_create(&fd_thread_id, NULL, fdcnn_fd_thread, (VOID*)(&fd_thread_parm));
#endif //NN_FDCNN_HIGH_PRIORITY
    if (ret < 0) {
        DBG_ERR("create fdcnn fd thread failed");
        goto exit;
    }
#endif //NN_FDCNN_FD_MODE

#if NN_PVDCNN_MODE
    THREAD_PARM pvd_thread_parm;
    pthread_t pvd_thread_id;
    UINT32 pvd_mem_size = pvdcnn_calcbuffsize();

    pvd_thread_parm.mem = getmem(&local_mem, pvd_mem_size);
    pvd_thread_parm.stream = stream[2];

#if NN_PVDCNN_HIGH_PRIORITY
	pthread_attr_t pvd_attr;
	struct sched_param pvd_param;

	pthread_attr_init(&pvd_attr);
	pvd_param.sched_priority = 99-3;//99-3;
	pthread_attr_setschedpolicy(&pvd_attr,SCHED_RR);
	pthread_attr_setschedparam(&pvd_attr,&pvd_param);
	pthread_attr_setinheritsched(&pvd_attr,PTHREAD_EXPLICIT_SCHED);

	ret = pthread_create(&pvd_thread_id, &pvd_attr, pvdcnn_thread, (VOID*)(&pvd_thread_parm));
#else //NN_PVDCNN_HIGH_PRIORITY
	ret = pthread_create(&pvd_thread_id, NULL, pvdcnn_thread, (VOID*)(&pvd_thread_parm));
#endif //NN_PVDCNN_HIGH_PRIORITY
    if (ret < 0) {
        DBG_ERR("create pvdcnn thread failed");
        goto exit;
    }
#endif //NN_PVDCNN_MODE

#if (NN_FDCNN_FD_DRAW || NN_PVDCNN_DRAW)
    THREAD_DRAW_PARM fdcnn_pvdcnn_draw_parm;
    pthread_t fdcnn_pvdcnn_draw_id;
    fdcnn_pvdcnn_draw_parm.stream = stream[0];

#if NN_FDCNN_FD_DRAW
    fdcnn_pvdcnn_draw_parm.fd_mem = fd_thread_parm.mem;
#endif
#if NN_PVDCNN_DRAW
    fdcnn_pvdcnn_draw_parm.pvd_mem = pvd_thread_parm.mem;
#endif

#if (NN_FDCNN_HIGH_PRIORITY || NN_PVDCNN_HIGH_PRIORITY)
    pthread_attr_t draw_attr;
    struct sched_param draw_param;

    pthread_attr_init(&draw_attr);
    draw_param.sched_priority = 99-3;//99-3;
    pthread_attr_setschedpolicy(&draw_attr,SCHED_RR);
    pthread_attr_setschedparam(&draw_attr,&draw_param);
    pthread_attr_setinheritsched(&draw_attr,PTHREAD_EXPLICIT_SCHED);

    ret = pthread_create(&fdcnn_pvdcnn_draw_id, &draw_attr, draw_thread, (VOID*)(&fdcnn_pvdcnn_draw_parm));
#else//(NN_FDCNN_HIGH_PRIORITY || NN_PVDCNN_HIGH_PRIORITY)
    ret = pthread_create(&fdcnn_pvdcnn_draw_id, NULL, draw_thread, (VOID*)(&fdcnn_pvdcnn_draw_parm));
#endif//(NN_FDCNN_HIGH_PRIORITY || NN_PVDCNN_HIGH_PRIORITY)
    if (ret < 0) {
        DBG_ERR("create fdcnn pvdcnn draw thread failed");
        goto exit;
    }
#endif //(NN_FDCNN_FD_DRAW || NN_PVDCNN_DRAW)
#endif//ALG_FD_PD

	pthread_mutex_lock(&flow_start_lock);
	flow_audio_start = 0;
	encode_start = 0;
	acquire_start = 0;
	ai_start = 0;
	pthread_mutex_unlock(&flow_start_lock);

#if UVC_ON
	sleep(1);
	UVAC_enable(); //init usb device, start to prepare video/audio buffer to send
#endif

#if UVC_SUPPORT_YUV_FMT
	// create acquire_yuv_thread (acquire yuv thread)
	THREAD_CREATE(stream[0].acquire_thread_id, acquire_yuv_thread, (void *)stream, "acquire_yuv_thread");
	if (0 == stream[0].acquire_thread_id) {
		printf("create acquire_yuv_thread failed");;
		goto exit;
	}
	THREAD_RESUME(stream[0].acquire_thread_id);
#endif

	// query user key
	printf("Enter q to exit\n");
#if 0
	while (1) {
		key = GETCHAR();
		if (key == 'q' || key == 0x3) {
			pthread_mutex_lock(&flow_start_lock);
			flow_audio_start = 0;
			encode_start = 0;
			acquire_start = 0;
			ai_start = 0;
			pthread_mutex_unlock(&flow_start_lock);
#if ALG_FD_PD
			draw_exit = 1;
			pvdcnn_exit = 1;
			fdcnn_exit = 1;
#endif
			// let encode_thread stop loop and exit
			stream[0].enc_exit = 1; //stop video

#if AUDIO_ON
			au_cap.cap_exit = 1; //stop audio
#endif

#if UVC_SUPPORT_YUV_FMT
			// let acquire_yuv_thread stop loop and exit
			stream[0].acquire_exit = 1; //stop video
#endif

			// quit program
			break;
		}

		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			// enter debug menu
			hd_debug_run_menu();
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
		if (key == '0') {
			get_cap_sysinfo(stream[0].cap_ctrl);
		}
		if (key == 'u') {
			uvc_debug_menu_entry_p(uvc_debug_menu, "UVAC DEBUG MAIN");
		}
#if HID_FUNC
		if (key == '1') {
			UINT8 intr_in_test[8] = {1, 2, 3, 4, 5, 6, 7, 8};

			write_hid_test(intr_in_test, sizeof(intr_in_test), 0);
		}
		if (key == '2') {
			UINT8 intr_in_test[8] = {8, 7, 6, 4, 5, 6, 7, 8};

			write_hid_test(intr_in_test, sizeof(intr_in_test), 5000);
		}
		if (key == '3') {
			UINT8 intr_in_test[8] = {0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0, 1};

			write_hid_test(intr_in_test, sizeof(intr_in_test), -1);
		}
		if (key == '4') {
			read_hid_test(8, 0);
		}
		if (key == '5') {
			read_hid_test(100, 5000);
		}
		if (key == '6') {
			read_hid_test(8, -1);
		}
#else //internal test only
		if (key == '1') {
			printf("Stop video ++\r\n");
			_UVAC_stream_enable(0, 0);
			printf("Stop video --\r\n");
		}
		if (key == '2') {
			printf("Start video ++\r\n");
			_UVAC_stream_enable(0, 1);
			printf("Start video --\r\n");
		}
		if (key == '3') {
			printf("Stop aud ++\r\n");
			_UVAC_stream_enable(1, 0);
			printf("Stop aud --\r\n");
		}
		if (key == '4') {
			printf("Start aud ++\r\n");
			_UVAC_stream_enable(1, 1);
			printf("Start aud --\r\n");
		}
#endif
#if MULTI_STREAM
		if (key == '7') {
			//enable multi stream
			multi_stream_func = TRUE;
			UVAC_SetConfig(UVAC_CONFIG_VID_USER_DATA_SIZE, sizeof(STREAM_USER_DATA));
			printf("enable multi stream\r\n");
		}
		if (key == '8') {
			//disable multi stream
			multi_stream_func = FALSE;
			UVAC_SetConfig(UVAC_CONFIG_VID_USER_DATA_SIZE, 0);
			printf("disable multi stream\r\n");
		}
#endif
#if CDC_FUNC
		if (key == 'z') {
			UINT8 in_test[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

			write_cdc_test(in_test, sizeof(in_test), -1);
		}

		if (key == 'x') {
			read_cdc_test(5, -1);
		}
#endif
	}
#else // Reduce CPU loading. 8x% ---> 1x%.
	while(1) {sleep(1);}
#endif
	// destroy encode thread
	THREAD_DESTROY(stream[0].enc_thread_id);
#if AUDIO_ON
	THREAD_DESTROY(cap_thread_id); // stop audio
#endif

#if  UAC_RX_ON
	pthread_join(uac_thread_id, NULL);
#endif

#if UVC_ON
	UVAC_disable();
#endif

#if UVC_SUPPORT_YUV_FMT
	THREAD_DESTROY(stream[0].acquire_thread_id);
#endif

#if AUDIO_ON
	//stop audio capture module
	hd_audiocap_stop(au_cap.cap_path);
#endif
	// stop VIDEO_STREAM modules (main)
	if (g_capbind == 1){
		hd_videoproc_stop(stream[0].proc_main_path);
#if MULTI_STREAM
		hd_videoproc_stop(stream[0].proc_sub1_path);
		hd_videoproc_stop(stream[0].proc_sub2_path);
#endif
		hd_videoproc_stop(stream[0].ref_path);
		hd_videocap_stop(stream[0].cap_path);
	} else {
		hd_videocap_stop(stream[0].cap_path);
		hd_videoproc_stop(stream[0].proc_main_path);
#if MULTI_STREAM
		hd_videoproc_stop(stream[0].proc_sub1_path);
		hd_videoproc_stop(stream[0].proc_sub2_path);
#endif
		hd_videoproc_stop(stream[0].ref_path);
	}
#if ALG_FD_PD
	hd_videoproc_stop(stream[0].proc_alg_path);
    hd_videoproc_stop(stream[1].proc_alg_path);
    hd_videoproc_stop(stream[2].proc_alg_path);
#endif
	hd_videoenc_stop(stream[0].enc_main_path);
#if MULTI_STREAM
	hd_videoenc_stop(stream[0].enc_sub1_path);
	hd_videoenc_stop(stream[0].enc_sub2_path);
#endif

	// unbind VIDEO_STREAM modules (main)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_4);

exit:
#if UVC_SUPPORT_YUV_FMT
	for (i=0; i<GFX_QUEUE_MAX_NUM; i++) {
		if(gfx[i].buf_blk)
			hd_common_mem_release_block(gfx[i].buf_blk);
		if(gfx[i].buf_va)
			if(hd_common_mem_munmap((void *)gfx[i].buf_va, gfx[i].buf_size))
				printf("fail to unmap va(%x)\n", (int)gfx[i].buf_va);
	}
    ret = hd_gfx_uninit();
#endif

	// close VIDEO_STREAM modules (main)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

#if AUDIO_ON
	//close audio module
	ret = close_module2(&au_cap);
	if(ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}
#endif

#if ALG_FD_PD
    ret = close_module_extend(&stream[1]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

    ret = close_module_extend(&stream[2]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}
#endif

	// uninit all modules
	ret = exit_module();
	if (ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}

#if AUDIO_ON
	// uninit all modules
	ret = exit_module2();
	if (ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}
#endif

    ret = hd_gfx_uninit();
	if (ret != HD_OK) {
		printf("hd_gfx_uninit fail=%d\n", ret);
	}

#if ALG_FD_PD
	ret = release_mem_block();
	if (ret != HD_OK) {
		printf("mem_uninit fail=%d\n", ret);
	}
#endif
#if HID_FUNC
	ret = release_hid_mem_block();
	if (ret != HD_OK) {
		printf("mem_hid_uninit fail=%d\n", ret);
	}
#endif

#if CDC_FUNC
	ret = release_cdc_mem_block();
	if (ret != HD_OK) {
		printf("mem_cdc_uninit fail=%d\n", ret);
	}
#endif

	// uninit memory
	ret = mem_exit();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
	}

	// uninit hdal
	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
	}

}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include "hdal.h"
#include <kwrap/examsys.h>
#include <kwrap/cmdsys.h>

static int check_if_multiple_instance(char *p_name)
{
	char buf[128];
	FILE *pp;

	snprintf(buf, sizeof(buf), "ps | grep %s | wc -l", p_name);
	if ((pp = popen(buf, "r")) == NULL) {
		printf("popen() error!\n");
		exit(1);
	}

	while (fgets(buf, sizeof buf, pp)) {
	}

	pclose(pp);

	if(atoi(buf) > 3) {
		return 1;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	if (check_if_multiple_instance(argv[0]) == 1) {
		if (argc > 1) {
			nvt_cmdsys_ipc_cmd(argc, argv);
			return 0;
		}
		printf("%s has already in running. quit.\n", argv[0]);
		return -1;
	}

	nvt_cmdsys_init();      // command system
	nvt_examsys_init();     // exam system

	//start do your program
	app_main(argc, argv);
	return 0;
}
