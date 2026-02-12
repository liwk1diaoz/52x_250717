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
#include "autoframing.h"


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
#if 0
#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)
#endif
#define UVC_ON 1
#define AUDIO_ON 1
#define WRITE_BS 0
#define ALG_FD_PD 1
#define AUTO_FRAMING 1


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

typedef struct _CDCLineCoding {
	UINT32   uiBaudRateBPS; ///< Data terminal rate, in bits per second.
	UINT8    uiCharFormat;  ///< Stop bits.
	UINT8    uiParityType;  ///< Parity.
	UINT8    uiDataBits;    ///< Data bits (5, 6, 7, 8 or 16).
} CDCLineCoding;

typedef enum _CDC_PSTN_REQUEST {
	REQ_SET_LINE_CODING         =    0x20,
	REQ_GET_LINE_CODING         =    0x21,
	REQ_SET_CONTROL_LINE_STATE  =    0x22,
	REQ_SEND_BREAK              =    0x23,
	ENUM_DUMMY4WORD(CDC_PSTN_REQUEST)
} CDC_PSTN_REQUEST;

#define NVT_UI_UVAC_RESO_CNT  2
static UVAC_VID_RESO gUIUvacVidReso[NVT_UI_UVAC_RESO_CNT] = {
	{ 1920,   1080,   1,      30,      0,      0},
	{ 1280,    720,   1,      30,      0,      0},
};
#define NVT_UI_UVAC_AUD_SAMPLERATE_CNT                  1
//#define NVT_UI_FREQUENCY_16K                    0x003E80   //16k
#define NVT_UI_FREQUENCY_32K                    0x007D00   //32k
//#define NVT_UI_FREQUENCY_48K                    0x00BB80   //48k
static UINT32 gUIUvacAudSampleRate[NVT_UI_UVAC_AUD_SAMPLERATE_CNT] = {
	NVT_UI_FREQUENCY_32K
};

static  UVAC_VIDEO_FORMAT m_VideoFmt[UVAC_VID_DEV_CNT_MAX] = {0}; //UVAC_VID_DEV_CNT_MAX=2
static BOOL m_bIsStaticPattern = FALSE;

static UINT32 uigFrameIdx = 0, uigAudIdx = 0;
static UINT32 uvac_pa = 0;
static UINT32 uvac_va = 0;
static UINT32 uvac_size = 0;
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

#define SEN_OUT_FMT		HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT		HD_VIDEO_PXLFMT_RAW12
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

///////////////////////////////////////////////////////////////////////////////
HD_VIDEOENC_BUFINFO phy_buf_main;
HD_AUDIOCAP_BUFINFO phy_buf_main2;
UINT32 vir_addr_main;
UINT32 vir_addr_main2;
#define MAX_CAP_SIZE_W 1920
#define MAX_CAP_SIZE_H 1080
static int vdo_size_w =	1920;
static int vdo_size_h =	1080;
pthread_t cap_thread_id;
UINT32     flow_start=0;
#define JPG_YUV_TRANS 0
pthread_mutex_t flow_start_lock;  


#if ALG_FD_PD
#define NN_FDCNN_FD_MODE       ENABLE
#define NN_FDCNN_FD_PROF       DISABLE
#define NN_FDCNN_FD_DUMP       DISABLE//DISABLE
#define NN_FDCNN_FD_DRAW_YUV   DISABLE//ENABLE
#define NN_FDCNN_FD_FIX_FRM    DISABLE
#define NN_FDCNN_HIGH_PRIORITY DISABLE
#define NN_FDCNN_FD_TYPE       FDCNN_NETWORK_V20

#define NN_PVDCNN_MODE			ENABLE
#define NN_PVDCNN_PROF			DISABLE
#define NN_PVDCNN_DUMP			DISABLE
#define NN_PVDCNN_DRAW_YUV		DISABLE
#define NN_PVDCNN_FIX_FRM		DISABLE
#define NN_PVDCNN_HIGH_PRIORITY	DISABLE
#define AF_CROP_FUNC	ENABLE

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

#include "vendor_isp.h"

#define OSG_LCD_WIDTH             1920
#define OSG_LCD_HEIGHT            1080
#define NN_USE_DRAM2              ENABLE
#define VENDOR_AI_CFG             0x000f0000  //ai project config
#define SEI_HEADER                0 //H264 header info
#define VDO_ENC_OUT_BUFFER_SIZE  (1920*1080 * 1.5 * 0.8)
#define MAX_ENC_OUT_BLK_CNT       2

static VENDOR_AIS_FLOW_MEM_PARM g_mem = {0};
static HD_COMMON_MEM_VB_BLK g_blk_info[1];


#endif //ALG_FD_PD


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
#if ALG_FD_PD	
	UINT32     codec_type;
	pthread_t  af_thread_id;
	UINT32     do_snap;
	UINT32     show_ret;
	UINT32     wait_ms;
	UINT32	   shot_count;
	UINT32     skip_count;
	
    // osg
	HD_PATH_ID vp_afmask_path;
	HD_PATH_ID vp_stamp_path;
	HD_PATH_ID enc_stamp_path;


	UINT32 stamp_blk;
	UINT32 stamp_pa;
	UINT32 stamp_size;
	unsigned short *alphaVision_logo;

	// vout
	HD_VIDEOPROC_SYSCAPS proc1_syscaps;
	HD_PATH_ID proc1_ctrl;
	HD_PATH_ID proc1_path;
	HD_VIDEOOUT_SYSCAPS out_syscaps;
	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;
	HD_DIM  out_max_dim;
	HD_DIM  out_dim;

	
	//ALG
	HD_VIDEOPROC_SYSCAPS proc_alg_syscaps;
	HD_PATH_ID proc_alg_ctrl;
	HD_PATH_ID proc_alg_path;

	HD_DIM  proc_alg_max_dim;
	HD_DIM  proc_alg_dim;
	
	HD_PATH_ID mask_alg_path;
	
#if NN_FDCNN_FD_DRAW_YUV
	HD_PATH_ID mask_path0;
	HD_PATH_ID mask_path1;
	HD_PATH_ID mask_path2;
	HD_PATH_ID mask_path3;
#endif

#if NN_PVDCNN_DRAW_YUV
	HD_PATH_ID mask_path4;
	HD_PATH_ID mask_path5;
	HD_PATH_ID mask_path6;
	HD_PATH_ID mask_path7;
#endif
	//HD_VIDEOOUT_HDMI_ID hdmi_id;
#endif//ALG_FD_PD
} VIDEO_STREAM;

#if ALG_FD_PD
VIDEO_STREAM stream[1] = {0}; //0: main stream
#if NN_FDCNN_FD_DRAW_YUV
int fdcnn_fd_draw_info(VENDOR_AIS_FLOW_MEM_PARM fd_buf, VIDEO_STREAM *p_stream);
#endif

#if NN_PVDCNN_DRAW_YUV
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

