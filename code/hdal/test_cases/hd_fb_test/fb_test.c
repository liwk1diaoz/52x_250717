/**
	@brief Sample code of fb console.\n

	@file fb_console.c

	@author Janice Huang

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#if defined(__LINUX)

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_debug.h"
#include "vendor_videoout.h"

#include <sys/ioctl.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#define GETCHAR()				getchar()



#define DEBUG_MENU 		1

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////
#define CMAP_TEST  (1)

#define ARBG_8888_WHITE			0xFFFFFFFF
#define ARBG_8888_RED			0xFFFF0000
#define ARBG_8888_GREEN			0xFF00FF00
#define ARBG_8888_BLUE			0xFF0000FF
#define ARBG_8888_PINK	  		0xFFFF00FF

#define WAIT_TIME				(1.5*1000*1000)


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

#define VDO_SIZE_W		1920
#define VDO_SIZE_H		1080

#define FB_MAX_SIZE_W	1920
#define FB_MAX_SIZE_H	1080

#define FB_FORMAT       HD_VIDEO_PXLFMT_ARGB8888
#define FB_BLK_SIZE     VDO_YUV_BUFSIZE(FB_MAX_SIZE_W, FB_MAX_SIZE_H, FB_FORMAT)
///////////////////////////////////////////////////////////////////////////////


static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (IDE1 fb)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_DISP0_FB_POOL;
	mem_cfg.pool_info[0].blk_size = FB_BLK_SIZE;
	mem_cfg.pool_info[0].blk_cnt = 1;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
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

static HD_RESULT set_out_fb(HD_PATH_ID video_out_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_FB_FMT video_out_fmt={0};
	HD_FB_ATTR video_out_attr={0};

	video_out_fmt.fb_id = HD_FB0;
	video_out_fmt.fmt = FB_FORMAT;
	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_FMT, &video_out_fmt);
	if(ret!= HD_OK)
		return ret;

	video_out_attr.fb_id = HD_FB0;
	video_out_attr.alpha_blend = 0xFF;
	video_out_attr.alpha_1555 = 0xFF;
	video_out_attr.colorkey_en = 0;

	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_ATTR, &video_out_attr);
	if(ret!= HD_OK)
		return ret;

    return ret;
}

void Fill8888Color(UINT32 color,UINT32 uiBufAddr,UINT32 uiBufSize)
{
	UINT32 i=0;

	for(i=0;i<uiBufSize/4;i++)
	{
		*(UINT32 *)(uiBufAddr+4*i) = color;
	}
}


///////////////////////////////////////////////////////////////////////////////

typedef struct _VIDEO_LIVEVIEW {

	HD_DIM  out_max_dim;
	HD_DIM  out_dim;

	// (3)
	HD_VIDEOOUT_SYSCAPS out_syscaps;
	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;

    HD_VIDEOOUT_HDMI_ID hdmi_id;
} VIDEO_LIVEVIEW;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videoout_init()) != HD_OK)
		return ret;
	return HD_OK;
}
static HD_RESULT open_module(VIDEO_LIVEVIEW *p_stream,UINT32 out_type)
{
	HD_RESULT ret;

	// set videoout config
	ret = set_out_cfg(&p_stream->out_ctrl, out_type,p_stream->hdmi_id);
	if (ret != HD_OK) {
		printf("set out-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	if ((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0, &p_stream->out_path)) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT close_module(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoout_close(p_stream->out_path)) != HD_OK)
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
//////////////////////////////////

#define COLORS 256  	/* num of colors */

enum {
    BITS_PER_BYTE = 8,
	X_SCALE = 1,		/* horizontal scale */
};

