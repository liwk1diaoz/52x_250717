/**
 * @file hd_common_mapp_test.c
 * @brief test common map APIs.
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
#define DEBUG_COMMON_INFO 	  1
#define TEST_PATTERN          0x11

#define DBG_IND(fmtstr, args...) printf(fmtstr, ##args)
#define DBG_ERR(fmtstr, args...) printf("\033[31mERR:%s(): \033[0m" fmtstr,__func__, ##args)

static int mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = 0x800000;
	mem_cfg.pool_info[0].blk_cnt = 1;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;

	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", (int)(ret));
	}
#if (DEBUG_COMMON_INFO == 1)		
	system("cat /proc/hdal/comm/info");
#endif
	return ret;
}

static HD_RESULT mem_exit(void)
{
	return hd_common_mem_uninit();
}

static HD_RESULT test_map_memory(
	UINT32 blk_size, 
	HD_COMMON_MEM_POOL_TYPE pool_type,
	HD_COMMON_MEM_MEM_TYPE mem_type, 
	HD_COMMON_MEM_DDR_ID ddr_id
	)
{
	HD_COMMON_MEM_VB_BLK blk;
	UINT32               i;
	UINT32               pa, va;
	HD_RESULT         ret, func_ret = HD_OK;

	DBG_IND("#########################################################################\r\n");
	DBG_IND("Test mapping memory: Start...\r\n");
	DBG_IND("\033[33mInput Param -- blk_size: 0x%x, pool_type: %d, mem_type: %d, ddr_id: %d\033[0m\r\n", (unsigned int)(blk_size), (int)(pool_type), (int)(mem_type), (int)(ddr_id));
#if (DEBUG_COMMON_INFO == 1)	
	system("cat /proc/hdal/comm/info");
#endif
	DBG_IND("\r\nblk_size = 0x%x\r\n", (unsigned int)(blk_size));
	blk = hd_common_mem_get_block(pool_type, blk_size, ddr_id);
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_ERR("get block fail, blk = 0x%x\r\n", (unsigned int)(blk));
		return HD_ERR_NG;
	}
	DBG_IND("blk = 0x%x\r\n", (unsigned int)(blk));
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		DBG_ERR("blk2pa fail, blk = 0x%x\r\n", (unsigned int)(blk));
		func_ret = HD_ERR_SYS;
		goto blk2pa_err;
	}
	DBG_IND("pa = 0x%x\r\n", (unsigned int)(pa));
	if (pa > 0) {
		va = (UINT32)hd_common_mem_mmap(mem_type, pa, blk_size);
		if (va == 0) {
			func_ret = HD_ERR_SYS;
			goto map_err;
		}
		DBG_IND("va = 0x%x\r\n", (unsigned int)(va));
		memset((void*)va, TEST_PATTERN, blk_size);
		DBG_IND("va = 0x%x\r\n", (unsigned int)(va));
		/* check data */
		for (i=0; i<blk_size; i++) {			
			if ( *((UINT8 *)va + i) != TEST_PATTERN ) {
				DBG_ERR("Test Pattern:%u != Virtual memory data:%u (with offset %u bytes)\r\n", (unsigned int)(TEST_PATTERN), (unsigned int)(*((UINT8 *)va + i)), (unsigned int)(i));
				goto data_err;
			}
		}
		DBG_IND("Virtual memory data with test pattern are correct!\r\n");		
data_err:
		hd_common_mem_flush_cache((void*)va, blk_size);
		hd_common_mem_munmap((void*)va, blk_size);
	}
blk2pa_err:
map_err:
#if (DEBUG_COMMON_INFO == 1)	
	system("cat /proc/hdal/comm/info");
#endif
	ret = hd_common_mem_release_block(blk);
	if (HD_OK != ret) {
		DBG_ERR("release blk fail %d\r\n", (int)(ret));
		return HD_ERR_NG;
	}
	
	DBG_IND("Test mapping memory: Done...\r\n");
	DBG_IND("#########################################################################\r\n");
	return func_ret;
}