#if SEI_HEADER
#include "detection.h"

#define MD              1       // face detection
#define PD              2       // people detection
#define FD              3       // face detection
#define FTG             4       // face tracking granding

#define OSG_WIDTH 		1920
#define OSG_HEIGHT     1080

face_result *gfd_result;
people_result *gpd_result;



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

/*************************************
 * Contain user data
 *
 * Input parameter:
 *  out_stream: pointer of output bitstream
 *  data:       pointer of input user data
 *  length:     length of user data
 *
 * Return:
 *  length of output bitstream
 *************************************/
int gen_nalu_sei(int enc_type,unsigned char *out_stream, unsigned char *data, int length)
{
        int byte_pos = 0;
        int tmp = length;
        int i;
        int counter = 0;
        //printf("enc_type = %d\n",enc_type);
        if(enc_type == HD_CODEC_TYPE_H264) //for H264
        {
                // 1. encode start code & SEI header
                out_stream[byte_pos++] = 0x00;
                out_stream[byte_pos++] = 0x00;
                out_stream[byte_pos++] = 0x00;
                out_stream[byte_pos++] = 0x01;
                out_stream[byte_pos++] = 0x06;// type SEI
                // 2. encode sei type
                out_stream[byte_pos++] = 0x40;
        }
        else //for H265
        {
                // 1. encode start code & SEI header
                out_stream[byte_pos++] = 0x00;
                out_stream[byte_pos++] = 0x00;
                out_stream[byte_pos++] = 0x00;
                out_stream[byte_pos++] = 0x01;
                out_stream[byte_pos++] = 0x4E;   // type SEI
                out_stream[byte_pos++] = 0x00;   // type SEI
                // 2. encode sei type
                out_stream[byte_pos++] = 0x96;
        }
        // 3. encode sei length
		while (tmp > 255) {
                out_stream[byte_pos++] = 0xFF;
                tmp -= 255;
        }
        out_stream[byte_pos++] = tmp;
        // 4. encode sei data
        for (i = 0; i < length; i++) {
                //if (0 == out_stream[byte_pos-2] && 0 == out_stream[byte_pos-2]
                if (2 == counter && !(data[i] & 0xFC)) {
                        out_stream[byte_pos++] = 0x03;
                        counter = 0;
                }
                out_stream[byte_pos++] = data[i];
                if (0x00 == data[i])
                        counter++;
                else
                        counter = 0;
        }
        out_stream[byte_pos++] = 0x80;
        return byte_pos;
}

int copy_result_data(unsigned int type, unsigned char *data, int *len_offset)
{
        int ret = 0;

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
#if 0
                for (i = 0; i < (int)gftg_result->face_num; i++) {
                        printf("FTG id(%d) x(%u)  y(%u) width(%u) height(%u)\n", gftg_result->ftg[i].id, gftg_result->ftg[i].x, gftg_result->ftg[i].y, gftg_result->ftg[i].w, gftg_result->ftg[i].h);

                }
#endif
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
#if 0
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
#endif
        return ;
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
#if 1

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
#endif

}

static int cnn_fill_sei_info(VENDOR_AIS_FLOW_MEM_PARM cnn_buf, int detect_type)
{
	HD_RESULT ret = HD_OK;
    static PVDCNN_RSLT pdcnn_info[FDCNN_MAX_OUTNUM];
	static FDCNN_RESULT fdcnn_info[FDCNN_MAX_OUTNUM] = {0};
    UINT32 cnn_num;
    UINT32 index;
    static HD_URECT cnn_size = {0, 0, OSG_WIDTH, OSG_HEIGHT};
	printf("charlesdddddddddddd\r\n");
	switch(detect_type)
	{
		case FD: 
			cnn_num = fdcnn_getresults(cnn_buf, fdcnn_info, &cnn_size, FDCNN_MAX_OUTNUM);
			for(index = 0;index < 16 ;index++)
			{
				printf("charles:FD x=%d y=%d w=%d h=%d index=%d, cnn_num=%d\r\n",fdcnn_info->x, fdcnn_info->y,fdcnn_info->w,fdcnn_info->h, index,cnn_num );
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
					printf("1charlesdddddddddddd\r\n");
				    //generate_fdt_idx(index,index, fdcnn_info->x, fdcnn_info->y,fdcnn_info->w,fdcnn_info->h);
					printf("2charlesdddddddddddd\r\n");
                }
                else
                {
                    fdcnn_info->x = 1;
                    fdcnn_info->y = 1;
                    fdcnn_info->w = 1;
                    fdcnn_info->h = 1;
					printf("3charlesdddddddddddd\r\n");
                    generate_fdt_idx(index,index, fdcnn_info->x, fdcnn_info->y,fdcnn_info->w,fdcnn_info->h);
					printf("4charlesdddddddddddd\r\n");
                }
			}
			break;
		case PD:
			cnn_num = pvdcnn_get_result(cnn_buf, pdcnn_info, &cnn_size, FDCNN_MAX_OUTNUM);
			for(index = 0;index < 16 ;index++)
			{
				printf("charles:PD x=%d y=%d w=%d h=%d index=%d, cnn_num=%d\r\n",pdcnn_info->box.x, pdcnn_info->box.y,pdcnn_info->box.w,pdcnn_info->box.h, index,cnn_num);
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
#if ALG_FD_PD
	UINT32 mem_size = 0;

#if NN_FDCNN_FD_MODE
    mem_size += fdcnn_calcbuffsize(NN_FDCNN_FD_TYPE);
#endif

#if NN_PVDCNN_MODE
    mem_size += pvdcnn_calcbuffsize();
#endif

#endif//ALG_FD_PD

	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(MAX_CAP_SIZE_W, MAX_CAP_SIZE_H, CAP_OUT_FMT)
														+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
														+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	mem_cfg.pool_info[0].blk_cnt = 2;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;

	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(MAX_CAP_SIZE_W, MAX_CAP_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[1].blk_cnt = 3;
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

#if SEI_HEADER
	mem_cfg.pool_info[6].type     = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[6].blk_size = VDO_ENC_OUT_BUFFER_SIZE;
	mem_cfg.pool_info[6].blk_cnt  = MAX_ENC_OUT_BLK_CNT;
	mem_cfg.pool_info[6].ddr_id   = DDR_ID0;
#endif
	ret = hd_common_mem_init(&mem_cfg);
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

	ret = hd_audiocap_open(0, HD_AUDIOCAP_0_CTRL, &audio_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	// set audiocap dev parameter
	audio_dev_cfg.in_max.sample_rate = HD_AUDIO_SR_32000;
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

	*p_audio_cap_ctrl = audio_cap_ctrl;
	return ret;
}

static HD_RESULT set_cap_param2(HD_PATH_ID audio_cap_path, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOCAP_IN audio_cap_param = {0};

	// set audiocap input parameter
	audio_cap_param.sample_rate = HD_AUDIO_SR_32000;
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

static void *capture_thread(void *arg)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIO_FRAME  data_pull;
	int get_phy_flag=0;
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

		if (!flow_start){
			if (get_phy_flag){
				/* mummap for bs buffer */
				//printf("release common buffer2\r\n");
				if (vir_addr_main2)hd_common_mem_munmap((void *)vir_addr_main2, phy_buf_main2.buf_info.buf_size);

				get_phy_flag = 0;
			}
			/* wait flow_start */
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
			strmFrm.path = UVAC_STRM_AUD;
			strmFrm.addr = data_pull.phy_addr[0];
			strmFrm.size = data_pull.size;
			UVAC_SetEachStrmInfo(&strmFrm);
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

		usleep(1000000/32000*2);
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

	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x301;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0|PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB;
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}

static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};

		video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
		video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		video_in_param.pxlfmt = SEN_OUT_FMT;
		video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN, &video_in_param);
		//printf("set_cap_param MODE=%d\r\n", ret);
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
		video_cfg_param.ctrl_max.func = 0;
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

	video_ctrl_param.func = 0;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);

	*p_video_proc_ctrl = video_proc_ctrl;

	return ret;
}

