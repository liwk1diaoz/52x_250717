/**
 * @file hd_common_test.c
 * @brief test common memory APIs.
 * @author Lincy Lin
 * @date in the year 2018
 */

#if defined(__LINUX)
#define _GNU_SOURCE
#include <sched.h>
#include <sys/sysinfo.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "hdal.h"
#include "hd_debug.h"
#include "vendor_common.h"
#include <kwrap/examsys.h>
#ifdef __LINUX
#include <sys/stat.h>
#include "libfdt.h"
#include "compiler.h"
#endif




#define DEBUG_MENU 		1
#define TEST_DDR2 		0
#ifdef __LINUX
#define TEST_FDT 		1
#else
#define TEST_FDT 		0
#endif


#define CHKPNT    printf("\033[37mCHK: %d, %s\033[0m\r\n", __LINE__, __func__)

static UINT32 g_pid = 0;  //0:server, 1:client

static int mem_init(UINT32 cfg)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
	UINT32                 i = 0;

	if (cfg == 0) {
		mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
		mem_cfg.pool_info[i].blk_size = 0x200;
		mem_cfg.pool_info[i].blk_cnt = 3;
		mem_cfg.pool_info[i].ddr_id = DDR_ID0;
		i+=1;
		mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
		mem_cfg.pool_info[i].blk_size = 0x200000;
		mem_cfg.pool_info[i].blk_cnt = 3;
		mem_cfg.pool_info[i].ddr_id = DDR_ID0;
		i+=1;
		mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
		mem_cfg.pool_info[i].blk_size = 0x300000;
		mem_cfg.pool_info[i].blk_cnt = 3;
		mem_cfg.pool_info[i].ddr_id = DDR_ID0;
		i+=1;
		mem_cfg.pool_info[i].type = HD_COMMON_MEM_OSG_POOL;
		mem_cfg.pool_info[i].blk_size = 0x100000;
		mem_cfg.pool_info[i].blk_cnt = 2;
		mem_cfg.pool_info[i].ddr_id = DDR_ID0;
		i+=1;
		mem_cfg.pool_info[i].type = HD_COMMON_MEM_OSG_POOL;
		mem_cfg.pool_info[i].blk_size = 0x200000;
		mem_cfg.pool_info[i].blk_cnt = 2;
		mem_cfg.pool_info[i].ddr_id = DDR_ID0;
		i+=1;
		mem_cfg.pool_info[i].type = HD_COMMON_MEM_GFX_POOL;
		mem_cfg.pool_info[i].blk_size = 0x400;
		mem_cfg.pool_info[i].blk_cnt = 255;
		mem_cfg.pool_info[i].ddr_id = DDR_ID0;
		i+=1;
#if TEST_DDR2
		mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
		mem_cfg.pool_info[i].blk_size = 0x200000;
		mem_cfg.pool_info[i].blk_cnt = 3;
		mem_cfg.pool_info[i].ddr_id = DDR_ID1;
		i+=1;
		mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
		mem_cfg.pool_info[i].blk_size = 0x300000;
		mem_cfg.pool_info[i].blk_cnt = 3;
		mem_cfg.pool_info[i].ddr_id = DDR_ID1;
		i+=1;
#endif
		ret = hd_common_mem_init(&mem_cfg);
		if (HD_OK != ret) {
			printf("hd_common_mem_init err: %d\r\n", ret);
		}
	} else {
		ret = hd_common_mem_init(NULL);
	}
	return ret;
}

static HD_RESULT mem_exit(void)
{
	return hd_common_mem_uninit();
}


static HD_RESULT test_get_block_from_common(void)
{
	HD_COMMON_MEM_VB_BLK blk;
	UINT32            pa, va;
	UINT32            blk_size;
	HD_COMMON_MEM_DDR_ID ddr_id;
	HD_COMMON_MEM_DDR_ID max_ddr;
	HD_RESULT         ret, func_ret = HD_OK;
	HD_VIDEO_FRAME    my_frame = {0};


#if TEST_DDR2
	max_ddr = DDR_ID1;
#else
	max_ddr = DDR_ID0;
#endif
	for (ddr_id=0; ddr_id <= max_ddr; ddr_id++) {
		printf("\r\ntest_get_block_from_common ddr_id %d\r\n", ddr_id);

		system("cat /proc/hdal/comm/info");

		my_frame.sign = MAKEFOURCC('V','F','R','M');
		my_frame.dim.w = 1920;
		my_frame.dim.h = 1080;
		my_frame.loff[0]= 1920;
		my_frame.loff[1]= 1920;
		my_frame.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		blk_size = hd_common_mem_calc_buf_size((void *)&my_frame);
		printf("blk_size = 0x%x\r\n", (int)blk_size);
		blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id);
		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get block fail\r\n");
			return HD_ERR_NG;
		}
		printf("blk = 0x%x\r\n", (int)blk);
		pa = hd_common_mem_blk2pa(blk);
		if (pa == 0) {
			printf("blk2pa fail, blk = 0x%x\r\n", (int)blk);
			func_ret = HD_ERR_SYS;
			goto blk2pa_err;
		}
		printf("pa = 0x%x\r\n", (int)pa);
		if (pa > 0) {
			va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, blk_size);
			if (va == 0) {
				func_ret = HD_ERR_SYS;
				goto map_err;
			}
			printf("va = 0x%x\r\n", (int)va);
			memset((void*)va, 0x11, blk_size);
			printf("va = 0x%x\r\n", (int)va);
			ret = hd_common_mem_flush_cache((void*)va, blk_size);
			printf("hd_common_mem_flush_cache ret = 0x%x\r\n", ret);
			hd_common_mem_munmap((void*)va, blk_size);
		}
		ret = vendor_common_mem_lock_block(blk);
		if (HD_OK != ret) {
			printf("lock blk fail %d\r\n", ret);
			return HD_ERR_NG;
		}
		printf("user lock block\r\n");
		system("cat /proc/hdal/comm/info");
		ret = hd_common_mem_release_block(blk);
		if (HD_OK != ret) {
			printf("release blk fail %d\r\n", ret);
			return HD_ERR_NG;
		}
	blk2pa_err:
	map_err:
		system("cat /proc/hdal/comm/info");
		ret = hd_common_mem_release_block(blk);
		if (HD_OK != ret) {
			printf("release blk fail %d\r\n", ret);
			return HD_ERR_NG;
		}
	}
	return func_ret;
}

static HD_RESULT test_get_block_from_osg(void)
{
	HD_COMMON_MEM_VB_BLK blk;
	UINT32            pa, va;
	UINT32            blk_size = 0x100000;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_RESULT         ret, func_ret = HD_OK;


	printf("\r\ntest_get_block_from_osg\r\n");
	system("cat /proc/hdal/comm/info");

	blk = hd_common_mem_get_block(HD_COMMON_MEM_OSG_POOL, blk_size, ddr_id);
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get block fail\r\n");
		return HD_ERR_NG;
	}
	printf("blk = 0x%x\r\n", (int)blk);
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		printf("blk2pa fail, blk = 0x%x\r\n", (int)blk);
		func_ret = HD_ERR_SYS;
		goto blk2pa_err;
	}
	printf("pa = 0x%x\r\n", (int)pa);
	if (pa > 0) {
		va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, blk_size);
		if (va == 0) {
			func_ret = HD_ERR_SYS;
			goto map_err;
		}
		printf("va = 0x%x\r\n", (int)va);
		memset((void*)va, 0x11, blk_size);
		printf("va = 0x%x\r\n", (int)va);
		hd_common_mem_flush_cache((void*)va, blk_size);
		hd_common_mem_munmap((void*)va, blk_size);
	}
blk2pa_err:
map_err:
	system("cat /proc/hdal/comm/info");
	ret = hd_common_mem_release_block(blk);
	if (HD_OK != ret) {
		printf("release blk fail %d\r\n", ret);
		return HD_ERR_NG;
	}
	return func_ret;
}