static HD_RESULT function_test(void)
{
	HD_RESULT    ret, end_ret = 0;
	
	DBG_IND("\033[33m[FUNCTION_TEST] Map 4k memory\033[0m\r\n");
	ret = test_map_memory(
						0x1000, 
						HD_COMMON_MEM_COMMON_POOL, 
						HD_COMMON_MEM_MEM_TYPE_CACHE, 
						DDR_ID0
						); 
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }	

	DBG_IND("\033[33m[FUNCTION_TEST] Map 1M memory\033[0m\r\n");
	ret = test_map_memory(
						0x100000, 
						HD_COMMON_MEM_COMMON_POOL, 
						HD_COMMON_MEM_MEM_TYPE_CACHE, 
						DDR_ID0
						); 
	end_ret |= ret;						
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }		
	
	DBG_IND("\033[33m[FUNCTION_TEST] Map 2M memory\033[0m\r\n");
	ret = test_map_memory(
						0x200000, 
						HD_COMMON_MEM_COMMON_POOL, 
						HD_COMMON_MEM_MEM_TYPE_CACHE, 
						DDR_ID0
						); 
	end_ret |= ret;					
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }			
	
	DBG_IND("\033[33m[FUNCTION_TEST] Map 8M memory\033[0m\r\n");
	ret = test_map_memory(
						0x800000, 
						HD_COMMON_MEM_COMMON_POOL, 
						HD_COMMON_MEM_MEM_TYPE_CACHE, 
						DDR_ID0
						); 
	end_ret |= ret;					
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }			

	return end_ret;
}

static HD_RESULT test_map_error(
	UINT32 blk_size, 
	HD_COMMON_MEM_POOL_TYPE pool_type,
	HD_COMMON_MEM_MEM_TYPE mem_type, 
	HD_COMMON_MEM_DDR_ID ddr_id,
	UINT32 map_size,
	UINT8 is_map_4k_align
	)
{
	HD_COMMON_MEM_VB_BLK blk;
	UINT32               pa, va;
	HD_RESULT         ret, func_ret = HD_OK;

	DBG_IND("#########################################################################\r\n");
	DBG_IND("Test mapping memory: Start...\r\n");
	DBG_IND("\033[33mInput Param -- blk_size: 0x%x, pool_type: %d, mem_type: %d, ddr_id: %d, map_size: 0x%x, is_map_4k_align: %d\033[0m\r\n", (unsigned int)(blk_size), (int)(pool_type), (int)(mem_type), (int)(ddr_id), (unsigned int)(map_size), (int)(is_map_4k_align));
#if (DEBUG_COMMON_INFO == 1)		
	system("cat /proc/hdal/comm/info");
#endif
	DBG_IND("\r\nblk_size = 0x%x\r\n", (unsigned int)(blk_size));
	blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id);
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_ERR("get block fail, blk = 0x%x\r\n", (unsigned int)(blk));
		return HD_ERR_NG;
	}
	DBG_IND("blk = 0x%x\r\n", (unsigned int)(blk));
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		DBG_ERR("blk2pa fail, blk = 0x%x\r\n", (unsigned int)(blk));
		func_ret = HD_ERR_NG;
		goto blk2pa_err;
	}
	DBG_IND("pa = 0x%x\r\n", (unsigned int)(pa));
	if (pa > 0) {
		if (is_map_4k_align == 1) {
			va = (UINT32)hd_common_mem_mmap(mem_type, pa, map_size);
		} else {
			va = (UINT32)hd_common_mem_mmap(mem_type, pa + 0x07, map_size);
		}
		if (va == 0) {
			func_ret = HD_ERR_SYS;
			DBG_IND("\033[33mError handle test: PASS\033[0m\r\n");
			goto map_err;
		} else {
			DBG_IND("\033[33mError handle test: Fail\033[0m\r\n");
		}

		hd_common_mem_flush_cache((void*)va, blk_size);
		hd_common_mem_munmap((void*)va, blk_size);
	}