#if AF_CROP_FUNC
static HD_RESULT set_proc_crop_param(HD_PATH_ID video_proc_path, af_final_result *af_rlt)
{
	HD_RESULT ret = HD_OK;
	double	ratio_w = 0;
	double	ratio_h = 0;
	HD_VIDEOPROC_CROP video_out_crop_param = {0};
	HD_VIDEOPROC_OUT video_out_param = {0};
	ratio_w = (double)1920 / (double)af_rlt->w;
	ratio_h = (double)1080 / (double)af_rlt->h;
	if (af_rlt != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		video_out_param.func = 0;
		video_out_param.dim.w = 1920 * ratio_w;
		video_out_param.dim.h = 1080 * ratio_h;
		video_out_param.dim.w = ALIGN_CEIL_4(video_out_param.dim.w);
		video_out_param.dim.h = ALIGN_CEIL_4(video_out_param.dim.h);
		printf ("video_out_param.dim.w: %u video_out_param.dim.h: %u\r\n", video_out_param.dim.w, video_out_param.dim.h);
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = 0; //set 1 to allow pull
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
		
		video_out_crop_param.mode = HD_CROP_ON;
		video_out_crop_param.win.rect.x = af_rlt->x * ratio_w;
		video_out_crop_param.win.rect.y = af_rlt->y * ratio_h;
		video_out_crop_param.win.rect.x = ALIGN_CEIL_4(video_out_crop_param.win.rect.x);
		video_out_crop_param.win.rect.y = ALIGN_CEIL_4(video_out_crop_param.win.rect.y);
		video_out_crop_param.win.rect.w = 1920;
		video_out_crop_param.win.rect.h = 1080;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_CROP, &video_out_crop_param);
	} else {
		video_out_crop_param.mode = HD_CROP_OFF;
		video_out_crop_param.win.rect.x = 0;
		video_out_crop_param.win.rect.y = 0;
		video_out_crop_param.win.rect.w = 0;
		video_out_crop_param.win.rect.h = 0;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_CROP, &video_out_crop_param);
	}
	
	return ret;
}

#endif



static HD_RESULT set_proc_param(HD_PATH_ID video_proc_path, HD_DIM* p_dim)
{
	HD_RESULT ret = HD_OK;
	
	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = 1;	// set > 0 to allow pull out (nn)
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
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
	
	return ret;
}

static HD_RESULT set_enc_param(HD_PATH_ID video_enc_path, HD_DIM *p_dim, UINT32 enc_type, UINT32 bitrate)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_IN  video_in_param = {0};
	HD_VIDEOENC_OUT video_out_param = {0};
	HD_H26XENC_RATE_CONTROL rc_param = {0};

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

		if (enc_type == 0) {
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
		} else if (enc_type == 1) {
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

		} else if (enc_type == 2) {
			//printf("enc_type = HD_CODEC_TYPE_JPEG\r\n");
			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_JPEG;
			video_out_param.jpeg.retstart_interval = 0;
			video_out_param.jpeg.image_quality = 50;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}
			
			//MJPG YUV trans(YUV420 to YUV422)
			VENDOR_VIDEOENC_JPG_YUV_TRANS_CFG yuv_trans_cfg = {0};
			yuv_trans_cfg.jpg_yuv_trans_en = JPG_YUV_TRANS;
			//printf("MJPG YUV TRANS(%d)\r\n", JPG_YUV_TRANS);
			vendor_videoenc_set(video_enc_path, VENDOR_VIDEOENC_PARAM_OUT_JPG_YUV_TRANS, &yuv_trans_cfg);

		} else {

			printf("not support enc_type\r\n");
			return HD_ERR_NG;
		}
	}

	return ret;
}

#if ALG_FD_PD
/*
static HD_RESULT set_out_cfg(HD_PATH_ID *p_video_out_ctrl, UINT32 out_type,HD_VIDEOOUT_HDMI_ID hdmi_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_MODE videoout_mode = {0};
	HD_PATH_ID video_out_ctrl = 0;

	ret = hd_videoout_open(0, HD_VIDEOOUT_0_CTRL, &video_out_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	printf("out_type=%d\r\n", out_type);

	#if 1
	videoout_mode.output_type = HD_COMMON_VIDEO_OUT_LCD;
	videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
	videoout_mode.output_mode.lcd = HD_VIDEOOUT_LCD_0;
	if (out_type != 1) {
		printf("520 only support LCD\r\n");
	}
	#else
	switch(out_type){
	case 0:
		videoout_mode.output_type = HD_COMMON_VIDEO_OUT_CVBS;
		videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.cvbs= HD_VIDEOOUT_CVBS_NTSC;
	break;
	case 1:
		videoout_mode.output_type = HD_COMMON_VIDEO_OUT_LCD;
		videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.lcd = HD_VIDEOOUT_LCD_0;
	break;
	case 2:
		videoout_mode.output_type = HD_COMMON_VIDEO_OUT_HDMI;
		videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.hdmi= hdmi_id;
	break;
	default:
		printf("not support out_type\r\n");
	break;
	}
	#endif
	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_MODE, &videoout_mode);

	*p_video_out_ctrl=video_out_ctrl ;
	return ret;
}

static HD_RESULT get_out_caps(HD_PATH_ID video_out_ctrl,HD_VIDEOOUT_SYSCAPS *p_video_out_syscaps)
{
	HD_RESULT ret = HD_OK;
    HD_DEVCOUNT video_out_dev = {0};

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_DEVCOUNT, &video_out_dev);
	if (ret != HD_OK) {
		return ret;
	}
	printf("##devcount %d\r\n", video_out_dev.max_dev_count);

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, p_video_out_syscaps);
	if (ret != HD_OK) {
		return ret;
	}
	return ret;
}

static HD_RESULT set_out_param(HD_PATH_ID video_out_path, HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_IN video_out_param={0};

	video_out_param.dim.w = p_dim->w;
	video_out_param.dim.h = p_dim->h;
	video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	video_out_param.dir = HD_VIDEO_DIR_NONE;
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		return ret;
	}
	memset((void *)&video_out_param,0,sizeof(HD_VIDEOOUT_IN));
	ret = hd_videoout_get(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		return ret;
	}
	printf("##video_out_param w:%d,h:%d %x %x\r\n", video_out_param.dim.w, video_out_param.dim.h, video_out_param.pxlfmt, video_out_param.dir);

	return ret;
}
*/
#endif//ALG_FD_PD

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