/*
    Standard VGA colors
    http://en.wikipedia.org/wiki/ANSI_escape_code
*/
const uint32_t color_list[COLORS] = {
    /* system color: 16 */
    0xFF000000, 0xFFAA0000, 0xFF00AA00, 0xFFAA5500, 0xFF0000AA, 0xFFAA00AA, 0xFF00AAAA, 0xFFAAAAAA,
    0xFF555555, 0xFFFF5555, 0xFF55FF55, 0xFFFFFF55, 0xFF5555FF, 0xFFFF55FF, 0xFF55FFFF, 0xFFFFFFFF,
#if (COLORS ==256)
    /* color cube: 216 */
    0xFF000000, 0xFF00005F, 0xFF000087, 0xFF0000AF, 0xFF0000D7, 0xFF0000FF, 0xFF005F00, 0xFF005F5F,
    0xFF005F87, 0xFF005FAF, 0xFF005FD7, 0xFF005FFF, 0xFF008700, 0xFF00875F, 0xFF008787, 0xFF0087AF,
    0xFF0087D7, 0xFF0087FF, 0xFF00AF00, 0xFF00AF5F, 0xFF00AF87, 0xFF00AFAF, 0xFF00AFD7, 0xFF00AFFF,
    0xFF00D700, 0xFF00D75F, 0xFF00D787, 0xFF00D7AF, 0xFF00D7D7, 0xFF00D7FF, 0xFF00FF00, 0xFF00FF5F,
    0xFF00FF87, 0xFF00FFAF, 0xFF00FFD7, 0xFF00FFFF, 0xFF5F0000, 0xFF5F005F, 0xFF5F0087, 0xFF5F00AF,
    0xFF5F00D7, 0xFF5F00FF, 0xFF5F5F00, 0xFF5F5F5F, 0xFF5F5F87, 0xFF5F5FAF, 0xFF5F5FD7, 0xFF5F5FFF,
    0xFF5F8700, 0xFF5F875F, 0xFF5F8787, 0xFF5F87AF, 0xFF5F87D7, 0xFF5F87FF, 0xFF5FAF00, 0xFF5FAF5F,
    0xFF5FAF87, 0xFF5FAFAF, 0xFF5FAFD7, 0xFF5FAFFF, 0xFF5FD700, 0xFF5FD75F, 0xFF5FD787, 0xFF5FD7AF,
    0xFF5FD7D7, 0xFF5FD7FF, 0xFF5FFF00, 0xFF5FFF5F, 0xFF5FFF87, 0xFF5FFFAF, 0xFF5FFFD7, 0xFF5FFFFF,
    0xFF870000, 0xFF87005F, 0xFF870087, 0xFF8700AF, 0xFF8700D7, 0xFF8700FF, 0xFF875F00, 0xFF875F5F,
    0xFF875F87, 0xFF875FAF, 0xFF875FD7, 0xFF875FFF, 0xFF878700, 0xFF87875F, 0xFF878787, 0xFF8787AF,
    0xFF8787D7, 0xFF8787FF, 0xFF87AF00, 0xFF87AF5F, 0xFF87AF87, 0xFF87AFAF, 0xFF87AFD7, 0xFF87AFFF,
    0xFF87D700, 0xFF87D75F, 0xFF87D787, 0xFF87D7AF, 0xFF87D7D7, 0xFF87D7FF, 0xFF87FF00, 0xFF87FF5F,
    0xFF87FF87, 0xFF87FFAF, 0xFF87FFD7, 0xFF87FFFF, 0xFFAF0000, 0xFFAF005F, 0xFFAF0087, 0xFFAF00AF,
    0xFFAF00D7, 0xFFAF00FF, 0xFFAF5F00, 0xFFAF5F5F, 0xFFAF5F87, 0xFFAF5FAF, 0xFFAF5FD7, 0xFFAF5FFF,
    0xFFAF8700, 0xFFAF875F, 0xFFAF8787, 0xFFAF87AF, 0xFFAF87D7, 0xFFAF87FF, 0xFFAFAF00, 0xFFAFAF5F,
    0xFFAFAF87, 0xFFAFAFAF, 0xFFAFAFD7, 0xFFAFAFFF, 0xFFAFD700, 0xFFAFD75F, 0xFFAFD787, 0xFFAFD7AF,
    0xFFAFD7D7, 0xFFAFD7FF, 0xFFAFFF00, 0xFFAFFF5F, 0xFFAFFF87, 0xFFAFFFAF, 0xFFAFFFD7, 0xFFAFFFFF,
    0xFFD70000, 0xFFD7005F, 0xFFD70087, 0xFFD700AF, 0xFFD700D7, 0xFFD700FF, 0xFFD75F00, 0xFFD75F5F,
    0xFFD75F87, 0xFFD75FAF, 0xFFD75FD7, 0xFFD75FFF, 0xFFD78700, 0xFFD7875F, 0xFFD78787, 0xFFD787AF,
    0xFFD787D7, 0xFFD787FF, 0xFFD7AF00, 0xFFD7AF5F, 0xFFD7AF87, 0xFFD7AFAF, 0xFFD7AFD7, 0xFFD7AFFF,
    0xFFD7D700, 0xFFD7D75F, 0xFFD7D787, 0xFFD7D7AF, 0xFFD7D7D7, 0xFFD7D7FF, 0xFFD7FF00, 0xFFD7FF5F,
    0xFFD7FF87, 0xFFD7FFAF, 0xFFD7FFD7, 0xFFD7FFFF, 0xFFFF0000, 0xFFFF005F, 0xFFFF0087, 0xFFFF00AF,
    0xFFFF00D7, 0xFFFF00FF, 0xFFFF5F00, 0xFFFF5F5F, 0xFFFF5F87, 0xFFFF5FAF, 0xFFFF5FD7, 0xFFFF5FFF,
    0xFFFF8700, 0xFFFF875F, 0xFFFF8787, 0xFFFF87AF, 0xFFFF87D7, 0xFFFF87FF, 0xFFFFAF00, 0xFFFFAF5F,
    0xFFFFAF87, 0xFFFFAFAF, 0xFFFFAFD7, 0xFFFFAFFF, 0xFFFFD700, 0xFFFFD75F, 0xFFFFD787, 0xFFFFD7AF,
    0xFFFFD7D7, 0xFFFFD7FF, 0xFFFFFF00, 0xFFFFFF5F, 0xFFFFFF87, 0xFFFFFFAF, 0xFFFFFFD7, 0xFFFFFFFF,
    /* gray scale: 24 */
    0xFF080808, 0xFF121212, 0xFF1C1C1C, 0xFF262626, 0xFF303030, 0xFF3A3A3A, 0xFF444444, 0xFF4E4E4E,
    0xFF585858, 0xFF626262, 0xFF6C6C6C, 0xFF767676, 0xFF808080, 0xFF8A8A8A, 0xFF949494, 0xFF9E9E9E,
    0xFFA8A8A8, 0xFFB2B2B2, 0xFFBCBCBC, 0xFFC6C6C6, 0xFFD0D0D0, 0xFFDADADA, 0xFFE4E4E4, 0xFFEEEEEE,
#endif
};