blk2pa_err:
map_err:
#if (DEBUG_COMMON_INFO == 1)	
	system("cat /proc/hdal/comm/info");
#endif
	ret = hd_common_mem_release_block(blk);
	if (HD_OK != ret) {
		DBG_ERR("release blk fail %d\r\n", (int)(ret));
		return HD_ERR_NG;
	}
	
	DBG_IND("Test mapping memory: Done...\r\n");
	DBG_IND("#########################################################################\r\n");

	return (func_ret == HD_ERR_SYS)?HD_OK:HD_ERR_SYS;
}

static HD_RESULT error_handle_test(void)
{
	HD_RESULT    ret, end_ret = 0;

#if defined(__LINUX)
	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Map address not 4K align\033[0m\r\n");
	ret = test_map_error(
						0x100000, 
						HD_COMMON_MEM_COMMON_POOL, 
						HD_COMMON_MEM_MEM_TYPE_CACHE, 
						DDR_ID0,
						0x100000,
						0
						); 
	end_ret |= ret;					
    if(ret != HD_OK) {
		DBG_ERR("error_handle_test fail=%d\r\n", (int)(ret));
    }	
#endif	
	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Map non-cache address\033[0m\r\n");
	ret = test_map_error(
						0x100000, 
						HD_COMMON_MEM_COMMON_POOL, 
						HD_COMMON_MEM_MEM_TYPE_NONCACHE, 
						DDR_ID0,
						0x100000,
						1
						); 
	end_ret |= ret;					
    if(ret != HD_OK) {
		DBG_ERR("error_handle_test fail=%d\r\n", (int)(ret));
    }	

	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Map size to 0\033[0m\r\n");
	ret = test_map_error(
						0x100000, 
						HD_COMMON_MEM_COMMON_POOL, 
						HD_COMMON_MEM_MEM_TYPE_CACHE, 
						DDR_ID0,
						0x00,
						1
						); 
	end_ret |= ret;					
    if(ret != HD_OK) {
		DBG_ERR("error_handle_test fail=%d\r\n", (int)(ret));
    }		
	
	return end_ret;
}

static HD_RESULT test_robust(
	UINT32 blk_size, 
	HD_COMMON_MEM_POOL_TYPE pool_type,
	HD_COMMON_MEM_MEM_TYPE mem_type, 
	HD_COMMON_MEM_DDR_ID ddr_id,
	UINT32 times
	)
{
	HD_COMMON_MEM_VB_BLK blk;
	UINT32               i,j;
	UINT32               pa, va;
	HD_RESULT         ret, func_ret = HD_OK;

	DBG_IND("#########################################################################\r\n");
	DBG_IND("Test mapping memory: Start...\r\n");
	DBG_IND("\033[33mInput Param -- blk_size: 0x%x, pool_type: %d, mem_type: %d, ddr_id: %d, times:%u\033[0m\r\n", (unsigned int)(blk_size), (int)(pool_type), (int)(mem_type), (int)(ddr_id), (unsigned int)(times));
#if (DEBUG_COMMON_INFO == 1)	
	system("cat /proc/hdal/comm/info");
#endif
	DBG_IND("\r\nblk_size = 0x%x\r\n", (unsigned int)(blk_size));
	blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id);
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_ERR("get block fail, blk = 0x%x\r\n", (unsigned int)(blk));
		return HD_ERR_NG;
	}
	DBG_IND("blk = 0x%x\r\n", (unsigned int)(blk));
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		DBG_ERR("blk2pa fail, blk = 0x%x\r\n", (unsigned int)(blk));
		func_ret = HD_ERR_SYS;
		goto blk2pa_err;
	}
	DBG_IND("pa = 0x%x\r\n", (unsigned int)(pa));
	if (pa > 0) {	
		for (j=0; j< times; j++) {	
			va = (UINT32)hd_common_mem_mmap(mem_type, pa, blk_size);
			if (va == 0) {
				func_ret = HD_ERR_SYS;
				goto map_err;
			}
			//DBG_IND("va = 0x%x\r\n", (unsigned int)(va));
			memset((void*)va, TEST_PATTERN, blk_size);
			//DBG_IND("va = 0x%x\r\n", (unsigned int)(va));
			/* check data */
			for (i=0; i<blk_size; i++) {			
				if ( *((UINT8 *)va + i) != TEST_PATTERN ) {
					DBG_ERR("Test Pattern:%u != Virtual memory data:%u (with offset %u bytes)\r\n", (unsigned int)(TEST_PATTERN), (unsigned int)(*((UINT8 *)va + i)), (unsigned int)(i));
					hd_common_mem_flush_cache((void*)va, blk_size);
					hd_common_mem_munmap((void*)va, blk_size);				
					func_ret = HD_ERR_SYS;
					goto map_err;
				}
			}
			//DBG_IND("Virtual memory data with test pattern are correct!\r\n");		
			hd_common_mem_flush_cache((void*)va, blk_size);
			ret = hd_common_mem_munmap((void*)va, blk_size);
			if (HD_OK != ret) {
				func_ret = HD_ERR_SYS;
				goto map_err;
			}		
		}	
	}
blk2pa_err:
map_err:
#if (DEBUG_COMMON_INFO == 1)	
	system("cat /proc/hdal/comm/info");
#endif
	ret = hd_common_mem_release_block(blk);
	if (HD_OK != ret) {
		DBG_ERR("release blk fail %d\r\n", (int)(ret));
		return HD_ERR_NG;
	}
	
	DBG_IND("Test mapping memory: Done...\r\n");
	DBG_IND("#########################################################################\r\n");
	return func_ret;
}