/*#if ALG_FD_PD

static HD_RESULT open_module(VIDEO_STREAM *p_stream, HD_DIM* p_proc_max_dim, UINT32 out_type)
#else
*/
static HD_RESULT open_module(VIDEO_STREAM *p_stream, HD_DIM* p_proc_max_dim)
//#endif //ALG_FD_PD
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
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_1, &p_stream->proc_alg_path)) != HD_OK)
		return ret;
	
#if NN_FDCNN_FD_DRAW_YUV
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

#if NN_PVDCNN_DRAW_YUV
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
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;
#endif //ALG_FD_PD
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_0, HD_VIDEOENC_0_OUT_0, &p_stream->enc_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module(VIDEO_STREAM *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_close(p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;

#if ALG_FD_PD
/*
	if ((ret = hd_videoproc_close(p_stream->proc1_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_close(p_stream->out_path)) != HD_OK)
		return ret;
*/
#if NN_FDCNN_FD_DRAW_YUV
    if ((ret = hd_videoproc_close(p_stream->mask_path0)) != HD_OK)
		return ret;
    if ((ret = hd_videoproc_close(p_stream->mask_path1)) != HD_OK)
		return ret;
    if ((ret = hd_videoproc_close(p_stream->mask_path2)) != HD_OK)
		return ret;
    if ((ret = hd_videoproc_close(p_stream->mask_path3)) != HD_OK)
		return ret;
#endif
#if NN_PVDCNN_DRAW_YUV
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
	if ((ret = hd_videoenc_close(p_stream->enc_path)) != HD_OK)
		return ret;
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
/*	
	if ((ret = hd_videoout_uninit()) != HD_OK)
		return ret;
*/
	return HD_OK;
}

#if SEI_HEADER
static INT32 blk2idx(HD_COMMON_MEM_VB_BLK blk)   // convert blk(0xXXXXXXXX) to index (0, 1, 2)
{
	static HD_COMMON_MEM_VB_BLK blk_registered[MAX_ENC_OUT_BLK_CNT] = {0};
	INT32 i;
	for (i=0; i< MAX_ENC_OUT_BLK_CNT; i++) {
		if (blk_registered[i] == blk) return i;

		if (blk_registered[i] == 0) {
			blk_registered[i] = blk;
			return i;
		}
	}

	printf("convert blk(%0x%x) to index fail !!!!\r\n", blk);
	return (-1);
}
#endif

#if AUTO_FRAMING
static void *af_thread(void *arg)
{
	VIDEO_STREAM* p_stream0 = (VIDEO_STREAM *)arg;
	int ret;
	af_final_result af_final_rlt;
	af_final_rlt.bufid = -1;
	af_final_result final_rlt_clear = {0};
	sleep(10);
	while (1){
		ret = get_autoframing_result(&af_final_rlt);
        switch(ret)
		{
			case AF_DATA_ERR:
				printf("ret: af data not ready\r\n", ret);
				sleep(1);
                break;
			case AF_DATA_DUP:
				//printf("AF_DATA_DUP\r\n");
				//ret = set_vp_afmask_param(p_stream0->vp_afmask_path, &af_final_rlt);
				//if (ret != HD_OK) {
				//	printf("fail to set vp afmask attr\r\n");
				//	goto exit;
				//}
                break;
			case AF_DATA_OK:
				//clear
printf("AF_DATA_OK bufid: %d %u %u %u %u\r\n", af_final_rlt.bufid, af_final_rlt.x, af_final_rlt.y, af_final_rlt.w, af_final_rlt.h);
#if AF_CROP_FUNC
				//set news
				ret = set_proc_crop_param(p_stream0->proc_path, &af_final_rlt);
				if (ret != HD_OK) {
					printf("fail to set vp afmask attr\r\n");
					goto exit;
				}
				ret = hd_videoproc_start(p_stream0->proc_path);
				if (ret != HD_OK) {
					printf("fail to start proc_path %d\n", ret);
				}
				break;		
#endif

			default:
				printf("unknow AF ret: %d\r\n", ret);
				break;
		}
	}
	
exit:
	return 0;
}
#endif
static void *encode_thread(void *arg)
{

	VIDEO_STREAM* p_stream0 = (VIDEO_STREAM *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_BS  data_pull;
	UINT32 j;
	int get_phy_flag=0;
#if SEI_HEADER
	unsigned char *sei_data;
    unsigned char *nalu_data;
//	unsigned char *new_nalu_data;
	int sei_header_offset=0;
    int nalu_len=0;
//	int new_nalu_len=0;
    int fd_sei_len=0;
    int pd_sei_len=0;
	int count = 0;
    unsigned char end_code[4] = {0x45, 0x4E, 0x44, 0x5F}; // 'E' 'N' 'D' '_'
    get_struct_size(FD,&fd_sei_len);
    get_struct_size(PD,&pd_sei_len);
	
	sei_data = malloc(1024);
	nalu_data = malloc(1024);
//	new_nalu_data = malloc(1024);

	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32 blk_size = VDO_ENC_OUT_BUFFER_SIZE;
	UINT32 pa_sei_frame_main[MAX_ENC_OUT_BLK_CNT] = {0};
	UINT32 va_sei_frame_main[MAX_ENC_OUT_BLK_CNT] = {0};
	INT32  blkidx;
	HD_COMMON_MEM_VB_BLK blk;
#endif

#if WRITE_BS
	char file_path_main[32] = "/mnt/sd/dump_bs_main.dat";
	FILE *f_out_main;
#endif
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))
	
#if UVC_ON
	UVAC_STRM_FRM      strmFrm = {0};
	UINT32             loop =0;
	UVAC_VID_BUF_INFO  vid_buf_info = {0};
	UINT32             uvacNeedMem=0;
	UINT32             ChannelNum = 1;
#endif
	//UINT32 temp=1;

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
		if (!flow_start){
			if (get_phy_flag){
				
				get_phy_flag = 0;
				/* mummap for bs buffer */
				//printf("release common buffer2\r\n");
				if (vir_addr_main)hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);

			}
			//------ wait flow_start ------
			usleep(10);
			continue;
		}else{
			if (!get_phy_flag){

				get_phy_flag = 1;
				// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
				ret = hd_videoenc_get(stream[0].enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);

				// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
				vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);

			}
		}