static HD_RESULT test_alloc(HD_COMMON_MEM_DDR_ID ddr_id)
{
	void                 *va;
	UINT32               pa;
	UINT32               size = 0x200000;
	HD_RESULT            ret;
	VENDOR_COMM_PA_TO_BLK blk_to_pa = {0};


	printf("\r\ntest_alloc\r\n");
	ret = hd_common_mem_alloc("osg1", &pa, (void **)&va, size, ddr_id);
	if (ret != HD_OK) {
		printf("err:alloc size 0x%x, ddr %d\r\n", (int)size, ddr_id+1);
		return HD_ERR_NG;
	}
	printf("pa = 0x%x, va = 0x%x\r\n", (int)pa, (int)va);
	memset((void*)va, 0x33, size);

	blk_to_pa.pa = pa;
	if (HD_OK != vendor_common_mem_get(VENDOR_COMMON_MEM_ITEM_PA_TO_BLK, &blk_to_pa)) {
		return HD_ERR_NG;
	}
	printf("blk = 0x%x\r\n", (int)blk_to_pa.blk);
	/*
	ret = vendor_common_mem_lock_block(blk_to_pa.blk);
	if (ret < 0) {
		printf("lock blk = 0x%x fail\r\n", (int)blk_to_pa.blk);
		return HD_ERR_NG;
	}
	*/
	ret = hd_common_mem_release_block(blk_to_pa.blk);
	if (ret < 0) {
		printf("release blk = 0x%x fail\r\n", (int)blk_to_pa.blk);
		return HD_ERR_NG;
	}
	system("cat /proc/hdal/comm/info");
	ret = hd_common_mem_free(pa, (void *)va);
	if (ret != HD_OK) {
		printf("err:free pa = 0x%x, va = 0x%x\r\n", (int)pa, (int)va);
		return HD_ERR_NG;
	}
	printf("free_mem\r\n\r\n");
	system("cat /proc/hdal/comm/info");
	return HD_OK;
}

static HD_RESULT test_alloc_max_pools(HD_COMMON_MEM_DDR_ID ddr_id)
{
	void                 *va;
	UINT32               pa;
	UINT32               size = 0x100;
	HD_RESULT            ret;
	UINT32               i;

	printf("\r\ntest_alloc_max_pools\r\n");
	for (i = 0;i< 4056;i++) {
		ret = hd_common_mem_alloc("osg1", &pa, (void **)&va, size, ddr_id);
		if (ret != HD_OK) {
			printf("err:alloc size 0x%x, ddr %d\r\n", (int)size, ddr_id+1);
			return HD_ERR_NG;
		}
	}
	system("cat /proc/hdal/comm/info");
	return HD_OK;
}

static HD_RESULT test_alloc_temp(HD_COMMON_MEM_DDR_ID ddr_id)
{
	void                 *va[3];
	UINT32               pa[3];
	UINT32               i, size = 0x200000;
	HD_RESULT            ret;

	printf("\r\ntest_alloc_temp\r\n");
	for (i = 0;i < 3;i++) {
		ret = hd_common_mem_alloc("NVTMPP_TEMP", &pa[i], (void **)&va[i], size, ddr_id);
		if (ret != HD_OK) {
			printf("err:alloc size 0x%x, ddr %d\r\n", (int)size, ddr_id+1);
			return HD_ERR_NG;
		}
		printf("pa[%d] = 0x%x, va[%d] = 0x%x\r\n", (int)i, (int)pa[i], (int)i, (int)va[i]);
		memset((void*)va[i], 0x33, size);
	}
	system("cat /proc/hdal/comm/info");
	for (i = 0;i < 3;i++) {
		ret = hd_common_mem_free(pa[i], (void *)va[i]);
		if (ret != HD_OK) {
			printf("err:free pa[%d] = 0x%x, va[%d] = 0x%x\r\n", (int)i, (int)pa[i], (int)i, (int)va[i]);
			return HD_ERR_NG;
		}
	}
	printf("free_mem\r\n\r\n");
	system("cat /proc/hdal/comm/info");
	return HD_OK;
}

static HD_RESULT test_va2pa(void)
{
	void                 *va1, *va2;
	UINT32               pa1, pa2;
	UINT32               size = 0x200000;
	UINT32               va1_offset = 0x1010, va2_offset = 0x2020;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_RESULT            ret, func_ret = HD_OK;
	HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};

	printf("\r\ntest_va2pa\r\n");
	ret = hd_common_mem_alloc("test1", &pa1, (void **)&va1, size, ddr_id);
	if (ret != HD_OK) {
		printf("err:alloc size 0x%x, ddr %d\r\n", (int)size, ddr_id+1);
		return HD_ERR_SYS;
	}
	printf("pa1 = 0x%x, va1 = 0x%x\r\n", (int)pa1, (int)va1);
	ret = hd_common_mem_alloc("test2", &pa2, (void **)&va2, size, ddr_id);
	if (ret != HD_OK) {
		printf("err:alloc size 0x%x, ddr %d\r\n", (int)size, ddr_id+1);
		func_ret = HD_ERR_SYS;
		goto alloc_err;
	}
	printf("pa2 = 0x%x, va2 = 0x%x\r\n", (int)pa2, (int)va2);
	system("cat /proc/hdal/comm/info");
	vir_meminfo.va = (void *)((UINT32)va1+va1_offset);
	if (hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) != HD_OK) {
		func_ret = HD_ERR_SYS;
		goto va2pa_err;
	}
	if (vir_meminfo.pa != pa1+va1_offset) {
		printf("err:vir_meminfo.pa = 0x%x != 0x%x\r\n", (int)vir_meminfo.pa, (int)(pa1+va1_offset));
		func_ret = HD_ERR_SYS;
		goto va2pa_err;
	}
	vir_meminfo.va = (void *)((UINT32)va2+va2_offset);
	if (hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) != HD_OK) {
		func_ret = HD_ERR_SYS;
		goto va2pa_err;
	}
	if (vir_meminfo.pa != pa2+va2_offset) {
		printf("err:vir_meminfo.pa = 0x%x != 0x%x\r\n", (int)vir_meminfo.pa, (int)(pa2+va2_offset));
		func_ret = HD_ERR_SYS;
		goto va2pa_err;
	}
va2pa_err:
	ret = hd_common_mem_free(pa2, (void *)va2);
	if (ret != HD_OK) {
		printf("err:free pa2 = 0x%x, va2 = 0x%x\r\n", (int)pa2, (int)va2);
	}
alloc_err:
	ret = hd_common_mem_free(pa1, (void *)va1);
	if (ret != HD_OK) {
		printf("err:free pa1 = 0x%x, va1 = 0x%x\r\n", (int)pa1, (int)va1);
	}
	return func_ret;
}



static HD_RESULT test_multi_va_map_same_phy(void)
{
	void                 *va1, *va2;
	UINT32               pa1, pa2;
	UINT32               size = 0x200000;
	UINT32               va1_offset = 0x1010, va2_offset = 0x2020;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_RESULT            ret, func_ret = HD_OK;
	HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};

	printf("\r\ntest_multi_va_map_same_phy\r\n");
	ret = hd_common_mem_alloc("test1", &pa1, (void **)&va1, size, ddr_id);
	if (ret != HD_OK) {
		printf("err:alloc size 0x%x, ddr %d\r\n", (int)size, ddr_id+1);
		return HD_ERR_SYS;
	}
	printf("pa1 = 0x%x, va1 = 0x%x\r\n", (int)pa1, (int)va1);
	pa2 = pa1;
	va2 = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa1, size);
	if (va2 == 0) {
		func_ret = HD_ERR_SYS;
		goto map_err;
	}
	printf("pa2 = 0x%x, va2 = 0x%x\r\n", (int)pa2, (int)va2);
	system("cat /proc/hdal/comm/info");
	vir_meminfo.va = (void *)((UINT32)va1+va1_offset);
	if (hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) != HD_OK) {
		func_ret = HD_ERR_SYS;
		goto va2pa_err;
	}
	if (vir_meminfo.pa != pa1+va1_offset) {
		printf("err:vir_meminfo.pa = 0x%x != 0x%x\r\n", (int)vir_meminfo.pa, (int)(pa1+va1_offset));
		func_ret = HD_ERR_SYS;
		goto va2pa_err;
	}
	vir_meminfo.va = (void *)((UINT32)va2+va2_offset);
	if (hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) != HD_OK) {
		func_ret = HD_ERR_SYS;
		goto va2pa_err;
	}
	if (vir_meminfo.pa != pa2+va2_offset) {
		printf("err:vir_meminfo.pa = 0x%x != 0x%x\r\n", (int)vir_meminfo.pa, (int)(pa2+va2_offset));
		func_ret = HD_ERR_SYS;
		goto va2pa_err;
	}
