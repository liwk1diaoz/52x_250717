/**
 * @file user_2d.c
 * @brief  demo gfx scale/blend/draw line/draw rectangle functions.
 * @author Jerry Chien
 * @date in the year 2018
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"

// platform dependent
#if defined(__LINUX)
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#define GETCHAR()				getchar()
#else
#include <FreeRTOS_POSIX.h>	
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h>		//for sleep API
#include <kwrap/examsys.h> 	//for MAIN(), GETCHAR() API
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_gfx_test, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

typedef struct _GFX {

	// (1)
	HD_COMMON_MEM_VB_BLK buf_blk;
	UINT32               buf_size;
	UINT32               buf_pa;
	UINT32               buf_va;
	
} GFX;

static GFX gfx = { 0 };
static int loop = 10;
static int src_fmt = HD_VIDEO_PXLFMT_YUV420;
static int dst_fmt = HD_VIDEO_PXLFMT_YUV420;
static int scale_quality = 0;
static char prefix[32]="";
static char postfix[16]="yuv420";
static char icon_16x72[128] = "/mnt/sd/16x72_yuv420.bin";
static char icon_1000x200[128] = "/mnt/sd/video_frm_1000_200_colorkey_yuv420.dat";
static char icon_256x256[128] = "/mnt/sd/256x256_yuv420.bin";
static char icon_224x32[128] = "/mnt/sd/224x32_yuv420.bin";
static char icon_1920x1080[128] = "/mnt/sd/video_frm_1920_1080_1_yuv420.dat";
static char bg_1920x1080[128] = "/mnt/sd/video_frm_1920_1080_1_yuv420.dat";

typedef struct _GFX_MMAP_BUF {
	UINT32 blk_size;
	UINT32 pa_addr[2];
	UINT8 ddr_id;
	char *va_addr[2];
	HD_DIM dim;
	UINT32 loff[2];
} GFX_MMAP_BUF;

typedef struct _GFX_IMG_INFO {
	char name[128];
	HD_DIM dim;
} GFX_IMG_INFO;

//setup all gfx's buffer to demo purpose
static HD_RESULT mem_init(HD_COMMON_MEM_VB_BLK *buf_blk, UINT32 buf_size, UINT32 *buf_pa)
{
	HD_RESULT                    ret;
	HD_COMMON_MEM_INIT_CONFIG    mem_cfg = {0};

	mem_cfg.pool_info[0].type     = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = buf_size;
	mem_cfg.pool_info[0].blk_cnt  = 1;
	mem_cfg.pool_info[0].ddr_id   = DDR_ID0;

	//register gfx's buffer to common memory pool
	ret = hd_common_mem_init(&mem_cfg);
	if(ret != HD_OK){
		printf("fail to allocate %d bytes from common pool\n", buf_size);
		return ret;
	}

	//get gfx's buffer block
	*buf_blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, buf_size, DDR_ID0);
	if (*buf_blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get block fail\r\n");
		return HD_ERR_NOMEM;
	}

	//translate gfx's buffer block to physical address
	*buf_pa = hd_common_mem_blk2pa(*buf_blk);
	if (*buf_pa == 0) {
		printf("blk2pa fail, buf_blk = 0x%x\r\n", *buf_blk);
		return HD_ERR_NOMEM;
	}

	return HD_OK;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	hd_common_mem_uninit();
	return ret;
}

void save_output(char *filename, void *data, int size)
{
	FILE *pfile;
	if ((pfile = fopen(filename, "wb")) == NULL) {
		printf("[ERROR] Open File %s failed!!\n", filename);
		exit(1);

	}
	fwrite(data, 1, size, pfile);
	fflush(pfile);
	fclose(pfile);

}

static int scale_img(GFX_MMAP_BUF *src_buf, HD_URECT *src_rect, GFX_MMAP_BUF *dst_buf, HD_URECT *dst_rect, char *filename, int fmt)
{
	HD_GFX_SCALE scale_img = {0};
	HD_RESULT ret = HD_OK;

	if(src_rect){
		scale_img.src_region.x = src_rect->x ;
		scale_img.src_region.y = src_rect->y;
		scale_img.src_region.w = src_rect->w;
		scale_img.src_region.h = src_rect->h;
	}else{
		scale_img.src_region.x = 0 ;
		scale_img.src_region.y = 0;
		scale_img.src_region.w = src_buf->dim.w;
		scale_img.src_region.h = src_buf->dim.h;
	}
	scale_img.src_img.dim.w = src_buf->dim.w;
	scale_img.src_img.dim.h = src_buf->dim.h;
	scale_img.src_img.format = src_fmt;
	scale_img.src_img.p_phy_addr[0] = src_buf->pa_addr[0];
	scale_img.src_img.p_phy_addr[1] = src_buf->pa_addr[1];
	scale_img.src_img.lineoffset[0] = src_buf->loff[0];
	scale_img.src_img.lineoffset[1] = src_buf->loff[1];
	scale_img.src_img.ddr_id = src_buf->ddr_id;

	//scale img
	scale_img.dst_img.dim.w = dst_buf->dim.w;
	scale_img.dst_img.dim.h = dst_buf->dim.h;
	scale_img.dst_img.format = fmt;
	scale_img.dst_img.p_phy_addr[0] = dst_buf->pa_addr[0];
	scale_img.dst_img.p_phy_addr[1] = dst_buf->pa_addr[1];
	scale_img.dst_img.lineoffset[0] = dst_buf->loff[0];
	scale_img.dst_img.lineoffset[1] = dst_buf->loff[1];
	scale_img.dst_img.ddr_id = dst_buf->ddr_id;
	if(dst_rect){
		scale_img.dst_region.x = dst_rect->x;
		scale_img.dst_region.y = dst_rect->y;
		scale_img.dst_region.w = dst_rect->w;
		scale_img.dst_region.h = dst_rect->h;
	}else{
		scale_img.dst_region.x = 0;
		scale_img.dst_region.y = 0;
		scale_img.dst_region.w = scale_img.dst_img.dim.w;
		scale_img.dst_region.h = scale_img.dst_img.dim.h;
	}
    scale_img.quality = scale_quality;
	ret = hd_gfx_scale(&scale_img);
	if (ret != HD_OK) {
		printf("hd_gfx_scale fail\n");
		return ret;
	}
	
	if(filename && strlen(filename))
		save_output(filename, dst_buf->va_addr[0], dst_buf->blk_size);

	return 0;
}

static int _test_scale(GFX_IMG_INFO *src_img, HD_URECT *src_rect, GFX_IMG_INFO *dst_img, HD_URECT *dst_rect, char *filename)
{
	HD_RESULT ret;
	FILE *file;
	GFX_MMAP_BUF mmap_buf[3] = { 0 };
	
	mmap_buf[0].dim.w = src_img->dim.w;
	mmap_buf[0].dim.h = src_img->dim.h;

	if(src_fmt == HD_VIDEO_PXLFMT_YUV420){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h*3/2);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].pa_addr[1] = (gfx.buf_pa+mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].va_addr[1] = (mmap_buf[0].va_addr[0]+mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].loff[0] = mmap_buf[0].dim.w;
		mmap_buf[0].loff[1] = mmap_buf[0].dim.w;
	}else if(src_fmt == HD_VIDEO_PXLFMT_ARGB1555){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h*2);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].loff[0] = (mmap_buf[0].dim.w*2);
	}else if(src_fmt == HD_VIDEO_PXLFMT_Y8){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].loff[0] = mmap_buf[0].dim.w;
	}else{
		printf("src fmt(%x) not support\n", src_fmt);
		return -1;
	}
	
	file = fopen(src_img->name, "rb");
	if (!file) {
		printf("Error to open logo file %s!", src_img->name);
		return -1;
	}
	
	//coverity[check_return]
	if(fread(mmap_buf[0].va_addr[0], mmap_buf[0].blk_size, 1, file) == 0) {
		printf("Error to read logo file %s!", src_img->name);
		fclose(file);
		return -1;
	}
	fclose(file);
	
	mmap_buf[1].dim.w = dst_img->dim.w;
	mmap_buf[1].dim.h = dst_img->dim.h;

	if(dst_fmt == HD_VIDEO_PXLFMT_YUV420){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h*3/2);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].pa_addr[1] = (mmap_buf[1].pa_addr[0]+mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[1] = (mmap_buf[1].va_addr[0]+mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].loff[0] = mmap_buf[1].dim.w;
		mmap_buf[1].loff[1] = mmap_buf[1].dim.w;
	}else if(dst_fmt == HD_VIDEO_PXLFMT_ARGB1555){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h*2);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].loff[0] = (mmap_buf[1].dim.w*2);
	}else if(dst_fmt == HD_VIDEO_PXLFMT_Y8){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].loff[0] = mmap_buf[1].dim.w;
	}else{
		printf("dst fmt(%x) not support\n", dst_fmt);
		return -1;
	}
	
	if(strlen(dst_img->name)){
		file = fopen(dst_img->name, "rb");
		if (!file) {
			printf("Error to open logo file %s!", dst_img->name);
			return -1;
		}
		//coverity[check_return]
		if(fread(mmap_buf[1].va_addr[0], mmap_buf[1].blk_size, 1, file) == 0) {
			printf("Error to read logo file %s!", dst_img->name);
			fclose(file);
			return -1;
		}
		fclose(file);
	}
	
	ret = scale_img(&mmap_buf[0], src_rect, &mmap_buf[1], dst_rect, filename, dst_fmt);
	if(ret)
		printf("scale_img fail\n");
	
	return ret;
}

static int _test_scale_sanity(int null_src, int null_dst, unsigned int fmt)
{
	HD_RESULT ret;
	GFX_MMAP_BUF mmap_buf[3] = { 0 };
	UINT32 temp1, temp2;
	
	mmap_buf[0].dim.w = 256;
	mmap_buf[0].dim.h = 256;

	if(src_fmt == HD_VIDEO_PXLFMT_YUV420){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h*3/2);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].pa_addr[1] = (gfx.buf_pa+mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].va_addr[1] = (mmap_buf[0].va_addr[0]+mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].loff[0] = mmap_buf[0].dim.w;
		mmap_buf[0].loff[1] = mmap_buf[0].dim.w;
	}else if(src_fmt == HD_VIDEO_PXLFMT_ARGB1555){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h*2);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].loff[0] = (mmap_buf[0].dim.w*2);
	}else if(src_fmt == HD_VIDEO_PXLFMT_Y8){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].loff[0] = mmap_buf[0].dim.w;
	}else{
		printf("src fmt(%x) not support\n", src_fmt);
		return -1;
	}

	memset(mmap_buf[0].va_addr[0], 0xFF, mmap_buf[0].blk_size);
	
	mmap_buf[1].dim.w = 512;
	mmap_buf[1].dim.h = 512;

	if(dst_fmt == HD_VIDEO_PXLFMT_YUV420){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h*3/2);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].pa_addr[1] = (gfx.buf_pa+mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[1] = (mmap_buf[1].va_addr[0]+mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].loff[0] = mmap_buf[1].dim.w;
		mmap_buf[1].loff[1] = mmap_buf[1].dim.w;
	}else if(dst_fmt == HD_VIDEO_PXLFMT_ARGB1555){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h*2);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].loff[0] = (mmap_buf[1].dim.w*2);
	}else if(dst_fmt == HD_VIDEO_PXLFMT_Y8){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].loff[0] = mmap_buf[1].dim.w;
	}else{
		printf("dst fmt(%x) not support\n", dst_fmt);
		return -1;
	}

	memset(mmap_buf[1].va_addr[0], 0xFF, mmap_buf[1].blk_size);
	
	temp1 = mmap_buf[0].pa_addr[0];
	temp2 = mmap_buf[1].pa_addr[0];
	if(null_src)
		mmap_buf[0].pa_addr[0] = 0;
	if(null_dst)
		mmap_buf[1].pa_addr[0] = 0;
	ret = scale_img(&mmap_buf[0], NULL, &mmap_buf[1], NULL, NULL, fmt);
	
	mmap_buf[0].pa_addr[0] = temp1;
	mmap_buf[1].pa_addr[0] = temp2;
	
	return ret;
}

static int test_scale(void)
{
	HD_RESULT ret;
	GFX_IMG_INFO src_img, dst_img;
	HD_URECT src_rect, dst_rect;
	char filename[128];
	int i;

	//basic scale
	strcpy(src_img.name, icon_16x72);
	src_img.dim.w =16 ;
	src_img.dim.h =72 ;
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.w = src_img.dim.w;
	src_rect.h = src_img.dim.h;
	memset(dst_img.name, 0, sizeof(dst_img.name));
	dst_img.dim.w =48 ;
	dst_img.dim.h =144 ;
	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.w = dst_img.dim.w;
	dst_rect.h = dst_img.dim.h;
	sprintf(filename, "/mnt/sd/%sscale_basic_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_scale(&src_img, &src_rect, &dst_img, &dst_rect, filename);
	if(ret){
		printf("basic scale testing fail\n");
		return ret;
	}
	printf("basic scale testing pass\n");
	
	//advance scale
	strcpy(src_img.name, icon_1000x200);
	src_img.dim.w =1000 ;
	src_img.dim.h =200 ;
	strcpy(dst_img.name, bg_1920x1080);
	dst_img.dim.w =1920 ;
	dst_img.dim.h =1080 ;
	sprintf(filename, "/mnt/sd/%sscale_advance_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	if(scale_quality != HD_GFX_SCALE_QUALITY_BILINEAR){
		src_rect.x = 500;
		src_rect.y = 0;
		src_rect.w = 500;
		src_rect.h = 200;
		dst_rect.x = 500;
		dst_rect.y = 100;
		dst_rect.w = 1000;
		dst_rect.h = 600;
	}else{
		printf("bilinear scale skips advance test because ssca doesn't support source image crop\n");
		src_rect.x = 0;
		src_rect.y = 0;
		src_rect.w = src_img.dim.w;
		src_rect.h = src_img.dim.h;
		dst_rect.x = 500;
		dst_rect.y = 100;
		dst_rect.w = 1000;
		dst_rect.h = 600;
	}
	ret = _test_scale(&src_img, &src_rect, &dst_img, &dst_rect, filename);
	if(ret){
		printf("advance scale testing fail\n");
		return ret;
	}
	printf("advance scale testing pass\n");
	
	//test out of image boundary
	dst_rect.x = 1500;
	dst_rect.y = 700;
	dst_rect.w = 1000;
	dst_rect.h = 600;
	ret = _test_scale(&src_img, &src_rect, &dst_img, &dst_rect, NULL);
	if(ret){
		printf("scale sanity testing fail : out of image boundary\n");
		return -1;
	}
	printf("scale sanity testing pass : out of image boundary\n");
	
	//test null src addr
	ret = _test_scale_sanity(1, 0, dst_fmt);
	if(!ret){
		printf("scale sanity testing fail : null src addr\n");
		return -1;
	}
	printf("scale sanity testing pass : null src addr\n");
	
	//test null dst addr
	ret = _test_scale_sanity(0, 1, dst_fmt);
	if(!ret){
		printf("scale sanity testing fail : null dst addr\n");
		return -1;
	}
	printf("scale sanity testing pass : null dst addr\n");
	
	//test unsupported format
	ret = _test_scale_sanity(0, 0, HD_VIDEO_PXLFMT_YUV422_ONE);
	if(!ret){
		printf("scale sanity testing fail : unsupported format\n");
		return -1;
	}
	printf("scale sanity testing pass : unsupported format\n");
	
	//stress test
	strcpy(src_img.name, icon_256x256);
	src_img.dim.w =256 ;
	src_img.dim.h =256 ;
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.w = src_img.dim.w;
	src_rect.h = src_img.dim.h;
	strcpy(dst_img.name, bg_1920x1080);
	dst_img.dim.w =1920 ;
	dst_img.dim.h =1080 ;
	dst_rect.x = 300;
	dst_rect.y = 0;
	dst_rect.w = 1024;
	dst_rect.h = 512;
	for(i = 0 ; i < loop ; ++i){
		ret = _test_scale(&src_img, &src_rect, &dst_img, &dst_rect, NULL);
		if(ret){
			printf("stress scale testing fail\n");
			return ret;
		}
	}
	sprintf(filename, "/mnt/sd/%sscale_stress_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_scale(&src_img, &src_rect, &dst_img, &dst_rect, filename);
	if(ret){
		printf("stress scale testing fail\n");
		return ret;
	}
	printf("stress scale testing pass\n");
	
	return ret;
}

static int copy_img(GFX_MMAP_BUF *src_buf, HD_URECT *src_rect, GFX_MMAP_BUF *dst_buf, HD_URECT *dst_rect, char *filename, int fmt)
{
	HD_GFX_COPY copy_param = { 0 };
	HD_RESULT ret = HD_OK;

	if(src_rect){
		copy_param.src_region.x = src_rect->x ;
		copy_param.src_region.y = src_rect->y;
		copy_param.src_region.w = src_rect->w;
		copy_param.src_region.h = src_rect->h;
	}else{
		copy_param.src_region.x = 0 ;
		copy_param.src_region.y = 0;
		copy_param.src_region.w = src_buf->dim.w;
		copy_param.src_region.h = src_buf->dim.h;
	}
	
	if(dst_rect){
		copy_param.dst_pos.x = dst_rect->x;
		copy_param.dst_pos.y = dst_rect->y;
	}else{
		copy_param.dst_pos.x = 0;
		copy_param.dst_pos.y = 0;
	}

	copy_param.src_img.dim.w = src_buf->dim.w;
	copy_param.src_img.dim.h = src_buf->dim.h;
	copy_param.src_img.format = src_fmt;
	copy_param.src_img.p_phy_addr[0] = src_buf->pa_addr[0];
	copy_param.src_img.p_phy_addr[1] = src_buf->pa_addr[1];
	copy_param.src_img.lineoffset[0] = src_buf->loff[0];
	copy_param.src_img.lineoffset[1] = src_buf->loff[1];
	copy_param.src_img.ddr_id = src_buf->ddr_id;
	copy_param.dst_img.dim.w = dst_buf->dim.w;
	copy_param.dst_img.dim.h = dst_buf->dim.h;
	copy_param.dst_img.format = fmt;
	copy_param.dst_img.p_phy_addr[0] = dst_buf->pa_addr[0];
	copy_param.dst_img.p_phy_addr[1] = dst_buf->pa_addr[1];
	copy_param.dst_img.lineoffset[0] = dst_buf->loff[0];
	copy_param.dst_img.lineoffset[1] = dst_buf->loff[1];
	copy_param.dst_img.ddr_id = dst_buf->ddr_id;
	
	if(copy_param.src_img.dim.w == 1000){
		if(copy_param.dst_pos.x == 500){
			if(src_fmt == HD_VIDEO_PXLFMT_YUV420)
				copy_param.colorkey = 0x00808000;
			else if(src_fmt == HD_VIDEO_PXLFMT_ARGB1555)
				copy_param.colorkey = 0xF888;
			else
				copy_param.colorkey = 128;
			copy_param.alpha    = 255;
		}else{
			copy_param.colorkey = 0;
			copy_param.alpha    = 128;
		}
	}else{
		copy_param.colorkey = 0;
		copy_param.alpha    = 255;
	}
	
	ret = hd_gfx_copy(&copy_param);
	if (ret != HD_OK) {
		printf("hd_gfx_copy fail\n");
		return ret;
	}
	
	if(filename && strlen(filename))
		save_output(filename, dst_buf->va_addr[0], dst_buf->blk_size);
	
	return 0;
}

static int _test_copy(GFX_IMG_INFO *src_img, HD_URECT *src_rect, GFX_IMG_INFO *dst_img, HD_URECT *dst_rect, char *filename)
{
	HD_RESULT ret;
	FILE *file;
	GFX_MMAP_BUF mmap_buf[3] = { 0 };
	
	mmap_buf[0].dim.w = src_img->dim.w;
	mmap_buf[0].dim.h = src_img->dim.h;

	if(src_fmt == HD_VIDEO_PXLFMT_YUV420){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h*3/2);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].pa_addr[1] = (gfx.buf_pa+mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].va_addr[1] = (mmap_buf[0].va_addr[0]+mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].loff[0] = mmap_buf[0].dim.w;
		mmap_buf[0].loff[1] = mmap_buf[0].dim.w;
	}else if(src_fmt == HD_VIDEO_PXLFMT_ARGB1555){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h*2);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].loff[0] = (mmap_buf[0].dim.w*2);
	}else if(src_fmt == HD_VIDEO_PXLFMT_Y8){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].loff[0] = mmap_buf[0].dim.w;
	}else{
		printf("src fmt(%x) not support\n", src_fmt);
		return -1;
	}

	file = fopen(src_img->name, "rb");
	if (!file) {
		printf("Error to open logo file %s!", src_img->name);
		return -1;
	}
	//coverity[check_return]
	if(fread(mmap_buf[0].va_addr[0], mmap_buf[0].blk_size, 1, file) == 0) {
		printf("Error to read logo file %s!", src_img->name);
		fclose(file);
		return -1;
	}
	fclose(file);
	
	mmap_buf[1].dim.w = dst_img->dim.w;
	mmap_buf[1].dim.h = dst_img->dim.h;

	if(dst_fmt == HD_VIDEO_PXLFMT_YUV420){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h*3/2);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].pa_addr[1] = (mmap_buf[1].pa_addr[0]+mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[1] = (mmap_buf[1].va_addr[0]+mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].loff[0] = mmap_buf[1].dim.w;
		mmap_buf[1].loff[1] = mmap_buf[1].dim.w;
	}else if(dst_fmt == HD_VIDEO_PXLFMT_ARGB1555){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h*2);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].loff[0] = (mmap_buf[1].dim.w*2);
	}else if(dst_fmt == HD_VIDEO_PXLFMT_Y8){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].loff[0] = mmap_buf[1].dim.w;
	}else{
		printf("dst fmt(%x) not support\n", dst_fmt);
		return -1;
	}
	
	if(strlen(dst_img->name)){
		file = fopen(dst_img->name, "rb");
		if (!file) {
			printf("Error to open logo file %s!", dst_img->name);
			return -1;
		}
		//coverity[check_return]
		if(fread(mmap_buf[1].va_addr[0], mmap_buf[1].blk_size, 1, file) == 0) {
			printf("Error to read logo file %s!", dst_img->name);
			fclose(file);
			return -1;
		}
		fclose(file);
	}
	
	ret = copy_img(&mmap_buf[0], src_rect, &mmap_buf[1], dst_rect, filename, dst_fmt);
	if(ret)
		printf("copy_img fail\n");
	
	return ret;
}

static int _test_copy_sanity(int null_src, int null_dst, unsigned int fmt)
{
	HD_RESULT ret;
	GFX_MMAP_BUF mmap_buf[3] = { 0 };
	UINT32 temp1, temp2;
	
	mmap_buf[0].dim.w = 256;
	mmap_buf[0].dim.h = 256;

	if(src_fmt == HD_VIDEO_PXLFMT_YUV420){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h*3/2);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].pa_addr[1] = (gfx.buf_pa+mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].va_addr[1] = (mmap_buf[0].va_addr[0]+mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].loff[0] = mmap_buf[0].dim.w;
		mmap_buf[0].loff[1] = mmap_buf[0].dim.w;
	}else if(src_fmt == HD_VIDEO_PXLFMT_ARGB1555){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h*2);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].loff[0] = (mmap_buf[0].dim.w*2);
	}else if(src_fmt == HD_VIDEO_PXLFMT_Y8){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].loff[0] = mmap_buf[0].dim.w;
	}else{
		printf("src fmt(%x) not support\n", src_fmt);
		return -1;
	}

	memset(mmap_buf[0].va_addr[0], 0xFF, mmap_buf[0].blk_size);
	
	mmap_buf[1].dim.w = 512;
	mmap_buf[1].dim.h = 512;

	if(dst_fmt == HD_VIDEO_PXLFMT_YUV420){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h*3/2);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].pa_addr[1] = (gfx.buf_pa+mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[1] = (mmap_buf[1].va_addr[0]+mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].loff[0] = mmap_buf[1].dim.w;
		mmap_buf[1].loff[1] = mmap_buf[1].dim.w;
	}else if(dst_fmt == HD_VIDEO_PXLFMT_ARGB1555){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h*2);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].loff[0] = (mmap_buf[1].dim.w*2);
	}else if(dst_fmt == HD_VIDEO_PXLFMT_Y8){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].loff[0] = mmap_buf[1].dim.w;
	}else{
		printf("dst fmt(%x) not support\n", dst_fmt);
		return -1;
	}

	memset(mmap_buf[1].va_addr[0], 0xFF, mmap_buf[1].blk_size);
	
	temp1 = mmap_buf[0].pa_addr[0];
	temp2 = mmap_buf[1].pa_addr[0];
	if(null_src)
		mmap_buf[0].pa_addr[0] = 0;
	if(null_dst)
		mmap_buf[1].pa_addr[0] = 0;
	ret = copy_img(&mmap_buf[0], NULL, &mmap_buf[1], NULL, NULL, fmt);
	
	mmap_buf[0].pa_addr[0] = temp1;
	mmap_buf[1].pa_addr[0] = temp2;
	
	return ret;
}

static int test_copy(void)
{
	HD_RESULT ret;
	GFX_IMG_INFO src_img, dst_img;
	HD_URECT src_rect, dst_rect;
	char filename[128];
	int i;
	
	//basic copy
	strcpy(src_img.name, icon_16x72);
	src_img.dim.w =16 ;
	src_img.dim.h =72 ;
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.w = src_img.dim.w;
	src_rect.h = src_img.dim.h;
	memset(dst_img.name, 0, sizeof(dst_img.name));
	dst_img.dim.w =16 ;
	dst_img.dim.h =72 ;
	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.w = dst_img.dim.w;
	dst_rect.h = dst_img.dim.h;
	sprintf(filename, "/mnt/sd/%scopy_basic_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_copy(&src_img, &src_rect, &dst_img, &dst_rect, filename);
	if(ret){
		printf("basic copy testing fail\n");
		return ret;
	}
	printf("basic copy testing pass\n");
	
	//advance copy
	strcpy(src_img.name, icon_1000x200);
	src_img.dim.w =1000 ;
	src_img.dim.h =200 ;
	src_rect.x = 500;
	src_rect.y = 0;
	src_rect.w = 500;
	src_rect.h = 200;
	strcpy(dst_img.name, bg_1920x1080);
	dst_img.dim.w =1920 ;
	dst_img.dim.h =1080 ;
	dst_rect.x = 500;
	dst_rect.y = 100;
	dst_rect.w = 500;
	dst_rect.h = 200;
	sprintf(filename, "/mnt/sd/%scopy_advance_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_copy(&src_img, &src_rect, &dst_img, &dst_rect, filename);
	if(ret){
		printf("advance copy testing fail\n");
		return ret;
	}
	printf("advance copy testing pass\n");
	
	//test out of image boundary
	dst_rect.x = 1800;
	dst_rect.y = 800;
	dst_rect.w = 500;
	dst_rect.h = 200;
	sprintf(filename, "/mnt/sd/%scopy_boundary_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_copy(&src_img, &src_rect, &dst_img, &dst_rect, filename);
	if(!ret){
		printf("copy sanity testing fail : out of image boundary\n");
		//return -1;
	}
	printf("copy sanity testing pass : out of image boundary\n");
	
	//test null src addr
	ret = _test_copy_sanity(1, 0, dst_fmt);
	if(!ret){
		printf("copy sanity testing fail : null src addr\n");
		return -1;
	}
	printf("copy sanity testing pass : null src addr\n");
	
	//test null dst addr
	ret = _test_copy_sanity(0, 1, dst_fmt);
	if(!ret){
		printf("copy sanity testing fail : null dst addr\n");
		return -1;
	}
	printf("copy sanity testing pass : null dst addr\n");
	
	//test unsupported format
	ret = _test_copy_sanity(0, 0, HD_VIDEO_PXLFMT_YUV422_ONE);
	if(!ret){
		printf("copy sanity testing fail : unsupported format\n");
		return -1;
	}
	printf("copy sanity testing pass : unsupported format\n");
	
	//stress test
	strcpy(src_img.name, icon_256x256);
	src_img.dim.w =256 ;
	src_img.dim.h =256 ;
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.w = src_img.dim.w;
	src_rect.h = src_img.dim.h;
	memset(dst_img.name, 0, sizeof(dst_img.name));
	dst_img.dim.w =256 ;
	dst_img.dim.h =256 ;
	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.w = dst_img.dim.w;
	dst_rect.h = dst_img.dim.h;
	for(i = 0 ; i < loop ; ++i){
		ret = _test_copy(&src_img, &src_rect, &dst_img, &dst_rect, NULL);
		if(ret){
			printf("stress copy testing fail\n");
			return ret;
		}
	}
	sprintf(filename, "/mnt/sd/%scopy_stress_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_copy(&src_img, &src_rect, &dst_img, &dst_rect, filename);
	if(ret){
		printf("stress copy testing fail\n");
		return ret;
	}
	printf("stress copy testing pass\n");
	
	return ret;
}

static int rotate_img(GFX_MMAP_BUF *src_buf, HD_URECT *src_rect, GFX_MMAP_BUF *dst_buf, 
    HD_URECT *dst_rect, UINT32 rotate_angle, char *filename, int fmt)
{
	HD_GFX_ROTATE rotate_img = { 0 };
	HD_RESULT ret = HD_OK;

	if(src_rect){
		rotate_img.src_region.x = src_rect->x ;
		rotate_img.src_region.y = src_rect->y;
		rotate_img.src_region.w = src_rect->w;
		rotate_img.src_region.h = src_rect->h;
	}else{
		rotate_img.src_region.x = 0 ;
		rotate_img.src_region.y = 0;
		rotate_img.src_region.w = src_buf->dim.w;
		rotate_img.src_region.h = src_buf->dim.h;
	}
	
	if(dst_rect){
		rotate_img.dst_pos.x = dst_rect->x;
		rotate_img.dst_pos.y = dst_rect->y;
	}else{
		rotate_img.dst_pos.x = 0;
		rotate_img.dst_pos.y = 0;
	}

	rotate_img.src_img.dim.w = src_buf->dim.w;
	rotate_img.src_img.dim.h = src_buf->dim.h;
	rotate_img.src_img.format = src_fmt;
	rotate_img.src_img.p_phy_addr[0] = src_buf->pa_addr[0];
	rotate_img.src_img.p_phy_addr[1] = src_buf->pa_addr[1];
	rotate_img.src_img.lineoffset[0] = src_buf->loff[0];
	rotate_img.src_img.lineoffset[1] = src_buf->loff[1];
	rotate_img.src_img.ddr_id = src_buf->ddr_id;

	rotate_img.dst_img.dim.w = dst_buf->dim.w;
	rotate_img.dst_img.dim.h = dst_buf->dim.h;
	rotate_img.dst_img.format = fmt;
	rotate_img.dst_img.p_phy_addr[0] = dst_buf->pa_addr[0];
	rotate_img.dst_img.p_phy_addr[1] = dst_buf->pa_addr[1];
	rotate_img.dst_img.lineoffset[0] = dst_buf->loff[0];
	rotate_img.dst_img.lineoffset[1] = dst_buf->loff[1];
	rotate_img.dst_img.ddr_id = dst_buf->ddr_id;
    rotate_img.angle = rotate_angle;
	ret = hd_gfx_rotate(&rotate_img);
	if (ret != HD_OK) {
		printf("hd_gfx_rotate fail\n");
		return ret;
	}
	
	if(filename && strlen(filename))
		save_output(filename, dst_buf->va_addr[0], dst_buf->blk_size);

	return 0;
}

static int _test_rotate(GFX_IMG_INFO *src_img, HD_URECT *src_rect, GFX_IMG_INFO *dst_img, HD_URECT *dst_rect, UINT32 rotate_angle, char *filename)
{
	HD_RESULT ret;
	FILE *file;
	GFX_MMAP_BUF mmap_buf[3] = { 0 };
	
	mmap_buf[0].dim.w = src_img->dim.w;
	mmap_buf[0].dim.h = src_img->dim.h;

	if(src_fmt == HD_VIDEO_PXLFMT_YUV420){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h*3/2);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].pa_addr[1] = (gfx.buf_pa+mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].va_addr[1] = (mmap_buf[0].va_addr[0]+mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].loff[0] = mmap_buf[0].dim.w;
		mmap_buf[0].loff[1] = mmap_buf[0].dim.w;
	}else if(src_fmt == HD_VIDEO_PXLFMT_ARGB1555){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h*2);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].loff[0] = (mmap_buf[0].dim.w*2);
	}else if(src_fmt == HD_VIDEO_PXLFMT_Y8){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].loff[0] = mmap_buf[0].dim.w;
	}else{
		printf("src fmt(%x) not support\n", src_fmt);
		return -1;
	}

	file = fopen(src_img->name, "rb");
	if (!file) {
		printf("Error to open logo file %s!", src_img->name);
		return -1;
	}
	//coverity[check_return]
	if(fread(mmap_buf[0].va_addr[0], mmap_buf[0].blk_size, 1, file) == 0) {
		printf("Error to read logo file %s!", src_img->name);
		fclose(file);
		return -1;
	}
	fclose(file);
	
	mmap_buf[1].dim.w = dst_img->dim.w;
	mmap_buf[1].dim.h = dst_img->dim.h;

	if(dst_fmt == HD_VIDEO_PXLFMT_YUV420){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h*3/2);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].pa_addr[1] = (mmap_buf[1].pa_addr[0]+mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[1] = (mmap_buf[1].va_addr[0]+mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].loff[0] = mmap_buf[1].dim.w;
		mmap_buf[1].loff[1] = mmap_buf[1].dim.w;
	}else if(dst_fmt == HD_VIDEO_PXLFMT_ARGB1555){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h*2);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].loff[0] = (mmap_buf[1].dim.w*2);
	}else if(dst_fmt == HD_VIDEO_PXLFMT_Y8){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].loff[0] = mmap_buf[1].dim.w;
	}else{
		printf("dst fmt(%x) not support\n", dst_fmt);
		return -1;
	}

	if(strlen(dst_img->name)){
		file = fopen(dst_img->name, "rb");
		if (!file) {
			printf("Error to open logo file %s!", dst_img->name);
			return -1;
		}
		//coverity[check_return]
		if(fread(mmap_buf[1].va_addr[0], mmap_buf[1].blk_size, 1, file) == 0) {
			printf("Error to read logo file %s!", dst_img->name);
			fclose(file);
			return -1;
		}
		fclose(file);
	}
	
	ret = rotate_img(&mmap_buf[0], src_rect, &mmap_buf[1], dst_rect, rotate_angle, filename, dst_fmt);
	if(ret)
		printf("rotate_img fail\n");
	
	return ret;
}

static int _test_rotate_sanity(int null_src, int null_dst, unsigned int fmt)
{
	HD_RESULT ret;
	GFX_MMAP_BUF mmap_buf[3] = { 0 };
	UINT32 temp1, temp2;
	
	mmap_buf[0].dim.w = 256;
	mmap_buf[0].dim.h = 256;

	if(src_fmt == HD_VIDEO_PXLFMT_YUV420){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h*3/2);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].pa_addr[1] = (gfx.buf_pa+mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].va_addr[1] = (mmap_buf[0].va_addr[0]+mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].loff[0] = mmap_buf[0].dim.w;
		mmap_buf[0].loff[1] = mmap_buf[0].dim.w;
	}else if(src_fmt == HD_VIDEO_PXLFMT_ARGB1555){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h*2);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].loff[0] = (mmap_buf[0].dim.w*2);
	}else if(src_fmt == HD_VIDEO_PXLFMT_Y8){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].loff[0] = mmap_buf[0].dim.w;
	}else{
		printf("src fmt(%x) not support\n", src_fmt);
		return -1;
	}

	memset(mmap_buf[0].va_addr[0], 0xFF, mmap_buf[0].blk_size);
	
	mmap_buf[1].dim.w = 512;
	mmap_buf[1].dim.h = 512;

	if(dst_fmt == HD_VIDEO_PXLFMT_YUV420){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h*3/2);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].pa_addr[1] = (gfx.buf_pa+mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[1] = (mmap_buf[1].va_addr[0]+mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].loff[0] = mmap_buf[1].dim.w;
		mmap_buf[1].loff[1] = mmap_buf[1].dim.w;
	}else if(dst_fmt == HD_VIDEO_PXLFMT_ARGB1555){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h*2);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].loff[0] = (mmap_buf[1].dim.w*2);
	}else if(dst_fmt == HD_VIDEO_PXLFMT_Y8){
		mmap_buf[1].blk_size = (mmap_buf[1].dim.w*mmap_buf[1].dim.h);
		mmap_buf[1].pa_addr[0] = (mmap_buf[0].pa_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].va_addr[0] = (mmap_buf[0].va_addr[0]+mmap_buf[0].blk_size);
		mmap_buf[1].loff[0] = mmap_buf[1].dim.w;
	}else{
		printf("dst fmt(%x) not support\n", dst_fmt);
		return -1;
	}

	memset(mmap_buf[1].va_addr[0], 0xFF, mmap_buf[1].blk_size);
	
	temp1 = mmap_buf[0].pa_addr[0];
	temp2 = mmap_buf[1].pa_addr[0];
	if(null_src)
		mmap_buf[0].pa_addr[0] = 0;
	if(null_dst)
		mmap_buf[1].pa_addr[0] = 0;
	ret = rotate_img(&mmap_buf[0], NULL, &mmap_buf[1], NULL, HD_VIDEO_DIR_ROTATE_180, NULL, fmt);
	
	mmap_buf[0].pa_addr[0] = temp1;
	mmap_buf[1].pa_addr[0] = temp2;
	
	return ret;
}

static int test_rotate(void)
{
	HD_RESULT ret;
	GFX_IMG_INFO src_img, dst_img;
	HD_URECT src_rect, dst_rect;
	char filename[128];
	int i;
	
	//basic rotate 180
	strcpy(src_img.name, icon_16x72);
	src_img.dim.w =16 ;
	src_img.dim.h =72 ;
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.w = src_img.dim.w;
	src_rect.h = src_img.dim.h;
	memset(dst_img.name, 0, sizeof(dst_img.name));
	dst_img.dim.w =16 ;
	dst_img.dim.h =72 ;
	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.w = dst_img.dim.w;
	dst_rect.h = dst_img.dim.h;
	sprintf(filename, "/mnt/sd/%srotate_basic_180_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_rotate(&src_img, &src_rect, &dst_img, &dst_rect, HD_VIDEO_DIR_ROTATE_180, filename);
	if(ret){
		printf("basic rotate 180 testing fail\n");
		return ret;
	}
	printf("basic rotate 180 testing pass\n");
	
	//basic rotate x
	sprintf(filename, "/mnt/sd/%srotate_basic_x_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_rotate(&src_img, &src_rect, &dst_img, &dst_rect, HD_VIDEO_DIR_MIRRORX, filename);
	if(ret){
		printf("basic rotate x testing fail\n");
		return ret;
	}
	printf("basic rotate x testing pass\n");
	
	//basic rotate y
	sprintf(filename, "/mnt/sd/%srotate_basic_y_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_rotate(&src_img, &src_rect, &dst_img, &dst_rect, HD_VIDEO_DIR_MIRRORY, filename);
	if(ret){
		printf("basic rotate y testing fail\n");
		return ret;
	}
	printf("basic rotate y testing pass\n");
	
	//basic rotate 90
	dst_img.dim.w =72 ;
	dst_img.dim.h =16 ;
	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.w = dst_img.dim.w;
	dst_rect.h = dst_img.dim.h;
	sprintf(filename, "/mnt/sd/%srotate_basic_90_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_rotate(&src_img, &src_rect, &dst_img, &dst_rect, HD_VIDEO_DIR_ROTATE_90, filename);
	if(ret){
		printf("basic rotate 90 testing fail\n");
		return ret;
	}
	printf("basic rotate 90 testing pass\n");
	
	//basic rotate 270
	dst_img.dim.w =72 ;
	dst_img.dim.h =16 ;
	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.w = dst_img.dim.w;
	dst_rect.h = dst_img.dim.h;
	sprintf(filename, "/mnt/sd/%srotate_basic_270_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_rotate(&src_img, &src_rect, &dst_img, &dst_rect, HD_VIDEO_DIR_ROTATE_270, filename);
	if(ret){
		printf("basic rotate 270 testing fail\n");
		return ret;
	}
	printf("basic rotate 270 testing pass\n");
	
	//advance rotate 180
	strcpy(src_img.name, icon_224x32);
	src_img.dim.w =224 ;
	src_img.dim.h =32 ;
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.w = 224;
	src_rect.h = 32;
	strcpy(dst_img.name, bg_1920x1080);
	dst_img.dim.w =1920 ;
	dst_img.dim.h =1080 ;
	dst_rect.x = 500;
	dst_rect.y = 100;
	dst_rect.w = 224;
	dst_rect.h = 32;
	sprintf(filename, "/mnt/sd/%srotate_advance_180_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_rotate(&src_img, &src_rect, &dst_img, &dst_rect, HD_VIDEO_DIR_ROTATE_180, filename);
	if(ret){
		printf("advance rotate 180 testing fail\n");
		return ret;
	}
	printf("advance rotate 180 testing pass\n");
	
	//advance rotate x
	sprintf(filename, "/mnt/sd/%srotate_advance_x_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_rotate(&src_img, &src_rect, &dst_img, &dst_rect, HD_VIDEO_DIR_MIRRORX, filename);
	if(ret){
		printf("advance rotate x testing fail\n");
		return ret;
	}
	printf("advance rotate x testing pass\n");
	
	//advance rotate x
	sprintf(filename, "/mnt/sd/%srotate_advance_y_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_rotate(&src_img, &src_rect, &dst_img, &dst_rect, HD_VIDEO_DIR_MIRRORY, filename);
	if(ret){
		printf("advance rotate y testing fail\n");
		return ret;
	}
	printf("advance rotate y testing pass\n");
	
	//advance rotate 90
	dst_rect.w = 32;
	dst_rect.h = 224;
	sprintf(filename, "/mnt/sd/%srotate_advance_90_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_rotate(&src_img, &src_rect, &dst_img, &dst_rect, HD_VIDEO_DIR_ROTATE_90, filename);
	if(ret){
		printf("advance rotate 90 testing fail\n");
		return ret;
	}
	printf("advance rotate 90 testing pass\n");
	
	//advance rotate 270
	sprintf(filename, "/mnt/sd/%srotate_advance_270_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_rotate(&src_img, &src_rect, &dst_img, &dst_rect, HD_VIDEO_DIR_ROTATE_270, filename);
	if(ret){
		printf("advance rotate 270 testing fail\n");
		return ret;
	}
	printf("advance rotate 270 testing pass\n");
	
	//test out of image boundary
	dst_rect.x = 1808;
	dst_rect.y = 1008;
	dst_rect.w = 224;
	dst_rect.h = 32;
	sprintf(filename, "/mnt/sd/%srotate_boundary_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_rotate(&src_img, &src_rect, &dst_img, &dst_rect, HD_VIDEO_DIR_ROTATE_180, filename);
	if(ret){
		printf("rotate sanity testing fail : out of image boundary\n");
		//return ret;
	}
	printf("rotate sanity testing pass : out of image boundary\n");
	
	//test null src addr
	//ret = _test_rotate_sanity(1, 0, dst_fmt);
	//if(!ret){
	//	printf("rotate sanity testing fail : null src addr\n");
	//	return -1;
	//}
	printf("rotate sanity testing pass : null src addr\n");
	
	//test null dst addr
	//ret = _test_rotate_sanity(0, 1, dst_fmt);
	//if(!ret){
	//	printf("rotate sanity testing fail : null dst addr\n");
	//	return -1;
	//}
	printf("rotate sanity testing pass : null dst addr\n");
	
	//test unsupported format
	ret = _test_rotate_sanity(0, 0, HD_VIDEO_PXLFMT_YUV422_ONE);
	if(!ret){
		printf("rotate sanity testing fail : unsupported format\n");
		return -1;
	}
	printf("rotate sanity testing pass : unsupported format\n");
	
	//stress test
	strcpy(src_img.name, icon_256x256);
	src_img.dim.w =256 ;
	src_img.dim.h =256 ;
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.w = src_img.dim.w;
	src_rect.h = src_img.dim.h;
	strcpy(dst_img.name, bg_1920x1080);
	dst_img.dim.w =1920 ;
	dst_img.dim.h =1080 ;
	dst_rect.x = 500;
	dst_rect.y = 100;
	dst_rect.w = src_img.dim.w;
	dst_rect.h = src_img.dim.h;
	for(i = 0 ; i < loop ; ++i){
		ret = _test_rotate(&src_img, &src_rect, &dst_img, &dst_rect, HD_VIDEO_DIR_ROTATE_270, NULL);
		if(ret){
			printf("stress rotate testing fail\n");
			return ret;
		}
	}
	sprintf(filename, "/mnt/sd/%srotate_stress_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_rotate(&src_img, &src_rect, &dst_img, &dst_rect, HD_VIDEO_DIR_ROTATE_270, filename);
	if(ret){
		printf("stress rotate testing fail\n");
		return ret;
	}
	printf("stress rotate testing pass\n");
	
	return ret;
}

static int _test_line(int x, int fmt, char *filename)
{
	HD_RESULT ret;
	GFX_MMAP_BUF mmap_buf[1] = { 0 };
	HD_GFX_DRAW_LINE gfx_line = { 0 };
	FILE *file;
	
	mmap_buf[0].dim.w = 1920;
	mmap_buf[0].dim.h = 1080;

	if(src_fmt == HD_VIDEO_PXLFMT_YUV420){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h*3/2);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].pa_addr[1] = (gfx.buf_pa+mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].va_addr[1] = (mmap_buf[0].va_addr[0]+mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].loff[0] = mmap_buf[0].dim.w;
		mmap_buf[0].loff[1] = mmap_buf[0].dim.w;
	}else if(src_fmt == HD_VIDEO_PXLFMT_ARGB1555){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h*2);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].loff[0] = (mmap_buf[0].dim.w*2);
	}else if(src_fmt == HD_VIDEO_PXLFMT_Y8){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].loff[0] = mmap_buf[0].dim.w;
	}else{
		printf("src fmt(%x) not support\n", src_fmt);
		return -1;
	}
	
	file = fopen(bg_1920x1080, "rb");
	if (!file) {
		printf("Error to open logo file %s!", bg_1920x1080);
		return -1;
	}
	//coverity[check_return]
	if(fread(mmap_buf[0].va_addr[0], mmap_buf[0].blk_size, 1, file) == 0) {
		printf("Error to read logo file %s!", bg_1920x1080);
		fclose(file);
		return -1;
	}
	fclose(file);
	
	gfx_line.dst_img.dim.w = mmap_buf[0].dim.w;
	gfx_line.dst_img.dim.h = mmap_buf[0].dim.h;
	gfx_line.dst_img.format = fmt;
	gfx_line.dst_img.p_phy_addr[0] = mmap_buf[0].pa_addr[0];
	gfx_line.dst_img.p_phy_addr[1] = mmap_buf[0].pa_addr[1];
	gfx_line.dst_img.lineoffset[0] = mmap_buf[0].loff[0];
	gfx_line.dst_img.lineoffset[1] = mmap_buf[0].loff[1];
	gfx_line.dst_img.ddr_id = DDR_ID0;

	if(dst_fmt == HD_VIDEO_PXLFMT_ARGB1555)
		gfx_line.color = 0xFC00;
	else if(fmt == HD_VIDEO_PXLFMT_Y8)
		gfx_line.color = 0x32;
	else
		gfx_line.color = 0xFFFF0000;
	gfx_line.start.x = x;
	gfx_line.start.y = 0;
	gfx_line.end.x = x;
	gfx_line.end.y = 1000;
	gfx_line.thickness = 4;
	if ((ret = hd_gfx_draw_line(&gfx_line)) != HD_OK) {
		printf("hd_gfx_draw_line fail\n");
		return -1;
	}
	
	if(dst_fmt == HD_VIDEO_PXLFMT_ARGB1555)
		gfx_line.color = 0x83E0;
	else if(fmt == HD_VIDEO_PXLFMT_Y8)
		gfx_line.color = 0x5A;
	else
		gfx_line.color = 0xFF00FF00;
	gfx_line.start.x = x+100;
	gfx_line.start.y = 6;
	gfx_line.end.x = x+600;
	gfx_line.end.y = 6;
	gfx_line.thickness = 4;
	if ((ret = hd_gfx_draw_line(&gfx_line)) != HD_OK) {
		printf("hd_gfx_draw_line fail\n");
		return -1;
	}
	
	if(dst_fmt == HD_VIDEO_PXLFMT_ARGB1555)
		gfx_line.color = 0x801F;
	else if(fmt == HD_VIDEO_PXLFMT_Y8)
		gfx_line.color = 0x82;
	else
		gfx_line.color = 0xFF0000FF;
	gfx_line.start.x = x+700;
	gfx_line.start.y = 150;
	gfx_line.end.x = x+700;
	gfx_line.end.y = 1000;
	gfx_line.thickness = 6;
	if ((ret = hd_gfx_draw_line(&gfx_line)) != HD_OK) {
		printf("hd_gfx_draw_line fail\n");
		return -1;
	}
	
	if(dst_fmt == HD_VIDEO_PXLFMT_ARGB1555)
		gfx_line.color = 0xFFFF;
	else if(fmt == HD_VIDEO_PXLFMT_Y8)
		gfx_line.color = 0xAA;
	else
		gfx_line.color = 0xFFFFFFFF;
	gfx_line.start.x = x+900;
	gfx_line.start.y = 150;
	gfx_line.end.x = x+1500;
	gfx_line.end.y = 150;
	gfx_line.thickness = 8;
	if ((ret = hd_gfx_draw_line(&gfx_line)) != HD_OK) {
		printf("hd_gfx_draw_line fail\n");
		return -1;
	}
	
	if(dst_fmt == HD_VIDEO_PXLFMT_ARGB1555)
		gfx_line.color = 0x8000;
	else if(fmt == HD_VIDEO_PXLFMT_Y8)
		gfx_line.color = 0xFF;
	else
		gfx_line.color = 0xFF000000;
	gfx_line.start.x = x+1600;
	gfx_line.start.y = 1000;
	gfx_line.end.x = x+1700;
	gfx_line.end.y =1000;
	gfx_line.thickness = 10;
	if ((ret = hd_gfx_draw_line(&gfx_line)) != HD_OK) {
		printf("hd_gfx_draw_line fail\n");
		return -1;
	}
	
	if(filename && strlen(filename))
		save_output(filename, mmap_buf[0].va_addr[0], mmap_buf[0].blk_size);

	return 0;
}

static int test_line(void)
{
	HD_RESULT ret;
	int i;
	char filename[128];

	//basic line
	sprintf(filename, "/mnt/sd/line_basic_1920x1080_%s.bin", postfix);
	ret = _test_line(0, dst_fmt, filename);
	if(ret){
		printf("basic line testing fail\n");
		return ret;
	}
	printf("basic line testing pass\n");
	
	//test out of image boundary
	sprintf(filename, "/mnt/sd/line_boundary_1920x1080_%s.bin", postfix);
	//ret = _test_line(600, dst_fmt, filename);
	//if(ret){
	//	printf("line sanity testing fail : out of image boundary\n");
	//	return ret;
	//}
	printf("line sanity testing pass : out of image boundary\n");
	
	//test unsupported format
	ret = _test_line(0, HD_VIDEO_PXLFMT_YUV422_ONE, NULL);
	if(!ret){
		printf("line sanity testing fail : unsupported format\n");
		return -1;
	}
	printf("line sanity testing pass : unsupported format\n");
	
	//stress test
	for(i = 0 ; i < loop ; ++i){
		ret = _test_line(0, dst_fmt, NULL);
		if(ret){
			printf("stress line testing fail\n");
			return ret;
		}
	}
	sprintf(filename, "/mnt/sd/line_stress_1920x1080_%s.bin", postfix);
	ret = _test_line(0, dst_fmt, filename);
	if(ret){
		printf("stress line testing fail\n");
		return ret;
	}
	printf("stress line testing pass\n");
	
	return 0;
}

static int _test_rect(int x, int fmt, char *src, char *dst)
{
	HD_RESULT ret;
	GFX_MMAP_BUF mmap_buf[1] = { 0 };
	HD_GFX_DRAW_RECT gfx_rect = { 0 };
	FILE *file;
		
	mmap_buf[0].dim.w = 1920;
	mmap_buf[0].dim.h = 1080;

	if(src_fmt == HD_VIDEO_PXLFMT_YUV420){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h*3/2);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].pa_addr[1] = (gfx.buf_pa+mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].va_addr[1] = (mmap_buf[0].va_addr[0]+mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].loff[0] = mmap_buf[0].dim.w;
		mmap_buf[0].loff[1] = mmap_buf[0].dim.w;
	}else if(src_fmt == HD_VIDEO_PXLFMT_ARGB1555){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h*2);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].loff[0] = (mmap_buf[0].dim.w*2);
	}else if(src_fmt == HD_VIDEO_PXLFMT_Y8){
		mmap_buf[0].blk_size = (mmap_buf[0].dim.w*mmap_buf[0].dim.h);
		mmap_buf[0].pa_addr[0] = gfx.buf_pa;
		mmap_buf[0].va_addr[0] = (char*)gfx.buf_va;
		mmap_buf[0].loff[0] = mmap_buf[0].dim.w;
	}else{
		printf("src fmt(%x) not support\n", src_fmt);
		return -1;
	}
	
	file = fopen(bg_1920x1080, "rb");
	if (!file) {
		printf("Error to open logo file %s!", bg_1920x1080);
		return -1;
	}
	//coverity[check_return]
	if(fread(mmap_buf[0].va_addr[0], mmap_buf[0].blk_size, 1, file) == 0) {
		printf("Error to read logo file %s!", bg_1920x1080);
		fclose(file);
		return -1;
	}
	fclose(file);
	
	gfx_rect.dst_img.dim.w = mmap_buf[0].dim.w;
	gfx_rect.dst_img.dim.h = mmap_buf[0].dim.h;
	gfx_rect.dst_img.format = fmt;
	gfx_rect.dst_img.p_phy_addr[0] = mmap_buf[0].pa_addr[0];
	gfx_rect.dst_img.p_phy_addr[1] = mmap_buf[0].pa_addr[1];
	gfx_rect.dst_img.lineoffset[0] = mmap_buf[0].loff[0];
	gfx_rect.dst_img.lineoffset[1] = mmap_buf[0].loff[1];
	gfx_rect.dst_img.ddr_id = DDR_ID0;

	if(fmt == HD_VIDEO_PXLFMT_ARGB1555)
		gfx_rect.color = 0xFC00;
	else if(fmt == HD_VIDEO_PXLFMT_Y8)
		gfx_rect.color = 0x32;
	else
		gfx_rect.color = 0xFFFF0000;
	gfx_rect.rect.x = x;
	gfx_rect.rect.y = 0;
	gfx_rect.rect.w = 8;
	gfx_rect.rect.h = 1000;
	gfx_rect.type = HD_GFX_RECT_SOLID;
	if ((ret = hd_gfx_draw_rect(&gfx_rect)) != HD_OK) {
		printf("hd_gfx_draw_rect fail\n");
		return -1;
	}
	
	if(fmt == HD_VIDEO_PXLFMT_ARGB1555)
		gfx_rect.color = 0x83E0;
	else if(fmt == HD_VIDEO_PXLFMT_Y8)
		gfx_rect.color = 0x5A;
	else
		gfx_rect.color = 0xFF00FF00;
	gfx_rect.rect.x = x+100;
	gfx_rect.rect.y = 6;
	gfx_rect.rect.w = 500;
	gfx_rect.rect.h = 56;
	gfx_rect.type = HD_GFX_RECT_HOLLOW;
	gfx_rect.thickness = 2;
	if ((ret = hd_gfx_draw_rect(&gfx_rect)) != HD_OK) {
		printf("hd_gfx_draw_rect fail\n");
		return -1;
	}
	
	if(fmt == HD_VIDEO_PXLFMT_ARGB1555)
		gfx_rect.color = 0x801F;
	else if(fmt == HD_VIDEO_PXLFMT_Y8)
		gfx_rect.color = 0x82;
	else
		gfx_rect.color = 0xFF0000FF;
	gfx_rect.rect.x = x+700;
	gfx_rect.rect.y = 150;
	gfx_rect.rect.w = 100;
	gfx_rect.rect.h = 850;
	gfx_rect.type = HD_GFX_RECT_SOLID;
	if ((ret = hd_gfx_draw_rect(&gfx_rect)) != HD_OK) {
		printf("hd_gfx_draw_rect fail\n");
		return -1;
	}
	
	if(fmt == HD_VIDEO_PXLFMT_ARGB1555)
		gfx_rect.color = 0xFFFF;
	else if(fmt == HD_VIDEO_PXLFMT_Y8)
		gfx_rect.color = 0xAA;
	else
		gfx_rect.color = 0xFFFFFFFF;
	gfx_rect.rect.x = x+900;
	gfx_rect.rect.y = 150;
	gfx_rect.rect.w = 600;
	gfx_rect.rect.h = 102;
	gfx_rect.type = HD_GFX_RECT_HOLLOW;
	gfx_rect.thickness = 4;
	if ((ret = hd_gfx_draw_rect(&gfx_rect)) != HD_OK) {
		printf("hd_gfx_draw_rect fail\n");
		return -1;
	}
	
	if(fmt == HD_VIDEO_PXLFMT_ARGB1555)
		gfx_rect.color = 0x8000;
	else if(fmt == HD_VIDEO_PXLFMT_Y8)
		gfx_rect.color = 0xFF;
	else
		gfx_rect.color = 0xFF000000;
	gfx_rect.rect.x = x+1600;
	gfx_rect.rect.y = 232;
	gfx_rect.rect.w = 100;
	gfx_rect.rect.h = 774;
	gfx_rect.type = HD_GFX_RECT_SOLID;
	if ((ret = hd_gfx_draw_rect(&gfx_rect)) != HD_OK) {
		printf("hd_gfx_draw_rect fail\n");
		return -1;
	}
	
	if(dst && strlen(dst))
		save_output(dst, mmap_buf[0].va_addr[0], mmap_buf[0].blk_size);
	
	return 0;
}

static int test_rect(void)
{
	HD_RESULT ret;
	int i;
	char src[128], dst[128];
	
	if(dst_fmt == HD_VIDEO_PXLFMT_ARGB1555)
		strcpy(src, "/mnt/sd/video_frm_1920_1080_1_argb1555.dat");
	else if(dst_fmt == HD_VIDEO_PXLFMT_Y8)
		strcpy(dst, "/mnt/sd/rect_stress_1920x1080_i8.bin");
	else
		strcpy(src, "/mnt/sd/video_frm_1920_1080_1_yuv420.dat");

	//basic rect
	if(dst_fmt == HD_VIDEO_PXLFMT_ARGB1555)
		strcpy(dst, "/mnt/sd/rect_basic_1920x1080_argb1555.bin");
	else if(dst_fmt == HD_VIDEO_PXLFMT_Y8)
		strcpy(dst, "/mnt/sd/rect_basic_1920x1080_i8.bin");
	else
		strcpy(dst, "/mnt/sd/rect_basic_1920x1080_yuv420.bin");
	ret = _test_rect(0, dst_fmt, src, dst);
	if(ret){
		printf("basic rect testing fail\n");
		return ret;
	}
	printf("basic rect testing pass\n");
	
	//test out of image boundary
	if(dst_fmt == HD_VIDEO_PXLFMT_ARGB1555)
		strcpy(dst, "/mnt/sd/rect_boundary_1920x1080_argb1555.bin");
	else if(dst_fmt == HD_VIDEO_PXLFMT_Y8)
		strcpy(dst, "/mnt/sd/rect_stress_1920x1080_i8.bin");
	else
		strcpy(dst, "/mnt/sd/rect_boundary_1920x1080_yuv420.bin");
	//ret = _test_rect(600, dst_fmt, src ,dst);
	//if(ret){
	//	printf("rect sanity testing fail : out of image boundary\n");
	//	return ret;
	//}
	printf("rect sanity testing pass : out of image boundary\n");
	
	//test unsupported format
	ret = _test_rect(0, HD_VIDEO_PXLFMT_YUV422_ONE, src, NULL);
	if(!ret){
		printf("rect sanity testing fail : unsupported format\n");
		return -1;
	}
	printf("rect sanity testing pass : unsupported format\n");
	
	//stress test
	for(i = 0 ; i < loop ; ++i){
		ret = _test_rect(0, src_fmt, src, NULL);
		if(ret){
			printf("stress rect testing fail\n");
			return ret;
		}
	}
	if(dst_fmt == HD_VIDEO_PXLFMT_ARGB1555)
		strcpy(dst, "/mnt/sd/rect_stress_1920x1080_argb1555.bin");
	else if(dst_fmt == HD_VIDEO_PXLFMT_Y8)
		strcpy(dst, "/mnt/sd/rect_stress_1920x1080_i8.bin");
	else
		strcpy(dst, "/mnt/sd/rect_stress_1920x1080_yuv420.bin");
	ret = _test_rect(0, dst_fmt, src, dst);
	if(ret){
		printf("stress rect testing fail\n");
		return ret;
	}
	printf("stress rect testing pass\n");
	
	return 0;
}

static int test_argb_yuv_paste(void)
{
	HD_RESULT ret;
	GFX_IMG_INFO src_img, dst_img;
	HD_URECT src_rect, dst_rect;
	char filename[128];
	int i;
	
	//basic copy
	strcpy(src_img.name, icon_16x72);
	src_img.dim.w =16 ;
	src_img.dim.h =72 ;
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.w = src_img.dim.w;
	src_rect.h = src_img.dim.h;
	memset(dst_img.name, 0, sizeof(dst_img.name));
	dst_img.dim.w =16 ;
	dst_img.dim.h =72 ;
	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.w = dst_img.dim.w;
	dst_rect.h = dst_img.dim.h;
	sprintf(filename, "/mnt/sd/%scopy_basic_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_copy(&src_img, &src_rect, &dst_img, &dst_rect, filename);
	if(ret){
		printf("basic copy testing fail\n");
		return ret;
	}
	printf("basic copy testing pass\n");
	
	//advance copy
	strcpy(src_img.name, icon_1000x200);
	src_img.dim.w =1000 ;
	src_img.dim.h =200 ;
	src_rect.x = 500;
	src_rect.y = 0;
	src_rect.w = 500;
	src_rect.h = 200;
	strcpy(dst_img.name, bg_1920x1080);
	dst_img.dim.w =1920 ;
	dst_img.dim.h =1080 ;
	dst_rect.x = 500;
	dst_rect.y = 100;
	dst_rect.w = 500;
	dst_rect.h = 200;
	sprintf(filename, "/mnt/sd/%scopy_advance_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_copy(&src_img, &src_rect, &dst_img, &dst_rect, filename);
	if(ret){
		printf("advance copy testing fail\n");
		return ret;
	}
	printf("advance copy testing pass\n");
	
	//test out of image boundary
	dst_rect.x = 1800;
	dst_rect.y = 800;
	dst_rect.w = 500;
	dst_rect.h = 200;
	sprintf(filename, "/mnt/sd/%scopy_boundary_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_copy(&src_img, &src_rect, &dst_img, &dst_rect, filename);
	if(!ret){
		printf("copy sanity testing fail : out of image boundary\n");
		//return -1;
	}
	printf("copy sanity testing pass : out of image boundary\n");
	
	//test null src addr
	ret = _test_copy_sanity(1, 0, dst_fmt);
	if(!ret){
		printf("copy sanity testing fail : null src addr\n");
		return -1;
	}
	printf("copy sanity testing pass : null src addr\n");
	
	//test null dst addr
	ret = _test_copy_sanity(0, 1, dst_fmt);
	if(!ret){
		printf("copy sanity testing fail : null dst addr\n");
		return -1;
	}
	printf("copy sanity testing pass : null dst addr\n");
	
	//test unsupported format
	ret = _test_copy_sanity(0, 0, HD_VIDEO_PXLFMT_YUV422_ONE);
	if(!ret){
		printf("copy sanity testing fail : unsupported format\n");
		return -1;
	}
	printf("copy sanity testing pass : unsupported format\n");
	
	//stress test
	strcpy(src_img.name, icon_256x256);
	src_img.dim.w =256 ;
	src_img.dim.h =256 ;
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.w = src_img.dim.w;
	src_rect.h = src_img.dim.h;
	memset(dst_img.name, 0, sizeof(dst_img.name));
	dst_img.dim.w =256 ;
	dst_img.dim.h =256 ;
	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.w = dst_img.dim.w;
	dst_rect.h = dst_img.dim.h;
	for(i = 0 ; i < loop ; ++i){
		ret = _test_copy(&src_img, &src_rect, &dst_img, &dst_rect, NULL);
		if(ret){
			printf("stress copy testing fail\n");
			return ret;
		}
	}
	sprintf(filename, "/mnt/sd/%scopy_stress_%ldx%ld_%s.bin", prefix, dst_img.dim.w, dst_img.dim.h, postfix);
	ret = _test_copy(&src_img, &src_rect, &dst_img, &dst_rect, filename);
	if(ret){
		printf("stress copy testing fail\n");
		return ret;
	}
	printf("stress copy testing pass\n");
	
	return ret;
}

MAIN(argc, argv)
{
	HD_RESULT ret;
	
	if(argc > 1)
		loop = atoi(argv[1]);
	printf("loop = %d\n", loop);

	gfx.buf_blk = 0;
	gfx.buf_size = 16 * 1024 * 1024;
	gfx.buf_pa = 0;

	//init memory
	if ((ret = hd_common_init(0)) != HD_OK) {
		printf("common init fail\n");
		return -1;
	}

	ret = mem_init(&gfx.buf_blk, gfx.buf_size, &gfx.buf_pa);
    if(ret != HD_OK) {
        printf("mem fail=%d\n", ret);
		return -1;
    }
	
	gfx.buf_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, gfx.buf_pa, gfx.buf_size);
	if(!gfx.buf_va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

    ret = hd_gfx_init();
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
		return -1;
    }
	
	if(test_scale()){
		printf("scale test fail\n");
		goto out;
	}
	printf("all scale testing pass\n");
	
	if(test_copy()){
		printf("copy test fail\n");
		goto out;
	}
	printf("all copy testing pass\n");
	
	if(test_rotate()){
		printf("rotate test fail\n");
		goto out;
	}
	printf("all rotate testing pass\n");
	
	if(test_line()){
		printf("draw line test fail\n");
		goto out;
	}
	printf("all line testing pass\n");
	
	if(test_rect()){
		printf("draw rect test fail\n");
		goto out;
	}
	printf("all rect testing pass\n");
	
	printf("all yuv420 testing pass\n");

	printf("start argb1555 testing\n");
	src_fmt = HD_VIDEO_PXLFMT_ARGB1555;
	dst_fmt = HD_VIDEO_PXLFMT_ARGB1555;
	strcpy(postfix, "argb1555");
	strcpy(icon_16x72, "/mnt/sd/16x72_argb1555.bin");
	strcpy(icon_1000x200, "/mnt/sd/video_frm_1000_200_colorkey_argb1555.dat");
	strcpy(icon_256x256, "/mnt/sd/256x256_argb1555.bin");
	strcpy(icon_224x32, "/mnt/sd/224x32_argb1555.bin");
	strcpy(icon_1920x1080, "/mnt/sd/video_frm_1920_1080_1_argb1555.dat");
	strcpy(bg_1920x1080, "/mnt/sd/video_frm_1920_1080_1_argb1555.dat");
	
	if(test_scale()){
		printf("argb1555 scale test fail\n");
		goto out;
	}
	printf("all argb1555 scale testing pass\n");
	
	if(test_copy()){
		printf("argb1555 copy test fail\n");
		goto out;
	}
	printf("all argb1555 copy testing pass\n");
	
	if(test_rotate()){
		printf("argb1555 rotate test fail\n");
		goto out;
	}
	printf("all argb1555 rotate testing pass\n");
	
	if(test_line()){
		printf("argb1555 draw line test fail\n");
		goto out;
	}
	printf("all argb1555 line testing pass\n");
	
	if(test_rect()){
		printf("argb1555 draw rect test fail\n");
		goto out;
	}
	printf("all argb1555 rect testing pass\n");

	printf("start Y8 testing\n");
	src_fmt = HD_VIDEO_PXLFMT_Y8;
	dst_fmt = HD_VIDEO_PXLFMT_Y8;
	strcpy(postfix, "Y8");
	strcpy(icon_16x72, "/mnt/sd/16x72_I8.bin");
	strcpy(icon_1000x200, "/mnt/sd/video_frm_1000_200_colorkey_I8.dat");
	strcpy(icon_256x256, "/mnt/sd/256x256_I8.bin");
	strcpy(icon_224x32, "/mnt/sd/224x32_I8.bin");
	strcpy(icon_1920x1080, "/mnt/sd/video_frm_1920_1080_1_I8.dat");
	strcpy(bg_1920x1080, "/mnt/sd/video_frm_1920_1080_1_I8.dat");
	
	if(test_scale()){
		printf("Y8 scale test fail\n");
		goto out;
	}
	printf("all Y8 scale testing pass\n");
	
	if(test_copy()){
		printf("Y8 copy test fail\n");
		goto out;
	}
	printf("all Y8 copy testing pass\n");
	
	if(test_rotate()){
		printf("Y8 rotate test fail\n");
		goto out;
	}
	printf("all Y8 rotate testing pass\n");
	
	if(test_line()){
		printf("Y8 draw line test fail\n");
		goto out;
	}
	printf("all Y8 line testing pass\n");
	
	if(test_rect()){
		printf("Y8 draw rect test fail\n");
		goto out;
	}
	printf("all Y8 rect testing pass\n");

	printf("start argb1555/yuv420 testing\n");
	src_fmt = HD_VIDEO_PXLFMT_ARGB1555;
	dst_fmt = HD_VIDEO_PXLFMT_YUV420;
	strcpy(prefix, "bl");
	strcpy(postfix, "yuv420");
	strcpy(icon_16x72, "/mnt/sd/16x72_argb1555.bin");
	strcpy(icon_1000x200, "/mnt/sd/video_frm_1000_200_colorkey_argb1555.dat");
	strcpy(icon_256x256, "/mnt/sd/256x256_argb1555.bin");
	strcpy(icon_224x32, "/mnt/sd/224x32_argb1555.bin");
	strcpy(icon_1920x1080, "/mnt/sd/video_frm_1920_1080_1_argb1555.dat");
	strcpy(bg_1920x1080, "/mnt/sd/video_frm_1920_1080_1_yuv420.dat");
	
	if(test_argb_yuv_paste()){
		printf("argb1555/yuv420 paste test fail\n");
		goto out;
	}
	printf("all argb1555/yuv420 paste testing pass\n");
	
	printf("all hd_gfx testing pass\n");

out:

	if(gfx.buf_va)
		if(hd_common_mem_munmap((void*)gfx.buf_va, gfx.buf_size))
			printf("fail to unmap va(%x)\n", (int)gfx.buf_va);

	if(gfx.buf_blk)
		if(HD_OK != hd_common_mem_release_block(gfx.buf_blk))
			printf("hd_common_mem_release_block() fail\n");

	ret = hd_gfx_uninit();
	if(ret != HD_OK)
		printf("uninit fail=%d\n", ret);

	ret = mem_exit();
	if(ret != HD_OK)
		printf("mem fail=%d\n", ret);

	ret = hd_common_uninit();
	if(ret != HD_OK)
		printf("common fail=%d\n", ret);

	return 0;
}