#if SEI_HEADER
		//--- Get memory ---
		blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id); // Get block from mem pool
		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get block fail (0x%x).. try again later.....\r\n", blk);
			sleep(1);
			continue;
		}

		if ((blkidx = blk2idx(blk)) == -1) {
			printf("ERROR !! blk to idx fail !!\r\n");
			goto rel_blk;
		}

		pa_sei_frame_main[blkidx] = hd_common_mem_blk2pa(blk); // Get physical addr
		if (pa_sei_frame_main[blkidx] == 0) {
			printf("blk2pa fail, blk = 0x%x\r\n", blk);
			goto rel_blk;
		}

		if (va_sei_frame_main[blkidx] == 0) { // if NOT mmap yet, mmap it
			va_sei_frame_main[blkidx] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa_sei_frame_main[blkidx], blk_size); // Get virtual addr
			if (va_sei_frame_main[blkidx] == 0) {
				printf("Error: mmap fail !! pa_sei_frame_main[%d], blk = 0x%x\r\n", blkidx, blk);
				goto rel_blk;
			}
		}
/*
		//--- Read YUV from file ---
		fread((void *)va_sei_frame_main[blkidx], 1, yuv_size, f_in_main);

		if (feof(f_in_main)) {
			fseek(f_in_main, 0, SEEK_SET);  //rewind
			fread((void *)va_sei_frame_main[blkidx], 1, yuv_size, f_in_main);
		}
*/		

#endif
		//pull data
		ret = hd_videoenc_pull_out_buf(p_stream0->enc_path, &data_pull, -1); // -1 = blocking mode
		
#if SEI_HEADER
		generate_fdt(16,3,3,0,0);
		generate_pdt(16,3,3,0,0);
		copy_result_data(FD,sei_data,&sei_header_offset);
		copy_result_data(PD,sei_data,&sei_header_offset);
		memcpy(sei_data+sei_header_offset,end_code,4);
		nalu_len = gen_nalu_sei(HD_CODEC_TYPE_H264,nalu_data, sei_data, fd_sei_len+pd_sei_len);
		printf("Charles_dbg:sei_header_offset=%d, fd_sei_len+pd_sei_len=%d \r\n",sei_header_offset, (fd_sei_len+pd_sei_len));
#endif

		if (ret == HD_OK) {
#if UVC_ON
			strmFrm.path = UVAC_STRM_VID ;

			strmFrm.addr = data_pull.video_pack[data_pull.pack_num-1].phy_addr;
			strmFrm.size = data_pull.video_pack[data_pull.pack_num-1].size;
			strmFrm.pStrmHdr = 0;
			strmFrm.strmHdrSize = 0;
			if (p_stream0->codec_type == HD_CODEC_TYPE_H264){

				if (data_pull.pack_num > 1) {
					//printf("I-frame\r\n");
					
					for (loop = 0; loop < data_pull.pack_num - 1; loop ++) {
						strmFrm.strmHdrSize += data_pull.video_pack[loop].size;
					}
#if SEI_HEADER
					UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.video_pack[0].phy_addr);
					memcpy(va_sei_frame_main, ptr,strmFrm.strmHdrSize);
					memcpy(va_sei_frame_main+strmFrm.strmHdrSize, nalu_data, nalu_len);
					strmFrm.strmHdrSize += nalu_len;
					strmFrm.pStrmHdr = (UINT8 *) pa_sei_frame_main;
#else					
					strmFrm.pStrmHdr = (UINT8 *)data_pull.video_pack[0].phy_addr;
#endif				
					
	
				} else {
#if SEI_HEADER
					memcpy(va_sei_frame_main, nalu_data, nalu_len);
					strmFrm.pStrmHdr = (UINT8 *) pa_sei_frame_main;
					strmFrm.strmHdrSize += nalu_len;
#else
					strmFrm.pStrmHdr = 0;
					strmFrm.strmHdrSize = 0;
#endif
					//printf("P-frame\r\n");
#if SEI_HEADER
					
#endif	
					
				}
			}
			UVAC_SetEachStrmInfo(&strmFrm);
			/*timstamp for test
			if (temp){
				system("echo \"uvc\" > /proc/nvt_info/bootts");
				temp=0;
			}
			*/
#endif
			
#if WRITE_BS		
			for (j=0; j< data_pull.pack_num; j++) {
				UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.video_pack[j].phy_addr);
				UINT32 len = data_pull.video_pack[j].size;
				if (f_out_main) fwrite(ptr, 1, len, f_out_main);
				if (f_out_main) fflush(f_out_main);
			}

#endif		

#if SEI_HEADER
rel_blk:
			//--- Release memory ---
			hd_common_mem_release_block(blk);
#endif
			// release data
			ret = hd_videoenc_release_out_buf(p_stream0->enc_path, &data_pull);
			if (ret != HD_OK) {
				printf("enc_release error=%d !!\r\n", ret);
			}
		}
		usleep(33000);
	}
#if SEI_HEADER	
	//------ [4] uninit & exit ------
	// mummap for sei_frame buffer
	for (j=0; j< MAX_ENC_OUT_BLK_CNT; j++) {
		if (va_sei_frame_main[j] != 0)
			hd_common_mem_munmap((void *)va_sei_frame_main[j], blk_size);
	}