static HD_RESULT robust_test(void)
{
	HD_RESULT    ret;
	
	DBG_IND("\033[33m[ROBUST_TEST] Map/unmap 1000 times\033[0m\r\n");
	ret = test_robust(
					0x200000, 
					HD_COMMON_MEM_COMMON_POOL, 
					HD_COMMON_MEM_MEM_TYPE_CACHE, 
					DDR_ID0,
					1000
					); 	
    if(ret != HD_OK) {
		DBG_ERR("robust_test fail=%d\r\n", (int)(ret));
    }		
	
	return ret;
}

EXAMFUNC_ENTRY(hd_common_map_test, argc, argv)
{
    HD_RESULT ret;
    INT param;

    if (argc != 2)
    {
		DBG_ERR("Usage:Number of parameter shall be 1.\r\n");
		DBG_IND("\r\nParam 1 to do Function Test.\r\n");
		DBG_IND("Param 2 to do Error Handle\r\n");
		DBG_IND("Param 3 to do Robust Test\r\n");
		DBG_IND("Param 4 to enter debug menu\r\n");		
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
		ret = function_test();
	    if(ret != HD_OK) {
			DBG_ERR("FUNCTION_TEST FAIL =%d\r\n", (int)(ret));
	    }
		DBG_IND("FUNCTION_TEST OK\r\n");
	} else 	if (param == 2) {
		ret = error_handle_test();
	    if(ret != HD_OK) {
			DBG_ERR("ERROR_HANDLE_TEST FAIL =%d\r\n", (int)(ret));
	    }
		DBG_IND("ERROR_HANDLE_TEST OK\r\n");
	} else if (param == 3) {
		ret = robust_test();
		if(ret != HD_OK) {
			DBG_ERR("ROBUST_TEST FAIL =%d\r\n", (int)(ret));
	    }
		DBG_IND("ROBUST_TEST OK\r\n");
	}
	#if (DEBUG_MENU == 1)
	else if (param == 4) {
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
        DBG_ERR("mem fail=%d\n", (int)(ret));
    }
    //uninit hdal
	ret = hd_common_uninit();
    if(ret != HD_OK) {
        DBG_ERR("common fail=%d\n", (int)(ret));
    }
	return 0;
}