va2pa_err:
	hd_common_mem_munmap((void*)va2, size);
map_err:
	ret = hd_common_mem_free(pa1, (void *)va1);
	if (ret != HD_OK) {
		printf("err:free pa1 = 0x%x, va1 = 0x%x\r\n", (int)pa1, (int)va1);
	}
	return func_ret;
}

int test_timestamp(UINT32 count)
{
	UINT64     time64_old = 0, time64 = 0;
	UINT64    *p_time64_list;
	UINT64     time32_old = 0, time32 = 0;
	UINT32    *p_time32_list;
	char      *p_buf;
	int        ret = HD_OK;
	UINT32     i;

	p_buf = malloc(count * sizeof(UINT64));
	if (p_buf == NULL)
		return HD_ERR_SYS;
	// test us
	p_time64_list = (UINT64 *)p_buf;
	for (i = 0; i < count; i++) {
		time64 = hd_gettime_us();
		if (time64 < time64_old) {
			printf("error time = %lld us, prev time = %lld us\r\n", time64, time64_old);
			ret = HD_ERR_SYS;
			goto err_time;
		}
		*p_time64_list++ = time64;
		time64_old = time64;
	}
	p_time64_list = (UINT64 *) p_buf;
	for (i = 0; i < count; i++) {
		printf("%d time = %lld us\r\n", (int)i, *p_time64_list++);
	}
	// test ms
	p_time32_list = (UINT32 *)p_buf;
	for (i = 0; i < count; i++) {
		time32 = hd_gettime_ms();
		if (time32 < time32_old) {
			printf("error time = %d ms, prev time = %d ms\r\n", (int)time32, (int)time32_old);
			ret = HD_ERR_SYS;
			goto err_time;
		}
		*p_time32_list++ = time32;
		time32_old = time32;
	}
	p_time32_list = (UINT32 *) p_buf;
	for (i = 0; i < count; i++) {
		printf("%d time = %d ms\r\n", (int)i, (int)*p_time32_list++);
	}

err_time:
	free(p_buf);
	return ret;
}


int test_cacheflush_perf(void)
{
	void                 *va;
	UINT32               pa, i;
	UINT32               size = 0x2000000;
	UINT32               flush_size;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_RESULT            ret;
	UINT64               time_b, time_e;

	printf("\r\ntest_cacheflush_perf\r\n");
	ret = hd_common_mem_alloc("test_1", &pa, (void **)&va, size, ddr_id);
	if (ret != HD_OK) {
		printf("err:alloc size 0x%x, ddr %d\r\n", (int)size, ddr_id+1);
		return HD_ERR_NG;
	}
	flush_size = 0x08000;
	for (i = 0; i < 15; i++) {
		memset(va, 0x00, 0x40000);
		time_b = hd_gettime_us();
		hd_common_mem_flush_cache((void*)va, flush_size);
		time_e = hd_gettime_us();
		printf("cacheflush size 0x%x, time = %lld us\r\n", (int)flush_size, time_e-time_b);
		flush_size = flush_size << 1;
		if (flush_size > size) {
			break;
		}
	}
	flush_size = 0x08000;
	for (i = 0; i < 15; i++) {
		memset(va, 0x00, 0x40000);
		time_b = hd_gettime_us();
		vendor_common_mem_cache_sync((void*)va, flush_size, VENDOR_COMMON_MEM_DMA_TO_DEVICE);
		time_e = hd_gettime_us();
		printf("cacheclean size 0x%x, time = %lld us\r\n", (int)flush_size, time_e-time_b);
		flush_size = flush_size << 1;
		if (flush_size > size) {
			break;
		}
	}
	flush_size = 0x08000;
	for (i = 0; i < 15; i++) {
		memset(va, 0x00, 0x40000);
		time_b = hd_gettime_us();
		vendor_common_mem_cache_sync((void*)va, flush_size, VENDOR_COMMON_MEM_DMA_FROM_DEVICE);
		time_e = hd_gettime_us();
		printf("cacheinvalid size 0x%x, time = %lld us\r\n", (int)flush_size, time_e-time_b);
		flush_size = flush_size << 1;
		if (flush_size > size) {
			break;
		}
	}
	ret = hd_common_mem_free(pa, (void *)va);
	if (ret != HD_OK) {
		printf("err:free pa = 0x%x, va = 0x%x\r\n", (int)pa, (int)va);
	}
	return HD_OK;
}

int test_cacheflush_all_perf(void)
{
	void                 *va;
	UINT32               pa, i;
	UINT32               size = 0x2000000;
	UINT32               flush_size;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_RESULT            ret;
	UINT64               time_b, time_e;

	#if defined(__LINUX)
	{
		cpu_set_t mask;
		int curr_bind_cpu;
		int cpu_id = 0;

		curr_bind_cpu = cpu_id;
		CPU_ZERO(&mask);
		CPU_SET(curr_bind_cpu, &mask);
		if (sched_setaffinity(0, sizeof(mask), &mask) == -1)  {
			printf("sched_setaffinity fail %d\r\n", cpu_id);
			return 0;
		}
		sleep(1);

		printf("This system has %d processors configured and "
               "%d processors available.\n",
               get_nprocs_conf(), get_nprocs());
	}
	#endif


	printf("\r\ntest_cacheflush_all_perf\r\n");
	ret = hd_common_mem_alloc("test_1", &pa, (void **)&va, size, ddr_id);
	if (ret != HD_OK) {
		printf("err:alloc size 0x%x, ddr %d\r\n", (int)size, ddr_id+1);
		return HD_ERR_NG;
	}
	// set data dirty and check flush all performance
	flush_size = 0x08000;
	for (i = 0; i < 15; i++) {
		memset(va, 0x00, 0x40000);
		time_b = hd_gettime_us();
		vendor_common_mem_cache_sync_all((void*)va, flush_size, VENDOR_COMMON_MEM_DMA_BIDIRECTIONAL);
		time_e = hd_gettime_us();
		printf("cacheflush_all data dirty size 0x%x, time = %lld us\r\n", (int)flush_size, time_e-time_b);
		flush_size = flush_size << 1;
		if (flush_size > size) {
			break;
		}
	}
	flush_size = 0x08000;
	for (i = 0; i < 15; i++) {
		memset(va, 0x00, 0x40000);
		time_b = hd_gettime_us();
		vendor_common_mem_cache_sync_all((void*)va, flush_size, VENDOR_COMMON_MEM_DMA_TO_DEVICE);
		time_e = hd_gettime_us();
		printf("cacheclean_all data dirty size 0x%x, time = %lld us\r\n", (int)flush_size, time_e-time_b);
		flush_size = flush_size << 1;
		if (flush_size > size) {
			break;
		}
	}
	flush_size = 0x08000;
	for (i = 0; i < 15; i++) {
		memset(va, 0x00, 0x40000);
		time_b = hd_gettime_us();
		vendor_common_mem_cache_sync_all((void*)va, flush_size, VENDOR_COMMON_MEM_DMA_FROM_DEVICE);
		time_e = hd_gettime_us();
		printf("cacheinvalid_all data dirty size 0x%x, time = %lld us\r\n", (int)flush_size, time_e-time_b);
		flush_size = flush_size << 1;
		if (flush_size > size) {
			break;
		}
	}
	// data not dirty and check flush all performance
	flush_size = 0x08000;
	for (i = 0; i < 15; i++) {
		time_b = hd_gettime_us();
		vendor_common_mem_cache_sync_all((void*)va, flush_size, VENDOR_COMMON_MEM_DMA_BIDIRECTIONAL);
		time_e = hd_gettime_us();
		printf("cacheflush_all data not dirty size 0x%x, time = %lld us\r\n", (int)flush_size, time_e-time_b);
		flush_size = flush_size << 1;
		if (flush_size > size) {
			break;
		}
	}
	flush_size = 0x08000;
	for (i = 0; i < 15; i++) {
		time_b = hd_gettime_us();
		vendor_common_mem_cache_sync_all((void*)va, flush_size, VENDOR_COMMON_MEM_DMA_TO_DEVICE);
		time_e = hd_gettime_us();
		printf("cacheclean_all data not dirty size 0x%x, time = %lld us\r\n", (int)flush_size, time_e-time_b);
		flush_size = flush_size << 1;
		if (flush_size > size) {
			break;
		}
	}
	flush_size = 0x08000;
	for (i = 0; i < 15; i++) {
		time_b = hd_gettime_us();
		vendor_common_mem_cache_sync_all((void*)va, flush_size, VENDOR_COMMON_MEM_DMA_FROM_DEVICE);
		time_e = hd_gettime_us();
		printf("cacheinvalid_all data not dirty size 0x%x, time = %lld us\r\n", (int)flush_size, time_e-time_b);
		flush_size = flush_size << 1;
		if (flush_size > size) {
			break;
		}
	}
	ret = hd_common_mem_free(pa, (void *)va);
	if (ret != HD_OK) {
		printf("err:free pa = 0x%x, va = 0x%x\r\n", (int)pa, (int)va);
	}
	return HD_OK;
}