#endif
	
	if (get_phy_flag){
		// mummap for bs buffer
		//printf("release common buffer\r\n");
		if (vir_addr_main) hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
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

	if (pStrmInfo) {
		//printf("xxxxUVAC Start[%d] resoIdx=%d,W=%d,H=%d,codec=%d,fps=%d,path=%d,tbr=0x%x\r\n", vidDevIdx, pStrmInfo->strmResoIdx, pStrmInfo->strmWidth, pStrmInfo->strmHeight, pStrmInfo->strmCodec, pStrmInfo->strmFps, pStrmInfo->strmPath, pStrmInfo->strmTBR);
		m_VideoFmt[vidDevIdx] = pStrmInfo->strmCodec;

		//stop poll data
		pthread_mutex_lock(&flow_start_lock);
		flow_start = 0;
		pthread_mutex_unlock(&flow_start_lock);
		// mummap for video bs buffer
		//if (vir_addr_main) hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
		/* mummap for audio bs buffer */
		//if (vir_addr_main2)hd_common_mem_munmap((void *)vir_addr_main2, phy_buf_main2.buf_info.buf_size);

		//stop engine(modules)
#if AUDIO_ON
		//stop audio capture module
		hd_audiocap_stop(au_cap.cap_path);
#endif
		// stop VIDEO_STREAM modules (main)
		hd_videocap_stop(stream[0].cap_path);
		hd_videoproc_stop(stream[0].proc_path);
#if ALG_FD_PD
		//hd_videoproc_stop(stream[3].proc1_path);
		//hd_videoout_stop(stream[3].out_path);
#endif		
		hd_videoenc_stop(stream[0].enc_path);

		//set codec,resolution
		switch (pStrmInfo->strmCodec){
			case 0:
				stream[0].codec_type = 1;
				//printf("set codec=H264\r\n");
				break;
			case 1:
				stream[0].codec_type = 2;
				//printf("set codec=MJPG\r\n");
				break;
			default:
				printf("unrecognized codec(%d)\r\n",pStrmInfo->strmCodec);
				break;
		}

		//set encode resolution to device
		stream[0].enc_dim.w = pStrmInfo->strmWidth;
		stream[0].enc_dim.h = pStrmInfo->strmHeight;

		// set videoproc parameter (main)
		ret = set_proc_param(stream[0].proc_path, &stream[0].enc_dim);
		if (ret != HD_OK) {
			printf("zzzzset proc fail=%d\n", ret);
		}
		printf("%s %d\r\n",__func__, __LINE__);
		// set videoenc parameter (main)
		ret = set_enc_param(stream[0].enc_path, &stream[0].enc_dim, stream[0].codec_type, 2 * 1024 * 1024);
		if (ret != HD_OK) {
			printf("set enc fail=%d\n", ret);
		}

		//start engine(modules)
		hd_videocap_start(stream[0].cap_path);
#if ALG_FD_PD
		//hd_videoproc_start(stream[3].proc1_path);
	//render mask
#if NN_FDCNN_FD_DRAW_YUV
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

#if NN_PVDCNN_DRAW_YUV
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
		printf("%s %d\r\n",__func__, __LINE__);
		hd_videoproc_start(stream[0].proc_path);
	
		printf("%s %d\r\n",__func__, __LINE__);
		hd_videoenc_start(stream[0].enc_path);
		//hd_videoout_start(stream[3].out_path);

#if AUDIO_ON
		//start audio capture module
		hd_audiocap_start(au_cap.cap_path);
#endif
		pthread_mutex_lock(&flow_start_lock);
		flow_start = 1;
		pthread_mutex_unlock(&flow_start_lock);
	}
	return E_OK;
}

static UINT32 xUvacStopVideoCB(UINT32 isClosed)
{
	//printf(":isClosed=%d\r\n", isClosed);
	printf("stop encode flow\r\n");
	pthread_mutex_lock(&flow_start_lock);
	flow_start = 0;
	pthread_mutex_unlock(&flow_start_lock);
	return E_OK;
}

static BOOL UVAC_enable(void)
{

	UVAC_INFO       UvacInfo = {0};
	UINT32          retV = 0;
	
	UVAC_VEND_DEV_DESC UIUvacDevDesc = {0};
	UVAC_AUD_SAMPLERATE_ARY uvacAudSampleRateAry = {0};
	UINT32 bVidFmtType = UVAC_VIDEO_FORMAT_H264_MJPEG;
	m_bIsStaticPattern = FALSE;
	UVAC_VID_BUF_INFO vid_buf_info = {0};
	UINT32            uvacNeedMem  = 0;
	UINT32            ChannelNum   = 1;
	
	printf("###Real Open UVAC-lib\r\n");
	
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32 pa;
	void *va;
	HD_RESULT hd_ret;
	
	timer_init();
	
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

	vid_buf_info.vid_buf_pa =  phy_buf_main.buf_info.phy_addr;
	vid_buf_info.vid_buf_size =  phy_buf_main.buf_info.buf_size;
	UVAC_SetConfig(UVAC_CONFIG_VID_BUF_INFO, (UINT32)&vid_buf_info);

	UvacInfo.fpStartVideoCB  = (UVAC_STARTVIDEOCB)xUvacStartVideoCB;
	UvacInfo.fpStopVideoCB  = (UVAC_STOPVIDEOCB)xUvacStopVideoCB;
	
	USBMakerInit_UVAC(&UIUvacDevDesc);
	
	UVAC_SetConfig(UVAC_CONFIG_VEND_DEV_DESC, (UINT32)&UIUvacDevDesc);
	UVAC_SetConfig(UVAC_CONFIG_HW_COPY_CB, (UINT32)hd_gfx_memcpy);
	//printf("%s:uvacAddr=0x%x,s=0x%x;IPLAddr=0x%x,s=0x%x\r\n", __func__, UvacInfo.UvacMemAdr, UvacInfo.UvacMemSize, m_VideoBuf.addr, m_VideoBuf.size);

	UvacInfo.hwPayload[0] = FALSE;//hit
	UvacInfo.hwPayload[1] = FALSE;//hit

	//must set to 1I14P ratio
	UvacInfo.strmInfo.strmWidth = MAX_CAP_SIZE_W;
	UvacInfo.strmInfo.strmHeight = MAX_CAP_SIZE_H;
	UvacInfo.strmInfo.strmFps = 30;
	//UvacInfo.strmInfo.strmCodec = MEDIAREC_ENC_H264;
	//w=1280,720,30,2
	UVAC_ConfigVidReso(gUIUvacVidReso, NVT_UI_UVAC_RESO_CNT);
	
	uvacAudSampleRateAry.aryCnt = NVT_UI_UVAC_AUD_SAMPLERATE_CNT;
	uvacAudSampleRateAry.pAudSampleRateAry = &gUIUvacAudSampleRate[0]; //NVT_UI_FREQUENCY_32K
	UVAC_SetConfig(UVAC_CONFIG_AUD_SAMPLERATE, (UINT32)&uvacAudSampleRateAry);
	UvacInfo.channel = ChannelNum;
	//printf("w=%d,h=%d,fps=%d,codec=%d\r\n", UvacInfo.strmInfo.strmWidth, UvacInfo.strmInfo.strmHeight, UvacInfo.strmInfo.strmFps, UvacInfo.strmInfo.strmCodec);

	UVAC_SetConfig(UVAC_CONFIG_VIDEO_FORMAT_TYPE, bVidFmtType);

	retV = UVAC_Open(&UvacInfo);
	if (retV != E_OK) {
		printf("Error open UVAC task\r\n", retV);
	}

	return TRUE;
}

static BOOL UVAC_disable(void)
{
	HD_RESULT    ret = HD_OK;
	ret = hd_common_mem_free(uvac_pa, &uvac_va);
	if (HD_OK == ret) {
		printf("Error handle test for free an invalid address: fail %d\r\n", (int)(ret));
	} 
	UVAC_Close();

	return TRUE;
}
#endif

#if ALG_FD_PD
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

#if (NN_FDCNN_FD_DRAW_YUV || NN_PVDCNN_DRAW_YUV)
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

#endif //(NN_FDCNN_FD_DRAW_YUV || NN_PVDCNN_DRAW_YUV)