const uint32_t bit_mask[] = {
    0x00,
    0x01, 0x03, 0x07, 0x0F,
    0x1F, 0x3F, 0x7F, 0xFF,
    0x1FF, 0x3FF, 0x7FF, 0xFFF,
    0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
    0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF,
    0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF,
    0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF, 0xFFFFFFF,
    0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};

const char *fb_path = "/dev/fb0";
struct pair { int x, y; };
struct color_t { uint32_t a, r, g, b; };

struct framebuffer {
    char *fp;               /* pointer of framebuffer(read only) */
    int fd;                 /* file descriptor of framebuffer */
    struct pair res;        /* resolution (x, y) */
    long screen_size;       /* screen data size (byte) */
    int line_length;        /* line length (byte) */
    int bpp;                /* BYTES per pixel */
    uint32_t color_palette[COLORS];
    struct fb_cmap *cmap, *cmap_org;
};

struct fb_cmap *cmap_create(struct fb_var_screeninfo *vinfo)
{
	struct fb_cmap *cmap;

	if ((cmap = (struct fb_cmap *) calloc(1, sizeof(struct fb_cmap))) == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}
	cmap->start = 0;
	cmap->len = COLORS;
	if ((cmap->red = (uint16_t *) calloc(1, sizeof(uint16_t) * COLORS)) == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}
	if ((cmap->green = (uint16_t *) calloc(1, sizeof(uint16_t) * COLORS)) == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}
	if ((cmap->blue = (uint16_t *) calloc(1, sizeof(uint16_t) * COLORS)) == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}
	cmap->transp = NULL;
	if ((cmap->transp = (uint16_t *) calloc(1, sizeof(uint16_t) * COLORS)) == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	return cmap;
}

void cmap_die(struct fb_cmap *cmap)
{
	if (cmap) {
		free(cmap->red);
		free(cmap->green);
		free(cmap->blue);
		free(cmap->transp);
		free(cmap);
	}
}

void get_rgb(int i, struct color_t *color)
{
	color->a = (color_list[i] >> 24) & bit_mask[8];
	color->r = (color_list[i] >> 16) & bit_mask[8];
	color->g = (color_list[i] >> 8) & bit_mask[8];
	color->b = (color_list[i] >> 0) & bit_mask[8];
}

uint32_t bit_reverse(uint32_t val, int bits)
{
	uint32_t ret = val;
	int shift = bits - 1;

	for (val >>= 1; val; val >>= 1) {
		ret <<= 1;
		ret |= val & 1;
		shift--;
	}

	return ret <<= shift;
}
int cmap_init(struct framebuffer *fb, struct fb_var_screeninfo *vinfo)
{
	int i;
	uint16_t r, g, b,a;
	struct color_t color;

	if (ioctl(fb->fd, FBIOGETCMAP, fb->cmap_org) < 0) { /* not fatal */
		cmap_die(fb->cmap_org);
		fb->cmap_org = NULL;
	}

	for (i = 0; i < COLORS; i++) {
		get_rgb(i, &color);
		r = (color.r << BITS_PER_BYTE) | color.r;
		g = (color.g << BITS_PER_BYTE) | color.g;
		b = (color.b << BITS_PER_BYTE) | color.b;
		a = (color.a << BITS_PER_BYTE) | color.a;

		*(fb->cmap->red + i) = (vinfo->red.msb_right) ?
			bit_reverse(r, 16) & bit_mask[16]: r;
		*(fb->cmap->green + i) = (vinfo->green.msb_right) ?
			bit_reverse(g, 16) & bit_mask[16]: g;
		*(fb->cmap->blue + i) = (vinfo->blue.msb_right) ?
			bit_reverse(b, 16) & bit_mask[16]: b;
		*(fb->cmap->transp + i) = (vinfo->transp.msb_right) ?
			bit_reverse(b, 16) & bit_mask[16]: a;

        //printf("%x %x %x %x \r\n",*(fb->cmap->transp + i),*(fb->cmap->red + i),*(fb->cmap->green + i),*(fb->cmap->blue + i));
	}

	if (ioctl(fb->fd, FBIOPUTCMAP, fb->cmap) < 0) {
		printf( "ioctl: FBIOPUTCMAP failed\n");
		return -1;
	}

    return 0;
}

uint32_t get_color(struct fb_var_screeninfo *vinfo, int i)
{
	uint32_t a,r, g, b;
	struct color_t color;

	if (vinfo->bits_per_pixel == 8)
		return i;

	get_rgb(i, &color);
	a = color.a >> (BITS_PER_BYTE - vinfo->transp.length);
	r = color.r >> (BITS_PER_BYTE - vinfo->red.length);
	g = color.g >> (BITS_PER_BYTE - vinfo->green.length);
	b = color.b >> (BITS_PER_BYTE - vinfo->blue.length);

	if (vinfo->transp.msb_right)
		r = bit_reverse(r, vinfo->transp.length) & bit_mask[vinfo->transp.length];
	if (vinfo->red.msb_right)
		r = bit_reverse(r, vinfo->red.length) & bit_mask[vinfo->red.length];
	if (vinfo->green.msb_right)
		g = bit_reverse(g, vinfo->green.length) & bit_mask[vinfo->green.length];
	if (vinfo->blue.msb_right)
		b = bit_reverse(b, vinfo->blue.length) & bit_mask[vinfo->blue.length];

	return (a << vinfo->transp.offset)
        + (r << vinfo->red.offset)
		+ (g << vinfo->green.offset)
		+ (b << vinfo->blue.offset);
}

int fb_init_for_cmap(struct framebuffer *fb)
{
	int i;
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;

    if ((fb->fd = open(fb_path, O_RDWR)) < 0) {
    	printf( "cannot open \"%s\"\n", fb_path);
    	perror("open");
    	return -1;
    }

	if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
		printf( "ioctl: FBIOGET_FSCREENINFO failed\n");
    	return -1;
	}