static int mem_init_with_same_type_and_size(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = 0x200000;
	mem_cfg.pool_info[0].blk_cnt = 2;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = 0x300000;
	mem_cfg.pool_info[1].blk_cnt = 2;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_OSG_POOL;
	mem_cfg.pool_info[2].blk_size = 0x100000;
	mem_cfg.pool_info[2].blk_cnt = 4;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;
	mem_cfg.pool_info[3].type = HD_COMMON_MEM_OSG_POOL;
	mem_cfg.pool_info[3].blk_size = 0x100000;
	mem_cfg.pool_info[3].blk_cnt = 4;
	mem_cfg.pool_info[3].ddr_id = DDR_ID0;


	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
	}
	return ret;
}

int test_err(void)
{
	HD_RESULT ret;
	void      *map_addr;
	HD_COMMON_MEM_VB_BLK  blk;
	void                 *va = NULL;
	UINT32                pa;

	mem_exit();
	ret = hd_common_uninit();
	if(ret != HD_OK) {
		printf("common_uninit fail=%d\n", ret);
		return HD_ERR_NG;
	}
	map_addr = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, (UINT32)0x100000 , (UINT32)0x100000);
	if (map_addr != NULL) {
		printf("munmap err handling fail\r\n");
		hd_common_mem_munmap(map_addr, 0x100000);
		return HD_ERR_NG;
	}
	ret = hd_common_mem_munmap((void *)0x100000 , 0x100000);
	if (ret >= 0) {
		printf("munmap err handling fail\r\n");
		return HD_ERR_NG;
	}
	blk = hd_common_mem_get_block(HD_COMMON_MEM_OSG_POOL, 0x100000, DDR_ID0);
	if (blk != HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get_block err handling fail\r\n");
		ret = hd_common_mem_release_block(blk);
		if (ret >= 0) {
			printf("rel_block err handling fail\r\n");
		}
		return HD_ERR_NG;
	}
	ret = hd_common_mem_release_block(blk);
	if (ret >= 0) {
		printf("rel_block err handling fail\r\n");
		return HD_ERR_NG;
	}
	ret = hd_common_init(g_pid);
	if(ret != HD_OK) {
		printf("init fail=%d\r\n", ret);
		return HD_ERR_NG;
	}
	ret = mem_init_with_same_type_and_size();
	if (ret >= 0) {
		printf("mem init handling fail\r\n");
		return HD_ERR_NG;
	}
	ret = hd_common_uninit();
	if(ret != HD_OK) {
		printf("common_uninit fail=%d\n", ret);
		return HD_ERR_NG;
	}
	ret = hd_common_init(g_pid);
	if(ret != HD_OK) {
		printf("init fail=%d\r\n", ret);
		return HD_ERR_NG;
	}
	mem_init(g_pid);
	ret = hd_common_mem_alloc("dspmmz", NULL, (void **)&va, 0x100000, DDR_ID0);
	if (HD_OK == ret) {
		printf("mem alloc ,phy_addr error handling fail\r\n");
		return HD_ERR_NG;
	}
	ret = hd_common_mem_alloc("dspmmz", &pa, NULL, 0x100000, DDR_ID0);
	if (HD_OK == ret) {
		printf("mem alloc ,virt_addr error handling fail\r\n");
		return HD_ERR_NG;
	}
	ret = hd_common_mem_alloc("dspmmz", &pa, (void **)&va, 0x40000000, DDR_ID0);
	if (HD_OK == ret) {
		printf("mem alloc , size error handling fail\r\n");
		system("cat /proc/hdal/comm/info");
		ret = hd_common_mem_free(pa, va);
		if (ret != HD_OK) {
			printf("err:free pa = 0x%x, va = 0x%x\r\n", (int)pa, (int)va);
		}
		return HD_ERR_NG;
	}
	ret = hd_common_mem_free((UINT32)NULL, NULL);
    if (HD_OK == ret) {
        printf("Error handle test for free an invalid address: fail %d\r\n", (int)(ret));
        return HD_ERR_NG;
    }
	return HD_OK;
}

int test_random_alloc_free(HD_COMMON_MEM_DDR_ID ddr_id)
{
	void                 *va[10];
	UINT32               pa[10], size[10] = { 0x200000, 0x400000, 0x450000, 0x210000, 0x600000, 0x3000000, 0x100, 0x9000, 0x50000, 0x500000};
	int                  i;
	HD_RESULT            ret;
	char                 tmp_str[10];

	printf("\r\test_random_alloc_free\r\n");

	for (i = 0;i < 10; i++) {
		sprintf(tmp_str, "test%d", i);
		ret = hd_common_mem_alloc(tmp_str, &pa[i], (void **)&va[i], size[i], ddr_id);
		if (ret != HD_OK) {
			printf("err:alloc size 0x%x, ddr %d\r\n", (int)size[i], ddr_id+1);
			return HD_ERR_NG;
		}
	}
	system("cat /proc/hdal/comm/info");

	ret = hd_common_mem_free(pa[5], va[5]);
	if (HD_OK != ret) {
        printf("Error free \r\n");
        return HD_ERR_NG;
    }
	ret = hd_common_mem_free(pa[6], va[6]);
	if (HD_OK != ret) {
        printf("Error free \r\n");
        return HD_ERR_NG;
    }
	ret = hd_common_mem_free(pa[7], va[7]);
	if (HD_OK != ret) {
        printf("Error free \r\n");
        return HD_ERR_NG;
    }

	system("cat /proc/hdal/comm/info");
	size[5] = 0x4000000;
	size[6] = 0x100000;
	size[7] = 0x8000;
	for (i = 5;i <= 7; i++) {
		sprintf(tmp_str, "test%d", i);
		ret = hd_common_mem_alloc(tmp_str, &pa[i], (void **)&va[i], size[i], ddr_id);
		if (ret != HD_OK) {
			printf("err:alloc size 0x%x, ddr %d\r\n", (int)size[i], ddr_id+1);
			return HD_ERR_NG;
		}
	}
	system("cat /proc/hdal/comm/info");
	ret = hd_common_mem_free(pa[5], va[5]);
	if (HD_OK != ret) {
        printf("Error free \r\n");
        return HD_ERR_NG;
    }
	ret = hd_common_mem_free(pa[6], va[6]);
	if (HD_OK != ret) {
        printf("Error free \r\n");
        return HD_ERR_NG;
    }
	ret = hd_common_mem_free(pa[7], va[7]);
	if (HD_OK != ret) {
        printf("Error free \r\n");
        return HD_ERR_NG;
    }
	system("cat /proc/hdal/comm/info");
	size[5] = 0x1000000;
	size[6] = 0x70000;
	size[7] = 0x9000;
	for (i = 5;i <= 7; i++) {
		sprintf(tmp_str, "test%d", i);
		ret = hd_common_mem_alloc(tmp_str, &pa[i], (void **)&va[i], size[i], ddr_id);
		if (ret != HD_OK) {
			printf("err:alloc size 0x%x, ddr %d\r\n", (int)size[i], ddr_id+1);
			return HD_ERR_NG;
		}
	}
	system("cat /proc/hdal/comm/info");

	ret = hd_common_mem_free(pa[5], va[5]);
	if (HD_OK != ret) {
        printf("Error free \r\n");
        return HD_ERR_NG;
    }
	ret = hd_common_mem_free(pa[6], va[6]);
	if (HD_OK != ret) {
        printf("Error free \r\n");
        return HD_ERR_NG;
    }
	ret = hd_common_mem_free(pa[7], va[7]);
	if (HD_OK != ret) {
        printf("Error free \r\n");
        return HD_ERR_NG;
    }
	system("cat /proc/hdal/comm/info");
	size[5] = 0x2000000;
	size[6] = 0x100000;
	size[7] = 0x19000;
	for (i = 5;i <= 7; i++) {
		sprintf(tmp_str, "test%d", i);
		ret = hd_common_mem_alloc(tmp_str, &pa[i], (void **)&va[i], size[i], ddr_id);
		if (ret != HD_OK) {
			printf("err:alloc size 0x%x, ddr %d\r\n", (int)size[i], ddr_id+1);
			return HD_ERR_NG;
		}
	}
	system("cat /proc/hdal/comm/info");

	ret = hd_common_mem_free(pa[5], va[5]);
	if (HD_OK != ret) {
        printf("Error free \r\n");
        return HD_ERR_NG;
    }
	ret = hd_common_mem_free(pa[6], va[6]);
	if (HD_OK != ret) {
        printf("Error free \r\n");
        return HD_ERR_NG;
    }
	ret = hd_common_mem_free(pa[7], va[7]);
	if (HD_OK != ret) {
        printf("Error free \r\n");
        return HD_ERR_NG;
    }
	system("cat /proc/hdal/comm/info");
	size[5] = 0x3100000;
	size[6] = 0x40000;
	size[7] = 0x569000;
	for (i = 5;i <= 7; i++) {
		sprintf(tmp_str, "test%d", i);
		ret = hd_common_mem_alloc(tmp_str, &pa[i], (void **)&va[i], size[i], ddr_id);
		if (ret != HD_OK) {
			printf("err:alloc size 0x%x, ddr %d\r\n", (int)size[i], ddr_id+1);
			return HD_ERR_NG;
		}
	}
	system("cat /proc/hdal/comm/info");
	for (i = 0;i < 10; i++) {
		ret = hd_common_mem_free(pa[i], va[i]);
		if (HD_OK != ret) {
	        printf("Error free \r\n");
	        return HD_ERR_NG;
    	}
	}
	system("cat /proc/hdal/comm/info");

	return HD_OK;
}

