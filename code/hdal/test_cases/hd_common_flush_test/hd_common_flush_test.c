/**
 * @file hd_common_mem_init_test.c
 * @brief test common APIs.
 * @author Vanness Yang
 * @date in the year 2019
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "hdal.h"
#include "hd_debug.h"
#include <kwrap/examsys.h>

#if defined(__LINUX)
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#include <kwrap/task.h>
#include <kwrap/util.h>
#define sleep(x)    vos_task_delay_ms(1000*x)
#define usleep(x)   vos_task_delay_us(x)
#endif
#define DEBUG_MENU 		      1
#define DEBUG_COMMON_INFO 	  0
#define TEST_PATTERN          0x11

#define DBG_IND(fmtstr, args...) printf(fmtstr, ##args)
#define DBG_ERR(fmtstr, args...) printf("\033[31mERR:%s(): \033[0m" fmtstr,__func__, ##args)

static int mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = 0x200000;
	mem_cfg.pool_info[0].blk_cnt = 3;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;

	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", (int)(ret));
	}
	return ret;
}

static HD_RESULT mem_exit(void)
{
	return hd_common_mem_uninit();
}

static HD_RESULT test_alloc(HD_COMMON_MEM_DDR_ID ddr_id, UINT32 size)
{
	UINT32               i;
	void                 *va;
	UINT32               pa;
	HD_RESULT            ret;
	char                 str[128];
	
	DBG_IND("#########################################################################\r\n");
	DBG_IND("%s: Start...\r\n", __FUNCTION__);	
	DBG_IND("\033[33mInput Param -- ddr_id: %d, size: %d\033[0m\r\n", (int)(ddr_id), (int)(size));
	
	ret = hd_common_mem_alloc("Test", &pa, (void **)&va, size, ddr_id);
	if (ret != HD_OK) {
		DBG_ERR("err:alloc size 0x%x, ddr %d\r\n", (unsigned int)(size), (int)(ddr_id+1));
		return HD_ERR_NG;
	}
	DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
	memset((void*)va, TEST_PATTERN, size);
	DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
	/* check data */
	for (i=0; i<size; i++) {			
		if ( *((UINT8 *)va + i) != TEST_PATTERN ) {
			DBG_ERR("Test Pattern:%u != Virtual memory data:%u (with offset %u bytes)\r\n", (unsigned int)(TEST_PATTERN), (unsigned int)(*((UINT8 *)va + i)), (unsigned int)(i));
			break;
		}
	}
	if (i == size) {
		DBG_IND("Virtual memory data with test pattern are correct!\r\n");	
	}
	
	DBG_IND("The pattern in the memory is 0x%x\r\n", (unsigned int)(TEST_PATTERN));
	DBG_IND("Check and verify\r\n");
//	sprintf(str, "mem r 0x%lx 0x%lx", pa, size);
//	system(str);
	sprintf(str, "mem dump 0x%lx 0x%lx /tmp/flush_0x%lx", pa, size, size);
	system(str);
		
	system("cat /proc/hdal/comm/info");
	ret = hd_common_mem_free(pa, (void *)va);
	if (ret != HD_OK) {
		DBG_ERR("err:free pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
		return HD_ERR_NG;
	}
	DBG_IND("free_mem\r\n\r\n");
	system("cat /proc/hdal/comm/info");
	
	DBG_IND("%s: Done...\r\n", __FUNCTION__);	
	DBG_IND("#########################################################################\r\n");	
	
	return HD_OK;
}

static HD_RESULT function_test(void)
{
	HD_RESULT    ret, end_ret = 0;
	
	DBG_IND("\033[33m[FUNCTION_TEST] Allocate 1 MB memory and write data and flush\033[0m\r\n");
	ret = test_alloc(DDR_ID0, 0x100000);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }		
	
	DBG_IND("\033[33m[FUNCTION_TEST] Allocate 2 MB memory and write data and flush\033[0m\r\n");
	ret = test_alloc(DDR_ID0, 0x200000);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }		
	
	return end_ret;
}