#if NN_FDCNN_FD_DRAW_YUV
int fdcnn_fd_draw_info(VENDOR_AIS_FLOW_MEM_PARM fd_buf, VIDEO_STREAM *p_stream)
{
	HD_RESULT ret = HD_OK;
	static FDCNN_RESULT fdcnn_info[FDCNN_MAX_OUTNUM] = {0};
	UINT32 fdcnn_num, num;
	static HD_URECT fdcnn_size = {0, 0, OSG_LCD_WIDTH, OSG_LCD_HEIGHT};

	fdcnn_num = fdcnn_getresults(fd_buf, fdcnn_info, &fdcnn_size, FDCNN_MAX_OUTNUM);
#if NN_FDCNN_FD_DRAW_YUV
	fdcnn_mask_draw(p_stream->mask_path0, fdcnn_info + 0, (BOOL)(fdcnn_num >= 1), 0x00FF0000);
	fdcnn_mask_draw(p_stream->mask_path1, fdcnn_info + 1, (BOOL)(fdcnn_num >= 2), 0x00FF0000);
	fdcnn_mask_draw(p_stream->mask_path2, fdcnn_info + 2, (BOOL)(fdcnn_num >= 3), 0x00FF0000);
	fdcnn_mask_draw(p_stream->mask_path3, fdcnn_info + 3, (BOOL)(fdcnn_num >= 4), 0x00FF0000);
#else
	for (num = 0; num < 16; num++){
//		printf("insert_fdcnn_result %d %d %d %d \r\n", fdcnn_info[num].x, fdcnn_info[num].y, fdcnn_info[num].w, fdcnn_info[num].h);
		//insert_fdcnn_result(fdcnn_info[num].x, fdcnn_info[num].y, fdcnn_info[num].w, fdcnn_info[num].h);
	}
#endif
	return ret;
}
#endif

#if NN_PVDCNN_DRAW_YUV
int pvdcnn_draw_info(VENDOR_AIS_FLOW_MEM_PARM buf, VIDEO_STREAM *p_stream)
{
	HD_URECT size = {0, 0, OSG_LCD_WIDTH, OSG_LCD_HEIGHT};
	static PVDCNN_RSLT pdcnn_info[FDCNN_MAX_OUTNUM] = {0};
	UINT32 num;
	UINT32 pdcnn_num = pvdcnn_get_result(buf, pdcnn_info, &size, FDCNN_MAX_OUTNUM);
#if NN_PVDCNN_DRAW_YUV
	pvdcnn_mask_draw(p_stream->mask_path4, info + 0, (BOOL)(num >= 1), 0x0000FF00);
	pvdcnn_mask_draw(p_stream->mask_path5, info + 1, (BOOL)(num >= 2), 0x0000FF00);
	pvdcnn_mask_draw(p_stream->mask_path6, info + 2, (BOOL)(num >= 3), 0x0000FF00);
	pvdcnn_mask_draw(p_stream->mask_path7, info + 3, (BOOL)(num >= 4), 0x0000FF00);
#else
	for (num = 0; num < 16; num++){		
//		printf("insert_pvcnn_result %d %d %d %d \r\n", pdcnn_info[num].box.x, pdcnn_info[num].box.y, pdcnn_info[num].box.w, pdcnn_info[num].box.h);
		insert_pvcnn_result(pdcnn_info[num].box.x, pdcnn_info[num].box.y, pdcnn_info[num].box.w, pdcnn_info[num].box.h);
	}
#endif

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
		if (!flow_start){
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

        printf("[FD] ----------- num : %ld ----------- \n", fdcnn_num);
        for(i = 0; i < fdcnn_num; i++ )
        {
            printf("[FD] %ld\t%ld\t%ld\t%ld\t%ld\r\n", fdcnn_info[i].x, fdcnn_info[i].y, fdcnn_info[i].w, fdcnn_info[i].h, fdcnn_info[i].score);
        }
#endif

#if AUTO_FRAMING
	HD_RESULT ret = HD_OK;
	static FDCNN_RESULT fdcnn_info[FDCNN_MAX_OUTNUM] = {0};
	UINT32 fdcnn_num, num;
	static HD_URECT fdcnn_size = {0, 0, OSG_LCD_WIDTH, OSG_LCD_HEIGHT};
	fdcnn_num = fdcnn_getresults(fdcnn_buf, fdcnn_info, &fdcnn_size, FDCNN_MAX_OUTNUM);

	for (num = 0; num < fdcnn_num; num++){
//		printf("insert_fdcnn_result %d %d %d %d \r\n", fdcnn_info[num].x, fdcnn_info[num].y, fdcnn_info[num].w, fdcnn_info[num].h);
		//insert_fdcnn_result(fdcnn_info[num].x, fdcnn_info[num].y, fdcnn_info[num].w, fdcnn_info[num].h);
	}
#endif

#if SEI_HEADER
	cnn_fill_sei_info(fdcnn_buf, FD);
#endif
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
		if (!flow_start){
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

#if AUTO_FRAMING
	HD_URECT pvcnn_size = {0, 0, OSG_LCD_WIDTH, OSG_LCD_HEIGHT};
	static PVDCNN_RSLT pdcnn_info[FDCNN_MAX_OUTNUM] = {0};
	UINT32 num;
	UINT32 pdcnn_num = pvdcnn_get_result(buf, pdcnn_info, &pvcnn_size, FDCNN_MAX_OUTNUM);

	for (num = 0; num < pdcnn_num; num++){		
//		printf("insert_pvcnn_result %d %d %d %d \r\n", pdcnn_info[num].box.x, pdcnn_info[num].box.y, pdcnn_info[num].box.w, pdcnn_info[num].box.h);
		insert_pvcnn_result(pdcnn_info[num].box.x, pdcnn_info[num].box.y, pdcnn_info[num].box.w, pdcnn_info[num].box.h);
	}
#endif

#if SEI_HEADER
	cnn_fill_sei_info(buf, PD);
#endif
    }
    ret = pvdcnn_uninit(buf);
	if (ret != HD_OK) {
		printf("pvdcnn_uninit fail\r\n");
	}
    return 0;
}

#endif//ALG_FD_PD

MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;

	UINT32 enc_type = 1;//0:h265, 1:h264, 2:MJPG
	HD_DIM main_dim;
#if ALG_FD_PD
	//UINT32 out_type = 1;
	AET_CFG_INFO cfg_info = {0};
	//stream[3].hdmi_id=HD_VIDEOOUT_HDMI_1920X1080I60;//default
#endif
	// query program options
	if (argc == 2) {
		enc_type = atoi(argv[1]);
		printf("enc_type %d\r\n", enc_type);
		if(enc_type > 2) {
			printf("error: not support enc_type!\r\n");
			return 0;
		}
	}
#if 0//ALG_FD_PD
	//allocate logo buffer
	stream[0].alphaVision_logo = malloc(STAMP_WIDTH * STAMP_HEIGHT * sizeof(unsigned short));
	if(!stream[0].alphaVision_logo){
        printf("alphaVision_logo = %d\n",stream[0].alphaVision_logo);
		printf("fail to allocate logo buffer\n");
		return -1;
	}

	//load novatek logo from sd card
	if(init_alphaVision_logo(stream[0].alphaVision_logo,STAMP_WIDTH,STAMP_HEIGHT)){

		printf("fail to load stamp image\n");
		free(stream[0].alphaVision_logo);
		return -1;
	}

	// init stamp data
	stream[0].stamp_blk  = 0;
	stream[0].stamp_pa   = 0;
	stream[0].stamp_size = query_osg_buf_size();
	if(stream[0].stamp_size <= 0){
		printf("query_osg_buf_size() fail\n");
		return -1;
	}
#endif
	
	
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
	ret = mem_init(stream[0].stamp_size);
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

	// init all modules
	ret = init_module();
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}

	ret = hd_gfx_init();
	if (ret != HD_OK) {
		DBG_ERR("hd_gfx_init fail=%d\n", ret);
		goto exit;
	}
	
	// open VIDEO_STREAM modules (main)
	stream[0].proc_max_dim.w =MAX_CAP_SIZE_W; //assign by user
	stream[0].proc_max_dim.h =MAX_CAP_SIZE_H; //assign by user

	ret = open_module(&stream[0], &stream[0].proc_max_dim);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