	if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
		printf( "ioctl: FBIOGET_VSCREENINFO failed\n");
    	return -1;
	}

	fb->res.x = vinfo.xres;
	fb->res.y = vinfo.yres;
	fb->screen_size = finfo.smem_len;
	fb->line_length = finfo.line_length;

    printf("finfo.visual  %d bits_per_pixel %d \r\n",finfo.visual,vinfo.bits_per_pixel);
    finfo.visual = FB_VISUAL_TRUECOLOR;
	if ((finfo.visual == FB_VISUAL_TRUECOLOR || finfo.visual == FB_VISUAL_DIRECTCOLOR)
		&& (vinfo.bits_per_pixel == 15 || vinfo.bits_per_pixel == 16
		|| vinfo.bits_per_pixel == 24 || vinfo.bits_per_pixel == 32)) {
		fb->cmap = fb->cmap_org = NULL;
		fb->bpp = (vinfo.bits_per_pixel + BITS_PER_BYTE - 1) / BITS_PER_BYTE;

		fb->cmap = cmap_create(&vinfo);
		fb->cmap_org = cmap_create(&vinfo);
		cmap_init(fb, &vinfo);
	}
	else if (finfo.visual == FB_VISUAL_PSEUDOCOLOR
		&& vinfo.bits_per_pixel == 8) {
		fb->cmap = cmap_create(&vinfo);
		fb->cmap_org = cmap_create(&vinfo);
		cmap_init(fb, &vinfo);
		fb->bpp = 1;
	}
	else {
		/* non packed pixel, mono color, grayscale: not implimented */
		printf( "unsupported framebuffer type\n");
    	return -1;
	}

	for (i = 0; i < COLORS; i++) { /* init color palette */
		fb->color_palette[i] = get_color(&vinfo, i);
        printf("color_palette[%d]:0x%x  \r\n",i,fb->color_palette[i]);

	}

	if ((fb->fp = (char *) mmap(0, fb->screen_size,
		PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0)) == MAP_FAILED) {
		perror("mmap");
    	return -1;
	}

    printf("### fp:%x,screen_size:%x   ###\r\n",fb->fp,fb->screen_size);

    return 0;
}