static HD_RESULT test_flush_performance(void)
{
	void                 *va;
	UINT32               pa, i;
	UINT32               size = 0x2000000;
	UINT32               flush_size;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_RESULT            ret;
	UINT64               time_b, time_e;

	DBG_IND("#########################################################################\r\n");
	DBG_IND("%s: Start...\r\n", __FUNCTION__);		

	ret = hd_common_mem_alloc("test_1", &pa, (void **)&va, size, ddr_id);
	if (ret != HD_OK) {
		DBG_ERR("err:alloc size 0x%x, ddr %d\r\n", (unsigned int)(size), (int)(ddr_id+1));
		return HD_ERR_NG;
	}
	flush_size = 0x08000;
	for (i = 0; i < 15; i++) {
		time_b = hd_gettime_us();
		hd_common_mem_flush_cache((void*)va, flush_size);
		time_e = hd_gettime_us();
		printf("cacheflush size 0x%x, time = %lld us\r\n", (unsigned int)(flush_size), (long long)(time_e-time_b));
		flush_size = flush_size << 1;
		if (flush_size > size) {
			break;
		}
	}
	ret = hd_common_mem_free(pa, (void *)va);
	if (ret != HD_OK) {
		DBG_ERR("err:free pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
	}
	
	DBG_IND("%s: Done...\r\n", __FUNCTION__);	
	DBG_IND("#########################################################################\r\n");	
	
	return HD_OK;
}

static HD_RESULT performance_test(void)
{
	HD_RESULT    ret;
	
	DBG_IND("\033[33m[PERFORMANCE_TEST] Measure flush performance\033[0m\r\n");
	ret = test_flush_performance();
    if(ret != HD_OK) {
		DBG_ERR("performance_test fail=%d\r\n", (int)(ret));
    }		

	return ret;
}

EXAMFUNC_ENTRY(hd_common_flush_test, argc, argv)
{
    HD_RESULT ret;
	INT param;

    if (argc != 2)
    {
		DBG_ERR("Usage:Number of parameter shall be 1.\r\n");
		DBG_IND("\r\nParam 1 to do Function Test.\r\n");
		DBG_IND("Param 2 to do Performance Test\r\n");
		DBG_IND("Param 3 to enter debug menu\r\n");		
        return 0;
    }
		
	param = atoi(argv[1]);
	
	//init hdal
	ret = hd_common_init(0);
    if(ret != HD_OK) {
		DBG_ERR("init fail=%d\r\n", (int)(ret));
		goto exit;
    }
	//init memory
	ret = mem_init();
    if(ret != HD_OK) {
		DBG_ERR("init fail=%d\r\n", (int)(ret));
		goto exit;
	}	

	if (param == 1) {
		DBG_IND("FUNCTION_TEST START\r\n");
		ret = function_test();
	    if(ret != HD_OK) {
			DBG_ERR("FUNCTION_TEST FAIL =%d\r\n", (int)(ret));
	    }
		DBG_IND("FUNCTION_TEST OK\r\n");
	} else 
		if (param == 2) {
		DBG_IND("PERFORMANCE_TEST START\r\n");
		ret = performance_test();
	    if(ret != HD_OK) {
			DBG_ERR("PERFORMANCE_TEST FAIL =%d\r\n", (int)(ret));
	    }
		DBG_IND("PERFORMANCE_TEST OK\r\n");
	} 
	#if (DEBUG_MENU == 1)	
	else if (param == 3) {
		// enter debug menu
		hd_debug_run_menu();
	}
	#endif
	else {
		DBG_ERR("Param (%d) is not exist.\r\n", (int)(param));
	}

exit:
    //uninit memory
    ret = mem_exit();
    if(ret != HD_OK) {
        printf("mem fail=%d\n", (int)(ret));
    }
    //uninit hdal
	ret = hd_common_uninit();
    if(ret != HD_OK) {
        DBG_ERR("common fail=%d\n", (int)(ret));
    }
	return 0;
}