#if ALG_FD_PD

#if NN_FDCNN_FD_DRAW_YUV
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

#if NN_PVDCNN_DRAW_YUV
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
	au_cap.sample_rate_max = HD_AUDIO_SR_32000; //assign by user
	ret = open_module2(&au_cap);
	if(ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	//set audiocap parameter
	au_cap.sample_rate = HD_AUDIO_SR_32000; //assign by user
	ret = set_cap_param2(au_cap.cap_path, au_cap.sample_rate);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}
#endif

#if ALG_FD_PD
	//render mask
#if NN_FDCNN_FD_DRAW_YUV
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

#if NN_PVDCNN_DRAW_YUV
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
	ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	// assign parameter by program options
	main_dim.w = MAX_CAP_SIZE_W;
	main_dim.h = MAX_CAP_SIZE_H;
	
	printf("%s %d\r\n",__func__, __LINE__);

	// set videoproc parameter (main)
	ret = set_proc_param(stream[0].proc_path, &main_dim);
	if (ret != HD_OK) {
		printf("1111set proc fail=%d\n", ret);
		goto exit;
	}

#if ALG_FD_PD
	// set videoproc parameter (alg)
	stream[0].proc_alg_max_dim.w = MAX_CAP_SIZE_W;
	stream[0].proc_alg_max_dim.h = MAX_CAP_SIZE_H;
	ret = set_proc_param(stream[0].proc_alg_path, &stream[0].proc_alg_max_dim);
	if (ret != HD_OK) {
		printf("set proc alg fail=%d\n", ret);
		goto exit;
	}
#endif
	
	// set videoenc config (main)
	stream[0].enc_max_dim.w = MAX_CAP_SIZE_W;
	stream[0].enc_max_dim.h = MAX_CAP_SIZE_H;
	ret = set_enc_cfg(stream[0].enc_path, &stream[0].enc_max_dim, 2 * 1024 * 1024);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}

	// set videoenc parameter (main)
	stream[0].enc_dim.w = main_dim.w;
	stream[0].enc_dim.h = main_dim.h;
	ret = set_enc_param(stream[0].enc_path, &stream[0].enc_dim, enc_type, 2 * 1024 * 1024);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}

	// bind VIDEO_STREAM modules (main)
	hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOENC_0_IN_0);

	// create encode_thread (pull_out bitstream)
	ret = pthread_create(&stream[0].enc_thread_id, NULL, encode_thread, (void *)stream);
	if (ret < 0) {
		printf("create encode thread failed");
		goto exit;
	}
#if AUDIO_ON
	//audio create capture thread
	ret = pthread_create(&cap_thread_id, NULL, capture_thread, (void *)&au_cap);
	if (ret < 0) {
		printf("create encode thread failed");
		goto exit;
	}

	//start audio capture module
	hd_audiocap_start(au_cap.cap_path);
#endif	
	
		printf("%s %d\r\n",__func__, __LINE__);
	// start VIDEO_STREAM modules (main)
	hd_videocap_start(stream[0].cap_path);
	hd_videoproc_start(stream[0].proc_path);
#if ALG_FD_PD
	hd_videoproc_start(stream[0].proc_alg_path);
#endif
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);
	hd_videoenc_start(stream[0].enc_path);
    init_autoframing_data();

#if ALG_FD_PD
	VENDOR_AIS_FLOW_MEM_PARM local_mem = g_mem;
	printf("total mem size = %ld\r\n", local_mem.size);
	
	    // main process
#if NN_FDCNN_FD_MODE
    THREAD_PARM fd_thread_parm;
    pthread_t fd_thread_id;
    UINT32 fd_mem_size = fdcnn_calcbuffsize(NN_FDCNN_FD_TYPE);

    fd_thread_parm.mem = getmem(&local_mem, fd_mem_size);
    fd_thread_parm.stream = stream[0];

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
    pvd_thread_parm.stream = stream[0];

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
#endif//ALG_FD_PD

#if AUTO_FRAMING
	//auto framing
	ret = pthread_create(&stream[0].af_thread_id, NULL, af_thread, (void *)(&stream[0]));
	if (ret < 0) {
		printf("create af thread failed");
		goto exit;
	}
	
#endif

	pthread_mutex_lock(&flow_start_lock);
	flow_start=1;
	pthread_mutex_unlock(&flow_start_lock);

#if UVC_ON
	UVAC_enable(); //init usb device, start to prepare video/audio buffer to send
#endif

	// query user key
	printf("Enter q to exit\n");
	while (1) {
		key = GETCHAR();
		if (key == 'q' || key == 0x3) {
			pthread_mutex_lock(&flow_start_lock);
			flow_start=0;
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
	}

	// destroy encode thread
	pthread_join(stream[0].enc_thread_id, NULL);
#if AUDIO_ON	
	pthread_join(cap_thread_id, NULL); // stop audio
#endif

#if UVC_ON
	UVAC_disable();
#endif

#if AUDIO_ON
	//stop audio capture module
	hd_audiocap_stop(au_cap.cap_path);
#endif
	// stop VIDEO_STREAM modules (main)
	hd_videocap_stop(stream[0].cap_path);
	hd_videoproc_stop(stream[0].proc_path);
#if ALG_FD_PD
	hd_videoproc_stop(stream[0].proc_alg_path);
#endif
	hd_videoenc_stop(stream[0].enc_path);

	// unbind VIDEO_STREAM modules (main)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);

exit:
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
		DBG_ERR("hd_gfx_uninit fail=%d\n", ret);
	}

#if ALG_FD_PD
	ret = release_mem_block();
	if (ret != HD_OK) {
		DBG_ERR("mem_uninit fail=%d\n", ret);
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
	
	return 0;
}
