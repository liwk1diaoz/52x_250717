/**
	@brief Sample code of framebuffer.\n

	@file vdoout_fb_test.c

	@author Janice Huang

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
// platform dependent
#if defined(__LINUX_USER__)
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#define GETCHAR()				getchar()
#define msleep(x)    			usleep(1000*(x))
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#include <kwrap/examsys.h> 	//for MAIN(), GETCHAR() API
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_videoout_test, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          vout_test
#define __DBGLVL__          6 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
 #include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////


#define VDO_DDR_ID		DDR_ID0

#include "hd_videoout_test_func.h"
#include "vendor_videoout.h"
#if defined(CONFIG_NVT_FPGA_EMULATION) || defined(_NVT_FPGA_)
#define _FPGA_EMULATION_    1
#else
#define _FPGA_EMULATION_    0
#endif
#define HDMI_SUP 		    0
#define MAX_ITEM            9
#define MAX_FILE_NAME		64
#define _GFX_ROTATE_        1

typedef struct _VIDEOOUT_TEST {
    UINT32 item;
    UINT32 osd_type;
    UINT32 out_type;
    UINT32 start;
    UINT32 stop;
    UINT32 change_pat;
    UINT32 colorkey;
    UINT32 blend;
    HD_URECT cur_win;
    char filename[MAX_FILE_NAME];
	HD_DIM  out_dim;
    HD_DIM  fb_buf_dim;
    HD_VIDEO_PXLFMT pxlfmt;

    HD_VIDEOOUT_SYSCAPS out_syscaps;
    HD_PATH_ID out_ctrl;
    HD_PATH_ID out_path;
    HD_VIDEOOUT_HDMI_ID hdmi_id;
    pthread_t yuv_thread_id;

	int fb_fd;
    char *fbp;
	HD_COMMON_MEM_VB_BLK blk;
    long int screensize;
    HD_FB_ID fb_id;

} VIDEOOUT_TEST;

typedef UINT32 (*VDOOUT_FUNC)(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);

typedef struct _VOUT_TEST_FUNC {
    char name[32];
    VDOOUT_FUNC test_fp;
    VDOOUT_FUNC err_hdl_fp;
} VOUT_TEST_FUNC;


UINT32 vout_basic(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 vout_basic_err(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 vout_fmt(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 vout_fmt_err(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 vout_dir(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 vout_dir_err(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 vout_dim(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 vout_dim_err(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 vout_fb_fmt(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 vout_fb_fmt_err(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 vout_fb_color(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 vout_fb_blend(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 vout_fb_blend_err(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 vout_fb_dim(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 vout_fb_dim_err(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 vout_all(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 vout_all_err(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
UINT32 testcase(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3);
void show_help(void);
typedef enum _VDOOUT_ITEM {
	VIDEOOUT_OPEN     = 0,
	VIDEOOUT_DIR,
	VIDEOOUT_FMT,
	VIDEOOUT_DIM,
	VIDEOOUT_FB_FMT,
	VIDEOOUT_FB_COLOR,
	VIDEOOUT_FB_BLEND,
	VIDEOOUT_FB_DIM,
	VIDEOOUT_ALL,
	VIDEOOUT_MAX_ITEM,
	ENUM_DUMMY4WORD(VDOOUT_ITEM)
} VDOOUT_ITEM;

VOUT_TEST_FUNC vdoout_item[VIDEOOUT_MAX_ITEM]={
    {"open/close",vout_basic,vout_basic_err},
    {"dir",vout_dir,vout_dir_err},
    {"vdo fmt",vout_fmt,vout_fmt_err},
    {"vdo dim",vout_dim,vout_dim_err},
    {"fb fmt",vout_fb_fmt,vout_fb_fmt_err},
    {"fb colorkey",vout_fb_color,NULL},
    {"fb blend",vout_fb_blend,vout_fb_blend_err},
    {"fb dim",vout_fb_dim,vout_fb_dim_err},
    {"all",vout_all,vout_all_err},
};


typedef enum _VOUT_FB_TEST {
	VOUT_FB_ARGB8888,
	VOUT_FB_ARGB4444,
	VOUT_FB_ARGB1555,
	VOUT_FB_ARGB8565,
	VOUT_FB_INDEX8,
	VOUT_FB_MAX,
} VOUT_FB_TEST;

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

#define DEBUG_MENU 		        (1)

#define VDO_MAX_SIZE_W         1920
#define VDO_MAX_SIZE_H         1080
#define VDO_MAX_BLK_CNT         3
#define FB_MAX_SIZE_W         1920
#define FB_MAX_SIZE_H         1080


#define VDO_420_PAT_W         352
#define VDO_420_PAT_H         288
#define VDO_420_PAT_FMT       HD_VIDEO_PXLFMT_YUV420
#define VDO_420_PAT_PATH  "/mnt/sd/video_frm_352_288_30_yuv420.dat"

#define VDO_422_PAT_W         320
#define VDO_422_PAT_H         240
#define VDO_422_PAT_FMT       HD_VIDEO_PXLFMT_YUV422
#define VDO_422_PAT_PATH  "/mnt/sd/video_frm_320_240_3_yuv422.dat"

#define YUV_BLK_SIZE       (DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_MAX_SIZE_W, VDO_MAX_SIZE_H, VDO_420_PAT_FMT))
#define FB_BLK_SIZE     VDO_YUV_BUFSIZE(FB_MAX_SIZE_W, FB_MAX_SIZE_H, HD_VIDEO_PXLFMT_ARGB8888)

#define FB_BUF_W     320
#define FB_BUF_H     240
///////////////////////////////////////////////////////////////////////////////


static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	mem_cfg.pool_info[0].type     = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = YUV_BLK_SIZE;
	mem_cfg.pool_info[0].blk_cnt  = VDO_MAX_BLK_CNT;
	mem_cfg.pool_info[0].ddr_id   = VDO_DDR_ID;

	// config common pool (IDE1 fb)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_DISP0_FB_POOL;
	mem_cfg.pool_info[1].blk_size = FB_BLK_SIZE;
	mem_cfg.pool_info[1].blk_cnt = 1;
	mem_cfg.pool_info[1].ddr_id = VDO_DDR_ID;
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

static HD_RESULT set_out_cfg(HD_PATH_ID *p_video_out_ctrl, UINT32 out_type,HD_VIDEOOUT_HDMI_ID hdmi_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_MODE videoout_mode = {0};
	HD_PATH_ID video_out_ctrl = 0;

	ret = hd_videoout_open(0, HD_VIDEOOUT_0_CTRL, &video_out_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	//printf("out_type=%d\r\n", (int)(out_type));
    #if 0
    if(out_type ==2) {
        HD_VIDEOOUT_HDMI_ABILITY hdmi_ability ={0};
        ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_HDMI_ABILITY, &hdmi_ability);
        if(ret==0)
        {
            UINT32 ii=0;
            for (ii = 0; ii < hdmi_ability.len; ii++) {
                printf("Video Ability %d: %d\r\n", (int)((UINT32)ii), (int)((UINT32)hdmi_ability.video_id[ii]));
            }
		}
    }
    #endif
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
		DBG_ERR("not support out_type %d\r\n",out_type);
	break;
	}
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
	//DBG_IND("##devcount %d\r\n", (int)(video_out_dev.max_dev_count));

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, p_video_out_syscaps);
	if (ret != HD_OK) {
		return ret;
	}
	return ret;
}

static HD_RESULT set_out_param(HD_PATH_ID video_out_path, HD_DIM *p_dim,UINT32 fmt)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_IN video_out_param={0};

	video_out_param.dim.w = p_dim->w;
	video_out_param.dim.h = p_dim->h;
	video_out_param.pxlfmt = fmt;
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
	//DBG_IND("##video_out_param w:%d,h:%d %x %x\r\n", (int)(video_out_param.dim.w), (int)(video_out_param.dim.h), (unsigned int)(video_out_param.pxlfmt), (unsigned int)(video_out_param.dir));
	return ret;
}

static BOOL check_test_pattern(char *filename)
{
	FILE *f_in;
	char filepath[64];

	sprintf(filepath, filename);

	if ((f_in = fopen(filepath, "rb")) == NULL) {
		DBG_ERR("fail to open %s\n", filepath);
		DBG_ERR("%s is in SDK/code/hdal/samples/pattern/%s\n", filename, filename);
		return FALSE;
	}

	fclose(f_in);
	return TRUE;
}
UINT32 tttt_get_hdal_fmt(UINT32 osd_type)
{
	switch(osd_type) {
		case VOUT_FB_ARGB1555:
			return HD_VIDEO_PXLFMT_ARGB1555;
		case VOUT_FB_ARGB8888:
			return HD_VIDEO_PXLFMT_ARGB8888;
		case VOUT_FB_ARGB4444:
			return HD_VIDEO_PXLFMT_ARGB4444;
		case VOUT_FB_ARGB8565:
			return HD_VIDEO_PXLFMT_ARGB8565;
        case VOUT_FB_INDEX8:
			return HD_VIDEO_PXLFMT_I8;
		default:
			DBG_ERR("not supp %d\r\n", (int)(osd_type));
			return 1;
	}
}
static HD_RESULT set_out_fb(VIDEOOUT_TEST *p_vdoout_test)
{
	HD_RESULT ret = HD_OK;
    HD_PATH_ID video_out_ctrl= p_vdoout_test->out_ctrl;
    UINT32 osd_type = p_vdoout_test->osd_type;
    UINT32 fb_fmt=0;
    UINT32 r=0,g=0,b=0;
    UINT32 g_blend=0xFF;
    HD_FB_ID fb_id =  p_vdoout_test->fb_id;
    //DBG_IND("osd_type %x colorkey %d,blend %d\r\n", (unsigned int)(osd_type), (int)(p_vdoout_test->colorkey), (int)(p_vdoout_test->blend));

    switch(osd_type) {
    case VOUT_FB_ARGB8888:
    {
        r=0;
        g=0xff;
        b=0;
    }
    break;
    case VOUT_FB_ARGB4444:
    {
        r=0;
        g=0xf;
        b=0;
    }
    break;
    case VOUT_FB_ARGB1555:
    {
        r=0;
        g=0x1f;
        b=0;
        if(p_vdoout_test->blend)
            g_blend = 0x80;

    }
    break;
    case VOUT_FB_ARGB8565:
    {
        r=0;
        g=0x3F;
        b=0;
    }
    break;
    case VOUT_FB_INDEX8:
    {
        r=0;
        g=0xff;
        b=0;
        vdoout_test_palette(video_out_ctrl,fb_id);
    }
    break;
    default:
        DBG_ERR("not sup %d\r\n", (int)(osd_type));
        return -1;
    }

    fb_fmt = tttt_get_hdal_fmt(osd_type);
	ret =vdoout_test_format(video_out_ctrl,fb_id,fb_fmt);
	if (ret != HD_OK) {
		return ret;
	}
    if(p_vdoout_test->colorkey) {
    	ret = vdoout_test_attr(video_out_ctrl,fb_id,g_blend,r,g,b);
    } else {
        ret = vdoout_test_attr(video_out_ctrl,fb_id,g_blend,0,0,0);
    }
	if (ret != HD_OK) {
		return ret;
	}
    ret =vdoout_test_fb_dim(video_out_ctrl, &p_vdoout_test->fb_buf_dim, &p_vdoout_test->cur_win);
    return ret;
}

static void Fill8888Color(UINT32 color,UINT32 uiBufAddr,UINT32 uiBufSize)
{
	UINT32 i=0;

	for(i=0;i<uiBufSize/4;i++)
	{
		*(UINT32 *)(uiBufAddr+4*i) = color;
	}
}

static void FillColor16bit(UINT16 color,UINT32 uiBufAddr,UINT32 uiBufSize)
{
	UINT32 i=0;

	for(i=0;i<uiBufSize/2;i++)
	{
		*(UINT16 *)(uiBufAddr+2*i) = color;
	}
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videoout_init()) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module(VIDEOOUT_TEST *p_vdoout_test,UINT32 out_type)
{
    HD_RESULT ret;

	ret = set_out_cfg(&p_vdoout_test->out_ctrl,out_type,p_vdoout_test->hdmi_id);
	if (ret != HD_OK) {
		DBG_ERR("set out-cfg fail=%d\n", (int)(ret));
		return HD_ERR_NG;
	}

    if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0, &p_vdoout_test->out_path)) != HD_OK)
        return ret;
    return HD_OK;
}

static HD_RESULT close_module(VIDEOOUT_TEST *p_vdoout_test)
{
	HD_RESULT ret;
	if ((ret = hd_videoout_close(p_vdoout_test->out_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videoout_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}

static INT32 blk2idx(HD_COMMON_MEM_VB_BLK blk)   // convert blk(0xXXXXXXXX) to index (0, 1, 2)
{
	static HD_COMMON_MEM_VB_BLK blk_registered[VDO_MAX_BLK_CNT] = {0};
	INT32 i;
	for (i=0; i< VDO_MAX_BLK_CNT; i++) {
		if (blk_registered[i] == blk) return i;

		if (blk_registered[i] == 0) {
			blk_registered[i] = blk;
			return i;
		}
	}

	DBG_ERR("convert blk(%0x%x) to index fail !!!!\r\n", (unsigned int)(blk));
	return (-1);
}

static void *feed_yuv_thread(void *arg)
{
    VIDEOOUT_TEST *p_vdoout_test = (VIDEOOUT_TEST *)arg;
	HD_RESULT ret = HD_OK;
	int i;
	HD_COMMON_MEM_DDR_ID ddr_id = VDO_DDR_ID;
	UINT32 blk_size = YUV_BLK_SIZE;
	UINT32 yuv_size = 0;
	char filepath_yuv_main[MAX_FILE_NAME];
	FILE *f_in_main;
	UINT32 pa_yuv_main[VDO_MAX_BLK_CNT] = {0};
	UINT32 va_yuv_main[VDO_MAX_BLK_CNT] = {0};
	INT32  blkidx;
	HD_COMMON_MEM_VB_BLK blk;
    UINT32 readbyte = 0 ;

	//------ [1] wait flow_start ------
	while (p_vdoout_test->start== 0) sleep(1);

    yuv_size = VDO_YUV_BUFSIZE(p_vdoout_test->out_dim.w, p_vdoout_test->out_dim.h, p_vdoout_test->pxlfmt);
	//------ [2] open input files ------
	sprintf(filepath_yuv_main, p_vdoout_test->filename);
	if ((f_in_main = fopen(filepath_yuv_main, "rb")) == NULL) {
		DBG_ERR("open file (%s) fail !!....\r\nPlease copy test pattern to SD Card !!\r\n\r\n", filepath_yuv_main);
		return 0;
	}

	//------ [3] feed yuv ------
	while (p_vdoout_test->stop == 0) {

        if(p_vdoout_test->change_pat == 1) {
        	// close file
        	if (f_in_main)  {
                fclose(f_in_main);
                f_in_main = 0;
        	}

            yuv_size = VDO_YUV_BUFSIZE(p_vdoout_test->out_dim.w, p_vdoout_test->out_dim.h, p_vdoout_test->pxlfmt);
        	//------ [2] open input files ------
        	sprintf(filepath_yuv_main, p_vdoout_test->filename);
        	if ((f_in_main = fopen(filepath_yuv_main, "rb")) == NULL) {
        		DBG_ERR("open file (%s) fail !!....\r\nPlease copy test pattern to SD Card !!\r\n\r\n", filepath_yuv_main);
        		return 0;
        	}
            p_vdoout_test->change_pat = 0;
        }

		//--- Get memory ---
		blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id); // Get block from mem pool
		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			DBG_ERR("get block fail (0x%x).. try again later.....\r\n", (unsigned int)(blk));
			sleep(1);
			continue;
		}

		if ((blkidx = blk2idx(blk)) == -1) {
			DBG_ERR("ERROR !! blk to idx fail !!\r\n");
			goto rel_blk;
		}

		pa_yuv_main[blkidx] = hd_common_mem_blk2pa(blk); // Get physical addr
		if (pa_yuv_main[blkidx] == 0) {
			DBG_ERR("blk2pa fail, blk = 0x%x\r\n", (unsigned int)(blk));
			goto rel_blk;
		}

		if (va_yuv_main[blkidx] == 0) { // if NOT mmap yet, mmap it
			va_yuv_main[blkidx] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa_yuv_main[blkidx], blk_size); // Get virtual addr
			if (va_yuv_main[blkidx] == 0) {
				DBG_ERR("Error: mmap fail !! pa_yuv_main[%d], blk = 0x%x\r\n", (int)(blkidx), (unsigned int)(blk));
			    goto rel_blk;
			}
		}

		//--- Read YUV from file ---
		readbyte = fread((void *)va_yuv_main[blkidx], 1, yuv_size, f_in_main);
        if(readbyte<yuv_size){
            DBG_DUMP("read data %d< %d\r\n", (int)(readbyte), (int)(yuv_size));
        }
		//printf("blkidx %d yuv_size %x %x\r\n ", (int)(blkidx), (unsigned int)(yuv_size), (unsigned int)(*(UINT32 *)va_yuv_main[blkidx]));

		if (feof(f_in_main)) {
			fseek(f_in_main, 0, SEEK_SET);  //rewind
			readbyte = fread((void *)va_yuv_main[blkidx], 1, yuv_size, f_in_main);
            if(readbyte<yuv_size)
                DBG_DUMP("loop read data %d< %d\r\n", (int)(readbyte), (int)(yuv_size));

		}

		//--- data is written by CPU, flush CPU cache to PHY memory ---
		hd_common_mem_flush_cache((void *)va_yuv_main[blkidx], yuv_size);

		//--- push_in ---
		{
			HD_VIDEO_FRAME video_frame;

			video_frame.sign        = MAKEFOURCC('V','F','R','M');
			video_frame.p_next      = NULL;
			video_frame.ddr_id      = ddr_id;
			video_frame.pxlfmt      = p_vdoout_test->pxlfmt;
			video_frame.dim.w       = p_vdoout_test->out_dim.w;
			video_frame.dim.h       = p_vdoout_test->out_dim.h;
			video_frame.count       = 0;
			video_frame.timestamp   = hd_gettime_us();
			video_frame.loff[0]     = p_vdoout_test->out_dim.w; // Y
			video_frame.loff[1]     = p_vdoout_test->out_dim.w; // UV
			video_frame.phy_addr[0] = pa_yuv_main[blkidx];                          // Y
			video_frame.phy_addr[1] = pa_yuv_main[blkidx] + video_frame.dim.w*video_frame.dim.h;  // UV pack
			video_frame.blk         = blk;
            //DBG_IND("w:%d,h:%d\r\n",(int)video_frame.dim.w,(int)video_frame.dim.h);
			ret = hd_videoout_push_in_buf(p_vdoout_test->out_path, &video_frame, NULL, 0); // only support non-blocking mode now
			if (ret != HD_OK) {
				DBG_ERR("push_in error %d!!\r\n", (int)(ret));
			}
		}
rel_blk:
		//--- Release memory ---
		ret = hd_common_mem_release_block(blk);
    	if(ret != HD_OK) {
    		DBG_ERR("rel block=%d\n", (int)(ret));
    	}

		msleep(500);

	}
	//------ [4] uninit & exit ------
	// mummap for yuv buffer
	for (i=0; i< VDO_MAX_BLK_CNT; i++) {
        if(va_yuv_main[i]){
    		hd_common_mem_munmap((void *)va_yuv_main[i], blk_size);
            va_yuv_main[i] = 0;
        }
	}
	// close file
	if (f_in_main)  fclose(f_in_main);

	return 0;
}
MAIN(argc, argv)
{
    VIDEOOUT_TEST vdoout_test = {0};
    VIDEOOUT_TEST vdoout_test_keep = {0};
    INT32 count =1; //defult run once
    INT32 i=0;
	HD_RESULT ret;
    INT32 error_test =0;

	if (argc >= 2) {

        if(*argv[1] == '?') {
            show_help();
            return 0;
        }

		vdoout_test.item = atoi(argv[1]);
		DBG_DUMP("item %d\r\n", (int)(vdoout_test.item));
		if(vdoout_test.item > MAX_ITEM) {
			DBG_ERR("error: not support item! MAX_ITEM is %d\r\n",MAX_ITEM);
			return 0;
		}
	} else {
        show_help();
        return 0;
    }

    if (argc >= 3) {
		count = atoi(argv[2]);
		DBG_DUMP("count %d\r\n", (int)(count));
	}

    vdoout_test.osd_type = VOUT_FB_ARGB8888; //default ARGB8888

    if (argc >= 4) {
		vdoout_test.osd_type = atoi(argv[3]);
		DBG_DUMP("osd_type %d\r\n", (int)(vdoout_test.osd_type));
		if(vdoout_test.osd_type > VOUT_FB_MAX) {
			DBG_ERR("error: not support osd_type!\r\n");
			return 0;
		}
	}

    //set default YUV420 pattern
	strncpy(vdoout_test.filename,VDO_420_PAT_PATH,MAX_FILE_NAME-1);

	// check TEST pattern exist
	if (check_test_pattern(vdoout_test.filename) == FALSE) {
		DBG_ERR("test_pattern isn't exist\r\n");
		return -1;
	}

    vdoout_test.out_dim.w = VDO_420_PAT_W;
    vdoout_test.out_dim.h = VDO_420_PAT_H;
    vdoout_test.pxlfmt = VDO_420_PAT_FMT;


    vdoout_test.out_type = 1; //520 only support LCD
    vdoout_test.fb_id = HD_FB0;
    #if (HDMI_SUP)

	if (argc >= 5) {
		vdoout_test.out_type = atoi(argv[4]);
		DBG_DUMP("out_type %d\r\n", (int)(vdoout_test.out_type));
		if(vdoout_test.out_type > 2) {
			DBG_ERR("error: not support out_type!\r\n");
			return 0;
		}
	}

    vdoout_test.hdmi_id=HD_VIDEOOUT_HDMI_1920X1080I60;//default
	// query program options
	if (argc >= 6) {
		vdoout_test.hdmi_id = atoi(argv[5]);
		DBG_DUMP("hdmi_mode %d\r\n", (int)(vdoout_test.hdmi_id));
	}
    #else
    //test error handle
	if (argc >= 5) {
		error_test = atoi(argv[4]);
		DBG_DUMP("error_test %d\r\n", error_test);
	}

    #endif

    //keep item setting for burn-in test
    memcpy((void *)&vdoout_test_keep,(void *)&vdoout_test,sizeof(VIDEOOUT_TEST));

	// init hdal
	ret = hd_common_init(0);
    if(ret != HD_OK) {
        DBG_ERR("common fail=%d\n", (int)(ret));
        return -1;
    }
    //vdoout_test memory init
	ret = mem_init();
	if (ret != HD_OK) {
		DBG_ERR("mem fail=%d\n", (int)(ret));
		return -1;
	}

    if((count>1)&&(count<65565)) {
        for(i=0;i<count;i++) {
            DBG_DUMP("######## loop %d\r\n", (int)(i));
            testcase(&vdoout_test,0,0,0);
            memcpy((void *)&vdoout_test,(void *)&vdoout_test_keep,sizeof(VIDEOOUT_TEST));
        }
    }else if(error_test==1) {
        testcase(&vdoout_test,1,0,1);
    }else {
        testcase(&vdoout_test,1,0,0);
    }

    //uninit memory
    ret = mem_exit();
    if(ret != HD_OK) {
        DBG_ERR("mem fail=%d\n", (int)(ret));
    }
    //uninit hdal
	ret = hd_common_uninit();
    if(ret != HD_OK) {
        DBG_ERR("uninit common fail=%d\n", (int)(ret));
    }

    return 0;
}

UINT32 tttt_waitkey(UINT32 b_wait,char *chk_str)
{
	UINT32 key = 0;
    UINT32 result =0;
    if(b_wait) {
        DBG_DUMP("check %s,enter 'c' to continue \r\n",chk_str);
        while (1) {
    		key = GETCHAR();
    		if (key == 'c' ) {
                result =0;
    			break;
    		}
    		if (key == 'f' ) {
                result =-1;
    			break;
    		}
    		if (key == 0x3) {
                result = -3;
    			break;
    		}
        }
    } else {
        DBG_DUMP("check %s\r\n",chk_str);
        sleep(1);
    }

    return result;
}
int tttt_start(VIDEOOUT_TEST *p_vdoout_test)
{
	HD_RESULT ret;

    //vdoout_test module init
    ret = init_module();
    if(ret != HD_OK) {
        DBG_ERR("init fail=%d\n", (int)(ret));
        return -1;
    }
    //open vdoout_test module
    ret = open_module(p_vdoout_test,p_vdoout_test->out_type);
    if(ret != HD_OK) {
        DBG_ERR("open fail=%d\n", (int)(ret));
        return -1;
    }

	// get videoout capability
	ret = get_out_caps(p_vdoout_test->out_ctrl, &p_vdoout_test->out_syscaps);
	if (ret != HD_OK) {
		DBG_ERR("get out-caps fail=%d\n", (int)(ret));
		return -1;
	}
    memcpy((void *)&p_vdoout_test->fb_buf_dim,(void *)&p_vdoout_test->out_syscaps.output_dim,sizeof(HD_DIM));
    p_vdoout_test->cur_win.x=0;
    p_vdoout_test->cur_win.y=0;
    p_vdoout_test->cur_win.w=p_vdoout_test->out_syscaps.output_dim.w;
    p_vdoout_test->cur_win.h=p_vdoout_test->out_syscaps.output_dim.h;
	// set videoout parameter
	ret = set_out_param(p_vdoout_test->out_path, &p_vdoout_test->out_dim,p_vdoout_test->pxlfmt);
	if (ret != HD_OK) {
		DBG_ERR("set out fail=%d\n", (int)(ret));
		return -1;
	}

	ret = pthread_create(&p_vdoout_test->yuv_thread_id, NULL, feed_yuv_thread, (void *)p_vdoout_test);
	if (ret < 0) {
		DBG_ERR("create vout thread failed");
		return -1;
	}

	ret = hd_videoout_start(p_vdoout_test->out_path);
    if (ret == HD_OK) {
        p_vdoout_test->stop = 0;
    	p_vdoout_test->start = 1;
        msleep(500); //for thread load pattern
	}
    return ret;
}

void tttt_stop(VIDEOOUT_TEST *p_vdoout_test)
{
    UINT32 ret=0;

    //exit:
    p_vdoout_test->stop = 1;
	p_vdoout_test->start = 0;

	pthread_join(p_vdoout_test->yuv_thread_id, NULL);
	ret=hd_videoout_stop(p_vdoout_test->out_path);
    if (ret != HD_OK) {
		DBG_ERR("stop fail=%d\n", (int)(ret));
	}
    #if defined(__LINUX_USER__)
	if(p_vdoout_test->fb_fd>=0) {
   		close(p_vdoout_test->fb_fd);
        p_vdoout_test->fb_fd = 0;
	}
    #endif
	if(p_vdoout_test->fbp!=0) {
        VENDOR_FB_UNINIT fb_uninit={0};
        #if defined(__LINUX_USER__)
		munmap(p_vdoout_test->fbp,p_vdoout_test->screensize);
        #endif
        fb_uninit.fb_id = p_vdoout_test->fb_id;
    	ret = vendor_videoout_set(VENDOR_VIDEOOUT_ID0, VENDOR_VIDEOOUT_ITEM_FB_UNINIT, &fb_uninit);
    	if(ret!= HD_OK)
    		DBG_ERR("FB_DEINIT fail=%d\n", (int)(ret));

        p_vdoout_test->fbp=0;
	}

	//--- Release memory ---
	if(p_vdoout_test->blk) {
		ret = hd_common_mem_release_block(p_vdoout_test->blk);
    	if(ret != HD_OK) {
    		DBG_ERR("rel block=%d\n", (int)(ret));
    	}
        p_vdoout_test->blk = 0 ;
	}
    //close all module
	ret = close_module(p_vdoout_test);
	if(ret != HD_OK) {
		DBG_ERR("close fail=%d\n", (int)(ret));
	}
    //uninit all module
	ret = exit_module();
	if(ret != HD_OK) {
		DBG_ERR("exit fail=%d\n", (int)(ret));
	}

    msleep(500);

}

void tttt_fill_pattern(VIDEOOUT_TEST *p_vdoout_test)
{
    char *fbp = 0;
    UINT32 osd_type = p_vdoout_test->osd_type;
    UINT32 screenW = p_vdoout_test->fb_buf_dim.w;
    UINT32 screenH = p_vdoout_test->fb_buf_dim.h;
    UINT32 fmt_coef =0;
    //DBG_IND("screensize %x %d %d\r\n", (unsigned int)(p_vdoout_test->screensize), (int)(screenW), (int)(screenH));

    fbp=vdoout_fbm_getflip();
    if(!fbp){
        DBG_ERR("no fbp\r\n");
        return ;
    }

    switch(osd_type) {
        case VOUT_FB_ARGB8888:
        {
            fmt_coef = 4;
        	Fill8888Color(ARBG_8888_RED,(UINT32)fbp,screenW*screenH*4);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
            fbp=vdoout_fbm_getflip();
            if(!fbp){
                DBG_ERR("no fbp\r\n");
                return ;
            }
        	Fill8888Color(ARBG_8888_RED,(UINT32)fbp,screenW*screenH*4);
        	Fill8888Color(ARBG_8888_GREEN,(UINT32)fbp+screenW*screenH*2,screenW*screenH*2);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
            fbp=vdoout_fbm_getflip();
            if(!fbp){
                DBG_ERR("no fbp\r\n");
                return ;
            }
        	Fill8888Color(ARBG_8888_BLUE,(UINT32)fbp,screenW*screenH*4);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
            fbp=vdoout_fbm_getflip();
            if(!fbp){
                DBG_ERR("no fbp\r\n");
                return ;
            }
        	Fill8888Color(ARBG_8888_BLUE,(UINT32)fbp,screenW*screenH*4);
            if(p_vdoout_test->blend) {
        	    Fill8888Color(ARBG_8888_PINK_50,(UINT32)fbp,screenW*screenH*2);
            } else {
        	    Fill8888Color(ARBG_8888_PINK,(UINT32)fbp,screenW*screenH*2);
            }
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
        }
        break;
        case VOUT_FB_ARGB4444:
        {
            fmt_coef = 2;
        	FillColor16bit(ARBG_4444_RED,(UINT32)fbp,screenW*screenH*2);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
            fbp=vdoout_fbm_getflip();
            if(!fbp){
                DBG_ERR("no fbp\r\n");
                return ;
            }
        	FillColor16bit(ARBG_4444_RED,(UINT32)fbp,screenW*screenH*2);
        	FillColor16bit(ARBG_4444_GREEN,(UINT32)fbp+screenW*screenH,screenW*screenH);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
            fbp=vdoout_fbm_getflip();
            if(!fbp){
                DBG_ERR("no fbp\r\n");
                return ;
            }
        	FillColor16bit(ARBG_4444_BLUE,(UINT32)fbp,screenW*screenH*2);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
            fbp=vdoout_fbm_getflip();
            if(!fbp){
                DBG_ERR("no fbp\r\n");
                return ;
            }
        	FillColor16bit(ARBG_4444_BLUE,(UINT32)fbp,screenW*screenH*2);
            if(p_vdoout_test->blend) {
        	    FillColor16bit(ARBG_4444_PINK_50,(UINT32)fbp,screenW*screenH);
            } else {
        	    FillColor16bit(ARBG_4444_PINK,(UINT32)fbp,screenW*screenH);
            }
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
        }
        break;
        case VOUT_FB_ARGB1555:
        {
            fmt_coef = 2;
        	FillColor16bit(ARBG_1555_RED,(UINT32)fbp,screenW*screenH*2);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
            fbp=vdoout_fbm_getflip();
            if(!fbp){
                DBG_ERR("no fbp\r\n");
                return ;
            }
        	FillColor16bit(ARBG_1555_RED,(UINT32)fbp,screenW*screenH*2);
        	FillColor16bit(ARBG_1555_GREEN,(UINT32)fbp+screenW*screenH,screenW*screenH);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
            fbp=vdoout_fbm_getflip();
            if(!fbp){
                DBG_ERR("no fbp\r\n");
                return ;
            }
        	FillColor16bit(ARBG_1555_BLUE,(UINT32)fbp,screenW*screenH*2);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
            fbp=vdoout_fbm_getflip();
            if(!fbp){
                DBG_ERR("no fbp\r\n");
                return ;
            }
        	FillColor16bit(ARBG_1555_BLUE,(UINT32)fbp,screenW*screenH*2);
        	FillColor16bit(ARBG_1555_PINK,(UINT32)fbp,screenW*screenH);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
        }
        break;
        case VOUT_FB_ARGB8565:
        {
            //ARGB8565 is 2 plan
            //alpha plan is the end of RGB
            #if 0
            char *fbp_alpha = (char *)ARGB8565_A_ADDR((UINT32)fbp,screenW,screenH);
            #else
            char *fbp_alpha = (char *)ARGB8565_A_ADDR((UINT32)fbp,screenW,screenH);
            #endif
            memset(fbp_alpha,0xFF,screenW*screenH); //set alpha plan 0xFF

            fmt_coef = 3;
        	FillColor16bit(ARBG_565_RED,(UINT32)fbp,screenW*screenH*2);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
            fbp=vdoout_fbm_getflip();
            if(!fbp){
                DBG_ERR("no fbp\r\n");
                return ;
            }
            fbp_alpha = (char *)ARGB8565_A_ADDR((UINT32)fbp,screenW,screenH);
            memset(fbp_alpha,0xFF,screenW*screenH); //set alpha plan 0xFF
        	FillColor16bit(ARBG_565_RED,(UINT32)fbp,screenW*screenH*2);
        	FillColor16bit(ARBG_565_GREEN,(UINT32)fbp+screenW*screenH,screenW*screenH);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
            fbp=vdoout_fbm_getflip();
            if(!fbp){
                DBG_ERR("no fbp\r\n");
                return ;
            }
            fbp_alpha = (char *)ARGB8565_A_ADDR((UINT32)fbp,screenW,screenH);
            memset(fbp_alpha,0xFF,screenW*screenH); //set alpha plan 0xFF
        	FillColor16bit(ARBG_565_BLUE,(UINT32)fbp,screenW*screenH*2);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
            fbp=vdoout_fbm_getflip();
            if(!fbp){
                DBG_ERR("no fbp\r\n");
                return ;
            }

            fbp_alpha = (char *)ARGB8565_A_ADDR((UINT32)fbp,screenW,screenH);
            memset(fbp_alpha,0xFF,screenW*screenH); //set alpha plan 0xFF
        	FillColor16bit(ARBG_565_BLUE,(UINT32)fbp,screenW*screenH*2);
            if(p_vdoout_test->blend) {
                memset(fbp_alpha,0x80,screenW*screenH/2); //set alpha plan 0x80 for pink
            }
        	FillColor16bit(ARBG_565_PINK,(UINT32)fbp,screenW*screenH);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
        }
        break;
        case VOUT_FB_INDEX8:
        {
            fmt_coef = 1;

        	memset(fbp,INDEX8_RED,screenW*screenH);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
            fbp=vdoout_fbm_getflip();
            if(!fbp){
                DBG_ERR("no fbp\r\n");
                return ;
            }
            memset(fbp,INDEX8_RED,screenW*screenH);
        	memset(fbp+screenW*screenH/2,INDEX8_GREEN,screenW*screenH/2);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
            fbp=vdoout_fbm_getflip();
            if(!fbp){
                DBG_ERR("no fbp\r\n");
                return ;
            }
        	memset(fbp,INDEX8_BLUE,screenW*screenH);
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
            fbp=vdoout_fbm_getflip();
            if(!fbp){
                DBG_ERR("no fbp\r\n");
                return ;
            }
            memset(fbp,INDEX8_BLUE,screenW*screenH);
            if(p_vdoout_test->blend) {
                memset(fbp,INDEX8_PINK_50,screenW*screenH/2); //set alpha plan 0xF0 for pink
            } else {
            	memset(fbp,INDEX8_PINK,screenW*screenH/2);
            }
            hd_common_mem_flush_cache((void *)fbp, screenW*screenH*fmt_coef);
            vdoout_fbm_setflip();
        	sleep(1);
        }
        break;
        default:
            DBG_ERR("not sup %d\r\n", (int)(osd_type));
        }

}

int tttt_fb(VIDEOOUT_TEST *p_vdoout_test)
{
    ////////FB param begin
    #if defined(__LINUX_USER__)
    struct fb_fix_screeninfo finfo;
    #endif
	HD_COMMON_MEM_DDR_ID ddr_id = VDO_DDR_ID;
	UINT32 blk_size = FB_BLK_SIZE;
	UINT32 pa_fb0 = 0;
    VENDOR_FB_INIT fb_init;
    UINT32 ret=0;
    VOUT_FBM_INIT init_obj={0};

    ////////FB param end


	//--- Get DISP0_FB memory ---
	p_vdoout_test->blk = hd_common_mem_get_block(HD_COMMON_MEM_DISP0_FB_POOL, blk_size, ddr_id); // Get block from mem pool
	if (p_vdoout_test->blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_ERR("get block fail (0x%x).. try again later.....\r\n", (unsigned int)(p_vdoout_test->blk));
		return -1;
	}

	pa_fb0 = hd_common_mem_blk2pa(p_vdoout_test->blk); // Get physical addr
	if (pa_fb0 == 0) {
		DBG_ERR("blk2pa fail, blk = 0x%x\r\n", (unsigned int)(p_vdoout_test->blk));
		return -1;
	}

    // set videoout fb config
	ret = set_out_fb(p_vdoout_test);
	if (ret != HD_OK) {
		DBG_ERR("set fb fail=%d\n", (int)(ret));
		return -1;
	}

    msleep(500); //for fb_misc device

    //assign bf buffer to FB_INIT
    fb_init.fb_id = p_vdoout_test->fb_id;
    fb_init.pa_addr = pa_fb0;
    fb_init.buf_len = blk_size;
	ret = vendor_videoout_set(VENDOR_VIDEOOUT_ID0, VENDOR_VIDEOOUT_ITEM_FB_INIT, &fb_init);
	if(ret!= HD_OK) {
		DBG_ERR("init FB %d\n",ret);
		return ret;
	}
    #if _FPGA_EMULATION_
    sleep(5); //for FPGA /dev/fb0 need 5 sec
    #else
    sleep(1);
    #endif

    #if defined(__LINUX_USER__)
	p_vdoout_test->fb_fd = open("/dev/fb0", O_RDWR);
	if (p_vdoout_test->fb_fd<0)	{
		DBG_ERR("Cannot open framebuffer device.\n");
		return -1;
	}

    if (ioctl(p_vdoout_test->fb_fd, FBIOGET_FSCREENINFO, &finfo)) {
        DBG_ERR("Error reading fixed information.\n");
        return -1;
    }

    p_vdoout_test->screensize =  finfo.smem_len;

    p_vdoout_test->fbp = (char *)mmap(0, p_vdoout_test->screensize, PROT_READ | PROT_WRITE, MAP_SHARED,p_vdoout_test->fb_fd, 0);
    #else

    p_vdoout_test->screensize=blk_size;
    p_vdoout_test->fbp = (char *)pa_fb0;
    //DBG_IND("freertos: addr %x size %x \r\n", (unsigned int)(pa_fb0), (unsigned int)(blk_size));

    #endif
    if ((int)p_vdoout_test->fbp == -1) {
        DBG_ERR("failed to map framebuffer device to memory.\n");
    	return -1;
    }

    //for buffer flip
    init_obj.width = p_vdoout_test->fb_buf_dim.w;
    init_obj.height = p_vdoout_test->fb_buf_dim.h;
    init_obj.addr = (UINT32)p_vdoout_test->fbp;
    init_obj.size = FB_BLK_SIZE;
    init_obj.format = tttt_get_hdal_fmt(p_vdoout_test->osd_type);
    ret = vdoout_fbm_init(&init_obj);

    vdoout_test_enable(p_vdoout_test->out_ctrl,p_vdoout_test->fb_id,1);

    return ret;
}
UINT32 vout_basic(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    int ret=0;

    ret=tttt_start(p_vdoout_test);
    if(ret!=0){
        return ret;
    }
    tttt_waitkey(parm1,"basic");
    tttt_stop(p_vdoout_test);
    return ret;
}

UINT32 tttt_fmt(VIDEOOUT_TEST *p_vdoout_test,UINT32 fmt)
{
	HD_VIDEOOUT_IN video_in_param = {0};
    int ret=0;

    ret = hd_videoout_get(p_vdoout_test->out_path, HD_VIDEOOUT_PARAM_IN, &video_in_param);
    if(ret!=0){
        return ret;
    }
    video_in_param.pxlfmt = fmt;

	ret = hd_videoout_set(p_vdoout_test->out_path, HD_VIDEOOUT_PARAM_IN, &video_in_param);
    if(ret!=0){
        return ret;
    }
	ret = hd_videoout_start(p_vdoout_test->out_path);
    return ret;

}
UINT32 vout_fmt(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    VIDEOOUT_TEST vdoout_test_keep = {0};
    int ret=0;

    memcpy((void *)&vdoout_test_keep,(void *)p_vdoout_test,sizeof(VIDEOOUT_TEST));

    //set YUV420 pattern
	strncpy(p_vdoout_test->filename,VDO_420_PAT_PATH,MAX_FILE_NAME-1);

	// check TEST pattern exist
	if (check_test_pattern(p_vdoout_test->filename) == FALSE) {
		DBG_ERR("test_pattern isn't exist\r\n");
		return -1;
	}
    p_vdoout_test->out_dim.w = VDO_420_PAT_W;
    p_vdoout_test->out_dim.h = VDO_420_PAT_H;
    p_vdoout_test->pxlfmt = VDO_420_PAT_FMT;

    ret=tttt_start(p_vdoout_test);
    if(ret!=0){
        return ret;
    }
    ret = tttt_fmt(p_vdoout_test,p_vdoout_test->pxlfmt);
    if(ret!=0){
        return ret;
    }
    tttt_waitkey(parm1,"YUV420");


    //set YUV420 pattern
	strncpy(p_vdoout_test->filename,VDO_422_PAT_PATH,MAX_FILE_NAME-1);

	// check TEST pattern exist
	if (check_test_pattern(p_vdoout_test->filename) == FALSE) {
		DBG_ERR("test_pattern isn't exist\r\n");
		return -1;
	}
    p_vdoout_test->out_dim.w = VDO_422_PAT_W;
    p_vdoout_test->out_dim.h = VDO_422_PAT_H;
    p_vdoout_test->pxlfmt = VDO_422_PAT_FMT;
    p_vdoout_test->change_pat = 1;
    sleep(1);
    ret=tttt_fmt(p_vdoout_test,p_vdoout_test->pxlfmt);
    if(ret!=0){
        return ret;
    }
    tttt_waitkey(parm1,"YUV422");

    tttt_stop(p_vdoout_test);
    return ret;
}
UINT32 tttt_dir(VIDEOOUT_TEST *p_vdoout_test,UINT32 dir)
{
	HD_VIDEOOUT_IN video_in_param = {0};
    int ret=0;

    ret = hd_videoout_get(p_vdoout_test->out_path, HD_VIDEOOUT_PARAM_IN, &video_in_param);
    if(ret!=0) {
        return ret;
    }
    video_in_param.dir = dir;

	ret = hd_videoout_set(p_vdoout_test->out_path, HD_VIDEOOUT_PARAM_IN, &video_in_param);
    if(ret!=0) {
        DBG_ERR("ret %d \r\n",ret);
        return ret;
    }
	ret = hd_videoout_start(p_vdoout_test->out_path);
    return ret;

}
UINT32 vout_dir(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    int ret=0;
    ret=tttt_start(p_vdoout_test);
    if(ret!=0){
        return ret;
    }
    ret=tttt_dir(p_vdoout_test,HD_VIDEO_DIR_NONE);
    if(ret==0){
        tttt_waitkey(parm1,"basic");
    }
    ret=tttt_dir(p_vdoout_test,HD_VIDEO_DIR_MIRRORX);
    if(ret==0){
        tttt_waitkey(parm1,"mirror x");
    }
    ret=tttt_dir(p_vdoout_test,HD_VIDEO_DIR_MIRRORY);
    if(ret==0){
        tttt_waitkey(parm1,"mirror y");
    }
    ret=tttt_dir(p_vdoout_test,HD_VIDEO_DIR_MIRRORX|HD_VIDEO_DIR_MIRRORY);
    if(ret==0){
        tttt_waitkey(parm1,"mirror x|y");
    }
    #if _GFX_ROTATE_
    ret=tttt_dir(p_vdoout_test,HD_VIDEO_DIR_ROTATE_90);
    if(ret==0){
        tttt_waitkey(parm1,"rotate 90");
    }
    ret=tttt_dir(p_vdoout_test,HD_VIDEO_DIR_ROTATE_270);
    if(ret==0){
        tttt_waitkey(parm1,"rotate 270");
    }
    ret=tttt_dir(p_vdoout_test,HD_VIDEO_DIR_ROTATE_90|HD_VIDEO_DIR_MIRRORX);
    if(ret==0){
        tttt_waitkey(parm1,"rotate 90 and mirror x");
    }
    ret=tttt_dir(p_vdoout_test,HD_VIDEO_DIR_ROTATE_270|HD_VIDEO_DIR_MIRRORY);
    if(ret==0){
        tttt_waitkey(parm1,"rotate 270 and mirror y");
    }
    #endif
    tttt_stop(p_vdoout_test);
    return ret;
}
UINT32 tttt_dim(VIDEOOUT_TEST *p_vdoout_test,HD_URECT *p_out_cur_win)
{
    HD_VIDEOOUT_WIN_ATTR video_out_param;
    UINT32 ret = 0;

    video_out_param.visible = 1;
    video_out_param.rect.x = p_out_cur_win->x;
    video_out_param.rect.y = p_out_cur_win->y;
    video_out_param.rect.w = p_out_cur_win->w;
    video_out_param.rect.h = p_out_cur_win->h;
    video_out_param.layer = HD_LAYER1;

    ret =  hd_videoout_set(p_vdoout_test->out_path,HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &video_out_param);
    if(ret!=0){
        return ret;
    }
    ret = hd_videoout_start(p_vdoout_test->out_path);
    if(ret!=0){
        return ret;
    }
    memset((void *)&video_out_param,0,sizeof(HD_VIDEOOUT_WIN_ATTR));
    ret=hd_videoout_get(p_vdoout_test->out_path,HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &video_out_param);
    if(ret!=0){
        return ret;
    }
    DBG_IND("x:%d,y:%d,w:%d,h:%d \r\n", (int)(video_out_param.rect.x), (int)(video_out_param.rect.y), (int)(video_out_param.rect.w), (int)(video_out_param.rect.h));
    return ret;
}

/*
    OSD/VDO window is byte alignment
*/
UINT32 vout_dim(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    //buf is 352*288,device is 960*240,cannot scale down > 2
    HD_URECT cur_win[] = {{79,39,880,200},{120,57,840,180},{165,89,793,145},{3,5,759,234},{0,0,960,219}};
    HD_DIM cur_img[] = {{351,287},{123,99},{225,77},{228,111},{313,201}};
    HD_URECT *p_out_cur_win = cur_win;
    HD_DIM *p_out_cur_img = cur_img;
    int ret=0;
    char tmp_buf[64]={0};
    UINT32 i=0;

    ret=tttt_start(p_vdoout_test);
    if(ret!=0){
        return ret;
    }

    for(i=0;i<sizeof(cur_win)/sizeof(HD_URECT);i++) {
        tttt_dim(p_vdoout_test,p_out_cur_win);
        sprintf(tmp_buf,"win:{%d,%d,%d,%d},buf:{%d,%d}",(int)p_out_cur_win->x,(int)p_out_cur_win->y,(int)p_out_cur_win->w,(int)p_out_cur_win->h,(int)p_out_cur_img->w,(int)p_out_cur_img->h);
        tttt_waitkey(parm1,tmp_buf);
        p_out_cur_win++;
        p_out_cur_img++;
    }

    tttt_stop(p_vdoout_test);
    return 0;
}
UINT32 vout_fb_fmt(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    int ret=0;
    ret=tttt_start(p_vdoout_test);
    if(ret!=0){
        return ret;
    }
    ret=tttt_fb(p_vdoout_test);
    if(ret!=0) {
        goto ret_stop;
    }
    tttt_fill_pattern(p_vdoout_test);
    tttt_waitkey(parm1,vdoout_item[p_vdoout_test->item].name);

ret_stop:
    tttt_stop(p_vdoout_test);
    return ret;
}
/*
    x change differrent color key
*/
UINT32 vout_fb_color(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    int ret=0;
    ret=tttt_start(p_vdoout_test);
    if(ret!=0)
        return ret;
    p_vdoout_test->colorkey = 1;
    ret=tttt_fb(p_vdoout_test);
    if(ret!=0){
        return ret;
    }
    tttt_fill_pattern(p_vdoout_test);
    tttt_waitkey(parm1,vdoout_item[p_vdoout_test->item].name);
    tttt_stop(p_vdoout_test);
    return 0;
}
/*
    x change differrent blending 0~255 level
*/
UINT32 vout_fb_blend(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    int ret=0;
    ret=tttt_start(p_vdoout_test);
    if(ret!=0){
        return ret;
    }
    p_vdoout_test->blend = 1;
    ret=tttt_fb(p_vdoout_test);
    if(ret!=0){
        return ret;
    }
    tttt_fill_pattern(p_vdoout_test);
    tttt_waitkey(parm1,vdoout_item[p_vdoout_test->item].name);
    tttt_stop(p_vdoout_test);
    return 0;
}
/*
    OSD/VDO window is byte alignment,buf lineoffset is word alignment
*/
UINT32 vout_fb_dim(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    //test LCD is 960*240 ,UI buf usually is 320*240
    HD_URECT cur_win[] = {{5,19,640,200},{48,48,320,192},{7,15,659,213},{33,3,911,129}};
    HD_DIM cur_img[] = {{320,217},{124,99},{220,77},{888,111},{552,199}};
    HD_URECT *p_out_cur_win = cur_win;
    HD_DIM *p_out_cur_img = cur_img;
    int ret=0;
    char tmp_buf[64]={0};
    UINT32 i=0;

    // linux fb need to set dim before assigne buffer
    // if change buffer size,need to re-init fb for ARGB8565
    for(i=0;i<sizeof(cur_win)/sizeof(HD_URECT);i++) {
        ret=tttt_start(p_vdoout_test);
        if(ret!=0){
            return ret;
        }

        p_vdoout_test->fb_buf_dim.w = FB_BUF_W;
        p_vdoout_test->fb_buf_dim.h = FB_BUF_H;

        memcpy((void *)&p_vdoout_test->cur_win,(void *)p_out_cur_win,sizeof(HD_URECT));
        ret=tttt_fb(p_vdoout_test);
        if(ret!=0){
            return ret;
        }
        tttt_fill_pattern(p_vdoout_test);
        sprintf(tmp_buf,"win:{%d,%d,%d,%d},buf:{%d,%d}",(int)p_out_cur_win->x,(int)p_out_cur_win->y,(int)p_out_cur_win->w,(int)p_out_cur_win->h,(int)p_vdoout_test->fb_buf_dim.w,(int)p_vdoout_test->fb_buf_dim.h);
        tttt_waitkey(parm1,tmp_buf);
        tttt_stop(p_vdoout_test);
        p_out_cur_win++;
    }

    for(i=0;i<sizeof(cur_img)/sizeof(HD_DIM);i++) {
        ret=tttt_start(p_vdoout_test);
        if(ret!=0){
            return ret;
        }

        p_vdoout_test->fb_buf_dim.w = p_out_cur_img->w;
        p_vdoout_test->fb_buf_dim.h = p_out_cur_img->h;
        p_vdoout_test->cur_win.x = 0;
        p_vdoout_test->cur_win.y = 0;
        p_vdoout_test->cur_win.w = p_out_cur_img->w;
        p_vdoout_test->cur_win.h = p_out_cur_img->h;

        ret=tttt_fb(p_vdoout_test);
        if(ret!=0){
            return ret;
        }
        tttt_fill_pattern(p_vdoout_test);
        sprintf(tmp_buf,"win:{%d,%d,%d,%d},buf:{%d,%d}",(int)p_vdoout_test->cur_win.x,(int)p_vdoout_test->cur_win.y,(int)p_vdoout_test->cur_win.w,(int)p_vdoout_test->cur_win.h,(int)p_out_cur_img->w,(int)p_out_cur_img->h);
        tttt_waitkey(parm1,tmp_buf);
        tttt_stop(p_vdoout_test);
        p_out_cur_img++;

    }


           p_vdoout_test->fb_buf_dim.w = p_out_cur_img->w;
        p_vdoout_test->fb_buf_dim.h = p_out_cur_img->h;

    return 0;
}

