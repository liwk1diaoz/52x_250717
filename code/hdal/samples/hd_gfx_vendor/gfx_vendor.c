/**
	@brief Sample code of gfx.\n

	@file gfx_only.c

	@author YongChang Qui

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include "hdal.h"
#include "hd_debug.h"
#include "vendor_gfx.h"

// platform dependent
#if defined(__LINUX)
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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_gfx_only, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT test_alpha_plane(UINT32 buf_pa, UINT32 buf_size)
{
	int                            fd, i;
	VENDOR_GFX_ALPHA_BLEND         param;
	void                           *va;
	HD_RESULT                      ret;
	HD_VIDEO_FRAME                 frame;
	UINT32                         src_size, src_pa, src_va;
	UINT32                         dst_size, dst_pa, dst_va;
	UINT32                         alpha_size, alpha_pa, alpha_va;
	int                            len;

	//calculate background's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	frame.loff[1] = 1920;
	dst_size = hd_common_mem_calc_buf_size(&frame);
	if(!dst_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	src_size = dst_size;
	alpha_size = src_size;

	if((src_size + dst_size + alpha_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (src_size + dst_size + alpha_size), buf_size);
		return -1;
	}

	//calculate physical address
	src_pa   = buf_pa;
	alpha_pa = src_pa   + src_size;
	dst_pa   = alpha_pa + alpha_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	//calculate virtual address
	src_va   = (UINT32)va;
	alpha_va = src_va + src_size;
	dst_va   = alpha_va + alpha_size;
	
	//fill source image with gray
	for(i = 0 ; i < (int)src_size ; ++i)
		*(unsigned char*)(src_va+i) = 0x80;
		
	//fill alpha buffer's y plane with vertical gradient
	for(i = 0 ; i < 1080 ; ++i)
		memset((void*)(alpha_va+i*1920), ((i*255)/1080), 1920);
		
	//fill alpha buffer's uv plane with vertical gradient
	for(i = 0 ; i < 540 ; ++i)
		memset((void*)(alpha_va+1920*1080+i*1920), ((i*255)/540), 1920);

	//load background image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv420.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		printf("video_frm_1920_1080_1_yuv420.dat is in SDK/code/hdal/samples/pattern/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	//use gfx engine to blend gray src to dst with alpha effect
	memset(&param, 0, sizeof(VENDOR_GFX_ALPHA_BLEND));
	param.src_img.dim.w            = 1920;
	param.src_img.dim.h            = 1080;
	param.src_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.src_img.p_phy_addr[0]    = src_pa;
	param.src_img.p_phy_addr[1]    = src_pa + 1920 * 1080;
	param.src_img.lineoffset[0]    = 1920;
	param.src_img.lineoffset[1]    = 1920;
	param.dst_img.dim.w            = 1920;
	param.dst_img.dim.h            = 1080;
	param.dst_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.dst_img.p_phy_addr[0]    = dst_pa;
	param.dst_img.p_phy_addr[1]    = dst_pa + 1920 * 1080;
	param.dst_img.lineoffset[0]    = 1920;
	param.dst_img.lineoffset[1]    = 1920;
	param.dst_pos.x                = 0;
	param.dst_pos.y                = 0;
	param.p_alpha                  = alpha_pa;

	ret = vendor_gfx_alpha_blend(&param);
	if(ret != HD_OK){
		printf("vendor_gfx_alpha_blend fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_alpha_plane_1920_1080_1.yuv420", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_alpha_plane_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to write /mnt/sd/hd_gfx_alpha_plane_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_alpha_plane_1920_1080_1.yuv420\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

static HD_RESULT test_acc(UINT32 buf_pa, UINT32 buf_size)
{
	int                            fd;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	void                           *va;
	HD_RESULT                      ret;
	HD_VIDEO_FRAME                 frame;
	UINT32                         src_size, src_pa, src_va;
	int                            len;

	//calculate background's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_ARGB1555;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	frame.loff[1] = 1920;
	src_size = hd_common_mem_calc_buf_size(&frame);
	if(!src_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	if(src_size > buf_size){
		printf("required size(%d) > allocated size(%d)\n", src_size, buf_size);
		return -1;
	}

	src_pa = buf_pa;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	src_va = (UINT32)va;

	//load image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_argb1555.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_argb1555.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)src_va, src_size);
	close(fd);
	if(len != (int)src_size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_argb1555.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_ACC;
	param.format = VENDOR_GFX_GRPH_FORMAT_16BITS;
	param.property[0].en = 1;
	param.property[0].id = VENDOR_GFX_GRPH_PROPERTY_ID_ACC_RESULT;
	param.property[0].data.property = 0;
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
	param.images[0].dram_addr = src_pa;
	param.images[0].lineoffset = 3840;
	param.images[0].width = 1920;
	param.images[0].height = 1080;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}
	
	printf("acc = %x\n", param.property[0].data.property);

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

static HD_RESULT test_multiply_div(UINT32 buf_pa, UINT32 buf_size)
{
	void                *va;
	UINT32              op1_pa, op1_va, op2_pa, op2_va, out_pa, out_va;
	int                 i;
	UINT32              size = 1024;
	HD_RESULT           ret = HD_ERR_NOT_SUPPORT;
	HD_GFX_ARITHMETIC   param;
	
	if((size * 3 * sizeof(short)) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (size * 3 * sizeof(short)), buf_size);
		return -1;
	}
	
	op1_pa = buf_pa;
	op2_pa = op1_pa + (size * sizeof(short));
	out_pa = op2_pa + (size * sizeof(char));
	
	//map physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	op1_va = (UINT32)va;
	op2_va = op1_va + (size * sizeof(short));
	out_va = op2_va + (size * sizeof(char));
	
	//generate random data
	for(i = 0 ; i < (int)size ; ++i){
		((short*)op1_va)[i] = (i % 10);
		((char*)op2_va)[i] = (i % 255);
	}
	memset((void*)out_va, 0, size * sizeof(short));

	//use gfx engine to perform array multiply 
	memset(&param, 0, sizeof(HD_GFX_ARITHMETIC));
	param.p_op1        = op1_pa;
	param.p_op2        = op2_pa;
	param.p_out        = out_pa;
	param.size         = size;
	param.operation    = HD_GFX_ARITH_OP_MULTIPLY;
	param.bits         = 16;
	
	if(hd_gfx_arithmetic(&param) != HD_OK){
		printf("hd_gfx_arithmetic fail\n");
		goto exit;
	}
	
	//check data integrity
	for(i = 0 ; i < (int)size ; ++i)
		if(((short*)out_va)[i] != ((i%10)*(i%255))){
			printf("out[%d] is %d, not %d. op1 is %d, op2 is %d\n", i, 
				((short*)out_va)[i], ((i%10)*(i%255)), ((short*)op1_va)[i], ((char*)op2_va)[i]);
			goto exit;
		}
		
	printf("multiply operation of two 16bits arrays ok\n");
	ret = HD_OK;

exit:
		
	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);
	
	return ret;
}

static HD_RESULT test_alpha_blend(UINT32 buf_pa, UINT32 buf_size)
{
	int                 fd;
	HD_GFX_COPY         param;
	void                *va;
	HD_RESULT           ret;
	HD_VIDEO_FRAME      frame;
	UINT32              src_size, src_pa, src_va, dst_size, dst_pa, dst_va;
	int                 len;

	//calculate logo's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = 200;
	frame.loff[0] = 1000;
	frame.loff[1] = 1000;
	src_size = hd_common_mem_calc_buf_size(&frame);
	if(!src_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	//calculate background's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	frame.loff[1] = 1920;
	dst_size = hd_common_mem_calc_buf_size(&frame);
	if(!dst_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	if((src_size + dst_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (src_size + dst_size), buf_size);
		return -1;
	}

	src_pa = buf_pa;
	dst_pa = src_pa + src_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	src_va = (UINT32)va;
	dst_va = src_va + src_size;

	//load logo from sd card
	fd = open("/mnt/sd/video_frm_1000_200_colorkey_yuv420.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1000_200_colorkey_yuv420.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)src_va, src_size);
	close(fd);
	if(len != (int)src_size){
		printf("fail to read /mnt/sd/video_frm_1000_200_colorkey_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	//load background image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv420.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	//use gfx engine to copy the logo to the background image
	memset(&param, 0, sizeof(HD_GFX_COPY));
	param.src_img.dim.w            = 1000;
	param.src_img.dim.h            = 200;
	param.src_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.src_img.p_phy_addr[0]    = src_pa;
	param.src_img.p_phy_addr[1]    = src_pa + 1000 * 200;
	param.src_img.lineoffset[0]    = 1000;
	param.src_img.lineoffset[1]    = 1000;
	param.dst_img.dim.w            = 1920;
	param.dst_img.dim.h            = 1080;
	param.dst_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.dst_img.p_phy_addr[0]    = dst_pa;
	param.dst_img.p_phy_addr[1]    = dst_pa + 1920 * 1080;
	param.dst_img.lineoffset[0]    = 1920;
	param.dst_img.lineoffset[1]    = 1920;
	param.src_region.x             = 0;
	param.src_region.y             = 0;
	param.src_region.w             = 1000;
	param.src_region.h             = 200;
	param.dst_pos.x                = 300;
	param.dst_pos.y                = 500;
	param.colorkey                 = 0;
	param.alpha                    = 64;

	ret = hd_gfx_copy(&param);
	if(ret != HD_OK){
		printf("hd_gfx_copy fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_alpha_blend_1920_1080_1.yuv420", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_alpha_blend_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to write /mnt/sd/hd_gfx_alpha_blend_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_alpha_blend_1920_1080_1.yuv420\n");

exit:

	return ret;
}

static HD_RESULT test_text_mul(UINT32 buf_pa, UINT32 buf_size)
{
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	void                           *va;
	HD_RESULT                      ret;
	HD_VIDEO_FRAME                 frame;
	UINT32                         src_size, src_pa, src_va, dst_size, dst_pa, dst_va;
	int                            i, j;

	//calculate buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_Y8;
	frame.dim.h   = 200;
	frame.loff[0] = 1000;
	frame.loff[1] = 1000;
	src_size = hd_common_mem_calc_buf_size(&frame);
	if(!src_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	dst_size = src_size;
	if((src_size + dst_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (src_size + dst_size), buf_size);
		return -1;
	}

	src_pa = buf_pa;
	dst_pa = src_pa + src_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	src_va = (UINT32)va;
	dst_va = src_va + src_size;
	
	//generate random data
	for(i = 0 ; i < 200 ; ++i)
		for(j = 0 ; j < 1000 ; ++j)
			((char*)src_va)[i*1000 + j] = (i>>1);
	memset((void*)dst_va, 0, dst_size);

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_TEXT_MUL;
	param.format = VENDOR_GFX_GRPH_FORMAT_8BITS;
	param.property[0].en = 1;
	param.property[0].id = VENDOR_GFX_GRPH_PROPERTY_ID_NORMAL;
	param.property[0].data.property = VENDOR_GFX_GRPH_TEXT_MULT_PROPTY(VENDOR_GFX_GRPH_TEXTMUL_BYTE, VENDOR_GFX_GRPH_TEXTMUL_SIGNED, 7, 2);
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
	param.images[0].dram_addr = src_pa;
	param.images[0].lineoffset = 1000;
	param.images[0].width = 1000;
	param.images[0].height = 200;
	param.images[0].inoutop.en = 1;
	param.images[0].inoutop.id = VENDOR_GFX_GRPH_INOUT_ID_IN_A;
	param.images[0].inoutop.op = VENDOR_GFX_GRPH_INOP_INVERT;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
	param.images[1].dram_addr = dst_pa;
	param.images[1].lineoffset = 1000;
	param.images[1].inoutop.en = 1;
	param.images[1].inoutop.id = VENDOR_GFX_GRPH_INOUT_ID_OUT_C;
	param.images[1].inoutop.op = VENDOR_GFX_GRPH_OUTOP_SHF;
	param.images[1].inoutop.shifts = 3;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}
	
	//check data integrity
	for(i = 0 ; i < 200 ; ++i)
		for(j = 0 ; j < 1000 ; ++j)
			if((((char*)dst_va)[i*1000 + j]) != ((((255-(i>>1))*7 + 2 ) / 4) / 8)){
				printf("element[%d][%d] has incorrect value %d, it should be %d\n", 
						j, i, ((char*)dst_va)[i*1000 + j], ((((255-(i>>1))*7 + 2 ) / 4) / 8));
				goto exit;
			}
		
	printf("text mul operation ok\n");
	ret = HD_OK;

exit:
		
	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);
	
	return ret;
}

//copy an argb4444 logo to a yuv420 background image
static HD_RESULT test_copy(UINT32 buf_pa, UINT32 buf_size)
{
	int                            fd;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	void                           *va;
	HD_RESULT                      ret;
	HD_VIDEO_FRAME                 frame;
	UINT32                         src_size, src_pa, src_va, dst_size, dst_pa, dst_va;
	int                            len;

	//calculate logo's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_ARGB4444;
	frame.dim.h   = 200;
	frame.loff[0] = 1000;
	frame.loff[1] = 1000;
	src_size = hd_common_mem_calc_buf_size(&frame);
	if(!src_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	//calculate background's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	frame.loff[1] = 1920;
	dst_size = hd_common_mem_calc_buf_size(&frame);
	if(!dst_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	if((src_size + dst_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (src_size + dst_size), buf_size);
		return -1;
	}

	src_pa = buf_pa;
	dst_pa = src_pa + src_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	src_va = (UINT32)va;
	dst_va = src_va + src_size;

	//load logo from sd card
	fd = open("/mnt/sd/video_frm_1000_200_1_argb4444.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1000_200_1_argb4444.dat\n");
		printf("hd_video_record_osg.bin is in SDK/code/hdal/samples/pattern/\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)src_va, src_size);
	close(fd);
	if(len != (int)src_size){
		printf("fail to read /mnt/sd/video_frm_1000_200_1_argb4444.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	//load background image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv420.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		printf("video_frm_1920_1080_1_yuv420.dat is in SDK/code/hdal/samples/pattern/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_RGBYUV_BLEND;
	param.format = VENDOR_GFX_GRPH_FORMAT_16BITS_ARGB4444_RGB;
	param.property[0].en = 1;
	param.property[0].id = VENDOR_GFX_GRPH_PROPERTY_ID_YUVFMT;
	param.property[0].data.property = 1;
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A; //src argb
	param.images[0].dram_addr = src_pa;
	param.images[0].lineoffset = 2000;
	param.images[0].width = 2000;
	param.images[0].height = 200;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_B; //dst Y
	param.images[1].dram_addr = dst_pa + (300*1920+500);
	param.images[1].lineoffset = 1920;
	param.images[2].img_id = VENDOR_GFX_GRPH_IMG_ID_C; //dst UV
	param.images[2].dram_addr = (dst_pa + 1920*1080 + (300*1920)/2 + 500);
	param.images[2].lineoffset = 1920;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_copy_1920_1080_1.yuv420", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_copy_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to write /mnt/sd/hd_gfx_copy_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_copy_1920_1080_1.yuv420\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

//create mirror for a yuv420 image
static HD_RESULT test_rotation(UINT32 buf_pa, UINT32 buf_size)
{
	int                            fd;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	void                           *va;
	HD_RESULT                      ret;
	HD_VIDEO_FRAME                 frame;
	UINT32                         size, src_pa, src_va, dst_pa, dst_va;
	int                            len, i;

	//calculate yuv420 image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	frame.loff[1] = 1920;
	size = hd_common_mem_calc_buf_size(&frame);
	if(!size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}
	if((size * 2) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", size * 2, buf_size);
		return -1;
	}

	src_pa = buf_pa;
	dst_pa = src_pa + size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	src_va = (UINT32)va;
	dst_va = src_va + size;


	//load yuv420 image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv420.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		printf("video_frm_1920_1080_1_yuv420.dat is in SDK/code/hdal/samples/pattern/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)src_va, size);
	close(fd);
	if(len != (int)size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	for(i = 0 ; i < 2 ; ++i){
		param.command = VENDOR_GFX_GRPH_CMD_VTC_FLIP;
		if(i == 0)
			param.format = VENDOR_GFX_GRPH_FORMAT_8BITS;
		else
			param.format = VENDOR_GFX_GRPH_FORMAT_16BITS;
		param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
		param.images[0].dram_addr = src_pa + (i * 1920 * 1080);
		param.images[0].lineoffset = 1920;
		param.images[0].width = 1920;
		if(i == 0)
			param.images[0].height = 1080;
		else
			param.images[0].height = 540;
		param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
		param.images[1].dram_addr = dst_pa + (i * 1920 * 1080);
		param.images[1].lineoffset = 1920;

		ret = vendor_gfx_grph_trigger(1, &param);
		if(ret != HD_OK){
			printf("vendor_gfx_grph_trigger fail=%d\n", ret);
			goto exit;
		}
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_rotate_1920_1080_1.yuv420", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_rotate_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)dst_va, size);
	close(fd);
	if(len != (int)size){
		printf("fail to write /mnt/sd/hd_gfx_rotate_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_rotate_1920_1080_1.yuv420\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

//transform an argb4444 image to a yuv420 image
static HD_RESULT test_color_transform(UINT32 buf_pa, UINT32 buf_size)
{
	int                            fd, i;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	void                           *va;
	HD_RESULT                      ret;
	HD_VIDEO_FRAME                 frame;
	UINT32                         rgb_size, yuv_size;
	int                            len;
	int                            logo_w = 1000, logo_h = 200;
	UINT32                         rgb_pa, rgb_va, yuv_pa, yuv_va;

	//calculate yuv420 image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = logo_h;
	frame.loff[0] = logo_w;
	frame.loff[1] = logo_w;
	yuv_size = hd_common_mem_calc_buf_size(&frame);
	if(!yuv_size){
		printf("hd_common_mem_calc_buf_size() fail to calculate yuv size\n");
		return -1;
	}

	//calculate argb4444 image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_ARGB4444;
	frame.dim.h   = logo_h;
	frame.loff[0] = logo_w;
	rgb_size = hd_common_mem_calc_buf_size(&frame);
	if(!rgb_size){
		printf("hd_common_mem_calc_buf_size() fail to calculate yuv size\n");
		return -1;
	}

	if((yuv_size + rgb_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (yuv_size + rgb_size), buf_size);
		return -1;
	}

	rgb_pa = buf_pa;
	yuv_pa = rgb_pa + rgb_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	rgb_va = (UINT32)va;
	yuv_va = rgb_va + rgb_size;

	fd = open("/mnt/sd/video_frm_1000_200_1_argb4444.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1000_200_1_argb4444.dat\n");
		printf("hd_video_record_osg.bin is in SDK/code/hdal/samples/pattern/\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	//load argb4444 image from sd card
	len = read(fd, (void*)rgb_va, rgb_size);
	close(fd);
	if(len != (int)rgb_size){
		printf("fail to read /mnt/sd/video_frm_1000_200_1_argb4444.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	for(i = 0 ; i < logo_w*logo_h ; ++i)
		if(!((unsigned short*)rgb_va)[i])
			((unsigned short*)rgb_va)[i] = 0xFFFF;

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_RGBYUV_BLEND;
	param.format = VENDOR_GFX_GRPH_FORMAT_16BITS_ARGB4444_RGB;
	param.property[0].en = 1;
	param.property[0].id = VENDOR_GFX_GRPH_PROPERTY_ID_YUVFMT;
	param.property[0].data.property = 1;
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A; //src argb
	param.images[0].dram_addr = rgb_pa;
	param.images[0].lineoffset = 2000;
	param.images[0].width = 2000;
	param.images[0].height = 200;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_B; //dst Y
	param.images[1].dram_addr = yuv_pa;
	param.images[1].lineoffset = 1000;
	param.images[2].img_id = VENDOR_GFX_GRPH_IMG_ID_C; //dst UV
	param.images[2].dram_addr = (yuv_pa + 1000*200);
	param.images[2].lineoffset = 1000;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_color_transform_1000_200_1.yuv420", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_color_transform_1000_200_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)yuv_va, yuv_size);
	close(fd);
	if(len != (int)yuv_size){
		printf("fail to write /mnt/sd/hd_gfx_color_transform_1000_200_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_color_transform_1000_200_1.yuv420\n");

	ret = HD_OK;

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

//transform a 1920x1080 yuv420 3 planar image to a yuv420 2 planar image
static HD_RESULT test_4203P_to_4202P(UINT32 buf_pa, UINT32 buf_size)
{
	int                            fd;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	void                           *va;
	HD_RESULT                      ret;
	HD_VIDEO_FRAME                 frame;
	UINT32                         file_size, packed_size;
	int                            len;
	int                            w = 1920, h = 1080;
	UINT32                         file_pa, file_va, packed_pa, packed_va;

	//calculate planar image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420_PLANAR;
	frame.dim.h   = h;
	frame.loff[0] = w;
	frame.loff[1] = w;
	frame.loff[2] = w;
	file_size = hd_common_mem_calc_buf_size(&frame);
	if(!file_size){
		printf("hd_common_mem_calc_buf_size() fail to calculate file_size size\n");
		return -1;
	}

	//calculate packed image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = h;
	frame.loff[0] = w;
	frame.loff[1] = w;
	packed_size = hd_common_mem_calc_buf_size(&frame);
	if(!packed_size){
		printf("hd_common_mem_calc_buf_size() fail to calculate packed size\n");
		return -1;
	}

	if((file_size + packed_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (file_size + packed_size), buf_size);
		return -1;
	}

	file_pa = buf_pa;
	packed_pa = file_pa + file_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	file_va = (UINT32)va;
	packed_va = file_va + file_size;

	//load test image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv4203P.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv4203P.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)file_va, file_size);
	close(fd);
	if(len != (int)file_size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv4203P.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_A_COPY;
	param.format = VENDOR_GFX_GRPH_FORMAT_8BITS;
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
	param.images[0].dram_addr = file_pa;
	param.images[0].lineoffset = w;
	param.images[0].width = w;
	param.images[0].height = h;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
	param.images[1].dram_addr = packed_pa;
	param.images[1].lineoffset = w;
	param.images[1].width = w;
	param.images[1].height = h;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_PACKING;
	param.format = VENDOR_GFX_GRPH_FORMAT_8BITS;
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
	param.images[0].dram_addr = (file_pa + w*h);
	param.images[0].lineoffset = w;
	param.images[0].width = w;
	param.images[0].height = h;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_B;
	param.images[1].dram_addr = (file_pa + w*h*3/2);
	param.images[1].lineoffset = w;
	param.images[2].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
	param.images[2].dram_addr = (packed_pa + w*h);
	param.images[2].lineoffset = w;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_3p_to_2p_1920_1080_1_yuv420.dat", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_3p_to_2p_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)packed_va, packed_size);
	close(fd);
	if(len != (int)packed_size){
		printf("fail to write /mnt/sd/hd_gfx_3p_to_2p_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_3p_to_2p_1920_1080_1_yuv420.dat\n");

	ret = HD_OK;

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

static UINT32 argb_to_ycbcr(UINT32 argb)
{
#define cst_prom0   21
#define cst_prom1   79
#define cst_prom2   29
#define cst_prom3   43
#define RGB_GET_Y(r,g,b)    (((INT32)g) + ((cst_prom1 * (((INT32)r)-((INT32)g))) >> 8) + ((cst_prom2 * (((INT32)b)-((INT32)g))) >> 8) )
#define RGB_GET_U(r,g,b)    (128 + ((cst_prom3 * (((INT32)g)-((INT32)r))) >> 8) + ((((INT32)b)-((INT32)g)) >> 1) )
#define RGB_GET_V(r,g,b)    (128 + ((cst_prom0 * (((INT32)g)-((INT32)b))) >> 8) + ((((INT32)r)-((INT32)g)) >> 1) )

	UINT32 ycbcr = 0;

	ycbcr = RGB_GET_V((argb & 0x0FF0000) >> 16, (argb & 0x0FF00) >> 8, argb & 0x0FF);
	ycbcr <<= 8;
	ycbcr += RGB_GET_U((argb & 0x0FF0000) >> 16, (argb & 0x0FF00) >> 8, argb & 0x0FF);
	ycbcr <<= 8;
	ycbcr += RGB_GET_Y((argb & 0x0FF0000) >> 16, (argb & 0x0FF00) >> 8, argb & 0x0FF);

	return ycbcr;
}

static HD_RESULT test_draw_rectangle(UINT32 buf_pa, UINT32 buf_size)
{
	int                            fd;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	void                           *va;
	HD_RESULT                      ret;
	HD_VIDEO_FRAME                 frame;
	UINT32                         size;
	int                            len;
	UINT32                         y, cb, cr, cbcr;
	UINT32                         i, color, fill_data[3];

	//calculate image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	frame.loff[1] = 1920;
	size = hd_common_mem_calc_buf_size(&frame);
	if(!size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}
	if(size > buf_size){
		printf("required size(%d) > allocated size(%d)\n", size, buf_size);
		return -1;
	}

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	//load image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv420.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		printf("video_frm_1920_1080_1_yuv420.dat is in SDK/code/hdal/samples/pattern/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	//coverity[string_null_argument]
	len = read(fd, va, size);
	close(fd);
	if(len != (int)size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	color = argb_to_ycbcr(0x0FF00);
	y = (color & 0x000000FF);
	y = (y + (y << 8) + (y << 16) + (y << 24));
	cb = ((color & 0x0000FF00) >> 8);
	cb = cb + (cb << 8) + (cb << 16) + (cb << 24);
	cr = ((color & 0x00FF0000) >> 16);
	cr = cr + (cr << 8) + (cr << 16) + (cr << 24);
	cbcr = ((color & 0x0000FF00) >> 8) + ((color & 0x00FF0000) >> 8) ;
	cbcr = cbcr + (cbcr << 16);
	fill_data[0] = y;
	fill_data[1] = cbcr;

	for(i = 0 ; i < 2 ; ++i){
		memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
		param.command = VENDOR_GFX_GRPH_CMD_TEXT_COPY;
		param.format = VENDOR_GFX_GRPH_FORMAT_8BITS;
		param.property[0].en = 1;
		param.property[0].id = VENDOR_GFX_GRPH_PROPERTY_ID_NORMAL;
		param.property[0].data.property = fill_data[i];
		param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
		if(i == 0)
			param.images[0].dram_addr = (buf_pa + 500*1920 + 1000);
		else
			param.images[0].dram_addr = (buf_pa + 250*1920 + 1000 + 1920*1080);
		param.images[0].lineoffset = 1920;
		param.images[0].width = 500;
		if(i == 0)
			param.images[0].height = 250;
		else
			param.images[0].height = 125;

		ret = vendor_gfx_grph_trigger(0, &param);
		if(ret != HD_OK){
			printf("vendor_gfx_grph_trigger fail=%d\n", ret);
			goto exit;
		}
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_draw_rect_1920_1080_1.yuv420", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_draw_rect_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, va, size);
	close(fd);
	if(len != (int)size){
		printf("fail to write /mnt/sd/hd_gfx_draw_rect_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_draw_rect_1920_1080_1.yuv420\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

//use hardware engine to minus two 16bits arrays of 1024 elements
static HD_RESULT test_arithmetic(UINT32 buf_pa, UINT32 buf_size)
{
	void                           *va;
	UINT32                         op1_pa, op1_va, op2_pa, op2_va, out_pa, out_va;
	int                            i;
	UINT32                         size = 1024;
	HD_RESULT                      ret = HD_ERR_NOT_SUPPORT;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	
	if((size * 3 * sizeof(short)) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (size * 3 * sizeof(short)), buf_size);
		return -1;
	}
	
	op1_pa = buf_pa;
	op2_pa = op1_pa + (size * sizeof(short));
	out_pa = op2_pa + (size * sizeof(short));
	
	//map physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	op1_va = (UINT32)va;
	op2_va = op1_va + (size * sizeof(short));
	out_va = op2_va + (size * sizeof(short));
	
	//generate random data
	for(i = 0 ; i < (int)size ; ++i){
		((short*)op1_va)[i] = 6000 + (i & 32767);
		((short*)op2_va)[i] = (i & 32767);
	}
	memset((void*)out_va, 0, size * sizeof(short));

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_MINUS_SHF;
	param.format = VENDOR_GFX_GRPH_FORMAT_16BITS;
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
	param.images[0].dram_addr = op1_pa;
	param.images[0].lineoffset = size*2;
	param.images[0].width = size*2;
	param.images[0].height = 1;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_B;
	param.images[1].dram_addr = op2_pa;
	param.images[1].lineoffset = size*2;
	param.images[2].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
	param.images[2].dram_addr = out_pa;
	param.images[2].lineoffset = size*2;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}
	
	//check data integrity
	for(i = 0 ; i < (int)size ; ++i)
		if(((short*)out_va)[i] != 6000){
			printf("out[%d] is %d, not 6000. op1 is %d, op2 is %d\n", i, 
				((short*)out_va)[i], ((short*)op1_va)[i], ((short*)op2_va)[i]);
			goto exit;
		}
		
	printf("minus operation of two 16bits arrays ok\n");
	ret = HD_OK;

exit:
		
	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);
	
	return ret;
}

static HD_RESULT test_draw_frame(UINT32 buf_pa, UINT32 buf_size)
{
	int                            fd;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	void                           *va;
	HD_RESULT                      ret;
	HD_VIDEO_FRAME                 frame;
	UINT32                         size;
	int                            len;
	UINT32                         i, color, thickness = 10;

	//calculate image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	frame.loff[1] = 1920;
	size = hd_common_mem_calc_buf_size(&frame);
	if(!size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}
	if(size > buf_size){
		printf("required size(%d) > allocated size(%d)\n", size, buf_size);
		return -1;
	}

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	//load image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv420.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		printf("video_frm_1920_1080_1_yuv420.dat is in SDK/code/hdal/samples/pattern/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	//coverity[string_null_argument]
	len = read(fd, va, size);
	close(fd);
	if(len != (int)size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	color = argb_to_ycbcr(0x0FF00);

	for(i = 0 ; i < 2 ; ++i){
		memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
		param.command = VENDOR_GFX_GRPH_CMD_VCOV;
		if(i == 0)
			param.format = VENDOR_GFX_GRPH_FORMAT_8BITS;
		else
			param.format = VENDOR_GFX_GRPH_FORMAT_16BITS;
		param.property[0].en = 1;
		param.property[0].id = VENDOR_GFX_GRPH_PROPERTY_ID_YUVFMT;
		param.property[0].data.property = 1;
		param.property[1].en = 1;
		param.property[1].id = VENDOR_GFX_GRPH_PROPERTY_ID_QUAD_PTR;
		param.property[1].data.quad.top_left.x = 1000;
		param.property[1].data.quad.top_left.y = 500;
		param.property[1].data.quad.top_right.x = 1500;
		param.property[1].data.quad.top_right.y = 500;
		param.property[1].data.quad.bottom_right.x = 1500;
		param.property[1].data.quad.bottom_right.y = 750;
		param.property[1].data.quad.bottom_left.x = 1000;
		param.property[1].data.quad.bottom_left.y = 750;
		param.property[1].data.quad.blend_en = 1;
		param.property[1].data.quad.alpha = 0xFF;
		param.property[1].data.quad.mosaic_width = 0;
		param.property[1].data.quad.mosaic_height = 0;
		if(i == 0){
			param.property[2].en = 1;
			param.property[2].id = VENDOR_GFX_GRPH_PROPERTY_ID_NORMAL;
			param.property[2].data.property = (color & 0x000000FF);
			param.property[3].en = 1;
			param.property[3].id = VENDOR_GFX_GRPH_PROPERTY_ID_QUAD_INNER_PTR;
			param.property[3].data.quad.top_left.x = param.property[1].data.quad.top_left.x + thickness;
			param.property[3].data.quad.top_left.y = param.property[1].data.quad.top_left.y + thickness;
			param.property[3].data.quad.top_right.x = param.property[1].data.quad.top_right.x - thickness;
			param.property[3].data.quad.top_right.y = param.property[1].data.quad.top_right.y + thickness;
			param.property[3].data.quad.bottom_right.x = param.property[1].data.quad.bottom_right.x - thickness;
			param.property[3].data.quad.bottom_right.y = param.property[1].data.quad.bottom_right.y - thickness ;
			param.property[3].data.quad.bottom_left.x = param.property[1].data.quad.bottom_left.x + thickness;
			param.property[3].data.quad.bottom_left.y = param.property[1].data.quad.bottom_left.y - thickness;
			param.property[3].data.quad.blend_en = 1;
			param.property[3].data.quad.alpha = 0xFF;
			param.property[3].data.quad.mosaic_width = 0;
			param.property[3].data.quad.mosaic_height = 0;
		}else{
			param.property[2].en = 1;
			param.property[2].id = VENDOR_GFX_GRPH_PROPERTY_ID_U;
			param.property[2].data.property = ((color & 0x0000FF00) >> 8);
			param.property[3].en = 1;
			param.property[3].id = VENDOR_GFX_GRPH_PROPERTY_ID_V;
			param.property[3].data.property = ((color & 0x00FF0000) >> 16);
			param.property[4].en = 1;
			param.property[4].id = VENDOR_GFX_GRPH_PROPERTY_ID_QUAD_INNER_PTR;
			param.property[4].data.quad.top_left.x = param.property[1].data.quad.top_left.x + thickness;
			param.property[4].data.quad.top_left.y = param.property[1].data.quad.top_left.y + thickness;
			param.property[4].data.quad.top_right.x = param.property[1].data.quad.top_right.x - thickness;
			param.property[4].data.quad.top_right.y = param.property[1].data.quad.top_right.y + thickness;
			param.property[4].data.quad.bottom_right.x = param.property[1].data.quad.bottom_right.x - thickness;
			param.property[4].data.quad.bottom_right.y = param.property[1].data.quad.bottom_right.y - thickness ;
			param.property[4].data.quad.bottom_left.x = param.property[1].data.quad.bottom_left.x + thickness;
			param.property[4].data.quad.bottom_left.y = param.property[1].data.quad.bottom_left.y - thickness;
			param.property[4].data.quad.blend_en = 1;
			param.property[4].data.quad.alpha = 0xFF;
			param.property[4].data.quad.mosaic_width = 0;
			param.property[4].data.quad.mosaic_height = 0;
		}
		param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
		if(i == 0)
			param.images[0].dram_addr = buf_pa;
		else
			param.images[0].dram_addr = (buf_pa + i*1920*1080);
		param.images[0].lineoffset = 1920;
		param.images[0].width = 1920;
		if(i == 0)
			param.images[0].height = 1080;
		else
			param.images[0].height = 540;
		param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
		if(i == 0)
			param.images[1].dram_addr = buf_pa;
		else
			param.images[1].dram_addr = (buf_pa + i*1920*1080);
		param.images[1].lineoffset = 1920;

		ret = vendor_gfx_grph_trigger(1, &param);
		if(ret != HD_OK){
			printf("vendor_gfx_grph_trigger fail=%d\n", ret);
			goto exit;
		}
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_draw_frame_1920_1080_1.yuv420", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_draw_frame_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, va, size);
	close(fd);
	if(len != (int)size){
		printf("fail to write /mnt/sd/hd_gfx_draw_frame_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_draw_frame_1920_1080_1.yuv420\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

static HD_RESULT test_color_key(UINT32 buf_pa, UINT32 buf_size)
{
	int                            fd;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	void                           *va;
	HD_RESULT                      ret;
	HD_VIDEO_FRAME                 frame;
	UINT32                         src_size, src_pa, src_va, dst_size, dst_pa, dst_va;
	int                            len;

	//calculate logo's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_ARGB1555;
	frame.dim.h   = 200;
	frame.loff[0] = 1000;
	frame.loff[1] = 1000;
	src_size = hd_common_mem_calc_buf_size(&frame);
	if(!src_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	//calculate background's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_ARGB1555;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	frame.loff[1] = 1920;
	dst_size = hd_common_mem_calc_buf_size(&frame);
	if(!dst_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	if((src_size + dst_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (src_size + dst_size), buf_size);
		return -1;
	}

	src_pa = buf_pa;
	dst_pa = src_pa + src_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	src_va = (UINT32)va;
	dst_va = src_va + src_size;

	//load logo from sd card
	fd = open("/mnt/sd/video_frm_1000_200_colorkey_argb1555.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1000_200_colorkey_argb1555.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)src_va, src_size);
	close(fd);
	if(len != (int)src_size){
		printf("fail to read /mnt/sd/video_frm_1000_200_colorkey_argb1555.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	//load background image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_argb1555.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_argb1555.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_argb1555.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_COLOR_EQ;
	param.format = VENDOR_GFX_GRPH_FORMAT_16BITS;
	param.property[0].en = 1;
	param.property[0].id = VENDOR_GFX_GRPH_PROPERTY_ID_NORMAL;
	param.property[0].data.property = 0;
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
	param.images[0].dram_addr = dst_pa + (300*1920+500)*2;
	param.images[0].lineoffset = 3840;
	param.images[0].width = 2000;
	param.images[0].height = 200;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_B;
	param.images[1].dram_addr = src_pa;
	param.images[1].lineoffset = 2000;
	param.images[1].width = 2000;
	param.images[1].height = 200;
	param.images[2].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
	param.images[2].dram_addr = dst_pa + (300*1920+500)*2;
	param.images[2].lineoffset = 3840;
	param.images[2].width = 3840;
	param.images[2].height = 1080;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_colorkey_1920_1080_1.argb1555", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_colorkey_1920_1080_1.argb1555\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to write /mnt/sd/hd_gfx_colorkey_1920_1080_1.argb1555\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_colorkey_1920_1080_1.argb1555\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

static HD_RESULT test_color_key_less(UINT32 buf_pa, UINT32 buf_size)
{
	int                            fd;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	void                           *va;
	HD_RESULT                      ret;
	HD_VIDEO_FRAME                 frame;
	UINT32                         src_size, src_pa, src_va, dst_size, dst_pa, dst_va;
	int                            len;

	//calculate logo's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_Y8;
	frame.dim.h   = 200;
	frame.loff[0] = 1000;
	src_size = hd_common_mem_calc_buf_size(&frame);
	if(!src_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	//calculate background's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_Y8;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	dst_size = hd_common_mem_calc_buf_size(&frame);
	if(!dst_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	if((src_size + dst_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (src_size + dst_size), buf_size);
		return -1;
	}

	src_pa = buf_pa;
	dst_pa = src_pa + src_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	src_va = (UINT32)va;
	dst_va = src_va + src_size;

	//load logo from sd card
	fd = open("/mnt/sd/video_frm_1000_200_colorkey_I8.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1000_200_colorkey_I8.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)src_va, src_size);
	close(fd);
	if(len != (int)src_size){
		printf("fail to read /mnt/sd/video_frm_1000_200_colorkey_I8.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	//load background image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_I8.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_I8.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_I8.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_COLOR_LE;
	param.format = VENDOR_GFX_GRPH_FORMAT_8BITS;
	param.property[0].en = 1;
	param.property[0].id = VENDOR_GFX_GRPH_PROPERTY_ID_NORMAL;
	param.property[0].data.property = VENDOR_GFX_GRPH_COLOR_KEY_PROPTY(129);
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
	param.images[0].dram_addr = dst_pa + (300*1920+500);
	param.images[0].lineoffset = 1920;
	param.images[0].width = 1000;
	param.images[0].height = 200;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_B;
	param.images[1].dram_addr = src_pa;
	param.images[1].lineoffset = 1000;
	param.images[1].width = 1000;
	param.images[1].height = 200;
	param.images[2].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
	param.images[2].dram_addr = dst_pa + (300*1920+500);
	param.images[2].lineoffset = 1920;
	param.images[2].width = 1000;
	param.images[2].height = 200;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_colorkey_le_1920_1080_1.I8", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_colorkey_le_1920_1080_1.I8\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to write /mnt/sd/hd_gfx_colorkey_le_1920_1080_1.I8\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_colorkey_le_1920_1080_1.I8\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

static HD_RESULT test_and(UINT32 buf_pa, UINT32 buf_size)
{
	void                           *va;
	UINT32                         op1_pa, op1_va, op2_pa, op2_va, out_pa, out_va;
	int                            i;
	UINT32                         size = 1024;
	HD_RESULT                      ret = HD_ERR_NOT_SUPPORT;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	
	if((size * 3) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (size * 3), buf_size);
		return -1;
	}
	
	op1_pa = buf_pa;
	op2_pa = op1_pa + size;
	out_pa = op2_pa + size;
	
	//map physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	op1_va = (UINT32)va;
	op2_va = op1_va + size;
	out_va = op2_va + size;
	
	//generate random data
	for(i = 0 ; i < (int)size ; ++i){
		((unsigned char*)op1_va)[i] = i;
		((unsigned char*)op2_va)[i] = (255-i);
	}
	memset((void*)out_va, 0, size);

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_A_AND_B;
	param.format = VENDOR_GFX_GRPH_FORMAT_8BITS;
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
	param.images[0].dram_addr = op1_pa;
	param.images[0].lineoffset = size;
	param.images[0].width = size;
	param.images[0].height = 1;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_B;
	param.images[1].dram_addr = op2_pa;
	param.images[1].lineoffset = size;
	param.images[2].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
	param.images[2].dram_addr = out_pa;
	param.images[2].lineoffset = size;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}
	
	//check data integrity
	for(i = 0 ; i < (int)size ; ++i)
		if(((unsigned char*)out_va)[i] != (((unsigned char*)op1_va)[i] & ((unsigned char*)op2_va)[i])){
			printf("out[%d] is %x, not op1(%x) & op2(%x)\n", i, 
				((unsigned char*)out_va)[i], ((unsigned char*)op1_va)[i], ((unsigned char*)op2_va)[i]);
			ret = HD_ERR_NG;
			goto exit;
		}
		
	printf("and ok\n");
	ret = HD_OK;

exit:
		
	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);
	
	return ret;
}

static HD_RESULT test_text_or(UINT32 buf_pa, UINT32 buf_size)
{
	void                           *va;
	UINT32                         op_pa, op_va, out_pa, out_va;
	int                            i;
	UINT32                         size = 1024;
	HD_RESULT                      ret = HD_ERR_NOT_SUPPORT;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	
	if((size * 2) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (size * 2), buf_size);
		return -1;
	}
	
	op_pa = buf_pa;
	out_pa = op_pa + size;
	
	//map physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	op_va = (UINT32)va;
	out_va = op_va + size;
	
	//generate random data
	for(i = 0 ; i < (int)size ; ++i)
		((unsigned char*)op_va)[i] = i;
	memset((void*)out_va, 0, size);

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_TEXT_OR_A;
	param.format = VENDOR_GFX_GRPH_FORMAT_8BITS;
	param.property[0].en = 1;
	param.property[0].id = VENDOR_GFX_GRPH_PROPERTY_ID_NORMAL;
	param.property[0].data.property = 0x80808080;
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
	param.images[0].dram_addr = op_pa;
	param.images[0].lineoffset = size;
	param.images[0].width = size;
	param.images[0].height = 1;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
	param.images[1].dram_addr = out_pa;
	param.images[1].lineoffset = size;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}
	
	//check data integrity
	for(i = 0 ; i < (int)size ; ++i)
		if(((unsigned char*)out_va)[i] != (((unsigned char*)op_va)[i] | 0x80)){
			printf("out[%d] is %u, not %u | 0x80\n", i, 
				((unsigned char*)out_va)[i], ((unsigned char*)op_va)[i]);
			ret = HD_ERR_NG;
			goto exit;
		}
		
	printf("text or ok\n");
	ret = HD_OK;

exit:
		
	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);
	
	return ret;
}

static HD_RESULT test_depack(UINT32 buf_pa, UINT32 buf_size)
{
	int                            fd;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	void                           *va;
	HD_RESULT                      ret;
	UINT32                         src_size, src_pa, src_va, dst_size, dst_pa, dst_va;
	UINT32                         u_size, u_pa, v_size, v_pa;
	int                            len;
	
	src_size = 1920*1080*3/2;
	dst_size = src_size;
	u_size = 1920*1080/4;
	v_size = 1920*1080/4;

	if((src_size + dst_size + u_size + v_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (src_size + dst_size + u_size + v_size), buf_size);
		return -1;
	}

	src_pa = buf_pa;
	dst_pa = src_pa + src_size;
	u_pa   = dst_pa + dst_size;
	v_pa   = u_pa   + u_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	src_va = (UINT32)va;
	dst_va = src_va + src_size;

	//load background image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv420.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)src_va, src_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	//depack uv plane
	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_DEPACKING;
	param.format = VENDOR_GFX_GRPH_FORMAT_16BITS;
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
	param.images[0].dram_addr = src_pa + (1920*1080);
	param.images[0].lineoffset = 1920;
	param.images[0].width = 1920;
	param.images[0].height = 540;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_B;
	param.images[1].dram_addr = u_pa;
	param.images[1].lineoffset = 960;
	param.images[2].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
	param.images[2].dram_addr = v_pa;
	param.images[2].lineoffset = 960;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}
	
	//copy y plane
	memcpy((void*)dst_va, (void*)src_va, 1920*1080);

	//pack uv plane
	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_PACKING;
	param.format = VENDOR_GFX_GRPH_FORMAT_8BITS;
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
	param.images[0].dram_addr = u_pa;
	param.images[0].lineoffset = 960;
	param.images[0].width = 960;
	param.images[0].height = 540;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_B;
	param.images[1].dram_addr = v_pa;
	param.images[1].lineoffset = 960;
	param.images[2].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
	param.images[2].dram_addr = dst_pa + (1920*1080);
	param.images[2].lineoffset = 1920;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_depack_1920_1080_1.yuv420", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_depack_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to write /mnt/sd/hd_gfx_depack_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_depack_1920_1080_1.yuv420\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

static HD_RESULT test_1d_lut(UINT32 buf_pa, UINT32 buf_size)
{
	void                           *va;
	UINT32                         op_pa, op_va, lut_pa, lut_va, out_pa, out_va;
	int                            i;
	UINT32                         size = 1024;
	HD_RESULT                      ret = HD_ERR_NOT_SUPPORT;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	
	if((size * 3) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (size * 2), buf_size);
		return -1;
	}
	
	op_pa = buf_pa;
	lut_pa = op_pa + size;
	out_pa = lut_pa + size;
	
	//map physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	op_va = (UINT32)va;
	lut_va = op_va + size;
	out_va = lut_va + size;
	
	//generate random data
	for(i = 0 ; i < (int)size ; ++i){
		((unsigned char*)op_va)[i] = i;
		((unsigned char*)lut_va)[i] = (255-i);
	}
	memset((void*)out_va, 0, size);

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_1D_LUT;
	param.format = VENDOR_GFX_GRPH_FORMAT_8BITS;
	param.property[0].en = 1;
	param.property[0].id = VENDOR_GFX_GRPH_PROPERTY_ID_LUT_BUF;
	param.property[0].data.property = lut_pa;
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
	param.images[0].dram_addr = op_pa;
	param.images[0].lineoffset = size;
	param.images[0].width = size;
	param.images[0].height = 1;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
	param.images[1].dram_addr = out_pa;
	param.images[1].lineoffset = size;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}
	
	//check data integrity
	for(i = 0 ; i < (int)size ; ++i)
		if(((unsigned char*)out_va)[i] != (255 - ((unsigned char*)op_va)[i])){
			printf("out[%d] is %u, not 255 - %u\n", i, 
				((unsigned char*)out_va)[i], ((unsigned char*)op_va)[i]);
			ret = HD_ERR_NG;
			goto exit;
		}
		
	printf("1d lut ok\n");
	ret = HD_OK;

exit:
		
	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);
	
	return ret;
}

static HD_RESULT test_color_key_more(UINT32 buf_pa, UINT32 buf_size)
{
	int                            fd;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	void                           *va;
	HD_RESULT                      ret;
	HD_VIDEO_FRAME                 frame;
	UINT32                         src_size, src_pa, src_va, dst_size, dst_pa, dst_va;
	int                            len;

	//calculate logo's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_Y8;
	frame.dim.h   = 200;
	frame.loff[0] = 1000;
	src_size = hd_common_mem_calc_buf_size(&frame);
	if(!src_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	//calculate background's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_Y8;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	dst_size = hd_common_mem_calc_buf_size(&frame);
	if(!dst_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	if((src_size + dst_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (src_size + dst_size), buf_size);
		return -1;
	}

	src_pa = buf_pa;
	dst_pa = src_pa + src_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	src_va = (UINT32)va;
	dst_va = src_va + src_size;

	//load logo from sd card
	fd = open("/mnt/sd/video_frm_1000_200_colorkey_I8.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1000_200_colorkey_I8.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)src_va, src_size);
	close(fd);
	if(len != (int)src_size){
		printf("fail to read /mnt/sd/video_frm_1000_200_colorkey_I8.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	//load background image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_I8.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_I8.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_I8.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_COLOR_MR;
	param.format = VENDOR_GFX_GRPH_FORMAT_8BITS;
	param.property[0].en = 1;
	param.property[0].id = VENDOR_GFX_GRPH_PROPERTY_ID_NORMAL;
	param.property[0].data.property = VENDOR_GFX_GRPH_COLOR_KEY_PROPTY(1);
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
	param.images[0].dram_addr = dst_pa + (300*1920+500);
	param.images[0].lineoffset = 1920;
	param.images[0].width = 1000;
	param.images[0].height = 200;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_B;
	param.images[1].dram_addr = src_pa;
	param.images[1].lineoffset = 1000;
	param.images[1].width = 1000;
	param.images[1].height = 200;
	param.images[2].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
	param.images[2].dram_addr = dst_pa + (300*1920+500);
	param.images[2].lineoffset = 1920;
	param.images[2].width = 1000;
	param.images[2].height = 200;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_colorkey_mr_1920_1080_1.I8", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_colorkey_mr_1920_1080_1.I8\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to write /mnt/sd/hd_gfx_colorkey_mr_1920_1080_1.I8\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_colorkey_mr_1920_1080_1.I8\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

static HD_RESULT test_minus_shf_abs(UINT32 buf_pa, UINT32 buf_size)
{
	void                           *va;
	UINT32                         op1_pa, op1_va, op2_pa, op2_va, out_pa, out_va;
	int                            i;
	UINT32                         size = 1024;
	HD_RESULT                      ret = HD_ERR_NOT_SUPPORT;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	
	if((size * 3 * sizeof(short)) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (size * 3 * sizeof(short)), buf_size);
		return -1;
	}
	
	op1_pa = buf_pa;
	op2_pa = op1_pa + (size * sizeof(short));
	out_pa = op2_pa + (size * sizeof(short));
	
	//map physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	op1_va = (UINT32)va;
	op2_va = op1_va + (size * sizeof(short));
	out_va = op2_va + (size * sizeof(short));
	
	//generate random data
	for(i = 0 ; i < (int)size ; ++i){
		((unsigned short*)op1_va)[i] = (32767 + 512 - i);
		((unsigned short*)op2_va)[i] = 32767 + i;
	}
	memset((void*)out_va, 0, size * sizeof(short));

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_MINUS_SHF_ABS;
	param.format = VENDOR_GFX_GRPH_FORMAT_16BITS;
	param.property[0].en = 1;
	param.property[0].id = VENDOR_GFX_GRPH_PROPERTY_ID_NORMAL;
	param.property[0].data.property = VENDOR_GFX_GRPH_SUB_PROPTY(1);
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
	param.images[0].dram_addr = op1_pa;
	param.images[0].lineoffset = size*2;
	param.images[0].width = size*2;
	param.images[0].height = 1;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_B;
	param.images[1].dram_addr = op2_pa;
	param.images[1].lineoffset = size*2;
	param.images[2].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
	param.images[2].dram_addr = out_pa;
	param.images[2].lineoffset = size*2;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}
	
	//check data integrity
	for(i = 0 ; i < (int)size ; ++i){
		int op1 = ((unsigned short*)op1_va)[i];
		int op2 = ((unsigned short*)op2_va)[i];
		int out = op1 - (op2>>1);
		short v = out;
		if(v < 0)
			v = -v;
		if(((unsigned short*)out_va)[i] != (unsigned short)v){
			printf("out[%d] is %u, not %u. op1 is %d, op2 is %d\n", i, 
				((unsigned short*)out_va)[i], (unsigned short)v, op1, op2);
			ret = HD_ERR_NG;
			goto exit;
		}
	}
		
	printf("minus shf abs operation of two 16bits arrays ok\n");
	ret = HD_OK;

exit:
		
	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);
	
	return ret;
}

static HD_RESULT test_rgbyuv_colorkey(UINT32 buf_pa, UINT32 buf_size)
{
	int                            fd;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	void                           *va;
	HD_RESULT                      ret;
	HD_VIDEO_FRAME                 frame;
	UINT32                         src_size, src_pa, src_va, dst_size, dst_pa, dst_va;
	int                            len;

	//calculate logo's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_ARGB4444;
	frame.dim.h   = 200;
	frame.loff[0] = 1000;
	frame.loff[1] = 1000;
	src_size = hd_common_mem_calc_buf_size(&frame);
	if(!src_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	//calculate background's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	frame.loff[1] = 1920;
	dst_size = hd_common_mem_calc_buf_size(&frame);
	if(!dst_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	if((src_size + dst_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (src_size + dst_size), buf_size);
		return -1;
	}

	src_pa = buf_pa;
	dst_pa = src_pa + src_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	src_va = (UINT32)va;
	dst_va = src_va + src_size;

	//load logo from sd card
	fd = open("/mnt/sd/video_frm_1000_200_colorkey_argb4444.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1000_200_colorkey_argb4444.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)src_va, src_size);
	close(fd);
	if(len != (int)src_size){
		printf("fail to read /mnt/sd/video_frm_1000_200_colorkey_argb4444.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	//load background image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv420.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_RGBYUV_COLORKEY;
	param.format = VENDOR_GFX_GRPH_FORMAT_16BITS_ARGB4444_RGB;
	param.property[0].en = 1;
	param.property[0].id = VENDOR_GFX_GRPH_PROPERTY_ID_YUVFMT;
	param.property[0].data.property = 1;
	param.property[1].en = 1;
	param.property[1].id = VENDOR_GFX_GRPH_PROPERTY_ID_R;
	param.property[1].data.property = 0;
	param.property[2].en = 1;
	param.property[2].id = VENDOR_GFX_GRPH_PROPERTY_ID_G;
	param.property[2].data.property = 0;
	param.property[3].en = 1;
	param.property[3].id = VENDOR_GFX_GRPH_PROPERTY_ID_B;
	param.property[3].data.property = 0x0F;
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
	param.images[0].dram_addr = src_pa;
	param.images[0].lineoffset = 2000;
	param.images[0].width = 2000;
	param.images[0].height = 200;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_B;
	param.images[1].dram_addr = dst_pa + (300*1920+500);
	param.images[1].lineoffset = 1920;
	param.images[1].width = 1920;
	param.images[1].height = 1080;
	param.images[2].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
	param.images[2].dram_addr = dst_pa + (1920*1080) + (300*1920/2+500);
	param.images[2].lineoffset = 1920;
	param.images[2].width = 1920;
	param.images[2].height = 540;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_rgbyuv_colorkey_1920_1080_1.yuv420", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_rgbyuv_colorkey_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to write /mnt/sd/hd_gfx_rgbyuv_colorkey_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_rgbyuv_colorkey_1920_1080_1.yuv420\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

static HD_RESULT test_rgb_invert(UINT32 buf_pa, UINT32 buf_size)
{
	int                            fd;
	VENDOR_GFX_GRPH_TRIGGER_PARAM  param;
	void                           *va;
	HD_RESULT                      ret;
	HD_VIDEO_FRAME                 frame;
	UINT32                         src_size, src_pa, src_va, dst_size, dst_pa, dst_va;
	int                            len;

	//calculate logo's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_ARGB4444;
	frame.dim.h   = 200;
	frame.loff[0] = 1000;
	src_size = hd_common_mem_calc_buf_size(&frame);
	if(!src_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}
	dst_size = src_size;

	if((src_size + dst_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (src_size + dst_size), buf_size);
		return -1;
	}

	src_pa = buf_pa;
	dst_pa = src_pa + src_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	src_va = (UINT32)va;
	dst_va = src_va + src_size;

	//load logo from sd card
	fd = open("/mnt/sd/video_frm_1000_200_colorkey_argb4444.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1000_200_colorkey_argb4444.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)src_va, src_size);
	close(fd);
	if(len != (int)src_size){
		printf("fail to read /mnt/sd/video_frm_1000_200_colorkey_argb4444.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	memset(&param, 0, sizeof(VENDOR_GFX_GRPH_TRIGGER_PARAM));
	param.command = VENDOR_GFX_GRPH_CMD_RGB_INVERT;
	param.format = VENDOR_GFX_GRPH_FORMAT_16BITS_ARGB4444_RGB;
	param.property[0].en = 1;
	param.property[0].id = VENDOR_GFX_GRPH_PROPERTY_ID_INVRGB;
	param.property[0].data.property = VENDOR_GFX_GRPH_RGB_INV_PROPTY(0, 0);
	param.images[0].img_id = VENDOR_GFX_GRPH_IMG_ID_A;
	param.images[0].dram_addr = src_pa;
	param.images[0].lineoffset = 2000;
	param.images[0].width = 2000;
	param.images[0].height = 200;
	param.images[1].img_id = VENDOR_GFX_GRPH_IMG_ID_C;
	param.images[1].dram_addr = dst_pa;
	param.images[1].lineoffset = 2000;
	param.images[1].width = 2000;
	param.images[1].height = 200;

	ret = vendor_gfx_grph_trigger(0, &param);
	if(ret != HD_OK){
		printf("vendor_gfx_grph_trigger fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_rgb_invert_1000x200.argb4444", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_rgb_invert_1000x200.argb4444\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to write /mnt/sd/hd_gfx_rgb_invert_1000x200.argb4444\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_rgb_invert_1000x200.argb4444\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _GFX {

	// (1)
	HD_COMMON_MEM_VB_BLK buf_blk;
	UINT32               buf_size;
	UINT32               buf_pa;
	
} GFX;

MAIN(argc, argv)
{
    HD_RESULT    ret;
	GFX          gfx;
    INT          key;

	gfx.buf_blk = 0;
	gfx.buf_size = 16 * 1024 * 1024;
	gfx.buf_pa = 0;

	ret = hd_common_init(0);
    if(ret != HD_OK) {
        printf("common fail=%d\n", ret);
        goto exit;
    }

	ret = mem_init(&gfx.buf_blk, gfx.buf_size, &gfx.buf_pa);
    if(ret != HD_OK) {
        printf("mem fail=%d\n", ret);
        goto exit;
    }

    ret = hd_gfx_init();
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
        goto exit;
    }

	printf("Enter a to test alpha plane\n");
	printf("Enter b to test acc\n");
	printf("Enter c to test multiply div\n");
	printf("Enter d to test alpha blend\n");
	printf("Enter e to test text mul\n");
	printf("Enter f to test copy\n");
	printf("Enter g to test rotation\n");
	printf("Enter h to test color transform\n");
	printf("Enter i to test draw rectangle\n");
	printf("Enter j to test arithmetic\n");
	printf("Enter k to transform yuv420 3p to 2p\n");
	printf("Enter l to test draw frame\n");
	printf("Enter m to test colorkey\n");
	printf("Enter n to test colorkey le\n");
	printf("Enter o to test and\n");
	printf("Enter p to test text or\n");
	printf("Enter q to test depack\n");
	printf("Enter r to test 1d lut\n");
	printf("Enter s to test colorkey mr\n");
	printf("Enter t to test minus sht abs\n");
	printf("Enter u to test rgbyuv colorkey\n");
	printf("Enter v to test rgb invert\n");
	key = GETCHAR();
	if (key == 'a') {
		ret = test_alpha_plane(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("alpha plane fail=%d\n", ret);
	}else if (key == 'b') {
		ret = test_acc(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("acc fail=%d\n", ret);
	}else if (key == 'c') {
		ret = test_multiply_div(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("multiply div fail=%d\n", ret);
	}else if (key == 'd') {
		ret = test_alpha_blend(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("alpha blend fail=%d\n", ret);
	}else if (key == 'e') {
		ret = test_text_mul(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("text mul fail=%d\n", ret);
	}else if (key == 'f') {
		ret = test_copy(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("copy fail=%d\n", ret);
	}else if (key == 'g') {
		ret = test_rotation(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("rotation fail=%d\n", ret);
	}else if (key == 'h') {
		ret = test_color_transform(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("color transform fail=%d\n", ret);
	}else if (key == 'i') {
		ret = test_draw_rectangle(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("drawing rectangle fail=%d\n", ret);
	}else if (key == 'j') {
		ret = test_arithmetic(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("arithmetic fail=%d\n", ret);
	}else if (key == 'k') {
		ret = test_4203P_to_4202P(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("420 3p to 2p fail=%d\n", ret);
	}else if (key == 'l') {
		ret = test_draw_frame(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("drawing frame fail=%d\n", ret);
	}else if (key == 'm') {
		ret = test_color_key(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("color key fail=%d\n", ret);
	}else if (key == 'n') {
		ret = test_color_key_less(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("color key less fail=%d\n", ret);
	}else if (key == 'o') {
		ret = test_and(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("and fail=%d\n", ret);
	}else if (key == 'p') {
		ret = test_text_or(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("text or fail=%d\n", ret);
	}else if (key == 'q') {
		ret = test_depack(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("depack fail=%d\n", ret);
	}else if (key == 'r') {
		ret = test_1d_lut(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("1d lut fail=%d\n", ret);
	}else if (key == 's') {
		ret = test_color_key_more(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("color key more fail=%d\n", ret);
	}else if (key == 't') {
		ret = test_minus_shf_abs(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("minus shf abs fail=%d\n", ret);
	}else if (key == 'u') {
		ret = test_rgbyuv_colorkey(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("rgbyuv_colorkey fail=%d\n", ret);
	}else if (key == 'v') {
		ret = test_rgb_invert(gfx.buf_pa, gfx.buf_size);
		if(ret != HD_OK)
			printf("rgb invert fail=%d\n", ret);
	}

exit:

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
