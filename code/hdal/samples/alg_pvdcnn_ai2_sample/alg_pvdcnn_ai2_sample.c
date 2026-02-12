/**
	@brief Source file of vendor net application sample using user-space net flow.

	@file alg_pvdcnn_ai2_sample.c

	@ingroup alg_pvdcnn_ai2_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "hdal.h"
#include "hd_type.h"
#include "hd_common.h"

#include "vendor_isp.h"
#include "vendor_gfx.h"
#include "vendor_ai.h"
#include "vendor_ai_util.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "vendor_ai_cpu_postproc.h"

#include "pvdcnn_lib.h"

/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
#define DEFAULT_DEVICE          "/dev/" VENDOR_AIS_FLOW_DEV_NAME

#define MAX_INPUT_WIDTH         1920
#define MAX_INPUT_HEIGHT        1080
#define MAX_INPUT_IMAGE_SIZE    (1920*1080)

#define NN_PVDCNN_MODE        	ENABLE
#define NN_PVDCNN_PROF        	ENABLE//DISABLE
#define NN_PVDCNN_DUMP        	ENABLE//DISABLE
#define NN_PVDCNN_SAVE        	ENABLE//DISABLE
#define NN_PVDCNN_FIX_FRM     	DISABLE

#define NN_USE_DRAM2            ENABLE

#define VENDOR_AI_CFG  0x000f0000  //ai project config

typedef struct _THREAD_PARM {
    VENDOR_AIS_FLOW_MEM_PARM mem;
    CHAR   *dir;
    UINT32  start;
    UINT32  end;
} THREAD_PARM;


/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

static VENDOR_AIS_FLOW_MEM_PARM g_mem       = {0};
static HD_COMMON_MEM_VB_BLK g_blk_info[1]   = {0};

#if NN_PVDCNN_PROF
static struct timeval pvd_tstart, pvd_tend;
#endif


/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/

static int mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
    UINT32 mem_size = 0;

#if NN_PVDCNN_MODE
    mem_size += pvdcnn_calcbuffsize() + MAX_INPUT_IMAGE_SIZE*2;
#endif

    mem_cfg.pool_info[0].type = HD_COMMON_MEM_CNN_POOL;
	mem_cfg.pool_info[0].blk_size = mem_size; // fd buff
	mem_cfg.pool_info[0].blk_cnt = 1;

#if NN_USE_DRAM2
    mem_cfg.pool_info[0].ddr_id = DDR_ID1;
#else
    mem_cfg.pool_info[0].ddr_id = DDR_ID0;
#endif

	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
	}
	return ret;
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

#if NN_PVDCNN_MODE
    mem_size += pvdcnn_calcbuffsize() + MAX_INPUT_IMAGE_SIZE*2;
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

#if NN_PVDCNN_MODE
    mem_size += pvdcnn_calcbuffsize() + MAX_INPUT_IMAGE_SIZE*2;
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
static HD_RESULT mem_uninit(void)
{
	return hd_common_mem_uninit();
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

static UINT32 load_file(CHAR *p_filename, UINT32 va)
{
	FILE  *fd;
	UINT32 file_size = 0, read_size = 0;
	const UINT32 model_addr = va;

	fd = fopen(p_filename, "rb");
	if (!fd) {
		printf("cannot read %s\r\n", p_filename);
		return 0;
	}

	fseek ( fd, 0, SEEK_END );
	file_size = ALIGN_CEIL_4( ftell(fd) );
	fseek ( fd, 0, SEEK_SET );

	read_size = fread ((void *)model_addr, 1, file_size, fd);
	if (read_size != file_size) {
		printf("size mismatch, real = %d, idea = %d\r\n", (int)read_size, (int)file_size);
	}
	fclose(fd);
	return read_size;
}

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

VOID *pvdcnn_thread(VOID *arg)
{
	THREAD_PARM *p_parm = (THREAD_PARM*)arg;
	HD_RESULT ret = HD_OK;

	VENDOR_AIS_FLOW_MEM_PARM mem = p_parm->mem;
    CHAR *dir = p_parm->dir;
    UINT32 start = p_parm->start;
    UINT32 end = p_parm->end;

    UINT32 height, width, bitcount;
    UINT32 idx;
    CHAR filename[256];


#if NN_PVDCNN_PROF
    static struct timeval tstart, tend;
    static UINT64 cur_time = 0, mean_time = 0, sum_time = 0;
    static UINT32 icount = 0;
#endif

    VENDOR_AIS_FLOW_MEM_PARM input_y  = getmem(&mem, MAX_INPUT_IMAGE_SIZE);
    VENDOR_AIS_FLOW_MEM_PARM input_uv = getmem(&mem, MAX_INPUT_IMAGE_SIZE);

    UINT32 buf_size = pvdcnn_calcbuffsize();
    VENDOR_AIS_FLOW_MEM_PARM buf = getmem(&mem, buf_size);
	printf("buf_size = %d\r\n", (int)buf_size);


    UINT32 read_size = load_file("/mnt/sd/CNNLib/para/pvdcnn/nvt_model.bin", buf.va);
	printf("load model size = %d\r\n", (int)read_size);


	PVDCNN_INIT_PRMS init_prms = {0};
	init_prms.net_id = 0;
	init_prms.conf_thr = 0.395;
	init_prms.conf_thr2 = 0.4;
	init_prms.nms_thr = 0.3;

    ret = pvdcnn_init(buf, &init_prms);
	if (ret != HD_OK) {
		printf("pvdcnn_init fail\r\n");
		return 0;
	}

    for (idx = start; idx <= end; idx++) {
        sprintf(filename, "/mnt/sd/PVDCNN/%s_uv/%06ld.bmp", dir, idx);
        pvdcnn_readbmpheader(&height, &width, &bitcount, filename);
        printf("read uv image : %s %ldx%ld  bit %ld\r\n", filename, width, height, bitcount);
		if (height < 8 || height > 1080 || width < 8 || width > 1920 || bitcount != 8) {
			printf("input image dim error\r\n");
			break;
		}
        pvdcnn_readbmpbody((UINT8 *)input_uv.va, height, width, bitcount, NULL, 1, filename);

        sprintf(filename, "/mnt/sd/PVDCNN/%s_y/%06ld.bmp", dir, idx);
        pvdcnn_readbmpheader(&height, &width, &bitcount, filename);
        printf("read Y image : %s %ldx%ld  bit %ld\r\n", filename, width, height, bitcount);
		if (height < 8 || height > 1080 || width < 8 || width > 1920 || bitcount != 8) {
			printf("input image dim error\r\n");
			break;
		}
        pvdcnn_readbmpbody((UINT8 *)input_y.va, height, width, bitcount, NULL, 1, filename);

        // init image
        HD_GFX_IMG_BUF input_image;
        input_image.dim.w = width;
        input_image.dim.h = height;
        input_image.format = HD_VIDEO_PXLFMT_YUV420;
        input_image.p_phy_addr[0] = input_y.pa;
        input_image.p_phy_addr[1] = input_uv.pa;
        input_image.p_phy_addr[2] = input_uv.pa;
        input_image.lineoffset[0] = ALIGN_CEIL_4(width);
        input_image.lineoffset[1] = ALIGN_CEIL_4(width);
        input_image.lineoffset[2] = ALIGN_CEIL_4(width);

#if NN_PVDCNN_FIX_FRM
        while (1) {
#endif

#if NN_PVDCNN_PROF
	        gettimeofday(&tstart, NULL);
	        gettimeofday(&pvd_tstart, NULL);
#endif
	        ret = pvdcnn_set_img(buf, &input_image);
			if (ret != HD_OK) {
				printf("pvdcnn_set_img fail\r\n");
				break;
			}

	        ret = pvdcnn_process(buf);
			if (ret != HD_OK) {
				printf("pvdcnn_process fail\r\n");
				break;
			}

#if NN_PVDCNN_PROF
	        gettimeofday(&pvd_tend, NULL);
	        gettimeofday(&tend, NULL);
	        cur_time = (UINT64)(tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec);
	        sum_time += cur_time;
	        mean_time = sum_time/(++icount);
	        //#if (!NN_PVDCNN_FIX_FRM)
	        printf("[PVD] cur time(us): %lld, mean time(us): %lld\r\n", cur_time, mean_time);
	        //#endif
#endif

#if (NN_PVDCNN_SAVE || NN_PVDCNN_DUMP)
	    	HD_URECT ref_size = {0};
	        ref_size.w = width;
	        ref_size.h = height;
			
			static PVDCNN_RSLT rslt[16] = {0};
	        UINT32 rslt_num = pvdcnn_get_result(buf, rslt, &ref_size, 16);
#endif

#if NN_PVDCNN_DUMP
	        pvdcnn_print_rslt(rslt, rslt_num, "pvdcnn");
#endif

#if NN_PVDCNN_SAVE
			sprintf(filename, "/mnt/sd/PVDCNN/%s_rslt/%06ld.bmp", dir, idx);
			pvdcnn_draw_rslt(rslt, rslt_num, (UINT8 *)input_y.va, ALIGN_CEIL_4(width), height, width);
			pvdcnn_writebmpfile((UINT8 *)input_y.va, ALIGN_CEIL_4(width), height, width, 8, NULL, 1, filename);

	        sprintf(filename, "/mnt/sd/PVDCNN/%s_rslt/%06ld.txt", dir, idx);
			pvdcnn_write_rslt(rslt, rslt_num, filename);
#endif


#if NN_PVDCNN_FIX_FRM
			if (cur_time < 100000) {
				usleep(100000 - cur_time + (mean_time*0));
			}
		}
#endif
    }
    ret = pvdcnn_uninit(buf);
	if (ret != HD_OK) {
		printf("pvdcnn_uninit fail\r\n");
	}
    return 0;
}


int main(int argc, char *argv[])
{
	HD_RESULT ret;

    if (argc != 4) {
        printf("ERR cmd : alg_pvdcnn_ai2_sample [dir name] [start idx] [end idx]\n");
        return HD_ERR_NOT_SUPPORT;
    }
    UINT32 start_idx = atoi(argv[2]);
    UINT32 end_idx = atoi(argv[3]);

	ret = hd_common_init(0);
	if (ret != HD_OK) {
		DBG_ERR("hd_common_init fail=%d\n", ret);
		goto exit;
	}
	//set project config for AI
	hd_common_sysconfig(0, (1<<16), 0, VENDOR_AI_CFG); //enable AI engine

	// init memory
	ret = mem_init();
	if (ret != HD_OK) {
		DBG_ERR("mem_init fail=%d\n", ret);
		goto exit;
	}

	ret = get_mem_block();
	if (ret != HD_OK) {
		DBG_ERR("mem_init fail=%d\n", ret);
		goto exit;
	}

	ret = hd_videoproc_init();
	if (ret != HD_OK) {
		DBG_ERR("hd_videoproc_init fail=%d\n", ret);
		goto exit;
	}

    ret = hd_gfx_init();
	if (ret != HD_OK) {
		DBG_ERR("hd_gfx_init fail=%d\n", ret);
		goto exit;
	}

    VENDOR_AIS_FLOW_MEM_PARM local_mem = g_mem;


#if NN_PVDCNN_MODE
    THREAD_PARM pvd_thread_parm;
    pthread_t pvd_thread_id;
    UINT32 pvd_mem_size = pvdcnn_calcbuffsize() + MAX_INPUT_IMAGE_SIZE*2;

    pvd_thread_parm.mem   = getmem(&local_mem, pvd_mem_size);
    pvd_thread_parm.dir   = argv[1];
    pvd_thread_parm.start = start_idx;
    pvd_thread_parm.end   = end_idx;


    pthread_create(&pvd_thread_id, NULL, pvdcnn_thread, (VOID*)(&pvd_thread_parm));	

    pthread_join(pvd_thread_id, NULL);
#endif

exit:
    ret = hd_gfx_uninit();
	if (ret != HD_OK) {
		DBG_ERR("hd_gfx_uninit fail=%d\n", ret);
	}

	ret = hd_videoproc_uninit();
	if (ret != HD_OK) {
		DBG_ERR("hd_videoproc_uninit fail=%d\n", ret);
	}

	ret = release_mem_block();
	if (ret != HD_OK) {
		DBG_ERR("mem_uninit fail=%d\n", ret);
	}

	ret = mem_uninit();
	if (ret != HD_OK) {
		DBG_ERR("mem_uninit fail=%d\n", ret);
	}

	ret = hd_common_uninit();
	if (ret != HD_OK) {
		DBG_ERR("hd_common_uninit fail=%d\n", ret);
	}

	DBG_DUMP("network test finish...\r\n");

	return ret;
}