int test_get_bridge_mem(void)
{
	#define BRIDGE_FOURCC 0x47445242 ///< MAKEFOURCC('B', 'R', 'D', 'G');
	#define BRIDGE_MAX_OPT_CNT 128
	// bridge memory description !! (DO NOT MODIFY ANY MEMBER IN BRIDGE_DESC and BRIDGE_BOOT_OPTION)
	typedef struct _BRIDGE_BOOT_OPTION {
		unsigned int tag; //a fourcc tag
		unsigned int val; //the value
	} BRIDGE_BOOT_OPTION;

	typedef struct _BRIDGE_DESC {
		unsigned int bridge_fourcc;     ///< always BRIDGE_FOURCC
		unsigned int bridge_size;       ///< sizeof(BRIDGE_DESC) for check if struct match on rtos and linux
		unsigned int phy_addr;          ///< address of bridge memory described on fdt
		unsigned int phy_size;          ///< size of whole bridge memory described on fdt
		BRIDGE_BOOT_OPTION opts[BRIDGE_MAX_OPT_CNT]; ///< boot options from rtos
	} BRIDGE_DESC;

	int i;
	BRIDGE_DESC *p_bridge = NULL;
	VENDOR_COMM_BRIDGE_MEM bridge_mem = {0};
	if (HD_OK != vendor_common_mem_get(VENDOR_COMMON_MEM_ITEM_BRIDGE_MEM, &bridge_mem)) {
		return HD_ERR_NG;
	}

	p_bridge = (BRIDGE_DESC *)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, bridge_mem.phys_addr, bridge_mem.size);

	if (p_bridge == NULL) {
		return HD_ERR_NG;
	}
	if (p_bridge->bridge_fourcc != BRIDGE_FOURCC) {
		printf("invalid bridge format.\n");
		hd_common_mem_munmap((void*)p_bridge, bridge_mem.size);
		return HD_ERR_BAD_DATA;
	}
	if (p_bridge->bridge_size != sizeof(BRIDGE_DESC)) {
		printf("invalid bridge version, size not matched rtos(%d)!=linux(%d).\n", p_bridge->bridge_size, sizeof(BRIDGE_DESC));
		hd_common_mem_munmap((void*)p_bridge, bridge_mem.size);
		return HD_ERR_BAD_DATA;
	}

	// find a tag
	#define SENSOR_PRESET_EXPT 0x54455053 //MAKEFOURCC('S', 'P', 'E', 'T')
	for (i = 0; i < BRIDGE_MAX_OPT_CNT; i++) {
		if (p_bridge->opts[i].tag == SENSOR_PRESET_EXPT) {
			printf("find tag SENSOR_PRESET_EXPT = 0x%08X\n", p_bridge->opts[i].val);
			break;
		} else if (p_bridge->opts[i].tag == 0) {
			printf("unable to find tag: 0x%08X\n", SENSOR_PRESET_EXPT);
			break;
		}
	}
	hd_common_mem_munmap((void*)p_bridge, bridge_mem.size);
	return HD_OK;
}


int test_alloc_maxblk(HD_COMMON_MEM_DDR_ID ddr_id)
{
	void                 *va;
	UINT32               pa;
	UINT32               size;
	HD_RESULT            ret;
	VENDOR_COMM_MAX_FREE_BLOCK max_free = {0};
	VENDOR_COMM_FREE_SIZE      free = {0};


	printf("\r\ntest_alloc_maxblk\r\n");
	system("cat /proc/hdal/comm/info");
	free.ddr = ddr_id;
	if (HD_OK != vendor_common_mem_get(VENDOR_COMMON_MEM_ITEM_FREE_SIZE, &free)) {
		return HD_ERR_NG;
	}
	printf("\r\nfree = 0x%x\r\n", (int)free.size);
	max_free.ddr = ddr_id;
	if (HD_OK != vendor_common_mem_get(VENDOR_COMMON_MEM_ITEM_MAX_FREE_BLOCK_SIZE, &max_free)) {
		return HD_ERR_NG;
	}
	size = max_free.size;
	ret = hd_common_mem_alloc("test_1", &pa, (void **)&va, size, ddr_id);
	if (ret != HD_OK) {
		printf("err:alloc size 0x%x, ddr %d\r\n", (int)size, ddr_id+1);
		return HD_ERR_NG;
	}
	system("cat /proc/hdal/comm/info");
	ret = hd_common_mem_free(pa, (void *)va);
	if (ret != HD_OK) {
		printf("err:free pa = 0x%x, va = 0x%x\r\n", (int)pa, (int)va);
	}
	return HD_OK;
}

int test_get_common_pool_range(HD_COMMON_MEM_DDR_ID ddr_id)
{
	VENDOR_COMM_POOL_RANGE pool_range = {0};
	void                   *va;

	pool_range.ddr = ddr_id;
	if (HD_OK != vendor_common_mem_get(VENDOR_COMMON_MEM_ITEM_COMM_POOL_RANGE, &pool_range)) {
		return HD_ERR_NG;
	}
	printf("pool_range pa= 0x%x, size=0x%x\r\n", (int)pool_range.phys_addr, (int)pool_range.size);
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pool_range.phys_addr, pool_range.size);
	if (va == NULL) {
		return HD_ERR_NG;
	}
	printf("pool_range va= 0x%x\r\n", (int)va);
	hd_common_mem_munmap(va, pool_range.size);
	return HD_OK;
}