UINT32 vout_all(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    VIDEOOUT_TEST vdoout_test_keep = {0};
    UINT32 i=0;
    UINT32 ret=0;
    UINT32 j=0;
    UINT32 fb_fmt_cnt=0;
    parm1 = 0; //run all,no wait key
    memcpy((void *)&vdoout_test_keep,(void *)p_vdoout_test,sizeof(VIDEOOUT_TEST));

    for(i=0;i<VIDEOOUT_ALL;i++) {
        p_vdoout_test->item = i;
        if(i>=VIDEOOUT_FB_FMT){
            fb_fmt_cnt= VOUT_FB_MAX;
        } else {
            fb_fmt_cnt= 1;
        }
        for(j=0;j<fb_fmt_cnt;j++){
            p_vdoout_test->osd_type = j;
            ret = vdoout_item[i].test_fp(p_vdoout_test,parm1,parm2,parm3);
            DBG_DUMP("######## run %d:%s ,ret %d ########\r\n", (int)(i), vdoout_item[i].name, (int)(ret));
            memcpy((void *)p_vdoout_test,(void *)&vdoout_test_keep,sizeof(VIDEOOUT_TEST));
        }

    }

    return 0;

}

/*
   1.not init,opne module
   2.not set mode
   3.no 0_IN_1
   4.520 not support 2nd dev
   5.520 not support HDMI
   6.520 not support TV
   7.open and open again
   8.set all _HD_VIDEOOUT_PARAM_ID , path NULL
   9.set all _HD_VIDEOOUT_PARAM_ID , p_param NULL
   10.get all _HD_VIDEOOUT_PARAM_ID , path -1
   11.get all _HD_VIDEOOUT_PARAM_ID , p_param NULL
   12.set all _HD_VIDEOOUT_PARAM_ID is ctrl_path ,but p_param is 0 array
   13.set all _HD_VIDEOOUT_PARAM_ID is out_path ,but p_param is 0 array
   14.set all _HD_VIDEOOUT_PARAM_ID is ctrl_path ,but p_param is 1 array
   15.get all _HD_VIDEOOUT_PARAM_ID is ctrl_path ,but p_param is 1 array
   16.set all _HD_VIDEOOUT_PARAM_ID is ctrl_path ,but p_param is 0xFF array
*/
UINT32 vout_basic_err(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    int ret=0;
    int i=0;
    //sizeof(HD_VIDEOOUT_SYSCAPS)=224
    char p_param[240]={0};

    //1.not init,opne module
    ret = open_module(p_vdoout_test,p_vdoout_test->out_type);
    if(ret==0){
        DBG_ERR("****not init fail,error but ret %d\r\n",ret);
    } else if(ret==HD_ERR_UNINIT) {
        DBG_DUMP("****not init ok,ret %d\r\n",ret);
    }


    //vdoout_test module init
    ret = init_module();
    if(ret != HD_OK) {
        DBG_ERR("init fail=%d\n", (int)(ret));
        return -1;
    }
    //2.not set mode
    ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0, &p_vdoout_test->out_path);
    if(ret==0){
        DBG_ERR("****vout open no mode fail,error but ret %d\r\n",ret);
    } else if(ret==HD_ERR_NO_CONFIG) {
        DBG_DUMP("****vout open not init ok,ret %d\r\n",ret);
    }

    //3.no 0_IN_1
    ret = hd_videoout_open(HD_VIDEOOUT_0_IN_1, HD_VIDEOOUT_0_OUT_0, &p_vdoout_test->out_path);
    if(ret==0){
        DBG_ERR("****vout 0_IN_1 fail,error but ret %d\r\n",ret);
    } else {
        DBG_DUMP("****vout 0_IN_1 ok,ret %d\r\n",ret);
    }

    //4.520 not support 2nd dev
    ret = hd_videoout_open(HD_VIDEOOUT_1_IN_0, HD_VIDEOOUT_1_OUT_0, &p_vdoout_test->out_path);
    if(ret==0){
        DBG_ERR("****vout 1_OUT_0 %d:fail,error but ret %d\r\n",i,ret);
    } else if(ret==HD_ERR_DEV) {
        DBG_DUMP("****vout 1_OUT_0 %d:ok,ret %d\r\n",i,ret);
    }

    //5.520 not support HDMI
    ret = open_module(p_vdoout_test,2);
    if(ret==0){
        DBG_ERR("****vout HDMI fail,error but ret %d\r\n",ret);
    } else if(ret==HD_ERR_NOT_SUPPORT) {
        DBG_DUMP("****vout HDMI ok,ret %d\r\n",ret);
    }

    //6.520 not support TV
    ret = open_module(p_vdoout_test,0);
    if(ret==0){
        DBG_ERR("****vout TV fail,error but ret %d\r\n",ret);
    } else if(ret==HD_ERR_NOT_SUPPORT) {
        DBG_DUMP("****vout TV ok,ret %d\r\n",ret);
    }

    //for init_module next
	ret = exit_module();
	if(ret != HD_OK) {
		DBG_ERR("exit fail=%d\n", (int)(ret));
	}

    ret=tttt_start(p_vdoout_test);
    if(ret!=0){
        return ret;
    }

    //7.open and open again
    ret = open_module(p_vdoout_test,1);
    if(ret==0){
        DBG_ERR("****vout open open,error but ret %d\r\n",ret);
    } else {
        DBG_DUMP("****vout open open ok,ret %d\r\n",ret);
    }

    //8.set all _HD_VIDEOOUT_PARAM_ID , path NULL
    for(i=HD_VIDEOOUT_PARAM_DEVCOUNT;i<HD_VIDEOOUT_PARAM_OUT_STAMP_BUF;i++){
	    ret = hd_videoout_set(0, i, 0);
        if(ret==0){
            DBG_ERR("****set parm%d pid 0:fail,error but ret %d\r\n",i,ret);
        } else {
            DBG_DUMP("****set parm%d pid 0:ok,ret %d\r\n",i,ret);
        }
    }
    //9.set all _HD_VIDEOOUT_PARAM_ID , p_param NULL
    for(i=HD_VIDEOOUT_PARAM_DEVCOUNT;i<HD_VIDEOOUT_PARAM_OUT_STAMP_BUF;i++){
	    ret = hd_videoout_set(p_vdoout_test->out_path, i, 0);
        if(ret==0){
            DBG_ERR("****set parm%d parm 0:fail,error but ret %d\r\n",i,ret);
        } else if(ret==HD_ERR_NULL_PTR){
            DBG_DUMP("****set parm%d parm 0:ok,ret %d\r\n",i,ret);
        }
    }
    //10.get all _HD_VIDEOOUT_PARAM_ID , path -1
    for(i=HD_VIDEOOUT_PARAM_DEVCOUNT;i<HD_VIDEOOUT_PARAM_OUT_STAMP_BUF;i++){
	    ret = hd_videoout_get(-1, i, 0);
        if(ret==0){
            DBG_ERR("****get parm%d pid -1:fail,error but ret %d\r\n",i,ret);
        } else {
            DBG_DUMP("****get parm%d pid -1:ok,ret %d\r\n",i,ret);
        }
    }
    //11.get all _HD_VIDEOOUT_PARAM_ID , p_param NULL
    for(i=HD_VIDEOOUT_PARAM_DEVCOUNT;i<HD_VIDEOOUT_PARAM_OUT_STAMP_BUF;i++){
	    ret = hd_videoout_get(p_vdoout_test->out_path, i, 0);
        if(ret==0){
            DBG_ERR("****get parm%d parm 0:fail,error but ret %d\r\n",i,ret);
        } else if(ret==HD_ERR_NULL_PTR){
            DBG_DUMP("****get parm%d parm 0:ok,ret %d\r\n",i,ret);
        }
    }
    //12.set all _HD_VIDEOOUT_PARAM_ID is ctrl_path ,but p_param is 0 array
    for(i=HD_VIDEOOUT_PARAM_DEVCOUNT;i<HD_VIDEOOUT_PARAM_OUT_STAMP_BUF;i++){
	    ret = hd_videoout_set(p_vdoout_test->out_ctrl, i, (void *)p_param);
        if((ret==0)&&(i!=HD_VIDEOOUT_PARAM_FB_ENABLE)){
            DBG_ERR("****set parm %d:fail,error but ret %d\r\n",i,ret);
        } else {
            DBG_DUMP("****set parm %d:ok,ret %d\r\n",i,ret);
        }
    }
    //13.set all _HD_VIDEOOUT_PARAM_ID is out_path ,but p_param is 0 array
    for(i=HD_VIDEOOUT_PARAM_DEVCOUNT;i<HD_VIDEOOUT_PARAM_OUT_STAMP_BUF;i++){
	    ret = hd_videoout_set(p_vdoout_test->out_path, i, (void *)p_param);
        if(ret==0){
            DBG_ERR("****set parm %d:fail,error but ret %d\r\n",i,ret);
        } else {
            DBG_DUMP("****set parm %d:ok,ret %d\r\n",i,ret);
        }
    }
    //14.set all _HD_VIDEOOUT_PARAM_ID is ctrl_path ,but p_param is 1 array
    memset((void *)p_param,1,sizeof(p_param));
    for(i=HD_VIDEOOUT_PARAM_DEVCOUNT;i<HD_VIDEOOUT_PARAM_OUT_STAMP_BUF;i++){
	    ret = hd_videoout_set(p_vdoout_test->out_ctrl, i, (void *)p_param);
        if(ret==0){
            DBG_ERR("****set parm %d:fail,error but ret %d\r\n",i,ret);
        } else {
            DBG_DUMP("****set parm %d:ok,ret %d\r\n",i,ret);
        }
    }
    //15.get all _HD_VIDEOOUT_PARAM_ID is ctrl_path ,but p_param is 1 array
    memset((void *)p_param,1,sizeof(p_param));
    for(i=HD_VIDEOOUT_PARAM_DEVCOUNT;i<HD_VIDEOOUT_PARAM_OUT_STAMP_BUF;i++){
	    ret = hd_videoout_get(p_vdoout_test->out_ctrl, i, (void *)p_param);
        if((ret==0)&&(i!=HD_VIDEOOUT_PARAM_DEVCOUNT)&&(i!=HD_VIDEOOUT_PARAM_SYSCAPS)&&(i!=HD_VIDEOOUT_PARAM_MODE)){
            DBG_ERR("****get parm %d:fail,error but ret %d\r\n",i,ret);
        } else {
            DBG_DUMP("****get parm %d:ok,ret %d\r\n",i,ret);
        }
    }
    //16.set all _HD_VIDEOOUT_PARAM_ID is ctrl_path ,but p_param is 0xFF array
    memset((void *)p_param,0xFF,sizeof(p_param));
    for(i=HD_VIDEOOUT_PARAM_DEVCOUNT;i<HD_VIDEOOUT_PARAM_OUT_STAMP_BUF;i++){
	    ret = hd_videoout_set(p_vdoout_test->out_ctrl, i, (void *)p_param);
        if(ret==0){
            DBG_ERR("****set parm %d:fail,error but ret %d\r\n",i,ret);
        } else {
            DBG_DUMP("****set parm %d:ok,ret %d\r\n",i,ret);
        }
    }

    tttt_waitkey(parm1,"basic err handle");
    tttt_stop(p_vdoout_test);
    return ret;
}
/*
    HD_VIDEOOUT_IN ,set not support fmt
*/
UINT32 vout_fmt_err(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    VIDEOOUT_TEST vdoout_test_keep = {0};
    int ret=0;
    UINT32 i=0;
    UINT32 err_fmt[]={HD_VIDEO_PXLFMT_I8,HD_VIDEO_PXLFMT_ARGB8888,HD_VIDEO_PXLFMT_ARGB4444,HD_VIDEO_PXLFMT_ARGB1555,HD_VIDEO_PXLFMT_ARGB8565};

    memcpy((void *)&vdoout_test_keep,(void *)p_vdoout_test,sizeof(VIDEOOUT_TEST));

    //set YUV420 pattern
	strncpy(p_vdoout_test->filename,VDO_420_PAT_PATH,MAX_FILE_NAME-1);

	// check TEST pattern exist
	if (check_test_pattern(p_vdoout_test->filename) == FALSE) {
		DBG_ERR("test_pattern isn't exist\r\n");
		return -1;
	}
    p_vdoout_test->out_dim.w = VDO_420_PAT_W;
    p_vdoout_test->out_dim.h = VDO_420_PAT_H;
    p_vdoout_test->pxlfmt = VDO_420_PAT_FMT;

    ret=tttt_start(p_vdoout_test);
    if(ret!=0){
        return ret;
    }
    ret = tttt_fmt(p_vdoout_test,p_vdoout_test->pxlfmt);
    if(ret!=0){
        return ret;
    }

    for(i=0;i<sizeof(err_fmt)/sizeof(UINT32);i++) {
        p_vdoout_test->pxlfmt = err_fmt[i];

        ret = tttt_fmt(p_vdoout_test,p_vdoout_test->pxlfmt);
        if(ret==0){
            DBG_ERR("****i %d:fail fmt %x not sup,but ret %d\r\n",i,p_vdoout_test->pxlfmt,ret);
        } else {
            DBG_DUMP("****i %d:ok fmt %x not sup ret %d\r\n",i,p_vdoout_test->pxlfmt,ret);
        }
    }

    p_vdoout_test->pxlfmt = HD_VIDEO_PXLFMT_YUV420;
    ret = tttt_fmt(p_vdoout_test,p_vdoout_test->pxlfmt);
    if(ret!=0){
        return ret;
    }

    tttt_waitkey(parm1,"fmt err handle");

    tttt_stop(p_vdoout_test);
    return ret;
}
/*
    HD_VIDEOOUT_IN ,set not support dir,should auto correct
    change direction rotate90 | rotate270 at the same time

*/
UINT32 vout_dir_err(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    int ret=0;
    UINT32 i=0;
    UINT32 err_dir[]={HD_VIDEO_DIR_MIRRORX|22,HD_VIDEO_DIR_ROTATE_90|55,HD_VIDEO_DIR_MIRRORY|HD_VIDEO_DIR_ROTATE_270|33,HD_VIDEO_DIR_ROTATE(45)};

    ret=tttt_start(p_vdoout_test);
    if(ret!=0){
        return ret;
    }
    for(i=0;i<sizeof(err_dir)/sizeof(UINT32);i++) {
        ret=tttt_dir(p_vdoout_test,err_dir[i]);

        if(ret==0){
            DBG_ERR("****i %d:fail dir %x not sup,but ret %d\r\n",i,err_dir[i],ret);
        } else {
            DBG_DUMP("****i %d:ok dir %x not sup,ret %d\r\n",i,(unsigned int)err_dir[i],ret);
        }
    }

    ret=tttt_dir(p_vdoout_test,HD_VIDEO_DIR_NONE);
    tttt_waitkey(parm1,"dir err handle");

    tttt_stop(p_vdoout_test);
    return ret;
}
/*
    HD_VIDEOOUT_WIN_ATTR ,set not support rect
    (layer and visible ,IPC not work,no test)
    HD_VIDEOOUT_IN ,set not support dim
    not align 4
*/
UINT32 vout_dim_err(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    //buf is 352*288,device is 960*240,cannot scale down > 2
    HD_URECT err_win[] = {{0,0,0,0},{0,0,960,60},{0,0,80,240},{-1,-1,-1,-1},{1080,0,960,240},{0,960,960,240},{0,0,-1,-1},{65535,65535,65535,65535}};
    HD_DIM err_img[] = {{1920,240},{960,480},{-1,-1},{65535,65535},{759,240}};
    HD_URECT *p_out_cur_win = err_win;
    HD_DIM *p_out_cur_img = err_img;
    UINT32 i=0;
    int ret=0;
    ret=tttt_start(p_vdoout_test);
    if(ret!=0){
        return ret;
    }

    for(i=0;i<sizeof(err_img)/sizeof(HD_DIM);i++) {

    	HD_VIDEOOUT_IN video_out_param={0};

    	video_out_param.dim.w = p_out_cur_img->w;
    	video_out_param.dim.h = p_out_cur_img->h;
    	video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV422;
    	video_out_param.dir = HD_VIDEO_DIR_NONE;
        //for push test
        p_vdoout_test->out_dim.w = video_out_param.dim.w;
        p_vdoout_test->out_dim.h = video_out_param.dim.h;

    	ret = hd_videoout_set(p_vdoout_test->out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
        if(ret==0)
            ret = hd_videoout_start(p_vdoout_test->out_path);

        if(ret==0){
            if(i==4) {
                DBG_DUMP("****i %d:ok w:%d,h:%d out, ret %d,push would fail\r\n",i,p_out_cur_img->w,p_out_cur_img->h,ret);
            } else {
                DBG_ERR("****i %d:fail w:%d,h:%d out,but ret %d\r\n",i,p_out_cur_img->w,p_out_cur_img->h,ret);
            }
        } else {
            DBG_DUMP("****i %d:ok w:%d,h:%d out, ret %d\r\n",i,p_out_cur_img->w,p_out_cur_img->h,ret);
        }
        p_out_cur_img++;
    }
    sleep(1);//for push lineoffset error

    //set correct image buffer,for next test
    {

    	HD_VIDEOOUT_IN video_out_param={0};

    	video_out_param.dim.w = p_vdoout_test->out_dim.w;
    	video_out_param.dim.h = p_vdoout_test->out_dim.h;
    	video_out_param.pxlfmt = p_vdoout_test->pxlfmt;
    	video_out_param.dir = HD_VIDEO_DIR_NONE;
        p_vdoout_test->out_dim.w = VDO_420_PAT_W;
        p_vdoout_test->out_dim.h = VDO_420_PAT_H;
    	ret = hd_videoout_set(p_vdoout_test->out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
        if(ret!=0){
            return ret;
        }
    }

    for(i=0;i<sizeof(err_win)/sizeof(HD_URECT);i++) {

        HD_VIDEOOUT_WIN_ATTR video_out_param ={0};

        video_out_param.visible = 1;
        video_out_param.rect.x = p_out_cur_win->x;
        video_out_param.rect.y = p_out_cur_win->y;
        video_out_param.rect.w = p_out_cur_win->w;
        video_out_param.rect.h = p_out_cur_win->h;
        video_out_param.layer = HD_LAYER1;

        ret = hd_videoout_set(p_vdoout_test->out_path,HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &video_out_param);
        if(ret==0)
            ret = hd_videoout_start(p_vdoout_test->out_path);

        if(ret==0){
            DBG_ERR("****i %d:fail x:%d,y:%d,w:%d,h:%d out,but ret %d\r\n",i,p_out_cur_win->x,p_out_cur_win->y,p_out_cur_win->w,p_out_cur_win->h,ret);
        } else {
            DBG_DUMP("****i %d:ok x:%d,y:%d,w:%d,h:%d out, ret %d\r\n",i,p_out_cur_win->x,p_out_cur_win->y,p_out_cur_win->w,p_out_cur_win->h,ret);
        }
        p_out_cur_win++;
    }


    tttt_waitkey(parm1,"vdo dim handle");

    tttt_stop(p_vdoout_test);
    return 0;
}
/*
    HD_FB_FMT set not support fb_fmt
    fb_id is test in basic_err
*/
UINT32 vout_fb_fmt_err(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    int ret=0;
    UINT32 i=0;
    UINT32 err_fmt[]={HD_VIDEO_PXLFMT_YUV420,HD_VIDEO_PXLFMT_YUV422,HD_VIDEO_PXLFMT_YUV444,0xFFFFFFFF};
    UINT32 err_lyr[]={HD_FB1,HD_FB2,HD_FB3};
    ret=tttt_start(p_vdoout_test);
    if(ret!=0){
        return ret;
    }

    for(i=0;i<sizeof(err_fmt)/sizeof(UINT32);i++) {
    	ret = vdoout_test_format(p_vdoout_test->out_ctrl,p_vdoout_test->fb_id,err_fmt[i]);
        if(ret==0){
            DBG_ERR("****i %d:fail fmt %x not sup,but ret %d\r\n",i,err_fmt[i],ret);
        } else {
            DBG_DUMP("****i %d:ok fmt %x not sup ret %d\r\n",i,err_fmt[i],ret);
        }
    }

    for(i=0;i<sizeof(err_lyr)/sizeof(UINT32);i++) {
    	ret = vdoout_test_format(p_vdoout_test->out_ctrl,err_lyr[i],HD_VIDEO_PXLFMT_ARGB8888);
        if(ret==0){
            DBG_ERR("****i %d:fail lyr %x not sup,but ret %d\r\n",i,err_lyr[i],ret);
        } else {
            DBG_DUMP("****i %d:ok lyr %x not sup ret %d\r\n",i,err_lyr[i],ret);
        }
    }

    p_vdoout_test->osd_type = 0;
    ret=tttt_fb(p_vdoout_test);
    if(ret!=0) {
        goto ret_stop;
    }
    tttt_waitkey(parm1,"fb fmt err handle");

ret_stop:
    tttt_stop(p_vdoout_test);
    return ret;
}

/*
      palette table size 0
      null palette table
      fb_id is test in basic_err
*/
UINT32 vout_fb_blend_err(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    int ret=0;
	HD_FB_PALETTE_TBL video_out_palette={0};

    p_vdoout_test->osd_type = VOUT_FB_INDEX8;
    ret=tttt_start(p_vdoout_test);
    if(ret!=0){
        return ret;
    }
    ret=tttt_fb(p_vdoout_test);
    if(ret!=0){
        return ret;
    }

    video_out_palette.fb_id = p_vdoout_test->fb_id;

	ret = hd_videoout_set(p_vdoout_test->out_ctrl, HD_VIDEOOUT_PARAM_FB_PALETTE_TABLE, &video_out_palette);
    if(ret==0){
        DBG_ERR("****palette size:fail not sup,but ret %d\r\n",ret);
    } else {
        DBG_DUMP("****palette size:ok not sup ret %d\r\n",ret);
    }

    video_out_palette.table_size = 300 ;
	ret = hd_videoout_set(p_vdoout_test->out_ctrl, HD_VIDEOOUT_PARAM_FB_PALETTE_TABLE, &video_out_palette);
    if(ret==0){
        DBG_ERR("****palette size:fail not sup,but ret %d\r\n",ret);
    } else {
        DBG_DUMP("****palette size:ok not sup ret %d\r\n",ret);
    }

    video_out_palette.table_size = 5 ;

	ret = hd_videoout_set(p_vdoout_test->out_ctrl, HD_VIDEOOUT_PARAM_FB_PALETTE_TABLE, &video_out_palette);
    if(ret==0){
        DBG_ERR("****palette addr:fail not sup,but ret %d\r\n",ret);
    } else {
        DBG_DUMP("****palette addr:ok not sup ret %d\r\n",ret);
    }

    tttt_waitkey(parm1,vdoout_item[p_vdoout_test->item].name);
    tttt_stop(p_vdoout_test);
    return 0;
}
/*
    HD_FB_DIM set not support input_dim
    fb_id is test in basic_err
	x HD_URECT output_rect;   ///< set fb out dim and x/y
	x not align 4
*/
UINT32 vout_fb_dim_err(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    //test LCD is 960*240 ,UI buf usually is 320*240
    HD_URECT err_win[] = {{0,0,FB_BUF_W/2-1,240},{5,5,960,FB_BUF_H/2-1},{12,12,0,228},{48,48,288,0},{920,0,920,240},{0,241,960,240},{0,0,-1,-1},{-1,-1,-1,-1},{33,3,960,129}};
    HD_DIM err_img[] = {{1920,240},{960,481},{0,240},{1,219},{-1,-1},{13,217}};
    HD_URECT *p_out_cur_win = err_win;
    HD_DIM *p_out_cur_img = err_img;
    int ret=0;
    UINT32 i=0;


    ret = tttt_start(p_vdoout_test);
    if(ret!=0){
        return ret;
    }

    ret=tttt_fb(p_vdoout_test);
    if(ret!=0){
        return ret;
    }
    for(i=0;i<sizeof(err_img)/sizeof(HD_DIM);i++) {
        HD_FB_DIM fb_dim_param ={0};

        fb_dim_param.fb_id = p_vdoout_test->fb_id;
        fb_dim_param.input_dim.w = p_out_cur_img->w;
        fb_dim_param.input_dim.h = p_out_cur_img->h;
        fb_dim_param.output_rect.x = 0;
        fb_dim_param.output_rect.y = 0;
        fb_dim_param.output_rect.w = p_vdoout_test->out_syscaps.output_dim.w;
        fb_dim_param.output_rect.h = p_vdoout_test->out_syscaps.output_dim.h;

        ret = hd_videoout_set(p_vdoout_test->out_ctrl,HD_VIDEOOUT_PARAM_FB_DIM, &fb_dim_param);
        if(ret==0){
            DBG_ERR("****i %d:fail w:%d,h:%d out,but ret %d\r\n",i,p_out_cur_img->w,p_out_cur_img->h,ret);
        } else {
            DBG_DUMP("****i %d:ok w:%d,h:%d out, ret %d\r\n",i,p_out_cur_img->w,p_out_cur_img->h,ret);
        }
        p_out_cur_img++;
    }

    p_vdoout_test->fb_buf_dim.w = FB_BUF_W;
    p_vdoout_test->fb_buf_dim.h = FB_BUF_H;


    // linux fb need to set dim before assigne buffer
    // if change buffer size,need to re-init fb for ARGB8565
    for(i=0;i<sizeof(err_win)/sizeof(HD_URECT);i++) {
        HD_FB_DIM fb_dim_param ={0};

        fb_dim_param.fb_id = p_vdoout_test->fb_id;
        fb_dim_param.input_dim.w = FB_BUF_W;
        fb_dim_param.input_dim.h = FB_BUF_H;

        memcpy((void *)&fb_dim_param.output_rect,(void *)p_out_cur_win,sizeof(HD_URECT));

        ret = hd_videoout_set(p_vdoout_test->out_ctrl,HD_VIDEOOUT_PARAM_FB_DIM, &fb_dim_param);
        if(ret==0){
            DBG_ERR("****i %d:fail x:%d,y:%d,w:%d,h:%d out,but ret %d\r\n",i,p_out_cur_win->x,p_out_cur_win->y,p_out_cur_win->w,p_out_cur_win->h,ret);
        } else {
            DBG_DUMP("****i %d:ok x:%d,y:%d,w:%d,h:%d out, ret %d\r\n",i,p_out_cur_win->x,p_out_cur_win->y,p_out_cur_win->w,p_out_cur_win->h,ret);
        }
        p_out_cur_win++;
    }

    tttt_waitkey(parm1,"fb dim err handle");
    tttt_stop(p_vdoout_test);

    return 0;
}
/*ex:
	o HD_VIDEOOUT_MODE,ex:open «e,¤£ set mode
	o HD_VIDEOOUT_PARAM_FB_PALETTE_TABLE UINT32  *p_table
	o HD_VIDEOOUT_PARAM_IN_WIN_ATTR,
	o HD_VIDEOOUT_PARAM_FB_ATTR  test fb_id in basic_err
	o HD_VIDEOOUT_PARAM_FB_ENABLE  test fb_id in basic_err
	x set alpha_1555,but fb fmt is not ARGB1555,can not check
	o open and open again,close and close again
	x not insert vout.ko,should in linux
    o open IDE2 in 520
    x push in param,ISF would check
*/
UINT32 vout_all_err(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    VIDEOOUT_TEST vdoout_test_keep = {0};
    UINT32 i=0;
    UINT32 ret=0;
    parm1 = 0; //run all,no wait key
    memcpy((void *)&vdoout_test_keep,(void *)p_vdoout_test,sizeof(VIDEOOUT_TEST));

    for(i=0;i<VIDEOOUT_ALL;i++) {
        p_vdoout_test->item = i;
        if(vdoout_item[i].err_hdl_fp){
            DBG_DUMP("######## begin err_hdl_fp %d:%s ##\r\n", (int)(i), vdoout_item[i].name);
            ret = vdoout_item[i].err_hdl_fp(p_vdoout_test,parm1,parm2,parm3);
            DBG_DUMP("######## end err_hdl_fp %d:%s ,ret %d ##\r\n", (int)(i), vdoout_item[i].name, (int)(ret));
            memcpy((void *)p_vdoout_test,(void *)&vdoout_test_keep,sizeof(VIDEOOUT_TEST));
        }
    }

    return 0;

}
void show_help(void)
{
    UINT32 i=0;
    #if (HDMI_SUP)
    DBG_DUMP("usage: vout_test [item] [cnt] [osd] [out_type] [hdmi_id]\r\n");
    #else
    DBG_DUMP("usage: vout_test [item] [cnt] [osd] [err]\r\n");
    #endif
    for(i=0;i<MAX_ITEM;i++)
        DBG_DUMP("%d:%s\n", (int)(i), vdoout_item[i].name);

    DBG_DUMP("osd type:\n");
    DBG_DUMP("0:ARGB8888\n");
    DBG_DUMP("1:ARGB4444\n");
    DBG_DUMP("2:ARGB1555\n");
    DBG_DUMP("3:ARGB8565\n");
    DBG_DUMP("4:INDEX8\n");

}

//parm1 is wait key or not
UINT32 testcase(VIDEOOUT_TEST *p_vdoout_test,UINT32 parm1,UINT32 parm2,UINT32 parm3)
{
    UINT32 item = p_vdoout_test->item;
    int ret=0;
    if(item<MAX_ITEM){
        DBG_DUMP("######## run %d:%s ,begin  ########\r\n", (int)(item), vdoout_item[item].name);
    } else {
        DBG_ERR("out range %d > MAX_ITEM:%d\r\n", (int)(item), (int)(MAX_ITEM));
        return -1;
    }

    if(parm3){
        if(vdoout_item[item].err_hdl_fp){
            ret= vdoout_item[item].err_hdl_fp(p_vdoout_test,parm1,parm2,parm3);
            DBG_DUMP("######## err_hdl_fp %d:%s ,ret %d ########\r\n", (int)(item), vdoout_item[item].name, (int)(ret));
            return ret;
        } else {
            DBG_ERR("not sup %d err test\n", (int)(item));
            return -1;
        }
    } else {
        if(vdoout_item[item].test_fp){
            ret= vdoout_item[item].test_fp(p_vdoout_test,parm1,parm2,parm3);
            DBG_DUMP("######## run %d:%s ,ret %d ########\r\n", (int)(item), vdoout_item[item].name, (int)(ret));
            return ret;
        } else {
            DBG_ERR("not sup %d\n", (int)(item));
            return -1;
        }
    }

}