void fb_die(struct framebuffer *fb)
{
	cmap_die(fb->cmap);
	if (fb->cmap_org) {
		ioctl(fb->fd, FBIOPUTCMAP, fb->cmap_org); /* not fatal */
		cmap_die(fb->cmap_org);
	}
	//free(fb->buf);
	if ((munmap(fb->fp, fb->screen_size)) < 0) {
		perror("munmap");
		exit(EXIT_FAILURE);
	}
	if (close(fb->fd) < 0) {
		perror("close");
		exit(EXIT_FAILURE);
	}
}

void fb_putpixel(struct framebuffer *fb, int x, int y, int color_index)
{
	int i;
#if 1
    if(x==0) {
	    printf("color[%d]: 0x%.6X\n", color_index, fb->color_palette[color_index]);
    }
#endif
	for (i = 0; i < X_SCALE; i++)
		memcpy(fb->fp + (x * X_SCALE + i) * fb->bpp + (y * fb->line_length), &fb->color_palette[color_index], fb->bpp);
}

//////////////////////////////////////////////////////
MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	VIDEO_LIVEVIEW stream[1] = {0}; //0: main stream
	UINT32 out_type = 1;
	UINT32 screenW=0,screenH=0;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32 blk_size = FB_BLK_SIZE;
	UINT32 pa_fb0 = 0;
	HD_COMMON_MEM_VB_BLK blk =0;
    VENDOR_FB_INIT fb_init;
    VENDOR_FB_UNINIT fb_uninit;
	HD_FB_ENABLE video_out_enable={0};
    struct framebuffer fb = {0};

	// query program options
	if (argc >= 2) {
		out_type = atoi(argv[1]);
		printf("out_type %d\r\n", out_type);
		if(out_type > 2) {
			printf("error: not support out_type!\r\n");
			return 0;
		}
	}
    stream[0].hdmi_id=HD_VIDEOOUT_HDMI_1920X1080I60;//default
	// query program options
	if (argc >= 3 && (atoi(argv[2]) !=0)) {
		stream[0].hdmi_id = atoi(argv[2]);
		printf("hdmi_mode %d\r\n", stream[0].hdmi_id);
	}

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
	}

	// init memory
	ret = mem_init();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		goto exit;
	}

	// init all modules
	ret = init_module();
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}

	ret = open_module(&stream[0], out_type);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	// get videoout capability
	ret = get_out_caps(stream[0].out_ctrl, &stream[0].out_syscaps);
	if (ret != HD_OK) {
		printf("get out-caps fail=%d\n", ret);
		goto exit;
	}
	stream[0].out_max_dim = stream[0].out_syscaps.output_dim;

	// set videoout parameter (main)
	stream[0].out_dim.w = stream[0].out_max_dim.w; //using device max dim.w
	stream[0].out_dim.h = stream[0].out_max_dim.h; //using device max dim.h
	ret = set_out_param(stream[0].out_path, &stream[0].out_dim);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}

    // set videoout fb config
	ret = set_out_fb(stream[0].out_ctrl);
	if (ret != HD_OK) {
		printf("set fb fail=%d\n", ret);
		goto exit;
	}

	//--- Get DISP0_FB memory ---
	blk = hd_common_mem_get_block(HD_COMMON_MEM_DISP0_FB_POOL, blk_size, ddr_id); // Get block from mem pool
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get block fail (0x%x).. try again later.....\r\n", blk);
		goto exit;
	}

	pa_fb0 = hd_common_mem_blk2pa(blk); // Get physical addr
	if (pa_fb0 == 0) {
		printf("blk2pa fail, blk = 0x%x\r\n", blk);
		goto exit;
	}


    //assign bf buffer to FB_INIT
    fb_init.fb_id = HD_FB0;
    fb_init.pa_addr = pa_fb0;
    fb_init.buf_len = blk_size;
	ret = vendor_videoout_set(VENDOR_VIDEOOUT_ID0, VENDOR_VIDEOOUT_ITEM_FB_INIT, &fb_init);
	if(ret!= HD_OK)
		return ret;

    //enable fb after HD_VIDEOOUT_PARAM_FB_INIT which would clear buffer,avoid garbage on screen
	video_out_enable.fb_id = HD_FB0;
	video_out_enable.enable = 1;

	ret = hd_videoout_set(stream[0].out_ctrl, HD_VIDEOOUT_PARAM_FB_ENABLE, &video_out_enable);
	if(ret!= HD_OK)
		return ret;

	//Write fb buffer
	screenW = stream[0].out_syscaps.output_dim.w;
	screenH = stream[0].out_syscaps.output_dim.h;

    //init fb and set cmap for fb draw library
    usleep(5000);//delay for dev/fb0 created
    fb_init_for_cmap(&fb);

    printf("fb.screen_size %x %d %d fb.fp %x \r\n",fb.screen_size,screenW,screenH,(UINT32)fb.fp);

    //fill fb
	Fill8888Color(ARBG_8888_RED,(UINT32)fb.fp,screenW*screenH*4);
	hd_common_mem_flush_cache((void *)fb.fp, screenW*screenH*4);
	usleep(WAIT_TIME);
	Fill8888Color(ARBG_8888_GREEN,(UINT32)fb.fp,screenW*screenH*4);
	hd_common_mem_flush_cache((void *)fb.fp, screenW*screenH*4);
	usleep(WAIT_TIME);

	Fill8888Color(ARBG_8888_WHITE,(UINT32)fb.fp,screenW*screenH*4);
	Fill8888Color(ARBG_8888_RED,(UINT32)(fb.fp+screenW*screenH),screenW*screenH);
	Fill8888Color(ARBG_8888_GREEN,(UINT32)(fb.fp+screenW*screenH*2),screenW*screenH);
	Fill8888Color(ARBG_8888_BLUE,(UINT32)(fb.fp+screenW*screenH*3),screenW*screenH);
	hd_common_mem_flush_cache((void *)fb.fp, screenW*screenH*4);
	usleep(WAIT_TIME);

	Fill8888Color(0,(UINT32)fb.fp,screenW*screenH*4);
	hd_common_mem_flush_cache((void *)fb.fp, screenW*screenH*4);

	// query user key
	printf("Enter q to exit\n");
	while (1) {
		key = GETCHAR();
		if (key == 'q' || key == 0x3) {
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
	}

exit:
	//--- Release memory ---
	if(blk)
		hd_common_mem_release_block(blk);

	// close video_liveview modules (main)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

	// uninit all modules
	ret = exit_module();
	if (ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}

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

	if(fb.fp!=0)
		munmap(fb.fp,fb.screen_size);

	if(fb.fp>=0)
		close((int)fb.fp);

    fb_uninit.fb_id = HD_FB0;
	ret = vendor_videoout_set(VENDOR_VIDEOOUT_ID0, VENDOR_VIDEOOUT_ITEM_FB_UNINIT, &fb_uninit);
	if(ret!= HD_OK)
		printf("FB_DEINIT fail=%d\n", ret);

    return 0;
}

#endif