int test_ddr_monitor(HD_COMMON_MEM_DDR_ID ddr_id)
{
	VENDOR_COMM_DDR_MONITOR_ID   mon_ddr;
	VENDOR_COMM_DDR_MONITOR_DATA mon_data;
	char *buf1, *buf2;
	UINT32  size = 0x100000, bufsize = 0x400000;

	buf1 = malloc(bufsize);
	buf2 = malloc(bufsize);
	mon_ddr.ddr = ddr_id;
	printf("start ddr mon test\r\n");
	if (HD_OK != vendor_common_mem_set(VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_START, &mon_ddr)) {
		goto monitor_err;
	}
	mon_data.ddr = ddr_id;
	if (HD_OK != vendor_common_mem_get(VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_DATA, &mon_data)) {
		goto monitor_err;
	}
	sleep(1);
	printf("ddr mon data count = %lld, byte = %lld\r\n", mon_data.cnt, mon_data.byte);
	printf("memcpy 1M bytes \r\n");
	memcpy(buf1, buf2, size);
	sleep(1);
	if (HD_OK != vendor_common_mem_get(VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_DATA, &mon_data)) {
		goto monitor_err;
	}
	printf("ddr mon data count = %lld, byte = %lld\r\n", mon_data.cnt, mon_data.byte);
	if (HD_OK != vendor_common_mem_set(VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_RESET, &mon_ddr)) {
		goto monitor_err;
	}
	printf("reset\r\n");
	printf("memcpy 1M bytes \r\n");
	memcpy(buf1, buf2, size);
	sleep(1);
	if (HD_OK != vendor_common_mem_get(VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_DATA, &mon_data)) {
		goto monitor_err;
	}
	printf("ddr mon data count = %lld, byte = %lld\r\n", mon_data.cnt, mon_data.byte);
	if (HD_OK != vendor_common_mem_set(VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_RESET, &mon_ddr)) {
		goto monitor_err;
	}
	printf("reset\r\n");
	printf("memcpy 2M bytes \r\n");
	size = 0x200000;
	memcpy(buf1, buf2, size);
	sleep(1);
	if (HD_OK != vendor_common_mem_get(VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_DATA, &mon_data)) {
		goto monitor_err;
	}
	printf("ddr mon data count = %lld, byte = %lld\r\n", mon_data.cnt, mon_data.byte);
	printf("stop\r\n");
	if (HD_OK != vendor_common_mem_set(VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_STOP, &mon_ddr)) {
		goto monitor_err;
	}
	if (HD_OK != vendor_common_mem_get(VENDOR_COMMON_MEM_ITEM_DDR_MONITOR_DATA, &mon_data)) {
		goto monitor_err;
	}
	printf("ddr mon data count = %lld, byte = %lld\r\n", mon_data.cnt, mon_data.byte);
	free(buf1);
	free(buf2);
	return HD_OK;
monitor_err:
	free(buf1);
	free(buf2);
	return HD_ERR_NG;
}

#if TEST_FDT
int parse_sys_dtb(UINT32 ai_dram1[2], UINT32 ai_dram2[2])
{
	char *path_dtb = "/sys/firmware/fdt";
	FILE *fin;
	void *fdt;
	struct stat st;
	int i, len, n;
	int nodeoffset;
	size_t fdt_size;
	const void *nodep;
	unsigned int *p_reg;

	ai_dram1[0] = 0;
	ai_dram1[1] = 0;
	ai_dram2[0] = 0;
	ai_dram2[1] = 0;
    fin = fopen(path_dtb, "rb");
    if (!fin) {
		fprintf(stderr, "unable to open %s.\n", path_dtb);
		return -1;
    }

    if(stat(path_dtb, &st) != 0) {
		fprintf(stderr, "unable to stat %s.\n", path_dtb);
		fclose(fin);
		return -1;
    }
    fdt = (void *)malloc(st.st_size);
    if (fdt == NULL) {
		fprintf(stderr, "memory is not enough.\n");
		fclose(fin);
		return -1;
    }

    fdt_size = fread(fdt, 1, st.st_size, fin);
    if ((int)fdt_size != st.st_size) {
            fprintf(stderr, "unable to read %s %d %d.\n", path_dtb, (int)fdt_size, (int)st.st_size);
            free(fdt);
            fclose(fin);
            return -1;
    }
    fclose(fin);
	nodeoffset = fdt_path_offset(fdt, "/ai-memory/dram1");
	if (nodeoffset >= 0) {
		nodep = fdt_getprop(fdt, nodeoffset, "reg", &len);
		if (len) {
			n = len / sizeof(unsigned int);
			if (n!=2) {
				free(fdt);
				printf("ERR: dram1 reg set error \r\n");
				return -1;
			}
			p_reg = (unsigned int *)nodep;
			printf("dram1 reg: ");
			for (i = 0; i < n; i++) {
				ai_dram1[i] = be32_to_cpu(p_reg[i]);
				printf("0x%x ", ai_dram1[i]);

			}
			printf("\r\n");
		}
	}
	nodeoffset = fdt_path_offset(fdt, "/ai-memory/dram2");
	if (nodeoffset >= 0) {
		nodep = fdt_getprop(fdt, nodeoffset, "reg", &len);
		if (len == 0) {
			free(fdt);
			return -1;
		}
		n = len / sizeof(unsigned int);
		if (n!=2) {
			free(fdt);
			printf("ERR: dram2 reg set error \r\n");
			return -1;
		}
		p_reg = (unsigned int *)nodep;
		printf("dram2 reg: ");
		for (i = 0; i < n; i++) {
			ai_dram2[i] = be32_to_cpu(p_reg[i]);
			printf("0x%x ", ai_dram2[i]);
		}
		printf("\r\n");
	}
	free(fdt);
    return 0;
}


int test_ai_memory(void)
{
	UINT32               ai_dram1[2], ai_dram2[2];
	void                 *va1 = NULL, *va2 = NULL;
	UINT32               pa1, pa2;
	UINT32               size1, size2;
	UINT32               va1_offset = 0x1010, va2_offset = 0x2020;
	UINT32               test_size1 = 0x100000, test_size2 = 0x100000;
	HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};
	int                         ret;

	ret = parse_sys_dtb(ai_dram1, ai_dram2);
	if (ret < 0) {
		printf("ERR: No ai memory setting on dts\r\n");
		return HD_ERR_SYS;
	}
	pa1 = ai_dram1[0];
	size1 = ai_dram1[1];
	if (pa1) {
		va1 = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa1, size1);
		if (va1 == NULL) {
			ret = HD_ERR_SYS;
			goto map_err;
		}
		printf("pa1 = 0x%x, va1 = 0x%x\r\n", (int)pa1, (int)va1);
		vir_meminfo.va = (void *)((UINT32)va1+va1_offset);
		if (hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) != HD_OK) {
			ret = HD_ERR_SYS;
			goto va2pa_err;
		}
		if (vir_meminfo.pa != pa1+va1_offset) {
			printf("err:vir_meminfo.pa = 0x%x != 0x%x\r\n", (int)vir_meminfo.pa, (int)(pa1+va1_offset));
			ret = HD_ERR_SYS;
			goto va2pa_err;
		}
		// test memcpy
		if (size1/2 < test_size1) {
			test_size1 = size1/2;
		}
		printf("test memcpy dst = 0x%x, src = 0x%x, size= 0x%x\r\n", (int)va1, (int)va1+test_size1, (int) test_size1);
		memcpy(va1, va1+test_size1, test_size1);
	}
	pa2 = ai_dram2[0];
	size2 = ai_dram2[1];
	if (pa2) {
		va2 = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa2, size2);
		if (va2 == NULL) {
			ret = HD_ERR_SYS;
			goto map_err;
		}
		printf("pa2 = 0x%x, va2 = 0x%x\r\n", (int)pa2, (int)va2);
		vir_meminfo.va = (void *)((UINT32)va2+va2_offset);
		if (hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo) != HD_OK) {
			ret = HD_ERR_SYS;
			goto va2pa_err;
		}
		if (vir_meminfo.pa != pa2+va2_offset) {
			printf("err:vir_meminfo.pa = 0x%x != 0x%x\r\n", (int)vir_meminfo.pa, (int)(pa2+va2_offset));
			ret = HD_ERR_SYS;
			goto va2pa_err;
		}
		if (size2/2 < test_size2) {
			test_size2 = size2/2;
		}
		printf("test memcpy dst = 0x%x, src = 0x%x, size= 0x%x\r\n", (int)va2, (int)va2+test_size2, (int) test_size2);
		memcpy(va2, va2+test_size2, test_size2);
	}
map_err:
va2pa_err:
	if (va2) {
		hd_common_mem_munmap((void*)va2, size2);
	}
	if (va1) {
		hd_common_mem_munmap((void*)va1, size1);
	}
	return ret;
}
#endif

static HD_RESULT test_map_noncache(UINT32 pa)
{
	void                 *va = NULL, *va2 = NULL;
	UINT32               size = 0x100000;
	HD_RESULT            func_ret = HD_OK;
	UINT64               time_b, time_e;
	HD_COMMON_MEM_VB_BLK blk = HD_COMMON_MEM_VB_INVALID_BLK;


	printf("\r\ntest_map_noncache 0x%x\r\n", (int)pa);
	if (pa == 0) {
		blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, size, DDR_ID0);
		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get block fail\r\n");
			return HD_ERR_NG;
		}
		printf("blk = 0x%x\r\n", (int)blk);
		pa = hd_common_mem_blk2pa(blk);
		if (pa == 0) {
			printf("blk2pa fail, blk = 0x%x\r\n", (int)blk);
			func_ret = HD_ERR_SYS;
			goto blk2pa_err;
		}
	}
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, pa, size);
	if (va == NULL) {
		func_ret = HD_ERR_SYS;
		goto map_err;
	}
	time_b = hd_gettime_us();
	memset(va, 0x11, size);
	time_e = hd_gettime_us();
	printf("memset non-cache size 0x%x, time = %lld us\r\n", (int)size, time_e-time_b);
	va2 = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, size);
	if (va2 == NULL) {
		func_ret = HD_ERR_SYS;
		goto map_err;
	}
	time_b = hd_gettime_us();
	memset(va2, 0x22, size);
	time_e = hd_gettime_us();
	printf("memset cache size 0x%x, time = %lld us\r\n", (int)size, time_e-time_b);
map_err:
	if (va) {
		hd_common_mem_munmap((void*)va, size);
	}
	if (va2) {
		hd_common_mem_munmap((void*)va2, size);
	}
blk2pa_err:
	if (blk != HD_COMMON_MEM_VB_INVALID_BLK) {
		func_ret = hd_common_mem_release_block(blk);
		if (HD_OK != func_ret) {
			printf("release blk fail %d\r\n", func_ret);
		}
	}
	return func_ret;
}


static HD_RESULT test_create_destory_pool(void)
{
	HD_COMMON_MEM_VB_BLK blk, blk2;
	UINT32            pa, va, i;
	UINT32            blk_size = 0x100000, blk_cnt = 2;
	HD_COMMON_MEM_DDR_ID ddr_id;
	HD_COMMON_MEM_DDR_ID max_ddr;
	HD_RESULT         ret, func_ret = HD_OK;
	VENDOR_COMMON_MEM_VB_POOL pool;


#if TEST_DDR2
	max_ddr = DDR_ID1;
#else
	max_ddr = DDR_ID0;
#endif
	for (ddr_id=0; ddr_id <= max_ddr; ddr_id++) {
		printf("\r\ntest_create_destory_pool ddr_id %d\r\n", ddr_id);
		pool = vendor_common_mem_create_pool("test_c", blk_size, blk_cnt, ddr_id);
		if (pool == VENDOR_COMMON_MEM_VB_INVALID_POOL) {
			printf("create pool fail\r\n");
			return HD_ERR_NG;
		}
		system("cat /proc/hdal/comm/info");
		blk = hd_common_mem_get_block(pool, blk_size, ddr_id);
		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get block fail\r\n");
			return HD_ERR_NG;
		}
		printf("blk = 0x%x\r\n", (int)blk);
		pa = hd_common_mem_blk2pa(blk);
		if (pa == 0) {
			printf("blk2pa fail, blk = 0x%x\r\n", (int)blk);
			func_ret = HD_ERR_SYS;
			goto blk2pa_err;
		}
		printf("pa = 0x%x\r\n", (int)pa);
		if (pa > 0) {
			va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, blk_size);
			if (va == 0) {
				func_ret = HD_ERR_SYS;
				goto map_err;
			}
			printf("va = 0x%x\r\n", (int)va);
			memset((void*)va, 0x11, blk_size);
			ret = hd_common_mem_flush_cache((void*)va, blk_size);
			printf("hd_common_mem_flush_cache ret = 0x%x\r\n", ret);
			hd_common_mem_munmap((void*)va, blk_size);
		}
		for (i= 0 ; i < 3; i++) {
			system("cat /proc/hdal/comm/info");
			blk2 = hd_common_mem_get_block(pool, blk_size, ddr_id);
			if (blk2 == HD_COMMON_MEM_VB_INVALID_BLK) {
				printf("get block fail\r\n");
				return HD_ERR_NG;
			}
			printf("blk2 = 0x%x\r\n", (int)blk2);
			system("cat /proc/hdal/comm/info");
			ret = hd_common_mem_release_block(blk2);
			if (HD_OK != ret) {
				printf("release blk2 fail %d\r\n", ret);
				return HD_ERR_NG;
			}
		}
	blk2pa_err:
	map_err:
		system("cat /proc/hdal/comm/info");
		ret = hd_common_mem_release_block(blk);
		if (HD_OK != ret) {
			printf("release blk fail %d\r\n", ret);
			return HD_ERR_NG;
		}
		vendor_common_mem_destroy_pool(pool);
	}
	return func_ret;
}



static HD_RESULT test_ddr_auto(void)
{
	#define BLK_CNT   6
	#define BUFF_CNT  7

	UINT32                 i, blk_size = 0x300000, alloc_size = 0x6000000;
	HD_COMMON_MEM_VB_BLK   blk[BLK_CNT], tmp_blk;
	HD_COMMON_MEM_DDR_ID   ddr_id = DDR_ID_AUTO;
	HD_RESULT              ret;
	UINT32                 pa[BUFF_CNT] = {0}, va[BUFF_CNT] = {0}, tmp_va, tmp_pa;


	printf("\r\ntest_ddr_auto\r\n");

	#if !TEST_DDR2
	printf("test Fail ! Pelease enable TEST_DDR2 !!\r\n");
	return HD_ERR_NG;
	#endif
	for (i = 0;i < BLK_CNT; i++) {
		blk[i] = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id);
		if (blk[i] == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get block fail\r\n");
			return HD_ERR_NG;
		}
		system("cat /proc/hdal/comm/info");
	}

	printf("\r\n====  Error case begin ======\r\n");
	tmp_blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id);
	if (tmp_blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get block fail\r\n");
	}
	printf("\r\n====  Error case end ======\r\n");

	for (i = 0;i < BUFF_CNT; i++) {
		ret = hd_common_mem_alloc("test_1", &pa[i], (void **)&va[i], alloc_size, ddr_id);
		if (ret != HD_OK) {
			printf("err:alloc size 0x%x, ddr %d\r\n", (int)alloc_size, ddr_id);
			return HD_ERR_NG;
		}
		system("cat /proc/hdal/comm/info");
	}

	printf("\r\n====  Error case begin ======\r\n");
	ret = hd_common_mem_alloc("test_1", &tmp_pa, (void **)&tmp_va, alloc_size, ddr_id);
	if (ret != HD_OK) {
		printf("err:alloc size 0x%x, ddr %d\r\n", (int)alloc_size, ddr_id);
	}
	printf("\r\n====  Error case end ======\r\n");


	// release buffers
	for (i = 0;i < BLK_CNT; i++) {
		ret = hd_common_mem_release_block(blk[i]);
		if (HD_OK != ret) {
			printf("release blk fail %d\r\n", ret);
			return HD_ERR_NG;
		}
	}
	for (i = 0;i < BUFF_CNT; i++) {
		ret = hd_common_mem_free(pa[i], (void *)va[i]);
		if (ret != HD_OK) {
			printf("err:free pa = 0x%x, va = 0x%x\r\n", (int)pa[i], (int)va[i]);
		}
	}
	system("cat /proc/hdal/comm/info");
	return HD_OK;
}


EXAMFUNC_ENTRY(hd_common_test, argc, argv)
{
    HD_RESULT ret;
    INT key;

	if (argc >= 2) {
		printf("param2=%s!\r\n", argv[2]);
		if (argv[1][0] == 's') {
			g_pid = 0;
			printf("running as <server>\r\n");
		} else if (argv[1][0] == 'c') {
			g_pid = 1;
			printf("running as <client>\r\n");
		} else {

		}
	}
	//init hdal
	ret = hd_common_init(g_pid);
    if(ret != HD_OK) {
		printf("init fail=%d\r\n", ret);
		goto exit;
    }
	//init memory
	ret = mem_init(g_pid);
    if(ret != HD_OK) {
		printf("init fail=%d\r\n", ret);
		goto exit;
	}
	//test get block from common pool
	ret = test_get_block_from_common();
    if(ret != HD_OK) {
		printf("test_get_block_from_common fail=%d\r\n", ret);
		goto exit;
    }
	//test get block from osg pool
	ret = test_get_block_from_osg();
    if(ret != HD_OK) {
		printf("test_get_block_from_osg fail=%d\r\n", ret);
		goto exit;
    }
	//test alloc memory
	ret = test_alloc(DDR_ID0);
    if(ret != HD_OK) {
		printf("test_alloc fail=%d\r\n", ret);
		goto exit;
    }
	#if TEST_DDR2
	ret = test_alloc(DDR_ID1);
    if(ret != HD_OK) {
		printf("test_alloc fail=%d\r\n", ret);
		goto exit;
    }
	#endif
	ret = test_alloc_temp(DDR_ID0);
	if(ret != HD_OK) {
		printf("test_alloc fail=%d\r\n", ret);
		goto exit;
    }
	//test va to pa
	ret = test_va2pa();
    if(ret != HD_OK) {
		printf("test_va2pa fail=%d\r\n", ret);
		goto exit;
    }
	// test multiple va mmap to the same physical
	ret = test_multi_va_map_same_phy();
	if(ret != HD_OK) {
		printf("test_multi_va_map_same_phy fail=%d\r\n", ret);
		goto exit;
    }
	// test cache flush performance
	ret = test_cacheflush_perf();
	if(ret != HD_OK) {
		printf("test_cacheflush_perf fail=%d\r\n", ret);
		goto exit;
    }
	// test allocate maximum block
	ret = test_alloc_maxblk(DDR_ID0);
	if(ret != HD_OK) {
		printf("test_alloc_maxblk fail=%d\r\n", ret);
		goto exit;
    }
	#if TEST_DDR2
	ret = test_alloc_maxblk(DDR_ID1);
	if(ret != HD_OK) {
		printf("test_alloc_maxblk fail=%d\r\n", ret);
		goto exit;
    }
	#endif
	// test_get_common_pool_range
	ret = test_get_common_pool_range(DDR_ID0);
	if(ret != HD_OK) {
		printf("test_get_common_pool_range fail=%d\r\n", ret);
		goto exit;
    }
	#if TEST_DDR2
	ret = test_get_common_pool_range(DDR_ID1);
	if(ret != HD_OK) {
		printf("test_get_common_pool_range fail=%d\r\n", ret);
		goto exit;
    }
	#endif
	ret = test_create_destory_pool();
	if(ret != HD_OK) {
		printf("test_create_destory_pool fail=%d\r\n", ret);
		goto exit;
    }
	while (1) {
		printf("\r\nEnter q to exit\r\n");
		printf("Enter d to enter debug menu\r\n");
		printf("Enter 1 to test alloc memory on ddr1\r\n");
		printf("Enter 2 to test alloc memory on ddr2\r\n");
		printf("Enter 3 to test timestamp\r\n");
		printf("Enter 4 to test cacheflush_perf\r\n");
		printf("Enter 5 to test err handling\r\n");
		printf("Enter 6 to test random alloc, free\r\n");
		printf("Enter 7 to test get bridge memory\r\n");
		printf("Enter 8 to test get common pool range\r\n");
		printf("Enter 9 to test ddr monitor\r\n");
		printf("Enter a to test map ai memory\r\n");
		printf("Enter b to test cacheflush_all_perf\r\n");
		printf("Enter c to test test_alloc_max_pools\r\n");
		printf("Enter e to test map non-cache memory\r\n");
		printf("Enter f to test ddr-auto\r\n");
		key = NVT_EXAMSYS_GETCHAR();
		if (key == 0xa) {
			key = NVT_EXAMSYS_GETCHAR();
		}
		if (key == 'q' || key == 0x3) {
			break;
		}
		if (key == '1') {
			ret = test_alloc(DDR_ID0);
		    if(ret != HD_OK) {
				printf("test_alloc fail=%d\r\n", ret);
				goto exit;
		    }
			continue;
		}
		if (key == '2') {
			ret = test_alloc(DDR_ID1);
		    if(ret != HD_OK) {
				printf("test_alloc fail=%d\r\n", ret);
				goto exit;
		    }
			continue;
		}
		if (key == '3') {
			ret = test_timestamp(1000);
			if(ret != HD_OK) {
				printf("test_timestamp fail=%d\r\n", ret);
				goto exit;
		    }
			printf("test_timestamp OK\r\n");
			continue;
		}
		if (key == '4') {
			ret = test_cacheflush_perf();
			if(ret != HD_OK) {
				printf("test_cacheflush_perf fail=%d\r\n", ret);
				goto exit;
		    }
			printf("test_cacheflush_perf OK\r\n");
			continue;
		}
		if (key == '5') {
			ret = test_err();
			if(ret != HD_OK) {
				printf("test_err fail\r\n");
				goto exit;
		    }
			printf("test_err OK\r\n");
			continue;
		}
		if (key == '6') {
			ret = test_random_alloc_free(DDR_ID0);
			if(ret != HD_OK) {
				printf("test_random_alloc_free fail=%d\r\n", ret);
				goto exit;
		    }
			printf("test_random_alloc_free OK\r\n");
			continue;
		}
		if (key == '7') {
			ret = test_get_bridge_mem();
			if(ret != HD_OK) {
				printf("test_get_bridge_mem fail=%d\r\n", ret);
				goto exit;
		    }
			printf("test_get_bridge_mem OK\r\n");
			continue;
		}
		if (key == '8') {
			ret = test_get_common_pool_range(DDR_ID0);
			if(ret != HD_OK) {
				printf("test_get_common_pool_range fail=%d\r\n", ret);
				goto exit;
		    }
			printf("test_get_common_pool_range OK\r\n");
			continue;
		}
		if (key == '9') {
			ret = test_ddr_monitor(DDR_ID0);
			if(ret != HD_OK) {
				printf("test_ddr_monitor fail=%d\r\n", ret);
				goto exit;
		    }
			printf("test_ddr_monitor OK\r\n");
			continue;
		}
		#if TEST_FDT
		if (key == 'a') {
			ret = test_ai_memory();
			if(ret != HD_OK) {
				sleep(1);
				printf("ERR: test_ai_memory fail=%d\r\n", ret);
				goto exit;
		    }
			printf("test_ai_memory OK\r\n");
			continue;
		}
		#endif
		if (key == 'b') {
			ret = test_cacheflush_all_perf();
			if(ret != HD_OK) {
				printf("test_cacheflush_all_perf fail=%d\r\n", ret);
				goto exit;
		    }
			printf("test_cacheflush_all_perf OK\r\n");
			continue;
		}
		if (key == 'c') {
			ret = test_alloc_max_pools(DDR_ID2);
		    if(ret != HD_OK) {
				printf("test_alloc_max_pools fail=%d\r\n", ret);
				goto exit;
		    }
			continue;
		}
		if (key == 'e') {
			UINT32 pa = 0x50000000;

			ret = test_map_noncache(pa);
		    if(ret != HD_OK) {
				printf("test_map_noncache 0x%x fail = %d\r\n", (int)pa, ret);
				goto exit;
		    }
			pa = 0x1D800000;
			ret = test_map_noncache(pa);
		    if(ret != HD_OK) {
				printf("test_map_noncache 0x%x fail = %d\r\n", (int)pa, ret);
		    }
			pa = 0;
			ret = test_map_noncache(pa);
		    if(ret != HD_OK) {
				printf("test_map_noncache 0x%x fail = %d\r\n", (int)pa, ret);
		    }
			continue;
		}
		if (key == 'f') {
			test_ddr_auto();
			continue;
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
    //uninit memory
    ret = mem_exit();
    if(ret != HD_OK) {
        printf("mem fail=%d\n", ret);
    }
    //uninit hdal
	ret = hd_common_uninit();
    if(ret != HD_OK) {
        printf("common fail=%d\n", ret);
    }
	return 0;
}
